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

#include "mv_include.h" // MFC requires a common header, please dont checkin

#ifdef SOFTWARE_XOR

#include "core_type.h"
#include "core_hal.h"
#include "core_manager.h"
#include "core_resource.h"
#include "core_protocol.h"
#include "core_util.h"
#include "com_util.h"
#include "math_gf.h"
#ifdef RAID6_MULTIPLE_PARITY
#define XOR_AS_GF
#endif
#if defined(XOR_AS_GF)
typedef MV_U8 XORUNIT;
#else
typedef MV_U32 XORUNIT;
#endif

/*******************************************************************************
*                                                                              *
* SOFTWARE XOR Utilities                                                       *
*                                                                              *
*******************************************************************************/
XOR_COEF xor_get_coef(
	PMV_XOR_Request xor_req,
	MV_U32 source,
	MV_U32 target)
{
	//PXOR_COEF coef_ptr = &xor_req->Coef[0][0];
	//return (coef_ptr[source + target * xor_req->Target_SG_Table_Count]);
	return xor_req->Coef[target][source];
}
MV_U32 min_of(MV_U32* ele, MV_U32 cnt)
{
        MV_U32 v = *ele++;

        while (--cnt) {
                if( *ele < v )
                        v = *ele;
                ele++;
        }
        return v;
}

/*******************************************************************************
*                                                                              *
* SOFTWARE XOR Core Logic                                                      *
*                                                                              *
*******************************************************************************/

/*
 * swxor_cmp_sg needs not to be changed for SGD_PCTX since RAID always
 * use memory with known virtual address.
 */
void
swxor_cmp_sg(PMV_XOR_Request xor_req, PMV_SG_Table srctbl)
{
        MV_PU32         pSrc[2] = { NULL, };
        sgd_t           sgd[2];
        sgd_iter_t      sg_iter[2];
        MV_U32          wCount[2];
        MV_U8           bFinished = MV_FALSE;
        MV_U8           bIndex;
        MV_U32          offset = 0;
        MV_PVOID        p = NULL;

        MV_ASSERT( srctbl[0].Byte_Count == srctbl[1].Byte_Count );

        for (bIndex = 0; bIndex < 2; bIndex++) {
                sgd_iter_init(
                        &sg_iter[bIndex],
                        srctbl[bIndex].Entry_Ptr,
                        0,
                        srctbl[bIndex].Byte_Count);

                sgd_iter_get_next(&sg_iter[bIndex], sgd);
                sgd_get_vaddr(sgd, p);
                pSrc[bIndex] = (MV_PU32) p;
                sgd_getsz(sgd, wCount[bIndex]);

        }

        while (!bFinished) {
                if (*pSrc[0] != *pSrc[1]) {
                        xor_req->Request_Status = XOR_STATUS_ERROR;
                        xor_req->Error_Offset = offset;
                        return;
                }

                offset += sizeof(MV_U32);

                for (bIndex = 0; bIndex < 2; bIndex++) {
                        pSrc[bIndex]++;
                        wCount[bIndex] -= sizeof(MV_U32);
                        if (wCount[bIndex] == 0) {
                                if (!sgd_iter_get_next(&sg_iter[bIndex], sgd)) {
                                        bFinished = MV_TRUE;
                                        break;
                                }
                                sgd_get_vaddr(sgd, p);
                                pSrc[bIndex] = (MV_PU32) p;
                                sgd_getsz(sgd, wCount[bIndex]);
                        }
                }
        }
}

#if defined(XOR_AS_GF) 
XORUNIT *swxor_xor_get_one_unit(xor_strm_t *strm,
        int i,
        MV_U32 off,
        MV_BOOLEAN *p_mapped,
        MV_PVOID *pp_tmp)
{
	XORUNIT         *p;
	MV_BOOLEAN      mapped = MV_FALSE;
	MV_PVOID        ptmp;

        if (strm[i].sgd[0].flags & SGD_PCTX) {
                p = (XORUNIT *)sgd_kmap(strm[i].sgd);
                ptmp = p;
                mapped = MV_TRUE;
#ifdef _OS_LINUX
                p = (XORUNIT *)(((MV_PU8)p) +
                                strm[i].sgd[1].size);
#endif
	} else {
                ptmp = NULL;
                sgd_get_vaddr(strm[i].sgd, ptmp);
                p = (XORUNIT *)ptmp;
	}

	p = (XORUNIT *)(((MV_PU8)p) + strm[i].off + off);

	*p_mapped = mapped;
	*pp_tmp = ptmp;
	return p;
}

void swxor_xor(xor_strm_t *strm,
        MV_U8 src_cnt,
        MV_U8 dst_cnt,
        MV_U32 byte_cnt,
        PMV_XOR_Request xor_req)
{
	XORUNIT         *p, *t_p;
	MV_PVOID        ptmp, t_ptmp;
	MV_BOOLEAN      mapped, t_mapped;
	/* If source, target share the same buffer, so we have to use buffer. */
	XORUNIT	        tmp[XOR_TARGET_SG_COUNT];

        MV_U32	        off = 0;
        MV_U32          i, j;
        XORUNIT         value = 0;

        while (byte_cnt) {
                for (i = 0; i < src_cnt; i++) {
                        p = swxor_xor_get_one_unit(strm, i, off,
                                &mapped, &ptmp);

                        for (j = 0; j < dst_cnt; j++) {
	                        t_p = swxor_xor_get_one_unit(strm, src_cnt+j,
                                        off, &t_mapped, &t_ptmp);

	                        if (i == 0)
		                        tmp[j]= GF_Multiply(
                                                        xor_get_coef(
                                                                xor_req, i, j),
                                                                *p);
	                        else
		                        tmp[j]= GF_Add(tmp[j],
                                                GF_Multiply(xor_get_coef(
                                                                xor_req, i, j),
                                                                *p));

	                        if (i == src_cnt-1)
		                        *t_p = tmp[j];

	                        if (t_mapped)
		                        sgd_kunmap(strm[i+j].sgd, t_ptmp);
                        }

                        if (mapped)
	                        sgd_kunmap(strm[i].sgd, ptmp);
                }

                byte_cnt -= sizeof(XORUNIT);
                off += sizeof(XORUNIT);
        }
}

#else /* defined(XOR_AS_GF) || defined(_OS_LINUX) */

void optimize_sg_xor(xor_strm_t *strm,
        MV_U8 src_cnt,
        MV_U8 dst_cnt,
        MV_U32 byte_cnt,
        PMV_XOR_Request xor_req) 
{
        XORUNIT         *p[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
        MV_PVOID        ptmp[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
        int             i;
        MV_U32          off = 0;
        MV_BOOLEAN      mapped[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
	unsigned long flags = 0;
	MV_U32 extra_count;

    MV_ZeroMemory(p, sizeof(XORUNIT *) * (XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT));
    MV_ZeroMemory(mapped, sizeof(MV_BOOLEAN) * (XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT));
    MV_ZeroMemory(ptmp, sizeof(MV_PVOID) * (XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT));

	hba_local_irq_save(flags);

	for (i = 0; i < src_cnt + dst_cnt; i++) {
		mapped[i] = MV_FALSE;
		if (strm[i].sgd[0].flags & SGD_PCTX) {
			p[i] = (XORUNIT *)sgd_kmap(strm[i].sgd);
			ptmp[i] = p[i];
#ifdef _OS_LINUX
			p[i] = (XORUNIT *)(((MV_PU8)p[i]) + 
			              strm[i].sgd[1].size);
#endif
			mapped[i] = MV_TRUE;
		} else {
			sgd_get_vaddr(strm[i].sgd, ptmp[i]);
			p[i] = (XORUNIT*)ptmp[i];
		}
	}
	
#define DO(x,i) (*(XORUNIT *)(((MV_PU8)x) + strm[i].off + off))
#define DO1(x,i) (*(XORUNIT *)(((MV_PU8)x) + strm[i].off + off +\
                        1 * sizeof(XORUNIT)))
#define DO2(x,i) (*(XORUNIT *)(((MV_PU8)x) + strm[i].off + off +\
                        2 * sizeof(XORUNIT)))
#define DO3(x,i) (*(XORUNIT *)(((MV_PU8)x) + strm[i].off + off +\
                        3 * sizeof(XORUNIT)))

	/*1*/
	extra_count = byte_cnt & ((sizeof(XORUNIT) * 4) - 1 );

	if (src_cnt == 2) {
		
		while ( extra_count ) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1], 1);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );

		}
				
		while (byte_cnt) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1], 1);
			DO1(p[src_cnt], src_cnt) = DO1(p[0], 0) ^ DO1(p[1], 1);
			DO2(p[src_cnt], src_cnt) = DO2(p[0], 0) ^ DO2(p[1], 1);
			DO3(p[src_cnt], src_cnt) = DO3(p[0], 0) ^ DO3(p[1], 1);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
				
		}	
	} else if (src_cnt == 3) {

		while ( extra_count ) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1], 1)
                                ^ DO(p[2], 2);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );

		}
		
		
		while (byte_cnt) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1], 1)
                                ^ DO(p[2], 2);
			DO1(p[src_cnt], src_cnt) = DO1(p[0], 0) ^ DO1(p[1], 1)
                                ^ DO1(p[2], 2);
			DO2(p[src_cnt], src_cnt) = DO2(p[0], 0) ^ DO2(p[1], 1)
                                ^ DO2(p[2], 2);
			DO3(p[src_cnt], src_cnt) = DO3(p[0], 0) ^ DO3(p[1], 1)
                                ^ DO3(p[2], 2);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}	
	} else if (src_cnt == 4) {

		while ( extra_count ) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1],1) 
                                ^ DO(p[2], 2) ^ DO(p[3], 3);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}
		
	
		while (byte_cnt) {
			DO(p[src_cnt], src_cnt) = DO(p[0], 0) ^ DO(p[1],1) 
                                ^ DO(p[2], 2) ^ DO(p[3], 3);
			DO1(p[src_cnt], src_cnt) = DO1(p[0], 0) ^ DO1(p[1], 1) 
                                ^ DO1(p[2], 2) ^ DO1(p[3], 3);
			DO2(p[src_cnt], src_cnt) = DO2(p[0], 0) ^ DO2(p[1], 1) 
                                ^ DO2(p[2], 2) ^ DO2(p[3], 3);
			DO3(p[src_cnt], src_cnt) = DO3(p[0], 0) ^ DO3(p[1], 1) 
                                ^ DO3(p[2], 2) ^ DO3(p[3], 3);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		
		}	
	} else if ( src_cnt == 5) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt)  = DO(p[0],0) ^ DO(p[1],1) 
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}
	

	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt)  = DO(p[0],0) ^ DO(p[1],1) 
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4);
			DO1(p[src_cnt],src_cnt)  = DO1(p[0],0) ^ DO1(p[1],1) 
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4);
			DO2(p[src_cnt],src_cnt)  = DO2(p[0],0) ^ DO2(p[1],1) 
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4);
			DO3(p[src_cnt],src_cnt)  = DO3(p[0],0) ^ DO3(p[1],1) 
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
			
	} else if ( src_cnt == 6) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1) 
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4) 
                                ^ DO(p[5],5);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );

		}
	

	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1) 
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4) 
                                ^ DO(p[5],5);
			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1) 
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4) 
                                ^ DO1(p[5],5);
			DO2(p[src_cnt],src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
                                ^ DO2(p[5],5);
			DO3(p[src_cnt],src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
                                ^ DO3(p[5],5);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else if ( src_cnt == 7) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}
		

	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6);
			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1)
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4)
                                ^ DO1(p[5],5) ^ DO1(p[6],6);
			DO2(p[src_cnt],src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
                                ^ DO2(p[5],5) ^ DO2(p[6],6);
			DO3(p[src_cnt],src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
                                ^ DO3(p[5],5) ^ DO3(p[6],6);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else if ( src_cnt == 8) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}
		

	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7);

			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1)
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4)
                                ^ DO1(p[5],5) ^ DO1(p[6],6) ^ DO1(p[7],7);

			DO2(p[src_cnt],src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
                                ^ DO2(p[5],5) ^ DO2(p[6],6) ^ DO2(p[7],7);

			DO3(p[src_cnt],src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
                                ^ DO3(p[5],5) ^ DO3(p[6],6) ^ DO3(p[7],7);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else if ( src_cnt ==9) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7)
                                ^ DO(p[8],8);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}



	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7)
                                ^ DO(p[8],8);

			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1)
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4)
                                ^ DO1(p[5],5) ^ DO1(p[6],6) ^ DO1(p[7],7)
                                ^ DO1(p[8],8);
			
			DO2(p[src_cnt],src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
                                ^ DO2(p[5],5) ^ DO2(p[6],6) ^ DO2(p[7],7)
                                ^ DO2(p[8],8);

			DO3(p[src_cnt],src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
                                ^ DO3(p[5],5) ^ DO3(p[6],6) ^ DO3(p[7],7)
                                ^ DO3(p[8],8);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else if ( src_cnt == 10) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7)
                                ^ DO(p[8],8) ^ DO(p[9],9);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}
	


	
		while (byte_cnt) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7)
                                ^ DO(p[8],8) ^ DO(p[9],9);

			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1)
                                ^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4)
                                ^ DO1(p[5],5) ^ DO1(p[6],6) ^ DO1(p[7],7)
                                ^ DO1(p[8],8) ^ DO1(p[9],9);
			
			DO2(p[src_cnt],src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
                                ^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
                                ^ DO2(p[5],5) ^ DO2(p[6],6) ^ DO2(p[7],7)
                                ^ DO2(p[8],8) ^ DO2(p[9],9);

			DO3(p[src_cnt],src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
                                ^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
                                ^ DO3(p[5],5) ^ DO3(p[6],6) ^ DO3(p[7],7)
                                ^ DO3(p[8],8) ^ DO3(p[9],9);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else if ( src_cnt == 11) {

		while ( extra_count ) {
			DO(p[src_cnt],src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^DO(p[7],7)
                                ^ DO(p[8],8) ^ DO(p[9],9) ^ DO(p[10],10);
			extra_count -= (sizeof(XORUNIT) );
			off += (sizeof(XORUNIT) );
			byte_cnt -= (sizeof(XORUNIT) );
		}

	
		while (byte_cnt) {
			DO(p[src_cnt], src_cnt) = DO(p[0],0) ^ DO(p[1],1)
                                ^ DO(p[2],2) ^ DO(p[3],3) ^ DO(p[4],4)
                                ^ DO(p[5],5) ^ DO(p[6],6) ^ DO(p[7],7)
                                ^ DO(p[8],8) ^ DO(p[9],9) ^ DO(p[10],10);

			DO1(p[src_cnt],src_cnt) = DO1(p[0],0) ^ DO1(p[1],1)
								^ DO1(p[2],2) ^ DO1(p[3],3) ^ DO1(p[4],4)
								^ DO1(p[5],5) ^ DO1(p[6],6) ^ DO1(p[7],7)
								^ DO1(p[8],8) ^ DO1(p[9],9) ^ DO1(p[10],10);
			
			DO2(p[src_cnt], src_cnt) = DO2(p[0],0) ^ DO2(p[1],1)
								^ DO2(p[2],2) ^ DO2(p[3],3) ^ DO2(p[4],4)
								^ DO2(p[5],5) ^ DO2(p[6],6) ^ DO2(p[7],7)
								^ DO2(p[8],8) ^ DO2(p[9],9) ^ DO2(p[10],10);

			DO3(p[src_cnt], src_cnt) = DO3(p[0],0) ^ DO3(p[1],1)
								^ DO3(p[2],2) ^ DO3(p[3],3) ^ DO3(p[4],4)
								^ DO3(p[5],5) ^ DO3(p[6],6) ^ DO3(p[7],7)
								^ DO3(p[8],8) ^ DO3(p[9],9) ^ DO3(p[10],10);
			byte_cnt -= (sizeof(XORUNIT) * 4);
			off += (sizeof(XORUNIT) * 4);
		}
	} else {
			MV_ASSERT(0);
	}
		

#undef  DO
#undef  DO1
#undef  DO2
#undef  DO3
	for (i = 0; i < src_cnt + dst_cnt; i++)
		if (mapped[i])
			sgd_kunmap(strm[i].sgd, ptmp[i]);
	hba_local_irq_restore(flags);
	
}

void swxor_xor(xor_strm_t *strm,
        MV_U8 src_cnt,
        MV_U8 dst_cnt,
        MV_U32 byte_cnt,
        PMV_XOR_Request xor_req)

{
	//XORUNIT	*pSrc, *pDst;
	XORUNIT		*p;
	MV_PVOID 	ptmp;
	int		i;
	XORUNIT	value = 0;
	MV_BOOLEAN	mapped;
	MV_U32	off = 0;
	unsigned long flags = 0;

	while( byte_cnt )
	{
		for( i = 0; i < src_cnt+dst_cnt; i++ )
		{
			mapped = MV_FALSE;
			if( strm[i].sgd[0].flags & SGD_PCTX )
			{
				hba_local_irq_save(flags);
				p = (XORUNIT*) sgd_kmap(strm[i].sgd);
				ptmp = p;
				mapped = MV_TRUE;
	#ifdef _OS_LINUX
				p = (XORUNIT*)(((MV_PU8)p) + 
				              strm[i].sgd[1].size);
	#endif
			}
			else
			{
				ptmp = NULL;
				sgd_get_vaddr( strm[i].sgd, ptmp );
				p = (XORUNIT*) ptmp;
			}

			p = (XORUNIT*) (((MV_PU8) p) + strm[i].off + off);

			if( i == 0 )
				value = *p;
			else if( i >= src_cnt )
				*p = value;
			else
				value ^= *p;

			if( mapped ){
				sgd_kunmap(strm[i].sgd, ptmp );
			 	hba_local_irq_restore(flags);	
			}
		}
		byte_cnt -= sizeof(XORUNIT);
		off += sizeof(XORUNIT);
	}
}
#endif /* defined(XOR_AS_GF) */

void
swxor_xor_sg(
        MV_U8           srcCount,
        MV_U8           dstCount,
        PMV_SG_Table    srctbl,
        PMV_SG_Table    dsttbl,
        PMV_XOR_Request xor_req)
{
        xor_strm_t      strm[XOR_SOURCE_SG_COUNT+ XOR_TARGET_SG_COUNT];
        sgd_iter_t      sg_iter[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
        MV_U32          wCount[XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT];
        MV_U8           bIndex;
        MV_U8           bFinished = MV_FALSE;
        MV_U32          tmp = srctbl[0].Byte_Count;
        MV_U32          count = 0xffffffffL;

        MV_ZeroMemory(wCount, sizeof(MV_U32) * (XOR_SOURCE_SG_COUNT + XOR_TARGET_SG_COUNT));
        for (bIndex = 0; bIndex < srcCount; bIndex++) {
                MV_ASSERT( srctbl[bIndex].Byte_Count == tmp );
                sgd_iter_init(
                        &sg_iter[bIndex],
                        srctbl[bIndex].Entry_Ptr,
                        0,
                        srctbl[bIndex].Byte_Count);
        }

        for (bIndex = 0; bIndex < dstCount; bIndex++) {
                MV_ASSERT( dsttbl[bIndex].Byte_Count == tmp );
                sgd_iter_init(
                        &sg_iter[srcCount+bIndex],
                        dsttbl[bIndex].Entry_Ptr,
                        0,
                        dsttbl[bIndex].Byte_Count);
        }

        for (bIndex = 0; bIndex < srcCount + dstCount; bIndex++) {
                strm[bIndex].off = 0;
                sgd_iter_get_next(&sg_iter[bIndex], strm[bIndex].sgd);
                sgd_getsz(strm[bIndex].sgd, wCount[bIndex]);

                if(wCount[bIndex] < count)
                        count = wCount[bIndex];
        }

        while (!bFinished) {
#if defined(XOR_AS_GF) || defined(_OS_LINUX)
                swxor_xor(strm, srcCount, dstCount, count, xor_req);
#else
		optimize_sg_xor(strm, srcCount, dstCount, count, xor_req);
#endif
                for(bIndex = 0; bIndex < srcCount + dstCount; bIndex++) {
                        wCount[bIndex] -= count;
                        if(wCount[bIndex] == 0) {
                                if(!sgd_iter_get_next(
                                        &sg_iter[bIndex],
                                        strm[bIndex].sgd)) {

                                        bFinished = MV_TRUE;
                                        break;
                                }

                                strm[bIndex].off = 0;
                                sgd_getsz(strm[bIndex].sgd, wCount[bIndex]);

                        } else {

                                strm[bIndex].off += count;
                        }
                }
                count = min_of(wCount, srcCount + dstCount);
        }

}

void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request xor_req)

{
        core_extension * core = (core_extension *)This;

        switch (xor_req->Request_Type)
        {
                case XOR_REQUEST_WRITE:
                        swxor_xor_sg(
                                xor_req->Source_SG_Table_Count,
                                xor_req->Target_SG_Table_Count,
                                xor_req->Source_SG_Table_List,
                                xor_req->Target_SG_Table_List,
                                xor_req);
                        break;
                case XOR_REQUEST_COMPARE:
                        MV_ASSERT(xor_req->Source_SG_Table_Count == 2);
                        swxor_cmp_sg(
                                xor_req,
                                xor_req->Source_SG_Table_List);
                        break;
                case XOR_REQUEST_DMA:
                        swxor_cpy_sg(
                                xor_req->Source_SG_Table_List,
                                xor_req->Target_SG_Table_List);
                        break;
                default:
                        xor_req->Request_Status = XOR_STATUS_INVALID_REQUEST;
                        break;
        }

        xor_req->Completion(xor_req->Cmd_Initiator, xor_req);
}

#endif /* SOFTWARE_XOR */

