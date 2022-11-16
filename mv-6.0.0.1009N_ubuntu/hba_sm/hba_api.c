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

#include "mv_include.h"


#ifndef _OS_LINUX
#if !defined(SIMULATOR)
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#endif/*!SIMULATOR*/
#include <stdio.h>
#include <stdarg.h>
#endif
#include "com_request_detail.h"
#include "hba_api.h"
#include "hba_exp.h"
#include "hba_inter.h"
#ifndef _OS_LINUX
#include "win_eventlog.h"
#endif
#ifdef RAID_DRIVER
	#include "raid_structure.h"
#endif

#include "core_header.h"

#if defined(SUPPORT_MODULE_CONSOLIDATE)
#include "cc.h"
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

#define SMART_TAG  'gSMt'

#ifdef _OS_LINUX 
#define MvAllocatePool(Size)    \
    hba_mem_alloc(Size, MV_FALSE)
    
#define MvFreePool(Buffer, Size)    \
    hba_mem_free(Buffer, Size, MV_FALSE)
    
#else
#define MvAllocatePool(Size)    \
    ExAllocatePoolWithTag(NonPagedPool, Size, SMART_TAG)

#define MvFreePool(Buffer, Size)    \
    ExFreePoolWithTag(Buffer, SMART_TAG)
    
#endif    
#ifdef RAID_DRIVER
extern void Raid_GetDeviceId(
	MV_PVOID pModule,
	MV_U16 begin_id,
	PDevice_Index pDeviceIndex);
#endif
extern void Core_GetDeviceId(
	MV_PVOID pModule,
	MV_U16 begin_id,
	PDevice_Index pDeviceIndex);

#ifdef SUPPORT_TIMER
#define SMART_STATUS_REQUEST_TIMER	100

void HBA_SmartThresholdStatusCallback(MV_PVOID pModule, PMV_Request pMvReq)
{
	MV_U8 i = 0, nextDevice = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PHD_SMART_Status_Request smart_buf;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	void HBA_CheckSmartStatus (MV_PVOID ModulePointer, MV_PVOID temp);

	nextDevice = (((MV_U16)pMvReq->Cdb[6]) << 8) | ((MV_U16)pMvReq->Cdb[5]);
	nextDevice++;
#ifdef RAID_DRIVER
	if (!pHBA->RunAsNonRAID) {
		func =  Raid_GetDeviceId;
	} else
#endif
	{
		func =  Core_GetDeviceId;
	}
	func(pHBA,nextDevice,&DeviceIndex);
	while (!DeviceIndex.end) {
			pMvReq->Cdb[0] = APICDB0_PD;
			pMvReq->Cdb[1] = APICDB1_PD_GETSTATUS;
			smart_buf = (PHD_SMART_Status_Request)pMvReq->Data_Buffer;
			MV_ZeroMemory(&smart_buf->header, sizeof(RequestHeader));
			smart_buf->header.startingIndexOrId = DeviceIndex.device_id;
			smart_buf->header.requestType = REQUEST_BY_ID;
			smart_buf->header.numRequested = 1;
			MV_ZeroMemory(&smart_buf->hdSmartStatus, sizeof(HD_SMART_Status));
			pMvReq->Data_Buffer = ( MV_PVOID )smart_buf;
			pMvReq->Data_Transfer_Length = sizeof(HD_SMART_Status_Request);
			pMvReq->Cdb[4] = APICDB4_PD_SMART_RETURN_STATUS;
			pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
			pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
			pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

			pMvReq->Scsi_Status = REQ_STATUS_PENDING;
			pMvReq->Cmd_Flag = 0;
			pMvReq->Cmd_Initiator = praid_feature;

			praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
			return;

	}
	if(pMvReq->Data_Buffer != NULL){
		smart_buf = (PHD_SMART_Status_Request)pMvReq->Data_Buffer;
		MvFreePool(smart_buf, sizeof(HD_SMART_Status_Request));
		}
	List_Add(&pMvReq->Queue_Pointer, &praid_feature->Internal_Request);
	praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest( pHBA, SMART_STATUS_REQUEST_TIMER, HBA_CheckSmartStatus , praid_feature, NULL);
}


void HBA_CheckSmartStatus (MV_PVOID pModule, MV_PVOID temp)
{
	MV_U16 id = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PHD_SMART_Status_Request smart_buf;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

#ifdef RAID_DRIVER
	if(!pHBA->RunAsNonRAID) {
		func =  Raid_GetDeviceId;
	} else
#endif
	{
		func =  Core_GetDeviceId;
	}
	func(pHBA,id,&DeviceIndex);

	while (!DeviceIndex.end) {
		PMV_Request pMvReq = List_GetFirstEntry((&praid_feature->Internal_Request), MV_Request, Queue_Pointer);
		if (pMvReq) {
			//pMvReq->Data_Buffer = NULL;
			pMvReq->Completion = HBA_SmartThresholdStatusCallback;
			pMvReq->Cdb[0] = APICDB0_PD;
			pMvReq->Cdb[1] = APICDB1_PD_GETSTATUS;
			smart_buf = (PHD_SMART_Status_Request)MvAllocatePool(sizeof(HD_SMART_Status_Request));
			if (smart_buf == NULL) {
				MV_DPRINT(("Fail to get memory for HD_SMART_Status_Request!\n"));
        			return;
   			 }
		
			MV_ZeroMemory(&smart_buf->header, sizeof(RequestHeader));
			smart_buf->header.startingIndexOrId = DeviceIndex.device_id;
			smart_buf->header.requestType = REQUEST_BY_ID;
			smart_buf->header.numRequested = 1;
			MV_ZeroMemory(&smart_buf->hdSmartStatus, sizeof(HD_SMART_Status));
			pMvReq->Data_Buffer = ( MV_PVOID )smart_buf;
			pMvReq->Data_Transfer_Length = sizeof(HD_SMART_Status_Request);
			pMvReq->Cdb[4] = APICDB4_PD_SMART_RETURN_STATUS;
			pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
			pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
			pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

			pMvReq->Scsi_Status = REQ_STATUS_PENDING;
			pMvReq->Cmd_Flag = 0;
			pMvReq->Cmd_Initiator = praid_feature;

			praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
			return;
		}
	}
	praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest( pHBA, SMART_STATUS_REQUEST_TIMER, HBA_CheckSmartStatus , praid_feature, NULL);
}


void HBA_SmartOnCallback(MV_PVOID pModule, PMV_Request pMvReq)
{
	MV_U8 i = 0, nextDevice = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	nextDevice = (((MV_U16)pMvReq->Cdb[6]) << 8) | ((MV_U16)pMvReq->Cdb[5]);
	nextDevice++;
#ifdef RAID_DRIVER
	if(!pHBA->RunAsNonRAID)
		func =  Raid_GetDeviceId;
	else
#endif
		func =  Core_GetDeviceId;
	func(pHBA,nextDevice,&DeviceIndex );
	while(!DeviceIndex.end){
		pMvReq->Cdb[0] = APICDB0_PD;
		pMvReq->Cdb[1] = APICDB1_PD_SETSETTING;
		pMvReq->Cdb[2] = (MV_U8)DeviceIndex.device_id;
		pMvReq->Cdb[3] = (MV_U8)((DeviceIndex.device_id & 0xFF00) >> 8);
		pMvReq->Cdb[4] = APICDB4_PD_SET_SMART_ON;
		pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
		pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
		pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

		pMvReq->Scsi_Status = REQ_STATUS_PENDING;
		pMvReq->Cmd_Flag = 0;
		pMvReq->Cmd_Initiator = praid_feature;

		praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
		return;

	}
	 List_Add(&pMvReq->Queue_Pointer, &praid_feature->Internal_Request);
}

void hba_handle_set_smart(MV_PVOID pModule, PMV_Request pReq)
{
	get_device_id  func;
	Device_Index DeviceIndex;
	MV_U16 id = 0;
	PAdapter_Config pAdapterConfig = NULL;
	PMV_Request pTmpMvReq = NULL;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
#ifdef RAID_DRIVER
	if(!pHBA->RunAsNonRAID)
		func =  Raid_GetDeviceId;
	else
#endif
		func =  Core_GetDeviceId;
	func(pHBA,id,&DeviceIndex);

	if (pAdapterConfig->PollSMARTStatus) {
			// Stop the current timer if exists.
		if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER) {
				Timer_CancelRequest( pHBA, praid_feature->SMART_Status_Timer_Handle );
		}
		praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
		while(!DeviceIndex.end) {
			pTmpMvReq = List_GetFirstEntry((&praid_feature->Internal_Request), MV_Request, Queue_Pointer);
			if (pTmpMvReq) {
				pTmpMvReq->Completion = HBA_SmartOnCallback;
				pTmpMvReq->Cdb[0] = APICDB0_PD;
				pTmpMvReq->Cdb[1] = APICDB1_PD_SETSETTING;
				pTmpMvReq->Cdb[2] = (MV_U8)DeviceIndex.device_id;
				pTmpMvReq->Cdb[3] = (MV_U8)((DeviceIndex.device_id & 0xFF00) >> 8);
				pTmpMvReq->Cdb[4] = APICDB4_PD_SET_SMART_ON;
				pTmpMvReq->Cdb[5] = (MV_U8)(DeviceIndex.index);
				pTmpMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
				pTmpMvReq->Device_Id = VIRTUAL_DEVICE_ID;

				pTmpMvReq->Scsi_Status = REQ_STATUS_PENDING;
				pTmpMvReq->Cmd_Flag = 0;
				pTmpMvReq->Cmd_Initiator = praid_feature;
				praid_feature->pNextFunction( praid_feature->pNextExtension, pTmpMvReq);
				break;
			}
		}
		praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest(pHBA, SMART_STATUS_REQUEST_TIMER,HBA_CheckSmartStatus, praid_feature, NULL);
	} else {
			if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER)
				Timer_CancelRequest( pHBA,praid_feature->SMART_Status_Timer_Handle );
			praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
		}
}
#endif



void mvGetAdapterConfig( MV_PVOID This, PMV_Request pReq)
{
	HBA_Info_Page		HBA_Info_Param;
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;
	PAdapter_Config	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
	PAdapter_Config_V2	pAdapterConfigV2 = NULL;

	if ( pReq->Data_Transfer_Length>=sizeof(Adapter_Config_V2) )
		pAdapterConfigV2 = (PAdapter_Config_V2)pAdapterConfig;
    if (!pAdapterConfigV2)
        MV_ASSERT(MV_FALSE);

#if !defined(SIMULATOR)
	if ( mv_nvram_init_param(pHBA, &HBA_Info_Param) ) {
		pAdapterConfigV2->InterruptCoalescing =
				(HBA_Info_Param.HBA_Flag&HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) ? MV_TRUE: MV_FALSE;
		pAdapterConfigV2->PollSMARTStatus=
				(HBA_Info_Param.HBA_Flag&HBA_FLAG_SMART_ON) ? MV_TRUE: MV_FALSE;
#ifdef SUPPORT_MODULE_CONSOLIDATE
		pAdapterConfigV2->ModuleConsolidate =
				(HBA_Info_Param.HBA_Flag&HBA_FLAG_DISABLE_MOD_CONSOLIDATE) ? MV_FALSE: MV_TRUE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
		pAdapterConfigV2->AlarmOn = 
			(HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) ?
				MV_TRUE : MV_FALSE;
#endif
	} else
#endif		/* SIMULATOR */
	{
		pAdapterConfigV2->InterruptCoalescing = MV_FALSE;
    #ifdef SUPPORT_MODULE_CONSOLIDATE
		pAdapterConfigV2->ModuleConsolidate = MV_TRUE;
    #endif	/* SUPPORT_MODULE_CONSOLIDATE */

	/* Check if SMART polling is enabled */
    #ifdef SUPPORT_TIMER
		if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER)
			pAdapterConfigV2->PollSMARTStatus = MV_TRUE;
		else
    #endif	/* SUPPORT_TIMER */
			pAdapterConfigV2->PollSMARTStatus = MV_FALSE;
	}
	if (pHBA->RunAsNonRAID)
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	else
		praid_feature->pNextFunction( praid_feature->pNextExtension, pReq );
}

void mvSetAdapterConfig( MV_PVOID This, PMV_Request pReq)
{
	HBA_Info_Page		HBA_Info_Param;
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;
	PAdapter_Config	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
	PAdapter_Config_V2	pAdapterConfigV2 = NULL;
	MV_BOOLEAN need_update = MV_FALSE;

	if (pReq->Data_Transfer_Length>=sizeof(Adapter_Config_V2))
		pAdapterConfigV2 = (PAdapter_Config_V2)pAdapterConfig;

    if (!pAdapterConfigV2)
        MV_ASSERT(MV_FALSE);

#if !defined(SIMULATOR)
	if (mv_nvram_init_param(pHBA, &HBA_Info_Param)) {
#ifdef SUPPORT_MODULE_CONSOLIDATE
		if (pAdapterConfigV2->ModuleConsolidate) {
				if (HBA_Info_Param.HBA_Flag & HBA_FLAG_DISABLE_MOD_CONSOLIDATE) {
					HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
					need_update = MV_TRUE;
				}
			} else {
				if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_DISABLE_MOD_CONSOLIDATE) == 0) {
					HBA_Info_Param.HBA_Flag |= HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
					need_update = MV_TRUE;
				}
			}
		if (need_update) {
				/* Consolidation policy is changed */
				ModConsolid_ChangeConsSetting((MV_PVOID)pHBA,
					(pAdapterConfigV2->ModuleConsolidate==0 ? MV_FALSE:MV_TRUE));
			}
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

		if ( pAdapterConfigV2->InterruptCoalescing ) {
			if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) == 0) {
					HBA_Info_Param.HBA_Flag |= HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
					need_update = MV_TRUE;
				}
			} else {
				if (HBA_Info_Param.HBA_Flag & HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) {
					HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
					need_update = MV_TRUE;
				}
			}
		if ( pAdapterConfigV2->PollSMARTStatus) {
				if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_SMART_ON) == 0) {
					struct mod_notif_param param ={NULL,0,0,EVT_ID_SMART_FROM_OFF_TO_ON,0,SEVERITY_INFO,0,0,NULL,0};
					HBA_Info_Param.HBA_Flag |= HBA_FLAG_SMART_ON;
					HBA_ModuleNotification(pHBA,EVENT_LOG_GENERATED,&param);
					need_update = MV_TRUE;
				}
			} else {
				if (HBA_Info_Param.HBA_Flag & HBA_FLAG_SMART_ON) {
					struct mod_notif_param param ={NULL,0,0,EVT_ID_SMART_FROM_ON_TO_OFF,0,SEVERITY_INFO,0,0,NULL,0};
					HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_SMART_ON;
					HBA_ModuleNotification(pHBA, EVENT_LOG_GENERATED,&param);
					need_update = MV_TRUE;
				}
			}
#ifdef SUPPORT_BOARD_ALARM
		if (pAdapterConfigV2->AlarmOn == MV_TRUE) {
			if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) == 0) {
				struct mod_notif_param param ={NULL,0,0,EVT_ID_ALARM_TURN_ON,0,SEVERITY_INFO,0,0,NULL,0};
				HBA_Info_Param.HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
				HBA_ModuleNotification(pHBA,EVENT_LOG_GENERATED,&param);
				need_update = MV_TRUE;
			}
		} else {
			if (HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) {
				struct mod_notif_param param ={NULL,0,0,EVT_ID_ALARM_TURN_OFF,0,SEVERITY_INFO,0,0,NULL,0};
				HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_ENABLE_BUZZER;
				HBA_ModuleNotification(pHBA, EVENT_LOG_GENERATED,&param);
				need_update = MV_TRUE;
			}
		}
#endif
		if (pAdapterConfigV2->SerialNo[0] != 0) {
			MV_U32 i;
			for(i=0; i<20; i++)
				HBA_Info_Param.Serial_Num[i] = pAdapterConfigV2->SerialNo[i];
			need_update = MV_TRUE;
		}
#ifndef SUPPORT_MITAC
		if (pAdapterConfigV2->ModelNumber[0] != 0) {
			MV_U32 i;
			for(i=0;i<20;i++)
				HBA_Info_Param.model_number[i] = pAdapterConfigV2->ModelNumber[i];
			need_update = MV_TRUE;
		}
#endif

		if (need_update)
			mvuiHBA_modify_param(pHBA,&HBA_Info_Param);
	}
#endif

	hba_handle_set_smart(praid_feature,pReq);
	if(pHBA->RunAsNonRAID)
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	else
		praid_feature->pNextFunction( praid_feature->pNextExtension, pReq );

}


MV_U32 RAID_Feature_GetResourceQuota( MV_U16 maxIo)
{
	 MV_U32 size = 0;
	if (maxIo==1)
		size += sizeof(MV_SG_Entry) * MAX_SG_ENTRY_REDUCED * maxIo;
	else
		size += sizeof(MV_SG_Entry) * MAX_SG_ENTRY * maxIo;
	size += maxIo * sizeof(MV_Request);
	return size;
}
void RAID_Feature_Initialize(MV_PVOID This,MV_U16 maxIo)
{
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PMV_Request pReq;
	MV_U8 sgEntryCount;
	MV_U16 i;
	MV_PTR_INTEGER temp, tmpSG;

	temp = (MV_PTR_INTEGER) This;
	if (maxIo==1)
		sgEntryCount = MAX_SG_ENTRY_REDUCED;
	else
		sgEntryCount = MAX_SG_ENTRY;
	MV_LIST_HEAD_INIT(&praid_feature->Internal_Request);
	tmpSG = temp;
	temp = tmpSG + sizeof(MV_SG_Entry) * sgEntryCount * maxIo;
	for ( i=0; i<maxIo; i++ ) {
		pReq = (PMV_Request)temp;
		MV_ZeroMemory(pReq, sizeof(MV_Request));
		pReq->SG_Table.Entry_Ptr = (PMV_SG_Entry)tmpSG;
		pReq->SG_Table.Max_Entry_Count = sgEntryCount;
		List_AddTail(&pReq->Queue_Pointer, &praid_feature->Internal_Request);
		temp += sizeof(MV_Request);	/* MV_Request is 64bit aligned. */
		tmpSG += sizeof(MV_SG_Entry) * sgEntryCount;
	}
	return;
}

void RAID_Feature_SetSendFunction( MV_PVOID This,
    MV_PVOID current_ext,
    MV_PVOID next_ext,
    MV_VOID (*next_function) (MV_PVOID, PMV_Request))
{
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	praid_feature->pUpperExtension = current_ext;
	praid_feature->pNextExtension = next_ext;
	praid_feature->pNextFunction = next_function;
	praid_feature->pHBA = current_ext;
	praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
	return;
}
