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

#if !defined(CORE_BBU_H)
#define CORE_BBU_H

#ifdef SUPPORT_BBU
#include "mvCpuIfRegs.h"

typedef enum _twsi_addr_type
{
    ADDR7_BIT,                      /* 7 bit address    */
    ADDR10_BIT                      /* 10 bit address   */
}twsi_addr_type;

/* This structure describes TWSI address.                                   */
typedef struct _twsi_addr
{
    MV_U32              address;    /* address          */
    twsi_addr_type   type;       /* Address type     */
}twsi_addr;

/* This structure describes a TWSI slave.                                   */
typedef struct _twsi_slave
{
    twsi_addr	slave_addr;
    MV_U32		offset;		/* offset in the slave.					*/
    MV_BOOLEAN 		valid_offset;		/* whether the slave has offset (i.e. Eeprom  etc.) 	*/
    MV_BOOLEAN		more_than_256;	/* whether the ofset is bigger then 256 		*/
    MV_U8 reserved[2];
    MV_U16          xfer_count;
    MV_U16          xfer_length;
}twsi_slave;
#endif

typedef struct _lib_bbu {	
#ifdef SUPPORT_BBU
	MV_U16      i2c_state;
	MV_U16       bbu_state;
	twsi_slave bbu_addr;
#else
	MV_U32 dummy;
#endif
} lib_bbu;

#ifdef SUPPORT_BBU
/* bbu_device_statemachine state */
#define BBU_STATE_INIT                                0x00
#define BBU_STATE_GET_STATUS                  0x01
#define BBU_STATE_CHECK_STATUS              0x02
#define BBU_STATE_RESET                             0x03
#define BBU_STATE_ERROR                             0x0F
/* This enumerator describes TWSI protocol commands.                        */
typedef enum _mvTwsiCmd
{
    MV_TWSI_WRITE,   /* TWSI write command - 0 according to spec   */
    MV_TWSI_READ   /* TWSI read command  - 1 according to spec */
}MV_TWSI_CMD;

#endif
#define  I2C_BBU_LINK_RATE        100*1000          /* 100 kbps   */
#define I2C_CFG_TCLK			ROC_T_CLOCK	/* Default Tclk 1663MHz */

/*
The following is gas gauge IC definition
*/

/* BBU address */
#define BBU_SLAVE_ADDR              0xAA /* TI bq27000 gas gauge IC */
#define RTC_SLAVE_ADDR              0xD0 /* TI bq27000 gas gauge IC */

/* BBU gas gauge IC bp27000 register map */

/* Device Control Register */
#define BBU_REG_CTRL                 0x00
/* Device Mode Register */
#define BBU_REG_MODE                 0x01
/* At-Rate Register */
#define BBU_REG_AR_LO                0x02
#define BBU_REG_AR_HI                0x03
/* At-Rate Time-to-Empty Register */
#define BBU_REG_ARTTE_LO             0x04
#define BBU_REG_ARTTE_HI             0x05
/* Reported Temperature Register */
#define BBU_REG_TEMP_LO              0x06
#define BBU_REG_TEMP_HI              0x07
/* Reported Voltage Register */
#define BBU_REG_VOLT_LO              0x08
#define BBU_REG_VOLT_HI              0x09
/* Status Flags Register */
#define BBU_REG_FLAGS                0x0A
/* Relative State-of-Charge Register */
#define BBU_REG_RSOC                 0x0B
/* Nominal Available Capacity Register */
#define BBU_REG_NAC_LO               0x0C
#define BBU_REG_NAC_HI               0x0D
/* Discharge Compensated NAC Register */
#define BBU_REG_CACD_LO              0x0E
#define BBU_REG_CACD_HI              0x0F
/* Temperature Compensated CACD NAC Register */
#define BBU_REG_CACT_LO              0x10
#define BBU_REG_CACT_HI              0x11
/* Last Measured Discharge Register */
#define BBU_REG_LMD_LO             0x12
#define BBU_REG_LMD_HI             0x13
/* Average Current Register */
#define BBU_REG_AI_LO               0x14
#define BBU_REG_AI_HI               0x15
/* Time-to-Empty Register */
#define BBU_REG_TTE_LO              0x16
#define BBU_REG_TTE_HI              0x17
/* Time-to-Full Register */
#define BBU_REG_TTF_LO              0x18
#define BBU_REG_TTF_HI              0x19
/* Standby Current Register */
#define BBU_REG_SI_LO                 0x1A
#define BBU_REG_SI_HI                 0x1B
/* Standby Time-to-Empty Register */
#define BBU_REG_STTE_LO             0x1C
#define BBU_REG_STTE_HI             0x1D
/* Max Load Current Register */
#define BBU_REG_MLI_LO                0x1E
#define BBU_REG_MLI_HI                0x1F
/* Max Load Time-to-Empty Register */
#define BBU_REG_MLTTE_LO            0x20
#define BBU_REG_MLTTE_HI             0x21
/* Available Energy Register */
#define BBU_REG_SAE_LO                 0x22
#define BBU_REG_SAE_HI                 0x23
/* Available Power Register */
#define BBU_REG_AP_LO                  0x24
#define BBU_REG_AP_HI                  0x25
/* Time-to-Empty At Constant Power Register */
#define BBU_REG_TTECP_LO             0x26
#define BBU_REG_TTECP_HI             0x27
/* Cycle Count Since Learning Cycle Register */
#define BBU_REG_CYCL_LO               0x28
#define BBU_REG_CYCL_HI               0x29
/* Cycle Count Total Register */
#define BBU_REG_CYCT_LO               0x2A
#define BBU_REG_CYCT_HI                0x2B
/* Compensated State-of-Charge Register */
#define BBU_REG_CSOC_HI                0x2C

/* battery upper bound and lower bound */
#define BBU_CAPACITY_FULL                       100
#define BBU_CAPACITY_CHARGE_BOUND        90
#define BBU_CAPACITY_STOP_DISCHARGE_BOUND        20
#define BBU_CAPACITY_EMPTY                     0
#define BBU_TEMP_UPPER_BOUND        50
#define BBU_TEMP_LOWER_BOUND        0
#define BBU_VOLT_UPPER_BOUND        4250 //mV
#define BBU_VOLT_LOWER_BOUND        3000 //mV

/* battery status threshold */
#define BBU_WARNING_THRESHOLD               0.1	/* over or under 10% */
#define BBU_ERROR_THRESHOLD                    0.2	/* over or under 20% */
#if defined(SUPPORT_TWSI)
void bbu_initialize(
        MV_PVOID core);
#else
MV_BOOLEAN bbu_state_machine(
	MV_PVOID core_p);
#endif /*!SUPPORT_TWSI*/
#endif /* defined(CORE_BBU_H) */
