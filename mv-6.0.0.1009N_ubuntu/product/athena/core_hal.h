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

#ifndef __CORE_HAL_H
#define __CORE_HAL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_discover.h"

#include "core_cpu.h"

#ifdef  ATHENA_FPGA_WORKAROUND
#ifndef MAX_NUMBER_IO_CHIP
#define MAX_NUMBER_IO_CHIP                      1 /* how many io chips per hardware */
#endif
#define MAX_REGISTER_SET_PER_IO_CHIP            64
#else
#define MAX_NUMBER_IO_CHIP                      2 /* how many io chips per hardware */
#define MAX_REGISTER_SET_PER_IO_CHIP            128
#endif

#define MAX_PORT_PER_PL                         8
#define MAX_PHY_PER_PL                          8
#ifdef ATHENA_FPGA_WORKAROUND
#define CORE_MAX_REQUEST_NUMBER                 120 //0x7F /* per io chip */
#else
#ifdef _OS_LINUX
//Due to Athena now allocate command struct in core_init_dma_memory, so CORE_MAX_REQUEST_NUMBER need to be small.
#define CORE_MAX_REQUEST_NUMBER                 1024//4096 ///* per io chip */ 
#else
#define CORE_MAX_REQUEST_NUMBER                 768 /* per io chip */
#endif
#endif

typedef enum _VANIR_REVISION_ID {
	VANIR_A0_REV		= 0xA0,
	VANIR_B0_REV		= 0x01,
	VANIR_C0_REV		= 0x02,
	VANIR_C1_REV		= 0x03,
	VANIR_C2_REV		= 0xC2,
} VANIR_REVISION_ID;
/*
  ========================================================================
  14xx chip registers
  ========================================================================
*/
#define MV_IO_CHIP_REGISTER_BASE                                     0x20000
#define MV_IO_CHIP_REGISTER_RANGE                                    0x04000

enum athena_common_regs {
	/* SATA/SAS port common registers */
	COMMON_PORT_IMPLEMENT        = 0x20,  /* port implement register */
	COMMON_PORT_TYPE        = 0x24,  /* port type register */
	COMMON_FIS_ADDR         = 0x30, /* FIS rx buf addr */
	COMMON_FIS_ADDR_HI      = 0x34, /* FIS rx buf addr hi */

	COMMON_LST_ADDR         = 0x38, /* command list DMA addr -- External IO Context Base Address*/
	COMMON_LST_ADDR_HI      = 0x3C, /* command list DMA addr hi --  External IO Context Base Address hi*/
	COMMON_CONFIG           = 0x40, /* configuration register */
	COMMON_CONTROL          = 0x44, /* control register */
	
	COMMON_SATA_REG_SET0    = 0x50, /* SATA/STP Register Set 0 */
	COMMON_SATA_REG_SET1    = 0x54, /* SATA/STP Register Set 1 */
	COMMON_SATA_REG_SET2    = 0x58, /* SATA/STP Register Set 2 */
	COMMON_SATA_REG_SET3    = 0x5C, /* SATA/STP Register Set 3 */

	COMMON_SRS_IRQ_STAT0    = 0xD0, /* SRS interrupt status 0 */
	COMMON_SRS_IRQ_MASK0    = 0xD4, /* SRS intr enable/disable mask 0 */
	COMMON_SRS_IRQ_STAT1    = 0xD8, /* SRS interrupt status 1*/
	COMMON_SRS_IRQ_MASK1    = 0xDC, /* SRS intr enable/disable mask 1*/
	COMMON_SRS_IRQ_STAT2    = 0xE0, /* SRS interrupt status 2 */
	COMMON_SRS_IRQ_MASK2    = 0xE4, /* SRS intr enable/disable mask 2 */
	COMMON_SRS_IRQ_STAT3    = 0xE8, /* SRS interrupt status 3*/
	COMMON_SRS_IRQ_MASK3    = 0xEC, /* SRS intr enable/disable mask 3*/
	
	COMMON_NON_SPEC_NCQ_ERR0= 0x0110, /* Non Specific NCQ Error 0 */
	COMMON_NON_SPEC_NCQ_ERR1= 0x0114, /* Non Specific NCQ Error 1 */
	COMMON_NON_SPEC_NCQ_ERR2= 0x0118, /* Non Specific NCQ Error 2 */
	COMMON_NON_SPEC_NCQ_ERR3= 0x011C, /* Non Specific NCQ Error 3 */

	COMMON_CMD_ADDR         = 0x0130, /* Command Address Port */
	COMMON_CMD_DATA         = 0x0134, /* Command Data Port */

	COMMON_STP_CLR_AFFILIATION_DIS0   = 0x0150, /* STP Clear Affiliation Disable 0*/
	COMMON_STP_CLR_AFFILIATION_DIS1   = 0x0154, /* STP Clear Affiliation Disable 1*/
	COMMON_STP_CLR_AFFILIATION_DIS2   = 0x0158, /* STP Clear Affiliation Disable 2*/
	COMMON_STP_CLR_AFFILIATION_DIS3   = 0x015C, /* STP Clear Affiliation Disable 3*/

	/* Port interrupt status/mask register set $i (0x180/0x184-0x1b8/0x1bc) */
//	COMMON_PORT_IRQ_STAT0   = 0x0180,
//	COMMON_PORT_IRQ_MASK0   = 0x0184,

//	COMMON_PORT_ALL_IRQ_STAT   = 0x01c0, /* All Port interrupt status */
//	COMMON_PORT_ALL_IRQ_MASK   = 0x01c4, /* All Port interrupt enable/disable mask */

	/* port config address/data regsiter set $i (0x170/0x174- 0x208/0x20c) */
	COMMON_PORT_CONFIG_ADDR0  = 0x0170,
	COMMON_PORT_CONFIG_DATA0  = 0x0174,
	COMMON_PORT_ALL_CONFIG_ADDR  = 0x01B0, /* All Port config address */
	COMMON_PORT_ALL_CONFIG_DATA  = 0x01B4, /* All Port config data */


	/*phy control/data register set $i (0x220/0x224- 0x258/0x25c) */
	COMMON_PORT_PHY_CONTROL0  = 0x0220,
	COMMON_PORT_PHY_CONTROL_DATA0  = 0x0224,

	COMMON_PORT_ALL_PHY_CONTROL = 0x0260, /* All phy Config/Control Address Port*/
	COMMON_PORT_ALL_PHY_CONTROL_DATA = 0x0264, /* All phy Config/Control Data Port */


	/* port vendor specific address/data register set $i (0x250/0x254-0x268/0x26c) */
//	COMMON_PORT_VSR_ADDR0      = 0x0250,
//	COMMON_PORT_VSR_DATA0      = 0x0254,
//	COMMON_PORT_ALL_VSR_ADDR   = 0x0290, /* All port Vendor Specific Register addr */
//	COMMON_PORT_ALL_VSR_DATA   = 0x0294, /* All port Vendor Specific Register Data */

        /* Athena new reqister */
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	COMMON_KEY_UNWRAP_ENG_CTRL  = 0x02A0, /* Key Unwrap Engine Control */
	COMMON_KEY_UNWRAP_ENG_STS  = 0x02A4, /* Key Unwrap Engine Status */
	COMMON_S_KEK0_INIT_VAL  = 0x02A8,           /* S_KEK 0 Initial Value Lower 32 Bits */
	COMMON_S_KEK0_INIT_VAL_HI  = 0x02AC, /* S_KEK 0 Initial Value Upper 32 Bits */
	COMMON_S_KEK1_INIT_VAL  = 0x02B0,       /* S_KEK 1 Initial Value Lower 32 Bits */
	COMMON_S_KEK1_INIT_VAL_HI  = 0x02B4, /* S_KEK 1 Initial Value Upper 32 Bits */
	COMMON_KEY_UNWRAP_ENG_IRQ_STAT  = 0x02B8, /* Key Unwrap Engine Interrupt Cause*/
	COMMON_KEY_UNWRAP_ENG_IRQ_MASK  = 0x02BC, /* Key Unwrap Engine Interrupt Enable */
#endif
	COMMON_INTL_MEM_PARITY_ERR  = 0x02D0, /* Internal Memory Parity Error */
	COMMON_INTL_MEM_PARITY_ERR_EN  = 0x02D4, /* Internal Memory Parity Error Enable*/
	COMMON_DATA_PATH_PARITY_CTRL  = 0x02D8, /*Data Path Parity Control*/
	COMMON_DATA_PATH_PARITY_STS  = 0x02DC,   /* Data Path Parity Status*/
	COMMON_DATA_PATH_ERR_ADDR = 0x02E0,     /*Data Path Error Address Low*/
	COMMON_DATA_PATH_ERR_ADDR_HI  = 0x02E4, /* Data Path Error Address High*/
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	COMMON_GLOBAL_SECURITY_CONFIG = 0x0300, /* Global Security Configuration */
	COMMON_GLOBAL_ZERIOCATION_CTRL  = 0x0304, /* Global Zeriozation Control*/
	COMMON_R_KEK_VAULT_CTRL  = 0x0308,              /*R-KEK Vault Control*/
	COMMON_R_KEK_VAULT_STS  = 0x030C,               /*R-KEK Vault Status*/
	COMMON_R_KEK0_INIT_VAL  = 0x0310,           /* R_KEK Initial Value Lower 32 Bits */
	COMMON_R_KEK0_INIT_VAL_HI  = 0x0314, /* R_KEK Initial Value Upper 32 Bits */
	COMMON_VAULT_PORT_CTRL  = 0x0318,              /*Vault Port Control*/
	COMMON_VAULT_PORT_DATA  = 0x031C,               /*Vault Port Data*/
	COMMON_GLOBAL_SECURITY_IRQ_STAT = 0x0320, /* Global Security Interrupt Cause*/
	COMMON_GLOBAL_SECURITY_IRQ_MASK = 0x0324, /* Global Security Interrupt Enable */
#endif
	COMMON_PORTLAYER_Q0    = 0x350, /* PortLayerQ 0 Enable */
	COMMON_PORTLAYER_Q1    = 0x354, /* PortLayerQ 1 Enable */
	COMMON_PORTLAYER_Q2    = 0x358, /* PortLayerQ 2 Enable */
	COMMON_PORTLAYER_Q3    = 0x35C, /* PortLayerQ 3 Enable */
	COMMON_PORTLAYER_Q0_STAT	= 0x3D0, /* PortLayerQ 0 Interrupt Status */
	COMMON_PORTLAYER_Q0_MASK	= 0x3D4, /* PortLayerQ 0 Interrupt Enable*/
	COMMON_PORTLAYER_Q1_STAT	= 0x3D8, /* PortLayerQ 1 Interrupt Status*/
	COMMON_PORTLAYER_Q1_MASK	= 0x3DC, /* PortLayerQ 1 Interrupt Enable*/
	COMMON_PORTLAYER_Q2_STAT	= 0x3E0, /* PortLayerQ 2 Interrupt Status */
	COMMON_PORTLAYER_Q2_MASK	= 0x3E4, /* PortLayerQ 2 Interrupt Enable*/
	COMMON_PORTLAYER_Q3_STAT	= 0x3E8, /* PortLayerQ 3 Interrupt Status*/
	COMMON_PORTLAYER_Q3_MASK	= 0x3EC, /* PortLayerQ 3 Interrupt Enable*/
	/* Multi-queue */
	COMMON_DELV_Q0_CONFIG	 = 0x400, /* delivery queue 0 configuration */
	COMMON_DELV_Q0_ADDR 	 = 0x404, /* delivery queue 0 base address */
	COMMON_DELV_Q0_ADDR_HI	 = 0x408, /* delivery queue 0 base address hi */
	COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR	= 0x40C, /* delivery queue 0 Read Pointer Shadow Address */
	COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR_HI   = 0x410, /* delivery queue 0 Read Pointer Shadow Address hi*/
	COMMON_DELV_Q0_WR_PTR	 = 0x414, /* delivery queue 0 write pointer */
	COMMON_DELV_Q0_RD_PTR	 = 0x418, /* delivery queue 0 read pointer */

	COMMON_CMPL_Q0_CONFIG	 = 0x600, /* completion queue 0 configuration */
	COMMON_CMPL_Q0_ADDR 	 = 0x604, /* completion queue 0 base address */
	COMMON_CMPL_Q0_ADDR_HI	 = 0x608, /* completion queue 0 base address hi */
	COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR	= 0x60C, /* completion queue 0 Write Pointer Shadow Address */
	COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR_HI   = 0x610, /* completion queue 0 Write Pointer Shadow Address hi*/
	COMMON_CMPL_Q0_WR_PTR	 = 0x614, /* completion queue 0 write pointer */
	COMMON_CMPL_Q0_RD_PTR	 = 0x618, /* completion queue 0 read pointer */

	COMMON_COAL0_CONFIG 	 = 0x700, /* interrupt coalescing 0 config */
	COMMON_COAL0_TIMEOUT	 = 0x740, /* interrupt coalescing 0 time wait */
	COMMON_IRQ_STAT 		= 0x800, /* SAS/SATA Common Interrupt Cause  */
	COMMON_CMPLQ_IRQN_STAT	 = 0x804, /* SAS/SATA CmplQ N Interrupt Cause */
	COMMON_CMPLQ_IRQN_MASK	 = 0x808, /* SAS/SATA CmplQ N Interrupt Enable */
	COMMON_CMPLQ_IRQ0_STAT	 = 0x810, /* SAS/SATA CmplQ 0 Interrupt Cause */
	COMMON_CMPLQ_IRQ0_MASK	 = 0x840, /* SAS/SATA CmplQ 0 Interrupt Enable */
	COMMON_IRQ_ROUTING0   = 0x880, /* SAS/SATA Interrupt Routing 0 */
};

enum sas_sata_phy_regs{
 	GENERATION_1_SETTING_0     = 0x10D,
	GENERATION_1_SETTING_1     = 0x10E,
    	GENERATION_2_SETTING_0     = 0x10F,    
    	GENERATION_2_SETTING_1     = 0x110,
    	GENERATION_3_SETTING_0     = 0x111,  
    	GENERATION_3_SETTING_1     = 0x112, 
    	GENERATION_4_SETTING_0     = 0x113, 
   	GENERATION_4_SETTING_1     = 0x114, 
};

enum odin_common_regs_bits {
	/* COMMON_PORT_TYPE (R20024H) */
	PORT_PORT_TYPE      = (0xFFFFU << 0),

	/* COMMON_CONFIG register bits (R20040H)*/
	CONFIG_CMD_TBL_BE       = (1U << 0),
	CONFIG_OPEN_ADDR_BE     = (1U << 1),
	CONFIG_RSPNS_FRAME_BE   = (1U << 2),
	CONFIG_DATA_BE          = (1U << 3),
	CONFIG_SAS_SATA_RST     = (1U << 5),
	CONFIG_RERR_FOR_UNKNWN_FIS	= (1U << 24),
	CONFIG_STP_STOP_ON_ERR	= (1U << 25),
	CONFIG_RCVD_FIS_LIST_IFC_SLCT_SHIFT = 16 ,
	CONFIG_IO_CNTXT_INTRFC_SLCT_SHIFT = 8 ,
	/* COMMON_PHY_CTRL register definition */
//	PHY_PHY_DSBL            = (0xFU << 12),
//	PHY_PWR_OFF             = (0xFU << 24),

	/* COMMON_CONTROL : port control/status bits (R20044H) */
	CONTROL_EN_CMD_ISSUE        = (1U << 0),
	CONTROL_RESET_CMD_ISSUE     = (1U << 1),
	CONTROL_ERR_STOP_CMD_ISSUE  = (1U << 3),
	CONTROL_FIS_RCV_EN          = (1U << 4),

	CONTROL_EN_SATA_RETRY       = (1U << 6),
	CONTROL_RSPNS_RCV_EN        = (1U << 7),


	CONTROL_EN_PORT_XMT_START   = 8,

	/* COMMON_DELV_Q_CONFIG (R20070H) bits */
	DELV_QUEUE_SIZE_MASK        = (0x3FFFU << 0),
	DELV_QUEUE_ENABLE           = (1U << 31),
	DELV_QUEUE_RD_PTR_SHDW_EN = (1U << 30),
	DELV_DLVRY_Q_INTRFC_SLCT_SHIFT = 16 ,
	
	/* COMMON_CMPL_Q_CONFIG (R20090H) bits */
	CMPL_QUEUE_SIZE_MASK        = (0x3FFFU << 0),
	CMPL_QUEUE_ENABLE           = (1U << 31),
	CMPL_QUEUE_DSBL_ATTN_POST	= (1U << 30),
	CMPL_QUEUE_WRT_PTR_SHDW_EN = (1U << 29),
	CMPL_CMPLT_Q_INTRFC_SLCT_SHIFT = 16 ,
	/* COMMON_COAL_CONFIG (R200B0H) bits */
	INT_COAL_COUNT_MASK      = (0x1FFU << 0),
	INT_COAL_ENABLE          = (1U << 16),

	/* COMMON_COAL_TIMEOUT (R200B4H) bits */
	COAL_TIMER_MASK          = (0xFFFFU << 0),
	COAL_TIMER_UNIT_1MS      = (1U << 16),   /* 6.67 ns if set to 0 */

	/* RE4020800h SAS/SATA Common Interrupt Cause */
	INT_CMPLQ_NOT_EMPTY				 = (1U << 0),
	INT_DLVRYQ_NOT_FULL =  (1U << 1),
	INT_SRS 				   = (1U << 4),
	INT_PORT_LAYER	   = (1U << 5),
	INT_NON_SPCFC_NCQ_ERR	   = (1U << 6),

	INT_UART_IRQ =	(1U << 8),
	INT_I2C_IRQ =  (1U << 9),
	INT_SGPIO_IRQ =  (1U << 10),
	INT_GPIO_IRQ =	(1U << 11),
	INT_SYS_ERR_IRQ =  (1U << 12),
	INT_CMD_ISSUE_STOPPED	   = (1U << 13),
	INT_MEM_PAR_ERR 		   = (1U << 14),
	INT_DP_PAR_ERR			= (1U << 15),

	INT_PORT_MASK_OFFSET	   = 16,
	INT_PORT_MASK			   = (0xFF << INT_PORT_MASK_OFFSET),
	INT_PORT_STOP_MASK_OFFSET  = 24,
	INT_PORT_STOP_MASK		   = (0xFF << INT_PORT_STOP_MASK_OFFSET),

	/* RE4020880h sas/sata interrupt routing */
	INT_MSIX_VECTOR_MASK = (0x1F << 0),

	/* Internal Memory Parity Error Enable (0x2D4) bits */
	INTER_MEM_PARITY_ERR_MASK                = 0xFFFFFFFF,
	/* Data Path Parity Control 0x2D8) bits */
	DPP_CTRL_DP_FERR_EN                 = (0x1L << 2),
	DPP_CTRL_DP_PERR_EN                 = (0x1L << 1),
	DPP_CTRL_DP_PAR_ODD                = (0x1L << 0),  // Datapath Parity Select. 0: Even Parity. 1: Odd Parity.

};


/* COMMON_DELV_Q_RD_PTR bits */
typedef struct _REG_COMMON_DELV_Q_RD_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32   Reserved:20;
	MV_U32   DELV_QUEUE_RD_PTR:12;
#else
	MV_U32   DELV_QUEUE_RD_PTR:12;
	MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_COMMON_DELV_Q_RD_PTR, *PREG_COMMON_DELV_Q_RD_PTR;


/* COMMON_CMPL_Q_WR_PTR bits */
typedef struct _REG_COMMON_CMPL_Q_WR_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32   Reserved:20;
	MV_U32   CMPLN_QUEUE_WRT_PTR:12;
#else
	MV_U32   CMPLN_QUEUE_WRT_PTR:12;
	MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_COMMON_CMPL_Q_WR_PTR, *PREG_COMMON_CMPL_Q_WR_PTR;

/* sas/sata command port registers */
enum athena_cmd_regs {
    /* command registor -- Monitor Data/Select */
    CMD_MONITOR_DATA_SELECT = 0x018C,
    /* command registor -- Application Error Configuration */
    CMD_APP_ERR_CONFIG = 0x01A4,
    /* command registor -- Pending FIFO Control 0 */
    CMD_PEND_FIFO_CTRL0 = 0x1A8,
    /* command registor -- Host Control Status */
    CMD_HOST_CTRL_STS = 0x01AC,
    /* command registor -- Host Write Data */
    CMD_HOST_WRITE_DATA = 0x01B0,
    /* command registor -- Host Read Data */
    CMD_HOST_READ_DATA = 0x01B4,
    /* command registor -- Host Address */
    CMD_HOST_ADDR = 0x01B8,
    /* command registor -- Pending FIFO Control 1 */
    CMD_PEND_FIFO_CTRL1 = 0x01C4,
    
    /* cmd port active register $i ((0x400-0x7ff) -- 1023*/
    CMD_PORT_ACTIVE0  = 0x400,

    /* SATA register set $i (0x800-0x9ff) task file data register -- 511*/
    CMD_SATA_TFDATA0  = 0x800,

    /* SATA register set $i (0xa00-0xbff) association reg -- 511 */
    CMD_SATA_ASSOC0   = 0xa00,

};
/* SAS/SATA Port Config/Control Register */
enum athena_config_regs {
	CONFIG_LED_CONTROL    = 0x10, /* LED Control */
	CONFIG_SATA_CONTROL    = 0x18, /* port SATA control register */
	CONFIG_PHY_CONTROL     = 0x1c, /* port phy control register */

	CONFIG_SATA_SIG0       = 0x20, /* port SATA signature FIS(Byte 0-3) */
	CONFIG_SATA_SIG1       = 0x24, /* port SATA signature FIS(Byte 4-7) */
	CONFIG_SATA_SIG2       = 0x28, /* port SATA signature FIS(Byte 8-11) */
	CONFIG_SATA_SIG3       = 0x2c, /* port SATA signature FIS(Byte 12-15)*/
	CONFIG_R_ERR_COUNT     = 0x30, /* port R_ERR count register */
	CONFIG_CRC_ERR_COUNT   = 0x34, /* port CRC error count register */
	CONFIG_WIDE_PORT       = 0x38, /* port wide participating register */

	CONFIG_CRN_CNT_INFO0   = 0x80, /* port current connection info register 0*/
	CONFIG_CRN_CNT_INFO1   = 0x84, /* port current connection info register 1*/
	CONFIG_CRN_CNT_INFO2   = 0x88, /* port current connection info register 2*/
	CONFIG_ID_FRAME0       = 0x100, /* Port device ID frame register 0, DEV Info*/
	CONFIG_ID_FRAME1       = 0x104, /* Port device ID frame register 1*/
	CONFIG_ID_FRAME2       = 0x108, /* Port device ID frame register 2*/
	CONFIG_ID_FRAME3       = 0x10c, /* Port device ID frame register 3, SAS Address lo*/
	CONFIG_ID_FRAME4       = 0x110, /* Port device ID frame register 4, SAS Address hi*/
	CONFIG_ID_FRAME5       = 0x114, /* Port device ID frame register 5, Phy Id*/
	CONFIG_ID_FRAME6       = 0x118, /* Port device ID frame register 6*/
	CONFIG_ATT_ID_FRAME0   = 0x11c, /* attached device ID frame register 0*/
	CONFIG_ATT_ID_FRAME1   = 0x120, /* attached device ID frame register 1*/
	CONFIG_ATT_ID_FRAME2   = 0x124, /* attached device ID frame register 2*/
	CONFIG_ATT_ID_FRAME3   = 0x128, /* attached device ID frame register 3*/
	CONFIG_ATT_ID_FRAME4   = 0x12c, /* attached device ID frame register 4*/
	CONFIG_ATT_ID_FRAME5   = 0x130, /* attached device ID frame register 5*/
	CONFIG_ATT_ID_FRAME6   = 0x134, /* attached device ID frame register 6*/
	CONFIG_PORT_SERIAL_CTRL_STS = 0x140, /* Port Serial Control / Status */
	CONFIG_BROADCAST_RECIEVED = 0x144, /* Broadcast Recieved */
	CONFIG_PORT_IRQ_STAT = 0x150, /* Port Interrupt Status */
	CONFIG_PORT_IRQ_MASK = 0x154, /* Port Interrupt Enable */
	CONFIG_SAS_CTRL0 = 0x170, /* SAS Control 0 */
	CONFIG_SAS_CTRL1 = 0x174, /* SAS Control 1 */
	CONFIG_MS_CNT_TIMER = 0x178, /* Mili-Second Count Timer */
	CONFIG_OPEN_RETRY = 0x17C, /* Open Retry */
	CONFIG_ID_TEST = 0x180, /* ID Test */
	CONFIG_PL_TIMER = 0x184, /* PL Timer */
	CONFIG_WD_TIMER = 0x188, /* WD Timer */
	CONFIG_PORT_SELECTOR_CNT = 0x18C, /* Port Selector Count */
	CONFIG_SL_MODE0 = 0x190, /* SL Mode 0 */
	CONFIG_SL_MODE1 = 0x194, /* SL Mode 1 */
	CONFIG_PORT_LAYER_TIMER1 = 0x198, /* Port Layer Timer 1 */
	CONFIG_LINK_LAYER_TIMER = 0x19C, /* Link Layer Timer */
};
	/* common port serial control/status (R140h) bits */
#define SCTRL_STP_LINK_LAYER_RESET  (1L << 0)
#define SCTRL_PHY_HARD_RESET_SEQ    (1L << 1)
#define SCTRL_PHY_BRDCST_CHNG_NOTIFY    (1L << 2)
#define SCTRL_SSP_LINK_LAYER_RESET       (1L << 3)
#define SCTRL_MIN_SPP_PHYS_LINK_RATE_MASK   (0xFL << 8),
#define SCTRL_MAX_SPP_PHYS_LINK_RATE_MASK   (0xFL << 12)
#define SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET    16
#define SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK   (0xFL << SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET)
#define SCTRL_PHY_READY_MASK              (1L << 20)

#ifdef SUPPORT_PHY_POWER_MODE
#define PMODE_EN_MASK       0x00000c00
#define PMODE_EN_SATA_PARTIAL  0x00000800
#define PMODE_EN_SATA_SLUMBER 0x00000400
#define PMODE_EN_SAS_SLUMBER  0x00000800
#define PMODE_EN_SAS_PARTIAL 0x00000400
#define PM_CNTRL_MASK       0xc0000000
#define PM_CNTRL_SATA_PARTIAL  0x80000000
#define PM_CNTRL_SATA_SLUMBER  0x40000000
#define PM_CNTRL_SAS_SLUMBER  0x80000000
#define PM_CNTRL_SAS_PARTIAL  0x40000000
#define PMODE_STATUS_MASK  0x0c000000
#endif
	/* COMMON_PORT_IRQ_STAT/MASK (R154h) bits */
#define IRQ_PHY_RDY_CHNG_MASK         (1U << 0)
#define IRQ_HRD_RES_DONE_MASK         (1U << 1)
#define IRQ_PHY_ID_DONE_MASK          (1U << 2)
#define IRQ_PHY_ID_FAIL_MASK          (1U << 3)
#define IRQ_PHY_ID_TIMEOUT            (1U << 4)
#define IRQ_HARD_RESET_RCVD_MASK      (1U << 5)
#define IRQ_PORT_SEL_PRESENT_MASK     (1U << 6)
#define IRQ_COMWAKE_RCVD_MASK         (1U << 7)
#define IRQ_BRDCST_CHNG_RCVD_MASK     (1U << 8)
#define IRQ_UNKNOWN_TAG_ERR           (1U << 9)
#define IRQ_IU_TOO_SHRT_ERR           (1U << 10)
#define IRQ_IU_TOO_LNG_ERR            (1U << 11)
#define IRQ_PHY_RDY_CHNG_1_TO_0       (1U << 12)
#define IRQ_SIG_FIS_RCVD_MASK         (1U << 16)
#define IRQ_BIST_ACTVT_FIS_RCVD_MASK  (1U << 17)
#define IRQ_ASYNC_NTFCN_RCVD_MASK     (1U << 18)
#define IRQ_UNASSOC_FIS_RCVD_MASK     (1U << 19)
#define IRQ_STP_SATA_RX_ERR_MASK      (1U << 20)
#define IRQ_STP_SATA_TX_ERR_MASK      (1U << 21)
#define IRQ_STP_SATA_CRC_ERR_MASK     (1U << 22)
#define IRQ_STP_SATA_DCDR_ERR_MASK    (1U << 23)
#define IRQ_STP_SATA_PHY_DEC_ERR_MASK (1U << 24)
#define IRQ_STP_SATA_SYNC_ERR_MASK    (1U << 25)
#define IRQ_DMA_PEX_TO    (1U << 30)
#define IRQ_PRD_BC_ERR     (1U << 31)

/*  odin_config_regs_bits */
/* CONFIG_DEV_INFO/CONFIG_ATT_DEV_INFO (ID Frame R100-R118 Attached ID Frame R11C-R134) bits */
#define	PORT_DEV_TYPE_MASK     (0x7U << 0)
#define	PORT_DEV_INIT_MASK     (0x7U << 9)
#define	PORT_DEV_TRGT_MASK     (0x7U << 17)
#define	PORT_DEV_SMP_INIT      (1U << 9)
#define	PORT_DEV_STP_INIT      (1U << 10)
#define	PORT_DEV_SSP_INIT      (1U << 11)
#define	PORT_DEV_SMP_TRGT      (1U << 17)
#define	PORT_DEV_STP_TRGT      (1U << 18)
#define	PORT_DEV_SSP_TRGT      (1U << 19)
#define	PORT_PHY_ID_MASK       (0xFFU << 24)

/* CONFIG_WIDE_PORT (R038H) bits */
#define	WIDE_PORT_PHY_MASK     (0xFF << 0) /* phy map in a wide port */

// SAS/SATA PHY Vendor Specific Register
enum athena_vsr_regs {
	VSR_IRQ_STATUS      = 0x00, /* PHY Interrupt Status*/
	VSR_IRQ_MASK        = 0x04, /* PHY Interrupt Enable*/
	VSR_PHY_CONFIG      = 0x08, /* PHY Config */
	VSR_PHY_STATUS      = 0x0C, /* PHY Status */
	VSR_PHY_COUNTER0      = 0x10, /* PHY Counter0 */
	VSR_PHY_COUNTER1      = 0x14, /* PHY Counter1 */
	VSR_EVENT_COUNTER_CTRL= 0x18, /* Event Counter Control */
	VSR_EVENT_COUNTER_SELECT= 0x1C, /* Event Counter Select */
	VSR_EVENT_COUNTER0      = 0x20, /* EVENT Counter0 */
	VSR_EVENT_COUNTER1      = 0x24, /* EVENT Counter1 */
	VSR_EVENT_COUNTER2      = 0x28, /* EVENT Counter2 */
	VSR_EVENT_COUNTER3      = 0x2C, /* EVENT Counter3 */
	VSR_ACT_LEN_CTRL           = 0x30, /* Activity LED Control */
	VSR_PHY_TIMER           = 0x34, /* PHY Timer */
	VSR_OOB_TIMER0           = 0x38, /* OOB Timer 0 */
	VSR_OOB_TIMER1           = 0x3C, /* OOB Timer 1 */
	VSR_ANALOG_TIMER           = 0x40, /* Analog Timer */
	VSR_PHY_RXDET_CONFIG0  = 0x44, /* PHY RXDET Configuration 0 */
	VSR_PHY_RXDET_CONFIG1  = 0x48, /* PHY RXDET Configuration 1 */
	VSR_EVENT_COUNTER0_TH      = 0x50, /* Event Counter 0 Threshold */
	VSR_EVENT_COUNTER1_TH      = 0x54, /* Event Counter 1 Threshold */
	VSR_EVENT_COUNTER2_TH      = 0x58, /* Event Counter 2 Threshold */
	VSR_EVENT_COUNTER3_TH      = 0x5C, /* Event Counter 3 Threshold */
	VSR_PHY_MODE_REG_0      = 0x060, /*PHY Mode Register 0*/
	VSR_PHY_MODE_REG_1	= 0x064,  /*PHY Mode Register 1*/
	VSR_PHY_MODE_REG_2	= 0x068,  /*PHY Mode Register 2*/
	VSR_PHY_MODE_REG_3	= 0x06C,  /*PHY Mode Register 3*/
#ifdef ATHENA_FPGA_WORKAROUND
	VSR_PHY_WORKARPUND_REG0	= 0x30D,
	VSR_PHY_WORKARPUND_REG1	= 0x300,
	VSR_PHY_WORKARPUND_REG2	= 0x151,
	VSR_PHY_WORKARPUND_REG3 = 0x106,
#endif
#ifdef ATHENA_A0_WORKAROUND
    VSR_A0_PHY_WORKARPUND_REG0= 0x0102,
    VSR_A0_PHY_WORKARPUND_REG1= 0x0123,
    VSR_A0_PHY_WORKARPUND_REG2= 0x0113,
    VSR_A0_PHY_WORKARPUND_REG3= 0x0114,
#endif
};

/* VSR PHY Configuration (R08)*/
#define VSR_PHY_CONFIG_RSVD_25	(0x1U<<25)
#define VSR_PM_WORKAROUND	(0x1U<<24)
#define VSR_DSBL_PHY			(0x1U<<23)
#define VSR_PASS_OOB			(0x1U<<22)
#define VSR_SATA_SPIN_UP_ENABLE	(0x1U<<21)
#define VSR_SATA_SPIN_UP_SUPPORT	(0x1U<<20)
#define VSR_TX_SCC_TYPE		(0x1U<<19)
#define VSR_TX_RQSTD_LGCL_LNK_RATE_1_5_G (0x8U<<15)
#define VSR_TX_RQSTD_LGCL_LNK_RATE_3_0_G (0x9U<<15)
#define VSR_TX_RQSTD_LGCL_LNK_RATE_6_0_G (0xaU<<15)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G1 (0x1U<<14)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G1_SSC (0x1U<<13)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G2 (0x1U<<12)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G2_SSC (0x1U<<11)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G3 (0x1U<<10)
#define VSR_TX_SPRTD_PHYSCL_SNK_RATES_G3_SSC (0x1U<<9)
#define VSR_TX_LINK_RATE_PARITY	(0x1U<<8)
#define VSR_SNW_3_SPRTD			(0x1U<<7)
#define VSR_6_0_G_SUPPORT		(0x1U<<6)
#define VSR_3_0_G_SUPPORT		(0x1U<<5)
#define VSR_1_5_G_SUPPORT		(0x1U<<4)
#define VSR_SATA_HOST_MODE	(0x1U<<3)
#define VSR_SATA_SUPPORT		(0x1U<<2)
#define VSR_SAS_SUPPORT		(0x1U<<1)
#define VSR_PHY_RESET			(0x1U<<0)
enum odin_vsr_reg_bits {
	/* VSR_IRQ_STATUS(R000h-R004h) bits */
	VSR_IRQ_PHY_TIMEOUT     = (1U << 10),
	VSR_SATA_SPIN_HOLD  = (1L<< 8),

	/* VSR_PHY_STATUS bits (R00Ch)*/
	VSR_PHY_STATUS_MASK     = 0x3f0000,
	VSR_PHY_STATUS_IDLE     = 0x00,
	VSR_PHY_STATUS_SAS_RDY  = 0x10,
	VSR_PHY_STATUS_HR_RESET= 0x15,
	VSR_PHY_STATUS_HR_RDY   = 0x1d,
	VSR_PHY_STATUS_SATA_SPIN_HOLD= 0x2c,
};

enum mv_pci_regs {
	MV_PCI_REG_CMD      = 0x04,
	MV_PCI_REG_DEV_CTRL = 0x78,
	MV_PCI_REG_MSI_CTRL = 0x90,
	MV_PCI_REG_MSIX_CTRL = 0xB0,

	MV_PCI_REG_ADDRESS_BASE = 0x8000,

	CORE_NVSRAM_MAPPING_BASE = 0x0,
	MV_PCI_REG_WIN0_CTRL = (MV_PCI_REG_ADDRESS_BASE + 0x420),
	MV_PCI_REG_WIN0_BASE = (MV_PCI_REG_ADDRESS_BASE + 0x424),
        MV_PCI_REG_WIN0_REMAP = (MV_PCI_REG_ADDRESS_BASE + 0x428),

	MV_PCI_REG_INT_CAUSE = (MV_PCI_REG_ADDRESS_BASE + 0x0),
	MV_PCI_REG_INT_ENABLE = (MV_PCI_REG_ADDRESS_BASE + 0x4),
};

enum mv_pci_regs_bits {
    	/* MV_PCI_REG_MSI_CTRL(R0x0B0h) bits */
	MV_PCI_MSIX_EN       = (1L << 31),
	/* MV_PCI_REG_MSI_CTRL(R0x090h) bits */
	MV_PCI_MSI_EN       = (1 << 16),

	/* MV_PCI_REG_DEV_CTRL(R0x004h) bits */
	MV_PCI_IO_EN        = (1 << 0),
	MV_PCI_MEM_EN       = (1 << 1),
	MV_PCI_BM_EN        = (1 << 2),    /* enable bus master */
	MV_PCI_INT_DIS      = (1 << 10),   /* disable INTx for MSI_EN */
	MV_PCI_DEV_EN       =  MV_PCI_MEM_EN | MV_PCI_BM_EN,

	/* MV_PCI_REG_DEV_CTRL (R0x078h) bits */
	MV_PCI_RD_REQ_SIZE  = 0x2000,   // MAXRDRQSTSZ 512B
	MV_PCI_RD_REQ_MASK  = 0x00007000,
};

enum mv_sgpio_regs{
	SGPIO_REG_BASE    = 0x1c200, /* SGPIO register start */
};

enum mv_i2c_regs{
	I2C_SOFTWARE_CONTROL_A = 0x1c51c,
	I2C_HARDWARE_CONTROL_A = 0x1c520,
	I2C_STATUS_DATA_A      = 0x1c524,

	I2C_SOFTWARE_CONTROL_B = 0x1c61c,
	I2C_HARDWARE_CONTROL_B = 0x1c620,
	I2C_STATUS_DATA_B      = 0x1c624,

	I2C_SOFTWARE_CONTROL_C = 0x1c71c,
	I2C_HARDWARE_CONTROL_C = 0x1c720,
	I2C_STATUS_DATA_C      = 0x1c724,

};

enum mv_spi_regs{
	ODIN_SPI_CTRL_REG = 0x1c800,
	ODIN_SPI_ADDR_REG = 0x1c804,
	ODIN_SPI_WR_DATA_REG	= 0x1c808,
	ODIN_SPI_RD_DATA_REG = 0x1c80c,
};

enum mv_spi_reg_bit{
	SPI_CTRL_READ = MV_BIT( 2 ),
	SPI_CTRL_AddrValid = MV_BIT( 1 ),
	SPI_CTRL_SpiStart = MV_BIT( 0 ),
};

/* for buzzer */
enum test_pin_regs {
	TEST_PIN_OUTPUT_VALUE = 0x10068,
	TEST_PIN_OUTPUT_ENABLE = 0x1006C,
	TEST_PIN_BUZZER		= MV_BIT(2),
};
#ifdef SUPPORT_SECURITY_KEY_RECORDS
enum crypto_management_regs {
    CRYPTO_GLOBAL_SECURITY_CONFIG = 0x24000,
    CRYPTO_GLOBAL_ZERIOZATION_CTRL = 0x24004,
    CRYPTO_R_KEK_VAULT_CTRL         = 0x24010,
    CRYPTO_R_KEK_VAULT_STS         = 0x24014,
    CRYPTO_R_KEK_INIT_VAL_LOW         = 0x24020,
    CRYPTO_R_KEK_INIT_VAL_HIGH         = 0x24024,
    CRYPTO_VAULT_PORT_CTRL         = 0x24030,
    CRYPTO_VAULT_PORT_DATA        = 0x24034,
    CRYPTO_GLOBAL_SECURITY_INT_CAUSE         = 0x24040,
    CRYPTO_GLOBAL_SECURITY_INT_MASK         = 0x24044,
};
enum crypto_management_regs_bit {
    // Global Security Configuration
    CRYPTO_BIT_USR_IV_REQUIRED = MV_BIT(1),
    CRYPTO_BIT_EN_DEK_REUSE = MV_BIT(0),
};
enum crypto_global_zeriozation_regs_bit {
    // Global zeriozation Configuration
    CRYPTO_ZERI_ZEROIZE_ALL = MV_BIT(31),
    CRYPTO_ZERI_ZEROIZE_NV_MEM = MV_BIT(30),
    CRYPTO_ZERI_ZEROIZE_CRPTO_CNTXT_MEM = MV_BIT(29),
    CRYPTO_ZERI_ZEROIZE_UNWRP_ENGN_1 = MV_BIT(28),
    CRYPTO_ZERI_ZEROIZE_UNWRP_ENGN_0 = MV_BIT(27),
    CRYPTO_ZERI_DEBUG_MODE = MV_BIT(16),
    CRYPTO_ZERI_ACCEPT = MV_BIT(4),
    CRYPTO_ZERI_SAFETY_SEQUENCE_SHIFT = 0,
};
enum crypto_int_mask_regs_bit {
    // Global Security Configuration
    CRYPTO_INT_PNDNG_KEY_NOT_VLD = MV_BIT(19),
    CRYPTO_INT_VAULT_OP_ERR = MV_BIT(17),
    CRYPTO_INT_VAULT_OP_CMPLT = MV_BIT(16),
    CRYPTO_INT_VLT_ZEROIZED = MV_BIT(10),
    CRYPTO_INT_UNWRP_ENGN_0_ZEROIZED = MV_BIT(8),
    CRYPTO_INT_JTAG_TAMPER_RSPNS = MV_BIT(2),
    CRYPTO_INT_UART_TAMPER_RSPNS = MV_BIT(1),
    CRYPTO_INT_TAMPER_PIN_RSPNS = MV_BIT(0),
};
#endif
/*
  ========================================================================
	Software data structures/macros related to 64xx
  ========================================================================
*/

#define MAX_SSP_RESP_SENSE_SIZE     sizeof(MV_Sense_Data)
#define MAX_SMP_RESP_SIZE			 1016

/*
 * Hardware related format. Never change their size. Must follow hardware
 * specification.
 */
enum _mv_command_header_bit_ops {
	CH_BIST=  (1UL << 4),
	CH_ATAPI = (1UL << 5),
	CH_FPDMA = (1UL << 6),
	CH_RESET = (1UL << 7),
	CH_PI_PRESENT = (1UL <<8),
	CH_SSP_TP_RETRY = (1UL << 9),
	CH_SSP_VERIFY_DATA_LEN = ( 1UL <<10),
	CH_SSP_FIRST_BURST = (1UL << 11),
	CH_SSP_PASS_THRU = (1UL << 12),
	CH_SSP_TLR_CTRL_SHIFT = 26,
	CH_SSP_FRAME_TYPE_SHIFT = 13,
	CH_PRD_TABLE_LEN_SHIFT = 16,
	CH_DATA_SKIIP_ENTRY_IN_PRD = (1UL << 24),
	CH_LOAD_SECURITY_KEY = (1UL << 28),
	CH_S_KEK_POSITION = (1UL << 29),
	CH_VERIFY_KEY_TAG = (1UL << 30),
	CH_PM_PORT_MASK = 0xf,
#ifndef SUPPORT_BALDUR
	CH_FRAME_LEN_MASK = 0x1ff,
#else
	CH_FRAME_LEN_MASK = 0xff,
	CH_LEAVE_AFFILIATION_OPEN_SHIFT = 9,
	CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT = 12,
#endif
	CH_MAX_RSP_FRMAE_LEN_MASK = 0x1ff,

	XBAR_CT_CS_SHIFT = 0,
	XBAR_OAF_CS_SHIFT = 4,
	XBAR_SB_CS_SHIFT = 0,
	XBAR_PRD_CS_SHIFT = 4,

	PI_BLK_INDX_FLDS_PRNST = (1UL << 13),
	PI_KEY_TAG_FLDS_PRNST = (1UL << 14),
	PI_T10_FLDS_PRNST = (1UL << 15),
};
/* for command list */
typedef struct _mv_command_header
{
	MV_U32   ctrl_nprd;
	MV_U32 frame_len:9;
	MV_U32 leave_aff_open:1;
	MV_U32 reserved4:2;
	MV_U32 max_sim_conn:4;
	MV_U32 max_rsp_frame_len:9;
	MV_U32 reserved5:7;
	MV_U16 tag;
	MV_U16 target_tag;
/* DWORD 3 */
	MV_U32   data_xfer_len;         /* in bytes */
/* DWORD 4-5*/
	MV_U32  reserved[2];
/* DWORD 6-7*/
	_MV_U64  open_addr_frame_addr;
/* DWORD 8-9*/
	_MV_U64  status_buff_addr;
	_MV_U64  prd_table_addr;
/* DWORD 12 13*/
	MV_U32	 interface_select;
//	MV_U32    security_key_interface_select;
	MV_U32	 security_key_interface_select:8;
	MV_U32	 reserved2:20;
	MV_U32	 pir_fmt:4;
/* DWORD 14-15*/
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	_MV_U64	 security_key_rec_base_addr;
#else
	MV_U32  reserved3[2];
#endif
} mv_command_header;

/* SSP_SSPFrameType sas2 spec*/
#define SSP_FRAME_FMT_FRAME_TYPE_DATA       0x01    // SSP initiator port or SSP target port
#define SSP_FRAME_FMT_FRAME_TYPE_XFER_RDY       0x05  // SSP target port
#define SSP_FRAME_FMT_FRAME_TYPE_COMMAND       0x06 // SSP initiator port
#define SSP_FRAME_FMT_FRAME_TYPE_RESPONSE      0x07 // SSP target port
#define SSP_FRAME_FMT_FRAME_TYPE_TASK      0x16     // SSP initiator port
/* SSP_SSPFrameType for vanir*/
#define FRAME_TYPE_COMMAND         0x00
#define FRAME_TYPE_TASK            0x01
#define FRAME_TYPE_XFER_RDY         0x04 /* Target mode */
#define FRAME_TYPE_RESPONSE         0x05 /* Target mode */
#define FRAME_TYPE_RD_DATA         0x06 /* Target mode */
#define FRAME_TYPE_RD_DATA_RESPONSE   0x07 /* Target mode, read data after response frame */
/* Tag */
#define SSP_I_HEADR_TAG_MASK      0xfe00   /* lower 9 bits from HW command slot */
#define SSP_T_HEADR_TAG_MASK      0xffff
#define STP_HEADR_TAG_MASK         0x001f   /* NCQ uses lower 5 bits  */
/* TargetTag */
#define SSP_T_HEADR_TGTTAG_MASK      0xfe00   /* lower 9 bits from HW command slot */

/* for command table */
/* SSP frame header */
typedef struct _ssp_frame_header
{
    MV_U8   frame_type;
    MV_U8   hashed_dest_sas_addr[3];
    MV_U8   reserved1;
    MV_U8   hashed_src_sas_addr[3];
    MV_U8   reserved2[2];
    MV_U8   changing_data_pointer:1;
    MV_U8   retransmit:1;
    MV_U8   retry_data_frame:1;
    MV_U8   tlr_control:2;
    MV_U8   reserved3:3;
    MV_U8   number_of_fill_bytes:2;
    MV_U8   reserved4:6;
    MV_U8   reserved5[4];
    MV_U16  tag;         /* command tag */
    MV_U16  target_tag;      /* Target Port Transfer Tag, for target to tag multiple XFER_RDY */
    MV_U32  data_offset;
}ssp_frame_header;

/* SSP Command UI */
#ifdef SUPPORT_VAR_LEN_CDB
typedef struct _ssp_command_iu
{
    MV_U8   lun[8];
    MV_U8   reserved1;
    MV_U8   task_attribute:3;
    MV_U8   task_priority:4;
    MV_U8   enable_first_burst:1;
    MV_U8   reserved3;
    MV_U8   reserved4:2;
    MV_U8   additional_cdb_length:6;
    MV_U8   cdb[16];
	MV_U8   e_cdb[16];
}ssp_command_iu;
#else
typedef struct _ssp_command_iu
{
    MV_U8   lun[8];
    MV_U8   reserved1;
    MV_U8   task_attribute:3;
    MV_U8   task_priority:4;
    MV_U8   enable_first_burst:1;
    MV_U8   reserved3;
    MV_U8   reserved4:2;
    MV_U8   additional_cdb_length:6;
    MV_U8   cdb[16];
}ssp_command_iu;
#endif

/* SSP TASK UI */
typedef struct _ssp_task_iu
{
	MV_U8   lun[8];
	MV_U8   reserved1[2];
	MV_U8   task_function;
	MV_U8   reserved2;
	MV_U16  tag;
	MV_U8   reserved3[14];
}ssp_task_iu;

/* SSP XFER_RDY UI */
typedef struct _ssp_xferrdy_iu
{
	MV_U32   data_offset;
	MV_U32   data_len;
	MV_U8    reserved3[4];
}ssp_xferrdy_iu;

/* SSP RESPONSE UI */
typedef struct _ssp_response_iu
{
	MV_U8   reserved1[10];
	MV_U8  data_pres;
	MV_U8   status;
	MV_U32  reserved3;
	MV_U32  sense_data_len;
	MV_U32  resp_data_len;
	MV_U8   data[MAX_SSP_RESP_SENSE_SIZE];
}ssp_response_iu;

/* DataPres */
#define NO_SENSE_RESPONSE   0x0
#define RESPONSE_ONLY      0x1
#define SENSE_ONLY         0x2
#define RESERVED         0x3

/* SSP Protection Information Record */
typedef struct _protect_info_record
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	/*DWORD 0*/
	MV_U32 USR_DT_SZ:12;
	MV_U32 ENCRYPT_T10:1;
	MV_U32 ENCR_TYPE:3;
	MV_U32 ENCR_EN:1;
    MV_U32 KEY_TAG_CHK_EN:1;
    MV_U32 LOAD_BLK_INDX:1;
    MV_U32 reserved2:1;
	MV_U32 LGB_TYPE_GEN:1;
	MV_U32 LBG_TYPE_CHK:1;
	MV_U32 LOAD_T10_FLDS:1;
	MV_U32 reserved1:1;
	MV_U32 PRD_DATA_INCL_T10:1;
	MV_U32 INCR_LBAT:1;
	MV_U32 INCR_LBRT:1;
	MV_U32 CHK_DSBL_MD:1;
	MV_U32 T10_CHK_EN:1;
	MV_U32 T10_RPLC_EN:1;
	MV_U32 T10_RMV_EN:1;
	MV_U32 T10_INSRT_EN:1;

	/*DWORD 1,2 PIR T10 Fields (Four DWords)  */
	MV_U32 LBRT_CHK_VAL;      /* Logical Block Reference Tag */
	MV_U32 LBRT_GEN_VAL;      /* Logical Block Reference Tag Gen Value*/
	/*DWORD 3*/
	MV_U32 LBAT_CHK_MASK:16;	/* Logical Block Application Tag Check Mask*/
	MV_U32 LBAT_CHK_VAL:16;      /* Logical Block Application Tag Check Value*/
	/*DWORD 4*/
	MV_U32 T10_RPLC_MSK:8;	/*T10 Replace Mask*/
	MV_U32 T10_CHK_MSK:8;	/*T10 Check Mask*/
	MV_U32 LBAT_GEN_VAL:16;      /* Logical Block Application Tag Gen Value*/
    
	/*DWORD 5,6 PIR Key Tag Fields (Two DWords)*/
	MV_U32 KEY_TAG[2];
	/*DWORD 7,8,9,10 PIR Block Index Fields (Four DWords)*/
	MV_U32 BLK_INDX[4];
	/*DWORD 7,8,9,10 Chained PIR Fields (Three DWords)*/
	MV_U32 NXT_PIR_ADDR_LO;
	MV_U32 NXT_PIR_ADDR_HI;
	MV_U32 IFC_SLCT:8;
	MV_U32 CHND_PI_FLDS_PRSNT:1;
	MV_U32 T10_FLDS_PRSNT:1;
	MV_U32 KEY_TAG_FLDS_PRSNT:1;
	MV_U32 BLK_INDX_FLDS_PRSNT:1;
	MV_U32 reserved3:4;
	MV_U32 NM_BLKS_PIR_VLD:16;
#else

	/*DWORD 0*/
	MV_U32 T10_INSRT_EN:1;
	MV_U32 T10_RMV_EN:1;
	MV_U32 T10_RPLC_EN:1;
	MV_U32 T10_CHK_EN:1;
	MV_U32 CHK_DSBL_MD:1;
	MV_U32 INCR_LBRT:1;
	MV_U32 INCR_LBAT:1;
	MV_U32 PRD_DATA_INCL_T10:1;
	MV_U32 reserved1:1;
    	MV_U32 LOAD_T10_FLDS:1;
    	MV_U32 LBG_TYPE_CHK:1;
    	MV_U32 LGB_TYPE_GEN:1;
    	MV_U32 reserved2:1;
    	MV_U32 LOAD_BLK_INDX:1;
    	MV_U32 KEY_TAG_CHK_EN:1;
    	MV_U32 ENCR_EN:1;
	MV_U32 ENCR_TYPE:3;
	MV_U32 ENCRYPT_T10:1;
	MV_U32 USR_DT_SZ:12;
	/*DWORD 1,2 PIR T10 Fields (Four DWords)  */
	MV_U32 LBRT_CHK_VAL;      /* Logical Block Reference Tag */
	MV_U32 LBRT_GEN_VAL;      /* Logical Block Reference Tag Gen Value*/
	/*DWORD 3*/
	MV_U32 LBAT_CHK_VAL:16;      /* Logical Block Application Tag Check Value*/
	MV_U32 LBAT_CHK_MASK:16;	/* Logical Block Application Tag Check Mask*/
	/*DWORD 4*/
	MV_U32 LBAT_GEN_VAL:16;      /* Logical Block Application Tag Gen Value*/
	MV_U32 T10_CHK_MSK:8;	/*T10 Check Mask*/
	MV_U32 T10_RPLC_MSK:8;	/*T10 Replace Mask*/
    
	/*DWORD 5,6 PIR Key Tag Fields (Two DWords)*/
    	MV_U32 KEY_TAG[2];
	/*DWORD 7,8,9,10 PIR Block Index Fields (Four DWords)*/
    	MV_U32 BLK_INDX[4];
	/*DWORD 7,8,9,10 Chained PIR Fields (Three DWords)*/
    	MV_U32 NXT_PIR_ADDR_LO;
    	MV_U32 NXT_PIR_ADDR_HI;
    	MV_U32 NM_BLKS_PIR_VLD:16;
    	MV_U32 reserved3:4;
    	MV_U32 BLK_INDX_FLDS_PRSNT:1;
    	MV_U32 KEY_TAG_FLDS_PRSNT:1;
    	MV_U32 T10_FLDS_PRSNT:1;
    	MV_U32 CHND_PI_FLDS_PRSNT:1;
    	MV_U32 IFC_SLCT:8;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
}protect_info_record;


/* SSP Command Table */
typedef struct _mv_ssp_command_table
{
	ssp_frame_header frame_header;
	union
	{
		struct {
			ssp_command_iu command_iu;
			protect_info_record pir;
		} command;
		ssp_task_iu task;
		ssp_xferrdy_iu xfer_rdy;
		ssp_response_iu response;
	} data;
} mv_ssp_command_table;

/* SATA STP Command Table */

typedef struct _mv_sata_stp_command_table
{
	MV_U8   fis[64];                        /* Command FIS  00h Command FIS (up to 64 bytes) */
	MV_U8   atapi_cdb[32];                     /* ATAPI CDB  40h ATAPI Command (CDB) (up to 32 bytes) */
	protect_info_record pir;            /* 60h Optional Protection Information Record */
} mv_sata_stp_command_table;

#define OF_MODE_SHIFT 7
#define OF_MODE_TARGET 0x0
#define OF_MODE_INITIATOR 0x1
#define OF_PROT_TYPE_SHIFT 4
/* Open Address Frame */
typedef struct _open_addr_frame
{
	MV_U8   frame_control; /* frame, protocol, initiator etc. */
	MV_U8   connection_rate;   /* connection rate, feature etc. */
	MV_U8   connect_tag[2];
	MV_U8   dest_sas_addr[8];
/* HW will generate Byte 12 after... */
	MV_U8   src_sas_addr[8];
	MV_U8   src_zone_grp;
	MV_U8   blocked_count;
	MV_U8   awt[2];
	MV_U8   cmp_features2[4];
	MV_U32  first_burst_size;      /* for hardware use*/
}open_addr_frame;

/* Protocol */
#define PROTOCOL_SMP      0x0
#define PROTOCOL_SSP      0x1
#define PROTOCOL_STP      0x2

/* _mv_error_record_info_type_ */
/* Error Information Record, DWord 0*/
#define BFFR_PERR               (1UL << 0)
#define WD_TMR_TO_ERR           (1UL << 1)
#define CREDIT_TO_ERR           (1UL << 2)
#define WRONG_DEST_ERR          (1UL << 3)
#define CNCTN_RT_NT_SPRTD_ERR   (1UL << 4)
#define PRTCL_NOT_SPRTD_ERR     (1UL << 5)
#define BAD_DEST_ERR            (1UL << 6)
#define BRK_RCVD_ERR            (1UL << 7)
#define STP_RSRCS_BSY_ERR       (1UL << 8)
#define NO_DEST_ERR             (1UL << 9)
#define PTH_BLKD_ERR            (1UL << 10)
#define OPEN_TMOUT_ERR          (1UL << 11)
#define CNCTN_CLSD_ERR          (1UL << 12)
#define ACK_NAK_TO              (1UL << 13)
#define NAK_ERR                 (1UL << 14)
#define INTRLCK_ERR             (1UL << 15)
#define DATA_OVR_UNDR_FLW_ERR   (1UL << 16)
//#define Reserved1             (1UL << 17)
#define UNEXP_XFER_RDY_ERR      (1UL << 18)
#define XFR_RDY_OFFST_ERR       (1UL << 19)
#define RD_DATA_OFFST_ERR       (1UL << 20)
#define TX_STOPPED_EARLY        (1UL << 22)
#define R_ERR                   (1UL << 23)
#define TFILE_ERR               (1UL << 24)
#define SYNC_ERR                (1UL << 25)
#define DMAT_RCVD               (1UL << 26)
#define UNKNWN_FIS_ERR          (1UL << 27)
#define RTRY_LMT_ERR            (1UL << 28)
#define RESP_BFFR_OFLW          (1UL << 29)
#define PI_ERR                  (1UL << 30)
#define CMD_ISS_STPD            (1UL << 31)
#define USR_BLK_NM_MASK         0xfff,
/* Protection information */
/* Protection information Error Information Record, DWord 1 */
#define REF_CHK_ERR             (1UL << 12)
#define APP_CHK_ERR             (1UL << 13)
#define GRD_CHK_ERR             (1UL << 14)
#define T10_CNTRL_ERR            (1UL << 15)
#define MSSNG_T10_ERR           (1UL << 19)       /* Missing T10 Error */
#define MSSNG_USR_IV_ERR     (1UL << 20)       /* Missing User IV Error.*/
#define KEY_LD_ERR                  (1UL << 21)       /* Key Load Error */
#define MSSNG_KEY_TAG_ERR  (1UL << 22)       /* Missing Key Tag Error. */
#define MSSNG_BLK_INDX_ERR (1UL << 23)       /* Missing Block Index Error. */
#define MSSNG_KEY_ERR           (1UL << 24)       /* Missing Key Error. */
#define KEY_TYPE_ERR              (1UL << 25)       /* Key Type Error. */
#define ENCR_CTL_ERR              (1UL << 26)       /* Encryption Control Error. */ 
#define KEY_TAG_ERR                (1UL << 27)      /* Key Tag Error. */
#define KEY_INTRGTY_ERR        (1UL << 28)       /* Key Integrity Error. */
#define KEK_NOT_VALID_ERR    (1UL << 29)       /* KEK Not Valid Error. */
#define SLOT_BSY_ERR            (1UL << 31)
/* _mv_error_record_info_type_ */

/* Error Information Record */
typedef struct _err_info_record
{
	MV_U32 err_info_field_1;
	MV_U32 err_info_field_2;
}err_info_record;


/* Status Buffer */
typedef struct _status_buffer
{
	err_info_record err_info;
	union
	{
		struct _ssp_response_iu ssp_resp;
		MV_U8   smp_resp[MAX_SMP_RESP_SIZE];
	} data;
}status_buffer;

#define MAX_RESPONSE_FRAME_LENGTH \
	(MV_MAX(sizeof(struct _ssp_response_iu), MAX_SMP_RESP_SIZE))


#ifdef SUPPORT_SECURITY_KEY_RECORDS
// Security Key Record, Type 00h (plain text, no Tag, 128 bit, XTS-AES) 8 DW
typedef struct _security_key_record_type_00h
{
    MV_U32 XTS_AES_DEK[4];
    MV_U32 XTS_AES_Tweak_Key[4];
} security_key_record_type_00h;
// Security Key Record, Type 01h (plain text, no IV, 128 bit, S-KEK) 4 DW
typedef struct _security_key_record_type_01h
{
    MV_U32 S_KEK[4];
} security_key_record_type_01h;
// Security Key Record, Type 04h (plain text, no Tag, 256 bit, XTS-AES) 16 DW
typedef struct _security_key_record_type_04h
{
    MV_U32 XTS_AES_DEK[8];
    MV_U32 XTS_AES_Tweak_Key[8];
} security_key_record_type_04h;
// Security Key Record, Type 05h (plain text, no IV, 256 bit, S-KEK) 8 DW
typedef struct _security_key_record_type_05h
{
    MV_U32 S_KEK[8];
} security_key_record_type_05h;
// Security Key Record, Type 08h (plain text, Tag, 128 bit, XTS-AES) 10 DW
typedef struct _security_key_record_type_08h
{
    MV_U32 XTS_AES_DEK[4];
    MV_U32 XTS_AES_Tweak_Key[4];
    MV_U32 key_Tag[2];
} security_key_record_type_08h;
// Security Key Record, Type 09h (plain text, IV, 128 bit, S-KEK) 6DW
typedef struct _security_key_record_type_09h
{
    MV_U32 S_KEK[4];
    MV_U32 IV[2];
} security_key_record_type_09h;
// Security Key Record, Type 0Ch (plain text, Tag, 256 bit, XTS-AES) 18 DW
typedef struct _security_key_record_type_0Ch
{
    MV_U32 XTS_AES_DEK[8];
    MV_U32 XTS_AES_Tweak_Key[8];
    MV_U32 key_Tag[2];
} security_key_record_type_0Ch;
// Security Key Record, Type 0Dh (plain text, IV, 256 bit, S-KEK) 10 DW
typedef struct _security_key_record_type_0Dh
{
    MV_U32 S_KEK[8];
    MV_U32 IV[2];
} security_key_record_type_0Dh;
// Security Key Record, Type 10h (wrapped, no Tag, 128 bit, XTS-AES) 10 DW
typedef struct _security_key_record_type_10h
{
    MV_U32 XTS_AES_DEK[10];
} security_key_record_type_10h;
// Security Key Record, Type 11h (wrapped, no IV, 128 bit, S-KEK) 6 DW
typedef struct _security_key_record_type_11h
{
    MV_U32 S_KEK[6];
} security_key_record_type_11h;
// Security Key Record, Type 14h (wrapped, no Tag, 256 bit, XTS-AES) 18 DW
typedef struct _security_key_record_type_14h
{
    MV_U32 XTS_AES_DEK[18];
} security_key_record_type_14h;
// Security Key Record, Type 15h (wrapped, no IV, 256 bit, S-KEK) 10 DW
typedef struct _security_key_record_type_15h
{
    MV_U32 S_KEK_AND_IV[10];
} security_key_record_type_15h;
// Security Key Record, Type 18h (wrapped, Tag, 128 bit, XTS-AES) 12 DW
typedef struct _security_key_record_type_18h
{
    MV_U32 XTS_AES_DEK_AND_TAG[12];
} security_key_record_type_18h;
// Security Key Record, Type 19h (wrapped, IV, 128 bit, S-KEK) 8 DW
typedef struct _security_key_record_type_19h
{
    MV_U32 S_KEK_AND_IV[8];
} security_key_record_type_19h;
// Security Key Record, Type 1Ch (wrapped, Tag, 256 bit, XTS-AES) 20 DW
typedef struct _security_key_record_type_1Ch
{
    MV_U32 XTS_AES_DEK_AND_TAG[20];
} security_key_record_type_1Ch;
// Security Key Record, Type 1Dh (wrapped, IV, 256 bit, S-KEK) 12 DW
typedef struct _security_key_record_type_1Dh
{
    MV_U32 S_KEK_AND_IV[12];
} security_key_record_type_1Dh;
typedef struct _mv_security_key_record
{
    union
    {
        security_key_record_type_00h type_00h;
        security_key_record_type_01h type_01h;
        security_key_record_type_04h type_04h;
        security_key_record_type_05h type_05h;
        security_key_record_type_08h type_08h;
        security_key_record_type_09h type_09h;
        security_key_record_type_0Ch type_0Ch;
        security_key_record_type_0Dh type_0Dh;
        security_key_record_type_10h type_10h;
        security_key_record_type_11h type_11h;
        security_key_record_type_14h type_14h;
        security_key_record_type_15h type_15h;
        security_key_record_type_18h type_18h;
        security_key_record_type_19h type_19h;
        security_key_record_type_1Ch type_1Ch;
        security_key_record_type_1Dh type_1Dh;
    }record_type;
} mv_security_key_record;

#endif

/* Command Table */
typedef struct _mv_command_table
{
	union
	{
		mv_ssp_command_table ssp_cmd_table;
		mv_smp_command_table smp_cmd_table;
		mv_sata_stp_command_table stp_cmd_table;
	} table;
	open_addr_frame   open_address_frame;
	status_buffer	  status_buff;
} mv_command_table;
#define MAX_CMD_HEADER_SIZE 0x40
typedef struct _mv_command_struct
{
    union
    {
        struct _mv_command_header mv_cmd_header;
        MV_U8   header_data[MAX_CMD_HEADER_SIZE];
    } header;
    mv_command_table mv_cmd_table;
}mv_command_struct;

enum sas_rx_tx_ring_bits {
/* RX (completion) ring bits */
	RXQ_RSPNS_GOOD		= (1U << 23),	/* Response good */
	RXQ_DEL_Q_NOT_FULL	= (1U << 22),	/* Delivery Queue not full */
	RXQ_SLOT_RST_CMPLT	= (1U << 21),	/* Slot reset complete */
	RXQ_CMD_RCVD		= (1U << 20),	/* target cmd received */
	RXQ_ATTN		= (1U << 19),	/* attention */
	RXQ_RSPNS_XFRD		= (1U << 18),	/* response frame xfer'd */
	RXQ_ERR_RCRD_XFRD	= (1U << 17),	/* err info rec xfer'd */
	RXQ_CMD_CMPLT		= (1U << 16),	/* cmd complete */
	RXQ_SLOT_MASK		= 0xfff,	/* slot number */

/* TX (delivery) ring bits */
	TXQ_MODE_I              = (1UL << 28),
	TXQ_CMD_SSP             = (1UL << 29),
	TXQ_CMD_SMP             = (2UL << 29),
	TXQ_CMD_STP             = (3UL << 29),
	TXQ_CMD_KEY_LOAD   = (6UL << 29),
	
	TXQ_REGSET_SHIFT = 20,
	TXQ_PRIORITY_SHIFT = 27,
	TXQ_PORT_SHIFT  = 8,
};

#define CMD_SSP         0x01
#define CMD_SMP         0x02
#define CMD_STP         0x03
#define CMD_SSP_TGT      0x04
#define CMD_SLOT_RESET   0x07

/* prd_slt_chain_bit */
#define PRD_CHAIN_BIT       (1UL<<23)
#define PRD_IF_SELECT_BIT   (0xFFUL<<24)
#define PRD_IF_SELECT_SHIFT 24
/* prd_slt_chain_bit */

/*prd table for vanir 3 DW*/
typedef struct _prd_t
{
	MV_U32	baseAddr_low;
	MV_U32  baseAddr_high;
	/*DW3: IF select, chain and size*/
	MV_U32 size;

} prd_t;

typedef struct _prd_context
{
	prd_t * prd;
	MV_U16 avail;
	MV_U16 reserved;
}prd_context;

typedef struct _prd_skip_t
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32 init_block_skip:16;
	MV_U32 init_block_xfer:16;
	MV_U32 sub_block_skip:16;
	MV_U32 sub_block_xfer:16;
	MV_U32 ctrl_size;
#else
	MV_U32 init_block_xfer:16;
	MV_U32 init_block_skip:16;
	MV_U32 sub_block_xfer:16;
	MV_U32 sub_block_skip:16;
	MV_U32 ctrl_size;
#endif
} prd_skip_t;
typedef struct _delv_q_context
{
    MV_U32 Delivery_Queue_Entry_DW0;
    MV_U32 Delivery_Queue_Entry_DW1;
    MV_U32 Cmd_Struct_Addr;
    MV_U32 Cmd_Struct_Addr_Hi;
} delv_q_context;

/*
  ========================================================================
	Accessor macros and functions
  ========================================================================
*/
#if 0 //def ATHENA_A0_WORKAROUND
#define READ_PORT_CONFIG_DATA(root, phy) \
        (phy->asic_id < 6) ? (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8))) : \
           (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA6 + ((phy->asic_id-6)* 8)))
#define WRITE_PORT_CONFIG_DATA(root, phy, tmp) \
    (phy->asic_id < 6) ? (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8), tmp)) : \
    (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA6 + ((phy->asic_id-6) * 8), tmp))

#define WRITE_PORT_CONFIG_ADDR(root, phy, tmp) \
    (phy->asic_id < 6) ? (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_ADDR0 + (phy->asic_id * 8), tmp)) : \
    (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_ADDR6 + ((phy->asic_id-6) * 8), tmp))

#else
#define READ_PORT_CONFIG_DATA(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8)))

#define WRITE_PORT_CONFIG_DATA(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8), tmp))

#define WRITE_PORT_CONFIG_ADDR(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_ADDR0 + (phy->asic_id * 8), tmp))
#endif
#define READ_ALL_PORT_CONFIG_DATA(root) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_ALL_CONFIG_DATA))

#define WRITE_ALL_PORT_CONFIG_DATA(root, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_CONFIG_DATA, tmp))

#define WRITE_ALL_PORT_CONFIG_ADDR(root, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_CONFIG_ADDR, tmp))

#define READ_PORT_VSR_DATA(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL_DATA0 + (phy->asic_id * 8)))

#define WRITE_PORT_VSR_DATA(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL_DATA0 + (phy->asic_id * 8), tmp))

#define WRITE_PORT_VSR_ADDR(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL0 + (phy->asic_id * 8), tmp))

#define READ_ALL_PORT_VSR_DATA(root) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL_DATA ))

#define WRITE_ALL_PORT_VSR_DATA(root, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL_DATA, tmp))

#define WRITE_ALL_PORT_VSR_ADDR(root, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL, tmp))

#define READ_SAS_REGISTER_SET_ENABLE(root, i) \
	MV_REG_READ_DWORD(root->mmio_base,(((i) > 31) ? COMMON_PORTLAYER_Q1 : COMMON_PORTLAYER_Q0))

#define WRITE_SAS_REGISTER_SET_ENABLE(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i) > 31) ? COMMON_PORTLAYER_Q1 : COMMON_PORTLAYER_Q0), tmp)

#define READ_SAS_REGISTER_SET2_ENABLE(root, i) \
	MV_REG_READ_DWORD(root->mmio_base,(((i) > 31) ? COMMON_PORTLAYER_Q3 : COMMON_PORTLAYER_Q2))

#define WRITE_SAS_REGISTER_SET2_ENABLE(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i) > 31) ? COMMON_PORTLAYER_Q3 : COMMON_PORTLAYER_Q2), tmp)

#define READ_REGISTER_SET_ENABLE(root, i) \
	MV_REG_READ_DWORD(root->mmio_base,(((i) > 31) ? COMMON_SATA_REG_SET1 : COMMON_SATA_REG_SET0))

#define WRITE_REGISTER_SET_ENABLE(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i) > 31) ? COMMON_SATA_REG_SET1 : COMMON_SATA_REG_SET0), tmp)

#define READ_REGISTER_SET2_ENABLE(root, i) \
	MV_REG_READ_DWORD(root->mmio_base,(((i) > 31) ? COMMON_SATA_REG_SET3 : COMMON_SATA_REG_SET2))

#define WRITE_REGISTER_SET2_ENABLE(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i) > 31) ? COMMON_SATA_REG_SET3 : COMMON_SATA_REG_SET2), tmp)
	
#define MV_MAX_DELV_QUEUE 0x3FFF
#define MV_MAX_CMPL_QUEUE 0x3FFF

#define READ_SAS_IRQ_STAT(root, i) \
	MV_REG_READ_DWORD(root->mmio_base, (((i)>31) ? COMMON_PORTLAYER_Q1_STAT : COMMON_PORTLAYER_Q0_STAT))

#define WRITE_SAS_IRQ_STAT(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i)>31) ? COMMON_PORTLAYER_Q1_STAT : COMMON_PORTLAYER_Q0_STAT), tmp)

#define READ_SAS_IRQ_STAT2(root, i) \
	MV_REG_READ_DWORD(root->mmio_base, (((i)>31) ? COMMON_PORTLAYER_Q3_STAT : COMMON_PORTLAYER_Q2_STAT))

#define WRITE_SAS_IRQ_STAT2(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i)>31) ? COMMON_PORTLAYER_Q3_STAT : COMMON_PORTLAYER_Q2_STAT), tmp)
	
#define READ_SRS_IRQ_STAT(root, i) \
	MV_REG_READ_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT1 : COMMON_SRS_IRQ_STAT0))

#define WRITE_SRS_IRQ_STAT(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT1 : COMMON_SRS_IRQ_STAT0), tmp)
	
#define READ_SRS_IRQ_STAT2(root, i) \
	MV_REG_READ_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT3 : COMMON_SRS_IRQ_STAT2))

#define WRITE_SRS_IRQ_STAT2(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT3 : COMMON_SRS_IRQ_STAT2), tmp)
/* Multi queue */
#define MV_DLVRYQ_OFFSET    0x20UL
#define MV_CMPLQ_OFFSET    0x20UL
#define MV_COAL_OFFSET    0x4UL
#define MV_CMPLQ_IRQ_OFFSET   0x4UL
#define MV_IRQ_ROUTING_OFFSET    0x4UL

#define READ_DLVRYQ_CONFIG(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_CONFIG + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_CONFIG(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_CONFIG + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_DLVRYQ_BASE_ADDR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_ADDR + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_BASE_ADDR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_ADDR + i*MV_DLVRYQ_OFFSET, tmp)
    	
#define READ_DLVRYQ_BASE_ADDR_HI(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_ADDR_HI + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_BASE_ADDR_HI(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_ADDR_HI + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_DLVRYQ_RD_SHADOW_ADDR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_RD_SHADOW_ADDR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_DLVRYQ_RD_SHADOW_ADDR_HI(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR_HI + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_RD_SHADOW_ADDR_HI(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR_HI + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_DLVRYQ_WR_PTR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_WR_PTR + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_WR_PTR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_WR_PTR + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_DLVRYQ_RD_PTR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR + i*MV_DLVRYQ_OFFSET)

#define WRITE_DLVRYQ_RD_PTR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q0_RD_PTR + i*MV_DLVRYQ_OFFSET, tmp)

#define READ_CMPLQ_CONFIG(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_CONFIG + i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_CONFIG(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_CONFIG + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_BASE_ADDR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_ADDR + i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_BASE_ADDR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_ADDR + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_BASE_ADDR_HI(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_ADDR_HI + i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_BASE_ADDR_HI(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_ADDR_HI + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_WR_SHADOW_ADDR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR+ i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_WR_SHADOW_ADDR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_WR_SHADOW_ADDR_HI(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR_HI+ i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_WR_SHADOW_ADDR_HI(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR_HI + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_WR_PTR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR + i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_WR_PTR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_WR_PTR + i*MV_CMPLQ_OFFSET, tmp)

#define READ_CMPLQ_RD_PTR(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPL_Q0_RD_PTR + i*MV_CMPLQ_OFFSET)

#define WRITE_CMPLQ_RD_PTR(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPL_Q0_RD_PTR + i*MV_CMPLQ_OFFSET, tmp)

#define READ_IRQ_COAL_CONFIG(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_COAL0_CONFIG + i*MV_COAL_OFFSET)

#define WRITE_IRQ_COAL_CONFIG(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_COAL0_CONFIG + i*MV_COAL_OFFSET, tmp)

#define READ_IRQ_COAL_TIMEOUT(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_COAL0_TIMEOUT+ i*MV_COAL_OFFSET)

#define WRITE_IRQ_COAL_TIMEOUT(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_COAL0_TIMEOUT + i*MV_COAL_OFFSET, tmp)

#define READ_CMPLQ_IRQ_STAT(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPLQ_IRQ0_STAT+ i*MV_CMPLQ_IRQ_OFFSET)

#define WRITE_CMPLQ_IRQ_STAT(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQ0_STAT + i*MV_CMPLQ_IRQ_OFFSET, tmp)

#define READ_CMPLQ_IRQ_MASK(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPLQ_IRQ0_MASK+ i*MV_CMPLQ_IRQ_OFFSET)

#define WRITE_CMPLQ_IRQ_MASK(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQ0_MASK + i*MV_CMPLQ_IRQ_OFFSET, tmp)

#define READ_IRQ_ROUTING(root, i) \
    	MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_ROUTING0 + i*MV_IRQ_ROUTING_OFFSET)

#define WRITE_IRQ_ROUTING(root, i, tmp) \
    	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_ROUTING0 + i*MV_IRQ_ROUTING_OFFSET, tmp)

MV_VOID mv_set_dev_info(MV_PVOID root_p, MV_PVOID phy_p);
MV_VOID mv_set_sas_addr(MV_PVOID root_p, MV_PVOID phy_p, MV_PU8 sas_addr);
MV_VOID mv_reset_phy(MV_PVOID root_p, MV_U8 logic_phy_map, MV_BOOLEAN hard_reset);

MV_VOID hal_clear_srs_irq(MV_PVOID root_p, MV_PVOID device, MV_U32 set, MV_BOOLEAN clear_all);
MV_VOID hal_enable_register_set(MV_PVOID root_p, MV_PVOID base_p);
MV_BOOLEAN core_is_register_set_stopped(MV_PVOID root_p, MV_PVOID device,MV_U8 set);
MV_VOID hal_disable_io_chip(MV_PVOID root_p);

#ifdef SUPPORT_NVSRAM
MV_LPVOID nvsram_init(MV_LPVOID register_base, MV_LPVOID memory_base);
MV_PU8 NVRAM_BASE(MV_PVOID extension);
#endif

#define hal_has_phy_int(common_irq, phy)	\
	(common_irq & \
		(MV_BIT(phy->asic_id + INT_PORT_MASK_OFFSET)))


#define hal_remove_phy_int(common_irq, phy)	\
	(common_irq & \
		~(MV_BIT(phy->asic_id + INT_PORT_MASK_OFFSET)))

MV_U32 mv_is_phy_ready(MV_PVOID root_p, MV_PVOID phy_p);
MV_U32 READ_PORT_IRQ_STAT(MV_PVOID root_p, MV_PVOID phy_p);
void WRITE_PORT_IRQ_STAT(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 value);
MV_U32 READ_PORT_IRQ_MASK(MV_PVOID root_p, MV_PVOID phy_p);
void WRITE_PORT_IRQ_MASK(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 irq_mask); 
MV_U32 READ_PORT_PHY_CONTROL(MV_PVOID root_p, MV_PVOID phy_p); 
void WRITE_PORT_PHY_CONTROL(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 value); 

#define get_phy_link_rate(phy_status)	(MV_U8)\
	(((phy_status&SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK) >> \
	SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET) + SAS_LINK_RATE_1_5_GBPS)

#ifdef SUPPORT_SGPIO
#define mv_sgpio_write_register(mmio, reg, value)		\
	 MV_REG_WRITE_DWORD(mmio, SGPIO_REG_BASE+reg, value)

#define mv_sgpio_read_register(mmio, reg, value)		\
	 value = MV_REG_READ_DWORD(mmio, SGPIO_REG_BASE+reg)
#endif
#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
#define TWSI_DEFAULT_PORT               1
#define EXT_I2C_DEV_PORT               1
#define TWSI_REG_OFF(x)                     (x * 0x100)
#define TWSI_REG_BASE(x)                   (0x1cb1c + TWSI_REG_OFF(x))
#define TWSI_MAX_PORT                       3
#define TWSI_MAX_DETECT_COUNT        3

/* ADRS_PORT values,  MI2C register address */
#define MI2C_SLAVE_ADDRESS		0x0
#define MI2C_X_SLAVE_ADDRESS	0x4
#define MI2C_DATA				0x1
#define MI2C_CONTROL			0x2
#define MI2C_STATUS				0x3 /* READ */
#define MI2C_CLOCK_CONTROL		0x3 /* WRITE */
#define MI2C_SOFT_RESET			0x7
MV_U8 read_i2c_register(MV_PVOID core_p, MV_U8 port, MV_U8 reg_index);
void write_i2c_register(MV_PVOID core_p, MV_U8 port, MV_U8 reg_index, MV_U8 reg_val);
#endif





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

//MV_VOID core_init_handlers(core_extension *core);



#if defined(SUPPORT_ROC)
#if 0
#define core_prepare_hwprd(core, source, sg)\
({                                                                      \
	register MV_U16 a;              					\
	MV_U32 sgtmp[512];              					\
	a=sgdt_prepare_hwprd((core), (source), (sgtmp), \
		((core_extension *)core)->hw_sg_entry_count, INTRFC_DDR_CS0);\
	MV_CopyMemory(sg,sgtmp,a*12);\
	a;\
})
#else
int core_prepare_hwprd(MV_PVOID core, sgd_tbl_t * source, MV_PVOID prd);
#endif

#else
int core_prepare_hwprd(MV_PVOID core, sgd_tbl_t * source, MV_PVOID prd);

#endif
MV_U8 core_get_register_set(MV_PVOID root_p, MV_PVOID device);
MV_VOID core_free_register_set(MV_PVOID root_p, MV_PVOID device, MV_U8 set);
//MV_VOID io_chip_handle_cmpl_queue_int(MV_PVOID root_p);
MV_U16 prot_set_up_sg_table(MV_PVOID root_p, MV_Request *req, MV_PVOID sg_wrapper_p);
MV_VOID core_fill_prd(MV_PVOID prd_ctx, MV_U64 bass_addr, MV_U32 size);
#define mv_reset_stp(x, y)

#ifdef SUPPORT_CONFIG_FILE
MV_BOOLEAN core_phy_get_status(MV_PVOID core_p, PMV_Request req);
MV_BOOLEAN core_phy_config(MV_PVOID core_p, PMV_Request req);
MV_BOOLEAN core_phy_test(MV_PVOID core_p, PMV_Request req);
#endif

#ifdef SUPPORT_BOARD_ALARM
MV_VOID core_alarm_enable_register(MV_PVOID core_p);
MV_VOID core_alarm_set_register(MV_PVOID core_p, MV_U8 value);
#endif
MV_VOID core_dump_common_reg(void *root_p);
MV_VOID core_dump_phy_reg(void *root_p, void *phy_p);
#endif /* __CORE_HAL_H */

