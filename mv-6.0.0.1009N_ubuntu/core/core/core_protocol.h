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

#ifndef __CORE_PROTOCOL_H
#define __CORE_PROTOCOL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"

typedef struct _hw_buf_wrapper {
	struct _hw_buf_wrapper		*next;
	MV_PVOID					vir;	/* virtual memory address */
	MV_PHYSICAL_ADDR			phy;	/* physical memory address */
} hw_buf_wrapper;

typedef struct _saved_fis {
	MV_U32 dw1;
	MV_U32 dw2;
	MV_U32 dw3;
	MV_U32 dw4;
} saved_fis;

struct _pl_queue {
	MV_PVOID root;
	MV_U16 msix_idx;	/* global id in core */
	MV_U16 id;			/* queue id in this root */

	MV_U32 irq_status;
	MV_U32 irq_mask;

	MV_U16 delv_q_size;
	MV_U16 cmpl_q_size;
	MV_U16 last_delv_q;
	MV_U16 last_cmpl_q;
	MV_U32 attention;
	Counted_List_Head waiting_queue;
	Counted_List_Head high_priority_queue;
	List_Head complete_queue;

	/* delivery queue */
	delv_q_context *delv_q;
	MV_PHYSICAL_ADDR delv_q_dma;
	delv_q_context *delv_q_shadow;
	MV_PHYSICAL_ADDR delv_q_shadow_dma;

	/* completion queue */
	MV_PVOID cmpl_q;
	MV_PHYSICAL_ADDR cmpl_q_dma;
	MV_PVOID cmpl_q_shadow;
	MV_PHYSICAL_ADDR cmpl_q_shadow_dma;
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK waiting_queue_SpinLock;
	KSPIN_LOCK queue_attention_SpinLock;
	KSPIN_LOCK high_priority_SpinLock;
	KSPIN_LOCK complete_SpinLock;
	KSPIN_LOCK queue_SpinLock;
	KSPIN_LOCK handle_cmpl_SpinLock;
	KIRQL      queue_OldIrql;
#elif defined(_OS_LINUX)
	spinlock_t waiting_queue_SpinLock;
	spinlock_t queue_attention_SpinLock;
	spinlock_t high_priority_SpinLock;
	spinlock_t complete_SpinLock;
	spinlock_t queue_SpinLock;
	spinlock_t handle_cmpl_SpinLock;
#endif
#endif
#ifdef ATHENA_MISS_MSIX_INT_WA
	MV_U32 msi_x_miss_count;
#endif

};

struct _pl_root {
	MV_LPVOID	mmio_base;
	MV_PVOID	core;
	lib_device_mgr	 *lib_dev;
	lib_resource_mgr *lib_rsrc;
	MV_U8		root_id;

	MV_U64      sata_reg_set;		   /* bit map. need to change if register set is more than 64 */
	MV_U64      sata_reg_set2;	
	MV_U32		phy_num;
	MV_U32		max_register_set;	   /* per chip core */
	MV_U32       running_num; /* how many requests are outstanding on the ASIC */

	MV_U32		slot_count_support;
	MV_U32		unassoc_fis_offset;
	MV_U32		capability;

	PMV_Request	*running_req;
	saved_fis	*saved_fis_area;
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK root_SpinLock;
	KSPIN_LOCK waiting_queue_SpinLock;
	KIRQL      root_OldIrql;
	KIRQL      waiting_queue_OldIrql;
#elif defined(_OS_LINUX)
	spinlock_t waiting_queue_SpinLock;
	spinlock_t root_SpinLock;
#endif
#endif
#ifdef ROOT_WAITING_QUEUE
    Counted_List_Head          waiting_queue;
#endif
	domain_port	ports[MAX_PORT_PER_PL];
	domain_phy	phy[MAX_PHY_PER_PL];

	Tag_Stack slot_pool;

	/* variables to save the interrupt status */
	MV_U32		comm_irq;
	MV_U32          comm_irq_mask;

	/*keeps the base phy num of the root*/
	MV_U32		base_phy_num;
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
    /* command struct */ /*0~0x40 command header 0x40 ~ command table */
	hw_buf_wrapper *cmd_struct_wrapper;

	/* received FIS */
	MV_PVOID rx_fis;
	MV_PHYSICAL_ADDR rx_fis_dma;
#else
	MV_U16 delv_q_size;
	MV_U16 cmpl_q_size;
	
	/* command header list */
	MV_PVOID cmd_list;
	MV_PHYSICAL_ADDR cmd_list_dma;

	/* command table */
	hw_buf_wrapper *cmd_table_wrapper;
	
	/* received FIS */
	MV_PVOID rx_fis;
	MV_PHYSICAL_ADDR rx_fis_dma;
	
	/* delivery queue */
	MV_PU32 delv_q;
	MV_PHYSICAL_ADDR delv_q_dma;
	
	/* completion queue */
	MV_PVOID cmpl_wp;
	MV_PHYSICAL_ADDR cmpl_wp_dma;
	MV_PU32  cmpl_q;
	MV_PHYSICAL_ADDR cmpl_q_dma;
#endif


#ifdef SUPPORT_SECURITY_KEY_RECORDS
    mv_security_key_record *security_key;
    MV_PHYSICAL_ADDR security_key_dma;
#endif
	MV_U32 max_cmd_slot_width; /* 9 for 512, 10 for 1024 */
#ifdef SUPPORT_INTL_PARITY
	MV_U32 parity_enable_mask; 
#endif
	/* multi queue */
	MV_U16 queue_num;
	pl_queue queues[MAX_MULTI_QUEUE];
	// Jira#QTS00000-2594 Debug
	MV_U16 qnap_interrupt_count;
};

#define MAX_EVENTS 20
typedef struct _event_record{
	List_Head queue_pointer;
	struct _pl_root *root;
	MV_U8 phy_id;
	MV_U8 event_id;
	unsigned long handle_time;
} event_record;

enum _core_context_type {
	CORE_CONTEXT_TYPE_NONE = 0, /* no valid context */
	CORE_CONTEXT_TYPE_CDB =	1, /* context is the cdb and buffer */
	CORE_CONTEXT_TYPE_LARGE_REQUEST = 2, /* context for the large request support */
        CORE_CONTEXT_TYPE_SUB_REQUEST = 3,
        CORE_CONTEXT_TYPE_ORG_REQ = 4, /* Used in SAT and error handling */
	CORE_CONTEXT_TYPE_API = 5,
	CORE_CONTEXT_TYPE_RESET_SATA_PHY = 6,
	CORE_CONTEXT_TYPE_CLEAR_AFFILIATION = 7,
	CORE_CONTEXT_TYPE_MAX,
};

enum _core_req_type {
	CORE_REQ_TYPE_NONE = 0,
	CORE_REQ_TYPE_INIT = 1, /* request generated in init state machine */
	CORE_REQ_TYPE_ERROR_HANDLING = 2, /* request generated during error handling */
	CORE_REQ_TYPE_RETRY = 3, /* is error handling request too, retry org req */
};

enum _core_req_flag {
	CORE_REQ_FLAG_NEED_D2H_FIS	= (1 << 0),
};

/* eh req includes retried req */
#define CORE_IS_EH_RETRY_REQ(ctx) \
	((ctx)->req_type==CORE_REQ_TYPE_RETRY)
#define CORE_IS_EH_REQ(ctx) \
	(((ctx)->req_type==CORE_REQ_TYPE_ERROR_HANDLING) \
		|| ((ctx)->req_type==CORE_REQ_TYPE_RETRY))
#define CORE_IS_INIT_REQ(ctx) \
	((ctx)->req_type==CORE_REQ_TYPE_INIT)

#ifdef SUPPORT_SG_RESET
#define CORE_IS_HBA_RESET_REQ(req) \
	((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) && \
		(req->Cdb[1] == CDB_HBA_RESET))	
#endif

enum _error_info {
	EH_INFO_CMD_ISS_STPD = (1U << 0),
	EH_INFO_NEED_RETRY = (1U << 1),
	EH_INFO_STP_WD_TO_RETRY = (1U << 2),
	EH_INFO_SSP_WD_TO_RETRY = (1U << 3),
};

#define EH_INFO_WD_TO_RETRY (EH_INFO_STP_WD_TO_RETRY | EH_INFO_SSP_WD_TO_RETRY)

/*
 * core context is used for one time request handling.
 * when calling Completion routine, the core_context will be released.
 * even you want to retry the request, the core cotext will be a new one.
 * so dont try to save information like retry_count in core context
 */
typedef struct _core_context {
	struct _core_context *next;

	/*
	 * following is the request context
	 */
	MV_U16 slot; /* which slot it is used */
	MV_U8 type; /* for the following union data structure CORE_CONTEXT_TYPE_XXX */
	MV_U8 req_type; /* core internal request type CORE_REQ_TYPE_XXX */
	MV_U32 req_state;

	MV_U8  ncq_tag;
	MV_U8  req_flag;	/* core internal request flag CORE_REQ_FLAG_XXX */
	MV_U8  reserved[2];

	MV_PVOID buf_wrapper; /* wrapper for the scratch buffer */
	MV_PVOID sg_wrapper; /* wrapper for the hw sg buffer */

	/*
	 *
	 */
	MV_PVOID handler;

	/*
	 * for error handling
	 */
	MV_U32 error_info;

        MV_PVOID received_fis;
        union {
                struct {
                        MV_Request *org_req;
                        MV_U32 other;
                } org;
                struct {
                        MV_U8 sub_req_cmplt;
                        MV_U8 sub_req_count;
                } large_req;
                struct {
                        MV_PVOID org_buff_ptr;
                } sub_req;
                /* for smp virtual reqs */
                struct {
                        MV_U8 current_phy_id;
                        MV_U8 req_remaining;
                } smp_discover;
		struct {
                        MV_U8 curr_dev_count;
                        MV_U8 total_dev_count;
                        MV_U8 req_remaining;
		} smp_reset_sata_phy;
		struct {
			MV_U8 state;
			MV_U8 curr_dev_count;
                        MV_U8 total_dev_count;
			MV_U8 req_remaining;
			MV_U8 need_wait;
		} smp_clear_aff;
                struct {
                        MV_U8 phy_count;
                        MV_U8 address_count;
                        MV_U8 current_phy;
                        MV_U8 current_addr;
                        MV_U8 req_remaining;
                        MV_PVOID org_exp;
                } smp_config_route;
                /*for API expander request*/
                struct{
                        MV_U16 start;
                        MV_U16 end;
                        MV_U32 remaining;
                        MV_PVOID pointer;
                }api_req;
                struct{
                        MV_U16 phy_index;
                        MV_PVOID buffer;
                }api_smp;
		struct{
			MV_U8 affiliation_valid;
		}smp_report_phy;
		sgd_tbl_t org_sg_table;
        } u;
} core_context;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
typedef struct _cmd_resource {
    struct _cmd_resource *next;
    LIST_ENTRY list_head;

    /*
    * following is the request context
    */
    MV_U16 slot; /* which slot it is used */
    MV_U8 type; /* for the following union data structure CORE_CONTEXT_TYPE_XXX */
    MV_U8 req_type; /* core internal request type CORE_REQ_TYPE_XXX */
    MV_U32 req_state;

    MV_U8  ncq_tag;
    MV_U8  req_flag;	/* core internal request flag CORE_REQ_FLAG_XXX */
    MV_U16 reserved;
    MV_PVOID sg_wrapper; /* wrapper for the hw sg buffer */
    MV_PVOID cmd_table_wrapper; /* wrapper for the hw sg buffer */
    MV_U32 error_info;
} cmd_resource;
#endif
#define DC_ATA                                      MV_BIT(0)
#define DC_SCSI                                     MV_BIT(1)
#define DC_SERIAL                                   MV_BIT(2)
#define DC_PARALLEL                                 MV_BIT(3)
#define DC_ATAPI                                    MV_BIT(4)
#define DC_SGPIO                                    MV_BIT(5)
#define DC_I2C                                      MV_BIT(6)

/* PD's Device type defined in SCSI-III specification */
#define DT_DIRECT_ACCESS_BLOCK                      0x00
#define DT_SEQ_ACCESS                               0x01
#define DT_PRINTER                                  0x02
#define DT_PROCESSOR                                0x03
#define DT_WRITE_ONCE                               0x04
#define DT_CD_DVD                                   0x05
#define DT_OPTICAL_MEMORY                           0x07
#define DT_MEDIA_CHANGER                            0x08
#define DT_STORAGE_ARRAY_CTRL                       0x0C
/* an actual enclosure */
#define DT_ENCLOSURE                                0x0D


/* The following are defined by Marvell */
#define DT_EXPANDER                                 0x20
#define DT_PM                                       0x21
/* a device that provides some SES services (ie, Luigi) */
#define DT_SES_DEVICE                               0x22


/* Some macros to make device type easier to use */
#define IS_SSP(dev) ((dev->connection & DC_SCSI) && \
                     (dev->connection & DC_SERIAL) && \
                     !(dev->connection & DC_ATA))

#define IS_STP(dev) ((dev->connection & DC_SCSI) && \
                     (dev->connection & DC_SERIAL) && \
                     (dev->connection & DC_ATA))

#define IS_SATA(dev) (!(dev->connection & DC_SCSI) && \
                      (dev->connection & DC_SERIAL) && \
                      (dev->connection & DC_ATA))

#define IS_STP_OR_SATA(dev) ((dev->connection & DC_ATA) && \
                             (dev->connection & DC_SERIAL))

#define IS_ATAPI(dev) ((dev->connection & DC_ATAPI) && \
                       (dev->connection & DC_ATA))

#define IS_TAPE(dev)           (dev->dev_type == DT_SEQ_ACCESS)
#define IS_ENCLOSURE(dev)      (dev->dev_type == DT_ENCLOSURE)
#define IS_HDD(dev)            (dev->dev_type == DT_DIRECT_ACCESS_BLOCK)
#define IS_OPTICAL(dev)        ((dev->dev_type == DT_WRITE_ONCE) || \
                                (dev->dev_type == DT_CD_DVD) || \
                                (dev->dev_type == DT_OPTICAL_MEMORY))

#define IS_SGPIO(dev)          (dev->connection & DC_SGPIO)
#define IS_I2C(dev)            (dev->connection & DC_I2C)

#define IS_BEHIND_PM(dev)      (dev->pm != NULL)

MV_BOOLEAN prot_init_pl(pl_root *root, MV_U16 max_io,
	MV_PVOID core, MV_LPVOID mmio,
	lib_device_mgr *lib_dev, lib_resource_mgr *lib_rsrc, MV_U16 root_idx);

MV_QUEUE_COMMAND_RESULT prot_send_request(pl_root *root,
	struct _domain_base *base, MV_Request *req);

MV_U32 prot_get_delv_q_entry(pl_root *root, MV_U16 queue_id);
MV_VOID prot_write_delv_q_entry(pl_root *root, MV_U32 entry, MV_U16 queue_id);

PMV_Request get_intl_req_resource(pl_root *root, MV_U32 buf_size);
MV_VOID intl_req_release_resource(lib_resource_mgr *rsrc, PMV_Request req);

MV_VOID io_chip_clear_int(pl_root *root);
MV_VOID io_chip_handle_int(pl_root *root);

MV_BOOLEAN io_chip_handle_cmpl_queue(pl_queue *queue);
MV_VOID io_chip_handle_attention(pl_queue *queue);

void prot_fill_sense_data(MV_Request *req, MV_U8 sense_key,
	MV_U8 ad_sense_code);

MV_VOID prot_clean_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req);

/* if return pointer if there is some kind of error */
err_info_record * prot_get_command_error_info(mv_command_table *cmd_table,
	 MV_PU32 cmpl_q);
#endif /* __CORE_PROTOCOL_H */
