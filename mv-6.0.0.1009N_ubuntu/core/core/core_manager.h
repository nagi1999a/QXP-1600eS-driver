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

#ifndef __CORE_MANAGER_H
#define __CORE_MANAGER_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_resource.h"
#include "core_device.h"
#if defined(HARDWARE_XOR) || defined(SOFTWARE_XOR)
#include "core_xor.h"
#endif
#ifdef SUPPORT_SGPIO
#include "core_gpio.h"
#endif
#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
#include "core_i2c.h"
#ifdef SUPPORT_TWSI
#include "core_twsi.h"
#endif
#ifdef SUPPORT_BBU
#include "core_bbu.h"
#endif
#endif
#ifdef SUPPORT_RMW_FLASH
#include "core_rmw_flash.h"
#endif
#ifdef SUPPORT_PD_PAGE
#include "core_pd_map.h"
#endif
#include "core_err_inj.h"
#ifdef SUPPORT_BOARD_ALARM
#include "core_alarm.h"
#endif
#ifdef SUPPORT_STAGGERED_SPIN_UP
struct device_spin_up{
	List_Head	list;
	pl_root		*roots;	
	struct _domain_base *base;
};
#endif
enum CORE_STATE {
	CORE_STATE_IDLE = 0,
	CORE_STATE_STARTING,
	CORE_STATE_STARTED,
};
enum PHY_RATE {
    PHY_RATE_1_5G = 0,
    PHY_RATE_3G,
    PHY_RATE_6G,
    PHY_RATE_6G_COMPATIBLE  // Because so sale our product use 3 as 6G setting.
};
typedef struct _core_extension {
	MV_PVOID			  desc;
	MV_U16                          vendor_id;
	MV_U16                          device_id;

	const struct mvs_chip_info *chip_info;

    int             submitting;

	MV_U8                           revision_id;
	MV_U8                           is_dump;
	MV_U8                           state;
	union {
		MV_U8                   bh_enabled; //Linux only
		MV_U8                   reserved0;
	};
#if defined(CORE_PCIE_PNP_DETECTION)
	MV_U8				pci_state; //TB PNP only
#else
	MV_U8				s3_state; //Linux only
#endif
	MV_PVOID                        sg_pool; //Linux only
	MV_PVOID                        ct_pool; // Linux only
#ifdef PCI_POOL_SUPPORT
#ifndef HARDWARE_XOR
	char				sg_name[CACHE_NAME_LEN];
#endif
	char				ct_name[CACHE_NAME_LEN];
#endif

	MV_LPVOID                       mmio_base;
	MV_LPVOID                       io_base;
	MV_LPVOID                       nvsram_base;
	command_handler                 handlers[MAX_NUMBER_HANDLER];

	MV_U16                          max_io;
	MV_U16                          hw_sg_entry_count; /* It's hardware sg entry count per buffer */
	MV_U16                          init_queue_count; /* count the number devices need initialization */

	MV_U32                          irq_mask; /* mask for main irq */
	MV_U32                          main_irq; /* save interrupt status */

	lib_resource_mgr                lib_rsrc;
	lib_device_mgr                  lib_dev;
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK core_queue_running_SpinLock;
	KSPIN_LOCK core_SpinLock;
	KSPIN_LOCK pause_waiting_queue_SpinLock;
	KIRQL      core_queue_running_OldIrql;
	KIRQL      pause_waiting_queue_OldIrql;
	KSPIN_LOCK init_queue_SpinLock;
	KSPIN_LOCK event_queue_SpinLock;
	KSPIN_LOCK error_queue_SpinLock;
	KSPIN_LOCK high_priority_SpinLock;
	KSPIN_LOCK core_global_SpinLock;
#elif defined(_OS_LINUX)
	spinlock_t core_queue_running_SpinLock;
	spinlock_t core_SpinLock;
	spinlock_t pause_waiting_queue_SpinLock;
	spinlock_t init_queue_SpinLock;
	spinlock_t event_queue_SpinLock;
	spinlock_t error_queue_SpinLock;
	spinlock_t high_priority_SpinLock;
	spinlock_t core_global_SpinLock;
#endif
#endif
	MV_BOOLEAN						core_queue_running;
	MV_BOOLEAN						pause_waiting_queue;
	MV_U16							waiting_queue_running;

	/* domain_base is queued up for initialization */
	Counted_List_Head               init_queue;
	/* event_record is queued up for event handling */
	List_Head                       event_queue;
	/* error requests are queued up. trigger handling at proper time */
	Counted_List_Head               error_queue;

	/* MV_Request waiting for handling.
	 * it is for normal IO */
#ifndef ROOT_WAITING_QUEUE
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK waiting_queue_SpinLock;
	KIRQL      waiting_queue_OldIrql;
#elif defined(_OS_LINUX)
	spinlock_t waiting_queue_SpinLock;
#endif
#endif
	Counted_List_Head               waiting_queue;
#endif
	/* compared with waiting_queue, it's high priority
	 * it is used for error handling request only */
	Counted_List_Head               high_priority_queue;
	/* MV_Request finished */
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK cmpl_queue_SpinLock;
	KIRQL      cmpl_queue_OldIrql;
#elif defined(_OS_LINUX)
	spinlock_t cmpl_queue_SpinLock;
#endif
#endif
	List_Head                       complete_queue;

	/* Internal request completion queue */
	List_Head						internal_compl_queue;
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK internal_compl_SpinLock;
#elif defined(_OS_LINUX)
	spinlock_t internal_compl_SpinLock;
#endif
#endif
#ifdef HARDWARE_XOR
	pl_xor                          xors;
	/* sequential complete request for xor engineer*/
	List_Head			xor_run_fifo_header;
#endif
	pl_root                         roots[MAX_NUMBER_IO_CHIP];

#ifdef SUPPORT_SGPIO
	lib_gpio			lib_gpio;
#endif
#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
	lib_i2c                       lib_i2c;
#ifdef SUPPORT_TWSI
	lib_twsi			lib_twsi;
#endif
#ifdef SUPPORT_BBU
	lib_bbu			lib_bbu;
#endif
#endif
#ifdef SUPPORT_RMW_FLASH
        lib_rmw_flash                   lib_flash;
#endif
#ifdef SUPPORT_PD_PAGE
        lib_pd_page                     lib_pd;
#endif
	lib_error_inj                 lib_error;

#ifdef SUPPORT_BOARD_ALARM
	lib_alarm			alarm;
#endif
#ifdef HOTPLUG_BYTE_COUNT_ERROR_WORKAROUND
	MV_PHYSICAL_ADDR                trash_bucket_dma;
#endif
#if defined(DOUBLEBUF_4_SINGLE_PRD) || defined(TEST_PRD_ENTRY_IN_SRAM)
    MV_PVOID    doublebuf_vir;
    MV_PHYSICAL_ADDR doublebuf_dma;
#endif
#ifdef TEST_SRAM
    MV_PVOID    sram_vir;
#endif

	MV_U8	run_as_none_raid;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	MV_U8 		spin_up_group;
	MV_U8		spin_up_time;		
	List_Head	device_spin_up_list;
	MV_U16	 	device_spin_up_timer;
	MV_U8		reserved[2];
#else
	MV_U8	reserved[3];
#endif
#ifdef SUPPORT_DIRECT_SATA_SSU
	MV_U8	sata_ssu_group;
	MV_U8	sata_ssu_time;
#endif
	MV_U8 	sata_ssu_mode;
    MV_U8   enable_sgpio;
    MV_U8   has_port_queue;
	MV_U8 enable_multi_wcq;
    MV_U8   cpu_count;
#ifdef SUPPORT_PHY_POWER_MODE
	MV_U32	PHY_power_mode_port_map;//bit mapping to port
	MV_U16	PHY_power_mode_HIPM_timer;
	MV_U16   PHY_power_mode_timer_index;
	MV_U8	PHY_power_mode_HIPM;
	MV_U8	PHY_power_mode_DIPM;
	MV_U16   PHY_power_mode_port_enabled;
#endif
#if defined(SUPPORT_SP2)
	MV_U32 page_size;
#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	MV_U8	run_multi_thread_send_cmd;	
#endif
} core_extension;
#define IS_CORE_STARTED(core)	(((core_extension *)core)->state & CORE_STATE_STARTED)
#ifdef SUPPORT_PHY_POWER_MODE
MV_VOID Core_EnablePowerStateHIPM(MV_PVOID core_p,MV_PVOID context2);
MV_VOID Core_Disable_PHY_Interrupt(domain_phy *phy);
MV_VOID Core_Enable_PHY_Interrupt(domain_phy *phy);
MV_VOID Core_Enable_Receive_PHY_Power_Request(domain_phy *phy);
#endif
MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
MV_VOID Core_ModuleInitialize(MV_PVOID ModulePointer, MV_U32 extensionSize, MV_U16 maxIo);
MV_VOID Core_ModuleStart(MV_PVOID This);
MV_VOID Core_ModuleSendRequest(MV_PVOID This, PMV_Request pReq);
MV_VOID Core_ModuleMonitor(MV_PVOID This);
MV_VOID Core_ModuleNotification(MV_PVOID This, enum Module_Event event, struct mod_notif_param *param);
MV_VOID Core_ModuleReset(MV_PVOID This);
MV_VOID Core_ModuleShutdown(MV_PVOID This);

MV_VOID core_set_chip_options(MV_PVOID ext);
MV_VOID core_queue_completed_req(MV_PVOID ext, MV_Request *req);
MV_VOID core_queue_completed_req_lock(MV_PVOID ext, MV_Request *req);
MV_VOID core_complete_requests(MV_PVOID core_p);

/* mult-waiting&complete-queue function */
MV_VOID core_complete_single_queue(MV_PVOID core_p, MV_PVOID queue_p);
MV_VOID core_complete_base_queue(MV_PVOID core_p, MV_PVOID base_p);
MV_VOID core_complete_all_queues(MV_PVOID core_p);
MV_VOID core_push_base_queue(core_extension * core, domain_base * base);
MV_VOID core_push_all_queues(MV_PVOID core_p);
MV_VOID core_send_mv_request(MV_PVOID core_p, MV_PVOID req_p);
MV_BOOLEAN core_append_queue_cmpl(core_extension *core, PMV_Request req);
MV_BOOLEAN core_append_queue_request(core_extension *core, 
						PMV_Request req, MV_BOOLEAN is_high);
pl_queue *core_get_hw_queue_of_base(core_extension *core, domain_base *base);

#ifdef _OS_LINUX
MV_VOID core_abort_all_running_requests(MV_PVOID core_p);
#endif

void core_disable_ints(void *ext);
void core_enable_ints(void *ext);
void core_disable_queue_ints(void *ext, MV_U16 queue_msk);
void core_enable_queue_ints(void *ext, MV_U16 queue_msk);
MV_BOOLEAN core_reset_controller(core_extension * core);
MV_BOOLEAN core_clear_int(core_extension *core);
MV_VOID core_handle_int(core_extension *core);
void update_port_phy_map(pl_root *root, domain_phy *phy);
MV_VOID controller_init(core_extension *core);
MV_VOID io_chip_init_registers(pl_root *root);
void update_phy_info(pl_root *root, domain_phy *phy);
void core_set_cmd_header_selector(mv_command_header *cmd_header);
void set_phy_tuning(pl_root *root, domain_phy *phy, PHY_TUNING phy_tuning_G1, PHY_TUNING phy_tuning_G2, PHY_TUNING phy_tuning_G3, PHY_TUNING phy_tuning_G4);
MV_VOID set_phy_ffe_tuning(pl_root *root, domain_phy *phy, FFE_CONTROL ffe);
MV_VOID set_phy_rate(pl_root *root, domain_phy *phy, MV_U8 rate);
MV_U8 get_min_negotiated_link_rate(domain_port *port);
void core_set_hw_queue_of_base(core_extension *core, domain_base *base);
#ifdef IGNORE_FIRST_PARITY_ERR
MV_VOID core_enable_intl_parity(MV_PVOID core_p);
#endif
struct mvs_chip_info {
	MV_U16		chip_id;
	MV_U8 		n_host;
	MV_U8		start_host;
	MV_U32 		n_phy;
	MV_U32 		fis_offs;
	MV_U32 		fis_count;
	MV_U32 		srs_sz;
	MV_U32 		slot_width;
};

#endif /* __CORE_MANAGER_H */
