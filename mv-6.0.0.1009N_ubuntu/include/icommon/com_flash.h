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

#ifndef __MV_COM_FLASH_H__
#define __MV_COM_FLASH_H__

#include "com_define.h"
#include "com_event_struct.h"

#define DRIVER_LENGTH                      1024*16

#ifdef   SUPPORT_CONFIG_FILE
#define   AUTOLOAD_FLASH_OFFSET			0x38
#ifdef SUPPORT_ATHENA
#define   AUTOLOAD_FLASH_LENGTH			1024
#else
#define   AUTOLOAD_FLASH_LENGTH			0x108
#endif
#define	RAID_BIOS_ROM_SIZE				(64*1024L)
#define   RELOCATE_ROM_SIZE					0x1000
#define   ROM_IMAGE_4K_CHECKSUM 			0x36  //byte
#endif


#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _Flash_DriverData
{
	MV_U16            Size;
	MV_U8             PageNumber;
	MV_BOOLEAN        isLastPage;
	MV_U16            Reserved[2];
	MV_U8             Data[DRIVER_LENGTH];
}
Flash_DriveData, *PFlash_DriveData;

#if defined(SAVE_EVENTS_TO_FLASH)
//#define EVENT_FLASH_SECTOR_SIZE (128 * 1024)
#define EVENT_FLASH_SECTOR_SIZE (MX29LV640_MAIN_SECTOR_SIZE)
#define MAX_FLASH_EVENT_PER_BLOCK	(EVENT_FLASH_SECTOR_SIZE / sizeof(struct flash_event_log_entry))

#define MAX_FLASH_EVENT_LOG_ENTRY	(((EVENT_FLASH_SECTOR_SIZE * 2) / sizeof(struct flash_event_log_entry)) - 2) /*A block is 128K in Flash. Event Log will use 2 blocks. two will be to use the event log header(first and last, they will in different block)*/
#define EVENT_LOG_VERSION 0x000010C

/*The size of flash_event_log_entry equal to flash_event_log_head*/
struct flash_event_log_entry
{
#if defined(ROC_V2_EVENT_SUPPORT)
 	DriverEvent_V2 event;
#else
	DriverEvent event;
#endif
	MV_U32 crc;

	MV_U8    valid;
	MV_U8    reserved0[7];

#if defined(ROC_V2_EVENT_SUPPORT)
	MV_U8  	reserved1[52]; 
#else
	MV_U8  	reserved1[84]; 
#endif
       /*please to keep total is 128 bytes.*/
};

struct flash_event_log_head
{
	MV_U8	signature[4];
	MV_U32	next_tbl_off;
	MV_U16	version;
	MV_U16  	nr_max_entries;
	MV_U32  	crc;
	MV_U8  	reserved[112];
       /*please to keep total is 128 bytes.*/
};
#endif /*SAVE_EVENTS_TO_FLASH*/

#if defined(ADDING_FLASH_LAYOUT_DESC)
struct flash_layout_item
{
        MV_U16    module_type;
        MV_U16    id;

        MV_U32    reserved0[1];
        MV_U32    start_address;
        MV_U32    size;
};


struct flash_layout_head
{
        MV_U8      signature[8];

        MV_U32    next_tbl_off;

        MV_U16    version;
        MV_U16    reserved0[1];

        MV_U32    length;

        MV_U32    crc;

        MV_U32    reserved1[2];

        struct flash_layout_item item[1];
};
#define FLASH_LAYOUT_SIGN "MRVL_MAP"
#define FLASH_LAYOUT_SIGN_LENGTH MV_StrLen(FLASH_LAYOUT_SIGN)
//#define FLASH_LAYOUT_OFFSET	return_image_place_on_flash(FLASH_IMAGE_LAYOUTDESC , 0, FLASH_IMAGE_RETURN_ADDRESS)
#define FLASH_LAYOUT_SIZE		0x00400000
#define FLASH_LAYOUT_START_ADDR	0x00000000
#define FLASH_LAYOUT_OFFSET		0x00100000

/*The u-boot Start address to be decide by Flash Size(Please check Flash Library).
   To got the SingleBlock and FlashSize by the Flash Library(if have any concern, please check Flash Library).*/

/* Note : all images in description layout must be 32k multiplied, if using the image update or backup feature*/
/*Note 1: The Layout String had been removed. Now it will be put by MVFlash or Windows Loader.*/
#if 0
#define FLASH_LAYOUT_DESC 	"Loki(NOR FLASH Layout) 1 BLOCK = 128K\
			\n{\"BIOS\",0,[0x00000000],[32k],[1 Block]},\
			\n{\"HBA.txt\",0,[0x000020000],[32k],[1 Block]},\
			\n{\"HBA_Info\",0,[0x00040000],[32k],[1 Block]},\
			\n{\"PD_Page\",0,[0x00060000],[128k],[1 Block]},\
			\n{\"Event_Log\",0,[0x00080000],[256k],[2 Blocks]},\
			\n{\"Desc.txt\",0,[0x000c0000],[32k],[1 Block]},\
			\n{\"HBA_Info\",1,[0x000e0000],[32k],[1 Block]},\
			\n{\"Firmware\",0,[0x00100000],[1024k],[4 Blocks]},\
			\n{\"Firmware\",1,[0x00200000],[1024k],[4 Blocks]},\
			\n{\"u-boot\",0,[0x00ff0000],[192k],[2 Block]},\
			\n{\"SingleBlock\",0,[0xffffffff],[128k],[1 Block]},\
			\n{\"FlashSize\",0,[0xffffffff],[16M],[128 Blocks]};"
#endif

enum {
    FLASH_IMAGE_CONFIG = 0x000000000, /*The config had merged the HBA.txt and Layout Description */
    FLASH_IMAGE_HBAINFO,
    FLASH_IMAGE_FMLOADER,
    FLASH_IMAGE_FIRMWARE,
    FLASH_IMAGE_BIOS,
    FLASH_IMAGE_EVENTLOG,
    FLASH_IMAGE_PDINFOPAGE,
    FLASH_IMAGE_SINGLEBLOCK,
    FLASH_IMAGE_FLASHSIZE,
    FLASH_IMAGE_RAW,
    FLASH_IMAGE_MAX
};

#define FLASH_IMAGE_RETURN_ADDRESS 0
#define FLASH_IMAGE_RETURN_SIZE    1

/*
	BIOS,     			[0x00000000],[128k]
	HBA.txt, 			[0x000020000],[128k]
	HBA_Info Primary,	[0x00040000],[128k]
	PD Page,			[0x00060000],[128k]
	Event Log,		[0x00080000],[256k]
	Desc.txt,			[0x000C0000],[128k]
	HBA_Info Secondary[0x000e0000],[128k]
	Firmware Secondary[0x00100000],[1024k]
	Firmware,			[0x00220000],[1024k]
	u-boot,			[0x00FD0000],[128k]

*/

#define FLASH_BOOTLOADER_ADDR           0x00010000
#define FLASH_BOOTLOADER_SIZE           0x00010000
#define FLASH_PROD_STS_ADDR             0x00030000
#define FLASH_PROD_STS_SIZE             0x00010000
#define FLASH_BIOS_ADDR                 0x00040000
#define FLASH_BIOS_SIZE                 0x00020000
#define FLASH_FIRMWARE_ADDR             0x00100000
#define FLASH_FIRMWARE_SECOND_ADDR      0x00200000
#define FLASH_FIRMWARE_SIZE				0x00100000
#define FLASH_HBA_CONFIG_ADDR			0x003E0000
#define FLASH_HBA_CONFIG_SECOND_ADDR    0x003F0000
#define FLASH_HBA_CONFIG_SIZE           0x00010000
#define FLASH_RAW_IMAGE_ADDR            0x00000000
#define FLASH_RAW_IMAGE_SIZE            0x00800000

#define IMAGE_UPDATE	FLASH_UPLOAD
#define IMAGE_BACKUP	FLASH_DOWNLOAD

#ifdef SCSI_RW_BUFFER_CMD
#define GENERATION_SIZE	8
#define IMAGE_CRC_SIZE	4

/* flash operation error type define
0: FLASH_NONE_ERR
1: FLASH_WRITE_ERR
2: FLASH_READ_ERR
3: GENERATION_ERR
4: FATAL_GENERATION_ERR
5: IMAGE_DATA_ERR
6: IMAGE_HEADER_ERR
7: IMAGE_HEADER_AND_DATA_ERR
*/
#define    FLASH_NONE_ERR					0
#define    FLASH_WRITE_ERR					1
#define    FLASH_READ_ERR					2
#define    GENERATION_ERR					3
#define    FATAL_GENERATION_ERR			4 // dual image's genreation all crashed
#define    IMAGE_DATA_ERR					5 // single image's generation crashed
#define    IMAGE_HEADER_ERR					6
#define    IMAGE_HEADER_AND_DATA_ERR		7

#define FLASH_DPRINT	 FM_PRINT

#define RESTORE_GENERATION(gen)	do{gen.parts.high = 0xFFFFFFFF; gen.parts.low = 0xFFFFFFFF;}while(0)

#ifdef FLASH_IMAGE_HEADER_CHECK
static MV_U32	global_header_check = 1;
#else
static MV_U32	global_header_check = 0;
#endif
void flash_err_event_generate(MV_VOID * p_core, MV_U8 err_type);
#endif

MV_U32 return_image_place_on_flash(MV_U32 component_type, MV_U16 component_number, MV_U16 component_return_type);

#endif

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif /* __MV_COM_FLASH_H__ */
