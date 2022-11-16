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

#if !defined( _MVF_UTILS_H_ )
#define _MVF_UTILS_H_

#include "mvf_type.h"
#include "mvstdio.h"

#define MV_OFFSET_OF(t, m)        ((MV_PTR_INTEGER)&(((t *) 0)->m))

#define SWAP_U32(x) \
	((MV_U32)( \
		(((MV_U32)(x) & (MV_U32)0x000000ffUL) << 24) | \
		(((MV_U32)(x) & (MV_U32)0x0000ff00UL) <<  8) | \
		(((MV_U32)(x) & (MV_U32)0x00ff0000UL) >>  8) | \
		(((MV_U32)(x) & (MV_U32)0xff000000UL) >> 24) ))
#define SWAP_U16(x) \
	((MV_U16)( \
		(((MV_U16)(x) & (MV_U16)0x00ffU) << 8) | \
		(((MV_U16)(x) & (MV_U16)0xff00U) >> 8) ))
#define SWAP_U64(x) \
	((MV_U64)( \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x00000000000000ffULL) << 56) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x000000000000ff00ULL) << 40) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x0000000000ff0000ULL) << 24) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x00000000ff000000ULL) <<  8) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x000000ff00000000ULL) >>  8) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x0000ff0000000000ULL) >> 24) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0x00ff000000000000ULL) >> 40) | \
		(MV_U64)(((MV_U64)(x) & (MV_U64)0xff00000000000000ULL) >> 56) ))
		
#define     _SWAP_UXX(a)	SWAP_UXX(&(a),sizeof(a))
#if !defined( __GNUC__ )
#define     ntohl(a)	_SWAP_UXX(a)
#define     htonl(a)	_SWAP_UXX(a)
#define     ntohs(a)	_SWAP_UXX(a)
#define     htons(a)	_SWAP_UXX(a)
#endif
#if 1
#undef      ntohl
#undef      htonl
#undef      ntohs
#undef      htons
#define     ntohl(a)	    _SWAP_UXX(a)
#define     htonl(a)	    _SWAP_UXX(a)
#define     ntohs(a)	    _SWAP_UXX(a)
#define     htons(a)	    _SWAP_UXX(a)
#define     ntohlaa(a)	    ntohl(a)
#define     htonlaa(a)	    htonl(a)
#define     ntohsaa(a)	    ntohs(a)
#define     htonsaa(a)	    htons(a)
#else
#define     ntohlaa(a)	    (a) = ntohl(a)
#define     htonlaa(a)	    (a) = htonl(a)
#define     ntohsaa(a)	    (a) = ntohs(a)
#define     htonsaa(a)	    (a) = htons(a)
#endif
//_MARVELL_SDK_PACKAGE_NONRAID
#define     MVF_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define     MVF_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define     MVF_MIN3(a,b,c) (MIN((a), MIN(b, c)))
#ifndef MAX
#define     MAX     MVF_MAX
#define     MIN     MVF_MIN
#define     MIN3    MVF_MIN3
#endif

#define     MAX_CRC_SIZE(t)                 MAX_IO_SIZE(t)
#if defined( __U_BOOT__ )
#define     CRC32( a, b, c )                    crc32( (MV_U32)a, (const MV_U8*)b, (unsigned  int)c )
#else
#define     CRC32( a, b, c )                    mv_crc32( (MV_U32)a, (const MV_U8*)b, (unsigned  int)c )
#endif
#if !defined( MV_CopyMemory )
#define     MV_CopyMemory( a, b, c )            memcpy((char*)(a), (char*)(b), c)
#define     MV_ZeroMemory( a, b )               memset( (char*)(a), 0, b )
#define     MV_FillMemory( a, b, c )            memset( (char*)(a), b, c )
#define     MV_CompareMemory( a, b, c )         memcmp( (char*)(a), (char*)(b), c )
#endif
#if !defined (_WIN32)
#define     mvf_fprintf( a, ... )                MVS_PRINTF(  __VA_ARGS__ )
#define     mvf_printf( ... )                    MVS_PRINTF(  __VA_ARGS__ )
#else
#define     mvf_fprintf                          fprintf
#define     mvf_printf                           printf
#endif

#ifndef MV_BIT
#define     MV_BIT(x)			                (1L << (x))
#endif
#define     ALIGN_ANY(value, size)	            \
((MV_U32)(value)-((MV_U32)(value)%(MV_U32)(size)))
#ifndef MVF_ROUND
#define     MVF_ALIGN(value, size)	                (((MV_U32)(value)) & (~((MV_U32)(size)-1)))
#define     MVF_ROUND(value, size)	                MVF_ALIGN(((MV_U32)(value))+((size)-1), size)
#endif
#ifndef MV_IS_NOT_ALIGN
#define     MV_IS_NOT_ALIGN(number, size)      ((number) & ((size) - 1))
#define     MV_ALIGN_UP(number, size)          (((number) + (size-1)) & ~((size)-1))
#define     MV_ALIGN_DOWN(number, size)        ((number) & ~((size)-1))
#endif

#define     SWAP_EX(a, b, t)                    do{ t tmp; tmp = (t)(a); a = (t)b; b = (t)tmp; }while(0)

#define     AZFLAG_GET( a, c )                  (a&mv_azflag(0, 'i', c ))
#define     AZFLAG_SET( a, c )                  do{ a|=mv_azflag(a, 'c', c ); }while(0)
#define     AZFLAG_GET_E( a, A, c )             (AZFLAG_GET( a, c )|AZFLAG_GET( A, c ))
#define     AZFLAG_SET_E( a, A, c )             do{ a|=mv_azflag(0, 'c', c );A|=mv_azflag(0, 'C', c ); }while(0)
#define     AZBIT( c )                          mv_azflags( 0, 'I', c )
#define     AZFLAG_GET_4( a, c )                (a&mv_azflags(0, 'I', c))
#define     AZFLAG_SET_4( a, c )                do{ a|=mv_azflags( a, 's', c ); }while(0)
#define     AZFLAG_GET_E_4( a, A, c )           (AZFLAG_GET_4( a, c )|AZFLAG_GET_4( A, c ))
#define     AZFLAG_SET_E_4( a, A, c )           do{ a|=mv_azflags( a, 's', c );A|=mv_azflags( a, 'S', c ); }while(0)
#define     AZFLAG_RESET( a, c )                MVF_SET_BIT( a, AZFLAG_GET( 0, c ), 0 )
#define     AZFLAG_RESET_4( a, c )              MVF_SET_BIT( a, AZFLAG_GET_4( 0, c ), 0 )

#define     azBIT_M( c )                       ((1<<((c)-'a')))
#define     azBITS_M( c )                       ((1<<(MV_U32_GET_BITS(c, 0, 8)-'a')) \
                                               |(1<<(MV_U32_GET_BITS(c, 8, 8)-'a')) \
                                               |(1<<(MV_U32_GET_BITS(c, 16, 8)-'a')) \
                                               |(1<<(MV_U32_GET_BITS(c, 24, 8)-'a')))
#define 	azBIT(a)        					(1<<(a-'a'))

#define     MOD2(value, size)	                ( ((long)value)&((size)-1) )
#define     MV_GET_BIT(a, m, b)                 ((a)&(~(m)))|((b)&(m)))
#define     MVF_SET_BIT(a, m, b)                a = (((a)&(~(m)))|((b)&(m)))
#if defined( __GNUC__ )
#if defined( __U_BOOT__ )
#define     GETCH()     getc();
#else
#define     GETCH()     getc(stdin);
#endif
#else
#define     GETCH()     getch()
#endif
#if !defined (_WIN32)
#define     MV_PAUSE(...)                           do{ if (MVF_GFLAG(GF_YES)) break; mvf_fprintf( MV_ROC_IOS_OUT, "Any key continue...\n" );mvf_fprintf( MV_ROC_IOS_OUT, __VA_ARGS__ );GETCH();}while(0);
#else
#define     MV_PAUSE( _X_ )                         do { if (MVF_GFLAG(GF_YES)) break; printf _X_; GETCH();}while(0);
#endif

#define     ADD32MAX(a, b)                      ((b)<=(0xFFFFFFFF-(a))?((a)+(b)):0xFFFFFFFF)
#define     POINT_CHECK_INSIDE(a, s, e)         (((a)>=(s))&&(((a))<=ADD32MAX(s,(e)-1)))
#define     SEG_CHECK_INTERSECT(a, b, s, e)     ((((a)<(s))&&ADD32MAX(a,(b)-1)>=(s)))|| \
                                                (((a)<=ADD32MAX(s,(e)-1))&&(ADD32MAX(a,(b)-1)>(ADD32MAX(s,(e)-1)))) 
#define     SEG_CHECK_INSIDE(a, b, s, e)        (((a)>=(s))&&(ADD32MAX(a,(b)-1)<=ADD32MAX(s,(e)-1)))
#define     SEG_CHECK_OVERLAPPED(a, b, s, e)    (SEG_CHECK_INTERSECT(a, b, s, e)||SEG_CHECK_INSIDE(a, b, s, e))

#define     LCAST_SAFE( a, type, b )            do{ type c = (type)(a); b; }while(0)
#define     MV_ARRAY_NUM(a, depth)              (sizeof(a)/sizeof(a##depth))
#define     MV_CHAR_LWR(a)                      ((((a)>='A')&&((a)<='Z'))?(a)+('a'-'A'):(a))
#define     MV_BITS(b)                          (MV_BIT(b)-1)
#define     MV_U32_GET_BITS(a, s, n)            ((((MV_U32)(a))>>(s))&MV_BITS(n))
#define     MV_U32_PUT_BITS(a, s, n, d)         MVF_SET_BIT ( a, MV_BITS(n)<<s, MV_U32_GET_BITS(d, 0, n)<<s )

#define     UniT( a, u )                       ((u=='K')?((long)(a)<<10):((u=='M')?((long)(a)<<20):((u=='G')?((long)(a)<<30):0)))

MV_U32      mv_crc32 (MV_U32, const MV_U8 *, unsigned int);
void
DumpMemory
(
    void *mem,
    int  count
);
#endif
