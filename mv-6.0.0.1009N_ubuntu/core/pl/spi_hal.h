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

#if !defined( _SPI_HAL_H_ )
#define _SPI_HAL_H_

#include "core_header.h"
#if defined( _MVFLASH_ )
#if defined( __DOS__ )
#include "pcibios.h"
#define FMemR8( addr )          read8bitMemBar( addr )
#define FMemR16( addr )         read16bitMemBar( addr )
#define FMemR32( addr )         read32bitMemBar( addr )
#define FMemW8( addr, var )     write8bitMemBar( addr, var )
#define FMemW16( addr, var )    write16bitMemBar( addr, var )
#define FMemW32( addr, var )    write32bitMemBar( addr, var )
#else
#define FMemR8( addr )          (*( (volatile MV_U8*) (addr) ))
#define FMemR16( addr )         (*( (volatile MV_U16*)(addr) ))
#define FMemR32( addr )         (*( (volatile MV_U32*)(addr) ))
#define FMemW8( addr, var )     (*( (volatile MV_U8*) (addr) ) = (MV_U8)var )
#define FMemW16( addr, var )    (*( (volatile MV_U16*)(addr) ) = (MV_U16)var )
#define FMemW32( addr, var )    (*( (volatile MV_U32*)(addr) ) = (MV_U32)var )
#endif
#elif defined(_OS_WINDOWS) || defined(_OS_LINUX) || defined(__QNXNTO__) || defined(_OS_FIRMWARE)
#define FMemR8( addr, off  )         MV_REG_READ_BYTE( addr, off )
#define FMemR16( addr, off )         MV_REG_READ_WORD( addr, off )
#define FMemR32( addr, off )         MV_REG_READ_DWORD( addr, off )
#define FMemW8( addr, off, var )     MV_REG_WRITE_BYTE( addr, off, var )
#define FMemW16( addr, off, var )    MV_REG_WRITE_WORD( addr, off, var )
#define FMemW32( addr, off, var )    MV_REG_WRITE_DWORD( addr, off, var )
#define MV_IOR8( addr, off )         MV_IO_READ_BYTE( addr, off )
#define MV_IOR16( addr, off )        MV_IO_READ_WORD( addr, off )
#define MV_IOR32( addr, off )        MV_IO_READ_DWORD( addr, off )
#define MV_IOW8( addr, off, data )   MV_IO_WRITE_BYTE( addr, off, data )
#define MV_IOW16( addr, off, data )  MV_IO_WRITE_WORD( addr, off, data )
#define MV_IOW32( addr, off, data )  MV_IO_WRITE_DWORD( addr, off, data )
#endif

#ifdef _MVFLASH_
void
Delayms
(
    unsigned msec
);
void
Delayus
(
    unsigned long usec
);
#   define DelayMSec( _X_ )        Delayms( _X_ )
#   define DelayUSec( _X_ )        Delayus( _X_ )
#elif defined(_OS_WINDOWS) || defined(__QNXNTO__) || defined(_OS_LINUX) || defined(_OS_FIRMWARE)
#   define DelayMSec(_X_)        HBA_SleepMillisecond(NULL, _X_)
#   define DelayUSec(_X_)        HBA_SleepMicrosecond(NULL, _X_)
#endif /*  _MVFLASH_ */

#endif
