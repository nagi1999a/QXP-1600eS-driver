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

#ifndef __MV_PRODUCT_ATHENA_H__
#define __MV_PRODUCT_ATHENA_H__

#define BALDUR_FPGA  /*Nancy: from bring up driver, need sync about the flash access*/
#define SUPPORT_ATHENA	/* fushun: only support athena16 in branch code */
#define MAX_MULTI_QUEUE	8

/*hardware related*/
#define ODIN_DRIVER                      1
#define SUPPORT_BALDUR			1
#define RUN_AS_PCIE_DRIVER
#define SUPPORT_6G_PHYRATE	1
#define ROOT_WAITING_QUEUE

/*Work around for hardware*/


#if 1//def SUPPORT_ATHENA
#define ATHENA_A0_WORKAROUND
#define ATHENA_A1_WORKAROUND
#define ATHENA_MICRON_DETECT_WA
//#define ATHENA2_Z1_WIDE_PORT_MULTI_PORT_ISSUE		//for z1 workaround
#define SUPPORT_PHY_POWER_MODE		
//#define REWRITE_READ_POINTER_WORKAROUND			//for z1 workaround
//#define ATHENA_DISABLE_NCQ
//#define ATHENA_TEST_PRD_SKIP
//#define ATHENA_TEST_PIR

//#define SUPPORT_SECURITY_KEY_RECORDS
//#define ATHENA_SECTOR_BY_SECTOR
//#define ATHENA_SECTOR_BY_SECTOR_SAS

//#define ATHENA_TEST_WORKAROUND
//#define SUPPORT_ATHENA_DONT_ACCESS_BAR1
//#define ATHENA_FPGA_WORKAROUND
#define ATHENA_XOR_DELV_SHADOW_ENABLE
#define ATHENA_XOR_CMPL_Q_SHADOW_ENABLE

#define ATHENA_DELV_SHADOW_ENABLE
#define ATHENA_CMPL_Q_SHADOW_ENABLE
#ifdef ATHENA_CMPL_Q_SHADOW_ENABLE
#define IGNORE_READ_REGISTER
#endif
//#define ATHENA_IGNORE_OVERFLOW_ERR
//#define REPLACE_READ_WRITE_TO_VERITY
//#define SUPPORT_PIR
#define ATHENA_SATA_PHY_UNSTABLE_WA
//#define ATHENA_Z1_PM_WA
//#define SUPPORT_DPP
//#define SUPPORT_INTL_PARITY
//#define DISABLE_PHY_1_2_3

//#define SUPPORT_QUICK_PM_INIT	1
#endif

#ifdef SUPPORT_BALDUR
#define NOT_SPINUP_WORKAROUND
//        #define XOR_PI_TABLE                    1
//        #define PRD_DATA_SKIP                   1
//        #define DDR_MEM_SUPPORT                 1
        #define XOR_COALESCE_INT                1
#ifdef ATHENA2_Z1_WIDE_PORT_MULTI_PORT_ISSUE
        #define CORE_WIDEPORT_LOAD_BALACE_WORKAROUND
#endif
	//#define	DEBUG_PCI_E_ERROR
#endif

/*Software workaround and new features*/
//#define I2C_NAK20_WORKAROUND			1
//#define SUPPORT_MULTIPATH				1
#ifndef SKIP_INTERNAL_INITIALIZE
#define CORE_SUPPORT_LARGE_REQUEST
#endif

#define CORE_SUPPORT_HBA_PAGE
#define SUPPORT_READ_LOG_EXT
#ifndef RAID_DRIVER
//#define HAVE_HW_COMPLIANT_SG
#endif

/*product based raid/xor feature*/
#ifdef RAID_DRIVER
#define SUPPORT_RAID1E                  1
#define HARDWARE_XOR
//#define SOFTWARE_XOR
#ifndef SOFTWARE_XOR
//        #define XOR_LOAD_BALANCE                1
#define XOR_USE_SG_PTR                  1
#endif
/*XOR*/
//#define XOR_LOKI_MISSED_ISR_WORKAROUND          1
//#define LOKI_MISSED_ISR_WORKAROUND              1
//#define CORE_FORCED_ISR_WORKAROUND              1
#endif

/* driver capabilities */
#define MAX_BASE_ADDRESS                6
#ifdef CORE_SUPPORT_LARGE_REQUEST
#ifdef RAID_DRIVER
#define MV_MAX_TRANSFER_SIZE            128*1024 //Nancy: org 256*1024, need check /*can't be larger than 512k*/
#else
#define MV_MAX_TRANSFER_SIZE            4*1024*1024
#endif
#define MAX_SG_ENTRY                    130





#else
#define MV_MAX_TRANSFER_SIZE            (128*1024)
#define MAX_SG_ENTRY                    34
#endif
#define MAX_SG_ENTRY_REDUCED            16
#define MV_MAX_PHYSICAL_BREAK           (MAX_SG_ENTRY - 1)

#if (defined(WIN_NONRAID) || defined(LINUX_NONRAID))
#define MAX_REQUEST_NUMBER_PERFORMANCE	    4096
#ifndef SUPPORT_OEM_PROJECT
#define MAX_REQUEST_PER_LUN_PERFORMANCE		128 // set 128, sometimes SAS disk will task full, if performance ok, can try 64. TBD.
#else
#define MAX_REQUEST_PER_LUN_PERFORMANCE		8 // when supporting max 1024 drives. 
#endif

#else
#define MAX_REQUEST_NUMBER_PERFORMANCE	     1024
#endif
#ifdef SUPPORT_OEM_PROJECT
#define MV_MAX_TARGET_NUMBER            1024
#define MAX_EXPANDER_SUPPORTED		      20
#else
#define MV_MAX_TARGET_NUMBER            256
#define MAX_EXPANDER_SUPPORTED		      10
#endif

/* hardware capabilities */
#define MAX_PM_SUPPORTED                      16 //8
#define MAX_BLOCK_PER_HD_SUPPORTED            8
#define MAX_DEVICE_SUPPORTED_WHQL             8
#define MAX_DEVICE_SUPPORTED_PERFORMANCE      MV_MAX_TARGET_NUMBER
#define MAX_DEVICE_SUPPORTED_RAID             64

/*
define for Flash operation
*/
//#define SUPPORT_FLASH_ROM	1
/*support flash update command in core_api*/
//#define SUPPORT_FLASH        1
#ifdef SUPPORT_FLASH_ROM
        #define SUPPORT_RMW_FLASH       1
	#define SUPPORT_BOARD_ALARM		1
#endif


/* for PD Page support */
#ifdef RAID_DRIVER
#ifndef SCSI_ID_MAP
//#define SUPPORT_PD_PAGE					1
#endif
#ifdef SUPPORT_PD_PAGE
	#define SUPPORT_KEEP_SCSI_ID		1
#endif
#endif	

/* for PD Write Cache Setting saved in Flash (PD Page) */
#define SUPPORT_SAVE_CACHE_IN_FLASH	1
//#define SUPPORT_ERASE_PD_PAGE			1
//#define    SUPPORT_CONFIG_FILE                 1
//#define EXP_ABORT_REQ_DURING_BROADCAST	1

#ifndef _OS_LINUX
#ifdef RAID_DRIVER
/*support NVSRAM memory to save transaction log*/
#define SUPPORT_TRANSACTION_LOG			1
#endif

#else
#define	DISABLE_VSR_IRQ_PHY_TIMEOUT				1
#endif
//#define DISABLE_SPI
//#define SUPPORT_I2C                     1
#define SUPPORT_SES                     1
#ifndef SKIP_INTERNAL_INITIALIZE
#define SUPPORT_SGPIO					1
#endif
#ifdef SUPPORT_SGPIO
#define SUPPORT_SGPIO_DATA_IN			1
#define SUPPORT_SGPIO_ACTIVE_LED		1
#endif
//#define SUPPORT_CSMI                    1
#if defined(SUPPORT_I2C)&&defined(SUPPORT_SES)
#define SUPPORT_I2C_SES					1
#endif
#define SAS_12G_SSD_QUEUE_DEPTH_WA
#define SUPPORT_ACTIVE_CABLE

#endif/*__MV_PRODUCT_VANIR_H__*/
