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
#include "hba_inter.h"
#include "com_nvram.h"
#ifdef CACHE_MODULE_SUPPORT
#include "cache_exp.h"
#endif		/* CACHE_MODULE_SUPPORT */

#ifdef SUPPORT_MODULE_CONSOLIDATE

#define MODCONS_MAX_EXTERNAL_REQUEST_SIZE  (1024 * 128)
#define MODCONS_SEQUENTIAL_MAX             0xFE            /* Avoid overflow. It's determined by Sequential variable size */
#define MODCONS_SEQUENTIAL_THRESHOLD       64

#define MODCONS_MAX_INTERNAL_REQUEST_SIZE  (1024 * 128)    /* The maximum request size hardware can handle. */
#define MODCONS_MIN_INTERNAL_REQUEST_SIZE  (1024 * 128)    /* We'll accumulate the request to this size and then fire. */


#include "cc.h"

#define UpdateConsolidateStatistics(x)

#define ModConsolid_ReleaseInternalRequest(pCons, pReq) \
		List_AddTail(                                    \
			&pReq->Queue_Pointer,                        \
			&pCons->Free_Queue)

#define ModConsolid_CloseRequest(pCons, pConsDevice, pInternal)              \
		do {                                                                  \
			pInternal->Req_Flag |= REQ_FLAG_CONSOLIDATE;					  \
			pInternal->Sector_Count =(pInternal->Data_Transfer_Length >> 9); \
			MV_CodeReadWriteCDB(                                              \
			pInternal->Cdb,                                               \
			pInternal->LBA,                                               \
			pInternal->Sector_Count,                                      \
			pInternal->Cdb[0]);                                           \
			pConsDevice->Holding_Request = NULL;                              \
		} while (0)

#define CONS_GET_EXTENSION(This)            ((PModConsolidate_Extension) This)
#define CONS_GET_DEVICE(This, Device_Id)    (&(This->pConsolid_Obj[Device_Id]))
#define CONS_DEVICE_IS_BUSY(dev)            (dev->io_count > 0)

//#define CONS_SEND_REQUEST(This, pInternal)		(This->pNextFunction(This->pNextExtension, pInternal))
#define CONS_SEND_REQUEST(This, pInternal)                                       \
		do {                                                                         \
			if (SCSI_IS_READ(pInternal->Cdb[0]) || SCSI_IS_WRITE(pInternal->Cdb[0])) \
			This->pConsolid_Obj[pInternal->Device_Id].io_count++;             \
			This->pNextFunction(This->pNextExtension, pInternal);                    \
		} while (0)

/*============= Consolidate usage ==========================
step:
	1. call "ModConsolid_GetResourceQuota" to get resource 	requirement of Consolidate.
	2. call "ModConsolid_InitializeExtension" to initial resource of Consolidate.
	3. call "ModConsolid_SetSendFunction" to set executing function of Consolidate.
	4. call "ModConsolid_ModuleSendRequest" to process your request.
	5. call "ModConsolid_PushFireRequest" to chech if holding request need to push.

========================================================*/

MV_ReqCompletion	g_check_func;
void
ModConsolid_RequestCallBack(MV_PVOID This, PMV_Request pReq);

void
ModConsolid_ConsolidateRequest(
    IN PModConsolidate_Extension pCons,
    IN OUT PMV_Request pInternal,
    IN PMV_Request pExternal)
{
	PConsolidate_Object pConsDevice = CONS_GET_DEVICE(pCons, pExternal->Device_Id);
	PMV_Request pAttachedReq = NULL;
	MV_U32 i = 0;

	/* So far we only handle SCSI Read 10 and SCSI Write 10 */
	MV_DASSERT(
	SCSI_IS_READ(pExternal->Cdb[0]) || SCSI_IS_WRITE(pExternal->Cdb[0]));

	if (pInternal->Data_Transfer_Length == 0) {
		pInternal->Device_Id = pExternal->Device_Id;
		pInternal->Org_Req = pExternal;
		pInternal->Req_Flag = pExternal->Req_Flag;
		pInternal->Cmd_Flag = pExternal->Cmd_Flag;
		pInternal->Scsi_Status = REQ_STATUS_PENDING;
		pInternal->Data_Transfer_Length = 0;
		pInternal->Req_Type = pExternal->Req_Type;
		pInternal->Splited_Count = pExternal->Splited_Count;
		SGTable_Init(&pInternal->SG_Table, 0);
		MV_ZeroMemory(
		pInternal->Context,
		sizeof(MV_PVOID) * MAX_POSSIBLE_MODULE_NUMBER);

		pConsDevice->Cons_Req_Count = 0;
		MV_LIST_HEAD_INIT(&pExternal->Queue_Pointer);
		pInternal->Cdb[0] = pExternal->Cdb[0];  /* Command type */
	} else {
		pAttachedReq = (PMV_Request) pInternal->Org_Req;
		MV_DASSERT(pInternal->Device_Id == pExternal->Device_Id);
		MV_DASSERT(pAttachedReq != NULL);
		List_AddTail(&pExternal->Queue_Pointer, &pAttachedReq->Queue_Pointer);
	}

	pConsDevice->Cons_Req_Count++;
	/* Don't set the sector count every time. Just before send, set the count. */
	pInternal->Data_Transfer_Length += pExternal->Data_Transfer_Length;
	pInternal->SG_Table.Occupy_Entry_Count += pExternal->SG_Table.Valid_Entry_Count;
#ifdef USE_NEW_SGTABLE
	sgdt_append_reftbl(
		&pInternal->SG_Table,
		&pExternal->SG_Table,
		0,
		pExternal->SG_Table.Byte_Count);
#else
	for (i = 0; i < pExternal->SG_Table.Valid_Entry_Count; i++) {
		SGTable_Append(
		&pInternal->SG_Table,
		pExternal->SG_Table.Entry_Ptr[i].Base_Address,
		pExternal->SG_Table.Entry_Ptr[i].Base_Address_High,
		pExternal->SG_Table.Entry_Ptr[i].Size);
	}

#endif
}

/*
 * Consolidate sub-module has got a request.
 * Two parameters:
 * This: is the pointer of the command initiator extention pointer.
 * pReq: request
 * Will fire:
 *		a. one internal request
 *		b. this external request and maybe one holding internal request if exists.
 *		c. NULL if consolidate module holds this request.
 */

void
ModConsolid_ModuleSendRequest(MV_PVOID This, PMV_Request pReq)
{
	PModConsolidate_Extension pCons = CONS_GET_EXTENSION(This);
	MV_U16 deviceId = pReq->Device_Id;
	PConsolidate_Object pConsDevice = NULL;
	PMV_Request pInternal = NULL;
	MV_LBA startLBA;
	MV_U32 sectorCount;

	if (deviceId >= pCons->dev_count)
		goto return_original_req;

	pConsDevice = CONS_GET_DEVICE(pCons, deviceId);
	/* Check if consolidate is enabled on this device */
	if (pConsDevice->Enable_Consolidate == MV_FALSE)
		goto return_original_req;

	if (pReq->Req_Flag & REQ_FLAG_NO_CONSOLIDATE)
		goto return_original_req;

	/*
	* We only handle CDB 10 read/write.
	* Otherwise, change the following code which gets the LBA and Sector Count from the CDB.
	*/
	if (!(SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0]))) {
		UpdateConsolidateStatistics(CONSOLIDATE_NOT_READ_WRITE);
		goto return_original_req;
	}

	/* It's read/write request. But is it too big for command consolidate */
	if (pReq->Data_Transfer_Length > MODCONS_MAX_EXTERNAL_REQUEST_SIZE) {
		UpdateConsolidateStatistics(CONSOLIDATE_REQUEST_TOO_BIG);
		goto return_original_req;
	}

	/* Check whether they are all read requests or write requests. */
	if (((SCSI_IS_READ(pReq->Cdb[0])) && (!pConsDevice->Is_Read))
		|| ((SCSI_IS_WRITE(pReq->Cdb[0])) && (pConsDevice->Is_Read))) {
		UpdateConsolidateStatistics(CONSOLIDATE_READ_WRITE_DIFFERENT);
		pConsDevice->Is_Read = (SCSI_IS_READ(pReq->Cdb[0])) ? 1 : 0;
		goto return_original_req;
	}

	/* Update the consolidate device statistic including last LBA and sequential counter. */
	if (!(pReq->Req_Flag & REQ_FLAG_LBA_VALID))
		MV_SetLBAandSectorCount(pReq);

	startLBA = pReq->LBA;
	sectorCount = pReq->Sector_Count;

	/* Check whether it's a sequential request. */
	if (U64_COMPARE_U64(startLBA, pConsDevice->Last_LBA))
		pConsDevice->Sequential = 0;
	else
		pConsDevice->Sequential++;  /* When equals, return 0. */


	/* Last_LBA is actually the next expect sequential LBA. */
	pConsDevice->Last_LBA = U64_ADD_U32(startLBA, sectorCount);
	if (pConsDevice->Sequential > MODCONS_SEQUENTIAL_MAX) {

	/* To avoid overflow */
	pConsDevice->Sequential = MODCONS_SEQUENTIAL_THRESHOLD;

	}

	/* Is there any requests running on this device? If no, by pass. */
	if (!CONS_DEVICE_IS_BUSY(pConsDevice)) {
		UpdateConsolidateStatistics(CONSOLIDATE_NO_RUNNING_REQUEST);
		goto return_original_req;
	}

	/* Do we reach the sequential counter threshold? */
	if (pConsDevice->Sequential < MODCONS_SEQUENTIAL_THRESHOLD) {
		UpdateConsolidateStatistics(CONSOLIDATE_LESS_THAN_SEQUENTIAL_THRESHOLD);
		goto return_original_req;
	}

	pInternal = pConsDevice->Holding_Request;

	/* Don't accumulate this request too big. */
	if (pInternal) {
	if ((pInternal->Data_Transfer_Length + pReq->Data_Transfer_Length > MODCONS_MAX_INTERNAL_REQUEST_SIZE ) ||
		(pInternal->SG_Table.Occupy_Entry_Count + pReq->SG_Table.Valid_Entry_Count >= pInternal->SG_Table.Max_Entry_Count) ||
		(pReq->Cdb[0] != pInternal->Cdb[0])) {
		ModConsolid_CloseRequest(pCons, pConsDevice, pInternal);
		CONS_SEND_REQUEST(pCons, pInternal);
		pInternal = NULL;       /* After ModConsolid_CloseRequest, pConsDevice->Holding_Request==NULL */
		}
	}

	/* Get one internal request if we don't have. */
	if (pConsDevice->Holding_Request == NULL) {
		PMV_Request pReq_tmp = NULL;
		if (!List_Empty(&pCons->Free_Queue)) {
			pReq_tmp = List_GetFirstEntry(
			&pCons->Free_Queue,
			MV_Request,
			Queue_Pointer);
		}

		if (pReq_tmp) {
			pReq_tmp->Data_Transfer_Length = 0;
			pReq_tmp->LBA = startLBA;
		}
		pConsDevice->Holding_Request = pReq_tmp;
	}

	pInternal = pConsDevice->Holding_Request;

	/* We are out of resource. */
	if (pInternal == NULL) {
		UpdateConsolidateStatistics(CONSOLIDATE_NO_RESOURCE);
		goto return_original_req;
	}

	/* Now we should be able to do consolidate requests now. */
	ModConsolid_ConsolidateRequest(pCons, pInternal, pReq);

	/* Check if consolidated requests number */
	/* Is this internal request bigger enough to fire? */
	/* We also need to check consolidate request count */
	/* because we have limited SG count for Cache Request */
	/* For 64bit OS, one consolidate request needs two SG entries */
	if ( (pInternal->Data_Transfer_Length >= MODCONS_MIN_INTERNAL_REQUEST_SIZE) ||
		((pConsDevice->Cons_Req_Count*2) >= pConsDevice->Cons_SG_Count) ) {
		ModConsolid_CloseRequest(pCons, pConsDevice, pInternal);
		CONS_SEND_REQUEST(pCons, pInternal);
		return;                     /* Send this internal request. */
	} else {
		return;                     /* Hold this request. */
	}

	return_original_req:
	/*
	* To keep the command order,
	* if we cannot do the consolidate for pReq but we hold some internal request,
	* run the internal request and then run the new pReq.
	*/
	if (pConsDevice && (pConsDevice->Holding_Request)) {
		pInternal = pConsDevice->Holding_Request;
		ModConsolid_CloseRequest(pCons, pConsDevice, pInternal);
		/* After ModConsolid_CloseRequest, pConsDevice->Holding_Request is NULL. */
		CONS_SEND_REQUEST(pCons, pInternal);
	}

	CONS_SEND_REQUEST(pCons, pReq);
	return;
}

void
ModConsolid_RequestCallBack(MV_PVOID This, PMV_Request pReq)
{
	PModConsolidate_Extension pCons = CONS_GET_EXTENSION(This);
	PConsolidate_Object pConsDevice = CONS_GET_DEVICE(pCons, pReq->Device_Id);
	PMV_Request pExternal;
	PMV_Request pAttachedReq = (PMV_Request) pReq->Org_Req;
	List_Head *q = pAttachedReq->Queue_Pointer.prev;

	if (pReq->Scsi_Status == REQ_STATUS_SUCCESS) {
		/* Extract all the external requests. Update status and return. */
		while (!List_Empty(q)) {
			pConsDevice->io_count++;
			pExternal = List_GetFirstEntry(
			q,
			MV_Request,
			Queue_Pointer);
			pExternal->Scsi_Status = REQ_STATUS_SUCCESS;
			pExternal->Completion(pExternal->Cmd_Initiator, pExternal);
		}
		pExternal = LIST_ENTRY(
				q,
				MV_Request,
				Queue_Pointer);
		pExternal->Scsi_Status = REQ_STATUS_SUCCESS;
		pExternal->Completion(pExternal->Cmd_Initiator, pExternal);
	} else {
		pConsDevice->io_count--;

		/* Make sure we won't do consolidate again for these requests. */
		pConsDevice->Sequential = 0;

		/* Re-send this request. */
		while (!List_Empty(q)) {
			pExternal = List_GetFirstEntry(
			q,
			MV_Request,
			Queue_Pointer);
			CONS_SEND_REQUEST(pCons, pExternal);
		}
		pExternal = LIST_ENTRY(
		q,
		MV_Request,
		Queue_Pointer);
		CONS_SEND_REQUEST(pCons, pExternal);
	}

	/* Release this request back to the pool. */
	ModConsolid_ReleaseInternalRequest(pCons, pReq);
}

MV_U32
ModConsolid_GetResourceQuota(MV_U16 reqCount, MV_U16 devCount, MV_U16 sgCount)
{
	MV_U32 size = 0;
	size += ROUNDING(sizeof(ModConsolidate_Extension), 8);
	size += ROUNDING(sizeof(Consolidate_Object), 8) * devCount;

	/* resource for consolidate request */
	size += ( sizeof(MV_Request) + sizeof( MV_Sense_Data ) ) *  reqCount;
	size += sizeof(MV_SG_Entry) * sgCount * reqCount;
	return size;
}

void
ModConsolid_SetSendFunction(
		MV_PVOID This,
		MV_PVOID current_ext,
		MV_PVOID next_ext,
		MV_VOID (*next_function) (MV_PVOID, PMV_Request))
{
	PModConsolidate_Extension pCons = (PModConsolidate_Extension) This;
	pCons->pUpperExtension = current_ext;
	pCons->pNextExtension = next_ext;
	pCons->pNextFunction = next_function;
	return;
}

/* Initialize the ModConsolidate_Extension */
void
ModConsolid_InitializeExtension(
		MV_PVOID This,
		MV_U16 reqCount,
		MV_U16 devCount,
		MV_U16 sgCount )
{
	PModConsolidate_Extension pCons = CONS_GET_EXTENSION(This);
	PMV_Request pReq;
	MV_U32 i;
	MV_PTR_INTEGER temp, tmpSG, tmpSense;

	temp = (MV_PTR_INTEGER) This;
	temp += ROUNDING(sizeof(ModConsolidate_Extension), 8);
	MV_LIST_HEAD_INIT(&pCons->Free_Queue);
	pCons->pConsolid_Obj = (PConsolidate_Object) temp;
	temp += ROUNDING(sizeof(Consolidate_Object), 8) * devCount;
	tmpSG = temp;
	tmpSense = temp + sizeof(MV_SG_Entry) * sgCount * reqCount;
	temp = tmpSense + sizeof( MV_Sense_Data ) * reqCount;

	for (i = 0; i < reqCount; i++) {
		pReq = (PMV_Request)(temp);
		MV_ZeroMemory(pReq, sizeof(MV_Request));

		pReq->Sense_Info_Buffer = (MV_PVOID)tmpSense;
		tmpSense += sizeof( MV_Sense_Data );
		pReq->Sense_Info_Buffer_Length = sizeof( MV_Sense_Data );

#ifdef USE_NEW_SGTABLE
		sgd_table_init(&pReq->SG_Table, (MV_U8)sgCount, tmpSG);
#else
		MV_PRINT("!!! CC only support New SG format.\n");
#endif
		tmpSG += (sizeof(MV_SG_Entry) * sgCount);
		pReq->Cmd_Initiator = pCons;
		pReq->Completion = ModConsolid_RequestCallBack;
		List_AddTail(&pReq->Queue_Pointer, &pCons->Free_Queue);
			temp += sizeof(MV_Request);
	}

	MV_ZeroMemory(
	pCons->pConsolid_Obj,
	sizeof(Consolidate_Object) * devCount);

	pCons->dev_count = devCount;

	g_check_func = ModConsolid_RequestCallBack;
	return;
}

/*
 * Caller pushes us to fire the holding request if any.
 */
void
ModConsolid_PushFireRequest(MV_PVOID This, PMV_Request pReq)
{
	PModConsolidate_Extension pCons = CONS_GET_EXTENSION(This);
	PConsolidate_Object pConsDevice = CONS_GET_DEVICE(pCons, pReq->Device_Id);
	PMV_Request pInternal;

	if (pReq->Device_Id >= pCons->dev_count)
		return;

	pConsDevice = CONS_GET_DEVICE(pCons, pReq->Device_Id);
	if (SCSI_IS_READ(pReq->Cdb[0]) || SCSI_IS_WRITE(pReq->Cdb[0]))
		pConsDevice->io_count--;

	if (CONS_DEVICE_IS_BUSY(pConsDevice))
		return;

	pInternal = pConsDevice->Holding_Request;
	if (pInternal == NULL)
		return;

	UpdateConsolidateStatistics(CONSOLIDATE_GOT_PUSHED);
	ModConsolid_CloseRequest(pCons, pConsDevice, pInternal);
	/* After ModConsolid_CloseRequest pConsDevice->Holding_Request is NULL. */
	CONS_SEND_REQUEST(pCons, pInternal);
}

void ModConsolid_SetDevice(MV_PVOID This, struct mod_notif_param *p_notify)
{
	PModConsolidate_Extension	p_cons = CONS_GET_EXTENSION(This);
	MV_U16			dev_id = p_notify->lo;
	PConsolidate_Object	p_cons_dev = CONS_GET_DEVICE(p_cons, dev_id);

	/* Module consolidation is disabled on the controller */
	/* No need to set consolidate policy for individual device when */
	/* cache policy is changed or device is arrived */
	if (p_cons->ModCons_Enabled == MV_FALSE)
		return;

	/* Set default value */
	p_cons_dev->Enable_Consolidate = MV_TRUE;
	p_cons_dev->Cons_SG_Count = MAX_SG_ENTRY;

	if (p_notify->param_count > 0) {
		MV_PU8	p_ad_param = (MV_PU8)p_notify->p_param;
		/* If we want to enable or disable consolidate based on cache setting, check p_ad_param[0] */
		/* If cache is enabled, disable consolidate for better performance. Then no need to set SG_Count */
		MV_ASSERT(p_ad_param);
		if (p_ad_param[0] == MV_TRUE)
			p_cons_dev->Enable_Consolidate = MV_FALSE;
	}
}

void ModConsolid_ChangeConsSetting(MV_PVOID p_module, MV_BOOLEAN f_enable)
{
	PHBA_Extension				p_hba = (PHBA_Extension)p_module;
	PModConsolidate_Extension	p_cons = CONS_GET_EXTENSION(p_hba->PCC_Extension);
	PConsolidate_Object			p_cons_dev;
	MV_U16						i;

	/* Set the module consolidation enable/disable flag for the controller */
	p_cons->ModCons_Enabled = f_enable;

	for (i = 0; i < MAX_DEVICE_SUPPORTED_PERFORMANCE; i++) {
		p_cons_dev = CONS_GET_DEVICE(p_cons, i);
		p_cons_dev->Enable_Consolidate = f_enable;
		if (f_enable) {
			p_cons_dev->Cons_SG_Count = MAX_SG_ENTRY;
#ifdef CACHE_MODULE_SUPPORT
			{
				MV_U8 cache_param[2];
				/* Fill in default value */
				cache_param[0] = MV_FALSE;
				cache_param[1] = MAX_SG_ENTRY;
				CACHE_report_cache_status_to_cc(p_hba, i, cache_param);
				if (cache_param[0]) {
					/* Cache is enabled on this device, disable consolidate */
					p_cons_dev->Enable_Consolidate = MV_FALSE;
				}
			}
#endif		/* CACHE_MODULE_SUPPORT */
		}
	}
}

void ModConsolid_InitConsDev(MV_PVOID p_module)
{
	PHBA_Extension	p_hba = (PHBA_Extension)p_module;
	MV_BOOLEAN	f_enable = MV_TRUE;		/* enable considation by default */

	/* Get setting saved in flash */
#if !defined(SIMULATOR) && !defined(SUPPORT_MAGNI) && !defined(SUPPORT_THOR)
	{
		HBA_Info_Page HBA_Info_Param;
		if( mv_nvram_init_param(p_hba, &HBA_Info_Param) ) {
			if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_DISABLE_MOD_CONSOLIDATE)==HBA_FLAG_DISABLE_MOD_CONSOLIDATE)
				f_enable = MV_FALSE;
		}
	}
#endif	
	ModConsolid_ChangeConsSetting((MV_PVOID)p_hba, f_enable);
}
#endif
