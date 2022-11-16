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

#ifndef _U64_H__
#define _U64_H__
#define U64_ASSIGN_SAFE(v1, v2)	\
do{	int i; \
	for (i=0; i<sizeof((v1)); i++)	\
		((char*)&v1)[i] = ((char*)(v2))[i];	\
}while(0)
#if defined (MV_ROC_IOP_TUNED) ||defined(_OS_UKRN)
#define U64_LIB_TYPE		0
#else
#define U64_LIB_TYPE		2
#endif
#if U64_LIB_TYPE==0||U64_LIB_TYPE==1
#define U64_ADD_U32(v1, v2)											\
({                                                                      \
	register MV_U64 a;              					\
	a.value=(v1).value+(v2); a;                                          \
})                                                                      
#define U64_SUBTRACT_U32(v1, v2)                        \
({                                                                      \
	register MV_U64 a;              					\
	a.value=(v1).value-(v2); a;                                                      \
})                                                                      
#define U64_ADD_U64(v1, v2)                                \
({                                                                      \
	register MV_U64 a;              					\
	a.value=(v1).value+(v2).value; a;                                                      \
})                                                                      
#define U64_SUBTRACT_U64(v1, v2) 	\
({	\
	register MV_U64 a;              					\
	a.value=(v1).value-(v2).value; a;                                                      \
})
#define U64_COMPARE_U64_(v1, v2) 	\
({	\
	register int r;	\
	if ((v1)==(v2)) r = 0; else if((v1)>(v2)) r = 1; else r = -1; r;                                                   \
})

#define U64_COMPARE_U64(v1, v2)     U64_COMPARE_U64_((v1).value, (v2).value)
#define U64_COMPARE_U32(v1, v2)		U64_COMPARE_U64_((v1).value, (v2))
#define U64_SET_VALUE(v64, v32)		(v64).value = (_MV_U64)(v32)
#define U64_SET_MAX_VALUE(v64)		(v64).value = 0xFFFFFFFFFFFFFFFFLL
#endif
#if U64_LIB_TYPE==0
#define U64_MULTIPLY_U32(v1, v2)                         \
({                                                                      \
	register MV_U64 a;              					\
	a.value=(v1).value*(v2); a;                                                      \
})                                                                      
#define U64_DIVIDE_U64(v1, v2)                             				\
({                                                                      \
	register MV_U32 a;              					\
	a=(v1).value/(v2).value; a;                                                      \
})                                                                      
#define U64_MOD_U32(v1, v2)		\
({                                                                      \
	register MV_U32 a;              					\
	a=(v1).value%(v2); a;                                                      \
})                                                                      
#define U64_DIVIDE_U32(v1, v2)                             				\
({                                                                      \
	register MV_U64 a;              					\
	a.value=(v1).value/(v2); a;                                                      \
})                                                                      
#elif U64_LIB_TYPE==1
MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32);
MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32);
MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2);
#elif U64_LIB_TYPE==2
MV_U64 U64_ADD_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_SUBTRACT_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32);
MV_I32 U64_COMPARE_U32(MV_U64 v64, MV_U32 v32);
MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32);

MV_U64 U64_ADD_U64(MV_U64 v1, MV_U64 v2);
MV_U64 U64_SUBTRACT_U64(MV_U64 v1, MV_U64 v2);
MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2);
MV_I32 U64_COMPARE_U64(MV_U64 v1, MV_U64 v2);

#ifndef _OS_BIOS
#define U64_SET_VALUE(v64, v32)	do { v64.value = v32; } while(0)
#else
#define U64_SET_VALUE(v64, v32)	do {v64.low = v32;v64.high=0;} while(0)
MV_U64 U64_SHIFT_LEFTN(MV_U64 v64, MV_U8 num);
MV_U64 U64_SHIFT_RIGHTN(MV_U64 v64, MV_U8 num);
#endif

#define U64_SET_MAX_VALUE(v64)	do { v64.parts.low = v64.parts.high = 0xFFFFFFFFL; } while(0);

#endif

#endif

