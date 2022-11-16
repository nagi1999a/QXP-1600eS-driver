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

#ifndef __MV_COM_TYPE_H__
#define __MV_COM_TYPE_H__

#include "com_define.h"
#include "com_list.h"

#include "mv_config.h"	// USE_NEW_SGTABLE is defined in mv_config.h

/*
 * Data Structure
 */
#ifndef SUPPORT_ROC
#ifdef _OS_BIOS
#define MAX_CDB_SIZE                            16
#else
#define MAX_CDB_SIZE                            32
#endif
#else
#define MAX_CDB_SIZE							16
#endif

struct _MV_Request;
typedef struct _MV_Request MV_Request, *PMV_Request;

#if defined(SOFTWARE_XOR) || defined(HARDWARE_XOR)
typedef struct _MV_XOR_Request MV_XOR_Request, *PMV_XOR_Request;
#endif

#define REQ_STATUS_SUCCESS                      0x0
#define REQ_STATUS_NOT_READY                    0x1
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_BUSY                         0x3
#define REQ_STATUS_INVALID_REQUEST              0x4
#define REQ_STATUS_INVALID_PARAMETER            0x5
#define REQ_STATUS_NO_DEVICE                    0x6
/* Sense data structure is the SCSI "Fixed format sense datat" format. */
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8
#define REQ_STATUS_DPP_ERROR                    0x9
#define REQ_STATUS_ERROR_WITH_SENSE             0x10
#define REQ_STATUS_TIMEOUT                      0x11
#define REQ_STATUS_DIF_GUARD_ERROR		0x12
#define REQ_STATUS_DIF_REF_TAG_ERROR		0x13
#define REQ_STATUS_DIF_APP_TAG_ERROR		0x14
#define REQ_STATUS_FROZEN                       0x15

/* Request initiator must set the status to REQ_STATUS_PENDING. */
#define REQ_STATUS_PENDING                      0x80
#ifndef NEW_CORE_DRIVER
#define REQ_STATUS_RETRY                        0x81
#define REQ_STATUS_REQUEST_SENSE                0x82
#endif
#define REQ_STATUS_TIME_OUT										 0x83	/* Hardware no trigger interrupt, do not retry */

#ifdef MAGNI_BIOS
/* HW SG Table and SG Entry */
typedef struct _MV_HW_SG_Entry
{
	MV_U32 Base_Address;
	MV_U32 Base_Address_High;
	MV_U32 Reserved0;
	MV_U32 Size;
}MV_HW_SG_Entry, *pMV_HW_SG_Entry;
#endif

/*
 * Don't change the order here.
 * Module_StartAll will start from big id to small id.
 * Make sure module_set setting matches the Module_Id
 * MODULE_HBA must be the first one. Refer to Module_AssignModuleExtension.
 * And HBA_GetNextModuleSendFunction has an assumption that the next level
 * has larger ID.
 */
enum Module_Id
{
        MODULE_HBA = 0,
#ifdef CACHE_MODULE_SUPPORT
        MODULE_CACHE,
#endif /*  CACHE_MODULE_SUPPORT */

#ifdef SAMPLE_MODULE_SUPPORT
        MODULE_SAMPLE,
#endif
#ifdef RAID_DRIVER
        MODULE_RAID,
#endif /*  RAID_DRIVER */
        MODULE_CORE,
        MAX_MODULE_NUMBER
};
#define MAX_POSSIBLE_MODULE_NUMBER              MAX_MODULE_NUMBER

#ifdef USE_NEW_SGTABLE
#include "com_sgd.h"

typedef sgd_tbl_t MV_SG_Table, *PMV_SG_Table;
typedef sgd_t MV_SG_Entry, *PMV_SG_Entry;

#else

struct _MV_SG_Table;
typedef struct _MV_SG_Table MV_SG_Table, *PMV_SG_Table;

struct _MV_SG_Entry;
typedef struct _MV_SG_Entry MV_SG_Entry, *PMV_SG_Entry;



/* SG Table and SG Entry */
struct _MV_SG_Entry
{
	MV_U32 Base_Address;
	MV_U32 Base_Address_High;
#if defined(SUPPORT_BALDUR)
	MV_U32 Size : 22;
	MV_U32 Reserved0 : 2;
	MV_U32 Chain : 1;
	MV_U32 Reserved1 : 3;
	MV_U32 ifsel:4;
#else
	MV_U32 Reserved0;
	MV_U32 Size;
#endif
};


#ifndef _OS_BIOS
struct _MV_SG_Table
{
	MV_U16 Max_Entry_Count;
	MV_U16 Valid_Entry_Count;
	MV_U16 Flag;
	MV_U16 Occupy_Entry_Count;
	MV_U32 Byte_Count;
	PMV_SG_Entry Entry_Ptr;
};
#else
struct _MV_SG_Table
{
	MV_U8 Valid_Entry_Count;
	MV_U32 Byte_Count;
	MV_SG_Entry Entry_Ptr[MAX_SG_ENTRY];
};
#endif


#endif

#ifdef SIMULATOR
#define	MV_REQ_COMMON_SCSI	          0
#define	MV_REQ_COMMON_XOR	          1
#endif	/*SIMULATOR*/

/*
 * MV_Request is the general request type passed through different modules.
 * Must be 64 bit aligned.
 */

/* TBD : REMOVE IT A.S.A.P. */
#define DEV_ID_TO_TARGET_ID(_dev_id)    ((MV_U8)((_dev_id) & 0x00FF))
#define DEV_ID_TO_LUN(_dev_id)                ((MV_U8) (((_dev_id) & 0xFF00) >> 8))
#define __MAKE_DEV_ID(_target_id, _lun)   (((MV_U16)(_target_id)) | (((MV_U16)(_lun)) << 8))

typedef void (*MV_ReqCompletion)(MV_PVOID,PMV_Request);

struct _MV_Request {
#ifdef SIMULATOR
	MV_U32 CommonType;	// please keep it as the first field
#endif	/* SIMULATOR */
	List_Head pool_entry;     /* don't bother, res_mgmt use only */
	List_Head Queue_Pointer;
#ifndef NEW_CORE_DRIVER
	List_Head Complete_Queue_Pointer;
#endif

#ifdef DEBUG_TIMEOUT_REQUEST
	List_Head HBA_Queue_Pointer;
#endif

#ifdef PCI_POOL_SUPPORT
	MV_PVOID cmd_table;
	MV_PVOID sg_table;
	MV_PHYSICAL_ADDR cmd_table_phy;
	MV_PHYSICAL_ADDR sg_table_phy;
#endif

	MV_PVOID Virtual_Buffer;
	MV_U16 Device_Id;

	MV_U16 Req_Flag;                  /* Check the REQ_FLAG definition */
	MV_U8 Scsi_Status;
	MV_U8 Tag;                        /* Request tag */

	MV_U16 Req_Flag_Ext;
#ifdef NEW_CORE_DRIVER
	MV_U16 Req_Type;
#else
	MV_U8 Req_Type;                   /* Check the REQ_TYPE definition */
#ifdef _OS_BIOS
	MV_U8		OpCode;
#else
	MV_U8 Reserved0[1];
#endif
#endif
#ifdef ATHENA_SECTOR_BY_SECTOR
        MV_U32 sector_offset;
#endif
	MV_PVOID Cmd_Initiator;           /* Which module(extension pointer)
					     creates this request. */					   
#ifdef SCSI_ID_MAP
	MV_U8 pass_id; /*pass_id == 1, do not need translate device id*/
	MV_U8 Reserved5[3];
#endif
#ifdef RAID_ERROR_HANDLING
	MV_U16 PD_Index_In_Raid;
#else
	MV_U16 Reserved1;
#endif		/* RAID_ERROR_HANDLING */

#ifdef SUPPORT_HYPERIO
	MV_U32	rc_cmd_idx;
	MV_U32	rc_sgd_idx;
	MV_U32	subcmd_offset;
	MV_U32	subcmd_cnt;
#endif

	MV_U8 Sense_Info_Buffer_Length;
	MV_U8 NCQ_Tag;

	MV_U32 Data_Transfer_Length;
#ifdef SUPPORT_MUL_LUN
	MV_U8 lun[8];
#endif
#if defined(SUPPORT_DIF) || defined(SUPPORT_DIX)
	MV_U16		EEDPFlags;
    	//MV_U32		EEDPBlockSize;
    	//MV_U32		SecondaryReferenceTag;
    	//MV_U16		SecondaryApplicationTag;
    	//MV_U16		ApplicationTagTranslationMask;

    	//MV_U32		PrimaryReferenceTag;        /* 0x14 */
    	//MV_U16		PrimaryApplicationTag;      /* 0x18 */
    	//MV_U16		PrimaryApplicationTagMask;  /* 0x1A */
    	//MV_U32		TransferLength;             /* 0x1C */
#endif
	MV_U8 Cdb[MAX_CDB_SIZE];
	MV_PVOID Data_Buffer;
	MV_PVOID Sense_Info_Buffer;

	MV_SG_Table SG_Table;

	MV_PVOID Org_Req;                /* The original request. */

	/* Each module should only use Context to store module information. */
	MV_PVOID Context[MAX_POSSIBLE_MODULE_NUMBER];

	MV_PVOID Scratch_Buffer;          /* pointer to the scratch buffer
										 that this request used */
#ifndef NEW_CORE_DRIVER
	MV_PVOID SG_Buffer;
#endif
	MV_PVOID pRaid_Request;

	MV_LBA LBA;
	MV_U32 Sector_Count;
	MV_U32 Cmd_Flag;

	MV_U32 Time_Out;                  /* how many seconds we should wait
					     before treating request as
					     timed-out */
	MV_U32 Splited_Count;
#ifdef _OS_BIOS
	MV_U16   SlotNo;
	MV_U16		Features;
	MV_U32		BufferAddress;
#endif

#ifdef RAID_ERROR_HANDLING
	/* This data structure is defined to roll-over request or split request.
	 * It's a generic roll-over scheme.
	 * Can apply to each module or level. */
	MV_PVOID	Roll_CTX;
#endif

#ifdef SUPPORT_TRANSACTION_LOG
	MV_U16 Log_Entry;
	MV_U8   reserved4[2];
#endif

#ifdef _OS_LINUX
 	MV_PVOID Org_Req_Scmd;                /* The original scmd request from OS*/
#endif /* _OS_LINUX */
	MV_ReqCompletion	Completion; /* call back function */

#ifdef HAVE_PRD_SKIP
	/* For PRD SKIP */
	MV_U16 init_block_xfer;
	MV_U16 init_block_skip;
	MV_U16 sub_block_xfer;
	MV_U16 sub_block_skip;
#endif
#ifdef HAVE_HW_COMPLIANT_SG
	MV_PHYSICAL_ADDR       bus_addr;
#endif

#ifdef SPLIT_RCC_CMD
    MV_U16 target_sent_rcc_cmd_num;
    MV_U16 target_returned_rcc_cmd_num;
    MV_U16 reserved[2];
#endif

#if defined(SUPPORT_MAGNI)
    MV_PVOID pcmd_header;
#endif
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
    MV_PVOID base_p;
#endif
    MV_U16 hard_reset_cnt;
};

#define MV_REQUEST_SIZE                   sizeof(MV_Request)
#define SIMPLE_REQUEST_NUMBER		8192
#define RCC_DATABUF_SIZE				528
#define RCC_PAYLOAD_SIZE				2016
#define OFFSET_SUB_CMD				1008
#define RCC_PACKET_CMD_OFFSET           992
#define PACKET_CMD_DATA_SIZE            PACKET_HEADER_SIZE + MAX_RCC_CMD_COUNT * 8;//272B

/*
 * Request flag is the flag for the MV_Request data structure.
 */
#define REQ_FLAG_LBA_VALID                MV_BIT(0)
#define REQ_FLAG_CMD_FLAG_VALID           MV_BIT(1)
#define REQ_FLAG_RETRY                    MV_BIT(2)
#define REQ_FLAG_INTERNAL_SG              MV_BIT(3)
#ifndef USE_NEW_SGTABLE
#define REQ_FLAG_USE_PHYSICAL_SG          MV_BIT(4)
#define REQ_FLAG_USE_LOGICAL_SG           MV_BIT(5)
#else
/*temporarily reserve bit 4 & 5 until NEW_SG is enabled fixedly for all OS */
#endif
#define REQ_FLAG_FLUSH                    MV_BIT(6)
#define REQ_FLAG_ALIGN				MV_BIT(7)
#define REQ_FLAG_CONSOLIDATE			  MV_BIT(8)
#define REQ_FLAG_NO_CONSOLIDATE           MV_BIT(9)
#define REQ_FLAG_EXTERNAL				  MV_BIT(10)
#if !defined(NEW_CORE_DRIVER) || defined(SIMULATOR)
#define REQ_FLAG_CORE_SUB                 MV_BIT(11)
#endif
#define REQ_FLAG_BYPASS_HYBRID            MV_BIT(12)	// hybrid disk simulation
#define REQ_FLAG_CONTINUE_ON_ERROR        MV_BIT(13)	/* Continue to handle the request even hit error. */
#define REQ_FLAG_NO_ERROR_RECORD          MV_BIT(14)	/* Needn't record error. */
#define REQ_FLAG_TRIM_CMD          MV_BIT(15)	/* Trim support by verify. */


#define REQ_FLAG_EXT_REQ_COUNTED                MV_BIT(0)
/*
 * Request Type is the type of MV_Request.
 */
enum {
	/* use a value other than 0, and now they're bit-mapped */
	REQ_TYPE_OS       = 0x01,
	REQ_TYPE_RAID     = 0x02,
	REQ_TYPE_CACHE    = 0x04,
	REQ_TYPE_INTERNAL = 0x08,
	REQ_TYPE_SUBLD    = 0x10,
	REQ_TYPE_SUBBGA   = 0x20,
	REQ_TYPE_MP       = 0x40,
	REQ_TYPE_DS		  = 0x80,
	REQ_TYPE_CORE     = 0x100,
	REQ_TYPE_COPYBACK_CLEAR	= 0x200,
#ifdef RAID_IO_DEBUG
	REQ_TYPE_IO_COUNTED	= 0x400,
#endif
#ifdef SUPPORT_MODULE_CONSOLIDATE
	REQ_TYPE_CC = 0x800,
	REQ_TYPE_GO_THRU_CC = 0x1000,
#endif
};

#define CMD_FLAG_NON_DATA                 MV_BIT(0)  /* 1-non data;
							0-data command */
#define CMD_FLAG_DMA                      MV_BIT(1)  /* 1-DMA */
#define CMD_FLAG_PIO			  MV_BIT(2)  /* 1-PIO */
#define CMD_FLAG_DATA_IN                  MV_BIT(3)  /* 1-host read data */
#define CMD_FLAG_DATA_OUT                 MV_BIT(4)	 /* 1-host write data */

#define CMD_FLAG_SMART                    MV_BIT(5)  /* 1-SMART command; 0-non SMART command*/
#define CMD_FLAG_SMART_ATA_12       MV_BIT(6)  /* SMART ATA_12  */
#define CMD_FLAG_SMART_ATA_16       MV_BIT(7)  /* SMART ATA_16; */
#define CMD_FLAG_CACHE_OS_DATABUF	 MV_BIT(9)


#define CMD_FLAG_CACHE_OS_DATABUF	  MV_BIT(9)
/*
 * The last 16 bit only can be set by the target. Only core driver knows
 * the device characteristic.
 */
#define CMD_FLAG_NCQ                      MV_BIT(16)
#define CMD_FLAG_TCQ                      MV_BIT(17)
#define CMD_FLAG_48BIT                    MV_BIT(18)
#define CMD_FLAG_PACKET                   MV_BIT(19)  /* ATAPI packet cmd */

#define CMD_FLAG_SCSI_PASS_THRU           MV_BIT(20)
#define CMD_FLAG_ATA_PASS_THRU            MV_BIT(21)

#define CMD_FLAG_SOFT_RESET               MV_BIT(22)
#define CMD_FLAG_PRD_SKIP                 MV_BIT(23)
#define CMD_FLAG_SECURITY_KEY             MV_BIT(24)
#define CMD_FLAG_S_KEK_POSITION           MV_BIT(25)
#define CMD_FLAG_VERIFY_KEY_TAG           MV_BIT(26)
#define CMD_FLAG_PIR                      MV_BIT(27)
#ifdef ATHENA_Z1_PM_WA
#define CMD_FLAG_CMD_CMPL                      MV_BIT(28)
#endif
#ifdef SUPPORT_INEJECT_ERROR
#define CMD_FLAG_INJECT_ERR                     MV_BIT(29)
#endif
#if defined(HARDWARE_XOR) || defined(SOFTWARE_XOR)
/* XOR request types */
#define    XOR_REQUEST_WRITE              0
#define    XOR_REQUEST_COMPARE            1
#define    XOR_REQUEST_DMA                2
#define    XOR_REQUEST_MEMSET             3

/* XOR request status */
#define XOR_STATUS_SUCCESS                0
#define XOR_STATUS_INVALID_REQUEST        1
#define XOR_STATUS_ERROR                  2
#define XOR_STATUS_INVALID_PARAMETER      3

#ifdef SUPPORT_LARGE_CONFIG
#define XOR_SOURCE_SG_COUNT               36  /* TBD support 32 disks RAID5 */
#else
#define XOR_SOURCE_SG_COUNT               11  /* TBD support 8 disks RAID5 */
#endif		/* SUPPORT_LARGE_CONFIG */

#ifdef RAID6_MULTIPLE_PARITY
#   define XOR_TARGET_SG_COUNT               3   /* TBD */
#else
#   define XOR_TARGET_SG_COUNT               1   /* TBD */
#endif

typedef MV_U8    XOR_COEF, *PXOR_COEF;        /* XOR Coefficient */

struct _MV_XOR_Request {
#ifdef SIMULATOR
	MV_U32 CommonType;	// please keep it as the first field
#endif	/*SIMULATOR*/
#ifndef NEW_CORE_DRIVER
	MV_U32	seq_id;
#endif
	MV_U32 Error_Offset;                 /* byte, not sector */

	List_Head Queue_Pointer;

	List_Head QP_Cmpl_Seq;
	MV_U32 Is_Active ;

	MV_U16 Device_Id;

	MV_U8 Request_Type;
	MV_U8 Request_Status;

	MV_U8 Source_SG_Table_Count;        /* how many items in the
					       SG_Table_List */
	MV_U8 Target_SG_Table_Count;
#ifdef NEW_CORE_DRIVER
#ifdef SUPPORT_FILL_MEMORY
	MV_U32 data;
#endif
#else
#ifdef SOFTWARE_XOR
	MV_U8 Reserved[2];
#else
	MV_U16 SlotNo;
#endif /* SOFTWARE_XOR */
#endif
	MV_SG_Table Source_SG_Table_List[XOR_SOURCE_SG_COUNT];
	MV_SG_Table Target_SG_Table_List[XOR_TARGET_SG_COUNT];
#ifdef XOR_USE_SG_PTR
        MV_SG_Table *src_sg_tbl_ptr;
        MV_SG_Table *tgt_sg_tbl_ptr;
#endif
	/* TBD: Use one task or several tasks. */
	XOR_COEF    Coef[XOR_TARGET_SG_COUNT][XOR_SOURCE_SG_COUNT];

	MV_PVOID Cmd_Initiator;              /* Which module(extension pointer
						) creates this request. */

	MV_PVOID Context[MAX_POSSIBLE_MODULE_NUMBER];

#ifndef NEW_CORE_DRIVER
	MV_PVOID SG_Buffer;
#endif
	void (*Completion)(MV_PVOID, PMV_XOR_Request);    /* callback */
};
#endif

typedef struct _MV_Target_ID_Map
{
	MV_U16   Device_Id;
	MV_U8    Type;                    /* 0:LD, 1:Free Disk */
	MV_U8    Reserved;
} MV_Target_ID_Map, *PMV_Target_ID_Map;

/* Resource type */
enum Resource_Type
{
	RESOURCE_CACHED_MEMORY = 0,
#ifdef SUPPORT_DISCARDABLE_MEM
	RESOURCE_DISCARDABLE_MEMORY,
#endif
	RESOURCE_UNCACHED_MEMORY
};

/* Module event type */
enum Module_Event
{
	EVENT_MODULE_ALL_STARTED = 0,
#ifdef CACHE_MODULE_SUPPORT
	EVENT_DEVICE_CACHE_MODE_CHANGED,
#endif /* CACHE_MODULE_SUPPORT */
#ifdef SUPPORT_DISCARDABLE_MEM
	EVENT_SPECIFY_RUNTIME_DEVICE,
	EVENT_DISCARD_RESOURCE,
#endif
	EVENT_DEVICE_ARRIVAL,
	EVENT_DEVICE_REMOVAL,
	EVENT_LOG_GENERATED,
#if defined(SUPPORT_BBU) && defined(_OS_FIRMWARE)
       EVENT_BBU_NORMAL,
       EVENT_BBU_ABNORMAL,
#endif
#if defined(LACIE_THUNDERBOLT)
       EVENT_SHORT_PRESS_START,
       EVENT_SHORT_PRESS_STOP,
       EVENT_SHORT_PRESS_CORE_INIT_DONE,
#endif
};

/* Error_Handling_State */
enum EH_State
{
	EH_NONE = 0,
	EH_ABORT_REQUEST,
	EH_LU_RESET,
	EH_DEVICE_RESET,
	EH_PORT_RESET,
	EH_CHIP_RESET,
	EH_SET_DISK_DOWN
};

typedef enum
{
	EH_REQ_NOP = 0,
	EH_REQ_ABORT_REQUEST,
	EH_REQ_HANDLE_TIMEOUT,
	EH_REQ_RESET_BUS,
	EH_REQ_RESET_CHANNEL,
	EH_REQ_RESET_DEVICE,
	EH_REQ_RESET_ADAPTER
}eh_req_type_t;

struct mod_notif_param {
        MV_PVOID  p_param;
        MV_U16    hi;
        MV_U16    lo;

        /* for event processing */
        MV_U32    event_id;
        MV_U16    dev_id;
        MV_U8     severity_lvl;
        MV_U8     param_count;
	MV_U8	sense_length;
	MV_PVOID p_sense;
	MV_U16	tran_hex_bit;
};

/*
 * Exposed Functions
 */

/*
 *
 * Miscellaneous Definitions
 *
 */
/* Rounding */

/* Packed */

#define MV_MAX(x,y)        (((x) > (y)) ? (x) : (y))
#define MV_MIN(x,y)        (((x) < (y)) ? (x) : (y))

#ifndef _OS_BIOS
#define MV_MAX_U64(x, y)   ((((x).value) > ((y).value)) ? (x) : (y))
#define MV_MIN_U64(x, y)   ((((x).value) < ((y).value)) ? (x) : (y))
#else
MV_U64 MV_MAX_U64(MV_U64 x, MV_U64 y);
MV_U64 MV_MIN_U64(MV_U64 x, MV_U64 y);
#endif

#define MV_MAX_U8          0xFF
#define MV_MAX_U16         0xFFFF
#define MV_MAX_U32         0xFFFFFFFFL

#ifdef _OS_LINUX
#   define ROUNDING_MASK(x, mask)  (((x)+(mask))&~(mask))
#   define ROUNDING(value, align)  ROUNDING_MASK(value,   \
						 (typeof(value)) (align-1))
#   define OFFSET_OF(type, member) offsetof(type, member)
#else
#   define ROUNDING(value, align)  ( ((value)+(align)-1)/(align)*(align) )
#   define OFFSET_OF(type, member)    ((MV_U32)(MV_PTR_INTEGER)&(((type *) 0)->member))
#   define ALIGN ROUNDING
#endif /* _OS_LINUX */

#define SIZE_OF_POINTER (sizeof(void*))

#endif /* __MV_COM_TYPE_H__ */
