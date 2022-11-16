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

#ifndef __CORE_RESOURCE_H
#define __CORE_RESOURCE_H

#include "mv_config.h"
#include "core_type.h"
#include "core_protocol.h"

struct _core_extension;

/*
 * resource management for sas/sata part
 */
MV_U32 core_get_cached_memory_quota(MV_U16 max_io);
MV_U32 core_get_dma_memory_quota(MV_U16 max_io);
MV_BOOLEAN core_init_cached_memory(struct _core_extension *core, 
	lib_resource_mgr *lib_rsrc, MV_U16 max_io);
MV_BOOLEAN core_init_dma_memory(struct _core_extension * core,
	lib_resource_mgr *lib_rsrc, MV_U16 max_io);

/*
 * resource management interface
 */
void core_get_supported_dev(MV_U16 max_io, MV_PU16 hd_count, MV_PU8 exp_count,
	MV_PU8 pm_count, MV_PU8 enc_count);

void core_get_supported_pl_counts(MV_U16 max_io, MV_PU16 slot_count,
	MV_PU16 delv_q_size, MV_PU16 cmpl_q_size, MV_PU16 received_fis_count);

void xor_get_supported_pl_counts(MV_U16 max_io, MV_PU16 slot_count,
	MV_PU16 delv_q_size, MV_PU16 cmpl_q_size);

void core_get_supported_pal_counts(MV_U16 max_io, MV_PU16 intl_req_count,
	MV_PU16 req_sg_entry_count, MV_PU16 hw_sg_entry_count, MV_PU16 hw_sg_buf_count,
	MV_PU16 scratch_buf_count, MV_PU16 context_count,
	MV_PU16 event_count);

enum resource_type {
	CORE_RESOURCE_TYPE_INTERNAL_REQ = 1,
	CORE_RESOURCE_TYPE_CONTEXT,
	CORE_RESOURCE_TYPE_SCRATCH_BUFFER,
	CORE_RESOURCE_TYPE_SG_BUFFER,
	CORE_RESOURCE_TYPE_EVENT_RECORD,

	CORE_RESOURCE_TYPE_DOMAIN_DEVICE,
	CORE_RESOURCE_TYPE_DOMAIN_EXPANDER,
	CORE_RESOURCE_TYPE_DOMAIN_PM,
	CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE,
#ifdef SUPPORT_STAGGERED_SPIN_UP
	CORE_RESOURCE_TYPE_SPIN_UP_DEVICE,
#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	CORE_RESOURCE_TYPE_CMD_RESOURCE,
#endif
};

typedef struct _resource_func_tbl {
	/* dynamic memory malloc free function */
	void *(*malloc)(void *extension, MV_U32 size, MV_U8 mem_type, MV_U16 alignment, MV_PHYSICAL_ADDR *phy);
	void (*free)(const void * obj);
	void * extension;
} resource_func_tbl;

struct _lib_resource_mgr {
	/*
	 * global cached/dma memory management variables
	 */
	MV_PVOID core;

	/* global cached memory buffer */
	MV_PVOID global_cached_vir;
	MV_PVOID free_cached_vir;
	MV_U32 total_cached_size;
	MV_U32 free_cached_size;

	/* global dma memory virtual and physical address */
	MV_PVOID global_dma_vir;
	MV_PHYSICAL_ADDR global_dma_phy;
	MV_PVOID free_dma_vir;
	MV_PHYSICAL_ADDR free_dma_phy;
	MV_U32 total_dma_size;
	MV_U32 free_dma_size;

	resource_func_tbl	func_tbl;
	lib_device_mgr          *lib_dev;

	/*
	 * allocated cached and dma memory
	 */
	MV_U16                 intl_req_count;
	MV_U16                 context_count;
	MV_U16                 scratch_buf_count;
	MV_U16                 hw_sg_buf_count;
	MV_U8                  event_count;

	/* resource pool for cached memory */
	MV_Request             *intl_req_pool;	/* internal requests pool */
	core_context           *context_pool; /* core context pool */

	/* resource pool for dma memory */
	hw_buf_wrapper         *scratch_buf_pool; /* scratch buffer */
	hw_buf_wrapper         *hw_sg_buf_pool; /* dma sg buffer for ASIC access */
	event_record           *event_pool; /* event record pool */
#ifdef 	SUPPORT_STAGGERED_SPIN_UP
	struct device_spin_up	*spin_up_device_pool;
#endif
	/*
	 * allocated device data structure
	 */
	MV_U16                  hd_count; /* how many domain_device available */
	MV_U8                  exp_count;
	MV_U8                  pm_count;
	MV_U8                  enc_count;
#ifdef 	SUPPORT_STAGGERED_SPIN_UP
	MV_U16                  device_count;
#else
	MV_U8			reserved0[2];
#endif
	domain_device          *hds;
	domain_expander        *expanders;
	domain_pm              *pms;
	domain_enclosure       *enclosures;
#ifdef ATHENA_PERFORMANCE_TUNNING
	MV_U16                 cmd_resource_count;
	MV_U16                 reserved001;
#if defined(_OS_WINDOWS)
	cmd_resource           *cmd_resource_pool; /* core context pool */
	KSPIN_LOCK resource_SpinLock;
	KIRQL      resource_OldIrql;
#elif defined(_OS_LINUX)
	spinlock_t	resource_SpinLock;
#endif
#endif

};

void * core_malloc(MV_PVOID root_p, lib_resource_mgr *rsrc, MV_U8 obj_type);
void core_free(MV_PVOID root_p, lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type);

void * lib_rsrc_malloc_cached(lib_resource_mgr *rsrc, MV_U32 size);
void * lib_rsrc_malloc_dma(lib_resource_mgr *rsrc, MV_U32 size, 
	MV_U16 alignment, MV_PHYSICAL_ADDR *phy);

void lib_rsrc_init(lib_resource_mgr *rsrc, MV_PVOID cached_vir, MV_U32 cached_size,
	MV_PVOID dma_vir, MV_PHYSICAL_ADDR dma_phy, MV_U32 dma_size,
	resource_func_tbl *func, lib_device_mgr *lib_dev);

#define get_intl_req(rsrc) \
	(PMV_Request)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_INTERNAL_REQ)

#define free_intl_req(rsrc, req) \
	core_free(NULL, rsrc, req, CORE_RESOURCE_TYPE_INTERNAL_REQ)

#define get_core_context(rsrc) \
	(core_context *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_CONTEXT)
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
#define get_cmd_resource(rsrc) \
	(cmd_resource *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_CMD_RESOURCE)
#define free_cmd_resource(rsrc, cmd_res) \
	core_free(NULL, rsrc, cmd_res, CORE_RESOURCE_TYPE_CMD_RESOURCE)
#endif
/* to avoid multiple release, set to NULL after release */
#define free_core_context(rsrc, context) \
	core_free(NULL, rsrc, context, CORE_RESOURCE_TYPE_CONTEXT)

#define get_scratch_buf(rsrc) \
	(hw_buf_wrapper *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SCRATCH_BUFFER)

/* to avoid multiple release, set to NULL after release */
#define free_scratch_buf(rsrc, wrapper) \
	core_free(NULL, rsrc, wrapper, CORE_RESOURCE_TYPE_SCRATCH_BUFFER)

#define get_sg_buf(rsrc) \
	(hw_buf_wrapper *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SG_BUFFER)

/* to avoid multiple release, set to NULL after release */
#define free_sg_buf(rsrc, wrapper) \
	core_free(NULL, rsrc, wrapper, CORE_RESOURCE_TYPE_SG_BUFFER)
#ifdef SUPPORT_STAGGERED_SPIN_UP
#define get_spin_up_device_buf(rsrc) \
		(struct device_spin_up *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SPIN_UP_DEVICE)
#define free_spin_up_device_buf(rsrc, device) \
		core_free(NULL, rsrc, device, CORE_RESOURCE_TYPE_SPIN_UP_DEVICE)
#endif
#define get_event_record(rsrc) \
	(event_record *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_EVENT_RECORD)

#define free_event_record(rsrc, event) \
	core_free(NULL, rsrc, event, CORE_RESOURCE_TYPE_EVENT_RECORD)

/* 
 * device object data structure allocation and free 
 */
#define get_device_obj(root_p, rsrc) \
	(domain_device *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_DEVICE)

#define free_device_obj(root_p, rsrc, dev) \
	core_free(root_p, rsrc, dev, CORE_RESOURCE_TYPE_DOMAIN_DEVICE)

#define get_expander_obj(root_p, rsrc) \
	(domain_expander *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_EXPANDER)

#define free_expander_obj(root_p, rsrc, exp) \
	core_free(root_p, rsrc, exp, CORE_RESOURCE_TYPE_DOMAIN_EXPANDER)

#define get_pm_obj(root_p, rsrc) \
	(domain_pm *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_PM)

#define free_pm_obj(root_p, rsrc, pm) \
	core_free(root_p, rsrc, pm, CORE_RESOURCE_TYPE_DOMAIN_PM)

#define get_enclosure_obj(root_p, rsrc) \
	(domain_enclosure *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE)

#define free_enclosure_obj(root_p, rsrc,enc) \
	core_free(root_p, rsrc,enc,CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE)

#endif /* __CORE_RESOURCE_H */
