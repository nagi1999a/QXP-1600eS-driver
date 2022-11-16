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

#ifdef SUPPORT_FLASH
#include "mv_config.h"
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_api.h"
#include "core_util.h"
#include "com_flash.h"
#include "hba_inter.h"
#include "core_spi.h"
#include "core_flash.h"

#ifdef SUPPORT_FLASH_CMD_VIA_SCSI_RW_BUFFER
#include "com_buffer.h"
#endif

/*TBD: erase PD page API*/
#if 0
MV_BOOLEAN Core_Flash_Erase_PD_Page(
	IN MV_PVOID		extension,
	IN PMV_Request	p_req
	)
{
	PCore_Driver_Extension	p_core_ext = (PCore_Driver_Extension)extension;
	AdapterInfo			adapter_info;

	p_pd_info_page 		p_pd_page = NULL;
	p_page_header		p_pd_page_header;
	MV_PTR_INTEGER		pd_page_addr = 0;
	MV_PVOID 			p_page_buf = NULL;
	MV_U16				HBA_next_page_offset = 0;
	MV_U32				buf_start_addr;

	/* step 1: get PD page from Flash */
	if(!mv_prepare_pd_page(p_core_ext,&buf_start_addr,&HBA_next_page_offset)){
		p_req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_FALSE;
	}
	/* step 2: get pd page from buffer */
	p_page_buf = p_core_ext->Page_Buffer_Pointer;
	pd_page_addr = (MV_PTR_INTEGER)p_page_buf + PAGE_BUFFER_SIZE - sizeof(HBA_Info_Page) -HBA_next_page_offset;
	p_pd_page = (p_pd_info_page)pd_page_addr;

	/* step 3: init pd page with 0xff */
	if(p_req->Cdb[2]==FLASH_PD_PAGE_FORCE)
		MV_FillMemory((MV_PVOID)p_pd_page, FLASH_PD_INFO_PAGE_SIZE, 0xFF);
	else
	{
		p_pd_page_header = (p_page_header)p_pd_page ;
		mvflash_init_pd_page(p_core_ext,p_pd_page);

		/* step 4: calculate check sum of PD page */
		p_pd_page_header->check_sum = 0;
		p_pd_page_header->check_sum = mvCalculateChecksum((MV_PU8)p_pd_page,sizeof(pd_info_page));
	}

	adapter_info.bar[2] = p_core_ext->Base_Address[2];
	if (-1 == OdinSPI_Init(&adapter_info))
		return MV_FALSE;
	/* step 5: erase flash */
	if(OdinSPI_SectErase( &adapter_info, buf_start_addr) != -1) {
		CORE_DPRINT(("FLASH ERASE SUCCESS!\n"));
	}
	else {
		CORE_DPRINT(("FLASH ERASE FAILED!\n"));
		p_req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_FALSE;
	}
	/* step 6: write clean PD page to flash */
	Core_FlashWrite(p_core_ext,buf_start_addr,PAGE_BUFFER_SIZE,p_page_buf);
	p_req->Scsi_Status=REQ_STATUS_SUCCESS;
	return MV_TRUE;

}
#endif

#ifdef AMCC_CLI_WORKROUND
extern unsigned char page_buffer[4000];
#endif

MV_U8
core_flash_bin(
	IN MV_PVOID core_p,
	IN PMV_Request req)
{
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core_p, MODULE_HBA);
	AdapterInfo		adapter_info;
	MV_U32               size = 0,length;
	MV_U32               buf_start_addr = 0,flash_start_addr = 0;
	MV_BOOLEAN       need_erase;
	MV_U8			flash_op,checksum;
	MV_U16			i,erase_sect_num;
	MV_U8			buff[16];
	PFlash_DriveData    data;

        data =(PFlash_DriveData)core_map_data_buffer(req);
	flash_op = 0;
	need_erase = MV_FALSE;
	length = 0;

	if (data == NULL ||
		req->Data_Transfer_Length < sizeof(Flash_DriveData)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	switch (req->Cdb[2]) {
	case FLASH_UPLOAD:
		if (data->PageNumber == 0 && data->Data !=NULL)
			need_erase = MV_TRUE;
		else if(data->PageNumber>0)
			need_erase = MV_FALSE;
		flash_op = 1;
		size = data->Size;
		data->Size = 0;
		CORE_DPRINT(("Upload!\n"));
		break;
	case FLASH_DOWNLOAD:
		flash_op = 2;
		CORE_DPRINT(("Download!\n"));
		break;
#ifdef SUPPORT_CONFIG_FILE
	case FLASH_ERASE_PAGE:
		flash_op = 3;
		need_erase = MV_TRUE;
		CORE_DPRINT(("Erase!\n"));
		break;
#endif
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		CORE_DPRINT(("Unknown action!\n"));
	}

#ifdef SUPPORT_RMW_FLASH
	adapter_info = *(AdapterInfo *)core->lib_flash.adapter_info;
#else
	adapter_info.bar[FLASH_BAR_NUMBER] = hba->Base_Address[FLASH_BAR_NUMBER];

	if (OdinSPI_Init(&adapter_info) == -1) {
		CORE_DPRINT(("FLASH Init FAILED!\n"));
		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif

	switch (req->Cdb[3]) {
	case  FLASH_TYPE_BIOS:
		buf_start_addr = 0x00000000;
		erase_sect_num = (MV_U16)(MAX_BIOS_SIZE /adapter_info.FlashSectSize);
		CORE_DPRINT(("BIOS Type!\n"));
		break;
	case  FLASH_TYPE_CONFIG:
		buf_start_addr = 0x30000;
		erase_sect_num = 1;
		CORE_DPRINT(("CONFIG Type!\n"));
		break;
#ifdef SUPPORT_CONFIG_FILE
	case FLASH_TYPE_AUTOLOAD:
		buf_start_addr = 0; //autoload data offset
		erase_sect_num = 1;
		CORE_DPRINT(("AutoLoad Type!\n"));
		break;
#endif
	default:
		req->Scsi_Status=REQ_STATUS_INVALID_PARAMETER;
		CORE_DPRINT(("Unknown Type!\n"));
		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (req->Cdb[3] == FLASH_TYPE_CONFIG) {
		if(need_erase) {
			flash_start_addr = buf_start_addr;
			if(OdinSPI_SectErase( &adapter_info, flash_start_addr) != -1){
				CORE_DPRINT(("Core_Flash_Bin: FLASH ERASE SUCCESS!Erase Sector\n"));
			} else {
				CORE_DPRINT(("Core_Flash_Bin: FLASH ERASE FAILED!Erase Sector\n"));
				req->Scsi_Status = REQ_STATUS_ERROR;
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
		}

	if (flash_op==1){
		checksum = mvCalculateChecksum(data->Data, size);
		flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;

#ifdef AMCC_CLI_WORKROUND
		MV_CopyMemory(data->Data, page_buffer,0x4000);
#endif
		if (OdinSPI_WriteBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Core_Flash_Bin: Write FLASH FAILED!PageNumber is %d\n",data->PageNumber));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Core_Flash_Bin: Write FLASH SUCCESS!PageNumber is %d,address is %x\n",\
				data->PageNumber,flash_start_addr));
		}

		MV_ZeroMemory(data->Data,size);
		if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Core_Flash_Bin: Read FLASH SUCCESS!Address is %x\n",flash_start_addr));
		}

		//check the sum and return the writen size
		if (checksum == mvCalculateChecksum(data->Data, size)){
			data->Size = (MV_U16)size;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			CORE_DPRINT(("Core_Flash_Bin: CheckSum is OK!\n"));
		}
	}

	if (flash_op == 2){
		length = 64*1024;
		flash_start_addr = 0;

		/* calculate the page size */
		if((MV_U32)((data->PageNumber + 1) * DRIVER_LENGTH)< length)//read the whole page
		{
			size = DRIVER_LENGTH;
			data->Size=0;
			flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
				CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
				req->Scsi_Status=REQ_STATUS_ERROR;
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
			data->Size=(MV_U16)size;
			data->PageNumber++;
		} else {
			size=(MV_U16)(length - data->PageNumber * DRIVER_LENGTH);
			if(size != 0)
			{
				data->Size = 0;
				flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
			}
			data->Size = (MV_U16)size;
			data->isLastPage = MV_TRUE;
		}
	}
    }else if(req->Cdb[3] == FLASH_TYPE_BIOS){
	/* erase flash */
	if (need_erase) {
		for (i = 0; i < erase_sect_num; i++) {
			flash_start_addr = buf_start_addr + i * adapter_info.FlashSectSize;
			if (OdinSPI_SectErase(&adapter_info, flash_start_addr) != -1) {
				CORE_DPRINT(("FLASH ERASE SUCCESS!Erase Sector %d\n", i));
			} else {
				CORE_DPRINT(("FLASH ERASE FAILED!Erase Sector %d\n", i));
				req->Scsi_Status = REQ_STATUS_ERROR;
                                core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
		}
	}

	/* write flash */
	if (flash_op == 1) {
		/*get CheckSum */
		checksum  = mvCalculateChecksum(data->Data, size);
		flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
		if (OdinSPI_WriteBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Write FLASH FAILED!PageNumber is %d\n",data->PageNumber));
			req->Scsi_Status = REQ_STATUS_ERROR;
                        core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Write FLASH SUCCESS!PageNumber is %d,address is %x\n",\
                                data->PageNumber, flash_start_addr));
		}

		MV_ZeroMemory(data->Data, size);
		if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Read FLASH FAILED!Address is %x\n", flash_start_addr));
			req->Scsi_Status = REQ_STATUS_ERROR;
                        core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Read FLASH SUCCESS!Address is %x\n", flash_start_addr));
		}

		/* check the sum and return the writen size */
		if (checksum != mvCalculateChecksum(data->Data, size)){
			CORE_DPRINT(("The Checksum is wrong! \n"));
			//req->Scsi_Status = REQ_STATUS_ERROR;
                     //   core_unmap_data_buffer(req);
			//return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			data->Size = (MV_U16)size;
			CORE_DPRINT(("CheckSum is OK!\n"));
		}
	}

		if (flash_op == 2) {
			/* calculate the bios size */
			flash_start_addr = 0;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, buff, 16) == -1) {
				CORE_DPRINT(("Read FLASH FAILED!Address is %x\n", flash_start_addr));
				req->Scsi_Status = REQ_STATUS_ERROR;
				core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}

			length=buff[2];

			flash_start_addr = 0x140;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, buff, 16) == -1) {
				CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
				req->Scsi_Status = REQ_STATUS_ERROR;
				core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}

			length += buff[0];
			length += buff[1];
			length *= 512;

			flash_start_addr = 0;
			buf_start_addr = 0;

			/* calculate the page size */
			if ((MV_U32)((data->PageNumber + 1) * DRIVER_LENGTH) <= length) {//read the whole page
				size = DRIVER_LENGTH;
				data->Size = 0;
				flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					core_unmap_data_buffer(req);
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
				data->Size=(MV_U16)size;
				data->PageNumber++;
			} else { /* read the last page */
				size = (MV_U16)(length-data->PageNumber * DRIVER_LENGTH);
				data->Size = 0;
				flash_start_addr = buf_start_addr+data->PageNumber*DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					core_unmap_data_buffer(req);
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
				data->Size = (MV_U16)size;
				data->isLastPage = MV_TRUE;
			}
		}
	}

#ifdef SUPPORT_CONFIG_FILE

    else if(req->Cdb[3] == FLASH_TYPE_AUTOLOAD)
   {
   	MV_U16 			bios_signature;
   	MV_U32          	length;


	//write flash
	if(flash_op==1)
	{
		 /* Check if  BIOS code exists */
		flash_start_addr = 0;
		if(OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)&bios_signature,sizeof(bios_signature))  == -1)
		{
			CORE_DPRINT((" Read Bios header FAILED!\n"));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		if ( bios_signature != 0xaa55 )
     	       {
     	       	data->Size=0;
			data->isLastPage=MV_TRUE;
	     		MV_PRINT("No valid autoload data exist on specified adapter!!\n");
			req->Scsi_Status=REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	     	}

		int temp = 0;
		MV_U32* auto_load;
		unsigned char image[RAID_BIOS_ROM_SIZE]; /* 64 KByte */
		unsigned long romsize = RAID_BIOS_ROM_SIZE;

		if(OdinSPI_ReadBuf(&adapter_info, flash_start_addr,(MV_U8*)&image,RAID_BIOS_ROM_SIZE)  == -1 )
		{
			CORE_DPRINT(("Restore AutoLoad: Read Bios first sector FAILED!\n"));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: Read Bios first sector Success!\n"));
		}
		auto_load = (MV_U32 *)&image[AUTOLOAD_FLASH_OFFSET];
		MV_CopyMemory(auto_load, data->Data, size);//AUTOLOAD_FLASH_LENGTH

		//calculate ROM_IMAGE_4K_CHECKSUM
		image[ROM_IMAGE_4K_CHECKSUM] = 0;
		checksum = 0;
        	for (temp = 0 ; temp < RELOCATE_ROM_SIZE ; temp ++)
       	{
                	checksum += image[temp];
        	}
        	image[ROM_IMAGE_4K_CHECKSUM] = (~checksum) + 1;

        	  /* Calculate checksum of whole image */
        	 image[romsize-1] = 0;
	        checksum = 0;
	        for (temp = 0 ; temp < romsize ; temp ++)
	        {
	                checksum += image[temp];
	        }

	        image[romsize-1] = (~checksum) + 1;

		//erase first sector
		if(OdinSPI_SectErase( &adapter_info, flash_start_addr) != -1)
		{
			CORE_DPRINT(("Restore AutoLoad: FLASH ERASE SUCCESS!Erase Sector 1\n"));
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: FLASH ERASE FAILED!Erase Sector 1\n"));
			req->Scsi_Status=REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

              //write new data to first sector
		if(OdinSPI_WriteBuf(&adapter_info, flash_start_addr,(MV_U8*)&image,romsize)  == -1 )
		{
			CORE_DPRINT(("Restore AutoLoad: Write Bios first sector FAILED.\n"));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: Write Bios first sector SUCESS.\n"));
		}

		checksum=mvCalculateChecksum(data->Data,size);
		MV_ZeroMemory(data->Data,size);
		flash_start_addr = AUTOLOAD_FLASH_OFFSET;
		if(OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data,size)  == -1 )
		{
			CORE_DPRINT(("Restore AutoLoad: Read AutoLoad Data FAILED!Address is %x\n",flash_start_addr));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		else
		{
			CORE_DPRINT(("Restore AutoLoad: Read AutoLoad Data SUCCESS!Address is %x\n",flash_start_addr));
		}

		//check the sum and return the writen size
		if(checksum==mvCalculateChecksum(data->Data,size))
		{
			data->Size = (MV_U16)size;
			req->Scsi_Status=REQ_STATUS_SUCCESS;
			CORE_DPRINT(("Restore AutoLoad: CheckSum is OK!\n"));
		}
	}

	if(flash_op==2)
	{
		 /* Check if  BIOS code exists */
		flash_start_addr=0;
		if(OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)&bios_signature,sizeof( bios_signature ))  == -1 )
		{
			CORE_DPRINT((" Read Bios header FAILED!\n"));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		if ( bios_signature != 0xaa55 )
     	       {
     	       	 data->Size=0;
			 data->isLastPage=MV_TRUE;
	     		MV_PRINT("No valid image exist on specified adapter!!\n");
			req->Scsi_Status=REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	     	}

		length = AUTOLOAD_FLASH_LENGTH;
		flash_start_addr = AUTOLOAD_FLASH_OFFSET;
		data->Size=0;
		if(OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data,length)  == -1 )
		{
			CORE_DPRINT((" Read FLASH FAILED!Address is %x\n",flash_start_addr));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
		data->Size=(MV_U16)length;
		data->isLastPage=MV_TRUE;
	}
   }
#endif

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
core_flash_rwtest(
	IN MV_PVOID core_p,
	IN PMV_Request req)
{
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core_p, MODULE_HBA);
	AdapterInfo		adapter_info;
	MV_U32               bytes = 0,offset = 0;
	MV_BOOLEAN		need_erase;
	MV_U8			flash_op,checksum;
	PDBG_Flash    data;

	data = (PDBG_Flash)core_map_data_buffer(req);
	need_erase = MV_FALSE;
	flash_op = 0;

	if (data == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Flash)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	switch (req->Cdb[2]) {	
		case FLASH_BYTE_WRITE:
				flash_op = 1;
				bytes = data->NumBytes;
				offset = data->OffSet;
				if (!(offset % FLASH_SECTOR_SIZE)){
					need_erase = MV_TRUE;
				}
				CORE_DPRINT(("Write Flash Test!\n"));
				break;
		case FLASH_BYTE_READ:
				flash_op = 2;
				bytes = data->NumBytes;
				CORE_DPRINT(("Read Flash Test!\n"));
				break;
		default:
			req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
			CORE_DPRINT(("Unknown action!\n"));
	}
	
#ifdef SUPPORT_RMW_FLASH
	adapter_info = *(AdapterInfo *)core->lib_flash.adapter_info;
#else
	adapter_info.bar[FLASH_BAR_NUMBER] = hba->Base_Address[FLASH_BAR_NUMBER];

    if (OdinSPI_Init( &adapter_info ) == -1) {
    		CORE_DPRINT(("Core_Flash_ReadWrite_Test: FLASH INIT FAILED!\n"));
		core_unmap_data_buffer(req);
       	return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
#endif

	if(need_erase) {
		if(OdinSPI_SectErase( &adapter_info,  (offset/FLASH_SECTOR_SIZE)*FLASH_SECTOR_SIZE) != -1){
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: FLASH ERASE SUCCESS!Erase Sector %d\n", offset/FLASH_SECTOR_SIZE));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: FLASH ERASE FAILED!Erase Sector %d\n", offset/FLASH_SECTOR_SIZE));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_FALSE;
		}
	}
	
	//write flash
	if (flash_op==1) {
		//get CheckSum
		checksum = mvCalculateChecksum(data->Data, bytes);
		
		if (OdinSPI_WriteBuf(&adapter_info, offset, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Write FLASH SUCCESS!\n"));
		} else {			
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Write FLASH FAILED!\n"));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
		
		bytes = data->NumBytes;
		MV_ZeroMemory(data->Data, data->NumBytes);
		if (OdinSPI_ReadBuf(&adapter_info, data->OffSet, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH SUCCESS!Address is %x\n", data->OffSet));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH FAILED!Address is %x\n", data->OffSet));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
		
		//check the sum and return the writen size
		if (checksum == mvCalculateChecksum(data->Data, data->NumBytes)) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: CheckSum is OK!\n"));
		} else	
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: CheckSum Failed!\n"));
	}

	if (flash_op==2) {
		if(OdinSPI_ReadBuf(&adapter_info, data->OffSet, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH SUCCESS!Address is %x\n", data->OffSet));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH FAILED!Address is %x\n", data->OffSet));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
	}
	
	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
core_flash_command(
	IN MV_PVOID core_p,
	IN PMV_Request p_req
	)
{
	MV_U8 status = MV_QUEUE_COMMAND_RESULT_FINISHED;

	switch (p_req->Cdb[1]) {
#if !defined(SUPPORT_ROC)
		case APICDB1_FLASH_BIN:
			status = core_flash_bin(core_p, p_req);
			break;

#ifdef SUPPORT_ERASE_PD_PAGE
		case APICDB1_ERASE_FLASH:
			if (core_pd_erase_pd_page(core_p) == MV_TRUE)
				p_req->Scsi_Status = REQ_STATUS_SUCCESS;
                     else
				p_req->Scsi_Status = REQ_STATUS_ERROR;
			break;
#endif
		case APICDB1_TEST_FLASH:
			status = core_flash_rwtest(core_p, p_req);
			break;
#endif //SUPPORT_ROC
#ifdef SUPPORT_FLASH_CMD_VIA_SCSI_RW_BUFFER
        case APICDB1_BUFFER_DETAIL_FLASH:
            status = core_get_flash_image_detail_info(core_p, p_req);
            break;
#endif
            
		default:
            if (p_req->Sense_Info_Buffer != NULL)
                ((MV_PU8)p_req->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
            p_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
			break;
	}
	return status;
}

#ifdef SUPPORT_FLASH_CMD_VIA_SCSI_RW_BUFFER


#ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
MV_U32 flash_image_generation_crc_calculate(MV_PU32 generation)
{
    MV_U32 crc32 = 0;
    MV_PU8 pdata = (MV_PU8)generation;

//    FM_PRINT("Y.L %s %d %s ...  generation = [0x%x]\n", __FILE__, __LINE__, __FUNCTION__, *generation);

    if(*generation==0xFFFFFFFF)
    	return *generation;
#if 1
    crc32 = MV_CRC(pdata, 4);
    //FM_PRINT("JL %s gen %x, crc %x\n", __FUNCTION__, *generation, crc32);
#else
	for(i=0;i<4;i++){
		if(i==0||i==2)
			tmp = ~(*p_check_sum);
		else
			tmp = *p_check_sum;
#ifdef FLASH_DEBUG
		FM_PRINT("Y.L %s %d %s ...  check_sum = [0x%x], tmp=[0x%x]\n", __FILE__, __LINE__, __FUNCTION__, check_sum,tmp);
#endif
		check_sum +=  (MV_U32)tmp;
#ifdef FLASH_DEBUG
		FM_PRINT("Y.L %s %d %s ...  check_sum = [0x%x], tmp=[0x%x]\n", __FILE__, __LINE__, __FUNCTION__, check_sum,tmp);
#endif
		p_check_sum++;
	}
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s %d %s ...  generation = [0x%x], check_sum = [0x%x]\n", __FILE__, __LINE__, __FUNCTION__, *generation,check_sum);
#endif
#endif
    return crc32;
}
MV_BOOLEAN flash_image_generation_crc_check(MV_PU32 generation, MV_PU32 crc)
{
    MV_U32 crc32 = 0;
    MV_PU8 pdata = (MV_PU8)generation;

    if(*generation==0xFFFFFFFF&&*crc==0xFFFFFFFF)
    	return MV_TRUE;
#if 1
    crc32 = MV_CRC(pdata, 4);
    if (*crc == crc32) {
        //FM_PRINT("JL %s, gen %x, crc %x\n",  __FUNCTION__, *generation, *crc);
        return MV_TRUE;
    } else {
        FM_PRINT("%s CRC error: gen %x, *crc %x, crc32 %x\n", __FUNCTION__, *generation, *crc, crc32);
        return MV_FALSE;
    }
#else
	for(i=0;i<4;i++){
		if(i==0||i==2)
			tmp = ~(*p_check_sum);
		else
			tmp = *p_check_sum;
#ifdef FLASH_DEBUG
		FM_PRINT("Y.L %s %d %s ...  check_sum = [0x%x], tmp=[0x%x]\n", __FILE__, __LINE__, __FUNCTION__, check_sum,tmp);
#endif
		check_sum += (MV_U32)tmp;
#ifdef FLASH_DEBUG
		FM_PRINT("Y.L %s %d %s ...  check_sum = [0x%x], tmp=[0x%x]\n", __FILE__, __LINE__, __FUNCTION__, check_sum,tmp);
#endif
		p_check_sum++;
	}
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s %d %s ...  generation = [0x%x], check_sum = [0x%x],crc = [0x%x]\n", __FILE__, __LINE__, __FUNCTION__, *generation,check_sum, *crc);
#endif
	if(check_sum!=*crc){
		return MV_FALSE;
	}
	return MV_TRUE;
#endif
}
#endif


MV_BOOLEAN buffer_type_check(
	MV_U8	type
	)
{
	switch(type){
		case BUFFER_TYPE_FIRMWARE:
		case BUFFER_TYPE_BIOS:
		case BUFFER_TYPE_HBA_TXT:
		case BUFFER_TYPE_CONFIG:
		case BUFFER_TYPE_UBOOT:
			return MV_TRUE;
		default:
			FM_PRINT("Y.L %s %d %s ... Unsupport BUFFER TYPE =0x%x\n", type);
			return MV_FALSE;
	}
}
MV_BOOLEAN is_dual_image(
	MV_U8 type
	)
{
	switch(type){
		case BUFFER_TYPE_FIRMWARE:
			return MV_TRUE;
		case BUFFER_TYPE_BIOS:
		case BUFFER_TYPE_HBA_TXT:
		case BUFFER_TYPE_CONFIG:
		case BUFFER_TYPE_UBOOT:
			return MV_FALSE;
		default:
			FM_PRINT("Y.L %s %d %s ... Unsupport BUFFER TYPE =0x%x\n", type);
			return MV_FALSE;
	}
}
MV_BOOLEAN core_get_image_start_addr(
	MV_U8		type,
	MV_PU32		p_start_addr,
	MV_PU32		p_size,
	MV_BOOLEAN	base
	)
{
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ... buffer type[0x%x], base [0x%x].\n", __FUNCTION__,type,base);
#endif
	MV_U32 tmp_addr[2] = {0,0};
	MV_U32 tmp_size[2] = {0,0};
	MV_PU32 p_tmp_addr = p_start_addr;
	MV_PU32 p_tmp_size = p_size;

	if(!is_dual_image(type)){

		if(base){
			switch(type){
			case BUFFER_TYPE_CONFIG:
				*p_start_addr = FLASH_HBA_CONFIG_ADDR;
				*p_size = FLASH_HBA_CONFIG_SIZE;
				break;
			case BUFFER_TYPE_BIOS:
				*p_start_addr = FLASH_BIOS_ADDR;
				*p_size = FLASH_BIOS_SIZE;
				break;
			case BUFFER_TYPE_FIRMWARE:
				*p_start_addr = FLASH_FIRMWARE_ADDR;
				*p_size = FLASH_FIRMWARE_SIZE;
				break;
			case BUFFER_TYPE_UBOOT:
				*p_start_addr = FLASH_UBOOT_ADDR;
				*p_size = FLASH_UBOOT_SIZE;
				break;
			case BUFFER_TYPE_HBA_TXT:
				*p_start_addr = FLASH_HBA_TXT_ADDR;
				*p_size = FLASH_HBA_TXT_SIZE;
				break;
			}
		}else{
			switch(type){
			case BUFFER_TYPE_CONFIG:
				*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0,FLASH_IMAGE_RETURN_ADDRESS);
				*p_size = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0,FLASH_IMAGE_RETURN_SIZE);
				break;
			case BUFFER_TYPE_BIOS:
				*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_BIOS, 0,FLASH_IMAGE_RETURN_ADDRESS);
				*p_size = return_image_place_on_flash(FLASH_IMAGE_BIOS, 0,FLASH_IMAGE_RETURN_SIZE);
				break;
			case BUFFER_TYPE_FIRMWARE:
				*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 0,FLASH_IMAGE_RETURN_ADDRESS);
				*p_size = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 0,FLASH_IMAGE_RETURN_SIZE);
				break;
			case BUFFER_TYPE_UBOOT:
				*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0,FLASH_IMAGE_RETURN_ADDRESS);
				*p_size = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0,FLASH_IMAGE_RETURN_SIZE);
				break;
			case BUFFER_TYPE_HBA_TXT:
				*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 0,FLASH_IMAGE_RETURN_ADDRESS);
				*p_size = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 0,FLASH_IMAGE_RETURN_SIZE);
				break;
			}
		}
#ifdef FLASH_DEBUG
		FM_PRINT("Y.L %s ... buffer type[0x%x], single image, addr[0x%x], size[0x%x] \n", __FUNCTION__, type, *p_start_addr,*p_size);
#endif
		if(*p_size==0xffffffff ||*p_start_addr==0xffffffff||*p_size==0x0){
			MV_ASSERT(base==MV_FALSE);
			return MV_FALSE;
		}
		else
			return MV_TRUE;
	}
	if(base){
			switch(type){
			case BUFFER_TYPE_CONFIG:
				*p_start_addr = FLASH_HBA_CONFIG_ADDR;
				*p_size = FLASH_HBA_CONFIG_SIZE;
				p_start_addr++;
				p_size++;
				*p_start_addr = FLASH_HBA_CONFIG_ADDR;
				*p_size = FLASH_HBA_CONFIG_SIZE;
				break;
			case BUFFER_TYPE_BIOS:
				*p_start_addr = FLASH_BIOS_ADDR;
				*p_size = FLASH_BIOS_SIZE;
				p_start_addr++;
				p_size++;
				*p_start_addr = FLASH_BIOS_ADDR;
				*p_size = FLASH_BIOS_SIZE;
				break;
			case BUFFER_TYPE_FIRMWARE:
				*p_start_addr = FLASH_FIRMWARE_ADDR;
				*p_size = FLASH_FIRMWARE_SIZE;
				p_start_addr++;
				p_size++;
				*p_start_addr = FLASH_FIRMWARE_SECOND_ADDR;
				*p_size = FLASH_FIRMWARE_SIZE;
				break;
			case BUFFER_TYPE_UBOOT:
				*p_start_addr = FLASH_UBOOT_ADDR;
				*p_size = FLASH_UBOOT_SIZE;
				p_start_addr++;
				p_size++;
				*p_start_addr = FLASH_UBOOT_ADDR;
				*p_size = FLASH_UBOOT_SIZE;
				break;
			case BUFFER_TYPE_HBA_TXT:
				*p_start_addr = FLASH_HBA_TXT_ADDR;
				*p_size = FLASH_HBA_TXT_SIZE;
				p_start_addr++;
				p_size++;
				*p_start_addr = FLASH_HBA_TXT_ADDR;
				*p_size = FLASH_HBA_TXT_SIZE;
				break;
			}
	}else{
		switch(type){
		case BUFFER_TYPE_CONFIG:
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0,FLASH_IMAGE_RETURN_SIZE);
			p_start_addr++;
			p_size++;
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 1,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 1,FLASH_IMAGE_RETURN_SIZE);
			break;
		case BUFFER_TYPE_BIOS:
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_BIOS, 0,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_BIOS, 0,FLASH_IMAGE_RETURN_SIZE);
			p_start_addr++;
			p_size++;
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_BIOS, 1,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_BIOS, 1,FLASH_IMAGE_RETURN_SIZE);
			break;
		case BUFFER_TYPE_FIRMWARE:
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 0,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 0,FLASH_IMAGE_RETURN_SIZE);
			p_start_addr++;
			p_size++;
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 1,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 1,FLASH_IMAGE_RETURN_SIZE);
			break;
		case BUFFER_TYPE_UBOOT:
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0,FLASH_IMAGE_RETURN_SIZE);
			p_start_addr++;
			p_size++;
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 1,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 1,FLASH_IMAGE_RETURN_SIZE);
			break;
		case BUFFER_TYPE_HBA_TXT:
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 0,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 0,FLASH_IMAGE_RETURN_SIZE);
			p_start_addr++;
			p_size++;
			*p_start_addr = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 1,FLASH_IMAGE_RETURN_ADDRESS);
			*p_size = return_image_place_on_flash(FLASH_IMAGE_CONFIG, 1,FLASH_IMAGE_RETURN_SIZE);
			break;
		}
	}
	MV_CopyMemory(&tmp_addr, p_tmp_addr, 2*sizeof(MV_U32));
	MV_CopyMemory(&tmp_size, p_tmp_size, 2*sizeof(MV_U32));
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ... buffer type[0x%x], dual image, pri_addr[0x%x], pri_size[0x%x], sec_addr[0x%x], sec_size[0x%x] \n", __FUNCTION__, type, tmp_addr[0],tmp_size[0],tmp_addr[1],tmp_size[1]);
#endif
	if(tmp_addr[0]==0xffffffff ||tmp_addr[1]==0xffffffff||tmp_size[0]==0x0||tmp_size[1]==0x0||tmp_size[0]==0xffffffff||tmp_size[1]==0xffffffff){
		MV_ASSERT(base==MV_FALSE);
		return MV_FALSE;
	}
	else
		return MV_TRUE;
}

 MV_U8 core_flash_dual_image_addr_arbitrate(
 	IN MV_PVOID core_p,
 	MV_U8	action,
 	MV_PU32	p_source_addr,
 	MV_PU32	p_source_size,
 	MV_PU32	target_addr,
 	MV_PU64	p_base_gen
 	)
{
    core_extension  *core = (core_extension *)core_p;
	MV_U8	i=0,valid, invalid;
	MV_U64	gen[2];
	MV_U64	tmp_gen[2];
	MV_U32	gen_addr[2] = {0,0};
	MV_U8	decided = 0;
	MV_PU32	p_tmp_addr = p_source_addr;
	MV_PU32 p_tmp_size = p_source_size;
	MV_U32	source_addr[2] ={0,0};
	MV_U32	source_size[2] ={0,0};
#ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
	MV_U8	crc_0_check=0,crc_1_check=0;
#endif

	MV_CopyMemory(&source_addr[0], p_source_addr, 2*sizeof(MV_U32));
	MV_CopyMemory(&source_size[0], p_source_size, 2*sizeof(MV_U32));

	//FM_PRINT("Y.L %s %d %s ... action[0x%x], dual image, pri_addr[0x%x], pri_size[0x%x], sec_addr[0x%x], sec_size[0x%x] \n", __FILE__, __LINE__, __FUNCTION__, action, source_addr[0],source_size[0],source_addr[1],source_size[1]);
	gen_addr[0] = (*p_source_addr)+(*p_source_size)-GENERATION_SIZE;
	p_source_addr++;
	p_source_size++;
	gen_addr[1] = (*p_source_addr)+(*p_source_size)-GENERATION_SIZE;
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ... dual image, pri_generation_addr[0x%x], sec_generation_addr[0x%x]\n", __FUNCTION__, gen_addr[0],gen_addr[1]);
#endif
	U64_ZERO_VALUE(gen[0]);
	U64_ZERO_VALUE(gen[1]);
	if(!Core_NVMRd(core, MFR_FLASH_DEV(0), gen_addr[0], GENERATION_SIZE, (MV_PU8)&gen[0], 0)){
		FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
		return FLASH_READ_ERR;
	}
	if(!Core_NVMRd(core, MFR_FLASH_DEV(0), gen_addr[1], GENERATION_SIZE, (MV_PU8)&gen[1], 0)){
		FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
		return FLASH_READ_ERR;
	}
	U64_ZERO_VALUE(tmp_gen[0]);
	U64_ZERO_VALUE(tmp_gen[1]);
	U64_ASSIGN_U64(tmp_gen[0],gen[0]);
	U64_ASSIGN_U64(tmp_gen[1],gen[1]);
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ... image update, dual image, pri_generation[0x%x], sec_generation[0x%x]\n", __FUNCTION__, tmp_gen[0],tmp_gen[1]);
#endif
#ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
	if(action==IMAGE_UPDATE||action==IMAGE_BACKUP){
		// first need dual image's  genertion CRC are all OK, otherwise we can not arbitrate the generation since it is not reliable
		// if any of the generation's CRC is wrong, tell the App what happened as an event.
		// also restore the corrupted generation as default value 0xFFFFFFFFF
		if(!flash_image_generation_crc_check(&tmp_gen[0].parts.high,&tmp_gen[0].parts.low)){
			FM_PRINT("Y.L %s ... image update, pri_generation CRC is wrong, pri_generation_addr[0x%x],pri_generation[0x%x], pri_generation_crc[0x%x]\n", __FUNCTION__, gen_addr[0],tmp_gen[0].parts.high,tmp_gen[0].parts.low);
			//restore the corrupted generation as default value 0xFFFFFFFFF
			RESTORE_GENERATION(gen[0]);
			if(!Core_NVMWr(core, MFR_FLASH_DEV(0), gen_addr[0], GENERATION_SIZE, (MV_PU8)&gen[0], 0)){
				FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
					return FLASH_WRITE_ERR;
			}
			crc_0_check = 1;
		}
		if(!flash_image_generation_crc_check(&tmp_gen[1].parts.high,&tmp_gen[1].parts.low)){
			FM_PRINT("Y.L %s ... image update, sec_generation CRC is wrong, sec_generation_addr[0x%x],sec_generation[0x%x], sec_generation_crc[0x%x]\n", __FUNCTION__, gen_addr[1],tmp_gen[1].parts.high,tmp_gen[1].parts.low);
			//restore the corrupted generation as default value 0xFFFFFFFFF
			RESTORE_GENERATION(gen[1]);
			if(!Core_NVMWr(core, MFR_FLASH_DEV(0), gen_addr[1], GENERATION_SIZE, (MV_PU8)&gen[1], 0)){
				FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
					return FLASH_WRITE_ERR;
			}
			crc_1_check = 1;
		}

        //JL 2012/04/17
        //Only backup action need Gen. CRC correct
        //Update can overwrite it and 
        if (action == IMAGE_BACKUP) {
            if (crc_0_check!=0 && crc_1_check!=0) {
                core_generate_event(core, EVT_ID_FLASH_FATAL_GENERATION_ERR, 0, SEVERITY_WARNING, 0, NULL, 0);
                return FATAL_GENERATION_ERR;
            }
            if (crc_0_check != 0 || crc_1_check != 0) {
                core_generate_event(core, EVT_ID_FLASH_GENERATION_ERR, 0, SEVERITY_WARNING, 0, NULL, 0);
                return GENERATION_ERR;
            }
        }

	}
#endif
	if(action==IMAGE_UPDATE){
		for(i=0;i<2;i++){
			if(tmp_gen[i].parts.high==0xFFFFFFFF){
				decided++;
				invalid = i;
				continue;
			}
			valid = i;
		}
#ifdef FLASH_DEBUG
		FLASH_DPRINT("Y.L %s ... dual image update, decided = [0x%x]\n", __FUNCTION__, decided);
#endif
		if(decided>=2){
			// dual image's generation = 0xFFFFFFFF
			*target_addr = source_addr[1];
			p_base_gen->parts.high = 0;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
			p_base_gen->parts.low = gen[1].parts.low;
#endif
		}
		else if(decided==0){
			// dual image's generation valid check
			if(tmp_gen[0].parts.high<tmp_gen[1].parts.high){
				*target_addr = source_addr[0];
				p_base_gen->parts.high = tmp_gen[1].parts.high+1;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
				p_base_gen->parts.low = tmp_gen[1].parts.low;
#endif

			}
			else{
				*target_addr = source_addr[1];
				p_base_gen->parts.high = tmp_gen[0].parts.high+1;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
				p_base_gen->parts.low = tmp_gen[0].parts.low;
#endif
			}
		}else{
			*target_addr = source_addr[invalid];
			p_base_gen->parts.high = tmp_gen[valid].parts.high+1;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
			p_base_gen->parts.low = tmp_gen[valid].parts.low;
#endif
		}

#ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
		//generate Generation's CRC
		p_base_gen->parts.low = flash_image_generation_crc_calculate((MV_PU32)&p_base_gen->parts.high);
#endif
#ifdef FLASH_DEBUG
		FLASH_DPRINT("Y.L %s ... image update, dual image, target_addr[0x%x], base_gen[h:0x%x, l:0x%x]\n", __FUNCTION__, *target_addr,p_base_gen->parts.high,p_base_gen->parts.low);
#endif
		return FLASH_NONE_ERR;
	}

	for(i=0;i<2;i++){
		if(tmp_gen[i].parts.high==0xFFFFFFFF){
			decided++;
			invalid = i;
			continue;
		}
		valid = i;
	}
	if(decided>=2){
		// dual image's generation = 0xFFFFFFFF
		// should not come here.
		*target_addr = source_addr[0];
		p_base_gen->parts.high = tmp_gen[0].parts.high;
		p_base_gen->parts.low = tmp_gen[0].parts.low;
	}else if(decided==0){
		// dual image's generation valid check
		if(tmp_gen[0].parts.high<tmp_gen[1].parts.high){
			*target_addr = source_addr[1];
			p_base_gen->parts.high = tmp_gen[1].parts.high;
			p_base_gen->parts.low = tmp_gen[1].parts.low;
		}
		else{
			*target_addr = source_addr[0];
			p_base_gen->parts.high = tmp_gen[0].parts.high;
			p_base_gen->parts.low = tmp_gen[0].parts.low;
		}
	}else{
			*target_addr = source_addr[valid];
			p_base_gen->parts.high = tmp_gen[valid].parts.high;
			p_base_gen->parts.low = tmp_gen[valid].parts.low;
	}
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... dual image backup, target_addr[0x%x], gen[0x%x], crc[0x%x]\n", __FUNCTION__, *target_addr,p_base_gen->parts.high,p_base_gen->parts.low);
#endif
	return FLASH_NONE_ERR;
}

#ifdef NEW_IMAGE_UPDATE_FLOW
MV_BOOLEAN core_flash_image_addr_locate(
 	IN MV_PVOID core_p,
	IN MV_U8				type,
	IN MV_U8				action,
    OUT MV_BUFFER_DETAIL    *output_buf
	)
{
	core_extension  *core = (core_extension *)core_p;
	MV_U32	start_addr[2]={0,0};
	MV_U32	size[2] = {0,0};
	MV_U8	err_code = ERR_NONE;
#ifdef FLASH_IMAGE_HEADER_CHECK
    MV_U32  header_addr = 0;
	MV_U8	header_err = 0, data_err = 0;
    FLASH_IMAGE_HEADER  tmp_buffer, tmp_buffer1;
    MV_U32  last_id, last_offset;
#endif

    //Get image address and size info
    if (!core_get_image_start_addr(type, &start_addr[0], &size[0], MV_FALSE)){
        core_get_image_start_addr(type, &start_addr[0], &size[0], MV_TRUE);
    }

    //Since boot loader always boot from 1st image if it's correct.
    //If 1st image CRC error, it boot from 2nd image
    header_addr = start_addr[0]+size[0]-sizeof(FLASH_IMAGE_HEADER)-sizeof(FLASH_IMAGE_GENERATION);

#ifdef FLASH_IMAGE_HEADER_CHECK
#ifdef SUPPORT_ROC
    if (!Core_NVMRd(core, MFR_FLASH_DEV(0), header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer, 0)) {
    	FM_PRINT("%s Read Flash is Fail....\n", __FUNCTION__);
    	//*p_error_code = ERR_FLASH_IMAGE_HEADER_READ_FAILED;
    	return FLASH_READ_ERR;
    }
#else
    if(!Core_FlashRead(core, header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer))
    	return FLASH_READ_ERR;
#endif
	MV_ZeroMemory(&tmp_buffer1, sizeof(FLASH_IMAGE_HEADER));
	if (!core_flash_image_header_check(&tmp_buffer,&tmp_buffer1)) {
		//generate flash event: header check error
		FM_PRINT("%s core_flash_image_header_check is failed....\n", __FUNCTION__);
		header_err = 1;
	}
    
	//last buffer id
	last_id = ((start_addr[0] + size[0]) >> MAX_FLASH_BUFFER_CAPACITY_BIT_POWER) - 1;
	// here need calculate last buffer offset, currently we sure that last buffer offset = 0
	if(!core_flash_image_data_crc_check(core, (MV_PVOID)&tmp_buffer, last_id, 0, NULL)) {
		//generate flash event: header data CRC error
		FM_PRINT("%s core_flash_image_data_crc_check is failed....\n", __FUNCTION__);
		data_err = 1;
	}
#endif
    
	if (header_err || data_err) {
        //1st image corrupt, it boot from 2nd image
        //pick up 1st image
		//FM_PRINT("%s pick up 1st\n", __FUNCTION__);
	    output_buf->startBufferID = start_addr[0] >> MAX_FLASH_BUFFER_CAPACITY_BIT_POWER;
        output_buf->startBufferOffsetHigh = 0;
        output_buf->startBufferOffsetMiddle= 0;
        output_buf->startBufferOffsetLow = 0;
        output_buf->totalSizeHigh = (size[0] >> 16) & 0xFF;
        output_buf->totalSizeMiddle = (size[0] >> 8) & 0xFF;
        output_buf->totalSizeLow = (size[0]) & 0xFF;
        output_buf->Generation.parts.high = 0xFFFFFFFF;
        output_buf->Generation.parts.low = 0xFFFFFFFF;
	} else {
	    //1st image OK, it boot from 1st image
	    //pick up 2nd image
		//FM_PRINT("%s pick up 2nd\n", __FUNCTION__);
	    output_buf->startBufferID = start_addr[1] >> MAX_FLASH_BUFFER_CAPACITY_BIT_POWER;
        output_buf->startBufferOffsetHigh = 0;
        output_buf->startBufferOffsetMiddle= 0;
        output_buf->startBufferOffsetLow = 0;
        output_buf->totalSizeHigh = (size[1] >> 16) & 0xFF;
        output_buf->totalSizeMiddle = (size[1] >> 8) & 0xFF;
        output_buf->totalSizeLow = (size[1]) & 0xFF;
        output_buf->Generation.parts.high = 0xFFFFFFFF;
        output_buf->Generation.parts.low = 0xFFFFFFFF;
    }

    return FLASH_NONE_ERR;
}

#else
MV_BOOLEAN core_flash_image_addr_locate(
 	IN MV_PVOID core_p,
	IN MV_U8				type,
	IN MV_U8				action,
	OUT MV_PU8				p_start_id,
	OUT MV_PU8				p_offset_hi,
	OUT MV_PU8				p_offset_mid,
	OUT MV_PU8				p_offset_low,
	OUT MV_PU8				p_size_hi,
	OUT MV_PU8				p_size_mid,
	OUT MV_PU8				p_size_low,
	OUT MV_PU64				p_gen
	)
{
	core_extension  *core = (core_extension *)core_p;
	MV_U32	start_addr[2]={0,0};
	MV_U32	size[2] = {0,0};
	MV_U32	target_addr;
	MV_U64	base_gen;
	MV_U32	gen_addr;
	MV_U64	gen;
	MV_U8	err_code = ERR_NONE;
	MV_U8	check_0 = 0,check_1=0;
#ifdef FLASH_IMAGE_HEADER_CHECK
	MV_U32	header_addr = 0;
	FLASH_IMAGE_HEADER	tmp_buffer,tmp_buffer1;
	MV_U32	last_id,last_offset;
	MV_U64	tmp_gen;
	MV_U32	tmp_gen_addr = 0;
#endif
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... \n", __FUNCTION__);
#endif
	// first use flash description layout, if description layout was crashed or not correct, use default value

    if (!core_get_image_start_addr(type, &start_addr[0], &size[0], MV_FALSE)){
        core_get_image_start_addr(type, &start_addr[0], &size[0], MV_TRUE);
    }
    if (!is_dual_image(type)) {
        *p_start_id = start_addr[0]>>MAX_FLASH_BUFFER_CAPACITY_BIT_POWER;
        *p_offset_hi = 0;
        *p_offset_mid = 0;
        *p_offset_low = 0;
        *p_size_hi = (size[0]>>16)&0xFF;
        *p_size_mid = (size[0]>>8)&0xFF;
        *p_size_low = size[0]&0xFF;
		//U64_SET_VALUE(*p_gen,0);
#ifdef FLASH_DEBUG
		FLASH_DPRINT("Y.L %s ... single image type[0x%x], start_id[0x%x], size[hi:mid:low][0x%x:%x:%x]\n", __FUNCTION__,type, *p_start_id,*p_size_hi,*p_size_mid,*p_size_low);
		FLASH_DPRINT("Y.L %s ... action[0x%x], single image, start_addr[0x%x], size[0x%x] \n", __FUNCTION__, action, start_addr[0],size[0]);
#endif
        gen_addr = start_addr[0]+size[0]-GENERATION_SIZE;
#ifdef FLASH_DEBUG
		FLASH_DPRINT("Y.L %s ... single image, generation_addr[0x%x]\n", __FUNCTION__, gen_addr);
#endif
        U64_ZERO_VALUE(gen);
        if(!Core_NVMRd(core, MFR_FLASH_DEV(0), gen_addr, GENERATION_SIZE, (MV_PU8)&gen, 0)){
            FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
            return FLASH_READ_ERR;
        }
	//check generation CRC
#ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
	if(!flash_image_generation_crc_check(&gen.parts.high,&gen.parts.low)){
        // generate flash event: Generation CRC error
        //EVT_ID_FLASH_GENERATION_ERR_IN_FLASH
        //core_generate_event(ext, eid, did, slv, pc, ptr, tran)
        //JL 2012/04/17 if gen CRC or image CRC error and action is Update
        //This means it's corruption, we can use this area first.
        if (action == IMAGE_UPDATE) {
            if (gen.parts.high == 0xFFFFFFFF) {
                p_gen->parts.high = 0;
            #ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
                // low is the generation's CRC
                p_gen->parts.low = gen.parts.low;
            #endif
            } else {
                p_gen->parts.high = gen.parts.high + 1;
            #ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
                // low is the generation's CRC
                p_gen->parts.low = gen.parts.low;
            #endif
            }
        #ifdef FLASH_IMAGE_GENERATION_CRC_SUPPORT
            p_gen->parts.low = flash_image_generation_crc_calculate((MV_PU32)&p_gen->parts.high);
        #endif
        #ifdef FLASH_DEBUG
            FLASH_DPRINT("Y.L %s ... image update, single image, update to new generation[0x%x,0x%x]\n", __FUNCTION__, p_gen->parts.high, p_gen->parts.low);
        #endif
            return FLASH_NONE_ERR;
        } else if (action == IMAGE_BACKUP) {
            FM_PRINT("%s ... flash image's generation crc is not correct, so abort the request!, crc = 0x%x, generation= 0x%x\n", __FUNCTION__,gen.parts.low,gen.parts.high);
            return GENERATION_ERR;
        }
	}
#endif
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... single image read from flash, generation[0x%x,0x%x]\n", __FUNCTION__, gen.parts.high,gen.parts.low);
#endif
    if (action == IMAGE_UPDATE) {
        if (gen.parts.high == 0xFFFFFFFF) {
            p_gen->parts.high = 0;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
            // low is the generation's CRC
            p_gen->parts.low = gen.parts.low;
#else
            p_gen->parts.low = flash_image_generation_crc_calculate((MV_PU32)&p_gen->parts.high);
#endif
        } else {
            p_gen->parts.high = gen.parts.high+1;
#ifndef FLASH_IMAGE_GENERATION_CRC_SUPPORT
            // low is the generation's CRC
            p_gen->parts.low = gen.parts.low;
#else
            p_gen->parts.low = flash_image_generation_crc_calculate((MV_PU32)&p_gen->parts.high);
#endif
        }
#ifdef FLASH_DEBUG
        FLASH_DPRINT("Y.L %s ... image update, single image, update to new generation[0x%x,0x%x]\n", __FUNCTION__, p_gen->parts.high,p_gen->parts.low);
#endif
        return FLASH_NONE_ERR;
    }
    
    if (action == IMAGE_BACKUP) {
        p_gen->parts.high = gen.parts.high;
        p_gen->parts.low = gen.parts.low;
#ifdef FLASH_DEBUG
        FLASH_DPRINT("Y.L %s ... image backup, single image, generation[0x%x,0x%x]\n", __FUNCTION__,p_gen->parts.high,p_gen->parts.low);
#endif
	}

#ifdef FLASH_IMAGE_HEADER_CHECK
		// CRC check for backup image
		if (action == IMAGE_BACKUP) {
			header_addr = start_addr[0]+size[0]-sizeof(FLASH_IMAGE_HEADER)-sizeof(FLASH_IMAGE_GENERATION);
#ifdef SUPPORT_ROC
			if (!Core_NVMRd(core, MFR_FLASH_DEV(0), header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer, 0)) {
				FM_PRINT("%s Read Flash is Fail....\n", __FUNCTION__);
				//*p_error_code = ERR_FLASH_IMAGE_HEADER_READ_FAILED;
				return FLASH_READ_ERR;
			}
#else
			if(!Core_FlashRead(core, header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer))
				return FLASH_READ_ERR;
#endif
			// if Adapter Device ID = 0xFFFF and Image Length = 0xFFFF and type =0xFF, CRC=0xFFFFFFFF, we considered that there is no header
            if ((tmp_buffer.crc == 0xFFFFFFFF)
                && (tmp_buffer.adapter_device_id == 0xFFFF)
                && (tmp_buffer.type == 0xFF)) {
				global_header_check = 0;
				FM_PRINT("%s Attention: this image has no header, so bypass image data CRC check!....\n", __FUNCTION__);
			}
			if(global_header_check){
				MV_ZeroMemory(&tmp_buffer1, sizeof(FLASH_IMAGE_HEADER));
				if(!core_flash_image_header_check(&tmp_buffer,&tmp_buffer1)){
					//generate flash event: header check error
					FM_PRINT("%s core_flash_image_header_check is failed....\n", __FUNCTION__);
					check_0 = 1;
				}
				//last buffer id
#ifdef FLASH_DEBUG
				FLASH_DPRINT("%s ... flash image's header is valid, header crc =[0x%x]\n", __FUNCTION__,tmp_buffer.crc);
#endif
				last_id = ((start_addr[0]+size[0])>>MAX_FLASH_BUFFER_CAPACITY_BIT_POWER)-1;
				// here need calculate last buffer offset, currently we sure that last buffer offset = 0
				if(!core_flash_image_data_crc_check(core, (MV_PVOID)&tmp_buffer,last_id,0,NULL)){
					//generate flash event: header data CRC error
					FM_PRINT("%s core_flash_image_data_crc_check is failed....\n", __FUNCTION__);
					check_1 = 1;
				}
				if (check_0 != 0 || check_1 != 0) {
					// restore generation as 0xFFFFFFFF
					U64_ZERO_VALUE(tmp_gen);
					tmp_gen_addr = start_addr[0]+size[0]-GENERATION_SIZE;
					RESTORE_GENERATION(tmp_gen);
#ifdef SUPPORT_ROC
				if (!Core_NVMWr(core, MFR_FLASH_DEV(0), tmp_gen_addr, GENERATION_SIZE, (MV_PU8)&tmp_gen, 0)) {
					FM_PRINT("%s Read Flash is Fail....\n", __FUNCTION__);
					return FLASH_WRITE_ERR;
				}
#else
				if(!Core_FlashWrite(core, header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer))
					return FLASH_WRITE_ERR;
#endif
				if(check_0&&check_1)
					return IMAGE_HEADER_AND_DATA_ERR;
				else if(check_0)
					return IMAGE_HEADER_ERR;
				else if(check_1)
					return IMAGE_DATA_ERR;
				}
			}
		}
#endif
		return FLASH_NONE_ERR;
	}
	// dual image
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... action[0x%x], dual image, pri_addr[0x%x], pri_size[0x%x], sec_addr[0x%x], sec_size[0x%x] \n", __FUNCTION__, action, start_addr[0],size[0],start_addr[1],size[1]);
#endif

	err_code = core_flash_dual_image_addr_arbitrate(core, action, &start_addr[0], &size[0], &target_addr, &base_gen);
	if(err_code!=FLASH_NONE_ERR)
	{
		return err_code;
	}

#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... dual image type[0x%x], target_addr[0x%x], size[0x%x], base_gen[0x%x], gen_crc[0x%x]\n", __FUNCTION__,type, target_addr,size[0],base_gen.parts.high,base_gen.parts.low);
#endif

	*p_start_id = target_addr>>MAX_FLASH_BUFFER_CAPACITY_BIT_POWER;
	*p_offset_hi = 0;
	*p_offset_mid = 0;
	*p_offset_low = 0;
	*p_size_hi = (size[0]>>16)&0xFF;
	*p_size_mid = (size[0]>>8)&0xFF;
	*p_size_low = size[0]&0xFF;
	p_gen->parts.high = base_gen.parts.high;
	p_gen->parts.low = base_gen.parts.low;

#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ... dual image type[0x%x], start_id[0x%x], size[hi:mid:low][0x%x:%x:%x]\n", __FUNCTION__,type, *p_start_id,*p_size_hi,*p_size_mid,*p_size_low);
#endif

#ifdef FLASH_IMAGE_HEADER_CHECK
	// CRC check for backup image
	if(action==IMAGE_BACKUP){
			header_addr = start_addr[0]+size[0]-sizeof(FLASH_IMAGE_HEADER)-sizeof(FLASH_IMAGE_GENERATION);
#ifdef SUPPORT_ROC
			if (!Core_NVMRd(core, MFR_FLASH_DEV(0), header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer, 0)) {
				FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
				return FLASH_READ_ERR;
			}
#else
			if(!Core_FlashRead(core, header_addr, MAX_FLASH_BUFFER_CAPACITY, (MV_PU8)&tmp_buffer))
				return FLASH_READ_ERR;
#endif
		// if Adapter Device ID = 0xFFFF and Image Length = 0xFFFF and type =0xFF, CRC=0xFFFFFFFF, we considered that there is no header
		if ((tmp_buffer.crc == 0xFFFFFFFF)
            && (tmp_buffer.adapter_device_id==0xFFFF)
            && (tmp_buffer.type==0xFF)) {
            global_header_check = 0;
            FM_PRINT("%s Attention: this image has no header, so bypass image data CRC check!....\n", __FUNCTION__);
		}

		if (global_header_check) {
			if(!core_flash_image_header_check(&tmp_buffer,&tmp_buffer1)){
				// generate flash event: header check error
				FM_PRINT("%s core_flash_image_header_check is failed....\n", __FUNCTION__);
				check_0 = 1;
			}
			//last buffer id
			last_id = ((start_addr[0]+size[0])>>MAX_FLASH_BUFFER_CAPACITY_BIT_POWER)-1;
			if(!core_flash_image_data_crc_check(core, (MV_PVOID)&tmp_buffer,last_id,0,NULL)){
				// generate flash event: header data CRC error
				FM_PRINT("%s core_flash_image_data_crc_check is failed....\n", __FUNCTION__);
				check_1 = 1;
			}
			if(check_0!=0||check_1!=0){
				// restore generation as 0xFFFFFFFF
				U64_ZERO_VALUE(tmp_gen);
				tmp_gen_addr = start_addr[0]+size[0]-GENERATION_SIZE;
				RESTORE_GENERATION(tmp_gen);
#ifdef SUPPORT_ROC
				if (!Core_NVMWr(core, MFR_FLASH_DEV(0), tmp_gen_addr, GENERATION_SIZE, (MV_PU8)&tmp_gen, 0)) {
					FM_PRINT("%s Read Flash is Fail....\n", __FUNCTION__);
					return FLASH_WRITE_ERR;
				}
#else
				if(!Core_FlashWrite(core, header_addr, sizeof(FLASH_IMAGE_HEADER), (MV_PU8)&tmp_buffer))
					return FLASH_WRITE_ERR;
#endif
				if(check_0&&check_1)
					return IMAGE_HEADER_AND_DATA_ERR;
				else if(check_0)
					return IMAGE_HEADER_ERR;
				else if(check_1)
					return IMAGE_DATA_ERR;
			}
		}
	}
#endif
	return FLASH_NONE_ERR;

}
#endif //NEW_IMAGE_UPDATE_FLOW

#define IS_FLASH_ACTION_VALID(act)	((act==IMAGE_UPDATE)||(act=IMAGE_BACKUP))
MV_U8 core_get_flash_image_detail_info(
 	IN MV_PVOID core_p,
	IN PMV_Request p_req
	)
{
	core_extension  *core = (core_extension *)core_p;
	/* get all type of flash */
	MV_U8	err_code = FLASH_NONE_ERR;
	MV_U8	type, action;
	MV_BUFFER_DETAIL tmp_buffer;

#ifdef FLASH_DEBUG
	MV_PTR_INTEGER core_ext_addr = NULL;
	FLASH_DPRINT("Y.L %s ...Cdb[%x,%x,%x,%x] \n", __FUNCTION__,p_req->Cdb[0],p_req->Cdb[1],p_req->Cdb[2],p_req->Cdb[3]);
#endif
	MV_ZeroMemory(&tmp_buffer, sizeof(MV_BUFFER_DETAIL));
	tmp_buffer.PathID = 0;
	tmp_buffer.Lun = 0;
	tmp_buffer.TargetID= VIRTUAL_DEVICE_ID;
	tmp_buffer.VirtualID = VIRTUAL_DEVICE_ID;

	action = p_req->Cdb[2];
	type = p_req->Cdb[3];
	if(!IS_FLASH_ACTION_VALID(action)){
		FM_PRINT("Y.L %s ...invalid action [0x%x]. \n", __FUNCTION__,action);
		p_req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	if(!buffer_type_check(type)){
		FM_PRINT("Y.L %s ...Unsupport Buffer Type [0x%x]. \n", __FUNCTION__,type);
		p_req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

    #ifdef NEW_IMAGE_UPDATE_FLOW
    err_code = core_flash_image_addr_locate(core, type, action, &tmp_buffer);
    #else
	err_code = core_flash_image_addr_locate(core, type, action, &tmp_buffer.startBufferID, \
				&tmp_buffer.startBufferOffsetHigh, &tmp_buffer.startBufferOffsetMiddle, &tmp_buffer.startBufferOffsetLow, \
					&tmp_buffer.totalSizeHigh, &tmp_buffer.totalSizeMiddle, &tmp_buffer.totalSizeLow, &tmp_buffer.Generation);
    #endif
	if(err_code!=FLASH_NONE_ERR){
		// generate event for all
		flash_err_event_generate(core, err_code);
		if(action==IMAGE_UPDATE)
			core_generate_event(core, EVT_ID_FLASH_WRITE_IMAGE_FAILED, 0, SEVERITY_WARNING,  0,  NULL, 0 );
		if(action==IMAGE_BACKUP)
			core_generate_event(core, EVT_ID_FLASH_READ_IMAGE_FAILED, 0, SEVERITY_WARNING,  0,  NULL, 0 );
		MV_CopyMemory((MV_PU8)p_req->Sense_Info_Buffer,&err_code,sizeof(MV_U8));
		p_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	MV_CopyMemory(p_req->Data_Buffer,&tmp_buffer,sizeof(MV_BUFFER_DETAIL));
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ...image type [0x%x], start_id[0x%x], size[hi:mid:low][0x%x:%x:%x], generation[h:0x%x, l:0x%x]. \n", __FUNCTION__,type,tmp_buffer.startBufferID,tmp_buffer.totalSizeHigh,tmp_buffer.totalSizeMiddle,tmp_buffer.totalSizeLow,tmp_buffer.Generation.parts.high,tmp_buffer.Generation.parts.low);
	core_ext_addr = (MV_PTR_INTEGER)core;
	FLASH_DPRINT("Y.L %s ...core_ext_addr = 0x%x\n", __FUNCTION__, core_ext_addr);
#endif
	p_req->Scsi_Status=REQ_STATUS_SUCCESS;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

#endif /* SUPPORT_FLASH_CMD_VIA_SCSI_RW_BUFFER */

#endif /* SUPPORT_FLASH */

