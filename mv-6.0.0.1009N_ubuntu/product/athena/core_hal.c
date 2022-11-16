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

#include "mv_config.h"
#include "core_internal.h"
#include "core_hal.h"
#include "core_manager.h"
#include "core_protocol.h"
#include "core_util.h"
#include "hba_inter.h"
#include "core_error.h"
#ifdef FREY_B2_PORT1_PORT5_WORKAROUND
#include "Pcieregs.h"
#include "Regdefs.h"
#endif
extern MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req);
MV_VOID mv_set_sas_addr(MV_PVOID root_p, MV_PVOID phy_p, MV_PU8 sas_addr)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U64 val64;

	MV_CopyMemory(&val64.value, sas_addr, 8);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
	WRITE_PORT_CONFIG_DATA(root, phy, val64.parts.high);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
	WRITE_PORT_CONFIG_DATA(root, phy, val64.parts.low);
}

MV_VOID mv_set_dev_info(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	reg &= 0xffff0000;
	reg |= (0x1800|phy->asic_id);//Add partial and slumber Capable bits
	WRITE_PORT_CONFIG_DATA(root, phy, reg);

	reg = (SAS_END_DEV << 4) + ((PORT_DEV_STP_INIT | PORT_DEV_SMP_INIT | PORT_DEV_SSP_INIT) << 8);
	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
	WRITE_PORT_CONFIG_DATA(root, phy, reg);
#ifdef FREY_B2_PORT1_PORT5_WORKAROUND
	// Port 1, 5 issue in Frey B2 bring up
	if ((((core_extension *)root->core)->revision_id == VANIR_C2_REV)
	    && ((MV_REG_READ_DWORD(0, PCIE_REG(0, PCIE_CC_REV)) & 0xff) == 0xB2)) {
		WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_MODE_REG_0);
		reg = READ_PORT_VSR_DATA(root, phy);
		if (phy->id == 1) {
			reg = 2;
			WRITE_PORT_VSR_DATA(root, phy, reg);
			MV_PRINT("revision_id(%x), rev(%x) phyid %d. set VSR_PHY_MODE_REG_0 to 0x%x\n", ((core_extension *)root->core)->revision_id, (MV_REG_READ_DWORD(0, PCIE_REG(0, PCIE_CC_REV)) & 0xff), (root->base_phy_num+phy->id), reg);
		}
	}
#endif
}

/* logic_phy_map the map is using phy->id as the map instead of phy->asic_id */
MV_VOID mv_reset_phy(MV_PVOID root_p, MV_U8 logic_phy_map, MV_BOOLEAN hard_reset)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy;
	MV_U32 reg;
	MV_U32 i;
	MV_U32 phy_irq_mask = 0;
	MV_U8 init = MV_FALSE;
	MV_U32 phyrdy_wait_time = 0;

	/* disable all interrupt not only several phy interrupt bits like ready change
	 * otherwise some other interrupt will trigger ISR
	 * ISR will kick in and read the phy status before the function return.
	 * even this, if interrupt is shared, ISR will be called anyway.
	 * so set phy_irq_mask */
	core_disable_ints(root->core);
#if 0 // def ATHENA_FPGA_WORKAROUND
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG2);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL_DATA, 0x0A80);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG3);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_ALL_PHY_CONTROL_DATA, 0x08A7);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_ADDR0, CONFIG_SAS_CTRL0);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0, 0x00320FFF);
#endif
	for (i = 0; i < root->phy_num; i++) {
		if (!(logic_phy_map & MV_BIT(i))) {
			continue;
		}

		phy = &root->phy[i];

		if (init == MV_FALSE) {
			phy_irq_mask = phy->phy_irq_mask;
			init = MV_TRUE;
		}
		MV_ASSERT(phy_irq_mask == phy->phy_irq_mask);

		phy->phy_irq_mask &= ~(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
		/* disable interrupt */
		//reg = READ_PORT_IRQ_MASK(root, phy);
		//reg &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
		//WRITE_PORT_IRQ_MASK(root, phy, reg);

		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
		WRITE_PORT_CONFIG_DATA(root, phy, phy->phy_irq_mask);

		WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
		reg = READ_PORT_VSR_DATA(root, phy);
		reg &= ~VSR_IRQ_PHY_TIMEOUT;
		WRITE_PORT_VSR_DATA(root, phy, reg);

		core_sleep_millisecond(root->core, 10);

		/* reset */
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
		reg = READ_PORT_CONFIG_DATA(root, phy);

		if (hard_reset) {
			reg |= SCTRL_PHY_HARD_RESET_SEQ;
		}
		else {
			reg |= SCTRL_STP_LINK_LAYER_RESET;
		}

		/*without the link layer reset, sometimes hotplug in 4 drives, can't detect all drives, no sig fis and identify timeout.
		From SATA trace, the phy seems to be stuck, no any tranfer from target, but H2D transfer is ok.
		Add below link reset, not happened again. Test with Seagate 6G SATA:ST3500413AS*/
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
		WRITE_PORT_CONFIG_DATA(root, phy, (reg | SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET));
	}

	/* Polling for phy ready after OOB */
	phyrdy_wait_time = 100; // wait_phyrdy_time set to 100 * (10 ms per polling) = max 1s
	while ((phyrdy_wait_time > 0) && logic_phy_map) {
		for (i = 0; i < root->phy_num; i++) {
			if (!(logic_phy_map & MV_BIT(i))) {
				continue;
			}
			phy = &root->phy[i];

			/* clear phy change status may trigger */
			if (mv_is_phy_ready(root, phy) ||
			    (phyrdy_wait_time == 1)) { /* always clear for the last loop */
				WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
				WRITE_PORT_CONFIG_DATA(root, phy, (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0));

				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
				WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);

				/* enable phy interrupt */
				//reg = READ_PORT_IRQ_MASK(root, phy);
				//reg |= (IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
				//WRITE_PORT_IRQ_MASK(root, phy, reg);
				phy->phy_irq_mask = phy_irq_mask;

				WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
				WRITE_PORT_CONFIG_DATA(root, phy, phy->phy_irq_mask);

#ifndef DISABLE_VSR_IRQ_PHY_TIMEOUT
				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
				reg = READ_PORT_VSR_DATA(root, phy);
				reg |= VSR_IRQ_PHY_TIMEOUT;
				WRITE_PORT_VSR_DATA(root, phy, reg);
#endif

				logic_phy_map &= ~(MV_BIT(i)); //change phy map for phy to 0, so we may exit before 1s expire;
			}
		}
		core_sleep_millisecond(root->core, 10);
		phyrdy_wait_time--;
	}
	core_enable_ints(root->core);
}

MV_VOID hal_clear_srs_irq(MV_PVOID root_p, MV_PVOID device, MV_U32 set, MV_BOOLEAN clear_all)
{
	pl_root *root = (pl_root *)root_p;
	MV_U32 reg;
	domain_device *dev = (domain_device *)device;

	if (set < 64) {
		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (clear_all == MV_TRUE) {
				reg = READ_SAS_IRQ_STAT(root, 0);
				if (reg) {
					WRITE_SAS_IRQ_STAT(root, 0, reg);
				}
				reg = READ_SAS_IRQ_STAT(root, 32);
				if (reg) {
					WRITE_SAS_IRQ_STAT(root, 32, reg);
				}
			}
			else {
				reg = READ_SAS_IRQ_STAT(root, set);
				if (reg & MV_BIT(set % 32)) {
					CORE_DPRINT(("register set 0x%x was stopped.\n", set));
					WRITE_SAS_IRQ_STAT(root, set, MV_BIT(set % 32));
				}
			}
		}
		else {
			if (clear_all == MV_TRUE) {
				reg = READ_SRS_IRQ_STAT(root, 0);
				if (reg) {
					WRITE_SRS_IRQ_STAT(root, 0, reg);
				}
				reg = READ_SRS_IRQ_STAT(root, 32);
				if (reg) {
					WRITE_SRS_IRQ_STAT(root, 32, reg);
				}
			}
			else {
				reg = READ_SRS_IRQ_STAT(root, set);
				if (reg & MV_BIT(set % 32)) {
					CORE_DPRINT(("register set 0x%x was stopped.\n", set));
					WRITE_SRS_IRQ_STAT(root, set, MV_BIT(set % 32));
				}
			}
		}
	}
	else {
		set -= 64;
		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (clear_all == MV_TRUE) {
				reg = READ_SAS_IRQ_STAT2(root, 0);
				if (reg) {
					WRITE_SAS_IRQ_STAT2(root, 0, reg);
				}
				reg = READ_SAS_IRQ_STAT2(root, 32);
				if (reg) {
					WRITE_SAS_IRQ_STAT2(root, 32, reg);
				}
			}
			else {
				reg = READ_SAS_IRQ_STAT2(root, set);
				if (reg & MV_BIT(set % 32)) {
					CORE_DPRINT(("register set 0x%x was stopped.\n", set));
					WRITE_SAS_IRQ_STAT2(root, set, MV_BIT(set % 32));
				}
			}
		}
		else {
			if (clear_all == MV_TRUE) {
				reg = READ_SRS_IRQ_STAT2(root, 0);
				if (reg) {
					WRITE_SRS_IRQ_STAT2(root, 0, reg);
				}
				reg = READ_SRS_IRQ_STAT2(root, 32);
				if (reg) {
					WRITE_SRS_IRQ_STAT2(root, 32, reg);
				}
			}
			else {
				reg = READ_SRS_IRQ_STAT2(root, set);
				if (reg & MV_BIT(set % 32)) {
					CORE_DPRINT(("register set 0x%x was stopped.\n", set));
					WRITE_SRS_IRQ_STAT2(root, set, MV_BIT(set % 32));
				}
			}
		}

	}
}

MV_VOID hal_enable_register_set(MV_PVOID root_p, MV_PVOID base_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base = (domain_base *)base_p;
	domain_device *dev;
	MV_U32 reg;

	// COMMON_CONFIG (R20100h) bit CONFIG_STP_STOP_ON_ERR (bit 25) if set, stops SATA
	// transmit on only the Register Set with following errors by setting SRS_IRQ for
	// that set to 1:
	//        Non Spcfc NCQ Error
	//        Watch Dog Timeout
	//        Any Receive Error
	// Upon receiving errors Abort all device running requests before clearing SRS_IRQ
	// Only clear the SRS_IRQ for which the error has already been handled.

	if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
		domain_expander *exp = (domain_expander *)base;
		if (exp->register_set != NO_REGISTER_SET) {
			hal_clear_srs_irq(root, exp, exp->register_set, MV_FALSE);
		}
	}
	else if (base->type == BASE_TYPE_DOMAIN_ENCLOSURE) {
		domain_enclosure *enc = (domain_enclosure *)base;
		if (enc->register_set != NO_REGISTER_SET) {
			hal_clear_srs_irq(root, enc, enc->register_set, MV_FALSE);
		}
	}
	else if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (IS_SSP(dev) && (dev->register_set != NO_REGISTER_SET)) {
			hal_clear_srs_irq(root, dev, dev->register_set, MV_FALSE);
		}
		else if (IS_STP_OR_SATA(dev) && (dev->register_set != NO_REGISTER_SET)) {
			hal_clear_srs_irq(root, dev, dev->register_set, MV_FALSE);
		}

	}

	// COMMON_CONTROL (R20104h) bit CONTROL_ERR_STOP_CMD_ISSUE (bit 3) if set, and
	// SL MODE 1 (R1C0h indirect access) bits
	//        9:  Broadcast Change
	//        10: Watchdog Timeout
	//        11: Phy Rdy Change
	// SAS transmit on ALL ports with errors by generating COMMON_IRQ_STAT bit
	// INT_CMD_ISSUE_STOPPED.
	// Clearing the INT_CMD_ISSUE_STOPPED bit and setting COMMON_CONTROL
	// EN_PORT_#_XMT bits will re-enable port transmits.
	reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_STAT);
	reg |= INT_CMD_ISSUE_STOPPED;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, reg);
#if 0
	reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_STAT);
	reg |= INT_CMD_ISSUE_STOPPED;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_STAT, reg);

	reg = READ_CMPLQ_IRQ_STAT(root, dev->base.queue_id);
	reg |= INT_CMD_ISSUE_STOPPED;
	WRITE_CMPLQ_IRQ_STAT(root, dev->base.queue_id, reg);
#endif
	/* fs TODO: need reset all queue's interrupt status(INT_CMD_ISSUE_STOPPED)? */

	reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_CONTROL);
	reg |= 0xff00;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CONTROL, reg);
}

MV_VOID hal_disable_io_chip(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 tmp;

	/* disable CMD/CMPL_Q/RESP mode */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
	tmp &= ~CONTROL_EN_CMD_ISSUE;
	tmp &= ~CONTROL_RSPNS_RCV_EN;
	/* tmp &= ~CONTROL_CMD_CMPLT_IRQ_MD; */
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);

	/* disable interrupt */
	tmp = 0;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMPLQ_IRQN_MASK, tmp);;
}

void core_disable_ints(void *ext)
{
#ifndef SUPPORT_ROC
	core_extension *core = (core_extension *)ext;
#if 1
	/*if(msix_enabled(core))*/
	{
	  int i,j;
	  pl_root* root;
	  for(i=0; i<core->chip_info->n_host;i++){
	    root=&core->roots[i];
	    for(j=0;j<root->queue_num;j++){
	      WRITE_CMPLQ_IRQ_MASK(root, j, 0);
	    }
	  }
	}
#endif
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, 0);
#endif
}

void core_enable_ints(void *ext)
{
#ifndef SUPPORT_ROC
	core_extension *core = (core_extension *)ext;
#if 1
	/*if(msix_enabled(core))*/
	{
	  int i,j;
	  pl_root* root;
	  for(i=0; i<core->chip_info->n_host;i++){
	    root=&core->roots[i];
	    for(j=0;j<root->queue_num;j++){
	      WRITE_CMPLQ_IRQ_MASK(root, j, root->queues[j].irq_mask);
	    }
	  }
	}
#endif
	MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, core->irq_mask);
#endif
}

void core_disable_queue_ints(void *ext, MV_U16 queue_msk)
{
	core_extension *core = (core_extension *)ext;
	pl_root *root = NULL;
	MV_U8 queue_cnt = 0;
	int i,j;

	for(i=0; i<core->chip_info->n_host;i++){
		root=&core->roots[i];
		for(j=0;j<root->queue_num;j++){
			if (queue_msk & (MV_U16)(MV_BIT(queue_cnt)))
				WRITE_CMPLQ_IRQ_MASK(root, j, 0);
			queue_cnt++;
		}
	}
	
}

void core_enable_queue_ints(void *ext, MV_U16 queue_msk)
{
	core_extension *core = (core_extension *)ext;
	pl_root *root = NULL;
	MV_U8 queue_cnt = 0;
	int i,j;

	for(i=0; i<core->chip_info->n_host;i++){
		root=&core->roots[i];
		for(j=0;j<root->queue_num;j++){
			if (queue_msk & (MV_U16)(MV_BIT(queue_cnt)))
				WRITE_CMPLQ_IRQ_MASK(root, j, root->queues[j].irq_mask);
			queue_cnt++;
		}
	}
	
}

MV_BOOLEAN core_check_int(void *ext)
{
	core_extension *core = (core_extension *)ext;
	pl_root *root;
	MV_U32 main_irq;
	MV_U8 hasInt = 0;
	main_irq = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_INT_CAUSE_REG);
	main_irq = main_irq & core->irq_mask;
	if (main_irq == 0) {
		return(MV_FALSE);
	}
	if (main_irq & INT_MAP_SAS) {
		if (main_irq & INT_MAP_SASINTA) {
			root = &core->roots[0];
			io_chip_clear_int(root);
			
		}
		if (main_irq & INT_MAP_SASINTB) {
			root = &core->roots[1];
			io_chip_clear_int(root);
			
		}
	}
#ifdef HARDWARE_XOR
	if (main_irq & INT_MAP_XOR) {
		xor_clear_int(core);
	}
#endif

#ifdef DEBUG_PCI_E_ERROR
	if (main_irq & INT_MAP_PCIE_ERR) {
		MV_U32 reg;
		reg = MV_REG_READ_DWORD(core->mmio_base, MV_PCI_REG_INT_CAUSE);
		CORE_PRINT(("main irq :0x%x.\n", main_irq));
		CORE_PRINT(("PCI cause irq: 0x%x.\n", reg));
		//MV_ASSERT(MV_FALSE);
	}
#endif
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	if (main_irq & INT_MAP_CRYPTO_MGMT) {
		MV_U32 reg;
		reg = MV_REG_READ_DWORD(core->mmio_base, CRYPTO_GLOBAL_SECURITY_INT_CAUSE);
		CORE_PRINT(("main irq :0x%x.\n", main_irq));
		CORE_PRINT(("CRYPTO INT CAUSE: 0x%x.\n", reg));
	}
#endif

	return MV_TRUE;
}
MV_BOOLEAN core_clear_int(core_extension *core)
{
	MV_U32 main_irq;
	pl_root *root;

	main_irq = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_INT_CAUSE_REG);
	main_irq = main_irq & core->irq_mask;
	if (main_irq == 0) {
		return(MV_FALSE);
	}

	core->main_irq |= main_irq;

	if (main_irq & INT_MAP_SAS) {
		if (main_irq & INT_MAP_SASINTA) {
			root = &core->roots[0];
			io_chip_clear_int(root);
		}
		if (main_irq & INT_MAP_SASINTB) {
			root = &core->roots[1];
			io_chip_clear_int(root);
		}
	}

#ifdef HARDWARE_XOR
	if (main_irq & INT_MAP_XOR) {
		xor_clear_int(core);
	}
#endif

#ifdef DEBUG_PCI_E_ERROR
	if (main_irq & INT_MAP_PCIE_ERR) {
		MV_U32 reg;
		reg = MV_REG_READ_DWORD(core->mmio_base, MV_PCI_REG_INT_CAUSE);
		CORE_PRINT(("main irq :0x%x.\n", main_irq));
		CORE_PRINT(("PCI cause irq: 0x%x.\n", reg));
		//MV_ASSERT(MV_FALSE);
	}
#endif
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	if (main_irq & INT_MAP_CRYPTO_MGMT) {
		MV_U32 reg;
		reg = MV_REG_READ_DWORD(core->mmio_base, CRYPTO_GLOBAL_SECURITY_INT_CAUSE);
		CORE_PRINT(("main irq :0x%x.\n", main_irq));
		CORE_PRINT(("CRYPTO INT CAUSE: 0x%x.\n", reg));
	}
#endif

	if (main_irq) {
		return MV_TRUE;
	}
	else {
		return MV_FALSE;
	}
}

MV_VOID core_handle_int(core_extension *core)
{
	pl_root *root;
#ifdef SUPPORT_ACTIVE_CABLE
	MV_U32 reg_value;
#endif

	/* handle io chip interrupt */
	if (core->main_irq & INT_MAP_SAS) {
		if (core->main_irq & INT_MAP_SASINTA) {
			root = &core->roots[0];
			io_chip_handle_int(root);
		}
		if (core->main_irq & INT_MAP_SASINTB) {
			root = &core->roots[1];
			io_chip_handle_int(root);
		}

	}

#ifdef HARDWARE_XOR
	if (core->main_irq & INT_MAP_XOR) {
		xor_handle_int(core);
	}
#endif
#ifdef SUPPORT_ACTIVE_CABLE
	if (core->main_irq & INT_MAP_GPIO) {
		reg_value = MV_REG_READ_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG);
		if (reg_value & CABLE_GPIO_BITS) {
			MV_REG_WRITE_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG, 0xFF);
			core_handle_cable_gpio_int(core, reg_value);
		}
	}
#endif
#ifdef SUPPORT_I2C
	if (core->main_irq & INT_MAP_TWSI) {
		i2c_interrupt_service_routine(core);
	}
#endif /* SUPPORT_I2C */
	core->main_irq = 0;
}

#ifdef SUPPORT_NVSRAM
/*
 * register_base: base address for core to access register
 * memory_base: base address for mapping memory like NVSRAM
 * return: the mapped NVSRAM base address for NVSRAM
 */
MV_LPVOID nvsram_init(MV_LPVOID register_base, MV_LPVOID memory_base)
{
	MV_LPVOID nvsram_base = 0;

#ifndef SUPPORT_ROC
	/* initialize PCI-E window for accessing NVSRAM
	 * use PCI windows zero to map the NVSRAM
	 * NVSRAM is mapped to memory_base at offset CORE_NVSRAM_MAPPING_BASE */
	MV_REG_WRITE_DWORD(register_base, MV_PCI_REG_WIN0_CTRL, 0x1F001); /* 32K */
	MV_REG_WRITE_DWORD(register_base, MV_PCI_REG_WIN0_BASE, CORE_NVSRAM_MAPPING_BASE);
	MV_REG_WRITE_DWORD(register_base, MV_PCI_REG_WIN0_REMAP, 0xE8000001);
	nvsram_base = (MV_PU8)memory_base + CORE_NVSRAM_MAPPING_BASE;
#else
	nvsram_base = ((MV_PU8)0xE8000000L);
#endif

	return nvsram_base;
}

MV_PU8 NVRAM_BASE(MV_PVOID caller_ext)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(
	                           caller_ext, MODULE_CORE);
	return core->nvsram_base;
}
#endif

#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
MV_U8 read_i2c_register(MV_PVOID core_p, MV_U8 port, MV_U8 reg_index)
{
	core_extension *core = (core_extension *)core_p;
	MV_U32	reg;
	reg = 0;
	((reg_i2c_software_control *)&reg)->address_port = reg_index;
	((reg_i2c_software_control *)&reg)->read_en = 1;
	MV_REG_WRITE_DWORD(core->mmio_base, TWSI_REG_BASE(port), reg);
	reg = MV_REG_READ_DWORD(core->mmio_base, TWSI_REG_BASE(port));

	return((MV_U8)((reg_i2c_software_control *)&reg)->data_port);
}

void write_i2c_register(MV_PVOID core_p, MV_U8 port, MV_U8 reg_index, MV_U8 reg_val)
{
	core_extension *core = (core_extension *)core_p;
	MV_U32	reg;
	reg = 0;
	((reg_i2c_software_control *)&reg)->address_port = reg_index;
	((reg_i2c_software_control *)&reg)->data_port = reg_val;
	((reg_i2c_software_control *)&reg)->write_en = 1;
	MV_REG_WRITE_DWORD(core->mmio_base, TWSI_REG_BASE(port), reg);
	((reg_i2c_software_control *)&reg)->write_en = 0;
	MV_REG_WRITE_DWORD(core->mmio_base, TWSI_REG_BASE(port), reg);

	return;
}
#endif
MV_BOOLEAN core_is_register_set_stopped(MV_PVOID root_p, MV_PVOID device, MV_U8 set)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)device;
	MV_U32 reg_set_irq_stat;

	if (IS_SSP(dev)) {
		if (set < 64) {
			reg_set_irq_stat = READ_SAS_IRQ_STAT(root, set);
		}
		else {
			set -= 64;
			reg_set_irq_stat = READ_SAS_IRQ_STAT2(root, set);
		}
	}
	else {
		if (set < 64) {
			reg_set_irq_stat = READ_SRS_IRQ_STAT(root, set);
		}
		else {
			set -= 64;
			reg_set_irq_stat = READ_SRS_IRQ_STAT2(root, set);
		}
	}

	if (reg_set_irq_stat & MV_BIT(set)) {
		return MV_TRUE;
	}
	else {
		return MV_FALSE;
	}
}
MV_U8 core_get_register_set(MV_PVOID root_p, MV_PVOID device)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)device;
	int i;
	MV_U32 set;
	MV_ULONG flags;
	MV_U8 register_set = NO_REGISTER_SET;
	//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
	i = ffc64(root->sata_reg_set);
	if (i != -1) {
		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (i >= 32) {
				root->sata_reg_set.parts.high |= MV_BIT(i - 32);
				WRITE_SAS_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.high);
				register_set = (MV_U8) i;
			}
			else if (i >= 0) {
				root->sata_reg_set.parts.low |= MV_BIT(i);
				WRITE_SAS_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.low);
				set = READ_SAS_REGISTER_SET_ENABLE(root, i);
				register_set = (MV_U8) i;
			}
		}
		else {
			if (i >= 32) {
				root->sata_reg_set.parts.high |= MV_BIT(i - 32);
				WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.high);
				register_set = (MV_U8) i;
			}
			else if (i >= 0) {
				root->sata_reg_set.parts.low |= MV_BIT(i);
				WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.low);
				register_set = (MV_U8) i;
			}
		}
	}
	else {

		i = ffc64(root->sata_reg_set2);

		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (i >= 32) {
				root->sata_reg_set2.parts.high |= MV_BIT(i - 32);
				WRITE_SAS_REGISTER_SET2_ENABLE(root, i, root->sata_reg_set2.parts.high);
				register_set = (MV_U8)(i + 64);
			}
			else if (i >= 0) {
				root->sata_reg_set2.parts.low |= MV_BIT(i);
				WRITE_SAS_REGISTER_SET2_ENABLE(root, i, root->sata_reg_set2.parts.low);
				set = READ_SAS_REGISTER_SET2_ENABLE(root, i);
				register_set = (MV_U8)(i + 64);
			}
		}
		else {
			if (i >= 32) {
				root->sata_reg_set2.parts.high |= MV_BIT(i - 32);
				WRITE_REGISTER_SET2_ENABLE(root, i, root->sata_reg_set2.parts.high);
				register_set = (MV_U8)(i + 64);
			}
			else if (i >= 0) {
				root->sata_reg_set2.parts.low |= MV_BIT(i);
				WRITE_REGISTER_SET2_ENABLE(root, i, root->sata_reg_set2.parts.low);
				register_set = (MV_U8)(i + 64);
			}
		}

	}
	//CORE_DPRINT(("no register set, %x, %x, %x %x.\n", root->sata_reg_set2.parts.high, root->sata_reg_set2.parts.low, root->sata_reg_set.parts.high, root->sata_reg_set.parts.low));
	OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
	//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
	return register_set;
}
#if 0
MV_U8 sata_get_register_set(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;

	int i;
	i = ffc64(root->sata_reg_set);
	if (i >= 32) {
		root->sata_reg_set.parts.high |= MV_BIT(i - 32);
		WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.high);

		return (MV_U8) i;
	}
	else if (i >= 0) {
		root->sata_reg_set.parts.low |= MV_BIT(i);
		WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.low);

		return (MV_U8) i;
	}

	//CORE_DPRINT(("no register set.\n"));
	return NO_REGISTER_SET;
}
#endif

void core_free_register_set(MV_PVOID root_p, MV_PVOID device, MV_U8 set)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)device;
	MV_U32 i;
	MV_ULONG flags;
	//OSSW_SPIN_LOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_LOCK(&root->root_SpinLock, flags);
	if (set < 64) {
		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (set < 32) {
				root->sata_reg_set.parts.low &= ~MV_BIT(set);
				WRITE_SAS_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.low);
				i = READ_SAS_REGISTER_SET_ENABLE(root, set);
				//            MV_PRINT("Free SAS Register Set 0x%x \n",i);

			}
			else {
				root->sata_reg_set.parts.high &= ~MV_BIT(set - 32);
				WRITE_SAS_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.high);
			}

		}
		else {
			if (set < 32) {
				root->sata_reg_set.parts.low &= ~MV_BIT(set);
				WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.low);
			}
			else {
				root->sata_reg_set.parts.high &= ~MV_BIT(set - 32);
				WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.high);
			}
		}
	}
	else {
		set -= 64;
		if (dev && (IS_SSP(dev) || (dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE) || (dev->base.type == BASE_TYPE_DOMAIN_EXPANDER))) {
			if (set < 32) {
				root->sata_reg_set2.parts.low &= ~MV_BIT(set);
				WRITE_SAS_REGISTER_SET2_ENABLE(root, set, root->sata_reg_set2.parts.low);
				i = READ_SAS_REGISTER_SET2_ENABLE(root, set);
				//            MV_PRINT("Free SAS Register Set 0x%x \n",i);

			}
			else {
				root->sata_reg_set2.parts.high &= ~MV_BIT(set - 32);
				WRITE_SAS_REGISTER_SET2_ENABLE(root, set, root->sata_reg_set2.parts.high);
			}

		}
		else {
			if (set < 32) {
				root->sata_reg_set2.parts.low &= ~MV_BIT(set);
				WRITE_REGISTER_SET2_ENABLE(root, set, root->sata_reg_set2.parts.low);
			}
			else {
				root->sata_reg_set2.parts.high &= ~MV_BIT(set - 32);
				WRITE_REGISTER_SET2_ENABLE(root, set, root->sata_reg_set2.parts.high);
			}
		}
	}
	/*
	* in some device hot-removal/command-aborting case, the register set
	* was stopped by the HW, saw STP request time out in IO/hot-plug testing
	*/
	hal_clear_srs_irq(root, device, set, MV_FALSE);
	//OSSW_SPIN_UNLOCK_ROOT(root->core, flags, root->root_id);
	OSSW_SPIN_UNLOCK(&root->root_SpinLock, flags);
}
#if 0
void sata_free_register_set(MV_PVOID root_p, MV_U8 set)
{
	pl_root *root = (pl_root *)root_p;

	if (set < 32) {
		root->sata_reg_set.parts.low &= ~MV_BIT(set);
		WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.low);
	}
	else {
		root->sata_reg_set.parts.high &= ~MV_BIT(set - 32);
		WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.high);
	}

	/*
	 * in some device hot-removal/command-aborting case, the register set
	 * was stopped by the HW, saw STP request time out in IO/hot-plug testing
	 */
	hal_clear_srs_irq(root, set, MV_FALSE);
}
#endif
MV_U8	exp_check_plugging_finished(pl_root *root, domain_port *port);

#ifdef SUPPORT_DIRECT_SATA_SSU
MV_VOID sata_phy_spinup_hold(pl_root *root, domain_phy *phy)
{
	MV_U32 reg;

	/* enable spin up hold */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG);
	reg = READ_PORT_VSR_DATA(root, phy);
	reg &= ~VSR_SATA_SPIN_UP_ENABLE;
	reg |= VSR_PHY_RESET;
	phy->is_spinup_hold = MV_TRUE;
	WRITE_PORT_VSR_DATA(root, phy, reg);
}

MV_VOID mv_sata_ssu_timer(MV_PVOID root_p, MV_PVOID phy_p)
{
	MV_U32 reg;
	pl_root *root = root_p;
	domain_phy *phy = phy_p;
	core_extension *core = root->core;

	if (core->state != CORE_STATE_STARTED) {
	
		/* pal_notify_event will not handle phy_change event if core state is not CORE_STATE_STARTED
		*/
		phy->sata_ssu_timer = core_add_timer(core, phy->sata_ssu_time, 
								mv_sata_ssu_timer, root, phy);

		CORE_DPRINT(("core state is not started, phy %d sata spin up hold need to delay...  \n", phy->id));
		return;
	}

	/* enable COMWAKE and reset phy */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG);
	reg = READ_PORT_VSR_DATA(root, phy);
	reg |= VSR_SATA_SPIN_UP_ENABLE | VSR_PHY_RESET;
	phy->is_spinup_hold = MV_FALSE;
	WRITE_PORT_VSR_DATA(root, phy, reg);
	phy->sata_ssu_timer = NO_CURRENT_TIMER;

	/* enable phy sata spin hold interrupt */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
	reg = READ_PORT_VSR_DATA(root, phy);
	reg |= VSR_SATA_SPIN_HOLD;
	WRITE_PORT_VSR_DATA(root, phy, reg);
}

MV_VOID mv_sata_handle_ssu(pl_root *root, domain_phy *phy)
{
	MV_U32 reg, ssu_time = 0;
	core_extension *core = root->core;

	WRITE_PORT_VSR_DATA(root, phy, VSR_SATA_SPIN_HOLD);
	if (!phy->is_spinup_hold) {
		return;
	}

	/* disable phy sata spin hold interrupt */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
	reg = READ_PORT_VSR_DATA(root, phy);
	reg &= ~VSR_SATA_SPIN_HOLD;
	WRITE_PORT_VSR_DATA(root, phy, reg);

	if (core->sata_ssu_group) {
		ssu_time = ((phy->id + root->base_phy_num)/ core->sata_ssu_group)
					* core->sata_ssu_time;
	}

	phy->sata_ssu_time = ssu_time;

	phy->sata_ssu_timer = core_add_timer(core, phy->sata_ssu_time, 
								mv_sata_ssu_timer, root, phy);
	
	CORE_DPRINT(("sata spin up hold. timer 0x%x time %d phy %d group %d \n",
		phy->sata_ssu_timer, ssu_time, phy->id + root->base_phy_num, core->sata_ssu_group));
	MV_ASSERT(phy->sata_ssu_timer != NO_CURRENT_TIMER);
}
#endif

MV_VOID io_chip_clear_int(pl_root *root)
{
	MV_U32 irq;
	domain_phy *phy;
	MV_U8 i;
	MV_U32 reg;
	MV_U32 irq_sts;

	/* fs TODO: should read COMMON_CMPLQ_IRQN_STAT? */
	irq = MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_STAT);
	root->comm_irq |= irq & root->comm_irq_mask;

	if (root->comm_irq & INT_MEM_PAR_ERR) {
		/*
		    When asserted, a bit in Internal Memory Parity Error (R202D0h)
		    is set and is enabled by the corresponding bit in Internal
		    Memory Parity Error Enable (R202D4h).
		    Information related to the error is captured in the registers listed
		    in Table 9-8, Parity Error Registers.
		*/
		MV_PRINT("Error Interrupt: Internal Mem Parity Error (Bit %d) , COMMON_IRQ_STAT: 0x%08x\n",
		         ossw_ffs(INT_MEM_PAR_ERR), irq);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR);
		MV_PRINT("%p off 0x%x  COMMON_PORT_INTL_MEM_PERR(0x%x) \n", root->mmio_base, COMMON_INTL_MEM_PARITY_ERR, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_MEM_PAR_ERR);
	}
#if 0 //def ATHENA_FPGA_WORKAROUND
	WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_COUNTER0);
	reg = READ_ALL_PORT_VSR_DATA(root);
	if (reg) {
		CORE_PRINT(("Error Interrupt: VSR_PHY_COUNTER0(0x%x) = 0x%08x\n", VSR_PHY_COUNTER0, reg));
		WRITE_ALL_PORT_VSR_DATA(root, 0);
		WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_WORKARPUND_REG2);
		reg = READ_ALL_PORT_VSR_DATA(root);
		CORE_PRINT(("Error Interrupt: VSR_PHY_WORKARPUND_REG2(0x%x) = 0x%08x\n", VSR_PHY_WORKARPUND_REG2, reg));
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SAS_CTRL0);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			CORE_PRINT(("Error Interrupt: phy %d, CONFIG_SAS_CTRL0(0x%x) = 0x%08x\n", phy->asic_id, CONFIG_SAS_CTRL0, reg));
		}
		WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_WORKARPUND_REG3);
		reg = READ_ALL_PORT_VSR_DATA(root);
		CORE_PRINT(("Error Interrupt: VSR_PHY_WORKARPUND_REG2(0x%x) = 0x%08x\n", VSR_PHY_WORKARPUND_REG3, reg));
	}
#endif

	if (root->comm_irq & INT_DP_PAR_ERR) {
		/*
		        Data Path Parity Error.
		        When asserted, a data path parity error was detected.
		        Information related to the error is captured in the registers listed
		        in Table 9-8, Parity Error Registers.
		        See Data Path Parity Status (R202DCh) for more information.
		*/
		MV_PRINT("Error Interrupt: Data Path Parity Error (Bit %d) , COMMON_IRQ_STAT: 0x%08x\n",
		         ossw_ffs(INT_DP_PAR_ERR), irq);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS);
		MV_PRINT("%p off 0x%x  COMMON_PORT_DPP_STATUS(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_PARITY_STS, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_ERR_ADDR);
		MV_PRINT("%p off 0x%x  COMMON_PORT_DP_ERR_ADDR_LOW(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_ERR_ADDR, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_ERR_ADDR_HI);
		MV_PRINT("%p off 0x%x  COMMON_PORT_DP_ERR_ADDR_HIGH(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_ERR_ADDR_HI, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_DP_PAR_ERR);
	}

	if (root->comm_irq & (INT_PORT_MASK)) {
		/* for port interrupt and PHY interrupt */
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			if (hal_has_phy_int(root->comm_irq, phy)) {
				/* clear PORT interrupt status */
				WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
				reg = READ_PORT_CONFIG_DATA(root, phy);
				CORE_EVENT_PRINT(("phy %d CONFIG_PORT_IRQ_STAT: 0x%08x\n", (root->base_phy_num+phy->id), reg));
				phy->irq_status |= (reg & phy->phy_irq_mask);
				WRITE_PORT_CONFIG_DATA(root, phy, reg);
				//Read back IRQ status register
				READ_PORT_CONFIG_DATA(root, phy);
				if (reg & IRQ_PRD_BC_ERR) {
					/*
					    Set when the amount of data transferred by a target is greater than the buffer size provided by the PRD table.
					    May be caused by a programming error (PRD too small), or by a target sending too much data.
					*/
					CORE_EVENT_PRINT(("Error Interrupt: PRD BC error (Bit %d), CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_PRD_BC_ERR), reg));
				}
				if (reg & IRQ_DMA_PEX_TO) {
					CORE_EVENT_PRINT(("Error Interrupt: DMA to PCIe Timeout (Bit %d) , CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_DMA_PEX_TO), reg));
				}
				if (reg & IRQ_STP_SATA_PHY_DEC_ERR_MASK) {
					CORE_EVENT_PRINT(("Error Interrupt: PHY Decoding Error (Bit %d) , CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_STP_SATA_PHY_DEC_ERR_MASK), reg));
				}
				/* clear PHY interrupt status */
				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
				reg = READ_PORT_VSR_DATA(root, phy);
				if (reg & VSR_IRQ_PHY_TIMEOUT) {
					CORE_EVENT_PRINT(("READ_PORT_VSR_DATA: 0x%08x\n", reg));
#ifndef DISABLE_VSR_IRQ_PHY_TIMEOUT
					phy->irq_status |= IRQ_PHY_RDY_CHNG_1_TO_0;
#endif
					WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);
				}
#ifdef SUPPORT_DIRECT_SATA_SSU
				if (reg & VSR_SATA_SPIN_HOLD) {
					mv_sata_handle_ssu(root, phy);
				}
#endif

				CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", (root->base_phy_num+phy->id), phy->irq_status));
			}
		}
	}
}
MV_VOID mv_sata_decode_err_check(MV_PVOID root_p, MV_PVOID phy_p)
{
	MV_U32 reg;
	pl_root *root = root_p;
	domain_phy *phy = phy_p;
	core_extension *core = root->core;
	phy->phy_decode_timer = NO_CURRENT_TIMER;
	if(phy->port){
		WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
		reg = READ_PORT_VSR_DATA(root, phy);
		CORE_EVENT_PRINT(("phy %d READ_PORT_VSR_DATA: 0x%08x\n", (root->base_phy_num+phy->id), reg));
		WRITE_PORT_CONFIG_ADDR(root, phy,CONFIG_PORT_SERIAL_CTRL_STS);
		reg = READ_PORT_CONFIG_DATA(root, phy); 
		CORE_EVENT_PRINT(("phy %d CONFIG_PORT_SERIAL_CTRL_STS: 0x%08x\n", (root->base_phy_num+phy->id), reg));
		CORE_EVENT_PRINT(("mv_sata_decode_err_check : phy %d irq_status = 0x%x phy_status = 0x%x.\n", (root->base_phy_num+phy->id), phy->irq_status, phy->phy_status));
		if(!mv_is_phy_ready(root, phy))
		{
			phy->irq_status  |= (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
			pal_notify_event(root, phy->asic_id, PL_EVENT_PHY_CHANGE);
			CORE_EVENT_PRINT(("mv_sata_decode_err_check : phy %d is not ready notify unplug event.\n", (root->base_phy_num+phy->id)));
		}
		
	}
	
}
MV_VOID io_chip_handle_port_int(pl_root *root)
{
	domain_phy *phy;
	MV_U8 i;
	MV_U32 port_irq, reg;
	core_extension *core = (core_extension *)root->core;
	for (i = 0; i < root->phy_num; i++) {
		phy = &root->phy[i];
		if (!hal_has_phy_int(root->comm_irq, phy)) {
			continue;
		}

		port_irq = phy->irq_status;
#ifdef SUPPORT_PHY_POWER_MODE
		port_irq &= phy->phy_irq_mask;
#endif
		root->comm_irq = hal_remove_phy_int(root->comm_irq, phy);
		CORE_EVENT_PRINT(("phy %d irq_status %08X.\n", (root->base_phy_num+phy->id), port_irq));

		if (port_irq & IRQ_ASYNC_NTFCN_RCVD_MASK) {
			CORE_EVENT_PRINT(("PHY asynchronous notification for root %p phy %d\n", \
			                  root, (root->base_phy_num+phy->id)));

			if (port_irq & IRQ_UNASSOC_FIS_RCVD_MASK) {
				/* SDB FIS with Notificatin bit is routed to unassociated FIS, from spec, need set it to all zero*/
				MV_U32 tmp;

				tmp = MV_REG_READ_DWORD(root->rx_fis, SATA_UNASSOC_SDB_FIS(phy) + 0x0);
				if (((tmp & 0xFF) == SATA_FIS_TYPE_SET_DEVICE_BITS) && (tmp & MV_BIT(15))) {
					MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_SDB_FIS(phy), 0x0);
					MV_REG_WRITE_DWORD(root->rx_fis, SATA_UNASSOC_SDB_FIS(phy) + 0x4, 0x0);
				}
				CORE_DPRINT(("Receive Async Notify SDB FIS from unassociated FIS, 0x%x\n", tmp));
				phy->irq_status &= ~IRQ_UNASSOC_FIS_RCVD_MASK;
				port_irq &= ~IRQ_UNASSOC_FIS_RCVD_MASK;
			}
			pal_notify_event(root, i, PL_EVENT_ASYNC_NOTIFY);
			phy->irq_status &= ~IRQ_ASYNC_NTFCN_RCVD_MASK;
		}

		if (port_irq & IRQ_UNASSOC_FIS_RCVD_MASK ||
		    port_irq & IRQ_SIG_FIS_RCVD_MASK) {
			/*
			 * STP might generate unassociated FIS too, so make sure
			 * it is a sata port
			 */
			if ((phy->type == PORT_TYPE_SATA) && (phy->port)) {
				sata_port_notify_event(phy->port, port_irq);
			}
			phy->irq_status &=
			    ~(IRQ_UNASSOC_FIS_RCVD_MASK | IRQ_SIG_FIS_RCVD_MASK);
		}

		if (port_irq & (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0)) {
			pal_notify_event(root, i, PL_EVENT_PHY_CHANGE);
			/* IRQ_PHY_RDY_CHNG_MASK and IRQ_PHY_RDY_CHNG_1_TO_0
			 * will be cleared later in event handler pl_handle_hotplug_event */
		}

		if (port_irq & IRQ_STP_SATA_PHY_DEC_ERR_MASK) {
			/* can do nothing for it */
			CORE_EVENT_PRINT(("PHY decoding error for root %p phy %d\n", \
			                  root, (root->base_phy_num+phy->id)));
			if(phy->phy_decode_timer != NO_CURRENT_TIMER){
				core_cancel_timer(core, phy->phy_decode_timer);
				phy->phy_decode_timer = NO_CURRENT_TIMER;
			}
			phy->phy_decode_timer = core_add_timer(core, 1, mv_sata_decode_err_check, root, phy);
			phy->phy_decode_err_cnt++;
			CORE_EVENT_PRINT(("cnt: %x\n", phy->phy_decode_err_cnt));
			if(phy->phy_decode_err_cnt >= CORE_PHY_DECODE_ERR_CNT){
				if(phy->port){
					CORE_EVENT_PRINT(("PHY Decode err do phy reset : phy %d irq_status = 0x%x.\n", (root->base_phy_num+phy->id), phy->irq_status));
					if(phy->phy_decode_err_cnt == CORE_PHY_DECODE_ERR_CNT)
						mv_reset_phy(root, phy->port->phy_map, MV_TRUE);
					//if(!mv_is_phy_ready(root, phy))
					{
						phy->irq_status  |= (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
						pal_notify_event(root, phy->asic_id, PL_EVENT_PHY_CHANGE);
						if(phy->phy_decode_timer != NO_CURRENT_TIMER){
							core_cancel_timer(core, phy->phy_decode_timer);
							phy->phy_decode_timer = NO_CURRENT_TIMER;
						}
					}
					CORE_EVENT_PRINT(("PHY Decode err after phy reset : phy %d irq_status = 0x%x.\n", (root->base_phy_num+phy->id), phy->irq_status));
				}				
			}
			phy->irq_status &= ~IRQ_STP_SATA_PHY_DEC_ERR_MASK;
		}

		if (port_irq & IRQ_BRDCST_CHNG_RCVD_MASK) {
			pal_notify_event(root, i, PL_EVENT_BROADCAST_CHANGE);
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_BROADCAST_RECIEVED);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			WRITE_PORT_CONFIG_DATA(root, phy, reg);
			CORE_EVENT_PRINT(("phy %d CONFIG_BROADCAST_RECIEVED: 0x%08x\n", (root->base_phy_num+phy->id), reg));
			phy->irq_status &= ~IRQ_BRDCST_CHNG_RCVD_MASK;
		}
	}
}

MV_VOID sata_handle_non_spcfc_ncq_err(pl_root *root, MV_U8 register_set)
{
	domain_device *device;
	MV_Request *req;
	core_context *ctx;
	struct _error_context *err_ctx;
	MV_ULONG flags, flags1, flags_queue;
#ifdef CORE_EH_LOG_EVENTS
	MV_U32 params[MAX_EVENT_PARAMS];
#endif

	device = get_device_by_register_set(root, register_set);
	if (device) {
		/* Check if any commands has completed including on non spcfc ncq error device */
		#if 0 /* fs TODO */
		io_chip_handle_cmpl_queue_int(root);
		#endif

		err_ctx = &device->base.err_ctx;
		err_ctx->interrupt_error |= INT_NON_SPCFC_NCQ_ERR;
		OSSW_SPIN_LOCK(&device->base.queue->handle_cmpl_SpinLock, flags_queue);
		OSSW_SPIN_LOCK(&err_ctx->sent_req_list_SpinLock, flags1);
		mv_cancel_timer(root->core, &device->base);
		if (!List_Empty(&err_ctx->sent_req_list)) {
			req = LIST_ENTRY(
			          (&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
			ctx = (core_context *)req->Context[MODULE_CORE];
#ifdef CORE_EH_LOG_EVENTS
			params[0] = CORE_EVENT;		            /* CDB Field */
			params[1] = CORE_EVENT_NON_SPCFC_NCQ_ERROR; /* Sense Key */
			params[2] = device->base.id;                /* Additional Sense Code */
			params[3] = ctx->req_type;		    /* Additional Sense Code Qualifier */

			core_generate_event(root->core, EVT_ID_HD_SC_ERROR, device->base.id,
			                    SEVERITY_INFO, 4, params, 0);
#endif
			//OSSW_SPIN_LOCK_Device(root->core, flags, &device->base);
			OSSW_SPIN_LOCK( &device->base.base_SpinLock, flags);
			if (core_is_register_set_stopped(root, device, device->register_set)) {
				CORE_EH_PRINT(("Register set %d is disabled\n", device->register_set));
				device->base.cmd_issue_stopped = MV_TRUE;
			}
//			device->base.blocked = MV_TRUE;
			//OSSW_SPIN_UNLOCK_Device(root->core, flags, &device->base);
			OSSW_SPIN_UNLOCK( &device->base.base_SpinLock, flags);
			/* reset that command slot */
			prot_reset_slot(root, &device->base, ctx->slot, req);

			if (CORE_IS_EH_REQ(ctx) || CORE_IS_INIT_REQ(ctx)) {
				/* Set timeout for EH and INIT req let EH Handler handle request */
				req->Scsi_Status = REQ_STATUS_TIMEOUT;

				/* put to the completion queue let Error handler or Init State Machine handle the error*/
				core_queue_completed_req(root->core, req);
				OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
				OSSW_SPIN_UNLOCK(&device->base.queue->handle_cmpl_SpinLock, flags_queue);

				/* no need to abort requests since EH and INIT should be single IO */
			}
			else {
				/* treat it as media error */
				req->Scsi_Status = REQ_STATUS_ERROR;

				/* put to the error queue to run the error handling */
				core_queue_error_req(root, req, MV_TRUE);
				OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
				OSSW_SPIN_UNLOCK(&device->base.queue->handle_cmpl_SpinLock, flags_queue);
				/* abort rest requests when hit NCQ non-specific error */
				pal_abort_device_running_req(root, &device->base);
			}
		}else{
			OSSW_SPIN_UNLOCK(&err_ctx->sent_req_list_SpinLock, flags1);
			OSSW_SPIN_UNLOCK(&device->base.queue->handle_cmpl_SpinLock, flags_queue);
		}
	}
}

MV_VOID io_chip_handle_non_spcfc_ncq_err(pl_root *root)
{
	MV_U32 err_0, err_1, err_2, err_3;
	MV_U8 i;

	err_0 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR0);
	err_1 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR1);
	err_2 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR2);
	err_3 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR3);

	for (i = 0; i < 32; i++) {
		if (err_0 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, i);
		}

		if (err_1 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, (i + 32));
		}
		if (err_2 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, (i + 64));
		}
		if (err_3 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, (i + 96));
		}
	}

	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR0, err_0);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR1, err_1);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR0, err_2);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR1, err_3);   
}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
MV_VOID prot_process_cmpl_fast_req(pl_root *root, MV_Request *req)
{
    cmd_resource *cmd_res = (cmd_resource *)req->Context[MODULE_HBA];
    domain_device *dev;
    core_extension *core = root->core;
    dev = (domain_device *)req->base_p;
    MV_DASSERT(cmd_res != NULL);
    MV_DASSERT(dev != NULL);
    root->running_req[cmd_res->slot] = NULL;
    // prot_clean_slot
    if (dev->base.outstanding_req) {
        dev->base.outstanding_req--;
    }
    else {
        CORE_DPRINT(("device %d outstanding req is zero??.\n",dev->base.id));
    }
    if(dev->base.blocked)
		dev->base.blocked=MV_FALSE;
    if ((dev->base.outstanding_req == 0) && (dev->register_set != NO_REGISTER_SET)) {
        core_free_register_set(root, dev, dev->register_set);
        dev->register_set = NO_REGISTER_SET;
    }
//prot_clean_slot(root, base, ctx->slot, req);
    {
        /* this request is ready for callback
        * add this request to upper layer's complete queue */
        if(req->Completion != NULL)
		req->Completion(req->Cmd_Initiator, req);
        //core_queue_completed_req_lock(root->core, req);
    }
}
#endif
MV_VOID io_chip_handle_cmpl_multi_queue_int(MV_PVOID root_p, MV_U16 queue_id)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *req = NULL, *tmp_req = NULL;
	core_context *ctx = NULL, *tmp_ctx = NULL;
	pl_queue *queue = &root->queues[queue_id];
	MV_PU32 cmpl_q; /* points to the completion entry zero */
	MV_U32 cmpl_entry;
	MV_U16 cmpl_wp; /* the completion write pointer */
	MV_U16 i, slot, j;
	struct _domain_base *base;

	cmpl_wp = (MV_U16) MV_LE32_TO_CPU(*(MV_U32 *)queue->cmpl_q_shadow) & 0x3fff;
	cmpl_q = (MV_PU32)queue->cmpl_q;
	
	i = queue->last_cmpl_q; /* last_cmpl_q is handled. will start from the next. */
	queue->last_cmpl_q = cmpl_wp;
	if (i == queue->last_cmpl_q) {	/* no new entry */
		return;
	}

	if (queue->last_cmpl_q == 0x3fff) { /* write pointer is not updated yet */
		return;
	}

	while (i != queue->last_cmpl_q) {
		i++;
		if (i >= queue->cmpl_q_size) {
			i = 0;
		}

		cmpl_entry = MV_LE32_TO_CPU(cmpl_q[i]);
		if (cmpl_entry & RXQ_ATTN) {
			/*
			 * some hardware event happend, just ignore it.
			 * We'll handle the common interrupt register
			 * which already has the information.
			 */
			//CORE_DPRINT(("attention.\n"));
			continue;
		}

		slot = (MV_U16)(cmpl_entry & 0xfff);
		if (slot >= root->slot_count_support) {
			CORE_DPRINT(("finished  slot %x exceed max slot %x.\n", slot, root->slot_count_support));
			continue;
		}
		req = root->running_req[slot];
		if (req == NULL) {
			CORE_EH_PRINT(("attention: "\
			               "cannot find corresponding req on slot 0x%x\n", \
			               slot));
			/* It can only happen in simulate error case. */
			/* Loki will hit this problem because of read log ext */
			//MV_DASSERT(MV_FALSE);
			continue;
		}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
            if(req->Req_Flag & REQ_FLAG_BYPASS_HYBRID){
                cmd_resource *cmd_res;
                MV_ASSERT(req->base_p);
                MV_ASSERT(req->Context[MODULE_HBA]);
                cmd_res = (cmd_resource *)req->Context[MODULE_HBA];
                base =  (struct _domain_base *)req->base_p;
                base->handler->process_command(root, base, &cmpl_entry, (MV_PVOID)((MV_PU8)root->cmd_struct_wrapper[slot].vir + 0x40), req);
                prot_process_cmpl_fast_req(root, req);
                continue;
            }
#endif
		ctx = req->Context[MODULE_CORE];
		base = (struct _domain_base *)get_device_by_id(root->lib_dev,
		        req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
		if (base == NULL) {
			//JING TBD: Is this possible? If it's removed, can it sent out?
			/* Seen some reuests sent/returned after notified/removed the device */
			/* these requests were not seen in Running_Req[]/Waiting_list during Port_AbortRequest() */
			/* they may be fired by consolidate/cache after that */
			MV_ASSERT(MV_FALSE);
		}

		/*
		 * process_command:
		 * 1. if success, return REQ_STATUS_SUCCESS
		 * 2. if final error like disk is gone, return REQ_STATUS_NO_DEVICE
		 * 3. if want to do error handling,
		 *    set ctx->error_info with EH_INFO_NEED_RETRY
		 *    and also set the Scsi_Status properly to
		 *    a. REQ_STATUS_HAS_SENSE if has sense
		 *    b. for timeout, the status should be REQ_STATUS_TIMEOUT
		 *    c. REQ_STATUS_ERROR for all other error
		 */
		((command_handler *)ctx->handler)->process_command(
		    root, base, &cmpl_entry, (MV_PVOID)((MV_PU8)root->cmd_struct_wrapper[slot].vir + 0x40), req);

		MV_DASSERT((req->Scsi_Status == REQ_STATUS_SUCCESS)
		           || (req->Scsi_Status == REQ_STATUS_TIMEOUT)
		           || (req->Scsi_Status == REQ_STATUS_NO_DEVICE)
		           || (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
		           || (req->Scsi_Status == REQ_STATUS_ERROR)
		           || (req->Scsi_Status == REQ_STATUS_BUSY));

		prot_process_cmpl_req(root, req);
	}
}
#if 0
MV_VOID io_chip_handle_cmpl_queue_int(MV_PVOID root_p)
{
	MV_U16 queue_id;
	MV_U16 queue_num;
	pl_root *root = (pl_root *)root_p;

	queue_num = root->queue_num;
	for (queue_id = 0; queue_id < queue_num; queue_id++) {
		io_chip_handle_cmpl_multi_queue_int(root_p, queue_id);
	}
}
#endif

MV_VOID io_chip_handle_int(pl_root *root)
{
#if 0
	io_chip_handle_cmpl_queue_int(root);
#endif
	if (root->comm_irq & (INT_PORT_MASK)) {
		io_chip_handle_port_int(root);
	}

#if 1 /* handled in queue's intr handler */
	if (root->comm_irq & INT_NON_SPCFC_NCQ_ERR) {
		io_chip_handle_non_spcfc_ncq_err(root);
	}
#endif
	root->comm_irq = 0;
	return ;
}

MV_BOOLEAN core_check_queue_int(pl_queue *queue)
{
	MV_U32 queue_irq;
	pl_root *root = queue->root;

	queue_irq = READ_CMPLQ_IRQ_STAT(root, queue->id);
	if(!msix_enabled(root->core)){
		root->comm_irq |= queue_irq;
		queue->irq_status |= root->comm_irq & queue->irq_mask;
		if(queue->irq_status & queue->irq_mask){
			return MV_TRUE;
		}
		else{ 
			return MV_FALSE;
		}
	}else if (queue_irq & queue->irq_mask) {
		queue->irq_status |= queue_irq & queue->irq_mask;
		return MV_TRUE;
	}
	else {
		return MV_FALSE;
	}
}

MV_VOID core_handle_queue_int(pl_queue *queue)
{
	/*Nancy: move queue int check from outside to here.*/
	if (!core_check_queue_int(queue)) {
		return;
	}

	if (queue->irq_status & INT_NON_SPCFC_NCQ_ERR) {
		io_chip_handle_non_spcfc_ncq_err(queue->root);
	}

	queue->irq_status = 0;
}

/*
* yuxl: handle attention for athena2 according to queue interrupt info
*/
MV_VOID core_clear_root_int_from_queue(pl_root *root, MV_U16 queue_idx)
{
	MV_U32 reg;
	MV_U8 i;
	domain_phy *phy;
	pl_queue *queue = &root->queues[queue_idx];
	core_extension *core = (core_extension *)root->core;

	if (queue->irq_status & INT_MEM_PAR_ERR) {
		/*
		    When asserted, a bit in Internal Memory Parity Error (R202D0h)
		    is set and is enabled by the corresponding bit in Internal
		    Memory Parity Error Enable (R202D4h).
		    Information related to the error is captured in the registers listed
		    in Table 9-8, Parity Error Registers.
		*/
		MV_PRINT("Error Interrupt: Internal Mem Parity Error (Bit %d) , COMMON_IRQ_STAT: 0x%08x\n",
		         ossw_ffs(INT_MEM_PAR_ERR), queue->irq_status);

		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR);
		MV_PRINT("%p off 0x%x  COMMON_PORT_INTL_MEM_PERR(0x%x) \n", root->mmio_base, COMMON_INTL_MEM_PARITY_ERR, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_MEM_PAR_ERR);
	}

	if (queue->irq_status & INT_DP_PAR_ERR) {
		/*
		        Data Path Parity Error.
		        When asserted, a data path parity error was detected.
		        Information related to the error is captured in the registers listed
		        in Table 9-8, Parity Error Registers.
		        See Data Path Parity Status (R202DCh) for more information.
		*/

		MV_PRINT("Error Interrupt: Data Path Parity Error (Bit %d) , COMMON_IRQ_STAT: 0x%08x\n",
		         ossw_ffs(INT_DP_PAR_ERR), queue->irq_status);

		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS);
		MV_PRINT("%p off 0x%x  COMMON_PORT_DPP_STATUS(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_PARITY_STS, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_ERR_ADDR);
		MV_PRINT("%p off 0x%x  COMMON_PORT_DP_ERR_ADDR_LOW(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_ERR_ADDR, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_ERR_ADDR_HI);
		MV_PRINT("%p off 0x%x COMMON_PORT_DP_ERR_ADDR_HIGH(0x%x) \n", root->mmio_base, COMMON_DATA_PATH_ERR_ADDR_HI, reg);
		reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS, reg);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_DP_PAR_ERR);
	}

	if (queue->irq_status & (INT_PORT_MASK)) {
		/* for port interrupt and PHY interrupt */
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			if (hal_has_phy_int(queue->irq_status, phy)) {

				/* clear PORT interrupt status */
				WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
				reg = READ_PORT_CONFIG_DATA(root, phy);
				CORE_EVENT_PRINT(("phy %d CONFIG_PORT_IRQ_STAT: 0x%08x\n", (root->base_phy_num+phy->id), reg));
				phy->irq_status |= (reg & phy->phy_irq_mask);
				WRITE_PORT_CONFIG_DATA(root, phy, reg);
				//Read back IRQ status register
				READ_PORT_CONFIG_DATA(root, phy);
				if (reg & IRQ_PRD_BC_ERR) {
					/*
					    Set when the amount of data transferred by a target is greater than the buffer size provided by the PRD table.
					    May be caused by a programming error (PRD too small), or by a target sending too much data.
					*/
					CORE_EVENT_PRINT(("Error Interrupt: PRD BC error (Bit %d), CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_PRD_BC_ERR), reg));
				}
				if (reg & IRQ_DMA_PEX_TO) {
					CORE_EVENT_PRINT(("Error Interrupt: DMA to PCIe Timeout (Bit %d) , CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_DMA_PEX_TO), reg));
				}
				if (reg & IRQ_STP_SATA_PHY_DEC_ERR_MASK) {
					CORE_EVENT_PRINT(("Error Interrupt: PHY Decoding Error (Bit %d) , CONFIG_PORT_IRQ_STAT: 0x%08x\n",
					                  ossw_ffs(IRQ_STP_SATA_PHY_DEC_ERR_MASK), reg));
					WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
					reg = READ_PORT_CONFIG_DATA(root, phy);
					WRITE_PORT_CONFIG_DATA(root, phy, reg);
				}
				/* clear PHY interrupt status */
				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
				reg = READ_PORT_VSR_DATA(root, phy);
				if (reg & VSR_IRQ_PHY_TIMEOUT) {
					CORE_EVENT_PRINT(("READ_PORT_VSR_DATA: 0x%08x\n", reg));
#ifndef DISABLE_VSR_IRQ_PHY_TIMEOUT
					phy->irq_status |= IRQ_PHY_RDY_CHNG_1_TO_0;
#endif
					WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);
				}
#ifdef SUPPORT_DIRECT_SATA_SSU
				if (reg & VSR_SATA_SPIN_HOLD) {
					mv_sata_handle_ssu(root, phy);
				}
#endif
				CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", (root->base_phy_num+phy->id), phy->irq_status));
			}
		}
	}
}

MV_VOID core_clear_queue_int(pl_root *root, MV_U16 queue_idx)
{
	MV_U32 main_irq;
	core_extension *core = (core_extension *)root->core;
	pl_queue *queue = &root->queues[queue_idx];
	if (!core_check_queue_int(queue)) {
		return;
	}

	core_clear_root_int_from_queue(root, queue_idx);

#ifdef DEBUG_PCI_E_ERROR
	if (queue->irq_status & INT_SYS_ERR_IRQ) {
		/* need to check SOC main irq */
		main_irq = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_INT_CAUSE_REG);
		main_irq &= core->irq_mask;

		if (main_irq & INT_MAP_PCIE_ERR) {
			MV_U32 reg;
			reg = MV_REG_READ_DWORD(core->mmio_base,
					CRYPTO_GLOBAL_SECURITY_INT_CAUSE);
			CORE_PRINT(("main irq: 0x%x.\n", main_irq));
			CORE_PRINT(("crypto int casue: 0x%x.\n",reg));
		}
	}
#endif
}

MV_VOID core_handle_root_int_from_queue(pl_root *root, MV_U16 queue_idx)
{
	pl_queue *queue = &root->queues[queue_idx];
	core_extension *core = (core_extension *)root->core;
#ifdef SUPPORT_ACTIVE_CABLE
		MV_U32 reg_value;
#endif

	if (queue->irq_status & INT_NON_SPCFC_NCQ_ERR)
		io_chip_handle_non_spcfc_ncq_err(root);

	if (queue->irq_status & INT_PORT_MASK)
	{
		root->comm_irq = queue->irq_status;
		io_chip_handle_port_int(root);
	}
#ifdef SUPPORT_ACTIVE_CABLE
	if (queue->irq_status & INT_GPIO_IRQ) {
		reg_value = MV_REG_READ_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG);
		if (reg_value & CABLE_GPIO_BITS) {
			MV_REG_WRITE_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG, 0xFF);
			core_handle_cable_gpio_int(core, reg_value);
		}
	}
#endif
#ifdef SUPPORT_I2C
	if (queue->irq_status & INT_I2C_IRQ)
		i2c_interrupt_service_routine(core);
#endif

	root->comm_irq = 0;
	queue->irq_status = 0;
}


MV_VOID io_chip_handle_attention(pl_queue *queue)
{
	pl_root *root = (pl_root *)queue->root;
#if 0
	core_extension *core = (core_extension *)root->core;
	MV_U32 queue_irq;
#endif

	//core_disable_ints(core);

#if 0 /*Nancy: for non-queue related interrupts, will be returned and not handled*/
	if (!core_check_queue_int(queue)) {
		core_enable_ints(core);
		return;
	}
#endif

#if 0
	core_clear_int(core);

	/* handle hw queue's interrupts (NON_SPCFC_NCQ_ERR) */
	core_handle_queue_int(queue);

	/* handle core's interrupts */
	core_handle_int(core);
#else		/* yuxl: queue attention handling */
	core_clear_queue_int(root, queue->id);
	core_handle_root_int_from_queue(root, queue->id);
#endif

	//core_enable_ints(core);
}

/* MV_TRUE - completion queue process finished
 * MV_FALSE - need process ATTENTION */
MV_BOOLEAN io_chip_handle_cmpl_queue(pl_queue *queue)
{
	pl_root *root = (pl_root *)queue->root;
	MV_Request *req = NULL;
	core_context *ctx = NULL;
	MV_PU32 cmpl_q; /* points to the completion entry zero */
	MV_U32 cmpl_entry;
	MV_U16 i, slot, cmpl_wp;
	struct _domain_base *base;
	MV_BOOLEAN has_attn = MV_FALSE;
	MV_ULONG flags;
#ifdef SUPPORT_PHY_POWER_MODE
		domain_phy *phy = NULL;
		core_extension *core = NULL;
		domain_port *port;
		domain_device *device;
#endif
	OSSW_SPIN_LOCK(&queue->handle_cmpl_SpinLock, flags);
	// Jira#QTS00000-2594 Debug
	root->qnap_interrupt_count++;
	cmpl_wp = (MV_U16)MV_LE32_TO_CPU(*(MV_U32 *)queue->cmpl_q_shadow) & 0x3fff;
	cmpl_q = (MV_PU32)queue->cmpl_q;
	i = queue->last_cmpl_q;
	queue->last_cmpl_q = cmpl_wp;
	if (i == queue->last_cmpl_q) {	/* no new entry */
//#ifdef REWRITE_READ_POINTER_WORKAROUND
		WRITE_CMPLQ_RD_PTR(root, queue->id, (MV_U16)cmpl_wp);
//#endif
		OSSW_SPIN_UNLOCK(&queue->handle_cmpl_SpinLock, flags);
		return MV_TRUE;
	}

	if (queue->last_cmpl_q == 0x3fff) { /* write pointer is not updated yet */
		OSSW_SPIN_UNLOCK(&queue->handle_cmpl_SpinLock, flags);
		return MV_TRUE;
	}

	while (i != queue->last_cmpl_q) {
		i++;
		if (i >= queue->cmpl_q_size) {
			i = 0;
		}

		cmpl_entry = MV_LE32_TO_CPU(cmpl_q[i]);
		if (cmpl_entry & RXQ_DEL_Q_NOT_FULL) {
			CORE_EH_PRINT(("Deliver queue not full, entry %d\n", i));
			WRITE_CMPLQ_IRQ_STAT(root, queue->id, INT_DLVRYQ_NOT_FULL);
			continue;
		}
		
		if (cmpl_entry & RXQ_ATTN) {
			/* ATTENTION */
			//queue->last_cmpl_q = i;
			CORE_EH_PRINT(("root %d queue %d Attention entry %d\n", root->root_id, queue->id, i));
			has_attn = MV_TRUE;
			continue;
		}
		slot = (MV_U16)(cmpl_entry & 0x1fff);
		if (slot >= root->slot_count_support) {
			CORE_DPRINT(("finished	slot %x exceed max slot %x.\n",
			             slot, root->slot_count_support));
			continue;
		}
		req = root->running_req[slot];
		if (req == NULL) {
			CORE_EH_PRINT(("attention: "\
			               "cannot find corresponding req on slot 0x%x on queue:%d entry:%d\n", \
			               slot, queue->id, i));
			continue;
		}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
            if(req->Req_Flag & REQ_FLAG_BYPASS_HYBRID){
                cmd_resource *cmd_res;
                MV_ASSERT(req->base_p);
                MV_ASSERT(req->Context[MODULE_HBA]);
                cmd_res = (cmd_resource *)req->Context[MODULE_HBA];
                base =  (struct _domain_base *)req->base_p;
                base->handler->process_command(root, base, &cmpl_entry, (MV_PVOID)((MV_PU8)root->cmd_struct_wrapper[slot].vir + 0x40), req);
                prot_process_cmpl_fast_req(root, req);
                continue;
            }
#endif
		ctx = req->Context[MODULE_CORE];
		base = (struct _domain_base *)get_device_by_id(root->lib_dev,
		        req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
		if (base == NULL) {
			CORE_EH_PRINT(("device base is NULL when completion slot 0x%x\n", slot));
			MV_ASSERT(MV_FALSE);
		}

		/*
		 * process_command:
		 * 1. if success, return REQ_STATUS_SUCCESS
		 * 2. if final error like disk is gone, return REQ_STATUS_NO_DEVICE
		 * 3. if want to do error handling,
		 *	  set ctx->error_info with EH_INFO_NEED_RETRY
		 *	  and also set the Scsi_Status properly to
		 *	  a. REQ_STATUS_HAS_SENSE if has sense
		 *	  b. for timeout, the status should be REQ_STATUS_TIMEOUT
		 *	  c. REQ_STATUS_ERROR for all other error
		 */
		// Jira#QTS00000-2594 Debug
		port = base->port;
		port->qnap_i_counter = i;
		port->qnap_last_cmpl_q = queue->last_cmpl_q;
		((command_handler *)ctx->handler)->process_command(root, base, &cmpl_entry,
		        (MV_PVOID)((MV_PU8)root->cmd_struct_wrapper[slot].vir + 0x40), req);

		MV_DASSERT((req->Scsi_Status == REQ_STATUS_SUCCESS)
		           || (req->Scsi_Status == REQ_STATUS_TIMEOUT)
		           || (req->Scsi_Status == REQ_STATUS_NO_DEVICE)
		           || (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
		           || (req->Scsi_Status == REQ_STATUS_ERROR)
		           || (req->Scsi_Status == REQ_STATUS_BUSY));
#ifdef SUPPORT_PHY_POWER_MODE//after IO re-count device timer
		if(base->port){
			port = base->port;
			phy = port->phy;
			core = (core_extension *)root->core;
			device = (domain_device *)base;
			if(phy && core && (port->device_count != 0) 
			&& (port->pm == NULL) && (port->expander_count == 0)
			&& (core->state == CORE_STATE_STARTED) 
			&& (core->PHY_power_mode_HIPM != 0) 
			&& (core->PHY_power_mode_port_map & (1<<(root->base_phy_num + phy->asic_id)))
			&& (device->base.type == BASE_TYPE_DOMAIN_DEVICE )
			&& (device->capability & DEVICE_CAPABILITY_HIPM_SUPPORTED)){
				phy->In_PHY_power_mode_HIPM = 0;
				phy->PHY_power_mode_HIPM_timer = 0;
				if(core->PHY_power_mode_timer_index == 0xffff){
					core->PHY_power_mode_timer_index = Timer_AddRequest(core, 2, Core_EnablePowerStateHIPM, core, NULL);
				}
			}
		}
#endif
		prot_process_cmpl_req(root, req);
	}
	WRITE_CMPLQ_RD_PTR(root, queue->id, (MV_U16)cmpl_wp);
	OSSW_SPIN_UNLOCK(&queue->handle_cmpl_SpinLock, flags);
	if (MV_TRUE == has_attn)
		return MV_FALSE;
	else
		return MV_TRUE;
}

MV_U16 prot_set_up_sg_table(MV_PVOID root_p, MV_Request *req,
                            MV_PVOID sg_wrapper_p)
{
	pl_root *root = (pl_root *)root_p;
#if defined(HAVE_PRD_SKIP)
	prd_skip_t *prd_skip;
	MV_U32 dword;
#endif
	hw_buf_wrapper *sg_wrapper = (hw_buf_wrapper *)sg_wrapper_p;
	MV_U16 consumed = 0;
	if (req->SG_Table.Valid_Entry_Count > 0) {
		MV_DASSERT(req->Data_Transfer_Length > 0);
		MV_DASSERT(req->SG_Table.Byte_Count == req->Data_Transfer_Length);
		
		if ((req->SG_Table.Flag & SGT_FLAG_PRDT_IN_HOST) || 
			(req->SG_Table.Flag & SGT_FLAG_PRDT_HW_COMPLIANT)){
			MV_U32 dw2 = 0;
			MV_U32 tmp_low, tmp_high;
			prd_t* prd;
			PMV_SG_Entry sg;
			MV_U16 i = 0, remain = 0;
			
			sg = ((PMV_SG_Entry)sg_wrapper->vir);
			consumed = req->SG_Table.Valid_Entry_Count;
			remain = consumed;
			while(remain) {
				if(sgd_eot(sg))
					sgd_clear_eot(sg);
				//sg->flags = 0; /*clear interal used flags*/
				prd = (prd_t *)sg;
				tmp_low = sg->baseAddr.parts.low;
				tmp_high = sg->baseAddr.parts.high;	
				dw2 = prd->size;
#if defined(SUPPORT_ROC)
				dw2 |= INTRFC_CORE_DMA<<PRD_IF_SELECT_SHIFT;
#else
				dw2 |= INTRFC_PCIEA<<PRD_IF_SELECT_SHIFT;
#endif
				prd->baseAddr_low = MV_CPU_TO_LE32(tmp_low); /*prd entry is always little-endian*/
				prd->baseAddr_high = MV_CPU_TO_LE32(tmp_high);
				prd->size = MV_CPU_TO_LE32(dw2);

				sg++;
				i++;
				remain --;
				
			} 
		}
		else {
#if defined(HAVE_PRD_SKIP)
			prd_skip = (prd_skip_t *)sg_wrapper->vir;
			if (req->Cmd_Flag & CMD_FLAG_PRD_SKIP) {
				dword = MV_CPU_TO_LE32(SECTOR_SIZE >> 2);
				prd_skip->ctrl_size = dword;
				prd_skip->init_block_xfer = req->init_block_xfer;
				prd_skip->init_block_skip = req->init_block_skip;
				prd_skip->sub_block_xfer = req->sub_block_xfer;
				prd_skip->sub_block_skip = req->sub_block_skip;
				prd_skip++;
				consumed++;
			}
			consumed += (MV_U16) core_prepare_hwprd(root->core,
				&req->SG_Table, (MV_PVOID)prd_skip);
#else
			consumed = (MV_U16) core_prepare_hwprd(root->core,
				&req->SG_Table, sg_wrapper->vir);
#endif

			if (consumed == 0) {
				/* resource not enough... */
				CORE_DPRINT(("Run out of PRD entry.\n"));
				/* check why upper layer send request with too many sg items... */
				MV_ASSERT(MV_FALSE);
			}

		}

	}
	else {
		MV_DASSERT(req->Data_Transfer_Length == 0);
	}

	return consumed;
}

MV_VOID core_fill_prd(MV_PVOID prd_ctx, MV_U64 bass_addr, MV_U32 size)
{
	prd_context *ctx = (prd_context *)prd_ctx;
	MV_U32 dw2 = 0;

	MV_ASSERT(ctx->avail);

	dw2 = size;
	dw2 &= ~PRD_CHAIN_BIT;

	ctx->prd->baseAddr_low = MV_CPU_TO_LE32(bass_addr.parts.low);
	ctx->prd->baseAddr_high = MV_CPU_TO_LE32(bass_addr.parts.high);

#if defined(MEM_SELECT_INTERFACE)
	if (MEM_INTERFACE_CHECK_HOST(ctx->prd->baseAddr_high)) {
		/*
		 * XH TODO: frey has 4 PCIe functions, needs revise if all
		 * functions are used in the future.
		 */
		MEM_INTERFACE_CLEAR_HOST(ctx->prd->baseAddr_high);
		dw2 |= INTRFC_PCIEA << PRD_IF_SELECT_SHIFT;
	}
	else
#if defined(SUPPORT_ROC)
		dw2 |= INTRFC_CORE_DMA << PRD_IF_SELECT_SHIFT;
#else
		dw2 |= INTRFC_PCIEA << PRD_IF_SELECT_SHIFT;
#endif
#else //defined(MEM_SELECT_INTERFACE)
#if defined(SUPPORT_ROC)
	dw2 |= INTRFC_CORE_DMA << PRD_IF_SELECT_SHIFT;
#else
	dw2 |= INTRFC_PCIEA << PRD_IF_SELECT_SHIFT;
#endif
#endif /* MEM_SELECT_INTERFACE */
	ctx->prd->size = MV_CPU_TO_LE32(dw2);

	ctx->prd++;
	MV_ASSERT(ctx->avail > 0);
	ctx->avail--;

}

int core_prepare_hwprd(MV_PVOID core, sgd_tbl_t *source,
                       MV_PVOID prd)
{
	prd_context ctx;

	ctx.prd = (prd_t *)prd;
	ctx.avail = ((core_extension *)core)->hw_sg_entry_count;
	return sgdt_prepare_hwprd(core, source, &ctx, core_fill_prd);
}


#ifdef SUPPORT_CONFIG_FILE

MV_U8 core_phy_get_status(MV_PVOID core_p, PMV_Request req)
{
	MV_U8		phy_id = 0xFF;
	MV_U8		index = 0xFF;
	pl_root		*root = NULL;
	domain_phy	*phy = NULL;
	domain_port	*port = NULL;
	core_extension	*core = NULL;
	pMv_Phy_Status	phy_status = NULL;

	core = (core_extension *)core_p;
	       phy_id	= req->Cdb[2];

	if (phy_id < 4) {
		root = &core->roots[0];

	}
	else {
		root = &core->roots[1];
		phy_id	= req->Cdb[2] - 4;
	}
	if (phy_id >= root->phy_num) {
		MV_PRINT("PHY ID should be 0~ %d!\n", (req->Cdb[2] - 1));
		return REQ_STATUS_INVALID_PARAMETER;
	}

	phy_status = (pMv_Phy_Status)core_map_data_buffer(req);

	if (phy_status == NULL) {
		return REQ_STATUS_ERROR;
	}
	for (index = 0; index < root->phy_num; index++) {
		port =  &root->ports[index];
		if (port != NULL) {
			phy = &root->phy[phy_id];
			if (port->phy_map & MV_BIT(phy_id)) {
				phy_status->device_type = port->type;
				phy_status->subtractive = MV_TRUE;
				MV_CopyMemory((MV_PVOID)&phy_status->sas_address, (MV_PVOID) & (phy->att_dev_sas_addr), 8);
				phy_status->link_rate = get_min_negotiated_link_rate(port);
				break;
			}
		}
	}
	core_unmap_data_buffer(req);

	return	REQ_STATUS_SUCCESS;
};

MV_U8 set_phy_ffe(pl_root *root, domain_phy *phy, MV_U8 phy_ffe)
{
	MV_U32 temp = 0;

	if (phy_ffe == 0xFF) { //invalid
		phy_ffe = 0x77;    //default value
	}

	phy_ffe &= 0x7F; /*bit 0 ~ 6*/

	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_FFE_CONTROL);
	temp = READ_PORT_VSR_DATA(root, phy);
	temp &= 0xFFFFFF80L; /*clear bit 0~6*/
	temp |= phy_ffe;
	WRITE_PORT_VSR_DATA(root, phy, temp);

	return	REQ_STATUS_SUCCESS;
}

MV_U8 core_phy_config(MV_PVOID core_p, PMV_Request req)
{
	MV_U32          temp = 0;
	MV_U8		phy_id = 0xFF;
	pl_root 	*root = NULL;
	domain_phy      *phy = NULL;
	core_extension	*core = NULL;
	HBA_Info_Page	hba_info_param;

	core = (core_extension *)core_p;

	phy_id	= req->Cdb[2];
	if (phy_id < 4) {
		root = &core->roots[0];
	}
	else {
		root = &core->roots[1];
		phy_id	= req->Cdb[2] - 4;
	}
	if (phy_id >= root->phy_num) {
		MV_PRINT("PHY ID should be 0~ %d!\n", (req->Cdb[2] - 1));
		return REQ_STATUS_INVALID_PARAMETER;
	}

	phy = &root->phy[phy_id];
	MV_ZeroMemory(&hba_info_param, sizeof(hba_info_param));

	if (!mv_nvram_init_param(HBA_GetModuleExtension((root->core), MODULE_HBA),
	                         &hba_info_param)) {
		MV_DPRINT(("Core_phy_test:hba info invalid .\n"));
	}
	temp = (MV_U8)(*(MV_PU8)(&(hba_info_param.FFE_Control[phy_id])));
	set_phy_ffe(root, phy, (MV_U8)temp);

	temp = (MV_U32)(*(MV_PU32)&hba_info_param.PHY_Tuning[phy_id]);
	if (temp != 0xFFFFFFFFL) { // check the field if valid
		MV_DPRINT(("Core_phy_test:set phy tunning phy_id = 0x%x\n", phy_id));
		set_phy_tuning(root, phy, hba_info_param.PHY_Tuning[phy_id]);
		set_phy_ffe_tuning(root, phy, hba_info_param.FFE_Control[phy_id]);
	}
	return	REQ_STATUS_SUCCESS;
}


MV_U8 core_phy_test(MV_PVOID core_p, PMV_Request req)
{
	MV_U8		phy_id = 0xFF;
	MV_U8		phy_rate = 0;
	MV_U32		tmp = 0, temp = 0;
	pl_root 	*root = NULL;
	domain_phy      *phy = NULL;
	core_extension  *core = NULL;
	HBA_Info_Page	hba_info_param;

	core = (core_extension *)core_p;

	phy_id	= req->Cdb[2];
	if (phy_id < 4) {
		root = &core->roots[0];
	}
	else {
		root = &core->roots[1];
		phy_id	= req->Cdb[2] - 4;
	}

	if (phy_id >= root->phy_num) {
		MV_PRINT("PHY ID should be 0~ %d!\n", (req->Cdb[2] - 1));
		return REQ_STATUS_INVALID_PARAMETER;
	}

	phy_rate = req->Cdb[3];
	phy = &root->phy[phy_id];
	MV_ZeroMemory(&hba_info_param, sizeof(hba_info_param));

	if (!mv_nvram_init_param(HBA_GetModuleExtension((root->core), MODULE_HBA),
	                         &hba_info_param)) {
		MV_DPRINT(("Core_phy_test:hba info invalid .\n"));
	}


	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CONFIG, 0x2F);

	/*R4Eh -- Set PHY speed control enable*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x19C);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp1 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x01000000);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp2 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	/*R00h -- Chip ID*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x100);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp3 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0xF5241000);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp4 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	if (phy_rate == 0) { // 1.5 G
		/*R26h -- Select 6G(22) rate for Tx/Rx,3G(11),1.5G(00)*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x14C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp5 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x00008000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp6 = 0x%x\n", tmp));
		HBA_SleepMillisecond(core, 200);
	}
	else if (phy_rate == 0x1) { // 3.0 G
		/*R26h -- Select 6G(22) rate for Tx/Rx,3G(11),1.5G(00)*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x14C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp5 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x00008011);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp6 = 0x%x\n", tmp));
		HBA_SleepMillisecond(core, 200);

	}
	else if (phy_rate == 0x2) { // 6.0 G
		/*R26h -- Select 6G(22) rate for Tx/Rx,3G(11),1.5G(00)*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x14C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp5 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x00008022);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp6 = 0x%x\n", tmp));
		HBA_SleepMillisecond(core, 200);

	}
	else { // 3.0 G
		/*R26h -- Select 6G(22) rate for Tx/Rx,3G(11),1.5G(00)*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x14C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp5 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x00008011);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp6 = 0x%x\n", tmp));
		HBA_SleepMillisecond(core, 200);

	}
	/*R22h & R23h -- PW and external loopback*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x144);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp7 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x08001006);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp8 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	/*R02h & R03h -- KVCO and Impedance cal*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x104);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp9 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x3633F544);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp10 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	/*R15h -- Pattern sel (Jit and PRBS)*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x128);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp11 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x01E00000);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp12 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	/*R1Bh -- PT_Data (PRBS7)*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x134);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp13 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x00800000);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp14 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	/*R1Bh -- PT_Data (1T)*/
	/*
	WRITE_PORT_VSR_ADDR(pSASModule,phyID, 0x134);
	tmp = READ_PORT_VSR_DATA(pSASModule, phyID);
	MV_DPRINT(("Core_phy_test:tmp15 = 0x%x\n",tmp));

	WRITE_PORT_VSR_DATA(pSASModule, phyID, 0x00100000);
	tmp = READ_PORT_VSR_DATA(pSASModule, phyID);
	MV_DPRINT(("Core_phy_test:tmp16 = 0x%x\n",tmp));
	HBA_SleepMillisecond(pCore, 200);
	*/
	/*R15h -- PT_EN*/
	WRITE_PORT_VSR_ADDR(root, phy, 0x128);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp17 = 0x%x\n", tmp));

	WRITE_PORT_VSR_DATA(root, phy, 0x81E00000);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp18 = 0x%x\n", tmp));
	HBA_SleepMillisecond(core, 200);

	if (phy_rate == 0) { // 1.5 G
		/*Adjust TX AMP*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x118);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp19 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0xC96A0000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp20 = 0x%x\n", tmp));

		/*Adjust G1_TXAMP_ADJ*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x11C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp21 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0xAA619000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp22 = 0x%x\n", tmp));


	}
	else if (phy_rate == 0x1) { // 3.0 G
		/*Adjust TX AMP*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x11C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp19 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0xAA710000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp20 = 0x%x\n", tmp));

		/*Adjust G1_TXAMP_ADJ*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x120);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp21 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x0B6B9000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp22 = 0x%x\n", tmp));

	}
	else if (phy_rate == 0x2) { // 6.0 G
		WRITE_PORT_VSR_ADDR(root, phy, 0x120);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp19 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x0F7F0000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp20 = 0x%x\n", tmp));

		/*Adjust G1_TXAMP_ADJ*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x124);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp21 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x0BEBD155);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp22 = 0x%x\n", tmp));



	}
	else {
		MV_PRINT("2 phy %d:Unknown phy rate, use default value 3.0Gbps to do phy test!\n", phy_id);

		/*Adjust TX AMP*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x11C);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp19 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0xAA710000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp20 = 0x%x\n", tmp));

		/*Adjust G1_TXAMP_ADJ*/
		WRITE_PORT_VSR_ADDR(root, phy, 0x120);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp21 = 0x%x\n", tmp));

		WRITE_PORT_VSR_DATA(root, phy, 0x0B6B9000);
		tmp = READ_PORT_VSR_DATA(root, phy);
		MV_DPRINT(("Core_phy_test:tmp22 = 0x%x\n", tmp));

	}


	temp = (MV_U32)(*(MV_PU32)&hba_info_param.PHY_Tuning[phy_id]);
	MV_DPRINT(("Core_phy_test:temp = 0x%x\n", temp));
	if (temp != 0xFFFFFFFFL) { // check the field if valid
		MV_DPRINT(("Core_phy_test:set phy tunning phyID = 0x%x\n", phy_id));
		set_phy_tuning(root, phy, hba_info_param.PHY_Tuning[phy_id]);
		set_phy_ffe_tuning(root, phy, hba_info_param.FFE_Control[phy_id]);
		//set_phy_rate(root, phy,hba_info_param.PHY_Rate[phy_id]);
	}
	else {
		MV_PRINT("phy %d:Invalid phy tuning value stored in SPI flash,need restore_cfg config file.\n", phy_id);
		return REQ_STATUS_INVALID_PARAMETER;
	}

	/*R15h -- ##read - Bit29:PT_PASS,Bit28:PT_LOCK*/
	//MV_REG_WRITE_DWORD(root->mmio_base, 0x290, 0x128);
	WRITE_PORT_VSR_ADDR(root, phy, 0x128);
	tmp = READ_PORT_VSR_DATA(root, phy);
	MV_DPRINT(("Core_phy_test:tmp23 = 0x%x\n", tmp));
	MV_DPRINT(("Core_phy_test:return MV_TRUE!\n"));

	return REQ_STATUS_SUCCESS;
}

#endif

#ifdef SUPPORT_BOARD_ALARM
MV_VOID
core_alarm_enable_register(MV_PVOID core_p)
{
	MV_U32 reg;
	core_extension *core = (core_extension *) core_p;

	reg = MV_REG_READ_DWORD(core->mmio_base, TEST_PIN_OUTPUT_ENABLE);
	MV_REG_WRITE_DWORD(core->mmio_base, TEST_PIN_OUTPUT_ENABLE, reg | TEST_PIN_BUZZER);
}

MV_VOID
core_alarm_set_register(MV_PVOID core_p, MV_U8 value)
{
	MV_U32 reg;
	core_extension *core = (core_extension *) core_p;

	reg = MV_REG_READ_DWORD(core->mmio_base, TEST_PIN_OUTPUT_VALUE);

	if (value == MV_TRUE) {
		reg |= TEST_PIN_BUZZER;
	}
	else {
		reg &= ~TEST_PIN_BUZZER;
	}

	MV_REG_WRITE_DWORD(core->mmio_base, TEST_PIN_OUTPUT_VALUE, reg);
}
#endif /* SUPPORT_BOARD_ALARM */

MV_U32 mv_is_phy_ready(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;
	WRITE_PORT_CONFIG_ADDR(root, phy , CONFIG_PORT_SERIAL_CTRL_STS);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	return (reg & SCTRL_PHY_READY_MASK);
}

MV_U32 READ_PORT_IRQ_STAT(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	return reg;
}

void WRITE_PORT_IRQ_STAT(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 value)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
	WRITE_PORT_CONFIG_DATA(root, phy, value);
}

MV_U32 READ_PORT_IRQ_MASK(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	return reg;
}
void WRITE_PORT_IRQ_MASK(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 irq_mask)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
	WRITE_PORT_CONFIG_DATA(root, phy, irq_mask);
}

MV_U32 READ_PORT_PHY_CONTROL(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	return reg;
}

void WRITE_PORT_PHY_CONTROL(MV_PVOID root_p, MV_PVOID phy_p, MV_U32 value)
{
	pl_root *root = root_p;
	domain_phy *phy = (domain_phy *)phy_p;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
	WRITE_PORT_CONFIG_DATA(root, phy, value);
}

