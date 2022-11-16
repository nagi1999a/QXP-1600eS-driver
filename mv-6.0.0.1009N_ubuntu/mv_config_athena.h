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

#ifndef __MV_CONFIG_ATHENA_H__
#define __MV_CONFIG_ATHENA_H__

#define ATHENA_PRODUCT

/* driver configuration */
#define mv_driver_name   "mv14xx"
#define mv_product_name  "ATHENA"

/*Driver Version for Command Line Interface Query.*/
#define VER_MAJOR           6
#define VER_MINOR        0
#define VER_OEM        	0
#ifdef RAID_DRIVER
#define VER_BUILD        1411
#define VER_TEST         ""
#else
#define VER_BUILD        1009
#define VER_TEST         "N"
#endif
#define  SUPPORT_MSIX_INT     1
#define 	MV_DEBUG	1
#ifdef MV_DEBUG
//#define	STRONG_DEBUG	1
#endif

#ifndef RAID_DRIVER
#define LINUX_NONRAID
#define ATHENA_PERFORMANCE_TUNNING      1
#define SUPPORT_STAGGERED_SPIN_UP			1
#define SUPPORT_DIRECT_SATA_SSU 	1
//#define SUPPORT_DIRECT_SATA_SSU	1
//#define	SUPPORT_OEM_PROJECT		1	//Enable it only OEM project, generic driver should disable it.
#ifndef SUPPORT_OEM_PROJECT
/*Temp disable DIF support. For hw has some problem on data write*/
#define		SUPPORT_DIF	1
#define		SUPPORT_VAR_LEN_CDB 1
//#define		SUPPORT_DIX 1
//#define		USE_OS_TIMEOUT_VALUE 1
#endif

#endif
//#define 	SUPPORT_IO_DELAY	1
//#define	DEBUG_EXPANDER			1
#ifdef SUPPORT_OEM_PROJECT
#define	SKIP_INTERNAL_INITIALIZE	1
#define 	ENABLE_HARD_RESET_EH		1
#endif

#define 	ENABLE_HARD_RESET_EH		1
#define SUPPORT_ENHANCED_EH	1
#ifdef SUPPORT_ENHANCED_EH
/* This workaround can recovered all disks drop issue with AIC expanders.
   Temp disable it, if one real bad disk exists, which can't be recoverd,
   we shouldn't reset all expander port. This will block IO on all devices.*/
//#define SUPPORT_PORT_RESET_FOR_STP	1
#endif

#ifndef MV_VMK_ESX35
#if (!defined(RAID_DRIVER) && !defined(__VMKLNX__))
#define SUPPORT_MUL_LUN					1
#endif
#endif

#define SUPPORT_SG_RESET 1

#define SUPPORT_LUIGI	1
#ifdef SUPPORT_LUIGI
#ifndef RAID_DRIVER
#define USE_OS_TIMEOUT_VALUE	1
#endif
#endif

#include "mv_product_athena.h"
#include "mv_hba.h"
#ifdef RAID_DRIVER
#include "mv_raid.h"
#endif

//#define SUPPORT_VU_CMD				1
//#define ASIC_WORKAROUND_WD_TO_RESET
//#define SWAP_VANIR_PORT_FOR_LENOVO
#ifndef RAID_DRIVER
#define FIX_SCSI_ID_WITH_PHY_ID
#endif
#ifdef FIX_SCSI_ID_WITH_PHY_ID
	#define PORT_NUMBER 16
#endif
#include "mv_linux.h"

#endif/*__MV_CONFIG_ATHENA_H__*/
