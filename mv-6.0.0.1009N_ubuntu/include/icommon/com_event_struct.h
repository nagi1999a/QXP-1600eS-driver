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

#ifndef COM_EVENT_DRIVER_H
#define COM_EVENT_DRIVER_H

#ifndef COM_DEFINE_H //for magni not use include com_define to define MV_U32
#include "com_define.h"
#endif

#define MAX_EVENTS                      20
#define MAX_EVENTS_WAITED	  	16
#define MAX_EVENT_PARAMS                4
#define MAX_EVENTS_RETURNED             6
#define MAX_EVENT_SENSE_DATA_COUNT	30

#ifndef _OS_BIOS
#pragma pack(8)
#endif /*  _OS_BIOS */

#if defined(_OS_FIRMWARE)
typedef struct _HotPlugEvent
{
	MV_U16	size;
	MV_U8	dummy[2];
	MV_U8	bitmap[0];
}HotPlugEvent;
#endif

typedef struct _DriverEvent
{
	MV_U32  TimeStamp;
	MV_U32  SequenceNo; /* (contiguous in a single adapter) */
	MV_U32  EventID;    /* 1st 16 bits - Event class */
                        /* last 16 bits - Event code of this particula Event class */
	MV_U8   Severity;
	MV_U8   AdapterID; /*For LokiPlus, the byte will use to check whether event is vaild or not. If yes, it will be added to systme event log.*/
	MV_U16  DeviceID;   /* Device ID relate to the event class (HD ID, LD ID etc) */
	MV_U32  Params[MAX_EVENT_PARAMS]; /* Additional information if ABSOLUTELY necessary. */
} DriverEvent, * PDriverEvent;

typedef struct _EventRequest
{
	MV_U8        Count; /* [OUT] # of actual events returned */
	MV_U8        Reserved[3];
	DriverEvent  Events[MAX_EVENTS_RETURNED];
} EventRequest, * PEventRequest;

// Event support sense code
typedef struct _DriverEvent_V2
{
    DriverEvent  Event;                /* same as the current one */
    MV_U8        SenseDataLength;      /* actual length of SenseData.  Driver set it to 0 if no SenseData */
    MV_U8        accurated;
    MV_U8        SenseData[30];        /* (24+6) just for making this structure on 64-bits boundary */
} DriverEvent_V2, * PDriverEvent_V2;

typedef struct _EventRequest_V2
{
    MV_U8           Count; /* [OUT] # of actual events returned */
    MV_U8           Reserved[3];
    DriverEvent_V2  Events[MAX_EVENTS_RETURNED];
} EventRequest_V2, * PEventRequest_V2;


#define EVENTL0G_HEAD_SIGNATURE                "TLEM"
#define EVENTLOG_HEAD_RESERVED_BYTES      16
#define EVENTLOG_ENTRY_VALID                        0x00000001L

#if defined(SUPPORT_NVRAM) && defined(_OS_FIRMWARE)
#define NVRAM_START_OFFSET			0

struct nvram_event_log_head
{
	MV_U8	signature[4];
	MV_U32	next_tbl_off;
	MV_U16	version;
	MV_U16  	nr_max_entries;
	MV_U32  	crc;
	MV_U8  	reserved[EVENTLOG_HEAD_RESERVED_BYTES];
};

struct nvram_event_log
{
	struct nvram_event_log_head head;
	MV_U32	sequence_no;
	MV_U16	valid_slot_start;
	MV_U16	valid_slot_end;
	MV_U32	reserved;
	MV_U32 	log_crc;
};

struct nvram_normal_head
{
	MV_U8	signature[4];
	MV_U32	next_tbl_off;
	MV_U16	version;
	MV_U16  	nr_max_entries;
	MV_U32  	crc;
	MV_U8  	reserved[16];
};
#endif /*SUPPORT_NVRAM*/

#ifndef _OS_BIOS
#pragma pack()
#endif /*  _OS_BIOS */

#endif /*  COM_EVENT_DRIVER_H */
