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

/*******************************************************************************
*
*                   Copyright 2007,MARVELL SEMICONDUCTOR, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND
* SYSKONNECT GMBH.
*
********************************************************************************/

#if !defined(_MVF_TYPE_H_)
#define _MVF_TYPE_H_

#if defined( MV_PTR_INTEGER)
#define CPU_ADDR_BUS_WIDE       16
#else
#define CPU_ADDR_BUS_WIDE       4
#endif

#if defined( _MV_STANDALONE_ )
#if !defined( MV_U8 )&&!defined( __U_BOOT__ )
typedef unsigned char	MV_U8, *MV_PU8,U8,*PU8;
typedef signed char		MV_I8, *MV_PI8,I8,*PI8;

typedef unsigned short	MV_U16, *MV_PU16,U16,*PU16;
typedef signed short	MV_I16, *MV_PI16,I16,*PI16;

typedef unsigned long	MV_U32, *MV_PU32,U32,*PU32;
typedef signed long		MV_I32, *MV_PI32,I32,*PI32;

#if defined( __GNUC__ )
typedef unsigned long long	MV_U64, *MV_PU64,U64,*PU64;
typedef long long	MV_I64, *MV_PI64,I64,*PI64;
#else
typedef unsigned long long	MV_U64, *MV_PU64,U64,*PU64;
typedef signed long	long	MV_I64, *MV_PI64,I64,*PI64;
#endif

#endif

typedef void *          MV_PVOID;


#define MV_INLINE  // inline
#endif

#if CPU_ADDR_BUS_WIDE==2
#define MV_PTR_INTEGER       MV_U16
#elif CPU_ADDR_BUS_WIDE==4
#define MV_PTR_INTEGER       MV_U32
#elif CPU_ADDR_BUS_WIDE==8
#define MV_PTR_INTEGER       MV_U64
#else
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define MVF_TRUE    1
#define MVF_FALSE   0

#define __IN__
#define _OUT__
#define __IO__

typedef union _MV_PTR_X
{
	MV_U8	*pu8;
	MV_U16	*pu16;
	MV_U32	*pu32;
	void	*p;
} MV_PTR_X;


#endif
