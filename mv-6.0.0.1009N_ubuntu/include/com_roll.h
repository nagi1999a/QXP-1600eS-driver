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

#ifndef COM_ROLL_H
#define COM_ROLL_H

#ifdef RAID_ERROR_HANDLING

#define ROLL_RESOURCE_TYPE_CONTEXT		MV_BIT(0)
#define ROLL_RESOURCE_TYPE_RANGE		MV_BIT(1)
#define ROLL_RESOURCE_TYPE_SGTABLE		MV_BIT(2)

typedef struct _EH_RANGE
{
	struct _EH_RANGE	*next;
	MV_LBA				Bad_Lba;
	MV_U32				Bad_Sectors;
}EH_RANGE, *PEH_RANGE;

/* The original data saved during the rollover. */
typedef struct _ROLL_ORG
{
	struct _ROLL_ORG	*next;
	MV_SG_Table			SG_Table;			/* Original SG Table */
	MV_LBA				Org_LBA;
	MV_U32				Org_Sector;
	MV_U8				Org_Cdb;			/* Original Cdb [0] */
	MV_U8				reserved[3];
}ROLL_ORG, *PROLL_ORG;

/* Resource management function:
 * context is the module extension.
 * pResource is valid if allocate = MV_FALSE.
 * resourceType is defined as ROLL_RESOURCE_TYPE_XXX
 * allocate is a MV_BOOLEAN variable. 
 * When it is MV_TRUE, the function is called to allocate resource. 
 * When it is MV_FALSE, the function is called to release the resource specified by pResource. */
typedef MV_PVOID (*Roll_ResourceManager) (MV_PVOID context, MV_PVOID pResource, MV_U8 resourceType, MV_BOOLEAN allocate);

#define ROLL_FLAG_NONE					0
#define ROLL_FLAG_VALID_ORG_DATA		MV_BIT(0)
#define ROLL_FLAG_SHRINK_SIZE			MV_BIT(1)

typedef struct _ROLL_CTX
{
	List_Head			Queue_Pointer;

	/* Fixed variables */	
	MV_PVOID			Module_Ext;			/* Module Extension */
	MV_U32				Completed_Sector;	/* Completed Sector Count */
	MV_U8				Flag;				/* ROLL_FLAG_XXX */
	MV_U8				Reserved0[3];

	/* Resource will be allocated on demand */
	PROLL_ORG			pRoll_Org;			/* MV_Request Original Variables */
	PEH_RANGE			pEH_Range;			/* Error Ranges have been hit */

	/* Function Tables */
	Roll_ResourceManager	pFResourceManager;	/* Function used to allocate and release resource. */
}ROLL_CTX, * PROLL_CTX;

typedef enum _ROLL_ACTION{
	ROLL_ACTION_ADJUST_SIZE	= 0,	/* Won't roll the request. Just adjust request size. 
									 * After that, the following rollover will try the whole left size. */
	ROLL_ACTION_SKIP		= 1,	/* Skip the current range and record error. Then continue the left range. */
	ROLL_ACTION_CONTINUE	= 2,	/* Continue the left range */
	ROLL_ACTION_CHANGE_CDB	= 3,	/* Change CDB code. So far just supports from R/W to Verify. */
	ROLL_ACTION_SHRINK_SIZE = 4		/* Won't roll the request. Just adjust request size. 
									 * After that, the following rollover will try the size less/equal to this size. */

} ROLL_ACTION;


/* Initialize the rollover request. */
MV_VOID Rollover_Init(
	PMV_Request pMvReq,
	MV_PVOID pModuleExt,
	Roll_ResourceManager pFResource
	);

/* Cleanup the rollover request. */
MV_VOID Rollover_Cleanup(
	PMV_Request pMvReq
	);

/* Four actions.	
 * ROLL_ACTION_ADJUST_SIZE and ROLL_ACTION_SHRINK_SIZE: Won't roll the request. Just adjust request size.
 * The difference between them is the following ROLL_ACTION_CONTINUE.
 * For ROLL_ACTION_ADJUST_SIZE, continue will try the whole left size.
 * For ROLL_SHRINK_SIZE, continue will try size less or equal to the adjusted size.
 * ROLL_ACTION_SKIP:Skip the current range and record error. Then continue the left range.
 * ROLL_ACTION_CONTINUE: Continue the left range
 * pMvReq and action must be specified for all actions.
 * size is only used for ROLL_ACTION_ADJUST_SIZE and ROLL_ACTION_SHRINK_SIZE.
 * isFinished indicated whether the request is already finished.
 * MV_TRUE means this time, no access range is left to handle.
 */
MV_VOID Rollover_Roll(
	PMV_Request pMvReq,
	ROLL_ACTION action,
	MV_U32		size,
	MV_BOOLEAN  *isFinished
	);

/* Whether ever did Rollover_Roll three actions before. */
MV_BOOLEAN Rollover_HasRolled(
	PMV_Request pMvReq
	);

/* Whether has skipped ranges. */
#define Rollover_HasEHRange(pMvReq)	\
((pMvReq)->Roll_CTX!=NULL) ? ((PROLL_CTX)((pMvReq)->Roll_CTX))->pEH_Range!=NULL?MV_TRUE:MV_FALSE : MV_FALSE

/* Copy the EH Range. */
MV_VOID Rollover_CopyEHRange(
	PMV_Request pDestReq,
	PMV_Request pSourceReq
	);

MV_VOID Rollover_CleanEHRange(
	PMV_Request pMvReq
	);

MV_VOID Rollover_AddEHRange(
	PMV_Request	pMvReq,
	MV_LBA		lba,
	MV_U32		sector
	);

/* Returned the skipped ranges.*/
#define Rollover_GetEHRange(pMvReq) ((pMvReq)->Roll_CTX!=NULL)?((PROLL_CTX)((pMvReq)->Roll_CTX))->pEH_Range:NULL

MV_U8 Rollover_GetOriginalCDB(
	PMV_Request pMvReq
	);

#endif /* RAID_ERROR_HANDLING */

#endif /* COM_ROLL_H */
