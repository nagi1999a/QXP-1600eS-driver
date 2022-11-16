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

#ifndef __HBA_TIMER_H__
#define __HBA_TIMER_H__

#include "com_tag.h"
#include "hba_exp.h"

enum _tag_hba_msg_state{
	MSG_QUEUE_IDLE=0,
	MSG_QUEUE_PROC,
	MSG_QUEUE_NO_START
};

struct mv_hba_msg {
	MV_PVOID data;
	MV_U32   msg;
	MV_U32   param;
	List_Head msg_list;
};

struct mv_hba_msg_queue {
	spinlock_t lock;
	List_Head free;
	List_Head tasks;
	struct mv_hba_msg msgs[MSG_QUEUE_DEPTH];
};

enum {
	HBA_TIMER_IDLE = 0,
	HBA_TIMER_RUNNING,
	HBA_TIMER_LEAVING,
};
void hba_house_keeper_init(void);
void hba_house_keeper_run(void);
void hba_house_keeper_exit(void);
void hba_msg_insert(void *data, unsigned int msg, unsigned int param);
void hba_init_timer(PMV_Request req);
void hba_remove_timer(PMV_Request req);
void hba_remove_timer_sync(PMV_Request req);
void hba_add_timer(PMV_Request req, int timeout,
		   MV_VOID (*function)(MV_PVOID data));
extern struct mv_lu *mv_get_device_by_target_lun(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun);
extern struct mv_lu *mv_get_avaiable_device(struct hba_extension * hba, MV_U16 target_id, MV_U16 lun);
extern MV_U16 get_id_by_targetid_lun(MV_PVOID ext, MV_U16 id,MV_U16 lun);

#if  defined(__VMKLNX__) || defined (MV_VMK_ESX35)
/*vmware use 100 for HZ*/
#define TIMER_INTERVAL_OS		100		/* 10 millisecond */
#define TIMER_INTERVAL_LARGE_UNIT	50		/* 10 millisecond */
#define TIMER_INTERVAL_SMALL_UNIT	10		/* 10 millisecond */
#else
#define TIMER_INTERVAL_OS		1000		/* millisecond */
#define TIMER_INTERVAL_LARGE_UNIT	500		/* millisecond */
#define TIMER_INTERVAL_SMALL_UNIT	100		/* millisecond */
#endif

#endif /* __HBA_TIMER_H__ */


