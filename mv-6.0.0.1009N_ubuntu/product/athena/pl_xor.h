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

#ifndef PL_XOR_H
#define PL_XOR_H

#include "mv_config.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_resource.h"
#include "core_protocol.h"

/*******************************************************************************
*                                                                              *
* XOR Defines                                                                  *
*                                                                              *
*******************************************************************************/

enum {
        XOR_CONTEXT_TYPE_NONE           = 0,
        XOR_CONTEXT_TYPE_TABLE,
};

enum {
        CORE_MAX_XOR_CMD_ENTRY          = 32,

        /* Vanir supports 3-parity GF XOR */
        CORE_MAX_XOR_TARGET_COUNT       = 3,

        /* Number of PRD items field in XOR Command Table is 7 bits */
        CORE_MAX_XOR_PRD_ENTRY          = 0x7F,
};

enum
{
        OP_NO_ACTION =0,
        OP_DIRECT_FILL,
        OP_XOR_ENABLE,
        OP_GF_ENABLE,
        OP_GF_XOR_ENABLE,
        OP_WRITE_ENABLE,
        OP_ZERO_CHECK_ENABLE
};

#define XOR_TABLE_LENGTH  (((sizeof(xor_cmd_entry) * CORE_MAX_XOR_CMD_ENTRY)\
                                / sizeof(MV_U32)) + 6)

#if defined(SUPPORT_ROC)
enum {
        MV_XOR_CORE_0           = 0,
        MV_XOR_CORE_1,
        MV_XOR_CORE_2,
        MV_XOR_CORE_3,
        MV_MAX_XOR_ID
};
#else
enum {
        MV_XOR_CORE_0           = 0,
#ifndef ATHENA_FPGA_WORKAROUND
        MV_XOR_CORE_1,
#endif
        MV_MAX_XOR_ID
};
#endif

enum {
        MV_XOR_CORE_BASE_ADDR   = 0x18000,
        MV_XOR_CORE_ADDR_INCR   = 0x02000,
};


/* XOR_PORT_REG */
/* XOR port registers */
#define XOR_CONFIG              0x000                       /* config reg */
#define XOR_CONTROL             0x004                       /* control/status reg */
#define XOR_IRQ_ROUTING       0x008                       /* XOR Engine Interrupt Routing reg */
#define XOR_DELV_Q_CONFIG       0x010                       /* delv queue config */
#define XOR_DELV_Q_ADDR         0x014                       /* delv queue base addr */
#define XOR_DELV_Q_ADDR_HI      0x018                       /* delv queue base addr hi */
#define XOR_DELV_Q_SHDW_ADDR    0x01c                       /* delv shadow queue base addr */

#define XOR_DELV_Q_WR_PTR       0x020                       /* delv queue write ptr */
#define XOR_DELV_Q_RD_PTR       0x024                       /* delv queue read ptr */
#define XOR_CMPL_Q_CONFIG       0x028                       /* cmpl queue config */
#define XOR_CMPL_Q_ADDR         0x02C                       /* cmpl queue base addr */
#define XOR_CMPL_Q_ADDR_HI      0x030                       /* cmpl queue base addr hi */
#define XOR_CMPL_Q_SHDW_ADDR    0x034                       /* cmpl shadow queue base addr */
#define XOR_CMPL_Q_WR_PTR       0x038                       /* cpml queue write ptr */
#define XOR_CMPL_Q_RD_PTR       0x03C                       /* cpml queue read ptr */

#define XOR_COAL_CONFIG         0x040                       /* intr coalescing config */
#define XOR_COAL_TIMEOUT        0x044                       /* intr coalescing time wait */
#define XOR_IRQ_STAT            0x048                       /* interrupt status */
#define XOR_IRQ_MASK            0x04C                       /* intr enable/disable mask */

#define XOR_T10_ERR_OFF         0x050                       /* XOR Engine T10 Error Offset (R18050h) */
#define XOR_T10_EXPECT_LBRT     0x054                       /* XOR Engine T10 Error Expected LBRT (R18054h) */
#define XOR_T10_EXPECT_LBAT_LBG 0x058                       /* XOR Engine T10 Error Expected LBAT LBG (R18058h) */
#define XOR_T10_ACTUAL_LBRT     0x05C                       /* XOR Engine T10 Error Actual LBRT (R1805Ch) */
#define XOR_T10_ACTUAL_LBAT_LBG 0x060                       /* XOR Engine T10 Error Actual LBAT LBG (R18060h) */

#define XOR_ERR_INFO            0x070
#define XOR_ERR_TBL_ADDR        0x074                       /* err table base addr */
#define XOR_ERR_TBL_ADDR_HI     0x078                       /* err table base addr hi */
#define XOR_ERR_PRD_ADDR        0x07c
#define XOR_ERR_PRD_ADDR_HI     0x080
#define XOR_ERR_MEM_PRERR       0x088
#define XOR_ERR_MEM_PRERR_EN    0x08c
#   define XOR_ERR_MEM_PRERR_MASK	0x0000007F

#define XOR_DP_PCRTL            0x090
#   define XOR_DP_PCRTL_DP_FERR_EN	0x00000004
#   define XOR_DP_PCRTL_DP_PERR_EN	0x00000002
#   define XOR_DP_PCRTL_DP_PAR_ODD  0x00000001

#define XOR_DP_PSTAT            0x094
#define XOR_DP_ERR_ADDR         0x098
#define XOR_DP_ERR_ADDR_HI      0x09c

//#define XOR_NEXT_TAB_ADDR       0x0A0                       /* next xor table base addr */

/* XOR CONFIG   (R000h) */
#define XOR_POLY_11D                0 /* GF: (X^8 + X^4 + X^3 + X^2 + 1) */
#define XOR_POLY_14D                1 /* GF: (X^8 + X^6 + X^3 + X^2 + 1) */

/* XOR_CONTROL  (R004h) */
#define XOR_ENABLE                  (1 << 0)
#define XOR_RESET                   (1 << 1)
#define XOR_ABORT                   (1 << 2)
#define ABORT_ZRC_ERROR             (1 << 3) /* ZRC : Zero Result Check */
#define PAUSE_AT_TABLE_BOUNDRY      (1 << 4)
#define CMD_LIST_INTF_SEL_MASK      (0xF   << 8)
#define CURRENT_CMD_SLOT_MASK       (0xFFF << 16)
#define XOR_ERROR                   (1 << 29)
#define XOR_PAUSE                   (1 << 30)
#define XOR_CMD_RUNNING             (1 << 31)
/* XOR_IRQ_ROUTING  (R008h) */
#define XOR_PCIE_FNCTN_SEL_0_FUN_0 (0L << 8)
#define XOR_PCIE_FNCTN_SEL_0_FUN_1 (1L << 8)
#define XOR_MSI_X_VCTR_SEL_0_MASK    (0x1F << 0)
#define XOR_MSI_X_VCTR_SEL_0_SHIFT    (0)
/* XOR_DELV_Q_CONFIG (R010) bits */
#define XOR_MAX_DELV_QUEUE_SIZE    0x1FFF 

#define XOR_DELV_QUEUE_SIZE_MASK    (0x1FFF << 0)
#define XOR_DELV_QUEUE_INT_SEL_MASK (0xF << 12)
#define XOR_DELV_QUEUE_ENABLE       (1 << 31)
#define XOR_DELV_Q_WP_SHDW_EN       (1 << 30)

/* XOR_CMPL_Q_CONFIG (R028) bits */
#define XOR_MAX_CMPL_QUEUE_SIZE    0x1FFF 

#define XOR_CMPL_QUEUE_SIZE_MASK    (0x1FFF << 0)
#define XOR_CMPL_QUEUE_INT_SEL_MASK (0xF << 12)
#define XOR_CMPL_QUEUE_ENABLE       (1 << 31)
#define XOR_CMPL_Q_WP_SHDW_EN       (1 << 30)

/* XOR_COAL_CONFIG (R040) bits */
#define XOR_INT_COAL_COUNT_MASK     (0x1FF << 0)
#define XOR_INT_COAL_ENABLE         (1 << 16)

/* XOR_COAL_TIMEOUT (R044) bits */
#define XOR_COAL_TIMER_MASK         (0xFFFF << 0)
#define XOR_COAL_TIMER_UNIT_1US     (1 << 16) /* 1us, 6.67 ns if not set (0) */

/* XOR_IRQ_CAUSE (R048) bits */
#define XOR_CQ_NOT_EMPTY            (0x1)
#define XOR_TBL_PROC_IRQ            (0x2)
#define XOR_PAUSED_IRQ              (0x4)
#define XOR_DLVRY_Q_NOT_FULL_IRQ  (0x8)
#define XOR_OVERRUN_IRQ             (0x100)
#define XOR_UNDERRUN_IRQ            (0x200)
#define XOR_B0_ZERO_RSLTCHK_ERR_IRQ (0x400)
#define XOR_B1_ZERO_RSLTCHK_ERR_IRQ (0x800)
#define XOR_B2_ZERO_RSLTCHK_ERR_IRQ (0x1000)
#define XOR_REF_CHK_ERR                      (0x2000)
#define XOR_APP_CHK_ERR                      (0x4000)
#define XOR_GRD_CHK_ERR                      (0x8000)
#define XOR_T10_CNTRL_ERR                  (0x10000)
#define XOR_PI_CHAIN_MISMATCH_ERR  (0x20000)
#define XOR_NM_PI_ENTRY_MISMATCH_ERR  (0x40000)

#define XOR_DP_PERR_IRQ                 (0x400000)
#define XOR_INT_MEM_PERR_IRQ        (0x800000)
#define XOR_UART_IRQ                         (0x8000000)
#define XOR_I2C_IRQ                             (0x10000000)
#define XOR_SGPIO_IRQ                        (0x20000000)
#define XOR_GPIO_IRQ                         (0x40000000)
#define XOR_SYS_ERR_IRQ                   (0x80000000)

/* XOR_ERR_INFO (R070) bits */
#define XOR_ERR_INFO_ERR_OFFSET		(0xFFFFFF << 0)
#define XOR_ERR_INFO_ERR_OP		    (0x1F << 24)


#define XOR_ZERO_RSLTCHK(x)		((x & XOR_B0_ZERO_RSLTCHK_ERR_IRQ)\
					|| (x & XOR_B1_ZERO_RSLTCHK_ERR_IRQ)\
					|| (x & XOR_B2_ZERO_RSLTCHK_ERR_IRQ))
#define MV_DMA_CORE_ID                  MV_XOR_CORE_0
#ifdef SUPPORT_ATHENA
#define MV_XOR_CORE_ID                  MV_XOR_CORE_0
#else
#define MV_XOR_CORE_ID                  MV_XOR_CORE_1
#endif
/*******************************************************************************
*                                                                              *
* XOR Structures                                                               *
*                                                                              *
*******************************************************************************/

/* XOR Delivery Queue Entry */
typedef struct _xor_delv_q_entry
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32  XOR_PI_TBL_LNGTH:8;  // XOR/PI Table Length.
        MV_U32  IFC_SLCT:8;	    // Interface Select.This field contains the MXI Port ID of the memory that contains the XOR Table and optional PI Table.
        MV_U32  Reserved1:1;
        MV_U32  PIT_PRSNT:1;
        MV_U32  NO_IRQ:1;   //kernel has defined NO_IRQ
        MV_U32  CMD_SKIP:1;
        MV_U32  SLOT_NM:12;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32  SLOT_NM:12;
        MV_U32  CMD_SKIP:1;
        MV_U32  NO_IRQ:1;   //kernel has defined NO_IRQ
        MV_U32  PIT_PRSNT:1;
        MV_U32  Reserved1:1;
        MV_U32  IFC_SLCT:8;	    // Interface Select.This field contains the MXI Port ID of the memory that contains the XOR Table and optional PI Table.
        MV_U32  XOR_PI_TBL_LNGTH:8;  // XOR/PI Table Length.
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32 PI_TBL_ADDR_L;
        MV_U32 XOR_TBL_ADDR_L;
        MV_U32 XOR_PI_ADDR_H;

} xor_delv_q_entry;
/* XOR Completion Queue Entry */
typedef struct _xor_cmpl_q_entry
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
    MV_U32 ATT : 1;
    MV_U32 Reserved2 : 8;
    MV_U32 CMD_ERR:1;
    MV_U32 XOR_DLVRY_Q_NOT_FULL : 1;
    MV_U32 CMD_PAUSE : 1;
    MV_U32 CMD_ABORT : 1;
    MV_U32 CMD_SKIP : 1;
    MV_U32 CMD_PAUSE : 1;
    MV_U32 TBL_CMPL : 1;
    MV_U32 CMD_CMPL : 1;
    MV_U32 Reserved1 : 4;
    MV_U32 SLOT_NM : 12;
#else
    MV_U32 SLOT_NM : 12;
    MV_U32 Reserved1 : 4;
    MV_U32 CMD_CMPL : 1;
    MV_U32 TBL_CMPL : 1;
    MV_U32 CMD_SKIP : 1;
    MV_U32 CMD_ABORT : 1;
    MV_U32 CMD_PAUSE : 1;
    MV_U32 XOR_DLVRY_Q_NOT_FULL : 1;
    MV_U32 CMD_ERR : 1;
    MV_U32 Reserved2 : 8;
    MV_U32 ATT : 1;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
} xor_cmpl_q_entry;

/* XOR_IRQ_STAT bits */
typedef struct _REG_XOR_IRQ_STAT
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32  Reserved3:7;
        MV_U32  B2_ZR0_RSLT_CHK_ERR:1;
        MV_U32  B1_ZR0_RSLT_CHK_ERR:1;
        MV_U32  Reserved2:6;
        MV_U32  B0_ZR0_RSLT_CHK_ERR:1;
        MV_U32  UNDERRUN_IRQ:1;
        MV_U32  MBUS_PERR:1;
        MV_U32  DATA_OVERRUN:1;
        MV_U32  XOR_PI_TABLE_MISMATCH:1;
        MV_U32  PI_CHAIN_ERR:1;
        MV_U32  T10_CRTL_ERR:1;
        MV_U32  Reserved1:7;
        MV_U32  XOR_PAUSED:1;
        MV_U32  TBL_PRCSSD:1;
        MV_U32  CMD_CMPL:1;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32  CMD_CMPL:1;
        MV_U32  TBL_PRCSSD:1;
        MV_U32  XOR_PAUSED:1;
        MV_U32  Reserved1:7;
        MV_U32  T10_CRTL_ERR:1;
        MV_U32  PI_CHAIN_ERR:1;
        MV_U32  XOR_PI_TABLE_MISMATCH:1;
        MV_U32  DATA_OVERRUN:1;
        MV_U32  MBUS_PERR:1;
        MV_U32  UNDERRUN_IRQ:1;
        MV_U32  B0_ZR0_RSLT_CHK_ERR:1;
        MV_U32  Reserved2:6;
        MV_U32  B1_ZR0_RSLT_CHK_ERR:1;
        MV_U32  B2_ZR0_RSLT_CHK_ERR:1;
        MV_U32  Reserved3:7;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_XOR_IRQ_STAT, *PREG_XOR_IRQ_STAT;

/* XOR_ERR_PRD bits */
typedef struct _REG_XOR_ERR_PRD
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   Reserved4:3;
        MV_U32   B2_ERR_PRD:5;
        MV_U32   Reserved3:1;
        MV_U32   ERR_TBL:7;
        MV_U32   Reserved2:3;
        MV_U32   B1_ERR_PRD:5;
        MV_U32   Reserved1:3;
        MV_U32   B0_ERR_PRD:5;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32   B0_ZR0_RSLT_CHK_ERR:1;
        MV_U32   B0_ERR_PRD:5;
        MV_U32   Reserved1:3;
        MV_U32   B1_ERR_PRD:5;
        MV_U32   Reserved2:3;
        MV_U32   ERR_TBL:7;
        MV_U32   Reserved3:1;
        MV_U32   B2_ERR_PRD:5;
        MV_U32   Reserved4:3;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_XOR_ERR_PRD, *PREG_XOR_ERR_PRD;

/* XOR_ERR_INTRFC bits */
typedef struct _REG_XOR_ERR_INTRFC
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   Reserved2:13;
        MV_U32   ERR_OFFSET:11;
        MV_U32   Reserved1:4;
        MV_U32   ERR_INTRFC:4;
#else
        MV_U32   Reserved1:4;
        MV_U32   ERR_INTRFC:4;
        MV_U32   ERR_OFFSET:11;
        MV_U32   Reserved2:13;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_XOR_ERR_INTRFC, *PREG_XOR_ERR_INTRFC;

/* XOR_DELV_Q_WR_PTR bits */
typedef struct _REG_XOR_DELV_Q_WR_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   Reserved:20;
        MV_U32   DELV_QUEUE_WRT_PTR:12;
#else
        MV_U32   DELV_QUEUE_WRT_PTR:12;
        MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_XOR_DELV_Q_WR_PTR, *PREG_XOR_DELV_Q_WR_PTR;

/* XOR_DELV_Q_RD_PTR bits */
typedef struct _REG_XOR_DELV_Q_RD_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   Reserved:20;
        MV_U32   DELV_QUEUE_RD_PTR:12;
#else
        MV_U32   DELV_QUEUE_RD_PTR:12;
        MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_XOR_DELV_Q_RD_PTR, *PREG_XOR_DELV_Q_RD_PTR;

/* XOR Command entry */
typedef struct _xor_cmd_entry
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
/* DWORD 0 */
        MV_U32   memory_fill:1;
        MV_U32   prd_entry_count:7;
        MV_U32   prd_tbl_intrfc:8;
        MV_U32   reserved_1:1;
        MV_U32   crc32_en_2:1;
        MV_U32   crc32_en_1:1;
        MV_U32   crc32_en_0:1;
        MV_U32   buffer_2_opcode:4;
        MV_U32   buffer_1_opcode:4;
        MV_U32   buffer_0_opcode:4;
/* DWORD 1 */
        MV_U32   data_byte_3:8;
        MV_U32   data_byte_2:8;
        MV_U32   data_byte_1:8;
        MV_U32   data_byte_0:8;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
/* DWORD 0 */
        MV_U32   buffer_0_opcode:4;
        MV_U32   buffer_1_opcode:4;
        MV_U32   buffer_2_opcode:4;
        MV_U32   crc32_en_0:1;
        MV_U32   crc32_en_1:1;
        MV_U32   crc32_en_2:1;
        MV_U32   reserved_1:1;
        MV_U32   prd_tbl_intrfc:8;
        MV_U32   prd_entry_count:7;
        MV_U32   memory_fill:1;
/* DWORD 1 */
        MV_U32   data_byte_0:8;
        MV_U32   data_byte_1:8;
        MV_U32   data_byte_2:8;
        MV_U32   data_byte_3:8;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
/* DWORD 2 */
        MV_U32   prd_tbl_addr;
/* DWORD 3 */
        MV_U32   prd_tbl_addr_high;
} xor_cmd_entry;
enum _xor_prd_chained_flag {
        XOR_PRD_CHAINED_ENABLE          = MV_BIT(24),
        XOR_PRD_CHAINED_INTRFC_SLCT     = 28,
};

typedef struct _xor_prd_chained_entry {
        MV_U32 next_addr_lo;
        MV_U32 next_addr_hi;
        MV_U32 flags;
} xor_prd_chained_entry;

enum _xor_prd_data_flag {
        XOR_PRD_DATA_INTRFC_SLCT        = 28,
};

typedef struct _xor_prd_data_entry {
        MV_U32 next_addr_lo;
        MV_U32 next_addr_hi;
        MV_U32 flags;
} xor_prd_data_entry;

typedef union _xor_prd_entry {
        xor_prd_data_entry      data_buf_entry;
        xor_prd_chained_entry   chained_tbl_entry;
} xor_prd_entry;

/* XOR Protection Information Table */


/******************************************************************************
 *        XOR PI Record                                                       *
 *   ________________________                                                 *
 *  |                        |                                                *
 *  |   PIR Control Field    |                                                *
 *  |________________________|                                                *
 *  |                        |                                                *
 *  |       T10 Field        |                                                *
 *  |       (optional)       |                                                *
 *  |________________________|                                                *
 *  |                        |                                                *
 *  |    Chained PI Field    |                                                *
 *  |       {optional)       |                                                *
 *  |________________________|                                                *
 *                                                                            *
 * The XOR PI Record contains 3 fields: control field, T10 field, and chained *
 * PI field. The control field is required, while the T10 and chained field   *
 * are optional.                                                              *
 *                                                                            *
 * The T10 field is present if the T10Present bit is set in PIR Control       *
 * / Interface Select in the PI Table entry.                                  *
 *                                                                            *
 * The Chained PI field is present if the ChainedPI bit is set in the PIR     *
 * Control field of the XOR PI Record.                                        *
 *                                                                            *
 *****************************************************************************/
 typedef struct _XOR_PIR_CRTL_FLD
{
    /* DWORD 0 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
    MV_U32 UserDataSize : 12;
    MV_U32 Reserved1 : 8;
    MV_U32 LbgTypeGen:1;
    MV_U32 LbgTypeChk:1;
    MV_U32 LoadT10 : 1;
    MV_U32 Reserved2 : 1;
    MV_U32 PRDInclT10 : 1;
    MV_U32 IncrLBATag : 1;
    MV_U32 IncrLBRTag : 1;
    MV_U32 CheckDisable : 1;
    MV_U32 CheckT10 : 1;
    MV_U32 ReplaceT10 : 1;
    MV_U32 RemoveT10 : 1;
    MV_U32 InsertT10 : 1;
#else
    MV_U32 InsertT10 : 1;
    MV_U32 RemoveT10 : 1;
    MV_U32 ReplaceT10 : 1;
    MV_U32 CheckT10 : 1;
    MV_U32 CheckDisable : 1;
    MV_U32 IncrLBRTag : 1;
    MV_U32 IncrLBATag : 1;
    MV_U32 PRDInclT10 : 1;
    MV_U32 Reserved2 : 1;
    MV_U32 LoadT10 : 1;
    MV_U32 LbgTypeChk:1;
    MV_U32 LbgTypeGen:1;
    MV_U32 Reserved1 : 8;
    MV_U32 UserDataSize : 12;
#endif
} XOR_PIR_CRTL_FLD, *PXOR_PIR_CRTL_FLD;
typedef struct _XOR_PIR_T10_FLD
{
    /* DWORD 0 */
    MV_U32 LBRTagChk;

    /* DWORD 1 */
    MV_U32 LBRTagGen;

#ifdef __MV_BIG_ENDIAN_BITFIELD__
    /* DWORD 2 */
    MV_U32 LBATagChkMask : 16;
    MV_U32 LBATagChkValue : 16;

    /* DWORD 3 */
    MV_U32 T10ReplaceMask : 8;
    MV_U32 T10ChkMask : 8;
    MV_U32 LBATagGenValue : 16;
#else
    /* DWORD 2 */
    MV_U32 LBATagChkValue : 16;
    MV_U32 LBATagChkMask : 16;

    /* DWORD 3 */
    MV_U32 LBATagGenValue : 16;
    MV_U32 T10ChkMask : 8;
    MV_U32 T10ReplaceMask : 8;
#endif
} XOR_PIR_T10_FLD, *PXOR_PIR_T10_FLD;

typedef struct _XOR_PIR_CHAINED_FLD
{
    /* DWORD 0 & 1 */
    MV_U64 NextPIRAddr;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
    /* DWORD 2 */
    MV_U32 NextPIRIntrfc : 8;

    /* This entry must be set to 1 in a Chained Record Pointer entry */
    MV_U32 ChainedPIPresent : 1;
    MV_U32 T10Present : 1;
    MV_U32 KeyTagPresent : 1;
    MV_U32 BlockIndexPresent : 1;
    MV_U32 Reserved1 : 4;
    MV_U32 NMValidBlk : 16;
#else
    MV_U32 NMValidBlk : 16;
    MV_U32 Reserved1 : 4;
    MV_U32 BlockIndexPresent : 1;
    MV_U32 KeyTagPresent : 1;
    MV_U32 T10Present : 1;

    /* This entry must be set to 1 in a Chained Record Pointer entry */
    MV_U32 ChainedPIPresent : 1;
    MV_U32 NextPIRIntrfc : 8;
#endif
} XOR_PIR_CHAINED_FLD, *PXOR_PIR_CHAINED_FLD;
typedef struct _xor_pi_structure
{
/* DWORD 0 */
    XOR_PIR_CRTL_FLD pi_ctrl;
/* DWORD 1-4 */
    XOR_PIR_T10_FLD   t10_flds;
/* DWORD 5-7 */
    XOR_PIR_CHAINED_FLD   chained_flds;
} xor_pi_structure;
typedef struct _xor_pi_cmd_entry
{
/* DWORD 0 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   pir_ifc_slct:8;
        MV_U32   chnd_pi_flds_prsnt:1;
        MV_U32   t10_flds_prsnt:1;
        MV_U32   reserved_2_2:2;
        MV_U32   pir_prsnt:1;
        MV_U32   resserved_2_1:19;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32   resserved_2_1:19;
        MV_U32   pir_prsnt:1;
        MV_U32   reserved_2_2:2;
        MV_U32   t10_flds_prsnt:1;
        MV_U32   chnd_pi_flds_prsnt:1;
        MV_U32   pir_ifc_slct:8;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
/* DWORD 1 */
        MV_U32   pir_tbl_addr;
/* DWORD 2 */
        MV_U32   pir_tbl_addr_high;
/* DWORD 3 */
        MV_U32   reserved;
} xor_pi_cmd_entry;
/* Protection Information Table Format */
typedef struct _xor_pi_cmd_table
{
/* DWORD 0 */
        MV_U32   next_pi_addr_lo;
/* DWORD 1 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   reserved_1_2:2;
        MV_U32   nm_valid_pir:6;
        MV_U32   reserved_1_1:24;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32   reserved_1_1:24;
        MV_U32   nm_valid_pir:6;
        MV_U32   reserved_1_2:2;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
/* DWORD 2-5 */
        MV_U32   reserved[4];
        xor_pi_cmd_entry xor_pi[CORE_MAX_XOR_CMD_ENTRY];
} xor_pi_cmd_table;
enum {
	XOR_CMD_TBL_IRQ_EN	= (1 << 0),
	XOR_CMD_TBL_CMPL	= (1 << 1),
	XOR_CMD_TBL_ZRC_ERR	= (1 << 2),
	XOR_CMD_TBL_CRC_RVRSE	= (1 << 14),
};

/* XOR Command Table */
typedef struct _xor_cmd_table
{
/* DWORD 0 */
        MV_U32   next_tbl_addr_lo;
/* DWORD 1 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
        MV_U32   reserved_2:17;
        MV_U32   crc32_rev_order:1;
        MV_U32   valid_entry:6;
        MV_U32   reserved_1:5;
        MV_U32   zero_chk_err:1;
        MV_U32   tbl_cmpl:1;
        MV_U32   interrupt_en:1;
#else /* __MV_BIG_ENDIAN_BITFIELD__ */
        MV_U32   interrupt_en:1;
        MV_U32   tbl_cmpl:1;
        MV_U32   zero_chk_err:1;
        MV_U32   reserved_1:5;
        MV_U32   valid_entry:6;
        MV_U32   crc32_rev_order:1;
        MV_U32   reserved_2:17;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
/* DWORD 2 */
        MV_U32   total_byte_count;        /* Byte count for this table only */
/* DWORD 3 */
        MV_U32   crc32_0_sr;
/* DWORD 4 */
        MV_U32   crc32_1_sr;
/* DWORD 5 */
        MV_U32   crc32_2_sr;

        xor_cmd_entry xor_prd[CORE_MAX_XOR_CMD_ENTRY];
} xor_cmd_table;
#if defined(SOFTWARE_XOR) || defined(HARDWARE_XOR)
extern void xor_sg_table_release(
        MV_PVOID core,
        MV_XOR_Request *xor_req);

extern void xor_set_context_finished(PMV_XOR_Request xor_req,
        MV_U16 slot_num);
extern MV_U8 xor_is_request_finished(PMV_XOR_Request xor_req);
void xor_write_command(MV_PVOID core_p, MV_PVOID xor_p,
	PMV_XOR_Request xor_req);
void xor_dma_command(MV_PVOID core_p, MV_PVOID xor_p,
	PMV_XOR_Request xor_req);
void xor_compare_command(MV_PVOID core_p, MV_PVOID xor_p, 
	PMV_XOR_Request xor_req);
#endif // #if defined(SOFTWARE_XOR) || defined(HARDWARE_XOR)
void xor_reset_all_cores(MV_PVOID core_p);
void xor_init_core(MV_PVOID core_p, MV_U16 core_id);
MV_U8 xor_get_xor_core_id(MV_PVOID core_p, MV_PVOID xor_req_p);
#endif



