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

#ifndef CORE_GPIO_H
#define CORE_GPIO_H

#include "mv_config.h"
struct _domain_sgpio
{
	MV_U8 Data_In_Low[8];
	MV_U8 Data_In_High[8];
	MV_U32 CRC;
};

struct _sgpio_config_reg {
	MV_U8 sup_dev_cnt;
	MV_U8 gp_reg_cnt:4;
	MV_U8 cfg_reg_cnt:3;
	MV_U8 sgpio_enable:1;
	MV_U8 version:4;
	MV_U8 rsrv_byte2_bit4_7:4;
	MV_U8 rsrv_byte3;
};

typedef struct _lib_gpio {
	struct _domain_sgpio		sgpio_result;
	MV_PVOID			sgpio_cb_context;
	struct _sgpio_config_reg	sgpio_config;
	void                            (*sgpio_callback)(MV_PVOID extension, MV_PVOID context);
	MV_U8                           sgpio_sdout_inuse;
	MV_U8	                        reserved3[3];
} lib_gpio;

/*
========================================================================
ODIN chip SGPIO Defines - Register reside in PCI Configuration
========================================================================
*/

/* SGPIO Registers that are available */
#ifndef SUPPORT_ROC
#define Vendor_Unique2           0x44 // Register name used in Spec
#define SGPIO_Init               Vendor_Unique2 //Name as appeared in code
#define SGPIO_Control            0x64
#define SGPIO_Data_Out_L         0x68
#define SGPIO_Data_Out_H         0x6C
#define SGPIO_Data_In_L          0x70
#define SGPIO_Data_In_H          0x74
#endif

/* For setting SGPIO mode during initializaton */
#define SGPIO_Mode               (1<<7)

/* For setting SGPIO_Control Register */
#define SGPIO_TRANSFER_LEN(x)    ((x-1)<<4)
#define SGPIO_SELECT_SPEED(x)    (x<<1)
#define SGPIO_START_BIT          (1<<0)

/* Clk Frequency availabe for SGPIO */
#define SGPIO_1KHz               0
#define SGPIO_10KHz              1
#define SGPIO_50KHz              2
#define SGPIO_100KHz             3


/*
========================================================================
ODIN 2 chip SGPIO Defines
========================================================================
*/

/* SGPIO Registers that are available on Odin 2 -
   These are accessed through Vendor Specific Port Registers */
#define SGPIO_REG_CONFIG0            0x000
#define SGPIO_REG_CONFIG1            0x004
#define SGPIO_REG_CONFIG2            0x008
#define SGPIO_REG_CONTROL            0x00C
#define SGPIO_REG_INT_CAUSE          0x010
#define SGPIO_REG_INT_ENABLE         0x014
#define SGPIO_REG_DRV_SRC_BASE       0x020 //0x020...0x034 , Drive Source 0..23
#define SGPIO_REG_DRV_CTRL_BASE      0x038 //0x038...0x04C , Drive Control 0..23
#define SGPIO_REG_RAW_DOUT0          0x050 //DOut0..31
#define SGPIO_REG_RAW_DOUT1          0x054 //DOut32..63
#define SGPIO_REG_RAW_DOUT2          0x058 //DOut64..95
#define SGPIO_REG_RAW_DIN0           0x05C //DIn0..31
#define SGPIO_REG_RAW_DIN1           0x060 //DIn32..63
#define SGPIO_REG_RAW_DIN2           0x064 //DIn64..95
/* x=0(SGPIO0) or 1(SGPIO1) , y= SGPIO_REG_CONFIG0,... */
#define SGPIO_REG_ADDR(x,y)          (y + x*0x100)

/* SGPIO_REG_CONFIG0 */
#define SGPIO_EN           (1<<0)
#define BLINK_GEN_EN_B     (1<<1)
#define BLINK_GEN_EN_A     (1<<2)
#define INVRT_SCLK         (1<<3)
#define INVRT_SLOAD        (1<<4)
#define INVRT_SDOUT        (1<<5)
#define NEG_SLOAD_EDGE     (1<<6)
#define NEG_SDOUT_EDGE     (1<<7)
#define POS_SDIN_EDGE      (1<<8)
#define MANUAL_BIT_LEN     0x00ff0000
#define AUTO_BIT_LEN       0xff000000
#define MANUAL_BIT_LEN_OFFSET 16
#define AUTO_BIT_LEN_OFFSET 24

/* SGPIO_REG_CONFIG1 */
#define BLINK_LOW_TM_A     0x0000000f
#define BLINK_HI_TM_A      0x000000f0
#define BLINK_LOW_TM_B     0x00000f00
#define BLINK_HI_TM_B      0x0000f000
#define MAX_ACTV_ON        0x000f0000
#define FORCE_ACTV_OFF     0x00f00000
#define STRCH_ACTV_ON      0x0f000000
#define STRCH_ACTV_OFF     0xf0000000

/* SGPIO_REG_CONFIG2 */
  #define SCLK_SELECT        0x000000ff //32Hz..100kHz

/* SGPIO_REG_CONTROL */
#define SDIN_MD_MASK              (0x3<<0)
#define SDOUT_MD_MASK             (0x3<<2)
#define MANUAL_MD_REP_CNT_MASK    (0xfff<<4)
#define MANUAL_MD_SLOAD_PTRN_MASK (0xf<<16)
#define AUTO_MD_SLOAD_PTRN_MASK   (0xf<<20)
#define MAUNAL_MD_REP_CNT_OFFSET  4
enum {
        SCLK_HALT=0,
        SDOUT_MD_MANUAL=1,
        SDOUT_MD_AUTO=2,
        SDOUT_MD_MAUTO=3,
        SDIN_MD_ONCE=1,
        SDIN_MD_CONT=2,
};
#define SDOUT_MD_OFFSET      2

/* SGPIO_REG_INT_CAUSE */
#define SDIN_DONE            (1<<0)
#define MANUAL_MD_REP_DONE   (1<<1)
#define MANUAL_MD_REP_REMAIN (0xfff<<8)

/* SGPIO_REG_DRV_SRC_BASE */
#define DRV_SRC_PHY0_A        0x00
#define DRV_SRC_SAS_PORT0_A   0x08
#define DRV_SRC_SAS_PORT4_A   0x10
#define DRV_SRC_PM_PHY0_A     DRV_SRC_SAS_PORT0_A

  /* x=0(A) or 1(B), y=phy number 0..3 */
#define DRV_SRC_PHY(x,y)              (DRV_SRC_PHY0_A+y+x*4)
  /* x=0(A) or 1(B), y=phy number 0..3 */
#define DRV_SRC_SAS_PORT0_3(x,y)      (DRV_SRC_SAS_PORT0_A+y+x*4)
  /* x=0(A) or 1(B), y=phy number 4..7 */
#define DRV_SRC_SAS_PORT4_7(x,y)      (DRV_SRC_SAS_PORT4_A+y+x*4)
  /* x=0(A) or 1(B), y=phy number 0..3, z=PM port id */
#define DRV_SRC_PM_PHY(x,y,z)         (DRV_SRC_PM_PHY0_A+y+(x*4)+(z*8))

/* SGPIO_REG_DRV_CTRL_BASE */
#define MAX_AUTO_CTRL_DWORD 6

#define DRV_ACTV_LED_MASK  (0x7<<5)
#define DRV_LOC_LED_MASK   (0x3<<3)
#define DRV_ERR_LED_MASK   0x7
#define DRV_ACTV_LED_OFFSET 5
#define DRV_LOC_LED_OFFSET  3
#define DRV_ERR_LED_OFFSET  0
enum {
	LED_LOW           = 0,
	LED_HI            = 1,
	LED_BLINK_A       = 2,
	LED_BLINK_INVRT_A = 3, //above for loc 0..3
	LED_BLINK_SOF     = 4, //only on active
	LED_BLINK_EOF     = 5, //only on active
	LED_BLINK_B       = 6,
	LED_BLINK_INVRT_B = 7,
};

/* SGPIO_REG_RAW_DIN0 SGPIO_REG_RAW_DOUT0 */
#define MAX_SDIN_DWORD 3
#define MAX_SDOUT_DWORD 3
#define SDIN_BACK_PLAN_PRESENCE_PATTERN  	0x492
#define SDIN_DATA_MASK						0xFFF
#define SDIN_DEVICE0_PRESENCE_PATTERN		0x3
#define SDIN_DEVICE1_PRESENCE_PATTERN		0x18
#define SDIN_DEVICE2_PRESENCE_PATTERN		0xC0
#define SDIN_DEVICE3_PRESENCE_PATTERN		0x600
/* application */

enum {
	REG_TYPE_CONFIG = 0,
	REG_TYPE_RX = 1,
	REG_TYPE_RX_GP = 2,
	REG_TYPE_TC = 3,
	REG_TYPE_TC_GP = 4,
};

/* functions */
void sgpio_initialize(MV_PVOID This);
void sgpio_isr(MV_PVOID This, MV_U32 sgpio);
MV_U8 Core_SGPIO_set_LED(MV_PVOID extension, MV_U16 device_id, MV_U8 light_type, MV_U8 light_behavior, MV_U8 flag);
#if !defined(SUPPORT_ROC)
void sgpio_sendsgpioframe(MV_PVOID This, MV_U32 value);
#endif
#ifndef SUPPORT_ROC
MV_U32 sgpio_read_pci_register(MV_PVOID core_p, MV_U8 reg_address);
void sgpio_write_pci_register(MV_PVOID core_p, MV_U8 reg_address, MV_U32 reg_value);

#endif
#endif
