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

#ifdef SUPPORT_BOARD_ALARM
#include "core_header.h"
#include "core_internal.h"
#include "core_manager.h"
#include "core_util.h"
#include "core_alarm.h"

MV_VOID
core_alarm_init(MV_PVOID core_p)
{
	core_extension *core = (core_extension *) core_p;

	core->alarm.alarm_state = ALARM_STATE_OFF;
	core->alarm.reg_setting = 0;
	core->alarm.timer_id = 0;

	core_alarm_enable_register(core);
}

MV_VOID
core_alarm_timer_callback(MV_PVOID core_p, MV_PVOID tmp)
{
	core_extension *core = (core_extension *)core_p;
	MV_U8 time;

	core->alarm.timer_id = 0;

	switch (core->alarm.alarm_state) {
	case ALARM_STATE_1S_1S:
		/* 1 second on , 1 second off */
		if (core->alarm.reg_setting == ALARM_REG_ON) {
			time = 1;
			core->alarm.reg_setting = ALARM_REG_OFF;
			core_alarm_set_register(core, MV_FALSE);
		} else {
			time = 1;
			core->alarm.reg_setting = ALARM_REG_ON;
			core_alarm_set_register(core, MV_TRUE);
		}
		core->alarm.timer_id = core_add_timer(core, time,
			core_alarm_timer_callback, core, NULL);
		break;
	case ALARM_STATE_OFF:	
		core->alarm.reg_setting = ALARM_REG_OFF;
		core_alarm_set_register(core, MV_FALSE);
		break;
	case ALARM_STATE_MUTE:	
		core->alarm.reg_setting = ALARM_REG_OFF;
		core_alarm_set_register(core, MV_FALSE);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
}

MV_VOID
core_alarm_change_state(MV_PVOID core_p, MV_U8 state)
{
	core_extension *core = (core_extension *)core_p;

	switch (state) {
	case ALARM_STATE_OFF:
		core->alarm.alarm_state = state;
		if (core->alarm.timer_id == 0) 
			MV_DASSERT(core->alarm.reg_setting == ALARM_REG_OFF);
		break;
	case ALARM_STATE_1S_1S:
		/* 1 second on , 1 second off */
		core->alarm.alarm_state = state;
		if (core->alarm.timer_id == 0) {
			/* lets start in on state first */
			core->alarm.timer_id = core_add_timer(core, 1, 
				core_alarm_timer_callback, core, NULL);
			core->alarm.reg_setting = ALARM_REG_ON;
			core_alarm_set_register(core, MV_TRUE);
		}
		break;
	case ALARM_STATE_MUTE:
		core->alarm.alarm_state = ALARM_STATE_MUTE;
		break;
	default:
		CORE_DPRINT(("Invalid alarm state: %d.\n", state));
		break;
	}
}

#endif /* SUPPORT_BOARD_ALARM */
