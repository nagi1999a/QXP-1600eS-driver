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
#include "core_sata.h"
#include "core_internal.h"
#include "core_init.h"
#include "core_resource.h"
#include "core_exp.h"
#include "core_device.h"
#include "core_sat.h"
#include "core_util.h"
#include "core_manager.h"
#include "core_error.h"
#include "core_console.h"
#include "hba_exp.h"
#include "com_define.h"
#include "core_expander.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif

extern MV_Request *smp_make_report_phy_sata_req(domain_device *device,
	MV_PVOID callback);
extern MV_VOID pm_sig_time_out(domain_pm *pm, MV_PVOID tmp);
extern MV_VOID _pm_sig_time_out(domain_pm *pm, MV_PVOID tmp);
extern MV_VOID stp_req_disc_callback(MV_PVOID root_p, MV_Request *req);
extern MV_Request *stp_make_report_phy_sata_req(domain_device *dev, MV_PVOID callback);
extern MV_Request *stp_make_phy_reset_req(domain_device *device, MV_U8 operation,
	MV_PVOID callback);
extern MV_VOID smp_physical_req_callback(MV_PVOID root_p, MV_Request *req);
extern MV_VOID update_base_id(pl_root *root,domain_port *port,domain_device *dev,MV_U8 skip);
MV_U32 g_PI_ENABLE=0;


MV_VOID sata_ata2host_string(MV_U16 *source, MV_U16 *target,
	MV_U32 words_count)
{
	MV_U32 i;
	for (i=0; i < words_count; i++) {
		target[i] = (source[i] >> 8) | ((source[i] & 0xff) << 8);
		target[i] = MV_LE16_TO_CPU(target[i]);
	}
}

MV_U8 sata_entry_exist(List_Head *list, List_Head *entry)
{
	List_Head *ptr;

	for (ptr = list->next; ptr != list; ptr = ptr->next)
	if (ptr == entry)
	return MV_TRUE;

	return MV_FALSE;
}

/*
 * Create WWN for SATA device from IDENTIFY data based on the description
 * in SAS-2 spec. This can only be used on SATA disk which supports ATA8
 * or higher
 */
MV_U64 sata_create_wwn_ata8(ata_identify_data *identify_data)
{
	MV_U64	wwn;
	MV_U8	tmp_b;

	wwn.parts.low = wwn.parts.high = 0;
	/*
	 * byte0 [7:4] = identify word 108 [15:12],
	 * byte0 [3:0] = identify word 108 [11:8]
	 */
	tmp_b = (MV_U8)(identify_data->naa >> 8);
	wwn.parts.low = (MV_U32)tmp_b;
	/* byte1 = identify word 108 [7:0] */
	tmp_b = (MV_U8)(identify_data->naa);
	wwn.parts.low |= (MV_U32)((MV_U32)tmp_b << 8);
	/* byte2 = identify word 109 [15:8] */
	tmp_b = (MV_U8)(identify_data->unique_id1 >> 8);
	wwn.parts.low |= (MV_U32)((MV_U32)tmp_b << 16);
	/*
	 * byte3 [7:4] = identify word 109 [7:4],
	 * byte3 [3:0] = identify word 109 [3:0]
	 */
	tmp_b = (MV_U8)(identify_data->unique_id1);
	wwn.parts.low |= (MV_U32)((MV_U32)tmp_b << 24);
	/* byte4 = identify word 110 [15:8] */
	tmp_b = (MV_U8)(identify_data->unique_id2 >> 8);
	wwn.parts.high |= (MV_U32)tmp_b;
	/* byte5 = identify word 110 [7:0] */
	tmp_b = (MV_U8)(identify_data->unique_id2);
	wwn.parts.high |= (MV_U32)((MV_U32)tmp_b << 8);
	/* byte6 = identify word 111 [15:8] */
	tmp_b = (MV_U8)(identify_data->unique_id3 >> 8);
	wwn.parts.high |= (MV_U32)((MV_U32)tmp_b << 16);
	/* byte7 = identify word 111 [7:0] */
	tmp_b = (MV_U8)(identify_data->unique_id3);
	wwn.parts.high |= (MV_U32)((MV_U32)tmp_b << 24);
	return wwn;
}

/*
 * Create WWN for SATA device from serial number and model number in
 * IDENTIFY data. This is used on SATA disk which does not support ATA 8
 * or higher
 */
MV_U64 sata_create_wwn_old(ata_identify_data *identify_data)
{
	MV_U64 wwn;

	wwn.parts.low = wwn.parts.high = 0;
	/* Use CRC of serial number as lower part of WWN */
	wwn.parts.low = MV_CRC((MV_PU8)identify_data->serial_number, 20);
	/* Use CRC of model number as higher part of WWN */
	wwn.parts.high = MV_CRC((MV_PU8)identify_data->model_number, 40);
	return wwn;
}

MV_U64 sata_create_wwn(ata_identify_data *identify_data)
{
	/* Some kinds of disks claim they are following ATA8, */
	/* but they did not fill in valid NAA and Unique ID. */
	/* We then got wrong WWN for those kinds of disks. */
	/* To avoid it, right now, for all SATA disks, */
	/* use identify data to create WWN */
	return sata_create_wwn_old(identify_data);
}

static void swap_buf_le16(MV_PU16 buf, unsigned int buf_words)
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        unsigned int i;

        for (i = 0; i < buf_words; i++)
                buf[i] = MV_LE16_TO_CPU(buf[i]);
#endif
}
struct model_number{
        char            *model_number;
        unsigned char   size;
	int  (*func)(ata_identify_data *,domain_device *);

};
int dec_queue_depth_workaround(ata_identify_data *identify_data,domain_device *device){

	if(device->base.queue_depth > 1 ){
		device->base.queue_depth -= 1;
		}
	return 0;
}
int esata_workaround(ata_identify_data *identify_data,domain_device *device){
	if (device->capability | DEVICE_CAPABILITY_NCQ_SUPPORTED) {
			device->capability &= ~DEVICE_CAPABILITY_NCQ_SUPPORTED;
	}
	return 0;
}

void check_model_number(ata_identify_data *identify_data,domain_device *device,struct model_number *check_model,unsigned int num){
	unsigned int i,result;
	unsigned char *check=device->model_number;
	 for(i=0;i<num;i++){
                result=memcmp(check,check_model[i].model_number,check_model[i].size);
		if(result==0){
			check_model[i].func(identify_data,device);
			}
        }
}

MV_VOID sata_parse_identify_data(domain_device *device,
	ata_identify_data *identify_data)
{
	MV_U8 i;
	MV_U32 capability = 0x0;
	MV_U16 setting = 0x0;
	MV_U8 ssd_capability = 0x0;
	struct model_number check_model[]={
        	{"STT_FTD28GX25H",14,dec_queue_depth_workaround},
		{"eSATA SSD",9,esata_workaround},
	};
	unsigned int num=sizeof(check_model)/sizeof(struct model_number);

	swap_buf_le16((MV_PU16)identify_data, 256);

	if (identify_data->general_config & IDENTIFY_GENERAL_CONFIG_ATAPI )
		device->dev_type = (identify_data->general_config &
			IDENTIFY_CMD_PACKET_SET_MASK) >> 8;

	/* Get serial number, firmware revision and model number. */
	MV_CopyMemory(device->serial_number, identify_data->serial_number, 20);
	MV_CopyMemory(device->firmware_revision,
		identify_data->firmware_revision, 8);
	MV_CopyMemory(device->model_number, identify_data->model_number, 40);

	sata_ata2host_string((MV_U16 *)device->serial_number,
		(MV_U16 *)device->serial_number, 10);
	sata_ata2host_string((MV_U16 *)device->firmware_revision,
		(MV_U16 *)device->firmware_revision, 4);
	sata_ata2host_string((MV_U16 *)device->model_number,
		(MV_U16 *)device->model_number, 20);

	/* capacity: 48 bit LBA, smart, write cache and NCQ */

	if (identify_data->command_set_supported[1] & MV_BIT(10))
		capability |= DEVICE_CAPABILITY_48BIT_SUPPORTED;
	if (identify_data->command_set_supported[0] & MV_BIT(0)) {
		capability |= DEVICE_CAPABILITY_SMART_SUPPORTED;
		if (identify_data->command_set_enabled[0] & MV_BIT(0))
			setting |= DEVICE_SETTING_SMART_ENABLED;
		if (identify_data->command_set_supported_extension & MV_BIT(0))
			capability |= DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED;
	}
	if (identify_data->command_set_supported[0] & MV_BIT(5)) {
		capability |= DEVICE_CAPABILITY_WRITECACHE_SUPPORTED;
		if (identify_data->command_set_enabled[0] & MV_BIT(5))
			setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
	}
	if (identify_data->command_set_supported[0] & MV_BIT(6)
		&& !(device->dev_type & DT_CD_DVD)) {
		capability |= DEVICE_CAPABILITY_READ_LOOK_AHEAD_SUPPORTED;
		if (identify_data->command_set_enabled[0] & MV_BIT(6))
			setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
	}
	if (((identify_data->specific_config == 0x37c8
			|| identify_data->specific_config == 0x738c))
		|| ((identify_data->command_set_supported[1] & MV_BIT(5))
			&& (identify_data->command_set_supported[1] & MV_BIT(6)))){
		capability |= DEVICE_CAPABILITY_POIS_SUPPORTED;
	}
	if ((identify_data->specific_config == 0x37c8
			|| identify_data->specific_config == 0x738c)
		&& (!(identify_data->command_set_enabled[1] & MV_BIT(5))
			&& !(identify_data->command_set_enabled[1] & MV_BIT(6)))) {
		setting |= DEVICE_SETTING_POIS_ENABLED;
	} else
		setting &= ~DEVICE_SETTING_POIS_ENABLED;
	if (identify_data->sata_capabilities & MV_BIT(9)) {
		capability |= DEVICE_CAPABILITY_HIPM_SUPPORTED;
		CORE_DPRINT(("SATA dev support HIPM.\n"));
	}
	if (identify_data->sata_capabilities & MV_BIT(8)) {
		if (capability & DEVICE_CAPABILITY_48BIT_SUPPORTED) {
#ifndef ATHENA_DISABLE_NCQ
			capability |= DEVICE_CAPABILITY_NCQ_SUPPORTED;
#endif
			/* no longer true. some SSD devices support only 31 */
			// device->base.queue_depth = 32;
            //WORKAROUND_SILICONPOWER_ESATASSD_DISABLENCQ(device);
		}
	}
	if (identify_data->sata_feature_supported & MV_BIT(3)) {
		capability |= DEVICE_CAPABILITY_DIPM_SUPPORTED;
		if (identify_data->sata_feature_enabled & MV_BIT(3))
			setting |= DEVICE_SETTING_DIPM_ENABLED;
	}
	if (!IS_ATAPI(device)) {
		/* capacity: trim support*/
		if(identify_data->reserved7[8]&MV_BIT(0)){
				capability |= DEVICE_CAPABILITY_TRIM_SUPPORTED;
				if (identify_data->reserved2[0] & MV_BIT(14)) {
					ssd_capability |= SSD_CAPABILITY_TRIM_ANCHORED_LBA;

					if (identify_data->reserved2[0] & MV_BIT(5)) {
						ssd_capability |= SSD_CAPABILITY_TRIM_READ_ZERO;
					}
				}
		}

		/* capacity: non rotating media */
		if(identify_data->reserved8[11] == 0x0001){
				capability |= DEVICE_CAPABILITY_SSD;
		}
	}

	if (!(identify_data->reserved1 & MV_BIT(15))
		&& (identify_data->reserved1 & MV_BIT(14))
		&& (identify_data->reserved1 & MV_BIT(0))) {
		capability |= DEVICE_CAPABILITY_TRUST_SUPPORTED;
	}

	device->base.queue_depth = identify_data->queue_depth + 1;

	if(device->base.queue_depth > 32)
		device->base.queue_depth = 32;
	
	if (device->negotiated_link_rate == PHY_LINKRATE_12)
		capability |= DEVICE_CAPABILITY_RATE_12G;

	if (identify_data->sata_capabilities & MV_BIT(3))
		capability |= DEVICE_CAPABILITY_RATE_6G;
	/* minor workaround: sometimes 6G SATA drives do not report their
	 * 6G capability in identify. Set it manually */
	if (device->negotiated_link_rate == PHY_LINKRATE_6)
		capability |= DEVICE_CAPABILITY_RATE_6G;
	if (identify_data->sata_capabilities & MV_BIT(2))
		capability |= DEVICE_CAPABILITY_RATE_3G;
	if (identify_data->sata_capabilities & MV_BIT(1))
		capability |= DEVICE_CAPABILITY_RATE_1_5G;
	if (identify_data->command_set_supported_extension & MV_BIT(5)) {
		if (identify_data->command_set_default & MV_BIT(5))
			capability |= DEVICE_CAPABILITY_READLOGEXT_SUPPORTED;
	}

	/* Disk size */
	if (capability & DEVICE_CAPABILITY_48BIT_SUPPORTED) {
		device->max_lba.parts.low = *((MV_PU32)&identify_data->max_lba[0]);
		device->max_lba.parts.high = *((MV_PU32)&identify_data->max_lba[2]);
	} else {
		device->max_lba.parts.low =
			*((MV_PU32)&identify_data->user_addressable_sectors[0]);
		device->max_lba.parts.high = 0;
	}
	/*
	 * The disk size as indicated by the ATA spec is the total addressable
	 * sectors on the drive; while Max LBA is the LBA of the last logical
	 * block on the drive
	 */
	device->max_lba = U64_SUBTRACT_U32(device->max_lba, 1);

	/* PIO, MDMA and UDMA mode */
   	if ((identify_data->fields_valid & MV_BIT(1)) &&
		(identify_data->pio_modes & 0x0F)) {
       	if ((MV_U8)identify_data->pio_modes >= 0x2)
		  	device->pio_mode = 0x04;
		else
	  		device->pio_mode = 0x03;
	} else {
       	device->pio_mode = 0x02;
	}
	device->udma_mode = 0xFF;
	if (identify_data->fields_valid & MV_BIT(2)) {
		for (i=0; i<7; i++) {
			if (identify_data->udma_modes & MV_BIT(i))
				device->udma_mode = i;
		}
	}

	/* ikkwong: TBD */
	device->max_transfer_size = CORE_MAX_TRANSFER_SIZE;

	if (device->dev_type == DT_CD_DVD)
		device->sector_size = 4 * SECTOR_SIZE;
	else
		device->sector_size = SECTOR_SIZE;

	device->logical_sector_size = device->sector_size;

	/* check identify data word 106 */
	if ((identify_data->physical_logical_sector_size & 0xd000) == 0x5000)
		device->logical_sector_size = ((MV_U32)	(((MV_U32)identify_data->words_per_logical_sector[1]) << 16) |
				((MV_U32)identify_data->words_per_logical_sector[0])) * 2;

	if ((identify_data->physical_logical_sector_size & 0xe000) == 0x6000)
		device->sector_size = device->logical_sector_size*(1<<(identify_data->physical_logical_sector_size & 0xf));
	else
		device->sector_size = device->logical_sector_size;

	device->WWN = sata_create_wwn(identify_data);

	device->capability = capability;
	device->setting = setting;
	device->ssd_capability = ssd_capability;
	
	check_model_number(identify_data,device,check_model,num);
}

MV_VOID sata_port_intl_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_pm *pm = (domain_pm *)get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, MV_FALSE);
	domain_port *port = pm->base.port;
#ifdef _OS_LINUX
	if (port->sata_sig_timer != NO_CURRENT_TIMER) {
		core_cancel_timer(root->core, port->sata_sig_timer);
		port->sata_sig_timer = NO_CURRENT_TIMER;
	}
#endif
	CORE_DPRINT(("port %p phy %d state 0x%x req %p,port->sata_sig_timer=%x.\n", \
		port, (root->base_phy_num+port->phy->id), port->state, req,port->sata_sig_timer));

	switch(port->state) {
	case PORT_SATA_STATE_SOFT_RESET_1:
		core_sleep_millisecond(root->core, 50);
		port->phy->sata_signature = 0xFFFFFFFF;
		port->state = PORT_SATA_STATE_SOFT_RESET_0;
		break;
	case PORT_SATA_STATE_SOFT_RESET_0:
		port->state = PORT_SATA_STATE_WAIT_SIG;
		break;
	default:
		MV_DASSERT(MV_FALSE);
		return;
	}

	core_queue_init_entry(root, &port->base, MV_FALSE);
}

MV_VOID sata_device_intl_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *device = (domain_device *)get_device_by_id(
		root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	ata_identify_data *identify_data;
#ifdef SUPPORT_PHY_POWER_MODE
	domain_phy *phy = NULL;
	domain_port *port = NULL;
	core_extension *core = (core_extension *)root->core;
#endif
	if (req==NULL || device == NULL) {
		core_init_entry_done(root, NULL, NULL);
		CORE_EH_PRINT(("sata_device_intl_req_callback Req (%p) req->Scsi_Status : %x  device(%p)\n", req, req->Scsi_Status, device));
		return;
	}
	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, &device->base, req);
		return;
	}

	identify_data = (ata_identify_data *)core_map_data_buffer(req);

	switch (req->Cdb[0]) {
	case SCSI_CMD_INQUIRY:
		MV_ASSERT(device->state == DEVICE_STATE_RESET_DONE);
		if (IS_ATAPI(device)) {
			sata_parse_identify_data(device, identify_data);
		}
		device->state = DEVICE_STATE_IDENTIFY_DONE;
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		switch (req->Cdb[2]) {
		case CDB_CORE_SET_PIO_MODE:
#ifdef SUPPORT_PHY_POWER_MODE
			MV_DASSERT(device->state == DEVICE_STATE_ENABLE_DIPM_DONE);
#else
			MV_DASSERT(device->state == DEVICE_STATE_SPIN_UP_DONE);
#endif
			device->state = DEVICE_STATE_SET_PIO_DONE;
			break;
		case CDB_CORE_SET_UDMA_MODE:
			MV_DASSERT(device->state == DEVICE_STATE_SET_PIO_DONE);
			device->state = DEVICE_STATE_SET_UDMA_DONE;
			break;
		case CDB_CORE_ENABLE_WRITE_CACHE:
			MV_DASSERT(device->state == DEVICE_STATE_SET_UDMA_DONE);
			device->state = DEVICE_STATE_ENABLE_WRITE_CACHE_DONE;
			device->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
			break;
		case CDB_CORE_ENABLE_READ_AHEAD:
			MV_DASSERT(device->state ==
			        DEVICE_STATE_ENABLE_WRITE_CACHE_DONE);
			device->state = DEVICE_STATE_ENABLE_READ_AHEAD_DONE;
			break;
#ifdef SUPPORT_PHY_POWER_MODE
		case CDB_CORE_ATA_ENABLE_DIPM:
			MV_DASSERT(device->state == DEVICE_STATE_SPIN_UP_DONE);
			port = device->base.port;
			phy = port->phy;
			device->setting |= DEVICE_SETTING_DIPM_ENABLED;
			//Core_Disable_PHY_Interrupt(phy);
			core->PHY_power_mode_port_enabled |= (1<<(root->base_phy_num + phy->asic_id));//record the device enabled DIPM or HIPM!
			Core_Enable_Receive_PHY_Power_Request(phy);
			device->state = DEVICE_STATE_ENABLE_DIPM_DONE;
			break;
#endif
		case CDB_CORE_SET_FEATURE_SPINUP:
			MV_DASSERT(device->state ==
			        DEVICE_STATE_IDENTIFY_DONE);
			/* The last identify data is wrong, */
			/* We have to issue identify again to get corrected data */
			//device->state = DEVICE_STATE_SPIN_UP_DONE;
			device->state = DEVICE_STATE_RESET_DONE;
			break;
		}
		break;
	}

	core_unmap_data_buffer(req);
	core_queue_init_entry(root, &device->base, MV_FALSE);
}



MV_Request *sata_make_soft_reset_req(domain_port *port, domain_device *device,
	MV_BOOLEAN srst, MV_BOOLEAN is_port_reset, MV_PVOID callback)
{
	pl_root *root = port->base.root;
	MV_Request *req =  get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = srst ? CDB_CORE_SOFT_RESET_1 : CDB_CORE_SOFT_RESET_0;
	req->Cdb[3] = is_port_reset ? 0xF : device->pm_port;
	if (device)
		req->Device_Id = device->base.id;
	else
		req->Device_Id = port->pm->base.id;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))callback;
	return req;
}

MV_Request *sata_make_identify_req(domain_device *device,
	MV_BOOLEAN EVPD, MV_U8 page)
{
	pl_root *root = device->base.root;
	/* make big buffer size for ATAPI device */
	MV_Request *req = get_intl_req_resource(root, 0x200);

	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_INQUIRY;
	if (EVPD) {
		req->Cdb[1] = 0x01;
		req->Cdb[2] = page;
	} else {
		req->Cdb[1] = 0;
	}

	/* buffer size */
	req->Cdb[3] = 0x2;
	req->Cdb[4] = 0x00;
	req->Device_Id = device->base.id;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))
		sata_device_intl_req_callback;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	return req;
}
#ifdef SUPPORT_PHY_POWER_MODE
MV_Request *sata_make_enable_dipm_req(domain_device *device,MV_U8 enable)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE; /* disable block descriptors */
	if (enable)
		req->Cdb[2] = CDB_CORE_ATA_ENABLE_DIPM;
	else
		req->Cdb[2] = CDB_CORE_ATA_DISABLE_DIPM;

	req->Device_Id = device->base.id;
	req->Completion = sata_device_intl_req_callback;
	return req;
}
#endif
MV_Request *sata_make_set_pio_mode_req(domain_device *device)
{
	pl_root *root = device->base.root;
	MV_Request *req;
	MV_U8 mode = device->pio_mode;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	/*
	 * For Blu-ray DVD-RW, have to downgrade PIO and UDMA mode.
	 * Otherwise DVD-Write will fail during data verification.
	 */
	if (IS_ATAPI(device)) {
		if (mode > 2)
			mode = device->pio_mode = 2;
	}

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SET_PIO_MODE;
	req->Cdb[3] = mode;
	req->Device_Id = device->base.id;
	req->Completion = (void(*)(MV_PVOID,MV_Request *))
		sata_device_intl_req_callback;
	device->current_pio = mode;

	return req;
}

MV_Request *sata_make_set_udma_mode_req(domain_device *device)
{
	pl_root *root = device->base.root;
	MV_Request *req;
	MV_U8 mode = device->udma_mode;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	/* For Blu-ray DVD-RW, have to downgrade PIO and UDMA mode.
	 * Otherwise DVD-Write will fail during data verification. */
	if (IS_ATAPI(device)) {
		if (mode > 4)
			mode = device->udma_mode = 4;
	}

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SET_UDMA_MODE;
	req->Cdb[3] = mode;
	req->Device_Id = device->base.id;
	req->Completion = (void(*)(MV_PVOID,PMV_Request))
		sata_device_intl_req_callback;
	device->current_udma = mode;
	return req;
}

MV_Request *sata_make_enable_write_cache_req(domain_device *device,
        MV_U8 enable)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE; /* disable block descriptors */
	if (enable)
		req->Cdb[2] = CDB_CORE_ENABLE_WRITE_CACHE;
	else
		req->Cdb[2] = CDB_CORE_DISABLE_WRITE_CACHE;

	req->Device_Id = device->base.id;
	req->Completion = sata_device_intl_req_callback;
	return req;
}

MV_Request *sata_make_enable_read_ahead_req(domain_device *device)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE; /* disable block descriptors */
	req->Cdb[2] = CDB_CORE_ENABLE_READ_AHEAD;

	req->Device_Id = device->base.id;
	req->Completion = sata_device_intl_req_callback;
	return req;
}

MV_Request *sata_make_set_feature_spinup_req(domain_device *device)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE; /* disable block descriptors */
	req->Cdb[2] = CDB_CORE_SET_FEATURE_SPINUP;

	req->Device_Id = device->base.id;
	req->Completion = sata_device_intl_req_callback;
	return req;
}

MV_Request *sata_make_start_stop_req(domain_device *device,
        MV_U8 start)
{
	pl_root *root = device->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_START_STOP_UNIT;
	req->Cdb[1] = 0;                       /* IMMED=0 */
	req->Cdb[2] = 0;
	req->Cdb[3] = 0;
	if (start == MV_TRUE)
		req->Cdb[4] = 1;
	else
		req->Cdb[4] = 0;

	req->Device_Id = device->base.id;
	req->Completion = sata_device_intl_req_callback;
	return req;
}

MV_VOID sata_set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device)
{
	set_up_new_device(root, port, device,
		(command_handler *)
		core_get_handler(root, HANDLER_SATA));

	device->state = DEVICE_STATE_IDLE;
	device->negotiated_link_rate = port->link_rate;

}

MV_VOID sata_port_process_new(domain_port *port)
{
	domain_phy *phy;
	pl_root *root = port->base.root;
	domain_device *device = NULL;
    core_extension *core = (core_extension *)root->core;
#ifdef SCSI_ID_MAP
    PHBA_Extension hba = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
#endif
#ifdef SUPPORT_STAGGERED_SPIN_UP
   //core_extension *core = (core_extension *)root->core;
	MV_ULONG flags;
	struct device_spin_up *tmp=NULL;
#endif

	phy = port->phy;
	if (phy->sata_signature == 0x96690101) {
		MV_DASSERT(port->pm != NULL);
		port->pm->base.struct_size = sizeof(domain_pm);
#ifdef SCSI_ID_MAP
        if (core->state != CORE_STATE_STARTED) {
            hba->have_pm = MV_TRUE;
        }
#endif
		core_queue_init_entry(root, &port->pm->base, MV_TRUE);
	} else {
		/* release the PM that was obtained during port soft reset */
		free_pm_obj(root, root->lib_rsrc, port->pm);
		port->pm = NULL;

		/* get a new device */
		device = get_device_obj(root, root->lib_rsrc);
		if (device == NULL) {
			CORE_DPRINT(("no more free device\n"));
			return;
		}
		sata_set_up_new_device(root, port, device);
		update_base_id(root,port,device,MV_FALSE);
		device->signature = port->phy->sata_signature;
		if (device->signature == 0xEB140101) {
			device->connection = DC_SERIAL | DC_ATA | DC_ATAPI;
		} else {
			device->connection = DC_SERIAL | DC_ATA;
		}

        if (core->enable_sgpio)
    		device->connection |= DC_SGPIO;
        else
            device->connection &= ~DC_SGPIO;
        
#ifdef SUPPORT_SGPIO
		device->sgpio_drive_number = (MV_U8)port->base.id;
#ifdef SUPPORT_SGPIO_ACTIVE_LED
		device->active_led_off_timer = NO_CURRENT_TIMER;
#endif
#endif
		device->dev_type = DT_DIRECT_ACCESS_BLOCK;
		device->base.parent = &port->base;
#ifdef SKIP_INTERNAL_INITIALIZE
		device->state = DEVICE_STATE_SKIP_INIT_DONE;
#else
		device->state = DEVICE_STATE_RESET_DONE;
#endif

		List_AddTail(&device->base.queue_pointer, &port->device_list);
		port->device_count++;
#ifndef SUPPORT_STAGGERED_SPIN_UP
		core_queue_init_entry(root, &device->base, MV_TRUE);
#else
		tmp=get_spin_up_device_buf(root->lib_rsrc);
		if(!tmp || (core->spin_up_group == 0)){
			if(tmp){
				free_spin_up_device_buf(root->lib_rsrc,tmp);
			}
			core_queue_init_entry(root, &device->base, MV_TRUE);
		}else{
			MV_LIST_HEAD_INIT(&tmp->list);
			tmp->roots=root;
			tmp->base=&device->base;

			OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
			List_AddTail(&tmp->list,&core->device_spin_up_list);
			if(core->device_spin_up_timer ==NO_CURRENT_TIMER){
				core->device_spin_up_timer=core_add_timer(core, 3, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, &device->base, NULL);
			}
			OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
		}

#endif
	}
}

/* no lock */
MV_VOID _sata_sig_time_out(domain_port *port, MV_PVOID tmp)
{
	pl_root *root = port->base.root;
	CORE_DPRINT(("port %p\n", port));
	/*
	 * Signature FIS did not return. We proceed with initialization anyway
	 * (sometimes the drive can still recover. If not, we will deal with
	 * it during time out handling)
	 */
	if (port->state == PORT_SATA_STATE_POWER_ON) {
		port->state = PORT_SATA_STATE_SOFT_RESET_1;
	} else if (port->state == PORT_SATA_STATE_WAIT_SIG) {
		port->state = PORT_SATA_STATE_SIG_DONE;
	} else {
		MV_DASSERT(MV_FALSE);
	}

	port->sata_sig_timer = NO_CURRENT_TIMER;
	core_queue_init_entry(root, &port->base, MV_FALSE);
}

MV_VOID sata_sig_time_out(domain_port *port, MV_PVOID tmp)
{
	MV_ULONG flags;
	core_extension *core = port->base.root->core;
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(port->base.root->core, &flags);
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
	_sata_sig_time_out(port, tmp);
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(port->base.root->core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
}

MV_BOOLEAN sata_port_state_machine(MV_PVOID dev)
{
	domain_port *port = (domain_port *)dev;
	pl_root *root = port->base.root;
	MV_Request *req = NULL;
	core_extension *core = (core_extension *)root->core;

	CORE_DPRINT(("port %p phy %d state 0x%x,port->sata_sig_timer=%x, port->phy->sata_signature=%x.\n", port, (root->base_phy_num+port->phy->id), port->state,port->sata_sig_timer,port->phy->sata_signature));

	switch (port->state) {
	case PORT_SATA_STATE_POWER_ON:
		if (port->phy->sata_signature == 0xFFFFFFFF) {
#ifdef SUPPORT_QUICK_PM_INIT
			port->state = PORT_SATA_STATE_SOFT_RESET_1;
#else
			MV_ASSERT(port->sata_sig_timer == NO_CURRENT_TIMER);
			port->sata_sig_timer = core_add_timer(
				root->core, 12,
				(void(*)(void *, void *))sata_sig_time_out, port, NULL);
			MV_ASSERT(port->sata_sig_timer != NO_CURRENT_TIMER);
			CORE_DPRINT(("port %p PORT_SATA_STATE_POWER_ON generate timer %d.\n", \
				port, port->sata_sig_timer));
			return MV_TRUE;
#endif
		} else {
			port->state = PORT_SATA_STATE_SOFT_RESET_1;
			/* no break, go directly to next case */
		}
	case PORT_SATA_STATE_SOFT_RESET_1:
		req = sata_make_soft_reset_req(port, NULL, MV_TRUE, MV_TRUE,
			sata_port_intl_req_callback);
		break;
	case PORT_SATA_STATE_SOFT_RESET_0:
		req = sata_make_soft_reset_req(port, NULL, MV_FALSE, MV_TRUE,
			sata_port_intl_req_callback);
		break;
	case PORT_SATA_STATE_WAIT_SIG:
		if (port->phy->sata_signature == 0xFFFFFFFF) {
			MV_ASSERT(port->sata_sig_timer == NO_CURRENT_TIMER);

			/* the time of receiving signature FIS may be need 60s for some WD green drives */
			if(core->is_dump)	// quick init in hibernation driver
				port->sata_sig_timer = core_add_timer(
						root->core, 10,
						(void (*) (void *, void *))sata_sig_time_out, port, NULL);
			else
				port->sata_sig_timer = core_add_timer(
						root->core, 60,
						(void (*) (void *, void *))sata_sig_time_out, port, NULL);
			MV_ASSERT(port->sata_sig_timer != NO_CURRENT_TIMER);
			CORE_DPRINT(("port %p PORT_SATA_STATE_WAIT_SIG generate timer %d.\n", \
				port, port->sata_sig_timer));
			return MV_TRUE;
		} else {
			port->state = PORT_SATA_STATE_SIG_DONE;
			/* no break, go directly to next case */
		}
	case PORT_SATA_STATE_SIG_DONE:
		/*
		 * this port state machine is done, next step is to detect device,
		 * and either go into device or PM state machine
		 */
		sata_port_process_new(port);
		/*
		 * finish port init entry after queuing device init entries -
		 * this is necessary to ensure event handling is pushed
		 * properly
		 */
		core_init_entry_done(root, port, &port->base);
		return MV_TRUE;
	}

	if (req) {
		core_append_init_request(root, req);
		return MV_TRUE;
	} else {
		/* req is invalid, out of resource */
		return MV_FALSE;
	}
}

MV_BOOLEAN sata_device_state_machine(MV_PVOID dev)
{
	domain_device *device = (domain_device *)dev;
	pl_root *root = device->base.root;
	MV_Request *req = NULL;
#ifdef SUPPORT_PHY_POWER_MODE
	core_extension *core = (core_extension *)root->core;
	domain_port *port = NULL;
	domain_phy *phy = NULL;
#endif
	CORE_DPRINT(("dev %p(%d) state 0x%x.\n", dev, device->base.id, device->state));

	switch (device->state) {
	case DEVICE_STATE_STP_RESET_PHY:
		req = stp_make_phy_reset_req(device, HARD_RESET,
			stp_req_disc_callback);
		break;
	case DEVICE_STATE_STP_REPORT_PHY:
		req = stp_make_report_phy_sata_req(device,
			stp_req_disc_callback);
		break;
	case DEVICE_STATE_IDLE: // when hotplug with pm, plugin and  plugout quickly, the device state is idle
		if((device->pm)&&(device->state==DEVICE_STATE_IDLE)){
			device->state=DEVICE_STATE_RESET_DONE;
		}
		break;
	case DEVICE_STATE_RESET_DONE:
		req = sata_make_identify_req(device,
			MV_FALSE, 0x00);
		break;
	case DEVICE_STATE_IDENTIFY_DONE:
		if ((device->capability & DEVICE_CAPABILITY_POIS_SUPPORTED)
			&& (device->setting & DEVICE_SETTING_POIS_ENABLED)) {
			req = sata_make_set_feature_spinup_req(device);
			break;
		}
		device->state=DEVICE_STATE_SPIN_UP_DONE;
	case DEVICE_STATE_SPIN_UP_DONE:
		if (core_check_duplicate_device(root, device) == MV_TRUE) {
			CORE_PRINT(("Duplicate SATA device. Set down\n"));
			core_init_entry_done(root, device->base.port, NULL);
            device->state = DEVICE_STATE_DUPLICATE_DEVICE;
			//pal_set_down_disk(root, device, MV_FALSE);
			return MV_TRUE;
		}
#ifdef SUPPORT_PHY_POWER_MODE
		if((device->capability & DEVICE_CAPABILITY_DIPM_SUPPORTED)
			&& !(device->setting & DEVICE_SETTING_DIPM_ENABLED)
			&& core->PHY_power_mode_DIPM
			&& (core->PHY_power_mode_port_map & (1 << (root->base_phy_num + device->base.port->phy->asic_id)))){
			port = device->base.port;
				if(!port->pm){
					req = sata_make_enable_dipm_req(device,MV_TRUE);
					break;
			}
		}
		device->state = DEVICE_STATE_ENABLE_DIPM_DONE;
	case DEVICE_STATE_ENABLE_DIPM_DONE:
		req = sata_make_set_pio_mode_req(device);
		break;
#else
		req = sata_make_set_pio_mode_req(device);
		break;
#endif
	case DEVICE_STATE_SET_PIO_DONE:
		req = sata_make_set_udma_mode_req(device);
		break;
	case DEVICE_STATE_SET_UDMA_DONE:
		if (device->capability & DEVICE_CAPABILITY_WRITECACHE_SUPPORTED) {
			req = sata_make_enable_write_cache_req(device, MV_TRUE);
			break;
		}else
			device->state = DEVICE_STATE_ENABLE_WRITE_CACHE_DONE;
	case DEVICE_STATE_ENABLE_WRITE_CACHE_DONE:
		if (device->capability & DEVICE_CAPABILITY_READ_LOOK_AHEAD_SUPPORTED) {
	                req = sata_make_enable_read_ahead_req(device);
	                break;
		}else
			device->state = DEVICE_STATE_ENABLE_READ_AHEAD_DONE;

	case DEVICE_STATE_ENABLE_READ_AHEAD_DONE:
	case DEVICE_STATE_SKIP_INIT_DONE:
		device->state = DEVICE_STATE_INIT_DONE;
		core_init_entry_done(root, device->base.port, &device->base);
		return MV_TRUE;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	case DEVICE_STATE_INIT_DONE:
		if(device->status & DEVICE_STATUS_NO_DEVICE){
			core_init_entry_done(root, device->base.port, &device->base);
			return MV_TRUE;
		}
#endif
    case DEVICE_STATE_DUPLICATE_DEVICE:
        return MV_TRUE;
    
        default:
                MV_ASSERT(MV_FALSE);
                return MV_FALSE;
	}

	if (req) {
		core_append_init_request(root, req);
		return MV_TRUE;
	} else {
		/* req is invalid, out of resource */
		return MV_FALSE;
	}
}

MV_BOOLEAN sata_port_store_sig(domain_port *port, MV_BOOLEAN from_sig_reg)
{
	domain_phy *phy = port->phy;
	pl_root *root = port->base.root;
	MV_U32 tmp, status;
	MV_U32 tmp_signature = 0xFFFFFFFF;
	MV_U8 pm_port = 0xFF;

	if (from_sig_reg) {
		/* read signature from signature registers */
		tmp_signature = 0;

        /* check the STATUS field in the FIS */
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG0);
        tmp = READ_PORT_CONFIG_DATA(root, phy);
        status = (tmp>>16) & 0xFF;
        if (status & 0x80) {
            /* busy */
            CORE_DPRINT(("phy %d signature busy\n", (root->base_phy_num+phy->id)));
            return MV_FALSE;
        }
		pm_port = (tmp >> 8) & 0xF;

		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG3);
		tmp = READ_PORT_CONFIG_DATA(root, phy) & 0xFF;
		tmp_signature |= tmp;

		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG1);
		tmp = READ_PORT_CONFIG_DATA(root, phy) & 0xFFFFFF;
		tmp_signature |= (tmp<<8);

		CORE_DPRINT(("sig from signature FIS, 0x%x\n", tmp_signature));
	} else {
		
	        /* check the STATUS field in the FIS */
	        tmp = MV_REG_READ_DWORD(root->rx_fis,
				(SATA_UNASSOC_D2H_FIS(phy)));
	        status = (tmp>>16) & 0xFF;
	        if (status & 0x80) {
	            /* busy */
	            CORE_DPRINT(("phy %d signature busy\n", (root->base_phy_num+phy->id)));
	            return MV_FALSE;
	        }
		
		pm_port = (tmp >> 8) & 0xF;	
		tmp = tmp & 0xFF;
		if (tmp != SATA_FIS_TYPE_REG_D2H) {
			CORE_DPRINT(("pm_port:%d Unassociated FIS type %x routed from Non-D2H type, not signature FIS %x.\n", pm_port, tmp, port->pm->pm_port_sig[pm_port]));
					/* Zero unassociated FIS as Athena spec */
			MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy), 0x0);
			MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0x4, 0x0);
			MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0x8, 0x0);
			MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0xC, 0x0);
			
			return MV_FALSE;
		}

		/* read signature from unassociated FIS structure */
		tmp_signature = 0;

		tmp = MV_REG_READ_DWORD(root->rx_fis,
			(SATA_UNASSOC_D2H_FIS(phy)+0xc));
		tmp_signature |= tmp;

		tmp = MV_REG_READ_DWORD(root->rx_fis,
			(SATA_UNASSOC_D2H_FIS(phy)+0x4));
		tmp_signature |= (tmp<<8);

		/* Zero unassociated FIS as Athena spec */
		MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy), 0x0);
		MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0x4, 0x0);
		MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0x8, 0x0);
		MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_D2H_FIS(phy) + 0xC, 0x0);

		CORE_DPRINT(("sig from unassociated FIS, 0x%x\n", tmp_signature));
	}

	if ((PORT_SATA_STATE_SIG_DONE == port->state) && port->pm && (pm_port < CORE_MAX_DEVICE_PER_PM)) {
		CORE_DPRINT(("pm_port:%d receive signature FIS:%x\n", pm_port, tmp_signature));
		port->pm->pm_port_sig[pm_port] = tmp_signature;
		if (pm_port != port->pm->active_port)
			return MV_FALSE;
	} else { 
		CORE_DPRINT(("sata port:%d receive signature FIS:%x\n", pm_port, tmp_signature));
		phy->sata_signature = tmp_signature;
	}
		
	return MV_TRUE;
}

MV_VOID sata_port_notify_event(MV_PVOID port_p, MV_U32 irq_status)
{
	domain_port *port = (domain_port *)port_p;
	pl_root *root = port->base.root;
	MV_BOOLEAN ret = MV_TRUE;

	/* we only process the notification if something is initializing */
	if (port->state != PORT_SATA_STATE_SIG_DONE) {
		CORE_DPRINT(("port %p state %d, irq_status %08X.\n", port, port->state,irq_status));
		if (irq_status & IRQ_SIG_FIS_RCVD_MASK) {
			ret = sata_port_store_sig(port, MV_TRUE);
            if (!ret) {
                /* WD drive workaround: when ret is FALSE, means the signature FIS
                * returned with status as 0xC0 ("busy"). It will send a second
                * signature FIS with status as 0x50 once it is ready. So we ignore
                * the first FIS and only starts initialization after the second
                * FIS is sent, which will trigger an unknown FIS interrupt instead
                * of signature FIS interrupt.
                */
				CORE_DPRINT(("WD workaround, wait for second FIS\n"));
                return;
            }
		} else if (irq_status & IRQ_UNASSOC_FIS_RCVD_MASK) {
			ret = sata_port_store_sig(port, MV_FALSE);
		}

		if (ret && (port->sata_sig_timer != NO_CURRENT_TIMER)) {
			core_cancel_timer(root->core, port->sata_sig_timer);
			port->sata_sig_timer = NO_CURRENT_TIMER;
			CORE_DPRINT(("got FIS interrupt, call sata_sig_time_out directly.\n"));
			_sata_sig_time_out(port, NULL);
		}
	} else if (port->pm != NULL) {
	#if 0//def _OS_LINUX
			/* For MV4140, different hardware behavior */
		CORE_DPRINT(("pm port %p state %d.\n", port, port->pm->state));
		/* PM is waiting for this */
		if( port->pm->state == PM_STATE_CLEAR_X_BIT ){
			if (irq_status & IRQ_SIG_FIS_RCVD_MASK) {
				sata_port_store_sig(port, MV_TRUE);
				goto got_sig;
			} else if (irq_status & IRQ_UNASSOC_FIS_RCVD_MASK) {
				sata_port_store_sig(port, MV_FALSE);
				goto got_sig;
			}
			return ;
	        got_sig:
			if (port->pm->sata_sig_timer != NO_CURRENT_TIMER) {
					core_cancel_timer(root->core,
						port->pm->sata_sig_timer);
				port->pm->sata_sig_timer = NO_CURRENT_TIMER;
					CORE_DPRINT(("got FIS interrupt, call pm_sig_time_out directly.\n"));
				pm_sig_time_out(port->pm, NULL);
			}
		}
	#else
#ifdef SUPPORT_QUICK_PM_INIT
			/* For frey, different hardware behavior */
		CORE_PRINT(("pm port %p state %d. active_port: %d\n", port, port->pm->state, port->pm->active_port));
		/* PM is waiting for this */
		if (irq_status & IRQ_SIG_FIS_RCVD_MASK) {
			ret = sata_port_store_sig(port, MV_TRUE);
		} else if (irq_status & IRQ_UNASSOC_FIS_RCVD_MASK) {
			ret = sata_port_store_sig(port, MV_FALSE);
		}
		if( (port->pm->state >= PM_STATE_CLEAR_X_BIT) && (port->pm->state <= PM_STATE_WAIT_SIG)) {
			if (ret && (port->pm->sata_sig_timer != NO_CURRENT_TIMER)) {
					core_cancel_timer(root->core,
						port->pm->sata_sig_timer);
				port->pm->sata_sig_timer = NO_CURRENT_TIMER;
					MV_PRINT("got FIS interrupt, call pm_sig_time_out directly.\n");
				_pm_sig_time_out(port->pm, NULL);
			}
		}
#else


		CORE_DPRINT(("pm port %p state %d.\n", port, port->pm->state));
		/* PM is waiting for this */
		if (port->pm->state == PM_STATE_ISSUE_SOFT_RESET_0) {
			if (irq_status & IRQ_UNASSOC_FIS_RCVD_MASK) {
				ret = sata_port_store_sig(port, MV_FALSE);
				if (ret && (port->pm->sata_sig_timer != NO_CURRENT_TIMER)) {
					core_cancel_timer(root->core,
						port->pm->sata_sig_timer);
					port->pm->sata_sig_timer = NO_CURRENT_TIMER;
					CORE_DPRINT(("got FIS interrupt, call pm_sig_time_out directly.\n"));
					_pm_sig_time_out(port->pm, NULL);
				}
			}
		}
#endif
	#endif
	}
}

MV_VOID sata_scsi_report_lun(pl_root *root, MV_Request *req)
{
	MV_U32 alloc_len, lun_list_len;
	MV_PU8 buf_ptr;

	alloc_len = ((MV_U32)(req->Cdb[6] << 24)) |
		((MV_U32)(req->Cdb[7] << 16)) |
		((MV_U32)(req->Cdb[8] << 8)) |
		((MV_U32)(req->Cdb[9]));

	/* allocation length should not less than 16 bytes */
	if (alloc_len < 16) {
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB);
		return;
	}

	buf_ptr = core_map_data_buffer(req);
	MV_ZeroMemory(buf_ptr, alloc_len);
	/* LUN1 may have device */
	if (req->Device_Id == VIRTUAL_DEVICE_ID) {
		lun_list_len = 16;
		if (alloc_len >= 24 && req->Data_Transfer_Length >= 24)
			buf_ptr[23] = 0x01;	/* LUN1 has device */
	} else {
		/* Only LUN0 has device */
		lun_list_len = 8;
	}

	if (req->Data_Transfer_Length >= 4) {
		buf_ptr[0] = (MV_U8)((lun_list_len & 0xFF000000) >> 24);
		buf_ptr[1] = (MV_U8)((lun_list_len & 0x00FF0000) >> 16);
		buf_ptr[2] = (MV_U8)((lun_list_len & 0x0000FF00) >> 8);
		buf_ptr[3] = (MV_U8)(lun_list_len & 0x000000FF);
	}

	core_unmap_data_buffer(req);
	req->Scsi_Status = REQ_STATUS_SUCCESS;
}

MV_VOID sata_scsi_fill_self_test_log_page(MV_PU8 buf,
	MV_U8 page_num)
{
	// Parameter Code
	buf[0] = 0;
	buf[1] = page_num + 1;

	// DU, DS, TSD, ETC, TMC, LBIN, LP set to 0
	buf[2] = 0;

	// Parameter length
	buf[3] = 0x10;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;
	buf[8] = 0;
	buf[9] = 0;
	buf[10] = 0;
	buf[11] = 0;
	buf[12] = 0;
	buf[13] = 0;
	buf[14] = 0;
	buf[15] = 0;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;
	buf[19] = 0;
}

MV_U32 sata_scsi_fill_log_sense_supported(domain_device *device,
        MV_PU8 data_buf)
{
	MV_U8 length = 0;

	if (device->capability &
		DEVICE_CAPABILITY_SMART_SUPPORTED) {
		data_buf[length++] = WRITE_ERROR_COUNTER_LOG_PAGE;
		data_buf[length++] = READ_ERROR_COUNTER_LOG_PAGE;
		data_buf[length++] = READ_REVERSE_ERROR_COUNTER_LOG_PAGE;
		data_buf[length++] = VERIFY_ERROR_COUNTER_LOG_PAGE;
		data_buf[length++] = TEMPERATURE_LOG_PAGE;
		if (device->capability &
			DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) {
			data_buf[length++] = SELF_TEST_RESULTS_LOG_PAGE;
		}
		data_buf[length++] = INFORMATIONAL_EXCEPTIONS_LOG_PAGE;
	}

        return (length);
}

MV_U32 sata_scsi_fill_log_sense_self_test(MV_PU8 data_buf)
{
	MV_U8 i;
	for (i = 0; i < 20; i++) {
		sata_scsi_fill_self_test_log_page(
		&data_buf[i * 20],
		i);
	}

	return 0x190;
}

MV_U32 sata_scsi_fill_log_sense_error_counter(MV_PU8 data_buf,
        MV_U8 page_code)
{
	/* DU=0, DS=0, TSD=0, ETC=0, TMC=0, LBIN=1, LP=1 */
	data_buf[0] = 0x03;
	data_buf[1] = 2; /* parameter length */
	data_buf[2] = 0;
	data_buf[3] = 0; /* no errors */

	return 4;
}

MV_U32 sata_scsi_fill_log_sense_temperature(MV_PU8 data_buf)
{
	data_buf[0] = 0;
	data_buf[1] = 0; /* parameter code = 0*/
	data_buf[2] = 0x03; /* LBIN=1, LP=1 */
	data_buf[3] = 0x02; /* parameter length = 2 */
	data_buf[4] = 0;
	data_buf[5] = 0xFF; /* temperature in Celsius */
	data_buf[6] = 0;
	data_buf[7] = 01; /* parameter code = 1*/
	data_buf[8] = 0x03; /* LBIN=1, LP=1 */
	data_buf[9] = 0x02; /* parameter length = 2 */
	data_buf[10] = 0;
	data_buf[11] = 0xFF; /* reference temperature in Celsius */

	return 0x0C;
}

MV_U8 sata_scsi_log_sense(pl_root *root, MV_Request *req)
{
	MV_U32 length = 0;
	MV_PU8 data_buf;
	domain_device *device;

	device = (domain_device *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	// Check page control (PC) settings
	if ((req->Cdb[2] & 0xC0) != 0x40) {
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB);

		return MV_TRUE;
	}

	data_buf = core_map_data_buffer(req);
	// Report log depending on page code
	switch (req->Cdb[2] & 0x3F) {
	case SUPPORTED_LOG_PAGES_LOG_PAGE:
		if (req->Data_Transfer_Length >= 11)
			length = sata_scsi_fill_log_sense_supported(device,
			&data_buf[4]);
			break;

	case SELF_TEST_RESULTS_LOG_PAGE:
		if (req->Data_Transfer_Length >= 404)
			length = sata_scsi_fill_log_sense_self_test(
			&data_buf[4]);
			break;
	case WRITE_ERROR_COUNTER_LOG_PAGE:
	case READ_ERROR_COUNTER_LOG_PAGE:
	case READ_REVERSE_ERROR_COUNTER_LOG_PAGE:
	case VERIFY_ERROR_COUNTER_LOG_PAGE:
		if (req->Data_Transfer_Length >= 8)
			length = sata_scsi_fill_log_sense_error_counter(
				&data_buf[4], req->Cdb[2] & 0x3F);
		break;
	case TEMPERATURE_LOG_PAGE:
		if (req->Data_Transfer_Length >= 16)
			length = sata_scsi_fill_log_sense_temperature(
				&data_buf[4]);
		break;
	case INFORMATIONAL_EXCEPTIONS_LOG_PAGE:
		core_unmap_data_buffer(req);
		return MV_FALSE;
	default:
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB);
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		return MV_TRUE;
	}

	if (req->Data_Transfer_Length >= 4) {
		data_buf[0] = req->Cdb[2] & 0x3F;
		data_buf[1] = 0; /* reserved */
		data_buf[2] = (MV_U8)(length >> 8) & 0xFF; /* length */
		data_buf[3] = (MV_U8)length & 0xFF; /* length */
	}

	core_unmap_data_buffer(req);
	req->Data_Transfer_Length = length;
	req->Scsi_Status = REQ_STATUS_SUCCESS;

	return MV_TRUE;
}

MV_U8 sata_scsi_fill_mode_page_direct_access(MV_PU8 buf, domain_device *device)
{
        buf[0] = 0x00;
        buf[1] = 0x01;
        buf[2] = 0x00;

        return 3;
}

MV_U8 sata_scsi_fill_mode_page_caching(MV_PU8 buf, domain_device *device)
{
	buf[0] = 0x08;		/* Page Code, PS = 0; */
	buf[1] = 0x12;		/* Page Length */
	buf[2] = 0;
	/* set the WCE bit based on device identification data */
	if (device->setting & DEVICE_SETTING_WRITECACHE_ENABLED)
		buf[2] |= MV_BIT(2);
	buf[3] = 0;	/* Demand read/write retention priority */
	buf[4] = 0xff;	/* Disable pre-fetch trnasfer length (4,5) */
	buf[5] = 0xff;	/* all anticipatory pre-fetching is disabled */
	buf[6] = 0;	/* Minimum pre-fetch (6,7) */
	buf[7] = 0;
	buf[8] = 0;	/* Maximum pre-fetch (8,9) */
	buf[9] = 0x01;
	buf[10] = 0;	/* Maximum pre-fetch ceiling (10,11) */
	buf[11] = 0x01;
	buf[12] = 0;

	/* set the DRA bit based on device identification data */

	/* A disable read-ahead (DRA) bit set to one specifies that the device
	 * server shall not read into the pre-fetch buffer any logical blocks
	 * beyond the addressed logical block(s). A DRA bit set to zero
	 * specifies that the device server may continue to read logical blocks
	 * into the pre-fetch buffer beyond the addressed logical block(s).
	 */
	if (!(device->setting & DEVICE_SETTING_READ_LOOK_AHEAD))
		buf[12] |= MV_BIT(5);
	buf[13] = 0x01;	/* Number of cache segments */
	buf[14] = 0xff;	/* Cache segment size (14, 15) */
	buf[15] = 0xff;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;
	buf[19] = 0;
	return 0x14;	/* Total page length in byte */
}

MV_U8 sata_scsi_fill_mode_page_rw_error(MV_PU8 data_buf, domain_device *device)
{
	data_buf[0] = RW_ERROR_RECOVERY_MODE_PAGE;
	data_buf[1] = 0x0A; /* page length */
	data_buf[2] = 0x80; /* AWRE bit */
	data_buf[3] = 0; /* read retry count */
	data_buf[4] = 0;
	data_buf[5] = 0;
	data_buf[6] = 0;
	data_buf[7] = 0;
	data_buf[8] = 0; /* write retry count */
	data_buf[9] = 0;
	data_buf[10] = 0;
	data_buf[11] = 0;

	return 12;
}

MV_U8 sata_scsi_fill_mode_page_ctrl(MV_PU8 data_buf, domain_device *device)
{
	data_buf[0] = CONTROL_MODE_PAGE;
	data_buf[1] = 0x0A; /* page length */
	data_buf[2] = 0x02; /* GLTSD=1 */
	data_buf[3] = 0;
	data_buf[4] = 0;
	data_buf[5] = 0;
	data_buf[6] = 0;
	data_buf[7] = 0;
	data_buf[8] = 0xFF;
	data_buf[9] = 0xFF; /* busy timeout period */
	if (device->capability & DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) {
		data_buf[10] = 0x02; /* extended self-test cmpltn time */
		data_buf[11] = 0x58; /* see 05-359r1 */
	} else {
		data_buf[10] = 0;
		data_buf[11] = 0;
	}

	return 12;
}

MV_U8 sata_scsi_fill_mode_page_port(MV_PU8 data_buf, domain_device *device)
{
	data_buf[0] = PORT_MODE_PAGE;
	data_buf[1] = 0x01; /* page length */
	data_buf[2] = 0x08; /* protocol identifier = ATA */

	return 3;
}

MV_U8 sata_scsi_fill_mode_page_iec(MV_PU8 data_buf, domain_device *device)
{
	data_buf[0] = INFORMATIONAL_EXCEPTIONS_CONTROL_MODE_PAGE;
	data_buf[1] = 0x0A; /* page length */
	/* PERF=1, RSRV, EBF=1, EWasc=1, DExcpt=0, TEST=1, EBACKerr=1, LOGERR=1 */
	data_buf[2] = 0xB7;
	data_buf[3] = 0x04; /* MRIE=Unconditionally generate recovered error */
	data_buf[4] = 0;
	data_buf[5] = 0;
	data_buf[6] = 0;
	data_buf[7] = 0; /* INTERVAL TIMER */
	data_buf[8] = 0;
	data_buf[9] = 0;
	data_buf[10] = 0;
	data_buf[11] = 0; /* REPORT COUNT */

	return 12;
}

MV_U8 sata_scsi_fill_mode_page_all(MV_PU8 data_buf, domain_device *device)
{
	MV_U8 length = 0;

	length += sata_scsi_fill_mode_page_direct_access(&data_buf[length],
	                device);
	length += sata_scsi_fill_mode_page_caching(&data_buf[length], device);
	length += sata_scsi_fill_mode_page_rw_error(&data_buf[length], device);
	length += sata_scsi_fill_mode_page_ctrl(&data_buf[length], device);
	length += sata_scsi_fill_mode_page_port(&data_buf[length], device);
	length += sata_scsi_fill_mode_page_iec(&data_buf[length], device);

	return length;
}
MV_U8 sata_check_page_control(MV_Request *req,MV_U8 page_control){
	MV_U8 ret=0;
	switch(page_control){
		case 0:/* only support current */
			ret=0;
			break;
		case 3:
			ret=1;
			req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORT);
			break;
		case 1:
		case 2:
		default:
			ret=1;
			req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_FEILD_IN_CDB);
			break;
		}
	return ret;
}

MV_BOOLEAN sata_scsi_maintenance_in(pl_root *root, MV_Request *req)
{
     if (req->Cdb[1] == 0x0c){/*MI_REPORT_SUPPORTED_OPERATION_CODES */
	 	req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB);

		return MV_TRUE;
     }
		
	 return MV_FALSE;
}

MV_VOID sata_scsi_mode_sense(pl_root *root, MV_Request *req)
{
	MV_U8 page_code = req->Cdb[2] & 0x3F;	/* Same for mode sense 6 and 10 */
	MV_U8 page_control=req->Cdb[2]>>6;
	MV_U32 page_len = 0, tmp_len = 0;
	domain_device *device;
	MV_PU8 buf_ptr = core_map_data_buffer(req);
    MV_U32 offset;

	device = (domain_device *)get_device_by_id(root->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
    if (buf_ptr == NULL) 
    {
        req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB);
		core_unmap_data_buffer(req);
		return;
    }

	if (req->Cdb[0] == SCSI_CMD_MODE_SENSE_6)
		offset = 4;
	else
		offset = 8;

	switch (page_code) {
	case DIRECT_ACCESS_BLOCK_DEVICE_MODE_PAGE:
		page_len = 3;
		if (req->Data_Transfer_Length >= (3 + offset))
		page_len = sata_scsi_fill_mode_page_direct_access(
		                &buf_ptr[offset], device);
		break;
	case RW_ERROR_RECOVERY_MODE_PAGE:
		page_len = 12;
		if (req->Data_Transfer_Length >= (12 + offset))
		page_len = sata_scsi_fill_mode_page_rw_error(
		                &buf_ptr[offset], device);
		break;
	case CACHE_MODE_PAGE:
		page_len = 20;
		if(sata_check_page_control(req,page_control))
			break;
		if (req->Data_Transfer_Length >= (20 + offset))
		page_len = sata_scsi_fill_mode_page_caching(
		                &buf_ptr[offset], device);
		break;
	case CONTROL_MODE_PAGE:
		page_len = 12;
		if (req->Data_Transfer_Length >= (12 + offset))
		page_len = sata_scsi_fill_mode_page_ctrl(
		                &buf_ptr[offset], device);
		break;
	case PORT_MODE_PAGE:
		page_len = 3;
		if (req->Data_Transfer_Length >= (3 + offset))
		page_len = sata_scsi_fill_mode_page_port(
		                &buf_ptr[offset], device);
		break;
	case INFORMATIONAL_EXCEPTIONS_CONTROL_MODE_PAGE:
		page_len = 12;
		if (req->Data_Transfer_Length >= (12 + offset))
		page_len = sata_scsi_fill_mode_page_iec(
		                &buf_ptr[offset], device);
		break;
	case ALL_MODE_PAGE:
		page_len = 62;
		if (req->Data_Transfer_Length >= (62 + offset))
		page_len = sata_scsi_fill_mode_page_all(
		                &buf_ptr[offset], device);
		break;
	default:
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		prot_fill_sense_data(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB);
		return;
	}


	/* Mode Data Length, it does not include the number of bytes in */
	/* Mode Data Length field */
	if (req->Cdb[0] == SCSI_CMD_MODE_SENSE_6) {
		buf_ptr[0] = (MV_U8)(page_len + 3);    /* Mode data length */
		buf_ptr[1] = 0;                /* Medium type=0 */
		buf_ptr[2] = 0;                /* Device-Specific Parameter*/
		buf_ptr[3] = 0;                /* Block Descriptor Length=0 */
		tmp_len = MV_MIN(req->Data_Transfer_Length, (page_len + 4));
	} else {
		tmp_len = page_len + 6;
		buf_ptr[0] = (MV_U8)(((MV_U16)tmp_len) >> 8);
		buf_ptr[1] = (MV_U8)tmp_len;
		buf_ptr[2] = 0;
		buf_ptr[3] = 0;
		buf_ptr[4] = 0;
		buf_ptr[5] = 0;
		buf_ptr[6] = 0;
		buf_ptr[7] = 0;
		tmp_len = MV_MIN(req->Data_Transfer_Length, (page_len + 8));
	}

	core_unmap_data_buffer(req);
	req->Data_Transfer_Length = tmp_len;
	req->Scsi_Status = REQ_STATUS_SUCCESS;
}

MV_U8
mode_select_helper(pl_root *root, MV_Request *req)
{
	MV_U32  length = req->Data_Transfer_Length;
	MV_U8   PF = (req->Cdb[1] & MV_BIT(4)) >> 4;
	MV_U8   SP = (req->Cdb[1] & MV_BIT(0));
	MV_U8*	list = core_map_data_buffer(req);
	MV_U32  offset = 0;
	MV_U8   ret = 0;
	
	if (PF == 0 && SP == 0) {
		ret = 0x4;
		goto mode_select_helper_end;
	}

	if (PF == 0) {
		ret = 0x2; /* Invalid field in CDB */
		goto mode_select_helper_end;
	}

	if (SP == 1) {
		ret = 0x2; /* PARAMETER LIST LENGTH ERROR */
		goto mode_select_helper_end;
	}

	if (length == 0) {
		ret = 0;
		goto mode_select_helper_end;
	}

	if (length < 4) {
		ret = 0x1; /* PARAMETER LIST LENGTH ERROR */
		goto mode_select_helper_end;
	}

	if (list[0] || (list[1] != MV_SCSI_DIRECT_ACCESS_DEVICE) || list[2]) {
		ret = 0x3; /* Invalid field in parameter list */
		goto mode_select_helper_end;
	}

	if (req->Cdb[0] == SCSI_CMD_MODE_SELECT_6){
	if (list[3]) {
		if (list[3] != 8) {
			ret = 0x3; /* Invalid field in parameter list */
			goto mode_select_helper_end;
		}

		if (length < 12) {
			ret = 0x1; /* PARAMETER LIST LENGTH ERROR */
			goto mode_select_helper_end;
		}

		if (list[4] || list[5] || list[6] || list[7] || list[8] || list[9] ||
		(list[10] != 0x2) || list[11]) {
			ret = 0x3; /* Invalid field in parameter list */
			goto mode_select_helper_end;
		}
	}
	offset = 4 + list[3];/* skip the mode parameter block descriptor */
   }else{
	   if (list[7]){
		   if (list[7] != 8){
			   ret = 0x3; /* Invalid field in parameter list */
			   goto mode_select_helper_end;
		   }

		   if (length < 16){
			   ret = 0x1; /* PARAMETER LIST LENGTH ERROR */
			   goto mode_select_helper_end;
		   }
			   
		   if (list[8] || list[9] || list[10] || list[11] || list[12] || list[13] ||
		   (list[14] != 0x2) || list[15]) {
			   ret = 0x3; /* Invalid field in parameter list */
			   goto mode_select_helper_end;
		   }
	   }

	   offset = 8 + list[7];   
    }
	
	if (length == offset) {
		ret = 0;
		goto mode_select_helper_end;
	}

	while ((offset + 2) < length) {
		switch (list[offset] & 0x3f) {
		case CACHE_MODE_PAGE:
			ret = 0xff;
			goto mode_select_helper_end;
		case CONTROL_MODE_PAGE:
			if ((list[offset] != 0xa) || (list[offset+1] != 0xa)) {
			ret = 0x3;
			goto mode_select_helper_end;
			}

			if (list[offset + 3] != MV_BIT(4)) {
				ret = 0x3;
				goto mode_select_helper_end;
			}

			if (list[offset + 2] || list[offset + 4] || list[offset + 5] ||
			list[offset + 6] || list[offset + 7]||list[offset + 8] ||
			list[offset + 9]|| list[offset + 10] || list[offset + 11]) {
				ret = 0x3;
				goto mode_select_helper_end;
			}

			offset += list[offset + 1] + 2;
			break;
		default:
			ret = 0x3; /* Invalid field in parameter list */
			goto mode_select_helper_end;
		}
	}

	if (length != offset)
		ret = 0x1; /* PARAMETER LIST LENGTH ERROR */

mode_select_helper_end:
	core_unmap_data_buffer(req);
	return ret;
}

MV_BOOLEAN sata_scsi_mode_select(pl_root *root, MV_Request *req)
{
	MV_U8 result;
	result = mode_select_helper(root, req);

	if (result == 0xff)
		return MV_FALSE;

	if (result == 0x0) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
	} else if (req->Sense_Info_Buffer_Length > sizeof(MV_Sense_Data)) {
		if (result == 0x1) {/*PARAMETER LIST LENGTH ERROR*/
			MV_SetSenseData(
			(PMV_Sense_Data)req->Sense_Info_Buffer,
			SCSI_SK_ILLEGAL_REQUEST, 0x1a, 0);
		} else if (result == 0x2) {
			MV_SetSenseData(
			(PMV_Sense_Data)req->Sense_Info_Buffer,
			SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ADSENSE_INVALID_CDB, 0);	//TBD: Definition: Don't use windows.
		} else if (result == 0x3) {
			MV_SetSenseData(
			(PMV_Sense_Data)req->Sense_Info_Buffer,
			SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FIELD_IN_PARAMETER, 0);
		} else if (result == 0x4) {
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_TRUE;
		} else {
			MV_SetSenseData(
			(PMV_Sense_Data)req->Sense_Info_Buffer,
			SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ADSENSE_NO_SENSE, 0);
		}
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
	} else {
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
	}

	return MV_TRUE;
}

void sata_scsi_read_defect_data(pl_root *root, MV_Request *req)
{
	MV_U32 length;
	MV_PU8 buf_ptr = core_map_data_buffer(req);

	length = MV_MIN(req->Data_Transfer_Length, 4);

	if (length >= 4) {
		buf_ptr[0] = 0;
		buf_ptr[1] = req->Cdb[2] & 0x07;
		buf_ptr[2] = 0;
		buf_ptr[3] = 0;
	}

	core_unmap_data_buffer(req);
	req->Data_Transfer_Length = length;
	req->Scsi_Status = REQ_STATUS_SUCCESS;
}

void sata_get_ncq_tag(MV_PVOID dev_p, MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
	int tag;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_ULONG flags;

	MV_DASSERT(IS_STP_OR_SATA(device));
	MV_DASSERT(req->Cmd_Flag & CMD_FLAG_NCQ);
	OSSW_SPIN_LOCK(&device->ncq_tags_spin_lock, flags); 
#ifndef _OS_LINUX
	tag = fc(device->ncq_tags, device->next_ncq);
#else
	//NCQ command will time out when execute FDISK with SATA disks behind  expander if get tag by FIFIO mode instead of sequence order.
	//TBD:I'm not sure hardware behavior.
	//if(IS_STP(device))
		tag = ffc(device->ncq_tags);
	//else
	//	tag = fc(device->ncq_tags, device->next_ncq);	//Fixed issue -- device timeout under RHEL 4.8

#endif
	MV_DASSERT(tag != -1);

	if (tag >= 32) tag -= 32;
	device->next_ncq = tag+1;
	if (device->next_ncq >= 32) device->next_ncq -= 32;

	MV_DASSERT(tag >= 0);

	device->ncq_tags |= MV_BIT(tag);
	OSSW_SPIN_UNLOCK(&device->ncq_tags_spin_lock, flags);
	ctx->ncq_tag = (MV_U8) tag;
}

void sata_free_ncq_tag(MV_PVOID dev_p, MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_ULONG flags;
	MV_U8 tag = ctx->ncq_tag;

	MV_DASSERT(IS_STP_OR_SATA(device));
	MV_DASSERT(req->Cmd_Flag & CMD_FLAG_NCQ);
	OSSW_SPIN_LOCK(&device->ncq_tags_spin_lock, flags); 
	device->ncq_tags &= ~MV_BIT(tag);
	OSSW_SPIN_UNLOCK(&device->ncq_tags_spin_lock, flags);
}

MV_BOOLEAN sata_check_instant_req(pl_root *root, domain_device *device,
	MV_Request *req)
{
	MV_BOOLEAN ret = MV_TRUE;

	domain_port *port=device->base.port;
	domain_phy *phy = port->phy;
	MV_U32 i,reg;
 /* tempature disable shutdown command */
	if (req->Cdb[0] == APICDB0_ADAPTER){
		/* skip adapter commands */
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_TRUE;
	}


	if (IS_ATAPI(device)) {
		if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
			(req->Cdb[1] == CDB_CORE_MODULE) &&
			(req->Cdb[2] == CDB_CORE_OS_SMART_CMD)) {
			/* skip SMART commands */
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_TRUE;
		}
		
		//for port_reset
		if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
			(req->Cdb[1] == CDB_CORE_MODULE) &&
			(req->Cdb[2] == CDB_CORE_RESET_PORT)) {
			mv_reset_phy(root, req->Cdb[3], MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_TRUE;
		}
		return MV_FALSE;
	}

	switch (req->Cdb[0]) {
	case SCSI_CMD_RESERVE_6:	/* For Odin, just return good status */
	case SCSI_CMD_RELEASE_6:
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		break;
	case SCSI_CMD_REPORT_LUN:
		sata_scsi_report_lun(root, req);
		break;
	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
		sata_scsi_mode_sense(root, req);
		break;
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		ret = sata_scsi_mode_select(root, req);
		break;
	case SCSI_CMD_LOG_SENSE:
		ret = sata_scsi_log_sense(root, req);
		break;
	case SCSI_CMD_READ_DEFECT_DATA_10:
		sata_scsi_read_defect_data(root, req);
		break;
	case SCSI_CMD_MAINTENANCE_IN:
		ret = sata_scsi_maintenance_in(root, req);
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (req->Cdb[1] == CDB_CORE_MODULE) {
			if (req->Cdb[2] == CDB_CORE_RESET_DEVICE) {
				if (device->base.parent->type == BASE_TYPE_DOMAIN_PM)
				{
					extern MV_VOID pm_device_phy_reset(domain_pm *pm, MV_U8 device_port);
					pm_device_phy_reset(device->pm, device->pm_port);
					req->Scsi_Status = REQ_STATUS_SUCCESS;
					ret = MV_TRUE;
				}
				else
				{
					mv_reset_phy(root, req->Cdb[3], MV_FALSE);
					req->Scsi_Status = REQ_STATUS_SUCCESS;
					ret = MV_TRUE;
				}
			} else if (req->Cdb[2] == CDB_CORE_RESET_PORT) {
				mv_reset_phy(root, req->Cdb[3], MV_TRUE);
				for(i=0; i<50; i++)
				{
					reg = READ_PORT_IRQ_STAT(root, phy);
					if((reg & IRQ_UNASSOC_FIS_RCVD_MASK || reg & IRQ_SIG_FIS_RCVD_MASK))
					{
						CORE_DPRINT(("PHYreset FIS has return!!!\n"));
						break;
					}
					core_sleep_millisecond(root->core, 10);
				}
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				ret = MV_TRUE;
			} else {
				ret = MV_FALSE;
			}
		} else {
			ret = MV_FALSE;
		}
		break;
	default:
		ret = MV_FALSE;
		break;
	}

	return ret;
}

MV_U8 sata_verify_command(MV_PVOID root, MV_PVOID dev_p,
	MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
        core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U8 set, ret;
	MV_ULONG flags;
#ifdef CORE_SUPPORT_LARGE_REQUEST
        MV_Request *sub_req;
#endif
	MV_Request *new_req = NULL;

	if (!(device->status & DEVICE_STATUS_FUNCTIONAL)) {
	#ifdef SUPPORT_VU_CMD
		if( !(IS_ATA_PASSTHRU_CMD(req,SCSI_CMD_MARVELL_VENDOR_UNIQUE)))
	#endif
		{
			req->Scsi_Status = REQ_STATUS_NO_DEVICE;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	}

    if (SCSI_IS_READ(req->Cdb[0]) || SCSI_IS_WRITE(req->Cdb[0])) {
        
#ifdef CORE_SUPPORT_LARGE_REQUEST
            sub_req = core_split_large_request(root, req);
            if (sub_req == NULL)
                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
            else if (sub_req != req)
                return MV_QUEUE_COMMAND_RESULT_REPLACED;
#endif
        

        /* if false, we don't support this command. */
        if (sat_categorize_cdb(root, req) == MV_FALSE) {
            req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
            return MV_QUEUE_COMMAND_RESULT_FINISHED;
        }

    } else {

         /* ikkwong TBD
               Move these 2 to scsi_ata_translation or create a new
               scsi_smp_translation function
             */

    	/* stp phy control reset virtual request */
    	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
    		(req->Cdb[1] == CDB_CORE_MODULE) &&
    		(req->Cdb[2] == CDB_CORE_STP_VIRTUAL_PHY_RESET)) {
    		new_req = smp_make_phy_control_req(device, req->Cdb[3],
    			smp_physical_req_callback);
    		if (!new_req) {
    			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
    		}
    		((core_context *)new_req->Context[MODULE_CORE])->u.org.org_req = req;
    		core_queue_eh_req(root, new_req);
    		return MV_QUEUE_COMMAND_RESULT_REPLACED;
    	}

    	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
    		(req->Cdb[1] == CDB_CORE_MODULE) &&
    		(req->Cdb[2] == CDB_CORE_STP_VIRTUAL_REPORT_SATA_PHY)) {
    			new_req = smp_make_report_phy_sata_req(device, smp_physical_req_callback);
    			if (!new_req)
    				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
    			((core_context *)new_req->Context[MODULE_CORE])->u.org.org_req = req;
    			if (CORE_IS_INIT_REQ(ctx))
    				core_append_init_request(root, new_req);
    			else if (CORE_IS_EH_REQ(ctx))
    				core_queue_eh_req(root, new_req);
    			else
    				core_append_request(root, new_req);

    			return MV_QUEUE_COMMAND_RESULT_REPLACED;
    		}

        if (sata_check_instant_req(root, device, req))
            return MV_QUEUE_COMMAND_RESULT_FINISHED;
#ifdef SUPPORT_PASS_THROUGH_DIRECT
        if((req->Cdb[0] == APICDB0_PASS_THRU_CMD_SCSI)||
                    (req->Cdb[0] == APICDB0_PASS_THRU_CMD_ATA)){
            return (core_pass_thru_send_command(
                                    ((pl_root *)root)->core, req));
        }
#endif

    	/* if false, we don't support this command. */
    	if (sat_categorize_cdb(root, req) == MV_FALSE) {
    		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
    		return MV_QUEUE_COMMAND_RESULT_FINISHED;
    	}

    	ret = scsi_ata_translation(root, req);
    	if (ret == MV_QUEUE_COMMAND_RESULT_FINISHED) {
    		return MV_QUEUE_COMMAND_RESULT_FINISHED;
    	} else if (ret == MV_QUEUE_COMMAND_RESULT_NO_RESOURCE ||
    		ret == MV_QUEUE_COMMAND_RESULT_REPLACED) {
    		req->Scsi_Status = REQ_STATUS_PENDING;
    		return ret;
    	}


    } //if (SCSI_IS_READ(req->Cdb[0]) || SCSI_IS_WRITE(req->Cdb[0]))

	/* only get a register set if this device doesn't already have one */
	//OSSW_SPIN_LOCK_Device(device->base.root->core, flags, &device->base);
	OSSW_SPIN_LOCK( &device->base.base_SpinLock, flags);
	if (device->register_set == NO_REGISTER_SET) {
		set = core_get_register_set(root, device);
		if (set == NO_REGISTER_SET){
			//OSSW_SPIN_UNLOCK_Device(device->base.root->core, flags, &device->base);
			OSSW_SPIN_UNLOCK( &device->base.base_SpinLock, flags);
			return MV_QUEUE_COMMAND_RESULT_FULL;
		}
		else{
			device->register_set = set;
		}
	}
	//OSSW_SPIN_UNLOCK_Device(device->base.root->core, flags, &device->base);
	OSSW_SPIN_UNLOCK( &device->base.base_SpinLock, flags);
	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		//OSSW_SPIN_LOCK(&device->base.outstanding_req_SpinkLock);
		if (device->base.outstanding_req != 0) {
			if (!(device->setting & DEVICE_SETTING_NCQ_RUNNING)) {
				/* non ncq request is running */
				return MV_QUEUE_COMMAND_RESULT_FULL;
			} else {
				/* already checked in prot_send_request by queue_depth */
                if((device->setting & DEVICE_SETTING_STOP_RUN_NCQ) && (!CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)
#ifdef SUPPORT_SG_RESET
				&& !CORE_IS_HBA_RESET_REQ(req)
#endif
			))
                		return MV_QUEUE_COMMAND_RESULT_BLOCKED;
				#if 0
				/* ncq request is running */
				if (device->base.outstanding_req >= 32)
				return MV_QUEUE_COMMAND_RESULT_FULL;
				#else
				MV_DASSERT(device->base.outstanding_req < 32);
				#endif
			}
		} else {
			if((device->setting & DEVICE_SETTING_STOP_RUN_NCQ) && (!CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)
#ifdef SUPPORT_SG_RESET
				&& !CORE_IS_HBA_RESET_REQ(req)
#endif
			))
                return MV_QUEUE_COMMAND_RESULT_BLOCKED;
			device->setting |= DEVICE_SETTING_NCQ_RUNNING;
		}
	} else{
		/* if its non-ncq command, we do one by one.
		 * because we may need read the FIS data like smart return status command */
		if (device->base.outstanding_req != 0) {
			device->setting |= DEVICE_SETTING_STOP_RUN_NCQ;
			return MV_QUEUE_COMMAND_RESULT_FULL;
		} else {
			device->setting &= ~DEVICE_SETTING_NCQ_RUNNING;
			device->setting &= ~DEVICE_SETTING_STOP_RUN_NCQ;
		}
	}

	return MV_QUEUE_COMMAND_RESULT_PASSED;
}

MV_VOID sata_prepare_command_header(MV_Request *req,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_U8 tag, MV_U8 pm_port)
{
	/* clear sata ctrl and sas ctrl */
	MV_U32 dword_0= 0;
	if (req->Cmd_Flag & CMD_FLAG_PACKET)
		dword_0 |= CH_ATAPI;
	if (req->Cmd_Flag & CMD_FLAG_SOFT_RESET)
		dword_0 |= CH_RESET;
	if (req->Cmd_Flag & CMD_FLAG_NCQ)
		dword_0 |= CH_FPDMA;
#ifdef SUPPORT_PIR
	if(req->Cmd_Flag & CMD_FLAG_PIR) 
	{
		dword_0 |= CH_PI_PRESENT;
		cmd_header->pir_fmt=0x8;
	}
#endif
#ifdef HAVE_PRD_SKIP
	if (req->Cmd_Flag & CMD_FLAG_PRD_SKIP) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		dword_0 |= CH_DATA_SKIIP_ENTRY_IN_PRD;
#else
		dword_0 |= CH_PI_PRESENT;
#endif
	}
#endif

	dword_0 |= (pm_port & CH_PM_PORT_MASK);
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(dword_0);
	//JING: remove related function to sub directory
#ifndef SUPPORT_BALDUR
	cmd_header->frame_len = MV_CPU_TO_LE16(FIS_REG_H2D_SIZE_IN_DWORD);
#else
	//cmd_header->frame_len = MV_CPU_TO_LE16(
	//	FIS_REG_H2D_SIZE_IN_DWORD | (1<<CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT));
        /* for sata, set the max simultaneous connection to zero for performance */
	cmd_header->frame_len = MV_CPU_TO_LE16(FIS_REG_H2D_SIZE_IN_DWORD);
#endif
	cmd_header->tag = MV_CPU_TO_LE16(tag);
}

MV_VOID sata_prepare_command_table(MV_Request *req, mv_command_table *cmd_table,
	ata_taskfile *taskfile, MV_U8 pm_port)
{
	scsi_to_sata_fis(req, cmd_table->table.stp_cmd_table.fis, taskfile, pm_port);

	if (req->Cmd_Flag & CMD_FLAG_PACKET) {
		MV_CopyMemory(&cmd_table->table.stp_cmd_table.atapi_cdb, req->Cdb,
			MAX_CDB_SIZE);
	}
#ifdef SUPPORT_PIR
	if (req->Cmd_Flag & CMD_FLAG_PIR){
		MV_ZeroMemory( &cmd_table->table.stp_cmd_table.pir, sizeof(protect_info_record));
		cmd_table->table.stp_cmd_table.pir.T10_CHK_EN=1;
		cmd_table->table.stp_cmd_table.pir.USR_DT_SZ=0x40;
	}
#endif
#if defined(HAVE_PRD_SKIP) && !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	if (req->Cmd_Flag & CMD_FLAG_PRD_SKIP) {
		protect_info_record *pir = &cmd_table->table.stp_cmd_table.pir;

		pir->SKIP_EN = 1;
		pir->USR_DT_SZ = MV_CPU_TO_LE16(SECTOR_SIZE >> 2);
		pir->init_block_xfer = MV_CPU_TO_LE16(req->init_block_xfer);
		pir->init_block_skip = MV_CPU_TO_LE16(req->init_block_skip);
		pir->sub_block_xfer = MV_CPU_TO_LE16(req->sub_block_xfer);
		pir->sub_block_skip = MV_CPU_TO_LE16(req->sub_block_skip);
	}
#endif
}
#ifdef SUPPORT_SECURITY_KEY_RECORDS
MV_VOID sata_fill_security_key(MV_Request *req, mv_command_header *cmd_header,
mv_command_table *cmd_table, mv_security_key_record* key_record)
{
    MV_U32 dword_0= 0;
    if (req->Cmd_Flag & CMD_FLAG_SECURITY_KEY)
        dword_0 |= CH_LOAD_SECURITY_KEY;
    if (req->Cmd_Flag & CMD_FLAG_S_KEK_POSITION)
        dword_0 |= CH_S_KEK_POSITION;
    if (req->Cmd_Flag & CMD_FLAG_VERIFY_KEY_TAG)
        dword_0 |= CH_VERIFY_KEY_TAG;
    cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(dword_0);
    cmd_header->security_key_rec_fmt=0;
}
#endif
MV_VOID sata_prepare_command(MV_PVOID root, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p, MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	ata_taskfile taskfile;
	MV_BOOLEAN ret;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_U8 tag;
#ifdef SUPPORT_SECURITY_KEY_RECORDS
    pl_root *pRoot= (pl_root *)root;
    mv_security_key_record* pkey_record;
    pkey_record = &pRoot->security_key[cmd_header->tag];
#endif

	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		sata_get_ncq_tag(device, req);
		tag = ctx->ncq_tag;
	} else {
		tag = req->Tag;
	}

	if (IS_ATAPI(device) && (req->Cdb[0] != SCSI_CMD_ATA_PASSTHRU_16)) {
		ret = atapi_fill_taskfile(device, req, &taskfile);
	} else {
		ret = ata_fill_taskfile(device, req, tag, &taskfile);
	}
	if (!ret) {
		// HARRIET_TBD
		MV_DASSERT(MV_FALSE);
	}

	sata_prepare_command_header(req, cmd_header, cmd_table, tag,
		device->pm_port);
	if (req->Cmd_Flag & CMD_FLAG_SOFT_RESET) {
		if (!taskfile.control) {    // send the second SRST FIS with reset as 0 so HW will wait until a D2H is received.
			cmd_header->ctrl_nprd &= MV_CPU_TO_LE32(~CH_RESET);
		}
	}

	sata_prepare_command_table(req, cmd_table, &taskfile, device->pm_port);
#ifdef SUPPORT_SECURITY_KEY_RECORDS
        sata_fill_security_key(req, cmd_header, cmd_table, pkey_record);
#endif
}
MV_VOID
sata_send_command(
    MV_PVOID root_p,
    MV_PVOID dev_p,
    MV_PVOID struct_wrapper,
    MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_U8 phy_map;
	MV_U32 entry_nm;
	domain_device *device = (domain_device *)dev_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
    	hw_buf_wrapper *wrapper = (hw_buf_wrapper *) struct_wrapper;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
    	if((req->Req_Flag & REQ_FLAG_BYPASS_HYBRID)){
            cmd_resource *com_res;
            if(device->register_set == NO_REGISTER_SET){
                device->register_set = core_get_register_set(root, device);
            }
            if(device->register_set == NO_REGISTER_SET){
                MV_DPRINT(("dev %d no register_set \n", device->base.id));
            }
            device->base.outstanding_req++;
            MV_ASSERT(req->Context[MODULE_HBA]);
            com_res = (cmd_resource *) req->Context[MODULE_HBA];
            entry_nm = prot_get_delv_q_entry(root, device->queue_id);

            phy_map = device->base.port->asic_phy_map; /* from Phy 0 */
            MV_ASSERT(wrapper != NULL);
            root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(
                TXQ_MODE_I |com_res->slot | device->register_set <<TXQ_REGSET_SHIFT | TXQ_CMD_STP);
#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
            root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_SRAM);
#else
            root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_PCIEA);
#endif
            root->queues[device->queue_id].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32( wrapper->phy.parts.low);
            root->queues[device->queue_id].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32( wrapper->phy.parts.high);
	//    CORE_PRINT(("Slot(%d), entry_nm(%d) addr(%p) CDB(0x%x)\n", ctx->slot,entry_nm, &root->delv_q[entry_nm],req->Cdb[0]));
            prot_write_delv_q_entry(root, entry_nm, device->queue_id);
            return;
        }
#endif
	entry_nm = prot_get_delv_q_entry(root, device->queue_id);
	phy_map = device->base.port->asic_phy_map; /* from Phy 0 */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_ASSERT(wrapper != NULL);
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	if (req->Cmd_Flag & (CMD_FLAG_SECURITY_KEY|CMD_FLAG_S_KEK_POSITION|CMD_FLAG_VERIFY_KEY_TAG))
	{
		root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(
			TXQ_MODE_I |ctx->slot | device->register_set <<TXQ_REGSET_SHIFT | TXQ_CMD_KEY_LOAD);
	}else
#endif
	{
		root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(
			TXQ_MODE_I |ctx->slot | device->register_set <<TXQ_REGSET_SHIFT | TXQ_CMD_STP);
	}
#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
	root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_SRAM);
#else
	root->queues[device->queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_PCIEA);
#endif
	root->queues[device->queue_id].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32( wrapper->phy.parts.low);
	root->queues[device->queue_id].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32( wrapper->phy.parts.high);
	//    CORE_PRINT(("Slot(%d), entry_nm(%d) addr(%p) CDB(0x%x)\n", ctx->slot,entry_nm, &root->delv_q[entry_nm],req->Cdb[0]));
#else
	root->queues[device->queue_id].delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | phy_map << TXQ_PHY_SHIFT | \
		device->register_set << TXQ_REGSET_SHIFT |TXQ_CMD_STP);
#endif

	prot_write_delv_q_entry(root, entry_nm, device->queue_id);
}

MV_VOID sata_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *device = (domain_device *)dev_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	MV_U32 error,tmp, cmpl_q = *(MV_PU32)cmpl_q_p;
	err_info_record *err_info;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
    MV_U32 i = 0;
	#ifdef CORE_EH_LOG_EVENTS
	MV_U32 params[MAX_EVENT_PARAMS];
	#endif

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info == 0) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
	} else {
		CORE_EH_PRINT(("dev %d, req %p.\n", req->Device_Id, req));
		error = MV_LE32_TO_CPU(err_info->err_info_field_1);
		req->Scsi_Status = REQ_STATUS_ERROR;
#ifdef ATHENA_PERFORMANCE_TUNNING
            if(req->Req_Flag & REQ_FLAG_BYPASS_HYBRID){
                return;
            }
#endif
		#ifdef CORE_EH_LOG_EVENTS
		{
			params[0] = CORE_EVENT_ERR_INFO;        /* CDB Field */
			params[1] = error;                      /* Sense Key */
			params[2] = req->Cdb[0];                /* Additional Sense Code */

			if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
				(req->Cdb[1] == CDB_CORE_MODULE) ) {
				params[3] = req->Cdb[2];        /* Additional Sense Code Qualifier */
			}
			else {
				MV_SetLBAandSectorCount(req);
				params[3] = req->LBA.parts.low; /* Additional Sense Code Qualifier */
			}

			core_generate_event(root->core, EVT_ID_HD_SC_ERROR, device->base.id,
			SEVERITY_INFO, 4, params, 0);
		}
		#endif

		if (error & CMD_ISS_STPD){
			ctx->error_info |= EH_INFO_CMD_ISS_STPD;
			device->base.cmd_issue_stopped = MV_TRUE;
			/* dont return */
		}

		if ((error & WD_TMR_TO_ERR) || (error & TX_STOPPED_EARLY)
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		|| ((error & R_ERR) && (error & RTRY_LMT_ERR))
#endif
		){
		#if 1 
            //Check PHY ready
            if(device->base.port && device->base.port->phy)
            {
                for(i=0;i<100;i++)
                {
#if defined(_OS_LINUX)
                    udelay(1);
#else
                    HBA_SleepMicrosecond(root->core, 1);
#endif
                    if(mv_is_phy_ready(root, device->base.port->phy))
                        break;
                }
            }
            else{
                i=100;  //mean device has been removed;
            }
            
            if(i>=100)
            {
                CORE_EH_PRINT(("PHY is not ready dev %d, req %p.\n", req->Device_Id, req));
                req->Scsi_Status = REQ_STATUS_NO_DEVICE;
                // Jira#QTS00000-2594 Debug
                CORE_EH_PRINT(("QNAP dbg : intrrupt 0x%x. i 0x%x. last_cmpl_q 0x%x. cmpl_q 0x%x\n", root->qnap_interrupt_count, (device->base.port)->qnap_i_counter, (device->base.port)->qnap_last_cmpl_q, cmpl_q));
                CORE_EH_PRINT(("QNAP dbg : CDB %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", req->Cdb[0], req->Cdb[1], req->Cdb[2], req->Cdb[3]
                                                                                                                                    , req->Cdb[4], req->Cdb[5], req->Cdb[6], req->Cdb[7]
                                                                                                                                    , req->Cdb[8], req->Cdb[9], req->Cdb[10], req->Cdb[11]
                                                                                                                                    , req->Cdb[12], req->Cdb[13], req->Cdb[14], req->Cdb[15]));
                return;
            }else{
                CORE_EH_PRINT(("PHY is ready dev %d, req %p.\n", req->Device_Id, req));
                req->Scsi_Status = REQ_STATUS_TIMEOUT;
			    #ifdef ASIC_WORKAROUND_WD_TO_RESET
			    ctx->error_info |= EH_INFO_STP_WD_TO_RETRY;
			    #endif //ASIC_WORKAROUND_WD_TO_RESET
			    ctx->error_info |= EH_INFO_NEED_RETRY;
            }
        #else
			req->Scsi_Status = REQ_STATUS_TIMEOUT;
			#ifdef ASIC_WORKAROUND_WD_TO_RESET
			ctx->error_info |= EH_INFO_STP_WD_TO_RETRY;
			#endif //ASIC_WORKAROUND_WD_TO_RESET
			ctx->error_info |= EH_INFO_NEED_RETRY;
        #endif
			return;
		}

		if (error & TFILE_ERR){
			/* handle task file error */
			sata_handle_taskfile_error(root, req);
	#if 0//def SUPPORT_ODIN
			if (req->Cdb[0] == SCSI_CMD_READ_10)
			req->Scsi_Status = REQ_STATUS_TIMEOUT;
	#endif

                        /*
                        * If the request was not translated, then report error event here
                        * else report the error event in scsi_ata_translation_callback
                        */
                        if (sat_get_org_req(req) == NULL)
                                core_generate_error_event(root->core, req);
			return;
                }
		/* for other type of errors */
		MV_ASSERT(((MV_U64 *)err_info)->value != 0);

		CORE_EH_PRINT(("attention: req %p[0x%x] unknown error 0x%08x 0x%08x! "\
			"treat as media error.\n",\
			req, req->Cdb[0],
			((MV_U64 *)err_info)->parts.high, \
			((MV_U64 *)err_info)->parts.low));

		req->Scsi_Status = REQ_STATUS_ERROR;
		ctx->error_info |= EH_INFO_NEED_RETRY;
	}
}

MV_PVOID sata_store_received_fis(pl_root *root, MV_U8 register_set, MV_U32 flag)
{
	saved_fis *fis = &root->saved_fis_area[register_set];
	/*According to SATA spec. Only for successful PIO data_in command
	 * we need get the status from E_Status of final PIO setup FIS .
	 * For all other protocol commands, will received D2H FIS for the status.*/
	if ((flag & CMD_FLAG_PIO) && (flag & CMD_FLAG_DATA_IN)) {
		fis->dw1 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_PIO_FIS(root, register_set) + 0x0);
		fis->dw2 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_PIO_FIS(root, register_set) + 0x4);
		fis->dw3 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_PIO_FIS(root, register_set) + 0x8);
		fis->dw4 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_PIO_FIS(root, register_set) + 0xC);
	} else {
		fis->dw1 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_D2H_FIS(root, register_set) + 0x0);
		fis->dw2 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_D2H_FIS(root, register_set) + 0x4);
		fis->dw3 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_D2H_FIS(root, register_set) + 0x8);
		fis->dw4 = MV_REG_READ_DWORD(root->rx_fis,
			SATA_RECEIVED_D2H_FIS(root, register_set) + 0xC);
	}
	return (MV_PVOID)fis;
}
