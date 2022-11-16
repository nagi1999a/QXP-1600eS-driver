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

#include "com_type.h"
#include "com_define.h"
#include "com_dbg.h"
#include "com_util.h"
#include "com_u64.h"
#include "com_struct.h"
#include "com_roll.h"
#include "com_scsi.h"
#ifdef RAID_DRIVER
#include "raid_structure.h"
#endif
/* 
 * The Rollover scheme is used to roll the request and complete the request.
 * The reason for that can be error is hit or request size is too big for handling.
 * In three cases in RAID module, we'll use this sub module.
 * 1. Error Handling with CONTINUE_ON_ERROR flag.
 *  If we want to complete the request even part of the data cannot be read or written.
 * 2. RAID module internal redundant retry.
 *  Using RAID internal redundant disk to retry the request by marking down some HDD.
 * 3. Request is too big for handling when resource is limited.
 *  Split the request and finish it gradually.
 */
#ifdef RAID_ERROR_HANDLING
void RollSetupRequest(
	PMV_Request pMvReq,
	MV_LBA startLBA,
	MV_U32 sector,
	PMV_SG_Table pSourceSGTable,
	MV_LBA orgStartLBA
	);

MV_VOID Rollover_Init(
	PMV_Request pMvReq,
	MV_PVOID pModuleExt,
	Roll_ResourceManager pFResource
	)
{
	PROLL_CTX pRollCtx;

	MV_DASSERT(pMvReq->Roll_CTX==NULL);
	pRollCtx = (PROLL_CTX)pFResource( pModuleExt, NULL, ROLL_RESOURCE_TYPE_CONTEXT, MV_TRUE );

    MV_DASSERT(pRollCtx);

	pRollCtx->Module_Ext = pModuleExt;
	pRollCtx->pEH_Range = NULL;
	pRollCtx->Completed_Sector = 0;
	pRollCtx->Flag = ROLL_FLAG_NONE;
	pRollCtx->pRoll_Org = NULL;

	pRollCtx->pFResourceManager = pFResource;

	pMvReq->Roll_CTX = pRollCtx;
}

MV_VOID Rollover_Cleanup(
	 PMV_Request pMvReq
	 )
{
	PROLL_CTX pRollCtx = (PROLL_CTX)pMvReq->Roll_CTX;
	PEH_RANGE pRange;
	PROLL_ORG pRollOrg;
#ifdef HAVE_RAID_NATIVE_4K
    MV_U32 sectorSize = pMvReq->Data_Transfer_Length / pMvReq->Sector_Count;
#endif

	/* Restore the request and release resource */
	if ( pRollCtx!=NULL ) {
		while ( pRollCtx->pEH_Range ) {
			pRange = pRollCtx->pEH_Range;
			pRollCtx->pEH_Range = pRange->next;
			(*pRollCtx->pFResourceManager)(
				pRollCtx->Module_Ext, pRange, ROLL_RESOURCE_TYPE_RANGE, MV_FALSE);
		}

		pRollOrg = pRollCtx->pRoll_Org;
		if ( pRollOrg ) {
			/* Restore the request. */
			#if	0
			MV_CopySGTable(&pMvReq->SG_Table, &pRollOrg->SG_Table);
			#else
			{
				MV_SG_Table backup;
				backup = pMvReq->SG_Table;
				pMvReq->SG_Table = pRollOrg->SG_Table;
				pRollOrg->SG_Table = backup;
			}
			#endif

			pMvReq->LBA = pRollOrg->Org_LBA;
			pMvReq->Sector_Count = pRollOrg->Org_Sector;
			pMvReq->Cdb[0] = pRollOrg->Org_Cdb;
			MV_CodeReadWriteCDB(pMvReq->Cdb, pMvReq->LBA, pMvReq->Sector_Count, pMvReq->Cdb[0]);
			pMvReq->Req_Flag |= REQ_FLAG_LBA_VALID;
			if ( SCSI_IS_VERIFY(pMvReq->Cdb[0]) ) {
				pMvReq->Data_Transfer_Length = 0;
			} else {
#ifdef HAVE_RAID_NATIVE_4K
				pMvReq->Data_Transfer_Length = pRollOrg->Org_Sector * sectorSize;
#else
				pMvReq->Data_Transfer_Length = pRollOrg->Org_Sector*SECTOR_LENGTH;
#endif
			}

			(*pRollCtx->pFResourceManager)(
				pRollCtx->Module_Ext, pRollOrg, ROLL_RESOURCE_TYPE_SGTABLE, MV_FALSE);
		}

		(*pRollCtx->pFResourceManager)(
			pRollCtx->Module_Ext, pRollCtx, ROLL_RESOURCE_TYPE_CONTEXT, MV_FALSE);
	}

	pMvReq->Roll_CTX = NULL;
}

/*
 * Caller found one of the rollover condition is hit. So ask Rollover sub module to do its job.
 * ROLL_ACTION_ADJUST_SIZE/ROLL_ACTION_SHRINK_SIZE: Adjust the MV_Request size using the input parameter size.
 * ROLL_ACTION_SKIP: Skip the current range and record error. Then continue the left range.
 * ROLL_ACTION_CONTINUE: Continue the left range.
 */
MV_VOID Rollover_Roll( PMV_Request pMvReq, ROLL_ACTION action, MV_U32 size, MV_BOOLEAN *isFinished )
{
	PROLL_CTX	pRollCtx = (PROLL_CTX)pMvReq->Roll_CTX;
	PROLL_ORG	pRollOrg = NULL;
	PEH_RANGE	pEHRange = NULL;
	MV_U32		newSize = 0;
	MV_LBA		newStartLBA;
    
	MV_DASSERT( pRollCtx!=NULL );
	MV_DASSERT( pMvReq->Req_Flag&REQ_FLAG_LBA_VALID );

	/* Request hasn't been split yet. For this request, it's the first time come here. */
	if ( pRollCtx->Flag==ROLL_FLAG_NONE ) {
		MV_DASSERT( pRollCtx->pRoll_Org==NULL );
		pRollOrg = (PROLL_ORG)(*pRollCtx->pFResourceManager)
			(pRollCtx->Module_Ext, NULL, ROLL_RESOURCE_TYPE_SGTABLE, MV_TRUE);
		pRollCtx->pRoll_Org = pRollOrg;

		/* Save the original data. */
		#if 0 
		MV_CopySGTable(&pRollOrg->SG_Table, &pMvReq->SG_Table);
		#else
		/* Save the original SG Table.
		 * Don't use MV_CopySGTable. Different module has different SG Items available. */
		{
			MV_SG_Table backup;

			backup = pRollOrg->SG_Table;
			pRollOrg->SG_Table = pMvReq->SG_Table;

			// initialize the new sg table.
			sgd_table_init( &pMvReq->SG_Table, backup.Max_Entry_Count, backup.Entry_Ptr );
			sgdt_append_reftbl( &pMvReq->SG_Table, &pRollOrg->SG_Table, 0, pRollOrg->SG_Table.Byte_Count );
		}
		#endif
		pRollOrg->Org_LBA = pMvReq->LBA;
		pRollOrg->Org_Sector = pMvReq->Sector_Count;
		pRollOrg->Org_Cdb = pMvReq->Cdb[0];

		MV_DASSERT( pRollCtx->Completed_Sector==0 );
		MV_DASSERT( pRollCtx->pEH_Range==NULL );
		pRollCtx->Flag |= ROLL_FLAG_VALID_ORG_DATA;
	} else {
		MV_DASSERT( pRollCtx->pRoll_Org!=NULL );
		pRollOrg = pRollCtx->pRoll_Org;
	}
	
	if ( action==ROLL_ACTION_ADJUST_SIZE ) {
		newSize = size;
		newStartLBA.value = pMvReq->LBA.value;
		MV_DASSERT( (newSize<=pMvReq->Sector_Count) && (newSize>0) );
		pRollCtx->Flag &= ~ROLL_FLAG_SHRINK_SIZE;
	} else if ( action==ROLL_ACTION_SHRINK_SIZE ) {
		newSize = size;
		newStartLBA.value = pMvReq->LBA.value;
		MV_DASSERT( (newSize<=pMvReq->Sector_Count) && (newSize>0) );
		pRollCtx->Flag |= ROLL_FLAG_SHRINK_SIZE;
	} else if ( action==ROLL_ACTION_SKIP ) {
		pEHRange = (PEH_RANGE)(*pRollCtx->pFResourceManager)
			(pRollCtx->Module_Ext, NULL, ROLL_RESOURCE_TYPE_RANGE, MV_TRUE);
		pEHRange->Bad_Lba = pMvReq->LBA;
		pEHRange->Bad_Sectors = pMvReq->Sector_Count;
		pEHRange->next = pRollCtx->pEH_Range;
		pRollCtx->pEH_Range = pEHRange;

		/* Continue the next LBA now. */
		pRollCtx->Completed_Sector += pMvReq->Sector_Count;
		newStartLBA = U64_ADD_U32(pMvReq->LBA, pMvReq->Sector_Count);
		MV_DASSERT( pRollOrg->Org_Sector>=pRollCtx->Completed_Sector );
		newSize = pRollOrg->Org_Sector-pRollCtx->Completed_Sector;
	} else if ( action==ROLL_ACTION_CONTINUE ) {

		/* Continue the next LBA now. */
		pRollCtx->Completed_Sector += pMvReq->Sector_Count;
		newStartLBA = U64_ADD_U32(pMvReq->LBA, pMvReq->Sector_Count);
		MV_DASSERT( pRollOrg->Org_Sector>=pRollCtx->Completed_Sector );
		if ( pRollCtx->Flag&ROLL_FLAG_SHRINK_SIZE ) {
			/* To speed up error handling, if ever shrink to sector, will do sector by sector. */
			newSize = MV_MIN(
				pRollOrg->Org_Sector-pRollCtx->Completed_Sector,
				pMvReq->Sector_Count
				);
		} else {
			/* For normal adjust size request like RAID during hibernation, will try the whole left size. */
			newSize = pRollOrg->Org_Sector-pRollCtx->Completed_Sector;
		}
	} else if ( action==ROLL_ACTION_CHANGE_CDB ) {
		/* So far only supports change CDB from read/write to read verfiy. */
		newSize = pMvReq->Sector_Count;
		newStartLBA.value = pMvReq->LBA.value;
		pMvReq->Cdb[0]=(MV_U8)size;
		MV_ASSERT( SCSI_IS_VERIFY(pMvReq->Cdb[0]) );
	}
	
    /* Check whether the command is finished. */
	if ( newSize>0 ) {
		/* Set up this MV_Request based on new startLBA and sector. */
		RollSetupRequest(pMvReq, newStartLBA, newSize, &pRollOrg->SG_Table, pRollOrg->Org_LBA);
		*isFinished = MV_FALSE;
	} else {
		/* Command is finished. */
		*isFinished = MV_TRUE;
	}
}

MV_BOOLEAN Rollover_HasRolled( PMV_Request pMvReq )
{
	PROLL_CTX	pRollCtx = (PROLL_CTX)pMvReq->Roll_CTX;

	if ( pRollCtx 
		&& (pRollCtx->Flag!=ROLL_FLAG_NONE)		/* Resource is allocated but didn't do any rollover. */
	) {
		return MV_TRUE;
	} else {
		return MV_FALSE;
	}
}

MV_VOID Rollover_CopyEHRange(
	PMV_Request pDestReq,
	PMV_Request pSourceReq
	)
{
	PROLL_CTX pSourceCtx = (PROLL_CTX)pSourceReq->Roll_CTX;
	PROLL_CTX pDestCtx = (PROLL_CTX)pDestReq->Roll_CTX;

	PEH_RANGE pSourceRange, pDestRange;

	MV_DASSERT( pSourceCtx!=NULL && pDestCtx!=NULL );
	MV_DASSERT( pDestCtx->pEH_Range==NULL );

	pSourceRange = pSourceCtx->pEH_Range;
	while ( pSourceRange ) {
		pDestRange = (PEH_RANGE)(*pDestCtx->pFResourceManager)(
			pDestCtx->Module_Ext, NULL, ROLL_RESOURCE_TYPE_RANGE, MV_TRUE);

		pDestRange->Bad_Lba = pSourceRange->Bad_Lba;
		pDestRange->Bad_Sectors = pSourceRange->Bad_Sectors;
		pDestRange->next = pDestCtx->pEH_Range;
		pDestCtx->pEH_Range = pDestRange;

		pSourceRange = pSourceRange->next;
	}
}

MV_VOID Rollover_AddEHRange(
	PMV_Request pMvReq,
	MV_LBA		lba,
	MV_U32		sector
	)
{
	PROLL_CTX pCtx = (PROLL_CTX)pMvReq->Roll_CTX;
	PEH_RANGE pRange;
    
	MV_DASSERT( pCtx!=NULL );
	pRange = (PEH_RANGE)(*pCtx->pFResourceManager)(
		pCtx->Module_Ext, NULL, ROLL_RESOURCE_TYPE_RANGE, MV_TRUE);
	MV_DASSERT( pRange );

	if ( pRange ) {
		pRange->Bad_Lba = lba;
		pRange->Bad_Sectors = sector;
		pRange->next = pCtx->pEH_Range;

		pCtx->pEH_Range = pRange;
	}
}

MV_VOID Rollover_CleanEHRange(
	PMV_Request pMvReq
	)
{
	PROLL_CTX pRollCtx = (PROLL_CTX)pMvReq->Roll_CTX;
	PEH_RANGE pRange;

	if ( pRollCtx!=NULL ) {
		while ( pRollCtx->pEH_Range ) {
			pRange = pRollCtx->pEH_Range;
			pRollCtx->pEH_Range = pRange->next;
			(*pRollCtx->pFResourceManager)(
				pRollCtx->Module_Ext, pRange, ROLL_RESOURCE_TYPE_RANGE, MV_FALSE);
		}
	}
}

MV_U8 Rollover_GetOriginalCDB(
	PMV_Request pMvReq
	)
{
	PROLL_CTX	pRollCtx = (PROLL_CTX)pMvReq->Roll_CTX;
	PROLL_ORG	pRollOrg = NULL;

	if ( pRollCtx==NULL ) return pMvReq->Cdb[0];
	if ( pRollCtx->Flag==ROLL_FLAG_NONE ) return pMvReq->Cdb[0];

	pRollOrg = pRollCtx->pRoll_Org;
	if ( pRollOrg==NULL ) return pMvReq->Cdb[0];

	return pRollOrg->Org_Cdb;
}

void RollSetupRequest(
	PMV_Request pMvReq,
	MV_LBA startLBA,
	MV_U32 sector,
	PMV_SG_Table pSourceSGTable,
	MV_LBA orgStartLBA
	)
{
	MV_U32 offset;
#ifdef HAVE_RAID_NATIVE_4K
    PRAID_Request pRaidReq = pMvReq->pRaid_Request;
    MV_U32 sectorSize = pRaidReq->pRC_Sub->Original_Layout.LD_Sector_Size;
#endif


	pMvReq->Cmd_Flag = 0;

	if ( SCSI_IS_READ(pMvReq->Cdb[0]) || SCSI_IS_WRITE(pMvReq->Cdb[0]) ) {

		offset = U64_SUBTRACT_U64(startLBA,orgStartLBA).parts.low;

#ifdef HAVE_RAID_NATIVE_4K
		MV_CopyPartialSGTable(
			&pMvReq->SG_Table,
			pSourceSGTable,
			offset * sectorSize,
			sector * sectorSize
			);
		pMvReq->Data_Transfer_Length = sector * sectorSize;
#else
		MV_CopyPartialSGTable(
			&pMvReq->SG_Table,
			pSourceSGTable,
			offset*SECTOR_LENGTH,
			sector*SECTOR_LENGTH
			);
		pMvReq->Data_Transfer_Length = sector * SECTOR_LENGTH;
#endif


		pMvReq->Cmd_Flag |= CMD_FLAG_DMA;
                if ( SCSI_IS_READ(pMvReq->Cdb[0]) ) {
			pMvReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
                }
	} else {
		MV_DASSERT( SCSI_IS_VERIFY(pMvReq->Cdb[0]) );
		/* Verify command has no data transfer, clear data transfer length and SG table */
		SGTable_Init(&pMvReq->SG_Table, 0);
		pMvReq->Data_Transfer_Length = 0;
	}

	MV_CodeReadWriteCDB(pMvReq->Cdb, startLBA, sector, pMvReq->Cdb[0]);

	pMvReq->Req_Flag |= REQ_FLAG_LBA_VALID;
	pMvReq->LBA = startLBA;
	pMvReq->Sector_Count = sector;
}

#endif
