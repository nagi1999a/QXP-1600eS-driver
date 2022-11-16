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

/* ;This file is discarded after POST. */
/*******************************************************************************
*
*                   Copyright 2006,MARVELL SEMICONDUCTOR ISRAEL, LTD.
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
********************************************************************************
* com_flash.c - File for implementation of the Driver Intermediate Application Layer
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*   mv_include.h
*
* FILE REVISION NUMBER:
*       $Revision: 1.1.1.1 $
*******************************************************************************/
#ifndef SIMULATOR
#include "mv_include.h"
#include "core_header.h"
#include "core_spi.h"
#include "com_nvram.h"
#include "hba_inter.h"
#include "hba_exp.h"
#ifdef SUPPORT_ROC
	#ifdef NEW_CORE_DRIVER
		#include "core_flash.h"
	#else
		#include "roc_exp.h"
		#include "core_nvm.h"
	#endif
	#include "flash.h"
	#include "com_flash.h"
#endif
#ifdef _OS_UKRN
#include "../../../inc/loader/img_creator.h"
#endif
#ifdef SCSI_RW_BUFFER_CMD
#include "core_util.h"
#endif
//#define EVENT_PRINT 	MV_DPRINT 
//#define EVENT_PRINT(x) MV_PRINT x
#define EVENT_PRINT(x)

#ifdef SCSI_RW_BUFFER_CMD
void flash_err_event_generate( MV_VOID *  pcore, MV_U8 err_type){
	core_extension *  p_core = ( core_extension * )pcore;
	MV_U32	evt_id = 0;
	switch(err_type){
		case FLASH_WRITE_ERR:
			evt_id = EVT_ID_FLASH_WRITE_ERR;
			break;
		case FLASH_READ_ERR:
			evt_id = EVT_ID_FLASH_READ_ERR;
			break;
		case GENERATION_ERR:
			evt_id = EVT_ID_FLASH_GENERATION_ERR;
			break;
		case FATAL_GENERATION_ERR:
			evt_id = EVT_ID_FLASH_FATAL_GENERATION_ERR;
			break;
		case IMAGE_DATA_ERR:
			evt_id = EVT_ID_FLASH_DATA_ERR;
			break;
		case IMAGE_HEADER_ERR:
			evt_id = EVT_ID_FLASH_HEADER_ERR;
			break;
		case IMAGE_HEADER_AND_DATA_ERR:
			evt_id = EVT_ID_FLASH_DATA_AND_HEADER_ERR;
			break;
		default:
		FM_PRINT("Y.L:  %s %d %s : Unsupport flash error type, 0x%x\n", __FILE__, __LINE__, __FUNCTION__,err_type);

		return;
	}
	core_generate_event(p_core, evt_id, 0, SEVERITY_WARNING, 0, 0, 0);
}
#endif
#if defined(ADDING_FLASH_LAYOUT_DESC)
#define MV_StrLen(source)	(int)strlen(source)

const char flash_image_component_name[FLASH_IMAGE_MAX][20] = {
    "Config",          /*FLASH_IMAGE_CONFIG*/ /*The config had merged the HBA.txt and Layout Description */
    "HBA_Info",      /*FLASH_IMAGE_HBAINFO*/
    "FMLoader",      /*FLASH_IMAGE_FMLOADER*/
    "Firmware",      /*FLASH_IMAGE_FIRMWARE*/
    "BIOS",            /*FLASH_IMAGE_BIOS*/
    "Event_Log",     /*FLASH_IMAGE_EVENTLOG*/
    "PD_Page",       /*FLASH_IMAGE_PDPAGE*/
    "SingleBlock",   /*FLASH_IMAGE_SINGLEBLOCK*/
    "FlashSize"       /*FLASH_IMAGE_FLASHSIZE*/
};

static MV_U32 STR_TO_HEX_U32(char *Strbuf, MV_PU32 pSUM)
{
	if ((*Strbuf >= '0') && (*Strbuf <= '9')) {
		MV_U32 ReturnValue = STR_TO_HEX_U32((Strbuf + 1), pSUM);
		*pSUM += (*Strbuf - 0x30) * ReturnValue;
		return ReturnValue * 0x10;
	} else if ((*Strbuf >= 'a') && (*Strbuf <= 'f')) {
		MV_U32 ReturnValue = STR_TO_HEX_U32((Strbuf + 1), pSUM);
		*pSUM += (*Strbuf - 0x57) * ReturnValue;
		return ReturnValue * 0x10;
	} else if ((*Strbuf >= 'A') && (*Strbuf <= 'F')) {
		MV_U32 ReturnValue = STR_TO_HEX_U32((Strbuf + 1), pSUM);
		*pSUM += (*Strbuf - 0x37) * ReturnValue;
		return ReturnValue * 0x10;
	} else {
		*pSUM = 0;
		return 1;
	}
}

static MV_U32 STR_TO_DEC_U32(char *Strbuf, MV_PU32 pSUM)
{
	if ((*Strbuf >= 0x30) && (*Strbuf <= 0x39)) {
		MV_U32 ReturnValue = STR_TO_DEC_U32((Strbuf + 1),pSUM);
		*pSUM += (*Strbuf % 0x10) * ReturnValue;
		return ReturnValue * 10;
	}
	else {
		*pSUM = 0;
		return 1;
	}
}

static void *flash_layout_head_ptr = NULL;
#define INITIAL_FLASH_LAYOUT_BUFF_SIZE    128
#ifdef _OS_UKRN
#define FLASH_LAYOUT_MAP_SIZE (sizeof(flash_map_tbl_t))
int have_flash_layout = 0;
MV_VOID initial_flash_layout_for_frey(void)
{
    MV_U8 layout_buffer[FLASH_LAYOUT_MAP_SIZE];
    flash_map_tbl_t *ptr_layout_tbl = (flash_map_tbl_t *)layout_buffer;
    flash_layout_head_ptr = FLASH_LAYOUT_OFFSET_G2;

    if (!Core_NVMRd( NULL, MFR_FLASH_DEV(0), (MV_U32)flash_layout_head_ptr, sizeof(flash_map_tbl_t), (MV_PU8)ptr_layout_tbl, 0)) {
        FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
    }
    if(_crc32(~0, (void *)ptr_layout_tbl, sizeof(flash_map_tbl_t) - sizeof(u32)) == ptr_layout_tbl->CRC) {
        FM_PRINT("\nThe Layout Description has been find at 0x%lx\n", flash_layout_head_ptr);
        have_flash_layout = 1;
    }
    else {
        FM_PRINT("\nThe Layout Description can not be found at 0x%lx\n", flash_layout_head_ptr);
        have_flash_layout = 0;
    }
}
#else
int have_flash_layout = 1;	//TBD
#endif

MV_VOID initial_flash_layout(void)
{
    MV_U32 ptr_buffer[INITIAL_FLASH_LAYOUT_BUFF_SIZE/4];
    MV_U32 step = 0;
    struct flash_layout_head *ptr_layout_header = (struct flash_layout_head *)ptr_buffer;
    MV_U32 size, temp;
    MV_U32 crc, temp_crc;
    MV_U32 ret;
    MV_U32 processing_ptr, keep_this_ptr;

#ifdef _OS_UKRN
    initial_flash_layout_for_frey();
    return;
#endif
    if ((MV_U32)flash_layout_head_ptr== 0xFFFFFFFFUL) {
        FM_PRINT("Firmware didn't find the Layout Description, Please Make sure it had existed on Flash by aligned 0x%X.\n", step);
        return;
    }
    
    if (flash_layout_head_ptr)
        return;
    
    flash_layout_head_ptr = FLASH_LAYOUT_START_ADDR;

    step = FLASH_LAYOUT_SIZE/2;
    while (1) {
        if ((MV_U32)flash_layout_head_ptr >= (FLASH_LAYOUT_OFFSET << 2)) { /*x 4 = 4M*/
            step/= 2;
            if (step<=(3*1024)) {
                flash_layout_head_ptr = ((MV_PVOID)0xFFFFFFFF);
                FM_PRINT("Firmware didn't find the Layout Description, Please Make sure it had existed on Flash by aligned 0x%X.\n", step);
                break;
            }
            flash_layout_head_ptr = ((MV_PVOID)(FLASH_LAYOUT_START_ADDR+step));
        }
        
        if (!Core_NVMRd( NULL, MFR_FLASH_DEV(0), (MV_U32)flash_layout_head_ptr, sizeof(struct flash_layout_head), (MV_PU8)ptr_layout_header, 0)) {
            FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        } else if (MV_CompareMemory((MV_PU8)ptr_layout_header, FLASH_LAYOUT_SIGN, FLASH_LAYOUT_SIGN_LENGTH)) { 
        } else {
            FM_PRINT("The Header is matching the %s sign.<0x%lx>\n", FLASH_LAYOUT_SIGN, (MV_U32)flash_layout_head_ptr);
            size = (MV_U32)&(((struct flash_layout_head *)0)->item);
            temp_crc = ptr_layout_header->crc;
            ptr_layout_header->crc = MV_MAX_U32;
            crc = MV_CRC_EXT(MV_MAX_U32, (MV_PU8)ptr_layout_header, size);

            processing_ptr = (MV_U32)flash_layout_head_ptr + size;
            keep_this_ptr  = (MV_U32)flash_layout_head_ptr + ptr_layout_header->next_tbl_off;
            
            if (ptr_layout_header->length>=(FLASH_LAYOUT_SIZE-processing_ptr)) {
                FM_PRINT("The Layout Description at 0x%lx has illegal size\n", flash_layout_head_ptr);
                break;
            }
            size = ptr_layout_header->length - size;
            while (size > 0) {
                temp = MV_MIN(INITIAL_FLASH_LAYOUT_BUFF_SIZE, size);
                if (!Core_NVMRd( NULL, MFR_FLASH_DEV(0), processing_ptr, temp, (MV_PU8)ptr_buffer, 0)) {
                    FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                    break;
                }
                crc = MV_CRC_EXT(crc, (MV_PU8)ptr_buffer, temp);
                processing_ptr+= temp;
                size-= temp;
            }
            crc = MV_CPU_TO_BE32(crc);
    	        
            if (crc == temp_crc) {
                flash_layout_head_ptr = ((MV_PVOID)keep_this_ptr);
                FM_PRINT("The Layout Description is at 0x%lx\n", flash_layout_head_ptr);
                break;
            } else {
                MV_DPRINT(("CRC Check sum is Fail<0x%lx, 0x%lx>.\n", crc, temp_crc));
            }
        }
        flash_layout_head_ptr = (char*)flash_layout_head_ptr + step;
    }
    FM_PRINT("\nThe Layout Description has been find at 0x%lx\n", flash_layout_head_ptr);
}

MV_VOID initial_flash_layout_sl(void)
{
    MV_U8 ptr_buffer[INITIAL_FLASH_LAYOUT_BUFF_SIZE], step = 0;
    struct flash_layout_head *ptr_layout_header = (struct flash_layout_head *)ptr_buffer;
    MV_U32 size, i, temp;
    MV_U32 crc, temp_crc;
    void *processing_ptr, *keep_this_ptr;

    flash_layout_head_ptr = (void *)FLASH_LAYOUT_START_ADDR;

    while(1) {

        if(!Core_NVMRd( NULL, MFR_FLASH_DEV(0), ((MV_U32)flash_layout_head_ptr), sizeof(struct flash_layout_head), (MV_PU8)ptr_layout_header, 0)) {
            FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
            MV_ASSERT(0);
            return;
        }

        if (!MV_CompareMemory((MV_PU8)ptr_layout_header, FLASH_LAYOUT_SIGN, FLASH_LAYOUT_SIGN_LENGTH)) {
            FM_PRINT("The Header is matching the %s sign.<0x%lx>\n", FLASH_LAYOUT_SIGN, (MV_U32)flash_layout_head_ptr);
            size = (MV_U32)&(((struct flash_layout_head *)0)->item);
            temp_crc = ptr_layout_header->crc;
            ptr_layout_header->crc = MV_MAX_U32;
            crc = MV_CRC_EXT(MV_MAX_U32, (MV_PU8)ptr_layout_header, size);
        
            processing_ptr = (void *)((MV_U32)flash_layout_head_ptr + size);
            keep_this_ptr  = (void *)((MV_U32)flash_layout_head_ptr + ptr_layout_header->next_tbl_off);
            size = ptr_layout_header->length - size;
     
            for (i = 0; i < size; ) {
                temp = ((i + INITIAL_FLASH_LAYOUT_BUFF_SIZE) > size) ? (size - i) : INITIAL_FLASH_LAYOUT_BUFF_SIZE;
                if(!Core_NVMRd( NULL, MFR_FLASH_DEV(0), ((MV_U32)((MV_PU8)processing_ptr + i)), temp, (MV_PU8)ptr_buffer, 0)) {
                    FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                    MV_ASSERT(0);
                    return;
                }
                crc = MV_CRC_EXT(crc, ptr_buffer, temp);
                i += temp;
            }
            crc = MV_CPU_TO_BE32(crc);
        
            if(crc == temp_crc) {
                flash_layout_head_ptr = keep_this_ptr;
                FM_PRINT("The Layout Description is at 0x%lx\n", (MV_PU8)flash_layout_head_ptr);
                break;
            } else {
                MV_DPRINT(("CRC Check sum is Fail<0x%lx, 0x%lx>.\n", crc, temp_crc));
            }
        } 

        switch (step) {
        case 0:
            flash_layout_head_ptr = (void *)((MV_U32)flash_layout_head_ptr + FLASH_LAYOUT_OFFSET); /*+ 1M*/
            if ((MV_U32)flash_layout_head_ptr >= (FLASH_LAYOUT_OFFSET << 2)) { /*x 4 = 4M*/
                 flash_layout_head_ptr = (void *)FLASH_LAYOUT_START_ADDR;
                 step = 1;
                 FM_PRINT("Firmware didn't find the Layout Description, Please Make sure it had existed on Flash by aligned 0x100000(1M).\n");
            }
            break;
        case 1:
            flash_layout_head_ptr = (void *)((MV_U32)flash_layout_head_ptr + 0x20000 ); /*+ 128K*/
            if ((MV_U32)flash_layout_head_ptr >= (FLASH_LAYOUT_OFFSET << 2)) { /*x 4 = 4M*/
                 flash_layout_head_ptr = (void *)FLASH_LAYOUT_START_ADDR;
                 step = 2;
                 FM_PRINT("Firmware didn't find the Layout Description, Please Make sure it had existed on Flash by aligned 0x20000(128K).\n");
            }
            break;
        case 2:
        default:
            flash_layout_head_ptr = (void *)((MV_U32)flash_layout_head_ptr + sizeof(MV_U32));
            break;
        }
    }
    FM_PRINT("\nThe Layout Description has been find at 0x%lx\n", (MV_PU8)flash_layout_head_ptr);
}

#define MAX_LAYOUT_DESCRIPTION_STRING_SIZE  (2*1024)
#define FLASH_LAYOUT_BUFF_STRING_SIZE    128

#ifdef _OS_UKRN
MV_U32 default_flash_layout(MV_U32 component_type, MV_U16 component_number, MV_U16 component_return_type)
{
    MV_U32 return_address = 0xffffffffL;
    MV_U32 return_size = 0xffffffffL;
    switch (component_type) {
    case FLASH_IMAGE_HBAINFO:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            if (0 == component_number) {
                return_size = PRIMARY_HBA_INFO_SIZE;
            }
            else {
                return_size = SECONDARY_HBA_INFO_SIZE;
            }
        }
        else {
            if (0 == component_number) {
                return_address = PRIMARY_HBA_INFO_OFFSET;
             }
             else {
                return_address = SECONDARY_HBA_INFO_OFFSET;
             }
        }
        break;

    case FLASH_IMAGE_PDINFOPAGE:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = PD_PAGE_SIZE_G2;
        }
        else {
            return_address = PD_PAGE_OFFSET_G2;
        }
        break;

    case FLASH_IMAGE_CONFIG:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = CONFIG_DATA_SIZE;
        }
        else {
            return_address = CONFIG_DATA_OFFSET;
        }
        break;

    case FLASH_IMAGE_EVENTLOG:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = EVENT_LOG_SIZE;
        }
        else {
            return_address = EVENT_LOG_OFFSET;
        }
        break;

    case FLASH_IMAGE_FIRMWARE:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            if (0 == component_number) {
                return_size = FW_IMAGE0_SIZE;
            }
            else {
                return_size = FW_IMAGE1_SIZE;
            }
        }
        else {
            if (0 == component_number) {
                return_address = FW_IMAGE0_OFFSET;
            }
            else {
                return_address = FW_IMAGE1_OFFSET;
            }
        }
        break;

    case FLASH_IMAGE_BIOS:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = UMI_BIOS_SIZE;
        }
        else {
            return_address = UMI_BIOS_OFFSET;
        }
        break;

    case FLASH_IMAGE_SINGLEBLOCK:
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_A_BLOCK)) {
            FM_PRINT("%s %d %s ... Get Flash block size is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }
        FM_PRINT("One block size is 0x%lx in Flash.\n", return_size);
        break;

    case FLASH_IMAGE_FLASHSIZE:
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_ALL)) {
            FM_PRINT("%s %d %s ... Get Flash Capacity is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }
        FM_PRINT("Flash size is 0x%lx.\n", return_size);
        break;

    case FLASH_IMAGE_FMLOADER:
        if (FLASH_IMAGE_RETURN_ADDRESS == component_return_type) {
            return_address = 0;
        }
        break;

    default:
        FM_PRINT("%s %d %s ... bad type....\n", __FILE__, __LINE__, __FUNCTION__);
        break;
    }

    if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
        return return_size;
    }
    else {
        return return_address;
    }
}

MV_U32 return_image_place_on_flash_for_frey(MV_U32 component_type, MV_U16 component_number, MV_U16 component_return_type)
{
    MV_U8 layout_buffer[FLASH_LAYOUT_MAP_SIZE];
    MV_U32 return_address = 0xffffffffL;
    MV_U32 return_size = 0xffffffffL;
    MV_U32 ret;
    flash_map_tbl_t *ptr_layout_tbl = (flash_map_tbl_t *)layout_buffer;
    if (flash_layout_head_ptr == NULL) {
        initial_flash_layout();
    }
    if (!have_flash_layout) {
        ret = default_flash_layout(component_type, component_number, component_return_type);
        return ret;
    }
    if(!Core_NVMRd( NULL, MFR_FLASH_DEV(0), ((MV_U32)(MV_PU8)flash_layout_head_ptr), FLASH_LAYOUT_MAP_SIZE, (MV_PU8)ptr_layout_tbl, 0)) {
        FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        return 0xffffffffL;
    }

    switch (component_type) {
    case FLASH_IMAGE_HBAINFO:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            if (0 == component_number) {
                return_size = ptr_layout_tbl->prime_hba_info_size;
            }
            else {
                return_size = ptr_layout_tbl->second_hba_info_size;
            }
        }
        else {
            if (0 == component_number) {
                return_address = ptr_layout_tbl->prime_hba_info_offset;
             }
             else {
                return_address = ptr_layout_tbl->second_hba_info_offset;
             }
        }
        break;

    case FLASH_IMAGE_PDINFOPAGE:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = ptr_layout_tbl->pd_page_size;
        }
        else {
            return_address = ptr_layout_tbl->pd_page_offset;
        }
        break;

    case FLASH_IMAGE_CONFIG:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = ptr_layout_tbl->config_data_size;
        }
        else {
            return_address = ptr_layout_tbl->config_data_offset;
        }
        break;

    case FLASH_IMAGE_EVENTLOG:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = ptr_layout_tbl->event_log_size;
        }
        else {
            return_address = ptr_layout_tbl->event_log_offset;
        }
        break;

    case FLASH_IMAGE_FIRMWARE:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            if (0 == component_number) {
                return_size = ptr_layout_tbl->fw_primary_image_size;
            }
            else {
                return_size = ptr_layout_tbl->fw_secondary_image_size;
            }
        }
        else {
            if (0 == component_number) {
                return_address = ptr_layout_tbl->fw_primary_image_offset;
            }
            else {
                return_address = ptr_layout_tbl->fw_secondary_image_offset;
            }
        }
        break;

    case FLASH_IMAGE_BIOS:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = ptr_layout_tbl->umi_bios_size;
        }
        else {
            return_address = ptr_layout_tbl->umi_bios_offset;
        }
        break;

    case FLASH_IMAGE_SINGLEBLOCK:
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_A_BLOCK)) {
            FM_PRINT("%s %d %s ... Get Flash block size is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }
        FM_PRINT("One block size is 0x%lx in Flash.\n", return_size);
        break;

    case FLASH_IMAGE_FLASHSIZE:
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_ALL)) {
            FM_PRINT("%s %d %s ... Get Flash Capacity is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }
        FM_PRINT("Flash size is 0x%lx.\n", return_size);
        break;

    case FLASH_IMAGE_RAW:
        if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
            return_size = FLASH_RAW_IMAGE_SIZE;
        } else {
            return_address = FLASH_RAW_IMAGE_ADDR;
        }
        break;

    default:
        FM_PRINT("%s %d %s ... bad type....\n", __FILE__, __LINE__, __FUNCTION__);
        break;
    }

    if (FLASH_IMAGE_RETURN_SIZE == component_return_type) {
        return return_size;
    }
    else {
        return return_address;
    }
}
#endif
MV_U32 return_image_place_on_flash(MV_U32 component_type, MV_U16 component_number, MV_U16 component_return_type)
{
    MV_U8 ptr_buffer[FLASH_LAYOUT_BUFF_STRING_SIZE];
    MV_U32 ret;
    MV_PU8 source_str = ptr_buffer;
    MV_U8 str_title[24], str_buf[36];
    MV_U16 i = 0, count = 0, str_field = 0, str_buf_length = 0, str_title_length = 0;
    MV_U16 start_getting = 0x00, start_getting_title = 0x00;
    MV_U32 component_num = 0;
    MV_U32 return_address = 0xffffffffL;
    MV_U32 return_size = 0xffffffffL;
#ifdef _OS_UKRN
    ret = return_image_place_on_flash_for_frey(component_type, component_number, component_return_type);
    return ret;
#endif

#if !defined(SUPPORT_SEARCH_ON_FILE)
    if (flash_layout_head_ptr == NULL) {
        initial_flash_layout();
    }
    if ((MV_U32)flash_layout_head_ptr == 0xFFFFFFFFUL) {
        FM_PRINT("%s %d %s ... no valid Flash layout....\n", __FILE__, __LINE__, __FUNCTION__);
        return 0xffffffffL;
    }		
    if(!Core_NVMRd( NULL, MFR_FLASH_DEV(0), ((MV_U32)(MV_PU8)flash_layout_head_ptr), FLASH_LAYOUT_BUFF_STRING_SIZE, (MV_PU8)source_str, 0)) {
        FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        MV_ASSERT(0);
        return 0xffffffffL;
    }
#else
    fseek(input_file, 0, SEEK_SET);
    while (i < FLASH_LAYOUT_BUFF_STRING_SIZE) {
        int c = fgetc(input_file);
        ptr_buffer[i] = (MV_U8)c;
        if (c == EOF) {
            ptr_buffer[i] = (MV_U8)0x00;
            break;
        }
//        printf("%c", c);
        i ++;
    }
    i = 0;
#endif

#if !defined(SUPPORT_SEARCH_ON_FILE)
    if ((FLASH_IMAGE_SINGLEBLOCK == component_type)
        && (component_return_type == FLASH_IMAGE_RETURN_SIZE)) {
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_A_BLOCK)) {
            FM_PRINT("%s %d %s ... Get Flash Capacity is fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return 0xffffffffL;
        }
      
        FM_PRINT("One block size is 0x%lx in Flash.\n", return_size);
        return return_size;
    } else if ((FLASH_IMAGE_FLASHSIZE == component_type)
        && (component_return_type == FLASH_IMAGE_RETURN_SIZE)) {
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_size), (MV_PU8)&return_size, MVFR_FLAG_GETSIZE_ALL)) {
            FM_PRINT("%s %d %s ... Get Flash Capacity is fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return 0xffffffffL;
        }
      
        FM_PRINT("Flash size is 0x%lx.\n", return_size);
        return return_size;
    } else if ((FLASH_IMAGE_FMLOADER == component_type)
        && (component_return_type == FLASH_IMAGE_RETURN_ADDRESS)) {
#ifndef _OS_UKRN
        if (!Core_NVMInfo(NULL, MFR_FLASH_DEV(0), NULL, sizeof(return_address), (MV_PU8)&return_address, MVFR_FLAG_GETSIZE_ALL)) {
            FM_PRINT("%s %d %s ... Get Flash Capacity is fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return 0xffffffffL;
        }
        return_size = return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0,FLASH_IMAGE_RETURN_SIZE);
      
        FM_PRINT("FWLOAD start address is (0x%lx-0x%lx=)0x%lx.\n", return_address, return_size, (return_address - return_size));
        return (return_address - return_size);
#else
        return 0;
#endif
    }
#endif

    while ((source_str[i] != '\0') && (component_type < FLASH_IMAGE_MAX) && (count < MAX_LAYOUT_DESCRIPTION_STRING_SIZE)) {

	   if (source_str[i] == '{') {
              start_getting = 0x01;
              str_buf_length = 0;
              str_field = 0;
	   }
	   else if (source_str[i] == '}') {
           start_getting = 0x00;
  		   if ((!MV_CompareMemory(flash_image_component_name[component_type], str_title , MV_StrLen(flash_image_component_name[component_type]))) && (component_num == component_number)) {
                     if (component_return_type == FLASH_IMAGE_RETURN_ADDRESS) {
      	                  MV_DPRINT((">(address)>>>>>%s %ld 0x%lx %ld\n", str_title, component_num, return_address, return_size));
                         return return_address;
                     } else if (component_return_type == FLASH_IMAGE_RETURN_SIZE) {
                         MV_DPRINT((">(size)>>>>>%s %ld 0x%lx %ld\n", str_title, component_num, return_address, return_size));
                         return return_size;
                     } else {
      	                  MV_DPRINT((">(error)>>>>>%s %ld 0x%lx %ld\n", str_title, component_num, return_address, return_size));
                         return 0xffffffffL;
                     }
		   }
	   }
	   else {
		   if (start_getting) {
				switch (source_str[i]) {
					case '\"':
						if (start_getting_title) {
							start_getting_title = 0x00;
							str_title[str_title_length] = '\0';
							str_title_length = 0x00;
						} else
							start_getting_title = 0x01;
						break;
					case ',':
						str_buf[str_buf_length] = '\0';
						if (str_field == 1) {
							STR_TO_DEC_U32((char *)str_buf, &component_num);
						} else if (str_field == 2){
							STR_TO_HEX_U32((char *)str_buf, &return_address);
						} else if (str_field == 3) {
							MV_U32 unit = 1;
//							printf(">>%s\n", str_buf);
							switch(str_buf[str_buf_length - 1]) {
								case 'k':
								case 'K':
									unit = 1024;
									str_buf[str_buf_length - 1] = '\0';
									break;
								case 'm':
								case 'M':
									unit = 1024 * 1024;
									str_buf[str_buf_length - 1] = '\0';
									break;
								case 'g':
								case 'G':
									unit = 1024 * 1024 * 1024;
									str_buf[str_buf_length - 1] = '\0';
									break;
								default:
									unit = 1;
									break;
							}
							if ((str_buf[0] == '0') && ((str_buf[1] == 'x') || (str_buf[1] == 'X'))) {
								STR_TO_HEX_U32((char *)str_buf, &return_size);
							} else {
								STR_TO_DEC_U32((char *)str_buf, &return_size);
							}
							return_size *= unit;
//							printf ("return_size>>%ld, 0x%lx", return_size, return_size);
						}
						str_field ++;
						str_buf_length = 0;
						break;
					default:
						if ((str_field == 0) && (start_getting_title) && (source_str[i] != ' ')) {
							str_title[str_title_length] = source_str[i];
							str_title_length ++;
						}
						else if (((str_field == 1) || (str_field == 3) || ((str_field == 2) && (source_str[i] != 'x') && (source_str[i] != 'X'))) && (source_str[i] != ' ') && (source_str[i] != '[')&& (source_str[i] != ']')) {
							str_buf[str_buf_length] = source_str[i];
							str_buf_length ++;
						}
						break;
				}
		   }
	   }
	   i ++;
	   count ++;

	   if (i >= FLASH_LAYOUT_BUFF_STRING_SIZE) {
#if !defined(SUPPORT_SEARCH_ON_FILE)
              if(!Core_NVMRd( NULL, MFR_FLASH_DEV(0), ((MV_U32)((MV_PU8)flash_layout_head_ptr + count)), FLASH_LAYOUT_BUFF_STRING_SIZE, (MV_PU8)source_str, 0)) {
                  FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                  MV_ASSERT(0);
                  return 0xffffffffL;
              }
#else
              i = 0;
              while (i < FLASH_LAYOUT_BUFF_STRING_SIZE) {
                 int c = fgetc(input_file);
                 ptr_buffer[i] = (MV_U8)c;
                  if (c == EOF) {
                     ptr_buffer[i] = (MV_U8)0x00;
                     break;
                  }
//                 printf("%c", c);
                  i ++;
              }
#endif
              i = 0;
	   }
   }

   FM_PRINT("This return value<0x%lx> is fail.\n", return_address);
   return 0xffffffffL;
}
#endif

#if defined(SAVE_EVENTS_TO_FLASH)
MV_U32 event_log_header_offset;

struct event_log_parameters
{

	MV_U16	max_entries;
	MV_U16	didnot_sign_fromslot;
	MV_U16	valid_slot_start;
	MV_U16	valid_slot_end;
//	MV_U32	flags;
	MV_U32	timeStamp;
	MV_U8	reserved1[4];
};
struct event_log_parameters global_events_para[1]; 

#if defined(SUPPORT_NVRAM)
struct nvram_event_log *global_nvram_event_log; 

MV_PVOID finding_hander_from_nvram(MV_PU8 signature)
{
    struct nvram_normal_head *ptr_normal_hd = (struct nvram_normal_head *)(MV_REG_READ_DWORD(ROC_INTER_REGS_BASE, AHB_TO_MBUS_WIN_BASE_REG(5)) + NVRAM_START_OFFSET);;
    MV_U16 size = (MV_U16)sizeof(struct nvram_normal_head);
    MV_U32 crc = 0, crc_temp = 0;
    do {
        crc = ptr_normal_hd->crc;
        ptr_normal_hd->crc = MV_MAX_U32;
        crc_temp = MV_CRC((MV_PU8)ptr_normal_hd, size);
        ptr_normal_hd->crc = crc;
        if((!MV_CompareMemory(ptr_normal_hd->signature, signature, 4)) && (crc == crc_temp)) {
//           EVENT_PRINT(("\nptr_normal_hd->signature[0]=0x%x",ptr_normal_hd->signature[0]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[1]=0x%x",ptr_normal_hd->signature[1]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[2]=0x%x",ptr_normal_hd->signature[2]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[3]=0x%x",ptr_normal_hd->signature[3]));
//           EVENT_PRINT(("\nCRC=0x%x, CalculationCRC=0x%x",crc,crc_temp));
//           EVENT_PRINT(("\nIs this a header in NVRAM? Yes"));
           return (MV_PVOID)ptr_normal_hd;
        }
        ptr_normal_hd = (struct nvram_normal_head *)((MV_PU8)ptr_normal_hd + ptr_normal_hd->next_tbl_off);
    } while (crc == crc_temp);
    return NULL;
}

MV_PVOID gotting_free_space_from_nvram(void)
{
    struct nvram_normal_head *ptr_normal_hd = (struct nvram_normal_head *)(MV_REG_READ_DWORD(ROC_INTER_REGS_BASE, AHB_TO_MBUS_WIN_BASE_REG(5)) + NVRAM_START_OFFSET);;
    MV_U16 size = (MV_U16)sizeof(struct nvram_normal_head);
    MV_U32 crc = 0, crc_temp = 0;
    do {
        crc = ptr_normal_hd->crc;
        ptr_normal_hd->crc = MV_MAX_U32;
        crc_temp = MV_CRC((MV_PU8)ptr_normal_hd, size);
        ptr_normal_hd->crc = crc;
        if (crc != crc_temp)
            break;
        ptr_normal_hd = (struct nvram_normal_head *)((MV_PU8)ptr_normal_hd + ptr_normal_hd->next_tbl_off);
    } while (1);

//    EVENT_PRINT(("\nThe Space is 0x%x in NVRAM.", (MV_U32)ptr_normal_hd));
    if (((MV_U32)ptr_normal_hd & 0xfffff) > (128 * 1024)) /*NVRAM max size*/
        return NULL;
    else
        return (MV_PVOID)ptr_normal_hd;
}

MV_BOOLEAN wipe_out_header_in_nvram(MV_PU8 signature)
{
    struct nvram_normal_head *ptr_normal_hd = (struct nvram_normal_head *)(MV_REG_READ_DWORD(ROC_INTER_REGS_BASE, AHB_TO_MBUS_WIN_BASE_REG(5)) + NVRAM_START_OFFSET);;
    MV_U16 size = (MV_U16)sizeof(struct nvram_normal_head);
    MV_U32 crc = 0, crc_temp = 0;
    do {
        crc = ptr_normal_hd->crc;
        ptr_normal_hd->crc = MV_MAX_U32;
        crc_temp = MV_CRC((MV_PU8)ptr_normal_hd, size);
        ptr_normal_hd->crc = crc;
        if((!MV_CompareMemory(ptr_normal_hd->signature, signature, 4)) && (crc == crc_temp)) {
//           EVENT_PRINT(("\nptr_normal_hd->signature[0]=0x%x",ptr_normal_hd->signature[0]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[1]=0x%x",ptr_normal_hd->signature[1]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[2]=0x%x",ptr_normal_hd->signature[2]));
//           EVENT_PRINT(("\nptr_normal_hd->signature[3]=0x%x",ptr_normal_hd->signature[3]));
//           EVENT_PRINT(("\nCRC=0x%x, CalculationCRC=0x%x",crc,crc_temp));
//           EVENT_PRINT(("\nthis header will be wipe out."));
           MV_ZeroMemory((MV_PU8)ptr_normal_hd, ptr_normal_hd->next_tbl_off);
           return MV_TRUE;
        }
        ptr_normal_hd = (struct nvram_normal_head *)((MV_PU8)ptr_normal_hd + ptr_normal_hd->next_tbl_off);
    } while (crc == crc_temp);
    return MV_FALSE;
}

MV_BOOLEAN check_nvram_eventlog_head(MV_PVOID event_log_head_ptr)
{
    struct nvram_event_log *ptr_evt_log = (struct nvram_event_log *)event_log_head_ptr;
    struct nvram_event_log_head *ptr_evt_hd = &ptr_evt_log->head;
    MV_U16 size = (MV_U16)sizeof(struct nvram_event_log_head);
    MV_U32 crc, temp_crc;
    crc = ptr_evt_hd->crc;
    ptr_evt_hd->crc = MV_MAX_U32;
    temp_crc = MV_CRC((MV_PU8)ptr_evt_hd, size);
    ptr_evt_hd->crc = crc;
    EVENT_PRINT(("\nnvRAM ptr_evt_hd->signature[0]=0x%x",ptr_evt_hd->signature[0]));
    EVENT_PRINT(("\nnvRAM ptr_evt_hd->signature[1]=0x%x",ptr_evt_hd->signature[1]));
    EVENT_PRINT(("\nnvRAM ptr_evt_hd->signature[2]=0x%x",ptr_evt_hd->signature[2]));
    EVENT_PRINT(("\nnvRAM ptr_evt_hd->signature[3]=0x%x",ptr_evt_hd->signature[3]));
    EVENT_PRINT(("\nnvRAM CRC=0x%x, CalculationCRC=0x%x",crc,temp_crc));
    EVENT_PRINT(("\nnvRAM Is this EVENTL0G_HEAD_SIGNATURE? "));

    if((!MV_CompareMemory(ptr_evt_hd->signature, EVENTL0G_HEAD_SIGNATURE, 4)) && (crc == temp_crc)) {
        crc = ptr_evt_log->log_crc;
        ptr_evt_log->log_crc = MV_MAX_U32;
        temp_crc = MV_CRC((MV_PU8)ptr_evt_log, (MV_U16)sizeof(struct nvram_event_log));
        ptr_evt_log->log_crc = crc;
        if (crc == temp_crc) {
            EVENT_PRINT(("Yes"));
            return MV_TRUE;
        }
    }
    EVENT_PRINT(("No"));
    return MV_FALSE;
}

MV_VOID initialize_nvram_eventlog_head(MV_PVOID event_log_head_ptr)
{
    struct nvram_event_log *ptr_evt_log = (struct nvram_event_log *)event_log_head_ptr;
    struct nvram_event_log_head *ptr_evt_hd = &ptr_evt_log->head;

    /* TBD */
    MV_CopyMemory((MV_PU8)ptr_evt_hd->signature, EVENTL0G_HEAD_SIGNATURE, 4);
    ptr_evt_hd->next_tbl_off = (MV_U32)sizeof(struct nvram_event_log);
    ptr_evt_hd->version = 0x0100;
    ptr_evt_hd->nr_max_entries = MAX_FLASH_EVENT_LOG_ENTRY;
    MV_FillMemory(ptr_evt_hd->reserved, EVENTLOG_HEAD_RESERVED_BYTES, 0xFF);
    ptr_evt_hd->crc = MV_MAX_U32;
    ptr_evt_hd->crc = MV_CRC((MV_PU8)ptr_evt_hd, (MV_U16)sizeof(struct nvram_event_log_head));

    ptr_evt_log->didnot_sign_fromslot = 0;
    ptr_evt_log->valid_slot_start = 0;
    ptr_evt_log->valid_slot_end = 0;
    ptr_evt_log->sequence_no = 0;
    ptr_evt_log->reserved = 0xffffffffL;

    ptr_evt_log->log_crc = MV_MAX_U32;
    ptr_evt_log->log_crc = MV_CRC((MV_PU8)ptr_evt_log, (MV_U16)sizeof(struct nvram_event_log));
}
#endif /*SUPPORT_NVRAM*/

MV_BOOLEAN check_flash_eventlog_head(MV_U16 entry_id)
{
    struct flash_event_log_head ptr_evt_hd[1];
    MV_U16 size = (MV_U16)sizeof(struct flash_event_log_head);
    MV_U32 crc, temp_crc;

    if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), (event_log_header_offset + sizeof(struct flash_event_log_entry) * entry_id), size, (MV_PU8)ptr_evt_hd, 0)) {
        EVENT_PRINT(("%s %d %s ... read Flash(entry %d) is fail....\n", __FILE__, __LINE__, __FUNCTION__, entry_id));
        return MV_FALSE;
    }

    crc = ptr_evt_hd->crc;
    ptr_evt_hd->crc = MV_MAX_U32;
    temp_crc = MV_CRC((MV_PU8)ptr_evt_hd, size);
    ptr_evt_hd->crc = crc;
    EVENT_PRINT(("\nflash entry%d event log head size=0x%x, flash event log entry size=0x%x", entry_id, sizeof(struct flash_event_log_head), sizeof(struct flash_event_log_entry)));
    EVENT_PRINT(("\nflash entry%d ptr_evt_hd->signature[0]=0x%x", entry_id, ptr_evt_hd->signature[0]));
    EVENT_PRINT(("\nflash entry%d ptr_evt_hd->signature[1]=0x%x", entry_id, ptr_evt_hd->signature[1]));
    EVENT_PRINT(("\nflash entry%d ptr_evt_hd->signature[2]=0x%x", entry_id, ptr_evt_hd->signature[2]));
    EVENT_PRINT(("\nflash entry%d ptr_evt_hd->signature[3]=0x%x", entry_id, ptr_evt_hd->signature[3]));
    EVENT_PRINT(("\nflash entry%d ptr_evt_hd->version=0x%x", entry_id, ptr_evt_hd->version));
    EVENT_PRINT(("\nflash entry%d CRC=0x%x, CalculationCRC=0x%x", entry_id, crc,temp_crc));
    EVENT_PRINT(("\nflash entry%d, Is this EVENTL0G_HEAD_SIGNATURE? ", entry_id));

    if((!MV_CompareMemory(ptr_evt_hd->signature, EVENTL0G_HEAD_SIGNATURE, 4)) && (ptr_evt_hd->version == EVENT_LOG_VERSION) && (crc == temp_crc)) {
        //ptr_evt_para->max_entries = ptr_evt_hd->nr_max_entries;
        EVENT_PRINT(("Yes\n"));
        return MV_TRUE;
    }
    EVENT_PRINT(("No\n"));
    return MV_FALSE;
}

MV_VOID initialize_flash_eventlog_head(MV_U16 entry_id)
{
    struct flash_event_log_head ptr_evt_hd[1];

    /* TBD */
    MV_CopyMemory((MV_PU8)ptr_evt_hd->signature, EVENTL0G_HEAD_SIGNATURE, 4);
    ptr_evt_hd->next_tbl_off = 0x00000000;
    ptr_evt_hd->version = EVENT_LOG_VERSION;
    ptr_evt_hd->nr_max_entries = MAX_FLASH_EVENT_LOG_ENTRY;
    MV_FillMemory(ptr_evt_hd->reserved, EVENTLOG_HEAD_RESERVED_BYTES, 0xFF);
    ptr_evt_hd->crc = MV_MAX_U32;
    ptr_evt_hd->crc = MV_CRC((MV_PU8)ptr_evt_hd, (MV_U16)sizeof(struct flash_event_log_head));

    EVENT_PRINT(("\nInitialize event hander at entry%d", entry_id));
    if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), (event_log_header_offset + sizeof(struct flash_event_log_entry) * entry_id), sizeof(struct flash_event_log_head), (MV_PU8)ptr_evt_hd, 0))) {
        EVENT_PRINT(("%s %d %s ... write Flash(entry %d) is fail....\n", __FILE__, __LINE__, __FUNCTION__,entry_id));
        return;
    }
}

void clean_flash_block_for_eventlog(MV_U16 num)
{
   /*This function will erase this block.*/
   // MV_U8 buf[128 * 1024];
  //  MV_FillMemory(buf, (128 * 1024), 0xFF);
    if((have_flash_layout)&&(!Core_NVMEs(NULL, MFR_FLASH_DEV(0), (event_log_header_offset + (EVENT_FLASH_SECTOR_SIZE * num)), EVENT_FLASH_SECTOR_SIZE, NULL, 0))) {
        EVENT_PRINT(("%s %d %s ... Erase Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__));
        return;
    }
}

#define STORED_EVENT_COUNT(ptr_evt_para) (ptr_evt_para->valid_slot_start > ptr_evt_para->valid_slot_end) ? \
                    (MAX_FLASH_EVENT_LOG_ENTRY - ptr_evt_para->valid_slot_start + ptr_evt_para->valid_slot_end) : \
                    (ptr_evt_para->valid_slot_end - ptr_evt_para->valid_slot_start)



MV_U16 check_and_mark_valid_events(MV_PVOID This)
{
    PHBA_Extension ptr_hba = (PHBA_Extension)This;
    struct event_log_parameters *ptr_evt_para = global_events_para;
    struct flash_event_log_entry ptr_event[1];
    MV_U16 stored_events_count = ptr_hba->Num_Stored_Events;
#ifdef EVENT_SIMPLE_POLICY
    MV_U16 valid_reserved0;
#endif
    /*Adding a sign that the next time will not be read.*/
    if (ptr_evt_para->didnot_sign_fromslot != ptr_evt_para->valid_slot_start) {
        MV_U32 count = (ptr_evt_para->didnot_sign_fromslot > ptr_evt_para->valid_slot_start) ? 
                    (MAX_FLASH_EVENT_LOG_ENTRY - ptr_evt_para->didnot_sign_fromslot + ptr_evt_para->valid_slot_start) :
                    (ptr_evt_para->valid_slot_start - ptr_evt_para->didnot_sign_fromslot);

        EVENT_PRINT(("\nHaving %d events didn't sign in flash from slot=%d to slot=%d.", count, ptr_evt_para->didnot_sign_fromslot, ptr_evt_para->valid_slot_start));

        while ((count > 0) && (ptr_evt_para->didnot_sign_fromslot != ptr_evt_para->valid_slot_start)) {
             if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->didnot_sign_fromslot), sizeof(struct flash_event_log_entry), (MV_PU8)ptr_event, 0)) {
                 EVENT_PRINT(("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__));
                 return 0;
             }
             if (ptr_event->valid == MV_TRUE) {
                 ptr_event->valid = 0x00; /*This event make a sign that next time it will not be read.*/
#ifndef EVENT_SIMPLE_POLICY
                 if((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->didnot_sign_fromslot), sizeof(struct flash_event_log_entry), (MV_PU8)ptr_event, 0))) {
#else
                 valid_reserved0 = 0xFF00;
                 if((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->didnot_sign_fromslot) + OFFSET_OF(struct flash_event_log_entry, valid), 2, &valid_reserved0, MVFR_FLAG_AUTO))) {

#endif
//                 if (!EVT_UPDATE(ptr_core_ext, ptr_evt_para->didnot_sign_fromslot, ptr_event, valid)) {
                     EVENT_PRINT(("%s %d %s ... write Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__));
                 	return 0;
                 }
             }
             ptr_evt_para->didnot_sign_fromslot ++;
             if (ptr_evt_para->didnot_sign_fromslot >= ptr_evt_para->max_entries)
                 ptr_evt_para->didnot_sign_fromslot = 0;
             count --;
        }
    }

    if (ptr_evt_para->valid_slot_start == ptr_evt_para->valid_slot_end){
        EVENT_PRINT(("\nNo Event on Flash.(%d)", ptr_hba->Num_Stored_Events));
        return 0;
    }

    stored_events_count = STORED_EVENT_COUNT(ptr_evt_para);

    EVENT_PRINT(("\nRemaining %d events in flash.\n", stored_events_count));
    return stored_events_count;
}

MV_U16 check_remaining_events_in_flash(MV_PVOID This)
{
    struct event_log_parameters *ptr_evt_para = global_events_para;
 
    if (ptr_evt_para->valid_slot_start == ptr_evt_para->valid_slot_end) {
        check_and_mark_valid_events(This);
    }
    return STORED_EVENT_COUNT(ptr_evt_para);
}

void get_much_info_from_flasheventlog(MV_PVOID This)
{
    PHBA_Extension ptr_hba = (PHBA_Extension)This;
    struct event_log_parameters *ptr_evt_para = global_events_para;
    struct flash_event_log_entry ptr_event[1];
    MV_U32 temp_crc, crc;
    MV_U32 count = 0;
    MV_U16 last_slot = 0, num_valid_events = 0, num_stored_events = 0;
#if defined(SYNC_BY_EVENT_TIMESTAMP)
    MV_U32 last_time_stamp = 0;
    MV_U32 tmp_SequenceNumber = 0;
#endif

    ptr_hba->SequenceNumber = 0;
    ptr_hba->Num_Stored_Events = 0;
    ptr_evt_para->didnot_sign_fromslot = 0;
    ptr_evt_para->valid_slot_start = 0;
    ptr_evt_para->valid_slot_end = 0;

    /*Search maximum Sequence Number*/
    while (count < MAX_FLASH_EVENT_LOG_ENTRY) {
        if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(count), sizeof(struct flash_event_log_entry), (MV_PU8)ptr_event, 0)) {
            FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return;
        }

#if defined(ROC_V2_EVENT_SUPPORT)
        if (ptr_event->event.Event.TimeStamp != 0xFFFFFFFF) 
#else
        if (ptr_event->event.TimeStamp != 0xFFFFFFFF) 
#endif
        { /*Speed uo to check*/
            crc = ptr_event->crc;
    //        ptr_event->crc = MV_MAX_U32;
            temp_crc = MV_CRC((MV_PU8)ptr_event, (MV_U16)&(((struct flash_event_log_entry *)0)->crc));
    
            if (crc == temp_crc) {
#if defined(ROC_V2_EVENT_SUPPORT)
                if (ptr_hba->SequenceNumber < ptr_event->event.Event.SequenceNo) {
                    ptr_hba->SequenceNumber = ptr_event->event.Event.SequenceNo;
                    last_slot = (MV_U16)count;
                }
#else
                if(ptr_hba->SequenceNumber < ptr_event->event.SequenceNo) {
                    ptr_hba->SequenceNumber = ptr_event->event.SequenceNo;
                    last_slot = (MV_U16)count;
                }
#endif

#if defined(SYNC_BY_EVENT_TIMESTAMP)
                if (ptr_event->event.Event.TimeStamp > (30 * 24 * 60 * 60)) {   //we'd like to get the latest valid timestame as current time
#if defined(ROC_V2_EVENT_SUPPORT)
                    if (tmp_SequenceNumber <= ptr_event->event.Event.SequenceNo) {
                        tmp_SequenceNumber = ptr_event->event.Event.SequenceNo;
                        last_time_stamp = ptr_event->event.Event.TimeStamp;
                    }
#else
                    if (tmp_SequenceNumber <= ptr_event->event.SequenceNo) {
                        tmp_SequenceNumber = ptr_event->event.SequenceNo;
                        last_time_stamp = ptr_event->event.TimeStamp;
                    }
#endif
                }
#endif
                if (ptr_event->valid == MV_TRUE) {
                    num_valid_events ++;
                }
                num_stored_events ++;
            }
    	 }
        count ++;
    }

#if defined(SYNC_BY_EVENT_TIMESTAMP)
    if ((HBA_GetTimeInSecond() < (30 * 24 * 60 * 60)) && (last_time_stamp > (30 * 24 * 60 * 60))) /*over than 30 days since 1970/01/01*/
    { /*Set RTC to latest time_stamp from events recorded.*/ 
        HBA_SetTimeInSecond(0, last_time_stamp);
        MV_PRINT("\Sync Software RTC from latest time_stamp=0x%X by events recorded.\n", last_time_stamp);
    }
#endif

    /*No kept event in flash*/
    if (num_stored_events == 0) {
        ptr_evt_para->didnot_sign_fromslot = last_slot + 1;
        ptr_evt_para->valid_slot_start = ptr_evt_para->didnot_sign_fromslot;
        ptr_evt_para->valid_slot_end = ptr_evt_para->valid_slot_start;
        EVENT_PRINT(("\nSequenceNumber=0x%x, start=%d, end=%d, mask=%d\n", ptr_hba->SequenceNumber, ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end, ptr_evt_para->didnot_sign_fromslot));
        return;
    }

    ptr_hba->SequenceNumber += 1;
    ptr_evt_para->valid_slot_end = last_slot + 1;
    if (ptr_evt_para->valid_slot_end >= ptr_evt_para->max_entries)
         ptr_evt_para->valid_slot_end = 0;

  {
    /*note: this formula come from svn://10.68.79.211/root/win/trunk/lib/common/com_nvram.c, Revision: 9404
    At booting, it will report 50 events to HOST but they had been read before. 
    The rule will fix lost events issue at booting time. */
    MV_U16 temp = ((num_valid_events + 50) > num_stored_events) ?
                      num_stored_events : (num_valid_events + 50);

    ptr_evt_para->valid_slot_start = (last_slot < temp) ? 
                    (((temp - last_slot - 1) == 0) ? 0 : (MAX_FLASH_EVENT_LOG_ENTRY - temp + last_slot)) :
                    (last_slot - temp);
    ptr_evt_para->didnot_sign_fromslot = ptr_evt_para->valid_slot_start;
    ptr_hba->Num_Stored_Events = temp;
   }
   EVENT_PRINT(("\nThere are %d events in Flash. SequenceNumber=0x%x, start=%d, end=%d, mask=%d\n", ptr_hba->Num_Stored_Events, ptr_hba->SequenceNumber, ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end, ptr_evt_para->didnot_sign_fromslot));
}

MV_BOOLEAN add_event_to_flash( 
	MV_PVOID This,
#if defined(ROC_V2_EVENT_SUPPORT)
	PDriverEvent_V2 ptr_driver_event
#else
	PDriverEvent ptr_driver_event
#endif
    )
{
    struct event_log_parameters *ptr_evt_para = global_events_para;
    struct flash_event_log_entry ptr_event[1];
    MV_U32 stored_events_count;
    MV_U32 error_flag = 0, retry_count = MAX_FLASH_EVENT_PER_BLOCK;

add_event_retry_loop:

    if (ptr_evt_para->valid_slot_end >= ptr_evt_para->max_entries) {
       EVENT_PRINT(("\nThe Event Entry will be restart from 0."));
	ptr_evt_para->valid_slot_end = 0;
    }

    stored_events_count = STORED_EVENT_COUNT(ptr_evt_para);

    EVENT_PRINT(("\nAdding start=0x%x,end=0x%x", ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end));

//    ptr_event->flags = 0xFFFFFFFF;
//    ptr_event->flags&=~EVENTLOG_ENTRY_INVALID;
    MV_FillMemory((MV_PU8)ptr_event->reserved0, 7, 0xFF);
#if defined(ROC_V2_EVENT_SUPPORT)
    MV_FillMemory((MV_PU8)ptr_event->reserved1, 52, 0xFF);
#else
    MV_FillMemory((MV_PU8)ptr_event->reserved1, 84, 0xFF);
#endif

    MV_CopyMemory(
        (MV_PU8)&ptr_event->event,
        (MV_PU8)ptr_driver_event,
#if defined(ROC_V2_EVENT_SUPPORT)
       sizeof(DriverEvent_V2)
#else
        sizeof(DriverEvent)
#endif
    );
//    ptr_event->crc = MV_MAX_U32;
    ptr_event->crc = MV_CRC((MV_PU8)ptr_event, (MV_U16)&(((struct flash_event_log_entry *)0)->crc));
    ptr_event->valid = MV_TRUE;
#if defined(ROC_V2_EVENT_SUPPORT)
    EVENT_PRINT((", SQNO=%d, valid=%d", ptr_driver_event->Event.SequenceNo, ptr_event->valid));
#else
    EVENT_PRINT((", SQNO=%d, valid=%d", ptr_driver_event->SequenceNo, ptr_event->valid));
#endif

    EVENT_PRINT(("\nWrite a event slot%d address 0x%x\n", ptr_evt_para->valid_slot_end, EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->valid_slot_end)));

    if (ptr_evt_para->valid_slot_end == (MAX_FLASH_EVENT_PER_BLOCK - 2)) { /*switch to block 1*/
        EVENT_PRINT(("\nreinitialize Block 1 for Event Log"));
        MV_DASSERT(stored_events_count > (MAX_FLASH_EVENT_PER_BLOCK - 2));
        if (stored_events_count > (MAX_FLASH_EVENT_PER_BLOCK - 2)) {
            /*In this case, it will lose 1023 events. we only guarantee 1023 events.*/
            ptr_evt_para->valid_slot_start = 0;
            ptr_evt_para->didnot_sign_fromslot = 0;
            FM_PRINT("Event log memory overflow, erase memory block 1, the process will cause %d event logs lost.\n", stored_events_count - 1023);
        }
        clean_flash_block_for_eventlog(1);//block 1
        initialize_flash_eventlog_head((MAX_FLASH_EVENT_PER_BLOCK * 2) - 1); /*last entry*/
    }
    else if (ptr_evt_para->valid_slot_end == 0) { /*switch to block 0*/
        EVENT_PRINT(("\nreinitialize Block 0 for Event Log"));
        MV_DASSERT(stored_events_count > (MAX_FLASH_EVENT_PER_BLOCK - 2));
	 if (stored_events_count > (MAX_FLASH_EVENT_PER_BLOCK - 2)) {
            /*In this case, it will lose 1023 events. we only guarantee 1023 events.*/
            ptr_evt_para->valid_slot_start = (MAX_FLASH_EVENT_PER_BLOCK - 2);
            ptr_evt_para->didnot_sign_fromslot = (MAX_FLASH_EVENT_PER_BLOCK - 2);
            FM_PRINT("Event log memory overflow, erase memory block 0, the process will cause %d event logs lost.\n", stored_events_count - 1023);
        }
        clean_flash_block_for_eventlog(0);//block 0
        initialize_flash_eventlog_head(0); /*first entry*/
    }
#ifndef EVENT_SIMPLE_POLICY
    if((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->valid_slot_end), sizeof(struct flash_event_log_entry), (MV_PU8)ptr_event, 0))) { //Write //MVFR_FLAG_RMW_BUF
#else
    if((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->valid_slot_end), sizeof(struct flash_event_log_entry), (MV_PU16)ptr_event, MVFR_FLAG_AUTO))) { //Write //MVFR_FLAG_RMW_BUF
#endif
        FM_PRINT("%s %d %s ... write Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
        //return MV_FALSE;
        error_flag = 1;
    } else {
        error_flag = 0;
    }

    ptr_evt_para->valid_slot_end ++;
    if (ptr_evt_para->valid_slot_end >= ptr_evt_para->max_entries)
         ptr_evt_para->valid_slot_end = 0;

    if (error_flag == 1) {
        if (retry_count == 0)
            return MV_FALSE;
        else {
            retry_count --;
            goto add_event_retry_loop;
        }
    }

    return MV_TRUE;
}

/*Get events from flash*/
void mvGetEvent(MV_PVOID This, PMV_Request pReq)
{
    PHBA_Extension ptr_hba = (PHBA_Extension)This;
 #if defined(ROC_V2_EVENT_SUPPORT)
    PEventRequest_V2 pEventReq = (PEventRequest_V2) pReq->Data_Buffer;
#else
    PEventRequest pEventReq = (PEventRequest) pReq->Data_Buffer;
#endif
    struct event_log_parameters *ptr_evt_para = global_events_para;
    struct flash_event_log_entry ptr_event[1];
    MV_U32 temp_crc, crc;
    MV_U32 count, stored_events_count;

    pEventReq->Count = 0;
    pReq->Scsi_Status = REQ_STATUS_SUCCESS;

    stored_events_count = check_and_mark_valid_events(This);

    if (stored_events_count == 0){
        return;
    }

    if (stored_events_count > 0) {
        count = 0;
        while ((stored_events_count) && (count < MAX_EVENTS_RETURNED)) {
             EVENT_PRINT(("\nGetting=%d, slot_start=0x%x, slot_end=0x%x, remaining=%d",count, ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end, stored_events_count));

             if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(ptr_evt_para->valid_slot_start), sizeof(struct flash_event_log_entry), (MV_PU8)ptr_event, 0)) {
                 FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                 return;
             }

             crc = ptr_event->crc;
//             ptr_event->crc = MV_MAX_U32;
             temp_crc = MV_CRC((MV_PU8)ptr_event, (MV_U16)&(((struct flash_event_log_entry *)0)->crc));
#if defined(ROC_V2_EVENT_SUPPORT)
                 EVENT_PRINT((", SQNO=%d, valid=%d", ptr_event->event.Event.SequenceNo, ptr_event->valid));
#else
                 EVENT_PRINT((", SQNO=%d, valid=%d", ptr_event->event.SequenceNo, ptr_event->valid));
#endif
#if defined(ROC_V2_EVENT_SUPPORT)
             ptr_event->event.Event.AdapterID = ptr_event->valid;
#else
             ptr_event->event.AdapterID = ptr_event->valid;
#endif
             if (crc == temp_crc) {
#if defined(ROC_V2_EVENT_SUPPORT)
                 if (ptr_event->event.Event.TimeStamp < (30 * 24 * 60 * 60)) /*one month*/
#else
                 if (ptr_event->event.TimeStamp < (30 * 24 * 60 * 60))
#endif
                 { /*Some events be set 1970/01/01, because it was happened before handshake.*/ 
                      if (ptr_evt_para->timeStamp) {
                          /*Base on latest event which has correct timestamp, and count the next timestamp.*/
#if defined(ROC_V2_EVENT_SUPPORT)
                          ptr_event->event.Event.TimeStamp = ptr_evt_para->timeStamp + ptr_event->event.Event.TimeStamp;
#else
                          ptr_event->event.TimeStamp = ptr_evt_para->timeStamp + ptr_event->event.TimeStamp;
#endif
                      } else {
                          MV_U8  use_current_time = MV_TRUE;
                          MV_DPRINT(("\nThe event's TimerStamp(19700101) has modified to another one, it would refer to next event."));
#if 1
                          { /*Use last time stamp which is valid.*/
                              struct flash_event_log_entry last_event[1];
                              MV_U16 get_last_event = (ptr_evt_para->valid_slot_start == 0) ? 0 : ptr_evt_para->valid_slot_start - 1;
                              do {
                                  //FM_PRINT("valid_slot_start=%d, valid_slot_end=%d, get_next_event=%d\n", ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end, get_next_event);
                                  if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(get_last_event), sizeof(struct flash_event_log_entry), (MV_PU8)last_event, 0)) {
                                      FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                                      return;
                                  }
                                  if (last_event->crc == MV_CRC((MV_PU8)last_event, (MV_U16)&(((struct flash_event_log_entry *)0)->crc))) {
#if defined(ROC_V2_EVENT_SUPPORT)
                                      if (last_event->event.Event.TimeStamp > (30 * 24 * 60 * 60)) /*one month*/
#else
                                      if (last_event->event.TimeStamp > (30 * 24 * 60 * 60))
#endif
                                      {
#if defined(ROC_V2_EVENT_SUPPORT)
                                           ptr_evt_para->timeStamp = last_event->event.Event.TimeStamp;
                                           ptr_event->event.Event.TimeStamp = last_event->event.Event.TimeStamp + ptr_event->event.Event.TimeStamp;
#else
                                           ptr_evt_para->timeStamp = last_event->event.TimeStamp;
                                           ptr_event->event.TimeStamp = last_event->event.TimeStamp + ptr_event->event.TimeStamp;
#endif
                                           use_current_time = MV_FALSE;
                                           break;
                                      }
                                  }

                             } 
                             while (get_last_event-- > 0);
                         }

#else
                          { /*Use next time stamp is valid that is found in event slots.*/
                              struct flash_event_log_entry next_event[1];
                              MV_U16 get_next_event = (ptr_evt_para->valid_slot_start == 0) ? 0 : ptr_evt_para->valid_slot_start - 1;
                              while (get_next_event != ptr_evt_para->valid_slot_end) {
                                  //FM_PRINT("valid_slot_start=%d, valid_slot_end=%d, get_next_event=%d\n", ptr_evt_para->valid_slot_start, ptr_evt_para->valid_slot_end, get_next_event);
                                  if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), EVENT_LOG_ENTRY_OFFSET(get_next_event), sizeof(struct flash_event_log_entry), (MV_PU8)next_event, 0)) {
                                      FM_PRINT("%s %d %s ... read Flash is fail....\n", __FILE__, __LINE__, __FUNCTION__);
                                      return;
                                  }
                                  if (next_event->crc == MV_CRC((MV_PU8)next_event, (MV_U16)&(((struct flash_event_log_entry *)0)->crc))) {
#if defined(ROC_V2_EVENT_SUPPORT)
                                      if (next_event->event.Event.TimeStamp > (30 * 24 * 60 * 60)) /*one month*/
#else
                                      if (next_event->event.TimeStamp > (30 * 24 * 60 * 60))
#endif
                                      {
#if defined(ROC_V2_EVENT_SUPPORT)
                                           ptr_evt_para->timeStamp = next_event->event.Event.TimeStamp;
                                           ptr_event->event.Event.TimeStamp = next_event->event.Event.TimeStamp + ptr_event->event.Event.TimeStamp;
#else
                                           ptr_evt_para->timeStamp = next_event->event.TimeStamp;
                                           ptr_event->event.TimeStamp = next_event->event.TimeStamp + ptr_event->event.TimeStamp;
#endif
                                           use_current_time = MV_FALSE;
                                           break;
                                      }
                                  }

                                  get_next_event ++;
                                  if (get_next_event >= ptr_evt_para->max_entries)
                                      get_next_event = 0;
                             }
                         }
#endif
                         if (use_current_time) {
                             ptr_evt_para->timeStamp = HBA_GetTimeInSecond();
#if defined(ROC_V2_EVENT_SUPPORT)
                             ptr_event->event.Event.TimeStamp = HBA_GetTimeInSecond();
#else
                             ptr_event->event.TimeStamp = HBA_GetTimeInSecond();
#endif
                             EVENT_PRINT (("\nThe TimeStamp will use current time. Slot is %d", ptr_evt_para->valid_slot_start));
                         }
                     }
             	   }
             	   else
             	   {
                     ptr_evt_para->timeStamp = 0;
             	   }

                 MV_CopyMemory(
                    (MV_PU8)&pEventReq->Events[count],
                    (MV_PU8)&ptr_event->event,
#if defined(ROC_V2_EVENT_SUPPORT)
                    sizeof(DriverEvent_V2)
#else
                    sizeof(DriverEvent)
#endif
                  );
    
                 //Below code will avoid MRU's Sequence Number be Reset to Zero(it only reset once with MRU).
#if defined(ROC_V2_EVENT_SUPPORT)
                 if (!((ptr_event->event.Event.SequenceNo == 0x00000000) && (ptr_event->valid)))
#else
                 if (!((ptr_event->event.SequenceNo == 0x00000000) && (ptr_event->valid)))
#endif
				 count++;
            } else {
                EVENT_PRINT(("\nThis event(slot %d) is not valid.", ptr_evt_para->valid_slot_start));
            }

            ptr_evt_para->valid_slot_start ++;
            if (ptr_evt_para->valid_slot_start >= ptr_evt_para->max_entries)
                ptr_evt_para->valid_slot_start = 0;
            stored_events_count --;
            ptr_hba->Num_Stored_Events = (MV_U16)stored_events_count;
        }
        pEventReq->Count = count;
    }
}

#if defined(TEST_EVETLOG_FUNCTION) /*the test code is for event log*/
MV_U8 Timer_AddRequest(
    IN MV_PVOID extension,
    IN MV_U32 time_unit,
    IN MV_VOID (*routine) (MV_PVOID, MV_PVOID),
    IN MV_PVOID context1,
    IN MV_PVOID context2);
void HBA_ModuleNotification(
    MV_PVOID This,
    enum Module_Event event,
    struct mod_notif_param *param);

#define testevent_generate_event(ext, eid, did, slv, pc, ptr, tran)                \
   {                                                                    \
       struct mod_notif_param param = {ptr, 0, 0, eid, did, slv, pc,0,NULL,tran};   \
       HBA_ModuleNotification(ext, EVENT_LOG_GENERATED, &param);        \
   }

MV_BOOLEAN test_eventlog_function(
	IN OUT MV_PVOID This,
	IN MV_PVOID temp
	)
{
       EVENT_PRINT(("\nGenerate test 10 events. and add another one timer."));
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 246, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 247, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 248, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 249, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 250, SEVERITY_WARNING, 0, NULL,0);

	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 251, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 252, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 253, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 254, SEVERITY_WARNING, 0, NULL,0);
	testevent_generate_event(NULL, EVT_ID_HD_TIMEOUT, 255, SEVERITY_WARNING, 0, NULL,0);

       Timer_AddRequest(This, 3, test_eventlog_function, NULL, NULL);//500ms * 3
       return MV_TRUE;
}
#endif

void initial_flash_eventlog(MV_PVOID This)
{
#if defined(ADDING_FLASH_LAYOUT_DESC)
    if ((MV_U32)flash_layout_head_ptr == 0xFFFFFFFF) {
        FM_PRINT("No valid event layout.\n");
        return;
    }
#endif

    {   /*Initialize event parameter*/
         struct event_log_parameters *ptr_evt_para = global_events_para;
         ptr_evt_para->max_entries = MAX_FLASH_EVENT_LOG_ENTRY;
         ptr_evt_para->didnot_sign_fromslot = 0;
         ptr_evt_para->valid_slot_start = 0;
         ptr_evt_para->valid_slot_end = 0;
    }

    event_log_header_offset = EVENT_LOG_HEADER_OFFSET;
    /*The critical events will save in Flash. the event will report latest 50 events to host when OS is started.*/
    {
        MV_U8 temp = 0;
        if (!check_flash_eventlog_head(0)) { /*first entry*/
            clean_flash_block_for_eventlog(0);//block 0
            initialize_flash_eventlog_head(0);
            temp |= MV_BIT(0);
        }
        if (!check_flash_eventlog_head((MAX_FLASH_EVENT_PER_BLOCK * 2) - 1)) { /*last entry*/
            clean_flash_block_for_eventlog(1);//block 1
            initialize_flash_eventlog_head((MAX_FLASH_EVENT_PER_BLOCK * 2) - 1);
            temp |= MV_BIT(1);
        }
        if ((temp & 3) == 3)
            return;
    }
    get_much_info_from_flasheventlog(This);
#if 0  /*just for test flash layout*/
    MV_DPRINT(("Test Uboot Start Address = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_FMLOADER, 0, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to get a sector = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_SINGLEBLOCK, 0, FLASH_IMAGE_RETURN_SIZE)));
    MV_DPRINT(("Test to get Flash Size = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_FLASHSIZE, 0, FLASH_IMAGE_RETURN_SIZE)));
    MV_DPRINT(("Test to  param_flash_addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to secondly flash addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 1, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to pd page addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_PDINFOPAGE, 0, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to event log addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_EVENTLOG, 0, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to firmware 0 addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 0, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to firmware 1 addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_FIRMWARE, 1, FLASH_IMAGE_RETURN_ADDRESS)));
    MV_DPRINT(("Test to config data addr = 0x%lx\n", return_image_place_on_flash(FLASH_IMAGE_CONFIG, 0, FLASH_IMAGE_RETURN_ADDRESS)));
#endif
}
#endif /*SAVE_EVENTS_TO_FLASH*/
#endif
