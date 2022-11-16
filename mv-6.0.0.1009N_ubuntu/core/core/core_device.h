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

#ifndef _CORE_DEVICE_H
#define _CORE_DEVICE_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"

struct _lib_device_mgr {
	domain_base      *device_map[MAX_ID];
#ifdef SUPPORT_MUL_LUN	
	MV_U16 target_id_map[MV_MAX_TARGET_NUMBER];
#endif
};

#ifdef SCSI_ID_MAP
#define CORE_IS_ID_MAPPED(req) \
	(((req->Req_Type == REQ_TYPE_CORE) \
	|| (req->Req_Type == 0) || (req->pass_id == 1)) ? (MV_FALSE) : (MV_TRUE))
#else
#define CORE_IS_ID_MAPPED(req) (MV_FALSE)
#endif

#if defined (SUPPORT_MUL_LUN) && defined (_OS_WINDOWS)
#define ID_IS_OS_TYPE(req) ((req->Req_Type == REQ_TYPE_OS) ? (MV_TRUE) : (MV_FALSE))
#else
#define ID_IS_OS_TYPE(req) (MV_FALSE)
#endif

domain_base *get_device_by_id(lib_device_mgr *lib_dev, MV_U16 id, MV_BOOLEAN mapped, MV_BOOLEAN type);
MV_U16 get_id_by_phyid(lib_device_mgr *lib_dev, MV_U16 phy_id);

#ifdef  SUPPORT_MUL_LUN
MV_U16 get_id_by_targetid_lun(MV_PVOID ext, MV_U16 id,MV_U16 lun);
MV_U16 get_device_lun(lib_device_mgr *lib_dev, MV_U16 id);
MV_U16 get_device_targetid(lib_device_mgr *lib_dev, MV_U16 id);
MV_U16 get_base_id_by_target_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun);
domain_base *get_base_by_target_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun);
#endif
#ifdef SUPPORT_BALDUR
domain_device *get_device_by_register_set(pl_root *root,
	MV_U8 register_set);
#endif
#if defined(FIX_SCSI_ID_WITH_PHY_ID)
MV_U16 get_available_dev_id_new(lib_device_mgr *lib_dev);
#endif
MV_U16 get_available_dev_id(lib_device_mgr *lib_dev);
MV_U16 get_avail_non_storage_dev_id(lib_device_mgr *lib_dev);
MV_VOID release_available_dev_id(lib_device_mgr *lib_dev, MV_U16 id);

MV_U16 add_device_map(lib_device_mgr *lib_dev, domain_base *base);
MV_VOID remove_device_map(lib_device_mgr *lib_dev, MV_U16 id);
MV_BOOLEAN change_device_map(lib_device_mgr *lib_dev, domain_base *base,
	MV_U16 old_id, MV_U16 new_id);

MV_VOID set_up_new_base(pl_root *root, domain_port *port,
	domain_base *base,
	command_handler *handler, enum base_type type, MV_U16 size);
MV_VOID set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device, command_handler *handler);
MV_VOID set_up_new_pm(pl_root *root, domain_port *port,
	domain_pm *pm);
MV_VOID set_up_new_expander(pl_root *root, domain_port *port,
	domain_expander *expander);
MV_VOID set_up_new_enclosure(pl_root *root, domain_port *port,
	domain_enclosure *enclosure,
	command_handler *handler);
#endif /* _CORE_DEVICE_H */
