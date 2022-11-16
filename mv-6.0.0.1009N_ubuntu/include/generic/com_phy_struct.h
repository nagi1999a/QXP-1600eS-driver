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

#ifndef __MV_COM_PHY_STRUCT_H__
#define __MV_COM_PHY_STRUCT_H__

#include "com_define.h"

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _NEW_PHY_TUNING {
#ifdef __MV_BIG_ENDIAN__
         MV_U8    Trans_Emphasis_Amp:4;                   /* 4 bits, transmitter emphasis amplitude */
         MV_U8    Reserved_bit_1:3;                       /* 3 bits, reserved space */
         MV_U8    Trans_Emphasis_En:1;                    /* 1 bit,  transmitter emphasis enable  */
#else
         MV_U8    Trans_Emphasis_En:1;                    /* 1 bit,  transmitter emphasis enable  */
         MV_U8    Reserved_bit_1:3;                       /* 3 bits, reserved space */
         MV_U8    Trans_Emphasis_Amp:4;                   /* 4 bits, transmitter emphasis amplitude */
#endif

#ifdef __MV_BIG_ENDIAN__
         MV_U8    Reserved_bit_2:3;                       /* 3 bits, reserved space */
         MV_U8    Trans_Amp:5;                            /* 5 bits, transmitter amplitude */
#else
         MV_U8    Trans_Amp:5;                            /* 5 bits, transmitter amplitude */
         MV_U8    Reserved_bit_2:3;                       /* 3 bits, reserved space */
#endif

#ifdef __MV_BIG_ENDIAN__
         MV_U8    Reserved_bit_3:6;                       /* 6 bit,    reserved space */
         MV_U8    Trans_Amp_Adjust:2;                     /* 2 bits, transmitter amplitude adjust */
#else
         MV_U8    Trans_Amp_Adjust:2;                     /* 2 bits, transmitter amplitude adjust */
         MV_U8    Reserved_bit_3:6;                       /* 6 bit,    reserved space */
#endif         
         MV_U8    Reserved;                               /* 1 bytes, reserved space */
} NEW_PHY_TUNING, *PNEW_PHY_TUNING;

typedef struct _PHY_TUNING_CONFIG {
         NEW_PHY_TUNING Value;
         MV_U8    Gen_Speed;                              /* 1: Gen1; 2:Gen2; 3: Gen3 */
         MV_U8    reserved[3];
} MV_PHY_TUNING_CONFIG;

typedef union {
    struct {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];        
} MV_SAS_ADDR;

typedef union {
    MV_PHY_TUNING_CONFIG tuning;
    MV_SAS_ADDR          sas; 
} PHY_CONFIG, *PPHY_CONFIG;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif
