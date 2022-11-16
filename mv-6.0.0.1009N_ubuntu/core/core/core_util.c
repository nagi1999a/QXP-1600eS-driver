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
#include "core_util.h"
#include "core_internal.h"
#include "core_manager.h"

#include "core_type.h"
#include "core_error.h"
#include "hba_exp.h"

#include "core_spi.h"
#include "core_eeprom.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#include "devid_map.h"
#ifdef SUPPORT_ROC
#ifdef NEW_CORE_DRIVER
#include "core_flash.h"
#endif
#include "com_flash.h"
#endif
#endif
#ifdef ATHENA_PERFORMANCE_TUNNING
MV_U32 g_enable_pre_handle_resource=0;
#endif
extern int have_flash_layout;
int core_suspend(void *ext)
{
	core_extension *core_ext = (core_extension *)ext;

	core_disable_ints(core_ext);

	return 0;
}

int core_resume(void *ext)
{
	MV_U8 i, j;
	core_extension *core_ext = (core_extension *)ext;
	pl_root * root = NULL;

	/* we need chip level resetting */
	if(core_reset_controller(core_ext) == MV_FALSE)
		return -1;

	/* enable the interrupt */
	core_enable_ints(core_ext);

	/* Phy level init */
	for(i = core_ext->chip_info->start_host; i < (core_ext->chip_info->start_host + core_ext->chip_info->n_host); i++){
		root = &core_ext->roots[i];
		io_chip_init_registers(root);
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		for(j =0; j< root->queue_num; j ++){
			root->queues[j].last_cmpl_q = 0x3fff;
			root->queues[j].last_delv_q = 0x3fff;
		}
#else
		root->last_cmpl_q = 0xfff;
		root->last_delv_q = 0xfff;
#endif
		root->running_num = 0;
	}

	return 0;
}

MV_LPVOID get_core_mmio(pl_root *root)
{
	core_extension *core = (core_extension *)root->core;
	return core->mmio_base;
}

MV_VOID core_queue_init_entry(pl_root *root, MV_PVOID base_ext, MV_BOOLEAN start_init)
{
	core_extension *core = (core_extension *)root->core;
	struct _domain_base *base = (struct _domain_base *)base_ext;
	struct _error_context *err_ctx = &base->err_ctx;

#ifdef MV_DEBUG
	{
		MV_U32 count;
		//MV_U8 i;
		struct _domain_base *item;

		count = Counted_List_GetCount(&core->init_queue, MV_TRUE);
		if (count != Counted_List_GetCount(&core->init_queue, MV_FALSE)) {
			CORE_DPRINT(("init_queue count  is conflict %d[%d]\n",count,Counted_List_GetCount(&core->init_queue, MV_FALSE)));

		}
		MV_DASSERT(count == Counted_List_GetCount(&core->init_queue, MV_FALSE));
		LIST_FOR_EACH_ENTRY_TYPE(item , &core->init_queue, struct _domain_base,
			init_queue_pointer) {
			if (item == base) {
				CORE_DPRINT(("find same base at init queue %p.\n",base));
#ifdef STRONG_DEBUG
				MV_DASSERT(item != base);
#endif
				return;
			} 
		}
	}
#endif

	if (MV_TRUE == start_init) {
		core->init_queue_count++;
		base->port->init_count++;
		err_ctx->retry_count = 0;
		Counted_List_AddTail(&base->init_queue_pointer, &core->init_queue);
	} else {
		/* for dump mode and hotplug, we need totally finish one entry first */
		Counted_List_Add(&base->init_queue_pointer, &core->init_queue);
	}
}
#ifdef SUPPORT_STAGGERED_SPIN_UP
MV_U8 enable_spin_up(MV_PVOID hba){
	core_extension *core = (core_extension *)HBA_GetModuleExtension(hba, MODULE_CORE);
	if (core->spin_up_group != 0) {
		core_notify_device_hotplug(&core->roots[core->chip_info->start_host],
						MV_TRUE,VIRTUAL_DEVICE_ID, MV_FALSE);
		return MV_TRUE;
	} else
		return MV_FALSE;

}
MV_U32 core_spin_up_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	size=sizeof(struct device_spin_up)*MV_MAX_TARGET_NUMBER;

	return size;
}

MV_VOID staggered_spin_up_handler(domain_base * base, MV_PVOID tmp)
{
	domain_device *device;
	core_extension *core = (core_extension *)base->root->core;
	MV_U8 num=0,renew_timer=0;
	MV_ULONG flags, flags2;
	struct device_spin_up *su=NULL;
	if((base == NULL) || (core == NULL)){
		return;
		}
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	CORE_DPRINT(("Staggered spin up ...\n"));
	device = (domain_device *)base;
	//OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP((void *)core,flags);
	while(!List_Empty(&core->device_spin_up_list)){
		if((num >= core->spin_up_group) || (core->state != CORE_STATE_STARTED)){
			renew_timer=1;
			break;
		}
		su = (struct device_spin_up *)List_GetFirstEntry(
			&core->device_spin_up_list, struct device_spin_up,list);

		if(su){
			if(su->roots==NULL){
				CORE_DPRINT(("Got a NULL device\n"));
				continue;
			}else{
				if(base->type == BASE_TYPE_DOMAIN_DEVICE){
					domain_device *tmp_device=(domain_device *)su->base;
					tmp_device->state=DEVICE_STATE_RESET_DONE;
				}
				//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags2);
				core_queue_init_entry(su->roots, su->base, MV_TRUE);
				//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags2);
			}
			num++;
			free_spin_up_device_buf(su->roots->lib_rsrc,su);
		}
		
	}
	if(List_Empty(&core->device_spin_up_list)){
		if(core->device_spin_up_timer != NO_CURRENT_TIMER)
			core_cancel_timer(core, core->device_spin_up_timer);
		core->device_spin_up_timer =NO_CURRENT_TIMER;
		}else{
			
			if(core->device_spin_up_timer ==NO_CURRENT_TIMER){
				core->device_spin_up_timer=core_add_timer(core, core->spin_up_time, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, base, NULL);
			}else{
				if(renew_timer){
					core->device_spin_up_timer=core_add_timer(core, core->spin_up_time, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, base, NULL);
				}
			}
		}
	//OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}
#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
MV_VOID core_device_malloc_cmd_resource(core_extension *core,pl_root *root, domain_device *dev){
    MV_U16 i=0, slot;
    cmd_resource *cmd_res=NULL;
    mv_command_header *cmd_header = NULL;
    mv_command_table *cmd_table = NULL;
    mv_command_struct *cmd_struct = NULL;
    hw_buf_wrapper *table_wrapper = NULL, *sg_wrapper = NULL;
    MV_PHYSICAL_ADDR tmpPhy;
    MV_ASSERT(IsListEmpty(&dev->resource_queue));
    MV_ASSERT(dev->base.type ==BASE_TYPE_DOMAIN_DEVICE);
    if(!IS_HDD(dev)){
        MV_DPRINT(("dev%d is not HDD. It's no mallocate resource for it.\n",dev->base.id));
        return;
    }
    for(i=0; i<dev->base.queue_depth; i ++){
        cmd_res = get_cmd_resource(root->lib_rsrc);
        if (Tag_IsEmpty(&root->slot_pool)) {
            free_cmd_resource(root->lib_rsrc, cmd_res);
            return;
        } else {
            slot = Tag_GetOne(&root->slot_pool);
        }
        sg_wrapper = get_sg_buf(root->lib_rsrc);
        if(sg_wrapper == NULL){
            Tag_ReleaseOne(&root->slot_pool, slot);
            free_cmd_resource(root->lib_rsrc, cmd_res);
            return;
        }
        cmd_res->slot = slot;
        cmd_res->cmd_table_wrapper = (MV_PVOID)&root->cmd_struct_wrapper[slot];
        cmd_res->sg_wrapper = (MV_PVOID)sg_wrapper;
        cmd_res->ncq_tag = (MV_U8)i;
        table_wrapper = &root->cmd_struct_wrapper[slot];
        cmd_struct= (mv_command_struct *)table_wrapper->vir;
        cmd_header = &cmd_struct->header.mv_cmd_header;
        cmd_table= &cmd_struct->mv_cmd_table;
        MV_ZeroMemory(table_wrapper->vir, sizeof(mv_command_table)+sizeof(mv_command_header));
        cmd_header->ctrl_nprd = 0;
#if defined(SUPPORT_SP2)
        cmd_header->sgl_prp.prp.prp1= MV_CPU_TO_LE64(sg_wrapper->phy);
#else
        cmd_header->prd_table_addr = MV_CPU_TO_LE64(sg_wrapper->phy);
#endif
        tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, status_buff);
        cmd_header->status_buff_addr = MV_CPU_TO_LE64(tmpPhy);
#ifdef SUPPORT_ATHENA
        cmd_header->reserved[0] = 0;
        cmd_header->reserved[1] = 0;
#endif
        tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, open_address_frame);
        cmd_header->open_addr_frame_addr  = MV_CPU_TO_LE64(tmpPhy);

        /* cmd_header->tag is different between SSP and STP/SATA
        * it'll be further handled in prepare_command handler.
        * refer to ASIC spec. */
        cmd_header->tag = MV_CPU_TO_LE16(slot);
        cmd_header->data_xfer_len = 0;
        core_set_cmd_header_selector(cmd_header);
        if(IS_SSP(dev)){
            MV_U64 val64;
            MV_U32 ch_dword_0 = 0;
            cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(CH_SSP_TP_RETRY);
            cmd_header->max_rsp_frame_len = MV_CPU_TO_LE16(MAX_RESPONSE_FRAME_LENGTH >> 2);
            cmd_header->frame_len = MV_CPU_TO_LE16(((sizeof(ssp_command_iu) + sizeof(ssp_frame_header)+3)/4) & CH_FRAME_LEN_MASK );
#ifdef SUPPORT_BALDUR
            cmd_header->max_sim_conn = 1;
#endif
            ch_dword_0 |= FRAME_TYPE_COMMAND << CH_SSP_FRAME_TYPE_SHIFT;

#ifdef SUPPORT_BALDUR
            ch_dword_0 |= CH_SSP_VERIFY_DATA_LEN;
#endif
            cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(ch_dword_0);

	/*set task attribute according to tcq queue type*/
            cmd_table->table.ssp_cmd_table.data.command.command_iu.task_attribute |= dev->queue_type;

            cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |PROTOCOL_SSP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);

        /* Setting entire byte as negotiated link rate even though only the
           lower 4-bit is link rate is to clear the Feature field in the upper
           4-bit as well. Feature field should always be set to zero */
		((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;

            *(MV_U16 *)(cmd_table->open_address_frame.connect_tag) = MV_CPU_TO_BE16(dev->base.id + 1);

            U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));
            MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
        }else if(IS_STP_OR_SATA(dev)){
            // prepare command header
	/* clear sata ctrl and sas ctrl */
            MV_U32 dword_0= 0;
            MV_U64 val64;
            dword_0 |= (dev->pm_port & CH_PM_PORT_MASK);
            if(dev->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED){
                dword_0 |= 	CH_FPDMA;
                cmd_header->tag = MV_CPU_TO_LE16(cmd_res->ncq_tag);
            }
            cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(dword_0);
            cmd_header->frame_len = MV_CPU_TO_LE16(FIS_REG_H2D_SIZE_IN_DWORD);
            // prepare command table
            {
                sata_fis_reg_h2d *fis = (sata_fis_reg_h2d *)cmd_table->table.stp_cmd_table.fis;
                if(dev->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED){
                    fis->sector_count = (cmd_res->ncq_tag<<3);
                }
                fis->fis_type = SATA_FIS_TYPE_REG_H2D;
                fis->cmd_pm = dev->pm_port & 0x0f;
                fis->cmd_pm |= H2D_COMMAND_SET_FLAG;
                fis->control = 0;
                fis->ICC = 0x0;
                *((MV_PU32)&fis->reserved2[0]) = 0x0;
            }
            if(IS_STP(dev)){
                cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |PROTOCOL_STP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
                    /* Clears the Feature bits */
                ((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;
                *(MV_U16 *)cmd_table->open_address_frame.connect_tag = MV_CPU_TO_BE16(dev->base.id+1);
                U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));
                MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
            }
        }else{
            MV_ASSERT(0);
        }
        InsertTailList(&dev->resource_queue, &cmd_res->list_head);
    }
}
#endif
MV_VOID core_init_entry_done(pl_root *root, domain_port *port,
	domain_base *base)
{
	core_extension *core = (core_extension *)root->core;
	domain_device *dev;
	domain_expander *exp;
	domain_enclosure *enc;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	MV_ULONG flags;
#endif
	if(core->init_queue_count){
		core->init_queue_count--;
	} else {
                #ifdef STRONG_DEBUG
                MV_ASSERT(MV_FALSE);
                #endif
		CORE_DPRINT(("core->init_queue_count is zero.\n"));
	}
#if 0 //def ATHENA_PERFORMANCE_TUNNING
	if(core->init_queue_count == 0){
		MV_U32 i=0;
		domain_base *tmp_base;
		for(i=0; i < MAX_ID; i++){
			if(core->lib_dev.device_map[i] == NULL)
				continue;
			tmp_base = core->lib_dev.device_map[i];
			if(tmp_base->blocked)
				tmp_base->blocked = MV_FALSE;
		}
	}
#endif
        if(port && port->init_count) {
		port->init_count--;
        } else {
        #ifdef STRONG_DEBUG
                MV_ASSERT(MV_FALSE);
        #endif
		CORE_DPRINT(("port(%p) or port->init_queue_count is zero.\n", port));
        }
        /* if init failure, base == NULL */
	if (base != NULL) {
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
		if(g_enable_pre_handle_resource)
            core_device_malloc_cmd_resource(core, root, dev);
#endif
		if(dev->status & DEVICE_STATUS_INTERNAL_HOTPLUG) {
			dev->status &= ~DEVICE_STATUS_INTERNAL_HOTPLUG;
			List_AddTail(&base->queue_pointer, &port->device_list);
			break;
		}

//			MV_ASSERT(dev->status & DEVICE_STATUS_FUNCTIONAL);
			if ((core->state == CORE_STATE_STARTED)){
				core_notify_device_hotplug(root, MV_TRUE,
					base->id, MV_TRUE);
#ifdef SUPPORT_STAGGERED_SPIN_UP
				if(core->spin_up_group){
					OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
					if(dev->status&DEVICE_STATUS_SPIN_UP){
						dev->status &=~DEVICE_STATUS_SPIN_UP;
					}
					OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
				}
#endif
			}
			break;
		case BASE_TYPE_DOMAIN_ENCLOSURE:
			enc = (domain_enclosure *)base;
			if (enc->enc_flag & ENC_FLAG_FIRST_INIT) {
				enc->enc_flag &= ~ENC_FLAG_FIRST_INIT;
				if (core->state == CORE_STATE_STARTED)
					core_notify_device_hotplug(root, MV_TRUE,
						base->id, MV_TRUE);
			}
			break;
		case BASE_TYPE_DOMAIN_PM:
			/* pm is a pointer */
			port->pm = (domain_pm *)base;
#if 0
			if ((core->state == CORE_STATE_STARTED))
			        core_pd_update_device_id(core, base);
#endif
			break;
		case BASE_TYPE_DOMAIN_EXPANDER:
			exp = (domain_expander *)base;
			if ((core->state == CORE_STATE_STARTED) &&
				(exp->enclosure) &&
				(exp->enclosure->enc_flag & ENC_FLAG_NEED_REINIT)) {
				exp->enclosure->state = ENCLOSURE_INQUIRY_DONE;
				exp->enclosure->enc_flag &= ~ENC_FLAG_NEED_REINIT;
				core_queue_init_entry(root,
					&exp->enclosure->base, MV_TRUE);
			}
#if 0
			if ((core->state == CORE_STATE_STARTED))
				core_pd_update_device_id(core, base);
#endif
				if (exp->need_report_plugin) {
					core_generate_event(root->core,
						EVT_ID_EXPANDER_PLUG_IN, exp->base.id,
						SEVERITY_INFO, 0, NULL, 0);
					exp->need_report_plugin = MV_FALSE;
				}
			break;
		case BASE_TYPE_DOMAIN_I2C:
		/* only update the device id one-by-one if
		        it is hotplug */
#if 0
			if ((core->state == CORE_STATE_STARTED))
				core_pd_update_device_id(core, base);
#endif
			break;
		case BASE_TYPE_DOMAIN_PORT:
			/* Nothing to do for port */
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
	}
}

MV_PVOID core_get_handler(pl_root *root, MV_U8 handler_id)
{
	core_extension *core = (core_extension *)root->core;
	return (MV_PVOID)&core->handlers[handler_id];
}

MV_VOID core_append_request(pl_root *root, PMV_Request req)
{
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	if (core_append_queue_request(core, req, MV_FALSE))
		return;

	//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root->root_id);
#ifdef ROOT_WAITING_QUEUE
	OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
	Counted_List_AddTail(&req->Queue_Pointer, &root->waiting_queue);
	OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
#else
	OSSW_SPIN_LOCK(&core->waiting_queue_SpinLock, flags);
	Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
	OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
#endif
	//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root->root_id);
	/* don't push list. avoid recursive call. */
}

MV_VOID core_push_running_request_back(pl_root *root, PMV_Request req)
{
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;

	/* for aborted running request,
	 * we cannot call core_append_high_priority_request
	 * it'll be mixed with eh request.
	 * so have to push back to the normal queue head */

	if (core_append_queue_request(core, req, MV_FALSE))
		return;
	
	//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root->root_id);
#ifdef ROOT_WAITING_QUEUE
	OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
	Counted_List_Add(&req->Queue_Pointer, &root->waiting_queue);
	OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);

#else
	OSSW_SPIN_LOCK(&core->waiting_queue_SpinLock, flags);
	Counted_List_Add(&req->Queue_Pointer, &core->waiting_queue);
	OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
#endif
	//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root->root_id);
	/* don't push list. avoid recursive call. */
}

MV_VOID core_append_high_priority_request(pl_root *root, PMV_Request req)
{
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	MV_U8 root_id=0;

	if (core_append_queue_request(core, req, MV_TRUE))
		return;

	//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, 0);
	OSSW_SPIN_LOCK(&core->high_priority_SpinLock, flags);
	Counted_List_AddTail(&req->Queue_Pointer, &core->high_priority_queue);
	//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, 0);
	OSSW_SPIN_UNLOCK(&core->high_priority_SpinLock, flags);
	/* don't push list. avoid recursive call. */
}

void core_append_init_request(pl_root *root, MV_Request *req)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	
	MV_ASSERT(ctx != NULL);

	ctx->req_type = CORE_REQ_TYPE_INIT;
	req->Time_Out = HBA_CheckIsFlorence(core) ?
		CORE_REQ_FLORENCE_INIT_TIMEOUT : CORE_REQ_INIT_TIMEOUT;
	core_append_request(root, req);
}

void core_queue_eh_req(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	/* error handling request can have two flags
	 * 1. CORE_REQ_TYPE_ERROR_HANDLING
	 * 2. CORE_REQ_TYPE_RETRY
	 * they are all error handling request.
	 * so when fail, won't treat as a new error.
	 * will continue the error handling state machine */
	if (ctx->req_type == CORE_REQ_TYPE_NONE) {
	ctx->req_type = CORE_REQ_TYPE_ERROR_HANDLING;
	}
	MV_ASSERT((ctx->req_type == CORE_REQ_TYPE_ERROR_HANDLING)
		|| (ctx->req_type == CORE_REQ_TYPE_RETRY));

	/* make the timer shorter because
	 * a. single command per device
	 * b. maybe disk is gone already like during hotplug */
#if 0//def SUPPORT_ATHENA
	req->Time_Out = 10;
#else
	req->Time_Out = 2;
#endif
	core_append_high_priority_request(root, req);
}

/*
 * will queue original error request or error handling request
 * 1. original request, has error, waiting for handling
 * 2. error handling request, if state machine cannot continue
 *    because lack of resource
 * compared with core_queue_eh_req, that req is waiting for sending
 * but this function is waiting for state machine handling
 */
void core_queue_error_req(pl_root *root, MV_Request *req, MV_BOOLEAN new_error)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base;
	MV_ULONG flags, flags1;

	base = (domain_base *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	MV_ASSERT(ctx);
	MV_ASSERT(!CORE_IS_INIT_REQ(ctx));
	OSSW_SPIN_LOCK(&core->error_queue_SpinLock, flags);
	if (new_error) {
		/* original error request */
		CORE_EH_PRINT(("dev %d queue original error req %p [0x%x].\n",\
			base->id, req, req->Cdb[0]));
		//OSSW_SPIN_LOCK(&base->err_ctx.sent_req_list_SpinLock, flags1);
		OSSW_SPIN_LOCK( &base->base_SpinLock, flags1);
		base->err_ctx.error_count++;

		MV_ASSERT(!CORE_IS_EH_REQ(ctx));
		Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
		OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags1);

//		OSSW_SPIN_UNLOCK(&base->err_ctx.sent_req_list_SpinLock, flags1);
	} else {
		/* any request in the error handling state machine
		 * it's pushed back to queue because lack of resource
		 * it can be the original request too if it's retried */
		CORE_EH_PRINT(("no resource, push back error req %p [0x%x].\n",\
			req, req->Cdb[0]));

                /* sata_media_error_state_machine may queue original error again
                 * in order to switch from media error state machine to timeout state machine. */
		//MV_ASSERT(CORE_IS_EH_REQ(ctx));
		Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
	}
	OSSW_SPIN_UNLOCK(&core->error_queue_SpinLock, flags);
}

/*
 * only the original error req will come here
 * it's used to restore some data structure for the original error req
 */
void core_complete_error_req(pl_root *root, MV_Request *req, MV_U8 status)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base;
	MV_ULONG flags, flags_base;
	struct _error_context *err_ctx;

	base = (domain_base *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
    err_ctx = &base->err_ctx;
	OSSW_SPIN_LOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	OSSW_SPIN_LOCK( &base->base_SpinLock, flags_base);

	CORE_DPRINT(("returns %p with status 0x%x.\n", \
		req, status));
	MV_ASSERT(ctx->req_type == CORE_REQ_TYPE_NONE);
	if ((req->Scsi_Status == REQ_STATUS_NO_DEVICE) && (err_ctx->eh_type == EH_TYPE_TIMEOUT)) {
		domain_device *device = (domain_device *)base;
		CORE_DPRINT(("Find device %d is bad disk\n",base->id));
		device->status = (DEVICE_STATUS_NO_DEVICE|DEVICE_STATUS_FROZEN);
	}

	/* stop the state machine */
	err_ctx->eh_type = EH_TYPE_NONE;
	err_ctx->eh_state = EH_STATE_NONE;
	err_ctx->error_count--;
	err_ctx->error_req = NULL;
	/* maybe interrupt handling kick in */
	//MV_ASSERT(List_Empty(&(err_ctx->sent_req_list)));
	/* may hit ASSERT if jump to set down disk when timeout > allowed times. */
	//MV_ASSERT(err_ctx->state == BASE_STATE_ERROR_HANDLING);
#ifdef SUPPORT_ENHANCED_EH		
	if ((status == REQ_STATUS_SUCCESS)
		&& (err_ctx->state == BASE_STATE_ERROR_HANDLING)) {
		err_ctx->state = BASE_STATE_ERROR_HANDLED;
		//CORE_DPRINT(("set base state to ERROR handled.\n"));
	} else
#endif
		err_ctx->state = BASE_STATE_NONE;

	/* if error handling ever used timer to run the eh state machine, cancel the timer. */
	if (err_ctx->eh_timer != NO_CURRENT_TIMER) {
		core_cancel_timer(core, err_ctx->eh_timer);
		err_ctx->eh_timer = NO_CURRENT_TIMER;
	}

	/* never return this status to upper layer */
	MV_ASSERT(status != REQ_STATUS_TIMEOUT);
	req->Scsi_Status = status;

	if ((req->Scsi_Status == REQ_STATUS_SUCCESS) 
		&& (req->Sense_Info_Buffer_Length != 0)
		&& (req->Sense_Info_Buffer != NULL)) {
 		((MV_PU8)req->Sense_Info_Buffer)[0] = 0; 
		((MV_PU8)req->Sense_Info_Buffer)[2] = 0;
		/* additional sense length */
		((MV_PU8)req->Sense_Info_Buffer)[7] = 0;
		/* additional sense code */
		((MV_PU8)req->Sense_Info_Buffer)[12] = 0;
		/* additional sense code qualifier*/
		((MV_PU8)req->Sense_Info_Buffer)[13] = 0;
	}
	OSSW_SPIN_UNLOCK(&base->base_SpinLock, flags_base);
	OSSW_SPIN_UNLOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
	core_queue_completed_req(core, req);
}
void
core_notify_device_hotplug(pl_root *root, MV_BOOLEAN plugin,
	MV_U16 dev_id, MV_U8 generate_event)
{
	core_extension *core = (core_extension *)root->core;

	struct mod_notif_param param;
	MV_PVOID upper_layer;
	MV_VOID (*notify_func)(MV_PVOID, enum Module_Event, struct mod_notif_param *);
	domain_base *base;
	MV_U16 report_id = 0xffff;
#ifdef SCSI_ID_MAP
    domain_device *device;
    MV_U16 spec_id = 0;
    MV_U32 addr;
    PHBA_Extension hba = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
#endif
	if (core->state != CORE_STATE_STARTED) return;

	base = get_device_by_id(root->lib_dev, dev_id, MV_FALSE, MV_FALSE);
	if (base == NULL) {
		MV_ASSERT(MV_FALSE);
#ifdef SUPPORT_STAGGERED_SPIN_UP
		return;
#endif
		goto core_notify_device_hotplug_done;
	}
#ifdef SCSI_ID_MAP
    device = (domain_device *)base;
#ifdef SUPPORT_SAVE_TABLE
#ifdef SUPPORT_ROC
    addr = FLASH_PDINFOPAGE_ADDR;
    if (!Core_NVMRd(NULL, MFR_FLASH_DEV(0), addr, FLASH_TABLE_SIZE, (MV_PU8)hba->flash_table, 0)) {
        FM_PRINT("%s %d %s ... Read Flash for flash table is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
    }
#else
    addr = ODIN_FLASH_SIZE - PD_INFO_PAGE_OFFSET;
    if (core_rmw_read_flash(core, addr,(MV_PU8)hba->flash_table, FLASH_TABLE_SIZE) == MV_FALSE) {
        MV_ASSERT(MV_FALSE);
    }
#endif
#endif
#endif
	/* PD page updates should be done before notifying upper layers */
	if (plugin == MV_TRUE) {
#ifdef IGNORE_FIRST_PARITY_ERR
        if(base->type == BASE_TYPE_DOMAIN_PM || base->type == BASE_TYPE_DOMAIN_EXPANDER || base->type == BASE_TYPE_DOMAIN_DEVICE){
            core_enable_intl_parity(core);
        }
#endif
#ifdef SCSI_ID_MAP
        if (base->parent->type == BASE_TYPE_DOMAIN_PM) {
            if (hba->have_pm) {
                spec_id = base->port->phy->id + root->base_phy_num;
                spec_id *= device->pm->num_ports;
                spec_id += device->pm_port;
                spec_id += DEVICE_PORT_NUMBER;
                report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
#ifdef SUPPORT_SAVE_TABLE
                hba->flash_table[spec_id] = device->WWN;
#endif
            }
            else {
#ifdef SUPPORT_SAVE_TABLE
                spec_id = get_flash_map_index_ex(hba->flash_table, device->WWN, hba->have_pm);
                if (spec_id != 0xFFFF) {
                    report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
                }
                else {
                    spec_id = get_unused_device_map_item(hba->map_table, hba->have_pm);
                    if (spec_id != 0xFFFF) {
                    	replace_flash_map_item(hba->flash_table, spec_id, device->WWN);
                        report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
                    }
                    else {
                        FM_PRINT("%s %d %s ... flash table is full....\n", __FILE__, __LINE__, __FUNCTION__);
                        MV_ASSERT(MV_FALSE);
                    }
                }
#else
                report_id = get_device_map_index_ex(hba->map_table, device->WWN, base->id, hba->have_pm, DEVICE_TYPE_PD);
#endif
            }
        }
        else if (base->parent->type == BASE_TYPE_DOMAIN_PORT) {
            spec_id = base->port->phy->id + root->base_phy_num;
            report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
#ifdef SUPPORT_SAVE_TABLE
            hba->flash_table[spec_id] = device->WWN;
#endif
        }
        else if (base->parent->type == BASE_TYPE_DOMAIN_EXPANDER){
#ifdef SUPPORT_SAVE_TABLE
            spec_id = get_flash_map_index_ex(hba->flash_table, device->WWN, hba->have_pm);
            if (spec_id != 0xFFFF) {
                report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
            }
            else {
                spec_id = get_unused_device_map_item(hba->map_table, hba->have_pm);
                if (spec_id != 0xFFFF) {
                	replace_flash_map_item(hba->flash_table, spec_id, device->WWN);
                    report_id = add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
                }
                else {
                    FM_PRINT("%s %d %s ... flash table is full....\n", __FILE__, __LINE__, __FUNCTION__);
                    MV_ASSERT(MV_FALSE);
                }
            }
#else
            report_id = get_device_map_index_ex(hba->map_table, device->WWN, base->id, hba->have_pm, DEVICE_TYPE_PD);
#endif
        }
#else
#if 0
		core_pd_update_device_id(core, base);
#ifdef SUPPORT_SAVE_CACHE_IN_FLASH
		core_pd_update_device_cache_setting(core, base);
#endif
		core_pd_flush_pd_buffer(core); //ikwong:tbd
#endif
	
#if (defined(FIX_SCSI_ID_WITH_PHY_ID) || defined(FIXED_DEVICE_ID_6440))
	{
#endif
			report_id = base->id;
#if (defined(FIX_SCSI_ID_WITH_PHY_ID) || defined(FIXED_DEVICE_ID_6440))
}
#endif
#endif
		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_ONLINE,
					base->id, SEVERITY_INFO, 0, NULL ,0);
		} else {
#ifdef SUPPORT_SES
			if(base->type != BASE_TYPE_DOMAIN_ENCLOSURE)
#endif
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_ONLINE,
					base->id, SEVERITY_INFO, 0, NULL ,0);
		}
	} else {
#ifdef SCSI_ID_MAP
        report_id = get_device_id_by_wwn_id(hba->map_table, device->WWN, base->id);
        if (report_id == 0xFFFF)
            return;
#ifdef SUPPORT_SAVE_TABLE
        if ((report_id<8) || ((hba->have_pm) && (report_id < (DEVICE_PORT_NUMBER * (MAX_PM_PORT_NUMBER))))) {
            release_available_wwn_item(hba->flash_table, device->WWN);
        }
#endif
#else
			report_id = base->id;
#endif

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_OFFLINE,
					base->id, SEVERITY_WARNING, 0, NULL ,0);
		} else {
#ifdef SUPPORT_SES
			if(base->type != BASE_TYPE_DOMAIN_ENCLOSURE)
#endif
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_OFFLINE,
					base->id, SEVERITY_WARNING, 0, NULL ,0);
		}
	}
#ifdef SUPPORT_SAVE_TABLE
#ifdef SUPPORT_ROC
    if ((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), addr, FLASH_TABLE_SIZE, (MV_PU8)hba->flash_table, 0))) {
        FM_PRINT("%s %d %s ... Write Flash for flash table is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
    }
#else
    if ((core_rmw_write_flash(core, addr,(MV_PU8)hba->flash_table, FLASH_TABLE_SIZE) == MV_FALSE)) {
        MV_ASSERT(MV_FALSE);
    }
#endif
#endif

core_notify_device_hotplug_done:

	CORE_PRINT(("%s device %d.\n",
		plugin? "plugin" : "unplug",
		report_id));

	HBA_GetUpperModuleNotificationFunction(core, &upper_layer, &notify_func);

	/* We need to use the ID in the domain_base instead of using dev_id
	  * because the ID may be different after running PD Page device ID
	  * mapping
	  */
#ifdef SUPPORT_MUL_LUN
	param.lo = base->TargetID;
	param.hi = base->LUN;
#else
	param.lo = report_id;
#endif
    	param.param_count = 0;

	notify_func(upper_layer,
		plugin ? EVENT_DEVICE_ARRIVAL : EVENT_DEVICE_REMOVAL,
		&param);
}

MV_U16 core_add_timer(MV_PVOID core, MV_U32 seconds,
	MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	MV_PVOID context1, MV_PVOID context2)
{

	MV_U16 timer_id =
		Timer_AddRequest(core, seconds*2, routine, context1, context2);
	MV_DASSERT(timer_id != NO_CURRENT_TIMER);
	return timer_id;
}

MV_VOID core_cancel_timer(MV_PVOID core, MV_U16 timer)
{
	Timer_CancelRequest(core, timer);
}

MV_VOID core_generate_error_event(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = core_p;
	domain_device *dev;
	MV_U32 params[MAX_EVENT_PARAMS];
	MV_PU8 sense;// = (MV_PU8)req->Sense_Info_Buffer;    //sometimes req->Sense_Info_Buffer = NULL
	MV_U8 sense_key;// = sense[2] & 0x0f;

	dev = (domain_device *)get_device_by_id(&core->lib_dev,
			req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));

        if(req->Sense_Info_Buffer == NULL) {
                if (HBA_CheckIsFlorence(core)) {
                        params[0] = req->Cdb[0];
                        params[1] = 0xFF; /* fake sense code */
                        params[2] = req->Scsi_Status;
                        params[3] = 0;

        	        core_generate_event(core, EVT_ID_HD_SC_ERROR, dev->base.id,
	                        SEVERITY_INFO, 4, params, 0);
                }
		return;
        }
	sense = (MV_PU8)req->Sense_Info_Buffer;
	sense_key = sense[2] & 0x0f;

#ifdef REDUCED_SENSE
        if (!HBA_CheckIsFlorence(core)) {
	        if ((sense_key == SCSI_SK_NO_SENSE) ||
	                (sense_key == SCSI_SK_UNIT_ATTENTION) ||
	                (sense_key == SCSI_SK_NOT_READY) ||
	                (sense_key == SCSI_SK_ILLEGAL_REQUEST))
	                return;
        }
#endif

	params[0] = req->Cdb[0];
	params[1] = sense[2];           /* Sense Key */
	params[2] = sense[12];          /* Additional Sense Code */
	params[3] = sense[13];          /* Additional Sense Code Qualifier */

	core_generate_event(core, EVT_ID_HD_SC_ERROR, dev->base.id,
	        SEVERITY_INFO, 4, params, 0x1e);
}

MV_U32 List_Get_Count(List_Head * head)
{
	List_Head *pPos;
	MV_U32 count = 0;

	LIST_FOR_EACH(pPos, head) {
		count++;
	}
	return count;
}

MV_PVOID core_map_data_buffer(MV_Request *req)
{
	sg_map(req);
	return req->Data_Buffer;
}

MV_VOID core_unmap_data_buffer(MV_Request *req)
{
	sg_unmap(req);
}

MV_U8
core_check_duplicate_device(pl_root *root,
	domain_device *dev)
{
	core_extension  *core = (core_extension *)root->core;
	domain_base     *tmp_base;
	domain_device   *tmp_dev;
	MV_U16          dev_id;
#if (defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)) && defined(_OS_WINDOWS)
	extern ULONG g_enable_multi_path;
#endif

	if (U64_COMPARE_U32(dev->WWN, 0) == 0) {
#ifndef SUPPORT_MUL_LUN
		MV_DASSERT(MV_FALSE);
#endif
		CORE_PRINT(("Device WWN is 0. Not checking duplicate.\n"));
		return MV_FALSE;
	}
#if (defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)) && defined(_OS_WINDOWS)
	if(g_enable_multi_path)
		return MV_FALSE;
#endif
	for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
		if (dev_id == dev->base.id)
			continue;

		tmp_base = get_device_by_id(&core->lib_dev, dev_id, MV_FALSE, MV_FALSE);

		if (tmp_base == NULL)
			continue;

		if (tmp_base->type != BASE_TYPE_DOMAIN_DEVICE)
			continue;

		tmp_dev = (domain_device*)tmp_base;

		if (U64_COMPARE_U64(dev->WWN, tmp_dev->WWN) == 0)
			return MV_TRUE;
	}

	return MV_FALSE;
}
MV_U32
core_check_outstanding_req(MV_PVOID core_p)
{
        core_extension *core = (core_extension *)core_p;
        domain_base *base;
        MV_U32 result = 0;
        MV_U16 dev_id;

        for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
                base = get_device_by_id(&core->lib_dev, dev_id, MV_FALSE, MV_FALSE);
                if (base == NULL)
                        continue;
                result += base->outstanding_req;
        }

        return result;
}


#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND

/*
 * core_wideport_load_balance_asic_phy_map
 *
 * This function returns the asic phy map to use to send a frame to a device.
 * The phy maps returned are of round robin per SAS device, and that SAS
 * device will use the same phy for all commands. STP devices should not
 * use software load balance as hardware will balance the register sets used.
 */
MV_U8
core_wideport_load_balance_asic_phy_map(domain_port *port, domain_device *dev)
{
	MV_U8 phy_map;
	MV_U8 asic_phy_map = port->asic_phy_map;
	domain_base *base = &dev->base;
	/* Only domain_device defined "curr_phy_map", just return for other device(eg, enclosure).*/
	if (BASE_TYPE_DOMAIN_DEVICE != base->type)
		return asic_phy_map;

	if ((dev->curr_phy_map & asic_phy_map) != 0) return dev->curr_phy_map;

	if (asic_phy_map == 0) return asic_phy_map;

	phy_map = port->curr_phy_map << 1;
	if (phy_map > asic_phy_map || phy_map == 0)
		phy_map = (MV_U8)MV_BIT(ossw_ffs(asic_phy_map));
	while (phy_map <= asic_phy_map) {
		if (phy_map & asic_phy_map) {
			port->curr_phy_map = phy_map;
			dev->curr_phy_map = phy_map;
			return phy_map;
		}
		phy_map <<= 1;
	}

	MV_DASSERT(MV_FALSE);

	return asic_phy_map;
}
#endif /* CORE_WIDEPORT_LOAD_BALACE_WORKAROUND */

MV_U32 mv_read_addr_data_regs(MV_LPVOID *mmio_base,
									MV_U32 addr_reg,
									MV_U32 data_reg,
									MV_U32 cmd)
{
	MV_REG_WRITE_DWORD(mmio_base, addr_reg, cmd);
	return(MV_REG_READ_DWORD(mmio_base, data_reg));
}

MV_LPVOID mv_get_mmio_base(void *ext)
{
	core_extension *core = NULL;
	
	core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
	if (!core)
	{
		MV_PRINT("core_extension NULL\n");
		return NULL;
	}

	return core->mmio_base;
}

int mv_dump_sas_sata_port_cfg_ctrl_reg(MV_LPVOID *mmio_base,
											char *pBuffer, MV_U16 buf_len, int len)
{
	MV_U32	 cmds[] = { CONFIG_SATA_CONTROL,     // Port n SATA Control						
						CONFIG_SATA_SIG0,
						CONFIG_SATA_SIG1,
						CONFIG_SATA_SIG2,
						CONFIG_SATA_SIG3,        // Port n SATA Signature(0..3)
						CONFIG_R_ERR_COUNT,
						CONFIG_CRC_ERR_COUNT,    // Port n SATA/STP (R_ERR, CRC Error) Counter
						CONFIG_WIDE_PORT         // Port n Wide Port Participating
						};
	char *name[] = {"port_SATAControl           ",					
					"port_SATASignature0        ",//20
					"port_SATASignature1        ",
					"port_SATASignature2        ",
					"port_SATASignature3        ",
					"port_SATA/STPR_ERRCounter  ",
					"port_SATA/STPCRCCounter    ",
					"port_WidePortParticipating "
					};
		            
	int		portIdx = 0;
	int		cmdIdx =  0;
	MV_U32	cmd =     0;
	MV_U32	addr =    0;
	MV_U32	data =    0;
	const int	cmdNum =  sizeof(cmds) / sizeof(MV_U32);
#ifdef VS_2012
	len += sprintf_s(pBuffer + len, buf_len - len, "SAS/SATA Port Cfg Ctrl Address(0x018-0x38)(%d)\n", cmdNum);
#else
	len += sprintf(pBuffer + len, "SAS/SATA Port Cfg Ctrl Address(0x018-0x38)(%d)\n", cmdNum);
#endif
	for (cmdIdx = 0; cmdIdx < cmdNum; cmdIdx++)
	{
		cmd = cmds[cmdIdx];
#ifdef VS_2012
		len += sprintf_s(pBuffer + len, buf_len - len, "  %s %x[", name[cmdIdx], cmd);
#else
		len += sprintf(pBuffer + len, "  %s %x[", name[cmdIdx], cmd);
#endif
		for (portIdx = 0; portIdx < 8; portIdx++)
		{
			if (portIdx != 0) {
#ifdef VS_2012
				len += sprintf_s(pBuffer + len, buf_len - len, ",");
#else
				len += sprintf(pBuffer + len, ",");
#endif
                        }
			addr = MV_IO_CHIP_REGISTER_BASE + COMMON_PORT_CONFIG_ADDR0 + portIdx * 8;
			data = mv_read_addr_data_regs(mmio_base, addr, addr + 4, cmd);
#ifdef VS_2012
			len += sprintf_s(pBuffer + len, buf_len - len, "%x ", data);
#else
			len += sprintf(pBuffer + len, "%x ", data);
#endif
			if (portIdx == 7) {
#ifdef VS_2012
				len += sprintf_s(pBuffer + len, buf_len - len, "]\n");
#else
				len += sprintf(pBuffer + len, "]\n");
#endif

                        }
		}
	}
#ifdef VS_2012
	len += sprintf_s(pBuffer + len, buf_len - len, "\n");
#else
	len += sprintf(pBuffer + len, "\n");
#endif

	return len;
}

 int mv_dump_intr_reg(MV_LPVOID *mmio_base,
					char *pBuffer, MV_U16 buf_len, int len)
{
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2) /*fs TODO */
	int 	  idx;	
	
	MV_U32 	  intr[] = {COMMON_COAL_CONFIG,   /* interrupt coalescing config */
						COMMON_COAL_TIMEOUT,  /* interrupt coalescing time wait */
						COMMON_IRQ_STAT,      /* interrupt status */
						COMMON_IRQ_MASK     /* interrupt enable/disable mask */										
						}; /* SRS intr enable/disable mask 1*/

	char *name[] = {"intr_CoalescingCfg     ", //0x20148, 
					"intr_CoalsecingTimeout ", //0x2014c, 
					"intr_SAS/SATACause     ", //0x20150,
					"intr_SAS/SATAEnable    " //0x20154, 					
				  };
	const int	addr_num = sizeof(intr) / sizeof(MV_U32);
#ifdef VS_2012
	len += sprintf_s(pBuffer + len, buf_len - len, "Interrupt Regs dump(Addr root:0x148-0x154)\n");
	for (idx = 0; idx < addr_num; idx++)
		len += sprintf_s(pBuffer + len, buf_len - len, "  %s 0X%x:%x \n", name[idx],
					  intr[idx] + MV_IO_CHIP_REGISTER_BASE, MV_REG_READ_DWORD(mmio_base, intr[idx] + MV_IO_CHIP_REGISTER_BASE));
	len += sprintf_s(pBuffer + len, buf_len - len, "\n");
#else
	len += sprintf(pBuffer + len, "Interrupt Regs dump(Addr root:0x148-0x154)\n");
	for (idx = 0; idx < addr_num; idx++)
		len += sprintf(pBuffer + len, "  %s 0X%x:%x \n", name[idx],
					  intr[idx] + MV_IO_CHIP_REGISTER_BASE, MV_REG_READ_DWORD(mmio_base, intr[idx] + MV_IO_CHIP_REGISTER_BASE));
	len += sprintf(pBuffer + len, "\n");
#endif
#endif
	return len;
}

int mv_read_type_reg(MV_LPVOID *mmio_base, char *buf, MV_U32 offset, MV_U16 buf_len, char *type, int len)
{
	MV_U32 value = 0;
	int i;
	MV_U32 addr;
	if (MV_CompareMemory(type, "port", 5) == 0)
	{
#ifdef VS_2012
		len += sprintf_s(buf + len, buf_len - len, "SAS/SATA Port Cfg Ctrl Reg: %x[", offset);
#else
		len += sprintf(buf + len, "SAS/SATA Port Cfg Ctrl Reg: %x[", offset);
#endif
		for (i = 0; i < 8; i++)
		{
			if (i != 0) {
#ifdef VS_2012
				len += sprintf_s(buf + len, buf_len - len, ",");
#else
				len += sprintf(buf + len, ",");
#endif
                        }
			addr = MV_IO_CHIP_REGISTER_BASE + COMMON_PORT_CONFIG_ADDR0 + i * 8;
			value = mv_read_addr_data_regs(mmio_base, addr, addr + 4, offset);
#ifdef VS_2012
			len += sprintf_s(buf + len, buf_len - len, "%x ", value);
#else
			len += sprintf(buf + len, "%x ", value);
#endif
			if (i == 7){
#ifdef VS_2012
				len += sprintf_s(buf + len, buf_len - len, "]\n");
#else
				len += sprintf(buf + len, "]\n");
#endif
                         }
		}	    
	}
	else if (MV_CompareMemory(type, "cmds", 5) == 0)
	{
		value = mv_read_addr_data_regs(mmio_base, MV_IO_CHIP_REGISTER_BASE + COMMON_CMD_ADDR,
						 MV_IO_CHIP_REGISTER_BASE + COMMON_CMD_DATA, offset);
#ifdef VS_2012
		len += sprintf_s(buf + len, buf_len - len, "SAS/SATA Cmd Reg:0X%x  Value:%x  \n",
#else
		len += sprintf(buf + len, "SAS/SATA Cmd Reg:0X%x  Value:%x  \n",
#endif
				 offset, value);
	}
	else if (MV_CompareMemory(type, "intr", 5) == 0)
	{
		value = MV_REG_READ_DWORD(mmio_base, offset);
#ifdef VS_2012
		len += sprintf_s(buf + len, buf_len - len, "Interrupt Reg:0X%x  Value:%x  \n",
				 offset, value);
#else
		len += sprintf(buf + len, "Interrupt Reg:0X%x  Value:%x  \n",
				 offset, value);
#endif
	}
	else if (MV_CompareMemory(type, "comm", 5) == 0)
	{
		value = MV_REG_READ_DWORD(mmio_base, offset);
#ifdef VS_2012
		len += sprintf_s(buf + len, buf_len - len, "Reg:0X%x  Value:%x  \n",
				 offset, value);
#else
		len += sprintf(buf + len, "Reg:0X%x  Value:%x  \n",
				 offset, value);
#endif
	}	
	else
	{
#ifdef VS_2012
		len += sprintf_s(buf + len, buf_len - len, "The reg type or data format is Unknown, type should be comm/intr/port/cmds.\n");				
#else
		len += sprintf(buf + len, "The reg type or data format is Unknown, type should be comm/intr/port/cmds.\n");				
#endif
	}

	return len;
}

void mv_write_type_reg(MV_LPVOID *mmio_base, MV_U32 addr_reg,MV_U32 value_reg, char *type)
{
    int i = 0;
	
	if (MV_CompareMemory(type, "port", 5) == 0)
	{
		for (i = 0; i < 8; i++)
		{
			MV_REG_WRITE_DWORD(mmio_base, MV_IO_CHIP_REGISTER_BASE + COMMON_PORT_CONFIG_ADDR0 + i * 8, addr_reg);
			MV_REG_WRITE_DWORD(mmio_base, MV_IO_CHIP_REGISTER_BASE + COMMON_PORT_CONFIG_ADDR0 + i * 8 + 4, value_reg);
		} 
	}
	else if (MV_CompareMemory(type, "cmds", 5) == 0)
	{
		MV_REG_WRITE_DWORD(mmio_base,  MV_IO_CHIP_REGISTER_BASE + COMMON_CMD_ADDR, addr_reg);
		MV_REG_WRITE_DWORD(mmio_base, MV_IO_CHIP_REGISTER_BASE + COMMON_CMD_DATA, value_reg);
	}
	else if (MV_CompareMemory(type, "intr", 5) == 0)
	{
		MV_REG_WRITE_DWORD(mmio_base, addr_reg, value_reg);
	}
	else if (MV_CompareMemory(type, "comm", 5) == 0)
	{
		MV_REG_WRITE_DWORD(mmio_base, addr_reg, value_reg);
	}
	else
	{
		MV_DPRINT(("The reg type is not contain.\n"));
	}
}

void core_global_lock(void *ext, unsigned long *flags)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
#if defined(_OS_LINUX) || defined(__QNXNTO__)
	//MV_ULONG local_flags;
	//spin_lock_irqsave((spinlock_t *)&core->core_global_SpinLock, *flags);
	//*flags = save_flag;
	//spin_lock_irq((spinlock_t *)&core->core_global_SpinLock);
	//OSSW_SPIN_LOCK(&core->core_global_SpinLock, *flags);
	//OSSW_SPIN_LOCK_CORE_QUEUE(ext, local_flags);
	core->pause_waiting_queue = MV_TRUE;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, local_flags);

	while (core->waiting_queue_running) {
		//core_sleep_millisecond(core, 1);
		udelay(1);
	}
#else		// _OS_WINDOWS
	MV_ULONG local_flags=0;
	OSSW_SPIN_LOCK(&core->core_global_SpinLock, local_flags);
	*flags = local_flags;
	//OSSW_SPIN_LOCK_CORE_QUEUE(ext, local_flags);
	core->pause_waiting_queue = MV_TRUE;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, local_flags);

	while (core->waiting_queue_running) {
		core_sleep_millisecond(core, 1);
	}
#endif
	//OSSW_SPIN_LOCK_CORE(ext, *flags);
}

void core_global_unlock(void *ext, unsigned long *flags)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
#if defined(_OS_LINUX) || defined(__QNXNTO__)
	//MV_ULONG save_flag = *flags;

	core->pause_waiting_queue = MV_FALSE;

	//spin_unlock_irqrestore((spinlock_t *)&core->core_global_SpinLock, *flags);
	//spin_unlock_irq((spinlock_t *)&core->core_global_SpinLock);
	//OSSW_SPIN_UNLOCK(&core->core_global_SpinLock, *flags);
	//OSSW_SPIN_UNLOCK_CORE(ext, *flags);
#else		// _OS_WINDOWS
	MV_ULONG save_flag = *flags;
	core->pause_waiting_queue = MV_FALSE;
	OSSW_SPIN_UNLOCK(&core->core_global_SpinLock, save_flag);
#endif
}

