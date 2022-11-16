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
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_console.h"
#include "core_util.h"
#include "core_api.h"
#ifdef SUPPORT_SGPIO
#include "core_gpio.h"
#endif
#ifdef SUPPORT_SGPIO
MV_U8 core_sgpio_set_led( MV_PVOID extension, MV_U16 device_id, MV_U8 light_type,
        MV_U8 light_behavior, MV_U8 flag)
{
	core_extension *core = (core_extension *)extension;
	domain_device * device;
	domain_base * base;
	pl_root * root;
	MV_LPVOID mmio = core->mmio_base;
	MV_U16 index = device_id;//MapDeviceId(pCore, device_id);
	MV_U32 tmp = 0x0;
	MV_U8 offset = 0, mask = 0, i = 0, sgpio_numb;
	MV_BOOLEAN set_status = MV_FALSE;

	device = (domain_device *)get_device_by_id(&core->lib_dev, index, MV_FALSE, MV_FALSE);
	if (device == NULL) return ERR_INVALID_HD_ID;
	base = &device->base;
	for (i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		root = &core->roots[i];
		if (base->root == root) break;
	}
	if (i == core->chip_info->n_host)
		return ERR_INVALID_HD_ID;
	sgpio_numb = device->sgpio_drive_number;
    LED_DBG_PRINT(("JL %s id %x, light_type %x, light_behavior %x, flag %x, sgpio_num %x\n", __FUNCTION__, device_id, light_type, light_behavior, flag, sgpio_numb));
	if ((core->device_id == DEVICE_ID_6480) || (core->device_id == DEVICE_ID_6485)) {
		if (sgpio_numb>=4) {
			i = 1;
			sgpio_numb -= 4;
		}
	}
	// if flag is valid, we ignore light_type and light_behavior; and vice versa
	switch (flag) {
	case LED_FLAG_REBUILD:
		light_type = SGPIO_LED_REBUILD;
		light_behavior = SGPIO_LED_HIGH;
		break;
	case LED_FLAG_HOT_SPARE:
		light_type = SGPIO_LED_LOCATE;
		light_behavior = SGPIO_LED_BLINK_A;
		break;
	case LED_FLAG_FAIL_DRIVE:
		light_type = SGPIO_LED_ERROR;
		light_behavior = SGPIO_LED_HIGH;
		break;
	case LED_FLAG_ACT:
		light_type = SGPIO_LED_ACTIVITY;
		light_behavior = SGPIO_LED_HIGH;
		break;
	case LED_FLAG_HOT_SPARE_OFF:
	case LED_FLAG_OFF_ALL:
	case LED_FLAG_REBUILD_OFF:
		light_type = SGPIO_LED_ALL;
		light_behavior = SGPIO_LED_LOW;
		break;
	}

/* at the request of MRU/API: only set pDevice->SGPIO_Locate_LED_Status
   for API requests, and if the light is about to be turned off */
	if (flag == 0 || flag == LED_FLAG_OFF_ALL)
		set_status = MV_TRUE;

	if (!(IS_SGPIO(device))) return ERR_INVALID_HD_ID;

#if !defined(SUPPORT_ROC)
	switch (core->device_id) {
	case DEVICE_ID_6440:
	case DEVICE_ID_6445:
	case DEVICE_ID_6320:
	case DEVICE_ID_6340:
		tmp = sgpio_read_pci_register(core, SGPIO_Data_Out_L);
		break;
	case DEVICE_ID_6480:
	case DEVICE_ID_6485:
	case DEVICE_ID_8180:
	case DEVICE_ID_9480:
	case DEVICE_ID_9580:
	case DEVICE_ID_9548:
	case DEVICE_ID_9588:
	case DEVICE_ID_9485:
	case DEVICE_ID_9440:
	case DEVICE_ID_9445:
	case DEVICE_ID_948F:
	case DEVICE_ID_ATHENA_FPGA:
	case DEVICE_ID_ATHENA_1480:
	case DEVICE_ID_ATHENA_1475:
	case DEVICE_ID_ATHENA_1485:
	case DEVICE_ID_ATHENA_1495:
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE+sgpio_numb/4*4),
			tmp);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
#else
	mv_sgpio_read_register(mmio,
	SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE+sgpio_numb/4*4),
	tmp);
#endif

        /* NOTE: Supermicro backplanes have the following modes for SGPIO:
        *
        *		Locate(fast blink, 4 Hz) : Locate light = 1, error light = 0
        *		Error(high)			     : Locate light = 0, error light = 1
        *		Rebuild(slow blink, 1 Hz): Locate light = 1, error light = 1
        *
        *   Therefore, when we want to change to any of these modes, we must modify
        *   BOTH locate and error lights.
        */
#if !defined(SUPPORT_ROC)
	if ((core->device_id == DEVICE_ID_6440) || (core->device_id == DEVICE_ID_6445)
                        ||(core->device_id == 0x6320) ||(core->device_id == DEVICE_ID_6340)) {
		if (light_behavior > 1)
			light_behavior = 1;

		switch(light_type){
		case SGPIO_LED_ACTIVITY:
			offset = 0;
			mask = 0x1;
			device->sgpio_act_led_status = light_behavior;
			break;
		case SGPIO_LED_LOCATE:
			offset = 1;
			mask = 0x3;
			if (set_status)
				device->sgpio_locate_led_status = light_behavior;
			break;
		case SGPIO_LED_ERROR:
			offset = 1;
			mask = 0x3;
			device->sgpio_error_led_status = light_behavior;
			break;
		case SGPIO_LED_REBUILD:
			offset = 1;
			mask = 0x3;
			device->sgpio_error_led_status = light_behavior;
			if (set_status)
				device->sgpio_locate_led_status = light_behavior;
			light_behavior = 0x3;
			break;
		case SGPIO_LED_ALL:
			offset = 0;
			mask = 0x7;
			device->sgpio_act_led_status = SGPIO_LED_LOW;
			device->sgpio_locate_led_status = SGPIO_LED_LOW;
			device->sgpio_error_led_status = SGPIO_LED_LOW;
			break;
		}
	} else
#endif /*SUPPORT_ROC*/
	{
		switch (light_type) {
		case SGPIO_LED_ACTIVITY:
			offset = DRV_ACTV_LED_OFFSET;
			mask = DRV_ACTV_LED_MASK;
			device->sgpio_act_led_status = light_behavior;
			break;
		case SGPIO_LED_LOCATE:
			offset = DRV_ERR_LED_OFFSET;
			mask = DRV_LOC_LED_MASK | DRV_ERR_LED_MASK;
			if (set_status)
				device->sgpio_locate_led_status = light_behavior;
			light_behavior = light_behavior << 3;
			break;
		case SGPIO_LED_ERROR:
			offset = DRV_ERR_LED_OFFSET;
			mask = DRV_LOC_LED_MASK | DRV_ERR_LED_MASK;
			device->sgpio_error_led_status = light_behavior;
			break;
		case SGPIO_LED_REBUILD:
			offset = DRV_ERR_LED_OFFSET;
			mask = DRV_LOC_LED_MASK | DRV_ERR_LED_MASK;
			device->sgpio_error_led_status = light_behavior;
			if (set_status)
				device->sgpio_locate_led_status = light_behavior;
			light_behavior = 0x9; /* set both locate and error to 1 (high) */
			break;
		case SGPIO_LED_ALL:
			offset = 0;
			mask = 0xff;
			device->sgpio_act_led_status = SGPIO_LED_LOW;
			device->sgpio_locate_led_status = SGPIO_LED_LOW;
			device->sgpio_error_led_status = SGPIO_LED_LOW;
			break;
		}
	}

#if !defined(SUPPORT_ROC)
	switch(core->device_id) {
	case DEVICE_ID_6440:
	case DEVICE_ID_6445:
	case DEVICE_ID_6320:
	case DEVICE_ID_6340:
		tmp &= ~(mask<<(device->sgpio_drive_number*3+offset));
		tmp |= light_behavior<<(device->sgpio_drive_number*3+offset);
		//sgpio_sendsgpioframe(core, tmp);	//temp disable it, it reduce performance
		break;
	case DEVICE_ID_6480:
	case DEVICE_ID_6485:
	case DEVICE_ID_8180:
	case DEVICE_ID_9480:
	case DEVICE_ID_9580:
	case DEVICE_ID_9548:
	case DEVICE_ID_9588:
	case DEVICE_ID_9485:
	case DEVICE_ID_9440:
	case DEVICE_ID_9445:
	case DEVICE_ID_948F:
	case DEVICE_ID_ATHENA_FPGA:
	case DEVICE_ID_ATHENA_1480:
	case DEVICE_ID_ATHENA_1475:
	case DEVICE_ID_ATHENA_1485:
	case DEVICE_ID_ATHENA_1495:
		tmp &= ~(mask<<(3-sgpio_numb)*8);
		tmp |= light_behavior<<(offset+(3-sgpio_numb)*8);
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE+sgpio_numb/4*4),
			tmp);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

#else /*SUPPORT_ROC*/
        tmp &= ~(mask<<(3-sgpio_numb)*8);
        tmp |= light_behavior<<(offset+(3-sgpio_numb)*8);
        LED_DBG_PRINT(("JL %s tmp %x, light_behavior %x, offset %x, mask %x\n", __FUNCTION__, tmp, light_behavior, offset, mask));
        mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(i, SGPIO_REG_DRV_CTRL_BASE+sgpio_numb/4*4),
	tmp);

#endif /*SUPPORT_ROC*/
	return REQ_STATUS_SUCCESS;
}
MV_VOID core_sgpio_control_active_led(MV_PVOID core_p, domain_base * base)
{
	core_extension *core = (core_extension *)core_p;
	domain_device *device;

		device = (domain_device *)base;
		if(device==NULL)
			return;
		if((device->base.type != BASE_TYPE_DOMAIN_DEVICE)||(!IS_SGPIO(device)))
			return;
		if(((device->base.outstanding_req > 0)&&(device->sgpio_act_led_status == SGPIO_LED_HIGH))
			||((device->base.outstanding_req == 0)&&(device->sgpio_act_led_status == SGPIO_LED_LOW)))
			return;
		if((device->base.outstanding_req > 0)&&(device->sgpio_act_led_status == SGPIO_LED_LOW)){
			core_sgpio_set_led(core,base->id,0,0,LED_FLAG_ACT);
			LED_DBG_PRINT(("JL device %x outstanding req %x, sgpio_act_led_status: %x turn on\n",base->id, base->outstanding_req, device->sgpio_act_led_status));
			CORE_DPRINT(("device %x outstanding req %x, sgpio_act_led_status: %x turn on\n",base->id, base->outstanding_req, device->sgpio_act_led_status));
		}
		else if((device->base.outstanding_req == 0)&&(device->sgpio_act_led_status == SGPIO_LED_HIGH)){
			core_sgpio_set_led(core,base->id,0,0,LED_FLAG_OFF_ALL);
			LED_DBG_PRINT(("JL device %x outstanding req %x, sgpio_act_led_status: %x turn off\n",base->id, base->outstanding_req, device->sgpio_act_led_status));
			CORE_DPRINT(("device %x outstanding req %x, sgpio_act_led_status: %x turn off\n",base->id, base->outstanding_req, device->sgpio_act_led_status));
		}

}
#ifdef SUPPORT_SGPIO_ACTIVE_LED
MV_VOID core_sgpio_led_off_timeout(domain_base * base, MV_PVOID tmp)
{
	domain_device *device;
	if(base == NULL)
		return;
	if(base->type != BASE_TYPE_DOMAIN_DEVICE)
		return;
	device = (domain_device *)base;
	if(!IS_SGPIO(device))
		return;
	if (device->active_led_off_timer != NO_CURRENT_TIMER) {
		core_cancel_timer(base->root->core, device->active_led_off_timer);
		device->active_led_off_timer = NO_CURRENT_TIMER;
		core_sgpio_set_led(base->root->core,base->id,0,0,LED_FLAG_OFF_ALL);
        LED_DBG_PRINT(("JL device %x outstanding req %x, sgpio_act_led_status: %x turn off\n",base->id, base->outstanding_req, device->sgpio_act_led_status));
	} else {
		CORE_DPRINT(("warning: LED timer is abnormal.\n"));
	}
	//CORE_DPRINT(("device %x outstanding req %x, sgpio_act_led_status: %x turn off\n",base->id, base->outstanding_req, device->sgpio_act_led_status));

}
#endif
#endif

/*TBD: after support SES*/
#ifdef SUPPORT_SES
MV_Request *ses_make_receive_diagnostic_request(domain_enclosure *enc,MV_U8 page_code,
	MV_ReqCompletion completion);
void ses_get_status_callback(MV_PVOID root_p, PMV_Request req);
#define	ses_make_enclosure_status_request(a)		(ses_make_receive_diagnostic_request(a,SES_PG_ENCLOSURE_STATUS,ses_get_status_callback))

MV_BOOLEAN ses_check_supported_page(domain_enclosure *enc, MV_U8 page_code);
void core_expander_set_led(
	IN core_extension *core,
	IN domain_device *device,
	IN MV_U8 flag
	)
{
	MV_Request *req = NULL;
	if (device->enclosure== NULL)
		return;
	device->ses_request_flag = flag;
	req = ses_make_enclosure_status_request(device->enclosure);
	if (req == NULL) {
		CORE_PRINT(("error: no internal request resource\n"));
		return;
	}
	core_append_request(device->enclosure->base.root, req);
}
#endif /* SUPPORT_SES */

void Core_Change_LED(
	IN MV_PVOID	extension,
	IN MV_U16 device_id,
	IN MV_U8 flag)
{
	core_extension * core = (core_extension *)extension;
	domain_device *device = NULL;
	domain_base *base = NULL;

	base = get_device_by_id(&core->lib_dev, device_id, MV_FALSE, MV_FALSE);
	if ((base == NULL) || (base->type != BASE_TYPE_DOMAIN_DEVICE))
		return;
	else
		device = (domain_device *)base;

#ifdef SUPPORT_SGPIO
	if (IS_SGPIO(device)) {
		if (flag != LED_FLAG_FAIL_VD) {
		core_sgpio_set_led(core, device_id, 0, 0, flag);
		return;
		}
	} else {
#ifdef SUPPORT_SES
		if (flag == LED_FLAG_FAIL_VD) {
			flag = LED_FLAG_FAIL_DRIVE;
		}
		core_expander_set_led(core, device, flag);
#endif
	}
#else
#ifdef SUPPORT_SES
	core_expander_set_led(core, device, flag);
#endif
#endif
	//CORE_DPRINT(("no sgpio or expander exist\n"));
}

MV_VOID
core_send_ses_command_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req = req->Org_Req;
	domain_enclosure *enc = (domain_enclosure *)get_device_by_id(root->lib_dev,
		req->Device_Id, MV_FALSE, MV_FALSE);
	MV_PVOID buf_ptr, new_buf_ptr;

	org_req->Scsi_Status = req->Scsi_Status;
	if ((req->Scsi_Status != SCSI_STATUS_GOOD)&& (req->Sense_Info_Buffer))
		MV_CopyMemory(org_req->Sense_Info_Buffer, req->Sense_Info_Buffer, org_req->Sense_Info_Buffer_Length);
	if (req->Scsi_Status == SCSI_STATUS_GOOD) {
		if (req->Cmd_Flag & CMD_FLAG_DATA_IN) {
			buf_ptr = core_map_data_buffer(org_req);
			new_buf_ptr = core_map_data_buffer(req);

			MV_CopyMemory(buf_ptr, new_buf_ptr, req->Data_Transfer_Length);

			core_unmap_data_buffer(org_req);
			core_unmap_data_buffer(req);
		}
		core_queue_completed_req(root->core, org_req);
	}
}

MV_U8
core_send_ses_command(domain_enclosure *enc, PMV_Request req)
{
	pl_root *root = enc->base.root;

	req->Device_Id = enc->base.id;
	req->Cdb[5] = 0;
	/* Send this internal request */
	core_append_request(root, req);
	/* ikkwong:TBD */
	/* We are sending this request down directly without
	 * allocating a new request. I think this should be
	 * passed instead of replaced
	 */
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

MV_U8
core_ses_command(
	IN MV_PVOID core_p,
	IN PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base = NULL;
	domain_enclosure *enc;
	MV_U8 temp;

	base = (domain_base *)get_device_by_id(&core->lib_dev, req->Cdb[5], MV_FALSE, MV_FALSE);
	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	if((base->type == BASE_TYPE_DOMAIN_ENCLOSURE)
#ifdef SUPPORT_I2C
		||(base->type == BASE_TYPE_DOMAIN_I2C)
#endif
		) {
		enc = (domain_enclosure *)base;
#ifdef SUPPORT_SES
		if( !ses_check_supported_page(enc, req->Cdb[2]) )
		{
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
#endif
		return (core_send_ses_command(enc, req));
	}
#ifdef SUPPORT_SGPIO
	else if (IS_SGPIO(((domain_device *)base))){
		temp = core_sgpio_set_led(core, req->Cdb[5], req->Cdb[6], req->Cdb[7], 0);
		if (temp == REQ_STATUS_SUCCESS)
			req->Scsi_Status = REQ_STATUS_SUCCESS;
		else {
			if (req->Sense_Info_Buffer != NULL)
				((MV_PU8)req->Sense_Info_Buffer)[0] = temp;
			req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		}
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
#endif
	else
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}
