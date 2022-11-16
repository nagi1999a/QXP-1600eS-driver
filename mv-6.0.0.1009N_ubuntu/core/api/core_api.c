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
//#include "core_header.h"
#include "com_flash.h"
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_extern.h"
#include "com_error.h"
#include "core_util.h"
#include "core_console.h"
#include "hba_inter.h"
#include "core_sas.h"
#include "core_expander.h"
#include "core_api.h"
#ifdef SUPPORT_SGPIO
#include "core_gpio.h"
#endif
#include "core_spi.h"
#include "core_pd_map.h"
#ifdef SUPPORT_BOARD_ALARM
#include "core_alarm.h"
#endif

#if defined(ENABLE_PHYTUNING)
MV_U8 get_info_page(/*PCore_Driver_Extension pCore*/AdapterInfo    *pAI,MV_U8* pbuffer,MV_U8 page_code){

    MV_U32 flash_offset = HBAINFO_OFFSET;
    page_header *ppage_header = (page_header*)pbuffer;

    if (!pbuffer){
        MV_DPRINT(("[ERROR](get_info_page) pbuffer = NULL ?!! \n"));
        MV_HALTKEY;
        return MV_FALSE;
    }

    while(flash_offset != (NULL_PAGE_NUMBER * DEFAULT_PAGE_SIZE) && flash_offset > 0 ){
        /* step 1 read param from flash offset */
        MagniSPI_ReadBuf( pAI, flash_offset, (MV_PU8)ppage_header, DEFAULT_PAGE_SIZE);

        if(ppage_header->signature[0] == 'M'&& \
            ppage_header->signature[1] == 'R'&& \
            ppage_header->signature[2] == 'V'&& \
            ppage_header->signature[3] == 'L' && \
            (!mvVerifyChecksum((MV_PU8)ppage_header,sizeof(page_header))))
        {
            if (ppage_header->page_code == page_code){
                MV_DPRINT(("[Sucess](get_info_page) get page in offset 0x%X\n",flash_offset));
                return MV_TRUE;

            }else{
                flash_offset = (MV_U32)ppage_header->next_page * DEFAULT_PAGE_SIZE;
                MV_DPRINT(("Try to search PHY_INFO page in 0x%X\n",flash_offset));
                continue;
            }
        }else{
            MV_DPRINT(("[ERROR](get_info_page) get incorrect page in offset 0x%X\n",flash_offset));
            return MV_FALSE;

        }
    }

    /* we shold not reach here, because of it return MV_TRUE immediately*/
    return MV_FALSE;
}
#endif

MV_U8 api_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_U8 ret;

	/* check core internal requests */
	switch (req->Cdb[0]) {
	case SCSI_CMD_INQUIRY:
		ret = core_api_inquiry(root->core, req);
		break;
/* virtual device don't support scsi read capacity command */
#if 0
	case SCSI_CMD_READ_CAPACITY_10:
		ret = core_api_read_capacity(root->core, req);
		break;
	case SCSI_CMD_READ_CAPACITY_16:
		ret = core_api_read_capacity_16(root->core, req);
		break;
#endif		
#if defined(SUPPORT_ROC) && defined(SUPPORT_BALDUR)
	case SCSI_CMD_REPORT_LUN:
		ret = core_api_report_lun(root->core, req);
		break;
#endif
	case APICDB0_ADAPTER: {
        switch (req->Cdb[1]) {
            case APICDB1_ADAPTER_BBU_INFO:
                ret = mv_get_bbu_info(root->core, req);
                break;
            case APICDB1_ADAPTER_BBU_SET_THRESHOLD:
                ret = mv_set_bbu_threshold(root->core, req);
                break;
            case APICDB1_ADAPTER_BBU_POWER_CHANGE:
                ret = mv_bbu_power_change(root->core, req);
                break;
	    default:
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		ret = MV_QUEUE_COMMAND_RESULT_FINISHED;
		break;
        	}
    	}
		break;
	case APICDB0_PD:
		ret = core_pd_command(root->core, req);
		break;
	case APICDB0_DBG:
		ret = core_dbg_command(root->core, req);
		break;
#ifdef SUPPORT_FLASH
	case APICDB0_FLASH:
		ret = core_flash_command(root->core, req);
		break;
#endif
#ifdef SUPPORT_CONFIG_FILE
	case APICDB0_PHY:
		ret = core_api_phy_command(root->core, req);
		break;
#endif
#ifdef SUPPORT_BOARD_ALARM
	case APICDB0_BUZZER:
		ret = core_api_alarm_command(root->core, req);
		break;
#endif
	case APICDB0_IOCONTROL:
		ret = core_api_disk_io_control(root->core, req);
		break;

#ifdef SUPPORT_PASS_THROUGH_DIRECT
	case APICDB0_PASS_THRU_CMD_SCSI:
	case APICDB0_PASS_THRU_CMD_SCSI_16:
	case APICDB0_PASS_THRU_CMD_ATA:
		ret = core_pass_thru_send_command(root->core, req);
		break;
#endif
	case SCSI_CMD_RCV_DIAG_RSLT:
	case SCSI_CMD_SND_DIAG:
		ret = core_ses_command(root->core, req);
		break;
#ifdef _OS_LINUX
	case SCSI_CMD_MARVELL_SPECIFIC:
		if ((req->Cdb[1] == CDB_CORE_MODULE)
			&& (req->Cdb[2] == CDB_CORE_SHUTDOWN)) {
			core_abort_all_running_requests(root->core);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			ret = MV_QUEUE_COMMAND_RESULT_FINISHED;;
              	break;
		}
#endif		
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		ret = MV_QUEUE_COMMAND_RESULT_FINISHED;
		break;
	}

        return ret;
}

MV_U8 core_get_disk_info(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	POS_disk_info os_disk_p;
	domain_base *p_base;
	domain_device *dev;
	MV_U16 id;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(OS_disk_info)) {

		core_unmap_data_buffer(req);
		return REQ_STATUS_INVALID_REQUEST;
	}
	MV_ZeroMemory(buf_ptr, req->Data_Transfer_Length);
	id = __MAKE_DEV_ID(req->Cdb[2], req->Cdb[3]);
	p_base = get_device_by_id(&core->lib_dev, id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));

	if (!p_base) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		core_unmap_data_buffer(req);
		return REQ_STATUS_ERROR_WITH_SENSE;
	}
	os_disk_p = (POS_disk_info)buf_ptr;
	os_disk_p->queue_depth = p_base->queue_depth;
	/* Need add support multiple LUN check here */
	//os_disk_p->disk_type = p_base->type;

	if (p_base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)p_base;
		if (IS_STP_OR_SATA(dev))
			os_disk_p->disk_type = DISK_TYPE_SATA;
		else
			os_disk_p->disk_type = DISK_TYPE_SAS;
		os_disk_p->capacity = dev->capability;
	}
	core_unmap_data_buffer(req);
	return REQ_STATUS_SUCCESS;
}

MV_U8 core_set_disk_queue_depth(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	POS_disk_info os_disk_p;
	domain_base *p_base;
	domain_device *dev;
	MV_U16 id;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(OS_disk_info)) {

		core_unmap_data_buffer(req);
		return REQ_STATUS_INVALID_REQUEST;
	}
	id = __MAKE_DEV_ID(req->Cdb[2], req->Cdb[3]);
	p_base = get_device_by_id(&core->lib_dev, id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));

	if (!p_base)
		goto core_set_disk_queue_depth_id_error;

	os_disk_p = (POS_disk_info)buf_ptr;

	if (os_disk_p->queue_depth == 0)
		goto core_set_disk_queue_depth_param_error;

	switch (p_base->type) {
	case BASE_TYPE_DOMAIN_DEVICE:
		dev = (domain_device *)p_base;
		if (IS_SSP(dev)) {
			if (os_disk_p->queue_depth > CORE_SAS_DISK_QUEUE_DEPTH) {
				goto core_set_disk_queue_depth_param_error;
			}
		}
		else if (IS_STP_OR_SATA(dev) || IS_ATAPI(dev)) {
			if (os_disk_p->queue_depth > 32) {
				goto core_set_disk_queue_depth_param_error;
			}
		}
		else {
			goto core_set_disk_queue_depth_id_error;
		}
		break;
	case BASE_TYPE_DOMAIN_EXPANDER:
	case BASE_TYPE_DOMAIN_ENCLOSURE:
	case BASE_TYPE_DOMAIN_PM:
		if (os_disk_p->queue_depth > 1)
			goto core_set_disk_queue_depth_param_error;
		break;
	default:
		goto core_set_disk_queue_depth_id_error;
		//break;
	}

	p_base->queue_depth = os_disk_p->queue_depth;
	core_unmap_data_buffer(req);
	return REQ_STATUS_SUCCESS;

core_set_disk_queue_depth_id_error:
	if (req->Sense_Info_Buffer != NULL)
		((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
	core_unmap_data_buffer(req);
	return REQ_STATUS_ERROR_WITH_SENSE;

core_set_disk_queue_depth_param_error:
	if (req->Sense_Info_Buffer != NULL)
		((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_PARAMETER;
	core_unmap_data_buffer(req);
	return REQ_STATUS_ERROR_WITH_SENSE;
}

MV_U8 core_set_disk_queue_type(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	POS_disk_info os_disk_p;
	domain_device *dev;
	MV_U16 id;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(OS_disk_info)) {

		core_unmap_data_buffer(req);
		return REQ_STATUS_INVALID_REQUEST;
	}
	id = __MAKE_DEV_ID(req->Cdb[2], req->Cdb[3]);
	dev = (domain_device *)get_device_by_id(&core->lib_dev, id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));

	if (!dev)
		goto core_set_disk_queue_type_id_error;

	os_disk_p = (POS_disk_info)buf_ptr;

	/*for linux only these two types are supported*/
	if (os_disk_p->queue_type != TCQ_QUEUE_TYPE_SIMPLE
		&& os_disk_p->queue_type != TCQ_QUEUE_TYPE_ORDERED)
		goto core_set_disk_queue_type_param_error;

	dev->queue_type = os_disk_p->queue_type;
	core_unmap_data_buffer(req);
	return REQ_STATUS_SUCCESS;

core_set_disk_queue_type_id_error:
	if (req->Sense_Info_Buffer != NULL)
		((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
	core_unmap_data_buffer(req);
	return REQ_STATUS_ERROR_WITH_SENSE;

core_set_disk_queue_type_param_error:
	if (req->Sense_Info_Buffer != NULL)
		((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_PARAMETER;
	core_unmap_data_buffer(req);
	return REQ_STATUS_ERROR_WITH_SENSE;
}


MV_U8 core_api_disk_io_control(MV_PVOID core_p, PMV_Request req)
{
	switch (req->Cdb[1]) {
	case APICDB1_GET_OS_DISK_INFO:
		req->Scsi_Status = core_get_disk_info(core_p, req);
		break;
	case APICDB1_SET_OS_DISK_QUEUE_DEPTH:
		req->Scsi_Status = core_set_disk_queue_depth(core_p, req);
		break;
	case APICDB1_SET_OS_DISK_QUEUE_TYPE:
		req->Scsi_Status = core_set_disk_queue_type(core_p, req);
		break;
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		break;
	}
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_api_inquiry(MV_PVOID core_p, PMV_Request req)
{
	MV_PVOID buf_ptr = core_map_data_buffer(req);
	MV_U32 length;

	MV_ZeroMemory(buf_ptr, req->Data_Transfer_Length);

	if (req->Cdb[1] & CDB_INQUIRY_EVPD) {
	/*
		MV_U8 MV_INQUIRY_VPD_PAGE0_DATA[6] = {0x00, 0x00, 0x00, 0x02, 0x00, 0x80};
		MV_U8 MV_INQUIRY_VPD_PAGE83_DATA[16] = {
			0x00, 0x83, 0x00, 0x0C, 0x01, 0x02, 0x00, 0x08,
			0x00, 0x50, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00};
	 */
		MV_U32 tmpLen = 0;
		req->Scsi_Status = REQ_STATUS_SUCCESS;

		/* Shall return the specific page of Vital Production Data */
		switch (req->Cdb[2]) {
		case 0x00:	/* Supported VPD pages */
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				tmpLen = MV_MIN(req->Data_Transfer_Length, VPD_PAGE0_VIRTUALD_SIZE);
				MV_CopyMemory(buf_ptr, MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA, tmpLen);
			}

			break;
		case 0x80:	/* Unit Serial Number VPD Page */
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				tmpLen = MV_MIN(req->Data_Transfer_Length, VPD_PAGE80_VIRTUALD_SIZE);
				MV_CopyMemory(buf_ptr, MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA, tmpLen);
			}

			break;
		case 0x83:	/* Device Identification VPD Page */
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				tmpLen = MV_MIN(req->Data_Transfer_Length, VPD_PAGE83_VIRTUALD_SIZE);
				MV_CopyMemory(buf_ptr, MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA, tmpLen);
			}
			break;
		default:
			req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB);
			break;
		}
		req->Data_Transfer_Length = tmpLen;
	} else {
		/* Standard inquiry */
		if (req->Cdb[2]!=0) {
			/* PAGE CODE field must be zero when EVPD is zero for a valid request */
			/* sense key as ILLEGAL REQUEST and additional sense code as INVALID FIELD IN CDB */
			req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB);
		} else {
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				length = MV_MIN(req->Data_Transfer_Length, VIRTUALD_INQUIRY_DATA_SIZE);
				MV_CopyMemory(buf_ptr, MV_INQUIRY_VIRTUALD_DATA, length);
				req->Data_Transfer_Length = length;
				req->Scsi_Status = REQ_STATUS_SUCCESS;
			}
		}
        }

        core_unmap_data_buffer(req);
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_api_read_capacity_16(core_extension *core, PMV_Request req)
{
	domain_device *device = NULL;
	MV_LBA max_lba;
	MV_U32 block_length;
	MV_PU32 p_buffer;

#ifndef SECTOR_SIZE
	#define SECTOR_SIZE	512	//TBD
#endif
    
    MV_U16 device_id;
	domain_base *base = NULL;

	if ((req->Cdb[1] & 0x1f)!=SCSI_CMD_SAI_READ_CAPACITY_16) {
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	p_buffer = (MV_PU32)core_map_data_buffer(req);
	if (p_buffer == NULL || req->Data_Transfer_Length < 12) {
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

    device_id = req->Cdb[2] | (req->Cdb[3] << 8); // targetID

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
									req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));

    if (base == NULL) {
        req->Scsi_Status = REQ_STATUS_NO_DEVICE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }

    if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
        if (req->Sense_Info_Buffer != NULL)
            ((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
        
    device = (domain_device *)base;

	/*
	 *  The disk size as indicated by the ATA spec is the total addressable
	 *  sectors on the drive ; while the SCSI translation of the command
	 *  should be the last addressable sector.
	 */

	max_lba = device->max_lba;
	block_length = device->logical_sector_size; //SECTOR_SIZE;			//TBD
	p_buffer[0] = MV_CPU_TO_BE32(max_lba.parts.high);
	p_buffer[1] = MV_CPU_TO_BE32(max_lba.parts.low);
	p_buffer[2] = MV_CPU_TO_BE32(block_length);

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_api_read_capacity(core_extension *core, PMV_Request req)
{
	domain_device *device = NULL;
	MV_LBA max_lba;
	MV_U32 block_length;
	MV_PU32 p_buffer;

#ifndef SECTOR_SIZE
	#define SECTOR_SIZE	512	//TBD
#endif

    MV_U16 device_id;
    domain_base *base = NULL;

	if ((req->Cdb[8] & MV_BIT(1)) == 0) {
		if (req->Cdb[2] || req->Cdb[3] || req->Cdb[4] || req->Cdb[5]) {
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	}

	p_buffer = (MV_PU32)core_map_data_buffer(req);
	if (p_buffer == NULL || req->Data_Transfer_Length < 8) {
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

    device_id = req->Cdb[2] | (req->Cdb[3] << 8); // targetID

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
    if (base == NULL) {
        req->Scsi_Status = REQ_STATUS_NO_DEVICE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }

    if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
        if (req->Sense_Info_Buffer != NULL)
            ((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
        
    device = (domain_device *)base;

	/*
	 *  The disk size as indicated by the ATA spec is the total addressable
              *  sectors on the drive ; while the SCSI translation of the command
              *  should be the last addressable sector.
              */

	max_lba = device->max_lba;
	block_length = device->logical_sector_size;//SECTOR_SIZE;			//TBD

	if(max_lba.parts.high != 0)
		max_lba.parts.low = 0xFFFFFFFF;
	p_buffer[0] = MV_CPU_TO_BE32(max_lba.parts.low);
	p_buffer[1] = MV_CPU_TO_BE32(block_length);

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_api_report_lun(MV_PVOID core_p, PMV_Request req)
{
	MV_U32 alloc_len, lun_list_len;
	MV_PU8 buf_ptr;

	alloc_len = ((MV_U32)(req->Cdb[6] << 24)) |
				((MV_U32)(req->Cdb[7] << 16)) |
				((MV_U32)(req->Cdb[8] << 8)) |
				((MV_U32)(req->Cdb[9]));

	buf_ptr = core_map_data_buffer(req);

	/* allocation length should not less than 16 bytes */
	if (alloc_len < 16 || buf_ptr == NULL) {
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	MV_ZeroMemory(buf_ptr, alloc_len);
#ifdef _OS_UKRN
	lun_list_len = 8;
#else
	/* LUN1 may have device */
	lun_list_len = 16;
	if (alloc_len >= 24 && req->Data_Transfer_Length >= 24)
		buf_ptr[23] = 0x01;	/* LUN1 has device */
#endif

	if (req->Data_Transfer_Length >= 4) {
		buf_ptr[0] = (MV_U8)((lun_list_len & 0xFF000000) >> 24);
		buf_ptr[1] = (MV_U8)((lun_list_len & 0x00FF0000) >> 16);
		buf_ptr[2] = (MV_U8)((lun_list_len & 0x0000FF00) >> 8);
		buf_ptr[3] = (MV_U8)(lun_list_len & 0x000000FF);
	}

	core_unmap_data_buffer(req);
	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}


MV_VOID
core_get_pm_information(core_extension *core, domain_pm *pm,
PPM_Info pm_info)
{
	domain_port *port = pm->base.port;
	MV_U8 i;

	pm_info->Link.Self.DevID = pm->base.id;
	if (pm->state != PM_STATE_DONE) {
		pm_info->Link.Self.DevType = DEVICE_TYPE_NONE;
		return;
	}

	for(i=0;i<pm->base.root->phy_num;i++){
		if(port->phy_map&MV_BIT(i))
			break;
	}

	pm_info->Link.Self.DevType = DEVICE_TYPE_PM;
	pm_info->Link.Self.PhyCnt = 1;
	pm_info->Link.Self.PhyID[0] = i;
	pm_info->Link.Parent.DevType = DEVICE_TYPE_PORT;
	pm_info->Link.Parent.DevID = 0;
	pm_info->Link.Parent.PhyCnt = 1;
	pm_info->Link.Parent.PhyID[0] = i+(MV_U8)pm->base.root->base_phy_num;
	pm_info->Link.Parent.EnclosureID = 0xFFFF;
	pm_info->Link.Self.EnclosureID = 0xFFFF;

	pm_info->VendorId = pm->vendor_id;
	pm_info->DeviceId = pm->device_id;
	pm_info->ProductRevision = pm->product_rev;
	pm_info->PMSpecRevision = pm->spec_rev;

	pm_info->NumberOfPorts = pm->num_ports;
}

MV_VOID
core_get_pm_info(MV_PVOID extension, MV_PVOID buffer)
{
	core_extension *core = (core_extension *)extension;
	MV_U16 start_id = 0, end_id = 0;
	domain_base *base = NULL;
	domain_pm *pm = NULL;
	PPM_Info pm_info;
	PPM_Info_Request pm_req;
	PRequestHeader header = NULL;
	MV_U16 i;

	pm_req = (PPM_Info_Request)buffer;
	header = &pm_req->header;

	if (header->requestType == REQUEST_BY_RANGE) {
		start_id = header->startingIndexOrId;
		end_id = MAX_ID;
	} else if (header->requestType == REQUEST_BY_ID) {
		start_id = header->startingIndexOrId;
		end_id = header->startingIndexOrId;
	}
	pm_info = pm_req->pmInfo;
	header->numReturned = 0;
	header->nextStartingIndex = NO_MORE_DATA;

	for (i = start_id; i <= end_id; i++) {
		base = get_device_by_id(&core->lib_dev, i, MV_FALSE, MV_FALSE);
		if ((base != NULL)&&(base->type == BASE_TYPE_DOMAIN_PM)) {
			pm = (domain_pm *)base;
			core_get_pm_information( core, pm, pm_info );
		} else {
			pm_info->Link.Self.DevID = i;
			pm_info->Link.Self.DevType = DEVICE_TYPE_NONE;
		}

		if (pm_info->Link.Self.DevType != DEVICE_TYPE_NONE) {
			header->numReturned++;
			pm_info++;
			if ((header->requestType == REQUEST_BY_RANGE) &&
				(header->numReturned == header->numRequested)) {
				header->nextStartingIndex = i+1;
				break;
			}
		}
	}
}

MV_U8
core_pd_request_get_pm_info(core_extension * core_p, PMV_Request req)
{
    MV_PVOID buf_ptr = core_map_data_buffer(req);
	PPM_Info_Request pm_req = (PPM_Info_Request)buf_ptr;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(PM_Info_Request)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (pm_req->header.requestType == REQUEST_BY_RANGE &&
		pm_req->header.startingIndexOrId == 0){
		pm_req->header.startingIndexOrId = 0;
	} else if (pm_req->header.startingIndexOrId >= MAX_ID) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_PM_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	core_get_pm_info(core_p, buf_ptr);
	if (pm_req->header.requestType == REQUEST_BY_ID &&
		pm_req->header.numReturned == 0) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_PM_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

        core_unmap_data_buffer(req);
	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}



MV_U8 core_pd_request_bsl_dump(core_extension * core_p, PMV_Request req)
{
	req->Scsi_Status = REQ_STATUS_ERROR;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

core_management_command_handler BASEATTR core_pd_cmd_handler[APICDB1_PD_MAX] =
{
	core_pd_request_get_hd_info,
	core_pd_request_get_expander_info,
	core_pd_request_get_pm_info,
	core_pd_request_get_hd_config,
	core_pd_request_set_hd_config,
	core_pd_request_bsl_dump,
#ifdef SUPPORT_SES
	core_pd_request_get_enclosure_info,
#else
	NULL,
#endif
	NULL,
	core_pd_request_get_hd_status,
	core_pd_request_get_hd_info_ext
};

MV_U8 core_pd_command(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;

	if ( req->Cdb[1] >= APICDB1_PD_MAX )
	{
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	if (core_pd_cmd_handler[req->Cdb[1]] == NULL)
	{
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	return core_pd_cmd_handler[req->Cdb[1]](core, req);
}

void Core_GetDeviceId(MV_PVOID pModule,MV_U16 begin_id,PDevice_Index pDeviceIndex)
{
	core_extension *core;
	MV_U16 index;
	domain_base *base = NULL;
	domain_device *device = NULL;
	core = (core_extension *)HBA_GetModuleExtension(pModule, MODULE_CORE);
	pDeviceIndex->end = MV_FALSE;
	pDeviceIndex->device_id = 0xFFFF;
	pDeviceIndex->index = 0xFFFF;
	index = begin_id;
	for(;index<MAX_ID;index++)
	{
		base = get_device_by_id(&core->lib_dev, index, MV_FALSE, MV_FALSE);
		if ((base != NULL) && (base->type == BASE_TYPE_DOMAIN_DEVICE)) {
			device = (domain_device *)base;
			if((device->status & DEVICE_STATUS_FUNCTIONAL)&&(IS_HDD(device))&&(!IS_ATAPI(device)))
			{
				pDeviceIndex->device_id = device->base.id ;
				pDeviceIndex->index = index;
				break;
			}
		}
	}
	if(index>=MAX_ID)
		pDeviceIndex->end = MV_TRUE;
	return;
}

#ifdef SUPPORT_PASS_THROUGH_DIRECT
PPassThorugh_Config pConfig;
MV_U32 Data_Length;

MV_U32
core_pass_thru_cal_correct_length (
	MV_U32 length
)
{
	if( length & 0x80000000)
		return (0xFFFFFFFF - length )+2;
	else
		return 0;
}

void
core_pass_thru_callback(MV_PVOID root_p, MV_Request *req)
{
	MV_Request *org_req = req->Org_Req;
	PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)org_req->Org_Req;
	pl_root *root = (pl_root *)root_p;
	PHBA_Extension hba;
	core_extension *core = (core_extension *)root->core;
	MV_U32 length;
	MV_U32 reg[3];
	MV_U16 deviceId;
	MV_U8 testsize;
        MV_PVOID buf_ptr, new_buf_ptr;

        hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);

	if (req->Scsi_Status == REQ_STATUS_PENDING) {
		MV_DASSERT(MV_FALSE);
		return;
	}
        org_req->Scsi_Status = req->Scsi_Status;

        buf_ptr = core_map_data_buffer(org_req);
        //pConfig = (PPassThorugh_Config)buf_ptr;

	// SCSI Pass Through
	if (org_req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI ||
		org_req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI_16) {
#ifdef SAT_RETURN_FIS_IN_CDB
		switch (org_req->Cdb[0]) {
		case APICDB0_PASS_THRU_CMD_SCSI:
			MV_CopyMemory(&org_req->Cdb[4], req->Cdb, 12);
			break;
		case APICDB0_PASS_THRU_CMD_SCSI_16:
			MV_CopyMemory(buf_ptr, req->Cdb, 16);
			break;
		default:
			break;
		}
#endif /* SAT_RETURN_FIS_IN_CDB */
		if (req->Cdb[0] == SCSI_CMD_READ_LONG_10) {
			if (req->Sense_Info_Buffer != NULL) {
				((MV_PU8)org_req->Sense_Info_Buffer)[0] = 0;
				// Find the sense buffer return info
				length = (((MV_PU8)req->Sense_Info_Buffer)[3] << 24)|
					(((MV_PU8)req->Sense_Info_Buffer)[4] << 16)|
					(((MV_PU8)req->Sense_Info_Buffer)[5] << 8)|
					((MV_PU8)req->Sense_Info_Buffer)[6];

				// Calculate the correct length for ECC
				if (((MV_PU8)req->Sense_Info_Buffer)[2] == 0x25) {
					length = core_pass_thru_cal_correct_length(length);

	                                if (org_req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI_16 ||
                                                org_req->Data_Transfer_Length < MAX_PASS_THRU_DATA_BUFFER_SIZE) {
		                                // from SCSI commander application
		                                org_req->Data_Transfer_Length = length;
	                                } else {
		                                // from CLI application
		                                //pConfig->Data_Length = length;
	                                }
					goto HBACallback;
				}
			}
		}
	} else if (org_req->Cdb[0] == APICDB0_PASS_THRU_CMD_ATA) {
	    //ATA Pass Through
	    //FM_PRINT("%s ata pass thru callback\n", __FUNCTION__);
    }

HBACallback:
	req->Data_Buffer = NULL;
	req->Data_Transfer_Length = 0;
	((core_context *)(req->Context[MODULE_CORE]))->buf_wrapper = NULL;
        SGTable_Init(&req->SG_Table, 0);

        core_unmap_data_buffer(org_req);
	core_queue_completed_req(root->core, org_req);// not call callback function directly
}

MV_U8
core_pass_thru_send_scsi_command(
	IN core_extension *core,
	IN PMV_Request req)
{
	PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)req->Org_Req;
	MV_U16 device_id;
	domain_base *base = NULL;
    domain_device *device = NULL;
	PMV_SG_Table pSGTable;
	MV_U32 lengthLocation;
	MV_Request *new_req = NULL;
	MV_PVOID buf_ptr, new_buf_ptr;

	if (req == NULL) {
		MV_ASSERT(MV_FALSE);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	device_id = req->Cdb[2] | (req->Cdb[3] << 8); // targetID
	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

    if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
        
    device = (domain_device *)base;
    if (IS_STP_OR_SATA(device) || IS_ATAPI(device)) {
        //SCSI pass thru command should ONLY sent to SCSI device
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }

	buf_ptr = core_map_data_buffer(req);
	//pConfig = (PPassThorugh_Config)buf_ptr;

	if (req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI_16 && 
		(buf_ptr == NULL || req->Data_Transfer_Length < 16)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = get_intl_req_resource(base->root, 0);
	if(new_req == NULL){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}


	if (req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI_16 ||
                req->Data_Transfer_Length < MAX_PASS_THRU_DATA_BUFFER_SIZE)
		Data_Length = req->Data_Transfer_Length;
	//else
	//	Data_Length = pConfig->Data_Length;

	switch (req->Cdb[1]) {
	case APICDB1_SCSI_NON_DATA:
		new_req->Cmd_Flag = 0;
		break;
	case APICDB1_SCSI_PIO_IN:
		new_req->Cmd_Flag = CMD_FLAG_PIO | CMD_FLAG_DATA_IN;
		break;
	case APICDB1_SCSI_PIO_OUT:
		new_req->Cmd_Flag = CMD_FLAG_PIO;
		break;
	default:
		break;
	}

	new_req->Device_Id = device_id;

	if (req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI_16) {
		MV_CopyMemory(new_req->Cdb, buf_ptr, 16);
		if (req->Data_Transfer_Length > 16) {
			new_req->Data_Buffer = (MV_PVOID)&((MV_PU8)buf_ptr)[16];
			new_req->Data_Transfer_Length =
				req->Data_Transfer_Length - 16;
			((core_context *)new_req->Context[MODULE_CORE])->buf_wrapper =
				((core_context *)req->Context[MODULE_CORE])->buf_wrapper;
			MV_CopyPartialSGTable(
				&new_req->SG_Table,
				&req->SG_Table,
				16, /* offset */
				req->SG_Table.Byte_Count - 16 /* size */
				);
		}
	} else {
		MV_CopyMemory(new_req->Cdb, &req->Cdb[4], 12);
		new_req->Data_Buffer = req->Data_Buffer;
		new_req->Data_Transfer_Length = req->Data_Transfer_Length;
		((core_context *)new_req->Context[MODULE_CORE])->buf_wrapper =
			((core_context *)req->Context[MODULE_CORE])->buf_wrapper;

		if (req->Data_Transfer_Length > 0) {
			MV_CopyPartialSGTable(
				&new_req->SG_Table,
				&req->SG_Table,
				0, /* offset */
				req->SG_Table.Byte_Count /* size */
				);
		}
	}

	new_req->Completion = (void(*)(MV_PVOID,PMV_Request))core_pass_thru_callback;
	new_req->Org_Req = req;

	/* Send this internal request */
	core_append_request(base->root, new_req);

	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

MV_U8
core_pass_thru_send_ata_command(
	IN core_extension *core,
	IN PMV_Request req)
{
    PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)req->Org_Req;
    MV_U16 device_id;
    domain_base *base = NULL;
    domain_device *device = NULL;
    PMV_SG_Table pSGTable;
    MV_U32 lengthLocation;
    MV_Request *new_req = NULL;
    MV_PVOID buf_ptr, new_buf_ptr;
    ata_reg *smart_buf;
    
    if (req == NULL) {
        MV_ASSERT(MV_FALSE);
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
    
    device_id = req->Cdb[3] | (req->Cdb[2] << 8); // targetID
	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
    if (base == NULL) {
        req->Scsi_Status = REQ_STATUS_NO_DEVICE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }

    if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
        if (req->Sense_Info_Buffer != NULL)
            ((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
        
    device = (domain_device *)base;
    if (!IS_STP_OR_SATA(device)) {
        //ATA pass thru command should ONLY sent to ATA device
        req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }

    if (req->Cdb[1] != APICDB1_ATA_PD) {
        req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
        return MV_QUEUE_COMMAND_RESULT_FINISHED;
    }
    
    buf_ptr = core_map_data_buffer(req);

    new_req = get_intl_req_resource(base->root, 0);
    if (new_req == NULL){
        CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
        core_unmap_data_buffer(req);
        return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
    }

        
    smart_buf = (ata_reg *)req->Data_Buffer;

    new_req->Data_Transfer_Length = req->Data_Transfer_Length - sizeof(ata_reg) + 1;

    //TBD: ATA pass thru protocol & direction depened on upper API use
    // CDB or certain way send it
    if (req->Cmd_Flag & CMD_FLAG_DATA_IN) {
        new_req->Cdb[1] = ATA_PROTOCOL_PIO_IN << 1;
        new_req->Cdb[2] = (ATA_PASSTHRU_TDIR_IN << 3) | (1 << 2) | ATA_PASSTHRU_LENGTH_IN_SEC_CNT;
    } else if (req->Cmd_Flag & CMD_FLAG_DATA_OUT) {
        new_req->Cdb[1] = ATA_PROTOCOL_PIO_OUT << 1;
        new_req->Cdb[2] = (ATA_PASSTHRU_TDIR_OUT << 3) | (1 << 2) | ATA_PASSTHRU_LENGTH_IN_SEC_CNT;
    } else {
        new_req->Cdb[1] = ATA_PROTOCOL_NON_DATA << 1;
        new_req->Cdb[2] = ATA_PASSTHRU_LENGTH_IN_ZERO;
    }

    new_req->Cdb[0] = SCSI_CMD_ATA_PASSTHRU_12;
    /*
        cdb[3]:feature reg
        cdb[4]:sec_cnt reg
        cdb[5]:lba_low reg
        cdb[6]:lba_mid reg
        cdb[7]:lba_hi reg
        cdb[8]:device reg
        cdb[9]:command reg
        */
    MV_CopyMemory(&new_req->Cdb[3], &smart_buf->drive_regs, 7);

    switch (smart_buf->drive_regs[0]) {
        case ATA_CMD_SMART_READ_DATA:
        case ATA_CMD_SMART_READ_ATTRIBUTE_THRESHOLDS:
            new_req->Cmd_Flag = CMD_FLAG_PIO | CMD_FLAG_DATA_IN;
            new_req->Data_Transfer_Length = SMART_DATA_LENGTH;
            new_req->Cdb[1] = ATA_PROTOCOL_PIO_IN << 1;
            new_req->Cdb[2] = (ATA_PASSTHRU_TDIR_IN << 3) | (1 << 2) | ATA_PASSTHRU_LENGTH_IN_SEC_CNT;
            break;
        case ATA_CMD_SMART_WRITE_LOG:
            new_req->Cmd_Flag = CMD_FLAG_PIO | CMD_FLAG_DATA_OUT;
            new_req->Data_Transfer_Length = SMART_DATA_LENGTH * smart_buf->drive_regs[0];
            new_req->Cdb[1] = ATA_PROTOCOL_PIO_OUT << 1;
            new_req->Cdb[2] = (ATA_PASSTHRU_TDIR_OUT << 3) | (1 << 2) | ATA_PASSTHRU_LENGTH_IN_SEC_CNT;
            break;
        case ATA_CMD_SMART_READ_LOG:
            new_req->Cmd_Flag = CMD_FLAG_PIO | CMD_FLAG_DATA_IN;
            new_req->Data_Transfer_Length = SMART_DATA_LENGTH * smart_buf->drive_regs[0];
            new_req->Cdb[1] = ATA_PROTOCOL_PIO_IN << 1;
            new_req->Cdb[2] = (ATA_PASSTHRU_TDIR_IN << 3) | (1 << 2) | ATA_PASSTHRU_LENGTH_IN_SEC_CNT;
            break;
            
        case ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE:
        case ATA_CMD_SMART_SAVE_ATTRIBUTE_VALUES:
        case ATA_CMD_SMART_EXECUTE_OFFLINE:
        case ATA_CMD_ENABLE_SMART:
        case ATA_CMD_DISABLE_SMART:
        case ATA_CMD_SMART_RETURN_STATUS:
            new_req->Data_Transfer_Length = 0;
            new_req->Cdb[1] = ATA_PROTOCOL_NON_DATA << 1;
            new_req->Cdb[2] = ATA_PASSTHRU_LENGTH_IN_ZERO;
            break;
            
        default:
            break;
    }

    if (new_req->Data_Transfer_Length) {
		new_req->Data_Buffer = &smart_buf->data;
		((core_context *)new_req->Context[MODULE_CORE])->buf_wrapper =
			((core_context *)req->Context[MODULE_CORE])->buf_wrapper;

		MV_CopyPartialSGTable(
			&new_req->SG_Table,
			&req->SG_Table,
			16, /* offset */
			req->SG_Table.Byte_Count - 16 /* size */
			);
    }

    new_req->Device_Id = device_id;
    new_req->Org_Req = req;
    new_req->Completion = core_pass_thru_callback;

    core_append_request(base->root, new_req);

    core_unmap_data_buffer(req);
    return MV_QUEUE_COMMAND_RESULT_REPLACED;

}

MV_U8
core_pass_thru_send_command(
	IN MV_PVOID *core_p,
	IN PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	MV_U8 ret;

	switch(req->Cdb[0]) {
    	case APICDB0_PASS_THRU_CMD_SCSI:
    	case APICDB0_PASS_THRU_CMD_SCSI_16:
    		ret = core_pass_thru_send_scsi_command(core, req);
	        break;
        case APICDB0_PASS_THRU_CMD_ATA:
    		ret = core_pass_thru_send_ata_command(core, req);
            break;
    	default:
    		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
    		ret = MV_QUEUE_COMMAND_RESULT_FINISHED;
    		break;
	}
	return ret;
}

#endif /* SUPPORT_PASS_THROUGH_DIRECT */



#define VersionLen 16
MV_BOOLEAN is_old_version_format(MV_PU8 buff)
{
	MV_U8 i = 0, dot_count = 0;
	/*now we search count of '.' in string*/
	for (i =0; i < VersionLen; i++) {
		if(buff[i] == '.')
			dot_count++;
	}
	if (dot_count == 3)
		return MV_TRUE;
	else
		return MV_FALSE;
}
void Core_Flash_BIOS_Version(MV_PVOID extension,PMV_Request req)// extern declared in mvGetAdapterInfo is "req" not "Req"
{
	AdapterInfo	adapter_info;
	MV_U32	flash_start_addr,Size;
	MV_U8	buff[VersionLen];
	PAdapter_Info data;
	core_extension *core = (core_extension *)extension;
	MV_U8 i=0,j=0,temp=0,dot[4]={0,0,0,0};
	PHBA_Extension pHBA = (PHBA_Extension)HBA_GetModuleExtension(core,MODULE_HBA);
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(AdapterInfo)) {
		
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return;
	}

	adapter_info.bar[FLASH_BAR_NUMBER] = pHBA->Base_Address[FLASH_BAR_NUMBER];
	if (OdinSPI_Init( &adapter_info )==-1)
    		CORE_DPRINT(("Core_Flash_Bin: FLASH Init FAILED!\n"));

	flash_start_addr=0x00000002;
	Size=VersionLen;

	OdinSPI_ReadBuf(&adapter_info, flash_start_addr, buff, Size);
	data =(PAdapter_Info)buf_ptr;

	if ((*(MV_PU32)(&buff[0])!=0xffffffff)||(*(MV_PU32)(&buff[4])!=0xffffffff)) {
		if(is_old_version_format(buff)){
			/*old BIOS version format*/
			/*parse BIOS version string*/
			data->BIOSVersion.MajorVersion = buff[0]-0x30;
			data->BIOSVersion.MinorVersion = buff[2]-0x30;
			data->BIOSVersion.RevisionNumber  = buff[4]-0x30;
			data->BIOSVersion.BuildNumber  = (buff[6]-0x30)*10+(buff[7]-0x30);

			/* 1. dot check */
			for(i = 0;i < VersionLen;i++) {
				if(buff[i]=='.')
					dot[j++]=i;
			}
			/* 2. fill number */
			for(i = 0;i < dot[0]; i++){
				if((dot[0]-i)>1)
					temp += (buff[i]-0x30)*10;
				else
					break;
			}
			temp += (buff[i]-0x30);
			data->BIOSVersion.MajorVersion = temp;
			temp = 0;
			for(i = dot[0]+1;i < dot[1]; i++){
				if ((dot[1]-i)>1)
					temp += (buff[i]-0x30)*10;
				else
					break;
			}
			temp += (buff[i]-0x30);
			data->BIOSVersion.MinorVersion = temp;
			temp =0;
			for (i = dot[1]+1; i < dot[2]; i++) {
				if((dot[2]-i)>1)
					temp += (buff[i]-0x30)*10;
				else
					break;
			}
			temp += (buff[i]-0x30);
			data->BIOSVersion.RevisionNumber = temp;
			temp =0;
			for (i = dot[2]+1;i < VersionLen; i++) {
				if(buff[i]!='0'&&buff[i]!='1'&&buff[i]!='2'&&buff[i]!='3'&&buff[i]!='4'&&buff[i]!='5'
					&&buff[i]!='6'&&buff[i]!='7'&&buff[i]!='8'&&buff[i]!='9'){
					dot[3]=i;
					break;
				}
			}
			for (i = dot[2]+1; i < dot[3]; i++) {
				if((dot[3]-i)>1)
					temp += (buff[i]-0x30)*10;
				else
					break;
			}
			temp += (buff[i]-0x30);
			data->BIOSVersion.BuildNumber = temp;
		}else
		MV_CopyMemory(&data->BIOSVersion, buff, sizeof (Version_Info_CIM));
	}
	core_unmap_data_buffer(req);
}

#ifdef SUPPORT_CONFIG_FILE
MV_U8 core_api_phy_command(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;

	switch (req->Cdb[1])
	{
	case APICDB1_PHY_TEST:
		req->Scsi_Status = core_phy_test(core, req);
		break;
	case APICDB1_PHY_STATUS:
		req->Scsi_Status = core_phy_get_status(core,req);
		break;
	case APICDB1_PHY_CONFIG:
		req->Scsi_Status = core_phy_config(core, req);
		break;
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		break;
	}
	
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

#endif

#ifdef SUPPORT_BOARD_ALARM
MV_U8
core_api_alarm_command(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;

	switch (req->Cdb[1]) {
	case APICDB1_BUZZER_ON:
		core_alarm_change_state(core, ALARM_STATE_1S_1S);
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case APICDB1_BUZZER_OFF:
		core_alarm_change_state(core, ALARM_STATE_OFF);
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case APICDB1_BUZZER_MUTE:
		core_alarm_change_state(core, ALARM_STATE_MUTE);
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case APICDB1_BUZZER:
	default:
		req->Scsi_Status = REQ_STATUS_ERROR;
		break;
	}

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}
#endif

