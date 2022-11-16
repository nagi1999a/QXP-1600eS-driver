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

#ifndef __MV_COM_REQUEST_DETAIL_H__
#define __MV_COM_REQUEST_DETAIL_H__

//request related each structure, remove these in the future.
//deprecated.
#include "com_pd_struct.h"
#include "com_enc_struct.h"
#include "com_vd_struct.h"
#include "com_raid_struct.h"
#include "com_array_struct.h"
#include "com_adapter_struct.h"

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _HD_Info_Request
{
    RequestHeader header;
    HD_Info  hdInfo[1];			// Application need to allocate enough space based on numRequested?in the header.
								// For example, if numRequested?is 3, application should allocate 
								// (sizeof(HD_Info_Request) + (3-1) * sizeof(HD_Info)) of space
								// for HD_Info_Request.  Driver can fill max of 3 entries starting from hdInfo[0].
} HD_Info_Request, *PHD_Info_Request;

typedef struct _HD_FreeSpaceInfo_Request
{
    RequestHeader header;
    HD_FreeSpaceInfo  hdFreeSpaceInfo[1];
} HD_FreeSpaceInfo_Request, *PHD_FreeSpaceInfo_Request;

typedef struct _HD_Config_Request
{
    RequestHeader header;
    HD_Config  hdConfig[1];
} HD_Config_Request, *PHD_Config_Request;

typedef struct _HD_SMART_Status_Request
{
    RequestHeader header;
    HD_SMART_Status  hdSmartStatus[1];
} HD_SMART_Status_Request, *PHD_SMART_Status_Request;

typedef struct _HD_Block_Info_Request
{
    RequestHeader header;
    HD_Block_Info  hdBlockInfo[1];
} HD_Block_Info_Request, *PHD_Block_Info_Request;

#ifndef _MARVELL_SDK_PACKAGE_NONRAID

typedef struct _HD_RAID_Status_Request
{
    RequestHeader header;
    HD_RAID_Status  hdRaidStatus[1];
} HD_RAID_Status_Request, *PHD_RAID_Status_Request;

typedef struct _HD_BGA_Status_Request
{
    RequestHeader header;
    HD_BGA_Status  hdBgaStatus[1];
} HD_BGA_Status_Request, *PHD_BGA_Status_Request;

typedef struct _Block_Info_Request
{
    RequestHeader header;
    Block_Info  blockInfo[1];
} Block_Info_Request, *PBlock_Info_Request;

typedef struct _LD_Info_Request
{
    RequestHeader header;
    LD_Info  ldInfo[1];
} LD_Info_Request, *PLD_Info_Request;

typedef struct _LD_Status_Request
{
    RequestHeader header;
    LD_Status  ldStatus[1];
} LD_Status_Request, *PLD_Status_Request;

typedef struct _LD_Config_Request
{
    RequestHeader header;
    LD_Config  ldConfig[1];
} LD_Config_Request, *PLD_Config_Request;

typedef struct _DG_Info_Request
{
    RequestHeader header;
    DG_Info  dgInfo[1];
} DG_Info_Request, *PDG_Info_Request;

typedef struct _DG_Config_Request
{
    RequestHeader header;
    DG_Config  dgConfig[1];
} DG_Config_Request, *PDG_Config_Request;

typedef struct _RCT_Record_Request
{
    RequestHeader header;
    RCT_Record  rctRecord[1];
} RCT_Record_Request, *PRCT_Record_Request;
#endif
//_MARVELL_SDK_PACKAGE_NONRAID

// Port Multiplexier 
typedef struct _PM_Info_Request
{
    RequestHeader header;
    PM_Info  pmInfo[1];
} PM_Info_Request, *PPM_Info_Request;

// Expander 
typedef struct _Exp_Info_Request
{
    RequestHeader header;
    Exp_Info  expInfo[1];
} Exp_Info_Request, *PExp_Info_Request;

typedef struct _Enclosure_Info_Request
{
    RequestHeader header;
    Enclosure_Info  encInfo[1];
} Enclosure_Info_Request, *PEnclosure_Info_Request;

typedef struct _EncElementType_Info_Request
{
    RequestHeader header;
    EncElementType_Info  encEleTypeInfo[1];
} EncElementType_Info_Request, *PEncElementType_Info_Request;

typedef struct _EncElement_Config_Request
{
    RequestHeader header;
    EncElement_Config  encEleConfig[1];
} EncElement_Config_Request, *PEncElement_Config_Request;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
