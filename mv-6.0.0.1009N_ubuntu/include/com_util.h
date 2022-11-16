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

#ifndef __MV_COM_UTIL_H__
#define __MV_COM_UTIL_H__

#include "com_define.h"
#include "com_type.h"

#ifdef _OS_BIOS
#include "com_sgd.h"
void MV_ZeroMemory(MV_PVOID buffer, MV_U32 byte_count);
void MV_FillMemory(MV_PVOID buffer, MV_U32 byte_count, MV_U8 pattern);
void MV_CopyMemory(MV_PVOID dest, MV_PVOID source, MV_U32 byte_count);
MV_BOOLEAN MV_CompareMemory(MV_PVOID buf0, MV_PVOID buf1, MV_U32 len);
#else //_os_bios

#ifdef _OS_UKRN

    extern void memory_byte_copy(void *dst, const void *src, unsigned int nr_bytes);

#   define MV_ZeroMemory(buf, len)           memset(buf, 0, len)
#   define MV_FillMemory(buf, len, pattern)  memset(buf, pattern, len)

#   define MV_CopyMemory(dest, source, len)  do {           \
        if ((unsigned) dest & 0x3 || (unsigned) source & 3) {  \
            memory_byte_copy(dest, source, len);            \
        }                                                   \
        else {                                              \
            memcpy(dest, source, len);                      \
        }                                                   \
    } while (0)

#   define MV_CompareMemory(buf0, buf1, len)  memcmp(buf0, buf1, len)

#   define _rotr(v, i)		( (v>>i) | (v<<(32-i)) )

static __forceinline unsigned int _clz(unsigned int v)
{
    __asm { CLZ v, v }
    return v;
}

static __forceinline unsigned int  _ctz(unsigned int  v)
{
    /*
     * ~v & (v-1) forms a mask that identifies the trailing 0's, producing
     * all 1's if v = 0 (e.g., 01011000->00000111)
     */

    return 32 - _clz(~v & (v-1));
}

// ffs: find first set, returns the position of the LSB set to 1
static __inline int ossw_ffs(MV_U32 v)
{
    return v ? (int) _ctz(v) : -1;
}

// ffc: find first clear, returns the position of the LSB set to 0
#   define ffc(v)  ossw_ffs(~(v))
#   define fc(v, i) (ossw_ffs(~(_rotr(v, i))) + i)

static __inline int
ffc64(MV_U64 v)
{
    int i;

    if ((i = ffc(v.parts.low)) >= 0) {
        return i;
    }

    if ((i = ffc(v.parts.high)) >= 0) {
        return 32 + i;
    }

	return -1;
}

#elif defined( SUPPORT_ROC )
#if !defined(SUPPORT_BALDUR)

#define MV_ZeroMemory(buf, len)           __builtin_memset((buf), 0, (len))
#define MV_FillMemory(buf, len, pattern)  __builtin_memset((buf), (pattern), (len))
//#define MV_CopyMemory(dest, source, len)  __builtin_memcpy((dest), (source), (len))
#define MV_CopyMemory(dest, source, len)  memcpy(dest, source, len)
#define MV_CompareMemory(buf0, buf1, len) __builtin_memcmp((buf0), (buf1), (len))

#define ossw_ffs(a)		(__builtin_ffsl(a) - 1)
#define ffc(v)  ffs(~(v))
#define ffc64(a)	(__builtin_ffsll(~((a).value)) - 1)
#define _rotr(v, i)		( (v>>i) | (v<<(32-i)) )
#define fc(v, i) (ossw_ffs(~(_rotr(v, i)))+ i)

#else //SUPPORT_BALDUR

#define MV_ZeroMemory(buf, len)           memset(buf, 0, len)
#define MV_FillMemory(buf, len, pattern)  memset(buf, pattern, len)
#define MV_CopyMemory(dest, source, len)  memcpy(dest, source, len)
#define MV_CompareMemory(buf0, buf1, len)  memcmp(buf0, buf1, len)

//#define MV_CLZ(a)		__builtin_clzl(a)
#define ossw_ffs(a)		(__builtin_ffsl(a) - 1)
#define ffc(v)  ffs(~(v))
#define ffc64(a)	(__builtin_ffsll(~((a).value)) - 1)
#define _rotr(v, i)		( (v>>i) | (v<<(32-i)) )
#define fc(v, i) (ossw_ffs(~(_rotr(v, i)))+ i)
#endif //SUPPORT_BALDUR

#else //SUPPORT_ROC


#ifndef _OS_LINUX
#define MV_ZeroMemory(buf, len)           memset(buf, 0, len)
#define MV_FillMemory(buf, len, pattern)  memset(buf, pattern, len)
#define MV_CopyMemory(dest, source, len)  memcpy(dest, source, len)
#define MV_CompareMemory(buf0, buf1, len)  memcmp(buf0, buf1, len)
#else
#define MV_ZeroMemory(buf, len)           ossw_memset(buf, 0, len)
#define MV_FillMemory(buf, len, pattern)  ossw_memset(buf, pattern, len)
#define MV_CopyMemory(dest, source, len)  ossw_memcpy(dest, source, len)
#define MV_CompareMemory(buf0, buf1, len)  ossw_memcmp(buf0, buf1, len)
#endif//_OS_LINUX


#ifndef SIMULATOR
#ifndef _OS_LINUX
/*
 * _BitScanForward searches the mask data from least significant bit (LSB) to
 * the most significant bit (MSB) for a set bit (1)
 */
#pragma intrinsic(_BitScanForward)

// ffs: find first set, returns the position of the LSB set to 1
__inline int
ossw_ffs(MV_U32 v)
{
	unsigned long index;
	return _BitScanForward(&index, v) ? (int) index : -1;
}

// ffc: find first clear, returns the position of the LSB set to 0
#define ffc(v)  ossw_ffs(~(v))
#define fc(v, i) (ossw_ffs(~(_rotr(v, i))) + i)
#else

 /* ffz - find first zero in word. define in linux kernel bitops.h*/
#define ffc(v)  ossw_ffz(v) 
#define _rotr(v, i)		ossw_rotr32(v, i)
/*ffs - normally return from 1 to MSB, if not find set bit, return 0*/
#define fc(v, i) (ossw_ffs(~(_rotr(v, i))) + i)
#endif /*#ifndef _OS_LINUX*/

static __inline int
ffc64(MV_U64 v)
{
    int i;

    if ((i = ffc(v.parts.low)) >= 0) {
        return i;
    }

    if ((i = ffc(v.parts.high)) >= 0) {
        return 32 + i;
    }

	return -1;
}
#endif //#ifndef SIMULATOR
#endif //SUPPORT_ROC
#endif //_os_bios

void MV_ZeroMvRequest(PMV_Request pReq);
void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable);
/* offset and size are all byte count. */
void MV_CopyPartialSGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable, MV_U32 offset, MV_U32 size);

MV_BOOLEAN MV_Equals(MV_PU8 des, MV_PU8 src, MV_U32 len);

#ifndef _OS_BIOS
#define	U64_ASSIGN(x,y)				  	((x).value = (y))
#define	U64_ASSIGN_U64(x,y)			  	((x).value = (y).value)
#define	U64_COMP_U64(x,y)			  	((x) == (y).value)
#define U64_COMP_U64_VALUE(x,y)			((x).value == (y).value)
#define U32_ASSIGN_U64(v64, v32)		((v64).value = (v32))
#define	U64_SHIFT_LEFT(v64, v32)		((v64).value=(v64).value << (v32))
#define	U64_SHIFT_RIGHT(v64, v32)		((v64).value=(v64).value >> (v32))
#define U64_ZERO_VALUE(v64)				((v64).value = 0)
#else
#define	U64_ASSIGN(x,y)					((x) = (y))
#define	U64_ASSIGN_U64(x,y)				((x) = (y))
#define	U64_COMP_U64(x,y)			  	(U64_COMPARE_U64(x, y)==0)
#define U64_COMP_U64_VALUE(x,y)			(U64_COMPARE_U64(x, y)==0)
#define U32_ASSIGN_U64(v64, v32)		do { (v64).parts.low = v32; (v64).parts.high = 0; } while(0);
#define U64_SHIFT_LEFT(v64, v32) 		do {(v64).parts.high = ((v64).parts.high << v32) | ((v64).parts.low >> (32 - v32)); (v64).parts.low = (v64).parts.low << v32; } while(0);
#define U64_SHIFT_RIGHT(v64, v32) 		do {(v64).parts.low = ((v64).parts.high << (32 - v32)) | ((v64).parts.low >> v32); (v64).parts.high = (v64).parts.high >> v32; } while(0);
#define U64_ZERO_VALUE(v64)				do { (v64).parts.low = (v64).parts.high = 0; } while(0);
#endif

#define MV_SWAP_32(x)                             \
           (((MV_U32)((MV_U8)(x)))<<24 |          \
            ((MV_U32)((MV_U8)((x)>>8)))<<16 |     \
            ((MV_U32)((MV_U8)((x)>>16)))<<8 |     \
            ((MV_U32)((MV_U8)((x)>>24))) )
#define MV_SWAP_64(x)                             \
           (((_MV_U64) (MV_SWAP_32((x).parts.low))) << 32 | \
	    MV_SWAP_32((x).parts.high))
#define MV_SWAP_16(x)                             \
           (((MV_U16) ((MV_U8) (x))) << 8 |       \
	    (MV_U16) ((MV_U8) ((x) >> 8)))

#if !defined(_OS_LINUX) && !defined(__QNXNTO__)
#   ifndef __MV_BIG_ENDIAN__

#      define MV_CPU_TO_BE32(x)     MV_SWAP_32(x)
#      define MV_CPU_TO_BE64(x)     MV_SWAP_64(x)
#      define MV_CPU_TO_BE16(x)     MV_SWAP_16(x)
#      define MV_BE16_TO_CPU(x)     MV_SWAP_16(x)
#      define MV_BE32_TO_CPU(x)     MV_SWAP_32(x)
#      define MV_BE64_TO_CPU(x)     MV_SWAP_64(x)

#      define MV_CPU_TO_LE16(x)     x
#      define MV_CPU_TO_LE32(x)     x
#      define MV_CPU_TO_LE64(x)     (x).value
#      define MV_LE16_TO_CPU(x)     x
#      define MV_LE32_TO_CPU(x)     x
#      define MV_LE64_TO_CPU(x)     (x).value

#   else  /* __MV_BIG_ENDIAN__ */

#      define MV_CPU_TO_BE32(x)     x
#      define MV_CPU_TO_BE64(x)     (x).value
#      define MV_CPU_TO_BE16(x)     x
#      define MV_BE16_TO_CPU(x)     x
#      define MV_BE32_TO_CPU(x)     x
#      define MV_BE64_TO_CPU(x)     (x).value

#      define MV_CPU_TO_LE16(x)     MV_SWAP_16(x)
#      define MV_CPU_TO_LE32(x)     MV_SWAP_32(x)
#      define MV_CPU_TO_LE64(x)     MV_SWAP_64(x)
#      define MV_LE16_TO_CPU(x)     MV_SWAP_16(x)
#      define MV_LE32_TO_CPU(x)     MV_SWAP_32(x)
#      define MV_LE64_TO_CPU(x)     MV_SWAP_64(x)

#   endif /* __MV_BIG_ENDIAN__ */

#else /* !_OS_LINUX */

#define MV_CPU_TO_LE16      ossw_cpu_to_le16
#define MV_CPU_TO_LE32      ossw_cpu_to_le32
#define MV_CPU_TO_LE64(x)   ossw_cpu_to_le64((x).value)
#define MV_CPU_TO_BE16      ossw_cpu_to_be16
#define MV_CPU_TO_BE32      ossw_cpu_to_be32
#define MV_CPU_TO_BE64(x)   ossw_cpu_to_be64((x).value)

#define MV_LE16_TO_CPU      ossw_le16_to_cpu
#define MV_LE32_TO_CPU      ossw_le32_to_cpu
#define MV_LE64_TO_CPU(x)   ossw_le64_to_cpu((x).value)
#define MV_BE16_TO_CPU      ossw_be16_to_cpu
#define MV_BE32_TO_CPU      ossw_be32_to_cpu
#define MV_BE64_TO_CPU(x)   ossw_be64_to_cpu((x).value)

#endif /* !_OS_LINUX */

/*
 * big endian bit-field structs that are larger than a single byte
 * need swapping
 */
#ifdef __MV_BIG_ENDIAN__
#define MV_CPU_TO_LE16_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_CPU_TO_LE16(*(MV_PU16) (pu16))
#define MV_CPU_TO_LE32_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_CPU_TO_LE32(*(MV_PU32) (pu32))

#define MV_LE16_TO_CPU_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_LE16_TO_CPU(*(MV_PU16) (pu16))
#define MV_LE32_TO_CPU_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_LE32_TO_CPU(*(MV_PU32) (pu32))
# else  /* __MV_BIG_ENDIAN__ */
#define MV_CPU_TO_LE16_PTR(pu16)        /* Nothing */
#define MV_CPU_TO_LE32_PTR(pu32)        /* Nothing */
#define MV_LE16_TO_CPU_PTR(pu32)
#define MV_LE32_TO_CPU_PTR(pu32)
#endif /* __MV_BIG_ENDIAN__ */

/* definitions - following macro names are used by RAID module
   must keep consistent */
#define CPU_TO_BIG_ENDIAN_16(x)        MV_CPU_TO_BE16(x)
#define CPU_TO_BIG_ENDIAN_32(x)        MV_CPU_TO_BE32(x)
#ifdef _OS_BIOS
MV_U64 CPU_TO_BIG_ENDIAN_64(MV_U64 x);
#else
#define CPU_TO_BIG_ENDIAN_64(x)        MV_CPU_TO_BE64(x)
#endif

void SGTable_Init(
    OUT PMV_SG_Table pSGTable,
    IN MV_U8 flag
    );

void sgt_init(
    IN MV_U16 max_io,
    OUT PMV_SG_Table pSGTable,
    IN MV_U8 flag
    );

#ifndef USE_NEW_SGTABLE
void SGTable_Append(
    OUT PMV_SG_Table pSGTable,
    MV_U32 address,
    MV_U32 addressHigh,
    MV_U32 size
    );
#else
#define SGTable_Append sgdt_append
#endif

MV_BOOLEAN SGTable_Available(
    IN PMV_SG_Table pSGTable
    );

void MV_InitializeTargetIDTable(
    IN PMV_Target_ID_Map pMapTable
    );
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
MV_U16 MV_GetTargetID(
	IN PMV_Target_ID_Map	
	pMapTable,IN MV_U8 deviceType
	);
#endif
MV_U16 MV_MapTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

MV_U16 MV_RemoveTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount);

void MV_CodeReadWriteCDB(
	OUT MV_PU8	Cdb,
	IN MV_LBA	lba,
	IN MV_U32	sector,
	IN MV_U8	operationCode	/* The CDB[0] */
	);

#ifdef _OS_BIOS
void MV_SetLBAandSectorCount(PMV_Request pReq);
#else
#define MV_SetLBAandSectorCount(pReq) do {								\
	MV_DecodeReadWriteCDB(pReq->Cdb, &pReq->LBA, &pReq->Sector_Count);	\
	pReq->Req_Flag |= REQ_FLAG_LBA_VALID;								\
} while (0)
#endif

void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail);
#if defined(SUPPORT_RAID6) && (defined(HARDWARE_XOR) || defined(SOFTWARE_XOR))
void MV_DumpXORRequest(PMV_XOR_Request pXORReq, MV_BOOLEAN detail);
#endif
void MV_DumpSGTable(PMV_SG_Table pSGTable);
const char* MV_DumpSenseKey(MV_U8 sense);

MV_U32 MV_CRC(
	IN MV_PU8  pData,
	IN MV_U16  len
);
MV_U32 MV_CRC_EXT(
	IN	MV_U32		crc,
	IN	MV_PU8		pData,
	IN	MV_U32		len
);

#define MV_MOD_ADD(value, mod)                    \
           do {                                   \
              (value)++;                          \
              if ((value) >= (mod))               \
                 (value) = 0;                     \
           } while (0);

#ifdef MV_DEBUG
void MV_CHECK_OS_SG_TABLE(
    IN PMV_SG_Table pSGTable
    );
#endif /* MV_DEBUG */

/* used for endian-ness conversion */
static inline MV_VOID mv_swap_bytes(MV_PVOID buf, MV_U32 len)
{
	MV_U32 i;
	MV_U8  tmp, *p;

	/* we expect len to be in multiples of 2 */
	if (len & 0x1)
		return;

	p = (MV_U8 *) buf;
	for (i = 0; i < len / 2; i++)
	{
		tmp = p[i];
		p[i] = p[len - i - 1];
		p[len - i - 1] = tmp;
	}
}


#if defined(RAID_DRIVER)
typedef struct _xor_strm_t
{
        sgd_t           sgd[2];
        MV_U32          off;
} xor_strm_t;

/* cache module will use this function which is defined in software XOR */
void swxor_cpy_sg(PMV_SG_Table srctbl, PMV_SG_Table dsttbl);
MV_PVOID sgd_kmap(sgd_t  *sg);
MV_VOID sgd_kunmap(sgd_t  *sg,MV_PVOID mapped_addr);
MV_PVOID sgd_kmap_sec(sgd_t  *sg);
MV_VOID sgd_kunmap_sec(sgd_t  *sg,MV_PVOID	mapped_addr);
#endif /* RAID_DRIVER */
#ifdef SUPPORT_MUL_LUN
MV_VOID init_target_id_map(MV_U16 *map_table, MV_U32 size);
MV_U16 add_target_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id);
MV_U16 remove_target_map(MV_U16 *map_table, MV_U16 target_id, MV_U16 max_id);
#endif
#ifdef CORE_SCSI_ID_MAP_FOR_NONE_RAID
MV_VOID init_reported_id_map(MV_U16 *map_table, MV_U32 size);
MV_U16 get_device_id_by_reported_id(MV_U16 *map_table, MV_U16 reported_id, MV_U16 max_id);
MV_U16 get_reported_id_by_device_id(MV_U16 *map_table, MV_U16 device_id, MV_U16 max_id);
MV_U16 add_report_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id);
MV_U16 add_report_map_specific_id(MV_U16 *map_table, MV_U16 device_id, MV_U16 map_id, MV_U16 max_id);
MV_U16 remove_report_map(MV_U16 *map_table, MV_U16 device_id, MV_U16 max_id);
#endif

#endif /*  __MV_COM_UTIL_H__ */
