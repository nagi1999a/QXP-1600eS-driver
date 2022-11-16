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

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_manager.h"
#include "core_i2c.h"
#include "core_util.h"
#include "core_error.h"
#include "core_exp.h"
#include "hba_inter.h"
#include "core_twsi.h"
#if defined(SUPPORT_TWSI)
#include "mvCommon.h"
#include "mvTwsiSpec.h"
#include "mvTypes.h"

/*[Port Number][Feature][Slave Address][Link Rate(x 1000)]*/
const twsi_device_info_t twsi_devices_info_list[] = {
	{0, (TWSI_FEATURE_ADDR7_BIT), 0xA0, 100, TWSI_NORMAL_DEVICE, "AT24C04BN"}, /*AT24C04BN*/
	{0, (TWSI_FEATURE_ADDR7_BIT), 0xA2, 100, TWSI_NORMAL_DEVICE, "AT24C04BN"}, /*AT24C04BN*/
	{EXT_I2C_DEV_PORT, (/*TWSI_FEATURE_POLLING_MODE |*/ TWSI_FEATURE_WITHOUT_NACK | TWSI_FEATURE_ADDR7_BIT), 0xC0, 200, TWSI_SES_DEVICE, "Enclosure 0"}, /*Enclosure Slave Address*/
	{EXT_I2C_DEV_PORT, (/*TWSI_FEATURE_POLLING_MODE |*/ TWSI_FEATURE_WITHOUT_NACK | TWSI_FEATURE_ADDR7_BIT), 0xC2, 200, TWSI_SES_DEVICE, "Enclosure 1"}, /*Enclosure Slave Address*/
	{1, (TWSI_FEATURE_ADDR7_BIT), 0xAA, 100, TWSI_NORMAL_DEVICE, "TI bq27000 gas gauge IC"}, /*TI bq27000 gas gauge IC*/
	{2, (TWSI_FEATURE_ADDR7_BIT), 0xA0, 100, TWSI_NORMAL_DEVICE, "AT24C02BN"}, /*AT24C02BN*/
	{2, (TWSI_FEATURE_POLLING_MODE | TWSI_FEATURE_ADDR7_BIT), 0x90, 100, TWSI_NORMAL_DEVICE, "TMP75AIDGKTADM1085"}, /*TMP75AIDGKTADM1085*/
	{2, (TWSI_FEATURE_ADDR7_BIT), 0xD0, 100, TWSI_NORMAL_DEVICE, "DS1388Z-33"}, /*DS1388Z-33*/
	{0xFF, 0xFF, 0xFFFF, 0xFFFF, 0xFF, NULL} /*END*/
};

#define TWSI_DBG_PRINT //FM_PRINT
#define TWSI_PRINT FM_PRINT

#define READ_TWSI_REG8 read_i2c_register
#define WRITE_TWSI_REG8 write_i2c_register
#define TWSI_DELAY HBA_SleepMillisecond

#define TWSI_SEARCHING_LIST lib_twsi.searching_devices_list

#define TWSI_CURRENT_STATEMACHINE core->lib_twsi.major_state
#define TWSI_ACCESS_STATE port->twsi_state

#define TWSI_CURRENT_DATAADDR address

#define TWSI_CURRENT_PORTSNUM core->lib_twsi.ports_num
#define TWSI_CURRENT_DEVICESNUM core->lib_twsi.devices_num
#define TWSI_CURRENT_DETECTCNT core->lib_twsi.detecting_cnt

/*******************************************************************************
* twsiIntFlgClr - Clear Interrupt flag.
*
* DESCRIPTION:
*       This routine clears the interrupt flag. It does NOT poll the interrupt
*       to make sure the clear. After clearing the interrupt, it waits for at 
*       least 1 miliseconds.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
static void twsiIntFlgClr(core_extension * core, twsi_port_setting_t *port)
{
    MV_U32 temp; 
    MV_U16 counter = 5000;

    /* clear the int flag bit */
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp & ~TWSI_CONTROL_INT_FLAG_SET);

    if (port->feature & TWSI_FEATURE_POLLING_MODE) {
        while ((counter > 0) && (!(READ_TWSI_REG8(core, port->id, TWSI_CONTROL) & TWSI_CONTROL_INT_FLAG_SET))) {
            TWSI_DELAY(core, 1);
            counter --;
        }
    }
}

/*******************************************************************************
* twsiAckBitSet - Set acknowledge bit on the bus
*
* DESCRIPTION:
*       This routine set the acknowledge bit on the TWSI bus.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
static MV_VOID twsiAckBitSet(core_extension * core, twsi_port_setting_t *port)
{
    MV_U32 temp;

    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

    /*Set the Ack bit */
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp | TWSI_CONTROL_ACK);

    if (port->feature & TWSI_FEATURE_POLLING_MODE)
        twsiIntFlgClr(core, port);
}

/*******************************************************************************
* mvTwsiStopBitSet - Set stop bit on the bus
*
* DESCRIPTION:
*       This routine set the stop bit on the TWSI bus. 
*       The function then wait for the stop bit to be cleared by the HW. 
*       Finally the function checks for status of 0xF8.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE is stop bit was set successfuly on the bus.
*
*******************************************************************************/
MV_STATUS mvTwsisStopBitSet(core_extension * core, twsi_port_setting_t *port)
{
    MV_U32 temp;

    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

   /* Generate stop bit */
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp | TWSI_CONTROL_STOP_BIT);

    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, 0x00);

    return MV_OK;
}

static void twsiNackBitSet(core_extension * core, twsi_port_setting_t *port)
{
    MV_U32 temp;

    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

    /*Set the Ack bit */
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp & ~TWSI_CONTROL_ACK);

    if (port->feature & TWSI_FEATURE_POLLING_MODE)
        twsiIntFlgClr(core, port);
}

/*******************************************************************************
* mvTwsiStartBitSet - Set start bit on the bus
*
* DESCRIPTION:
*       This routine sets the start bit on the TWSI bus. 
*       The routine first checks for interrupt flag condition, then it sets 
*       the start bit  in the TWSI Control register. 
*       If the interrupt flag condition check previously was set, the function 
*       will clear it.
*       The function then wait for the start bit to be cleared by the HW. 
*       Then it waits for the interrupt flag to be set and eventually, the 
*       TWSI status is checked to be 0x8 or 0x10(repeated start bit).
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK is start bit was set successfuly on the bus.
*       MV_FAIL if interrupt flag was set before setting start bit.
*
*******************************************************************************/
void mvTwsisStartBitSet(core_extension * core,  twsi_port_setting_t *port)
{
    MV_U8 temp;
    
    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

     /* set start Bit */
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp | TWSI_CONTROL_START_BIT);

    if (port->feature & TWSI_FEATURE_POLLING_MODE)
        twsiIntFlgClr(core, port);
}


/*******************************************************************************
* twsiAddr10BitSet - Set 10 Bit address on TWSI bus.
*
* DESCRIPTION:
*       There are two address phases:
*       1) Write '11110' to data register bits [7:3] and 10-bit address MSB 
*          (bits [9:8]) to data register bits [2:1] plus a write(0) or read(1) bit 
*          to the Data register. Then it clears interrupt flag which drive 
*          the address on the TWSI bus. The function then waits for interrupt 
*          flag to be active and status 0x18 (write) or 0x40 (read) to be set.
*       2) write the rest of 10-bit address to data register and clears 
*          interrupt flag which drive the address on the TWSI bus. The 
*          function then waits for interrupt flag to be active and status 
*          0xD0 (write) or 0xE0 (read) to be set. 
*
* INPUT:
*       deviceAddress - twsi address.
*	command	 - read / write .
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK - if setting the address completed succesfully.
*	MV_FAIL otherwmise.
*
*******************************************************************************/
static void twsiAddr10BitSet(core_extension * core,  twsi_port_setting_t *port, MV_U16 command)
{
    twsi_device_setting_t *device = (twsi_device_setting_t *)port->current_working_device;
    twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
    MV_U32 val;

    /* writing the 2 most significant bits of the 10 bit address*/
    /*11110ddd ddddddd0*/
    val = (MV_U32)devices_info_list->slave_target | 0xf000 | command;
    WRITE_TWSI_REG8(core, port->id, TWSI_DATA, val);
}

/*******************************************************************************
* twsiAddr7BitSet - Set 7 Bit address on TWSI bus.
*
* DESCRIPTION:
*       This function writes 7 bit address plus a write or read bit to the 
*       Data register. Then it clears interrupt flag which drive the address on 
*       the TWSI bus. The function then waits for interrupt flag to be active
*       and status 0x18 (write) or 0x40 (read) to be set.
*
* INPUT:
*       deviceAddress - twsi address.
*	command	 - read / write .
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK - if setting the address completed succesfully.
*	MV_FAIL otherwmise.
*
*******************************************************************************/
static void twsiAddr7BitSet(core_extension * core, twsi_port_setting_t *port, MV_U8 command)
{
    twsi_device_setting_t *device = (twsi_device_setting_t *)port->current_working_device;
    twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
    MV_U32 temp = (MV_U32)devices_info_list->slave_target | command;

    WRITE_TWSI_REG8(core, port->id, TWSI_DATA, temp);
}

MV_STATUS mvTwsisAddrSet(core_extension * core, twsi_port_setting_t *port, MV_U8 command)
{

    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

    if (port->feature & TWSI_FEATURE_ADDR7_BIT) {
         /* 7 Bit address */
        twsiAddr7BitSet(core, port, command);
    } else {
        /* 10 Bit address */
        twsiAddr10BitSet(core, port, (MV_U16)command);
    }

    if (port->feature & TWSI_FEATURE_POLLING_MODE)
        twsiIntFlgClr(core, port);
    return MV_OK;
}

/*******************************************************************************
* twsiDataWrite - Trnasmit a data block over TWSI bus.
*
* DESCRIPTION:
*       This function writes a given data block to TWSI bus in 8 bit granularity.
*	first The function waits for interrupt flag to be active then
*       For each 8-bit data:
*        The function writes data to data register. It then clears 
*        interrupt flag which drives the data on the TWSI bus. 
*        The function then waits for interrupt flag to be active and status 
*        0x28 to be set. 
*      
*
* INPUT:
*       pBlock - Data block.
*	blockSize - number of chars in pBlock.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK - if transmiting the block completed succesfully,
*	MV_BAD_PARAM - if pBlock is NULL,
*	MV_FAIL otherwmise.
*
*******************************************************************************/
static MV_STATUS twsiDataTransmit(core_extension * core, twsi_port_setting_t *port, MV_U8 data)
{
    if (!(port->feature & TWSI_FEATURE_POLLING_MODE))
        twsiIntFlgClr(core, port);

    /* write the data*/
    WRITE_TWSI_REG8(core, port->id, TWSI_DATA, data);

//    TWSI_DBG_PRINT("send = 0x%x\n", data);

    if (port->feature & TWSI_FEATURE_POLLING_MODE)
        twsiIntFlgClr(core, port);

    return MV_OK;
}

/*******************************************************************************
* twsiDataReceive - Receive data block from TWSI bus.
*
* DESCRIPTION:
*       This function receive data block from TWSI bus in 8bit granularity 
*       into pBlock buffer.
*	first The function waits for interrupt flag to be active then
*       For each 8-bit data:
*        It clears the interrupt flag which allows the next data to be 
*        received from TWSI bus.
*	 The function waits for interrupt flag to be active,
*	 and status reg is 0x50. 
*	 Then the function reads data from data register, and copies it to 
*	 the given buffer. 
*
* INPUT:
*       blockSize - number of bytes to read.
*
* OUTPUT:
*       pBlock - Data block.
*
* RETURN:
*       MV_OK - if receive transaction completed succesfully,
*	MV_BAD_PARAM - if pBlock is NULL,
*	MV_FAIL otherwmise.
*
*******************************************************************************/
static MV_STATUS twsiDataReceive(core_extension * core,  twsi_port_setting_t *port, MV_U8 *pBlock)
{
    if (NULL == pBlock) {
	    TWSI_PRINT("JL %s NULL == pBlock\n", __FUNCTION__);
	    return MV_BAD_PARAM;
    }
   
    /* read the data*/
    *pBlock = (MV_U8)READ_TWSI_REG8(core, port->id, TWSI_DATA);

//    TWSI_DBG_PRINT("data = 0x%x\n", *pBlock);

    return 0x00;
}

MV_VOID twsi_reset(core_extension *core, twsi_port_setting_t *port)
{
	/* Reset the TWSI logic */
	WRITE_TWSI_REG8(core, port->id, TWSI_SOFT_RESET, 0);

	/* wait for 1m sec */
	TWSI_DELAY(core, 1);
}

/*******************************************************************************
* twsiInit - Initialize TWSI interface
*
* DESCRIPTION:
*       This routine:
*	-Reset the TWSI.
*	-Initialize the TWSI clock baud rate according to given frequancy
*	 parameter based on Tclk frequancy and enables TWSI slave.
*       -Set the ack bit.
*	-Assign the TWSI slave address according to the TWSI address Type.
*       
*
* INPUT:
*       frequancy - TWSI frequancy in KHz. (up to 100KHZ)
*
* OUTPUT:
*       None.
*
* RETURN:
*       Actual frequancy.
*
*******************************************************************************/
MV_U32 mvTwsisInit(core_extension * core, twsi_port_setting_t *port, MV_U32 Tclk, MV_BOOLEAN generalCallEnable)
{
    MV_U32 n,m,freq,margin,minMargin = 0xffffffff;
    MV_U32 power;
    MV_U32 actualFreq = 0, actualN = 0, actualM = 0, val;
    MV_U32 frequency = port->link_rate * 1000; /*x 1K Hz*/

    TWSI_DBG_PRINT("port%d frequency =>> %d...\n", port->id, frequency);
    TWSI_DBG_PRINT("TWSI_CURRENT_SLAVEADDR =>> 0x%x...\n", port->slave_host);
    TWSI_DBG_PRINT("TWSI_CURRENT_FEATURE=>> 0x%x...\n", port->feature);

    /* Calucalte N and M for the TWSI clock baud rate */
    for (n = 0 ; n < 8 ; n++) {
        for(m = 0 ; m < 16 ; m++) {
            power = 2 << n; /* power = 2^(n+1) */
            freq = Tclk/(10*(m+1)*power);
            margin = MV_ABS(frequency - freq);

            if (margin < minMargin) {
            	minMargin   = margin;
            	actualFreq  = freq;
            	actualN     = n;
            	actualM     = m;
            }
        }
    }

    /* Reset the TWSI logic */
    twsi_reset(core, port);

    /* Set the baud rate */
    val = ((actualM<< TWSI_BAUD_RATE_M_OFFS) | actualN << TWSI_BAUD_RATE_N_OFFS);
    WRITE_TWSI_REG8(core, port->id, TWSI_CLOCK_CONTROL, val);

    /* Enable the TWSI and slave */
    WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, TWSI_CONTROL_ENA | TWSI_CONTROL_ACK); 

    /* set the TWSI slave address */
    if (port->feature & TWSI_FEATURE_ADDR7_BIT) {
        /*7 bit address*/
        /* set the 7 Bits address */
        val = 0;
        WRITE_TWSI_REG8(core, port->id, TWSI_X_SLAVE_ADDRESS, val);
        val = port->slave_host;
        WRITE_TWSI_REG8(core, port->id, TWSI_SLAVE_ADDRESS, val);
    } else {
        /* 10 Bit deviceAddress */
        /* writing the 2 most significant bits of the 10 bit address*/
        val = ((port->slave_host & TWSI_SLAVE_ADDR_10BIT_MASK) >> TWSI_SLAVE_ADDR_10BIT_OFFS );
        /* bits 7:3 must be 0x11110 */
        val |= TWSI_SLAVE_ADDR_10BIT_CONST;
        /* set GCE bit */
        if (generalCallEnable)
            val |= TWSI_SLAVE_ADDR_GCE_ENA;
        /* write slave address */
        WRITE_TWSI_REG8(core, port->id, TWSI_SLAVE_ADDRESS, val);

        /* writing the 8 least significant bits of the 10 bit address*/
        val = (port->slave_host << TWSI_EXTENDED_SLAVE_OFFS) & TWSI_EXTENDED_SLAVE_MASK;  
        WRITE_TWSI_REG8(core, port->id, TWSI_X_SLAVE_ADDRESS, val);
    }

    if (!(port->feature & TWSI_FEATURE_POLLING_MODE)) {
        /* unmask twsi int */
        val = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
        WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, val | TWSI_CONTROL_INT_ENA);
    }

    return actualFreq;
} 

MV_BOOLEAN twsi_access_statemachine(core_extension * core, MV_PVOID ptr_dev);

void twsi_access_statemachine_timeout_handle(MV_PVOID module_pointer, MV_PVOID ptr_dev)
{
	core_extension * core = (core_extension *)module_pointer;
	twsi_device_setting_t *device = (twsi_device_setting_t *)ptr_dev;
	twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
	twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list->port_id];
	MV_U8 TwsiCtlReg, TwsiSateReg;

	port->timer_num = NO_CURRENT_TIMER;
	TWSI_PRINT("%s %d %s <I2C timeout> TWSI_CURRENT_STATEMACHINE = 0x%X, TWSI_ACCESS_STATE = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, TWSI_CURRENT_STATEMACHINE, TWSI_ACCESS_STATE);
	TwsiCtlReg = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
       TwsiSateReg = READ_TWSI_REG8(core, port->id, TWSI_STATUS);
	TWSI_PRINT("%s %d %s <I2C Timeout> TwsiCtlReg = 0x%X, TwsiSateReg = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, TwsiCtlReg, TwsiSateReg);

	TWSI_ACCESS_STATE = TWSI_STATE_TIMEOUT;
	twsi_access_statemachine(core, ptr_dev);
}

static void dump_i2c_register_status(core_extension * core, MV_U8 port)
{
    MV_U8 I2cCtlReg,I2cStatusReg;

    I2cCtlReg = READ_TWSI_REG8(core, port, TWSI_CONTROL);
    TWSI_PRINT("Error TWSI[%d].I2cCtlReg = 0x%X\n", port, I2cCtlReg);
    I2cStatusReg = READ_TWSI_REG8(core, port, TWSI_STATUS);
    TWSI_PRINT("Error TWSI[%d].I2cStatusReg = 0x%X\n", port, I2cStatusReg);
}

static MV_U8 twsiWaitStatusUpdate(core_extension * core, twsi_port_setting_t *port, MV_U8 input)
{
    MV_U8 status;
    MV_U8 temp;

    status = READ_TWSI_REG8(core, port->id, TWSI_STATUS);

#if 1 /*For Debugging*/
    TWSI_DBG_PRINT("TWSI[%d].I2cStatusReg = 0x%X\n", port->id, status);
    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
    TWSI_DBG_PRINT("TWSI[%d].I2cCtlReg = 0x%X\n", port->id, temp);
#endif
    if (status == input)
        return 0;
    return 1;
}

#define IS_NORMAL_TWSI_DEVICE(core, dev) (dev->dev_type == TWSI_NORMAL_DEVICE)
#define IS_SES_TWSI_DEVICE(core, dev) ((TWSI_CURRENT_STATEMACHINE == TWSI_DEVICES_STATEMACHINE_STARTED) && ((dev)->dev_type == TWSI_SES_DEVICE))
int twsi_polling_mode(
    core_extension * core,
    MV_PVOID ptr_dev)
{
    twsi_device_setting_t *device = (twsi_device_setting_t *)ptr_dev;
    twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
    twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list->port_id];
    twsi_data_slot_t *slot = &port->slot[port->current_slot_num ++];

    mvTwsisStartBitSet(core, port);
    if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_START_TX)) {
        TWSI_PRINT("Start bit is fail.\n");
        dump_i2c_register_status(core, port->id);
        twsiNackBitSet(core, port);
        return 1;
    }

    if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
        while (IS_TWSI_COMMAND_WRITE_SEQUENCE(slot->flag)) {
            if (!IS_TWSI_RUNNING_SEQUENCE(slot->flag)) {
                TWSI_PRINT("This is not running slot(Transmitted).\n");
                return 1;
            }
            for (; port->xfer_count < slot->size; port->xfer_count ++) {
                TWSI_DBG_PRINT("T[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + (port->xfer_count)));
                twsiDataTransmit(core, port, *((char *)slot->addr + (port->xfer_count)));
                if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_TX_ACK)) {
                    if (port->xfer_count == 0) {
                        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_ADDR_W_TX_ACK)) {
                              TWSI_PRINT("Address + write bit transmitted, ACK received fail..\n");
                             dump_i2c_register_status(core, port->id);
                             twsiNackBitSet(core, port);
                             return 1;
                         } 
                    } else {
                        TWSI_PRINT("Data byte transmitted in master mode, ACK received fail..\n");
                        dump_i2c_register_status(core, port->id);
                        return 1;
                    }
                }
            }
            CLR_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
            if (port->current_slot_num < port->max_slot_num) {
                slot = &port->slot[port->current_slot_num ++];
                port->xfer_count = 0;
            } else {
                TWSI_PRINT("No next slot.\n");
                return 1;
            }
        }

	{ /*mvTwsisStopBitSet*/
          MV_U8 temp;
	   /* Generate stop bit */
	    temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
           temp |= TWSI_CONTROL_STOP_BIT;
           WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp);

           twsiIntFlgClr(core, port);
           if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_SADDR_W_RX_ACK)) {
               TWSI_PRINT("Slave address + write bit received, ACK transmitted fail.\n");
               dump_i2c_register_status(core, port->id);
               return 1;
           }
 	}

       {
	     MV_U8 stop = 0;
            while ((IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag)) && (!stop)) {
                if (!IS_TWSI_RUNNING_SEQUENCE(slot->flag)) {
                    TWSI_PRINT("This is not running slot(Received).\n");
                    return 1;
                }
                for (; port->xfer_count < slot->size; port->xfer_count ++) {
                    twsiDataReceive(core, port, ((char *)slot->addr + (port->xfer_count)));
                    TWSI_DBG_PRINT("R[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + (port->xfer_count)));
                    twsiAckBitSet(core, port);
                    if (!twsiWaitStatusUpdate(core, port, TWSI_STATUS_STOP_RX)) {
                        TWSI_DBG_PRINT("STOP or repeated START condition received in slave mode..\n");
                        stop = 1;
                        break;
                    }
        
                    if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_SADDR_DATA_RX_ACK)) {
                        TWSI_PRINT("Data byte received after slave address received, ACK transmitted fail..\n");
                        dump_i2c_register_status(core, port->id);
                        return 1;
                    }
                }
                CLR_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
                if (port->current_slot_num < port->max_slot_num) {
                    slot = &port->slot[port->current_slot_num ++];
                    port->xfer_count = 0;
                } else {
                    break;
                }
            }
    	 }
         return 0;
    }
    else {
        mvTwsisAddrSet(core, port, TWSI_STATE_WRITE);
        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_ADDR_W_TX_ACK)) {
            TWSI_PRINT("To set Slave Address is fail.\n");
            dump_i2c_register_status(core, port->id);
            twsiNackBitSet(core, port);
            return 1;
        }
    
        twsiDataTransmit(core, port, (MV_U8)(port->TWSI_CURRENT_DATAADDR));
        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_TX_ACK)) {
            TWSI_PRINT("To set Offset is fail.\n");
            dump_i2c_register_status(core, port->id);
            twsiNackBitSet(core, port);
            return 1;
        }
    
#if 0
        twsiDataTransmit(core, port, (MV_U8)(port->TWSI_CURRENT_DATAADDR >> 8));
        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_TX_ACK)) {
            TWSI_DBG_PRINT("To set Offset is fail.\n");
            dump_i2c_register_status(core, port->id);
            twsiNackBitSet(core, port);
            return 1;
        }
#endif
    }

    if (IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag)) {
        mvTwsisStartBitSet(core, port);
        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_REP_START_TX)) {
            TWSI_PRINT("Repeated start bit is fail.\n");
            dump_i2c_register_status(core, port->id);
            twsiNackBitSet(core, port);
            return 1;
        }
    
        mvTwsisAddrSet(core, port, TWSI_STATE_READ);
        if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_ADDR_R_TX_ACK)) {
            TWSI_PRINT("Address + read bit transmitted, acknowledge not received.\n");
            dump_i2c_register_status(core, port->id);
            twsiNackBitSet(core, port);
            return 1;
        }

        for (; port->xfer_count < slot->size; port->xfer_count ++) {
            twsiAckBitSet(core, port);
            if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_RX_ACK)) {
                TWSI_PRINT("Master received read data, acknowledge not transmitted.\n");
                dump_i2c_register_status(core, port->id);
                twsiNackBitSet(core, port);
                return 1;
            }
            twsiDataReceive(core, port, ((char *)slot->addr + (port->xfer_count)));
            TWSI_DBG_PRINT("R[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + (port->xfer_count)));
        }

	/*Workaround for Enclosure IDs.*/
	if (IS_NORMAL_TWSI_DEVICE(core, devices_info_list)) {
		if (!(devices_info_list->feature & TWSI_FEATURE_WITHOUT_NACK)) {
	           twsiNackBitSet(core, port);
	           twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_RX_ACK);
		}
	}
    } 
    else { /*TWSI_WRITE_SEQUENCE*/
       for (; port->xfer_count < slot->size; port->xfer_count ++) {
            TWSI_DBG_PRINT("T[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + (port->xfer_count)));
            twsiDataTransmit(core, port, *((char *)slot->addr + (port->xfer_count)));
            if (twsiWaitStatusUpdate(core, port, TWSI_STATUS_MDATA_TX_ACK)) {
                TWSI_PRINT("Master received read data, acknowledge not transmitted.\n");
                dump_i2c_register_status(core, port->id);
                return 1;
            }
        }
    }

    return 0;
}

twsi_device_setting_t * twsi_device_map(core_extension * core, MV_U8 port_id, MV_U16 slave_id)
{
	twsi_device_setting_t *device = NULL;
	twsi_device_info_t *devices_info_list = NULL;
	MV_U8 i;

	for (i = 0; i < TWSI_CURRENT_DEVICESNUM; i ++) {
		device = core->lib_twsi.device[i];
		if (device != NULL) {
			devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
			if ((devices_info_list->port_id == port_id) && (devices_info_list->slave_target == slave_id)) {
				return device;
			}
		} else {
			return device;
		}
	}

	return NULL;
}

void i2c_to_twsi_call_back(MV_PVOID module_pointer, MV_PVOID temp)
{
	core_extension *core = (core_extension *)module_pointer;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;

	twsi_device_setting_t *device = (twsi_device_setting_t *)temp;
	twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
	twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list->port_id];

	MV_Request *req = (MV_Request *)i2c_link->i2c_request;

	domain_enclosure *enc = NULL;
	MV_ULONG flags;
	enc = (domain_enclosure *)get_device_by_id(&core->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	if (enc == NULL) {
		TWSI_PRINT("NO Enclosure id(0x%lx)\n", req);
		return;
	}

	i2c_link->i2c_request = NULL;
	
	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		TWSI_DBG_PRINT("Enclosure command is successful(0x%lx 0x%lx)\n", core, req);
		i2c_link->i2c_state = I2C_STATE_IDLE;
	} else  {
		TWSI_DBG_PRINT("Wait command timeout(0x%lx 0x%lx)\n", core, req);
		req->Scsi_Status = REQ_STATUS_ERROR;
		i2c_link->i2c_state = I2C_STATE_ERROR;
	}

	OSSW_SPIN_LOCK(&enc->base.err_ctx.sent_req_list_SpinLock, flags);
	enc->base.outstanding_req --;
	mv_renew_timer(core, req);
	OSSW_SPIN_UNLOCK(&enc->base.err_ctx.sent_req_list_SpinLock, flags);

	if (req->Scsi_Status == REQ_STATUS_ERROR) {
		if(enc->base.err_ctx.retry_count < 5 ) {	/* Change the retry count from 3000 to 3 */
#ifdef ATHENA_PERFORMANCE_TUNNING
			MV_ULONG flags;
			OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
#endif
			Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
			enc->base.err_ctx.retry_count ++;
#ifdef ATHENA_PERFORMANCE_TUNNING
			OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
#endif
			return MV_TRUE;
		}
	}
	enc->base.err_ctx.retry_count = 0;

	if (port->feature & TWSI_FEATURE_POLLING_MODE) {
//		if (req->Completion) {
//			req->Completion(req->Cmd_Initiator, req);
//		}
		TWSI_DBG_PRINT("Command Completionl(0x%lx)\n", req);
	} else {
		core_queue_completed_req(core, req);
		TWSI_DBG_PRINT("Command Callback(0x%lx)\n", req);
	}
}

MV_QUEUE_COMMAND_RESULT i2c_to_twsi(core_extension *core, MV_Request *req)
{
	MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
	PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	twsi_port_setting_t *port = NULL;
	twsi_device_setting_t *device = NULL;
	twsi_device_info_t *devices_info_list = NULL;
	MV_U8 err = 0;

	if (TWSI_CURRENT_STATEMACHINE != TWSI_DEVICES_STATEMACHINE_STARTED) {
		TWSI_DBG_PRINT("Waiting TWSI module ready(Enclosure).\n");
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}

	if (device = twsi_device_map(core, EXT_I2C_DEV_PORT, i2c_link->i2c_sep_address)) {
		devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
		port = &core->lib_twsi.port[devices_info_list->port_id];
		if (port->running_req) {
			TWSI_PRINT("Waiting TWSI module request done.\n");
			return MV_QUEUE_COMMAND_RESULT_FULL;
		}
	}

	TWSI_DBG_PRINT("TWSI request enter.(0x%lx 0x%lx)\n", core, req);
	
	i2c_link->i2c_request = req;
	i2c_link->i2c_state = I2C_STATE_CMD;

	if (device == NULL) {
		i2c_link->i2c_request = NULL;
		i2c_link->i2c_state = I2C_STATE_ERROR;
		req->Scsi_Status = REQ_STATUS_ERROR;
//		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		TWSI_PRINT("No Supported this twsi device.((1)port=%d, slave=0x%x)\n", EXT_I2C_DEV_PORT, i2c_link->i2c_sep_address);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	req->Scsi_Status = REQ_STATUS_PENDING;
	port->running_req = req;
	port->current_working_device = (MV_PVOID)device;
	port->TWSI_CURRENT_DATAADDR = (MV_U32)0x00L;
	port->slave_host = I2C_SEMB_ADDR;

	port->current_slot_num = 0;
	port->max_slot_num = 0;
	MV_ZeroMemory(port->slot, TWSI_DATA_MAX_SLOTS * sizeof(twsi_data_slot_t));
	if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
		if (i2c_link->i2c_cmd_header[1] == I2C_CT_D2H_SES) {
			twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)i2c_link->i2c_cmd_header;
			slot->size = 7;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)i2c_link->i2c_cmd_header;
			slot->size = 7;
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)core_map_data_buffer(req);
			slot->size = (MV_U16)req->Data_Transfer_Length;
			core_unmap_data_buffer(req);
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;
		} else if (i2c_link->i2c_cmd_header[1] == I2C_CT_H2D_SES) {
			twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)i2c_link->i2c_cmd_header;
			slot->size = 6;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)core_map_data_buffer(req);
			slot->size = (MV_U16)req->Data_Transfer_Length;
			core_unmap_data_buffer(req);
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)&i2c_link->i2c_cmd_header[6]; /*Check Sum*/
			slot->size = 1;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)i2c_link->i2c_cmd_header;
			slot->size = 8;
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (MV_PVOID)core_map_data_buffer(req);
			slot->size = (MV_U16)req->Data_Transfer_Length;
			core_unmap_data_buffer(req);
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;
		}
		else {
			err = 1;
		}
	}
	else {
		err = 1;
	}

	if (err) {
		port->running_req = NULL;
		port->current_working_device = NULL;
		port->TWSI_CURRENT_DATAADDR = (MV_U32)0x00L;
		port->slave_host = 0x00L;

		TWSI_PRINT("No Supported this twsi device.((2)port=%d, slave=0x%x)\n", EXT_I2C_DEV_PORT, i2c_link->i2c_sep_address);
		i2c_link->i2c_state = I2C_STATE_ERROR;
		req->Scsi_Status = REQ_STATUS_ERROR;
//		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	} else {
		TWSI_ACCESS_STATE = TWSI_STATE_TIMING_INIT; /* set the i2c frequency first*/
		twsi_access_statemachine(core, device);
	}
	if (req->Scsi_Status != REQ_STATUS_PENDING) {
		TWSI_DBG_PRINT("I2C to TWSI MV_QUEUE_COMMAND_RESULT_FINISHED....done\n");
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	TWSI_DBG_PRINT("I2C to TWSI MV_QUEUE_COMMAND_RESULT_SENT.....\n");
	return MV_QUEUE_COMMAND_RESULT_SENT;
}

void twsi_send_request(MV_PVOID module_pointer, PMV_Request pmvreq);
void twsi_check_initial_devices_is_done(MV_PVOID module_pointer, MV_PVOID temp);

#define GO_OUT_WAITING_NEXT_STEP go_next_step = 0x00
#define GO_NEXT_STEP go_next_step = 0x01
MV_U8 twsi_access_statemachine(
	core_extension * core,
	MV_PVOID ptr_dev
)
{
	MV_PVOID upper_layer_ptr = HBA_GetModuleExtension(core, MODULE_HBA);
	PHBA_Extension hba_ptr = (PHBA_Extension)upper_layer_ptr;
	MV_U8 TwsiCtlReg, TwsiSateReg;

	twsi_device_setting_t *device = (twsi_device_setting_t *)ptr_dev;
	twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
	twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list->port_id];
	PMV_Request	ptr_req = port->running_req;
	twsi_data_slot_t *slot = NULL;


	MV_U16 go_next_step;

	if (port->max_slot_num <= 0) {
		TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
	} else {
		slot = &port->slot[port->current_slot_num];
//		if ((slot == NULL) || (port->current_slot_num > TWSI_DATA_MAX_SLOTS) || (!IS_TWSI_RUNNING_SEQUENCE(slot->flag)))
//			TWSI_ACCESS_STATE = TWSI_STATE_STOP;
	}

	do {
		GO_OUT_WAITING_NEXT_STEP;

		TwsiCtlReg = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
	       TwsiSateReg = READ_TWSI_REG8(core, port->id, TWSI_STATUS);
		TWSI_DBG_PRINT("<I2C test> TwsiCtlReg = 0x%X, TwsiSateReg = 0x%X\n", TwsiCtlReg, TwsiSateReg);
		TWSI_DBG_PRINT(">>>TWSI_ACCESS_STATE => 0x%X...slot%d->flag=0x%x size=0x%x\n", TWSI_ACCESS_STATE, port->current_slot_num, slot->flag, slot->size);

		/*Init a timer for detecting command timeout.*/
		if (port->timer_num != NO_CURRENT_TIMER){
			Timer_CancelRequest (hba_ptr, port->timer_num);
			port->timer_num = NO_CURRENT_TIMER;
		}
		port->timer_num = Timer_AddRequest(hba_ptr, 15, twsi_access_statemachine_timeout_handle, (MV_PVOID)core, (MV_PVOID)ptr_dev);

		switch (TWSI_ACCESS_STATE) {
		case TWSI_STATE_TIMING_INIT:
			{
				port->link_rate = devices_info_list->link_rate;
				port->feature = devices_info_list->feature;
				port->xfer_count = 0x00;

				TWSI_DBG_PRINT("TWSI_CURRENT_PORTNUM =>> %d...\n", port->id);
				TWSI_DBG_PRINT("TWSI_TARGET_LINKRATE =>> %d...\n", devices_info_list->link_rate);
				TWSI_DBG_PRINT("TWSI_TARGET_SLAVEADDR =>> 0x%x...\n", (devices_info_list->slave_target));
				TWSI_DBG_PRINT("TWSI_TARGET_FEATURE =>> 0x%x...\n", devices_info_list->feature);
			}
			TWSI_ACCESS_STATE = TWSI_STATE_TIMING_SET;
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_TIMING_SET:
			mvTwsisInit(core, port, TWSI_CFG_TCLK, 0);
 			if (port->feature & TWSI_FEATURE_POLLING_MODE) {
				if (twsi_polling_mode(core, ptr_dev))
					TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
				else
					TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
				GO_NEXT_STEP;
				break;
			}

			TWSI_ACCESS_STATE = TWSI_STATE_START;
			GO_NEXT_STEP;
			break;
		
		case TWSI_STATE_START:
			mvTwsisStartBitSet(core, port);
			TWSI_ACCESS_STATE = TWSI_STATE_START_ACK;
			break;

		case TWSI_STATE_START_ACK:
			{
				if (
					(TwsiSateReg == TWSI_STATUS_START_TX)
				) {
					if (IS_SES_TWSI_DEVICE(core, devices_info_list))
						TWSI_ACCESS_STATE = TWSI_STATE_DATA_TRANSMIT;
					else
						TWSI_ACCESS_STATE = TWSI_STATE_SLAVE_SET;
				} else if (
					(TwsiSateReg == TWSI_STATUS_REP_START_TX)
				) {
					TWSI_ACCESS_STATE = TWSI_STATE_SLAVE_SET_RDWT;
				} else {
					TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
				}
			}
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_SLAVE_SET_RDWT:
			mvTwsisAddrSet(core, port, ((IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag)) ? TWSI_STATE_READ : TWSI_STATE_WRITE));
			TWSI_ACCESS_STATE = TWSI_STATE_SLAVE_SET_RDWT_ACK;
			break;

		case TWSI_STATE_SLAVE_SET_RDWT_ACK:
			if ((slot->size - port->xfer_count) > 0) {
				TWSI_ACCESS_STATE = TWSI_STATE_DATA_RECEIVE;
				twsiAckBitSet(core, port);
				break;
			} else {
				if (IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag)) {
					TWSI_ACCESS_STATE = TWSI_STATE_NACK;
				} else {
					TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
				}
			}
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_SLAVE_SET:
			mvTwsisAddrSet(core, port, TWSI_STATE_WRITE);
			TWSI_ACCESS_STATE = TWSI_STATE_SLAVE_SET_ACK;
			break;

		case TWSI_STATE_SLAVE_SET_ACK:
			if (TwsiSateReg == TWSI_STATUS_ADDR_W_TX_ACK) {
				TWSI_ACCESS_STATE = TWSI_STATE_ADDR0_SET;
			} else {
				TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
			}
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_ADDR0_SET: /*Word Address byte 0*/
			twsiDataTransmit(core, port, (MV_U8)(port->TWSI_CURRENT_DATAADDR));
//			TWSI_ACCESS_STATE = TWSI_STATE_ADDR1_SET;
			TWSI_ACCESS_STATE = TWSI_STATE_ADDR_SET_ACK;
			break;

//		case TWSI_STATE_ADDR1_SET: /*Word Address byte 1*/
//			twsiDataTransmit(core, port, (MV_U8)(port->TWSI_CURRENT_DATAADDR >> 8));
//			TWSI_ACCESS_STATE = TWSI_STATE_ADDR_SET_ACK;
//			break;

//		case TWSI_STATE_ADDR2_SET: /*Word Address byte 2*/
//			twsiDataTransmit(core, port->id, (MV_U8)(port->TWSI_CURRENT_DATAADDR >> 8));
//			TWSI_ACCESS_STATE = TWSI_STATE_ADDR_SET_ACK;
//			break;

		case TWSI_STATE_ADDR_SET_ACK:
			{
				if (
					(TwsiSateReg == TWSI_STATUS_MDATA_TX_ACK)
				) {
					TWSI_DBG_PRINT(">>>>>>>>>>>>>>The Device is valid on Port%d<Target SlaveAddr=0x%x>.\n", port->id, devices_info_list->slave_target);
					SET_TO_TWSI_TRANSMIT_DATA_SEQUENCE(slot->flag);
					if (IS_TWSI_COMMAND_WRITE_SEQUENCE(slot->flag)) { /*For Page Write*/
						if ((slot->size - port->xfer_count) > 0) {
							TWSI_ACCESS_STATE = TWSI_STATE_DATA_TRANSMIT;
						} else {
							TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
						}
					} else { /*For Random Read*/
						if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
							TWSI_ACCESS_STATE = TWSI_STATE_DATA_TRANSMIT;
						} else {
							TWSI_ACCESS_STATE = TWSI_STATE_START;
						}
					}
				} else {
					TWSI_DBG_PRINT(">>>>>>>>>>>>>>Didn't detectd this device on port%d<Target SlaveAddr=0x%x>.\n", port->id, devices_info_list->slave_target);
					TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
					break;
				}
			}
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_DATA_RECEIVE:
			if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
//				if (port->xfer_count > 0) {
					if (TwsiSateReg == TWSI_STATUS_STOP_RX) {
						TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
						GO_NEXT_STEP;
						break;
					}
					if ((TwsiSateReg != TWSI_STATUS_SADDR_W_RX_ACK) && (TwsiSateReg != TWSI_STATUS_SADDR_DATA_RX_ACK)) {
						TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
						GO_NEXT_STEP;
						break;
					}
//				}
			}

			twsiDataReceive(core, port, ((char *)slot->addr + port->xfer_count));
			TWSI_DBG_PRINT("Receive data[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + port->xfer_count));
			port->xfer_count ++;

			if ((slot->size - port->xfer_count) > 0) {
    				twsiAckBitSet(core, port);
			} else {
				if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
					port->xfer_count = 0;
					CLR_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
					port->current_slot_num ++;
					if (port->current_slot_num >= port->max_slot_num) {
						TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
						GO_NEXT_STEP;
						break;
					}
					slot = &port->slot[port->current_slot_num];
					if ((IS_TWSI_RUNNING_SEQUENCE(slot->flag)) && IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag)) {
    						twsiAckBitSet(core, port);
						break;
					}
					TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
				} else {
					TWSI_ACCESS_STATE = TWSI_STATE_NACK;
				}
				GO_NEXT_STEP;
			}
			break;

		case TWSI_STATE_DATA_TRANSMIT:
			if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
				if (port->xfer_count > 0) {
					if ((TwsiSateReg != TWSI_STATUS_ADDR_W_TX_ACK) && (TwsiSateReg != TWSI_STATUS_MDATA_TX_ACK)) {
						TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
						GO_NEXT_STEP;
						break;
					}
				}
			}

			TWSI_DBG_PRINT("Send data[%d]=0x%x\n", port->xfer_count, *((char *)slot->addr + (port->xfer_count)));
			twsiDataTransmit(core, port, (MV_U8)*((char *)slot->addr + (port->xfer_count ++)));

  			if ((slot->size - port->xfer_count) <= 0) {
				/* last byte transfered, generate a STOP signal */
				if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
					port->xfer_count = 0;
					CLR_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
					port->current_slot_num ++;
					if (port->current_slot_num > port->max_slot_num) {
						TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
						GO_NEXT_STEP;
						break;
					}
					slot = &port->slot[port->current_slot_num];
					if (!IS_TWSI_RUNNING_SEQUENCE(slot->flag)) {
						TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
						GO_NEXT_STEP;
						break;
					}
					if (IS_TWSI_COMMAND_READ_SEQUENCE(slot->flag))
						TWSI_ACCESS_STATE = TWSI_STATE_SES_STOP;
				} else {
					TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
				}
			}
			break;

		case TWSI_STATE_FAIL:
		case TWSI_STATE_TIMEOUT:
			TWSI_PRINT("%s %d %s TWSI_CURRENT_STATEMACHINE = 0x%X, TWSI_ACCESS_STATE = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, TWSI_CURRENT_STATEMACHINE, TWSI_ACCESS_STATE);
			TWSI_PRINT("%s %d %s TwsiCtlReg = 0x%X, TwsiSateReg = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, TwsiCtlReg, TwsiSateReg);

			if (TWSI_CURRENT_STATEMACHINE == TWSI_DEVICES_STATEMACHINE_STARTED) {
				if (ptr_req) {
					ptr_req->Scsi_Status = ((TWSI_ACCESS_STATE == TWSI_STATE_FAIL) ? REQ_STATUS_ERROR : REQ_STATUS_TIMEOUT);
				}
			} else {
				MV_ZeroMemory(device, sizeof(twsi_device_setting_t));
				core->lib_twsi.device[TWSI_CURRENT_DEVICESNUM] = NULL;
			}
			TWSI_ACCESS_STATE = TWSI_STATE_STOP;
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_NACK:
			TWSI_ACCESS_STATE = TWSI_STATE_FINISH;
			/*Workaround for Enclosure IDs.*/
			if (IS_NORMAL_TWSI_DEVICE(core, devices_info_list)) {
				if (!(devices_info_list->feature & TWSI_FEATURE_WITHOUT_NACK)) {
					twsiNackBitSet(core, port);
					break;
				}
			}
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_FINISH:
			if (TWSI_CURRENT_STATEMACHINE == TWSI_DEVICES_STATEMACHINE_STARTED) {
				if (ptr_req) {
					ptr_req->Scsi_Status = REQ_STATUS_SUCCESS;
				}
			} else  {
				TWSI_CURRENT_DEVICESNUM ++;
			}
			TWSI_ACCESS_STATE = TWSI_STATE_STOP;
			GO_NEXT_STEP;
			break;

		case TWSI_STATE_SES_STOP:
			if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
				MV_U8 temp;
				/* Generate stop bit */
				temp = READ_TWSI_REG8(core, port->id, TWSI_CONTROL);
				temp &= ~TWSI_CONTROL_INT_FLAG_SET;
				temp |= TWSI_CONTROL_STOP_BIT;
				WRITE_TWSI_REG8(core, port->id, TWSI_CONTROL, temp);

				TWSI_ACCESS_STATE = TWSI_STATE_DATA_RECEIVE;
			} else {
				TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
				GO_NEXT_STEP;
			}
			break;

		case TWSI_STATE_STOP:
			mvTwsisStopBitSet(core, port);

			/*Cancel the I2C Timer*/
			if (port->timer_num != NO_CURRENT_TIMER){
				Timer_CancelRequest (hba_ptr, port->timer_num);
				port->timer_num = NO_CURRENT_TIMER;
			}

			port->current_working_device = NULL;
			port->running_req = NULL;

			if (TWSI_CURRENT_STATEMACHINE == TWSI_DEVICES_STATEMACHINE_STARTED) {
				if (ptr_req->Completion) {
#if defined(SUPPORT_I2C)
					domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
					if ((i2c_link->i2c_request) && (port->id == EXT_I2C_DEV_PORT) && (devices_info_list->slave_target == I2C_SEP_ADDR0)) {
//						TWSI_PRINT("%s %d %s Enclosure command callback\n", __FILE__, __LINE__, __FUNCTION__);
						i2c_to_twsi_call_back(core, device);
					} else
#endif
					{
//						TWSI_PRINT("%s %d %s Normal command callback\n", __FILE__, __LINE__, __FUNCTION__);
						ptr_req->Completion(ptr_req->Cmd_Initiator, ptr_req);

						if (!(List_Empty(&port->waiting_queue))) {
							PMV_Request ptr_newreq = List_GetFirstEntry(&port->waiting_queue, MV_Request, Queue_Pointer);
							twsi_send_request(core, ptr_newreq);
						}
					}
				}
			} else  {
				/*Go to check next twsi device and wait 0.5 seconds.*/
				Timer_AddRequest(hba_ptr, 1, twsi_check_initial_devices_is_done, (MV_PVOID)core, NULL);;
			}
			break;

		default:
			TWSI_PRINT("<I2C ERROR> No this State.\n");
			TWSI_ACCESS_STATE = TWSI_STATE_FAIL;
			GO_NEXT_STEP;
			break;
		}

		TWSI_DBG_PRINT("%s %d %s go_next_step = 0x%x\n", __FILE__, __LINE__, __FUNCTION__, go_next_step);
		if (go_next_step) {
			TWSI_DBG_PRINT("%s %d %s Resatrt Step..\n", __FILE__, __LINE__, __FUNCTION__);
		}
	}while (go_next_step);

	return MV_TRUE;
}

void twsi_initial_devices_statemachine(
	MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_U16 go_next_step;

	do {
		GO_OUT_WAITING_NEXT_STEP;
		TWSI_DBG_PRINT("<<**twsi_initial_devices_statemachine**>>TWSI_CURRENT_STATEMACHINE =>> %d...\n", TWSI_CURRENT_STATEMACHINE);

		switch (TWSI_CURRENT_STATEMACHINE) {
		case TWSI_DEVICES_STATEMACHINE_INIT:
			core->TWSI_SEARCHING_LIST = (MV_PVOID)twsi_devices_info_list;
			{
				MV_U16 i;
				TWSI_CURRENT_PORTSNUM = 0;
				for (i = 0; i < TWSI_MAX_PORT; i ++) {
					twsi_port_setting_t *port = &core->lib_twsi.port[i];

					port->id = i;
					port->twsi_state = TWSI_STATE_TIMING_INIT;
					port->link_rate = 100; /*100KHz*/
					port->slave_host = 0x5A; /*set Host Slave Address*/
					port->feature = 0x00;
					port->timer_num = NO_CURRENT_TIMER;
					port->current_working_device = NULL;
					port->running_req = NULL;

					MV_LIST_HEAD_INIT(&port->waiting_queue);
					TWSI_CURRENT_PORTSNUM ++;
				}

				TWSI_CURRENT_DEVICESNUM = 0;
				MV_LIST_HEAD_INIT(&core->lib_twsi.device_list);

				for (i = 0; i < TWSI_MAX_DEVICE_NUMBER; i ++) {
					twsi_device_setting_t *device = &core->lib_twsi.devices_source[i];
					core->lib_twsi.device[i] = NULL;
					MV_ZeroMemory(device, sizeof(twsi_device_setting_t));
					MV_LIST_HEAD_INIT(&device->queue_pointer);
					List_AddTail(&device->queue_pointer, &core->lib_twsi.device_list);
				}
				TWSI_CURRENT_DETECTCNT = 0;
			}
			
			TWSI_CURRENT_STATEMACHINE = TWSI_DEVICES_STATEMACHINE_START;
			GO_NEXT_STEP;
			break;

		case TWSI_DEVICES_STATEMACHINE_START:
			{
				twsi_device_info_t *devices_info_list = (twsi_device_info_t *)core->TWSI_SEARCHING_LIST;
				twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list[TWSI_CURRENT_DETECTCNT].port_id];
				twsi_device_setting_t *device = NULL;

				/*If NO resource, the sequence will be stop.*/
				if (
					(List_Empty(&core->lib_twsi.device_list)) || 
					((devices_info_list[TWSI_CURRENT_DETECTCNT].port_id == 0xFF) && (devices_info_list[TWSI_CURRENT_DETECTCNT].link_rate == 0xFFFF))
				) {
					TWSI_DBG_PRINT("No resource related device's structure. or finished the TWSI device detection.\n");
					TWSI_CURRENT_STATEMACHINE = TWSI_DEVICES_STATEMACHINE_STARTED;
					GO_NEXT_STEP;
					break;
				}

				if (devices_info_list[TWSI_CURRENT_DETECTCNT].port_id >= TWSI_MAX_PORT) {
					TWSI_DBG_PRINT("Port Number is fail on the TWSI devices list.\n");
					TWSI_CURRENT_DETECTCNT ++;
					GO_NEXT_STEP;
					break;
				}

				device = List_GetFirstEntry(&core->lib_twsi.device_list, twsi_device_setting_t, queue_pointer);
				/* Search all devices, re-set the port setting*/
				device->id = TWSI_CURRENT_DEVICESNUM;
				device->ptr_device_info = (MV_PVOID)&devices_info_list[TWSI_CURRENT_DETECTCNT];
				
				port->current_working_device = (MV_PVOID)device;
				if (IS_SES_TWSI_DEVICE(core, &devices_info_list[TWSI_CURRENT_DETECTCNT]))
					port->slave_host = 0xD0;
				else 
					port->slave_host = 0x5A;

				MV_ZeroMemory(port->slot, TWSI_DATA_MAX_SLOTS * sizeof(twsi_data_slot_t));
				port->max_slot_num = 0;
				{
					twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
					slot->addr = NULL;
					slot->size = 0;
					slot->flag = 0;
					SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
					SET_TO_TWSI_TRANSMIT_ADDRESS_SEQUENCE(slot->flag);
					SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
					port->max_slot_num ++;
				}
				port->current_slot_num = 0;
				port->TWSI_CURRENT_DATAADDR = 0x00L;
				
				core->lib_twsi.device[TWSI_CURRENT_DEVICESNUM] = device;
				
				TWSI_DBG_PRINT("TWSI_CURRENT_DETECTCNT =>> %d...\n", TWSI_CURRENT_DETECTCNT);

				TWSI_CURRENT_DETECTCNT ++;

				TWSI_ACCESS_STATE = TWSI_STATE_TIMING_INIT; /* set the i2c frequency first*/
				twsi_access_statemachine(core, device);
			}
			break;

		case TWSI_DEVICES_STATEMACHINE_STARTED:
			{
				MV_U16 i;
				TWSI_DBG_PRINT("TWSI has %d devices.\n", TWSI_CURRENT_DEVICESNUM);
				for (i = 0; i < TWSI_CURRENT_DEVICESNUM; i ++) {
					if (core->lib_twsi.device[i] != NULL) {
						twsi_device_setting_t *device = core->lib_twsi.device[i];
						twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
						twsi_port_setting_t *port = &core->lib_twsi.port[devices_info_list->port_id];
						TWSI_PRINT("TWSI Port%d ->Device%d[%s][0x%X]\n", port->id, device->id, devices_info_list->name, devices_info_list->slave_target);
					}
				}
			}
 //			TWSI_CURRENT_STATEMACHINE = TWSI_DEVICES_STATEMACHINE_INIT; /*For test. When the function will be okay, it will be removed.*/
//			GO_NEXT_STEP; /*For test. When the function will be okay, it will be removed.*/
			break;

		default:
			break;
		}

		TWSI_DBG_PRINT("%s %d %s go_next_step = 0x%x\n", __FILE__, __LINE__, __FUNCTION__, go_next_step);
		if (go_next_step) {
			TWSI_DBG_PRINT("%s %d %s Resatrt Step..\n", __FILE__, __LINE__, __FUNCTION__);
		}
	}while (go_next_step);

}

void twsi_check_initial_devices_is_done(MV_PVOID module_pointer, MV_PVOID temp)
{
	core_extension * core = (core_extension *)module_pointer;
	twsi_initial_devices_statemachine(core);
}

void twsi_send_request(MV_PVOID module_pointer, PMV_Request pmvreq) {
	core_extension * core = (core_extension *)module_pointer;
	twsi_port_setting_t *port = NULL;
	twsi_device_setting_t *device = NULL;
	twsi_device_info_t *devices_info_list = NULL;

	if ((pmvreq->Cdb[0] != APICDB0_ADAPTER) || (pmvreq->Cdb[1] != 0x5A)) {
		TWSI_DBG_PRINT("%s %d %s TWSI Error..\n", __FILE__, __LINE__, __FUNCTION__);
		pmvreq->Scsi_Status = REQ_STATUS_ERROR;
		if(pmvreq->Completion)
			pmvreq->Completion(pmvreq->Cmd_Initiator, pmvreq);
		return;
	}

	if (pmvreq->Data_Transfer_Length <= 0) {
		TWSI_DBG_PRINT("%s %d %s Data Size is 0.\n", __FILE__, __LINE__, __FUNCTION__);
		pmvreq->Scsi_Status = REQ_STATUS_SUCCESS;
		if(pmvreq->Completion)
			pmvreq->Completion(pmvreq->Cmd_Initiator, pmvreq);
		return;
	}

	if (!pmvreq->Data_Buffer) {
		TWSI_DBG_PRINT("%s %d %s No Data Buffer.\n", __FILE__, __LINE__, __FUNCTION__);
		pmvreq->Scsi_Status = REQ_STATUS_ERROR;
		if(pmvreq->Completion)
			pmvreq->Completion(pmvreq->Cmd_Initiator, pmvreq);
		return;
	}

	if (pmvreq->Context[0]) {
		device = (twsi_device_setting_t *)pmvreq->Context[0];
		devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
		port = &core->lib_twsi.port[devices_info_list->port_id];
	} else {
		TWSI_DBG_PRINT("%s %d %s No device information.\n", __FILE__, __LINE__, __FUNCTION__);
		pmvreq->Scsi_Status = REQ_STATUS_NO_DEVICE;
		if(pmvreq->Completion)
			pmvreq->Completion(pmvreq->Cmd_Initiator, pmvreq);
		return;
	}

	if (devices_info_list->port_id >= TWSI_MAX_PORT) {
		TWSI_DBG_PRINT("%s %d %s Port Number %d is fail on the TWSI devices list.\n", __FILE__, __LINE__, __FUNCTION__, devices_info_list->port_id);
		pmvreq->Scsi_Status = REQ_STATUS_NO_DEVICE;
		if(pmvreq->Completion)
			pmvreq->Completion(pmvreq->Cmd_Initiator, pmvreq);
		return;
	}


	if (port->running_req) {
		List_AddTail(&pmvreq->Queue_Pointer, &port->waiting_queue);
		return;
	}

	pmvreq->Scsi_Status = REQ_STATUS_PENDING;
	port->running_req = pmvreq;
	port->current_working_device = (MV_PVOID)device;
	port->TWSI_CURRENT_DATAADDR = (MV_U32)pmvreq->Cdb[4];			//Address 0
	port->TWSI_CURRENT_DATAADDR |= ((MV_U32)pmvreq->Cdb[5])<<8;	//Address 1
	port->TWSI_CURRENT_DATAADDR |= ((MV_U32)pmvreq->Cdb[6])<<16;	//Address 2
	port->TWSI_CURRENT_DATAADDR |= ((MV_U32)pmvreq->Cdb[7])<<24;	//Address 3

	port->current_slot_num = 0;
	port->max_slot_num = 0;
	MV_ZeroMemory(port->slot, TWSI_DATA_MAX_SLOTS * sizeof(twsi_data_slot_t));
	if (IS_SES_TWSI_DEVICE(core, devices_info_list)) {
		if (pmvreq->Cmd_Flag & CMD_FLAG_DATA_IN) {
			twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
			slot->addr = pmvreq->Data_Buffer;
			slot->size = 7;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (char *)pmvreq->Data_Buffer;
			slot->size = pmvreq->Data_Transfer_Length;
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;
		} else {
			twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
			slot->addr = pmvreq->Data_Buffer;
			slot->size = 6;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (char *)pmvreq->Data_Buffer + 8;
			slot->size = pmvreq->Data_Transfer_Length;
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;

			slot = &port->slot[port->max_slot_num];
			slot->addr = (char *)pmvreq->Data_Buffer;
			slot->size = 8 + pmvreq->Data_Transfer_Length;
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
			SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
			port->max_slot_num ++;
		}
	}
	else {
		twsi_data_slot_t *slot = &port->slot[port->max_slot_num];
		slot->addr = pmvreq->Data_Buffer;
		slot->size = pmvreq->Data_Transfer_Length;
		if (pmvreq->Cmd_Flag & CMD_FLAG_DATA_IN)
			SET_TO_TWSI_READ_COMMAND_SEQUENCE(slot->flag);
		else
			SET_TO_TWSI_WRITE_COMMAND_SEQUENCE(slot->flag);
		SET_TO_TWSI_TRANSMIT_ADDRESS_SEQUENCE(slot->flag);
		SET_TO_TWSI_RUNNING_SEQUENCE(slot->flag);
		port->max_slot_num ++;
	}
	port->slave_host = pmvreq->Cdb[10];

	TWSI_ACCESS_STATE = TWSI_STATE_TIMING_INIT; /* set the i2c frequency first*/
	twsi_access_statemachine(core, device);
}

MV_BOOLEAN 
twsi_isr_handler(MV_PVOID core_p)
{
    core_extension * core=(core_extension *)core_p;
    MV_U32 interrupt = 0x00L;
    MV_U32 temp;
    MV_U16 i;

    TWSI_DBG_PRINT("**********TWSI Interrupt  Start**********\n");
    /*Keep this interrupt in this teime.*/
    for (i = 0; i < TWSI_CURRENT_PORTSNUM; i ++) {
        temp = READ_TWSI_REG8(core, i, TWSI_CONTROL);
        if (temp & TWSI_CONTROL_INT_FLAG_SET) {
            interrupt |= MV_BIT(i);
        }
    }

    for (i = 0; i < TWSI_CURRENT_PORTSNUM; i ++) {
        if (interrupt & MV_BIT(i)) {
            twsi_port_setting_t *port = &core->lib_twsi.port[i];
            if (port->current_working_device != NULL) {
                 TWSI_DBG_PRINT("****TWSI Interrupt <port%d>\n", i);
                 twsi_access_statemachine(core, port->current_working_device);
            }
            else {
                 TWSI_DBG_PRINT("****TWSI Interrupt <port%d>, but the port no working device currently.\n", i);
            }
        }
        else {
            TWSI_DBG_PRINT("****No this TWSI Interrupt on<port%d>\n", i);
        }
    }
    TWSI_DBG_PRINT("**********TWSI Interrupt  End**********\n");
    return MV_TRUE;          
}

#endif /*SUPPORT_TWSI*/

