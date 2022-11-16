/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.
   
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_manager.h"

#include "core_sas.h"
#include "core_hal.h"
#include "core_error.h"
#include "core_util.h"
#include "core_expander.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
	MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);
static void ssp_error_handling_callback(MV_PVOID root_ptr, MV_Request *req);
MV_VOID ssp_wait_hard_reset(MV_PVOID dev_p, MV_PVOID tmp);

/*
 * two cases will call this function
 * 1. ssp_error_handler: original error request comes here.
 * 2. ssp_error_handling_callback:
 *    a. it's the error handling(eh) request either success or fail
 *    b. retried req will come here only if fails.
 *       if success, ssp_error_handling_callback has returned it.
 */
static MV_BOOLEAN ssp_timeout_state_machine(pl_root *root, 
        domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	domain_port *port = (domain_port *)dev->base.port;
	MV_Request *eh_req = NULL;
	MV_Request *org_req = err_ctx->error_req;
    #ifdef ASIC_WORKAROUND_WD_TO_RESET
	domain_expander *tmp_exp;
    MV_U8 i;
	#endif

	MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d state %d success 0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	/* for all following state, if eh request returns successfully,
	 * retry the original request
	 * if retry req success, already returned org by ssp_error_handling_callback */
#ifndef ENABLE_HARD_RESET_EH
	if (success == MV_TRUE) {
                /* error handling request comes back with success status */
                /* it's not retry request */
#ifdef _OS_LINUX
		if ((org_req->Org_Req_Scmd) 
			&& ((struct scsi_cmnd *)org_req->Org_Req_Scmd)->allowed
		) {
			eh_req = core_eh_retry_org_req(
					root, org_req, 
	                                (MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			core_queue_eh_req(root, eh_req);
		} else
			core_complete_error_req(root, org_req, REQ_STATUS_SUCCESS);

#else
		eh_req = core_eh_retry_org_req(
				root, org_req, 
                                (MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		core_queue_eh_req(root, eh_req);
#endif
		return MV_TRUE;
	}
#endif

	switch (err_ctx->eh_state) {
	case SAS_EH_TIMEOUT_STATE_NONE:
		/* case =
		 * a. original error request
		 * b. media error state machine got timeout */
		err_ctx->timeout_count++;

#ifdef ASIC_WORKAROUND_WD_TO_RESET
		if (((core_extension *)(root->core))->revision_id != VANIR_C2_REV) {
			pal_abort_port_running_req(root, port);

			// Wait
			core_sleep_millisecond(root->core, 10);

			// ASIC STP reset
			for (i = 0; i < dev->base.root->phy_num; i++) {
				if (dev->base.root->phy[i].port == port) {
					WRITE_PORT_PHY_CONTROL(root,
						(&dev->base.root->phy[i]),
						(SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET));
				}
			}

			// Wait
			core_sleep_millisecond(root->core, 50);

			LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
					domain_expander, base.queue_pointer) {
					pal_clean_expander_outstanding_req(root, tmp_exp);
			}
		}
#endif

#ifndef ENABLE_HARD_RESET_EH
		/* we've already cleaned the org req slot. */
		eh_req = sas_make_task_req(
			root, dev, org_req, TMF_ABORT_TASK,
			(MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_ABORT_REQUEST;
		CORE_EH_PRINT(("device %d sending abort TMF.\n",\
			dev->base.id));
		break;
#else
		if (IS_BEHIND_EXP(dev)) {
#ifdef ASIC_WORKAROUND_WD_TO_RESET
			if (((core_extension *)(root->core))->revision_id == VANIR_C2_REV)
#endif
			pal_abort_device_running_req(root, &dev->base);
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				HARD_RESET, ssp_error_handling_callback);			
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_DEVICE_RESET_WAIT;
		} else {
		/* we've already cleaned the org req slot. */
			eh_req = sas_make_task_req(
				root, dev, org_req, TMF_ABORT_TASK,
				(MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_ABORT_REQUEST;
			CORE_EH_PRINT(("device %d sending abort TMF.\n",\
				dev->base.id));
		}
		break;
		
        case SAS_EH_TIMEOUT_STATE_DEVICE_RESET_WAIT:
        	if (success == MV_TRUE) {
        		CORE_EH_PRINT(("device %d waiting after hard reset.\n", dev->base.id));
        		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_PORT_RESET;
        		MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
        		err_ctx->eh_timer = core_add_timer(root->core, 10, ssp_wait_hard_reset, dev, NULL);
        		MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
        		return MV_TRUE;
        	} else {
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_SET_DOWN;
			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d set down disk.\n",\
				dev->base.id));

			/* set down disk. after that, dont use dev any more */
			pal_set_down_error_disk(root, dev, MV_TRUE);
			return MV_TRUE;
		}
#endif

	case SAS_EH_TIMEOUT_STATE_ABORT_REQUEST:
		/* case =
		 * a. task management request comes back but failed
		 * b. retried req which is failed */

		eh_req = sas_make_task_req(
			root, dev, org_req, TMF_LOGICAL_UNIT_RESET,
			(MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_LU_RESET;
		CORE_EH_PRINT(("device %d sending logical unit reset TMF.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_LU_RESET:
		/* case =
		 * a. task management request finished but failed
		 * b. retried req which is failed */

		pal_abort_device_running_req(root, &dev->base);
		if (IS_BEHIND_EXP(dev)) {
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				LINK_RESET, ssp_error_handling_callback);

		} else {
                        /* PHY reset, no init state machine? */
			eh_req = core_make_device_reset_req(
				root, port, dev,
				(MV_ReqCompletion)ssp_error_handling_callback);
		}
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_DEVICE_RESET;
		CORE_EH_PRINT(("device %d sending device reset.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_DEVICE_RESET:
		/* case =
		 * a. device reset request is failed
		 * b. retried req which is failed */

		if (IS_BEHIND_EXP(dev)) {
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				HARD_RESET, ssp_error_handling_callback);
		} else {
			eh_req = core_make_port_reset_req(
				root, port, dev,
				(MV_ReqCompletion)ssp_error_handling_callback);
		}
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_PORT_RESET;
		CORE_EH_PRINT(("device %d sending port reset.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_PORT_RESET:
		/* case =
		 * a. port reset request failed
		 * b. retried req which is failed
		 * c. if > MAX_TIMEOUT_ALLOWED, jump to set down*/
#ifdef ENABLE_HARD_RESET_EH
		if (success == MV_TRUE) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
					root, org_req, 
                                        (MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			break;
		} else {
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_SET_DOWN;
			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d set down disk.\n",\
				dev->base.id));

			/* set down disk. after that, dont use dev any more */
			pal_set_down_error_disk(root, dev, MV_TRUE);
			return MV_TRUE;
		}
#else

#ifndef RAID_DRIVER 
			CORE_EH_PRINT(("device %d: complete req with ERROR\n",dev->base.id));
			dev->status |= DEVICE_STATUS_FROZEN;
			core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
#else
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_SET_DOWN;
			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d set down disk.\n",\
				dev->base.id));

			/* set down disk. after that, dont use dev any more */
			pal_set_down_error_disk(root, dev, MV_TRUE);
#endif

		return MV_TRUE;
#endif

	default:
		MV_ASSERT(MV_FALSE);
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root, eh_req);
	return MV_TRUE;
}

/*
 * it's called either by the ssp_error_handler
 * or ssp_error_handling_callback
 * refer to sas_timeout_state_machine
 */
static MV_BOOLEAN ssp_media_error_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	MV_Request *org_req = err_ctx->error_req;
	MV_Request *eh_req = NULL;

	MV_ASSERT(err_ctx->eh_type == EH_TYPE_MEDIA_ERROR);
	MV_ASSERT(org_req != NULL);

	/* if TIMEOUT will switch to timeout state machine */

	CORE_EH_PRINT(("device %d state %d success 0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	/* different with the timeout state machine,
	 * we make the retry as a stage because this state machine is simple
	 * retried request is returned already by callback if success */

	switch (err_ctx->eh_state) {
	case SAS_EH_MEDIA_STATE_NONE:
		/* case: error request, first step */
		MV_ASSERT(success == MV_FALSE);
		
#ifndef ENABLE_HARD_RESET_EH
		eh_req = core_eh_retry_org_req(
				root, org_req, (MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		core_queue_eh_req(root, eh_req);
		err_ctx->eh_state = SAS_EH_MEDIA_STATE_RETRY;
		break;
#else
		if (IS_BEHIND_EXP(dev)) {
			pal_abort_device_running_req(root, &dev->base);
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				HARD_RESET, ssp_error_handling_callback);			
			if (eh_req == NULL) return MV_FALSE;
			core_queue_eh_req(root, eh_req);
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_HARD_RESET_WAIT;
		} else {
			eh_req = core_eh_retry_org_req(
					root, org_req, (MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			core_queue_eh_req(root, eh_req);
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_RETRY;
		}
		break;
#endif
#ifdef ENABLE_HARD_RESET_EH
	case SAS_EH_MEDIA_STATE_HARD_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d waiting after hard reset.\n", dev->base.id));
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_AFTER_RESET;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			err_ctx->eh_timer = core_add_timer(root->core, 10, ssp_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
		} else {
			core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
		}
		break;
	case SAS_EH_MEDIA_STATE_AFTER_RESET:
		if (IS_BEHIND_EXP(dev)) {
			eh_req = core_eh_retry_org_req(
					root, org_req, (MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			core_queue_eh_req(root, eh_req);
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_RETRY;
			break;
		}		
#endif

	case SAS_EH_MEDIA_STATE_RETRY:
		/* case: retried request which is failed */
		MV_ASSERT(success == MV_FALSE);

		/* complete this request no matter success or not */
		core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
		break;

	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

	return MV_TRUE;
}

/*
 * in two cases code will come here
 * 1. the original error req is called using handler when error is first hit
 * 2. if in state machine lack of resource, 
 *    the original error request got pushed to the queue again.
 *    see the comments in the function.
 */
MV_BOOLEAN ssp_error_handler(MV_PVOID dev_p, MV_Request *req)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	struct _error_context *err_ctx = &dev->base.err_ctx;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(IS_SSP(dev));

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d status 0x%x req %p Cdb[0x%x]\n",\
                dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
                err_ctx->scsi_status, req, req->Cdb[0]));

	/* process_command can only return five status
	 * REQ_STATUS_SUCCESS, XXX_HAS_SENSE, XXX_ERROR, XXX_TIMEOUT, XXX_NO_DEVICE */
	if (err_ctx->eh_type == EH_TYPE_NONE) {
		/* the original error request comes here */
		MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
			|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
			|| (req->Scsi_Status == REQ_STATUS_TIMEOUT));
		/* no device shouldn't go to the error handling. */
		MV_ASSERT((err_ctx->error_req == NULL)
                        || (err_ctx->error_req == req)); /* if out of resource */
		err_ctx->error_req = req;

		if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
			err_ctx->eh_type = EH_TYPE_TIMEOUT;
			#if 0
			if (err_ctx->timeout_count >= MAX_TIMEOUT_ALLOWED) {
				/* will be set down */
				err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_PORT_RESET;
			} else {
				err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_NONE;
			}
			#else
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_NONE;
			#endif
                        return ssp_timeout_state_machine(root, dev, MV_FALSE);
		} else {
			err_ctx->eh_type = EH_TYPE_MEDIA_ERROR;
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_NONE;
			return ssp_media_error_state_machine(root, dev, MV_FALSE);
		}
	} else {
		/* code comes here if ssp_error_handling_callback 
                 * cannot continue because short of resource
		 * no device should be handled already 
                 * but the req is the original error request not eh req
                 * because the eh req has released already 
                 * the eh req scsi status is saved in err_ctx->scsi_status */
                MV_ASSERT(req == err_ctx->error_req);

		MV_ASSERT((err_ctx->scsi_status == REQ_STATUS_ERROR)
			|| (err_ctx->scsi_status == REQ_STATUS_HAS_SENSE)
			|| (err_ctx->scsi_status == REQ_STATUS_SUCCESS)
			|| (err_ctx->scsi_status == REQ_STATUS_TIMEOUT));

		MV_ASSERT((err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
			||(err_ctx->eh_type == EH_TYPE_TIMEOUT));

		if (err_ctx->eh_type == EH_TYPE_TIMEOUT) {
                        return ssp_timeout_state_machine(root, dev, 
                                (err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		} else {
			return ssp_media_error_state_machine(root, dev, 
                                (err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		}
	}
}
extern void sas_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
static void ssp_error_handling_callback(MV_PVOID root_ptr, MV_Request *req)
{
	pl_root *root = (pl_root *)root_ptr;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev;
	struct _error_context *err_ctx;
	MV_BOOLEAN ret = MV_TRUE;
	MV_Request *org_req;
#ifdef SUPPORT_PHY_POWER_MODE
	core_extension *core = root->core;
	domain_port *port;
	domain_phy *phy;
	MV_U32 reg = 0;
#endif
	dev = (domain_device *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
    if(dev == NULL)
    {
        req->Scsi_Status = REQ_STATUS_NO_DEVICE;
        return;
    }
    err_ctx = &dev->base.err_ctx;
    org_req = err_ctx->error_req;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(IS_SSP(dev));
	MV_ASSERT(CORE_IS_EH_REQ(ctx));
	MV_ASSERT((err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
		|| (err_ctx->eh_type == EH_TYPE_TIMEOUT));
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d eh_status 0x%x req %p Cdb[0x%x] status 0x%x\n",\
                dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
                err_ctx->scsi_status, req, req->Cdb[0], req->Scsi_Status));
#ifdef SUPPORT_PHY_POWER_MODE
	if(req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && req->Cdb[2] == CDB_CORE_RESET_PORT){
		port = dev->base.port;
		if(port && List_Empty(&port->expander_list)){
			phy = port->phy;
			if(phy && !mv_is_phy_ready(root,phy)){
				req->Scsi_Status = REQ_STATUS_NO_DEVICE;
				core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);
				//Core_Enable_PHY_Interrupt(phy);
				core->PHY_power_mode_port_enabled &= ~(1<<(root->base_phy_num + phy->asic_id));//record the device set down and disabled DIPM or HIPM to enable hot plug!
				sas_handle_unplug(root,port,phy);
				return;
			}
		}
	}
#endif
	/* internal request will be released by itself */
	if (req->Scsi_Status == REQ_STATUS_NO_DEVICE) {
		/* complete the original request */
		core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);
		goto end_point;
	} else if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
		/* no matter it's the retried req or the eh req */
		if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
			err_ctx->eh_type = EH_TYPE_TIMEOUT;
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_NONE;
		}
		/* continue the state machine */
	} else if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		if (CORE_IS_EH_RETRY_REQ(ctx)) {
			/* original req retry success, complete state machine*/
			core_complete_error_req(root, org_req, REQ_STATUS_SUCCESS);
			goto end_point;
		} else {
			/* error handling request success, run state machine */
		}
	} else if (req->Scsi_Status == REQ_STATUS_HAS_SENSE) {
		if (CORE_IS_EH_RETRY_REQ(ctx)) {
			/* copy sense code */
			MV_CopyMemory(org_req->Sense_Info_Buffer,
				req->Sense_Info_Buffer,
				MV_MIN(org_req->Sense_Info_Buffer_Length,
				req->Sense_Info_Buffer_Length));
			core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			goto end_point;
		}
	}


	MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
		|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
		|| (req->Scsi_Status == REQ_STATUS_TIMEOUT)
		|| (req->Scsi_Status == REQ_STATUS_SUCCESS));

	if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
		ret = ssp_media_error_state_machine(root, dev, 
                        (req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
	} else {
        ret = ssp_timeout_state_machine(root, dev, 
                (req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
	}

end_point:
	if (ret == MV_FALSE) {
                /* save the eh request status because req will be released */
                err_ctx->scsi_status = req->Scsi_Status;
                /* lack of resource, queue for further handling. */
		core_queue_error_req(root, org_req, MV_FALSE);
        }
}

MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
	MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func)
{
	core_context *ctx;
	MV_Request *req;
	MV_U16 tag = 0;

        /* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	/*If the TASK MANAGEMENT FUNCTION field is set to ABORT TASK or QUERY TASK, 
	the TAG OF TASK TO BE MANAGED field specifies the TAG value from the COMMAND
	frame that contained the task to be aborted or checked. For all other 
	task management functions, the TAG OF TASK TO BE MANAGED field shall be ignored.*/

	switch(task_function)
	{
	case TMF_ABORT_TASK:
	case TMF_QUERY_TASK:
		ctx = (core_context *)org_req->Context[MODULE_CORE];
		tag = (org_req->Tag<<root->max_cmd_slot_width) | ctx->slot;
		break;
	default:
		break;
	}

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_TASK_MGMT;
	req->Cdb[3] = (MV_U8)(tag&0xff);
	req->Cdb[4] = (MV_U8)(tag>>8);
	req->Cdb[5] = task_function;
	
	MV_ZeroMemory(&(req->Cdb[6]), 8);
#ifdef SUPPORT_MUL_LUN
	mv_int_to_reqlun(dev->base.LUN, &(req->Cdb[6]));
#endif
	//MV_CopyMemory(&req->Cdb[6], dev->saslun, 8);

	req->Cmd_Flag = 0;
	req->Device_Id = dev->base.id;
	req->Completion = func;
	return req;
}

MV_VOID ssp_wait_hard_reset(MV_PVOID dev_p, MV_PVOID tmp)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_BOOLEAN ret = MV_FALSE; 
	struct _error_context *err_ctx = &dev->base.err_ctx;
	MV_ULONG flags;
	core_extension *core =dev->base.root->core;
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(dev->base.root->core, &flags);
	err_ctx->eh_timer = NO_CURRENT_TIMER;
	if (err_ctx->eh_type == EH_TYPE_TIMEOUT)
		ret = ssp_timeout_state_machine(root, dev, MV_TRUE);
	else if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
		ret = ssp_media_error_state_machine(root, dev, MV_TRUE);

	if (ret == MV_FALSE) {
		/* save the eh request status because req will be released */
		err_ctx->scsi_status = REQ_STATUS_SUCCESS;
		/* lack of resource, queue for further handling. */
		core_queue_error_req(root, err_ctx->error_req, MV_FALSE);
	}
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(dev->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}


