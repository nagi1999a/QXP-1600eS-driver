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
#include "core_api.h"
#include "core_util.h"
#include "hba_inter.h"
#ifdef SUPPORT_BBU
#include "core_bbu.h"
#endif
MV_U8 core_get_battery_status(MV_PVOID this)
{
    MV_U8 bbu_status = BBU_NORMAL;

#if defined(SUPPORT_BBU)
    PHBA_Extension hba_ptr = (PHBA_Extension)HBA_GetModuleExtension(this, MODULE_HBA);
    if (hba_ptr->bbu.status == BBU_STATUS_NOT_PRESENT)		/* BBU_STATUS_NOT_PRESENT equals to 0 */
        bbu_status = BBU_NOT_PRESENT;

    if (hba_ptr->bbu.status & 
        (BBU_STATUS_LOW_BATTERY | BBU_STATUS_OVER_TEMP_ERROR |BBU_STATUS_UNDER_TEMP_ERROR
        | BBU_STATUS_OVER_VOLT_ERROR | BBU_STATUS_UNDER_VOLT_ERROR)
        )
        bbu_status = BBU_ABNORMAL;
    
#else
    bbu_status = BBU_NOT_PRESENT;
#endif
    return bbu_status;
}

MV_U8
mv_get_bbu_info(MV_PVOID core, PMV_Request pReq)
{
#if defined(SUPPORT_BBU)
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
    PBBU_Info bbu_ptr;
    /* initialize */
    bbu_ptr = (PBBU_Info) pReq->Data_Buffer;
    MV_ZeroMemory(bbu_ptr, sizeof(BBU_Info));

    MV_CopyMemory(bbu_ptr, &(hba_ptr->bbu), sizeof(BBU_Info));

    pReq->Scsi_Status = REQ_STATUS_SUCCESS;
#else
    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
#endif
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}


MV_U8
mv_set_bbu_threshold(MV_PVOID core, PMV_Request pReq)
{
#if defined(SUPPORT_BBU)
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
    HBA_Info_Page		hba_info_param;
    MV_U8 type = pReq->Cdb[2];
    MV_U16 lowerbound = (pReq->Cdb[3] << 8) | (pReq->Cdb[4]);
    MV_U16 upperbound = (pReq->Cdb[5] << 8) | (pReq->Cdb[6]);

    if (hba_ptr->bbu.status == BBU_STATUS_NOT_PRESENT) {
        if (pReq->Sense_Info_Buffer != NULL)
            ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
        pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
    } else {

    //    MV_DPRINT(("JL %s type %d, ub %d, lb %d\n", __FUNCTION__, type, upperbound, lowerbound));
        pReq->Scsi_Status = REQ_STATUS_SUCCESS;
        switch (type)
        {
            case BBU_THRESHOLD_TYPE_CAPACITY:
                if (hba_ptr->bbu.percent_to_charge != lowerbound) {
                    //Check if user's value is correct.
                    if ((lowerbound > BBU_CAPACITY_EMPTY) && (lowerbound < BBU_CAPACITY_FULL)) {
                        //Set percent_to_charge as user set
                        hba_ptr->bbu.percent_to_charge = lowerbound;
                        mv_nvram_init_param(hba_ptr, &hba_info_param);
                        hba_info_param.bbu_charge_threshold = hba_ptr->bbu.percent_to_charge;
                        mvuiHBA_modify_param(hba_ptr, &hba_info_param);
                    } else {
                        MV_PRINT("set BBU_THRESHOLD_TYPE_CAPACITY parameter wrong. upper %d, lower %d\n", upperbound, lowerbound);
                        if (pReq->Sense_Info_Buffer != NULL)
                            ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_INVALID_PARAMETER;
                        pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                    }
                }
                break;
                
            case BBU_THRESHOLD_TYPE_TEMPERATURE:
                if ((hba_ptr->bbu.temp_lowerbound != lowerbound) ||
                    (hba_ptr->bbu.temp_upperbound != upperbound)) {
                    if ((lowerbound >= BBU_TEMP_LOWER_BOUND) && (upperbound <= BBU_TEMP_UPPER_BOUND)) {
                        hba_ptr->bbu.temp_lowerbound = lowerbound;
                        hba_ptr->bbu.temp_upperbound = upperbound;
                       
                        mv_nvram_init_param(hba_ptr, &hba_info_param);
                        hba_info_param.bbu_temp_lowerbound = hba_ptr->bbu.temp_lowerbound;
                        hba_info_param.bbu_temp_upperbound = hba_ptr->bbu.temp_upperbound;
                        mvuiHBA_modify_param(hba_ptr, &hba_info_param);
                    } else {
                        MV_PRINT("set BBU_THRESHOLD_TYPE_TEMPERATURE parameter wrong. upper %d, lower %d\n", upperbound, lowerbound);
                        if (pReq->Sense_Info_Buffer != NULL)
                            ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_INVALID_PARAMETER;
                        pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                    }
                }
                break;

            case BBU_THRESHOLD_TYPE_VOLTAGE:
                if ((hba_ptr->bbu.volt_lowerbound != lowerbound) ||
                    (hba_ptr->bbu.volt_upperbound != upperbound)) {
                    if ((lowerbound >= BBU_VOLT_LOWER_BOUND) && (upperbound <= BBU_VOLT_UPPER_BOUND)) {
                        hba_ptr->bbu.volt_lowerbound = lowerbound;
                        hba_ptr->bbu.volt_upperbound = upperbound;

                        mv_nvram_init_param(hba_ptr, &hba_info_param);
                        hba_info_param.bbu_volt_lowerbound = hba_ptr->bbu.volt_lowerbound;
                        hba_info_param.bbu_volt_upperbound = hba_ptr->bbu.volt_upperbound;
                        mvuiHBA_modify_param(hba_ptr, &hba_info_param);
                    } else {
                        MV_PRINT("set BBU_THRESHOLD_TYPE_VOLTAGE parameter wrong. upper %d, lower %d\n", upperbound, lowerbound);
                        if (pReq->Sense_Info_Buffer != NULL)
                            ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_INVALID_PARAMETER;
                        pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                    }
                }
                break;
            default:
                MV_PRINT("mv_set_bbu_threshold type error, CDB %x %x %x %x %x %x %x\n",
                    pReq->Cdb[0], pReq->Cdb[1], pReq->Cdb[2], pReq->Cdb[3], pReq->Cdb[4], pReq->Cdb[5], pReq->Cdb[6]);
                break;
        }
    }
#else
    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
#endif
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
mv_bbu_power_change(MV_PVOID core, PMV_Request pReq)
{
#if defined(SUPPORT_BBU)
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
    MV_U8 type = pReq->Cdb[2];

    if (hba_ptr->bbu.status == BBU_STATUS_NOT_PRESENT) {
        if (pReq->Sense_Info_Buffer != NULL)
            ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
        pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
    } else {

        pReq->Scsi_Status = REQ_STATUS_SUCCESS;

        switch (type) {
            case BBU_ACT_RELEARN:
            case BBU_ACT_FORCE_DISCHARGE:
                bbu_enable_discharger(core);
                hba_ptr->bbu.status &= ~(BBU_STATUS_CHARGING | BBU_STATUS_FULL_CHARGED | BBU_STATUS_POWER_STOP_ALL);
                hba_ptr->bbu.status |= BBU_STATUS_DISCHARGE;

            #ifdef CACHE_MODULE_SUPPORT
                core_notify_battery((MV_PVOID)core, MV_FALSE);
                core_generate_event(core, EVT_ID_BAT_FORCE_WRITE_THROUGH, 0, SEVERITY_WARNING,  0,  NULL, 0 );
            #endif
                core_generate_event(core, EVT_ID_BAT_DISCHARGING, 0, SEVERITY_WARNING,  0,  NULL, 0 );
                
                break;
            case BBU_ACT_FORCE_CHARGE:
                bbu_enable_charger(core);
                hba_ptr->bbu.status &= ~(BBU_STATUS_DISCHARGE | BBU_STATUS_FULL_CHARGED | BBU_STATUS_POWER_STOP_ALL);
                hba_ptr->bbu.status |= BBU_STATUS_CHARGING;
                break;
            case BBU_ACT_STOP_ALL:
                bbu_disable_charger(core);
                bbu_disable_discharger(core);
                hba_ptr->bbu.status &= ~(BBU_STATUS_CHARGING | BBU_STATUS_DISCHARGE | BBU_STATUS_FULL_CHARGED);
                hba_ptr->bbu.status |= BBU_STATUS_POWER_STOP_ALL;
                break;
        }
    }

#else
    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
#endif
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

