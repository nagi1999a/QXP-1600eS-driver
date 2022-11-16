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

#ifndef __CORE_I2C_H
#define __CORE_I2C_H

#include "mv_config.h"

typedef struct _domain_i2c_link
{
	MV_U8 i2c_state;
	MV_U8 i2c_clock;
	MV_U16 i2c_cmd_frame_len;
	MV_PVOID i2c_request;
	MV_U16 i2c_rsp_frame_len;
	MV_U16 i2c_xfer_count;
	MV_U8 i2c_sep_address;
	MV_U8 i2c_cmd_header[8];
	MV_U8 i2c_identify[64];
#if defined(SUPPORT_I2C_SES)
	MV_U8 supported_page_count;
	MV_U8 supported_page[32];
#endif
#if defined(SUPPORT_ACTIVE_CABLE)
	MV_U8 cable_status;
	MV_U8 cable_id;
#endif
}domain_i2c_link;

typedef struct _lib_i2c {
	domain_port	i2c_port;     //i2c port
	domain_i2c_link i2c_link;
} lib_i2c;

#define I2C_SOFTWARE_CONTROL              0x1c
#define I2C_HARDWARE_CONTROL              0x20
#define I2C_STATUS_DATA                   0x24

		/* I2C_SOFTWARE_CONTROL bits */
typedef struct _reg_i2c_software_control
{
	MV_U32	data_port:8;
	MV_U32	address_port:3;
	MV_U32	reserved1:1;
	MV_U32	write_en:1;
	MV_U32	read_en:1;
	MV_U32	reserved2:18;
} reg_i2c_software_control;

		/* MI2C_SLAVE_ADDRESS bits */
typedef struct _reg_mi2c_slave_address
{
	MV_U8	gce:1; /* general call address enable */
	MV_U8	sla:7; /* Slave address 0-6 */
} reg_mi2c_slave_address;



typedef struct _reg_mi2c_control
{
	MV_U8	reserved:2;
	MV_U8	aak:1; /* Assert AcKnowledge */
	MV_U8	iflg:1; /* Interrupt FLaG */
	MV_U8	stp:1; /* Master Stop */
	MV_U8	sta:1; /* Master Start */
	MV_U8	enab:1; /* Bus Enable */
	MV_U8	ien:1; /* Interrupt Enable */
} reg_mi2c_control;

		/* MI2C_STATUS bits */
#define MI2C_STATUS_BUS_ERROR				0x00
#define MI2C_STATUS_START_TX				0x08
#define MI2C_STATUS_REP_START_TX			0x10
#define MI2C_STATUS_ADDR_W_TX_ACK			0x18
#define MI2C_STATUS_ADDR_W_TX_NAK			0x20
#define MI2C_STATUS_MDATA_TX_ACK			0x28
#define MI2C_STATUS_MDATA_TX_NAK			0x30
#define MI2C_STATUS_ARBLST_ADDR_DATA		0x38
#define MI2C_STATUS_ADDR_R_TX_ACK			0x40
#define MI2C_STATUS_ADDR_R_TX_NAK			0x48
#define MI2C_STATUS_MDATA_RX_ACK			0x50
#define MI2C_STATUS_MDATA_RX_NAK			0x58
#define MI2C_STATUS_SADDR_W_RX_ACK			0x60
#define MI2C_STATUS_ARBLST_SADDR_W_RX_ACK	0x68
#define MI2C_STATUS_GC_ADDR_RX_ACK			0x70
#define MI2C_STATUS_ARBLST_GC_RX_ACK		0x78
#define MI2C_STATUS_SADDR_DATA_RX_ACK		0x80
#define MI2C_STATUS_SADDR_DATA_RX_NAK		0x88
#define MI2C_STATUS_GC_DATA_RX_ACK			0x90
#define MI2C_STATUS_GC_DATA_RX_NAK			0x98
#define MI2C_STATUS_STOP_RX					0xA0
#define MI2C_STATUS_SADDR_R_RX_ACK			0xA8
#define MI2C_STATUS_ARBLST_SADDR_R_RX_ACK	0xB0
#define MI2C_STATUS_SDATA_TX_ACK			0xB8
#define MI2C_STATUS_SDATA_TX_NAK			0xC0
#define MI2C_STATUS_LAST_SDATA_TX_ACK		0xC8
#define MI2C_STATUS_ADDR2_W_TX_ACK			0xD0
#define MI2C_STATUS_ADDR2_W_TX_NAK			0xD8
#define MI2C_STATUS_ADDR2_R_TX_ACK			0xE0
#define MI2C_STATUS_ADDR2_R_TX_NAK			0xE8
#define MI2C_STATUS_NONE					0xF8

/* I2C_State in Core_Driver_Extension*/
enum 
{
	I2C_STATE_IDLE=0,
	I2C_STATE_CMD,
	I2C_STATE_RESPONSE,
	I2C_STATE_ERROR,
#ifdef SUPPORT_BBU
/* bbu i2c state */
	I2C_STATE_START  = 0x11,
	I2C_STATE_STOP,
	I2C_STATE_ADDR_SET_WT,
	I2C_STATE_ADDR_SET_RD,
	I2C_STATE_DATA_TRANSMIT,
	I2C_STATE_DATA_RECEIVE, 
	I2C_STATE_REPEATED_START,
	I2C_STATE_CHECK, 
	I2C_STATE_RESET = 0xFF,
#endif
};

		/* I2C_HARDWARE_CONTROL bits */
typedef struct _reg_i2c_hardware_control
{
	MV_U32	slv_addrs:8;
	MV_U32	bus_ptl:8;
	MV_U32	cmd_code:8;
	MV_U32	bus_ptl_start:1;
	MV_U32	reserved:6;
	MV_U32	intr:1;
} reg_i2c_hardware_control;

/* BUS_PTL value*/
#define BUS_PTL_WRITE_QUICK	0 /* Write quick. */
#define BUS_PTL_READ_QUICK	0x01 /* Read quick. */
#define BUS_PTL_SEND_BYTE	0x02 /* Send byte. */
#define BUS_PTL_RCV_BYTE	0x03 /* Receive byte. */
#define BUS_PTL_WRITE_BYTE	0x04 /* Write byte. */
#define BUS_PTL_READ_BYTE	0x05 /* Read byte. */
#define BUS_PTL_WRITE_WORD	0x06 /* Write word. */
#define BUS_PTL_READ_WORD	0x07 /* Read word. */
#define BUS_PTL_RCV_WORD	0x0F /* Receive word. */

		/* I2C_STATUS_DATA bits */
typedef struct _reg_i2c_status_data
{
	MV_U32	data_lo:8;
	MV_U32	data_hi:8;
	MV_U32	reserved:8;
	MV_U32	int_st:8; /* for INT_ST value, see MI2C_STATUS */
} reg_i2c_status_data;

/* 150MHz/10 */
#define I2C_SYS_CLOCK           150000000/10

/* I2C link rate definitions */
//#define  I2C_LINK_RATE        100*1000          /* 100 kbps   */
#define  I2C_LINK_RATE        400*1000          /* 400 kbps   */
/*
 Definition for SES commands over I2C (to SEP)

Key components:
[SEP address] - [R/W direction] - [Command Type] - [Checksum] - [SEMB address] - [Seq number] -
[SEP/SES commands] - [Data...
*/

/* [SEP address] */
#define I2C_SEP_ADDR0       0xC0 /* i2c address for SEP 0           */
#define I2C_SEP_ADDR1       0xC2 /* i2c address for SEP 1           */

/* [SEMB address] */
#define I2C_SEMB_ADDR        0xD0 /* i2c address for SEMB           */

/* SEP ATA command code */
#define SATA_SEP_ATTN        0x67 /* for all SEP command             */

/* I2C command type for SEP device */
#define I2C_CT_D2H_SAFTE     0x00 /* SEP-to-Host SAFTE data transfer */
#define I2C_CT_H2D_SAFTE     0x80 /* Host-to-SEP SAFTE data transfer */
#define I2C_CT_D2H_SES       0x02 /* SEP-to-Host SES data transfer   */
#define I2C_CT_H2D_SES       0x82 /* Host-to-SEP SES data transfer   */

/* command type flag */
#define I2C_CT_H2D           0x80

/* macro for judge H2D/D2H command type */
#define mis_i2c_ct_d2h(ct)    (!((ct) & I2C_CT_H2D))


/* SEP command */
#define I2C_CMD_IDENTIFY_SEP 0xEC /* identify sep                    */
/* IDENDIFY SEP data page definitions */
typedef struct _identify_sep_data
{
   MV_U8       data_length;
   MV_U8       subenclosure_id;
   MV_U8       logical_id[8];
   MV_U8       vendor_id[8];
   MV_U8       product_id[16];
   MV_U8       revision_level[4];
   MV_U8       channel_id;
   MV_U8       firmware_revision_level[4];
   MV_U8       interface_id_string[6];
   MV_U8       interface_revision_level[4];
   MV_U8       vendor_specific[11];
} identify_sep_data;

typedef struct _cable_low_page0_data
{
    MV_U8       identifier;          // 0 Read-Only
    MV_U8       status[2];          // 1-2 Read-Only
    MV_U8       int_flag[19];     // 3-21 Interrupt Flags (19 Bytes) Read-Only
    MV_U8       free_side_monitor[12]; // 22-33 Free Side Monitors (12 Bytes) Read-Only
    MV_U8       channel_monitor[48]; // 34-81 Channel Monitors (48 Bytes) Read-Only
    MV_U8       reserved[4];         // 82-85 Reserved (4 Bytes) Read-Only
    MV_U8       control[12];         //  86-97 Control (12 Bytes) R/W
    MV_U8       reserved2[2];    //  98-99 Reserved (2 Bytes) R/W
    MV_U8       module_channel_mask[7];  // 100-106 Module and Channel Masks (7 Bytes) R/W
    MV_U8       reserved3;           //  107 Reserved (1 Byte) R/W
    MV_U8       free_side_device_property[4]; // 108-111 Free Side Device Properties (4 Bytes) R/W
    MV_U8       reserved4[7]; // 112-118 Reserved (7 Bytes) R/W
    MV_U8       passwd_change_entry_area[4]; // 119-122 Password Change Entry Area (optional) (4 Bytes)  R/W
    MV_U8       passwd_entry_area[4]; // 123-126 Password Entry Area (optional) 4 Bytes  R/W
    MV_U8       page_select; // 127 Page Select Byte   R/W
} cable_low_page0_data;
typedef struct _cable_upper_page0_data
{
    MV_U8       identifier;          // 128 1 Identifier
    MV_U8       ext_identifier;          // 129 1 Ext. Identifier
/* byte 129 
Bit Description of Device Type
7-6 00: Power Class 1 Module (1.5W max. Power consumption)
    01: Power Class 2 Module (2.0W max. Power consumption)
    10: Power Class 3 Module (2.5W max. Power consumption)
    11: Power Class 4 Module (3.5W max. Power consumption)
5 Reserved
4 0: No CLEI code present in Page 02h
    1: CLEI code present in Page 02h
3 0: No CDR in TX , 1: CDR present in TX
2 0: No CDR in RX , 1: CDR present in RX
1-0 Reserved
*/
    MV_U8       connect;     // 130 1 Connector
/* byte 130
Value Description of Connector
00h Unknown or unspecified
01h SC
02h FC Style 1 copper connector
03h FC Style 2 copper connector
04h BNC/TNC
05h FC coax headers
06h Fiberjack
07h LC
08h MT-RJ
09h MU
0Ah SG
0Bh Optical Pigtail
0Ch MPO
0D-1Fh Reserved
20h HSSDC II
21h Copper pigtail
22h RJ45
23h No separable connector
24h Shielded Mini Multilane HD
25h-7Fh Reserved
80-FFh Vendor specific
*/
    MV_U8       module[8]; //131-138 8 Module
/* byte 131
Address Bit Description of Module data
10/40G Ethernet Compliance Code
    131 7 Reserved
    131 6 10GBASE-LRM
    131 5 10GBASE-LR
    131 4 10GBASE-SR
    131 3 40GBASE-CR4
    131 2 40GBASE-SR4
    131 1-0 Reserved
*/
/* byte 132
SONET Compliance codes
    132 7-3 Reserved
    132 2 OC 48, long reach
    132 1 OC 48, intermediate reach
    132 0 OC 48 short reach
*/
/* byte 133
SAS/SATA compliance codes
    133 7-6 Reserved SAS
    133 5 SAS 6.0Gbps
    133 4 SAS 3.0Gbps
*/
/* byte 134
Gigabit Ethernet Compliant codes
    134 3 1000BASE-T
    134 2 1000BASE-CX
    134 1 1000BASE-LX
    134 0 1000BASE-SX
*/
/*
Fibre Channel link length
    135 7 Very long distance (V)
    135 6 Short distance (S)
    135 5 Intermediate distance (I)
    135 4 Long distance (L)
    135 3 Medium (M)
Fibre Channel Transmitter
Technology
    135 2 Reserved
    135 1 Longwave laser (LC)
    135 0 Electrical inter-enclosure(EL)
    136 7 Electrical intra-enclosure
    136 6 Shortwave laser w/o OFC (SN)
    136 5 Shortwave laser w OFC (SL)
    136 4 Longwave Laser (LL)
    136 0-3 Reserved
Fibre Channel transmission media
    137 7 Twin Axial Pair (TW)
    137 6 Shielded Twisted Pair (TP)
    137 5 Miniature Coax (MI)
    137 4 Video Coax (TV)
    137 3 Multi-mode 62.5m (M6)
    137 2 Multi-mode 50m (M5)
    137 1 Multi-mode 50um (OM3)
    137 0 Single Mode (SM)
Fibre Channel Speed
    138 7 1200 Mbytes/Sec
    138 6 800 Mbytes/Sec
    138 5 1600 Mbytes/Sec
    138 4 400 Mbytes/Sec
    138 3 Reserved
    138 2 200 Mbytes/Sec
    138 1 Reserved
    138 0 100 Mbytes/Sec 
*/
    MV_U8       encoding; // 139 1 Encoding
    MV_U8       br_nominal;         // 140 1 BR, nominal
/* The nominal bit rate (BR, nominal) is specified in units of 100 Megabits per second */
    MV_U8       ext_rate_comp;         // 141 1 Extended rateselect Compliance
    MV_U8       length_smf;    //  142 1 Length(SMF)
    MV_U8       length_om3;  // 143 1 Length(OM3 50 um)
    MV_U8       length_om2;  // 144 1 Length(OM2 50 um)
    MV_U8       length_om1;           //  145 1 Length(OM1 62.5um)
    MV_U8       length_copper;           //  146 1 Length(Copper)
    MV_U8       dev_tech; // 147 1 Device tech
/* 
The technology used in the device is described in table 25 and table 26. The top 4
bits of the Device Tech byte describe the device technology used. The lower four
bits (bits 7-4) of the Device Tech byte are used to describe the transmitter
technology.
Bits Description of Physical device
    7-4 Transmitter technology
    3 0: No wavelength control
        1: Active wavelength control
    2 0: Uncooled transmitter device
        1: Cooled transmitter
    1 0: Pin detector
        1: APD detector
    0 0: Transmitter not tuneable
        1: Transmitter tuneable
Value Description of physical device
    0000b 850 nmVCSEL
    0001b 1310 nmVCSEL
    0010b 1550 nm VCSEL
    0011b 1310 nm FP
    0100b 1310 nm DFB
    0101b 1550 nm DFB
    0110b 1310 nm EML
    0111b 1550 nm EML
    1000b Copper or others
    1001b 1490 nm DFB
    1010b Copper cable unequalized
    1011b Copper cable passive equalized
    1100b Copper cable, near and far end equalizers
    1101b Copper cable, far end equalizers
    1110b Copper cables, near end equalizer
    1111b Reserved
*/
    MV_U8       vendor_name[16]; // 148-163 16 Vendor name
    MV_U8       ext_module; // 164 1 Extended Module
    MV_U8       vendor_oui[3]; // 165-167 3 Vendor OUI
    MV_U8       vendor_pin[16]; // 168-183 16 Vendor PN
    MV_U8       vendor_rev[2]; // 184-185 2 Vendor rev
    MV_U8       wave_len_cable_att[2]; // 186-187 2 Wave length or Copper cable Attenuation
    MV_U8       wave_tolerance[2]; // 188-189 2 Wavelength tolerance
    MV_U8       max_case_temp; // 190 1 Max case temp
    MV_U8       cc_base; // 191 1 CC_BASE
    MV_U8       options[4]; // 192-195 4 Options
/* 
Addrs Bit Description of option
    192 7-0 Reserved

    193 7-1 Reserved
    193 0 RX output amplitude programming, coded 1 if implemented, else 0.

    194 7-4 Reserved
    194 3 Rx Squelch Disable implemented, coded 1 if implemented, else 0.
    194 2 Rx Output Disable capable: coded 1 if implemented, else 0.
    194 1 Tx Squelch Disable implemented: coded 1 if implemented, else 0.
    194 0 Tx Squelch implemented: coded 1 if implemented, else 0.

    195 7 Memory page 02 provided: coded 1 if implemented, else 0.
    195 6 Memory page 01 provided: coded 1 if implemented, else 0.
    195 5 RATE_SELECT is implemented. If the bit is set to 1 then active
            control of the select bits in the upper memory table is required
            to change rates. If the bit is set to 0, no control of the rate
            select bits in the upper memory table is required. In all cases,
            compliance with multiple rate standards should be determined by
            Module Codes in Bytes 132, 133, 134 and 135 of Page 00h.
    195 4 Tx_DISABLE is implemented and disables the serial output.
    195 3 Tx_FAULT signal implemented, coded 1 if implemented, else 0
    195 2 Tx Squelch implemented to reduce OMA coded 0, implemented to
            reduce Pave coded 1.
    195 1 Loss of Signal implemented, coded 1 if implemented, else 0
    195 0 Reserved
*/
    MV_U8       vendor_sn[16]; // 196-211 16 Vendor SN
    MV_U8       date_code[8]; // 212-219 8 Date Code
/*
Address Description of field
    212-213 ASCII code, two low order digits of year. (00=2000)
    214-215 ASCII code digits of month(01=Jan through 12=Dec
    216-217 ASCII code day of month (01-31)
    218-219 ASCII code, vendor specific lot code, may be blank
*/
    MV_U8       diag_monitor_type; // 220 1 Diagnostic Monitoring Type
    MV_U8       enhance_options; // 221 1 Enhanced Options
    MV_U8       reserved01; // 222 1 Reserved
    MV_U8       cc_ext; // 223 1 CC_EXT
/*
    The check code is a one-byte code that can be used to verify that the first 32 bytes
    of extended serial information in the free side device is valid. The check code
    shall be the low order 8 bits of the sum of the contents of all the bytes from byte
    192 to byte 222, inclusive.
*/
    MV_U8       vendor_specific[32]; // 224-255 32 Vendor Specific
} cable_upper_page0_data;

void i2c_init(MV_PVOID core_p);
MV_BOOLEAN i2c_device_state_machine(MV_PVOID i2c_device_p);
MV_QUEUE_COMMAND_RESULT i2c_send_command(MV_PVOID root, MV_PVOID dev_p, 
	MV_Request *req);
MV_VOID i2c_interrupt_service_routine(MV_PVOID core_p);

#define GPIO_DATA_OUT 0x10094
#define GPIO_DATA_OUT_EN_CTRL 0x10098
#define TWSI_BBU_PORT 1

#ifdef SUPPORT_ACTIVE_CABLE
#define GPIO_INT_CAUSE_REG		0x2C04C
#define GPIO_INT_EN_REG		0x2C050
#define GPIO_PIN_OUTPUT_VAL	0x2C090
#define GPIO_PIN_OUT_EN		0x2C094
#define GPIO_PIN_IN_EN_REG		0x2C098

#define I2C_SGPIO_SLCT			0x2C104

#define CABLE_GPIO_BITS		(MV_BIT(1) | MV_BIT(3) | MV_BIT(5) | MV_BIT(7))

#define PHY_MODE_REG1			0x64

#define SLAVE_ADDR_WRITE		0xA0
#define SLAVE_ADDR_READ		0xA1

#define CABLE_TYPE_ADDR1		130
#define CABLE_TYPE_ADDR2		147
#define CABLE_LENGTH_ADDR		146

#define MAX_CABLE_NUM		5

/* CABLE_State in _domain_i2c_link*/
enum {
	CABLE_STATE_I2C_MODE = 0,
	CABLE_STATE_INIT,
	CABLE_STATE_FINISH,
	CABLE_STATE_SUCCESS,
	CABLE_STATE_ERROR,
};

MV_BOOLEAN i2c_cable_isr(MV_PVOID core_p);
void i2c_check_cable_callback(MV_PVOID core_p, MV_Request *req);
void cable_init(MV_PVOID core_p, MV_U8 cable_id);
void i2c_handle_hotplug_cable(MV_PVOID core_p, MV_U8 cable_id);
void core_handle_cable_gpio_int(MV_PVOID core_p, MV_U32 reg_val);
#endif

#endif
