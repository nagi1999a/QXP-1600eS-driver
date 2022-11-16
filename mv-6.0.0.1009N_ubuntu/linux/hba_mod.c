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

#include "hba_header.h"
#include "linux_main.h"
#include "linux_iface.h"
#include "hba_mod.h"
#include "hba_timer.h"
#include "hba_api.h"
#if defined(SUPPORT_MODULE_CONSOLIDATE)
#include "cc.h"
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

static MV_LIST_HEAD(mv_online_adapter_list);

int __mv_get_adapter_count(void)
{
	struct mv_adp_desc *p;
	int i = 0;
	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
	i++;

	return i;
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
struct mv_adp_desc *__dev_to_desc(platform_device_t *dev)
#else
struct mv_adp_desc *__dev_to_desc(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *p;

	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
	if (p->dev == dev)
		return p;
	return NULL;
}

MV_PVOID *mv_get_hba_extension(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	LIST_FOR_EACH_ENTRY(p, &hba_desc->online_module_list, mod_entry)
		if (MODULE_HBA == p->module_id)
			return p->extension;
	return NULL;
}

static inline void __mv_release_hba(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *p;

	LIST_FOR_EACH_ENTRY_SAFE(mod_desc,
				p,
				&hba_desc->online_module_list,
				mod_entry) {
		List_Del(&mod_desc->mod_entry);
		hba_mem_free(mod_desc,sizeof(struct mv_mod_desc),MV_FALSE);
	}

	List_Del(&hba_desc->hba_entry);
	hba_mem_free(hba_desc,sizeof(struct mv_adp_desc),MV_FALSE);
}

#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
extern  struct mv_adp_desc *gl_hba_desc;
#endif
#if defined(SUPPORT_MV_SAS_CONTROLLER)
static struct mv_adp_desc *mv_hba_init_modmm(platform_device_t *dev)
#else
static struct mv_adp_desc *mv_hba_init_modmm(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *hba_desc;

	hba_desc = hba_mem_alloc(sizeof(struct mv_adp_desc),MV_FALSE);
	if (NULL == hba_desc) {
		MV_PRINT("Unable to get memory at hba init.\n");
		return NULL;
	}
#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
	gl_hba_desc = hba_desc;
#endif
	memset(hba_desc, 0, sizeof(struct mv_adp_desc));
	hba_desc->dev = dev;
	MV_LIST_HEAD_INIT(&hba_desc->online_module_list);
	List_Add(&hba_desc->hba_entry, &mv_online_adapter_list);

	return hba_desc;
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
static void mv_hba_release_modmm(platform_device_t *dev)
#else
static void mv_hba_release_modmm(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *hba_desc;

	hba_desc = __dev_to_desc(dev);

	if (hba_desc)
		__mv_release_hba(hba_desc);
	else
		MV_PRINT("Weired! dev %p unassociated with any desc.\n", dev);
}

static inline struct mv_mod_desc *__alloc_mod_desc(void)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = hba_mem_alloc(sizeof(struct mv_mod_desc),MV_FALSE);
	if (mod_desc)
		memset(mod_desc, 0, sizeof(struct mv_mod_desc));
	return mod_desc;
}

struct mv_module_ops *mv_hba_register_module(void)
{
	static struct mv_module_ops hba_module_interface = {
		.module_id              = MODULE_HBA,
		.get_res_desc           = HBA_ModuleGetResourceQuota,
		.module_initialize      = HBA_ModuleInitialize,
		.module_start           = HBA_ModuleStart,
		.module_stop            = HBA_ModuleShutdown,
		.module_notification    = HBA_ModuleNotification,
		.module_sendrequest     = HBA_ModuleSendRequest,
	};

	return &hba_module_interface;
}


static int register_online_modules(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *prev;
	struct mv_module_ops *ops;

	/*
	 * iterate through online_module_list manually , from the lowest(CORE)
	 * to the highest layer (HBA)
	 */
	hba_desc->running_mod_num = 0;
	/* CORE */
	ops = mv_core_register_module();
	if (NULL == ops) {
		MV_PRINT("No core no life.\n");
		return -1;
	}
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->hba_desc	= hba_desc;
	mod_desc->ops		= ops;
	mod_desc->status	= MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_CORE;
	mod_desc->child 	= NULL;
	List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;

	/* when running in non-RAID, both CACHE and RAID must be disabled */
	if(!hba_desc->RunAsNonRAID)
	{
#ifdef RAID_DRIVER
	/* RAID */
	ops = mv_raid_register_module();
	if (ops) {
		prev = mod_desc;
		mod_desc = __alloc_mod_desc();
		if (NULL == mod_desc)
			goto disaster;

		mod_desc->hba_desc	= hba_desc;
		mod_desc->ops		= ops;
		mod_desc->status	= MV_MOD_REGISTERED;
		mod_desc->module_id = MODULE_RAID;
		mod_desc->child 	= prev;
		prev->parent		= mod_desc;
		List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
		hba_desc->running_mod_num++;
	}
#endif

#ifdef CACHE_MODULE_SUPPORT
	/* CACHE */
	ops = mv_cache_register_module();
	if (ops) {
		prev = mod_desc;
		mod_desc = __alloc_mod_desc();
		if (NULL == mod_desc)
			goto disaster;

		mod_desc->hba_desc	= hba_desc;
		mod_desc->ops		= ops;
		mod_desc->status	= MV_MOD_REGISTERED;
		mod_desc->module_id = MODULE_CACHE;
		mod_desc->child 	= prev;
		prev->parent		= mod_desc;
		List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
		hba_desc->running_mod_num++;
	}
#endif

	}

	/* HBA */
	prev = mod_desc;
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->ops = mv_hba_register_module();
	if (NULL == mod_desc->ops) {
		MV_PRINT("No HBA no life.\n");
		return -1;
	}

	mod_desc->hba_desc	= hba_desc;
	mod_desc->status	= MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_HBA;
	mod_desc->child 	= prev;
	mod_desc->parent	= NULL;
	prev->parent		= mod_desc;
	List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;

	return 0;
disaster:
	return -1;

}

#if defined(SUPPORT_MV_SAS_CONTROLLER)
static void __release_consistent_mem(struct mv_mod_res *mod_res,
				     platform_device_t *dev)
#else
static void __release_consistent_mem(struct mv_mod_res *mod_res,
				     struct pci_dev *dev)
#endif
{
	dma_addr_t       dma_addr;
	MV_PHYSICAL_ADDR phy_addr;

	phy_addr = mod_res->bus_addr;
	dma_addr = (dma_addr_t) ((u64)phy_addr.parts.low |
				 ((u64) phy_addr.parts.high << 32));
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	dma_free_coherent(&dev->dev,
			    mod_res->size,
			    mod_res->virt_addr,
			    dma_addr);
#else
	pci_free_consistent(dev,
			    mod_res->size,
			    mod_res->virt_addr,
			    dma_addr);
#endif
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  platform_device_t *dev)
#else
int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  struct pci_dev *dev)
#endif
{
	unsigned long size;
	dma_addr_t    dma_addr;
	BUS_ADDRESS   bus_addr;
	MV_PHYSICAL_ADDR phy_addr;

	size = mod_res->size;
	size = ROUNDING(size, 8);
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	mod_res->virt_addr = (MV_PVOID) dma_alloc_coherent(&dev->dev,
							     size,
							     &dma_addr,
							     GFP_ATOMIC);
#else
	mod_res->virt_addr = (MV_PVOID) pci_alloc_consistent(dev,
							     size,
							     &dma_addr);
#endif

	if (NULL == mod_res->virt_addr) {
		MV_DPRINT(("unable to alloc 0x%lx consistent mem.\n",
		       size));
		return -1;
	}

	memset(mod_res->virt_addr, 0, size);
	bus_addr            = (BUS_ADDRESS) dma_addr;
	phy_addr.parts.low  = LO_BUSADDR(bus_addr);
	phy_addr.parts.high = HI_BUSADDR(bus_addr);
	mod_res->bus_addr   = phy_addr;
    
	return 0;
}



static void __release_resource(struct mv_adp_desc *hba_desc,
			       struct mv_mod_desc *mod_desc)
{
	struct mv_mod_res *mod_res, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(mod_res,
				tmp,
				&mod_desc->res_list,
				res_entry) {
		switch (mod_res->type) {
		case RESOURCE_UNCACHED_MEMORY :
			__release_consistent_mem(mod_res, hba_desc->dev);
			break;
		case RESOURCE_CACHED_MEMORY :
			hba_mem_free(mod_res->virt_addr,mod_res->size,MV_FALSE);
			break;
		default:
			MV_DPRINT(("res type %d unknown.\n",
			       mod_res->type));
			break;
		}
		List_Del(&mod_res->res_entry);
		hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
	}
}

static void __release_module_resource(struct mv_mod_desc *mod_desc)
{
	__release_resource(mod_desc->hba_desc, mod_desc);
}

static int __alloc_module_resource(struct mv_mod_desc *mod_desc,
				   unsigned int max_io)
{
	struct mv_mod_res *mod_res = NULL;
	unsigned int size = 0;

	/*
	 * alloc only cached mem at this stage, uncached mem will be alloc'ed
	 * during mod init.
	 */
	MV_LIST_HEAD_INIT(&mod_desc->res_list);
	mod_res = hba_mem_alloc(sizeof(struct mv_mod_res),MV_FALSE);
	if (NULL == mod_res)
		return -1;
	memset(mod_res, 0, sizeof(sizeof(struct mv_mod_res)));
	mod_desc->res_entry = 1;

	size = mod_desc->ops->get_res_desc(RESOURCE_CACHED_MEMORY, max_io);
	if (size) {
		mod_res->virt_addr = hba_mem_alloc(size,MV_FALSE);
		if (NULL == mod_res->virt_addr) {
			hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
			MV_DASSERT(MV_FALSE);
			return -1;
		}
		memset(mod_res->virt_addr, 0, size);
		mod_res->type                = RESOURCE_CACHED_MEMORY;
		mod_res->size                = size;
		mod_desc->extension          = mod_res->virt_addr;
		mod_desc->extension_size     = size;
		List_Add(&mod_res->res_entry, &mod_desc->res_list);
	}
	MV_DPRINT(("show module id[%d] cached size[0x%x], addr[0x%p].\n",mod_desc->module_id,size,mod_res->virt_addr));

	return 0;
}

static void mv_release_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
}

static int mv_alloc_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;
	int ret;

	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		ret = __alloc_module_resource(mod_desc, hba_desc->max_io);
		if (ret)
			goto err_out;
		mod_desc->status = MV_MOD_INITED;
		__ext_to_gen(mod_desc->extension)->desc = mod_desc;
	}
	return 0;

err_out:
	MV_DPRINT(("error %d allocating resource for mod %d.\n",
	       ret, mod_desc->module_id));
	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
	return -1;
}

struct mv_mod_desc * __get_lowest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	/* take a random module, and trace through its child */
	p = LIST_ENTRY(hba_desc->online_module_list.next,
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->child)
			break;
		p = p->child;
	}
	return p;
}

struct mv_mod_desc * __get_highest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	/* take a random module, and trace through its parent */
	p = LIST_ENTRY(hba_desc->online_module_list.next,
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->parent)
			break;
		p = p->parent;
	}
	return p;
}
#if 0//defined(SUPPORT_MV_SAS_CONTROLLER)
#define SP2_REGS_PHYS_BASE			0xE0000000
#define SP2_REGS_VIRT_BASE			0xFD000000
#define SP2_XOR0_OFFSET				(0xE00000)
#define SP2_XOR1_OFFSET				(0xE08000)
#define SP2_SAS0_OFFSET				(0x1000000)
#define SP2_SAS0_SGPIO0_OFFSET		(0x107C000)
#define SP2_SAS0_SGPIO1_OFFSET		(0x107E000)
#define SP2_SAS1_OFFSET				(0x2000000)
#define SP2_SAS1_SGPIO0_OFFSET		(0x207C000)
#define SP2_SAS1_SGPIO1_OFFSET		(0x207E000)
#define SP2_SAS2_OFFSET				(0x3000000)
#define SP2_SAS2_SGPIO0_OFFSET		(0x307C000)
#define SP2_SAS2_SGPIO1_OFFSET		(0x307E000)

#define SP2_SAS0_PHYS_BASE	(SP2_REGS_PHYS_BASE + SP2_SAS0_OFFSET)
#define SP2_SAS1_PHYS_BASE	(SP2_REGS_PHYS_BASE + SP2_SAS1_OFFSET)
#define SP2_SAS2_PHYS_BASE	(SP2_REGS_PHYS_BASE + SP2_SAS2_OFFSET)

#define SP2_SAS0_VIRT_BASE	(SP2_REGS_VIRT_BASE + SP2_SAS0_OFFSET)
#define SP2_SAS1_VIRT_BASE	(SP2_REGS_VIRT_BASE + SP2_SAS1_OFFSET)
#define SP2_SAS1_VIRT_BASE	(SP2_REGS_VIRT_BASE + SP2_SAS2_OFFSET)
#endif
#if defined(SUPPORT_MV_SAS_CONTROLLER)
static int __map_pci_addr(platform_device_t *dev, MV_PVOID *addr_array)
#else
static void __map_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
#endif
{
#if defined(SUPPORT_MV_SAS_CONTROLLER)
    struct resource *res;

    memset(addr_array, 0, sizeof(MV_PVOID)*MAX_BASE_ADDRESS);

    res = platform_get_resource(dev, IORESOURCE_MEM, 0);
//printk(KERN_INFO "###%s, map pci addr=%p sz=%p###\n", __FUNCTION__, res->start, resource_size(res));
    if (res == NULL)
        goto err_out;

#if 0
//printk(KERN_INFO "###%s, physical sata mmio addr=%p sz=%p###\n", __FUNCTION__, DRAGON_SATA0_PHYS_BASE, 0x4000);
    addr_array[2/*MV_PCI_BAR*/] = (MV_PVOID) ioremap_nocache(DRAGON_SATA0_PHYS_BASE, 0x4000);
//printk(KERN_INFO "###%s, virtual sata mmio addr=%p sz=%p###\n", __FUNCTION__, addr_array[2/*MV_PCI_BAR*/], 0x4000);
#else
    addr_array[2/*MV_PCI_BAR*/] = (MV_PVOID) ioremap_nocache(res->start, resource_size(res));
#endif

//printk(KERN_INFO "###%s, addr_array[MV_PCI_BAR]=%p###\n", __FUNCTION__, addr_array[2/*MV_PCI_BAR*/]);

    if (!addr_array[2/*MV_PCI_BAR*/])
        goto err_out;
#if defined(SUPPORT_MV_DMA_CONTROLLER)
    addr_array[MV_DMA_CORE_BASE_ADDR_INDEX/*DMA*/] = (MV_PVOID) ioremap_nocache(0xE0210000, 0x3FFF);
    MV_PRINT("addr_array[%d]=%p len 0x%x\n",MV_DMA_CORE_BASE_ADDR_INDEX, addr_array[MV_DMA_CORE_BASE_ADDR_INDEX/*DMA*/], 0x3FFF);
    if (!addr_array[MV_DMA_CORE_BASE_ADDR_INDEX])
        goto err_out;
#endif
#if defined(HARDWARE_XOR)
    addr_array[MV_XOR_CORE_BASE_ADDR_INDEX/*XOR*/] = (MV_PVOID) ioremap_nocache(0xE0200000, 0xBFFF);
    MV_PRINT("addr_array[%d]=%p len 0x%x\n",MV_XOR_CORE_BASE_ADDR_INDEX, addr_array[MV_XOR_CORE_BASE_ADDR_INDEX/*XOR*/], 0x3FFF);
    if (!addr_array[MV_XOR_CORE_BASE_ADDR_INDEX])
        goto err_out;
#endif
    return 0;

err_out:
    return -1;

#else
	int i;
	resource_size_t addr;
	resource_size_t range;
	for (i = 0; i < MAX_BASE_ADDRESS; i++) {
		addr  = pci_resource_start(dev, i);
		range = pci_resource_len(dev, i);

		if (pci_resource_flags(dev, i) & IORESOURCE_MEM){
			addr_array[i] =(MV_PVOID) ioremap(addr, (unsigned long)range);
		}
		else{
			addr_array[i] = (MV_PVOID)((unsigned long)addr);
		}
		MV_DPRINT(( "%s : BAR %d : %p.\n", mv_product_name,
		       i, addr_array[i]));
	}
#endif
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
static void __unmap_pci_addr(platform_device_t *dev, MV_PVOID *addr_array)
#else
static void __unmap_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
#endif
{
#if defined(SUPPORT_MV_SAS_CONTROLLER)
    iounmap(addr_array[2/*MV_PCI_BAR*/]);
#if defined(SUPPORT_MV_DMA_CONTROLLER)
    iounmap(addr_array[MV_DMA_CORE_BASE_ADDR_INDEX/*DMA*/]);
#endif
#if defined(HARDWARE_XOR)
    iounmap(addr_array[MV_XOR_CORE_BASE_ADDR_INDEX/*XOR*/]);
#endif

#else
	int i;

	for (i = 0; i < MAX_BASE_ADDRESS; i++)
		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
                        iounmap(addr_array[i]);
#endif
}

int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = __get_lowest_module(adp_desc);

	while (mod_desc) {
		if (MV_MOD_STARTED != mod_desc->status)
			return 0;

		mod_desc = mod_desc->parent;
	}
	return 1;
}

static void __mv_save_hba_configuration(struct mv_adp_desc *hba_desc, void *hba_ext)
{
	u8 i;
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	phba->Vendor_Id = hba_desc->Vendor_Id;
	phba->Device_Id = hba_desc->Device_Id ;
	phba->Revision_Id = hba_desc->Revision_Id;
	phba->Sub_Vendor_Id = hba_desc->Sub_Vendor_Id;
	phba->Sub_System_Id = hba_desc->Sub_System_Id;
	phba->pcie_max_lnk_spd = hba_desc->pcie_max_lnk_spd;
	phba->pcie_max_bus_wdth = hba_desc->pcie_max_bus_wdth;
	phba->pcie_neg_lnk_spd = hba_desc->pcie_neg_lnk_spd;
	phba->pcie_neg_bus_wdth = hba_desc->pcie_neg_bus_wdth;
	for (i = 0;i < MAX_BASE_ADDRESS; i++)
		phba->Base_Address[i] = hba_desc->Base_Address[i];

	MV_DPRINT(( "HBA device id 0x%x, RunAsNonRAID:%x.\n", phba->Device_Id, phba->RunAsNonRAID));

}

static void __hba_module_stop(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		return;

	/* starting from highest module, unlike module_start */
	while (mod_desc) {
		if (MV_MOD_STARTED == mod_desc->status) {
			mod_desc->ops->module_stop(mod_desc->extension);
			mod_desc->status = MV_MOD_INITED;
		}
		mod_desc = mod_desc->child;
	}
}

struct hba_extension *__mv_get_ext_from_adp_id(int id)
{
	struct mv_adp_desc *p;

	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
		if (p->id == id)
			return __get_highest_module(p)->extension;

	return NULL;
}
#ifdef SUPPORT_STAGGERED_SPIN_UP
extern MV_U8 enable_spin_up(MV_PVOID hba);
#endif
#if defined(SUPPORT_MV_SAS_CONTROLLER)
int mv_hba_start(platform_device_t *dev)
#else
int mv_hba_start(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *hba_desc = __dev_to_desc(dev);
	struct mv_mod_desc *mod_desc;
	struct hba_extension *hba;
	unsigned long flags;
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	struct resource *res_irq;
	res_irq = platform_get_resource(hba_desc->dev, IORESOURCE_IRQ, 0);
#endif
	/* YC 1. HBA module setup IRQ and so on. */
	if(NULL == (mod_desc = __get_highest_module(hba_desc)))
		return -1;

	mod_desc->ops->module_start(mod_desc->extension);
	hba = (struct hba_extension *)mod_desc->extension;
	if(hba_desc->hba_host == NULL){
		MV_DPRINT(("Start highest module failed.\n"));
		return	-1;
	}
#ifndef SUPPORT_STAGGERED_SPIN_UP
	hba->first_scan = 1;
#endif

	HBA_GetNextModuleSendFunction(hba, &hba->pNextExtension, &hba->pNextFunction);
	RAID_Feature_SetSendFunction(hba->p_raid_feature, hba, hba->pNextExtension, hba->pNextFunction);
#ifdef SUPPORT_MODULE_CONSOLIDATE
    	ModConsolid_SetSendFunction(hba->PCC_Extension, hba, hba->pNextExtension, hba->pNextFunction);
	/* Initialize Consolidate device */
	ModConsolid_InitConsDev(hba);
#endif


#ifdef SUPPORT_TIMER
	ossw_add_timer(&hba->desc->hba_desc->hba_timer, TIMER_INTERVAL_OS, MV_Timer_CheckRequest);
#endif

	/* YC 2. Core Module SAS/SATA initializing */
	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba_desc->hba_sync, 1);
#endif

	mod_desc->ops->module_start(mod_desc->extension);

	/* YC 3.  Set HBA module started status */
	hba->desc->status = MV_MOD_STARTED;
	HBA_ModuleStarted(hba);
	hba_house_keeper_run();

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	/* Fixed issue --- reboot failed on SUSE platform.
                      Increase wait time to 1000 seconds */
	if (0 == __hba_wait_for_atomic_timeout(&hba_desc->hba_sync, 1000 * HZ))
		goto err_wait_cmpl;
#else
	if (0 == wait_for_completion_timeout(&hba_desc->cmpl, 1000 * HZ))
		goto err_wait_cmpl;
#endif

#ifndef MV_VMK_ESX35
#ifdef MV_VMK_ESXI5
		memcpy(hba_desc->hba_host->name, mv_esxi_apt_name, 12);
		hba_desc->hba_host->useDriverNamingDevice = 1;
#endif
	
#if defined(SUPPORT_DIF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	if (mv_prot_mask) {
#ifdef SUPPORT_DIX
		if (mv_prot_guard) {
			scsi_host_set_guard(hba_desc->hba_host, mv_prot_guard);
			mv_prot_mask |= (SHOST_DIX_TYPE1_PROTECTION | SHOST_DIX_TYPE2_PROTECTION 
					| SHOST_DIX_TYPE3_PROTECTION);
		}
#endif
		/*
		* This must be done before scsi_add_host():
		* with scsi_mod.use_blk_mq=1 scsi_add_host uses the
		* scsi_host_get_prot() in scsi_mq_setup_tags() to calculate
		* the buffer size, and then blk_mq_init_rq_map() allocates the buffers.
		* If scsi_host_set_prot is done after this, the buffers
		* are too small and memory is corrupted in scsi_lib.c:scsi_mq_prep_fn(),
		* which based on scsi_host_get_prot() populates prot_sdb after sg,
		* while buffers do not have space for it.
		*/
		scsi_host_set_prot(hba_desc->hba_host, mv_prot_mask);
	}
#endif

	if (scsi_add_host(hba_desc->hba_host, &hba_desc->dev->dev))
		goto err_wait_cmpl;

#ifdef SUPPORT_STAGGERED_SPIN_UP
	if(!enable_spin_up(hba)) {
#endif
	MV_PRINT("Start scsi_scan_host.\n");
	scsi_scan_host(hba_desc->hba_host);

	/* after scsi_scan_host, enable hotplug process switch */
	hba->first_scan = 0;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	}
#endif
#endif


#ifndef MV_BLK_IOCTL
	if (mv_register_chdev(hba))
		MV_PRINT("Unable to register character device interface.\n");
#endif
	MV_DPRINT(("Finished Driver Initialization.\n"));

	return 0;

err_wait_cmpl:
	MV_PRINT("Timeout waiting for module start.\n");
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	free_irq(res_irq->start, hba);
	free_irq(res_irq->end, hba);
#else
	free_irq(hba_desc->dev->irq, hba);
#endif
#if defined(HARDWARE_XOR)
    free_irq(IRQ_DRAGON_XOR0, hba);
    free_irq(IRQ_DRAGON_XOR1, hba);
#endif
	return -1;
}


/* stop all HBAs if dev == NULL */
#if defined(SUPPORT_MV_SAS_CONTROLLER)
void mv_hba_stop(platform_device_t *dev)
#else
void mv_hba_stop(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *hba_desc;
	MV_DPRINT(("mv_hba_stop: before hba_house_keeper_exit\n"));
	hba_house_keeper_exit();

	if (dev) {
		hba_desc = __dev_to_desc(dev);
		__hba_module_stop(hba_desc);
	} else {
		list_for_each_entry(hba_desc, &mv_online_adapter_list, hba_entry)
			__hba_module_stop(hba_desc);
	}
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
void mv_hba_release(platform_device_t *dev)
#else
void mv_hba_release(struct pci_dev *dev)
#endif
{
	struct mv_adp_desc *hba_desc;

	hba_desc = __dev_to_desc(dev);
	MV_DPRINT(("mv_hba_release\n"));
	if (NULL != hba_desc) {
		MV_DPRINT(("NULL != hba_desc\n"));
		__unmap_pci_addr(hba_desc->dev, hba_desc->Base_Address);
		mv_release_module_resource(hba_desc);
		mv_hba_release_modmm(hba_desc->dev);
	}
}

#ifdef RAID_DRIVER
void mv_get_hba_page_info( MV_PVOID This, MV_U16 Device_Id, MV_U8 *NonRaid);
#endif

#ifdef PRODUCTNAME_ODIN
MV_U16 core_set_device_id(MV_U32 pad_test);
#endif
#if defined(SUPPORT_MV_SAS_CONTROLLER)
int mv_hba_init(platform_device_t *dev, MV_U32 max_io)
#else
int mv_hba_init(struct pci_dev *dev, MV_U32 max_io)
#endif
{
	struct mv_adp_desc *hba_desc;
	struct mv_mod_desc *mod_desc;
	PHBA_Extension phba = NULL ;
	MV_U32 tmp1 = 0, tmp2 = 0, i;

	int    dbg_ret = 0;
	hba_desc = mv_hba_init_modmm(dev);
	if (NULL == hba_desc)
		goto ext_err_init;
	hba_desc->dev = dev;
	hba_desc->max_io = max_io;
	hba_desc->id     = __mv_get_adapter_count() - 1;
	MV_DPRINT(("hba_desc->id=%d.\n", hba_desc->id));

#if defined(SUPPORT_MV_SAS_CONTROLLER)
	hba_desc->Vendor_Id = VENDOR_ID_EXT;
	hba_desc->Device_Id = DEVICE_ID_1580;
	hba_desc->Sub_Vendor_Id = VENDOR_ID_EXT;
	hba_desc->Sub_System_Id = DEVICE_ID_1580;
	//hba_desc->Revision_Id = 0xC2; /*VANIR_C2_REV*/
	//hba_desc->Revision_Id = 0xC3; /*VANIR_C3_REV*/
	hba_desc->Revision_Id =  0x01; /*VANIR_B0_REV*/
	MV_PRINT("%s : HBA ID=%d Get hba's revision id=%x.\n",
	       mv_product_name, hba_desc->id, hba_desc->Revision_Id);
#else
	if (pci_read_config_byte(hba_desc->dev,
				 PCI_REVISION_ID,
				 &hba_desc->Revision_Id)) {
		MV_PRINT("%s : Failed to get hba's revision id.\n",
		       mv_product_name);
		goto ext_err_pci;
	}

	hba_desc->Vendor_Id = dev->vendor;
	hba_desc->Device_Id = dev->device;
	hba_desc->Sub_Vendor_Id = dev->subsystem_vendor;
	hba_desc->Sub_System_Id = dev->subsystem_device;
#endif
	MV_DPRINT(("original device id=%04X.\n",hba_desc->Device_Id));
	/* WORKAROUND: Odin 2 was released with Device ID set to 6440, and
	   Sub Device ID set to 6480. Supposedly both should be 6480.
	   We correct this here. */
	if (hba_desc->Device_Id == DEVICE_ID_6440) {
		if (hba_desc->Sub_System_Id == DEVICE_ID_6480)
			hba_desc->Device_Id = DEVICE_ID_6480;
	}
	__map_pci_addr(dev, hba_desc->Base_Address);

#if !defined(SUPPORT_MV_SAS_CONTROLLER)
	/* PCIe Link Speed and Bus Width */
#if defined( PRODUCTNAME_ODIN) || defined(PRODUCTNAME_THOR)
	pci_read_config_dword(dev, 0xec, &tmp1);
	pci_read_config_dword(dev, 0xf0, &tmp2);
#elif defined(PRODUCTNAME_VANIR) || defined(PRODUCTNAME_VANIRLITES)
	pci_read_config_dword(dev, 0x7c, &tmp1);;
	pci_read_config_dword(dev, 0x80, &tmp2);
#elif defined(PRODUCTNAME_ATHENA)
	pci_read_config_dword(dev, 0xcc, &tmp1);
	pci_read_config_dword(dev, 0xd0, &tmp2);
#endif
	hba_desc->pcie_max_lnk_spd = (MV_U8)(tmp1 & 0x0F);
	hba_desc->pcie_max_bus_wdth = (MV_U8)((tmp1 >> 4) & 0x3F);
	hba_desc->pcie_neg_lnk_spd = (MV_U8)((tmp2 >> 16) & 0x0F);
	hba_desc->pcie_neg_bus_wdth = (MV_U8)((tmp2 >> 20) & 0x3F);

#ifdef PRODUCTNAME_ODIN
	if ((hba_desc->Device_Id != DEVICE_ID_6480)&&( hba_desc->Device_Id != DEVICE_ID_6485)) {
		MV_U32 padTest = 0;
		pci_read_config_dword(dev, 0x60, &padTest);
		hba_desc->Device_Id = core_set_device_id(padTest);
		if (hba_desc->Device_Id == DEVICE_ID_6445)
			hba_desc->RunAsNonRAID = MV_TRUE;

	}
#endif
#endif
		/*Read RAID  information saved in Flash/NVRAM*/
#ifdef RAID_DRIVER
	mv_get_hba_page_info(hba_desc, hba_desc->Device_Id, &hba_desc->RunAsNonRAID);
#endif

	spin_lock_init(&hba_desc->global_lock);
#ifdef SUPPORT_STAGGERED_SPIN_UP
	spin_lock_init(&hba_desc->device_spin_up);
#endif

    spin_lock_init(&hba_desc->hba_lock);
    spin_lock_init(&hba_desc->resource_lock);
    spin_lock_init(&hba_desc->wait_queue_lock);
    spin_lock_init(&hba_desc->compl_queue_lock);
    spin_lock_init(&hba_desc->core_queue_lock);
	spin_lock_init(&hba_desc->core_lock);
    for(i = 0; i < MAX_NUMBER_IO_CHIP; i++) {
        spin_lock_init(&hba_desc->root_lock[i]);
    }
    for(i = 0; i < MAX_NUMBER_IO_CHIP * MAX_MULTI_QUEUE; i++) {
        	spin_lock_init(&hba_desc->hw_queue_lock[i]);
		spin_lock_init(&hba_desc->hw_wq_lock[i]);
		spin_lock_init(&hba_desc->hw_cq_lock[i]);
    }

	MV_DPRINT(( "HBA ext struct init'ed at %p.\n",hba_desc));

	if (register_online_modules(hba_desc))
		goto ext_err_modmm;

	if (mv_alloc_module_resource(hba_desc))
		goto ext_err_modmm;

	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		goto ext_err_pci;
	__mv_save_hba_configuration(hba_desc, mod_desc->extension);
	
	phba=(PHBA_Extension)mod_desc->extension;
#ifdef RAID_DRIVER
	raid_get_hba_page_info(mod_desc->extension);
#else
	phba->RunAsNonRAID = 1;
#endif
	hba_desc->RunAsNonRAID = phba->RunAsNonRAID;

#ifdef CONFIG_PM
	mod_desc =  __get_highest_module(hba_desc);
	pci_set_drvdata(dev,mod_desc);
#endif
	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		goto ext_err_pci;

	hba_desc->alloc_uncahemem_failed = MV_FALSE;
	while (mod_desc) {
		if (MV_MOD_INITED != mod_desc->status)
			continue;
		// Save mod description at module extension, do not zero module extension.
		mod_desc->ops->module_initialize(mod_desc->extension,
						 mod_desc->extension_size,
						 hba_desc->max_io);
		if (hba_desc->alloc_uncahemem_failed)
			goto ext_err_pci;
		/* there's no support for sibling module at the moment  */
		mod_desc = mod_desc->parent;
	}

	return 0;

ext_err_pci:
	++dbg_ret;
	mv_release_module_resource(hba_desc);
ext_err_modmm:
	++dbg_ret;
	mv_hba_release_modmm(dev);
ext_err_init:
        ++dbg_ret;
	return dbg_ret;
}


