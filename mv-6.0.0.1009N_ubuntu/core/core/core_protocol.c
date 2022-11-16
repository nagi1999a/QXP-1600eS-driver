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
#include "core_resource.h"
#include "core_internal.h"
#include "core_util.h"
#include "core_manager.h"
#include "com_util.h"

#include "core_error.h"
#include "core_expander.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
extern MV_PVOID sata_store_received_fis(pl_root *root, MV_U8 register_set, MV_U32 flag);

#if defined(DEVICE_SLEEP_SUPPORT)
static void sata_device_wakeup_callback(pl_root *root, MV_Request *req)
{
	domain_device *dev = (domain_device *) get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, MV_FALSE);

	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		MV_DPRINT(("device %d waking up from sleep.\n", req->Device_Id));
		dev->status &= ~(DEVICE_STATUS_SLEEPING |
					DEVICE_STATUS_HANDLE_SLEEP);
	}
}
static char sata_device_sleeping(pl_root *root, domain_base *base, MV_Request *req) {
	domain_device *dev =  (domain_device *) base;
	domain_port *port = base->port;
	MV_Request *reset_req = NULL;

	if (dev->status & DEVICE_STATUS_HANDLE_SLEEP) {
		if ((req->Cdb[0] != SCSI_CMD_MARVELL_SPECIFIC &&
			req->Cdb[1] != CDB_CORE_MODULE && 
			req->Cdb[2] != CDB_CORE_SSP_VIRTUAL_PHY_RESET &&
			dev->base.parent->type != BASE_TYPE_DOMAIN_EXPANDER))
			return -1;
		else
			return 0;
	}


	if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		reset_req = ssp_make_virtual_phy_reset_req(dev,
			HARD_RESET, sata_device_wakeup_callback);
		if (reset_req) {
			MV_DPRINT(("device %d waking up from sleep.\n", req->Device_Id));
			dev->status |= DEVICE_STATUS_HANDLE_SLEEP;
			core_queue_eh_req(root, reset_req);
		}
		return -1;
	} else {
		if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
			extern MV_VOID pm_device_phy_reset(domain_pm *pm, MV_U8 device_port);
			pm_device_phy_reset(dev->pm, dev->pm_port);
		} else {
			mv_reset_phy(root, port->phy_map, MV_TRUE);
		}				
		MV_DPRINT(("device %d waking up from sleep.\n", req->Device_Id));
		dev->status &= ~(DEVICE_STATUS_SLEEPING |
				DEVICE_STATUS_HANDLE_SLEEP);
	}
	return 0;
}
#endif

MV_BOOLEAN prot_init_pl(pl_root *root, MV_U16 max_io,
	MV_PVOID core, MV_LPVOID mmio,
	lib_device_mgr *lib_dev, lib_resource_mgr *rsrc, MV_U16 root_idx)
{
	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U32 item_size;
	MV_PU8 vir;
	MV_U16 i;
	domain_port *port;
	domain_phy *phy;
	core_extension* core_ext = (core_extension*)core;
	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);

	root->mmio_base = mmio;

	root->lib_dev = lib_dev;
	root->lib_rsrc = rsrc;
	root->core = (MV_PVOID)core;

	root->phy_num = core_ext->chip_info->n_phy;
	root->slot_count_support = slot_count;

	for (i = 0; i < root->queue_num; i++) {
		MV_COUNTED_LIST_HEAD_INIT(&root->queues[i].waiting_queue);
		MV_COUNTED_LIST_HEAD_INIT(&root->queues[i].high_priority_queue);
		MV_LIST_HEAD_INIT(&root->queues[i].complete_queue);
		
		root->queues[i].root = root;
		root->queues[i].msix_idx = root_idx * root->queue_num + i;
		root->queues[i].id = i;
		root->queues[i].delv_q_size = delv_q_size;
		root->queues[i].cmpl_q_size = cmpl_q_size;
		root->queues[i].last_delv_q = 0x3fff;
		root->queues[i].last_cmpl_q = 0x3fff;
		root->queues[i].attention = 0;
#ifdef ATHENA_PERFORMANCE_TUNNING
		OSSW_INIT_SPIN_LOCK(&root->queues[i].waiting_queue_SpinLock);
		OSSW_INIT_SPIN_LOCK(&root->queues[i].high_priority_SpinLock);
		OSSW_INIT_SPIN_LOCK(&root->queues[i].complete_SpinLock);
		OSSW_INIT_SPIN_LOCK(&root->queues[i].queue_SpinLock);
		OSSW_INIT_SPIN_LOCK(&root->queues[i].handle_cmpl_SpinLock);
		OSSW_INIT_SPIN_LOCK(&root->queues[i].queue_attention_SpinLock);
#endif
	}

	/* allocate memory for running_req */
	item_size = sizeof(PMV_Request) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->running_req = (PMV_Request *)vir;
	root->running_num = 0;

	/* allocate memory for saved fis */
	item_size = sizeof(saved_fis) * received_fis_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->saved_fis_area = (saved_fis *)vir;

	/* allocate memory for command table wrapper */
	/* this is so far used as a array, not a pool */
	item_size = sizeof(hw_buf_wrapper) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	root->cmd_struct_wrapper = (hw_buf_wrapper *) vir;
#else
	root->cmd_table_wrapper = (hw_buf_wrapper *) vir;
#endif

	/* slot pool */
	item_size = sizeof(MV_U16) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->slot_pool.Stack = (MV_PU16)vir;
	root->slot_pool.Size = slot_count;
	Tag_Init_FIFO(&root->slot_pool, root->slot_pool.Size);

	for (i = 0; i < core_ext->chip_info->n_phy; i++) {
		phy = &(root->phy[i]);
		phy->id = (MV_U8)i;
		phy->asic_id = (MV_U8)i; /* will be updated later */
		phy->root = root;
#ifdef SUPPORT_DIRECT_SATA_SSU
		phy->sata_ssu_timer = NO_CURRENT_TIMER;
#endif
#ifdef SUPPORT_PHY_POWER_MODE
		phy->PHY_power_mode_HIPM_timer = NO_CURRENT_TIMER;
		phy->In_PHY_power_mode_HIPM = 0;
#endif
		phy->phy_decode_timer = NO_CURRENT_TIMER;
		phy->phy_decode_err_cnt = 0;
	}

	for (i = 0; i < core_ext->chip_info->n_phy; i++) {
		extern void init_port(pl_root *root, domain_port *port);
		port = &(root->ports[i]);
#ifdef ATHENA_PERFORMANCE_TUNNING
		OSSW_INIT_SPIN_LOCK(&port->base.err_ctx.sent_req_list_SpinLock);
		OSSW_INIT_SPIN_LOCK(&port->base.base_SpinLock);
		OSSW_INIT_SPIN_LOCK(&port->base.outstanding_SpinLock);
		OSSW_INIT_SPIN_LOCK(&port->base.cmd_issue_stopped_SpinLock);
#endif
		port->base.id = (MV_U8)i;
		port->type = PORT_TYPE_SAS; /*default*/

		port->phy_num = 0;
		port->phy_map = 0;
		port->asic_phy_map = 0;
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
		port->curr_phy_map = 0;
#endif
		port->phy = NULL;

		init_port(root, port);
	}

	return MV_TRUE;
}

MV_QUEUE_COMMAND_RESULT prot_send_request(pl_root *root, struct _domain_base *base,
	MV_Request *req)
{
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	mv_command_header *cmd_header = NULL;
	mv_command_table *cmd_table = NULL;
	mv_command_struct *cmd_struct = NULL;
#else
	mv_command_header *cmd_list = (mv_command_header *)root->cmd_list;
	mv_command_header *cmd_header = NULL;
#endif
	core_context *ctx = req->Context[MODULE_CORE];
	core_extension * core = (core_extension *)root->core;
	command_handler *handler = (command_handler *)ctx->handler;
	hw_buf_wrapper *table_wrapper = NULL, *sg_wrapper = NULL, hbw;
	MV_PHYSICAL_ADDR tmpPhy;
	MV_U16 slot = 0, slot2=0, prd_num = 0;
	MV_QUEUE_COMMAND_RESULT result;
	domain_phy *phy = NULL;
	MV_ULONG flags, flags1;
	OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
	if (base->outstanding_req >= base->queue_depth) {
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return MV_QUEUE_COMMAND_RESULT_FULL;
	}
#ifdef DOUBLEBUF_4_SINGLE_PRD
    if (base->outstanding_req) {
	OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
        return MV_QUEUE_COMMAND_RESULT_FULL;
    }
#endif


	/* for console device, base->port == NULL */
	if (base->port != NULL) {
		/* If error count is not zero, only do CORE_IS_EH_REQ */
		  //OSSW_SPIN_LOCK_Device(root->core, flags, base);
		  OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
               if ( (base->blocked == MV_TRUE) || port_has_error_req(root, base->port) ||
			port_has_init_req(root, base->port)) {
			/* cannot use outstanding_req > 0
			 * some error handling req is instant req
			 * so outstanding_req==0
			 * and we want to block whole port */
			if (!CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)
#ifdef SUPPORT_SG_RESET
				&& !CORE_IS_HBA_RESET_REQ(req)
#endif
			) {
				//OSSW_SPIN_UNLOCK_Device(root->core, flags, base);
				OSSW_SPIN_UNLOCK(&base->base_SpinLock, flags);
				OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
				return MV_QUEUE_COMMAND_RESULT_FULL;
			}
		}
		//OSSW_SPIN_UNLOCK_Device(root->core, flags, base);
		OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
	}
#if 0
	if (base->cmd_issue_stopped) {
		/* SATA/STP will stop issue when hit first error
		 * abort the running request first. */
		//pal_abort_device_running_req(root, base);
		//hal_enable_register_set(root, base);
		base->cmd_issue_stopped = MV_FALSE;
		return MV_QUEUE_COMMAND_RESULT_FULL;
	}
#endif
#ifndef RAID_DRIVER
	if ((BASE_TYPE_DOMAIN_DEVICE == base->type)
		&& (((domain_device *)base)->status & DEVICE_STATUS_FROZEN) && (!CORE_IS_EH_REQ(ctx))) {
		CORE_EH_PRINT(("device %d is frozen, status:%x\n", base->id, ((domain_device *)base)->status));
		req->Scsi_Status = REQ_STATUS_FROZEN;
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif

#if defined(DEVICE_SLEEP_SUPPORT)
	if ((base->type == BASE_TYPE_DOMAIN_DEVICE)
        &&(((domain_device *)base)->status & DEVICE_STATUS_SLEEPING)) {
		if (sata_device_sleeping(root, base, req)){
			OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
			return MV_QUEUE_COMMAND_RESULT_BLOCKED;
			}
	}
#endif

	/* verify this commands. Maybe can be finished right away without resource */
	result = (MV_QUEUE_COMMAND_RESULT) handler->verify_command(root, base, req);
	if (result != MV_QUEUE_COMMAND_RESULT_PASSED) {
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return result;
	}
	base->outstanding_req++;
	OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
	if (SCSI_IS_PACKET_RW(req->Cdb[0])) {
		slot = (((MV_U16)(req->Cdb[2]) << 8) | ((MV_U16)req->Cdb[3]));
		//CORE_DPRINT(("pkt_r/W slot:%x, req:%x %x %x\n", slot, req->Cdb[0], req->Cdb[2], req->Cdb[3]));
	} else {
		//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
		if (Tag_IsEmpty(&root->slot_pool)) {
			//CORE_DPRINT(("run out of slot.\n"));
			//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
			OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
			base->outstanding_req--;
			OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		} else {
			slot = Tag_GetOne(&root->slot_pool);
			//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
    	}
    } //if (SCSI_IS_PACKET_RW(req->Cdb[0])) {
	ctx->slot = slot;

	if (SCSI_IS_PACKET(req->Cdb[0])) { 
		//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
		if (Tag_IsEmpty(&root->slot_pool)) {
			Tag_ReleaseOne(&root->slot_pool, slot);
			//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
			OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
			base->outstanding_req--;
			OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		} else {
			slot2 = Tag_GetOne(&root->slot_pool);
			//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
		}
		req->Cdb[2] = (MV_U8)((slot2 >> 8)&0xff);
		req->Cdb[3] = (MV_U8)(slot2 & 0xff);
		//CORE_DPRINT(("get slot2 for pkt_rw:%x, req:%x, cdb M:%x, L:%x\n", slot2, req->Cdb[0],req->Cdb[2], req->Cdb[3]));
	}


#if defined(HAVE_HW_COMPLIANT_SG)
if (!(req->SG_Table.Flag & SGT_FLAG_PRDT_IN_HOST) && 
    (!(req->SG_Table.Flag & SGT_FLAG_PRDT_HW_COMPLIANT))) 
#endif
	{
	/* set up hardware SG table */
	sg_wrapper = get_sg_buf(root->lib_rsrc);
	if (sg_wrapper == NULL) {
		//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
		Tag_ReleaseOne(&root->slot_pool, slot);
		//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
		OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
		base->outstanding_req--;
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}

#ifdef PCI_POOL_SUPPORT
#ifndef HARDWARE_XOR
		req->sg_table = ossw_pci_pool_alloc (core->sg_pool, &req->sg_table_phy.value);

		if (req->sg_table == NULL) {
			//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
			Tag_ReleaseOne(&root->slot_pool, slot);
			//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
			OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
			OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
			base->outstanding_req--;
			OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		} else {
			//MV_ZeroMemory(req->sg_table, CORE_HW_SG_ENTRY_SIZE*CORE_MAX_HW_SG_ENTRY_COUNT);
		}
		sg_wrapper->vir = req->sg_table;
		sg_wrapper->phy =  req->sg_table_phy;
#endif
#endif

#ifdef CORE_ZERO_MEMORY_TEST
	//MV_ZeroMemory(sg_wrapper->vir, CORE_HW_SG_ENTRY_SIZE*CORE_MAX_HW_SG_ENTRY_COUNT);
#endif
	MV_DASSERT(ctx->sg_wrapper == NULL);
	ctx->sg_wrapper = sg_wrapper;

	}

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
       /* set up command header */
     	table_wrapper = &root->cmd_struct_wrapper[slot];

#ifdef PCI_POOL_SUPPORT
	req->cmd_table = ossw_pci_pool_alloc(core->ct_pool, &req->cmd_table_phy.value);
	if (req->cmd_table == NULL) {
		//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
		Tag_ReleaseOne(&root->slot_pool, slot);
		//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
		OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
		base->outstanding_req--;
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	} else {
		//MV_ZeroMemory(req->cmd_table, sizeof(mv_command_struct));
	}
	table_wrapper->vir = req->cmd_table;
	table_wrapper->phy =  req->cmd_table_phy;
#endif

    cmd_struct= (mv_command_struct *)table_wrapper->vir;
    cmd_header = &cmd_struct->header.mv_cmd_header;
    cmd_table= &cmd_struct->mv_cmd_table;
#else
	/* set up command header */
	cmd_header = &cmd_list[slot];
	table_wrapper = &root->cmd_table_wrapper[slot];
	
#ifdef PCI_POOL_SUPPORT
	//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
	req->cmd_table = ossw_pci_pool_alloc(core->ct_pool, &req->cmd_table_phy.value);
	if (req->cmd_table == NULL) {
		//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
		Tag_ReleaseOne(&root->slot_pool, slot);
		//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
		OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
		OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
		base->outstanding_req--;
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	} else {
		//MV_ZeroMemory(req->cmd_table, sizeof(mv_command_table));
	}
	table_wrapper->vir = req->cmd_table;
	table_wrapper->phy =  req->cmd_table_phy;
	//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
#endif

#endif

#ifdef CORE_ZERO_MEMORY_TEST
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	//MV_ZeroMemory(table_wrapper->vir, sizeof(mv_command_table)+sizeof(mv_command_header));
#else
	MV_ZeroMemory(cmd_header, sizeof(mv_command_header));
	MV_ZeroMemory(table_wrapper->vir, sizeof(mv_command_table));
#endif //SUPPORT_ATHENA
#endif //CORE_ZERO_MEMORY_TEST

	//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
	root->running_num++;	// add spin lock
	root->running_req[slot] = req;
	OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
	//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
#if defined(HAVE_HW_COMPLIANT_SG)
	if ((req->SG_Table.Flag & SGT_FLAG_PRDT_IN_HOST) || 
		(req->SG_Table.Flag & SGT_FLAG_PRDT_HW_COMPLIANT)){
	
		sg_wrapper = &hbw;

		sg_wrapper->phy.parts.low = req->SG_Table.prdt_bus_addr.parts.low;
		sg_wrapper->phy.parts.high = req->SG_Table.prdt_bus_addr.parts.high;

		sg_wrapper->vir = req->SG_Table.Entry_Ptr;;
		ctx->sg_wrapper = NULL;
		//TODO,dont check req type
		//SGD_REFTBL, SGD_REFSGD
	}
#endif
#if defined(SUPPORT_SP2)
	prot_set_up_sg_table(root, req, sg_wrapper, cmd_struct);

//	cmd_header->sgl_prp0_addr = MV_CPU_TO_LE64(sg_wrapper->phy);

#else
	prd_num = prot_set_up_sg_table(root, req, sg_wrapper);
#ifdef DOUBLEBUF_4_SINGLE_PRD
    if (SCSI_IS_WRITE(req->Cdb[0])) {
        if (req->Data_Buffer) {
            MV_CopyMemory(core->doublebuf_vir, req->Data_Buffer, req->Data_Transfer_Length);
        }
        prd_num = 1;
        ((prd_t *)sg_wrapper->vir)->baseAddr_low = core->doublebuf_dma.parts.low;
        ((prd_t *)sg_wrapper->vir)->baseAddr_high = core->doublebuf_dma.parts.high;
        ((prd_t *)sg_wrapper->vir)->size = req->Data_Transfer_Length;
        ((prd_t *)sg_wrapper->vir)->size |= INTRFC_SRAM << PRD_IF_SELECT_SHIFT;
    }
#endif
#ifdef TEST_SRAM_LK
    if ((req->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC) &&( req->Cdb[2] ==CDB_CORE_IDENTIFY)) 
    {
        ((prd_t *)sg_wrapper->vir)->size = req->Data_Transfer_Length;
        ((prd_t *)sg_wrapper->vir)->size |= INTRFC_SRAM << PRD_IF_SELECT_SHIFT;
    }
#endif
#ifdef TEST_DATA_IN_SRAM
    if (SCSI_IS_READ(req->Cdb[0]) || SCSI_IS_WRITE(req->Cdb[0])) {
        if (req->Cmd_Flag & CMD_FLAG_DATA_OUT) {
            if (req->Data_Buffer) {
                MV_CopyMemory(core->doublebuf_vir, req->Data_Buffer, req->Data_Transfer_Length);
            }
        }
            
        if (req->Data_Transfer_Length <= 64 * 1024) {
            #ifdef SINGLE_PRD
            prd_num = 1;
            ((prd_t *)((MV_PU8)sg_wrapper->vir ))->baseAddr_low = core->doublebuf_dma;
            ((prd_t *)((MV_PU8)sg_wrapper->vir ))->baseAddr_high = 0;
            ((prd_t *)((MV_PU8)sg_wrapper->vir ))->size = req->Data_Transfer_Length;
            ((prd_t *)((MV_PU8)sg_wrapper->vir))->size |= INTRFC_SRAM << PRD_IF_SELECT_SHIFT;
            #else
            for (j = 0; j < prd_num; j++) {
                ((prd_t *)((MV_PU8)sg_wrapper->vir + j * 12))->baseAddr_low = core->doublebuf_dma;
                ((prd_t *)((MV_PU8)sg_wrapper->vir + j * 12))->baseAddr_high = 0;
                ((prd_t *)((MV_PU8)sg_wrapper->vir + j * 12))->size &= 0x00FFFFFF;
                ((prd_t *)((MV_PU8)sg_wrapper->vir + j * 12))->size |= INTRFC_SRAM << PRD_IF_SELECT_SHIFT;
            }
            #endif
        }
    }
#endif

	cmd_header->ctrl_nprd = MV_CPU_TO_LE32(prd_num << CH_PRD_TABLE_LEN_SHIFT);
	cmd_header->prd_table_addr = MV_CPU_TO_LE64(sg_wrapper->phy);
#endif
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#if defined(PCI_POOL_SUPPORT)
    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, status_buff);
    cmd_header->status_buff_addr = MV_CPU_TO_LE64(tmpPhy);
 //   tmpPhy.value = table_wrapper->phy.value +OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, table);
 //   cmd_header->table_addr = MV_CPU_TO_LE64(tmpPhy);
    cmd_header->reserved[0] = 0;
    cmd_header->reserved[1] = 0;
#if 0
    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_sata_stp_command_table, pir);
    cmd_header->security_key_rec_base_addr= MV_CPU_TO_LE64(tmpPhy);
#endif

    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, open_address_frame);
    cmd_header->open_addr_frame_addr  = MV_CPU_TO_LE64(tmpPhy);

#ifdef SUPPORT_SECURITY_KEY_RECORDS
    tmpPhy.value = root->security_key_dma.value + ((MV_PU8)&root->security_key[slot] - (MV_PU8)root->security_key);
    cmd_header->security_key_rec_base_addr= MV_CPU_TO_LE64(tmpPhy);
#endif
#endif
#else
    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, status_buff);
    cmd_header->status_buff_addr = MV_CPU_TO_LE64(tmpPhy);
    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, table);
    cmd_header->table_addr = MV_CPU_TO_LE64(tmpPhy);
    tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, open_address_frame);
    cmd_header->open_addr_frame_addr  = MV_CPU_TO_LE64(tmpPhy);
#endif

	/* cmd_header->tag is different between SSP and STP/SATA
	 * it'll be further handled in prepare_command handler.
	 * refer to ASIC spec. */
	cmd_header->tag = MV_CPU_TO_LE16(slot);
	cmd_header->data_xfer_len = MV_CPU_TO_LE32(req->Data_Transfer_Length);
#if defined(PCI_POOL_SUPPORT)
	core_set_cmd_header_selector(cmd_header);
#endif
	

	/* splits off to handle protocol-specific tasks */

	handler->prepare_command(root, base, cmd_header, cmd_table, req);
	// add spin lock
	if(base->queue)
		//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, base->queue->msix_idx);
		OSSW_SPIN_LOCK(&root->queues[(base->queue->msix_idx)%root->queue_num].queue_SpinLock, flags);
	else
		//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, (root->root_id * root->queue_num));
		OSSW_SPIN_LOCK(&root->queues[0].queue_SpinLock, flags);

	mv_add_timer(core, req);	//0512
	handler->send_command(root, base, table_wrapper, req);
	if(base->queue)
		//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, base->queue->msix_idx);
		OSSW_SPIN_UNLOCK(&root->queues[(base->queue->msix_idx)%root->queue_num].queue_SpinLock, flags);
	else
		//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, (root->root_id * root->queue_num));
		OSSW_SPIN_UNLOCK(&root->queues[0].queue_SpinLock, flags);
	return MV_QUEUE_COMMAND_RESULT_SENT;
}

MV_U32 prot_get_delv_q_entry(pl_root *root, MV_U16 queue_id)
{
	MV_U32 tmp;

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	root->queues[queue_id].last_delv_q++;
	if (root->queues[queue_id].last_delv_q >= root->queues[queue_id].delv_q_size)
		root->queues[queue_id].last_delv_q = 0;
	tmp = (MV_U32)(root->queues[queue_id].last_delv_q);
#else
	root->last_delv_q++;
	if (root->last_delv_q >= root->delv_q_size)
		root->last_delv_q = 0;
	tmp = (MV_U32)(root->last_delv_q);
#endif
	return tmp;
}

MV_VOID prot_write_delv_q_entry(pl_root *root, MV_U32 entry, MV_U16 queue_id)
{
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q_WR_PTR, entry);
#else
#ifdef ATHENA_MISS_MSIX_INT_WA
    pl_queue *queue = &root->queues[queue_id];
    queue->msi_x_miss_count = 0;
#endif
    WRITE_DLVRYQ_WR_PTR(root, queue_id, entry);
#endif
}

PMV_Request get_intl_req_resource(pl_root *root, MV_U32 buf_size)
{
	PMV_Request req;
	core_context *ctx;
	hw_buf_wrapper *wrapper = NULL;

	PMV_SG_Table sg_tbl;

	req = get_intl_req(root->lib_rsrc);
	if (req == NULL) return NULL;

	MV_DASSERT(req->Context[MODULE_CORE] == NULL);
	ctx = get_core_context(root->lib_rsrc);
	if (ctx == NULL) {
		free_intl_req(root->lib_rsrc, req);
		return NULL;
	}

	MV_DASSERT(buf_size <= SCRATCH_BUFFER_SIZE);

	if (buf_size > SCRATCH_BUFFER_SIZE)
		printk(KERN_ALERT "[%s] requested buf_size 0x%u is larger than 0x%lu\n", __func__, buf_size, SCRATCH_BUFFER_SIZE);

	if (buf_size != 0) {
		wrapper = get_scratch_buf(root->lib_rsrc);
		if (wrapper == NULL) {
			free_intl_req(root->lib_rsrc, req);
			free_core_context(root->lib_rsrc, ctx);
			return NULL;
		}
		memset(wrapper->vir, 0x0, buf_size);
		ctx->buf_wrapper = wrapper;
		req->Data_Buffer = wrapper->vir;
	} else {
		ctx->buf_wrapper = NULL;
		req->Data_Buffer = NULL;
	}
	req->Tag = 0xaa;
	req->Context[MODULE_CORE] = ctx;
	req->Cmd_Initiator = root;
	req->Data_Transfer_Length = buf_size;
	req->Scsi_Status = REQ_STATUS_PENDING;
	req->Req_Flag = 0; /* no special flag needed for internal request */
	req->Cmd_Flag = 0;
	req->Time_Out = CORE_REQ_INIT_TIMEOUT;

	/* Make the SG table. */
	sg_tbl = &req->SG_Table;
	SGTable_Init(sg_tbl, 0);
	if (buf_size != 0) {
		SGTable_Append(
			sg_tbl,
			wrapper->phy.parts.low,
			wrapper->phy.parts.high,
			buf_size);
	}
#if defined(HAVE_HW_COMPLIANT_SG)
	req->SG_Table.Flag |= SGT_FLAG_PRDT_HW_COMPLIANT;
	req->SG_Table.prdt_bus_addr.parts.low = req->bus_addr.parts.low;//(MV_U32)req->SG_Table.Entry_Ptr; //FIXME
	req->SG_Table.prdt_bus_addr.parts.high = req->bus_addr.parts.high;
#endif

	/* clean data structure if neccessary */
	MV_ZeroMemory(req->Cdb, sizeof(req->Cdb));
	return req;
}

void intl_req_release_resource(lib_resource_mgr *rsrc, PMV_Request req)
{
	core_context *ctx;
	hw_buf_wrapper *wrapper;

	ctx = req->Context[MODULE_CORE];
	MV_ASSERT(ctx != NULL);

	wrapper = ctx->buf_wrapper;
	if (wrapper) {
		free_scratch_buf(rsrc, wrapper);
		ctx->buf_wrapper = NULL;
	}

	if (ctx) {
		free_core_context(rsrc, ctx);
		req->Context[MODULE_CORE] = NULL;
	}

	free_intl_req(rsrc, req);
}
MV_VOID prot_release_sata_register_set(core_extension *core, pl_root *root, domain_device *dev, MV_Request *req){
	MV_ULONG flags;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		sata_free_ncq_tag(dev, req);
	}
	if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
		//OSSW_SPIN_LOCK_Device(core, flags, &dev->base);
		OSSW_SPIN_LOCK(&dev->base.base_SpinLock, flags);
		if (dev->register_set != NO_REGISTER_SET) {
			/* If PIO protocol command failed, need parse D2H FIS
			    for error&status register.*/
			if ((req->Scsi_Status != REQ_STATUS_SUCCESS)
                        		&& (req->Cmd_Flag & CMD_FLAG_PIO))
    				ctx->received_fis =
        				sata_store_received_fis(root,
			        			dev->register_set,
				        		(req->Cmd_Flag & (~CMD_FLAG_PIO)));
                        	else
        			ctx->received_fis =
	        			sata_store_received_fis(root,
		        			dev->register_set,
			        		req->Cmd_Flag);
		} else {
                                ctx->received_fis = NULL;
		}
		//OSSW_SPIN_UNLOCK_Device(core, flags, &dev->base);
		OSSW_SPIN_UNLOCK(&dev->base.base_SpinLock, flags);
	}
	//OSSW_SPIN_LOCK_Device(core, flags, &dev->base);
	OSSW_SPIN_LOCK(&dev->base.base_SpinLock, flags);
	if (dev->register_set != NO_REGISTER_SET) {
		if (dev->base.outstanding_req == 0) {
			core_free_register_set(root, dev, dev->register_set);
			dev->register_set = NO_REGISTER_SET;
		}
	}
	//OSSW_SPIN_UNLOCK_Device(core, flags, &dev->base);
	OSSW_SPIN_UNLOCK(&dev->base.base_SpinLock, flags);
}
MV_VOID prot_release_pm_register_set(core_extension *core, pl_root *root, domain_pm *pm, MV_Request *req){
	MV_ULONG flags;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
		//OSSW_SPIN_LOCK_Device(core, flags, &pm->base);
		OSSW_SPIN_LOCK(&pm->base.base_SpinLock, flags);
		if (pm->register_set != NO_REGISTER_SET) {
			ctx->received_fis =
			sata_store_received_fis(root,
			pm->register_set,
			req->Cmd_Flag);
		} else {
		    ctx->received_fis = NULL;
		}
		//OSSW_SPIN_UNLOCK_Device(core, flags, &pm->base);
		OSSW_SPIN_UNLOCK(&pm->base.base_SpinLock, flags);
	}
	//OSSW_SPIN_LOCK_Device(core, flags, &pm->base);
	OSSW_SPIN_LOCK(&pm->base.base_SpinLock, flags);
	if (pm->register_set != NO_REGISTER_SET) {
		if (pm->base.outstanding_req == 0) {
			core_free_register_set(root,NULL, pm->register_set);
			pm->register_set = NO_REGISTER_SET;
		}
	}
	//OSSW_SPIN_UNLOCK_Device(core, flags, &pm->base);
	OSSW_SPIN_UNLOCK( &pm->base.base_SpinLock, flags);
}
MV_VOID prot_release_register_set(core_extension *core, pl_root *root, domain_base *base){
	MV_ULONG flags;
	//OSSW_SPIN_LOCK_Device(core, flags, base);
	OSSW_SPIN_LOCK(&base->base_SpinLock, flags);

	switch(base->type){
		case BASE_TYPE_DOMAIN_DEVICE:
		{
			domain_device *dev=(domain_device *)base;
			if (dev->register_set != NO_REGISTER_SET && !base->outstanding_req){
				core_free_register_set(root, dev, dev->register_set);
				dev->register_set = NO_REGISTER_SET;
			}
		}
		break;
		case BASE_TYPE_DOMAIN_EXPANDER:
		{
			domain_expander *exp=(domain_expander *)base;
			if (exp->register_set != NO_REGISTER_SET && !base->outstanding_req) {
				core_free_register_set(root, base, exp->register_set);
				exp->register_set = NO_REGISTER_SET;
			}
		}
		break;
		case BASE_TYPE_DOMAIN_ENCLOSURE:
		{
			domain_enclosure *enc=(domain_enclosure *)base;
			if (enc->register_set != NO_REGISTER_SET && !base->outstanding_req) {			
				core_free_register_set(root, base, enc->register_set);
				enc->register_set = NO_REGISTER_SET;
			}
		}
		break;

	}
	//OSSW_SPIN_UNLOCK_Device(core, flags, base);
	OSSW_SPIN_UNLOCK(&base->base_SpinLock, flags);
}
MV_VOID prot_clean_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req)
{
	hw_buf_wrapper *sg_wrapper;
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_ULONG flags, flags1;

	/* clean up used resources */
//	OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
#ifdef STRONG_DEBUG
        MV_ASSERT(base->outstanding_req > 0);
#endif
	if(root->running_req[slot] == NULL){
		CORE_EH_PRINT(("root %x dev 0x%x slot 0x%x root->running_req is NULL, io 0x%x root num 0x%x\n", root->root_id, base->id, slot, base->outstanding_req, root->running_num));
	}
	root->running_req[slot] = NULL;
	root->running_num--;
	Tag_ReleaseOne(&root->slot_pool, slot);
	OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
//	OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);

#ifdef PCI_POOL_SUPPORT
	ossw_pci_pool_free(core->ct_pool, req->cmd_table, req->cmd_table_phy.value);
#ifndef HARDWARE_XOR
	if (req->sg_table != NULL) {
		ossw_pci_pool_free(core->sg_pool, req->sg_table, req->sg_table_phy.value);
		req->sg_table = NULL;
	}
#endif
#endif
	mv_renew_timer(root->core, req);

	/* not every req will have a sg_wrapper (eg, instant requests) */
	sg_wrapper = ctx->sg_wrapper;
	if (sg_wrapper != NULL) {
		free_sg_buf(root->lib_rsrc, sg_wrapper);
		ctx->sg_wrapper = NULL;
	}
	OSSW_SPIN_LOCK(&base->outstanding_SpinLock, flags1);
	if (base->outstanding_req) {
		base->outstanding_req--;
	}
	else {
		CORE_DPRINT(("device %d outstanding req is zero??.\n",base->id));
	}
	if(core->has_port_queue){
		switch(base->type){
			case BASE_TYPE_DOMAIN_DEVICE:
				{
					domain_device *dev = (domain_device *)base;
					if(IS_STP_OR_SATA(dev)){
						prot_release_sata_register_set(core, root, dev, req);
					}else{
						prot_release_register_set(core, root, base);
					}
					break;
				}
			case BASE_TYPE_DOMAIN_PM:
				prot_release_pm_register_set(core, root, (domain_pm *)base, req);
				break;
			case BASE_TYPE_DOMAIN_EXPANDER:
			case BASE_TYPE_DOMAIN_ENCLOSURE:
				prot_release_register_set(core, root, base);
				break;
		}
	}else{
		if(base->type == BASE_TYPE_DOMAIN_DEVICE) {
			domain_device *dev = (domain_device *)base;
			if (IS_STP_OR_SATA(dev)){
				prot_release_sata_register_set(core, root, dev, req);
			}
		}
		if(base->type == BASE_TYPE_DOMAIN_PM ){
			prot_release_pm_register_set(core, root, (domain_pm *)base, req);
		}
	}
	OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock, flags1);
#if 0
	/* release NCQ tag & register set */
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *dev = (domain_device *)base;
		if (IS_STP_OR_SATA(dev)) {
			if (req->Cmd_Flag & CMD_FLAG_NCQ) {
				sata_free_ncq_tag(dev, req);
			}
			OSSW_SPIN_LOCK_Device(base->root->core, flags, base);
			if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
				if (dev->register_set != NO_REGISTER_SET) {
					/* If PIO protocol command failed, need parse D2H FIS
					    for error&status register.*/
					if ((req->Scsi_Status != REQ_STATUS_SUCCESS)
                                		&& (req->Cmd_Flag & CMD_FLAG_PIO))
	        				ctx->received_fis =
		        				sata_store_received_fis(root,
					        			dev->register_set,
						        		(req->Cmd_Flag & (~CMD_FLAG_PIO)));
                                	else
		        			ctx->received_fis =
			        			sata_store_received_fis(root,
				        			dev->register_set,
					        		req->Cmd_Flag);
				} else {
                                        ctx->received_fis = NULL;
				}
			}
			if (dev->register_set != NO_REGISTER_SET) {
				OSSW_SPIN_LOCK(&base->outstanding_req_SpinLock);
				if (dev->base.outstanding_req == 0) {
					OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
					core_free_register_set(root, dev, dev->register_set);
					dev->register_set = NO_REGISTER_SET;
				}else{
					OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
				}
			}
			
		} else {
			OSSW_SPIN_LOCK_Device(base->root->core, flags, base);
			if ((core->has_port_queue) 
				&& dev->register_set != NO_REGISTER_SET) {
				OSSW_SPIN_LOCK(&base->outstanding_req_SpinLock);
				if (dev->base.outstanding_req == 0) {
					OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
					core_free_register_set(root, dev, dev->register_set);
					dev->register_set = NO_REGISTER_SET;
				}else{
					OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
				}
			}
			OSSW_SPIN_UNLOCK_Device(base->root->core, flags, base);
		}
	} 
#if 0 // def SUPPORT_ATHENA
    else  if (base->type == BASE_TYPE_DOMAIN_EXPANDER){
        domain_expander *exp = (domain_expander *)base;
		if (exp->register_set != NO_REGISTER_SET) {
			if (exp->base.outstanding_req == 0) {
				core_free_register_set(root,NULL, exp->register_set);
				exp->register_set = NO_REGISTER_SET;
			}
		}
    }
#endif
    else if (base->type == BASE_TYPE_DOMAIN_PM) {
		domain_pm *pm = (domain_pm *)base;
		OSSW_SPIN_LOCK_Device(base->root->core, flags, base);
		if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
                        if (pm->register_set != NO_REGISTER_SET) {
			        ctx->received_fis =
				        sata_store_received_fis(root,
					        pm->register_set,
					        req->Cmd_Flag);
                        } else {
                                ctx->received_fis = NULL;
                        }
		}

		if (pm->register_set != NO_REGISTER_SET) {
			OSSW_SPIN_LOCK(&base->outstanding_req_SpinLock);
			if (pm->base.outstanding_req == 0) {
				OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
				core_free_register_set(root,NULL, pm->register_set);
				pm->register_set = NO_REGISTER_SET;
			}else{
				OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
			}
		}
		OSSW_SPIN_UNLOCK_Device(base->root->core, flags, base);
	} else if (core->has_port_queue) {
		if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
			domain_expander *exp = (domain_expander *)base;
			OSSW_SPIN_LOCK_Device(base->root->core, flags, base);
			if (exp->register_set != NO_REGISTER_SET) {
				OSSW_SPIN_LOCK(&base->outstanding_req_SpinLock);
				if (exp->base.outstanding_req == 0) {
					OSSW_SPIN_UNLOCK(&base->outstanding_req_SpinLock);
					core_free_register_set(root,exp, exp->register_set);
					exp->register_set = NO_REGISTER_SET;
				}
			}
			OSSW_SPIN_UNLOCK_Device(base->root->core, flags, base);
 		} else if (base->type == BASE_TYPE_DOMAIN_ENCLOSURE) {
 			domain_enclosure *enc = (domain_enclosure *)base;
			OSSW_SPIN_LOCK_Device(base->root->core, flags, base);
			if (enc->register_set != NO_REGISTER_SET) {
				if (enc->base.outstanding_req == 0) {
					core_free_register_set(root,enc, enc->register_set);
					enc->register_set = NO_REGISTER_SET;
				}
			}
			OSSW_SPIN_UNLOCK_Device(base->root->core, flags, base);
		}
	}
#endif
#if 0	//this black cause single-thread mode bug when outside multi-thread clean base->blocked state
	//OSSW_SPIN_LOCK_Device(root->core, flags, base);
	OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
	if(base->blocked){
		base->blocked = MV_FALSE;
	}
	//OSSW_SPIN_UNLOCK_Device(root->core, flags, base);
	OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
#endif
}

MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base;
	core_extension *core = root->core;
	MV_ULONG flags;
#ifdef ASIC_WORKAROUND_WD_TO_RESET
	domain_base *err_req_base;
	domain_port *err_req_port;
	PMV_Request err_req = NULL;
	core_context *err_req_ctx;
//	core_extension *core = root->core;
	MV_BOOLEAN push_back_req = FALSE;
#endif
	base = (domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	MV_DASSERT(ctx != NULL);
	OSSW_SPIN_LOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	prot_clean_slot(root, base, ctx->slot, req);
	OSSW_SPIN_UNLOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	if ((ctx->error_info & EH_INFO_NEED_RETRY)
		&& !CORE_IS_EH_REQ(ctx)
		&& !CORE_IS_INIT_REQ(ctx)) {
		MV_ASSERT(req->Scsi_Status != REQ_STATUS_SUCCESS);
		/*
		 * this request needs further handling like retry
		 * dont do error handling for init request and error handling request
		 */
#ifdef ASIC_WORKAROUND_WD_TO_RESET
		if ((core->revision_id != VANIR_C2_REV) &&
			(ctx->error_info & EH_INFO_WD_TO_RETRY) && (!Counted_List_Empty(&core->error_queue))) {
			// Search for error_queue requests already with WatchDog Timeout error
			LIST_FOR_EACH_ENTRY_TYPE(err_req, &core->error_queue, MV_Request, Queue_Pointer) {
				err_req_ctx = err_req->Context[MODULE_CORE];
				if (err_req_ctx->error_info & EH_INFO_WD_TO_RETRY) {
					// Check if the same port
					err_req_base = (domain_base *)get_device_by_id(
											&core->lib_dev,
											err_req->Device_Id,
											MV_FALSE,
											MV_FALSE
											);
					err_req_port = err_req_base->port;
					if (err_req_port == base->port) {
						push_back_req = MV_TRUE;
						break;
					}
				}
			}
			if (push_back_req) {
				core_push_running_request_back(root, req);
			}
			else {
				// Did not find request already needing WatchDog Timeout Workaround.
				core_queue_error_req(root, req, MV_TRUE);
				// Wait 1 Second for other requeusts to finish
				core_sleep_millisecond(core, 1000);
			}
		}
		else
#endif
		{
			//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
			OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
			core_queue_error_req(root, req, MV_TRUE);
			//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
			OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		}
	} else {
		/* this request is ready for callback
		 * add this request to upper layer's complete queue */
		core_queue_completed_req_lock(root->core, req);
	}
}

/* will return pointer if there is some kind of error */
err_info_record * prot_get_command_error_info(mv_command_table *cmd_table,
	 MV_PU32 cmpl_q)
{
	MV_U64 tmp;
	err_info_record *err_info;

	if (!(*cmpl_q & RXQ_ERR_RCRD_XFRD))
		return NULL;

	tmp.value = (_MV_U64)(*(_MV_U64 *)(&cmd_table->status_buff.err_info));
    if (U64_COMPARE_U32(tmp, 0) != 0) {
#ifdef SUPPORT_BALDUR
        if (MV_LE32_TO_CPU((MV_U32)(*(MV_U32*)(&cmd_table->status_buff.err_info)))
                != DATA_OVR_UNDR_FLW_ERR) {
            CORE_EH_PRINT(("error info 0x%016llx.\n", MV_CPU_TO_LE64(tmp)));
        }
#else
        CORE_EH_PRINT(("error info 0x%016llx.\n", MV_CPU_TO_LE64(tmp)));
#endif
		err_info = &cmd_table->status_buff.err_info;
		return err_info;
	} else {
		return NULL;
	}
}


void prot_fill_sense_data(MV_Request *req, MV_U8 sense_key,
	MV_U8 ad_sense_code)
{
	if (req->Sense_Info_Buffer != NULL) {
		((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70;	/* Current */
		((MV_PU8)req->Sense_Info_Buffer)[2] = sense_key;
		/* additional sense length */
		((MV_PU8)req->Sense_Info_Buffer)[7] = 0xa;
		/* additional sense code */
		((MV_PU8)req->Sense_Info_Buffer)[12] = ad_sense_code;
	}
}
