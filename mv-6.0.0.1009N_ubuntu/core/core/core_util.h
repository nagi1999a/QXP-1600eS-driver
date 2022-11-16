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

#ifndef CORE_UTIL_H
#define CORE_UTIL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_protocol.h"
#include "hba_timer.h"

/* 
 * this file includes functions that protocol layer needs from manager layer.
 * therefore, it is different in HP/Marvell implementations 
 */
#ifdef SUPPORT_STAGGERED_SPIN_UP
MV_VOID staggered_spin_up_handler(domain_base * base, MV_PVOID tmp);
MV_U32 core_spin_up_get_cached_memory_quota(MV_U16 max_io);
#endif

MV_LPVOID get_core_mmio(pl_root *root);

MV_PVOID core_get_handler(pl_root *root, MV_U8 handler_id);

/* 
 * init state machine related
 * a. core_queue_init_entry with core_init_entry_done
 *    queue domain_base as the member
 * b. core_append_init_request will queue MV_Request 
 *    which generated by init state machine
 */
MV_VOID core_queue_init_entry(pl_root *root, MV_PVOID base_ext, 
	MV_BOOLEAN start_init);
MV_VOID core_init_entry_done(pl_root *root, domain_port *port, 
	domain_base *base);
MV_VOID core_append_init_request(pl_root *root, MV_Request *req);

/*
 * error handling related
 * a. core_queue_error_req and core_complete_error_req
 *    for the original error request
 * b. core_queue_eh_req
 *    for error handling request
 */
MV_VOID core_queue_error_req(pl_root *root, MV_Request *req, MV_BOOLEAN new_error);
MV_VOID core_complete_error_req(pl_root *root, MV_Request *req, MV_U8 status);
MV_VOID core_queue_eh_req(pl_root *root, MV_Request *req);

/*
 * normal request
 */
MV_VOID core_append_request(pl_root *root, MV_Request *req);
MV_VOID core_push_running_request_back(pl_root *root, MV_Request *req);
MV_VOID core_append_high_priority_request(pl_root *root, MV_Request *req);

MV_VOID core_notify_device_hotplug(pl_root *root, MV_BOOLEAN plugin,
	MV_U16 dev_id, MV_U8 generate_event);
MV_VOID core_generate_error_event(MV_PVOID core_p, MV_Request *req);

#ifdef CORE_SUPPORT_LARGE_REQUEST
/* ikkwong: put here for now */
MV_Request *core_split_large_request(pl_root *root, MV_Request *large_req);
#endif
#ifdef _OS_LINUX
#ifdef __VMKLNX__
#define CORE_REQUEST_TIME_OUT_SECONDS	60
#else
#define CORE_REQUEST_TIME_OUT_SECONDS	15
#endif
#else
#define CORE_REQUEST_TIME_OUT_SECONDS	10
#endif
#define CORE_REQ_INIT_TIMEOUT		15
#define CORE_INQUIRE_TIME_OUT_SECONDS	5
#define CORE_REQ_FLORENCE_INIT_TIMEOUT	30
#define CORE_PHY_DECODE_ERR_CNT	5
#if defined(CORE_PCIE_PNP_DETECTION)
#define CORE_PCIE_DETECT_TIME_OUT	4
#endif
MV_U16 core_add_timer(MV_PVOID core, MV_U32 seconds,
	MV_VOID (*routine) (MV_PVOID, MV_PVOID), 
	MV_PVOID context1, MV_PVOID context2);

MV_VOID core_cancel_timer(MV_PVOID core, MV_U16 timer);

#define core_sleep_millisecond     HBA_SleepMillisecond
#define core_sleep_microsecond     HBA_SleepMicrosecond

MV_U32 List_Get_Count(List_Head * head);

#define core_generate_event(ext, eid, did, slv, pc, ptr, tran)                 \
        {                                                                      \
                struct mod_notif_param param = {ptr, 0, 0, eid, did, slv, pc,  \
                                                0, NULL, tran};                \
                HBA_ModuleNotification(ext, EVENT_LOG_GENERATED, &param);      \
        }
#define core_generate_event_with_sense(ext, eid, did, slv, pc, ptr,            \
                                        senselength, psense)                   \
	{                                                                      \
                struct mod_notif_param param = {ptr, 0, 0, eid, did, slv, pc,  \
                                                senselength, psense};          \
                HBA_ModuleNotification(ext, EVENT_LOG_GENERATED, &param);      \
        }
#if defined(SUPPORT_CHANGE_DEBUG_MODE)
extern MV_U16 mv_debug_mode;
#define CORE_PLAIN_DPRINT(_x_)        do {if (CORE_DEBUG_INFO & mv_debug_mode)\
        MV_DPRINT(_x_);\
        } while (0)
#define CORE_DPRINT(_x_)        do {if (CORE_DEBUG_INFO & mv_debug_mode)\
	{MV_DPRINT(("CORE: %s ", __FUNCTION__));\
	MV_DPRINT(_x_);}\
	} while (0)

#define CORE_PRINT(_x_)        do {if (CORE_DEBUG_INFO & mv_debug_mode)\
		{if (GENERAL_DEBUG_INFO & mv_debug_mode) \
	  	{CORE_DPRINT(_x_);} else\
	  	{MV_PRINT("CORE:  ");\
	  	MV_PRINT _x_;} }\
	} while (0)

#else
#define CORE_PLAIN_DPRINT(_x_) do {\
        MV_DPRINT(_x_);\
        } while (0)
#define CORE_DPRINT(_x_)        do {\
	MV_DPRINT(("CORE: %s ", __FUNCTION__));\
	MV_DPRINT(_x_);\
	} while (0)
	
#ifdef MV_DEBUG
#define CORE_PRINT(_x_)         CORE_DPRINT(_x_)
#else
#define CORE_PRINT(_x_)        	    do {\
	MV_PRINT("CORE:  ");\
	MV_PRINT _x_;\
	} while (0)
#endif

#endif /*SUPPORT_CHANGE_DEBUG_MODE*/
MV_PVOID core_map_data_buffer(MV_Request *req);
MV_VOID core_unmap_data_buffer(MV_Request *req);
MV_U8  core_check_duplicate_device(pl_root *root, domain_device *dev);
#if 0
MV_U8 core_check_duplicate_exp(pl_root *root, domain_expander *exp);
#endif

MV_U32 core_check_outstanding_req(MV_PVOID core_p);
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
MV_U8 
core_wideport_load_balance_asic_phy_map(domain_port *port,
	domain_device *dev);
#endif

MV_U32 mv_read_addr_data_regs(MV_LPVOID *mmio_base,MV_U32 addr_reg,MV_U32 data_reg,MV_U32 cmd);
MV_LPVOID mv_get_mmio_base(void *ext);
int mv_read_type_reg(MV_LPVOID *mmio_base, char *buf, MV_U32 offset, MV_U16 buf_len, char *type, int len);
void mv_write_type_reg(MV_LPVOID *mmio_base, MV_U32 addr_reg,MV_U32 value_reg, char *type);
int mv_dump_intr_reg(MV_LPVOID *mmio_base,char *pBuffer, MV_U16 buf_len, int len);
int mv_dump_sas_sata_port_cfg_ctrl_reg(MV_LPVOID *mmio_base,char *pBuffer, MV_U16 buf_len, int len);

extern void core_global_lock(void *ext, unsigned long *flags);
extern void core_global_unlock(void *ext, unsigned long *flags);
#endif /* CORE_UTIL_H */