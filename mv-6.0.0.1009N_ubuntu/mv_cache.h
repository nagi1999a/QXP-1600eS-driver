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

#ifndef _CACHE_PARAMETER_FILE_
#define _CACHE_PARAMETER_FILE_
/*
*
 *  these is used for count related and the number is sector
 */

#define SECTOR_SHIFT     9

////////////////////////////////////////////////
#define MAX_PAGES_PER_CU ( 32*4 )
#define COUNT_PAGE    (512 * 4  )
#define SHIFT_CNSECTOR_PER_PAGE (3)  //This valu is equal to log2 (COUNT_SECTOR_PER_PAGE)



#define MIN_CACHE_UNIT	 ( ( 1 << ( K_SHIFT - SECTOR_SHIFT ) )   * 128)   //sector  128k
#define SHIFT_FACTOR_WIDTH 31

//////////////////////////////////////////////////////////////////////////////

#define COUNT_CU      ((COUNT_PAGE) >> 4 )
#define COUNT_CACHE_REQUEST  (COUNT_CU << 2)
#define COUNT_VALIDRANGE (COUNT_CU * 3)


#define MAX_CDEVICE_SUPPORT MAX_LD_SUPPORTED_PERFORMANCE
#define MAX_CTARGET_NUM MV_MAX_TARGET_NUMBER 


#define COUNT_SECTOR_LARGE_REQUEST  (512*2)  //in sectors, so the value no is 512k


/* dertermine the low and high mark
*/
#define CACHE_LOW_MARK_PAGE  ( COUNT_PAGE >> 2)  
#define CACHE_HIGH_MARK_PAGE (  (COUNT_PAGE >> 2) * 3 )

#define CACHE_LOW_MARK_CU  ( COUNT_CU >> 3 )
#define CACHE_HIGH_MARK_CU  (COUNT_CU >> 1)

#define CACHE_LOW_MARK_VR  ((COUNT_VALIDRANGE) >> 3)
#define CACHE_HIGH_MARK_VR   (  COUNT_VALIDRANGE >> 1)

/*this define are used for seq recognize*/
#define WATERMARK_SERIAL_SEQ 1 
#define WATERMARK_SERIAL_LOC 8

#define MARK_SMALL_BLOCK 16 //8K


/*MAX AND MIN PREREAD LENGTH  */
#define MAX_PREREAD_LENGTH	1024
#define MIN_PREREAD_LENGTH	128


/*these define are used for release resource config*/

#define RELEASE_LOOP 0xff
#define RELEASE_COUNT_PER_DEVICE 0xfff


#define COUNT_MAXLOGSEQ_PER_DEVICE		20
#define COUNT_MAXLOGLOC_PER_DEVICE		10
#define MAX_MISS_COUNT		20
#define MAX_HIT_COUNT_ADJUST  30

/*
 * these macros are for time interval adjust
 */
#define TIME_INTERVAL_IDLE 2
#define TIME_INTERVAL_BUSY 2





#if 0
#define CACHE_DBG_INFO(_x_)	do {if (g_cache_print & PRINT_D0) {MV_PRINT("Cache Engine: ");	MV_PRINT _x_; } } while (0)
#else
#define CACHE_DBG_INFO(_x_)	
#if defined(SUPPORT_CHANGE_DEBUG_MODE)
extern MV_U16 mv_debug_mode;
#define CACHE_DBGn_INFO(_x_,n)	do {if ((CACHE_DEBUG_INFO & mv_debug_mode) && (g_cache_print & MV_BIT(n))) {MV_PRINT("Cache Engine d"#n":" );	MV_PRINT _x_; } } while (0)

#else

//#define CACHE_DBG_PRINT
#ifdef CACHE_DBG_PRINT
#define CACHE_DBGn_INFO(_x_,n)	do {if (g_cache_print & MV_BIT(n)) {MV_PRINT("Cache Engine d"#n":" );	MV_PRINT _x_; } } while (0)
#else
#define CACHE_DBGn_INFO(_x_,n)
#endif

#endif /*SUPPORT_CHANGE_DEBUG_MODE*/

#endif
#if 0 
#define C_PRINT MV_PRINT("\n%s %s %d",__FILE__,__FUNCTION__,__LINE__);MV_PRINT
#else
#define C_PRINT MV_PRINT
#endif

/*dertermine system busy?*/
#define WATER_MARK_BUSY 10


/*
  debug options
*/
#ifndef DBG
#define CACHE_DEBUG_RES 	0
#define CACHE_DEBUG_PAGE 	0
#define CACHE_CRC_CHECK		0 /* this should always be 0*/
#define CACHE_DEBUG_BBU         0
#undef	CACHE_DEBUG_SEQ
#else 
#define CACHE_DEBUG_RES 	1
#define CACHE_DEBUG_PAGE 	1
#define CACHE_CRC_CHECK		0  /* this should always be 0*/
#define CACHE_DEBUG_BBU         1
#undef	CACHE_DEBUG_SEQ
#endif

/*template configures
*/
//#define  CACHE_BBU_ECC

//feature control
#define CACHE_DEVICE_LOCK 	0
#define CACHE_SUPPORT_LOC	0
#define CACHE_EXTENT_WIDTH_R10  0
#define CACHE_SUPPORT_SYNC_CMD  1
#define CACHE_WT_FEATURE	0
#define CACHE_OP_LOG		0

#if defined(_OS_LINUX)
#define _XOR_DMA
#endif
#endif



