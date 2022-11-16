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

#if !defined( _HBA_INFO_H_ )
#define _HBA_INFO_H_ 

#define MAX_BOOTDEVICE_SUPPORTED        8
#define MAX_PORT_NUMBER                 8

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

typedef struct _PHY_TUNING {
	MV_U8	AMP:4;							/* 4 bits,  amplitude  */
	MV_U8	Pre_Emphasis:3;					/* 3 bits,  pre-emphasis */
	MV_U8	Reserved_1bit_1:1;				/* 1 bit,   reserved space */
	MV_U8	Drive_En:6;						/* 6 bits,	drive enable */
	MV_U8	Pre_Half_En:1;					/* 1 bit,	Half Pre-emphasis Enable*/
	MV_U8	Reserved_1bit_2:1;				/* 1 bit, 	reserved space */
    MV_U8	Reserved[2];					/* 2 bytes, reserved space */
} PHY_TUNING, *PPHY_TUNING;

/* 
      HBA_Info_Page is saved in Flash/NVRAM, total 256 bytes.
      The data area is valid only Signature="MRVL".
      If any member fills with 0xFF, the member is invalid.
*/
#define HIM_RF_RAID_DISABLE      MV_BIT(0)
typedef struct _HBA_Info_Main{
      // Dword 0
      MV_U8       signature[4];	/* 4 bytes, structure signature,should be "MRVL" at first initial */

      // Dword 1
	MV_U8     	MinorRev;     /* 2 bytes, NVRAM data structure version */
	MV_U8		MajorRev;
	MV_U16		Next_Page;	/* 2 bytes, For future data structure expansion, 0 is for NULL pointer and current page is the last page. */

      // Dword 2
      MV_U8       major;		/* 1 byte,  BIOS major version */
      MV_U8       Minor;		/* 1 byte,  BIOS minor version */
      MV_U8       oem_num;	/* 1 byte,  OEM number */
      MV_U8       build_num;	/* 1 byte,  Build number */

      // Dword 3
      MV_U8       Page_Code;	/* 1 byte,  eg. 0 for the 1st page  */
      MV_U8       Max_PHY_Num;	/* 1 byte,   maximum PHY number */
      MV_U8       RaidFeature;
                                /*
                                   BIT 0:	Disable RAID 5 
                                */
      MV_U8       Reserved2[1];

      // Dword 4
      MV_U32     hba_flag;	/* 
							4 bytes, should be 0x0000,0000 at first initial
							HBA flag:  refers to HBA_FLAG_XX
							bit 0   --- HBA_FLAG_BBS_ENABLE
							bit 1   --- HBA_FLAG_SILENT_MODE_ENABLE
							bit 2   --- HBA_FLAG_ERROR_STOP
							bit 3   --- HBA_FLAG_INT13_ENABLE
							bit 4   --- HBA_FLAG_ERROR_STOP
							bit 5   --- HBA_FLAG_ERROR_PASS
							bit 6   --- HBA_FLAG_SILENT_MODE_ENABLE
					*/

      // Dword 5                                                
      MV_U32     Boot_Device;                            /* 4 bytes, select boot device */
                                                                        /* for ata device, it is CRC of the serial number + model number. */
                                                                        /* for sas device, it is CRC of sas address */
                                                                        /* for VD, it is VD GUI */
 
      // Dword 6-8                                                                    
      MV_U32     Reserved3[3];                             /* 12 bytes, reserved   */

      // Dword 9-13
      MV_U8       serialnum[20];                           /* 20 bytes, controller serial number */

      // Dword 14-29
      SAS_ADDRESS    sas_address[MAX_PORT_NUMBER];	/* 64 bytes, SAS address for each port */

      // Dword 30-43
      MV_U8       Reserved4[56];                  /* 56 bytes, reserve space for future,initial as 0xFF */   

      // Dword 44-45
      MV_U8       PHY_Rate[MAX_PORT_NUMBER];	/* 8 bytes, 1byte*MAX_PORT_NUMBER  0:  1.5G, 1: 3.0G, should be 0x01 at first initial */
 
      // Dword 46-53
      PHY_TUNING    PHY_Tuning[MAX_PORT_NUMBER]; 	/* 32 bytes, PHY tuning parameters for each PHY*/

      // Dword 54-62
      MV_U32     Reserved5[9];		/* 9 dword, reserve space for future,initial as 0xFF */

      // Dword 63
      MV_U8       Reserved6[3];		/* 3 bytes, reserve space for future,initial as 0xFF */
      MV_U8       checksum;			/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
}HBA_Info_Main, *pHBA_Info_Main;                /* total 256 bytes */

#endif
