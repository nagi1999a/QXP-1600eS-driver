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

#include "com_type.h"
#include "com_define.h"
#include "com_dbg.h"
#include "com_scsi.h"
#include "com_util.h"
#ifdef _OS_UKRN
#include "string.h"
#endif

/* NODRV device is used to send controller commands. 
 * Now we are using "Storage array controller device" as Microsoft recommended.
 * Peripheral Device Type: 03h Processor ( can be 3h or 0ch )
 * Peripheral Qualifier: 0h
 * Response Data Format: 2h ( must be 2 )
 * Version: 4h ( must be 4, 5 or 6 ) 
 * Only need support minimum 36 bytes inquiry data. 
 * Must return EVPD 0x0, 0x83, 0x80 */
#ifndef SUPPORT_VIRTUAL_DEVICE
	/* Standard Inquiry Data for Virtual Device */
	MV_U8 BASEATTR MV_INQUIRY_VIRTUALD_DATA[] = {
				0x03,0x00,0x03,0x03,0xFA,0x00,0x00,0x30,
				'M', 'a', 'r', 'v', 'e', 'l', 'l', ' ',
				0x52,0x41,0x49,0x44,0x20,0x43,0x6F,0x6E,  /* "Raid Con" */
				0x73,0x6F,0x6C,0x65,0x20,0x20,0x20,0x20,  /* "sole    " */
				0x31,0x2E,0x30,0x30,0x20,0x20,0x20,0x20,  /* "1.00    " */
				0x53,0x58,0x2F,0x52,0x53,0x41,0x46,0x2D,  /* "SX/RSAF-" */
				0x54,0x45,0x31,0x2E,0x30,0x30,0x20,0x20,  /* "TE1.00  " */
				0x0C,0x20,0x20,0x20,0x20,0x20,0x20,0x20
			};

	/* EVPD Inquiry Page for Virtual Device */
	//#define MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA	MV_INQUIRY_VPD_PAGE0_DEVICE_DATA
	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA[] = {
		0x03, 0x80, 0x00, 0x08, 'C', 'o', 'n', 's', 'o', 'l', 'e', ' '};
	//#define MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA	MV_INQUIRY_VPD_PAGE83_DEVICE_DATA
#else
	/* If VIRTUAL_DEVICE_TYPE==0x10, Device ID is SCSI\BridgeMARVELL_Virtual_Device__
	 * If VIRTUAL_DEVICE_TYPE==0x0C, Device ID is SCSI\ArrayMARVELL_Virtual_Device__
	 * If VIRTUAL_DEVICE_TYPE==0x03, Device ID is SCSI\ProcessorMARVELL_Virtual_Device__ */
	#define VIRTUAL_DEVICE_TYPE	0x0C
	/* Standard Inquiry Data for Virtual Device */
	MV_U8 BASEATTR MV_INQUIRY_VIRTUALD_DATA[] = {
					VIRTUAL_DEVICE_TYPE,0x00,0x04,0x02,0x20,0x00,0x00,0x00,//?Version should be 0x4 instead of 0x2.
					'M', 'A', 'R', 'V', 'E', 'L', 'L', ' ',
					'V', 'i', 'r', 't', 'u', 'a', 'l', ' ',
					'D', 'e', 'v', 'i', 'c', 'e', ' ', ' ',
					0x31,0x2E,0x30,0x30
					};

	/* EVPD Inquiry Page for Virtual Device */
	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x00, 0x00, 0x03, 0x00, 0x80, 0x83};

	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x80, 0x00, 0x08, 'V', ' ', 'D', 'e', 'v', 'i', 'c', 'e'};

	//MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[] = {
	//	VIRTUAL_DEVICE_TYPE, 0x83, 0x00, 0x0C, 0x01, 0x02, 0x00, 0x08,
	//	0x00, 0x50, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00};
	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x83, 0x00, 0x14, 0x02, 0x01, 0x00, 0x10,
		'M',  'A',  'R',  'V',  'E',  'L',  'L',  ' ',	/* T10 Vendor Identification */
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01	/* Vendor Specific Identifier */
	};
#endif	/* SUPPORT_VIRTUAL_DEVICE */

MV_VOID MV_SetSenseData(
	IN PMV_Sense_Data pSense,
	IN MV_U8 SenseKey,
	IN MV_U8 AdditionalSenseCode,
	IN MV_U8 ASCQ
	)
{
	/* The caller should make sure it's a valid sense buffer. */
	MV_DASSERT( pSense!=NULL );

	if ( pSense!=NULL ) {
		MV_ZeroMemory(pSense, sizeof(MV_Sense_Data));

		pSense->Valid = 0;	/* TBD: Why? */
		pSense->ErrorCode = MV_SCSI_RESPONSE_CODE;
		pSense->SenseKey = SenseKey;
		pSense->AdditionalSenseCode = AdditionalSenseCode;
		pSense->AdditionalSenseCodeQualifier = ASCQ;
		pSense->AdditionalSenseLength = sizeof(MV_Sense_Data) - 8;
	}
}
#if defined( MV_ROC_IOP_TUNED )
MV_U8	mv_scsi_cmd_q_chk_tbl[256] = { 0, };

void mv_scsi_init_q_chk_tbl( void )
{
	int i;
	for (i=0; i<sizeof(mv_scsi_cmd_q_chk_tbl)/sizeof(mv_scsi_cmd_q_chk_tbl[0]); i++)
		mv_scsi_cmd_q_chk_tbl[i] = 0;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_READ_6]|= MSIQCT_READ|MSIQCT_CDB6;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_READ_10]|= MSIQCT_READ|MSIQCT_CDB10;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_READ_12]|= MSIQCT_READ|MSIQCT_CDB12;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_READ_16]|= MSIQCT_READ|MSIQCT_CDB16;

    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_WRITE_6]|= MSIQCT_WRITE|MSIQCT_CDB6;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_WRITE_10]|= MSIQCT_WRITE|MSIQCT_CDB10;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_WRITE_12]|= MSIQCT_WRITE|MSIQCT_CDB12;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_WRITE_16]|= MSIQCT_WRITE|MSIQCT_CDB16;

    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_VERIFY_10]|= MSIQCT_VERIFY|MSIQCT_CDB10;
    mv_scsi_cmd_q_chk_tbl[SCSI_CMD_VERIFY_16]|= MSIQCT_VERIFY|MSIQCT_CDB16;
}
#endif
