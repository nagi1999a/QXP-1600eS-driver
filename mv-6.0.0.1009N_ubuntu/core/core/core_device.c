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

#include "core_device.h"
#include "core_internal.h"
#include "core_resource.h"
#include "core_manager.h"
#include "com_list.h"

#include "core_error.h"
#include "core_util.h"

domain_base *get_device_by_id(lib_device_mgr *lib_dev, MV_U16 id, MV_BOOLEAN mapped, MV_BOOLEAN type)
{
	MV_U16 map_id;
	core_extension *core = CONTAINER_OF(lib_dev, core_extension, lib_dev);
#ifdef SCSI_ID_MAP
	PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);

	if (mapped) {
		map_id = get_internal_id_by_device_id(hba->map_table, id);
		if (map_id == 0xFFFF) {
			return NULL;
		}
		id = map_id;
	}
#else
#if defined (SUPPORT_MUL_LUN) && defined (_OS_WINDOWS)
    if (type) {
        map_id = get_base_id_by_target_lun(core, DEV_ID_TO_TARGET_ID(id), DEV_ID_TO_LUN(id));
        if (map_id == 0xFFFF) {
            return NULL;
        }
    id = map_id;
    }
#endif
#endif

    if (id >= MAX_ID)
        return NULL;

	return lib_dev->device_map[id];
}

MV_U16 get_id_by_phyid(lib_device_mgr *lib_dev, MV_U16 phy_id)
{
	domain_base *base;
	MV_U16 i = 0xFFFF;

	for(i=0; i< MAX_ID; i++) {
		base = lib_dev->device_map[i];
		if (base == NULL) continue;
		if (base->port == NULL) continue;
		if (base->port->phy == NULL) continue;
		if (base->port->phy->id == phy_id){
			return i;
		}				
	}
	return 0xFFFF;
}

#ifdef SUPPORT_MUL_LUN
MV_U16 get_id_by_targetid_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun)
{
	core_extension *core = HBA_GetModuleExtension(ext, MODULE_CORE);
	lib_device_mgr *lib_dev = &core->lib_dev;
	MV_U16 i = id;
	if (id >= MAX_ID)
		return i;
	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base ==NULL)
			continue;
		if ( (base->TargetID== id)&&(base->LUN == lun)){
			return i;
		}				
	}
	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base ==NULL)
			return i;			
	}
	return 0xffff;
}
MV_U16 get_base_id_by_target_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun)
{
    core_extension *core = HBA_GetModuleExtension(ext, MODULE_CORE);
    lib_device_mgr *lib_dev = &core->lib_dev;
    MV_U16 i = id;
    if (id >= MAX_ID)
        return 0xffff;
    for(i=0; i<MAX_ID; i++) {
        domain_base *base;
        base = lib_dev->device_map[i];
        if (base ==NULL)
            continue;
        if ((base->TargetID== id)&&(base->LUN == lun)){
            return i;
        }
    }
    return 0xffff;
}
domain_base *get_base_by_target_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun)
{
    core_extension *core = HBA_GetModuleExtension(ext, MODULE_CORE);
    lib_device_mgr *lib_dev = &core->lib_dev;
    MV_U16 i = id;
    if (id >= MAX_ID)
        return NULL;
    for(i=0; i<MAX_ID; i++) {
        domain_base *base;
        base = lib_dev->device_map[i];
        if (base ==NULL)
            continue;
        if ((base->TargetID== id)&&(base->LUN == lun)){
            return base;
        }
    }
    return NULL;
}

MV_U16 get_device_lun(lib_device_mgr *lib_dev, MV_U16 id)
{
	domain_base *base=NULL;
	base = lib_dev->device_map[id];
	if(base == NULL)
		return 0xffff;
	return base->LUN ;

}
MV_U16 get_device_targetid(lib_device_mgr *lib_dev, MV_U16 id)
{
	domain_base *base=NULL;
	base = lib_dev->device_map[id];
	if(base == NULL)
		return 0xFFFF;
	return base->TargetID;

}

#endif
#ifdef SUPPORT_BALDUR
/* only applies to SATA or STP drives */
domain_device *get_device_by_register_set(pl_root *root,
	MV_U8 register_set)
{
	lib_device_mgr *lib_dev = root->lib_dev;
	domain_device *device;
	domain_base *base;
	MV_U16 i;

	for (i = 0; i < MAX_ID; i++) {
		base = lib_dev->device_map[i];
		if ((base != NULL) && (base->type == BASE_TYPE_DOMAIN_DEVICE)) {
			device = (domain_device *)base;
			if (IS_STP_OR_SATA(device) &&
				(device->base.root == root) &&
				(device->register_set == register_set)) {
				return device;
			}
		}
	}

	return NULL;
}
#endif
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
MV_U16 get_available_dev_id_new(lib_device_mgr *lib_dev)
{
	MV_U16 i;
	for (i=PORT_NUMBER; i<MAX_ID; i++) {
		if (lib_dev->device_map[i] == NULL){
			return i;
			}
	}
	return MAX_ID;
}
#endif
MV_U16 get_available_dev_id(lib_device_mgr *lib_dev)
{
	MV_I16 i;

#if (defined(FIX_SCSI_ID_WITH_PHY_ID) || defined(FIXED_DEVICE_ID_6440))
	for (i=MAX_ID-1; i>=0; i--) {
#else
	for (i=0; i<MAX_ID; i++) {
#endif
		if (lib_dev->device_map[i] == NULL){
			return (MV_U16)i;
			}
	}
	return MAX_ID;
}

MV_U16
get_avail_non_storage_dev_id(lib_device_mgr *lib_dev)
{
	MV_U16 i;

	for (i=MV_MAX_HD_DEVICE_ID; i<MAX_ID; i++) {
		if (lib_dev->device_map[i] == NULL)
			return i;
	}
	return MAX_ID;
}

MV_VOID release_available_dev_id(lib_device_mgr *lib_dev, MV_U16 id)
{
	MV_ASSERT(id < MAX_ID);
	lib_dev->device_map[id] = NULL;
}

MV_BOOLEAN change_device_map(lib_device_mgr *lib_dev, domain_base *base,
	MV_U16 old_id, MV_U16 new_id)
{
	MV_ASSERT(old_id < MAX_ID);
	MV_ASSERT(new_id < MAX_ID);
	MV_ASSERT(lib_dev->device_map[old_id] == base);

	if (lib_dev->device_map[new_id] != NULL)
		return MV_FALSE;

	lib_dev->device_map[old_id] = NULL;
	lib_dev->device_map[new_id] = base;
	return MV_TRUE;
}

MV_U16 add_device_map(lib_device_mgr *lib_dev, domain_base *base)
{
	MV_U16 id = get_available_dev_id(lib_dev);
	if (id >= MAX_ID) {
		CORE_DPRINT(("out of device ids.\n"));
		return MAX_ID;
	}
	lib_dev->device_map[id] = base;
	return id;
}

MV_VOID remove_device_map(lib_device_mgr *lib_dev, MV_U16 id)
{
	release_available_dev_id(lib_dev, id);
}

MV_VOID set_up_new_base(pl_root *root, domain_port *port,
	domain_base *base,
	command_handler *handler, enum base_type type, MV_U16 size)
{
	base->port = port;
	base->handler = handler;
	base->queue_depth = 1; /* will be update later */
	base->root = root;
	base->type = type;
#ifdef SUPPORT_MUL_LUN
	base->LUN = 0;
	base->TargetID = 0xFFFF;
	base->multi_lun = MV_FALSE;
#endif
	base->struct_size = size;
	base->outstanding_req = 0;
	base->parent = &port->base; /* by default set to port */
        base->blocked = MV_FALSE;
	base->cmd_issue_stopped = MV_FALSE;

	/* error handling */
	MV_LIST_HEAD_INIT(&base->err_ctx.sent_req_list);
	MV_DASSERT(base->err_ctx.eh_state == 0);
	base->err_ctx.eh_type = EH_TYPE_NONE;
	base->err_ctx.eh_state = EH_STATE_NONE;
	base->err_ctx.error_req = NULL;
	base->err_ctx.state = BASE_STATE_NONE;
	base->err_ctx.pm_eh_error_port = 0xff;
	base->err_ctx.pm_eh_active_port = 0xff;
	base->err_ctx.timeout_count = 0;
	base->err_ctx.timer_id = NO_CURRENT_TIMER;
	base->err_ctx.eh_timer = NO_CURRENT_TIMER;
	base->err_ctx.error_count = 0;
#ifdef ATHENA_MISS_MSIX_INT_WA
	base->msix_timer_id=NO_CURRENT_TIMER;
#endif
	core_set_hw_queue_of_base(root->core, base);
}

MV_VOID set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device,
	command_handler *handler)
{
	set_up_new_base(root, port, &device->base,
		handler,
		BASE_TYPE_DOMAIN_DEVICE,
		sizeof(domain_device));
    if (test_enabled(root->core))
	    MV_LIST_HEAD_INIT(&device->injected_error_list);

	device->status = DEVICE_STATUS_FUNCTIONAL;
	device->register_set = NO_REGISTER_SET;
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
	device->curr_phy_map = 0;
#endif
}

MV_VOID set_up_new_pm(pl_root *root, domain_port *port,
	domain_pm *pm)
{
	set_up_new_base(root, port, &pm->base,
		(command_handler *)core_get_handler(root, HANDLER_PM),
                BASE_TYPE_DOMAIN_PM,
                sizeof(domain_pm));
	pm->status = PM_STATUS_EXISTING | PM_STATUS_FUNCTIONAL;
	pm->state = PM_STATE_RESET_DONE;
	pm->register_set = NO_REGISTER_SET;
	pm->sata_sig_timer = NO_CURRENT_TIMER;
	
	set_up_new_base(root, port, &port->base,
		(command_handler *)core_get_handler(root, HANDLER_SATA_PORT),
		BASE_TYPE_DOMAIN_PORT,
		sizeof(domain_port));
	port->pm = pm;
	port->sata_sig_timer = NO_CURRENT_TIMER;
}

MV_VOID set_up_new_expander(pl_root *root, domain_port *port,
	domain_expander *expander)
{
	set_up_new_base(root, port, &expander->base,
		(command_handler *)core_get_handler(root, HANDLER_SMP),
		BASE_TYPE_DOMAIN_EXPANDER,
		sizeof(domain_expander));

	expander->state = EXP_STATE_REPORT_GENERAL;
	expander->status = EXP_STATUS_EXISTING | EXP_STATUS_FUNCTIONAL;
	expander->register_set = NO_REGISTER_SET;
	MV_LIST_HEAD_INIT(&expander->device_list);
	MV_LIST_HEAD_INIT(&expander->expander_list);
	expander->device_count = 0;
	expander->expander_count = 0;
        expander->has_been_setdown = MV_FALSE;
       /*First not reset each sata phy, we have phy reset EH for STP disks errors: 
         *1. Verified with local cascading PMC expander, will hang during initialization.
         *2. which increased the initialization time seriously. */
	expander->has_been_reset = MV_TRUE;
	expander->timer_tag = NO_CURRENT_TIMER;
	MV_ZeroMemory(expander->route_table, sizeof(MV_U16)*MAXIMUM_EXPANDER_PHYS);
}
MV_VOID set_up_new_enclosure(pl_root *root, domain_port *port,
	domain_enclosure *enclosure,
	command_handler *handler)
{
	set_up_new_base(root, port, &enclosure->base,
		handler,
		BASE_TYPE_DOMAIN_ENCLOSURE,
		sizeof(domain_enclosure));
	MV_LIST_HEAD_INIT(&enclosure->expander_list);
	enclosure->status = ENCLOSURE_STATUS_EXISTING | ENCLOSURE_STATUS_FUNCTIONAL;
	enclosure->enc_flag = ENC_FLAG_FIRST_INIT;
	enclosure->register_set = NO_REGISTER_SET;
}
