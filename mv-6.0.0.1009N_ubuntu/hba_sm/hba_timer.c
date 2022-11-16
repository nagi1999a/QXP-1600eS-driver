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
#if defined(BREAK_TEST)
extern MV_U8 gBreak;
#endif
#ifdef SUPPORT_TIMER
MV_U16 Timer_GetRequestCount(MV_U16 maxIo)
{
	MV_U16 reqCount;

	/*
	  *when not in hibernation mode, allocate twice as many timers as # of
	  * devices because hot plug now requires a timer also for each device
	  */
	if (maxIo==1)
		reqCount = MAX_DEVICE_SUPPORTED_PERFORMANCE; /* no smart timer */
	else if (maxIo==MAX_REQUEST_NUMBER_PERFORMANCE)
		reqCount = (MAX_DEVICE_SUPPORTED_PERFORMANCE + 1) * 2;
	else
		reqCount = (MAX_DEVICE_SUPPORTED_WHQL + 1) * 2;

	return reqCount;
}
#endif

MV_U32 Timer_GetResourceQuota(MV_U16 maxIo)
{
#ifdef SUPPORT_TIMER
	MV_U32 sz;
	MV_U16 reqCount;

	reqCount = Timer_GetRequestCount(maxIo);
	/* Memory for timer tag pool */
	sz = ROUNDING((sizeof(MV_U16) * reqCount), 8);
	/* Memory for timer request array */
	sz += ROUNDING((sizeof(PTimer_Request) * reqCount), 8);
	/* Memory for timer request */
	sz += ROUNDING((sizeof(Timer_Request) * reqCount), 8);
	return sz;
#else
	return 0;
#endif
}

MV_PU8 Timer_Initialize(
	IN OUT PTimer_Module This,
	IN MV_PU8 pool,
	IN MV_U16 max_io
	)
{
#ifdef SUPPORT_TIMER
	MV_PTR_INTEGER temp = (MV_PTR_INTEGER)pool;
	PTimer_Request pTimerReq;
	MV_U16 i, reqCount;

	reqCount = Timer_GetRequestCount(max_io);
	This->Timer_Request_Number = reqCount;
	/* allocate memory for timer request tag pool */
	This->Tag_Pool.Stack = (MV_PU16)temp;
	This->Tag_Pool.Size = reqCount;
	temp += sizeof(MV_U16) * reqCount;
	Tag_Init( &This->Tag_Pool, reqCount );

	U64_ZERO_VALUE(This->Time_Stamp);
	MV_ASSERT( sizeof(Timer_Request)==ROUNDING(sizeof(Timer_Request),8) );
	/* allocate memory for timer request array */
	This->Running_Requests = (PTimer_Request *)temp;
	temp += sizeof(PTimer_Request) * reqCount;
	for (i = 0; i < reqCount; i++ ) {
		pTimerReq = (PTimer_Request)temp;
		MV_ZeroMemory(pTimerReq, sizeof(Timer_Request));
		This->Running_Requests[i] = pTimerReq;
		temp += sizeof( Timer_Request );
	}

	return (MV_PU8)temp;
#else
	return pool;
#endif
}

void Timer_Stop(MV_PVOID This)
{
#ifdef SUPPORT_TIMER
	PHBA_Extension pHBA = (PHBA_Extension)This;

	ScsiPortNotification( RequestTimerCall,
			  pHBA->Device_Extension,
			  Timer_CheckRequest,
			  0 );
#endif
}

#ifdef SUPPORT_TIMER

#ifdef SUPPORT_SMALL_TIMER
MV_U16 Timer_AddSmallRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	)
{
	PHBA_Extension pHBA = (PHBA_Extension)Module_GetHBAExtension(extension);
	PTimer_Module pTimer = &pHBA->Timer_Module;
	PTimer_Request pTimerReq;
	MV_U16 index;
#ifdef ATHENA_PERFORMANCE_TUNNING
	MV_ULONG flags;
	OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
	if (!Tag_IsEmpty(&pTimer->Tag_Pool)) {
		//KeAcquireSpinLockAtDpcLevel(&pTimer->timer_SpinLock);
		index = Tag_GetOne( &pTimer->Tag_Pool );
		OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
		//KeReleaseSpinLockFromDpcLevel(&pTimer->timer_SpinLock);
		pTimerReq = pTimer->Running_Requests[index];

		pTimerReq->Valid = MV_TRUE;
		pTimerReq->Context1 = context1;
		pTimerReq->Context2 = context2;
		pTimerReq->Routine = routine;
		if (pHBA->Device_Id == DEVICE_ID_948F) {
			pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
				time_unit * TIMER_INTERVAL_SMALL_UNIT_FLORENCE );
		} else {
			pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
				time_unit * TIMER_INTERVAL_SMALL_UNIT );
		}
		return index;
	}
	OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
#else
	if (!Tag_IsEmpty(&pTimer->Tag_Pool)) {
		index = Tag_GetOne( &pTimer->Tag_Pool );
		pTimerReq = pTimer->Running_Requests[index];

		pTimerReq->Valid = MV_TRUE;
		pTimerReq->Context1 = context1;
		pTimerReq->Context2 = context2;
		pTimerReq->Routine = routine;
		if (pHBA->Device_Id == DEVICE_ID_948F) {
			pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
				time_unit * TIMER_INTERVAL_SMALL_UNIT_FLORENCE );
		} else {
			pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
				time_unit * TIMER_INTERVAL_SMALL_UNIT );
		}
		return index;
	}
#endif
	MV_DASSERT( MV_FALSE );
	return NO_CURRENT_TIMER;
}
#endif

MV_U16 Timer_AddRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	)
{
	PHBA_Extension pHBA = (PHBA_Extension)Module_GetHBAExtension(extension);
	PTimer_Module pTimer = &pHBA->Timer_Module;
	PTimer_Request pTimerReq;
	MV_U16 index;
#ifdef ATHENA_PERFORMANCE_TUNNING
	MV_ULONG flags;
	OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
	if (!Tag_IsEmpty( &pTimer->Tag_Pool)) {
		//KeAcquireSpinLockAtDpcLevel(&pTimer->timer_SpinLock);
		index = Tag_GetOne( &pTimer->Tag_Pool );
		OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
		//KeReleaseSpinLockFromDpcLevel(&pTimer->timer_SpinLock);
		pTimerReq = pTimer->Running_Requests[index];

		pTimerReq->Valid = MV_TRUE;
		pTimerReq->Context1 = context1;
		pTimerReq->Context2 = context2;
		pTimerReq->Routine = routine;
		pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
			time_unit * TIMER_INTERVAL_LARGE_UNIT );
		return index;
	}
	OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
#else
	if (!Tag_IsEmpty( &pTimer->Tag_Pool)) {
		index = Tag_GetOne( &pTimer->Tag_Pool );
		pTimerReq = pTimer->Running_Requests[index];

		pTimerReq->Valid = MV_TRUE;
		pTimerReq->Context1 = context1;
		pTimerReq->Context2 = context2;
		pTimerReq->Routine = routine;
		pTimerReq->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp,
			time_unit * TIMER_INTERVAL_LARGE_UNIT );
		return index;
	}
#endif
	/* shouldn't happen - we should always allocate enough timer slots for all devices */
	MV_DASSERT( MV_FALSE );
	return NO_CURRENT_TIMER;
}

void Timer_CheckRequest(
	IN MV_PVOID DeviceExtension
	)
{
	PHBA_Extension pHBA = (PHBA_Extension)
		Module_GetHBAExtensionFromDeviceExtension(DeviceExtension);
	PTimer_Module pTimer = &pHBA->Timer_Module;
	PTimer_Request pTimerReq;
	MV_U16 i;
#ifdef ATHENA_PERFORMANCE_TUNNING
	MV_ULONG flags;
#endif
	if (pHBA->State == DRIVER_STATUS_STOP) {
		MV_DPRINT(("Do not check timer request since module stop already\n"));
		return;
	}
#ifdef ATHENA_PERFORMANCE_TUNNING
	OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
	pTimer->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp, TIMER_INTERVAL_OS);
	OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
	for (i = 0; i < pTimer->Timer_Request_Number; i++) {
		OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
		pTimerReq = pTimer->Running_Requests[i];
#if defined(BREAK_TEST)
		if((gBreak==3)&&pTimerReq->Valid&&(pTimerReq->Context1!=pTimerReq->Context2)) {
			if((pTimerReq->Time_Stamp.value&0x07)>=0x01)
				pTimerReq->Time_Stamp.value = pTimer->Time_Stamp.value;
		}
#endif
		if(pTimerReq != NULL && pTimerReq->Valid && (pTimerReq->Time_Stamp.value <= pTimer->Time_Stamp.value)) {
			MV_DASSERT( pTimerReq->Routine != NULL );
			OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
			pTimerReq->Routine( pTimerReq->Context1, pTimerReq->Context2 );
			OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
			if(pTimerReq->Valid) {
				pTimerReq->Valid = MV_FALSE;
				//KeAcquireSpinLockAtDpcLevel(&pTimer->timer_SpinLock);
				
				Tag_ReleaseOne( &pTimer->Tag_Pool, i );
				//KeReleaseSpinLockFromDpcLevel(&pTimer->timer_SpinLock);

			}
			OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
		}else{
			OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
		}
	}
#else
	pTimer->Time_Stamp = U64_ADD_U32(pTimer->Time_Stamp, TIMER_INTERVAL_OS);

	for (i = 0; i < pTimer->Timer_Request_Number; i++) {
		pTimerReq = pTimer->Running_Requests[i];
#if defined(BREAK_TEST)
		if((gBreak==3)&&pTimerReq->Valid&&(pTimerReq->Context1!=pTimerReq->Context2)) {
			if((pTimerReq->Time_Stamp.value&0x07)>=0x01)
				pTimerReq->Time_Stamp.value = pTimer->Time_Stamp.value;
		}
#endif
		if(pTimerReq != NULL && pTimerReq->Valid && (pTimerReq->Time_Stamp.value <= pTimer->Time_Stamp.value)) {
			MV_DASSERT( pTimerReq->Routine != NULL );
			pTimerReq->Routine( pTimerReq->Context1, pTimerReq->Context2 );
			if(pTimerReq->Valid) {
				pTimerReq->Valid = MV_FALSE;
				Tag_ReleaseOne( &pTimer->Tag_Pool, i );
			}
		}
	}
#endif
	if (pHBA->Device_Id == DEVICE_ID_948F) {
		ScsiPortNotification( RequestTimerCall,
				  pHBA->Device_Extension,
				  Timer_CheckRequest,
				  TIMER_INTERVAL_OS_FLORENCE * 1000 );
	} else {	
		ScsiPortNotification( RequestTimerCall,
				  pHBA->Device_Extension,
				  Timer_CheckRequest,
				  TIMER_INTERVAL_OS * 1000 );
	}
#ifdef CORE_NO_RECURSIVE_CALL
	{
		extern MV_VOID core_push_queues(MV_PVOID core_p);
		MV_PVOID core = pHBA->Module_Manage.resource[MODULE_CORE].module_extension;
		core_push_queues(core);
	}
#endif
}

void Timer_CancelRequest(
	IN MV_PVOID extension,
	IN MV_U16 request_index
	)
{
	PHBA_Extension pHBA = (PHBA_Extension)Module_GetHBAExtension(extension);
	PTimer_Module pTimer = &pHBA->Timer_Module;
	PTimer_Request pTimerReq;
#ifdef ATHENA_PERFORMANCE_TUNNING
	MV_ULONG flags;
	OSSW_SPIN_LOCK(&pTimer->timer_SpinLock, flags);
	if(request_index < pTimer->Timer_Request_Number) {
		pTimerReq = pTimer->Running_Requests[request_index];

		if(pTimerReq->Valid && (pTimerReq->Time_Stamp.value >=  pTimer->Time_Stamp.value)) {
			pTimerReq->Valid = MV_FALSE;
			Tag_ReleaseOne( &pTimer->Tag_Pool, (MV_U16)request_index );
		}
	}
	OSSW_SPIN_UNLOCK(&pTimer->timer_SpinLock, flags);
#else
	if(request_index < pTimer->Timer_Request_Number) {
		pTimerReq = pTimer->Running_Requests[request_index];

		if(pTimerReq->Valid && (pTimerReq->Time_Stamp.value >=  pTimer->Time_Stamp.value)) {
			pTimerReq->Valid = MV_FALSE;
			Tag_ReleaseOne( &pTimer->Tag_Pool, (MV_U16)request_index );
		}
	}
#endif
}
#endif
