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

/*******************************************************************************
*
*                   Copyright 2007,MARVELL SEMICONDUCTOR, LTD.
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
********************************************************************************/

#if !defined( _MVFLASH_H_ )
#define _MVFLASH_H_

#include "mvf_type.h"
/**********************************************************************************/
#define MAX_FLASH_PER_ADAPTER   2
#define MAX_FAKE_ADAPTER_SUPPORTED   2
#define MAX_FAKE_FLASH          (MAX_FAKE_ADAPTER_SUPPORTED*MAX_FLASH_PER_ADAPTER)
#define MAX_ADAPTER_SUPPORTED   (2+MAX_FAKE_ADAPTER_SUPPORTED)
#define MAX_FLASH_SUPPORTED     ( MAX_ADAPTER_SUPPORTED * MAX_FLASH_PER_ADAPTER+MAX_FAKE_FLASH)

#define ADP_INFO_FLASH_MAP_BASE(ai)         (ai->pFI[0]->DecodeAddress)
#define ADP_INFO_FLASH_MAP_SIZE(ai)         ((ai->pFI[ai->FlashCount-1]->DecodeAddress+ai->pFI[ai->FlashCount-1]->Size) \
                                            -ADP_INFO_FLASH_MAP_BASE(ai))      
#define MVF_IOCTL_CODE_START_UNIT     0
#define MVF_IOCTL_CODE_STOP_UNIT      1
#define MVF_IOCTL_CODE_MOUNT_UNIT     2
#define MVF_IOCTL_CODE_UNMOUNT_UNIT   3
#define MVF_IOCTL_CODE_LOCK_UNIT      4
#define MVF_IOCTL_CODE_UNLOCK_UNIT    5
#define MVF_IOCTL_CODE_RESET_UNIT     0xFF
typedef struct _FlashInfo FlashInfo;
typedef struct _AdapterInfo AdapterInfo;
struct _AdapterInfo
{
    MV_U32      flags;
    MV_U32      classCode;
    MV_U16      venId;    
    MV_U16      devId;
    MV_U8       bus;
    MV_U8       devFunc;
    MV_U8       version;
    MV_U8       subVersion;
    void*       bar[6];
    MV_U32      bar_size[6];
    void*       ExpRomBaseAddr;

    char        *buf;
    MV_U32      buf_size;

    void*       FlashBar;
    MV_U8       FlashCount;
    FlashInfo   *pFI[MAX_FLASH_PER_ADAPTER];

    MV_U32
    ( *HAL_RegR32 )
    (
        void*   addr,
        MV_U32  off
    );
    void
    ( *HAL_RegW32 )
    (
        void*   addr,
        MV_U32  off,
        MV_U32  data
    );
    /* pFI==NULL for extra close up of hardware */
    int
    (*Flash_Init)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI
    );
    /* pFI==NULL for extra close up of hardware */
    int
    (*Flash_Shutdown)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI
    );
    int
    (*Flash_ReadBuf)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      Addr,
        void       *Data,
        MV_U32      Count
    );
    int
    (*Flash_WriteBuf)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      Addr,
        void       *Data,
        MV_U32      Count
    )    ;
    int
    (*Flash_RMWBuf)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      Addr,
        void       *Data,
        MV_U32      Count,
        void       *Buf
    );
    int
    (*Flash_Sect2Sect)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      SAddr,
        MV_U32      TAddr,
        MV_U32      Count
    );
    int
    (*Flash_SectRMW)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      Addr,
        void       *Data,
        MV_U32      Count,
        MV_U32      SectorAddr
    );
    int
    (*Flash_SectErase)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      blkaddr
    );
    int
    (*Flash_Erase)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      blkaddr,
        MV_U32      count
    );
    int
    (*Flash_Ioctl)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        int         code,
        void        *OsContext
    );

    int
    (*Flash_GetBlkBase)
    (
        AdapterInfo *pAI,
        FlashInfo   *pFI,
        MV_U32      addr
    );
};
static inline int 
MVF_IOCTL( AdapterInfo *pAI, FlashInfo *pFI, int code, void *OsContext ) {
    if (pAI->Flash_Ioctl) 
        return pAI->Flash_Ioctl(pAI, pFI, code, OsContext);
    return 0;    
}

#define FI_FLAG_MEM_MAPPED      MV_BIT(0)
struct _FlashInfo
{
    AdapterInfo *pAI;
    MV_U32      DevBase;
    MV_U32      DecodeAddress;

    MV_U32      ID;
    MV_U32      Size; 
    MV_U32      SectSize;
    MV_U32		BlkSize;
    MV_U8       chip_sel;
    MV_U8       WriteBufSize;
    MV_U8       flags;
    MV_U8       bus_width;
};    

typedef struct _RomInfo
{
    MV_U32          Flags;
    void            *File;
    MV_U8           FileName[64]; 
    MV_U32          Size;

    MV_U16          devId;
    MV_U8           version;
    MV_U8           subVersion;
    MV_U8           szVersion[32];
}RomInfo;

typedef struct _MV_IMG_OsInfo MV_IMG_OsInfo;
struct _MV_IMG_OsInfo
{
    AdapterInfo *pAI;
    MV_U16      ven_id_of_img;
    MV_U16      dev_id_of_img;
};
#endif
