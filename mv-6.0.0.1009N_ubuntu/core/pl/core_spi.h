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

#if !defined( _CORE_SPI_H_ )
#define _CORE_SPI_H_

#include "core_header.h"

#ifdef _MVFLASH_
#   include "stdio.h"
#   define SPI_DBG( _X_ )       printf _X_
#elif defined(_OS_WINDOWS) || defined(_OS_LINUX) || defined(__QNXNTO__) || defined(_OS_FIRMWARE)
#   define SPI_DBG( _X_ )       MV_DPRINT( _X_ )
#   define EEPROM_DBG( _X_ )    MV_DPRINT( _X_ )
#endif /* _MVFLASH_ */

#define AT25F2048               0x0101
#define AT25DF041A               0x0102
#define AT25DF021               0x0103
#define PM25LD010               0x2110

#define MX25L2005               0x0201
#define MX25L4005               0x0202
#define MX25L8005               0x0203
#define MX25V8006               0x0204
#define W25X40					0x0301
#define EN25F20					0x0401
#define SST25VF040B					0x0501
#define M25PX80                 0x0601

#ifndef _OS_WINDOWS
#define SPI_CTRL_REG            0xc0
#define SPI_CMD_REG             0xc4
#define SPI_DATA_REG            0xc8
#endif

#define DEFAULT_PAGE_SIZE   256 //bytes
//#define FLASH_DEVICE_SIZE       0x80000
#define FLASH_DEVICE_SIZE    g_SPIFlashInfo.size
#define HBAINFO_OFFSET          FLASH_DEVICE_SIZE - DEFAULT_PAGE_SIZE   //512k-256 bytes=524032


// copy the page code define from magni FW
#define PAGE_CODE_HBA_INFO	0
#define PAGE_CODE_PD_INFO	1
#define PAGE_CODE_AES_INFO	2
#define PAGE_CODE_EVT_INFO	3
#define PAGE_CODE_CRYPTO_PD 4
#define PAGE_CODE_PHY_INFO  0x05 // reference to Magni FW code and HBA INFO spec.
#define NULL_PAGE_NUMBER    0xff


#define SPI_INS_WREN            0
#define SPI_INS_WRDI            1
#define SPI_INS_RDSR            2
#define SPI_INS_WRSR            3
#define SPI_INS_READ            4
#define SPI_INS_PROG            5
#define SPI_INS_SERASE          6
#define SPI_INS_CERASE          7
#define SPI_INS_RDID            8
#define SPI_INS_PTSEC            9
#define SPI_INS_UPTSEC            10
#define SPI_INS_RDPT            11

#define DEFAULT_SPI_FLASH_SIZE          256L * 1024
#define DEFAULT_SPI_FLASH_SECT_SIZE     64L * 1024

typedef struct SPIFlashInfo_
{
    MV_U8    id_count;
    MV_U8    id[15];
    MV_U32  size;
    MV_U32  sect_size;
    MV_U32  block_size;
    MV_U8    CmdSet[16];
}SPIFlashInfo;

extern SPIFlashInfo g_SPIFlashInfo;

typedef struct _AdapterInfo AdapterInfo;
struct _AdapterInfo
{
    MV_U32      flags;
    MV_U16      devId;
    MV_U32      classCode;
    MV_U8       revId;
    MV_U8       bus;
    MV_U8       devFunc;
    void*       bar[6];
    MV_U32      ExpRomBaseAddr;
    MV_U8       version;
    MV_U8       subVersion;

    MV_U16      FlashID;
    MV_U32      FlashSize; 
    MV_U32      FlashSectSize;
};

typedef union 
{
    struct 
    {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];
} SAS_ADDRESS, *PSAS_ADDRESS;

#define MAX_BOOTDEVICE_SUPPORTED        8

#pragma pack(1)
typedef struct _HBA_Info_Main
{

	MV_U8 signature[4];	//offset 0x0h,4bytes,structure signature
	 
	MV_U16 image_size;	//offset 0x4h,2bytes,512bytes per indecate romsize

	MV_U16 option_rom_checksum;	//offset 0x6h,2 bytes,16BIT CHECKSUM OPTIONROM caculation

	MV_U8  major;	//offset 0x8h,1 byte,BIOS major version

	MV_U8  minor;	//offset 0x9h,1byte,BIOS minor version

	MV_U8  oem_num;	//offset 0xah,1byte,OEM number

	MV_U8  build_version;	//offset 0xbh,1byte,build number

	MV_U32  hba_flag;	/* offset 0xch,4 bytes,
 				     HBA flags:  refers to HBA_FLAG_XX
 					bit 0   --- HBA_FLAG_BBS_ENABLE
					bit 1   --- HBA_FLAG_SUPPORT_SMART
					bit 2   --- HBA_FLAG_ODD_BOOT
					bit 3   --- HBA_FLAG_INT13_ENABLE
					bit 4   --- HBA_FLAG_ERROR_STOP
					bit 5   --- HBA_FLAG_ERROR_PASS
					bit 6   --- HBA_FLAG_SILENT_MODE_ENABLE
				*/

	MV_U16 boot_order[MAX_BOOTDEVICE_SUPPORTED];	//offset 0x10h,2bytes*Max_bootdevice_supported,BootOrder[0] = 3,means the first bootable device is device 3

	MV_U8 bootablecount;	//bootable device count
	
	MV_U8  serialnum[20];	//offset 0x20h,20bytes,serial number

	MV_U8  chip_revision;	//offset 0x34h,1 byte,chip revision

	SAS_ADDRESS   sas_address[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];	//offset ,8bytes*MAX_PHYSICAL_PORT_NUMBER

	MV_U8  phy_rate[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];	//offset ,1byte*MAX_PHYSICAL_PORT_NUMBER,0:  1.5 GB/s, 1: 3.0 GB/s

    MV_U32  bootdev_wwn[8]; //this is initial as 0xFFFF,FFFF

	MV_U8  reserve[97];		//reserve space for future

	MV_U8  checksum;	//offset 0xFF,checksum for this structure
}HBA_Info_Main, *pHBA_Info_Main;
#pragma pack()

int
OdinSPI_SectErase
(
    AdapterInfo *pAI,
    MV_U32      addr
);
int
OdinSPI_ChipErase
(
    AdapterInfo *pAI
);
int
OdinSPI_Write
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      Data
);
int
OdinSPI_WriteBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
);
int
OdinSPI_Read
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U8       Size
);
int
OdinSPI_ReadBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
);
int
OdinSPI_RMWBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count,
    MV_U8       *Buf
);
int
OdinSPI_Init
(
    AdapterInfo *pAI
);
int
LoadHBAInfo
(
    AdapterInfo *pAI,
    HBA_Info_Main *pHBAInfo
);

int
OdinSPI_Write_SST
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U8*      Data, 
    MV_U32      Count
);
#endif
