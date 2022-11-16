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
#include "core_util.h"
#include "core_error.h"
#include "core_protocol.h"
#include "core_manager.h"
#include "core_internal.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
MV_U16 core_get_free_intl_req_count(pl_root *root)
{
        return (root->lib_rsrc->intl_req_count);
}

#ifdef CORE_SUPPORT_LARGE_REQUEST
void core_sub_request_cmpltn_callback(MV_PVOID root_p, MV_Request *req)
{
	MV_Request *large_req;
	core_context *large_ctx;
	core_context *sub_ctx;

	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *) root->core;

	if (!req->Org_Req)
		return;

	large_req = req->Org_Req;
	large_ctx = (core_context *)large_req->Context[MODULE_CORE];
	sub_ctx = (core_context *)req->Context[MODULE_CORE];
    
	if (large_ctx == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}

	if (large_ctx->type != CORE_CONTEXT_TYPE_LARGE_REQUEST)
		MV_DASSERT(MV_FALSE);

	if (sub_ctx->type != CORE_CONTEXT_TYPE_SUB_REQUEST)
		MV_DASSERT(MV_FALSE);

	large_ctx->u.large_req.sub_req_cmplt++;

	/*
	* Let's set the status first. Ideally we need set the error to the most
	* severest level.
	*/
	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		large_req->Scsi_Status = req->Scsi_Status;
		MV_CopyMemory(
		large_req->Sense_Info_Buffer,
		req->Sense_Info_Buffer,
		MV_MIN(large_req->Sense_Info_Buffer_Length,
		req->Sense_Info_Buffer_Length));
	}

	/*
	* Current implementation is to queue the original large request back
	* into the completion queue to Complete it when all the splitted sub
	* requests are finished.
	*/
	if (large_ctx->u.large_req.sub_req_cmplt ==
		large_ctx->u.large_req.sub_req_count) {

		if (large_req->Scsi_Status == REQ_STATUS_PENDING)
			large_req->Scsi_Status = REQ_STATUS_SUCCESS;

		core_queue_completed_req(core, large_req);
	}
}

void core_construct_sub_request(core_extension *core,
        domain_device *device,
        MV_Request *large_req,
        MV_Request *sub_req,
        MV_PVOID data_buff,
        MV_U32 sector_count,
        MV_LBA lba,
        MV_U32 sg_offset)
{
	core_context *large_ctx = (core_context *)large_req->Context[MODULE_CORE];
	core_context *sub_ctx = (core_context *)sub_req->Context[MODULE_CORE];
	pl_root *root = device->base.root;

	MV_U16 sg_entry;
	MV_PVOID sg_entry_ptr;

	MV_LIST_HEAD_INIT(&sub_req->Queue_Pointer);

#ifdef _OS_LINUX
	if (large_req->Org_Req_Scmd)
		sub_req->Org_Req_Scmd = large_req->Org_Req_Scmd;
#endif
	sub_req->Device_Id              = large_req->Device_Id;
	sub_req->Req_Flag               = large_req->Req_Flag;
	sub_req->Scsi_Status            = large_req->Scsi_Status;
	sub_req->Tag                    = large_req->Tag;
	sub_req->Cmd_Initiator          = root;
	sub_req->Data_Transfer_Length   = sector_count * device->logical_sector_size;
	sub_req->Org_Req                = large_req;
	sub_req->Scratch_Buffer         = NULL;
	sub_req->pRaid_Request          = NULL;
	sub_req->Cmd_Flag               = large_req->Cmd_Flag;
	sub_req->Time_Out               = large_req->Time_Out;
	sub_req->Splited_Count          = 0;
	sub_req->LBA                    = lba;
	sub_req->Sector_Count           = sector_count;
	sub_req->Req_Flag               |= REQ_FLAG_LBA_VALID;

	sub_ctx->slot = 0;
	sub_ctx->handler = large_ctx->handler;
	sub_ctx->error_info = large_ctx->error_info;
	sub_ctx->req_type = large_ctx->req_type;

	sub_ctx->type = CORE_CONTEXT_TYPE_SUB_REQUEST;
	sub_ctx->u.sub_req.org_buff_ptr = data_buff;
	sub_req->Completion = core_sub_request_cmpltn_callback;

	if (SCSI_IS_READ(large_req->Cdb[0]))
		sub_req->Cdb[0] = SCSI_CMD_READ_10;
	else if (SCSI_IS_WRITE(large_req->Cdb[0]))
		sub_req->Cdb[0] = SCSI_CMD_WRITE_10;
	else
		MV_DASSERT(MV_FALSE);

	sg_entry = sub_req->SG_Table.Max_Entry_Count;
	sg_entry_ptr = sub_req->SG_Table.Entry_Ptr;
	sgd_table_init(&sub_req->SG_Table, sg_entry, sg_entry_ptr);
	sgdt_append_reftbl(&sub_req->SG_Table,
	                        &large_req->SG_Table,
	                        sg_offset,
	                        sub_req->Data_Transfer_Length);

	SCSI_CDB10_SET_LBA(sub_req->Cdb, lba.value);
	SCSI_CDB10_SET_SECTOR(sub_req->Cdb, sector_count);

}

MV_Request *core_split_large_request(pl_root *root,
        MV_Request *large_req)
{
	core_extension  *core = (core_extension *)root->core;
	domain_device   *device;
	core_context    *ctx;
	MV_Request      *sub_req=NULL;

	MV_LBA          lba;
	MV_PU8          data_buff;
	MV_U8           i;
	MV_U8           sub_req_count;
	MV_U32          transfer_size;
	MV_U32          max_transfer_size;
	MV_U32          sector_size;
	MV_U32          sector_count=0;
	MV_U32          sg_offset;
	MV_U32          remaining_transfer;

	device = (domain_device *)get_device_by_id(root->lib_dev,
		large_req->Device_Id, CORE_IS_ID_MAPPED(large_req), ID_IS_OS_TYPE(large_req));
	ctx = (core_context *)large_req->Context[MODULE_CORE];
	max_transfer_size = device->max_transfer_size;
	sector_size = device->logical_sector_size;
	MV_ASSERT(max_transfer_size !=0);
	MV_ASSERT(sector_size !=0);
	if (large_req->Data_Transfer_Length <= max_transfer_size)
	        return large_req;

	if ((!SCSI_IS_READ(large_req->Cdb[0])) &&
		(!SCSI_IS_WRITE(large_req->Cdb[0]))) {

		return large_req;
	}

	if (!IS_STP_OR_SATA(device) || !IS_HDD(device) ||
	        device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED) {
	        return large_req;
	}

	sub_req_count = (MV_U8) ((large_req->Data_Transfer_Length
	                        + max_transfer_size - 1)
	                                / max_transfer_size);

	if (core_get_free_intl_req_count(root) < sub_req_count)
		return NULL;

	ctx->u.large_req.sub_req_cmplt = 0;
	ctx->u.large_req.sub_req_count = sub_req_count;
	ctx->type = CORE_CONTEXT_TYPE_LARGE_REQUEST;

	if (!(large_req->Req_Flag & REQ_FLAG_LBA_VALID))
		MV_SetLBAandSectorCount(large_req);

	U64_ASSIGN_U64(lba, large_req->LBA);
	data_buff = core_map_data_buffer(large_req);
	sg_offset = 0;
	remaining_transfer = large_req->Data_Transfer_Length;

	for (i = 0; remaining_transfer > 0; i++) {
		if (remaining_transfer > max_transfer_size)
			transfer_size = max_transfer_size;
		else
			transfer_size = remaining_transfer;

		sector_count = (transfer_size + sector_size - 1) / sector_size;
		sub_req = get_intl_req_resource(root, 0);

		if (sub_req == NULL) {
			MV_DASSERT(MV_FALSE);
			core_unmap_data_buffer(large_req);
			return large_req;
		}

		core_construct_sub_request(core, device, large_req, sub_req,
		                        data_buff, sector_count, lba,
		                        sg_offset);

		lba = U64_ADD_U32(lba, sector_count);
		sg_offset += transfer_size;
		remaining_transfer -= transfer_size;
		if (data_buff != NULL)
			data_buff += transfer_size;

		core_append_request(root, sub_req);
	}

	if (i != sub_req_count) {
		CORE_DPRINT(("predicted number of split = %d actual number of \
		split = %d\n", sub_req_count, i));
	}

	MV_DASSERT(sg_offset == large_req->Data_Transfer_Length);
	MV_DASSERT(sector_count + (max_transfer_size / sector_size)
	                * (sub_req_count - 1) == large_req->Sector_Count);
	core_unmap_data_buffer(large_req);
	large_req->Scsi_Status = REQ_STATUS_PENDING;
	return (sub_req);
}
#endif /* CORE_SUPPORT_LARGE_REQUEST */
