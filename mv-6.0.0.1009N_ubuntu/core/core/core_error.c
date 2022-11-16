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
#include "core_util.h"
#include "core_hal.h"
#include "core_sas.h"
#include "core_sata.h"

#include "core_error.h"
#include "core_util.h"
#include "core_protocol.h"
#include "core_api.h"

#include "core_exp.h"
#include "core_expander.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#elif defined(ATHENA_MISS_MSIX_INT_WA)
#include "hba_inter.h"
#endif
/*
 * Error handling principles, to be as simple as possible.
 * Otherwise error handling itself is the trouble maker.
 *
 * 1. Handle one request error for one device.
 *   Don't handle multiple errors on one device at the same time,
 *   otherwise it'll make the device state machine messed up.
 *
 * 2. Always finish the whole error state machine
 *    In old implementation, one failure request makes the error
 *    state machine goes somewhere. Then another failure request
 *    makes the state machine moving forward. It is not clean.
 *    After finishing one failure request handling,
 *    state machine returns back to the initial state
 *    unless device is gone or set down.
 *
 * 3. Don't do error handling for error handling request
 *    For example, we may send TMF abort request but it fails,
 *    Don't do error handling for that request.
 *
 * 4. error handling for init request
 *    During init state, we may don't have enough information to
 *    do the error handling. (And is the device ready for TMF?)
 *    Make it simple, no matter media error or timeout, just retry.
 *
 * 5. Media error with timeout
 *    Media error refers to any error status returned by the device.
 *    Timeout case is that device doesn't response.
 *    Their handling is different.
 *    If during media error handling, we hit timeout,
 *    then clean up the state, treat it as timeout error.
 *    If during timeout handling, we hit media error,
 *    treat the handling req as failure and continue.
 *    Again don't do error handling for error handling request.
 *
 * 6. Other outstanding requests or waiting requests.
 *    Sometimes we need abort other outstanding requests
 *    besides the error request, it's ok.
 *    But don't just complete these requests with error status.
 *    Must rerun these requests.
 *
 * 7. don't do initialize again during error handling
 *    If hard reset or sth else, it's not good to notify unplug
 *    and then do initialize and plugin again.
 *    And suppose we have all the device information,
 *    we don't need to do the init state again.
 *    (And ideally init state only submit min sets of requests).
 *
 * 8. Req->Scsi_Status: in the process_command, limit the Scsi_Status type
 *    to REQ_STATUS_HAS_SENSE, REQ_STATUS_NO_DEVICE, REQ_STATUS_ERROR
 *    REQ_STATUS_SUCCESS and REQ_STATUS_TIMEOUT.
 *    REQ_STATUS_TIMEOUT won't be sent to the upper layer.
 *    Status is status. Don't use it as error handling strategy like
 *    REQ_STATUS_RETRY or REQ_STATUS_REQUEST_SENSE.
 *
 * 9. Separate Init/EH/Event/IO
 *    To avoid state machine messed up, at one time,
 *    only one of these four state is running.
 *    a. If one port has error request, error handling will be done first.
 *    since normal IO won't run at this moment, this state won't continue for ever.
 *    At one time, only one request is doing eh for one device.
 *    b. After error handling and init request handling, we will run event handling.
 *    And for each port, at one time, only run one event.
 *    Event handling will complete right away but it'll generate init requests.
 *    Before the former init requests complete, next event won't run.
 *    c. normal IO won't run until init request and error request are
 *    all handled for that port.
 *
 */

 /*
  * some other consideration not implemented
  * 1. do we want to consider using timer per request?
  *    the # of devices has grown so big it takes up
  *    the same amount of memory as timer per slot.
  *    This way it might make coding easier.
  *    But right now not implemented for two reasons,
  *    a. runtime, timer module will receive a lot of requests
  */

MV_VOID core_req_timeout(MV_PVOID dev_p, MV_PVOID param);
extern MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);
extern MV_VOID core_clean_init_queue_entry(MV_PVOID root_p,
				domain_base *base);

/*
 * timer related functions
 */
 #ifdef ATHENA_MISS_MSIX_INT_WA
MV_VOID core_check_timeout_req(MV_PVOID dev_p, MV_PVOID req_p);
#endif
MV_VOID mv_add_timer(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base;
	MV_U32 seconds;
	MV_ULONG flags;
	struct _error_context *err_ctx;

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	err_ctx = &base->err_ctx;
#ifdef ATHENA_MISS_MSIX_INT_WA
	if(msix_enabled(core) && base->msix_timer_id == NO_CURRENT_TIMER){
            base->msix_timer_id = core_add_timer(core, 1, core_check_timeout_req, base, NULL);
	}
#endif
	OSSW_SPIN_LOCK(&err_ctx->sent_req_list_SpinLock, flags);
	if (err_ctx->timer_id == NO_CURRENT_TIMER) {
		if (req->Time_Out != 0) {
			seconds = req->Time_Out;
		} else {
			seconds = CORE_REQUEST_TIME_OUT_SECONDS;
		}

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			domain_device *dev = (domain_device *)base;
			if (IS_HDD(dev)) {
			#ifdef _OS_LINUX
				seconds = MV_MAX(seconds,
					CORE_REQUEST_TIME_OUT_SECONDS);
				if (req->Cdb[0] == SCSI_CMD_INQUIRY) {
					seconds = CORE_INQUIRE_TIME_OUT_SECONDS;
				}
			#endif
			} else if (IS_OPTICAL(dev)) {
				seconds = 200;
			} else if (IS_TAPE(dev)) {
				seconds = 400;
			}
		}

#if defined(CORE_PCIE_PNP_DETECTION)
		if ((req->Time_Out == seconds) && (seconds > CORE_PCIE_DETECT_TIME_OUT)) {
			err_ctx->timeout_count_down = (MV_U16)(seconds - CORE_PCIE_DETECT_TIME_OUT);
			seconds = CORE_PCIE_DETECT_TIME_OUT;
		}
#endif		
		err_ctx->timer_id = core_add_timer(
			core, seconds, core_req_timeout, base, req);
	}

	MV_DASSERT(err_ctx->timer_id != NO_CURRENT_TIMER);
	List_AddTail(&req->Queue_Pointer, &err_ctx->sent_req_list);
	OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags);
}

MV_VOID mv_cancel_timer(MV_PVOID core_p, MV_PVOID base_p)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base = (domain_base *)base_p;
	struct _error_context *err_ctx = &base->err_ctx;

	/* refer to core_req_timeout */
	if (err_ctx->timer_id != NO_CURRENT_TIMER) {
		core_cancel_timer(core, err_ctx->timer_id);
		err_ctx->timer_id = NO_CURRENT_TIMER;
#if defined(CORE_PCIE_PNP_DETECTION)
		err_ctx->timeout_count_down = 0;
#endif
	}
}

MV_VOID mv_renew_timer(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base;
	MV_Request *oldest_req;
//	MV_ULONG flags;
	struct _error_context *err_ctx;

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	if (base == NULL) {
		/* Is this possible? */
		CORE_PRINT(("device %d is gone?", req->Device_Id));
		return;
	}
	err_ctx = &base->err_ctx;

	/* this request is not removed from the queue yet */
//	OSSW_SPIN_LOCK(&err_ctx->sent_req_list_SpinLock, flags);
	if(List_Empty(&err_ctx->sent_req_list)){
		CORE_EH_PRINT(("send req is NULL. dev 0x%x io 0x%x, timer_id 0x%x req(%p)\n", base->id, base->outstanding_req, err_ctx->timer_id, req));
	}
	MV_DASSERT(!List_Empty(&err_ctx->sent_req_list));

	/* check if this is the oldest request */
	oldest_req = (MV_Request *)List_GetFirstEntry(
		&err_ctx->sent_req_list, MV_Request, Queue_Pointer);

	if (oldest_req == req) {
		mv_cancel_timer(core, base);

		if (!List_Empty(&err_ctx->sent_req_list)) {
			/* now the new oldest_req */
			oldest_req = LIST_ENTRY(
				(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);

			if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
				/* this should not happen on a ATAPI device */
				MV_DASSERT(!IS_ATAPI(((domain_device *)base)));
				MV_DASSERT(!IS_TAPE(((domain_device *)base)));
			}
#ifdef USE_OS_TIMEOUT_VALUE  //For linux internal test
			err_ctx->timer_id = core_add_timer(
				core, req->Time_Out,core_req_timeout, base, oldest_req);
#else
#if defined(CORE_PCIE_PNP_DETECTION)
			err_ctx->timeout_count_down = (MV_U16)(CORE_REQUEST_TIME_OUT_SECONDS - CORE_PCIE_DETECT_TIME_OUT);
			err_ctx->timer_id = core_add_timer(
				core, CORE_PCIE_DETECT_TIME_OUT,
				core_req_timeout, base, oldest_req);
#else
			err_ctx->timer_id = core_add_timer(
				core, CORE_REQUEST_TIME_OUT_SECONDS,
				core_req_timeout, base, oldest_req);
#endif //defined(CORE_PCIE_PNP_DETECTION)		
#endif //USE_OS_TIMEOUT_VALUE
			MV_DASSERT(err_ctx->timer_id != NO_CURRENT_TIMER);
		} else {
			MV_DASSERT(err_ctx->timer_id == NO_CURRENT_TIMER);
		}
	} else {
		List_Add(&oldest_req->Queue_Pointer, &err_ctx->sent_req_list);
		/* remove this request from the sent_req_list */
		List_Del(&req->Queue_Pointer);
	}
	//KeReleaseSpinLockFromDpcLevel(&err_ctx->sent_req_list_SpinLock);
//	OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags);
}

/*
 * error handling common functions
 */
/* reset slot - clear command active and command issue */
MV_VOID prot_reset_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req)
{
	CORE_EH_PRINT(("dev %d resetting slot %x, req[%p] io 0x%x\n",base->id, slot,req, base->outstanding_req));

	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_ADDR, CMD_PORT_ACTIVE0 + ((slot >> 5) << 2));
	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_DATA, MV_BIT(slot % 32));
	prot_clean_slot(root, base, slot, req);
}

#ifdef MV_DEBUG
MV_U16 gSMPRetryCount = 0;
#endif
extern MV_VOID core_push_queues(MV_PVOID core_p);
extern MV_VOID core_req_handler(MV_PVOID core_p);
MV_VOID
sas_wait_for_spinup(MV_PVOID dev, MV_PVOID  tmp)
{
	MV_ULONG flags;
	domain_device* device = (domain_device *)dev;
	core_extension *core=device->base.root->core;
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(device->base.root->core, &flags);
	core_queue_init_entry(device->base.root, &device->base, MV_FALSE);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(device->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
#if 0 //def DRIVER_IO_PATH
    core_req_handler(device->base.root->core);
#else
	core_push_base_queue(device->base.root->core, &device->base);
#endif
}
/* handle init request error */
#define SSP_INIT_REQUEST_RETRY_COUNT         100
#define SMP_INIT_REQUEST_RETRY_COUNT         10
#define OTHER_INIT_REQUEST_RETRY_COUNT       10
void core_handle_init_error(pl_root *root, domain_base *base, MV_Request *req)
{
	domain_port *port;
	domain_device *dev;
	domain_base *tmp_base;
	domain_expander *exp;
	domain_enclosure *enc;
	struct _error_context *err_ctx;
	MV_U16 retry_count;
#ifdef ASIC_WORKAROUND_WD_TO_RESET
	domain_expander *tmp_exp;
	core_context *ctx = NULL;
	MV_U8 i;
#endif
	if(base == NULL)
	{
		CORE_EH_PRINT(("base == NULL in Req : %p \n", req));
		return;
	}	
	tmp_base=get_device_by_id(root->lib_dev, req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	MV_ASSERT(tmp_base !=NULL);
	MV_ASSERT(base == tmp_base);
	err_ctx = &base->err_ctx;
	port = base->port;

	/* get the retry count */
	retry_count = OTHER_INIT_REQUEST_RETRY_COUNT;
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (IS_SSP(dev)) {
			retry_count = SSP_INIT_REQUEST_RETRY_COUNT;
		}
	}
	else if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
		retry_count = SMP_INIT_REQUEST_RETRY_COUNT;
		#ifdef MV_DEBUG
		if (gSMPRetryCount < err_ctx->retry_count) {
			gSMPRetryCount = err_ctx->retry_count;
		}
		MV_ASSERT(gSMPRetryCount <= SMP_INIT_REQUEST_RETRY_COUNT);
		#endif
	}

#ifdef ASIC_WORKAROUND_WD_TO_RESET
	if (((core_extension *)(root->core))->revision_id != VANIR_C2_REV) {
		ctx = req->Context[MODULE_CORE];
		if (ctx->error_info & EH_INFO_WD_TO_RETRY) {   
			pal_abort_port_running_req(root, port);

			// Wait
			core_sleep_millisecond(root->core, 10);

			// ASIC STP reset
			for (i = 0; i < root->phy_num; i++) {
				if (root->phy[i].port == port) {
		                    WRITE_PORT_PHY_CONTROL(
		                        root,
		                        (&root->phy[i]),
		                        ( SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET));
				}
			}

			// Wait
			core_sleep_millisecond(root->core, 50);

			LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
				domain_expander, base.queue_pointer) {
				pal_clean_expander_outstanding_req(root, tmp_exp);
			}

			// continue with retry.
		}
	}
#endif


	/* make the init stage error handling as simple as possible
	 * don't mess up init state machine with runtime error handling state machine
	 * if it's media error, just retry several times
	 * if it's timeout, just give up */
	MV_DASSERT(req->Scsi_Status != REQ_STATUS_SUCCESS);

	err_ctx->retry_count++;

	if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
		/* for timeout, just retry once */
		if (err_ctx->retry_count < retry_count ) {
			/* Jira#QTS00000-3062 [External Device][QXP1600eS+TL-D400S] Disk 4 was missed after TL-D400S cold boot */
			//err_ctx->retry_count = retry_count;
			if((base->port) && (base->port->phy) && (base->type == BASE_TYPE_DOMAIN_DEVICE) && (base->parent == (domain_base *)base->port) )
				mv_reset_phy(root, MV_BIT(base->port->phy->asic_id), MV_TRUE);
		}
	} 
	{
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
			if (IS_STP(dev)) {
				CORE_DPRINT(("STP device %d status %x has error_count %d, req status %X.\n", base->id, dev->state, err_ctx->retry_count, req->Scsi_Status));
				//dev->state = DEVICE_STATE_STP_RESET_PHY; //useless code.
			}
			break;
		default:
			break;
		}
	}

	if ((req->Scsi_Status == REQ_STATUS_NO_DEVICE)
		|| (err_ctx->retry_count > retry_count)) {
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
			pal_set_down_error_disk(root, dev, MV_TRUE);
			break;
		case BASE_TYPE_DOMAIN_PM:
			pal_set_down_port(root, port);
			break;
		case BASE_TYPE_DOMAIN_EXPANDER:
			exp = (domain_expander *)base;
                        if (exp->has_been_setdown == MV_TRUE) {
                                MV_DASSERT(MV_FALSE);
                                /* multiple request call set down one expander
                                 * no need to do core_init_entry_done */
                                return;
                        }
			exp->has_been_setdown = MV_TRUE;
			pal_set_down_expander(root, exp);
			break;
		case BASE_TYPE_DOMAIN_ENCLOSURE:
#ifdef SUPPORT_SES
			enc = (domain_enclosure *)base;
			if(req->Scsi_Status == REQ_STATUS_NO_DEVICE){
				/*enclosure is gone, set down it and return*/
				pal_set_down_enclosure(root, enc);
			}else{
				/*if request fail during initialize stage, report this enclosure to OS although maybe cannot locate LED*/
				enc->state = ENCLOSURE_INIT_DONE;
				core_init_entry_done(root, port, base);
				return;
			}
			break;
#endif
		case BASE_TYPE_DOMAIN_PORT:
                case BASE_TYPE_DOMAIN_VIRTUAL:
		default:
			/* no request will send to these domain base type
			 * during in init stage */
			MV_ASSERT(MV_FALSE);
			break;
		}

		core_init_entry_done(root, port, NULL);
	} else {
		/* give some time for Hitachi SAS HD,
 		 * which takes long time to spin up
		 * and won't get ready if we keep retrying
		 * retry limit is 3000x10ms,
		 * some tape drive may need more than 15 sec. */
		if((req->Scsi_Status == REQ_STATUS_HAS_SENSE)){
			MV_U8 sense_key;
			MV_U16 timer_id;
			sense_key = ((MV_U8 *)req->Sense_Info_Buffer)[2] & 0x0f;
			if(sense_key == SCSI_SK_NOT_READY){
				timer_id = core_add_timer(root->core, 1, sas_wait_for_spinup, base, NULL);
				MV_ASSERT(timer_id != NO_CURRENT_TIMER);
				return;
			}
		}
		core_sleep_millisecond(root->core, 10);

		core_queue_init_entry(root, base, MV_FALSE);
	}
}
#ifdef ATHENA_MISS_MSIX_INT_WA
MV_U32 g_msix_timeout_value = 10;
#if 1//def _OS_LINUX
MV_VOID core_check_timeout_req(MV_PVOID dev_p, MV_PVOID req_p){
    domain_base *base= (domain_base *)dev_p;
    pl_root *root = base->root;
    core_extension *core = (core_extension *)root->core;
    HBA_Extension *pHBA = (HBA_Extension *)HBA_GetModuleExtension(core, MODULE_HBA);
    MV_U16 message_id = (root->root_id * root->queue_num);
    pl_queue *queue;
    MV_U32 *cmpl;
    MV_U32 cmpl_read_point,cmpl_write_point;
    base->msix_timer_id = NO_CURRENT_TIMER;
    if(base->type == BASE_TYPE_DOMAIN_DEVICE){
        domain_device *dev = (domain_device *)base;
        queue = &root->queues[dev->queue_id];
        cmpl = (MV_PU32)queue->cmpl_q_shadow;
        cmpl_read_point = READ_CMPLQ_RD_PTR(root, queue->id);
        cmpl_write_point = READ_CMPLQ_WR_PTR(root, queue->id);
        if(queue->last_cmpl_q !=(MV_U16) (*cmpl)){
            if(queue->msi_x_miss_count > g_msix_timeout_value){
                MV_PRINT("dev%d miss interrupt last 0x%x, cmpl sha 0x%x R 0x%x W 0x%x miss cnt %d\n",dev->base.id, queue->last_cmpl_q, *cmpl, cmpl_read_point, cmpl_write_point, queue->msi_x_miss_count);
#if 0
                core_disable_ints(base->root->core);
                message_id+=queue->id;
                MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
                Core_QueueInterruptHandler(core, message_id);
                core_enable_ints(core);
#else
#ifdef SUPPORT_MULTI_DPC
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc[message_id],
            root,
            queue);
#else
            message_id+=queue->id;
 //           core_disable_queue_ints(core, (MV_U16)MV_BIT(message_id));
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc,
            ULongToPtr(message_id),
            NULL);
#endif
//            core_enable_queue_ints(core, (MV_U16)MV_BIT(root->queue_num * root->root_id + queue->id));
#endif
            }
            queue->msi_x_miss_count++;
        }
    }else{
        queue = &root->queues[0];
        cmpl = (MV_U32 *)queue->cmpl_q_shadow;
        cmpl_read_point = READ_CMPLQ_RD_PTR(root, queue->id);
        cmpl_write_point = READ_CMPLQ_WR_PTR(root, queue->id);
        if(queue->last_cmpl_q !=(MV_U16) (*cmpl)){
            if(queue->msi_x_miss_count > g_msix_timeout_value){
            MV_PRINT("device %d type %d miss interrupt last 0x%x, cmpl 0x%x R 0x%x W 0x%x\n",base->id, base->type, queue->last_cmpl_q, *cmpl, cmpl_read_point, cmpl_write_point);
#if 0
            core_disable_ints(core);
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            Core_QueueInterruptHandler(core, message_id);
            core_enable_ints(core);
#else
#ifdef SUPPORT_MULTI_DPC
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc[message_id],
            root,
            queue);
#else
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc,
            ULongToPtr(message_id),
            NULL);
#endif
#endif
            }
            queue->msi_x_miss_count++;
        }
    }
    if(base->msix_timer_id == NO_CURRENT_TIMER){
        base->msix_timer_id = core_add_timer(core, 1, core_check_timeout_req, base, NULL);
    }
    return;
}
#else
MV_BOOLEAN core_check_timeout_req(domain_base *base, MV_Request *req){
    pl_root *root = base->root;
    core_extension *core = (core_extension *)root->core;
    HBA_Extension *pHBA = (HBA_Extension *)HBA_GetModuleExtension(core, MODULE_HBA);
    ULONG message_id = (root->root_id * root->queue_num);
    pl_queue *queue;
    MV_U32 *cmpl;
    MV_U32 cmpl_read_point,cmpl_write_point;
    if(base->type == BASE_TYPE_DOMAIN_DEVICE){
        domain_device *dev = (domain_device *)base;
        queue = &root->queues[dev->queue_id];
        cmpl = (MV_PU32)queue->cmpl_q_shadow;
        cmpl_read_point = READ_CMPLQ_RD_PTR(root, queue->id);
        cmpl_write_point = READ_CMPLQ_WR_PTR(root, queue->id);
        if(queue->last_cmpl_q !=(MV_U16) (*cmpl)){
            MV_PRINT("dev%d miss interrupt last 0x%x, cmpl sha 0x%x R 0x%x W 0x%x\n",dev->base.id, queue->last_cmpl_q, *cmpl, cmpl_read_point, cmpl_write_point);
            core_disable_ints(base->root->core);
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc,
            ULongToPtr(message_id),
            NULL);
            core_enable_ints(base->root->core);
            return MV_TRUE;
        }
    }else{
        queue = &root->queues[0];
        cmpl = (MV_U32 *)queue->cmpl_q_shadow;
        cmpl_read_point = READ_CMPLQ_RD_PTR(root, queue->id);
        cmpl_write_point = READ_CMPLQ_WR_PTR(root, queue->id);
        if(queue->last_cmpl_q !=(MV_U16) (*cmpl)){
            MV_PRINT("device %d type %d miss interrupt last 0x%x, cmpl 0x%x R 0x%x W 0x%x\n",base->id, base->type, queue->last_cmpl_q, *cmpl, cmpl_read_point, cmpl_write_point);
            core_disable_ints(base->root->core);
            message_id+=queue->id;
            MV_PRINT("StorPortIssueDpc msg id 0x%x\n",message_id);
            StorPortIssueDpc(
            pHBA->Device_Extension,
            &pHBA->MsixDpc,
            ULongToPtr(message_id),
            NULL);
            core_enable_ints(base->root->core);
            return MV_TRUE;
        }
    }
    return MV_FALSE;
}
#endif
#endif
MV_VOID core_req_timeout(MV_PVOID dev_p, MV_PVOID req_p)
{

	domain_base *base = NULL;
	MV_Request *err_req = NULL;
	pl_root *root = NULL;
	struct _error_context *err_ctx = NULL;
	core_context *ctx = NULL;
	MV_Request *first_req = NULL;
	MV_U32 tmp=0;
	domain_device *dev = NULL;
	MV_ULONG flags, flags1,flags_queue;
	MV_U16 map_id;
    core_extension *core = NULL;
#ifdef SCSI_ID_MAP
    PHBA_Extension hba;
#endif
	MV_DASSERT(dev_p!= NULL);
	MV_DASSERT(req_p!= NULL);

	base = (domain_base *)dev_p;
	err_req = (MV_Request *)req_p;
	root = (pl_root *)base->root;
	err_ctx = &base->err_ctx;
	ctx = err_req->Context[MODULE_CORE];
	core = (core_extension *)root->core;
#ifdef SCSI_ID_MAP
    hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
#endif
	if(IS_ATA_PASSTHRU_CMD(err_req,ATA_CMD_SEC_ERASE_UNIT)){
		CORE_EH_PRINT(("The command of security erase is running...\n"));
		err_ctx->timer_id = core_add_timer(root->core, 60,core_req_timeout, base, err_req);
		return;
	}
	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_ADDR, CMD_PORT_ACTIVE0 + ((ctx->slot >> 5) << 2));
	tmp = MV_REG_READ_DWORD(root->mmio_base, COMMON_CMD_DATA);

	CORE_EH_PRINT(("device %d request %p [0x%x] time out "\
		"on slot(0x%x) while active=0x%x. io 0x%x\n", \
		base->id, err_req, err_req->Cdb[0],
		ctx->slot, tmp, base->outstanding_req));
	MV_DumpRequest(err_req, MV_FALSE);
#if defined(CORE_PCIE_PNP_DETECTION)
	tmp = MV_REG_READ_DWORD(root->mmio_base, HBA_CONTROL);
	if ((tmp == 0xFFFFFFFF) || (tmp == 0)) {
		MV_VOID core_set_pcie_unplug(pl_root *root);
	
		/* Unplug disk of timeout request */
		pal_set_down_port(root, base->port);	
		core_set_pcie_unplug(root);
		return;
	}

	if(err_ctx->timeout_count_down) {
		if (err_ctx->timeout_count_down > CORE_PCIE_DETECT_TIME_OUT) {
			err_ctx->timer_id = core_add_timer(root->core, CORE_PCIE_DETECT_TIME_OUT,core_req_timeout, base, err_req);
			err_ctx->timeout_count_down -= CORE_PCIE_DETECT_TIME_OUT;
		}
		else {
			err_ctx->timer_id = core_add_timer(root->core, err_ctx->timeout_count_down,core_req_timeout, base, err_req);
			err_ctx->timeout_count_down = 0;
		}	
		return;
	}
#endif
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	if(!get_device_by_id(root->lib_dev, base->id, MV_FALSE, MV_FALSE)){
		CORE_DPRINT(("dev %d not exist!\n", base->id));
		//MV_DASSERT(MV_FALSE);
		//core_global_unlock(root->core, &flags);
		//spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
		return;
	}
#ifdef SUPPORT_I2C
	if(base->type == BASE_TYPE_DOMAIN_I2C)
	{

		domain_i2c_link *i2c_link=&((core_extension *)(root->core))->lib_i2c.i2c_link;
		base->outstanding_req = 0;
		if( i2c_link->i2c_request==NULL )
		{
			//OSSW_SPIN_UNLOCK_IRQRESTORE(&pCore->desc->hba_desc->global_lock,flag);
			//core_global_unlock(root->core, &flags);
			//spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			return ;
		}
		first_req=(PMV_Request)i2c_link->i2c_request;
		first_req->Scsi_Status=REQ_STATUS_ERROR;
		i2c_link->i2c_state=I2C_STATE_ERROR;
		if(first_req != err_req) {
			//core_global_unlock(root->core, &flags);
			//spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			return;
		}
		/* If command timeout, clear it to avoid enter i2c isr repeatly */
		i2c_link->i2c_request = NULL;
		List_DelInit(&first_req->Queue_Pointer);
		core_queue_completed_req(root->core, err_req);
		//OSSW_SPIN_UNLOCK_IRQRESTORE(&pCore->desc->hba_desc->global_lock,flag);
		goto end_point;
	}
#endif /* SUPPORT_I2C */

	/*
	 * Timer_CheckRequest() will release the timer after return.
	 * Found __cancel_timer() here releases Timer ID right away, and
	 * the same Timer ID will be re-assigned for issuing command before
	 * leaving this routine, this Timer ID then be reset by Timer_CheckRequest().
	 * This causes the Timer missing/Timer_ID inconsistant
	 */
	/* since this timer is triggerred, just set it to NO_CURRENT_TIMER */
	OSSW_SPIN_LOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
	OSSW_SPIN_LOCK(&err_ctx->sent_req_list_SpinLock, flags1);
	/* get the request that caused this time out */
	if (!List_Empty(&err_ctx->sent_req_list)) {
		first_req = LIST_ENTRY(
			(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
		//MV_ASSERT(first_req == err_req);
		if(first_req != err_req){
			CORE_EH_PRINT((" dev 0x%x first_req(%p) !=err_req(%p) timer id 0x%x\n", base->id, first_req, err_req, err_ctx->timer_id));
			MV_ASSERT(err_ctx->timer_id !=NO_CURRENT_TIMER);
			OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
			OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			return;
		}
	} else {
		CORE_EH_PRINT(("no outstanding req on dev %d?\n", base->id));
		//MV_DASSERT(MV_FALSE);
		err_ctx->timer_id = NO_CURRENT_TIMER;
		OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
		OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
		//core_global_unlock(root->core, &flags);
		//spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
		return;
	}
	err_ctx->timer_id = NO_CURRENT_TIMER;

#if 0 /*temp ignore missed interrupt req, below calling will result to dead-lock.*/
	#ifndef SUPPORT_ATHENA /* fs TODO: should handle ATTENTION? */
	/* let's check whether we've missed one interrupt */
	io_chip_handle_cmpl_queue_int(root);
	#else
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		io_chip_handle_cmpl_queue(&root->queues[dev->queue_id]);
	} else
		io_chip_handle_cmpl_queue(&root->queues[0]);
	#endif
#endif

	if (List_Empty(&err_ctx->sent_req_list)) { 
		CORE_PRINT(("sent_req_list empty\n"));
		CORE_PRINT(("device %d missed one interrupt for req %p.\n", \
			base->id, err_req));
		OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
		OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
		goto end_point;
	} else {
		first_req = LIST_ENTRY(
			(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
		if (first_req != err_req) {
			CORE_PRINT(("device %d missed one interrupt for req %p.\n", \
				base->id, err_req));
			OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
			OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
			goto end_point;
		}
	}

	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (dev->state == DEVICE_STATE_INIT_DONE) {
			core_generate_event(root->core, EVT_ID_HD_TIMEOUT, base->id,
				SEVERITY_WARNING, 0, NULL,0);
#ifdef CORE_EH_LOG_EVENTS
                        if (HBA_CheckIsFlorence(root->core)) {
                                /* for florence fake one event to show the timeout LBA */
	                        MV_U32 params[MAX_EVENT_PARAMS];
                                MV_SetLBAandSectorCount(err_req);
                                params[0] = CORE_EVENT; /* should be CDB */
                                params[1] = err_req->LBA.parts.high; /* Sense Key */
                                params[2] = err_req->LBA.parts.low; /* Additional Sense Code */
                                params[3] = err_req->Cdb[0]; /* Additional Sense Code Qualifier */
                	        core_generate_event(root->core, EVT_ID_HD_SC_ERROR, dev->base.id,
	                                SEVERITY_INFO, 4, params, 0);
                        }
#endif
                }
        }
#ifdef SCSI_ID_MAP
    if (CORE_IS_ID_MAPPED(err_req)) {
        map_id = err_req->Device_Id;
    }
    else {
        map_id = get_internal_id_by_device_id(hba->map_table, err_req->Device_Id);
    }
    MV_ASSERT(base->id == map_id);
    MV_ASSERT(base == get_device_by_id(root->lib_dev, map_id, MV_FALSE, MV_FALSE));
#else
#if defined (SUPPORT_MUL_LUN) && defined(_OS_WINDOWS)
    if (ID_IS_OS_TYPE(err_req)) {
        map_id = get_base_id_by_target_lun(core, DEV_ID_TO_TARGET_ID(err_req->Device_Id), DEV_ID_TO_LUN(err_req->Device_Id));
        MV_ASSERT(base->id == map_id);
        MV_ASSERT(base == get_device_by_id(root->lib_dev, map_id, MV_FALSE, MV_FALSE));
    }
    else
#endif
    {
    MV_ASSERT(base->id == err_req->Device_Id);
    MV_ASSERT(base == get_device_by_id(root->lib_dev, err_req->Device_Id, MV_FALSE, MV_FALSE));
    }
#endif
	/* reset the error req slot */
	prot_reset_slot(root, base, ctx->slot, err_req);
	OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
	OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);

#ifdef SUPPORT_SG_RESET
    if (CORE_IS_HBA_RESET_REQ(err_req))	
	{
	    CORE_PRINT(("device %d sg reset timeout for req %p.\n", \
				base->id, err_req));
		err_req->Scsi_Status = REQ_STATUS_ERROR;
	    core_queue_completed_req(root->core,err_req);
		goto end_point;	
	}
#endif

	/* removed from the queue already in prot_reset_slot */
	MV_ASSERT(err_req->Queue_Pointer.next == NULL);
	MV_ASSERT(err_req->Queue_Pointer.prev == NULL);

	/* dont do error handling for error handling req and init req */
	if (CORE_IS_EH_REQ(ctx) || CORE_IS_INIT_REQ(ctx)) {
		if (CORE_IS_EH_REQ(ctx)) {
			//err_req->Scsi_Status = REQ_STATUS_ERROR;
			/* may need switch state machine to timeout state machine */
			err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
		} else {
			/* init request set to timeout
			 * so core_handle_init_error won't retry so many times */
		#ifdef SUPPORT_STAGGERED_SPIN_UP
			core_extension *core=(core_extension *)root->core;
			if(core->spin_up_group){
				if(base->type == BASE_TYPE_DOMAIN_DEVICE){
					dev = (domain_device *)base;
					if(dev->state != DEVICE_STATE_INIT_DONE){
						dev->state = DEVICE_STATE_INIT_DONE;
						dev->status |= DEVICE_STATUS_NO_DEVICE;
						}
					}
			}
		#endif
			err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
		}
		core_queue_completed_req(root->core, err_req);
	} else {
		/* queue to the error queue */
		err_req->Scsi_Status = REQ_STATUS_TIMEOUT;

		//if ((dev != NULL)
		//	&& (dev->register_set != NO_REGISTER_SET)
		//	&& (sata_is_register_set_disabled(root, dev->register_set))) {
		//
		//	dev->base.cmd_issue_stopped = MV_TRUE;
		//}
#if defined( _OS_LINUX)
#ifdef ENABLE_HARD_RESET_EH
		if (IS_BEHIND_EXP(dev)) {
				CORE_EH_PRINT(("device %d after expander timeout, reset the phy.\n",base->id));
				err_req->Req_Flag &= ~REQ_FLAG_RETRY;
				core_queue_error_req(root, err_req, MV_TRUE);
		#ifndef SUPPORT_ENHANCED_EH
				pal_abort_device_running_req(root, base);
		#endif
				goto end_point;
		}
#endif
		CORE_PRINT(("device %d prepare to do EH for timeout request %p.\n",base->id, err_req));
		#ifdef SUPPORT_VU_CMD
		if ((NULL != dev) && (dev->status & DEVICE_STATUS_WAIT_ASYNC)) 
		{
			err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
			core_queue_completed_req(root->core, err_req);
		#ifndef SUPPORT_ENHANCED_EH
			pal_abort_device_running_req(root, base);
		#endif
			core_complete_device_waiting_queue(root, &dev->base, REQ_STATUS_NO_DEVICE);
		} else 
		#endif
		{
			core_queue_error_req(root, err_req, MV_TRUE);
		#ifndef SUPPORT_ENHANCED_EH
			pal_abort_device_running_req(root, base);
		#endif
		}
#else
		core_queue_error_req(root, err_req, MV_TRUE);
		pal_abort_device_running_req(root, base);
#endif
	}

end_point:
//	core_global_unlock(root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	/* need finish completed requests and push queue */
#if 0 //def DRIVER_IO_PATH
		core_req_handler(core);
#else
	core_send_mv_request(core, err_req);
#endif
}

#ifdef _OS_LINUX
MV_VOID core_clean_error_queue(core_extension *core)
{
	MV_Request *req;
	domain_base *base;
	pl_root *root;

	while (!Counted_List_Empty(&core->error_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
					req->Device_Id, CORE_IS_ID_MAPPED(req), MV_FALSE);
		root = base->root;
		
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_complete_error_req(root, req, req->Scsi_Status);
	}
}
#endif

MV_VOID core_handle_error_queue(core_extension *core)
{
	MV_Request *req;
	core_context *ctx;
	domain_base *base;
	domain_device *device;
	MV_BOOLEAN ret = MV_FALSE;
	MV_Request *old_req = NULL; /* already has gone through this req */
	domain_port *port;
	pl_root *root;
	MV_U8 i;
	MV_ULONG flags;
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
#ifdef SUPPORT_ENHANCED_EH
	MV_U16 k, running_count = 0;
	MV_U8 j;
	/*before error handling, waiting for other running requests to finish.*/
	for ( j=core->chip_info->start_host; j<(core->chip_info->start_host + core->chip_info->n_host); j++) {
		root = &core->roots[j];

		for (k = 0; k < root->slot_count_support; k++) {
			req = root->running_req[k];
			if (req == NULL) continue;
			running_count++;
		}
	}
	if (running_count)
		return;
#endif
#endif

	/* for one device, only handle one error at a time */
	OSSW_SPIN_LOCK(&core->error_queue_SpinLock, flags);
	while (!Counted_List_Empty(&core->error_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		if (req == old_req) {
			/* has gone through the list */
			Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
			OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
			return;
		}

		ctx = (core_context *)req->Context[MODULE_CORE];
		base = (domain_base *)get_device_by_id(&core->lib_dev,
					req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
		port = base->port;
		root = base->root;

		/*
		 * don't do error handling if there is event handling going
		 * on any other port
		 */

		for (i = 0; i < root->phy_num; i++) {
			port = &root->ports[i];
			if (port_has_init_req(root, port)) {
				Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
				OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
				return;
			}
		}

		/* this device has multiple error reqs in the queue
		 * only do one at a time
		 * but sometimes the first error cannot continue
		 * because of resource,
		 * (it has been kicked off but during error handling state machine,
		 * short of resource)
		 * it's pushed back to the queue.
		 * for this case, CORE_IS_EH_REQ(ctx)==MV_TRUE
		 * refer to core_queue_error_req
		 * for original error, CORE_IS_EH_REQ(ctx)==MV_FALSE */
#ifdef SUPPORT_ENHANCED_EH
		/*one time do eh for only one of the devices attached to same port.*/
		if(port_is_running_error_handling(root, base->port)) {
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
				continue;
		}
		
		/*If error device is recovered or port is reset, retry the rest error request.*/
		if ((base->err_ctx.state == BASE_STATE_ERROR_HANDLED) 
			|| (base->port->base.err_ctx.state == BASE_STATE_ERROR_HANDLED)){
			CORE_DPRINT((" device or port are recovered, retry the error req %p to device %d.\n", req, base->id));
			ctx->error_info = 0x0; /*clear error info for new-retried req.*/
			core_push_running_request_back(root, req);
			if (--base->err_ctx.error_count == 0)
				base->err_ctx.state = BASE_STATE_NONE;
			if (!port_has_error_req(root, base->port))
				base->port->base.err_ctx.state = BASE_STATE_NONE;
			continue;
		}
#else
		if (base->err_ctx.state == BASE_STATE_ERROR_HANDLING) {
			if (!CORE_IS_EH_REQ(ctx)) {
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
				continue;
			}
		}
#endif

		/* let the normal requests finish first */
		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			device = (domain_device *)base;
            
		/* [Vanir] Changed SATA device error handling for media error to abort outstanding requests immediately and then proceed with media error state machine. */
			if (IS_SSP(device) && (base->outstanding_req >= 1) ) {
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
				continue;
			}
		}

		/* at this moment, normal IO won't send to this device */
		if (base->handler->error_handler != NULL) {
			base->err_ctx.state = BASE_STATE_ERROR_HANDLING;
			ret = base->handler->error_handler(base, req);

			if (ret == MV_FALSE) {
				base->err_ctx.state = BASE_STATE_NONE;
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
			}
		} else {
			/* if the device doesn't have the error handler,
			 * just return the error request and restore domain_base status */
			MV_ASSERT(base->err_ctx.state == BASE_STATE_NONE);
			/* don't return the temporary status REQ_STATUS_TIMEOUT */
			if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
				req->Scsi_Status = REQ_STATUS_ERROR;
			}
			OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
			core_complete_error_req(root, req, req->Scsi_Status);
			OSSW_SPIN_LOCK(&core->error_queue_SpinLock, flags);
		}
	}
	OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
}

MV_U32 pal_abort_device_running_req(pl_root *root, domain_base *base)
{
	MV_U16 i;
	MV_Request *req;
	MV_U32 abort_count = 0;
	MV_ULONG flags, flags_queue, flags_roots;
	MV_U16 msix_idx=0;
#ifdef MV_DEBUG
	core_context *ctx;
#endif
	pl_queue *queue;
    MV_U16 map_id;
    core_extension *core = (core_extension *)root->core;
#ifdef SCSI_ID_MAP
    PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
#endif
	/* clean cmpl queue in case request is already finished */
	#if 0 /* fs TODO */
	io_chip_handle_cmpl_queue_int(root);
	#endif
	OSSW_SPIN_LOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
	OSSW_SPIN_LOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	queue = core_get_hw_queue_of_base(root->core, base);
	if(queue)
		msix_idx=queue->msix_idx;
	for (i = 0; i < root->slot_count_support; i++) {
		//OSSW_SPIN_LOCK_ROOT(core, flags_roots, base->root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags_roots);
		req = root->running_req[i];
		if (req == NULL){
			//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
			continue;
		}
#ifdef SCSI_ID_MAP
        if (!CORE_IS_ID_MAPPED(req)) {
            map_id = req->Device_Id;
        }
        else {
            map_id = get_internal_id_by_device_id(hba->map_table, req->Device_Id);
        }
        if (base->id != map_id) {
		//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
		continue;
	}
        MV_ASSERT(base == get_device_by_id(root->lib_dev, map_id, MV_FALSE, MV_FALSE));
#else
#if defined (SUPPORT_MUL_LUN) && defined (_OS_WINDOWS)
        if (ID_IS_OS_TYPE(req)) {
            map_id = get_base_id_by_target_lun(core, DEV_ID_TO_TARGET_ID(req->Device_Id), DEV_ID_TO_LUN(req->Device_Id));
            if (base->id != map_id){
			//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
			continue;
		}
            MV_ASSERT(base == get_device_by_id(root->lib_dev, map_id, MV_FALSE, MV_FALSE));
        }
        else
#endif
    {
		if (base->id != req->Device_Id){
			//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
			continue;
		}
		/* make sure this device should still exist */
		MV_ASSERT(base == get_device_by_id(root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE));
    }
#endif
		#ifdef MV_DEBUG
		ctx = (core_context *)req->Context[MODULE_CORE];
		#endif
		 root->running_req[i]= NULL;
		//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
		prot_reset_slot(root, base, i, req);
		MV_DASSERT(ctx->slot == i);

		/* it has been removed from the sent list in prot_reset_slot */
		MV_DASSERT(req->Queue_Pointer.next == NULL);
		MV_DASSERT(req->Queue_Pointer.prev == NULL);
		/* sg_wrapper should have been released in prot_reset_slot*/
		MV_DASSERT(ctx->sg_wrapper == NULL);

		core_push_running_request_back(root, req);
		abort_count++;
	}
	MV_ASSERT(List_Empty(&(base->err_ctx.sent_req_list)));
	CORE_EH_PRINT(("%d requests aborted.\n", abort_count));
	OSSW_SPIN_UNLOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);

	/* clear register set, if any */
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *device = (domain_device *)base;

		if (device->register_set != NO_REGISTER_SET) {
			CORE_EH_PRINT(("dev%d requests 0x%x didn't abort.\n", device->base.id, device->base.outstanding_req));
			MV_DASSERT(device->base.outstanding_req == 0);
			core_free_register_set(root, device, device->register_set);
			device->register_set = NO_REGISTER_SET;
		}
	}
	//OSSW_SPIN_UNLOCK_ROOT(core, flags_roots, base->root->root_id);

	return abort_count;
}

extern void core_handle_waiting_queue(core_extension *core);
MV_U32 pal_abort_port_running_req(pl_root *root, domain_port *port)
{
	MV_U16 i;
	MV_U32 abort_count = 0;
	MV_ULONG flags, flags_roots,flags_queue;
	domain_base *req_base;
	domain_device *device;
	MV_Request *req;
#ifdef MV_DEBUG	
	core_context *ctx;
#endif
	/* clean cmpl queue in case request is already finished */
//	CORE_EH_PRINT(("abort: cmpl queue cleaned, pointer 0x%x\n", root->last_cmpl_q));
//	io_chip_handle_cmpl_queue_int(root);	
	for (i = 0; i < root->slot_count_support; i++) {
		//OSSW_SPIN_LOCK_ROOT(root->core, flags_roots, root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags_roots);
		req = root->running_req[i];
		//OSSW_SPIN_UNLOCK_ROOT(root->core, flags_roots, root->root_id);
		if (req == NULL){ 
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
			continue;
		}
		req_base = (struct _domain_base *)get_device_by_id(root->lib_dev,
						req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
		MV_ASSERT(req_base != NULL);
		if (req_base->port != port) {
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
			continue;
		}
#ifdef MV_DEBUG	
		ctx = (core_context *)req->Context[MODULE_CORE];
#endif
		if (req_base->type == BASE_TYPE_DOMAIN_DEVICE) {
			domain_device *dev = (domain_device *)req_base;
			if (dev->register_set != NO_REGISTER_SET)
				CORE_EH_PRINT(("slot's reg set is %d\n", dev->register_set));
		}	
#if 0 /*temp ignore missed interrupt req, below calling will result to dead-lock.*/
			//io_chip_handle_cmpl_queue(&root->queues[dev->queue_id]);
			//io_chip_handle_cmpl_queue(&root->queues[0]);
#endif
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags_roots);
		OSSW_SPIN_LOCK(&req_base->queue->handle_cmpl_SpinLock, flags_queue);
		OSSW_SPIN_LOCK(&req_base->err_ctx.sent_req_list_SpinLock, flags);
		prot_reset_slot(root, req_base, i, req);
		OSSW_SPIN_UNLOCK(&req_base->err_ctx.sent_req_list_SpinLock, flags);
		OSSW_SPIN_UNLOCK(&req_base->queue->handle_cmpl_SpinLock, flags_queue);
		MV_DASSERT(ctx->slot == i);

		/* it has been removed from the sent list in prot_reset_slot */
		MV_DASSERT(req->Queue_Pointer.next == NULL);
		MV_DASSERT(req->Queue_Pointer.prev == NULL);
		/* sg_wrapper should have been released in prot_reset_slot*/
		MV_DASSERT(ctx->sg_wrapper == NULL);

		core_push_running_request_back(root, req);
		abort_count++;
	}

	/* clear register set, if any */
	LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device,
		base.queue_pointer) {
		if (device->register_set != NO_REGISTER_SET) {
			MV_DASSERT(device->base.outstanding_req == 0);
			CORE_EH_PRINT(("freeing reg set %d\n", device->register_set));
			core_free_register_set(root, device, device->register_set);
			device->register_set = NO_REGISTER_SET;
		}
	}
#if defined(SUPPORT_BALDUR)
	CORE_EH_PRINT(("end of function, reg set enable 0 = 0x%x 0x%x\n",
		READ_REGISTER_SET_ENABLE(root, 0), READ_REGISTER_SET2_ENABLE(root, 0)));
#endif
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	CORE_EH_PRINT(("end of function, SAS reg set enable 0 = 0x%x 0x%x\n",
		READ_SAS_REGISTER_SET_ENABLE(root, 0), READ_SAS_REGISTER_SET2_ENABLE(root, 0)));
#endif
	//CORE_EH_PRINT(("end of function, cmpl queue pointer 0x%x\n", root->last_cmpl_q));

	CORE_EH_PRINT(("%d requests aborted.\n", abort_count));
        return abort_count;
}

void pal_clean_req_callback(pl_root *root, MV_Request *req)
{
	domain_base *base;

	base = (domain_base *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), MV_FALSE);
	if (base)
		CORE_EH_PRINT(("base id %d clean up done, type %d\n", base->id, base->type));
	else
		MV_ASSERT(MV_FALSE);
}

void pal_clean_expander_outstanding_req(pl_root *root, domain_expander *exp)
{
	domain_device *device;
	MV_Request *req=NULL;

	LIST_FOR_EACH_ENTRY_TYPE(device, &exp->device_list, domain_device,
		base.exp_queue_pointer) {
		if (IS_SSP(device)) {
			req = sas_make_task_req(root, device, NULL,
				TMF_ABORT_TASK_SET, (MV_ReqCompletion)pal_clean_req_callback);
		} else if (IS_STP(device)) {
			req = smp_make_phy_control_req(device, LINK_RESET,
				pal_clean_req_callback);
		}
		// HARRIET_TBD: what if resources run out?
		if (!req) {
			CORE_DPRINT(("clean up ran out of reqs\n"));
			return;
		} else {
			CORE_DPRINT(("sending clean up req to device %d\n",
				device->base.id));
		}
		core_queue_eh_req(root, req);
	}
}


/* hw waiting_queue and high_priority_queue */
#if 1
MV_U32 pal_complete_hw_waiting_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 cmpl_count = 0;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	pl_queue *queue = core_get_hw_queue_of_base(core, base);
	OSSW_SPIN_LOCK(&queue->waiting_queue_SpinLock, flags);
	//KeAcquireSpinLockAtDpcLevel(&queue->waiting_queue_SpinLock);
	do{
		//OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		if (Counted_List_Empty(&queue->waiting_queue)) {
			break;
		}
		
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->waiting_queue, MV_Request, Queue_Pointer);
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &queue->waiting_queue);
			continue;
		}
	}while(Counted_List_GetCount(&queue->waiting_queue, MV_FALSE)>0);
	OSSW_SPIN_UNLOCK(&queue->waiting_queue_SpinLock, flags);
	//KeReleaseSpinLockFromDpcLevel(&queue->waiting_queue_SpinLock);	
	OSSW_SPIN_LOCK(&queue->high_priority_SpinLock, flags);
	//KeAcquireSpinLockAtDpcLevel(&queue->high_priority_SpinLock);
	do{
		if (Counted_List_Empty(&queue->high_priority_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->high_priority_queue, MV_Request, Queue_Pointer);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &queue->high_priority_queue);
			continue;
		}
	}while(Counted_List_GetCount(&queue->high_priority_queue, MV_FALSE)>0);
	OSSW_SPIN_UNLOCK(&queue->high_priority_SpinLock, flags);
	//KeReleaseSpinLockFromDpcLevel(&queue->high_priority_SpinLock);

	return cmpl_count;
}
/* waiting_queue and high_priority_queue */
MV_U32 pal_complete_device_waiting_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 cmpl_count = 0;
	MV_U32 count = 0;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	MV_U8 root_id=0;
	pl_root *root_tmp;
#ifdef ROOT_WAITING_QUEUE
	MV_ASSERT(base->root == root);
  //     count = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
	/* Return the waiting request related to this device. */
	/* waiting queue */
	root_id = 0;
	root_tmp = &core->roots[root_id];
	//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
	OSSW_SPIN_LOCK(&root_tmp->waiting_queue_SpinLock, flags);
	count = Counted_List_GetCount(&core->roots[root_id].waiting_queue, MV_FALSE);
	do{
		if (Counted_List_Empty(&core->roots[root_id].waiting_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[root_id].waiting_queue, MV_Request, Queue_Pointer);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->roots[root_id].waiting_queue);
		}
		count--;
	}while(count> 0);
	//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
	OSSW_SPIN_UNLOCK(&root_tmp->waiting_queue_SpinLock, flags);
	root_id = 1;
	root_tmp = &core->roots[root_id];
	/* Return the waiting request related to this device. */
	/* waiting queue */
	//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
	OSSW_SPIN_LOCK(&root_tmp->waiting_queue_SpinLock, flags);
	count = Counted_List_GetCount(&core->roots[root_id].waiting_queue, MV_FALSE);
	do{
		if (Counted_List_Empty(&core->roots[root_id].waiting_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[root_id].waiting_queue, MV_Request, Queue_Pointer);		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->roots[root_id].waiting_queue);
		}
		count--;
	}while(count > 0);//(Counted_List_GetCount_Lock(&core->roots[root_id].waiting_queue, &core->roots[root_id].waiting_queue_SpinLock, MV_FALSE)> 0);
	//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
	OSSW_SPIN_UNLOCK(&root_tmp->waiting_queue_SpinLock, flags);
#else
	/* Return the waiting request related to this device. */
	/* waiting queue */
	OSSW_SPIN_LOCK(&core->waiting_queue_SpinLock, flags);
	//KeAcquireSpinLockAtDpcLevel(&core->waiting_queue_SpinLock);
	count = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
	do{
		if (Counted_List_Empty(&core->waiting_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->waiting_queue, MV_Request, Queue_Pointer);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);			
		}
		count--;
	}while(count>0);
	//KeReleaseSpinLockFromDpcLevel(&core->waiting_queue_SpinLock);
	OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
#endif
	/* high priority queue */
	OSSW_SPIN_LOCK(&core->high_priority_SpinLock, flags);
	count = Counted_List_GetCount(&core->high_priority_queue, MV_FALSE);
	//KeAcquireSpinLockAtDpcLevel(&core->high_priority_SpinLock);
	do{
		if (Counted_List_Empty(&core->high_priority_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->high_priority_queue, MV_Request, Queue_Pointer);
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->high_priority_queue);			
		}
		count--;
	}while(count>0);
	//KeReleaseSpinLockFromDpcLevel(&core->high_priority_SpinLock);
	OSSW_SPIN_UNLOCK(&core->high_priority_SpinLock, flags);
	return cmpl_count;
}

#else
MV_U32 pal_complete_hw_waiting_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 count, cmpl_count = 0;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	pl_queue *queue = core_get_hw_queue_of_base(core, base);

	count = Counted_List_GetCount(&queue->waiting_queue, MV_FALSE);
	/* Return the waiting request related to this device. */
	/* hw waiting queue */
	while (count > 0) {
		OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		if (Counted_List_Empty(&queue->waiting_queue)) {
			OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
			Counted_List_AddTail(&req->Queue_Pointer, &queue->waiting_queue);
			OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			count--;
			continue;
		}
	}

	/* hw high priority queue */
	count = Counted_List_GetCount(&queue->high_priority_queue, MV_FALSE);
	while (count > 0) {
		OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		if (Counted_List_Empty(&queue->high_priority_queue)) {
			OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->high_priority_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
			Counted_List_AddTail(&req->Queue_Pointer, &queue->high_priority_queue);
			OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			count--;
			continue;
		}
	}
	
	return cmpl_count;
}
/* waiting_queue and high_priority_queue */
MV_U32 pal_complete_device_waiting_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 count, cmpl_count = 0;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	MV_U8 root_id=0;
#ifdef ROOT_WAITING_QUEUE
       count = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
	/* Return the waiting request related to this device. */
	/* waiting queue */
	root_id = 0;
	while (count > 0) {
		OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		if (Counted_List_Empty(&core->roots[0].waiting_queue)) {
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[0].waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
			Counted_List_AddTail(&req->Queue_Pointer, &core->roots[0].waiting_queue);
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			count--;
			continue;
		}
	}
	 count = Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
	/* Return the waiting request related to this device. */
	/* waiting queue */
	root_id = 1;
	while (count > 0) {
		OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		if (Counted_List_Empty(&core->roots[1].waiting_queue)) {
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[1].waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
			Counted_List_AddTail(&req->Queue_Pointer, &core->roots[1].waiting_queue);
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			count--;
			continue;
		}
	}
#else
	count = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
	/* Return the waiting request related to this device. */
	/* waiting queue */
	while (count > 0) {
		OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		if (Counted_List_Empty(&core->waiting_queue)) {
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
			Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			count--;
			continue;
		}
	}
#endif
	/* high priority queue */
	count = Counted_List_GetCount(&core->high_priority_queue, MV_FALSE);
	while (count > 0) {
		OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		if (Counted_List_Empty(&core->high_priority_queue)) {
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->high_priority_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
			Counted_List_AddTail(&req->Queue_Pointer, &core->high_priority_queue);
			OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			count--;
			continue;
		}
	}
	
	return cmpl_count;
}

#endif

MV_U32 pal_complete_base_waiting_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	core_extension *core = (core_extension *)root->core;

	if (core->enable_multi_wcq)
		return pal_complete_hw_waiting_queue(root, base, scsi_status);
	else
		return pal_complete_device_waiting_queue(root, base, scsi_status);
}
#if 1
/* complete device error_queue */
MV_U32 pal_complete_device_error_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 count, cmpl_count = 0;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;

	/* Return the error request related to this device. */
	/* fs: needn't acquire core lock, this function is already in the core lock */
	OSSW_SPIN_LOCK(&core->error_queue_SpinLock, flags);
	count = Counted_List_GetCount(&core->error_queue, MV_FALSE);
	do {
		if (Counted_List_Empty(&core->error_queue)) {
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
		}
		count--;
	}while(count > 0);
	OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
	return cmpl_count;
}
#else
/* complete device error_queue */
MV_U32 pal_complete_device_error_queue(pl_root *root, domain_base *base,
											MV_U8 scsi_status)
{
	MV_Request *req;
	MV_U32 count, cmpl_count = 0;
	core_extension *core = (core_extension *)root->core;

	/* Return the error request related to this device. */
	/* fs: needn't acquire core lock, this function is already in the core lock */
	count = Counted_List_GetCount(&core->error_queue, MV_FALSE);
	while (count > 0) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		
		if (base->id == req->Device_Id) {
			req->Scsi_Status = scsi_status;
			core_queue_completed_req(core, req);
			cmpl_count++;
			count--;
		} else {
			Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
			count--;
			continue;
		}
	}
	
	return cmpl_count;
}
#endif
MV_U32 core_complete_device_waiting_queue(pl_root *root, domain_base *base,
	MV_U8 scsi_status)
{
	core_extension *core = (core_extension *)root->core;
	MV_U32 cmpl_count = 0;
	MV_U32 error_count = 0;

	/* push completion queue to clear up the aborted requests */
	/* this complete may recursively generate new requests due to 
	   some callback functions such as error handling callback
	   so we must call this one time before and after completing
	   device waiting queues */
	core_complete_internal_requests(core);
	core_complete_base_queue(core, base);

	cmpl_count = pal_complete_base_waiting_queue(root, base, scsi_status);
	error_count = pal_complete_device_error_queue(root, base, scsi_status);

	CORE_EH_PRINT(("completed %d error request.\n", error_count));
	cmpl_count += error_count;

	/* if we're in the middle of error handling,
	 * the EH request will immediately complete the error_req if REQ_STATUS_NO_DEVICE
	 * cannot do
	 * if (base->err_ctx.error_req) {
	 * 	core_complete_error_req(root, base->err_ctx.error_req, scsi_status);
	 *	base->err_ctx.error_req = NULL;
         * }
	 * otherwise, will hit sata_error_handling_callback MV_ASSERT(org_req != NULL) */
	MV_ASSERT(scsi_status == REQ_STATUS_NO_DEVICE);

	/* push completion queue to clear up the aborted requests */
	core_complete_internal_requests(core);
	core_complete_base_queue(core, base);

	/* normally EH request will complete the error_req, refer to above comment
         * but in some cases, there is no more EH request at this moment
         * like in sata_timeout_state_machine, the hardreset waiting period.
         * we only have one outstanding timer, no EH request to complete err_ctx.error_req
         * so have to do it manually if it is still there */
	if (base->err_ctx.error_req) {
		core_complete_error_req(root, base->err_ctx.error_req, scsi_status);
		base->err_ctx.error_req = NULL;
		CORE_EH_PRINT(("completed one outstanding error request.\n"));
		/* core_complete_error_req already minus err_ctx.error_count */
		/* complete this request */
		core_complete_internal_requests(core);
		core_complete_base_queue(core, base);
	}

	/* the following code is not important.
	 * suppose after calling core_complete_device_waiting_queue,
	 * object is released */
#ifdef STRONG_DEBUG
	MV_ASSERT(base->err_ctx.eh_type == EH_TYPE_NONE);
	MV_ASSERT(base->err_ctx.error_count == error_count);
	MV_ASSERT(base->err_ctx.state == BASE_STATE_NONE);
	MV_ASSERT(base->err_ctx.error_req == NULL);
#else
	base->err_ctx.eh_type = EH_TYPE_NONE;
	base->err_ctx.state = BASE_STATE_NONE;
#endif
	base->err_ctx.error_count = 0;

	MV_ASSERT(List_Empty(&base->err_ctx.sent_req_list));

	CORE_EH_PRINT(("%d requests completed with status 0x%x.\n", \
		cmpl_count, scsi_status));
	return cmpl_count;
}

/* after calling this function, domain_expander *exp is not valid any more
 * all the devices under this expander got set down too */
void pal_set_down_expander(pl_root *root, domain_expander *exp)
{
	domain_port *port = exp->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *parent_exp, *tmp_exp;
	domain_device *dev;

        CORE_EH_PRINT(("begin to set down expander %d\n", exp->base.id));
        MV_ASSERT(get_device_by_id(root->lib_dev, exp->base.id, MV_FALSE, MV_FALSE) != NULL);

	/* Abort expander running req first, because of exp_req may contain org_req
	 * which point to devices attached to the exp.
	 * So before devices release, we need abort requests point to them.*/
	pal_abort_device_running_req(root, &exp->base);		// spin lock ?
	core_complete_device_waiting_queue(root, &exp->base, REQ_STATUS_NO_DEVICE);

	/* free all devices/expanders attached to this expander */
	// spin lock ?
	while (!List_Empty(&exp->device_list)) {
		dev = List_GetFirstEntry(&exp->device_list, domain_device,
			base.exp_queue_pointer);
		pal_set_down_disk(root, dev, MV_TRUE);
		exp->device_count--;
	}
	// spin lock ?
	while (!List_Empty(&exp->expander_list)) {
		tmp_exp = List_GetFirstEntry(&exp->expander_list, domain_expander,
			base.exp_queue_pointer);
		pal_set_down_expander(root, tmp_exp);
		exp->expander_count--;
	}

#ifdef SUPPORT_SES

	// spin lock ?
	if(exp->enclosure){
		domain_enclosure *associate_enc = exp->enclosure;
		domain_expander *tmp_exp;
		attached = MV_FALSE;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp_exp, &associate_enc->expander_list, domain_expander, enclosure_queue_pointer){
			if(tmp_exp == exp)
				attached = MV_TRUE;
		}
		if(attached == MV_TRUE){
			List_Del(&exp->enclosure_queue_pointer);
			exp->enclosure = NULL;
		}
		if(List_Empty(&associate_enc->expander_list))
			pal_set_down_enclosure(root,associate_enc);
	}
#endif

	/* it's not DEVICE_STATUS_FUNCTIONAL */
        MV_ASSERT(get_device_by_id(root->lib_dev, exp->base.id, MV_FALSE, MV_FALSE) != NULL);

	exp->status = 0;
	exp->state = EXP_STATE_IDLE;

	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->expander_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	/* can be FALSE if called by pal_set_down_port */
	if (attached) {
		List_Del(&exp->base.queue_pointer);
		port->expander_count--;
	}

	/* check whether it has been attached to the other lists */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->current_tier, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	if (attached)
		List_Del(&exp->base.queue_pointer);

	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->next_tier, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	if (attached)
		List_Del(&exp->base.queue_pointer);

	/* check whether it has been attached to an expander */
	MV_ASSERT(exp->base.parent != NULL);
	if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		parent_exp = (domain_expander *)exp->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &parent_exp->expander_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)exp) attached = MV_TRUE;
		}
		/* attached can be false */
		if (attached) {
			List_Del(&exp->base.exp_queue_pointer);
			parent_exp->expander_count--;
		}
	}

	core_clean_init_queue_entry(root, &exp->base);

	MV_ASSERT(List_Empty(&exp->device_list));
	MV_ASSERT(List_Empty(&exp->expander_list));
	MV_ASSERT(exp->device_count == 0);
	MV_ASSERT(exp->expander_count == 0);

	core_generate_event(root->core, EVT_ID_EXPANDER_PLUG_OUT, exp->base.id,
			SEVERITY_INFO, 0, NULL ,0);
	CORE_EH_PRINT(("set down expander %d is done\n", exp->base.id));

	free_expander_obj(root, root->lib_rsrc, exp);
}

/* Set disk status to offline, we still have the data structure and link to it. */
void pal_set_disk_offline(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	MV_U8 state = dev->state;

	MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id, MV_FALSE, MV_FALSE) != NULL);
	pal_abort_device_running_req(root, &dev->base);	 	// spin lock ?
	core_complete_device_waiting_queue(root, &dev->base, REQ_STATUS_NO_DEVICE);
	MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id, MV_FALSE, MV_FALSE) != NULL);

	/* it's not DEVICE_STATUS_FUNCTIONAL 
         * so won't be able to send request to it any more */
	dev->status = DEVICE_STATUS_NO_DEVICE;
	dev->state = DEVICE_STATE_INIT_DONE;

	core_clean_init_queue_entry(root, &dev->base);

	if (dev->register_set != NO_REGISTER_SET) {
		CORE_EH_PRINT(("free device %d register set %d\n", \
                        dev->base.id, dev->register_set));
		MV_DASSERT(dev->base.outstanding_req == 0);
		core_free_register_set(root, dev, dev->register_set);
		dev->register_set = NO_REGISTER_SET;
	}
	
	Core_Change_LED(root->core, dev->base.id, LED_FLAG_OFF_ALL);
    LED_DBG_PRINT(("JL %s device %x LED_FLAG_OFF_ALL\n", __FUNCTION__, dev->base.id));

        if(notify_os) {
		core_notify_device_hotplug(root, MV_FALSE, dev->base.id,
			state == DEVICE_STATE_INIT_DONE);
        }

	CORE_EH_PRINT(("set down disk %d\n", dev->base.id));

}

void pal_set_down_error_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
        /* if it's direct attached disk, remove data structure either */
    if(dev->status & DEVICE_STATUS_INTERNAL_HOTPLUG){
        domain_port *port=dev->base.port;
        dev->status &= ~DEVICE_STATUS_INTERNAL_HOTPLUG;
        List_AddTail(&dev->base.queue_pointer, &port->device_list);
    }
    if (!IS_BEHIND_EXP(dev)) {
        pal_set_down_disk(root, dev, notify_os);
    } else {
        pal_set_disk_offline(root, dev, notify_os);
    }

}
#ifdef SUPPORT_MUL_LUN
void pal_set_down_multi_lun_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	domain_port *port = dev->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *exp;
	domain_pm *pm;
	MV_U32 i;
	
	pal_set_disk_offline(root, dev, notify_os);
	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->device_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)dev) attached = MV_TRUE;
	}
	/* can be false if called by pal_set_down_port */
	if (attached) {
		List_Del(&dev->base.queue_pointer);
		port->device_count--;
	}

	/* check whether it has been attached to the expander */
	MV_ASSERT(dev->base.parent != NULL);
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		exp = (domain_expander *)dev->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &exp->device_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)dev) attached = MV_TRUE;
		}
		/* if set down by pal_set_down_expander, maybe not attached anymore */
		if (attached) {
			List_Del(&dev->base.exp_queue_pointer);
			exp->device_count--;
		}
	}

	/* check whether it is attached to the PM */
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
		attached = MV_FALSE;
		pm = (domain_pm *)dev->base.parent;
		for (i = 0; i < CORE_MAX_DEVICE_PER_PM; i++) {
			if (pm->devices[i] == dev) {
				attached = MV_TRUE;
				pm->devices[i] = NULL;
			}
		}
		MV_ASSERT(attached == MV_TRUE);
	}


	free_device_obj(root, root->lib_rsrc, dev);
}
#endif
/* after calling this function, domain_device *dev is not valid any more
 * no matter direct attached or under expander
 */
void pal_set_down_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	domain_port *port = dev->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *exp;
	domain_pm *pm;
	MV_U32 i;

	 
#ifdef SUPPORT_MUL_LUN
	core_extension *core = root->core;
	lib_device_mgr *lib_dev = &core->lib_dev;
	domain_device *dev_multi_lun;
         if((dev->base.multi_lun == MV_TRUE)&& (dev->base.LUN==0)){
		CORE_EH_PRINT(("set down multi lun disk %d\n", dev->base.id));	
	        	for(i=0; i<MAX_ID; i++) {
			domain_base *base;
			base = lib_dev->device_map[i];
			if (base && ( base->TargetID== dev->base.TargetID) && ( base->LUN!=0)){
				dev_multi_lun = (domain_device *)base;
                                   pal_set_down_multi_lun_disk(root, dev_multi_lun, notify_os);
			}				
		} 
         }else if(dev->base.LUN > 0){
         	pal_set_down_multi_lun_disk(root, dev, notify_os);
		return;
         }		
#endif	
	pal_set_disk_offline(root, dev, notify_os);
	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->device_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)dev) attached = MV_TRUE;
	}
	/* can be false if called by pal_set_down_port */
	if (attached) {
		List_Del(&dev->base.queue_pointer);
		port->device_count--;
	}

	/* check whether it has been attached to the expander */
	MV_ASSERT(dev->base.parent != NULL);
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		exp = (domain_expander *)dev->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &exp->device_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)dev) attached = MV_TRUE;
		}
		/* if set down by pal_set_down_expander, maybe not attached anymore */
		if (attached) {
			List_Del(&dev->base.exp_queue_pointer);
			exp->device_count--;
		}
	}

	/* check whether it is attached to the PM */
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
		attached = MV_FALSE;
		pm = (domain_pm *)dev->base.parent;
		for (i = 0; i < CORE_MAX_DEVICE_PER_PM; i++) {
			if (pm->devices[i] == dev) {
				attached = MV_TRUE;
				pm->devices[i] = NULL;
			}
		}
		MV_ASSERT(attached == MV_TRUE);
	}

#ifdef SUPPORT_MUL_LUN
	remove_target_map(root->lib_dev->target_id_map, dev->base.TargetID, MV_MAX_TARGET_NUMBER);
#endif
	free_device_obj(root, root->lib_rsrc, dev);
}

MV_U8 pal_check_disk_exist(void *ext, MV_U16 device_target_id, MV_U16 device_lun)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
	domain_base *base = NULL;
	
#if defined (SUPPORT_MUL_LUN) && defined(_OS_WINDOWS)
	base = get_device_by_id(&core->lib_dev, get_id_by_targetid_lun(core, device_target_id, device_lun), MV_FALSE, MV_FALSE);
#else
	base = get_device_by_id(&core->lib_dev, device_target_id, MV_FALSE, MV_FALSE);
#endif
	if (base == NULL)
		return 0;
	
	return 1;
}
MV_U8 pal_set_down_disk_from_upper(void *ext, MV_U16 device_target_id, MV_U16 device_lun)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
	pl_root *root = NULL;
	domain_base *base = NULL;
	
#if defined (SUPPORT_MUL_LUN) && defined(_OS_WINDOWS)
	base = get_device_by_id(&core->lib_dev, get_id_by_targetid_lun(core, device_target_id, device_lun), MV_FALSE, MV_FALSE);
#else
	base = get_device_by_id(&core->lib_dev, device_target_id, MV_FALSE, MV_FALSE);
#endif
	if (base == NULL)
		return 0;
	
	root = (pl_root *)base->root;
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *device = (domain_device *)base;
#if 0
		if (device->status == DEVICE_STATUS_NO_DEVICE) {
			CORE_DPRINT(("device %d is bad disk\n",base->id));
			return 0;
		}
#else
/* if device->state is not DEVICE_STATE_INIT_DONE, disk is initializing or its state is wrong */
		if(device->state != DEVICE_STATE_INIT_DONE) {
			CORE_DPRINT(("device %d is bad disk \n",base->id));
			return 0;
		}
#endif
	}
	
	if (base->type == BASE_TYPE_DOMAIN_DEVICE)
		pal_set_down_disk(root, (domain_device *)base, MV_FALSE);
#ifdef SUPPORT_SES
	else if (base->type == BASE_TYPE_DOMAIN_ENCLOSURE)
		pal_set_down_enclosure(root, (domain_enclosure *)base);
#endif
	else
		MV_ASSERT(0);
	CORE_DPRINT(("upper layer notify to set down device %d.\n", device_target_id));
	return 0;
}

#ifdef SUPPORT_SES
void pal_set_down_enclosure(pl_root *root, domain_enclosure *enc)
{
	domain_expander *tmp_exp;
	domain_device *tmp_dev;
	domain_base *tmp_base;
	MV_U16 dev_id;

	/*set all associated expander->enclosure = NULL*/
	if(!List_Empty(&enc->expander_list)){
		tmp_exp = List_GetFirstEntry(&enc->expander_list, domain_expander, enclosure_queue_pointer);
		MV_ASSERT(tmp_exp->enclosure == enc);
		tmp_exp->enclosure = NULL;
	}

	/* clean up the enclosure pointer for the attached device */
	for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
		tmp_base = get_device_by_id(root->lib_dev, dev_id, MV_FALSE, MV_FALSE);
		if (tmp_base == NULL || tmp_base->type != BASE_TYPE_DOMAIN_DEVICE)
			continue;
		tmp_dev = (domain_device*)tmp_base;
		if (tmp_dev->enclosure == enc)
			tmp_dev->enclosure = NULL;
	}

	pal_abort_device_running_req(root, &enc->base);

	core_complete_device_waiting_queue(root, &enc->base, REQ_STATUS_NO_DEVICE);

        core_clean_init_queue_entry(root, &enc->base);

	/* it's not DEVICE_STATUS_FUNCTIONAL */
	enc->status = 0;
	enc->state = ENCLOSURE_INQUIRY_DONE;

	core_notify_device_hotplug(root, MV_FALSE, enc->base.id, MV_TRUE);
	CORE_EH_PRINT(("set down enclosure %d\n", enc->base.id));
	
#ifdef SUPPORT_MUL_LUN
	remove_target_map(root->lib_dev->target_id_map, enc->base.TargetID, MV_MAX_TARGET_NUMBER);
#endif
	free_enclosure_obj(root, root->lib_rsrc, enc);
}
#endif

void pal_set_down_pm(pl_root *root, domain_pm *pm, MV_BOOLEAN notify_os)
{
	domain_port *port = pm->base.port;
	MV_U8 i;

	pal_abort_device_running_req(root, &pm->base);
	core_complete_device_waiting_queue(root, &pm->base, REQ_STATUS_NO_DEVICE);

	for (i=0; i<CORE_MAX_DEVICE_PER_PM; i++) {
		if (pm->devices[i])
			pal_set_down_disk(root, pm->devices[i], MV_TRUE);
	}


        core_clean_init_queue_entry(root, &pm->base);

	pm->status = 0;
	pm->state = PM_STATE_RESET_DONE;

	CORE_EH_PRINT(("set down pm %d\n", pm->base.id));

	if (pm->register_set != NO_REGISTER_SET) {
		MV_DASSERT(pm->base.outstanding_req == 0);
		core_free_register_set(root, NULL, pm->register_set);
		pm->register_set = NO_REGISTER_SET;
	}

	MV_ASSERT(port->pm == pm);
	port->pm = NULL;
	free_pm_obj(root, root->lib_rsrc, pm);
}

/* after calling this function, all devices under this port is invalid.
 * dont access them after that. */
void pal_set_down_port(pl_root *root, domain_port *port)
{
	domain_base *base;
	domain_device *dev;
	domain_expander *exp;
	domain_pm   *pm;

	/* supposedly shouldn't happen because only one event will be handled
	 * at a time, and these lists are only used during init/hot plug
	 * process. */
	MV_ASSERT(List_Empty(&port->current_tier));
	MV_ASSERT(List_Empty(&port->next_tier));
//	OSSW_SPIN_LOCK(&port->base.base_SpinLock);
	/* expanders & their devices */
	while(!List_Empty(&port->expander_list)) {
		exp = (domain_expander *)List_GetFirstEntry(&port->expander_list,
			domain_expander, base.queue_pointer);
		pal_set_down_expander(root, exp);
		port->expander_count--;
	}

	/* direct attached, for those without targets */
	while (!List_Empty(&port->device_list)) {
		base = List_GetFirstEntry(
			&port->device_list, domain_base, queue_pointer);
		MV_ASSERT(base->type == BASE_TYPE_DOMAIN_DEVICE);
		dev = (domain_device *)base;
		pal_set_down_disk(root, dev, MV_TRUE);
		port->device_count--;
	}

	/* set down pm if any */
	pm = port->pm;
	if (pm != NULL) {
		pal_set_down_pm(root, pm, MV_TRUE);
		port->pm = NULL;
	}

	MV_ASSERT(port->device_count == 0);
	MV_ASSERT(port->expander_count == 0);
	MV_ASSERT(List_Empty(&port->expander_list));
	MV_ASSERT(List_Empty(&port->device_list));
	MV_ASSERT(port->pm == NULL);
//	OSSW_SPIN_UNLOCK(&port->base.base_SpinLock);
	CORE_EH_PRINT(("set down port %d\n", port->base.id));
}

/* retry we allocate internal request to do the retry
 * if we use original req, we have to do some tricky thing
 * like we have to replace completion routine
 * so it'll be completed twice one with eh callback and
 * one with original completion routine */
MV_Request *core_eh_retry_org_req(pl_root *root, MV_Request *req,
	MV_ReqCompletion func)
{
	MV_Request *eh_req;
	core_context *ctx;
#ifdef SCSI_ID_MAP
    core_extension *core = (core_extension *)root->core;
    PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
    MV_U16 map_id;
#endif
	/* dont allocate any data buffer, use the original */
	eh_req = get_intl_req_resource(root, 0);
	if (eh_req == NULL) return NULL;
	ctx = eh_req->Context[MODULE_CORE];

	eh_req->Device_Id = req->Device_Id;
#ifdef SCSI_ID_MAP
    if (!CORE_IS_ID_MAPPED(req)) {
        map_id = req->Device_Id;
    }
    else {
        map_id = get_internal_id_by_device_id(hba->map_table, req->Device_Id);
    }
    eh_req->Device_Id = map_id;
#endif
	eh_req->Data_Buffer = req->Data_Buffer;
	eh_req->Data_Transfer_Length = req->Data_Transfer_Length;

	MV_CopyMemory(eh_req->Cdb, req->Cdb, MAX_CDB_SIZE);
	eh_req->Cmd_Flag = req->Cmd_Flag;
	eh_req->LBA.value = req->LBA.value;
	eh_req->Sector_Count = req->Sector_Count;

	MV_DASSERT(req->Data_Transfer_Length == req->SG_Table.Byte_Count);
	/* refer to the original SG.
	 * actually cover the whole buffer because offset=0, size=whole size */
	if (req->Data_Transfer_Length > 0) {
		MV_CopyPartialSGTable(
			&eh_req->SG_Table,
			&req->SG_Table,
			0, /* offset */
			req->SG_Table.Byte_Count /* size */
		);
	}
	MV_DASSERT(eh_req->Data_Transfer_Length == eh_req->SG_Table.Byte_Count);

	ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	ctx->u.org.org_req = req;
	ctx->req_type = CORE_REQ_TYPE_RETRY;

	eh_req->Scsi_Status = REQ_STATUS_PENDING;
	eh_req->Completion = func;

	return eh_req;
}
#ifdef SUPPORT_ENHANCED_EH
MV_BOOLEAN port_is_running_error_handling(pl_root *root, domain_port *port)
{
	domain_base *base;

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->device_list, domain_base, queue_pointer) {
		if (base->err_ctx.state == BASE_STATE_ERROR_HANDLING)
			return MV_TRUE;
	}
	

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->expander_list, domain_base, queue_pointer) {
		if (base->err_ctx.state == BASE_STATE_ERROR_HANDLING)
			return MV_TRUE;
	}

	return MV_FALSE;
}
#endif
MV_BOOLEAN port_has_error_req(pl_root *root, domain_port *port)
{
	domain_base *base;

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->device_list, domain_base, queue_pointer) {
		if (base->err_ctx.error_count > 0) return MV_TRUE;
	}

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->expander_list, domain_base, queue_pointer) {
		if (base->err_ctx.error_count > 0) return MV_TRUE;
	}

	if (port->pm &&
		(port->pm->base.err_ctx.error_count > 0)) {
		return MV_TRUE;
	}

	return MV_FALSE;
}

MV_BOOLEAN port_has_init_req(pl_root *root, domain_port *port)
{
	if (port->init_count != 0)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOLEAN device_has_error_req(pl_root *root, domain_base *base)
{
	if (base->err_ctx.error_count > 0)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_Request *core_make_device_reset_req(pl_root *root, domain_port *port,
	domain_device *dev, MV_ReqCompletion func)
{

	MV_Request *req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_RESET_DEVICE;
	req->Cdb[3] = port->phy_map; /* use logical phy_map */
	req->Cmd_Flag = 0;
	req->Device_Id = dev->base.id;
	req->Completion = func;
	return req;
}

MV_Request *core_make_port_reset_req(pl_root *root, domain_port *port,
	MV_PVOID base, MV_ReqCompletion func)
{
	MV_Request *req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_RESET_PORT;
	req->Cdb[3] = port->phy_map; /* use logical phy_map */
	req->Cmd_Flag = 0;
	req->Device_Id = ((domain_base *)base)->id;
	req->Completion = func;
	return req;
}

#ifdef SUPPORT_VU_CMD

/* Set disk status to offline, we still have the data structure and link to it. */
void pal_abort_running_req_for_async_sdb_fis(pl_root *root, domain_device *dev)
{
	MV_Request *req;
	MV_U8 state = dev->state;

        MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id, MV_FALSE) != NULL);
	pal_abort_device_running_req(root, &dev->base);
	core_complete_device_waiting_queue(root, &dev->base, REQ_STATUS_NO_DEVICE);
        MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id, MV_FALSE) != NULL);


	dev->status &= ~DEVICE_STATUS_FUNCTIONAL;
	dev->status |= DEVICE_STATUS_WAIT_ASYNC;
	dev->state = DEVICE_STATE_INIT_DONE;

	if (dev->register_set != NO_REGISTER_SET) {
		CORE_EH_PRINT(("free device %d register set %d\n", \
                        dev->base.id, dev->register_set));
		MV_DASSERT(dev->base.outstanding_req == 0);
		core_free_register_set(root, dev,dev->register_set);
		dev->register_set = NO_REGISTER_SET;
	}

	CORE_PRINT(("abort disk %d running req for async sdb fis.\n", dev->base.id));

}

#endif

