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
#include "core_manager.h"
#include "core_gpio.h"
#include "com_error.h"
#include "core_util.h"

#ifdef SUPPORT_SGPIO

#if defined(SUPPORT_ROC)
MV_U32 sgpio_read_pci_register(MV_PVOID core_p, MV_U32 reg_address) {
	core_extension *core = (core_extension *)core_p;
	MV_LPVOID mmio = core->mmio_base;
	return MV_REG_READ_DWORD(mmio, (0x30000 + reg_address));
}

void sgpio_write_pci_register(MV_PVOID core_p, MV_U32 reg_address, MV_U32 reg_value) {
	core_extension *core = (core_extension *)core_p;
	MV_LPVOID mmio = core->mmio_base;
	MV_REG_WRITE_DWORD(mmio, (0x30000 + reg_address), reg_value);
}
#else /*SUPPORT_ROC*/
MV_U32 sgpio_read_pci_register(MV_PVOID core_p, MV_U8 reg_address) {
	MV_U32 reg=0;
	reg = MV_PCI_READ_DWORD( core_p, reg_address );
	return reg;
}

void sgpio_write_pci_register(MV_PVOID core_p, MV_U8 reg_address, MV_U32 reg_value) {
	MV_PCI_WRITE_DWORD( core_p, reg_value, reg_address );
	return;
}
#endif /*SUPPORT_ROC*/



/*
* Known Hardware issues for the SGPIO on Odin:
* 1. The clock frequency by default is 1 KHz.  Changing clock frequency to 100KHz
*    (SGPIO spec uses 100KHz) could cause a phase shift that result in data being written on
*    the falling edge instead of rising edge.
*    Possible solution: Write to register of target multiple times until the desired data is
*    correctly written into the register.
* 2. The SGPIO pins stated on the spec could be wrong.  The following is the correct pin layout.
*/

#if !defined(SUPPORT_ROC)
void
sgpio_sendsgpioframe(MV_PVOID this, MV_U32 value)
{
	core_extension * pCore=(core_extension *)this;
	MV_U32 sgpio_ctl_reg;
	MV_U32 sgpio_data_out_l;
	MV_U32 sgpio_data_out_h;

	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );
	//CORE_DPRINT(("*  SGPIO_CtlReg before (start) = 0x%x \n", sgpio_ctl_reg));
	sgpio_data_out_l = sgpio_read_pci_register( pCore, SGPIO_Data_Out_L );
	sgpio_data_out_h = sgpio_read_pci_register( pCore, SGPIO_Data_Out_H );
	//CORE_DPRINT(("SGPIO_DataOut_L before write (start) = 0x%x \n", sgpio_data_out_l ));
	//CORE_DPRINT(("SGPIO_DataOut_H before write (start) = 0x%x \n", sgpio_data_out_h ));

	sgpio_write_pci_register( pCore, SGPIO_Data_Out_L, value);
	sgpio_write_pci_register( pCore, SGPIO_Data_Out_H, 0x0);
	sgpio_data_out_l = sgpio_read_pci_register( pCore, SGPIO_Data_Out_L );
	sgpio_data_out_h = sgpio_read_pci_register( pCore, SGPIO_Data_Out_H );
	//CORE_DPRINT(("SGPIO_DataOut_L after write (start) = 0x%x \n", sgpio_data_out_l ));
	//CORE_DPRINT(("SGPIO_DataOut_H after write (start) = 0x%x \n", sgpio_data_out_h ));

	// Setup the desired SGPIO_Control register value
	sgpio_ctl_reg = 0xe0;

	sgpio_ctl_reg |= SGPIO_START_BIT;
	sgpio_write_pci_register( pCore, SGPIO_Control, sgpio_ctl_reg);
	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );
	//CORE_DPRINT(("*  SGPIO_CtlReg what is in the register now (start) = 0x%x \n", sgpio_ctl_reg ));

	core_sleep_millisecond(pCore, 500);

	sgpio_ctl_reg &= ~SGPIO_START_BIT;
	sgpio_write_pci_register( pCore, SGPIO_Control, sgpio_ctl_reg);
	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );
	//CORE_DPRINT(("*  SGPIO_CtlReg what is in the register now (start) = 0x%x \n", sgpio_ctl_reg ));
}
#endif

/*void
sgpio_stopsgpioframe(MV_PVOID This, MV_U32 value)
{
        PCore_Driver_Extension pCore=(PCore_Driver_Extension)This;
        MV_U32 SGPIO_CtlReg;

        CORE_PRINT((" \n"));
        SGPIO_CtlReg = sgpio_read_pci_register( pCore, SGPIO_Control );
        CORE_PRINT(("*  SGPIO_CtlReg first (stop) = 0x%x \n", SGPIO_CtlReg ));

        sgpio_write_pci_register( pCore, SGPIO_Data_Out_L, value);
        sgpio_write_pci_register( pCore, SGPIO_Data_Out_H, value);
        SGPIO_CtlReg &= ~SGPIO_START_BIT;
        CORE_PRINT(("*  SGPIO_CtlReg middle (stop) = 0x%x \n", SGPIO_CtlReg ));
        sgpio_write_pci_register( pCore, SGPIO_Control, SGPIO_CtlReg);
        SGPIO_CtlReg = sgpio_read_pci_register( pCore, SGPIO_Control );
        CORE_PRINT(("*  SGPIO_CtlReg last (stop) = 0x%x \n", SGPIO_CtlReg ));
}*/

void sgpio_initialize( MV_PVOID this ) {
	core_extension * core=(core_extension *)this;
	pl_root *root=NULL;
	domain_port *port;
	domain_device * device;
	MV_U32 sgpio_init_reg=0;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp, reg_config0, reg_config1, reg_config2, reg_config3, dword_offset0, dword_offset1;
#ifdef SUPPORT_SGPIO_DATA_IN
	MV_U32 count,sgpio;
#endif //SUPPORT_SGPIO_DATA_IN
	MV_U8 i, device_cnt0, device_cnt1, j;
    MV_U8 led_value;
    
#ifndef SUPPORT_ROC
	if ((core->device_id == DEVICE_ID_6440) || (core->device_id == DEVICE_ID_6445)
                        ||(core->device_id == 0x6320) ||(core->device_id == DEVICE_ID_6340))
	{
		sgpio_init_reg = sgpio_read_pci_register( core, SGPIO_Init );
		CORE_PRINT(("*  SGPIO_InitReg before initialization = 0x%x \n", sgpio_init_reg ));
		sgpio_init_reg &= ~SGPIO_Mode;
		sgpio_write_pci_register(core, SGPIO_Init, sgpio_init_reg );
		core_sleep_microsecond(core, 1);
		sgpio_init_reg |= SGPIO_Mode;

		/*
		*The following lines setup the desired clock frequency in SGPIO_Control register
		* before clk is started by SGPIO_Init.  Allowing clk to switch frequency during run-time
		* could result in writing on the wrong edge of the clk.
		*/
		/*
		SGPIO_CtlReg |= SGPIO_SELECT_SPEED(SGPIO_100KHz);
		CORE_PRINT(("*  0 SGPIO_CtlReg going to be written into register (start) = 0x%x \n", SGPIO_CtlReg ));
		sgpio_write_pci_register( core, SGPIO_Control, SGPIO_CtlReg ); */

		sgpio_write_pci_register( core, SGPIO_Init, sgpio_init_reg );
		sgpio_init_reg = sgpio_read_pci_register( core, SGPIO_Init );
		CORE_PRINT(("*  SGPIO_InitReg after initialization = 0x%x \n", sgpio_init_reg ));
	}
#endif //SUPPORT_ROC
#ifdef SUPPORT_ROC
    if (core->device_id == DEVICE_ID_8180)
#else
	else if ((core->device_id == DEVICE_ID_6480)  || (core->device_id == DEVICE_ID_6485))
#endif //SUPPORT_ROC
	{

	/*####### disable first ######*/
	/* sgpio 0 registers */
	mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);
    CORE_PRINT(("*  SGPIO 0 REG_CONFIG0 before initialization = 0x%x \n", reg_config0 ));
	reg_config0 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
	mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);

	/* sgpio 1 registers */
	mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);
    CORE_PRINT(("*  SGPIO 1 REG_CONFIG0 before initialization = 0x%x \n", reg_config1 ));
	reg_config1 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
	mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);

	/*####### enable SGPIO mode in PCI config register ######*/
#if !defined(SUPPORT_ROC)
	sgpio_write_pci_register(core, 0x44, 0x80);
#endif //!defined(SUPPORT_ROC)
	/*###### setting Drive Source ######*/
	device_cnt0=0;
	dword_offset0=0;
	device_cnt1=0;
	dword_offset1=0;

	root = &core->roots[core->chip_info->start_host];
	for(j=0;j<core->chip_info->n_phy;j++){
		port = &root->ports[j];
		if(port->device_count==0)
		continue;
		LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device, base.queue_pointer){
			if(device==NULL)
			break;
			if(device->base.parent->type == BASE_TYPE_DOMAIN_PORT){
			device->connection |= DC_SGPIO;
			////////////////
			if (j < 4){
				/* sgpio 0 */
				mv_sgpio_read_register(mmio, 
					SGPIO_REG_ADDR(0,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
					tmp);
				tmp &=~(0xff<<(dword_offset0*8));

				mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(0,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
					tmp+(DRV_SRC_PHY(0,j)<<(dword_offset0*8)));

				device->sgpio_drive_number = j;
				device_cnt0++;
				dword_offset0++;
				dword_offset0%=4; //0..3 to locate next drive source byte
			}else{
				/* sgpio 1 */
				mv_sgpio_read_register(mmio, 
					SGPIO_REG_ADDR(1,SGPIO_REG_DRV_SRC_BASE + device_cnt1 - dword_offset1),
					tmp);
				tmp &=~(0xff<<(dword_offset1*8));

				mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(1,SGPIO_REG_DRV_SRC_BASE + device_cnt1 - dword_offset1),
					tmp+(DRV_SRC_PHY(0,j)<<(dword_offset1*8)));

				device->sgpio_drive_number = j;
				device_cnt1++;
				dword_offset1++;
				dword_offset1%=4; //0..3 to locate next drive source byte
			}	
			////////////////

			}
		}
	}
	

	/* do we need pre-set values for LEDs or just dark? Leave as dark for now */
	/*##### setting control mode #####*/
#ifdef SUPPORT_SGPIO_DATA_IN
	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

	/* sgpio 1 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
#else
	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));

	/* sgpio 1 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));
#endif //SUPPORT_SGPIO_DATA_IN
	/*##### setting blink speeds #####*/
	/* sgpio 0 */
	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
		tmp);
	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
	tmp |= 0x11;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
		tmp);

	/* sgpio 1 */
	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
		tmp);
	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
	tmp |= 0x11;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
		tmp);

	/*##### enable int #####*/

	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_INT_ENABLE),
	(MANUAL_MD_REP_DONE|SDIN_DONE));

	/* sgpio 1 */
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(1,SGPIO_REG_INT_ENABLE),
	(MANUAL_MD_REP_DONE|SDIN_DONE));

	/*##### re-enable SGPIO #####*/

	/* sgpio 0 */
	reg_config0 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
	/* SGPIO stream needs minimum 12 bits*/
	if (device_cnt0 < 4)
		device_cnt0 = 4;
	reg_config0 += (device_cnt0*3)<<AUTO_BIT_LEN_OFFSET;

	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		reg_config0);

	/* sgpio 1 */
	reg_config1 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
	/* SGPIO stream needs minimum 12 bits*/
	if (device_cnt1 < 4)
		device_cnt1 = 4;
	reg_config1 += (device_cnt1*3)<<AUTO_BIT_LEN_OFFSET;

	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0),
		reg_config1);

	mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG0), reg_config0);
    CORE_PRINT(("*  SGPIO 0 REG_CONFIG0 after initialization = 0x%x \n", reg_config0));
	mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG0), reg_config1);
    CORE_PRINT(("*  SGPIO 0 REG_CONFIG0 after initialization = 0x%x \n", reg_config1));
#ifdef SUPPORT_SGPIO_DATA_IN
	for(sgpio=0;sgpio<2;sgpio++){
		count=0;
		for(i=0;i<100;i++) {
			mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
			tmp);

			if(tmp&SDIN_DONE){
				CORE_PRINT(("SGPIO%d count:%d, interrupt:%d\n",sgpio,count,tmp));
				/*for 6480, SDIN one shot capture mode first capture data is invalid so we discard first value.*/
				if (count==0) {
					mv_sgpio_read_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
					tmp);
					mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
					tmp);
					mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
					((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
				} else {
					sgpio_isr(this,sgpio);
					break;
				}
				count++;
			} else
				core_sleep_millisecond(root->core, 10); //delay 5ms
				if (i == 10) {
					CORE_PRINT(("no sgpio data-in device connected\n"));
				break;
			}
		}
	}
#endif //SUPPORT_SGPIO_DATA_IN
	}

	if (IS_VANIR(core) || core->device_id == DEVICE_ID_948F || IS_ATHENA_CORE(core)) {
		/* sgpio 0 registers */
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG0), 0);

		/* sgpio 1 registers */
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG0), 0);

		/* sgpio 2 registers */
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG0), 0);

		/* sgpio 3 registers */
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG0), 0);

		/*####### enable SGPIO mode in PCI config register ######*/
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		tmp = MV_REG_READ_DWORD(mmio, 0x2c104) & ~0x00000300L;
		tmp |= 0x00000000L;
		MV_REG_WRITE_DWORD(mmio, 0x2c104, tmp);
#else
		tmp = MV_REG_READ_DWORD(mmio, 0x10104) & ~0x00000300L;
		tmp |= 0x00000100L;
		MV_REG_WRITE_DWORD(mmio, 0x10104, tmp);
#endif
		/*###### setting Drive Source ######*/
		device_cnt0=0;
		dword_offset0=0;
		device_cnt1=0;
		dword_offset1=0;

		for (i = core->chip_info->start_host;i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
			root = &core->roots[i];
			for(j = 0;j < MAX_PORT_PER_PL; j++) {
				port = &root->ports[j];
				if (port->device_count==0)
					continue;
				LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device, base.queue_pointer) {
					if (device==NULL)
						break;
					if (device->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
                        if (core->enable_sgpio)
    						device->connection |= DC_SGPIO;
                        
						mv_sgpio_read_register(mmio,
						        SGPIO_REG_ADDR(i,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						        tmp);
						tmp &=~(0xff<<(dword_offset0*8));
						mv_sgpio_write_register(mmio,
							SGPIO_REG_ADDR(i,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						    tmp+(DRV_SRC_PHY(0,i)<<(dword_offset0*8)));

                        led_value= (LED_LOW<<DRV_ACTV_LED_OFFSET)|
                            (LED_LOW<<DRV_LOC_LED_OFFSET)|
                            (LED_LOW<<DRV_ERR_LED_OFFSET);
                        
                        mv_sgpio_read_register(mmio, 
                            SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE + device_cnt0 - dword_offset0),
                            tmp);
                        tmp &=~(0xff<<(dword_offset0*8));
                        mv_sgpio_write_register(mmio,
                            SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE + device_cnt0 - dword_offset0),
                            tmp+(led_value<<(dword_offset0*8)));
                        
						device->sgpio_drive_number = j;
                        //FM_PRINT("device->sgpio_drive_number %x\n", device->sgpio_drive_number);
                        
						device_cnt0++;
						dword_offset0++;
						dword_offset0 %= 4; //0..3 to locate next drive source byte

					}
				}
			}
		}

		mv_sgpio_write_register(mmio,SGPIO_REG_ADDR(0, SGPIO_REG_DRV_SRC_BASE),0x03020100);
		mv_sgpio_write_register(mmio,SGPIO_REG_ADDR(1, SGPIO_REG_DRV_SRC_BASE),0x03020100);
		mv_sgpio_write_register(mmio,SGPIO_REG_ADDR(2, SGPIO_REG_DRV_SRC_BASE),0x03020100);
		mv_sgpio_write_register(mmio,SGPIO_REG_ADDR(3, SGPIO_REG_DRV_SRC_BASE),0x03020100);

	/* do we need pre-set values for LEDs or just dark? Leave as dark for now */
	/*##### setting control mode #####*/
    #ifdef SUPPORT_SGPIO_DATA_IN
    	/* sgpio 0 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
    		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

    	/* sgpio 1 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
    		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));


    	/* sgpio 2 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(2,SGPIO_REG_CONTROL),
    		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

    	/* sgpio 3 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(3,SGPIO_REG_CONTROL),
    		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
    #else
    	/* sgpio 0 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
    		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));

    	/* sgpio 1 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
    		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));

	/* sgpio 2 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(2,SGPIO_REG_CONTROL),
    		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));

    	/* sgpio 3 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(3,SGPIO_REG_CONTROL),
    		(SDOUT_MD_AUTO<<SDOUT_MD_OFFSET));
    #endif //SUPPORT_SGPIO_DATA_IN
    	/*##### setting blink speeds #####*/
    	/* sgpio 0 */
    	mv_sgpio_read_register(mmio,
    		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
    		tmp);
    	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
    	tmp |= 0x11;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
    		tmp);

    	/* sgpio 1 */
    	mv_sgpio_read_register(mmio,
    		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
    		tmp);
    	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
    	tmp |= 0x11;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
    		tmp);

    	/* sgpio 2 */
    	mv_sgpio_read_register(mmio,
    		SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG1),
    		tmp);
    	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
    	tmp |= 0x11;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG1),
    		tmp);

    	/* sgpio 3 */
    	mv_sgpio_read_register(mmio,
    		SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG1),
    		tmp);
    	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
    	tmp |= 0x11;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG1),
    		tmp);
		
        //From Lenovo PB2800m project experience, Add SClock setting
		/* sgpio 0 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG2),
			tmp);
        //30KHz
        //tmp = 8000;
        //10KHz
        //tmp = 24000;
        //5KHz
        //tmp = 48000;
        // 2KHz
        //tmp = 120000;
        // 1KHz
        tmp = 240000;
        // 500Hz
        //tmp = 480000;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG2),
			tmp);
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG2),
			tmp);

		/* sgpio 1 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG2),
			tmp);
        //30KHz
        //tmp = 8000;
        //10KHz
        //tmp = 24000;
        //5KHz
        //tmp = 48000;
        // 2KHz
        //tmp = 120000;
        // 1KHz
        tmp = 240000;
        // 500Hz
        //tmp = 480000;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG2),
			tmp);
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG2),
			tmp);        

	/* sgpio 2 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG2),
			tmp);
        //30KHz
        //tmp = 8000;
        //10KHz
        //tmp = 24000;
        //5KHz
        //tmp = 48000;
        // 2KHz
        //tmp = 120000;
        // 1KHz
        tmp = 240000;
        // 500Hz
        //tmp = 480000;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG2),
			tmp);
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG2),
			tmp);

		/* sgpio 3 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG2),
			tmp);
        //30KHz
        //tmp = 8000;
        //10KHz
        //tmp = 24000;
        //5KHz
        //tmp = 48000;
        // 2KHz
        //tmp = 120000;
        // 1KHz
        tmp = 240000;
        // 500Hz
        //tmp = 480000;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG2),
			tmp);
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG2),
			tmp);        
    	/*##### enable int #####*/

    	/* sgpio 0 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(0,SGPIO_REG_INT_ENABLE),
    		(MANUAL_MD_REP_DONE|SDIN_DONE));

    	/* sgpio 1 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(1,SGPIO_REG_INT_ENABLE),
    		(MANUAL_MD_REP_DONE|SDIN_DONE));

    	/* sgpio 2 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(2,SGPIO_REG_INT_ENABLE),
    		(MANUAL_MD_REP_DONE|SDIN_DONE));

    	/* sgpio 3 */
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(3,SGPIO_REG_INT_ENABLE),
    		(MANUAL_MD_REP_DONE|SDIN_DONE));

    	/*##### re-enable SGPIO #####*/

    	/* sgpio 0 */
    	reg_config0 = (SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
    	/* SGPIO stream needs minimum 12 bits*/
    	if (device_cnt0 < 4)
    		device_cnt0 = 4;
    	reg_config0 += (device_cnt0*3)<<AUTO_BIT_LEN_OFFSET;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
    		reg_config0);

    	/* sgpio 1 */
    	reg_config1 = (SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
    	/* SGPIO stream needs minimum 12 bits*/
    	if (device_cnt1 < 4)
    		device_cnt1 = 4;
    	reg_config1 += (device_cnt1*3)<<AUTO_BIT_LEN_OFFSET;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0),
    		reg_config1);

    	/* sgpio 2 */
    	reg_config2 = (SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
    	/* SGPIO stream needs minimum 12 bits*/
    	if (device_cnt0 < 4)
    		device_cnt0 = 4;
    	reg_config2 += (device_cnt0*3)<<AUTO_BIT_LEN_OFFSET;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(2,SGPIO_REG_CONFIG0),
    		reg_config2);

    	/* sgpio 3 */
    	reg_config3 = (SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
    	/* SGPIO stream needs minimum 12 bits*/
    	if (device_cnt1 < 4)
    		device_cnt1 = 4;
    	reg_config3 += (device_cnt1*3)<<AUTO_BIT_LEN_OFFSET;
    	mv_sgpio_write_register(mmio,
    		SGPIO_REG_ADDR(3,SGPIO_REG_CONFIG0),
    		reg_config3);

		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG0), reg_config0);
        CORE_PRINT(("*  SGPIO 0 REG_CONFIG0 after initialization = 0x%x \n", reg_config0));
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG0), reg_config1);
        CORE_PRINT(("*  SGPIO 1 REG_CONFIG0 after initialization = 0x%x \n", reg_config1));
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(2, SGPIO_REG_CONFIG0), reg_config2);
        CORE_PRINT(("*  SGPIO 2 REG_CONFIG0 after initialization = 0x%x \n", reg_config0));
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(3, SGPIO_REG_CONFIG0), reg_config3);
        CORE_PRINT(("*  SGPIO 3 REG_CONFIG0 after initialization = 0x%x \n", reg_config1));
		
    #ifdef SUPPORT_SGPIO_DATA_IN
    	for (sgpio = 0; sgpio < 4; sgpio++) {
            count = 0;
            for (i = 0; i < 100; i++) {
                mv_sgpio_read_register(mmio,
                SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
                tmp);

                if (tmp & SDIN_DONE) {
                    CORE_PRINT(("SGPIO%d count:%d, interrupt:%d\n", sgpio, count, tmp));
        			if (count == 0) {
                        mv_sgpio_read_register(mmio,
                        SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
                        tmp);
                        mv_sgpio_write_register(mmio,
                        SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
                        tmp);
                        mv_sgpio_write_register(mmio,
                        SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
                        ((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
                    } else {
                        sgpio_isr(this,sgpio);
                        break;
                    }
                    count++;
        		} else
                    core_sleep_millisecond(root->core, 10); //delay 5ms

                if (i == 10) {
                    CORE_PRINT(("no sgpio data-in device connected\n"));
                    break;
                }
            }
	    }
    #endif //SUPPORT_SGPIO_DATA_IN
    }
}

/*TBD: need test when I2C and SMP command ready*/
void sgpio_smpreq_callback(core_extension * core, PMV_Request req)
{
	smp_response *smp_resp;
	struct _domain_sgpio * sgpio_result = &core->lib_gpio.sgpio_result;
	smp_resp= (smp_response *)req->Scratch_Buffer;

	switch (smp_resp->function) {
	case READ_GPIO_REGISTER:
		CORE_PRINT(("Read GPIO Register Response: \n"));
		CORE_PRINT(("Data In High 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		smp_resp->response.ReadGPIORegister.Data_In_High[0],
		smp_resp->response.ReadGPIORegister.Data_In_High[1],
		smp_resp->response.ReadGPIORegister.Data_In_High[2],
		smp_resp->response.ReadGPIORegister.Data_In_High[3],
		smp_resp->response.ReadGPIORegister.Data_In_High[4],
		smp_resp->response.ReadGPIORegister.Data_In_High[5],
		smp_resp->response.ReadGPIORegister.Data_In_High[6],
		smp_resp->response.ReadGPIORegister.Data_In_High[7]));
		CORE_PRINT(("Data In Low 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		smp_resp->response.ReadGPIORegister.Data_In_Low[0],
		smp_resp->response.ReadGPIORegister.Data_In_Low[1],
		smp_resp->response.ReadGPIORegister.Data_In_Low[2],
		smp_resp->response.ReadGPIORegister.Data_In_Low[3],
		smp_resp->response.ReadGPIORegister.Data_In_Low[4],
		smp_resp->response.ReadGPIORegister.Data_In_Low[5],
		smp_resp->response.ReadGPIORegister.Data_In_Low[6],
		smp_resp->response.ReadGPIORegister.Data_In_Low[7]));
		break;
	case WRITE_GPIO_REGISTER:
		CORE_PRINT(("Receive Write GPIO Register Response: \n"));
		break;
	}
	MV_CopyMemory(sgpio_result, req->Scratch_Buffer, sizeof(req->Scratch_Buffer) );
}

void sgpio_smprequest_read(pl_root * root, PMV_Request req)
{
	smp_request*smp_req;
	PMV_SG_Table sg_table;

	if( req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}

	sg_table = &req->SG_Table;
	/* Prepare identify ATA task */
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP;
	req->Device_Id = 0xFFFF; // Indicating SGPIO operation
	req->Data_Transfer_Length = sizeof(smp_request);
	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function=READ_GPIO_REGISTER;
	smp_req->smp_frame_type=SMP_REQUEST_FRAME;
	core_unmap_data_buffer(req);
	req->Completion = (void(*)(MV_PVOID,PMV_Request))sgpio_smpreq_callback;

	/* Make SG table */
	SGTable_Init(sg_table, 0);

	/* Send this internal request */
	Core_ModuleSendRequest(root->core, req);
}

void sgpio_smprequest_write(core_extension * core)
{
	PMV_Request req = get_intl_req_resource(&core->roots[core->chip_info->start_host],sizeof(smp_request));
	smp_request*smp_req;
	PMV_SG_Table sg_table;

	if(req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}
	sg_table = &req->SG_Table;

	/* Prepare identify ATA task */
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP;
	req->Device_Id = 0xFFFF; // Indicating SGPIO operation
	req->Cmd_Initiator = core;
	req->Data_Transfer_Length = sizeof(smp_request);
	smp_req=(smp_request *)core_map_data_buffer(req);
	smp_req->function=WRITE_GPIO_REGISTER;
	smp_req->smp_frame_type=SMP_REQUEST_FRAME;
	core_unmap_data_buffer(req);
	req->Completion = (void(*)(MV_PVOID,PMV_Request))sgpio_smpreq_callback;
	/* Make SG table */
	SGTable_Init(sg_table, 0);

	/* Send this internal request */
	Core_ModuleSendRequest(core, req);
}

void sgpio_process_sdin(MV_PVOID this, MV_U32 sgpio)
{
	core_extension * core=(core_extension *)this;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;

	CORE_DPRINT(("sgpio: sdin done :"));
	{
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(sgpio,SGPIO_REG_RAW_DIN0 ),
			tmp);
		CORE_DPRINT((" 0x%08x",tmp));
		/* for 6480 chip SGPIO 0 and SGPIO 1 will be set at same time even no back plan on SGPIO1.*/
		if(((tmp&SDIN_BACK_PLAN_PRESENCE_PATTERN)==SDIN_BACK_PLAN_PRESENCE_PATTERN)
		&&((tmp&SDIN_DATA_MASK)!=SDIN_DATA_MASK)){
			CORE_DPRINT(("sgpio data-in device present\n"));
			if((tmp&SDIN_DEVICE0_PRESENCE_PATTERN)==SDIN_DEVICE0_PRESENCE_PATTERN){
				CORE_DPRINT(("device 0 presence\n"));
				
			}
			if((tmp&SDIN_DEVICE1_PRESENCE_PATTERN)==SDIN_DEVICE1_PRESENCE_PATTERN)
			CORE_DPRINT(("device 1 presence\n"));
			if((tmp&SDIN_DEVICE2_PRESENCE_PATTERN)==SDIN_DEVICE2_PRESENCE_PATTERN)
			CORE_DPRINT(("device 2 presence\n"));
			if((tmp&SDIN_DEVICE3_PRESENCE_PATTERN)==SDIN_DEVICE3_PRESENCE_PATTERN)
			CORE_DPRINT(("device 3 presence\n"));
		} else
			CORE_PRINT(("no sgpio data-in device connected\n"));
	}
	CORE_DPRINT(("\n"));
	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
	((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

}

void sgpio_isr(MV_PVOID this, MV_U32 sgpio)
{
	core_extension * core=(core_extension *)this;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;
	void (*callback)(MV_PVOID PortExtension, MV_PVOID context) = core->lib_gpio.sgpio_callback;

	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(sgpio, SGPIO_REG_INT_CAUSE),
		tmp);
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(sgpio, SGPIO_REG_INT_CAUSE),
		tmp);

	if (tmp & SDIN_DONE)
		sgpio_process_sdin(this,sgpio);
	else if (tmp & MANUAL_MD_REP_DONE)
		callback(this, core->lib_gpio.sgpio_cb_context);
}

MV_U8 sgpio_send_sdout(MV_PVOID This, MV_U8 byte_len, MV_PVOID pbuffer,
	void (*call_back)(MV_PVOID PortExtension, MV_PVOID context),	MV_PVOID cb_context)
{
	core_extension * pCore=(core_extension *)This;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp,cntrl_tmp;
	MV_U8 i,j;

	if(pCore->lib_gpio.sgpio_sdout_inuse)
		return(MV_FALSE);

	mv_sgpio_read_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp);
	cntrl_tmp &=~(SDIN_MD_MASK|SDOUT_MD_MASK|MANUAL_MD_REP_CNT_MASK);
#ifdef SUPPORT_SGPIO_DATA_IN
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp+(SCLK_HALT<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE);
#else
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp+(SCLK_HALT<<SDOUT_MD_OFFSET));
#endif

	pCore->lib_gpio.sgpio_sdout_inuse=1;
	pCore->lib_gpio.sgpio_callback = call_back;
	pCore->lib_gpio.sgpio_cb_context = cb_context;
	for (i = 0; (i < (byte_len/4)) && (i < MAX_SDOUT_DWORD); i++) {
		tmp=*((MV_U32 *)pbuffer + i*4);
		mv_sgpio_write_register(mmio,
		        SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
		        tmp);
	}
	if ((i<MAX_SDOUT_DWORD) && (byte_len%4)) {
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
			tmp);
	for(j=0;j<(byte_len%4);j++)
		tmp |= (*((MV_U8 *)pbuffer + i*4 + j)) << (j*8);
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
			tmp);
	}

	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		tmp);
	tmp &= ~MANUAL_BIT_LEN;
	tmp |= (byte_len*8)<<MANUAL_BIT_LEN_OFFSET;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		tmp);
#ifdef SUPPORT_SGPIO_DATA_IN
	cntrl_tmp +=
	(SDOUT_MD_MAUTO<<SDOUT_MD_OFFSET)+
	SDIN_MD_ONCE+
	(1<<MAUNAL_MD_REP_CNT_OFFSET);
#else
	cntrl_tmp +=
	(SDOUT_MD_MAUTO<<SDOUT_MD_OFFSET)+
	(1<<MAUNAL_MD_REP_CNT_OFFSET);
#endif
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp);

	return MV_TRUE;
}

MV_U8 sgpio_read_register(MV_PVOID this, MV_U8  reg_type, MV_U8 reg_indx,
		MV_U8    reg_cnt, MV_PVOID pbuffer)
{
	core_extension * pCore=(core_extension *)this;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp;
	MV_U8 i;

	switch(reg_type){
		case REG_TYPE_CONFIG:
			MV_CopyMemory(pbuffer, (MV_PVOID)&pCore->lib_gpio.sgpio_config, sizeof(struct _sgpio_config_reg));
			break;
		case REG_TYPE_RX:
			/* 4 drive per register, 3 bits in one byte per drive,  */
			for (i = 0; (i < reg_cnt) && (i < MAX_SDIN_DWORD); i++) {
				mv_sgpio_read_register(mmio,
				SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DIN0 + (reg_indx + i)*4),
				tmp);
				*((MV_U32 *)pbuffer + i*4)=tmp;
			}
			break;
		case REG_TYPE_RX_GP:
			for (i = 0; (i < reg_cnt) && ((reg_indx + i) < (MAX_SDIN_DWORD+1)); i++) {
				if((reg_indx+i)==0)
					*(MV_U32 *)(pbuffer)=0;
				else {
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,
						SGPIO_REG_RAW_DIN0 + (reg_indx + i -1)*4),
						tmp);
					*((MV_U32 *)pbuffer + i*4)=tmp;
				}
			}
			break;
		case REG_TYPE_TC:
			for(i = 0; (i < reg_cnt) && (i < MAX_AUTO_CTRL_DWORD); i++) {
			mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_DRV_CTRL_BASE + (reg_indx + i)*4),
			tmp);
			*((MV_U32 *)pbuffer + i*4)=tmp;
			}
			break;
		case REG_TYPE_TC_GP:
			for (i = 0; (i < reg_cnt) && (i < MAX_SDOUT_DWORD); i++) {
				if((reg_indx+i)==0){
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
						tmp);
					tmp &=~(MANUAL_MD_SLOAD_PTRN_MASK|AUTO_MD_SLOAD_PTRN_MASK);
					tmp|=((*((MV_U8 *)pbuffer+3))<<16 )+
					((*((MV_U8 *)pbuffer+3))<<20 );
					mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),tmp);
				} else {
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + (reg_indx + i -1)*4),
						tmp);
					*((MV_U32 *)pbuffer + i*4) = tmp;
				}
			}
			break;
		default:
			return(MV_FALSE);
	}
	return(MV_TRUE);
}

MV_U8 sgpio_write_register( MV_PVOID this,MV_U8 reg_type,MV_U8 reg_indx,
	MV_U8 reg_cnt, MV_PVOID pbuffer)
{
	core_extension * pCore=(core_extension *)this;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp;
	MV_U8 i;

	switch (reg_type) {
		case REG_TYPE_CONFIG:
			MV_CopyMemory((MV_PVOID)&pCore->lib_gpio.sgpio_config, pbuffer, sizeof(struct _sgpio_config_reg));
			break;
		case REG_TYPE_TC:
			for (i = 0; (i < reg_cnt) && (i < MAX_AUTO_CTRL_DWORD); i++) {
				tmp = *((MV_U32 *)pbuffer + i*4);
				mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(0,SGPIO_REG_DRV_CTRL_BASE + (reg_indx + i)*4), tmp);
			}
			break;
		case REG_TYPE_TC_GP:
			for(i=0;(i<reg_cnt)&&(i<MAX_SDOUT_DWORD);i++){
				mv_sgpio_read_register(mmio,
					SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + (reg_indx + i)*4),
					tmp);
				*((MV_U32 *)pbuffer + i*4) = tmp;
			}
			break;
		case REG_TYPE_RX:
			default:
		return(MV_FALSE);
	}
	return(MV_TRUE);
}

#endif
