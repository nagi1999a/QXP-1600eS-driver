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

#ifndef __LINUX_OS_H__
#define __LINUX_OS_H__

#ifndef LINUX_VERSION_CODE
#   include <linux/version.h>
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)) && (!defined AUTOCONF_INCLUDED)
#   include <linux/config.h>
#endif

#include <linux/list.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/reboot.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>

#ifdef MV_VMK_ESX35
#include <linux/libata-compat.h>
#endif

#ifndef __VMKLNX__
#include <linux/device.h>
#include <linux/nmi.h>
#endif

#include <linux/slab.h>
#include <linux/mempool.h>
#include <linux/ctype.h>
#include "linux/hdreg.h"

#ifndef __VMKLNX__
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>
#else
#include <asm-x86_64/uaccess.h>
#include <asm-x86_64/io.h>
#include <asm-x86_64/div64.h>
#endif

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_ioctl.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
#include <scsi/scsi_request.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
#include <linux/freezer.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,6))
	#include <linux/moduleparam.h>
	#endif
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) */

/* OS specific flags */


#ifndef NULL
#   define NULL 0
#endif
#ifndef  MV_VMK_ESX35
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
#define PCI_D0 0
#include <linux/suspend.h>
typedef u32 pm_message_t;

static inline int try_to_freeze(unsigned long refrigerator_flags)
{
        if (unlikely(current->flags & PF_FREEZE)) {
               refrigerator(refrigerator_flags);
                return 1;
        } else
             return 0;
}
#endif
#endif

#define MV_INLINE inline
#define CDB_INQUIRY_EVPD    1 //TBD



/* If VER_BUILD ,the 4th bit is 0 */
#if (VER_BUILD < 1000)
#define NUM_TO_STRING(num1, num2, num3, num4) # num1"."# num2"."# num3".""0"# num4
#else
#define NUM_TO_STRING(num1, num2, num3, num4) # num1"."# num2"."# num3"."# num4
#endif
#define VER_VAR_TO_STRING(major, minor, oem, build) NUM_TO_STRING(major, \
								  minor, \
								  oem,   \
								  build)

#define mv_version_linux   VER_VAR_TO_STRING(VER_MAJOR, VER_MINOR,       \
					     VER_OEM, VER_BUILD) VER_TEST

#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#ifdef CONFIG_64BIT
#   define __KCONF_64BIT__
#endif /* CONFIG_64BIT */

#if defined(__LITTLE_ENDIAN)
#   define __MV_LITTLE_ENDIAN__  1
#elif defined(__BIG_ENDIAN)
#   define __MV_BIG_ENDIAN__     1
#else
#   error "error in endianness"
#endif

#if defined(__LITTLE_ENDIAN_BITFIELD)
#   define __MV_LITTLE_ENDIAN_BITFIELD__   1
#elif defined(__BIG_ENDIAN_BITFIELD)
#   define __MV_BIG_ENDIAN_BITFIELD__      1
#else
#   error "error in endianness"
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define mv_use_sg(cmd)	cmd->use_sg
#define mv_rq_bf(cmd)	cmd->request_buffer
#define mv_rq_bf_l(cmd)	cmd->request_bufflen
#else
#define mv_use_sg(cmd)	scsi_sg_count(cmd)
#define mv_rq_bf(cmd)	scsi_sglist(cmd)
#define mv_rq_bf_l(cmd)	scsi_bufflen(cmd)
#endif
#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#define mv_prot_use_sg(cmd)	scsi_prot_sg_count(cmd)
#define mv_prot_bf(cmd)		scsi_prot_sglist(cmd)
#define mv_prot_bf_l(cmd)	((scsi_bufflen(cmd)/512)*8)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#define map_sg_page(sg)		kmap_atomic(sg->page, KM_IRQ0)
#define map_sg_page_sec(sg)		kmap_atomic(sg->page, KM_IRQ1)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
#define map_sg_page(sg)		kmap_atomic(sg_page(sg))
#define map_sg_page_sec(sg)		kmap_atomic(sg_page(sg))
#else
#define map_sg_page(sg)		kmap_atomic(sg_page(sg), KM_IRQ0)
#define map_sg_page_sec(sg)		kmap_atomic(sg_page(sg), KM_IRQ1)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#define mv_set_resid(cmd, len)    do{ cmd->resid = len; } while(0)
#else
#define mv_set_resid(cmd, len)    scsi_set_resid(cmd, len)
#endif

struct gen_module_desc {
/* Must the first */
	struct mv_mod_desc *desc;
};
#define __ext_to_gen(_ext)       ((struct gen_module_desc *) (_ext))


/* os timer function */
void ossw_init_timer(struct timer_list *timer);
u8 ossw_add_timer(struct timer_list *timer,
		    u32 msec,
		    void (*function)(struct timer_list *t));
void ossw_del_timer(struct timer_list *timer);

int ossw_time_expired(unsigned long time_value);
unsigned long ossw_set_expired_time(u32 msec);

/* os spin lock function */
void  ossw_local_irq_save(unsigned long *flags);
void ossw_local_irq_restore(unsigned long *flags);
void  ossw_local_irq_disable(void);
void ossw_local_irq_enable(void);

/* expect pointers */
void ossw_init_spin_lock(void *ext);
void ossw_spin_lock(void *ext, unsigned long *flags);
void ossw_spin_unlock(void *ext, unsigned long *flags);
void ossw_spin_lock_irq(void *ext);
void ossw_spin_unlock_irq(void *ext);
void ossw_spin_lock_irq_save(void *ext, unsigned long *flags);
void ossw_spin_unlock_irq_restore(void *ext, unsigned long *flags);
#ifdef SUPPORT_STAGGERED_SPIN_UP
void ossw_spin_lock_irq_save_spin_up(void *ext, unsigned long *flags);
void ossw_spin_unlock_irq_restore_spin_up(void *ext, unsigned long *flags);
#endif


/* os get time function */
u32 ossw_get_time_in_sec(void);
u32 ossw_get_msec_of_time(void);
u32 ossw_get_local_time(void);

/* os kmem_cache */
#define MV_ATOMIC GFP_ATOMIC

struct pci_pool *ossw_pci_pool_create(char *name, void *ext,
	size_t size, size_t align, size_t alloc);
void ossw_pci_pool_destroy(struct pci_pool * pool);
void *ossw_pci_pool_alloc(struct pci_pool *pool, u64 *dma_handle);
void ossw_pci_pool_free(struct pci_pool *pool, void *vaddr, u64 addr);


void * ossw_kmem_cache_create(const char *, size_t, size_t, unsigned long);
void * ossw_kmem_cache_alloc( void *, unsigned);
void ossw_kmem_cache_free(void *, void *);
void ossw_kmem_cache_distroy(void *);

unsigned long ossw_virt_to_phys(void *);
void * ossw_phys_to_virt(unsigned long address);


/* os bit endian function */
u16 ossw_cpu_to_le16(u16 x);
u32 ossw_cpu_to_le32(u32 x);
u64 ossw_cpu_to_le64(u64 x);
u16 ossw_cpu_to_be16(u16 x);
u32 ossw_cpu_to_be32(u32 x);
u64 ossw_cpu_to_be64(u64 x);

u16 ossw_le16_to_cpu(u16 x);
u32 ossw_le32_to_cpu(u32 x);
u64 ossw_le64_to_cpu(u64 x) ;
u16 ossw_be16_to_cpu(u16 x);
u32 ossw_be32_to_cpu(u32 x);
u64 ossw_be64_to_cpu(u64 x) ;

/* os map sg address function */
void *ossw_kmap(void  *sg);
void ossw_kunmap(void  *sg, void *mapped_addr);
void *ossw_kmap_sec(void  *sg);
void ossw_kunmap_sec(void  *sg, void *mapped_addr);

/* MISC Services */
#define ossw_udelay(x) udelay(x)

/* os u64 div function */
u64 ossw_u64_div(u64 n, u64 base);
u64 ossw_u64_mod(u64 n, u64 base);


/* bit operation */
u32 ossw_rotr32(u32 v, int count);

int ossw_ffz(unsigned long v);
int ossw_ffs(unsigned long v);

void *ossw_memcpy(void *dest, const void *source, size_t len) ;
void *ossw_memset(void *buf, int patten, size_t len) ;
int ossw_memcmp(const void *buf0, const void *buf1, size_t len) ;

/* os read pci config function */
int MV_PCI_READ_CONFIG_DWORD(void * ext, u32 offset, u32 *ptr);
int MV_PCI_READ_CONFIG_WORD(void * ext, u32 offset, u16 *ptr);
int MV_PCI_READ_CONFIG_BYTE(void * ext, u32 offset, u8 *ptr);
int MV_PCI_WRITE_CONFIG_DWORD(void *ext, u32 offset, u32 val);
int MV_PCI_WRITE_CONFIG_WORD(void *ext, u32 offset, u16 val);
int MV_PCI_WRITE_CONFIG_BYTE(void *ext, u32 offset, u8 val);

/* System dependent macro for flushing CPU write cache */
void MV_CPU_WRITE_BUFFER_FLUSH(void);
void MV_CPU_READ_BUFFER_FLUSH(void);
void MV_CPU_BUFFER_FLUSH(void);

/* register read write: memory io */
void MV_REG_WRITE_BYTE(void *base, u32 offset, u8 val);
void MV_REG_WRITE_WORD(void *base, u32 offset, u16 val);
void MV_REG_WRITE_DWORD(void *base, u32 offset, u32 val);

u8 		MV_REG_READ_BYTE(void *base, u32 offset);
u16 		MV_REG_READ_WORD(void *base, u32 offset)	;
u32 		MV_REG_READ_DWORD(void *base, u32 offset);

/* register read write: port io */
void	MV_IO_WRITE_BYTE(void *base, u32 offset, u8 val);
void MV_IO_WRITE_WORD(void *base, u32 offset, u16 val) ;
void MV_IO_WRITE_DWORD(void *base, u32 offset, u32 val) ;

u8	MV_IO_READ_BYTE(void *base, u32 offset);
u16	MV_IO_READ_WORD(void *base, u32 offset);
u32 	MV_IO_READ_DWORD(void *base, u32 offset);


/* os print function */
int  ossw_printk(char *fmt, ...);

/* os get cpu number */
int ossw_get_cpu_num(void);

/* for multi-queue */
void ossw_spin_lock_hba(void *ext, unsigned long *flags);
void ossw_spin_unlock_hba(void *ext, unsigned long *flags);
void ossw_spin_lock_resource(void *ext, unsigned long *flags);
void ossw_spin_unlock_resource(void *ext, unsigned long *flags);
void ossw_spin_lock_wait_queue(void *ext, unsigned long *flags, unsigned int root_id);
void ossw_spin_unlock_wait_queue(void *ext, unsigned long *flags, unsigned int root_id);
void ossw_spin_lock_compl_queue(void *ext, unsigned long *flags);
void ossw_spin_unlock_compl_queue(void *ext, unsigned long *flags);
void ossw_spin_lock_core_queue(void *ext, unsigned long *flags);
void ossw_spin_unlock_core_queue(void *ext, unsigned long *flags);
void ossw_spin_lock_root(void *ext, unsigned long *flags, unsigned int root_id);
void ossw_spin_unlock_root(void *ext, unsigned long *flags, unsigned int root_id);
void ossw_spin_lock_hw_queue(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_unlock_hw_queue(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_lock_core(void *ext, unsigned long *flags);
int ossw_spin_trylock_core(void *ext, unsigned long *flags);
void ossw_spin_unlock_core(void *ext, unsigned long *flags);
void ossw_spin_lock_hw_wq(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_unlock_hw_wq(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_lock_hw_cq(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_unlock_hw_cq(void *ext, unsigned long *flags, unsigned int queue_id);
void ossw_spin_lock_device(void *ext, unsigned long *flags, void *base_p);
void ossw_spin_unlock_device(void *ext, unsigned long *flags, void *base_p);

#ifdef MV_DEBUG
void MV_DUMP_SP(void);
/* Sleeping is disallowed if any of these macroes evalute as true*/
void MV_DUMP_CTX(void);
#else
#define MV_DUMP_SP()	do{}while(0)
#define MV_DUMP_CTX()	do{}while(0)
#endif	//#ifdef MV_DEBUG

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#ifdef _SUPPORT_64_BIT
	typedef u64 resource_size_t;
 #else
	typedef u32 resource_size_t;
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
typedef struct kmem_cache kmem_cache_t;
#endif

#endif /* LINUX_OS_H */


