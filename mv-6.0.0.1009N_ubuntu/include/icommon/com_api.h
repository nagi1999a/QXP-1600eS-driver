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

#ifndef  __MV_COM_API_H__
#define  __MV_COM_API_H__

#define APICDB15_VARIABLE_SIZE_REQUEST			1

/* CDB definitions */
#define APICDB0_ADAPTER                        0xF0
#define APICDB0_LD                             0xF1
#define APICDB0_BLOCK                          0xF2
#define APICDB0_PD                             0xF3
#define APICDB0_EVENT                          0xF4
#define APICDB0_DBG                            0xF5
#define APICDB0_FLASH                          0xF6
#define APICDB0_AES                            0xF7

#define APICDB0_IOCONTROL		 			   0xF9
#define APICDB0_PASS_THRU_CMD_SCSI			   0xFA
#define APICDB0_PASS_THRU_CMD_ATA			   0xFB
#define APICDB0_PASS_THRU_COMMON			   0xFD

/* Pass through for CDB[16].  The actually CDB is in the first 16 bytes of the input buffer. */
#define APICDB0_PASS_THRU_CMD_SCSI_16          0xFC

#define APICDB0_EXTENSION                      0xFF		// reserved, used only when 0xF0 to 0xFE above are all used up!

/* for IOCONTROL */
#define APICDB1_GET_OS_DISK_INFO		0
#define APICDB1_SET_OS_DISK_QUEUE_DEPTH	1
#define APICDB1_SET_OS_DISK_QUEUE_TYPE	2

/* for Adapter */
#define APICDB1_ADAPTER_GETCOUNT               0
#define APICDB1_ADAPTER_GETINFO                (APICDB1_ADAPTER_GETCOUNT + 1)
#define APICDB1_ADAPTER_GETCONFIG              (APICDB1_ADAPTER_GETCOUNT + 2)
#define APICDB1_ADAPTER_SETCONFIG              (APICDB1_ADAPTER_GETCOUNT + 3)
#define APICDB1_ADAPTER_POWER_STATE_CHANGE     (APICDB1_ADAPTER_GETCOUNT + 4)
#define APICDB1_ADAPTER_BBU_INFO               (APICDB1_ADAPTER_GETCOUNT + 5)
#define APICDB1_ADAPTER_BBU_SET_THRESHOLD      (APICDB1_ADAPTER_GETCOUNT + 6)
#define APICDB1_ADAPTER_BBU_POWER_CHANGE       (APICDB1_ADAPTER_GETCOUNT + 7)
#define APICDB1_ADAPTER_AES_GETCONFIG          (APICDB1_ADAPTER_GETCOUNT + 8)
#define APICDB1_ADAPTER_AES_SETCONFIG          (APICDB1_ADAPTER_GETCOUNT + 9)
#define APICDB1_ADAPTER_BBU_SETSTATUS_DBG      (APICDB1_ADAPTER_GETCOUNT + 10)// only for test BBU function
#define APICDB1_ADAPTER_MUTE                   (APICDB1_ADAPTER_GETCOUNT + 11)
#define APICDB1_ADAPTER_ROOT_RESET             (APICDB1_ADAPTER_GETCOUNT + 12)
#define APICDB1_ADAPTER_SET_HOST_BUFFER        (APICDB1_ADAPTER_GETCOUNT + 13)
#define APICDB1_ADAPTER_READ_REGISTER          (APICDB1_ADAPTER_GETCOUNT + 14)
#define APICDB1_ADAPTER_WRITE_REGISTER         (APICDB1_ADAPTER_GETCOUNT + 15)
#define APICDB1_ADAPTER_OEM_GET_DATA           (APICDB1_ADAPTER_GETCOUNT + 16)
#define APICDB1_ADAPTER_OEM_SET_DATA           (APICDB1_ADAPTER_GETCOUNT + 17)
#define APICDB1_ADAPTER_GET_SENSOR_DATA        (APICDB1_ADAPTER_GETCOUNT + 18)
#define APICDB1_ADAPTER_OEM_GET_INFO           (APICDB1_ADAPTER_GETCOUNT + 19)
#define APICDB1_ADAPTER_CREATE_VD              (APICDB1_ADAPTER_GETCOUNT + 20)
#define APICDB1_ADAPTER_DELETE_VD              (APICDB1_ADAPTER_GETCOUNT + 21)
#define APICDB1_ADAPTER_MAX                    (APICDB1_ADAPTER_GETCOUNT + 22)

#define APICDB1_ADAPTER_BBU_CHARGE_THRESHOLD   (APICDB1_ADAPTER_GETCOUNT + 6)

#define APICDB2_ADAPTER_POWER_CHANGE_LEAVING_S0   0
#define APICDB2_ADAPTER_POWER_CHANGE_ENTER_S0     1
#define APICDB2_ADAPTER_POWER_CHANGE_SHUTDOWN     2

/* for LD */
#define APICDB1_LD_CREATE                      0
#define APICDB1_LD_GETMAXSIZE                  (APICDB1_LD_CREATE + 1)
#define APICDB1_LD_GETINFO                     (APICDB1_LD_CREATE + 2)
#define APICDB1_LD_GETTARGETLDINFO             (APICDB1_LD_CREATE + 3)
#define APICDB1_LD_DELETE                      (APICDB1_LD_CREATE + 4)
#define APICDB1_LD_GETSTATUS                   (APICDB1_LD_CREATE + 5)
#define APICDB1_LD_GETCONFIG                   (APICDB1_LD_CREATE + 6)
#define APICDB1_LD_SETCONFIG                   (APICDB1_LD_CREATE + 7)
#define APICDB1_LD_STARTREBUILD                (APICDB1_LD_CREATE + 8)	// single target rebuild on VD only (see APICDB1_LD_DG_STARTREBUILD)
#define APICDB1_LD_STARTCONSISTENCYCHECK       (APICDB1_LD_CREATE + 9)
#define APICDB1_LD_STARTINIT                   (APICDB1_LD_CREATE + 10)
#define APICDB1_LD_STARTMIGRATION              (APICDB1_LD_CREATE + 11)
#define APICDB1_LD_BGACONTROL                  (APICDB1_LD_CREATE + 12)
#define APICDB1_LD_WIPEMDD                     (APICDB1_LD_CREATE + 13)
#define APICDB1_LD_GETSPARESTATUS              (APICDB1_LD_CREATE + 14)
#define APICDB1_LD_SETGLOBALSPARE              (APICDB1_LD_CREATE + 15)
#define APICDB1_LD_SETLDSPARE                  (APICDB1_LD_CREATE + 16)
#define APICDB1_LD_REMOVESPARE                 (APICDB1_LD_CREATE + 17)
#define APICDB1_LD_HD_SETSTATUS                (APICDB1_LD_CREATE + 18)
#define APICDB1_LD_SHUTDOWN                    (APICDB1_LD_CREATE + 19)
#define APICDB1_LD_HD_FREE_SPACE_INFO          (APICDB1_LD_CREATE + 20)
#define APICDB1_LD_HD_GETMBRINFO               (APICDB1_LD_CREATE + 21)
#define APICDB1_LD_SIZEOF_MIGRATE_TARGET       (APICDB1_LD_CREATE + 22)
#define APICDB1_LD_TARGET_LUN_TYPE			   (APICDB1_LD_CREATE + 23)
#define APICDB1_LD_HD_START_MEDIAPATROL        (APICDB1_LD_CREATE + 24)
#define APICDB1_LD_HD_RESERVED				   (APICDB1_LD_CREATE + 25)
#define APICDB1_LD_HD_GET_RCT_COUNT            (APICDB1_LD_CREATE + 26)
#define APICDB1_LD_HD_RCT_REPORT               (APICDB1_LD_CREATE + 27)
#define APICDB1_LD_HD_START_DATASCRUB          (APICDB1_LD_CREATE + 28)
#define APICDB1_LD_HD_BGACONTROL	           (APICDB1_LD_CREATE + 29)
#define APICDB1_LD_HD_GETBGASTATUS             (APICDB1_LD_CREATE + 30)
#define APICDB1_LD_CREATE_MODIFY_DG			   (APICDB1_LD_CREATE + 31)	// Create or modify Disk Group (DG)
#define APICDB1_LD_CREATE_MODIFY_VD			   (APICDB1_LD_CREATE + 32)	// Create or modify VD using DG
#define APICDB1_LD_DELETE_DG				   (APICDB1_LD_CREATE + 33)	// Delete DG
#define APICDB1_LD_GET_DG_INFO				   (APICDB1_LD_CREATE + 34)	// Get DG info
#define APICDB1_LD_DG_STARTREBUILD			   (APICDB1_LD_CREATE + 35) // Start rebuild on VD or DG with one or multiple target (see APICDB1_LD_STARTREBUILD)
#define APICDB1_LD_DG_STARTCONSISTENCYCHECK    (APICDB1_LD_CREATE + 36) // Start sync on DG
#define APICDB1_LD_DG_STARTINIT                (APICDB1_LD_CREATE + 37)	// Start init on DG
#define APICDB1_LD_DG_STARTMIGRATION           (APICDB1_LD_CREATE + 38) // Start migration on DG
#define APICDB1_LD_DG_BGACONTROL               (APICDB1_LD_CREATE + 39) // BGA control on DG
#define APICDB1_LD_RESERVED1                   (APICDB1_LD_CREATE + 40) // Reserved, can be re-used!
#define APICDB1_LD_SET_DG_SPARE                (APICDB1_LD_CREATE + 41)
#define APICDB1_LD_DG_GETSETTING               (APICDB1_LD_CREATE + 42)
#define APICDB1_LD_DG_SETSETTING               (APICDB1_LD_CREATE + 43)
#define APICDB1_LD_COPYBACK                    (APICDB1_LD_CREATE + 44) // Currently internal used by RAID core
#define APICDB1_LD_IMPORT                      (APICDB1_LD_CREATE + 45)
#define APICDB1_LD_GET_WORKING_LD 		       (APICDB1_LD_CREATE + 46)
#define APICDB1_LD_REPORTED_LD		           (APICDB1_LD_CREATE + 47) // used to report the LD to OS
#define APICDB1_LD_GETCROSSINFO		           (APICDB1_LD_CREATE + 48)
#define APICDB1_LD_GETHYPERFREEINFO            (APICDB1_LD_CREATE + 49)
#define APICDB1_LD_FS_ENTRIES                  (APICDB1_LD_CREATE + 50)
#define APICDB1_LD_REMOVE_ENTRIES_TABLE        (APICDB1_LD_CREATE + 51)
#define APICDB1_LD_GET_ENTRIES_TABLE           (APICDB1_LD_CREATE + 52)
#define APICDB1_LD_HYPERDUO_MAP_SYSTEM         (APICDB1_LD_CREATE + 53)
#define APICDB1_HYPERDUO_GET_MAP_SYSTEM        (APICDB1_LD_CREATE + 54)
#define APICDB1_HYPERDUO_REMOVE_ALL_MAP_SYSTEM (APICDB1_LD_CREATE + 55)
#define APICDB1_LD_GET_HDD_LINK                (APICDB1_LD_CREATE + 56)
#define APICDB1_LD_ADD_HDD_LINK                (APICDB1_LD_CREATE + 57)
#define APICDB1_LD_GET_TARGET_INFO             (APICDB1_LD_CREATE + 58)
#define APICDB1_LD_OEM                         (APICDB1_LD_CREATE + 59)
#define APICDB1_LD_MAX                         (APICDB1_LD_CREATE + 60)

/* for PD */
#define APICDB1_PD_GETHD_INFO                  0
#define APICDB1_PD_GETEXPANDER_INFO            (APICDB1_PD_GETHD_INFO + 1)
#define APICDB1_PD_GETPM_INFO                  (APICDB1_PD_GETHD_INFO + 2)
#define APICDB1_PD_GETSETTING                  (APICDB1_PD_GETHD_INFO + 3)
#define APICDB1_PD_SETSETTING                  (APICDB1_PD_GETHD_INFO + 4)
#define APICDB1_PD_BSL_DUMP                    (APICDB1_PD_GETHD_INFO + 5)
#define APICDB1_PD_GETENCLOSURE_INFO		   (APICDB1_PD_GETHD_INFO + 6)
#define APICDB1_PD_RESERVED2				   (APICDB1_PD_GETHD_INFO + 7)	// not used
#define APICDB1_PD_GETSTATUS                   (APICDB1_PD_GETHD_INFO + 8)
#define APICDB1_PD_GETHD_INFO_EXT              (APICDB1_PD_GETHD_INFO + 9)	// APICDB1_PD_GETHD_INFO extension
#define APICDB1_PD_VERIFY_DISK                 (APICDB1_PD_GETHD_INFO + 10)
#define APICDB1_PD_FORMATUNIT_DISK             (APICDB1_PD_GETHD_INFO + 11)
#define APICDB1_PD_GET_PERCENTAGE              (APICDB1_PD_GETHD_INFO + 12)
#define APICDB1_PD_FORCE_ONLINE				   (APICDB1_PD_GETHD_INFO + 13) // cdb[2] is did;  force disk online that be offlined by AES protection method.
#define APICDB1_PD_SECURITY_FEATURE		       (APICDB1_PD_GETHD_INFO + 14)
#define APICDB1_PD_OEM                         (APICDB1_PD_GETHD_INFO + 15)
#define APICDB1_PD_MAX						   (APICDB1_PD_GETHD_INFO + 16)

/* Sub command for APICDB1_PD_SETSETTING */
#define APICDB4_PD_SET_WRITE_CACHE_OFF         0
#define APICDB4_PD_SET_WRITE_CACHE_ON          1
#define APICDB4_PD_SET_SMART_OFF               2
#define APICDB4_PD_SET_SMART_ON                3
#define APICDB4_PD_SMART_RETURN_STATUS         4
#define APICDB4_PD_SET_SPEED_3G				   5
#define APICDB4_PD_SET_SPEED_1_5G			   6
#define APICDB4_PD_GET_SMART_INFO			   7

/* for Block */
#define APICDB1_BLOCK_GETINFO                  0
#define APICDB1_BLOCK_HD_BLOCKIDS              (APICDB1_BLOCK_GETINFO + 1)
#define APICDB1_BLOCK_MAX                      (APICDB1_BLOCK_GETINFO + 2)

/* for event */
#define APICDB1_EVENT_GETEVENT                 0
#define APICDB1_HOST_GETEVENT                  1
#define APICDB1_EVENT_MAX                      (APICDB1_HOST_GETEVENT + 1)

/* for DBG */
#define APICDB1_DBG_PDWR                       0
#define APICDB1_DBG_MAP						   (APICDB1_DBG_PDWR + 1)
#define APICDB1_DBG_LDWR					   (APICDB1_DBG_PDWR + 2)
#define APICDB1_DBG_ADD_RCT_ENTRY			   (APICDB1_DBG_PDWR + 3)
#define APICDB1_DBG_REMOVE_RCT_ENTRY		   (APICDB1_DBG_PDWR + 4)
#define APICDB1_DBG_REMOVE_ALL_RCT			   (APICDB1_DBG_PDWR + 5)
#define APICDB1_DBG_ADD_CORE_ERROR			   (APICDB1_DBG_PDWR + 6)
#define APICDB1_DBG_REMOVE_CORE_ERROR		   (APICDB1_DBG_PDWR + 7)
#define APICDB1_DBG_REMOVE_ALL_CORE_ERROR	   (APICDB1_DBG_PDWR + 8)
#define APICDB1_DBG_GET_CORE_ERROR		       (APICDB1_DBG_PDWR + 9)

#define APICDB1_DBG_ABORT_TASK                 (APICDB1_DBG_PDWR + 10)
#define APICDB1_DBG_SMP_PHY_CTL				   (APICDB1_DBG_PDWR + 11)
#define APICDB1_DBG_MAX						   (APICDB1_DBG_PDWR + 12)


#define DBG_REQUEST_READ						MV_BIT(0)
#define DBG_REQUEST_WRITE						MV_BIT(1)
#define DBG_REQUEST_VERIFY						MV_BIT(2)

#define DBG_INJECT_TYPE_NORMAL				MV_BIT(0)
#define DBG_INJECT_TYPE_HW				MV_BIT(1)

/* for FLASH */
#define APICDB1_FLASH_BIN                      0
#define APICDB1_ERASE_FLASH	                   (APICDB1_FLASH_BIN + 1)
#define APICDB1_TEST_FLASH                     (APICDB1_FLASH_BIN + 2)
#define APICDB1_BUFFER_DETAIL_FLASH            (APICDB1_FLASH_BIN + 3)
#define APICDB1_FLASH_NVSRAM                   (APICDB1_FLASH_BIN + 4) // WD request
#define APICDB1_FLASH_PRE_DATA                 (APICDB1_FLASH_BIN + 5) // WD request
#define APICDB1_FLASH_CMD_RW_BUF            (APICDB1_FLASH_BIN + 6)
#define APICDB1_FLASH_MAX                      (APICDB1_FLASH_BIN + 7)

/* for AES */
#define APICDB1_AES_INFO          		0 //For Magni used only
#define APICDB1_AES_SET_LINK		    (APICDB1_AES_INFO + 1)
#define APICDB1_AES_CLEAR_LINK          (APICDB1_AES_INFO + 2)
#define APICDB1_AES_GETPORTCONFIG		(APICDB1_AES_INFO + 3)  
#define APICDB1_AES_SETPORTCONFIG		(APICDB1_AES_INFO + 4)  
#define APICDB1_AES_GETENTRYCONFIG		(APICDB1_AES_INFO + 5) //For Magni used only
#define APICDB1_AES_SETENTRYCONFIG		(APICDB1_AES_INFO + 6)  
#define APICDB1_AES_VERIFY_KEY          (APICDB1_AES_INFO + 7) //For Magni used only
#define APICDB1_AES_CHANGE_PASSWORD     (APICDB1_AES_INFO + 8)
#define APICDB1_AES_PORT_INFO           (APICDB1_AES_INFO + 9)  /*For Magni-Plus*/
#define APICDB1_AES_KEY_ENTRY_INFO          (APICDB1_AES_INFO + 10)  /*For Magni-Plus*/

/* for passthru commands
	Cdb[0]: APICDB0_PASS_THRU_CMD_SCSI or APICDB0_PASS_THRU_CMD_ATA
	Cdb[1]: APICDB1 (Data flow)
	Cdb[2]: TargetID MSB
	Cdb[3]: TargetID LSB
	Cdb[4]-Cdb[15]: SCSI/ATA command is embedded here
		SCSI command: SCSI command Cdb bytes is in the same order as the spec
		ATA Command:
			Features = pReq->Cdb[0];
			Sector_Count = pReq->Cdb[1];
			LBA_Low = pReq->Cdb[2];
			LBA_Mid = pReq->Cdb[3];
			LBA_High = pReq->Cdb[4];
			Device = pReq->Cdb[5];
			Command = pReq->Cdb[6];

			if necessary:
			Feature_Exp = pReq->Cdb[7];
			Sector_Count_Exp = pReq->Cdb[8];
			LBA_Low_Exp = pReq->Cdb[9];
			LBA_Mid_Exp = pReq->Cdb[10];
			LBA_High_Exp = pReq->Cdb[11];
*/
#define APICDB0_PASS_THRU_CMD_SCSI			      0xFA
#define APICDB0_PASS_THRU_CMD_ATA				  0xFB

#define APICDB1_SCSI_NON_DATA					  0x00
#define APICDB1_SCSI_PIO_IN					  0x01 // goes with Read Long
#define APICDB1_SCSI_PIO_OUT					  0x02 // goes with Write Long

#define APICDB1_ATA_NON_DATA					  0x00
#define APICDB1_ATA_PIO_IN						  0x01
#define APICDB1_ATA_PIO_OUT				         0x02

#define API_SCSI_CMD_RCV_DIAG_RSLT			  0x1C
#define API_SCSI_CMD_SND_DIAG					  0x1D

#define API_GENERIC						0xD1
#define APICDB0_PHY                          		(API_GENERIC + 1)
#define APICDB0_I2C 						(API_GENERIC + 2)
#define APICDB0_NVSRAM					(API_GENERIC + 3)
#define APICDB0_SGPIO 					(API_GENERIC + 4)
#define APICDB0_BUZZER 					(API_GENERIC + 5)
#define APICDB0_LED						(API_GENERIC + 6)
#define APICDB0_BIST					(API_GENERIC + 7)

#define APICDB1_PHY       					 0
#define APICDB1_PHY_TEST		    		 (APICDB1_PHY + 1)
#define APICDB1_PHY_STATUS		    		 (APICDB1_PHY + 2)
#define APICDB1_PHY_CONFIG                   (APICDB1_PHY + 3)
#define APICDB1_PHY_SET_CONFIG               (APICDB1_PHY + 4)
#define APICDB1_PHY_GET_CONFIG               (APICDB1_PHY + 5)
#define APICDB1_PHY_CLEAR_SHOW_PHY_ERROR_COUNT             (APICDB1_PHY + 6)

#define APICDB1_I2C_READ_EEPROM		0

#define APICDB1_SGPIO               			0
#define APICDB1_SGPIO_TEST                  	(APICDB1_SGPIO + 1)

#define APICDB1_BUZZER 					0
#define APICDB1_BUZZER_OFF				(APICDB1_BUZZER + 1)
#define APICDB1_BUZZER_ON				(APICDB1_BUZZER + 2)
#define APICDB1_BUZZER_MUTE				(APICDB1_BUZZER + 3)

#define APICDB1_LED						0
#define APICDB1_LED_ACT_OFF			(APICDB1_LED + 1)
#define APICDB1_LED_ACT_ON				(APICDB1_LED + 2)
#define APICDB1_LED_FLT_OFF			(APICDB1_LED + 3)
#define APICDB1_LED_FLT_ON				(APICDB1_LED + 4)

#define APICDB1_BIST      					0
#define APICDB1_BIST_TEST		    	 	(APICDB1_BIST + 1)

#define APICDB2_SSD_INIT 				0xF0
#define APICDB2_SSD_SAFECODE_MODE	(APICDB2_SSD_INIT + 1)
#define APICDB2_SSD_DO_RESET			(APICDB2_SSD_INIT + 2)
#define APICDB2_SSD_NORMAL_MODE		(APICDB2_SSD_INIT + 3)
#define APICDB2_SSD_DETECT_PHY_READY (APICDB2_SSD_INIT + 4)

#define APICDB1_ATA_PD                      0
#define APICDB1_ATA_SCSI                    (APICDB1_ATA_PD + 1)
#define APICDB1_ATA_MAX                     (APICDB1_ATA_PD + 2)

#endif /*  __MV_COM_API_H__ */

