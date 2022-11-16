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

#ifndef  __MV_COM_EXTERN_H__
#define  __MV_COM_EXTERN_H__

#include "com_define.h"
/* Target device type */
#define TARGET_TYPE_LD                          	0
#define TARGET_TYPE_FREE_PD                 	1

#define DISK_TYPE_RAID					0
#define DISK_TYPE_SATA					1
#define DISK_TYPE_SAS					2

/*TCQ queue types*/		
#define TCQ_QUEUE_TYPE_SIMPLE			0x00
#define TCQ_QUEUE_TYPE_HDOFQUEUE		0x01
#define TCQ_QUEUE_TYPE_ORDERED			0x02
#define TCQ_QUEUE_TYPE_ACA				0x04

// Giving TargetID and LUN, returns it Type and DeviceID.  If returned Type or DeviceID is 0xFF, not found.
typedef struct    _TargetLunType
{
 MV_U8            AdapterID;
 MV_U8            TargetID;
 MV_U8            Lun;
 MV_U8            Type;            // TARGET_TYPE_LD or TARGET_TYPE_FREE_PD
 MV_U16           DeviceID;        // LD ID or PD ID depends on Type
 MV_U8            Reserved[34];
}TargetLunType, * PTargetLunType;

typedef struct	_OS_disk_info
{
	MV_U8 		ataper_id;	/* ataper disk locates */
	MV_U8 		disk_type;	/* RAID disk, SATA disk or SAS disk */
	MV_U8		queue_type; /*tcq queue types*/
	MV_U16		device_id;	/* contain target id and lun */
	MV_U16		queue_depth;	/* queue depth support this disk */
	MV_U32		capacity;	/* queue depth support this disk */
}OS_disk_info, *POS_disk_info;

#ifdef VANIR_PRODUCT
/* domain device capability */
enum device_capability {
	DEVICE_CAPABILITY_48BIT_SUPPORTED                   = (1U << 0),
	DEVICE_CAPABILITY_SMART_SUPPORTED                   = (1U << 1),
	DEVICE_CAPABILITY_WRITECACHE_SUPPORTED              = (1U << 2),
	DEVICE_CAPABILITY_NCQ_SUPPORTED                     = (1U << 3),
	DEVICE_CAPABILITY_RATE_1_5G                         = (1U << 4),
	DEVICE_CAPABILITY_RATE_3G                           = (1U << 5),
	DEVICE_CAPABILITY_RATE_6G                           = (1U << 6),
	DEVICE_CAPABILITY_READLOGEXT_SUPPORTED              = (1U << 7),
	DEVICE_CAPABILITY_READ_LOOK_AHEAD_SUPPORTED         = (1U << 8),
	DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED         = (1U << 9),
	DEVICE_CAPABILITY_PROTECTION_INFORMATION_SUPPORTED  = (1U << 10),
	DEVICE_CAPABILITY_POIS_SUPPORTED = (1U << 11),
	DEVICE_CAPABILITY_TRIM_SUPPORTED  = (1U << 12),
	DEVICE_CAPABILITY_SSD  = (1U << 13),
};
#endif

#endif
