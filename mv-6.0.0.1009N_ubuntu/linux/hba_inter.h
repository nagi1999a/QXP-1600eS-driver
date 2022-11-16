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

#ifndef HBA_INTERNAL_H
#define HBA_INTERNAL_H
#include "hba_header.h"
#include "com_tag.h"
#ifdef SUPPORT_TIMER
typedef struct _Timer_Request
{
	List_Head Queue_Pointer;
	MV_PVOID Context1;
	MV_PVOID Context2;
	MV_PVOID Reserved0;
	MV_VOID (*Routine) (MV_PVOID, MV_PVOID);
	MV_BOOLEAN Valid;
	MV_U8 Reserved1[7];
	MV_U64 Time_Stamp;		// when this requested function wants to be called
} Timer_Request, *PTimer_Request;


typedef struct _Timer_Module
{
	/* Because we are supporting performance mode, */
	/* we cannot use array for running request */
	/*	PTimer_Request Running_Requests[MAX_TIMER_REQUEST]; */
	/* Use pointer, remember to allocate continuous memory for it */
	PTimer_Request *Running_Requests;
	MV_U16 Timer_Request_Number;
	MV_U8 Reserved0[6];
	Tag_Stack Tag_Pool;
	MV_U64 Time_Stamp;		// current time
	spinlock_t timer_Spinlock;
} Timer_Module, *PTimer_Module;

#endif

enum hba_info_flags {
	MVF_MSI		= (1U << 0),	/* MSI is enabled */
#ifdef SUPPORT_TARGET
	MVF_TARGET_MODE_ENABLE	= (1U << 1),	/* Target Mode Enable */
	MVF_HOST_SHUTTING_DOWN	= (1U << 2),	/* Shutting down HBA */
#endif
};

/* Per logical unit */
struct mv_lu
{
	MV_U16 id; //device->ID
	MV_U16 lun;
	MV_U16 reserved0[2];	
	MV_U16 target_id;	
	struct scsi_device	*sdev;		/* attached SCSI device */
};

/* Per msix intr data */
struct msix_data
{
	struct hba_extension *hba;

	MV_U16 index;
	MV_U16 root;
	MV_U16 queue;
	MV_U16 valid;
	MV_U32 vector;
	char name[32];
};

typedef struct hba_extension
{
	/* self-descriptor */
	struct mv_mod_desc *desc;

	void    *req_pool;
	/* Device extention */
	MV_PVOID Device_Extension;
	/* System resource */
	MV_LPVOID Base_Address[MAX_BASE_ADDRESS];
	MV_U32 State;
	MV_BOOLEAN Is_Dump;		/* Is OS during hibernation or crash dump? */
	atomic_t Io_Count;			/* Outstanding requests count */
	MV_U32	hba_flags;
	
	/* Adapter information */
	MV_U8 Adapter_Bus_Number;
	MV_U8 Adapter_Device_Number;
	MV_U16 Vendor_Id;
	MV_U16 Device_Id;
	MV_U8 Revision_Id;
	MV_U8 RunAsNonRAID;		/* initialize it before InitModules is called */
	MV_BOOLEAN msi_enabled;
	spinlock_t Waiting_Request_SpinLock;
	spinlock_t Events_SpinLock;
	spinlock_t Io_Count_SpinLock;
#ifdef SUPPORT_MSIX_INT
	MV_BOOLEAN msix_enabled;
#else
	MV_BOOLEAN boolean_reserved;
#endif
	MV_BOOLEAN test_enabled;
	MV_U8 reserved1;
	MV_U16 RaidMode;			/*raid mode*/
	MV_U16 Sub_Vendor_Id;
	MV_U16 Sub_System_Id;

	MV_U8 pcie_max_lnk_spd;	/* PCIe Max Supported Link Speed */
	MV_U8 pcie_max_bus_wdth;	/* PCIe Max Supported Bus Width */
	MV_U8 pcie_neg_lnk_spd;		/* PCIe Negotiated Link Speed */
	MV_U8 pcie_neg_bus_wdth;	/* PCIe Negotiated Bus Width */
	
	MV_U32 MvAdapterSignature;
#ifdef SUPPORT_ESATA
	MV_U8 eSATAPortCount;
	MV_U8 reserved3[7];

	MV_U8 eSATAPort[8];
#endif
	/* Timer module */
	Timer_Module TimerModule;
	MV_PVOID			uncached_virtual;
	MV_PHYSICAL_ADDR	uncached_physical;
	MV_U32				uncached_quota;
	MV_U32				scsiport_allocated_uncached;
	MV_U16				Max_Io;
	MV_U16				waiting_cb_cnt;

	/* Request queue */
#ifndef USE_SRBEXT_AS_REQ
	List_Head Free_Request;		/* Free MV_Request queue */
#endif
	List_Head Waiting_Request;	/* MV_Request waiting queue */

#ifdef USE_REQ_POOL
	MV_U32 max_sg_count;
	mempool_t * mv_mempool;
	
	 kmem_cache_t  *mv_request_cache;
	 kmem_cache_t  *mv_request_sg_cache;
	
	char cache_name[CACHE_NAME_LEN];
	char sg_name[CACHE_NAME_LEN];

#ifdef HAVE_HW_COMPLIANT_SG
    char sgpool_name[CACHE_NAME_LEN];
    struct pci_pool *mv_sgtable_pool;
#endif
#endif

#ifdef SUPPORT_EVENT
	List_Head Stored_Events;
	List_Head Free_Events;
	MV_U32	SequenceNumber;
	MV_U8 Num_Stored_Events;
	MV_U8 Reserved2[3];	/* make the structure 8 byte aligned */
#endif

	struct mv_lu mv_unit[MV_MAX_TARGET_NUMBER];

	MV_U8 FlashBad;
	MV_U8 FlashErase;
	atomic_t Ioctl_Io_Count;
	MV_U8 first_scan;

#ifdef SUPPORT_MODULE_CONSOLIDATE
   	MV_PVOID PCC_Extension;
#endif
	MV_PVOID		pNextExtension;
	MV_VOID 		(*pNextFunction)(MV_PVOID , PMV_Request);
	MV_PVOID p_raid_feature;

    struct proc_dir_entry *proc_dir;
    struct proc_dir_entry *reg_info;
    char proc_name[15];
	MV_BOOLEAN reg_enabled;

	/* msi-x */
	MV_U16 msix_count;
	struct msix_entry *msix_entries;
	struct msix_data *msix_data;
	spinlock_t startIO_lock;
}HBA_Extension, *PHBA_Extension;

#define DRIVER_STATUS_IDLE      1    /* The first status */
#define DRIVER_STATUS_STARTING  2    /* Begin to start all modules */
#define DRIVER_STATUS_STARTED   3    /* All modules are all settled. */
#define DRIVER_STATUS_SHUTDOWN   4   /* All modules shutdown. */

#endif /* HBA_INTERNAL_H */
