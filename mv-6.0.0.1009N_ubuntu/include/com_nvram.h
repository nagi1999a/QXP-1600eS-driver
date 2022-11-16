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

#if !defined(COM_NVRAM_H)
#define COM_NVRAM_H
#include "mv_config.h"
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#include "core_hal.h"
#endif
typedef union {
    struct {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];        
} SAS_ADDR, *PSAS_ADDR;

/* Generate  PHY tunning parameters */
typedef struct _PHY_TUNING {
#ifndef SUPPORT_BALDUR
	MV_U8	AMP:4;						  	/* 4 bits,  amplitude  */
	MV_U8	Pre_Emphasis:3;				 	/* 3 bits,  pre-emphasis */
	MV_U8	Reserved_1bit_1:1;				/* 1 bit,   reserved space */
         MV_U8	Drive_En:6;						/* 6 bits,	drive enable */
	MV_U8	Pre_Half_En:1;					/* 1 bit,	Half Pre-emphasis Enable*/
	MV_U8	Reserved_1bit_2:1;				/* 1 bit, 	reserved space */
	MV_U8	Reserved[2];						/* 2 bytes, reserved space */
#else
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_U8	Trans_Emphasis_En:1;			/* 1 bit,  transmitter emphasis enable	*/
	MV_U8	Trans_Emphasis_Amp:4;			/* 4 bits, transmitter emphasis amplitude */
	MV_U8	Reserved_2bit_1:3;				/* 3 bits, reserved space */
	MV_U8	Trans_Amp_Adjust:1;				/* 1 bits, transmitter amplitude adjust */
	MV_U8	Trans_Amp:5;					/* 5 bits, transmitter amplitude */
	MV_U8	Reserved_2bit_2:2;				/* 2 bits,	reserved space */
	MV_U8	Reserved[2];					/* 2 bytes, reserved space */
#else
	MV_U8	Trans_Emphasis_En:1;			/* 1 bit,  transmitter emphasis enable  */
	MV_U8	Trans_Emphasis_Amp:4;		    	/* 4 bits, transmitter emphasis amplitude */
	MV_U8	Reserved_2bit_1:3;				/* 3 bits, reserved space */
       MV_U8	Trans_Amp:5;					/* 5 bits, transmitter amplitude */
	MV_U8	Trans_Amp_Adjust:2;			/* 2 bits, transmitter amplitude adjust */
	MV_U8	Reserved_2bit_2:1;				/* 1 bit, 	reserved space */
	MV_U8	Reserved[2];						/* 2 bytes, reserved space */
#endif
#endif
} PHY_TUNING, *PPHY_TUNING;

#if defined(PRODUCTNAME_ATHENA) || defined(PRODUCTNAME_SP2)
typedef struct _PHY_TUNING_State {
	MV_U8	Gen1:1;			/* 1 bit, 0 means not be tunned; 1 means be tunned */
	MV_U8	Gen2:1;		    	/* 1 bit, 0 means not be tunned; 1 means be tunned */
	MV_U8	Gen3:1;			/* 1 bit, 0 means not be tunned; 1 means be tunned */
	MV_U8	Gen4:1;			/* 1 bit, 0 means not be tunned; 1 means be tunned */
	MV_U8	Reserved_4bit:4;	/* 4 bit, reserved space */
} PHY_TUNING_State, *PPHY_TUNING_State;
#endif
typedef struct _FFE_CONTROL {
	MV_U8	FFE_Capacitor_Select:4;			/* 4 bits,  FFE Capacitor Select  (value range 0~F)  */
	MV_U8	FFE_Resistor_Select	:3;		    /* 3 bits,  FFE Resistor Select (value range 0~7) */
   	MV_U8	Reserved			:1;			/* 1 bit reserve*/
} FFE_CONTROL, *pFFE_CONTROL;

typedef struct _Mv_Phy_Status{
	MV_U32	phy_id;
	MV_U32	subtractive;
	MV_U8	sas_address[8];
	MV_U32	device_type;	//sata or sas
	MV_U32	link_rate;
}Mv_Phy_Status, *pMv_Phy_Status;


/* HBA_FLAG_XX */
#define HBA_FLAG_INT13_ENABLE				MV_BIT(0)	//int 13h enable/disable
#define HBA_FLAG_SILENT_MODE_ENABLE			MV_BIT(1)	//silent mode enable/disable
#define HBA_FLAG_ERROR_STOP					MV_BIT(2)	//if error then stop
#define HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY	MV_BIT(3)	//if 1, enable interrupt coalescing, optimize CPU efficiency
#define HBA_FLAG_AUTO_REBUILD_ON            MV_BIT(4) // auto rebuild on/off                                                   //XIN
#ifdef _BGA_COPY_BACK
#define HBA_FLAG_COPY_BACK_ON               MV_BIT(5) // copy back on/off
#endif
#define HBA_FLAG_SMART_ON					MV_BIT(6) // smart on/off
#define HBA_FLAG_DISABLE_MOD_CONSOLIDATE	MV_BIT(7)	//if 1, disable module consolidation
#define HBA_FLAG_ENABLE_BUZZER			    MV_BIT(8)
#define HBA_FLAG_SERIAL_CONSOLE_ENABLE		MV_BIT(9) //Bios enable or disable console redirection feature
#define HBA_FLAG_ENABLE_SGPIO               MV_BIT(10) //HBA support SGPIO backplane
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define HBA_FLAG_SATA_SSU_MODE				MV_BIT(11) //HBA support sata ssu spin up mode, if 1,ssu mode, if 0, puis mode
#endif
#define HBA_FLAG_FIXED_PORT_MAPPING         MV_BIT(12) //Force PD ID = Port ID
#define HBA_FLAG_DISABLE_TWSI_I2C           MV_BIT(13) //To disable TWSI I2C
#define HBA_FLAG_MULTI_PATH			MV_BIT(14)
#define HBA_FLAG_WINDOWS_UPDATE_LUIGI_YOSHI_FW	MV_BIT(15)

#define NVRAM_DATA_MAJOR_VERSION		0
#define NVRAM_DATA_MINOR_VERSION		1

/* for RAID_Feature */
#define RAID_FEATURE_DISABLE_RAID5			MV_BIT(0)
#define RAID_FEATURE_ENABLE_RAID			MV_BIT(1)

#if defined(SUPPORT_ROC)
#define Max_SpinUpDisk 	8
#define Max_SpinUpTime	8
#endif

#if defined(_OS_FIRMWARE)
typedef struct _boot_dev_struct
{
	MV_U32   WWN_CRC;
} boot_dev_info_t;
#endif

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define PAGE_HEADER_MAJOR_VER  0x01
#define PAGE_HEADER_MINOR_VER  0x01
#endif

typedef struct _page_header{
    // Dword 0
    MV_U8	signature[4];		/* 4 bytes, structure signature,should be "MRVL" at first initial */
    // Dword 1
    MV_U8	page_code;		/* 1 byte, page code , here should be 0x01  for PD info Page */
    MV_U8	check_sum;		/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
    MV_U16	data_length; 		/* 2 byte, data length, indicates the size of invalid data besides page header */
    // Dword 2
    MV_U16	next_page;		/* 2 byte, next page. Bit0: 0:=>Address Increase, 1:=>Address Decrease */
    MV_U8   major_ver;  /*1 byte, major version of page header.*/
    MV_U8   minor_ver;  /*1 byte, minor version of page header.*/
}page_header, *p_page_header;

/* 
	HBA_Info_Page is saved in Flash/NVRAM, total 256 bytes.
	The data area is valid only Signature="MRVL".
	If any member fills with 0xFF, the member is invalid.
*/
/*
    After spec. BIOS_HBA_page_v1.8, the first 3 DWORD shall be replaced by page_header structure.
*/
typedef struct _HBA_Info_Page{
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	page_header header;
#else
	// Dword 0
	MV_U8     	Signature[4];                 	/* 4 bytes, structure signature,should be "MRVL" at first initial */

	// Dword 1
	MV_U8     	MinorRev;                 		/* 2 bytes, NVRAM data structure version */
	MV_U8		MajorRev;
	MV_U16    	Next_Page;					  	/* 2 bytes, For future data structure expansion, 0 is for NULL pointer and current page is the last page. */

	// Dword 2
	MV_U8     	Major;                   		/* 1 byte,  BIOS major version */
	MV_U8     	Minor;                  		/* 1 byte,	BIOS minor version */
	MV_U8     	OEM_Num;                     	/* 1 byte,  OEM number */
	MV_U8     	Build_Num;                    	/* 1 byte,  Build number */
#endif

	// Dword 3
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_U8 		Reserved1;
#else
	MV_U8     	Page_Code;					  	/* 1 byte,  eg. 0 for the 1st page  */
#endif
	MV_U8     	Max_PHY_Num;				  	/* 1 byte,   maximum PHY number */
	MV_U8		RAID_Feature;					/* 1 byte,  RAID settings (see RAID_FEATURE_XX)
															bit 0 - disable RAID 5 (default 1 = disabled)
												*/
#ifdef SUPPORT_READ_ONLY
	MV_U8		Host_Read_Only;					/* 1 byte,  host read only flag: bit n - host n, 1 - readonly */
#else
	MV_U8		Reserved2;
#endif

	// Dword 4
	MV_U32     	HBA_Flag;                     	/* 4 bytes, should be 0x0000,0000 at first initial
													HBA flag:  refers to HBA_FLAG_XX
												*/
	// Dword 5	                                              
	MV_U32     	Boot_Device;					/* 4 bytes, select boot device */
												/* for ata device, it is CRC of the serial number + model number. */
												/* for sas device, it is CRC of sas address */
												/* for VD, it is VD GUI */

	// Dword 6-8	
	MV_U8		DSpinUpGroup;				/* spin up group */
	MV_U8		DSpinUpTimer;				/* spin up timer */
	MV_U8		Delay_Time;					/* delay time, default value = 5 second */
       MV_U8           bbu_charge_threshold;
       MV_U8           bbu_temp_lowerbound;
       MV_U8           bbu_temp_upperbound;
       MV_U16          bbu_volt_lowerbound;
       MV_U16          bbu_volt_upperbound;
	MV_U8		Reserved3[2];				  	/* 4 bytes, reserved	*/

	// Dword 9-13
	MV_U8     	Serial_Num[20];				  	/* 20 bytes, controller serial number */

	// Dword 14-29
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	//SAS_ADDR	SAS_Address[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];               /* 64 bytes, SAS address for each port */
	MV_U8   		Reserved21[64];				/* sas addr move to the phy info page */
#else
	SAS_ADDR	SAS_Address[8];                                     /* 64 bytes, SAS address for each port */
#endif
#if defined(_OS_FIRMWARE)
	//Dword 30-37
	boot_dev_info_t	device[8];			/* 32 bytes, record boot devices information.*/

	//Dword 38-39
	FFE_CONTROL  FFE_Control[8];	
	//Dword 40-43
	MV_U8     	Reserved4[16];                  /* 24bytes, reserve space for future,initial as 0xFF */   
#else // defined(_OS_FIRMWARE)

#if defined(SUPPORT_MITAC)  //odin bios&mdu for mitac support
	//Dword 30
	MV_U32			Bios_Flag;					/*4 bytes should be 0xffff_ffff at first inital, 0 means enable
																					bit0	---	MITAC_BIOS
																					.
																					.
																					etc
																	 */
	//Dword31-38
	MV_U16			Port[8];					/*port for group member, 'NA' means no group*/
	//Dword 39-43
	MV_U8			Reserved4[36];		/* 52 bytes, reserve space for future,initial as 0xFF */ 
#else //support_mitac

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	MV_U8 			Reserved31[8]; 		/*ffe_control move to the phy info page */
#else
	// Dword 30-31
	FFE_CONTROL  FFE_Control[8];			/* 8 bytes for vanir 8 port PHY FFE seeting
										BIT 0~3 : FFE Capacitor select(value range 0~F)
										BIT 4~6 : FFE Resistor select(value range 0~7)
										BIT 7: reserve. */
#endif
#ifdef VANIR_BIOS
	//Dword 32 -38
   	MV_U8		 Reserved4[28];			/* 28 bytes, reserve space for future,initial as 0xFF */
#else
	//Dword 32 -33
	MV_U8		Product_ID[8];			/* 8 bytes for vanir bios to differentiate VA6800m HBA and VA6400m HBA */

	//Dword 34 -38
	MV_U8		Reserved4[20];			/* 20 bytes, reserve space for future,initial as 0xFF */
#endif

 	//Dword 39 -43
	MV_U8		  model_number[20];		 /* 20 bytes, Florence model name */
#endif //defined(SUPPORT_MITAC)

#endif //defined(_OS_FIRMWARE)

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	// Dword 44-45
	MV_U8 Reserved41[8]; /*phy_rate move to the phy info page*/
	// Dword 46-53
	MV_U8 Reserved51[32]; /*phy_tuning move to the phy info page */	
#else
	// Dword 44-45
	MV_U8     	PHY_Rate[8];                  	/* 8 bytes,  0:  1.5G, 1: 3.0G, should be 0x01 at first initial */

	// Dword 46-53
	PHY_TUNING   PHY_Tuning[8];				/* 32 bytes, PHY tuning parameters for each PHY*/
#endif

	// Dword 54-62
	MV_U32     	Reserved5[7];                 	/* 9 dword, reserve space for future,initial as 0xFF */
	MV_U8  		BGA_Rate;
	MV_U8  		Sync_Rate;
	MV_U8 		Init_Rate;
	MV_U8  		Rebuild_Rate;
	MV_U8  		Migration_Rate;
	MV_U8  		Copyback_Rate;
	MV_U8  		MP_Rate;
	MV_U8     	Reserved7; 
	// Dword 63
	MV_U8     	Reserved6[3];                 	/* 3 bytes, reserve space for future,initial as 0xFF */
	MV_U8     	Check_Sum;                    	/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
}HBA_Info_Page, *pHBA_Info_Page;			/* total 256 bytes */

/*
    After spec. BIOS_HBA_page_v1.8, the first 3 DWORD shall be replaced by page_header structure.
*/
#if defined(_OS_FIRMWARE)
typedef struct _hba_info_Page{
	// Dword 0 - 2
	page_header header;

	// Dword 3
	MV_U8     	Max_PHY_Num;				  	/* 1 byte,   maximum PHY number */
	MV_U8		RAID_Feature;					/* 1 byte,  RAID settings (see RAID_FEATURE_XX)
															bit 0 - disable RAID 5 (default 1 = disabled)
												*/
    MV_U8       Reserved1[2];

	// Dword 4
	MV_U32     	HBA_Flag;                     	/* 4 bytes, should be 0x0000,0000 at first initial
													HBA flag:  refers to HBA_FLAG_XX
												*/
	// Dword 5	                                              
	MV_U32     	Boot_Device;					/* 4 bytes, select boot device */
												/* for ata device, it is CRC of the serial number + model number. */
												/* for sas device, it is CRC of sas address */
												/* for VD, it is VD GUI */

	// Dword 6
    MV_U8       DSpinUpGroup;				/* spin up group */
    MV_U8       DSpinUpTimer;				/* spin up timer */
    MV_U8       Delay_Time;					/* delay time, default value = 5 second */
    MV_U8       Reserved2;
    
	// Dword 7
    MV_U16      bbu_volt_lowerbound;
    MV_U16      bbu_volt_upperbound;

	// Dword 8
    MV_U8       bbu_charge_threshold;
    MV_U8       bbu_temp_lowerbound;
    MV_U8       bbu_temp_upperbound;
    MV_U8       Reserved3;

	// Dword 9-13
	MV_U8     	Serial_Num[20];				  	/* 20 bytes, controller serial number */

	// Dword 14-29
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
	SAS_ADDR	SAS_Address[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];               /* 64 bytes, SAS address for each port */
#else
	SAS_ADDR	SAS_Address[8];                                     /* 64 bytes, SAS address for each port */
#endif
	//Dword 30-37
	boot_dev_info_t	device[8];			/* 32 bytes, record boot devices information.*/

    //Dword 38   
	MV_U8  		BGA_Rate;
	MV_U8  		Sync_Rate;
	MV_U8 		Init_Rate;
	MV_U8  		Rebuild_Rate;

    //Dword 39
	MV_U8  		Migration_Rate;
	MV_U8  		Copyback_Rate;
	MV_U8  		MP_Rate;
	MV_U8     	Reserved4;

    //Dword 40 - 63
	MV_U8     	Reserved5[96];
}hba_info_page, *phba_info_page;			/* total 256 bytes */
#endif


#ifdef SUPPORT_ROC
#define PHY_INFO_PAGE_MAJOR_VER 1
#define PHY_INFO_PAGE_MINOR_VER 0
#endif

#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
typedef struct _Phy_Info_Page{
	//Dword 0-2
	page_header header;  /* refer to  HBA information page header */

	//Dword 3-34
	SAS_ADDR SAS_Address[16]; /*128bytes, SAS address for each port */
	
	//Dword 35-50
	MV_U8		Reserved1[64];

	// Dword 51-54
	MV_U8    		PHY_Rate[16];	/* 16 bytes, support phy rate 0: 1.5G, 1: 1.5G and 3.0G,  2: 1.5G, 3.0G and 6.0G*/
	// Dword 55-58
	FFE_CONTROL  FFE_Control[16];		/* 16 bytes for 16 PHY FFE seeting
							BIT 0~3 : FFE Capacitor select(value range 0~F)
							BIT 4~6 : FFE Resistor select(value range 0~7)
							BIT 7: reserve. */ 

	//Driver will depend on each parameter setting of link rate and write to register
	// Dword 59-74
	PHY_TUNING   PHY_Tuning_Gen4[16];	/* 64 bytes, PHY tuning parameters for each PHY*/
	// Dword 75-90
	PHY_TUNING   PHY_Tuning_Gen3[16];	/* 64 bytes, PHY tuning parameters for each PHY*/
	// Dword 91-106
	PHY_TUNING   PHY_Tuning_Gen2[16];	/* 64 bytes, PHY tuning parameters for each PHY*/
	// Dword 107-122
	PHY_TUNING   PHY_Tuning_Gen1[16];	/* 64 bytes, PHY tuning parameters for each PHY*/
	// Dword 123-126
	PHY_TUNING_State PHY_Tuning_State[16];	/* 16 bytes, To check each phy need to tunned or not*/

	//Dword 127
	MV_U8		Reserved3[3];
	MV_U8		check_sum; /* checksum of data part of phy info structure (exclude header) */
}Phy_Info_Page, *pPhy_Info_Page;
#else
typedef struct _phy_info_page{
    // DWORD 0 - 2
    page_header header; /* refer to 3.1 HBA information page header */

    // DWORD 3
    MV_U8 reserved_1[4];
    
    // DWORD 4 - 11
    PHY_TUNING  phy_tuning_1[8];    //PHY tuning parameters for each PHY for gen 1.
    // DWORD 12 - 19
    MV_U32      reserved_2[8];      //Reserved for another 8 port in the future.
    // DWORD 20 - 27
    PHY_TUNING  phy_tuning_2[8];    //PHY tuning parameters for each PHY for gen 2.
    // DWORD 28 - 35
    MV_U32      reserved_3[8];      //Reserved for another 8 port in the future.
    // DWORD 36 - 43
    PHY_TUNING  phy_tuning_3[8];    //PHY tuning parameters for each PHY for gen 3.
    // DWORD 44 - 51
    MV_U32      reserved_4[8];      //Reserved for another 8 port in the future.

    //DWORD 52 - 53
	FFE_CONTROL  FFE_Control[8];
    //DWORD 54 - 55
    MV_U8       reserved_5[8];      //Reserved for another 8 port in the future.
    // DWORD 56 - 57
	MV_U8     	PHY_Rate[8];        //0:  1.5G, 1: 3.0G, 2: 6.0G
    // DWORD 58 - 59
    MV_U8       reserved_6[8];      //Reserved for another 8 port in the future.
    // DWORD 60 - 63
    MV_U8       reserved_7[16];
} phy_info_page, *pphy_info_page;
#endif

#define FLASH_PARAM_SIZE 	(sizeof(HBA_Info_Page))
#define HBA_INFO_PAGE_SIZE 	(sizeof(hba_info_page))
#define PHY_INFO_PAGE_SIZE 	(sizeof(phy_info_page))
#define NEXT_PAGE_OFFSET    0x2000                  //8KB
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define ATHENA_FLASH_SIZE	0x100000L					//1024K bytes
#define PARAM_OFFSET		ATHENA_FLASH_SIZE - 0x100	//255k bytes
#else
#define ODIN_FLASH_SIZE		0x40000  				//.256k bytes
#define PARAM_OFFSET		ODIN_FLASH_SIZE - 0x100 //.255k bytes
#endif
#ifdef SCSI_ID_MAP
#define FLASH_TABLE_SIZE     (MV_MAX_TARGET_NUMBER * 8 * sizeof(MV_U64))
#define PD_INFO_PAGE_OFFSET  (FLASH_PARAM_SIZE + 0x100)
#endif
#if defined(_OS_FIRMWARE)
#if defined(ADDING_FLASH_LAYOUT_DESC)
#include "com_flash.h"
#define HBA_INFO_PRIMARY_OFFSET           return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 0, FLASH_IMAGE_RETURN_ADDRESS)
#define HBA_INFO_SECONDARY_OFFSET	return_image_place_on_flash(FLASH_IMAGE_HBAINFO, 1, FLASH_IMAGE_RETURN_ADDRESS)
#define HBA_INFO_OFFSET				HBA_INFO_PRIMARY_OFFSET

#ifdef SUPPORT_ROC
#define PHY_INFO_PRIMARY_OFFSET      0x3e2000
#define PHY_INFO_SECONDARY_OFFSET	0x3f2000
#endif

#define FLASH_PDINFOPAGE_SIZE			return_image_place_on_flash(FLASH_IMAGE_PDINFOPAGE, 0, FLASH_IMAGE_RETURN_SIZE)
#define FLASH_PDINFOPAGE_OFFSET		0x00020000
#define FLASH_PDINFOPAGE_ADDR			return_image_place_on_flash(FLASH_IMAGE_PDINFOPAGE, 0, FLASH_IMAGE_RETURN_ADDRESS)

#if defined(SAVE_EVENTS_TO_FLASH)
#define EVENT_LOG_HEADER_OFFSET		return_image_place_on_flash(FLASH_IMAGE_EVENTLOG, 0, FLASH_IMAGE_RETURN_ADDRESS)
#define EVENT_LOG_ENTRY_OFFSET(slot)	(event_log_header_offset + sizeof(struct flash_event_log_head) + (sizeof(struct flash_event_log_entry) * slot))
#endif	
#define MAX_HBA_INFO_PAGE_SIZE      0x10000
#else
#	if defined(SUPPORT_BALDUR)
#define HBA_INFO_PRIMARY_OFFSET		0x3F0000	/*Refer to Flash Layout*/
#define HBA_INFO_SECONDARY_OFFSET	(HBA_INFO_PRIMARY_OFFSET + 0x2000)
#define HBA_INFO_OFFSET				HBA_INFO_PRIMARY_OFFSET
#define FLASH_PDINFOPAGE_SIZE			0x00020000
#define FLASH_PDINFOPAGE_OFFSET		0x00020000
#define FLASH_PDINFOPAGE_ADDR			0x00060000
#		if defined(SAVE_EVENTS_TO_FLASH)
#define EVENT_LOG_HEADER_OFFSET		0x00080000
#define EVENT_LOG_ENTRY_OFFSET(slot)	(EVENT_LOG_HEADER_OFFSET + sizeof(struct flash_event_log_head) + (sizeof(struct flash_event_log_entry) * slot))
#		endif
#	else
#define HBA_INFO_PRIMARY_OFFSET		0x00040000	/*Refer to Flash Layout*/
#define HBA_INFO_SECONDARY_OFFSET	0x000e0000
#define HBA_INFO_OFFSET				HBA_INFO_PRIMARY_OFFSET
#define FLASH_PDINFOPAGE_SIZE			0x00020000
#define FLASH_PDINFOPAGE_OFFSET		0x00020000
#define FLASH_PDINFOPAGE_ADDR			0x00060000
#		if defined(SAVE_EVENTS_TO_FLASH)
#define EVENT_LOG_HEADER_OFFSET		0x00080000
#define EVENT_LOG_ENTRY_OFFSET(slot)	(EVENT_LOG_HEADER_OFFSET + sizeof(struct flash_event_log_head) + (sizeof(struct flash_event_log_entry) * slot))
#		endif	
#	endif
#endif
#endif

// PD infomation Page
#define FLASH_PD_INFO_PAGE_SIZE 	(sizeof(pd_info_page))
#define PAGE_INTERVAL_DISTANCE		0x100

#define PD_PAGE_PD_ID_INAVAILABLE 	0
#define PD_PAGE_PD_ID_AVAILABLE 	1

#define IS_PD_PAGE_PD_ID_AVAILABLE(flag) ((flag)==PD_PAGE_PD_ID_AVAILABLE)

#define PAGE_CODE_HBA_INFO	0
#define PAGE_CODE_PD_INFO	1
#define PAGE_CODE_AES_INFO	2
#define PAGE_CODE_EVT_INFO	3
#define PAGE_CODE_CRYPTO_PD     4
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
#define PAGE_CODE_PHY_INFO			0x05
#define PAGE_CODE_NULL_PAGE			0xFF
#define PAGE_CODE_TAIL_PAGE			PAGE_CODE_NULL_PAGE
#define SEARCH_ADDR_DECREASE		MV_TRUE
#else
#define PAGE_CODE_PHY_TUNING    5
#endif
#define PAGE_CODE_OEM_CFG         6


#define PD_PAGE_PD_STATE_OFFLINE 0
#define PD_PAGE_PD_STATE_ONLINE 1

#ifdef MULTI_PD_PAGE_SUPPORT
#define MAX_NUM_PD_PAGE	2
#else
#define MAX_NUM_PD_PAGE	1
#endif

#ifdef MULTI_PD_PAGE_SUPPORT
#ifdef SUPPORT_ROC
#define MULTI_PD_PAGE_BUFFER_SIZE  (MAX_NUM_PD_PAGE*(FLASH_PD_INFO_PAGE_SIZE+PAGE_INTERVAL_DISTANCE))// 
#endif
#define PAGE_BUFFER_SIZE  (MAX_NUM_PD_PAGE*(FLASH_PD_INFO_PAGE_SIZE+PAGE_INTERVAL_DISTANCE)+FLASH_PARAM_SIZE)
#else
#define PAGE_BUFFER_SIZE  (FLASH_PD_INFO_PAGE_SIZE+PAGE_INTERVAL_DISTANCE+FLASH_PARAM_SIZE)// total 0x1240 bytes(128 PDs); 0x2240 bytes(256 PDs)
#endif

/* This number may not equal to MAX_DEVICE_SUPPORTED_PERFORMANCE */
#define MAX_PD_IN_PD_PAGE_FLASH	128 // currently every PD page support 128 PD entry
#define MAX_PD_IN_TOTAL_PD_PAGE		(MAX_PD_IN_PD_PAGE_FLASH * MAX_NUM_PD_PAGE)
/* 
	PD_Info_Page is saved in Flash/NVRAM, currently total size is 0x1040 bytes.
	The data area is valid only Signature="MRVL".
	If any member fills with 0xFF, the member is invalid.
*/

typedef struct _Device_Index {
	MV_U16	index;
	MV_U16	device_id;
	MV_BOOLEAN end;
}Device_Index,*PDevice_Index;

typedef void(*get_device_id) (MV_PVOID, MV_U16,PDevice_Index);

typedef struct _pd_status_info {
	MV_U16	pd_id;
	MV_U8	status;
	MV_U8	reserved;
}pd_status_info,*p_pd_status_info;

struct _pd_info {
	// Dword 0-1
	MV_U64	pd_guid;

	// Dword 2
	MV_U16	pd_id;
	MV_U16	pd_scsi_id;

	// Dword 3
	MV_U8	status;
	MV_U8	type;
	MV_U8	cache_policy;
	MV_U8	reserved1;
	
	// Dword 4-7
	MV_U8	reserved2[16];
};


struct _pd_info_page{

	page_header header;

	MV_U8	reserved2[52];		/* 52 byte,   reserve space for future,initial as 0xFF */

	struct _pd_info pd_info_data[MAX_PD_IN_PD_PAGE_FLASH]; /* MAX_PD_IN_PD_PAGE_FLASH*32 bytes, could save 128 PD data in flash */
};

typedef struct _pci_auto_load_config
{
    MV_U8        signature[4];
    MV_U32      data[1];
} pci_auto_load_config;

typedef struct _pd_info pd_info, *p_pd_info;
typedef struct _pd_info_page pd_info_page, *p_pd_info_page;

#if defined(ENABLE_PHYTUNING)
#define PHY_INFO_PAGE_MAJOR_VER 1
#define PHY_INFO_PAGE_MINOR_VER 1

typedef struct _phy_info_page_st{
    // DWORD 0 - 2
    page_header    header; /* refer to 3.1 HBA information page header */
    // DWORD 3
    MV_U8 major_ver;	/* major version of this page.*/
    MV_U8 minor_ver;	/* minor version of this page.*/
    MV_U8 reserved_1[2];
    // DWORD 4 - 11
    MV_U32          phy_tuning_1[8];   //PHY tuning parameters for each PHY for gen 1.
    // DWORD 12 - 19
    MV_U32          phy_tuning_2[8];   //PHY tuning parameters for each PHY for gen 2.
    // DWORD 20 - 27
    MV_U32          phy_tuning_3[8];   //PHY tuning parameters for each PHY for gen 3.
    // DWORD 28 - 63
    MV_U8	        reserved_2[143];
    MV_U8           page_checksum;  /* checksum of data part of phy info structure (exclude header)*/
}phy_info_page_t;

//typedef _phy_info_page_st phy_info_page_t,*p_phy_info_page_t;
#endif

#if defined(ENABLE_RAID_OEM_CFG_PAGE_INFO)
#define MODIFYING_RAID_OEM_CONFIG_PAGE_MAJOR_VER 1
#define MODIFYING_RAID_OEM_CONFIG_PAGE_MINOR_VER 2

typedef struct _raid_oem_config_page_st{
    // DWORD 0 - 2
    page_header    header;
    // DWORD 3 - 12
    MV_U8             vd_name[40];
    // DWORD 13
    MV_U32           feature_set_low;
    // DWORD 14
    MV_U32           feature_set_high;
    // DWORD 15 - 63
    MV_U32           reserved_u32[49];
} raid_oem_config_page_t;

typedef struct _raid_oem_config_cell_st{
    MV_U16           size;
    MV_U16           type;
} raid_oem_config_cell_t;

#define ENABLE_OEM_FEATURE(x, bit) ((bit >= 32) ? ((x)->feature_set_high |= (1L << (bit - 32))) : ((x)->feature_set_low |= (1L << bit)))
#define DISABLE_OEM_FEATURE(x, bit) ((bit >= 32) ? ((x)->feature_set_high &= ~(1L << (bit - 32))) : ((x)->feature_set_low &= ~(1L << bit)))
#define CHECK_OEM_FEATURE(x, bit) ((bit >= 32) ? ((x)->feature_set_high & (1L << (bit - 32))) : ((x)->feature_set_low & (1L << bit)))

#define GET_OEM_CFG_CELL_SIZE(x) (x->size)
#define GET_OEM_CFG_CELL_TYPE(x) (x->type)
#define GET_OEM_CFG_CELL_HEADER(x) ((u8 *)x - x->size)

#define OEM_CFG_MODIFIED_VDNAME_DISABLE                          0
#define OEM_CFG_PNP_AUTO_REBUILD_DISABLE                         1
#define OEM_CFG_UPGRADED_FW_DISABLE                                 2
#define OEM_CFG_RAID_CONFIG_DISABLE                                   3
#define OEM_CFG_BUZZER_DISABLE                                             4
#define OEM_CFG_BIOS_POST_SHOWING_TIME_DISABLE            5
#define OEM_CFG_BIOS_SCANING_DIRECTION                             6
#define OEM_CFG_BIOS_INITIALING_STRING                               7
#define OEM_CFG_AES_OS_BOOTING                                            8
#define OEM_CFG_SMART_CHECK                                                  9
#define OEM_CFG_HALT_ON_ERROR                                              10
#define OEM_CFG_RAID0_MIGRATE_RAID1                                   11
#define OEM_CFG_FRONTEND_PORT_SETTING                              12
#endif

#if defined(ADDING_FLASH_LAYOUT_DESC)
MV_VOID initial_flash_layout(void);
#endif

#if defined(SAVE_EVENTS_TO_FLASH)
#include "com_event_struct.h"
#if defined(SUPPORT_NVRAM)
MV_PVOID finding_hander_from_nvram(MV_PU8 signature);
MV_PVOID gotting_free_space_from_nvram(void);
MV_BOOLEAN wipe_out_header_in_nvram(MV_PU8 signature);
#endif /*SUPPORT_NVRAM*/
void initial_flash_eventlog(MV_PVOID This);
MV_U16 check_remaining_events_in_flash(MV_PVOID This);

MV_BOOLEAN add_event_to_flash( 
	MV_PVOID This,
#if defined(ROC_V2_EVENT_SUPPORT)
	PDriverEvent_V2 ptr_driver_event
#else
	PDriverEvent ptr_driver_event
#endif
    );
#endif /*SAVE_EVENTS_TO_FLASH*/
 
extern MV_BOOLEAN mv_page_signature_check( MV_PU8 Signature );

//extern MV_U8 mvui_init_param(MV_PVOID This, pHBA_Info_Page pHBAInfo);//get initial data from flash
//extern MV_BOOLEAN mvuiHBA_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);
#ifdef SUPPORT_ROC
MV_BOOLEAN mv_nvram_init_param(MV_PVOID This, phba_info_page pHBA_Info_Param);
MV_VOID fill_default_hba_info(MV_PVOID this, phba_info_page pHBA_Info_Param);
extern MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, phba_info_page pHBA_Info_Param);
#else
#if defined(SUPPORT_ATHENA) || defined(SUPPORT_SP2)
MV_BOOLEAN mv_nvram_init_phy_param(MV_PVOID This, pPhy_Info_Page pPhy_Info_Param);
#endif
MV_BOOLEAN mv_nvram_init_param(MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);
extern MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);
#endif
MV_BOOLEAN mv_nvram_moddesc_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);

MV_BOOLEAN mv_read_hba_info( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset);
MV_BOOLEAN mv_read_autoload_data( MV_PVOID This, char *pBuffer, MV_U16 buf_len, int *offset);
/* Caution: Calling this function, please do Read-Modify-Write. 
 * Please call to get the original data first, then modify corresponding field,
 * Then you can call this function. */
extern MV_U8	mvCalculateChecksum(MV_PU8	Address, MV_U32 Size);
extern MV_U8	mvVerifyChecksum(MV_PU8	Address, MV_U32 Size);
#endif		/* COM_NVRAM_H */
