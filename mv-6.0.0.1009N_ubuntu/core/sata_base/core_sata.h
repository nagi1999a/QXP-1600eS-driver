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

#ifndef __CORE_SATA_H
#define __CORE_SATA_H

#include "mv_config.h"
#include "core_type.h"

MV_BOOLEAN sata_port_state_machine(MV_PVOID dev);
MV_BOOLEAN sata_device_state_machine(MV_PVOID dev);

MV_VOID sata_port_notify_event(MV_PVOID port_p, MV_U32 irq_status);
MV_U8 sata_verify_command(MV_PVOID root_p, MV_PVOID dev,
       MV_Request *req);
MV_U8 old_sata_verify_command(MV_PVOID root_p, MV_PVOID dev,
       MV_Request *req);
MV_VOID sata_prepare_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmd_header, MV_PVOID cmd_table, 
	MV_Request *req);
MV_VOID sata_send_command(
            MV_PVOID root_p,
            MV_PVOID dev,
            MV_PVOID struct_wrapper,
            MV_Request *req);
MV_VOID sata_process_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table, 
	MV_Request *req);
MV_BOOLEAN sata_check_complete_command(MV_PVOID root_p, MV_Request *req);
MV_BOOLEAN sata_detect_phy_rdy(MV_PVOID sata_port);

void sata_get_ncq_tag(MV_PVOID dev_p, MV_Request *req);
void sata_free_ncq_tag(MV_PVOID dev_p, MV_Request *req);
/*
 * ======================================================================
 * ATA definition 
 * ======================================================================
 */

/* PIO command */
#define ATA_CMD_READ_PIO				0x20
#define ATA_CMD_READ_PIO_MULTIPLE                       0xC4
#define ATA_CMD_READ_PIO_EXT			0x24
#define ATA_CMD_READ_PIO_MULTIPLE_EXT 	0x29
#define ATA_CMD_WRITE_PIO				0x30
#define ATA_CMD_WRITE_PIO_MULTIPLE                      0xC5
#define ATA_CMD_WRITE_PIO_EXT			0x34
#define ATA_CMD_WRITE_PIO_MULTIPLE_EXT	0x39
#define ATA_CMD_WRITE_PIO_MULTIPLE_FUA_EXT              0xCE

#define IS_ATA_MULTIPLE_READ_WRITE(x) \
        ((x == ATA_CMD_READ_PIO_MULTIPLE)\
        ||(x == ATA_CMD_READ_PIO_MULTIPLE_EXT)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE_EXT)\
        ||(x == ATA_CMD_WRITE_PIO_MULTIPLE_FUA_EXT))

/* DMA read write command */
#define ATA_CMD_READ_DMA				0xC8	/* 24 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED			0xC7	/* 24 bit TCQ DMA read */
#define ATA_CMD_READ_DMA_EXT			0x25	/* 48 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED_EXT		0x26	/* 48 bit TCQ DMA read */
#define ATA_CMD_READ_FPDMA_QUEUED		0x60	/* NCQ DMA read: SATA only. 
												 * Always 48 bit */

#define ATA_CMD_WRITE_DMA				0xCA	
#define ATA_CMD_WRITE_DMA_QUEUED		0xCC
#define ATA_CMD_WRITE_DMA_EXT  			0x35
#define ATA_CMD_WRITE_DMA_QUEUED_EXT	0x36
#define ATA_CMD_WRITE_FPDMA_QUEUED		0x61

/* Identify command */
#define ATA_CMD_IDENTIFY_ATA			0xEC
#define ATA_CMD_IDENTIFY_ATAPI			0xA1

#define ATA_CMD_VERIFY					0x40	/* 24 bit read verifty */
#define ATA_CMD_VERIFY_EXT				0x42	/* 48 bit read verify */

#define ATA_CMD_FLUSH					0xE7	/* 24 bit flush */
#define ATA_CMD_FLUSH_EXT				0xEA	/* 48 bit flush */

#define ATA_CMD_PACKET					0xA0
#define ATA_CMD_SMART					0xB0
	#define ATA_CMD_SMART_READ_DATA 					0xD0
        #define ATA_CMD_SMART_READ_ATTRIBUTE_THRESHOLDS 0xD1
	#define ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE	0xD2
        #define ATA_CMD_SMART_SAVE_ATTRIBUTE_VALUES     0xD3
	#define ATA_CMD_SMART_EXECUTE_OFFLINE				0xD4
	#define ATA_CMD_SMART_READ_LOG						0xD5
	#define ATA_CMD_SMART_WRITE_LOG 					0xD6
	#define ATA_CMD_ENABLE_SMART				0xD8
	#define ATA_CMD_DISABLE_SMART				0xD9
	#define ATA_CMD_SMART_RETURN_STATUS			0xDA

#define ATA_CMD_SET_FEATURES			0xEF
	#define ATA_CMD_ENABLE_WRITE_CACHE			0x02
	#define ATA_CMD_SET_TRANSFER_MODE			0x03
	#define ATA_CMD_SET_POIS_SPINUP			0x07
	#define ATA_CMD_ENABLE_DIPM			0x10
	#define ATA_CMD_DISABLE_READ_LOOK_AHEAD		0x55
	#define ATA_CMD_DISABLE_REVERT_TO_DEFAULT	0x66
	#define ATA_CMD_DISABLE_WRITE_CACHE			0x82
	#define ATA_CMD_DISABLE_DIPM			0x90
	#define ATA_CMD_ENABLE_READ_LOOK_AHEAD		0xAA
	#define ATA_CMD_ENABLE_REVERT_TO_DEFAULT	0xCC

#define ATA_CMD_SEEK					0x70
#define ATA_CMD_READ_LOG_EXT			0x2F
#define ATA_CMD_DOWNLOAD_MICROCODE		0x92

/* Trusted command  */
#define ATA_CMD_TRUSTED_NON_DATA		0x5B
#define ATA_CMD_TRUSTED_RECEIVED			0x5C
#define ATA_CMD_TRUSTED_RECEIVED_DMA	0x5D
#define ATA_CMD_TRUSTED_SEND				0x5E
#define ATA_CMD_TRUSTED_SEND_DMA		0x5F

/* PM commands & registers */
#define ATA_CMD_PM_READ				0xE4
#define ATA_CMD_PM_WRITE			0xE8
#ifndef ATA_CMD_PM_CHECK
#define ATA_CMD_PM_CHECK			0xE5
#endif
#define ATA_CMD_MEDIA_EJECT                     0xED
#define ATA_CMD_IDLE                            0xE3
#define ATA_CMD_IDLE_IMMEDIATE                  0xE1
#define ATA_CMD_STANDBY                         0xE2
#define ATA_CMD_STANDBY_IMMEDIATE               0xE0
#define ATA_CMD_SLEEP                           0xE6
#define ATA_CMD_EXECUTE_DEVICE_DIAGNOSTIC		0x90

struct _ata_taskfile;
typedef struct _ata_taskfile ata_taskfile;

struct _ata_taskfile {
	MV_U8	features;
	MV_U8	sector_count;
	MV_U8	lba_low;
	MV_U8	lba_mid;
	MV_U8	lba_high;
	MV_U8	device;
	MV_U8	command;

	MV_U8	control;

	/* extension */
	MV_U8	feature_exp;
	MV_U8	sector_count_exp;
	MV_U8	lba_low_exp;
	MV_U8	lba_mid_exp;
	MV_U8	lba_high_exp;
};

/* ATA device identify frame */
typedef struct _ata_identify_data {
	MV_U16 general_config;							/*	0	*/
	MV_U16 obsolete0;								/*	1	*/
	MV_U16 specific_config;							/*	2	*/
	MV_U16 obsolete1;								/*	3	*/
	MV_U16 retired0[2];								/*	4-5	*/
	MV_U16 obsolete2;								/*	6	*/
	MV_U16 reserved0[2];							/*	7-8	*/
	MV_U16 retired1;								/*	9	*/
	MV_U8 serial_number[20];				        /*	10-19	*/
	MV_U16 retired2[2];								/*	20-21	*/
	MV_U16 obsolete3;								/*	22	*/
	MV_U8 firmware_revision[8];						/*	23-26	*/
	MV_U8 model_number[40];							/*	27-46	*/
	MV_U16 maximum_block_transfer;					/*	47	*/
	MV_U16 reserved1;								/*	48	*/
	MV_U16 capabilities[2];							/*	49-50	*/
	MV_U16 obsolete4[2];							/*	51-52	*/
	MV_U16 fields_valid;							/*	53	*/
	MV_U16 obsolete5[5];							/*	54-58	*/
	MV_U16 current_multiple_sector_setting;			/*	59	*/
	MV_U16 user_addressable_sectors[2];				/*	60-61	*/
	MV_U16 atapi_dmadir;							/*	62	*/
	MV_U16 multiword_dma_modes;						/*	63	*/
	MV_U16 pio_modes;								/*	64	*/
	MV_U16 minimum_multiword_dma_cycle_time;		/*	65	*/
	MV_U16 recommended_multiword_dma_cycle_time;	/*	66	*/
	MV_U16 minimum_pio_cycle_time;					/*	67	*/
	MV_U16 minimum_pio_cycle_time_iordy;			/*	68	*/
	MV_U16 reserved2[2];							/*	69-70	*/
	MV_U16 atapi_reserved[4];						/*	71-74	*/
	MV_U16 queue_depth;								/*	75	*/
	MV_U16 sata_capabilities;						/*	76	*/
	MV_U16 sata_reserved;							/*	77	*/
	MV_U16 sata_feature_supported;					/*	78	*/
	MV_U16 sata_feature_enabled;					/*	79	*/
 	MV_U16 major_version;							/*	80	*/
	MV_U16 minor_version;							/*	81	*/
	MV_U16 command_set_supported[2];				/*	82-83	*/
	MV_U16 command_set_supported_extension;			/*	84	*/
	MV_U16 command_set_enabled[2];					/*	85-86	*/
	MV_U16 command_set_default;						/*	87	*/
	MV_U16 udma_modes;								/*	88	*/
	MV_U16 time_for_security_erase;					/*	89	*/
	MV_U16 time_for_enhanced_security_erase;		/*	90	*/
	MV_U16 current_advanced_power_manage_value;		/*	91	*/
	MV_U16 master_password_revision;				/*	92	*/
	MV_U16 hardware_reset_result;					/*	93	*/
	MV_U16 acoustic_manage_value;					/*	94	*/
	MV_U16 stream_minimum_request_size;				/*	95	*/
	MV_U16 stream_transfer_time_dma;				/*	96	*/
	MV_U16 stream_access_latency;					/*	97	*/
	MV_U16 stream_performance_granularity[2];		/*	98-99	*/
	MV_U16 max_lba[4];								/*	100-103	*/
	MV_U16 stream_transfer_time_pio;				/*	104	*/	
	MV_U16 reserved3;								/*	105	*/
	MV_U16 physical_logical_sector_size;			/*	106	*/
	MV_U16 delay_acoustic_testing;					/*	107	*/
	MV_U16 naa;										/*	108	*/
	MV_U16 unique_id1;								/*	109	*/
	MV_U16 unique_id2;								/*	110	*/
	MV_U16 unique_id3;								/*	111	*/
	MV_U16 reserved4[4];							/*	112-115	*/
	MV_U16 reserved5;								/*	116	*/
	MV_U16 words_per_logical_sector[2];				/*	117-118	*/
	MV_U16 reserved6[8];							/*	119-126	*/
	MV_U16 removable_media_status_notification;		/*	127	*/
	MV_U16 security_status;							/*	128	*/
	MV_U16 vendor_specific[31];						/*	129-159	*/
	MV_U16 cfa_power_mode;							/*	160	*/
	MV_U16 reserved7[15];							/*	161-175	*/
	MV_U16 current_media_serial_number[30];			/*	176-205	*/
	MV_U16 reserved8[49];							/*	206-254	*/
	MV_U16 integrity_word;							/*	255	*/
} ata_identify_data;

#define IDENTIFY_CMD_PACKET_SET_MASK 0x1f00
#define IDENTIFY_CMD_PACKET_SET_TAPE 0x0100
#define IDENTIFY_GENERAL_CONFIG_ATAPI MV_BIT(15)

#define ATA_REGISTER_DATA			0x08
#define ATA_REGISTER_ERROR			0x09
#define ATA_REGISTER_FEATURES		0x09
#define ATA_REGISTER_SECTOR_COUNT	0x0A
#define ATA_REGISTER_LBA_LOW		0x0B
#define ATA_REGISTER_LBA_MID		0x0C
#define ATA_REGISTER_LBA_HIGH		0x0D
#define ATA_REGISTER_DEVICE			0x0E
#define ATA_REGISTER_STATUS			0x0F
#define ATA_REGISTER_COMMAND		0x0F

#define ATA_REGISTER_ALT_STATUS		0x16
#define ATA_REGISTER_DEVICE_CONTROL	0x16

#define MAX_MODE_PAGE_LENGTH            66
#define MAX_MODE_LOG_LENGTH             404

/*
 * ======================================================================
 * SATA definition 
 * ======================================================================
 */

#define FIS_REG_H2D_SIZE_IN_DWORD	5

#define RX_FIS_DMA_REG	0x00
#define RX_FIS_PIO_REG	0x20
#define RX_FIS_RES_REG	0x34
#define RX_FIS_D2H_REG	0x40
#define RX_FIS_SDB_REG	0x58
#define RX_FIS_UNK_REG	0x60


/* offset for SATA FIS areas */
#define SATA_RECEIVED_DMA_FIS(root, reg_set)  (root->unassoc_fis_offset + \
						0x100 * reg_set + RX_FIS_DMA_REG)
#define SATA_RECEIVED_PIO_FIS(root, reg_set)  (root->unassoc_fis_offset + \
						0x100 * reg_set + RX_FIS_PIO_REG)
#define SATA_RECEIVED_D2H_FIS(root, reg_set)  (root->unassoc_fis_offset + \
						0x100 * reg_set + RX_FIS_D2H_REG)
#define SATA_RECEIVED_SDB_FIS(root, reg_set)  (root->unassoc_fis_offset + \
						0x100 * reg_set + RX_FIS_SDB_REG)

#define SATA_UNASSOC_D2H_FIS(phy)	(0x100 * phy->asic_id)
#define SATA_UNASSOC_SDB_FIS(phy) (0x100 * phy->asic_id)

/* FIS type definition */
#define SATA_FIS_TYPE_REG_H2D			0x27	/* Register FIS - Host to Device */
#define SATA_FIS_TYPE_REG_D2H			0x34	/* Register FIS - Device to Host */

#define SATA_FIS_TYPE_DMA_ACTIVATE		0x39	/* DMA Activate FIS - Device to Host */
#define SATA_FIS_TYPE_DMA_SETUP			0x41	/* DMA Setup FIS - Bi-directional */

#define SATA_FIS_TYPE_DATA				0x46	/* Data FIS - Bi-directional */
#define SATA_FIS_TYPE_BIST_ACTIVATE		0x58	/* BIST Activate FIS - Bi-directional */
#define SATA_FIS_TYPE_PIO_SETUP			0x5F	/* PIO Setup FIS - Device to Host */
#define SATA_FIS_TYPE_SET_DEVICE_BITS	0xA1	/* Set Device Bits FIS - Device to Host */

#define H2D_COMMAND_SET_FLAG 0x80

/* SATA FIS: Register-Host to Device*/
typedef struct _sata_fis_reg_h2d
{
	MV_U8	fis_type;
	MV_U8	cmd_pm;
	MV_U8	command;
	MV_U8	features;

	MV_U8	lba_low;
	MV_U8	lba_mid;
	MV_U8	lba_high;
	MV_U8	device;

	MV_U8	lba_low_exp;
	MV_U8	lba_mid_exp;
	MV_U8	lba_high_exp;
	MV_U8	features_exp;

	MV_U8	sector_count;
	MV_U8	sector_count_exp;
	MV_U8	ICC;				//ICC (SATA 3.0) - Isochronous Command Completion (ICC) contains a value is set by the host to inform device of a time limit. If a command does not define the use of this field, it shall be reserved.
	MV_U8	control;

	MV_U8	reserved2[4];

} sata_fis_reg_h2d;

typedef struct _sata_fis_reg_d2h
{
	MV_U8 fis_type;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U8	reserved0 : 1;
	MV_U8	interrupt : 1;
	MV_U8	reserved1 : 2;
	MV_U8	pm_port : 4;
#else
	MV_U8	pm_port : 4;
	MV_U8	reserved1 : 2;
	MV_U8	interrupt : 1;
	MV_U8	reserved0 : 1;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
	MV_U8	status;
	MV_U8	error;

	MV_U8	lba_low;
	MV_U8	lba_mid;
	MV_U8	lba_high;
	MV_U8	device;

	MV_U8	lba_low_exp;
	MV_U8	lba_mid_exp;
	MV_U8	lba_high_exp;
	MV_U8	reserved2;

	MV_U8	sector_count;
	MV_U8	sector_count_exp;
	MV_U8	reserved3[2];

	MV_U8	reserved4[4];
} sata_fis_reg_d2h;

/* FIS TYPE 5F : PIO setup FIS D2H*/
typedef struct _sata_fis_pio_setup_d2h
{
	MV_U8 fis_type;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U8	reserved0 : 1;
	MV_U8	interrupt : 1;
	MV_U8	direction : 1;
	MV_U8	reserved1 : 1;
	MV_U8	pm_port : 4;
#else
	MV_U8	pm_port : 4;
	MV_U8	reserved1 : 1;
	MV_U8	direction : 1;
    MV_U8   interrupt : 1;
	MV_U8	reserved0 : 1;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
	MV_U8	status;
	MV_U8	error;

	MV_U8	lba_low;
	MV_U8	lba_mid;
	MV_U8	lba_high;
	MV_U8	device;

	MV_U8	lba_low_exp;
	MV_U8	lba_mid_exp;
	MV_U8	lba_high_exp;
	MV_U8	reserved2;

	MV_U8	count;
	MV_U8	count_exp;
	MV_U8	reserved3;
	MV_U8	e_status;

    MV_U16  transfer_count;
	MV_U8	reserved4[2];
} sata_fis_pio_setup_d2h;


#define NO_REGISTER_SET			0xFF

#define SMART_DATA_LENGTH   512

#ifndef _OS_BIOS
#pragma pack(1)
#endif /* _OS_BIOS */

typedef struct _ata_reg
{
     MV_U8        drive_regs[7];
     MV_U8        bReserved;
     MV_U32       buffer_size;
     MV_U32       reserved;
     MV_U8        data[1];
}ata_reg, pata_reg;

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

enum _ata_err_byte {
        ATA_ERR_ICRC    = MV_BIT(7),
        ATA_ERR_UNC     = MV_BIT(6),
        ATA_ERR_MC      = MV_BIT(5),
        ATA_ERR_IDNF    = MV_BIT(4),
        ATA_ERR_MCR     = MV_BIT(3),
        ATA_ERR_ABRT    = MV_BIT(2),
        ATA_ERR_NM      = MV_BIT(1),
        ATA_ERR_MED     = MV_BIT(0),
};

enum _ata_status_byte {
        ATA_STATUS_BSY  = MV_BIT(7),
        ATA_STATUS_DRDY = MV_BIT(6),
        ATA_STATUS_DF   = MV_BIT(5),
        ATA_STATUS_DRQ  = MV_BIT(3),
        ATA_STATUS_ERR  = MV_BIT(0),
};

enum _ata_self_test_code {
        DEFAULT_SELF_TEST                       = 0x0,
        BACKGROUND_SHORT_SELF_TEST              = 0x1,
        BACKGROUND_EXTENDED_SELF_TEST           = 0x2,
        ABORT_BACKGROUND_SELF_TEST              = 0x4,
        FOREGROUND_SHORT_SELF_TEST              = 0x5,
        FOREGROUND_EXTENDED_SELF_TEST           = 0x6,
};

enum _ata_passthru_protocol {
        ATA_PROTOCOL_HARD_RESET                 = 0x00,
        ATA_PROTOCOL_SRST                       = 0x01,
        ATA_PROTOCOL_NON_DATA                   = 0x03,
        ATA_PROTOCOL_PIO_IN                     = 0x04,
        ATA_PROTOCOL_PIO_OUT                    = 0x05,
        ATA_PROTOCOL_DMA                        = 0x06,
        ATA_PROTOCOL_DMA_QUEUED                 = 0x07,
        ATA_PROTOCOL_DEVICE_DIAG                = 0x08,
        ATA_PROTOCOL_DEVICE_RESET               = 0x09,
        ATA_PROTOCOL_UDMA_IN                    = 0x0A,
        ATA_PROTOCOL_UDMA_OUT                   = 0x0B,
        ATA_PROTOCOL_FPDMA                      = 0x0C,
        ATA_PROTOCOL_RTN_INFO                   = 0x0F,
};

enum _ata_passthru_t_length {
    ATA_PASSTHRU_LENGTH_IN_ZERO         = 0,
    ATA_PASSTHRU_LENGTH_IN_FEATURE      = 1,
    ATA_PASSTHRU_LENGTH_IN_SEC_CNT      = 2,
    ATA_PASSTHRU_LENGTH_IN_STPSIU       = 3,
};

enum _ata_passthru_t_dir {
    ATA_PASSTHRU_TDIR_OUT        = 0,
    ATA_PASSTHRU_TDIR_IN         = 1,
};

// Workaround for SiliconPower eSATA SSD, this device has problem when sent with NCQ cmd
#define WORKAROUND_SILICONPOWER_ESATASSD_DISABLENCQ(device)               \
    if (device->model_number[0]=='e' && device->model_number[1]=='S'      \
        && device->model_number[2]=='A' && device->model_number[3]=='T'   \
        && device->model_number[4]=='A' && device->model_number[5]==' '   \
        && device->model_number[6]=='S' && device->model_number[7]=='S'   \
        && device->model_number[8]=='D')                                  \
   		    device->capability &= ~DEVICE_CAPABILITY_NCQ_SUPPORTED;


#endif /* __CORE_SATA_H */
