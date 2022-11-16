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

#ifndef CORE_XOR_H
#define CORE_XOR_H

#include "mv_config.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_resource.h"
#include "core_protocol.h"
#include "pl_xor.h"
#if defined(SOFTWARE_XOR) || defined(HARDWARE_XOR)
void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq);
#endif
#ifdef HARDWARE_XOR
typedef struct _xor_context {
	struct _xor_context     *next;
	MV_U8                   context_type;
	MV_U8                   finished;
	MV_U16                  slot_num;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_PHYSICAL_ADDR        xor_tbl_dma;
#else
	xor_cmd_header          *xor_cmd_hdr;
#endif
	xor_cmd_table           *xor_tbl;
	hw_buf_wrapper          *sg_wrapper;
} xor_context, *pxor_context;

typedef struct _xor_engine {
	MV_LPVOID       xor_mmio_base;
	MV_U8		xor_id;
	MV_U8		reserved01[3];
	MV_U32		xor_int_status;
	MV_U32		xor_int_mask;
	MV_U16          slot_count_support;
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	/* XOR Command Header */
	MV_PVOID        xor_cmd_hdr;
	MV_PHYSICAL_ADDR xor_cmd_hdr_dma;
#endif
	/* XOR Table */
	MV_PVOID        xor_tbl;
	MV_PHYSICAL_ADDR xor_tbl_dma;

	/* Delivery Queue */
	MV_PVOID        xor_delv_q;
	MV_PHYSICAL_ADDR xor_delv_q_dma;
#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
	MV_PVOID        xor_shadow_delv_q;
	MV_PHYSICAL_ADDR xor_shadow_delv_q_dma;
#endif
	/* Completion Queue */
#ifdef ATHENA_XOR_CMPL_Q_SHADOW_ENABLE
	MV_PVOID        xor_shadow_cmpl_q;
	MV_PHYSICAL_ADDR xor_shadow_cmpl_q_dma;
#endif
	MV_PVOID        xor_cmpl_q;
	MV_PHYSICAL_ADDR xor_cmpl_q_dma;

	MV_U16          xor_delv_q_size;
	MV_U16          xor_cmpl_q_size;

	MV_U16          xor_last_delv_q;
	MV_U16          xor_last_cmpl_q;

	PMV_XOR_Request *xor_running_req;
	List_Head       xor_waiting_list;
	List_Head       xor_cmpl_list;
	MV_U16          xor_core_req_count;
#ifdef XOR_LOAD_BALANCE
	MV_U32          xor_core_data_count;
#endif
	MV_U32          irq_rounting;
} xor_engine;

typedef struct _pl_xor {
	/* a list of XOR_Table_Wrapper */
	xor_context     *xor_context_list;
	xor_engine      engine[MV_MAX_XOR_ID];
} pl_xor;

/*******************************************************************************
*                                                                              *
* XOR Function Prototypes                                                      *
*                                                                              *
*******************************************************************************/

MV_U32 xor_get_cached_memory_quota(MV_U16 max_io);
MV_U32 xor_get_dma_memory_quota(MV_U16 max_io);
MV_U8 xor_init_memory(MV_PVOID extension, lib_resource_mgr *lib_rsrc,
        MV_U16 max_io);
void xor_init(MV_PVOID extension);

#ifdef XOR_LOAD_BALANCE
void xor_load_balance_increment(PMV_XOR_Request xor_req, xor_engine *xor_core);
void xor_load_balance_decrement(PMV_XOR_Request xor_req, xor_engine *xor_core);
#endif

void xor_dump_register(MV_PVOID This);
XOR_COEF xor_get_coef(PMV_XOR_Request xor_req, MV_U32 source, MV_U32 target);
#ifdef CORE_USE_NEW_XOR
void xor_get_width_depth(PMV_XOR_Request xor_req, MV_PU8 w, MV_PU8 d);
#endif
void xor_disable_ints(MV_PVOID This, MV_U16 xor_id);
void xor_enable_ints(MV_PVOID This, MV_U16 xor_id);
void xor_clear_int(MV_PVOID This);
void xor_handle_int(MV_PVOID This);
void xor_handle_waiting_list(MV_PVOID This);
void xor_handle_cmpl_list(MV_PVOID This);
MV_BOOLEAN xor_handle_int_core(MV_PVOID This, MV_U16 core_id);
MV_VOID xor_handle_att_int(MV_PVOID This, MV_U16 core_id);
#endif /* HARDWARE_XOR */

#endif
