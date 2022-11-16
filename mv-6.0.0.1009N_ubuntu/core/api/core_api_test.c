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
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_api.h"
#include "core_util.h"
#include "core_err_inj.h"
#include "core_console.h"

#include "core_sas.h"
#include "core_error.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
extern MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);

extern MV_Request *smp_make_phy_crtl_by_id_req(MV_PVOID exp_p,
MV_U8 phy_id, MV_U8 operation, MV_PVOID callback);

#ifdef SUPPORT_PASS_THROUGH_DIRECT
void
do_test_pdwr_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	PMV_Request org_req = (PMV_Request)req->Org_Req;

	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		if (req->Cdb[0] == SCSI_CMD_READ_10) {
			CORE_PRINT(("doTest success:reading from PD startLBA=0x%x .\n",\
				req->LBA.parts.low));
		}
		else if (req->Cdb[0] == SCSI_CMD_WRITE_10) {
			CORE_PRINT(("doTest success:writing to PD startLBA=0x%x .\n",\
				req->LBA.parts.low));
		}
		org_req->Scsi_Status = REQ_STATUS_SUCCESS;
	} else {
		/* For SAS device, because req has no sense buffer, it'll return REQ_STATUS_ERROR_WITH_SENSE.
		 * API doesn't know there is any error checking from org_req->Sense_Buffer. */
		org_req->Scsi_Status = REQ_STATUS_MEDIA_ERROR;
	}

	core_queue_completed_req(root->core, org_req);
}

MV_U8
core_dbg_request_pdrw(
	IN core_extension *core,
	IN PMV_Request req)
{
	PMV_Request new_req;
	PMV_SG_Table sgtable;
	PDBG_Data dbg_buf;
        MV_PVOID buf_ptr = core_map_data_buffer(req);
	MV_U8 status = REQ_STATUS_SUCCESS;
	MV_U16 device_id, index;
	domain_base *base = NULL;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Data)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	MV_CopyMemory(&device_id, &req->Cdb[2], 2);
	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);

	if ((base == NULL) || (base->type != BASE_TYPE_DOMAIN_DEVICE)) {
		status = ERR_INVALID_HD_ID;
	} else {
		new_req = get_intl_req_resource(base->root, 0);

		if (new_req == NULL) {
			core_unmap_data_buffer(req);
                        return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		}

		sgtable = &new_req->SG_Table;
		dbg_buf =(PDBG_Data)buf_ptr;
		new_req->Device_Id = device_id;

		MV_CopyMemory(&new_req->LBA, &dbg_buf->LBA, 8);
		new_req->Sector_Count = dbg_buf->NumSectors;
		new_req->Data_Transfer_Length = new_req->Sector_Count * SECTOR_LENGTH;
		new_req->Data_Buffer = dbg_buf->Data;
		new_req->Cdb[0] = (req->Cdb[4] == SECTOR_READ) ?
			SCSI_CMD_READ_10 : SCSI_CMD_WRITE_10;

		SCSI_CDB10_SET_LBA(new_req->Cdb, new_req->LBA.value);
		SCSI_CDB10_SET_SECTOR(new_req->Cdb, new_req->Sector_Count);

		/* Construct SG table */
		//////////////////

		if (new_req->Data_Transfer_Length > 0) {
			MV_CopyPartialSGTable(
				&new_req->SG_Table,
				&req->SG_Table,
				12, /* offset */
				new_req->Data_Transfer_Length /* size */
				);
		}
		//////////////////

		/* set command flag */
		new_req->Cmd_Flag = 0;
		if (new_req->Data_Transfer_Length!=0) {
			new_req->Cmd_Flag |= CMD_FLAG_DMA;
			if (new_req->Cdb[0] == SCSI_CMD_READ_10)
				new_req->Cmd_Flag |= CMD_FLAG_DATA_IN;
		}

		new_req->Cmd_Initiator = base->root;
		new_req->Org_Req = req;

		new_req->Completion = do_test_pdwr_callback;
		/* Send this internal request */
                core_unmap_data_buffer(req);
		core_append_request(base->root, new_req);
	}

	if (status != REQ_STATUS_SUCCESS) {
		if (req->Sense_Info_Buffer != NULL) {
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		}
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		/* finished */
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	} else {
		/* not finished yet */
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
	}
}
#endif	/* SUPPORT_PASS_THROUGH_DIRECT */


MV_U8 core_dbg_request_add_core_error(core_extension *core, PMV_Request req)
{
	domain_base *base = NULL;
	domain_device *device = NULL;
	PDBG_Error_Injection dbg_err;
	injected_error *err;
	MV_U16 device_id, index;
	MV_U8 status = REQ_STATUS_SUCCESS;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Error_Injection)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	MV_CopyMemory(&device_id, &req->Cdb[2], 2);
	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);
	if ((base == NULL)||(base->type != BASE_TYPE_DOMAIN_DEVICE))
		status = ERR_INVALID_HD_ID;
	else {
		device = (domain_device *)base;

		dbg_err = (PDBG_Error_Injection)buf_ptr;
		err = get_inject_error_from_pool(core);
		if (err == NULL) {
			status = ERR_NO_RESOURCE;
		} else {
			MV_CopyMemory(&err->lba, &dbg_err->LBA, 8);
			err->count = dbg_err->Count;
			err->device_id = device_id;
			err->error_status = dbg_err->Error_Status;
			err->req_type = dbg_err->Request_Type;

			List_AddTail(&err->queue_pointer, &device->injected_error_list);
		}
	}        
	core_unmap_data_buffer(req);

	if (status != REQ_STATUS_SUCCESS){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	}else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 core_dbg_request_remove_core_error(core_extension *core, PMV_Request req)
{
	domain_base *base = NULL;
	domain_device *device = NULL;
	PDBG_Error_Injection dbg_err;
	injected_error *err;
	MV_U16 device_id, index;
	MV_U8 status = REQ_STATUS_SUCCESS;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Error_Injection)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	MV_CopyMemory(&device_id, &req->Cdb[2], 2);
	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);
	if((base == NULL)||(base->type != BASE_TYPE_DOMAIN_DEVICE))
		status = ERR_INVALID_HD_ID;
	else{
		device = (domain_device *)base;

		dbg_err = (PDBG_Error_Injection)buf_ptr;
		LIST_FOR_EACH_ENTRY_TYPE(err, &device->injected_error_list, injected_error, queue_pointer){
			if ((err->lba.value == dbg_err->LBA.value) && (err->count == dbg_err->Count)){
				List_Del(&err->queue_pointer);
				free_inject_error_to_pool(core, err);
				break;
			}
		}
	}
	core_unmap_data_buffer(req);

	if (status != REQ_STATUS_SUCCESS){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	}else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 core_dbg_request_remove_all_core_error(core_extension *core, PMV_Request req)
{
	domain_base *base = NULL;
	domain_device *device = NULL;
	injected_error *err;
	MV_U16 device_id, index;
	MV_U8 status = REQ_STATUS_SUCCESS;

	MV_CopyMemory(&device_id, &req->Cdb[2], 2);
	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);
	if((base == NULL)||(base->type != BASE_TYPE_DOMAIN_DEVICE))
		status = ERR_INVALID_HD_ID;
	else{
		device = (domain_device *)base;

		while (!List_Empty(&device->injected_error_list)){
			err = (injected_error *)List_GetFirstEntry(&device->injected_error_list, injected_error, queue_pointer);
			free_inject_error_to_pool(core, err);
		}
	}

	if (status != REQ_STATUS_SUCCESS){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	}else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 core_dbg_request_get_core_error(core_extension *core, PMV_Request req)
{
	domain_base *base = NULL;
	domain_device *device = NULL;
	PDBG_Error_Injection dbg_err;
	injected_error *err;
	MV_U16 device_id, index;
	MV_U8 status = REQ_STATUS_SUCCESS;
	MV_PVOID buf_ptr = core_map_data_buffer(req);

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Error_Injection)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	MV_CopyMemory(&device_id, &req->Cdb[2], 2);
	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);
	if((base == NULL)||(base->type != BASE_TYPE_DOMAIN_DEVICE))
		status = ERR_INVALID_HD_ID;
	else{
		device = (domain_device *)base;

		dbg_err = (PDBG_Error_Injection)buf_ptr;
		LIST_FOR_EACH_ENTRY_TYPE(err, &device->injected_error_list, injected_error, queue_pointer)	{
			MV_CopyMemory(&dbg_err->LBA, &err->lba, 8);
			dbg_err->Count = err->count;
			dbg_err->HDID = err->device_id;
			dbg_err->Error_Status = err->error_status;
			dbg_err->Request_Type = err->req_type;
			dbg_err++;
		}
		dbg_err->HDID = 0xffff;
	}

	core_unmap_data_buffer(req);

	if (status != REQ_STATUS_SUCCESS){
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = status;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	}else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

extern void scsi_ata_check_condition(MV_Request * req, MV_U8 sense_key, MV_U8 sense_code, MV_U8 sense_qualifier);
static void ssp_abort_task_callback(pl_root *root, MV_Request *req)
{
	core_extension  *core = (core_extension *)root->core;
	MV_U16 slot = 0xffff;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_Request *org_req = ctx->u.org.org_req; 
	struct _domain_base *base = (struct _domain_base *)get_device_by_id(&core->lib_dev, org_req->Device_Id, MV_FALSE, MV_FALSE);
	struct _domain_base *abort_base = NULL;
	MV_ULONG flags,flags_queue;

	MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_ORG_REQ);
	ctx->type = CORE_CONTEXT_TYPE_NONE;
	ctx->u.org.org_req = NULL;
 
        //CORE_DPRINT(("org_reg %p, req %p, root:%p\n",org_req,req, root));
	if ((org_req == NULL) || (req == NULL)) {
        	return;
       }    
       MV_ASSERT(org_req != NULL);
       MV_ASSERT(org_req->Scsi_Status);
	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		org_req->Scsi_Status = REQ_STATUS_ERROR;
	} else {
        	org_req->Scsi_Status = REQ_STATUS_SUCCESS;
		if (org_req->Cdb[4] == TMF_ABORT_TASK) {
			MV_Request *abort_req = NULL;
			slot = (((MV_U16)(req->Cdb[4] << 8)) | ((MV_U16)(req->Cdb[3]))) & ((MV_U16)MV_BIT(root->max_cmd_slot_width)-1);
			OSSW_SPIN_LOCK(&abort_base->queue->handle_cmpl_SpinLock, flags_queue);
			abort_req = root->running_req[slot];
			MV_ASSERT((abort_req!= NULL));
			if (abort_req != NULL) {
	        		abort_base = (struct _domain_base *)get_device_by_id(&core->lib_dev, abort_req->Device_Id, MV_FALSE, MV_FALSE);
				OSSW_SPIN_LOCK(&abort_base->err_ctx.sent_req_list_SpinLock, flags);
				prot_reset_slot(root, abort_base, slot, abort_req);
				OSSW_SPIN_UNLOCK(&abort_base->err_ctx.sent_req_list_SpinLock, flags);
				abort_req->Scsi_Status = REQ_STATUS_HAS_SENSE;
				scsi_ata_check_condition(abort_req, 
					SCSI_SK_ABORTED_COMMAND, 
					SCSI_ASC_NO_ASC, 
					SCSI_ASCQ_NO_ASCQ);
		 		core_queue_completed_req(core, abort_req);
	 		} else{
	        		CORE_DPRINT(("Aborted req %p isn't existed\n", abort_req));
			}
			OSSW_SPIN_UNLOCK(&abort_base->queue->handle_cmpl_SpinLock, flags_queue);
		}
        }
	 core_queue_completed_req(core, org_req);		
	return;

}

MV_U8 core_dbg_request_abort_task(core_extension *core, PMV_Request req)
{
	MV_Request *eh_req = NULL;
	MV_Request *abort_req = NULL;
	core_context    *new_ctx;
	struct _error_context *err_ctx;
	domain_base *base = NULL;
	domain_device *device = NULL;
    	MV_U16 device_id, i,lun,target_id;
	MV_U8 status = REQ_STATUS_SUCCESS;
	pl_root *root;
       MV_U16 tag;
      MV_ULONG flags;

       target_id = req->Cdb[2];
       lun = req->Cdb[3];
       //CORE_DPRINT(("Target id 0x%x, lun 0x%x \n",target_id,lun));]
       device_id = __MAKE_DEV_ID(req->Cdb[2], req->Cdb[3]);
	base = get_device_by_id(&core->lib_dev, device_id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
  
	err_ctx = &base->err_ctx;
        
	if((base == NULL)||(base->type != BASE_TYPE_DOMAIN_DEVICE)){
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED ;
       }         
	else
		device = (domain_device *)base;
       root = base->root;

       /*Handle completed requests' interrupt before choosing the aborted req.*/  
//	io_chip_handle_cmpl_queue_int(root);
	io_chip_handle_cmpl_queue(&root->queues[device->queue_id]);

	/*Find newest req sent to the device to abort.*/
	OSSW_SPIN_LOCK(&err_ctx->sent_req_list_SpinLock, flags);
	if (!List_Empty(&err_ctx->sent_req_list)) {
		abort_req = LIST_ENTRY(
			(&err_ctx->sent_req_list)->prev, MV_Request, Queue_Pointer);
	}
	OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags);
       if((abort_req == NULL) &&(req->Cdb[4] == TMF_ABORT_TASK)) {
            CORE_DPRINT(("no running request or required requests\n"));
            req->Scsi_Status = REQ_STATUS_SUCCESS;
            return MV_QUEUE_COMMAND_RESULT_FINISHED ;
       } 
       
        //if(abort_req  != NULL)
            //CORE_DPRINT(("abort_req %p,tag 0x%x\n", abort_req, abort_req->Tag));
       eh_req = sas_make_task_req(
        root, device, abort_req, req->Cdb[4],//TMF_LOGICAL_UNIT_RESET, //TMF_ABORT_TASK_SET, //TMF_ABORT_TASK,
        (MV_ReqCompletion)ssp_abort_task_callback);
	if (eh_req == NULL) return MV_FALSE;

    	new_ctx = (core_context *)eh_req->Context[MODULE_CORE];
	new_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
    	new_ctx->u.org.org_req = req;

	//if (req->Cdb[4] == TMF_ABORT_TASK)
	//	CORE_DPRINT(("Send TMF_ABORT_TASK, tag  cdb[3] 0x%x, cdb[4] 0x%x\n",eh_req->Cdb[3],eh_req->Cdb[4]));

	core_append_high_priority_request(root, eh_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

static void smp_phy_control_callback(pl_root *root, MV_Request *req)
{
	core_extension  *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_Request *org_req = ctx->u.org.org_req; 

	MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_ORG_REQ);
	ctx->type = CORE_CONTEXT_TYPE_NONE;
	ctx->u.org.org_req = NULL;
 
	if ((org_req == NULL) || (req == NULL))
		return;
	
	MV_ASSERT(org_req != NULL);
	MV_ASSERT(org_req->Scsi_Status);

	CORE_DPRINT(("req->Scsi_Status=0x%x.\n", req->Scsi_Status));

	if (req->Scsi_Status != REQ_STATUS_SUCCESS)
		org_req->Scsi_Status = REQ_STATUS_ERROR;
	else
		org_req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_queue_completed_req(core, org_req);		
	return;
}

MV_U8 core_dbg_request_smp_phy_control(core_extension *core, PMV_Request req)
{
	MV_Request *phy_ctl_req = NULL;
	core_context    *new_ctx;
	struct _error_context *err_ctx;
	domain_base *base = NULL;
	domain_expander *exp = NULL;
	MV_U16 device_id;
	MV_U8 phy_id, operation;
	pl_root *root;

	phy_id = req->Cdb[3];
	operation = req->Cdb[4];
	device_id = __MAKE_DEV_ID(req->Cdb[2], 0);
	base = get_device_by_id(&core->lib_dev, device_id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
 
	if ((base == NULL)||(base->type != BASE_TYPE_DOMAIN_ENCLOSURE)) {
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED ;
	} else
		exp = (domain_expander *)base->parent; 

	err_ctx = &base->err_ctx;
	root = base->root;

	phy_ctl_req = smp_make_phy_crtl_by_id_req(exp, phy_id, operation, smp_phy_control_callback);
	if (phy_ctl_req == NULL)
		return MV_FALSE;

	new_ctx = (core_context *)phy_ctl_req->Context[MODULE_CORE];
	new_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	new_ctx->u.org.org_req = req;

	core_append_high_priority_request(root, phy_ctl_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

MV_U8
core_dbg_request_pdrw(
	IN core_extension *core,
	IN PMV_Request req);


core_management_command_handler BASEATTR core_dbg_cmd_handler[APICDB1_DBG_MAX] =
{
#ifdef SUPPORT_PASS_THROUGH_DIRECT
	core_dbg_request_pdrw,
#else
	NULL,
#endif
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	core_dbg_request_add_core_error,
	core_dbg_request_remove_core_error,
	core_dbg_request_remove_all_core_error,
	core_dbg_request_get_core_error,
	core_dbg_request_abort_task,
	core_dbg_request_smp_phy_control,

};

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_dbg_command(
	IN MV_PVOID extension,
	IN PMV_Request req
	)
{
	core_extension *core = (core_extension *)extension;

	if (req->Cdb[1] >= APICDB1_DBG_MAX) {
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	if (core_dbg_cmd_handler[req->Cdb[1]] == NULL) {
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	return core_dbg_cmd_handler[req->Cdb[1]](core, req);
}


