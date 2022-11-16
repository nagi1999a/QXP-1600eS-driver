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
#include "core_sat.h"
#include "core_error.h"
#include "core_util.h"
#include "core_sata.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
#define IS_READ_LOG_EXT_REQ(req) \
	((req->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC)&& \
	 (req->Cdb[1]==CDB_CORE_MODULE)&& \
	 (req->Cdb[2]==CDB_CORE_READ_LOG_EXT))

static MV_BOOLEAN sata_timeout_state_machine(pl_root *root,
        domain_device *dev, MV_BOOLEAN success);
static MV_BOOLEAN sata_media_error_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success);
static void sata_error_handling_callback(pl_root *root, MV_Request *req);
static void pm_error_handling_callback(pl_root *root, MV_Request *req);
extern MV_Request *sata_make_soft_reset_req(domain_port *port,
	domain_device *device, MV_BOOLEAN srst, MV_BOOLEAN is_port_reset,
	MV_PVOID callback);
extern MV_Request *stp_make_phy_reset_req(domain_device *device, MV_U8 operation,
	MV_PVOID callback);
MV_VOID sata_wait_hard_reset(MV_PVOID dev_p, MV_PVOID req_p);
MV_VOID pm_eh_wait(MV_PVOID dev_p, MV_PVOID tmp);

MV_Request *stp_make_report_phy_sata_req(domain_device *dev, MV_PVOID callback);

static MV_BOOLEAN atapi_media_error_state_machine(pl_root *root, 
        domain_device *dev, MV_BOOLEAN success);
static MV_Request *atapi_make_request_sense_req(domain_device *device,
	MV_ReqCompletion func);

/*
 * two cases will call this function
 * 1. sata_error_handler: original error request comes here.
 * 2. sata_error_handling_callback:
 *    a. it's the error handling(eh) request either success or fail
 *    b. retried req will come here only if fails.
 *       if success, sata_error_handling_callback has returned it.
 */
static MV_BOOLEAN sata_timeout_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &dev->base.err_ctx;
	domain_port *port = (domain_port *)dev->base.port;
	MV_Request *eh_req = NULL;
	MV_Request *org_req = err_ctx->error_req;
    #ifdef ASIC_WORKAROUND_WD_TO_RESET
	domain_phy *phy = NULL;
	domain_expander *tmp_exp;
	MV_U8 i;
	MV_U32 reg;
	#endif
	
	MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);
	MV_ASSERT(err_ctx->error_req != NULL);

	CORE_EH_PRINT(("device %d state %d success=0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	/* for sata timeout state machine, only at certain step, we can do the retry
	 * it's different with the sas time out state machine */

	switch (err_ctx->eh_state) {
	case SATA_EH_TIMEOUT_STATE_NONE:
		/* case =
		 * a. original error request
		 * b. media error state machine got timeout */
		err_ctx->timeout_count++;

		// Abort port running request
#ifdef ASIC_WORKAROUND_WD_TO_RESET
		if ((((core_extension *)(root->core))->revision_id != VANIR_C2_REV)
			&& (IS_STP(dev))) {
			pal_abort_port_running_req(root, port);
			
			// Wait
			core_sleep_millisecond(root->core, 10);

			// ASIC STP reset
			for (i = 0; i < dev->base.root->phy_num; i++) {
				if (dev->base.root->phy[i].port == port) {
					phy = &(dev->base.root->phy[i]);
					
					/* disable interrupt */
					phy->phy_irq_mask &= ~(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
					WRITE_PORT_IRQ_MASK(root, phy, phy->phy_irq_mask);
					core_sleep_millisecond(root->core, 10);

					/* reset */

					reg = READ_PORT_PHY_CONTROL(root, phy);
					reg |= (SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET);
					WRITE_PORT_PHY_CONTROL(root, 
						phy,
						reg);
					core_sleep_millisecond(root->core, 50);
					
					/* clear phy change interrupt */
					WRITE_PORT_IRQ_STAT(root, phy, \
						(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0));
					/* enable interrupt */
					phy->phy_irq_mask |= (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
					WRITE_PORT_IRQ_MASK(root, phy, phy->phy_irq_mask);
				}
			}

			// Wait
			core_sleep_millisecond(root->core, 50);

			LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
			  domain_expander, base.queue_pointer) {
			  pal_clean_expander_outstanding_req(root, tmp_exp);
			}
		}
		else {
			pal_abort_device_running_req(root, &dev->base);
		}
#else
		pal_abort_device_running_req(root, &dev->base);
#endif // ASIC_WORKAROUND_WD_TO_RESET


		if (IS_STP(dev)) {
			#ifdef ENABLE_HARD_RESET_EH
			eh_req = stp_make_phy_reset_req(dev, HARD_RESET,
				sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT;
			CORE_EH_PRINT(("device %d sending phy control hard reset request.\n",\
				dev->base.id));
			#else
			eh_req = stp_make_phy_reset_req(dev, LINK_RESET,
				sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT;
			CORE_EH_PRINT(("device %d sending phy control link reset request.\n",\
				dev->base.id));
			#endif //ENABLE_HARD_RESET_EH

		} else {
#ifdef SUPPORT_ENHANCED_EH
			/*directly do hard-reset for direct-attached SATA disk timeout EH.*/
			{
				eh_req = core_make_port_reset_req(root, port, dev,
					(MV_ReqCompletion)sata_error_handling_callback);
				if (eh_req == NULL) {
					return MV_FALSE;
				}
				CORE_EH_PRINT(("device %d sending hard reset request.\n",\
					dev->base.id));
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT;
			}
#else
			eh_req = sata_make_soft_reset_req(
				port, dev, MV_TRUE, MV_FALSE, sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_1;
			CORE_EH_PRINT(("device %d sending soft reset(1) request.\n",\
				dev->base.id));
#endif
		}
		break;

	case SATA_EH_TIMEOUT_STATE_SOFT_RESET_1:
		eh_req = sata_make_soft_reset_req(
			port, dev, MV_FALSE, MV_FALSE, sata_error_handling_callback);
		if (eh_req == NULL) {
			return MV_FALSE;
		}
		err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT;
		CORE_EH_PRINT(("device %d sending soft reset(0) request.\n",\
			dev->base.id));
		break;

    case SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d waiting after soft reset.\n", dev->base.id));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_0;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			err_ctx->eh_timer = core_add_timer(root->core, 10, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		} else {
			/* If reset command self failed, not add delay and go to
			  * below retry flow, but go to higher level error handling
			  * directly.*/
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_0;		
		}

	case SATA_EH_TIMEOUT_STATE_SOFT_RESET_0:
		/* case 
		 * a. soft reset request comes back, either fail or success
		 * b. retried req which failed, (if success, 
		 *    won't come here because it's finished). */
		if (success == MV_TRUE) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req, 
				(MV_ReqCompletion)sata_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
		} 
		else {
			if (IS_STP(dev)) {
				eh_req = stp_make_phy_reset_req(dev, HARD_RESET,
					sata_error_handling_callback);
				if (eh_req == NULL) {
					return MV_FALSE;
				}
				CORE_EH_PRINT(("device %d sending phy control hard reset request.\n",\
					dev->base.id));
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT;
			}
			else {
				eh_req = core_make_port_reset_req(root, port, dev,
					(MV_ReqCompletion)sata_error_handling_callback);
				if (eh_req == NULL) {
					return MV_FALSE;
				}
				CORE_EH_PRINT(("device %d sending hard reset request.\n",\
					dev->base.id));
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT;
			}
		}
		break;

	case SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d waiting after hard reset.\n", dev->base.id));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			err_ctx->eh_timer = core_add_timer(root->core, 10, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		} else {
			/* If reset command self failed, not add delay and go to
			 * below retry flow, but go to higher level error handling
			 * directly.*/
        	err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET;
        }

	case SATA_EH_TIMEOUT_STATE_HARD_RESET:
#ifdef SUPPORT_PORT_RESET_FOR_STP		
		CORE_EH_PRINT(("define for stp --> (device, hard reset) = (%d, %d) \n", dev->base.id, org_req->hard_reset_cnt));
		if (success == MV_TRUE && org_req->hard_reset_cnt < 3) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req, 
				(MV_ReqCompletion)sata_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
            org_req->hard_reset_cnt += 1;
		} 
		else {
			if (IS_STP(dev)) {
				eh_req = core_make_port_reset_req(root, port, &dev->base,
					sata_error_handling_callback);
				if (eh_req == NULL) {
					return MV_FALSE;
				}
				CORE_EH_PRINT(("device %d sending hard reset port request.\n",\
					dev->base.id));
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PORT_RESET;
				port->base.err_ctx.state = BASE_STATE_ERROR_HANDLED;
			}
			else {
#ifndef RAID_DRIVER 
				CORE_EH_PRINT(("device %d: complete req with ERROR\n",dev->base.id));
				dev->status |= DEVICE_STATUS_FROZEN;
				core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
#else
				dev->status |= DEVICE_STATUS_FROZEN;
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SET_DOWN;
				core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

				CORE_EH_PRINT(("device %d set down disk.\n",\
					dev->base.id));

				/* set down disk. after that, dont use dev any more */
				pal_set_down_error_disk(root, dev, MV_TRUE);
#endif
				return MV_TRUE;
			}
		}
		break;
	case SATA_EH_TIMEOUT_STATE_PORT_RESET:
#endif
		/* case =
		 * a. hard reset request comes back
		 * b. retried req which is failed. (if success, won't come to state machine)
		 * c. if time count>MAX_TIMEOUT_ALLOWED, may jump to this step to set down */
		CORE_EH_PRINT(("(device, hard reset) = (%d, %d, %x) \n", dev->base.id, org_req->hard_reset_cnt, org_req));
		if (success == MV_TRUE && org_req->hard_reset_cnt < 3) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req, 
				(MV_ReqCompletion)sata_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
            org_req->hard_reset_cnt += 1;
			break;
		}
		else {
#ifndef RAID_DRIVER 
			CORE_EH_PRINT(("device %d: complete req with ERROR\n",dev->base.id));
			dev->status |= DEVICE_STATUS_FROZEN;
			core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
#else
			dev->status |= DEVICE_STATUS_FROZEN;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SET_DOWN;
			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d set down disk.\n",\
				dev->base.id));

			/* set down disk. after that, dont use dev any more */
			pal_set_down_error_disk(root, dev, MV_TRUE);
#endif
			return MV_TRUE;
		}

	default:
		MV_ASSERT(MV_FALSE);
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root,eh_req);
	return MV_TRUE;
}

extern MV_Request *pm_make_pm_register_req(domain_pm *pm, MV_BOOLEAN op,
	MV_BOOLEAN is_control, MV_U8 pm_port, MV_U16 reg_num, MV_U32 reg_value,
	MV_PVOID callback);

MV_BOOLEAN pm_eh_make_init_ports_reqs(
	pl_root *root,
	domain_pm *pm,
	MV_Request *last_eh_req,
	MV_BOOLEAN reset_device,
	MV_PVOID callback)
{
	struct _error_context *pm_err_ctx = &pm->base.err_ctx;
	MV_Request *eh_req = NULL;
	MV_U32 tmp = 0;
	saved_fis *fis;
	MV_U32 fis1, fis2;
	core_context *ctx = NULL;

	if (pm_err_ctx->eh_state == PM_EH_STATE_NONE) {
		if (!reset_device) {
			pm_err_ctx->pm_eh_active_port = 0;
			pm_err_ctx->eh_state = PM_EH_QRIGA_WORKAROUND;
		}
		else {
			pm_err_ctx->pm_eh_active_port = pm_err_ctx->pm_eh_error_port;
			pm_err_ctx->eh_state = PM_EH_DISABLE_ASYNOTIFY;
		}
	}

	switch (pm_err_ctx->eh_state) {
	/* From pm_state_machine */
	
	case PM_EH_STATE_NONE:
	case PM_EH_HARD_RESET_DONE:
		/* Reserved State for future use */
	case PM_EH_QRIGA_WORKAROUND:
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0, 0x9B,
			0xF0, callback);
		pm_err_ctx->eh_state = PM_EH_ENABLE_FEATURES;
		break;
	
	case PM_EH_ENABLE_FEATURES:
	case PM_EH_DISABLE_ASYNOTIFY:
		// Do not enable Asynchronous Notification Yet.
		// Disables Asynchronous Notification for reset device
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, pm->feature_enabled & (~MV_BIT(3)),
			callback);
		if (!reset_device) {
			pm_err_ctx->eh_state = PM_EH_CLEAR_ERROR_INFO;
		}
		else {
			pm_err_ctx->eh_state = PM_EH_ENABLE_PM_PORT_1;
		}
		break;

	case PM_EH_CLEAR_ERROR_INFO:
		tmp = MV_BIT(16) | MV_BIT(26);
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_ERROR_ENABLE, tmp, callback);
		pm_err_ctx->eh_state = PM_EH_SPIN_UP_ALL_PORTS;
		break;
	
	case PM_EH_SPIN_UP_ALL_PORTS:
	case PM_EH_ENABLE_PM_PORT_1:
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm_err_ctx->pm_eh_active_port,
			PM_PSCR_SCONTROL, 0x01, callback);
		if (!reset_device) {
			pm_err_ctx->pm_eh_active_port++; 
			if (pm_err_ctx->pm_eh_active_port == pm->num_ports) {
				pm_err_ctx->pm_eh_active_port = 0;
				pm_err_ctx->eh_state = PM_EH_SPIN_UP_DONE;
			}
		}
		else {
			pm_err_ctx->eh_state = PM_EH_ENABLE_PM_PORT_0;
		}
		break;
	
	case PM_EH_SPIN_UP_DONE:
	case PM_EH_ENABLE_PM_PORT_0:
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm_err_ctx->pm_eh_active_port,
			PM_PSCR_SCONTROL, 0x00, callback);
		if (!reset_device) {
			pm_err_ctx->pm_eh_active_port++; 
			if (pm_err_ctx->pm_eh_active_port == pm->num_ports) {
				pm_err_ctx->pm_eh_active_port = 0;
				pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY;
			}
		}
		else {
			pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY;
		}
		break;

	case PM_EH_SPIN_UP_WAIT:
		CORE_EH_PRINT(("Waiting after pm hard reset phys.\n"));
		pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY;
		MV_ASSERT(pm_err_ctx->eh_timer == NO_CURRENT_TIMER);
		pm_err_ctx->eh_timer = core_add_timer(root->core, 5, pm_eh_wait, pm, NULL);
		MV_ASSERT(pm_err_ctx->eh_timer != NO_CURRENT_TIMER);
		return MV_TRUE;

	case PM_EH_CHECK_PHY_RDY:
		eh_req = pm_make_pm_register_req(pm, PM_OP_READ, MV_FALSE, pm_err_ctx->pm_eh_active_port,
			PM_PSCR_SSTATUS, 0, callback);
		pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY_DONE;
		pm->pm_port_sig[pm_err_ctx->pm_eh_active_port] = 0xFFFFFFFF;
		break;

	case PM_EH_CHECK_PHY_RDY_DONE:

		MV_ASSERT(last_eh_req != NULL); // Should only be called from callback completion.
		ctx = (core_context *)last_eh_req->Context[MODULE_CORE];
		fis = (saved_fis *)ctx->received_fis;
		if (fis == NULL) MV_ASSERT(MV_FALSE);
		fis1 = fis->dw4;
		fis2 = fis->dw2;

		pm->sstatus = ((MV_U8)fis1) | (((MV_U8)fis2) << 8);
//		pm->pm_port_sig[pm_err_ctx->pm_eh_active_port] = 0xFFFFFFFF;

		if ((pm->sstatus & 0x00F) != 0x003) {
			if (!reset_device) {
				/* 1. Set down this disk if there is one, and not EH disk. Phy is gone. */
				if (pm_err_ctx->pm_eh_active_port != pm_err_ctx->pm_eh_error_port) {
					if (pm->devices[pm_err_ctx->pm_eh_active_port]) {
						//sata_pm_disk_unplug(pm, pm_err_ctx->pm_eh_active_port);
						pal_set_down_disk(pm->base.root, 
							pm->devices[pm_err_ctx->pm_eh_active_port],
							MV_TRUE);
						
					}
				}
				
				/* 2. Move to next port. */
				pm_err_ctx->pm_eh_active_port++;
				if (pm_err_ctx->pm_eh_active_port >= pm->num_ports) {
					pm_err_ctx->pm_eh_active_port = 0xff;
					//pm_err_ctx->eh_state = PM_EH_DONE;
					pm_err_ctx->eh_state = PM_EH_ENABLE_ASYNOTIFY;  // enable asynotify after return.

				} else {
					pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY;
				}
			}
			else {
				//pm_err_ctx->eh_state = PM_EH_DONE;
				pm_err_ctx->eh_state = PM_EH_ENABLE_ASYNOTIFY;  // enable asynotify after return.
			}
			return MV_FALSE;
		}
		pm_err_ctx->eh_state = PM_EH_CLEAR_X_BIT;
		// No Break
	
	case PM_EH_CLEAR_X_BIT:
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm_err_ctx->pm_eh_active_port,
			PM_PSCR_SERROR, 0xFFFFFFFF, callback);
		
#ifdef SUPPORT_QUICK_PM_INIT
		pm_err_ctx->eh_state = PM_EH_WAIT_SIG;
#else
		pm_err_ctx->eh_state = PM_EH_ISSUE_SOFT_RESET_1;
#endif
		break;

	case PM_EH_ISSUE_SOFT_RESET_1:
		eh_req = sata_make_soft_reset_req(pm->base.port,
			pm->devices[pm_err_ctx->pm_eh_active_port], MV_TRUE, MV_FALSE,
			callback);
		pm_err_ctx->eh_state = PM_EH_ISSUE_SOFT_RESET_0;
		break;

	case PM_EH_ISSUE_SOFT_RESET_0:
		core_sleep_millisecond(root->core, 50);
		eh_req = sata_make_soft_reset_req(pm->base.port,
			pm->devices[pm_err_ctx->pm_eh_active_port], MV_FALSE, MV_FALSE,
			callback);
		pm_err_ctx->eh_state = PM_EH_WAIT_SIG;
		break;

	case PM_EH_WAIT_SIG:
		if (pm->pm_port_sig[pm_err_ctx->pm_eh_active_port] == 0xFFFFFFFF) {
			MV_ASSERT(pm_err_ctx->eh_timer == NO_CURRENT_TIMER);
			pm_err_ctx->eh_timer = core_add_timer(root->core, 5, pm_eh_wait, pm, NULL);
			MV_ASSERT(pm_err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		}
		else {
			pm->devices[pm_err_ctx->pm_eh_active_port]->signature =
				pm->pm_port_sig[pm_err_ctx->pm_eh_active_port];
			pm_err_ctx->eh_state = PM_EH_SIG_DONE;
		}

	case PM_EH_SIG_DONE:
		if (!reset_device) {
			pm_err_ctx->pm_eh_active_port++;
			if (pm_err_ctx->pm_eh_active_port >= pm->num_ports) {
				pm_err_ctx->pm_eh_active_port = 0xff;
				pm_err_ctx->eh_state = PM_EH_ENABLE_ASYNOTIFY;
			} else {
				pm_err_ctx->eh_state = PM_EH_CHECK_PHY_RDY;
			}
		}
		else {
			pm_err_ctx->eh_state = PM_EH_ENABLE_ASYNOTIFY;
		}
#if 0 
	case PM_EH_WAIT_AFTER_RESET:
		CORE_EH_PRINT(("Waiting after pm soft reset phys.\n"));
		pm_err_ctx->eh_state = PM_EH_ENABLE_ASYNOTIFY;
		MV_ASSERT(pm_err_ctx->eh_timer == NO_CURRENT_TIMER);
		pm_err_ctx->eh_timer = core_add_timer(root->core, 5, pm_eh_wait, pm, NULL);
		MV_ASSERT(pm_err_ctx->eh_timer != NO_CURRENT_TIMER);
		return MV_TRUE;
#endif

	case PM_EH_ENABLE_ASYNOTIFY:
		// Do not enable Asynchronous Notification Yet.
		eh_req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, pm->feature_enabled | MV_BIT(3),
			callback);
		pm_err_ctx->eh_state = PM_EH_DONE;
		break;

	case PM_EH_ENABLE_ASYNOTIFY_WAIT:
		CORE_EH_PRINT(("Waiting after enable asynchronous notification.\n"));
		pm_err_ctx->eh_state = PM_EH_DONE;
		MV_ASSERT(pm_err_ctx->eh_timer == NO_CURRENT_TIMER);
		pm_err_ctx->eh_timer = core_add_timer(root->core, 5, pm_eh_wait, pm, NULL);
		MV_ASSERT(pm_err_ctx->eh_timer != NO_CURRENT_TIMER);
		return MV_TRUE;

	case PM_EH_DONE:
		return MV_FALSE;
	default:
		MV_ASSERT(MV_FALSE);
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root,eh_req);
	return MV_TRUE;
}

extern void pal_set_down_pm(pl_root *root, domain_pm *pm, MV_BOOLEAN notify_os);

static MV_BOOLEAN 
sata_timeout_state_machine_behind_pm(
	pl_root *root,
	MV_PVOID dev_p,
	MV_BOOLEAN success,
	MV_Request *last_eh_req)
{
	domain_base *base = (domain_base *) dev_p;
	domain_device *dev = NULL;
	domain_pm *pm = NULL;
	struct _error_context *err_ctx = NULL;
	struct _error_context *pm_err_ctx = NULL;
	domain_port *port = (domain_port *)base->port;
	MV_Request *eh_req = NULL;
	MV_Request *org_req = NULL;
	MV_U8 pm_port = 0;
	MV_BOOLEAN pm_eh_return;

	switch (base->type) {
	case BASE_TYPE_DOMAIN_PM:
		pm = (domain_pm *) dev_p;
		pm_err_ctx = &pm->base.err_ctx;
		pm_port = pm_err_ctx->pm_eh_error_port; 
		dev = pm->devices[pm_port];
		err_ctx = &dev->base.err_ctx;

		break;
	
	case BASE_TYPE_DOMAIN_DEVICE:
		dev = (domain_device *) dev_p;
		err_ctx = &dev->base.err_ctx;
		pm = dev->pm;
		pm_port = dev->pm_port;
		pm_err_ctx = &pm->base.err_ctx;

		break;
	
	default:
		MV_ASSERT(MV_FALSE);
	}
	org_req = err_ctx->error_req;

	//MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);
	//MV_ASSERT(err_ctx->error_req != NULL);

	CORE_EH_PRINT(("device %d state %d, pm %d state %d success=0x%x\n",\
		dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state,\
		success));

	/* for sata timeout state machine, only at certain step, we can do the retry
	 * it's different with the sas time out state machine */

	switch (err_ctx->eh_state) {
	case SATA_EH_TIMEOUT_STATE_NONE:

		/* case =
		 * a. original error request
		 * b. media error state machine got timeout */
		err_ctx->timeout_count++;
		MV_ASSERT(pm_err_ctx->eh_state == PM_EH_STATE_NONE);
		MV_ASSERT(pm_err_ctx->state == BASE_STATE_NONE);
		MV_ASSERT(pm_err_ctx->error_req == NULL);
		MV_ASSERT(pm_err_ctx->eh_type == EH_TYPE_NONE);
		pm_err_ctx->state = BASE_STATE_ERROR_HANDLING;
		pm_err_ctx->pm_eh_error_port = pm_port;
		pm_err_ctx->pm_eh_active_port = 0;
		pm_err_ctx->timeout_count++;
		/* Sync Vinar fixing BT-575 */
		//pm_err_ctx->error_req = org_req;
		pm_err_ctx->eh_type = EH_TYPE_TIMEOUT;
		pm_err_ctx->eh_state = PM_EH_STATE_NONE;

		// Abort port running request
		pal_abort_device_running_req(root, &dev->base);

		eh_req = sata_make_soft_reset_req(
			port, dev, MV_TRUE, MV_FALSE, pm_error_handling_callback);
		if (eh_req == NULL) {
			return MV_FALSE;
		}
		err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_1;
		CORE_EH_PRINT(("device %d state %d, pm %d state %d sending soft reset(1) request to device.\n",\
			dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));

		break;
	case SATA_EH_TIMEOUT_STATE_SOFT_RESET_1:
		eh_req = sata_make_soft_reset_req(
			port, dev, MV_FALSE, MV_FALSE, pm_error_handling_callback);
		if (eh_req == NULL) {
			return MV_FALSE;
		}
		err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT;
		CORE_EH_PRINT(("device %d state %d, pm %d state %d sending soft reset(0) request to device.\n",\
			dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
		break;

	case SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d state %d, pm %d state %d waiting after device soft reset.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_0;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			err_ctx->eh_timer = core_add_timer(root->core, 5, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		} else {
			/* If reset command self failed, do not add delay and go to
			 * next state and directly do hard reset.*/
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SOFT_RESET_0;		
		}

	case SATA_EH_TIMEOUT_STATE_SOFT_RESET_0:
		/* case 
		 * a. soft reset request comes back, either fail or success
		 * b. retried req which failed, (if success, 
		 *    won't come here because it's finished). */
		if (success == MV_TRUE) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req, 
				(MV_ReqCompletion)pm_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}

			break;
		}
		else {
			// Continue with Device Hard Reset.		
			success = MV_TRUE;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET;
		}
	case SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET:
		if (success == MV_TRUE) {
			// Reset Device behind PM, issue requests to PM.
			pm_eh_return = pm_eh_make_init_ports_reqs(
				root, pm, last_eh_req, MV_TRUE, pm_error_handling_callback);
			if (pm_err_ctx->eh_state == PM_EH_DONE) {
				pm_err_ctx->eh_state = PM_EH_STATE_NONE;
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET_DONE;
				// Continue to next state
			}
			// Always return here.
			return pm_eh_return;
		}
		else {
		/* Reset Device behind PM failed */
			pm_err_ctx->eh_state = PM_EH_STATE_NONE;
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET_DONE;
		}

	case SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET_DONE:
		if (success == MV_TRUE) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req, 
				(MV_ReqCompletion)pm_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
		}
		/* Hard reset the PM 
		 * if Soft Reset fails or Retry request fails */
		else {
			// dev->base.port port is same as pm->base.port.
			// Abort all requests to PM port.
			pal_abort_port_running_req(root, port);

			// Wait
			core_sleep_millisecond(root->core, 10);

			eh_req = core_make_port_reset_req(root, port, pm,
				(MV_ReqCompletion)pm_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
			CORE_EH_PRINT(("device %d state %d, pm %d state %d sending OOB reset request to pm.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_RESET_WAIT;
		}
		break;
	
	case SATA_EH_TIMEOUT_STATE_PM_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d state %d, pm %d state %d waiting after pm hard reset.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_SRST_1;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			/* Don't need to wait a long time here,
			 * wait is needed after enabling async notification
			 */
			err_ctx->eh_timer = core_add_timer(root->core, 2, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		}
		else {
			/* If reset command self failed, do not add delay and go to
			 * below retry to try reinit PM.*/
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_SRST_1;
		}

	case SATA_EH_TIMEOUT_STATE_PM_SRST_1:
		eh_req = sata_make_soft_reset_req(port, NULL, MV_TRUE, MV_TRUE,
			pm_error_handling_callback);
		if (eh_req == NULL) {
			return MV_FALSE;
		}
		err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_SRST_0;
		CORE_EH_PRINT(("device %d state %d, pm %d state %d sending soft reset(1) request to pm.\n",\
			dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
		break;

	case SATA_EH_TIMEOUT_STATE_PM_SRST_0:
		eh_req = sata_make_soft_reset_req(port, NULL, MV_FALSE, MV_TRUE,
			pm_error_handling_callback);
		if (eh_req == NULL) {
			return MV_FALSE;
		}
		err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_SRST_WAIT;
		CORE_EH_PRINT(("device %d state %d, pm %d state %d sending soft reset(0) request to pm.\n",\
			dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
		break;

	case SATA_EH_TIMEOUT_STATE_PM_SRST_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d state %d, pm %d state %d waiting after pm soft reset.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_REINIT;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			/* Don't need to wait a long time here,
			 * wait is needed after enabling async notification
			 */
			err_ctx->eh_timer = core_add_timer(root->core, 2, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		}
		else {
			/* If reset command self failed, not add delay and go to
			 * below retry flow, but go to higher level error handling
			 * directly.*/
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_PM_REINIT;
        }

	case SATA_EH_TIMEOUT_STATE_PM_REINIT:
		if (success == MV_TRUE) {
			pm_eh_return = pm_eh_make_init_ports_reqs(
				root, pm, last_eh_req, MV_FALSE, pm_error_handling_callback);
			if (pm_err_ctx->eh_state == PM_EH_DONE) {
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_RETRY_ORG_REQ;
				// Continue to next state
			}
			// Always return here.
			return pm_eh_return;
		}
		else {
			/* PM Reset Failed
			 * let's set down the PM and Disks. */

			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SET_DOWN;
			pm_err_ctx->eh_state = PM_EH_STATE_NONE;
			pm_err_ctx->eh_type = EH_TYPE_NONE;
			pm_err_ctx->error_req = NULL;
			pm_err_ctx->state = BASE_STATE_NONE;

			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d state %d, pm %d state %d set down pm.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));

			/* set down pm. after that, dont use pm and attached disks any more */
			pal_set_down_pm(root, pm, MV_TRUE);

			return MV_TRUE;
		}
	case SATA_EH_TIMEOUT_STATE_RETRY_ORG_REQ:
		/* case =
		 * a. hard reset request comes back
		 * b. retried req which is failed. (if success, won't come to state machine)
		 * c. if time count>MAX_TIMEOUT_ALLOWED, may jump to this step to set down */
		if (success == MV_TRUE) {
			/* error handling req success, retry original req */
			eh_req = core_eh_retry_org_req(
				root, org_req,
				(MV_ReqCompletion)pm_error_handling_callback);
			if (eh_req == NULL) {
				return MV_FALSE;
			}
			break;
		}
		else {

			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_SET_DOWN;
			pm_err_ctx->state = BASE_STATE_NONE;
			pm_err_ctx->eh_state = PM_EH_STATE_NONE;
			pm_err_ctx->pm_eh_error_port = 0xff;
			pm_err_ctx->pm_eh_active_port = 0xff;
			pm_err_ctx->error_req = NULL;
			pm_err_ctx->eh_type = EH_TYPE_NONE;
			core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);

			CORE_EH_PRINT(("device %d state %d, pm %d state %d set down disk.\n",\
				dev->base.id, err_ctx->eh_state, pm->base.id, pm_err_ctx->eh_state));

			/* set down disk. after that, dont use dev any more */
			pal_set_down_error_disk(root, dev, MV_TRUE);

			return MV_TRUE;
		}

	default:
		MV_ASSERT(MV_FALSE);
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root,eh_req);
	return MV_TRUE;
}

static MV_BOOLEAN
sata_timeout_error_handler(
	pl_root *root,
	domain_device *dev,
	MV_BOOLEAN success,
	MV_Request *last_eh_req)
{

	if (IS_BEHIND_PM(dev)) {
		return sata_timeout_state_machine_behind_pm(root, dev, success, last_eh_req);
	}
	else {
		return sata_timeout_state_machine(root, dev, success);
	}
}
/*
 * sata_error_handler: in two cases code will come here
 * 1. the original error req is called using handler when error is first hit
 * 2. if in state machine lack of resource, 
 *    the original error request got pushed to the queue again.
 *    see the comments in the function.
 */
MV_BOOLEAN sata_error_handler(MV_PVOID dev_p, MV_Request *req)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	struct _error_context *err_ctx = &dev->base.err_ctx;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d status 0x%x req %p Cdb[0x%x]\n",\
		dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
		err_ctx->scsi_status, req, req->Cdb[0]));
        if(SCSI_IS_READ_WRITE_VERIFY(req->Cdb[0]))
        	CORE_EH_PRINT(("CDB[0]=0x%x LBA=0x%x, 0x%x, Sector_cnt =0x%x\n", req->Cdb[0],req->LBA.parts.high,req->LBA.parts.low,req->Sector_Count));
	/* process_command can only return five status
	 * REQ_STATUS_SUCCESS, XXX_HAS_SENSE, XXX_ERROR, XXX_TIMEOUT, XXX_NO_DEVICE */
	if (err_ctx->eh_type == EH_TYPE_NONE) {
		/* this is the first step for the error handling for this error req. */
		MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
			|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
			|| (req->Scsi_Status == REQ_STATUS_TIMEOUT));
		/* no device shouldn't go to the error handling. */
		MV_ASSERT((err_ctx->error_req == NULL)
			|| (err_ctx->error_req == req)); /* if out of resource */
		err_ctx->error_req = req;
			err_ctx->scsi_status = req->Scsi_Status;

		if (err_ctx->scsi_status == REQ_STATUS_TIMEOUT) {
			if (IS_BEHIND_PM(dev)) {
				domain_pm *pm = dev->pm;
				struct _error_context *pm_err_ctx = &pm->base.err_ctx;

				/* 1. If PM is doing error handling initiated by different
				 *    drive do not start error handling on this device.
				 *    push back the org request to waiting queue.
				 * 2. Drive already in EH should not come here,
				 *    eh_type should not be EH_TYPE_NONE
				 */
				if (pm_err_ctx->state == BASE_STATE_ERROR_HANDLING) {
					/* the eh_type in pm_err_ctx describes the pm_port */
					if (pm_err_ctx->pm_eh_error_port != dev->pm_port) {
						core_push_running_request_back(root, req);
						return MV_TRUE;
					}
				}
			}

			err_ctx->eh_type = EH_TYPE_TIMEOUT;
			#if 0
			if (err_ctx->timeout_count >= MAX_TIMEOUT_ALLOWED) {
				/* will be set down */
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_HARD_RESET;
			} else {
				err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_NONE;
			}
			#else
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_NONE;
			#endif
			return sata_timeout_error_handler(root, dev, MV_FALSE, NULL);
		}
		else {
			err_ctx->eh_type = EH_TYPE_MEDIA_ERROR;
			err_ctx->eh_state = SATA_EH_MEDIA_STATE_NONE;

			if (IS_ATAPI(dev)) {
				return atapi_media_error_state_machine(
					root, dev, MV_FALSE);
			}
			else {
				return sata_media_error_state_machine(
					root, dev, MV_FALSE);
			}
		}
	}
	else {
		/* code comes here if sata_error_handling_callback
		 * or pm_error_handling_callback
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
			return sata_timeout_error_handler(root, dev, 
				(err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE), NULL);
		}
		else {
			if (IS_ATAPI(dev)) {
				return atapi_media_error_state_machine(
					root, dev, 
					(err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
			}
			else {
				return sata_media_error_state_machine(
					root, dev, 
					(err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
			}
		}
	}
}

static MV_Request *sata_make_read_log_ext_req(domain_device *device,
	MV_ReqCompletion func)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, SATA_SCRATCH_BUFFER_SIZE);
	if (req == NULL) {
		return NULL;
	}

	/* We support READ LOG EXT command with log page of 10h. */
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_READ_LOG_EXT;

	req->Device_Id = device->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = func;
	MV_DASSERT(SATA_SCRATCH_BUFFER_SIZE >= sizeof(ata_identify_data));
	MV_DASSERT(req->Data_Transfer_Length % 2 == 0);

	return req;
}

/*
 * media error handling state machine:
 * a. if it's NCQ request, send "read log extent" command;
 * b. retry request but always non-NCQ mode;
 * c. if still has error, return the request with error status.
 */
static MV_BOOLEAN sata_media_error_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	domain_port *port = (domain_port *)dev->base.port;
	MV_Request *eh_req;
	MV_Request *org_req = err_ctx->error_req;
    core_extension *core = (core_extension *)root->core;
    
	MV_ASSERT(err_ctx->eh_type == EH_TYPE_MEDIA_ERROR);
	MV_ASSERT(org_req != NULL);
	MV_ASSERT(!IS_ATAPI(dev));

	CORE_EH_PRINT(("device %d state %d success=0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	/* different with the timeout state machine,
	 * we make the retry as a stage because this state machine is simple
	 * retried request is returned already by callback if success */

	switch (err_ctx->eh_state) {
	case SATA_EH_MEDIA_STATE_NONE:
		/* case: original error request comes failed status */
		MV_ASSERT(success == MV_FALSE);

		pal_abort_device_running_req(root, &dev->base);

#ifdef SUPPORT_READ_LOG_EXT
#ifndef ENABLE_HARD_RESET_EH
		if ((org_req->Cmd_Flag&CMD_FLAG_NCQ)
			&& (dev->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED)) {
			if(!(err_ctx->interrupt_error & INT_NON_SPCFC_NCQ_ERR)){
				/* read log ext */
				eh_req = sata_make_read_log_ext_req(
					dev, (MV_ReqCompletion)sata_error_handling_callback);
			}else{
				/* phy reset*/
				eh_req = core_make_port_reset_req(root, port, dev,
					(MV_ReqCompletion)sata_error_handling_callback);
				err_ctx->interrupt_error = 0;
			}
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state =
				SATA_EH_MEDIA_STATE_READ_LOG_EXTENT;
			break;
		}
#else		
		if (IS_STP(dev)) {
			eh_req = stp_make_phy_reset_req(dev, HARD_RESET,//LINK_RESET,
				sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_MEDIA_STATE_HARD_RESET_WAIT;
			CORE_EH_PRINT(("device %d sending phy control hard/link reset request.\n",\
				dev->base.id));
			break;
 		} else {
			if ((org_req->Cmd_Flag&CMD_FLAG_NCQ)
				&& (dev->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED)) {
				/* read log ext */
				eh_req = sata_make_read_log_ext_req(
						dev, (MV_ReqCompletion)sata_error_handling_callback);
				if (eh_req == NULL) return MV_FALSE;
				err_ctx->eh_state =
					SATA_EH_MEDIA_STATE_READ_LOG_EXTENT;
				break;
			} else {
				/* no break */
			}
 		}
 #endif

	case SATA_EH_MEDIA_STATE_READ_LOG_EXTENT:
		/* case = read log extent comes back, either success or failure */
#else
		/* req =
		 * a. original error request
		 * b. media error state machine got timeout */
		if (IS_STP(dev)) {
			eh_req = stp_make_phy_reset_req(dev, LINK_RESET,	sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_MEDIA_STATE_SOFT_RESET_0;
			CORE_EH_PRINT(("device %d sending phy control link reset request.\n",dev->base.id));
		} else {
			eh_req = sata_make_soft_reset_req(port, dev, MV_TRUE, MV_FALSE, sata_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			err_ctx->eh_state = SATA_EH_MEDIA_STATE_SOFT_RESET_1;
			CORE_EH_PRINT(("device %d sending soft reset(1) request.\n",dev->base.id));
		}

		break;

	case SATA_EH_MEDIA_STATE_SOFT_RESET_1:
		/* req =
		 * a. soft reset request 
		 * b. original error request if out of resource	*/
		eh_req = sata_make_soft_reset_req(port, dev, MV_FALSE, MV_FALSE, sata_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SATA_EH_MEDIA_STATE_SOFT_RESET_0;
		CORE_EH_PRINT(("device %d sending soft reset(0) request.\n",dev->base.id));
		break;

	case SATA_EH_MEDIA_STATE_SOFT_RESET_0:
		/* req =
    	 * a. soft reset request
		 * b. retried req which is failed
		 * c. original request if out of resource */
		if (success != MV_TRUE) {
			core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
			return MV_TRUE;
		} 

		CORE_EH_PRINT(("device %d retry org request %p.\n",dev->base.id, org_req));
#endif
                if (success) {
        	        eh_req = core_eh_retry_org_req(
				    root, org_req, (MV_ReqCompletion)sata_error_handling_callback);
        		    if (eh_req == NULL) return MV_FALSE;
	        	        err_ctx->eh_state = SATA_EH_MEDIA_STATE_RETRY;
                } else {
                        /* put back the error request as timeout to trigger the timeout handling */
                    org_req = err_ctx->error_req;
                    org_req->Scsi_Status = REQ_STATUS_TIMEOUT;

	                err_ctx->eh_type = EH_TYPE_NONE;
	                err_ctx->eh_state = EH_STATE_NONE;
	                err_ctx->error_req = NULL;
                    err_ctx->scsi_status = REQ_STATUS_TIMEOUT;
                    MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
                    MV_ASSERT(err_ctx->timer_id == NO_CURRENT_TIMER);
                    MV_ASSERT(err_ctx->state == BASE_STATE_ERROR_HANDLING);
                    /* clean it to BASE_STATE_NONE, 
                     * otherwise core_handle_error_queue won't continue
                     * refer to core_handle_error_queue */
                    err_ctx->state = BASE_STATE_NONE;

                    //core_queue_error_req(root, org_req, MV_FALSE);
                    Counted_List_Add(&org_req->Queue_Pointer, &core->error_queue);
                    return MV_TRUE;
                }
		break;

#ifdef ENABLE_HARD_RESET_EH
	case SATA_EH_MEDIA_STATE_HARD_RESET_WAIT:
		if (success == MV_TRUE) {
			CORE_EH_PRINT(("device %d waiting after hard reset.\n", dev->base.id));
			err_ctx->eh_state = SATA_EH_MEDIA_STATE_AFTER_RESET;
			MV_ASSERT(err_ctx->eh_timer == NO_CURRENT_TIMER);
			err_ctx->eh_timer = core_add_timer(root->core, 10, sata_wait_hard_reset, dev, NULL);
			MV_ASSERT(err_ctx->eh_timer != NO_CURRENT_TIMER);
		} else {
			/* complete this request no matter success or not */
			core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
		}
		return MV_TRUE;

	case SATA_EH_MEDIA_STATE_AFTER_RESET:
		  if (success) {
        	        eh_req = core_eh_retry_org_req(
				root, org_req, (MV_ReqCompletion)sata_error_handling_callback);
        		if (eh_req == NULL) return MV_FALSE;
	        	err_ctx->eh_state = SATA_EH_MEDIA_STATE_RETRY;
	        	break;
                } else
	        	err_ctx->eh_state = SATA_EH_MEDIA_STATE_RETRY;
                	/*no break, go to following step.*/
#endif
	case SATA_EH_MEDIA_STATE_RETRY:
                /* case: retried command comes here not success */
		MV_ASSERT(success == MV_FALSE);

		/* complete this request no matter success or not */
		core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
		return MV_TRUE;

	default:
		MV_ASSERT(MV_FALSE);
		return MV_FALSE;
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root,eh_req);

	return MV_TRUE;
}

void sata_log_err_cmd(MV_Request *req)
{
    MV_Request *preq;
	struct scsi_cmnd *cmd;
	struct scsi_device *sdev;

    preq = sat_get_org_req(req);
    if(preq == NULL)
    {
        cmd = req->Org_Req_Scmd;
    }
    else
    {
        cmd = preq->Org_Req_Scmd;
    }
    
    if(cmd)
    {
        sdev = cmd->device;
        if(cmd->cmnd[0] == 0xa1)
        {
	        CORE_EH_PRINT(("%s: [%d:%d:%d:%d] %12ph\n", __func__, sdev->host->host_no, sdev->channel, sdev->id, sdev->lun, cmd->cmnd));
        }
        else if(cmd->cmnd[0] == 0x85)
        {
	        CORE_EH_PRINT(("%s: [%d:%d:%d:%d] %16ph\n", __func__, sdev->host->host_no, sdev->channel, sdev->id, sdev->lun, cmd->cmnd));
        }
        else
        {
	        CORE_EH_PRINT(("%s: [%d:%d:%d:%d] %9ph\n", __func__, sdev->host->host_no, sdev->channel, sdev->id, sdev->lun, cmd->cmnd));
        }
    }
    else
    {
	    CORE_EH_PRINT(("%s:scsi_cmd is null\n", __func__));
    }
}

void sata_handle_taskfile_error(pl_root *root, MV_Request *req)
{
	domain_device *dev = NULL;
	MV_U32 reg;
	MV_U8 err_reg, status_reg;
	MV_U8 lba[8];
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	dev = (domain_device *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	if (IS_ATAPI(dev)) {
		req->Scsi_Status = REQ_STATUS_ERROR;
		ctx->error_info |= EH_INFO_NEED_RETRY;
		return;
	}

	reg = MV_REG_READ_DWORD(root->rx_fis,
		SATA_RECEIVED_D2H_FIS(root, dev->register_set));
	status_reg = (MV_U8)((reg >> 16) & 0xff);
	err_reg = (MV_U8)((reg >> 24) & 0xff);

	CORE_EH_PRINT(("device %d task file error, D2H[0]=0x%x.\n", \
		dev->base.id, reg));

    sata_log_err_cmd(req);
	reg = MV_REG_READ_DWORD(root->rx_fis,
		SATA_RECEIVED_D2H_FIS(root, dev->register_set) + 4);
	lba[0] = (MV_U8)(reg & 0xff);
	lba[1] = (MV_U8)((reg >> 8) & 0xff);
	lba[2] = (MV_U8)((reg >> 16) & 0xff);
	reg = MV_REG_READ_DWORD(root->rx_fis,
		SATA_RECEIVED_D2H_FIS(root, dev->register_set) + 8);
	lba[3] = (MV_U8)(reg & 0xff);
	lba[4] = (MV_U8)((reg >> 8) & 0xff);
	lba[5] = (MV_U8)((reg >> 16) & 0xff);
	lba[6] = 0;
	lba[7] = 0;
	CORE_DPRINT(("task file error, status=0x%x, error=0x%x, lba[0-3]=0x%x, "\
		"lba[4-7]=0x%x.\n", status_reg, err_reg,\
		*(MV_PU32)&lba[0], *(MV_PU32)&lba[4]));
	CORE_DPRINT(("request info: \n"));
	MV_DumpRequest(req, MV_FALSE);

	/* If this command is an ATA Passthru then don't retry since
	   it should be "pass thru" with no additional error handling */
	/*Let NCQ command by ATA_PASSThrough also go through 
	   media error state machine - read log ext step, to return the 
	   sense with error info from read log ext command.*/
#if defined(SUPPORT_ROC)
    if (req->Cmd_Flag & CMD_FLAG_NCQ) {
#else
	if (((req->Cdb[0] != SCSI_CMD_ATA_PASSTHRU_12) &&
		(req->Cdb[0] != SCSI_CMD_ATA_PASSTHRU_16) &&
		(req->Cdb[0] != SCSI_CMD_FORMAT_UNIT)) ||
		(req->Cmd_Flag & CMD_FLAG_NCQ)) {
#endif

        ctx->error_info |= EH_INFO_NEED_RETRY;
    }

	if (!(status_reg & (ATA_STATUS_ERR | ATA_STATUS_DF))
		&& (req->Cmd_Flag & CMD_FLAG_NCQ)) {                // try SDB FIS then
		reg = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_SDB_FIS(root, dev->register_set));
		status_reg = (MV_U8) ((reg >> 16) & 0xff);
		err_reg = (MV_U8) (reg >> 24);
	}

	/* set the return status */
	if ((status_reg & (ATA_STATUS_ERR | ATA_STATUS_DF)) && err_reg) {
		sat_make_sense(root, req, status_reg, err_reg,
                        (MV_U64*)&lba[0]);
		/* for format unit */
		if ((req->Cdb[0] == SCSI_CMD_FORMAT_UNIT) &&
			(dev->state == DEVICE_STATE_FORMAT_VERIFY) &&
			(err_reg & ATA_ERR_UNC)) {
			dev->total_formatted_lba_count.value = (*(MV_U64*)&lba[0]).value;
		}
	} else {
		/*
		 * When the status register shows as OK,
		 * the D2H FIS might have been overwritten already.
		 * Therefore we still must handle the error.
		 * Since the sense data will not be available
		 * in this case, we will return a generic error
		 */
		CORE_EH_PRINT(("device %d task file registers shows up OK "\
			 "but still need to handle the error.\n", \
			 dev->base.id));
		req->Scsi_Status = REQ_STATUS_ERROR;
	}

	return;
}

static void pm_error_handling_callback(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base;
	domain_device *dev = NULL;
	domain_pm *pm = NULL;
	struct _error_context *err_ctx = NULL;
	struct _error_context *pm_err_ctx = NULL;
	MV_BOOLEAN ret = MV_TRUE;
	MV_Request *org_req = NULL;

	base = (domain_base *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), MV_FALSE);

	//MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(CORE_IS_EH_REQ(ctx));

	switch(base->type) {
	case BASE_TYPE_DOMAIN_PM:
		pm = (domain_pm *)base;
		 //Request is to PM, so get PM err_ctx
		pm_err_ctx = &base->err_ctx;
		MV_ASSERT(pm_err_ctx->state == BASE_STATE_ERROR_HANDLING);

		/* Sync Vinar fixing BT-575 */
		if (pm->devices[pm_err_ctx->pm_eh_error_port] == NULL)
		{
			pm_err_ctx->eh_state = PM_EH_STATE_NONE;
			pm_err_ctx->eh_type = EH_TYPE_NONE;
			pm_err_ctx->error_req = NULL;
			pm_err_ctx->state = BASE_STATE_NONE;
			
			CORE_EH_PRINT(("The device was set down already!\n"));
		    return;
		}

		err_ctx = &(pm->devices[pm_err_ctx->pm_eh_error_port]->base.err_ctx);
		MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);

		org_req = err_ctx->error_req;
		MV_ASSERT(org_req != NULL);

		ret = sata_timeout_state_machine_behind_pm(root, pm,
			(req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE), req);
		if (ret == MV_FALSE) {
			err_ctx->scsi_status = req->Scsi_Status;
			err_ctx->state = BASE_STATE_NONE;
			/* lack of resource, queue for further handling. */
			core_queue_error_req(root, org_req, MV_FALSE);
		}
		return;
	case BASE_TYPE_DOMAIN_DEVICE:
		dev = (domain_device *)base;
		// device must be behind PM
		MV_ASSERT(IS_BEHIND_PM(dev));
		pm = dev->pm;
		pm_err_ctx = &pm->base.err_ctx;
		MV_ASSERT(pm_err_ctx->state == BASE_STATE_ERROR_HANDLING);
		
		err_ctx = &(pm->devices[pm_err_ctx->pm_eh_error_port]->base.err_ctx);
		MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);

		org_req = err_ctx->error_req;
		MV_ASSERT(org_req != NULL);

		if (pm_err_ctx->pm_eh_error_port != dev->pm_port) {
			/* The device is not the original error device because we may
			   need to soft reset the devices on other PM Ports during pm
			   reset, call the state machine directly with the pm object.
			*/

			ret = sata_timeout_state_machine_behind_pm(root, pm,
				MV_TRUE, req);
			if (ret == MV_FALSE) {
				err_ctx->state = BASE_STATE_NONE;
				err_ctx->scsi_status = REQ_STATUS_SUCCESS;
				/* lack of resource, queue for further handling. */
				core_queue_error_req(root, org_req, MV_FALSE);
			}
			/* function will return at the end of this case */
		}
		else {
			/* continue here only if the Dev is Original Error Device */
			MV_ASSERT(pm_err_ctx->pm_eh_error_port == dev->pm_port);

			//CORE_EH_PRINT(("device %d eh_type %d eh_state %d eh_status 0x%x req %p Cdb[0x%x] status 0x%x\n",
			//	dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
			//	err_ctx->scsi_status, req, req->Cdb[0], req->Scsi_Status));

			if (req->Scsi_Status == REQ_STATUS_NO_DEVICE || 
					(	CORE_IS_EH_RETRY_REQ(ctx) && 
						(	req->Scsi_Status == REQ_STATUS_SUCCESS
							//|| req->Scsi_Status == REQ_STATUS_HAS_SENSE
						)
					)
				) {
				if ((pm_err_ctx->state == BASE_STATE_ERROR_HANDLING)
					&& (pm_err_ctx->pm_eh_error_port == dev->pm_port)) {
						pm_err_ctx->pm_eh_error_port = 0xff;
						pm_err_ctx->pm_eh_active_port = 0xff;
						pm_err_ctx->state = BASE_STATE_NONE;
						pm_err_ctx->eh_state = EH_STATE_NONE;
						pm_err_ctx->error_req = NULL;
						pm_err_ctx->eh_type = EH_TYPE_NONE;
				}
			}

			/* internal request will be released by itself */
			if (req->Scsi_Status == REQ_STATUS_NO_DEVICE) {
				/* complete the original request */
				core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);
				goto end_point;
			} else if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
				/* no matter it's the retried req or the eh req */
				if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
					err_ctx->eh_type = EH_TYPE_TIMEOUT;
					err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_NONE;
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
					/* Let State Machine complete non successful request
					 * if we complete request here it will block state machine
					 * from setting device down
					 */
					//core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
					//goto end_point;
				}
			}

			MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
				|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
				|| (req->Scsi_Status == REQ_STATUS_TIMEOUT)
				|| (req->Scsi_Status == REQ_STATUS_SUCCESS));

			ret = sata_timeout_error_handler(root, dev, 
				(req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE), req);
end_point:
			if (ret == MV_FALSE) {
				/* save the eh request status because req will be released */
				err_ctx->scsi_status = req->Scsi_Status;
				err_ctx->state = BASE_STATE_NONE;
				/* lack of resource, queue for further handling. */
				core_queue_error_req(root, org_req, MV_FALSE);
			}
		}
		return;
	default:
		/* We do not expect any other device types to come to the callback */
		MV_ASSERT(MV_FALSE);
	}

}

#ifdef SUPPORT_PHY_POWER_MODE
extern void sata_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
#endif
static void sata_error_handling_callback(pl_root *root, MV_Request *req)
{
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
    err_ctx = &dev->base.err_ctx;
    org_req = err_ctx->error_req;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(CORE_IS_EH_REQ(ctx));
	MV_ASSERT((err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
		|| (err_ctx->eh_type == EH_TYPE_TIMEOUT));
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d eh_status 0x%x req %p Cdb[0x%x] status 0x%x\n",\
		dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
		err_ctx->scsi_status, req, req->Cdb[0], req->Scsi_Status));

	/* just print out the read log extent information */
	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC)
		&& (req->Cdb[1] == CDB_CORE_MODULE)
		&& (req->Cdb[2] == CDB_CORE_READ_LOG_EXT)) {
#if 0
		MV_U8 i,j;
		MV_U8 temp_buffer[17];
		/* Based on the Linux kernel Libata-scsi.c */
		static const unsigned char sense_table[][4] = {
			{0xd1,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Device busy					Aborted command
			{0xd0,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Device busy					Aborted command
			{0x61,		SCSI_SK_HARDWARE_ERROR, 0x00, 0x00},	// Device fault 				Hardware error
			{0x84,		SCSI_SK_ABORTED_COMMAND, 0x47, 0x00},	// Data CRC error				SCSI parity error
			{0x37,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Unit offline 				Not ready
			{0x09,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Unrecovered disk error		Not ready
			{0x01,		SCSI_SK_MEDIUM_ERROR, 0x13, 0x00},	// Address mark not found		Address mark not found for data field
			{0x02,		SCSI_SK_HARDWARE_ERROR, 0x00, 0x00},	// Track 0 not found		  Hardware error
			{0x04,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Aborted command				Aborted command
			{0x08,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Media change request   FIXME: faking offline
			{0x10,		SCSI_SK_ABORTED_COMMAND, 0x14, 0x00},	// ID not found 				Recorded entity not found
			{0x08,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Media change 	  FIXME: faking offline
			{0x40,		SCSI_SK_MEDIUM_ERROR, 0x11, 0x04},	// Uncorrectable ECC error		Unrecovered read error
			{0x80,		SCSI_SK_MEDIUM_ERROR, 0x11, 0x04},	// Block marked bad 	  Medium error, unrecovered read error
			{0xFF, 0xFF, 0xFF, 0xFF}, // END mark
		};
				
        MV_PU8 buf_ptr = core_map_data_buffer(req);
		MV_DASSERT(buf_ptr != NULL);
		if ((buf_ptr != NULL) && (req->Data_Transfer_Length >= 16)) {
			CORE_EH_PRINT(("read log ext page 0x10: \n"));
			for (i = 0; i < 16; i++) {
				CORE_PLAIN_DPRINT((" %x", buf_ptr[i]));
			}
			CORE_PLAIN_DPRINT(("\n"));
			CORE_EH_PRINT(("the original request:\n"));
			MV_DumpRequest(org_req, MV_FALSE);
		}
		
		if (buf_ptr[2] & (1 << 7)) {
			buf_ptr[3] = 0;	/* Ignore the err bits, they're invalid */
		}
		
		for (j=0; j<17; j++)
			temp_buffer[j] = 0;

		temp_buffer[0] = 0x72;
		if (buf_ptr[3]) {
			for (j = 0; sense_table[j][0] != 0xFF; j++) {
				if ((sense_table[j][0] & buf_ptr[3]) == sense_table[j][0]) {
					temp_buffer[1] = sense_table[j][1];
					temp_buffer[2] = sense_table[j][2];
					temp_buffer[3] = sense_table[j][3];
				}
			}
			temp_buffer[1] &= 0x0F;
		}
		temp_buffer[7] = 12;   /*ADDITIONAL SENSE LENGTH*/
		temp_buffer[8] = 0x00; /*DESCRIPTOR TYPE*/
		temp_buffer[9] = 10;   /*ADDITIONAL LENGTH = 12-2 */
		
		temp_buffer[10] |= 0x80;  /* valid */
		temp_buffer[11] = buf_ptr[10]; /*LBA 48:24*/
		temp_buffer[12] = buf_ptr[9];
		temp_buffer[13] = buf_ptr[8];
		
		temp_buffer[14] = buf_ptr[6];  /*LBA 24:0*/
		temp_buffer[15] = buf_ptr[5];
		temp_buffer[16] = buf_ptr[4];		
		if( org_req->Sense_Info_Buffer != NULL )
			MV_CopyMemory(org_req->Sense_Info_Buffer,temp_buffer,17);
		
        core_unmap_data_buffer(req);
#else
	if ( (org_req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12) ||
		(org_req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16) ) {
		/*update received fis from read log ext data for ATA_Passthrough command, 
		   scsi_ata_ata_passthru_callback will make the sense data.*/
		core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];
		saved_fis *fis = (saved_fis *)org_ctx->received_fis;
		//CORE_DPRINT(("old fis:%x %x %x %x\n", fis->dw1, fis->dw2, fis->dw3, fis->dw4));
		if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
	        	MV_PU32 buf_ptr = (MV_PU32)core_map_data_buffer(req);
			fis->dw1 = buf_ptr[0];
			fis->dw2 = buf_ptr[1];
			fis->dw3 = buf_ptr[2];
			fis->dw4 = buf_ptr[3];
			org_ctx->received_fis = (MV_PVOID)fis;
			core_unmap_data_buffer(req);
			//CORE_DPRINT(("new fis:%x %x %x %x\n", fis->dw1, fis->dw2, fis->dw3, fis->dw4));

			org_req->Scsi_Status = REQ_STATUS_HAS_SENSE;

			core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			goto end_point;
		}

	} else {
		/*make sense data based on read log ext data for the failed NCQ command.*/
		MV_U8 i,j;

//Follow synology request to modify sense data information arranged format. (matched SPEC)
#if 0
		MV_U8 temp_buffer[17];
#else
		MV_U8 temp_buffer[20];
#endif
		/* Based on the Linux kernel Libata-scsi.c */
		static const unsigned char sense_table[][4] = {
			{0xd1,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Device busy					Aborted command
			{0xd0,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Device busy					Aborted command
			{0x61,		SCSI_SK_HARDWARE_ERROR, 0x00, 0x00},	// Device fault 				Hardware error
			{0x84,		SCSI_SK_ABORTED_COMMAND, 0x47, 0x00},	// Data CRC error				SCSI parity error
			{0x37,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Unit offline 				Not ready
			{0x09,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Unrecovered disk error		Not ready
			{0x01,		SCSI_SK_MEDIUM_ERROR, 0x13, 0x00},	// Address mark not found		Address mark not found for data field
			{0x02,		SCSI_SK_HARDWARE_ERROR, 0x00, 0x00},	// Track 0 not found		  Hardware error
			{0x04,		SCSI_SK_ABORTED_COMMAND, 0x00, 0x00},	// Aborted command				Aborted command
			{0x08,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Media change request   FIXME: faking offline
			{0x10,		SCSI_SK_ABORTED_COMMAND, 0x14, 0x00},	// ID not found 				Recorded entity not found
			{0x08,		SCSI_SK_NOT_READY, 0x04, 0x00}, 	// Media change 	  FIXME: faking offline
			{0x40,		SCSI_SK_MEDIUM_ERROR, 0x11, 0x04},	// Uncorrectable ECC error		Unrecovered read error
			{0x80,		SCSI_SK_MEDIUM_ERROR, 0x11, 0x04},	// Block marked bad 	  Medium error, unrecovered read error
			{0xFF, 0xFF, 0xFF, 0xFF}, // END mark
		};
				
        MV_PU8 buf_ptr = core_map_data_buffer(req);
        if (!buf_ptr)
            MV_ASSERT(MV_FALSE);
            
		if ((buf_ptr != NULL) && (req->Data_Transfer_Length >= 16)) {
			CORE_EH_PRINT(("read log ext page 0x10: \n"));
			for (i = 0; i < 16; i++) {
				CORE_PLAIN_DPRINT((" %x", buf_ptr[i]));
			}
			CORE_PLAIN_DPRINT(("\n"));
			CORE_EH_PRINT(("the original request:\n"));
			MV_DumpRequest(org_req, MV_FALSE);
		}
		if (buf_ptr[2] & (1 << 7)) {
			buf_ptr[3] = 0;	/* Ignore the err bits, they're invalid */
		}
#if 0
		for (j=0; j<17; j++)
			temp_buffer[j] = 0;
#else
		MV_ZeroMemory(temp_buffer, sizeof(temp_buffer));
#endif


		temp_buffer[0] = 0x72;
		if (buf_ptr[3]) {
			for (j = 0; sense_table[j][0] != 0xFF; j++) {
				if ((sense_table[j][0] & buf_ptr[3]) == sense_table[j][0]) {
					temp_buffer[1] = sense_table[j][1];
					temp_buffer[2] = sense_table[j][2];
					temp_buffer[3] = sense_table[j][3];
				}
			}
			temp_buffer[1] &= 0x0F;
		}
		temp_buffer[7] = 12;   /*ADDITIONAL SENSE LENGTH*/
		temp_buffer[8] = 0x00; /*DESCRIPTOR TYPE*/
		temp_buffer[9] = 10;   /*ADDITIONAL LENGTH = 12-2 */
		
		temp_buffer[10] |= 0x80;  /* valid */
#if 0	
		temp_buffer[11] = buf_ptr[10]; /*LBA 48:24*/
		temp_buffer[12] = buf_ptr[9];
		temp_buffer[13] = buf_ptr[8];
		
		temp_buffer[14] = buf_ptr[6];  /*LBA 24:0*/
		temp_buffer[15] = buf_ptr[5];
		temp_buffer[16] = buf_ptr[4];		
#else
		temp_buffer[11] = 0x0;
		temp_buffer[12] = 0x0;
		temp_buffer[13] = 0x0;
		
		temp_buffer[14] = buf_ptr[10];  /*LBA 48:24*/
		temp_buffer[15] = buf_ptr[9];
		temp_buffer[16] = buf_ptr[8];	
		
		temp_buffer[17] = buf_ptr[6];   /*LBA 24:0*/
		temp_buffer[18] = buf_ptr[5];		
		temp_buffer[19] = buf_ptr[4];		
#endif

		if( org_req->Sense_Info_Buffer != NULL )
#if 0	
			MV_CopyMemory(org_req->Sense_Info_Buffer,temp_buffer,17);
#else
			MV_CopyMemory(org_req->Sense_Info_Buffer,temp_buffer,sizeof(temp_buffer));
#endif

		
        	core_unmap_data_buffer(req);
        	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
			org_req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			goto end_point;
		}
	}

#endif

    }
#ifdef SUPPORT_PHY_POWER_MODE
	if((core->PHY_power_mode_DIPM || core->PHY_power_mode_HIPM) &&
		req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && req->Cdb[2] == CDB_CORE_RESET_PORT){
		port = dev->base.port;
		if(port && !port->pm && IS_SATA(dev)){
			phy = port->phy;
			if(phy && !mv_is_phy_ready(root,phy)){
				req->Scsi_Status = REQ_STATUS_NO_DEVICE;
				core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);
				//Core_Enable_PHY_Interrupt(phy);
				MV_PRINT("root %x phy %x set down drive!!!\n",root->root_id,phy->id);
				core->PHY_power_mode_port_enabled &= ~(1<<(root->base_phy_num + phy->asic_id));//record the device set down and disabled DIPM or HIPM to enable hot plug!
				sata_handle_unplug(root,port,phy);
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
			err_ctx->eh_state = SATA_EH_TIMEOUT_STATE_NONE;
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
		    if (org_req->Sense_Info_Buffer) {
    			/* copy sense code */
    			MV_CopyMemory(org_req->Sense_Info_Buffer,
    				req->Sense_Info_Buffer,
    				MV_MIN(org_req->Sense_Info_Buffer_Length,
    				req->Sense_Info_Buffer_Length));
			}
			/* Let State Machine complete non successful request
			 * if we complete request here it will block state machine
			 * from setting device down
			 */

			//core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			//goto end_point;
		}
	}

	MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
		|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
		|| (req->Scsi_Status == REQ_STATUS_TIMEOUT)
		|| (req->Scsi_Status == REQ_STATUS_SUCCESS));

	if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
		if (IS_ATAPI(dev)) {
			/* copy sense data if necessary */
			if ((req->Cdb[0] == SCSI_CMD_REQUEST_SENSE)
				&& (req->Scsi_Status == REQ_STATUS_SUCCESS)) {
				MV_PVOID buf_ptr = core_map_data_buffer(req);
				/* copy sense code */
				MV_CopyMemory(org_req->Sense_Info_Buffer,
					buf_ptr,
					MV_MIN(org_req->Sense_Info_Buffer_Length,
					req->Data_Transfer_Length));

				org_req->Scsi_Status = REQ_STATUS_HAS_SENSE;
				core_unmap_data_buffer(req);
			}

			ret = atapi_media_error_state_machine(root, dev, 
				(req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		}
		else {
			ret = sata_media_error_state_machine(root, dev,
				(req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		}
	}
	else {
		/* For non-NCQ command has sense,
		* driver need return request with sense.
		* After retry command failed, we need complete sense command in there.
		* In timeout handler, it may complete command with wrong status.
		*/
		if ((req->Scsi_Status == REQ_STATUS_HAS_SENSE)
				&& (CORE_IS_EH_RETRY_REQ(ctx))) {
			core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			goto end_point;
		}
		
		ret = sata_timeout_error_handler(root, dev, 
			(req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE), req);
	}

end_point:
	if (ret == MV_FALSE) {
		/* save the eh request status because req will be released */
		err_ctx->scsi_status = req->Scsi_Status;
		err_ctx->state = BASE_STATE_NONE;
		/* lack of resource, queue for further handling. */
		core_queue_error_req(root, org_req, MV_FALSE);
	}
}

static MV_BOOLEAN atapi_media_error_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	MV_Request *eh_req;
	MV_Request *org_req = err_ctx->error_req;

	MV_DASSERT(err_ctx->eh_type == EH_TYPE_MEDIA_ERROR);
	MV_DASSERT(org_req != NULL);
        /* it's media error only */

	switch (err_ctx->eh_state) {
	case ATAPI_EH_MEDIA_STATE_NONE:
		/* case: the error request, first step */
		MV_DASSERT(success == MV_FALSE);

		eh_req = atapi_make_request_sense_req(
				dev, (MV_ReqCompletion)sata_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		core_queue_eh_req(root, eh_req);
		err_ctx->eh_state =
			ATAPI_EH_MEDIA_STATE_REQUEST_SENSE;
		break;

	case ATAPI_EH_MEDIA_STATE_REQUEST_SENSE:
		/* req = request sense request comes back */
		if (success == MV_TRUE) {
                        /* sense buffer has copied in the callback */
			MV_DASSERT(org_req->Scsi_Status == REQ_STATUS_HAS_SENSE);
		} else {
			org_req->Scsi_Status = REQ_STATUS_ERROR;
		}

		/* complete this request no matter success or not */
		core_complete_error_req(root, org_req, org_req->Scsi_Status);
		break;

	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

	return MV_TRUE;
}

static MV_Request *atapi_make_request_sense_req(domain_device *device,
	MV_ReqCompletion func)
{
	pl_root *root = device->base.root;
	MV_Request *req;
	MV_U8 sense_size = 18;

	req = get_intl_req_resource(root, sense_size);
	if (req == NULL) {
		return NULL;
	}

	req->Cdb[0] = SCSI_CMD_REQUEST_SENSE;
	req->Cdb[4] = sense_size;
	req->Device_Id = device->base.id;
	req->Completion = func;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;

	return req;
}

MV_VOID sata_wait_hard_reset(MV_PVOID dev_p, MV_PVOID tmp)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_BOOLEAN ret = MV_FALSE; 
	struct _error_context *err_ctx = &dev->base.err_ctx;
	MV_ULONG flags;
	core_extension *core = dev->base.root->core;
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(dev->base.root->core, &flags);
	err_ctx->eh_timer = NO_CURRENT_TIMER;        
	if (err_ctx->eh_type == EH_TYPE_TIMEOUT)
		ret = sata_timeout_error_handler(root, dev, MV_TRUE, NULL);
	else if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
		ret = sata_media_error_state_machine(root, dev, MV_TRUE);
	if (ret == MV_FALSE) {
		/* save the eh request status because req will be released */
		err_ctx->scsi_status = REQ_STATUS_SUCCESS;
		err_ctx->state = BASE_STATE_NONE;
		/* lack of resource, queue for further handling. */
		core_queue_error_req(root, err_ctx->error_req, MV_FALSE);
	}
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(dev->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}

MV_VOID pm_eh_wait(MV_PVOID dev_p, MV_PVOID tmp)
{
	domain_pm *pm = (domain_pm *)dev_p;
	domain_device *dev;
	pl_root *root = pm->base.root;
	MV_BOOLEAN ret = MV_FALSE, get_sig = MV_TRUE; 
	struct _error_context *pm_err_ctx = &pm->base.err_ctx;
	struct _error_context *err_ctx;
	MV_ULONG flags;
	core_extension *core = root->core;
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	dev = pm->devices[pm_err_ctx->pm_eh_error_port];
	if (dev == NULL) {
		/* device has been hotplugged during wait.
		 * error req should have been completed in pal_set_down_disk
		 */
//		core_global_unlock(root->core, &flags);
//		spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
		return;
	}
	err_ctx	= &(dev->base.err_ctx);

	if (pm_err_ctx->eh_state == PM_EH_WAIT_SIG) {
		if (pm->pm_port_sig[pm_err_ctx->pm_eh_active_port] == 0xFFFFFFFF)
			get_sig = MV_FALSE;
	}

	pm_err_ctx->eh_timer = NO_CURRENT_TIMER;        
	if (err_ctx->eh_type == EH_TYPE_TIMEOUT)
		ret = sata_timeout_error_handler(root, dev, get_sig, NULL);
	else {
		// right now not possible to trigger PM EH from media error
		MV_ASSERT(MV_FALSE);
	}
	if (ret == MV_FALSE) {
		/* save the eh request status because req will be released */
		err_ctx->scsi_status = REQ_STATUS_SUCCESS;
		err_ctx->state = BASE_STATE_NONE;
		/* lack of resource, queue for further handling. */
		core_queue_error_req(root, err_ctx->error_req, MV_FALSE);
	}
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}
