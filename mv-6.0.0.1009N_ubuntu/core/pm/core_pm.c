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
#include "core_protocol.h"
#include "core_sat.h"
#include "core_pm.h"
#include "core_util.h"
#include "core_device.h"
#include "core_error.h"
#include "core_resource.h"
#include "core_sata.h"
#include "core_manager.h"
extern MV_VOID sata_prepare_command_header(MV_Request *req,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_U8 tag, MV_U8 pm_port);
extern MV_VOID sata_prepare_command_table(MV_Request *req,
	mv_command_table *cmd_table, ata_taskfile *taskfile, MV_U8 pm_port);
extern MV_Request *sata_make_soft_reset_req(domain_port *port,
	domain_device *device, MV_BOOLEAN srst, MV_BOOLEAN is_port_reset,
	MV_PVOID callback);
extern MV_VOID sata_set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device);
extern void get_new_and_update_base_id(pl_root *root,domain_base *base);

MV_VOID sata_pm_disk_unplug(domain_pm *pm, MV_U8 unplug_index)
{
	domain_device *device = pm->devices[unplug_index];
	pl_root *root = pm->base.root;

	pal_set_down_disk(root, device, MV_TRUE);

	//hal_clear_srs_irq(root, 0, MV_TRUE);
}

MV_VOID pm_wait_for_spinup(MV_PVOID pm_p, MV_PVOID tmp)
{
	MV_ULONG flags;
	domain_pm *pm = (domain_pm *)pm_p;
	core_extension *core = pm->base.root->core;

//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(pm->base.root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	pm->state = PM_STATE_SPIN_UP_DONE;
	core_queue_init_entry(pm->base.root, &pm->base, MV_FALSE);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(pm->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}

/* no lock */
MV_VOID _pm_sig_time_out(domain_pm *pm, MV_PVOID tmp)
{
#ifdef SUPPORT_QUICK_PM_INIT
	if (pm->pm_port_sig[pm->active_port] == 0xEB140101){
		pm->devices[pm->active_port]->connection |= DC_ATAPI;
		pm->devices[pm->active_port]->dev_type = DT_CD_DVD;

	} else{
		pm->devices[pm->active_port]->dev_type = DT_DIRECT_ACCESS_BLOCK;
	}
	if ((pm->pm_port_sig[pm->active_port] == 0xFFFFFFFF) && (pm->srst_retry_cnt)) {
		pm->srst_retry_cnt--;		
		pm->state = PM_STATE_ENABLE_PM_PORT_1;
	}
	else{
		pm->devices[pm->active_port]->signature=pm->pm_port_sig[pm->active_port];
		pm->state = PM_STATE_SIG_DONE;
	}
	pm->sata_sig_timer = NO_CURRENT_TIMER;
	core_queue_init_entry(pm->base.root, &pm->base, MV_FALSE);
#else
	/*
	 * Signature FIS did not return. We proceed with initialization anyway
	 * (sometimes the drive can still recover. If not, we will deal with
	 * it during time out handling)
	 */
	pm->state = PM_STATE_SIG_DONE;
	pm->sata_sig_timer = NO_CURRENT_TIMER;
	core_queue_init_entry(pm->base.root, &pm->base, MV_FALSE);
#endif
}

MV_VOID pm_sig_time_out(domain_pm *pm, MV_PVOID tmp)
{
	MV_ULONG flags;
	core_extension *core = pm->base.root->core;

//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(pm->base.root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	_pm_sig_time_out(pm, tmp);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(pm->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}

MV_VOID pm_init_req_callback(MV_PVOID ext, MV_Request *req)
{
	pl_root *root = (pl_root *)ext;
	domain_pm *pm;
	domain_device *device = NULL;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	saved_fis *fis;
	MV_U32 fis1=0, fis2=0;
	MV_U16 timer_id;

	if ((req->Cdb[2] == CDB_CORE_SOFT_RESET_1) ||
		(req->Cdb[2] == CDB_CORE_SOFT_RESET_0)) {
		device = (domain_device *)get_device_by_id(
			root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
		pm = device->pm;

		if ((pm->status & PM_STATUS_DEVICE_PHY_RESET) &&
			(req->Cdb[2] == CDB_CORE_SOFT_RESET_0)) {
			pm->state = PM_STATE_ENABLE_ASYNOTIFY;
			core_queue_init_entry(root, &pm->base, MV_FALSE);
			return;
		}

		if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
			if (pm->status & PM_STATUS_HOT_PLUG) {
				pm->state = PM_STATE_SIG_DONE;
				core_queue_init_entry(root, &pm->base, MV_FALSE);
			}
			else {
				core_handle_init_error(root, &device->base, req);
			}
			return;
		}

	} else {
		pm = (domain_pm *)get_device_by_id(root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);

		if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
			core_handle_init_error(root, &pm->base, req);
			return;
		}
	}

	if ((pm->state != PM_STATE_ISSUE_SOFT_RESET_1) &&
		(pm->state != PM_STATE_ISSUE_SOFT_RESET_0)) {

		fis = (saved_fis *)ctx->received_fis;
		if (fis == NULL) MV_ASSERT(MV_FALSE);
		fis1 = fis->dw4;
		fis2 = fis->dw2;
	}
	switch (pm->state) {
	case PM_STATE_RESET_DONE:
		if (fis1 <= CORE_MAX_DEVICE_PER_PM)
			pm->num_ports = (MV_U8)fis1;
		else
			pm->num_ports = CORE_MAX_DEVICE_PER_PM;
		pm->state = PM_STATE_READ_INFO_DONE;
		break;
	case PM_STATE_READ_INFO_DONE:
		pm->vendor_id = ((MV_U8)fis1) | (((MV_U8)fis2) << 8);
		pm->device_id = (MV_U16)(fis2 >> 8);
		pm->state = PM_STATE_READ_ID_DONE;
		if((pm->vendor_id==0x11ab)&&(pm->device_id==0x4140)){
			if(pm->num_ports==0x05){
				pm->num_ports=0x04;
				}
			pm->state = PM_STATE_QRIGA_WORKAROUND;
		} else/*((pm->vendor_id==0x1B4B)
		   &&((pm->device_id==0x9715)
		   || (pm->device_id==0x9705)
		   || (pm->device_id==0x9713)
		   || (pm->device_id==0x9703)))*/{			
			pm->state = PM_STATE_AKUPA_WORKAROUND;	
			pm->autoload_index = 1;
			pm->pm_index = 0xff;
		}
		break;
	case PM_STATE_QRIGA_WORKAROUND:
		pm->state = PM_STATE_READ_ID_DONE;
		break;
	case PM_STATE_AKUPA_WORKAROUND:
		pm->autoload_index +=2;		
		if (support_akupa_pm_devs[pm->pm_index].autoload[pm->autoload_index] == 0xFFFFFFFFL) {
			pm->autoload_index = 1;
			pm->pm_index = 0xff;
			pm->state = PM_STATE_READ_ID_DONE;
		}		
		break;	
	case PM_STATE_READ_ID_DONE:
		pm->product_rev = (MV_U8)fis2;
		if (fis1 & MV_BIT(3))
			pm->spec_rev = 12;
		else if (fis1 & MV_BIT(2))
			pm->spec_rev = 11;
		else if (fis1 & MV_BIT(1))
			pm->spec_rev = 10;
		else
			pm->spec_rev = 0;
		pm->state = PM_STATE_READ_REV_DONE;
		break;
	case PM_STATE_READ_REV_DONE:
		pm->feature_enabled = (MV_U8)(fis1 | MV_BIT(3));
		pm->state = PM_STATE_ENABLE_FEATURES;
		break;
	case PM_STATE_ENABLE_FEATURES:
		pm->state = PM_STATE_CLEAR_ERROR_INFO;
		break;
	case PM_STATE_CLEAR_ERROR_INFO:
		MV_DASSERT(pm->active_port == 0);
#ifdef SUPPORT_QUICK_PM_INIT
		pm->retry_cnt = 3;	/* give 3 ms to check phy rdy */
		pm->srst_retry_cnt = 1;
#endif
		pm->state = PM_STATE_SPIN_UP_ALL_PORTS;
		break;
	case PM_STATE_SPIN_UP_ALL_PORTS:
		pm->active_port++;
		if (pm->active_port >= pm->num_ports) {
			pm->active_port = 0;
			timer_id = core_add_timer(root->core, 1, pm_wait_for_spinup, pm, NULL);
			MV_ASSERT(timer_id != NO_CURRENT_TIMER);
			return;
		}
		break;
	case PM_STATE_SPIN_UP_DONE:
		pm->active_port++;
		if (pm->active_port >= pm->num_ports) {
			pm->active_port = 0;
#ifdef SUPPORT_QUICK_PM_INIT
			pm->retry_cnt = 3;	/* give 3 ms to check phy rdy */
#else
			pm->retry_cnt = 100;	/* give 100 ms to check phy rdy */
#endif
			pm->srst_retry_cnt = 1;
			pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
		}
		break;
	case PM_STATE_ENABLE_PM_PORT_1:
		pm->state = PM_STATE_ENABLE_PM_PORT_0;
		break;
	case PM_STATE_ENABLE_PM_PORT_0:
		pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
		break;
	case PM_STATE_PORT_CHECK_PHY_RDY:
		pm->sstatus = ((MV_U8)fis1) | (((MV_U8)fis2) << 8);
#ifndef SUPPORT_QUICK_PM_INIT
		pm->pm_port_sig[pm->active_port] = 0xFFFFFFFF;
#endif

		if ((pm->srst_retry_cnt > 0) && ((pm->sstatus & 0x00F) == 0x003)) {
			pm->state = PM_STATE_CLEAR_X_BIT;
		} else {
			if (pm->retry_cnt > 0) {
				pm->retry_cnt--;
				core_sleep_millisecond(root->core, 1);
				break;
			} else {
				/* done retrying, phy not ready, move to next port */
				if(pm->srst_retry_cnt > 0){
					pm->srst_retry_cnt--;
					pm->state = PM_STATE_ENABLE_PM_PORT_1;
					break;
				}
				pm->active_port++;
				if (pm->active_port >= pm->num_ports) {
					pm->active_port = 0;
					pm->state = PM_STATE_DONE;
				}
			}
		}
#ifdef SUPPORT_QUICK_PM_INIT
		pm->retry_cnt = 3;	/* give 3 ms to check phy rdy */
		pm->srst_retry_cnt = 1;
#endif
		break;
	case PM_STATE_CLEAR_X_BIT:
#ifdef SUPPORT_QUICK_PM_INIT
		pm->state = PM_STATE_WAIT_SIG;
#else
		pm->state = PM_STATE_ISSUE_SOFT_RESET_1;
#endif
		if (pm->devices[pm->active_port] == NULL) { 
			/* found a new device */
			device = get_device_obj(root, root->lib_rsrc);
			if (device == NULL) {
				CORE_DPRINT(("no more free device\n"));
				return;
			}
			sata_set_up_new_device(root, pm->base.port, device);
			get_new_and_update_base_id(root,&device->base);
			List_AddTail(&device->base.queue_pointer, &device->base.port->device_list);
			device->base.port->device_count++;
			pm->devices[pm->active_port] = device;
			device->pm = pm;
			device->pm_port = pm->active_port;
			device->connection = DC_SERIAL | DC_ATA;

			if ( (pm->sstatus & 0xF0) == 0x30 )
				device->negotiated_link_rate = PHY_LINKRATE_6;
			else if ( (pm->sstatus & 0xF0) == 0x20 )
				device->negotiated_link_rate = PHY_LINKRATE_3;
			else
				device->negotiated_link_rate = PHY_LINKRATE_1_5;

#ifdef SUPPORT_QUICK_PM_INIT
			//CORE_DPRINT(("active_port:%d signature:%x\n", pm->active_port, pm->pm_port_sig[pm->active_port]));
			if (pm->pm_port_sig[pm->active_port] == 0xFFFFFFFF) {
				device->base.parent = &pm->base;
				pm->sata_sig_timer = core_add_timer(root->core, 20,
					(void (*) (void *, void *))pm_sig_time_out, pm, NULL);
				return;
			}
#endif

			if (pm->pm_port_sig[pm->active_port] == 0xEB140101){
				device->connection |= DC_ATAPI;
				device->dev_type=DT_CD_DVD;
			} else{
				device->dev_type = DT_DIRECT_ACCESS_BLOCK;
				}
			device->base.parent = &pm->base;
		}
		break;
	case PM_STATE_ISSUE_SOFT_RESET_1:
		core_sleep_millisecond(root->core, 50);
		pm->state = PM_STATE_ISSUE_SOFT_RESET_0;
		break;
	case PM_STATE_ISSUE_SOFT_RESET_0:
		pm->state = PM_STATE_WAIT_SIG;
		if (pm->pm_port_sig[pm->active_port] == 0xEB140101){
			pm->devices[pm->active_port]->connection |= DC_ATAPI;
			pm->devices[pm->active_port]->dev_type=DT_CD_DVD;
			}
		break;
	default:
		CORE_DPRINT(("unknown state\n"));
		return;
	}

	core_queue_init_entry(root, &pm->base, MV_FALSE);
}

MV_VOID pm_hot_plug_req_callback(MV_PVOID ext, MV_Request *req)
{
	pl_root *root = (pl_root *)ext;
	domain_pm *pm = (domain_pm *)get_device_by_id(
		root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	saved_fis *fis;
	MV_U32 fis1, fis2, tmp;
	MV_U8 i;
	MV_BOOLEAN plug_in;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, &pm->base, req);
		return;
	}

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	fis1 = fis->dw4;
	fis2 = fis->dw2;

	switch (pm->state) {
	case PM_STATE_DISABLE_ASYNOTIFY:
		pm->state = PM_STATE_ENABLE_PM_PORT_1;
		break;
	case PM_STATE_ENABLE_PM_PORT_1:
		pm->state = PM_STATE_ENABLE_PM_PORT_0;
		break;
	case PM_STATE_ENABLE_PM_PORT_0:
		/* give 500 ms for phy to become ready */
		core_sleep_millisecond(root->core, 100);
		pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
		break;
	case PM_STATE_READ_GLOBAL_ERROR:
		pm->global_serror = ((MV_U8)fis1) | (fis2 << 8);
		for (i=0; i<pm->num_ports; i++) {
			if (pm->global_serror & MV_BIT(i)) {
				pm->active_port = i;
				pm->global_serror &= ~MV_BIT(i);
				break;
			}
		}
		if (i == pm->num_ports) {
			CORE_DPRINT(("pm error reg is empty. aborting hot plug\n"));
			pm->state = PM_STATE_DONE;
			core_init_entry_done(root, pm->base.port, &pm->base);
			return;
		}
		pm->state = PM_STATE_READ_PORT_ERROR;
		break;
	case PM_STATE_READ_PORT_ERROR:
		tmp = ((MV_U8)fis1) | (fis2 << 8);
		if (!((tmp & PM_SERROR_PHYRDY_CHANGE) || (tmp & PM_SERROR_EXCHANGED))) {
			CORE_DPRINT(("pm error reg not sdb. aborting hot plug\n"));
			pm->state = PM_STATE_DONE;
			core_init_entry_done(root, pm->base.port, &pm->base);
			return;
		}
		plug_in = (pm->devices[pm->active_port] == NULL);
		if (((tmp & PM_SERROR_COMM_WAKE) && !plug_in) ||
			(!(tmp & PM_SERROR_COMM_WAKE) && plug_in)) {
			CORE_DPRINT(("pm error reg comm_wake doesn't match.\n"));
		}
		
		pm->state = PM_STATE_CLEAR_PORT_ERROR;
		break;
	case PM_STATE_CLEAR_PORT_ERROR:
#ifdef SUPPORT_QUICK_PM_INIT
		pm->retry_cnt = 3;	/* give 3 ms to check phy rdy */
		pm->srst_retry_cnt = 1;
		pm->state = PM_STATE_ENABLE_PM_PORT_1;
#else
		pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
#endif
		break;

	case PM_STATE_PORT_CHECK_PHY_RDY:
		pm->sstatus = ((MV_U8)fis1) | (((MV_U8)fis2) << 8);
		if ((pm->sstatus & 0x00F) == 0x003) {
			/* We always need to clear x bit
			 * 1. If device exists and is device reset pm_init_req_callback will not make new dev
			 * 2. If device does not exit and is plug in pm_init_req_callback will make new dev
			 */
			pm->state = PM_STATE_CLEAR_X_BIT;
		} else {
			if (pm->devices[pm->active_port] == NULL) {
				CORE_DPRINT(("pm hot plug: phy is not rdy but no device \
					exists on this port\n"));
				if (pm->retry_cnt > 0) {
					pm->retry_cnt--;
					core_sleep_millisecond(root->core, 1);
					break;
				}	
			} else {
				sata_pm_disk_unplug(pm, pm->active_port);
				pm->pm_port_sig[pm->active_port] = 0xFFFFFFFF; /*clear signature for plug out dev.*/
			}
			pm->state = PM_STATE_SIG_DONE;
		}
		break;
	case PM_STATE_ENABLE_ASYNOTIFY:
		pm->state = PM_STATE_SIG_DONE;
		break;
	}

	core_queue_init_entry(root, &pm->base, MV_FALSE);
}

MV_VOID pm_hot_plug(domain_pm *pm)
{
	pm->state = PM_STATE_READ_GLOBAL_ERROR;
	pm->status |= PM_STATUS_HOT_PLUG;
	core_queue_init_entry(pm->base.root, &pm->base, MV_TRUE);
}

MV_VOID pm_device_phy_reset(domain_pm *pm, MV_U8 device_port)
{
	pm->active_port = device_port;
	pm->state = PM_STATE_DISABLE_ASYNOTIFY;
	pm->status |= PM_STATUS_DEVICE_PHY_RESET;
	//sata_pm_disk_unplug(pm, pm->active_port);
	core_queue_init_entry(pm->base.root, &pm->base, MV_TRUE);
}

MV_Request *pm_make_pm_register_req(domain_pm *pm, MV_BOOLEAN op,
	MV_BOOLEAN is_control, MV_U8 pm_port, MV_U16 reg_num, MV_U32 reg_value,
	MV_PVOID callback)
{
	pl_root *root = pm->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);
	core_context *ctx;

	if (req == NULL) return NULL;

	ctx = req->Context[MODULE_CORE];

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = (op == PM_OP_WRITE) ?
		CDB_CORE_PM_WRITE_REG : CDB_CORE_PM_READ_REG;
	req->Cdb[3] = is_control ? 0xF : pm_port;
	req->Cdb[4] = (MV_U8)(reg_num & 0xff);

	if (op == PM_OP_WRITE) {
		req->Cdb[5] = (MV_U8)((reg_value & 0xff00) >> 8);			// LBA_Low
		req->Cdb[6] = (MV_U8)((reg_value & 0xff0000) >> 16);		// LBA_Mid
		req->Cdb[7] = (MV_U8)((reg_value & 0xff000000) >> 24) ;		// LBA_High
		req->Cdb[8] = (MV_U8)(reg_value & 0xff);				// Sector_Count
	}
	
	req->Cdb[9] = (MV_U8)((reg_num & 0xf00) >> 8);
	req->Device_Id = pm->base.id;
	ctx->req_flag |= CORE_REQ_FLAG_NEED_D2H_FIS;
	req->Completion = (void(*)(MV_PVOID, MV_Request *))callback;
	return req;
}

MV_BOOLEAN pm_state_machine(MV_PVOID dev)
{
	domain_pm *pm = (domain_pm *)dev;
	pl_root *root = pm->base.root;
	MV_Request *req = NULL;
	MV_U32 tmp = 0;
	MV_U8 i;
	MV_PVOID callback;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	core_extension *core = (core_extension *)root->core;
	MV_ULONG flags;				
	struct device_spin_up *devsp=NULL;
#endif
	MV_U32 reg_num, reg_value;

	CORE_DPRINT(("pm %d state 0x%x active_port:%d.\n", pm->base.id, pm->state, pm->active_port));

	if (pm->status & (PM_STATUS_HOT_PLUG | PM_STATUS_DEVICE_PHY_RESET))
		callback = pm_hot_plug_req_callback;
	else
		callback = pm_init_req_callback;

	switch (pm->state) {
	case PM_STATE_RESET_DONE:
		pm->active_port = 0;
		for (i=0; i<CORE_MAX_DEVICE_PER_PM; i++) {
			pm->devices[i] = NULL;
			pm->pm_port_sig[i] = 0xFFFFFFFF;
		}
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_TRUE, 0, PM_GSCR_INFO,
			0, pm_init_req_callback);
		break;
	case PM_STATE_QRIGA_WORKAROUND:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0, 0x9B,
			0xF0, pm_init_req_callback);
		break;
	case PM_STATE_AKUPA_WORKAROUND:
		if (pm->pm_index == 0xff) {
			/*first autoload step of one pm.*/
			i = 0;
			do {
				tmp = *(MV_U32 *)&support_akupa_pm_devs[i];
			 	if ((support_akupa_pm_devs[i].vendor_id== pm->vendor_id) &&
				((support_akupa_pm_devs[i].device_id[0] == pm->device_id) ||
				(support_akupa_pm_devs[i].device_id[1] == pm->device_id)))
					break;
				i++;
			 } while (tmp != 0x00000000L);
			if (tmp != 0x00000000L)
				pm->pm_index = i;
		}
	         
		 if (pm->pm_index != 0xff){
		 	if ((support_akupa_pm_devs[pm->pm_index].autoload[0] == 0xA5A5A5A5)
				&& (support_akupa_pm_devs[pm->pm_index].autoload[pm->autoload_index] != 0xFFFFFFFFL)){
				reg_num = support_akupa_pm_devs[pm->pm_index].autoload[pm->autoload_index];
				reg_value = support_akupa_pm_devs[pm->pm_index].autoload[pm->autoload_index+1];
				
				req = pm_make_pm_register_req(pm,PM_OP_WRITE, MV_FALSE, (MV_U8)((reg_num >> 12) & 0xF), 
			          (MV_U16)(reg_num & 0xFFF), reg_value, pm_init_req_callback);
				break;
		     }
		}
		pm->state = PM_STATE_READ_ID_DONE; /*no find valid autoload, go to next step directly.*/
	case PM_STATE_READ_INFO_DONE:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_TRUE, 0, PM_GSCR_ID,
			0, pm_init_req_callback);
		break;
	case PM_STATE_READ_ID_DONE:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_TRUE, 0,
			PM_GSCR_REVISION, 0, pm_init_req_callback);
		break;
	case PM_STATE_READ_REV_DONE:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, 0, pm_init_req_callback);
		break;
	case PM_STATE_ENABLE_FEATURES:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, pm->feature_enabled,
			pm_init_req_callback);
		break;
	case PM_STATE_CLEAR_ERROR_INFO:
		tmp = MV_BIT(16) | MV_BIT(26);
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_ERROR_ENABLE, tmp, pm_init_req_callback);
		break;
	case PM_STATE_SPIN_UP_ALL_PORTS:
	case PM_STATE_ENABLE_PM_PORT_1:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm->active_port,
			PM_PSCR_SCONTROL, 0x01, callback);
		break;
	case PM_STATE_SPIN_UP_DONE:
	case PM_STATE_ENABLE_PM_PORT_0:
#ifdef SUPPORT_QUICK_PM_INIT
		//pm->pm_port_sig[pm->active_port] = 0xFFFFFFFF;
#endif
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm->active_port,
			PM_PSCR_SCONTROL, 0x00, callback);
		break;
	case PM_STATE_PORT_CHECK_PHY_RDY:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_FALSE, pm->active_port,
			PM_PSCR_SSTATUS, 0, callback);
		break;
	case PM_STATE_CLEAR_X_BIT:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm->active_port,
			PM_PSCR_SERROR, 0xFFFFFFFF, pm_init_req_callback);
		break;
	case PM_STATE_ISSUE_SOFT_RESET_1:
		req = sata_make_soft_reset_req(pm->base.port,
			pm->devices[pm->active_port], MV_TRUE, MV_FALSE,
			pm_init_req_callback);
		break;
	case PM_STATE_ISSUE_SOFT_RESET_0:
		req = sata_make_soft_reset_req(pm->base.port,
			pm->devices[pm->active_port], MV_FALSE, MV_FALSE,
			pm_init_req_callback);
		break;
	case PM_STATE_WAIT_SIG:
		if (pm->pm_port_sig[pm->active_port] == 0xFFFFFFFF) {
			MV_ASSERT(pm->sata_sig_timer == NO_CURRENT_TIMER);
			pm->sata_sig_timer = core_add_timer(root->core, 10,
					(void (*) (void *, void *))pm_sig_time_out, pm, NULL);
			MV_ASSERT(pm->sata_sig_timer != NO_CURRENT_TIMER);
			return MV_TRUE;
		} else {
			pm->devices[pm->active_port]->signature=pm->pm_port_sig[pm->active_port];
			pm->state = PM_STATE_SIG_DONE;
			/* no break, go directly to next case */
		}
	case PM_STATE_SIG_DONE:


		/* this device is done, go to the next device */
		if (pm->status & (PM_STATUS_HOT_PLUG | PM_STATUS_DEVICE_PHY_RESET)) {
			for (i=0; i<pm->num_ports; i++) {
				if (pm->global_serror & MV_BIT(i)) {
					pm->active_port = i;
					pm->global_serror &= ~MV_BIT(i);
					break;
				}
			}
			if (i == pm->num_ports) {
				/* done handling all hot plug */
				pm->active_port = 0;
				pm->state = PM_STATE_DONE;
				pm->status &= ~(PM_STATUS_HOT_PLUG | PM_STATUS_DEVICE_PHY_RESET);
			} else {
				pm->state = PM_STATE_READ_PORT_ERROR;
			}
		} else {
			pm->active_port++;
			if (pm->active_port >= pm->num_ports) {
				pm->active_port = 0;
				pm->state = PM_STATE_DONE;
			} else {
#ifdef SUPPORT_QUICK_PM_INIT
				//pm->state = PM_STATE_ENABLE_PM_PORT_1;
				pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
				//pm->base.port->phy->sata_signature = 0xFFFFFFFF;
				pm->retry_cnt = 3;	/* give 3 ms to check phy rdy */
				pm->srst_retry_cnt = 1;
#else
				pm->state = PM_STATE_PORT_CHECK_PHY_RDY;
#endif
			}
		}
		core_queue_init_entry(root, &pm->base, MV_FALSE);
		return MV_TRUE;

	case PM_STATE_DISABLE_ASYNOTIFY:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, pm->feature_enabled & (~MV_BIT(3)),
			pm_hot_plug_req_callback);
		break;
	
	case PM_STATE_READ_GLOBAL_ERROR:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_TRUE, 0,
			PM_GSCR_ERROR, 0, pm_hot_plug_req_callback);
		break;
	case PM_STATE_READ_PORT_ERROR:
		req = pm_make_pm_register_req(pm, PM_OP_READ, MV_FALSE, pm->active_port,
			PM_PSCR_SERROR, 0, pm_hot_plug_req_callback);
		break;
	case PM_STATE_CLEAR_PORT_ERROR:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_FALSE, pm->active_port,
			PM_PSCR_SERROR, 0xFFFFFFFF, pm_hot_plug_req_callback);
		break;

	case PM_STATE_ENABLE_ASYNOTIFY:
		req = pm_make_pm_register_req(pm, PM_OP_WRITE, MV_TRUE, 0,
			PM_GSCR_FEATURES_ENABLE, pm->feature_enabled | MV_BIT(3),
			pm_hot_plug_req_callback);
		break;

	case PM_STATE_DONE:
		for (i=0; i<pm->num_ports; i++) {
			if (pm->devices[i] && pm->devices[i]->state == DEVICE_STATE_IDLE) {
				pm->devices[i]->state = DEVICE_STATE_RESET_DONE;
				
#ifndef SUPPORT_STAGGERED_SPIN_UP
				core_queue_init_entry(root, &pm->devices[i]->base, MV_TRUE);
#else
				devsp=get_spin_up_device_buf(root->lib_rsrc);
				if(!devsp || (core->spin_up_group == 0)){
					if(devsp){
						free_spin_up_device_buf(root->lib_rsrc,devsp);
					}
					core_queue_init_entry(root, &pm->devices[i]->base, MV_TRUE);
				}else{
					MV_LIST_HEAD_INIT(&devsp->list);
					devsp->roots=root;
					devsp->base=&pm->devices[i]->base;
					
					OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
					List_AddTail(&devsp->list,&core->device_spin_up_list);
					if(core->device_spin_up_timer ==NO_CURRENT_TIMER){
						core->device_spin_up_timer=core_add_timer(core, 3, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, &pm->devices[i]->base, NULL);
					}
					OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
				}
						
#endif
			}
		}

		/*
		 * finish pm init entry after queuing device init entries -
		 * this is necessary to ensure event handling is pushed
		 * properly
		 */
		core_init_entry_done(root, pm->base.port, &pm->base);
		return MV_TRUE;
	default:
		return MV_TRUE;
	}

	if (req) {
		core_append_init_request(root, req);
		return MV_TRUE;
	} else {
		/* req is invalid, out of resource */
		return MV_FALSE;
	}
}

MV_U8 pm_verify_command(MV_PVOID root_p, MV_PVOID dev, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_pm *pm = (domain_pm *)dev;
	MV_U8 set;
core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC
	&& req->Cdb[1] == CDB_CORE_MODULE
	&& req->Cdb[2] == CDB_CORE_RESET_PORT) {
		mv_reset_phy(root, req->Cdb[3], MV_TRUE);
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (!sat_categorize_cdb(root, req)) {
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

#if 1
	if ( !(CORE_IS_INIT_REQ(ctx)) && !(CORE_IS_EH_REQ(ctx))) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif

	/* only get a register set if this pm doesn't already have one */
	if (pm->register_set == NO_REGISTER_SET) {
		set = core_get_register_set(root, NULL);
		if (set == NO_REGISTER_SET)
			return MV_QUEUE_COMMAND_RESULT_FULL;
		else
			pm->register_set = set;
	}

	return MV_QUEUE_COMMAND_RESULT_PASSED;
}

MV_VOID pm_prepare_command(MV_PVOID root, MV_PVOID dev, MV_PVOID cmd_header_p,
	MV_PVOID cmd_table_p, MV_Request *req)
{
	ata_taskfile taskfile;
	MV_BOOLEAN ret;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	domain_device *device = (domain_device *)dev;

	ret = ata_fill_taskfile(device, req, 0, &taskfile);
	if (!ret) {
		// HARRIET_TBD
		MV_DASSERT(MV_FALSE);
	}

	sata_prepare_command_header(req, cmd_header, cmd_table, 0, 0xF);
	sata_prepare_command_table(req, cmd_table, &taskfile, 0xF);
}

MV_VOID
pm_send_command(
    MV_PVOID root_p,
    MV_PVOID dev,
    MV_PVOID struct_wrapper,
    MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_U32 entry_nm;
	domain_pm *pm = (domain_pm *)dev;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
    	hw_buf_wrapper *wrapper = (hw_buf_wrapper *) struct_wrapper;

	entry_nm = prot_get_delv_q_entry(root, 0);
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_ASSERT(wrapper != NULL);
    	root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(
            TXQ_MODE_I |
                ctx->slot |
                pm->register_set <<
                TXQ_REGSET_SHIFT |
                TXQ_CMD_STP);
#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
        root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((pm->base.port->asic_phy_map << TXQ_PORT_SHIFT) | INTRFC_SRAM);
#else
        root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((pm->base.port->asic_phy_map << TXQ_PORT_SHIFT) | INTRFC_PCIEA);
#endif
        root->queues[0].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32(wrapper->phy.parts.low);
        root->queues[0].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32(wrapper->phy.parts.high);
#else
	root->queues[0].delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
		pm->base.port->asic_phy_map << TXQ_PHY_SHIFT |\
		 pm->register_set << TXQ_REGSET_SHIFT |TXQ_CMD_STP);
#endif
	prot_write_delv_q_entry(root, entry_nm, 0);
}

MV_VOID pm_process_command(MV_PVOID root_p, MV_PVOID dev, MV_PVOID cmpl_q_p,
	MV_PVOID cmd_table_p, MV_Request *req)
{
	domain_pm *pm = (domain_pm *)dev;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	MV_U32 error, cmpl_q = *(MV_PU32)cmpl_q_p;
	err_info_record *err_info;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_ULONG flags;
	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info == 0) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
	} else {
		error = MV_LE32_TO_CPU(err_info->err_info_field_1);
		CORE_EH_PRINT(("dev %d.\n", req->Device_Id));
		req->Scsi_Status = REQ_STATUS_ERROR;
		OSSW_SPIN_LOCK( &pm->base.base_SpinLock, flags);
		if (error & CMD_ISS_STPD) {
			ctx->error_info |= EH_INFO_CMD_ISS_STPD;
			pm->base.cmd_issue_stopped = MV_TRUE;
		}
		ctx->error_info |= EH_INFO_NEED_RETRY;
		OSSW_SPIN_UNLOCK( &pm->base.base_SpinLock, flags);
	}
}
	
