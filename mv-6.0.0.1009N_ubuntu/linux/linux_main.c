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

/*
 *
 *  Copyright (C) 2006 Marvell Technology Group Ltd. All Rights Reserved.
 *  linux_main.c
 *
 *
 */
#include "linux_main.h"
#include "hba_mod.h"
#include "linux_iface.h"
#include "hba_timer.h"
#include "csmisas.h"

#if defined(__VMKLNX__)
#if (VMWARE_ESX_DDK_VERSION == 41000)
#include <vmklinux26/vmklinux26_scsi.h>
#else
#include <vmklinux_9/vmklinux_scsi.h>
#endif
#endif

#if defined(SUPPORT_MODULE_CONSOLIDATE)
#include "cc.h"
#endif		/* SUPPORT_MODULE_CONSOLIDATE */
#if defined(HARDWARE_XOR)
#include "pl_xor.h"
#endif
#ifdef RAID_DRIVER
#include "raid_structure.h"
#endif
#if defined(HARDWARE_XOR) || defined(SUPPORT_MV_SAS_CONTROLLER) || defined(SUPPORT_MV_DMA_CONTROLLER)
#define IRQ_SP2_BASE					0
#define IRQ_SP2_XOR_IRQ(x)				(IRQ_SP2_BASE+x)		/0 - 1
#define IRQ_SP2_SAS0_IRQ				(IRQ_SP2_BASE + 3)
#define IRQ_SP2_SAS0_CMPL_IRQ(x)		(IRQ_SP2_SAS0_IRQ+ 1 + x)	// 4-11
#define IRQ_SP2_SAS0_SGPIO_IRQ(x)		(IRQ_SP2_SAS0_IRQ+ 9 +x)			//SAS0 SGPIO(0-1) 12 - 13
#define IRQ_SP2_SAS1_IRQ				(IRQ_SP2_BASE + 14)
#define IRQ_SP2_SAS1_CMPL_IRQ(x)		(IRQ_SP2_SAS1_IRQ+ 1 + x)	// 15-22
#define IRQ_SP2_SAS1_SGPIO_IRQ(x)		(IRQ_SP2_SAS1_IRQ+ 9 + x)			//SAS1 SGPIO(0-1) 23 - 24
#define IRQ_SP2_SAS2_IRQ				(IRQ_SP2_BASE + 25)
#define IRQ_SP2_SAS2_CMPL_IRQ(x)		(IRQ_SP2_SAS2_IRQ+ 1 + x)	// 26-33
#define IRQ_SP2_SAS2_SGPIO_IRQ(x)		(IRQ_SP2_SAS2_IRQ+ 9 + x)	//SAS1 SGPIO(0-1) 34 - 35
#define IRQ_SP2_UDMA0_IRQ(x)		(IRQ_SP2_BASE + 70 + x)			// 70-78
#define IRQ_SP2_UDMA1_IRQ(x)		(IRQ_SP2_BASE + 79 + x)			// 79-87
#endif
#if defined(__VMKLNX__) || defined(MV_VMK_ESX35)
#define PCI_DEVICE(vend,dev) \
       .vendor = (vend), .device = (dev), \
        .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
spinlock_t io_request_lock;
#endif

static const struct pci_device_id mv_pci_ids[] = {
#ifdef THOR_PRODUCT
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THORLITE_0S1P)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THORLITE_1S1P)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THORLITE_2S1P)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THORLITE_2S1P_WITH_FLASH)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THOR_4S1P)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_THOR_4S1P_NEW)},
#endif
#ifdef MAGNI_PRODUCT
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9123)},
#endif
#ifdef ODIN_PRODUCT
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_6320)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_6340)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_6440)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_6480)},
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_6485)},
#endif
#ifdef VANIR_PRODUCT
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_9480)},
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_9480)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9485)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9440)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9445)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9340)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9345)},
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_948F)},
#endif
#ifdef ATHENA_PRODUCT
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_ATHENA_1480)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1580)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1475)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1485)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1495)},
#endif
#ifdef SP2_PRODUCT
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_ATHENA_1480)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1580)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1475)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1485)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_ATHENA_1495)},
#endif
	{0}
};


/*
 *  cmd line parameters
 */
static int mv_int_mode = 2;
module_param(mv_int_mode, int, 0);
MODULE_PARM_DESC(mv_int_mode, " Interrupt Mode for Marvell \
	controllers (0:legacy, 1:msi, 2:msix)");

static int mv_test_enable;
module_param(mv_test_enable, int, S_IRUGO);
MODULE_PARM_DESC(mv_test_enable, " Support do test for Marvell \
	controllers (default=0)");

static int mv_reg_enable;
module_param(mv_reg_enable, int, S_IRUGO);
MODULE_PARM_DESC(mv_reg_enable, " Support register r/w for Marvell \
	controllers (default=0)");
#ifdef SUPPORT_PHY_POWER_MODE
unsigned long mv_PHY_power_mode_HIPM;
module_param(mv_PHY_power_mode_HIPM, ulong, 0);
MODULE_PARM_DESC(mv_PHY_power_mode_HIPM, " HIPM Mode for Marvell controllers (0:disable, 1:Partial, 2:Slumber)");

unsigned long mv_PHY_power_mode_HIPM_timer = 10;
module_param(mv_PHY_power_mode_HIPM_timer, ulong, 0);
MODULE_PARM_DESC(mv_PHY_power_mode_HIPM_timer, " HIPM timer for Marvell controllers (unit: second)");

unsigned long mv_PHY_power_mode_port_map = 0xffff;
module_param(mv_PHY_power_mode_port_map, ulong, 0);
MODULE_PARM_DESC(mv_PHY_power_mode_port_map, " PHY Power Mode Port Map for Marvell controllers (Port Map)");

unsigned long mv_PHY_power_mode_DIPM;
module_param(mv_PHY_power_mode_DIPM, ulong, 0);
MODULE_PARM_DESC(mv_PHY_power_mode_DIPM, " DIPM Mode enable for Marvell controllers(0:disable 1:enable) ");
#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))

/* notifier block to get notified on system shutdown/halt/reboot/down */
static int mv_linux_halt(struct notifier_block *nb, unsigned long event,
			 void *buf)
{
	switch (event) {
	case SYS_RESTART:
	case SYS_HALT:
	case SYS_POWER_OFF:
		MV_DPRINT(("%s assert!\n",__func__));
              	mv_hba_stop(NULL);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
static struct notifier_block mv_linux_notifier = {
	mv_linux_halt, NULL, 0
};
#endif
#endif /*#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))*/


#if defined(SUPPORT_MV_SAS_CONTROLLER)
static int mv_probe(platform_device_t *dev)
#else
static int mv_probe(struct pci_dev *dev, const struct pci_device_id *id)
#endif
{
	unsigned int ret;
	int err = 0;
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	struct resource *res_irq = platform_get_resource(dev, IORESOURCE_IRQ, 0);
	MV_PRINT("Marvell Storage Controller ID%d is found, using IRQ %d, driver version %s.\n",
	       dev->id, res_irq->start, mv_version_linux);

#else /*!SUPPORT_MV_SAS_CONTROLLER*/
	ret = PCIBIOS_SUCCESSFUL;
#if defined(PRODUCTNAME_ATHENA) /*for Athena Z0/Z1, only function 0 works ok*/
	if (dev->devfn != 0)
		return -ENODEV;
#endif

	ret = pci_enable_device(dev);
	if (ret) {
		MV_PRINT("%s : enable device failed.\n", mv_product_name);
		return ret;
	}

	ret = pci_request_regions(dev, mv_driver_name);
	if (ret)
		goto err_req_region;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,0,13)
	if ( !pci_set_dma_mask(dev, DMA_64BIT_MASK) ) {
#ifndef MV_VMK_ESX35
		ret = pci_set_consistent_dma_mask(dev, DMA_64BIT_MASK);
		if (ret) {
			ret = pci_set_consistent_dma_mask(dev,
							  DMA_32BIT_MASK);
			if (ret)
				goto err_dma_mask;
		}
#endif
	} else {
		ret = pci_set_dma_mask(dev, DMA_32BIT_MASK);
		if (ret)
			goto err_dma_mask;
#ifndef MV_VMK_ESX35
		ret = pci_set_consistent_dma_mask(dev, DMA_32BIT_MASK);
		if (ret)
			goto err_dma_mask;
#endif
	}
#else
    if ( !pci_set_dma_mask(dev, DMA_BIT_MASK(64)) ) {
#ifndef MV_VMK_ESX35
        ret = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64));
        if (ret) {
            ret = pci_set_consistent_dma_mask(dev,
                              DMA_BIT_MASK(32));
            if (ret)
                goto err_dma_mask;
        }
#endif
    } else {
        ret = pci_set_dma_mask(dev, DMA_BIT_MASK(32));
        if (ret)
            goto err_dma_mask;
#ifndef MV_VMK_ESX35
        ret = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32));
        if (ret)
            goto err_dma_mask;
#endif
    }
#endif

	pci_set_master(dev);

	MV_PRINT("Marvell Storage Controller is found, using IRQ %d, driver version %s.\n",
	       dev->irq, mv_version_linux);
#endif /*!SUPPORT_MV_SAS_CONTROLLER*/

	MV_PRINT("Marvell Linux driver %s, driver version %s.\n",
	      mv_driver_name, mv_version_linux);

	MV_DPRINT(("Start mv_hba_init.\n"));

	ret = mv_hba_init(dev, MV_MAX_IO);
	if (ret) {
		MV_DPRINT(( "Error no %d.\n", ret));
		ret = -ENOMEM;
		goto err_dma_mask;
	}

	MV_DPRINT(("Start mv_hba_start.\n"));

	if (mv_hba_start(dev)) {
		ret = -ENODEV;
		goto err_mod_start;
	}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)) && !defined (MV_VMK_ESX35)
		if (__mv_get_adapter_count() == 1) {
			register_reboot_notifier(&mv_linux_notifier);
		}
#endif
#endif
	MV_DPRINT(("Finished mv_probe.\n"));

	return 0;
err_mod_start:
	err++;
	mv_hba_stop(dev);
	mv_hba_release(dev);
err_dma_mask:
	err++;
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
	pci_release_regions(dev);
err_req_region:
	err++;
	pci_disable_device(dev);
#endif

	MV_PRINT("%s : error counter %d.\n", mv_product_name, err);
	return ret;
}
#if defined(SUPPORT_MV_SAS_CONTROLLER)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
static int mv_remove(platform_device_t *dev)
#else
static int  mv_remove(platform_device_t *dev)
#endif
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
static void mv_remove(struct pci_dev *dev)
#else
static void __devexit mv_remove(struct pci_dev *dev)
#endif
#endif
{
	mv_hba_stop(dev);
	mv_hba_release(dev);
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
	pci_release_regions(dev);
	pci_disable_device(dev);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))
	if (__mv_get_adapter_count() == 0) {
		unregister_reboot_notifier(&mv_linux_notifier);
	}
#endif
#endif
	MV_PRINT("%s : Marvell %s linux driver removed !\n", mv_product_name, mv_product_name);
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	return 0;
#endif
}
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
static void mv_shutdown(struct pci_dev * pdev)
{
	mv_hba_stop(NULL);
}
#endif
#endif
void core_disable_ints(void * ext);
void core_enable_ints(void * ext);
MV_BOOLEAN core_check_int(void *ext);
MV_BOOLEAN Core_QueueInterruptHandler(MV_PVOID This, MV_U16 index);
#ifdef SUPPORT_MSIX_INT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
static irqreturn_t mv_msix_check_int(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}
#endif
/* msix interrupt handler, TODO: tasklet */
#ifdef MV_VMK_ESX35
static int mv_msix_intr_handler(int irq, void *dev_id)
#else
static irqreturn_t mv_msix_intr_handler(int irq, void *dev_id)
#endif
{
	int ret;
	unsigned long flags;
	struct msix_data *msix_data = dev_id;
	struct hba_extension *hba = msix_data->hba;
	struct mv_mod_desc *core_desc;

	core_desc = __get_lowest_module(hba->desc->hba_desc);

	ret = Core_QueueInterruptHandler(core_desc->extension, msix_data->index);
	return IRQ_RETVAL(ret);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#ifdef __VMKERNLE_MODULE__
 int mv_hba_msix_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
#else
 irqreturn_t mv_hba_msix_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	return mv_msix_intr_handler(irq, dev_id);
}
#else
 irqreturn_t mv_hba_msix_intr_handler(int irq, void *dev_id)
{
	return mv_msix_intr_handler(irq, dev_id);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */
#endif

#ifdef MV_VMK_ESX35
static int mv_hba_int_handler(void *dev_id)
#else
static irqreturn_t mv_hba_int_handler(void *dev_id)
#endif
{
#ifndef MV_VMK_ESX35
	irqreturn_t retval = MV_FALSE;
#else
	int retval = MV_FALSE;
#endif
	struct hba_extension *hba = (struct hba_extension *) dev_id;
#ifndef SUPPORT_TASKLET
	unsigned long flags;

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	retval = hba->desc->child->ops->module_service_isr(hba->desc->child->extension);
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);
	
#else
	struct mv_mod_desc *core_desc;
	core_desc=__get_lowest_module(hba->desc->hba_desc);
			   
	core_disable_ints(core_desc->extension);

	if (!hba->msi_enabled) {
		/*filter the interrupts from other device sharing IRQ.*/
		retval = core_check_int(core_desc->extension);
		if (!retval) {
			core_enable_ints(core_desc->extension);
			return IRQ_RETVAL(retval);
		}
	} else {
		/*no IRQ sharing with MSI-enabled, schedule tasklet unconditionally.*/
		retval = MV_TRUE;
	}
	
	tasklet_schedule(&hba->desc->hba_desc->mv_tasklet);
#endif	/*SUPPORT_TASKLET*/

	return IRQ_RETVAL(retval);
}
#if defined(HARDWARE_XOR)
#ifdef MV_VMK_ESX35
static int mv_hba_int_xor_handler(void *dev_id)
#else
static irqreturn_t mv_hba_int_xor_handler(void *dev_id)
#endif
{
#ifndef MV_VMK_ESX35
	irqreturn_t retval = MV_FALSE;
#else
	int retval = MV_FALSE;
#endif
	struct hba_extension *hba = (struct hba_extension *) dev_id;
#ifndef SUPPORT_TASKLET
	unsigned long flags;

	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	retval = hba->desc->child->ops->module_service_isr(hba->desc->child->extension);
	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);
	
#else
	struct mv_mod_desc *core_desc;
	core_desc=__get_lowest_module(hba->desc->hba_desc);
			   
	xor_disable_ints(core_desc->extension);
       // printk("mv_hba_int_xor_handler \n");
	if (!hba->msi_enabled) {
		/*filter the interrupts from other device sharing IRQ.*/
		retval = xor_check_int(core_desc->extension);
		if (!retval) {
			xor_enable_ints(core_desc->extension);
			return IRQ_RETVAL(retval);
		}
	} else {
		/*no IRQ sharing with MSI-enabled, schedule tasklet unconditionally.*/
		retval = MV_TRUE;
	}
	
	tasklet_schedule(&hba->desc->hba_desc->mv_tasklet_xor);
#endif	/*SUPPORT_TASKLET*/

	return IRQ_RETVAL(retval);
}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#ifdef __VMKERNLE_MODULE__
 int mv_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
#else
 irqreturn_t mv_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	return mv_hba_int_handler(dev_id);
}
#else
 irqreturn_t mv_intr_handler(int irq, void *dev_id)
{
	return mv_hba_int_handler(dev_id);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */
#if defined(HARDWARE_XOR)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#ifdef __VMKERNLE_MODULE__
 int mv_intr_xor_handler(int irq, void *dev_id, struct pt_regs *regs)
#else
 irqreturn_t mv_intr_xor_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	return mv_hba_int_xor_handler(dev_id);
}
#else
 irqreturn_t mv_intr_xor_handler(int irq, void *dev_id)
{
	return mv_hba_int_xor_handler(dev_id);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */
#endif
#if defined(CONFIG_PM) && !defined(SUPPORT_MV_SAS_CONTROLLER)
int core_suspend (void *ext);
int core_resume (void *ext);
#ifdef RAID_DRIVER
static void hba_suspend_resume_cb(MV_PVOID This, PMV_Request req)
{
	struct hba_extension *hba = (struct hba_extension *) This;
#ifdef USE_REQ_POOL
	hba_req_cache_free(hba,req);
#else
	res_free_req_to_pool(hba->req_pool, req);
#endif
    
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 0);
#else
	complete(&hba->desc->hba_desc->ioctl_cmpl);
#endif
}
#endif
static int mv_hba_suspend(struct mv_mod_desc *hba_mod)
{
	unsigned long flags;
	PMV_Request pReq;
	struct hba_extension *phba;
	struct mv_mod_desc *core_mod;
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	BUG_ON(!ioc);
	MV_PRINT("start  mv_hba_suspend.\n");

	phba = (struct hba_extension *)hba_mod->extension;
	core_mod = __get_lowest_module(ioc);

#ifndef MV_VMK_ESX35
#ifdef RAID_DRIVER
	if (!phba->desc->hba_desc->RunAsNonRAID) {		
#ifdef USE_REQ_POOL
		pReq = hba_req_cache_alloc(phba);
#else
		pReq = res_get_req_from_pool(phba->req_pool);
#endif
		if (NULL == pReq) {
			MV_PRINT("%s : cannot allocate memory for req.\n",
			       mv_product_name);
			return -1;
		}

		WARN_ON(atomic_read(&hba->Io_Count) != 0);
		WARN_ON(atomic_read(&phba->Ioctl_Io_Count) != 0);

		pReq->Cmd_Initiator = phba;
		pReq->Org_Req = pReq;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;	
		pReq->Completion = hba_suspend_resume_cb;

		pReq->Cdb[0] = APICDB0_ADAPTER;
		pReq->Cdb[1] = APICDB1_ADAPTER_POWER_STATE_CHANGE;
		pReq->Cdb[2] = APICDB2_ADAPTER_POWER_CHANGE_LEAVING_S0;
		pReq->Cmd_Flag = 0;
		pReq->Req_Type = REQ_TYPE_INTERNAL;

		atomic_inc(&phba->Io_Count);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
		phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		if (!__hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync,60*HZ))
#else
		if (!wait_for_completion_timeout(&phba->desc->hba_desc->ioctl_cmpl,60*HZ))
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		{
		    MV_DPRINT(("mv_hba_suspend cmd failed.\n"));
#ifdef USE_REQ_POOL
			hba_req_cache_free(phba,pReq);
#else
			res_free_req_to_pool(phba->req_pool, pReq);
#endif
			atomic_dec(&phba->Io_Count);
			return -1;
		}

		atomic_dec(&phba->Io_Count);
	}
#endif
#endif

	if (core_suspend(core_mod->extension)){
		MV_DPRINT(("core_suspend failed.\n"));
		return -1;
	}

	return 0;	
}

static int mv_hba_resume(struct mv_mod_desc *hba_mod)
{
	unsigned long flags;
	PMV_Request pReq;
	struct hba_extension *phba;
	struct mv_mod_desc *core_mod;
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	BUG_ON(!ioc);
	MV_PRINT("start  mv_hba_resume.\n");

	phba = (struct hba_extension *)hba_mod->extension;
	core_mod = __get_lowest_module(ioc);

	if (core_resume(core_mod->extension)) {
		MV_PRINT("mv_resume_core failed.\n");
		return -1;
	}	

#ifndef MV_VMK_ESX35
#ifdef RAID_DRIVER
	if (!phba->desc->hba_desc->RunAsNonRAID) {		
#ifdef USE_REQ_POOL
		pReq = hba_req_cache_alloc(phba);
#else
		pReq = res_get_req_from_pool(phba->req_pool);
#endif
		if (NULL == pReq) {
			MV_PRINT("%s : cannot allocate memory for req.\n",
				   mv_product_name);
			return -1;
		}

		WARN_ON(atomic_read(&phba->Io_Count) != 0);
		WARN_ON(atomic_read(&phba->Ioctl_Io_Count) != 0);

		pReq->Cmd_Initiator = phba;
		pReq->Org_Req = pReq;
		pReq->Scsi_Status = REQ_STATUS_SUCCESS; 
		pReq->Completion = hba_suspend_resume_cb;

		pReq->Cdb[0] = APICDB0_ADAPTER;
		pReq->Cdb[1] = APICDB1_ADAPTER_POWER_STATE_CHANGE;
		pReq->Cdb[2] = APICDB2_ADAPTER_POWER_CHANGE_ENTER_S0;
		pReq->Cmd_Flag = 0;
		pReq->Req_Type = REQ_TYPE_INTERNAL;

		atomic_inc(&phba->Io_Count);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
		phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		if (!__hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync,60*HZ))
#else
		if (!wait_for_completion_timeout(&phba->desc->hba_desc->ioctl_cmpl,60*HZ))
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		{
			MV_DPRINT(("mv_hba_resume cmd failed.\n"));
#ifdef USE_REQ_POOL
			hba_req_cache_free(phba,pReq);
#else
			res_free_req_to_pool(phba->req_pool, pReq);
#endif
			atomic_dec(&phba->Io_Count);
			return -1;
		}

		atomic_dec(&phba->Io_Count);
	}
#endif
#endif
	
	return 0;	
}

#ifndef MV_VMK_ESX35
static int mv_suspend(struct pci_dev *pdev, pm_message_t state)
#else
static int mv_suspend(struct pci_dev *pdev, int state)
#endif
{
	struct hba_extension *ext;
	struct mv_mod_desc *hba_mod = pci_get_drvdata(pdev);
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	BUG_ON(!ioc);
	MV_PRINT("start  mv_suspend.\n");

	ext = (struct hba_extension *)hba_mod->extension;
	
    if (mv_hba_suspend(hba_mod)){
        MV_DPRINT(("mv_hba_suspend failed.\n"));
		return -1;
    }

	/* fs TODO: support msi-x */
	free_irq(ioc->dev->irq,ext);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_save_state(pdev);
#else
	pci_save_state(pdev,ioc->pci_config_space);
#endif
	pci_disable_device(pdev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_set_power_state(pdev,pci_choose_state(pdev,state));
#else
	pci_set_power_state(pdev,state);
#endif

	return 0;
}
#ifdef MV_VMK_ESX35
#define  PCI_D0 	0
#endif

static int mv_resume (struct pci_dev *pdev)
{
	int ret;
	struct hba_extension *ext;
	struct mv_mod_desc *hba_mod = pci_get_drvdata(pdev);
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	ext = (struct hba_extension *)hba_mod->extension;	
	MV_PRINT("start  mv_resume.\n");

	pci_set_power_state(pdev, PCI_D0);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
#else
	pci_restore_state(pdev,ioc->pci_config_space);
#endif
	pci_set_master(pdev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	ret = request_irq(ioc->dev->irq, mv_intr_handler, IRQF_SHARED,
	                  mv_driver_name, ext);
#else
	ret = request_irq(ioc->dev->irq, mv_intr_handler, SA_SHIRQ,
		mv_driver_name, ext);
#endif
	if (ret < 0) {
	        MV_PRINT("request IRQ failed.\n");
	        return -1;
	}

    if (mv_hba_resume(hba_mod)){
        MV_DPRINT(("mv_hba_resume failed.\n"));
		return -1;
    }
	
	return 0;
}
#endif

#if !defined(SUPPORT_MV_SAS_CONTROLLER)
static struct pci_driver mv_pci_driver = {
	.name     = mv_driver_name,
	.id_table = mv_pci_ids,
	.probe    = mv_probe,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
	.remove   = mv_remove,
#else
	.remove   = __devexit_p(mv_remove),
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	.shutdown = mv_shutdown,
#endif
#ifdef CONFIG_PM
	.resume = mv_resume,
	.suspend = mv_suspend,
#endif
};
#endif
#ifdef USE_REQ_POOL

int hba_req_cache_create(MV_PVOID hba_ext)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	struct mv_adp_desc   *hba_desc=phba->desc->hba_desc;
	sprintf(phba->cache_name,"%s%d%d","mv_request_",hba_desc->Device_Id, hba_desc->id);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	phba->mv_request_cache = kmem_cache_create(phba->cache_name,sizeof(struct _MV_Request),
			0, SLAB_HWCACHE_ALIGN, NULL);
#else
	phba->mv_request_cache = kmem_cache_create(phba->cache_name,sizeof(struct _MV_Request),
			0, SLAB_HWCACHE_ALIGN, NULL,NULL);
#endif
	if(phba->mv_request_cache == NULL)
		return -ENOMEM;
	sprintf(phba->sg_name,"%s%d%d","mv_sgtable_",hba_desc->Device_Id, hba_desc->id);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	phba->mv_request_sg_cache = kmem_cache_create(phba->sg_name,
		phba->max_sg_count  * sizeof(MV_SG_Entry),
			0,SLAB_HWCACHE_ALIGN, NULL);
#else
	phba->mv_request_sg_cache = kmem_cache_create(phba->sg_name,
		phba->max_sg_count  * sizeof(MV_SG_Entry),
			0,SLAB_HWCACHE_ALIGN, NULL,NULL);
#endif
	if(phba->mv_request_sg_cache == NULL) {
		kmem_cache_destroy(phba->mv_request_cache);
		return -ENOMEM;
	}

	phba->mv_mempool= mempool_create(2, mempool_alloc_slab,mempool_free_slab,
		phba->mv_request_sg_cache);
	if(!phba->mv_mempool){
		kmem_cache_destroy(phba->mv_request_cache);
		kmem_cache_destroy(phba->mv_request_sg_cache);
		return -ENOMEM;
	}

#if defined(HAVE_HW_COMPLIANT_SG)
    sprintf(phba->sgpool_name, "%s%d%d", "mv_sgpool_", 
        hba_desc->Device_Id, hba_desc->id);
    phba->mv_sgtable_pool = pci_pool_create(phba->sgpool_name, hba_desc->dev, 
            phba->max_sg_count * sizeof(MV_SG_Entry), 16, 0);
    if(!phba->mv_sgtable_pool)
    {
        mempool_destroy(phba->mv_mempool);
        kmem_cache_destroy(phba->mv_request_cache);
        kmem_cache_destroy(phba->mv_request_sg_cache);
        return -ENOMEM;
    }
#endif
	return 0;
}

PMV_Request hba_req_cache_alloc(MV_PVOID hba_ext)
{
	int max_sg = 0;
	MV_SG_Entry * sgtable= NULL;
	struct _MV_Request * req = NULL;
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	
#if defined(HAVE_HW_COMPLIANT_SG)
	struct mv_mod_desc *mod_desc = __ext_to_gen(hba_ext)->desc;
	dma_addr_t	  dma_addr;
	BUS_ADDRESS   bus_addr;
	MV_PHYSICAL_ADDR phy_addr;
#endif

	max_sg = phba->max_sg_count;
	req = kmem_cache_alloc(phba->mv_request_cache, GFP_ATOMIC);
	if (!req) {
		MV_DPRINT(("cache alloc req failed.\n"));
		return NULL;
	}
	memset(req,0x00,sizeof(struct _MV_Request));
#if !defined(HAVE_HW_COMPLIANT_SG)
	sgtable =  mempool_alloc(phba->mv_mempool, GFP_ATOMIC);
#else
    sgtable = pci_pool_alloc(phba->mv_sgtable_pool, GFP_ATOMIC, &dma_addr);
	if (NULL == sgtable) {
	    kmem_cache_free(phba->mv_request_cache, req);
		MV_DPRINT(("unable to alloc 0x%lx consistent mem.\n",
		       phba->max_sg_count  * sizeof(MV_SG_Entry)));
		return NULL;
	}

	bus_addr            = (BUS_ADDRESS) dma_addr;
	phy_addr.parts.low  = LO_BUSADDR(bus_addr);
	phy_addr.parts.high = HI_BUSADDR(bus_addr);
	req->bus_addr = phy_addr;	
#endif

	if (sgtable) {
		memset(sgtable, 0x00,sizeof(max_sg  * sizeof(MV_SG_Entry)));
		req->SG_Table.Entry_Ptr= sgtable;
		req->SG_Table.Max_Entry_Count = max_sg;
	} else {
		kmem_cache_free(phba->mv_request_cache,req);
		//MV_DPRINT(("cache alloc sg failed, running io %d.\n",phba->Io_Count));
		return NULL;
	}
	MV_ZeroMvRequest(req);
	return req;
}

void hba_req_cache_free(MV_PVOID hba_ext,PMV_Request req)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
#if defined(HAVE_HW_COMPLIANT_SG)
	dma_addr_t       dma_addr;
	MV_PHYSICAL_ADDR phy_addr;

	struct mv_mod_desc *mod_desc = __ext_to_gen(hba_ext)->desc;

	if(!mod_desc){
		MV_DPRINT(("mod decript is destried.\n"));
	}

	phy_addr = req->bus_addr;
	dma_addr = (dma_addr_t) (phy_addr.parts.low |
				 ((u64) phy_addr.parts.high << 32));
    pci_pool_free(phba->mv_sgtable_pool, (void *)req->SG_Table.Entry_Ptr, 
                  dma_addr);
#else
	mempool_free((void *)req->SG_Table.Entry_Ptr,phba->mv_mempool);
#endif
	kmem_cache_free(phba->mv_request_cache,req);
}

void  hba_req_cache_destroy(MV_PVOID hba_ext)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
#if defined(HAVE_HW_COMPLIANT_SG)
    pci_pool_destroy(phba->mv_sgtable_pool);
#endif
	mempool_destroy(phba->mv_mempool);
	kmem_cache_destroy(phba->mv_request_cache);
	kmem_cache_destroy(phba->mv_request_sg_cache);
}
#else
struct mv_request_pool *res_reserve_req_pool(MV_U32 mod_id,
					     MV_U32 size,
					     MV_U32 sg_count)
{
	int i;
	unsigned int mem_size;
	struct mv_request_pool *pool;
	struct _MV_Request *req;
	MV_SG_Entry *sg;

	MV_DPRINT(( "module %u reserves reqcount=%d, sgcount=%d,request pool of size %lu.\n",
	       mod_id,size,sg_count,
	       (unsigned long) (sizeof(struct _MV_Request) * size +
				sizeof(MV_SG_Entry)* sg_count * size)));

	mem_size = sizeof(struct mv_request_pool);
	pool = hba_mem_alloc(mem_size,MV_FALSE);
	if (NULL == pool)
		goto res_err_pool;
	memset(pool, 0, mem_size);

	/* assuming its size is already 64bit-aligned */
	mem_size = sizeof(struct _MV_Request) * size;
	req = hba_mem_alloc(mem_size,MV_FALSE);
	if (NULL == req)
		goto res_err_req;
	memset(req, 0, mem_size);

	mem_size = sizeof(MV_SG_Entry) * sg_count * size;
	sg  = hba_mem_alloc(mem_size,MV_FALSE);
	if (NULL == sg)
		goto res_err_sg;
	memset(sg, 0, mem_size);

	MV_LIST_HEAD_INIT(&pool->free_list);
	MV_LIST_HEAD_INIT(&pool->use_list);
	spin_lock_init(&pool->lock);

	pool->mod_id  = mod_id;
	pool->size    = size;
	pool->sg_count    = sg_count;
	pool->req_mem = (void *) req;
	pool->sg_mem = (void *)sg;

	for (i = 0; i < size; i++) {
		req->SG_Table.Entry_Ptr  = sg;
		req->SG_Table.Max_Entry_Count = sg_count;
		List_AddTail(&req->pool_entry, &pool->free_list);
		req++;
		sg += sg_count;
	}

	return pool;

res_err_sg:
	hba_mem_free(req,sizeof(struct _MV_Request) * size,MV_FALSE);
	MV_ASSERT(MV_FALSE);
res_err_req:
	hba_mem_free(pool,sizeof(struct mv_request_pool),MV_FALSE);
	MV_ASSERT(MV_FALSE);
res_err_pool:
	MV_ASSERT(MV_FALSE);
	return NULL;
}

struct _MV_Request *res_get_req_from_pool(struct mv_request_pool *pool)
{
	struct _MV_Request *req;
	unsigned long flags;

	BUG_ON(pool == NULL);
	spin_lock_irqsave(&pool->lock, flags);
	if (List_Empty(&pool->free_list)) {
		res_dump_pool_info(pool);
		spin_unlock_irqrestore(&pool->lock, flags);
		return NULL;
	}

	req = LIST_ENTRY(pool->free_list.next, struct _MV_Request, pool_entry);
	MV_ZeroMvRequest(req); /* FIX: we can do better than this */
	List_MoveTail(pool->free_list.next, &pool->use_list);
	spin_unlock_irqrestore(&pool->lock, flags);

	return req;
}

void res_free_req_to_pool(struct mv_request_pool *pool,
			  struct _MV_Request *req)
{
	unsigned long flags;
	BUG_ON((NULL == pool) || (NULL == req));
	spin_lock_irqsave(&pool->lock, flags);
	List_Move(&req->pool_entry, &pool->free_list);
	spin_unlock_irqrestore(&pool->lock, flags);
}

void res_dump_pool_info(struct mv_request_pool *pool)
{

}

void res_release_req_pool(struct mv_request_pool *pool)
{
	BUG_ON(NULL == pool);

	MV_DPRINT(("module %d release pool at %p.\n",
	       pool->mod_id, pool));
	hba_mem_free(pool->req_mem,sizeof(struct _MV_Request) *(pool->size),MV_FALSE);
	hba_mem_free(pool->sg_mem,sizeof(MV_SG_Entry) * (pool->sg_count) *(pool->size),MV_FALSE);
	hba_mem_free(pool,sizeof(struct mv_request_pool),MV_FALSE);
}
#endif

#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
int mv_prot_dma_map(struct scsi_cmnd *cmd)
{
	int nseg = 0;

	if (mv_prot_use_sg(cmd)) {
		struct device *dev = cmd->device->host->dma_dev;

		nseg = dma_map_sg(dev, mv_prot_bf(cmd), mv_prot_use_sg(cmd),
				  cmd->sc_data_direction);
		if (unlikely(!nseg))
			return -ENOMEM;
	}
	return nseg;
}

void mv_prot_dma_unmap(struct scsi_cmnd *cmd)
{
	if (mv_prot_use_sg(cmd)) {
		struct device *dev = cmd->device->host->dma_dev;

		dma_unmap_sg(dev, mv_prot_bf(cmd), mv_prot_use_sg(cmd),
			     cmd->sc_data_direction);
	}
}

#define left_len(x, y)  (y - (x % y))

static void append_intersect_sg_table(struct scatterlist *data_sg,
		struct scatterlist *prot_sg, PMV_SG_Table sg_table, unsigned int data_len)
{
	dma_addr_t data_ent_addr = 0;
	dma_addr_t prot_ent_addr = 0;
	unsigned int pro_len = 0, pro_prot_len = 0;
	unsigned int data_sg_len, prot_sg_len;
	unsigned int data_sg_num = 0, prot_sg_num = 0;
	unsigned int i;

	data_ent_addr = sg_dma_address(&data_sg[0]);
	prot_ent_addr = sg_dma_address(&prot_sg[0]);
	data_sg_len = sg_dma_len(&data_sg[0]);
	prot_sg_len = sg_dma_len(&prot_sg[0]);

	while (pro_len < data_len) {
		do {
			if (data_sg_len > left_len(pro_len, 512)) {
#ifdef HAVE_HW_COMPLIANT_SG
				sgdt_append(sg_table, LO_BUSADDR(data_ent_addr),
					HI_BUSADDR(data_ent_addr), left_len(pro_len, 512));
#else
				sgdt_append_pctx(sg_table, LO_BUSADDR(data_ent_addr),
					HI_BUSADDR(data_ent_addr), left_len(pro_len, 512), data_sg + data_sg_num);
#endif
				data_sg_len -= left_len(pro_len, 512);
				data_ent_addr += left_len(pro_len, 512);
				pro_len += left_len(pro_len, 512);
			} else {
#ifdef HAVE_HW_COMPLIANT_SG
				sgdt_append(sg_table, LO_BUSADDR(data_ent_addr),
					HI_BUSADDR(data_ent_addr), data_sg_len);
#else
				sgdt_append_pctx(sg_table, LO_BUSADDR(data_ent_addr),
					HI_BUSADDR(data_ent_addr), data_sg_len, data_sg + data_sg_num);
#endif
				pro_len += data_sg_len;
				data_sg_num++;
				data_ent_addr = sg_dma_address(&data_sg[data_sg_num]);
				data_sg_len = sg_dma_len(&data_sg[data_sg_num]);
			}
		} while (pro_len % 512 != 0);

		do {
			if (prot_sg_len > left_len(pro_prot_len, 8)) {
#ifdef HAVE_HW_COMPLIANT_SG
				sgdt_append(sg_table, LO_BUSADDR(prot_ent_addr),
					HI_BUSADDR(prot_ent_addr), left_len(pro_prot_len, 8));
#else
				sgdt_append_pctx(sg_table, LO_BUSADDR(prot_ent_addr),
					HI_BUSADDR(prot_ent_addr), left_len(pro_prot_len, 8), prot_sg + prot_sg_num);
#endif
				prot_sg_len -= left_len(pro_prot_len, 8);
				prot_ent_addr += left_len(pro_prot_len, 8);
				pro_prot_len += left_len(pro_prot_len, 8);
			} else {
#ifdef HAVE_HW_COMPLIANT_SG
				sgdt_append(sg_table, LO_BUSADDR(prot_ent_addr),
					HI_BUSADDR(prot_ent_addr), prot_sg_len);
#else
				sgdt_append_pctx(sg_table, LO_BUSADDR(prot_ent_addr),
					HI_BUSADDR(prot_ent_addr), prot_sg_len, prot_sg + prot_sg_num);
#endif
				pro_prot_len += prot_sg_len;
				prot_sg_num++;
				prot_ent_addr = sg_dma_address(&prot_sg[prot_sg_num]);
				prot_sg_len = sg_dma_len(&prot_sg[prot_sg_num]);
			}
		} while (pro_prot_len % 8 != 0);
	}
	if (pro_len != data_len || pro_prot_len != (data_len / 512)*8) {
		MV_PRINT("ERR: process not complete: pro len is %d , %d; actual len is %d\n",
			pro_len, pro_prot_len, data_len);
	}
}
#endif

static void generate_sg_table(struct hba_extension *phba,
			      struct scsi_cmnd *scmd,
			      PMV_SG_Table sg_table)
{
#if 0 //defined(SUPPORT_SP2)	// Todo
	struct scatterlist *sg;
	int dma_len;
	int length = mv_rq_bf_l(scmd);
	unsigned int sg_count = 0;
	dma_addr_t dma_addr, prp_dma;
	unsigned int page_size = (1 << 12);
	int offset = dma_addr & (page_size - 1);
	unsigned long *prp_list;
	unsigned long **list;
	{
		length -= (page_size - offset);
		if (length <= 0)
			assert(0);
		sg = (struct scatterlist *) mv_rq_bf(scmd);
		dma_len = sg_dma_len(sg);
		dma_addr = sg_dma_address(sg);
		dma_len -= (page_size - offset);
		if (dma_len) {
			dma_addr += (page_size - offset);
		} else {
			sg = sg_next(sg);
			dma_addr = sg_dma_address(sg);
			dma_len = sg_dma_len(sg);
		}
	}

#else
	struct scatterlist *sg;
	unsigned int sg_count = 0;
	unsigned int length;
	dma_addr_t busaddr = 0;
	int i;
#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	struct scatterlist *prot_sg;
	unsigned int prot_sg_count = 0;
	unsigned int prot_buf_len = 0;
#endif
//#ifdef MV_VMK_ESXI5
#if 1
	struct scatterlist *sgl;
#endif
	if (mv_rq_bf_l(scmd) > (mv_scmd_host(scmd)->max_sectors << 9)) {
		MV_DPRINT(( "ERROR: request length exceeds "
		"the maximum alowed value.\n"));
	}

	if (0 == mv_rq_bf_l(scmd))
		return ;

	if (scsi_sg_count(scmd)) {
		sg = (struct scatterlist *) mv_rq_bf(scmd);
	if (MV_SCp(scmd)->mapped == 0){
#ifndef MV_VMK_ESX35
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
			sg_count = scsi_dma_map(scmd);
#else
			sg_count = pci_map_sg(phba->desc->hba_desc->dev,sg,scsi_sg_count(scmd),
			scsi_to_pci_dma_dir(scmd->sc_data_direction));
#endif
#else
			sg_count = scmd->use_sg;
#endif

			if (sg_count != scsi_sg_count(scmd)) {
				MV_PRINT("WARNING sg_count(%d) != scmd->use_sg(%d)\n",
				(unsigned int) sg_count, scsi_sg_count(scmd));
			}
			MV_SCp(scmd)->mapped = 1;
		}
#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
		prot_sg = (struct scatterlist *) mv_prot_bf(scmd);
		if (prot_sg != NULL) {
			prot_sg_count = mv_prot_dma_map(scmd);

			for (i = 0; i < prot_sg_count; i++) {
				prot_buf_len += sg_dma_len(&prot_sg[i]);
			}
			if ((prot_buf_len / 8) * 512 != mv_rq_bf_l(scmd)) {
				MV_PRINT("DIF (%d) is not in accord with Data (%d)\n",
				prot_buf_len, mv_rq_bf_l(scmd));
			} else {
				append_intersect_sg_table(sg, prot_sg, sg_table, mv_rq_bf_l(scmd));
				return;
			}
		}
#endif

//#ifdef MV_VMK_ESXI5
#if 1
		scsi_for_each_sg(scmd, sgl, sg_count, i) {
			busaddr = sg_dma_address(sgl);
			length = sg_dma_len(sgl);
#else
		for (i = 0; i < sg_count; i++) {
			busaddr = sg_dma_address(&sg[i]);
			length = sg_dma_len(&sg[i]);
#endif

#if defined(MV_DEBUG) && defined(RAID_DRIVER) && !defined(__VMKLNX__)
			if(length > PAGE_SIZE)
			MV_DPRINT(("Warning: sg[%i] length %d > PAGE_SIZE\n", i, length));
#endif
#ifndef HAVE_HW_COMPLIANT_SG
			//sgdt_append_pctx(sg_table,LO_BUSADDR(busaddr), HI_BUSADDR(busaddr), length,  sg + i);
			sgdt_append_pctx(sg_table,LO_BUSADDR(busaddr), HI_BUSADDR(busaddr), length,  sgl);
#else
			sgdt_append(sg_table,LO_BUSADDR(busaddr), HI_BUSADDR(busaddr),length);
#endif

		}
	} else {
		if (MV_SCp(scmd)->mapped == 0) {
#ifndef MV_VMK_ESX35
			busaddr = dma_map_single(&phba->desc->hba_desc->dev->dev,mv_rq_bf(scmd),mv_rq_bf_l(scmd),
			scsi_to_pci_dma_dir(scmd->sc_data_direction));
#else
			busaddr = scmd->request_bufferMA;
#endif
			MV_SCp(scmd)->bus_address = busaddr;
			MV_SCp(scmd)->mapped = 1;
		}
#if defined(MV_DEBUG) && defined(RAID_DRIVER) && !defined(__VMKLNX__)
		if(mv_rq_bf_l(scmd) > PAGE_SIZE)
			MV_DPRINT(("Warning: single sg request_bufflen %d > PAGE_SIZE\n", mv_rq_bf_l(scmd)));
#endif

#ifndef HAVE_HW_COMPLIANT_SG		
		sgdt_append_vp(sg_table,mv_rq_bf(scmd), mv_rq_bf_l(scmd), LO_BUSADDR(busaddr),   HI_BUSADDR(busaddr));
#else
		sgdt_append(sg_table,LO_BUSADDR(busaddr),HI_BUSADDR(busaddr), mv_rq_bf_l(scmd));
#endif
	}
#endif
}
#if (defined(SUPPORT_DIF) || defined(SUPPORT_DIX)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
/* SCSI IO EEDPFlags bits */

#define MV_EEDPFLAGS_INC_PRI_REFTAG        (0x8000)
#define MV_EEDPFLAGS_INC_SEC_REFTAG        (0x4000)
#define MV_EEDPFLAGS_INC_PRI_APPTAG        (0x2000)
#define MV_EEDPFLAGS_INC_SEC_APPTAG        (0x1000)

#define MV_EEDPFLAGS_CHECK_REFTAG          (0x0400)
#define MV_EEDPFLAGS_CHECK_APPTAG          (0x0200)
#define MV_EEDPFLAGS_CHECK_GUARD           (0x0100)

/*zheng: change the operation value of eedpflags into bitwise control on
 * the corresponding four DIF control bit supplied by HW*/

#define MV_EEDPFLAGS_MASK_OP               (0x0007)
#define MV_EEDPFLAGS_NOOP_OP               (0x0000)
#define MV_EEDPFLAGS_CHECK_OP              (0x0001)
#define MV_EEDPFLAGS_STRIP_OP              (0x0002)
#define MV_EEDPFLAGS_CHECK_REMOVE_OP       (0x0003)
#define MV_EEDPFLAGS_INSERT_OP             (0x0004)
#define MV_EEDPFLAGS_REPLACE_OP            (0x0008)
#define MV_EEDPFLAGS_CHECK_REGEN_OP        (0x0005)
/**
 * mv_setup_eedp - setup MV request for EEDP transfer
 * @scmd: pointer to scsi command object
 * @pReq: pointer to the SCSI_IO reqest message frame
 *
 * Supporting protection type 1 and 3.
 *
 * Returns nothing
 */
static void
mv_setup_eedp(struct scsi_cmnd *scmd, PMV_Request pReq)
{
	MV_U16 eedp_flags;
	unsigned char prot_op = scsi_get_prot_op(scmd);
	unsigned char prot_type = scsi_get_prot_type(scmd);

	if (prot_type == SCSI_PROT_DIF_TYPE0 ||
	   prot_op == SCSI_PROT_NORMAL)
		return;

	switch (prot_op) {
		case SCSI_PROT_READ_STRIP:
			eedp_flags = MV_EEDPFLAGS_CHECK_REMOVE_OP;
			break;
		case SCSI_PROT_WRITE_INSERT:
			eedp_flags = MV_EEDPFLAGS_INSERT_OP;
			break;
		case SCSI_PROT_READ_INSERT:
			eedp_flags = MV_EEDPFLAGS_INSERT_OP;
			break;
		case SCSI_PROT_WRITE_STRIP:
			eedp_flags = MV_EEDPFLAGS_CHECK_REMOVE_OP;
			break;
		case SCSI_PROT_READ_PASS:
			eedp_flags = MV_EEDPFLAGS_CHECK_OP;
			break;
		case SCSI_PROT_WRITE_PASS:
			eedp_flags = MV_EEDPFLAGS_CHECK_OP;
			break;
		default:
			return;
	}

	switch (prot_type) {
	case SCSI_PROT_DIF_TYPE1:
	case SCSI_PROT_DIF_TYPE2:
		/*
		* enable ref/guard checking
		* auto increment ref tag
		*/
		pReq->EEDPFlags = eedp_flags |
		    MV_EEDPFLAGS_INC_PRI_REFTAG |
		    MV_EEDPFLAGS_CHECK_REFTAG |
		    MV_EEDPFLAGS_CHECK_GUARD;
		break;

	case SCSI_PROT_DIF_TYPE3:

		/*
		* enable guard checking
		*/
		pReq->EEDPFlags = eedp_flags |
		    MV_EEDPFLAGS_CHECK_GUARD;

		break;
	}
}
/**
 * mv_eedp_error_handling - return sense code for EEDP errors
 * @scmd: pointer to scsi command object
 * @ioc_status: ioc status
 *
 * Returns nothing
 */
static void
mv_eedp_error_handling(struct scsi_cmnd *scmd, MV_U8 req_status)
{
	MV_U8	sk;
	MV_U8	asc;
	MV_U8	ascq;

	sk = SCSI_SK_ABORTED_COMMAND;
	asc = SCSI_ASC_ECC_ERROR;
	switch (req_status) {
	/* 
	when asc is SCSI_ASC_ECC_ERROR
	the ascq refers to:
	0x01  Logical Block Gurad check failed
	0x02  Logical Block Application tag check failed
	0x03  Logical Block Reference tag check failed
	*/
	case REQ_STATUS_DIF_GUARD_ERROR:
		ascq = 0x01;
		break;
	case REQ_STATUS_DIF_APP_TAG_ERROR:
		ascq = 0x02;
		break;
	case REQ_STATUS_DIF_REF_TAG_ERROR:
		ascq = 0x03;
		break;
	default:
		ascq = 0x01;
		break;
	}
	MV_SetSenseData((PMV_Sense_Data)scmd->sense_buffer, sk, asc, ascq);
	scmd->result = (DID_OK << 16);
	return;
}
#endif

void mv_complete_request(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq)
{
	PMV_Sense_Data  senseBuffer = (PMV_Sense_Data)pReq->Sense_Info_Buffer;

	if (mv_rq_bf_l(scmd)) {
		if (MV_SCp(scmd)->mapped) {
#ifndef MV_VMK_ESX35
			if (scsi_sg_count(scmd)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
				scsi_dma_unmap(scmd);
#else
				pci_unmap_sg(phba->desc->hba_desc->dev,
					     mv_rq_bf(scmd),
					     scsi_sg_count(scmd),
					 	scsi_to_pci_dma_dir(scmd->sc_data_direction));
#endif
			} else {
				dma_unmap_single(&phba->desc->hba_desc->dev->dev,
						 MV_SCp(scmd)->bus_address,
						 mv_rq_bf_l(scmd),
				     	scsi_to_pci_dma_dir(scmd->sc_data_direction));
			}
#endif
		}
	}
#if 0//def SUPPORT_IO_DELAY
		if ((pReq->Scsi_Status != REQ_STATUS_SUCCESS) && (pReq->Cdb[0] != SCSI_CMD_INQUIRY)) {
			MV_DPRINT(("Device %d [0x%x] return status 0x%x\n",pReq->Device_Id, pReq->Cdb[0], pReq->Scsi_Status));
		}
#endif

	switch (pReq->Scsi_Status) {
	case REQ_STATUS_SUCCESS:
		scmd->result = 0x00;
#ifdef SUPPORT_BALDUR
        /* when IO complete, Data_Transfer_Length is the acutal transfer length
         * if it's less than request length, notify OS */
        if (pReq->Data_Transfer_Length < mv_rq_bf_l(scmd)) {
            mv_set_resid(scmd, mv_rq_bf_l(scmd) - pReq->Data_Transfer_Length);
            if (pReq->Data_Transfer_Length < scmd->underflow) {
                scmd->result = (DID_ERROR << 16);
            }
        }
#endif
		break;
	case REQ_STATUS_MEDIA_ERROR:
		scmd->result = (DID_BAD_TARGET << 16);
		break;
	case REQ_STATUS_BUSY:
		scmd->result = (DID_BUS_BUSY << 16);
		break;
	case REQ_STATUS_NO_DEVICE:
		scmd->result = (DID_NO_CONNECT << 16);
		break;
	case REQ_STATUS_HAS_SENSE:
		scmd->result  = (DID_OK << 16) | SAM_STAT_CHECK_CONDITION;

		if (scmd->cmnd[0]== 0x85 || scmd->cmnd[0]== 0xa1)
			break;

		if (((MV_PU8) senseBuffer)[0] >= 0x72) {
			MV_DPRINT(("dev %d MV Sense: response %x SK %s  "
		       		  "ASC %x ASCQ %x.\n\n", mv_scmd_target(scmd), ((MV_PU8) senseBuffer)[0],
		       		MV_DumpSenseKey(((MV_PU8) senseBuffer)[1]),
		       		((MV_PU8) senseBuffer)[2],((MV_PU8) senseBuffer)[3]));
		} else {
			MV_DPRINT(("dev:%d MV Sense: response %x SK %s "
		       		  "ASC %x ASCQ %x.\n\n", mv_scmd_target(scmd), ((MV_PU8) senseBuffer)[0],
		       		MV_DumpSenseKey(((MV_PU8) senseBuffer)[2]),
		       		((MV_PU8) senseBuffer)[12],((MV_PU8) senseBuffer)[13]));
		}
		break;
#if (defined(SUPPORT_DIF) || defined(SUPPORT_DIX)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
case REQ_STATUS_DIF_GUARD_ERROR:
	case REQ_STATUS_DIF_REF_TAG_ERROR:
	case REQ_STATUS_DIF_APP_TAG_ERROR:
		mv_eedp_error_handling(scmd, pReq->Scsi_Status);
		break;
#endif
	case REQ_STATUS_FROZEN:
		scmd->result = (DID_TARGET_FAILURE << 16);
		break;
	default:
		scmd->result = DID_ERROR << 16;
		break;
	}
	if(scmd && scmd->scsi_done) {
#if defined(MV_VMK_ESX35) || defined(MV_VMK_ESXI5)
		spin_unlock(&phba->desc->hba_desc->global_lock);
#endif

		scmd->scsi_done(scmd);

#if defined(MV_VMK_ESX35) || defined(MV_VMK_ESXI5)
		spin_lock(&phba->desc->hba_desc->global_lock);
#endif
	} else
		MV_DPRINT(("scmd %p no scsi_done.\n",scmd));
}

/* This should be the _only_ os request exit point. */
static void hba_req_callback(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)This;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req_Scmd;

	/* Return this request to OS. */
	mv_complete_request(phba, scmd, pReq);
	atomic_dec(&phba->Io_Count);
#ifdef SUPPORT_MODULE_CONSOLIDATE
	ModConsolid_PushFireRequest (phba->PCC_Extension, pReq);
#endif

#ifdef USE_REQ_POOL
	hba_req_cache_free(phba,pReq);
#else
	res_free_req_to_pool(phba->req_pool, pReq);
#endif
}

static int scsi_cmd_to_req_conv(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq)
{
#if (defined(SUPPORT_DIF) || defined(SUPPORT_DIX)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	mv_setup_eedp(scmd, pReq);
#endif
	/*
	 * Set three flags: CMD_FLAG_NON_DATA
	 *                  CMD_FLAG_DATA_IN
	 *                  CMD_FLAG_DMA
	 * currently data in/out all go thru DMA
	 */
	pReq->Cmd_Flag = 0;
	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		//pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
		break;
	case DMA_FROM_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
	case DMA_TO_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DMA;
		break;
	case DMA_BIDIRECTIONAL :
		MV_DPRINT(( " unexpected DMA_BIDIRECTIONAL.\n"));
		break;
	default:
		break;
	}

	/* max CDB length set for 32 */
	memset(pReq->Cdb, 0, MAX_CDB_SIZE);
	
#ifdef USE_OS_TIMEOUT_VALUE
#if LINUX_VERSION_CODE >KERNEL_VERSION(2, 6, 27)
	pReq->Time_Out = jiffies_to_msecs(scsi_cmd_to_rq(scmd)->timeout)/1000;
#else
	#if (LINUX_VERSION_CODE ==KERNEL_VERSION(2, 6, 27) && (IS_OPENSUSE_SLED_SLES))
		pReq->Time_Out = jiffies_to_msecs(scsi_cmd_to_rq(scmd)->timeout)/1000;
	#else
		pReq->Time_Out = jiffies_to_msecs(scmd->timeout_per_command)/1000;
	#endif
#endif
#endif
	switch (scmd->cmnd[0]) {
/* per smartctl, it sets SCSI_TIMEOUT_DEFAULT to 6 , but for captive mode, we extends to 60 HZs */
	case SCSI_CMD_ATA_PASSTHRU_16:
		if (scmd->cmnd[14] != ATA_CMD_PM_CHECK)
			pReq->Time_Out = 60;
		pReq->Cmd_Flag = hba_parse_ata_protocol(scmd);
		break;
	case SCSI_CMD_ATA_PASSTHRU_12:
		if (scmd->cmnd[9] != ATA_CMD_PM_CHECK)
			pReq->Time_Out = 60;
		pReq->Cmd_Flag = hba_parse_ata_protocol(scmd);
		break;
	default:
		break;
	}
	memcpy(pReq->Cdb, scmd->cmnd, scmd->cmd_len);

	pReq->Data_Buffer = mv_rq_bf(scmd);
#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	if (mv_prot_use_sg(scmd))
		pReq->Data_Transfer_Length = mv_rq_bf_l(scmd) + mv_prot_bf_l(scmd);
	else {
		pReq->Data_Transfer_Length = mv_rq_bf_l(scmd);
		if (scmd->cmnd[0] == SCSI_CMD_READ_10) {
			pReq->Cdb[1] = 0x00; //temp solution
		}
	}
#else
	pReq->Data_Transfer_Length = mv_rq_bf_l(scmd);
#endif
	pReq->Sense_Info_Buffer = scmd->sense_buffer;
	pReq->Sense_Info_Buffer_Length = SCSI_SENSE_BUFFERSIZE;

	SGTable_Init(&pReq->SG_Table, 0);
	generate_sg_table(phba, scmd, &pReq->SG_Table);
#if defined(HAVE_HW_COMPLIANT_SG)
	//pReq->SG_Table.Flag |= SGT_FLAG_PRDT_IN_HOST;
	pReq->SG_Table.prdt_bus_addr.parts.low = pReq->bus_addr.parts.low;//(MV_U32)pReq->SG_Table.Entry_Ptr; //FIXME
	pReq->SG_Table.prdt_bus_addr.parts.high = pReq->bus_addr.parts.high;//0;
#endif

	MV_SetLBAandSectorCount(pReq);

	pReq->Req_Type      = REQ_TYPE_OS;
	pReq->Org_Req_Scmd       = scmd;
	pReq->Tag           = scsi_cmd_to_rq(scmd)->tag;
	pReq->Scsi_Status   = REQ_STATUS_PENDING;
	pReq->Completion    = hba_req_callback;
	pReq->Cmd_Initiator = phba;
	//pReq->Scsi_Status   = REQ_STATUS_INVALID_REQUEST;
#ifdef SUPPORT_MUL_LUN
	pReq->Device_Id     = (MV_U16)scmd->device->hostdata;
#else
	pReq->Device_Id     = __MAKE_DEV_ID(mv_scmd_target(scmd),
					    mv_scmd_lun(scmd));
#endif
    pReq->hard_reset_cnt = 0;
	return 0;
}

#if defined(MV_BLK_IOCTL)
extern void * kbuf_array[512];
static void hba_ioctl_req_callback(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)This;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req_Scmd;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
/*openSUSE 11.1 SLES 11 SLED 11*/
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(!IS_OPENSUSE_SLED_SLES))
        /* Return this request to OS. */
	scmd->eh_timeout.expires = jiffies + 1;
	add_timer(&scmd->eh_timeout);
#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19) || LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
	/* 
	 * In kernel 5.x, scsi mq is a default setting, and the scsi_request tag has its own purpuse. 
	 * Do not modify it
	 */
	//scmd->request->tag = pReq->Scsi_Status;
#else
	scsi_cmd_to_rq(scmd)->rq_status = pReq->Scsi_Status;
#endif
        scsi_req(scsi_cmd_to_rq(scmd))->sense = pReq->Sense_Info_Buffer;
        scsi_req(scsi_cmd_to_rq(scmd))->sense_len = pReq->Sense_Info_Buffer_Length;
	mv_complete_request(phba, scmd, pReq);
	atomic_dec(&phba->Io_Count);
#ifdef USE_REQ_POOL
	hba_req_cache_free(phba,pReq);
#else
	res_free_req_to_pool(phba->req_pool, pReq);
#endif
}

//extern unsigned char mvcdb[512][16];

static int scsi_ioctl_cmd_adjust(struct hba_extension *phba,
                                struct scsi_cmnd *scmd,
                                PMV_Request pReq)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
/*openSUSE 11.1 SLES 11 SLED 11*/
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(!IS_OPENSUSE_SLED_SLES))
	del_timer(&scmd->eh_timeout);
#endif
#endif
#ifdef SUPPORT_SES
	if( __is_scsi_cmd_rcv_snd_diag(pReq->Cdb[0])) {
		/* SES-2, PCV set to one (0x01) and PF set to one (0x10) with
		* data transfer and GOOD status, so can't bypass */
		if (pReq->Cdb[1] == 0x0) {
			if(pReq->Cdb[0] ==API_SCSI_CMD_RCV_DIAG_RSLT)
				pReq->Cmd_Flag = CMD_FLAG_DATA_IN;
			if(pReq->Cdb[0] ==API_SCSI_CMD_SND_DIAG  )
				pReq->Cmd_Flag &= ~CMD_FLAG_DATA_IN;
			goto bypass;
		}
	}
#endif
#ifdef SUPPORT_SES
bypass:
#endif
        //pReq->Sense_Info_Buffer = scsi_req(scmd->request)->sense;
        scsi_req(scsi_cmd_to_rq(scmd))->result = 0;
        pReq->Sense_Info_Buffer_Length = scsi_req(scsi_cmd_to_rq(scmd))->sense_len;
	pReq->Req_Type = REQ_TYPE_INTERNAL;
	pReq->Org_Req = pReq;
	pReq->Completion =  hba_ioctl_req_callback;
	return 0;
}
#endif

static void hba_shutdown_req_cb(MV_PVOID this, PMV_Request req)
{
	struct hba_extension *phba = (struct hba_extension *) this;
	#ifdef SUPPORT_REQUEST_TIMER
	if(req!=NULL)
	{
		MV_DPRINT(( "Shutdown HBA timer!\n"));
		hba_remove_timer_sync(req);
	}
	#endif

#ifdef USE_REQ_POOL
	hba_req_cache_free(phba,req);
#else
	res_free_req_to_pool(phba->req_pool, req);
#endif
	atomic_dec(&phba->Io_Count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 0);
#else
	complete(&phba->desc->hba_desc->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
/* will wait for atomic value atomic to become zero until timed out */
/* return how much 'timeout' is left or 0 if already timed out */
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout)
{
	unsigned intv = HZ/20;

	while (timeout) {
		if (0 == atomic_read(atomic))
			break;

		if (timeout < intv)
			intv = timeout;
		set_current_state(TASK_INTERRUPTIBLE);
		timeout -= (intv - schedule_timeout(intv));
	}
	return timeout;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

#ifdef CACHE_MODULE_SUPPORT
static void _hba_send_shutdown_req(PHBA_Extension phba)
{
	unsigned long flags;
	PMV_Request pReq;

	/*Send MV_REQUEST to do something.*/
#ifdef USE_REQ_POOL
	pReq = hba_req_cache_alloc(phba);
#else
	pReq = res_get_req_from_pool(phba->req_pool);
#endif
	if (NULL == pReq) {
		MV_PRINT("%s : cannot allocate memory for req.\n",
		       mv_product_name);
		return;
	}
	if (atomic_read(&phba->Io_Count) || atomic_read(&phba->Ioctl_Io_Count))
	{
		MV_DPRINT(("Shutdown CACHE have running request, wait...\n"));
		msleep(100);
	}
	WARN_ON(atomic_read(&phba->Io_Count) != 0);
	WARN_ON(atomic_read(&phba->Ioctl_Io_Count) != 0);
	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = NULL;
	pReq->Completion = hba_shutdown_req_cb;
	pReq->Req_Type = REQ_TYPE_OS;
	pReq->Cmd_Flag = 0;
	//pReq->Cmd_Flag |= CMD_FLAG_NON_DATA;
	pReq->Sense_Info_Buffer_Length = 0;
	pReq->Data_Transfer_Length = 0;
	pReq->Data_Buffer = NULL;
	pReq->Sense_Info_Buffer = NULL;

	pReq->LBA.value = 0;
	pReq->Sector_Count = 0;

	pReq->Scsi_Status = REQ_STATUS_SUCCESS;

	SGTable_Init(&pReq->SG_Table, 0);
	memset(pReq->Context, 0, sizeof(MV_PVOID) * MAX_POSSIBLE_MODULE_NUMBER);

	MV_DPRINT(("send SHUTDOWN request to CACHE.\n"));
	pReq->Cdb[0] = APICDB0_ADAPTER;
	pReq->Cdb[1] = APICDB1_ADAPTER_POWER_STATE_CHANGE;
	pReq->Cdb[2] = 0;


	/* Must lock IRQ during flush cache to hard disk, for we use one BIG same global_lock */
	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	atomic_inc(&phba->Io_Count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);

	MV_DPRINT(("wait finished send_shutdown_cache_req.\n"));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync, 10 * HZ))
		goto err_shutdown_cache_req;
#else
	if(0 == wait_for_completion_timeout(&phba->desc->hba_desc->cmpl, 10*HZ))
		goto err_shutdown_cache_req;
#endif
	MV_DPRINT(("finished send_shutdown_cache_req.\n"));
	return;
err_shutdown_cache_req:
	MV_PRINT("send_shutdown_cache_req failed.\n");
}
#endif /* CACHE_MODULE_SUPPORT */

void hba_send_shutdown_req(MV_PVOID extension)
{
	unsigned long flags;
	PMV_Request pReq;
	MV_U32 timeout=100;
	struct hba_extension *phba = (struct hba_extension *)extension;
#ifdef CACHE_MODULE_SUPPORT
	if(!phba->desc->hba_desc->RunAsNonRAID)
		_hba_send_shutdown_req(phba);
#endif

#ifdef USE_REQ_POOL
	pReq = hba_req_cache_alloc(phba);
#else
	pReq = res_get_req_from_pool(phba->req_pool);
#endif
	if (NULL == pReq) {
		MV_PRINT("%s : cannot allocate memory for req.\n",
		       mv_product_name);
		return;
	}

	while ((atomic_read(&phba->Io_Count) || atomic_read(&phba->Ioctl_Io_Count)) && timeout) {
//		MV_DPRINT(("have running request, Io_Count = %d,, ioctl_count=%d, wait...\n",
//			phba->Io_Count,phba->Ioctl_Io_Count));
		msleep(100);
		timeout--;
	}
//	WARN_ON(phba->Io_Count != 0);
//	WARN_ON(phba->Ioctl_Io_Count != 0);
	pReq->Device_Id = VIRTUAL_DEVICE_ID;
	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = pReq;
	pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
	pReq->Completion = hba_shutdown_req_cb;

#ifdef RAID_DRIVER
	if (!phba->desc->hba_desc->RunAsNonRAID) {
		MV_DPRINT(("Send SHUTDOWN request to RAID.\n"));
		pReq->Cdb[0] = APICDB0_LD;
		pReq->Cdb[1] = APICDB1_LD_SHUTDOWN;
		pReq->Req_Type = REQ_TYPE_INTERNAL;
	} else
#endif
	{
		MV_DPRINT(("Send SHUTDOWN request to CORE.\n"));
		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		pReq->Cdb[1] = CDB_CORE_MODULE;
		pReq->Cdb[2] = CDB_CORE_SHUTDOWN;
		pReq->Req_Type = REQ_TYPE_OS;
	}

	atomic_inc(&phba->Io_Count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);

	MV_DPRINT(("wait finished send_shutdown_req.\n"));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync, 10 * HZ))
		goto err_shutdown_req;
#else
	if(0 == wait_for_completion_timeout(&phba->desc->hba_desc->cmpl, 10 * HZ))
		goto err_shutdown_req;
#endif
	MV_DPRINT(("finished send_shutdown_req.\n"));


	return;
err_shutdown_req:
	MV_PRINT("hba_send_shutdown_req failed.\n");
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7)
/*openSUSE 11.1 SLES 11 SLED 11*/
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&& (IS_OPENSUSE_SLED_SLES))
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_DONE;

}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static enum scsi_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
#if 0
	MV_DPRINT(("command time out, device %d,  cdb[%2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x].\n",
		__MAKE_DEV_ID(mv_scmd_target(cmd),
					    mv_scmd_lun(cmd)),
		cmd->cmnd[0],
		cmd->cmnd[1],
		cmd->cmnd[2],
		cmd->cmnd[3],
		cmd->cmnd[4],
		cmd->cmnd[5],
		cmd->cmnd[6],
		cmd->cmnd[7],
		cmd->cmnd[8],
		cmd->cmnd[9],
		cmd->cmnd[10],
		cmd->cmnd[11]
		));
#endif
	return (ret)?EH_RESET_TIMER:EH_NOT_HANDLED;

}
#else
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
	MV_DPRINT(("mv_linux_timed_out: scmd=%x.\n", cmd));
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_DONE;
}
#endif
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) */

void dump_scsi_cmd(const char * prefix, struct scsi_cmnd * scmd)
{
	int i = 0;
	MV_PRINT("%s dump cdb[",prefix);
	for(i = 0; i < 16; i++)
		MV_PRINT("0x%02x ", scmd->cmnd[i]);
	MV_PRINT("]\n");
}
#ifdef RAID_DRIVER
extern void RAID_HandleWaitingReq(PRAID_Core pRaidCore);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
//static int mv_linux_queue_command_lck(struct scsi_cmnd *scmd,
//				  void (*done) (struct scsi_cmnd *))
static int mv_linux_queue_command(struct Scsi_Host *shost, struct scsi_cmnd *scmd)
#else
static int mv_linux_queue_command(struct scsi_cmnd *scmd,
				  void (*done) (struct scsi_cmnd *))
#endif	
{
	struct Scsi_Host *host = mv_scmd_host(scmd);
	struct hba_extension *hba = *((struct hba_extension * *) host->hostdata);
	PMV_Request req;
	unsigned long flags;
	unsigned long irq_flags = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
	if (done == NULL) {
		MV_PRINT( ": in queuecommand, done function can't be NULL\n");
		return 0;
    	}
#endif

	scmd->result = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
	scmd->scsi_done = done;
#endif
	MV_SCp(scmd)->bus_address = 0;
	MV_SCp(scmd)->mapped = 0;
	MV_SCp(scmd)->map_atomic = 0;

	if (mv_scmd_channel(scmd)) {
		scmd->result = DID_BAD_TARGET << 16;
		goto done;
	}
#if 0
	dump_scsi_cmd(__func__, scmd);
#endif
	/*
	 * Get mv_request resource and translate the scsi_cmnd request
	 * to mv_request.
	 */
#ifdef USE_REQ_POOL
	req = hba_req_cache_alloc(hba);
#else
	req = res_get_req_from_pool(hba->req_pool);
#endif

	if (req == NULL) {
		//MV_PRINT("No sufficient request.\n");
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	if (scsi_cmd_to_req_conv(hba, scmd, req)) {
		/*
		 * Even TranslateOSRequest failed,
		 * it still should set some of the variables to the MV_Request
		 * especially MV_Request.Org_Req and MV_Request.Scsi_Status;
		 */
		MV_DPRINT(( "ERROR - Translation from OS Request failed.\n"));
		hba_req_callback(hba, req);
		return 0;
	}

	/* transfer length set to 0 shall not considered as an error */
	if ((req->Sector_Count == 0) && (SCSI_IS_READ(req->Cdb[0])
		|| SCSI_IS_WRITE(req->Cdb[0]) || SCSI_IS_VERIFY(req->Cdb[0]))) {
		req->Data_Transfer_Length = 0;
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		atomic_inc(&hba->Io_Count);
		hba_req_callback(hba, req);
		return 0;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
	spin_unlock_irq(host->host_lock);
#endif
	atomic_inc(&hba->Io_Count);

	MV_ASSERT(hba->State == DRIVER_STATUS_STARTED);
	
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);

#ifdef CORE_NO_RECURSIVE_CALL
	{
		MV_PVOID core = (MV_PVOID)HBA_GetModuleExtension(hba, MODULE_CORE);
		core_send_mv_request(core, req);
	}
#endif

#ifdef RAID_DRIVER
	//extern void RAID_HandleWaitingReq(PRAID_Core pRaidCore);
	/* Run stress on VD, hotplug HDD under expander, saw system hang.
	 * Found requests are hold in RAID while Core Driver is free. So add push. */
	if (!hba->RunAsNonRAID)// added by liang, if set NonRAID feature, below case should not happened
	{
		PRAID_Core pRaidCore;
		pRaidCore = (PRAID_Core)HBA_GetModuleExtension(hba, MODULE_RAID);;
		if(pRaidCore!= NULL)
			RAID_HandleWaitingReq(pRaidCore);
	}
#endif		/* RAID_DRIVER */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
	spin_lock_irq(host->host_lock);
#endif

	return 0;
done:
        scmd->scsi_done(scmd);
        return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
//static DEF_SCSI_QCMD(mv_linux_queue_command)
#endif

#if defined(MV_VMK_ESX35) || defined(MV_VMK_ESXI5)
#ifdef __VMKLNX__
static int mv_linux_detect(struct scsi_host_template *sht)
#else
static int mv_linux_detect (Scsi_Host_Template * sht)
#endif
{
	return __mv_get_adapter_count();
}

#ifdef MV_VMK_ESXI5
int mv_linux_abort(struct scsi_cmnd *scmd)
#else
int mv_linux_abort(Scsi_Cmnd * scmd)
#endif
{
	printk("[%s]\n",__func__);
#ifdef MV_VMK_ESXI5
	return FAILED;
#else
	return SCSI_ABORT_ERROR;
#endif
}
#endif

static int mv_linux_reset (struct scsi_cmnd *scmd)
{
	MV_PRINT("__MV__ reset handler %p.\n", scmd);
	return FAILED;
}

#if defined(SUPPORT_SG_RESET) || defined(__VMKLNX__)
#define MV_HBA_RESET				0
#define MV_BUS_RESET				1
#define MV_TARGET_RESET				2
#define MV_TASK_MANAGEMENT			3

void tm_cmd_callback(MV_PVOID This, PMV_Request req)
{
		struct hba_extension *hba = (struct hba_extension *) This;
		hba->desc->hba_desc->ioctl_cmpl_Scsi_Status = req->Scsi_Status;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 0);
#else
		complete(&hba->desc->hba_desc->ioctl_cmpl);
#endif

#ifdef USE_REQ_POOL
		hba_req_cache_free(hba,req);
#else
		res_free_req_to_pool(hba->req_pool, req);
#endif
}

static int tm_cmd_to_req_conv(struct hba_extension *phba,
							struct scsi_cmnd *scmd, PMV_Request pReq,
							MV_U8 ctl_func, MV_U8 tm_func)
{
	pReq->Cmd_Flag = 0;
	/* max CDB length set for 32 */
	memset(pReq->Cdb, 0, MAX_CDB_SIZE);

    pReq->Time_Out = 30;
	pReq->Req_Type		= REQ_TYPE_OS;
	pReq->Org_Req_Scmd	= scmd;
	pReq->Tag			= scsi_cmd_to_rq(scmd)->tag;
	pReq->Scsi_Status	= REQ_STATUS_PENDING;
	pReq->Completion	= tm_cmd_callback;
	pReq->Cmd_Initiator = phba;

#ifdef SUPPORT_MUL_LUN
	pReq->Device_Id 	= get_id_by_targetid_lun(phba,mv_scmd_target(scmd),mv_scmd_lun(scmd));
#else
	pReq->Device_Id 	= __MAKE_DEV_ID(mv_scmd_target(scmd),
						mv_scmd_lun(scmd));
#endif
	switch (ctl_func) {
	case MV_HBA_RESET:
		/*not support yet*/
		return -1;
	case MV_BUS_RESET:
#ifdef SUPPORT_SG_RESET
		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		pReq->Cdb[1] = CDB_HBA_RESET;
		pReq->Cdb[2] = CDB_HBA_BUS_RESET;
		pReq->Cmd_Flag = 0;		
		break;
#endif
	case MV_TARGET_RESET:
		tm_func= CSMI_SAS_SSP_LOGICAL_UNIT_RESET;
	case MV_TASK_MANAGEMENT:
#ifdef __VMKLNX__
		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		pReq->Cdb[1] = CDB_CORE_MODULE;
		pReq->Cdb[2] = CDB_CORE_TASK_MGMT;
		
#else
        pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
        pReq->Cdb[1] = CDB_HBA_RESET;
		pReq->Cdb[2] = CDB_HBA_DEVICE_RESET;

#endif
	    pReq->Cdb[3] = (MV_U8)(scsi_cmd_to_rq(scmd)->tag&0xff);
		pReq->Cdb[4] = (MV_U8)(scsi_cmd_to_rq(scmd)->tag>>8);
		pReq->Cdb[5] = tm_func;
		pReq->Cmd_Flag = 0;

	}

	return 0;
}

static int mv_linux_ctl (struct scsi_cmnd *scmd, MV_U8 ctl_func, MV_U8 tm_func)
{
	struct Scsi_Host *host = mv_scmd_host(scmd);
	struct hba_extension *hba = *((struct hba_extension * *) host->hostdata);
	PMV_Request req;
	unsigned long flags;
	MV_U8 Scsi_Status=REQ_STATUS_PENDING;
#ifdef __VMKLNX__
	scsi_print_command(scmd);
	return SUCCESS;
#endif

	if (mv_scmd_channel(scmd)) 
		return FAILED;

#ifdef USE_REQ_POOL
	req = hba_req_cache_alloc(hba);
#else
	req = res_get_req_from_pool(hba->req_pool);
#endif
	if (req == NULL)
		return FAILED;
	if (tm_cmd_to_req_conv(hba, scmd, req, ctl_func, tm_func)) {
		/*
		 * Even TranslateOSRequest failed,
		 * it still should set some of the variables to the MV_Request
		 * especially MV_Request.Org_Req and MV_Request.Scsi_Status;
		 */
		MV_PRINT(( "ERROR - Translation from OS Request failed.\n"));
		tm_cmd_callback(hba, req);
		return FAILED;
	}

	atomic_inc(&hba->Ioctl_Io_Count);
	hba->desc->hba_desc->ioctl_cmpl_Scsi_Status = req->Scsi_Status;
	if (hba->State != DRIVER_STATUS_STARTED) {
		MV_ASSERT(0);
	} else {
		hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	}
#ifdef CORE_NO_RECURSIVE_CALL
	{
		MV_PVOID core = (MV_PVOID)HBA_GetModuleExtension(hba, MODULE_CORE);
		core_send_mv_request(core, req);
	}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (!__hba_wait_for_atomic_timeout(&hba->desc->hba_desc->hba_sync,60*HZ))
#else
	if (!wait_for_completion_timeout(&hba->desc->hba_desc->ioctl_cmpl,60*HZ))
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	{
#ifdef USE_REQ_POOL
		hba_req_cache_free(hba,req);
#else
		res_free_req_to_pool(hba->req_pool, req);
#endif
	}
	Scsi_Status = hba->desc->hba_desc->ioctl_cmpl_Scsi_Status;
	if (atomic_read(&hba->Ioctl_Io_Count)) {
		atomic_dec(&hba->Ioctl_Io_Count);
	}

	if (/*req->Scsi_Status*/Scsi_Status == REQ_STATUS_SUCCESS){
		return SUCCESS;
	} else {
		return FAILED;
	}
}

static int mv_device_reset (struct scsi_cmnd *scmd)
{
	MV_PRINT("__MV__ device reset handler %p.\n", scmd);
#ifdef __VMKLNX__
	if (scmd->vmkflags & VMK_FLAGS_USE_LUNRESET)
		return mv_linux_ctl(scmd, MV_TASK_MANAGEMENT, CSMI_SAS_SSP_LOGICAL_UNIT_RESET);
	else
#endif
		return mv_linux_ctl(scmd, MV_TASK_MANAGEMENT, CSMI_SAS_SSP_LOGICAL_UNIT_RESET);
}

static int mv_bus_reset (struct scsi_cmnd *scmd)
{
	MV_PRINT("__MV__ bus reset handler %p.\n", scmd);
	return mv_linux_ctl(scmd, MV_BUS_RESET, 0);
}
#endif

#ifdef __VMKLNX__
static int mv_host_reset (struct scsi_cmnd *scmd)
{
        MV_PRINT("__MV__ host reset handler %p.\n", scmd);
        return SUCCESS;
}

#endif


struct mv_lu *mv_get_avaiable_device(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun)
{
#ifdef SUPPORT_MUL_LUN
	MV_U16 id=0;
	struct mv_lu * lu=NULL;
	for (id =0; id < MV_MAX_TARGET_NUMBER; id++) {
		lu = &hba->mv_unit[id];
		if (lu && (lu->sdev == NULL) && (lu->lun == 0xFFFF)&& (lu->target_id == 0xFFFF)) {
			return lu;
		}
	}
	MV_PRINT("invalid target id %d, lun %d.\n",target_id, lun);
	return NULL;
#else
	if (target_id >= MV_MAX_TARGET_NUMBER)
		return NULL;
	else
		return &hba->mv_unit[target_id];
#endif
}

static int mv_scsi_slave_alloc(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	struct mv_lu * lu = mv_get_avaiable_device (hba, sdev->id, sdev->lun);
	MV_U16 base_id;
	if(lu == NULL)
		return -1;

	lu->sdev = sdev;
	lu->lun = sdev->lun;
	lu->target_id = sdev->id;
#ifdef SUPPORT_MUL_LUN
	base_id = get_id_by_targetid_lun(hba, sdev->id, sdev->lun);
	if (base_id == 0xFFFF) {
		MV_DPRINT(("device %d-%d is not exist.\n", sdev->id, sdev->lun));
		return -1;
	}
	sdev->hostdata = (void *)base_id;
	
	sdev->scsi_level=SCSI_SPC_2;
#endif

	return 0;
}

struct mv_lu *mv_get_device_by_target_lun(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun)
{
#ifdef SUPPORT_MUL_LUN
	MV_U16 id=0;
	struct mv_lu * lu=NULL;
	for (id =0; id < MV_MAX_TARGET_NUMBER; id++) {
		lu = &hba->mv_unit[id];
		if (lu && lu->sdev && (lu->lun == lun) && (lu->target_id == target_id)) {
			return lu;
		}
	}
	return NULL;
#else
	if (target_id >= MV_MAX_TARGET_NUMBER)
		return NULL;
	else
		return &hba->mv_unit[target_id];
#endif
}


static void mv_scsi_slave_destroy(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	struct mv_lu *lu = mv_get_device_by_target_lun (hba, sdev->id, sdev->lun);
	if(lu == NULL)
		return;
	lu->sdev = NULL;
	lu->lun = 0xFFFF;
	lu->target_id = 0xFFFF;

	return;
}

static void hba_send_ioctl_cb(MV_PVOID this, PMV_Request req)
{
	struct hba_extension *phba = (struct hba_extension *) this;
#ifdef SUPPORT_REQUEST_TIMER
	if(req!=NULL)
	{
		MV_DPRINT(( "Io control HBA timer!\n"));
		hba_remove_timer_sync(req);
	}
#endif
#ifdef USE_REQ_POOL
	hba_req_cache_free(phba,req);
#else
	res_free_req_to_pool(phba->req_pool, req);
#endif
	atomic_dec(&phba->Io_Count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 0);
#else
	complete(&phba->desc->hba_desc->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

void hba_send_internal_ioctl(struct scsi_device *sdev, MV_PVOID extension, MV_PVOID buffer, MV_U8 cdb1)
{
	unsigned long flags;
	PMV_Request pReq;
	struct hba_extension *phba = (struct hba_extension *)extension;

#ifdef USE_REQ_POOL
	pReq = hba_req_cache_alloc(phba);
#else
	pReq = res_get_req_from_pool(phba->req_pool);
#endif
	if (NULL == pReq) {
		MV_PRINT("%s : cannot allocate memory for req.\n",
		       mv_product_name);
		return;
	}

	if (atomic_read(&phba->Io_Count) || atomic_read(&phba->Ioctl_Io_Count)) {
		//MV_DPRINT(("have running request, Io_Count = %d,, ioctl_count=%d, wait...\n",
		//	phba->Io_Count,phba->Ioctl_Io_Count));
		msleep(100);
	}
	WARN_ON(atomic_read(&phba->Ioctl_Io_Count) != 0);
	pReq->Device_Id = VIRTUAL_DEVICE_ID;
	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = pReq;
	pReq->Data_Buffer = buffer;
	pReq->Data_Transfer_Length = sizeof(OS_disk_info);
	pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
	pReq->Completion = hba_send_ioctl_cb;
	pReq->Cdb[0] = APICDB0_IOCONTROL;
	pReq->Cdb[1] = cdb1;
	pReq->Cdb[2] = sdev->id;
	pReq->Cdb[3] = sdev->lun;
	pReq->Req_Type = REQ_TYPE_INTERNAL;
	init_completion(&phba->desc->hba_desc->cmpl);
	atomic_inc(&phba->Io_Count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync, 10 * HZ))
		goto err_get_hdinfo_req;
#else
	if(0 == wait_for_completion_timeout(&phba->desc->hba_desc->cmpl, 10 * HZ))
		goto err_get_hdinfo_req;
#endif
	MV_DPRINT(("finished io control.\n"));
	return;

err_get_hdinfo_req:
	MV_PRINT("io control req failed.\n");
}

#ifndef MV_VMK_ESX35
static int mv_scsi_slave_configure(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	OS_disk_info disk_info;
	int dev_qth = 1;

	MV_ZeroMemory(&disk_info, sizeof(OS_disk_info));
	if (hba->RunAsNonRAID) {
		hba_send_internal_ioctl(sdev, hba, &disk_info, APICDB1_GET_OS_DISK_INFO);
		if (disk_info.queue_depth)
			dev_qth = disk_info.queue_depth;

		/* Not set auto wake up for SATA disk */
		if (disk_info.disk_type != DISK_TYPE_SATA)
			sdev->allow_restart = 1;
	} else
		dev_qth = MV_MAX_REQUEST_PER_LUN;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
	hba_send_internal_ioctl(sdev, hba, &disk_info, APICDB1_GET_OS_DISK_INFO);
	if(disk_info.disk_type == DISK_TYPE_SATA || disk_info.disk_type == DISK_TYPE_RAID){
		sdev->no_write_same = 1;
	}
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
	if (sdev->tagged_supported)
		scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev),
					dev_qth);
	else {
		scsi_adjust_queue_depth(sdev, 0, 1);
		dev_qth = 1;
	}
#else
	if (sdev->tagged_supported)
		scsi_change_queue_depth(sdev, dev_qth);
	else {
		scsi_change_queue_depth(sdev, 1);
		dev_qth = 1;
	}
#endif
	return 0;
}
#endif
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
static int mv_change_queue_depth(struct scsi_device *sdev, int new_depth)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
static int mv_change_queue_depth(struct scsi_device *sdev, int new_depth)
#else
static int mv_change_queue_depth(struct scsi_device *sdev, int new_depth, int reason)
#endif
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	OS_disk_info disk_info;
	int res = 0, dev_qth = 1;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33) && LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
	if (reason != SCSI_QDEPTH_DEFAULT)
		return -EOPNOTSUPP;
#endif
	
	MV_ZeroMemory(&disk_info, sizeof(OS_disk_info));
	if (hba->RunAsNonRAID) {
#ifdef SUPPORT_MUL_LUN
	if(new_depth > MAX_REQUEST_PER_LUN_PERFORMANCE)
		return sdev->queue_depth;
#else
		hba_send_internal_ioctl(sdev, hba, &disk_info, APICDB1_GET_OS_DISK_INFO);
		if (disk_info.queue_depth)
			dev_qth = disk_info.queue_depth;
#endif
	} else
		dev_qth = MV_MAX_REQUEST_PER_LUN;
#ifdef SUPPORT_MUL_LUN
	if (hba->RunAsNonRAID)
		res = new_depth;
	else
		res = min(new_depth, dev_qth);
#else
	res = min(new_depth, dev_qth);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
	if (sdev->tagged_supported) {
		scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), res);
		//hba_send_internal_ioctl(sdev, hba, &hd_info, APICDB1_GET_OS_DISK_INFO);
	} else {
		scsi_adjust_queue_depth(sdev, 0, 1);
		res = 1;
	}
#else
	if (sdev->tagged_supported) {
		scsi_change_queue_depth(sdev, res);
		//hba_send_internal_ioctl(sdev, hba, &hd_info, APICDB1_GET_OS_DISK_INFO);
	} else {
		scsi_change_queue_depth(sdev, 1);
		res = 1;
	}
#endif
	return res;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
static int mv_change_queue_type(struct scsi_device *scsi_dev, int qt)
{
	struct hba_extension *hba = *((struct hba_extension * *) scsi_dev->host->hostdata);
	OS_disk_info disk_info;

	switch (qt) {
	case MSG_ORDERED_TAG:
		disk_info.queue_type = TCQ_QUEUE_TYPE_ORDERED;
		break;
	case 0:
		/*fall through*/
	case MSG_SIMPLE_TAG:
		disk_info.queue_type = TCQ_QUEUE_TYPE_SIMPLE;
		break;
	default:
		break;
	}

	if (hba->RunAsNonRAID) {
		hba_send_internal_ioctl(scsi_dev, hba, &disk_info, APICDB1_SET_OS_DISK_QUEUE_TYPE);
	}
	if (!scsi_dev->tagged_supported)
		return 0;

	scsi_deactivate_tcq(scsi_dev, 1);

	scsi_set_tag_type(scsi_dev, qt);
	scsi_activate_tcq(scsi_dev, scsi_dev->queue_depth);

	return qt;
}
#endif
#endif

#ifndef MV_VMK_ESX35
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
struct class_device_attribute *mvs_host_attrs[];
#else
struct device_attribute *mvs_host_attrs[];
#endif
#endif

#ifndef MV_VMK_ESX35
static struct scsi_host_template mv_driver_template = {
#else
Scsi_Host_Template mv_driver_template = {
#endif
	.module                      =  THIS_MODULE,
	.name                        =  "Marvell Storage Controller",
	.proc_name                   =  mv_driver_name,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	.show_info			= mv_linux_show_info,
#else
	.proc_info                   =  mv_linux_proc_info,
#endif
	.queuecommand                =  mv_linux_queue_command,
	.eh_host_reset_handler       =  mv_linux_reset,

#if defined(SUPPORT_SG_RESET) || defined(__VMKLNX__)
	.eh_device_reset_handler     =  mv_device_reset,
	.eh_bus_reset_handler        =  mv_bus_reset,
#endif
#ifdef __VMKLNX__
	.eh_host_reset_handler       =  mv_host_reset,
#endif
#ifndef MV_VMK_ESX35
	.slave_alloc                = mv_scsi_slave_alloc,
	.slave_configure         = mv_scsi_slave_configure,
	.slave_destroy            = mv_scsi_slave_destroy,
#endif
#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10)
	.change_queue_depth	= mv_change_queue_depth,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
	.change_queue_type 	= mv_change_queue_type,
#endif
#endif
	.eh_timed_out                =  mv_linux_timed_out,
	.can_queue                   =  MV_MAX_REQUEST_DEPTH,
	.this_id                     =  MV_SHT_THIS_ID,
	.max_sectors                 =  (MV_MAX_TRANSFER_SIZE >> 9),
	.sg_tablesize                =  MV_MAX_SG_ENTRY,
	.cmd_per_lun                 =  MV_MAX_REQUEST_PER_LUN,
	//.use_clustering              =  MV_SHT_USE_CLUSTERING,
	.emulated                    =  MV_SHT_EMULATED,
#if defined(MV_BLK_IOCTL) && !defined(MV_VMK_ESX35)
	.ioctl                       =  mv_new_ioctl,
#endif
#ifdef MV_VMK_ESX35
	.detect                      = mv_linux_detect,
	.abort                       = mv_linux_abort,
#elif defined(MV_VMK_ESXI5)
	.detect				= mv_linux_detect,
	.eh_abort_handler		= mv_linux_abort,
#endif
#ifndef MV_VMK_ESX35
	.shost_attrs		= mvs_host_attrs,
#endif
};

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
static struct scsi_transport_template mv_transport_template = {
};
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */
#ifdef SUPPORT_MSIX_INT
static int mv_request_msix_irq(struct hba_extension *hba,
								int index, unsigned int vector)
{
	int ret;
	struct msix_data *msix_data;

	msix_data = &hba->msix_data[index];
	msix_data->hba = hba;
	msix_data->index = index;
	msix_data->vector = vector;
	if (index == 0) {
		msix_data->root = 0;
		msix_data->queue = 0;
	} else {
		msix_data->root = index/(hba->msix_count/2);
		msix_data->queue = index%(hba->msix_count/2);
	}
	snprintf(msix_data->name, 32, "%s-%d", mv_driver_name, index);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	ret = request_threaded_irq(vector, mv_msix_check_int, mv_msix_intr_handler, 0,
				msix_data->name, msix_data);
	//ret = request_irq(vector, mv_hba_msix_intr_handler, IRQF_SHARED,
	//			msix_data->name, msix_data);
#else
	ret = request_irq(vector, mv_hba_msix_intr_handler, 0,
				msix_data->name, msix_data);
#endif
	if (ret) {
		MV_PRINT("allocate interrupt for msix idx %d vector %d failed\n",
			index, vector);
		return -1;
	}
	msix_data->valid = 1;
	return 0;
}

void mv_free_msix_irq(struct hba_extension *hba)
{
	int i;
	struct msix_data *msix_data;

	for (i = 0; i < hba->msix_count; i++) {
		msix_data = &hba->msix_data[i];
		if (msix_data->valid) {
			//free_cpumask_var(msix_data->affinity_mask);
			free_irq(msix_data->vector, msix_data);
		}
	}
}
#endif

void HBA_ModuleStart(MV_PVOID extension)
{
	struct Scsi_Host *host = NULL;
	struct hba_extension *hba;
	struct mv_adp_desc *hba_desc;
	struct proc_reg_data* data;
#if defined(SUPPORT_MV_SAS_CONTROLLER)
    struct resource *res_irq;
#endif
	int ret;	

	hba = (struct hba_extension *) extension;
	hba_desc = hba->desc->hba_desc;
#ifdef MV_VMK_ESX35
	host = vmk_scsi_register(&mv_driver_template,sizeof(void *),
	                         hba_desc->dev->bus->number, hba_desc->dev->devfn);
#else
	host = scsi_host_alloc(&mv_driver_template, sizeof(void *));
    hba->reg_enabled = mv_reg_enable;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    if (hba->reg_enabled)
    {
		sprintf(hba->proc_name,"%d_register", host->host_no);
		hba->proc_dir = mv_driver_template.proc_dir;
		hba->reg_info = create_proc_entry(hba->proc_name, S_IFREG | S_IRWXU, hba->proc_dir);
		if (NULL == hba->reg_info)
		{	   
		   MV_DPRINT(("%s create proc entry failed.\n", __func__));
		}

		data = kmalloc(sizeof(struct proc_reg_data), GFP_ATOMIC);
		data->hba = hba;
		data->flag = 0;
		hba->reg_info->data = (void *)data;
		hba->reg_info->write_proc = reg_info_write;
		hba->reg_info->read_proc = reg_info_read; 
    }
#endif

#endif
	if (NULL == host) {
		MV_PRINT("%s : Unable to allocate a scsi host.\n",
		       mv_product_name);
		goto err_out;
	}

	*((MV_PVOID *) host->hostdata) = extension;
	hba_desc->hba_host = host;
#ifdef MV_VMK_ESX35
	host->pci_dev = hba_desc->dev;
#endif
#if defined(SUPPORT_MV_SAS_CONTROLLER)
    res_irq = platform_get_resource(hba_desc->dev, IORESOURCE_IRQ, 0);
	host->irq          = res_irq->start;
#else
	host->irq          = hba_desc->dev->irq;
#endif
#ifdef SUPPORT_OEM_PROJECT
	host->max_id	   = MV_MAX_TARGET_NUMBER-1;	//do not report virtual device
#else
	host->max_id       = MV_MAX_TARGET_NUMBER;
#endif
	host->max_lun      = MV_MAX_LUN_NUMBER;
	host->max_channel  = 0;
#ifdef SUPPORT_VAR_LEN_CDB
	host->max_cmd_len  = 32;
#else
	host->max_cmd_len = 16;
#endif
	host->unique_id = host->host_no;
{
	if ((hba->Max_Io) && (hba->Max_Io < MV_MAX_REQUEST_DEPTH)){
		/* set can queue as resource limitation */
		host->can_queue = 1;
	}

	MV_DPRINT(("HBA queue command = %d.\n", host->can_queue));
}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
	host->transportt   = &mv_transport_template;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	{
		hba->msi_enabled = 0;
		MV_DPRINT(( "start install request_irq %d.\n", res_irq->start));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
		ret = request_irq(res_irq->start, mv_intr_handler, IRQF_SHARED,
			  mv_driver_name, hba);
#else
		ret = request_irq(res_irq->start, mv_intr_handler, SA_SHIRQ,
			  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
		if (ret < 0) {
			MV_PRINT("%s : Error upon requesting IRQ %d.\n",
				mv_product_name, res_irq->start);
			goto  err_request_irq;
		}
		MV_DPRINT(( "start install request_irq %d.\n", res_irq->end));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
		ret = request_irq(res_irq->end, mv_intr_handler, IRQF_SHARED,
				mv_driver_name, hba);
#else
		ret = request_irq(res_irq->end, mv_intr_handler, SA_SHIRQ,
				mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
		if (ret < 0) {
			MV_PRINT("%s : Error upon requesting IRQ %d.\n",
				mv_product_name, res_irq->end);
			goto  err_request_irq;
		}
	}
#else
    hba->test_enabled = mv_test_enable;
    switch (mv_int_mode) {
	case 1:
		hba->msi_enabled = 1;
		break;
#ifdef SUPPORT_MSIX_INT
	case 2:
		hba->msix_enabled = 1;
		break;
#endif
	case 0:
	default:
		break;
	
    }
    printk("mv_do_test value is %d, mv_int_mode value is %d\n", mv_test_enable, mv_int_mode);
#if 0
	if (hba->msi_enabled)
		pci_enable_msi(hba->desc->hba_desc->dev);

	MV_DPRINT(( "start install request_irq.\n"));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	ret = request_irq(hba_desc->dev->irq, mv_intr_handler, IRQF_SHARED,
			  mv_driver_name, hba);
#else
	ret = request_irq(hba_desc->dev->irq, mv_intr_handler, SA_SHIRQ,
			  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
	if (ret < 0) {
		MV_PRINT("%s : Error upon requesting IRQ %d.\n",
		       mv_product_name, hba_desc->dev->irq);
		goto  err_request_irq;
	}
	MV_DPRINT(("request_irq has been installed.\n"));
#else
/* add support for msi-x, fs TODO: athena16 device id? */
	MV_DPRINT(( "start install request_irq %x.\n", hba->Device_Id));
#ifdef SUPPORT_MSIX_INT
	if (hba->msix_enabled) {
		int base = 0, i, msix_count;
		u16 message_ctl;

		base = pci_find_capability(hba->desc->hba_desc->dev, PCI_CAP_ID_MSIX);
		if (!base) {
			MV_PRINT("Athena16 not support msi-x.\n");
			hba->msix_enabled = 0;
			goto ENABLE_LEGACY_MODE;
		}
		pci_read_config_word(hba->desc->hba_desc->dev, base + 2, &message_ctl);
		msix_count = (message_ctl & 0x3FF) + 1;

		hba->msix_count = MV_MIN(OSSW_GET_CPU_NUM(),
				MV_MIN(MAX_MULTI_QUEUE * 2, msix_count));
		hba->msix_entries = kzalloc((sizeof(struct msix_entry))*hba->msix_count,
				GFP_KERNEL);
		if (!hba->msix_entries) {
			MV_PRINT("Memory alloc failed.\n");
			hba->msix_enabled = 0;
			goto ENABLE_LEGACY_MODE;
		}
		hba->msix_data = kzalloc((sizeof(struct msix_data))*hba->msix_count,
				GFP_KERNEL);
		if (!hba->msix_data) {
			MV_PRINT("Memory alloc failed.\n");
			kfree(hba->msix_entries);
			hba->msix_enabled = 0;
			goto ENABLE_LEGACY_MODE;
		}
		MV_DPRINT(("Athena16 supports max msix vector %d, allocate %d\n", msix_count, hba->msix_count));
		for (i = 0; i < hba->msix_count; i++) {
			hba->msix_entries[i].entry = i;
		}
#if 0
		ret = pci_enable_msix(hba->desc->hba_desc->dev,
					hba->msix_entries, hba->msix_count);
		if (ret) {
			MV_PRINT("Enable msi-x failed %d.\n", ret);
			kfree(hba->msix_entries);
			kfree(hba->msix_data);
			goto err_request_irq;
		}
#else
		for (;;) {
			ret = pci_enable_msix_exact(hba->desc->hba_desc->dev, hba->msix_entries, hba->msix_count);
			if (ret == 0) {
				break;
			} else if (ret > 0) {
				hba->msix_count = ret;
				continue;
			} else {
				MV_PRINT("Enable msi-x failed %d.\n", ret);
				kfree(hba->msix_entries);
				kfree(hba->msix_data);
				hba->msix_enabled = 0;
				goto ENABLE_LEGACY_MODE;
				break;
			}
		}
#endif
		for (i = 0; i < hba->msix_count; i++) {
			ret = mv_request_msix_irq(hba, i, hba->msix_entries[i].vector);
			if (ret) {
				mv_free_msix_irq(hba);
				pci_disable_msix(hba->desc->hba_desc->dev);
				kfree(hba->msix_entries);
				kfree(hba->msix_data);
				hba->msix_enabled = 0;
				goto ENABLE_LEGACY_MODE;
			} /*else {
				//irq_set_affinity(hba->msix_entries[i].vector, MV_BIT(i));
				if (!alloc_cpumask_var(&hba->msix_data[i].affinity_mask, GFP_KERNEL))
				goto err_request_irq;
				cpumask_set_cpu(i, hba->msix_data[i].affinity_mask);
				irq_set_affinity_hint(hba->msix_entries[i].vector,
					      hba->msix_data[i].affinity_mask);
			}*/
		}
	} else
#endif
	{
		if (hba->msi_enabled){
			pci_enable_msi(hba->desc->hba_desc->dev);
		}
ENABLE_LEGACY_MODE:	
		MV_DPRINT(( "start install request_irq.\n"));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
		ret = request_irq(hba_desc->dev->irq, mv_intr_handler, IRQF_SHARED,
				  mv_driver_name, hba);
#else
		ret = request_irq(hba_desc->dev->irq, mv_intr_handler, SA_SHIRQ,
				  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
		if (ret < 0) {
			MV_PRINT("%s : Error upon requesting IRQ %d.\n",
				   mv_product_name, hba_desc->dev->irq);
			goto  err_request_irq;
		}
		MV_DPRINT(("request_irq has been installed.\n"));
	}
#endif
#endif

#if defined(HARDWARE_XOR)
	{
		MV_DPRINT(( "start install request_irq %d.\n", IRQ_DRAGON_XOR0));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
		ret = request_irq(IRQ_SP2_XOR_IRQ(0), mv_intr_xor_handler, IRQF_SHARED,
			  mv_driver_name, hba);
#else
		ret = request_irq(IRQ_SP2_XOR_IRQ(0),  mv_intr_xor_handler, SA_SHIRQ,
			  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
		if (ret < 0) {
			MV_PRINT("%s : Error upon requesting IRQ %d.\n",
				mv_product_name, IRQ_SP2_XOR_IRQ(0));
			goto  err_request_irq;
		}
		MV_DPRINT(( "start install request_irq %d.\n", IRQ_DRAGON_XOR1));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
		ret = request_irq(IRQ_SP2_XOR_IRQ(1), mv_intr_xor_handler, IRQF_SHARED,
				mv_driver_name, hba);
#else
		ret = request_irq(IRQ_SP2_XOR_IRQ(1), mv_intr_xor_handler, SA_SHIRQ,
				mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
		if (ret < 0) {
			MV_PRINT("%s : Error upon requesting IRQ %d.\n",
				mv_product_name, IRQ_SP2_XOR_IRQ(1));
			goto  err_request_irq;
		}
	}
#endif

	MV_DPRINT(("request_irq has been installed.\n"));

	return ;

err_request_irq:
#ifndef MV_VMK_ESX35
	scsi_host_put(host);
#endif
	hba_desc->hba_host = NULL;
err_out:
	return;
}

#ifndef MV_VMK_ESX35

MV_U16 mv_register_mode = 0;

MV_U16 mv_get_register_mode(void)
{
    return mv_register_mode;
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_register_mode(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_register_mode(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "%d\n", mv_register_mode);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_register_mode(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_register_mode(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif
	int val = 0;

	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "%d", &val) != 1) {
		printk( "Input invalid register mode, please input number:0 or 1.\n");
		return -EINVAL;
	}

	mv_register_mode = val;
	if(mv_register_mode != 0)
	{
		mv_register_mode = 1;
	}

	printk( "set register mode to 0x%x\n",  mv_register_mode);
	return strlen(buffer);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(register_mode,
			 S_IRUGO|S_IWUSR,
			 mvs_show_register_mode,
			 mvs_store_register_mode);
#else
static DEVICE_ATTR(register_mode, S_IRUGO|S_IWUSR,
			 mvs_show_register_mode,
			 mvs_store_register_mode);
#endif

#ifdef SUPPORT_IO_DELAY
MV_U16 io_delay=10;
#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_io_delay(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_io_delay(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif

	int val = 0;

	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "%d", &val) != 1)
		return -EINVAL;

	io_delay = val;
	if(io_delay > 10){
		printk( "io delay time %d seconds is too long, max is 10s\n",  io_delay);
		io_delay = 10;
		return strlen(buffer);
	}
	printk( "set io delay time to %d seconds\n",  io_delay);
	return strlen(buffer);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_io_delay(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_io_delay(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "%d\n", io_delay);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(io_delay,
			 S_IRUGO|S_IWUSR,
			 mvs_show_io_delay,
			 mvs_store_io_delay);
#else
static DEVICE_ATTR(io_delay,
			 S_IRUGO|S_IWUSR,
			 mvs_show_io_delay,
			 mvs_store_io_delay);
#endif


MV_U16 hba_get_io_delay_value(void)
{
	MV_U16 value=0;
	value = io_delay;
	return value;
}

#endif	/* #ifdef SUPPORT_IO_DELAY */

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_show_driver_version(struct class_device *cdev,  char *buffer)
{
#else
static ssize_t
mvs_show_driver_version(struct device *cdev, struct device_attribute *attr,  char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "%s\n", mv_version_linux);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(driver_version,
			 S_IRUGO,
			 mvs_show_driver_version,
			 NULL);
#else
static DEVICE_ATTR(driver_version,
			 S_IRUGO,
			 mvs_show_driver_version,
			 NULL);
#endif

#ifdef SUPPORT_CHANGE_CAN_QUEUE

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_host_can_queue(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_host_can_queue(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hba_extension *hba = *((struct hba_extension * *) shost->hostdata);
	struct mv_adp_desc *hba_desc;
	hba_desc = hba->desc->hba_desc;
	return snprintf(buffer, PAGE_SIZE, "%d\n", hba_desc->hba_host->can_queue);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_host_can_queue(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_host_can_queue(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hba_extension *hba = *((struct hba_extension * *) shost->hostdata);
	struct mv_adp_desc *hba_desc;
	int val = 0;

	hba_desc = hba->desc->hba_desc;
	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "%d", &val) != 1)
		return -EINVAL;

	if(val > MAX_REQUEST_NUMBER_PERFORMANCE - 2){
		printk( "can_queue %d exceeds max vaule:%d\n",  val, MV_MAX_REQUEST_DEPTH);
		hba_desc->hba_host->can_queue = MV_MAX_REQUEST_DEPTH;
		return strlen(buffer);
	} else if (val < 1){
		printk( "can_queue legal value is >= 1\n");
		hba_desc->hba_host->can_queue = 1;
		return strlen(buffer);
	} else
		hba_desc->hba_host->can_queue = val;
	printk( "set host can queue to %d \n",  hba_desc->hba_host->can_queue);
	return strlen(buffer);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(host_can_queue,
			 S_IRUGO|S_IWUSR,
			 mvs_show_host_can_queue,
			 mvs_store_host_can_queue);
#else
static DEVICE_ATTR(host_can_queue, S_IRUGO|S_IWUSR,
			 mvs_show_host_can_queue,
			 mvs_store_host_can_queue);
#endif

#endif	/* #ifdef SUPPORT_CHANGE_CAN_QUEUE */

#ifdef SUPPORT_CHANGE_DEBUG_MODE
MV_U16 mv_debug_mode = CORE_DEBUG_INFO;
#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_debug_mode(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_debug_mode(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "0x%x\n", mv_debug_mode);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_debug_mode(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_debug_mode(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif

	int val = 0;

	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "0x%x", &val) != 1) {
		printk( "Input invalid debug mode, please input hexadecimal number:0x0~0xf.\n");
		return -EINVAL;
	}

	mv_debug_mode = val;
	if(mv_debug_mode > 0xF){
		printk( "Invalid debug mode, close all debug info!\n");
		mv_debug_mode = 0x0;
		return strlen(buffer);
	} else
		printk( "set debug mode to 0x%x\n",  mv_debug_mode);
	return strlen(buffer);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(debug_mode,
			 S_IRUGO|S_IWUSR,
			 mvs_show_debug_mode,
			 mvs_store_debug_mode);
#else
static DEVICE_ATTR(debug_mode, S_IRUGO|S_IWUSR,
			 mvs_show_debug_mode,
			 mvs_store_debug_mode);
#endif

#endif /* #ifdef SUPPORT_CHANGE_DEBUG_MODE */

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
struct class_device_attribute *mvs_host_attrs[] = {
	&class_device_attr_driver_version,
#ifdef SUPPORT_IO_DELAY
	&class_device_attr_io_delay,
#endif

#ifdef SUPPORT_TARGET
	&class_device_attr_target_mode,
#endif
#ifdef SUPPORT_CHANGE_CAN_QUEUE
	&class_device_attr_host_can_queue,
#endif
#ifdef SUPPORT_CHANGE_DEBUG_MODE
	&class_device_attr_debug_mode,
#endif
    &class_device_attr_register_mode,

	NULL,
};
#else
struct device_attribute *mvs_host_attrs[] = {
	&dev_attr_driver_version,
#ifdef SUPPORT_IO_DELAY
	&dev_attr_io_delay,
#endif

#ifdef SUPPORT_TARGET
	&dev_attr_target_mode,
#endif
#ifdef SUPPORT_CHANGE_CAN_QUEUE
	&dev_attr_host_can_queue,
#endif
#ifdef SUPPORT_CHANGE_DEBUG_MODE
	&dev_attr_debug_mode,
#endif
    &dev_attr_register_mode,
    
	NULL,
};
#endif

#if defined(SUPPORT_DIF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
/*
# mv_prot_mask: i
#	- Bit mask of host protection capabilities used to register with the
#	  SCSI mid-layer
#
*/
unsigned int mv_prot_mask =  SHOST_DIF_TYPE1_PROTECTION |SHOST_DIF_TYPE3_PROTECTION
						| SHOST_DIF_TYPE2_PROTECTION;

module_param(mv_prot_mask, uint, 0);
MODULE_PARM_DESC(mv_prot_mask, "host protection mask");
#if defined(SUPPORT_DIX) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
/*
# mv_prot_guard: i
#	- Bit mask of protection guard types to register with the SCSI mid-layer
# 	- Guard types are currently either 1) IP checksum 2) T10-DIF CRC
#
*/
unsigned char mv_prot_guard = SHOST_DIX_GUARD_CRC;
module_param(mv_prot_guard, byte, 0);
MODULE_PARM_DESC(mv_prot_guard, "host protection guard type");
#endif
#endif

#endif

#if defined(SUPPORT_MV_SAS_CONTROLLER)
static struct platform_device_id mvs_soc_table[] = { //platform_device_id_t
	{ "mvs-sp2", 0 },
	{ }	/* terminate list */
};

static struct platform_driver mvs_soc_driver = { //platform_device_t
	.driver		= {
		.name	= mv_driver_name,
		.owner	= THIS_MODULE,
	},
	.id_table	= mvs_soc_table,
	.probe		= mv_probe,
	.remove		= mv_remove,
};
#endif

static int __init sas_hba_init(void)
{
#ifdef MV_VMK_ESX35
	int rc=0;
	if(!vmk_set_module_version(mv_version_linux))
		return 0;
	spin_lock_init(&io_request_lock);
	mv_driver_template.driverLock = &io_request_lock;
#endif
	hba_house_keeper_init();
#ifdef MV_VMK_ESX35
	rc = pci_register_driver(&mv_pci_driver);
	if(rc < 0) {
		spin_lock_destroy(&io_request_lock);
		return -ENODEV;
	}
	rc = scsi_register_module(MODULE_SCSI_HA,&mv_driver_template);
	if(rc){
		spin_lock_destroy(&io_request_lock);
		pci_unregister_driver(&mv_pci_driver);
	}
	return 0;
#else
#if defined(SUPPORT_MV_SAS_CONTROLLER)
    return platform_driver_register(&mvs_soc_driver);
#else
    return pci_register_driver(&mv_pci_driver);
#endif
#endif
}

static void __exit sas_hba_exit(void)
{
#if defined(SUPPORT_MV_SAS_CONTROLLER)
	platform_driver_unregister(&mvs_soc_driver);
#else
	pci_unregister_driver(&mv_pci_driver);
#endif
	MV_DPRINT(("sas_hba_exit: before hba_house_keeper_exit\n"));
	hba_house_keeper_exit();
#ifdef MV_VMK_ESX35
	spin_lock_destroy(&io_request_lock);
#endif
}

MODULE_AUTHOR ("Marvell Technolog Group Ltd.");
MODULE_DESCRIPTION ("Marvell SAS hba driver");

MODULE_LICENSE("GPL");
MODULE_VERSION(mv_version_linux);
MODULE_DEVICE_TABLE(pci, mv_pci_ids);
module_init(sas_hba_init);
module_exit(sas_hba_exit);

