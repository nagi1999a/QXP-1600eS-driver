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

#if !defined( _MVF_HAL_H_ )
#define _MVF_HAL_H_

#if defined( _MVFLASH_ )

#if defined( __DOS__ )
#include "pcibios.h"
#define MV_MMR8( addr, off )             read8bitMemBar( ((MV_U32)addr)+(off) )
#define MV_MMR16( addr, off )            read16bitMemBar( ((MV_U32)addr)+(off) )
#define MV_MMR32( addr, off )            read32bitMemBar( ((MV_U32)addr)+(off) )
#define MV_MMW8( addr, off, var )        write8bitMemBar( ((MV_U32)addr)+(off), var )
#define MV_MMW16( addr, off, var )       write16bitMemBar( ((MV_U32)addr)+(off), var )
#define MV_MMW32( addr, off, var )       write32bitMemBar( ((MV_U32)addr)+(off), var )
#define MV_IOR8( addr, off )            inp( (MV_U16)(addr)+(off) )
#define MV_IOR16( addr, off )           inpw( (MV_U16)(addr)+(off) )
#define MV_IOR32( addr, off )           read32bitIoBar( (MV_U16)addr, off )
#define MV_IOW8( addr, off, data )      outp( (MV_U16)(addr)+(off), data )
#define MV_IOW16( addr, off, data )     outpw( (MV_U16)(addr)+(off), data )
#define MV_IOW32( addr, off, data )     write32bitIoBar( (MV_U16)addr, off, data )
#define MV_REG_READ( off )              MV_MMR32( off, 0 )
#define MV_REG_WRITE( off, data )       MV_MMW32( off, 0, data )

#else

#define MV_MMR8( addr, off )            (*( (volatile MV_U8*) ((char*)(addr)+(off))))
#define MV_MMR16( addr, off )           (*( (volatile MV_U16*)((char*)(addr)+(off))))
#define MV_MMR32( addr, off )           (*( (volatile MV_U32*)((char*)(addr)+(off))))
#define MV_MMW8( addr, off, var )       MV_MMR8( addr, off ) = (MV_U8)var
#define MV_MMW16( addr, off, var )      MV_MMR16( addr, off ) = (MV_U16)var
#define MV_MMW32( addr, off, var )      MV_MMR32( addr, off ) = (MV_U32)var
#define MV_IOR8( addr, off )            inp( (addr)+(off) )
#define MV_IOR16( addr, off )           inpw( (addr)+(off) )
#define MV_IOR32( addr, off )           inpd( (addr)+(off) )
#define MV_IOW8( addr, off, data )      outp( (addr)+(off), data )
#define MV_IOW16( addr, off, data )     outpw( (addr)+(off), data )
#define MV_IOW32( addr, off, data )     outpd( (addr)+(off), data )
#define MV_REG_READ( off )              MV_MMR32( off, 0 )
#define MV_REG_WRITE( off, data )       MV_MMW32( off, 0, data )

#endif

#elif defined( _MV_MARVELL_HAL_ )

#define MV_MMR8( addr, off  )           MV_REG_READ_BYTE( addr, off )
#define MV_MMR16( addr, off )           MV_REG_READ_WORD( addr, off )
#define MV_MMR32( addr, off )           MV_REG_READ_DWORD( addr, off )
#define MV_MMW8( addr, off, var )       MV_REG_WRITE_BYTE( addr, off, var )
#define MV_MMW16( addr, off, var )      MV_REG_WRITE_WORD( addr, off, var )
#define MV_MMW32( addr, off, var )      MV_REG_WRITE_DWORD( addr, off, var )
#define MV_REG_READ( off )              (*(MV_U32 volatile *)(INTER_REGS_BASE+(off)))
#define MV_REG_WRITE( off, data )       MV_REG_READ( off )=(data)
#define MV_REG_SET_BIT( off, m, b )     MVF_SET_BIT( MV_REG_READ( off ), m, b );
#define DelayMSec( _X_ )        HBA_SleepMillisecond( NULL, _X_ )
#define DelayUSec( _X_ )        HBA_SleepMicrosecond( NULL, _X_ )

#elif defined( _MV_MMIO_ )

#define MV_MMR8( addr, off )            (*( (volatile MV_U8*) ((char*)(addr)+(long)(off))))
#define MV_MMR16( addr, off )           (*( (volatile MV_U16*)((char*)(addr)+(long)(off))))
#define MV_MMR32( addr, off )           (*( (volatile MV_U32*)((char*)(addr)+(long)(off))))
#define MV_MMW8( addr, off, var )       MV_MMR8( addr, off ) = (MV_U8)var
#define MV_MMW16( addr, off, var )      MV_MMR16( addr, off ) = (MV_U16)var
#define MV_MMW32( addr, off, var )      MV_MMR32( addr, off ) = (MV_U32)var
#ifdef MV_REG_READ
#undef MV_REG_READ
#undef MV_REG_WRITE
#endif
#define MV_REG_READ( off )              MV_MMR32( INTER_REGS_BASE, off )
#define MV_REG_WRITE( off, data )       MV_REG_READ( off )=(data)
#endif
#ifdef MV_REG_SET_BIT
#undef MV_REG_SET_BIT
#endif
#define MV_REG_SET_BIT( off, m, b )     \
do {MV_U32 volatile *p = (MV_U32*)(INTER_REGS_BASE+(off));\
    MVF_SET_BIT(*p, m, b );}while(0);

#if defined( _MVFLASH_ )
    void Delayms(unsigned msec);
    void Delayus(unsigned long usec);
#   define DelayMSec( _X_ )        Delayms( _X_ )
#   define DelayUSec( _X_ )        Delayus( _X_ )
#elif defined(_OS_UKRN)
#   define DelayUSec( _X_ )        pl_udelay( _X_ )
#   define DelayMSec( _X_ )        pl_mdelay( _X_ )
#elif defined( __GNUC__ )
#   define DelayUSec( _X_ )        udelay( _X_ )
#   define DelayMSec( _X_ )        DelayUSec( (_X_)*1000 )
#endif

#endif
