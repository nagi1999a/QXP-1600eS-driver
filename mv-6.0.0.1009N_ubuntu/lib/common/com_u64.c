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
#include "com_u64.h"

#if U64_LIB_TYPE==1
MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value *= v32;
#else
	v64.parts.low *= v32;
	v64.parts.high = 0;	//TBD
#endif
	return v64;
}
MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	if(v32)
		return ossw_u64_mod(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		return	0;	
	}
#else
	return (MV_U32) (v64.value % v32);
#endif /* _OS_LINUX */
}

MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	if(v32)
		v64.value = ossw_u64_div(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		v64.value = 0;
	}
#else
#ifdef _64_BIT_COMPILER
	v64.value /= v32;
#else
	v64.parts.high = 0;	//TBD
	v64.parts.low /= v32;
#endif /* _64_BIT_COMPILER */

#endif /* _OS_LINUX */
	return v64;
}
MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _OS_LINUX
	MV_U32 ret = 0;
	while (v1.value > v2.value) {
		v1.value -= v2.value;
		ret++;
	}
	return ret;
#else
#ifdef _64_BIT_COMPILER
	v1.value /= v2.value;
#else
	v1.parts.high = 0;	//TBD
	v1.parts.low /= v2.parts.low;
#endif
	return v1.parts.low;
#endif
}
#elif U64_LIB_TYPE==2
MV_U64 U64_ADD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value += v32;
#else
	v64.parts.low += v32;
	v64.parts.high = 0;	//TBD
#endif
	return v64;
}

MV_U64 U64_SUBTRACT_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value -= v32;
#else
	v64.parts.low -= v32;
	v64.parts.high = 0;	//TBD
#endif
	return v64;
}

MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value *= v32;
#else
	v64.parts.low *= v32;
	v64.parts.high = 0;	//TBD
#endif
	return v64;
}

MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	if(v32)
		return ossw_u64_mod(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		return	0;	
	}
#else
	return (MV_U32) (v64.value % v32);
#endif /* _OS_LINUX */
}

MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _OS_LINUX
	if(v32)
		v64.value = ossw_u64_div(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		v64.value = 0;
	}
#else
#ifdef _64_BIT_COMPILER
	v64.value /= v32;
#else
	v64.parts.high = 0;	//TBD
	v64.parts.low /= v32;
#endif /* _64_BIT_COMPILER */

#endif /* _OS_LINUX */
	return v64;
}

MV_I32 U64_COMPARE_U32(MV_U64 v64, MV_U32 v32)
{
	if (v64.parts.high > 0)
		return 1;
	if (v64.parts.low > v32)
		return 1;
#ifdef _64_BIT_COMPILER
	else if (v64.value == v32)
#else
	else if (v64.parts.low == v32)
#endif
		return 0;
	else
		return -1;
}

MV_U64 U64_ADD_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value += v2.value;
#else
	v1.parts.low += v2.parts.low;
	v1.parts.high = 0;	//TBD
	//v1.parts.high += v2.parts.high;
#endif
	return v1;
}

MV_U64 U64_SUBTRACT_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value -= v2.value;
#else
	v1.parts.low -= v2.parts.low;
	v1.parts.high = 0;	//TBD
	//v1.parts.high -= v2.parts.high;
#endif
	return v1;
}

MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _OS_LINUX
	MV_U32 ret = 0;
	while (v1.value > v2.value) {
		v1.value -= v2.value;
		ret++;
	}
	return ret;
#else
#ifdef _64_BIT_COMPILER
	v1.value /= v2.value;
#else
	v1.parts.high = 0;	//TBD
	v1.parts.low /= v2.parts.low;
#endif
	return v1.parts.low;
#endif
}

MV_I32 U64_COMPARE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;
#else
#if 0
	if (v1.parts.high > v2.parts.high)
		return 1;
	else if((v1.parts.low > v2.parts.low) && (v1.parts.high == v2.parts.high))
		return 1;
		
	else if ((v1.parts.low == v2.parts.low) && (v1.parts.high == v2.parts.high))
		return 0;
	else
		return -1;
#endif
//_MARVELL_SDK_PACKAGE_NONRAID
	//TBD
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;

#endif

}

#ifdef _OS_BIOS
MV_U64 ZeroU64(MV_U64 v1)
{
	v1.parts.low = 0;
	v1.parts.high = 0;

	return	v1;
}
#endif /*  _OS_BIOS */
#endif

