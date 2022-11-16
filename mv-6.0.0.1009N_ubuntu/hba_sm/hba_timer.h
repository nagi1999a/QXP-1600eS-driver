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

#if !defined(TIMER_H)

#define TIMER_H

#include "mv_include.h"
#include "com_tag.h"

#ifndef SUPPORT_SMALL_TIMER
	#ifndef KERNEL_SIMULATION
	#define TIMER_INTERVAL_LARGE_UNIT	500	/* millisecond */
	#else
	#define TIMER_INTERVAL_LARGE_UNIT	1	/* millisecond */
	#endif
	#define TIMER_INTERVAL_OS		TIMER_INTERVAL_LARGE_UNIT
#else
	#define TIMER_INTERVAL_OS			100	/* millisecond */
	#define TIMER_INTERVAL_OS_FLORENCE		10
	#define TIMER_INTERVAL_LARGE_UNIT		500	/* millisecond */
	#define TIMER_INTERVAL_SMALL_UNIT		100	/* millisecond */
	#define TIMER_INTERVAL_SMALL_UNIT_FLORENCE	10 /* millisecond */
#endif

#define NO_CURRENT_TIMER		0xffff

typedef struct _Timer_Request
{
	List_Head Queue_Pointer;
	MV_PVOID Context1;
	MV_PVOID Context2;
	MV_PVOID Reserved0;
	VOID (*Routine) (MV_PVOID, MV_PVOID);
	MV_BOOLEAN Valid;
	MV_U8 Reserved1[3];
	MV_U64 Time_Stamp;		// when this requested function wants to be called
} Timer_Request, *PTimer_Request;

#ifdef SUPPORT_TIMER

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
#if defined(_OS_WINDOWS) && defined(ATHENA_PERFORMANCE_TUNNING)
	KSPIN_LOCK timer_SpinLock;
#endif
} Timer_Module, *PTimer_Module;

#else

typedef struct _Timer_Module
{
	MV_PVOID context;
	VOID (*routine) (MV_PVOID);
} Timer_Module, *PTimer_Module;

#endif

/*
 * Exposed functions
 */
MV_U32 Timer_GetResourceQuota(MV_U16 maxIo);

void Timer_Stop(MV_PVOID This);

MV_PU8 Timer_Initialize(
	IN OUT PTimer_Module This,
	IN MV_PU8 pool,
	IN MV_U16 maxIo
	);

#ifdef SUPPORT_SMALL_TIMER
MV_U16 Timer_AddSmallRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);
#else
#define Timer_AddSmallRequest Timer_AddRequest
#endif

MV_U16 Timer_AddRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);

void Timer_CheckRequest(
	IN MV_PVOID extension
	);

void Timer_CancelRequest(
	IN MV_PVOID extension,
	IN MV_U16 request_index
	);

#endif /* TIMER_H */
