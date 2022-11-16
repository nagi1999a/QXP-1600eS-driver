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

#include "com_define.h"
#include "com_dbg.h"
#include "com_scsi.h"
#include "com_util.h"
#include "com_u64.h"
#include "hba_exp.h"
#include "com_extern.h"

#ifdef SUPPORT_ROC
	#include "mv_roc.h"
#endif

void MV_ZeroMvRequest(PMV_Request pReq)
{
	PMV_SG_Entry pSGEntry;
	MV_U16 maxEntryCount;
	MV_PVOID pSenseBuffer;
	List_Head list;
#if defined(HAVE_HW_COMPLIANT_SG)
		MV_PHYSICAL_ADDR bus_addr;
		MV_DASSERT(pReq);
		bus_addr.parts.low = pReq->bus_addr.parts.low;
		bus_addr.parts.high = pReq->bus_addr.parts.high;
#else
		MV_DASSERT(pReq);
#endif

	pSGEntry = pReq->SG_Table.Entry_Ptr;
	maxEntryCount = pReq->SG_Table.Max_Entry_Count;
	pSenseBuffer = pReq->Sense_Info_Buffer;
	list = pReq->pool_entry;
#ifdef  RAID_ERROR_HANDLING
	MV_DASSERT( pReq->Roll_CTX==NULL );
#endif

	MV_ZeroMemory(pReq, MV_REQUEST_SIZE);
 	pReq->pool_entry = list;
	pReq->SG_Table.Entry_Ptr = pSGEntry;
	pReq->SG_Table.Max_Entry_Count = maxEntryCount;
	pReq->Sense_Info_Buffer = pSenseBuffer;
#if defined(HAVE_HW_COMPLIANT_SG)
	pReq->bus_addr.parts.low = bus_addr.parts.low;
	pReq->bus_addr.parts.high = bus_addr.parts.high;
#endif
}

void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable)
{
	MV_ASSERT(pTargetSGTable->Max_Entry_Count >= pSourceSGTable->Valid_Entry_Count);
	pTargetSGTable->Valid_Entry_Count = pSourceSGTable->Valid_Entry_Count;
	pTargetSGTable->Occupy_Entry_Count = pSourceSGTable->Occupy_Entry_Count;
	pTargetSGTable->Flag = pSourceSGTable->Flag;
	pTargetSGTable->Byte_Count = pSourceSGTable->Byte_Count;
	MV_CopyMemory(pTargetSGTable->Entry_Ptr, pSourceSGTable->Entry_Ptr,
					sizeof(MV_SG_Entry)*pTargetSGTable->Valid_Entry_Count);
	pTargetSGTable->prdt_bus_addr.parts.high = pSourceSGTable->prdt_bus_addr.parts.high;
	pTargetSGTable->prdt_bus_addr.parts.low = pSourceSGTable->prdt_bus_addr.parts.low;
}

void MV_CopyPartialSGTable(
	OUT PMV_SG_Table pTargetSGTable,
	IN PMV_SG_Table pSourceSGTable,
	IN MV_U32 offset,
	IN MV_U32 size
	)
{
#ifdef USE_NEW_SGTABLE
	MV_U16  count = pTargetSGTable->Max_Entry_Count;
	sgd_t *entry = pTargetSGTable->Entry_Ptr;

	MV_DASSERT( (count>0) && (entry!=NULL) );
	sgd_table_init( pTargetSGTable, count, entry);
	sgdt_append_reftbl(pTargetSGTable, pSourceSGTable, offset, size);
#else
	MV_ASSERT(MV_FALSE);
#endif
}

#ifdef _OS_BIOS
MV_U64 CPU_TO_BIG_ENDIAN_64(MV_U64 x)

{
	MV_U64 x1;
	ZeroU64(x1);
	x1.parts.low=CPU_TO_BIG_ENDIAN_32(x.parts.high);
	x1.parts.high=CPU_TO_BIG_ENDIAN_32(x.parts.low);
	return x1;
}
#endif	/* #ifdef _OS_BIOS */

MV_BOOLEAN MV_Equals(
	IN MV_PU8		des,
	IN MV_PU8		src,
	IN MV_U32		len
)
{
	MV_U32 i;

	for (i=0; i<len; i++) {
		if (*des != *src)
			return MV_FALSE;
		des++;
		src++;
	}
	return MV_TRUE;
}
/*
 * SG Table operation
 */
void SGTable_Init(
	OUT PMV_SG_Table pSGTable,
	IN MV_U8 flag
	)
{
/*	pSGTable->Max_Entry_Count = MAX_SG_ENTRY;  set during module init */
	pSGTable->Valid_Entry_Count = 0;
#if defined(HAVE_HW_COMPLIANT_SG)
	pSGTable->Flag = SGT_FLAG_PRDT_HW_COMPLIANT | flag;
#else
	pSGTable->Flag = flag;
#endif
	pSGTable->Byte_Count = 0;
	pSGTable->Occupy_Entry_Count = 0;
}

void sgt_init(
       IN MV_U16 max_io,
	OUT PMV_SG_Table pSGTable,
	IN MV_U8 flag
	)
{
	if (max_io == 1)
		pSGTable->Max_Entry_Count = MAX_SG_ENTRY_REDUCED;
	else
		pSGTable->Max_Entry_Count = MAX_SG_ENTRY;

	pSGTable->Valid_Entry_Count = 0;
	pSGTable->Flag = flag;
	pSGTable->Byte_Count = 0;
	pSGTable->Occupy_Entry_Count = 0;
}
#ifndef USE_NEW_SGTABLE
void SGTable_Append(
	OUT PMV_SG_Table pSGTable,
	MV_U32 address,
	MV_U32 addressHigh,
	MV_U32 size
	)
{
	PMV_SG_Entry pSGEntry;

	pSGEntry = &pSGTable->Entry_Ptr[pSGTable->Valid_Entry_Count];

	MV_ASSERT(pSGTable->Valid_Entry_Count < pSGTable->Max_Entry_Count);
	/*
	 * Workaround hardware issue:
	 * If the transfer size is odd, some request cannot be finished.
	 * Hopefully the workaround won't damage the system.
	 */
#ifdef PRD_SIZE_WORD_ALIGN
	if ( size%2 ) size++;
#endif

	pSGTable->Valid_Entry_Count += 1;
	pSGTable->Byte_Count += size;

	pSGEntry->Base_Address = address;
	pSGEntry->Base_Address_High = addressHigh;
	pSGEntry->Size = size;
	#if defined(SUPPORT_BALDUR)
	pSGEntry->ifsel = 0x00;
	#endif
	pSGEntry->Reserved0 = 0;
}
#endif

MV_BOOLEAN SGTable_Available(
	IN PMV_SG_Table pSGTable
	)
{
	return (pSGTable->Valid_Entry_Count < pSGTable->Max_Entry_Count);
}

void MV_InitializeTargetIDTable(
	IN PMV_Target_ID_Map pMapTable
	)
{
	MV_FillMemory((MV_PVOID)pMapTable, sizeof(MV_Target_ID_Map)*MV_MAX_TARGET_NUMBER, 0xFF);
}
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
MV_U16 MV_GetTargetID(IN PMV_Target_ID_Map	pMapTable,IN MV_U8 deviceType)
{
	MV_U16 i;
	for (i=PORT_NUMBER; i < MV_MAX_TARGET_NUMBER; i++) {

		if ( (pMapTable[i].Type==0xff) && (pMapTable[i].Device_Id==0xffff) ) {
			return i;
		}
	}
	return 0xffff;
}
#endif
MV_U16 MV_MapTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i=0xffff;
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
	if(deviceType==TARGET_TYPE_FREE_PD){
		if (pMapTable[deviceId].Type==0xFF) {	/* not mapped yet */
			pMapTable[deviceId].Device_Id = deviceId;
			pMapTable[deviceId].Type = deviceType;

		} else{
			i=MV_GetTargetID(pMapTable,deviceType);

			pMapTable[i].Device_Id = deviceId;
			pMapTable[i].Type = deviceType;
			deviceId=i;
		}
		i=deviceId;
	}else if(deviceType==TARGET_TYPE_LD){
			for (i=PORT_NUMBER; i<MV_MAX_TARGET_NUMBER; i++) {
				if (pMapTable[i].Type==0xFF) {	/* not mapped yet */
					pMapTable[i].Device_Id = deviceId;
					pMapTable[i].Type = deviceType;
					break;
					}
			}
	}

#else
	for (i=0; i<MV_MAX_TARGET_NUMBER; i++) {
		if (pMapTable[i].Type==0xFF) {	/* not mapped yet */
			pMapTable[i].Device_Id = deviceId;
			pMapTable[i].Type = deviceType;
			break;
		}
	}
#endif
	return i;
}

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	/* first check if the device can be mapped to the specific ID */
	if (specificId < MV_MAX_TARGET_NUMBER) {
		if (pMapTable[specificId].Type==0xFF) {	/* not used yet */
			pMapTable[specificId].Device_Id = deviceId;
			pMapTable[specificId].Type = deviceType;
			return specificId;
		}
	}
	/* cannot mapped to the specific ID */
	/* just map the device to first available ID */
	return MV_MapTargetID(pMapTable, deviceId, deviceType);
}

MV_U16 MV_RemoveTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i;
	for (i=0; i < MV_MAX_TARGET_NUMBER; i++) {
		if ( (pMapTable[i].Type==deviceType) && (pMapTable[i].Device_Id==deviceId) ) {
			pMapTable[i].Type = 0xFF;
			pMapTable[i].Device_Id = 0xFFFF;
			break;
		}
	}
	if (i == MV_MAX_TARGET_NUMBER)
		i = 0xFFFF;
	return i;
}

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 mappedID;

	for (mappedID=0; mappedID<MV_MAX_TARGET_NUMBER; mappedID++) {
		if ( (pMapTable[mappedID].Type==deviceType) && (pMapTable[mappedID].Device_Id==deviceId) ){
			break;
			}

	}
	if (mappedID >= MV_MAX_TARGET_NUMBER)
		mappedID = 0xFFFF;
	else {
		MV_DASSERT(mappedID < MV_MAX_TARGET_NUMBER);
#ifdef SUPPORT_SCSI_PASSTHROUGH
		if (mappedID == VIRTUAL_DEVICE_ID) {
			/* Device is on LUN 1 */
			mappedID |= 0x0100;
		}
#endif /* SUPPORT_SCSI_PASSTHROUGH */
	}
	return mappedID;
}

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount)
{
	MV_LBA tmpLBA;
	MV_U32 tmpSectorCount;

	if ((!SCSI_IS_READ(Cdb[0])) &&
	    (!SCSI_IS_WRITE(Cdb[0])) &&
	    (!SCSI_IS_VERIFY(Cdb[0])))
		return;

	/* This is READ/WRITE command */
	switch (Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		tmpLBA.value = (MV_U32)((((MV_U32)(Cdb[1] & 0x1F))<<16) |
					((MV_U32)Cdb[2]<<8) |
					((MV_U32)Cdb[3]));
		tmpSectorCount = (MV_U32)Cdb[4];
		/* sbc, read/write 6 transfer size 0 means 256 blocks  */
		if (tmpSectorCount == 0) {
			tmpSectorCount = 256;
		}
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) |
					((MV_U32)Cdb[3]<<16) |
					((MV_U32)Cdb[4]<<8) |
					((MV_U32)Cdb[5]));
		tmpSectorCount = ((MV_U32)Cdb[7]<<8) | (MV_U32)Cdb[8];
		break;
	case SCSI_CMD_READ_12:
	case SCSI_CMD_WRITE_12:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) |
					((MV_U32)Cdb[3]<<16) |
					((MV_U32)Cdb[4]<<8) |
					((MV_U32)Cdb[5]));
		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[6]<<24) |
					  ((MV_U32)Cdb[7]<<16) |
					  ((MV_U32)Cdb[8]<<8) |
					  ((MV_U32)Cdb[9]));
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
	case SCSI_CMD_VERIFY_16:
		tmpLBA.parts.high = (MV_U32)(((MV_U32)Cdb[2]<<24) |
				       ((MV_U32)Cdb[3]<<16) |
				       ((MV_U32)Cdb[4]<<8) |
				       ((MV_U32)Cdb[5]));
		tmpLBA.parts.low = (MV_U32)(((MV_U32)Cdb[6]<<24) |
				      ((MV_U32)Cdb[7]<<16) |
				      ((MV_U32)Cdb[8]<<8) |
				      ((MV_U32)Cdb[9]));

		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[10]<<24) |
					  ((MV_U32)Cdb[11]<<16) |
					  ((MV_U32)Cdb[12]<<8) |
					  ((MV_U32)Cdb[13]));
		break;

#ifdef SPLIT_RCC_CMD
    case SCSI_MARVELL_CMD_RCC_READ_8:
    case SCSI_MARVELL_CMD_RCC_WRITE_8:
        U64_SET_VALUE(tmpLBA, 0);
        tmpSectorCount = 0;
        break;
#endif

	default:
		MV_DPRINT(("Unsupported READ/WRITE command [%x]\n", Cdb[0]));
		U64_SET_VALUE(tmpLBA, 0);
		tmpSectorCount = 0;
	}
	*pLBA = tmpLBA;
	*pSectorCount = tmpSectorCount;
}

void MV_CodeReadWriteCDB(
	OUT MV_PU8	Cdb,
	IN MV_LBA	lba,
	IN MV_U32	sector,
	IN MV_U8	operationCode	/* The CDB[0] */
	)
{
	MV_DASSERT(
		SCSI_IS_READ(operationCode)
		|| SCSI_IS_WRITE(operationCode)
		|| SCSI_IS_VERIFY(operationCode)
		);

	MV_ZeroMemory(Cdb, MAX_CDB_SIZE);
	Cdb[0] = operationCode;

	/* This is READ/WRITE command */
	switch ( Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		Cdb[1] = (MV_U8)(lba.value>>16);
		Cdb[2] = (MV_U8)(lba.value>>8);
		Cdb[3] = (MV_U8)lba.value;
		Cdb[4] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
		Cdb[2] = (MV_U8)(lba.value>>24);
		Cdb[3] = (MV_U8)(lba.value>>16);
		Cdb[4] = (MV_U8)(lba.value>>8);
		Cdb[5] = (MV_U8)lba.value;
		Cdb[7] = (MV_U8)(sector>>8);
		Cdb[8] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_12:
	case SCSI_CMD_WRITE_12:
		Cdb[2] = (MV_U8)(lba.value>>24);
		Cdb[3] = (MV_U8)(lba.value>>16);
		Cdb[4] = (MV_U8)(lba.value>>8);
		Cdb[5] = (MV_U8)lba.value;
		Cdb[6] = (MV_U8)(sector>>24);
		Cdb[7] = (MV_U8)(sector>>16);
		Cdb[8] = (MV_U8)(sector>>8);
		Cdb[9] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
	case SCSI_CMD_VERIFY_16:
		Cdb[2] = (MV_U8)(lba.parts.high>>24);
		Cdb[3] = (MV_U8)(lba.parts.high>>16);
		Cdb[4] = (MV_U8)(lba.parts.high>>8);
		Cdb[5] = (MV_U8)lba.parts.high;
		Cdb[6] = (MV_U8)(lba.parts.low>>24);
		Cdb[7] = (MV_U8)(lba.parts.low>>16);
		Cdb[8] = (MV_U8)(lba.parts.low>>8);
		Cdb[9] = (MV_U8)lba.parts.low;
		Cdb[10] = (MV_U8)(sector>>24);
		Cdb[11] = (MV_U8)(sector>>16);
		Cdb[12] = (MV_U8)(sector>>8);
		Cdb[13] = (MV_U8)sector;
		break;
	default:
		MV_DPRINT(("Unsupported READ/WRITE command [%x]\n", Cdb[0]));
		MV_ASSERT(0);
	}
}


#ifndef _OS_BIOS
void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail)
{
#ifndef SUPPORT_INEJECT_ERROR
	MV_DPRINT(("Device %d MV_Request: Cdb[%02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x].\n",
		pReq->Device_Id,
		pReq->Cdb[0],
		pReq->Cdb[1],
		pReq->Cdb[2],
		pReq->Cdb[3],
		pReq->Cdb[4],
		pReq->Cdb[5],
		pReq->Cdb[6],
		pReq->Cdb[7],
		pReq->Cdb[8],
		pReq->Cdb[9],
		pReq->Cdb[10],
		pReq->Cdb[11],
		pReq->Cdb[12],
		pReq->Cdb[13],
		pReq->Cdb[14],
		pReq->Cdb[15]));

	if (detail) {
		MV_PRINT("Req Flag=0x%x Sts=0x%x Tag 0x%x len 0x%x, sense len %d Org_req: %p\n", 
			pReq->Cmd_Flag, 
			pReq->Scsi_Status,
			pReq->Tag,
			pReq->Data_Transfer_Length,
			pReq->Sense_Info_Buffer_Length,
			pReq->Org_Req);
	}
#endif
}

#ifndef _MARVELL_SDK_PACKAGE_NONRAID
#if defined(SUPPORT_RAID6) && (defined(HARDWARE_XOR) || defined(SOFTWARE_XOR))
void MV_DumpXORRequest(PMV_XOR_Request pXORReq, MV_BOOLEAN detail)
{
	MV_U32 i,j;

	MV_PRINT("MV_XOR_Request: Type=0x%x, Source count=%d, Target count=%d\n",
		pXORReq->Request_Type,
		pXORReq->Source_SG_Table_Count,
		pXORReq->Target_SG_Table_Count
		);

	if ( detail ) {
		MV_PRINT("Source SG table...\n");
		for ( i=0; i<pXORReq->Source_SG_Table_Count; i++ )
			MV_DumpSGTable(&pXORReq->Source_SG_Table_List[i]);

		MV_PRINT("Target SG table...\n");
		for ( i=0; i<pXORReq->Target_SG_Table_Count; i++ )
			MV_DumpSGTable(&pXORReq->Target_SG_Table_List[i]);

		MV_PRINT("Coefficient...\n");
		for ( i=0; i<pXORReq->Target_SG_Table_Count; i++ ) {
			for ( j=0; j<pXORReq->Source_SG_Table_Count; j++ ) {
				MV_PRINT("[%d,%d]=0x%x\n", i, j, pXORReq->Coef[i][j]);
			}
		}
	}
}
#endif /* SUPPORT_RAID6 */
#endif

void MV_DumpSGTable(PMV_SG_Table pSGTable)
{
#ifdef USE_NEW_SGTABLE
	sgdt_dump(pSGTable, " ");
#else
	PMV_SG_Entry pSGEntry;
	MV_U32 i;
	MV_PRINT("SG Table: size(0x%x)\n", pSGTable->Byte_Count);
	for (i=0; i<pSGTable->Valid_Entry_Count; i++) {
		pSGEntry = &pSGTable->Entry_Ptr[i];
		MV_PRINT("%d: addr(0x%x-0x%x), size(0x%x).\n",
			i, pSGEntry->Base_Address_High, pSGEntry->Base_Address, pSGEntry->Size);
	}
#endif
}

const char* MV_DumpSenseKey(MV_U8 sense)
{
	switch ( sense )
	{
		case SCSI_SK_NO_SENSE:
			return "SCSI_SK_NO_SENSE";
		case SCSI_SK_RECOVERED_ERROR:
			return "SCSI_SK_RECOVERED_ERROR";
		case SCSI_SK_NOT_READY:
			return "SCSI_SK_NOT_READY";
		case SCSI_SK_MEDIUM_ERROR:
			return "SCSI_SK_MEDIUM_ERROR";
		case SCSI_SK_HARDWARE_ERROR:
			return "SCSI_SK_HARDWARE_ERROR";
		case SCSI_SK_ILLEGAL_REQUEST:
			return "SCSI_SK_ILLEGAL_REQUEST";
		case SCSI_SK_UNIT_ATTENTION:
			return "SCSI_SK_UNIT_ATTENTION";
		case SCSI_SK_DATA_PROTECT:
			return "SCSI_SK_DATA_PROTECT";
		case SCSI_SK_BLANK_CHECK:
			return "SCSI_SK_BLANK_CHECK";
		case SCSI_SK_VENDOR_SPECIFIC:
			return "SCSI_SK_VENDOR_SPECIFIC";
		case SCSI_SK_COPY_ABORTED:
			return "SCSI_SK_COPY_ABORTED";
		case SCSI_SK_ABORTED_COMMAND:
			return "SCSI_SK_ABORTED_COMMAND";
		case SCSI_SK_VOLUME_OVERFLOW:
			return "SCSI_SK_VOLUME_OVERFLOW";
		case SCSI_SK_MISCOMPARE:
			return "SCSI_SK_MISCOMPARE";
		default:
			MV_DPRINT(("Unknown sense key 0x%x.\n", sense));
			return "Unknown sense key";
	}
}
#endif	/* #ifndef _OS_BIOS */

static MV_U32  BASEATTR crc_tab[] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
        0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
        0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
        0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
        0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
        0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
        0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
        0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
        0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
        0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
        0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
        0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
        0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
        0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
        0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
        0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
        0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
        0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
        0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
        0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
        0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
        0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
        0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
        0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
        0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
        0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
        0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
        0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
        0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
        0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
        0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
        0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
        0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
        0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
        0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
        0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
        0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
        0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
        0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
        0x2d02ef8dL
};

/* Calculate CRC and generate PD_Reference number */
MV_U32 MV_CRC_LOOP(
	IN	MV_PU8		pData,
	IN	MV_U16		len,
	IN    MV_U32		next_crc,
	IN    MV_U8		start
)
{
    MV_U16 i;
    MV_U32 crc;
    if(start)
    	crc = MV_MAX_U32;
    else
    	crc = MV_BE32_TO_CPU(next_crc);
    for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
    }
    return MV_CPU_TO_BE32(crc);
}

MV_U32 MV_CRC(
	IN	MV_PU8		pData,
	IN	MV_U16		len
)
{
	MV_U16 i;
	MV_U32 crc = MV_MAX_U32;

	for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
	}
	return MV_CPU_TO_BE32(crc);
}

MV_U32 MV_CRC_EXT(
	IN	MV_U32		crc,
	IN	MV_PU8		pData,
	IN	MV_U32		len
)
{
	MV_U16 i;

	for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

#ifdef MV_DEBUG
void MV_CHECK_OS_SG_TABLE(
	IN PMV_SG_Table pSGTable
	)
{
#ifndef USE_NEW_SGTABLE
	/* Check whether there are duplicated entries pointed to the same physical address. */
	MV_U32 i,j;
	static MV_BOOLEAN assertSGTable = MV_TRUE;

	if ( assertSGTable ) {
		for ( i=0; i<pSGTable->Valid_Entry_Count; i++ ) {
			for ( j=i+1; j<pSGTable->Valid_Entry_Count; j++ ) {
				MV_DASSERT( pSGTable->Entry_Ptr[i].Base_Address!=pSGTable->Entry_Ptr[j].Base_Address );
			}
		}
	}
#endif
}
#endif /* MV_DEBUG */


#if defined(RAID_DRIVER)
MV_PVOID sgd_kmap(sgd_t  *sg)
{
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( pctx->u.xctx );
	return ossw_kmap(pctx->u.xctx);
#endif

#ifdef _OS_WINDOWS
	sgd_pctx_t      *pctx = (sgd_pctx_t *) sg;
	MV_PTR_INTEGER  addr = (MV_PTR_INTEGER) (pctx->baseAddr.value);

	MV_DASSERT(sg->flags & SGD_PCTX);
	/* just for fun, refer to GenerateSGTable in win_helper.c */
	addr &= ~0x80000000L;

	return (MV_PVOID) addr;
#endif
}

MV_VOID sgd_kunmap(sgd_t  *sg,MV_PVOID mapped_addr)
{
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( pctx->u.xctx );
	ossw_kunmap(pctx->u.xctx, mapped_addr);
#endif

#ifdef _OS_WINDOWS
#endif
}
 MV_PVOID sgd_kmap_sec(sgd_t  *sg)
{
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( pctx->u.xctx );
	return ossw_kmap_sec(pctx->u.xctx);
#endif

#ifdef _OS_WINDOWS
	return sgd_kmap(sg);
#endif
}

MV_VOID sgd_kunmap_sec(sgd_t  *sg,MV_PVOID	mapped_addr)
{
#ifdef _OS_LINUX
	sgd_pctx_t* pctx = (sgd_pctx_t*)sg;
	MV_DASSERT( sg->flags & SGD_PCTX );
	MV_DASSERT( pctx->u.xctx );
	ossw_kunmap_sec(pctx->u.xctx, mapped_addr);
#endif

#ifdef _OS_WINDOWS
	sgd_kunmap(sg,mapped_addr);
#endif
}

#ifdef SOFTWARE_XOR
static MV_VOID swxor_sg_memcpy(
        xor_strm_t      *strm, /*strm[0]:source, strm[1]:destination*/
        MV_U32          byte_cnt)
{
	MV_U32		i;
	#ifdef MV_DEBUG
	MV_U32      sz;
	#endif
	MV_PU32		pSrc[2] = { NULL, NULL };
	MV_BOOLEAN	mapped[2] = { MV_FALSE, MV_FALSE };
	MV_PVOID	p = NULL;
	MV_PVOID 	ptmp = NULL;
	unsigned long flags = 0;

	for( i = 0; i < 2; i++ ){
	    #ifdef MV_DEBUG
		sgd_getsz(strm[i].sgd,sz);
		#endif
		MV_DASSERT( strm[i].off < sz );
		MV_DASSERT( strm[i].off+byte_cnt <= sz );
	}
	hba_local_irq_save(flags);

	for( i = 0; i < 2; i++ ){
		if( strm[i].sgd[0].flags & SGD_PCTX ){
			if(i==0)
				ptmp = (MV_PU32) sgd_kmap(strm[i].sgd);
			else
				ptmp = (MV_PU32) sgd_kmap_sec(strm[i].sgd);
			mapped[i] = MV_TRUE;
		#ifdef _OS_LINUX
			pSrc[i] = (MV_PU32)((MV_PU8)ptmp + strm[i].sgd[1].size);
		#else
			pSrc[i] = (MV_PU32)ptmp;
		#endif
		}else{
			sgd_get_vaddr( strm[i].sgd, p );
			pSrc[i] = (MV_PU32) p;
		}
	}
	MV_CopyMemory(
		pSrc[1]+strm[1].off/sizeof(MV_U32),
		pSrc[0]+strm[0].off/sizeof(MV_U32),
		byte_cnt );

	for(i = 0; i < 2; i++ ) {
		if( mapped[i] ) {
			if(i==0)
				sgd_kunmap(strm[i].sgd, ptmp);
			else
				sgd_kunmap_sec(strm[i].sgd, ptmp);
		}
	}
	hba_local_irq_restore(flags);
	return;
}

MV_VOID swxor_cpy_sg(
        PMV_SG_Table    srctbl,
        PMV_SG_Table    dsttbl)
{
        sgd_iter_t      sg_iter[2];
        MV_U32          wCount[2], count;
        MV_U8           bFinished = MV_FALSE;
        MV_U8           bIndex;
        xor_strm_t      strm[2];

        MV_ASSERT(srctbl->Byte_Count == dsttbl->Byte_Count);

        sgd_iter_init(&sg_iter[0], srctbl->Entry_Ptr, 0, srctbl->Byte_Count);
        sgd_iter_init(&sg_iter[1], dsttbl->Entry_Ptr, 0, dsttbl->Byte_Count);

        for (bIndex = 0; bIndex < 2; bIndex++) {
                strm[bIndex].off = 0;
                sgd_iter_get_next(&sg_iter[bIndex], strm[bIndex].sgd);
                sgd_getsz(strm[bIndex].sgd, wCount[bIndex]);
        }

        while(!bFinished) {
                count = MV_MIN(wCount[0], wCount[1]);

                swxor_sg_memcpy(strm, count);

                for (bIndex = 0; bIndex < 2; bIndex++) {
                        wCount[bIndex] -= count;
                        if (wCount[bIndex] == 0) {
                                if (!sgd_iter_get_next(
                                        &sg_iter[bIndex],
                                        strm[bIndex].sgd )) {

                                        bFinished = MV_TRUE;
                                        break;
                                }
                                sgd_getsz(strm[bIndex].sgd, wCount[bIndex]);
                                strm[bIndex].off = 0;
                        }
                        else
                                strm[bIndex].off += count;
                }
        }
}
#endif

#endif /* RAID_DRIVER */
#ifdef SUPPORT_MUL_LUN
MV_VOID init_target_id_map(MV_U16 *map_table, MV_U32 size)
{
	MV_FillMemory(map_table, size, 0xFFFF);
}

MV_U16 get_available_target_id(MV_U16 *map_table, MV_U16 id)
{
	MV_U16 i;

	for (i=0; i<id; i++) {
		if (map_table[i] == 0xffff)
			return i;
	}
	return id;

}
MV_U16 add_target_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id)
{
	MV_U16 target_id;
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
	target_id=device_id;
	if(target_id >= max_id)
		return 0xFFFF;
	map_table[target_id] = device_id;

#else
	target_id = get_available_target_id(map_table, max_id);
	if(target_id >= max_id)
		return 0xFFFF;
	map_table[target_id] = device_id;
#endif
	return target_id;
}

MV_U16 remove_target_map(MV_U16 *map_table, MV_U16 target_id, MV_U16 max_id)
{
	//if(target_id == 0xFFFF)
	//	return target_id;
	if((target_id != 0xFFFF) && ((target_id >= max_id)))
		return target_id;
	MV_FillMemory(&map_table[target_id], sizeof(MV_U16), 0xFFFF);
	return target_id;

}
#endif
#ifdef CORE_SCSI_ID_MAP_FOR_NONE_RAID
MV_VOID init_reported_id_map(MV_U16 *map_table, MV_U32 size)
{
	MV_FillMemory(map_table, size, 0xFF);
}
MV_U16 get_device_id_by_reported_id(MV_U16 *map_table, MV_U16 reported_id, MV_U16 max_id)
{
	if(!(reported_id < max_id))
		return 0xFFFF;
	return map_table[reported_id];
}
MV_U16 get_reported_id_by_device_id(MV_U16 *map_table, MV_U16 device_id, MV_U16 max_id)
{
	MV_U16 i;
	for(i = 0; i< max_id; i++){
		if((map_table[i] == device_id))
			return i;
		}
	return 0xFFFF;
}
MV_U16 get_available_report_id(MV_U16 *map_table, MV_U16 max_id)
{
	MV_U16 i;

	for (i=0; i<max_id; i++) {
		if (map_table[i] == 0xFFFF)
			return i;
	}
	return max_id;
}
MV_U16 release_available_report_id(MV_U16 *map_table, MV_U16 device_id, MV_U16 max_id)
{
	MV_U16 reported_id = get_reported_id_by_device_id(map_table, device_id,  max_id);

	if(reported_id == 0xFFFF)
		return reported_id;
	if(reported_id >= max_id)
		return reported_id;
	MV_FillMemory(&map_table[reported_id], sizeof(MV_U16), 0xFF);
	return reported_id;

}
MV_U16 add_report_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id)
{
	MV_U16 report_id;

	report_id = get_available_report_id(map_table,max_id);
	if(report_id >= max_id)
		return 0xFFFF;
	map_table[report_id] = device_id;
	return report_id;
}
MV_U16 add_report_map_specific_id(MV_U16 *map_table, MV_U16 device_id, MV_U16 map_id, MV_U16 max_id)
{
	if(map_id >= max_id)
		return 0;
	if(map_table[map_id] != 0xFFFF)
		return 0;
	map_table[map_id] = device_id;
	return 1;

}
MV_U16 remove_report_map(MV_U16 *map_table, MV_U16 device_id, MV_U16 max_id)
{
	return release_available_report_id(map_table, device_id, max_id);
}
#endif
