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

#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_console.h"
#include "core_util.h"
#include "core_sas.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
MV_VOID
core_get_hd_information(core_extension *core, domain_device *dev,
	PHD_Info hd)
{
	MV_U8 i;
	MV_U8 count;
	pl_root *root;
	domain_enclosure *enc = NULL;

	hd->Link.Self.DevID = dev->base.id ;
	if (dev->base.type == BASE_TYPE_DOMAIN_DEVICE) {
			if (!(dev->status&DEVICE_STATUS_FUNCTIONAL) ||
				 (dev->status & DEVICE_STATUS_SECONDARY_PATH) ||
				 (dev->state != DEVICE_STATE_INIT_DONE)) {
				hd->Link.Self.DevType = DEVICE_TYPE_NONE;
				return;
			}
		hd->Link.Self.DevType = DEVICE_TYPE_HD;
		hd->Link.Self.EnclosureID = 0xFFFF;
		hd->Link.Parent.EnclosureID = 0xFFFF;

		if (IS_ENCLOSURE(dev))
			hd->Link.Self.DevType = DEVICE_TYPE_ENCLOSURE;

		if (IS_TAPE(dev))
			hd->Link.Self.DevType = DEVICE_TYPE_TAPE;

		hd->Link.Self.PhyID[0] = 0;
		hd->Link.Self.PhyCnt = 1;

		if (dev->base.parent != NULL) {
			if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
				hd->Link.Parent.DevID = dev->base.parent->id;
				hd->Link.Parent.DevType = DEVICE_TYPE_EXPANDER;
				hd->Link.Parent.PhyID[0] = dev->parent_phy_id;
				hd->Link.Parent.PhyCnt = 1;
#ifdef SUPPORT_SES
				if (((domain_expander *)(dev->base.parent))->enclosure)
					hd->Link.Parent.EnclosureID = ((domain_expander *)(dev->base.parent))->enclosure->base.id;
#else
				hd->Link.Parent.EnclosureID = 0xFFFF;
#endif
				MV_CopyMemory(hd->Link.Parent.SAS_Address, &((domain_expander *)dev->base.parent)->sas_addr.value, 8);
				MV_CopyMemory(hd->Link.Self.SAS_Address, &dev->sas_addr.value, 8);
			} else if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
				domain_pm *pm =(domain_pm *)dev->base.parent;
				root = dev->base.root;
				hd->Link.Parent.DevID = dev->base.parent->id;  /* PM ID */
				hd->Link.Parent.DevType = DEVICE_TYPE_PM;
				MV_ASSERT(dev->base.parent->port!=NULL);
				hd->Link.Parent.PhyID[0] =  ((MV_U8)(dev->base.parent->port->base.id<<4))|dev->pm_port;
				hd->Link.Parent.PhyCnt = 1;
				if (pm->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
					MV_CopyMemory(hd->Link.Parent.SAS_Address, &root->phy[pm->base.parent->id].dev_sas_addr.value, 8);
				}
			} else if (dev->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
#if defined(SUPPORT_I2C_SES)
				if (dev->enclosure !=NULL) {
					hd->Link.Self.EnclosureID = dev->enclosure->base.id;
					hd->Link.Parent.EnclosureID = dev->enclosure->base.id;
				}
#endif
				hd->Link.Parent.DevID = 0;						/* Utility expect the HBA ID. */
				hd->Link.Parent.DevType = DEVICE_TYPE_PORT;

				count = 0;
				root = dev->base.root;
				for (i = 0; i < root->phy_num; i++) {
					if (dev->base.port->phy_map & MV_BIT(i)) {
						if ((count == 0) && IS_SSP(dev)) {
							MV_CopyMemory(hd->Link.Parent.SAS_Address, &root->phy[i].dev_sas_addr.value, 8);
							MV_CopyMemory(hd->Link.Self.SAS_Address, &root->phy[i].att_dev_sas_addr.value, 8);
						} else if (count == 0) {
							MV_CopyMemory(hd->Link.Parent.SAS_Address, &root->phy[i].dev_sas_addr.value, 8);
						}
						MV_DASSERT(count < MAX_WIDEPORT_PHYS);
						hd->Link.Parent.PhyID[count++] = i+(MV_U8)root->base_phy_num;	/* Not pDevice->pPort->Id */
					}
				}
				hd->Link.Parent.PhyCnt = count;
			}

		}

		hd->InitStatus = PD_INIT_STATUS_OK;
		if (IS_STP_OR_SATA(dev)) {
			hd->HDType = HD_TYPE_SATA;
			if (IS_ATAPI(dev))
				hd->HDType |= HD_TYPE_ATAPI;
		} else {
			hd->HDType = HD_TYPE_SAS;
			/* Only change for SAS TAPE case right now. */
			if (IS_TAPE(dev)) {
				hd->HDType |= HD_TYPE_TAPE;
			}
			if (IS_ENCLOSURE(dev)) {
				hd->HDType |= HD_TYPE_SES;
			}
		}

		mv_swap_bytes(hd->Link.Parent.SAS_Address, 8);
		mv_swap_bytes(hd->Link.Self.SAS_Address, 8);

		hd->PIOMode = dev->pio_mode;
		hd->UDMAMode = dev->udma_mode;
		hd->CurrentPIOMode = dev->current_pio;
		hd->CurrentUDMAMode = dev->current_udma;
		hd->FeatureSupport = 0;
		if (dev->capability& DEVICE_CAPABILITY_NCQ_SUPPORTED)
			hd->FeatureSupport |= HD_FEATURE_NCQ;
		if (dev->capability & DEVICE_CAPABILITY_WRITECACHE_SUPPORTED)
			hd->FeatureSupport |= HD_FEATURE_WRITE_CACHE;
		if (dev->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
			hd->FeatureSupport |= HD_FEATURE_48BITS;
		if (dev->capability & DEVICE_CAPABILITY_SMART_SUPPORTED)
			hd->FeatureSupport |= HD_FEATURE_SMART;

		if (dev->capability & DEVICE_CAPABILITY_RATE_1_5G)
			hd->FeatureSupport |= HD_FEATURE_1_5G;
		if (dev->capability & DEVICE_CAPABILITY_RATE_3G)
			hd->FeatureSupport |= HD_FEATURE_3G;
		if (dev->capability & DEVICE_CAPABILITY_RATE_6G)
			hd->FeatureSupport |= HD_FEATURE_6G;
		if (dev->capability & DEVICE_CAPABILITY_RATE_12G)
			hd->FeatureSupport |= HD_FEATURE_12G;

#ifndef _OS_BIOS
		hd->HD_SSD_Type = HD_SSD_TYPE_NOT_SSD;
		if (dev->capability & DEVICE_CAPABILITY_SSD)
			hd->HD_SSD_Type = HD_SSD_TYPE_UNKNOWN_SSD;
#endif

		MV_CopyMemory(hd->Model, dev->model_number, 40);
		MV_CopyMemory(hd->SerialNo, dev->serial_number, 20);
		MV_CopyMemory(hd->FWVersion, dev->firmware_revision, 8);

		if ((hd->HDType == HD_TYPE_SATA) ||
			(hd->HDType == HD_TYPE_SAS))
			MV_CopyMemory(hd->WWN, &dev->WWN, 8);

		/* Set PD Sector Size */
		hd->BlockSize = dev->logical_sector_size;
		if (U64_COMPARE_U32(dev->max_lba, 0) != 0) {
			hd->Size = U64_ADD_U32(dev->max_lba, 1);
		} else {
			hd->Size.value = 0;
		}
	}else if((dev->base.type == BASE_TYPE_DOMAIN_I2C)||
		(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE)){
		enc = (domain_enclosure *)dev;
	if ( !(enc->status&ENCLOSURE_STATUS_FUNCTIONAL) ||
		 (enc->state != ENCLOSURE_INIT_DONE)) {
		hd->Link.Self.DevType = DEVICE_TYPE_NONE;
		return;
	}

	hd->Link.Self.DevType = DEVICE_TYPE_ENCLOSURE;
	hd->Link.Self.EnclosureID = 0xFFFF;
	hd->Link.Parent.EnclosureID = 0xFFFF;

	hd->Link.Self.PhyID[0] = 0;
	hd->Link.Self.PhyCnt = 1;
	hd->DeviceType = DT_ENCLOSURE;

	if (dev->base.parent != NULL) {
		if(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
			hd->Link.Parent.DevID = dev->base.parent->id;
			hd->Link.Parent.DevType = DEVICE_TYPE_EXPANDER;
			hd->Link.Parent.PhyID[0] = dev->parent_phy_id;
			hd->Link.Parent.PhyCnt = 1;
			hd->Link.Parent.EnclosureID = enc->base.id;

			MV_CopyMemory( hd->Link.Parent.SAS_Address, &((domain_expander *)enc->base.parent)->sas_addr.value, 8 );
		} else if(dev->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
			hd->Link.Self.EnclosureID = enc->base.id;
			hd->Link.Parent.EnclosureID =enc->base.id;
			hd->Link.Parent.DevID = 0;						/* Utility expect the HBA ID. */
			hd->Link.Parent.DevType = DEVICE_TYPE_PORT;
			hd->Link.Parent.PhyCnt = 0;
		}
	}

	hd->InitStatus = PD_INIT_STATUS_OK;
	hd->HDType = HD_TYPE_SAS |HD_TYPE_SES;
	mv_swap_bytes(hd->Link.Parent.SAS_Address, 8);
	mv_swap_bytes(hd->Link.Self.SAS_Address, 8);
	MV_CopyMemory(hd->Model, enc->vendor_id, 8);
	MV_CopyMemory(hd->Model+8, enc->product_id, 16);
	MV_CopyMemory(hd->FWVersion, enc->product_revision, 4);

	/* Set PD Sector Size */
	hd->BlockSize = 0;
	hd->Size.value = 0;

	}
}

MV_VOID
Core_GetHDInfo(
	IN MV_PVOID extension,
	IN MV_U16 hd_id,
	OUT MV_PVOID buffer,
	IN MV_BOOLEAN useVariableSize
	)
{
	core_extension *core = (core_extension *)extension;
	MV_U16 start_id = 0, end_id = 0;
	domain_device *dev = NULL;
	PHD_Info_Request hd_req;
	PHD_Info hd_info;
	PRequestHeader header = NULL;
	MV_U16 i;

	if (!useVariableSize) {
		if (hd_id == 0xFF) {
			start_id = 0;
			end_id = MAX_ID;
		} else {
			start_id = hd_id;
			end_id = hd_id;
		}
		hd_info = (PHD_Info)buffer;
	} else {
		hd_req = (PHD_Info_Request)buffer;
		header = &hd_req->header;
		if (header->requestType == REQUEST_BY_RANGE) {
			start_id = header->startingIndexOrId;
			end_id = MAX_ID;
		} else if (header->requestType == REQUEST_BY_ID) {
			start_id = header->startingIndexOrId;
			end_id = header->startingIndexOrId;
		}
		hd_info = hd_req->hdInfo;
		header->numReturned = 0;
		header->nextStartingIndex = NO_MORE_DATA;
	}

	for (i = start_id; i <= end_id; i++) {
		dev = (domain_device *)get_device_by_id(&core->lib_dev, i, MV_FALSE, MV_FALSE);
		if ((dev != NULL) &&
			((dev->base.type == BASE_TYPE_DOMAIN_DEVICE)||
			(dev->base.type == BASE_TYPE_DOMAIN_I2C)||
			(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE))) {
			core_get_hd_information(core, dev, hd_info);
		} else {
			hd_info->Link.Self.DevID = i;
			hd_info->Link.Self.DevType = DEVICE_TYPE_NONE;
		}

		if (!useVariableSize) {
			hd_info++;
		} else {
			if (hd_info->Link.Self.DevType != DEVICE_TYPE_NONE) {
				header->numReturned++;
				hd_info++;
				if ((header->requestType == REQUEST_BY_RANGE) &&
					(header->numReturned == header->numRequested)){
					header->nextStartingIndex = i+1;
					break;
				}
			}
		}
	}
}



MV_U8
core_pd_request_get_hd_info(core_extension * core_p, PMV_Request req)
{
	MV_U16 hd_id = 0xFFFF;
        MV_PVOID buf_ptr = core_map_data_buffer(req);
	PHD_Info_Request hd_req = (PHD_Info_Request)buf_ptr;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(HD_Info_Request)) {
		
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	Core_GetHDInfo(core_p, hd_id, buf_ptr, MV_TRUE);

	if (hd_req->header.requestType == REQUEST_BY_ID &&
		hd_req->header.numReturned == 0) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	} else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

        core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_VOID
core_get_hd_information_ext(core_extension * core_p, domain_device *dev,
	PHD_Info hd)
{
	MV_U8 i;
	MV_U8 count;
	pl_root * root;
	domain_enclosure *enc = NULL;

	hd->Link.Self.DevID = dev->base.id;
	hd->Link.Self.EnclosureID = 0xFFFF;
	hd->Link.Parent.EnclosureID = 0xFFFF;
	if(dev->base.type == BASE_TYPE_DOMAIN_DEVICE){
	if ( !(dev->status&DEVICE_STATUS_FUNCTIONAL) ||
		 (dev->status & DEVICE_STATUS_SECONDARY_PATH) ||
		 (dev->state != DEVICE_STATE_INIT_DONE))
	{
		hd->Link.Self.DevType = DEVICE_TYPE_NONE;
		return;
	}

	if (IS_ENCLOSURE(dev)) {
		/* Virtual enclosure device, do not report */
		hd->Link.Self.DevType = DEVICE_TYPE_ENCLOSURE;
	}

	// TBD: check if device type is correct; if not, generate sense
	switch( dev->dev_type ) {
	case DT_DIRECT_ACCESS_BLOCK:
		hd->Link.Self.DevType = DEVICE_TYPE_HD;
		break;
	case DT_SEQ_ACCESS:
		hd->Link.Self.DevType = DEVICE_TYPE_TAPE;
		break;
	case DT_CD_DVD:
		hd->Link.Self.DevType = DEVICE_TYPE_CD_DVD;
		break;
	case DT_PRINTER:
		hd->Link.Self.DevType = DEVICE_TYPE_PRINTER;
		break;
	case DT_PROCESSOR:
		hd->Link.Self.DevType = DEVICE_TYPE_PROCESSOR;
		break;
	case DT_WRITE_ONCE:
		hd->Link.Self.DevType = DEVICE_TYPE_WRITE_ONCE;
		break;
	case DT_OPTICAL_MEMORY:
		hd->Link.Self.DevType = DEVICE_TYPE_OPTICAL_MEMORY;
		break;
	case DT_MEDIA_CHANGER:
		hd->Link.Self.DevType = DEVICE_TYPE_MEDIA_CHANGER;
		break;
	default:
		hd->Link.Self.DevType = DEVICE_TYPE_HD;
	}

	hd->Link.Self.PhyID[0] = 0;
	hd->Link.Self.PhyCnt = 1;

	if (dev->base.parent != NULL) {
		if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
			hd->Link.Parent.DevID = dev->base.parent->id;
			hd->Link.Parent.DevType = DEVICE_TYPE_EXPANDER;
			hd->Link.Parent.PhyID[0] = dev->parent_phy_id;
			hd->Link.Parent.PhyCnt = 1;
#ifdef SUPPORT_SES
			if (((domain_expander *)(dev->base.parent))->enclosure) {
				hd->Link.Parent.EnclosureID = ((domain_expander *)(dev->base.parent))->enclosure->base.id;
			}
#else
			hd->Link.Parent.EnclosureID = 0xFFFF;
#endif
			MV_CopyMemory( hd->Link.Parent.SAS_Address, &((domain_expander *)dev->base.parent)->sas_addr.value, 8 );
			MV_CopyMemory( hd->Link.Self.SAS_Address, &dev->sas_addr.value, 8 );
		} else if(dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
			hd->Link.Parent.DevID = dev->base.parent->id;  /* PM ID */
			hd->Link.Parent.DevType = DEVICE_TYPE_PM;
			hd->Link.Parent.PhyID[0] = dev->pm_port;
			hd->Link.Parent.PhyCnt = 1;
		} else if(dev->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
#if defined(SUPPORT_I2C_SES)
			if(dev->enclosure !=NULL){
				hd->Link.Self.EnclosureID = dev->enclosure->base.id;
				hd->Link.Parent.EnclosureID = dev->enclosure->base.id;
			}
#endif
			hd->Link.Parent.DevID = 0;						/* Utility expect the HBA ID. */
			hd->Link.Parent.DevType = DEVICE_TYPE_PORT;

			count = 0;
			root = dev->base.root;
			for (i = 0; i < root->phy_num; i++) {
				if (dev->base.port->phy_map & MV_BIT(i)) {
					if ((count == 0) && IS_SSP(dev)) {
						MV_CopyMemory(hd->Link.Parent.SAS_Address, &root->phy[i].dev_sas_addr, 8);
						MV_CopyMemory(hd->Link.Self.SAS_Address, &root->phy[i].att_dev_sas_addr, 8);
					}
					MV_DASSERT(count < MAX_WIDEPORT_PHYS);
					hd->Link.Parent.PhyID[count++] = i+(MV_U8)root->base_phy_num;	/* Not pDevice->pPort->Id */
				}
			}
			hd->Link.Parent.PhyCnt = count;
		}
	}

	hd->InitStatus = PD_INIT_STATUS_OK;

	hd->ConnectionType = dev->connection;
	hd->DeviceType = dev->dev_type;
	if (dev->dev_type == DT_SES_DEVICE)
		hd->DeviceType = DT_DIRECT_ACCESS_BLOCK;

	mv_swap_bytes(hd->Link.Parent.SAS_Address, 8);
	mv_swap_bytes(hd->Link.Self.SAS_Address, 8);

	hd->PIOMode = dev->pio_mode;
	hd->MDMAMode = 0;//not defined dev->m;
	hd->UDMAMode = dev->udma_mode;
	hd->CurrentPIOMode = dev->current_pio;
	hd->CurrentMDMAMode = 0;//not defined in dev dev->Current_MDMA;
	hd->CurrentUDMAMode = dev->current_udma;
#ifdef SUPPORT_SGPIO
	hd->ActivityLEDStatus = dev->sgpio_act_led_status;
	hd->LocateLEDStatus = dev->sgpio_locate_led_status;
	hd->ErrorLEDStatus = dev->sgpio_error_led_status;
#endif
#ifdef SUPPORT_SES
	hd->ElementIdx = dev->ses_overall_element_index;
	hd->SesDeviceType = dev->ses_element_type;
	if(dev->enclosure)
		hd->Link.Self.EnclosureID = dev->enclosure->base.id;
#endif
	hd->FeatureSupport = 0;
	if (dev->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED)
		hd->FeatureSupport |= HD_FEATURE_NCQ;
	if (dev->capability & DEVICE_CAPABILITY_WRITECACHE_SUPPORTED)
		hd->FeatureSupport |= HD_FEATURE_WRITE_CACHE;
	if (dev->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		hd->FeatureSupport |= HD_FEATURE_48BITS;
	if (dev->capability & DEVICE_CAPABILITY_SMART_SUPPORTED)
		hd->FeatureSupport |= HD_FEATURE_SMART;

	if (dev->capability & DEVICE_CAPABILITY_RATE_1_5G)
		hd->FeatureSupport |= HD_FEATURE_1_5G;
	if (dev->capability & DEVICE_CAPABILITY_RATE_3G)
		hd->FeatureSupport |= HD_FEATURE_3G;
	if (dev->capability & DEVICE_CAPABILITY_RATE_6G)
		hd->FeatureSupport |= HD_FEATURE_6G;
	if (dev->capability & DEVICE_CAPABILITY_RATE_12G)
		hd->FeatureSupport |= HD_FEATURE_12G;

#ifndef _OS_BIOS
	hd->HD_SSD_Type = HD_SSD_TYPE_NOT_SSD;
	if (dev->capability & DEVICE_CAPABILITY_SSD)
		hd->HD_SSD_Type = HD_SSD_TYPE_UNKNOWN_SSD;
#endif

	MV_CopyMemory(hd->Model, dev->model_number, 40);
	MV_CopyMemory(hd->SerialNo, dev->serial_number, 20);
	MV_CopyMemory(hd->FWVersion, dev->firmware_revision, 8);

	if ((IS_STP_OR_SATA(dev)) || IS_SSP(dev))
		MV_CopyMemory(hd->WWN, &dev->WWN, 8);

	hd->BlockSize = dev->logical_sector_size;
//	hd->Size = pDevice->Max_LBA;
	hd->Size = U64_ADD_U32(dev->max_lba, 1);

	if((DC_ATA & hd->ConnectionType) && (DC_SERIAL & hd->ConnectionType)){
		hd->sata_signature=dev->signature;
		}
	else
		hd->sata_signature=0;

	} else if((dev->base.type == BASE_TYPE_DOMAIN_I2C)||
		(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE)) {

        enc = (domain_enclosure *)dev;
	if ( !(enc->status&ENCLOSURE_STATUS_FUNCTIONAL) ||
		 (enc->state != ENCLOSURE_INIT_DONE)) {
		hd->Link.Self.DevType = DEVICE_TYPE_NONE;
		return;
	}

	// TBD: check if device type is correct; if not, generate sense
	hd->Link.Self.DevType = DEVICE_TYPE_ENCLOSURE;
	hd->Link.Self.EnclosureID = 0xFFFF;
	hd->Link.Parent.EnclosureID = 0xFFFF;

	hd->Link.Self.PhyID[0] = 0;
	hd->Link.Self.PhyCnt = 1;
	hd->DeviceType = DT_ENCLOSURE;

	if (dev->base.parent != NULL) {
		if(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
			hd->Link.Parent.DevID = dev->base.parent->id;
			hd->Link.Parent.DevType = DEVICE_TYPE_EXPANDER;
			hd->Link.Parent.PhyID[0] = dev->parent_phy_id;
			hd->Link.Parent.PhyCnt = 1;
			hd->Link.Parent.EnclosureID = enc->base.id;

			MV_CopyMemory( hd->Link.Parent.SAS_Address, &((domain_expander *)enc->base.parent)->sas_addr.value, 8 );
			//MV_CopyMemory( hd->Link.Self.SAS_Address, &dev->sas_addr.value, 8 );	TBD: only for expander enclosure
		} else if(dev->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
			hd->Link.Self.EnclosureID = enc->base.id;
			hd->Link.Parent.EnclosureID =enc->base.id;
			hd->Link.Parent.DevID = 0;						/* Utility expect the HBA ID. */
			hd->Link.Parent.DevType = DEVICE_TYPE_PORT;
			hd->Link.Parent.PhyCnt = 0;

                }

        }

	hd->InitStatus = PD_INIT_STATUS_OK;
	/*TBD: need verify*/
	//if(pDevice->InitErrorBitMap)
		//hd->InitStatus|=PD_INIT_STATUS_ERROR;
	hd->HDType = HD_TYPE_SAS |HD_TYPE_SES;

	mv_swap_bytes(hd->Link.Parent.SAS_Address, 8);
	mv_swap_bytes(hd->Link.Self.SAS_Address, 8);

	MV_CopyMemory(hd->Model, enc->vendor_id, 8);
	MV_CopyMemory(hd->Model+8, enc->product_id, 16);
	//MV_CopyMemory(hd->SerialNo, enc->u.expander_ses.product_revision, 4);
	MV_CopyMemory(hd->FWVersion, enc->product_revision, 4);

	/* Set PD Sector Size */
	hd->BlockSize = 0;
	hd->Size.value = 0;
        }
}

MV_VOID core_get_hd_info_ext(MV_PVOID extension, MV_U16 hd_id, MV_PVOID buffer)
{
	core_extension * core = (core_extension *)extension;
	MV_U16 start_id = 0, end_id = 0;
	domain_device *dev = NULL;
	PHD_Info_Request hd_req;
	PHD_Info hd_info;
	PRequestHeader header = NULL;
	MV_U16 i;

	hd_req = (PHD_Info_Request)buffer;
	header = &hd_req->header;
	if (header->requestType == REQUEST_BY_RANGE) {
		start_id = header->startingIndexOrId;
		end_id = MAX_ID;
	} else if (header->requestType == REQUEST_BY_ID) {
		start_id = header->startingIndexOrId;
		end_id = header->startingIndexOrId;
	}
	hd_info = hd_req->hdInfo;
	header->numReturned = 0;
	header->nextStartingIndex = NO_MORE_DATA;

	for (i = start_id; i <= end_id; i++) {
		dev = (domain_device *)get_device_by_id(&core->lib_dev, i, MV_FALSE, MV_FALSE);
		if ((dev != NULL) &&
			((dev->base.type == BASE_TYPE_DOMAIN_DEVICE)||
			(dev->base.type == BASE_TYPE_DOMAIN_I2C)||
			(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE))) {
			core_get_hd_information_ext(core, dev, hd_info );
		} else {
			hd_info->Link.Self.DevID = i;
			hd_info->Link.Self.DevType = DEVICE_TYPE_NONE;
		}

		if (hd_info->Link.Self.DevType != DEVICE_TYPE_NONE) {
			header->numReturned++;
			hd_info++;
			if ((header->requestType == REQUEST_BY_RANGE) &&
				(header->numReturned == header->numRequested)) {
				header->nextStartingIndex = i + 1;
				break;
			}
		}
	}
}

MV_U8 core_pd_request_get_hd_info_ext(core_extension * core_p, PMV_Request req)
{
	MV_U16 hd_id = 0xFFFF;
        MV_PVOID buf_ptr = core_map_data_buffer(req);
	PHD_Info_Request hd_req = (PHD_Info_Request)buf_ptr;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(HD_Info_Request)) {
		
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (hd_req->header.startingIndexOrId >= MAX_ID) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	core_get_hd_info_ext(core_p, hd_id, buf_ptr);

	if (hd_req->header.requestType == REQUEST_BY_ID &&
		hd_req->header.numReturned == 0) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	} else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

        core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_VOID core_get_hd_configuration(core_extension *core, domain_port *port,
	domain_device *device, PHD_Config hd)
{
	if (!(device->status & DEVICE_STATUS_FUNCTIONAL)) {
		hd->HDID = 0xFF;
		return;
	}

	hd->HDID = device->base.id ;

	if (device->setting & DEVICE_SETTING_WRITECACHE_ENABLED)
		hd->WriteCacheOn = MV_TRUE;
	else
		hd->WriteCacheOn = MV_FALSE;

	if (device->setting & DEVICE_SETTING_SMART_ENABLED)
		hd->SMARTOn = MV_TRUE;
	else
		hd->SMARTOn = MV_FALSE;

	if (device->negotiated_link_rate == PHY_LINKRATE_12)
		hd->DriveSpeed = HD_SPEED_12G;
	else if (device->negotiated_link_rate == PHY_LINKRATE_6)
		hd->DriveSpeed = HD_SPEED_6G;
	else if (device->negotiated_link_rate == PHY_LINKRATE_3)
		hd->DriveSpeed = HD_SPEED_3G;
	else
		hd->DriveSpeed = HD_SPEED_1_5G;
}

MV_VOID core_get_hd_config(MV_PVOID extension, MV_PVOID buffer)
{
	core_extension * core = (core_extension *)extension;
	MV_U16 start_id = 0, end_id = 0;
	domain_port *port = NULL;
	domain_device *device = NULL;
	domain_base *base = NULL;
	PHD_Config hd;
	PHD_Config_Request config_req;
	PRequestHeader header = NULL;
	MV_U16 i;

	config_req = (PHD_Config_Request)buffer;
	header = &config_req->header;
	if (header->requestType == REQUEST_BY_RANGE) {
		start_id = header->startingIndexOrId;
		end_id = MAX_ID;
	} else if (header->requestType == REQUEST_BY_ID) {
		start_id = header->startingIndexOrId;
		end_id = header->startingIndexOrId;
	}
	hd = config_req->hdConfig;
	header->numReturned = 0;
	header->nextStartingIndex = NO_MORE_DATA;

	for (i = start_id; i <= end_id; i++) {
		base = get_device_by_id(&core->lib_dev, i, MV_FALSE, MV_FALSE);
		if ((base != NULL) && (base->type == BASE_TYPE_DOMAIN_DEVICE)) {
			device = (domain_device *)base;
			port = device->base.port;
			core_get_hd_configuration( core, port, device, hd );
		} else
			hd->HDID = 0xFF;

		if (hd->HDID != 0xFF) {
			header->numReturned++;
			hd++;
			if ((header->requestType == REQUEST_BY_RANGE) &&
				(header->numReturned == header->numRequested)) {
				header->nextStartingIndex = i+1;
				break;
			}
		}
	}
}

MV_BOOLEAN core_pd_request_get_hd_config(core_extension * core_p, PMV_Request req)
{
        MV_PVOID buf_ptr = core_map_data_buffer(req);
	PHD_Config_Request config_req = (PHD_Config_Request)buf_ptr;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(HD_Config_Request)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (config_req->header.startingIndexOrId >= MAX_ID){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	core_get_hd_config(core_p, buf_ptr);

	if (config_req->header.requestType == REQUEST_BY_ID &&
		config_req->header.numReturned == 0){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_HD_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	} else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

        core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 core_pd_request_smart_return_status_sata_callback(MV_Request *req)
{
        PHD_SMART_Status data_buf = core_map_data_buffer(req);
        MV_U8 ret = MV_FALSE;
        if (data_buf != NULL && 
		req->Data_Transfer_Length >= sizeof(HD_SMART_Status))
        	ret = data_buf->SmartThresholdExceeded;
        core_unmap_data_buffer(req);

        return (ret);
}

MV_U8 core_pd_request_smart_return_status_ssp_callback(MV_Request *req)
{
        MV_U32 length;
        MV_PU8 data_buf = (MV_PU8)core_map_data_buffer(req);
        MV_U8 ret;

        if (data_buf[0] != INFORMATIONAL_EXCEPTIONS_LOG_PAGE) {
                req->Scsi_Status = REQ_STATUS_ERROR;
                core_unmap_data_buffer(req);
                return MV_FALSE;
        }

        length = (data_buf[2] << 8) + data_buf[3];
        if (length < 7 || data_buf[7] < 3) {
                req->Scsi_Status = REQ_STATUS_ERROR;
                core_unmap_data_buffer(req);
                return MV_FALSE;
        }

        ret = ssp_ata_parse_log_sense_threshold_exceed(data_buf,
                        req->Data_Transfer_Length);

        core_unmap_data_buffer(req);
        return ret;
}

MV_VOID core_pd_request_get_hd_status_callback(MV_PVOID root_p,
        MV_Request *req)
{
        pl_root *root = (pl_root *)root_p;
        core_extension *core = (core_extension *)root->core;
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, ID_IS_OS_TYPE(req));
        core_context *org_ctx;

	PHD_SMART_Status hd_status;
	PHD_SMART_Status_Request status_req = NULL;
        MV_Request *org_req;
        MV_PVOID buf_ptr = NULL;
        MV_U16 length;
        MV_U8 ret = MV_FALSE;

        org_req = req->Org_Req;
	req->Org_Req = NULL;
	MV_ASSERT(org_req != NULL);

	org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	MV_ASSERT(org_ctx != NULL);
	MV_ASSERT(org_ctx->type == CORE_CONTEXT_TYPE_API);
	
	if (org_req->Scsi_Status == REQ_STATUS_PENDING || 
		req->Scsi_Status != REQ_STATUS_SUCCESS)
		org_req->Scsi_Status = req->Scsi_Status;

        if (req->Scsi_Status != REQ_STATUS_SUCCESS) {

                length = MV_MIN(org_req->Sense_Info_Buffer_Length,
                                req->Sense_Info_Buffer_Length);

                if (org_req->Sense_Info_Buffer != NULL &&
                        req->Sense_Info_Buffer != NULL &&
                        length != 0)

                        MV_CopyMemory(org_req->Sense_Info_Buffer,
                                        req->Sense_Info_Buffer,
                                        length);

                switch (org_req->Cdb[4]) {
                case APICDB4_PD_SMART_RETURN_STATUS:
                        core_generate_event(core, EVT_ID_HD_SMART_POLLING_FAIL,
                                device->base.id, SEVERITY_WARNING, 0, NULL ,0);
                        break;

                default:
                        MV_DASSERT(MV_FALSE);
                        break;
                }

                return;
        }

        switch (org_req->Cdb[4]) {
        case APICDB4_PD_SMART_RETURN_STATUS:
                if (IS_STP_OR_SATA(device))
                        ret = core_pd_request_smart_return_status_sata_callback(
                                req);
                else if (IS_SSP(device))
                        ret = core_pd_request_smart_return_status_ssp_callback(
                                req);
                else
                        MV_DASSERT(MV_FALSE);

                if (ret == MV_TRUE)
                        core_generate_event(core,
                                EVT_ID_HD_SMART_THRESHOLD_OVER,
                                device->base.id, SEVERITY_WARNING,
                                0, NULL ,0);

                buf_ptr = core_map_data_buffer(org_req);
                status_req = (PHD_SMART_Status_Request)buf_ptr;
		hd_status = &status_req->hdSmartStatus[status_req->header.numReturned++];
		hd_status->HDID = device->base.id;
		hd_status->SmartThresholdExceeded = ret;
                break;

        default:
                MV_ASSERT(MV_FALSE);
                org_req->Scsi_Status = REQ_STATUS_ERROR;
                break;
        }

	org_ctx->u.api_req.remaining--;
	if (org_ctx->u.api_req.remaining == 0) {
		if (status_req) {
			if (status_req->header.requestType == REQUEST_BY_ID ||
				(status_req->header.requestType == REQUEST_BY_RANGE &&
				org_ctx->u.api_req.end == MAX_ID))
				status_req->header.nextStartingIndex = NO_MORE_DATA;
			else
				status_req->header.nextStartingIndex = org_ctx->u.api_req.end;
		}
		core_queue_completed_req(core, org_req);
	}
	if(buf_ptr)
		core_unmap_data_buffer(org_req);
}

MV_Request *core_pd_make_sata_hd_request(pl_root *root,
        domain_device *dev,
        MV_U8 cdb2,
        MV_ReqCompletion completion)
{
        MV_Request *new_req;

        new_req = get_intl_req_resource(root, 0);
        if (new_req == NULL)
                return NULL;

        new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
        new_req->Cdb[1] = CDB_CORE_MODULE;
        new_req->Cdb[2] = cdb2;

        new_req->Completion = completion;
        new_req->Device_Id = dev->base.id;
        return new_req;
}

MV_U8 core_pd_request_get_hd_status(core_extension * core_p, PMV_Request req)
{
	PHD_SMART_Status_Request status_req = NULL;
	domain_device *device;
	domain_base *base;
        pl_root *root;
	MV_U16 hd_id, start_id, end_id, count;
	//MV_U8 status;
        MV_PU8 sense_buf;
        MV_Request *new_req;
	core_context *ctx;
	MV_U16 numRequested;
        //status = REQ_STATUS_SUCCESS;
        sense_buf = (MV_PU8)req->Sense_Info_Buffer;
        new_req = NULL;
        status_req = (PHD_SMART_Status_Request)core_map_data_buffer(req);

	if (status_req == NULL ||
		req->Data_Transfer_Length < sizeof(HD_SMART_Status_Request)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (status_req->header.requestType == REQUEST_BY_RANGE) {
		start_id = status_req->header.startingIndexOrId;
		end_id = MAX_ID;
	} else if (status_req->header.requestType == REQUEST_BY_ID) {
		start_id = status_req->header.startingIndexOrId;
		end_id = start_id + 1;
	} else {
		core_unmap_data_buffer(req);
		if (sense_buf != NULL)
			sense_buf[0] = ERR_INVALID_PARAMETER;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	status_req->header.numReturned = 0;
	numRequested = status_req->header.numRequested;
	core_unmap_data_buffer(req);
	
	ctx = (core_context *)req->Context[MODULE_CORE];
	ctx->type = CORE_CONTEXT_TYPE_API;
	ctx->u.api_req.pointer = req->Cmd_Initiator;
	count = 0;

	for (hd_id = start_id; 
		count < numRequested && hd_id < end_id; 
		hd_id++) {
		
		base = get_device_by_id(&core_p->lib_dev, hd_id, MV_FALSE, MV_FALSE);
		if (base == NULL)
			continue;
		if (base->type != BASE_TYPE_DOMAIN_DEVICE)
			continue;

		device = (domain_device *)base;
		if (!(device->status & DEVICE_STATUS_FUNCTIONAL))
			continue;

		root = device->base.root;
		switch (req->Cdb[4]) {
		case APICDB4_PD_SMART_RETURN_STATUS:
			if (IS_STP_OR_SATA(device)) {
				new_req = core_pd_make_sata_hd_request(root, device,
					CDB_CORE_SMART_RETURN_STATUS,
					core_pd_request_get_hd_status_callback);

			} else if (IS_SSP(device)) {
				if (!(device->setting & DEVICE_SETTING_SMART_ENABLED))
					continue;

				new_req = sas_make_log_sense_req(device, 0x2F,
					core_pd_request_get_hd_status_callback);

			} else {
				new_req = NULL;
				//status = ERR_INVALID_HD_ID;
			}
			break;

		default:
			new_req = NULL;
			//status = ERR_INVALID_REQUEST;
			break;
		}

		if (new_req == NULL)
			continue;

		new_req->Org_Req = req;
		count++;
		core_append_request(root, new_req);
	}

	if (count == 0) {
		if (sense_buf != NULL)
			sense_buf[0] = ERR_INVALID_HD_ID;

		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	ctx->u.api_req.start = start_id;
	ctx->u.api_req.end = hd_id;
	ctx->u.api_req.remaining = count;
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

#if 0
MV_VOID core_api_send_mode_select_command(domain_device *dev,MV_Request *org_req);
MV_VOID core_api_modify_device_cache_mode_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req =req->Org_Req;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);

	org_req->Scsi_Status = req->Scsi_Status;
	if((req->Scsi_Status != SCSI_STATUS_GOOD)&&
		(req->Sense_Info_Buffer))
		MV_CopyMemory(org_req->Sense_Info_Buffer, req->Sense_Info_Buffer, org_req->Sense_Info_Buffer_Length);
	if((req->Scsi_Status == SCSI_STATUS_GOOD)&&
		(req->Cdb[0] == SCSI_CMD_MODE_SENSE_6)){	//send mode select command to modify cache mode
		core_api_send_mode_select_command(device, req);
		}
	else
		core_queue_completed_req(root->core, req->Org_Req);
}
MV_VOID core_api_send_mode_select_command(domain_device *dev,MV_Request *org_req)
{
	pl_root *root = dev->base.root;
	MV_U8 length =0x12+6;		//cache mode page leng (12)+ mode select 6 parameter header (4)+mode page header (2)
	MV_Request *req = get_intl_req_resource(root, length);
	MV_U8 *org_buf, new_buf;

	if(req == NULL){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}
        new_buf = core_map_data_buffer(req);
        org_buf = core_map_data_buffer(org_req);

	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MODE_SELECT_6;		//send mode SENSE command
	req->Cdb[1] = 0x10;		//PF =1,  SP=0
	req->Cdb[4] = length; //allocation length
	MV_ZeroMemory(new_buf, req->Data_Transfer_Length);
	MV_CopyMemory(new_buf, org_buf, org_req->Data_Transfer_Length);
	req->Org_Req =org_req->Org_Req;
	new_buf[0] = new_buf[2] = new_buf[3] = 0;
	new_buf[4] &=0x3F;  //page code
	if(((MV_Request *)org_req->Org_Req)->Cdb[4] ==APICDB4_PD_SET_WRITE_CACHE_ON){
		new_buf[4+new_buf[3]+2] |= MV_BIT(2);
	}else if(((MV_Request *)org_req->Org_Req)->Cdb[4] ==APICDB4_PD_SET_WRITE_CACHE_OFF){
		new_buf[4+new_buf[3]+2] &= ~(MV_BIT(2));
	}
	req->Cmd_Flag = 0;
	req->Completion = core_api_modify_device_cache_mode_callback;
        core_unmap_data_buffer(req);
        core_unmap_data_buffer(org_req);
	core_append_request(root, req);
}

MV_VOID core_api_modify_device_cache_mode(domain_device *dev,MV_Request *org_req,MV_U8 cache_mode)
{
	pl_root *root = dev->base.root;
	MV_U8 length =MAX_MODE_PAGE_LENGTH;		//cache mode page leng (12)+ mode select 6 parameter header (2)+mode page header (2)
	MV_Request *req = get_intl_req_resource(root, length);
	core_context *ctx;
	MV_U8 *buf;
/*first send MODE SENSE command to get cache page*/
	if(req == NULL){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}
	ctx = req->Context[MODULE_CORE];
	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MODE_SENSE_6;		//send mode SENSE command
	req->Cdb[1] = 0x08;		//DBD =0
	req->Cdb[2] = CACHE_MODE_PAGE; // page code
	req->Cdb[4] = MAX_MODE_PAGE_LENGTH; //allocation length

        buf = core_map_data_buffer(req);
	MV_ZeroMemory(buf, req->Data_Transfer_Length);
        core_unmap_data_buffer(req);

	req->Org_Req =org_req;
	req->Completion = core_api_modify_device_cache_mode_callback;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	core_append_request(root, req);
}
#endif /* 0 */

MV_VOID
core_pd_request_set_hd_config_callback(MV_PVOID root_p, MV_Request *req)
{
        pl_root *root = (pl_root *)root_p;
        core_extension *core = (core_extension *)root->core;
        domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
			req->Device_Id, MV_FALSE, ID_IS_OS_TYPE(req));
        MV_Request *org_req;
        MV_U32 length;

        org_req = sas_clear_org_req(req);
        if (org_req == NULL) {
                MV_DASSERT(MV_FALSE);
                return;
        }

        org_req->Scsi_Status = req->Scsi_Status;
        if (req->Scsi_Status != REQ_STATUS_SUCCESS) {

                length = MV_MIN(req->Sense_Info_Buffer_Length,
                                org_req->Sense_Info_Buffer_Length);

                if (req->Sense_Info_Buffer != NULL &&
                        org_req->Sense_Info_Buffer != NULL)

                        MV_CopyMemory(org_req->Sense_Info_Buffer,
                                        req->Sense_Info_Buffer,
                                        length);

                        core_queue_completed_req(core, org_req);
                        return;
        }

        switch (org_req->Cdb[4]) {
        /* SAS/SATA should have set the device settings already */
        case APICDB4_PD_SET_WRITE_CACHE_OFF:
                dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
                core_generate_event(core, EVT_ID_HD_CACHE_MODE_CHANGE,
                        dev->base.id, SEVERITY_INFO, 0, NULL ,0);
                break;
        case APICDB4_PD_SET_WRITE_CACHE_ON:
                dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
                core_generate_event(core, EVT_ID_HD_CACHE_MODE_CHANGE,
                        dev->base.id, SEVERITY_INFO, 0, NULL ,0);
                break;
        case APICDB4_PD_SET_SMART_OFF:
                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
                break;
	case APICDB4_PD_SET_SMART_ON:
                dev->setting |= DEVICE_SETTING_SMART_ENABLED;
                break;

        default:
                MV_DASSERT(MV_FALSE);
                org_req->Scsi_Status = REQ_STATUS_ERROR;
                break;
        }

        core_queue_completed_req(core, org_req);
}

MV_U8
core_pd_request_set_hd_config(core_extension * core_p, PMV_Request req)
{
	domain_device *device;
	domain_base *base;
        pl_root *root;
	MV_U16 hd_id;
	MV_U8 status;
        MV_PU8 sense_buf;
        MV_Request *new_req = NULL;

        status = REQ_STATUS_SUCCESS;
        sense_buf = req->Sense_Info_Buffer;
	hd_id = req->Cdb[2] | (req->Cdb[3] << 8); // targetID
	base = get_device_by_id(&core_p->lib_dev, hd_id, MV_FALSE, ID_IS_OS_TYPE(req));

        if (base == NULL) {
		if (sense_buf != NULL)
			sense_buf[0] = ERR_INVALID_HD_ID;

		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
        }

	if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
		if (sense_buf != NULL)
			sense_buf[0] = ERR_INVALID_HD_ID;

		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
        }

	device = (domain_device *)base;
        root = device->base.root;

	if (!(device->status & DEVICE_STATUS_FUNCTIONAL)) {
		if (sense_buf != NULL)
			sense_buf[0] = ERR_INVALID_HD_ID;

		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
        }

        switch (req->Cdb[4]) {
	case APICDB4_PD_SET_WRITE_CACHE_OFF:
		if (!(device->capability &
                        DEVICE_CAPABILITY_WRITECACHE_SUPPORTED)) {

			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

                if (IS_STP_OR_SATA(device) || IS_SSP(device)) {
                        new_req = core_pd_make_sata_hd_request(root,
                                device,
                                CDB_CORE_DISABLE_WRITE_CACHE,
                                core_pd_request_set_hd_config_callback);
                } else {
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
                        status = ERR_INVALID_HD_ID;
                }
		break;

	case APICDB4_PD_SET_WRITE_CACHE_ON:
		if (!(device->capability &
                        DEVICE_CAPABILITY_WRITECACHE_SUPPORTED)) {
			status = ERR_NOT_SUPPORTED;

		} else if (IS_STP_OR_SATA(device) || IS_SSP(device)) {
                        new_req = core_pd_make_sata_hd_request(root,
                                device,
                                CDB_CORE_ENABLE_WRITE_CACHE,
                                core_pd_request_set_hd_config_callback);
                } else {
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
                        status = ERR_INVALID_HD_ID;
                }
		break;

        case APICDB4_PD_SET_SMART_OFF:
                if (IS_STP_OR_SATA(device)) {
                        new_req = core_pd_make_sata_hd_request(root, device,
                                CDB_CORE_DISABLE_SMART,
                                core_pd_request_set_hd_config_callback);

                } else if (IS_SSP(device)) {
                        device->setting &= ~DEVICE_SETTING_SMART_ENABLED;
                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

                } else {
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
                        status = ERR_INVALID_HD_ID;
                }
                break;

	case APICDB4_PD_SET_SMART_ON:
                if (IS_STP_OR_SATA(device)) {
                        new_req = core_pd_make_sata_hd_request(root, device,
                                CDB_CORE_ENABLE_SMART,
                                core_pd_request_set_hd_config_callback);

                } else if (IS_SSP(device)) {
		        if (device->capability &
                                DEVICE_CAPABILITY_SMART_SUPPORTED) {

                                device->setting |= DEVICE_SETTING_SMART_ENABLED;
                        }

                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

                } else {
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
                        status = ERR_INVALID_HD_ID;
                }
		break;

	case APICDB4_PD_SET_SPEED_3G:
	case APICDB4_PD_SET_SPEED_1_5G:
		status = ERR_INVALID_PARAMETER;
		break;

	default:
		status = ERR_INVALID_REQUEST;
		break;
        }

        if (new_req != NULL)
                sas_replace_org_req(root, req, new_req);
        else if (status == REQ_STATUS_SUCCESS)
                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	if (status != REQ_STATUS_SUCCESS) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	} else {
		req->Scsi_Status = REQ_STATUS_PENDING;
		return MV_QUEUE_COMMAND_RESULT_REPLACED;	// need to access hardware.
	}
}
