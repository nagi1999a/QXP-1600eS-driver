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

#ifndef __MODULE_MANAGE_H__
#define __MODULE_MANAGE_H__

#include "hba_header.h"

/* fs TODO:multi define. */
#define MAX_NUMBER_IO_CHIP	2
#define MAX_MULTI_QUEUE		8

/* module management & hba module code */
extern struct mv_module_ops *mv_core_register_module(void);
extern struct mv_module_ops *mv_hba_register_module(void);

#ifdef RAID_DRIVER
extern struct mv_module_ops *mv_raid_register_module(void);
#else
static inline struct mv_module_ops *mv_raid_register_module(void)
{
	return NULL;
}
#endif /* RAID_DRIVER */

#if defined(CACHE_MODULE_SUPPORT)
extern struct mv_module_ops *mv_cache_register_module(void);
#else
static inline struct mv_module_ops *mv_cache_register_module(void)
{
	return NULL;
}
#endif /* CACHE_DRIVER */

/* adapter descriptor */
struct mv_adp_desc {
	List_Head hba_entry;
	List_Head  online_module_list;
	spinlock_t   global_lock;
	spinlock_t   isr_lock;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	spinlock_t   device_spin_up;
#endif
	/* for athena16 multi-queue */
	spinlock_t hba_lock;			/* lock hba extension */
	spinlock_t resource_lock;		/* lock core lib_resource_mgr */
	spinlock_t root_lock[MAX_NUMBER_IO_CHIP];	/* lock root's resource */
	spinlock_t wait_queue_lock;		/* lock core waiting/high priority queue */
	spinlock_t compl_queue_lock;	/* lock core completion queue */
	spinlock_t core_queue_lock;		/* lock core init/event/error queue */
	spinlock_t hw_queue_lock[MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE];	/* hw queue lock */
	spinlock_t hw_wq_lock[MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE];	/* hw waiting queue lock */
	spinlock_t hw_cq_lock[MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE];		/* hw complete queue lock */
	spinlock_t core_lock;

	struct timer_list hba_timer;
	struct pci_dev    *dev;
	dev_t   dev_no;
#ifndef MV_VMK_ESX35
	struct cdev 	cdev;
#endif
	struct Scsi_Host	*hba_host;
	struct completion		cmpl;
	struct completion		ioctl_cmpl;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_t				hba_sync;
	atomic_t				hba_ioctl_sync;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

#ifdef SUPPORT_TASKLET
	struct tasklet_struct mv_tasklet;
	atomic_t		tasklet_active_count;
	spinlock_t	tasklet_count_lock;
#endif

	/* adapter information */
	MV_U8             Adapter_Bus_Number;
	MV_U8             Adapter_Device_Number;
	MV_U8             Revision_Id;
	MV_U8             id;             /* multi-hba support, start from 0 */
	MV_U8             running_mod_num;/* number of up & running modules */
	MV_U8             RunAsNonRAID;		/* initialize it before InitModules is called */
	MV_U16            RaidMode;			/*raid mode*/
	MV_U16            Vendor_Id;
	MV_U16            Device_Id;
	MV_U16            Sub_System_Id;
	MV_U16            Sub_Vendor_Id;	
	
	MV_U8		pcie_max_lnk_spd;	/* PCIe Max Supported Link Speed */
	MV_U8		pcie_max_bus_wdth;	/* PCIe Max Supported Bus Width */
	MV_U8		pcie_neg_lnk_spd;		/* PCIe Negotiated Link Speed */
	MV_U8		pcie_neg_bus_wdth;	/* PCIe Negotiated Bus Width */
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
	unsigned int   pci_config_space[16];
#endif
	/* System resource */
	MV_PVOID          Base_Address[ROUNDING(MAX_BASE_ADDRESS, 2)];
	MV_U32            max_io;
	MV_BOOLEAN    alloc_uncahemem_failed;
	MV_U8		ioctl_cmpl_Scsi_Status;
	MV_U8		reserved01[2];
};

int  mv_hba_init(struct pci_dev *dev, MV_U32 max_io);
void mv_hba_release(struct pci_dev *dev);
void mv_hba_stop(struct pci_dev *dev);
int  mv_hba_start(struct pci_dev *dev);
MV_PVOID *mv_get_hba_extension(struct mv_adp_desc *hba_desc);
int __mv_get_adapter_count(void);
void raid_get_hba_page_info( MV_PVOID This);
struct mv_mod_desc * __get_lowest_module(struct mv_adp_desc *hba_desc);
struct mv_mod_desc * __get_highest_module(struct mv_adp_desc *hba_desc);
int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc);
int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  struct pci_dev *dev);

#endif /* __MODULE_MANAGE_H__ */

