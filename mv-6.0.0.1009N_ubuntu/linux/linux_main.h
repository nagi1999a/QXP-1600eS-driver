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

#ifndef _LINUX_MAIN_H
#define _LINUX_MAIN_H

#include "hba_header.h"

#define __mv_get_ext_from_host(phost) \
          (((HBA_Extension **) (phost)->hostdata)[0])

/* for communication with OS/SCSI mid layer only */
enum {
	MV_MAX_REQUEST_DEPTH		 = MAX_REQUEST_NUMBER_PERFORMANCE - 2,
	MV_MAX_IO                = MAX_REQUEST_NUMBER_PERFORMANCE,
#ifdef RAID_DRIVER
	MV_MAX_REQUEST_PER_LUN   = MAX_REQUEST_NUMBER_PERFORMANCE/MAX_LD_SUPPORTED_PERFORMANCE,
	MV_MAX_SG_ENTRY          = MV_MAX_TRANSFER_SIZE/PAGE_SIZE, /*MV_MAX_TRANSFER_SIZE should be <=512Kbytes*/
	MV_SHT_USE_CLUSTERING    = DISABLE_CLUSTERING,  /*raid driver only support 4k sg entry length*/
#else /* RAID_DRIVER */
	MV_MAX_REQUEST_PER_LUN   = MAX_REQUEST_PER_LUN_PERFORMANCE,
	MV_MAX_SG_ENTRY          = SG_ALL,
#endif /* RAID_DRIVER */
	MV_MAX_IOCTL_REQUEST = 30,
	MV_SHT_EMULATED          = 0,
	MV_SHT_THIS_ID           = -1,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#define mv_scmd_host(cmd)    cmd->device->host
#define mv_scmd_channel(cmd) cmd->device->channel
#define mv_scmd_target(cmd)  cmd->device->id
#define mv_scmd_lun(cmd)     cmd->device->lun
#else
#define mv_scmd_host(cmd)    cmd->host
#define mv_scmd_channel(cmd) cmd->channel
#define mv_scmd_target(cmd)  cmd->target
#define mv_scmd_lun(cmd)     cmd->lun
#endif

#define LO_BUSADDR(x) ((MV_U32)(x))
#define HI_BUSADDR(x) (MV_U32)(sizeof(BUS_ADDRESS)>4? (u64)(x) >> 32 : 0)

struct _MV_SCP {
	MV_U16           mapped;
	MV_U16           map_atomic;
	BUS_ADDRESS bus_address;
};

#define MV_SCp(cmd) ((struct _MV_SCP *)(&((struct scsi_cmnd *)cmd)->SCp))

#ifndef scsi_to_pci_dma_dir
#define scsi_to_pci_dma_dir(scsi_dir) ((int)(scsi_dir))
#endif

#ifdef USE_REQ_POOL
void  hba_req_cache_destroy(MV_PVOID hba_ext);
int hba_req_cache_create(MV_PVOID hba_ext);
PMV_Request hba_req_cache_alloc(MV_PVOID hba_ext);
void hba_req_cache_free(MV_PVOID hba_ext,PMV_Request req) ;
#else /* USE_REQ_POOL */
struct mv_request_pool {
	List_Head free_list;
	List_Head use_list;
	void        *req_mem;  /* starting address of the request mem pool */
	void 		*sg_mem;
	spinlock_t  lock;
	MV_U32      mod_id;
	MV_U32      size;
	MV_U32      sg_count;
};

/* request structure related
 * mod_id   : id of the module that is making the request
 * size     : size of the pool, measured in the number of requests
 * sg_count : scatter gather list entries supported by each request
 */
struct mv_request_pool *res_reserve_req_pool(MV_U32 mod_id,
					     MV_U32 size,
					     MV_U32 sg_count);

struct _MV_Request *res_get_req_from_pool(struct mv_request_pool *pool);

void res_free_req_to_pool(struct mv_request_pool *pool,
			  struct _MV_Request *req);
void res_release_req_pool(struct mv_request_pool *pool);
void res_dump_pool_info(struct mv_request_pool *pool);
#endif /* !defined(USE_REQ_POOL)*/

void mv_complete_request(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
/* will wait for atomic value atomic to become zero until timed out */
/* return how much 'timeout' is left or 0 if already timed out */
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout);
#endif

#ifdef MV_BLK_IOCTL
extern void * kbuf_array[512];
extern unsigned char mvcdb[512][16];
extern int mv_new_ioctl(struct scsi_device *dev, unsigned int cmd, void __user *arg);
#endif
void hba_send_shutdown_req(MV_PVOID);

MV_U16 mv_get_register_mode(void);
#ifdef SUPPORT_MSIX_INT
void mv_free_msix_irq(struct hba_extension *hba);
#endif
#endif /*_LINUX_MAIN_H*/

