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

#ifndef _MODULE_CC_H
#define _MODULE_CC_H

/*
 * Here is the definition for the command consolidate sub module
 * This is only changed when we modify the consolidate algorithm.
 */
typedef struct _Consolidate_Object
{
	MV_LBA Last_LBA;                                    /* last LBA*/
	PMV_Request Holding_Request;                        /* Internal request which already consolidate some external requests. */
	MV_U8 Sequential;                                   /* sequential counter */
	MV_BOOLEAN Is_Read;                                 /* The last request is read or write. */
	MV_U16 io_count;
	MV_BOOLEAN	Enable_Consolidate;	/* If consolidation is enalbed on this device */
	MV_U8	Cons_SG_Count;		/* Consolidation SG Count */										/* (SG Count of cache request is different from core request) */
	MV_U8	Cons_Req_Count;
	MV_U8	Reserved;
} Consolidate_Object, *PConsolidate_Object;

typedef struct _ModConsolidate_Extension
{
	List_Head Free_Queue;
	PConsolidate_Object pConsolid_Obj;
	MV_PVOID pUpperExtension;
	MV_PVOID pNextExtension;
	MV_VOID (*pNextFunction) (MV_PVOID, PMV_Request);
	MV_U16 dev_count;
	MV_BOOLEAN ModCons_Enabled;		/* 1 - module consolidation is enabled on the adapter */
	MV_U8 Reserved0;
} ModConsolidate_Extension, *PModConsolidate_Extension;

/*============= Consolidate usage ==========================
step:
	1. call "Consolid_GetResourceQuota" to get resource 	requirement of Consolidate.
	2. call "Consolid_InitializeExtension" to initial resource of Consolidate.
	3. call "Consolid_SetSendFunction" to set executing function of Consolidate.
	4. call "Consolid_ModuleSendRequest" to process your request.
	5. call "Consolid_PushFireRequest" to chech if holding request need to push.

========================================================*/
MV_U32 ModConsolid_GetResourceQuota(
		MV_U16 reqCount,
		MV_U16 DevCount,
		MV_U16 sgCount);

void ModConsolid_InitializeExtension(
		MV_PVOID This,
		MV_U16 reqCount,
		MV_U16 devCount,
		MV_U16 sgCount);

void ModConsolid_SetSendFunction(
		MV_PVOID This,
		MV_PVOID current_ext,
		MV_PVOID next_ext,
		MV_VOID (*next_function) (MV_PVOID, PMV_Request));

void ModConsolid_ModuleSendRequest(MV_PVOID This, PMV_Request pReq);
void ModConsolid_PushFireRequest(MV_PVOID This, PMV_Request pReq);
void ModConsolid_SetDevice(MV_PVOID This, struct mod_notif_param *p_notify);
void ModConsolid_InitConsDev(MV_PVOID p_module);
#endif
