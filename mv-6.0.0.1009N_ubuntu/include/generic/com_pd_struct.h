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

#ifndef __MV_COM_PD_STRUCT_H__
#define __MV_COM_PD_STRUCT_H__

#include "com_physical_link.h"

//PD
#ifndef _OS_BIOS
#define MAX_HD_SUPPORTED_API                    128
#define MAX_BLOCK_PER_HD_SUPPORTED_API          16
#endif

// Macros used for error injection. 
// Error status that should be returned upon encountering the specified LBA
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8

#define PD_INIT_STATUS_OK						0
#define PD_INIT_STATUS_ERROR					MV_BIT(0) // for HD_Info InitStatus, set if there was ever a failure during init

// Definition used for old driver.
#define HD_TYPE_SATA                            MV_BIT(0)
#define HD_TYPE_PATA                            MV_BIT(1)
#define HD_TYPE_SAS                             MV_BIT(2)
#define HD_TYPE_ATAPI                           MV_BIT(3)
#define HD_TYPE_TAPE                            MV_BIT(4)
#define HD_TYPE_SES                             MV_BIT(5)

// Definition used for PD interface type as defined in DDF spec
#define PD_INTERFACE_TYPE_UNKNOWN               0x0000
#define PD_INTERFACE_TYPE_SCSI                  0x1000
#define PD_INTERFACE_TYPE_SAS                   0x2000
#define PD_INTERFACE_TYPE_SATA                  0x3000
#define PD_INTERFACE_TYPE_FC                    0x4000

// PD's Protocol/Connection type (used by new driver)
#define DC_ATA                                  MV_BIT(0)
#define DC_SCSI                                 MV_BIT(1)
#define DC_SERIAL                               MV_BIT(2)
#define DC_PARALLEL                             MV_BIT(3)
#define DC_ATAPI                                MV_BIT(4)  // used by core driver to prepare FIS
#define DC_SGPIO                                MV_BIT(5)

// PD's Device type defined in SCSI-III specification (used by new driver)
#define DT_DIRECT_ACCESS_BLOCK                  0x00
#define DT_SEQ_ACCESS                           0x01
#define DT_PRINTER                              0x02
#define DT_PROCESSOR                            0x03
#define DT_WRITE_ONCE                           0x04
#define DT_CD_DVD                               0x05
#define DT_OPTICAL_MEMORY                       0x07
#define DT_MEDIA_CHANGER                        0x08
#define DT_STORAGE_ARRAY_CTRL                   0x0C
#define DT_ENCLOSURE                            0x0D
// The following are defined by Marvell
#define DT_EXPANDER                             0x20
#define DT_PM                                   0x21

#define HD_FEATURE_NCQ                          MV_BIT(0)
#define HD_FEATURE_TCQ                          MV_BIT(1)
#define HD_FEATURE_1_5G                         MV_BIT(2)
#define HD_FEATURE_3G                           MV_BIT(3)
#define HD_FEATURE_WRITE_CACHE                  MV_BIT(4)
#define HD_FEATURE_48BITS                       MV_BIT(5)
#define HD_FEATURE_SMART                        MV_BIT(6)
#define HD_FEATURE_6G                           MV_BIT(7)
#define HD_FEATURE_CRYPTO                       MV_BIT(8)
#define HD_FEATURE_TRIM                       MV_BIT(9)
#define HD_FEATURE_12G                        MV_BIT(10)
#ifdef MAGNI_BIOS
#define HD_FEATURE_PATA_MASTER		MV_BIT(31)
#endif

#define HD_SPEED_1_5G                           1  
#define HD_SPEED_3G                             2
#define HD_SPEED_6G                             3
#define HD_SPEED_12G                            4

#define HD_AES_CRYPTO_DISK                      MV_BIT(0)
#define HD_AES_KEY_MATCHED                      MV_BIT(1)

#define HD_WIPE_MDD                             0
#define HD_WIPE_FORCE                           1

#define HD_DMA_NONE                             0
#define HD_DMA_1                                1
#define HD_DMA_2                                2
#define HD_DMA_3                                3
#define HD_DMA_4                                4
#define HD_DMA_5                                5
#define HD_DMA_6                                6
#define HD_DMA_7                                7
#define HD_DMA_8                                8
#define HD_DMA_9                                9

#define HD_PIO_NONE                             0
#define HD_PIO_1                                1
#define HD_PIO_2                                2
#define HD_PIO_3                                3
#define HD_PIO_4                                4
#define HD_PIO_5                                5

#define HD_XCQ_OFF                              0
#define HD_NCQ_ON                               1
#define HD_TCQ_ON                               2

#define HD_SSD_TYPE_NOT_SSD                     0
#define HD_SSD_TYPE_UNKNOWN_SSD                 1

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _HD_Info
{
 Link_Entity     Link;             /* Including self DevID & DevType */
 MV_U8           AdapterID;
 MV_U8           InitStatus;       /* Refer to PD_INIT_STATUS_XXX */
 MV_U8           HDType;           /* HD_Type_xxx, replaced by new driver with ConnectionType & DeviceType */
 MV_U8           PIOMode;          /* Max PIO mode */
 MV_U8           MDMAMode;         /* Max MDMA mode */
 MV_U8           UDMAMode;         /* Max UDMA mode */
 MV_U8           ConnectionType;   /* DC_XXX, ConnectionType & DeviceType in new driver to replace HDType above */
 MV_U8           DeviceType;       /* DT_XXX */

 MV_U32          FeatureSupport;   /* Support 1.5G, 3G, TCQ, NCQ, and etc, MV_BIT related */
#ifdef _OEM_LENOVO_M2
 MV_U8           Model [20];
 MV_U8           Fru[12];
 MV_U8           Rsvd[4];
 MV_U8           Mfa[4];
#else
 MV_U8           Model[40];
#endif
 MV_U8           SerialNo[20];
 MV_U8           FWVersion[8];
 MV_U64          Size;             /* In unit of BlockSize between API and driver */
 MV_U8           WWN[8];           /* ATA/ATAPI-8 has such definitions for the identify buffer */
 MV_U8           CurrentPIOMode;   /* Current PIO mode */
 MV_U8           CurrentMDMAMode;  /* Current MDMA mode */
 MV_U8           CurrentUDMAMode;  /* Current UDMA mode */
 MV_U8           ElementIdx;       /* corresponding element index in enclosure */
 MV_U32          BlockSize;        /* Bytes in one sector/block, if 0, set it to be 512 */

 MV_U8           ActivityLEDStatus;
 MV_U8           LocateLEDStatus;
 MV_U8           ErrorLEDStatus;
 MV_U8           SesDeviceType;	   /* ENC_ELEMENTTYPE_DEVICE or ENC_ELEMENTTYPE_ARRAYDEVICE */
#ifndef _OS_BIOS
 MV_U32		 sata_signature;
 MV_U8           Reserved4[8];
 MV_U8           HD_SSD_Type;
 MV_U8           Reserved5[1];
 MV_U16          Media_Rotation_Rate;
#ifdef _OEM_LENOVO_M2
 MV_U8           Lenovo_8S_l2_PN[10];
 MV_U8           Lenovo_8S_SN[30];
 MV_U8           Reserved1[20];
#else
 MV_U8           Reserved1[60];
#endif
#else
MV_U8			ID;
MV_U8			DeviceID;	/* only used show in BIOS setup */
MV_U16			PortID;		//Point to Port, PM or Expander ID
MV_U8 			SAS_Address[8];
#ifdef SUPPORT_FLORENCE
MV_U8			HD_SSD_Type;
MV_U8			HD_SSD_Operation_Mode;
MV_U8			Reserve1[2];
#endif
//
#ifdef SUPPORT_MUL_LUN
MV_U8    multi_lun;
MV_U16    lun_id;
#endif

#endif
}HD_Info, *PHD_Info;

typedef struct _HD_MBR_Info
{
 MV_U8           HDCount;
 MV_U8           Reserved[7];
 MV_U16          HDIDs[MAX_HD_SUPPORTED_API];    
 MV_BOOLEAN      hasMBR[MAX_HD_SUPPORTED_API];
} HD_MBR_Info, *PHD_MBR_Info;


typedef struct _HD_FreeSpaceInfo
{
 MV_U16          ID;               /* ID should be unique*/
 MV_U8           AdapterID;
 MV_U8           reserved1;
 MV_U16          BlockSize;        /* Bytes in one sector/block, if 0, set it to be 512 */
 MV_U8           reserved2;
 MV_BOOLEAN      isFixed;

 MV_U64          Size;             /* In unit of BlockSize between API and driver */
}HD_FreeSpaceInfo, *PHD_FreeSpaceInfo;

typedef struct _HD_Block_Info
{
 MV_U16          ID;               /* ID in the HD_Info*/
 MV_U8           Type;             /* Refer to DEVICE_TYPE_xxx */
 MV_U8           BlkCount;         /* The valid entry count of BlockIDs */
                                   /* This field is added for supporting large block count */
                                   /* If BlkCount==0, "0x00FF" means invalid entry of BlockIDs */
 MV_U8           Reserved1[4];

 /* Free is 0xff */
 MV_U16          BlockIDs[MAX_BLOCK_PER_HD_SUPPORTED_API];  
}HD_Block_Info, *PHD_Block_Info;


typedef struct _HD_CONFIG
{
 MV_BOOLEAN        WriteCacheOn; // 1: enable write cache 
 MV_BOOLEAN        SMARTOn;      // 1: enable S.M.A.R.T
 MV_BOOLEAN        Online;       // 1: to set HD online
 MV_U8             DriveSpeed;   // For SATA & SAS.  HD_SPEED_1_5G, HD_SPEED_3G etc
 MV_U8             crypto;
 MV_U8             AESPercentage;
 MV_U16            HDID;   
}HD_Config, *PHD_Config;

typedef struct  _HD_SMART_STATUS
{
 MV_BOOLEAN        SmartThresholdExceeded;        
 MV_U8             LinkSpeed;
 MV_U16            HDID;
 MV_U8             Reserved2[4];
}HD_SMART_Status, *PHD_SMART_Status;

typedef struct _HD_BGA_STATUS
{
 MV_U16            HDID;
 MV_U16            Percentage;      /* xx% */
 MV_U8             Bga;             /* Refer to HD_BGA_TYPE_xxx */
 MV_U8             Status;          /* not used */
 MV_U8             BgaStatus;       /* Refer to HD_BGA_STATE_xxx */
 MV_U8             Reserved[57];  
}HD_BGA_Status, *PHD_BGA_Status;

/* 
 * RCT entry flag 
 */
/* request type related */
#define EH_READ_VERIFY_REQ_ERROR                MV_BIT(0) /* Read or Read Verify request is failed. */
#define EH_WRITE_REQ_ERROR                      MV_BIT(1) /* Write request is failed */
/* error type related */
#define EH_MEDIA_ERROR                          MV_BIT(3) /* Media Error or timeout */
#define EH_LOGICAL_ERROR                        MV_BIT(4) /* Logical Error because of BGA activity. */
/* other flag */
#define EH_FIX_FAILURE                          MV_BIT(5) /* Ever tried to fix this error but failed */
/* extra flag */
#define EH_TEMPORARY_ERROR                      MV_BIT(7) /* Temporary error. Used when BGA rebuild write mark target disk up. */


typedef struct _RCT_Record{
 MV_LBA  lba;
 MV_U32  sec;                       //sector count
 MV_U8   flag;
 MV_U8   rev[3];
}RCT_Record, *PRCT_Record;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

//PM
#ifndef _OS_BIOS
#ifndef MAX_PM_SUPPORTED
#define MAX_PM_SUPPORTED                            16
#endif
#define MAX_PM_SUPPORTED_API                    MAX_PM_SUPPORTED
#endif

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef  struct _PM_Info{
 Link_Entity       Link;           /* Including self DevID & DevType */
 MV_U8             AdapterID;
 MV_U8             ProductRevision;
 MV_U8             PMSpecRevision; /* 10 means 1.0, 11 means 1.1 */
 MV_U8             NumberOfPorts;
 MV_U16            VendorId;
 MV_U16            DeviceId;
 MV_U8             Reserved1[8];
}PM_Info, *PPM_Info;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */


#ifndef _OS_BIOS
#define MAX_EXPANDER_SUPPORTED_API              16
#endif

#define EXP_SSP                                 MV_BIT(0)
#define EXP_STP                                 MV_BIT(1)
#define EXP_SMP                                 MV_BIT(2)

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

typedef struct _Exp_Info
{
 Link_Entity       Link;            /* Including self DevID & DevType */
 MV_U8             AdapterID;
 MV_BOOLEAN        Configuring;      
 MV_BOOLEAN        RouteTableConfigurable;
 MV_U8             PhyCount;
 MV_U16            ExpChangeCount;
 MV_U16            MaxRouteIndexes;
 MV_U8             VendorID[8+1];
 MV_U8             ProductID[16+1];
 MV_U8             ProductRev[4+1];
 MV_U8             ComponentVendorID[8+1];
 MV_U16            ComponentID;
 MV_U8             ComponentRevisionID;
 MV_U8             Reserved1[17];
}Exp_Info, * PExp_Info;

typedef struct _PD_OEM_Data {
    MV_U16          cbSize;
	MV_U8           vendor_id[4];
	MV_U16          ID;
	MV_U8           flow;
	MV_U8           command[7];
    MV_U8           data[32];
} PD_OEM_Data, *PPD_OEM_Data;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif
