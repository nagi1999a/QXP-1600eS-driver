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
#ifdef SUPPORT_RMW_FLASH
#include "core_type.h"
#include "core_manager.h"
#include "core_internal.h"
#include "core_util.h"
#include "hba_inter.h"
#ifdef SUPPORT_ROC
#include "core_flash.h"
#endif

#include "core_spi.h"


MV_U8
core_init_rmw_flash_memory(MV_PVOID core_p,
        lib_resource_mgr *rsrc, MV_U16 max_io)
{
	core_extension  *core = (core_extension *)core_p;
#ifndef SUPPORT_ROC
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
	AdapterInfo     *adapter;
#endif
	MV_U32          item_size;
	MV_PU8          vir;
	MV_U8 tmp_buf[0x100] = {0};
	pci_auto_load_config *autoload_c;
	MV_U32 i;
	MV_U32 addr = 0;

#ifndef SUPPORT_ROC
	item_size = sizeof(AdapterInfo);
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	core->lib_flash.adapter_info = (MV_PVOID)vir;
	adapter = (AdapterInfo *)vir;
#endif

#ifdef SUPPORT_ROC
	item_size = FLASH_SECTOR_SIZE;
#else /* SUPPORT_ROC */
	adapter->bar[FLASH_BAR_NUMBER] = hba->Base_Address[FLASH_BAR_NUMBER];
	if (-1 == OdinSPI_Init(adapter)) {
	        core->lib_flash.buf_ptr = NULL;
	        core->lib_flash.buf_size = 0;
	        core->lib_flash.success = MV_FALSE;
	        CORE_DPRINT(("Flash init failed\n"));
	        return MV_FALSE;
	}else{
	    CORE_DPRINT(("Begin SPI flash test...\n"));
	    if (OdinSPI_ReadBuf(adapter, 0, tmp_buf, 0x100) == -1)
	    {
	   		 CORE_DPRINT(("READ AutoLoad failed\n"));
			 goto end;
	    }

		autoload_c = (pci_auto_load_config *)(tmp_buf + 0x38);
		if((autoload_c->signature[0] == 0x55) && (autoload_c->signature[1] == 0xAA)) 
		{
			//printk("0x0038::0x0000AA55\n");

	        for (i=0; i < (0x100/sizeof(MV_U32)); i++) {			
			   //printk("0x%04X::0x%08X\n", 0x3C + 4*i, autoload_c->data[i]);
				if (autoload_c->data[i] == 0xFFFFFFFF)
				{
				    addr = i;
					//printk("0x%x\n", addr);
				    break;
				}
	        }
		}
		else
		{
			CORE_DPRINT(("Invaild AutoLoad.\n"));
			goto end;
		}
				
		//erase first sector
		if(OdinSPI_SectErase(adapter, 0) != -1)
		{
			CORE_DPRINT(("Restore AutoLoad: FLASH ERASE SUCCESS!Erase Sector 1\n"));
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: FLASH ERASE FAILED!Erase Sector 1\n"));
			goto end;
		}
		
	    //write new data to first sector
	    if ((autoload_c->data[addr-2] == 0xE4008000) && 
			(autoload_c->data[addr-1] == 0x15801B4B)){
			autoload_c->data[addr-2] = 0xFFFFFFFF;
			autoload_c->data[addr-1] = 0xFFFFFFFF;	    
	    }else{
		    autoload_c->data[addr] = 0xE4008000; //0xF4
		    autoload_c->data[addr+1] = 0x15801B4B; //0xF8
		    autoload_c->data[addr+2] = 0xFFFFFFFF;
	    }
			
		if(OdinSPI_WriteBuf(adapter, 0, tmp_buf, 0x100)  == -1 )
		{
			CORE_DPRINT(("Restore AutoLoad: Write Bios first sector FAILED.\n"));
			goto end;
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: Write Bios first sector SUCESS.\n"));
		}

        memset(tmp_buf, 0, sizeof(tmp_buf));
        // read new data to first sector and compare the data
		if (OdinSPI_ReadBuf(adapter, 0, tmp_buf, 0x100) == -1)
	    {
	   		 CORE_DPRINT(("READ AutoLoad failed\n"));
			 goto end;
	    }

		autoload_c = (pci_auto_load_config *)(tmp_buf + 0x38);
		if((autoload_c->signature[0] == 0x55) && (autoload_c->signature[1] == 0xAA)) 
		{
			//printk("0x0038::0x0000AA55\n");

	        for (i=0; i < (0x100/sizeof(MV_U32)); i++) {			
			  // printk("0x%04X::0x%08X\n", 0x3C + 4*i, autoload_c->data[i]);
				if (autoload_c->data[i] == 0xFFFFFFFF)
				{
				    if (i != addr)
						CORE_DPRINT(("Test SPI success.\n"));
					break;
				}					
	        }		
		}
		else
		{
			CORE_DPRINT(("Invaild AutoLoad.\n"));
			goto end;
		}
		
end:				
		CORE_DPRINT(("End SPI flash test.\n"));

	}
	item_size = adapter->FlashSectSize;
#endif /* SUPPORT_ROC */

	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL)
		return MV_FALSE;
	core->lib_flash.buf_ptr = vir;
	core->lib_flash.buf_size = item_size;
#ifndef SUPPORT_ROC
	core->lib_flash.success = MV_TRUE;
#endif
        return MV_TRUE;
}

MV_U32
core_rmw_flash_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32          size = 0;

	size += sizeof(AdapterInfo);
	/* ikkwong: TBD */
	/* Unable to determine the actual sector size */
	/* Using this hard-coded value for now */
	size += FLASH_SECTOR_SIZE;

	return size;
}

MV_U32
core_rmw_flash_get_dma_memory_quota(MV_U16 max_io)
{
        return 0;
}

MV_U8
core_rmw_read_flash(MV_PVOID core_p,
        MV_U32 addr,
        MV_PU8 data,
        MV_U32 size)
{
#ifndef SUPPORT_ROC
        core_extension  *core = (core_extension *)core_p;
        HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
       AdapterInfo	*adapter_info;
        MV_PU8          vir;

        adapter_info = (AdapterInfo *)core->lib_flash.adapter_info;
        if (core->lib_flash.success == MV_FALSE) {
                if (-1 == OdinSPI_Init(adapter_info)) {
                        CORE_DPRINT(("Flash init failed\n"));
                        return MV_FALSE;
                }
                vir = (MV_PU8)lib_rsrc_malloc_cached(&core->lib_rsrc,
                                adapter_info->FlashSectSize);
                if (vir == NULL) return MV_FALSE;
                core->lib_flash.buf_ptr = vir;
                core->lib_flash.buf_size = adapter_info->FlashSectSize;
                core->lib_flash.success = MV_TRUE;
        }
#endif

#ifdef SUPPORT_ROC
        /* ikkwong: TBD */
        if (!Core_NVMRd(NULL, MFR_FLASH_DEV(0), addr,
                size, data, 0)) {

                CORE_DPRINT(("Reading from flash failed\n"));
	        return MV_FALSE;
        }
#else
	OdinSPI_ReadBuf(adapter_info, addr, data, size);
#endif
        return MV_TRUE;
}

MV_U8
core_rmw_write_flash(MV_PVOID core_p,
		MV_U32 addr,
		MV_PU8 data,
		MV_U32 size)
{
	MV_U32 data_tmp_addr, buf_tmp_addr;
	MV_U32 data_tmp_size, buf_tmp_size;
	MV_U32 end, data_offset, buf_offset;
	MV_PU8 tmp_buf;
	core_extension  *core = (core_extension *)core_p;

#ifndef SUPPORT_ROC
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
	AdapterInfo	*adapter_info;
	MV_PU8          vir;

	adapter_info = (AdapterInfo *)core->lib_flash.adapter_info;
	if (core->lib_flash.success == MV_FALSE) {
		if (-1 == OdinSPI_Init(adapter_info)) {
			CORE_DPRINT(("Flash init failed\n"));
			return MV_FALSE;
		}
		vir = (MV_PU8)lib_rsrc_malloc_cached(&core->lib_rsrc,
		adapter_info->FlashSectSize);
		if (vir == NULL)
			return MV_FALSE;
		core->lib_flash.buf_ptr = vir;
		core->lib_flash.buf_size = adapter_info->FlashSectSize;
		core->lib_flash.success = MV_TRUE;
	}
#endif
	tmp_buf = core->lib_flash.buf_ptr;
	if ((data == NULL) || (tmp_buf == NULL))
		return MV_FALSE;

	data_tmp_addr = addr;
	data_tmp_size = 0;
	data_offset = 0;
	end = addr + size;

#ifdef SUPPORT_ROC
	/* ikkwong: TBD */
	buf_tmp_size = core->lib_flash.buf_size;
#else
	buf_tmp_size = adapter_info->FlashSectSize;
#endif
	while (data_tmp_addr + data_tmp_size < end) {
	data_tmp_addr += data_tmp_size;
	buf_tmp_addr = (data_tmp_addr / buf_tmp_size) * buf_tmp_size;

	data_tmp_size = MV_MIN(buf_tmp_addr + buf_tmp_size, end)
	- data_tmp_addr;

	data_offset = data_tmp_addr - addr;
	if (data_offset == 0)
		buf_offset = data_tmp_addr - buf_tmp_addr;
	else
		buf_offset = 0;
#ifdef SUPPORT_ROC
	/* ikkwong: TBD */
	if (!Core_NVMRd(core, MFR_FLASH_DEV(0), buf_tmp_addr,
		buf_tmp_size, tmp_buf, 0)) {
		CORE_DPRINT(("Reading from flash failed\n"));
		return MV_FALSE;
	}
#else
	OdinSPI_ReadBuf(adapter_info, buf_tmp_addr,
	tmp_buf, buf_tmp_size);
#endif

	MV_CopyMemory(&tmp_buf[buf_offset],
	&data[data_offset], data_tmp_size);

#ifdef SUPPORT_ROC
	if (!Core_NVMWr(core, MFR_FLASH_DEV(0), buf_tmp_addr,
		buf_tmp_size, tmp_buf, 0)) {
		CORE_DPRINT(("Writing to flash failed\n"));
		return MV_FALSE;
	}
#else
	if (OdinSPI_SectErase(adapter_info, data_tmp_addr)
		== -1) {
		CORE_DPRINT(("Flash Erase Failed\n"));
		return MV_FALSE;
	}

	OdinSPI_WriteBuf(adapter_info, buf_tmp_addr,
	tmp_buf, buf_tmp_size);
#endif
	}

	//MV_DASSERT(data_tmp_addr + data_tmp_size == end);
	return MV_TRUE;
}

#endif
