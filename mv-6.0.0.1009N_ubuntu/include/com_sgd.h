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

#ifndef __MV_COM_SGD_H__
#define __MV_COM_SGD_H__

struct _sgd_tbl_t;
struct _sgd_t;

#define SGD_DOMAIN_MASK	0xF0000000L

#ifndef HAVE_HW_COMPLIANT_SG
#define SGD_EOT			(1L<<27)	/* End of table */
#define SGD_WIDE		(1L<<25)	/* 32 byte SG format */
#define SGD_X64			(1L<<24)	/* the 2nd part of SGD_WIDE */
#define SGD_NEXT_TBL	(1L<<23)	/* Next SG table format */
#define SGD_VIRTUAL		(1L<<22)	/* Virtual SG format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFTBL		(1L<<21)	/* sg table reference format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFSGD		(1L<<20)	/* sg item reference format */
#define SGD_VP			(1L<<19)	/* virtual and physical, not verified yet */
#define SGD_VWOXCTX		(1L<<18)	/* virtual without translation context */
#define SGD_PCTX		(1L<<17)	/* sgd_pctx_t, 64 bit only */
#else
#define SGD_X64			(0)
#define SGD_NEXT_TBL	(0)
#define SGD_VIRTUAL		(1L<<6)
#define SGD_VP			(1L<<5)
#define SGD_VWOXCTX		(0)
#define SGD_PCTX		(1L<<4)
#define SGD_WIDE		(1L<<3)
#define SGD_REFTBL		(1L<<2)	/* sg table reference format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFSGD		(1L<<1)	/* sg item reference format */
#define SGD_EOT			(1L<<0)	/* End of table */
#endif

typedef struct _sg_common_t
{
	MV_U32	dword1;
	MV_U32	dword2;
	MV_U32	dword3;
	MV_U32	flags;	/* SGD_xxx */
} sg_common_t;

/*---------------------------------------------------------------------------*/

#ifndef HAVE_HW_COMPLIANT_SG
typedef struct _sgd_t
{
	MV_U64	baseAddr;
	MV_U32	flags;
	MV_U32 size ;
} sgd_t;

typedef struct _sgd_v32_t
{
	MV_PVOID	vaddr;
	MV_PVOID	xctx;
	MV_U32	flags;
	MV_U32 size ;
} sgd_v32_t;

/* sgd_v_t defines 32/64 bit virtual sgd without translation context */
typedef struct _sgd_v_t
{
	union {
		MV_PVOID	vaddr;
		MV_U64		dummy;
	} u;

	MV_U32	flags;
	MV_U32 size ;
} sgd_v_t;

typedef struct _sgd_v64_t
{
	union {
		MV_PVOID	vaddr;
		MV_U64		dummy;
	} u1;

	MV_U32	flags;
	MV_U32 size ;

	union {
		MV_PVOID	xctx;
		MV_U64		dummy;
	} u2;
	MV_U32	flagsEx;
	MV_U32	rsvd;
} sgd_v64_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_ref32_t
{
	MV_PVOID	ref;
	MV_U32		offset;

	MV_U32	flags;
	MV_U32 size ;
} sgd_ref32_t;

typedef struct _sgd_ref64_t
{
	union {
		MV_PVOID	ref;
		MV_U64		dummy;
	} u;

	MV_U32	flags;
	MV_U32 size ;

	MV_U32	offset;
	MV_U32	rsvd1;

	MV_U32	flagsEx;
	MV_U32	rsvd2;
} sgd_ref64_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_nt_t
{
	union {
		struct _sgd_tbl_t*	next;
		MV_U64	dummy;
	} u;
	MV_U32	rsvd;
	MV_U32	flags;	/* SGD_xxx */
} sgd_nt_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_vp_t
{
	MV_U64	baseAddr;

	MV_U32	flags;
	MV_U32 size ;
	union {
		MV_PVOID vaddr;
		MV_U64   dummy;
	} u;

	MV_U32	flagsEx;	// SGD_X64
	MV_U32	rsvd;
} sgd_vp_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_pctx_t
{
	MV_U64	baseAddr;
	MV_U32	flags;
	MV_U32 size ;
	union {
		MV_PVOID xctx;
		MV_U64   dummy;
	} u;

	MV_U32	flagsEx;	// SGD_X64
	MV_U32	rsvd;
} sgd_pctx_t;

#else /* define HAVE_HW_COMPLIANT_SG */
#pragma pack(4)

typedef struct _sgd_t
{
	MV_U64  baseAddr;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif
} sgd_t;

typedef struct _sgd_v_t
{
	union {
		MV_PVOID	vaddr;
		MV_U64		dummy;
	} u1;

#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif

	union {
		MV_PVOID	xctx;
		MV_U64		dummy;
	} u2;
	
	MV_U32	rsvd;
} sgd_v_t;

/*---------------------------------------------------------------------------*/
typedef struct _sgd_ref_t
{
	union {
		MV_PVOID	ref;
		MV_U64		dummy;
	} u;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif
	
	MV_U32	offset;
	MV_U64  rsvd;
} sgd_ref_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_nt_t
{
	union {
		struct _sgd_tbl_t*	next;
		MV_U64	dummy;
	} u;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif
} sgd_nt_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_vp_t
{
	MV_U64	baseAddr;

#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif
	union {
		MV_PVOID vaddr;
		MV_U64   dummy;
	} u;

	MV_U32 rsvd;
} sgd_vp_t;

/*---------------------------------------------------------------------------*/

typedef struct _sgd_pctx_t
{
	MV_U64	baseAddr;
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32  flags:10;
	MV_U32  size:22;
#else
	MV_U32  size:22;
	MV_U32  flags:10;
#endif
	union {
		MV_PVOID xctx;
		MV_U64   dummy;
	} u;

	
	MV_U32  rsvd;
} sgd_pctx_t;

#pragma pack(0)
#endif

/*---------------------------------------------------------------------------*/

#define SGT_FLAG_PRDT_HW_COMPLIANT	(1U << 7)
#define SGT_FLAG_PRDT_IN_HOST		(1U << 6)
#define SGT_FLAG_SGL				(1U << 5)

typedef struct _sgd_tbl_t
{
	MV_U16 Max_Entry_Count;
	MV_U16 Valid_Entry_Count;
	MV_U8 Flag;
	MV_U16 Occupy_Entry_Count;
	MV_U32 Byte_Count;
	sgd_t* Entry_Ptr;
	MV_U64 prdt_bus_addr;
} sgd_tbl_t;

#define sgd_table_init(sgdt,maxCnt,entries) do {	\
	MV_ZeroMemory(sgdt,sizeof(sgd_tbl_t));		\
	(sgdt)->Max_Entry_Count = (maxCnt);				\
	(sgdt)->Entry_Ptr = (sgd_t*)(entries);			\
} while(0)

/*---------------------------------------------------------------------------*/

#ifndef HAVE_HW_COMPLIANT_SG
#define sgd_inc(sgd) do {	\
	if( (sgd)->flags & SGD_WIDE )					\
		sgd = (sgd_t*)(((unsigned char*) (sgd)) + 32);	\
	else sgd = (sgd_t*)(((unsigned char*) (sgd)) + 16);	\
} while(0)

#define sgd_get_vaddr(sgd,v) do {				\
		if( (sgd)->flags & SGD_VIRTUAL ) {			\
			if( (sgd)->flags & SGD_WIDE )			\
				(v) = ((sgd_v64_t*)(sgd))->u1.vaddr;\
			else (v) = ((sgd_v32_t*)(sgd))->vaddr;	\
		}											\
		else if( (sgd)->flags & SGD_VWOXCTX )		\
			(v) = ((sgd_v_t*)sgd)->u.vaddr; 		\
		else if( (sgd)->flags & SGD_VP )			\
			(v) = ((sgd_vp_t*)(sgd))->u.vaddr;		\
		else										\
			MV_ASSERT(MV_FALSE);					\
	} while(0)
	
#define sgd_get_xctx(sgd,v) do {	\
		if( (sgd)->flags & SGD_WIDE )	(v) = ((sgd_v64_t*)(sgd))->u2.xctx; \
		else (v) = ((sgd_v32_t*)(sgd))->xctx;	\
	} while(0)

#define sgd_get_ref(sgd,_ref) do {	\
		if( (sgd)->flags & SGD_WIDE ) (_ref) = ((sgd_ref64_t*)(sgd))->u.ref;	\
		else (_ref) = ((sgd_ref32_t*)(sgd))->ref;	\
	} while(0)
	
#define sgd_set_ref(sgd,_ref) do {	\
		if( (sgd)->flags & SGD_WIDE ) ((sgd_ref64_t*)(sgd))->u.ref = (_ref);	\
		else ((sgd_ref32_t*)(sgd))->ref = (_ref);	\
	} while(0)
	
#define sgd_get_reftbl(sgd,reft) do {	\
		if( (sgd)->flags & SGD_WIDE )		\
			(reft) = (sgd_tbl_t*) (((sgd_ref64_t*)(sgd))->u.ref);	\
		else (reft) = (sgd_tbl_t*)(((sgd_ref32_t*)(sgd))->ref); \
	} while(0)
	
#define sgd_get_refsgd(sgd,reft) do {	\
		if( (sgd)->flags & SGD_WIDE )		\
			(reft) = (sgd_t*) (((sgd_ref64_t*)(sgd))->u.ref);	\
		else (reft) = (sgd_t*)(((sgd_ref32_t*)(sgd))->ref); \
	} while(0)
	
#define sgd_get_refoff(sgd,off) do {	\
		if( (sgd)->flags & SGD_WIDE )	(off) = ((sgd_ref64_t*)(sgd))->offset;	\
		else (off) = ((sgd_ref32_t*)(sgd))->offset; \
	} while(0)
	
#define sgd_set_refoff(sgd,off) do {	\
		if( (sgd)->flags & SGD_WIDE )	((sgd_ref64_t*)(sgd))->offset = (off);	\
		else ((sgd_ref32_t*)(sgd))->offset = (off); \
	} while(0)

#define sgd_get_nexttbl(sgd,n) do {	\
		n = ((sgd_nt_t*)(sgd))->u.next; \
	} while(0)

#define sgd_copy(sgdDst,sgdSrc) do {	\
	*(sgdDst) = *(sgdSrc);	\
	if( (sgdSrc)->flags & SGD_WIDE )	\
		(sgdDst)[1] = (sgdSrc)[1];	\
} while(0)

#define sgdt_get_lastsgd(sgdt,sgd) do {		\
		(sgd) = &(sgdt)->Entry_Ptr[(sgdt)->Valid_Entry_Count];	\
		(sgd)--;								\
		if( (sgd)->flags & SGD_X64 ) (sgd)--;	\
	} while(0)

#else
#define sgd_inc(sgd) do {	\
	if( (sgd)->flags & (SGD_WIDE) ) \
		sgd = (sgd_t *)(((unsigned char*) (sgd)) + 2 * sizeof(sgd_t));	\
	else sgd = (sgd_t *)(((unsigned char*) (sgd)) + sizeof(sgd_t));	\
} while(0)

#define sgd_get_vaddr(sgd,v) do {				    \
		if( (sgd)->flags & SGD_VIRTUAL ) {			\
				(v) = ((sgd_v_t*)(sgd))->u1.vaddr;  \
		}											\
		else if( (sgd)->flags & SGD_VP )			\
			(v) = ((sgd_vp_t*)(sgd))->u.vaddr;		\
		else										\
			MV_ASSERT(MV_FALSE);					\
	} while(0)
	
#define sgd_get_xctx(sgd,v) do {	\
		(v) = ((sgd_v_t*)(sgd))->u2.xctx; \
	} while(0)

#define sgd_get_ref(sgd,_ref) do {	\
		 (_ref) = ((sgd_ref_t*)(sgd))->u.ref;	\
	} while(0)
	
#define sgd_set_ref(sgd,_ref) do {	\
		 ((sgd_ref_t*)(sgd))->u.ref = (_ref);	\
	} while(0)
	
#define sgd_get_reftbl(sgd,reft) do {	\
		 (reft) = (sgd_tbl_t*)(((sgd_ref_t*)(sgd))->u.ref); \
	} while(0)
	
#define sgd_get_refsgd(sgd,reft) do {	\
		(reft) = (sgd_t*)(((sgd_ref_t*)(sgd))->u.ref);	\
	} while(0)
	
#define sgd_get_refoff(sgd,off) do {	\
		(off) = ((sgd_ref_t*)(sgd))->offset;	\
	} while(0)
	
#define sgd_set_refoff(sgd,off) do {	\
		((sgd_ref_t*)(sgd))->offset = (off);	\
	} while(0)

#define sgd_copy(sgdDst,sgdSrc) do {	\
	*(sgdDst) = *(sgdSrc);	\
	if ((sgdSrc)->flags & (SGD_WIDE) ) \
		(sgdDst)[1] = (sgdSrc)[1];	\
} while(0)

#define sgdt_get_lastsgd(sgdt,sgd) do {\
		if (((sgdt)->Valid_Entry_Count) > 1) {\
			(sgd) = &(sgdt)->Entry_Ptr[((sgdt)->Valid_Entry_Count) - 2]; \
			if (!((sgd)->flags & (SGD_WIDE))) (sgd)++; \
		}\
		else { \
			(sgd) = &(sgdt)->Entry_Ptr[(sgdt)->Valid_Entry_Count];\
			(sgd)--;\
		}\
	} while(0)

#endif

#define sgd_mark_eot(sgd) \
	((sgd)->flags |= SGD_EOT)

#define sgd_clear_eot(sgd) \
	((sgd)->flags &= ~SGD_EOT)

#define sgd_eot(sgd)	\
	((sgd)->flags & SGD_EOT)

#define sgd_getsz(sgd,sz) do {				\
	(sz) = (sgd)->size;				\
} while(0)

#define sgd_setsz(sgd,sz) do {				\
	(sgd)->size = (sz);				\
} while(0)

/*---------------------------------------------------------------------------*/

typedef int (*sgd_visitor_t)(sgd_t* sgd, MV_PVOID pContext);

int sgd_table_walk(
	sgd_tbl_t*		sgdt,
	sgd_visitor_t	visitor,
	MV_PVOID		ctx
	);

/*---------------------------------------------------------------------------*/

typedef struct _sgd_iter_t
{
	sgd_t*	sgd;		/* current SG */
	MV_U32	offset;		/* offset in the SG */
	MV_U32	remainCnt;
} sgd_iter_t;

void  sgd_iter_init(
	sgd_iter_t*	iter,
	sgd_t*		sgd,
	MV_U32		offset,
	MV_U32		count
	);

int sgd_iter_get_next(
	sgd_iter_t*	iter,
	sgd_t*		sgd
	);

/*---------------------------------------------------------------------------*/

void sgd_dump(sgd_t* sg, char* prefix);
void sgdt_dump(sgd_tbl_t *SgTbl, char* prefix);

/*---------------------------------------------------------------------------*/

void sgdt_append(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size
	);

#ifndef HAVE_HW_COMPLIANT_SG
void sgdt_append_pctx(
	sgd_tbl_t*	sgdt,
	MV_U32		address,
	MV_U32		addressHigh,
	MV_U32		size,
	MV_PVOID	xctx
	);
#endif

int sgdt_append_virtual(
	sgd_tbl_t* sgdt,
	MV_PVOID virtual_address,
	MV_PVOID translation_ctx,
	MV_U32 size
	);

int sgdt_append_ref(
	sgd_tbl_t*	sgdt,
	MV_PVOID	ref,
	MV_U32		offset,
	MV_U32		size,
	MV_BOOLEAN	refTbl
	);

int sgdt_append_vp(
	sgd_tbl_t*	sgdt,
	MV_PVOID	virtual_address,
	MV_U32		size,
	MV_U32		address,
	MV_U32		addressHigh
	);

void
sgdt_copy_partial(
	sgd_tbl_t* sgdt,
	sgd_t**	ppsgd,
	MV_PU32	poff,
	MV_U32	size
	);

void sgdt_append_sgd(
	sgd_tbl_t*	sgdt,
	sgd_t*		sgd
	);

#define sgdt_append_reftbl(sgdt,refSgdt,offset,size)	\
	sgdt_append_ref(sgdt,refSgdt,offset,size,MV_TRUE)

#define sgdt_append_refsgd(sgdt,refSgd,offset,size)	\
	sgdt_append_ref(sgdt,refSgd,offset,size,MV_FALSE)

/*---------------------------------------------------------------------------*/
typedef MV_VOID (*sgd_to_prd_fn)(MV_PVOID prd_ctx, MV_U64 base_addr, MV_U32 size);
int sgdt_prepare_hwprd(
	MV_PVOID		pCore,
	sgd_tbl_t*		pSource,
	MV_PVOID		prd_ctx,
	sgd_to_prd_fn		prd_fn
	);
#endif	/*__MV_COM_SGD_H__*/
