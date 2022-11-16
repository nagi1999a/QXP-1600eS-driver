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

#ifndef __MV_COM_DBG_STRUCT_H__
#define __MV_COM_DBG_STRUCT_H__

#include "com_define.h"

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

// Type of request for which the error should trigger.
#define DBG_REQUEST_READ                        MV_BIT(0)
#define DBG_REQUEST_WRITE                       MV_BIT(1)
#define DBG_REQUEST_VERIFY                      MV_BIT(2)


// The following data structure is dynamically allocated.  Depends on
// NumSectors, the total size of the data structure should be
// (8 + 4 + (SECTOR_LENGTH * NumSectors))
// Where 8 is the size of LBA and 4 is the size of NumSectors itself.
typedef struct _DBG_DATA
{
	MV_U64            LBA;
	MV_U32            NumSectors;
	MV_U8             Data[1];
}DBG_Data, *PDBG_Data;

typedef struct _DBG_HD
{
	MV_U64            LBA;
	MV_U16            HDID;
	MV_BOOLEAN        isUsed;
	MV_U8             Reserved[5];
}DBG_HD;

typedef struct _DBG_FLASH
{
	MV_U32            OffSet;
	MV_U16            NumBytes;
	MV_U8             Data[1];
}DBG_Flash, *PDBG_Flash;

typedef struct _DBG_NVSRAM
{
	MV_U32     Offset;
	MV_U16     NumBytes;
	MV_U16     ReturnedCount;
	MV_U8      Reserved[8];
	MV_U8      Data[256];
}DBG_NVSRAM, *PDBG_NVSRAM;

// Map to/from VD LBA to/from PD LBA
typedef struct _DBG_MAP
{
	MV_U64            VD_LBA;
	MV_U64            PD_LBA;
	MV_U16            VDID;          // if 'mapDirection' is DBG_PD2VD, set it to 0xFFFF as input and driver will return the mapped VD ID
	MV_U16            PDID;          // must specified in any case
	MV_BOOLEAN        parity;        // [out](true or false) if 'mapDirection' is DBG_VD2PD, this tells if the specified PD is a parity disk or not.
	MV_U8             mapDirection;  // DBG_VD2PD or DBG_PD2VD
	MV_U8             Reserved[34];
}DBG_Map, *PDBG_Map;

typedef struct _DBG_Error_Injection
{
	MV_U64     LBA;
	MV_U32     Count;
	MV_U16     HDID;
	MV_U8      Error_case;
	MV_U8      Error_Status;
	MV_U8      Request_Type;
	MV_U8      Sense_idx;
	MV_U8      Reserved[6];
}DBG_Error_Injection, *PDBG_Error_Injection;

#define DBG_VD2PD                               0
#define DBG_PD2VD                               1

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
