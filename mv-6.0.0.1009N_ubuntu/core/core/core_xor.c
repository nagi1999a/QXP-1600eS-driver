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
#ifdef HARDWARE_XOR
#include "core_xor.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_manager.h"
#include "core_resource.h"
#include "core_protocol.h"
#include "core_util.h"

#ifdef XOR_LOAD_BALANCE
void xor_load_balance_increment(PMV_XOR_Request xor_req, xor_engine *xor_core)
{
	MV_U32 count = xor_req->Source_SG_Table_Count
			+ xor_req->Target_SG_Table_Count;

	xor_core->xor_core_data_count +=
		xor_req->Source_SG_Table_List[0].Byte_Count * count;
}

void xor_load_balance_decrement(PMV_XOR_Request xor_req, xor_engine *xor_core)
{
	MV_U32 count = xor_req->Source_SG_Table_Count
			+ xor_req->Target_SG_Table_Count;

	xor_core->xor_core_data_count -=
		xor_req->Source_SG_Table_List[0].Byte_Count * count;
}

MV_U8 xor_load_balance_get_core(core_extension *core, PMV_XOR_Request xor_req)
{
	MV_U8  i, result;
	MV_U32  load;
	pl_xor  *xor_mod = &core->xors;

	/*
	 * For DMA request we need to maintain FIFO order for performance
	 * reasons. All DMA requests will go to the same core to maintain
	 * this order. XOR and compare requests will then be issued
	 * according to the load of the XOR cores
	 */
	if (xor_req->Request_Type == XOR_REQUEST_DMA)
		return MV_DMA_CORE_ID;

	/*
	 * Currently there is a maximum of 4 XOR cores for the Frey ASIC, so
	 * iterating through the cores is acceptable. However, if and when we
	 * move to a new architecture with more XOR cores, then we may need to
	 * change the implementation to a heap / AVL tree.
	 */
	load = xor_mod->engine[0].xor_core_data_count;
	result = 0;
	for (i = 1; i < MV_MAX_XOR_ID; i++) {
		if (load > xor_mod->engine[i].xor_core_data_count) {
			load = xor_mod->engine[i].xor_core_data_count;
			result = i;
		}
	}

	return result;
}
#endif /* XOR_LOAD_BALANCE */

#ifdef CORE_USE_NEW_XOR
void xor_get_width_depth (PMV_XOR_Request xor_req, MV_PU8 w, MV_PU8 d)
{
    MV_U8 width_count, depth_count, width_per_table, src_count;

    width_count =
        (
            xor_req->Target_SG_Table_Count +
            CORE_MAX_XOR_TARGET_COUNT -
            1
        ) /
        CORE_MAX_XOR_TARGET_COUNT;

    width_per_table = MV_MIN(xor_req->Target_SG_Table_Count, CORE_MAX_XOR_TARGET_COUNT);
    src_count = xor_req->Source_SG_Table_Count;
    depth_count = 1;
    if (src_count > (CORE_MAX_XOR_CMD_ENTRY - width_per_table)) {
        src_count -= (CORE_MAX_XOR_CMD_ENTRY - width_per_table);
        depth_count += 1 + (src_count - 1) / (CORE_MAX_XOR_CMD_ENTRY - (width_per_table * 2));
    }

    *w = width_count;
    *d = depth_count;
    return;
}
#endif /* CORE_USE_NEW_XOR */

void
xor_free_xor_context(xor_context **pool, xor_context **context)
{
	xor_context *tmp;

	while (*context != NULL) {
		tmp = *context;
		*context = (*context)->next;
		tmp->next = *pool;
		(*pool) = tmp;
	}
}

#ifdef CORE_USE_NEW_XOR
pxor_context
xor_get_xor_context(pl_xor *xor_core, PMV_XOR_Request req)
{
	xor_context *context, *result;
	MV_U8   i, xor_cmd_count;

	if ((req->Request_Type == XOR_REQUEST_COMPARE)
                || (req->Request_Type == XOR_REQUEST_DMA)) {
        	/* req->Target_SG_Table_Count == 0 means it's a compare request */
	        /* req->Source_SG_Table_Count == 1 means it's a DMA request */
                MV_DASSERT((req->Target_SG_Table_Count == 0) \
                        || (req->Source_SG_Table_Count == 1));
		xor_cmd_count = 1;
		}
#ifdef SUPPORT_FILL_MEMORY
		else if (req->Request_Type == XOR_REQUEST_MEMSET){
			xor_cmd_count = 1;
        }
#endif
		else {
		MV_U8 wid, dep;
		xor_get_width_depth(req, &wid, &dep);
		xor_cmd_count = wid * dep;
	}

	result = NULL;
	for (i = 0; i < xor_cmd_count; i++) {
		if (xor_core->xor_context_list == NULL) {
			xor_free_xor_context(&xor_core->xor_context_list,
				&result);
			CORE_DPRINT(("XOR Core out of slots. Request "\
				"asking for %d slots.\n", xor_cmd_count));
			return NULL;
		}
		context = xor_core->xor_context_list;
		xor_core->xor_context_list = context->next;
		context->next = result;
		result = context;
		context->finished = MV_FALSE;
                MV_DASSERT(context->sg_wrapper == NULL);
		context->sg_wrapper = NULL;
	}

	return result;
}

#else /* CORE_USE_NEW_XOR */

pxor_context
xor_get_xor_context(pl_xor *xor_core, PMV_XOR_Request req)
{
	xor_context *context, *result;
	MV_U8   i, xor_cmd_count;

	/* req->Target_SG_Table_Count == 0 means it's a compare request */
	if (req->Target_SG_Table_Count > 0)
		xor_cmd_count = ((req->Source_SG_Table_Count +
			req->Target_SG_Table_Count + CORE_MAX_XOR_CMD_ENTRY - 1)
			/ CORE_MAX_XOR_CMD_ENTRY)
			* ((req->Target_SG_Table_Count + CORE_MAX_XOR_TARGET_COUNT - 1)
			/ CORE_MAX_XOR_TARGET_COUNT);
	else
		xor_cmd_count = 1;

	result = NULL;
	for (i = 0; i < xor_cmd_count; i++) {
		if (xor_core->xor_context_list == NULL) {
			xor_free_xor_context(&xor_core->xor_context_list,
				&result);
			CORE_DPRINT(("XOR Core out of slots. Request "\
				"asking for %d slots.\n", xor_cmd_count));
			return NULL;
		}
		context = xor_core->xor_context_list;
		xor_core->xor_context_list = context->next;
		context->next = result;
		result = context;
		context->finished = MV_FALSE;
                MV_DASSERT(context->sg_wrapper == NULL);
		context->sg_wrapper = NULL;
	}

	return result;
}

#endif /* CORE_USE_NEW_XOR */

/*
 xor_free_tbl_wrapper

 Releases an XOR Command Table.
*/
//JING TBD
void xor_free_tbl_wrapper(hw_buf_wrapper **pool, hw_buf_wrapper **wrapper)
{
	MV_DASSERT(*wrapper != NULL);
	(*wrapper)->next = *pool;
	*pool = *wrapper;
	*wrapper = NULL;
}

/*
 xor_get_tbl_wrapper

 Returns an XOR Command Table.
*/
hw_buf_wrapper *xor_get_tbl_wrapper(hw_buf_wrapper **pool)
{
	hw_buf_wrapper *result;
	if (*pool == NULL) return NULL;

	result = *pool;
	*pool = result->next;
	result->next = NULL;

	return result;
}

/*
 xor_sg_table_release

 Releases all of the resources related to an XOR request except for the XOR
 context.
*/
void xor_sg_table_release(
	MV_PVOID core_ext,
	MV_XOR_Request *xor_req)
{
	core_extension *core = (core_extension *)core_ext;
	xor_context     *context = xor_req->Context[MODULE_CORE];
	hw_buf_wrapper  *wrapper;

	context = (xor_context *)xor_req->Context[MODULE_CORE];
	MV_DASSERT(context->context_type == XOR_CONTEXT_TYPE_TABLE);

	while (context) {
		while (context->sg_wrapper) {
			wrapper = context->sg_wrapper;
			context->sg_wrapper = wrapper->next;
			free_sg_buf(&core->lib_rsrc, wrapper);
		}
		context->sg_wrapper = NULL;
		context = context->next;
	}
}

/*
 xor_get_coef

 Returns the XOR Coefficient.
*/
XOR_COEF xor_get_coef(
	PMV_XOR_Request xor_req,
	MV_U32 source,
	MV_U32 target)
{
	//PXOR_COEF coef_ptr = &xor_req->Coef[0][0];
	//return (coef_ptr[source + target * xor_req->Target_SG_Table_Count]);
	return xor_req->Coef[target][source];
}

MV_U8 xor_is_request_finished(PMV_XOR_Request xor_req)
{
	xor_context *context = (xor_context *)xor_req->Context[MODULE_CORE];

	while (context) {
		if (context->finished != MV_TRUE)
			return MV_FALSE;
		context = context->next;
	}

	return MV_TRUE;
}

void xor_set_context_finished(PMV_XOR_Request xor_req,
	MV_U16 slot_num)
{
	xor_context *context = (xor_context *)xor_req->Context[MODULE_CORE];

	while (context) {
		if (context->slot_num == slot_num) {
			context->finished = MV_TRUE;
			return;
		}
		context = context->next;
	}

	MV_DASSERT(MV_FALSE);
}


/*******************************************************************************
*                                                                              *
* XOR Initializaion                                                            *
*                                                                              *
*******************************************************************************/

/*
 xor_get_dma_memory_quota

 Returns the amount of cached memory required by the XOR engine.
*/
MV_U32 xor_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32  size;
	MV_U16  slot_count;
	MV_U16  delv_q_size;
	MV_U16  cmpl_q_size;

	size = 0;
	xor_get_supported_pl_counts(max_io,
		&slot_count,
		&delv_q_size,
		&cmpl_q_size);

	size += sizeof(PMV_XOR_Request) * slot_count * MV_MAX_XOR_ID;

	/* Memory for XOR request context */
	size += sizeof(xor_context) * slot_count;

	return size;
}

/*
 xor_get_dma_memory_quota

 Returns the amount of uncached memory required by the XOR engine.
*/
MV_U32 xor_get_dma_memory_quota(MV_U16 max_io)
{
	MV_U32  size;
	MV_U16  slot_count;
	MV_U16  delv_q_size;
	MV_U16  cmpl_q_size;

	size = 0;
	xor_get_supported_pl_counts(max_io,
		&slot_count,
		&delv_q_size,
		&cmpl_q_size);

	/* XOR Delivery Queue */
	size += sizeof(xor_delv_q_entry) * delv_q_size;
	size += 64; /* rounding */
#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
    size += sizeof(xor_delv_q_entry);
#endif
	/* XOR Completion Queue */
	size += sizeof(xor_cmpl_q_entry) * cmpl_q_size;
	size += 64; /* rounding */
#ifdef ATHENA_XOR_CMPL_Q_SHADOW_ENABLE
    size += sizeof(xor_cmpl_q_entry);
#endif
	size *= MV_MAX_XOR_ID;

	/* The structures below are shared between different XOR engines */
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	/* XOR Command List */
	size += sizeof(xor_cmd_header) * slot_count;
	size += 64; /* rounding */
#endif
	/* XOR Table */
	size += sizeof(xor_cmd_table) * slot_count;
	size += 64; /* rounding */

	return size;
}

MV_U8 xor_init_memory(
	MV_PVOID extension,
	lib_resource_mgr *rsrc,
	MV_U16 max_io)
{
	core_extension    *core = (core_extension *)extension;
	pl_xor      *xor_mod = &core->xors;
	xor_engine      *xor_core;
	MV_PVOID		vir;
	MV_PHYSICAL_ADDR	dma;
	MV_U32      item_size;

	xor_context       *context;
	MV_U32      i;
	MV_U16      slot_count, delv_q_size, cmpl_q_size;
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	MV_PVOID		xor_hdr_cached_ptr;
	MV_PHYSICAL_ADDR	xor_hdr_dma_ptr;
#endif
	MV_PVOID		xor_tbl_cached_ptr;
	MV_PHYSICAL_ADDR	xor_tbl_dma_ptr;

	xor_get_supported_pl_counts(max_io,
		&slot_count,
		&delv_q_size,
		&cmpl_q_size);
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	/* XOR Command Header */
	item_size = sizeof(xor_cmd_header) * slot_count;
	xor_hdr_cached_ptr = lib_rsrc_malloc_dma(rsrc, item_size, 64,
				&xor_hdr_dma_ptr);
	if (xor_hdr_cached_ptr == NULL) return MV_FALSE;
#endif

	/* XOR Table */
	item_size = sizeof(xor_cmd_table) * slot_count;
	xor_tbl_cached_ptr = lib_rsrc_malloc_dma(rsrc, item_size, 64,
				&xor_tbl_dma_ptr);
	if (xor_tbl_cached_ptr == NULL) return MV_FALSE;

#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	dma = xor_tbl_dma_ptr;
	for (i = 0; i < slot_count; i++) {
		xor_fill_cmd_header(
			&(((xor_cmd_header *)xor_hdr_cached_ptr)[i]),
			dma);
		dma = U64_ADD_U32(dma, sizeof(xor_cmd_table));
	}
#endif
	/* XOR Request Context */
	xor_mod->xor_context_list = NULL;
	for (i = 0; i < slot_count ; i++) {
		item_size = sizeof(xor_context);
		vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
		if (vir == NULL) return MV_FALSE;
		context = (xor_context *)vir;
		context->next = xor_mod->xor_context_list;
		context->context_type = XOR_CONTEXT_TYPE_TABLE;

		context->slot_num = (MV_U16)i;    /* static slot number */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		context->xor_tbl =
			&((xor_cmd_table *)xor_tbl_cached_ptr)[i];
		context->xor_tbl_dma =
			U64_ADD_U32(xor_tbl_dma_ptr, i*sizeof(xor_cmd_table));
#else
		context->xor_cmd_hdr =
			&((xor_cmd_header *)xor_hdr_cached_ptr)[i];
		context->xor_tbl =
			&((xor_cmd_table *)xor_tbl_cached_ptr)[i];
#endif
		context->sg_wrapper = NULL;
		xor_mod->xor_context_list = context;
	}

	for (i = 0; i < MV_MAX_XOR_ID; i++) {

		xor_core = &xor_mod->engine[i];
		xor_core->xor_id = (MV_U8)i;
		xor_core->xor_mmio_base =
			(MV_LPVOID)((MV_PU8)core->mmio_base
				+ (MV_XOR_CORE_BASE_ADDR
				+ (i * MV_XOR_CORE_ADDR_INCR)));

		xor_core->slot_count_support = slot_count;
		xor_core->xor_delv_q_size = delv_q_size;
		xor_core->xor_cmpl_q_size = cmpl_q_size;
		xor_core->xor_last_delv_q = XOR_MAX_DELV_QUEUE_SIZE;
		xor_core->xor_last_cmpl_q = XOR_MAX_CMPL_QUEUE_SIZE;
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
		xor_core->xor_cmd_hdr = xor_hdr_cached_ptr;
		xor_core->xor_cmd_hdr_dma = xor_hdr_dma_ptr;
#endif
		xor_core->xor_tbl = xor_tbl_cached_ptr;
		xor_core->xor_tbl_dma = xor_tbl_dma_ptr;

		MV_LIST_HEAD_INIT(&xor_core->xor_waiting_list);
		MV_LIST_HEAD_INIT(&xor_core->xor_cmpl_list);

		/* XOR Running Request */
		item_size = sizeof(PMV_XOR_Request) * slot_count;
		vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
		if (vir == NULL) return MV_FALSE;
		xor_core->xor_running_req = (PMV_XOR_Request *)vir;

		/* XOR Delivery Queue */
		item_size = sizeof(xor_delv_q_entry) * delv_q_size;
		vir = lib_rsrc_malloc_dma(rsrc, item_size, 64, &dma);
		if (vir == NULL) return MV_FALSE;
		xor_core->xor_delv_q = vir;
		xor_core->xor_delv_q_dma = dma;
#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
		vir = lib_rsrc_malloc_dma(rsrc, sizeof(xor_delv_q_entry), 4, &dma);
		xor_core->xor_shadow_delv_q = vir;
		xor_core->xor_shadow_delv_q_dma = dma;
#endif
		/* XOR Completion Queue */
		item_size = sizeof(xor_cmpl_q_entry) * cmpl_q_size;
		vir = lib_rsrc_malloc_dma(rsrc, item_size, 64, &dma);
		if (vir == NULL) return MV_FALSE;
		xor_core->xor_cmpl_q = vir ;
		xor_core->xor_cmpl_q_dma = dma;
#ifdef ATHENA_XOR_CMPL_Q_SHADOW_ENABLE
		vir = lib_rsrc_malloc_dma(rsrc, sizeof(xor_cmpl_q_entry), 4, &dma);
		xor_core->xor_shadow_cmpl_q = vir;
		xor_core->xor_shadow_cmpl_q_dma = dma;
#endif
		xor_core->irq_rounting = XOR_PCIE_FNCTN_SEL_0_FUN_0;
		xor_core->irq_rounting |= ((MAX_MULTI_QUEUE*core->chip_info->n_host+i)<<XOR_MSI_X_VCTR_SEL_0_SHIFT) & XOR_MSI_X_VCTR_SEL_0_MASK;
	}
	return MV_TRUE;
}

/*
 xor_init

 Initializes the XOR engine with the required flags and memory addresses to
 access. This should be done after xor_init_cached_memory and
 xor_init_dma_memory.
*/
void xor_init(MV_PVOID extension)
{
	core_extension  *core = (core_extension *) extension;
	MV_U16    i = 0;

	MV_LIST_HEAD_INIT( &core->xor_run_fifo_header );
	xor_reset_all_cores(core);

	for (i = 0; i < MV_MAX_XOR_ID; i++)
		xor_init_core(core, i);
}

/*
 xor_complete_xor_request

 Releases the associated resources for a request and calls its upper layer
 callback function.
*/
void xor_complete_xor_request(
	core_extension *core,
	xor_engine *xor_core,
	PMV_XOR_Request xor_req
	)
{
	PMV_XOR_Request p_pend_xor_req = NULL;
	xor_sg_table_release(core, xor_req);

	xor_free_xor_context(&core->xors.xor_context_list,
		(xor_context **)&xor_req->Context[MODULE_CORE]);

	xor_req->Is_Active = 0;

	while(!List_Empty(&core->xor_run_fifo_header )) {
		p_pend_xor_req = List_GetFirstEntry(
			&core->xor_run_fifo_header,
			MV_XOR_Request,
			QP_Cmpl_Seq);
		if (p_pend_xor_req->Is_Active) {
			break;
		}
		p_pend_xor_req->Completion(p_pend_xor_req->Cmd_Initiator, p_pend_xor_req);
		p_pend_xor_req = NULL;
	}

	if ( p_pend_xor_req )
		List_Add(
		    &p_pend_xor_req->QP_Cmpl_Seq,
		    &core->xor_run_fifo_header
		);	
}

void xor_handle_waiting_list_core(MV_PVOID This, MV_U16 core_id)
{
	core_extension  *core = (core_extension *)This;
	pl_xor    *xor_mod = &core->xors;
	xor_engine      *xor_core = &xor_mod->engine[core_id];
	PMV_XOR_Request xor_req;
	PMV_XOR_Request last_xor_req = NULL;

	while(!List_Empty(&xor_core->xor_waiting_list)) {
		xor_req = (PMV_XOR_Request)List_GetFirstEntry(
				&xor_core->xor_waiting_list,
				MV_XOR_Request,
				Queue_Pointer);

		if (xor_req == NULL) {
			MV_DASSERT(MV_FALSE);
			return;
		}

		if (xor_req == last_xor_req) { //No resource
		    List_Add(&xor_req->Queue_Pointer,
				&xor_core->xor_waiting_list);

		    return;
		}
		last_xor_req = xor_req;

		/* ikkwong: TBD */
		/* Remove this when application layer uses src_sg_tbl_ptr and
		   tgt_sg_tbl_ptr */
		xor_req->src_sg_tbl_ptr = xor_req->Source_SG_Table_List;
		xor_req->tgt_sg_tbl_ptr = xor_req->Target_SG_Table_List;

		/* Allocate Core Driver Context to the XOR request. */
		if (xor_req->Context[MODULE_CORE] == NULL) {
			xor_req->Context[MODULE_CORE] =
				xor_get_xor_context(xor_mod, xor_req);
			if (xor_req->Context[MODULE_CORE] == NULL) {
				List_Add(&xor_req->Queue_Pointer,
					&xor_core->xor_waiting_list);
				return;
			}
		}

		switch(xor_req->Request_Type) {
		case XOR_REQUEST_WRITE:
			xor_write_command(core, xor_core, xor_req);
			break;
		case XOR_REQUEST_COMPARE:
			xor_compare_command(core, xor_core, xor_req);
			break;
		case XOR_REQUEST_DMA:
			xor_dma_command(core, xor_core, xor_req);
			break;
#ifdef SUPPORT_FILL_MEMORY
		case XOR_REQUEST_MEMSET:
			xor_memset_command(core, xor_core, xor_req);
			break;
#endif
		default:
                        MV_ASSERT(MV_FALSE);
                        break;
		}
	}
}

void xor_handle_cmpl_list_core(MV_PVOID This, MV_U16 core_id)
{
	core_extension  *core = (core_extension *)This;
	xor_engine      *xor_core;
	PMV_XOR_Request xor_req;

	xor_core = &core->xors.engine[core_id];
	while (!List_Empty(&xor_core->xor_cmpl_list)) {
		xor_req = (PMV_XOR_Request)List_GetFirstEntry(
				&xor_core->xor_cmpl_list,
				MV_XOR_Request,
				Queue_Pointer);

		MV_DASSERT(xor_req != NULL);

		xor_complete_xor_request(core, xor_core, xor_req);
		xor_core->xor_core_req_count--;
#ifdef XOR_LOAD_BALANCE
		xor_load_balance_decrement(xor_req, xor_core);
#endif
	}
}


/*
 Core_ModuleSendXORRequest

 External API for sending a request to the XOR engine. The requests are first
 enqueued to the different XOR Core structures and then are handled core by
 core.
*/
void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request xor_req)
{
	core_extension  *core = (core_extension *)This;
	MV_U8 core_num = 0;

#ifdef XOR_LOAD_BALANCE
	core_num = xor_load_balance_get_core(core, xor_req);
#else
	core_num = xor_get_xor_core_id(core, xor_req);
#endif

	xor_req->Is_Active = 1;
	List_AddTail( &xor_req->QP_Cmpl_Seq, &core->xor_run_fifo_header );

	List_AddTail(&xor_req->Queue_Pointer,
		&core->xors.engine[core_num].xor_waiting_list);
	/* have to push the queue. otherwise RAID 5 bootup may hang */
	xor_handle_waiting_list(core);
}

/*
 xor_handle_int

 XOR Engine interrupt handler wrapper. Checks for any completed XOR requests
 for the specified XOR Core and enqueues them to the completed list.
*/
void xor_handle_int(MV_PVOID This)
{
//	core_extension *core = (core_extension *) This;
	MV_U16 i;
	MV_BOOLEAN att=MV_FALSE;
	for (i = 0; i < MV_MAX_XOR_ID; i++){
		att=xor_handle_int_core(This, i);
		if(att == MV_FALSE){
			xor_handle_att_int(This, i);
		}
	}
}


/*
 xor_handle_waiting_list

 Sorts the XOR requests in the waiting queue into XOR requests, DMA requests,
 and compare requests. The respective handling functions will acquire required
 resources and dispatches the request to the XOR Core they are assigned to.
*/
void xor_handle_waiting_list(MV_PVOID This)
{
	core_extension  *core = (core_extension *)This;
	MV_U16 i;

	for (i = 0; i < MV_MAX_XOR_ID; i++)
		xor_handle_waiting_list_core(core, i);
}


/*
 xor_handle_cmpl_list

 Completes XOR requests in the the completed queue by calling releasing their
 associated resources and calling their upper layer callback function.
*/
void xor_handle_cmpl_list(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	MV_U16 i;

	for (i = 0; i < MV_MAX_XOR_ID; i++)
		xor_handle_cmpl_list_core(core, i);
}
#endif /* HARDWARE_XOR */
