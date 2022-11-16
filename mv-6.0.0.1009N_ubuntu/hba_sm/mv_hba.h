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

/*Macros within this file are OS independently */
#ifndef __MV_HBA_H__
#define __MV_HBA_H__

#define NEW_CORE_DRIVER		1
#define CORE_NO_RECURSIVE_CALL
#ifndef _OS_LINUX
#ifndef ATHENA_PERFORMANCE_TUNNING
#define SUPPORT_MODULE_CONSOLIDATE		1
#endif
#endif
/* allocate uncached memory multiple times for RAID/Cache module*/
#ifdef _OS_LINUX
#define MULTI_UNCACHE_ALLOC
#endif
/*open this macro to zero sg entry, command header/table memory
 * of each sending request, R/W test on SATA disk easily timeout.*/
#ifndef ATHENA_PERFORMANCE_TUNNING
#define CORE_ZERO_MEMORY_TEST
#endif

#define SUPPORT_TIMER                1
#define SUPPORT_SMALL_TIMER			1
#define SUPPORT_EVENT                1
#define REDUCED_SENSE              1
#define SUPPORT_SCSI_PASSTHROUGH     1
#define SUPPORT_PASS_THROUGH_DIRECT	1
#define REQUEST_TIME_OUT				20		// in multiples of TIMER_INTERVAL, see hba_timer.h

#define MV_MAX_LUN_NUMBER               128
#ifdef SUPPORT_SCSI_PASSTHROUGH
#   define SAT_RETURN_FIS_IN_CDB	1
#   define SUPPORT_VIRTUAL_DEVICE       1
#if !defined (__VMKLNX__)  && !defined(MV_VMK_ESX35)
#   define VIRTUAL_DEVICE_ID            (MV_MAX_TARGET_NUMBER - 1)
#else /*for not use virtual device with vmware*/
#   define VIRTUAL_DEVICE_ID            MV_MAX_TARGET_NUMBER
#endif
#endif /* SUPPORT_SCSI_PASSTHROUGH */
#define MV_MAX_HD_DEVICE_ID                   		MAX_DEVICE_SUPPORTED_PERFORMANCE

/*
 * define USE_NEW_SGTABLE to use the new SG format as defined in
 * "General SG Format" document.
 */
#define USE_NEW_SGTABLE
#ifndef WIN_STORPORT
#if defined(USE_NEW_SGTABLE) && (defined(SOFTWARE_XOR) || defined(CACHE_MODULE_SUPPORT))
#define USE_VIRTUAL_PRD_TABLE           1
#endif
#endif		/* WIN_STORPORT*/

/* support to use NVSRAM memory to save transaction log */
#ifdef SUPPORT_TRANSACTION_LOG
#define SUPPORT_NVSRAM	1
#endif

#endif /*__MV_HBA_H__*/
