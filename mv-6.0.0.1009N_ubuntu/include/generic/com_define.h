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

#ifndef COM_DEFINE_H
#define COM_DEFINE_H

#include "mv_os.h"

/*
 *  This file defines Marvell OS independent primary data type for all OS.
 *
 *  We have macros to differentiate different CPU and OS.
 *
 *  CPU definitions:
 *  _CPU_X86_16B  
 *  Specify 16bit x86 platform, this is used for BIOS and DOS utility.
 *  _CPU_X86_32B
 *  Specify 32bit x86 platform, this is used for most OS drivers.
 *  _CPU_IA_64B
 *  Specify 64bit IA64 platform, this is used for IA64 OS drivers.
 *  _CPU_AMD_64B
 *  Specify 64bit AMD64 platform, this is used for AMD64 OS drivers.
 *
 *  OS definitions:
 *  _OS_WINDOWS
 *  _OS_LINUX
 *  _OS_FREEBSD
 *  _OS_BIOS
 *  __QNXNTO__
 */

#ifdef _OS_UEFI
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>

#include <Protocol/EfiShell.h>
#include <Library/ShellLib.h>

//#include <Protocol/SimpleFileSystem.h>
#endif

#if defined(_OS_DOS) && defined(COM_DEBUG)
 #include "vsprintf.h"
#endif

#if !defined(IN)
#   define IN
#endif

#if !defined(OUT)
#   define OUT
#endif

#if defined(_OS_LINUX) || defined(__QNXNTO__) || defined(_OS_UKRN)
#   define    BUFFER_PACKED               __attribute__((packed))
#elif defined(_OS_WINDOWS) || defined(_OS_DOS)
    #if !defined(_OS_FIRMWARE)
#   define    BUFFER_PACKED
    #else
#   define    BUFFER_PACKED               __attribute__((packed))
    #endif
#elif defined(_OS_BIOS)
#   define    BUFFER_PACKED
#endif /* defined(_OS_LINUX) || defined(__QNXNTO__) */

#define MV_BIT(x)                         (1UL << (x))

#if !defined(NULL)
#   define NULL 0
#endif  /* NULL */

#define MV_TRUE                           1
#define MV_FALSE                          0
#define MV_SUCCESS						  1
#define MV_FAIL							  0
#define MV_REG_FAIL 1
#define MV_REG_SUCCESS 0
#define MV_IN                           1
#define MV_OUT                          0


typedef unsigned char  MV_BOOLEAN, *MV_PBOOLEAN;
typedef unsigned char  MV_U8, *MV_PU8;
typedef signed char  MV_I8, *MV_PI8;

typedef unsigned short  MV_U16, *MV_PU16;
typedef signed short  MV_I16, *MV_PI16;

typedef void    MV_VOID, *MV_PVOID;
typedef unsigned long MV_ULONG, *MV_PULONG;

#ifdef _OS_BIOS
    typedef MV_U8 GEN_FAR*  MV_LPU8;
    typedef MV_I8 GEN_FAR*  MV_LPI8;
    typedef MV_U16 GEN_FAR* MV_LPU16;
    typedef MV_I16 GEN_FAR* MV_LPI16;

    typedef MV_U32 GEN_FAR* MV_LPU32;
    typedef MV_I32 GEN_FAR* MV_LPI32;
    typedef void GEN_FAR*   MV_LPVOID;
#else
    typedef void            *MV_LPVOID;
#endif /* _OS_BIOS */

/* Pre-define segment in C code*/
#if defined(_OS_BIOS)
#   define BASEATTR         __based(__segname("_CODE")) 
#   define BASEATTRData     __based(__segname("_CODE")) 
#else
#   define BASEATTR 
#endif /* _OS_BIOS */

#ifdef DEBUG_COM_SPECIAL
	#define MV_DUMP_COM_SPECIAL(pString)  						{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM_SPECIAL(pString, value) 				bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM_SPECIAL(pString, value)  				bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM_SPECIAL3(pString, value1, value2)  	bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM_SPECIAL(pString, value)  				bDbgPrintStr_U8(pString, value)	
	#define MV_HALTKEY_SPECIAL									waitForKeystroke()

#else
	#define MV_DUMP_COM_SPECIAL(pString)  						
	#define MV_DUMP32_COM_SPECIAL(pString, value) 				
	#define MV_DUMP16_COM_SPECIAL(pString, value)  				
	#define MV_DUMP32_COM_SPECIAL3(pString, value1, value2)  	
	#define MV_DUMP8_COM_SPECIAL(pString, value)  				
	#define MV_HALTKEY_SPECIAL									
#endif
/* For debug version only */
#if (defined(DEBUG_BIOS)) || (defined(_OS_DOS) && defined(COM_DEBUG))
 #ifdef DEBUG_SHOW_ALL
	#define MV_DUMP32(_x_) 		{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				bDbgPrintStr_U8(pString, value)
 #else
	#define MV_DUMP32(_x_) 		//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						//{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				//bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				//bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	//bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				//bDbgPrintStr_U8(pString, value)
 
 #endif
 
	#define MV_BIOSDEBUG(pString, value)				bDbgPrintStr_U32(pString, value)				
	#define MV_HALTKEY			waitForKeystroke()
	#define MV_ENTERLINE		//mvChangLine()
#else
	#define MV_DUMP32(_x_) 		//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMP16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMP8(_x_)  		//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPC32(_x_)  	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPC16(_x_)  	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPC8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
	#define MV_DUMPE32(_x_) 	//{mvDebugDumpU32(_x_);bCOMEnter();}
	#define MV_DUMPE16(_x_) 	//{mvDebugDumpU16(_x_);bCOMEnter();}
	#define MV_DUMPE8(_x_)  	//{mvDebugDumpU8(_x_);bCOMEnter();}
 	#define MV_DUMP_COM(pString)  						//{bDbgPrintStr(pString);bCOMEnter();}
	#define MV_DUMP32_COM(pString, value) 				//bDbgPrintStr_U32(pString, value)
	#define MV_DUMP16_COM(pString, value)  				//bDbgPrintStr_U16(pString, value)
	#define MV_DUMP32_COM3(pString, value1, value2)  	//bDbgPrintStr_U32_3(pString, value1, value2)
	#define MV_DUMP8_COM(pString, value)  				//bDbgPrintStr_U8(pString, value)

	#define MV_BIOSDEBUG(pString, value)
#if defined(_OS_DOS) && defined(MV_DEBUG)
	#define MV_HALTKEY mv_waitfor_key_stroke()
#else
#if defined(DEBUG_FW30)
	#define MV_HALTKEY  waitForKeystroke()
#else
	#define MV_HALTKEY
#endif
#endif	
	#define MV_ENTERLINE
	
#endif

#if defined(_OS_LINUX) || defined(__QNXNTO__) || defined(_OS_FIRMWARE) || defined(_OS_MAC) || defined(_OS_VMWARE) || defined(_OS_UEFI)
    /* unsigned/signed long is 64bit for AMD64, so use unsigned int instead */
    typedef unsigned int MV_U32, *MV_PU32;
    typedef   signed int MV_I32, *MV_PI32;
    typedef   signed long MV_ILONG, *MV_PILONG;
#else
    /* unsigned/signed long is 32bit for x86, IA64 and AMD64 */
    typedef unsigned long MV_U32, *MV_PU32;
    typedef   signed long MV_I32, *MV_PI32;
#endif /*  defined(_OS_LINUX) || defined(__QNXNTO__) */

#if defined(_OS_WINDOWS) || defined(_OS_DOS) || defined(_OS_FIRMWARE)
  #if defined(_OS_FIRMWARE)
    typedef unsigned __int64 _MV_U64;
    typedef   signed __int64 _MV_I64;
  #else
    typedef unsigned long long _MV_U64;
    typedef   signed long long _MV_I64;
  #endif  
#elif defined(_OS_LINUX) || defined(__QNXNTO__) || defined (_OS_VMWARE) || defined(_OS_UEFI)
    typedef unsigned long long _MV_U64;
    typedef   signed long long _MV_I64;
#elif defined(_OS_FREEBSD)
#   error "No Where"
#elif defined(_OS_BIOS)
//
//NA
//
#elif defined(_OS_MAC)
/*typedef unsigned char  MV_BOOLEAN, *MV_PBOOLEAN;
typedef unsigned char  MV_U8, *MV_PU8;
typedef signed char  MV_I8, *MV_PI8;

typedef unsigned short  MV_U16, *MV_PU16;
typedef signed short  MV_I16, *MV_PI16;

typedef void    MV_VOID, *MV_PVOID;
typedef unsigned int MV_U32, *MV_PU32;
typedef   signed int MV_I32, *MV_PI32;*/
typedef unsigned long long _MV_U64;
typedef   signed long long _MV_I64;
#endif /* _OS_WINDOWS */

//typedef _MV_U64 BUS_ADDRESS;

#if defined(_OS_LINUX) || defined(_OS_UEFI)
#   if defined(__KCONF_64BIT__)
#      define _SUPPORT_64_BIT
#   else
#      ifdef _SUPPORT_64_BIT
#         error Error 64_BIT CPU Macro
#      endif
#   endif /* defined(__KCONF_64BIT__) */
#elif defined(_OS_BIOS) || defined(__QNXNTO__)
#   undef  _SUPPORT_64_BIT
#else
  #if !defined(_OS_FIRMWARE)
#   define _SUPPORT_64_BIT
  #else
#   if defined(__KCONF_64BIT__)
#      define _SUPPORT_64_BIT
#   else
#      ifdef _SUPPORT_64_BIT
#         error Error 64_BIT CPU Macro
#      endif
#   endif /* defined(__KCONF_64BIT__) */
  #endif /* _OS_FIRMWARE */
#endif /* _OS_LINUX */

/*
 * Primary Data Type
 */
#if defined(_OS_WINDOWS) || defined(_OS_DOS) || defined(_OS_FIRMWARE) || defined(_OS_MAC)
  #if !defined(_OS_FIRMWARE)
    typedef union {
        struct {
            MV_U32 low;
            MV_U32 high;
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;
  #else
    typedef union {
        struct {
#   if defined (__MV_LITTLE_ENDIAN__)
            MV_U32 low;
            MV_U32 high;
#   elif defined (__MV_BIG_ENDIAN__)
            MV_U32 high;
            MV_U32 low;
#   else
#           error "undefined endianness"
#   endif /* __MV_LITTLE_ENDIAN__ */
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;
  #endif	
#elif defined(_OS_LINUX) || defined(__QNXNTO__) || defined(_OS_VMWARE) || defined(_OS_UEFI)
    typedef union {
        struct {
#   if defined (__MV_BIG_ENDIAN__)
            MV_U32 high;
            MV_U32 low;
#   else
			MV_U32 low;
			MV_U32 high;
#   endif /* __MV_LITTLE_ENDIAN__ */
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;
#else
/* BIOS compiler doesn't support 64 bit data structure. */
    typedef union {
        struct {
             MV_U32 low;
             MV_U32 high;
        };
				struct {
					MV_U32 low;
					MV_U32 high;
				}parts;
        struct {
             MV_U32 value;
             MV_U32 value1;
        };
    } _MV_U64,MV_U64, *MV_PU64, *PMV_U64;
#endif /* defined(_OS_LINUX) || defined(_OS_WINDOWS) || defined(__QNXNTO__)*/

typedef _MV_U64 BUS_ADDRESS;

/* PTR_INTEGER is necessary to convert between pointer and integer. */
#if defined(_SUPPORT_64_BIT)
   typedef _MV_U64 MV_PTR_INTEGER;
#else
   typedef MV_U32 MV_PTR_INTEGER;
#endif /* defined(_SUPPORT_64_BIT) */

#ifdef _SUPPORT_64_BIT
#define _64_BIT_COMPILER     1
#endif

/* LBA is the logical block access */
typedef MV_U64 MV_LBA;

#if defined(_CPU_16B)
    typedef MV_U32 MV_PHYSICAL_ADDR;
#else
    typedef MV_U64 MV_PHYSICAL_ADDR;
#endif /* defined(_CPU_16B) */

#if defined (_OS_WINDOWS)
	#if !defined(_OS_FIRMWARE)
		typedef MV_PVOID MV_FILE_HANDLE;
	#else
		typedef MV_I32 MV_FILE_HANDLE;
	#endif
#elif defined(_OS_LINUX) || defined(__QNXNTO__) || defined (_OS_MAC) || defined(_OS_VMWARE)
typedef MV_I32 MV_FILE_HANDLE;
#elif defined (_OS_DOS)
typedef int MV_FILE_HANDLE;
#elif defined (_OS_UEFI)
typedef EFI_FILE_PROTOCOL *MV_FILE_HANDLE;
#endif

#ifdef _OS_UEFI
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif

#include "com_product.h"

/* OS_LINUX depedent definition*/

#if defined(_OS_LINUX) || defined(__QNXNTO__)
#define hba_local_irq_disable() ossw_local_irq_disable()
#define hba_local_irq_enable() ossw_local_irq_enable()
#define hba_local_irq_save(flag) ossw_local_irq_save(&flag)
#define hba_local_irq_restore(flag) ossw_local_irq_restore(&flag)

/* expect pointers */
#ifdef ATHENA_PERFORMANCE_TUNNING
#define OSSW_LOCAL_IRQ_SAVE(flag)	ossw_local_irq_save(&flag)
#define OSSW_LOCAL_IRQ_RESTORE(flag)	ossw_local_irq_restore(&flag)
#define OSSW_INIT_SPIN_LOCK(ext)		spin_lock_init(ext)
#define OSSW_SPIN_LOCK(ext, flag)		spin_lock_irqsave(ext, flag)//ossw_spin_lock(ext, &flag)
#define OSSW_SPIN_UNLOCK(ext, flag)		spin_unlock_irqrestore(ext, flag)  //ossw_spin_unlock(ext, &flag)
#define OSSW_SPIN_LOCK_IRQ(ext)            //ossw_spin_lock_irq(ext)
#define OSSW_SPIN_UNLOCK_IRQ(ext)          //ossw_spin_unlock_irq(ext)
#define OSSW_SPIN_LOCK_IRQSAVE(ext, flag)          //ossw_spin_lock_irq_save(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE(ext, flag)           //ossw_spin_unlock_irq_restore(ext, &flag)
#define OSSW_SPIN_LOCK_GLOBAL(core, ext, flag)         \
	do{\
		spin_lock_irqsave(ext, flag);\
		core_global_lock(core, &flag);\
	}while(0);
#define OSSW_SPIN_UNLOCK_GLOBAL(core, ext, flag)          \
	do{\
		core_global_unlock(core, &flag);\
		spin_unlock_irqrestore(ext, flag);\
	}while(0);

/* for hw multi-queue */
#define OSSW_GET_CPU_NUM() (MV_MIN(ossw_get_cpu_num(), 8))

#define OSSW_SPIN_LOCK_HBA(ext, flag) 	//ossw_spin_lock_hba(ext, &flag)
#define OSSW_SPIN_UNLOCK_HBA(ext, flag) //ossw_spin_unlock_hba(ext, &flag)
#define OSSW_SPIN_LOCK_RESOURCE(ext, flag) ossw_spin_lock_resource(ext, &flag)
#define OSSW_SPIN_UNLOCK_RESOURCE(ext, flag) ossw_spin_unlock_resource(ext, &flag)
#define OSSW_SPIN_LOCK_WAIT_QUEUE(ext, flag, root_id) ossw_spin_lock_wait_queue(ext, &flag, root_id)
#define OSSW_SPIN_UNLOCK_WAIT_QUEUE(ext, flag, root_id) ossw_spin_unlock_wait_queue(ext, &flag, root_id)
#define OSSW_SPIN_LOCK_COMPL_QUEUE(ext, flag) ossw_spin_lock_compl_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_COMPL_QUEUE(ext, flag) ossw_spin_unlock_compl_queue(ext, &flag)
#define OSSW_SPIN_LOCK_CORE_QUEUE(ext, flag) ossw_spin_lock_core_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, flag) ossw_spin_unlock_core_queue(ext, &flag)
#define OSSW_SPIN_LOCK_ROOT(ext, flag, root_id) ossw_spin_lock_root(ext, &flag, root_id)
#define OSSW_SPIN_UNLOCK_ROOT(ext, flag, root_id) ossw_spin_unlock_root(ext, &flag, root_id)
#define OSSW_SPIN_LOCK_HW_QUEUE(ext, flag, queue_id) ossw_spin_lock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_QUEUE(ext, flag, queue_id) ossw_spin_unlock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_CORE(ext, flag) //ossw_spin_lock_core(ext, &flag)
//#define OSSW_SPIN_TRYLOCK_CORE(ext, flag) ossw_spin_trylock_core(ext, &flag)
#define OSSW_SPIN_UNLOCK_CORE(ext, flag) //ossw_spin_unlock_core(ext, &flag)
#define OSSW_SPIN_LOCK_HW_WQ(ext, flag, queue_id) //ossw_spin_lock_hw_wq(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_WQ(ext, flag, queue_id) //ossw_spin_unlock_hw_wq(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_HW_CQ(ext, flag, queue_id) ossw_spin_lock_hw_cq(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_CQ(ext, flag, queue_id) ossw_spin_unlock_hw_cq(ext, &flag, queue_id)

#define OSSW_SPIN_LOCK_Device(ext, flag, base) ossw_spin_lock_device(ext, &flag, base)
#define OSSW_SPIN_UNLOCK_Device(ext, flag, base) ossw_spin_unlock_device(ext, &flag, base)

#ifdef SUPPORT_STAGGERED_SPIN_UP
#define OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(ext, flag)          //ossw_spin_lock_irq_save_spin_up(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(ext, flag)           //ossw_spin_unlock_irq_restore_spin_up(ext, &flag)
#endif
#else	// #ifdef ATHENA_PERFORMANCE_TUNNING
#define OSSW_LOCAL_IRQ_SAVE(flag)	ossw_local_irq_save(&flag)
#define OSSW_LOCAL_IRQ_RESTORE(flag)	ossw_local_irq_restore(&flag)
#define OSSW_INIT_SPIN_LOCK(ext) ossw_init_spin_lock(ext)
#define OSSW_SPIN_LOCK(ext)            ossw_spin_lock(ext)
#define OSSW_SPIN_UNLOCK(ext)          ossw_spin_unlock(ext)
#define OSSW_SPIN_LOCK_IRQ(ext)            ossw_spin_lock_irq(ext)
#define OSSW_SPIN_UNLOCK_IRQ(ext)          ossw_spin_unlock_irq(ext)
#define OSSW_SPIN_LOCK_IRQSAVE(ext, flag)          ossw_spin_lock_irq_save(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE(ext, flag)           ossw_spin_unlock_irq_restore(ext, &flag)
#define OSSW_SPIN_LOCK_GLOBAL(core, ext, flag)
#define OSSW_SPIN_UNLOCK_GLOBAL(core, ext, flag)

/* for hw multi-queue */
#define OSSW_GET_CPU_NUM() (MV_MIN(ossw_get_cpu_num(), 8))

#define OSSW_SPIN_LOCK_HBA(ext, flag) ossw_spin_lock_hba(ext, &flag)
#define OSSW_SPIN_UNLOCK_HBA(ext, flag) ossw_spin_unlock_hba(ext, &flag)
#define OSSW_SPIN_LOCK_RESOURCE(ext, flag) ossw_spin_lock_resource(ext, &flag)
#define OSSW_SPIN_UNLOCK_RESOURCE(ext, flag) ossw_spin_unlock_resource(ext, &flag)
#define OSSW_SPIN_LOCK_WAIT_QUEUE(ext, flag) ossw_spin_lock_wait_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_WAIT_QUEUE(ext, flag) ossw_spin_unlock_wait_queue(ext, &flag)
#define OSSW_SPIN_LOCK_COMPL_QUEUE(ext, flag) ossw_spin_lock_compl_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_COMPL_QUEUE(ext, flag) ossw_spin_unlock_compl_queue(ext, &flag)
#define OSSW_SPIN_LOCK_CORE_QUEUE(ext, flag) ossw_spin_lock_core_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, flag) ossw_spin_unlock_core_queue(ext, &flag)
#define OSSW_SPIN_LOCK_ROOT(ext, flag, root_id) ossw_spin_lock_root(ext, &flag, root_id)
#define OSSW_SPIN_UNLOCK_ROOT(ext, flag, root_id) ossw_spin_unlock_root(ext, &flag, root_id)
#define OSSW_SPIN_LOCK_HW_QUEUE(ext, flag, queue_id) ossw_spin_lock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_QUEUE(ext, flag, queue_id) ossw_spin_unlock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_CORE(ext, flag) ossw_spin_lock_core(ext, &flag)
//#define OSSW_SPIN_TRYLOCK_CORE(ext, flag) ossw_spin_trylock_core(ext, &flag)
#define OSSW_SPIN_UNLOCK_CORE(ext, flag) ossw_spin_unlock_core(ext, &flag)
#define OSSW_SPIN_LOCK_HW_WQ(ext, flag, queue_id) ossw_spin_lock_hw_wq(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_WQ(ext, flag, queue_id) ossw_spin_unlock_hw_wq(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_HW_CQ(ext, flag, queue_id) ossw_spin_lock_hw_cq(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_CQ(ext, flag, queue_id) ossw_spin_unlock_hw_cq(ext, &flag, queue_id)

#ifdef SUPPORT_STAGGERED_SPIN_UP
#define OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(ext, flag)          ossw_spin_lock_irq_save_spin_up(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(ext, flag)           ossw_spin_unlock_irq_restore_spin_up(ext, &flag)
#endif	// #ifdef SUPPORT_STAGGERED_SPIN_UP
#endif	// #ifdef ATHENA_PERFORMANCE_TUNNING else
/* Delayed Execution Services */
#define OSSW_INIT_TIMER(ptimer) ossw_init_timer(ptimer)

#else /* For Windows */

#define hba_local_irq_disable()
#define hba_local_irq_enable()
#define hba_local_irq_save(flag) 
#define hba_local_irq_restore(flag) 

/* expect pointers */
#ifdef ATHENA_PERFORMANCE_TUNNING
extern MV_VOID windows_lock_wait_queue(MV_PVOID ext, MV_PU32 flag, MV_U16 root_id);
extern MV_VOID windows_unlock_wait_queue(MV_PVOID ext, MV_PU32 flag, MV_U16 root_id);
extern MV_VOID windows_lock_core_resource(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_unlock_core_resource(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_lock_cmpl_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_unlock_cmpl_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_lock_core_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_unlock_core_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_lock_root(MV_PVOID ext, MV_PU32 flag, MV_U16 root_id);
extern MV_VOID windows_unlock_root(MV_PVOID ext, MV_PU32 flag, MV_U16 root_id);
extern MV_VOID windows_lock_hw_queue(MV_PVOID ext, MV_PU32 flag, MV_U16 queue_id);
extern MV_VOID windows_unlock_hw_queue(MV_PVOID ext, MV_PU32 flag, MV_U16 queue_id);
extern MV_VOID windows_lock_cmpl_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_unlock_cmpl_queue(MV_PVOID ext, MV_PU32 flag);
extern MV_VOID windows_lock_device(MV_PVOID ext, MV_PU32 flag, MV_PVOID base_p);
extern MV_VOID windows_unlock_device(MV_PVOID ext, MV_PU32 flag, MV_PVOID base_p);
extern MV_VOID windows_lock_cmpl_queue_cq(MV_PVOID ext, MV_PU32 flag, MV_U16 queue_id);
extern MV_VOID windows_unlock_cmpl_queue_cq(MV_PVOID ext, MV_PU32 flag, MV_U16 queue_id);
extern MV_VOID windows_lock(MV_PVOID plock);
extern MV_VOID windows_unlock(MV_PVOID plock);

#define OSSW_LOCAL_IRQ_SAVE(flag)
#define OSSW_LOCAL_IRQ_RESTORE(flag)
#define OSSW_INIT_SPIN_LOCK(plock) KeInitializeSpinLock(plock);
#define OSSW_SPIN_LOCK(plock, flags)           windows_lock(plock)
#define OSSW_SPIN_UNLOCK(plock, flags)         windows_unlock(plock)
#define OSSW_SPIN_LOCK_IRQ(plock)
#define OSSW_SPIN_UNLOCK_IRQ(plock)
#define OSSW_SPIN_LOCK_IRQSAVE(plock, flag)          
#define OSSW_SPIN_UNLOCK_IRQRESTORE(plock, flag)
#define OSSW_SPIN_LOCK_GLOBAL(core, ext, flag)         \
	do{\
		core_global_lock(core, &flag);\
	}while(0);
#define OSSW_SPIN_UNLOCK_GLOBAL(core, ext, flag)          \
	do{\
		core_global_unlock(core, &flag);\
	}while(0);
#define OSSW_GET_CPU_NUM() (16)
#define OSSW_SPIN_LOCK_HBA(ext, flag)
#define OSSW_SPIN_UNLOCK_HBA(ext, flag)
#define OSSW_SPIN_LOCK_RESOURCE(ext, flag)    windows_lock_core_resource(ext, &flag)
#define OSSW_SPIN_UNLOCK_RESOURCE(ext, flag)    windows_unlock_core_resource(ext, &flag)
#define OSSW_SPIN_LOCK_WAIT_QUEUE(ext, flag, root_id)    windows_lock_wait_queue(ext, &flag, root_id)
#define OSSW_SPIN_UNLOCK_WAIT_QUEUE(ext, flag, root_id)    windows_unlock_wait_queue(ext, &flag, root_id)
#define OSSW_SPIN_LOCK_COMPL_QUEUE(ext, flag) windows_lock_cmpl_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_COMPL_QUEUE(ext, flag)  windows_unlock_cmpl_queue(ext, &flag)
#define OSSW_SPIN_LOCK_CORE_QUEUE(ext, flag)    windows_lock_core_queue(ext, &flag)
#define OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, flag)    windows_unlock_core_queue(ext, &flag)
#define OSSW_SPIN_LOCK_ROOT(ext, flag, root_id)    windows_lock_root(ext, &flag, root_id)
#define OSSW_SPIN_UNLOCK_ROOT(ext, flag, root_id)    windows_unlock_root(ext, &flag, root_id)
#define OSSW_SPIN_LOCK_HW_QUEUE(ext, flag, queue_id)    windows_lock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_QUEUE(ext, flag, queue_id)    windows_unlock_hw_queue(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_CORE(ext, flag) 
//#define OSSW_SPIN_TRYLOCK_CORE(ext, flag) 
#define OSSW_SPIN_UNLOCK_CORE(ext, flag)
#define OSSW_SPIN_LOCK_HW_WQ(ext, flag, queue_id)	
#define OSSW_SPIN_UNLOCK_HW_WQ(ext, flag, queue_id)   
#define OSSW_SPIN_LOCK_HW_CQ(ext, flag, queue_id) windows_lock_cmpl_queue_cq(ext, &flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_CQ(ext, flag, queue_id)windows_unlock_cmpl_queue_cq(ext, &flag, queue_id)
#define OSSW_SPIN_LOCK_Device(ext, flag, base)    windows_lock_device(ext, &flag, base)
#define OSSW_SPIN_UNLOCK_Device(ext, flag, base)    windows_unlock_device(ext, &flag, base)

#ifdef SUPPORT_STAGGERED_SPIN_UP
#define OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(plock, flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(plock, flag)
#endif
#else
#define OSSW_LOCAL_IRQ_SAVE(flag)
#define OSSW_LOCAL_IRQ_RESTORE(flag)
#define OSSW_INIT_SPIN_LOCK(plock) 
#define OSSW_SPIN_LOCK(plock)           
#define OSSW_SPIN_UNLOCK(plock)           
#define OSSW_SPIN_LOCK_IRQ(plock)
#define OSSW_SPIN_UNLOCK_IRQ(plock)
#define OSSW_SPIN_LOCK_IRQSAVE(plock, flag)          
#define OSSW_SPIN_UNLOCK_IRQRESTORE(plock, flag)
#define OSSW_GET_CPU_NUM() (16)
#define OSSW_SPIN_LOCK_HBA(ext, flag)
#define OSSW_SPIN_UNLOCK_HBA(ext, flag)
#define OSSW_SPIN_LOCK_RESOURCE(ext, flag)
#define OSSW_SPIN_UNLOCK_RESOURCE(ext, flag)
#define OSSW_SPIN_LOCK_WAIT_QUEUE(ext, flag)
#define OSSW_SPIN_UNLOCK_WAIT_QUEUE(ext, flag)
#define OSSW_SPIN_LOCK_COMPL_QUEUE(ext, flag)
#define OSSW_SPIN_UNLOCK_COMPL_QUEUE(ext, flag)
#define OSSW_SPIN_LOCK_CORE_QUEUE(ext, flag)
#define OSSW_SPIN_UNLOCK_CORE_QUEUE(ext, flag)
#define OSSW_SPIN_LOCK_ROOT(ext, flag, root_id) 
#define OSSW_SPIN_UNLOCK_ROOT(ext, flag, root_id)
#define OSSW_SPIN_LOCK_HW_QUEUE(ext, flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_QUEUE(ext, flag, queue_id)
#define OSSW_SPIN_LOCK_CORE(ext, flag) 
//#define OSSW_SPIN_TRYLOCK_CORE(ext, flag) 
#define OSSW_SPIN_UNLOCK_CORE(ext, flag)
#define OSSW_SPIN_LOCK_HW_WQ(ext, flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_WQ(ext, flag, queue_id)
#define OSSW_SPIN_LOCK_HW_CQ(ext, flag, queue_id)
#define OSSW_SPIN_UNLOCK_HW_CQ(ext, flag, queue_id)
#ifdef SUPPORT_STAGGERED_SPIN_UP
#define OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(plock, flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(plock, flag)
#endif
#endif
/* Delayed Execution Services */
#define OSSW_INIT_TIMER(ptimer) 

#endif


#if !defined( likely )
#if defined( SUPPORT_ROC )&&defined( _OS_FIRMWARE )
#define likely(x)		__builtin_expect((x),1)
#define unlikely(x)		__builtin_expect((x),0)
#else
#define likely(x)		(x)
#define unlikely(x)		(x)
#endif
#endif

#if defined( SUPPORT_ROC )&&defined( _OS_FIRMWARE )
#define MV_GO_SECT(class, grp)		__attribute__((section(".cold_rain_"#grp)))
#else
#define MV_GO_SECT(class, grp)
#endif

#if defined(_OS_LINUX)

#if defined(SUPPORT_CHANGE_DEBUG_MODE)
#define GENERAL_DEBUG_INFO	MV_BIT(0)	/*For MV_DPRINT*/
#define CORE_DEBUG_INFO		MV_BIT(1)	/*Core debug info: CORE_PRINT, CORE_EH_PRINT*/
#define RAID_DEBUG_INFO		MV_BIT(2)	/*Raid debug info*/
#define CACHE_DEBUG_INFO	MV_BIT(3)	/*Cache debug info*/
#define LL_DEBUG_INFO	MV_BIT(4)	/*LowLevel debug info*/
#define CORE_FULL_DEBUG_INFO	(CORE_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) /*CORE_DPRINT, CORE_EH_PRINT, CORE_EVENT_PRINT*/
#define RAID_FULL_DEBUG_INFO	(RAID_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) 
#define CACHE_FULL_DEBUG_INFO	(CACHE_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) 
#endif /*SUPPORT_CHANGE_DEBUG_MODE*/

#define sg_map(x)	hba_map_sg_to_buffer(x)
#define sg_unmap(x)	hba_unmap_sg_to_buffer(x)

#define time_is_expired(x) 	ossw_time_expired(x)
#define EVENT_SET_DELAY_TIME(x, y) ((x) = ossw_set_expired_time(y))

#define msi_enabled(x)	hba_msi_enabled(x)
#ifdef SUPPORT_MSIX_INT
#define msix_enabled(x)	hba_msix_enabled(x)
#else
#define msix_enabled(x)   MV_FALSE
#endif
#define test_enabled(x) hba_test_enabled(x)
#else
#define sg_map(x)
#define sg_unmap(x)

#define time_is_expired(x)	MV_TRUE	
#define EVENT_SET_DELAY_TIME(x, y)
#ifdef SUPPORT_MSI
#define msi_enabled(x)	hba_msi_enabled(x)
#else
#define msi_enabled(x)	MV_FALSE
#endif
#ifdef SUPPORT_MSIX_INT
#define msix_enabled(x)	hba_msix_enabled(x)
#else
#define msix_enabled(x) MV_FALSE
#endif
#ifdef SUPPORT_INEJECT_ERROR
#define test_enabled(x) MV_TRUE
#else
#define test_enabled(x) MV_FALSE
#endif
#ifndef MV_API_EXPORTS

#if defined(_OS_BIOS)||defined(SUPPORT_ROC)
static inline MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type){return MV_FALSE;};
#endif

#endif

#endif

#endif /* COM_DEFINE_H */
