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

#ifndef __CORE_SAS_H
#define __CORE_SAS_H

#include "mv_config.h"
#include "core_type.h"

enum sas_link_rate {
	SAS_LINK_RATE_UNKNOWN = 0,
	SAS_PHY_DISABLED = 1,
	SAS_PHY_RESET_PROBLEM = 2,
	SAS_SATA_SPINUP_HOLD = 3,
	SAS_SATA_PORT_SELECTOR = 4,
	SAS_PHY_RESET_IN_PROGRESS = 5,
	SAS_LINK_RATE_1_5_GBPS = 8,
	SAS_LINK_RATE_G1 = SAS_LINK_RATE_1_5_GBPS,
	SAS_LINK_RATE_3_0_GBPS = 9,
	SAS_LINK_RATE_G2 = SAS_LINK_RATE_3_0_GBPS,
	SAS_LINK_RATE_6_0_GBPS = 10,
	SAS_LINK_RATE_G3 = SAS_LINK_RATE_6_0_GBPS,
	SAS_LINK_RATE_12_0_GBPS = 11,
};

/*
 * Task Management Functions.
 */
enum TMF {
	TMF_ABORT_TASK = 0x01,
	TMF_ABORT_TASK_SET = 0x02,
	TMF_CLEAR_TASK_SET = 0x04,
	TMF_LOGICAL_UNIT_RESET = 0x08,
	TMF_CLEAR_ACA = 0x40,
	TMF_QUERY_TASK = 0x80,
};

/* fixed format sense data from SCSI */
typedef struct _sense_data
{
	MV_U8 response_code : 7; /* 0x70h or 0x71h */
	MV_U8 valid : 1;
	MV_U8 obsolete;
	MV_U8 sense_key : 4;
	MV_U8 reserved : 1;
	MV_U8 incorrect_length_indicator : 1;
	MV_U8 end_of_media : 1;
	MV_U8 file_mark : 1;
	MV_U8 information[4];
	MV_U8 additional_sense_length;

	MV_U8 command_specific_information[4];
	MV_U8 additional_sense_code;
	MV_U8 additional_sense_code_qualifier;
	MV_U8 field_replaceable_unit_code;
	MV_U8 sense_key_specific[3];
	MV_U8 additional_sense_bytes[2]; /* change from 0 to 2, make it aligned */
} sense_data;

#define IS_A_TSK_REQ(req) \
	((req->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC)&& \
	 (req->Cdb[1]==CDB_CORE_MODULE)&& \
	 (req->Cdb[2]==CDB_CORE_TASK_MGMT))

#ifdef SAS_12G_SSD_QUEUE_DEPTH_WA
#define CORE_SAS_DISK_QUEUE_DEPTH 63
#else
#define CORE_SAS_DISK_QUEUE_DEPTH 64
#endif

#define IS_BEHIND_EXP(dev) \
	(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER)
#define SAS_SLOW_SPINUP(dev) IS_FUJITSU(dev)
#define IS_FUJITSU(dev) \
	(MV_CompareMemory(dev->model_number, "FUJITSU", sizeof("FUJITSU") - 1)\
	== 0)

MV_U8 ssp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req);
MV_VOID ssp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req);
MV_VOID ssp_send_command(
            MV_PVOID root_p,
            MV_PVOID dev_p,
            MV_PVOID struct_wrapper,
            MV_Request *req);
MV_VOID ssp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req);

MV_U8 smp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req);
MV_VOID smp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req);
MV_VOID smp_send_command(
            MV_PVOID root_p,
            MV_PVOID dev_p,
            MV_PVOID struct_wrapper,
            MV_Request *req);
MV_VOID smp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req);

MV_VOID stp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req);
MV_VOID stp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req);

MV_BOOLEAN sas_init_state_machine(MV_PVOID dev);

MV_Request *sas_make_marvell_specific_req(MV_PVOID dev_p,
        MV_U8 cmd, MV_ReqCompletion cmpltn);
MV_Request *sas_make_mode_sense_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn);
#ifdef SUPPORT_PHY_POWER_MODE
MV_Request *sas_make_powermode_mode_sense_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn);
#endif
MV_Request *sas_make_log_sense_req(MV_PVOID dev_p,
        MV_U8 page, MV_ReqCompletion cmpltn);
MV_Request *sas_make_sync_cache_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn);
MV_VOID sas_replace_org_req(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *new_req);
MV_Request *sas_get_org_req(MV_Request *req);
MV_Request *sas_clear_org_req(MV_Request *req);
MV_U8 ssp_ata_parse_log_sense_threshold_exceed(MV_PU8 data_buf, MV_U32 length);
MV_Request *sas_make_inquiry_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_BOOLEAN EVPD, MV_U8 page, MV_ReqCompletion completion);
MV_Request *ssp_make_virtual_phy_reset_req(MV_PVOID dev_p,
	MV_U8 operation, MV_ReqCompletion callback);
#ifdef SUPPORT_MUL_LUN
MV_Request *sas_make_report_lun_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_ReqCompletion completion);
void mv_int_to_reqlun(MV_U16 lun, MV_U8*reqlun);
#endif
void sas_check_wide_port_device(MV_PVOID port_p);

#endif /* __CORE_SAS_H */
