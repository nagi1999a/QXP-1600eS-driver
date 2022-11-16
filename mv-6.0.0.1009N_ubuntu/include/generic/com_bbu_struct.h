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

#ifndef __MV_COM_BBU_STRUCT_H__
#define __MV_COM_BBU_STRUCT_H__

#include "com_define.h"

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

/* battery status */
#define BBU_STATUS_NOT_PRESENT          0
#define BBU_STATUS_PRESENT              MV_BIT(0)
#define BBU_STATUS_LOW_BATTERY          MV_BIT(1)
#define BBU_STATUS_CHARGING             MV_BIT(2)
#define BBU_STATUS_FULL_CHARGED         MV_BIT(3)
#define BBU_STATUS_DISCHARGE            MV_BIT(4)
#define BBU_STATUS_RELEARN              MV_BIT(5)
#define BBU_STATUS_OVER_TEMP_WARNING    MV_BIT(6)
#define BBU_STATUS_OVER_TEMP_ERROR      MV_BIT(7)
#define BBU_STATUS_UNDER_TEMP_WARNING   MV_BIT(8)
#define BBU_STATUS_UNDER_TEMP_ERROR     MV_BIT(9)
#define BBU_STATUS_OVER_VOLT_WARNING    MV_BIT(10)
#define BBU_STATUS_OVER_VOLT_ERROR      MV_BIT(11)
#define BBU_STATUS_UNDER_VOLT_WARNING   MV_BIT(12)
#define BBU_STATUS_UNDER_VOLT_ERROR     MV_BIT(13)
#define BBU_STATUS_GREATER_LOWERBOUND	MV_BIT(14)
#define BBU_STATUS_POWER_STOP_ALL	MV_BIT(15)

#define BBU_NOT_PRESENT         0
#define BBU_NORMAL                   1
#define BBU_ABNORMAL               2

#define BBU_SUPPORT_NONE                 0
#define BBU_SUPPORT_SENSOR_TEMPERATURE   MV_BIT(0)
#define BBU_SUPPORT_SENSOR_VOLTAGE       MV_BIT(1)
#define BBU_SUPPORT_SENSOR_ECAPACITY     MV_BIT(2)
#define BBU_SUPPORT_CHANGE_MAXEC         MV_BIT(3)
#define BBU_SUPPORT_RELEARN              MV_BIT(4)

#define BBU_SENSOR_NOTSUPPORT    0
#define BBU_SENSOR_TEMPERATURE   MV_BIT(0)
#define BBU_SENSOR_VOLTAGE       MV_BIT(1)
#define BBU_SENSOR_ECAPACITY     MV_BIT(2)
#define BBU_CHANGE_MAXEC         MV_BIT(3)
#define BBU_SUPPORT_RELEARN      MV_BIT(4)

#define BBU_FLAG_EDVF            MV_BIT(0)
#define BBU_FLAG_EDV1            MV_BIT(1)
#define BBU_FLAG_NO_ACT          MV_BIT(6)
#define BBU_FLAG_CHARGING        MV_BIT(7)

#define BBU_MAX_RETRY_CHARGE_COUNT      10

typedef struct _BBU_Info
{
    //Total 64 Bytes
    MV_U32          status;
    MV_U32          prev_status;
    
    MV_U16          voltage; /* unit is 1 mV */
    MV_U16          supportMaxCapacity;
    MV_U16          supportMinCapacity;
    MV_U16          maxCapacity;
    
    MV_U16          curCapacity;
    MV_U16          time_to_empty;
    MV_U16          time_to_full;
    MV_U16          recharge_cycle;

    MV_U16          featureSupport;
    MV_I16          temperature; /*unit is 0.01 Celsius */
    MV_U16           volt_lowerbound;
    MV_U16           volt_upperbound;
    
    MV_U8           temp_lowerbound;
    MV_U8           temp_upperbound;
    MV_U8           percentage;			// current percentage of barrtery capacity.
    MV_U8           percent_to_charge;	// if percentage is lower than this number,it should begin to charge.
    MV_U8           flags;
    MV_U8           stop_charge_count;
    MV_U8           last_notify_state;
	MV_U8           reserved2[25];
} BBU_Info, *PBBU_Info;

#define BBU_ACT_RELEARN          0
#define BBU_ACT_FORCE_CHARGE     1
#define BBU_ACT_FORCE_DISCHARGE  2
#define BBU_ACT_STOP_ALL         3

#define BBU_THRESHOLD_TYPE_CAPACITY        0
#define BBU_THRESHOLD_TYPE_TEMPERATURE     1
#define BBU_THRESHOLD_TYPE_VOLTAGE         2

#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
