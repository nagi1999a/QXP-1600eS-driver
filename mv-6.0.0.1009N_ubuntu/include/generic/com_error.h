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

#ifndef __COM_ERROR_H__
#define __COM_ERROR_H__

#define ERR_GENERIC                              2
#define ERR_RAID                                 50
#define ERR_CORE                                 100
#define ERR_API                                  150
#define ERR_FLO									200
#define ERR_AES                                 240
#define ERR_NONE                                 0
#define ERR_FAIL                                 1
/* generic error */
#define ERR_UNKNOWN                              (ERR_GENERIC + 1)
#define ERR_NO_RESOURCE                          (ERR_GENERIC + 2)
#define ERR_REQ_OUT_OF_RANGE                     (ERR_GENERIC + 3)
#define ERR_INVALID_REQUEST                      (ERR_GENERIC + 4)
#define ERR_INVALID_PARAMETER                    (ERR_GENERIC + 5)
#define ERR_INVALID_LD_ID                        (ERR_GENERIC + 6)
#define ERR_INVALID_HD_ID                        (ERR_GENERIC + 7)
#define ERR_INVALID_EXP_ID                       (ERR_GENERIC + 8)
#define ERR_INVALID_PM_ID                        (ERR_GENERIC + 9)
#define ERR_INVALID_BLOCK_ID                     (ERR_GENERIC + 10)
#define ERR_INVALID_ADAPTER_ID                   (ERR_GENERIC + 11)
#define ERR_INVALID_RAID_MODE                    (ERR_GENERIC + 12)
#define ERR_INVALID_ENC_ID                       (ERR_GENERIC + 13)
#define ERR_INVALID_BU_ID                        (ERR_GENERIC + 14)
#define ERR_INVALID_DG_ID                        (ERR_GENERIC + 15)
#define ERR_INVALID_ENC_ELEMENT_ID               (ERR_GENERIC + 16)
#define ERR_NOT_SUPPORTED                        (ERR_GENERIC + 17)
#define ERR_DRIVER_SENSOR                        (ERR_GENERIC + 18)
#define ERR_INVALID_KEY_PRESENT                  (ERR_GENERIC + 19)	// can not add key due to key present
#define ERR_INVALID_KEY_ABSENT                   (ERR_GENERIC + 20)	// can not enable due to key absent
#define ERR_COMMAND_PHASE_ERROR                  (ERR_GENERIC + 21) // Davinci diagnostic command phase error
#define ERR_DATA_PHASE_ERROR                     (ERR_GENERIC + 22) // Davinci diagnostic data phase error
#define ERR_STATUS_PHASE_ERROR                   (ERR_GENERIC + 23) // Davinci diagnostic status phase error
#define ERR_STATUS_PHASE_PHASE_ERROR             (ERR_GENERIC + 24) // Davinci diagnostic status phase returns phase error
#define ERR_STATUS_PHASE_CMD_ERROR               (ERR_GENERIC + 25) // Davinci diagnostic status phase returns command error
#define ERR_INVALID_SSD_NUM                      (ERR_GENERIC + 26) // HyperHDD
#define ERR_INVALID_ERASE_HDD                    (ERR_GENERIC + 27) // HyperHDD
#define ERR_REGISTER_WRITING                     (ERR_GENERIC + 28) // dirty data protection
#define ERR_PASS_THROUGH_BUSY                    (ERR_GENERIC + 29) // error handle is busy, cannot send pass through command

#ifndef _MARVELL_SDK_PACKAGE_NONRAID
/* RAID errors */
#define ERR_TARGET_IN_LD_FUNCTIONAL              (ERR_RAID + 1)
#define ERR_TARGET_NO_ENOUGH_SPACE               (ERR_RAID + 2)
#define ERR_HD_IS_NOT_SPARE                      (ERR_RAID + 3)
#define ERR_HD_IS_SPARE                          (ERR_RAID + 4)
#define ERR_HD_NOT_EXIST                         (ERR_RAID + 5)
#define ERR_HD_IS_ASSIGNED_ALREADY               (ERR_RAID + 6)
#define ERR_INVALID_HD_COUNT                     (ERR_RAID + 7)
#define ERR_LD_NOT_READY                         (ERR_RAID + 8)
#define ERR_LD_NOT_EXIST                         (ERR_RAID + 9)
#define ERR_LD_IS_FUNCTIONAL                     (ERR_RAID + 10)
#define ERR_HAS_BGA_IN_VD                        (ERR_RAID + 11)
#define ERR_NO_BGA_ACTIVITY                      (ERR_RAID + 12)
#define ERR_BGA_RUNNING                          (ERR_RAID + 13)
#define ERR_RAID_NO_AVAILABLE_ID                 (ERR_RAID + 14)
#define ERR_LD_NO_ATAPI                          (ERR_RAID + 15)
#define ERR_INVALID_RAID6_PARITY_DISK_COUNT      (ERR_RAID + 16)
#define ERR_INVALID_BLOCK_SIZE                   (ERR_RAID + 17)
#define ERR_MIGRATION_NOT_NEED                   (ERR_RAID + 18)
#define ERR_STRIPE_BLOCK_SIZE_MISMATCH           (ERR_RAID + 19)
#define ERR_MIGRATION_NOT_SUPPORT                (ERR_RAID + 20)
#define ERR_LD_NOT_FULLY_INITED                  (ERR_RAID + 21)
#define ERR_LD_NAME_INVALID	                     (ERR_RAID + 22)
#define ERR_HD_TYPE_MISMATCH                     (ERR_RAID + 23)
#define ERR_HD_SECTOR_SIZE_MISMATCH              (ERR_RAID + 24)
#define ERR_NO_LD_IN_DG                          (ERR_RAID + 25)
#define ERR_HAS_LD_IN_DG                         (ERR_RAID + 26)
#define ERR_NO_ROOM_FOR_SPARE					 (ERR_RAID + 27)
#define ERR_SPARE_IS_IN_MULTI_DG				 (ERR_RAID + 28)
#define ERR_DG_HAS_MISSING_PD					 (ERR_RAID + 29)
#define ERR_LD_NOT_IMPORTABLE                    (ERR_RAID + 30)
#define ERR_HAS_MIGRATION_ON_DG					 (ERR_RAID + 31)
#define ERR_HAS_BGA_IN_DG				         (ERR_RAID + 32)
#define ERR_HD_CANNOT_SET_DOWN                   (ERR_RAID + 33)
#define ERR_HD_NOT_OFFLINE					     (ERR_RAID + 34)
#define ERR_LD_STATUS_WRONG     			     (ERR_RAID + 35)
#define ERR_LD_NOT_REPORTABLE                    (ERR_RAID + 36)
#define ERR_RAID_NOT_REDUNDANT                   (ERR_RAID + 37)
#define ERR_MP_RUNNING                           (ERR_RAID + 38)
#define ERR_SPARE_IS_IN_USED                      (ERR_RAID + 39)
#endif
//_MARVELL_SDK_PACKAGE_NONRAID

/* API errors */
#define ERR_INVALID_MATCH_ID                     (ERR_API + 1)
#define ERR_INVALID_HDCOUNT                      (ERR_API + 2)
#define ERR_INVALID_BGA_ACTION                   (ERR_API + 3)
#define ERR_HD_IN_DIFF_CARD                      (ERR_API + 4)
#define ERR_INVALID_FLASH_TYPE                   (ERR_API + 5)
#define ERR_INVALID_FLASH_ACTION                 (ERR_API + 6)
#define ERR_TOO_FEW_EVENT                        (ERR_API + 7)
#define ERR_VD_HAS_RUNNING_OS                    (ERR_API + 8)
#define ERR_DISK_HAS_RUNNING_OS                  (ERR_API + 9)
#define ERR_COMMAND_NOT_SUPPORTED                (ERR_API + 10)
#define ERR_MIGRATION_LIMIT	                     (ERR_API + 11)  // not used
#define ERR_SGPIO_CONTROL_NOT_SUPPORTED			 (ERR_API + 12)
#define ERR_COUNT_OUT_OF_RANGE      			 (ERR_API + 13)	 // internal error
#define ERR_IOCTL_NO_RESOURCE                    (ERR_API + 14)
#define ERR_INVALID_FILE                         (ERR_API + 15)
#define ERR_INVALID_MICROCODE                    (ERR_API + 16)
#define ERR_USER_NOT_FOUND 				         (ERR_API+17)
#define ERR_USER_NOT_INUSE   			         (ERR_API+18)
#define ERR_USER_INUSE		 			         (ERR_API+19)
#define ERR_DEVICE_IS_BUSY				         (ERR_API+31)
#define ERR_SHELL_CMD_FAIL				         (ERR_API+32)
#define ERR_LOAD_LOKI_API_FAIL			         (ERR_API+33)
#define ERR_INVALID_FLASH_DATA			         (ERR_API+34)
#define ERR_INVALID_FLASH_DESCRIPTOR	         (ERR_API+35)
#define ERR_NEED_RESCAN                          (ERR_API+36)
#define ERR_RESCANING                            (ERR_API+37)
#define ERR_NOT_SUPPORT_MIGRATE                  (ERR_API+38)

/* AES error */
#define ERR_ENTRY_OUT_OF_RANGE                  (ERR_AES + 1)
#define ERR_PORT_OUT_OF_RANGE                   (ERR_AES + 2)
#define ERR_INVALID_NUM_REQUESTED               (ERR_AES + 3)
#define ERR_INVALID_REQUEST_TYPE                (ERR_AES + 4)
#define ERR_INVALID_KEY_LENGTH                  (ERR_AES + 5)
#define ERR_NOT_OFFLINE_DISK                    (ERR_AES + 6)
#define ERR_KEY_MISMATCH                        (ERR_AES + 7)
#define ERR_PASSWORD_MISMATCH                   (ERR_AES + 8)
#define ERR_PORT_ID_NOT_FOUND                   (ERR_AES + 9)
#define ERR_ENTRY_NO_KEY                        (ERR_AES + 10) // the specified entry has no key.

#endif /*  __COM_ERROR_H__ */
