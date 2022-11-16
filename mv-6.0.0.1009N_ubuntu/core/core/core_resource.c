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
#include "core_resource.h"
#include "core_manager.h"

#include "com_u64.h"
#include "com_dbg.h"

#include "core_sas.h"
#include "core_error.h"
#include "core_expander.h"
#include "core_console.h"

#include "core_util.h"
#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
#include "core_i2c.h"
#include "hba_exp.h"
#endif

void core_get_supported_dev (
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 hd_count, /* HDD count */
	OUT MV_PU8 exp_count, /* expander count */
	OUT MV_PU8 pm_count, /* pm count */
	OUT MV_PU8 enc_count /* enclosure count */
	)
{
#ifdef SUPPORT_ROC
	*hd_count = (48 + 1);//CORE_MAX_DEVICE_SUPPORTED;
#else
	*hd_count = CORE_MAX_DEVICE_SUPPORTED;
#endif
	*pm_count = CORE_MAX_PM_SUPPORTED;
	if (max_io == 1) {
		*exp_count = CORE_MIN_EXPANDER_SUPPORTED;
		*enc_count = CORE_MIN_ENC_SUPPORTED;
	} else {
		*exp_count = CORE_MAX_EXPANDER_SUPPORTED;
		*enc_count = CORE_MAX_ENC_SUPPORTED;
	}
}

void core_get_supported_pl_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 slot_count, /* hardware I/O slot count */
	OUT MV_PU16 delv_q_size, /* deliver queue size */
	OUT MV_PU16 cmpl_q_size, /* completion queue size */
	OUT MV_PU16 received_fis_count /* max received FISes we can save. Depends on register set */
	)
{
	if (max_io == 1) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		*slot_count = 1;
//		*delv_q_size = MAX_MULTI_QUEUE;
		*delv_q_size =MV_MAX((*slot_count + 1), 4); //add 1 delivery queue for queue move
#else
		*slot_count = 1;
		/*
		 * Deliver must be at least slotCount.
		 * No pointer entry, no need add one for the pointer comparing with completion queue.
		 * Add extra 1 entry for read/write pointer compare. Otherwise when slotCount==1, hardware don't know pointer is moved.
		 */
		*delv_q_size = *slot_count + 1;
#endif
		*received_fis_count = 1;
	} else {
		*slot_count = CORE_MAX_REQUEST_NUMBER;
		/* Deliver queue size can be at most 512 - hardware limitation */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		*delv_q_size = (*slot_count);
#else
		*delv_q_size = *slot_count;
#endif
		*received_fis_count = MAX_REGISTER_SET_PER_IO_CHIP;
	}

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	/* athena hasn't write pointer. */
	*cmpl_q_size = MV_MAX((*slot_count + 1), 4);
#else
	/*
	 * Completion must be at least slotCount.
	 * Add 1 because the first entry of completion queue is the pointer,
	 * refer to chip spec.
	 * Add extra 10 for hardware event like hotplug
	 */
	*cmpl_q_size = *slot_count + 1 + 10;
#endif
}

void xor_get_supported_pl_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 slot_count, /* hardware I/O slot count */
	OUT MV_PU16 delv_q_size, /* deliver queue size */
	OUT MV_PU16 cmpl_q_size /* completion queue size */
	)
{
	if (max_io == 1) {
		*slot_count = 1;
		/*
		 * Deliver must be at least slotCount.
		 * No pointer entry, no need add one for the pointer comparing with completion queue.
		 * Add extra 1 entry for read/write pointer compare. Otherwise when slotCount==1, hardware don't know pointer is moved.
		 */
		*delv_q_size = *slot_count + 1;
	} else {
		*slot_count = XOR_MAX_SLOT_NUMBER;
		/* Deliver queue size can be at most 512 - hardware limitation */
		*delv_q_size = *slot_count;
	}

	/*
	 * Completion must be at least slotCount.
	 * Add 1 because the first entry of completion queue is the pointer,
	 * refer to chip spec.
	 */
	*cmpl_q_size = *slot_count + 1;
}


/*
 * Although SMP scratch buffer is used as pure cached memory,
 * I merge it with scratch buffer because we don't want so many different type of buffers.
 */
void core_get_supported_pal_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 intl_req_count, /* internal request count, used for */
		/* a. initialization(InternalReqCount) b. sub req for large request support(SubReqCount) */
	OUT MV_PU16 req_sg_entry_count, /* cached memory sg entry count for internal requests */
	OUT MV_PU16 hw_sg_entry_count, /* dma sg scratch buffer entry count */
	OUT MV_PU16 hw_sg_buf_count, /* dma sg scratch buffer count */
	OUT MV_PU16 scratch_buf_count, /* dma internal req scratch buffer, used for */
		/* a.identify(SATAScratchCount), b. SES (SESControlCount), c. SMP(SMPScratchCount) */
	OUT MV_PU16 context_count, /* core request context for both internal/external req */
	OUT MV_PU16 event_count /* hardware event count */
   )
{
	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);

	if (max_io == 1) {
		*intl_req_count = MV_MAX(2*CORE_MIN_INTERNAL_REQ_COUNT, 4); //need 4 request at least.
		*req_sg_entry_count = CORE_MIN_REQ_SG_ENTRY_COUNT;
		*hw_sg_entry_count = CORE_MIN_HW_SG_ENTRY_COUNT;
		*hw_sg_buf_count = CORE_MIN_HW_SG_BUFFER_COUNT;
		*scratch_buf_count = CORE_MIN_SCRATCH_BUFFER_COUNT;
		*event_count = CORE_MIN_HW_EVENT_COUNT;
	} else {
		/* internal req count = init req count + sub req for large request support */
		*intl_req_count = CORE_MAX_INTERNAL_REQ_COUNT;
		*req_sg_entry_count = CORE_MAX_REQ_SG_ENTRY_COUNT;

		*hw_sg_entry_count = CORE_MAX_HW_SG_ENTRY_COUNT;
		//JING TBD: consider core_xor.c
		*hw_sg_buf_count = MV_MAX(CORE_MAX_HW_SG_BUFFER_COUNT, slot_count*MAX_NUMBER_IO_CHIP); //CORE_MAX_HW_SG_BUFFER_COUNT;
		*scratch_buf_count = CORE_MAX_SCRATCH_BUFFER_COUNT; /* SATA + SES + SMP */
		*event_count = CORE_MAX_HW_EVENT_COUNT;
	}

	*context_count = *intl_req_count + slot_count * MAX_NUMBER_IO_CHIP;
}


#define INTERNAL_REQ_TOTAL_SIZE(sg_entry_n) \
	(sizeof(MV_Request) + sizeof(MV_SG_Entry) * sg_entry_n + sizeof(struct _sense_data))

MV_U32 core_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;
	MV_U8 exp_count, pm_count, enc_count;
	MV_U16 hd_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);
	core_get_supported_dev(max_io, &hd_count, &exp_count, &pm_count, &enc_count);
	size += ROUNDING(sizeof(core_extension), 8);
	/*
	 * The following are the pure cached memory allocation.
	 */
	/* memory for running_req to store the running requests on slot */
	size += ROUNDING(sizeof(PMV_Request) * slot_count * MAX_NUMBER_IO_CHIP, 8);

	/* memory for internal reqs */
	size += ROUNDING(intl_req_count * INTERNAL_REQ_TOTAL_SIZE(req_sg_entry_count), 8);
	/* request context for both external and internal requests */
	size += ROUNDING(context_count * sizeof(core_context), 8);
	/* tag pools */
	size += ROUNDING(slot_count * MAX_NUMBER_IO_CHIP * sizeof(MV_U16), 8);
	//JING TESTING: Need a better to make it pointer size aligned.
	size += ROUNDING(MAX_NUMBER_IO_CHIP * sizeof(MV_U16), 8);

	/*
	 * The following are the wrapper for dma memory allocation.
	 */
	size += ROUNDING(sizeof(hw_buf_wrapper) * scratch_buf_count, 8);
	size += ROUNDING(sizeof(hw_buf_wrapper) * hw_sg_buf_count, 8);
	/* it's used to store command table address. command table is not contiguent. */
	size += ROUNDING(sizeof(hw_buf_wrapper) * slot_count * MAX_NUMBER_IO_CHIP, 8);
	size += ROUNDING(sizeof(event_record) * event_count, 8);
	size += ROUNDING(sizeof(saved_fis) * received_fis_count * MAX_NUMBER_IO_CHIP, 8);

	/* get device data structure memory */
	size += ROUNDING(sizeof(domain_device) * hd_count, 8);
	size += ROUNDING(sizeof(domain_expander) * exp_count, 8);
	size += ROUNDING(sizeof(domain_pm) * pm_count, 8);
	size += ROUNDING(sizeof(domain_enclosure) * enc_count, 8);
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	size += ROUNDING(context_count * sizeof(cmd_resource), 8);
#endif
	return size;
}

MV_U32 core_get_dma_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	MV_U16 scratch_buf_count, intl_req_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;
	MV_U16 slot_count, hw_sg_buf_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 context_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	/*
	 * For dma memory, need extra bytes for ASIC alignment requirement.
	 * delivery queue/completion queue base address must be 64 byte aligned.
	 */
	/* I didn't find the other aligned requirement from the spec.
	 * And somehow the old code is 8 byte aligned for delivery/completion queue. */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	size = 64 +ROUNDING(sizeof(mv_command_struct), 64) * slot_count * MAX_NUMBER_IO_CHIP;	/* command list*/
#else
	size = 64 + sizeof(mv_command_header) * slot_count * MAX_NUMBER_IO_CHIP;	/* command list*/

	size += 128 + sizeof(mv_command_table) * slot_count * MAX_NUMBER_IO_CHIP;	/* Command Table */
#endif
	/* received FIS */
	if (max_io > 1) {
		size += (256 + MAX_RX_FIS_POOL_SIZE) * MAX_NUMBER_IO_CHIP;
	} else {
		size += 256 + MIN_RX_FIS_POOL_SIZE;
	}
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)       // Deliver Queue Entry is 4 DW
	if (max_io > 1) {
		size += 64 + sizeof(delv_q_context) * delv_q_size * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* delivery shadow. */
		size += sizeof(delv_q_context) * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* complettion queue */
		size += 64 + sizeof(MV_U32) * cmpl_q_size * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* completion shadow. */
		size += sizeof(MV_U32) * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
	}
	else{
		size += 64 + sizeof(delv_q_context) * delv_q_size * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* delivery shadow. */
		size += sizeof(delv_q_context) * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* complettion queue */
		size += 64 + sizeof(MV_U32) * cmpl_q_size * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
		/* completion shadow. */
		size += sizeof(MV_U32) * MAX_MULTI_QUEUE * MAX_NUMBER_IO_CHIP;
	}
#else
	size += (64 + sizeof(MV_U32) * delv_q_size) * MAX_NUMBER_IO_CHIP;
	size += (64 + sizeof(MV_U32) * cmpl_q_size) * MAX_NUMBER_IO_CHIP;
#endif

	/* scratch buffer for initialization/SES/SMP */
	size += 8 + SCRATCH_BUFFER_SIZE * scratch_buf_count;

	/* buffer for dma SG tables */
	size += 8 + CORE_HW_SG_ENTRY_SIZE * hw_sg_entry_count * hw_sg_buf_count;

#ifdef HOTPLUG_BYTE_COUNT_ERROR_WORKAROUND
	if( max_io != 1 ) {
		size += 8 + TRASH_BUCKET_SIZE;
	}
#endif
#if defined(HAVE_HW_COMPLIANT_SG)
	size += intl_req_count * (req_sg_entry_count * sizeof(MV_SG_Entry) + 64);
#endif

#ifdef SUPPORT_SECURITY_KEY_RECORDS
        size += 8 + sizeof(mv_security_key_record)*slot_count;
#endif
	return size;
}

MV_BOOLEAN lib_rsrc_allocate_device(lib_resource_mgr *rsrc, MV_U16 max_io)
{
	MV_U16 hd_count;
	MV_U8 exp_count, pm_count, enc_count;
	MV_U32 item_size;
	int i;
#ifdef ATHENA_PERFORMANCE_TUNNING
	domain_device *dev;
	domain_expander *exp;
	domain_pm *pm;
	domain_enclosure *enc;
#endif
	core_get_supported_dev(max_io, &hd_count, &exp_count, &pm_count, &enc_count);

	/* allocate memory for hd */
	if (hd_count > 0) {
		item_size = sizeof(domain_device) * hd_count;
		rsrc->hds = (domain_device *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->hds == NULL) return MV_FALSE;
		for (i = 0; i < hd_count-1; i++){
#ifdef ATHENA_PERFORMANCE_TUNNING
			dev= &rsrc->hds[i];
			OSSW_INIT_SPIN_LOCK(&dev->resource_lock);
			OSSW_INIT_SPIN_LOCK(&dev->ncq_tags_spin_lock);
#if defined(_OS_WINDOWS)
			InitializeListHead(&dev->resource_queue);
#endif
			OSSW_INIT_SPIN_LOCK(&dev->base.err_ctx.sent_req_list_SpinLock);
			OSSW_INIT_SPIN_LOCK(&dev->base.base_SpinLock);
			OSSW_INIT_SPIN_LOCK(&dev->base.outstanding_SpinLock);
#endif
			rsrc->hds[i].base.queue_pointer.next =
				(List_Head *)&rsrc->hds[i+1];
		}
#ifdef ATHENA_PERFORMANCE_TUNNING
		dev= &rsrc->hds[hd_count-1];
		OSSW_INIT_SPIN_LOCK(&dev->resource_lock);
		OSSW_INIT_SPIN_LOCK(&dev->ncq_tags_spin_lock);
#if defined(_OS_WINDOWS)
		InitializeListHead(&dev->resource_queue);
#endif
		OSSW_INIT_SPIN_LOCK(&dev->base.err_ctx.sent_req_list_SpinLock);
		OSSW_INIT_SPIN_LOCK(&dev->base.base_SpinLock);
		OSSW_INIT_SPIN_LOCK(&dev->base.outstanding_SpinLock);
#endif
		rsrc->hds[hd_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->hd_count = hd_count;

	/* allocate memory for expander */
	if (exp_count > 0) {
		item_size = sizeof(domain_expander) * exp_count;
		rsrc->expanders = (domain_expander *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->expanders == NULL) return MV_FALSE;
		for (i = 0; i < exp_count-1; i++){
#ifdef ATHENA_PERFORMANCE_TUNNING
			exp= &rsrc->expanders[i];
			OSSW_INIT_SPIN_LOCK(&exp->base.err_ctx.sent_req_list_SpinLock);
			OSSW_INIT_SPIN_LOCK(&exp->base.base_SpinLock);
			OSSW_INIT_SPIN_LOCK(&exp->base.outstanding_SpinLock);
#endif
			rsrc->expanders[i].base.queue_pointer.next =
				(List_Head *)&rsrc->expanders[i+1];
		}
#ifdef ATHENA_PERFORMANCE_TUNNING
		exp= &rsrc->expanders[exp_count-1];
		OSSW_INIT_SPIN_LOCK(&exp->base.err_ctx.sent_req_list_SpinLock);
		OSSW_INIT_SPIN_LOCK(&exp->base.base_SpinLock);
		OSSW_INIT_SPIN_LOCK(&exp->base.outstanding_SpinLock);
#endif
		rsrc->expanders[exp_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->exp_count = exp_count;

	/* allocate memory for pm */
	if (pm_count > 0) {
		item_size = sizeof(domain_pm) * pm_count;
		rsrc->pms = (domain_pm *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->pms == NULL) return MV_FALSE;
		for (i = 0; i < pm_count-1; i++){
#ifdef ATHENA_PERFORMANCE_TUNNING
			pm= &rsrc->pms[i];
			OSSW_INIT_SPIN_LOCK(&pm->base.base_SpinLock);
			OSSW_INIT_SPIN_LOCK(&pm->base.outstanding_SpinLock);
			OSSW_INIT_SPIN_LOCK(&pm->base.err_ctx.sent_req_list_SpinLock);
#endif
			rsrc->pms[i].base.queue_pointer.next =
				(List_Head *)&rsrc->pms[i+1];
		}
#ifdef ATHENA_PERFORMANCE_TUNNING
		pm= &rsrc->pms[pm_count-1];
		OSSW_INIT_SPIN_LOCK(&pm->base.base_SpinLock);
		OSSW_INIT_SPIN_LOCK(&pm->base.outstanding_SpinLock);
		OSSW_INIT_SPIN_LOCK(&pm->base.err_ctx.sent_req_list_SpinLock);
#endif
		rsrc->pms[pm_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->pm_count = pm_count;

	/* allocate memory for enclosure */
	if (enc_count > 0) {
		item_size = sizeof(domain_enclosure) * enc_count;
		rsrc->enclosures = (domain_enclosure *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->enclosures == NULL) return MV_FALSE;
		for (i = 0; i < enc_count-1; i++){
#ifdef ATHENA_PERFORMANCE_TUNNING
			enc= &rsrc->enclosures[i];
			OSSW_INIT_SPIN_LOCK(&enc->base.err_ctx.sent_req_list_SpinLock);
			OSSW_INIT_SPIN_LOCK(&enc->base.base_SpinLock);
			OSSW_INIT_SPIN_LOCK(&enc->base.outstanding_SpinLock);
#endif

			rsrc->enclosures[i].base.queue_pointer.next =
				(List_Head *)&rsrc->enclosures[i+1];
		}
#ifdef ATHENA_PERFORMANCE_TUNNING
		enc= &rsrc->enclosures[enc_count-1];
		OSSW_INIT_SPIN_LOCK(&enc->base.err_ctx.sent_req_list_SpinLock);
		OSSW_INIT_SPIN_LOCK(&enc->base.base_SpinLock);
		OSSW_INIT_SPIN_LOCK(&enc->base.outstanding_SpinLock);
#endif
		rsrc->enclosures[enc_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->enc_count = enc_count;

	return MV_TRUE;
}


MV_BOOLEAN lib_rsrc_allocate(lib_resource_mgr *rsrc, MV_U16 max_io)
{
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	MV_U32 item_size;
	MV_PU8 vir;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	cmd_resource*cmd_res= NULL;
#endif
	MV_Request *req = NULL;
	hw_buf_wrapper *wrapper = NULL;
	core_context *context = NULL;
	event_record *event = NULL;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	struct device_spin_up *device_su=NULL;
#endif
	MV_U16 i;

	MV_BOOLEAN ret;
#if defined(HAVE_HW_COMPLIANT_SG)
	MV_PHYSICAL_ADDR mem_p;
    MV_PVOID tmp_vir;
#endif

	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	//JING TESTING: Need a way to make cached memory size pointer value aligned.
	/* allocate memory for internal request including sg entry and sense */
	item_size = INTERNAL_REQ_TOTAL_SIZE(req_sg_entry_count) * intl_req_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->intl_req_pool = NULL;
	for (i = 0; i < intl_req_count; i++) {
		req = (MV_Request *)vir;
		vir += sizeof(MV_Request);

#if defined(HAVE_HW_COMPLIANT_SG)
		tmp_vir = lib_rsrc_malloc_dma(rsrc,
			sizeof(MV_SG_Entry) * req_sg_entry_count, 64, &mem_p);
		req->SG_Table.Entry_Ptr = tmp_vir;
		req->bus_addr.parts.low = mem_p.parts.low;
		req->bus_addr.parts.high = mem_p.parts.high;
		MV_ASSERT(req->SG_Table.Entry_Ptr);
#else
		req->SG_Table.Entry_Ptr = (PMV_SG_Entry)vir;
#endif
		req->SG_Table.Max_Entry_Count = req_sg_entry_count;

		vir += sizeof(MV_SG_Entry) * req_sg_entry_count;

		req->Sense_Info_Buffer = vir;
		req->Sense_Info_Buffer_Length = sizeof(sense_data);
		vir += sizeof(sense_data);

		req->Queue_Pointer.next = (List_Head *)rsrc->intl_req_pool;
		rsrc->intl_req_pool = req;
	}
	rsrc->intl_req_count = intl_req_count;

	item_size = sizeof(core_context) * context_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->context_pool = NULL;
	for (i = 0; i < context_count; i++) {
		context = (core_context *)vir;
		context->type = CORE_CONTEXT_TYPE_NONE;
		context->next = rsrc->context_pool;
		rsrc->context_pool = context;
		vir += sizeof(core_context);
	}
	rsrc->context_count = context_count;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	item_size = sizeof(cmd_resource) * context_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->cmd_resource_pool = NULL;
	for (i = 0; i < context_count; i++) {
		cmd_res = (cmd_resource *)vir;
		cmd_res->type = CORE_CONTEXT_TYPE_NONE;
		cmd_res->next = rsrc->cmd_resource_pool;
		rsrc->cmd_resource_pool = cmd_res;
		vir += sizeof(cmd_resource);
	}
	rsrc->cmd_resource_count = context_count;
#endif
	OSSW_INIT_SPIN_LOCK(&rsrc->resource_SpinLock);
	item_size = sizeof(hw_buf_wrapper) * scratch_buf_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->scratch_buf_pool = NULL;
	for (i = 0; i < scratch_buf_count; i++) {
		wrapper = (hw_buf_wrapper *)vir;
		wrapper->next = rsrc->scratch_buf_pool;
		rsrc->scratch_buf_pool = wrapper;
		vir += sizeof(hw_buf_wrapper);
	}
	rsrc->scratch_buf_count = scratch_buf_count;

	item_size = sizeof(hw_buf_wrapper) * hw_sg_buf_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->hw_sg_buf_pool = NULL;
	for (i = 0; i < hw_sg_buf_count; i++) {
		wrapper = (hw_buf_wrapper *)vir;
		wrapper->next = rsrc->hw_sg_buf_pool;
		rsrc->hw_sg_buf_pool = wrapper;
		vir += sizeof(hw_buf_wrapper);
	}
	rsrc->hw_sg_buf_count = hw_sg_buf_count;
	item_size = sizeof(event_record) * event_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->event_pool = NULL;
	for (i = 0; i < event_count; i++) {
		event = (event_record *)vir;
		event->queue_pointer.next = (List_Head *)rsrc->event_pool;
		rsrc->event_pool = event;
		vir += sizeof(event_record);
	}
	rsrc->event_count = (MV_U8)event_count;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	item_size = sizeof(struct device_spin_up) * CORE_MAX_DEVICE_SUPPORTED;
		vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
		if (vir == NULL) return MV_FALSE;
		rsrc->spin_up_device_pool = NULL;
		for (i = 0; i < CORE_MAX_DEVICE_SUPPORTED; i++) {
			device_su = (struct device_spin_up *)vir;
			device_su->list.next = (List_Head *)rsrc->spin_up_device_pool;
			rsrc->spin_up_device_pool = device_su;
			vir += sizeof(struct device_spin_up);
		}
		rsrc->device_count = (MV_U16)CORE_MAX_DEVICE_SUPPORTED;
#endif

	ret = lib_rsrc_allocate_device(rsrc, max_io);
	if (ret==MV_FALSE) return MV_FALSE;

	return MV_TRUE;
}

#ifdef SUPPORT_SES
MV_BOOLEAN ses_state_machine(MV_PVOID enc_p);
#endif

extern MV_VOID core_init_handlers(core_extension *core);
MV_BOOLEAN core_init_cached_memory(core_extension *core,
	lib_resource_mgr *rsrc, MV_U16 max_io)
{
	pl_root *root;

	MV_U8 i, j;
	MV_BOOLEAN ret;

	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	/* don't zero memory. lib_rsrc is initialized already. */
	core->max_io = max_io;
	if ( max_io==1 )
		core->is_dump = MV_TRUE;
	else
		core->is_dump = MV_FALSE;
	core->hw_sg_entry_count = hw_sg_entry_count;
	core->init_queue_count = 0;
	core->state = CORE_STATE_IDLE;

	MV_COUNTED_LIST_HEAD_INIT(&core->init_queue);
	MV_COUNTED_LIST_HEAD_INIT(&core->error_queue);
#ifdef ROOT_WAITING_QUEUE
#ifdef ATHENA_PERFORMANCE_TUNNING
	OSSW_INIT_SPIN_LOCK(&core->roots[0].waiting_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->roots[1].waiting_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->roots[0].root_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->roots[1].root_SpinLock);
#endif
    MV_COUNTED_LIST_HEAD_INIT(&core->roots[0].waiting_queue);
	MV_COUNTED_LIST_HEAD_INIT(&core->roots[1].waiting_queue);
#else
#ifdef ATHENA_PERFORMANCE_TUNNING
	OSSW_INIT_SPIN_LOCK(&core->waiting_queue_SpinLock);
#endif
	MV_COUNTED_LIST_HEAD_INIT(&core->waiting_queue);
#endif
	MV_COUNTED_LIST_HEAD_INIT(&core->high_priority_queue);
#ifdef ATHENA_PERFORMANCE_TUNNING
	OSSW_INIT_SPIN_LOCK(&core->cmpl_queue_SpinLock);
#endif
	MV_LIST_HEAD_INIT(&core->complete_queue);
	MV_LIST_HEAD_INIT(&core->internal_compl_queue);
	MV_LIST_HEAD_INIT(&core->event_queue);
#ifdef ATHENA_PERFORMANCE_TUNNING
	OSSW_INIT_SPIN_LOCK(&core->core_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->core_queue_running_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->pause_waiting_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->internal_compl_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->init_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->event_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->error_queue_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->high_priority_SpinLock);
	OSSW_INIT_SPIN_LOCK(&core->core_global_SpinLock);
#endif

	core->core_queue_running = MV_FALSE;
	core->pause_waiting_queue = MV_FALSE;
	core->waiting_queue_running = 0;

#ifdef SUPPORT_STAGGERED_SPIN_UP
	MV_LIST_HEAD_INIT(&core->device_spin_up_list);
	core->device_spin_up_timer=NO_CURRENT_TIMER;
#endif
	core_init_handlers(core);

	/* setup PL variables */
	for(i = core->chip_info->start_host, j = 0;
		i < (core->chip_info->start_host + core->chip_info->n_host); i++, j++) {
		root = &core->roots[i];
		root->root_id = j;

		ret = prot_init_pl(root,
			max_io,
			core,
			(MV_LPVOID)((MV_PU8)core->mmio_base +
				(MV_IO_CHIP_REGISTER_BASE + (i * MV_IO_CHIP_REGISTER_RANGE))),
			&core->lib_dev,
			&core->lib_rsrc,
			j);
		if (MV_FALSE == ret) return ret;
	}

	/* resource lib */
	ret = lib_rsrc_allocate(rsrc, max_io);
	if (MV_FALSE == ret) return ret;

	return MV_TRUE;
}

//JING EH TBD: free resource if return MV_FALSE
MV_BOOLEAN core_init_dma_memory(core_extension * core,
	lib_resource_mgr *lib_rsrc, MV_U16 max_io)
{
	MV_PVOID mem_v=NULL;
	MV_PHYSICAL_ADDR mem_p;
	hw_buf_wrapper *wrapper = NULL;
	MV_U32 item_size, tmp_count, tmp_size, allocated_count;
	lib_resource_mgr *rsrc = lib_rsrc;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	mv_command_struct *cmd_struct;
	MV_PHYSICAL_ADDR tmpPhy;
#endif
	MV_U16 i, j;

	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);
	/*
	* HW binds the list with Command List Base Address register
	*  so no cache-tune permitted.
	*/
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)

#ifndef PCI_POOL_SUPPORT
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		item_size =   ROUNDING(sizeof(mv_command_struct), 64) * slot_count; /*sizeof(mv_command_header) * slot_count + (0x40+sizeof(mv_command_table))* slot_count */;	
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;

		item_size =  ROUNDING(sizeof(mv_command_struct), 64); //sizeof(mv_command_struct) ; /*sizeof(mv_command_header) (0x40 + sizeof(mv_command_table));*/
		for (j = 0; j < slot_count; j++) {
			core->roots[i].cmd_struct_wrapper[j].vir = mem_v;
			core->roots[i].cmd_struct_wrapper[j].phy = mem_p;
			MV_ZeroMemory(mem_v, item_size);
			cmd_struct= (mv_command_struct *)mem_v;
			tmpPhy.value = mem_p.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, status_buff);
			cmd_struct->header.mv_cmd_header.status_buff_addr = MV_CPU_TO_LE64(tmpPhy);
#if defined(SUPPORT_ATHENA)
			cmd_struct->header.mv_cmd_header.reserved[0] = 0;
			cmd_struct->header.mv_cmd_header.reserved[1] = 0;
#endif
   			tmpPhy.value = mem_p.value + OFFSET_OF(struct _mv_command_struct, mv_cmd_table) + OFFSET_OF(struct _mv_command_table, open_address_frame);
			cmd_struct->header.mv_cmd_header.open_addr_frame_addr  = MV_CPU_TO_LE64(tmpPhy);
			core_set_cmd_header_selector(&cmd_struct->header.mv_cmd_header);
			mem_v = (MV_PU8) mem_v + item_size;
			mem_p = U64_ADD_U32(mem_p, item_size);
		}
	}
#endif/*PCI_POOL_SUPPORT*/

#else
	item_size = sizeof(mv_command_header) * slot_count;
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		core->roots[i].cmd_list = mem_v;
		core->roots[i].cmd_list_dma = mem_p;
	}	
#endif /*SUPPORT_ATHENA*/

	/* assign delivery queue (64 byte align) */
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)       // Deliver Queue Entry is 4 DW
		item_size = sizeof(delv_q_context) * delv_q_size * core->roots[i].queue_num;
#else
		item_size = sizeof(MV_U32) * delv_q_size;
#endif
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		for(j =0; j < core->roots[i].queue_num; j++){
			core->roots[i].queues[j].delv_q = (delv_q_context *)mem_v;
			core->roots[i].queues[j].delv_q_dma = mem_p;
			mem_v = (MV_PU8) mem_v + delv_q_size* sizeof(delv_q_context);
			mem_p = U64_ADD_U32(mem_p, delv_q_size * sizeof(delv_q_context));
		}
		item_size = sizeof(delv_q_context) * core->roots[i].queue_num;
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 4, &mem_p);
		for(j =0; j< core->roots[i].queue_num; j++){
			core->roots[i].queues[j].delv_q_shadow = (delv_q_context *)mem_v;
			core->roots[i].queues[j].delv_q_shadow_dma = mem_p;
			mem_v = (MV_PU8) mem_v + sizeof(delv_q_context);
			mem_p = U64_ADD_U32(mem_p, sizeof(delv_q_context));
		}
#else
		core->roots[i].delv_q = mem_v;
		core->roots[i].delv_q_dma = mem_p;
#endif
	}

	/* assign completion queue (64 byte align) */
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)       // Deliver Queue Entry is 4 DW
		item_size = sizeof(MV_U32) * cmpl_q_size * core->roots[i].queue_num;
#else
		item_size = sizeof(MV_U32) * cmpl_q_size;
#endif
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		for (j =0; j < core->roots[i].queue_num; j++) {
			core->roots[i].queues[j].cmpl_q = mem_v;
			core->roots[i].queues[j].cmpl_q_dma = mem_p;
			mem_v = (MV_PU8) mem_v + cmpl_q_size * sizeof(MV_U32);
			mem_p = U64_ADD_U32(mem_p, cmpl_q_size * sizeof(MV_U32));
		}
		item_size = sizeof(MV_U32) * core->roots[i].queue_num;
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 4, &mem_p);
		for (j =0; j < core->roots[i].queue_num; j++) {
			core->roots[i].queues[j].cmpl_q_shadow = (MV_PVOID)mem_v;
			core->roots[i].queues[j].cmpl_q_shadow_dma = mem_p;
			mem_v = (MV_PU8) mem_v + sizeof(MV_U32);
			mem_p = U64_ADD_U32(mem_p, sizeof(MV_U32));
		}
#else
		core->roots[i].cmpl_wp = mem_v;
		core->roots[i].cmpl_wp_dma = mem_p;
		core->roots[i].cmpl_q = (MV_PU32)((MV_PU8)mem_v + sizeof(MV_U32));
		core->roots[i].cmpl_q_dma = U64_ADD_U32(mem_p, sizeof(MV_U32));
#endif
	}

	/* assign dma memory for received FIS (256 byte align) */
	//JING TBD: better change to centralized function for size. search is_dump here.
	if (core->is_dump) {
		item_size = MIN_RX_FIS_POOL_SIZE;
	} else {
		item_size = MAX_RX_FIS_POOL_SIZE;
	}
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		/* for hibernation, the FIS memory is shared */
		if (i==0 || !core->is_dump) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 256, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		}
		core->roots[i].rx_fis = mem_v;
		core->roots[i].rx_fis_dma = mem_p;
	}
#ifdef SUPPORT_SECURITY_KEY_RECORDS
    item_size=slot_count* sizeof(mv_security_key_record);
    for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
        mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 8, &mem_p);
        if (mem_v == NULL) return MV_FALSE;
        core->roots[i].security_key = (mv_security_key_record *)mem_v;
        core->roots[i].security_key_dma = mem_p;
    }
#endif
#ifndef PCI_POOL_SUPPORT
/* assign the command tables (128 byte align) */
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	item_size = sizeof(mv_command_table); /* command tables can be at separated address */
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		allocated_count = 0;
		tmp_count = slot_count;
		do {
			tmp_size = sizeof(mv_command_table) * tmp_count;
			/* try from largest buffer to smallest */
			mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 128, &mem_p);

			if (mem_v != NULL) {
				for (j = 0; j < tmp_count; j++) {
					MV_ASSERT(allocated_count < slot_count);
					core->roots[i].cmd_table_wrapper[allocated_count].vir = mem_v;
					core->roots[i].cmd_table_wrapper[allocated_count].phy = mem_p;
					mem_v = (MV_PU8)mem_v + item_size;
					mem_p = U64_ADD_U32(mem_p, item_size);
					allocated_count++;
					if (allocated_count == slot_count) {
						break;
					}
				}
			} else {
				CORE_DPRINT(("Can not allocate uncached memory %X.\n",tmp_size));
				tmp_count >>= 1;
			}
		} while ((tmp_count != 0) && (allocated_count < slot_count));

		if (allocated_count < slot_count) return MV_FALSE;
	}
#endif

/* 
    assign the hardware SG buffer (8 byte(size of AHCI command header) align)
    remember must satisfy AHCI spec. ch4.2.2, command table base address need 128 byte alignment
 */
	item_size = hw_sg_entry_count * CORE_HW_SG_ENTRY_SIZE;
	allocated_count = 0;
	tmp_count = hw_sg_buf_count;
	wrapper = rsrc->hw_sg_buf_pool;
	do {
		tmp_size = item_size * tmp_count;
		mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 8, &mem_p);
		if (mem_v != NULL){
			for (j=0; j < tmp_count; j++) {
				MV_ASSERT(wrapper != NULL);
				wrapper->vir = mem_v;
				wrapper->phy = mem_p;
				wrapper = wrapper->next;
				mem_v = (MV_PU8)mem_v + item_size;
				mem_p = U64_ADD_U32(mem_p, item_size);
				allocated_count++;
				if (allocated_count == hw_sg_buf_count) {
					break;
				}
			}
		} else {
			tmp_count >>= 1;
		}
	} while ((tmp_count != 0) && (allocated_count < hw_sg_buf_count));
	if (allocated_count < hw_sg_buf_count) return MV_FALSE;
	MV_ASSERT(wrapper == NULL);
#else
/* subtle memory managemet under Linux */
#ifdef VS_2012
	sprintf_s(core->ct_name,"%s","mv_ct_pool_");
#else
	sprintf(core->ct_name,"%s","mv_ct_pool_");
#endif
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	core->ct_pool= ossw_pci_pool_create(core->ct_name, core, sizeof(mv_command_table), 64 ,0);
#else
	core->ct_pool= ossw_pci_pool_create(core->ct_name, core, sizeof(mv_command_struct), 64 ,0);
#endif
	if (core->ct_pool == NULL)
		return MV_FALSE;
 /*hardware xor need use sg_buffer, not dynamicly allocate for hardware sg buffer
   *if use hardware xor temporarily,
   *fix stress test on rebuilding raid5 KP under linux.
 */
#ifndef HARDWARE_XOR
	item_size = hw_sg_entry_count * CORE_HW_SG_ENTRY_SIZE;
#ifdef VS_2012
	sprintf_s(core->sg_name,"%s","mv_sg_pool_");
#else
	sprintf(core->sg_name,"%s","mv_sg_pool_");
#endif
	core->sg_pool = ossw_pci_pool_create(core->sg_name, core, item_size, 16, 0);
	if (core->sg_pool == NULL)
		return MV_FALSE;
#else
	/* assign the hardware SG buffer (8 byte align) */
	item_size = hw_sg_entry_count * CORE_HW_SG_ENTRY_SIZE;
	allocated_count = 0;
	tmp_count = hw_sg_buf_count;
	wrapper = rsrc->hw_sg_buf_pool;
	do {
		tmp_size = item_size * tmp_count;
		mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 8, &mem_p);
		if (mem_v != NULL){
			for (j=0; j < tmp_count; j++) {
				MV_ASSERT(wrapper != NULL);
				wrapper->vir = mem_v;
				wrapper->phy = mem_p;
				wrapper = wrapper->next;
				mem_v = (MV_PU8)mem_v + item_size;
				mem_p = U64_ADD_U32(mem_p, item_size);
				allocated_count++;
				if (allocated_count == hw_sg_buf_count) {
					break;
				}
			}
		} else {
			tmp_count >>= 1;
		}
	} while ((tmp_count != 0) && (allocated_count < hw_sg_buf_count));
	if (allocated_count < hw_sg_buf_count) return MV_FALSE;
	MV_ASSERT(wrapper == NULL);
#endif
#endif

/* assign the scratch buffer resource (8 byte align) */
	item_size = SCRATCH_BUFFER_SIZE;
	allocated_count = 0;
	tmp_count = scratch_buf_count;
	wrapper = rsrc->scratch_buf_pool;
	do {
		tmp_size = item_size * tmp_count;
		mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 8, &mem_p);
		if (mem_v != NULL) {
			for (j = 0; j < tmp_count; j++) {
				MV_ASSERT(wrapper != NULL);
				wrapper->vir = mem_v;
				wrapper->phy = mem_p;
				wrapper = wrapper->next;
				mem_v = (MV_PU8)mem_v + item_size;
				mem_p = U64_ADD_U32(mem_p, item_size);
				allocated_count++;
				if (allocated_count == scratch_buf_count) {
					break;
				}
			}
		} else {
			tmp_count >>= 1;
		}
	} while ((tmp_count != 0) && (allocated_count < scratch_buf_count));
	if (allocated_count < scratch_buf_count) return MV_FALSE;
	MV_ASSERT(wrapper == NULL);

    #ifdef DOUBLEBUF_4_SINGLE_PRD
        #ifdef PRD_ENTRY_IN_SRAM
        core->doublebuf_vir = gHBA->Base_Address[5];
        core->doublebuf_dma.value = 0xF4000000;
        #else
        tmp_size = 128 * 1024;
        mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 128, &mem_p);
        if (mem_v != NULL) {
            core->doublebuf_dma = mem_p;
            core->doublebuf_vir = mem_v;
        } else {
            MV_PRINT("DoubleBuf is not allocated.\n");
            return MV_FALSE;
        }
        #endif
    #endif
#ifdef HOTPLUG_BYTE_COUNT_ERROR_WORKAROUND
	if(!core->is_dump) {
	/* assign trash bucket */
		mem_v = lib_rsrc_malloc_dma(rsrc, TRASH_BUCKET_SIZE, 8, &mem_p);
		if (mem_v != NULL) {
		/* MV_PHYSICAL_ADDR may be MV_U32, which hasn't "value" member*/
			core->trash_bucket_dma = mem_p;
		} else  {
			return MV_FALSE;
		}
	}
#endif
	return MV_TRUE;
}

PMV_Request get_intl_req_handler(lib_resource_mgr *rsrc);
void free_intl_req_handler(lib_resource_mgr *rsrc, PMV_Request req);
core_context *get_core_context_handler(lib_resource_mgr *rsrc);
void free_core_context_handler(lib_resource_mgr *rsrc, core_context *context);
hw_buf_wrapper *get_scratch_buf_handler(lib_resource_mgr *rsrc);
void free_scratch_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper);
hw_buf_wrapper *get_sg_buf_handler(lib_resource_mgr *rsrc);
void free_sg_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper);
#ifdef SUPPORT_STAGGERED_SPIN_UP
struct device_spin_up *get_device_spin_up_handler(lib_resource_mgr *rsrc);
void free_device_spin_up_handler(lib_resource_mgr *rsrc, struct device_spin_up *device);
#endif
event_record *get_event_record_handler(lib_resource_mgr *rsrc);
void free_event_record_handler(lib_resource_mgr *rsrc, event_record *event);
domain_device *get_device_handler(lib_resource_mgr *rsrc);
void free_device_handler(lib_resource_mgr *rsrc, domain_device *device);
domain_expander *get_expander_handler(lib_resource_mgr *rsrc);
void free_expander_handler(lib_resource_mgr *rsrc, domain_expander *exp);
domain_pm *get_pm_handler(lib_resource_mgr *rsrc);
void free_pm_handler(lib_resource_mgr *rsrc, domain_pm *pm);

domain_enclosure *get_enclosure_handler(lib_resource_mgr *rsrc);
void free_enclosure_handler(lib_resource_mgr *rsrc, domain_enclosure *enc);
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
cmd_resource *get_cmd_resource_handler(lib_resource_mgr *rsrc);
void free_cmd_resource_handler(lib_resource_mgr *rsrc, cmd_resource *cmd_res);
#endif
void lib_rsrc_init(lib_resource_mgr *rsrc, MV_PVOID cached_vir, MV_U32 cached_size,
	MV_PVOID dma_vir, MV_PHYSICAL_ADDR dma_phy, MV_U32 dma_size,
	resource_func_tbl *func, lib_device_mgr *lib_dev)
{
	/* global cached memory buffer */
	MV_ASSERT((((MV_PTR_INTEGER)cached_vir) & (SIZE_OF_POINTER-1)) == 0);
	rsrc->global_cached_vir = cached_vir;
	rsrc->free_cached_vir = cached_vir;
	rsrc->total_cached_size = cached_size;
	rsrc->free_cached_size = cached_size;

	/* global dma memory virtual and physical address */
	MV_ASSERT((((MV_PTR_INTEGER)dma_vir) & (SIZE_OF_POINTER-1)) == 0);
	rsrc->global_dma_vir = dma_vir;
	rsrc->free_dma_vir = dma_vir;
	rsrc->global_dma_phy = dma_phy;
	rsrc->free_dma_phy = dma_phy;
	rsrc->total_dma_size = dma_size;
	rsrc->free_dma_size = dma_size;

	rsrc->func_tbl = *func;
	rsrc->lib_dev = lib_dev;
}

void * lib_rsrc_malloc_cached(lib_resource_mgr *rsrc, MV_U32 size)
{
	void * vir;

	/* always give pointer-aligned buffer */

	size = ROUNDING(size, SIZE_OF_POINTER);
	if (rsrc->free_cached_size < size) {
		MV_ASSERT(rsrc->func_tbl.malloc != NULL);
		return rsrc->func_tbl.malloc(rsrc->func_tbl.extension,size, RESOURCE_CACHED_MEMORY, SIZE_OF_POINTER, NULL);
	}

	vir = rsrc->free_cached_vir;
	rsrc->free_cached_vir = (MV_PU8)vir + size;
	rsrc->free_cached_size -= size;

	MV_DASSERT((((MV_PTR_INTEGER)vir) & (SIZE_OF_POINTER-1)) == 0);
	return vir;
}

void * lib_rsrc_malloc_dma(lib_resource_mgr *rsrc, MV_U32 size,
	MV_U16 alignment, MV_PHYSICAL_ADDR *phy)
{
	MV_U32 offset = 0;
	void * vir;

	size = ROUNDING(size, SIZE_OF_POINTER);

	offset = (MV_U32)
		(ROUNDING(rsrc->free_dma_phy.value, alignment) - rsrc->free_dma_phy.value);
	if ((rsrc->free_dma_size + offset) < size) {
		MV_DASSERT(rsrc->func_tbl.malloc != NULL);
		return rsrc->func_tbl.malloc(rsrc->func_tbl.extension, size, RESOURCE_UNCACHED_MEMORY, alignment, phy);
	}

	vir = (MV_PU8)rsrc->free_dma_vir + offset;
	*phy = U64_ADD_U32(rsrc->free_dma_phy, offset);

	rsrc->free_dma_vir = (MV_PU8)vir + size;
	rsrc->free_dma_phy = U64_ADD_U32(*phy, size);
	rsrc->free_dma_size -= offset + size;

	return vir;
}

MV_BOOLEAN core_init_obj(lib_resource_mgr *rsrc, void *obj, MV_U8 obj_type)
{
	switch (obj_type) {
	case CORE_RESOURCE_TYPE_INTERNAL_REQ:
		{
			MV_Request *req = (MV_Request *)obj;
			req->Req_Type = REQ_TYPE_CORE;
			break;
		}
	case CORE_RESOURCE_TYPE_CONTEXT:
		{
			core_context *ctx = (core_context *)obj;
			ctx->req_type = CORE_REQ_TYPE_NONE;
			ctx->error_info = 0;
			ctx->type = CORE_CONTEXT_TYPE_NONE;
			ctx->req_state = 0;
			ctx->req_flag = 0;
			ctx->handler = NULL; /* in post_callback will check this field. */
			#ifdef MV_DEBUG
			/* should already clean up when free resource */
			MV_DASSERT(ctx->buf_wrapper == NULL);
			MV_DASSERT(ctx->sg_wrapper == NULL);
			#endif
			break;
		}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	case CORE_RESOURCE_TYPE_CMD_RESOURCE:
		{
			cmd_resource *cmd_res = (cmd_resource*)obj;
			cmd_res->req_type = 0;
			cmd_res->error_info = 0;
			cmd_res->type = 0;
			cmd_res->req_state = 0;
			cmd_res->req_flag = 0;
			cmd_res->sg_wrapper = 0;
            		cmd_res->cmd_table_wrapper = 0;
			break;
		}
#endif
	case CORE_RESOURCE_TYPE_EVENT_RECORD:
		{
			event_record *event = (event_record *)obj;
			event->handle_time = 0x0;
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
		{
			domain_device *dev = (domain_device *)obj;
                        /* check before release, it's detached. */
                        MV_ASSERT(dev->base.exp_queue_pointer.prev == NULL);
                        MV_ASSERT(dev->base.exp_queue_pointer.next == NULL);
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
			MV_ZeroMemory(dev, OFFSET_OF(domain_device, resource_queue));
#else
			MV_ZeroMemory(dev, sizeof(domain_device));
#endif
			dev->state = DEVICE_STATE_IDLE;
			dev->queue_id = 0xFFFF;
			dev->base.id = add_device_map(rsrc->lib_dev, &dev->base);
			if (dev->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
		{
			domain_expander *exp = (domain_expander *)obj;
			MV_ZeroMemory(exp, sizeof(domain_expander));
			exp->base.id = add_device_map(rsrc->lib_dev, &exp->base);
			if (exp->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_PM:
		{
			domain_pm *pm = (domain_pm *)obj;
			MV_ZeroMemory(pm, sizeof(domain_pm));
			pm->base.id = add_device_map(rsrc->lib_dev, &pm->base);
			if (pm->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
		{
			domain_enclosure *enc = (domain_enclosure *)obj;
			MV_ZeroMemory(enc, sizeof(domain_enclosure));
			enc->base.id = add_device_map(rsrc->lib_dev, &enc->base);
			if (enc->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
#ifdef SUPPORT_STAGGERED_SPIN_UP
	case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
		{
			struct device_spin_up *su=(struct device_spin_up *)obj;
			MV_ZeroMemory(su,sizeof(struct device_spin_up));
		}

		break;
#endif

	default:
		break;
	}

	return MV_TRUE;
}

//JING EH TBD: core_malloc and core_free function for dynamic allocation
void * core_malloc(MV_PVOID root_p, lib_resource_mgr *rsrc, MV_U8 obj_type)
{
	void * obj = NULL;
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)rsrc->core;
	MV_ULONG flags;

	/*root_p isn't NULL only for device(device, enc, pm, expander) resource allocate.*/
	if (NULL != root_p) {
		core_extension *core = (core_extension *)root->core;
		if (core == NULL) {
			MV_ASSERT(MV_FALSE);
			return NULL;
		}
	}

	//OSSW_SPIN_LOCK_RESOURCE(core, flags);
	OSSW_SPIN_LOCK(&core->lib_rsrc.resource_SpinLock, flags);
		switch (obj_type) {
		case CORE_RESOURCE_TYPE_INTERNAL_REQ:
			obj = get_intl_req_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_CONTEXT:
			obj = get_core_context_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_SCRATCH_BUFFER:
			obj = get_scratch_buf_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_SG_BUFFER:
			obj = get_sg_buf_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_EVENT_RECORD:
			obj = get_event_record_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
			obj = get_device_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
			obj = get_expander_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_PM:
			obj = get_pm_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
			obj = get_enclosure_handler(rsrc);
			break;
	#ifdef SUPPORT_STAGGERED_SPIN_UP
		case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
			obj =  get_device_spin_up_handler(rsrc);
			break;
	#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
		case CORE_RESOURCE_TYPE_CMD_RESOURCE:
			obj =  get_cmd_resource_handler(rsrc);
			break;
#endif
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}
	//OSSW_SPIN_UNLOCK_RESOURCE(core, flags);
	OSSW_SPIN_UNLOCK(&core->lib_rsrc.resource_SpinLock, flags);
	if (obj != NULL) {
		if (core_init_obj(rsrc, obj, obj_type) == MV_FALSE) {
			core_free(root_p, rsrc, obj, obj_type);
			obj = NULL;
		}
	}

	return obj;
}

static void core_clean_obj(lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type)
{
	switch (obj_type) {
	case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
		{
			domain_device *dev = (domain_device *)obj;
			if (dev->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, dev->base.id);
			}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
                    while(!IsListEmpty(&dev->resource_queue)){
                        cmd_resource * cmd_res;
                        cmd_res = (cmd_resource *)LIST_ENTRY(ExInterlockedRemoveHeadList(&dev->resource_queue, &dev->resource_lock), cmd_resource, list_head);
                        free_cmd_resource_handler(rsrc, cmd_res);
                    }
#endif
			MV_ASSERT(dev->base.err_ctx.timer_id == NO_CURRENT_TIMER);
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
		{
			domain_expander *exp = (domain_expander *)obj;
			if (exp->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, exp->base.id);
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_PM:
		{
			domain_pm *pm = (domain_pm *)obj;
			if (pm->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, pm->base.id);
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
		{
			domain_enclosure *enc = (domain_enclosure *)obj;
			if (enc->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, enc->base.id);
			}
			break;
		}
	default:
		break;
	}
}

void core_free(MV_PVOID root_p, lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type)
{
	pl_root *root = (pl_root *)root_p;
	core_extension  *core = (core_extension *)rsrc->core;
	MV_ULONG flags;

	/*root_p isn't NULL only for device(device, enc, pm, expander) resource allocate.*/
	if (NULL != root_p) {
		core_extension *core = (core_extension *)root->core;
		if (core == NULL) {
			MV_ASSERT(MV_FALSE);
			return;
		}
	}

	//OSSW_SPIN_LOCK_RESOURCE(core, flags);
	OSSW_SPIN_LOCK(&core->lib_rsrc.resource_SpinLock, flags);
	core_clean_obj(rsrc, obj, obj_type);

	if (rsrc->func_tbl.free == NULL) {
		switch (obj_type) {
		case CORE_RESOURCE_TYPE_INTERNAL_REQ:
			free_intl_req_handler(rsrc, (PMV_Request)obj);
			break;
		case CORE_RESOURCE_TYPE_CONTEXT:
			free_core_context_handler(rsrc, (core_context *)obj);
			break;
		case CORE_RESOURCE_TYPE_SCRATCH_BUFFER:
                        free_scratch_buf_handler(rsrc, (hw_buf_wrapper *)obj);
			break;
		case CORE_RESOURCE_TYPE_SG_BUFFER:
                        free_sg_buf_handler(rsrc, (hw_buf_wrapper *)obj);
			break;
		case CORE_RESOURCE_TYPE_EVENT_RECORD:
                        free_event_record_handler(rsrc, (event_record *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
			free_device_handler(rsrc, (domain_device *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
			free_expander_handler(rsrc, (domain_expander *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_PM:
			free_pm_handler(rsrc, (domain_pm *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
			free_enclosure_handler(rsrc, (domain_enclosure *)obj);
			break;
	#ifdef SUPPORT_STAGGERED_SPIN_UP
		case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
			free_device_spin_up_handler(rsrc,(struct device_spin_up *)obj);
			break;
	#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
		case CORE_RESOURCE_TYPE_CMD_RESOURCE:
			free_cmd_resource_handler(rsrc, (cmd_resource *)obj);
			break;
#endif
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
	} else {
		//rsrc->free(obj)
	}
	//OSSW_SPIN_UNLOCK_RESOURCE(core, flags);
	OSSW_SPIN_UNLOCK(&core->lib_rsrc.resource_SpinLock, flags);
}

PMV_Request get_intl_req_handler(lib_resource_mgr *rsrc)
{
	PMV_Request req = NULL;
	if (rsrc->intl_req_count > 0) {
		MV_DASSERT(rsrc->intl_req_pool != NULL);
		req = rsrc->intl_req_pool;
		rsrc->intl_req_pool = (PMV_Request)req->Queue_Pointer.next;
		rsrc->intl_req_count--;
	} else {
		MV_DASSERT(rsrc->intl_req_pool == NULL);
	}

	return req;
}

void free_intl_req_handler(lib_resource_mgr *rsrc, PMV_Request req)
{
	rsrc->intl_req_count++;
	req->Queue_Pointer.next = (List_Head *)rsrc->intl_req_pool;
	rsrc->intl_req_pool = req;
}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
cmd_resource *get_cmd_resource_handler(lib_resource_mgr *rsrc)
{
	cmd_resource *cmd_res = NULL;

	if (rsrc->cmd_resource_count > 0) {
		MV_DASSERT(rsrc->cmd_resource_pool != NULL);
		cmd_res = rsrc->cmd_resource_pool;
		rsrc->cmd_resource_pool = cmd_res->next;
		rsrc->cmd_resource_count--;
	} else {
		MV_DASSERT(rsrc->cmd_resource_pool == NULL);
	}

	return cmd_res;
}

void free_cmd_resource_handler(lib_resource_mgr *rsrc, cmd_resource *cmd_res)
{
	MV_DASSERT(cmd_res != NULL);
	cmd_res->next = rsrc->cmd_resource_pool;
	rsrc->cmd_resource_pool = cmd_res;
	rsrc->cmd_resource_count++;
}
#endif
core_context *get_core_context_handler(lib_resource_mgr *rsrc)
{
	core_context *context = NULL;

	if (rsrc->context_count > 0) {
		MV_DASSERT(rsrc->context_pool != NULL);
		context = rsrc->context_pool;
		rsrc->context_pool = context->next;
		rsrc->context_count--;
	} else {
		MV_DASSERT(rsrc->context_pool == NULL);
	}

	return context;
}

void free_core_context_handler(lib_resource_mgr *rsrc, core_context *context)
{
	MV_DASSERT(context != NULL);
	context->next = rsrc->context_pool;
	rsrc->context_pool = context;
	rsrc->context_count++;
}

hw_buf_wrapper *get_scratch_buf_handler(lib_resource_mgr *rsrc)
{
	hw_buf_wrapper *wrapper = NULL;
	if (rsrc->scratch_buf_count > 0) {
		MV_DASSERT(rsrc->scratch_buf_pool != NULL);
		wrapper = rsrc->scratch_buf_pool;
		rsrc->scratch_buf_pool = wrapper->next;
		rsrc->scratch_buf_count--;

		wrapper->next = NULL;
	} else {
		MV_DASSERT(rsrc->scratch_buf_pool == NULL);
	}

	return wrapper;
}

void free_scratch_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper)
{
	MV_DASSERT(wrapper != NULL);
	wrapper->next = rsrc->scratch_buf_pool;
	rsrc->scratch_buf_pool = wrapper;
	rsrc->scratch_buf_count++;
}

hw_buf_wrapper *get_sg_buf_handler(lib_resource_mgr *rsrc)
{
	hw_buf_wrapper *wrapper = NULL;
	if (rsrc->hw_sg_buf_count > 0) {
		MV_DASSERT(rsrc->hw_sg_buf_pool != NULL);
		wrapper = rsrc->hw_sg_buf_pool;
		rsrc->hw_sg_buf_pool = wrapper->next;
		rsrc->hw_sg_buf_count--;

		wrapper->next = NULL;
	} else {
		MV_DASSERT(rsrc->hw_sg_buf_pool == NULL);
	}

	return wrapper;
}

void free_sg_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper)
{
	MV_DASSERT(wrapper != NULL);
	wrapper->next = rsrc->hw_sg_buf_pool;
	rsrc->hw_sg_buf_pool = wrapper;
	rsrc->hw_sg_buf_count++;
}
#ifdef SUPPORT_STAGGERED_SPIN_UP
struct device_spin_up *get_device_spin_up_handler(lib_resource_mgr *rsrc)
{
	struct device_spin_up *device = NULL;

	if (rsrc->device_count > 0) {
		MV_DASSERT(rsrc->spin_up_device_pool != NULL);
		device = rsrc->spin_up_device_pool;
		rsrc->spin_up_device_pool = (struct device_spin_up *)device->list.next;
		rsrc->device_count--;
	} else {
		MV_DASSERT(rsrc->spin_up_device_pool == NULL);
	}
	return device;
}
void free_device_spin_up_handler(lib_resource_mgr *rsrc, struct device_spin_up *device)
{
	MV_DASSERT(device != NULL);
	device->list.next = (List_Head *)rsrc->spin_up_device_pool;
	rsrc->spin_up_device_pool = device;
	rsrc->device_count++;
}

#endif

event_record *get_event_record_handler(lib_resource_mgr *rsrc)
{
	event_record *event = NULL;

	if (rsrc->event_count > 0) {
		MV_DASSERT(rsrc->event_pool != NULL);
		event = rsrc->event_pool;
		rsrc->event_pool = (event_record *)event->queue_pointer.next;
		rsrc->event_count--;
	} else {
		MV_DASSERT(rsrc->event_pool == NULL);
	}

	return event;
}

void free_event_record_handler(lib_resource_mgr *rsrc, event_record *event)
{
	MV_DASSERT(event != NULL);
	event->queue_pointer.next = (List_Head *)rsrc->event_pool;
	rsrc->event_pool = event;
	rsrc->event_count++;
}

domain_device *get_device_handler(lib_resource_mgr *rsrc)
{
	domain_device *dev = NULL;

	if (rsrc->hd_count > 0) {
		dev = rsrc->hds;
		rsrc->hds = (domain_device *)dev->base.queue_pointer.next;
		rsrc->hd_count--;
	} else {
		MV_DASSERT(rsrc->hds == NULL);
	}

	return dev;
}

void free_device_handler(lib_resource_mgr *rsrc, domain_device *device)
{
	device->base.queue_pointer.next = (List_Head *)rsrc->hds;
	rsrc->hds = device;
	rsrc->hd_count++;
}

domain_expander *get_expander_handler(lib_resource_mgr *rsrc)
{
	domain_expander *exp = NULL;

	if (rsrc->exp_count > 0) {
		exp = rsrc->expanders;
		rsrc->expanders = (domain_expander *)exp->base.queue_pointer.next;
		rsrc->exp_count--;
	} else {
		MV_DASSERT(rsrc->expanders == NULL);
	}

	return exp;
}

void free_expander_handler(lib_resource_mgr *rsrc, domain_expander *exp)
{
	exp->base.queue_pointer.next = (List_Head *)rsrc->expanders;
	rsrc->expanders = exp;
	rsrc->exp_count++;
}

domain_pm *get_pm_handler(lib_resource_mgr *rsrc)
{
	domain_pm *pm = NULL;

	if (rsrc->pm_count > 0) {
		pm = rsrc->pms;
		rsrc->pms = (domain_pm *)pm->base.queue_pointer.next;
		rsrc->pm_count--;
	} else {
		MV_DASSERT(rsrc->pms == NULL);
	}
	return pm;
}

void free_pm_handler(lib_resource_mgr *rsrc, domain_pm *pm)
{
	pm->base.queue_pointer.next = (List_Head *)rsrc->pms;
	rsrc->pms = pm;
	rsrc->pm_count++;
}
domain_enclosure *get_enclosure_handler(lib_resource_mgr *rsrc)
{
	domain_enclosure *enc = NULL;

	if (rsrc->enc_count > 0) {
		enc = rsrc->enclosures;
		rsrc->enclosures = (domain_enclosure *)enc->base.queue_pointer.next;
		rsrc->enc_count--;
	} else {
		MV_DASSERT(rsrc->enclosures == NULL);
	}
	return enc;
}
void free_enclosure_handler(lib_resource_mgr *rsrc, domain_enclosure *enc)
{
	enc->base.queue_pointer.next = (List_Head *)rsrc->enclosures;
	rsrc->enclosures= enc;
	rsrc->enc_count++;
}
