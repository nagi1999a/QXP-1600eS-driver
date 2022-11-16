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

#include "mv_os.h"
#include "hba_mod.h"
#include "core_manager.h"
#include "core_protocol.h"

#if defined(MV_VMK_ESX35) || defined(MV_VMK_ESXI5)
#define do_div(n,base) ({ \
	unsigned long __upper, __low, __high, __mod; \
	asm("":"=a" (__low), "=d" (__high):"A" (n)); \
	__upper = __high; \
	if (__high) { \
	__upper = __high % (base); \
	__high = __high / (base); \
	} \
	asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (base), "0" (__low), "1" (__upper)); \
	asm("":"=A" (n):"a" (__low),"d" (__high)); \
	__mod; \
})
#endif


/* os timer function */
void ossw_init_timer(struct timer_list *timer)
{
	//timer->function = NULL;
	//init_timer(timer);
	timer_setup(timer, NULL, 0);
}

u8 ossw_add_timer(struct timer_list *timer, u32 msec, void (*function)(struct timer_list*))
{
	u64 jif;

	if(timer_pending(timer))
		del_timer(timer);

	timer->function = function;

	jif = (u64) (msec * HZ);
#ifdef __VMKLNX__
	do_div(jif, 100);         /* wait in unit of second */
#else
	do_div(jif, 1000);		   /* wait in unit of second */
#endif
	timer->expires = jiffies + 1 + jif;

	add_timer(timer);
	return	0;
}


void ossw_del_timer(struct timer_list *timer)
{
	if (timer->function)
		del_timer(timer);
	timer->function = NULL;
}

int ossw_time_expired(unsigned long time_value)
{
	return (time_before(time_value, jiffies));
}

unsigned long ossw_set_expired_time(u32 msec)
{
	return (jiffies + msecs_to_jiffies(msec));
}

/* os spin lock function */
void  ossw_local_irq_save(unsigned long *flags){ unsigned long save_flag;local_irq_save(save_flag); *flags = save_flag; }
void ossw_local_irq_restore(unsigned long *flags){unsigned long save_flag = *flags;local_irq_restore(save_flag);}
void  ossw_local_irq_disable(void){ local_irq_disable();}
void ossw_local_irq_enable(void){local_irq_enable();}
/* expect pointers */
void ossw_init_spin_lock(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock_init(plock);
}


void ossw_spin_lock(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	
	spin_lock_irqsave((spinlock_t *)ext, save_flag);
//printk("ossw_spin_lock %x %x\n",ext,save_flag);
	*flags = save_flag;
	//spin_lock_irq((spinlock_t *)ext);
#if 0
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock(plock);
#endif
}

void ossw_spin_unlock(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
//printk("ossw_spin_unlock %x %x\n",ext,save_flag);
	spin_unlock_irqrestore((spinlock_t *)ext, save_flag);
	//spin_unlock_irq((spinlock_t *)ext);
#if 0
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_unlock(plock);
#endif
}

void ossw_spin_lock_irq(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock_irq(plock);
}

void ossw_spin_unlock_irq(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_unlock_irq(plock);
}

void ossw_spin_lock_irq_save(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	//spin_lock_irqsave(plock, save_flag);
	spin_lock_irqsave((spinlock_t *)ext, save_flag);
	*flags = save_flag;
}

void ossw_spin_unlock_irq_restore(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	//spin_unlock_irqrestore(plock, save_flag);
	spin_unlock_irqrestore((spinlock_t *)ext, save_flag);
}

/* for athena16 multi-queue */
void ossw_spin_lock_hba(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hba_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
}

void ossw_spin_unlock_hba(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hba_lock;
	spin_unlock_irqrestore(plock, save_flag);
}

void ossw_spin_lock_resource(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;

	spin_lock_irqsave(&core->lib_rsrc.resource_SpinLock, save_flag);
//	printk("ossw_spin_lock_resource %x %x\n",&core->lib_rsrc.resource_SpinLock, save_flag);
	*flags = save_flag;
	//pl_root *root=&core->roots[root_id];
	//spin_lock_irq(&core->lib_rsrc.resource_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->resource_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_resource(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
//	printk("ossw_spin_unlock_resource %x %x\n",&core->lib_rsrc.resource_SpinLock, save_flag);
	spin_unlock_irqrestore(&core->lib_rsrc.resource_SpinLock, save_flag);
	//pl_root *root=&core->roots[root_id];
	//spin_unlock_irq(&core->lib_rsrc.resource_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->resource_lock;
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_wait_queue(void *ext, unsigned long *flags, unsigned int root_id)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	pl_root *root=&core->roots[root_id];
	
	spin_lock_irqsave(&root->waiting_queue_SpinLock, save_flag);
//printk("ossw_spin_lock_wait_queue %x %x\n",&root->waiting_queue_SpinLock,save_flag);
	*flags = save_flag;
	//spin_lock_irq(&root->waiting_queue_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->wait_queue_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_wait_queue(void *ext, unsigned long *flags, unsigned int root_id)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	pl_root *root=&core->roots[root_id];
//printk("ossw_spin_unlock_wait_queue %x %x\n",&root->waiting_queue_SpinLock,save_flag);
	spin_unlock_irqrestore(&root->waiting_queue_SpinLock, save_flag);
	//spin_unlock_irq(&root->waiting_queue_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->wait_queue_lock;
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_compl_queue(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;

	spin_lock_irqsave(&core->cmpl_queue_SpinLock, save_flag);
//printk("ossw_spin_lock_compl_queue %x %x\n",&core->cmpl_queue_SpinLock,save_flag);
	*flags = save_flag;
	//pl_root *root=&core->roots[root_id];
	//spin_lock_irq(&core->cmpl_queue_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->compl_queue_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_compl_queue(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
//printk("ossw_spin_unlock_compl_queue %x %x\n",&core->cmpl_queue_SpinLock,save_flag);
	spin_unlock_irqrestore(&core->cmpl_queue_SpinLock, save_flag);

	//pl_root *root=&core->roots[root_id];
	//spin_unlock_irq(&core->cmpl_queue_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->compl_queue_lock;
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_core_queue(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;

	spin_lock_irqsave(&core->core_queue_running_SpinLock, save_flag);
//printk("ossw_spin_lock_core_queue %x %x\n",&core->core_queue_running_SpinLock, save_flag);
	*flags = save_flag;
	//pl_root *root=&core->roots[root_id];
	//spin_lock_irq(&core->core_queue_running_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->core_queue_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_core_queue(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
//printk("ossw_spin_unlock_core_queue %x %x\n",&core->core_queue_running_SpinLock, save_flag);
	spin_unlock_irqrestore(&core->core_queue_running_SpinLock, save_flag);
	//pl_root *root=&core->roots[root_id];
	//spin_unlock_irq(&core->core_queue_running_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->core_queue_lock;
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_root(void *ext, unsigned long *flags, unsigned int root_id)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	pl_root *root=&core->roots[root_id];
	
	spin_lock_irqsave(&root->root_SpinLock, save_flag);
//printk("ossw_spin_lock_root %x %x\n",&root->root_SpinLock, save_flag);
	*flags = save_flag;
	//spin_lock_irq(&root->root_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->root_lock[root_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_root(void *ext, unsigned long *flags, unsigned int root_id)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	pl_root *root=&core->roots[root_id];
//printk("ossw_spin_unlock_root %x %x\n",&root->root_SpinLock, save_flag);
	spin_unlock_irqrestore(&root->root_SpinLock, save_flag);
	//spin_unlock_irq(&root->root_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->root_lock[root_id];
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_hw_queue(void *ext, unsigned long *flags, 
                                    unsigned int queue_id)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
	
	spin_lock_irqsave(&root->queues[queue_id%root->queue_num].queue_SpinLock, save_flag);
//printk("ossw_spin_lock_hw_queue %x %x\n",&root->queues[queue_id%root->queue_num].queue_SpinLock, save_flag);
	*flags = save_flag;
	//spin_lock_irq(&root->queues[queue_id%root->queue_num].queue_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_queue_lock[queue_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_hw_queue(void *ext, unsigned long *flags, 
										unsigned int queue_id)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
//printk("ossw_spin_unlock_hw_queue %x %x\n",&root->queues[queue_id%root->queue_num].queue_SpinLock, save_flag);
	spin_unlock_irqrestore(&root->queues[queue_id%root->queue_num].queue_SpinLock, save_flag);
	//spin_unlock_irq(&root->queues[queue_id%root->queue_num].queue_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_queue_lock[queue_id];
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_core(void *ext, unsigned long *flags)
{
	int i;
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;

	spin_lock_irqsave(&hba_desc->hw_queue_lock[0], save_flag);
	for (i = 1; i < MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE; i++) {
		spin_lock(&hba_desc->hw_queue_lock[i]);
	}
	spin_lock(&hba_desc->core_queue_lock);
	*flags = save_flag;
}

int ossw_spin_trylock_core(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->core_lock;
	if (spin_trylock_irqsave(plock, save_flag)) {
		*flags = save_flag;
		return 1;
	} else {
		return 0;
	}
}

void ossw_spin_unlock_core(void *ext, unsigned long *flags)
{
	
	int i;
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	//spinlock_t *plock = &hba_desc->core_lock;
	spin_unlock(&hba_desc->core_queue_lock);
	for (i = MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE - 1; i > 0; i--) {
		spin_unlock(&hba_desc->hw_queue_lock[i]);
	}
	spin_unlock_irqrestore(&hba_desc->hw_queue_lock[0], save_flag);
}

void ossw_spin_lock_hw_wq(void *ext, unsigned long *flags, 
                                    unsigned int queue_id)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
	
	spin_lock_irqsave(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
//printk("ossw_spin_lock_hw_wq %x %x\n",&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
	*flags = save_flag;
	//spin_lock_irq(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_wq_lock[queue_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_hw_wq(void *ext, unsigned long *flags, 
										unsigned int queue_id)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
//printk("ossw_spin_unlock_hw_wq %x %x\n",&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
	spin_unlock_irqrestore(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
	//spin_unlock_irq(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_wq_lock[queue_id];
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_hw_cq(void *ext, unsigned long *flags, 
                                    unsigned int queue_id)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
	
	spin_lock_irqsave(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
//printk("ossw_spin_lock_hw_cq %x %x\n",&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock,save_flag);
	*flags = save_flag;
	//spin_lock_irq(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_cq_lock[queue_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_hw_cq(void *ext, unsigned long *flags, 
										unsigned int queue_id)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	unsigned int root_id=queue_id/core->roots[0].queue_num;
	pl_root *root=&core->roots[root_id];
//printk("ossw_spin_unlock_hw_cq %x %x\n",&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock,save_flag);
	spin_unlock_irqrestore(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock, save_flag);
	//spin_unlock_irq(&root->queues[queue_id%root->queue_num].waiting_queue_SpinLock);
#if 0
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_cq_lock[queue_id];
	spin_unlock_irqrestore(plock, save_flag);
#endif
}

void ossw_spin_lock_device(void *ext, unsigned long *flags, 
                                    void *base_p)
{
	unsigned long save_flag;
	core_extension *core=(core_extension *)ext;
	domain_base *base=(domain_base *)base_p;
	
	spin_lock_irqsave(&base->base_SpinLock, save_flag);
//printk("ossw_spin_lock_device %x %x\n",&base->base_SpinLock, save_flag);
	*flags = save_flag;
	//spin_lock_irq(&base->base_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_cq_lock[queue_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}

void ossw_spin_unlock_device(void *ext, unsigned long *flags, 
                                    void *base_p)
{
	unsigned long save_flag = *flags;
	core_extension *core=(core_extension *)ext;
	domain_base *base=(domain_base *)base_p;
//printk("ossw_spin_unlock_device %x %x\n",&base->base_SpinLock, save_flag);
	spin_unlock_irqrestore(&base->base_SpinLock, save_flag);
	//spin_unlock_irq(&base->base_SpinLock);
#if 0
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->hw_cq_lock[queue_id];
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
#endif
}
/* athena16 multi-queue end */

#ifdef SUPPORT_STAGGERED_SPIN_UP
void ossw_spin_lock_irq_save_spin_up(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->device_spin_up;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
}

void ossw_spin_unlock_irq_restore_spin_up(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->device_spin_up;
	spin_unlock_irqrestore(plock, save_flag);
}
#endif

/* os get time function */
u32 ossw_get_time_in_sec(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	return (u32) ktime_get_real_seconds();
#else
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_sec;
#endif
}

u32 ossw_get_msec_of_time(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
	struct timespec64 ts;

	ktime_get_ts64(&ts);
        return (u32)ts.tv_nsec/1000*1000*1000;
#else
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_usec*1000*1000;
#endif
}

#if !defined(__VMKLNX__) && !defined(MV_VMK_ESX35)
/*kernel time.h:extern struct timezone sys_tz;*/
extern struct timezone sys_tz;
u32 ossw_get_local_time(void)
{
	static MV_U32 utc_time=0,local_time=0;
	utc_time=ossw_get_time_in_sec();
	local_time = (u32)(utc_time - (sys_tz.tz_minuteswest * 60));
	return local_time;
}
#endif

/* os bit endian function */
u16 ossw_cpu_to_le16(u16 x)  	{ return cpu_to_le16(x);}
u32 ossw_cpu_to_le32(u32 x)	{ return cpu_to_le32(x);}
u64 ossw_cpu_to_le64(u64 x)   	{ return cpu_to_le64(x);}
u16 ossw_cpu_to_be16(u16 x)      	{ return cpu_to_be16(x);}
u32 ossw_cpu_to_be32(u32 x)	{ return cpu_to_be32(x);}
u64 ossw_cpu_to_be64(u64 x)   	{ return cpu_to_be64(x);}

u16 ossw_le16_to_cpu(u16 x)      	{ return le16_to_cpu(x);}
u32 ossw_le32_to_cpu(u32 x)      	{ return le32_to_cpu(x);}
u64 ossw_le64_to_cpu(u64 x)      	{ return le64_to_cpu(x);}
u16 ossw_be16_to_cpu(u16 x)      	{ return be16_to_cpu(x);}
u32 ossw_be32_to_cpu(u32 x)      	{ return be32_to_cpu(x);}
u64 ossw_be64_to_cpu(u64 x)      	{ return be64_to_cpu(x);}

/* os map sg address function */
void *ossw_kmap(void  *sg)
{
#if !defined(CONFIG_ARM) && !defined(MV_VMK_ESXI5)
	struct scatterlist *ksg = (struct scatterlist *)sg;
	void *kvaddr = NULL;


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
		kvaddr = map_sg_page(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#else
	BUG_ON(1);
#endif	
}

void ossw_kunmap(void  *sg, void *mapped_addr)
{
#if !defined(CONFIG_ARM) && !defined(MV_VMK_ESXI5)
	struct scatterlist *ksg = (struct scatterlist *)sg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	kunmap_atomic(mapped_addr - ksg->offset);
#else
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ0);
#endif

#else
	BUG_ON(1);
#endif
}

void *ossw_kmap_sec(void  *sg)
{
#if !defined(CONFIG_ARM) && !defined(MV_VMK_ESXI5)
	struct scatterlist *ksg = (struct scatterlist *)sg;
	void *kvaddr = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
		kvaddr = map_sg_page_sec(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#else
	BUG_ON(1);
#endif
}

void ossw_kunmap_sec(void  *sg, void *mapped_addr)
{
#if !defined(CONFIG_ARM) && !defined(MV_VMK_ESXI5)
	struct scatterlist *ksg = (struct scatterlist *)sg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	kunmap_atomic(mapped_addr - ksg->offset);
#else
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ1);
#endif

#else
	BUG_ON(1);
#endif
}


#ifndef MV_VMK_ESX35
struct pci_pool *ossw_pci_pool_create( char *name, void *ext ,
	size_t size, size_t align, size_t alloc)
{
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	platform_device_t *dev;
#else
	struct pci_dev *dev;
#endif
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	MV_DASSERT(mod_desc);
	dev = mod_desc->hba_desc->dev;
	sprintf(name,"%s%d",name,mod_desc->hba_desc->id);
	return pci_pool_create(name, dev, size, align, alloc);
}

void ossw_pci_pool_destroy(struct pci_pool * pool)
{
	pci_pool_destroy(pool);
}

void *ossw_pci_pool_alloc(struct pci_pool *pool, u64 *dma_handle)
{
	return pci_pool_alloc(pool, GFP_ATOMIC, (dma_addr_t *)dma_handle);
}

void ossw_pci_pool_free(struct pci_pool *pool, void *vaddr, u64 addr)
{
	pci_pool_free(pool, vaddr, addr);
}

void * ossw_kmem_cache_create(const char *name, size_t size,
		size_t align, unsigned long flags)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	return (void *)kmem_cache_create(name, size, align, flags, NULL);
#else
	return (void *)kmem_cache_create(name, size, align, flags, NULL, NULL);
#endif
}

void * ossw_kmem_cache_alloc( void * cachep, u32 flags)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
#ifndef  __VMKLNX__
	return kmem_cache_alloc(cache, (gfp_t)flags);
#else
	return kmem_cache_alloc(cache, flags);
#endif
}

void ossw_kmem_cache_free(void *cachep, void *objp)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
	kmem_cache_free(cache, objp);
}

void ossw_kmem_cache_distroy(void *cachep)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
	kmem_cache_destroy(cache);
}

unsigned long ossw_virt_to_phys(void *address)
{
 	return virt_to_phys(address);
}

void * ossw_phys_to_virt(unsigned long address)
{
	return phys_to_virt(address);
}

#endif

/* os u64 div function */
u64 ossw_u64_div(u64 n, u64 base)
{
	do_div(n, (unsigned int)base);
	return n;
}

u64 ossw_u64_mod(u64 n, u64 base)
{
	return do_div(n, (unsigned int)base);
}


/* bit operation */
u32 ossw_rotr32(u32 v, int count)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	return ror32(v, count);
#else
	return (v << count) | (v >> (32 - count));
#endif
}

 /*ffs - normally return from 1 to MSB, if not find set bit, return 0*/
int ossw_ffz(unsigned long v)	{return (ffs(~v)-1);}
int ossw_ffs(unsigned long v)	{return (ffs(v)-1);}

void *ossw_memcpy(void *dest, const void *source, size_t len) { return memcpy(dest, source, len);}
void *ossw_memset(void *buf, int pattern, size_t len) {return memset(buf, pattern, len);}
int ossw_memcmp(const void *buf0, const void *buf1, size_t len) {return memcmp(buf0, buf1, len); }


/* os read pci config function */
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
int MV_PCI_READ_CONFIG_DWORD(void * ext, u32 offset, u32 *ptr)	{return pci_read_config_dword(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_READ_CONFIG_WORD(void * ext, u32 offset, u16 *ptr)	{return pci_read_config_word(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_READ_CONFIG_BYTE(void * ext, u32 offset, u8 *ptr) 		{return  pci_read_config_byte(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_WRITE_CONFIG_DWORD(void *ext, u32 offset, u32 val)	{return  pci_write_config_dword(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}
int MV_PCI_WRITE_CONFIG_WORD(void *ext, u32 offset, u16 val) 	{return pci_write_config_word(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}
int MV_PCI_WRITE_CONFIG_BYTE(void *ext, u32 offset, u8 val)		{return pci_write_config_byte(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}
#endif
/* System dependent macro for flushing CPU write cache */
void MV_CPU_WRITE_BUFFER_FLUSH(void) 	{smp_wmb();}
void MV_CPU_READ_BUFFER_FLUSH(void)  	{smp_rmb();}
void MV_CPU_BUFFER_FLUSH(void)       			{smp_mb();}

/* register read write: memory io */
#if defined(SUPPORT_MV_SAS_CONTROLLER)
void MV_REG_WRITE_BYTE(void *base, u32 offset, u8 val)		{smp_wmb();writeb(val, base + offset);}
void MV_REG_WRITE_WORD(void *base, u32 offset, u16 val)	{smp_wmb();writew(val, base + offset);}
void MV_REG_WRITE_DWORD(void *base, u32 offset, u32 val)   {smp_wmb();writel(val, base + offset);}
#else
void MV_REG_WRITE_BYTE(void *base, u32 offset, u8 val)		{writeb(val, base + offset);}
void MV_REG_WRITE_WORD(void *base, u32 offset, u16 val)	{writew(val, base + offset);}
void MV_REG_WRITE_DWORD(void *base, u32 offset, u32 val)   {writel(val, base + offset);}
#endif
u8 		MV_REG_READ_BYTE(void *base, u32 offset)			{return readb(base + offset);}
u16 		MV_REG_READ_WORD(void *base, u32 offset)			{return readw(base + offset);}
u32 		MV_REG_READ_DWORD(void *base, u32 offset)		{return readl(base + offset);}

/* register read write: port io */
void	MV_IO_WRITE_BYTE(void *base, u32 offset, u8 val)	{outb(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}
void MV_IO_WRITE_WORD(void *base, u32 offset, u16 val)    {outw(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}
void MV_IO_WRITE_DWORD(void *base, u32 offset, u32 val)    {outl(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}

u8	MV_IO_READ_BYTE(void *base, u32 offset)	{return inb((unsigned)(MV_PTR_INTEGER)(base + offset));}
u16	MV_IO_READ_WORD(void *base, u32 offset)	{return inw((unsigned)(MV_PTR_INTEGER)(base + offset));}
u32 	MV_IO_READ_DWORD(void *base, u32 offset)	{return inl((unsigned)(MV_PTR_INTEGER)(base + offset));}


/* os print function */
int  ossw_printk(char *fmt, ...)
{
#ifdef __VMK_LNX__
        printk(fmt, ...);
#else
	va_list args;
	static char buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return printk("%s",  buf);
#endif
}

int ossw_get_cpu_num(void)
{
	return num_online_cpus();
}

#ifdef MV_DEBUG
#ifdef _SUPPORT_64_BIT
void MV_DUMP_SP(void)
{
#ifndef __VMKLNX__
	printk("THREAD_SIZE= %d,PID =%d.\n", (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
	dump_stack();
#endif
}

#else
void MV_DUMP_SP(void )
{
#ifndef __VMKLNX__
	unsigned long sp;
#ifdef CONFIG_X86
	__asm__ __volatile__("andl %%esp,%0" :"=r" (sp) : "0" (THREAD_SIZE - 1));
	printk("SP = %ld ,THREAD_SIZE= %d,PID =%d.\n",sp, (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
#elif defined(CONFIG_PPC)
	__asm__ __volatile__("mr %0, 1":"=r"(sp));
	printk("SP = %ld ,THREAD_SIZE= %d,PID =%d.\n",sp, (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
#elif defined(CONFIG_ARM)

#else
#error "Please add the corresponding stack retrieval info."
#endif
	dump_stack();
#endif
}
#endif


/* Sleeping is disallowed if any of these macroes evalute as true*/
void MV_DUMP_CTX(void)
{
#if !defined(__VMKLNX__) && !defined(MV_VMK_ESX35)
	if( in_irq())
		printk("Present process is in hard IRQ context.\n");
	if(in_softirq())
		printk("Present process is in soft IRQ(BH) context.\n");
	if( in_interrupt())
		 printk("Present process is in hard/soft IRQ context.\n");
	if(in_atomic())
		 printk("Present process is  in preemption-disabled context .\n");
#endif
}

#endif	//#ifdef MV_DEBUG

