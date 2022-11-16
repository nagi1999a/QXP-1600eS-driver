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

#if !defined( _MVF_COMMON_INTERFACE_H_ )
#define _MVF_COMMON_INTERFACE_H_

#include "mvflash.h"

typedef struct _SupportDevType
{
    MV_U16  DevId;
    MV_U8   Type;
}SupportDevType;
/**********************************************************************************/
#define         SDT_TYPE_Unknown     0

#if !defined( SUPPORT_SAMSUNG  )
	#if defined( _MV88RC8XXX_ )
	#define         SDT_TYPE_Loki        3
	#define         SDT_TYPE_Vili        4
	#elif defined( _MVFLASH_ )
	#define         SDT_TYPE_Thor        1
	#define         SDT_TYPE_Odin        2
	#define         SDT_TYPE_Loki        3
	#define         SDT_TYPE_Vili        4
	#else
	#error          "Please define support types!!\n"
	#endif
#else
	#define         SDT_TYPE_FREY        5
//	#define         SUPPORT_SAMSUNG  1
#endif
/**********************************************************************************/
#define         Flash_IsType( ai, t )       ( Flash_GetSupportDevType(ai->devId)->Type==SDT_TYPE_##t )
/**********************************************************************************/
static MV_U32 inline Flash_GetSectAddr(FlashInfo *pFI, MV_U32 addr)
{
    return MVF_ALIGN( addr, pFI->SectSize );
}

int
Flash_CheckSkipWrite
(
    void       *Data,
    void        *Buf,
    MV_U32      Count
);
int
Flash_CheckAreaErased
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void       *Data, 
    MV_U32      Count
);
int
Flash_AssignAdapterIf
(
    AdapterInfo *pAI
);
SupportDevType*
Flash_GetSupportDevType
(
    MV_U16 DevId
);
SupportDevType const *Flash_GetSupportDevTypeBySeq(int Seq);
FlashInfo* Flash_CheckDecodeArea( AdapterInfo *pAI, MV_U32 addr, MV_U32 count);

#endif
