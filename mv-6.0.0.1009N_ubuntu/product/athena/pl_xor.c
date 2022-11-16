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
#ifdef HARDWARE_XOR
#include "core_xor.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_manager.h"
#include "core_resource.h"
#include "core_protocol.h"
#include "core_util.h"
#include "core_error.h"

/*******************************************************************************
*                                                                              *
* XOR Utilities                                                                *
*                                                                              *
*******************************************************************************/
/*
 xor_dump_register

 Prints out major registers inside the XOR engine.
*/
void xor_dump_register(MV_PVOID This)
{
        core_extension *core = (core_extension *) This;
        MV_LPVOID       mmio;
        MV_U32          reg;
        MV_U16          i;

        for (i = 0; i < MV_MAX_XOR_ID; i++) {
                mmio =  (MV_LPVOID)((MV_PU8)core->mmio_base
                        + (MV_XOR_CORE_BASE_ADDR
                        + (i * MV_XOR_CORE_ADDR_INCR)));

                CORE_DPRINT(("\nXOR CORE %d, BASE ADDRESS = 0x%x\n", i, mmio));

                reg = MV_REG_READ_DWORD(mmio, XOR_CONTROL);
                CORE_DPRINT(("XOR_CONTROL(R%x) = 0x%x\n",
                        XOR_CONTROL, reg));
                reg = MV_REG_READ_DWORD(mmio, XOR_IRQ_STAT);
                CORE_DPRINT(("XOR_IRQ_STAT(R%x) = 0x%x\n",
                        XOR_IRQ_STAT, reg));
                reg = MV_REG_READ_DWORD(mmio, XOR_ERR_TBL_ADDR_HI);
                CORE_DPRINT(("XOR_ERR_TBL_ADDR_HI(R%x) = 0x%x\n",
                        XOR_ERR_TBL_ADDR_HI, reg));
                reg = MV_REG_READ_DWORD(mmio, XOR_ERR_TBL_ADDR);
                CORE_DPRINT(("XOR_ERR_TBL_ADDR(R%x) = 0x%x\n",
                        XOR_ERR_TBL_ADDR, reg));
                
//                reg = MV_REG_READ_DWORD(mmio, COMMON_IRQ_MASK );
//                CORE_DPRINT(("COMMON_IRQ_MASK = 0x%x\n",reg));
                reg = MV_REG_READ_DWORD(mmio, COMMON_IRQ_STAT );
                CORE_DPRINT(("COMMON_IRQ_STAT = 0x%x\n",reg));
                reg = MV_REG_READ_DWORD(mmio, XOR_DELV_Q_RD_PTR );
                CORE_DPRINT(("XOR_DELV_Q_RD_PTR(R%x) = 0x%x\n",
                        XOR_DELV_Q_RD_PTR, reg));
                reg = MV_REG_READ_DWORD(mmio, XOR_CMPL_Q_WR_PTR );
                CORE_DPRINT(("XOR_CMPL_Q_WR_PTR(R%x) = 0x%x\n",
                        XOR_CMPL_Q_WR_PTR, reg));
        }

}
void xor_disable_ints(MV_PVOID This, MV_U16 xor_id){
    core_extension *core = (core_extension *)This;
    xor_engine* xor_core= &core->xors.engine[xor_id];
    MV_REG_WRITE_DWORD(xor_core->xor_mmio_base, XOR_IRQ_MASK, 0);
}
void xor_enable_ints(MV_PVOID This, MV_U16 xor_id){
    core_extension *core = (core_extension *)This;
    xor_engine* xor_core= &core->xors.engine[xor_id];
    MV_REG_WRITE_DWORD(xor_core->xor_mmio_base, XOR_IRQ_MASK, xor_core->xor_int_mask);
}
/*
 xor_write_delv_q_entry

 Prepares an XOR request for the specified core of the XOR engine.
*/
void xor_prepare_delv_q_entry(
        MV_U16 slot_num,
        xor_delv_q_entry *delv_q_entry,
        MV_PHYSICAL_ADDR xor_tbl_dma)
{
    memset(delv_q_entry, 0 , sizeof(xor_delv_q_entry));
    delv_q_entry->SLOT_NM = slot_num;
    delv_q_entry->IFC_SLCT = INTRFC_PCIEA;
    delv_q_entry->XOR_PI_TBL_LNGTH = XOR_TABLE_LENGTH;
    delv_q_entry->XOR_TBL_ADDR_L = xor_tbl_dma.parts.low;
    delv_q_entry->XOR_PI_ADDR_H = xor_tbl_dma.parts.high;
}
/*
 xor_write_delv_q_entry

 Writes an XOR request to the specified core of the XOR engine.
*/
void xor_write_delv_q_entry(
        xor_engine *xor_core,
        xor_delv_q_entry *delv_q_entry)
{
        MV_U32 tmp;
        MV_LPVOID mmio = xor_core->xor_mmio_base;
        xor_delv_q_entry *delv_q_list;

        delv_q_list = (xor_delv_q_entry *)xor_core->xor_delv_q;
        xor_core->xor_last_delv_q++;

        if (xor_core->xor_last_delv_q >= xor_core->xor_delv_q_size)
                xor_core->xor_last_delv_q = 0;

        tmp = (MV_U32)(xor_core->xor_last_delv_q);

        memcpy(&delv_q_list[tmp],delv_q_entry,sizeof(xor_delv_q_entry));
        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_WR_PTR, tmp);
}
#if defined RAID6_MULTIPLE_PARITY
void xor_fill_prd_entry(
        xor_cmd_entry   *prd,
        MV_XOR_Request  *xor_req,
        hw_buf_wrapper  *sg_buffer,
        MV_U32          consumed,
        MV_U8           opcode0,
        MV_U8           opcode1,
        MV_U8           opcode2,
        MV_U8           sindex,
        MV_U8           tindex,
        MV_U8           data0,
        MV_U8           data1,
        MV_U8           data2,
        MV_U8           target_sg_table_count)
#else
void xor_fill_prd_entry(
        xor_cmd_entry   *prd,
        MV_XOR_Request  *xor_req,
        hw_buf_wrapper  *sg_buffer,
        MV_U32          consumed,
        MV_U8           opcode0,
        MV_U8           sindex,
        MV_U8           tindex,
        MV_U8           data0,
        MV_U8           target_sg_table_count)
#endif
{
        *((MV_PU32)prd) = 0;
        prd->prd_entry_count = (MV_U8) consumed;
        prd->prd_tbl_addr = sg_buffer->phy.parts.low;
        prd->prd_tbl_addr_high = sg_buffer->phy.parts.high;
        prd->buffer_0_opcode = opcode0;

#if !defined(RUN_AS_PCIE_DRIVER)
#if defined(DDR_MEM_SUPPORT) || defined(SUPPORT_ROC)
        prd->prd_tbl_intrfc = INTRFC_DDR_CS0;
#else
        prd->prd_tbl_intrfc = INTRFC_SRAM;
#endif /* DDR_MEM_SUPPORT */

#else  /* RUN_AS_PCIE_DRIVER */
        prd->prd_tbl_intrfc = INTRFC_PCIEA;
#endif /* RUN_AS_PCIE_DRIVER */

        prd->buffer_0_opcode = opcode0;
        if (opcode0 == OP_GF_ENABLE || opcode0 == OP_GF_XOR_ENABLE)
                prd->data_byte_0 = xor_get_coef(xor_req, sindex, tindex);
        else
                prd->data_byte_0 = data0;

#if defined RAID6_MULTIPLE_PARITY
        prd->buffer_1_opcode = opcode1;
        if (opcode1 == OP_GF_ENABLE || opcode1 == OP_GF_XOR_ENABLE)
                prd->data_byte_1 = xor_get_coef(xor_req, sindex, tindex + 1);
        else
                prd->data_byte_1 = data1;
        prd->buffer_2_opcode = opcode2;
        if (opcode2 == OP_GF_ENABLE || opcode2 == OP_GF_XOR_ENABLE)
                prd->data_byte_2 = xor_get_coef(xor_req, sindex, tindex + 2);
        else
                prd->data_byte_2 = data2;
#endif /* RAID6_MULTIPLE_PARITY */
}

/*
 xor_reset_all_cores

 Resets the XOR engine.
*/
void xor_reset_all_cores(MV_PVOID core_p)
{
        core_extension *core = (core_extension *)core_p;
        MV_LPVOID mmio;
        MV_U16 i;

        /* reset */
        for (i = 0; i < MV_MAX_XOR_ID; i++) {
                mmio =  (MV_LPVOID)((MV_PU8)core->mmio_base
                        + (MV_XOR_CORE_BASE_ADDR
                        + (i * MV_XOR_CORE_ADDR_INCR)));

                MV_REG_WRITE_DWORD(
                        mmio,
                        XOR_CONTROL,
                        XOR_RESET);
        }

        core_sleep_millisecond(core, 1);

        /* turn off reset */
        for (i = 0; i < MV_MAX_XOR_ID; i++) {
                mmio =  (MV_LPVOID)((MV_PU8)core->mmio_base
                        + (MV_XOR_CORE_BASE_ADDR
                        + (i * MV_XOR_CORE_ADDR_INCR)));

                MV_REG_WRITE_DWORD(
                        mmio,
                        XOR_CONTROL,
                        0);
        }

        core_sleep_millisecond(core, 1);
}
#ifdef SUPPORT_INTL_PARITY
void xor_intl_parity_enable(MV_LPVOID mmio,MV_U8 enable,MV_U8 isr_enable){
    MV_U32 reg=0;
    if(enable){
        reg = XOR_ERR_MEM_PRERR_MASK;
    }
    else{
        reg = 0;
    }
    MV_PRINT("mmio(%p) 0x%x %s XOR_ERR_MEM_PRERR_EN(0x%x) \n", mmio, XOR_ERR_MEM_PRERR_EN, ((reg)?"Enable":"Disable"), reg);
    MV_REG_WRITE_DWORD(mmio,XOR_ERR_MEM_PRERR_EN,reg);
    reg = MV_REG_READ_DWORD(mmio,XOR_ERR_MEM_PRERR_EN);

    MV_REG_WRITE_DWORD(mmio,XOR_ERR_MEM_PRERR, 0);
    reg = MV_REG_READ_DWORD(mmio,XOR_ERR_MEM_PRERR);
    
    reg = MV_REG_READ_DWORD(mmio, XOR_IRQ_MASK);
    if(isr_enable)
        reg |= XOR_INT_MEM_PERR_IRQ; 
    else
        reg &= ~XOR_INT_MEM_PERR_IRQ;
    MV_REG_WRITE_DWORD(mmio, XOR_IRQ_MASK,reg);
}
#endif
#ifdef SUPPORT_DPP
void xor_dpp_enable(MV_LPVOID mmio,MV_U8 enable,MV_U8 odd,MV_U8 isr_enable){
    MV_U32 parity=0,reg=0;
    if(enable){
        if(odd)
            parity=(XOR_DP_PCRTL_DP_FERR_EN|XOR_DP_PCRTL_DP_PERR_EN|XOR_DP_PCRTL_DP_PAR_ODD);
        else
            parity=(XOR_DP_PCRTL_DP_FERR_EN|XOR_DP_PCRTL_DP_PERR_EN);
    }
    MV_PRINT("%s  Xor Engine Data Path Parity register Parity(0x%x)\n",((enable)?"Enable":"Disable"),parity);
    MV_REG_WRITE_DWORD(mmio,XOR_DP_PCRTL,parity);
    reg = MV_REG_READ_DWORD(mmio,XOR_DP_PCRTL);

    MV_REG_WRITE_DWORD(mmio,XOR_DP_PSTAT, 0);
    reg = MV_REG_READ_DWORD(mmio,XOR_DP_PSTAT);

    reg = MV_REG_READ_DWORD(mmio, XOR_IRQ_MASK);
    if(isr_enable)
        reg |= XOR_DP_PERR_IRQ; //(XOR_DP_PERR_IRQ | XOR_INT_MEM_PERR_IRQ);
    else
        reg &= ~XOR_DP_PERR_IRQ;//~(XOR_DP_PERR_IRQ | XOR_INT_MEM_PERR_IRQ);
    MV_REG_WRITE_DWORD(mmio, XOR_IRQ_MASK,reg);
//    MV_PRINT("mmio(%p) 0x%x  Enable XOR_IRQ_MASK(0x%x) \n", mmio, XOR_IRQ_MASK, reg);
}
#endif
void
xor_init_core(MV_PVOID core_p, MV_U16 core_id)
{
        core_extension  *core = (core_extension *)core_p;
        xor_engine      *xor_core;
        MV_LPVOID       mmio;
        MV_U32          tmp;

        xor_core = &core->xors.engine[core_id];
        mmio = xor_core->xor_mmio_base;

        /* assign XOR delivery queue address */
        tmp = 0;
        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_CONFIG, tmp);
        tmp =  XOR_DELV_QUEUE_SIZE_MASK & xor_core->xor_delv_q_size;


        tmp |= (INTRFC_PCIEA << 16) ;

#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
        tmp |= XOR_DELV_Q_WP_SHDW_EN;
#endif
        tmp |= XOR_DELV_QUEUE_ENABLE;
        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_CONFIG, tmp);

        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_ADDR,
                xor_core->xor_delv_q_dma.parts.low);
        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_ADDR_HI,
                xor_core->xor_delv_q_dma.parts.high);
#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
        MV_REG_WRITE_DWORD(mmio, XOR_DELV_Q_SHDW_ADDR,
                xor_core->xor_shadow_delv_q_dma.parts.low);
#endif
        /* assign XOR completion queue address */
        tmp = 0;
        MV_REG_WRITE_DWORD(mmio, XOR_CMPL_Q_CONFIG, tmp);
        tmp =  XOR_CMPL_QUEUE_SIZE_MASK & xor_core->xor_cmpl_q_size;

        tmp |= (INTRFC_PCIEA << 16) ;

#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
        tmp |= XOR_CMPL_Q_WP_SHDW_EN;
#endif
        tmp |= XOR_CMPL_QUEUE_ENABLE;
        MV_REG_WRITE_DWORD(mmio, XOR_CMPL_Q_CONFIG, tmp);

        MV_REG_WRITE_DWORD(mmio, XOR_CMPL_Q_ADDR,
                xor_core->xor_cmpl_q_dma.parts.low);
        MV_REG_WRITE_DWORD(mmio, XOR_CMPL_Q_ADDR_HI,
                xor_core->xor_cmpl_q_dma.parts.high);

        /* write in a default value for completion queue pointer
           in memory */
        tmp = XOR_CMPL_QUEUE_SIZE_MASK;
#ifdef ATHENA_XOR_DELV_SHADOW_ENABLE
        MV_REG_WRITE_DWORD(mmio, XOR_CMPL_Q_SHDW_ADDR,
                xor_core->xor_shadow_cmpl_q_dma.parts.low);
        (*(MV_PU32)xor_core->xor_shadow_cmpl_q) = tmp;
#endif

#ifdef XOR_COALESCE_INT
        /* XOR Interrupt Coalescing Configuration*/
        tmp  = XOR_INT_COAL_COUNT_MASK & xor_core->slot_count_support;
        tmp |= XOR_INT_COAL_ENABLE;
        MV_REG_WRITE_DWORD(mmio, XOR_COAL_CONFIG, tmp);
#else
        /* XOR coalescing needs to be turned on for some older odin chip      *
         * version but, once it is enabled, it may cause another problem of   *
         * interrupt missing and the max count is 0x1ff, while our slot count *
         * is 0x200, it will make the count 0                                 */
        tmp = 0;
        MV_REG_WRITE_DWORD(mmio, XOR_COAL_CONFIG, tmp);
#endif

        /* XOR Interrupt Coalescing Timeout*/
        tmp  = XOR_COAL_TIMER_MASK;
        MV_REG_WRITE_DWORD(mmio, XOR_COAL_TIMEOUT, tmp);

        /* enable XOR CMD/CMPL_Q/RESP mode */
        tmp = 0;

        tmp |= XOR_ENABLE;
        MV_REG_WRITE_DWORD(mmio, XOR_CONTROL, tmp);
        if(msix_enabled(core)){
            MV_REG_WRITE_DWORD(mmio, XOR_IRQ_ROUTING, xor_core->irq_rounting);
        }
/* enable XOR done queue interrupt */
        tmp = (XOR_CQ_NOT_EMPTY
                | XOR_TBL_PROC_IRQ
		| XOR_OVERRUN_IRQ
                | XOR_UNDERRUN_IRQ
                | XOR_B0_ZERO_RSLTCHK_ERR_IRQ
                | XOR_B1_ZERO_RSLTCHK_ERR_IRQ
                | XOR_B2_ZERO_RSLTCHK_ERR_IRQ
#ifdef DEBUG_PCI_E_ERROR
                | XOR_SYS_ERR_IRQ
#endif
                );
#ifdef SUPPORT_DIF
        tmp |=  (XOR_REF_CHK_ERR
                | XOR_APP_CHK_ERR
                | XOR_GRD_CHK_ERR
                | XOR_T10_CNTRL_ERR
                | XOR_PI_CHAIN_MISMATCH_ERR
                | XOR_NM_PI_ENTRY_MISMATCH_ERR);
#endif
#ifdef SUPPORT_DPP
        tmp |=  XOR_DP_PERR_IRQ;
#endif
#ifdef SUPPORT_INTL_PARITY
        tmp |=  XOR_INT_MEM_PERR_IRQ;
#endif
#ifdef XOR_ATT_PERIPHERALS
#ifdef SUPPORT_I2C
        tmp |=  XOR_I2C_IRQ;
#endif
#ifdef SUPPORT_ACTIVE_CABLE
        tmp |=  XOR_GPIO_IRQ;
#endif
#endif
        xor_core->xor_int_mask= tmp;
        MV_REG_WRITE_DWORD(mmio, XOR_IRQ_MASK, tmp);
#ifdef SUPPORT_DPP
       xor_dpp_enable(mmio,MV_TRUE, MV_FALSE, MV_TRUE);
#endif
#ifdef SUPPORT_INTL_PARITY
        xor_intl_parity_enable(mmio,MV_TRUE, MV_TRUE);
#endif
#ifdef XOR_LOAD_BALANCE
        xor_core->xor_core_data_count = 0;
#endif
}

MV_U32
xor_prepare_hwprd(MV_PVOID core_p, MV_PVOID xor_req_p, MV_PVOID src_p, MV_PVOID *sg)
{
        core_extension  *core = (core_extension *)core_p;
        xor_context     *xor_ctx;
        hw_buf_wrapper  *sg_buffer, hbw;
        MV_U32 total;
		sgd_tbl_t * src = (sgd_tbl_t *)src_p;

        xor_ctx = ((MV_XOR_Request *)xor_req_p)->Context[MODULE_CORE];

        sg_buffer = get_sg_buf(&core->lib_rsrc);
        if (sg_buffer == NULL) goto xor_prepare_hwprd_error;
        sg_buffer->next = xor_ctx->sg_wrapper;
        xor_ctx->sg_wrapper = sg_buffer;
        *sg = sg_buffer;
		
#if defined(HAVE_HW_COMPLIANT_SG) && defined(_OS_LINUX)
		if (src->Flag & SGT_FLAG_PRDT_HW_COMPLIANT) {
			if (sg_buffer != NULL) {
				free_sg_buf(&core->lib_rsrc, sg_buffer);
				xor_ctx->sg_wrapper = NULL;
			}
			sg_buffer = &hbw;
			sg_buffer->phy.parts.low = src->prdt_bus_addr.parts.low;
			sg_buffer->phy.parts.high = src->prdt_bus_addr.parts.high;
			
			total = src->Valid_Entry_Count;
		}else
			total = core_prepare_hwprd(core, src_p, sg_buffer->vir);
#else
        total = core_prepare_hwprd(core, src_p, sg_buffer->vir);
#endif
        MV_ASSERT(total != 0);
        MV_ASSERT(total <= CORE_MAX_XOR_PRD_ENTRY);
	return total;

xor_prepare_hwprd_error:
        xor_sg_table_release(core, (MV_XOR_Request *)xor_req_p);
        *sg = NULL;
        return 0;
}

#ifdef CORE_USE_NEW_XOR
MV_U8
xor_fill_table(
    core_extension *core,
    MV_XOR_Request *xor_req,
    xor_cmd_table *cmd_table,
    MV_U8 buffer_count,
    MV_U8 source_count,
    MV_U8 target_count,
    MV_U8 s_start_index,
    MV_U8 t_start_index)
{
    MV_U32 consumed;
    hw_buf_wrapper *sg_buffer;
    MV_SG_Table *sg_tbl_ptr;
    MV_U8 total_count, t_start_op_entry;
    MV_U8 i, base_offset, is_multiple_gf;
    MV_U8 op_code_list[3], op_code;

    total_count = buffer_count + source_count + target_count;
    t_start_op_entry = buffer_count + source_count;
    MV_ZeroMemory(cmd_table, 6 * sizeof(MV_U32));
    cmd_table->total_byte_count = xor_req->src_sg_tbl_ptr[0].Byte_Count;
    cmd_table->valid_entry = total_count;

    for (i = 0; i < total_count; i++) {
        op_code = OP_NO_ACTION;
        op_code_list[0] = OP_NO_ACTION;
        op_code_list[1] = OP_NO_ACTION;
        op_code_list[2] = OP_NO_ACTION;

        if (i < t_start_op_entry) {

            /* fill source entry */
            if (i < buffer_count) {

                /* the result of prior table */
                base_offset = i;
                op_code = OP_DIRECT_FILL;
                is_multiple_gf = MV_FALSE;
            }
            else {
                base_offset = i - buffer_count;
                is_multiple_gf = MV_TRUE;
                if (i == 0) {

                    /* 1st table */
                    op_code = OP_GF_ENABLE;
                }
                else {

                    /* 2nd..nth table */
                    op_code = OP_GF_XOR_ENABLE;
                }
            }
        }
        else {

            /* fill target entry */
            base_offset = i - t_start_op_entry;
            op_code = OP_WRITE_ENABLE;
            is_multiple_gf = MV_FALSE;
        }

        if (is_multiple_gf) {
            MV_U8 max_gf = target_count;
            sg_tbl_ptr = &xor_req->src_sg_tbl_ptr[base_offset + s_start_index];
            while (max_gf-- > 0) {
                op_code_list[max_gf] = op_code;
            }
        }
        else {
            sg_tbl_ptr = &xor_req->tgt_sg_tbl_ptr[base_offset + t_start_index];
            op_code_list[base_offset] = op_code;
        }

        consumed = xor_prepare_hwprd(
                core,
                xor_req,
                sg_tbl_ptr,
                (MV_PVOID) (&sg_buffer));

        if (consumed == 0) {
            return MV_FALSE;

        }

#if defined(RAID6_MULTIPLE_PARITY)
        xor_fill_prd_entry(
            &cmd_table->xor_prd[i],
            xor_req,
            sg_buffer,
            consumed,
            op_code_list[0],
            op_code_list[1],
            op_code_list[2],
            s_start_index + base_offset,
            t_start_index,
            0,
            0,
            0,
            target_count);
#else /* RAID6_MULTIPLE_PARITY */
        xor_fill_prd_entry(
            &cmd_table->xor_prd[i],
            xor_req,
            sg_buffer,
            consumed,
            op_code_list[0],
            s_start_index + base_offset,
            t_start_index,
            0,
            target_count);
#endif /* RAID6_MULTIPLE_PARITY */
    }

    return MV_TRUE;
}
#endif /* CORE_USE_NEW_XOR */

/*******************************************************************************
*                                                                              *
* XOR Core Logic                                                               *
*                                                                              *
*******************************************************************************/

#ifdef CORE_USE_NEW_XOR

void
xor_write_command(MV_PVOID core_p, MV_PVOID xor_p, PMV_XOR_Request xor_req)
{
    core_extension *core = (core_extension *) core_p;
    xor_engine *xor_core = (xor_engine *) xor_p;
    xor_cmd_table *cmd_table = NULL;
    xor_context *context;
    xor_delv_q_entry delv_q_entry;

    MV_U8 target_count, source_count, buffer_count;
    MV_U8 table_width, table_depth;
    MV_U8 i, k, ret;
    MV_U8 written_s_count, written_t_count;

    context = (xor_context *) xor_req->Context[MODULE_CORE];

    xor_get_width_depth(xor_req, &table_width, &table_depth);

    //CORE_DPRINT(("##slot[%x] table_width[%x] table_depth[%x]\n", context->slot_num,table_width, table_depth));
    written_t_count = 0;

    for (i = 0; i < table_width; i++) {
        written_s_count = 0;
        target_count = MV_MIN(
                xor_req->Target_SG_Table_Count - written_t_count,
                CORE_MAX_XOR_TARGET_COUNT);

        for (k = 0; k < table_depth; k++) {
            cmd_table = context->xor_tbl;

            /* calculate buffer, source and target number for this table */
            if (k != 0) {
                buffer_count = target_count;
            }
            else {
                buffer_count = 0;
            }

            source_count = MV_MIN(
                    xor_req->Source_SG_Table_Count - written_s_count,
                    CORE_MAX_XOR_CMD_ENTRY - (target_count + buffer_count));

            ret = xor_fill_table(
                    core,
                    xor_req,
                    cmd_table,
                    buffer_count,
                    source_count,
                    target_count,
                    written_s_count,
                    written_t_count);

            if (!ret) {
                xor_sg_table_release(core, xor_req);
                List_Add(&xor_req->Queue_Pointer, &xor_core->xor_waiting_list);
                return;
            }

            written_s_count += source_count;

            context->finished = MV_TRUE;
            context = context->next;

            //check whether chain xor table
            if ((i != table_width - 1) || (k != table_depth - 1)) {
                MV_DASSERT(context);
		  cmd_table->next_tbl_addr_lo = context->xor_tbl_dma.parts.low;
            }
        }

        written_t_count += target_count;
    }

    ((MV_PU32) cmd_table)[1] |= XOR_ENABLE;
    context = (xor_context *) xor_req->Context[MODULE_CORE];
    context->finished = MV_FALSE;
    xor_core->xor_running_req[context->slot_num] = xor_req;
    xor_core->xor_core_req_count++;
    xor_prepare_delv_q_entry(context->slot_num, &delv_q_entry,context->xor_tbl_dma);
    xor_write_delv_q_entry(xor_core, &delv_q_entry);

#ifdef XOR_LOAD_BALANCE
    xor_load_balance_increment(xor_req, xor_core);
#endif

    /* Request is sent to the hardware and not finished yet. */
    xor_req->Request_Status = XOR_STATUS_SUCCESS;
    return;
}

#else /* CORE_USE_NEW_XOR */

void
xor_write_command(MV_PVOID core_p, MV_PVOID xor_p, PMV_XOR_Request xor_req)
{
	core_extension          *core = (core_extension *)core_p;
	xor_engine              *xor_core = (xor_engine *)xor_p;
        xor_cmd_table           *cmd_table;
        xor_context             *context;
        hw_buf_wrapper          *sg_buffer;
        xor_delv_q_entry        delv_q_entry;

        MV_U32 consumed;
        MV_U8 table_count, tgt_tbl_count, target_count, source_count;
        MV_U8 i, j;
        MV_U8 num_s_entry, num_t_entry;
        MV_U8 written_t_entry, written_s_entry, written_t_count;
        MV_U8 total_entry;
        MV_U32 t_index;
        MV_U32 byte_count;
        MV_U8 ret;
        MV_U8 op_code;

	context = (xor_context *)xor_req->Context[MODULE_CORE];

        tgt_tbl_count = (xor_req->Target_SG_Table_Count
                + CORE_MAX_XOR_TARGET_COUNT - 1) / CORE_MAX_XOR_TARGET_COUNT;

        table_count = ((xor_req->Source_SG_Table_Count +
                MV_MIN(xor_req->Target_SG_Table_Count, CORE_MAX_XOR_TARGET_COUNT) +
                CORE_MAX_XOR_CMD_ENTRY - 1) / CORE_MAX_XOR_CMD_ENTRY) * tgt_tbl_count;

/*
  Note on XOR Command Table byte count

  The byte count for the entire XOR request should be the same and
  sometimes the Byte_Count field in the target SG tables are not filled
  in. The total_byte_count will use this value.

  In the event that the XOR Command Table needs to be split and the
  second table only has target SG tables, using the Byte_Count field in
  the target SG table will fill in a value of 0 into the XOR Command
  Table and not the actual byte count. This will cause the XOR engine
  to stall.
*/
        byte_count = xor_req->src_sg_tbl_ptr[0].Byte_Count;
        written_t_count = 0;
        written_t_entry = 0;

        for (i = 0; i < table_count;) {
                written_t_count += written_t_entry;
                written_s_entry = 0;
                written_t_entry = 0;

                target_count = MV_MIN(xor_req->Target_SG_Table_Count -
                        written_t_count, CORE_MAX_XOR_TARGET_COUNT);
                total_entry = xor_req->Source_SG_Table_Count + target_count;

                while (written_t_entry + written_s_entry < total_entry) {

                        cmd_table = context->xor_tbl;
                        MV_ZeroMemory(cmd_table, 6 * sizeof(MV_U32));
                        if (i == table_count - 1)
                                ((MV_PU32)cmd_table)[1] |= XOR_ENABLE;

                        num_s_entry = MV_MIN(xor_req->Source_SG_Table_Count
                                                - written_s_entry,
                                                CORE_MAX_XOR_CMD_ENTRY);

                        /* xor_prd 0 .. n-1 */
                        for (j = 0; j < num_s_entry; j++) {
                                consumed = xor_prepare_hwprd(core,
                                        xor_req,
                                        &xor_req->src_sg_tbl_ptr
                                                [j + written_s_entry],
                                        (MV_PVOID)(&sg_buffer));
                                if (consumed == 0) {
                                        List_Add(&xor_req->Queue_Pointer,
                                                &xor_core->xor_waiting_list);
                                        return;
                                }

                                if (j + written_s_entry == 0)
                                        op_code = OP_GF_ENABLE;
                                else
                                        op_code = OP_GF_XOR_ENABLE;

#if defined(RAID6_MULTIPLE_PARITY)
                                switch (target_count) {
                                case 1:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[j],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                op_code,
                                                OP_NO_ACTION,
                                                OP_NO_ACTION,
                                                j + written_s_entry,
                                                written_t_count,
                                                0, 0, 0,
                                                target_count);
                                        break;
                                case 2:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[j],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                op_code,
                                                op_code,
                                                OP_NO_ACTION,
                                                j + written_s_entry,
                                                written_t_count,
                                                0, 0, 0,
                                                target_count);
                                        break;
                                case 3:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[j],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                op_code,
                                                op_code,
                                                op_code,
                                                j + written_s_entry,
                                                written_t_count,
                                                0, 0, 0,
                                                target_count);
                                        break;
                                default:
                                        MV_DASSERT(MV_FALSE);
                                        break;
                                }
#else /* RAID6_MULTIPLE_PARITY */
                        xor_fill_prd_entry(
                                &cmd_table->xor_prd[j],
                                xor_req,
                                sg_buffer,
                                consumed,
                                op_code,
                                j + written_s_entry,
                                written_t_count,
                                0,
                                target_count);
#endif /* RAID6_MULTIPLE_PARITY */
                        }

                        written_s_entry += num_s_entry;
                        num_t_entry = MV_MIN(target_count - written_t_entry,
                                        CORE_MAX_XOR_CMD_ENTRY - num_s_entry);

                        /* xor_prd n..n+m-1(1 or 2) */
                        for(j = 0; j < num_t_entry; j++) {
                                t_index = j + num_s_entry;

                                consumed = xor_prepare_hwprd(core,
                                                xor_req,
                                                &xor_req->tgt_sg_tbl_ptr
                                                        [j + written_t_entry +
                                                        written_t_count],
                                                (MV_PVOID)(&sg_buffer));
                                if (consumed == 0) {
                                        List_Add(&xor_req->Queue_Pointer,
                                                &xor_core->xor_waiting_list);
                                        return;
                                }

#if defined(RAID6_MULTIPLE_PARITY)
                                switch (j + written_t_entry) {
                                case 0:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[t_index],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                OP_WRITE_ENABLE,
                                                OP_NO_ACTION,
                                                OP_NO_ACTION,
                                                0, 0, 0, 0, 0,
                                                target_count);
                                        break;
                                case 1:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[t_index],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                OP_NO_ACTION,
                                                OP_WRITE_ENABLE,
                                                OP_NO_ACTION,
                                                0, 0, 0, 0, 0,
                                                target_count);
                                        break;
                                case 2:
                                        xor_fill_prd_entry(
                                                &cmd_table->xor_prd[t_index],
                                                xor_req,
                                                sg_buffer,
                                                consumed,
                                                OP_NO_ACTION,
                                                OP_NO_ACTION,
                                                OP_WRITE_ENABLE,
                                                0, 0, 0, 0, 0,
                                                target_count);
                                        break;
                                default:
                                        MV_DASSERT(MV_FALSE);
                                        break;
                                }
#else /* RAID6_MULTIPLE_PARITY */
                        xor_fill_prd_entry(
                                &cmd_table->xor_prd[t_index],
                                xor_req,
                                sg_buffer,
                                consumed,
                                OP_WRITE_ENABLE,
                                0, 0, 0,
                                target_count);
#endif /* RAID6_MULTIPLE_PARITY */
                        }

                        written_t_entry = written_t_entry + num_t_entry;
                        cmd_table->total_byte_count = byte_count;
                        cmd_table->valid_entry = num_s_entry + num_t_entry;

                        xor_core->xor_running_req[context->slot_num] = xor_req;
                        xor_core->xor_core_req_count++;
                        xor_prepare_delv_q_entry(context->slot_num, &delv_q_entry, context->xor_tbl_dma);
                        xor_write_delv_q_entry(xor_core,
                                &delv_q_entry);

                        context = context->next;
                        i++;
                }
        }

#ifdef XOR_LOAD_BALANCE
        xor_load_balance_increment(xor_req, xor_core);
#endif
        /* Request is sent to the hardware and not finished yet. */
        xor_req->Request_Status = XOR_STATUS_SUCCESS;
}

#endif /* CORE_USE_NEW_XOR */

/*
 xor_compare_command

 Handler for assembling and dispatching a compare request to the XOR engine.
 There should be no Target SG tables in the request.
*/
void
xor_compare_command(MV_PVOID core_p, MV_PVOID xor_p, PMV_XOR_Request xor_req)
{
        core_extension          *core = (core_extension *)core_p;
        xor_engine              *xor_core = (xor_engine *)xor_p;
        xor_cmd_table           *cmd_table;
        xor_context             *context;
        hw_buf_wrapper          *sg_buffer = NULL;
        xor_delv_q_entry        delv_q_entry;

        MV_U8                   i;
        MV_U32                  consumed;

	context = (xor_context *)xor_req->Context[MODULE_CORE];
        MV_DASSERT(context != NULL);
	MV_DASSERT(xor_req->Source_SG_Table_Count == 2);
	MV_DASSERT(xor_req->Target_SG_Table_Count == 0);

        cmd_table = context->xor_tbl;
        MV_ZeroMemory(cmd_table, 6 * sizeof(MV_U32));

        ((MV_PU32)cmd_table)[1] |= XOR_ENABLE;

        consumed = xor_prepare_hwprd(core,
                        xor_req,
                        &xor_req->Source_SG_Table_List[0],
                        (MV_PVOID)(&sg_buffer));

        if (consumed == 0) {
                List_Add(&xor_req->Queue_Pointer,
                        &xor_core->xor_waiting_list);
                return;
        }

        xor_fill_prd_entry(
                &cmd_table->xor_prd[0],
                xor_req,
                sg_buffer,
                consumed,
                OP_DIRECT_FILL,
#if defined RAID6_MULTIPLE_PARITY
                OP_NO_ACTION,
                OP_NO_ACTION,
#endif
                0, 0, 0,
#if defined RAID6_MULTIPLE_PARITY
                0, 0,
#endif
                0);

        for (i = 1; i < xor_req->Source_SG_Table_Count; i++) {
                consumed = xor_prepare_hwprd(core,
                                xor_req,
                                &xor_req->Source_SG_Table_List[i],
                                (MV_PVOID)(&sg_buffer));

                if (consumed == 0) {
                        List_Add(&xor_req->Queue_Pointer,
                                &xor_core->xor_waiting_list);
                        return;
                }

                xor_fill_prd_entry(
                        &cmd_table->xor_prd[i],
                        xor_req,
                        sg_buffer,
                        consumed,
                        OP_XOR_ENABLE,
#if defined RAID6_MULTIPLE_PARITY
                        OP_NO_ACTION,
                        OP_NO_ACTION,
#endif
                        0, 0, 0,
#if defined RAID6_MULTIPLE_PARITY
                        0, 0,
#endif
                        0);
        }

        /* to workaround hardware no-action check */
        cmd_table->xor_prd[xor_req->Source_SG_Table_Count].prd_entry_count = 1;

        /* Zero Check */
        cmd_table->xor_prd[xor_req->Source_SG_Table_Count].buffer_0_opcode
                = OP_ZERO_CHECK_ENABLE;

        /* add 1 for the zero check entry */
        cmd_table->total_byte_count
                = xor_req->Source_SG_Table_List[0].Byte_Count;
        cmd_table->valid_entry = xor_req->Source_SG_Table_Count + 1;

        xor_core->xor_running_req[context->slot_num] = xor_req;
#ifdef XOR_LOAD_BALANCE
        xor_load_balance_increment(xor_req, xor_core);
#endif
        xor_core->xor_core_req_count++;

        xor_prepare_delv_q_entry(context->slot_num, &delv_q_entry, context->xor_tbl_dma);
        xor_write_delv_q_entry(xor_core, &delv_q_entry);

        /* Request is sent to the hardware and not finished yet. */
        xor_req->Request_Status = XOR_STATUS_SUCCESS;
}
#ifdef SUPPORT_FILL_MEMORY
void
xor_memset_command(MV_PVOID core_p, MV_PVOID xor_p, PMV_XOR_Request xor_req)
{
	core_extension          *core = (core_extension *)core_p;
        xor_engine              *xor_core = (xor_engine *)xor_p;
        xor_cmd_table           *cmd_table = NULL;
        xor_context             *context = NULL;
        hw_buf_wrapper          *sg_buffer = NULL;
        xor_delv_q_entry        delv_q_entry;

        MV_U16                  slot_num;
        MV_U32                  consumed;

	core = (core_extension *)core_p;
	context = (xor_context *)xor_req->Context[MODULE_CORE];
        slot_num = context->slot_num;

        cmd_table = context->xor_tbl;
        MV_ZeroMemory(cmd_table, 6 * sizeof(MV_U32));
        ((MV_PU32)cmd_table)[1] |= XOR_ENABLE;

        consumed = xor_prepare_hwprd(core,
                        xor_req,
                        &xor_req->Source_SG_Table_List[0],
                        (MV_PVOID)(&sg_buffer));

        if (consumed == 0) {
                List_Add(&xor_req->Queue_Pointer,
                        &xor_core->xor_waiting_list);
                return;
        }

        xor_fill_prd_entry(
                &cmd_table->xor_prd[0],
                xor_req,
                sg_buffer,
                consumed,
                OP_NO_ACTION,
#if defined RAID6_MULTIPLE_PARITY
                OP_NO_ACTION,
                OP_NO_ACTION,
#endif
                0, 0, 0,
#if defined RAID6_MULTIPLE_PARITY
                0, 0,
#endif
                0);
        cmd_table->total_byte_count
                = xor_req->Source_SG_Table_List[0].Byte_Count;
        cmd_table->valid_entry = 1;

        xor_core->xor_running_req[slot_num] = xor_req;
#ifdef XOR_LOAD_BALANCE
        xor_load_balance_increment(xor_req, xor_core);
#endif
        xor_core->xor_core_req_count++;
        xor_prepare_delv_q_entry(context->slot_num, &delv_q_entry,context->xor_tbl_dma);
        xor_write_delv_q_entry(xor_core, &delv_q_entry);

        /* Request is sent to the hardware and not finished yet. */
        xor_req->Request_Status = XOR_STATUS_SUCCESS;
}
#endif

/*
 xor_dma_command

 Handler for assembling and dispatching a DMA request to the XOR engine.
 There should be 1 Source and 1 Target SG table in the request.
*/
void
xor_dma_command(MV_PVOID core_p, MV_PVOID xor_p, PMV_XOR_Request xor_req)
{
	core_extension          *core = (core_extension *)core_p;
        xor_engine              *xor_core = (xor_engine *)xor_p;
        xor_cmd_table           *cmd_table = NULL;
        xor_context             *context = NULL;
        hw_buf_wrapper          *sg_buffer = NULL;
        xor_delv_q_entry        delv_q_entry;
        MV_U32                  consumed;

	core = (core_extension *)core_p;
	context = (xor_context *)xor_req->Context[MODULE_CORE];

        cmd_table = context->xor_tbl;
        MV_ZeroMemory(cmd_table, 6 * sizeof(MV_U32));
        ((MV_PU32)cmd_table)[1] |= XOR_ENABLE;

        consumed = xor_prepare_hwprd(core,
                        xor_req,
                        &xor_req->Source_SG_Table_List[0],
                        (MV_PVOID)(&sg_buffer));

        if (consumed == 0) {
                List_Add(&xor_req->Queue_Pointer,
                        &xor_core->xor_waiting_list);
                return;
        }

        xor_fill_prd_entry(
                &cmd_table->xor_prd[0],
                xor_req,
                sg_buffer,
                consumed,
                OP_DIRECT_FILL,
#if defined RAID6_MULTIPLE_PARITY
                OP_NO_ACTION,
                OP_NO_ACTION,
#endif
                0, 0, 0,
#if defined RAID6_MULTIPLE_PARITY
                0, 0,
#endif
                0);

        consumed = xor_prepare_hwprd(core,
                        xor_req,
                        &xor_req->Target_SG_Table_List[0],
                        (MV_PVOID)(&sg_buffer));

        if (consumed == 0) {
                List_Add(&xor_req->Queue_Pointer,
                        &xor_core->xor_waiting_list);
                return;
        }

        xor_fill_prd_entry(
                &cmd_table->xor_prd[1],
                xor_req,
                sg_buffer,
                consumed,
                OP_WRITE_ENABLE,
#if defined RAID6_MULTIPLE_PARITY
                OP_NO_ACTION,
                OP_NO_ACTION,
#endif
                0, 0, 0,
#if defined RAID6_MULTIPLE_PARITY
                0, 0,
#endif
                0);

        cmd_table->total_byte_count
                = xor_req->Source_SG_Table_List[0].Byte_Count;
        cmd_table->valid_entry = 2;

        xor_core->xor_running_req[context->slot_num] = xor_req;
#ifdef XOR_LOAD_BALANCE
        xor_load_balance_increment(xor_req, xor_core);
#endif
        xor_core->xor_core_req_count++;
        xor_prepare_delv_q_entry(context->slot_num, &delv_q_entry,context->xor_tbl_dma);
        xor_write_delv_q_entry(xor_core, &delv_q_entry);

        /* Request is sent to the hardware and not finished yet. */
        xor_req->Request_Status = XOR_STATUS_SUCCESS;
}

/*
 xor_handle_int_error

 Handles any error encounted during the processing of an XOR request.
*/
void
xor_handle_int_error(xor_engine *xor_core, PMV_XOR_Request xor_req)
{
	xor_context *xor_ctx = (xor_context *)xor_req->Context[MODULE_CORE];

	if (xor_ctx->xor_tbl->zero_chk_err) {
		CORE_DPRINT(("XOR Interrupt Status : 0x%x\n", xor_core->xor_int_status));
		xor_req->Request_Status = XOR_STATUS_ERROR;
        }

}

MV_U32 xor_has_cmpl_queue( MV_PVOID This, MV_U16 core_id )
{
        core_extension          *core = (core_extension *) This;
        xor_engine              *xor_core;
        xor_cmpl_q_entry        *cmpl_wp;
        MV_U16 wp_mem;

        xor_core = &core->xors.engine[core_id];
#ifdef ATHENA_XOR_CMPL_Q_SHADOW_ENABLE
        cmpl_wp = (xor_cmpl_q_entry *)xor_core->xor_shadow_cmpl_q;
        wp_mem = (MV_U16) MV_LE32_TO_CPU((*(MV_U32 *)cmpl_wp))&XOR_CMPL_QUEUE_SIZE_MASK;
#else
        wp_mem =  (MV_U16) (MV_REG_READ_DWORD(xor_core->xor_mmio_base,XOR_CMPL_Q_WR_PTR) & XOR_CMPL_QUEUE_SIZE_MASK);
#endif
	if ( wp_mem != xor_core->xor_last_cmpl_q )
		return 1;

	return 0;
}
/*
 xor_dump_error_register

 dump error register
*/
void
xor_dump_error_inf_register(xor_engine *xor_core)
{
    MV_LPVOID       mmio = xor_core->xor_mmio_base;
    MV_U32 err_inf, err_table_addr,err_table_addr_hi;
    MV_U32 err_prd_addr,err_prd_addr_hi;
    err_inf = MV_REG_READ_DWORD(mmio, XOR_ERR_INFO);
    err_table_addr = MV_REG_READ_DWORD(mmio, XOR_ERR_TBL_ADDR);
    err_table_addr_hi = MV_REG_READ_DWORD(mmio, XOR_ERR_TBL_ADDR_HI);
    err_prd_addr = MV_REG_READ_DWORD(mmio, XOR_ERR_PRD_ADDR);
    err_prd_addr_hi = MV_REG_READ_DWORD(mmio, XOR_ERR_PRD_ADDR_HI);
    CORE_EH_PRINT(("xor error info 0x%x \n", err_inf));
    CORE_EH_PRINT(("xor error table addr 0x%x hi 0x%x \n", err_table_addr, err_table_addr_hi));
    CORE_EH_PRINT(("xor error prd addr 0x%x hi 0x%x \n", err_prd_addr, err_prd_addr_hi));
}
void
xor_dump_dif_error_register(xor_engine *xor_core)
{
    MV_LPVOID       mmio = xor_core->xor_mmio_base;
    MV_U32 t10_err_off, t10_err_exp_LBRT,t10_err_act_LBRT;
    MV_U32 t10_err_exp_LBAT_LBG,t10_err_act_LBAT_LBG;
    t10_err_off = MV_REG_READ_DWORD(mmio, XOR_T10_ERR_OFF);
    t10_err_exp_LBRT = MV_REG_READ_DWORD(mmio, XOR_T10_EXPECT_LBRT);
    t10_err_exp_LBAT_LBG = MV_REG_READ_DWORD(mmio, XOR_T10_EXPECT_LBAT_LBG);
    t10_err_act_LBRT = MV_REG_READ_DWORD(mmio, XOR_T10_ACTUAL_LBRT);
    t10_err_act_LBAT_LBG = MV_REG_READ_DWORD(mmio, XOR_T10_ACTUAL_LBAT_LBG);
    CORE_EH_PRINT(("xor T10 error off 0x%x \n", t10_err_off));
    CORE_EH_PRINT(("xor T10 expect LBRT 0x%x LBAT LBG 0x%x \n", t10_err_exp_LBRT, t10_err_exp_LBAT_LBG));
    CORE_EH_PRINT(("xor T10 actual LBRT 0x%x LBAT LBG 0x%x \n", t10_err_act_LBRT, t10_err_act_LBAT_LBG));
}
void
xor_dump_error_register(xor_engine *xor_core,  PMV_XOR_Request xor_req)
{
    MV_LPVOID       mmio = xor_core->xor_mmio_base;
    MV_U32		tmp;
    tmp = MV_REG_READ_DWORD(mmio, XOR_IRQ_STAT);
    CORE_EH_PRINT(("xor irq status 0x%x \n", tmp));
    if(tmp & XOR_OVERRUN_IRQ){
        CORE_EH_PRINT(("xor req %p over run.\n", xor_req));
    }
    if(tmp & XOR_UNDERRUN_IRQ){
        CORE_EH_PRINT(("xor req %p under run.\n", xor_req));
    }

    if(tmp & (XOR_B0_ZERO_RSLTCHK_ERR_IRQ|XOR_B1_ZERO_RSLTCHK_ERR_IRQ|XOR_B2_ZERO_RSLTCHK_ERR_IRQ)){
        CORE_EH_PRINT(("xor req %p B1/2/3 zero check error .\n", xor_req));

    }
    if((tmp & XOR_REF_CHK_ERR|XOR_APP_CHK_ERR|XOR_GRD_CHK_ERR|XOR_T10_CNTRL_ERR|XOR_PI_CHAIN_MISMATCH_ERR|XOR_NM_PI_ENTRY_MISMATCH_ERR)){
        CORE_EH_PRINT(("xor req %p T10 pi error.\n", xor_req));
        xor_dump_dif_error_register(xor_core);
    }
    xor_dump_error_inf_register(xor_core);
}

extern void xor_complete_xor_request(
	core_extension *core,
	xor_engine *xor_core,
	PMV_XOR_Request xor_req
	);
/* MV_TRUE - completion queue process finished
 * MV_FALSE - need process ATTENTION */
MV_BOOLEAN xor_handle_int_core(MV_PVOID This, MV_U16 core_id)
{
        core_extension          *core = (core_extension *) This;
        xor_engine              *xor_core;
        MV_XOR_Request          *xor_req;
        xor_cmpl_q_entry        *cmpl_wp, *cmpl_q;
        xor_cmpl_q_entry        tmp_entry;
        MV_U16 wp_mem;
        MV_BOOLEAN att=MV_TRUE;
        MV_U16 slot_num;
        MV_U16 i;

        xor_core = &core->xors.engine[core_id];
#ifdef ATHENA_XOR_CMPL_Q_SHADOW_ENABLE
        cmpl_wp = (xor_cmpl_q_entry *)xor_core->xor_shadow_cmpl_q;
        wp_mem = (MV_U16) MV_LE32_TO_CPU((*(MV_U32 *)cmpl_wp))&XOR_CMPL_QUEUE_SIZE_MASK;
#else
        wp_mem =  (MV_U16) (MV_REG_READ_DWORD(xor_core->xor_mmio_base,XOR_CMPL_Q_WR_PTR) & XOR_CMPL_QUEUE_SIZE_MASK);
#endif
        cmpl_q = (xor_cmpl_q_entry *)xor_core->xor_cmpl_q;
        i = xor_core->xor_last_cmpl_q;

        xor_core->xor_last_cmpl_q = wp_mem;

        if (xor_core->xor_last_cmpl_q == XOR_MAX_CMPL_QUEUE_SIZE)
                return MV_FALSE;
        if(i == xor_core->xor_last_cmpl_q)
            return MV_FALSE;
        while (i != xor_core->xor_last_cmpl_q) {
                i++;
                if (i >= xor_core->xor_cmpl_q_size)
                        i = 0;

                tmp_entry = cmpl_q[i];
                slot_num = (MV_U16)tmp_entry.SLOT_NM;

                if (xor_core->xor_running_req[slot_num] == NULL) {
                        MV_ASSERT(MV_FALSE);
                        continue;
                }
                if(tmp_entry.ATT){
                    att = MV_FALSE;
                    continue;
                }
                xor_req = xor_core->xor_running_req[slot_num];

                if (tmp_entry.CMD_ERR) {
                        xor_handle_int_error(xor_core, xor_req);
                        xor_dump_error_register(xor_core, xor_req);
                } else if (tmp_entry.CMD_ABORT || tmp_entry.CMD_PAUSE || tmp_entry.CMD_SKIP) {
                        /* we didn't implement these features. */
                        MV_ASSERT(MV_FALSE);
                }

                if (tmp_entry.CMD_CMPL || tmp_entry.CMD_ERR) {
                        xor_core->xor_running_req[slot_num] = NULL;
                        xor_set_context_finished(xor_req, slot_num);
                }

                if (xor_is_request_finished(xor_req)) {
                        if (xor_req->Request_Status != XOR_STATUS_ERROR)
                                xor_req->Request_Status = XOR_STATUS_SUCCESS;
#if defined(SUPPORT_ROC)
                        xor_complete_xor_request(core, xor_core, xor_req);
                        xor_core->xor_core_req_count--;
#ifdef XOR_LOAD_BALANCE
                        xor_load_balance_decrement(xor_req, xor_core);
#endif
#else
                        List_AddTail(&xor_req->Queue_Pointer,
                                &xor_core->xor_cmpl_list);
#endif
                }
        }
        MV_REG_WRITE_DWORD(xor_core->xor_mmio_base,XOR_CMPL_Q_RD_PTR, xor_core->xor_last_cmpl_q);
        return att;
}
MV_BOOLEAN xor_check_queue_int(xor_engine *xor_core)
{
	MV_LPVOID       mmio = xor_core->xor_mmio_base;
	MV_U32		tmp;
	tmp = MV_REG_READ_DWORD(mmio, XOR_IRQ_STAT);
	MV_REG_WRITE_DWORD(mmio, XOR_IRQ_STAT, tmp);
	xor_core->xor_int_status |= tmp;
	if (xor_core->xor_int_status) {
		return MV_TRUE;
	}
	else {
		return MV_FALSE;
	}
}

MV_VOID xor_handle_att_int(MV_PVOID This, MV_U16 core_id)
{
    core_extension *core = (core_extension *)This;
    xor_engine              *xor_core= &core->xors.engine[core_id];
    MV_LPVOID       xor_mmio=xor_core->xor_mmio_base;
#ifdef SUPPORT_ACTIVE_CABLE
	MV_U32 reg_value;
#endif
	if (!xor_check_queue_int(xor_core)) {
		return;
	}
   
#ifdef SUPPORT_ACTIVE_CABLE
    	if (xor_core->xor_int_status & XOR_GPIO_IRQ) {
		reg_value = MV_REG_READ_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG);
		if (reg_value & CABLE_GPIO_BITS) {
			MV_REG_WRITE_DWORD(core->mmio_base, GPIO_INT_CAUSE_REG, 0xFF);
			core_handle_cable_gpio_int(core, reg_value);
		}
	}
#endif

#ifdef SUPPORT_I2C
	if (xor_core->xor_int_status & XOR_I2C_IRQ) {
		i2c_interrupt_service_routine(core);
	}
#endif /* SUPPORT_I2C */

#if defined(SUPPORT_DPP) || defined(SUPPORT_INTL_PARITY)
            if(xor_core->xor_int_status&(XOR_DP_PERR_IRQ |XOR_INT_MEM_PERR_IRQ)){
                MV_U32 reg,addr_lo,addr_hi;
#if defined(SUPPORT_DPP)
                if(xor_core->xor_int_status&XOR_DP_PERR_IRQ){
                    reg=MV_REG_READ_DWORD(xor_mmio, XOR_DP_PSTAT);
                    addr_lo=MV_REG_READ_DWORD(xor_mmio, XOR_DP_ERR_ADDR);
                    addr_hi=MV_REG_READ_DWORD(xor_mmio, XOR_DP_ERR_ADDR_HI);
                    MV_PRINT("xor%p XOR_DP_PSTAT : 0x%08x addr(0x%08x,0x%08x)\n", xor_core,reg,addr_hi,addr_lo);
                    MV_REG_WRITE_DWORD(xor_mmio, XOR_DP_PSTAT,reg);
                }
#endif
#if defined(SUPPORT_INTL_PARITY)
                if(xor_core->xor_int_status&XOR_INT_MEM_PERR_IRQ){
                    reg=MV_REG_READ_DWORD(xor_mmio, XOR_ERR_MEM_PRERR);
                    if(reg) 
                        MV_PRINT("XOR_ERR_MEM_PRERR : 0x%08x\n", reg);
                    MV_REG_WRITE_DWORD(xor_mmio, XOR_ERR_MEM_PRERR,reg);
                }
#endif
            }
#endif

#ifdef DEBUG_PCI_E_ERROR
	if (xor_core->xor_int_status & XOR_SYS_ERR_IRQ) {
		/* need to check SOC main irq */
		MV_U32 main_irq;
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
	xor_core->xor_int_status = 0;
}
/*
 xor_clear_int

 Clears XOR interrupt for the specified XOR core.
*/
void xor_clear_int(MV_PVOID This)
{
        core_extension  *core = (core_extension *) This;
	xor_engine	*xor;
        MV_LPVOID       mmio;
	MV_U32		tmp;

        MV_U16 i;
        for (i = 0; i < MV_MAX_XOR_ID; i++) {
		xor = &core->xors.engine[i];
		mmio = xor->xor_mmio_base;

		tmp = MV_REG_READ_DWORD(mmio, XOR_IRQ_STAT);
		MV_REG_WRITE_DWORD(mmio, XOR_IRQ_STAT, tmp);
		xor->xor_int_status = tmp;
#if defined(SUPPORT_DPP) || defined(SUPPORT_INTL_PARITY)
            if(tmp&(XOR_DP_PERR_IRQ |XOR_INT_MEM_PERR_IRQ)){
                MV_U32 reg,addr_lo,addr_hi;
#if defined(SUPPORT_DPP)
                if(tmp&XOR_DP_PERR_IRQ){
                    reg=MV_REG_READ_DWORD(mmio, XOR_DP_PSTAT);
                    addr_lo=MV_REG_READ_DWORD(mmio, XOR_DP_ERR_ADDR);
                    addr_hi=MV_REG_READ_DWORD(mmio, XOR_DP_ERR_ADDR_HI);
                    MV_PRINT("xor%d XOR_DP_PSTAT : 0x%08x addr(0x%08x,0x%08x)\n", i,reg,addr_hi,addr_lo);
                    MV_REG_WRITE_DWORD(mmio, XOR_DP_PSTAT,reg);
                }
#endif
#if defined(SUPPORT_INTL_PARITY)
                if(tmp&XOR_INT_MEM_PERR_IRQ){
                    reg=MV_REG_READ_DWORD(mmio, XOR_ERR_MEM_PRERR);
                    if(reg) 
                        MV_PRINT("XOR_ERR_MEM_PRERR : 0x%08x\n", reg);
                    MV_REG_WRITE_DWORD(mmio, XOR_ERR_MEM_PRERR,reg);
                }
#endif
                continue;
            }
#endif
                /* strong assertion to see whether driver hit any XOR abnormal error */
                MV_ASSERT( !(tmp
                            & (XOR_PAUSED_IRQ | XOR_OVERRUN_IRQ | XOR_UNDERRUN_IRQ |
                               XOR_DP_PERR_IRQ | XOR_INT_MEM_PERR_IRQ)) );
        }
}

MV_U8 xor_get_xor_core_id(MV_PVOID core_p, MV_PVOID xor_req_p)
{
#ifdef SUPPORT_ROC
        core_extension *core = (core_extension *)core_p;
        MV_XOR_Request *xor_req = (MV_XOR_Request *)xor_req_p;

        if (xor_req->Request_Type == XOR_REQUEST_DMA) {
                if (core->xors.engine[0].xor_core_req_count <= core->xors.engine[1].xor_core_req_count)
                        return MV_XOR_CORE_0;
                else
                        return MV_XOR_CORE_1;
        }

        return MV_XOR_CORE_2;
#else
        MV_XOR_Request *xor_req = (MV_XOR_Request *)xor_req_p;

        if (xor_req->Request_Type == XOR_REQUEST_DMA)
                return MV_DMA_CORE_ID;

        return MV_XOR_CORE_ID;
#endif
}

#endif /* HARDWARE_XOR */
