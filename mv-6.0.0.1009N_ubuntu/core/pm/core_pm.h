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

#ifndef __CORE_PM_H
#define __CORE_PM_H

#include "mv_config.h"
#include "core_type.h"

MV_BOOLEAN pm_state_machine(MV_PVOID dev);
MV_U8 pm_verify_command(MV_PVOID root_p, MV_PVOID dev,
       MV_Request *req);
MV_VOID pm_prepare_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmd_header, MV_PVOID cmd_table, 
	MV_Request *req);
MV_VOID pm_send_command(
            MV_PVOID root_p,
            MV_PVOID dev,
            MV_PVOID struct_wrapper,
            MV_Request *req);
MV_VOID pm_process_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table, 
	MV_Request *req);

/* PM registers definitions */
#define PM_OP_READ					0
#define PM_OP_WRITE					1

#define PM_GSCR_ID					0
#define PM_GSCR_REVISION			1
#define PM_GSCR_INFO				2
#define PM_GSCR_ERROR				32
#define PM_GSCR_ERROR_ENABLE		33
#define PM_GSCR_FEATURES			64
#define PM_GSCR_FEATURES_ENABLE		96

typedef struct _SPI_PM_Autoload_Setting
{
    MV_U16 vendor_id;
	MV_U16 device_id[2];
	MV_U32 autoload[31];
} SPI_PM_Autoload_Setting;

/*below registers are mapped for accessing from host, bit0~11 means offset bit 12~15 means port num.*/
static SPI_PM_Autoload_Setting support_akupa_pm_devs[] = {
{
	0x1B4B, /*Vendor ID*/
	{0x9703, 0x9713}, /*Device ID*/
	{0xA5A5A5A5L, /*Start Sign*/
	0x0000F090L, 0x00000707L, /*Enable the FIFO of 3 ports*/
	0x0000F091L, 0xFFF0003AL, /*FIFO threshold control*/
	0x0000F248L, 0x000062D8L, /*Host port OOB upper bond*/
	0x00000048L, 0x000062D8L, /*PM device port0 OOB upper bond*/
	0x00001048L, 0x000062D8L, /*PM device port1 OOB upper bond*/
	0x00002048L, 0x000062D8L, /*PM device port2 OOB upper bond*/
	0xFFFFFFFFL,
	0xFFFFFFFFL,},
},
{
    0x1B4B, /*Vendor ID*/
	{0x9705, 0x9715}, /*Device ID*/
	{0xA5A5A5A5L, /*Start Sign*/
	0x0000F3E8L, 0x00000041L, /*Enable port4 blinking*/
	0x0000F090L, 0x00001F1FL, /*Enable the FIFO of 3 ports*/
	0x0000F091L, 0xFFF0003AL, /*FIFO threshold control*/
	0x0000F248L, 0x000062D8L, /*Host port OOB upper bond*/
	0x00000048L, 0x000062D8L, /*PM device port0 OOB upper bond*/
	0x00001048L, 0x000062D8L, /*PM device port1 OOB upper bond*/
	0x00002048L, 0x000062D8L, /*PM device port2 OOB upper bond*/
    0x00003048L, 0x000062D8L, /*PM device port3 OOB upper bond*/
	0x00004048L, 0x000062D8L, /*PM device port4 OOB upper bond*/
	0xFFFFFFFFL,
	0xFFFFFFFFL,},

},
{
    0x0000,
	{0x0000, 0x0000},
    {0x00000000L,},
},
};

#define PM_PSCR_SSTATUS				0
#define PM_PSCR_SERROR				1
#define PM_PSCR_SCONTROL			2
#define PM_PSCR_SACTIVE				3

#define PM_SERROR_EXCHANGED			MV_BIT(26)
#define PM_SERROR_COMM_WAKE			MV_BIT(18)
#define PM_SERROR_PHYRDY_CHANGE		MV_BIT(16)

#endif /* __CORE_PM_H */
