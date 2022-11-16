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

#ifndef __MV_COM_VD_STRUCT_H__
#define __MV_COM_VD_STRUCT_H__

#if !defined(_MARVELL_SDK_PACKAGE_NONRAID)

#include "com_raid_struct.h"

#ifndef _OS_BIOS
#define MAX_LD_SUPPORTED_API                    32
#endif

#define LD_MAX_NAME_LENGTH                      16

#define LD_STATUS_FUNCTIONAL                    0
#define LD_STATUS_DEGRADE                       1
#define LD_STATUS_DELETED                       2
#define LD_STATUS_MISSING                       3 /* LD missing in system. */
#define LD_STATUS_OFFLINE                       4
#define LD_STATUS_PARTIALLYOPTIMAL              5 /* r6 w/ 2 pd, 1 hd drops */
#define LD_STATUS_FOREIGN                       6
#define LD_STATUS_IMPORTABLE                    7
#define LD_STATUS_NOT_IMPORTABLE                8
#define LD_STATUS_MIGRATION                     9
#define LD_STATUS_REBUILDING                    10
#define LD_STATUS_CONFLICTED                    11
#define LD_STATUS_DEGRADE_PLUGIN                12 // this specail status only for raid1
#define LD_STATUS_HYPER_UNINIT                  13 // this specail status only for magni
#define LD_STATUS_HYPER_USING_TABLE             14 // check the table for read/write SSD or HDD
#define LD_STATUS_INITIALIZED                   15 // VD is functional and initialized.
#define LD_STATUS_INVALID                       0xFF

#define LD_BGA_EXT_NEED_SYS_VOL_INFO	MV_BIT(7)	//Notify AP that System Volume Info is needed

#ifndef MV_GUID_SIZE
#define MV_GUID_SIZE                            8
#endif /* MV_GUID_SIZE */

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _LD_Info
{
 MV_U16            ID;
 MV_U8             Status;         /* Refer to LD_STATUS_xxx */
 MV_U8             BGAStatus;      /* Refer to LD_BGA_STATE_xxx */
 MV_U16            StripeBlockSize;/* between RAIDAPI and driver, it is # of BlockSize */
 MV_U8             RaidMode;            
 MV_U8             HDCount;

 MV_U8             CacheMode;      /* Default is CacheMode_Default, see above */
 MV_U8             LD_GUID[MV_GUID_SIZE];
 MV_U8             SectorCoefficient; /* (sector size) 1=>512 (default), 2=>1024, 4=>2048, 8=>4096 */
 MV_U8             AdapterID;
 MV_U8             BlkCount;         /* The valid entry count of BlockIDs */
                                     /* This field is added for supporting large block count */
                                     /* If BlkCount==0, "0x00FF" means invalid entry of BlockIDs */
 MV_U16            time_hi;
 MV_U16            DGID;       

 MV_U64            Size;           /* In unit of BlockSize between API and driver. */

 MV_U8             Name[LD_MAX_NAME_LENGTH];

 MV_U16            BlockIDs[MAX_HD_SUPPORTED_API];
/* 
 * According to BLOCK ID, to get the related HD ID, then WMRU can 
 * draw the related graph like above.
 */
 MV_U8             SubLDCount;     /* for raid 10, 50,60 */
 MV_U8             NumParityDisk;  /* For RAID 6. */
 MV_U16            time_low;
 MV_U32            BlockSize;      /* in bytes. if 0, BlockSize is 512 */

 #ifdef _OS_BIOS
 MV_U16			Mv_Scsi_ID;
 MV_U8			Bypass_Flag;
 MV_U8             Reserved2[1];
 #endif
 
}LD_Info, *PLD_Info;

typedef struct _Create_LD_Param
{
 MV_U8             RaidMode;
 MV_U8             HDCount;
 MV_U8             RoundingScheme; /* please refer to the definitions of  ROUNDING_SCHEME_XXX. */
 MV_U8             SubLDCount;     /* for raid 10,50,60 */
 MV_U16            StripeBlockSize;/* between API and driver, it is # of BlockSize */
 MV_U8             NumParityDisk;  /* For RAID 6. */
 MV_U8             CachePolicy;    /* please refer to the definition of CACHEMODE_XXXX. */

 MV_U8             InitializationOption; /* please refer to the definitions of INIT_XXXX. */
 MV_U8             SectorCoefficient;    /* (sector size) 1=>512 (default), 2=>1024, 4=>2048, 8=>4096 */
 MV_U16            LDID;                 /* ID of the LD to be migrated or expanded */
 MV_U8             SpecifiedBlkSeq;      /* DG slot number, this is only used when DG is enforced */
 MV_U8             HypperWaterMark;
 MV_U8             Reserved2[1];
 MV_U8             ReservedForApp;	/* Reserved for application (CLI) */

 MV_U16            HDIDs[MAX_HD_SUPPORTED_API];    /* 32 */
 MV_U8             Name[LD_MAX_NAME_LENGTH];

 MV_U64            Size;           /* In unit of BlockSize between API and driver. API need to get the BlockSize for PD first and use it to set 'Size' */
} Create_LD_Param, *PCreate_LD_Param;

typedef struct _Hyper_Free_Info
{
 MV_U16            ID;
 MV_U8             Reserved[6];

 MV_U64            HyperFreeSize;
} Hyper_Free_Info, *PHyper_Free_Info;

typedef struct _Hyper_Map_Info
{
 MV_U16            VDID;
 MV_U8             SegmentID;
 MV_U8             Reserved[5];

 MV_U64            SegmentStartLBA; //start(Bytes) / 512
 MV_U64            SegmentSize;     //size (Bytes) / 512
} Hyper_Map_Info, *PHyper_Map_Info;

typedef struct _LD_STATUS
{
 MV_U8            Status;          /* Refer to LD_STATUS_xxx */
 MV_U8            Bga;             /* Refer to LD_BGA_xxx */
 MV_U16           BgaPercentage;   /* xx% */
 MV_U8            BgaState;        /* Refer to LD_BGA_STATE_xxx */
 MV_U8            BgaExt;          /* Not used yet. Extention of Bga. */
 MV_U16           LDID;
} LD_Status, *PLD_Status;

#define      LD_CONFIG_MODE_NORMAL_REBUILD         0
#define      LD_CONFIG_MODE_INTELLIGENT_REBUILD    1

typedef struct    _LD_Config
{
 MV_U8            CacheMode;        /* See definition 4.4.1 CacheMode_xxx */
 MV_U8            HypperWaterMark;        
 MV_BOOLEAN       AutoRebuildOn;    /* 1- AutoRebuild On */
 MV_U8            Status;
 MV_U16           LDID;
 MV_U8            HyperRebuildMode; // LD_CONFIG_MODE_XXX
 MV_U8            Reserved2;

 MV_U8            Name[LD_MAX_NAME_LENGTH];
}LD_Config, * PLD_Config;

typedef struct _VD_HDD_Link
{
 MV_U16            VDID;
 MV_U16            HDD;
 MV_U8             Reserved[4];
} VD_HDD_Link, *PVD_HDD_Link;

#if defined(CACHE_MODULE_SUPPORT) || defined(SUPPORT_MODULE_CONSOLIDATE)
typedef struct _LD_CACHE_STATUS
{
 MV_BOOLEAN       IsOnline;
 MV_U8            CachePolicy;
 MV_U16           StripeUnitSize;
 MV_U32           StripeSize;
 MV_U8            Sector_Coefficient;
 MV_U8            RAID_Level;
 MV_U8            IsSSD;
 MV_U8            Reserved[5];
 MV_LBA           MAX_LBA;
#ifdef _DDR_BBU_ENABLE
 MV_U8    LD_GUID[MV_GUID_SIZE];
#endif
#ifdef HAVE_RAID_NATIVE_4K
 MV_U32           SectorSize;
#endif
}
LD_CACHE_STATUS, *PLD_CACHE_STATUS;
#endif /* CACHE_MODULE_SUPPORT */

typedef struct _VD_OEM_Data {
	MV_U16          cbSize;
	MV_U8           vendor_id[4];
	MV_U16          ID;
	MV_U8           flow;
	MV_U8           command[7];
    MV_U8           data[32];
} VD_OEM_Data, *PVD_OEM_Data;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif

#endif
