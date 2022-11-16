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

#if !defined( _CORE_FLASH_H_ )
#define _CORE_FLASH_H_

#include "mv_config.h"
#include "com_define.h"

#define MVFR_FLAG_AUTO       MV_BIT(0)
#define MVFR_FLAG_ERASE      MV_BIT(1)
#define MVFR_FLAG_CONT       MV_BIT(2)
#define MVFR_FLAG_INIT       MV_BIT(3)
#define MVFR_FLAG_RMW_BUF    MV_BIT(4)
#define MVFR_FLAG_RMW_SECT   MV_BIT(5)

#define MVF_MAX_ID(a, b)        (((a)<(b))?(a):(b))
#define MFR_EEPROM_DEV(a)       ((0+MVF_MAX_ID(a, 8)))
#define MFR_FLASH_DEV(a)        (8+MFR_EEPROM_DEV(a))

#define MVFR_FLAG_GETSIZE_ALL              MV_BIT(0)
#define MVFR_FLAG_GETSIZE_A_BLOCK      MV_BIT(1)
#define MVFR_FLAG_GETSIZE_A_SECTOR    MV_BIT(2)

#define MVFR_OP_READ            0x28
#define MVFR_OP_WRITE          0x2A
#define MVFR_OP_ERASE          0x2C
#define MVFR_OP_CAPACITY    0x25


typedef struct _mvf_req
{
	char        op;
	MV_U32      did;
	MV_U32		off;
	MV_U32		len;
	void*	    buf;
	MV_U32      flags;
	void*       rmw_buf;
}mvf_req;

mvf_req *core_mvf_req_ptr(void);
mvf_req *core_mvf_req(char op,MV_U32 id,MV_U32	off,
	MV_U32 len,void*buf,MV_U32 flags);
int core_flash_init(MV_PVOID This);
int core_nvm_send_req(MV_PVOID This,mvf_req*req);
#define Core_NVMInit core_flash_init
#define NVM_ARG(a)          Core_MVfReqPtr()->(a)
#define Core_NVMLastOp(this)   core_nvm_send_req( this, Core_MvfReqPtr() );
#ifdef SUPPORT_FLASH_ROM
#ifdef _OS_UKRN
#include "precomp.h"
//#include "flash.h"

u8 flash_read(u32 addr, u8 *data, u32 count);
u8 flash_write_word(u32 addr, u16 *data, u32 count, u8 flag);
u8 flash_erase(u32 off, u32 len);
u8 flash_get_info(u32 off, u32 *buf, int capacity);

#define Core_NVMRd(this, did, off, len, buf, flags) \
    flash_read(off, buf, len)
#define Core_NVMWr(this, did, off, len, buf, flags) \    
    flash_write_word(off, buf, len>>1, flags)
#define Core_NVMEs(this, did, off, len, buf, flags) \
    flash_erase(off, len)
#define Core_NVMInfo(this, did, off, len, buf, flags) \
    flash_get_info(off, buf, (flags == MVFR_FLAG_GETSIZE_ALL))
#else
#define Core_NVMRd(this, did, off, len, buf, flags) \
core_nvm_send_req( this, core_mvf_req(MVFR_OP_READ, did, off, len, buf, flags))
#define Core_NVMWr(this, did, off, len, buf, flags) \
core_nvm_send_req( this, core_mvf_req(MVFR_OP_WRITE, did, off, len, buf, flags))
#define Core_NVMEs(this, did, off, len, buf, flags) \
core_nvm_send_req( this, core_mvf_req(MVFR_OP_ERASE, did, off, len, NULL, MVFR_FLAG_ERASE))
#define Core_NVMInfo(this, did, off, len, buf, flags) \
core_nvm_send_req( this, core_mvf_req(MVFR_OP_CAPACITY, did, off, len, buf, flags))
#endif
#else
#define Core_NVMRd(this, did, off, len, buf, flags) MV_FALSE

#define Core_NVMWr(this, did, off, len, buf, flags) MV_FALSE

#define Core_NVMEs(this, did, off, len, buf, flags) MV_FALSE

#define Core_NVMInfo(this, did, off, len, buf, flags) MV_FALSE

#endif

#endif
