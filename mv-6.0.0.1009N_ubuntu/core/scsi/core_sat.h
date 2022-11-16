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

#ifndef __CORE_SAT_H
#define __CORE_SAT_H

#include "mv_config.h"
MV_BOOLEAN sat_categorize_cdb(pl_root *root, MV_Request *req);
MV_U8 scsi_ata_translation(pl_root *root, MV_Request *req);
MV_U8 sat_release_org_req(pl_root *root, MV_Request *req);
MV_BOOLEAN ata_fill_taskfile(domain_device *device, MV_Request *req, 
	MV_U8 tag, ata_taskfile *taskfile);
MV_BOOLEAN atapi_fill_taskfile(domain_device *device, MV_Request *req, 
	ata_taskfile *taskfile);
MV_BOOLEAN passthru_fill_taskfile(MV_Request *req, ata_taskfile *taskfile);
MV_VOID scsi_to_sata_fis(MV_Request *req, MV_PVOID fis_pool, 
	ata_taskfile *taskfile, MV_U8 pm_port);
MV_VOID sat_make_sense(MV_PVOID root_p, MV_Request *req, MV_U8 status,
        MV_U8 error, MV_U64 *lba);

MV_Request *sat_get_org_req(MV_Request *req);
MV_Request *sat_clear_org_req(MV_Request *req);

/**************************************************************/
/* buffer is sizeof mdPage_t */
#define LOG_SENSE_WORK_BUFER_LEN 512
#ifdef _OS_WINDOWS
static MV_U8 log_sense_work_buffer[LOG_SENSE_WORK_BUFER_LEN];
#endif /* _OS_WINDOWS */

/**************************************************************/
/* Format Unit Scsi command translation 
 */
#define LBA_BLOCK_COUNT		256
#define STANDARD_INQUIRY_LENGTH                 0x60
#define ATA_RETURN_DESCRIPTOR_LENGTH            14
#define MV_MAX_INTL_BUFFER_SIZE                 768

enum _ATA_SEND_DIAGNOSTIC_STATE {
    SEND_DIAGNOSTIC_START = 0,
    SEND_DIAGNOSTIC_SMART,
    SEND_DIAGNOSTIC_VERIFY_0,
    SEND_DIAGNOSTIC_VERIFY_MID,
    SEND_DIAGNOSTIC_VERIFY_MAX,
    SEND_DIAGNOSTIC_FINISHED
};

enum _ATA_START_STOP_STATE {
    START_STOP_START = 0,
    START_STOP_ACTIVE,
    START_STOP_IDLE_IMMEDIATE_SYNC,
    START_STOP_IDLE_IMMEDIATE,
    START_STOP_STANDBY_IMMEDIATE_SYNC,
    START_STOP_STANDBY_IMMEDIATE,
    START_STOP_STANDBY_SYNC,
    START_STOP_STANDBY,
    START_STOP_EJECT,
    START_STOP_FINISHED
};

/* _ATA_REASSIGN_BLOCKS_STATE */
#define REASSIGN_BLOCKS_ERROR   MV_BIT(31)


enum _ATA_FORMAT_UNIT_STATE {
        FORMAT_UNIT_STARTED = 1,
};

#endif /* __CORE_SAT_H */
