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

#ifndef __CORE_INTERNAL_H
#define __CORE_INTERNAL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_sata.h"
#include "core_discover.h"
#include "core_hal.h"
#include "core_pm.h"
#ifdef SUPPORT_SES
#include "core_ses.h"
#endif

typedef struct _pl_queue pl_queue;
typedef struct _pl_root pl_root;
typedef struct _lib_device_mgr lib_device_mgr;
typedef struct _lib_resource_mgr lib_resource_mgr;
typedef struct _command_handler command_handler;

typedef struct _domain_phy domain_phy;
typedef struct _domain_port domain_port;
typedef struct _domain_device domain_device;
typedef struct _domain_expander domain_expander;
typedef struct _domain_pm domain_pm;
typedef struct _domain_enclosure domain_enclosure;

/*
 * Definition for supported count
 */
#define CORE_MAX_DEVICE_PER_PM                      5

#define CORE_MAX_DEVICE_SUPPORTED                   (MAX_DEVICE_SUPPORTED_PERFORMANCE + 1) /* Add 1 for virtual device */
#define CORE_MIN_EXPANDER_SUPPORTED                 2
#define CORE_MAX_EXPANDER_SUPPORTED                 MAX_EXPANDER_SUPPORTED
#define CORE_MAX_PM_SUPPORTED                       MAX_PM_SUPPORTED
#define CORE_MIN_ENC_SUPPORTED                      CORE_MIN_EXPANDER_SUPPORTED
#define CORE_MAX_ENC_SUPPORTED                      CORE_MAX_EXPANDER_SUPPORTED


/* this is the request sg entry for internal request in cached memory */
#define CORE_MIN_REQ_SG_ENTRY_COUNT                 8
#define CORE_MAX_REQ_SG_ENTRY_COUNT                 CORE_MIN_REQ_SG_ENTRY_COUNT

/* this is the sg entry for io chip access in uncached memory */
// HARRIET_TBD: increased from 128 to 132 for vanir workaround. 2 for end/tail 2 for workaround
#define CORE_MIN_HW_SG_ENTRY_COUNT                  (MAX_SG_ENTRY_REDUCED+2+2) //16+2+2
#define CORE_MAX_HW_SG_ENTRY_COUNT                  (MAX_SG_ENTRY+2) //130+2
#ifdef SUPPORT_SP2
#define CORE_HW_SG_ENTRY_SIZE                       sizeof(sgl_t)
#else
#define CORE_HW_SG_ENTRY_SIZE                       sizeof(prd_t)
#endif
/* dma sg buffer for io/xor, buffer count */
#define CORE_MIN_HW_SG_BUFFER_COUNT                 16
#define CORE_MAX_HW_SG_BUFFER_COUNT                 1152

/* perfect value can be MIN_SATA_BUFFER_COUNT + MIN_SES_BUFFER_COUNT + MIN_SMP_SCRATCH_COUNT */
/* We need at least two buffers for init especially for SATA(SAT) device.*/
#define CORE_MIN_SCRATCH_BUFFER_COUNT               2	

/* perfect value = CORE_MAX_DEVICE_SUPPORTED + MAX_SES_BUFFER_COUNT + CORE_MAX_EXPANDER_SUPPORTED (SATA + SES + SMP) */
#define CORE_MAX_SCRATCH_BUFFER_COUNT               CORE_MAX_DEVICE_SUPPORTED 

/* For some ATA HDD only supports 28 bit LBA DMA. This definition is only for them. */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define CORE_MAX_TRANSFER_SIZE                      MV_MAX_TRANSFER_SIZE
#else
#define CORE_MAX_TRANSFER_SIZE                      (128*1024)
#endif
/* initialize req + sub req for large req*/
#define MIN_INTERNAL_REQ_COUNT_FOR_LARGE_REQ        \
	(MV_U16)((MV_U32)(MV_MAX_TRANSFER_SIZE + CORE_MAX_TRANSFER_SIZE - 1) / CORE_MAX_TRANSFER_SIZE)
#define CORE_MIN_INTERNAL_REQ_COUNT                 MIN_INTERNAL_REQ_COUNT_FOR_LARGE_REQ
/* perfect value = CORE_MAX_DEVICE_SUPPORTED + (MV_MAX_TRANSFER_SIZE + CORE_MAX_TRANSFER_SIZE - 1) / CORE_MAX_TRANSFER_SIZE * (CORE_MAX_DEVICE_SUPPORTED) */

#ifdef SUPPORT_MUL_LUN
#define CORE_MAX_INTERNAL_REQ_COUNT                 \
	(MV_MAX(CORE_MAX_DEVICE_SUPPORTED, MIN_INTERNAL_REQ_COUNT_FOR_LARGE_REQ)*2)
#else
#define CORE_MAX_INTERNAL_REQ_COUNT                 \
	MV_MAX(CORE_MAX_DEVICE_SUPPORTED, MIN_INTERNAL_REQ_COUNT_FOR_LARGE_REQ)
#endif

/* hardware event count like hotplug */
#define CORE_MIN_HW_EVENT_COUNT                     10
#define CORE_MAX_HW_EVENT_COUNT                     20
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define XOR_MAX_SLOT_NUMBER                         256 /* per xor chip */
#else
#define XOR_MAX_SLOT_NUMBER                         64 /* per xor chip */
#endif
/* ID pool is shared by all devices */
#define MAX_ID                                      \
        (CORE_MAX_DEVICE_SUPPORTED + CORE_MAX_EXPANDER_SUPPORTED + \
         CORE_MAX_PM_SUPPORTED + CORE_MAX_ENC_SUPPORTED)

#define QNAP_TRIM_BUFFER_SIZE		0x800
#define SATA_SCRATCH_BUFFER_SIZE	sizeof(ata_identify_data)
#define SMP_SCRATCH_BUFFER_SIZE		sizeof(smp_request)
#define SES_SCRATCH_BUFFER_SIZE		0x416	//LSI 12G expander need 0x416 /* largest observed is 0x248 */
#define SCRATCH_BUFFER_SIZE		(MV_MAX(QNAP_TRIM_BUFFER_SIZE, MV_MAX(SATA_SCRATCH_BUFFER_SIZE, \
	MV_MAX(SMP_SCRATCH_BUFFER_SIZE, MV_MAX(SES_SCRATCH_BUFFER_SIZE,   \
	SMP_VIRTUAL_REQ_BUFFER_SIZE)))))

#define MAX_RX_FIS_NUMBER              MAX_REGISTER_SET_PER_IO_CHIP + 1
#define RX_FIS_SIZE                    256
#define UNASSOCIATED_FIS_POOL_SIZE     2048
#define MAX_RX_FIS_POOL_SIZE           (RX_FIS_SIZE*MAX_RX_FIS_NUMBER + UNASSOCIATED_FIS_POOL_SIZE)
#define MIN_RX_FIS_POOL_SIZE           (RX_FIS_SIZE*1 + UNASSOCIATED_FIS_POOL_SIZE)

/* max possible register set count between all products (odin, vanir, etc) 
 * current maximum is on Vanir, where each chip core has 64 sets, multiply
 * by two chip core, we have 128 
 */
#define MAX_REGISTER_SET				128

//#define SECTOR_SIZE				512	//TBD
/* SCSI IO EEDPFlags bits */
#if defined(SUPPORT_DIF) || defined(SUPPORT_DIX)
#define MV_EEDPFLAGS_INC_PRI_REFTAG        (0x8000)
#define MV_EEDPFLAGS_INC_SEC_REFTAG        (0x4000)
#define MV_EEDPFLAGS_INC_PRI_APPTAG        (0x2000)
#define MV_EEDPFLAGS_INC_SEC_APPTAG        (0x1000)

#define MV_EEDPFLAGS_CHECK_REFTAG          (0x0400)
#define MV_EEDPFLAGS_CHECK_APPTAG          (0x0200)
#define MV_EEDPFLAGS_CHECK_GUARD           (0x0100)

#define MV_EEDPFLAGS_PRD_DATA_INCL_T10          (0x0010)

/*zheng: change the operation value of eedpflags into bitwise control on
 * the corresponding four DIF control bit supplied by HW*/

#define MV_EEDPFLAGS_MASK_OP               (0x0007)
#define MV_EEDPFLAGS_NOOP_OP               (0x0000)
#define MV_EEDPFLAGS_CHECK_OP              (0x0001)
#define MV_EEDPFLAGS_STRIP_OP              (0x0002)
#define MV_EEDPFLAGS_REMOVE_OP              (0x0002)
#define MV_EEDPFLAGS_CHECK_REMOVE_OP       (0x0003)
#define MV_EEDPFLAGS_INSERT_OP             (0x0004)
#define MV_EEDPFLAGS_REPLACE_OP            (0x0008)
#define MV_EEDPFLAGS_CHECK_REGEN_OP        (0x0005)
#endif
enum mv_adapter_state	
{
	ADAPTER_INITIALIZING = 0,
	ADAPTER_READY,
	ADAPTER_FATAL_ERROR,
	ADAPTER_INIT_DONE
};
/*
enum command_handler_defs	
{
	HANDLER_SSP = 0,
	HANDLER_SATA,
	HANDLER_SATA_PORT,
	HANDLER_STP,
	HANDLER_PM,
	HANDLER_SMP,
	HANDLER_ENC,
	HANDLER_API,
	HANDLER_I2C,
	MAX_NUMBER_HANDLER
};
*/
typedef enum _MV_QUEUE_COMMAND_RESULT
{
	MV_QUEUE_COMMAND_RESULT_FINISHED = 0,
	MV_QUEUE_COMMAND_RESULT_FULL,
	MV_QUEUE_COMMAND_RESULT_NO_RESOURCE,
	MV_QUEUE_COMMAND_RESULT_SENT,
	MV_QUEUE_COMMAND_RESULT_PASSED,
        MV_QUEUE_COMMAND_RESULT_REPLACED,
        MV_QUEUE_COMMAND_RESULT_BLOCKED,
} MV_QUEUE_COMMAND_RESULT;

/*
 * domain_port related definition
 */
enum port_types {
	PORT_TYPE_SATA          = (1U << 0),
	PORT_TYPE_SAS           = (1U << 1),
	PORT_TYPE_PM            = (1U << 2),
	PORT_TYPE_I2C           = (1U << 3),
};

/* SATA use only */
#define PORT_SATA_STATE_POWER_ON		  0x01
#define PORT_SATA_STATE_SOFT_RESET_1	  0x02
#define PORT_SATA_STATE_SOFT_RESET_0	  0x03
#define PORT_SATA_STATE_WAIT_SIG		  0x04
#define PORT_SATA_STATE_SIG_DONE		  0x05

struct _command_handler 
{
	MV_BOOLEAN (*init_handler)(MV_PVOID dev_p);
	MV_U8 (*verify_command)(MV_PVOID root_p, MV_PVOID dev_p,
		MV_Request *req);
	MV_VOID (*prepare_command)(MV_PVOID root_p, MV_PVOID dev_p,
		MV_PVOID cmd_header_p, MV_PVOID cmd_table_p, 
		MV_Request *req);
	MV_VOID (*send_command) (MV_PVOID root_p, MV_PVOID dev_p,
		MV_PVOID struct_wrapper,
		MV_Request * req);
	MV_VOID (*process_command)(MV_PVOID root_p, MV_PVOID dev_p, 
		MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p, 
		MV_Request *req);

	MV_BOOLEAN (*error_handler)(MV_PVOID dev_p, MV_Request *req);
};

/* if add one domain base type, please handle both runtime error and init error
 * init error: core_handle_init_error 
 * runtime error: handler */
enum base_type {
	BASE_TYPE_DOMAIN_DEVICE = 0,
	BASE_TYPE_DOMAIN_PORT = 1,
	BASE_TYPE_DOMAIN_PM = 2,
	BASE_TYPE_DOMAIN_EXPANDER = 3,
	BASE_TYPE_DOMAIN_ENCLOSURE = 4,
	BASE_TYPE_DOMAIN_VIRTUAL = 5,
	BASE_TYPE_DOMAIN_I2C = 6,
};

enum base_state {
	BASE_STATE_NONE = 0,
	BASE_STATE_ERROR_HANDLING = 1,
	BASE_STATE_ERROR_HANDLED = 2,
};

struct _error_context
{
	MV_U16               timer_id; /* each device has a timer to check request timeout */
	MV_U16               eh_timer; /* timer used in error handling state machine */

	MV_U8                state; /* BASE_STATE_XXX */
	MV_U8                eh_type; /* EH_TYPE_XXX */
	MV_U8                eh_state; /* XXX_EH_XXX_STATE_XXX */
	MV_U8                scsi_status; /* save the eh req scsi status */

	MV_U8                pm_eh_error_port; /* port that generated PM error, pm error handling ONLY */
	MV_U8                pm_eh_active_port; /* port being reset during PM EH, pm error handling ONLY */
	MV_U8			interrupt_error; /* RE4020800h SAS/SATA Common Interrupt Cause */
	MV_U8               reserved;

	MV_U8                error_count; /* error requests number currently in the queue */
	MV_U8                timeout_count; /* number of times this device has timed out */
	MV_U16               retry_count; /* used in init state machine, init req will just retry */

	MV_Request          *error_req;
	List_Head            sent_req_list; /* requests sent to the device, in order */
#if defined(_OS_WINDOWS)
	KSPIN_LOCK		sent_req_list_SpinLock;
#elif defined(_OS_LINUX)
	spinlock_t		sent_req_list_SpinLock;
#endif
};

typedef struct _domain_base
{
	List_Head            queue_pointer;		/* primary queue pointer, usually for port's lists */
	List_Head            init_queue_pointer;	/* for init queue */
	List_Head            exp_queue_pointer;		/* for expander's lists */
	MV_U16               struct_size;
	MV_U8                type;
#ifdef SUPPORT_MUL_LUN
	MV_U8                multi_lun;
#else
	MV_U8                reserved;
#endif

	struct _command_handler *handler;

	MV_U16               id;
#ifdef SUPPORT_MUL_LUN
	MV_U16 			LUN;
	MV_U16 			TargetID;
#endif
	pl_root              *root;
	struct _domain_port  *port;

	MV_U16               queue_depth;
	MV_U16               outstanding_req;

	MV_BOOLEAN           blocked; 
	MV_BOOLEAN           cmd_issue_stopped;
#ifdef ATHENA_MISS_MSIX_INT_WA
	MV_U16               msix_timer_id;
	MV_U16               msix_reserved;
#endif
	/* error handling */
	struct _error_context err_ctx;
	struct _domain_base   *parent;
	pl_queue *queue;
#if defined(_OS_WINDOWS)
	KSPIN_LOCK base_SpinLock;
	KSPIN_LOCK outstanding_SpinLock;
	KSPIN_LOCK cmd_issue_stopped_SpinLock;
#elif defined(_OS_LINUX)
	spinlock_t base_SpinLock;
	spinlock_t outstanding_SpinLock;
	spinlock_t cmd_issue_stopped_SpinLock;
#endif
} domain_base;

struct _domain_port {
	struct _domain_base  base;

	MV_U8      state;              /* for sata only */
	MV_U8      type;               /* PORT_TYPE_XXX */
	MV_U8      setting;            /* PORT_SETTING_XXX */
	MV_U8      phy_num;

	MV_U16	   sata_sig_timer; 
	MV_U8      phy_map;             /* Software's phy map */
	MV_U8      asic_phy_map;        /* Actual phy map on ASIC */

	MV_U8      link_rate;
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
	MV_U8      curr_phy_map;        /* ASIC phy map currently in use*/
	MV_U8      reserved[2];
#else 
	MV_U8      reserved[3];
#endif

	domain_phy *phy; /* only points to the first phy, not queued, helpful for narrow port. */

	MV_U8    device_count;  
	MV_U8    expander_count;
	//MV_U8	   enc_count;

	MV_U16	 init_count;		/* how many devices on this port are currently 
								   in init queue */	

	MV_U32     dev_info;
	MV_U32     att_dev_info;
	MV_U64     dev_sas_addr;
	MV_U64     att_dev_sas_addr;

	List_Head   device_list;
	List_Head	expander_list;
	List_Head	current_tier;		/* for discovery */
	List_Head	next_tier;			/* for discovery */
	domain_pm   *pm;
	// Jira#QTS00000-2594 Debug
	MV_U16 qnap_i_counter;
	MV_U16 qnap_last_cmpl_q;
};

enum phy_state_type {
	PHY_STATE_NONE 			=  0, 
	PHY_STATE_PLUG_IN 		=(1U << 0), 
};

/*
 * domain_phy related definition
 */
struct _domain_phy
{
	domain_port *port;
        pl_root     *root;

	MV_U8       id;
	MV_U8       asic_id; /* use for phy id swap */
	MV_U8       type;
	MV_U8       link_err_cnt;

/* from PORT_CONFIG_ADDR0-3, PhyID/device protocol/sas_addr/SIG/wide_port */
	MV_U32      dev_info;
	MV_U32      att_dev_info;
	MV_U64      dev_sas_addr;
	MV_U64      att_dev_sas_addr;

/* from PORT_PHY_CONTROL0-3, REG_PORT_PHY_CONTROL, linkrate, current status */
	MV_U32      phy_status;

/* from PORT_IRQ_MASK0-3 */
	MV_U32      irq_status;
	MV_U32      sata_signature;

	MV_U32      phy_irq_mask;
#ifdef SUPPORT_DIRECT_SATA_SSU
	MV_BOOLEAN	is_spinup_hold;
	MV_U16		sata_ssu_timer;
	MV_U32          sata_ssu_time;
#endif
#ifdef SUPPORT_ACTIVE_CABLE
	MV_BOOLEAN is_cable_hotplug;
	/* Cable id [1-4] related to I2C [1-4] */
	MV_U8		cable_id;
#endif
#ifndef SUPPORT_PHY_POWER_MODE
	MV_U8 reserver[3];
#else
	MV_U16	PHY_power_mode_HIPM_timer;
	MV_U8	In_PHY_power_mode_HIPM;
#endif
	MV_U16	phy_decode_timer;
	MV_U8	phy_decode_err_cnt;
	MV_U8 	reserved;
};

/* domain device capability */
enum device_capability {
	DEVICE_CAPABILITY_48BIT_SUPPORTED                   = (1U << 0),
	DEVICE_CAPABILITY_SMART_SUPPORTED                   = (1U << 1),
	DEVICE_CAPABILITY_WRITECACHE_SUPPORTED              = (1U << 2),
	DEVICE_CAPABILITY_NCQ_SUPPORTED                     = (1U << 3),
	DEVICE_CAPABILITY_RATE_1_5G                         = (1U << 4),
	DEVICE_CAPABILITY_RATE_3G                           = (1U << 5),
	DEVICE_CAPABILITY_RATE_6G                           = (1U << 6),
	DEVICE_CAPABILITY_READLOGEXT_SUPPORTED              = (1U << 7),
	DEVICE_CAPABILITY_READ_LOOK_AHEAD_SUPPORTED         = (1U << 8),
	DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED         = (1U << 9),
	DEVICE_CAPABILITY_PROTECTION_INFORMATION_SUPPORTED  = (1U << 10),
	DEVICE_CAPABILITY_POIS_SUPPORTED = (1U << 11),
	DEVICE_CAPABILITY_TRIM_SUPPORTED  = (1U << 12),
	DEVICE_CAPABILITY_SSD  = (1U << 13),
	DEVICE_CAPABILITY_RATE_12G = (1U << 14),
	DEVICE_CAPABILITY_TRUST_SUPPORTED = (1U << 15),
	DEVICE_CAPABILITY_DIPM_SUPPORTED = (1U << 16),
	DEVICE_CAPABILITY_HIPM_SUPPORTED = (1U << 17),
};

enum device_state {
	/* Device initialization state */
	DEVICE_STATE_IDLE                          = 0x0,
	//DEVICE_STATE_PM_ENABLE_1                   = 0x1,
	//DEVICE_STATE_PM_ENABLE_0                   = 0x2,
	DEVICE_STATE_SRST_1                        = 0x3,
	DEVICE_STATE_SRST_0                        = 0x4,
	DEVICE_STATE_RESET_DONE                    = 0x5,
	// SATA device states
	DEVICE_STATE_IDENTIFY_DONE                 = 0x6,
	DEVICE_STATE_SET_PIO_DONE                  = 0x7,
	DEVICE_STATE_SET_UDMA_DONE                 = 0x8,
	DEVICE_STATE_ENABLE_WRITE_CACHE_DONE       = 0x9,
	DEVICE_STATE_ENABLE_READ_AHEAD_DONE        = 0xA,
	DEVICE_STATE_SPIN_UP_DONE                  = 0x25,
	DEVICE_STATE_ENABLE_DIPM_DONE                  = 0x26,
	/* SAS device states */
	DEVICE_STATE_INQUIRY_DONE                  = 0xB,
	DEVICE_STATE_INQUIRY_EVPD_DONE             = 0xD,
	DEVICE_STATE_REPORT_LUN		       =0xE,
	DEVICE_STATE_READ_CAPACITY_DONE            = 0xF,
	DEVICE_STATE_READ_CAPACITY16_DONE          = 0x29,
	DEVICE_STATE_SET_READ_AHEAD_DONE           = 0x10,
	DEVICE_STATE_SET_CACHE_ENABLE_DONE         = 0x11,
	DEVICE_STATE_LOG_SENSE_DONE                = 0x12,
	// Device states
	DEVICE_STATE_FORMAT_DONE                   = 0x13,
	DEVICE_STATE_FORMAT_WRITE                  = 0x14,
	DEVICE_STATE_FORMAT_VERIFY                 = 0x15,
	DEVICE_STATE_PM_SOFTRESET                  = 0x16,
	DEVICE_STATE_STARTSTOP_DONE                = 0x17,
	DEVICE_STATE_STP_REPORT_PHY				   = 0x18,
	DEVICE_STATE_STP_RESET_PHY				   = 0x19,
	DEVICE_STATE_TEST_UNIT_READY_DONE		   = 0x20,
	DEVICE_STATE_CHECK_DUPLICATE_DONE          = 0x21,
	DEVICE_STATE_RESMUE_DONE                   = 0x22,
	DEVICE_STATE_STP_CLEAN_AFFILIATION	   	   = 0x23,
	DEVICE_STATE_SKIP_INIT_DONE			= 0x24,
	DEVICE_STATE_READ_POWER_MODE_DONE			= 0x27,
	DEVICE_STATE_CHECK_POWER_MODE_DONE			= 0x28,
	DEVICE_STATE_DUPLICATE_DEVICE			    = 0x30,
	DEVICE_STATE_INIT_DONE                     = 0xFF,
};

enum device_setting {
	DEVICE_SETTING_SMART_ENABLED               = (1U << 0),
	DEVICE_SETTING_WRITECACHE_ENABLED          = (1U << 1),
	DEVICE_SETTING_PI_ENABLED                  = (1U << 2),
	DEVICE_SETTING_READ_LOOK_AHEAD             = (1U << 3),
	DEVICE_SETTING_NCQ_RUNNING				   = (1U << 4),
	DEVICE_SETTING_POIS_ENABLED				   = (1U << 5),
	DEVICE_SETTING_DIPM_ENABLED				   = (1U << 6),
	DEVICE_SETTING_POWERMODE_ENABLED		= (1U << 7),
	DEVICE_SETTING_STOP_RUN_NCQ = (1U << 8),
};

enum device_status {
	DEVICE_STATUS_NO_DEVICE					   = (1U << 0),
	DEVICE_STATUS_FUNCTIONAL						= (1U << 1),
	DEVICE_STATUS_SECONDARY_PATH				   = (1U << 2),
	DEVICE_STATUS_SPINNED_DOWN             	= (1U << 3),		/* ikkwong: TBD */
       DEVICE_STATUS_NEED_RESET                 = (1U << 4),         /* need to hard reset */
       DEVICE_STATUS_NEED_CLEAR                	= (1U << 5),       /* need to clear aff */  
       DEVICE_STATUS_WAIT_ASYNC                	= (1U << 6),      /* need to async sdb fis */   
	DEVICE_STATUS_FROZEN		   	= (1U << 7), 	
#ifdef SUPPORT_STAGGERED_SPIN_UP
	DEVICE_STATUS_SPIN_UP		   	= (1U << 8), 
#endif
#if defined(DEVICE_SLEEP_SUPPORT)
	DEVICE_STATUS_SLEEPING		   	= (1U << 9),
	DEVICE_STATUS_HANDLE_SLEEP	   	= (1U << 10), 
#endif
	DEVICE_STATUS_INTERNAL_HOTPLUG		= (1U << 11),
};

/* domain device capability */
enum ssd_capability {
	SSD_CAPABILITY_TRIM_ANCHORED_LBA  = (1U << 0),
	SSD_CAPABILITY_TRIM_READ_ZERO  = (1U << 1),
};
struct _domain_device
{
	struct _domain_base  base;

	MV_U8                connection;
	MV_U8                dev_type;	
	MV_U8                state;            /* DEVICE_STATE_XXX */

	MV_U8					queue_type;
	MV_U32					status; /*DEVICE_STATUS_XXX*/

	MV_U16				 route_index;
	MV_U8                negotiated_link_rate;
	MV_U8				 ssd_capability;

	MV_U64               sas_addr;

	struct _domain_pm	 *pm;

	MV_U32                capability; 
	MV_U16                setting; /* The supported features are enabled or not. */
	MV_U16                reserved7;
	//JING TBD: double check whether we need that. Or union
	//from inquiry
	MV_U8                 serial_number[20];
	MV_U8                 model_number[40];
	MV_U8                 firmware_revision[8];

	MV_U64                WWN;
        MV_U32                max_transfer_size; /* ikkwong: TBD */
	//from read capacity
	MV_U32                sector_size;
	MV_U64                max_lba;

	MV_U8                 sata_sig_timer;
	MV_U8                 pm_port;
	MV_U8				  parent_phy_id;	/* only used if dev is behind expander */


	MV_U8				  need_config_route:1;
	MV_U8				  reserved1:7;
	MV_U32			logical_sector_size;

	MV_U8                 pio_mode;
	MV_U8                 udma_mode;
	MV_U8                 current_pio;
	MV_U8                 current_udma;

	MV_U32                signature; /* HP wants this information */
	MV_U32                ncq_tags;
	MV_U8                 next_ncq; /* when search free ncq, start from this index */
	MV_U8                 register_set;
	MV_U8			phy_change_count;/* only used if dev is behind expander */
#ifdef CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
	MV_U8                 curr_phy_map;
#else 
	MV_U8                 reserved2;
#endif
        MV_U64                total_formatted_lba_count;
	MV_U16				queue_id;	/* multi queue id */	

#ifdef SUPPORT_SGPIO
	MV_U8 sgpio_drive_number;
	MV_U8 sgpio_act_led_status;
	MV_U8 sgpio_locate_led_status;
	MV_U8 sgpio_error_led_status;
#ifdef SUPPORT_SGPIO_ACTIVE_LED
	MV_U16 active_led_off_timer;
#else
	MV_U16 reserved4;
#endif
	MV_U16 reserved5;
#endif

#ifdef SUPPORT_SES
	MV_U8 ses_overall_element_index; /* include type overall */
	MV_U8 ses_element_index;   /* used only if a SES element, not include overall */
	MV_U8 ses_slot_number;
	MV_U8 ses_element_type;

	domain_enclosure *enclosure;		/* Points to the associated enclosure */
	/* could be used as control or status, type can be determined by SesElementType (device or array) */
	MV_U8 ses_request_flag;
	MV_U8 reserved3[3];
#endif
	List_Head	 injected_error_list;      /* list of injected errors on this drive */
#ifdef ATHENA_PERFORMANCE_TUNNING
#if defined(_OS_WINDOWS)
	KSPIN_LOCK		ncq_tags_spin_lock;
	LIST_ENTRY resource_queue;
	KSPIN_LOCK resource_lock;
#elif defined(_OS_LINUX)
	spinlock_t	ncq_tags_spin_lock;
	spinlock_t	resource_lock;
#endif
#endif
};

enum expander_state {
	/* expander discovery states */
	EXP_STATE_IDLE					= 0x0,
	EXP_STATE_REPORT_GENERAL		= 0x1,
	EXP_STATE_REPORT_MANU_INFO		= 0x2,
	EXP_STATE_DISCOVER				= 0x3,
	EXP_STATE_CONFIG_ROUTE			= 0x4,
	EXP_STATE_NEXT_TIER				= 0x5,
	/* broadcast/hot plug states */
	EXP_STATE_CHECK_EXP_CHANGE		= 0x6,
	EXP_STATE_CHECK_PHY_CHANGE		= 0x7,
	EXP_STATE_HOT_PLUG_NEXT_TIER	= 0x8,
	EXP_STATE_RESET_SATA_PHY		= 0x9,
	EXP_STATE_RESET_WAIT			= 0xA,
	EXP_STATE_CLEAR_AFFILIATION		= 0xB,
	EXP_STATE_RESET_WAIT_2			= 0xC,
	/* other */
	EXP_STATE_ABORT					= 0xfe,
	EXP_STATE_DONE					= 0xff,
};

/* HARRIET: do we still need EXP_STATUS_NO_DEVICE? */
enum exp_status {
	EXP_STATUS_NO_DEVICE		= (1U << 0),
	EXP_STATUS_EXISTING			= (1U << 1),
	EXP_STATUS_FUNCTIONAL		= (1U << 2),
	EXP_STATUS_HOT_PLUG			= (1U << 3),
};

struct _domain_expander 
{
	struct _domain_base  base;

	List_Head device_list;
	List_Head expander_list;
	List_Head enclosure_queue_pointer;

	MV_U64	sas_addr;

	MV_U8	state;
	MV_U8	status;
	MV_U8   parent_phy_count;      /* # of phys used to connect this expander 
								    * to its parent */
	MV_U8	phy_count;			   /* # of phys on this expander */

	MV_U8   parent_phy_id[MAX_WIDEPORT_PHYS];
	MV_U16	route_table[MAXIMUM_EXPANDER_PHYS];

	MV_U8	device_count;		   /* number of child devices */
	MV_U8	expander_count;		   /* number of child expanders */
	MV_U8	neg_link_rate;
	MV_U8	register_set;
	MV_U8	vendor_id[8];
	MV_U8	product_id[16];
	MV_U8	product_rev[4];
	MV_U8	component_vendor_id[8];

	MV_U16	component_id;
	MV_U8	component_rev_id;
	MV_U8	configurable_route_table:1;
	MV_U8	configures_others:1;
	MV_U8	need_report_plugin:1;	/* do we need to generate an event for plug in */
	MV_U8	has_been_setdown:1;
	MV_U8	has_been_reset:1;
	MV_U8	reserved1:3;

	MV_U16	timer_tag;
	MV_U8	reserved2[2];

	domain_enclosure *enclosure;	/* which enclosure this expander is associated with */
};


/* HARRIET: do we still need PM_STATUS_NO_DEVICE? */
enum pm_status {
	PM_STATUS_NO_DEVICE			= (1U << 0),
	PM_STATUS_EXISTING			= (1U << 1),
	PM_STATUS_FUNCTIONAL		= (1U << 2),
	PM_STATUS_HOT_PLUG			= (1U << 3),
	PM_STATUS_DEVICE_PHY_RESET	= (1U << 6),
};

enum pm_state {
	/* init states */
	PM_STATE_RESET_DONE				= 0x0,
	PM_STATE_READ_INFO_DONE			= 0x1,
	PM_STATE_READ_ID_DONE			= 0x2,
	PM_STATE_READ_REV_DONE			= 0x3,
	PM_STATE_ENABLE_FEATURES		= 0x4,
	PM_STATE_CLEAR_ERROR_INFO		= 0x5,
	PM_STATE_SPIN_UP_ALL_PORTS		= 0x6,
	PM_STATE_SPIN_UP_DONE			= 0x7,
	PM_STATE_ENABLE_PM_PORT_1		= 0x8,
	PM_STATE_ENABLE_PM_PORT_0		= 0x9,
	PM_STATE_PORT_CHECK_PHY_RDY		= 0xa,
	PM_STATE_CLEAR_X_BIT			= 0xb,
	PM_STATE_ISSUE_SOFT_RESET_1		= 0xc,
	PM_STATE_ISSUE_SOFT_RESET_0		= 0xd,
	PM_STATE_WAIT_SIG				= 0xe,
	PM_STATE_SIG_DONE				= 0xf,

	/* hot plug states */
	PM_STATE_READ_GLOBAL_ERROR		= 0x10,
	PM_STATE_READ_PORT_ERROR		= 0x11,
	PM_STATE_CLEAR_PORT_ERROR		= 0x12,
	PM_STATE_WAIT_FOR_READY		    = 0x13,
	PM_STATE_SET_PMPORT			    = 0x14,
	PM_STATE_QRIGA_WORKAROUND	    = 0x15,
	PM_STATE_DISABLE_ASYNOTIFY		= 0x16,
	PM_STATE_ENABLE_ASYNOTIFY		= 0x17,
	PM_STATE_AKUPA_WORKAROUND	    = 0x18,
	PM_STATE_DONE					= 0xff,
};

struct _domain_pm 
{
	struct _domain_base  base;

	domain_device *devices[CORE_MAX_DEVICE_PER_PM];
	MV_U32	pm_port_sig[CORE_MAX_DEVICE_PER_PM];
	
	MV_U8	register_set;
	MV_U16	sata_sig_timer;
	MV_U8	srst_retry_cnt;

	MV_U8	state;            /* PM_STATE_XXX */
	MV_U8	status;           /* PM_STATUS_XXX */
	MV_U8	num_ports;
	MV_U8	active_port;

	MV_U16	vendor_id;
	MV_U16	device_id;

	MV_U8	product_rev;
	MV_U8	spec_rev;		  /* 10 means 1.0, 11 means 1.1 */
	MV_U8	feature_enabled;
	MV_U8	retry_cnt;

	MV_U32	sstatus;
	MV_U32	global_serror;

	MV_U8	autoload_index;
	MV_U8	pm_index;
	MV_U8	reserved[2];
};

enum i2c_state{
	/* i2c device init state*/
	I2C_STATE_RESET_DONE		= 0x1,
	I2C_STATE_IDENTIFY_DONE = 0x2,
	I2C_STATE_INQUIRY_DONE= 0x3,
	I2C_STATE_INIT_DONE = 0xff,
};

enum enclosure_state{
	/*enclosure state*/
	ENCLOSURE_INQUIRY_DONE = 0x10,
	ENCLOSURE_CHECK_SUPPORT_PAGE_DONE = 0x11,
	ENCLOSURE_GET_DEVICE_ELEMENT_DONE =0x12,
	ENCLOSURE_GET_CONFIGUATION_DONE = 0x13,
	ENCLOSURE_GET_ELEMENT_DISCRIPTER_DONE =0x14,
	ENCLOSURE_SKIP_INIT_DONE = 0x15,
	ENCLOSURE_INIT_DONE = 0xFF,
};

enum enclosure_status{
	ENCLOSURE_STATUS_NO_DEVICE		= (1U << 0),
	ENCLOSURE_STATUS_EXISTING		= (1U << 1),
	ENCLOSURE_STATUS_FUNCTIONAL		= (1U << 2),	
};

enum _enclosure_flag {
	ENC_FLAG_NEED_REINIT			= (1U << 0),
	ENC_FLAG_FIRST_INIT			= (1U << 1),
};
struct _domain_enclosure 
{
	struct _domain_base  base;
	
	MV_U8   state;            /* PM_STATE_XXX */
	MV_U8   status;           /* PM_STATUS_XXX */
	MV_U8   negotiated_link_rate;
	MV_U8   register_set;

	MV_U16                setting; /* The supported features are enabled or not. */
	MV_U8			parent_phy_id;	/* only used if dev is behind expander */
	MV_U8			enc_flag;     /*when new PD plugged in enclosure need re-init*/
	
	MV_U64               sas_addr;
              
	List_Head expander_list;
	
	MV_U8 vendor_id[8];
	MV_U8 product_id[16];
	MV_U8 product_revision[4];
	MV_U8 enclosure_logical_id[8];
	
	MV_U8 supported_page_count;
	MV_U8 reserved1[3];
	MV_U8 supported_page[32];
      
};

enum pl_events {
	PL_EVENT_PHY_CHANGE = 0,
	PL_EVENT_PHY_DEC_ERROR = 1,
	PL_EVENT_ASYNC_NOTIFY = 2,
	PL_EVENT_BROADCAST_CHANGE = 3,
#ifdef SUPPORT_ACTIVE_CABLE
	PL_EVENT_CABLE_HOTPLUG = 4,
#endif
	PL_EVENT_SDB_ASYNC_NOTIFY = 0xFE00FE,
};

void pal_notify_event(pl_root *root, MV_U8 phy_id, MV_U8 event);

#endif /* __CORE_INTERNAL_H */
