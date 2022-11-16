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

#ifndef __MV_COM_AES_STRUCT_H__
#define __MV_COM_AES_STRUCT_H__

#include "com_define.h"

#define MAX_SUPPORT_AES_SATA_PORT	4 /*4:9230, 9236 2:9220*/
#define AES_KEY_SPI_ENTRY_NUMBER    10 // total entry number for user use

#define AES_KEY_CONTENT_LENGTH                 32

#define MV_AES_KEY_CLEAR                   0
#define MV_AES_KEY_128                     16
#define MV_AES_KEY_256                     32
#define MV_AES_KEY_PERSIST                 0xFF

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _aes_entry_info
{
    MV_U8 aes_port[MAX_SUPPORT_AES_SATA_PORT][2]; // supply current port id and aes entry
    MV_U8 entry[AES_KEY_SPI_ENTRY_NUMBER]; // available / not available
    MV_U8 reserved[2];
}aes_entry_info,*Paes_entry_info;

typedef struct _entry_link
{
    MV_U8       port_id;
    MV_U8       entry;
    MV_U8       reserved[6];                            // for 64 bit align
    MV_U8       user_auth[AES_KEY_CONTENT_LENGTH];      // user authentication key 32 bytes.
} entry_link, *Pentry_link;

typedef struct _AES_Entry_Config {
    MV_U8       entry_id;                               //SPI entry 0~9
    MV_U8       key_length;                             //0: remove, 16: 128bit, 32: 256bit, 0xFF: no change
    MV_U8       reserved[6];                            // for 64 bit align
    MV_U8       key_content[AES_KEY_CONTENT_LENGTH];    // Fill up 128 bit or 256 bit key.
    MV_U8       user_auth[AES_KEY_CONTENT_LENGTH];      // user authentication key 32 bytes.
} AES_Entry_Config, *PAES_Entry_Config;

typedef struct _AES_Port_Config{
    MV_U8       port_id;
    MV_BOOLEAN  aes_enable;
    MV_U8       reserved[6];
} AES_Port_Config, *PAES_Port_Config;

typedef struct _AES_Entry_Verify {
    MV_U8       entry_id;                               //SPI entry 0~9
    MV_U8       key_length;                             //16: 128bit, 32: 256bit
    MV_U8       reserved[6];                            // for 64 bit align
    MV_U8       key_content[AES_KEY_CONTENT_LENGTH];    // Fill up 128 bit or 256 bit key.
    MV_U8       user_auth[AES_KEY_CONTENT_LENGTH];      // user authentication key 32 bytes.
} AES_Entry_Verify, *PAES_Entry_Verify;

typedef struct _AES_Change_Passwd {
    MV_U8       entry_id;                               // SPI entry 0~9
    MV_U8       reserved[7];                            // for 64 bit align
    MV_U8       old_user_auth[AES_KEY_CONTENT_LENGTH];  // user authentication key 32 bytes.
    MV_U8       new_user_auth[AES_KEY_CONTENT_LENGTH];  // user authentication key 32 bytes.
} AES_Change_Passwd, *PAES_Change_Passwd;

typedef struct _AES_PORT_INFO
{
    MV_U8       id;
    MV_U8       entry;
    MV_U8       Reserved[6]; //Align to 64bits
} AES_PORT_INFO, *PAES_PORT_INFO;

#define AES_ENTRY_STATUS_AVAILABLE           1
#define AES_ENTRY_STATUS_NOT_AVAILABLE       0

typedef struct _AES_KEY_ENTRY_INFO
{
    MV_U8        id;
    MV_U8        status;//AES_ENTRY_STATUS_XXXXXX
    MV_U8        Reserved[6]; //Align to 64bits
} AES_KEY_ENTRY_INFO, *PAES_KEY_ENTRY_INFO;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif
