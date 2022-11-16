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

#ifndef __MV_MODULE_MGMT__
#define __MV_MODULE_MGMT__

#include "com_define.h"
#include "com_type.h"
#include "com_util.h"

enum {
	/* module status (module_descriptor) */
	MV_MOD_VOID   = 0,
	MV_MOD_UNINIT,
	MV_MOD_REGISTERED,  /* module ops pointer registered */
	MV_MOD_INITED,      /* resource assigned */ 
	MV_MOD_FUNCTIONAL,
	MV_MOD_STARTED,
	MV_MOD_DEINIT,      /* extension released, be gone soon */
	MV_MOD_GONE,
};



struct mv_mod_desc;


struct mv_mod_res {
	List_Head      res_entry;
	MV_PHYSICAL_ADDR       bus_addr;
	MV_PVOID               virt_addr;
	
	MV_U32                 size;
	
	MV_U16                 type;          /* enum Resource_Type */
	MV_U16                 align;
};

typedef struct _Module_Interface
{
	MV_U8      module_id;
	MV_U32     (*get_res_desc)(enum Resource_Type type, MV_U16 maxIo);
	MV_VOID    (*module_initialize)(MV_PVOID extension,
					MV_U32   size,
					MV_U16   max_io);
	MV_VOID    (*module_start)(MV_PVOID extension);
	MV_VOID    (*module_stop)(MV_PVOID extension);
	MV_VOID    (*module_notification)(MV_PVOID extension, 
					  enum Module_Event event, 
					  struct mod_notif_param *param);
	MV_VOID    (*module_sendrequest)(MV_PVOID extension, 
					 PMV_Request pReq);
	MV_VOID    (*module_reset)(MV_PVOID extension);
	MV_VOID    (*module_monitor)(MV_PVOID extension);
	MV_BOOLEAN (*module_service_isr)(MV_PVOID extension);
#ifdef RAID_DRIVER
	MV_VOID    (*module_send_xor_request)(MV_PVOID This, 
					      PMV_XOR_Request pXORReq);
#endif /* RAID_DRIVER */
} Module_Interface, *PModule_Interface;

#define mv_module_ops _Module_Interface

#define mv_set_mod_ops(_ops, _id, _get_res_desc, _init, _start,            \
		       _stop, _send, _reset, _mon, _send_eh, _isr, _xor)   \
           {                                                               \
		   _ops->id                      = id;                     \
		   _ops->get_res_desc            = _get_res_desc;          \
		   _ops->module_initialize       = _init;                  \
		   _ops->module_start            = _start;                 \
		   _ops->module_stop             = _stop;                  \
		   _ops->module_sendrequest      = _send;                  \
		   _ops->module_reset            = _reset;                 \
		   _ops->module_monitor          = _mon;                   \
		   _ops->module_send_eh_request  = _send_eh;               \
		   _ops->module_service_isr      = _isr;                   \
		   _ops->module_send_xor_request = _xor;                   \
	   }


/* module descriptor */
struct mv_mod_desc {
	List_Head          mod_entry;      /* kept in a list */
	
	struct mv_mod_desc         *parent;
	struct mv_mod_desc         *child;

	MV_U32                     extension_size;
	MV_U8                      status;
	MV_U8                      ref_count;
	MV_U8                      module_id;
	MV_U8                      res_entry;

	MV_PVOID                   extension;      /* module extention */
	struct mv_module_ops       *ops;           /* interface operations */

	struct mv_adp_desc         *hba_desc;
	List_Head           res_list;
};

#endif /* __MV_MODULE_MGMT__ */
