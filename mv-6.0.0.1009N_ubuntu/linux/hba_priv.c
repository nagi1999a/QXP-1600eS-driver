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
#include "hba_api.h"
#include "hba_mod.h"
#include "com_struct.h"
#include "com_nvram.h"
#if defined(SUPPORT_MODULE_CONSOLIDATE)
#include "cc.h"
#endif		/* SUPPORT_MODULE_CONSOLIDATE */

#ifdef CORE_NO_RECURSIVE_CALL
 void Core_Flash_BIOS_Version(MV_PVOID extension,PMV_Request req);
#endif /* CORE_NO_RECURSIVE_CALL */
 void raid_capability( MV_PVOID This, PAdapter_Info pAdInfo);

/* helper functions related to HBA_ModuleSendRequest */
void mvGetAdapterInfo( MV_PVOID This, PMV_Request pReq )
{
	PHBA_Extension pHBA = (PHBA_Extension)This;
	MV_PVOID pCore = HBA_GetModuleExtension(This,MODULE_CORE);

	MV_U8 i;
	HBA_Info_Page HBA_Info_Param;

	PAdapter_Info pAdInfo;
	/* initialize */
	pAdInfo = (PAdapter_Info)pReq->Data_Buffer;
	MV_ZeroMemory(pAdInfo, sizeof(Adapter_Info));
	pAdInfo->DriverVersion.VerMajor = VER_MAJOR;
	pAdInfo->DriverVersion.VerMinor = VER_MINOR;
	pAdInfo->DriverVersion.VerOEM = VER_OEM;
	pAdInfo->DriverVersion.VerBuild = VER_BUILD;

	pAdInfo->SystemIOBusNumber = pHBA->Adapter_Bus_Number;
	pAdInfo->SlotNumber = pHBA->Adapter_Device_Number;
	pAdInfo->VenID = pHBA->Vendor_Id;
	pAdInfo->DevID = pHBA->Device_Id;
	pAdInfo->SubDevID = pHBA->Sub_System_Id;
	pAdInfo->SubVenID = pHBA->Sub_Vendor_Id;
	pAdInfo->RevisionID = pHBA->Revision_Id;
	/*for api judge whether add sense data in event*/
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_EVENT_WITH_SENSE_CODE;
	/* for bios upload or download support */
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_BIOS_OPTION_SUPPORT;
	/* for compatible with thor can create muti vd on a pd */
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_NO_MUTIL_VD_PER_PD;
	pAdInfo->MaxBufferSize = 1;
	if ( pHBA->Device_Id == DEVICE_ID_THORLITE_2S1P ||
		pHBA->Device_Id == DEVICE_ID_THORLITE_2S1P_WITH_FLASH ) {
		pAdInfo->PortCount = 3;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_PATA;
	} else if (pHBA->Device_Id == DEVICE_ID_THORLITE_0S1P) {
		pAdInfo->PortCount = 1;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_PATA;
	} else if ((pHBA->Device_Id == DEVICE_ID_6440) ||
			   (pHBA->Device_Id == DEVICE_ID_6445) ||
			   (pHBA->Device_Id == DEVICE_ID_6340)||
			   (pHBA->Device_Id == DEVICE_ID_9440) ||
			   (pHBA->Device_Id == DEVICE_ID_9445)) {
		pAdInfo->PortCount = 4;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else if ((pHBA->Device_Id == DEVICE_ID_9340) ||
			   (pHBA->Device_Id == DEVICE_ID_9345)) {
		pAdInfo->PortCount = 4;
		pAdInfo->PortSupportType = HD_TYPE_SATA;
	} else if ((pHBA->Device_Id == DEVICE_ID_6485) ||
			(pHBA->Device_Id == DEVICE_ID_6480 )||
                           (pHBA->Device_Id == DEVICE_ID_9480) ||
                           (pHBA->Device_Id == DEVICE_ID_9485) ||
                           (pHBA->Device_Id == DEVICE_ID_948F)
                           ) {
		pAdInfo->PortCount = 8;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else if (pHBA->Device_Id == DEVICE_ID_6320) {
		pAdInfo->PortCount = 2;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else if ((pHBA->Device_Id == DEVICE_ID_ATHENA_1480) ||
			(pHBA->Device_Id == DEVICE_ID_ATHENA_1580) ||
			(pHBA->Device_Id == DEVICE_ID_ATHENA_1485)) {
		pAdInfo->PortCount = 8;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
		pAdInfo->AdvancedFeatures2 |= ADV_FEATURE_2_NO_FLASH_SUPPORT; //albert
	} else if ((pHBA->Device_Id == DEVICE_ID_ATHENA_1495)||(pHBA->Device_Id == DEVICE_ID_ATHENA_1475)) {
		pAdInfo->PortCount = 16;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
		pAdInfo->AdvancedFeatures2 |= ADV_FEATURE_2_NO_FLASH_SUPPORT;	//albert
	} else if (pHBA->Device_Id == DEVICE_ID_ATHENA_FPGA) {
		pAdInfo->PortCount = 1;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else {
		pAdInfo->PortCount = 5;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	}

	if(mv_nvram_init_param(pHBA, &HBA_Info_Param)) {
		for(i=0;i<=19;i++)
			pAdInfo->SerialNo[i] = HBA_Info_Param.Serial_Num[i];
		for(i=0;i<=19;i++)
			pAdInfo->ModelNumber[i] = HBA_Info_Param.model_number[i];
	}
	if(MAX_DEVICE_SUPPORTED_PERFORMANCE <= 0xFF)
		pAdInfo->MaxHD = (MV_U8)MAX_DEVICE_SUPPORTED_PERFORMANCE;
	else
		pAdInfo->MaxHD_Ext = MAX_DEVICE_SUPPORTED_PERFORMANCE;
	pAdInfo->MaxExpander = MAX_EXPANDER_SUPPORTED;
	pAdInfo->MaxPM = MAX_PM_SUPPORTED;
	
	pAdInfo->MaxSpeed = pHBA->pcie_max_lnk_spd;		/* PCIe */
	pAdInfo->MaxLinkWidth = pHBA->pcie_max_bus_wdth;
	pAdInfo->CurrentSpeed = pHBA->pcie_neg_lnk_spd;
	pAdInfo->CurrentLinkWidth = pHBA->pcie_neg_bus_wdth;
	
#if !defined(SUPPORT_MV_SAS_CONTROLLER)	
	pAdInfo->SystemIOBusNumber = pHBA->desc->hba_desc->dev->bus->number;
	pAdInfo->SlotNumber  = PCI_SLOT(pHBA->desc->hba_desc->dev->devfn);
	pAdInfo->InterruptLevel = pHBA->desc->hba_desc->dev->irq;
	pAdInfo->InterruptVector = pHBA->desc->hba_desc->dev->irq;
#endif

#ifdef RAID_DRIVER
	raid_capability(This,  pAdInfo);
#endif

#ifdef CORE_NO_RECURSIVE_CALL
	Core_Flash_BIOS_Version(pCore,pReq);
#endif
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
}

void HBA_ModuleSendRequest(MV_PVOID this, PMV_Request req)
{
	PHBA_Extension phba = (PHBA_Extension) this;

	if (phba->RunAsNonRAID) {
		switch (req->Cdb[0]) {
		case APICDB0_ADAPTER:
			if (req->Cdb[1] == APICDB1_ADAPTER_GETINFO) {
				mvGetAdapterInfo(phba, req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_GETCONFIG) {
				mvGetAdapterConfig(phba->p_raid_feature,req); //get adapter config for raid or non-raid
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_SETCONFIG) {
				mvSetAdapterConfig(phba->p_raid_feature,req); //set adapter config for raid or non-raid
				req->Completion(req->Cmd_Initiator, req);
			} else {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			}
			break;
		case APICDB0_EVENT:
			if (req->Cdb[1] == APICDB1_EVENT_GETEVENT)
				get_event(phba, req);
			else
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

			req->Completion(req->Cmd_Initiator, req);
			break;
		case APICDB0_LD:
		case APICDB0_BLOCK:
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			req->Completion(req->Cmd_Initiator, req);
			break;
		case APICDB0_PD:
				
	#ifdef SUPPORT_FLASH
		case APICDB0_FLASH:
	#endif
	#ifdef SUPPORT_PASS_THROUGH_DIRECT
		case APICDB0_PASS_THRU_CMD_SCSI:
		case APICDB0_PASS_THRU_CMD_ATA:
	#endif
	
	#ifdef SUPPORT_SES
		case API_SCSI_CMD_RCV_DIAG_RSLT:
		case API_SCSI_CMD_SND_DIAG:
	#endif
			/* submit to child layer */
			phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);

			break;
		case APICDB0_DBG:
			if (phba->test_enabled)
			{
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			}
			else
			{
			    req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			    req->Completion(req->Cmd_Initiator, req);
			}
			break;
			
		case APICDB0_IOCONTROL:
			if (req->Cdb[1] == APICDB1_GET_OS_DISK_INFO 
				|| req->Cdb[1] == APICDB1_SET_OS_DISK_QUEUE_TYPE) {
				/* submit to child layer */
				phba->desc->child->ops->module_sendrequest(
					phba->desc->child->extension,req);
			}
			break;

			/*non-raid, CORE module doesn't support REPORT_LUN */
		case SCSI_CMD_REPORT_LUN:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			} else {
				/* submit to child layer */
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			}

			break;
		/*non-raid, handle virtual device REQUEST SENSE cmd here. */
		case SCSI_CMD_REQUEST_SENSE:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				sg_map(req);
				if (req->Sense_Info_Buffer!= NULL) {
					((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70;		/* Current */
					((MV_PU8)req->Sense_Info_Buffer)[2] = 0x00;		/* Sense Key*/
					((MV_PU8)req->Sense_Info_Buffer)[7] = 0x00;		/* additional sense length */
					((MV_PU8)req->Sense_Info_Buffer)[12] = 0x00;		/* additional sense code */
				}
				sg_unmap(req);
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				req->Completion(req->Cmd_Initiator, req);
			} else {
				/* submit to child layer */
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			}
			break;
		default:
		/* submit to child layer */
		#ifdef SUPPORT_MODULE_CONSOLIDATE
				ModConsolid_ModuleSendRequest(phba->PCC_Extension, req);
		#else
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
		#endif
			break;
		}
	} else {
		switch (req->Cdb[0]) {
		case APICDB0_ADAPTER:
			if (req->Cdb[1] == APICDB1_ADAPTER_GETINFO) {
				mvGetAdapterInfo(phba, req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_GETCONFIG) {
				mvGetAdapterConfig(phba->p_raid_feature,req); //get adapter config for raid or non-raid
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_SETCONFIG) {
				mvSetAdapterConfig(phba->p_raid_feature,req); //set adapter config for raid or non-raid
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_POWER_STATE_CHANGE){
				/* submit to child layer */
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			} 
#ifdef SUPPORT_BOARD_ALARM
			  else if (req->Cdb[1] == APICDB1_ADAPTER_MUTE) {
				/* submit to child layer */
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			} 
#endif            
			  else {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			}
			break;
		case APICDB0_EVENT:
			if (req->Cdb[1] == APICDB1_EVENT_GETEVENT)
				get_event(phba, req);
			else
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

			req->Completion(req->Cmd_Initiator, req);
			break;
		default:
			/* submit to child layer */
#ifdef SUPPORT_MODULE_CONSOLIDATE
			ModConsolid_ModuleSendRequest(phba->PCC_Extension, req);
#else
			phba->desc->child->ops->module_sendrequest(
			phba->desc->child->extension,req);
#endif
			break;
		}
	}
}

 MV_BOOLEAN mv_nvram_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);

#ifdef RAID_DRIVER
 void mv_get_hba_page_info( MV_PVOID This, MV_U16 Device_Id, MV_U8 *NonRaid)
{
	HBA_Info_Page HBA_Info_Param;
	struct mv_adp_desc *pHBA=(struct mv_adp_desc *)This;
	switch ( Device_Id){
	case DEVICE_ID_THORLITE_0S1P:
	case DEVICE_ID_THORLITE_1S1P:
	case DEVICE_ID_THORLITE_2S1P:
	case DEVICE_ID_THORLITE_2S1P_WITH_FLASH :
	case DEVICE_ID_THOR_4S1P:
	case DEVICE_ID_THOR_4S1P_NEW:
		*NonRaid = 0;
		break;		
	case DEVICE_ID_6445:
	 /*case DEVICE_ID_6485:
		pHBA->RunAsNonRAID = 1;
		pHBA->RaidMode = SUPPORT_LD_MODE_JBOD;
		break;*/
    	case DEVICE_ID_6485:
	case DEVICE_ID_6340:
	case DEVICE_ID_6320:
	case DEVICE_ID_6480:
	case DEVICE_ID_6440:
	case DEVICE_ID_9480:
	case DEVICE_ID_9485:
	case DEVICE_ID_9440:
	case DEVICE_ID_9445:
	case DEVICE_ID_9340:
	case DEVICE_ID_9345:
	case DEVICE_ID_948F:
	case DEVICE_ID_8480:
	case DEVICE_ID_8180:
	case DEVICE_ID_ATHENA_FPGA:
	case DEVICE_ID_ATHENA_1480:
	case DEVICE_ID_ATHENA_1580:
	case DEVICE_ID_ATHENA_1475:
	case DEVICE_ID_ATHENA_1485:
	case DEVICE_ID_ATHENA_1495:
		/* Read HBA INFORMATION PAGE  */
		if( !mv_nvram_moddesc_init_param(pHBA, &HBA_Info_Param) ) {
			/* don't find the page information, set default value (running RAID0,1 mode) */
			HBA_Info_Param.RAID_Feature = RAID_FEATURE_ENABLE_RAID;
		}

		*NonRaid = (HBA_Info_Param.RAID_Feature & RAID_FEATURE_ENABLE_RAID) ?  0:1;
		MV_DPRINT(("RAID_Feature = 0x%x,RunAsNonRAID = 0x%x.\n",
			 HBA_Info_Param.RAID_Feature,*NonRaid));
		break;

	default:
		MV_DASSERT(0);
	}
}
#endif

 MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 max_io);
 MV_VOID Core_ModuleInitialize(MV_PVOID module, MV_U32 size, MV_U16 max_io);
 MV_VOID Core_ModuleStart(MV_PVOID This);
 MV_VOID Core_ModuleShutdown(MV_PVOID core_p);
 MV_VOID Core_ModuleNotification(MV_PVOID core_p, enum Module_Event event,
	 struct mod_notif_param *param);
 MV_VOID Core_ModuleSendRequest(MV_PVOID core_p, PMV_Request req);
 MV_VOID Core_ModuleReset(MV_PVOID core_p);
 MV_VOID Core_ModuleMonitor(MV_PVOID core_p);
 MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This);
#ifdef RAID_DRIVER
 void Core_ModuleSendXORRequest(MV_PVOID This, PMV_XOR_Request pXORReq);
#endif

#ifdef SUPPORT_TASKLET
MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This);
#endif

struct mv_module_ops *mv_core_register_module(void)
{
	static struct mv_module_ops __core_mod_ops = {
		.module_id              = MODULE_CORE,
		.get_res_desc           = Core_ModuleGetResourceQuota,
		.module_initialize      = Core_ModuleInitialize,
		.module_start           = Core_ModuleStart,
		.module_stop            = Core_ModuleShutdown,
		.module_notification    = Core_ModuleNotification,
		.module_sendrequest     = Core_ModuleSendRequest,
		.module_reset           = Core_ModuleReset,
		.module_monitor         = Core_ModuleMonitor,
	#ifdef SUPPORT_TASKLET
		.module_service_isr 	= Core_InterruptCheckIRQ,
	#else
		.module_service_isr     = Core_InterruptServiceRoutine,
	#endif
		#ifdef RAID_DRIVER
		.module_send_xor_request= Core_ModuleSendXORRequest,
		#endif /* RAID_DRIVER */
	};

	return &__core_mod_ops;
}




#ifdef CACHE_MODULE_SUPPORT
static inline MV_BOOLEAN __cache_ops_service_isr(MV_PVOID extension)
{
	return __ext_to_gen(extension)->desc->child->ops->module_service_isr(
	        __ext_to_gen(extension)->desc->child->extension);
}

  MV_U32 cache_module_resource_quota(enum Resource_Type type, MV_U16 maxoutstandingio);
void cache_module_init(MV_PVOID p_module, MV_U32 extensionsize, MV_U16 maxoutstandingio);
void cache_module_start(MV_PVOID p_module);
void cache_module_shutdown(MV_PVOID p_module);
void cache_module_notify(MV_PVOID This, enum Module_Event event, struct mod_notif_param* event_param);
void cache_module_sendreq(MV_PVOID p_module, PMV_Request p_req);
void cache_module_reset(MV_PVOID p_module);
void cache_module_monitor(MV_PVOID p_module);
static struct mv_module_ops cache_module_interface = {
	.module_id                = MODULE_CACHE,
	.get_res_desc            = cache_module_resource_quota,
	.module_initialize       = cache_module_init,
	.module_start             = cache_module_start,
	.module_stop             = cache_module_shutdown,
	.module_notification    = cache_module_notify,
	.module_sendrequest  = cache_module_sendreq,
	.module_reset            = cache_module_reset,
	.module_monitor        = cache_module_monitor,
	.module_service_isr    = __cache_ops_service_isr,
};


struct mv_module_ops *mv_cache_register_module(void)
{
        return &cache_module_interface;
}
#endif

#ifdef RAID_DRIVER
MV_U32 RAID_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 MaxOutstandingIO);
void RAID_ModuleInitialize(MV_PVOID ModulePointer, MV_U32 ExtensionSize, MV_U16 MaxOutstandingIO);
void RAID_ModuleStart(MV_PVOID ModulePointer);
void RAID_ModuleShutdown(MV_PVOID ModulePointer);
void RAID_ModuleNotification(MV_PVOID This,
			     enum Module_Event event,
			     struct mod_notif_param *param);
void RAID_ModuleSendRequest(MV_PVOID ModulePointer, PMV_Request pMvReq);
void RAID_ModuleReset(MV_PVOID ModulePointer);
void RAID_ModuleMonitor(MV_PVOID ModulePointer);


static 	inline MV_BOOLEAN __raid_ops_service_isr(MV_PVOID extension)
{
	return __ext_to_gen(extension)->desc->child->ops->module_service_isr(
		__ext_to_gen(extension)->desc->child->extension);
}

struct mv_module_ops __raid_mod_ops = {
	.module_id              = MODULE_RAID,
	.get_res_desc           = RAID_ModuleGetResourceQuota,
	.module_initialize      = RAID_ModuleInitialize,
	.module_start           = RAID_ModuleStart,
	.module_stop            = RAID_ModuleShutdown,
	.module_notification    = RAID_ModuleNotification,
	.module_sendrequest     = RAID_ModuleSendRequest,
	.module_reset           = RAID_ModuleReset,
	.module_monitor         = RAID_ModuleMonitor,
	.module_service_isr     = __raid_ops_service_isr,
};

struct mv_module_ops *mv_raid_register_module(void)
{
	return &__raid_mod_ops;
}
#endif	//#ifdef RAID_DRIVER

