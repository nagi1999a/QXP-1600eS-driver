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
#include "com_error.h"
#include "com_dbg.h"

#include "core_util.h"
#include "core_error.h"
#include "core_sat.h"
#include "core_console.h"
#include "core_expander.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
//JING TBD: remove this definition
#define IS_A_SMP_REQ(pReq) \
	((pReq->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC)&& \
	 (pReq->Cdb[1]==CDB_CORE_MODULE)&& \
	 (pReq->Cdb[2]==CDB_CORE_SMP))

extern MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req);
extern MV_VOID ssp_ata_intl_req_callback(MV_PVOID root_p, MV_Request *req);
extern MV_U8 ssp_ata_parse_smart_return_status(domain_device *device,
	MV_Request *org_req, MV_Request *new_req);
extern MV_Request *smp_make_discover_req(domain_expander *exp,
	MV_Request *vir_req, MV_U8 phy_id, MV_PVOID callback);
extern MV_VOID smp_req_discover_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_VOID smp_req_reset_sata_phy_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_VOID smp_req_clear_aff_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_Request *smp_make_config_route_req(domain_expander *exp,
	MV_Request *vir_req, MV_U16 route_index, MV_U8 phy_id, MV_U64 *sas_addr,
	MV_PVOID callback);
extern MV_VOID smp_req_config_route_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_Request *smp_make_phy_crtl_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_U8 operation, MV_PVOID callback);
extern MV_Request *smp_make_report_phy_sata_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_PVOID callback);
extern domain_base *exp_search_phy(domain_expander *exp, MV_U8 phy_id);
MV_VOID ssp_ata_fill_inquiry_device(domain_device *dev, MV_Request *req);
MV_VOID ssp_mode_page_rmw_callback(MV_PVOID root_p, MV_Request *req);
#ifdef SUPPORT_SG_RESET
MV_VOID ssp_hba_reset_callback(MV_PVOID root_p, MV_Request *req);
extern MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
        MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);
#endif
extern MV_VOID smp_physical_req_callback(MV_PVOID root_p, MV_Request *req);

#ifdef SUPPORT_HYPERIO
MV_VOID ssp_hyper_info_req_callback(MV_PVOID root_p, MV_Request *req);

void translate_compact_cmd_buffer(IN PMV_Request org_req, IN PMV_Request req)
{
    consolidate_rc_cmd_t *rc_cmd = (consolidate_rc_cmd_t *)((char*)org_req->SG_Table.Entry_Ptr + RCC_PAYLOAD_SIZE);
    MV_U32 i = 0, sub_cmd_cnt = org_req->Cdb[4], pkt_len = 0, lba_len = 0;
    hyperIO_packet_header *pkt_header = req->Data_Buffer;
    mv_hyperIO_subcmd *sub_cmd;
    MV_U8 sg_cnt = 0;

    pkt_len = PACKET_HEADER_SIZE;
    sub_cmd = (mv_hyperIO_subcmd *)(((MV_U8 *)pkt_header) + pkt_len);

    if (SCSI_IS_CDB10(org_req->Cdb[0])) {
        for (i = 1; i <= sub_cmd_cnt; i++) {
            sub_cmd->cmd.sub_cmd10.L_bit = 0;
            sub_cmd->cmd.sub_cmd10.sector0 = (MV_U8)(rc_cmd->scatter_8[-i].len & 0xFF);
            sub_cmd->cmd.sub_cmd10.sector1 = (MV_U8)((rc_cmd->scatter_8[-i].len >> 8) & 0xFF);
            sub_cmd->cmd.sub_cmd10.lba0 = (MV_U8)((rc_cmd->scatter_8[-i].lba) & 0xFF);
            sub_cmd->cmd.sub_cmd10.lba1 = (MV_U8)((rc_cmd->scatter_8[-i].lba >> 8) & 0xFF);
            sub_cmd->cmd.sub_cmd10.lba2 = (MV_U8)((rc_cmd->scatter_8[-i].lba >> 16) & 0xFF);
            sub_cmd->cmd.sub_cmd10.lba3 = (MV_U8)((rc_cmd->scatter_8[-i].lba >> 24) & 0xFF);
            pkt_len += sizeof(hyperIO_sub_cmd10);
            lba_len += (MV_U32)rc_cmd->scatter_8[-i].len;
            sg_cnt += rc_cmd->scatter_8[-i].sg_cnt;
            sub_cmd = (mv_hyperIO_subcmd *)(((MV_U8 *)pkt_header) + pkt_len);
        }
    }

    pkt_header->pkt_len0 = (MV_U8)(pkt_len & 0xFF);
    pkt_header->pkt_len1 = (MV_U8)((pkt_len >> 8) & 0xFF);
    pkt_header->pkt_len2 = (MV_U8)((pkt_len >> 16) & 0xFF);
    pkt_header->pkt_len3 = (MV_U8)((pkt_len >> 24) & 0xFF);
    pkt_header->sub_cmd_counts_low = (MV_U8)(sub_cmd_cnt & 0xFF);
    pkt_header->sub_cmd_counts_high = (MV_U8)((sub_cmd_cnt >> 8) & 0xFF);
    pkt_header->data_block0 = (MV_U8)((lba_len) & 0xFF);
    pkt_header->data_block1 = (MV_U8)((lba_len >> 8) & 0xFF);
    pkt_header->data_block2 = (MV_U8)((lba_len >> 16) & 0xFF);
    pkt_header->data_block3 = (MV_U8)((lba_len >> 24) & 0xFF);

}

MV_VOID fill_hyperIO_command(MV_PVOID dev_p, MV_U8 code, 
	MV_Request *req, MV_Request *org_req)
{
    domain_device *dev = (domain_device *)dev_p;
    pl_root *root = dev->base.root;
    core_extension *core = (core_extension *)root->core;
    MV_U32 len = 0; 
    MV_U32 cmd_flag = 0x0; 
    core_context *ctx = req->Context[MODULE_CORE];
    MV_U32 i = 0, j = 0, pkt_len = 0, lba_len = 0;
    MV_U8 sg_cnt = 0, sub_cmd_cnt;
    hw_buf_wrapper *wrapper = NULL;
    MV_U32 address, address_hi;
    MV_U8 *buf;

    if (SCSI_IS_PACKET(code)) {
        //MV_U32 block = org_req->Data_Transfer_Length >> 9;

        sub_cmd_cnt = org_req->Cdb[4];
        cmd_flag |= CMD_FLAG_DATA_OUT;

        if(SCSI_IS_CDB10(org_req->Cdb[0]))
            len = sub_cmd_cnt * sizeof(hyperIO_sub_cmd10) + PACKET_HEADER_SIZE;
        else
            len = sub_cmd_cnt * sizeof(hyperIO_sub_cmd16) + PACKET_HEADER_SIZE;

        #if 0
        MV_DASSERT(len <= SCRATCH_BUFFER_SIZE);
        if (len != 0) {
            wrapper = get_scratch_buf(root->lib_rsrc);
            if (wrapper == NULL) {
                return NULL;
            }
            ctx->buf_wrapper = wrapper;
            req->Data_Buffer = wrapper->vir;
        } else {
            ctx->buf_wrapper = NULL;
            req->Data_Buffer = NULL;
            FM_PRINT("%s MV_QUEUE_COMMAND_RESULT_NO_RESOURCE!!", __FUNCTION__);
            return NULL;
        }
        
        translate_compact_cmd_buffer(org_req, req);

        req->Data_Transfer_Length = len;
		SGTable_Append(
			&req->SG_Table,
			wrapper->phy.parts.low,
			wrapper->phy.parts.high,
			len);
        #else
        req->Data_Transfer_Length = len;
        address = (MV_U32)((char*)org_req->SG_Table.Entry_Ptr + RCC_PACKET_CMD_OFFSET);
        buf = (MV_U8 *)org_req->SG_Table.Entry_Ptr + RCC_PACKET_CMD_OFFSET;
        #if 0
        FM_PRINT("Packet header\n");
        for (i = 0; i < PACKET_HEADER_SIZE; i++) {
            FM_PRINT("%x ", *buf++);
        }
        FM_PRINT("\n");
        for (i = 0; i < 32; i++) {
            FM_PRINT("Cmd %d\n", i);
            for (j = 0; j < 8; j++) {
                FM_PRINT("%x ", *buf++);
            }
            FM_PRINT("\n");
        }
        #endif
        
		SGTable_Append(
			&req->SG_Table,
			address,
			0,
			len);
        
        #endif
    } else {
        len = req->Data_Transfer_Length >> 9;
        cmd_flag |= ((org_req->Cmd_Flag & CMD_FLAG_DATA_OUT) ? 
                    CMD_FLAG_DATA_OUT : CMD_FLAG_DATA_IN);
        //MV_CopySGTable(&req->SG_Table, &org_req->SG_Table);
        memcpy(&ctx->u, &req->SG_Table, sizeof(MV_SG_Table));
        req->SG_Table.Max_Entry_Count = org_req->SG_Table.Max_Entry_Count;
        req->SG_Table.Valid_Entry_Count = org_req->SG_Table.Valid_Entry_Count;
        req->SG_Table.Occupy_Entry_Count = org_req->SG_Table.Occupy_Entry_Count;
        req->SG_Table.Flag = org_req->SG_Table.Flag;
        req->SG_Table.Byte_Count = org_req->SG_Table.Byte_Count;
        req->SG_Table.prdt_bus_addr.parts.high = org_req->SG_Table.prdt_bus_addr.parts.high;
        req->SG_Table.prdt_bus_addr.parts.low = org_req->SG_Table.prdt_bus_addr.parts.low;
        req->SG_Table.Entry_Ptr = org_req->SG_Table.Entry_Ptr;	//borrow the original req SG entry.
    }

	req->Cdb[0] = code;
	req->Cdb[4] = (MV_U8)(len>>24);
	req->Cdb[5] = (MV_U8)((len>>16) & 0xff);
	req->Cdb[6] = (MV_U8)((len>>8) & 0xff);
	req->Cdb[7] = (MV_U8)(len & 0xff);

	req->Device_Id = dev->base.id;
	//CORE_DPRINT(("req:%x to device:%d\n", code, req->Device_Id));
	req->Cmd_Flag = cmd_flag;
	req->Completion = ssp_hyper_info_req_callback;
}

MV_VOID fill_packet_command_data_buf(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req, MV_Request *org_req)
{
	MV_PU8 buf_ptr, tmp;
	MV_U32 sector_count = org_req->Cdb[5] + org_req->Cdb[6];
	core_context *ctx = req->Context[MODULE_CORE];
	/*need define the packet header & sub-command structure*/
	/*below code for test*/
	/*
	org_req: CDB: 0xe1, 0x50, 0/1, lba0, lba1, s_cnt0, s_cnt1,
	note:lba can't exceed 0xff
	*/
	buf_ptr = core_map_data_buffer(req);
	MV_ZeroMemory(buf_ptr, req->Data_Transfer_Length);
	
	/*packet header*/
	buf_ptr[0] = (MV_U8)(req->Data_Transfer_Length>>24);
	buf_ptr[1] = (MV_U8)((req->Data_Transfer_Length>>16) & 0xff);
	buf_ptr[2] = (MV_U8)((req->Data_Transfer_Length>>8) & 0xff);
	buf_ptr[3] = (MV_U8)(req->Data_Transfer_Length & 0xff);

	//buf_ptr[4] = 0;
	buf_ptr[5] = 2;
	
	//buf_ptr[6] = (MV_U8)((ctx->slot >> 8) & 0xff);
	//buf_ptr[7] = (MV_U8)(ctx->slot & 0xff);
	
	buf_ptr[8] = (MV_U8)(sector_count>>24);
	buf_ptr[9] = (MV_U8)((sector_count>>16) & 0xff);
	buf_ptr[10] = (MV_U8)((sector_count>>8) & 0xff);
	buf_ptr[11] = (MV_U8)(sector_count & 0xff);

	/*sub-commands*/
	tmp = &buf_ptr[16];
	/*cmd1*/
	tmp[0] = 0;
	tmp[2] = (MV_U8)((org_req->Cdb[5]>>8) & 0xff);
	tmp[3] = (MV_U8)(org_req->Cdb[5] & 0xff);
	
	tmp[4] = (MV_U8)(org_req->Cdb[3]>>24);
	tmp[5] = (MV_U8)((org_req->Cdb[3]>>16) & 0xff);
	tmp[6] = (MV_U8)((org_req->Cdb[3]>>8) & 0xff);
	tmp[7] = (MV_U8)(org_req->Cdb[3] & 0xff);

	tmp = &buf_ptr[24];
	tmp[0] = 0;
	tmp[2] = (MV_U8)((org_req->Cdb[6]>>8) & 0xff);
	tmp[3] = (MV_U8)(org_req->Cdb[6] & 0xff);
	
	tmp[4] = (MV_U8)(org_req->Cdb[4]>>24);
	tmp[5] = (MV_U8)((org_req->Cdb[4]>>16) & 0xff);
	tmp[6] = (MV_U8)((org_req->Cdb[4]>>8) & 0xff);
	tmp[7] = (MV_U8)(org_req->Cdb[4] & 0xff);
	
	core_unmap_data_buffer(req);
}

MV_VOID
ssp_hyper_info_req_callback(MV_PVOID root_p, MV_Request *req)
{
    pl_root *root = (pl_root *)root_p;
    MV_Request *org_req, *new_req;
    domain_device *dev;
    MV_U32 length;
    //MV_PU8 org_buf_ptr, new_buf_ptr;
    MV_U16 rcc_tag;
    core_context *ctx = (core_context *)req->Context[MODULE_CORE];

    org_req = sas_clear_org_req(req);

    if (org_req == NULL) {
        MV_ASSERT(MV_FALSE);
        return;
    }

    dev = (domain_device *)get_device_by_id(root->lib_dev,
	    org_req->Device_Id, MV_FALSE, MV_FALSE);

    if (req->Cdb[0] == SCSI_CMD_PACKET) {
        hw_buf_wrapper *wrapper;
        wrapper = ctx->buf_wrapper;
        if (wrapper) {
            free_scratch_buf(root->lib_rsrc, wrapper);
            ctx->buf_wrapper = NULL;
        }
    }
    
    if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
        length = MV_MIN(req->Sense_Info_Buffer_Length,
        org_req->Sense_Info_Buffer_Length);

        MV_CopyMemory(org_req->Sense_Info_Buffer,
        req->Sense_Info_Buffer,
        length);

        Tag_ReleaseOne(&root->slot_pool, (((MV_U16)req->Cdb[2]) << 8) | (((MV_U16)req->Cdb[3])));
        org_req->Scsi_Status = req->Scsi_Status;
        core_queue_completed_req(root->core, org_req);
        return;
    }

    //org_buf_ptr = core_map_data_buffer(req);


    if (req->Cdb[0] == SCSI_CMD_PACKET) {
        rcc_tag = (((MV_U16)(req->Cdb[2]) << 8) | ((MV_U16)req->Cdb[3]));

        switch (org_req->Cdb[0]) {
            case SCSI_MARVELL_CMD_RCC_READ_8:
                //buf_ptr_32 = (MV_PU32)(&org_buf_ptr[8]);
                new_req = get_intl_req_resource(root, 0);		/*use org_req data buf*/	
                if (new_req == NULL) {
                    if (req->Sense_Info_Buffer != NULL)
                        ((MV_PU8)org_req->Sense_Info_Buffer)[0] = ERR_NO_RESOURCE;

                    org_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                    Tag_ReleaseOne(&root->slot_pool, rcc_tag);
                    //core_unmap_data_buffer(req);
                    core_queue_completed_req(root->core, org_req);
                    return;
                }

                new_req->Data_Buffer = 0;//org_req->Data_Buffer;
                new_req->Data_Transfer_Length = org_req->Data_Transfer_Length;
                //MV_DASSERT(length <= req->SG_Table.Byte_Count);

                fill_hyperIO_command(dev, SCSI_CMD_PACKET_READ, new_req, org_req);
                new_req->Cdb[2] = req->Cdb[2] ;
                new_req->Cdb[3] = req->Cdb[3] ; 

                //core_unmap_data_buffer(req);
                sas_replace_org_req(root, org_req, new_req);
                break;

            case SCSI_MARVELL_CMD_RCC_WRITE_8:
                new_req = get_intl_req_resource(root, 0);		/*use org_req data buf*/	
                if (new_req == NULL) {
                    if (req->Sense_Info_Buffer != NULL)
                        ((MV_PU8)org_req->Sense_Info_Buffer)[0] = ERR_NO_RESOURCE;

                    org_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                    Tag_ReleaseOne(&root->slot_pool, rcc_tag);
                    //core_unmap_data_buffer(req);
                    core_queue_completed_req(root->core, org_req);
                    return;
                }

                new_req->Data_Buffer = 0;//org_req->Data_Buffer;
                new_req->Data_Transfer_Length = org_req->Data_Transfer_Length;
                //MV_DASSERT(length <= req->SG_Table.Byte_Count);

                fill_hyperIO_command(dev, SCSI_CMD_PACKET_WRITE, new_req, org_req);
                new_req->Cdb[2] = req->Cdb[2] ;
                new_req->Cdb[3] = req->Cdb[3] ; 

                //core_unmap_data_buffer(req);
                sas_replace_org_req(root, org_req, new_req);
                break;

            default:
                MV_DASSERT(MV_FALSE);
                break;
        }
    } else {
        switch (req->Cdb[0]) {
            case SCSI_CMD_PACKET_READ:
                CORE_DPRINT(("read data buf len: %x\n", req->Data_Transfer_Length));
                org_req->Scsi_Status = req->Scsi_Status;
                MV_CopyMemory(&req->SG_Table, &ctx->u, sizeof(MV_SG_Table));	//we borrow the original req sg_table
                core_queue_completed_req(root->core, org_req);
                //core_unmap_data_buffer(req);
                return;
                break;

            case SCSI_CMD_PACKET_WRITE:
                CORE_DPRINT(("read data buf len: %x\n", req->Data_Transfer_Length));
                org_req->Scsi_Status = req->Scsi_Status;
                MV_CopyMemory(&req->SG_Table, &ctx->u, sizeof(MV_SG_Table));	//we borrow the original req sg_table
                core_queue_completed_req(root->core, org_req);
                //core_unmap_data_buffer(req);
                return;
                break;

            default:
                MV_DASSERT(MV_FALSE);
                break;
            }
        }

        //core_unmap_data_buffer(req);

}

#endif //SUPPORT_HYPERIO

MV_U8 ssp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	MV_QUEUE_COMMAND_RESULT result = MV_QUEUE_COMMAND_RESULT_PASSED;
	MV_Request *new_req;
	MV_U8 ret;
	HD_SMART_Status *status;

	/* tempature disable shutdown command */
	if (req->Cdb[0] == APICDB0_ADAPTER){
		/* skip adapter commands */
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

#ifdef SCSI_PORT_TRIM_SUPPORT
	/* tempature return success for Trim command on SAS */
	if (req->Cdb[0] == SCSI_CMD_DEALLOCATED){
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if ((req->Cdb[0] == SCSI_CMD_VERIFY_16) && (req->Req_Flag & REQ_FLAG_TRIM_CMD)){
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif

	/* power change need longer time to finish */
	if (req->Cdb[0] == SCSI_CMD_START_STOP_UNIT)
		req->Time_Out = 60;
	/*	if (((IS_TAPE(dev)) &&
		dev->base.outstanding_req > 0 ) ||
		(dev->base.outstanding_req >= dev->base.queue_depth) ){
		return MV_QUEUE_COMMAND_RESULT_FULL;
	} */
#ifdef SUPPORT_SG_RESET
	if (CORE_IS_HBA_RESET_REQ(req))
	{
		switch(req->Cdb[2]){
		case CDB_HBA_DEVICE_RESET:
			new_req = sas_make_task_req(root, dev, req, req->Cdb[5],
				ssp_hba_reset_callback);

			if (new_req == NULL)
			{
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE; 
			}
			
	        sas_replace_org_req(root, req, new_req);
		    return MV_QUEUE_COMMAND_RESULT_REPLACED;				
		
		case CDB_HBA_BUS_RESET:
			mv_reset_phy(root,dev->base.port->phy_map, MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		default:
			 req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			 return MV_QUEUE_COMMAND_RESULT_FINISHED;		
		}
	}
#endif

    #ifdef SUPPORT_HYPERIO
	if (SCSI_IS_COMPACT(req->Cdb[0])) {
		//MV_PRINT("receive hyper IO, sub_counts:%d!!!\n", req->Cdb[4]);
		new_req = get_intl_req_resource(root, 0);
	
		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		fill_hyperIO_command(dev, SCSI_CMD_PACKET, new_req, req);
	
		sas_replace_org_req(root, req, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
	}
    #endif
	
	/* check core internal requests */
	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE)) {

		/* by default is not supported unless clearly expressed */
		switch (req->Cdb[2]) {
		case CDB_CORE_TASK_MGMT:
			break;
		case CDB_CORE_RESET_DEVICE:
			mv_reset_phy(root, req->Cdb[3], MV_FALSE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_RESET_PORT:
			mv_reset_phy(root, req->Cdb[3], MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_ENABLE_SMART:
			if (!(dev->capability &
				DEVICE_CAPABILITY_SMART_SUPPORTED))
				CORE_DPRINT(("Device %d does not "\
				"support SMART, but returning "\
				"REQ_STATUS_SUCCESS anyway\n",\
				dev->base.id));
			else
				dev->setting |= DEVICE_SETTING_SMART_ENABLED;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_DISABLE_SMART:
			dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_ENABLE_WRITE_CACHE:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_DISABLE_WRITE_CACHE:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;
#ifdef SUPPORT_PHY_POWER_MODE			
		case CDB_CORE_CHECK_POWER_MODE:
		case CDB_CORE_READ_POWER_MODE:
			new_req = sas_make_powermode_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);
			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;
#endif
		case CDB_CORE_ENABLE_READ_AHEAD:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);
			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_DISABLE_READ_AHEAD:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_SMART_RETURN_STATUS:
		if (!(dev->capability &
			DEVICE_CAPABILITY_SMART_SUPPORTED)) {
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			status = core_map_data_buffer(req);
			if (status != NULL)
				status->SmartThresholdExceeded = MV_FALSE;
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		/* Page=2F(Informal Exceptions Log) */
		new_req = sas_make_log_sense_req(dev,
		0x2F,
		ssp_ata_intl_req_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		sas_replace_org_req(root, req, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

#if 0
		/* ikkwong: disabled for now */
		case CDB_CORE_SHUTDOWN:
		new_req = sas_make_sync_cache_req(dev,
		                ssp_ata_intl_req_callback);

		if (new_req == NULL)
		        return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		sas_replace_org_req(root, req, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
		break;
#endif
		case CDB_CORE_OS_SMART_CMD:
                        if (req->Cdb[3] == ATA_CMD_IDENTIFY_ATA) {

                                ssp_ata_fill_inquiry_device(dev, req);
                                req->Scsi_Status = REQ_STATUS_SUCCESS;
			        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        if (!(dev->capability &
                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

			        req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        switch (req->Cdb[4]) {
                        case ATA_CMD_ENABLE_SMART:
                                if (!(dev->capability &
                                        DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                        CORE_DPRINT(("Device %d does "\
                                                "not support SMART, but "\
                                                "returning REQ_STATUS_SUCCESS"\
                                                " anyway\n", dev->base.id));
                                } else if (!(dev->setting &
                                        DEVICE_SETTING_SMART_ENABLED)) {

                                        dev->setting |=
                                                DEVICE_SETTING_SMART_ENABLED;
                                }

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;

                        case ATA_CMD_DISABLE_SMART:
                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;

                        case ATA_CMD_SMART_RETURN_STATUS:
                                if (!(dev->capability &
                                        DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                                        status = (HD_SMART_Status *)
                                                core_map_data_buffer(req);
                                        if (status != NULL)
                                                status->SmartThresholdExceeded
                                                        = MV_FALSE;
                                        core_unmap_data_buffer(req);
                                        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                                }

                                /* Page=2F(Informal Exceptions Log) */
                                new_req = sas_make_log_sense_req(dev,
                                                0x2F,
                                                ssp_ata_intl_req_callback);

                                ret = MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
                                if (new_req == NULL)
                                        return ret;

                                sas_replace_org_req(root, req, new_req);
                                return MV_QUEUE_COMMAND_RESULT_REPLACED;

                        case ATA_CMD_SMART_READ_DATA:
                        case ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE:
                        case ATA_CMD_SMART_EXECUTE_OFFLINE:
                        case ATA_CMD_SMART_READ_LOG:
                        case ATA_CMD_SMART_WRITE_LOG:

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        break;
		case CDB_CORE_SSP_VIRTUAL_PHY_RESET:
			new_req = smp_make_phy_control_req(dev, req->Cdb[3],
				smp_physical_req_callback);
			if (!new_req) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			((core_context *)new_req->Context[MODULE_CORE])->u.org.org_req = req;
			core_queue_eh_req(root, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;
		default:
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

	}

	/* pass down all the SCSI requests including inquiry, read capacity... */
	switch (req->Cdb[0]) {
#ifdef SUPPORT_PASS_THROUGH_DIRECT
	case APICDB0_PASS_THRU_CMD_SCSI:
	case APICDB0_PASS_THRU_CMD_ATA:
		//MV_ASSERT(MV_FALSE);
		result = core_pass_thru_send_command(root->core, req);
		break;
#endif /* CORE_SUPPORT_API */
	case APICDB0_PD:
	/* Currently RAID module will send only these 2 API requests */
	switch (req->Cdb[1]) {
	case APICDB1_PD_SETSETTING:
		switch (req->Cdb[4]) {
		case APICDB4_PD_SET_WRITE_CACHE_OFF:
                        if (!(dev->setting &
                                DEVICE_SETTING_WRITECACHE_ENABLED)) {

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        new_req = sas_make_mode_sense_req(dev,
                                ssp_mode_page_rmw_callback);

                        if (new_req == NULL)
                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

                        sas_replace_org_req(root, req, new_req);
                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

                case APICDB4_PD_SET_WRITE_CACHE_ON:
                        if (dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED) {
                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        new_req = sas_make_mode_sense_req(dev,
                                ssp_mode_page_rmw_callback);

                        if (new_req == NULL)
                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

                        sas_replace_org_req(root, req, new_req);
                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

                case APICDB4_PD_SET_SMART_OFF:
                        if (dev->setting & DEVICE_SETTING_SMART_ENABLED) {
                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
                        }

                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

                case APICDB4_PD_SET_SMART_ON:
                        if (!(dev->capability &
                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                CORE_DPRINT(("Device %d does not "\
                                        "support SMART, but returning "\
                                        "REQ_STATUS_SUCCESS anyway\n",\
                                        dev->base.id));

                        } else if (!(dev->setting &
                                DEVICE_SETTING_SMART_ENABLED)) {

                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
                        }

                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

                default:
                        CORE_DPRINT(("Unsupported API PD "\
                                "Setting %d\n", req->Cdb[4]));

                        if (req->Sense_Info_Buffer != NULL &&
                                req->Sense_Info_Buffer_Length != 0)

                                ((MV_PU8)req->Sense_Info_Buffer)[0]
                                        = ERR_INVALID_PARAMETER;

                        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                        result = MV_QUEUE_COMMAND_RESULT_FINISHED;
                        break;
                }
                break;

        case APICDB1_PD_GETSTATUS:
                switch (req->Cdb[4]) {
                case APICDB4_PD_SMART_RETURN_STATUS:
                        if (!(dev->capability &
                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                status = core_map_data_buffer(req);
                                if (status != NULL)
                                        status->SmartThresholdExceeded = MV_FALSE;
                                core_unmap_data_buffer(req);
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        /* Page=2F(Informal Exceptions Log) */
                        new_req = sas_make_log_sense_req(dev, 0x2F,
                                        ssp_ata_intl_req_callback);

                        if (new_req == NULL)
                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

                        sas_replace_org_req(root, req, new_req);
                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

                default:
                        CORE_DPRINT(("Unsupported API PD "\
                                "Get Status %d\n", req->Cdb[4]));

                        if (req->Sense_Info_Buffer != NULL &&
                                req->Sense_Info_Buffer_Length != 0)

                                ((MV_PU8)req->Sense_Info_Buffer)[0]
                                        = ERR_INVALID_PARAMETER;

                        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                        result = MV_QUEUE_COMMAND_RESULT_FINISHED;
                        break;
                }
        }
        break;

	default:
		result = MV_QUEUE_COMMAND_RESULT_PASSED;
	}

	if((((core_extension *)(root->core))->has_port_queue)
		&& (result == MV_QUEUE_COMMAND_RESULT_PASSED))
	{
		if(dev->base.type==BASE_TYPE_DOMAIN_ENCLOSURE) {
			domain_enclosure *enc = (domain_enclosure *)dev;
			if (enc->register_set == NO_REGISTER_SET) {
				MV_U8 set;
				set = core_get_register_set(root,enc);
				if (set == NO_REGISTER_SET)
					result =MV_QUEUE_COMMAND_RESULT_FULL;
				else
					enc->register_set = set;
				}
		} else if (dev->register_set == NO_REGISTER_SET) {
			MV_U8 set;
			set = core_get_register_set(root,dev);
			if (set == NO_REGISTER_SET)
				result =MV_QUEUE_COMMAND_RESULT_FULL;
			else
				dev->register_set = set;
			}
	}

	return result;
}

MV_VOID ssp_ata_host_string2ata(MV_U16 *source, MV_U16 *target,
	MV_U32 words_count)
{
	MV_U32 i;
	for (i=0; i < words_count; i++) {
		target[i] = (source[i] >> 8) | ((source[i] & 0xff) << 8);
		target[i] = MV_LE16_TO_CPU(target[i]);
	}
}


MV_VOID ssp_ata_fill_inquiry_device(domain_device *dev, MV_Request *req)
{
        ata_identify_data *dst = (ata_identify_data *)core_map_data_buffer(req);
        MV_U16  tmp;
        MV_LBA max_lba;

        MV_ZeroMemory(dst, sizeof(ata_identify_data));

        tmp = 0x8000;
        ssp_ata_host_string2ata(&tmp, &dst->general_config, 1);

        /* serial number */
        ssp_ata_host_string2ata((MV_PU16)dev->serial_number,
                (MV_PU16)dst->serial_number, 10);

        /* firmware */
        ssp_ata_host_string2ata((MV_PU16)dev->firmware_revision,
                (MV_PU16)dst->firmware_revision, 4);

        /* model */
        ssp_ata_host_string2ata((MV_PU16)dev->model_number,
                (MV_PU16)dst->model_number, 20);

        tmp = 0;
        if (dev->capability & DEVICE_CAPABILITY_SMART_SUPPORTED)
                tmp |= MV_BIT(0);

        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_supported[0], 1);

        tmp = MV_BIT(10);
        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_supported[1], 1);

        tmp = 0;
        if (dev->setting & DEVICE_SETTING_SMART_ENABLED)
                tmp |= MV_BIT(0);
        if (dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED)
                tmp |= MV_BIT(5);
        if (dev->setting & DEVICE_SETTING_READ_LOOK_AHEAD)
                tmp |= MV_BIT(6);

        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_enabled[0], 1);

        /* max lba */
        max_lba = U64_ADD_U32(dev->max_lba, 1);
        tmp = (MV_U16)(max_lba.parts.low & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[0], 1);
        ssp_ata_host_string2ata(&tmp,
                &dst->user_addressable_sectors[0], 1);

        tmp = (MV_U16)((max_lba.parts.low >> 16) & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[1], 1);
        ssp_ata_host_string2ata(&tmp,
                &dst->user_addressable_sectors[1], 1);

        tmp = (MV_U16)(max_lba.parts.high & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[2], 1);
        tmp = (MV_U16)((max_lba.parts.high >> 16) & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[3], 1);

        core_unmap_data_buffer(req);
}

MV_VOID
ssp_ata_intl_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req = sas_clear_org_req(req);
	domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, MV_FALSE);
	core_extension *core = (core_extension *)root->core;
	MV_U32 length;
	MV_BOOLEAN is_changed=MV_FALSE;
	if (org_req == NULL) {
		MV_DASSERT(MV_FALSE);
		return;
	}

        org_req->Scsi_Status = req->Scsi_Status;
        if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
                length = MV_MIN(org_req->Sense_Info_Buffer_Length,
                                req->Sense_Info_Buffer_Length);

                if (length != 0 &&
                        org_req->Sense_Info_Buffer != NULL &&
                        req->Sense_Info_Buffer != NULL)

                        MV_CopyMemory(
                                org_req->Sense_Info_Buffer,
                                req->Sense_Info_Buffer,
                                length);

                if ((org_req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC &&
                        org_req->Cdb[2] == CDB_CORE_OS_SMART_CMD &&
                        org_req->Cdb[3] == ATA_CMD_SMART &&
                        org_req->Cdb[4] == ATA_CMD_SMART_RETURN_STATUS) ||
                        (org_req->Cdb[0] == APICDB0_PD &&
                        org_req->Cdb[1] == APICDB1_PD_GETSTATUS &&
                        org_req->Cdb[4] == APICDB4_PD_SMART_RETURN_STATUS)) {

                        core_generate_event(core, EVT_ID_HD_SMART_POLLING_FAIL,
                                dev->base.id, SEVERITY_WARNING, 0, NULL ,0);
                }

                core_queue_completed_req(core, org_req);
                return;
        }

	switch (org_req->Cdb[0]) {
        case SCSI_CMD_MARVELL_SPECIFIC:
                if (org_req->Cdb[1] != CDB_CORE_MODULE)
                        MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
                case CDB_CORE_ENABLE_WRITE_CACHE:
                        dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
                        break;

                case CDB_CORE_DISABLE_WRITE_CACHE:
                        dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
                        break;
#ifdef SUPPORT_PHY_POWER_MODE
		 case CDB_CORE_READ_POWER_MODE:
                        break;
#endif
                case CDB_CORE_ENABLE_READ_AHEAD:
                        dev->setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
                        break;

                case CDB_CORE_DISABLE_READ_AHEAD:
                        dev->setting &= ~DEVICE_SETTING_READ_LOOK_AHEAD;
                        break;

                case CDB_CORE_SMART_RETURN_STATUS:
                        ssp_ata_parse_smart_return_status(dev, org_req, req);
                        break;
                case CDB_CORE_OS_SMART_CMD:
                        switch (org_req->Cdb[4]) {
                        case ATA_CMD_SMART_RETURN_STATUS:
                                ssp_ata_parse_smart_return_status(dev,
                                        org_req, req);
                                break;

                        default:
                                MV_DASSERT(MV_FALSE);
                                break;
                        }
                        break;
                case CDB_CORE_SHUTDOWN:
                        break;
                default:
                        MV_DASSERT(MV_FALSE);
                        break;
                }
                break;

        case APICDB0_PD:
              /* Currently RAID module will send only these 2 API requests */
                switch (org_req->Cdb[1]) {
                case APICDB1_PD_SETSETTING:
                        switch (org_req->Cdb[4]) {
                        case APICDB4_PD_SET_WRITE_CACHE_OFF:
                                if(dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED)
                                    is_changed=MV_TRUE;

                                dev->setting &=
                                        ~DEVICE_SETTING_WRITECACHE_ENABLED;
                                if(is_changed && (org_req->Cdb[7] != MV_BIT(7))) {
                                core_generate_event(core,
                                        EVT_ID_HD_CACHE_MODE_CHANGE,
                                        dev->base.id, SEVERITY_INFO,
                                        0, NULL ,0);
                                }
                                break;

                        case APICDB4_PD_SET_WRITE_CACHE_ON:
                                if(!(dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED))
                                    is_changed=MV_TRUE;
                                dev->setting |=
                                        DEVICE_SETTING_WRITECACHE_ENABLED;
                                if((is_changed && (org_req->Cdb[7] != MV_BIT(7)))) {
                                core_generate_event(core,
                                        EVT_ID_HD_CACHE_MODE_CHANGE,
                                        dev->base.id, SEVERITY_INFO,
                                        0, NULL ,0);
                                }
                                break;

                        case APICDB4_PD_SET_SMART_OFF:
                        case APICDB4_PD_SET_SMART_ON:
                                break;
                        default:
		                MV_DASSERT(MV_FALSE);
                                return;
                        }
                        break;

                case APICDB1_PD_GETSTATUS:
                        switch (org_req->Cdb[4]) {
                        case APICDB4_PD_SMART_RETURN_STATUS:
                                ssp_ata_parse_smart_return_status(dev,
					org_req, req);
                                break;

                        default:
		                MV_DASSERT(MV_FALSE);
                                return;
                        }
                        break;
                }
                break;

	default:
		MV_DASSERT(MV_FALSE);
                return;
	}

        core_queue_completed_req(core, org_req);
}

MV_VOID
ssp_mode_page_rmw_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req, *new_req;
	domain_device *dev;
	MV_U32 length;
	MV_PU8 org_buf_ptr, new_buf_ptr;
#ifdef SUPPORT_PHY_POWER_MODE
	core_extension *core = root->core;
	domain_phy *phy;
	domain_port *port;
#endif
	org_req = sas_clear_org_req(req);

	if (org_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}

	dev = (domain_device *)get_device_by_id(root->lib_dev,
		org_req->Device_Id, CORE_IS_ID_MAPPED(org_req), MV_FALSE);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		length = MV_MIN(req->Sense_Info_Buffer_Length,
		org_req->Sense_Info_Buffer_Length);

		MV_CopyMemory(org_req->Sense_Info_Buffer,
			req->Sense_Info_Buffer,
			length);

		org_req->Scsi_Status = req->Scsi_Status;
		core_queue_completed_req(root->core, org_req);
		return;
	}

	org_buf_ptr = core_map_data_buffer(req);
#ifdef SUPPORT_PHY_POWER_MODE
	if((org_req->Cdb[2] != CDB_CORE_READ_POWER_MODE) && (org_req->Cdb[2] != CDB_CORE_CHECK_POWER_MODE))
#endif
	{
		MV_DASSERT(org_buf_ptr[3] == 0);
		MV_DASSERT((org_buf_ptr[4] & 0x6F) == 0x08);
	}
	switch (org_req->Cdb[0]) {
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (org_req->Cdb[1] != CDB_CORE_MODULE)
			MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
		case CDB_CORE_ENABLE_WRITE_CACHE:
			if (org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2)) {
				dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;
#ifdef SUPPORT_PHY_POWER_MODE
		case CDB_CORE_READ_POWER_MODE:
		case CDB_CORE_CHECK_POWER_MODE:
			if(core->PHY_power_mode_DIPM || core->PHY_power_mode_HIPM){//enable power mode
				if ((org_buf_ptr[39] & MV_BIT(2)) || (org_buf_ptr[39] & MV_BIT(1))) {//bit 2: Slumber bit 1: Partial
					dev->setting |= DEVICE_SETTING_POWERMODE_ENABLED;
					port = dev->base.port;
					phy = port->phy;
					if(core->PHY_power_mode_DIPM){
						//Core_Disable_PHY_Interrupt(phy);
						core->PHY_power_mode_port_enabled |= (1<<(root->base_phy_num + phy->asic_id));//record the device enabled DIPM or HIPM!
						Core_Enable_Receive_PHY_Power_Request(phy);
					}
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				if(org_req->Cdb[2] == CDB_CORE_CHECK_POWER_MODE){
					dev->capability &= ~(DEVICE_CAPABILITY_HIPM_SUPPORTED|DEVICE_CAPABILITY_DIPM_SUPPORTED);
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
			}else{//disable power mode
				if (!(org_buf_ptr[39] & MV_BIT(2)) && !(org_buf_ptr[39] & MV_BIT(1))) {
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				if(org_req->Cdb[2] == CDB_CORE_CHECK_POWER_MODE){
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
			}
			break;
#endif
		case CDB_CORE_DISABLE_WRITE_CACHE:
			if (!(org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2))) {
				dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		case CDB_CORE_ENABLE_READ_AHEAD:
			if (!(org_buf_ptr[16 + org_buf_ptr[3]] & MV_BIT(5))) {
				dev->setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		case CDB_CORE_DISABLE_READ_AHEAD:
			if (org_buf_ptr[16 + org_buf_ptr[3]] & MV_BIT(5)) {
				dev->setting &= ~DEVICE_SETTING_READ_LOOK_AHEAD;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	case APICDB0_PD:
		/* Currently RAID module will send only these 2 API requests */
		switch (org_req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (org_req->Cdb[4]) {
			case APICDB4_PD_SET_WRITE_CACHE_OFF:
				if (!(org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2))) {
					dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				break;

			case APICDB4_PD_SET_WRITE_CACHE_ON:
				if (org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2)) {
					dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				break;
			default:
				MV_DASSERT(MV_FALSE);
				break;
			}
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	default:
		MV_DASSERT(MV_FALSE);
		break;
	}
#ifdef SUPPORT_PHY_POWER_MODE
	if(org_req->Cdb[2] == CDB_CORE_READ_POWER_MODE){
		new_req = get_intl_req_resource(root, org_buf_ptr[0]+1);
	}else
#endif
	{
		new_req = get_intl_req_resource(root, 24);
	}
	if (new_req == NULL) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)org_req->Sense_Info_Buffer)[0] = ERR_NO_RESOURCE;

		org_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		core_unmap_data_buffer(req);
		core_queue_completed_req(root->core, org_req);
		return;
	}

	new_req->Cdb[0] = SCSI_CMD_MODE_SELECT_6;
#ifdef SUPPORT_PHY_POWER_MODE
	if(org_req->Cdb[2] == CDB_CORE_READ_POWER_MODE){
		new_req->Cdb[1] = 0x11;
		new_req->Cdb[4] = org_buf_ptr[0]+1;
	}else
#endif
	{
		new_req->Cdb[1] = 0x11;
		new_req->Cdb[4] = 24;
	}
	new_req->Device_Id = dev->base.id;
#ifdef SUPPORT_PHY_POWER_MODE
	if(org_req->Cdb[2] == CDB_CORE_READ_POWER_MODE){
		new_req->Cmd_Flag = CMD_FLAG_PIO;
	}else
#endif
	{
		new_req->Cmd_Flag = 0;
	}
	new_req->Completion = ssp_ata_intl_req_callback;
	new_buf_ptr = core_map_data_buffer(new_req);
#ifdef SUPPORT_PHY_POWER_MODE
	if(org_req->Cdb[2] == CDB_CORE_READ_POWER_MODE){
		MV_CopyMemory(&new_buf_ptr[0], &(org_buf_ptr[0]), org_buf_ptr[0]+1);
	}else
#endif
	{
		MV_CopyMemory(&new_buf_ptr[4], &(org_buf_ptr[4 + org_buf_ptr[3]]), 20);
	}
        /* Mode Data Length = 0 */
        /* When using the MODE SELECT command, this field is reserved */
#ifdef SUPPORT_PHY_POWER_MODE
	if(org_req->Cdb[2] == CDB_CORE_READ_POWER_MODE){
		new_buf_ptr[0] = 0;
		new_buf_ptr[1] = 0;
		new_buf_ptr[2] = 0;
		new_buf_ptr[3] = 8;
		new_buf_ptr[4] = 0;
		new_buf_ptr[5] = 0;
		new_buf_ptr[6] = 0;
		new_buf_ptr[7] = 0;
	}else
#endif
	{
		new_buf_ptr[0] = 0;
		new_buf_ptr[1] = 0;
		new_buf_ptr[2] = 0;
		new_buf_ptr[3] = 0;
		new_buf_ptr[4] &= 0x7F;
	}

	switch (org_req->Cdb[0]) {
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (org_req->Cdb[1] != CDB_CORE_MODULE)
		        MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
		case CDB_CORE_ENABLE_WRITE_CACHE:
			new_buf_ptr[6] |= MV_BIT(2);
			break;

		case CDB_CORE_DISABLE_WRITE_CACHE:
			new_buf_ptr[6] &= ~MV_BIT(2);
			break;
#ifdef SUPPORT_PHY_POWER_MODE
		case CDB_CORE_READ_POWER_MODE:
			if(core->PHY_power_mode_DIPM || core->PHY_power_mode_HIPM){
				if(!core->PHY_power_mode_DIPM){//for HIPM only, only enable one power mode
					if(core->PHY_power_mode_HIPM == 1){//partial mode
						new_buf_ptr[39] &= ~((MV_U8)0x6);
						new_buf_ptr[39] |= 0x2;//enable partial only
					}else{//slumber mode
						new_buf_ptr[39] &= ~((MV_U8)0x6);
						new_buf_ptr[39] |= 0x4;//enable slumber only
					}
				}else{//for DIPM or both enable HIPM and DIPM, enable both partial and slumber
					new_buf_ptr[39] |= 6;
				}
			}else{
				new_buf_ptr[39] &= ~(6);
			}
			break;
#endif
		case CDB_CORE_ENABLE_READ_AHEAD:
			new_buf_ptr[16] &= ~MV_BIT(5);
			break;

		case CDB_CORE_DISABLE_READ_AHEAD:
			new_buf_ptr[16] |= MV_BIT(5);
			break;

		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	case APICDB0_PD:
		/* Currently RAID module will send only these 2 API requests */
		switch (org_req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (org_req->Cdb[4]) {
			case APICDB4_PD_SET_WRITE_CACHE_OFF:
				new_buf_ptr[6] &= ~MV_BIT(2);
				break;
			case APICDB4_PD_SET_WRITE_CACHE_ON:
				new_buf_ptr[6] |= MV_BIT(2);
				break;
			default:
				MV_DASSERT(MV_FALSE);
				break;
			}
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

		default:
		MV_DASSERT(MV_FALSE);
		break;
	}

	core_unmap_data_buffer(req);
	core_unmap_data_buffer(new_req);

	sas_replace_org_req(root, org_req, new_req);
}

#ifdef SUPPORT_SG_RESET
MV_VOID ssp_hba_reset_callback(MV_PVOID root_p, MV_Request *req)
{
	MV_Request *org_req;
	pl_root *root = (pl_root *)root_p;
	core_extension * core = (core_extension *)root->core;
	struct hba_extension *phba = (struct hba_extension *)HBA_GetModuleExtension(core, MODULE_HBA);
	
	org_req = req->Org_Req;
	if ((org_req == NULL) || (req == NULL))
	{
		return;
    }	
		  
	org_req->Scsi_Status = req->Scsi_Status;
	org_req->Completion(phba, org_req);
}
#endif

MV_VOID ssp_prepare_task_management_command(pl_root *root,
	domain_device *dev, mv_command_header *cmd_header,
	mv_command_table *cmd_table,  MV_Request *req)
{
	MV_U64 val64;
	MV_U16 tag;
	MV_U32 ch_dword_0 = 0;

	/*
	 * CDB[3..4] = tag
	 * CDB[5] = task function
	 * CDB[6..13] = lun
	 */

	tag = (MV_U16)(req->Cdb[4]<<8) + (MV_U16)req->Cdb[3];
	cmd_header->frame_len = MV_CPU_TO_LE16(((sizeof(ssp_task_iu) + sizeof(ssp_frame_header)) / 4) & CH_FRAME_LEN_MASK);
#ifdef SUPPORT_BALDUR
	cmd_header->max_sim_conn = 1;
//#else
//	cmd_header->frame_len = MV_CPU_TO_LE16(
//			((sizeof(ssp_task_iu) + sizeof(ssp_frame_header)) / 4) & CH_FRAME_LEN_MASK );
#endif
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;

	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.task.lun, &req->Cdb[6], 8);
	cmd_table->table.ssp_cmd_table.data.task.tag = MV_CPU_TO_BE16(tag);
	cmd_table->table.ssp_cmd_table.data.task.task_function = req->Cdb[5];

        /* Set reserved values to zero */
        ((MV_PU16)cmd_table->table.ssp_cmd_table.data.task.reserved1)[0] = 0;
        cmd_table->table.ssp_cmd_table.data.task.reserved2 = 0;
        MV_ZeroMemory(cmd_table->table.ssp_cmd_table.data.task.reserved3, 14);

	ch_dword_0 |= FRAME_TYPE_TASK << CH_SSP_FRAME_TYPE_SHIFT;
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(ch_dword_0);

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SSP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
        ((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;
	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) =
		MV_CPU_TO_BE16(dev->base.id + 1);
	U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID ssp_prepare_io_command(pl_root *root, domain_device *dev,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_Request *req)
{
	MV_U64 val64;
	domain_enclosure *enc = NULL;
	MV_BOOLEAN is_enclosure = MV_FALSE;
	MV_U32 ch_dword_0 = 0;
#ifdef SUPPORT_MUL_LUN
	domain_base *base = (domain_base*)dev;
	mv_int_to_reqlun(base->LUN, req->lun);
#endif
#ifdef SUPPORT_PIR
//	if (req->Cmd_Flag & CMD_FLAG_PIR) 
	{   
		MV_ZeroMemory(&cmd_table->table.ssp_cmd_table.data.command.pir, sizeof(protect_info_record));
#ifdef ATHENA_A0_WORKAROUND
//                cmd_table->table.ssp_cmd_table.pir.T10_CHK_EN=1;
//                cmd_table->table.ssp_cmd_table.pir.USR_DT_SZ=0x40;
#endif

	}
#endif

#ifdef SUPPORT_SES
	if(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE){
		is_enclosure = MV_TRUE;
		enc = (domain_enclosure *)dev;
	}

	if(!is_enclosure)
#endif
	{
#if defined(SUPPORT_DIF) || defined(SUPPORT_DIX)
		/* it's normal SSP requests */
		if (dev->setting & DEVICE_SETTING_PI_ENABLED)
		{
			MV_ZeroMemory(&cmd_table->table.ssp_cmd_table.data.command.pir, sizeof(protect_info_record));
	              cmd_table->table.ssp_cmd_table.data.command.pir.USR_DT_SZ  =
				MV_CPU_TO_LE16((dev->logical_sector_size - 8)/4);
#if defined(SUPPORT_DIF) && !defined(SUPPORT_DIX)
			cmd_header->data_xfer_len +=
				MV_CPU_TO_LE32((cmd_header->data_xfer_len/
					(dev->logical_sector_size -8))
					*8);
#endif
			switch(req->Cdb[0])
			{
			/*READ*/
			case SCSI_CMD_READ_10:
			case SCSI_CMD_READ_12:
			case SCSI_CMD_READ_16:
			/*VERIFY*/
			case SCSI_CMD_VERIFY_10:
			case SCSI_CMD_VERIFY_12:
			case SCSI_CMD_VERIFY_16:
			/*WRITE*/
			case SCSI_CMD_WRITE_10:
			case SCSI_CMD_WRITE_12:
			case SCSI_CMD_WRITE_16:
			/*WRITE&VERIFY*/
			case SCSI_CMD_WRITE_VERIFY_10:
			case SCSI_CMD_WRITE_VERIFY_12:
			case SCSI_CMD_WRITE_VERIFY_16:
			/*WRITE SAME*/
			case SCSI_CMD_WRITE_SAME_10:
			case SCSI_CMD_WRITE_SAME_16:
                     case SCSI_CMD_UNMAP:
			/*XDWRITE*/
			case SCSI_CMD_XDWRITE_10:
			/*XPWRITE*/
			case SCSI_CMD_XPWRITE_10:
			/*XDREAD*/
			case SCSI_CMD_XDREAD_10:
			/*XDWRITEREAD*/
			case SCSI_CMD_XDWRITEREAD_10:
				if ((req->Cdb[0] & 0xf0) != 0x80) {
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL =
						MV_CPU_TO_LE32(req->Cdb[5] +
							(req->Cdb[4]<<8) +
							(req->Cdb[3]<<16) +
							(req->Cdb[2]<<24));
				} else {
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL =
						MV_CPU_TO_LE32(req->Cdb[9] +
							(req->Cdb[8]<<8) +
							(req->Cdb[7]<<16) +
							(req->Cdb[6]<<24));
				}
				cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_CHK_VAL =
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL;
				cmd_table->table.ssp_cmd_table.data.command.pir.INCR_LBRT = 1;

				if (req->EEDPFlags & 0x02)
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_RMV_EN = 1;
				if (req->EEDPFlags & 0x04)
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_INSRT_EN = 1;
				if (req->EEDPFlags & 0x01) {
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_EN = 1;
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_MSK = 0xf3;
				}
				if (req->EEDPFlags & 0x08) {
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_RPLC_EN = 1;
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_RPLC_MSK = 0x03;
				}

				/*Set T10_FLDS_PRSNT*/
#if defined(SUPPORT_ATHENA)
				cmd_header->pir_fmt = 0x8;
#endif
				ch_dword_0 |= CH_SSP_VERIFY_DATA_LEN;
				ch_dword_0 |= CH_PI_PRESENT;
				break;
/*prot type 1/3, not support 32byte command*/
			case 0x7f:
			/*
			READ(32)
			VERIFY(32)
			WRITE(32)
			WRITE&VERIFY(32)
			WRITE SAME(32)
			XDWRITE(32)
			XPWRITE(32)
			XDREAD(32)
			XDWRITEREAD(32)
			*/
				switch ((req->Cdb[8]<<8) + req->Cdb[9])
				{
				case 0x0003:
				case 0x0004:
				case 0x0006:
				case 0x0007:
				case 0x0009:
				case 0x000a:
				case 0x000b:
				case 0x000c:
				case 0x000d:
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL =
						MV_CPU_TO_LE32(req->Cdb[23] +
							(req->Cdb[22]<<8) +
							(req->Cdb[21]<<16) +
							(req->Cdb[20]<<24));

					cmd_table->table.ssp_cmd_table.data.command.pir.LBAT_GEN_VAL =
						MV_CPU_TO_LE16(req->Cdb[25] +
							(req->Cdb[24]<<8));

					cmd_table->table.ssp_cmd_table.data.command.pir.LBAT_CHK_MASK =
						MV_CPU_TO_LE16(req->Cdb[27] +
							(req->Cdb[26]<<8)); 
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_CHK_VAL =
						cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL;
					cmd_table->table.ssp_cmd_table.data.command.pir.INCR_LBRT = 1;
			
					if (req->EEDPFlags & 0x02)
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_RMV_EN = 1;
					if (req->EEDPFlags & 0x04)
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_INSRT_EN = 1;
					if (req->EEDPFlags & 0x01) {
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_EN = 1;
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_MSK = 0xf3;
					}
					if (req->EEDPFlags & 0x08) {
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_RPLC_EN = 1;
						cmd_table->table.ssp_cmd_table.data.command.pir.T10_RPLC_MSK = 0x03;
					}

					/*Set T10_FLDS_PRSNT*/
					// CH_PI_PRESENT ?
#if defined(SUPPORT_ATHENA)
					cmd_header->pir_fmt = 0x8;
#endif
					ch_dword_0 |= CH_SSP_VERIFY_DATA_LEN;
					break;
				default:
					break;
				}

			case SCSI_CMD_FORMAT_UNIT:
				ch_dword_0 |= CH_PI_PRESENT;
				break;
			default:
				ch_dword_0 &= (~ CH_SSP_VERIFY_DATA_LEN);
				break;
			}
		}
#endif
	}

	cmd_header->frame_len =	MV_CPU_TO_LE16(
			((sizeof(ssp_command_iu) + sizeof(ssp_frame_header)+3)/4) & CH_FRAME_LEN_MASK );
#ifdef SUPPORT_BALDUR
	cmd_header->max_sim_conn = 1;
#endif
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;
	ch_dword_0 |= FRAME_TYPE_COMMAND << CH_SSP_FRAME_TYPE_SHIFT;
#ifdef SUPPORT_PIR
//	if (req->Cmd_Flag & CMD_FLAG_PIR) 
	if(0)  // Don't enable PI in expander.
	{   
		ch_dword_0 |= CH_PI_PRESENT;
		cmd_header->pir_fmt=0x8;
	}
#endif

#ifdef HAVE_PRD_SKIP
	if (req->Cmd_Flag & CMD_FLAG_PRD_SKIP) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		ch_dword_0 |= CH_DATA_SKIIP_ENTRY_IN_PRD;
#else
		protect_info_record *pir = &cmd_table->table.ssp_cmd_table.data.command.pir;

		pir->SKIP_EN = 1;
		pir->USR_DT_SZ = MV_CPU_TO_LE16(SECTOR_SIZE >> 2);
		pir->init_block_xfer = MV_CPU_TO_LE16(req->init_block_xfer);
		pir->init_block_skip = MV_CPU_TO_LE16(req->init_block_skip);
		pir->sub_block_xfer = MV_CPU_TO_LE16(req->sub_block_xfer);
		pir->sub_block_skip = MV_CPU_TO_LE16(req->sub_block_skip);

		ch_dword_0 |= CH_PI_PRESENT;
#endif
		//MV_PRINT("SKIP\n");
	}
#endif
#ifdef SUPPORT_BALDUR
    ch_dword_0 |= CH_SSP_VERIFY_DATA_LEN;
#endif
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(ch_dword_0);

	//OLD CODE: Never set the frame_header
	//OLD CODE: for command_iu only set the CDB
	//HARRIET: have to zero command_iu. A lot of the fields have to be zero
	// or requests behind expanders will timeout.

	MV_ZeroMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu,
		sizeof(ssp_command_iu));
	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.cdb[0],
		&req->Cdb[0], 16);

#ifdef SUPPORT_VAR_LEN_CDB
	if (req->Cdb[0] == 0x7f) {
		cmd_table->table.ssp_cmd_table.data.command.command_iu.additional_cdb_length = 4;
		MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.e_cdb[0],
			&req->Cdb[16], 16);
	} else {
		/* for non-32byte cdb the pir has to be moved to the e_cdb's position to make the 
		cmd_table continuous*/
		MV_U8 i;
		for (i = 0; i < sizeof(protect_info_record); i++) {
			MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.e_cdb[0] + i,
				((MV_PU8)&cmd_table->table.ssp_cmd_table.data.command.pir) + i, 1);
		}
		cmd_header->frame_len -= 4; //substruct the 4 DWs for e_cdb
	}
#endif
	/*set task attribute according to tcq queue type*/
	cmd_table->table.ssp_cmd_table.data.command.command_iu.task_attribute |= dev->queue_type;

#ifdef SUPPORT_MUL_LUN
	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.lun[0],
		&req->lun[0], 8);
#endif
	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SSP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);

        /* Setting entire byte as negotiated link rate even though only the
           lower 4-bit is link rate is to clear the Feature field in the upper
           4-bit as well. Feature field should always be set to zero */
#ifdef SUPPORT_SES
	if(is_enclosure)
		((MV_PU8)&cmd_table->open_address_frame)[1] = enc->negotiated_link_rate;
	else
#endif
		((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;

	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) = MV_CPU_TO_BE16(dev->base.id + 1);
#if 1
#ifdef SUPPORT_SES
	if(is_enclosure)
		U64_ASSIGN(val64, MV_CPU_TO_BE64(enc->sas_addr));
	else
#endif
		U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));
#else
	val64.parts.high = MV_CPU_TO_BE32(enc->sas_addr.parts.high);
	val64.parts.low = MV_CPU_TO_BE32(enc->sas_addr.parts.low);
#endif

	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID ssp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;

	/* clear sata ctrl and sas ctrl */
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(CH_SSP_TP_RETRY);
	cmd_header->max_rsp_frame_len = MV_CPU_TO_LE16(MAX_RESPONSE_FRAME_LENGTH >> 2);
	//OLD CODE: for command header, never set target_tag? others are all set now.

	/* if it's task management request */
	if (IS_A_TSK_REQ(req)) {
		ssp_prepare_task_management_command(
			root, dev, cmd_header, cmd_table, req);
	} else {
		ssp_prepare_io_command(
			root, dev, cmd_header, cmd_table, req);
	}
}
MV_VOID
ssp_send_command(
    MV_PVOID root_p,
    MV_PVOID dev_p,
    MV_PVOID struct_wrapper,
    MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	domain_port *port = dev->base.port;
	core_context *ctx = req->Context[MODULE_CORE];
	MV_U8 phy_map;
	MV_U16 queue_id;
	MV_U32 entry_nm;
	hw_buf_wrapper *wrapper = (hw_buf_wrapper *) struct_wrapper;
	
    	MV_U8 register_set = NO_REGISTER_SET;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
    	if((req->Req_Flag & REQ_FLAG_BYPASS_HYBRID)){
            cmd_resource *com_res;
            if(dev->register_set == NO_REGISTER_SET){
                dev->register_set = core_get_register_set(root, dev);
            }
            if(dev->register_set == NO_REGISTER_SET){
                MV_DPRINT(("dev %d no register_set \n", dev->base.id));
            }
            MV_ASSERT(req->Context[MODULE_HBA]);
            com_res = (cmd_resource *) req->Context[MODULE_HBA];
            register_set = dev->register_set;
            dev->base.outstanding_req++;
            entry_nm = prot_get_delv_q_entry(root, dev->queue_id);
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
            phy_map = core_wideport_load_balance_asic_phy_map(port, dev);
#else
            phy_map = port->asic_phy_map; /* from Phy 0 */
#endif
            queue_id = dev->queue_id;
            MV_ASSERT(wrapper != NULL);
            root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(TXQ_MODE_I |com_res->slot |TXQ_CMD_SSP |1 << TXQ_PRIORITY_SHIFT | (register_set <<TXQ_REGSET_SHIFT));
#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
            root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_SRAM);
#else
            root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_PCIEA);
#endif
            root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32(wrapper->phy.parts.low);
            root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32(wrapper->phy.parts.high);
            prot_write_delv_q_entry(root, entry_nm, queue_id);
            return;
        }
#endif
	if (dev->base.type==BASE_TYPE_DOMAIN_ENCLOSURE) {
		domain_enclosure *enc = (domain_enclosure *)dev;
		register_set = enc->register_set;
		entry_nm = prot_get_delv_q_entry(root, 0);
		//CORE_DPRINT(("enclosure register set:%x for cmd %x\n", register_set, req->Cdb[0]));
	} else {
		register_set = dev->register_set;
		entry_nm = prot_get_delv_q_entry(root, dev->queue_id);
	}

#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND

#ifdef ATHENA2_Z1_WIDE_PORT_MULTI_PORT_ISSUE
	if(!IS_BEHIND_EXP(dev) || port->phy_num == 1) {
		phy_map = port->asic_phy_map;
	}else
#endif
	{
		if (CORE_IS_EH_REQ(ctx))
			phy_map = port->asic_phy_map;
	else
			phy_map = core_wideport_load_balance_asic_phy_map(port, dev);
	}
#else

	phy_map = port->asic_phy_map; /* from Phy 0 */

#endif

#ifdef ATHENA2_Z1_WIDE_PORT_MULTI_PORT_ISSUE
	if((port->asic_phy_map==0xf0)||(port->asic_phy_map==0x10))	
		phy_map = 0x10;	//set port 4 to transmit
	else if((port->asic_phy_map==0x0f)||(port->asic_phy_map==0x01))
		phy_map = 0x1;	//set port 0 to transmit
#endif	
	if (dev->base.type != BASE_TYPE_DOMAIN_DEVICE) {
		queue_id = 0;
	} else {
		queue_id = dev->queue_id;
	}

	if (IS_A_TSK_REQ(req)) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		MV_ASSERT(wrapper != NULL);
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(TXQ_MODE_I |ctx->slot |TXQ_CMD_SSP |1 << TXQ_PRIORITY_SHIFT | (register_set <<TXQ_REGSET_SHIFT));
		#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_SRAM);
		#else
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_PCIEA);
		#endif
		root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32(wrapper->phy.parts.low);
		root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32(wrapper->phy.parts.high);
#else
		root->queues[queue_id].delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
			phy_map  << TXQ_PHY_SHIFT | TXQ_CMD_SSP |\
			 				1 << TXQ_PRIORITY_SHIFT);
#endif
	} else {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		MV_ASSERT(wrapper != NULL);
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW0 = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot  | TXQ_CMD_SSP |(register_set <<TXQ_REGSET_SHIFT));
		#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_SRAM);
		#else
		root->queues[queue_id].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map<< TXQ_PORT_SHIFT) | INTRFC_PCIEA);
		#endif
		root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32(wrapper->phy.parts.low);
		root->queues[queue_id].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32(wrapper->phy.parts.high);
#else
		root->queues[queue_id].delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
			phy_map  << TXQ_PHY_SHIFT | TXQ_CMD_SSP);
#endif
	}

	prot_write_delv_q_entry(root, entry_nm, queue_id);
}

/* this includes error handling for SMP, STP and SSP*/
MV_VOID sas_process_command_error(pl_root *root, domain_base *base,
	mv_command_table *cmd_table, err_info_record *err_info, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U32 error = MV_LE32_TO_CPU(err_info->err_info_field_1);
	MV_U32 i;
	MV_ULONG flags;
#ifdef CORE_EH_LOG_EVENTS
	MV_U32 params[MAX_EVENT_PARAMS];
	params[0] = CORE_EVENT_ERR_INFO;            /* CDB Field */
	params[1] = error;                          /* Sense Key */
	params[2] = req->Cdb[0];                    /* Additional Sense Code */

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE) ) {
		params[3] = req->Cdb[2];            /* Additional Sense Code Qualifier */
	}
	else {
		MV_SetLBAandSectorCount(req);
		params[3] = req->LBA.parts.low;     /* Additional Sense Code Qualifier */
	}

	core_generate_event(root->core, EVT_ID_HD_SC_ERROR, base->id,
		SEVERITY_INFO, 4, params, 0);
#endif

	MV_ASSERT(base == get_device_by_id(root->lib_dev,
						req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req)));

	/* don't know what kind of error it is yet */
	if (error & CMD_ISS_STPD){
		/* normal will see task file error or phy decoding error(hotplug) */
		ctx->error_info |= EH_INFO_CMD_ISS_STPD;

		base->cmd_issue_stopped = MV_TRUE;
		/* dont return right now */
	}
	if (error & NO_DEST_ERR){
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		/* suppose CMD_ISS_STPD is not set. otherwise need start issue somewhere */
		MV_ASSERT(!(ctx->error_info & EH_INFO_NEED_RETRY));
		return;
	}

#define SAS_TIMEOUT_REASON (OPEN_TMOUT_ERR| NAK_ERR | ACK_NAK_TO | RD_DATA_OFFST_ERR |\
		XFR_RDY_OFFST_ERR | UNEXP_XFER_RDY_ERR | DATA_OVR_UNDR_FLW_ERR | WD_TMR_TO_ERR |\
		SYNC_ERR | TX_STOPPED_EARLY | CNCTN_RT_NT_SPRTD_ERR)

        if (error & SAS_TIMEOUT_REASON) {
			if (IS_A_SMP_REQ(req)) {
				/* smp request watchdog timer is too short even with the largest value */
				req->Scsi_Status = REQ_STATUS_ERROR;
			} else {
				req->Scsi_Status = REQ_STATUS_TIMEOUT;
			}
#if 1 
            //Check PHY ready
            if(base->port && base->port->phy)
            {
                for(i=0;i<100;i++)
                {
#if defined(_OS_LINUX)
                    udelay(1);
#else
                    HBA_SleepMicrosecond(root->core, 1);
#endif
                    if(mv_is_phy_ready(root, base->port->phy))
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
                return;
            }else{
                CORE_EH_PRINT(("PHY is ready dev %d, req %p.\n", req->Device_Id, req));
#ifdef ASIC_WORKAROUND_WD_TO_RESET
			if (error & (WD_TMR_TO_ERR | TX_STOPPED_EARLY))
				ctx->error_info |= EH_INFO_WD_TO_RETRY;
#endif
	        ctx->error_info |= EH_INFO_NEED_RETRY;
            }
        #else
#ifdef ASIC_WORKAROUND_WD_TO_RESET
			if (error & (WD_TMR_TO_ERR | TX_STOPPED_EARLY))
				ctx->error_info |= EH_INFO_WD_TO_RETRY;
#endif
	        ctx->error_info |= EH_INFO_NEED_RETRY;
        #endif
		return;
	}

	if (error & TFILE_ERR) {
		/* SSP or STP requests */
		MV_DASSERT(!IS_A_SMP_REQ(req));
		sata_handle_taskfile_error(root, req);
		return;
	}

	if (error & RESP_BFFR_OFLW) {
		CORE_EH_PRINT(("Response Buffer Overflow Condition meet.\n"));
	}
	/* for other type of errors */
	MV_ASSERT(((MV_U64 *)err_info)->value != 0);

	/* ever saw error 0x00000000 0x00004000 */
	CORE_EH_PRINT(("attention: req %p[0x%x] unknown error 0x%08x 0x%08x! "\
		"treat as media error.\n",\
		req, req->Cdb[0],
		((MV_U64 *)err_info)->parts.high, \
		((MV_U64 *)err_info)->parts.low));

	req->Scsi_Status = REQ_STATUS_ERROR;
	ctx->error_info |= EH_INFO_NEED_RETRY;
	return;
}

#ifdef SUPPORT_BALDUR
MV_VOID sas_process_over_under_flow(pl_root *root, MV_Request *req, 
                                    err_info_record *err_info)
{
    MV_U32  reg;
    core_context *ctx = (core_context *)req->Context[MODULE_CORE];
    core_extension *core = (core_extension *)root->core;

    if (IS_VANIR(core)) {
	    /* get the actual transfered length */
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_ADDR);
	    /* Bit 16:6 is the slot number, Bit 5:0 is 0C */
	    reg = ((ctx->slot << 6) | 0x0C) & 0x1FFFF;
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_DATA, reg);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_CTRL_STS);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_DATA, 0x200000);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_READ_DATA);
	    /* Read 2x174, Bit [31:7] is the length */
	    reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_CMD_DATA);
	    reg = reg >> 7;
    } else {
#if 0
        /* FIXME: athena should get the transfered length from command header?
	 * need hw's comfirm. set to request length temporary */
	    reg = req->Data_Transfer_Length;
#endif
	    /* get the actual transfered length */
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_ADDR);
	    /* Bit 20:8 is the CMD slot number, Bit 5:0 is 0xC */
	    reg = ((ctx->slot << 8) | 0x0C) & 0x1FFFFF;	
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_DATA, reg);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_CTRL_STS);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_DATA, 0x0);
	    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_ADDR, CMD_HOST_READ_DATA);
	    /* Read 0x134, Bit [31:7] is the length */
	    reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_CMD_DATA);
	    reg = reg >> 7;
#ifdef ATHENA_A1_WORKAROUND
	    /* Wrong register value include status length*/
	    /* so we need to minus 0x18 (Deciaml 24) to get the actual transfer length*/
	    reg -= 0x18;
#endif
    }

    /* underflow */
    if(reg < req->Data_Transfer_Length){
        req->Data_Transfer_Length = reg;
        /* clear error bit */
        err_info->err_info_field_1 &= MV_CPU_TO_LE32((MV_U32)~DATA_OVR_UNDR_FLW_ERR);
        //MV_DPRINT(("dev %d data underflow.\n", req->Device_Id));
        return;
    }
    /* overflow, sas_process_command_error() handle it */
    //MV_DPRINT(("dev %d data overflow.\n", req->Device_Id));
    return;
}
#endif

typedef struct mv_scsi_sense_hdr {
        MV_U8 response_code;       /* permit: 0x0, 0x70, 0x71, 0x72, 0x73 */
        MV_U8 sense_key;
        MV_U8 asc;
        MV_U8 ascq;
}*MV_PSB_HDR,MV_SB_HDR;

MV_BOOLEAN ssp_sense_valid(MV_PSB_HDR sshdr)
{
	if (!sshdr)
		return MV_FALSE;

	return (sshdr->response_code & 0x70) == 0x70 ;
}

MV_BOOLEAN ssp_normalize_sense(MV_PU8 sense_buffer, MV_U32 sb_len, MV_PSB_HDR sshdr)
{
	if (!sense_buffer || !sb_len)
		return MV_FALSE;

	MV_ZeroMemory(sshdr,sizeof(*sshdr));

	sshdr->response_code = (sense_buffer[0] & 0x7f);

	if (!ssp_sense_valid(sshdr))
		return MV_FALSE;

	if (sshdr->response_code >= 0x72) {
                if (sb_len > 1)
                        sshdr->sense_key = (sense_buffer[1] & 0xf);
                if (sb_len > 2)
                        sshdr->asc = sense_buffer[2];
                if (sb_len > 3)
                        sshdr->ascq = sense_buffer[3];
        } else {
                if (sb_len > 2)
                        sshdr->sense_key = (sense_buffer[2] & 0xf);
                if (sb_len > 7) {
                        sb_len = (sb_len < (MV_U32)(sense_buffer[7] + 8)) ?
                                         sb_len : (MV_U32)(sense_buffer[7] + 8);
                        if (sb_len > 12)
                                sshdr->asc = sense_buffer[12];
                        if (sb_len > 13)
                                sshdr->ascq = sense_buffer[13];
                }
        }
	return MV_TRUE;
}

/* if return MV_TRUE, means we want to retry this request */
MV_BOOLEAN parse_sense_error(MV_PU8 sense_buffer, MV_U32 sb_len)
{
	MV_SB_HDR sshdr;

	if(!ssp_normalize_sense(sense_buffer, sb_len, &sshdr))
		return MV_FALSE;

	/* Mode Parameters changed */
	if (sshdr.sense_key == SCSI_SK_UNIT_ATTENTION &&
		sshdr.asc == 0x2a && sshdr.ascq == 0x01)
		return MV_TRUE;

	return MV_FALSE;
}

MV_VOID ssp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;
	ssp_response_iu *resp_iu;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U32 sense_length;
	MV_SB_HDR sshdr;
	core_extension *core = (core_extension *)root->core;
	MV_ULONG flags;

#ifdef SUPPORT_MUL_LUN
	domain_device *device;
	MV_Sense_Data *sense_data;
#endif

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
#ifdef SUPPORT_BALDUR
    /* handle over/underflow */
    if (err_info && 
        (MV_LE32_TO_CPU(err_info->err_info_field_1) & DATA_OVR_UNDR_FLW_ERR)) {
        sas_process_over_under_flow(root, req, err_info);
#ifdef ATHENA_IGNORE_OVERFLOW_ERR
        if(!SCSI_IS_READ (req->Cdb[0]) && !SCSI_IS_WRITE (req->Cdb[0]))
            err_info->err_info_field_1 &= MV_CPU_TO_LE32(~((MV_U32)DATA_OVR_UNDR_FLW_ERR));
#endif
    }
    if (err_info && 
        ((err_info->err_info_field_1 != 0) || (err_info->err_info_field_2 != 0)))
#else
    if (err_info)
#endif
     {
	CORE_EH_PRINT(("Device %d MV_Request: Cdb[%02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x].\n",
		req->Device_Id,
		req->Cdb[0],
		req->Cdb[1],
		req->Cdb[2],
		req->Cdb[3],
		req->Cdb[4],
		req->Cdb[5],
		req->Cdb[6],
		req->Cdb[7],
		req->Cdb[8],
		req->Cdb[9],
		req->Cdb[10],
		req->Cdb[11],
		req->Cdb[12],
		req->Cdb[13],
		req->Cdb[14],
		req->Cdb[15]
		));
		/* error information record has some fields set. */
#ifdef ATHENA_PERFORMANCE_TUNNING
            if(req->Req_Flag & REQ_FLAG_BYPASS_HYBRID){
                return;
            }
#endif
		OSSW_SPIN_LOCK( &dev->base.base_SpinLock, flags);
		sas_process_command_error(
			root, &dev->base, cmd_table, err_info, req);
		OSSW_SPIN_UNLOCK( &dev->base.base_SpinLock, flags);
		return;
	}

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE)) {
		switch (req->Cdb[2])
		{
		}
	}

	resp_iu = &cmd_table->status_buff.data.ssp_resp;
	/* if response frame is not transferred, success */
	/* if response frame is transfered and status is GOOD, success */
	if (!(cmpl_q & RXQ_RSPNS_XFRD) || (cmpl_q & RXQ_RSPNS_XFRD && \
		(resp_iu->status==SCSI_STATUS_GOOD))) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
#if 0
	/* For device with protect information, os will caulate the sector size, not here.*/
		if ((req->Cdb[0] == SCSI_CMD_READ_CAPACITY_10) &&
			(req->Cmd_Initiator != root->core)) {
			MV_U32 sector_size;
           		MV_U8 *buf;
			if (dev->setting & DEVICE_SETTING_PI_ENABLED) {
				sector_size = dev->sector_size - sizeof(protect_info_record);
				sector_size = MV_CPU_TO_BE32(sector_size);
                		buf = (MV_U8 *)core_map_data_buffer(req);
				MV_CopyMemory(&buf[4], &sector_size, 4);
                		core_unmap_data_buffer(req);
			}
		}
#endif
		return;
	}

	sense_length = MV_BE32_TO_CPU(resp_iu->sense_data_len);
	if( sense_length && (resp_iu->data_pres & SENSE_ONLY) &&
		ssp_normalize_sense(resp_iu->data, sense_length, &sshdr)) {
			CORE_EH_PRINT(("dev %d req 0x%x status 0x%x sense length %d " \
				"sense(key 0x%x, asc 0x%x ascq 0x%x).\n", \
			        dev->base.id,req->Cdb[0], resp_iu->status, sense_length, \
			sshdr.sense_key, sshdr.asc, sshdr.ascq));

	}
#ifdef ATHENA_PERFORMANCE_TUNNING
            if(req->Req_Flag & REQ_FLAG_BYPASS_HYBRID){
                MV_DPRINT(("resp_iu->status(0x%x) \n",resp_iu->status));
                return;
            }
#endif
	switch (resp_iu->status) {
	case SCSI_STATUS_CHECK_CONDITION:
        /* LSI expander workaround */
		if ((req->Cdb[0] == SCSI_CMD_REPORT_LUN) && (IS_ENCLOSURE(dev))) {
			MV_U8 *buf = core_map_data_buffer(req);

			if (buf != NULL) {
				MV_ZeroMemory(buf, req->Data_Transfer_Length);
				buf[3] = 0x08;
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				core_unmap_data_buffer(req);
				return;
			} 
			core_unmap_data_buffer(req);
		}
#ifdef SUPPORT_MUL_LUN
		else{
/* To reduce takes time of delay whole req action, using timer instead of delay for cdb[0]=SCSI_CMD_START_STOP_UNIT of func: ssp_process_command */
#if 0
			if (req->Cdb[0] == SCSI_CMD_START_STOP_UNIT) {
				HBA_SleepMillisecond(NULL,500);
			}
#endif
			sense_data = (MV_Sense_Data *)&resp_iu->data[0];
			if(sense_data->SenseKey == SCSI_SK_UNIT_ATTENTION ){
				if((sense_data->AdditionalSenseCode== 0x3F) && (sense_data->AdditionalSenseCodeQualifier ==0x0e)){
					CORE_PRINT(("REPORT LUN DATA Changed\n"));
					device =(domain_device *) get_device_by_id(root->lib_dev,
						get_id_by_targetid_lun(core, get_device_targetid(root->lib_dev, req->Device_Id), 0), MV_FALSE, ID_IS_OS_TYPE(req));
					if(device == NULL)
						break;
					if(device->state == DEVICE_STATE_INIT_DONE){
						device->state = DEVICE_STATE_REPORT_LUN;
						//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
						OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
						core_queue_init_entry(root, &device->base, MV_FALSE);
						//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
						OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
					}
				}
			}
		}
#endif

		if (parse_sense_error(resp_iu->data, sense_length)) {
			req->Scsi_Status = REQ_STATUS_ERROR;
			ctx->error_info |= EH_INFO_NEED_RETRY;
		} else {
			sense_length = MV_MIN(req->Sense_Info_Buffer_Length,
				sense_length);
			if (sense_length != 0) {
				req->Scsi_Status = REQ_STATUS_HAS_SENSE;

				MV_CopyMemory(req->Sense_Info_Buffer,
					resp_iu->data, sense_length);
			} else {
				req->Scsi_Status = REQ_STATUS_ERROR;
			}
		}
		break;

	case SCSI_STATUS_FULL:
		/* retry on queue full */
		if (dev->base.outstanding_req > 1) {
			dev->base.queue_depth = dev->base.outstanding_req - 1;
		} else {
			dev->base.queue_depth = 1;
		}

		CORE_DPRINT(("set queue depth to %d.\n", dev->base.queue_depth));
		req->Scsi_Status = REQ_STATUS_ERROR;
		ctx->error_info |= EH_INFO_NEED_RETRY;
		break;

	default:
		req->Scsi_Status = REQ_STATUS_ERROR;
		break;
	}

        if (req->Scsi_Status != REQ_STATUS_SUCCESS)
                core_generate_error_event(root->core, req);
}

extern MV_VOID sata_prepare_command_header(MV_Request *req,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_U8 tag, MV_U8 pm_port);
extern MV_VOID sata_prepare_command_table(MV_Request *req,
	mv_command_table *cmd_table, ata_taskfile *taskfile, MV_U8 pm_port);

MV_VOID stp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p, MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	ata_taskfile taskfile;
	MV_BOOLEAN ret;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U8 tag;
	MV_U64 val64;

	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		sata_get_ncq_tag(device, req);
		tag = ctx->ncq_tag;
	} else {
		tag = req->Tag;
	}

	ret = ata_fill_taskfile(device, req, tag, &taskfile);

	if (!ret) {
		// HARRIET_TBD
		MV_DASSERT(MV_FALSE);
	}

	sata_prepare_command_header(req, cmd_header, cmd_table, tag,
		device->pm_port);
	sata_prepare_command_table(req, cmd_table, &taskfile, device->pm_port);

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_STP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
        /* Clears the Feature bits */
        ((MV_PU8)&cmd_table->open_address_frame)[1] = device->negotiated_link_rate;
	*(MV_U16 *)cmd_table->open_address_frame.connect_tag =
		MV_CPU_TO_BE16(device->base.id+1);
	U64_ASSIGN(val64, MV_CPU_TO_BE64(device->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID stp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info) {
		CORE_EH_PRINT(("dev %d, req %p has error info.\n", req->Device_Id, req));
		MV_DumpRequest(req, 0);
		/* error information record has some fields set. */
		sas_process_command_error(
			root, &dev->base, cmd_table, err_info, req);
		return;
	}

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return;
}

MV_U8 smp_translate_reset_sata_phy_req(pl_root *root,
	domain_expander *exp, MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx, *phy_ctx;
        domain_device *tmp_dev;

	ctx = req->Context[MODULE_CORE];
	if (ctx->type != CORE_CONTEXT_TYPE_RESET_SATA_PHY) MV_ASSERT(MV_FALSE);
	ctx->u.smp_reset_sata_phy.req_remaining = 0;
        req->Scsi_Status = REQ_STATUS_SUCCESS;

	for ( ; ctx->u.smp_reset_sata_phy.curr_dev_count <
                ctx->u.smp_reset_sata_phy.total_dev_count;
		ctx->u.smp_reset_sata_phy.curr_dev_count++) {

		tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device,
                        base.exp_queue_pointer);
                if (IS_STP(tmp_dev)) {
                        new_req = smp_make_phy_crtl_by_id_req(exp,
			        tmp_dev->parent_phy_id,
			        LINK_RESET,
					smp_physical_req_callback);
                } else {
                        List_AddTail(&tmp_dev->base.exp_queue_pointer, &exp->device_list);
                        continue;
                }

                if (new_req) {
                        List_AddTail(&tmp_dev->base.exp_queue_pointer, &exp->device_list);

	                phy_ctx = (core_context *)new_req->Context[MODULE_CORE];
	                phy_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	                phy_ctx->u.org.org_req = req;

	                if (CORE_IS_INIT_REQ(ctx))
		                core_append_init_request(root, new_req);
	                else if (CORE_IS_EH_REQ(ctx))
		                core_queue_eh_req(root, new_req);
	                else
		                core_append_request(root, new_req);

	                ctx->u.smp_reset_sata_phy.req_remaining++;

                } else {
                        List_Add(&tmp_dev->base.exp_queue_pointer, &exp->device_list);

	                if (ctx->u.smp_reset_sata_phy.req_remaining != 0) {
		                return MV_QUEUE_COMMAND_RESULT_REPLACED;
	                } else {
		                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	                }
                }
        }

	if (ctx->u.smp_reset_sata_phy.req_remaining > 0)
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 smp_translate_clear_aff_all_req(pl_root *root,
	domain_expander *exp, MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx, *phy_ctx;
	domain_device *tmp_dev;
	MV_U8 state;

	ctx = req->Context[MODULE_CORE];
	MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_CLEAR_AFFILIATION);
	ctx->u.smp_clear_aff.req_remaining = 0;
	state = ctx->u.smp_clear_aff.state;
        req->Scsi_Status = REQ_STATUS_SUCCESS;

        for ( ; ctx->u.smp_clear_aff.curr_dev_count <
                ctx->u.smp_clear_aff.total_dev_count;
                ctx->u.smp_clear_aff.curr_dev_count++) {

                switch (state) {
                case CLEAR_AFF_STATE_REPORT_PHY_SATA:

                        tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device,
                                base.exp_queue_pointer);
                        if (IS_STP(tmp_dev)) {
                                new_req = smp_make_report_phy_sata_by_id_req(exp,
											tmp_dev->parent_phy_id,
											smp_physical_req_callback);
                                if (new_req) {
                                        List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                                &exp->device_list);
                                } else {
                                        List_Add(&tmp_dev->base.exp_queue_pointer,
                                                &exp->device_list);
                                }
                        } else {
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                                continue;
                        }
	                break;

                case CLEAR_AFF_STATE_CLEAR_AFF:
                        tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device,
                                base.exp_queue_pointer);
                        if (tmp_dev->status & DEVICE_STATUS_NEED_RESET) {
                                new_req = smp_make_phy_crtl_by_id_req(exp,
			                tmp_dev->parent_phy_id,
			                HARD_RESET,
			                smp_physical_req_callback);
                        } else if (tmp_dev->status & DEVICE_STATUS_NEED_CLEAR) {
		                new_req = smp_make_phy_crtl_by_id_req(exp,
			                tmp_dev->parent_phy_id,
			                CLEAR_AFFILIATION,
			                smp_physical_req_callback);
                        } else {
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                                continue;
                        }

                        if (new_req) {
                                tmp_dev->status &= ~(DEVICE_STATUS_NEED_RESET |
                                        DEVICE_STATUS_NEED_CLEAR);
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                        } else {
                                List_Add(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                        }
	                ctx->u.smp_clear_aff.need_wait = MV_TRUE;
	                break;
                default:
	                MV_ASSERT(MV_FALSE);
	                break;
                }

	        if (new_req) {
		        phy_ctx = (core_context *)new_req->Context[MODULE_CORE];
		        phy_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
		        phy_ctx->u.org.org_req = req;

		        if (CORE_IS_INIT_REQ(ctx))
			        core_append_init_request(root, new_req);
		        else if (CORE_IS_EH_REQ(ctx))
			        core_queue_eh_req(root, new_req);
		        else
			        core_append_request(root, new_req);

		        ctx->u.smp_clear_aff.req_remaining++;
	        } else {
		        if (ctx->u.smp_clear_aff.req_remaining != 0) {
			        return MV_QUEUE_COMMAND_RESULT_REPLACED;
		        } else {
			        return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		        }
                }
        }

	if (ctx->u.smp_clear_aff.req_remaining > 0)
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 smp_translate_phy_req(pl_root *root, domain_expander *exp,
	MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx;
	smp_virtual_config_route_buffer *buf;
	MV_U8 i, j, phy_id;

	switch (req->Cdb[2]) {
	case CDB_CORE_SMP_VIRTUAL_DISCOVER:
		ctx = req->Context[MODULE_CORE];
		CORE_DPRINT(("discover: phy count = %d\n", exp->phy_count));
		ctx->u.smp_discover.req_remaining = 0;
		req->Scsi_Status = REQ_STATUS_SUCCESS;
#ifdef EXP_DONT_DISCOVER_FINAL_DEV
		for ( ; ctx->u.smp_discover.current_phy_id < (exp->phy_count-1);
			ctx->u.smp_discover.current_phy_id++) 
#else
		for ( ; ctx->u.smp_discover.current_phy_id < exp->phy_count;
			ctx->u.smp_discover.current_phy_id++) 
#endif
		{
			new_req = smp_make_discover_req(exp, req,
				ctx->u.smp_discover.current_phy_id, smp_physical_req_callback);
			if (new_req) {
				core_append_init_request(root, new_req);
				ctx->u.smp_discover.req_remaining++;
			} else {
				if (ctx->u.smp_discover.req_remaining != 0) {
					return MV_QUEUE_COMMAND_RESULT_REPLACED;
				} else {
					return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
				}
			}
		}
		if (ctx->u.smp_discover.req_remaining == 0)
			MV_ASSERT(MV_FALSE);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case CDB_CORE_SMP_VIRTUAL_CONFIG_ROUTE:
		ctx = req->Context[MODULE_CORE];
		buf = (smp_virtual_config_route_buffer *)core_map_data_buffer(req);

		CORE_DPRINT(("smp config: phy %d/%d addr %d/%d.\n",\
			ctx->u.smp_config_route.current_phy, \
			ctx->u.smp_config_route.phy_count,\
			ctx->u.smp_config_route.current_addr, \
			ctx->u.smp_config_route.address_count));
		ctx->u.smp_config_route.req_remaining = 0;
                req->Scsi_Status = REQ_STATUS_SUCCESS;
		for (i=ctx->u.smp_config_route.current_phy;
			i<ctx->u.smp_config_route.phy_count; i++) {
			phy_id = buf->phy_id[i];

			for (j=ctx->u.smp_config_route.current_addr;
				j<ctx->u.smp_config_route.address_count; j++) {

				new_req = smp_make_config_route_req(exp, req,
					exp->route_table[phy_id], phy_id, &buf->sas_addr[j],
					smp_physical_req_callback);
				if (new_req) {
					exp->route_table[phy_id]++;
					core_append_init_request(root, new_req);
					ctx->u.smp_config_route.req_remaining++;
				} else {
					ctx->u.smp_config_route.current_phy = i;
					ctx->u.smp_config_route.current_addr = j;
                                        core_unmap_data_buffer(req);
					if (ctx->u.smp_config_route.req_remaining == 0) {
						return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
					} else {
						return MV_QUEUE_COMMAND_RESULT_REPLACED;
					}
				}
			}
			ctx->u.smp_config_route.current_addr = 0;
		}
		ctx->u.smp_config_route.current_phy = ctx->u.smp_config_route.phy_count;
		ctx->u.smp_config_route.current_addr = ctx->u.smp_config_route.address_count;
                core_unmap_data_buffer(req);
		if (ctx->u.smp_config_route.req_remaining == 0)
			MV_ASSERT(MV_FALSE);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case CDB_CORE_SMP_VIRTUAL_RESET_SATA_PHY:
		return (smp_translate_reset_sata_phy_req(root, exp, req));
	case CDB_CORE_SMP_VIRTUAL_CLEAR_AFFILIATION_ALL:
		return (smp_translate_clear_aff_all_req(root, exp,req));
	default:
		break;
	}

	req->Scsi_Status = REQ_STATUS_NO_DEVICE;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 smp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE) &&
		(req->Cdb[2] == CDB_CORE_RESET_PORT)) {
			mv_reset_phy(root, req->Cdb[3], MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE) &&
		(req->Cdb[2] != CDB_CORE_SMP)) {

		return (smp_translate_phy_req(root, exp, req));
	} else {

#if defined(SUPPORT_BALDUR) && defined(SUPPORT_ROC)
		if ((req->Cdb[0] != SCSI_CMD_MARVELL_SPECIFIC) || (req->Cdb[2] != CDB_CORE_SMP))
		{
			req->Scsi_Status = REQ_STATUS_NO_DEVICE;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
#endif

		if (req->Data_Buffer == NULL) {
			CORE_DPRINT(("smp req no data buffer.\n"));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	}
	
	/* only get a register set if this expander doesn't already have one */
	if ((((core_extension *)(root->core))->has_port_queue) 
		&& (exp->register_set == NO_REGISTER_SET)) {
		MV_U8 set = core_get_register_set(root, exp);
		if (set == NO_REGISTER_SET)
			return MV_QUEUE_COMMAND_RESULT_FULL;
		else
			exp->register_set = set;
	}
	
	return MV_QUEUE_COMMAND_RESULT_PASSED;
}

MV_U32 smp_get_request_len(MV_U8 function)
{
	MV_U32 req_len = 4;

	switch (function) {
	case REPORT_GENERAL:
	case REPORT_MANUFACTURER_INFORMATION:
		req_len += sizeof(struct SMPRequestGeneralInput) - sizeof(MV_U32);
		break;
	case REPORT_SELF_CONFIGURATION_STATUS:
		req_len += sizeof(struct SMPRequestSelfConfigurationInput) -
			sizeof(MV_U32);
		break;
	case DISCOVER:
	case REPORT_PHY_ERROR_LOG:
	case REPORT_PHY_SATA:
	case REPORT_PHY_EVENT_INFORMATION:
	case REPORT_PHY_BROADCAST_COUNTS:
		req_len += sizeof(struct SMPRequestPhyInput) - sizeof(MV_U32);
		break;
	case REPORT_ROUTE_INFORMATION:
		req_len += sizeof(struct SMPRequestRouteInformationInput) -
			sizeof(MV_U32);
		break;
	case DISCOVER_LIST:
		req_len += sizeof(struct SMPRequestDiscoverList) - sizeof(MV_U32);
		break;
	case REPORT_EXPANDER_ROUTE_TABLE:
		req_len += sizeof(struct SMPRequestReportExpanderRouteTable) -
			sizeof(MV_U32);
		break;
	case CONFIGURE_GENERAL:
		req_len += sizeof(struct SMPRequestConfigureGeneral) - sizeof(MV_U32);
		break;
	case ENABLE_DISABLE_ZONING:
		req_len += sizeof(struct SMPRequestEnableDisableZoning) -
			sizeof(MV_U32);
		break;
	case ZONED_BROADCAST:
		req_len += sizeof(struct SMPRequestZonedBroadcast) - sizeof(MV_U32);
		break;
	case CONFIGURE_ROUTE_INFORMATION:
		req_len += sizeof(struct SMPRequestConfigureRouteInformation) -
			sizeof(MV_U32);
		break;
	case PHY_CONTROL:
		req_len += sizeof(struct SMPRequestPhyControl) - sizeof(MV_U32);
		break;
	case PHY_TEST:
		req_len += sizeof(struct SMPRequestPhyTest) - sizeof(MV_U32);
		break;
	case CONFIGURE_PHY_EVENT_INFORMATION:
		req_len += sizeof(struct SMPRequestConfigurePhyEventInformation) -
			sizeof(MV_U32);
		break;
	}

	return req_len;
}

MV_U32 smp_get_response_len(MV_U8 function)
{
	MV_U32 resp_len = 4;

	switch (function) {
	case REPORT_GENERAL:
		resp_len += sizeof(struct SMPResponseReportGeneral) - sizeof(MV_U32);
		break;
	case REPORT_MANUFACTURER_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportManufacturerInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_SELF_CONFIGURATION_STATUS:
		resp_len += sizeof(struct SMPResponseReportSelfConfigurationStatus) -
			sizeof(MV_U32);
		break;
	case DISCOVER:
		resp_len += sizeof(struct SMPResponseDiscover) - sizeof(MV_U32);
		break;
	case REPORT_PHY_ERROR_LOG:
		resp_len += sizeof(struct SMPResponseReportPhyErrorLog) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_SATA:
		resp_len += sizeof(struct SMPResponseReportPhySATA) - sizeof(MV_U32);
		break;
	case REPORT_ROUTE_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportRouteInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_EVENT_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportPhyEventInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_BROADCAST_COUNTS:
		resp_len += sizeof(struct SMPResponseReportBroadcastCounts) -
			sizeof(MV_U32);
		break;
	case DISCOVER_LIST:
		resp_len += sizeof(struct SMPResponseDiscoverList) - sizeof(MV_U32);
		break;
	case REPORT_EXPANDER_ROUTE_TABLE:
		resp_len += sizeof(struct SMPResponseReportExpanderTable) -
			sizeof(MV_U32);
		break;
	case CONFIGURE_GENERAL:
	case ENABLE_DISABLE_ZONING:
	case ZONED_BROADCAST:
	case CONFIGURE_ROUTE_INFORMATION:
	case PHY_CONTROL:
	case PHY_TEST:
	case CONFIGURE_PHY_EVENT_INFORMATION:
		resp_len += sizeof(struct SMPResponseConfigureFunction) -
			sizeof(MV_U32);
		break;
	}

	return resp_len;
}

MV_VOID smp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	MV_PVOID buf_ptr = core_map_data_buffer(req);
	smp_request *smp_req = (smp_request *)buf_ptr;
	MV_U32 req_len;
	MV_U64 val64;

	/* clear sata ctrl and sas ctrl */
	cmd_header->max_rsp_frame_len = MAX_RESPONSE_FRAME_LENGTH >> 2;
	req_len = smp_get_request_len(smp_req->function);
	
	cmd_header->frame_len = MV_CPU_TO_LE16((MV_U16)(req_len+3)/4);
#ifdef SUPPORT_BALDUR
	cmd_header->max_sim_conn = 1;
#endif
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SMP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) = MV_CPU_TO_BE16(0xffff);
	((MV_PU8)&cmd_table->open_address_frame)[1] = exp->neg_link_rate;
	U64_ASSIGN(val64, MV_CPU_TO_BE64(exp->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr,
		&val64, 8);

	MV_CopyMemory(&cmd_table->table.smp_cmd_table, buf_ptr, req_len);
	core_unmap_data_buffer(req);
}
MV_VOID
smp_send_command(
    MV_PVOID root_p,
    MV_PVOID dev_p,
    MV_PVOID struct_wrapper,
    MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	domain_port *port = exp->base.port;
	core_context *ctx = req->Context[MODULE_CORE];
    	hw_buf_wrapper *wrapper = (hw_buf_wrapper *) struct_wrapper;
	MV_U8 phy_map;
	MV_U32 entry_nm;

	entry_nm = prot_get_delv_q_entry(root, 0);
	phy_map = port->asic_phy_map; /* from Phy 0 */
	
#ifdef ATHENA2_Z1_WIDE_PORT_MULTI_PORT_ISSUE
	if((port->asic_phy_map==0xf0)||(port->asic_phy_map==0x10))	
		phy_map = 0x10;	//set port 4 to transmit
	else if((port->asic_phy_map==0x0f)||(port->asic_phy_map==0x01))
		phy_map = 0x1;	//set port 0 to transmit
#endif
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_ASSERT(wrapper != NULL);
	root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW0=MV_CPU_TO_LE32( TXQ_MODE_I | ctx->slot | TXQ_CMD_SMP|(exp->register_set <<TXQ_REGSET_SHIFT));
	#ifdef TEST_COMMAND_STRUCTURE_IN_SRAM
	root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map << TXQ_PORT_SHIFT) | INTRFC_SRAM);
	#else
	root->queues[0].delv_q[entry_nm].Delivery_Queue_Entry_DW1= MV_CPU_TO_LE32((phy_map << TXQ_PORT_SHIFT) | INTRFC_PCIEA);
	#endif
	root->queues[0].delv_q[entry_nm].Cmd_Struct_Addr = MV_CPU_TO_LE32(wrapper->phy.parts.low);
	root->queues[0].delv_q[entry_nm].Cmd_Struct_Addr_Hi= MV_CPU_TO_LE32(wrapper->phy.parts.high);
#else
	root->queues[0].delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
		phy_map << TXQ_PHY_SHIFT | TXQ_CMD_SMP);
#endif
	
	prot_write_delv_q_entry(root, entry_nm, 0);
}

MV_VOID smp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;
	smp_response *smp_resp =
		(smp_response *)cmd_table->status_buff.data.smp_resp;
	MV_U32 resp_len;
	MV_PVOID buf_ptr;

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info) {
		CORE_EH_PRINT(("dev %d has error info.\n", req->Device_Id));
		/* error information record has some fields set. */
		sas_process_command_error(
			root, &exp->base, cmd_table, err_info, req);
		return;
	}

	if (smp_resp->function_result == SMP_FUNCTION_ACCEPTED) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
                buf_ptr = core_map_data_buffer(req);
		if (buf_ptr != NULL) {
			if (cmd_table->status_buff.data.smp_resp[3] == 0) {
				/* forward compatible */
				resp_len = smp_get_response_len(smp_resp->function);
			} else {
				/* get response len from RESPONSE LENGTH field of response data */
				resp_len =4;
				resp_len += cmd_table->status_buff.data.smp_resp[3] * 4;
			}
			resp_len = (resp_len < req->Data_Transfer_Length) ? resp_len :
				req->Data_Transfer_Length;
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
			MV_CopyMemory(buf_ptr, &cmd_table->status_buff.data.smp_resp,
				resp_len);
#endif
		}
		core_unmap_data_buffer(req);
	} else {
        #if 0
		/*
		 * any error handling we want to do? most of them seems like it's
		 * caused by issuing a request incorrectly, or zoning-related.
		 */
		req->Scsi_Status = REQ_STATUS_ERROR;
        #else
                /* as long as the error information record has no error, it's ok.
                 * callback function should handle the SMP function result */
                req->Scsi_Status = REQ_STATUS_SUCCESS;

                /* We need allow the different SMP function result like SMP_PHY_VACANT */
                #ifdef CORE_EH_LOG_EVENTS
                {
                        MV_U32 params[MAX_EVENT_PARAMS];
                        params[0] = CORE_EVENT;			/* should be CDB */
                        params[1] = CORE_EVENT_SMP_FUNCTION_ERROR; /* Sense Key */
                        params[2] = smp_resp->function;  /* Additional Sense Code */
                        params[3] = smp_resp->function_result; /* Additional Sense Code Qualifier */
                        core_generate_event(root->core, EVT_ID_HD_SC_ERROR, exp->base.id,
	                        SEVERITY_INFO, 4, params, 0);
	        }
                #endif
        #endif
	}
}


