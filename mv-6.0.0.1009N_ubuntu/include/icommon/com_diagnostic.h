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

#ifndef __MV_COM_DIAGNOSTIC_H__
#define __MV_COM_DIAGNOSTIC_H__
#ifndef COM_DEFINE_H //for magni not use include com_define to define MV_U32
#include "com_define.h"
#endif

typedef struct _SendDiaCDB
{
	MV_U8 OperationCode;
#ifdef __MV_BIG_ENDIAN__
        MV_U8 SelfTestCode:3;
        MV_U8 PF:1;
        MV_U8 Reserved:1;
        MV_U8 SelfTest:1;
        MV_U8 DevOffL:1;
        MV_U8 UnitOffL:1;
#else
	MV_U8 UnitOffL:1;
	MV_U8 DevOffL:1;
	MV_U8 SelfTest:1;
	MV_U8 Reserved:1;
	MV_U8 PF:1;
	MV_U8 SelfTestCode:3;
#endif
	MV_U8 Reserved2;
	MV_U8 ParamLength[2];
	MV_U8 Control;
}SendDiaCDB;

typedef struct _ReceiveDiaCDB
{
	MV_U8 OperationCode;
#ifdef __MV_BIG_ENDIAN__
        MV_U8 Reserved:7;
        MV_U8 PCV:1;
#else
	MV_U8 PCV:1;
	MV_U8 Reserved:7;
#endif
	MV_U8 PageCode;
	MV_U8 AllocationLength[2];
	MV_U8 Control;
}ReceiveDiaCDB;

typedef struct _DianosticPage
{
	MV_U8    PageCode;
	MV_U8    PageCodeSpec;
	MV_U8    PageLength[2];
	MV_U8    Parameters[1];
}DianosticPage, *PDianosticPage;

#define MV_DIAGNOSTIC_CDB_LENGTH   12
#define MV_DIANOSTICPAGE_NONE_DATA  0

#define SCSI_CMD_RECEIVE_DIAGNOSTIC_RESULTS   0x1c  //spc3r23 p97
#define SCSI_CMD_SEND_DIAGNOSTIC              0x1d  //spc3r23 p97

typedef struct _MVATAPIDiagnosticParameters
{
	MV_U8    cdbOffset;
	MV_U8    senseOffset;
	MV_U8    dataOffset;
	MV_U8	 reserved;
	MV_U8    cdb[16];
	MV_U8    senseBuffer[32];
	MV_U32   dataLength;
	MV_U8    Data[1];
}MVATAPIDiagParas, *PMVATAPIDiagParas;

#define MV_API_SEND_PAGE                      0xe0
#define MV_API_RECEV_PAGE                     0xe1

typedef MVATAPIDiagParas send_diagnostic_page;
typedef DianosticPage diagnostic_page;
typedef MVATAPIDiagParas recv_diagnostic_page;

#endif
