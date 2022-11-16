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
#include "core_type.h"
#include "com_dbg.h"
#include "com_util.h"
#include "com_u64.h"

#include "core_hal.h"
#include "core_manager.h"
#include "core_init.h"
#if defined(HARDWARE_XOR) || defined(SOFTWARE_XOR)
#include "core_xor.h"
#endif
#include "core_device.h"
#include "core_sata.h"
#include "core_sas.h"
#include "core_resource.h"
#ifdef SUPPORT_BOARD_ALARM
#include "core_alarm.h"
#endif
#ifdef SUPPORT_RMW_FLASH
#include "core_rmw_flash.h"
#endif

#include "core_error.h"
#include "core_util.h"
#include "com_api.h"
#include "core_console.h"

#include "hba_exp.h"
#include "core_exp.h"
#if defined(SUPPORT_I2C) || defined(SUPPORT_ACTIVE_CABLE)
#include "core_i2c.h"
#endif
#include "core_err_inj.h"

#include "hba_inter.h"
#include "com_extern.h"
#ifdef SCSI_ID_MAP
#include "devid_map.h"
#ifdef SUPPORT_ROC
#ifdef NEW_CORE_DRIVER
#include "core_flash.h"
#endif
#include "com_flash.h"
#endif
#endif

/* For debug purpose only. */
core_extension *gCore = NULL;
#ifdef SUPPORT_PHY_POWER_MODE
extern MV_U32 mv_PHY_power_mode_DIPM;
extern MV_U32 mv_PHY_power_mode_HIPM;
extern MV_U32 mv_PHY_power_mode_HIPM_timer;
extern MV_U32 mv_PHY_power_mode_port_map;
#endif
MV_VOID core_handle_waiting_queue(core_extension *core);
MV_VOID core_return_finished_req(core_extension *core, MV_Request *req);
MV_VOID core_push_queues(MV_PVOID core_p);
MV_VOID core_push_single_queue(core_extension * core, pl_queue * queue);
extern int have_flash_layout;
extern MV_VOID core_handle_error_queue(core_extension *core);
extern void core_handle_event_queue(core_extension *core);
#ifdef _OS_LINUX
extern MV_VOID core_clean_waiting_queue(core_extension *core);
extern MV_VOID core_clean_error_queue(core_extension *core);
extern void core_clean_event_queue(core_extension *core);
#endif
#ifdef MV_DEBUG
MV_BOOLEAN g_dump_register=0;
#endif

void scsi_log_err_cmd(const char *func_name, core_extension *core, MV_Request *req)
{
    char logstr[1024] = {0};
    char buf[512] = {0};
	struct scsi_cmnd *cmd;
	struct scsi_device *sdev;
    unsigned char* sense;
    int i;

    CORE_EH_PRINT(("(func, cdb[0]) = (%s, %02x)\n", func_name, req->Cdb[0]));
    cmd = req->Org_Req_Scmd;
    if(cmd && cmd->device && cmd->device->host)
    {
        sdev = cmd->device;
        sprintf(logstr, "%s: [%d:%d:%d:%llu] req %p [0x%x] status 0x%x", func_name, sdev->host->host_no, sdev->channel, sdev->id, sdev->lun, req, req->Cdb[0], req->Scsi_Status);
    }
    else
    {
        sprintf(logstr, "%s: Device id %d req %p [0x%x] status 0x%x", func_name, req->Device_Id, req, req->Cdb[0], req->Scsi_Status);
    }

    if((req->Scsi_Status == REQ_STATUS_HAS_SENSE) && (req->Sense_Info_Buffer != NULL))
    {
        int j = 0;
        int k = 0;
        char tbuf[512] = {0};
        int add_len = 0;
        
        sense = req->Sense_Info_Buffer;
        for(i = 0 ; i < 8 ; i++)
        {
            char ttbuf[8] = {0};

            sprintf(ttbuf, "%02x", sense[i]);
            for(j = 0 ; j < strlen(ttbuf) && k < 511 ; j++)
            {
                tbuf[k] = ttbuf[j];
                k++;
            }
        }
        
        add_len = sense[7];
        for(i = 0 ; i < add_len && i < 96; i++)
        {
            char ttbuf[8] = {0};

            sprintf(ttbuf, "%02x", sense[i + 8]);
            for(j = 0 ; j < strlen(ttbuf) && k < 511 ; j++)
            {
                tbuf[k] = ttbuf[j];
                k++;
            }
        }
        tbuf[k] = '\0';
        sprintf(buf, "(length, add_len, sense) = (%d, %d, %s)", req->Sense_Info_Buffer_Length, add_len, tbuf);
        strcat(logstr, buf);
    }
    
    sprintf(buf, "cmd:%16ph\n", req->Cdb);
    strcat(logstr, buf);
    CORE_EH_PRINT((logstr));
}

MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 max_io)
{
	MV_U32 size = 0;

	if (type == RESOURCE_CACHED_MEMORY) {
		size += core_get_cached_memory_quota(max_io);
#ifdef HARDWARE_XOR
		size += xor_get_cached_memory_quota(max_io);
#endif
#ifdef SUPPORT_RMW_FLASH
		size += core_rmw_flash_get_cached_memory_quota(max_io);
#endif
		size += lib_err_inj_get_cached_memory_quota(max_io);

#ifdef SUPPORT_STAGGERED_SPIN_UP
		size += core_spin_up_get_cached_memory_quota(max_io);
#endif

		size += 1024; /* add some memory for pointer align. */
	} else if (type == RESOURCE_UNCACHED_MEMORY) {
		size += core_get_dma_memory_quota(max_io);
#ifdef HARDWARE_XOR
		size += xor_get_dma_memory_quota(max_io);
#endif
#ifdef SUPPORT_RMW_FLASH
		size += core_rmw_flash_get_dma_memory_quota(max_io);
#endif

		size += lib_err_inj_page_get_dma_memory_quota(max_io);

        #ifdef DOUBLEBUF_4_SINGLE_PRD
        size += 128 * 1024;
        #endif
	}

	return size;
}

MV_VOID init_virtual_device(pl_root *root, lib_resource_mgr *rsrc, lib_device_mgr *lib_dev)
{
	domain_device *dev = get_device_obj(root, rsrc);
	command_handler *handler;
	MV_U16 old_id = dev->base.id;
	MV_U16 new_id = VIRTUAL_DEVICE_ID;
	MV_BOOLEAN ret;
	
	handler = core_get_handler(root, HANDLER_API);
	/*
	 * init virtual device which is for API request
	 * upper layer force us to use VIRTUAL_DEVICE_ID
	 * so work around it right now
	 */
	if (old_id != new_id) {
		ret = change_device_map(lib_dev, &dev->base, old_id, new_id);
		MV_ASSERT(ret == MV_TRUE);
                dev->base.id = new_id;
	}

	set_up_new_device(root, NULL, dev, handler);
	set_up_new_base(root, NULL, &dev->base,
		handler,
		BASE_TYPE_DOMAIN_VIRTUAL,
		sizeof(domain_device));
#ifdef SUPPORT_MUL_LUN
	dev->base.TargetID = dev->base.id;
	root->lib_dev->target_id_map[dev->base.TargetID] = dev->base.id;
#endif
}
#ifdef _OS_LINUX
resource_func_tbl rsrc_func = {os_malloc_mem, NULL, NULL};
#else
resource_func_tbl rsrc_func = {NULL, NULL};
#endif

static const struct mvs_chip_info mvs_chips[] = {
	{ DEVICE_ID_THORLITE_0S1P, 1, 0, 1, 0x400, 17, 16,  9},
	{ DEVICE_ID_THORLITE_1S1P, 1, 0, 1, 0x400, 17, 16,  9},
	{ DEVICE_ID_THORLITE_2S1P, 1, 0, 3, 0x400, 17, 16,  9},
	{ DEVICE_ID_THORLITE_2S1P_WITH_FLASH, 1, 0, 3, 0x400, 17, 16, 9},
	{ DEVICE_ID_THOR_4S1P, 1, 0, 5, 0x400, 17, 16, 9},
	{ DEVICE_ID_THOR_4S1P_NEW, 1, 0, 5, 0x400, 17, 16,  9},
	{ DEVICE_ID_6320, 1, 0, 2, 0x400, 17, 16,  9},
	{ DEVICE_ID_6340, 1, 0, 4, 0x400, 17, 16,  9},
	{ DEVICE_ID_6440, 1, 0, 4, 0x400, 17, 16,  9},
	{ DEVICE_ID_6485, 1, 0, 8, 0x800, 33, 32, 10},
	{ DEVICE_ID_6480, 1, 0, 8, 0x800, 33, 32, 10},
	{ DEVICE_ID_9440, 1, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9445, 1, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9480, 2, 0, 4, 0x800, 64, 64,  11}, //VA6800m
	{ DEVICE_ID_9580, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9548, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9588, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9485, 2, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9480, 1, 1, 4, 0x800, 64, 64,  11}, //VA6400m
	{ DEVICE_ID_948F, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9123, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9122, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_914D, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9023, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_90A3, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9110, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9190, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9111, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9191, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9120, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91A0, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9122, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91A2, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9128, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9200, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9220, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9230, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9235, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9215, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_91F3, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9182, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9183, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9172, 1, 0, 8, 0x800, 64, 64,  11},
    { DEVICE_ID_9170, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9181, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9171, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9192, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91A1, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91A8, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9028, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_90A8, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91A3, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_9130, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_91B0, 1, 0, 8, 0x800, 64, 64,  11},
	{ DEVICE_ID_8180, 2, 0, 4, 0x400, 17, 16,  9},
   //Athena
  //  { DEVICE_ID_ATHENA, 2, 0, 8, 0x800, 64, 64, 11 },     // init 2 root fail
  { DEVICE_ID_ATHENA_1480, 1, 0, 8, 0x800, 64, 128, 11 },      // init second phy fail
 // { DEVICE_ID_ATHENA_1580, 1, 0, 8, 0x800, 64, 64, 11 },      // init second phy fail
//    { DEVICE_ID_ATHENA, 1, 0, 1, 0x800, 64, 64, 11 },
        { DEVICE_ID_ATHENA_FPGA, 1, 0, 1, 0x800, 64, 64, 11 },
       { DEVICE_ID_ATHENA_1475, 2, 0, 8, 0x800, 128, 128, 13 },      // init second phy fail
  	{ DEVICE_ID_ATHENA_1485, 1, 0, 8, 0x800, 128, 128, 13 },      // init second phy fail
  	{ DEVICE_ID_ATHENA_1495, 2, 0, 8, 0x800, 128, 128, 13 },      // init second phy fail
	{ 0 }
};
#define MVS_CHIP_NUM  sizeof(mvs_chips)/(sizeof(mvs_chips[0]))

MV_U32 core_get_chip_info(void *core_p)
{
	int i = 0;
	MV_U8 tmp_group, tmp_time = 0;
	core_extension * core = (core_extension *)core_p;
#ifdef SUPPORT_BALDUR
#ifndef _OS_FIRMWARE
	HBA_Info_Page hba_info_param;
	MV_U8 prod_id[8] = {'V', 'A', '6', '4', '0', '0', 'm', 0x00};
#endif
#endif

	for ( i = 0; i < MVS_CHIP_NUM; i++) {
		if (core->device_id == mvs_chips[i].chip_id) {
			core->chip_info = &mvs_chips[i];
#ifndef _OS_FIRMWARE            
			if (IS_VANIR(core) || IS_ATHENA_CORE(core)) {
				#ifdef CORE_SUPPORT_HBA_PAGE
				// Reads out Product ID, if VA6400m -> 4Port Vanir
				//                       if VA6800m -> 8Port Vanir
				if(mv_nvram_init_param(
					HBA_GetModuleExtension(core, MODULE_HBA),
					&hba_info_param)
					) {
					if (memcmp(hba_info_param.Product_ID, prod_id, 8) == 0) {
						if (core->chip_info->n_host != 1) {
							continue; // Keep looking for VA6400m
						}
					}

					if (hba_info_param.HBA_Flag & HBA_FLAG_SATA_SSU_MODE)
						core->sata_ssu_mode = 1;

					if (hba_info_param.DSpinUpGroup==0xff ||
							hba_info_param.DSpinUpTimer == 0x00 ||
							hba_info_param.DSpinUpTimer == 0xff)
						tmp_group = 0;
					else 
						tmp_group = hba_info_param.DSpinUpGroup;
					
					if (tmp_group)
						tmp_time = hba_info_param.DSpinUpTimer;

					if (tmp_group && tmp_time) {
						MV_ASSERT(tmp_time);
						if (!core->sata_ssu_mode) {
	#ifdef SUPPORT_STAGGERED_SPIN_UP
							core->spin_up_group = tmp_group;
							core->spin_up_time = tmp_time;
	#endif
						} else {
	#ifdef SUPPORT_DIRECT_SATA_SSU
							core->sata_ssu_group = tmp_group;
							core->sata_ssu_time = tmp_time;
	#endif
						}
					}
				}
				#endif
				CORE_DPRINT(("chip id %X.\n",core->device_id));
			}
#endif
			return 0;
		}
	}

	CORE_DPRINT(("Unknow chip id %X.\n",core->device_id));
	return 0;
}

MV_VOID core_set_multi_queue_num(core_extension *core, MV_U16 max_io)
{
	pl_root *root;
	MV_U16 i, cpu_num, queue_num;
	if(max_io >1){
		cpu_num = core->cpu_count;
#ifdef ATHENA_MUTI_QUEUE_PERFORMANCE_WA
		queue_num = MAX_ROOT_QUEUE_NUM;
#else
		queue_num = MV_MIN(cpu_num/core->chip_info->n_host, MAX_MULTI_QUEUE);
#endif
	}
	else{
		cpu_num = 1;
		queue_num = 1;
	}
    if(queue_num < 1)
        queue_num = 1;
	for(i = core->chip_info->start_host;
		i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		core->roots[i].queue_num = queue_num;
	}
}

MV_VOID Core_ModuleInitialize(MV_PVOID module, MV_U32 size, MV_U16 max_io)
{
	core_extension *core = NULL;
	lib_resource_mgr *rsrc = NULL;
	lib_device_mgr *lib_dev = NULL;
	Assigned_Uncached_Memory dma;
	MV_PU8 global_cached_vir = NULL;
	MV_U32 global_cached_size;
	MV_BOOLEAN ret;
	MV_U32 uncache_size;
	void * mod_desc;
	Controller_Infor controller;
	core = (core_extension*)module;

	/* zero cached memory */
	mod_desc = core->desc;
	MV_ZeroMemory(core, size);
	core->desc = mod_desc;
#ifdef SUPPORT_TASKLET
	core->bh_enabled = 1;
#endif
#ifdef SUPPORT_MULTI_WCQ
	core->enable_multi_wcq = 1;
#endif
	gCore = core;
	MV_ASSERT(((MV_PTR_INTEGER)core & (SIZE_OF_POINTER - 1)) == 0);
	rsrc = &core->lib_rsrc;
	lib_dev = &core->lib_dev;
	global_cached_vir = (MV_PU8)core + sizeof(core_extension);
	global_cached_size = size - sizeof(core_extension);
	/* set up controller information */
	HBA_GetControllerInfor(core, (PController_Infor)&controller);

#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	core->run_multi_thread_send_cmd = MV_TRUE;
#endif
	core->vendor_id = controller.Vendor_Id;
	core->device_id = controller.Device_Id;
	core->revision_id = controller.Revision_Id;
	core->cpu_count = controller.cpu_count;
#ifdef SUPPORT_ATHENA_DONT_ACCESS_BAR1
    core->mmio_base = controller.Base_Address[MV_PCI_BAR];
    core->io_base = controller.Base_Address[MV_PCI_BAR];
#else
    core->mmio_base = controller.Base_Address[MV_PCI_BAR];
    core->io_base = controller.Base_Address[MV_PCI_BAR_IO];
    #ifdef TEST_SRAM
    core->sram_vir = core->io_base;
    #endif
#endif
#if defined(SUPPORT_SP2)
	core->page_size = CH_MINI_PAGE_SIZE;
#endif
	core->run_as_none_raid = controller.run_as_none_raid;
	if (core_get_chip_info(core)) {
		return;
	}
	if (IS_ATHENA_CORE(core))
		core->has_port_queue = MV_TRUE;

	/* get dma memory */
#ifdef _OS_LINUX
	MV_ZeroMemory(&dma, sizeof(Assigned_Uncached_Memory));
	rsrc_func.extension = (void *)core;
#else // BELOW IS WINDOWS
//	HBA_GetResource(core, RESOURCE_UNCACHED_MEMORY, &dma);

	uncache_size = Core_ModuleGetResourceQuota( RESOURCE_UNCACHED_MEMORY, max_io  );

#ifndef MULTI_UNCACHE_ALLOC
	HBA_GetResource(core, RESOURCE_UNCACHED_MEMORY, &dma);
#else //MULTI_UNCACHE_ALLOC

		if (
		    hba_get_uncache_resource(
		        HBA_GetModuleExtension(core, MODULE_HBA),
			uncache_size,
			&dma
		    )
		) 
			goto err_alloc_core_mem;

#endif
	MV_ZeroMemory(dma.Virtual_Address, dma.Byte_Size);
#endif

	core_set_multi_queue_num(core, max_io);

	rsrc->core = core;
	lib_rsrc_init(rsrc, global_cached_vir, global_cached_size,
		dma.Virtual_Address, dma.Physical_Address, dma.Byte_Size,
		&rsrc_func, lib_dev);

	//JING EH TBD: Handle failure in Linux driver
	ret = core_init_cached_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;

	ret = core_init_dma_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;


#ifdef HARDWARE_XOR
	ret = xor_init_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;
#endif

#ifdef SUPPORT_MUL_LUN
	init_target_id_map(core->lib_dev.target_id_map, (sizeof(MV_U16)*MV_MAX_TARGET_NUMBER));
#endif

	init_virtual_device(&core->roots[core->chip_info->start_host], rsrc, lib_dev);

#ifdef SUPPORT_RMW_FLASH
	ret = core_init_rmw_flash_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;
#endif

    if (test_enabled(core))
	    ret = lib_err_inj_initialize(core, rsrc, max_io);

	return;

err_alloc_core_mem:
#ifndef _OS_LINUX
	MV_ASSERT(0);
#else
	alloc_uncached_failed(HBA_GetModuleExtension( core, MODULE_HBA ));
	return;
#endif
}
#ifdef SCSI_ID_MAP
MV_VOID core_scan_reported_id(MV_PVOID p_core)
{
    core_extension *core = (core_extension *)p_core;
    lib_device_mgr *lib_dev = &core->lib_dev;
    MV_U16 i;
    MV_U16 map_id;
    MV_U16 spec_id = 0;
    domain_base *base = NULL;
    domain_device *device;
    pl_root *root;
    MV_U32 addr;
    PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);

#if SUPPORT_SAVE_TABLE
#ifdef SUPPORT_ROC
    addr = FLASH_PDINFOPAGE_ADDR;
    if (!Core_NVMRd(NULL, MFR_FLASH_DEV(0), addr, FLASH_TABLE_SIZE, (MV_PU8)hba->flash_table, 0)) {
        FM_PRINT("%s %d %s ... Read Flash for flash table is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
    }
#else
    addr = ODIN_FLASH_SIZE - PD_INFO_PAGE_OFFSET;
    if (core_rmw_read_flash(core, addr,(MV_PU8)hba->flash_table, FLASH_TABLE_SIZE) == MV_FALSE) {
        MV_ASSERT(MV_FALSE);
    }
#endif
#endif
    for(i = 0; i < MAX_ID; i++){
        base = get_device_by_id(lib_dev, i, MV_FALSE, MV_FALSE);
        if (base == NULL)
            continue;
        device = (domain_device *)base;
        if((base->type == BASE_TYPE_DOMAIN_DEVICE)||
            (base->type == BASE_TYPE_DOMAIN_ENCLOSURE)){
            if (base->parent != NULL) {
                if (base->parent->type == BASE_TYPE_DOMAIN_PM) {
                    root = base->port->base.root;
                    spec_id = base->port->phy->id + root->base_phy_num;
                    spec_id *= device->pm->num_ports;
                    spec_id += device->pm_port;
                    spec_id += DEVICE_PORT_NUMBER;
                    add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
#if SUPPORT_SAVE_TABLE
                    hba->flash_table[spec_id] = device->WWN;
#endif
                }
                else if (base->parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
#if SUPPORT_SAVE_TABLE
                    spec_id = get_flash_map_index_ex(hba->flash_table, device->WWN, hba->have_pm);
                    if (spec_id != 0xFFFF) {
                        add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
                    }
                    else {
                        spec_id = get_unused_device_map_item(hba->map_table, hba->have_pm);
                        if (spec_id != 0xFFFF) {
                        	replace_flash_map_item(hba->flash_table, spec_id, device->WWN);
                            add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
                        }
                        else {
                            FM_PRINT("%s %d %s ... flash table is full....\n", __FILE__, __LINE__, __FUNCTION__);
                            MV_ASSERT(MV_FALSE);
                        }
                    }
#else
                    get_device_map_index_ex(hba->map_table, device->WWN, base->id, hba->have_pm, DEVICE_TYPE_PD);
#endif
                }
                else if (base->parent->type == BASE_TYPE_DOMAIN_PORT) {
                    root = base->port->base.root;
                    spec_id = base->port->phy->id + root->base_phy_num;
                    add_device_map_item_by_specific_id(hba->map_table, spec_id, base->id, device->WWN, DEVICE_TYPE_PD);
#if SUPPORT_SAVE_TABLE
                    hba->flash_table[spec_id] = device->WWN;
#endif
                }
            }
        }
        else if (base->type == BASE_TYPE_DOMAIN_VIRTUAL) {
            add_device_map_item_by_specific_id(hba->map_table,VIRTUAL_DEVICE_ID,VIRTUAL_DEVICE_ID, device->WWN, DEVICE_TYPE_VIRTUAL);
#if SUPPORT_SAVE_TABLE
            hba->flash_table[VIRTUAL_DEVICE_ID] = device->WWN;
#endif
        }
    }
#if SUPPORT_SAVE_TABLE
#ifdef SUPPORT_ROC
    if ((have_flash_layout)&&(!Core_NVMWr(NULL, MFR_FLASH_DEV(0), addr, FLASH_TABLE_SIZE, (MV_PU8)hba->flash_table, 0))) {
        FM_PRINT("%s %d %s ... Write Flash for flash table is Fail....\n", __FILE__, __LINE__, __FUNCTION__);
    }
#else
    if ((core_rmw_write_flash(core, addr,(MV_PU8)hba->flash_table, FLASH_TABLE_SIZE) == MV_FALSE)) {
        MV_ASSERT(MV_FALSE);
    }
#endif
#endif
}
#endif

void core_io_chip_init_done(core_extension *core, MV_BOOLEAN single)
{
	/* io chip init is done, anything else wanna do */
#ifdef IGNORE_FIRST_PARITY_ERR
        core_enable_intl_parity(core);
#endif
#ifdef SUPPORT_SGPIO
	sgpio_initialize(core);
#endif
#ifdef SUPPORT_I2C
	i2c_init(core);
#endif

#ifdef SUPPORT_BOARD_ALARM
	core_alarm_init(core);
#endif
#ifdef SCSI_ID_MAP
    core_scan_reported_id(core);
    core_start_cmpl_notify(core);
#endif
}
void core_handle_init_queue(core_extension *core, MV_BOOLEAN single)
{
	struct _domain_base *base = NULL;
	MV_BOOLEAN ret;
	MV_ULONG flags;
    domain_device *device;
    
	//MV_DASSERT(Counted_List_GetCount(&core->init_queue, MV_TRUE) == Counted_List_GetCount(&core->init_queue, MV_FALSE));
	if (single) {
		OSSW_SPIN_LOCK(&core->init_queue_SpinLock, flags);
		if (Counted_List_GetCount(&core->init_queue, MV_FALSE) < core->init_queue_count) {
			if (Counted_List_GetCount(&core->init_queue, MV_FALSE) != core->init_queue_count-1) {
				#ifdef STRONG_DEBUG
				MV_ASSERT(Counted_List_GetCount(&core->init_queue, MV_FALSE) == core->init_queue_count-1);
				#else
				CORE_DPRINT(("init_queue_current_count %d < init_queue_count %d count: %d \n", \
					Counted_List_GetCount(&core->init_queue, MV_FALSE),core->init_queue_count, Counted_List_GetCount(&core->init_queue, MV_TRUE) ));
				#endif
			}
			OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
			return;
		}
		OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
	}

	if ((core->state == CORE_STATE_IDLE) && !(core->is_dump)) {
		CORE_DPRINT(("pushing queue when core state is idle!\n"));
			return;
	}

	while (MV_TRUE) {
		OSSW_SPIN_LOCK(&core->init_queue_SpinLock, flags);
		if (Counted_List_Empty(&core->init_queue)) {
			OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
			break;
		}
		base = Counted_List_GetFirstEntry(&core->init_queue,
			struct _domain_base, init_queue_pointer);
		if(base == NULL){
			OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
			break;
		}
		ret = base->handler->init_handler(base);
		if (ret == MV_FALSE) {
			CORE_DPRINT(("init %p fail.\n", base));
			Counted_List_Add(&base->init_queue_pointer, &core->init_queue);
			OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
			return;
		}
		OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
        if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
            device = (domain_device *)base;
            if((device->state == DEVICE_STATE_DUPLICATE_DEVICE) && (ret == MV_TRUE))
                pal_set_down_disk(base->root, device, MV_FALSE);
        }
        
    
		/* just handle one init entry */
		if (single) {
		        break;
		}
	}

	if (core->state == CORE_STATE_STARTING) {
		if (core->init_queue_count == 0) {
			/* There is a possibility for init requests outstanding */
			OSSW_SPIN_LOCK(&core->init_queue_SpinLock, flags);
			if (core_check_outstanding_req(core) != 0){
				OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
				return;
			}
			MV_ASSERT(Counted_List_Empty(&core->init_queue));
			core->state = CORE_STATE_STARTED;
#ifdef SUPPORT_PHY_POWER_MODE
			if((core->PHY_power_mode_HIPM != 0) && (core->PHY_power_mode_timer_index == 0xffff)){
				core->PHY_power_mode_timer_index = Timer_AddRequest(core, 2, Core_EnablePowerStateHIPM, core, NULL);
						
			}
#endif
			OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock,flags);
			core_io_chip_init_done(core, single);

			/* RAID will send request down in notify function.*/
			/* ikkwong: TBD if there are other modules */
			CORE_DPRINT((" finished init, notify upper layer. \n" ));
			core_start_cmpl_notify(core);
			
			return;
		}
	}
}

MV_VOID core_clean_init_queue_entry(MV_PVOID root_p, domain_base *base)
{
	pl_root *root = (pl_root *)root_p;
	MV_ULONG flags;
	core_extension *core = (core_extension *)root->core;
	domain_base *tmp_base, *found_base = NULL;
	OSSW_SPIN_LOCK(&core->init_queue_SpinLock, flags);
	//KeAcquireSpinLockAtDpcLevel(&core->init_queue_SpinLock);
	LIST_FOR_EACH_ENTRY_TYPE(tmp_base, &core->init_queue, domain_base,
		init_queue_pointer) {
		if (tmp_base == base) {
			found_base = base;
			Counted_List_Del(&found_base->init_queue_pointer, &core->init_queue);
			break;
		}
	}

	if (found_base) {
		/* set the domain_base to NULL, 
		 * so it'll only minus some init count. 
		 * core->init_queue_count--;
		 * port->init_count */
		core_init_entry_done(root, base->port, NULL);
	}
	OSSW_SPIN_UNLOCK(&core->init_queue_SpinLock, flags);
	//KeReleaseSpinLockFromDpcLevel(&core->init_queue_SpinLock);

}

#ifdef DRIVER_IO_PATH
MV_VOID core_req_handler (MV_PVOID core_p) 
{
    core_extension *core = (core_extension *)core_p;
    PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
#ifdef _OS_WINDOWS
    StorPortIssueDpc(
        hba->Device_Extension,
        &hba->ReqHandlerDpc,
        NULL,
        NULL);
#else
    core_push_queues(core);
#endif
} 
#endif
#ifdef ATHENA_A1_HW_CC_WA
MV_VOID core_enable_hw_cc(core_extension *core, MV_PVOID p_tmp)
{
	MV_U32 i,j;
	MV_U32 tmp;
	pl_root *root;
	if(core == NULL)
		return;
	for (j=0; j<core->chip_info->n_host; j++){
		root = &core->roots[j];
		for(i = 0; i < root->queue_num; i++) {
			if (root->slot_count_support > 0x1ff )
			tmp =  INT_COAL_COUNT_MASK & 0x1ff;
			else
			tmp =  INT_COAL_COUNT_MASK & root->slot_count_support;
		    tmp |= INT_COAL_ENABLE;

		    WRITE_IRQ_COAL_CONFIG(root, i, tmp);
		    tmp = 0x10100; //tmp = 0x1001B;
		    WRITE_IRQ_COAL_TIMEOUT(root, i, tmp);
		}
       }
	CORE_DPRINT(("enable interrupt Coalescing\n"));
}
#endif
MV_VOID Core_ModuleStart(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	unsigned long flags;
#ifdef SWAP_VANIR_PORT_FOR_LENOVO
	MV_I8 j=0,i=0;
#else
	MV_U8 i;
#endif
	Controller_Infor controller;
#ifdef SUPPORT_PHY_POWER_MODE
	MV_U32 tmp = 0;
	core->PHY_power_mode_DIPM = (MV_U8)mv_PHY_power_mode_DIPM;
	core->PHY_power_mode_HIPM = (MV_U8)mv_PHY_power_mode_HIPM;
	core->PHY_power_mode_HIPM_timer = (MV_U16)mv_PHY_power_mode_HIPM_timer;
	core->PHY_power_mode_port_map = mv_PHY_power_mode_port_map;
	core->PHY_power_mode_timer_index = 0xffff;
	CORE_DPRINT(("mv_PHY_power_mode_DIPM is set to %x\n",mv_PHY_power_mode_DIPM));
	CORE_DPRINT(("mv_PHY_power_mode_HIPM is set to %x\n",mv_PHY_power_mode_HIPM));
	CORE_DPRINT(("mv_PHY_power_mode_HIPM_timer is set to %x\n",mv_PHY_power_mode_HIPM_timer));
	CORE_DPRINT(("mv_PHY_power_mode_port_map is set to %x\n",mv_PHY_power_mode_port_map));
#endif
	HBA_GetControllerInfor(core, (PController_Infor)&controller);

	if(core_reset_controller(core) == MV_FALSE){
		CORE_DPRINT(("Reset hba failed.\n"));
		return;
	}

	/*
	 * init this controller, especially global control
	 */
	controller_init(core);
	/*
	 * init each module
	 */
#ifdef SUPPORT_NVSRAM
	core->nvsram_base = nvsram_init(core->mmio_base, controller.Base_Address[2]);
#endif

#if 0
	i2c_init();

	sgpio_init();
#endif

#ifdef HARDWARE_XOR
	xor_init(core);
#endif
	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	spin_lock_irqsave(&core->core_global_SpinLock, flags);
//	core_global_lock(core, &flags);
#ifdef ATHENA_MICRON_DETECT_WA	// for SSD cold boot detection issue.
	for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
		core->roots[i].base_phy_num = i * core->chip_info->n_phy;
		io_chip_init_registers_wa(&core->roots[i], 1);		
	}
	core_sleep_millisecond(core, 1000);
#endif
#ifdef SWAP_VANIR_PORT_FOR_LENOVO
	for (i=(core->chip_info->start_host + core->chip_info->n_host)-1; i>=core->chip_info->start_host; i--){
		core->roots[i].base_phy_num = j * core->chip_info->n_phy;
		io_chip_init(&core->roots[i]);
		j++;
	}
#else
	for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
		core->roots[i].base_phy_num = i * core->chip_info->n_phy;
		io_chip_init(&core->roots[i]);
	}
#endif
	core->state = CORE_STATE_STARTING;
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//	core_global_unlock(core, &flags);
//	spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
#if 0 //def DRIVER_IO_PATH
		core_req_handler(core);
#else
	core_push_all_queues(core);
#endif
#if defined(SUPPORT_MSIX_INT) && defined(_OS_WINDOWS)
    if(msix_enabled(core)){
        HBA_Extension *pHBA = (HBA_Extension *)HBA_GetModuleExtension(core, MODULE_HBA);
        ULONG message_id = 0;
#ifdef SUPPORT_MULTI_DPC
        StorPortIssueDpc(
                pHBA->Device_Extension,
                &pHBA->MsixDpc[message_id],
                &core->roots[0],
                &core->roots[0].queues[0]);
#else
        StorPortIssueDpc(
                pHBA->Device_Extension,
                &pHBA->MsixDpc,
                ULongToPtr(message_id),
                NULL);
#endif
    }
#endif
#if defined(SUPPORT_MSI) && defined(_OS_WINDOWS)
    if(msi_enabled(core)){
        HBA_Extension *pHBA = (HBA_Extension *)HBA_GetModuleExtension(core, MODULE_HBA);
        ULONG message_id = 0;
	 StorPortIssueDpc(
        pHBA->Device_Extension,
        &pHBA->IsrDpc,
        NULL,
        NULL);
    }
#endif

#if defined(ATHENA_A1_HW_CC_WA) && !defined(DISABLE_HW_CC)
	core_add_timer(core, 30, (MV_VOID (*) (MV_PVOID, MV_PVOID))core_enable_hw_cc, core, NULL);
#endif
}

extern MV_VOID core_sgpio_led_off_timeout(domain_base * base, MV_PVOID tmp);
extern MV_U8 core_sgpio_set_led( MV_PVOID extension, MV_U16 device_id, MV_U8 light_type,
        MV_U8 light_behavior, MV_U8 flag);
MV_QUEUE_COMMAND_RESULT core_send_request(core_extension *core, MV_Request *req)
{
	core_context *ctx;
	pl_root *root;
	MV_QUEUE_COMMAND_RESULT result;
	struct _domain_base *base;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	MV_ULONG flags;
#endif

	//JING TBD: both core_send_request and prot_send_request will allocate resource,
	//better to put in one function to avoid resource leak.
	//if one resource allocation is failed, release all resources already got.
	if (req->Context[MODULE_CORE] == NULL)
		req->Context[MODULE_CORE] = get_core_context(&core->lib_rsrc);
	ctx = req->Context[MODULE_CORE];
	if (ctx == NULL){
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}
	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
			 req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	if(base->blocked == MV_TRUE && !CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)
#ifdef SUPPORT_SG_RESET
				&& !CORE_IS_HBA_RESET_REQ(req)
#endif
	){
		return MV_QUEUE_COMMAND_RESULT_BLOCKED;
	}
#ifdef ATHENA_ONLY_REPORT_DOMAIN_DEVICE
    if(req->Req_Type != REQ_TYPE_CORE)
    {
        if(base->type ==BASE_TYPE_DOMAIN_DEVICE){
            struct _domain_device *device = (struct _domain_device *)base;
            if(device->max_lba.value==0)
            {
                req->Scsi_Status = REQ_STATUS_NO_DEVICE;
                return MV_QUEUE_COMMAND_RESULT_FINISHED;
            }
        }else{
            req->Scsi_Status = REQ_STATUS_NO_DEVICE;
            return MV_QUEUE_COMMAND_RESULT_FINISHED;
        }
    }
#endif
	if (base->port != NULL) {
		/* If error count is not zero, only do CORE_IS_EH_REQ */
		MV_U32 flags;
               if ( (base->type == BASE_TYPE_DOMAIN_DEVICE)) {
			   	domain_device *dev=(domain_device *)base;
			if ((dev->state != DEVICE_STATE_INIT_DONE/*||dev->max_transfer_size == 0*/)&& !CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)
#ifdef SUPPORT_SG_RESET
				&& !CORE_IS_HBA_RESET_REQ(req)
#endif
			) {
				if(!(dev->status & DEVICE_STATUS_FUNCTIONAL)){
					req->Scsi_Status = REQ_STATUS_NO_DEVICE;
					//printk("no funtional\n");
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}else{
					//printk("full\n");
					return MV_QUEUE_COMMAND_RESULT_FULL;
				}
			}
		}
	}

	root = base->root;
	ctx->handler = base->handler;

#ifdef SUPPORT_I2C
	if(base->type == BASE_TYPE_DOMAIN_I2C)
		result = i2c_send_command(root, base, req);
	else
#endif
	result = prot_send_request(root, base, req);
/*	if (result == MV_QUEUE_COMMAND_RESULT_SENT){
		OSSW_SPIN_LOCK(&base->outstanding_SpinLock);
		base->outstanding_req++;		//add spin lockl
		OSSW_SPIN_UNLOCK(&base->outstanding_SpinLock);
	}
	else
*/
	if ((result == MV_QUEUE_COMMAND_RESULT_FULL)
		|| (result == MV_QUEUE_COMMAND_RESULT_NO_RESOURCE)) {
		/* suppose if return these two status,
		 * should release all resource
		 * but for internal req, don't release core context
		 * this is used to fix dump mode resource issue */
		if ((req->Context[MODULE_CORE] != NULL)
			&& (req->Req_Type != REQ_TYPE_CORE)) {
			free_core_context(root->lib_rsrc, req->Context[MODULE_CORE]);
			req->Context[MODULE_CORE] = NULL;
		}
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
		OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
		if (core->run_multi_thread_send_cmd == MV_TRUE){
			core->run_multi_thread_send_cmd = MV_FALSE;
		}
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
#endif
	}

#ifdef SUPPORT_SGPIO_ACTIVE_LED
/*use timer to turn off active LED. 
    after 1 second if there isn't any request pass to this device, then time out. 
    and in core_sgpio_led_off_timeout driver will turn off activity LED
    by this way, IO performance almost will not be influenced
    TBD: is timer count is enough?*/

	/* Odin/Odin Lite did not support active LED */
 	if ((core->device_id == DEVICE_ID_6480) || (core->device_id == DEVICE_ID_6485)
                        ||(core->device_id == 0x8180) || IS_VANIR(core)
			||(core->device_id == DEVICE_ID_948F)
			|| IS_ATHENA_CORE(core)) {
		if((core->state == CORE_STATE_STARTED)
			&& (base->type == BASE_TYPE_DOMAIN_DEVICE)
			&& (IS_SGPIO(((domain_device *)base)))){
			if(((domain_device *)base)->active_led_off_timer == NO_CURRENT_TIMER){
				core_sgpio_set_led(core,base->id,0,0,LED_FLAG_ACT);
			}else{
				core_cancel_timer(core, ((domain_device *)base)->active_led_off_timer);
				((domain_device *)base)->active_led_off_timer = NO_CURRENT_TIMER;
			}
			((domain_device *)base)->active_led_off_timer = core_add_timer(core, 1, (MV_VOID (*) (MV_PVOID, MV_PVOID))core_sgpio_led_off_timeout, base, NULL);
		}
	}
#endif

	return result;
}

#ifdef _OS_LINUX
MV_VOID core_clean_waiting_queue(core_extension *core)
{
	MV_Request *req;
	MV_ULONG flags;
	MV_U8 root_id=0;
	pl_root *root;
	while (MV_TRUE) {
#ifdef ROOT_WAITING_QUEUE
	root_id=0;
	root = &core->roots[root_id];
        //OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
        OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
        if (Counted_List_Empty(&core->roots[0].waiting_queue)) {
			//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[0].waiting_queue, MV_Request, Queue_Pointer);
		//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
#else
		OSSW_SPIN_LOCK(&core->waiting_queue_SpinLock, flags);
		if (Counted_List_Empty(&core->waiting_queue)) {
			OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
#endif
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}
#ifdef ROOT_WAITING_QUEUE
       while (MV_TRUE) {
	root_id= 1;	
	root = &core->roots[root_id];
	    //OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
	     OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
        if (Counted_List_Empty(&core->roots[1].waiting_queue)) {
			//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->roots[1].waiting_queue, MV_Request, Queue_Pointer);
             //OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
             OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}
#endif
	while (MV_TRUE) {
		//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		//spin_lock_irqsave(&root->waiting_queue_SpinLock, flags);
		OSSW_SPIN_LOCK(&core->high_priority_SpinLock, flags);
		if (Counted_List_Empty(&core->high_priority_queue)) {
			OSSW_SPIN_UNLOCK(&core->high_priority_SpinLock, flags);
			//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
			//spin_unlock_irqrestore(&root->waiting_queue_SpinLock, flags);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->high_priority_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK(&core->high_priority_SpinLock, flags);
		//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		//spin_unlock_irqrestore(&root->waiting_queue_SpinLock, flags);
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}
}

MV_VOID core_clean_single_queue(core_extension *core, pl_queue *queue)
{
	MV_Request *req;
	MV_ULONG flags;

	while (MV_TRUE) {
		OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		if (Counted_List_Empty(&queue->waiting_queue)) {
			OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->waiting_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);

		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}
	while (MV_TRUE) {
		OSSW_SPIN_LOCK(&queue->high_priority_SpinLock, flags);
		if (Counted_List_Empty(&queue->high_priority_queue)) {
			OSSW_SPIN_UNLOCK(&queue->high_priority_SpinLock, flags);
			break;
		}
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&queue->high_priority_queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK(&queue->high_priority_SpinLock, flags);
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}
}
MV_VOID core_clean_all_queues(core_extension * core)
{
	MV_U8 i, j;
	pl_root *root = NULL;

	if (!core->enable_multi_wcq)
		return core_clean_waiting_queue(core);
	
	for (i = 0; i < MAX_NUMBER_IO_CHIP; i++) {
		root = &core->roots[i];
		for (j = 0; j < root->queue_num; j++)
			core_clean_single_queue(core, &root->queues[j]);
	}
}

MV_VOID core_complete_all_queues(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	pl_root *root = NULL;
	MV_U8 i, j;

	if (!core->enable_multi_wcq)	
		return core_complete_requests(core_p);

	for (i = 0; i < MAX_NUMBER_IO_CHIP; i++) {
		root = &core->roots[i];
		for (j = 0; j < root->queue_num; j++)
			core_complete_single_queue(core, &root->queues[j]);
	}
}
#endif

/* set hw queue for each base devcie */
void core_set_hw_queue_of_base(core_extension *core, domain_base *base)
{
	pl_root *root = base->root;
	domain_device *device = NULL;
	MV_ASSERT(root);

	if (base->type != BASE_TYPE_DOMAIN_DEVICE) {
		base->queue = &root->queues[0];
	} else {
		device = (domain_device *)base;
		if (!msix_enabled(core))
			device->queue_id = 0;
		else
			device->queue_id = base->id % root->queue_num;
		
		base->queue = &root->queues[device->queue_id];
	}
}

pl_queue *core_get_hw_queue_of_base(core_extension *core, domain_base *base)
{
	return base->queue;
}

MV_VOID *core_get_hw_queue(MV_PVOID core_p, MV_PVOID req_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_Request *req = (MV_Request *)req_p;
	domain_base *base;

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev,
			 req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return NULL;
	}

	return core_get_hw_queue_of_base(core, base);
}

#ifdef ROOT_WAITING_QUEUE
#if defined(_OS_LINUX)
MV_VOID io_chip_handle_waiting_queue(core_extension *core, Counted_List_Head *queue, pl_root *root, spinlock_t *lock)
#else
MV_VOID io_chip_handle_waiting_queue(core_extension *core, Counted_List_Head *queue, pl_root *root, PKSPIN_LOCK lock)
#endif
#else
#if defined(_OS_LINUX)
MV_VOID io_chip_handle_waiting_queue(core_extension *core, Counted_List_Head *queue, spinlock_t *lock)
#else
MV_VOID io_chip_handle_waiting_queue(core_extension *core, Counted_List_Head *queue, PKSPIN_LOCK lock)
#endif
#endif
{
    MV_Request *req;
    MV_QUEUE_COMMAND_RESULT result;
    Counted_List_Head tmp_queue;
    domain_base *base = NULL;
    pl_queue *hw_queue = NULL;
    MV_ULONG flags, flags1;
    MV_U16 i=0, j=0;
#ifdef ROOT_WAITING_QUEUE
    if ((root) && Tag_IsEmpty(&root->slot_pool)) 
        return;
#endif
	MV_COUNTED_LIST_HEAD_INIT(&tmp_queue);
	
    /*
    if one device returned MV_QUEUE_COMMAND_RESULT_FULL, 
    should block other requests on the same device. 
    but don't block other devices
    */
	while (MV_TRUE) {
		//KeAcquireSpinLockAtDpcLevel(lock);
		OSSW_SPIN_LOCK(lock, flags1);
		if(Counted_List_Empty(queue)) {
			OSSW_SPIN_UNLOCK(lock, flags1);
			break;
		}
#ifdef ROOT_WAITING_QUEUE
		if ((root) && Tag_IsEmpty(&root->slot_pool)) {
			OSSW_SPIN_UNLOCK(lock, flags1);
			break;
		}
#endif
		req = (MV_Request *)Counted_List_GetFirstEntry(
			queue, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK(lock, flags1);

		/* support multi hw queue, set hw_queue_id. */
		hw_queue = core_get_hw_queue(core, req);
		if(!hw_queue) {
			core_queue_completed_req_lock(core, req);
			continue;
		}		

		//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, hw_queue->msix_idx);
   		result = core_send_request(core, req);

   		if (result == MV_QUEUE_COMMAND_RESULT_SENT) {
   			//mv_add_timer(core, req);
   			//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, hw_queue->msix_idx);
   			continue;
   		}

		//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, hw_queue->msix_idx);

		switch ( result )
		{
		case MV_QUEUE_COMMAND_RESULT_FINISHED:
			core_queue_completed_req(core, req);
			break;

		case MV_QUEUE_COMMAND_RESULT_FULL:
			Counted_List_Add(&req->Queue_Pointer, &tmp_queue);
			base = (domain_base *)get_device_by_id(&core->lib_dev,
						req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
			MV_DASSERT(base != NULL);
			//OSSW_SPIN_LOCK_Device(core, flags, base);
			OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
			base->blocked = MV_TRUE;
			//OSSW_SPIN_UNLOCK_Device(core, flags, base);
			OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_BLOCKED:
			Counted_List_Add(&req->Queue_Pointer, &tmp_queue);
			break;

		case MV_QUEUE_COMMAND_RESULT_NO_RESOURCE:
			Counted_List_Add(&req->Queue_Pointer, &tmp_queue);
			continue;

#if 0
		case MV_QUEUE_COMMAND_RESULT_SENT:
			mv_add_timer(core, req);
			break;
#endif
		case MV_QUEUE_COMMAND_RESULT_REPLACED:
			break;

		default:
			MV_ASSERT(MV_FALSE);
		}
	}

ending:
#if 0
	if (!Counted_List_Empty(&tmp_queue)) {
		OSSW_SPIN_LOCK(lock);
		Counted_List_AddCountedList(&tmp_queue, queue);
//		Counted_List_Add(&req->Queue_Pointer, queue);
		OSSW_SPIN_UNLOCK(lock);
	}
#endif
	
        while (!Counted_List_Empty(&tmp_queue)) {
                req = (MV_Request *)Counted_List_GetFirstEntry(
                        &tmp_queue, MV_Request, Queue_Pointer);
				
                base = (domain_base *)get_device_by_id(&core->lib_dev,
                                        req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
                MV_DASSERT(base != NULL);
		  //OSSW_SPIN_LOCK_Device(core, flags, base);
		  OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
                base->blocked = MV_FALSE;
		  //OSSW_SPIN_UNLOCK_Device(core, flags, base);
		  OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
#if 1
          OSSW_SPIN_LOCK(lock, flags1);
          Counted_List_Add(&req->Queue_Pointer, queue);
          OSSW_SPIN_UNLOCK(lock, flags1);
#else
		   if(!SCSI_IS_READ_WRITE_VERIFY(req->Cdb[0])){
		   	OSSW_SPIN_LOCK(&core->high_priority_SpinLock, flags1);
			Counted_List_Add(&req->Queue_Pointer, &core->high_priority_queue);
			OSSW_SPIN_UNLOCK(&core->high_priority_SpinLock, flags1);
		   }else{
		  OSSW_SPIN_LOCK(lock, flags1);
                Counted_List_Add(&req->Queue_Pointer, queue);
                OSSW_SPIN_UNLOCK(lock, flags1);
		}
 #endif
        }
}

MV_VOID core_handle_waiting_queue(core_extension *core)
{
	MV_ULONG flags;

#ifdef _OS_UKRN
	if (core->submitting) {
		please_call_me_later(CORE_PUSH_WAITING);
		return;
	}
#endif

	/* don't push waiting queues if pause_waiting_queue. */
	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	if (core->pause_waiting_queue) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
	core->waiting_queue_running++;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);

	core->submitting = 1;

	/* io chip queue: high priority and low priority */
 
#ifdef ROOT_WAITING_QUEUE
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, NULL, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->roots[0].waiting_queue, &core->roots[0], &core->roots[0].waiting_queue_SpinLock);
	io_chip_handle_waiting_queue(core, &core->roots[1].waiting_queue, &core->roots[1], &core->roots[1].waiting_queue_SpinLock);
#else
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->waiting_queue, &core->waiting_queue_SpinLock);
#endif

#ifdef HARDWARE_XOR
	/* xor queue */
	xor_handle_waiting_list(core);
#endif

	core->submitting = 0;

	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	core->waiting_queue_running--;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
}

#ifdef _OS_LINUX
MV_VOID core_abort_all_running_requests(MV_PVOID core_p)
{
	MV_U8 j;
	MV_U32 i;
	pl_root *root;
	domain_port *port;
	domain_base *base;
	PMV_Request req = NULL;
	core_extension *core = (core_extension *)core_p;
	PHBA_Extension pHBA = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
	MV_ULONG flags_queue, flags;
	core_clean_event_queue(core);
	core_clean_error_queue(core);
	core_clean_all_queues(core);
	
	for(j = core->chip_info->start_host; j < (core->chip_info->start_host + core->chip_info->n_host); j++) {
		root = &core->roots[j];
		for (i = 0; i < root->slot_count_support; i++) {
			req = root->running_req[i];
			if (req == NULL) continue;
			base = (struct _domain_base *)get_device_by_id(root->lib_dev,
						req->Device_Id, CORE_IS_ID_MAPPED(req), MV_FALSE);
			MV_ASSERT(base != NULL);
			OSSW_SPIN_LOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);
			OSSW_SPIN_LOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
			prot_reset_slot(root, base, i, req);
			OSSW_SPIN_UNLOCK(&base->err_ctx.sent_req_list_SpinLock, flags);
			OSSW_SPIN_UNLOCK(&base->queue->handle_cmpl_SpinLock, flags_queue);

			if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
				domain_device *device = (domain_device *)base;

				if (device->register_set != NO_REGISTER_SET) {
					MV_DASSERT(device->base.outstanding_req == 0);
					core_free_register_set(root, device, device->register_set);
					device->register_set = NO_REGISTER_SET;
				}
			}
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_queue_completed_req(core, req);
		}
	}

	core_complete_internal_requests(core);
	core_complete_all_queues(core);
}
#endif

MV_VOID Core_ModuleSendRequest(MV_PVOID core_p, PMV_Request req)
{
	MV_ULONG flags;
	MV_QUEUE_COMMAND_RESULT result;
	struct _domain_base *base;
	core_extension *core = (core_extension *)core_p;
	pl_queue *queue = NULL;
	pl_root *root;
	MV_U8 root_id=0;
	/* external request shouldn't have the context. internal request won't come here */
	MV_DASSERT(req->Context[MODULE_CORE] == NULL);

#ifdef GT_PERFORMANCE_TUNE	
extern MV_U32 GT_PERF;
if(GT_PERF & MV_BIT(4))
{
        if ( SCSI_IS_WRITE(req->Cdb[0]) ) 
        {
                req->Scsi_Status = REQ_STATUS_SUCCESS;
                core_queue_completed_req(core,req);
                return;
        }       
}
#endif
#ifdef ROOT_WAITING_QUEUE
#if defined (SUPPORT_MUL_LUN) && defined(_OS_WINDOWS)
    if (req->Req_Type == REQ_TYPE_OS) {
        base = (struct _domain_base *)get_base_by_target_lun(core, DEV_ID_TO_TARGET_ID(req->Device_Id), DEV_ID_TO_LUN(req->Device_Id));
    }
    else 
#endif
    base = (struct _domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id, MV_FALSE, MV_FALSE);


	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		core_queue_completed_req(core,req);
#ifndef CORE_NO_RECURSIVE_CALL
#if 0 //def DRIVER_IO_PATH
              core_req_handler(core);
#else
		core_send_mv_request(core, req);
#endif
#endif //CORE_NO_RECURSIVE_CALL
		return;
	}
	root_id = base->root->root_id;
	root = &core->roots[root_id];
#if !defined(_OS_WINDOWS)
	if(SCSI_IS_READ_WRITE(req->Cdb[0]) && Counted_List_Empty( &root->waiting_queue) && (req->Data_Transfer_Length <= 4096)){
		result = core_send_request(core,req);
		//printk("result: %x\n", result);
		switch ( result )
		{
		case MV_QUEUE_COMMAND_RESULT_FINISHED:
			core_queue_completed_req(core, req);
			break;

		case MV_QUEUE_COMMAND_RESULT_FULL:
			//printk("full\n");
			OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
			Counted_List_AddTail(&req->Queue_Pointer, &root->waiting_queue);
			OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_BLOCKED:
			//printk("blocked\n");
			OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
			Counted_List_AddTail(&req->Queue_Pointer, &root->waiting_queue);
			OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_NO_RESOURCE:
			//printk("no rsc\n");
			OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
			Counted_List_AddTail(&req->Queue_Pointer, &root->waiting_queue);
			OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_REPLACED:
			break;

		default:
			break;
			//MV_ASSERT(MV_FALSE);
		}
	}
	else
#endif
	{
		OSSW_SPIN_LOCK(&root->waiting_queue_SpinLock, flags);
		Counted_List_AddTail(&req->Queue_Pointer, &root->waiting_queue);
		OSSW_SPIN_UNLOCK(&root->waiting_queue_SpinLock, flags);
	}
#else
	if (core->enable_multi_wcq) {
		queue = core_get_hw_queue(core, req);
		if (!queue)
			return;
		OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		Counted_List_AddTail(&req->Queue_Pointer, &queue->waiting_queue);
		OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
	} else {
		root = &core->roots[root_id];
		//OSSW_SPIN_LOCK_WAIT_QUEUE(core, flags, root_id);
		OSSW_SPIN_LOCK(&core->waiting_queue_SpinLock, flags);
		Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
		//OSSW_SPIN_UNLOCK_WAIT_QUEUE(core, flags, root_id);
		OSSW_SPIN_UNLOCK(&core->waiting_queue_SpinLock, flags);
	}
#endif
#ifndef CORE_NO_RECURSIVE_CALL
#if 0 //def DRIVER_IO_PATH
		core_req_handler(core);
#else
	core_send_mv_request(core, req);
#endif
#endif //CORE_NO_RECURSIVE_CALL
}

MV_VOID Core_ModuleMonitor(MV_PVOID core_p)
{
}

MV_VOID Core_ModuleNotification(MV_PVOID core_p, enum Module_Event event,
	struct mod_notif_param *param)
{
}

MV_VOID Core_ModuleReset(MV_PVOID core_p)
{
}

MV_VOID Core_ModuleShutdown(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_U8 i;
	pl_root *root;
#ifdef ROOT_WAITING_QUEUE
    MV_ASSERT(Counted_List_Empty(&core->roots[0].waiting_queue));
    MV_ASSERT(Counted_List_Empty(&core->roots[1].waiting_queue));
#else
	MV_ASSERT(Counted_List_Empty(&core->waiting_queue));
#endif
	MV_ASSERT(Counted_List_Empty(&core->high_priority_queue));
	MV_ASSERT(Counted_List_Empty(&core->error_queue));

	for (i = core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++) {
		root = &core->roots[i];
		hal_disable_io_chip(root);
	}
/* It's not a good place to free the cache here, But ... */
#ifdef PCI_POOL_SUPPORT
#ifndef HARDWARE_XOR
	ossw_pci_pool_destroy(core->sg_pool);
#endif
	ossw_pci_pool_destroy(core->ct_pool);
#endif

}

MV_VOID core_set_chip_options(MV_PVOID ext)
{
	pl_root *root = (pl_root *)ext;
	core_extension *core = (core_extension *)root->core;
	root->max_register_set = core->chip_info->srs_sz;
	root->unassoc_fis_offset = core->chip_info->fis_offs;
	root->max_cmd_slot_width = core->chip_info->slot_width;

	/* Note: edit this part if register set is more than 64 */
	if (root->max_register_set > 64) {
		if (root->max_register_set > 128)
	        	MV_DASSERT(MV_FALSE);
		else if (root->max_register_set == 128) {
			root->sata_reg_set.value = 0;
			root->sata_reg_set2.value = 0;

		} else {
			root->sata_reg_set.value = 0;
			root->sata_reg_set2.value = (1ULL << (root->max_register_set - 64 + 1)) - 1;
			root->sata_reg_set2.value = ~root->sata_reg_set2.value;
		}

	/* if register set is 64 then initialize register set to 0 */
	} else if (root->max_register_set == 64) {
		root->sata_reg_set.value = 0;

	} else {
		root->sata_reg_set.value = (1ULL << (root->max_register_set + 1)) - 1;
		root->sata_reg_set.value = ~root->sata_reg_set.value;
	}
}



MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	return core_clear_int(core);
}
MV_U8 Core_CheckQueueShadow(pl_queue *queue)
{
	MV_U16 cmpl_wp,i;
	
	cmpl_wp = (MV_U16)MV_LE32_TO_CPU(*(MV_U32 *)queue->cmpl_q_shadow) & 0x3fff;
	i = queue->last_cmpl_q;
	if(i == cmpl_wp){
		return MV_FALSE;
	}else{
		return MV_TRUE;
	}
	
}
void Core_InterruptHandleIRQ(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	pl_root *root;
	MV_U8 i,j;
	MV_BOOLEAN ret = MV_TRUE;
	MV_ULONG flags;

	for (i = core->chip_info->start_host;
			i < (core->chip_info->start_host + core->chip_info->n_host); i++) {

		root = &core->roots[i];
		for (j = 0; j < root->queue_num; j++) {
			if(Core_CheckQueueShadow(&root->queues[j]))
			{
				//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, root->queues[j].msix_idx);
				OSSW_SPIN_LOCK(&root->queues[(root->queues[j].msix_idx)%root->queue_num].queue_SpinLock, flags);
				ret = io_chip_handle_cmpl_queue(&root->queues[j]);
				//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, root->queues[j].msix_idx);
				OSSW_SPIN_UNLOCK(&root->queues[(root->queues[j].msix_idx)%root->queue_num].queue_SpinLock, flags);
				if (ret == MV_FALSE) {
					/* all interrupts is handled by ATTENTION */
//					spin_lock_irqsave(&core->core_global_SpinLock, flags);
//					core_global_lock(core, &flags);
					OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
					io_chip_handle_attention(&root->queues[j]);
					OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//					core_global_unlock(core, &flags);
//					spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
				}
			
				/* notify upper layer we are done processing completed requests */
				core_complete_single_queue(core, &root->queues[j]);
#if 0 //def DRIVER_IO_PATH
				core_req_handler(core);
#else
				core_push_single_queue(core, &root->queues[j]);
#endif
			}
		}
	}
}

MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	pl_root *root;
	MV_U8 i;
	MV_BOOLEAN ret = MV_TRUE;
	MV_BOOLEAN no_attn = MV_TRUE;
	MV_ULONG flags;

	if(!core->bh_enabled)
    		ret = core_clear_int(core);

	if (ret == MV_TRUE) {
#if defined(CORE_PCIE_PNP_DETECTION)
		if (core->pci_state == CORE_PCIE_IS_INVALID) //PCIE PNP
			return ret;
#endif
		for (i = core->chip_info->start_host;
				i < (core->chip_info->start_host + core->chip_info->n_host); i++) {

			root = &core->roots[i];

			//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, root->queues[0].msix_idx);
			OSSW_SPIN_LOCK(&root->queues[(root->queues[0].msix_idx)%root->queue_num].queue_SpinLock, flags);
			no_attn = io_chip_handle_cmpl_queue(&root->queues[0]);
			//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, root->queues[0].msix_idx);
			OSSW_SPIN_UNLOCK(&root->queues[(root->queues[0].msix_idx)%root->queue_num].queue_SpinLock, flags);
			
			core_disable_ints(core);
			if (no_attn == MV_FALSE) {
				/* all interrupts is handled by ATTENTION */
//				spin_lock_irqsave(&core->core_global_SpinLock, flags);
//				core_global_lock(core, &flags);
				OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
				io_chip_handle_attention(&root->queues[0]);
				OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
				//core_global_unlock(core, &flags);
				//spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
			}

			/* notify upper layer we are done processing completed requests */
			core_complete_single_queue(core, &root->queues[0]);
#if 0 //def DRIVER_IO_PATH
		core_req_handler(core);
#else
			core_push_single_queue(core, &root->queues[0]);
#endif
			core_enable_ints(core);
		}
	}

	return ret;
}

MV_BOOLEAN Core_QueueInterruptHandler(MV_PVOID This, MV_U16 index)
{
	MV_U32 root_idx, queue_idx,tmp;
	MV_BOOLEAN ret = MV_TRUE;
	MV_ULONG flags, flags1;
	pl_root *root;
	pl_queue *queue;
	MV_U16 queue_msk = 0xffff;
	core_extension *core = (core_extension *)This;
	PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);

      root_idx = index / (hba->msix_count/core->chip_info->n_host);
      queue_idx = index % (hba->msix_count/core->chip_info->n_host);

	root = &core->roots[root_idx];
	queue = &root->queues[queue_idx];
	queue_msk = (MV_U16)MV_BIT(root->queue_num * root_idx + queue_idx);

	/* handle completion queue */
	//WRITE_CMPLQ_IRQ_MASK(root, queue_idx, 0x0);
	core_disable_queue_ints(core, queue_msk);
	
	//tmp = READ_CMPLQ_IRQ_STAT(root, queue_idx);
	//WRITE_CMPLQ_IRQ_STAT(root, queue_idx, tmp);
	// if(!(tmp & queue->irq_mask))  /*Is it possible entry complete but IRQ STAT not updated?*/
	{
		//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, queue->msix_idx);
		ret = io_chip_handle_cmpl_queue(queue);
		//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, queue->msix_idx);

		//core_disable_queue_ints(core, (MV_U16)(~queue_msk)); /*disable other queues' interrupts*/
		if (ret == MV_FALSE) {
			/* all interrupts is handled by ATTENTION */
//			spin_lock_irqsave(&core->core_global_SpinLock, flags);
//			core_global_lock(core, &flags);
			OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			//OSSW_SPIN_LOCK_HW_QUEUE(core, flags1, queue->msix_idx);
			//spin_lock_irqsave(&root->queues[0].queue_SpinLock, flags1);
			io_chip_handle_attention(queue);
			//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags1, queue->msix_idx);
			//spin_unlock_irqrestore(&root->queues[0].queue_SpinLock, flags1);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			core_global_unlock(core, &flags);
//			spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		}
		//core_enable_queue_ints(core, (MV_U16)(~queue_msk)); /*enable other queues' interrupts*/

		/* notify upper layer we are done processing completed requests */
		if (!List_Empty(&core->internal_compl_queue)) {
//			spin_lock_irqsave(&core->core_global_SpinLock, flags);
//			core_global_lock(core, &flags);
			OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			core_complete_internal_requests(core);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			core_global_unlock(core, &flags);
//			spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		}
		core_complete_single_queue(core, queue);
		core_push_single_queue(core, queue);
	} 
	
	/*update it within io_chip_handle_cmpl_queue();*/
	//WRITE_CMPLQ_RD_PTR(root, queue_idx, READ_CMPLQ_WR_PTR(root, queue_idx)); 
	
	//WRITE_CMPLQ_IRQ_MASK(root, queue_idx, root->queues[queue_idx].irq_mask);
	core_enable_queue_ints(core, queue_msk);
	return MV_TRUE;
}

#ifdef CORE_FORCED_ISR_WORKAROUND
/* Temporary work around for missed interrupts */
void core_forced_isr(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	pl_root *root;
        MV_U32 orig_irq;

        orig_irq = core->main_irq;
	core->main_irq = 0xFFFFFFFF;

	core_handle_int(core);

	/* notify upper layer we are done processing completed requests */
	core_complete_requests(core);
#if 0 //def DRIVER_IO_PATH
		core_req_handler(core);
#else
	core_push_queues(core);
#endif
        core->main_irq = orig_irq;
	return;
}
#endif

/* use this function when execute out of CORE_LOCK */
MV_VOID core_queue_completed_req_lock(MV_PVOID ext, MV_Request *req)
{
	core_extension *core = (core_extension *)ext;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_ULONG flags, flags1;

	/* fs TODO: all internal req is CORE_REQ_TYPE_NONE? */
	if((ctx != NULL)&& (ctx->req_type != CORE_REQ_TYPE_NONE)){
		OSSW_SPIN_LOCK(&core->internal_compl_SpinLock, flags1);
		//KeAcquireSpinLockAtDpcLevel(&core->internal_compl_SpinLock);
		List_AddTail(&req->Queue_Pointer, &core->internal_compl_queue);
		OSSW_SPIN_UNLOCK(&core->internal_compl_SpinLock, flags1);
		//KeReleaseSpinLockFromDpcLevel(&core->internal_compl_SpinLock);
	} else {
		if(core->enable_multi_wcq){
			pl_queue *queue = NULL;
			queue = core_get_hw_queue(core, req);
			OSSW_SPIN_LOCK_HW_CQ(core, flags, queue->msix_idx);
			List_AddTail(&req->Queue_Pointer, &queue->complete_queue);
			OSSW_SPIN_UNLOCK_HW_CQ(core, flags, queue->msix_idx);
		}else{
#if defined(_OS_LINUX)
			if(req->Data_Transfer_Length==4096)
				core_return_finished_req(core, req);
			else
#endif
			{
				OSSW_SPIN_LOCK(&core->cmpl_queue_SpinLock, flags);
				List_AddTail(&req->Queue_Pointer, &core->complete_queue);
				OSSW_SPIN_UNLOCK(&core->cmpl_queue_SpinLock, flags);
			}
		}
	}
}

MV_VOID core_queue_completed_req(MV_PVOID ext, MV_Request *req)
{
	core_extension *core = (core_extension *)ext;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_ULONG flags, flags1;

	/* fs TODO: all internal req is CORE_REQ_TYPE_NONE? */
	if((ctx != NULL)&& (ctx->req_type != CORE_REQ_TYPE_NONE)){
		//KeAcquireSpinLockAtDpcLevel(&core->internal_compl_SpinLock);
		OSSW_SPIN_LOCK(&core->internal_compl_SpinLock, flags1);
		List_AddTail(&req->Queue_Pointer, &core->internal_compl_queue);
		OSSW_SPIN_UNLOCK(&core->internal_compl_SpinLock, flags1);
		//KeReleaseSpinLockFromDpcLevel(&core->internal_compl_SpinLock);
	} else {
		if(core->enable_multi_wcq){
			pl_queue *queue = NULL;
			queue = core_get_hw_queue(core, req);
			OSSW_SPIN_LOCK_HW_CQ(core, flags, queue->msix_idx);
			List_AddTail(&req->Queue_Pointer, &queue->complete_queue);
			OSSW_SPIN_UNLOCK_HW_CQ(core, flags, queue->msix_idx);
		}else{
			//OSSW_SPIN_LOCK_COMPL_QUEUE(core, flags);
			OSSW_SPIN_LOCK(&core->cmpl_queue_SpinLock, flags);
			List_AddTail(&req->Queue_Pointer, &core->complete_queue);
			//OSSW_SPIN_UNLOCK_COMPL_QUEUE(core, flags);
			OSSW_SPIN_UNLOCK(&core->cmpl_queue_SpinLock, flags);
		}
	}
}

MV_VOID generic_post_callback(core_extension *core, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req->Req_Type != REQ_TYPE_CORE) {
		/*
		 * case 1: ctx == NULL,
		 * if requests are in the waiting queue ,
		 * but disk has been set down, it doesn't have context.
		 * case 2: ctx != NULL but ctx->handler==NULL
		 * In non-RAID mode, in core_send_request,
		 * return no device if cannot find base
		 * so handler is not set yet, especially in non-RAID mode
		 * case 3: disk is set down and in the completion queue
		 * but right after set down the device data structure is taken by new device,
		 * then when we handle the completed NO DEVICE request,
		 * still can get the handler but the handler is not for it.
		 */
		if (ctx != NULL) {
			MV_DASSERT(ctx->buf_wrapper == NULL);
			free_core_context(&core->lib_rsrc, ctx);
			req->Context[MODULE_CORE] = NULL;
		} else {
			MV_DASSERT(req->Scsi_Status == REQ_STATUS_NO_DEVICE);
		}

	} else {
		/* case 1 and case 2 won't happen here */
		intl_req_release_resource(&core->lib_rsrc, req);
	}
}


MV_VOID core_return_finished_req(core_extension *core, MV_Request *req)
{
/*
*	req->Req_Type is accessed after req->Completion(), which free's the req.   Fix: get a local copy of req->Req_Type
*
*/
	MV_U16 Req_Type = req->Req_Type;

	/*
	 * for external request: release resource before the callback function.
	 * after req->Completion we don't own the request any more.
	 * for internal request: we may need the context like PM, smart req.
	 * release resource after the Completion.
	 * so in all internal request Completion don't release the req
	 * or use it as other purpose.
	 */
	if (req->Req_Type != REQ_TYPE_CORE) {
		generic_post_callback(core, req);
		/*error inject for RAID error handling so add check_error_injection in none core internal request loop*/
    if (test_enabled(core))
		if (SCSI_IS_READ(req->Cdb[0]) || SCSI_IS_WRITE(req->Cdb[0]) ||
			SCSI_IS_VERIFY(req->Cdb[0]))
			check_error_injection(core, req);
	}
	
#ifdef SUPPORT_VU_CMD
	if ((IS_ATA_PASSTHRU_CMD(req,SCSI_CMD_MARVELL_VENDOR_UNIQUE))  
		&& (!IS_VU_CMD(req,MARVELL_VU_CMD_ASYNC_NOTIFY))) {
		domain_device *device  = NULL;
		device = (domain_device*)get_device_by_id(&core->lib_dev,req->Device_Id, MV_FALSE); 
		if(NULL != device){
			device->status &= ~DEVICE_STATUS_WAIT_ASYNC;
			device->status |= DEVICE_STATUS_FUNCTIONAL;
		}
	}
#endif

	if(req->Completion != NULL)
		req->Completion(req->Cmd_Initiator, req);

	if (Req_Type == REQ_TYPE_CORE) {
		generic_post_callback(core, req);
	}
}

#if 0
//JING TESTING: the original code is wrong. I've modified some.
//But need check the spec for double verification.
//Is there any standard way to get the transfer count from the io chip.
MV_VOID DTM_hot_fix(MV_Request *req)
{
	MV_U32 tmp = 0;
	MV_U8 *buf = core_map_data_buffer(req);
	/*
	 * The data transfer length doesn't update when DTM do SAS Free Disks test.
	 * we need this way to pass DTM test
	 */
	switch (req->Cdb[0]) {
	case SCSI_CMD_INQUIRY:
		if (req->Cdb[2] == 0x00) {
			tmp = (MV_U32)buf[4] + 5;
		} else if (req->Cdb[2] == 0x80) {
			tmp = req->Data_Transfer_Length;
			if (tmp >= 4) {
				tmp = MV_MIN((tmp - 4), 20);
			}
		}
		req->Data_Transfer_Length = MV_MIN(req->Data_Transfer_Length, tmp);
		break;

	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
		if (req->Cdb[0] == SCSI_CMD_MODE_SENSE_6) {
			tmp = (MV_U32)buf[0] + 1;
		} else {/* Mode Sense 10 */
			tmp = ((MV_U32)buf[0] << 8) | ((MV_U32)buf[1] + 2);
		}
		req->Data_Transfer_Length = MV_MIN(req->Data_Transfer_Length, tmp);
		break;

	default:
		break;
	}
        core_unmap_data_buffer(req);
}
#endif

MV_VOID io_chip_complete_requests(core_extension *core)
{
	MV_Request *req;
	MV_ULONG flags;

	while (MV_TRUE) {
		//OSSW_SPIN_LOCK_COMPL_QUEUE(core, flags);
		OSSW_SPIN_LOCK(&core->cmpl_queue_SpinLock, flags);
		if (List_Empty(&core->complete_queue)) {
			//OSSW_SPIN_UNLOCK_COMPL_QUEUE(core, flags);
			OSSW_SPIN_UNLOCK(&core->cmpl_queue_SpinLock, flags);
			break;
		}
		req = (MV_Request *)List_GetFirstEntry(&core->complete_queue,
				MV_Request, Queue_Pointer);
		//OSSW_SPIN_UNLOCK_COMPL_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->cmpl_queue_SpinLock, flags);
		MV_DASSERT(req);

		/* ignore SAT with sense */
		/* too many print will cause Loki BIOS UI works slowly*/
		if (req->Scsi_Status != REQ_STATUS_SUCCESS && \
			req->Scsi_Status != REQ_STATUS_NO_DEVICE &&\
			!(req->Scsi_Status == REQ_STATUS_HAS_SENSE && \
				(req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16 || req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12))\
			&& !((req->Scsi_Status == REQ_STATUS_ERROR_WITH_SENSE) && (req->Cdb[0] == APICDB0_PD)&&
				(req->Cdb[1] == APICDB1_PD_GETHD_INFO)&&(core->device_id == 0x8180))) {
            scsi_log_err_cmd(__func__, core, req);
		}

        #ifdef DOUBLEBUF_4_SINGLE_PRD
            if (SCSI_IS_READ(req->Cdb[0])) {
                if (req->Data_Buffer) {
                    MV_CopyMemory(req->Data_Buffer, core->doublebuf_vir, req->Data_Transfer_Length);
                }
            }
        #endif
		core_return_finished_req(core, req);
	}
}

MV_VOID core_complete_requests(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;

        io_chip_complete_requests(core);

#ifdef HARDWARE_XOR
        xor_handle_cmpl_list(core);
#endif

}

MV_VOID core_complete_internal_requests(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_Request *req;
	MV_ULONG flags;
	
	while (MV_TRUE) {
		OSSW_SPIN_LOCK(&core->internal_compl_SpinLock, flags);
		if (List_Empty(&core->internal_compl_queue)) {
			OSSW_SPIN_UNLOCK(&core->internal_compl_SpinLock, flags);
			break;
		}
		//KeAcquireSpinLockAtDpcLevel(&core->internal_compl_SpinLock);
		req = (MV_Request *)List_GetFirstEntry(&core->internal_compl_queue,
				MV_Request, Queue_Pointer);
		//KeReleaseSpinLockFromDpcLevel(&core->internal_compl_SpinLock);
		OSSW_SPIN_UNLOCK(&core->internal_compl_SpinLock, flags);
		MV_DASSERT(req);

		/* ignore SAT with sense */
		if ((req->Scsi_Status != REQ_STATUS_SUCCESS) &&
			(req->Scsi_Status != REQ_STATUS_NO_DEVICE) &&
			!(req->Scsi_Status == REQ_STATUS_HAS_SENSE &&
				(req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16 ||
				req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12)) &&
			!((req->Scsi_Status == REQ_STATUS_ERROR_WITH_SENSE) &&
				(req->Cdb[0] == APICDB0_PD)&&
				(req->Cdb[1] == APICDB1_PD_GETHD_INFO)&&
				(core->device_id == 0x8180))) {
            scsi_log_err_cmd(__func__, core, req);
		}
        core_return_finished_req(core, req);
    }
}
#ifdef IGNORE_FIRST_PARITY_ERR
MV_VOID core_enable_intl_parity(MV_PVOID core_p){
    core_extension *core= (core_extension *)core_p;
    pl_root *root;
    MV_U16 root_id, phy_id;
    domain_phy *phy;
    MV_U32 partiy_enable_mask;
    for(root_id=0;root_id<core->chip_info->n_host;root_id++){
        root = &core->roots[root_id];
        partiy_enable_mask = 0;
        for(phy_id=0 ; phy_id<MAX_PHY_PER_PL; phy_id++){
            phy = &root->phy[phy_id];
            if(!phy->port)
                continue;
            partiy_enable_mask |= (1L<<(8+phy->asic_id));
        }
        root->parity_enable_mask |=partiy_enable_mask;
        core_sas_sata_intl_parity_enable(root, MV_TRUE, MV_TRUE);
    }
}
#endif
#if 0
MV_VOID core_push_queues(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_U32 queue_count1;

	if (!List_Empty(&core->event_queue))
		core_handle_event_queue(core);

	if (!Counted_List_Empty(&core->error_queue))
		core_handle_error_queue(core);

	if ((core->state == CORE_STATE_STARTED)) {
		core_handle_init_queue(core, MV_FALSE);
	} else if (core->is_dump) {
		/* for dump mode, do one by one to save memory */
		/* for hotplug, do init one by one because we need remap device id */
		core_handle_init_queue(core, MV_TRUE);
	} else {
		core_handle_init_queue(core, MV_FALSE);
	}

	do{
		core_handle_waiting_queue(core);

		queue_count1 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);

		/* maybe we have some instant request finished */
		core_complete_internal_requests(core);
		core_complete_requests(core);
	}while(Counted_List_GetCount(&core->waiting_queue, MV_FALSE) != queue_count1);

}
#else

MV_VOID core_check_init_done(core_extension *core)
{
	if (core->state == CORE_STATE_STARTING) {
		if (core->init_queue_count == 0) {
			/* There is a possibility for init requests outstanding */
			if (core_check_outstanding_req(core) != 0)
				return;

			MV_ASSERT(Counted_List_Empty(&core->init_queue));
			core->state = CORE_STATE_STARTED;
#ifdef SUPPORT_PHY_POWER_MODE
			if((core->PHY_power_mode_HIPM != 0) && (core->PHY_power_mode_timer_index == 0xffff)){
				core->PHY_power_mode_timer_index = Timer_AddRequest(core, 2, Core_EnablePowerStateHIPM, core, NULL);
			}
#endif
			core_io_chip_init_done(core, MV_FALSE);

			/* RAID will send request down in notify function.*/
			/* ikkwong: TBD if there are other modules */
			CORE_DPRINT((" finished init, notify upper layer. \n" ));
			core_start_cmpl_notify(core);
		}
	}
}

MV_VOID core_push_core_queues(core_extension *core)
{
	MV_BOOLEAN need_push = MV_FALSE;
	MV_ULONG flags, flags_global;
	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	if (core->core_queue_running) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}

	if (!List_Empty(&core->event_queue)
		|| !Counted_List_Empty(&core->error_queue)
		|| !Counted_List_Empty(&core->init_queue)) {
		need_push = MV_TRUE;
	}

	if (!need_push) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		core_check_init_done(core);
		return;
	}
	//core->core_queue_running = MV_TRUE;
	core->core_queue_running ++;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);

	OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags_global);
	if (!List_Empty(&core->event_queue)){
		//core_global_lock(core, &flags);
		core_handle_event_queue(core);
		//core_global_unlock(core, &flags);
	}
	
	if (!Counted_List_Empty(&core->error_queue)){
		//core_global_lock(core, &flags);
		core_handle_error_queue(core);
		//core_global_unlock(core, &flags);
	}
	if ((core->state == CORE_STATE_STARTED)) {
		core_handle_init_queue(core, MV_FALSE);
	} else if (core->is_dump) {
		/* for dump mode, do one by one to save memory */
		/* for hotplug, do init one by one because we need remap device id */
		core_handle_init_queue(core, MV_TRUE);
	} else {
		core_handle_init_queue(core, MV_FALSE);
	}
	OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags_global);

	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	//core->core_queue_running = MV_FALSE;
	core->core_queue_running --;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
}
#if 1
MV_VOID core_handle_root_waiting_queue_ex(core_extension *core,pl_root *root)
{
	MV_ULONG flags;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
#endif

	/* don't push waiting queues if pause_waiting_queue. */
	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	if (core->pause_waiting_queue) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
	
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	if (core->submitting && !core->run_multi_thread_send_cmd){
		 StorPortIssueDpc(
	        hba->Device_Extension,
	        &hba->ReqHandlerDpc,
	        NULL,
	        NULL);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
#endif

	core->waiting_queue_running++;
	core->submitting = 1;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);



	/* io chip queue: high priority and low priority */
 
#ifdef ROOT_WAITING_QUEUE
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, NULL, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->roots[root->root_id].waiting_queue, &core->roots[root->root_id], &core->roots[root->root_id].waiting_queue_SpinLock);
	// io_chip_handle_waiting_queue(core, &core->roots[1].waiting_queue, &core->roots[1], &core->roots[1].waiting_queue_SpinLock);
#else
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->waiting_queue, &core->waiting_queue_SpinLock);
#endif

#ifdef HARDWARE_XOR
	/* xor queue */
	xor_handle_waiting_list(core);
#endif



	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	core->waiting_queue_running--;
	core->submitting = 0;
  #if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS) 
		if (core->run_multi_thread_send_cmd == MV_FALSE){
			MV_U8 is_not_empty = MV_FALSE, i =0;
			for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
				if (!Counted_List_Empty(&core->roots[i].waiting_queue)){
					is_not_empty = MV_TRUE;
					break;
				}
			}
			if (is_not_empty == MV_FALSE)
				core->run_multi_thread_send_cmd = MV_TRUE;
		} 
#endif
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
}
MV_VOID core_handle_waiting_queue_ex(core_extension *core)
{
	MV_ULONG flags;
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	PHBA_Extension hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
#endif

	/* don't push waiting queues if pause_waiting_queue. */
	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	if (core->pause_waiting_queue) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
	
#if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS)
	if (core->submitting && !core->run_multi_thread_send_cmd){
		StorPortIssueDpc(
	        hba->Device_Extension,
	        &hba->ReqHandlerDpc,
	        NULL,
	        NULL);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
#endif

	core->waiting_queue_running++;
	core->submitting = 1;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);



	/* io chip queue: high priority and low priority */
 
#ifdef ROOT_WAITING_QUEUE
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, NULL, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->roots[0].waiting_queue, &core->roots[0], &core->roots[0].waiting_queue_SpinLock);
	io_chip_handle_waiting_queue(core, &core->roots[1].waiting_queue, &core->roots[1], &core->roots[1].waiting_queue_SpinLock);
#else
	io_chip_handle_waiting_queue(core, &core->high_priority_queue, &core->high_priority_SpinLock);
	io_chip_handle_waiting_queue(core, &core->waiting_queue, &core->waiting_queue_SpinLock);
#endif

#ifdef HARDWARE_XOR
	/* xor queue */
	xor_handle_waiting_list(core);
#endif

	

	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	core->waiting_queue_running--;
	core->submitting = 0;
  #if defined(ATHENA_PERFORMANCE_TUNNING) && defined(_OS_WINDOWS) 
		if (core->run_multi_thread_send_cmd == MV_FALSE){
			MV_U8 is_not_empty = MV_FALSE, i =0;
			for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
				if (!Counted_List_Empty(&core->roots[i].waiting_queue)){
					is_not_empty = MV_TRUE;
					break;
				}
			}
			if (is_not_empty == MV_FALSE)
				core->run_multi_thread_send_cmd = MV_TRUE;
		} 
#endif
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
}
MV_VOID core_push_root_waiting_queues(core_extension *core,pl_root *root)
{
	MV_ULONG flags;
	MV_U32 queue_count1, queue_count2;

//	do {
		core_handle_root_waiting_queue_ex(core,root);
#ifdef ROOT_WAITING_QUEUE
		queue_count1 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count1 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count1 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif

		/* maybe we have some instant request finished */
		if (!List_Empty(&core->internal_compl_queue)) {
//			spin_lock_irqsave(&core->core_global_SpinLock, flags);
//			core_global_lock(core, &flags);
			OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			core_complete_internal_requests(core);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			core_global_unlock(core, &flags);
//			spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		}
		core_complete_requests(core);

#ifdef ROOT_WAITING_QUEUE
		queue_count2 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count2 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count2 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif
//	} while (queue_count1 != queue_count2);
}
MV_VOID core_push_waiting_queues(core_extension *core)
{
	MV_ULONG flags;
	MV_U32 queue_count1, queue_count2;

//	do {
		core_handle_waiting_queue_ex(core);
#ifdef ROOT_WAITING_QUEUE
		queue_count1 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count1 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count1 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif

		/* maybe we have some instant request finished */
		if (!List_Empty(&core->internal_compl_queue)) {
			OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			spin_lock_irqsave(&core->core_global_SpinLock, flags);
//			core_global_lock(core, &flags);
			core_complete_internal_requests(core);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			core_global_unlock(core, &flags);
//			spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		}
		core_complete_requests(core);

#ifdef ROOT_WAITING_QUEUE
		queue_count2 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count2 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count2 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif
//	} while (queue_count1 != queue_count2);
}
#else
MV_VOID core_push_waiting_queues(core_extension *core)
{
	MV_ULONG flags;
	MV_U32 queue_count1, queue_count2;

	do {
		core_handle_waiting_queue(core);
#ifdef ROOT_WAITING_QUEUE
		queue_count1 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count1 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count1 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif

		/* maybe we have some instant request finished */
		if (!List_Empty(&core->internal_compl_queue)) {
			core_global_lock(core, &flags);
			core_complete_internal_requests(core);
			core_global_unlock(core, &flags);
		}
		core_complete_requests(core);

#ifdef ROOT_WAITING_QUEUE
		queue_count2 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
              queue_count2 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
#else
		queue_count2 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
#endif
	} while (queue_count1 != queue_count2);
}
#endif
MV_VOID core_push_root_queues(MV_PVOID core_p,MV_PVOID root_p)
{
	MV_ULONG flags;
	MV_U32 queue_count1 = 0;
	core_extension *core = (core_extension *)core_p;
	pl_root *root = (pl_root *)root_p;
	if(!List_Empty(&core->event_queue) ||(core->state != CORE_STATE_STARTED) || !Counted_List_Empty(&core->error_queue) || !Counted_List_Empty(&core->init_queue)){
		OSSW_LOCAL_IRQ_SAVE(flags);
		core_push_core_queues(core);
		OSSW_LOCAL_IRQ_RESTORE(flags);
	}
	queue_count1 = Counted_List_GetCount(&core->roots[root->root_id].waiting_queue, MV_FALSE);
       queue_count1 += Counted_List_GetCount(&core->high_priority_queue, MV_FALSE);
	if(!List_Empty(&core->internal_compl_queue)|| !List_Empty(&core->complete_queue) || queue_count1){
		OSSW_LOCAL_IRQ_SAVE(flags);
		core_push_root_waiting_queues(core,root);
		OSSW_LOCAL_IRQ_RESTORE(flags);
	}
}
MV_VOID core_push_queues(MV_PVOID core_p)
{
	MV_ULONG flags;
	MV_U32 queue_count1 = 0;
	core_extension *core = (core_extension *)core_p;
	if(!List_Empty(&core->event_queue) || (core->state != CORE_STATE_STARTED) || !Counted_List_Empty(&core->error_queue) || !Counted_List_Empty(&core->init_queue)){
		OSSW_LOCAL_IRQ_SAVE(flags);
		core_push_core_queues(core);
		OSSW_LOCAL_IRQ_RESTORE(flags);
	}
	queue_count1 = Counted_List_GetCount(&core->roots[0].waiting_queue, MV_FALSE);
       queue_count1 += Counted_List_GetCount(&core->roots[1].waiting_queue, MV_FALSE);
	queue_count1 += Counted_List_GetCount(&core->high_priority_queue, MV_FALSE);
	if(!List_Empty(&core->internal_compl_queue)|| !List_Empty(&core->complete_queue) || queue_count1){
		OSSW_LOCAL_IRQ_SAVE(flags);
		core_push_waiting_queues(core);
		OSSW_LOCAL_IRQ_RESTORE(flags);
	}
}
#endif

/*
* yuxl: add support for multi-waiting&complete-queue, which will improve the IOPS performance
*/
MV_VOID io_chip_complete_hw_requests(core_extension *core, pl_queue *queue)
{
	MV_Request *req;
	MV_ULONG flags;

	while (MV_TRUE) {
		OSSW_SPIN_LOCK_HW_CQ(core, flags, queue->msix_idx);
		if (List_Empty(&queue->complete_queue)) {
			OSSW_SPIN_UNLOCK_HW_CQ(core, flags, queue->msix_idx);
			break;
		}
		req = (MV_Request *)List_GetFirstEntry(&queue->complete_queue,
				MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK_HW_CQ(core, flags, queue->msix_idx);
		MV_DASSERT(req);

		/* ignore SAT with sense */
		/* too many print will cause Loki BIOS UI works slowly*/
		if (req->Scsi_Status != REQ_STATUS_SUCCESS && \
			req->Scsi_Status != REQ_STATUS_NO_DEVICE &&\
			!(req->Scsi_Status == REQ_STATUS_HAS_SENSE && \
				(req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16 || req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12))\
			&& !((req->Scsi_Status == REQ_STATUS_ERROR_WITH_SENSE) && (req->Cdb[0] == APICDB0_PD)&&
				(req->Cdb[1] == APICDB1_PD_GETHD_INFO)&&(core->device_id == 0x8180))) {
            scsi_log_err_cmd(__func__, core, req);
		}

        #ifdef DOUBLEBUF_4_SINGLE_PRD
            if (SCSI_IS_READ(req->Cdb[0])) {
                if (req->Data_Buffer) {
                    MV_CopyMemory(req->Data_Buffer, core->doublebuf_vir, req->Data_Transfer_Length);
                }
            }
        #endif
		core_return_finished_req(core, req);
	}
}

MV_VOID core_complete_hw_requests(core_extension *core, pl_queue *queue)
{

       	io_chip_complete_hw_requests(core, queue);

#ifdef HARDWARE_XOR
        xor_handle_cmpl_list(core);
#endif
}

MV_VOID core_complete_single_queue(MV_PVOID core_p, MV_PVOID queue_p)
{
	core_extension *core = (core_extension *)core_p;
	pl_queue *queue = (pl_queue *)queue_p;

	if (core->enable_multi_wcq)
		core_complete_hw_requests(core, queue);
	else
		core_complete_requests(core_p);
}

MV_VOID core_complete_base_queue(MV_PVOID core_p, MV_PVOID base_p)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base = (domain_base *)base_p;
	pl_queue *queue = core_get_hw_queue_of_base(core, base);

	core_complete_single_queue(core_p, queue);
}
#ifdef _OS_LINUX
MV_VOID io_chip_handle_hw_waiting_queue(core_extension *core,
					pl_queue *queue, Counted_List_Head *head, spinlock_t *lock)
#else
MV_VOID io_chip_handle_hw_waiting_queue(core_extension *core,
					pl_queue *queue, Counted_List_Head *head, PKSPIN_LOCK lock)
#endif
{
	MV_Request *req;
	MV_QUEUE_COMMAND_RESULT result;
	List_Head tmp_queue;
	domain_base *base = NULL;
	MV_ULONG flags;
	
	MV_LIST_HEAD_INIT(&tmp_queue);
	
    /*
    if one device returned MV_QUEUE_COMMAND_RESULT_FULL, 
    should block other requests on the same device. 
    but don't block other devices
    */
	while (MV_TRUE) {
		//OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		OSSW_SPIN_LOCK(lock, flags);
		if(Counted_List_Empty(head)) {
			OSSW_SPIN_UNLOCK(lock, flags);
			//OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
			break;
		}

		req = (MV_Request *)Counted_List_GetFirstEntry(
			head, MV_Request, Queue_Pointer);
		OSSW_SPIN_UNLOCK(lock, flags);
		//OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);		

		//OSSW_SPIN_LOCK_HW_QUEUE(core, flags, queue->msix_idx);
   		result = core_send_request(core, req);
   		if (result == MV_QUEUE_COMMAND_RESULT_SENT) {
   			//mv_add_timer(core, req);
   			//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, queue->msix_idx);
   			continue;
   		}
		//OSSW_SPIN_UNLOCK_HW_QUEUE(core, flags, queue->msix_idx);

		switch ( result )
		{
		case MV_QUEUE_COMMAND_RESULT_FINISHED:
			core_queue_completed_req(core, req);
			break;

		case MV_QUEUE_COMMAND_RESULT_FULL:
			OSSW_SPIN_LOCK(lock, flags);
			List_Add(&req->Queue_Pointer, &tmp_queue);
			OSSW_SPIN_UNLOCK(lock, flags);
			base = (domain_base *)get_device_by_id(&core->lib_dev,
						req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
			MV_DASSERT(base != NULL);
			//OSSW_SPIN_LOCK_Device(core, flags, base);
			OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
			base->blocked = MV_TRUE;
			//OSSW_SPIN_UNLOCK_Device(core, flags, base);
			OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_BLOCKED:
			OSSW_SPIN_LOCK(lock, flags);
			List_Add(&req->Queue_Pointer, &tmp_queue);
			OSSW_SPIN_UNLOCK(lock, flags);
			break;

		case MV_QUEUE_COMMAND_RESULT_NO_RESOURCE:
			OSSW_SPIN_LOCK(lock, flags);
			List_Add(&req->Queue_Pointer, &tmp_queue);
			OSSW_SPIN_UNLOCK(lock, flags);
			goto ending;

#if 0
		case MV_QUEUE_COMMAND_RESULT_SENT:
			mv_add_timer(core, req);
			break;
#endif
		case MV_QUEUE_COMMAND_RESULT_REPLACED:
			break;

		default:
			MV_ASSERT(MV_FALSE);
		}
	}

ending:
	while (!List_Empty(&tmp_queue)) {
		req = (MV_Request *)List_GetFirstEntry(
			&tmp_queue, MV_Request, Queue_Pointer);
		base = (domain_base *)get_device_by_id(&core->lib_dev,
					req->Device_Id, CORE_IS_ID_MAPPED(req), ID_IS_OS_TYPE(req));
		MV_DASSERT(base != NULL);
		//OSSW_SPIN_LOCK_Device(core, flags, base);
		OSSW_SPIN_LOCK( &base->base_SpinLock, flags);
		base->blocked = MV_FALSE;
		//OSSW_SPIN_UNLOCK_Device(core, flags, base);
		OSSW_SPIN_UNLOCK( &base->base_SpinLock, flags);
		OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
		OSSW_SPIN_LOCK(lock, flags);
		Counted_List_Add(&req->Queue_Pointer, head);
		OSSW_SPIN_UNLOCK(lock, flags);
		OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);
	}
}

MV_VOID core_handle_hw_waiting_queue(core_extension *core, pl_queue *queue)
{
	MV_ULONG flags;

	/* don't push waiting queues if pause_waiting_queue. */
	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	if (core->pause_waiting_queue) {
		//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
		OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
		return;
	}
	core->waiting_queue_running++;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);

	core->submitting = 1;

	/* io chip queue: high priority and low priority */
	io_chip_handle_hw_waiting_queue(core, queue, &queue->high_priority_queue, &queue->high_priority_SpinLock);
	io_chip_handle_hw_waiting_queue(core, queue, &queue->waiting_queue, &queue->waiting_queue_SpinLock);

#ifdef HARDWARE_XOR
	/* xor queue */
	xor_handle_waiting_list(core);
#endif

	core->submitting = 0;

	//OSSW_SPIN_LOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_LOCK(&core->core_queue_running_SpinLock, flags);
	core->waiting_queue_running--;
	//OSSW_SPIN_UNLOCK_CORE_QUEUE(core, flags);
	OSSW_SPIN_UNLOCK(&core->core_queue_running_SpinLock, flags);
}

MV_VOID core_push_hw_waiting_queue(core_extension *core, pl_queue *queue)
{
	MV_ULONG flags;
	MV_U32 queue_count1, queue_count2;

	do {
		core_handle_hw_waiting_queue(core, queue);

		queue_count1 = Counted_List_GetCount(&queue->waiting_queue, MV_FALSE);

		/* maybe we have some instant request finished */
		if (!List_Empty(&core->internal_compl_queue)) {
//			spin_lock_irqsave(&core->core_global_SpinLock, flags);
//			core_global_lock(core, &flags);
			OSSW_SPIN_LOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
			core_complete_internal_requests(core);
			OSSW_SPIN_UNLOCK_GLOBAL(core, &core->core_global_SpinLock, flags);
//			core_global_unlock(core, &flags);
//			spin_unlock_irqrestore(&core->core_global_SpinLock, flags);
		}
		core_complete_single_queue(core, queue);
		queue_count2 = Counted_List_GetCount(&queue->waiting_queue, MV_FALSE);

	} while (queue_count1 != queue_count2);
}

MV_VOID core_push_single_queue(core_extension *core, pl_queue *queue)
{
	MV_ULONG flags;
	if (!core->enable_multi_wcq) {
		core_push_queues(core);
		return;
	}
	if (!queue)
		return;
	OSSW_LOCAL_IRQ_SAVE(flags);
	core_push_core_queues(core);
	core_push_hw_waiting_queue(core, queue);
	OSSW_LOCAL_IRQ_RESTORE(flags);
}

MV_VOID core_push_base_queue(core_extension *core, domain_base *base)
{
	pl_queue *queue = NULL;

	queue = core_get_hw_queue_of_base(core, base);

	if (core->enable_multi_wcq)
		core_push_single_queue(core, queue);
	else
		core_push_queues(core);
}

MV_VOID core_push_all_queues(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	pl_root *root = NULL;
	MV_ULONG flags;
	MV_U8 i, j;

	if (!core->enable_multi_wcq) {
		core_push_queues(core);
		return;
	}

	OSSW_LOCAL_IRQ_SAVE(flags);
	core_push_core_queues(core);
	for (i = 0; i < MAX_NUMBER_IO_CHIP; i++) {
		root = &core->roots[i];
		for (j = 0; j < root->queue_num; j++)
			core_push_hw_waiting_queue(core, &root->queues[j]);
	}
	core_push_core_queues(core);

	OSSW_LOCAL_IRQ_RESTORE(flags);
}

MV_VOID core_send_mv_request(MV_PVOID core_p, MV_PVOID req_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_Request *req;
	pl_queue *queue = NULL;
	if (core->enable_multi_wcq) {
		req = (MV_Request *)req_p;
		queue = (pl_queue *)core_get_hw_queue(core, req);
		if (!queue) {
			req->Completion(req->Cmd_Initiator, req);
			return;
		}
	}

	core_push_single_queue(core, queue);
}
#if 0
MV_BOOLEAN core_append_queue_cmpl(core_extension *core, PMV_Request req)
{
	pl_queue *queue = NULL;
	MV_ULONG flags;

	if (!core->enable_multi_wcq)
		return MV_FALSE;

	queue = core_get_hw_queue(core, req);
	OSSW_SPIN_LOCK_HW_CQ(core, flags, queue->msix_idx);
	List_AddTail(&req->Queue_Pointer, &queue->complete_queue);
	OSSW_SPIN_UNLOCK_HW_CQ(core, flags, queue->msix_idx);

	return MV_TRUE;
}
#endif
MV_BOOLEAN core_append_queue_request(core_extension *core, 
						PMV_Request req, MV_BOOLEAN is_high)
{
	pl_queue *queue = NULL;
	MV_ULONG flags;

	if (!core->enable_multi_wcq)
		return MV_FALSE;

	queue = core_get_hw_queue(core, req);
	//OSSW_SPIN_LOCK_HW_WQ(core, flags, queue->msix_idx);
	OSSW_SPIN_LOCK(&queue->high_priority_SpinLock, flags);
	Counted_List_AddTail(&req->Queue_Pointer,
		is_high? &queue->high_priority_queue : &queue->waiting_queue);
	OSSW_SPIN_UNLOCK(&queue->high_priority_SpinLock, flags);
	//OSSW_SPIN_UNLOCK_HW_WQ(core, flags, queue->msix_idx);

	return MV_TRUE;
}
/*
*  end
*/

MV_U8 get_min_negotiated_link_rate(domain_port *port)
{
	pl_root *root = port->base.root;
	MV_U8 i, min_rate, rate = 0;
	MV_U32 tmp;
	domain_phy *phy;

	min_rate = PHY_LINKRATE_12;

	for (i=0; i<root->phy_num; i++) {
		if (port->phy_map & MV_BIT(i)) {
			phy = &root->phy[i];
			tmp = READ_PORT_PHY_CONTROL(root, phy);
			rate = get_phy_link_rate(tmp);
			if (rate < min_rate)
				min_rate = rate;
		}
	}

	return min_rate;
}
MV_VOID update_base_id(pl_root *root,domain_port *port,domain_device *dev,MV_U8 skip){
#if (defined(FIX_SCSI_ID_WITH_PHY_ID) || defined(FIXED_DEVICE_ID_6440))	
	core_extension * core = (core_extension *)root->core;
	if(!skip){
		if(core->device_id==DEVICE_ID_6440 || core->device_id==DEVICE_ID_6340 ||
			core->device_id==DEVICE_ID_6480|| core->device_id==DEVICE_ID_6485){
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=NULL;
			dev->base.id=port->phy->id;
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=&dev->base;
		}else if(IS_VANIR(core) || IS_ATHENA_CORE(core)){
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=NULL;
			dev->base.id=(MV_U16)(port->phy->id+root->base_phy_num);
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=&dev->base;
			}
		}
#endif
#ifdef SUPPORT_MUL_LUN
	if (dev->base.id != VIRTUAL_DEVICE_ID)		
		dev->base.TargetID = add_target_map(root->lib_dev->target_id_map, dev->base.id, MV_MAX_TARGET_NUMBER);
#endif
	core_set_hw_queue_of_base(root->core, &dev->base);
}
void sas_set_up_new_device(pl_root *root, domain_port *port, domain_device *dev)
{
    core_extension *core = (core_extension *)root->core;

	set_up_new_device(root, port, dev,
		(command_handler *)
		core_get_handler(root, HANDLER_SSP));

	/* we can't detect SGPIO, depend on HBA_INFO setting to enable/disable it */
    if (core->enable_sgpio)
        dev->connection = DC_SCSI | DC_SERIAL |DC_SGPIO;
	else
        dev->connection = DC_SCSI | DC_SERIAL;
    
	dev->dev_type = DT_DIRECT_ACCESS_BLOCK;

#ifdef SKIP_INTERNAL_INITIALIZE
	dev->state = DEVICE_STATE_SKIP_INIT_DONE;
#else
	dev->state = DEVICE_STATE_RESET_DONE;
#endif

	dev->base.queue_depth = CORE_SAS_DISK_QUEUE_DEPTH;
	dev->base.parent = &port->base;

	dev->sas_addr = port->att_dev_sas_addr;
	dev->negotiated_link_rate = port->link_rate;
	dev->capability |= DEVICE_CAPABILITY_RATE_1_5G;
	if (dev->negotiated_link_rate >= SAS_LINK_RATE_3_0_GBPS)
		dev->capability |= DEVICE_CAPABILITY_RATE_3G ;
	if (dev->negotiated_link_rate >= SAS_LINK_RATE_6_0_GBPS)
		dev->capability |= DEVICE_CAPABILITY_RATE_6G ;
	if (dev->negotiated_link_rate >= SAS_LINK_RATE_12_0_GBPS)
		dev->capability |= DEVICE_CAPABILITY_RATE_12G ;
    CORE_DPRINT(("dev%d type(0x%x), negotiated_link_rate(0x%x) capability(0x%x)\n",dev->base.id,dev->base.type,dev->negotiated_link_rate,dev->capability));

#ifdef SUPPORT_SGPIO
	dev->sgpio_drive_number = (MV_U8)port->base.id;
#ifdef SUPPORT_SGPIO_ACTIVE_LED
	dev->active_led_off_timer = NO_CURRENT_TIMER;
#endif
#endif
	dev->queue_type = TCQ_QUEUE_TYPE_SIMPLE;

}

void sas_init_port(pl_root *root, domain_port *port)
{
	domain_device *device = NULL;
	domain_expander *expander = NULL;
	core_extension *core = (core_extension *)root->core;
	MV_U8 i, phy_id = 0;
#ifdef SUPPORT_STAGGERED_SPIN_UP
	MV_ULONG flags;				
	struct device_spin_up *tmp=NULL;
#endif

	MV_ASSERT(port->phy_map);

	if (port->att_dev_info & (PORT_DEV_SMP_TRGT | PORT_DEV_STP_TRGT)) {
		expander = get_expander_obj(root, root->lib_rsrc);
		if (expander == NULL) {
			CORE_DPRINT(("Ran out of expanders. Aborting.\n"));
			return;
		}
		set_up_new_expander(root, port, expander);
		expander->base.parent = &port->base;
		if (core->state == CORE_STATE_STARTED) {
			expander->need_report_plugin = MV_TRUE;
		}
		expander->has_been_setdown = MV_FALSE;

		expander->sas_addr = port->att_dev_sas_addr;
		expander->neg_link_rate = get_min_negotiated_link_rate(port);
		List_AddTail(&expander->base.queue_pointer, &port->expander_list);
		port->expander_count++;

		expander->parent_phy_count = 0;
		for (i=0; i<root->phy_num; i++) {
			if (port->phy_map & MV_BIT(i)) {
				expander->parent_phy_id[phy_id] = i;
				expander->parent_phy_count++;
				phy_id++;
			}
		}

		core_queue_init_entry(root, &expander->base, MV_TRUE);
	} else if (port->att_dev_info & PORT_DEV_SSP_TRGT) {
		device = get_device_obj(root, root->lib_rsrc);
		if (device == NULL) {
			CORE_DPRINT(("no more free device\n"));
			return;
		}
		sas_set_up_new_device(root, port, device);
		update_base_id(root,port,device,MV_FALSE);
		List_AddTail(&device->base.queue_pointer, &port->device_list);
		port->device_count++;
#ifndef SUPPORT_STAGGERED_SPIN_UP
		core_queue_init_entry(root, &device->base, MV_TRUE);
#else
		tmp=get_spin_up_device_buf(root->lib_rsrc);
		if(!tmp || (core->spin_up_group == 0)){
			if(tmp){
				free_spin_up_device_buf(root->lib_rsrc, tmp);
			}
			core_queue_init_entry(root, &device->base, MV_TRUE);
		}else{
			MV_LIST_HEAD_INIT(&tmp->list);
			tmp->roots=root;
			tmp->base=&device->base;
			
			OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
			List_AddTail(&tmp->list,&core->device_spin_up_list);
			if(core->device_spin_up_timer ==NO_CURRENT_TIMER) {
				core->device_spin_up_timer=core_add_timer(core, 3,
					(MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler,
					&device->base, NULL);
		}
			OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
	}
				
#endif
	}
}
#ifdef SUPPORT_MUL_LUN
void sas_init_target_device(pl_root *root, domain_port *port, domain_device *dev, MV_U16 targetid, MV_U16 lun)
{
	domain_device *device = NULL;
	domain_expander *expander = NULL;
	core_extension *core = (core_extension *)root->core;
	MV_U8 i, phy_id = 0;
		device = get_device_obj(root, root->lib_rsrc);
		if (device == NULL) {
			CORE_DPRINT(("no more free device\n"));
			return;
		}
		sas_set_up_new_device(root, port, device);
		update_base_id(root,port,device,MV_TRUE);
		remove_target_map(root->lib_dev->target_id_map, device->base.TargetID, MV_MAX_TARGET_NUMBER);
		device->base.LUN = lun;
		device->base.TargetID = targetid;
		device->sas_addr=dev->sas_addr;
		device->base.parent=dev->base.parent;
		CORE_DPRINT(("Init target device Target  ID %d ,  lun  %d\n",device->base.TargetID,device->base.LUN));
		List_AddTail(&device->base.queue_pointer, &port->device_list);
		port->device_count++;
		core_queue_init_entry(root, &device->base, MV_TRUE);
	
}
#endif

extern MV_BOOLEAN sata_port_store_sig(domain_port *port, MV_BOOLEAN from_sig_reg);
void sata_init_port(pl_root *root, domain_port *port)
{
	domain_pm *pm;
	domain_phy *phy = port->phy;
	CORE_DPRINT(("port %p\n", port));
	port->state = PORT_SATA_STATE_POWER_ON;

	/*
	 * we add on a dummy PM for port soft reset
	 */
	pm = get_pm_obj(root, root->lib_rsrc);
	if (pm == NULL) {
		CORE_DPRINT(("no more free pm\n"));
		return;
	}
	set_up_new_pm(root, port, pm);

	/* hardware won't clear the signature regsiter
	 * it may have the signature for the old hotplug */
	phy->sata_signature = 0xFFFFFFFF;

	port->state = PORT_SATA_STATE_POWER_ON;
	
#ifdef DISABLE_PM
        port->state = PORT_SATA_STATE_WAIT_SIG;
#endif
	core_queue_init_entry(root, &port->base, MV_TRUE);
}

void map_phy_id(pl_root *root);
#ifdef SUPPORT_PHY_POWER_MODE
MV_VOID Core_Enable_Receive_PHY_Power_Request(domain_phy *phy)
{
	pl_root *root = phy->root;
	MV_U32 tmp = 0;
	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
	tmp = READ_PORT_CONFIG_DATA(root, phy);
	if((tmp & PMODE_EN_SAS_SLUMBER) && (tmp & PMODE_EN_SAS_PARTIAL))
		return;
	tmp &= ~(PMODE_EN_MASK);
	tmp |= (PMODE_EN_SATA_SLUMBER|PMODE_EN_SATA_PARTIAL);
	WRITE_PORT_CONFIG_DATA(root, phy,tmp);
}
MV_VOID Core_Disable_PHY_Interrupt(domain_phy *phy)
{
	pl_root *root = phy->root;
	if(phy->phy_irq_mask & (IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0)){
		phy->phy_irq_mask &= ~(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
		WRITE_PORT_CONFIG_DATA(root, phy,phy->phy_irq_mask);
	}

}
MV_VOID Core_Enable_PHY_Interrupt(domain_phy *phy)
{
	pl_root *root = phy->root;	
	if(!(phy->phy_irq_mask & IRQ_PHY_RDY_CHNG_MASK) || !(phy->phy_irq_mask & IRQ_PHY_RDY_CHNG_1_TO_0)){
		phy->phy_irq_mask |= (IRQ_PHY_RDY_CHNG_MASK| IRQ_PHY_RDY_CHNG_1_TO_0);
		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PORT_IRQ_MASK);
		WRITE_PORT_CONFIG_DATA(root, phy,phy->phy_irq_mask);
	}
}
MV_BOOLEAN Core_PHYEnablePowerState(
	MV_PVOID phy_p, MV_BOOLEAN ISSATA
	)
{
	
	domain_phy *phy = phy_p;
	pl_root *root = phy->root;
	core_extension *core = root->core;
	MV_U32 tmp = 0,tmp_ctl_power = 0,tmp_en_power = 0;
	MV_LPVOID mmio;
	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
	tmp = READ_PORT_CONFIG_DATA(root, phy);
	if(tmp & PMODE_STATUS_MASK){
		return MV_FALSE;//already in power mode by DIPM
	}
		
	if(core->PHY_power_mode_HIPM == 1){//partial
		if(ISSATA){
			tmp_ctl_power = PM_CNTRL_SATA_PARTIAL;
		}
		else{
			tmp_ctl_power = PM_CNTRL_SAS_PARTIAL;
		}
	}
	else if(core->PHY_power_mode_HIPM == 2){//slumber
		if(ISSATA){
			tmp_ctl_power = PM_CNTRL_SATA_SLUMBER;
		}
		else{
			tmp_ctl_power = PM_CNTRL_SAS_SLUMBER;
		}
	}
	else{
		return MV_FALSE;
	}
	//Core_Disable_PHY_Interrupt(phy);
	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_SATA_CONTROL);
	tmp = READ_PORT_CONFIG_DATA(root, phy);
	tmp &= ~(PM_CNTRL_MASK);
	tmp |= tmp_ctl_power;
	WRITE_PORT_CONFIG_DATA(root, phy,tmp);
	phy->In_PHY_power_mode_HIPM = core->PHY_power_mode_HIPM;
	MV_PRINT("root id %x PHY id %x Enter Power Mode(%x)!\n",root->root_id,phy->asic_id,core->PHY_power_mode_HIPM);
	core->PHY_power_mode_port_enabled |= (1<<(root->base_phy_num + phy->asic_id));//record the device enter DIPM or HIPM!
	tmp = READ_PORT_CONFIG_DATA(root, phy);
	if( (tmp& PMODE_STATUS_MASK)==0x0){//retry to enter power mode
		MV_PRINT("root id %x PHY id %x Power Mode Fail!\n",root->root_id,phy->asic_id);
		phy->In_PHY_power_mode_HIPM = 0;
		return MV_TRUE;
	}
	MV_PRINT("root id %x PHY id %x Power Mode OK!\n",root->root_id,phy->asic_id);
	return MV_FALSE;
}
MV_VOID Core_EnablePowerStateHIPM(
	MV_PVOID core_p,
	MV_PVOID context2
	)
{
	core_extension *core = (core_extension *)core_p;
	pl_root *root;
	domain_phy *phy = NULL;
	MV_U8 i,j;
	MV_U32 tmp = 0,tmp_ctl_power = 0,tmp_en_power = 0;
	MV_LPVOID mmio;
	domain_device *device;
	domain_port *port;
	MV_BOOLEAN need_timer = MV_FALSE;
	core->PHY_power_mode_timer_index = 0xffff;
	for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
		root = &core->roots[i];
		mmio = root->mmio_base;
		for (j = 0; j < root->phy_num; j++) {
			if(core->PHY_power_mode_port_map & MV_BIT(i*8+j)){
				phy = &root->phy[j];
				port = phy->port;
				if(port == NULL)//no device
					continue;
				if(port->device_count == 0)
					continue;
				if(!List_Empty(&port->expander_list)||port->pm)
					continue;
				if(List_Empty(&port->device_list))
					continue;
				if(phy->In_PHY_power_mode_HIPM)
					continue;
				LIST_FOR_EACH_ENTRY_TYPE(device,&port->device_list,domain_device,base.queue_pointer){
					if(device->base.type == BASE_TYPE_DOMAIN_DEVICE 
						&& (device->capability & DEVICE_CAPABILITY_HIPM_SUPPORTED)
						&& (phy->In_PHY_power_mode_HIPM == 0)){
						
						if(phy->PHY_power_mode_HIPM_timer == (core->PHY_power_mode_HIPM_timer-1)){
							need_timer = Core_PHYEnablePowerState(phy,IS_SATA(device));
						}else{
							phy->PHY_power_mode_HIPM_timer++;
							need_timer = MV_TRUE;
						}
					}
				}				
			}
		}
	}
	if(need_timer && core->PHY_power_mode_timer_index == 0xffff){
		core->PHY_power_mode_timer_index = Timer_AddRequest(core, 2, Core_EnablePowerStateHIPM, core, NULL);
	}
}
#endif

void io_chip_init(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	domain_phy *phy = NULL;
	domain_port *port = NULL;
	MV_U8 i, tmp_phy_map = 0;

	/* ASIC label phy id doesn't match with ASIC real id.
	 * set up the id */
	map_phy_id(root);
	io_chip_init_registers(root);

#ifdef SUPPORT_ACTIVE_CABLE
	/* I2C[1-4] control cable */
	for (i = 1; i < 3; i++) {
		cable_init(core, root->root_id * 2 + i);
	}
#endif

	// Maybe redundant to wait extra 1000ms, but just to be safe.
	core_sleep_millisecond(core, 1000);

	/* get phy information */
	for (i = 0; i < root->phy_num; i++) {
		phy = &root->phy[i];
		update_phy_info(root, phy);
		update_port_phy_map(root, phy);
	}

	for (i = 0; i < root->phy_num; i++) {
		port = &root->ports[i];
		if (port->type & PORT_TYPE_SATA)
			tmp_phy_map |= port->phy_map;
	}

	mv_reset_phy(root, tmp_phy_map, MV_TRUE);

	// Maybe redundant to wait extra 100ms, but just to be safe.
	core_sleep_millisecond(root->core, 1000);
#ifdef MV_DEBUG
    if(g_dump_register)
        core_dump_common_reg(root_p);
#endif

	/* to init port, discover first level devices */
	for (i = 0; i < root->phy_num; i++) {
#if 0 //def SUPPORT_PHY_POWER_MODE
		if(core->PHY_power_mode_DIPM || core->PHY_power_mode_HIPM){
			phy = &root->phy[i];
			Core_Disable_PHY_Interrupt(phy);
		}
#endif
		port = &root->ports[i];
		if (port->phy_num == 0) continue;

		if (port->type & PORT_TYPE_SAS) {
			sas_init_port(root, port);
		} else {
			sata_init_port(root, port);
		}
	}
}



void write_wide_port_register(pl_root *root, domain_port *port)
{
	MV_U8 i;
	MV_U32 reg;
	domain_phy *phy;

	/* for all phy participated, need update CONFIG_WIDE_PORT register */
	for (i = 0; i < root->phy_num; i++) {
		if (port->phy_map & MV_BIT(i)) {
			phy = &root->phy[i];
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WIDE_PORT);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			reg &= ~WIDE_PORT_PHY_MASK;
			reg |= port->asic_phy_map;
			WRITE_PORT_CONFIG_DATA(root, phy, reg);
		}
	}
}

void init_port(pl_root *root, domain_port *port)
{

#ifdef _OS_LINUX
	if (port->phy_num) {
		CORE_DPRINT(("port %p has phy_num %d, phy_map %X, att_dev_sas_addr %016llx.\n",port,port->phy_num,port->phy_map,port->att_dev_sas_addr.value));
	}

	port->phy_num = 0;
	port->phy_map = 0;
	port->asic_phy_map = 0;
	port->phy = NULL;
	port->att_dev_sas_addr.value =0;
#endif
	MV_ASSERT(port->phy_num == 0);
	MV_ASSERT(port->phy_map == 0);
	MV_ASSERT(port->asic_phy_map == 0);
	MV_ASSERT(port->phy == NULL);
	MV_ASSERT(U64_COMPARE_U32(port->att_dev_sas_addr, 0) == 0);

	port->base.type = BASE_TYPE_DOMAIN_PORT;
	port->base.port = port;
	port->base.root = root;

	MV_LIST_HEAD_INIT(&port->device_list);
	MV_LIST_HEAD_INIT(&port->expander_list);
	MV_LIST_HEAD_INIT(&port->current_tier);
	MV_LIST_HEAD_INIT(&port->next_tier);

	port->device_count = 0;
	port->expander_count = 0;
	port->init_count = 0;
}

domain_port * get_port(pl_root *root, domain_phy *phy)
{
	domain_port *port;

	/* wide port will search the existing ports first.
	 * if cannot find one, will come here */
	port = &root->ports[phy->id];
	init_port(root, port);

	CORE_DPRINT(("phy %d on port %d.\n", (root->base_phy_num+phy->id), port->base.id));

	return port;
}

void update_port_phy_map(pl_root *root, domain_phy *phy)
{
	domain_port *port;
	MV_U32 i;
	port = phy->port;

	/*
	 * the possible case code comes here,
	 * phy is coming,
	 * phy is changed, sas to sata, wide port to narrow, different attached sas
	 * phy is removed,
	 * for easy approach, deform the port and regenerate the port
	 */
	if (port != NULL) {
		/* deform the wide port */
		MV_DASSERT(port->phy_num >= 1);

		if (port->phy_num >= 1) {
			port->phy_num--;
			port->phy_map &= ~MV_BIT(phy->id);
			port->asic_phy_map &= ~MV_BIT(phy->asic_id);
		}
		// JING TBD
		phy->port = NULL;
		if (port->phy_num == 0) {
			MV_ASSERT(port->phy_map == 0);
			MV_ASSERT(port->asic_phy_map == 0);
			port->phy = NULL;
			port->dev_info = 0;
			port->att_dev_info = 0;
			MV_ZeroMemory(&port->att_dev_sas_addr, sizeof(port->att_dev_sas_addr));
			MV_ZeroMemory(&port->dev_sas_addr, sizeof(port->dev_sas_addr));
			
			/* dont set port to NULL,
			 * still use this port because of fields like device_list */
		} else {
			/* update phy and link_rate */
			port->phy = NULL;
			port->link_rate = SAS_LINK_RATE_12_0_GBPS;
			for (i = 0; i < root->phy_num; i++) {
				domain_phy *tmp_phy;
				tmp_phy = &root->phy[i];
				if (port->phy_map & MV_BIT(tmp_phy->id)) {
					if (port->phy == NULL) {
						port->phy = tmp_phy;
					}
					port->link_rate =
						MV_MIN(port->link_rate,
						get_phy_link_rate(tmp_phy->phy_status));
				}
			}
			write_wide_port_register(root, port);
			/* set port to NULL because following code will search ports.
			 * It'll attach to the port */
			port = NULL;
		}
	}

	if (0 == (phy->phy_status & SCTRL_PHY_READY_MASK))
		return;

	/* treat it as a new phy, if it's sas, try to find a wide port */
	if (phy->type & PORT_TYPE_SAS) {
		if (!(phy->att_dev_info &
			(PORT_DEV_SMP_TRGT | PORT_DEV_SSP_TRGT | PORT_DEV_STP_TRGT)))
			return;

		for (i = 0; i < root->phy_num; i++) {
			domain_port *tmp_port;
			tmp_port = &root->ports[i];
			if ((tmp_port->phy_num > 0)
				&& (tmp_port->type & PORT_TYPE_SAS)
				&& (U64_COMPARE_U64(
					tmp_port->att_dev_sas_addr, phy->att_dev_sas_addr)==0)
				&& (U64_COMPARE_U64(
					tmp_port->dev_sas_addr, phy->dev_sas_addr)==0))
			{
				/* find the wide port */
				phy->port = tmp_port;
				tmp_port->phy_num++;
				tmp_port->phy_map |= MV_BIT(phy->id);
				tmp_port->asic_phy_map |= MV_BIT(phy->asic_id);
				MV_ASSERT(tmp_port->phy != NULL);
				tmp_port->link_rate =
					MV_MIN(tmp_port->link_rate,
					get_phy_link_rate(phy->phy_status));
				write_wide_port_register(root, tmp_port);
				return;
			}

		}
	}

	/* either a new port, or just update the narrow port information */
	if (port == NULL) {
		port = get_port(root, phy);
	}
	MV_ASSERT(port != NULL);
	phy->port = port;	
	port->type = phy->type;
	MV_ASSERT(port->phy_num == 0);
	port->phy_num = 1;
	port->phy_map = MV_BIT(phy->id);
	port->asic_phy_map = MV_BIT(phy->asic_id);
	port->phy = &root->phy[phy->id];
	port->link_rate = get_phy_link_rate(phy->phy_status);
	port->dev_info = phy->dev_info;
	MV_CopyMemory(&port->dev_sas_addr, &phy->dev_sas_addr,
		sizeof(phy->dev_sas_addr));
	port->att_dev_info = phy->att_dev_info;
	MV_CopyMemory(&port->att_dev_sas_addr, &phy->att_dev_sas_addr,
		sizeof(phy->att_dev_sas_addr));

	write_wide_port_register(root, port);
}

MV_VOID sas_port_reset(pl_root *root, domain_port *port)
{

}

