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

#ifndef __MV_COM_PASS_THROUGH_H__
#define __MV_COM_PASS_THROUGH_H__
#include "com_define.h"
#define PASSTHROUGH_ECC_WRITE	1
#define PASSTHROUGH_ECC_SIZE	2

#define SECTOR_LENGTH                           512
#define SECTOR_WRITE                            0
#define SECTOR_READ                             1

#define MAX_PASS_THRU_DATA_BUFFER_SIZE (SECTOR_LENGTH+128)

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct {
 // We put Data_Buffer[] at the very beginning of this structure because SCSI commander did so.
 MV_U8    Data_Buffer[MAX_PASS_THRU_DATA_BUFFER_SIZE];  // set by driver if read, by application if write
 MV_U8    Reserved1[128];
 MV_U32   Data_Length; // set by driver if read, by application if write 
 MV_U16   DevId;       // PD ID (used by application only)
 MV_U8    CDB_Type;    // define a CDB type for each CDB category (used by application only)
 MV_U8    Reserved2;
 MV_U32   lba;
 MV_U8    Reserved3[64];
} PassThrough_Config, * PPassThorugh_Config;

typedef struct {
 // The data structure is used in conjunction with APICDB0_PASS_THRU_CMD_SCSI_16 (see com_api.h)
 // when CDB[16] is required.
 MV_U8    CDB[16];	// CDB is embedded in data buffer
 MV_U32    buf[1];	// actually input/output data buffer
} PassThrough_Config_16, * PPassThorugh_Config_16;

typedef struct _Pass_Through_Cmd
{
	MV_U8 	cdb[16];
	MV_U16 	data_length;
	MV_U8	Reserved[2];	// pad 2 bytes for DWORD alignment.
	MV_U8 	data[1];
}Pass_Through_Cmd,*PPass_Through_Cmd;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
