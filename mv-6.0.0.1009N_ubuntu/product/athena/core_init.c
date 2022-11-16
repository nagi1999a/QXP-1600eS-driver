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
#include "core_init.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_util.h"
#include "core_manager.h"
#include "core_cpu.h"
#include "core_sas.h"
#include "core_sata.h"
#include "core_error.h"
#include "core_expander.h"
#include "core_console.h"

extern void core_handle_init_queue(core_extension *core, MV_BOOLEAN single);
extern void core_handle_waiting_queue(core_extension *core);

MV_VOID io_chip_init_registers(pl_root *root);

void update_phy_info(pl_root *root, domain_phy *phy);
void update_port_phy_map(pl_root *root, domain_phy *phy);
MV_BOOLEAN set_port_vsr_data(pl_root *root, domain_phy *phy, MV_U32 offset, MV_U32 reg);
//void set_phy_tuning(pl_root *root, domain_phy *phy, PHY_TUNING phy_tuning)
void set_phy_tuning(pl_root *root, domain_phy *phy, PHY_TUNING phy_tuning_G1, PHY_TUNING phy_tuning_G2, PHY_TUNING phy_tuning_G3, PHY_TUNING phy_tuning_G4)
{
	core_extension *core = (core_extension *)root->core;
	MV_U32 tmp, setting_0 = 0, setting_1 = 0;
	MV_U8 i;

	for (i=0; i<4; i++) {
		/* loop 4 times, set Gen 1, Gen 2, Gen 3, Gen 4*/
		switch (i) {
		case 0:
			setting_0 = GENERATION_1_SETTING_0;
			break;
		case 1:
			setting_0 = GENERATION_2_SETTING_0;
			break;
		case 2:
			setting_0 = GENERATION_3_SETTING_0;
			break;
		case 3:
			setting_0 = GENERATION_4_SETTING_0;
			break;
		}

		/* Set:
		*
		*   Transmitter Emphasis Enable
		*   Transmitter Emphasis Amplitude
		*   Transmitter Amplitude Adjust
		*   Transmitter Amplitude
		*/	

		// switch phy setting page 0
		WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
		WRITE_PORT_VSR_DATA(root, phy, 0x00000000);

		WRITE_PORT_VSR_ADDR(root, phy, setting_0);
		tmp = READ_PORT_VSR_DATA(root, phy);
		// clear bits 11, 10:7, 6, 5:1 
		tmp &= 0x0000F001UL; //~0xFFEL;
		// Set the bits
		/*
		tmp |= ((phy_tuning.Trans_Emphasis_En << 11) |
		(phy_tuning.Trans_Emphasis_Amp << 7) |
		(phy_tuning.Trans_Amp_Adjust <<6) |
		(phy_tuning.Trans_Amp << 1)) ;
		WRITE_PORT_VSR_DATA(root, phy, tmp);*/

		if (i == 0){
			tmp |= ((phy_tuning_G1.Trans_Emphasis_En << 11) |
			(phy_tuning_G1.Trans_Emphasis_Amp << 7) |
			(phy_tuning_G1.Trans_Amp_Adjust <<6) |
			(phy_tuning_G1.Trans_Amp << 1)) ;
		}else if (i == 1){
		tmp |= ((phy_tuning_G2.Trans_Emphasis_En << 11) |
			(phy_tuning_G2.Trans_Emphasis_Amp << 7) |
			(phy_tuning_G2.Trans_Amp_Adjust <<6) |
			(phy_tuning_G2.Trans_Amp << 1)) ;
		}else if (i == 2){
		tmp |= ((phy_tuning_G3.Trans_Emphasis_En << 11) |
			(phy_tuning_G3.Trans_Emphasis_Amp << 7) |
			(phy_tuning_G3.Trans_Amp_Adjust <<6) |
			(phy_tuning_G3.Trans_Amp << 1)) ;
		}else if (i == 3){
		tmp |= ((phy_tuning_G4.Trans_Emphasis_En << 11) |
			(phy_tuning_G4.Trans_Emphasis_Amp << 7) |
			(phy_tuning_G4.Trans_Amp_Adjust <<6) |
			(phy_tuning_G4.Trans_Amp << 1)) ;
		}
		//WRITE_PORT_VSR_DATA(root, phy, tmp);
		set_port_vsr_data(root, phy, setting_0, tmp);
		//tmp = READ_PORT_VSR_DATA(root, phy);
//		MV_PRINT("phy %d, R 0x%x value 0x%x\n", (root->base_phy_num+phy->asic_id), setting_0, tmp);
	}
}

MV_VOID set_phy_ffe_tuning(pl_root *root, domain_phy *phy, FFE_CONTROL ffe)
{
	core_extension *core = (core_extension *)root->core;
	MV_U32 tmp, offset;

	// switch phy setting page 0
 	WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
	WRITE_PORT_VSR_DATA(root, phy, 0x00000000);
	offset = 0x00000106;
	WRITE_PORT_VSR_ADDR(root, phy, offset);
	tmp = READ_PORT_VSR_DATA(root, phy);
	//clear bit 6:4, 3:0
	tmp &= 0x0000FF80;//0xFFFFFF80L;
	tmp |= ((ffe.FFE_Resistor_Select << 4) | ffe.FFE_Capacitor_Select);
	tmp |= 0x600;
	//WRITE_PORT_VSR_DATA(root, phy, tmp);
	set_port_vsr_data(root, phy, offset, tmp);
}

/*
Address            data
E4020260          0000017F          
E4020264          00000000
E4020260          00000106          
E4020264          000088BE
E4020260          00000107          
E4020264          0000FC26
E4020260          00000142          
E4020264          0000B0D4
E4020260          00000143          
E4020264          000034D4
E4020260          00000144          
E4020264          000030D4
E4020260          00000145          
E4020264          00003140
E4020260          0000014A          
E4020264          00000400
E4020260          0000014D          
E4020264          00002050
E4020260          00000161          
E4020264          00001017
E4020260          0000010D          
E4020264          0000C9F2
E4020260          0000010E          
E4020264          00000800
E4020260          0000010F          
E4020264          0000AA72
E4020260          00000110          
E4020264          00000BC0
E4020260          00000111          
E4020264          00000AF2
E4020260          00000112          
E4020264          00001789
E4020260          00000113          
E4020264          000009F2
E4020260          00000114          
E4020264          00001692
E4020260          00000126          
E4020264          00000133
E4020260          00000123          
E4020264          00000004
E4020260          00000101          
E4020264          0000FC24
E4020260          0000015A          
E4020264          0000E028
E4020260          0000017F          
E4020264          00000000
E4020260          00000125          
E4020264          00000FFF
E4020260          00000106          
E4020264          000088BE
E4020260          00000162          
E4020264          00002A60
E4020260          0000017F          
E4020264          00000001
E4020260          00000103          
E4020264          00000200
E4020260          0000010A          
E4020264          00002B40
E4020260          00000121          
E4020264          00004A47
E4020260          00000148          
E4020264          00001658
E4020260          00000158          
E4020264          00001903
E4020260          0000011E          
E4020264          000025F3
E4020260          0000017F          
E4020264          00000000
E4020260          00000060          
E4020264          00000002
E4020260          00000008          
E4020264          C0087EFf
E4020260          00000102          
E4020264          000050CF
E4020260          00000123          
E4020264          00000004
E4020260          00000060          
E4020264          00000002
*/
MV_VOID set_12G_sas_setting(pl_root *root, domain_phy *phy, MV_BOOLEAN phy_reset, MV_BOOLEAN phy_disable)
{
    MV_U32 tmp;
#ifdef SUPPORT_DIRECT_SATA_SSU
	core_extension *core = root->core;
#endif

    tmp = 0xC0087EFf; //tmp = 0xC0007EFF;   //set down-spreading as default //       tmp = 0xC0087EFF;
    if(phy_disable)
        tmp|= VSR_DSBL_PHY;
    if(phy_reset)
        tmp|=VSR_PHY_RESET;
#ifdef SUPPORT_DIRECT_SATA_SSU
	if (core->sata_ssu_time) {
		/* enable sata spin up hold */
		tmp |= VSR_SATA_SPIN_UP_SUPPORT;
	}
#endif
    if(!phy){
// phy setting page 0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000017F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000000);  // page=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000101);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000FD24);  // PHY_MODE=1, REF_FREF_SEL[4:0]=4
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000102);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000074CF);  // USE_MAX_PLL_RATE=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000104);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00008A82); //WRITE_ALL_PORT_VSR_DATA(root, 0x00008A89);  //ssc/amp 2250ppm //WRITE_ALL_PORT_VSR_DATA(root, 0x00008A8C);  // SSC_DSPREAD_RX=0, debug purpose
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000106);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000863E);  // AC_TERM_EN=1, SQ_THRESH_IN[4:0]=6, FFE_SETTING_FORCE=0, FFE_RES_SEL=3, FFE_CAP_SEL=E
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000107);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00007D26);  // DFE_RES_FORCE=0, DFE_RES=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010D);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000C9F2);  // G1_setting_0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010E);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000800);  // G1_setting_1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000AA72);  // G2_setting_0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000110);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000B80);  // G2_setting_1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000111);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000AF2);  // G3_setting_0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000112);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001788);  // G3_setting_1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000113);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000AF2);  // G4_setting_0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000114);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001651);  // G4_setting_1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000122);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001026);  // password=26
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000123);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000074);  // SEL_BITS=2
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000125);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000FFF);  // DET_BYPASS=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000126);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000133);  // PHY_GEN_TX=3, PHY_GEN_RX=3, not necessary       
        /* new setting */
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000013D);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000009C0);  // G1_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000013E);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000009C0);  // G2_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000013F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000009C0);  // G3_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000140);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000009C0);  // G4_TX_SSC_AMP[6:0] spread spectrum clock Amplitude
        /* end */
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000142);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000243B);  // vdd_cal_reg0
//        WRITE_ALL_PORT_VSR_DATA(root, 0x0000303B);  // vdd_cal_reg0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000143);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000E3B);  // vdd_cal_reg1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000144);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000D3B);  // vdd_cal_reg2
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000145);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000398);  // vdd_cal_reg3
//        WRITE_ALL_PORT_VSR_DATA(root, 0x00000390);  // vdd_cal_reg3
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000014A);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000400);  // ck100k_in_pll_freq_sel[1:0]=2
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000014D);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00002050);  // sellv_rxintp[2:0]=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000158);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000810A);  // txdcc_cal_stop_sel=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000015A);
//        WRITE_ALL_PORT_VSR_DATA(root, 0x0000D828);  // cal_rxclkalign90_ext_en=1, cal_os_ph_ext[6:0]=5A
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000DC28);  // cal_rxclkalign90_ext_en=1, cal_os_ph_ext[6:0]=5A
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000015C);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00006E4A);  // process_cal_subss, process_cal_ss2tt, process_cal_tt2ff
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000161);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001077);  // dtl_clamping_en=0, dtl_clamping_sel=7
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000162);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00002A60);  // no_seq=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000164);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_foffset_extraction_en=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000016A);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00004000);  // dfe_rotate_f2_5=1
// phy setting page 1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000017F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000001);  // page=1  
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000103);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000300);  // FFE_OS_FORCE=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010C);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000C00);  // max_blind_loop[4:0]=0, max_big_loop[4:0]=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010E);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_max_round_set1_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000010F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_max_round_set2_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000110);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000000DF);  // rx_ffe_test_index_init[3:0]=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000115);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000727);  // rx_ffe_test_r8, c8, r9, c9
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000116);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00004767);  // rx_ffe_test_ra, ca, rb, cb
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000117);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000020);  // rx_ffe_test_rc, cc, rd, cd
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000118);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00004060);  // rx_ffe_test_re, ce, rf, cf
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000011A);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000EC02);  // TX_TRAIN_P2P_HOLD=1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000121);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00004A40);  // cdr_fn1_mode=1, cdr_phase_step_size[1:0]=0, cdrphase_opt_last_always=0, cdrphase_opt_first_en=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000127);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000025A);  // tx_amp_max[5:0]=1A
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000137);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000A74);  // tx_amp_default1, tx_emph0_default1, tx_emph1_default1
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000147);
        WRITE_ALL_PORT_VSR_DATA(root, 0x0000001A); //  WRITE_ALL_PORT_VSR_DATA(root, 0x0000001B);  // preset index = 3
//        WRITE_ALL_PORT_VSR_DATA(root, 0x0000001B); //  WRITE_ALL_PORT_VSR_DATA(root, 0x0000001B);  // preset index = 3
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000148);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000016D0);  // edge_settle_time_en=1, dfe_settle_time_en=1, os_delta_max_2[4:0]=0x10
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000014C);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000C0F);  // SAS_RX_TRAIN_DIS=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000014E);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001032);  // tx_amp_default2, tx_emph0_default2, tx_emph1_default2
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000014F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001500);  // tx_amp_default3
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000153);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_max_round_set4_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000154);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_max_round_set5_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000155);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000000);  // rx_max_round_set6_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000156);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000000);  // rx_max_round_set7_*
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000157);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00005E09);  // remote_status_rechk_en
//        WRITE_ALL_PORT_VSR_DATA(root, 0x00001E09);  // remote_status_rechk_en
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000158);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001903);  // tx_f1_mustin_mode=0, tx_fn1_mustin_mode=0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x00000159);
        WRITE_ALL_PORT_VSR_DATA(root, 0x000001FF);  // tx_f1_high_thres_c[5:0]=1F, tx_f1_high_thres_k[3:0]=F
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000015A);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00001000);  // rx_max_round_set1_*_6g
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000015B);
//        WRITE_ALL_PORT_VSR_DATA(root, 0x000006F8); //  WRITE_ALL_PORT_VSR_DATA(root, 0x000002F8);  // dfe_res_train_en=1
        WRITE_ALL_PORT_VSR_DATA(root, 0x000002F8); //  WRITE_ALL_PORT_VSR_DATA(root, 0x000002F8);  // dfe_res_train_en=1
        // switch phy setting page 0
        WRITE_ALL_PORT_VSR_ADDR(root, 0x0000017F);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000000);
        WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_MODE_REG_0); //0x00000060);
        WRITE_ALL_PORT_VSR_DATA(root, 0x00000002);
        WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_CONFIG); // 0x00000008
        WRITE_ALL_PORT_VSR_DATA(root, tmp);      // didn't phy reset
       // WRITE_ALL_PORT_VSR_DATA(root, 0xC0007EFf);
    }else{
// phy setting page 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);  // page=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000101);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000FD24);  // PHY_MODE=1, REF_FREF_SEL[4:0]=4
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000102);
        WRITE_PORT_VSR_DATA(root, phy, 0x000074CF);  // USE_MAX_PLL_RATE=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000104);
        WRITE_PORT_VSR_DATA(root, phy, 0x00008A82); //WRITE_PORT_VSR_DATA(root, phy, 0x00008A89); //ssc/amp 2250ppm //WRITE_PORT_VSR_DATA(root, phy, 0x00008A8C);  // SSC_DSPREAD_RX=0, debug purpose
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000106);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000863E);  // AC_TERM_EN=1, SQ_THRESH_IN[4:0]=6, FFE_SETTING_FORCE=0, FFE_RES_SEL=3, FFE_CAP_SEL=E
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000107);
        WRITE_PORT_VSR_DATA(root, phy, 0x00007D26);  // DFE_RES_FORCE=0, DFE_RES=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010D);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000C9F2);  // G1_setting_0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010E);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000800);  // G1_setting_1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010F);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000AA72);  // G2_setting_0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000110);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000B80);  // G2_setting_1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000111);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000AF2);  // G3_setting_0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000112);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001788);  // G3_setting_1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000113);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000AF2);  // G4_setting_0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000114);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001651);  // G4_setting_1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000122);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001026);  // password=26
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000123);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000074);  // SEL_BITS=2
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000125);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000FFF);  // DET_BYPASS=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000126);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000133);  // PHY_GEN_TX=3, PHY_GEN_RX=3, not necessary       
         /* new setting */
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000013D);
        WRITE_PORT_VSR_DATA(root, phy, 0x000009C0);  // G1_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000013E);
        WRITE_PORT_VSR_DATA(root, phy, 0x000009C0);  // G2_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000013F);
        WRITE_PORT_VSR_DATA(root, phy, 0x000009C0);  // G3_TX_SSC_AMP[6:0] spread spectrum clock Amplitude        
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000140);
        WRITE_PORT_VSR_DATA(root, phy, 0x000009C0);  // G4_TX_SSC_AMP[6:0] spread spectrum clock Amplitude
        /* end */
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000142);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000243B);  // vdd_cal_reg0
//        WRITE_PORT_VSR_DATA(root, phy, 0x0000303B);  // vdd_cal_reg0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000143);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000E3B);  // vdd_cal_reg1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000144);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000D3B);  // vdd_cal_reg2
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000145);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000398);  // vdd_cal_reg3
//        WRITE_PORT_VSR_DATA(root, phy, 0x00000390);  // vdd_cal_reg3
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014A);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000400);  // ck100k_in_pll_freq_sel[1:0]=2
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014D);
        WRITE_PORT_VSR_DATA(root, phy, 0x00002050);  // sellv_rxintp[2:0]=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000158);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000810A);  // txdcc_cal_stop_sel=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015A);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000DC28);  // cal_rxclkalign90_ext_en=1, cal_os_ph_ext[6:0]=5A
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015C);
        WRITE_PORT_VSR_DATA(root, phy, 0x00006E4A);  // process_cal_subss, process_cal_ss2tt, process_cal_tt2ff
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000161);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001077);  // dtl_clamping_en=0, dtl_clamping_sel=7
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000162);
        WRITE_PORT_VSR_DATA(root, phy, 0x00002A60);  // no_seq=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000164);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_foffset_extraction_en=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000016A);
        WRITE_PORT_VSR_DATA(root, phy, 0x00004000);  // dfe_rotate_f2_5=1
// phy setting page 1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000001);  // page=1  
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000103);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000300);  // FFE_OS_FORCE=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010C);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000C00);  // max_blind_loop[4:0]=0, max_big_loop[4:0]=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010E);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_max_round_set1_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_max_round_set2_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000110);
        WRITE_PORT_VSR_DATA(root, phy, 0x000000DF);  // rx_ffe_test_index_init[3:0]=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000115);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000727);  // rx_ffe_test_r8, c8, r9, c9
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000116);
        WRITE_PORT_VSR_DATA(root, phy, 0x00004767);  // rx_ffe_test_ra, ca, rb, cb
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000117);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000020);  // rx_ffe_test_rc, cc, rd, cd
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000118);
        WRITE_PORT_VSR_DATA(root, phy, 0x00004060);  // rx_ffe_test_re, ce, rf, cf
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000011A);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000EC02);  // TX_TRAIN_P2P_HOLD=1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000121);
        WRITE_PORT_VSR_DATA(root, phy, 0x00004A40);  // cdr_fn1_mode=1, cdr_phase_step_size[1:0]=0, cdrphase_opt_last_always=0, cdrphase_opt_first_en=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000127);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000025A);  // tx_amp_max[5:0]=1A
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000137);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000A74);  // tx_amp_default1, tx_emph0_default1, tx_emph1_default1
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000147);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000001A);  // WRITE_PORT_VSR_DATA(root, phy, 0x0000001B);  // preset index = 3
//        WRITE_PORT_VSR_DATA(root, phy, 0x0000001B); // preset index = 3
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000148);
        WRITE_PORT_VSR_DATA(root, phy, 0x000016D0);  // edge_settle_time_en=1, dfe_settle_time_en=1, os_delta_max_2[4:0]=0x10
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014C);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000C0F);  // SAS_RX_TRAIN_DIS=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014E);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001032);  // tx_amp_default2, tx_emph0_default2, tx_emph1_default2
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001500);  // tx_amp_default3
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000153);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_max_round_set4_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000154);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_max_round_set5_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000155);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);  // rx_max_round_set6_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000156);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);  // rx_max_round_set7_*
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000157);
	WRITE_PORT_VSR_DATA(root, phy, 0x00005E09);  // remote_status_rechk_en
//        WRITE_PORT_VSR_DATA(root, phy, 0x00001E09);  // remote_status_rechk_en
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000158);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001903);  // tx_f1_mustin_mode=0, tx_fn1_mustin_mode=0
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000159);
        WRITE_PORT_VSR_DATA(root, phy, 0x000001FF);  // tx_f1_high_thres_c[5:0]=1F, tx_f1_high_thres_k[3:0]=F
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015A);
        WRITE_PORT_VSR_DATA(root, phy, 0x00001000);  // rx_max_round_set1_*_6g
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015B);
//        WRITE_PORT_VSR_DATA(root, phy, 0x000006F8);  // WRITE_PORT_VSR_DATA(root, phy, 0x000002F8);  // dfe_res_train_en=1
        WRITE_PORT_VSR_DATA(root, phy, 0x000002F8);  // dfe_res_train_en=1

        // switch phy setting page 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_MODE_REG_0); //0x00000060);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000002);
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG); // 0x00000008
        WRITE_PORT_VSR_DATA(root, phy, tmp);      // didn't phy reset
    }
#ifdef NOT_SPINUP_WORKAROUND
    if(phy){
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
        //		WRITE_PORT_CONFIG_DATA(root, phy, 0x04);
        WRITE_PORT_CONFIG_DATA(root, phy, 0x00);
    }else{
        WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_PHY_CONTROL);
        //		WRITE_PORT_CONFIG_DATA(root, phy, 0x04);
        WRITE_ALL_PORT_CONFIG_DATA(root, 0x00);
    }
#endif
}
#if 1
MV_BOOLEAN set_port_vsr_data(pl_root *root, domain_phy *phy, MV_U32 offset, MV_U32 reg){
	MV_U32 tmp, cnt;
	WRITE_PORT_VSR_DATA(root, phy, reg);
	tmp= (READ_PORT_VSR_DATA(root, phy) & 0xFFFF);
	cnt = 10;
	while((tmp != reg) && (cnt)){
		mdelay(1);
		WRITE_PORT_VSR_DATA(root, phy, reg);
		tmp= (READ_PORT_VSR_DATA(root, phy) & 0xFFFF);
		cnt--;
	}
	if(!cnt){
		CORE_EH_PRINT(("set_port_vsr_data phy %x reg 0x%x expect 0x%x, result 0x%x\n", (root->base_phy_num +phy->asic_id), offset, reg, tmp));
		return MV_FALSE;
	}
	return MV_TRUE;
}
MV_VOID set_12G_sas_setting_a0(pl_root *root, domain_phy *phy, MV_BOOLEAN phy_reset, MV_BOOLEAN phy_disable, Phy_Info_Page* p_phy_info_param)
{
    MV_U32 tmp;
	MV_U32 reg, offset;
#ifdef SUPPORT_DIRECT_SATA_SSU
	core_extension *core = root->core;
#endif
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);  // page=0
        //---------------------------------------------
        offset = 0x00000101;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	  reg = READ_PORT_VSR_DATA(root, phy);
	  reg &= 0x0000ff00;
	  reg |= 0x24;	// PHY_MODE=1, REF_FREF_SEL[4:0]=0x4
        //WRITE_PORT_VSR_DATA(root, phy, reg);
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------	
	  offset = 0x00000102;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	  reg = READ_PORT_VSR_DATA(root, phy);
	  reg &= 0x0000efff;
	  reg |= 0x1000;	// USE_MAX_PLL_RATE=0x1
        //WRITE_PORT_VSR_DATA(root, phy, reg);  
	set_port_vsr_data(root, phy, offset, reg);

	  //---------------------------------------------
	 offset = 0x00000106;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000e0ff;
		 reg |= 0x600;	// SQ_THRESH_IN[4:0]=0x6
        //WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);

	  //---------------------------------------------	
	 offset = 0x0000010D;
	  reg = 0xc8cc;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	  
        //WRITE_PORT_VSR_DATA(root, phy, 0xc8cc); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
	  offset = 0x0000010F;
	  reg = 0xa950;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
        //WRITE_PORT_VSR_DATA(root, phy, 0xa950); 
        set_port_vsr_data(root, phy, offset, reg);

		//---------------------------------------------	
	  offset = 0x00000111;
	  reg = 0x0b70;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
//        WRITE_PORT_VSR_DATA(root, phy, 0x0a56); 
//	 WRITE_PORT_VSR_DATA(root, phy, 0x0b70);
        set_port_vsr_data(root, phy, offset, reg);

		//---------------------------------------------	
	  offset = 0x00000113;
	  reg = 0x0ae0;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
        //WRITE_PORT_VSR_DATA(root, phy, 0x0ae0); 
        set_port_vsr_data(root, phy, offset, reg);

	  //---------------------------------------------
	  offset = 0x00000122;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	  	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000ff00;
		 reg |= 0x26;	// password=0x26
       // WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);

        //---------------------------------------------
        offset = 0x00000123;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000fff1;
		 reg |= 0x4;	// SEL_BITS=0x2
       // WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);

	  //---------------------------------------------
        offset = 0x00000125;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000efff;
		 reg |= 0x0;	// DET_BYPASS=0x0
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);

	  //---------------------------------------------
        offset = 0x0000013D;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
        offset = 0x0000013E;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
        offset = 0x0000013F;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
        offset = 0x00000140;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
        offset = 0x0000014A;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	 	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000f9ff;
		 reg |= 0x400;	// ck100k_in_pll_freq_sel=0x2
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
#if 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015A);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000d428); 
#endif
	  //---------------------------------------------
        offset = 0x00000161;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	  	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000ff8f;
		 reg |= 0x70;	//dtl_clamping_sel=0x7
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	   //---------------------------------------------
        offset = 0x00000164;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
        	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000efff;
		 reg |= 0x1000;	//rx_foffset_extraction_en=0x1
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
#if 1
	  //---------------------------------------------
	  // phy setting page 1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000001);  // page=1  
        //---------------------------------------------
        offset = 0x00000103;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000feff;
		 reg |= 0x0;	//EDGE_EQ_EN
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	 //---------------------------------------------
#if 0
	 WRITE_PORT_VSR_ADDR(root, phy, 0x00000110);
        WRITE_PORT_VSR_DATA(root, phy, 0x000000c6);  
#endif
	  //---------------------------------------------
        offset = 0x00000129;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
		reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000e7ff;
		 reg |= 0x0;	//TX_TRAIN_START_FRAME_DETECTED_EN = 0, TX_TRAIN_START_SQ_EN = 0 
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
#endif
#if 1
	// phy setting page 2
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000002);  // page=2 
	  //---------------------------------------------
        offset = 0x00000112;
	  reg = 0x00000000;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
        //WRITE_PORT_VSR_DATA(root, phy, 0x00000000);   
	 set_port_vsr_data(root, phy, offset, reg);
	  //---------------------------------------------
        offset = 0x0000011b;
        WRITE_PORT_VSR_ADDR(root, phy, offset);
	 reg = READ_PORT_VSR_DATA(root, phy);
        reg &= 0x0000fffd;	//Reg 0x1B [1]==0, R3P0_page3.
//        WRITE_PORT_VSR_DATA(root, phy, reg); 
        set_port_vsr_data(root, phy, offset, reg);
#endif
	  // switch phy setting page 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);
#if 1
	  //phy sw reset 2/5
	  WRITE_PORT_VSR_ADDR(root, phy, 0x00000152);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000e408);
	 //mdelay(20);
	  WRITE_PORT_VSR_DATA(root, phy, 0x0000e008);
	 mdelay(20);

	 // core_sleep_millisecond(root->core, 20);
#endif	  
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_MODE_REG_0); //0x00000060);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000002);
#if 1 //phy settings
	set_phy_ffe_tuning(root,  phy, p_phy_info_param->FFE_Control[root->base_phy_num+phy->asic_id]);
	set_phy_tuning(root,  phy, p_phy_info_param->PHY_Tuning_Gen1[root->base_phy_num+phy->asic_id] , p_phy_info_param->PHY_Tuning_Gen2[root->base_phy_num+phy->asic_id],  p_phy_info_param->PHY_Tuning_Gen3[root->base_phy_num+phy->asic_id], p_phy_info_param->PHY_Tuning_Gen4[root->base_phy_num+phy->asic_id]);
#endif

    tmp = 0xC0087EFF; //tmp = 0xC0007EFF;   //set down-spreading as default //       tmp = 0xC0087EFF;
#if 1 // phy rate
	switch(p_phy_info_param->PHY_Rate[root->base_phy_num+phy->asic_id]) {
	case 0x0:
		tmp = 0x0008601FL; //support 1.5Gbps
		break;
	case 0x1:
		tmp = 0x000C793FL; //support 1.5,3.0Gbps
		break;
	case 0x2:
		tmp = 0x000CFEFFL;//support 1.5,3.0Gbps,6.0Gbps
		break;
	case 0x3:
	default:		
		tmp = 0xC0087EFFL;//support 1.5,3.0Gbps,6.0Gbps,12.0Gbps
		break;
	}
#endif
    if(phy_disable){
        tmp|= VSR_DSBL_PHY;
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG); // 0x00000008
        WRITE_PORT_VSR_DATA(root, phy, tmp);      // didn't phy reset
    }
    if(phy_reset)
        tmp|=VSR_PHY_RESET;
#ifdef SUPPORT_DIRECT_SATA_SSU
	if (core->sata_ssu_time||core->sata_ssu_mode) {
		/* enable sata spin up hold */
		tmp |= VSR_SATA_SPIN_UP_SUPPORT;
	}
	if (core->sata_ssu_time||core->sata_ssu_mode) {
		/* should spin up immediately */
		if ((!core->sata_ssu_time) || (!core->sata_ssu_group) || ((phy->asic_id + root->base_phy_num) / core->sata_ssu_group)== 0) {
			tmp |= VSR_SATA_SPIN_UP_ENABLE;
			phy->is_spinup_hold = MV_FALSE;
		} else {
			/* bit 21(SATA_SPIN_UP_ENABLE) is 0 by default */
			phy->is_spinup_hold = MV_TRUE;
		}
	}
#endif
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG); // 0x00000008
        WRITE_PORT_VSR_DATA(root, phy, tmp);      // didn't phy reset
}
#else
MV_VOID set_12G_sas_setting_a0(pl_root *root, domain_phy *phy, MV_BOOLEAN phy_reset, MV_BOOLEAN phy_disable, Phy_Info_Page* p_phy_info_param)
{
    MV_U32 tmp;
	MV_U32 reg;
#ifdef SUPPORT_DIRECT_SATA_SSU
	core_extension *core = root->core;
#endif
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);  // page=0
        //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000101);
	  reg = READ_PORT_VSR_DATA(root, phy);
	  reg &= 0x0000ff00;
	  reg |= 0x24;	// PHY_MODE=1, REF_FREF_SEL[4:0]=0x4
        WRITE_PORT_VSR_DATA(root, phy, reg);  
	  //---------------------------------------------	
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000102);
	  reg = READ_PORT_VSR_DATA(root, phy);
	  reg &= 0x0000efff;
	  reg |= 0x1000;	// USE_MAX_PLL_RATE=0x1
        WRITE_PORT_VSR_DATA(root, phy, reg);  
	  //---------------------------------------------		
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000106);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000e0ff;
		 reg |= 0x600;	// SQ_THRESH_IN[4:0]=0x6
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------		
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010D);
        WRITE_PORT_VSR_DATA(root, phy, 0xc8cc); 
	  //---------------------------------------------		
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000010F);
        WRITE_PORT_VSR_DATA(root, phy, 0xa950); 
		//---------------------------------------------		
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000111);
//        WRITE_PORT_VSR_DATA(root, phy, 0x0a56); 
	 WRITE_PORT_VSR_DATA(root, phy, 0x0b70);
		//---------------------------------------------		
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000113);
        WRITE_PORT_VSR_DATA(root, phy, 0x0ae0); 
	  //---------------------------------------------	
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000122);
	  	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000ff00;
		 reg |= 0x26;	// password=0x26
        WRITE_PORT_VSR_DATA(root, phy, reg); 
        //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000123);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000fff1;
		 reg |= 0x4;	// SEL_BITS=0x2
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000125);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000efff;
		 reg |= 0x0;	// DET_BYPASS=0x0
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
	  WRITE_PORT_VSR_ADDR(root, phy, 0x0000013D);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
	  WRITE_PORT_VSR_ADDR(root, phy, 0x0000013E);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
	  WRITE_PORT_VSR_ADDR(root, phy, 0x0000013F);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
	  WRITE_PORT_VSR_ADDR(root, phy, 0x00000140);
		 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x000001ff;
		 reg |= 0x800;	// G1_TX_SSC_AMP = 0x04;
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000014A);
	 	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000f9ff;
		 reg |= 0x400;	// ck100k_in_pll_freq_sel=0x2
        WRITE_PORT_VSR_DATA(root, phy, reg);  
	  //---------------------------------------------
#if 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000015A);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000d428); 
#endif
	  //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000161);
	  	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000ff8f;
		 reg |= 0x70;	//dtl_clamping_sel=0x7
        WRITE_PORT_VSR_DATA(root, phy, reg);  
	   //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000164);
        	 reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000efff;
		 reg |= 0x1000;	//rx_foffset_extraction_en=0x1
        WRITE_PORT_VSR_DATA(root, phy, reg);  
#if 1
	  //---------------------------------------------
	  // phy setting page 1
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000001);  // page=1  
        //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000103);
		reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000feff;
		 reg |= 0x0;	//EDGE_EQ_EN
        WRITE_PORT_VSR_DATA(root, phy, reg);  
	 //---------------------------------------------
#if 0
	 WRITE_PORT_VSR_ADDR(root, phy, 0x00000110);
        WRITE_PORT_VSR_DATA(root, phy, 0x000000c6);  
#endif
	  //---------------------------------------------
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000129);
		reg = READ_PORT_VSR_DATA(root, phy);
		 reg &= 0x0000e7ff;
		 reg |= 0x0;	//TX_TRAIN_START_FRAME_DETECTED_EN = 0, TX_TRAIN_START_SQ_EN = 0 
        WRITE_PORT_VSR_DATA(root, phy, reg); 
	  //---------------------------------------------
#endif
#if 1
	// phy setting page 2
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000002);  // page=2 
        WRITE_PORT_VSR_ADDR(root, phy, 0x00000112);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);   
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000011b);
	  reg = READ_PORT_VSR_DATA(root, phy);
        reg &= 0x0000fffd;	//Reg 0x1B [1]==0, R3P0_page3.
        WRITE_PORT_VSR_DATA(root, phy, reg);   
#endif
	  // switch phy setting page 0
        WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000000);
#if 1
	  //phy sw reset 2/5
	  WRITE_PORT_VSR_ADDR(root, phy, 0x00000152);
        WRITE_PORT_VSR_DATA(root, phy, 0x0000e408);
	  WRITE_PORT_VSR_DATA(root, phy, 0x0000e008);
	  core_sleep_millisecond(root->core, 20);
#endif	  
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_MODE_REG_0); //0x00000060);
        WRITE_PORT_VSR_DATA(root, phy, 0x00000002);
#if 1 //phy settings
	set_phy_ffe_tuning(root,  phy, p_phy_info_param->FFE_Control[root->base_phy_num+phy->asic_id]);
	set_phy_tuning(root,  phy, p_phy_info_param->PHY_Tuning_Gen1[root->base_phy_num+phy->asic_id] , p_phy_info_param->PHY_Tuning_Gen2[root->base_phy_num+phy->asic_id],  p_phy_info_param->PHY_Tuning_Gen3[root->base_phy_num+phy->asic_id], p_phy_info_param->PHY_Tuning_Gen4[root->base_phy_num+phy->asic_id]);
#endif

    tmp = 0xC0087EFF; //tmp = 0xC0007EFF;   //set down-spreading as default //       tmp = 0xC0087EFF;
#if 1 // phy rate
	switch(p_phy_info_param->PHY_Rate[root->base_phy_num+phy->asic_id]) {
	case 0x0:
		tmp = 0x0008601FL; //support 1.5Gbps
		break;
	case 0x1:
		tmp = 0x000C793FL; //support 1.5,3.0Gbps
		break;
	case 0x2:
		tmp = 0x000CFEFFL;//support 1.5,3.0Gbps,6.0Gbps
		break;
	case 0x3:
	default:		
		tmp = 0xC0087EFFL;//support 1.5,3.0Gbps,6.0Gbps,12.0Gbps
		break;
	}
#endif
    if(phy_disable){
        tmp|= VSR_DSBL_PHY;
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG); // 0x00000008
        WRITE_PORT_VSR_DATA(root, phy, tmp);      // didn't phy reset
    }
    if(phy_reset)
        tmp|=VSR_PHY_RESET;
#ifdef SUPPORT_DIRECT_SATA_SSU
	if (core->sata_ssu_time||core->sata_ssu_mode) {
		/* enable sata spin up hold */
		tmp |= VSR_SATA_SPIN_UP_SUPPORT;
	}
	if (core->sata_ssu_time||core->sata_ssu_mode) {
		/* should spin up immediately */
		if ((!core->sata_ssu_time) || (!core->sata_ssu_group) || ((phy->asic_id + root->base_phy_num) / core->sata_ssu_group)== 0) {
			tmp |= VSR_SATA_SPIN_UP_ENABLE;
			phy->is_spinup_hold = MV_FALSE;
		} else {
			/* bit 21(SATA_SPIN_UP_ENABLE) is 0 by default */
			phy->is_spinup_hold = MV_TRUE;
		}
	}
#endif
        WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG); // 0x00000008
        WRITE_PORT_VSR_DATA(root, phy, tmp);      // didn't phy reset
}
#endif
/*Notice: this function must be called when phy is disabled*/
MV_VOID set_phy_rate(pl_root *root, domain_phy *phy, MV_U8 rate)
{
	MV_U32 tmp;
	// switch phy setting page 0
#if defined(ATHENA_FPGA_WORKAROUND)
	WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_CONFIG);
	tmp = 0x00001e;
	WRITE_ALL_PORT_VSR_DATA(root, tmp);
	return;
#endif

	WRITE_PORT_VSR_ADDR(root, phy, 0x0000017F);
	WRITE_PORT_VSR_DATA(root, phy, 0x00000000);
	   
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= 0x1FF0000FL; 

	//bit 31:4 for phy rate config. 		(1.5Gbps, 3Gbps, 6Gbps, 12Gbps).
	/*bit31:29      Tx Supported Physical Link Rates for G3 and G4 (000, 000, 000, 110)
	* Bit19:          Tx SSC type (1, 1, 1, 1)
	* Bit18:15:	Tx Requested Logical Link Rate(Multiplexing)(0000b, 1000b, 1001b, 1010b)
	* Bit14:9:	Tx Supported Physical Link Rates	(110000b, 111100b, 111111b, 111111b)
	* Bit8: 		Parity bit to ensure bit31:29 and bit19:8 is odd (0, 1, 0, 0)
	* Bit7: 		SNW3 supported				(0, 0, 1, 1)
	* Bit6:4		Support speed List				(001b, 011b, 111b, 111b)
	*/
		
	switch(rate) {
	case 0x0:
		tmp |= 0x0008601EL; //support 1.5Gbps
		break;
	case 0x1:
		tmp |= 0x000C793EL; //support 1.5,3.0Gbps
		break;
	case 0x2:
		tmp |= 0x000CFEFEL;//support 1.5,3.0Gbps,6.0Gbps
		break;
	case 0x3:
	default:		
		tmp |= 0xC00D7EFEL;//support 1.5,3.0Gbps,6.0Gbps,12.0Gbps
		break;
	}
	WRITE_PORT_VSR_DATA(root, phy, tmp);
	
}

#ifdef MV_DEBUG
MV_VOID athena_dump_common_reg(void *root_p)
{
	MV_U32 i = 0;
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
    MV_DPRINT(("COMMON_PORT_IMPLEMENT(0x%X)=%08X.\n", COMMON_PORT_IMPLEMENT, MV_REG_READ_DWORD(mmio, COMMON_PORT_IMPLEMENT)));
    MV_DPRINT(("COMMON_PORT_TYPE(0x%X)=%08X.\n", COMMON_PORT_TYPE, MV_REG_READ_DWORD(mmio, COMMON_PORT_TYPE)));
    MV_DPRINT(("COMMON_FIS_ADDR (0x%X)=%08X.\n", COMMON_FIS_ADDR, MV_REG_READ_DWORD(mmio, COMMON_FIS_ADDR)));
    MV_DPRINT(("COMMON_FIS_ADDR_HI(0x%X)=%08X.\n", COMMON_FIS_ADDR_HI, MV_REG_READ_DWORD(mmio, COMMON_FIS_ADDR_HI)));

    MV_DPRINT(("COMMON_LST_ADDR (0x%X)=%08X.\n", COMMON_LST_ADDR, MV_REG_READ_DWORD(mmio, COMMON_LST_ADDR)));
    MV_DPRINT(("COMMON_LST_ADDR_HI(0x%X)=%08X.\n", COMMON_LST_ADDR_HI, MV_REG_READ_DWORD(mmio, COMMON_LST_ADDR_HI)));
    MV_DPRINT(("COMMON_CONFIG   (0x%X)=%08X.\n", COMMON_CONFIG, MV_REG_READ_DWORD(mmio, COMMON_CONFIG)));
    MV_DPRINT(("COMMON_CONTROL  (0x%X)=%08X.\n", COMMON_CONTROL, MV_REG_READ_DWORD(mmio, COMMON_CONTROL)));
    	
    MV_DPRINT(("COMMON_SATA_REG_SET0(0x%X)=%08X.\n", COMMON_SATA_REG_SET0, MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET0)));
    MV_DPRINT(("COMMON_SATA_REG_SET1(0x%X)=%08X.\n", COMMON_SATA_REG_SET1, MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET1)));
    MV_DPRINT(("COMMON_SATA_REG_SET2(0x%X)=%08X.\n", COMMON_SATA_REG_SET2, MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET2)));
    MV_DPRINT(("COMMON_SATA_REG_SET3(0x%X)=%08X.\n", COMMON_SATA_REG_SET3, MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET3)));

    for(i=0;i<MAX_MULTI_QUEUE;i++){
        MV_DPRINT(("DlvryQ %d Configuration(0x%X)=%08X.\n", i, COMMON_DELV_Q0_CONFIG +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_CONFIG(root, i)));
        MV_DPRINT(("DlvryQ %d Base Addr(0x%X)=%08X.\n", i, COMMON_DELV_Q0_ADDR +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_BASE_ADDR(root, i)));
        MV_DPRINT(("DlvryQ %d Base Addr hi(0x%X)=%08X.\n", i, COMMON_DELV_Q0_ADDR_HI +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_BASE_ADDR_HI(root, i)));
        MV_DPRINT(("DlvryQ %d Read Pointer Shadow Addr(0x%X)=%08X.\n", i, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR+i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_RD_SHADOW_ADDR(root, i)));
        MV_DPRINT(("DlvryQ %d Read Pointer Shadow Addr Hi(0x%X)=%08X.\n", i, COMMON_DELV_Q0_RD_PTR_SHADOW_ADDR_HI +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_RD_SHADOW_ADDR_HI(root, i)));
        MV_DPRINT(("DlvryQ %d Write Pointer (0x%X)=%08X.\n", i, COMMON_DELV_Q0_WR_PTR +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_WR_PTR(root, i)));
        MV_DPRINT(("DlvryQ %d Read Pointer (0x%X)=%08X.\n", i, COMMON_DELV_Q0_RD_PTR +i*MV_DLVRYQ_OFFSET , READ_DLVRYQ_RD_PTR(root, i)));

        MV_DPRINT(("CmplQ %d Configuration(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_CONFIG +i*MV_CMPLQ_OFFSET , READ_CMPLQ_CONFIG(root, i)));
        MV_DPRINT(("CmplQ %d Base Addr(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_ADDR +i*MV_CMPLQ_OFFSET , READ_CMPLQ_BASE_ADDR(root, i)));
        MV_DPRINT(("CmplQ %d Base Addr Hi(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_ADDR_HI +i*MV_CMPLQ_OFFSET , READ_CMPLQ_BASE_ADDR_HI(root, i)));
        MV_DPRINT(("CmplQ %d Write Pointer Shadow Addr (0x%X)=%08X.\n", i, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR +i*MV_CMPLQ_OFFSET , READ_CMPLQ_WR_SHADOW_ADDR(root, i)));
        MV_DPRINT(("CmplQ %d Write Pointer Shadow Addr Hi(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_WR_PTR_SHADOW_ADDR_HI +i*MV_CMPLQ_OFFSET , READ_CMPLQ_WR_SHADOW_ADDR_HI(root, i)));
        MV_DPRINT(("CmplQ %d Write Pointer(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_WR_PTR +i*MV_CMPLQ_OFFSET , READ_CMPLQ_WR_PTR(root, i)));
        MV_DPRINT(("CmplQ %d Read Pointer(0x%X)=%08X.\n", i, COMMON_CMPL_Q0_RD_PTR +i*MV_CMPLQ_OFFSET , READ_CMPLQ_RD_PTR(root, i)));
    }
    MV_DPRINT(("SAS/SATA Common Interrupt Cause (0x%X)=%08X.\n", COMMON_IRQ_STAT, MV_REG_READ_DWORD(mmio, COMMON_IRQ_STAT)));
    MV_DPRINT(("SAS/SATA SAS/SATA CmplQ N Interrupt Cause (0x%X)=%08X.\n", COMMON_CMPLQ_IRQN_STAT, MV_REG_READ_DWORD(mmio, COMMON_CMPLQ_IRQN_STAT)));
    MV_DPRINT(("SAS/SATA SAS/SATA CmplQ N Interrupt Enable (0x%X)=%08X.\n", COMMON_CMPLQ_IRQN_MASK, MV_REG_READ_DWORD(mmio, COMMON_CMPLQ_IRQN_MASK)));

    for(i=0;i<MAX_MULTI_QUEUE;i++){
        MV_DPRINT(("Interrupt Coalescing %d Configuration(0x%X)=%08X.\n", i, COMMON_COAL0_CONFIG +i*MV_COAL_OFFSET , READ_IRQ_COAL_CONFIG(root, i)));
        MV_DPRINT(("Interrupt Coalescing %d Timer(0x%X)=%08X.\n", i, COMMON_COAL0_TIMEOUT +i*MV_COAL_OFFSET , READ_IRQ_COAL_TIMEOUT(root, i)));
    }
    for(i=0;i<MAX_MULTI_QUEUE;i++){
        MV_DPRINT(("SAS/SATA CmplQ %d Interrupt Cause(0x%X)=%08X.\n", i, COMMON_CMPLQ_IRQ0_STAT +i*MV_CMPLQ_IRQ_OFFSET , READ_CMPLQ_IRQ_STAT(root, i)));
        MV_DPRINT(("SAS/SATA CmplQ %d Interrupt Enable(0x%X)=%08X.\n", i, COMMON_CMPLQ_IRQ0_MASK +i*MV_CMPLQ_IRQ_OFFSET , READ_CMPLQ_IRQ_MASK(root, i)));
    }
    for(i=0;i<MAX_MULTI_QUEUE;i++){
        MV_DPRINT(("SAS/SATA Interrupt Routing %d  (0x%X)=%08X.\n", i, COMMON_IRQ_ROUTING0 +i*MV_IRQ_ROUTING_OFFSET , READ_IRQ_ROUTING(root, i)));
    }

    MV_DPRINT(("COMMON_SRS_IRQ_STAT0(0x%X)=%08X.\n", COMMON_SRS_IRQ_STAT0, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT0)));
    MV_DPRINT(("COMMON_SRS_IRQ_MASK0(0x%X)=%08X.\n", COMMON_SRS_IRQ_MASK0, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK0)));
    MV_DPRINT(("COMMON_SRS_IRQ_STAT1(0x%X)=%08X.\n", COMMON_SRS_IRQ_STAT1, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT1)));
    MV_DPRINT(("COMMON_SRS_IRQ_MASK1(0x%X)=%08X.\n", COMMON_SRS_IRQ_MASK1, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK1)));
    MV_DPRINT(("COMMON_SRS_IRQ_STAT2(0x%X)=%08X.\n", COMMON_SRS_IRQ_STAT2, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT2)));
    MV_DPRINT(("COMMON_SRS_IRQ_MASK2(0x%X)=%08X.\n", COMMON_SRS_IRQ_MASK2, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK2)));
    MV_DPRINT(("COMMON_SRS_IRQ_STAT3(0x%X)=%08X.\n", COMMON_SRS_IRQ_STAT3, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT3)));
    MV_DPRINT(("COMMON_SRS_IRQ_MASK3(0x%X)=%08X.\n", COMMON_SRS_IRQ_MASK3, MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK3)));
    	
    MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR0(0x%X)=%08X.\n", COMMON_NON_SPEC_NCQ_ERR0, MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR0)));
    MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR1(0x%X)=%08X.\n", COMMON_NON_SPEC_NCQ_ERR1, MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR1)));
    MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR2(0x%X)=%08X.\n", COMMON_NON_SPEC_NCQ_ERR2, MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR2)));
    MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR3(0x%X)=%08X.\n", COMMON_NON_SPEC_NCQ_ERR3, MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR3)));

    MV_DPRINT(("COMMON_CMD_ADDR (0x%X)=%08X.\n", COMMON_CMD_ADDR, MV_REG_READ_DWORD(mmio, COMMON_CMD_ADDR)));
    MV_DPRINT(("COMMON_CMD_DATA (0x%X)=%08X.\n", COMMON_CMD_DATA, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));

    MV_DPRINT(("COMMON_STP_CLR_AFFILIATION_DIS0(0x%X)=%08X.\n", COMMON_STP_CLR_AFFILIATION_DIS0, MV_REG_READ_DWORD(mmio, COMMON_STP_CLR_AFFILIATION_DIS0)));
    MV_DPRINT(("COMMON_STP_CLR_AFFILIATION_DIS1(0x%X)=%08X.\n", COMMON_STP_CLR_AFFILIATION_DIS1, MV_REG_READ_DWORD(mmio, COMMON_STP_CLR_AFFILIATION_DIS1)));
    MV_DPRINT(("COMMON_STP_CLR_AFFILIATION_DIS2(0x%X)=%08X.\n", COMMON_STP_CLR_AFFILIATION_DIS2, MV_REG_READ_DWORD(mmio, COMMON_STP_CLR_AFFILIATION_DIS2)));
    MV_DPRINT(("COMMON_STP_CLR_AFFILIATION_DIS3(0x%X)=%08X.\n", COMMON_STP_CLR_AFFILIATION_DIS3, MV_REG_READ_DWORD(mmio, COMMON_STP_CLR_AFFILIATION_DIS3)));

    	/* port config address/data regsiter set $i (0x170/0x174- 0x208/0x20c) */
    MV_DPRINT(("COMMON_PORT_CONFIG_ADDR0  (0x%X)=%08X.\n", COMMON_PORT_CONFIG_ADDR0, MV_REG_READ_DWORD(mmio, COMMON_PORT_CONFIG_ADDR0)));
    MV_DPRINT(("COMMON_PORT_CONFIG_DATA0  (0x%X)=%08X.\n", COMMON_PORT_CONFIG_DATA0, MV_REG_READ_DWORD(mmio, COMMON_PORT_CONFIG_DATA0)));
    MV_DPRINT(("COMMON_PORT_ALL_CONFIG_ADDR  (0x%X)=%08X.\n", COMMON_PORT_ALL_CONFIG_ADDR, MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_CONFIG_ADDR)));
    MV_DPRINT(("COMMON_PORT_ALL_CONFIG_DATA  (0x%X)=%08X.\n", COMMON_PORT_ALL_CONFIG_DATA, MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_CONFIG_DATA)));


    	/*phy control/data register set $i (0x220/0x224- 0x258/0x25c) */
    MV_DPRINT(("COMMON_PORT_PHY_CONTROL0  (0x%X)=%08X.\n",COMMON_PORT_PHY_CONTROL0, MV_REG_READ_DWORD(mmio, COMMON_PORT_PHY_CONTROL0)));
    MV_DPRINT(("COMMON_PORT_PHY_CONTROL_DATA0  (0x%X)=%08X.\n", COMMON_PORT_PHY_CONTROL_DATA0, MV_REG_READ_DWORD(mmio, COMMON_PORT_PHY_CONTROL_DATA0)));

    MV_DPRINT(("COMMON_PORT_ALL_PHY_CONTROL (0x%X)=%08X.\n",COMMON_PORT_ALL_PHY_CONTROL, MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_PHY_CONTROL)));
    MV_DPRINT(("COMMON_PORT_ALL_PHY_CONTROL_DATA (0x%X)=%08X.\n", COMMON_PORT_ALL_PHY_CONTROL_DATA, MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_PHY_CONTROL_DATA)));

            /* Athena new reqister */
#ifdef SUPPORT_SECURITY_KEY_RECORDS
    MV_DPRINT(("COMMON_KEY_UNWRAP_ENG_CTRL  (0x%X)=%08X.\n", COMMON_KEY_UNWRAP_ENG_CTRL, MV_REG_READ_DWORD(mmio, COMMON_KEY_UNWRAP_ENG_CTRL)));
    MV_DPRINT(("COMMON_KEY_UNWRAP_ENG_STS  (0x%X)=%08X.\n", COMMON_KEY_UNWRAP_ENG_STS, MV_REG_READ_DWORD(mmio, COMMON_KEY_UNWRAP_ENG_STS)));
    MV_DPRINT(("COMMON_S_KEK0_INIT_VAL  (0x%X)=%08X.\n", COMMON_S_KEK0_INIT_VAL, MV_REG_READ_DWORD(mmio, COMMON_S_KEK0_INIT_VAL)));
    MV_DPRINT(("COMMON_S_KEK0_INIT_VAL_HI  (0x%X)=%08X.\n", COMMON_S_KEK0_INIT_VAL_HI, MV_REG_READ_DWORD(mmio, COMMON_S_KEK0_INIT_VAL_HI)));
    MV_DPRINT(("COMMON_S_KEK1_INIT_VAL  (0x%X)=%08X.\n", COMMON_S_KEK1_INIT_VAL, MV_REG_READ_DWORD(mmio, COMMON_S_KEK1_INIT_VAL)));
    MV_DPRINT(("COMMON_S_KEK1_INIT_VAL_HI  (0x%X)=%08X.\n", COMMON_S_KEK1_INIT_VAL_HI, MV_REG_READ_DWORD(mmio, COMMON_S_KEK1_INIT_VAL_HI)));
    MV_DPRINT(("COMMON_KEY_UNWRAP_ENG_IRQ_STAT  (0x%X)=%08X.\n", COMMON_KEY_UNWRAP_ENG_IRQ_STAT, MV_REG_READ_DWORD(mmio, COMMON_KEY_UNWRAP_ENG_IRQ_STAT)));
    MV_DPRINT(("COMMON_KEY_UNWRAP_ENG_IRQ_MASK  (0x%X)=%08X.\n", COMMON_KEY_UNWRAP_ENG_IRQ_MASK, MV_REG_READ_DWORD(mmio, COMMON_KEY_UNWRAP_ENG_IRQ_MASK)));
#endif
    MV_DPRINT(("COMMON_INTL_MEM_PARITY_ERR  (0x%X)=%08X.\n", COMMON_INTL_MEM_PARITY_ERR, MV_REG_READ_DWORD(mmio, COMMON_INTL_MEM_PARITY_ERR)));
    MV_DPRINT(("COMMON_INTL_MEM_PARITY_ERR_EN  (0x%X)=%08X.\n", COMMON_INTL_MEM_PARITY_ERR_EN, MV_REG_READ_DWORD(mmio, COMMON_INTL_MEM_PARITY_ERR_EN)));
    MV_DPRINT(("COMMON_DATA_PATH_PARITY_CTRL  (0x%X)=%08X.\n", COMMON_DATA_PATH_PARITY_CTRL, MV_REG_READ_DWORD(mmio, COMMON_DATA_PATH_PARITY_CTRL)));
    MV_DPRINT(("COMMON_DATA_PATH_PARITY_STS  (0x%X)=%08X.\n", COMMON_DATA_PATH_PARITY_STS, MV_REG_READ_DWORD(mmio, COMMON_DATA_PATH_PARITY_STS)));
    MV_DPRINT(("COMMON_DATA_PATH_ERR_ADDR (0x%X)=%08X.\n", COMMON_DATA_PATH_ERR_ADDR, MV_REG_READ_DWORD(mmio, COMMON_DATA_PATH_ERR_ADDR)));
    MV_DPRINT(("COMMON_DATA_PATH_ERR_ADDR_HI  (0x%X)=%08X.\n", COMMON_DATA_PATH_ERR_ADDR_HI, MV_REG_READ_DWORD(mmio, COMMON_DATA_PATH_ERR_ADDR_HI)));
#ifdef SUPPORT_SECURITY_KEY_RECORDS
    MV_DPRINT(("COMMON_GLOBAL_SECURITY_CONFIG (0x%X)=%08X.\n",COMMON_GLOBAL_SECURITY_CONFIG, MV_REG_READ_DWORD(mmio, COMMON_GLOBAL_SECURITY_CONFIG)));
    MV_DPRINT(("COMMON_GLOBAL_ZERIOCATION_CTRL  (0x%X)=%08X.\n", COMMON_GLOBAL_ZERIOCATION_CTRL, MV_REG_READ_DWORD(mmio, COMMON_GLOBAL_ZERIOCATION_CTRL)));
    MV_DPRINT(("COMMON_R_KEK_VAULT_CTRL  (0x%X)=%08X.\n", COMMON_R_KEK_VAULT_CTRL, MV_REG_READ_DWORD(mmio, COMMON_R_KEK_VAULT_CTRL)));
    MV_DPRINT(("COMMON_R_KEK_VAULT_STS  (0x%X)=%08X.\n", COMMON_R_KEK_VAULT_STS, MV_REG_READ_DWORD(mmio, COMMON_R_KEK_VAULT_STS)));
    MV_DPRINT(("COMMON_R_KEK0_INIT_VAL  (0x%X)=%08X.\n", COMMON_R_KEK0_INIT_VAL, MV_REG_READ_DWORD(mmio, COMMON_R_KEK0_INIT_VAL)));
    MV_DPRINT(("COMMON_R_KEK0_INIT_VAL_HI  (0x%X)=%08X.\n", COMMON_R_KEK0_INIT_VAL_HI, MV_REG_READ_DWORD(mmio, COMMON_R_KEK0_INIT_VAL_HI)));
    MV_DPRINT(("COMMON_VAULT_PORT_CTRL  (0x%X)=%08X.\n", COMMON_VAULT_PORT_CTRL, MV_REG_READ_DWORD(mmio, COMMON_VAULT_PORT_CTRL)));
    MV_DPRINT(("COMMON_VAULT_PORT_DATA  (0x%X)=%08X.\n", COMMON_VAULT_PORT_DATA, MV_REG_READ_DWORD(mmio, COMMON_VAULT_PORT_DATA)));
    MV_DPRINT(("COMMON_GLOBAL_SECURITY_IRQ_STAT (0x%X)=%08X.\n", COMMON_GLOBAL_SECURITY_IRQ_STAT, MV_REG_READ_DWORD(mmio, COMMON_GLOBAL_SECURITY_IRQ_STAT)));
    MV_DPRINT(("COMMON_GLOBAL_SECURITY_IRQ_MASK (0x%X)=%08X.\n", COMMON_GLOBAL_SECURITY_IRQ_MASK, MV_REG_READ_DWORD(mmio, COMMON_GLOBAL_SECURITY_IRQ_MASK)));
#endif
}
MV_VOID athena_dump_port_config_reg_phy(void *root_p, void* phy_p)
{
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
    domain_phy *phy=(domain_phy *)phy_p;
    MV_U32 i=(root->base_phy_num +phy->asic_id);
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_LED_CONTROL);
        MV_DPRINT(("phy %d CONFIG_LED_CONTROL(0x%03X): 0x%08x\n", i,CONFIG_LED_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
        MV_DPRINT(("phy %d CONFIG_SATA_CONTROL(0x%03X): 0x%08x\n", i,CONFIG_SATA_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
        MV_DPRINT(("phy %d CONFIG_PHY_CONTROL (0x%03X): 0x%08x\n", i,CONFIG_PHY_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));

        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG0);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG0   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG1);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG1   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG2);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG2   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG3);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG3   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_R_ERR_COUNT);
        MV_DPRINT(("phy %d CONFIG_R_ERR_COUNT (0x%03X): 0x%08x\n", i,CONFIG_R_ERR_COUNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRC_ERR_COUNT);
        MV_DPRINT(("phy %d CONFIG_CRC_ERR_COUNT(0x%03X): 0x%08x\n", i,CONFIG_CRC_ERR_COUNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WIDE_PORT);
        MV_DPRINT(("phy %d CONFIG_WIDE_PORT   (0x%03X): 0x%08x\n", i,CONFIG_WIDE_PORT, READ_PORT_CONFIG_DATA(root, phy)));

        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO0);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO0(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO1);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO1(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO2);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO2(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME0   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME1);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME1   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME2);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME2   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME3   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME4   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME4, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME5   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME5, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME6);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME6   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME6, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME0);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME0(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME1);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME1(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME2);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME2(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME3);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME3(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME4);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME4(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME4, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME5);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME5(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME5, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME6);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME6(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME6, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
        MV_DPRINT(("phy %d CONFIG_PORT_SERIAL_CTRL_STS(0x%03X): 0x%08x\n", i,CONFIG_PORT_SERIAL_CTRL_STS, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_BROADCAST_RECIEVED);
        MV_DPRINT(("phy %d CONFIG_BROADCAST_RECIEVED(0x%03X): 0x%08x\n", i,CONFIG_BROADCAST_RECIEVED, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
        MV_DPRINT(("phy %d CONFIG_PORT_IRQ_STAT(0x%03X): 0x%08x\n", i,CONFIG_PORT_IRQ_STAT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
        MV_DPRINT(("phy %d CONFIG_PORT_IRQ_MASK(0x%03X): 0x%08x\n", i,CONFIG_PORT_IRQ_MASK, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SAS_CTRL0);
        MV_DPRINT(("phy %d CONFIG_SAS_CTRL0(0x%03X): 0x%08x\n", i,CONFIG_SAS_CTRL0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SAS_CTRL1);
        MV_DPRINT(("phy %d CONFIG_SAS_CTRL1(0x%03X): 0x%08x\n", i,CONFIG_SAS_CTRL1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_MS_CNT_TIMER);
        MV_DPRINT(("phy %d CONFIG_MS_CNT_TIMER(0x%03X): 0x%08x\n", i,CONFIG_MS_CNT_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_OPEN_RETRY);
        MV_DPRINT(("phy %d CONFIG_OPEN_RETRY(0x%03X): 0x%08x\n", i,CONFIG_OPEN_RETRY, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_TEST);
        MV_DPRINT(("phy %d CONFIG_ID_TEST(0x%03X): 0x%08x\n", i,CONFIG_ID_TEST, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PL_TIMER);
        MV_DPRINT(("phy %d CONFIG_PL_TIMER(0x%03X): 0x%08x\n", i,CONFIG_PL_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WD_TIMER);
        MV_DPRINT(("phy %d CONFIG_WD_TIMER(0x%03X): 0x%08x\n", i,CONFIG_WD_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SELECTOR_CNT);
        MV_DPRINT(("phy %d CONFIG_PORT_SELECTOR_CNT(0x%03X): 0x%08x\n", i,CONFIG_PORT_SELECTOR_CNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SL_MODE0);
        MV_DPRINT(("phy %d CONFIG_SL_MODE0(0x%03X): 0x%08x\n", i,CONFIG_SL_MODE0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SL_MODE1);
        MV_DPRINT(("phy %d CONFIG_SL_MODE1(0x%03X): 0x%08x\n", i,CONFIG_SL_MODE1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_LAYER_TIMER1);
        MV_DPRINT(("phy %d CONFIG_PORT_LAYER_TIMER1(0x%03X): 0x%08x\n", i,CONFIG_PORT_LAYER_TIMER1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_LINK_LAYER_TIMER);
        MV_DPRINT(("phy %d CONFIG_LINK_LAYER_TIMER(0x%03X): 0x%08x\n", i,CONFIG_LINK_LAYER_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
}
MV_VOID athena_dump_port_config_reg(void *root_p)
{
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
    MV_U32 i;
    domain_phy *phy;
    for (i = 0; i < root->phy_num; i++) {
        phy=&root->phy[i];
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_LED_CONTROL);
        MV_DPRINT(("phy %d CONFIG_LED_CONTROL(0x%03X): 0x%08x\n", i,CONFIG_LED_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
        MV_DPRINT(("phy %d CONFIG_SATA_CONTROL(0x%03X): 0x%08x\n", i,CONFIG_SATA_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
        MV_DPRINT(("phy %d CONFIG_PHY_CONTROL (0x%03X): 0x%08x\n", i,CONFIG_PHY_CONTROL, READ_PORT_CONFIG_DATA(root, phy)));

        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG0);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG0   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG1);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG1   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG2);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG2   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_SIG3);
        MV_DPRINT(("phy %d CONFIG_SATA_SIG3   (0x%03X): 0x%08x\n", i,CONFIG_SATA_SIG3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_R_ERR_COUNT);
        MV_DPRINT(("phy %d CONFIG_R_ERR_COUNT (0x%03X): 0x%08x\n", i,CONFIG_R_ERR_COUNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRC_ERR_COUNT);
        MV_DPRINT(("phy %d CONFIG_CRC_ERR_COUNT(0x%03X): 0x%08x\n", i,CONFIG_CRC_ERR_COUNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WIDE_PORT);
        MV_DPRINT(("phy %d CONFIG_WIDE_PORT   (0x%03X): 0x%08x\n", i,CONFIG_WIDE_PORT, READ_PORT_CONFIG_DATA(root, phy)));

        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO0);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO0(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO1);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO1(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_CRN_CNT_INFO2);
        MV_DPRINT(("phy %d CONFIG_CRN_CNT_INFO2(0x%03X): 0x%08x\n", i,CONFIG_CRN_CNT_INFO2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME0   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME1);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME1   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME2);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME2   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME3   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME4   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME4, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME5   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME5, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME6);
        MV_DPRINT(("phy %d CONFIG_ID_FRAME6   (0x%03X): 0x%08x\n", i,CONFIG_ID_FRAME6, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME0);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME0(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME1);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME1(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME2);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME2(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME2, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME3);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME3(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME3, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME4);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME4(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME4, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME5);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME5(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME5, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME6);
        MV_DPRINT(("phy %d CONFIG_ATT_ID_FRAME6(0x%03X): 0x%08x\n", i,CONFIG_ATT_ID_FRAME6, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SERIAL_CTRL_STS);
        MV_DPRINT(("phy %d CONFIG_PORT_SERIAL_CTRL_STS(0x%03X): 0x%08x\n", i,CONFIG_PORT_SERIAL_CTRL_STS, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_BROADCAST_RECIEVED);
        MV_DPRINT(("phy %d CONFIG_BROADCAST_RECIEVED(0x%03X): 0x%08x\n", i,CONFIG_BROADCAST_RECIEVED, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
        MV_DPRINT(("phy %d CONFIG_PORT_IRQ_STAT(0x%03X): 0x%08x\n", i,CONFIG_PORT_IRQ_STAT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
        MV_DPRINT(("phy %d CONFIG_PORT_IRQ_MASK(0x%03X): 0x%08x\n", i,CONFIG_PORT_IRQ_MASK, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SAS_CTRL0);
        MV_DPRINT(("phy %d CONFIG_SAS_CTRL0(0x%03X): 0x%08x\n", i,CONFIG_SAS_CTRL0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SAS_CTRL1);
        MV_DPRINT(("phy %d CONFIG_SAS_CTRL1(0x%03X): 0x%08x\n", i,CONFIG_SAS_CTRL1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_MS_CNT_TIMER);
        MV_DPRINT(("phy %d CONFIG_MS_CNT_TIMER(0x%03X): 0x%08x\n", i,CONFIG_MS_CNT_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_OPEN_RETRY);
        MV_DPRINT(("phy %d CONFIG_OPEN_RETRY(0x%03X): 0x%08x\n", i,CONFIG_OPEN_RETRY, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_TEST);
        MV_DPRINT(("phy %d CONFIG_ID_TEST(0x%03X): 0x%08x\n", i,CONFIG_ID_TEST, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PL_TIMER);
        MV_DPRINT(("phy %d CONFIG_PL_TIMER(0x%03X): 0x%08x\n", i,CONFIG_PL_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WD_TIMER);
        MV_DPRINT(("phy %d CONFIG_WD_TIMER(0x%03X): 0x%08x\n", i,CONFIG_WD_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_SELECTOR_CNT);
        MV_DPRINT(("phy %d CONFIG_PORT_SELECTOR_CNT(0x%03X): 0x%08x\n", i,CONFIG_PORT_SELECTOR_CNT, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SL_MODE0);
        MV_DPRINT(("phy %d CONFIG_SL_MODE0(0x%03X): 0x%08x\n", i,CONFIG_SL_MODE0, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SL_MODE1);
        MV_DPRINT(("phy %d CONFIG_SL_MODE1(0x%03X): 0x%08x\n", i,CONFIG_SL_MODE1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_LAYER_TIMER1);
        MV_DPRINT(("phy %d CONFIG_PORT_LAYER_TIMER1(0x%03X): 0x%08x\n", i,CONFIG_PORT_LAYER_TIMER1, READ_PORT_CONFIG_DATA(root, phy)));
        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_LINK_LAYER_TIMER);
        MV_DPRINT(("phy %d CONFIG_LINK_LAYER_TIMER(0x%03X): 0x%08x\n", i,CONFIG_LINK_LAYER_TIMER, READ_PORT_CONFIG_DATA(root, phy)));
    }
}
MV_VOID athena_dump_phy_vsr_reg_phy(void *root_p, void* phy_p)
{
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
    domain_phy *phy=(domain_phy *)phy_p;
    MV_U32 i=(root->base_phy_num +phy->asic_id);
        WRITE_PORT_VSR_ADDR(root, phy,VSR_IRQ_STATUS);
        MV_DPRINT(("phy %d VSR_IRQ_STATUS(0x%03X): 0x%08x\n", i, VSR_IRQ_STATUS, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_IRQ_MASK);
        MV_DPRINT(("phy %d VSR_IRQ_MASK(0x%03X): 0x%08x\n", i, VSR_IRQ_MASK, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_CONFIG);
        MV_DPRINT(("phy %d VSR_PHY_CONFIG(0x%03X): 0x%08x\n", i, VSR_PHY_CONFIG, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_STATUS);
        MV_DPRINT(("phy %d VSR_PHY_STATUS(0x%03X): 0x%08x\n", i, VSR_PHY_STATUS, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_COUNTER0);
        MV_DPRINT(("phy %d VSR_PHY_COUNTER0(0x%03X): 0x%08x\n", i, VSR_PHY_COUNTER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_COUNTER1);
        MV_DPRINT(("phy %d VSR_PHY_COUNTER1(0x%03X): 0x%08x\n", i, VSR_PHY_COUNTER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER_CTRL);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER_CTRL(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER_CTRL, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER_SELECT);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER_SELECT(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER_SELECT, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER0);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER0(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER1);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER1(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER2);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER2(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER2, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER3);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER3(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER3, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_ACT_LEN_CTRL);
        MV_DPRINT(("phy %d VSR_ACT_LEN_CTRL(0x%03X): 0x%08x\n", i, VSR_ACT_LEN_CTRL, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_TIMER);
        MV_DPRINT(("phy %d VSR_PHY_TIMER(0x%03X): 0x%08x\n", i, VSR_PHY_TIMER, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_OOB_TIMER0);
        MV_DPRINT(("phy %d VSR_OOB_TIMER0(0x%03X): 0x%08x\n", i, VSR_OOB_TIMER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_OOB_TIMER1);
        MV_DPRINT(("phy %d VSR_OOB_TIMER1(0x%03X): 0x%08x\n", i, VSR_OOB_TIMER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_ANALOG_TIMER);
        MV_DPRINT(("phy %d VSR_ANALOG_TIMER(0x%03X): 0x%08x\n", i, VSR_ANALOG_TIMER, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_RXDET_CONFIG0);
        MV_DPRINT(("phy %d VSR_PHY_RXDET_CONFIG0(0x%03X): 0x%08x\n", i, VSR_PHY_RXDET_CONFIG0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_RXDET_CONFIG1);
        MV_DPRINT(("phy %d VSR_PHY_RXDET_CONFIG1(0x%03X): 0x%08x\n", i, VSR_PHY_RXDET_CONFIG1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER0_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER0_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER0_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER1_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER1_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER1_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER2_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER2_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER2_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER3_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER3_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER3_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_0);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_0(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_1);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_1(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_2);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_2(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_2, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_3);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_3(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_3, READ_PORT_VSR_DATA(root, phy)));	
}
MV_VOID athena_dump_phy_vsr_reg(void *root_p)
{
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
    domain_phy *phy;
    MV_U32 i;
    for (i = 0; i < root->phy_num; i++) {
        phy=&root->phy[i];
        WRITE_PORT_VSR_ADDR(root, phy,VSR_IRQ_STATUS);
        MV_DPRINT(("phy %d VSR_IRQ_STATUS(0x%03X): 0x%08x\n", i, VSR_IRQ_STATUS, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_IRQ_MASK);
        MV_DPRINT(("phy %d VSR_IRQ_MASK(0x%03X): 0x%08x\n", i, VSR_IRQ_MASK, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_CONFIG);
        MV_DPRINT(("phy %d VSR_PHY_CONFIG(0x%03X): 0x%08x\n", i, VSR_PHY_CONFIG, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_STATUS);
        MV_DPRINT(("phy %d VSR_PHY_STATUS(0x%03X): 0x%08x\n", i, VSR_PHY_STATUS, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_COUNTER0);
        MV_DPRINT(("phy %d VSR_PHY_COUNTER0(0x%03X): 0x%08x\n", i, VSR_PHY_COUNTER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_COUNTER1);
        MV_DPRINT(("phy %d VSR_PHY_COUNTER1(0x%03X): 0x%08x\n", i, VSR_PHY_COUNTER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER_CTRL);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER_CTRL(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER_CTRL, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER_SELECT);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER_SELECT(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER_SELECT, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER0);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER0(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER1);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER1(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER2);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER2(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER2, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER3);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER3(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER3, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_ACT_LEN_CTRL);
        MV_DPRINT(("phy %d VSR_ACT_LEN_CTRL(0x%03X): 0x%08x\n", i, VSR_ACT_LEN_CTRL, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_TIMER);
        MV_DPRINT(("phy %d VSR_PHY_TIMER(0x%03X): 0x%08x\n", i, VSR_PHY_TIMER, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_OOB_TIMER0);
        MV_DPRINT(("phy %d VSR_OOB_TIMER0(0x%03X): 0x%08x\n", i, VSR_OOB_TIMER0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_OOB_TIMER1);
        MV_DPRINT(("phy %d VSR_OOB_TIMER1(0x%03X): 0x%08x\n", i, VSR_OOB_TIMER1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_ANALOG_TIMER);
        MV_DPRINT(("phy %d VSR_ANALOG_TIMER(0x%03X): 0x%08x\n", i, VSR_ANALOG_TIMER, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_RXDET_CONFIG0);
        MV_DPRINT(("phy %d VSR_PHY_RXDET_CONFIG0(0x%03X): 0x%08x\n", i, VSR_PHY_RXDET_CONFIG0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_RXDET_CONFIG1);
        MV_DPRINT(("phy %d VSR_PHY_RXDET_CONFIG1(0x%03X): 0x%08x\n", i, VSR_PHY_RXDET_CONFIG1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER0_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER0_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER0_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER1_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER1_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER1_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER2_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER2_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER2_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_EVENT_COUNTER3_TH);
        MV_DPRINT(("phy %d VSR_EVENT_COUNTER3_TH(0x%03X): 0x%08x\n", i, VSR_EVENT_COUNTER3_TH, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_0);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_0(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_0, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_1);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_1(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_1, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_2);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_2(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_2, READ_PORT_VSR_DATA(root, phy)));	
        WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_MODE_REG_3);
        MV_DPRINT(("phy %d VSR_PHY_MODE_REG_3(0x%03X): 0x%08x\n", i, VSR_PHY_MODE_REG_3, READ_PORT_VSR_DATA(root, phy)));	
    }
}
MV_VOID athena_dump_cmd_reg(void *root_p)
{
    pl_root *root = (pl_root *)root_p;
    MV_LPVOID mmio = root->mmio_base;
        /* command registor -- Monitor Data/Select */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_MONITOR_DATA_SELECT);
    MV_DPRINT(("CMD_MONITOR_DATA_SELECT(0x%X)=%08X.\n",  CMD_MONITOR_DATA_SELECT, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Application Error Configuration */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_APP_ERR_CONFIG);
    MV_DPRINT(("CMD_APP_ERR_CONFIG(0x%X)=%08X.\n",  CMD_APP_ERR_CONFIG, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Pending FIFO Control 0 */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_PEND_FIFO_CTRL0);
    MV_DPRINT(("CMD_PEND_FIFO_CTRL0(0x%X)=%08X.\n",  CMD_PEND_FIFO_CTRL0, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Host Control Status */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_HOST_CTRL_STS);
    MV_DPRINT(("CMD_HOST_CTRL_STS(0x%X)=%08X.\n",  CMD_HOST_CTRL_STS, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Host Write Data */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_HOST_WRITE_DATA);
    MV_DPRINT(("CMD_HOST_WRITE_DATA(0x%X)=%08X.\n",  CMD_HOST_WRITE_DATA, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Host Read Data */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_HOST_READ_DATA);
    MV_DPRINT(("CMD_HOST_READ_DATA(0x%X)=%08X.\n",  CMD_HOST_READ_DATA, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Host Address */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_HOST_ADDR);
    MV_DPRINT(("CMD_HOST_ADDR(0x%X)=%08X.\n",  CMD_HOST_ADDR, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* command registor -- Pending FIFO Control 1 */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_PEND_FIFO_CTRL1);
    MV_DPRINT(("CMD_PEND_FIFO_CTRL1(0x%X)=%08X.\n",  CMD_PEND_FIFO_CTRL1, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
        /* cmd port active register $i ((0x400-0x7ff) -- 1023*/
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_PORT_ACTIVE0);
    MV_DPRINT(("CMD_PORT_ACTIVE0 (0x%X)=%08X.\n",  CMD_PORT_ACTIVE0, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));

        /* SATA register set $i (0x800-0x9ff) task file data register -- 511*/
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_SATA_TFDATA0);
    MV_DPRINT(("CMD_SATA_TFDATA0 (0x%X)=%08X.\n",  CMD_SATA_TFDATA0, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));

        /* SATA register set $i (0xa00-0xbff) association reg -- 511 */
    MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_SATA_ASSOC0);
    MV_DPRINT(("CMD_SATA_ASSOC0  (0x%X)=%08X.\n",  CMD_SATA_ASSOC0, MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA)));
}
#endif
MV_VOID core_dump_common_reg(void *root_p)
{
#ifdef MV_DEBUG
    MV_DPRINT(("Print athena common register\n"));
    athena_dump_common_reg(root_p);
    MV_DPRINT(("Print athena cmd register\n"));
    athena_dump_cmd_reg(root_p);
    MV_DPRINT(("Print athena port config/data register\n"));
    athena_dump_port_config_reg(root_p);
    MV_DPRINT(("Print athena phy config/data register(VSR)\n"));
    athena_dump_phy_vsr_reg(root_p);
#endif
}
MV_VOID core_dump_phy_reg(void *root_p, void* phy_p)
{
#ifdef MV_DEBUG
    MV_DPRINT(("Print athena port config/data register\n"));
    if(root_p && phy_p)
    athena_dump_port_config_reg_phy(root_p, phy_p);
    MV_DPRINT(("Print athena phy config/data register(VSR)\n"));
    if(root_p && phy_p)
    athena_dump_phy_vsr_reg_phy(root_p, phy_p);
#endif
}
#ifdef SUPPORT_INTL_PARITY
MV_VOID core_sas_sata_intl_parity_enable(MV_LPVOID root_p, MV_U8 enable, MV_U8 isr_enable){
    MV_U32 reg=0;
    pl_root *root = (pl_root *)root_p;
    if(enable){
        reg = root->parity_enable_mask;
    }
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR_EN,reg);
    reg= MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR_EN);
    MV_PRINT("root%d mmio(%p) 0x%x %s COMMON_PORT_INTL_MEM_PERR_EN(0x%x) \n", root->root_id, root->mmio_base, COMMON_INTL_MEM_PARITY_ERR_EN, ((reg)?"Enable":"Disable"), reg);
    reg= MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR_EN);
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR, MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR));
    reg= MV_REG_READ_DWORD(root->mmio_base, COMMON_INTL_MEM_PARITY_ERR);
    reg =MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK);
    if(isr_enable){
        reg |= INT_MEM_PAR_ERR;
        root->queues[0].irq_mask |= INT_MEM_PAR_ERR;
        root->comm_irq_mask |= INT_MEM_PAR_ERR;
    }
    else{
        reg &= ~(INT_MEM_PAR_ERR);
        root->queues[0].irq_mask &= ~(INT_MEM_PAR_ERR);
        root->comm_irq_mask &= ~(INT_MEM_PAR_ERR);
    }
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK, reg);
}
#endif

#ifdef SUPPORT_DPP
MV_VOID core_sas_sata_dpp_enable(MV_LPVOID root_p, MV_U8 enable, MV_U8 odd, MV_U8 isr_enable){
    MV_U32 parity = 0, reg = 0;
    pl_root *root = (pl_root *)root_p;
    if(enable){
        if(odd)
            parity = (DPP_CTRL_DP_FERR_EN| DPP_CTRL_DP_PERR_EN| DPP_CTRL_DP_PAR_ODD);
        else
            parity = (DPP_CTRL_DP_FERR_EN| DPP_CTRL_DP_PERR_EN);
    }
    MV_PRINT("%s SATA/SAS Data Path Parity register Parity(0x%x)\n", ((enable)?"Enable":"Disable"), parity);
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_CTRL, parity);
    reg= MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_CTRL);
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS, 0);
    reg= MV_REG_READ_DWORD(root->mmio_base, COMMON_DATA_PATH_PARITY_STS);
    reg =MV_REG_READ_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK);
    if(isr_enable){
        reg |= INT_DP_PAR_ERR;
        root->comm_irq_mask |= INT_DP_PAR_ERR;
    }
    else{
        reg &= ~(INT_DP_PAR_ERR);
        root->comm_irq_mask &= ~(INT_DP_PAR_ERR);
    }
    MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK, reg);
//    MV_PRINT("mmio(%p) 0x%x Enable COMMON_IRQ_MASK(0x%x) \n", root->mmio_base, COMMON_IRQ_MASK, reg);

}
#endif
#if 0
MV_U32   g_PHY_Tuning[16] ={0x0B70, 0x0B70, 0x0B70, 0x0B70,\
						0x0B70, 0x0B70, 0x0B70, 0x0B70,\
						0x0B70,0x0B70, 0x0B70, 0x0B70,\
						0x0B70,0x0B70,0x0B70,0x0B70};
#endif
#ifdef ATHENA_MICRON_DETECT_WA
MV_VOID io_chip_init_registers_wa(MV_PVOID root_p, MV_U8 enable_wa)
{
	MV_LPVOID mmio;
	pl_root *root = (pl_root *)root_p;
	MV_U8 i,j;
	MV_U32 tmp;
	HBA_Info_Page hba_info_param;
	MV_U32 temp;
	domain_phy *phy;
	_MV_U64 u64_sas_addr;
	MV_U8 def_sas_addr[8] = {0x50,0x05,0x04,0x30,0x11,0xab,0x00,0x00};
	MV_U8 *sas_addr = (void*)&u64_sas_addr;
	core_extension *core = (core_extension *)root->core;
	MV_BOOLEAN phy_disable=MV_FALSE;
	Phy_Info_Page phy_info_param;
	MV_BOOLEAN phy_info_valid = MV_FALSE;	


	/* the first root will use 0x11ab0000, the second one will use 0x11ab0004 */
	def_sas_addr[7] = (MV_U8)root->base_phy_num;

	MV_CopyMemory(sas_addr, def_sas_addr, sizeof(u64_sas_addr) );
	mmio = root->mmio_base;
	core_set_chip_options(root);
	if (mv_nvram_init_phy_param(HBA_GetModuleExtension((root->core), MODULE_HBA), &phy_info_param))
		phy_info_valid = MV_TRUE;	
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
	tmp |= CONFIG_SAS_SATA_RST;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
	core_sleep_millisecond(core, 250); //Maybe able to shortent this to do in parallel


	/* reset control */
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, 0);

       MV_REG_WRITE_DWORD(root->mmio_base, 0xC00C, 0x5555);

	for (i=0; i<root->phy_num; i++) {
		phy = &root->phy[i];
		if (phy_info_valid){
			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen1 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Emphasis_Amp = 0x1;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp = 0x6;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}
			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen2 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Emphasis_Amp = 0x2;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp = 0x8;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;;
			}
			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen3 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Emphasis_Amp = 0x4;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp = 0xB;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}
			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen4 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Emphasis_Amp = 0x5;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp = 0x10;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}
			temp = (MV_U8)(*(MV_PU8)(&(phy_info_param.FFE_Control[root->base_phy_num+i])));
			if (temp == 0xFFL){
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select=0xE;
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x3;
			}
			temp = (MV_U8)(*(MV_PU8)(&(phy_info_param.PHY_Rate[root->base_phy_num+i])));
			if (temp == 0xFFL){
				phy_info_param.PHY_Rate[root->base_phy_num+i] = 0x3;//set default phy_rate=12Gbps
			}
			for(j=0; j<8; j++){
				sas_addr[j] = phy_info_param.SAS_Address[(root->base_phy_num+i)].b[j];				
			}
			U64_ASSIGN((*(MV_U64 *)sas_addr), MV_LE64_TO_CPU(*(MV_U64 *)sas_addr));			
	 	}else{
			// Gen1 1.5G
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Emphasis_Amp = 0x1;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp = 0x6;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			// Gen2 3G
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Emphasis_Amp = 0x2;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp = 0x8;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;;
			// Gen3 6G
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Emphasis_Amp = 0x4;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp = 0xB;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			// Gen4 12G
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Emphasis_Amp = 0x5;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp = 0x10;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			// FFE control
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select=0xE;
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x3;
			// 12G
				phy_info_param.PHY_Rate[root->base_phy_num+i] = 0x3;//set default phy_rate=12Gbps
		}
		if ((U64_COMPARE_U32((*(MV_U64 *)sas_addr), 0) == 0) ||
			(U64_COMP_U64(0xffffffffffffffffULL, (*(MV_U64 *)sas_addr)))) {
			U64_ASSIGN((*(MV_U64 *)sas_addr), 0x0000ab1130040550ULL);
		}

		/* Set Phy Id */
		mv_set_dev_info(root, phy);

		/* Set SAS Addr */
		mv_set_sas_addr(root, phy, sas_addr);
		set_12G_sas_setting_a0(root,phy, 1, 0, &phy_info_param);
	}

        if (HBA_CheckIsFlorence(root->core))
                core_sleep_millisecond(root->core, 400);
        else 
                core_sleep_millisecond(root->core, 400);
	if(enable_wa){
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
			tmp= READ_PORT_CONFIG_DATA(root, phy);
			WRITE_PORT_CONFIG_DATA(root, phy, tmp);
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
			WRITE_PORT_CONFIG_DATA(root, phy, 0);
		}

	/* reset CMD queue */
		tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
		tmp |= CONFIG_CMD_TBL_BE | CONFIG_DATA_BE;
		tmp &= ~CONFIG_OPEN_ADDR_BE;
		tmp |= CONFIG_RSPNS_FRAME_BE;
		tmp |= CONFIG_STP_STOP_ON_ERR;
		tmp	|=(INTRFC_PCIEA<<CONFIG_IO_CNTXT_INTRFC_SLCT_SHIFT);
		tmp	|=(INTRFC_PCIEA<<CONFIG_RCVD_FIS_LIST_IFC_SLCT_SHIFT);
		MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
		CORE_DPRINT(("COMMON_CONFIG (default) = 0x%x\n", tmp));
	
	/* assign command list address */


		MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR, 0);
		MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR_HI, 0);

	/* assign FIS address */
		MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR, root->rx_fis_dma.parts.low);
		MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR_HI, root->rx_fis_dma.parts.high);
	
	/* assign delivery queue address */
		tmp = 0;
		for (i = 0; i < root->queue_num; i++) {
			pl_queue *queue = &root->queues[i];
			WRITE_DLVRYQ_CONFIG(root, i, tmp);
			tmp = DELV_QUEUE_SIZE_MASK & queue->delv_q_size;
			tmp |= DELV_QUEUE_ENABLE;
			tmp |= DELV_QUEUE_RD_PTR_SHDW_EN;
			tmp	|= (INTRFC_PCIEA<<DELV_DLVRY_Q_INTRFC_SLCT_SHIFT);
			WRITE_DLVRYQ_CONFIG(root, i, tmp);
			WRITE_DLVRYQ_BASE_ADDR(root, i, queue->delv_q_dma.parts.low);
			WRITE_DLVRYQ_BASE_ADDR_HI(root, i, queue->delv_q_dma.parts.high);
			WRITE_DLVRYQ_RD_SHADOW_ADDR(root, i, queue->delv_q_shadow_dma.parts.low);
			WRITE_DLVRYQ_RD_SHADOW_ADDR_HI(root, i, queue->delv_q_shadow_dma.parts.high);
			/* write in a default value for completion queue pointer in memory */
			tmp = 0x3FFF;
			MV_CopyMemory(queue->cmpl_q_shadow, &tmp, 4);

			/* assign completion queue address */
			tmp = 0;
			WRITE_CMPLQ_CONFIG(root, i, tmp);
			tmp =  CMPL_QUEUE_SIZE_MASK & queue->cmpl_q_size;
			tmp |= CMPL_QUEUE_ENABLE;

			/* yuxl: attention only can post to queue0 */
			if (i != 0)
				tmp |= CMPL_QUEUE_DSBL_ATTN_POST;	
			tmp |= CMPL_QUEUE_WRT_PTR_SHDW_EN;
			tmp	|= INTRFC_PCIEA<<CMPL_CMPLT_Q_INTRFC_SLCT_SHIFT;
			WRITE_CMPLQ_CONFIG(root, i, tmp);
			WRITE_CMPLQ_BASE_ADDR(root, i, queue->cmpl_q_dma.parts.low);
			WRITE_CMPLQ_BASE_ADDR_HI(root, i, queue->cmpl_q_dma.parts.high);
			WRITE_CMPLQ_WR_SHADOW_ADDR(root, i, queue->cmpl_q_shadow_dma.parts.low);
			WRITE_CMPLQ_WR_SHADOW_ADDR_HI(root, i, queue->cmpl_q_shadow_dma.parts.high);
		}

	    /*
	    * interrupt coalescing may cause missing HW interrput in some case
	    * and the max count is 0x1ff, while our max slot is 0x200, it will
	    * make count 0
	    */
		for(i = 0; i < root->queue_num; i++) {
		    tmp = 0;
		    WRITE_IRQ_COAL_CONFIG(root, i, 0);
		    WRITE_IRQ_COAL_TIMEOUT(root, i, 0);
		}

		/* enable CMD/CMPL_Q/RESP mode */
		tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
		tmp |= CONTROL_EN_CMD_ISSUE|CONTROL_EN_SATA_RETRY;

		tmp |= CONTROL_FIS_RCV_EN;
		MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK, 0);
		
		for (i = 0; i < root->queue_num; i++) {
#ifdef SUPPORT_MSIX_INT
			if (msix_enabled(core)) {
				/* msix interrupt routing */
				tmp = root->queues[i].msix_idx & INT_MSIX_VECTOR_MASK;
				WRITE_IRQ_ROUTING(root, i, tmp);
				/* cmpl queue irq maks setting */
				WRITE_CMPLQ_RD_PTR(root, i, READ_CMPLQ_WR_PTR(root, i));/*avoid first unknown interrupt on each vector.*/
				WRITE_CMPLQ_IRQ_MASK(root, i, root->queues[i].irq_mask);
			}
#endif
			/* Handling I2C&GPIO INT only used roots[0]->queues[0] */
			WRITE_CMPLQ_IRQ_MASK(root, i, 0);
			
		}

		/* temp workaround for ocz v4/WD sata 'hold' exceed watchdog time */
		WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_WD_TIMER);
		tmp = 0x00200000;
		WRITE_ALL_PORT_CONFIG_DATA(root, tmp);

		/* Increase Nexus loss time */
		WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_SAS_CTRL0);
		tmp = READ_ALL_PORT_CONFIG_DATA(root);
		tmp |= 0x00005FFF;
		WRITE_ALL_PORT_CONFIG_DATA(root, tmp);

		/*extend nondata retry to 10 times for Akupa+4ST3500413AS seq write error: 0x90800000*/
		WRITE_ALL_PORT_CONFIG_ADDR(root,CONFIG_SAS_CTRL1);
		tmp = READ_ALL_PORT_CONFIG_DATA(root);
		//tmp = 0x0A0A; /*set nondata/data retry limit to 10*/
		tmp = 0x0F0A; 	//ASIC suggest value, otherwise (non_data_retry_lmt)0x0A will casue retry infinity.
	    WRITE_ALL_PORT_CONFIG_DATA(root,tmp);

		/* extent SMP link timeout value to its maximum to fix smp request waterdog timeout */
		WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_LINK_LAYER_TIMER);
		tmp =  READ_ALL_PORT_CONFIG_DATA(root);
		tmp |= 0xffff0000;
		WRITE_ALL_PORT_CONFIG_DATA(root, tmp);
	}
}
#endif
MV_VOID io_chip_init_registers(pl_root *root)
{
	MV_LPVOID mmio;
	MV_U8 i,j;
	MV_U32 tmp;
	HBA_Info_Page hba_info_param;
	MV_U32 temp;
	domain_phy *phy;
	_MV_U64 u64_sas_addr;
	MV_U8 def_sas_addr[8] = {0x50,0x05,0x04,0x30,0x11,0xab,0x00,0x00};
	MV_U8 *sas_addr = (void*)&u64_sas_addr;
	core_extension *core = (core_extension *)root->core;
	MV_BOOLEAN phy_disable=MV_FALSE;
	Phy_Info_Page phy_info_param;
	MV_BOOLEAN phy_info_valid = MV_FALSE;	


	/* the first root will use 0x11ab0000, the second one will use 0x11ab0004 */
	def_sas_addr[7] = (MV_U8)root->base_phy_num;

	MV_CopyMemory(sas_addr, def_sas_addr, sizeof(u64_sas_addr) );
	mmio = root->mmio_base;
	core_set_chip_options(root);
	if (mv_nvram_init_phy_param(HBA_GetModuleExtension((root->core), MODULE_HBA), &phy_info_param))
		phy_info_valid = MV_TRUE;	
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
	tmp |= CONFIG_SAS_SATA_RST;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
	core_sleep_millisecond(core, 250); //Maybe able to shortent this to do in parallel


	/* reset control */
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, 0);

//    WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_WD_TIMER);
//    WRITE_ALL_PORT_CONFIG_DATA(root, 0x000FFFFF); // for about 1sec
   // WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_SAS_CTRL0);
  //  WRITE_ALL_PORT_CONFIG_DATA(root, 0x019010FF); // for 256 ms

#if defined(ATHENA_FPGA_WORKAROUND)
    WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_WORKARPUND_REG0);
    WRITE_ALL_PORT_VSR_DATA(root, 0xD000);
    WRITE_ALL_PORT_VSR_ADDR(root, VSR_PHY_WORKARPUND_REG1);
    WRITE_ALL_PORT_VSR_DATA(root, 0x4000);
    set_phy_rate(root, phy, 3);
#elif defined(ATHENA_A0_WORKAROUND)
// SAS 12 G phy setting
//    set_12G_sas_setting(root, NULL, 1, phy_disable);
//    core_sleep_millisecond(core, 400); //Maybe able to shortent this to do in parallel
    //Workaround
    //Can you please try the PI issue by setting register R2C00C to 0x5555? This change the memory access time in the GP register.
    MV_REG_WRITE_DWORD(root->mmio_base, 0xC00C, 0x5555);
#else
    set_phy_rate(root, phy, 0);
#endif

	for (i=0; i<root->phy_num; i++) {
		phy = &root->phy[i];
	// set phy tuning register if PHY Info Page if valid
		if (phy_info_valid){

			/*temp = (MV_U32)(*(MV_PU32)(&(phy_info_param.PHY_Tuning[root->base_phy_num+i])));

			if (temp == 0xFFFFFFFFL){

				phy_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Emphasis_Amp = 0x3;

				phy_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Amp = 0x19;

				phy_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;

			}*/



			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen1 != 0x1 ){

				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Emphasis_Amp = 0x1;

				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp = 0x6;

				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;

			}

			

			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen2 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Emphasis_Amp = 0x2;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp = 0x8;

				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}

			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen3 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Emphasis_Amp = 0x4;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp = 0xB;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}

			if (phy_info_param.PHY_Tuning_State[root->base_phy_num+i].Gen4 != 0x1 ){
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Emphasis_Amp = 0x5;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp = 0x10;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}

			temp = (MV_U8)(*(MV_PU8)(&(phy_info_param.FFE_Control[root->base_phy_num+i])));
			if (temp == 0xFFL){
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select=0xE;
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x3;
			}

			temp = (MV_U8)(*(MV_PU8)(&(phy_info_param.PHY_Rate[root->base_phy_num+i])));
			if (temp == 0xFFL){
				phy_info_param.PHY_Rate[root->base_phy_num+i] = 0x3;//set default phy_rate=12Gbps

			}
	 	}else{
			{
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Emphasis_Amp = 0x1;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp = 0x6;
				phy_info_param.PHY_Tuning_Gen1[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}
			
			{
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Emphasis_Amp = 0x2;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp = 0x8;
				phy_info_param.PHY_Tuning_Gen2[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;;
			}

			{
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Emphasis_Amp = 0x4;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp = 0xB;
				phy_info_param.PHY_Tuning_Gen3[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}

			{
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Emphasis_Amp = 0x5;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp = 0x10;
				phy_info_param.PHY_Tuning_Gen4[root->base_phy_num+i].Trans_Amp_Adjust = 0x1;
			}
			{
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select=0xE;
				phy_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x3;
			}

			{
				phy_info_param.PHY_Rate[root->base_phy_num+i] = 0x3;//set default phy_rate=12Gbps

			}
		}
		if (phy_info_valid){
			for(j=0; j<8; j++){
				sas_addr[j] = phy_info_param.SAS_Address[(root->base_phy_num+i)].b[j];				
			}
			U64_ASSIGN((*(MV_U64 *)sas_addr), MV_LE64_TO_CPU(*(MV_U64 *)sas_addr));			
		}
		if ((U64_COMPARE_U32((*(MV_U64 *)sas_addr), 0) == 0) ||
			(U64_COMP_U64(0xffffffffffffffffULL, (*(MV_U64 *)sas_addr)))) {
			U64_ASSIGN((*(MV_U64 *)sas_addr), 0x0000ab1130040550ULL);
		}
		/* Set Phy Id */
		mv_set_dev_info(root, phy);
		/* Set SAS Addr */
		mv_set_sas_addr(root, phy, sas_addr);
		set_12G_sas_setting_a0(root,phy, 1, phy_disable, &phy_info_param);
	}

	core_sleep_millisecond(root->core, 400);

	for (i = 0; i < root->phy_num; i++) {
		phy = &root->phy[i];

		/* Unnessary to poll for Hard Reset Done here since we don't hard reset
		In this function. */
		//while((READ_PORT_PHY_CONTROL(root, phy) & SCTRL_PHY_HARD_RESET_SEQ) && (loop++<10)){
		//	core_sleep_millisecond(root->core, 10);
		//}
		
		/* reset irq */

	        WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
	        tmp= READ_PORT_CONFIG_DATA(root, phy);
	        WRITE_PORT_CONFIG_DATA(root, phy, tmp);

		/* enable phy change interrupt and broadcast change */
		  tmp = IRQ_PHY_RDY_CHNG_MASK | IRQ_BRDCST_CHNG_RCVD_MASK |
			IRQ_UNASSOC_FIS_RCVD_MASK | IRQ_SIG_FIS_RCVD_MASK |
			IRQ_ASYNC_NTFCN_RCVD_MASK | IRQ_PHY_RDY_CHNG_1_TO_0;
//			phy->phy_irq_mask = tmp;

                tmp|=IRQ_DMA_PEX_TO|IRQ_PRD_BC_ERR|IRQ_STP_SATA_PHY_DEC_ERR_MASK;
                //tmp|=IRQ_DMA_PEX_TO|IRQ_PRD_BC_ERR;
                phy->phy_irq_mask = tmp;
                WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
                WRITE_PORT_CONFIG_DATA(root, phy, tmp);

	#ifndef DISABLE_VSR_IRQ_PHY_TIMEOUT
	/* HBA always trigger phy timeout interrupt when connect to AIC expander, temporary disable the interrupt for Linux  */
		WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
		tmp = VSR_IRQ_PHY_TIMEOUT;
		WRITE_PORT_VSR_DATA(root, phy, tmp);
	#endif

#ifdef SUPPORT_DIRECT_SATA_SSU
		WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
		tmp = READ_PORT_VSR_DATA(root, phy);
		tmp |= VSR_SATA_SPIN_HOLD;
		WRITE_PORT_VSR_DATA(root, phy, tmp);
#endif
	}

	/* reset CMD queue */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
	tmp |= CONFIG_CMD_TBL_BE | CONFIG_DATA_BE;
	tmp &= ~CONFIG_OPEN_ADDR_BE;
	tmp |= CONFIG_RSPNS_FRAME_BE;
	tmp |= CONFIG_STP_STOP_ON_ERR;
#if defined(TEST_CONTEXT_IN_SRAM)
	tmp	|=(INTRFC_SRAM<<CONFIG_IO_CNTXT_INTRFC_SLCT_SHIFT);
#else
	tmp	|=(INTRFC_PCIEA<<CONFIG_IO_CNTXT_INTRFC_SLCT_SHIFT);
#endif

#if defined(TEST_RECV_FIS_IN_SRAM)
	tmp	|=(INTRFC_SRAM<<CONFIG_RCVD_FIS_LIST_IFC_SLCT_SHIFT);
#else
	tmp	|=(INTRFC_PCIEA<<CONFIG_RCVD_FIS_LIST_IFC_SLCT_SHIFT);
#endif
	MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
	CORE_DPRINT(("COMMON_CONFIG (default) = 0x%x\n", tmp));
	
	/* assign command list address */
#if 1 // ndef ATHENA_FPGA_WORKAROUND
	CORE_DPRINT(("cmd_struct_wrapper addr= 0x%x\n", root->cmd_struct_wrapper[0].phy.parts.low));

	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR, 0);
	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR_HI, 0);

//	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR, root->cmd_struct_wrapper[0].phy.parts.low);
//	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR_HI, root->cmd_struct_wrapper[0].phy.parts.high);
	CORE_DPRINT(("COMMON_LST_ADDR 0x%x\n", MV_REG_READ_DWORD(mmio, COMMON_LST_ADDR)));
#endif
	/* assign FIS address */
	MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR, root->rx_fis_dma.parts.low);
	MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR_HI,
		root->rx_fis_dma.parts.high);
	
	/* assign delivery queue address */
	tmp = 0;
	for (i = 0; i < root->queue_num; i++) {
		pl_queue *queue = &root->queues[i];
		WRITE_DLVRYQ_CONFIG(root, i, tmp);
		tmp = DELV_QUEUE_SIZE_MASK & queue->delv_q_size;
		tmp |= DELV_QUEUE_ENABLE;
		tmp |= DELV_QUEUE_RD_PTR_SHDW_EN;
#if defined(TEST_DEL_Q_IN_SRAM)
		tmp |= (INTRFC_SRAM << DELV_DLVRY_Q_INTRFC_SLCT_SHIFT);
#else
		tmp	|= (INTRFC_PCIEA<<DELV_DLVRY_Q_INTRFC_SLCT_SHIFT);
#endif
		WRITE_DLVRYQ_CONFIG(root, i, tmp);
		WRITE_DLVRYQ_BASE_ADDR(root, i, queue->delv_q_dma.parts.low);
		WRITE_DLVRYQ_BASE_ADDR_HI(root, i, queue->delv_q_dma.parts.high);
		WRITE_DLVRYQ_RD_SHADOW_ADDR(root, i, queue->delv_q_shadow_dma.parts.low);
		WRITE_DLVRYQ_RD_SHADOW_ADDR_HI(root, i, queue->delv_q_shadow_dma.parts.high);
		/* write in a default value for completion queue pointer in memory */
		tmp = 0x3FFF;
		MV_CopyMemory(queue->cmpl_q_shadow, &tmp, 4);

		/* assign completion queue address */
		tmp = 0;
		WRITE_CMPLQ_CONFIG(root, i, tmp);
		tmp =  CMPL_QUEUE_SIZE_MASK & queue->cmpl_q_size;
		tmp |= CMPL_QUEUE_ENABLE;

		/* yuxl: attention only can post to queue0 */
		if (i != 0)
			tmp |= CMPL_QUEUE_DSBL_ATTN_POST;
		
		tmp |= CMPL_QUEUE_WRT_PTR_SHDW_EN;
#if defined(TEST_CMPLT_Q_IN_SRAM)
		tmp |= (INTRFC_SRAM << CMPL_CMPLT_Q_INTRFC_SLCT_SHIFT);
#else
		tmp	|= INTRFC_PCIEA<<CMPL_CMPLT_Q_INTRFC_SLCT_SHIFT;
#endif
		WRITE_CMPLQ_CONFIG(root, i, tmp);
		WRITE_CMPLQ_BASE_ADDR(root, i, queue->cmpl_q_dma.parts.low);
		WRITE_CMPLQ_BASE_ADDR_HI(root, i, queue->cmpl_q_dma.parts.high);
		WRITE_CMPLQ_WR_SHADOW_ADDR(root, i, queue->cmpl_q_shadow_dma.parts.low);
		WRITE_CMPLQ_WR_SHADOW_ADDR_HI(root, i, queue->cmpl_q_shadow_dma.parts.high);
	}

	/* enable CMD/CMPL_Q/RESP mode */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
	tmp |= CONTROL_EN_CMD_ISSUE|CONTROL_EN_SATA_RETRY;

	tmp |= CONTROL_FIS_RCV_EN;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);

	/* enable completion queue interrupt */
	tmp = INT_PORT_MASK;
	tmp |= INT_NON_SPCFC_NCQ_ERR;
	tmp |= INT_CMPLQ_NOT_EMPTY;
	/* fs TODO: enable other interrupts */
	root->comm_irq_mask = tmp;
	for (i = 0; i < root->queue_num; i++) {
		if (i == 0) {
			/* enable COMMON interrupt in queue0, avoid unnecessary concurrency */
#if 0
			tmp = INT_PORT_MASK;
#else
			tmp = INT_PORT_MASK | INT_MEM_PAR_ERR |
					INT_DP_PAR_ERR | INT_SYS_ERR_IRQ;
#endif
		} else {
			tmp = 0;
		}
		tmp |= INT_NON_SPCFC_NCQ_ERR | INT_CMPLQ_NOT_EMPTY;
		root->queues[i].irq_mask = tmp;
#ifdef SUPPORT_MSIX_INT
		if (msix_enabled(core)) {
			/* msix interrupt routing */
			tmp = root->queues[i].msix_idx & INT_MSIX_VECTOR_MASK;
			WRITE_IRQ_ROUTING(root, i, tmp);

			/* cmpl queue irq maks setting */
			WRITE_CMPLQ_RD_PTR(root, i, READ_CMPLQ_WR_PTR(root, i));/*avoid first unknown interrupt on each vector.*/
			WRITE_CMPLQ_IRQ_MASK(root, i, root->queues[i].irq_mask);
		}
#endif
		/* Handling I2C&GPIO INT only used roots[0]->queues[0] */
		if ((i == 0) && (root->root_id == 0)) {
#ifdef SUPPORT_I2C
			root->queues[i].irq_mask |= INT_I2C_IRQ;
#endif
#ifdef SUPPORT_ACTIVE_CABLE
			root->queues[i].irq_mask |= INT_GPIO_IRQ;
#endif
			WRITE_CMPLQ_IRQ_MASK(root, i, root->queues[i].irq_mask);
		}
    /*
    * interrupt coalescing may cause missing HW interrput in some case
    * and the max count is 0x1ff, while our max slot is 0x200, it will
    * make count 0
    */
		{
			tmp = 0;
#if defined(DISABLE_HW_CC) || defined(ATHENA_A1_HW_CC_WA)
			WRITE_IRQ_COAL_CONFIG(root, i, 0);
			WRITE_IRQ_COAL_TIMEOUT(root, i, 0);
#else
			if (root->slot_count_support > 0x1ff )
			    tmp =  INT_COAL_COUNT_MASK & 0x1ff;
			else
			    tmp =  INT_COAL_COUNT_MASK & root->slot_count_support;
			tmp |= INT_COAL_ENABLE;

			WRITE_IRQ_COAL_CONFIG(root, i, tmp);
			tmp = 0x10100; //tmp = 0x1001B;
			WRITE_IRQ_COAL_TIMEOUT(root, i, tmp);
#endif
    	}
	}
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMPLQ_IRQN_MASK, root->comm_irq_mask);

#if 0 //def ATHENA_SATA_PHY_UNSTABLE_WA
    WRITE_ALL_PORT_CONFIG_ADDR(root,CONFIG_PORT_SERIAL_CTRL_STS);
    tmp = READ_ALL_PORT_CONFIG_DATA(root);
    tmp |= SCTRL_PHY_HARD_RESET_SEQ;
    WRITE_ALL_PORT_CONFIG_DATA(root,tmp);
    core_sleep_millisecond(core, 300); 
#endif
#ifdef SUPPORT_INTL_PARITY

#ifdef IGNORE_FIRST_PARITY_ERR
    root->parity_enable_mask = (INTER_MEM_PARITY_ERR_MASK&0xFFFF00FF);
#else
    root->parity_enable_mask = INTER_MEM_PARITY_ERR_MASK;
    core_sas_sata_intl_parity_enable(root, MV_TRUE, MV_TRUE);
#endif
#endif
#ifdef SUPPORT_DPP
    core_sas_sata_dpp_enable(root,MV_TRUE, MV_FALSE, MV_TRUE);
#endif 

	/* temp workaround for ocz v4/WD sata 'hold' exceed watchdog time */
	WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_WD_TIMER);
	tmp = 0x00200000;
	WRITE_ALL_PORT_CONFIG_DATA(root, tmp);

	/* Increase Nexus loss time */
	WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_SAS_CTRL0);
	tmp = READ_ALL_PORT_CONFIG_DATA(root);
	tmp |= 0x00005FFF;
	WRITE_ALL_PORT_CONFIG_DATA(root, tmp);

	/*extend nondata retry to 10 times for Akupa+4ST3500413AS seq write error: 0x90800000*/
	WRITE_ALL_PORT_CONFIG_ADDR(root,CONFIG_SAS_CTRL1);
	tmp = READ_ALL_PORT_CONFIG_DATA(root);
	//tmp = 0x0A0A; /*set nondata/data retry limit to 10*/
	tmp = 0x0F0A; 	//ASIC suggest value, otherwise (non_data_retry_lmt)0x0A will casue retry infinity.
    WRITE_ALL_PORT_CONFIG_DATA(root,tmp);

	/* extent SMP link timeout value to its maximum to fix smp request waterdog timeout */
	WRITE_ALL_PORT_CONFIG_ADDR(root, CONFIG_LINK_LAYER_TIMER);
	tmp =  READ_ALL_PORT_CONFIG_DATA(root);
	tmp |= 0xffff0000;
	WRITE_ALL_PORT_CONFIG_DATA(root, tmp);
}

void update_phy_info(pl_root *root, domain_phy *phy)
{
	MV_U32 reg;
	MV_U8 speed_gen=0;
	MV_U8 phy_sts=0;

	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_STATUS);
	reg = READ_PORT_VSR_DATA(root, phy);
	//CORE_DPRINT(("PHY STATUS!!!multiplexing or not is 0x%x\n", reg));
	CORE_EVENT_PRINT(("phy %d VSR_PHY_STATUS = 0x%x.\n", \
		(root->base_phy_num+phy->id), reg));
	speed_gen= (MV_U8)((reg>>14) & 0x3);
	reg = (MV_U8)((reg & VSR_PHY_STATUS_MASK) >> 16) & 0xff;
	phy_sts = (MV_U8)reg;
	switch(reg)
	{
	case VSR_PHY_STATUS_SAS_RDY:
		phy->type = PORT_TYPE_SAS;
		break;
	case VSR_PHY_STATUS_HR_RDY:
	default:
		phy->type = PORT_TYPE_SATA;
		break;
	}
	if(phy_sts == VSR_PHY_STATUS_SAS_RDY || phy_sts ==VSR_PHY_STATUS_HR_RDY){
		switch(speed_gen){
			case 0:
				CORE_EVENT_PRINT(("phy %d is link on  1.5G. phy sts(0x%x)\n", (root->base_phy_num+phy->id), phy_sts));
				break;
			case 1:
				CORE_EVENT_PRINT(("phy %d is link on  3.0G. phy sts(0x%x)\n", (root->base_phy_num+phy->id), phy_sts));
				break;
			case 2:
				CORE_EVENT_PRINT(("phy %d is link on  6.0G. phy sts(0x%x)\n", (root->base_phy_num+phy->id), phy_sts));
				break;
			case 3:
				CORE_EVENT_PRINT(("phy %d is link on 12.0G. phy sts(0x%x)\n", (root->base_phy_num+phy->id), phy_sts));
				break;
			}
	}else{
		CORE_EVENT_PRINT(("phy %d is no link ready. phy sts(0x%x)\n", (root->base_phy_num+phy->id), phy_sts));
	}
	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	phy->dev_info = (reg & 0xff) << 24; /* Phy ID */
 	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	/* 10000008 type(28..30), Target(16..19), Init(8..11) */
	phy->dev_info |= ((reg & 0x70) >> 4) +
		((reg & 0x0f000000) >> 8) +
		((reg & 0x0f0000) >> 8);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
	phy->dev_sas_addr.parts.low =
		CPU_TO_BIG_ENDIAN_32(READ_PORT_CONFIG_DATA(root, phy));

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
	phy->dev_sas_addr.parts.high =
		CPU_TO_BIG_ENDIAN_32(READ_PORT_CONFIG_DATA(root, phy));

	WRITE_PORT_CONFIG_ADDR(root, phy,CONFIG_PORT_SERIAL_CTRL_STS);
	//            tmp = READ_PORT_CONFIG_DATA(root, phy);

	//    WRITE_PORT_VSR_ADDR(root, phy,VSR_PHY_STATUS);
	phy->phy_status = READ_PORT_CONFIG_DATA(root, phy); //READ_PORT_VSR_DATA(root, phy);
	CORE_EVENT_PRINT(("phy %d phy_status = 0x%x.\n", \
		(root->base_phy_num+phy->id), phy->phy_status));

	if (phy->phy_status & SCTRL_PHY_READY_MASK)
	{
		if (phy->type & PORT_TYPE_SAS)
		{
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME5);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			phy->att_dev_info = (reg & 0xff) << 24; /* phy id */

 			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME0);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			/* 10000008 type(28..30), Target(16..19), Init(8..11) */
			phy->att_dev_info |= ((reg & 0x70) >> 4) +
				((reg & 0x0f000000) >> 8) +
				((reg & 0x0f0000) >> 8);

			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME4);
			phy->att_dev_sas_addr.parts.low =
				MV_BE32_TO_CPU(READ_PORT_CONFIG_DATA(root, phy));

			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME3);
			phy->att_dev_sas_addr.parts.high =
				MV_BE32_TO_CPU(READ_PORT_CONFIG_DATA(root, phy));
			/* SAS address is stored as BE, so ... */
			phy->att_dev_sas_addr.parts.low = MV_CPU_TO_LE32(phy->att_dev_sas_addr.parts.low);
			phy->att_dev_sas_addr.parts.high = MV_CPU_TO_LE32(phy->att_dev_sas_addr.parts.high);
		} else {
			phy->att_dev_info = 0;
			phy->att_dev_sas_addr.parts.low = 0;
			phy->att_dev_sas_addr.parts.high = 0;
		}

#ifdef NOT_SPINUP_WORKAROUND
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
		WRITE_PORT_CONFIG_DATA(root, phy, 0x04);
#endif
	} else {
#ifdef NOT_SPINUP_WORKAROUND
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
		WRITE_PORT_CONFIG_DATA(root, phy, 0);
#endif
	}

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_STAT);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	phy->irq_status |= (reg & phy->phy_irq_mask);

    WRITE_PORT_VSR_ADDR(root, phy,VSR_IRQ_STATUS);
    reg = READ_PORT_VSR_DATA(root, phy);

	if (reg & VSR_IRQ_PHY_TIMEOUT) {
		WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);
		CORE_DPRINT(("Has VSR_IRQ_PHY TIMEOUT: %08X.\n",reg));
#ifndef DISABLE_VSR_IRQ_PHY_TIMEOUT		
		phy->irq_status |= IRQ_PHY_RDY_CHNG_1_TO_0;
#endif
		WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_STATUS);
		reg = READ_PORT_VSR_DATA(root, phy);
		reg = (MV_U8)((reg >> 16) & 0x3f);
		CORE_EVENT_PRINT(("PHY State Machine Timeout. "\
				  "PHY Status 0x%x.\n", reg));
	}

	CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", \
		(root->base_phy_num+phy->id), phy->irq_status));
}


#define mv_enable_msi(_core)                                               \
{                                                               \
             MV_U32 temp;                                                  \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_CMD);        \
             temp |= MV_PCI_INT_DIS;                                       \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_CMD);        \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_MSI_CTRL);   \
             temp |= MV_PCI_MSI_EN;                                        \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_MSI_CTRL);  \
}

#define mv_enable_msix(_core)                                               \
{                                                               \
             MV_U32 temp;                                                  \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_CMD);        \
             temp |= MV_PCI_INT_DIS;                                       \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_CMD);        \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_MSIX_CTRL);   \
             temp |= MV_PCI_MSIX_EN;                                        \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_MSIX_CTRL);  \
}

MV_VOID controller_init(core_extension *core)
{
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;

#ifdef ATHENA_FPGA_WORKAROUND
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG0);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL_DATA, 0xD000);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG1);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL_DATA,0x4000);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_CONFIG);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL_DATA,0x001F);
        core_sleep_millisecond(core, 250);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG2);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL_DATA, 0x0A80);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL, VSR_PHY_WORKARPUND_REG3);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_ALL_PHY_CONTROL_DATA, 0x08A7);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_CONFIG_ADDR0, CONFIG_SAS_CTRL0);
        MV_REG_WRITE_DWORD(((MV_PU8)mmio+MV_IO_CHIP_REGISTER_BASE),COMMON_PORT_CONFIG_DATA0, 0x00320FFF);
#else
	tmp = MV_PCI_READ_DWORD(core, MV_PCI_REG_CMD);
	tmp |= MV_PCI_DEV_EN;
	MV_PCI_WRITE_DWORD(core, tmp, MV_PCI_REG_CMD);
#endif

	core->irq_mask = INT_MAP_SAS;
#ifdef HARDWARE_XOR
	core->irq_mask |= INT_MAP_XOR;
#endif
#ifdef SUPPORT_I2C
	core->irq_mask |= INT_MAP_TWSI;
#endif
#ifdef DEBUG_PCI_E_ERROR
	core->irq_mask |= INT_MAP_PCIE_ERR;
#endif
#ifdef SUPPORT_SECURITY_KEY_RECORDS
	core->irq_mask |= INT_MAP_CRYPTO_MGMT;
#endif
#ifdef SUPPORT_ACTIVE_CABLE
	core->irq_mask |= INT_MAP_GPIO;
#endif

	if (msi_enabled(core)) {
		mv_enable_msi(core);
	} else if (msix_enabled(core)){
		mv_enable_msix(core);
		tmp = MV_REG_READ_DWORD(mmio, MSIX_STATE_MACHINE_CTRL);
		tmp |= MSIX_STATE_MACHINE_CTRL_MSIX_SM_EN;
		tmp |= MSIX_STATE_MACHINE_CTRL_MSG_EN_F0;
		MV_REG_WRITE_DWORD(mmio, MSIX_STATE_MACHINE_CTRL, tmp);

		core->irq_mask |= INT_MAP_MSIX;
    }
	/* for vanir and frey, CPU_MAIN_INT_CAUSE_REG is read only */
        //tmp = MV_REG_READ_DWORD(mmio, CPU_MAIN_INT_CAUSE_REG);
	//MV_REG_WRITE_DWORD(mmio, CPU_MAIN_INT_CAUSE_REG, tmp);

	/* set main IRQ interrupt mask */
#ifdef SUPPORT_ROC
	MV_REG_WRITE_DWORD(mmio, CPU_MAIN_IRQ_MASK_REG,
		(MV_REG_READ_DWORD(mmio, CPU_MAIN_IRQ_MASK_REG) | core->irq_mask));
#else
	MV_REG_WRITE_DWORD(mmio, CPU_MAIN_IRQ_MASK_REG, core->irq_mask);
#endif

#ifdef SUPPORT_SECURITY_KEY_RECORDS
	tmp =MV_REG_READ_DWORD(core->mmio_base, CRYPTO_GLOBAL_ZERIOZATION_CTRL);
	tmp|= CRYPTO_ZERI_DEBUG_MODE;
	MV_REG_WRITE_DWORD(core->mmio_base, CRYPTO_GLOBAL_ZERIOZATION_CTRL, tmp);

	tmp =MV_REG_READ_DWORD(core->mmio_base, CRYPTO_GLOBAL_SECURITY_INT_MASK);
	tmp|= (CRYPTO_INT_PNDNG_KEY_NOT_VLD| CRYPTO_INT_VAULT_OP_ERR| CRYPTO_INT_VAULT_OP_CMPLT | CRYPTO_INT_VLT_ZEROIZED| CRYPTO_INT_UNWRP_ENGN_0_ZEROIZED);
	MV_REG_WRITE_DWORD(core->mmio_base, CRYPTO_GLOBAL_SECURITY_INT_MASK, tmp);
#endif

	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_ADDR, 0x00004840);
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA, MV_REG_READ_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA));
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_ADDR, 0x00004848);
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA, MV_REG_READ_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA));

#if defined(ATHENA_Z2_WORKAROUND) || defined(ATHENA_A0_WORKAROUND)
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_ADDR, ATHENA_A0_WORKAROUND_REG0); // 2nd read failed.
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA, 0); // 2nd read failed.
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_ADDR, 0x00006208); // Debug mux Control
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA, 0xC0000000); // bit[30] to 1 to disable check
#endif
#if defined(DISABLE_ASPM)
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_ADDR, 0xD0); // 2nd read failed.
	MV_REG_WRITE_DWORD(core->mmio_base, SOC_ACCESS_INDEX_DATA, 0x83); // 2nd read failed.
#endif
}

void core_set_cmd_header_selector(mv_command_header *cmd_header)
{
#ifdef TEST_OPEN_ADDR_IN_SRAM
	cmd_header->interface_select = (INTRFC_SRAM << 8);
#else
	cmd_header->interface_select = (INTRFC_PCIEA << 8);
#endif
#ifdef TEST_STATUS_BUF_IN_SRAM
	cmd_header->interface_select |= (INTRFC_SRAM << 16);
#else
	cmd_header->interface_select |= (INTRFC_PCIEA << 16);
#endif
#ifdef TEST_PRD_TABLE_IN_SRAM
	cmd_header->interface_select |= (INTRFC_SRAM << 24);
#else
	cmd_header->interface_select |= (INTRFC_PCIEA << 24);
#endif

#ifdef TEST_SECURITY_KEY_IN_SRAM
	cmd_header->security_key_interface_select = INTRFC_SRAM;
#else
	cmd_header->security_key_interface_select =CS_INTRFC_SECURITY_KEY;
#endif

}

void map_phy_id(pl_root *root) {}

MV_BOOLEAN core_reset_controller(core_extension *core){return MV_TRUE;}

extern MV_BOOLEAN ses_state_machine( MV_PVOID enc_p );
MV_VOID core_init_handlers(core_extension *core)
{

	core->handlers[HANDLER_SATA].init_handler = sata_device_state_machine;
	core->handlers[HANDLER_SATA].verify_command = sata_verify_command;
	core->handlers[HANDLER_SATA].prepare_command = sata_prepare_command;
	core->handlers[HANDLER_SATA].send_command = sata_send_command;
	core->handlers[HANDLER_SATA].process_command = sata_process_command;
	core->handlers[HANDLER_SATA].error_handler = sata_error_handler;

	core->handlers[HANDLER_SATA_PORT].init_handler = sata_port_state_machine;

	core->handlers[HANDLER_PM].init_handler = pm_state_machine;
	core->handlers[HANDLER_PM].verify_command = pm_verify_command;
	core->handlers[HANDLER_PM].prepare_command = pm_prepare_command;
	core->handlers[HANDLER_PM].send_command = pm_send_command;
	core->handlers[HANDLER_PM].process_command = pm_process_command;
	core->handlers[HANDLER_PM].error_handler = NULL;

	core->handlers[HANDLER_SSP].init_handler = sas_init_state_machine;
	core->handlers[HANDLER_SSP].verify_command = ssp_verify_command;
	core->handlers[HANDLER_SSP].prepare_command = ssp_prepare_command;
	core->handlers[HANDLER_SSP].send_command = ssp_send_command;
	core->handlers[HANDLER_SSP].process_command = ssp_process_command;
	core->handlers[HANDLER_SSP].error_handler = ssp_error_handler;

	core->handlers[HANDLER_SMP].init_handler = exp_state_machine;
	core->handlers[HANDLER_SMP].verify_command = smp_verify_command;
	core->handlers[HANDLER_SMP].prepare_command = smp_prepare_command;
	core->handlers[HANDLER_SMP].send_command = smp_send_command;
	core->handlers[HANDLER_SMP].process_command = smp_process_command;
	core->handlers[HANDLER_SMP].error_handler = NULL;

	core->handlers[HANDLER_STP].init_handler = sata_device_state_machine;
	core->handlers[HANDLER_STP].verify_command = sata_verify_command;
	core->handlers[HANDLER_STP].prepare_command = stp_prepare_command;
	core->handlers[HANDLER_STP].send_command = sata_send_command;
	core->handlers[HANDLER_STP].process_command = stp_process_command;
	core->handlers[HANDLER_STP].error_handler = sata_error_handler;
#ifdef SUPPORT_SES
	core->handlers[HANDLER_ENC].init_handler = ses_state_machine;
	core->handlers[HANDLER_ENC].verify_command = ssp_verify_command;
	core->handlers[HANDLER_ENC].prepare_command = ssp_prepare_command;
	core->handlers[HANDLER_ENC].send_command = ssp_send_command;
	core->handlers[HANDLER_ENC].process_command = ssp_process_command;
	core->handlers[HANDLER_ENC].error_handler = NULL;
#endif

	//core->handlers[HANDLER_API].init_handler =
	core->handlers[HANDLER_API].verify_command = api_verify_command;
	//core->handlers[HANDLER_API].prepare_command =
	//core->handlers[HANDLER_API].send_command = api_send_command;
	//core->handlers[HANDLER_API].process_command =
	//core->handlers[HANDLER_API].error_handler =
#ifdef SUPPORT_I2C
	core->handlers[HANDLER_I2C].init_handler = i2c_device_state_machine;
	//core->handlers[HANDLER_I2C].verify_command = i2c_send_command;
	//core->handlers[HANDLER_I2C].prepare_command =
	//core->handlers[HANDLER_I2C].send_command =
	//core->handlers[HANDLER_I2C].process_command =
	//core->handlers[HANDLER_I2C].error_handler =
#endif
}

