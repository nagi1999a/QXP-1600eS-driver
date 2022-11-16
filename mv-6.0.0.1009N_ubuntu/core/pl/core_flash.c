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

#include "mv_config.h"
#ifdef SUPPORT_FLASH_ROM
#include "mv_include.h"
#include "flash_lib/mv_incs.h"
#include "core_manager.h"
#include "core_flash.h"
#include "core_header.h"
//#include "core_helper.h"
#include "hba_inter.h"

#include "flash_lib/mvstdio.c"
#if defined(_MV88RC8XXX_)
#include "flash_lib/mvf_loki.c"
#else
#include "flash_lib/mvf_spi.c"
#endif
#include "flash_lib/mvf_apif.c"

static core_extension *pCore = NULL;
static AdapterInfo AI = { 0, };
static AdapterInfo *pAI = NULL;
static FlashInfo FI[2] = { { 0, }, };
#define RAED_MODIFY_WRITE_BUFFER_SIZE  (128 * 1024 + 8)
MV_U8 ptr_read_modify_write_buffer[RAED_MODIFY_WRITE_BUFFER_SIZE];

#define REINIT_REAL_TIME(This, req)\
do \
{   \
	if ( (pAI==NULL) )  \
		if( 0!=core_flash_init( This ) ) \
			return MVF_FALSE; \
}while(0);


#define INIT_FI_PTR(pfi, req, id_base) \
do{ int dev_id = req->did-id_base;    \
	if (dev_id>=pAI->FlashCount)     \
		return MVF_FALSE;   \
	if( (pfi=pAI->pFI[dev_id]) == NULL )    \
		return MVF_FALSE; }while(0);

#ifdef SUPPORT_ROC
static int core_initAI(FlashInfo * pFI)
{
#if defined(SUPPORT_BALDUR)	//remove? for ROC only
	MV_BoardWindowInfo WI;
	MV_U32 tmp;

	pAI->RC9XXX_REG_INTER_BASE = (void *)ROC_INTER_REGS_BASE;
	tmp = MV_REG_READ_DWORD( pAI->RC9XXX_REG_INTER_BASE, 0x8000 );
	pAI->devId = tmp>>16;
	if (0!=Flash_AssignAdapterIf(pAI))
		return 0;
	pAI->pFI[pAI->FlashCount] = pFI;
	pFI->DevBase = WI.base;
	pFI->flags|= FI_FLAG_MEM_MAPPED;
#if defined( _MV88RC8XXX_ )
	pFI->chip_sel = 3;
#endif

#elif defined( _MV88RC8XXX_ )
	MV_BoardWindowInfo WI;
	MV_U32 tmp;
	
	pAI->RC8XXX_REG_INTER_BASE = ROC_INTER_REGS_BASE;

	RC8XXX_BoardWindowInfoGet(pAI->RC8XXX_REG_INTER_BASE, &WI, 7);
	tmp = MV_REG_READ_DWORD( pAI->RC8XXX_REG_INTER_BASE, 0x30000 );
	pAI->devId = tmp>>16;
	if (0!=Flash_AssignAdapterIf(pAI))
	    return 0;
	pAI->pFI[pAI->FlashCount] = pFI;
	pFI->DevBase = WI.base;
    pFI->flags|= FI_FLAG_MEM_MAPPED;

    pFI->chip_sel = 3;

#else
#error "Not Support non-frey Firmware."
#endif

	return 1;
}
#else
static int core_initAI(FlashInfo * pFI)
{
	PHBA_Extension hba;
	PModule_Header mheader;
	MV_U32 i;
	/* Size must be 64 bit rounded. */
	mheader = ((PModule_Header)((MV_PTR_INTEGER)pCore
		- ROUNDING(sizeof(Module_Header), 8)));
	hba=(PHBA_Extension)mheader->hba_extension;
	for(i=0;i<MAX_BASE_ADDRESS;i++)
		pAI->bar[i]=hba->Base_Address[i];
	pAI->devId = hba->Device_Id;
	if (0!=Flash_AssignAdapterIf(pAI))
		return 0;
	pAI->pFI[pAI->FlashCount] = pFI;
		return 1;
}
#endif
int core_flash_init(MV_PVOID This)
{
	FlashInfo *pFI;
	MV_U32 tmp;

	if (pAI)
		return 0;
	pCore = (core_extension *)This;
	pAI = &AI;
	pFI = &FI[0];
	MV_ZeroMemory(pAI, sizeof( AdapterInfo ) );
	MV_ZeroMemory(pFI, sizeof( FlashInfo ) );
	if(core_initAI(pFI) == 0)
		return -1;
	if (0 == pAI->Flash_Init(pAI, pAI->pFI[pAI->FlashCount]) )
		pAI->FlashCount++;
	if(pAI->FlashCount>0) {
		for ( tmp=0; tmp<pAI->FlashCount; tmp++ )
			pAI->pFI[tmp]->DevBase+= pAI->pFI[tmp]->DecodeAddress;
		pCore = This;
		return 0;
	}
	return -1;
}

static MV_U32 core_flash_get_size(MV_PVOID This,mvf_req*req)
{
	if( req->did<MFR_EEPROM_DEV(0xFF))
		return 256;
	else if (req->did<MFR_FLASH_DEV(0xFF)) {
		FlashInfo *pFI;
		INIT_FI_PTR(pFI, req, MFR_FLASH_DEV(0) );
		req->len = pFI->Size;
		return MVF_TRUE;
	}
	return 0;
}

static int core_eeprom_meta_data_read(MV_PVOID	This,MV_U8 item_pos,
    void* data,MV_U16 count)
{
#if !defined( _MV88RC8XXX_ )
	if( 0== Odin_NVRamRead( pAI, item_pos, data, count) )
		return MVF_TRUE;
#endif
	return MVF_TRUE;
}

static int core_eeprom_meta_data_write(MV_PVOID	This,MV_U8 item_pos,
	void* data,MV_U16 count)
{
#if !defined( _MV88RC8XXX_ )
	if( 0== Odin_NVRamWrite( pAI, item_pos, data, count) )
		return MVF_TRUE;
#endif
	return MVF_TRUE;
}

static int core_flash_read(MV_PVOID This,mvf_req *req)
{
	if( req->did<MFR_EEPROM_DEV(0xFF) ){
		if( 0!=core_eeprom_meta_data_read(This, (MV_U8)req->off, req->buf, (MV_U16)req->len))
			return MVF_FALSE;
		} else if ( req->did<MFR_FLASH_DEV(0xFF)) {
		FlashInfo *pFI;

		INIT_FI_PTR(pFI, req, MFR_FLASH_DEV(0) );
		if ( 0!=pAI->Flash_ReadBuf(pAI, pFI, req->off, (MV_PU8)req->buf, req->len) )
			return MVF_FALSE;
	}
	return MVF_TRUE;
}

static int core_flash_write(MV_PVOID This,mvf_req *req)
{
	if( req->did<MFR_EEPROM_DEV(0xFF) ){
		if( 0!=core_eeprom_meta_data_write(This, (MV_U8)req->off, req->buf,(MV_U16)req->len) )
		return MVF_FALSE;
	} else if (req->did<MFR_FLASH_DEV(0xFF)) {
		FlashInfo *pFI;

		INIT_FI_PTR(pFI, req, MFR_FLASH_DEV(0));
		if ( req->flags&MVFR_FLAG_RMW_BUF &&
			req->rmw_buf!=NULL ) {
			if( 0!=pAI->Flash_RMWBuf(pAI, pFI, req->off, (MV_PU8)req->buf, req->len, req->rmw_buf) )
				return MVF_FALSE;
		} else if ( req->flags&MVFR_FLAG_RMW_SECT && Flash_GetSectAddr(pFI, (MV_U32)(MV_PTR_INTEGER)req->rmw_buf) != Flash_GetSectAddr(pFI, req->off)) {
			if( 0!=pAI->Flash_SectRMW(pAI, pFI, req->off, (MV_PU8)req->buf, req->len, (MV_U32)(MV_PTR_INTEGER)req->rmw_buf) )
			return MVF_FALSE;
		} else if (req->flags & MVFR_FLAG_ERASE) {
			if (0!=pAI->Flash_Erase(pAI, pFI, req->off, req->len))
			return MVF_FALSE;
		} else {
			if ( 0!=Flash_CheckAreaErased(pAI, pFI, req->off, req->buf, req->len)) {
				FM_PRINT("\nErase this block, offset=0x%x, length=0x%x.",(MV_U32)(MV_PTR_INTEGER)req->buf , req->len);
			if (0!=pAI->Flash_Erase(pAI, pFI, req->off, req->len))
				return MVF_FALSE;
		}
			if( 0!=pAI->Flash_WriteBuf(pAI, pFI, req->off, (MV_PU8)req->buf, req->len) )
				return MVF_FALSE;
		}
	}
	return MVF_TRUE;
}

static int core_flash_capacity
(
    MV_PVOID	This,
    mvf_req     *req
)
{
	if (req->did < MFR_EEPROM_DEV(0xFF))
		return MVF_FALSE;
	else if(req->did < MFR_FLASH_DEV(0xFF)) {
		FlashInfo *pFI;

		INIT_FI_PTR(pFI, req, MFR_FLASH_DEV(0));
		if (((MV_PU8)req->buf == NULL) || (req->len == 0x00))
			return MVF_FALSE;

		if (req->flags & MVFR_FLAG_GETSIZE_ALL)
			*(MV_PU32)req->buf = pFI->Size;    //all Flash Size
		else if (req->flags & MVFR_FLAG_GETSIZE_A_BLOCK)
			*(MV_PU32)req->buf = pFI->SectSize;    //one Block size
		else if (req->flags & MVFR_FLAG_GETSIZE_A_SECTOR)
			*(MV_PU32)req->buf = pFI->SectSize;    //one sector size
		else
			return MVF_FALSE;
	}
	return MVF_TRUE;
}


static mvf_req   req = {0,};
mvf_req *core_mvf_req_ptr(void)
{
	return &req;
}

mvf_req *core_mvf_req(char op,MV_U32 id,MV_U32	off,
	MV_U32 len,void*buf,MV_U32 flags)
{
	if (flags&MVFR_FLAG_INIT)
		MV_ZeroMemory(&req, sizeof(mvf_req));

	req.op = op;
	req.did = id;
	req.off = off;
	req.len = len;
	req.buf = buf;
	req.flags = flags;
	MV_FillMemory((MV_PU8)ptr_read_modify_write_buffer, RAED_MODIFY_WRITE_BUFFER_SIZE, 0xFF);
	req.rmw_buf = (void *)ptr_read_modify_write_buffer;
	return &req;
}

int core_nvm_send_req(MV_PVOID This,mvf_req*req)
{
	int ret = MV_FALSE;

	REINIT_REAL_TIME(This, req);

	if (req->off >= pAI->pFI[0]->Size) {
		FM_PRINT("The offset<0x%lx> more than 0x%lx.\n", req->off, pAI->pFI[0]->Size);
		return ret;
	}

	if (req->op==MVFR_OP_READ)
		ret = core_flash_read(This, req);
	else if (req->op==MVFR_OP_WRITE)
		ret = core_flash_write(This, req);
	else if (req->op==MVFR_OP_ERASE)
		ret = core_flash_write(This, req);
	else if (req->op == MVFR_OP_CAPACITY)
		ret = core_flash_capacity(This, req);
	return ret;
}
#endif
