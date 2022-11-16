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
#include "core_manager.h"
#include "core_err_inj.h"
#ifdef SCSI_ID_MAP
#include "hba_inter.h"
#endif
MV_U32 lib_err_inj_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	if ( max_io>1 ) {
		// during normal mode only, don't need this for hibernation
		size += sizeof(injected_error) * ERROR_INJECTION_COUNT;
	}
	return size;
}

MV_U32 lib_err_inj_page_get_dma_memory_quota(MV_U16 max_io)
{
	return 0;
}

MV_U8 lib_err_inj_initialize(MV_PVOID core_p,
        lib_resource_mgr *rsrc, MV_U16 max_io)
{
	core_extension *core = (core_extension *)core_p;
	MV_U16 i;

	MV_LIST_HEAD_INIT(&core->lib_error.free_error_list);

	if (max_io > 1){
		injected_error *err;
		MV_U32 item_size = sizeof(injected_error) * ERROR_INJECTION_COUNT;
		MV_PU8 vir =  (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);

        	if (vir == NULL)
			return MV_FALSE;

		for ( i=0; i<ERROR_INJECTION_COUNT; i++ ){
			err = (injected_error *)vir;
			List_AddTail(&err->queue_pointer, &core->lib_error.free_error_list);
			vir += sizeof(injected_error);
		}
	}

	/* Init device data structure
	for (i=0; i<core->lib_rsrc.hd_count; i++){
		MV_LIST_HEAD_INIT(&dev->injected_error_list);
		dev = (domain_device *)dev->base.queue_pointer.next;
	}*/
	return MV_TRUE;

}
injected_error *get_inject_error_from_pool(MV_PVOID core_p)
{
	injected_error *result;
	core_extension *core = (core_extension *)core_p;
	if ( List_Empty(&core->lib_error.free_error_list) ) return NULL;

	result = (injected_error *)List_GetFirstEntry(&core->lib_error.free_error_list, injected_error, queue_pointer);

	return result;
}

/* Release the buffer and set the pointer to NULL. */
void free_inject_error_to_pool(MV_PVOID core_p, injected_error *inject_error )
{
	core_extension *core = (core_extension *)core_p;
	List_AddTail( &inject_error->queue_pointer, &core->lib_error.free_error_list );
}


void check_error_injection(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	domain_device *device;
	MV_U16 deviceId = req->Device_Id;
	injected_error *err;
	MV_LBA err_start, err_end;
	MV_LBA req_start, req_end;

	device = (domain_device *)get_device_by_id(&core->lib_dev,
				req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	if ((device == NULL)||
		(device->base.type != BASE_TYPE_DOMAIN_DEVICE))
		return;

	if ( List_Empty(&device->injected_error_list) )
		return;

	if ( !(req->Req_Flag&REQ_FLAG_LBA_VALID) ) {
		MV_SetLBAandSectorCount(req);
	}
	req_start = req->LBA;
	req_end = U64_ADD_U32(req->LBA, req->Sector_Count);

	LIST_FOR_EACH_ENTRY_TYPE(err, &device->injected_error_list, injected_error, queue_pointer){
		err_start = err->lba;
		err_end = U64_ADD_U32(err->lba, err->count);

		/* Check whether these two ranges are overlapped. */
		if ( (U64_COMPARE_U64(req_start, err_end) < 0) &&
			 (U64_COMPARE_U64(req_end, err_start) > 0) )
		{
			if ( ((err->req_type & DBG_REQUEST_READ) && SCSI_IS_READ(req->Cdb[0]))||
				((err->req_type & DBG_REQUEST_WRITE) && SCSI_IS_WRITE(req->Cdb[0]) )||
				((err->req_type & DBG_REQUEST_VERIFY) && SCSI_IS_VERIFY(req->Cdb[0])) )
			{
				req->Scsi_Status = err->error_status;
				break;
			}
		}
	}
}


