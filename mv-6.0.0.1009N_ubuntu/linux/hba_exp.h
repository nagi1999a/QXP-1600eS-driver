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

#ifndef __HBA_EXPOSE_H__
#define __HBA_EXPOSE_H__
#include "hba_header.h"

typedef struct _Assigned_Uncached_Memory
{
	MV_PVOID		Virtual_Address;
	MV_PHYSICAL_ADDR	Physical_Address;
	MV_U32			Byte_Size;
	MV_U32			Reserved0;
} Assigned_Uncached_Memory, *PAssigned_Uncached_Memory;

typedef struct _Controller_Infor
{
	MV_LPVOID *Base_Address;
	MV_PVOID Pci_Device;
	MV_U16 Vendor_Id;
	MV_U16 Device_Id;
	MV_U8 Revision_Id;
	MV_U8 run_as_none_raid;
	MV_U8 cpu_count;
	MV_U8 Reserved;
} Controller_Infor, *PController_Infor;

typedef struct _SCSI_PASS_THROUGH_DIRECT {
	unsigned short Length;
	unsigned char  ScsiStatus;
	unsigned char  PathId;
	unsigned char  TargetId;
	unsigned char  Lun;
	unsigned char  CdbLength;
	unsigned char  SenseInfoLength;
	unsigned char  DataIn;
	unsigned long  DataTransferLength;
	unsigned long  TimeOutValue;
	void __user    *DataBuffer;
	unsigned long  SenseInfoOffset;
	unsigned char  Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT,SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

/*ATA Protocols*/
enum _ATA_PROTOCOL {
	HRST 			= 0x00,
	SRST  			= 0x01,
 	BUS_IDLE 		= 0x02,
	NON_DATA 		= 0x03,
	PIO_DATA_IN 	= 0x04,
	PIO_DATA_OUT 	= 0x05,     
	DMA			= 0x06, 
	DMA_QUEUED	= 0x07,
	DEVICE_DIAGNOSTIC	= 0x08,
	DEVICE_RESET		= 0x09,
	UDMA_DATA_IN		= 0x0A,
	UDMA_DATA_OUT	= 0x0B,
	FPDMA				= 0x0C,
	RTN_INFO			= 0x0F,
};

#include "com_event_struct.h"
#include "com_event_define.h"
#include "com_event_define_ext.h"
#define MSG_QUEUE_DEPTH	2048

#ifdef SUPPORT_EVENT
/* wrapper for DriverEvent, needed to implement queue */
typedef struct _Driver_Event_Entry
{
	List_Head Queue_Pointer;
	DriverEvent_V2 Event;
} Driver_Event_Entry, *PDriver_Event_Entry;
#endif /* SUPPORT_EVENT */

#ifdef SUPPORT_MODULE_CONSOLIDATE
void ModConsolid_ChangeConsSetting(MV_PVOID p_module, MV_BOOLEAN f_enable);
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

MV_VOID
HBA_GetNextModuleSendFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *next_extension,
	OUT MV_VOID (**next_function)(MV_PVOID , PMV_Request)
	);

MV_VOID
HBA_GetUpperModuleNotificationFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *upper_extension,
	OUT MV_VOID (**upper_notificaton_function)(MV_PVOID,
						   enum Module_Event,
						   struct mod_notif_param *));

void hba_notify_upper_md(
			IN MV_PVOID extension,
			  enum Module_Event notifyEvent,
			  MV_PVOID event_param);


#define HBA_SleepMicrosecond(_x, _y) ossw_udelay(_y)
#define HBA_GetTimeInSecond          ossw_get_time_in_sec
#define HBA_GetMillisecondInDay      ossw_get_msec_of_time

MV_BOOLEAN HBA_CheckIsFlorence(MV_PVOID ext);

/*read pci config space*/
MV_U32 MV_PCI_READ_DWORD(MV_PVOID This, MV_U8 reg);
MV_VOID  MV_PCI_WRITE_DWORD(MV_PVOID This, MV_U32 val, MV_U8 reg);
void HBA_ModuleStarted(MV_PVOID extension);


void HBA_GetControllerInfor(
	IN MV_PVOID extension,
	OUT PController_Infor pController
	);


/* map bus addr in sg entry into cpu addr (access via. Data_Buffer) */
void hba_map_sg_to_buffer(void *preq);
void hba_unmap_sg_to_buffer(void *preq);

MV_BOOLEAN hba_test_enabled(void *ext);
MV_BOOLEAN hba_msi_enabled(void *ext);
#ifdef SUPPORT_MSIX_INT
MV_BOOLEAN hba_msix_enabled(void *ext);
#endif
static inline MV_BOOLEAN
HBA_ModuleGetPhysicalAddress(MV_PVOID Module,
			     MV_PVOID Virtual,
			     MV_PVOID TranslationContext,
			     MV_PU64 PhysicalAddress,
			     MV_PU32 Length)
{
	panic("not supposed to be called.\n");
	return MV_FALSE;
};

int HBA_GetResource(void *extension,
		    enum Resource_Type type,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res);

int hba_get_uncache_resource(void *extension,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res);

void alloc_uncached_failed(void *extension);

MV_PVOID HBA_GetModuleExtension(MV_PVOID ext, MV_U32 mod_id);

MV_PVOID sgd_kmap(sgd_t  *sg);
MV_VOID sgd_kunmap(sgd_t  *sg,MV_PVOID mapped_addr);

MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type);
MV_BOOLEAN __is_scsi_cmd_rcv_snd_diag(MV_U8 cmd_type);

#ifdef SUPPORT_PASS_THROUGH_DIRECT
void HBARequestCallback(MV_PVOID This,PMV_Request pReq);
#endif
void HBA_SleepMillisecond(MV_PVOID ext, MV_U32 msec);
void HBA_ModuleInitialize(MV_PVOID ext,
				 MV_U32   size,
				 MV_U16   max_io);
void HBA_ModuleShutdown(MV_PVOID extension);
void HBA_ModuleNotification(MV_PVOID This,
			     enum Module_Event event,
			     struct mod_notif_param *event_param);

MV_U32 HBA_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
void HBA_ModuleStart(MV_PVOID extension);
void HBA_ModuleSendRequest(MV_PVOID this, PMV_Request req);


MV_U16 Timer_AddRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);

#ifdef SUPPORT_SMALL_TIMER
MV_U16 Timer_AddSmallRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);
#else
#define Timer_AddSmallRequest Timer_AddRequest
#endif


void Timer_CancelRequest(
	IN MV_PVOID extension,
	IN MV_U16 request_index
	);

void MV_Timer_CheckRequest(
    struct timer_list *timer
	);

void Timer_CheckRequest(
	IN MV_PVOID extension
	);

MV_U32 Timer_GetResourceQuota(MV_U16 maxIo);
void Timer_Initialize(
	IN  MV_PVOID This,
	IN MV_PU8 pool,
	IN MV_U16 max_io
	);
void Timer_Stop(MV_PVOID This);

void * os_malloc_mem(void *extension, MV_U32 size, MV_U8 mem_type, MV_U16 alignment, MV_PHYSICAL_ADDR *phy);
MV_VOID core_push_queues(MV_PVOID core_p);
MV_VOID core_send_mv_request(MV_PVOID core_p, MV_PVOID req_p);
MV_VOID core_push_all_queues(MV_PVOID core_p);
#define NO_CURRENT_TIMER		0xffff

void mv_hba_get_controller_pre(
	IN MV_PVOID extension,
	OUT PController_Infor pController
	);

MV_BOOLEAN add_event(IN MV_PVOID extension,
			    IN MV_U32 eventID,
			    IN MV_U16 deviceID,
			    IN MV_U8 severityLevel,
			    IN MV_U8 param_cnt,
			    IN MV_PU32 params,
			    IN MV_U8 SenseLength,
			    IN MV_PU8 psense,
			    IN MV_U16 trans_bit);

void get_event(MV_PVOID This, PMV_Request pReq);
MV_PVOID hba_mem_alloc(MV_U32 size,MV_BOOLEAN sg_use);
MV_VOID hba_mem_free(MV_PVOID mem_pool, MV_U32 size,MV_BOOLEAN sg_use);
void mvs_hexdump(u32 size, u8 *data, u32 baseaddr, const char *prefix);

#ifdef SUPPORT_IO_DELAY
MV_U16 hba_get_io_delay_value(void);
#endif
MV_U32 hba_parse_ata_protocol(struct scsi_cmnd *scmd);
#if (defined(SUPPORT_DIF) || defined(SUPPORT_DIX)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
extern unsigned int mv_prot_mask;
extern unsigned char mv_prot_guard;
#endif
#endif

