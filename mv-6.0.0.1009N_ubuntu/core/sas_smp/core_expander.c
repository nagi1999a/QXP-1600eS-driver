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
#include "core_util.h"
#include "core_device.h"
#include "core_sas.h"
#include "core_manager.h"
#include "core_expander.h"

#include "core_error.h"
#include "core_resource.h"

MV_VOID exp_found_new_device(smp_response *smp_resp, domain_expander *exp,
	MV_U64 sas_addr, MV_BOOLEAN is_stp, MV_BOOLEAN lsi_workaround);
MV_VOID exp_found_new_expander(smp_response *smp_resp, domain_expander *exp,
	MV_U64 sas_addr);
extern MV_VOID stp_req_report_phy_sata_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);

MV_VOID exp_bcst_unplug_dev(domain_expander *exp, domain_device *unplug_dev)
{
	pl_root *root = exp->base.root;
	pal_set_down_disk(root, unplug_dev, MV_TRUE);
}

MV_VOID exp_bcst_unplug_exp_phy(domain_expander *exp, domain_expander *unplug_exp,
	MV_U8 phy_id)
{
	pl_root *root = exp->base.root;
	MV_U8 i;
    #ifdef EXP_ABORT_REQ_DURING_BROADCAST
	domain_port *port = exp->base.port;
	domain_expander *tmp_exp = NULL;
	#endif			

	if (unplug_exp->parent_phy_count > 1) {
		/* is wideport */
		for (i=0; i<MAX_WIDEPORT_PHYS; i++) {
			if (unplug_exp->parent_phy_id[i] == phy_id) {
				/*
				 * parent_phy_id is arranged such that valid ids are
				 * located in indices 0 - parent_phy_count-1. so when we
				 * want to delete the current phy_id, we swap the last
				 * element to the current location and decrement count
				 */
				unplug_exp->parent_phy_id[i] =
					unplug_exp->parent_phy_id[unplug_exp->parent_phy_count-1];
				unplug_exp->parent_phy_count--;
				break;
			}
		}
	#ifdef EXP_ABORT_REQ_DURING_BROADCAST
		pal_abort_port_running_req(root, exp->base.port);

		/* can improve, not abort all the requests on that port */      
		LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
			domain_expander, base.queue_pointer) {
			pal_clean_expander_outstanding_req(root, tmp_exp);
		}
	#endif			
	} else {
		pal_set_down_expander(root, unplug_exp);
	}
}
void get_new_and_update_base_id(pl_root *root,domain_base *base){
#if (defined(FIX_SCSI_ID_WITH_PHY_ID) || defined(FIXED_DEVICE_ID_6440))			
	MV_U16 new_id=0xffff;
	core_extension * core = (core_extension *)root->core;
	if(core->device_id==DEVICE_ID_6440 || core->device_id==DEVICE_ID_6340 ||
		core->device_id==DEVICE_ID_6480|| core->device_id==DEVICE_ID_6485
			||IS_VANIR(core) || IS_ATHENA_CORE(core)){
			if(core->device_id == DEVICE_ID_ATHENA_1475){
				root->lib_rsrc->lib_dev->device_map[base->id]=NULL;
				base->id=(MV_U16)(base->port->phy->id+root->base_phy_num);
				if(root->lib_rsrc->lib_dev->device_map[base->id])
					base->id += 100;

				root->lib_rsrc->lib_dev->device_map[base->id]=base;
			}
			else{
				new_id=get_available_dev_id_new(&core->lib_dev);
				root->lib_rsrc->lib_dev->device_map[base->id]=NULL;
				base->id=new_id;
				root->lib_rsrc->lib_dev->device_map[new_id]=base;
			}
		}
#endif
#ifdef SUPPORT_MUL_LUN
	if (base->id != VIRTUAL_DEVICE_ID)		
		base->TargetID = add_target_map(root->lib_dev->target_id_map,base->id, MV_MAX_TARGET_NUMBER);
#endif
	core_set_hw_queue_of_base(root->core, base);
}
MV_VOID exp_found_new_device(smp_response *smp_resp, domain_expander *exp,
	MV_U64 sas_addr, MV_BOOLEAN is_stp, MV_BOOLEAN lsi_workaround)
{
	pl_root *root = exp->base.root;
	domain_device *device = get_device_obj(root, root->lib_rsrc);

	if (!device) {
		CORE_DPRINT(("ran out of device. abort initialization\n"));
		return;
	}

	if (is_stp) {
		set_up_new_device(root, exp->base.port, device,
			(command_handler *)
			core_get_handler(root, HANDLER_STP));

		if (lsi_workaround)
			device->status |= DEVICE_STATUS_NEED_RESET;
		
		device->state = DEVICE_STATE_STP_REPORT_PHY;
		device->connection = DC_SCSI | DC_SERIAL | DC_ATA;
		device->dev_type = DT_DIRECT_ACCESS_BLOCK;
	} else {
		set_up_new_device(root, exp->base.port, device,
			(command_handler *)
			core_get_handler(root, HANDLER_SSP));
#ifdef SKIP_INTERNAL_INITIALIZE
		device->state = DEVICE_STATE_SKIP_INIT_DONE;
#else
		device->state = DEVICE_STATE_RESET_DONE;
#endif
		device->connection = DC_SCSI | DC_SERIAL;
		device->dev_type = DT_DIRECT_ACCESS_BLOCK;
		device->base.queue_depth = CORE_SAS_DISK_QUEUE_DEPTH;
	}
	get_new_and_update_base_id(root,&device->base);

	device->sas_addr = sas_addr;
	device->parent_phy_id = smp_resp->response.Discover.PhyIdentifier;
	device->phy_change_count = smp_resp->response.Discover.PhyChangeCount;
	device->negotiated_link_rate =
		MV_MIN(smp_resp->response.Discover.NegotiatedPhysicalLinkRate,
		exp->neg_link_rate);
	/* max linkrate can not be higher than the current path to the expander */
	device->capability |= DEVICE_CAPABILITY_RATE_1_5G;
	if (device->negotiated_link_rate >= SAS_LINK_RATE_3_0_GBPS)
		device->capability |= DEVICE_CAPABILITY_RATE_3G;
	if (device->negotiated_link_rate >= SAS_LINK_RATE_6_0_GBPS)
		device->capability |= DEVICE_CAPABILITY_RATE_6G;
	if (device->negotiated_link_rate >= SAS_LINK_RATE_12_0_GBPS)
		device->capability |= DEVICE_CAPABILITY_RATE_12G;
        CORE_DPRINT(("dev%d type(0x%x), negotiated_link_rate(0x%x) capability(0x%x)\n",device->base.id,device->base.type,device->negotiated_link_rate,device->capability));

	device->base.parent = &exp->base;
	exp->base.port->device_count++;
	List_AddTail(&device->base.queue_pointer, &exp->base.port->device_list);
	exp->device_count++;
	List_AddTail(&device->base.exp_queue_pointer, &exp->device_list);
#ifdef SUPPORT_SES
	if(exp->enclosure){
		exp->enclosure->enc_flag |= ENC_FLAG_NEED_REINIT;	/*expander find new PD, assiciated enclosure need reinit*/
	}
#endif
	CORE_DPRINT(("add expander device list, device sas address %016llx, expander device count:0x%x\n",
		device->sas_addr.value, exp->device_count));
}

domain_expander *exp_check_existing_expander(domain_port *port,
	MV_U64 sas_addr)
{
	domain_expander *tmp_exp;

	/*
	 * assumption: any existing expander will be on the same port.
	 * even under multipath we should have two copies of domain_expanders
	 * for the same physical enclosure
	 */
	LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list, domain_expander,
		base.queue_pointer) {
		if (U64_COMPARE_U64(sas_addr, tmp_exp->sas_addr) == 0)
			return tmp_exp;
	}

	LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->current_tier, domain_expander,
		base.queue_pointer) {
		if (U64_COMPARE_U64(sas_addr, tmp_exp->sas_addr) == 0)
			return tmp_exp;
	}

	LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->next_tier, domain_expander,
		base.queue_pointer) {
		if (U64_COMPARE_U64(sas_addr, tmp_exp->sas_addr) == 0)
			return tmp_exp;
	}
	return MV_FALSE;
}

MV_VOID exp_found_new_expander(smp_response *smp_resp, domain_expander *exp,
	MV_U64 sas_addr)
{
	domain_expander *new_exp, *exist_exp = NULL;
	domain_port *port = exp->base.port;
	pl_root *root = exp->base.root;
	core_extension *core = (core_extension *)root->core;

	exist_exp = exp_check_existing_expander(port, sas_addr);
	if (exist_exp) {
		/*
		 * another expander with the same sas address already exists,
		 * we found a wideport
		 */
		MV_DASSERT(exist_exp->base.parent == &exp->base);
		exist_exp->parent_phy_id[exist_exp->parent_phy_count] =
			smp_resp->response.Discover.PhyIdentifier;
		exist_exp->parent_phy_count++;
		return;
	}

	new_exp = get_expander_obj(root, root->lib_rsrc);
	if (new_exp == NULL) {
		CORE_DPRINT(("Ran out of expanders. Aborting.\n"));
		return;
	}

	set_up_new_expander(root, port, new_exp);
	new_exp->base.parent = &exp->base;

	new_exp->sas_addr = sas_addr;
	new_exp->neg_link_rate =
		MV_MIN(smp_resp->response.Discover.NegotiatedPhysicalLinkRate,
		exp->neg_link_rate);
	new_exp->parent_phy_id[0] = smp_resp->response.Discover.PhyIdentifier;
	new_exp->parent_phy_count = 1;
	if (core->state == CORE_STATE_STARTED) {
		new_exp->need_report_plugin = MV_TRUE;
	}
	new_exp->has_been_setdown = MV_FALSE;

	/* do not add to port's expander lists yet. will do so after
	 * initialization is over */
	List_AddTail(&new_exp->base.queue_pointer, &port->next_tier);
	List_AddTail(&new_exp->base.exp_queue_pointer, &exp->expander_list);
	exp->expander_count++;
}

domain_base *exp_search_phy(domain_expander *exp, MV_U8 phy_id)
{
	domain_device *tmp_dev;
	domain_expander *tmp_exp;
	MV_U8 i;

	LIST_FOR_EACH_ENTRY_TYPE(tmp_dev, &exp->device_list, domain_device,
		base.exp_queue_pointer) {
		if (tmp_dev->parent_phy_id == phy_id) {
			return (domain_base *)tmp_dev;
		}
	}
	LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &exp->expander_list,
		domain_expander, base.exp_queue_pointer) {
		for (i = 0; i < tmp_exp->parent_phy_count; i++) {
			if (tmp_exp->parent_phy_id[i] == phy_id) {
				return (domain_base *)tmp_exp;
			}
		}
	}
	if (exp->enclosure) {
		if (exp->enclosure->parent_phy_id == phy_id)
			return (domain_base *)exp->enclosure;
	}

	return NULL;
}

MV_VOID smp_process_discover(smp_response *smp_resp, domain_expander *exp, MV_Request *req)
{
	domain_base *base;
	domain_expander *tmp_exp;
	MV_U64 sas_addr, parent_sas_addr;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_U8 phy_id = (MV_U8)(ctx->u.org.other);

	base = exp_search_phy(exp, phy_id);

#ifdef DEBUG_EXPANDER
	CORE_DPRINT(("phy :%d result:%x, attachtype:%x, TargetBit:%x, SASAddr:%016llx\n", 
	smp_resp->response.Discover.PhyIdentifier, 
	smp_resp->function_result,
	smp_resp->response.Discover.AttachedDeviceType,
	smp_resp->response.Discover.TargetBits, 
	(MV_U64)(*(MV_PU64)smp_resp->response.Discover.AttachedSASAddress)
	));
#endif
	if ((smp_resp->function_result == SMP_FUNCTION_ACCEPTED)) {
		MV_ASSERT(phy_id == smp_resp->response.Discover.PhyIdentifier);

		MV_CopyMemory(&sas_addr,
			smp_resp->response.Discover.AttachedSASAddress, 8);
		U64_ASSIGN(sas_addr, MV_CPU_TO_BE64(sas_addr));

		if ((smp_resp->response.Discover.AttachedDeviceType != NO_DEVICE) 
			/* below options are for LSI expander workaround */
			#ifdef LSI_EXP_NO_DEVICE_WORKAROUND
			||
			((smp_resp->response.Discover.AttachedDeviceType == NO_DEVICE) &&
			(smp_resp->response.Discover.TargetBits & FRAME_ATT_DEV_SATA_DEV))
			#endif
			) {
			if (base) {
				/*
				 * if found existing expander, check if it is already in
				 *     next_tier. Move to next_tier list if it is not.
				 * if found existing devices, check if phy change count is
				 *     different. If so, unplug and re-initialize.
				*/
				if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
					LIST_FOR_EACH_ENTRY_TYPE(tmp_exp,
						&exp->base.port->next_tier, domain_expander,
						base.queue_pointer) {
						if (&tmp_exp->base == base) {
							return;
						}
					}
					List_Del(&base->queue_pointer);
					base->port->expander_count--;
					((domain_expander *)base)->state = EXP_STATE_DISCOVER;
					List_AddTail(&base->queue_pointer, &base->port->next_tier);
				} else if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
					if (U64_COMPARE_U64(sas_addr, 
						((domain_device *)base)->sas_addr) != 0) {
						exp_bcst_unplug_dev(exp, (domain_device *)base);
						goto found_new;
					}
				}
				return;
			}
			
found_new:
			if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
				U64_ASSIGN_U64(parent_sas_addr,
					((domain_expander *)exp->base.parent)->sas_addr);
			} else {
				U64_ASSIGN_U64(parent_sas_addr, exp->sas_addr);
			}
			if (U64_COMPARE_U64(parent_sas_addr, sas_addr) != 0) {
				if (smp_resp->response.Discover.TargetBits &
					FRAME_ATT_DEV_TYPE_SSP) {
					CORE_PRINT(("ssp: phy :%d result:%x, attachtype:%x, TargetBit:%x, SASAddr:%016llx\n", 
					smp_resp->response.Discover.PhyIdentifier, 
					smp_resp->function_result,
					smp_resp->response.Discover.AttachedDeviceType,
					smp_resp->response.Discover.TargetBits, 
					sas_addr
					));

					exp_found_new_device(smp_resp, exp, sas_addr, MV_FALSE,
						MV_FALSE);
				} else if (smp_resp->response.Discover.TargetBits &
					(FRAME_ATT_DEV_TYPE_STP | FRAME_ATT_DEV_SATA_DEV)) {
					CORE_PRINT(("stp: phy :%d result:%x, attachtype:%x, TargetBit:%x, SASAddr:%016llx\n", 
					smp_resp->response.Discover.PhyIdentifier, 
					smp_resp->function_result,
					smp_resp->response.Discover.AttachedDeviceType,
					smp_resp->response.Discover.TargetBits, 
					sas_addr
					));
					if (smp_resp->response.Discover.AttachedDeviceType ==
						NO_DEVICE) {
						#ifdef LSI_EXPANDER_NO_DEV_WORKAROUND
						/* LSI workaround */
						exp_found_new_device(smp_resp, exp, sas_addr, MV_TRUE,
							MV_TRUE);
						#endif
					} else {
						exp_found_new_device(smp_resp, exp, sas_addr, MV_TRUE,
							MV_FALSE);
					}
				} else if (smp_resp->response.Discover.TargetBits &
					FRAME_ATT_DEV_TYPE_SMP) {
					exp_found_new_expander(smp_resp, exp, sas_addr);
				}
			}
		} else {
			if (base) {
				if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
					exp_bcst_unplug_dev(exp, (domain_device *)base);
				} else if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
					exp_bcst_unplug_exp_phy(exp, (domain_expander *)base, phy_id);
				}
			}
		}
	} else { //if ((smp_resp->function_result == SMP_FUNCTION_ACCEPTED))
		/* if function status is not SMP_FUNCTION_ACCEPTED, disk may gone.
		* expander may return SMP_PHY_VACANT */
		if (base) {
			if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
				exp_bcst_unplug_dev(exp, (domain_device *)base);
			} else if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
				exp_bcst_unplug_exp_phy(exp, (domain_expander *)base, phy_id);
			}
		}
	}
}

MV_VOID smp_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)get_device_by_id(
		root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	domain_expander *parent;
	smp_response *smp_resp;
	core_context *ctx;

	/*
	 * sometimes when processing hot plug, discovery req will fail with
	 * no_dest error. then init error handling will set down expander
	 * and complete all requests with REQ_STATUS_NO_DEVICE. after this
	 * the expander is no longer valid.
	 */
	/* May no longer be true. Put assert for now */
	if (!exp)
		MV_ASSERT(MV_FALSE);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, &exp->base, req);
		return;
	}

	smp_resp = (smp_response *)core_map_data_buffer(req);
	switch (exp->state) {
	case EXP_STATE_REPORT_GENERAL:
	if (smp_resp->function_result != SMP_FUNCTION_ACCEPTED) {
		// cannot continue
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		core_handle_init_error(root, &exp->base, req);
		return;
	}

		exp->phy_count = smp_resp->response.ReportGeneral.NumberOfPhys;
		exp->configurable_route_table =
			smp_resp->response.ReportGeneral.ConfigurableRouteTable;
		exp->configures_others =
			smp_resp->response.ReportGeneral.ConfiguresOthers;
		if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
			parent = (domain_expander *)exp->base.parent;
			if (parent->configures_others) {
				exp->configures_others = 1;
			}
		}
#ifdef DEBUG_EXPANDER
		{
		MV_U16 ExpanderChangeCount = 
			MV_BE16_TO_CPU((MV_U16)*(MV_U16 *)&smp_resp->response.ReportGeneral.ExpanderChangeCount[0]);
		MV_U16 ExpanderRouteIndexes= 
			MV_BE16_TO_CPU(*(MV_U16 *)&smp_resp->response.ReportGeneral.ExpanderRouteIndexes[0]);
		CORE_DPRINT(("exp %p ExpanderChangeCount %d.\n", exp, ExpanderChangeCount));		
		CORE_DPRINT(("exp %p ExpanderRouteIndexes %d.\n", exp, ExpanderRouteIndexes));		
		CORE_DPRINT(("exp %p NumberOfPhys %d.\n", exp, smp_resp->response.ReportGeneral.NumberOfPhys));		
		CORE_DPRINT(("exp %p ConfigurableRouteTable %x.\n", exp, smp_resp->response.ReportGeneral.ConfigurableRouteTable));		
		CORE_DPRINT(("exp %p Configuring %x.\n", exp, smp_resp->response.ReportGeneral.Configuring));	
		CORE_DPRINT(("exp %p ConfiguresOthers %x.\n", exp, smp_resp->response.ReportGeneral.ConfiguresOthers));	
		CORE_DPRINT(("exp %p TableToTableSupported %x.\n", exp, smp_resp->response.ReportGeneral.TableToTableSupported));	
		}
#endif
		exp->state = EXP_STATE_REPORT_MANU_INFO;
		break;
	case EXP_STATE_REPORT_MANU_INFO:
		exp->component_rev_id = smp_resp->response.
			ReportManufacturerInformation.ComponentRevisionID;
		MV_CopyMemory(&exp->component_id, smp_resp->response.
			ReportManufacturerInformation.ComponentID, 2);
		MV_CopyMemory(&exp->component_vendor_id, smp_resp->response.
			ReportManufacturerInformation.ComponentVendorIdentification,
			8);
		MV_CopyMemory(&exp->vendor_id, smp_resp->response.
			ReportManufacturerInformation.VendorIdentification, 8);
		MV_CopyMemory(&exp->product_id, smp_resp->response.
			ReportManufacturerInformation.ProductIdentification, 16);
		MV_CopyMemory(&exp->product_rev, smp_resp->response.
			ReportManufacturerInformation.ProductRevisionLevel, 4);
		exp->state = EXP_STATE_DISCOVER;
		break;
	case EXP_STATE_DISCOVER:
		if (exp->has_been_reset == MV_TRUE) {
			if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
				exp->state = EXP_STATE_CONFIG_ROUTE;
			}
			else {
				exp->state = EXP_STATE_NEXT_TIER;
			}
		}
		else {
			exp->state = EXP_STATE_RESET_SATA_PHY;
		}
		break;
	case EXP_STATE_RESET_SATA_PHY:
		exp->has_been_reset = MV_TRUE;
		exp->state = EXP_STATE_RESET_WAIT;
		break;

	case EXP_STATE_CLEAR_AFFILIATION:
		ctx = req->Context[MODULE_CORE];
		MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_CLEAR_AFFILIATION);
		if (ctx->u.smp_clear_aff.need_wait == MV_FALSE) {
			if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
				exp->state = EXP_STATE_CONFIG_ROUTE;
			}
			else {
				exp->state = EXP_STATE_NEXT_TIER;
			}
		} else {
			exp->state = EXP_STATE_RESET_WAIT_2;
		}
		break;

	case EXP_STATE_CONFIG_ROUTE:
		exp->state = EXP_STATE_NEXT_TIER;
		break;
	}

	core_unmap_data_buffer(req);
	core_queue_init_entry(root, &exp->base, MV_FALSE);
}

MV_VOID smp_req_discover_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)exp_p;
	smp_response *smp_resp;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_Request *vir_req = (MV_Request *)ctx->u.org.org_req;
	core_context *vir_ctx = vir_req->Context[MODULE_CORE];

	MV_ASSERT(vir_ctx->u.smp_discover.req_remaining > 0);
	vir_ctx->u.smp_discover.req_remaining--;

    if (exp) {
        if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
        	/* if success, needn't set REQ_STATUS_SUCCESS 
        	* by default, it's REQ_STATUS_SUCCESS
        	* and some physical requests for this virtual may failed */
        	smp_resp = (smp_response *)core_map_data_buffer(req);
        	smp_process_discover(smp_resp, exp, req);
        	core_unmap_data_buffer(req);
        }
    } else {
        return;
    }

	/* no matter this physical request success or not, wait for all finished */
	if (vir_ctx->u.smp_discover.req_remaining == 0) {
		if (vir_req->Scsi_Status == REQ_STATUS_SUCCESS) {
#ifdef EXP_DONT_DISCOVER_FINAL_DEV
			if (vir_ctx->u.smp_discover.current_phy_id < (exp->phy_count-1)) 
#else
			if (vir_ctx->u.smp_discover.current_phy_id < exp->phy_count) 
#endif
			{
				core_append_init_request(root, vir_req);
			} else {
				core_queue_completed_req(root->core, vir_req);
			}
		} else {
			core_queue_completed_req(root->core, vir_req);
		}
	}
}

MV_VOID smp_req_reset_sata_phy_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)exp_p;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_Request *vir_req = (MV_Request *)ctx->u.org.org_req;
	core_context *vir_ctx = vir_req->Context[MODULE_CORE];
	
	MV_ASSERT(vir_ctx->type == CORE_CONTEXT_TYPE_RESET_SATA_PHY);
	MV_ASSERT(vir_ctx->u.smp_reset_sata_phy.req_remaining > 0);
	vir_ctx->u.smp_reset_sata_phy.req_remaining--;

	/* no matter this physical request success or not, wait for all finished */
	if (vir_ctx->u.smp_reset_sata_phy.req_remaining == 0) {
		if (vir_req->Scsi_Status == REQ_STATUS_SUCCESS) {
			// device list should remain static throughout this process
			MV_ASSERT(vir_ctx->u.smp_reset_sata_phy.total_dev_count == 
				exp->device_count);
			if (vir_ctx->u.smp_reset_sata_phy.curr_dev_count < 
				vir_ctx->u.smp_reset_sata_phy.total_dev_count) {
				core_append_init_request(root, vir_req);
			} else {
				core_queue_completed_req(root->core, vir_req);
			}
		}
		else {
			core_queue_completed_req(root->core, vir_req);
		}
	}
}

MV_VOID smp_req_clear_aff_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)exp_p;
	smp_response *smp_resp;
	struct SMPResponseReportPhySATA *report;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_Request *vir_req = (MV_Request *)ctx->u.org.org_req;
	core_context *vir_ctx = vir_req->Context[MODULE_CORE];
	domain_base *base;

	MV_ASSERT(vir_ctx->u.smp_clear_aff.req_remaining > 0);
	vir_ctx->u.smp_clear_aff.req_remaining--;

	MV_ASSERT(vir_ctx->type == CORE_CONTEXT_TYPE_CLEAR_AFFILIATION);

	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		switch (vir_ctx->u.smp_clear_aff.state) {
		case CLEAR_AFF_STATE_REPORT_PHY_SATA:
			smp_resp = (smp_response *)core_map_data_buffer(req);
			report = &smp_resp->response.ReportPhySATA;

			base = exp_search_phy(exp, report->PhyIdentifier);
			if (base) {
				MV_ASSERT(base->type == BASE_TYPE_DOMAIN_DEVICE);
					if (report->AffiliationValid) {
						/* can choose option: hard reset or clear affliation commands
						* DEVICE_STATUS_NEED_RESET or DEVICE_STATUS_NEED_CLEAR
						* for now use reset */
						((domain_device *)base)->status |= DEVICE_STATUS_NEED_RESET;
					}
			} else {
				MV_ASSERT(MV_FALSE);
			}
			core_unmap_data_buffer(req);
			break;
		case CLEAR_AFF_STATE_CLEAR_AFF:
			/* do nothing */
			break;

		default:
			MV_ASSERT(MV_FALSE);
			break;
		}
	}

	/* no matter this physical request success or not, wait for all finished */
	if (vir_ctx->u.smp_clear_aff.req_remaining == 0) {
		if (vir_req->Scsi_Status == REQ_STATUS_SUCCESS) {
			// device list should remain static throughout this process
			MV_ASSERT(vir_ctx->u.smp_clear_aff.total_dev_count ==
				exp->device_count);
			if (vir_ctx->u.smp_clear_aff.curr_dev_count < 
				vir_ctx->u.smp_clear_aff.total_dev_count) {
				core_append_init_request(root, vir_req);
			} else if (vir_ctx->u.smp_clear_aff.state == 
				CLEAR_AFF_STATE_REPORT_PHY_SATA) {
				vir_ctx->u.smp_clear_aff.curr_dev_count = 0;
				vir_ctx->u.smp_clear_aff.state = CLEAR_AFF_STATE_CLEAR_AFF;
				core_append_init_request(root, vir_req);
			}
			else {
				core_queue_completed_req(root->core, vir_req);
			}
		}
		else {
			core_queue_completed_req(root->core, vir_req);
		}
	}
}

domain_expander *exp_find_config_route_ancestor(pl_root *root,
	domain_expander *exp)
{
	domain_expander *parent;

	while (exp->base.parent && (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER)) {
		parent = (domain_expander *)exp->base.parent;
		if (parent->configurable_route_table &&
			!parent->configures_others) {
			/*
			 * return the expander just before the parent to be configured.
			 * this is because we need the phy id information and this is
			 * stored in exp and not parent
			 */
			return exp;
		}
		exp = parent;
	}

	return NULL;
}

MV_VOID smp_req_config_route_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)exp_p;
	domain_expander *ancestor, *org_exp;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_Request *vir_req = (MV_Request *)ctx->u.org.org_req;
	core_context *vir_ctx = vir_req->Context[MODULE_CORE];
	smp_virtual_config_route_buffer *buf;
	smp_response *smp_resp;
	MV_U8 i;

	if (!exp) {
		/* expander is gone, abort */
		MV_ASSERT(MV_FALSE);
		vir_ctx->u.smp_config_route.req_remaining--;
		if (vir_ctx->u.smp_config_route.req_remaining == 0) {
			org_exp = (domain_expander *)vir_ctx->u.smp_config_route.org_exp;
			vir_req->Device_Id = org_exp->base.id;
			vir_req->Scsi_Status = REQ_STATUS_ERROR;
			core_queue_completed_req(root->core, vir_req);
		}
		return;
	}

	smp_resp = (smp_response *)core_map_data_buffer(req);
	if (smp_resp->function_result != SMP_FUNCTION_ACCEPTED) {
		req->Scsi_Status = REQ_STATUS_ERROR;
	}
	core_unmap_data_buffer(req);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, &exp->base, req);
		return;
	}

	vir_ctx->u.smp_config_route.req_remaining--;
	if (vir_ctx->u.smp_config_route.req_remaining == 0) {
		if ((vir_ctx->u.smp_config_route.current_phy <
			vir_ctx->u.smp_config_route.phy_count) ||
			(vir_ctx->u.smp_config_route.current_addr <
			vir_ctx->u.smp_config_route.address_count)) {

			core_append_init_request(root, vir_req);
		} else {
			/*
			* config route for this expander is finished.
			* check if parent needs to be configured also.
			*/
			ancestor = exp_find_config_route_ancestor(root, exp);
			if (ancestor) {
				MV_ASSERT(ancestor->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);
				/* update phy IDs */
				buf = (smp_virtual_config_route_buffer *)
				core_map_data_buffer(vir_req);
				for (i=0; i<ancestor->parent_phy_count; i++)
					buf->phy_id[i] = ancestor->parent_phy_id[i];
				core_unmap_data_buffer(vir_req);
				vir_ctx->u.smp_config_route.current_addr = 0;
				vir_ctx->u.smp_config_route.current_phy = 0;
				MV_ASSERT(vir_ctx->u.smp_config_route.org_exp);
				vir_ctx->u.smp_config_route.phy_count =
					ancestor->parent_phy_count;
				vir_req->Device_Id = ancestor->base.parent->id;
				core_append_init_request(root, vir_req);
				return;
			}
			/* reached port, we are done */
			org_exp = (domain_expander *)vir_ctx->u.smp_config_route.org_exp;
			vir_req->Device_Id = org_exp->base.id;
			vir_req->Scsi_Status = REQ_STATUS_SUCCESS;
			core_queue_completed_req(root->core, vir_req);
		}
	}
}

MV_VOID smp_req_fill_common_fields(MV_Request *req, domain_expander *exp,
	MV_PVOID callback)
{
	smp_request *smp_req;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP;
	req->Device_Id = exp->base.id;
	req->Cmd_Initiator = exp->base.root;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
#ifdef SCSI_ID_MAP
    req->pass_id = 1;
#endif
	/* Need to clear out these bytes */
	smp_req = core_map_data_buffer(req);
	smp_req->resp_len = 0;
	smp_req->req_len = 0;
	core_unmap_data_buffer(req);
}

MV_VOID smp_physical_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)get_device_by_id(
		root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	core_context *ctx = req->Context[MODULE_CORE];
	MV_Request *vir_req = (MV_Request *)ctx->u.org.org_req;
	core_context *vir_ctx = vir_req->Context[MODULE_CORE];

	if (vir_req->Scsi_Status == REQ_STATUS_PENDING) {
		vir_req->Scsi_Status = REQ_STATUS_SUCCESS;
	}

	if (!exp) {
		/* expander is gone, abort */
		MV_ASSERT(MV_FALSE);
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
	}

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		vir_req->Scsi_Status = req->Scsi_Status;
		if (ctx->error_info) {
			vir_ctx->error_info |= ctx->error_info;
		}
	}

	switch (vir_req->Cdb[2]) {
		case CDB_CORE_SMP_VIRTUAL_CLEAR_AFFILIATION_ALL:
			smp_req_clear_aff_callback(root_p, req, exp);
			break;
		case CDB_CORE_SMP_VIRTUAL_RESET_SATA_PHY:
			smp_req_reset_sata_phy_callback(root, req, exp);
			break;
		case CDB_CORE_SMP_VIRTUAL_CONFIG_ROUTE:
			smp_req_config_route_callback(root, req, exp);
			break;
		case CDB_CORE_SMP_VIRTUAL_DISCOVER:
			smp_req_discover_callback(root, req, exp);
			break;
		case CDB_CORE_STP_VIRTUAL_REPORT_SATA_PHY:
			stp_req_report_phy_sata_callback(root, req, exp);
			break;
		case CDB_CORE_STP_VIRTUAL_PHY_RESET:
		case CDB_CORE_SSP_VIRTUAL_PHY_RESET:
			core_queue_completed_req(root->core, vir_req);
			break;
		default:
			// something is wrong;
			MV_ASSERT(MV_FALSE);
			break;
	}

}

MV_Request *smp_make_report_general_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL)
		return NULL;
	smp_req_fill_common_fields(req, exp, callback);
	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = REPORT_GENERAL;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_report_manu_info_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL) {
		return NULL;
	}
	smp_req_fill_common_fields(req, exp, callback);

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = REPORT_MANUFACTURER_INFORMATION;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;

	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_discover_req(domain_expander *exp, MV_Request *vir_req,
	MV_U8 phy_id, MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;
	core_context *ctx;

	if (req == NULL)
		return NULL;
	smp_req_fill_common_fields(req, exp, callback);
	ctx = req->Context[MODULE_CORE];
	ctx->u.org.org_req = vir_req;
	ctx->u.org.other = phy_id;

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = DISCOVER;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.Discover.IgnoredByte4_7[0] = 0; /* byte 4-7 */
	smp_req->request.Discover.IgnoredByte4_7[1] = 0;
	smp_req->request.Discover.IgnoredByte4_7[2] = 0;
	smp_req->request.Discover.IgnoredByte4_7[3] = 0;
	smp_req->request.Discover.ReservedByte8 = 0; /* byte 8*/
	smp_req->request.Discover.PhyIdentifier = phy_id; /* byte 9*/
	smp_req->request.Discover.IgnoredByte10 = 0; /* byte 10-11 */
	smp_req->request.Discover.ReservedByte11 = 0; 
	/* smp_req->request.Discover.CRC byte 12-15 */

	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_config_route_req(domain_expander *exp,
	MV_Request *vir_req, MV_U16 route_index, MV_U8 phy_id, MV_U64 *sas_addr,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;
	core_context *ctx;

	if (req == NULL) {
		return NULL;
	}
	smp_req_fill_common_fields(req, exp, callback);
	ctx = req->Context[MODULE_CORE];
	ctx->u.org.org_req = vir_req;

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = CONFIGURE_ROUTE_INFORMATION;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.ConfigureRouteInformation.ExpanderRouteIndex[0] =
		(route_index & 0xFF00) >> 8;
	smp_req->request.ConfigureRouteInformation.ExpanderRouteIndex[1] =
		(route_index & 0xFF);
	smp_req->request.ConfigureRouteInformation.PhyIdentifier = phy_id;
	smp_req->request.ConfigureRouteInformation.DisableRouteEntry = 0;

	MV_CopyMemory(smp_req->request.ConfigureRouteInformation.RoutedSASAddress,
		sas_addr, 8);
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_phy_control_req(domain_device *device, MV_U8 operation,
	MV_PVOID callback)
{
	pl_root *root = device->base.root;
	domain_expander *exp = (domain_expander *)device->base.parent;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL) {
		return NULL;
	}
	smp_req_fill_common_fields(req, exp, callback);

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = PHY_CONTROL;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.PhyControl.PhyIdentifier = device->parent_phy_id;
	smp_req->request.PhyControl.PhyOperation = operation;
	smp_req->request.PhyControl.ProgrammedMaximumPhysicalLinkRate = 0;
	smp_req->request.PhyControl.ProgrammedMinimumPhysicalLinkRate = 0;
	smp_req->request.PhyControl.ExpectedExpanderChangeCount[0] = 0;
	smp_req->request.PhyControl.ExpectedExpanderChangeCount[1] = 0;
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_phy_crtl_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_U8 operation, MV_PVOID callback)
{
	domain_expander *exp = (domain_expander *)exp_p;
	pl_root *root = (pl_root *)exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL) return NULL;

	smp_req_fill_common_fields(req, exp, callback);

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = PHY_CONTROL;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.PhyControl.PhyIdentifier = phy_id;
	smp_req->request.PhyControl.PhyOperation = operation;
	smp_req->request.PhyControl.ProgrammedMaximumPhysicalLinkRate = 0;
	smp_req->request.PhyControl.ProgrammedMinimumPhysicalLinkRate = 0;
	smp_req->request.PhyControl.ExpectedExpanderChangeCount[0] = 0;
	smp_req->request.PhyControl.ExpectedExpanderChangeCount[1] = 0;
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_report_phy_sata_req(domain_device *device,
	MV_PVOID callback)
{
	pl_root *root = device->base.root;
	domain_expander *exp = (domain_expander *)device->base.parent;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL)
		return NULL;

	smp_req_fill_common_fields(req, exp, callback);

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = REPORT_PHY_SATA;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.ReportPhySATA.PhyIdentifier = device->parent_phy_id;
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_report_phy_sata_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_PVOID callback)
{
	domain_expander *exp = (domain_expander *)exp_p;
	pl_root *root = (pl_root *)exp->base.root;
	MV_Request *req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;

	if (req == NULL) return NULL;

	smp_req_fill_common_fields(req, exp, callback);

	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function = REPORT_PHY_SATA;
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	smp_req->request.ReportPhySATA.PhyIdentifier = phy_id;
	core_unmap_data_buffer(req);
	return req;
}

MV_Request *smp_make_virtual_discover_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);
	core_context *ctx;

	if (req == NULL) {
		return NULL;
	}

	ctx = req->Context[MODULE_CORE];
	ctx->u.smp_discover.current_phy_id = 0;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP_VIRTUAL_DISCOVER;
	req->Device_Id = exp->base.id;
	req->Cmd_Initiator = exp->base.root;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
	return req;
}

MV_Request *smp_make_virtual_reset_sata_phy_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req;
	core_context *ctx;

	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	ctx = req->Context[MODULE_CORE];
	ctx->type = CORE_CONTEXT_TYPE_RESET_SATA_PHY;
	ctx->u.smp_reset_sata_phy.curr_dev_count = 0;
        ctx->u.smp_reset_sata_phy.total_dev_count = exp->device_count;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP_VIRTUAL_RESET_SATA_PHY;
	req->Device_Id = exp->base.id;
	req->Cmd_Initiator = exp->base.root;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
	return req;
}

MV_Request *smp_make_vir_clear_aff_all_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	MV_Request *req;
	core_context *ctx;

	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	ctx = req->Context[MODULE_CORE];
	ctx->type = CORE_CONTEXT_TYPE_CLEAR_AFFILIATION;
	ctx->u.smp_clear_aff.state = CLEAR_AFF_STATE_REPORT_PHY_SATA;
	ctx->u.smp_clear_aff.curr_dev_count = 0;
	ctx->u.smp_clear_aff.total_dev_count = exp->device_count;
	ctx->u.smp_clear_aff.need_wait = MV_FALSE;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP_VIRTUAL_CLEAR_AFFILIATION_ALL;
	req->Device_Id = exp->base.id;
	req->Cmd_Initiator = exp->base.root;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
	return req;
}

MV_Request *smp_make_virtual_config_route_req(domain_expander *exp,
	MV_PVOID callback)
{
	pl_root *root = exp->base.root;
	domain_expander *tmp_exp, *ancestor;
	domain_device *tmp_dev;
	MV_Request *req = get_intl_req_resource(root, SMP_VIRTUAL_REQ_BUFFER_SIZE);
	smp_virtual_config_route_buffer *buf;
	core_context *ctx;
	MV_U64 val_64;
	MV_U8 addr_count = 0, phy_count, i;

	if (req == NULL)
		return NULL;

	ancestor = exp_find_config_route_ancestor(root, exp);
	if (ancestor == NULL){
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return req;
	}
	MV_ASSERT(ancestor->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	buf = (smp_virtual_config_route_buffer *)core_map_data_buffer(req);
	ctx = req->Context[MODULE_CORE];

	LIST_FOR_EACH_ENTRY_TYPE(tmp_dev, &exp->device_list, domain_device,
		base.exp_queue_pointer) {
		if (tmp_dev->status & DEVICE_STATUS_FUNCTIONAL) {
			U64_ASSIGN(val_64, MV_CPU_TO_BE64(tmp_dev->sas_addr));
			MV_CopyMemory(&buf->sas_addr[addr_count], &val_64, 8);
			addr_count++;
		}
	}
	if (exp->enclosure) {
		if(exp->enclosure->status & ENCLOSURE_STATUS_FUNCTIONAL){
			U64_ASSIGN(val_64, MV_CPU_TO_BE64(exp->enclosure->sas_addr));
			MV_CopyMemory(&buf->sas_addr[addr_count], &val_64, 8);
			addr_count++;
		}
	}
	LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &exp->expander_list, domain_expander,
		base.exp_queue_pointer) {
		if (tmp_exp->status & EXP_STATUS_FUNCTIONAL) {
			U64_ASSIGN(val_64, MV_CPU_TO_BE64(tmp_exp->sas_addr));
			MV_CopyMemory(&buf->sas_addr[addr_count], &val_64, 8);
			addr_count++;
		}
	}

	phy_count = ancestor->parent_phy_count;
	for (i=0; i<phy_count; i++)
		buf->phy_id[i] = ancestor->parent_phy_id[i];

	ctx->u.smp_config_route.current_addr = 0;
	ctx->u.smp_config_route.current_phy = 0;
	ctx->u.smp_config_route.address_count = addr_count;
	ctx->u.smp_config_route.phy_count = phy_count;
	ctx->u.smp_config_route.org_exp = exp;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP_VIRTUAL_CONFIG_ROUTE;
	req->Device_Id = ancestor->base.parent->id;
	req->Cmd_Initiator = root;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
	core_unmap_data_buffer(req);
	return req;
}

MV_VOID exp_init_reset_wait_callback(pl_root *root, domain_expander *exp)
{
	domain_base *base;
	MV_ULONG flags;
	core_extension *core = exp->base.root->core;

//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(exp->base.root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	base = get_device_by_id(root->lib_dev, exp->base.id, MV_FALSE, MV_FALSE);
	if ((base == NULL) || (base != &exp->base)) {
		CORE_PRINT(("Expander is gone.\n"));
//		core_global_unlock(exp->base.root->core, &flags);
//		spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
		return;
	}

	switch (exp->state) {
	case EXP_STATE_RESET_WAIT:
		exp->state = EXP_STATE_CLEAR_AFFILIATION;
		break;
	case EXP_STATE_RESET_WAIT_2:
		if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER)
			exp->state = EXP_STATE_CONFIG_ROUTE;
		else
			exp->state = EXP_STATE_NEXT_TIER;
		break;
	default:
		MV_ASSERT(MV_FALSE);
	}

	exp->timer_tag = NO_CURRENT_TIMER;
	core_queue_init_entry(root, &exp->base, MV_FALSE);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(exp->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}

MV_VOID exp_push_next_expander(pl_root *root, domain_port *port)
{
	domain_expander *exp;

	if (!List_Empty(&port->current_tier)) {
		exp = (domain_expander *)List_GetFirstEntry(&port->current_tier,
			domain_expander, base.queue_pointer);
		List_AddTail(&exp->base.queue_pointer, &port->expander_list);
		port->expander_count++;
		core_queue_init_entry(root, &exp->base, MV_TRUE);
	}
}

MV_BOOLEAN exp_state_machine(MV_PVOID expander)
{
	domain_expander *exp = (domain_expander *)expander, *tmp_exp;
	domain_device *tmp_device;
	domain_port *port = exp->base.port;
	pl_root *root = exp->base.root;
	MV_Request *req = NULL;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
#endif
	CORE_DPRINT(("exp %d state 0x%x.\n", exp->base.id, exp->state));

	switch (exp->state) {
	case EXP_STATE_REPORT_GENERAL:
		req = smp_make_report_general_req(exp, smp_req_callback);
		break;
	case EXP_STATE_REPORT_MANU_INFO:
		req = smp_make_report_manu_info_req(exp, smp_req_callback);
		break;
	case EXP_STATE_DISCOVER:
		req = smp_make_virtual_discover_req(exp, smp_req_callback);
		break;
	case EXP_STATE_RESET_SATA_PHY:
		req = smp_make_virtual_reset_sata_phy_req(exp, smp_req_callback);
		break;
	case EXP_STATE_RESET_WAIT:
		MV_ASSERT(exp->timer_tag == NO_CURRENT_TIMER);
		exp->timer_tag = core_add_timer(root->core, 2, 
			(void(*)(void *, void *))exp_init_reset_wait_callback, root, exp);
		MV_ASSERT(exp->timer_tag != NO_CURRENT_TIMER);
		return MV_TRUE;
	case EXP_STATE_CLEAR_AFFILIATION:
		req = smp_make_vir_clear_aff_all_req(exp, smp_req_callback);
		break;
	case EXP_STATE_RESET_WAIT_2:
		MV_ASSERT(exp->timer_tag == NO_CURRENT_TIMER);
		exp->timer_tag = core_add_timer(root->core, 2,
			(void(*)(void *, void *))exp_init_reset_wait_callback, root, exp);
		MV_ASSERT(exp->timer_tag != NO_CURRENT_TIMER);
		return MV_TRUE;
	case EXP_STATE_CONFIG_ROUTE:
		if (exp_find_config_route_ancestor(root, exp) != NULL) {
			req = smp_make_virtual_config_route_req(exp, smp_req_callback);
			break;
		}
		/* otherwise, don't need to config route, move on */
		/* no break */
	case EXP_STATE_NEXT_TIER:
		/*
		 * discovery for this expander is done. next step is to initialize any
		 * new devices and do discovery for new expanders
		 */
#ifndef SUPPORT_STAGGERED_SPIN_UP
		LIST_FOR_EACH_ENTRY_TYPE(tmp_device, &exp->device_list, domain_device,
			base.exp_queue_pointer) {
					if (tmp_device->state != DEVICE_STATE_INIT_DONE)
							core_queue_init_entry(root, &tmp_device->base, MV_TRUE);
					}
#else
		LIST_FOR_EACH_ENTRY_TYPE(tmp_device, &exp->device_list, domain_device,
			base.exp_queue_pointer) {
			
			if ((tmp_device->state != DEVICE_STATE_INIT_DONE) && 
				!(tmp_device->status&DEVICE_STATUS_SPIN_UP)){
				struct device_spin_up *tmp=NULL;				
				
				tmp=get_spin_up_device_buf(root->lib_rsrc);
				if(!tmp || (core->spin_up_group == 0)){
					if(tmp)
						free_spin_up_device_buf(root->lib_rsrc,tmp);
					core_queue_init_entry(root, &tmp_device->base, MV_TRUE);
				}else{
					MV_LIST_HEAD_INIT(&tmp->list);
					tmp->roots=root;
					tmp->base=&tmp_device->base;
					
					if(tmp_device->status & DEVICE_STATUS_SPIN_UP){
						free_spin_up_device_buf(root->lib_rsrc,tmp);
						continue;
					}
					
					OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
					tmp_device->status |=DEVICE_STATUS_SPIN_UP;
					List_AddTail(&tmp->list,&core->device_spin_up_list);
					if(core->device_spin_up_timer ==NO_CURRENT_TIMER){
						core->device_spin_up_timer=core_add_timer(core,3, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, &tmp_device->base, NULL);
					}
					OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
				}			
			}
		}
#endif
		if (List_Empty(&port->current_tier)) {
			while(!List_Empty(&port->next_tier)) {
				tmp_exp = (domain_expander *)List_GetFirstEntry(&port->next_tier,
					domain_expander, base.queue_pointer);
				List_AddTail(&tmp_exp->base.queue_pointer, &port->current_tier);
			}
		}
		exp->state = EXP_STATE_DONE;
		/* no break */
	case EXP_STATE_DONE:
		exp_push_next_expander(root, port);
		core_init_entry_done(root, port, &exp->base);
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

void exp_update_direct_attached_phys(domain_port *port)
{
	pl_root *root = port->base.root;
	domain_expander *exp = NULL;
	MV_U8 phy_id = 0, i;

	if (List_Empty(&port->expander_list))
		return;

	/*
	 * find the expander that was attached to the port, and update
	 * phy information
	 */
	LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list, domain_expander,
		base.queue_pointer) {
		if (exp->base.parent == &port->base)
			break;
	}
	MV_DASSERT(exp != NULL);
	exp->parent_phy_count = 0;
	for (i=0; i<root->phy_num; i++) {
		if (port->phy_map & MV_BIT(i)) {
			exp->parent_phy_id[phy_id] = i;
			exp->parent_phy_count++;
			phy_id++;
		}
	}
}
