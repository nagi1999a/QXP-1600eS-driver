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

#if !defined(CORE_CONSOLE_H)
#define CORE_CONSOLE_H

#include "mv_config.h"
#include "core_header.h"

typedef MV_U8 (*core_management_command_handler)(core_extension *, PMV_Request);
#define HD_WRITECACHE_OFF		0
#define HD_WRITECACHE_ON		1
MV_U8 api_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req);

MV_U8
core_pd_command(
	IN MV_PVOID root_p,
	IN PMV_Request pReq
	);

#ifdef SUPPORT_PASS_THROUGH_DIRECT
MV_U8
core_pass_thru_send_command(
	IN MV_PVOID *core_p,
	IN PMV_Request pReq
	);
#endif

MV_U8 core_ses_command(
	IN MV_PVOID core_p,
	IN PMV_Request req
	);
#ifdef SUPPORT_FLASH
MV_BOOLEAN
core_flash_command(
	IN MV_PVOID core_p,
	IN PMV_Request p_req
	);
#endif

MV_U8 core_api_inquiry(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_read_capacity(core_extension * core_p, PMV_Request req);
MV_U8 core_api_read_capacity_16(core_extension * core_p, PMV_Request req);
MV_U8 core_api_report_lun(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_disk_io_control(MV_PVOID core_p, PMV_Request req);
#ifdef SUPPORT_BOARD_ALARM
MV_U8 core_api_alarm_command(MV_PVOID core_p, PMV_Request req);
#endif


MV_BOOLEAN core_dbg_command(
	IN MV_PVOID extension,
	IN PMV_Request req
	);


MV_U8
mv_get_bbu_info(MV_PVOID core, PMV_Request pReq);
MV_U8
mv_set_bbu_threshold(MV_PVOID core, PMV_Request pReq);
MV_U8
mv_bbu_power_change(MV_PVOID core, PMV_Request pReq);

MV_U8 core_pd_request_get_hd_info(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_enclosure_info(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_expander_info(core_extension * core_p, PMV_Request req);
MV_BOOLEAN core_pd_request_get_hd_config(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_set_hd_config(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_hd_status(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_hd_info_ext(core_extension * core_p, PMV_Request req);
#endif /* CORE_CONSOLE_H */
