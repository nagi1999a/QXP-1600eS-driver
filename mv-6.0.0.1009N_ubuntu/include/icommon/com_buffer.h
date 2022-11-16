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

#ifndef __MV_COM_BUFFER_H__
#define __MV_COM_BUFFER_H__
#if !defined(MAGNI_PLUS)
#include "com_define.h"
#endif
/*
from document spc4r16.pdf
READ BUFFER command (6.16) to download data (BIOS, firmware) from driver
WRITE BUFFER command (6.39) to upload data (BIOS, firmware) to driver
*/
#ifndef _OS_BIOS
#pragma pack(8)
#endif /*  _OS_BIOS */
  typedef struct _MV_BUFFER_DETAIL {
	MV_U8      PathID;
	MV_U8      TargetID;
	MV_U8      Lun;
	MV_U8      VirtualID;
	MV_U8      totalSizeHigh;     			//third byte of 24 bits  total size  [16-23] bit
	MV_U8      totalSizeMiddle;  			//second byte of 24 bits  total size  [8-15] bit
	MV_U8      totalSizeLow;      			//first byte of 24 bits  total size [0-7] bit
	MV_U8      startBufferID;
	MV_U8      startBufferOffsetHigh;    	// third byte of 24 bits  total size  [16-23] bit
	MV_U8      startBufferOffsetMiddle; 	// second byte of 24 bits  total size  [8-15] bit
	MV_U8      startBufferOffsetLow;     	// first byte of 24 bits  total size  [0-7] bit
	MV_U8      reserved[5];
	MV_U64    Generation;
  } MV_BUFFER_DETAIL,*PMV_BUFFER_DETAIL;

#define    BUFFER_TYPE_START            0
#define    BUFFER_TYPE_CONFIG           0 // HBA info Page
#define    BUFFER_TYPE_BIN              1
#define    BUFFER_TYPE_BIOS             2 //BIOS
#define    BUFFER_TYPE_FIRMWARE         3 //firmware
#define    BUFFER_TYPE_AUTOLOAD         4 //
#define    BUFFER_TYPE_NVSRAM           5
#define    BUFFER_TYPE_RAW              6
#define    BUFFER_TYPE_MAX              7

//table 181 READ BUFFER command

typedef struct _MV_READ_BUFFER_COMMAND{
	MV_U8    operationCode;
#ifdef __MV_BIG_ENDIAN__
        MV_U8    reserved:3;
        MV_U8    mode:5;                                //mode code
#else
	MV_U8    mode:5;                    		//mode code
	MV_U8    reserved:3;
#endif
	MV_U8    bufferID;
	MV_U8    bufferOffsetHigh;            	//third byte of 24 bits buffersize   [16-23] bit
	MV_U8    bufferOffsetMiddle;         	//second byte of 24 bits buffersize  [8-15] bit
	MV_U8    bufferOffsetLow;             	//first byte of 24 bits buffersize   [0-7] bit
	MV_U8    allocationOffsetHigh;       	//third byte of 24 bits allocation offset  [16-23] bit
	MV_U8    allocationOffsetMiddle;      	//second byte of 24 bits allocation offset [8-15] bit
	MV_U8    allocationOffsetLow;         	//first byte of 24 bits allocation offset  [0-7] bit
	MV_U8    control;
} MV_READ_BUFFER_COMMAND;


//table 182 READ BUFFER mode field
#define READ_BUFFER_MODE_DATA                 0x2
#define READ_BUFFER_MODE_DESCRIPTOR           0x3

//table 181 READ BUFFER command

typedef struct _MV_READ_BUFFER_DESCRIPTOR{
	MV_U8    offsetBoundary;
	MV_U8    capacityHigh;              //third byte of 24 bits capacity   [16-23] bit
	MV_U8    capacityMiddle;           //second byte of 24 bits capacity  [8-15] bit
	MV_U8    capacityLow;               //first byte of 24 bits capacity   [0-7] bit
} MV_READ_BUFFER_DESCRIPTOR,*PMV_READ_BUFFER_DESCRIPTOR;


#define READ_BUFFER_COMMAND_OPERATION          0x3c

//table 196 WRITE BUFFER command
typedef struct _MV_WRITE_BUFFER_COMMAND
{
	MV_U8     operationCode;
#ifdef __MV_BIG_ENDIAN__
        MV_U8     reserved:3;
        MV_U8     mode:5;
#else
	MV_U8     mode:5;
	MV_U8     reserved:3;
#endif
	MV_U8     bufferID;
	MV_U8     bufferOffsetHigh;            	//third byte of 24 bits buffersize   [16-23] bit
	MV_U8     bufferOffsetMiddle;         	//second byte of 24 bits buffersize  [8-15] bit
	MV_U8     bufferOffsetLow;            	//first byte of 24 bits buffersize   [0-7] bit
	MV_U8     parameterListHigh;             //third byte of 24 bits parameterList offset  [16-23] bit
	MV_U8     parameterListMiddle;         //second byte of 24 bits parameterList offset [8-15] bit
	MV_U8     parameterListLow;             //first byte of 24 bits parameterList offset  [0-7] bit
	MV_U8     control;
}MV_WRITE_BUFFER_COMMAND;
#ifndef _OS_BIOS
#pragma pack()
#endif /*  _OS_BIOS */

#define WRITE_BUFFER_COMMAND_OPERATION		0x3b

//table 197 WRITE BUFFER MODE field
#define WRITE_BUFFER_MODE_DATA	0x2
#define MAX_FLASH_BUFFER_OFFSET_BOUNDARY		0x0a  // 1K
#define MAX_FLASH_BUFFER_CAPACITY_BIT_POWER	15
// 1 Buffer is 32KB, current flash size is 8MB
#define MAX_FLASH_BUFFER_ID						256
#define MAX_FLASH_BUFFER_CAPACITY	MV_BIT(MAX_FLASH_BUFFER_CAPACITY_BIT_POWER) //32K
#define FLASH_IMAGE_SIGNATURE "MV_FLASH"

#pragma pack(1)
typedef struct _FLASH_IMAGE_HEADER{
	MV_U8	signature[8];
	MV_U8	type;
	MV_U8	version_major;
	MV_U8	version_minor;
	MV_U8	version_oem;
	MV_U8	version_build_beta;
	MV_U8	version_build_alpha;
	MV_U32	image_length;
	MV_U16	adapter_device_id;
	MV_U8	reserved[40];
	MV_U32	crc;
}FLASH_IMAGE_HEADER,*PFLASH_IMAGE_HEADER;
#pragma pack()

typedef struct _FLASH_IMAGE_GENERATION{
	MV_U32	value;
	MV_U32	crc;
}FLASH_IMAGE_GENERATION,*PFLASH_IMAGE_GENERATION;

#ifdef SCSI_RW_BUFFER_CMD
MV_BOOLEAN core_flash_operation_flash_data(
	MV_VOID * p_core_ext,
	MV_U8		buf_id,
	MV_U32		offset,
	MV_U32		alloc_len,
	MV_PVOID	p_buffer,
	MV_U8		opCode
	);
MV_VOID core_flash_get_flashbuf_offset_boundary(
	MV_VOID *p_core_ext,
	MV_PVOID 			p_buf_desc
	);

#ifdef FLASH_IMAGE_HEADER_CHECK
MV_BOOLEAN core_flash_image_data_crc_check(
	MV_VOID *p_core_ext,
	MV_PVOID 			header,
	MV_U8				last_buffer_id,
	MV_U32				last_buffer_offset,
	MV_PVOID			last_buffer
	);
MV_BOOLEAN core_flash_image_header_check(
	MV_PVOID 			p_buffer,
	PFLASH_IMAGE_HEADER	tmp_header
	);
MV_BOOLEAN core_flash_image_header_signature_check(
	MV_PU8	signature
	);
#endif /* FLASH_IMAGE_HEADER_CHECK */

#endif/* SCSI_RW_BUFFER_CMD */
#endif
