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

#if !defined(CORE_TWSI_H)
#define CORE_TWSI_H

#if defined(SUPPORT_TWSI)
#include "mvCpuIfRegs.h"

#define TWSI_FEATURE_ADDR7_BIT MV_BIT(0) /*TWSI_FEATURE_ADDR10_BIT: 0 TWSI_FEATURE_ADDR7_BIT: 1*/
#define TWSI_FEATURE_POLLING_MODE MV_BIT(1) /*TWSI_FEATURE_INT_MODE: 0 TWSI_FEATURE_POLLING_MODE: 1*/
#define TWSI_FEATURE_WITHOUT_NACK MV_BIT(7)

/*Some Devices need the NACK signal, but some do not like SES Device.*/
#define TWSI_NORMAL_DEVICE	0
#define TWSI_SES_DEVICE		1
#ifdef EXT_I2C_DEV_PORT
#undef EXT_I2C_DEV_PORT
#define EXT_I2C_DEV_PORT 0
#endif

typedef struct _twsi_device_info_t {
	MV_U8 port_id;
	MV_U8 feature;
	MV_U16 slave_target;

	MV_U16 link_rate; /*speed*/
	MV_U16 dev_type;

	char *name;
} twsi_device_info_t;

typedef struct _twsi_device_setting
{
	List_Head		queue_pointer;
	MV_PVOID	ptr_device_info;

	MV_U8           id;
	MV_U8	       reserved0[3];
} twsi_device_setting_t;

#define TWSI_DATA_MAX_SLOTS	5
typedef struct _twsi_data_slot
{
	MV_PVOID	addr;

	MV_U16		size;
	MV_U16		flag;
} twsi_data_slot_t;

/* This structure describes a TWSI slave.                                   */
typedef struct _twsi_port_setting
{
	MV_U8           twsi_state;
	MV_U8           id;
	MV_U16         link_rate;

	MV_U8	       feature;
	MV_U8	       current_slot_num;
	MV_U8	       max_slot_num;
	MV_U8           reserved0[1];

	MV_U16		timer_num;
	MV_U16		slave_host;

	MV_U16		address;
	MV_U16		xfer_count;

	twsi_data_slot_t	slot[TWSI_DATA_MAX_SLOTS];

	MV_PVOID	current_working_device;

	PMV_Request	running_req; /*Now only support one request.*/
	List_Head		waiting_queue;
} twsi_port_setting_t;

#define TWSI_MAX_DEVICE_NUMBER 10
typedef struct _lib_twsi {	
#ifdef SUPPORT_TWSI
	MV_PVOID searching_devices_list; /*It will detect the devices on the list when power on first time.*/
	List_Head device_list;

	MV_U8 major_state;
	MV_U8 ports_num;
	MV_U8 devices_num;
	MV_U8 detecting_cnt;

	twsi_port_setting_t port[TWSI_MAX_PORT];
	
	twsi_device_setting_t devices_source[TWSI_MAX_DEVICE_NUMBER];
	twsi_device_setting_t *device[TWSI_MAX_DEVICE_NUMBER];
#else
	MV_U32 dummy;
#endif
} lib_twsi;

#define IS_TWSI_IDLE_SEQUENCE(x) (x == 0x00)
 /*Bit(0)=1: Write Bit(0)=0: Read*/
#define TWSI_COMMAND_SEQUENCE_BIT 0
#define IS_TWSI_COMMAND_WRITE_SEQUENCE(x) (x & MV_BIT(TWSI_COMMAND_SEQUENCE_BIT))
#define IS_TWSI_COMMAND_READ_SEQUENCE(x) ((x & MV_BIT(TWSI_COMMAND_SEQUENCE_BIT)) == 0x00) 
#define SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(x) x |= MV_BIT(TWSI_COMMAND_SEQUENCE_BIT)
#define SET_TO_TWSI_READ_COMMAND_SEQUENCE(x) x &= ~ MV_BIT(TWSI_COMMAND_SEQUENCE_BIT)

 /*Bit(1)=1: ADDRESS Bit(1)=0: DATA*/
#define TWSI_TRANSMIT_SEQUENCE_BIT 1
#define IS_TWSI_TRANSMIT_ADDRESS_SEQUENCE(x) (x & MV_BIT(TWSI_TRANSMIT_SEQUENCE_BIT))
#define IS_TWSI_TRANSMIT_DATA_SEQUENCE(x) ((x & MV_BIT(TWSI_TRANSMIT_SEQUENCE_BIT)) == 0x00)
#define SET_TO_TWSI_TRANSMIT_ADDRESS_SEQUENCE(x) x |= MV_BIT(TWSI_TRANSMIT_SEQUENCE_BIT)
#define SET_TO_TWSI_TRANSMIT_DATA_SEQUENCE(x) x &= ~ MV_BIT(TWSI_TRANSMIT_SEQUENCE_BIT)

 /*Bit(1)=1: ADDRESS Bit(1)=0: DATA*/
#define TWSI_RUNNING_SEQUENCE_BIT 7
#define IS_TWSI_RUNNING_SEQUENCE(x) (x & MV_BIT(TWSI_RUNNING_SEQUENCE_BIT))
#define SET_TO_TWSI_RUNNING_SEQUENCE(x) x |= MV_BIT(TWSI_RUNNING_SEQUENCE_BIT)
#define CLR_TO_TWSI_RUNNING_SEQUENCE(x) x &= ~ MV_BIT(TWSI_RUNNING_SEQUENCE_BIT)

/* TWSI_device_statemachine step */
enum TWSI_DEVICE_STATEMACHINE_STEP {
	TWSI_DEVICES_STATEMACHINE_INIT = 0x00,
	TWSI_DEVICES_STATEMACHINE_START,
	TWSI_DEVICES_STATEMACHINE_STARTED,
};

enum 
{
/* twsi state */
	TWSI_STATE_TIMING_INIT  = 0x10,
	TWSI_STATE_TIMING_SET,
	TWSI_STATE_START, /*0x12*/
	TWSI_STATE_START_ACK,
	TWSI_STATE_SLAVE_SET, /*0x14*/
	TWSI_STATE_SLAVE_SET_ACK,
	TWSI_STATE_SLAVE_SET_RDWT, /*0x16*/
	TWSI_STATE_SLAVE_SET_RDWT_ACK,
	TWSI_STATE_ADDR0_SET, /*0x18*/
	TWSI_STATE_ADDR1_SET,
	TWSI_STATE_ADDR_SET_ACK, /*0x1A*/
	TWSI_STATE_DATA_RECEIVE, 
	TWSI_STATE_STOP, /*0x1C*/
	TWSI_STATE_TIMEOUT,
	TWSI_STATE_FAIL, /*0x1E*/
	TWSI_STATE_FINISH,
	TWSI_STATE_NACK, /*0x20*/
	TWSI_STATE_DATA_TRANSMIT,
	TWSI_STATE_SES_STOP, /*0x22*/
	TWSI_STATE_RESET = 0xFF,
};

/* MI2C register address */
#define TWSI_SLAVE_ADDRESS	0x0
#define TWSI_X_SLAVE_ADDRESS	0x4
#define TWSI_DATA				0x1
#define TWSI_CONTROL			0x2
#define TWSI_STATUS			0x3 /* READ */
#define TWSI_CLOCK_CONTROL	0x3 /* WRITE */
#define TWSI_SOFT_RESET		0x7

/* MI2C_STATUS bits */
#define TWSI_STATUS_BUS_ERROR				0x00
#define TWSI_STATUS_START_TX				0x08
#define TWSI_STATUS_REP_START_TX			0x10
#define TWSI_STATUS_ADDR_W_TX_ACK			0x18
#define TWSI_STATUS_ADDR_W_TX_NAK			0x20
#define TWSI_STATUS_MDATA_TX_ACK			0x28
#define TWSI_STATUS_MDATA_TX_NAK			0x30
#define TWSI_STATUS_ARBLST_ADDR_DATA		0x38
#define TWSI_STATUS_ADDR_R_TX_ACK			0x40
#define TWSI_STATUS_ADDR_R_TX_NAK			0x48
#define TWSI_STATUS_MDATA_RX_ACK			0x50
#define TWSI_STATUS_MDATA_RX_NAK			0x58
#define TWSI_STATUS_SADDR_W_RX_ACK			0x60
#define TWSI_STATUS_ARBLST_SADDR_W_RX_ACK	0x68
#define TWSI_STATUS_GC_ADDR_RX_ACK			0x70
#define TWSI_STATUS_ARBLST_GC_RX_ACK		0x78
#define TWSI_STATUS_SADDR_DATA_RX_ACK		0x80
#define TWSI_STATUS_SADDR_DATA_RX_NAK		0x88
#define TWSI_STATUS_GC_DATA_RX_ACK			0x90
#define TWSI_STATUS_GC_DATA_RX_NAK			0x98
#define TWSI_STATUS_STOP_RX					0xA0
#define TWSI_STATUS_SADDR_R_RX_ACK			0xA8
#define TWSI_STATUS_ARBLST_SADDR_R_RX_ACK	0xB0
#define TWSI_STATUS_SDATA_TX_ACK			0xB8
#define TWSI_STATUS_SDATA_TX_NAK			0xC0
#define TWSI_STATUS_LAST_SDATA_TX_ACK		0xC8
#define TWSI_STATUS_ADDR2_W_TX_ACK			0xD0
#define TWSI_STATUS_ADDR2_W_TX_NAK			0xD8
#define TWSI_STATUS_ADDR2_R_TX_ACK			0xE0
#define TWSI_STATUS_ADDR2_R_TX_NAK			0xE8
#define TWSI_STATUS_NONE					0xF8

/* This enumerator describes TWSI protocol commands.                        */
enum {
    TWSI_STATE_WRITE = 0,   /* TWSI write command - 0 according to spec   */
    TWSI_STATE_READ   /* TWSI read command  - 1 according to spec */
};

#define TWSI_CFG_TCLK			ROC_T_CLOCK	/* Default Tclk 1663MHz */

#endif /*SUPPORT_TWSI*/
#endif /*defined(CORE_TWSI_H)*/

