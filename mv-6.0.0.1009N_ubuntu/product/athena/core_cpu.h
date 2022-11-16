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

#ifndef __CORE_CPU_H
#define __CORE_CPU_H

#include "mv_config.h"

#if defined( _OS_LINUX)
#define MV_PCI_BAR                              0//4
#define FLASH_BAR_NUMBER                        0
#elif defined(_OS_WINDOWS) || defined(_OS_FIRMWARE)
#define MV_PCI_BAR                              2
#define FLASH_BAR_NUMBER                        2
#endif

#define MV_PCI_BAR_IO                           5
/*
enum cpu_regs {
	CPU_MAIN_INT_CAUSE_REG        = 0x2D234,
	CPU_MAIN_IRQ_MASK_REG         =  0x2D238,
};
*/
enum general_purpose_regs {
	GP_CHIP_ID                           =  0x2C004,        /* Chip ID */
	GP_MEM_CONFIG                   =  0x2C00C,        /* Memory Configuration */
	GP_PLL0_CRTL                        =  0x2C010,        /* PLL 0 Control (SAS/SATA) */
	GP_PLL0_MODE                       =  0x2C014,        /* PLL 0 Mode (SAS/SATA) */
	GP_ANALOG_CRTL                       =  0x2C030,        /* Analog Control */
	GP_PAD_CRTL                             =  0x2C034,        /* Pad Control */
	GP_MONITOR_PORT_ADDR          =  0x2C044,        /* Monitor Port Address*/
	GP_MONITOR_SELECT                    =  0x2C048,        /* Monitor Select */
	GP_GPIO_INT_SRC                       =  0x2C04C,        /* GPIO Interrupt Source */
	GP_GPIO_INT_MASK                       =  0x2C050,        /* GPIO Interrupt Enable*/
	GP_TEST_PIN_INPUT_VAL             =  0x2C060,        /* Test Pin Input Values*/
	GP_TEST_PIN_LATCH_INPUT_VAL =  0x2C064,        /* Test Pin Latched Input Values*/
	GP_TEST_PIN_OUTPUT_VAL            =  0x2C068,        /* Test Pin Output Values*/
	GP_TEST_PIN_OUTPUT_EN              =  0x2C06C,        /* Test Pin Output Enable*/
	GP_GPIO_ACT_CINFIG                     =  0x2C070,        /* GPIO Active Configuration*/
	GP_GPIO_ACT_OUTPUT_VAL            =  0x2C074,        /* GPIO Active Output Values*/
	GP_GPIO_ACT_OUTPUT_EN              =  0x2C078,        /* GPIO Active Output Enable*/
	GP_GPIO_ACT_INPUT                       =  0x2C07C,        /* GPIO Active Input*/
	GP_GPIO_FAULT_CONFIG                =  0x2C080,        /* GPIO Fault Configuration*/
	GP_GPIO_FAULT_OUTPUT_VAL        =  0x2C084,        /* GPIO Fault Output Values*/
	GP_GPIO_FAULT_OUTPUT_EN          =  0x2C088,        /* GPIO Fault Output Enable*/
	GP_GPIO_FAULT_INPUT                   =  0x2C08C,        /* GPIO Fault Input*/
	GP_GPIO_PIN_OUTPUT_VAL            =  0x2C090,        /* GPIO Pin Output Values*/
	GP_GPIO_PIN_OUTPUT_EN              =  0x2C094,        /* GPIO Pin Output Enable*/
	GP_GPIO_PIN_INPUT_EN                 =  0x2C098,        /* GPIO Pin Input Enable*/
	GP_SAMPLE_AT_RESET                      =  0x2C100,        /* Sample At Reset*/
	GP_CHIP_CONFIG                             =  0x2C104,      /* Chip Configuration*/
	GP_RESET_REQ                                 =  0x2C108,    /* Reset Request */
	GP_RESET_ENABLE0                    =  0x2C10C,  // 10:0 CHIP_RST_EN0 /* Reset Enable 0*/
	GP_RESET_ENABLE1                     =  0x2C110, // 10:0 PCIE_RST_EN1 /* Reset Enable 1 */
	GP_RESET_ENABLE3                     =  0x2C118, // 10:0CHIP_RST_EN3 /* Reset Enable 3 */
	GP_RESET_OCCUR                       =  0x2C120,  /* Reset Occurred*/
	GP_BIST_FINISH                        =  0x2C200,  /* BIST Finish*/
	GP_BIST_FAIL                            =  0x2C204,  /* BIST Fail*/
	GP_BIST_EN                               =  0x2C208,  /* BIST Enable*/
	GP_CLOCK_EN                             =  0x2C20C,  /* Clock Enable*/
	GP_DRO_COUNTER0                    =  0x2C800,  /* DRO Counter 0*/
	GP_DRO_COUNTER1                    =  0x2C804,  /* DRO Counter 1*/
	GP_TCM_CTRL                            =  0x2CA00,  /* TCM Control*/
	GP_TCM_STS0                            =  0x2CA04,  /* TCM Status 0*/
	GP_TCM_STS1                            =  0x2CA08,  /* TCM Status 1*/
	GP_TCM_STS2                            =  0x2CA0C,  /* TCM Status 2*/
	GP_TEST_0                                =  0x2C900,  /* Test 0*/
	GP_TEST_1                                =  0x2C904,  /* Test 1*/
	GP_TEST_2                                =  0x2C908,  /* Test 2*/
	GP_TEST_3                                =  0x2C90C,  /* Test 3*/
	GP_TEST_4                                =  0x2C910,  /* Test 4*/
	GP_TEST_5                                =  0x2C914,  /* Test 5*/
	GP_TEST_6                                =  0x2C918,  /* Test 6*/
	GP_TEST_7                                =  0x2C91C,  /* Test 7*/
};
enum gp_regs_maps {
	GP_RESET_REQ_POR_RST = (1U << 0),
	GP_RESET_GP_WD_RST = (1U << 1),
	GP_RESET_PCIE_P0_FND_RST = (1U << 2),
	GP_RESET_PCIE_P1_FND_RST = (1U << 3),
	GP_RESET_PCIE_P2_FND_RST = (1U << 4),
	GP_RESET_SW_RST_0 = (1U <<5),
	GP_RESET_SW_RST_1 = (1U <<6),
	GP_RESET_CPU_0_WD_RST = (1U <<7),
	GP_RESET_CPU_1_WD_RST = (1U <<8),
	GP_RESET_CPU_2_WD_RST = (1U <<9),
	GP_RESET_CPU_3_WD_RST = (1U <<10),
	GP_RESET_ENABLE0_RST_OUT_EN_SHFT = 16,
	GP_RESET_ENABLE0_CHIP_RST_EN            = 0,
	GP_RESET_ENABLE1_PCIE_P1_RST_EN_SHFT = 16,
	GP_RESET_ENABLE1_PCIE_P0_RST_EN_EN = 0,
};
enum cpu_regs {
	CPU_MAIN_INT_CAUSE_REG        = 0x2D274,
	CPU_MAIN_IRQ_MASK_REG         =  0x2D278, 
	CPU_FAST_IRQ_MASK_REG         =  0x2D27C, 
};

enum soc_regs {
	SOC_ACCESS_INDEX_ADDR        = 0x8030,
	SOC_ACCESS_INDEX_DATA        = 0x8034,
};
#ifdef ATHENA_A0_WORKAROUND
enum pcie_regs {
	ATHENA_A0_WORKAROUND_REG0        = 0x480C,
};
#endif
enum msix_ctrl_regs {
	MSIX_BASE_ADDR                          = 0x30000,
	MSIX_PEND_BIT_ARY_0                 = 0x30000,
	MSIX_SET_PEND_BIT_ARY_0         =  0x30004, 
	MSIX_RESET_PEND_BIT_ARY_0     =  0x30008, 
	MSIX_PEND_BIT_ARY_1                 = 0x30010,
	MSIX_SET_PEND_BIT_ARY_1         =  0x30014, 
	MSIX_RESET_PEND_BIT_ARY_1     =  0x30018, 
	MSIX_MASK_BIT_SHADOW0           =  0x30050, 
	MSIX_MASK_BIT_SHADOW1           =  0x30054, 
	MSIX_MISCELLANEOUS_CTRL         =  0x30060, 
	MSIX_STATE_MACHINE_CTRL         =  0x30064, 
	MSIX_IRQ_CAUSE                           =  0x30080, 
	MSIX_IRQ_NASK                            =  0x30084, 
	MSIX_ERR_CTRL                             =  0x30090, 
	MSIX_ERR_STATS                            =  0x30094, 
	MSIX_AXI_ERR_INF                        =  0x30098, 
	MSIX_AXI_ERR_ADDR_LO                =  0x3009c, 
	MSIX_AXI_ERR_ADDR_HI                =  0x300A0, 
	MSIX_DMA_PAUSED                         =  0x300D0, 
	MSIX_DMA_PAUSED_EN                   =  0x300D4, 
};
enum MSIX_regs_maps {
// RE4030064h 80000000h State Machine Control
	MSIX_STATE_MACHINE_CTRL_MSIX_RST       = (1U << 31),
	MSIX_STATE_MACHINE_CTRL_MSIX_SM_EN    = (1U << 30),
// RE4030080h 00000000h MSI-X Interrupt Cause // RE4030084h 00000000h MSI-X Interrupt Enable
	MSIX_IRQ_AHB_SRAM_ERR_OCCRD       = (1U << 20),
	MSIX_IRQ_MSIX_DMA_PAUSED            = (1U << 19),
	MSIX_IRQ_MSIX_AXI_ERR                   = (1U << 17),
	MSIX_IRQ_MSIX_INT_MEM_PERR        = (1U << 16),
	MSIX_STATE_MACHINE_CTRL_MSG_EN_F1    = (1U << 1),
	MSIX_STATE_MACHINE_CTRL_MSG_EN_F0    = (1U << 0),

};
/* CPU_MAIN_INT_CAUSE_REG (R2D274H) bits */
#define	INT_MAP_PCIE_ERR            (1U << 31)
#define	INT_MAP_SRAM_ERR             (1U << 30)
//#define	INT_MAP_COM_ERR             (1U << 29)
//#define	INT_MAP_CRYPTO_MGMT          (1U << 28)
#define	INT_MAP_GPIO               (1U << 27)
#define	INT_MAP_SGPIO               (1U << 26)
#define	INT_MAP_TWSI                (1U << 25)      //I2C
//#define	INT_MAP_PFLASH              (1U << 24)
#define	INT_MAP_UART                (1U << 23)
#define	INT_MAP_EFUSE_IRQ          (1U << 22)
//#define	INT_MAP_CPU_CRTL            (1U << 21)
//#define	INT_MAP_BRIDGE              (INT_MAP_CPU_CRTL)
//#define	INT_MAP_P2_PCIE_LINKUP (1U << 20)
//#define	INT_MAP_P1_PCIE_LINKUP (1U << 19)
//#define	INT_MAP_P0_PCIE_LINKUP (1U << 18)
//#define	INT_MAP_P2_PCIE_ROOT (1U << 17)
//#define	INT_MAP_P1_PCIE_ROOT (1U << 16)
//#define	INT_MAP_P0_PCIE_ROOT (1U << 15)
//#define	INT_MAP_E_DMA               (1U << 14)
#define	INT_MAP_SASINTB             (1U << 13)
#define	INT_MAP_SASINTA             (1U << 12)
#define	INT_MAP_SAS                 (INT_MAP_SASINTA | INT_MAP_SASINTB)
#define	INT_MAP_MSIX                (1U << 10)
#define	INT_MAP_XOR_1             (1U << 9)
#define	INT_MAP_XOR_0             (1U << 8)
#define	INT_MAP_XOR                 (INT_MAP_XOR_0 | INT_MAP_XOR_1)

//#define	INT_MAP_DL_CPU2PCIE0        (1U << 7)      // PCIE_DRBL

//#define	INT_MAP_DL_PCIE02CPU        (1U << 6)       // CPU_DRBL
//#define	INT_MAP_I2O_HIU_PF_CPU_HOST_IRQ (1U << 5)
//#define	INT_I2O_HIU_VF_HOST_IRQ (1U << 4)
//#define	INT_I2O_HIU_VF_CPU_IRQ  (1U << 3)
//#define	INT_MAP_I2O_MDMA_IRQ     (1U << 2)
//#define	INT_MAP_COM0OUT             (1U << 1)
//#define	INT_MAP_COM0IN              (1U << 0)
//#define	INT_MAP_COM0INT             (INT_MAP_COM0IN | INT_MAP_COM_ERR)
//#define	INT_MAP_COMINT              (INT_MAP_COM0INT)
//#define	INT_MAP_MU                  (INT_MAP_DL_PCIE02CPU | INT_MAP_COM0INT)
/* CPU_MAIN_INT_CAUSE_REG (R2D274H) bits */

#define	INTRFC_SRAM                     0x02UL
#define	INTRFC_PCIE_HOST_MEM            0xC0UL
#define	INTRFC_PCIEA            INTRFC_PCIE_HOST_MEM

/* Interface Select-- Open Address Frame 8-15, Status Buffer 16-23, PRD Table 24-31*/
#ifdef RUN_AS_PCIE_DRIVER
#define CS_INTRFC_CORE_DMA ( INTRFC_PCIEA << 8 | INTRFC_PCIEA << 16 | INTRFC_PCIEA << 24) 
#define CS_INTRFC_SECURITY_KEY ( INTRFC_PCIEA ) 
#else
#define CS_INTRFC_CORE_DMA ( INTRFC_CORE_DMA << 8 | INTRFC_CORE_DMA << 16 | INTRFC_CORE_DMA << 24)
#define CS_INTRFC_SECURITY_KEY ( INTRFC_CORE_DMA ) 
#endif

#endif /* __CORE_CPU_H */

