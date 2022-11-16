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

#ifndef __MV_COM_ADAPTER_STRUCT_H__
#define __MV_COM_ADAPTER_STRUCT_H__

#include "com_define.h"

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

#define BASE_ADDRESS_MAX_NUM                    6

/* The following Version definition is kept here for backward compatability. */
/* Each fields are two bytes too big. */
typedef struct _Version_Info
{
	 MV_U32        VerMajor;
	 MV_U32        VerMinor;
	 MV_U32        VerOEM;
	 MV_U32        VerBuild;
}Version_Info, *PVersion_Info;

/* The following Version definition are copied from CIM_SoftwareIdentity class */
typedef struct _Version_Info_CIM
{
	 MV_U16        MajorVersion;
	 MV_U16        MinorVersion;
	 MV_U16        RevisionNumber;
	 MV_U16        BuildNumber;
}Version_Info_CIM, *PVersion_Info_CIM;

#ifndef _MARVELL_SDK_PACKAGE_NONRAID
#define SUPPORT_LD_MODE_RAID0                   MV_BIT(0)
#define SUPPORT_LD_MODE_RAID1                   MV_BIT(1)
#define SUPPORT_LD_MODE_RAID10                  MV_BIT(2)
#define SUPPORT_LD_MODE_RAID1E                  MV_BIT(3)
#define SUPPORT_LD_MODE_RAID5                   MV_BIT(4)
#define SUPPORT_LD_MODE_RAID6                   MV_BIT(5)
#define SUPPORT_LD_MODE_RAID50                  MV_BIT(6)
#define SUPPORT_LD_MODE_JBOD                    MV_BIT(7)
#define SUPPORT_LD_MODE_RAID60                  MV_BIT(8)
#define SUPPORT_LD_MODE_CROSS_SATA_SSD          MV_BIT(9)
#define SUPPORT_LD_MODE_HYPPER_HDD_MIRROR       MV_BIT(10)
#define SUPPORT_LD_MODE_HYBRID_HDD_MIRROR       MV_BIT(11)

#define FEATURE_BGA_REBUILD_SUPPORT             MV_BIT(0)
#define FEATURE_BGA_BKINIT_SUPPORT              MV_BIT(1)
#define FEATURE_BGA_SYNC_SUPPORT                MV_BIT(2)
#define FEATURE_BGA_MIGRATION_SUPPORT           MV_BIT(3)
#define FEATURE_BGA_MEDIAPATROL_SUPPORT         MV_BIT(4)
#define FEATURE_BGA_FEINIT_SUPPORT              MV_BIT(5)
#define FEATURE_BGA_COPY_BACK_SUPPORT           MV_BIT(6)
#define FEATURE_PD_OF_SAME_TYPE_WHEN_CREATE_VD  MV_BIT(7) // PDs must of the same type when used to create VD or DG
#endif
//_MARVELL_SDK_PACKAGE_NONRAID

#define ADV_FEATURE_EVENT_WITH_SENSE_CODE       MV_BIT(0)
#define ADV_FEATURE_BIG_STRIPE_SUPPORT          MV_BIT(1)
#define ADV_FEATURE_BIOS_OPTION_SUPPORT         MV_BIT(2) // to inform API if support upload/download bios
#define ADV_FEATURE_HAS_BBU                     MV_BIT(3)
#define ADV_FEATURE_CONFIG_IN_FLASH             MV_BIT(4)
#define ADV_FEATURE_CPU_EFFICIENCY_SUPPORT      MV_BIT(5)  // to inform API if support optimized CPU efficiency
#ifndef _MARVELL_SDK_PACKAGE_NONRAID
#define ADV_FEATURE_NO_MUTIL_VD_PER_PD          MV_BIT(6)
#endif
#define ADV_FEATURE_SPC_4_BUFFER                MV_BIT(7)
#define ADV_FEATURE_SES_DIRECT                  MV_BIT(8)
#define ADV_FEATURE_MODULE_CONSOLIDATE          MV_BIT(9)
#define ADV_FEATURE_IMAGE_HEALTH                MV_BIT(10) // support Image health
#define ADV_FEATURE_SATA_PHY_CTRL_BY_PORT       MV_BIT(11) // support SATA PHY Control, port base.
#define ADV_FEATURE_CRYPTO_SUPPORT              MV_BIT(12) // AES encryption, port based.
#define ADV_FEATURE_OS_TIME_SUPPORT             MV_BIT(13) // OS time support
#define ADV_FEATURE_NO_VD_WRITE_CACHE_SUPPORT   MV_BIT(14) // Magni not support VD write cache
#define ADV_FEATURE_NO_VD_READ_CACHE_SUPPORT    MV_BIT(15) // Magni not support VD read cache
#define ADV_FEATURE_NO_HD_SMART_SUPPORT         MV_BIT(16) // Magni not support HD smart
#define ADV_FEATURE_NO_VD_ROUNDING_SUPPORT      MV_BIT(17) // Magni not support VD GB Rounding
#define ADV_FEATURE_NO_HD_SETFREE_SUPPORT       MV_BIT(18) // Magni not support set free
#define ADV_FEATURE_NO_HD_WRITE_CACHE_SUPPORT   MV_BIT(19) // Magni not support HD write cache
#define ADV_FEATURE_NO_ENC_SUPPORT              MV_BIT(20) // Magni not support encloure
#define ADV_FEATURE_NO_BGA_RATE_CHANGE          MV_BIT(21) // Magni lite support bga but can not change rate
#define ADV_FEATURE_AES_PARTITION               MV_BIT(22) // Magni AES Partition
#define ADV_FEATURE_HYPERDUO_INTELLIGENT_INIT   MV_BIT(23) // Magni+
#define ADV_FEATURE_NO_SPARE_SUPPORT            MV_BIT(24) // Magni+ support rebuild without assign spare
#define ADV_FEATURE_HYBRID_SWITCH_SUPPORT       MV_BIT(25) // Magni+ support hybrid raid sp cache mode, cover vd cache policy
#define ADV_FEATURE_HOT_PLUG_SUPPORT            MV_BIT(26) // USB to SATA support Hot plug
#define ADV_FEATURE_ATA_PASS_THROUGH            MV_BIT(27) // Magni+ support ATA pass through
#define ADV_FEATURE_HYPERDUO_REMAP_SUPPORT      MV_BIT(28)
#define ADV_FEATURE_ACCESS_REGISTER             MV_BIT(29)
#define ADV_FEATURE_OEM_DATA                    MV_BIT(30) // enc notify for oem
#define ADV_FEATURE_FLASH_PER                   MV_BIT(31) // support image size api

#define ADV_FEATURE_2_FW_TWICE                  MV_BIT(0) // support frey flash firmware
#define ADV_FEATURE_2_NEW_VD_DG_CMD             MV_BIT(1) // support create/delete ARRAY_VD.
#define ADV_FEATURE_2_SCSI_PASS_THROUGH         MV_BIT(2) // support create/delete ARRAY_VD.
#define ADV_FEATURE_2_NO_FLASH_SUPPORT          MV_BIT(3) // flash operation support. !!!set to disable flash operation

#define PCIE_SPEED_UNKNOWN                      0
/* Gen I (2.5Gbps) */
#define PCIE_SPEED_GEN_I                        1
/* Gen II (5Gbps) */
#define PCIE_SPEED_GEN_II                       2
/* Gen III (8Gbps)*/
#define PCIE_SPEED_GEN_III                      3

#define PORT_PHY_PD     		MV_BIT(6)
#define PORT_PHY_DIS    		MV_BIT(5)
#define PORT_PHY_LOCK   		MV_BIT(4)
#define PORT_PHY_EN     		MV_BIT(3)
#define PORT_PHY_SPD    		0x7
#define PORT_PHY_SPD_G3 		3
#define PORT_PHY_SPD_G2 		2
#define PORT_PHY_SPD_G1 		1
#define PORT_PHY_SPD_MAX        0       // Max speed of HBA supports.

#define STRIPE_SIZE_16 			MV_BIT(0)	// 16K
#define STRIPE_SIZE_32 			MV_BIT(1)	// 32K
#define STRIPE_SIZE_64 			MV_BIT(2)	// 64K
#define STRIPE_SIZE_128 		MV_BIT(3)	// 128K
#define STRIPE_SIZE_256			MV_BIT(4)	// 256K
#define STRIPE_SIZE_512 		MV_BIT(5)	// 512K
#define STRIPE_SIZE_1024 		MV_BIT(6)	// 1024K
#define STRIPE_SIZE_MASK		(0x7F)		// all above

typedef struct _Adapter_Info
{
	Version_Info        DriverVersion;
	Version_Info_CIM    BIOSVersion;
	Version_Info_CIM    FirmwareVersion;
	Version_Info_CIM    BootLoaderVersion;
	MV_U64              Reserved1[1];     /* Reserve for firmware */

#if defined(_OS_EFI) || defined (_OS_UEFI)
    MV_U8               SystemIOBusNumber;
    MV_U8               SlotNumber;
    MV_U8               FunctionNumber;
    MV_U8               reserved_u8_0[1];
    
    MV_U32              SegmentNumber;
#else
    MV_U32              SystemIOBusNumber;
    MV_U32              SlotNumber;
#endif

	MV_U32              InterruptLevel;
	MV_U32              InterruptVector;

	MV_U16              VenID;
	MV_U16              SubVenID;
	MV_U16              DevID;
	MV_U16              SubDevID;

	MV_U8               PortCount;
	MV_U8               PortSupportType;  /* SATA, SAS, PATA etc, use MV_BIT */
	MV_U8               Features;         /* Feature bits.  See FEATURE_XXX */
	MV_BOOLEAN          AlarmSupport;
	MV_U8               RevisionID;       /* Chip revision */
	MV_U8				MaxPDPerVD;
	MV_U16              StripeSizeSupported;  // Supported stripe size.  See STRIPE_SIZE_XXX

	MV_U32              AdvancedFeatures; /* Advanced feature bits.  See ADV_FEATURE_XXX */
	MV_U8               MaxPDPerDG;
	MV_U8               MaxVDPerDG;
	MV_U8               MaxParityDisks;   /* Max parity disks for RAID6.  Multiply it by 2 for RAID60 */
	MV_U8               MaxDiskGroup;

	MV_U8               MaxTotalBlocks;
	MV_U8               MaxBlockPerPD;
	MV_U8               MaxHD;             /* Deprecated. Leave it here for backward compatibility.  New driver should use MaxHD_Ext */
	MV_U8               MaxExpander;
	MV_U8               MaxPM;
	MV_U8               MaxLogicalDrive;
	MV_U16              LogicalDriverMode;

	MV_U8               WWN[8];            /* For future VDS use. */

	MV_U16              MaxHD_Ext;		    /* Added to support max HD of 1024.  If it is 0, application should use MaxHD instead */
	MV_U16              MaxBufferSize;		/* Max contiguous buffer size driver can handle in unit of 1024. Application based on this to allocate space in API */
	MV_U16              MaxTotalBlocks_V2;	// For support 1024 and more blocks
	MV_U8               MaxAESPort;
	MV_U8               MaxHyperHdd;
	MV_U8               MaxSSDPerHyperDuo;
	MV_U8               MaxAESEntry;
	MV_U8               MaxSSDSegment;
    MV_U8               Reserved3[1];
	MV_U32              AdvancedFeatures2;
    MV_U8               Reserved32[4];
	
	MV_U8               SerialNo[20];
	MV_U8               PortSasAddress[16][8];  // SAS address of each port on the adapter
	MV_U8               MaxSpeed;//Gen I (2.5Gbps)/Gen II (5Gbps)/
	MV_U8               CurrentSpeed;//Gen I (2.5Gbps)/Gen II (5Gbps)/
	MV_U8               MaxLinkWidth;//1x,2x,4x,8x
	MV_U8               CurrentLinkWidth;//1x,2x,4x,8x
	            // ADV_FEATURE_IMAGE_HEALTH ***
	MV_BOOLEAN         img_health;         // AND all below image health state.
	MV_BOOLEAN         autoload_img_health;
	MV_BOOLEAN         boot_loader_img_health;
	MV_BOOLEAN         firmware_img_health;
	MV_BOOLEAN         boot_rom_img_health;    // func 0
	MV_BOOLEAN         hba_info_img_health;
	            // // ADV_FEATURE_IMAGE_HEALTH ###
	MV_U8              Reserved4[2];        // pad
	MV_U8              ModelNumber[20];
} Adapter_Info, *PAdapter_Info;

typedef struct _Adapter_Config_V2 {
	MV_BOOLEAN      AlarmOn;
	MV_BOOLEAN      AutoRebuildOn;
	MV_U8           BGARate;
	MV_BOOLEAN      PollSMARTStatus;
	MV_U8           MediaPatrolRate;
	MV_BOOLEAN      CopyBack;
	MV_U8           SyncRate;				// syncronization rate (consistency check and fix)
	MV_U8           InitRate;				// Initialization rate

	MV_U8           RebuildRate;
	MV_U8           MigrationRate;
	MV_U8           CopybackRate;
	MV_BOOLEAN      InterruptCoalescing;       // enable or disable
	MV_BOOLEAN      ModuleConsolidate;
	MV_BOOLEAN      v_atapi_disable;    // Virtual ATAPI disable. 0: enable/default; 1: disable
	MV_U8           sgpio_enable;       // sgpio enable									*/
#ifdef SUPPORT_READ_ONLY
    	MV_U8           read_only;
#else
	MV_BOOLEAN      rawimg_update_disabled;
#endif
	MV_U8			port_phy_ctrl[8];  // Phy control of ports, ADV_FEATURE_SATA_PHY_CTRL_BY_PORT
	MV_U8 		    SerialNo[20];
	MV_U8           ModelNumber[20];
} Adapter_Config_V2, *PAdapter_Config_V2;

/* To be backward compatible, keep the original Adapter_Config */
typedef struct _Adapter_Config {
	MV_BOOLEAN      AlarmOn;
	MV_BOOLEAN      AutoRebuildOn;
	MV_U8           BGARate;
	MV_BOOLEAN      PollSMARTStatus;
	MV_U8           MediaPatrolRate;
	MV_BOOLEAN      CopyBack;
	MV_U8           Reserved[2];
} Adapter_Config, *PAdapter_Config;

#define TYPE_MAILBOX_REGISTER         0
#define TYPE_INTERNAL_REGISTER        1

typedef struct _Adapter_Register_Value {
	MV_U8           offset;
	MV_U8           size;
    MV_U8           reserved1[6];
	MV_U32          value;
	MV_U8           reserved2[4];
} Adapter_Register_Value, *PAdapter_Register_Value;

typedef struct _Adapter_OEM_Data {
	MV_U16          cbSize;
    MV_U8           reserved[2];
	MV_U8           vendor_id[4];
	MV_U8           data[8];
	MV_U8           cdb[8];
	MV_U8           data2[48];
} Adapter_OEM_Data, *PAdapter_OEM_Data;

typedef struct _Adapter_OEM_Info {
	MV_U16          cbSize;
    MV_U8           reserved[2];
	MV_U8           vendor_id[4];
	MV_U32          support_bit;
	MV_U8           link_type;
	MV_U8           data[59];
} Adapter_OEM_Info, *PAdapter_OEM_Info;

#define  UNUSED_SENSOR   0xFF

typedef struct _Adapter_Sensor_Data {
	MV_I16          temperature1; /*unit is 0.01 Celsius */
    MV_U8           reserved[126];
} Adapter_Sensor_Data, *PAdapter_Sensor_Data;

#if(defined(_OS_DOS)&&defined(PRODUCTNAME_ATHENA))
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

    MV_U32      FlashID;
    MV_U32      FlashSize; 
    MV_U32      FlashSectSize;

    void*       FlashBar;
};
#endif

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
