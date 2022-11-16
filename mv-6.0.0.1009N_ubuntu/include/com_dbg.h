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

#if !defined(COMMON_DEBUG_H)
#define COMMON_DEBUG_H

/* 
 *	Marvell Debug Interface
 * 
 *	MACRO
 *		MV_DEBUG is defined in debug version not in release version.
 *	
 *	Debug funtions:
 *		MV_PRINT:	print string in release and debug build.
 *		MV_DPRINT:	print string in debug build.
 *		MV_TRACE:	print string including file name, line number in release and debug build.
 *		MV_DTRACE:	print string including file name, line number in debug build.
 *		MV_ASSERT:	assert in release and debug build.
 *		MV_DASSERT: assert in debug build.
  *		MV_WARNON: warn in debug build.
 */

#include "com_define.h"
/*
 *
 * Debug funtions
 *
 */

/* For both debug and release version */
#if defined(SIMULATOR)
#   include <assert.h>
#   define MV_PRINT printf
#   define MV_ASSERT assert
#   define MV_TRACE(_x_)                                   \
              do {	                                   \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);  \
                 MV_PRINT _x_;                             \
	      } while (0)
#   define MV_DUMP_SP()
#elif defined(_OS_WINDOWS)
	#if !defined(_OS_FIRMWARE)


/* for VISTA */
#   if (_WIN32_WINNT >= 0x0600)
       ULONG _cdecl MV_PRINT(char* format, ...);
#   else
#      define MV_PRINT                      DbgPrint
#   endif /* (_WIN32_WINNT >= 0x0600) */

#   if (defined(_CPU_IA_64B) || defined(_CPU_AMD_64B))
#      if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#         define NTAPI __stdcall
#      else
#         define NTAPI
#      endif /* (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) */
	
       void NTAPI DbgBreakPoint(void);
#      define MV_ASSERT(_condition_)    \
                 do { if (!(_condition_)) DbgBreakPoint(); } while(0)
#   else
#      define MV_ASSERT(_condition_)    \
                 do { if (!(_condition_)) {__asm int 3}; } while(0)
#   endif /*  (defined(_CPU_IA_64B) || defined(_CPU_AMD_64B)) */

#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 MV_PRINT _x_;                                  \
              } while (0)
	

	#else /* _OS_FIRMWARE */
		#define MV_TRACE(_x_)

		#define FM_DBG_PRINT
		#if !defined(FM_DBG_PRINT)
			#define MV_PRINT(_x_, ...)		        
			#define MV_ASSERT(_x_)

			#define FM_PRINT(_x_, ...)		
		#else
			void __std_printf (const char *fmt, ...);
			#define FM_PRINT			__std_printf
			#define MV_PRINT		FM_PRINT
			#define MV_ASSERT(_x_)	{															\
				if ( !(_x_) ) {																	\
					MV_DPRINT(("ASSERT FAILURE at %s %d %s!\n", __FILE__, __LINE__, __FUNCTION__));\
				}	\
			}

		#endif	

	#endif /* _OS_FIRMWARE */
#define MV_DUMP_SP()
#elif defined(_OS_LINUX)
#define MV_PRINT(format, arg...) 	ossw_printk(format,##arg);
#ifdef MV_LINUX_KGDB
#define MV_ASSERT(_x_)                do{ if (!(_x_)) asm("int3");}while(0)
#else
#define MV_ASSERT(_x_)  do{if (!(_x_)) MV_PRINT("ASSERT: function:%s, line:%d\n", __FUNCTION__, __LINE__);}while(0)//BUG_ON(!(_x_))
#endif

#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 MV_PRINT _x_;                                  \
           } while(0)
#elif defined(__QNXNTO__)
#   define MV_PRINT      printk("%s: ", mv_full_name), printk
#   define MV_ASSERT(_x_)                                       \
              do {                                              \
		 if (!(_x_))                                    \
                    MV_PRINT("Assert at File %s: Line %d.\n",   \
                             __FILE__, __LINE__);               \
              } while (0)
#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 printk   _x_;                                  \
           } while(0)
#elif defined(_OS_BIOS)
//
//TBD
//
#elif defined(_OS_UKRN)
extern int dbg_printf_ex(unsigned int flags, const char *fmt, ...);
#   define MV_PRINT(fmt, args...)    dbg_printf_ex(0x100, fmt, ##args)
#   define FM_PRINT MV_PRINT
#    define MV_ASSERT(_condition_)    \
                 do { if (!(_condition_)) {__breakpoint(1);}; } while(0)
#   define MV_TRACE(_x_)                                \
          do {	                                        \
             MV_PRINT("%s(%d) ", __FILE__, __LINE__);   \
             MV_PRINT _x_;                              \
      } while (0)
#   define MV_DUMP_SP()
#else /* OTHER OSes */
#   define MV_PRINT(_x_)
#   define MV_ASSERT(_condition_)
#   define MV_TRACE(_x_)
#define MV_DUMP_SP()
#endif /* _OS_WINDOWS */

 
/* For debug version only */
#if defined(SUPPORT_CHANGE_DEBUG_MODE)
extern MV_U16 mv_debug_mode;
#define MV_DPRINT(_x_)                do {\
	if (mv_debug_mode & GENERAL_DEBUG_INFO) \
	MV_PRINT _x_;\
	} while (0)
#define MV_DASSERT(x)	        do {\
	if (mv_debug_mode & GENERAL_DEBUG_INFO) \
	MV_ASSERT(x);\
	} while (0)
#define MV_DTRACE	        MV_DTRACE
#else
#if defined(MV_DEBUG)
#ifdef _OS_FIRMWARE
	#define MV_DPRINT(x)	   FM_PRINT x
#else
	#define MV_DPRINT(x)	   MV_PRINT x
#endif

#define MV_DASSERT	        MV_ASSERT
#define MV_DTRACE	        MV_DTRACE
#elif defined(_OS_BIOS)
//
//TBD
//
#else
#define MV_DPRINT(x)
#define MV_DASSERT(x)
#define MV_DTRACE(x)
#endif /* MV_DEBUG */
#endif /*SUPPORT_CHANGE_DEBUG_MODE*/

#ifdef SUPPORT_ROC
#define LED_DBG_PRINT(x)	   FM_PRINT x
#elif defined(_OS_LINUX)

#if defined(SUPPORT_CHANGE_DEBUG_MODE)
#define LED_DBG_PRINT(_x_)                do {\
	if (mv_debug_mode & LL_DEBUG_INFO) \
	MV_PRINT _x_;\
	} while (0)

#else
#define LED_DBG_PRINT(x)
#endif

#else
#define LED_DBG_PRINT(x)
#endif

MV_BOOLEAN mvLogRegisterModule(MV_U8 moduleId, MV_U32 filterMask, char* name);
MV_BOOLEAN mvLogSetModuleFilter(MV_U8 moduleId, MV_U32 filterMask);
MV_U32 mvLogGetModuleFilter(MV_U8 moduleId);
void mvLogMsg(MV_U8 moduleId, MV_U32 type, char* format, ...);


#endif /* COMMON_DEBUG_H */

