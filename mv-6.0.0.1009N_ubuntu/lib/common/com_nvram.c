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

/* ;This file is discarded after POST. */
/*******************************************************************************
*
*                   Copyright 2006,MARVELL SEMICONDUCTOR ISRAEL, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND
* SYSKONNECT GMBH.
*
********************************************************************************
* com_nvram.c - File for implementation of the Driver Intermediate Application Layer
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*   mv_include.h
*
* FILE REVISION NUMBER:
*       $Revision: 1.1.1.1 $
*******************************************************************************/
#ifndef SIMULATOR
#include "mv_include.h"
#include "core_header.h"
#ifndef NEW_CORE_DRIVER
#include "core_helper.h"
#include "core_extern.h"
#endif
#include "core_spi.h"
#include "com_nvram.h"
#include "hba_inter.h"
#include "hba_exp.h"

#if defined(SUPPORT_MAGNI)
#include "hba_api.h"
#endif

#ifdef SUPPORT_ROC
#include "version.h"
	#ifdef NEW_CORE_DRIVER
		#include "core_flash.h"
	#else
		#include "roc_exp.h"
		#include "core_nvm.h"
	#endif
	#include "com_flash.h"
#ifdef SCSI_RW_BUFFER_CMD
#include "com_buffer.h"
/*
Buffer Capacity:
	32k: Flash addr locate range (0-0x800000)
	16k: Flash addr locate range (0-0x400000)
*/
#define MAX_FLASH_BUFFER_CAPACITY_BIT_POWER	15
#define MAX_FLASH_BUFFER_CAPACITY	MV_BIT(MAX_FLASH_BUFFER_CAPACITY_BIT_POWER) //32K;
#endif
#endif

#define SUPPORT_SERIAL_NUM       1

#define SERIAL_NUM_SIG  "MRVL"
#ifdef SUPPORT_ROC
#define DEFAULT_SERIAL_NUM      "FFFFFFFFFF0000000022"
extern int have_flash_layout;
extern MV_BOOLEAN g_update_raw;
#endif
#if (defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)) && defined(_OS_WINDOWS)
extern ULONG g_enable_multi_path;
#endif
static void Itoa(MV_PU8 buffer, MV_U16 data)
{
	MV_U16 ndig;
	MV_U16 tmp;
	MV_U16 ttchar;

	tmp=data; ndig=1;

    while(tmp/16>0){
		ndig++;
		tmp/=16;
    }

    tmp=data;
    while(ndig){
    		ttchar = tmp%16;
    		switch(ttchar){
    			case 10:
    				buffer[ndig-1] = 'a';
    				break;
    			case 11:
    				buffer[ndig-1] = 'b';
    				break;
    			case 12:
    				buffer[ndig-1] =  'c';
    				break;
    			case 13:
    				buffer[ndig-1] = 'd';
    				break;
    			case 14:
    				buffer[ndig-1] = 'e';
    				break;
    			case 15:
    				buffer[ndig-1] = 'f';
    				break;
    			default:
    				buffer[ndig-1] = (MV_U8)ttchar + '0';
   		}
		//buffer[ndig-1] = char_hex;
        tmp/=16;
		ndig--;
    }
}

#ifdef SUPPORT_ROC
static hba_info_page p_hba_info_buf[1];

#if defined(FIRMWARE_BALDUR)
unsigned long get_feature_set_from_hba_page_info(void) {
    return (unsigned long)p_hba_info_buf->HBA_Flag;
}
#endif

#ifdef SUPPORT_SERIAL_NUM
void mv_get_adapter_serial_number(MV_PU8 serno)
{
    MV_CopyMemory(serno, p_hba_info_buf->Serial_Num, 20);
}
#endif
#endif


#ifdef DISABLE_SPI
MV_U8	mvCalculateChecksum(MV_PU8	Address, MV_U32 Size)
{
		MV_U8 checkSum;
		MV_U32 temp=0;
        checkSum = 0;
        for (temp = 0 ; temp < Size ; temp ++)
        {
                checkSum += Address[temp];
        }
        
        checkSum = (~checkSum) + 1;



		return	checkSum;
}
MV_BOOLEAN mv_nvram_init_param( MV_PVOID _phba_ext, pHBA_Info_Page pHBA_Info_Param){
	MV_U32 i = 0, temp;
	MV_U8 buffer[10];
	MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	pHBA_Info_Param->header.signature[0] = 'M';	
	pHBA_Info_Param->header.signature[1] = 'R';
   	pHBA_Info_Param->header.signature[2] = 'V';
  	pHBA_Info_Param->header.signature[3] = 'L';

	// Set BIOS Version
	pHBA_Info_Param->header.minor_ver= NVRAM_DATA_MAJOR_VERSION;
	pHBA_Info_Param->header.major_ver= NVRAM_DATA_MINOR_VERSION;
#else
	pHBA_Info_Param->Signature[0] = 'M';	
	pHBA_Info_Param->Signature[1] = 'R';
   	pHBA_Info_Param->Signature[2] = 'V';
  	pHBA_Info_Param->Signature[3] = 'L';

	// Set BIOS Version
	pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
	pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;
#endif

	// Set SAS address
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
#ifdef NEW_CORE_DRIVER
	for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++)
#else
	for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
#endif
	{
		pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
		pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
		pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
		pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
		pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
		pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
#if defined(SUPPORT_BALDUR)
		/* Baldur uses 4 phy each core, */
		/* two cores must use different SAS address*/
		pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
#else
		pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
#endif
		pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
		/*+(MV_U8)i; - All ports' WWN has to be same */
	}
	
	/* init phy link rate */
	for(i=0;i<8;i++)
	{
		/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
		pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
#else
		pHBA_Info_Param->PHY_Rate[i] = 0x1;/*Default 3Gbps*/
#endif
	}
#endif
	MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

	/* init bga rate */
	pHBA_Info_Param->BGA_Rate = 0x7D;

	pHBA_Info_Param->Rebuild_Rate = 0x7D;
	pHBA_Info_Param->Init_Rate = 0x7D;
	pHBA_Info_Param->Sync_Rate = 0x7D;

#ifdef SUPPORT_RAID_MP
	pHBA_Info_Param->MP_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
	pHBA_Info_Param->Copyback_Rate = 0;
#endif
#ifdef SUPPORT_MIGRATION
	pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
	
	/* init setting flags */
	pHBA_Info_Param->HBA_Flag = 0;
	pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
	pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
	pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
	/* By default auto rebuild is enabled */
	pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
	pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef _BGA_COPY_BACK
	pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
#ifdef SUPPORT_MODULE_CONSOLIDATE
	pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

#ifdef SUPPORT_ROC
	pHBA_Info_Param->RAID_Feature = RAID_FEATURE_ENABLE_RAID;
	/* set next page of HBA info Page*/
	pHBA_Info_Param->Next_Page = FLASH_PARAM_SIZE;
#else
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
	/* set next page of HBA info Page*/
	pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);
#endif
#endif

	pHBA_Info_Param->Check_Sum = 0;
	pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
	return MV_TRUE;
	/* init the parameter in ram */
}

MV_BOOLEAN mv_nvram_moddesc_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
{
#ifdef SUPPORT_ROC
	MV_U32	param_flash_addr=HBA_INFO_OFFSET,i = 0;
	MV_U8 is_secondary = MV_FALSE;
#else
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
#endif

#ifndef SUPPORT_ROC
	AdapterInfo	AI;
	MV_BOOLEAN is_pre_init = MV_TRUE;
#endif
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
#ifndef SUPPORT_ROC
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
	PHBA_Extension phba_ext=NULL;
#endif
	Controller_Infor Controller;

	MV_ZeroMemory(&Controller, sizeof(Controller_Infor));
	return mv_nvram_init_param(This, pHBA_Info_Param);
}
MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param){
	return MV_FALSE;
}
MV_BOOLEAN mv_nvram_init_phy_param(MV_PVOID _phba_ext, pPhy_Info_Page pPhy_Info_Param){
	return MV_FALSE;
}
MV_BOOLEAN mv_read_hba_info( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset)
{
	return MV_FALSE;
}
MV_BOOLEAN mv_read_autoload_data( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset)
{
	return MV_FALSE;
}
#else
#ifdef _OS_LINUX
MV_BOOLEAN mv_nvram_moddesc_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
{
#ifdef SUPPORT_ROC
	MV_U32	param_flash_addr=HBA_INFO_OFFSET,i = 0;
	MV_U8 is_secondary = MV_FALSE;
#else
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
#endif

#ifndef SUPPORT_ROC
	AdapterInfo	AI;
	MV_BOOLEAN is_pre_init = MV_TRUE;
#endif
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
#ifndef SUPPORT_ROC
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
	PHBA_Extension phba_ext=NULL;

#endif
	Controller_Infor Controller;

    MV_ZeroMemory(&Controller, sizeof(Controller_Infor));

	if (!This)
		return MV_FALSE;
#ifndef SUPPORT_ROC
	if(!is_pre_init){
		phba_ext = (PHBA_Extension)HBA_GetModuleExtension(This, MODULE_HBA);
	}
#endif
#ifdef SUPPORT_ROC
	/*Read Data from RAM faster than Flash, so add the HBA_INFO in RAM.*/
	if(p_hba_info_buf->Signature[0] == 'M'&& \
	    p_hba_info_buf->Signature[1] == 'R'&& \
	    p_hba_info_buf->Signature[2] == 'V'&& \
	    p_hba_info_buf->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)p_hba_info_buf, FLASH_PARAM_SIZE)))
	{
		MV_CopyMemory(pHBA_Info_Param, p_hba_info_buf, FLASH_PARAM_SIZE);
		//FM_PRINT("\nHBA_INFO cache hit.....");
		return MV_TRUE;
	}

	if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), param_flash_addr, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
		FM_PRINT("%s %d %s ... Read Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
		if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
			is_secondary = MV_TRUE;
			FM_PRINT("%s %d %s ... Read Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return MV_FALSE;
		}
	}
#else
    if (is_pre_init){
    	#ifdef _OS_LINUX	
		 mv_hba_get_controller_pre(This, &Controller);
    	#else
    		MV_PRINT("Win driver should not go here.\n");
    		MV_DASSERT(MV_FALSE);
    	#endif
	} else {
		HBA_GetControllerInfor(This, &Controller);
	}

#if defined(SUPPORT_BALDUR)
		AI.bar[5] = Controller.Base_Address[5];
#endif
		AI.bar[2] = Controller.Base_Address[2];

#if !defined(BALDUR_FPGA)
	if (-1 == OdinSPI_Init(&AI)) {
		MV_PRINT("Init flash rom failed.\n");
		return MV_FALSE;
	}

	//MV_DPRINT(("Init flash rom ok,flash type is 0x%x.\n",AI.FlashID));

    	/* step 1 read param from flash offset = 0x3FFF00 */
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
#endif /* BALDUR_FPGA */
#endif /* SUPPORT_ROC */

#ifdef SUPPORT_ROC
	/*If Primary HBA_INFO is fail, it will read Secondary.*/
	if (!is_secondary &&  !(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))) {
		//FM_PRINT("\nRead HBA_INFO_SECONDARY");
		if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
			FM_PRINT("%s %d %s ... Read Flash for Secondary Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return MV_FALSE;
		}
	}
#endif

	/* step 2 check the signature first */
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	if(pHBA_Info_Param->header.signature[0] == 'M'&& \
	    pHBA_Info_Param->header.signature[1] == 'R'&& \
	    pHBA_Info_Param->header.signature[2] == 'V'&& \
	    pHBA_Info_Param->header.signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
#else
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
#endif
	{
		if(pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			/* By default auto rebuild is enabled */
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
			pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef _BGA_COPY_BACK
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
#ifdef SUPPORT_MODULE_CONSOLIDATE
			/* By default the module considation is enabled */
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
		}
		
		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;
#ifdef SUPPORT_MIGRATION
		if(pHBA_Info_Param->Migration_Rate == 0xff)
			pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
		if(pHBA_Info_Param->Copyback_Rate == 0xff)
			pHBA_Info_Param->Copyback_Rate = 0;
#endif
#ifdef SUPPORT_RAID_MP
		if(pHBA_Info_Param->MP_Rate == 0xff)
			pHBA_Info_Param->MP_Rate = 0x7D;
#endif	
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
		for(i=0;i<8;i++)
		{
				/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
			if(pHBA_Info_Param->PHY_Rate[i]>0x2)
				pHBA_Info_Param->PHY_Rate[i] = 0x2; /*6Gbps*/
#else
			if(pHBA_Info_Param->PHY_Rate[i]>0x1)
				pHBA_Info_Param->PHY_Rate[i] = 0x1;/*3Gbps*/
#endif

			// validate phy tuning
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[0] = 0;
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[1] = 0;
		}
#endif
	}
	else
	{
		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
		pHBA_Info_Param->header.signature[0] = 'M';	
		pHBA_Info_Param->header.signature[1] = 'R';
	   	pHBA_Info_Param->header.signature[2] = 'V';
	  	pHBA_Info_Param->header.signature[3] = 'L';

		// Set BIOS Version
		pHBA_Info_Param->header.minor_ver= NVRAM_DATA_MAJOR_VERSION;
		pHBA_Info_Param->header.major_ver= NVRAM_DATA_MINOR_VERSION;
#else
		pHBA_Info_Param->Signature[0] = 'M';	
		pHBA_Info_Param->Signature[1] = 'R';
	   	pHBA_Info_Param->Signature[2] = 'V';
	  	pHBA_Info_Param->Signature[3] = 'L';

		// Set BIOS Version
		pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
		pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;
#endif

		// Set SAS address
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
#ifdef NEW_CORE_DRIVER
		for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++)
#else
		for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
#endif
		{
			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
			pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
			pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
#if defined(SUPPORT_BALDUR)
			/* Baldur uses 4 phy each core, */
			/* two cores must use different SAS address*/
			pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
#else
			pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
#endif
			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
			/*+(MV_U8)i; - All ports' WWN has to be same */
		}
		
		/* init phy link rate */
		for(i=0;i<8;i++)
		{
			/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
			pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
#else
			pHBA_Info_Param->PHY_Rate[i] = 0x1;/*Default 3Gbps*/
#endif
		}
#endif
		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

		/* init bga rate */
		pHBA_Info_Param->BGA_Rate = 0x7D;

		pHBA_Info_Param->Rebuild_Rate = 0x7D;
		pHBA_Info_Param->Init_Rate = 0x7D;
		pHBA_Info_Param->Sync_Rate = 0x7D;

#ifdef SUPPORT_RAID_MP
		pHBA_Info_Param->MP_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
		pHBA_Info_Param->Copyback_Rate = 0;
#endif
#ifdef SUPPORT_MIGRATION
		pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
		
		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
		/* By default auto rebuild is enabled */
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef _BGA_COPY_BACK
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
#ifdef SUPPORT_MODULE_CONSOLIDATE
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

#ifdef SUPPORT_SERIAL_NUM
		MV_DPRINT(("Add serial number.\n"));
		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
		temp = (MV_U16)(Controller.Device_Id);
		MV_DPRINT(("%x.\n",Controller.Device_Id));
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
		time = HBA_GetMillisecondInDay();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
		temp=(MV_U16)(time>>16);
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
		time = HBA_GetTimeInSecond();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);
			
		
#endif

#ifdef SUPPORT_ROC
		pHBA_Info_Param->RAID_Feature = RAID_FEATURE_ENABLE_RAID;
		/* set next page of HBA info Page*/
		pHBA_Info_Param->Next_Page = FLASH_PARAM_SIZE;
#else
#if !defined(SUPPORT_ATHENA) && !defined(SUPPORT_SP2)
		/* set next page of HBA info Page*/
		pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);
#endif
#endif

#ifndef SUPPORT_ROC
#if !defined(BALDUR_FPGA)
		/* write to flash and save it now */
		if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
			MV_DPRINT(("mv_nvram_moddesc_init_param: FLASH ERASE SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_moddesc_init_param: FLASH ERASE FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashErase = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param);
			}
		}
#endif /* BALDUR_FPGA */
#endif
		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
		/* init the parameter in ram */
#ifdef SUPPORT_ROC
		FM_PRINT("Reinitialize HBAInfo..\n");
		if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), param_flash_addr, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
			FM_PRINT("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return MV_FALSE;
		}
		//FM_PRINT("\ninitial HBA_INFO_PRIMARY");
		/*Dual HBA_INFO*/
		if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
			FM_PRINT("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return MV_FALSE;
		}
		//FM_PRINT("\ninitial HBA_INFO_SECONDARY");
#else
#if !defined(BALDUR_FPGA)
	//	OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
		if(OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE) != -1) {
			MV_DPRINT(("mv_nvram_moddesc_init_param: FLASH RECOVER SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_moddesc_init_param: FLASH RECOVER FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashBad = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param_bad);
			}
		}
#endif /* BALDUR_FPGA */
#endif
	}

#ifdef SUPPORT_ROC
	/*Keep HBA_Info to RAM*/
	pHBA_Info_Param->Check_Sum = 0;
	pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
	MV_CopyMemory(p_hba_info_buf, pHBA_Info_Param, FLASH_PARAM_SIZE);
#endif
	return MV_TRUE;
}
#endif //#ifdef _OS_LINUX

#ifdef SUPPORT_MAGNI
//copy from MDU trunk source code.
#if !defined(HBAINFO_OFFSET)
//#define HBAINFO_OFFSET		(0x80000 - 0x100) //512k-256 bytes=524032
#endif

/**
    copy from MDU trunk source code.
    You could use this function to get HBA_Info_Page from flash.
    @param pHBA(IN): PHBA_Extension contain bar address and devID for later use.
    @param pHBA_Info_Param(OUT): a pHBA_Info_Page pointer to buffer.
*/
//void mvf_get_HBA_info(void *pBar, pHBA_Info_Page pHBA_Info_Param)
MV_U8 mvf_get_hba_page_info(PHBA_Extension pHBA, pHBA_Info_Page pHBA_Info_Param)

{
	MV_U32    param_flash_addr=0,i = 0;
    
    /* initial a adapter for read flash api use.*/
    AdapterInfo    AI;
    AdapterInfo    *pAI = &AI;

    MV_ZeroMemory(pAI,sizeof(AdapterInfo));

    AI.bar[5] = pHBA->Base_Address[5];
    pAI->FlashBar = pAI->bar[5];

    AI.devId = pHBA->Device_Id;

    /*get address bar according to devID */
    //MV_DPRINT(("%s(),Get device ID : %X\n",__FUNCTION__,pAI->devId));
#if 0
    /* only support magni project now.(magni lite is not supported.)*/
    if (!IsMagni(pAI->devId))
        return;
#endif
	/* initial spi first */
    if (MagniSPI_Init(pAI) == -1){
        MV_DPRINT(("[ERROR] Initial SPI flash failed!!!!\n"));
        return MV_FAIL;
    }
	
    param_flash_addr = HBAINFO_OFFSET;
    if (param_flash_addr == 0){
        MV_DPRINT(("[ERROR]SPI did not initial !!!\n"));
        MV_DPRINT(("Initial SPI ....\n"));
        MagniSPI_Init(&AI);
        param_flash_addr = HBAINFO_OFFSET;
    }
    /* step 1 read param from flash offset */
    MagniSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);

    /* step 2 check the signature first */
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
	{
		//MV_DPRINT(("read param from flash success!\n"));
		return MV_TRUE;
	}
	else
	{
		MV_DPRINT(("read param from flash failed!\n"));
		return MV_FAIL;
    }
}
#endif

#ifndef SUPPORT_ROC
MV_BOOLEAN mv_read_hba_info( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset)
{
	PHBA_Extension phba_ext = ( PHBA_Extension )This;
	struct _HBA_Info_Page hba_info_param;
	unsigned char i = 0;
	int len = 0;
	MV_U8 tmp[21] = {0};

	if ( !phba_ext   )
		return MV_FALSE;	
	
	if (!mv_nvram_init_param(phba_ext, &hba_info_param))
	{
		MV_DPRINT(("get hba info param failed.\n"));
		return MV_FALSE;
	}
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_CopyMemory(tmp, hba_info_param.header.signature, sizeof(hba_info_param.header.signature));
#else
	MV_CopyMemory(tmp, hba_info_param.Signature, sizeof(hba_info_param.Signature));
#endif
#ifdef VS_2012 
	len = sprintf_s(pBuffer, buf_len, "[SIGNATURE]=%s\n", tmp);
#else
	len = sprintf(pBuffer,"[SIGNATURE]=%s\n", tmp);

#endif	
	MV_ZeroMemory(tmp, sizeof(tmp));
	MV_CopyMemory(tmp, hba_info_param.Serial_Num, sizeof(hba_info_param.Serial_Num));
#ifdef VS_2012 
	len += sprintf_s(pBuffer + len, buf_len - len, "[SERIAL_NUMBER]=%s\n", tmp);
#else
	len += sprintf(pBuffer + len,"[SERIAL_NUMBER]=%s\n", tmp);
#endif
#if !defined(_OS_FIRMWARE)&& !defined(SUPPORT_MITAC)
	MV_ZeroMemory(tmp, sizeof(tmp));
	MV_CopyMemory(tmp, hba_info_param.model_number, sizeof(hba_info_param.model_number));			  
#ifdef VS_2012 
	len += sprintf_s(pBuffer + len, buf_len - len, "[MODEL_NUMBER]=%s\n", tmp);
#else
	len += sprintf(pBuffer + len,"[MODEL_NUMBER]=%s\n", tmp);
#endif
#endif
#ifdef VS_2012 
	len += sprintf_s(pBuffer + len, buf_len - len, "[RAID FEATURE]=%d\n", hba_info_param.RAID_Feature);
	len += sprintf_s(pBuffer + len, buf_len - len, "[HBA FLAG]=0x%x\n", hba_info_param.HBA_Flag);
	len += sprintf_s(pBuffer + len, buf_len - len, "[SPINUP GROUP]=%d\n", hba_info_param.DSpinUpGroup);
	len += sprintf_s(pBuffer + len, buf_len - len, "[SPINUP TIME]=%d\n", hba_info_param.DSpinUpTimer);
	len += sprintf_s(pBuffer + len, buf_len - len, "[DELAY TIME]=%d\n", hba_info_param.Delay_Time);
#else
	len += sprintf(pBuffer + len,"[RAID FEATURE]=%d\n", hba_info_param.RAID_Feature);
	len += sprintf(pBuffer + len,"[HBA FLAG]=0x%x\n", hba_info_param.HBA_Flag);
	len += sprintf(pBuffer + len,"[SPINUP GROUP]=%d\n", hba_info_param.DSpinUpGroup);
	len += sprintf(pBuffer + len,"[SPINUP TIME]=%d\n", hba_info_param.DSpinUpTimer);
	len += sprintf(pBuffer + len,"[DELAY TIME]=%d\n", hba_info_param.Delay_Time);

#endif
	*offset += len;
	
    return MV_TRUE;
}
#endif

#define GET_AUTOLOAD_DATA_SIZE		0x100

MV_BOOLEAN mv_read_autoload_data( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset)
{
	AdapterInfo AI;	
	PHBA_Extension phba_ext = ( PHBA_Extension )This;    
	pci_auto_load_config *autoload_c;
	int len = 0;
	MV_U8 tmp_buf[GET_AUTOLOAD_DATA_SIZE] = {0};	
	Controller_Infor Controller;
	int i = 0;
   
	if ( !phba_ext   )
		return MV_FALSE;

	HBA_GetControllerInfor( phba_ext, &Controller);
	
#if defined(SUPPORT_BALDUR)
	AI.bar[5] = Controller.Base_Address[5];
#endif
	AI.bar[2] = Controller.Base_Address[2];

	if (-1 == OdinSPI_Init(&AI)) {
		MV_PRINT("Init flash rom failed.\n");
		return MV_FALSE;
	}
	//MV_DPRINT(("Init flash rom ok,flash type is 0x%x.\n",AI.FlashID));
	
	OdinSPI_ReadBuf( &AI, 0, tmp_buf, GET_AUTOLOAD_DATA_SIZE);	
	autoload_c = (pci_auto_load_config *)(tmp_buf + 0x38);	
 
	if((autoload_c->signature[0] == 0x55) && (autoload_c->signature[1] == 0xAA)) 
	{
#ifdef VS_2012
		len += sprintf_s(pBuffer + len, buf_len - len, "0x0038::0x0000AA55\n");
#else
		len += sprintf(pBuffer + len, "0x0038::0x0000AA55\n");
#endif

        for (i=0; i < (GET_AUTOLOAD_DATA_SIZE/sizeof(MV_U32)); i++) {			
#ifdef VS_2012
		    len += sprintf_s(pBuffer + len, buf_len - len, "0x%04X::0x%08X\n", 0x3C + 4*i, autoload_c->data[i]);
#else
		    len += sprintf(pBuffer + len, "0x%04X::0x%08X\n", 0x3C + 4*i, autoload_c->data[i]);
#endif
			if (autoload_c->data[i] == 0xFFFFFFFF)
			    break;
        }
	}
	*offset += len;
	
    return MV_TRUE;	
}



#ifdef SUPPORT_ROC

MV_U32 find_specified_page(MV_U8 page_code)
{
    hba_info_page hba_info;
    p_page_header pheader = &hba_info.header;
    MV_U32 page_offset = HBA_INFO_OFFSET;
    MV_U32 offset = 0xffffffffL, last_page_offset = 0xffffffffL;

    do {
        Core_NVMRd(NULL, MFR_FLASH_DEV(0), page_offset, HBA_INFO_PAGE_SIZE, (MV_PU8)&hba_info, 0);

        if (pheader->signature[0] == 'M' &&
            pheader->signature[1] == 'R' &&
            pheader->signature[2] == 'V' &&
            pheader->signature[3] == 'L') {
            
            MV_U32 tmp = 0;

            if (pheader->page_code == page_code) {
                offset = page_offset;
                break;
            }

            last_page_offset = page_offset; //Record last page address

            if (pheader->next_page == 0xffff)
                break;

            tmp = pheader->next_page & (~0x01L); //remove bit0
            if (tmp == 0x0000) {
                break;
            }

            if (pheader->next_page & BIT0) //Bit0: 0:=>Address Increase, 1:=>Address Decrease
                page_offset -= tmp;
            else
                page_offset += tmp;
        } else {
            break;
        }
    } while (page_offset < (HBA_INFO_OFFSET + MAX_HBA_INFO_PAGE_SIZE));

    return offset;
}

MV_VOID fill_default_hba_info(MV_PVOID this, phba_info_page pHBA_Info_Param)
{
    PHBA_Extension phba_ext = (PHBA_Extension)this;
    MV_U32 time;
    MV_U16 i, temp;
    MV_U8 buffer[10];
    
    MV_FillMemory((MV_PVOID)pHBA_Info_Param, HBA_INFO_PAGE_SIZE, 0xFF);
    pHBA_Info_Param->header.signature[0] = 'M'; 
    pHBA_Info_Param->header.signature[1] = 'R';
    pHBA_Info_Param->header.signature[2] = 'V';
    pHBA_Info_Param->header.signature[3] = 'L';

    // Set HBA Info version
    pHBA_Info_Param->header.major_ver = HBA_INFO_VER_MAJOR;
    pHBA_Info_Param->header.minor_ver = HBA_INFO_VER_MINOR;

    // Set SAS address
#ifdef NEW_CORE_DRIVER
    for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++)
#else
    for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
#endif
    {
        pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
        pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
        pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
        pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
        pHBA_Info_Param->SAS_Address[i].b[4]=  0x1B;
        pHBA_Info_Param->SAS_Address[i].b[5]=  0x4B;
#if defined(SUPPORT_BALDUR)
        /* Baldur uses 4 phy each core, */
        /* two cores must use different SAS address*/
        pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
#else
        pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
#endif
        pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
        /*+(MV_U8)i; - All ports' WWN has to be same */
    }
    
    MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

    /* init bga rate */
    pHBA_Info_Param->BGA_Rate = 0x7D;

    pHBA_Info_Param->Rebuild_Rate = 0x7D;
    pHBA_Info_Param->Init_Rate = 0x7D;
    pHBA_Info_Param->Sync_Rate = 0x7D;

#ifdef SUPPORT_MP
    pHBA_Info_Param->MP_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
    pHBA_Info_Param->Copyback_Rate = 0x7D;
#endif
#ifdef SUPPORT_MIGRATION
    pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
    
    /* init setting flags */
    pHBA_Info_Param->HBA_Flag = 0;
    pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
    pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
    pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
    pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
    pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
#ifdef _BGA_COPY_BACK
    pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
    pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
    /* By default the module considation is enabled */
    pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
    pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
    pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
    pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;
    pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_FIXED_PORT_MAPPING;
    pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_DISABLE_TWSI_I2C;

#ifdef SUPPORT_SERIAL_NUM
    MV_DPRINT(("Add serial number.\n"));
#ifdef SUPPORT_ROC
    MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), DEFAULT_SERIAL_NUM, 20); //we will generate new serial number later
#else
    MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
    temp = (MV_U16)(phba_ext->Device_Id);
    MV_DPRINT(("%x.\n",phba_ext->Device_Id));

    Itoa(buffer,temp);
    MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
    time = HBA_GetMillisecondInDay();
    temp = (MV_U16)time;
    Itoa(buffer,temp);
    MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
    temp=(MV_U16)(time>>16);
    Itoa(buffer,temp);
    MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
    time = HBA_GetTimeInSecond();
    temp = (MV_U16)time;
    Itoa(buffer,temp);
    MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);
#endif
#endif

    pHBA_Info_Param->RAID_Feature = RAID_FEATURE_ENABLE_RAID;
    /* set next page of HBA info Page*/
    pHBA_Info_Param->header.page_code = PAGE_CODE_HBA_INFO;
    pHBA_Info_Param->header.next_page = NEXT_PAGE_OFFSET;
    pHBA_Info_Param->header.data_length = sizeof(hba_info_page) - sizeof(page_header);

    pHBA_Info_Param->header.check_sum = 0;
    pHBA_Info_Param->header.check_sum = mvCalculateChecksum((MV_PU8)pHBA_Info_Param, HBA_INFO_PAGE_SIZE);

}

/*
Change HBA info content must increase version number
*/
MV_BOOLEAN mv_nvram_init_param( MV_PVOID _phba_ext, phba_info_page pHBA_Info_Param)
{
	MV_U32 param_flash_addr = HBA_INFO_OFFSET,i = 0;
	MV_U8 is_secondary = MV_FALSE;
	PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;
	MV_U32 time;
#ifdef SUPPORT_SERIAL_NUM
	MV_U32 rtcval = PLATFORM_RTC_READ();
#endif
	MV_U8 buffer[10];

    if (!phba_ext)
        return MV_FALSE;

    /*Read Data from RAM faster than Flash, so add the HBA_INFO in RAM.*/
    if (p_hba_info_buf->header.signature[0] == 'M' && \
        p_hba_info_buf->header.signature[1] == 'R' && \
        p_hba_info_buf->header.signature[2] == 'V' && \
        p_hba_info_buf->header.signature[3] == 'L' && \
        (!mvVerifyChecksum((MV_PU8)p_hba_info_buf, HBA_INFO_PAGE_SIZE)))
    {
        MV_CopyMemory(pHBA_Info_Param, p_hba_info_buf, HBA_INFO_PAGE_SIZE);
        //FM_PRINT("\nHBA_INFO cache hit.....");
        return MV_TRUE;
    }

    param_flash_addr = find_specified_page(PAGE_CODE_HBA_INFO);
    if (param_flash_addr == 0xffffffffL) {
        FM_PRINT("Can't find HBA info page\n");
		param_flash_addr = HBA_INFO_OFFSET; /*HBA_INFO Start Address*/
    }
    
    if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), param_flash_addr, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
        FM_PRINT("%s %d %s ... Read Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
        if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
            is_secondary = MV_TRUE;
            FM_PRINT("%s %d %s ... Read Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return MV_FALSE;
        }
    }

	/*If Primary HBA_INFO is fail, it will read Secondary.*/
	if (!is_secondary &&  !(pHBA_Info_Param->header.signature[0] == 'M'&& \
	    pHBA_Info_Param->header.signature[1] == 'R'&& \
	    pHBA_Info_Param->header.signature[2] == 'V'&& \
	    pHBA_Info_Param->header.signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,HBA_INFO_PAGE_SIZE)))) {
		//FM_PRINT("\nRead HBA_INFO_SECONDARY");
		if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0)) {
			FM_PRINT("%s %d %s ... Read Flash for Secondary Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return MV_FALSE;
		}
	}

	/* step 2 check the signature first */
	if(pHBA_Info_Param->header.signature[0] == 'M'&& \
	    pHBA_Info_Param->header.signature[1] == 'R'&& \
	    pHBA_Info_Param->header.signature[2] == 'V'&& \
	    pHBA_Info_Param->header.signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param, HBA_INFO_PAGE_SIZE)))
	{
		if (pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
#ifdef _BGA_COPY_BACK
            pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
            pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
			/* By default the module considation is enabled */
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_FIXED_PORT_MAPPING;
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_DISABLE_TWSI_I2C;
		}

		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;
#ifdef SUPPORT_MIGRATION
		if(pHBA_Info_Param->Migration_Rate == 0xff)
			pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
		if(pHBA_Info_Param->Copyback_Rate == 0xff)
			pHBA_Info_Param->Copyback_Rate = 0x7D;
#endif
#ifdef SUPPORT_MP
		if(pHBA_Info_Param->MP_Rate == 0xff)
			pHBA_Info_Param->MP_Rate = 0x7D;
#endif		
	}
	else
	{
	    //HBA info corrupt, fill default value.
        fill_default_hba_info(phba_ext, pHBA_Info_Param);
        
        /* init the parameter in ram */
        FM_PRINT("Reinitialize HBAInfo..\n");
        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), param_flash_addr, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return MV_FALSE;
        }
        //FM_PRINT("\ninitial HBA_INFO_PRIMARY");
        /*Dual HBA_INFO*/
        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return MV_FALSE;
        }
	}
#if 0
    pHBA_Info_Param->HBA_Flag |=   HBA_FLAG_FIXED_PORT_MAPPING;
    pHBA_Info_Param->HBA_Flag |=   HBA_FLAG_DISABLE_TWSI_I2C;
#endif

#ifdef SUPPORT_SERIAL_NUM
    if (memcmp(pHBA_Info_Param->Serial_Num,DEFAULT_SERIAL_NUM,20)==0)
    {
        MV_U32 temp;

        MV_PRINT("Generate new serial number.\n");
        MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
        temp = (MV_U16)(phba_ext->Device_Id);
        sprintf(buffer, "%04X", temp);
        MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);

        sprintf(buffer, "%08X", rtcval);
        MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 8);

        rtcval = PLATFORM_RTC_READ();
        sprintf(buffer, "%04X", (MV_U16)rtcval);
        MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);

        pHBA_Info_Param->header.check_sum = 0;
        pHBA_Info_Param->header.check_sum = mvCalculateChecksum((MV_PU8)pHBA_Info_Param, HBA_INFO_PAGE_SIZE);

        /* init the parameter in ram */
        FM_PRINT("Reinitialize HBAInfo..\n");
        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), param_flash_addr, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return MV_FALSE;
        }
        //FM_PRINT("\ninitial HBA_INFO_PRIMARY");
        /*Dual HBA_INFO*/
        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, HBA_INFO_PAGE_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
            return MV_FALSE;
        }
    }
#endif

#ifdef SUPPORT_SERIAL_NUM
    MV_PRINT("Serial number: ");
    for (i=0;i<20;i++)
        MV_PRINT("%c",pHBA_Info_Param->Serial_Num[i]);
    MV_PRINT("\n");
#endif

	/*Keep HBA_Info to RAM*/
	pHBA_Info_Param->header.check_sum = 0;
	pHBA_Info_Param->header.check_sum = mvCalculateChecksum((MV_PU8)pHBA_Info_Param, HBA_INFO_PAGE_SIZE);
	MV_CopyMemory(p_hba_info_buf, pHBA_Info_Param, HBA_INFO_PAGE_SIZE);
#ifdef SUPPORT_MODULE_CONSOLIDATE
	if (p_hba_info_buf->HBA_Flag & HBA_FLAG_DISABLE_MOD_CONSOLIDATE) {
		phba_ext->Cc_Enable = 0;
	} else {
		phba_ext->Cc_Enable = 1;
	}
	MV_PRINT("Module Command Consolidate status is [%s]\n", phba_ext->Cc_Enable? "Enable":"Disable");
#endif
    if (p_hba_info_buf->HBA_Flag & HBA_FLAG_FIXED_PORT_MAPPING)
        MV_PRINT("HBA_FLAG_FIXED_PORT_MAPPING  ----------- Enable\n");
    else
        MV_PRINT("HBA_FLAG_FIXED_PORT_MAPPING  ----------- Disable\n");
    MV_PRINT("TWSI I2C is ----------- %s\n", (p_hba_info_buf->HBA_Flag & HBA_FLAG_DISABLE_TWSI_I2C)? "Disable":"Enable");
	return MV_TRUE;

}


MV_VOID fill_default_phy_tune_page(pphy_info_page phy_info)
{
    int i = 0;

    MV_FillMemory((MV_PVOID)phy_info, PHY_INFO_PAGE_SIZE, 0xFF);
    phy_info->header.signature[0] = 'M'; 
    phy_info->header.signature[1] = 'R';
    phy_info->header.signature[2] = 'V';
    phy_info->header.signature[3] = 'L';

    // Set HBA Info version
    phy_info->header.major_ver = HBA_INFO_VER_MAJOR;
    phy_info->header.minor_ver = HBA_INFO_VER_MINOR;
    
    for (i = 0; i < 8; i++) {
#if defined(LACIE_THUNDERBOLT)
        *(MV_PU32)&phy_info->phy_tuning_1[i] = 0x0000700D;
        *(MV_PU32)&phy_info->phy_tuning_2[i] = 0x0000750D;
        *(MV_PU32)&phy_info->phy_tuning_3[i] = 0x0000750D;
        if (i == 1)
            *(MV_PU32)&phy_info->phy_tuning_3[i] = 0x0000770D;
        *(MV_PU8)&phy_info->FFE_Control[i] = 0x7C;
        phy_info->PHY_Rate[i] = 0x2; //max 6.0G  
#else
        *(MV_PU32)&phy_info->phy_tuning_1[i] = 0x00007A0D;
        *(MV_PU32)&phy_info->phy_tuning_2[i] = 0x00007A0D;
        *(MV_PU32)&phy_info->phy_tuning_3[i] = 0x00007A0D;
        *(MV_PU8)&phy_info->FFE_Control[i] = 0x7C;
        phy_info->PHY_Rate[i] = 0x2; //max 6.0G  
#endif
    }

    phy_info->header.page_code = PAGE_CODE_PHY_INFO;
    phy_info->header.next_page = NEXT_PAGE_OFFSET;
    phy_info->header.data_length = sizeof(phy_info_page) - sizeof(page_header);
    
    phy_info->header.check_sum = 0;
    phy_info->header.check_sum = mvCalculateChecksum((MV_PU8)phy_info, PHY_INFO_PAGE_SIZE);
}

MV_BOOLEAN mv_get_phy_tune_page(MV_PVOID _phba_ext, pphy_info_page pphy_info)
{
    MV_U32 param_flash_addr = 0xffffffffL, i = 0;
    PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;
	page_header *ppage_header;

    ppage_header = (struct page_header *)&pphy_info->header;

    param_flash_addr = find_specified_page(PAGE_CODE_PHY_TUNING);
    if (param_flash_addr == 0xffffffffL) {
        MV_PRINT("Can't find PHY Tuning page\n");
        param_flash_addr = PHY_INFO_PRIMARY_OFFSET; 
    }

    if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), param_flash_addr, PHY_INFO_PAGE_SIZE, (MV_PU8)pphy_info, 0)) {
        MV_PRINT("%s %d %s ... Read Flash for PHY tuning page failed\n", __FILE__, __LINE__, __FUNCTION__);
    }

    /* step 2 check the signature first */
    if(ppage_header->signature[0] == 'M'&& \
        ppage_header->signature[1] == 'R'&& \
        ppage_header->signature[2] == 'V'&& \
        ppage_header->signature[3] == 'L' && \
        ppage_header->data_length == (PHY_INFO_PAGE_SIZE - sizeof(page_header)) && \
        ppage_header->page_code == PAGE_CODE_PHY_TUNING  && \
        (!mvVerifyChecksum((MV_PU8)pphy_info, PHY_INFO_PAGE_SIZE))) {
        return MV_TRUE;
    } else {
        if(!Core_NVMRd(NULL, MFR_FLASH_DEV(0), PHY_INFO_SECONDARY_OFFSET, PHY_INFO_PAGE_SIZE, (MV_PU8)pphy_info, 0)) {
            MV_PRINT("%s %d %s ... Read Flash for PHY tuning page failed\n", __FILE__, __LINE__, __FUNCTION__);
        }
        
        if(ppage_header->signature[0] == 'M'&& \
            ppage_header->signature[1] == 'R'&& \
            ppage_header->signature[2] == 'V'&& \
            ppage_header->signature[3] == 'L' && \
            ppage_header->data_length == (PHY_INFO_PAGE_SIZE - sizeof(page_header)) && \
            ppage_header->page_code == PAGE_CODE_PHY_TUNING  && \
            (!mvVerifyChecksum((MV_PU8)pphy_info, PHY_INFO_PAGE_SIZE))) {
            return MV_TRUE;
        }
        
        MV_PRINT("check sum failed, fill default value\n");

        fill_default_phy_tune_page(pphy_info);

        /* init the parameter in ram */
        FM_PRINT("Reinitialize PHY Info..\n");
        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), param_flash_addr, PHY_INFO_PAGE_SIZE, (MV_PU8)pphy_info, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Primary PHY_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }

        if((have_flash_layout)&&(!Core_NVMWr( NULL, MFR_FLASH_DEV(0), PHY_INFO_SECONDARY_OFFSET, PHY_INFO_PAGE_SIZE, (MV_PU8)pphy_info, 0))) {
            FM_PRINT("%s %d %s ... Write Flash for Secondary PHY_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
        }
        
        return MV_TRUE;
    }

}

#else //#ifdef SUPPORT_ROC

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
/**
    Because of each page has a page header, you could prepare header by this function.
*/
void prepare_page_header(IN OUT void* in_p_page,IN MV_U32 page_size,IN MV_U8 page_code ,IN  MV_U16 next_page)
{
    p_page_header p_header = (p_page_header)in_p_page;
    /* fill signature */
    p_header->signature[0] = 'M';
    p_header->signature[1] = 'R';
    p_header->signature[2] = 'V';
    p_header->signature[3] = 'L';

    /* fill page_code */
    p_header->page_code = page_code;

    /* fill data_length */
    p_header->data_length = (MV_U16)(page_size - sizeof(page_header));

    /* fill next page number */
    p_header->next_page = next_page;

    /* fill page header reversion */
    p_header->major_ver = PAGE_HEADER_MAJOR_VER;
    p_header->minor_ver = PAGE_HEADER_MINOR_VER;

    /* caculate checksum */
    p_header->check_sum = 0;
	
    /* redefine checksum to includ page header only */
    p_header->check_sum= mvCalculateChecksum((MV_PU8)p_header, sizeof(page_header));
}

MV_U32 find_page_offset(AdapterInfo *pAI, MV_U8 page_code)
{
	page_header header;
    	MV_U32 page_offset = PARAM_OFFSET; /*First Page(HBA Info) 1024K-256*/
    	MV_U32 tmp = 0;
    	MV_U32 offset = 0xffffffffL;
	MV_BOOLEAN find_page = MV_FALSE;

    	do {
        	OdinSPI_ReadBuf(pAI, page_offset, (MV_U8*)&header, sizeof(page_header));
	    	if (header.signature[0] == 'M' &&
	        	header.signature[1] == 'R' &&
	        	header.signature[2] == 'V' &&
	        	header.signature[3] == 'L') {  
            	//MV_DPRINT(("Finding page header....(0x%08x)\n", page_offset));

		if (header.page_code == page_code) {
                offset = page_offset;
		  find_page = MV_TRUE;
                break;
            }

	       if (header.next_page == PAGE_CODE_NULL_PAGE)
                break;
	
           
            tmp = header.next_page & (~0x01L); //remove bit0
            if (tmp == 0x0000) {
                break;
            }

            if (header.next_page & SEARCH_ADDR_DECREASE) //Bit0: 0:=>Address Increase, 1:=>Address Decrease
                page_offset -= tmp;
            else
                page_offset += tmp;
        	} else {
            		break;
        	}
    	} while (page_offset < ATHENA_FLASH_SIZE);
   
    return offset;
}

/**
    This function will do some steps:
    1. read from spi flash
    2. modify specificed part
    3. write back.
*/
#define FLASH_TAG  'gALf'
MV_U8 read_modify_write_spi_flash(
    IN AdapterInfo *pAI, 
    IN MV_U32 flash_addr, 
    IN void* pdata_buffer,
    IN MV_U32 data_length
) {
	    MV_U32 sz = 0;
	    MV_U32 addr, offset;
	    MV_U8 *pflash_buf = NULL;
	    MV_U8 ret = 0;

	    if(data_length > pAI->FlashSectSize){
	    	MV_DPRINT(("[ERROR]flash size %x > BLOCK SIZE %x.(read_modify_write_spi_flash())\n", data_length, pAI->FlashSectSize));
		return -1;
	    }

	    /* reference FW flash_program_from_mem()*/
	    addr = (flash_addr & ~(pAI->FlashSectSize- 1));
	    /* allocate block buffer. */
	    sz = (flash_addr & (pAI->FlashSectSize - 1));
	    sz = ((sz + pAI->FlashSectSize - 1) / pAI->FlashSectSize) * pAI->FlashSectSize;
	    if (sz <= 0)
	        sz = pAI->FlashSectSize;
	    MV_DPRINT(("[read_modify_write_spi_flash]sz = 0x%x, addr=0x%x\n",sz, addr));

#ifdef _OS_LINUX
	    pflash_buf = kmalloc(sz, GFP_KERNEL);
#else
	    pflash_buf = ExAllocatePoolWithTag(NonPagedPool, sz, FLASH_TAG);
#endif
	    if (pflash_buf == NULL) {
	        MV_DPRINT(("[ERROR]malloc failed!! (read_modify_write_spi_flash())\n"));
	        return -1;
	    }

	    /* read original data from flash to buffer. */
	   OdinSPI_ReadBuf( pAI, addr, pflash_buf, sz);

	    /* replace the data. */
	    offset = flash_addr & (pAI->FlashSectSize - 1);
	    MV_DPRINT(("Modify with new data starting offset 0x%lx. len = 0x%lx size= 0x%lx\n", offset, data_length, sz));
	    if ((offset + data_length) > sz) {
	    	MV_DPRINT(("Data buffer is overflowing...(%x, %x)\n", offset + data_length, sz));
		ret = -1;
	    } else {
	        memcpy(pflash_buf + offset, pdata_buffer, data_length);

	   MV_DPRINT(("Erasing address...%lx\n", addr));
	   if( 0 != OdinSPI_SectErase( pAI, addr) ) {
	            MV_DPRINT(("[ERROR]read_modify_write_spi_flash: Erase Sector Failed!!\n"));
#ifdef _OS_LINUX                
	            kfree(pflash_buf);
#else
	            ExFreePoolWithTag(pflash_buf, FLASH_TAG);
#endif
	            return -1;
	   }

	        /* write back to flash */
	        MV_DPRINT(("Programming 0x%lx bytes...\n", sz));
		 OdinSPI_WriteBuf(pAI, addr, pflash_buf, sz);       
	    }
	    MV_DPRINT(("read_modify_write_spi_flash()...free resource...\n"));

#ifdef _OS_LINUX
	    kfree(pflash_buf);
#else
	    ExFreePoolWithTag(pflash_buf, FLASH_TAG);
#endif
	    MV_DPRINT(("read_modify_write_spi_flash()...done\n"));
	    return ret;
}

 MV_BOOLEAN mv_nvram_init_phy_param(MV_PVOID _phba_ext, pPhy_Info_Page pPhy_Info_Param)
{
	PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;	
	page_header *ppage_header = (page_header *)pPhy_Info_Param;
	HBA_Info_Page HBA_Info_Param;
	AdapterInfo AI;
	MV_U32 flash_addr = PARAM_OFFSET;
       MV_U32 previous_page_addr = PARAM_OFFSET;
	MV_U8 port_num = 16,  i=0;
	MV_BOOLEAN initState=MV_TRUE;
	
	MV_ZeroMemory(&AI, sizeof(AdapterInfo));
	if (!phba_ext)
        	return MV_FALSE;

	 AI.devId = phba_ext->Device_Id;
#ifdef _OS_LINUX	
	 AI.bar[0] = phba_ext->Base_Address[0];
#else
        AI.bar[2] = phba_ext->Base_Address[2];
#endif
	if (OdinSPI_Init(&AI) == -1) {
		MV_PRINT("SPI Init failed.\n");
		return MV_FALSE;
	}	
		
    	flash_addr = find_page_offset(&AI, PAGE_CODE_HBA_INFO);
	//printf("mv_nvram_init_phy_param hba flash_addr=%x\n", flash_addr);
      if (flash_addr == 0xffffffffL) {	
        	MV_PRINT("No HBA Page Info, generate a new one..\n");
		initState=mv_nvram_init_param(phba_ext,  &HBA_Info_Param);        
      }  else {
		OdinSPI_ReadBuf( &AI, flash_addr, (MV_PU8)&HBA_Info_Param, FLASH_PARAM_SIZE);
      }

	flash_addr = find_page_offset(&AI, PAGE_CODE_PHY_INFO);
	if (flash_addr == 0xffffffffL) {	
 		MV_PRINT("Create new Phy_info Page..\n");
		MV_FillMemory((MV_PVOID)pPhy_Info_Param, sizeof(Phy_Info_Page),0xFF);
		prepare_page_header((MV_PVOID)pPhy_Info_Param, sizeof(Phy_Info_Page), PAGE_CODE_PHY_INFO, PAGE_CODE_NULL_PAGE);
			
		//set phy info page default value
		 for (i=0; i < port_num; i++)
		 {		 
			pPhy_Info_Param->SAS_Address[i].b[0]=  0x50;
			pPhy_Info_Param->SAS_Address[i].b[1]=  0x05;
			pPhy_Info_Param->SAS_Address[i].b[2]=  0x04;
			pPhy_Info_Param->SAS_Address[i].b[3]=  0x36;
			pPhy_Info_Param->SAS_Address[i].b[4]=  0x03 + (MV_U8)(i/8);
			pPhy_Info_Param->SAS_Address[i].b[5]=  0x00;
			pPhy_Info_Param->SAS_Address[i].b[6]=  0x00;
			pPhy_Info_Param->SAS_Address[i].b[7]=  0x00; 
			/*+(MV_U8)i; - All ports' WWN has to be same */

			pPhy_Info_Param->PHY_Rate[i] = 0x3; //default 12G for athena

			pPhy_Info_Param->FFE_Control[i].FFE_Capacitor_Select = 0xE;
			pPhy_Info_Param->FFE_Control[i].FFE_Resistor_Select = 0x3;

			/*
			pPhy_Info_Param->PHY_Tuning[i].Trans_Amp=0x19;			
			pPhy_Info_Param->PHY_Tuning[i].Trans_Emphasis_Amp = 0x3;
			pPhy_Info_Param->PHY_Tuning[i].Trans_Amp_Adjust = 0x1;
			pPhy_Info_Param->PHY_Tuning[i].Trans_Emphasis_En = 0x1;
			*/
			pPhy_Info_Param->PHY_Tuning_Gen1[i].Trans_Emphasis_Amp = 0x1;
			pPhy_Info_Param->PHY_Tuning_Gen1[i].Trans_Amp = 0x6;
			pPhy_Info_Param->PHY_Tuning_Gen1[i].Trans_Amp_Adjust = 0x1;
			pPhy_Info_Param->PHY_Tuning_Gen1[i].Trans_Emphasis_En = 0x1;

			pPhy_Info_Param->PHY_Tuning_Gen2[i].Trans_Emphasis_Amp = 0x2;
			pPhy_Info_Param->PHY_Tuning_Gen2[i].Trans_Amp = 0x8;
			pPhy_Info_Param->PHY_Tuning_Gen2[i].Trans_Amp_Adjust = 0x1;
			pPhy_Info_Param->PHY_Tuning_Gen2[i].Trans_Emphasis_En = 0x1;

			pPhy_Info_Param->PHY_Tuning_Gen3[i].Trans_Emphasis_Amp = 0x4;
			pPhy_Info_Param->PHY_Tuning_Gen3[i].Trans_Amp = 0xB;
			pPhy_Info_Param->PHY_Tuning_Gen3[i].Trans_Amp_Adjust = 0x1;
			pPhy_Info_Param->PHY_Tuning_Gen3[i].Trans_Emphasis_En = 0x1;

			pPhy_Info_Param->PHY_Tuning_Gen4[i].Trans_Emphasis_Amp = 0x5;
			pPhy_Info_Param->PHY_Tuning_Gen4[i].Trans_Amp = 0x10;
			pPhy_Info_Param->PHY_Tuning_Gen4[i].Trans_Amp_Adjust = 0x1;
			pPhy_Info_Param->PHY_Tuning_Gen4[i].Trans_Emphasis_En = 0x1;
			

		}

             pPhy_Info_Param->check_sum = 0;
             pPhy_Info_Param->check_sum = mvCalculateChecksum((MV_PU8)pPhy_Info_Param, sizeof(Phy_Info_Page));

		HBA_Info_Param.header.next_page= sizeof(Phy_Info_Page) | SEARCH_ADDR_DECREASE;
		
		HBA_Info_Param.header.check_sum = 0;
		HBA_Info_Param.header.check_sum = mvCalculateChecksum((MV_PU8)&(HBA_Info_Param.header), sizeof(page_header));

		HBA_Info_Param.Check_Sum = 0;
		HBA_Info_Param.Check_Sum = mvCalculateChecksum((MV_PU8)&HBA_Info_Param, sizeof(HBA_Info_Page));

		flash_addr = (MV_U32)(PARAM_OFFSET - sizeof(Phy_Info_Page));
		if(initState == MV_TRUE){
			read_modify_write_spi_flash(&AI, flash_addr,  pPhy_Info_Param, sizeof(Phy_Info_Page));
			read_modify_write_spi_flash(&AI, PARAM_OFFSET,  &HBA_Info_Param, sizeof(HBA_Info_Page));
		}
	}
#if (defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)) && defined(_OS_WINDOWS)
	//Check if support multi path feature 
        if(HBA_Info_Param.HBA_Flag & HBA_FLAG_MULTI_PATH)
		g_enable_multi_path=MV_TRUE;
	 else 
	 	g_enable_multi_path=MV_FALSE;
#endif
    	 OdinSPI_ReadBuf(&AI, flash_addr, (MV_PU8)pPhy_Info_Param, sizeof(Phy_Info_Page));
        if (ppage_header->signature[0] == 'M' &&
            ppage_header->signature[1] == 'R' &&
            ppage_header->signature[2] == 'V' &&
            ppage_header->signature[3] == 'L' &&
            ppage_header->page_code == PAGE_CODE_PHY_INFO &&
            (!mvVerifyChecksum((MV_PU8)ppage_header, sizeof(page_header))) &&
            (!mvVerifyChecksum((MV_PU8)pPhy_Info_Param, sizeof(Phy_Info_Page)))) {
            MV_DPRINT(("The flash address of Phy_Info Page is at 0x%08lx.\n", flash_addr));
            return MV_TRUE;
        }

	 MV_PRINT("The Phy_info Page is invaild....\n");
        return MV_FALSE;
}

MV_BOOLEAN mv_nvram_init_param( MV_PVOID _phba_ext, pHBA_Info_Page pHBA_Info_Param)
{
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
	AdapterInfo	AI;
	PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
	page_header *phba_header;
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
       Controller_Infor Controller;

	//printf("mv_nvram_init_param\n");
	MV_ZeroMemory(&AI, sizeof(AdapterInfo));
       if (!phba_ext)
        	return MV_FALSE;
	HBA_GetControllerInfor( phba_ext, &Controller);
#ifdef _OS_LINUX	
 	AI.bar[0] = phba_ext->Base_Address[0];
#else //_OS_LINUX
	AI.bar[2] = phba_ext->Base_Address[2];
#endif
	if (OdinSPI_Init(&AI) == -1) {
		MV_PRINT("SPI Init failed.\n");
		return MV_FALSE;
	}
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
	phba_header = (page_header *)pHBA_Info_Param;
	
	/* step 2 check the signature first */
	if(pHBA_Info_Param->header.signature[0] == 'M'&& \
	    pHBA_Info_Param->header.signature[1] == 'R'&& \
	    pHBA_Info_Param->header.signature[2] == 'V'&& \
	    pHBA_Info_Param->header.signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)phba_header,sizeof(page_header))) && 
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page))))
	{
		if (pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 

            pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
			/* By default the module considation is enabled */
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;
		//pHBA_Info_Param->HBA_Flag |= HBA_FLAG_SATA_SSU_MODE;
		}

		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;
	}
	else
	{
	    //HBA info corrupt, fill default value.
	    	MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
	    	prepare_page_header((MV_PVOID)pHBA_Info_Param, sizeof(HBA_Info_Page), PAGE_CODE_HBA_INFO, PAGE_CODE_NULL_PAGE);
		
		/* init bga rate */
		pHBA_Info_Param->BGA_Rate = 0x7D;
		pHBA_Info_Param->Rebuild_Rate = 0x7D;
		pHBA_Info_Param->Init_Rate = 0x7D;
		pHBA_Info_Param->Sync_Rate = 0x7D;
	
		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 

        pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
		/* By default the module considation is enabled */
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;
		//pHBA_Info_Param->HBA_Flag |=  HBA_FLAG_SATA_SSU_MODE;
#ifdef _OS_LINUX	
		if ((Controller.Device_Id == DEVICE_ID_ATHENA_1485) ||(Controller.Device_Id == DEVICE_ID_ATHENA_1495)||(Controller.Device_Id == DEVICE_ID_ATHENA_1475))
#else
           	if ((phba_ext->Device_Id == DEVICE_ID_ATHENA_1485) ||(phba_ext->Device_Id == DEVICE_ID_ATHENA_1495)||(phba_ext->Device_Id == DEVICE_ID_ATHENA_1475))
#endif 
              {
			pHBA_Info_Param->RAID_Feature &= ~RAID_FEATURE_ENABLE_RAID;
			pHBA_Info_Param->RAID_Feature |= RAID_FEATURE_DISABLE_RAID5;		
		} else {
			pHBA_Info_Param->RAID_Feature |= RAID_FEATURE_ENABLE_RAID;
			pHBA_Info_Param->RAID_Feature &= ~RAID_FEATURE_DISABLE_RAID5;
		}

#ifdef SUPPORT_SERIAL_NUM
		MV_DPRINT(("Add serial number.\n"));

		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), "MRVL" , 4);

		temp = (MV_U16)(phba_ext->Device_Id);
		MV_DPRINT(("%x.\n",phba_ext->Device_Id));

		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
		time = HBA_GetMillisecondInDay();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
		temp=(MV_U16)(time>>16);
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
		time = HBA_GetTimeInSecond();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);

		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->model_number), "FFFFFFFFFFFFFFFFFFFF" , 20);
#endif

		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param, sizeof(HBA_Info_Page));
		
		//printf("hba checksum=%x,  value=%x\n",pHBA_Info_Param->Check_Sum, mvVerifyChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page)));
		/* init the parameter in ram */
		/* write to flash and save it now */
		if(AI.FlashSize!=0){
			if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
				MV_DPRINT(("mv_nvram_init_param: FLASH ERASE SUCCESS!\n"));
			}
			else {
				MV_DPRINT(("mv_nvram_init_param: FLASH ERASE FAILED!\n"));
				if(phba_ext) {
				phba_ext->FlashErase = 1; 
				if(phba_ext->State == DRIVER_STATUS_STARTED)
					HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param);
				}
			}

		//	OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
			if(OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE) != -1) {
				MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER SUCCESS!\n"));
			}
			else {
				MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER FAILED!\n"));
				if(phba_ext) {
				phba_ext->FlashBad = 1; 
				if(phba_ext->State == DRIVER_STATUS_STARTED)
					HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param_bad);
				}
			}	
		}else{
			return MV_FALSE;
		}
	}

	return MV_TRUE;
}
#else
MV_BOOLEAN mv_nvram_init_param( MV_PVOID _phba_ext, pHBA_Info_Page pHBA_Info_Param)
{
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
	AdapterInfo	AI;
	PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
#ifdef _OS_LINUX
        Controller_Infor Controller;
#endif

    if (!phba_ext)
        return MV_FALSE;

#if defined(SUPPORT_MAGNI)
    if ((phba_ext->RunAsNonRAID == MV_FALSE)
        || (phba_ext->RunAsNonRAID && phba_ext->report_to_gui))
    {
    		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
    		pHBA_Info_Param->Signature[0] = 'M';	
    		pHBA_Info_Param->Signature[1] = 'R';
    	   	pHBA_Info_Param->Signature[2] = 'V';
    	  	pHBA_Info_Param->Signature[3] = 'L';

    		// Set BIOS Version
    		pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
    		pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;

    		// Set SAS address
#ifdef NEW_CORE_DRIVER
    		for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++)
#else
    		for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
#endif
    		{
    			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
    			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
    			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
    			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
    			pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
    			pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
#if defined(SUPPORT_BALDUR)
    			/* Baldur uses 4 phy each core, */
    			/* two cores must use different SAS address*/
    			pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
#else
    			pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
#endif
    			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
    			/*+(MV_U8)i; - All ports' WWN has to be same */
    		}
    		
    		/* init phy link rate */
    		for(i=0;i<8;i++)
    		{
    			/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
    			pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
#else
    			pHBA_Info_Param->PHY_Rate[i] = 0x1;/*Default 3Gbps*/
#endif

    		}

    		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

    		/* init bga rate */
    		pHBA_Info_Param->BGA_Rate = 0x7D;

    		pHBA_Info_Param->Rebuild_Rate = 0x7D;
    		pHBA_Info_Param->Init_Rate = 0x7D;
    		pHBA_Info_Param->Sync_Rate = 0x7D;

#ifdef SUPPORT_MP
    		pHBA_Info_Param->MP_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
    		pHBA_Info_Param->Copyback_Rate = 0;
#endif
#ifdef SUPPORT_MIGRATION
    		pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
    		
    		/* init setting flags */
    		pHBA_Info_Param->HBA_Flag = 0;
    		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
    		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
    		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
    		/* By default auto rebuild is enabled */
    		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
    		
    		//pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
    		if ( ((PRAID_Feature)phba_ext->p_raid_feature)->SMART_Status_Timer_Handle != NO_CURRENT_TIMER )
    			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_SMART_ON;
    		else
    			pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;	

#ifdef SUPPORT_BOARD_ALARM
    			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
#ifdef _BGA_COPY_BACK
    		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
#ifdef SUPPORT_MODULE_CONSOLIDATE
    		/* By default the module considation is enabled */
    		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

#ifdef SUPPORT_SERIAL_NUM
    		MV_DPRINT(("Add serial number.\n"));
    		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
#ifdef _OS_LINUX
    		temp = (MV_U16)(Controller.Device_Id);
    		MV_DPRINT(("%x.\n",Controller.Device_Id));
#else
    		temp = (MV_U16)(phba_ext->Device_Id);
    		MV_DPRINT(("%x.\n",phba_ext->Device_Id));

#endif
    		Itoa(buffer,temp);
    		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
    		time = HBA_GetMillisecondInDay();
    		temp = (MV_U16)time;
    		Itoa(buffer,temp);
    		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
    		temp=(MV_U16)(time>>16);
    		Itoa(buffer,temp);
    		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
    		time = HBA_GetTimeInSecond();
    		temp = (MV_U16)time;
    		Itoa(buffer,temp);
    		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);
    			
    		
#endif

    		/* set next page of HBA info Page*/
    		pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);

    		pHBA_Info_Param->Check_Sum = 0;
    		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
    		/* init the parameter in ram */
    	return MV_TRUE;
    }
#endif

#ifdef _OS_LINUX	
    HBA_GetControllerInfor( phba_ext, &Controller);
    #if defined(SUPPORT_BALDUR)
        AI.bar[5] = Controller.Base_Address[5];
    #endif
    AI.bar[2] = Controller.Base_Address[2];
#else //_OS_LINUX
    #if defined(SUPPORT_BALDUR) 
        AI.bar[5] = phba_ext->Base_Address[5];
    #endif
    #if defined(SUPPORT_MAGNI)
        AI.bar[4] = phba_ext->Base_Address[4];
    #else
        AI.bar[2] = phba_ext->Base_Address[2];
    #endif 
#endif //_OS_LINUX

#if !defined(BALDUR_FPGA)
	if (-1 == OdinSPI_Init(&AI)) {
		MV_PRINT("Init flash rom failed.\n");
		return MV_FALSE;
	}

	//MV_DPRINT(("Init flash rom ok,flash type is 0x%x.\n",AI.FlashID));

    	/* step 1 read param from flash offset = 0x3FFF00 */
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
#endif /* BALDUR_FPGA */

	/* step 2 check the signature first */
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
	{
		if (pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
#ifdef _BGA_COPY_BACK
            pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
            pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
			/* By default the module considation is enabled */
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
    		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;
		}

		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;
#ifdef SUPPORT_MIGRATION
		if(pHBA_Info_Param->Migration_Rate == 0xff)
			pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
		if(pHBA_Info_Param->Copyback_Rate == 0xff)
			pHBA_Info_Param->Copyback_Rate = 0x7D;
#endif
#ifdef SUPPORT_MP
		if(pHBA_Info_Param->MP_Rate == 0xff)
			pHBA_Info_Param->MP_Rate = 0x7D;
#endif		
		for(i=0;i<8;i++)
		{
				/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
			if(pHBA_Info_Param->PHY_Rate[i] >= 0x2)
				pHBA_Info_Param->PHY_Rate[i] = 0x2; /*6Gbps*/
#else
			if(pHBA_Info_Param->PHY_Rate[i] >= 0x1)
				pHBA_Info_Param->PHY_Rate[i] = 0x1;/*3Gbps*/
#endif
			// validate phy tuning
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[0] = 0;
			//pHBA_Info_Param->PHY_Tuning[i].Reserved[1] = 0;
		}
	}
	else
	{
	    //HBA info corrupt, fill default value.
	    
		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
		pHBA_Info_Param->Signature[0] = 'M';	
		pHBA_Info_Param->Signature[1] = 'R';
	   	pHBA_Info_Param->Signature[2] = 'V';
	  	pHBA_Info_Param->Signature[3] = 'L';


		// Set SAS address
#ifdef NEW_CORE_DRIVER
		for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++)
#else
		for(i=0;i<MAX_PHYSICAL_PORT_NUMBER;i++)
#endif
		{
			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
			pHBA_Info_Param->SAS_Address[i].b[4]=  0x1b;
			pHBA_Info_Param->SAS_Address[i].b[5]=  0x4b;
#if defined(SUPPORT_BALDUR)
			/* Baldur uses 4 phy each core, */
			/* two cores must use different SAS address*/
			pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
#else
			pHBA_Info_Param->SAS_Address[i].b[6]=  0x00;
#endif
			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
			/*+(MV_U8)i; - All ports' WWN has to be same */
		}
		
		/* init phy link rate */
		for(i=0;i<8;i++)
		{
			/* phy host link rate */
#ifdef SUPPORT_6G_PHYRATE
			pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
#else
			pHBA_Info_Param->PHY_Rate[i] = 0x1;/*Default 3Gbps*/
#endif

		}

		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

		/* init bga rate */
		pHBA_Info_Param->BGA_Rate = 0x7D;

		pHBA_Info_Param->Rebuild_Rate = 0x7D;
		pHBA_Info_Param->Init_Rate = 0x7D;
		pHBA_Info_Param->Sync_Rate = 0x7D;

#ifdef SUPPORT_MP
		pHBA_Info_Param->MP_Rate = 0x7D;
#endif
#ifdef _BGA_COPY_BACK
		pHBA_Info_Param->Copyback_Rate = 0x7D;
#endif
#ifdef SUPPORT_MIGRATION
		pHBA_Info_Param->Migration_Rate = 0x7D;
#endif
		
		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_ERROR_STOP;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
#ifdef _BGA_COPY_BACK
        pHBA_Info_Param->HBA_Flag |= HBA_FLAG_COPY_BACK_ON;
#endif
        pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
#ifdef SUPPORT_MODULE_CONSOLIDATE
		/* By default the module considation is enabled */
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_DISABLE_MOD_CONSOLIDATE;
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#ifdef SUPPORT_BOARD_ALARM
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
#endif
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SERIAL_CONSOLE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_ENABLE_SGPIO;


#ifdef SUPPORT_SERIAL_NUM
		MV_DPRINT(("Add serial number.\n"));
		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
#ifdef _OS_LINUX
		temp = (MV_U16)(Controller.Device_Id);
		MV_DPRINT(("%x.\n",Controller.Device_Id));
#else
		temp = (MV_U16)(phba_ext->Device_Id);
		MV_DPRINT(("%x.\n",phba_ext->Device_Id));

#endif
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
		time = HBA_GetMillisecondInDay();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
		temp=(MV_U16)(time>>16);
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
		time = HBA_GetTimeInSecond();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);
			
		
#endif

		/* set next page of HBA info Page*/
		pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);

#if !defined(BALDUR_FPGA)
		/* write to flash and save it now */
		if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashErase = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param);
			}
		}
#endif /* BALDUR_FPGA */
		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
		/* init the parameter in ram */

#if !defined(BALDUR_FPGA)
	//	OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
		if(OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashBad = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param_bad);
			}
		}
#endif /* BALDUR_FPGA */
	}

	return MV_TRUE;

}
#endif
#endif //#ifdef SUPPORT_ROC


/* Caution: Calling this function, please do Read-Modify-Write. 
 * Please call to get the original data first, then modify corresponding field,
 * Then you can call this function. */
#ifdef SUPPORT_ROC
MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, phba_info_page pHBA_Info_Param)
#else
MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
#endif
{
//#ifdef _OS_UKRN // XH TODO
//    MV_DPRINT(("mvuiHBA_modify_param: TOOOOOOODOOOOOOOO!\n"));
//    return MV_FALSE;
//#else // _OS_UKRN
#ifndef SUPPORT_ROC
#ifdef NEW_CORE_DRIVER
        core_extension *p_core_ext;
#else
	PCore_Driver_Extension	p_core_ext;
#endif
#ifdef  SUPPORT_PD_PAGE
	MV_U32	buf_start_addr = ODIN_FLASH_SIZE-PAGE_BUFFER_SIZE;
	MV_PVOID	p_page_buf = NULL;
	MV_PTR_INTEGER	hba_addr = 0;
	pHBA_Info_Page	p_HBA_page = NULL;
#endif
	MV_U32	param_flash_addr = PARAM_OFFSET;
	AdapterInfo				AI;
	PHBA_Extension	pHBA = NULL;

#else
#ifdef NEW_CORE_DRIVER
        core_extension *p_core_ext;
#else
	PCore_Driver_Extension	p_core_ext;
#endif
#endif

	if (!This)
		return MV_FALSE;

#ifndef SUPPORT_ROC
	pHBA = (PHBA_Extension)HBA_GetModuleExtension(This,MODULE_HBA);

#if defined(SUPPORT_MAGNI)
	if (pHBA->RunAsNonRAID == MV_FALSE)
		return MV_FALSE;
#endif

#if defined(SUPPORT_BALDUR)
	AI.bar[5] = pHBA->Base_Address[5];
#endif
	AI.bar[2] = pHBA->Base_Address[2];
#if !defined(BALDUR_FPGA)
	if (-1 == OdinSPI_Init(&AI))
		return MV_FALSE;
#endif
#endif

#ifdef SUPPORT_ROC
    if ( pHBA_Info_Param->header.signature[0]!= 'M'
        || pHBA_Info_Param->header.signature[1]!= 'R'
        || pHBA_Info_Param->header.signature[2]!= 'V'
        || pHBA_Info_Param->header.signature[3]!= 'L' ) {
        /* Shouldn't come here. If comes here, coding error. 
         * Caller should do read-modify-write to call this function. */
        MV_DASSERT( MV_FALSE );
        return MV_FALSE;
    }
#else
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	if ( pHBA_Info_Param->header.signature[0]!= 'M'
		|| pHBA_Info_Param->header.signature[1]!= 'R'
		|| pHBA_Info_Param->header.signature[2]!= 'V'
		|| pHBA_Info_Param->header.signature[3]!= 'L' ) 
#else
	if ( pHBA_Info_Param->Signature[0]!= 'M'
		|| pHBA_Info_Param->Signature[1]!= 'R'
		|| pHBA_Info_Param->Signature[2]!= 'V'
		|| pHBA_Info_Param->Signature[3]!= 'L' ) 
#endif
	{
		/* Shouldn't come here. If comes here, coding error. 
		 * Caller should do read-modify-write to call this function. */
		MV_DASSERT( MV_FALSE );
		return MV_FALSE;
	}
#endif //SUPPORT_ROC
#ifndef SUPPORT_ROC
#if !defined(BALDUR_FPGA)
	p_core_ext = HBA_GetModuleExtension(This, MODULE_CORE);
	if( p_core_ext == NULL )
		return MV_FALSE;
#ifndef NEW_CORE_DRIVER
#ifdef  SUPPORT_PD_PAGE
	p_page_buf = p_core_ext->Page_Buffer_Pointer;
	//Caution: When modify HBA Page,  PD Page part MUST be not erased
	if(!Core_FlashRead(p_core_ext, buf_start_addr, PAGE_BUFFER_SIZE, p_page_buf))
		return MV_FALSE;
#endif
#endif
#ifndef SUPPORT_RMW_FLASH
	/* write to flash and save it now */
	if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
		MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE SUCCESS!\n"));
	} else {
		MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE FAILED!\n"));
		return MV_FALSE;
	}
#endif
#endif
#else
	/* Get Core extension pointer */
	p_core_ext = HBA_GetModuleExtension(This, MODULE_CORE);
#endif

#ifdef SUPPORT_ROC
    pHBA_Info_Param->header.check_sum = 0;
    pHBA_Info_Param->header.check_sum = mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(hba_info_page));
#else
	pHBA_Info_Param->Check_Sum = 0;
	pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
#endif //SUPPORT_ROC

#ifdef NEW_CORE_DRIVER
#ifdef SUPPORT_RMW_FLASH
#ifndef SUPPORT_ROC
        if (core_rmw_write_flash(p_core_ext, PARAM_OFFSET,
                (MV_PU8)pHBA_Info_Param, sizeof(HBA_Info_Page)) == MV_FALSE) {
	        if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
		        MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE SUCCESS!\n"));
	        } else {
		        MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE FAILED!\n"));
                        MV_DASSERT(MV_FALSE);
	        }
                OdinSPI_WriteBuf(&AI, param_flash_addr, (MV_PU8)pHBA_Info_Param,
                        sizeof(HBA_Info_Page));
        }
#else /* SUPPORT_ROC */
        if ((have_flash_layout)&&(core_rmw_write_flash(p_core_ext, HBA_INFO_OFFSET,
                (MV_PU8)pHBA_Info_Param, sizeof(HBA_Info_Page)) == MV_FALSE)) {
		FM_PRINT(("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
       }
        if ((have_flash_layout)&&(core_rmw_write_flash(p_core_ext, HBA_INFO_SECONDARY_OFFSET,
                (MV_PU8)pHBA_Info_Param, sizeof(HBA_Info_Page)) == MV_FALSE)) {
		FM_PRINT(("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
       }

#endif /* SUPPORT_ROC */

#else /* SUPPORT_RMW_FLASH */
         if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
            MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE SUCCESS!\n"));
            } else {
                MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE FAILED!\n"));
                MV_DASSERT(MV_FALSE);
            }
        OdinSPI_WriteBuf(&AI, param_flash_addr, (MV_PU8)pHBA_Info_Param,
                sizeof(HBA_Info_Page));
#endif /* SUPPORT_RMW_FLASH */
#else /* NEW_CORE_DRIVER */
#if !defined(BALDUR_FPGA)
#ifdef SUPPORT_PD_PAGE
	//use buffer to write flash including PD page
#ifdef SUPPORT_ROC
	if((have_flash_layout)&&(!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), HBA_INFO_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
		FM_PRINT(("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
		return MV_FALSE;
	}
	/*Dual HBA_INFO*/
	if((have_flash_layout)&&(!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
		FM_PRINT(("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
		return MV_FALSE;
	}
#else
	hba_addr = (MV_PTR_INTEGER)p_page_buf + PAGE_BUFFER_SIZE - sizeof(HBA_Info_Page);
	p_HBA_page = (pHBA_Info_Page)hba_addr;
	MV_CopyMemory( (pHBA_Info_Page)hba_addr, pHBA_Info_Param, sizeof(HBA_Info_Page));
	Core_FlashWrite(p_core_ext, buf_start_addr, PAGE_BUFFER_SIZE, p_page_buf);
#endif
#else
#ifdef SUPPORT_ROC
	if((have_flash_layout)&&(!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), HBA_INFO_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
		FM_PRINT(("%s %d %s ... Write Flash for Primary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
		return MV_FALSE;
	}
	/*Dual HBA_INFO*/
	if((have_flash_layout)&&(!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), HBA_INFO_SECONDARY_OFFSET, FLASH_PARAM_SIZE, (MV_PU8)pHBA_Info_Param, 0))) {
		FM_PRINT(("%s %d %s ... Write Flash for Secondary HBA_INFO is Fail....\n", __FILE__, __LINE__, __FUNCTION__));
		return MV_FALSE;
	}
#else
	OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);
#endif /* SUPPORT_ROC */
#endif /* SUPPORT_PD_PAGE */
#endif
#endif /* NEW_CORE_DRIVER */
#ifdef SUPPORT_ROC
	/*Keep HBA_Info to RAM*/
	MV_CopyMemory(p_hba_info_buf, pHBA_Info_Param, FLASH_PARAM_SIZE);
#endif
	return MV_TRUE;
//#endif
}

MV_U8	mvCalculateChecksum(MV_PU8	Address, MV_U32 Size)
{
		MV_U8 checkSum;
		MV_U32 temp=0;
        checkSum = 0;
        for (temp = 0 ; temp < Size ; temp ++)
        {
                checkSum += Address[temp];
        }
        
        checkSum = (~checkSum) + 1;



		return	checkSum;
}

MV_U8 mvVerifyChecksum(MV_PU8	Address, MV_U32 Size)
{
    MV_U8	checkSum = 0;
    MV_U32 	temp = 0;

    for (temp = 0 ; temp < Size ; temp ++) {
        checkSum += Address[temp];
    }

    return checkSum;
}

MV_BOOLEAN mv_page_signature_check( MV_PU8 Signature )
{
	if(	Signature[0] == 'M'&& \
		Signature[1] == 'R'&& \
		Signature[2] == 'V'&& \
		Signature[3] == 'L')
		return MV_TRUE;
	else
		return MV_FALSE;
	
}

#ifdef SCSI_RW_BUFFER_CMD
MV_U64 mv_flash_image_checksum(MV_PU64	Address, MV_U32 Size)
{
	MV_U64	checkSum;
	MV_U32 	temp=0;
	U64_ZERO_VALUE(checkSum);
        for (temp = 0 ; temp < Size ; temp ++)
        {
            U64_ASSIGN_U64(checkSum,U64_ADD_U64(checkSum,Address[temp]));
        }
 #ifdef FLASH_DEBUG
        FLASH_DPRINT("Y.L %s ...Address[0x%x,0x%x],Size[0x%x],checkSum[0x%x,0x%x]\n", __FUNCTION__,Address->parts.high,Address->parts.low,Size,checkSum.parts.high,checkSum.parts.low);
#endif
	return checkSum;
}

MV_BOOLEAN core_flash_operation_flash_data(
	MV_VOID * p_core_ext,
	MV_U8		buf_id,
	MV_U32		offset,
	MV_U32		alloc_len,
	MV_PVOID	p_buffer,
	MV_U8		opCode
	)
{
	MV_U16	i = 0,loop =0;
	MV_U32	flash_start_addr = 0x00000000;
	flash_start_addr = buf_id*MAX_FLASH_BUFFER_CAPACITY+offset;
	MV_PU8		tmp_buf = (MV_PU8)p_buffer;
	
#ifdef FLASH_DEBUG
	MV_PU64		p_temp_buffer = NULL;
	MV_PU64		p_temp_buffer_top = NULL;
	MV_PTR_INTEGER	buffer_addr;
	FM_PRINT("Y.L %s ...buffer_id[%d],offset[0x%x],alloc_len[0x%x],opCode[0x%x]\n", __FUNCTION__,buf_id,offset,alloc_len,opCode);
	FM_PRINT("Y.L %s ...flash_start_addr=0x%x\n", __FUNCTION__,flash_start_addr);

	if(buf_id==99||buf_id==63){
		buffer_addr = ((MV_PTR_INTEGER)(p_buffer)+MAX_FLASH_BUFFER_CAPACITY*sizeof(MV_U8)-sizeof(MV_U64));
		p_temp_buffer = (MV_PU64)buffer_addr;
		FM_PRINT("Y.L DEBUG LAST BUFFER WRITE %s ...buffer_addr=0x%x\n", __FUNCTION__,buffer_addr);
		FM_PRINT("Y.L DEBUG LAST BUFFER WRITE %s ...generation[high:0x%x; low:0x%x]\n", __FUNCTION__,p_temp_buffer->parts.high,p_temp_buffer->parts.low);
		FM_PRINT("Y.L DEBUG LAST BUFFER WRITE %s ...*******************end ******************\n", __FUNCTION__);
	}
#endif
	if(alloc_len>(MAX_FLASH_BUFFER_CAPACITY-offset))
		alloc_len = MAX_FLASH_BUFFER_CAPACITY-offset;
	
	// read flash
	if(opCode==0x00){
#ifdef SUPPORT_ROC
		if (!Core_NVMRd(NULL, MFR_FLASH_DEV(0), flash_start_addr, alloc_len, (MV_PU8)p_buffer, 0)) {
			FM_PRINT("%s %d %s ... Read Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
			return FLASH_READ_ERR;
		}
#else
		if(!Core_FlashRead(p_core_ext, flash_start_addr, alloc_len, (MV_PU8)p_buffer))
			return FLASH_READ_ERR;
#endif
	}
	// write flash
	else if(opCode==0x01){
        if (g_update_raw) {
            //Update RAW image, it erase whole flash in previous flow.
            //if (!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), flash_start_addr, alloc_len, (MV_PU8)tmp_buf, MVFR_FLAG_RMW_BUF)) {
            if (!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), flash_start_addr, alloc_len, (MV_PU8)tmp_buf, MVFR_FLAG_AUTO)) {
                FM_PRINT("%s %d %s ... Write Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
                return FLASH_WRITE_ERR;
            }
        } else {
            FM_PRINT("%s g_update_raw %x, buf_id %x\n", __FUNCTION__, g_update_raw, buf_id);
            //JL, Erase whole firmware at 1st write
            if ((buf_id == 0x20) || (buf_id == 0x40)) {
                //Erase generation & gen CRC area first
                if (!Core_NVMEs( p_core_ext, MFR_FLASH_DEV(0), flash_start_addr + FLASH_FIRMWARE_SIZE - GENERATION_SIZE, GENERATION_SIZE, NULL, 0)) {
                    FM_PRINT("%s %d %s ... Erase Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
                    return FLASH_WRITE_ERR;
                }
                if (!Core_NVMEs( p_core_ext, MFR_FLASH_DEV(0), flash_start_addr, FLASH_FIRMWARE_SIZE, NULL, 0)) {
                    FM_PRINT("%s %d %s ... Erase Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
                    return FLASH_WRITE_ERR;
                }
            }

            if (!Core_NVMWr( p_core_ext, MFR_FLASH_DEV(0), flash_start_addr, alloc_len, (MV_PU8)tmp_buf, 0)) {
                FM_PRINT("%s %d %s ... Write Flash is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
                return FLASH_WRITE_ERR;
            }
        }
	}
#ifdef FLASH_DEBUG
	if(opCode==0x00){
		FM_PRINT("Y.L DEBUG BUFFER READ %s ...flash_start_addr=0x%x,buf_id=%d\n", __FUNCTION__,flash_start_addr,buf_id);
		FM_PRINT("Y.L DEBUG BUFFER READ %s ...alloc_len=0x%x\n", __FUNCTION__,alloc_len);
		FM_PRINT("Y.L DEBUG BUFFER READ %s ...offset=0x%x\n", __FUNCTION__,offset);
		p_temp_buffer_top = (MV_PU64)(p_buffer);
		FM_PRINT("Y.L DEBUG BUFFER READ %s %d %s ...buffer_top64[high:0x%x; low:0x%x]\n", __FUNCTION__,p_temp_buffer_top->parts.high,p_temp_buffer_top->parts.low);
	}
	
	if((buf_id==99||buf_id==63)&&(opCode==0x00)){
		FM_PRINT("Y.L DEBUG LAST BUFFER READ %s ...flash_start_addr=0x%x,buf_id=%d\n", __FUNCTION__,flash_start_addr,buf_id);
		FM_PRINT("Y.L DEBUG LAST BUFFER READ %s ...alloc_len=0x%x\n", __FUNCTION__,alloc_len);
		FM_PRINT("Y.L DEBUG LAST BUFFER READ %s ...offset=0x%x\n", __FUNCTION__,offset);
		buffer_addr = ((MV_PTR_INTEGER)(p_buffer)+MAX_FLASH_BUFFER_CAPACITY*sizeof(MV_U8)-sizeof(MV_U64));
		p_temp_buffer = (MV_PU64)buffer_addr;
		FM_PRINT("Y.L FW DEBUG LAST BUFFER READ %s ...generation[high:0x%x; low:0x%x]\n", __FUNCTION__,p_temp_buffer->parts.high,p_temp_buffer->parts.low);
	}
#endif
	return FLASH_NONE_ERR;

}
MV_VOID core_flash_get_flashbuf_offset_boundary(
	MV_VOID * p_core_ext,
	MV_PVOID 	p_buf_desc
	)
{
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ...\n", __FUNCTION__);
#endif
	PMV_READ_BUFFER_DESCRIPTOR p_read_buf_desc = (PMV_READ_BUFFER_DESCRIPTOR) p_buf_desc;
	p_read_buf_desc->offsetBoundary = MAX_FLASH_BUFFER_OFFSET_BOUNDARY;// 1K
	p_read_buf_desc->capacityHigh =(MV_U8)(MAX_FLASH_BUFFER_CAPACITY>>16 & 0xFF);
	p_read_buf_desc->capacityMiddle =(MV_U8)((MAX_FLASH_BUFFER_CAPACITY>>8)&0xFF);
	p_read_buf_desc->capacityLow = (MV_U8)(MAX_FLASH_BUFFER_CAPACITY&0xFF);
#ifdef FLASH_DEBUG
	FM_PRINT("Y.L %s ...offset_bound[0x%x],capacity[0x%x,%x,%x]\n", __FUNCTION__,p_read_buf_desc->offsetBoundary,p_read_buf_desc->capacityHigh,p_read_buf_desc->capacityMiddle,p_read_buf_desc->capacityLow);
#endif
}
#ifdef FLASH_IMAGE_HEADER_CHECK 
MV_BOOLEAN core_flash_image_data_crc_check(
	MV_VOID* p_core_ext,
	MV_PVOID header,
	MV_U8	last_buffer_id,
	MV_U32	last_buffer_offset,
	MV_PVOID last_buffer	
	)
{
	MV_U16	i = 0;
	MV_U8	buffer[MAX_FLASH_BUFFER_CAPACITY];
	PFLASH_IMAGE_HEADER p_tmp_header = NULL;
	MV_U8 	total_ids = 0;
	MV_U8 	start_id = 0;
	MV_U32	flash_start_addr = 0x00000000;
	MV_U64 	checkSum;
	MV_U32	crc = 0;

	p_tmp_header = (PFLASH_IMAGE_HEADER)header;
	total_ids = (p_tmp_header->image_length>>MAX_FLASH_BUFFER_CAPACITY_BIT_POWER);
	start_id = last_buffer_id+1 - total_ids;
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ...p_tmp_header->image_length[%x],last_buffer_id[%d],last_buffer_offset[0x%x]\n", __FUNCTION__,p_tmp_header->image_length, last_buffer_id,last_buffer_offset);
#endif
	MV_DASSERT(start_id>=0);
	MV_DASSERT(last_buffer_id>start_id);
	U64_ZERO_VALUE(checkSum);
#ifdef FLASH_DEBUG
	FLASH_DPRINT("Y.L %s ...start_id[0x%x],last_buffer_id[%d],last_buffer_offset[0x%x]\n", __FUNCTION__,start_id,last_buffer_id,last_buffer_offset);
#endif
	for (i = start_id; i <= last_buffer_id; i++) {
		// Read image
		flash_start_addr = i*MAX_FLASH_BUFFER_CAPACITY;
		MV_ZeroMemory(&buffer[0], MAX_FLASH_BUFFER_CAPACITY * sizeof(MV_U8));
#ifdef SUPPORT_ROC
		if (!Core_NVMRd(NULL, MFR_FLASH_DEV(0), flash_start_addr, MAX_FLASH_BUFFER_CAPACITY, (MV_PU8)&buffer, 0)) {
			FM_PRINT("%s ... Read Flash is Fail....\n", __FUNCTION__);
			return MV_FALSE;
		}
#else
		if(!Core_FlashRead(p_core_ext, flash_start_addr, MAX_FLASH_BUFFER_CAPACITY, (MV_PU8)&buffer))
			return MV_FALSE;
#endif
		// calculate CRC
		if (i==start_id)
			crc = MV_CRC_LOOP((MV_PU8)&buffer, MAX_FLASH_BUFFER_CAPACITY, 0, MV_TRUE);
		else if (i == last_buffer_id) {
			if (last_buffer == NULL)
				crc = MV_CRC_LOOP((MV_PU8)&buffer, (MAX_FLASH_BUFFER_CAPACITY - GENERATION_SIZE - IMAGE_CRC_SIZE), crc, MV_FALSE);
			else
				crc = MV_CRC_LOOP((MV_PU8)last_buffer, (MAX_FLASH_BUFFER_CAPACITY - GENERATION_SIZE - IMAGE_CRC_SIZE), crc, MV_FALSE);
		} else
			crc = MV_CRC_LOOP((MV_PU8)&buffer, MAX_FLASH_BUFFER_CAPACITY, crc, MV_FALSE);
	}
    
	// compare CRC
	if (p_tmp_header->crc != crc) {
		FM_PRINT("%s %d %s ... flash image's data is not valid, header crc =[0x%x], calculated crc=[0x%x] ....\n", __FILE__, __LINE__, __FUNCTION__,p_tmp_header->crc ,crc);
		return MV_FALSE;
	}

#ifdef FLASH_DEBUG
	FM_PRINT("%s ... flash image's data is valid....\n", __FUNCTION__);
#endif
	return MV_TRUE;
		
}

MV_BOOLEAN core_flash_image_header_check(
	MV_PVOID 			p_buffer,
	PFLASH_IMAGE_HEADER	tmp_header
	)
{
	PFLASH_IMAGE_HEADER p_header = (PFLASH_IMAGE_HEADER)p_buffer;
	if(!core_flash_image_header_signature_check(p_header->signature))
		return MV_FALSE;

	if(!core_flash_image_header_product_check(p_header->adapter_device_id))
		return MV_FALSE;

	MV_CopyMemory(tmp_header, p_header, sizeof(FLASH_IMAGE_HEADER));
	return MV_TRUE;
}

MV_BOOLEAN core_flash_image_header_signature_check(
	MV_PU8	signature
	)
{
	if (signature[0] == 'M'&& \
		signature[1] == 'V'&& \
		signature[2] == '_'&& \
		signature[3] == 'F'&& \
		signature[4] == 'L'&& \
		signature[5] == 'A'&& \
		signature[6] == 'S'&& \
		signature[7] == 'H')
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOLEAN core_flash_image_header_product_check(
	MV_U16	device_id
	)
{
    if ((device_id == DEVICE_ID_9580) || (device_id == DEVICE_ID_9548) || 
        (device_id == DEVICE_ID_LACIE_8BIG) || (device_id == DEVICE_ID_LACIE_5BIG))
        return MV_TRUE;
    else
        return MV_FALSE;
}

#endif /*#ifdef FLASH_IMAGE_HEADER_CHECK */

#endif  /*#ifdef SCSI_RW_BUFFER_CMD*/
#endif	/* #ifdef DISABLE_SPI*/
#endif
