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

#ifndef __MV_COM_IOCTL_H__
#define __MV_COM_IOCTL_H__

#if defined(_OS_WINDOWS) && !defined(_OS_FIRMWARE)
#include <ntddscsi.h>
#elif defined(_OS_LINUX)

#endif /* _OS_WINDOWS */

/* private IOCTL commands */
#define MV_IOCTL_CHECK_DRIVER                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x900, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/*
 * MV_IOCTL_LEAVING_S0 is a notification when the system is going 
 * to leaving S0. This gives the driver a chance to do some house
 * keeping work before system really going to sleep.
 *
 * The MV_IOCTL_LEAVING_S0 will be translated to APICDB0_ADAPTER/
 * APICDB1_ADAPTER_POWER_STATE_CHANGE and passed down along the
 * module stack. A module shall handle this request if necessary.
 *
 * Upon this request, usually the Cache module shall flush all
 * cached data. And the RAID module shall auto-pause all background
 * activities.
 */
#define MV_IOCTL_LEAVING_S0                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x901, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/*
 * MV_IOCTL_REENTER_S0 is a notification when the system is going 
 * to re-entering S0. This gives the driver a chance to resume
 * something when system wakes up.
 *
 * The MV_IOCTL_REENTER_S0 will be translated to APICDB0_ADAPTER/
 * APICDB1_ADAPTER_POWER_STATE_CHANGE (CDB2 is 1) and passed down 
 * along the module stack. 
 * A module shall handle this request if necessary.
 */
#define MV_IOCTL_REENTER_S0                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x902, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/* Used for notifying shutdown from dispatch */
#define MV_IOCTL_SHUTDOWN                               \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x903, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	
// private ioctrl for scsi trim
#define MV_IOCTL_TRIM_SCSI                               \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x905, METHOD_BUFFERED,                 \
                     FILE_WRITE_ACCESS)	

// private ioctrl for scsi trim
#define MV_IOCTL_SCSI_PASS_THRU_DIRECT                        \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x406, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	

/* IOCTL signature */
#define MV_IOCTL_DRIVER_SIGNATURE                "mv61xxsg"
#define MV_IOCTL_DRIVER_SIGNATURE_LENGTH         8

/* IOCTL command status */
#define IOCTL_STATUS_SUCCESS                     0
#define IOCTL_STATUS_INVALID_REQUEST             1
#define IOCTL_STATUS_ERROR                       2


/*IOCTL code on linux*/
#ifdef _OS_LINUX
#define API_BLOCK_IOCTL_DEFAULT_FUN	0x1981
#define API_IOCTL_DEFAULT_FUN			0x00
#define	API_IOCTL_GET_VIRTURL_ID		(API_IOCTL_DEFAULT_FUN + 1)
#define	API_IOCTL_GET_HBA_COUNT		(API_IOCTL_DEFAULT_FUN + 2)
#define	API_IOCTL_LOOKUP_DEV			(API_IOCTL_DEFAULT_FUN + 3)
#define  API_IOCTL_CHECK_VIRT_DEV           (API_IOCTL_DEFAULT_FUN + 4)
#define  API_IOCTL_GET_ENC_ID               	(API_IOCTL_DEFAULT_FUN + 5)
#define  API_IOCTL_MAX                      		(API_IOCTL_DEFAULT_FUN + 6)
#endif


#if defined(SUPPORT_ESATA) || defined(SUPPORT_DRIVER_FEATURE)
#define MV_IOCTL_CHECK_DEVICE                                \
	    CTL_CODE(FILE_DEVICE_CONTROLLER,                 \
                     0x904, METHOD_BUFFERED,                 \
                     FILE_READ_ACCESS | FILE_WRITE_ACCESS)	
#endif

#ifndef _OS_BIOS
#pragma pack(8)
#endif  /* _OS_BIOS */

typedef struct _MV_IOCTL_BUFFER
{
#if defined(_OS_WINDOWS) && !defined(_OS_FIRMWARE)
	SRB_IO_CONTROL Srb_Ctrl;
#endif /* _OS_WINDOWS */
	MV_U8          Data_Buffer[32];
} MV_IOCTL_BUFFER, *PMV_IOCTL_BUFFER;
#if defined(SUPPORT_LUIGI_UPDATE_FW) && defined(_OS_WINDOWS)
typedef struct _SCSI_PASS_THROUGH_U32 {
  USHORT Length;
  UCHAR  ScsiStatus;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  CdbLength;
  UCHAR  SenseInfoLength;
  UCHAR  DataIn;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  ULONG  DataBuffer;
  ULONG  SenseInfoOffset;
  UCHAR  Cdb[16];
} SCSI_PASS_THROUGH_U32, *PSCSI_PASS_THROUGH_U32;
typedef struct _MV_IOCTL_BUFFER_EX
{
#if defined(_OS_WINDOWS) && !defined(_OS_FIRMWARE)
	SRB_IO_CONTROL Srb_Ctrl;
	MV_U32 reserved;
#endif /* _OS_WINDOWS */
        union {
            SCSI_PASS_THROUGH_DIRECT    pass;
            SCSI_PASS_THROUGH_U32            pass_u32;
        } scsi_pass_thru;
	MV_U8          Sense_info[32];
} MV_IOCTL_BUFFER_EX, *PMV_IOCTL_BUFFER_EX;
#endif
#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */

#endif /* __MV_COM_IOCTL_H__ */
