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

#ifndef __MV_COM_FLASH_STRUCT_H__
#define __MV_COM_FLASH_STRUCT_H__

#include "com_define.h"

#define 	 FLASH_DOWNLOAD                 0xf0
#define	 FLASH_UPLOAD                       0xf

//for read/write flash test command
#define  FLASH_BYTE_WRITE           0
#define  FLASH_BYTE_READ            1

#define    FLASH_TYPE_CONFIG           0
#define    FLASH_TYPE_BIN              1
#define    FLASH_TYPE_BIOS             2
#define    FLASH_TYPE_FIRMWARE         3
#define    FLASH_TYPE_AUTOLOAD         4	
#define    FLASH_TYPE_NVSRAM           5
#define    FLASH_TYPE_RAW              6 // the data is an image with header. as same as firmware. design for autoload,configure. oem only
#define    FLASH_TYPE_HBA		       7// Using for support cmd -a update -t rawfiles
#define    FLASH_TYPE_RawFiles	       8 // Using for support cmd -a update -t rawfiles
#define    FLASH_TYPE_FIRMWARE_BACKUP  9 // the data is an image with header. as same as firmware. design for autoload,configure. oem only
#define    FLASH_TYPE_MAX              FLASH_TYPE_FIRMWARE_BACKUP // for api check

#define 	FLASH_ERASE_PAGE                      0x1  //Erase bios or PD page or hba info page
#define	FLASH_PD_PAGE					1	// Erase PD page in flash memory but not in-uses PD id 
#define	FLASH_PD_PAGE_FORCE			254	// Force to erase the whole PD page even PD id is in-use. Used by manufacturing only!

typedef struct _MV_FLASH_PER_DATA{
	MV_U8    image_type;
	MV_U8    reserved[3];
	MV_U32   image_size;
} MV_FLASH_PER_DATA, *PMV_FLASH_PER_DATA;

#endif
