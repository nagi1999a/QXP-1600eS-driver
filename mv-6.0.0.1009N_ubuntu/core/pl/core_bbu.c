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
#include "core_bbu.h"
#ifdef SUPPORT_BBU
#include "mvCommon.h"
#include "mvTwsiSpec.h"
#include "mvTypes.h"
#include "core_twsi.h"

void i2c_port_init(
	core_extension * core, 
	MV_U8 port, 
	int speed, 
	int slaveaddr);
void core_notify_battery(
	MV_PVOID	core_ext,
	MV_BOOLEAN is_normal);

static MV_U8 g_twsi_detect_cnt = 0;
static MV_BOOLEAN first_get_bbu_info = MV_TRUE;
#if !defined(SUPPORT_TWSI)
static MV_U8 data[0x80];
#if 0
static MV_U8 prev_data[0x80], cur_data[0x80];
#endif
#endif
 #ifdef  I2C_ONE_BYTE_READ
 static MV_U8 g_index;
 #endif
 #if 0
static MV_U16 get_count = 0;
 #endif
void bbu_enable(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;

    /*
    Set GPIO2 and GPIO3 as high to disable charger/discharger
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(2) | MV_BIT(3));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(2) | MV_BIT(3) | MV_BIT(4);
    else
        reg &= ~(MV_BIT(2) | MV_BIT(3) | MV_BIT(4));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);

    /*
    Set GPIO4 as high to enable BBU
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(4));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(4);
    else
        reg &= ~(MV_BIT(4));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);

    /*
    Set GPIO5 as low then high to make latch enable
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg & (~MV_BIT(5)));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(5);
    else
        reg &= ~(MV_BIT(5));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);

    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(5));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(5);
    else
        reg &= ~(MV_BIT(5));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);

    FM_PRINT("JL %s Enable battery\n", __FUNCTION__);
}

void bbu_disable(MV_PVOID this)
{
    core_extension * core = (core_extension *)(core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;
    /*
    Set GPIO2 and GPIO3 as high to disable charger/discharger
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(2) | MV_BIT(3));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(2) | MV_BIT(3) | MV_BIT(4);
    else
        reg &= ~(MV_BIT(2) | MV_BIT(3) | MV_BIT(4));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);
    /*
    Set GPIO4 as low to disable BBU
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg & (~MV_BIT(4)));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(4);
    else
        reg &= ~(MV_BIT(4));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);
    /*
    Set GPIO5 as low then high to make latch enable
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg & (~MV_BIT(5)));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(5);
    else
        reg &= ~(MV_BIT(5));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);
	
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(5));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(5);
    else
        reg &= ~(MV_BIT(5));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
    HBA_SleepMillisecond(core, 5);

    FM_PRINT("JL %s Disable battery\n", __FUNCTION__);
}

void bbu_enable_charger(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;

    /*
    Set GPIO2 as high to disable discharger, GPIO3 as low to enable charger
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    reg |=  MV_BIT(2);
    reg &=  ~MV_BIT(3);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg);
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(3);
    else
        reg &= ~(MV_BIT(3));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);

    FM_PRINT("JL %s Enable battery charger.\n", __FUNCTION__);
}

void bbu_disable_charger(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;

    /*
    Set GPIO 3 as high to disable charger
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(3));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(3);
    else
        reg &= ~(MV_BIT(3));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);

    FM_PRINT("JL %s Disable battery charger.\n", __FUNCTION__);
}

void bbu_enable_discharger(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;

    /*
    Set GPIO2 as low to enable discharger, GPIO3 as high to disable charger.
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    reg &= ~MV_BIT(2);
    reg |= MV_BIT(3);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg);
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(2);
    else
        reg &= ~(MV_BIT(2));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);

    FM_PRINT("JL %s Enable battery discharger.\n", __FUNCTION__);
}

void bbu_disable_discharger(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;

    /*
    Set GPIO2 as high to disable discharger
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg | MV_BIT(2));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(2);
    else
        reg &= ~(MV_BIT(2));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);

    FM_PRINT("JL %s Disable battery discharger\n", __FUNCTION__);
}

void bbu_set_dirty_stripe_led(MV_PVOID this)
{
    core_extension * core = (core_extension *)HBA_GetModuleExtension(this, MODULE_CORE);
    MV_U32 reg;
    
    /*
    Set GPIO30 as low to indicate there're dirty stripe
    */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, reg & (~MV_BIT(30)));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(30);
    else
        reg &= ~(MV_BIT(30));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);
}

#if !defined(SUPPORT_TWSI)
static MV_VOID dump_status(core_extension * core, MV_U8 port)
{
    MV_U8 I2cCtlReg,I2cStatusReg;

    I2cCtlReg = read_i2c_register(core, port, MI2C_CONTROL);
    FM_PRINT("JL %s P[%d].I2cCtlReg = 0x%X\n", __FUNCTION__, port, I2cCtlReg);
    I2cStatusReg = read_i2c_register(core, port, MI2C_STATUS);
    FM_PRINT("JL %s P[%d].I2cStatusReg = 0x%X\n", __FUNCTION__, port, I2cStatusReg);
}


/*******************************************************************************
* twsiMainIntGet - Get twsi bit from main Interrupt cause.
*
* DESCRIPTION:
*       This routine returns the twsi interrupt flag value.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE is interrupt flag is set, MV_FALSE otherwise.
*
*******************************************************************************/
static MV_BOOLEAN twsiMainIntGet(core_extension * core, MV_U8 port)
{
    MV_U32 temp;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C get INT flag\n");
    #endif

    /* get the int flag bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    if (temp & TWSI_CONTROL_INT_FLAG_SET)
        return MV_TRUE;
    return MV_FALSE;
}


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
static void twsiIntFlgClr(core_extension * core, MV_U8 port)
{
    MV_U8 temp;
    MV_U16 counter = 5000;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C clear INT flag\n");
    #endif

    /* clear the int flag bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, temp & ~TWSI_CONTROL_INT_FLAG_SET);

    /* Add delay of 1ms */
    //HBA_SleepMillisecond(core, 1);
    HBA_SleepMicrosecond(core, 10);

    while ((counter > 0) && (!(read_i2c_register(core, port, MI2C_CONTROL) & TWSI_CONTROL_INT_FLAG_SET))) {
        //HBA_SleepMillisecond(core, 1);
        HBA_SleepMicrosecond(core, 10);
	counter --;
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
static MV_VOID twsiAckBitSet(core_extension * core, MV_U8 port)
{
    MV_U32 temp;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C set ACK\n");
    #endif

    /*Set the Ack bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, temp | TWSI_CONTROL_ACK);

    /* Add delay of 100 us */
    HBA_SleepMicrosecond(core, 10);
    return;
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
MV_STATUS mvTwsisStopBitSet(core_extension * core, MV_U8 port)
{
    MV_U32 temp;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C set Stop\n");
    #endif

    /* Generate stop bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, temp | TWSI_CONTROL_STOP_BIT);

    #ifdef I2C_DELAY
    //HBA_SleepMillisecond(core, 1);
    HBA_SleepMicrosecond(core, 10);
    #endif
    /* clear Int flag */
    twsiIntFlgClr(core, port);
    #ifdef I2C_DELAY
    //HBA_SleepMillisecond(core, 1);
    HBA_SleepMicrosecond(core, 10);
    #endif

    return MV_OK;
}

static MV_U8 twsiWaitStatusUpdate(core_extension * core, MV_U8 port)
{
    MV_U32 counter = 5000;
    MV_U8 temp;

    /*waiting the Ack bit */
    while (counter > 0) {
	 temp = read_i2c_register(core, port, MI2C_STATUS);
	 if (temp == MI2C_STATUS_START_TX)
	     break;
	 else if (temp == MI2C_STATUS_ADDR_W_TX_ACK)
	     break;
	 else if (temp == MI2C_STATUS_MDATA_TX_ACK)
	     break;
	 else if (temp == MI2C_STATUS_ADDR_R_TX_ACK)
	     break;
	 else if (temp == MI2C_STATUS_MDATA_RX_ACK)
	     break;
	 else if (temp == MI2C_STATUS_REP_START_TX)
	     break;
	 
        //HBA_SleepMillisecond(core, 1);
        HBA_SleepMicrosecond(core, 10);
        counter --;
    }

    if (counter == 0)
        return 1;
    return 0;
}

static void dump_i2c_register_status(core_extension * core, MV_U8 port)
{
    MV_U8 I2cCtlReg,I2cStatusReg;

    I2cCtlReg = read_i2c_register(core, port, MI2C_CONTROL);
    FM_PRINT("TWSI[%d].I2cCtlReg = 0x%X\n", port, I2cCtlReg);
    I2cStatusReg = read_i2c_register(core, port, MI2C_STATUS);
    FM_PRINT("TWSI[%d].I2cStatusReg = 0x%X\n", port, I2cStatusReg);
}

static void twsiNackBitSet(core_extension * core, MV_U8 port)
{
    MV_U8 temp;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C set NACK\n");
    #endif

    /*Set the Ack bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, temp & ~TWSI_CONTROL_ACK);

    /* Add delay of 1ms */
    //HBA_SleepMillisecond(core, 1);
    HBA_SleepMicrosecond(core, 10);

    twsiIntFlgClr(core, port);
}

void twsiStopStep(core_extension * core, MV_U8 port, MV_U8 command)
{
    if (command == MV_TWSI_READ) {
        twsiNackBitSet(core, port);
    }

    mvTwsisStopBitSet(core, port);

    HBA_SleepMillisecond(core, 1);//this delay can't be deleted.

    write_i2c_register(core, port, MI2C_CONTROL, 0x00);
}

MV_BOOLEAN twsiTimeoutChk(MV_U32 timeout, MV_U8 *pString)
{
    if (timeout >= TWSI_TIMEOUT_VALUE) {
        FM_PRINT("JL %s %s\n", __FUNCTION__, pString);
        return MV_TRUE;
    }
    return MV_FALSE;
}

/*******************************************************************************
* twsiStsGet - Get the TWSI status value.
*
* DESCRIPTION:
*       This routine returns the TWSI status value.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_U32 - the TWSI status.
*
*******************************************************************************/
static MV_U32 twsiStsGet(core_extension * core, MV_U8 port)
{
    #ifdef I2C_DBG
    FM_PRINT("JL I2C get Status\n");
    #endif

    return read_i2c_register(core, port, MI2C_STATUS);
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
MV_STATUS mvTwsisStartBitSet(core_extension * core,  MV_U8 port)
{
    MV_BOOLEAN isIntFlag = MV_FALSE;
    MV_U32 temp;
    
    /* check Int flag */
    if (twsiMainIntGet(core, port))
        isIntFlag = MV_TRUE;

    #ifdef I2C_DBG
    FM_PRINT("JL I2C set Start\n");
    #endif

    /* set start Bit */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, temp | TWSI_CONTROL_START_BIT);

    if (isIntFlag) {
        twsiIntFlgClr(core, port);
    }

    #if 0
    //Only use in probe device
    /* wait for interrupt */
    timeout = 0;
    while (!twsiMainIntGet(core, port) && (timeout++ < TWSI_TIMEOUT_VALUE));

    /* check for timeout */	
    if (MV_TRUE == twsiTimeoutChk(timeout,"TWSI: mvTwsiStartBitSet ERROR - Start Clear bit TimeOut .\n")) 
        return MV_TIMEOUT;
	

    /* check that start bit went down */
    if ((read_i2c_register(core, port, MI2C_CONTROL) & TWSI_CONTROL_START_BIT) != 0) {
        FM_PRINT("JL %s %d %s ... mvTwsiStartBitSet ERROR - start bit didn't went down\n", __FILE__, __LINE__, __FUNCTION__);
        return MV_FAIL;
    }	

    temp = twsiStsGet(core, port);
    FM_PRINT("JL %s %d %s ...status = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, temp);
    if (( temp != TWSI_START_CON_TRA) && ( temp != TWSI_REPEATED_START_CON_TRA)) {
        FM_PRINT("JL %s %d %s ... mvTwsiStartBitSet ERROR - status %x after Set Start Bit.\n", __FILE__, __LINE__, __FUNCTION__, temp);
        return MV_FAIL;
    }
    #endif
    //_MARVELL_SDK_PACKAGE_NONRAID

    return MV_OK;	
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
static MV_STATUS twsiAddr10BitSet(core_extension * core,  MV_U8 port, MV_U32 deviceAddress, MV_TWSI_CMD command)
{
    MV_U32 val;

    /* writing the 2 most significant bits of the 10 bit address*/
    val = ((deviceAddress & TWSI_DATA_ADDR_10BIT_MASK) >> TWSI_DATA_ADDR_10BIT_OFFS );
    /* bits 7:3 must be 0x11110 */
    val |= TWSI_DATA_ADDR_10BIT_CONST;
    /* set command */
    val |= command;
    write_i2c_register(core, port, MI2C_DATA, val);
    /* WA add a delay */
    HBA_SleepMicrosecond(core, 100);

    /* clear Int flag */
    twsiIntFlgClr(core, port);

    #if 0
    /* wait for Int to be Set */
    timeout = 0;
    while (!twsiMainIntGet(core, port) && (timeout++ < TWSI_TIMEOUT_VALUE));

    /* check for timeout */
    if (MV_TRUE == twsiTimeoutChk(timeout, "TWSI: twsiAddr10BitSet ERROR - 1st addr (10Bit) Int TimeOut.\n"))
    	return MV_TIMEOUT;

    /* check the status */
    val = twsiStsGet(core, port);
    if (((val != TWSI_AD_PLS_RD_BIT_TRA_ACK_REC) && (command == MV_TWSI_READ)) || 
        ((val != TWSI_AD_PLS_WR_BIT_TRA_ACK_REC) && (command == MV_TWSI_WRITE))) {
        FM_PRINT("JL %s %d %s ... twsiAddr10BitSet ERROR - status %x 1st addr (10 Bit) in %s mode.\n", __FILE__, __LINE__, __FUNCTION__, \
            val, ((command==MV_TWSI_WRITE)?"Write":"Read"));
        return MV_FAIL;
    }

    /* set 	8 LSB of the address */
    val = (deviceAddress << TWSI_DATA_ADDR_7BIT_OFFS) & TWSI_DATA_ADDR_7BIT_MASK;
    write_i2c_register(core, port, MI2C_DATA, val);

    /* clear Int flag */
    twsiIntFlgClr(core, port);

    /* wait for Int to be Set */
    timeout = 0;
    while (!twsiMainIntGet(core, port) && (timeout++ < TWSI_TIMEOUT_VALUE));

    /* check for timeout */
    if (MV_TRUE == twsiTimeoutChk(timeout,"TWSI: twsiAddr10BitSet ERROR - 2nd (10 Bit) Int TimOut.\n"))
        return MV_TIMEOUT;
	
    /* check the status */
    val = twsiStsGet(core, port);
    if (((val != TWSI_SEC_AD_PLS_RD_BIT_TRA_ACK_REC) && (command == MV_TWSI_READ)) || 
        ((val != TWSI_SEC_AD_PLS_WR_BIT_TRA_ACK_REC) && (command == MV_TWSI_WRITE))) {
        FM_PRINT("JL %s %d %s ... twsiAddr10BitSet ERROR - status %x 2nd addr(10 Bit) in %s mode.\n", __FILE__, __LINE__, __FUNCTION__, \
            val, ((command==MV_TWSI_WRITE)?"Write":"Read"));
        return MV_FAIL;
    }
    #endif
    //_MARVELL_SDK_PACKAGE_NONRAID

    return MV_OK;
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
static MV_STATUS twsiAddr7BitSet(core_extension * core, MV_U8 port, MV_U32 deviceAddress, MV_TWSI_CMD command)
{
    MV_U32 val;

    /* set the address */
    val = (deviceAddress << TWSI_DATA_ADDR_7BIT_OFFS) & TWSI_DATA_ADDR_7BIT_MASK;
    /* set command */
    val |= command;
    write_i2c_register(core, port, MI2C_DATA, val);
    /* WA add a delay */
    HBA_SleepMicrosecond(core, 100);


    /* clear Int flag */
    twsiIntFlgClr(core, port);

    return MV_OK;
}

MV_STATUS mvTwsisAddrSet(core_extension * core, MV_U8 port, twsi_addr *pTwsiAddr, MV_TWSI_CMD command)
{
    #ifdef I2C_DBG
    FM_PRINT("JL I2C Addr set\n");
    #endif

    /* 10 Bit address */
    if (pTwsiAddr->type == ADDR10_BIT) {
        return twsiAddr10BitSet(core, port, pTwsiAddr->address, command);
    } else {
         /* 7 Bit address */
        return twsiAddr7BitSet(core, port, pTwsiAddr->address,command);
    }
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
static MV_STATUS twsiDataTransmit(core_extension * core, MV_U8 port, MV_U8 *pBlock)
{

    if (NULL == pBlock) {
        FM_PRINT("JL %s NULL == pBlock\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }

    #ifdef I2C_DBG
    FM_PRINT("JL I2C Data transmit\n");
    #endif

    if (core->lib_bbu.bbu_addr.xfer_count < core->lib_bbu.bbu_addr.xfer_length) {

        /* write the data*/
        write_i2c_register(core, port, MI2C_DATA, (MV_U32)*pBlock);
        core->lib_bbu.bbu_addr.xfer_count++;
        
        /* clear Int flag */
        twsiIntFlgClr(core, port);
    }
    
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
static MV_STATUS twsiDataReceive(core_extension * core,  MV_U8 port, MV_U8 *pBlock)
{
    MV_U32 temp;

    #if 1
    if (NULL == pBlock) {
        FM_PRINT("JL %s NULL == pBlock\n", __FUNCTION__);
        return MV_BAD_PARAM;
    }


    #ifdef I2C_ONE_BYTE_READ
        /* clear ack and Int flag */
        temp = read_i2c_register(core, port, MI2C_CONTROL);
        temp &=  ~(TWSI_CONTROL_ACK);
        write_i2c_register(core, port, MI2C_CONTROL, temp);

        /* clear Int flag */
        twsiIntFlgClr(core, port);
    #else
        //Incremental Read
        /* clear Int flag */
        //Do this first will make set Addr Rd sent
        twsiIntFlgClr(core, port);

        #ifdef I2C_DBG
        FM_PRINT("JL I2C Data Recv\n");
        #endif
        /* read the data*/
        *pBlock = (MV_U8)read_i2c_register(core, port, MI2C_DATA);
        core->lib_bbu.bbu_addr.xfer_count++;

        if (core->lib_bbu.bbu_addr.xfer_length - core->lib_bbu.bbu_addr.xfer_count)
            twsiAckBitSet(core, port);

        //HBA_SleepMillisecond(core, 1);
        #ifdef I2C_DELAY
        HBA_SleepMicrosecond(core, 10);
        #endif
    #endif
    
    #else
    #if 0
    if (NULL == pBlock) {
        FM_PRINT("JL %s %d %s ... NULL == pBlock\n", __FILE__, __LINE__, __FUNCTION__);
        return MV_BAD_PARAM;
    }

    /* clear ack and Int flag */
    temp = read_i2c_register(core, port, MI2C_CONTROL);
    temp &=  ~(TWSI_CONTROL_ACK);
    write_i2c_register(core, port, MI2C_CONTROL, temp);

    /* clear Int flag */
    twsiIntFlgClr(core, port);

    /* read the data*/
    *pBlock = (MV_U8)read_i2c_register(core, port, MI2C_DATA);
    core->lib_bbu.bbu_addr.xfer_count++;
    #endif
    #endif
    #if 0
    if (NULL == pBlock) {
        FM_PRINT("JL %s %d %s ... NULL == pBlock\n", __FILE__, __LINE__, __FUNCTION__);
        return MV_BAD_PARAM;
    }

    do {
        if ((core->lib_bbu.bbu_addr.xfer_length - core->lib_bbu.bbu_addr.xfer_count) == 1) {
            /* clear ack and Int flag */
            temp = read_i2c_register(core, port, MI2C_CONTROL);
            temp &=  ~(TWSI_CONTROL_ACK);
            write_i2c_register(core, port, MI2C_CONTROL, temp);
        }
        
        /* clear Int flag */
        twsiIntFlgClr(core, port);
        
        /* read the data*/
        *pBlock = (MV_U8)read_i2c_register(core, port, MI2C_DATA);
        core->lib_bbu.bbu_addr.xfer_count++;
        pBlock++;
    } while (core->lib_bbu.bbu_addr.xfer_count < core->lib_bbu.bbu_addr.xfer_length);
    #endif

    return MV_OK;

}

/*******************************************************************************
* twsiTargetOffsSet - Set TWST target offset on TWSI bus.
*
* DESCRIPTION:
*       The function support TWSI targets that have inside address space (for
*       example EEPROMs). The function:
*       1) Convert the given offset into pBlock and size.
*		in case the offset should be set to a TWSI slave which support 
*		more then 256 bytes offset, the offset setting will be done
*		in 2 transactions.
*       2) Use twsiDataTransmit to place those on the bus.
*
* INPUT:
*       offset - offset to be set on the EEPROM device.
*	more_than_256 - whether the EEPROM device support more then 256 byte offset. 
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK - if setting the offset completed succesfully.
*	MV_FAIL otherwmise.
*
*******************************************************************************/
static MV_STATUS twsiTargetOffsSet(core_extension * core, MV_U8 port, MV_U32 offset, MV_BOOLEAN more_than_256)
{
    MV_U8 offBlock[2];
    MV_U32 offSize;

    if (more_than_256 == MV_TRUE) {
        offBlock[0] = (offset >> 8) & 0xff;
        offBlock[1] = offset & 0xff;
        offSize = 2;
    } else {
        offBlock[0] = offset & 0xff;
        offSize = 1;
    }

    return MV_OK;
}

int bbu_polling_read(
    core_extension * core,
    MV_U8 port,
    MV_U32 wordaddr, 
    MV_U32 length, 
    MV_U8 *p_data)
{
    MV_U8 data[2];
    MV_U16 i;

    core->lib_bbu.bbu_addr.slave_addr.address = (BBU_SLAVE_ADDR >> 1);
    core->lib_bbu.bbu_addr.slave_addr.type = ADDR7_BIT;

    i2c_port_init(core, port, I2C_BBU_LINK_RATE, TWSI_BBU_PORT);

    mvTwsisStartBitSet(core, port);
    if (twsiWaitStatusUpdate(core, port)) {
        FM_PRINT("Start bit is fail.\n");
        dump_i2c_register_status(core, port);
        twsiStopStep(core, port, MV_TWSI_READ);
        return 1;
    }
//dump_i2c_register_status(core, port);
    mvTwsisAddrSet(core, port, &(core->lib_bbu.bbu_addr.slave_addr), MV_TWSI_WRITE);
    if (twsiWaitStatusUpdate(core, port)) {
        FM_PRINT("To set Slave Address is fail.\n");
        dump_i2c_register_status(core, port);
        twsiStopStep(core, port, MV_TWSI_READ);
        return 1;
    }
//dump_i2c_register_status(core, port);
    data[0] = (MV_U8)wordaddr;
    twsiDataTransmit(core, port, &data[0]);
    if (twsiWaitStatusUpdate(core, port)) {
        FM_PRINT("To set Offset is fail.\n");
        dump_i2c_register_status(core, port);
        twsiStopStep(core, port, MV_TWSI_READ);
        return 1;
    }
//dump_i2c_register_status(core, port);
    mvTwsisStartBitSet(core, port);
    if (twsiWaitStatusUpdate(core, port)) {
        FM_PRINT("Repeated start bit is fail.\n");
        dump_i2c_register_status(core, port);
        twsiStopStep(core, port, MV_TWSI_READ);
        return 1;
    }
//dump_i2c_register_status(core, port);
    mvTwsisAddrSet(core, port, &(core->lib_bbu.bbu_addr.slave_addr), MV_TWSI_READ);
    if (twsiWaitStatusUpdate(core, port)) {
        FM_PRINT("Address + read bit transmitted, acknowledge not received.\n");
        dump_i2c_register_status(core, port);
        twsiStopStep(core, port, MV_TWSI_READ);
        return 1;
    }

//    FM_PRINT("data>>");
    for (i = 0; i < length; i++) {
        twsiAckBitSet(core, port);
        if (twsiWaitStatusUpdate(core, port)) {
            FM_PRINT("Master received read data, acknowledge not transmitted.\n");
            dump_i2c_register_status(core, port);
            twsiStopStep(core, port, MV_TWSI_READ);
            return 1;
        }
//dump_i2c_register_status(core, port);
        twsiDataReceive(core, port, &data[0]);
        *(p_data + i) = data[0];
//        FM_PRINT("0x%x,", data[0]);
    }
//    FM_PRINT("\n");
    twsiStopStep(core, port, MV_TWSI_READ);
    return 0;
}

#endif /*!SUPPORT_TWSI*/

void bbu_protect_mechanism(PHBA_Extension hba_ptr)
{
    core_extension * core = HBA_GetModuleExtension(hba_ptr, MODULE_CORE);

    if (hba_ptr->bbu.status & BBU_STATUS_CHARGING)
        bbu_disable_charger(core);

    if (hba_ptr->bbu.status & BBU_STATUS_DISCHARGE)
        bbu_disable_discharger(core);

    hba_ptr->bbu.status &= ~(BBU_STATUS_CHARGING | BBU_STATUS_DISCHARGE);
}

void i2c_bbu_check_status(
	core_extension * core,
	MV_U8 *data
	)
{
    MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
    MV_U16 tmp = 0, i;
    MV_U32 reg = 0;
    MV_U32 flag = 0;
    static MV_BOOLEAN cap_normal = MV_TRUE;
    static MV_BOOLEAN temp_normal = MV_TRUE;
    static MV_BOOLEAN volt_normal = MV_TRUE;
    static MV_BOOLEAN ever_fail = MV_FALSE;

    if (core->lib_bbu.bbu_state == BBU_STATE_ERROR)
        return;

    #if 0
    //Dump all register
    for (i = 0; i < 0x2D; i++) {
        FM_PRINT("[%.2X] 0x%X ", i, data[i]);
        if (((i + 1) % 0x10) == 0)
            FM_PRINT("\n");
    }
    #if 0
    for (i = 0x6E; i < 0x80; i++)
        FM_PRINT("data[%X] = 0x%X ", i, data[i]);
    #endif
    FM_PRINT("\n");

    #endif
    //_MARVELL_SDK_PACKAGE_NONRAID

    #if 0
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    FM_PRINT("JL GPIO_DATA_OUT 0x%X\n", reg);
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    FM_PRINT("JL GPIO_DATA_OUT_EN_CTRL 0x%X\n", reg);
    #endif
    //_MARVELL_SDK_PACKAGE_NONRAID
    hba_ptr->bbu.flags = data[BBU_REG_FLAGS];
    hba_ptr->bbu.percentage= data[BBU_REG_RSOC];
    hba_ptr->bbu.voltage = 256 * data[BBU_REG_VOLT_HI] + data[BBU_REG_VOLT_LO];
    hba_ptr->bbu.temperature = (25 * (256 * data[BBU_REG_TEMP_HI] + data[BBU_REG_TEMP_LO])) - (27316);
    hba_ptr->bbu.time_to_empty = 256 * data[BBU_REG_TTE_HI] + data[BBU_REG_TTE_LO];
    hba_ptr->bbu.time_to_full = 256 * data[BBU_REG_TTF_HI] + data[BBU_REG_TTF_LO];
    //hba_ptr->bbu.maxCapacity = 3.75 * (256 * data[BBU_REG_LMD_HI] + data[BBU_REG_LMD_LO]) / 20;
    //hba_ptr->bbu.curCapacity = 3.75 * (256 * data[BBU_REG_NAC_HI] + data[BBU_REG_NAC_LO]) / 20;
    hba_ptr->bbu.maxCapacity = (256 * data[BBU_REG_LMD_HI] + data[BBU_REG_LMD_LO]) * 15 / 4 / 20;
    hba_ptr->bbu.curCapacity = (256 * data[BBU_REG_NAC_HI] + data[BBU_REG_NAC_LO]) * 15 / 4 / 20;
    hba_ptr->bbu.recharge_cycle = 256 * data[BBU_REG_CYCT_HI] + data[BBU_REG_CYCT_LO];

    //first_get_bbu_info = MV_TRUE;
    if (first_get_bbu_info) {
        FM_PRINT("=================BBU Info=================\n");
        FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
        FM_PRINT("BBU status = 0x%X\n", hba_ptr->bbu.status);
        FM_PRINT("Current Capacity = %dmAh\n", hba_ptr->bbu.curCapacity);
        FM_PRINT("Max Capacity = %dmAh\n", hba_ptr->bbu.maxCapacity);
        FM_PRINT("Percentage = %d%%\n", hba_ptr->bbu.percentage);
        FM_PRINT("Recharge Cycle Times = %d times\n", hba_ptr->bbu.recharge_cycle);
        FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
        FM_PRINT("Temperature = %d.%d Celsius\n", hba_ptr->bbu.temperature/100, MV_ABS(hba_ptr->bbu.temperature%100));
        FM_PRINT("Time to Full = %d min\n", hba_ptr->bbu.time_to_full);
        FM_PRINT("Time to Empty = %d min\n", hba_ptr->bbu.time_to_empty);
        FM_PRINT("==========================================\n");
    }
    
    //Check Battery Capacity
    if (hba_ptr->bbu.percentage < hba_ptr->bbu.percent_to_charge) {
        if (!(hba_ptr->bbu.status & (BBU_STATUS_CHARGING | BBU_STATUS_DISCHARGE | BBU_STATUS_POWER_STOP_ALL))){
            hba_ptr->bbu.status |= BBU_STATUS_CHARGING;
            FM_PRINT("BBU charging\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Percentage = %d%%\n", hba_ptr->bbu.percentage);
            FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
            FM_PRINT("Time to Full = %d min\n", hba_ptr->bbu.time_to_full);
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);

            bbu_enable_charger(core);
            //change cache policy to write-through
#ifdef CACHE_MODULE_SUPPORT
            core_notify_battery((MV_PVOID)core, MV_FALSE);
            core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
#endif
            core_generate_event(core, EVT_ID_BAT_CAPACITY_BELOW_THRESHOLD, 0, SEVERITY_WARNING,  0,  NULL, 0 );
        }
        hba_ptr->bbu.status &= ~(BBU_STATUS_GREATER_LOWERBOUND | BBU_STATUS_FULL_CHARGED);
        hba_ptr->bbu.status |= BBU_STATUS_LOW_BATTERY;
        cap_normal = MV_FALSE;
        ever_fail = MV_TRUE;
    }
    else if (hba_ptr->bbu.percentage >= BBU_CAPACITY_FULL) { 
        if (!(hba_ptr->bbu.status & (BBU_STATUS_FULL_CHARGED | BBU_STATUS_DISCHARGE))) {
            hba_ptr->bbu.status |= BBU_STATUS_FULL_CHARGED;
            bbu_disable_charger(core);
            if (!first_get_bbu_info) {
                FM_PRINT("BBU charged complete\n");
                FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
                FM_PRINT("Percentage = %d%%\n", hba_ptr->bbu.percentage);
                FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
                FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);
                
                core_generate_event(core, EVT_ID_BAT_CHARGE_CMPLT, 0, SEVERITY_INFO,  0,  NULL, 0 );
            }
        }
        hba_ptr->bbu.status &= ~(BBU_STATUS_LOW_BATTERY | BBU_STATUS_CHARGING | BBU_STATUS_GREATER_LOWERBOUND);
    }
    else {
        // BBU_CAPACITY_CHARGE_BOUND < percentage
        if ((hba_ptr->bbu.status & BBU_STATUS_CHARGING)
            && (!(hba_ptr->bbu.status & BBU_STATUS_GREATER_LOWERBOUND))) {
            hba_ptr->bbu.status |= BBU_STATUS_GREATER_LOWERBOUND;
            cap_normal = MV_TRUE;
            FM_PRINT("BBU charging just over lower bound\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Percentage = %d%%\n", hba_ptr->bbu.percentage);
            FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);
        }
        hba_ptr->bbu.status &= ~(BBU_STATUS_LOW_BATTERY | BBU_STATUS_FULL_CHARGED);
    }

    if ((hba_ptr->bbu.status & BBU_STATUS_DISCHARGE)
        && (hba_ptr->bbu.percentage <= BBU_CAPACITY_STOP_DISCHARGE_BOUND)) {
        FM_PRINT("BBU discharge complete.\n");
        bbu_disable_discharger(core);
        hba_ptr->bbu.status &= ~(BBU_STATUS_DISCHARGE | BBU_STATUS_CHARGING | BBU_STATUS_FULL_CHARGED | BBU_STATUS_POWER_STOP_ALL);
    }
    
    //Check Battery Temperature
    if ((hba_ptr->bbu.temperature / 100) <= (hba_ptr->bbu.temp_lowerbound  - 1)) {
        if (!(hba_ptr->bbu.status & BBU_STATUS_UNDER_TEMP_ERROR)) {
            FM_PRINT("BBU temp too low\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Temperature = %d.%d Celsius\n", hba_ptr->bbu.temperature/100, MV_ABS(hba_ptr->bbu.temperature%100));
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);

            bbu_protect_mechanism(hba_ptr);
#ifdef CACHE_MODULE_SUPPORT
            core_notify_battery((MV_PVOID)core, MV_FALSE);
            core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
#endif
            core_generate_event(core, EVT_ID_BAT_TEMP_IS_LOW, 0, SEVERITY_WARNING,  0,  NULL, 0 );
        }
        hba_ptr->bbu.status |= BBU_STATUS_UNDER_TEMP_ERROR;
        temp_normal = MV_FALSE;
        ever_fail = MV_TRUE;
    } else if ((hba_ptr->bbu.temperature / 100) >= hba_ptr->bbu.temp_upperbound) {
        if (!(hba_ptr->bbu.status & BBU_STATUS_OVER_TEMP_ERROR)) {
            FM_PRINT("BBU temp too high\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Temperature = %d.%d Celsius\n", hba_ptr->bbu.temperature/100, MV_ABS(hba_ptr->bbu.temperature%100));
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);

            bbu_protect_mechanism(hba_ptr);
#ifdef CACHE_MODULE_SUPPORT
            core_notify_battery((MV_PVOID)core, MV_FALSE);
            core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
#endif
            core_generate_event(core, EVT_ID_BAT_TEMP_IS_HIGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
        }
        hba_ptr->bbu.status |= BBU_STATUS_OVER_TEMP_ERROR;
        temp_normal = MV_FALSE;
        ever_fail = MV_TRUE;
    } else {
        //Temp normal
        hba_ptr->bbu.status &= ~(BBU_STATUS_UNDER_TEMP_ERROR | BBU_STATUS_OVER_TEMP_ERROR);
    }

    //Check Battery Voltage
    if (hba_ptr->bbu.voltage < hba_ptr->bbu.volt_lowerbound) {
        if (!(hba_ptr->bbu.status & BBU_STATUS_UNDER_VOLT_ERROR)) {
            FM_PRINT("BBU volt too low\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);

//JL suggestion            bbu_protect_mechanism(hba_ptr);
#ifdef CACHE_MODULE_SUPPORT
            core_notify_battery((MV_PVOID)core, MV_FALSE);
            core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
#endif
            core_generate_event(core, EVT_ID_BAT_VOLTAGE_LOW, 0, SEVERITY_WARNING,  0,  NULL, 0 );
        }
        hba_ptr->bbu.status |= BBU_STATUS_UNDER_VOLT_ERROR;
        volt_normal = MV_FALSE;
        ever_fail = MV_TRUE;
    }
    else if (hba_ptr->bbu.voltage > hba_ptr->bbu.volt_upperbound) {
        if (!(hba_ptr->bbu.status & BBU_STATUS_OVER_VOLT_ERROR)) {
            FM_PRINT("BBU volt too high\n");
            FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
            FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
            FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);

            bbu_protect_mechanism(hba_ptr);
#ifdef CACHE_MODULE_SUPPORT
            core_notify_battery((MV_PVOID)core, MV_FALSE);
            core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
#endif
            core_generate_event(core, EVT_ID_BAT_VOLTAGE_HIGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
        }
        hba_ptr->bbu.status |= BBU_STATUS_OVER_VOLT_ERROR;
        volt_normal = MV_FALSE;
        ever_fail = MV_TRUE;
    } else {
        hba_ptr->bbu.status &= ~(BBU_STATUS_UNDER_VOLT_ERROR | BBU_STATUS_OVER_VOLT_ERROR);
    }

    //battery temp come back to normal
    flag = (BBU_STATUS_UNDER_TEMP_ERROR | BBU_STATUS_OVER_TEMP_ERROR);
    if ((hba_ptr->bbu.prev_status & flag)
        && (!(hba_ptr->bbu.status & flag))) {
        FM_PRINT("BBU temp back to normal\n");
        FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
        FM_PRINT("Temperature = %d.%d Celsius\n", hba_ptr->bbu.temperature/100, MV_ABS(hba_ptr->bbu.temperature%100));
        FM_PRINT("hba_ptr->bbu.prev_status 0x%X\n", hba_ptr->bbu.prev_status);
        FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);
        core_generate_event(core, EVT_ID_BAT_TEMP_IS_NORMAL, 0, SEVERITY_INFO,  0,  NULL, 0 );
        temp_normal = MV_TRUE;
    }

    //battery voltage come back to normal
    flag = (BBU_STATUS_UNDER_VOLT_ERROR |BBU_STATUS_OVER_VOLT_ERROR);
    if ((hba_ptr->bbu.prev_status & flag)
        && (!(hba_ptr->bbu.status & flag))) {
        FM_PRINT("BBU volt back to normal\n");
        FM_PRINT("Flag = 0x%X\n", hba_ptr->bbu.flags);
        FM_PRINT("Percentage = %d%%\n", hba_ptr->bbu.percentage);
        FM_PRINT("Voltage = %d mV\n", hba_ptr->bbu.voltage);
        FM_PRINT("hba_ptr->bbu.prev_status 0x%X\n", hba_ptr->bbu.prev_status);
        FM_PRINT("hba_ptr->bbu.status 0x%X\n", hba_ptr->bbu.status);
        core_generate_event(core, EVT_ID_BAT_VOLTAGE_NORMAL, 0, SEVERITY_INFO,  0,  NULL, 0 );
        volt_normal = MV_TRUE;
    }

    if (ever_fail && cap_normal && temp_normal && volt_normal)
    {
#ifdef CACHE_MODULE_SUPPORT
        core_notify_battery((MV_PVOID)core, MV_TRUE);
        core_generate_event(core, EVT_ID_BAT_SAFE_TO_USE_WRITE_BACK, 0, SEVERITY_INFO,  0,  NULL, 0 );
#endif
        ever_fail = MV_FALSE;
    }
                                                    
    hba_ptr->bbu.prev_status = hba_ptr->bbu.status;
    first_get_bbu_info = MV_FALSE;
    
}

#if !defined(SUPPORT_TWSI)
MV_VOID bbu_timer_addrequest(core_extension * core)
{
    MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;

    core->lib_bbu.i2c_state = I2C_STATE_IDLE;
    
    if (g_twsi_detect_cnt < TWSI_MAX_DETECT_COUNT) {
        // 1 unit means 500ms
        Timer_AddRequest(core, 60, bbu_state_machine, core, NULL);
    } else {
        FM_PRINT("JL g_twsi_detect_cnt %d\n", g_twsi_detect_cnt);
    }
}

MV_VOID bbu_error_handling(core_extension * core)
{
    MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
    MV_BOOLEAN lasttime_is_good = MV_FALSE;

    if (hba_ptr->bbu.status & BBU_STATUS_PRESENT)
        lasttime_is_good = MV_TRUE;

    hba_ptr->bbu.status = BBU_STATUS_NOT_PRESENT;
    core->lib_bbu.bbu_state = BBU_STATE_ERROR;

    if (g_twsi_detect_cnt >= TWSI_MAX_DETECT_COUNT)
        FM_PRINT("JL %s BBU_STATUS_NOT_PRESENT\n", __FUNCTION__);

    if (lasttime_is_good) {
    #ifdef CACHE_MODULE_SUPPORT
        core_notify_battery((MV_PVOID)core, MV_FALSE);
    #endif
        core_generate_event(core, EVT_ID_BAT_NOT_PRESENT, 0, SEVERITY_WARNING,  0,  NULL, 0);
    }
    
    bbu_state_machine(core);
}
#endif

void bbu_init_structure(PHBA_Extension hba_ptr)
{
    HBA_Info_Page hba_info_param;
    MV_BOOLEAN wrong_threshold = MV_FALSE;
    core_extension * core=(core_extension *)HBA_GetModuleExtension(hba_ptr, MODULE_CORE);

#if !defined(SUPPORT_TWSI)
    MV_ZeroMemory(&data, 0x80);
#if 0
    MV_ZeroMemory(&prev_data, 0x80);
    MV_ZeroMemory(&cur_data, 0x80);
#endif
#endif
 #ifdef  I2C_ONE_BYTE_READ
    g_index = 0;
 #endif
    core->lib_bbu.bbu_addr.offset = 0;

    mv_nvram_init_param(hba_ptr, &hba_info_param);

    hba_ptr->bbu.featureSupport = BBU_SUPPORT_SENSOR_TEMPERATURE | BBU_SUPPORT_SENSOR_VOLTAGE;
    hba_ptr->bbu.status = BBU_STATUS_NOT_PRESENT;
    hba_ptr->bbu.prev_status = hba_ptr->bbu.status;
    hba_ptr->bbu.percent_to_charge = hba_info_param.bbu_charge_threshold;
    hba_ptr->bbu.temp_lowerbound = hba_info_param.bbu_temp_lowerbound;
    hba_ptr->bbu.temp_upperbound = hba_info_param.bbu_temp_upperbound;
    hba_ptr->bbu.volt_lowerbound = hba_info_param.bbu_volt_lowerbound;
    hba_ptr->bbu.volt_upperbound = hba_info_param.bbu_volt_upperbound;

   
    //if threshold over range, reset to default.
    if ((hba_ptr->bbu.percent_to_charge == BBU_CAPACITY_EMPTY)
        ||(hba_ptr->bbu.percent_to_charge >= BBU_CAPACITY_FULL)) {
        hba_ptr->bbu.percent_to_charge = BBU_CAPACITY_CHARGE_BOUND;
        hba_info_param.bbu_charge_threshold = BBU_CAPACITY_CHARGE_BOUND;
        wrong_threshold = MV_TRUE;
    }

    if ((hba_ptr->bbu.temp_lowerbound < BBU_TEMP_LOWER_BOUND)
        || (hba_ptr->bbu.temp_lowerbound >= BBU_TEMP_UPPER_BOUND)) {
        MV_DPRINT(("temp lo %d, reset it\n", hba_ptr->bbu.temp_lowerbound));
        hba_info_param.bbu_temp_lowerbound = BBU_TEMP_LOWER_BOUND;
        hba_ptr->bbu.temp_lowerbound = BBU_TEMP_LOWER_BOUND;
        wrong_threshold = MV_TRUE;
    }
    
    if ((hba_ptr->bbu.temp_upperbound > BBU_TEMP_UPPER_BOUND)
        || (hba_ptr->bbu.temp_upperbound <= BBU_TEMP_LOWER_BOUND)) {
        MV_DPRINT(("temp up %d, reset it\n", hba_ptr->bbu.temp_upperbound));
        hba_info_param.bbu_temp_upperbound = BBU_TEMP_UPPER_BOUND;
        hba_ptr->bbu.temp_upperbound = BBU_TEMP_UPPER_BOUND;
        wrong_threshold = MV_TRUE;
    }

    if ((hba_ptr->bbu.volt_lowerbound < BBU_VOLT_LOWER_BOUND)
        || (hba_ptr->bbu.volt_lowerbound >= BBU_VOLT_UPPER_BOUND)) {
        MV_DPRINT(("volt lo %d, reset it\n", hba_ptr->bbu.volt_lowerbound));
        hba_info_param.bbu_volt_lowerbound = BBU_VOLT_LOWER_BOUND;
        hba_ptr->bbu.volt_lowerbound = BBU_VOLT_LOWER_BOUND;
        wrong_threshold = MV_TRUE;
    }
    
    if ((hba_ptr->bbu.volt_upperbound > BBU_VOLT_UPPER_BOUND)
        || (hba_ptr->bbu.volt_upperbound <= BBU_VOLT_LOWER_BOUND)) {
        MV_DPRINT(("volt up %d, reset it\n", hba_ptr->bbu.volt_upperbound));
        hba_info_param.bbu_volt_upperbound = BBU_VOLT_UPPER_BOUND;
        hba_ptr->bbu.volt_upperbound = BBU_VOLT_UPPER_BOUND;
        wrong_threshold = MV_TRUE;
    }

    MV_DPRINT(("JL, current threshold capacity %d, volt %d-%d, temp %d-%d\n", 
        hba_ptr->bbu.percent_to_charge, hba_ptr->bbu.volt_lowerbound, hba_ptr->bbu.volt_upperbound,
        hba_ptr->bbu.temp_lowerbound, hba_ptr->bbu.temp_upperbound));
    
    if (wrong_threshold) {
        mvuiHBA_modify_param(hba_ptr, &hba_info_param);
    }
}

#if defined(SUPPORT_TWSI)
#include "mm.h"
void bbu_timer_get_info(MV_PVOID module_pointer, MV_PVOID temp);
void bbu_timer_get_info_callback(MV_PVOID module_pointer, PMV_Request pmvreq)
{
	core_extension * core = (core_extension *)module_pointer;
       MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
       PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
	twsi_device_setting_t *device = (twsi_device_setting_t *)pmvreq->Context[0];
	twsi_device_info_t *devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
	unsigned char *data_buf = (unsigned char *)pmvreq->Data_Buffer;
	MV_U8 add_timer_sequence = 0x01;

	if (pmvreq->Scsi_Status == REQ_STATUS_SUCCESS) {
		if (!(hba_ptr->bbu.status & BBU_STATUS_PRESENT)) {
#ifdef CACHE_MODULE_SUPPORT
			core_notify_battery((MV_PVOID)core, MV_TRUE);
#endif
			core_generate_event(core, EVT_ID_BAT_PRESENT, 0, SEVERITY_INFO,  0,  NULL, 0);
		}
		hba_ptr->bbu.status |= BBU_STATUS_PRESENT;
		if (first_get_bbu_info)
			bbu_enable(core);
		i2c_bbu_check_status(core, data_buf);
		g_twsi_detect_cnt = 0;
	} else {
		if (hba_ptr->bbu.status & BBU_STATUS_PRESENT) {
#ifdef CACHE_MODULE_SUPPORT
			core_notify_battery((MV_PVOID)core, MV_FALSE);
#endif
			core_generate_event(core, EVT_ID_BAT_NOT_PRESENT, 0, SEVERITY_WARNING,  0,  NULL, 0);
		}
		hba_ptr->bbu.status = BBU_STATUS_NOT_PRESENT;
		core->lib_bbu.bbu_state = BBU_STATE_ERROR;
		FM_PRINT("JL %s Scsi_Status=0x%x\n", __FUNCTION__, pmvreq->Scsi_Status);
		if (g_twsi_detect_cnt >= TWSI_MAX_DETECT_COUNT) {
			FM_PRINT("JL %s BBU_STATUS_NOT_PRESENT\n", __FUNCTION__);
			add_timer_sequence = 0;
		}
		 g_twsi_detect_cnt ++;
	}

#if 0
	FREE_MEM(devices_info_list);
	FREE_MEM(device);
#endif
	FREE_MEM(pmvreq->Data_Buffer);
	FREE_MEM(pmvreq);
	if (add_timer_sequence) {
		Timer_AddRequest(hba_ptr, 60, bbu_timer_get_info, core, NULL);
	}
}

void twsi_send_request(MV_PVOID module_pointer, PMV_Request pmvreq);
twsi_device_setting_t * twsi_device_map(core_extension * core, MV_U8 port_id, MV_U16 slave_id);

void bbu_timer_get_info(MV_PVOID module_pointer, MV_PVOID temp)
{
	core_extension *core = (core_extension *)module_pointer;
	MV_PVOID upper_layer = HBA_GetModuleExtension(core, MODULE_HBA);
	PHBA_Extension hba_ptr = (PHBA_Extension)upper_layer;
	if (g_twsi_detect_cnt >= TWSI_MAX_DETECT_COUNT) {
		FM_PRINT("JL g_twsi_detect_cnt %d\n", g_twsi_detect_cnt);
		return;
	}
	if (core->lib_twsi.major_state != TWSI_DEVICES_STATEMACHINE_STARTED) {
                 FM_PRINT("Waiting TWSI module ready(BBU).\n");
		   Timer_AddRequest(hba_ptr, 5, bbu_timer_get_info, core, NULL);
		   return;
	}
	{
		MV_Request *point_new_req = NULL;
		twsi_device_setting_t *device = NULL;
		twsi_device_info_t *devices_info_list = NULL;
		char *data_buf = NULL;
		unsigned long address = 0x00000000L;


		if ((device = twsi_device_map(core, 1, 0xAA)) != NULL) {
			devices_info_list = (twsi_device_info_t *)device->ptr_device_info;
//			FM_PRINT("JL Found out twsi device.(port%d SlaveId=0x%x %s)\n", devices_info_list->port_id, devices_info_list->slave_target, devices_info_list->name);
		} else {
#if 1
			hba_ptr->bbu.status = BBU_STATUS_NOT_PRESENT;
			core->lib_bbu.bbu_state = BBU_STATE_ERROR;
			FM_PRINT("JL %s BBU_STATUS_NOT_PRESENT\n", __FUNCTION__);
			return;
#else
			*device = (twsi_device_setting_t *)ALLOCATE_MEM(sizeof(twsi_device_setting_t));
			*devices_info_list = (twsi_device_info_t *)ALLOCATE_MEM(sizeof(twsi_device_info_t));
			device->ptr_device_info = devices_info_list;
			device->id = 0x5A;

			devices_info_list->port_id = TWSI_BBU_PORT; /*Port 1*/
			devices_info_list->feature = (/*TWSI_FEATURE_POLLING_MODE |*/ TWSI_FEATURE_ADDR7_BIT);
			devices_info_list->slave_target = 0xAA;
			devices_info_list->link_rate = 100; /*100K Hz*/
			devices_info_list->dev_type = TWSI_NORMAL_DEVICE;
			devices_info_list->name = "BBU Controller";
#endif
		}
		point_new_req = (MV_Request *)ALLOCATE_MEM(sizeof(MV_Request));
		data_buf = (char *)ALLOCATE_MEM(256);
		if ((point_new_req == NULL) || (data_buf == NULL)) {
			FM_PRINT("No enough resource(BBU module).\n");
			if (point_new_req != NULL)
				FREE_MEM(point_new_req);
			if (data_buf != NULL)
				FREE_MEM(data_buf);

			FM_PRINT("Try again!(BBU module)\n");
			Timer_AddRequest(hba_ptr, 5, bbu_timer_get_info, core, NULL);
			return;
		}
		
		MV_ZeroMemory(point_new_req, sizeof(MV_Request));

		point_new_req->Context[0] = (MV_PVOID)device;
		point_new_req->Completion = bbu_timer_get_info_callback;
		point_new_req->Cdb[0] = APICDB0_ADAPTER;
		point_new_req->Cdb[1] = 0x5A;

		point_new_req->Cdb[4] = (unsigned char)(address);
		point_new_req->Cdb[5] = (unsigned char)(address >> 8);
		point_new_req->Cdb[6] = (unsigned char)(address >> 16);
		point_new_req->Cdb[7] = (unsigned char)(address >> 24);

		point_new_req->Data_Buffer = data_buf;
		point_new_req->Data_Transfer_Length = 0x30; /*48 Bytes*/
		point_new_req->Cmd_Flag |= CMD_FLAG_DATA_IN;
		point_new_req->Cmd_Initiator = core;

		twsi_send_request(core, point_new_req);
	}
}

void bbu_initialize(MV_PVOID core_p)
{
    core_extension *core = (core_extension *)core_p;
    MV_PVOID upper_layer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)upper_layer;
    MV_U8 port = TWSI_BBU_PORT;

    bbu_init_structure(hba_ptr);

    Timer_AddRequest(hba_ptr, 10, bbu_timer_get_info, core, NULL);
}
#endif /*defined(SUPPORT_TWSI)*/

#if !defined(SUPPORT_TWSI)

static MV_BOOLEAN bbu_i2c_state_machine(
	core_extension * core,
	MV_U8 port
	)
{
    MV_U8 offBlock[2];
    MV_U32 offSize;
    MV_U8 I2cCtlReg,I2cStatusReg;
    MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
    MV_U8 i;
    
    //FM_PRINT("JL %s %d %s ... core->lib_bbu.i2c_state = 0x%X, core->lib_bbu.bbu_state = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, core->lib_bbu.i2c_state, core->lib_bbu.bbu_state);
    switch (core->lib_bbu.i2c_state)
    {
        case I2C_STATE_START:
            mvTwsisStartBitSet(core, port);
            break;

        case I2C_STATE_STOP:
            twsiStopStep(core, port, MV_TWSI_READ);
            //mvTwsisStopBitSet(core, port);

            I2cCtlReg = read_i2c_register(core, port, MI2C_CONTROL);
            I2cStatusReg = read_i2c_register(core, port, MI2C_STATUS);

            if ((I2cCtlReg & TWSI_CONTROL_STOP_BIT) != 0) {
                FM_PRINT("JL %s Stop/Repeated Start ERROR - status %x\n", __FUNCTION__, \
                    I2cStatusReg);
                bbu_error_handling(core);
                return MV_FALSE;
            }

            switch (core->lib_bbu.bbu_state)
            {
                case BBU_STATE_INIT:
                    /* BBU exist, initialize structure */
                    FM_PRINT("JL Found BBU, init BBU\n");
                    bbu_enable(core);
                    bbu_init_structure(hba_ptr);
//                    bbu_disable_charger(core);

                    core->lib_bbu.bbu_state = BBU_STATE_GET_STATUS;
                    core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                    bbu_timer_addrequest(core);
                    return MV_TRUE;
                    
                case BBU_STATE_GET_STATUS:

                    //twsiAckBitSet(core, port);

                    #ifdef  I2C_ONE_BYTE_READ
                        if (g_index == 0x2D) {
                            core->lib_bbu.bbu_state = BBU_STATE_CHECK_STATUS;
                            core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                            bbu_state_machine(core);
                            g_index = 0;
                            //bbu_timer_addrequest(core);
                        } else {
                            core->lib_bbu.bbu_addr.offset = g_index;
                            core->lib_bbu.bbu_state = BBU_STATE_GET_STATUS;
                            core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                            bbu_state_machine(core);
                        }
                    #else
                        core->lib_bbu.bbu_state = BBU_STATE_CHECK_STATUS;
                        core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                        bbu_state_machine(core);
                        #if 0
                        if (!MV_CompareMemory(&prev_data, &cur_data, 0x80)) {
                            FM_PRINT("JL compare OK, get data count %d\n", get_count);
                            get_count = 0;
                            MV_CopyMemory(&data, &cur_data, 0x80);
                            MV_CopyMemory(&prev_data, &cur_data, 0x80);
                            core->lib_bbu.bbu_state = BBU_STATE_CHECK_STATUS;
                            core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                            bbu_state_machine(core);
                        } else {
                            get_count++;
                            FM_PRINT("JL get data count %d\n", get_count);
                            MV_CopyMemory(&prev_data, &cur_data, 0x80);
                            core->lib_bbu.bbu_state = BBU_STATE_GET_STATUS;
                            core->lib_bbu.i2c_state = I2C_STATE_IDLE;
                            bbu_timer_addrequest(core);
                        }
                        #endif
                    #endif
                    return MV_TRUE;
                    
            }

            break;

        case I2C_STATE_ADDR_SET_WT:
            mvTwsisAddrSet(core, port, &(core->lib_bbu.bbu_addr.slave_addr), MV_TWSI_WRITE);
            break;
            
        case I2C_STATE_ADDR_SET_RD:
            mvTwsisAddrSet(core, port, &(core->lib_bbu.bbu_addr.slave_addr), MV_TWSI_READ);
            break;

        case I2C_STATE_DATA_TRANSMIT:
            if (core->lib_bbu.bbu_addr.more_than_256 == MV_TRUE) {
                offBlock[0] = (core->lib_bbu.bbu_addr.offset >> 8) & 0xff;
                offBlock[1] = core->lib_bbu.bbu_addr.offset & 0xff;
                offSize = 2;
            } else {
                offBlock[0] = core->lib_bbu.bbu_addr.offset & 0xff;
                offSize = 1;
            }
            core->lib_bbu.bbu_addr.xfer_count = 0;
            core->lib_bbu.bbu_addr.xfer_length = offSize;
            twsiDataTransmit(core, port, &offBlock[core->lib_bbu.bbu_addr.xfer_count]);
            break;
            
        case I2C_STATE_REPEATED_START:
            mvTwsisStartBitSet(core, port);
            core->lib_bbu.bbu_addr.xfer_count = 0;
            #ifdef I2C_ONE_BYTE_READ
                core->lib_bbu.bbu_addr.xfer_length = 1;
            #else
                core->lib_bbu.bbu_addr.xfer_length = 0x2D;
            #endif
            break;
            
        case I2C_STATE_DATA_RECEIVE:
            #ifdef I2C_ONE_BYTE_READ
                twsiDataReceive(core, port, &data[g_index++]);
            #else
                twsiDataReceive(core, port, &data[core->lib_bbu.bbu_addr.xfer_count]);
            #endif
            break;

        case I2C_STATE_RESET:
           /* Reset the TWSI logic */
           write_i2c_register(core, port, MI2C_SOFT_RESET, 0);
           
           /* wait for 2 mili sec */
           HBA_SleepMillisecond(core, 2);

            core->lib_bbu.bbu_state = BBU_STATE_INIT;
            bbu_timer_addrequest(core);
            break;

        case I2C_STATE_CHECK:
            i2c_bbu_check_status(core, &data);
            core->lib_bbu.bbu_state = BBU_STATE_GET_STATUS;
            bbu_timer_addrequest(core);
            break;
            
        default:
            break;
    }

    return MV_TRUE;
}

MV_BOOLEAN bbu_state_machine(
	MV_PVOID core_p)
{
    core_extension *core = (core_extension *)core_p;
    MV_U32 i2cFreq = I2C_BBU_LINK_RATE;
    MV_U8 port = TWSI_BBU_PORT;


    //FM_PRINT("JL %s %d %s ... Core %p, BBU_State = 0x%X\n", __FILE__, __LINE__, __FUNCTION__, core, core->lib_bbu.bbu_state);

    /* Read all registers */
    core->lib_bbu.bbu_addr.slave_addr.address = (BBU_SLAVE_ADDR >> 1);
    core->lib_bbu.bbu_addr.slave_addr.type = ADDR7_BIT;
    core->lib_bbu.bbu_addr.valid_offset = MV_TRUE;
//    core->lib_bbu.bbu_addr.offset = 0;
    core->lib_bbu.bbu_addr.more_than_256 = MV_FALSE;
    core->lib_bbu.bbu_addr.xfer_count = 0;
    core->lib_bbu.i2c_state = I2C_STATE_IDLE;

    switch (core->lib_bbu.bbu_state)
    {
        case BBU_STATE_INIT:
            g_twsi_detect_cnt++;
            //init bbu twsi port
            i2c_port_init(core, port, i2cFreq, 0); /* set the i2c frequency */

            core->lib_bbu.i2c_state = I2C_STATE_START;
            bbu_i2c_state_machine(core, port);
            break;

    #if 1
        case BBU_STATE_GET_STATUS:
            i2c_port_init(core, port, i2cFreq, 0); /* set the i2c frequency */
            core->lib_bbu.i2c_state = I2C_STATE_START;
            bbu_i2c_state_machine(core, port);
            break;

        case BBU_STATE_CHECK_STATUS:
            core->lib_bbu.i2c_state= I2C_STATE_CHECK;
            bbu_i2c_state_machine(core, port);
            break;
    #else
        case BBU_STATE_GET_STATUS:
            bbu_polling_read(core, port, 0, 0x2D, &data[0]);
        case BBU_STATE_CHECK_STATUS:
            core->lib_bbu.i2c_state= I2C_STATE_CHECK;
            bbu_i2c_state_machine(core, port);
            break;
    #endif
        case BBU_STATE_ERROR:
            core->lib_bbu.i2c_state = I2C_STATE_RESET;
            bbu_i2c_state_machine(core, port);
            break;

        default:
            core->lib_bbu.bbu_state = BBU_STATE_INIT;
            break;
    }

}


MV_BOOLEAN 
i2c_bbu_isr(MV_PVOID core_p)
{
    core_extension * core=(core_extension *)core_p;
    MV_U8 I2cCtlReg,I2cStatusReg;
    MV_U8 port = TWSI_BBU_PORT;

    I2cCtlReg = read_i2c_register(core, port, MI2C_CONTROL);

    if (core->lib_bbu.i2c_state == I2C_STATE_IDLE)
        return MV_FALSE;
    
    I2cStatusReg = read_i2c_register(core, port, MI2C_STATUS);
    //FM_PRINT("JL %s %d %s ... i2c_state 0x%X, I2cCtlReg 0x%X, I2cStatusReg 0x%X\n", __FILE__, __LINE__, __FUNCTION__, core->lib_bbu.i2c_state, I2cCtlReg, I2cStatusReg);

    if ((!(I2cCtlReg & TWSI_CONTROL_INT_ENA)) || (!(I2cCtlReg &TWSI_CONTROL_INT_FLAG_SET))) {
        //FM_PRINT("JL %s %d %s ...i2c_state 0x%X, bbu_state 0x%X, I2cStatusReg 0x%X, I2cCtlReg->IEN 0x%X,  I2cCtlReg->IFLG 0x%X\n", __FILE__, __LINE__, __FUNCTION__, core->lib_bbu.i2c_state, core->lib_bbu.bbu_state, I2cStatusReg, ((reg_mi2c_control *)&I2cCtlReg)->ien, ((reg_mi2c_control *)&I2cCtlReg)->iflg);
        return MV_FALSE;
    }

    switch(core->lib_bbu.i2c_state)
    {
        case I2C_STATE_START:
            if ((I2cStatusReg != TWSI_START_CON_TRA)
                || ((I2cCtlReg & TWSI_CONTROL_START_BIT) != 0)) {
                FM_PRINT("JL %s mvTwsiStartBitSet ERROR - start bit didn't went down\n", __FUNCTION__);
                bbu_error_handling(core);
                return MV_FALSE;
            }
            core->lib_bbu.i2c_state = I2C_STATE_ADDR_SET_WT;
            break;
            
        case I2C_STATE_ADDR_SET_WT:
            if (I2cStatusReg != TWSI_AD_PLS_WR_BIT_TRA_ACK_REC) {
                bbu_error_handling(core);
                return MV_FALSE;
            }

            if (core->lib_bbu.bbu_state == BBU_STATE_INIT)
                core->lib_bbu.i2c_state = I2C_STATE_STOP;
            else if (core->lib_bbu.bbu_state == BBU_STATE_GET_STATUS)
                core->lib_bbu.i2c_state = I2C_STATE_DATA_TRANSMIT;
            break;

        case I2C_STATE_DATA_TRANSMIT:
            if (I2cStatusReg != TWSI_M_TRAN_DATA_BYTE_ACK_REC) {
                FM_PRINT("JL %s twsiDataTransmit ERROR - status %x in write trans\n", __FUNCTION__, I2cStatusReg);
                bbu_error_handling(core);
                return MV_FALSE;
            }

            if (core->lib_bbu.bbu_addr.xfer_count == core->lib_bbu.bbu_addr.xfer_length)
                core->lib_bbu.i2c_state = I2C_STATE_REPEATED_START;
            break;

        case I2C_STATE_REPEATED_START:
            if ((I2cStatusReg != TWSI_REPEATED_START_CON_TRA)
                || ((I2cCtlReg & TWSI_CONTROL_START_BIT) != 0)) {
                FM_PRINT("JL %s mvTwsiStartBitSet ERROR - start bit didn't went down\n", __FUNCTION__);
                bbu_error_handling(core);
                return MV_FALSE;
            }
            core->lib_bbu.i2c_state = I2C_STATE_ADDR_SET_RD;
            break;
            
        case I2C_STATE_ADDR_SET_RD:
            if (I2cStatusReg != TWSI_AD_PLS_RD_BIT_TRA_ACK_REC) {
                FM_PRINT("JL %s twsiAddr7BitSet ERROR - status %x addr (7 Bit) in Read mode.\n", __FUNCTION__, \
                    I2cStatusReg);
                bbu_error_handling(core);
                return MV_FALSE;
            }
            core->lib_bbu.i2c_state = I2C_STATE_DATA_RECEIVE;
            break;

        case I2C_STATE_DATA_RECEIVE:
            #ifdef I2C_ONE_BYTE_READ
                if ((core->lib_bbu.bbu_addr.xfer_length != core->lib_bbu.bbu_addr.xfer_count)
                    && (I2cStatusReg != TWSI_M_REC_RD_DATA_ACK_TRA)) {
                    FM_PRINT("JL %s twsiDataReceive ERROR - status %x in read trans\n", __FUNCTION__, I2cStatusReg);
                    bbu_error_handling(core);
                    return MV_FALSE;
                }
                else if ((core->lib_bbu.bbu_addr.xfer_length == core->lib_bbu.bbu_addr.xfer_count)
                    && (I2cStatusReg != TWSI_M_REC_RD_DATA_ACK_NOT_TRA)) {
                    FM_PRINT("JL %s twsiDataReceive ERROR - status %x in Rd Terminate\n", __FUNCTION__, I2cStatusReg);
                    bbu_error_handling(core);
                    return MV_FALSE;
                }
            #else
                if (I2cStatusReg != TWSI_M_REC_RD_DATA_ACK_TRA) {
                    FM_PRINT("JL %s twsiDataReceive ERROR - status %x in read trans\n", __FUNCTION__, I2cStatusReg);
                    bbu_error_handling(core);
                    return MV_FALSE;
                }
            #endif

            /*Received Completely */
            if (core->lib_bbu.bbu_addr.xfer_count == core->lib_bbu.bbu_addr.xfer_length)
                core->lib_bbu.i2c_state = I2C_STATE_STOP;
            break;

        case I2C_STATE_RESET:
            break;

        default:
            break;
    }

    bbu_i2c_state_machine(core, port);
    return MV_TRUE;          
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
MV_U32 mvTwsisInit(core_extension * core, MV_U8 port, MV_HZ frequency, MV_U32 Tclk, twsi_addr *pTwsiAddr, MV_BOOLEAN generalCallEnable)
{
    MV_U32	n,m,freq,margin,minMargin = 0xffffffff;
    MV_U32	power;
    MV_U32	actualFreq = 0,actualN = 0,actualM = 0,val;

    if (frequency > 100000) {
        FM_PRINT("JL %s Warning TWSI frequancy is too high, please use up tp 100Khz.\n", __FUNCTION__);
    }


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
    /* Reset the TWSI logic */
    write_i2c_register(core, port, MI2C_SOFT_RESET, 0);
    
    /* wait for 2 mili sec */
    HBA_SleepMillisecond(core, 2);

    /* Set the baud rate */
    val = ((actualM<< TWSI_BAUD_RATE_M_OFFS) | actualN << TWSI_BAUD_RATE_N_OFFS);
    write_i2c_register(core, port, MI2C_CLOCK_CONTROL, val);

    /* Enable the TWSI and slave */
    write_i2c_register(core, port, MI2C_CONTROL, TWSI_CONTROL_ENA | TWSI_CONTROL_ACK); 

    /* set the TWSI slave address */
    if (pTwsiAddr->type == ADDR10_BIT) {
        /* 10 Bit deviceAddress */
        /* writing the 2 most significant bits of the 10 bit address*/
        val = ((pTwsiAddr->address & TWSI_SLAVE_ADDR_10BIT_MASK) >> TWSI_SLAVE_ADDR_10BIT_OFFS );
        /* bits 7:3 must be 0x11110 */
        val |= TWSI_SLAVE_ADDR_10BIT_CONST;
        /* set GCE bit */
        if (generalCallEnable)
            val |= TWSI_SLAVE_ADDR_GCE_ENA;
        /* write slave address */
        write_i2c_register(core, port, MI2C_SLAVE_ADDRESS, val);

        /* writing the 8 least significant bits of the 10 bit address*/
        val = (pTwsiAddr->address << TWSI_EXTENDED_SLAVE_OFFS) & TWSI_EXTENDED_SLAVE_MASK;  
        write_i2c_register(core, port, MI2C_X_SLAVE_ADDRESS, val);
    } else {
        /*7 bit address*/
        /* set the 7 Bits address */
        val = 0;
        write_i2c_register(core, port, MI2C_X_SLAVE_ADDRESS, val);
        val = (pTwsiAddr->address << TWSI_SLAVE_ADDR_7BIT_OFFS) & TWSI_SLAVE_ADDR_7BIT_MASK;
        write_i2c_register(core, port, MI2C_SLAVE_ADDRESS, val);
    }

    /* unmask twsi int */
    val = read_i2c_register(core, port, MI2C_CONTROL);
    write_i2c_register(core, port, MI2C_CONTROL, val | TWSI_CONTROL_INT_ENA);

    return actualFreq;
} 

/* Assuming that there is only one master on the bus (us) */

void
i2c_port_init(core_extension * core, MV_U8 port, int speed, int slaveaddr)
{
    twsi_addr slave;
    slave.type = ADDR7_BIT;
    slave.address = slaveaddr;
    mvTwsisInit(core, port, speed, I2C_CFG_TCLK, &slave, 0);
}
#endif /*!SUPPORT_TWSI*/

void core_notify_battery(
	MV_PVOID	core_ext,
	MV_BOOLEAN is_normal)
{
    core_extension *core = (core_extension *)core_ext;
    MV_PVOID pUpperLayer = HBA_GetModuleExtension(core, MODULE_HBA);
    PHBA_Extension hba_ptr = (PHBA_Extension)pUpperLayer;
    struct mod_notif_param param;
    MV_VOID (*NotificationFunc)(MV_PVOID,
    				enum Module_Event,
    				struct mod_notif_param *);

    HBA_GetUpperModuleNotificationFunction(core_ext,
    	&pUpperLayer,
    	&NotificationFunc );

    param.lo = 0;

    NotificationFunc( 
    	pUpperLayer, 
    	is_normal ? EVENT_BBU_NORMAL : EVENT_BBU_ABNORMAL,
    	&param );
}

MV_BOOLEAN is_bbu_present(MV_PVOID this)
{
    PHBA_Extension hba_ptr = (PHBA_Extension)HBA_GetModuleExtension(this, MODULE_HBA);
    if (hba_ptr->bbu.status == BBU_STATUS_NOT_PRESENT)
        return MV_FALSE;
    else if (hba_ptr->bbu.status & BBU_STATUS_PRESENT)
        return MV_TRUE;
}


#endif /* SUPPORT_BBU */

