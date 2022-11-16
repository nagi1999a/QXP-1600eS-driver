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

#ifndef __MV_COM_PHYSICAL_LINK_H__
#define __MV_COM_PHYSICAL_LINK_H__

#include "com_define.h"

//Physical Link
#define DEVICE_TYPE_NONE                        0
#define DEVICE_TYPE_HD                          1       //  DT_DIRECT_ACCESS_BLOCK
#define DEVICE_TYPE_PM                          2
#define DEVICE_TYPE_EXPANDER                    3		// DT_EXPANDER
#ifndef _OS_BIOS
#define DEVICE_TYPE_TAPE						4		// DT_SEQ_ACCESS
#endif
#define DEVICE_TYPE_PRINTER						5		// DT_PRINTER
#define DEVICE_TYPE_PROCESSOR					6		// DT_PROCESSOR
#define DEVICE_TYPE_WRITE_ONCE					7 		// DT_WRITE_ONCE
#define DEVICE_TYPE_CD_DVD						8		// DT_CD_DVD
#define DEVICE_TYPE_OPTICAL_MEMORY				9 		// DT_OPTICAL_MEMORY
#define DEVICE_TYPE_MEDIA_CHANGER				10		// DT_MEDIA_CHANGER
#define DEVICE_TYPE_ENCLOSURE					11		// DT_ENCLOSURE
#define DEVICE_TYPE_I2C_ENCLOSURE				12		
#define DEVICE_TYPE_PORT                        0xFF	// DT_STORAGE_ARRAY_CTRL

#define MAX_WIDEPORT_PHYS                       8

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */
typedef struct _Link_Endpoint 
{
 MV_U16      DevID;
 MV_U8       DevType;         /* Refer to DEVICE_TYPE_xxx */
 MV_U8       PhyCnt;          /* Number of PHYs for this endpoint. Greater than 1 if it is wide port. */
 MV_U8       PhyID[MAX_WIDEPORT_PHYS];    /* Assuming wide port has max of 8 PHYs. */
 MV_U8       SAS_Address[8];  /* Filled with 0 if not SAS device. */
 MV_U16      EnclosureID;     // enclosure ID of this device if available, otherwise 0xFFFF
 MV_U8       Reserved[6];
} Link_Endpoint, * PLink_Endpoint;

typedef struct _Link_Entity 
{
 Link_Endpoint    Parent;
 MV_U8            Reserved[8];
 Link_Endpoint    Self;
} Link_Entity,  *PLink_Entity;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif
