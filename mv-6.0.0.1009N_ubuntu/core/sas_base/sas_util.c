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
#include "core_sas.h"
#include "core_internal.h"
#include "core_manager.h"
#include "com_api.h"
#include "com_u64.h"

#include "core_util.h"
#include "core_error.h"
#include "core_sat.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
MV_VOID sas_replace_org_req(MV_PVOID root_p,
	MV_Request *org_req,
	MV_Request *new_req)
{
	pl_root *root = (pl_root *) root_p;
	core_context *org_ctx;
	core_context *new_ctx;

	if (org_req == NULL || new_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}
	org_ctx = (core_context*)org_req->Context[MODULE_CORE];
	new_ctx = (core_context*)new_req->Context[MODULE_CORE];

	new_ctx->req_type = org_ctx->req_type;
	new_req->Time_Out = org_req->Time_Out;
	new_req->Org_Req = org_req;
	core_append_request(root, new_req);
}

MV_Request *sas_get_org_req(MV_Request *req)
{
	if (req == NULL) {
		MV_ASSERT(MV_FALSE);
		return NULL;
	}

	return (req->Org_Req);
}

MV_Request *sas_clear_org_req(MV_Request *req)
{
	MV_Request *org_req;

	if (req == NULL) {
		MV_ASSERT(MV_FALSE);
		return NULL;
	}

	org_req = req->Org_Req;
	req->Org_Req = NULL;

	return (org_req);
}

#if 0
MV_U32 sas_set_mode_page_caching(MV_PU8 buf, MV_U8 write_cache, MV_U8 pre_read)
{
	buf[0] = 0x08;		/* Page Code, PS = 0; */
	buf[1] = 0x12;		/* Page Length */
	if (write_cache == MV_TRUE)
		buf[2] = MV_BIT(2);
	else
		buf[2] = 0;
		buf[3] = 0;	/* Demand read/write retention priority */
		buf[4] = 0;	/* Disable pre-fetch trnasfer length (4,5) */
		buf[5] = 0;	/* all anticipatory pre-fetching is disabled */
		buf[6] = 0;	/* Minimum pre-fetch (6,7) */
		buf[7] = 0;
		buf[8] = 0;	/* Maximum pre-fetch (8,9) */
		buf[9] = 0;
		buf[10] = 0;	/* Maximum pre-fetch ceiling (10,11) */
		buf[11] = 0;
	if (pre_read == MV_TRUE)
		buf[12] = 0;    /* DRA = 0 */
	else
		buf[12] = MV_BIT(5);
		buf[13] = 0;	/* Number of cache segments */
		buf[14] = 0;	/* Cache segment size (14, 15) */
		buf[15] = 0;
		buf[16] = 0;
		buf[17] = 0;
		buf[18] = 0;
		buf[19] = 0;

	return 0x14;	/* Total page length in byte */
}

MV_Request *sas_make_mode_select_req(MV_PVOID dev_p,
	MV_U8 write_cache,
	MV_U8 pre_read,
	MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	core_context *ctx;
	MV_PU8 data_buf;
	MV_U32 page_len;

	req = get_intl_req_resource(root, 24);
	if (req == NULL)
		return NULL;

	ctx = (core_context *) req->Context[MODULE_CORE];
	req->Cdb[0] = SCSI_CMD_MODE_SELECT_6;
	req->Cdb[1] = 0x11;

	data_buf = core_map_data_buffer(req);
	page_len = sas_set_mode_page_caching(&data_buf[4],
	        write_cache, pre_read);

	/* Mode Data Length = 0 */
	/* When using the MODE SELECT command, this field is reserved */
	data_buf[0] = 0;
	data_buf[1] = 0;
	data_buf[2] = 0;
	data_buf[3] = 0;

	req->Data_Transfer_Length = page_len + 4;
	req->Cdb[4] = (MV_U8)page_len + 4;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = 0;
	req->Completion = cmpltn;

	core_unmap_data_buffer(req);
	return req;
}
#endif
#ifdef SUPPORT_PHY_POWER_MODE
MV_Request
*sas_make_powermode_mode_sense_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	MV_U8 length = MAX_MODE_PAGE_LENGTH;     

	req = get_intl_req_resource(root, length);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MODE_SENSE_6;
	req->Cdb[1] = 0x0; 
	req->Cdb[2] = 0x19; 
	req->Cdb[3] = 3;   
	req->Cdb[4] = length;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = cmpltn;
	return req;
}
#endif
MV_Request
*sas_make_mode_sense_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	MV_U8 length = MAX_MODE_PAGE_LENGTH;      /* Mode Page Cache */

	req = get_intl_req_resource(root, length);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MODE_SENSE_6;
	req->Cdb[1] = 0x08; /* disable block descriptors */
	req->Cdb[2] = 0x08; /* PC = 0, Page Code = 0x08(caching) */
	req->Cdb[3] = 0;    /* Subpage Code = 0 */
	req->Cdb[4] = length;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = cmpltn;
	return req;
}

MV_Request
*sas_make_marvell_specific_req(MV_PVOID dev_p,
        MV_U8 cmd,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = cmd;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = 0;
	req->Completion = cmpltn;
	return req;
}

/* This Log Sense is just used to check whether HDD supports Log Sense page 0x2F.
 * We'll use page 0x2F to simulate check smart status commands. */
MV_Request *sas_make_log_sense_req(MV_PVOID dev_p,
	MV_U8 page,
	MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	MV_U16 buf_size = MAX_MODE_LOG_LENGTH;

	req = get_intl_req_resource(root, buf_size);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_LOG_SENSE;
	req->Cdb[1] = 0;                        /* SP=0 PPC=0 */
	req->Cdb[2] = 0x40 | (page & 0x3F);     /* PC=01b */
	req->Cdb[7] = (buf_size >> 8) & 0xFF;
	req->Cdb[8] = buf_size & 0xFF;
	req->Cdb[9] = 0;                       /* Control */

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = cmpltn;
	return req;
}

MV_Request *sas_make_sync_cache_req(MV_PVOID dev_p,
	MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_SYNCHRONIZE_CACHE_10;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = 0;
	req->Completion = cmpltn;
	return req;
}


MV_U8 ssp_ata_parse_log_sense_threshold_exceed(MV_PU8 data_buf, MV_U32 length)
{
	if (data_buf == NULL || length < 9) {
		MV_DASSERT(MV_FALSE);
		return MV_FALSE;
	}

	if (data_buf[8] == SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_VOID ssp_ata_parse_smart_return_status(domain_device *dev,
	MV_Request *org_req, MV_Request *new_req)
{
	pl_root *root = dev->base.root;
	core_extension *core = (core_extension *)root->core;
	HD_SMART_Status *status;
	MV_PVOID new_buf_ptr;
	MV_U8 ret;

	if (org_req == NULL || new_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}

	new_buf_ptr = core_map_data_buffer(new_req);
	ret = ssp_ata_parse_log_sense_threshold_exceed(
	new_buf_ptr,
	new_req->Data_Transfer_Length);

	status = core_map_data_buffer(org_req);

	if (status != NULL)
		status->SmartThresholdExceeded = ret;

	if (ret == MV_TRUE)
		core_generate_event(core, EVT_ID_HD_SMART_THRESHOLD_OVER,
		dev->base.id, SEVERITY_WARNING, 0, NULL ,0);

	core_unmap_data_buffer(org_req);
	core_unmap_data_buffer(new_req);
}

MV_Request *sas_make_inquiry_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_BOOLEAN EVPD, MV_U8 page, MV_ReqCompletion completion)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	PMV_Request req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0x60);
	if (req == NULL)
		return NULL;

	/* Prepare inquiry */
	if (!EVPD) {
		req->Cdb[0] = SCSI_CMD_INQUIRY;
		req->Cdb[1] = 0;
		req->Cdb[4] = 0x60;
	} else {
		req->Cdb[0] = SCSI_CMD_INQUIRY;
		req->Cdb[1] = 0x01; /* EVPD inquiry */
		req->Cdb[2] = page;
		req->Cdb[4] = 0x60;
	}
	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = completion;
	return req;
}

#ifdef SUPPORT_MUL_LUN
MV_Request *sas_make_report_lun_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_ReqCompletion completion)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	PMV_Request req;
	MV_U16 buf_size = 0x400;
	
	/* allocate resource */
	req = get_intl_req_resource(root, buf_size);
	if (req == NULL)
		return NULL;
	
	req->Cdb[0] = SCSI_CMD_REPORT_LUN;
	req->Cdb[2] = 0;
	/*Cdb[6..9] means the allocation length, and it equals to buf_size */
	req->Cdb[8] = 0x4;
	req->Cdb[9] = 0x00;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = completion;
	return req;
}
#endif

MV_Request *
ssp_make_virtual_phy_reset_req(MV_PVOID dev_p,
	MV_U8 operation, MV_ReqCompletion callback)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);

	if (req == NULL) return NULL;

	MV_ASSERT(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SSP_VIRTUAL_PHY_RESET;
	req->Cdb[3] = operation;
	req->Completion = callback;
	return req;
}


MV_VOID
stp_req_report_phy_sata_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_Request *org_req = ctx->u.org.org_req;
	core_context *org_req_ctx = (core_context *)org_req->Context[MODULE_CORE];
	domain_expander *exp = (domain_expander *)exp_p;
	domain_device *device = NULL;
	smp_response *smp_resp = NULL;

	if (exp) {
		if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
			smp_resp = (smp_response *)core_map_data_buffer(req);
			if (smp_resp != NULL) {
				/* find the device that this request was intended for */
				LIST_FOR_EACH_ENTRY_TYPE(device,
				&exp->device_list, domain_device,
				base.exp_queue_pointer) {
					if (device->parent_phy_id ==
						smp_resp->response.ReportPhySATA.PhyIdentifier)
						break;
				}

				if (device == NULL) {
					MV_ASSERT(MV_FALSE);
					return;
				}

				switch (device->state) {
				case DEVICE_STATE_STP_REPORT_PHY:
	#ifdef DEBUG_EXPANDER
					CORE_DPRINT(("Check device %d phy %d AffiliationValid %X :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,smp_resp->response.ReportPhySATA.AffiliationValid));
					CORE_DPRINT(("Check device %d phy %d AffiliationSupported %X :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,smp_resp->response.ReportPhySATA.AffiliationSupported));
					CORE_DPRINT(("Check device %d phy %d STPNexusLossOccurred %X :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,smp_resp->response.ReportPhySATA.STPNexusLossOccurred));
					CORE_DPRINT(("Check device %d phy %d STPSASAddress %016llX :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,(MV_U64)(*(MV_U64 *)smp_resp->response.ReportPhySATA.STPSASAddress)));
					CORE_DPRINT(("Check device %d phy %d AffiliatedSTPInitiatorSASAddress %016llX :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,(MV_U64)(*(MV_U64 *)smp_resp->response.ReportPhySATA.AffiliatedSTPInitiatorSASAddress)));
					CORE_DPRINT(("Check device %d phy %d STPNexusLossSASAddress %016llX :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,(MV_U64)(*(MV_U64 *)smp_resp->response.ReportPhySATA.STPNexusLossSASAddress)));
					CORE_DPRINT(("Check device %d phy %d fis %016llX :\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier,(MV_U64)(*(MV_U64 *)&smp_resp->response.ReportPhySATA.fis)));
	#endif
					if (smp_resp->response.ReportPhySATA.AffiliationValid) {
						CORE_DPRINT(("Find device %d phy %d has affiliation_valid.\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier));
						org_req_ctx->u.smp_report_phy.affiliation_valid = 1;
					}
					else {
						device->signature =
						smp_resp->response.ReportPhySATA.fis.lba_high << 24 |
						smp_resp->response.ReportPhySATA.fis.lba_mid << 16 |
						smp_resp->response.ReportPhySATA.fis.lba_low << 8 |
						smp_resp->response.ReportPhySATA.fis.sector_count;
					}
					break;
				default:
					/* Nothing to handle */
					break;
				}
			}
			core_unmap_data_buffer(req);
		}
	}

	core_queue_completed_req(root->core, org_req);
}

MV_VOID stp_req_disc_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base;
	domain_device *device = NULL;
	core_context *ctx = NULL;

	base = (struct _domain_base *)get_device_by_id(root->lib_dev,
					req->Device_Id, CORE_IS_ID_MAPPED(req), MV_FALSE);
	if (!base)
		return;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, base, req);
		return;
	}

	MV_DASSERT(base->type == BASE_TYPE_DOMAIN_DEVICE);
	device = (domain_device *)base;
	ctx = (core_context *)req->Context[MODULE_CORE];

	switch (device->state) {
	case DEVICE_STATE_STP_RESET_PHY:
		device->state = DEVICE_STATE_STP_REPORT_PHY;
		//Work around: STP device need to delay time after HARD_RESET
		core_sleep_millisecond(root->core, 100);
		break;
	case DEVICE_STATE_STP_REPORT_PHY:
		if (ctx->u.smp_report_phy.affiliation_valid == 1) {
			device->state = DEVICE_STATE_STP_RESET_PHY;
		}
		else {
#ifdef SKIP_INTERNAL_INITIALIZE
			device->state = DEVICE_STATE_SKIP_INIT_DONE;
#else
			device->state = DEVICE_STATE_RESET_DONE;
#endif
		}
		break;
	}

	core_queue_init_entry(root, base, MV_FALSE);
}

MV_Request *stp_make_report_phy_sata_req(domain_device *dev, MV_ReqCompletion callback)
{
	pl_root *root = dev->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req == NULL)
		return NULL;

	MV_DASSERT(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_STP_VIRTUAL_REPORT_SATA_PHY;
	req->Completion = callback;
	ctx->u.smp_report_phy.affiliation_valid = 0;

	return req;
}

MV_Request *stp_make_phy_reset_req(domain_device *device, MV_U8 operation,
	MV_ReqCompletion callback)
{
	pl_root *root = device->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);

	if (req == NULL) {
		return NULL;
	}

	MV_DASSERT(device->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = device->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_STP_VIRTUAL_PHY_RESET;
	req->Cdb[3] = operation;
	req->Completion = callback;
	return req;
}

#ifdef SUPPORT_MUL_LUN
void mv_int_to_reqlun(MV_U16 lun, MV_U8*reqlun)
{
	reqlun[1] = lun & 0xFF;
	reqlun[0] = (lun >> 8) & 0xFF;
	reqlun[2] = 0;
	reqlun[3] = 0;
}
#endif

#ifdef SUPPORT_HYPERIO
MV_Request
*sas_make_hyper_info_req(MV_PVOID dev_p, MV_U8 code, MV_U32 len, MV_U8 in_out,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	//MV_U32 length = MAX_PACKET_COMMAND_BUFFER_SIZE; 

	req = get_intl_req_resource(root, len);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = code;
	req->Cdb[4] = (MV_U8)(len>>24);
	req->Cdb[5] = (MV_U8)((len>>16) & 0xff);
	req->Cdb[6] = (MV_U8)((len>>8) & 0xff);
	req->Cdb[7] = (MV_U8)(len & 0xff);

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = (in_out ? CMD_FLAG_DATA_OUT : CMD_FLAG_DATA_IN); /*0:in, 1:out*/
	req->Completion = cmpltn;

	return req;
}
#endif
