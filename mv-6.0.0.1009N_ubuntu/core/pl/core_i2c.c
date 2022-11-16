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
#ifdef _OS_UKRN
#include "stdio.h"
#endif

#ifdef SUPPORT_I2C
#if 0
/*should move these code to product folder xx_hal.c*/
/* XXX: It's appreciated to add SUPPORT_XXX like-wise for sharing source code. Y.C */
MV_U8 read_i2c_register(core_extension *core, MV_U8 port, MV_U8 reg_index)
{

    MV_LPVOID mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(port));
    MV_U32 reg = 0;
    #ifndef LOKI_PLUS_A1
    mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(EXT_I2C_DEV_PORT));
    reg = MV_REG_READ_DWORD(mmio, MI2C_CONTROL);
    mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(port));
    #endif
    reg = MV_REG_READ_DWORD(mmio, reg_index);
    #ifndef LOKI_PLUS_A1
    MV_REG_READ_DWORD(mmio, 0x18);
    #endif
    HBA_SleepMillisecond(core, 1);
    return ((MV_U8)(reg & 0xFF));
}

void write_i2c_register(core_extension *core, MV_U8 port, MV_U8 reg_index, MV_U8 reg_val)
{
    MV_LPVOID mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(port));

    #ifndef LOKI_PLUS_A1
    MV_U32 reg = 0;
    mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(EXT_I2C_DEV_PORT));
    reg = MV_REG_READ_DWORD(mmio, MI2C_CONTROL);
    mmio = (MV_LPVOID)((MV_PU8)core->mmio_base+TWSI_REG_BASE(port));
    #endif

    MV_REG_WRITE_DWORD(mmio, reg_index, reg_val);
    #ifndef LOKI_PLUS_A1
    MV_REG_READ_DWORD(mmio, 0x18);
    #endif
    HBA_SleepMillisecond(core, 1);

    return;
}
#endif
void i2c_disable_all_port(core_extension *core)
{
	MV_U8 port = 0;

	for (port = 0; port < 3; port++) {
	/* Reset the TWSI logic */
	write_i2c_register(core, port, MI2C_SOFT_RESET, 0);

		/* wait for 1m sec */
		HBA_SleepMillisecond(core, 1);

		write_i2c_register(core, port, MI2C_CONTROL, 0);
	}
}

#define READ_I2C_REG(core, regIndex) read_i2c_register(core, EXT_I2C_DEV_PORT, regIndex)
#define WRITE_I2C_REG(core, regIndex, regVal) write_i2c_register(core, EXT_I2C_DEV_PORT, regIndex,  regVal)
MV_U8 i2c_find_ccr(MV_U32 link_rate)
{
	MV_U8 n, m, best_n = 0, best_m = 0;
	MV_U32 exp_portion, current_freq, min_diff, current_diff;

	min_diff = 0xFFFFFFFF;
	/* f=sys_clock/((M+1)*2^(N+1)), sys_clock=OscFreq/10 */
	for(n=0; n<8; n++){
		exp_portion = 1<<(n+1); /*2^(N+1)*/
		for(m=0; m<16; m++){
			current_freq=I2C_SYS_CLOCK/((m+1)*exp_portion);
			current_diff=(current_freq<=link_rate)?(link_rate-current_freq):0xFFFFFFFF;
			if(current_diff<min_diff){
				min_diff   = current_diff;
				best_n=n;
				best_m=m;
			}
		}
	}
	return((best_m<<3) | best_n );
}
	/* Search through possible address - Starting with the default addresses of C0 and c2.
	 * If the two fails then start trying from  00 till FF.
	 * Return:
	 * TRUE if there are valid addresses to try
	 * FALSE if there are no valid addresses to try
	 */
MV_BOOLEAN i2c_cycle_address( domain_i2c_link *i2c_link ){

	if( i2c_link->i2c_sep_address == I2C_SEP_ADDR1 )
		return MV_FALSE;

	if( i2c_link->i2c_sep_address == I2C_SEP_ADDR0 )
		i2c_link->i2c_sep_address = I2C_SEP_ADDR1;
	else
		i2c_link->i2c_sep_address = I2C_SEP_ADDR0;
	return MV_TRUE;
}
void i2c_identify_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_i2c_link *i2c_link=&((core_extension *)(root->core))->lib_i2c.i2c_link;
	domain_enclosure *enc = (domain_enclosure *)get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, MV_FALSE);
	identify_sep_data *sep_identify;
	MV_PVOID buf_ptr;

	if ( req->Scsi_Status== REQ_STATUS_ERROR ){
		CORE_PRINT(("I2C callback error\n"));
		enc->status &= ~ENCLOSURE_STATUS_FUNCTIONAL;
		enc->state = I2C_STATE_RESET_DONE;
	} else {
		CORE_PRINT(("I2C Connected\n"));
                buf_ptr = core_map_data_buffer(req);
		MV_CopyMemory(&i2c_link->i2c_identify, buf_ptr, 64);
		sep_identify = (identify_sep_data *)i2c_link->i2c_identify;
		MV_CopyMemory(enc->enclosure_logical_id,
			sep_identify->logical_id,
			8);
		MV_CopyMemory(enc->vendor_id,
			sep_identify->vendor_id,
			8);
		MV_CopyMemory(enc->product_id,
			sep_identify->product_id,
			16);
		MV_CopyMemory(enc->product_revision,
			sep_identify->revision_level,
			4);
	   	enc->state = I2C_STATE_IDENTIFY_DONE;
                core_unmap_data_buffer(req);
	}

	core_queue_init_entry(root, &enc->base, MV_FALSE);

}

MV_Request *i2c_make_identify_request(domain_enclosure *enc)
{
	pl_root *root = enc->base.root;
	PMV_Request req = get_intl_req_resource(root, 64);

	if( req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return NULL;
	}

	CORE_DPRINT(("JL %s %d %s ... \n", __FILE__, __LINE__, __FUNCTION__));

	/* Prepare identify  */
	req->Cdb[0] = I2C_CMD_IDENTIFY_SEP;
	MV_LIST_HEAD_INIT(&req->Queue_Pointer);
	req->Device_Id = enc->base.id;
	req->Time_Out = 5;
	req->Completion = (void(*)(MV_PVOID,PMV_Request))i2c_identify_callback;
	/* Send this internal request */
	// core_append_init_request(root, req);
	return req;
}

static void i2c_stop_module(core_extension *core)
{
	WRITE_I2C_REG(core, MI2C_SOFT_RESET, 0);
	WRITE_I2C_REG(core,MI2C_CONTROL,0);
}

void pal_set_down_i2c_enclosure(core_extension *core, domain_enclosure *enc)
{
	core_complete_device_waiting_queue(enc->base.root, &enc->base, REQ_STATUS_NO_DEVICE);

	/* it's not DEVICE_STATUS_FUNCTIONAL */
	enc->status = 0;
	/*don't notify device unplug since at this time device hasn't init down*/
	//core_notify_device_hotplug(enc->base.root, MV_FALSE, enc->base.id, MV_TRUE);

	CORE_EH_PRINT(("set down disk %d\n", enc->base.id));

	free_enclosure_obj(enc->base.root, &core->lib_rsrc, enc);
}
#if defined(SUPPORT_I2C_SES)
void i2c_supported_page_callback(MV_PVOID root_p, PMV_Request req)
{
    pl_root *root = (pl_root *)root_p;
    domain_i2c_link *i2c_link=&((core_extension *)(root->core))->lib_i2c.i2c_link;
    domain_enclosure *enc = (domain_enclosure *)get_device_by_id(
        root->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
    MV_U8 *response = (MV_PU8)core_map_data_buffer(req);

    if( req->Scsi_Status!=REQ_STATUS_ERROR ){
        i2c_link->supported_page_count=response[3]; //32 pages max for now
        if(i2c_link->supported_page_count>32)
            i2c_link->supported_page_count = 32;
        MV_CopyMemory(i2c_link->supported_page,&response[4],i2c_link->supported_page_count);
        enc->supported_page_count = response[3];
        if (enc->supported_page_count > 32)
            enc->supported_page_count = 32;
        MV_CopyMemory(enc->supported_page,&response[4],enc->supported_page_count);
	}
   	enc->state = I2C_STATE_INQUIRY_DONE;

	core_unmap_data_buffer(req);
	core_queue_init_entry(root, &enc->base, MV_FALSE);
}

MV_BOOLEAN i2c_check_supported_page(core_extension *core,
	MV_U8 page_code)
{
	domain_i2c_link *i2c_link=&core->lib_i2c.i2c_link;
	MV_U8 i;
	if(page_code == SES_PG_SUPPORTED_DIAGNOSTICS)
		return MV_TRUE;
	for(i=0;i<i2c_link->supported_page_count;i++){
		if(i2c_link->supported_page[i] == page_code)
			return MV_TRUE;
	}
	return MV_FALSE;
}

void ses_internal_req_callback(MV_PVOID root_p, PMV_Request req) ;
void i2c_assign_device_element_number(domain_enclosure *i2c_enc);
MV_Request *ses_make_receive_diagnostic_request(domain_enclosure *enc,MV_U8 page_code,
	MV_ReqCompletion completion);
#define	ses_make_configuration_request(a,callback)		(ses_make_receive_diagnostic_request(a,SES_PG_CONFIGURATION,callback))
#define	ses_make_element_descriptor_request(a,callback)	(ses_make_receive_diagnostic_request(a,SES_PG_ELEMENT_DESCRIPTOR,callback))

#endif
MV_BOOLEAN i2c_device_state_machine(MV_PVOID i2c_enc_p)
{
	domain_enclosure *i2c_enc = (domain_enclosure *)i2c_enc_p;
	core_extension *core = (core_extension *)i2c_enc->base.root->core;
	domain_i2c_link *i2c_link=&core->lib_i2c.i2c_link;
	MV_Request *req = NULL;

	CORE_DPRINT(("i2c state: 0x%x\n",i2c_enc->state ));
	switch ( i2c_enc->state ){
		case I2C_STATE_RESET_DONE:

			/* To do inquiry */
			if( i2c_cycle_address( i2c_link ) ){
				i2c_enc->status |= ENCLOSURE_STATUS_FUNCTIONAL;
				req = i2c_make_identify_request(i2c_enc);
				break;
			}
			i2c_enc->state = I2C_STATE_IDENTIFY_DONE;
			i2c_stop_module(core);

		case I2C_STATE_IDENTIFY_DONE:
			if(i2c_enc->status&ENCLOSURE_STATUS_FUNCTIONAL){
#if defined(SUPPORT_I2C_SES)
				i2c_assign_device_element_number(i2c_enc);
				req = ses_make_receive_diagnostic_request(i2c_enc, SES_PG_SUPPORTED_DIAGNOSTICS,i2c_supported_page_callback);
				break;
#else
				i2c_enc->state=I2C_STATE_INIT_DONE;
#endif
			}else{
				pal_set_down_i2c_enclosure(core, i2c_enc);
				i2c_link->i2c_sep_address=0;
				/*####### enable SGPIO mode ######*/
				//MV_REG_WRITE_DWORD(core->mmio_base, 0x10104, 0x100);
				{
					MV_U32 tmp;
					tmp = MV_REG_READ_DWORD(core->mmio_base, 0x10104) & ~0x00000300L;
					tmp |= 0x00000100L;
					MV_REG_WRITE_DWORD(core->mmio_base, 0x10104, tmp);
				}
			}
			core_init_entry_done(i2c_enc->base.root,&core->lib_i2c.i2c_port,&i2c_enc->base);
			break;
#if defined(SUPPORT_I2C_SES)
		case I2C_STATE_INQUIRY_DONE:
			i2c_enc->state=I2C_STATE_INIT_DONE;
			req = ses_make_configuration_request(i2c_enc, ses_internal_req_callback);
			break;
		case ENCLOSURE_GET_CONFIGUATION_DONE:
			req = ses_make_element_descriptor_request(i2c_enc,ses_internal_req_callback);
		break;
		case ENCLOSURE_GET_ELEMENT_DISCRIPTER_DONE:
			i2c_enc->state = ENCLOSURE_INIT_DONE;
		case ENCLOSURE_INIT_DONE:
			return MV_TRUE;
#endif
		default:
			/* The first time initialization */
			break;
	}
	if(req)
		core_append_init_request(i2c_enc->base.root, req);
	return MV_TRUE;
}

void check_i2c_device(core_extension *core)
{
	domain_port *port;
	domain_enclosure *enc = NULL;

	enc = get_enclosure_obj(core->roots, &core->lib_rsrc);
	if (!enc) {
		CORE_DPRINT(("ran out of enclosure. abort initialization\n"));
		return;
	}

	port = &core->lib_i2c.i2c_port;
	port->base.type = BASE_TYPE_DOMAIN_PORT;
	port->base.port = port;
	port->base.root = core->roots;

	MV_LIST_HEAD_INIT(&port->device_list);
	MV_LIST_HEAD_INIT(&port->expander_list);
	MV_LIST_HEAD_INIT(&port->current_tier);
	MV_LIST_HEAD_INIT(&port->next_tier);
	port->device_count = 0;
	port->expander_count = 0;
	port->init_count = 0;
	port->phy_map=0;
	port->type = PORT_TYPE_SAS|PORT_TYPE_I2C; /* it is emulated as a SSP SES device */

	set_up_new_enclosure(core->roots, port, enc,
			(command_handler *)
			core_get_handler(core->roots, HANDLER_I2C));
	enc->base.type = BASE_TYPE_DOMAIN_I2C;
	enc->state = I2C_STATE_RESET_DONE;
#ifdef SUPPORT_MUL_LUN
	enc->base.TargetID = add_target_map(core->lib_dev.target_id_map, enc->base.id, MV_MAX_TARGET_NUMBER);
#endif

	/* Unmask TWSI IRQ */
#ifndef SUPPORT_ODIN
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG,
	MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG ) |INT_MAP_TWSI );
#endif
	core_queue_init_entry(enc->base.root,&enc->base,MV_TRUE);
}

void i2c_init(MV_PVOID core_p)
{
	core_extension *core=(core_extension *)core_p;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	MV_U8 regI2c;
#ifdef SUPPORT_BBU
	MV_U32 reg;
#endif
	CORE_DPRINT(("i2c init\n"));

#ifdef SUPPORT_ACTIVE_CABLE
	i2c_link->cable_status = CABLE_STATE_I2C_MODE;
#endif

#ifdef SUPPORT_BALDUR
	/*####### enable I2C mode ######*/
	//MV_REG_WRITE_DWORD(core->mmio_base, 0x10104, 0x300);
	/* for B0 board we use dedicated I2C connector, should not set register to share FLT pins,
	 if we still set this register, will cause I2C stop work*/
	if(core->revision_id == VANIR_A0_REV){
		MV_U32 tmp;
		tmp = MV_REG_READ_DWORD(core->mmio_base, 0x10104) & ~0x00000300L;
		tmp |= 0x00000300L;
		MV_REG_WRITE_DWORD(core->mmio_base, 0x10104, tmp);
	}
#elif defined(SUPPORT_ODIN)
{
  /* we had enabled INT_SLAVE_I2C before */
}
#else
	i2c_disable_all_port(core);

	/* Unmask TWSI IRQ */
	core->irq_mask |= INT_MAP_TWSI;
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG,
	MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG ) |INT_MAP_TWSI );
#endif

#ifdef SUPPORT_BBU
    //Frey B2 have to write R10104h[27] = 1 to enable BBU_REQ
    if (core->revision_id == VANIR_C2_REV) {
        reg = MV_REG_READ_DWORD(core->mmio_base, 0x10104);
        reg |= MV_BIT(27);
        MV_REG_WRITE_DWORD(core->mmio_base, 0x10104, reg);
        FM_PRINT("Enable BBU Req\n");
    }

	/* Set GPIO 3 as high to disable charger */
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT);
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT, (reg | MV_BIT(3)));
    reg = MV_REG_READ_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL);
    //Frey need to set 1 in GPIO_DATA_OUT_EN_CTRL to enable output
    if (IS_VANIR(core))
        reg |= MV_BIT(3);
    else
        reg &= ~(MV_BIT(3));
    MV_REG_WRITE_DWORD(core->mmio_base, GPIO_DATA_OUT_EN_CTRL, reg);

#if defined(SUPPORT_TWSI)
	bbu_initialize(core);
#else
	core->lib_bbu.bbu_state = BBU_STATE_INIT;
	bbu_state_machine(core);
#endif
#endif

#if defined(SUPPORT_TWSI)
	core->lib_twsi.major_state = TWSI_DEVICES_STATEMACHINE_INIT;
	twsi_initial_devices_statemachine(core);
#else
	/* init backplane */
	WRITE_I2C_REG(core, MI2C_SOFT_RESET, 0);

	regI2c=0;
	((reg_mi2c_control *)&regI2c)->enab=1;
	((reg_mi2c_control *)&regI2c)->ien=1;
	((reg_mi2c_control *)&regI2c)->aak=1;
	WRITE_I2C_REG(core, MI2C_CONTROL,regI2c);

	WRITE_I2C_REG(core, MI2C_SLAVE_ADDRESS, I2C_SEMB_ADDR);
	WRITE_I2C_REG(core, MI2C_X_SLAVE_ADDRESS,0);

	/* set the baud rate register to */
	regI2c=i2c_find_ccr(I2C_LINK_RATE);
	WRITE_I2C_REG(core, MI2C_CLOCK_CONTROL,regI2c);
	i2c_link->i2c_clock=regI2c;
#endif
	i2c_link->i2c_sep_address=0;
	check_i2c_device(core);
	return;
}

MV_U8 i2c_sata_checksum(MV_U8* buffer, MV_U32 length, MV_U8 sumseed)
{
	MV_U8	checksum = 0;
	MV_U32	index;

    checksum=-sumseed;

	/* Calculate the IPMB based checksum and return */
	for(index = 0; index < length; index ++){
		checksum += buffer[index];
	}

	/* Return type is a byte, a negative value is automatically adjusted */
	return -checksum;
}
#define I2C_STD_INQUIRY_SIZE	36
MV_U8 BASEATTR I2C_INQUIRY_DEVICE_DATA[I2C_STD_INQUIRY_SIZE] = {
	0x0D, 0x00, 0x05, 0x02, I2C_STD_INQUIRY_SIZE-5, 0x00, 0x40, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20 };

#define I2C_VPD_PAGE0_SIZE		7
MV_U8 BASEATTR I2C_INQUIRY_VPD_PAGE0_DATA[I2C_VPD_PAGE0_SIZE] = {
	0x0D, 0x00, 0x00, 0x03, 0x00, 0x80, 0x83};

/* VP PG80 Unit Serial Number, Header only */
#define I2C_VPD_PAGE80_SIZE		4
MV_U8 BASEATTR I2C_INQUIRY_VPD_PAGE80_DATA[I2C_VPD_PAGE80_SIZE] = {
	0x0D, 0x80, 0x00, 0x00};

/* VP PG83 Device Identification, Emulated whole page */
#define I2C_VPD_PAGE83_SIZE		0x18
MV_U8 BASEATTR I2C_INQUIRY_VPD_PAGE83_DATA[I2C_VPD_PAGE83_SIZE] = {
	0x0D, 0x83, 0x00, 0x14, 0x61, 0x93, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x61, 0x94, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
MV_U32 i2c_make_serial_number( MV_PU8 string, identify_sep_data *sep_identify, MV_U32 buffer_len )
{
	MV_U32 i;
	MV_CopyMemory(string, sep_identify->vendor_id, MV_MIN(buffer_len, 8));
	for(i=0;(((i*2)+8)<buffer_len)&&(i<8);i++){
#ifdef VS_2012
		sprintf_s((char *)(string+8+(i*2)),"%02x",sep_identify->logical_id[7-i]);
#else
		sprintf((char *)(string+8+(i*2)),"%02x",sep_identify->logical_id[7-i]);
#endif
        }
	return MV_MIN(buffer_len, (i*2)+8);
}
MV_QUEUE_COMMAND_RESULT i2c_inquiry_cmd(domain_i2c_link *i2c_link, PMV_Request req)
{
	identify_sep_data *sep_identify=(identify_sep_data *)(i2c_link->i2c_identify);
	MV_U8 status_code = REQ_STATUS_SUCCESS;
	MV_U8 sense_key = 0, ad_sense_code = 0;
	MV_U32 tmp_len = 0;
	MV_PU8 buf_ptr = (MV_PU8)core_map_data_buffer(req);

	if ((req->Cdb[1] & CDB_INQUIRY_EVPD) == 0) {
		/* Shall return standard INQUIRY data */
		if (req->Cdb[2] != 0) {
			/* PAGE CODE field must be zero when EVPD is zero for a valid request */
			/* sense key as ILLEGAL REQUEST and additional sense code as INVALID FIELD IN CDB */
			status_code = REQ_STATUS_HAS_SENSE;
			sense_key = SCSI_SK_ILLEGAL_REQUEST;
			ad_sense_code = SCSI_ASC_INVALID_FEILD_IN_CDB;
		} else {
			tmp_len = MV_MIN(req->Data_Transfer_Length, I2C_STD_INQUIRY_SIZE);
			MV_CopyMemory(buf_ptr, I2C_INQUIRY_DEVICE_DATA, tmp_len);
			if (req->Data_Transfer_Length > 8){
				MV_CopyMemory(&buf_ptr[8],
					sep_identify->vendor_id, MV_MIN((req->Data_Transfer_Length - 8), 8));

				if(req->Data_Transfer_Length > 16){
					MV_CopyMemory(&buf_ptr[16],
						sep_identify->product_id, MV_MIN((req->Data_Transfer_Length - 16), 16));

					if(req->Data_Transfer_Length > 32)
						MV_CopyMemory(&buf_ptr[32],
							sep_identify->revision_level, MV_MIN((req->Data_Transfer_Length - 32), 4));
				}
			}
		}
	} else {
			/* Shall return the specific page of Vital Production Data */
		switch (req->Cdb[2]) {
		case 0x00:	/* Supported VPD pages */
			tmp_len = MV_MIN(req->Data_Transfer_Length, I2C_VPD_PAGE0_SIZE);
			/* supported VPD page */
			MV_CopyMemory(buf_ptr, I2C_INQUIRY_VPD_PAGE0_DATA, tmp_len);
			break;
		case 0x80:	/* Unit Serial Number VPD Page */
			tmp_len = MV_MIN(req->Data_Transfer_Length, I2C_VPD_PAGE80_SIZE);
			MV_CopyMemory(buf_ptr, I2C_INQUIRY_VPD_PAGE80_DATA, tmp_len);
			if (req->Data_Transfer_Length > I2C_VPD_PAGE80_SIZE){
				tmp_len = i2c_make_serial_number(
					(MV_PU8)(buf_ptr)+I2C_VPD_PAGE80_SIZE,
					sep_identify,
					req->Data_Transfer_Length - I2C_VPD_PAGE80_SIZE);
				*(((MV_PU8)buf_ptr) + 3) = I2C_VPD_PAGE80_SIZE + 24;
				/* serial number= 8 charactors of Vendor String + 16 charactors of 8 bytes-naa id */
				tmp_len += I2C_VPD_PAGE80_SIZE;
			}
			break;
		case 0x83:	/* Device Identification VPD Page */
			tmp_len = MV_MIN(req->Data_Transfer_Length, I2C_VPD_PAGE83_SIZE);
			MV_CopyMemory(buf_ptr, I2C_INQUIRY_VPD_PAGE83_DATA, tmp_len);
			if (req->Data_Transfer_Length > 8){
				MV_CopyMemory((MV_PU8)buf_ptr + 8,
					sep_identify->logical_id, MV_MIN((req->Data_Transfer_Length - 8), 8));

				if(req->Data_Transfer_Length > 23)
					*((MV_PU8)buf_ptr+ 23) = sep_identify->channel_id;
			}
			break;
		default:
			status_code = REQ_STATUS_HAS_SENSE;
			sense_key = SCSI_SK_ILLEGAL_REQUEST;
			ad_sense_code = SCSI_ASC_INVALID_FEILD_IN_CDB;
			break;
		}
	}

	if (status_code==REQ_STATUS_HAS_SENSE) {
		if (req->Sense_Info_Buffer != NULL) {
			((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70;	/* Current */
			((MV_PU8)req->Sense_Info_Buffer)[2] = sense_key;
			((MV_PU8)req->Sense_Info_Buffer)[7] = 0;		/* additional sense length */
			((MV_PU8)req->Sense_Info_Buffer)[12] = ad_sense_code;	/* additional sense code */
		}
	}
	req->Data_Transfer_Length = tmp_len;
	req->Scsi_Status = status_code;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

#if !defined(SUPPORT_TWSI)
void i2c_send_i2c_frame(core_extension *core, PMV_Request req)
{
	domain_i2c_link *i2c_link=&core->lib_i2c.i2c_link;
	MV_U8 i2c_ctl_reg;
#if defined(I2C_NAK20_WORKAROUND)
	WRITE_I2C_REG(core, MI2C_SOFT_RESET, 0);

	i2c_ctl_reg=0;
	((reg_mi2c_control *)&i2c_ctl_reg)->enab=1;
	((reg_mi2c_control *)&i2c_ctl_reg)->ien=1;
	((reg_mi2c_control *)&i2c_ctl_reg)->aak=1;
	WRITE_I2C_REG(core,MI2C_CONTROL,i2c_ctl_reg);

	WRITE_I2C_REG(core,MI2C_SLAVE_ADDRESS,I2C_SEMB_ADDR);
	WRITE_I2C_REG(core,MI2C_X_SLAVE_ADDRESS,0);

	/* set the baud rate register to */
	i2c_ctl_reg=i2c_find_ccr(I2C_LINK_RATE);
	WRITE_I2C_REG(core,MI2C_CLOCK_CONTROL,i2c_ctl_reg);
#endif
	WRITE_I2C_REG(core,MI2C_DATA,i2c_link->i2c_cmd_header[0]);
	i2c_link->i2c_xfer_count=0;
	i2c_link->i2c_state=I2C_STATE_CMD;
	i2c_link->i2c_request=req;
	i2c_ctl_reg = READ_I2C_REG(core, MI2C_CONTROL);
	((reg_mi2c_control *)&i2c_ctl_reg)->sta=1;
	WRITE_I2C_REG(core, MI2C_CONTROL, i2c_ctl_reg);
}
#endif /*#!SUPPORT_TWSI*/

#if defined(SUPPORT_TWSI)
MV_QUEUE_COMMAND_RESULT i2c_to_twsi(core_extension *core, MV_Request *req);
#endif
MV_QUEUE_COMMAND_RESULT i2c_send_command(MV_PVOID root, MV_PVOID dev_p,
	MV_Request *req)
{
	core_extension *core = (core_extension *)((pl_root *)root)->core;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	MV_U32 data_transfer_length;
        MV_PVOID buf_ptr;

	if( (i2c_link->i2c_state==I2C_STATE_CMD)||
		(i2c_link->i2c_state==I2C_STATE_RESPONSE) )
		return MV_QUEUE_COMMAND_RESULT_FULL;
#if defined(SUPPORT_I2C_SES)
	if( ((req->Cdb[0]==SCSI_CMD_RCV_DIAG_RSLT)||
		(req->Cdb[0]==SCSI_CMD_SND_DIAG))&&!i2c_check_supported_page(core,req->Cdb[2]) )
	{
		req->Scsi_Status = REQ_STATUS_ERROR;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif
	switch(req->Cdb[0]){
	case SCSI_CMD_INQUIRY:
		return i2c_inquiry_cmd(i2c_link,req);
	case I2C_CMD_IDENTIFY_SEP:
	case SCSI_CMD_RCV_DIAG_RSLT:
	/* build the I2C SEP command frame */
		i2c_link->i2c_cmd_header[0] = (req->Req_Flag & REQ_FLAG_EXTERNAL) ? req->Cdb[1] : i2c_link->i2c_sep_address;
		i2c_link->i2c_cmd_header[1] = I2C_CT_D2H_SES;
		i2c_link->i2c_cmd_header[2] = i2c_sata_checksum(i2c_link->i2c_cmd_header, 2, 0);
		i2c_link->i2c_cmd_header[3] = (req->Req_Flag & REQ_FLAG_EXTERNAL) ? req->Cdb[3] : I2C_SEMB_ADDR;
		i2c_link->i2c_cmd_header[4] = 0;
		/*pI2cLink->I2cCmdHeader[4] = i2cCmdSeq++;*/
		if (req->Cdb[0] == I2C_CMD_IDENTIFY_SEP)
			i2c_link->i2c_cmd_header[5] = req->Cdb[0];
		else if (req->Cdb[0] == SCSI_CMD_RCV_DIAG_RSLT)
			i2c_link->i2c_cmd_header[5] = req->Cdb[2];		// for SES requests, use page code
		i2c_link->i2c_cmd_header[6] = i2c_sata_checksum(&i2c_link->i2c_cmd_header[3], 3, 0);

		i2c_link->i2c_cmd_frame_len = 7;
		i2c_link->i2c_rsp_frame_len= 8 + (MV_U16)req->Data_Transfer_Length;
		break;
	case SCSI_CMD_SND_DIAG:
		buf_ptr = core_map_data_buffer(req);
        data_transfer_length = ((((MV_U8*)buf_ptr)[2]<<8) + (((MV_U8*)buf_ptr)[3])) + 4;
        if(req->Data_Transfer_Length < data_transfer_length)
            data_transfer_length = req->Data_Transfer_Length;
#if defined(SUPPORT_TWSI)
        req->Data_Transfer_Length = data_transfer_length;
#endif
		/* build the I2C SEP command frame */
		i2c_link->i2c_cmd_header[0] = (req->Req_Flag & REQ_FLAG_EXTERNAL) ? req->Cdb[1] : i2c_link->i2c_sep_address;
		i2c_link->i2c_cmd_header[1] = I2C_CT_H2D_SES;
		i2c_link->i2c_cmd_header[2] = i2c_sata_checksum(i2c_link->i2c_cmd_header, 2, 0);
		i2c_link->i2c_cmd_header[3] = (req->Req_Flag & REQ_FLAG_EXTERNAL) ?req->Cdb[3] : I2C_SEMB_ADDR;
		i2c_link->i2c_cmd_header[4] = 0;
		/*pCore->I2cCmdHeader[4] = i2cCmdSeq++;*/
		i2c_link->i2c_cmd_header[5] = *((MV_U8*)buf_ptr);
		i2c_link->i2c_cmd_header[6] = i2c_sata_checksum(buf_ptr, data_transfer_length, i2c_sata_checksum(&i2c_link->i2c_cmd_header[3], 3, 0));

		i2c_link->i2c_cmd_frame_len = 7 + (MV_U16)data_transfer_length;
		i2c_link->i2c_rsp_frame_len = 8 + (MV_U16)data_transfer_length;
                core_unmap_data_buffer(req);
		break;
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
  /* why not SUPPORT_LOKI */
#if 0//!defined(SUPPORT_BALDUR) && !defined(SUPPORT_ODIN)
	reg = MV_REG_READ_DWORD(core->Mmio_Base, PCIE_IRQ_ENABLE_F0);
	reg |= IRQ_I2C;
	MV_REG_WRITE_DWORD(core->Mmio_Base, PCIE_IRQ_ENABLE_F0, reg);
	reg = MV_REG_READ_DWORD(core->Mmio_Base, MAIN_IRQ_ENABLE);
	reg |= IRQ_I2C;
	MV_REG_WRITE_DWORD(core->Mmio_Base, MAIN_IRQ_ENABLE, reg);
#endif

#if defined(SUPPORT_TWSI)
	return i2c_to_twsi(core, req);
#else
	i2c_send_i2c_frame(core,req);
	return MV_QUEUE_COMMAND_RESULT_SENT;
#endif
}

#if !defined(SUPPORT_TWSI)
MV_BOOLEAN i2c_enclosure_isr(core_extension *core)
{
	MV_U8 i2c_ctl_reg,i2c_status_reg;
	domain_i2c_link *i2c_link=&core->lib_i2c.i2c_link;
	MV_U8 *buffer;
	MV_BOOLEAN b_do_reset=MV_FALSE, b_completed=MV_FALSE;
	PMV_Request req;
	domain_enclosure *enc;
	MV_ULONG flags;

	if( i2c_link->i2c_request==NULL )
		return MV_FALSE;

	req = (PMV_Request)i2c_link->i2c_request;

	i2c_ctl_reg = READ_I2C_REG(core, MI2C_CONTROL);

	if( (!((reg_mi2c_control *)&i2c_ctl_reg)->ien) || (!((reg_mi2c_control *)&i2c_ctl_reg)->iflg) ) {
		CORE_DPRINT(("JL %s %d %s ...I2cCtlReg->IEN 0x%X,  I2cCtlReg->IFLG 0x%X\n",
			__FILE__, __LINE__, __FUNCTION__, ((reg_mi2c_control *)&i2c_ctl_reg)->ien, ((reg_mi2c_control *)&i2c_ctl_reg)->iflg));
		return MV_FALSE;
	}

	i2c_status_reg = READ_I2C_REG(core, MI2C_STATUS);
	buffer = core_map_data_buffer(req);

	switch(i2c_link->i2c_state) {
		case I2C_STATE_CMD:
			
			switch(i2c_status_reg) {
				case MI2C_STATUS_START_TX:
				case MI2C_STATUS_ADDR_W_TX_ACK:
				case MI2C_STATUS_MDATA_TX_ACK:
					if (i2c_link->i2c_xfer_count < i2c_link->i2c_cmd_frame_len) {
						if(i2c_link->i2c_cmd_header[1]==I2C_CT_H2D_SES) {
							/* I2cCmdHeader = [6 bytes of cmd header]+[checksum]
							I2cCmdFrameLen = length of SES request page + 7
							*/
							/*I2C_CT_H2D_SES: [6 bytes of cmd header]+[SES request page]+[checksum] */
							if(i2c_link->i2c_xfer_count<6)
								WRITE_I2C_REG(core,MI2C_DATA,i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]);
							else if(i2c_link->i2c_xfer_count==(i2c_link->i2c_cmd_frame_len-1))
								WRITE_I2C_REG(core,MI2C_DATA,i2c_link->i2c_cmd_header[6]);
							else
								WRITE_I2C_REG(core,MI2C_DATA,buffer[i2c_link->i2c_xfer_count-6]);
						} else
							WRITE_I2C_REG(core,MI2C_DATA,i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]);
					/*I2C_CT_D2H_SES: [6 bytes of cmd header]+[checksum] */
					i2c_link->i2c_xfer_count++;
					} else {
						/* last byte transfered, generate a STOP signal */
						((reg_mi2c_control *)&i2c_ctl_reg)->stp=1;
						/* move to next command phase, and reset the count for receiving */
						i2c_link->i2c_state=I2C_STATE_RESPONSE;
						i2c_link->i2c_xfer_count=0;
					}
					break;
				default:
					/* error occurred */
					if (i2c_link->i2c_xfer_count)
					    ((reg_mi2c_control *)&i2c_ctl_reg)->stp=1;
					else
					    b_do_reset=MV_TRUE;
					i2c_link->i2c_state=I2C_STATE_ERROR;
					req->Scsi_Status=REQ_STATUS_ERROR;
					b_completed=MV_TRUE;
					break;
			}
			break;

		case I2C_STATE_RESPONSE:
			switch(i2c_status_reg) {
				case MI2C_STATUS_ADDR_W_TX_ACK:
				case MI2C_STATUS_MDATA_TX_ACK:
				/* STOP sent */
					break;

				case MI2C_STATUS_SADDR_W_RX_ACK:
				case MI2C_STATUS_SADDR_DATA_RX_ACK:
					if (i2c_link->i2c_xfer_count < i2c_link->i2c_rsp_frame_len) {
						/* for response, I2cCmdHeader = [6 bytes of cmd header]+[status]+[checksum]
						 I2cRspFrameLen = length of SES request page + 8
						*/
						/*I2C_CT_H2D_SES: [6 bytes of cmd header]+[status]+[checksum]*/
						if (i2c_link->i2c_cmd_header[1]==I2C_CT_H2D_SES)
							i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]=READ_I2C_REG(core, MI2C_DATA);
						else {
							 /*I2C_CT_D2H_SES: [6 bytes of cmd header]+[status]+[SES response page]+[checksum] */
							 if (i2c_link->i2c_xfer_count < 7)
								i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]=READ_I2C_REG(core, MI2C_DATA);
							 else if (i2c_link->i2c_xfer_count == (i2c_link->i2c_cmd_frame_len - 1))
								i2c_link->i2c_cmd_header[7]=READ_I2C_REG(core, MI2C_DATA);
							 else
							 	buffer[i2c_link->i2c_xfer_count-7]=READ_I2C_REG(core, MI2C_DATA);
						}
						i2c_link->i2c_xfer_count++;
					} else {
						CORE_PRINT(("I2c Receiver buffer overflow, 0x%x\n",i2c_link->i2c_xfer_count++));
					}
					break;

				case MI2C_STATUS_STOP_RX:
					((reg_mi2c_control *)&i2c_ctl_reg)->stp=1;
					i2c_link->i2c_state=I2C_STATE_IDLE;
					req->Scsi_Status=REQ_STATUS_SUCCESS;
					b_completed=MV_TRUE;
					break;

				default:
					/* error occurred */
					b_do_reset=MV_TRUE;
					req->Scsi_Status=REQ_STATUS_ERROR;
					i2c_link->i2c_state=I2C_STATE_ERROR;
					b_completed=MV_TRUE;
					break;
			}
			break;

		case I2C_STATE_IDLE:
		case I2C_STATE_ERROR:
		default:
			b_do_reset=MV_TRUE;
			break;
	}

	/* re-enable I2C : Write to MI2C_CONTROL Register */
	((reg_mi2c_control *)&i2c_ctl_reg)->iflg=0;
	WRITE_I2C_REG(core, MI2C_CONTROL, i2c_ctl_reg);

	if (b_do_reset) {
		WRITE_I2C_REG(core,MI2C_SOFT_RESET,0);
		WRITE_I2C_REG(core,MI2C_CLOCK_CONTROL,i2c_link->i2c_clock);
	}

	if (b_completed) {
#if 0//!defined(SUPPORT_BALDUR) && !defined(SUPPORT_ODIN)
	reg = MV_REG_READ_DWORD(core->Mmio_Base, PCIE_IRQ_ENABLE_F0);
	//CORE_PRINT(("*   IRQ status = 0x%x\n", reg));
	reg &= ~IRQ_I2C;
	//CORE_PRINT(("*   IRQ status (after) = 0x%x\n", reg));
	MV_REG_WRITE_DWORD(core->Mmio_Base, PCIE_IRQ_ENABLE_F0, reg);
	//CORE_PRINT(("*   I2C interrupt completed\n"));
	reg = MV_REG_READ_DWORD(core->Mmio_Base, MAIN_IRQ_ENABLE);
	//CORE_PRINT(("*   IRQ status = 0x%x\n", reg));
	reg &= ~IRQ_I2C;
	//CORE_PRINT(("*   IRQ status (after) = 0x%x\n", reg));
	MV_REG_WRITE_DWORD(core->Mmio_Base, MAIN_IRQ_ENABLE, reg);
#endif
	i2c_link->i2c_request=NULL;
	enc = (domain_enclosure *)get_device_by_id(&core->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);
	OSSW_SPIN_LOCK(&enc->base.err_ctx.sent_req_list_SpinLock, flags);
	mv_renew_timer(core, req);
	enc->base.outstanding_req--;
	OSSW_SPIN_UNLOCK(&enc->base.err_ctx.sent_req_list_SpinLock, flags);

        core_unmap_data_buffer(req);
#if defined(I2C_NAK20_WORKAROUND)
	if (req->Scsi_Status==REQ_STATUS_ERROR) {
		if(enc->base.err_ctx.retry_count < 5 ) {	/* Change the retry count from 3000 to 3 */
			Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
			enc->base.err_ctx.retry_count++;
			return MV_TRUE;
		}
	}
	enc->base.err_ctx.retry_count=0;
#endif
	core_queue_completed_req(core,req);
	}
	return MV_TRUE;
}
#endif /*!SUPPORT_TWSI*/

MV_VOID i2c_interrupt_service_routine(MV_PVOID core_p)
{
	core_extension *core=(core_extension *)core_p;
#if defined(SUPPORT_TWSI)
	twsi_isr_handler(core);
#else

#ifdef SUPPORT_BBU
	i2c_bbu_isr(core);
#endif
#ifdef SUPPORT_ACTIVE_CABLE
	if (core->lib_i2c.i2c_link.cable_status == CABLE_STATE_I2C_MODE)
#endif
		i2c_enclosure_isr(core);
#endif
}

#endif

#ifdef SUPPORT_ACTIVE_CABLE
extern MV_VOID core_return_finished_req(core_extension *core, MV_Request *req);
MV_BOOLEAN i2c_cable_isr(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	PMV_Request req;
	MV_U32	reg_val;
	MV_U8 *buffer;
	MV_U8 i2c_ctl_reg, i2c_status_reg, cable_id = i2c_link->cable_id;
	MV_BOOLEAN b_completed = MV_FALSE;

	if (i2c_link->i2c_request == NULL) {
		return MV_FALSE;
	}

	req = (PMV_Request)i2c_link->i2c_request;

	i2c_ctl_reg = read_i2c_register(core, cable_id, MI2C_CONTROL);

	if ((!((reg_mi2c_control *)&i2c_ctl_reg)->ien) ||
		(!((reg_mi2c_control *)&i2c_ctl_reg)->iflg)) {
		CORE_DPRINT(("JL %s ...I2cCtlReg->IEN 0x%X,  I2cCtlReg->IFLG 0x%X\n",
						__FUNCTION__,
						((reg_mi2c_control *)&i2c_ctl_reg)->ien,
						((reg_mi2c_control *)&i2c_ctl_reg)->iflg));
		((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
		req->Scsi_Status = REQ_STATUS_ERROR;

		return MV_FALSE;
	}

	i2c_status_reg = read_i2c_register(core, cable_id, MI2C_STATUS);
	buffer = core_map_data_buffer(req);

	switch (i2c_link->i2c_state) {
	case I2C_STATE_CMD:
		switch (i2c_status_reg) {
		case MI2C_STATUS_START_TX:
		case MI2C_STATUS_REP_START_TX:
		case MI2C_STATUS_ADDR_W_TX_ACK:
		case MI2C_STATUS_ADDR_R_TX_ACK:
			if (i2c_link->i2c_xfer_count < i2c_link->i2c_cmd_frame_len) {
				write_i2c_register(core, cable_id, MI2C_DATA,
						i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]);
				i2c_link->i2c_xfer_count++;
			} else {
				/* move to next command phase,
				 *  and reset the count for receiving
				 */
				i2c_link->i2c_state = I2C_STATE_RESPONSE;
				i2c_link->i2c_xfer_count = 0;
			}
			break;

		case MI2C_STATUS_MDATA_TX_ACK:
			if (i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count] ==
												SLAVE_ADDR_READ) {
				/* Restart */
				i2c_ctl_reg = read_i2c_register(core, cable_id,
													MI2C_CONTROL);
				((reg_mi2c_control *)&i2c_ctl_reg)->sta = 1;
				write_i2c_register(core, cable_id,
										MI2C_CONTROL, i2c_ctl_reg);
				break;
			}

			if (i2c_link->i2c_xfer_count < i2c_link->i2c_cmd_frame_len) {
				write_i2c_register(core, cable_id, MI2C_DATA,
						i2c_link->i2c_cmd_header[i2c_link->i2c_xfer_count]);
				i2c_link->i2c_xfer_count++;
			} else {
				i2c_ctl_reg = read_i2c_register(core, cable_id,
													MI2C_CONTROL);
				((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
				write_i2c_register(core, cable_id,
										MI2C_CONTROL, i2c_ctl_reg);
				i2c_link->i2c_state = I2C_STATE_IDLE;
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				b_completed = MV_TRUE;
			}
			break;

		default:
			/* error occurred */
			((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
			req->Scsi_Status = REQ_STATUS_ERROR;
			b_completed = MV_TRUE;
			break;
		}
		break;

	case I2C_STATE_RESPONSE:
		switch (i2c_status_reg) {
		case MI2C_STATUS_MDATA_TX_ACK:
			i2c_ctl_reg = read_i2c_register(core, cable_id, MI2C_CONTROL);
			((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
			write_i2c_register(core, cable_id, MI2C_CONTROL, i2c_ctl_reg);
			i2c_link->i2c_state = I2C_STATE_IDLE;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			b_completed = MV_TRUE;
			break;

		case MI2C_STATUS_MDATA_RX_ACK:
			if (i2c_link->i2c_xfer_count < i2c_link->i2c_rsp_frame_len) {
				buffer[i2c_link->i2c_xfer_count] = read_i2c_register(core,
													cable_id, MI2C_DATA);
				((reg_mi2c_control *)&i2c_ctl_reg)->aak = 1;
				i2c_link->i2c_xfer_count++;
			} else {
				i2c_ctl_reg = read_i2c_register(core, cable_id,
													MI2C_CONTROL);
				((reg_mi2c_control *)&i2c_ctl_reg)->aak = 0;
				write_i2c_register(core, cable_id,
										MI2C_CONTROL, i2c_ctl_reg);
			}
			break;

		case MI2C_STATUS_MDATA_RX_NAK:
			i2c_ctl_reg = read_i2c_register(core, cable_id, MI2C_CONTROL);
			((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
			write_i2c_register(core, cable_id, MI2C_CONTROL, i2c_ctl_reg);
			i2c_link->i2c_state = I2C_STATE_IDLE;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			b_completed = MV_TRUE;
			break;

		default:
			/* error occurred */
			((reg_mi2c_control *)&i2c_ctl_reg)->stp = 1;
			req->Scsi_Status = REQ_STATUS_ERROR;
			b_completed = MV_TRUE;
			break;
		}
		break;
	}

	/* re-enable I2C : Write to MI2C_CONTROL Register */
	((reg_mi2c_control *)&i2c_ctl_reg)->iflg = 0;
	write_i2c_register(core, cable_id, MI2C_CONTROL, i2c_ctl_reg);

	if (b_completed) {
		i2c_link->i2c_request = NULL;
		core_unmap_data_buffer(req);
		i2c_check_cable_callback(core, req);
		core_return_finished_req(core, req);
		//core_queue_completed_req(core, req);
	}
	return MV_TRUE;
}

void i2c_check_cable_callback(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = (core_extension *)core_p;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	domain_phy *phy;
	pl_root *root;
	MV_PU8 buf_ptr = (MV_PU8)core_map_data_buffer(req);

	MV_U32 reg_value, ctrl_reg, data_reg, i;
	MV_U8 cable_info[18], cable_type;
	MV_U8 start_phy_id, max_phy_num;
#ifdef SAS_PLUGFEST_CABLE
	MV_U8 vendor_name[17], vendor_oui[4], vendor_pin[17], vendor_rev[3], vendor_sn[17];
	cable_low_page0_data *p_page0;
	cable_upper_page0_data *p_up_page0;
#endif
	/* Set OOB D.C mode as default */
	cable_type = 0;

	if (req->Scsi_Status == REQ_STATUS_ERROR) {
		CORE_EH_PRINT(("Get cable info error in connect %d\n",
								i2c_link->cable_id));
		goto out;
	}
#ifdef SAS_PLUGFEST_CABLE
	p_page0=(cable_low_page0_data *)buf_ptr;
	p_up_page0=(cable_upper_page0_data *)&buf_ptr[128];
	MV_CopyMemory(cable_info, &buf_ptr[130], 18);
	if(p_page0){
		CORE_PRINT(("1. cable %d a. 0x%x,b. 0x%x c. 0x%x \n", i2c_link->cable_id, p_page0->identifier, p_page0->status[0], p_page0->status[1]));
		CORE_PRINT(("1. d. 0x%x,e. 0x%x f. 0x%x\n", p_page0->free_side_device_property[0], p_page0->free_side_device_property[1], p_page0->free_side_device_property[2]));
		CORE_PRINT(("2. cable %d a. 0x%x,b.c.d. 0x%x e. 0x%x \n", i2c_link->cable_id, p_up_page0->identifier, p_up_page0->ext_identifier, p_up_page0->connect));
		CORE_PRINT(("2. f. 0x%x,g. 0x%x l.m.n.o. 0x%x dev_tech 0x%x\n", p_up_page0->module[2], p_up_page0->br_nominal, p_up_page0->options[2], p_up_page0->dev_tech));
        	MV_CopyMemory(vendor_name, p_up_page0->vendor_name, 16);
        	vendor_name[16]='\0';
        	MV_CopyMemory(vendor_oui, p_up_page0->vendor_oui, 3);
        	vendor_oui[3]='\0';
        	MV_CopyMemory(vendor_pin, p_up_page0->vendor_pin, 16);
        	vendor_pin[16]='\0';
        	MV_CopyMemory(vendor_rev, p_up_page0->vendor_rev, 2);
        	vendor_rev[2]='\0';
        	MV_CopyMemory(vendor_sn, p_up_page0->vendor_sn, 16);
        	vendor_sn[16]='\0';
		CORE_PRINT(("2. h. %s i. %s j. %s k. %s, p. %s\n",vendor_name, vendor_oui, vendor_pin, vendor_rev, vendor_sn));
		CORE_PRINT(("2. Y%c%c M%c%c D%c%c lot %c%c u. 0x%x\n", p_up_page0->date_code[0], p_up_page0->date_code[1], p_up_page0->date_code[2], p_up_page0->date_code[3],
						p_up_page0->date_code[4], p_up_page0->date_code[5], p_up_page0->date_code[6], p_up_page0->date_code[7], p_up_page0->cc_ext));
	}
#else
	MV_CopyMemory(cable_info, buf_ptr, 18);
#endif
	/* read cable type */
	if ((cable_info[0] == 0x0) && (cable_info[16] == 0x0)
			&& (cable_info[17] == 0x0)) {
		/* workaround: old copper cable with no data in EPPROM*/
		CORE_PRINT(("Copper active cable plug in connect %d\n",
								i2c_link->cable_id));
		cable_type = 0;
	} else if ((cable_info[0] == 0x21) || ((cable_info[17] & 0xF0) > 0x90)) {
		/* copper cable */
		CORE_PRINT(("Copper active cable plug in connect %d\n",
								i2c_link->cable_id));
		cable_type = 0;
	} else if (((cable_info[0] == 0xB) || (cable_info[0] == 0x23))
				&& ((cable_info[17] & 0xF0) <= 0x90)) {
		/* optical cable */
		CORE_PRINT(("Optical active cable plug in connect %d\n",
								i2c_link->cable_id));
		cable_type = 1;
	} else {
		/* unknow cable type */
		CORE_EH_PRINT(("Unkown cable type in connect %d\n",
								i2c_link->cable_id));
		goto out;
	}

	/* read cable length */
	if (cable_info[16] != 0x0) {
		CORE_PRINT(("cable length=%d\n", cable_info[16]));
	} else {
		CORE_EH_PRINT(("Unkown cable length\n"));
	}

out:
	ctrl_reg = COMMON_PORT_PHY_CONTROL0;
	data_reg = COMMON_PORT_PHY_CONTROL_DATA0;

	/* get root id by cable_id */
	if (i2c_link->cable_id < 3) {
		root = &core->roots[0];
	} else {
		root = &core->roots[1];
	}

	/* get phy num by cable_id */
	if (i2c_link->cable_id % 2 == 1) {
		/* set root phy[0-3] */
		max_phy_num = 4;
		start_phy_id = 0;
	} else {
		/* set root phy[4-7] */
		max_phy_num = 8;
		start_phy_id = 4;
	}

	/* Set phy OOB mode */
	for (i = start_phy_id; i < max_phy_num; i++) {

		if (cable_type == 1) {

			MV_REG_WRITE_DWORD(root->mmio_base,
									ctrl_reg + i * 8, PHY_MODE_REG1);
			reg_value = MV_REG_READ_DWORD(root->mmio_base, data_reg + i * 8);
			reg_value |= (1 << 26);
			MV_REG_WRITE_DWORD(root->mmio_base, data_reg + i * 8, reg_value);

		} else {

			MV_REG_WRITE_DWORD(root->mmio_base,
									ctrl_reg + i * 8, PHY_MODE_REG1);
			reg_value = MV_REG_READ_DWORD(root->mmio_base, data_reg + i * 8);
			reg_value &= ~(1 << 26);
			MV_REG_WRITE_DWORD(root->mmio_base, data_reg + i * 8, reg_value);
		}
	}

	/* reset cable related phys */
	for (i = start_phy_id; i < max_phy_num; i++) {
			phy = &root->phy[i];
			
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
			reg_value = READ_PORT_CONFIG_DATA(root, phy);
			reg_value |= SCTRL_PHY_HARD_RESET_SEQ | 
						SCTRL_STP_LINK_LAYER_RESET | 
						SCTRL_SSP_LINK_LAYER_RESET;
			WRITE_PORT_CONFIG_DATA(root, phy, reg_value);
	}
	core_sleep_millisecond(root->core, 100);

	write_i2c_register(core, i2c_link->cable_id, MI2C_SOFT_RESET, 0);
	core_unmap_data_buffer(req);
	i2c_link->cable_status = CABLE_STATE_FINISH;
}

void i2c_read_cable_mode_command(core_extension *core)
{
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;

	/* build the I2C SEP command frame */
	i2c_link->i2c_cmd_header[0] = SLAVE_ADDR_WRITE;
#ifdef SAS_PLUGFEST_CABLE
	i2c_link->i2c_cmd_header[1] = 0;	//offset
#else
	i2c_link->i2c_cmd_header[1] = 130;	//offset
#endif
	i2c_link->i2c_cmd_header[2] = SLAVE_ADDR_READ;

	/* TBD: read cable length */

	/* write byte length */
	i2c_link->i2c_cmd_frame_len = 3;

	/* read byte length */
#ifdef SAS_PLUGFEST_CABLE
	i2c_link->i2c_rsp_frame_len = 256;

#else
	i2c_link->i2c_rsp_frame_len = 18;
#endif
}

void i2c_send_frame(core_extension *core, PMV_Request req,
						domain_i2c_link *i2c_link)
{
	MV_U8 i2c_ctl_reg;

	write_i2c_register(core, i2c_link->cable_id, MI2C_DATA,
									i2c_link->i2c_cmd_header[0]);
	i2c_link->i2c_xfer_count = 0;
	i2c_link->i2c_state = I2C_STATE_CMD;
	i2c_link->i2c_request = req;
	i2c_ctl_reg = read_i2c_register(core, i2c_link->cable_id, MI2C_CONTROL);
	((reg_mi2c_control *)&i2c_ctl_reg)->sta = 1;
	write_i2c_register(core, i2c_link->cable_id, MI2C_CONTROL, i2c_ctl_reg);
}

void make_i2c_request(core_extension *core)
{
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	PMV_Request req = get_intl_req_resource(core->roots, 256);
	core_context *ctx = NULL;
	MV_U32 main_irq, time_out;

	if (req == NULL) {
		CORE_DPRINT(("ERROR: No more free internal requests. Aborted.\n"));
		return;
	}

	/* yuxl: change the context type for multi-queue wait_lock & cmpl_lock, make i2c req to internal */
	ctx = (core_context *)req->Context[MODULE_CORE];
	ctx->req_type = CORE_REQ_TYPE_INIT;

	/* Prepare identify  */
	req->Cdb[0] = SLAVE_ADDR_WRITE;
	req->Device_Id = 0;	//TDB
	req->Time_Out = 5;
	req->Completion = NULL;

	/* build read cable command frame */
	i2c_read_cable_mode_command(core);

	/* Enable I2C data transfer */
	i2c_send_frame(core, req, i2c_link);
#ifdef SAS_PLUGFEST_CABLE
	time_out = 1000*req->Time_Out;	// 100ms timeout // add more time to init.
#else
	time_out = 100;	// 100ms timeout // add more time to init.
#endif
	while (time_out > 0) {
		time_out--;
		core_sleep_millisecond(core, 1);
		main_irq = MV_REG_READ_DWORD(core->mmio_base,
										CPU_MAIN_INT_CAUSE_REG);
		if (main_irq & INT_MAP_TWSI) {
			i2c_cable_isr(core);
		}
		if (i2c_link->cable_status == CABLE_STATE_FINISH) {
			break;
		}
	}
	if(time_out == 0){
#ifdef SAS_PLUGFEST_CABLE
		CORE_EH_PRINT(("I2C cable id%d i2c req %p timeout(%d ms)\n", i2c_link->cable_id ,i2c_link->i2c_request, req->Time_Out*1000));
#else
		CORE_EH_PRINT(("I2C cable id%d i2c req %p timeout(100ms)\n", i2c_link->cable_id ,i2c_link->i2c_request));
#endif
		if(i2c_link->i2c_request){
			req= (PMV_Request) i2c_link->i2c_request;
			i2c_link->i2c_request = NULL;
			req->Scsi_Status = REQ_STATUS_ERROR;
			i2c_check_cable_callback(core, req);
			core_return_finished_req(core, req);
		}
	}
	MV_ASSERT(i2c_link->i2c_request == NULL);
	i2c_link->cable_status = CABLE_STATE_I2C_MODE;
}

void cable_init(MV_PVOID core_p, MV_U8 cable_id)
{
	core_extension *core = (core_extension *)core_p;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	pl_root *root;
	MV_U32 reg_value, i, j;
	MV_U8 regI2c, port = 0;

	i2c_link->cable_status = CABLE_STATE_INIT;
	i2c_link->cable_id = cable_id;

	/* Disable i2c */
	for (port = 0; port < 5; port++) {
		/* Reset the I2C logic */
		write_i2c_register(core, port, MI2C_SOFT_RESET, 0);

		/* wait for 1m sec */
		HBA_SleepMillisecond(core, 1);

		write_i2c_register(core, port, MI2C_CONTROL, 0);
	}

	/* Set signal pin to i2c mode */
	reg_value = MV_REG_READ_DWORD(core->mmio_base, I2C_SGPIO_SLCT);
	reg_value |= (1U << 9);
	MV_REG_WRITE_DWORD(core->mmio_base, I2C_SGPIO_SLCT, reg_value);

	/* Init I2C Control Reg */
	write_i2c_register(core, i2c_link->cable_id, MI2C_SOFT_RESET, 0);

	regI2c = 0;
	((reg_mi2c_control *)&regI2c)->enab = 1;
	((reg_mi2c_control *)&regI2c)->ien = 1;
	((reg_mi2c_control *)&regI2c)->aak = 1;
	write_i2c_register(core, i2c_link->cable_id, MI2C_CONTROL, regI2c);

	/* Build command and send to cable */
	make_i2c_request(core);

	/* Enable GPIOx Interrupt */
	reg_value = MV_REG_READ_DWORD(core->mmio_base, GPIO_INT_EN_REG);
	reg_value |= CABLE_GPIO_BITS;
	if(core->device_id == 0x1485)	//1485 is one root, so just enable bit1 and bit3
		reg_value &= 0x000f;
	MV_REG_WRITE_DWORD(core->mmio_base, GPIO_INT_EN_REG, reg_value);
	MV_REG_WRITE_DWORD(core->mmio_base, GPIO_PIN_IN_EN_REG, reg_value);
	MV_REG_WRITE_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG, 0xFF);
}

void i2c_handle_hotplug_cable(MV_PVOID core_p, MV_U8 cable_id)
{
	core_extension *core = (core_extension *)core_p;
	domain_i2c_link *i2c_link = &core->lib_i2c.i2c_link;
	MV_U32 reg;
	MV_U8 regI2c;

	/* Delay for cable ready */
	core_sleep_millisecond(core, 50);

#ifdef SUPPORT_I2C
	/* Disable I2C INT casue*/
	reg = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG);
	reg &= ~INT_MAP_TWSI;
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, reg);
#endif

	i2c_link->cable_status = CABLE_STATE_INIT;
	i2c_link->cable_id = cable_id;

	/* Init I2C Control Reg */
	write_i2c_register(core, i2c_link->cable_id, MI2C_SOFT_RESET, 0);

	regI2c = 0;
	((reg_mi2c_control *)&regI2c)->enab = 1;
	((reg_mi2c_control *)&regI2c)->ien = 1;
	((reg_mi2c_control *)&regI2c)->aak = 1;
	write_i2c_register(core, i2c_link->cable_id, MI2C_CONTROL, regI2c);

	make_i2c_request(core);

#ifdef SUPPORT_I2C
	/* Enable I2C INT casue*/
	reg = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG);
	reg |= INT_MAP_TWSI;
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, reg);
#endif
}

void core_handle_cable_gpio_int(MV_PVOID core_p, MV_U32 reg_val)
{
	core_extension *core = (core_extension *)core_p;
	pl_root *root;
	domain_phy *phy;
	MV_U8 i, cable_id, start_phy_id;

	if (reg_val & MV_BIT(1)) {
		/* GPIO INT BIT1 related to root 0 phy 0-3, use I2C 1 to control */
		cable_id = 1;
		start_phy_id = 0;
		root = &core->roots[0];
	} else if (reg_val & MV_BIT(3)) {
		/* GPIO INT BIT3 related to root 0 phy 4-7, use I2C 2 to control */
		cable_id = 2;
		start_phy_id = 4;
		root = &core->roots[0];
	} else if (reg_val & MV_BIT(5)) {
		/* GPIO INT BIT5 related to root 1 phy 0-3, use I2C 3 to control */
		cable_id = 3;
		start_phy_id = 0;
		root = &core->roots[1];
	} else if (reg_val & MV_BIT(7)) {
		/* GPIO INT BIT7 related to root 1 phy 4-7, use I2C 4 to control */
		cable_id = 4;
		start_phy_id = 4;
		root = &core->roots[1];
	} else {
		return;
	}

	/* Set cable port's flags */
	for (i = 0; i < 4; i++) {
		root->phy[start_phy_id + i].cable_id = cable_id;
		root->phy[start_phy_id + i].is_cable_hotplug = MV_TRUE;
	}

	/* For cable hotplug, we use the first phy of the port to handle the event */
	pal_notify_event(root, start_phy_id, PL_EVENT_CABLE_HOTPLUG);
}

#endif

