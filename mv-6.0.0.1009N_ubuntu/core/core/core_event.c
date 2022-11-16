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
#include "core_manager.h"
#include "core_resource.h"
#include "core_error.h"
#include "core_util.h"
#include "core_sas.h"

/*
 * PAL Manager drive the event handling
 * 1. dont do event handling when this port has error requests in the queue
 *    so error handling state machine wont mess up with event handling
 */

static void pal_event_handling(domain_phy *phy, MV_U8 event_id);
static void pl_handle_hotplug_event(pl_root *root, domain_phy *phy);
static void pl_handle_unplug(pl_root *root, domain_phy *phy);
#ifdef SUPPORT_VU_CMD
static void pl_handle_async_event(pl_root *root, domain_phy *phy);
#endif

void pl_handle_plugin(pl_root *root, domain_phy *phy);
void sas_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
void sas_handle_plugin(pl_root *root, domain_port *port, domain_phy *phy);
void sata_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
MV_VOID exp_handle_broadcast(pl_root *root, domain_phy *phy);

extern void update_phy_info(pl_root *root, domain_phy *phy);
extern void update_port_phy_map(pl_root *root, domain_phy *phy);
extern void sas_init_port(pl_root *root, domain_port *port);
extern void sata_init_port(pl_root *root, domain_port *port);
extern void free_port(pl_root *root, domain_port *port);
extern void exp_update_direct_attached_phys(domain_port *port);
extern void sata_phy_spinup_hold(pl_root *root, domain_phy *phy);

MV_BOOLEAN pal_check_duplicate_event(pl_root *root, event_record *old_event, 
        MV_U8 phy_id, MV_U8 event_id)
{
        // duplicate event on the same phy
        if ((old_event->root == root) && (old_event->event_id == event_id) && 
                (old_event->phy_id == phy_id)) {
                return MV_TRUE;
        }

        // duplicate broadcast for same wideport
	if ((event_id == PL_EVENT_BROADCAST_CHANGE) &&
		(old_event->event_id == event_id) && 
        	(root->phy[old_event->phy_id].port) && 
		(old_event->root->phy[old_event->phy_id].port == root->phy[phy_id].port)) {
		return MV_TRUE;
        }

        // duplicate phy change for same wideport
        if ((event_id == PL_EVENT_PHY_CHANGE) && 
        	(old_event->event_id == event_id) && 
        	(old_event->root->phy[old_event->phy_id].port) && 
        	(old_event->root->phy[old_event->phy_id].port == root->phy[phy_id].port)) {
              return MV_TRUE;
        }

        return MV_FALSE;
}

void pal_notify_event(pl_root *root, MV_U8 phy_id, MV_U8 event_id)
{
	core_extension *core = (core_extension *)root->core;
	event_record *event, *tmp_event;
	MV_ULONG flags;

	if (core->state != CORE_STATE_STARTED)
		return;
#ifdef SUPPORT_ACTIVE_CABLE
	MV_ASSERT((event_id == PL_EVENT_PHY_CHANGE)
		|| (event_id == PL_EVENT_ASYNC_NOTIFY)
		|| (event_id == PL_EVENT_CABLE_HOTPLUG)
		|| (event_id == PL_EVENT_BROADCAST_CHANGE));
#else
	MV_ASSERT((event_id == PL_EVENT_PHY_CHANGE)
		|| (event_id == PL_EVENT_ASYNC_NOTIFY)
		|| (event_id == PL_EVENT_BROADCAST_CHANGE));
#endif

	/*
	 * If the incoming event already has a duplicate in the event queue,
	 * don't need to add it again.
	 */
	OSSW_SPIN_LOCK(&core->event_queue_SpinLock, flags);
	LIST_FOR_EACH_ENTRY_TYPE(tmp_event, &core->event_queue, event_record,
		queue_pointer) {
                if (pal_check_duplicate_event(root, tmp_event, phy_id, event_id)) {
                        CORE_EVENT_PRINT(("duplicate event 0x%x on root %p phy 0x%x.\n",\
				event_id, root, phy_id));
			OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
			return;
                }
	}

	/* save the event for late handling */
	event = get_event_record(root->lib_rsrc);
	if (event) {
		event->root = root;
		event->phy_id = phy_id;
		event->event_id = event_id;
		EVENT_SET_DELAY_TIME(event->handle_time, 500); //msec
		List_AddTail(&event->queue_pointer, &core->event_queue);
	} else {
		CORE_EVENT_PRINT(("no resource to queue event 0x%x "
			"on root %p phy 0x%x.\n",
			event_id, root, phy_id));
	}
	OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
}

#ifdef _OS_LINUX
void core_clean_event_queue(core_extension *core)
{
	event_record *event = NULL;
	pl_root *root;
	
	while (!List_Empty(&core->event_queue)) {
		event = (event_record *)List_GetFirstEntry(
			&core->event_queue, event_record, queue_pointer);
		root = event->root;
		free_event_record(root->lib_rsrc, event);
	}
}
#endif

void core_handle_event_queue(core_extension *core)
{
	event_record *event = NULL;
	pl_root *root;
	domain_phy *phy;
	event_record *old_event = NULL;
	domain_port *port;
	MV_U32 i;
	MV_ULONG flags;
	OSSW_SPIN_LOCK(&core->event_queue_SpinLock, flags);
	while (!List_Empty(&core->event_queue)) {
		event = (event_record *)List_GetFirstEntry(
			&core->event_queue, event_record, queue_pointer);
		if (event == old_event) {
			List_Add(&event->queue_pointer, &core->event_queue);
			OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
			return;
		}

		root = event->root;
		phy = &root->phy[event->phy_id];
		port = phy->port;

#if 1
		port = &root->ports[event->phy_id];
		if (port_has_init_req(root, port)) {
			if (old_event == NULL) 
				old_event = event;
                     List_AddTail(&event->queue_pointer, &core->event_queue);
			continue;
		}
#else
		for (i = 0; i < root->phy_num; i++) {
			port = &root->ports[i];
			/* if there is init request running,
			 * dont do the event handling
			 * it can be two cases,
			 * a. during bootup(init state machine), event comes.
			 * b. hotplug comes again before the former one completes.
			 * we wont do multiple event handling on the same port */
			if (port_has_init_req(root, port)) {
				if (old_event == NULL) old_event = event;
				//List_AddTail(&event->queue_pointer, &core->event_queue);
				//continue;
                                List_Add(&event->queue_pointer, &core->event_queue);
				OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
				return;
			}
		}
#endif		
		if (event->event_id == PL_EVENT_PHY_CHANGE) {
#ifdef _OS_LINUX
			if (MV_FALSE == time_is_expired(event->handle_time)) {
				List_Add(&event->queue_pointer, &core->event_queue);
				OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
				return;
			}
#else
			/* here do not use timer trigger handle hotplug interrupt */
			core_sleep_millisecond(root->core, 500);
#endif
#ifdef SUPPORT_ACTIVE_CABLE
			if (phy->is_cable_hotplug == MV_TRUE) {
				List_AddTail(&event->queue_pointer, &core->event_queue);
				OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock, flags);
				return;
			}
#endif
		}

		pal_event_handling(phy, event->event_id);
		free_event_record(root->lib_rsrc, event);
	}
	OSSW_SPIN_UNLOCK(&core->event_queue_SpinLock,flags);
}

extern MV_VOID pm_hot_plug(domain_pm *pm);
static void pal_event_handling(domain_phy *phy, MV_U8 event_id)
{
    pl_root *root = phy->root;
    #ifdef SUPPORT_VU_CMD
    MV_U32 reg = 0;
   	MV_U16 dev_id = 0xFFFF;
	domain_device *dev  = NULL;
    #endif
    #ifdef SUPPORT_ACTIVE_CABLE
	MV_U8 i, start_phy_id;
    #endif

        CORE_EVENT_PRINT(("handle event: phy id %d, event id %d.\n", (root->base_phy_num+phy->id), event_id));
	switch (event_id) {
#ifdef SUPPORT_ACTIVE_CABLE
	case PL_EVENT_CABLE_HOTPLUG:
		i2c_handle_hotplug_cable(root->core, phy->cable_id);

		/* compute phy id by cable id*/
		if ((phy->cable_id % 2) == 1) {
			/* cable id 1,3 relate to phy0-3 */
			start_phy_id = 0;
		} else {
			/* cable id 2,4 relate to phy4-7 */
			start_phy_id = 4;
		}

		/* clean cable relate 4 phys' flag */
		for (i = 0; i < 4; i++) {
			root->phy[start_phy_id + i].is_cable_hotplug = MV_FALSE;
		}
		break;
#endif

	case PL_EVENT_PHY_CHANGE:
		CORE_EVENT_PRINT(("phy change event: phy %d, irq_status is 0x%x\n", \
			(root->base_phy_num+phy->id), phy->irq_status));

		pl_handle_hotplug_event(root, phy);
		break;

	case PL_EVENT_ASYNC_NOTIFY:
		CORE_EVENT_PRINT(("asynchronous notification event: " \
			"root %p phy id %d phy type =0x%x \n", root, (root->base_phy_num+phy->id),phy->type));
			
		if ((phy->port)&&(phy->port->type == PORT_TYPE_SATA) && (phy->port->pm)) {
			pm_hot_plug(phy->port->pm);
		} 
	#ifdef SUPPORT_VU_CMD
		else if ((phy->port) && (phy->type == PORT_TYPE_SATA)) { 
			/* unassociated FIS have been checked adn cleared before here */
			//reg = MV_REG_READ_DWORD(root->rx_fis,SATA_UNASSOC_D2H_FIS(phy) + 0x0);
			//if (((reg & 0xFF) == SATA_FIS_TYPE_SET_DEVICE_BITS) && (reg & MV_BIT(15))) {
			{
				dev_id = get_id_by_phyid(root->lib_dev,phy->id);
				dev = (domain_device *)get_device_by_id(root->lib_dev, dev_id, MV_FALSE, MV_FALSE);
	  			mv_cancel_timer(root->core,&dev->base);
				pal_abort_running_req_for_async_sdb_fis(root, dev);

				CORE_EVENT_PRINT(("Notify device %d async event(0x%x) to API.\n",dev_id,PL_EVENT_SDB_ASYNC_NOTIFY));
				core_generate_event(root->core, PL_EVENT_SDB_ASYNC_NOTIFY, dev_id,SEVERITY_OTHER, 0, NULL,0);

				CORE_EVENT_PRINT(("Sending Ack of SDB FIS to FW.\n"));
				pl_handle_async_event(root,phy);
			}
		}
	#endif
		break;

	case PL_EVENT_BROADCAST_CHANGE:
		CORE_EVENT_PRINT(("broadcast event: root %p phy %d.\n", \
			root, (root->base_phy_num+phy->id)));
		exp_handle_broadcast(root, phy);
		break;

	default:
		MV_ASSERT(MV_FALSE);
	}
}
#if 0
static int pl_internal_handle_hotplug(pl_root *root, domain_phy *phy)
{
	domain_port *port = phy->port;
	domain_device *dev = NULL;
	domain_base *base = NULL;
	core_extension *core = (core_extension *)root->core;
	MV_U16 j = 0;
	MV_U32 reg = 0;

	if(NULL == port) {
	//	CORE_EVENT_PRINT(("%s: phy is not exist.\n",__func__));
		return -1;
	}

	if(!List_Empty(&port->expander_list)) {
	//	CORE_EVENT_PRINT(("%s: can not handle expander device.\n",__func__));
		return -1;
	}
	if(port->pm != NULL) {
	//	CORE_EVENT_PRINT(("%s: can not handle PM device.\n",__func__));
		return -1;
	}
	if(List_Empty(&port->device_list)){
		return -1;
	}
	while (!List_Empty(&port->device_list)) {
		base = List_GetFirstEntry(&port->device_list, domain_base, queue_pointer);
		dev  = (domain_device *)base;

		/* direct attached devices? */
		if(base->type != BASE_TYPE_DOMAIN_DEVICE)
		{
		//	CORE_EVENT_PRINT(("%s: not direct attache devcie, can't handle ", __func__));
			return -1;
		}

		/* abort all pending request */
		pal_abort_port_running_req(root, port);

		/* hard reset phy */
		mv_reset_phy(root, port->phy_map, MV_TRUE);
		/* mv_reset_phy will enable interrupt */
		//core_disable_ints(root->core); /*I'm not sure whether affect other code */ 
												
		if(IS_STP_OR_SATA(dev)) {  /* device is sata, need to wait sig fis */

			j = 0;
			do {
				reg = READ_PORT_IRQ_STAT(root, phy);
				core_sleep_millisecond(root->core, 100);
				j++;
			}while(!(reg & IRQ_SIG_FIS_RCVD_MASK) && (j < 100));

			if(j > 99 ) {
				CORE_EVENT_PRINT((" can not get FIS sig."));
				return -1;
			}
			phy->irq_status &= ~IRQ_SIG_FIS_RCVD_MASK;
			WRITE_PORT_IRQ_STAT(root, phy,phy->irq_status);
		}

		dev->state = DEVICE_STATE_RESET_DONE;
		dev->status |= DEVICE_STATUS_INTERNAL_HOTPLUG;

		core_queue_init_entry(root, base, MV_TRUE);
	}
	phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
	return 0;
}
#endif
static void pl_handle_hotplug_event(pl_root *root, domain_phy *phy)
{
	MV_U32 tmp_irq, tmp_cnt = 500;
	MV_U32 irq = phy->irq_status;
	MV_U16 j = 0, i =0;
	MV_U16 phy_ready=0;
	domain_port *port = phy->port;
#ifdef SUPPORT_PHY_POWER_MODE
	core_extension *core = root->core;
	MV_U32 tmp;
#endif

	CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", (root->base_phy_num+phy->id), irq));
#if 1
	if (!(irq & IRQ_PHY_RDY_CHNG_MASK))  {
	    CORE_EVENT_PRINT(("nothing to do.\n"));
          return;
       }
	phy->phy_decode_err_cnt = 0;
	if (irq & IRQ_PHY_RDY_CHNG_1_TO_0) {
#ifdef SUPPORT_PHY_POWER_MODE
		if(core->PHY_power_mode_HIPM || core->PHY_power_mode_DIPM){
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
			tmp = READ_PORT_CONFIG_DATA(root, phy);
			MV_PRINT("root %x phy %x PMODE_STATUS %x!\n",root->root_id,phy->id, (tmp & PMODE_STATUS_MASK));
			if(core->PHY_power_mode_port_enabled & (1<<(root->base_phy_num + phy->asic_id))
				&& (tmp & PMODE_STATUS_MASK))	//Check if phy is in power mode
			{
				phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
				MV_PRINT("root %x phy %x in power mode, don't handle hotplug!\n",root->root_id,phy->id);
				return;
			}
		}
#endif
		for(i=0;i<3;i++){
			core_sleep_millisecond(root->core, 10);
			if(mv_is_phy_ready(root, phy))
				continue;
		}
		if (!mv_is_phy_ready(root, phy)) {
#ifdef SUPPORT_PHY_POWER_MODE
			if(core->PHY_power_mode_HIPM || core->PHY_power_mode_DIPM){
				if(core->PHY_power_mode_port_enabled & (1<<(root->base_phy_num + phy->asic_id))){
					core->PHY_power_mode_port_enabled &= ~(1<<(root->base_phy_num + phy->asic_id));
					MV_PRINT("Before unplug root %x phy %x clear the power mode record!\n",root->root_id,phy->id);
				}
			}
#endif
			pl_handle_unplug(root, phy);
		} else {
			if (phy->link_err_cnt == 0xff) {
				MV_PRINT("unplug interrupt and phy(%d) link err cnt to 0xff\n",(root->base_phy_num+phy->id));
				phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
				return;
			}
			phy->link_err_cnt++;
			MV_PRINT("unplug interrupt but phy(%d) is still connected. link error count %d\n",(root->base_phy_num+phy->id), phy->link_err_cnt);

			if(phy->link_err_cnt>= 0xff){
				phy->link_err_cnt = 0;
			}
#if defined(SUPPORT_SOC) || defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
			else {
				if (port && ((port->device_count != 0) || (port->pm != NULL) || (port->expander_count != 0))) {
					MV_PRINT("device is exist. Didn't handle plugin.\n");
				}
				else{
					core_sleep_millisecond(root->core, 10);
					if (mv_is_phy_ready(root, phy)){
						MV_PRINT("unplug interrupt but phy(%d) is still connected. link error count %d handle as plug in\n",phy->id, phy->link_err_cnt);

/* To solve Luigi2 detect long time bug */
/* Due to initial thread is handling Unplug event but other PHY Change attention of the same device Luigi2*/
/* Cause there are two plugin/unplug event need to handle */
/* So following root cause that will clear the second one PHY Change bit */
						do{
							WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
							tmp_irq = READ_PORT_CONFIG_DATA(root, phy);
							if (!mv_is_phy_ready(root, phy))
								break;	
							if ( (tmp_irq & IRQ_PHY_RDY_CHNG_MASK) && !(tmp_irq &IRQ_PHY_RDY_CHNG_1_TO_0))	//phy change and plug in
							{
								//First time to detect phy change and plug in then going to clear bit
								WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
								tmp_irq = READ_PORT_CONFIG_DATA(root, phy);
								WRITE_PORT_CONFIG_DATA(root, phy, tmp_irq);
								break;
							}
							core_sleep_millisecond(root->core, 10);
						}while(tmp_cnt--);//phy not change or unplug so waiting to phy state change (wrong event 2)
						if (mv_is_phy_ready(root, phy)){
							WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
							tmp_irq = READ_PORT_CONFIG_DATA(root, phy);
							if ( (tmp_irq & IRQ_PHY_RDY_CHNG_MASK) && !(tmp_irq &IRQ_PHY_RDY_CHNG_1_TO_0))	{
								//Second time to detect phy change and plug in then going to clear bit
								WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
								tmp_irq = READ_PORT_CONFIG_DATA(root, phy);
								WRITE_PORT_CONFIG_DATA(root, phy, tmp_irq);
							}
						}
						
						pl_handle_plugin(root, phy);
					}
				}
			}
#endif
		}
	} else {
#ifdef SUPPORT_PHY_POWER_MODE
		if(core->PHY_power_mode_HIPM || core->PHY_power_mode_DIPM){
			if(core->PHY_power_mode_port_enabled & (1<<(root->base_phy_num + phy->asic_id)))//Check if phy is in power mode
			{
				if( !core->PHY_power_mode_DIPM){//if enable DIPM, don't dis-or core->PHY_power_mode_port_enabled
					WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
					tmp = READ_PORT_CONFIG_DATA(root, phy);
					MV_PRINT("root %x phy %x PMODE_STATUS %x!\n",root->root_id,phy->id,(tmp&PMODE_STATUS_MASK));
					if((tmp&PMODE_STATUS_MASK) == 0){//Leave power mode, dis-or power mode record
						core->PHY_power_mode_port_enabled &= ~(1<<(root->base_phy_num + phy->asic_id));
					}
				}
				phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
				MV_PRINT("root %x phy %x in power mode, don't handle hotplug!\n",root->root_id,phy->id);
				return;
			}
		}
#endif
		/* wait for PHY to be ready */
		phy->link_err_cnt=0;
		while (j < 100) {	//0512
			core_sleep_millisecond(root->core, 10);
			if (mv_is_phy_ready(root, phy))
				break;
			j++;
		}
		if (mv_is_phy_ready(root, phy)) {
			if (port &&
				((port->device_count!=0) || (port->pm!=NULL) || (port->expander_count!=0))
				) {
				pl_handle_unplug(root, phy);
			}
			pl_handle_plugin(root, phy);
		} else {
			CORE_EVENT_PRINT(("plugin interrupt but phy is gone.\n"));
		}
	}
#else

	if (irq & IRQ_PHY_RDY_CHNG_1_TO_0) {
		if (!mv_is_phy_ready(root, phy)) {
			pl_handle_unplug(root, phy);
		} else {
			if(!pl_internal_handle_hotplug(root, phy))
				return;

			CORE_EVENT_PRINT(("unplug interrupt but "\
				"phy is still connected.\n"));
			pl_handle_unplug(root, phy);
			pl_handle_plugin(root, phy);
		}
	} else if (irq & IRQ_PHY_RDY_CHNG_MASK) {
		/* wait for PHY to be ready */
		while (j < 500) {
			core_sleep_millisecond(root->core, 10);
			if (mv_is_phy_ready(root, phy))
				break;
			j++;
		}

		if (mv_is_phy_ready(root, phy)) {
			if (port &&
				((port->device_count!=0) || (port->pm!=NULL) || (port->expander_count!=0))
				) {
				pl_handle_unplug(root, phy);
			}
			pl_handle_plugin(root, phy);
		} else {
			CORE_EVENT_PRINT(("plugin interrupt but phy is gone.\n"));
		}
	} else {
		CORE_EVENT_PRINT(("nothing to do.\n"));
	}
#endif
	phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
}

static void pl_handle_unplug(pl_root *root, domain_phy *phy)
{
	domain_port *port = phy->port;

	if (NULL == port) {
		/* HARRIET_TBD: this is possible if immediately after the phy change interrupt,
		 * another port interrupt occurs. We do not clear the phy change bit
		 * in irq_status until after the event is handled, so the hot plug event
		 * may be reported twice. The second time the hot plug is handled,
		 * port is already NULL.
		 *
		 * Let this pass for now, may need to modify code later to not
		 * report hot plug twice
		 */
		CORE_EVENT_PRINT(("unplug phy %d which not exist.\n",
			(root->base_phy_num+phy->id)));
		return;
	}
	if (port->type & PORT_TYPE_SATA) {
		sata_handle_unplug(root, port, phy);
	} else {
		sas_handle_unplug(root, port, phy);
	}

	/*
	 * have to do clean up here and not earlier,
	 * otherwise port will be lost
	 */
	CORE_DPRINT(("phy %d is now gone\n", (root->base_phy_num+phy->id)));
}

void pl_handle_plugin(pl_root *root, domain_phy *phy)
{
	domain_port *port;
	MV_U32 reg = 0;
	core_extension *core = (core_extension *)root->core;
	domain_base *item;
	if(phy->port){
		port = phy->port;
		LIST_FOR_EACH_ENTRY_TYPE(item , &core->init_queue, struct _domain_base,
				init_queue_pointer) {
			if (&port->base == item) {
				CORE_DPRINT(("find same base at init queue %p.\n", &port->base));
	//#ifdef STRONG_DEBUG
	//				MV_DASSERT(item != base);
	//#endif
				return;
			} 
		}
	}
	update_phy_info(root, phy);
	update_port_phy_map(root, phy);
	port = phy->port;
	/* port can be NULL if phy status was not ready in update_port_phy_map */
	if (port == NULL) {
#ifdef STRONG_DEBUG
		/* Possible? We've already checked the PHY is ready. */
		MV_DASSERT(MV_FALSE);
#endif
		return;
	}

	if (port->type & PORT_TYPE_SATA) {
		/* Should clean the unassociated FIS interrupt before issuing
		 * reset phy. Saw a case with a special WD hard drive that
		 * sends two signature FIS's. For this case, if we do not clean
		 * the status, hardware could get stuck not responding to X_RDY
		 * with R_RDY
		 */		
		reg = READ_PORT_IRQ_STAT(root, phy);
		if (reg & IRQ_UNASSOC_FIS_RCVD_MASK) {
			WRITE_PORT_IRQ_STAT(root, phy, IRQ_UNASSOC_FIS_RCVD_MASK);
		} 
		
		/* For Athena Write command with PI */
#if defined(GT_WA_FOR_ASIC_WRITE)
		{
			io_chip_init_registers(root);
			root->last_cmpl_q = 0xfff;
			root->last_delv_q = 0xfff;
			root->running_num = 0;
		}
#endif

		mv_reset_phy(root, port->phy_map, MV_TRUE);
		core_sleep_millisecond(root->core, 100);
		sata_init_port(root, port);
	} else {
		sas_handle_plugin(root, port, phy);
	}
}

void sas_handle_plugin(pl_root *root, domain_port *port, domain_phy *phy)
{
// Should only add below workaroud for Vanir chip.
#ifdef SUPPORT_BALDUR
	MV_U8 i;
	MV_U16 j = 0;
	MV_U8 phyrdy_check_mask = 0;
	core_extension *core = root->core;
	//*****************************************************************
	//Note: This work around is fixed in C1
	//Workaround for STP hotplug watchdog timeout issue
	//assumes vanir minisas cable is plugged in for expander STP.
	//when the first phy of one wide port comes in, we need to do STP
	//Link Reset before transmitting STP commands.  Additionally
	//STP Link Reset can only be sent when there is no IO traffic.
	//We have to reset all phys in a wide port when the first phy is
	//plugged in, because we may not be able to do STP Link Reset later
	//if IO has started, and expander may start transmitting via other
	//phys when it transmits data or response frames.
	//*****************************************************************
	
#if defined(PRODUCTNAME_VANIR) || defined(PRODUCTNAME_VANIRLITES)
	if(((core->revision_id != VANIR_C1_REV) && (core->revision_id != VANIR_C2_REV))
           && (port->phy_num == 1))        //Skip workaround if phy is not the first phy in the port.
	{
		// STP Reset for the first phy in this port.
		WRITE_PORT_PHY_CONTROL(root, phy, SCTRL_STP_LINK_LAYER_RESET);
		
		// Check Other phys that need to handle plugin event.
		for (i = 0; i < root->phy_num; i++) {
			if (root->phy[i].port == NULL)
				phyrdy_check_mask |= MV_BIT(i);
		}

		// Check Other phys for ready status within .1 seconds, if ready do STP reset to them too.
		while ((j < 10) && phyrdy_check_mask) {
			for (i = 0; i < root->phy_num; i++) {
				if ((phyrdy_check_mask & MV_BIT(i))	&&
					(mv_is_phy_ready(root, (&root->phy[i])))) {
						MV_DPRINT(("CORE: STP Hotplug WorkAround Phy%d STP Link Rst\n", i));
						WRITE_PORT_PHY_CONTROL(root, (&root->phy[i]), SCTRL_STP_LINK_LAYER_RESET);
						phyrdy_check_mask &= ~MV_BIT(i);
				}
			}
			core_sleep_millisecond(root->core, 10);
			j++;
		}
	}
#endif
#endif
	/* HARRIET_TBD: ugly code - possible to improve? */
	if (!List_Empty(&port->expander_list)) {
		/* part of a wide port. update phy info */
		/* is there any other kinds of wideport other than expander? */
		MV_ASSERT(port->att_dev_info & (PORT_DEV_SMP_TRGT | PORT_DEV_STP_TRGT));
		exp_update_direct_attached_phys(port);
	} else {
		/* a brand new port */
		sas_init_port(root, port);
	}
}

void sas_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy)
{
	domain_expander *exp = NULL;
	MV_U8 i;
        MV_U8 phy_map_left;
#if 0
        phy_map_left = port->phy_map & (MV_U8)(~MV_BIT(phy->id));
        update_phy_info(root, phy);
	update_port_phy_map(root, phy);
#else
        domain_phy *tmp_phy;
        phy_map_left = port->phy_map;
        /* delay a little bit to wait for all PHY removed */
        core_sleep_millisecond(root->core, 100);
        for (i = 0; i < root->phy_num; i++) {
                /* if it's wide port, check all related PHY
                 * maybe all the PHYs are gone.
                 * so we needn't handle phy down one by one and abort request */
                tmp_phy = &root->phy[i];
                if (phy_map_left & MV_BIT(i)) {
  	                update_phy_info(root, tmp_phy);
	                update_port_phy_map(root, tmp_phy);
                }
        }
        phy_map_left = port->phy_map; /* maybe at this moment, all PHYs are gone. */
        CORE_EVENT_PRINT(("handle unplug event port %p phy id %d, phy_map 0x%x.\n", \
                port, (root->base_phy_num+phy->id), phy_map_left));
#endif

	if (phy_map_left) {
		/* was wide port */

                /* 1)it was real wide port
                 * 2)we think it's a wide port but it's not.
                 * For example we receive a unplug interrupt, 
                 * but the PHY is back right now.*/
		if (!List_Empty(&port->expander_list)) {
			pal_abort_port_running_req(root, port);
			exp_update_direct_attached_phys(port);
			LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list,
				domain_expander, base.queue_pointer) {
				pal_clean_expander_outstanding_req(root, exp);
			}
		} else 
			pal_set_down_port(root, port);
	} else {
		/* was narrow port */
		/* remove devices attached. complete all requests */
		pal_set_down_port(root, port);

		/* port variables are cleared already in update_port_phy_map */
	}
}

void sata_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy)
{
	/* it means directly attached disk is unplugged
	 * or pm is unplugged. */
	pal_set_down_port(root, port);
	mv_reset_stp(root, port->phy_map);
	//hal_clear_srs_irq(root, 0, MV_TRUE);
#ifdef SUPPORT_DIRECT_SATA_SSU
	sata_phy_spinup_hold(root, phy);
#endif
	update_phy_info(root, phy);
}

MV_VOID exp_handle_broadcast(pl_root *root, domain_phy *phy)
{
	domain_port *port = phy->port;
	domain_expander *exp;

	if (!port)
		return;

	CORE_DPRINT(("double checking phy status before broadcast\n"));
#if 0
	if (!mv_is_phy_ready(root, phy)) {
		CORE_DPRINT(("phy %d not ready, updating port\n"));
		update_phy_info(root, phy);
		update_port_phy_map(root, phy);
		exp_update_direct_attached_phys(port);
	}
#else
        {
                MV_U8 phy_map_left;
                MV_U8 i;
                domain_phy *tmp_phy;
                phy_map_left = port->phy_map;
                /* unplug expander, broadcast happened before the PHY change
                 * so delay a little bit for the PHY gone */
                core_sleep_millisecond(root->core, 100);
                core_sleep_millisecond(root->core, 100);
                for (i = 0; i < root->phy_num; i++) {
                        /* if it's wide port, check all related PHY
                        * maybe all the PHYs are gone.
                        * so we needn't handle phy down one by one and abort request */
                        tmp_phy = &root->phy[i];
                        if (phy_map_left & MV_BIT(i)) {
  	                        update_phy_info(root, tmp_phy);
	                        update_port_phy_map(root, tmp_phy);
                        }
                }
                phy_map_left = port->phy_map; /* maybe at this moment, all PHYs are gone. */

	        if (phy_map_left == 0) {
		        /* remove devices attached. complete all requests */
		        pal_set_down_port(root, port);

		        /* port variables are cleared already in update_port_phy_map */
                        return;
                }

                exp_update_direct_attached_phys(port);
        }
#endif

#ifdef EXP_ABORT_REQ_DURING_BROADCAST
	/* push back running reqs before doing broadcast */
	pal_abort_port_running_req(root, port);

        LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list,
	        domain_expander, base.queue_pointer) {
		pal_clean_expander_outstanding_req(root, exp);
	}
#endif

	/* clear old routing tables & find root expander on this port */
	LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list, domain_expander,
		base.queue_pointer) {
		MV_ZeroMemory(exp->route_table, sizeof(MV_U16)*MAXIMUM_EXPANDER_PHYS);
		if (exp->base.parent == &port->base) {
			exp->state = EXP_STATE_DISCOVER;
			core_queue_init_entry(root, &exp->base, MV_TRUE);
		}
	}
}

#ifdef SUPPORT_VU_CMD

static void handle_async_event_callback(MV_PVOID root_p, PMV_Request req)
{	
	domain_device *device  = NULL;
	pl_root *root = (pl_root *)root_p;
	
	device = (domain_device*)get_device_by_id(root->lib_dev,req->Device_Id, MV_FALSE, MV_FALSE);
	if(NULL != device)
		CORE_EVENT_PRINT(("Sending ACK of SDB FIS finished,device->status = 0x%x\n",device->status));	
}

static void pl_handle_async_event(pl_root *root, domain_phy *phy)
{
	MV_Request *req;
	domain_device *device  = NULL;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return;
	
	req->Cdb[0] = SCSI_CMD_ATA_PASSTHRU_12; //Operation Code ATA 12
	req->Cdb[1] = ATA_PROTOCOL_NON_DATA << 1; //Multiple_Count,Protocol,Extend
	req->Cdb[2] = 0; //Off_Line,CC,Reserved,T_Dir,BB,T_Length
	req->Cdb[3] = MARVELL_VU_CMD_ASYNC_NOTIFY; //Feature
	req->Cdb[4] = 0;  // Sector Count
	req->Cdb[5] = 0; //LBA Low
	req->Cdb[6] = 0; //LBA Mid
	req->Cdb[7] = 0; //LBA High
	req->Cdb[8] = 0; //Device
	req->Cdb[9] = SCSI_CMD_MARVELL_VENDOR_UNIQUE; //Command
	req->Cdb[10] = 0;  //Reserved
	req->Cdb[11] = 0; //Contol
	
	req->Time_Out = CORE_REQUEST_TIME_OUT_SECONDS;
	req->Cmd_Flag = CMD_FLAG_NON_DATA;
	
	req->Device_Id = get_id_by_phyid(root->lib_dev,phy->id);
	device = (domain_device*)get_device_by_id(root->lib_dev,req->Device_Id, MV_FALSE, MV_FALSE);
	device->status |= DEVICE_STATUS_WAIT_ASYNC;
	
	req->Completion = handle_async_event_callback;	
	core_append_request(root, req);

}

#endif



