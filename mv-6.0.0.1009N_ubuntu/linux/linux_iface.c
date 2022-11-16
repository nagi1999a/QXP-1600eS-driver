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
 * linux_iface.c - Kernel/CLI interface
 *
 */

#include <linux/hdreg.h>
#include <linux/spinlock.h>
//#include <linux/ata.h>
#include "linux_main.h"
#include "linux_iface.h"
#include "hba_mod.h"
#include <scsi/scsi_eh.h>
#ifndef __GFP_WAIT
/* __GFP_WAIT has been renamed in recent (4.x) kernels. */
#define __GFP_WAIT __GFP_RECLAIM
#endif
#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
struct mv_adp_desc *gl_hba_desc = NULL;
#endif
//#define SECTOR_SIZE 512
#define HBA_REQ_TIMER_IOCTL (15)

#define MV_DEVFS_NAME "mv"
#define IOCTL_BUF_LEN (1024*1024)

#ifdef RAID_DRIVER
#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
static struct class *apidev_class = NULL;
#endif
static long
mv_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int mv_open(struct inode *inode, struct file *file);
#if defined (HAVE_UNLOCKED_IOCTL) && !defined (__VMKLNX__)
static long mv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
static int mv_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		    unsigned long arg);
#endif /* HAVE_UNLOCKED_IOCTL */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,9)) \
   || defined(__VMKERNEL_MODULE__)
static loff_t
mvctl_llseek(struct file *file, loff_t offset, int origin)
{
    return -ESPIPE;
}
#define no_llseek mvctl_llseek
#endif

#ifdef MV_VMK_ESX35
static ssize_t
mv_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    printk(": ioctl WRITE not yet supported\n");
    return 0;
}
                                                                                                                        
static ssize_t
mv_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
    printk(": ioctl READ not yet supported\n");
    return 0;
}

#endif

#ifdef MV_VMK_ESX35
struct fasync_struct *async_queue=NULL;
static int
mv_release(struct inode *inode, struct file *filep)
{
	printk("%s() called\n", __FUNCTION__);
	return fasync_helper(-1, filep, 0, &async_queue);
}

static int
mv_fasync(int fd, struct file *filep, int mode)
{
	printk("%s() called\n", __FUNCTION__);
	return fasync_helper(fd, filep, mode, &async_queue);
}

static unsigned int mv_poll(struct file *file, struct poll_table_struct *wait) 
{
   return 0;
}
#endif

static struct file_operations mv_fops = {
	.owner   =    THIS_MODULE,
	.llseek	 =    no_llseek,
#ifndef __VMKLNX__
	.open    =    mv_open,
#endif
/*Build for RHEL48 failed, temp disable this struct definition, if VMWare Driver
    need use it, need separate with kernel version >= 2.6.11*/
#if 0//def CONFIG_COMPAT
	.compat_ioctl = mv_unlock_ioctl,
#endif
#if defined (HAVE_UNLOCKED_IOCTL) && !defined (__VMKLNX__)
	.unlocked_ioctl = mv_unlock_ioctl,
#else
	.ioctl   =  mv_ioctl,
#endif
#ifdef MV_VMK_ESX35
	.read =     mv_read,
	.write =    mv_write,
#endif

#ifdef MV_VMK_ESX35
	.poll   =    mv_poll,
#endif
#ifdef MV_VMK_ESX35
	.fasync  =    mv_fasync,
	.release =    mv_release
#endif
};
#endif /* RAID_DRIVER */

void ioctlcallback(MV_PVOID This, PMV_Request req)
{
	struct hba_extension *hba = (struct hba_extension *) This;

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

#ifdef RAID_DRIVER
static MV_U16 API2Driver_ID(MV_U16 API_ID)
{
	MV_U16	returnID = API_ID;
	returnID &= 0xfff;
	return returnID;
}

static LD_Info ldinfo[LDINFO_NUM] = {{0}};
static int mv_proc_ld_info(struct Scsi_Host *host)
{
	struct hba_extension *hba;
	PMV_Request req;
	MV_U8 Cdb[MAX_CDB_SIZE];
	MV_U16 LD_ID = 0XFF;
	MV_U16 i=0;
	int ret   = 0;

	Cdb[0] = APICDB0_LD;
	Cdb[1] = APICDB1_LD_GETINFO;
	Cdb[2] = LD_ID & 0xff;
	Cdb[3] = API2Driver_ID(LD_ID)>>8;

	hba = __mv_get_ext_from_host(host);
	if(NULL == hba){
		MV_DPRINT((  "Marvell : proc info , Invalid operation.\n"));
		return -EPERM;
	}

#ifdef USE_REQ_POOL
	req = hba_req_cache_alloc(hba);
#else
	req = res_get_req_from_pool(hba->req_pool);
#endif
	if (req == NULL) {
		return -1;
	}

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Device_Id = VIRTUAL_DEVICE_ID;
	req->Cmd_Flag = 0;

	if (SCSI_IS_READ(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DATA_IN;
	if (SCSI_IS_READ(Cdb[0]) || SCSI_IS_WRITE(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DMA;

	req->Data_Transfer_Length = LDINFO_NUM*sizeof(LD_Info);
	memset(ldinfo, 0, LDINFO_NUM*sizeof(LD_Info));
	for(i=0;i<LDINFO_NUM;i++)
		ldinfo[i].Status = LD_STATUS_INVALID;

	req->Data_Buffer = ldinfo;
	SGTable_Init(&req->SG_Table, 0);
	memcpy(req->Cdb, Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);

	req->LBA.value = 0;
	req->Sector_Count = 0;
	req->Completion = ioctlcallback;
	req->Req_Type = REQ_TYPE_INTERNAL;
	req->Scsi_Status = REQ_STATUS_PENDING;
#ifdef SUPPORT_TASKLET
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif
#endif

	spin_lock(&hba->desc->hba_desc->global_lock);
//	hba->Ioctl_Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 1);
#endif
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	spin_unlock(&hba->desc->hba_desc->global_lock);

#ifdef SUPPORT_TASKLET
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if ( !__hba_wait_for_atomic_timeout(&hba->desc->hba_desc->hba_ioctl_sync, HBA_REQ_TIMER_IOCTL*HZ) ) {
#else
	if (wait_for_completion_timeout(&hba->desc->hba_desc->ioctl_cmpl, HBA_REQ_TIMER_IOCTL*HZ) == 0) {
#endif
		MV_DPRINT((  "mv_proc_ld_info req timed out.\n"));
	        ret = -1;
#ifdef USE_REQ_POOL
		hba_req_cache_free(hba,req);
#else
		res_free_req_to_pool(hba->req_pool, req);
#endif
		goto clean_reqpool;
	}

clean_reqpool:
//	res_free_req_to_pool(hba->req_pool, req);
	return ret;
}


static char* mv_ld_status(int status)
{
	switch (status) {
	case LD_STATUS_FUNCTIONAL:
		return "online";
	case LD_STATUS_DEGRADE:
		return "degraded";
	case LD_STATUS_DELETED:
		return "deleted";
	case LD_STATUS_PARTIALLYOPTIMAL:
		return "partially optimal";
	case LD_STATUS_OFFLINE:
		return "offline";
	default:
		return "unknown";
	}
}

static char* mv_ld_raid_mode(int status)
{
	switch (status) {
	case LD_MODE_RAID0:
		return "RAID0";
	case LD_MODE_RAID1:
		return "RAID1";
	case LD_MODE_RAID10:
		return "RAID10";
	case LD_MODE_RAID1E:
		return "RAID1E";
	case LD_MODE_RAID5:
		return "RAID5";
	case LD_MODE_RAID50:
		return "RAID50";
	case LD_MODE_RAID6:
		return "RAID6";
	case LD_MODE_RAID60:
		return "RAID60";
	case LD_MODE_JBOD:
		return "JBOD";
	default:
		return "unknown";
	}
}

static char* mv_ld_bga_status(int status)
{
	switch (status) {
	case LD_BGA_STATE_RUNNING:
		return "running";
	case LD_BGA_STATE_ABORTED:
		return "aborted";
	case LD_BGA_STATE_PAUSED:
		return "paused";
	case LD_BGA_STATE_AUTOPAUSED:
		return "auto paused";
	case LD_BGA_STATE_DDF_PENDING:
		return "DDF pending";
	default:
		return "N/A";
	}
}

static int mv_ld_get_status(struct Scsi_Host *host, MV_U16 ldid, LD_Status *ldstatus)
{
	struct hba_extension *hba;
	PMV_Request req;
	MV_U8 Cdb[MAX_CDB_SIZE];
	MV_U16 LD_ID = ldid;/*0XFF;*/
	//unsigned long flags;
	int ret   = 0;

	hba = __mv_get_ext_from_host(host);
	if(NULL == hba){
		MV_DPRINT((  "Marvell : get ld status , Invalid operation.\n"));
		return -EPERM;
	}
	Cdb[0] = APICDB0_LD;
	Cdb[1] = APICDB1_LD_GETSTATUS;
	Cdb[2] = LD_ID & 0xff;
	Cdb[3] = API2Driver_ID(LD_ID)>>8;

#ifdef USE_REQ_POOL
	req = hba_req_cache_alloc(hba);
#else
	req = res_get_req_from_pool(hba->req_pool);
#endif
	if (req == NULL)
		return -1;

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Device_Id = VIRTUAL_DEVICE_ID;
	req->Cmd_Flag = 0;

	if (SCSI_IS_READ(Cdb[0]))
		req->Cmd_Flag |= CMD_FLAG_DATA_IN;
	if ( SCSI_IS_READ(Cdb[0]) || SCSI_IS_WRITE(Cdb[0]) )
		req->Cmd_Flag |= CMD_FLAG_DMA;

	/* Data Buffer */
	req->Data_Transfer_Length = sizeof(LD_Status);
	memset(ldstatus, 0, sizeof(LD_Status));
	ldstatus->Status = LD_STATUS_INVALID;
	req->Data_Buffer = ldstatus;

	SGTable_Init(&req->SG_Table, 0);
	memcpy(req->Cdb, Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);
	req->LBA.value = 0;
	req->Sector_Count = 0;
	req->Completion = ioctlcallback;
	req->Req_Type = REQ_TYPE_INTERNAL;
	req->Scsi_Status = REQ_STATUS_PENDING;
#ifdef SUPPORT_TASKLET
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif

#endif

	//spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);
	spin_lock(&hba->desc->hba_desc->global_lock);
//	hba->Ioctl_Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 1);
#endif
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	//spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);
	spin_unlock(&hba->desc->hba_desc->global_lock);

#ifdef SUPPORT_TASKLET
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (!__hba_wait_for_atomic_timeout(&hba->desc->hba_desc->hba_ioctl_sync, HBA_REQ_TIMER_IOCTL*HZ)) {
#else
	if (!wait_for_completion_timeout(&hba->desc->hba_desc->ioctl_cmpl, HBA_REQ_TIMER_IOCTL*HZ)) {
#endif
		MV_DPRINT(("mv_ld_get_status timed out.\n"));
	        ret = -1;

#ifdef USE_REQ_POOL
                hba_req_cache_free(hba,req);
#else
                res_free_req_to_pool(hba->req_pool, req);
#endif
		goto clean_reqpool;
	}

clean_reqpool:
	return ret;
}

static int mv_ld_show_status(char *buf, PLD_Status pld_status)
{
	char *str, *str1;
	int ret = 0;

	if ( LD_BGA_STATE_RUNNING == pld_status->BgaState)
	{
		if (LD_BGA_REBUILD == pld_status->Bga)
			str = "rebuilding";
		else if (LD_BGA_INIT_QUICK == pld_status->Bga ||
		          LD_BGA_INIT_BACK == pld_status->Bga)
			str = "initializing";
		else if (LD_BGA_CONSISTENCY_CHECK == pld_status->Bga ||
		          LD_BGA_CONSISTENCY_FIX == pld_status->Bga)
			str = "synchronizing";
		else if (LD_BGA_MIGRATION == pld_status->Bga)
			str = "migration";
		else
			str = "unknown bga action";
		ret = sprintf(buf, "  %s is %d%% done", str, pld_status->BgaPercentage);
	}
	else if ((LD_BGA_STATE_ABORTED == pld_status->BgaState) ||
	         (LD_BGA_STATE_PAUSED == pld_status->BgaState) ||
	         (LD_BGA_STATE_AUTOPAUSED == pld_status->BgaState))
	{
		if (LD_BGA_REBUILD == pld_status->Bga)
			str = "rebuilding";
		else if (LD_BGA_INIT_QUICK == pld_status->Bga ||
		         LD_BGA_INIT_BACK == pld_status->Bga)
			str = "initializing";
		else if (LD_BGA_CONSISTENCY_CHECK == pld_status->Bga ||
		          LD_BGA_CONSISTENCY_FIX == pld_status->Bga)
			str = "synchronizing";
		else if (LD_BGA_MIGRATION == pld_status->Bga)
			str = "migration";
		else
			str = "unknown bga action";

		if (LD_BGA_STATE_ABORTED == pld_status->BgaState)
			str1 = "aborted";
		else if (LD_BGA_STATE_PAUSED == pld_status->BgaState)
			str1 = "paused";
		else if (LD_BGA_STATE_AUTOPAUSED == pld_status->BgaState)
			str1 = "auto paused";
		else
			str1 = "aborted";
		ret = sprintf(buf, "  %s is %s", str, str1);
	}
	return ret;
}
#endif /*RAID_DRIVER*/

extern MV_BOOLEAN mv_read_hba_info( MV_PVOID This, char *pBuffer, int *offset);
extern MV_BOOLEAN mv_read_autoload_data( MV_PVOID This, char *pBuffer, int *offset);
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
static int mv_proc_hba_info(struct Scsi_Host *host, char *pBuffer)
{
    struct hba_extension *hba ;
	int len = 0;
	
    hba = __mv_get_ext_from_host(host);
	if (hba == NULL)
		return 0;
	
	if (!mv_read_hba_info(hba, pBuffer + len, &len))
	{
	    MV_DPRINT(("mv read hba info failed.\n"));
	}
	
	len += sprintf(pBuffer + len,"\n[AUTOLOAD CONFIG]\n");
    if (!mv_read_autoload_data(hba, pBuffer + len, &len))
	{
	    MV_DPRINT(("mv read autoload data failed.\n"));
	}
	
	return len;	
}

static int mv_char_to_int(char *buf, MV_U32 *value)
{
	char *tmp;	
	int i;
	//MV_DPRINT(("buf %s****.\n", buf));

	if (((buf[0] == '0') && (buf[1] == 'x')) ||
		((buf[0] == '0') && (buf[1] == 'X')))
		tmp = &buf[2];
	else
	{
		MV_DPRINT(("the buf format is wrong.\n"));
		return -1;
	}

	for (i = 0; tmp[i] != '\0'; i++)
	{		
		if ((tmp[i] >= '0') && (tmp[i] <= '9'))
			*value = (*value) * 16 + tmp[i] - '0';
		else if ((tmp[i] >= 'a') && (tmp[i] <= 'f'))
			*value = (*value) * 16 + tmp[i] - 'a' + 10;
		else if ((tmp[i] >= 'A') && (tmp[i] <= 'F'))
			*value = (*value) * 16 + tmp[i] - 'A' + 10;
		else
		{
			MV_DPRINT(("the data format is wrong.%c\n", tmp[i]));
			return -1;
		}			
	}

	return 0;
}
#endif
extern MV_LPVOID mv_get_mmio_base(void *ext);
extern int mv_read_type_reg(MV_LPVOID *mmio_base, char *buf, MV_U32 offset, char *type, int len);
extern void mv_write_type_reg(MV_LPVOID *mmio_base, MV_U32 addr_reg,MV_U32 value_reg, char *type);
extern int mv_dump_intr_reg(MV_LPVOID *mmio_base, char *pBuffer, int len);
extern int mv_dump_sas_sata_port_cfg_ctrl_reg(MV_LPVOID *mmio_base, char *pBuffer, int len);

static int (*pDumpRegFunc[])(MV_LPVOID *mmio_base,
							char *pBuffer,
							int len) =
{		
	mv_dump_intr_reg,
	mv_dump_sas_sata_port_cfg_ctrl_reg,
	NULL    // Last element must be a NULL value.
};
#if !defined(SUPPORT_MV_SAS_CONTROLLER)
static int mv_getDumpFuncCnt(void)
{
	int idx = 0;
	while (pDumpRegFunc[idx] != NULL)
		idx++;
	return(idx);
}

static void mv_handle_proc_write(char *msg, void *data)
{
	MV_LPVOID  mmio_base = NULL;
	struct proc_reg_data *proc_reg_data = (struct proc_reg_data *)data;
	struct hba_extension *hba =	proc_reg_data->hba;
	char type[5] = {0};
	char addr[8] = {0};
	char value[9] = {0};
	int i, j;
	MV_U32 addr_reg = 0;
	MV_U32 value_reg = 0;	

	mmio_base = mv_get_mmio_base(hba);
	if (NULL == mmio_base)
	{
		return ; 
	}

	//the msg format is : default
	if (MV_CompareMemory(msg, "default", 7)== 0)
	{ 
		proc_reg_data->flag = 0;
		return;
	}
	//the msg format is : [addr]:[value] or [addr]
	if ((msg[0] == '0') && ((msg[1] == 'x') || (msg[1] == 'X')))
	{
		for (i = 0, j = 0; msg[i] != ':' && msg[i] != '\0'; i++, j++)
			addr[j] = msg[i];
		if (msg[i] == ':')
		{
			addr[j] = '\0';
			for (j = 0, i++; msg[i] != '\0'; i++, j++)
				value[j] = msg[i];
			value[j - 1] = '\0';

			if (mv_char_to_int(addr, &addr_reg) != 0)
				return;

			if (mv_char_to_int(value, &value_reg) != 0)
				return;

			MV_DPRINT(("%s, type %s, addr %s, value %s, addr_reg 0x%x, value_reg 0x%x.\n", \
						__func__, type, addr, value, addr_reg, value_reg));

			MV_REG_WRITE_DWORD(mmio_base, addr_reg, value_reg);
		}
		else
		{
			addr[j - 1] = '\0';
			if (mv_char_to_int(addr, &addr_reg) != 0)
				return;			
		}
		proc_reg_data->flag = 1;
		proc_reg_data->offset = addr_reg;
		MV_CopyMemory(proc_reg_data->type, "NULL", 5);
		return;
	}
		
	//the msg format is : [type][addr]:[value] or [type][addr]
	for (i = 0; i < 4; i++)
		type[i] = msg[i];
	type[i] = '\0';

	for (j = 0, i = 4; msg[i] != ':' && msg[i] != '\0'; i++, j++)
		addr[j] = msg[i]; 
	if (msg[i] == ':')
	{
		addr[j] = '\0';
		for (j = 0, i++; msg[i] != '\0'; i++, j++)
			value[j] = msg[i];
		value[j - 1] = '\0';

		if (mv_char_to_int(addr, &addr_reg) != 0)
			return;

		if (mv_char_to_int(value, &value_reg) != 0)
			return;

		MV_DPRINT(("%s, type %s, addr %s, value %s, addr_reg 0x%x, value_reg 0x%x.\n", \
			__func__, type, addr, value, addr_reg, value_reg));

		if (MV_CompareMemory(type, "pcie", 5) == 0)
		{			
			MV_PCI_WRITE_CONFIG_DWORD(hba, addr_reg, value_reg);
		}
		else
		{
			mv_write_type_reg(mmio_base, addr_reg, value_reg, type);
		}		
	}
	else
	{
		addr[j - 1] = '\0';
		if (mv_char_to_int(addr, &addr_reg) != 0)
			return;			
	}

	proc_reg_data->flag = 1;
	proc_reg_data->offset = addr_reg;
	MV_CopyMemory(proc_reg_data->type, type, 5);
	return;	
}

int reg_info_read(char *page, char **start, off_t off, int count,
					int *eof, void *data)
{
	MV_LPVOID  mmio_base = NULL;
	struct proc_reg_data *proc_reg_data = (struct proc_reg_data *)data;
	struct hba_extension *hba =  proc_reg_data->hba;
	MV_U32 value = 0;
	int numFunc = 0;
	int i = 0;
	int len = 0;
	char help[] = {"usage:----------------------------------\n"
"1. echo default > [file]                 -- cat [file] show default regs value\n"
"2. echo [offset] > [file]                -- cat [file] show specificed reg value\n"
"3. echo [offset]:[value] > [file]        -- write value to the offset regs, cat [file]show its value\n"
"4. echo [type][offset] > [file]          -- the [type] contains: cmds/port/intr/comm/pcie, cat [file]show the offset reg value\n"
"5. echo [type][offset]:[value] > [file]  -- the [type] same with 4, write the value to the type offset reg\n"};

	mmio_base = mv_get_mmio_base(hba);
	if (NULL == mmio_base)
	{
		return 0; 
	}

	if (mv_get_register_mode() == 0)
	{
		return 0;
	}
	
	if (proc_reg_data->flag == 0)
	{
		numFunc = mv_getDumpFuncCnt();
		for (i = 0; i < numFunc; i++)
			len = (*pDumpRegFunc[i])(mmio_base, page, len);

		len += sprintf(page + len,"%s\n", help);		 
		//len += sprintf(page + len,"Now: {jiffies=%ld}\n",jiffies);
	}
	else
	{
		if (MV_CompareMemory(proc_reg_data->type, "NULL", 4) == 0)
		{
			len += sprintf(page + len, "Reg:0X%x Value:%x  \n",
				 proc_reg_data->offset, MV_REG_READ_DWORD(mmio_base, proc_reg_data->offset));
		}
		else
		{
			if (MV_CompareMemory(proc_reg_data->type, "pcie", 5) == 0)
			{
				MV_PCI_READ_CONFIG_DWORD(hba, proc_reg_data->offset, &value);
				len += sprintf(page + len, "PCIE Reg:0X%x  Value:%x  \n",
						 proc_reg_data->offset, value);
			}
			else
			{
			    len = mv_read_type_reg(mmio_base, page, proc_reg_data->offset, proc_reg_data->type, len);
			}
		}		
	}
	
	if (off > len)
		return 0;

	if (count > len - off)
		count = len - off;

	return off + count;
}


int reg_info_write(struct file *file, const char __user * buffer,
		unsigned long count, void *data)
{
	//MV_DPRINT(("hba_info_write is called.\n"));	
	char msg[80];
	if (copy_from_user(msg, buffer, count))
	{
		return -EFAULT;
	}
	msg[count] = '\0';

	if (mv_get_register_mode() != 0)
	{
		MV_DPRINT(("%s, The data is %s\n", __func__, msg));
		mv_handle_proc_write(msg, data);
	}
	return count;
}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int mv_linux_show_info(struct seq_file *m, struct Scsi_Host *pSHost)
{
	int len = 0;
	int datalen = 0;/*use as a temp flag.*/

#ifdef RAID_DRIVER
	int i = 0;
	int j = 0;
	int ret = -1;
	LD_Status ld_status;
	char *tmp = NULL;
	int tmplen = 0;
	struct hba_extension *hba;
#endif

	seq_printf(m, "Marvell %s Driver, Version %s\n",
		      mv_product_name, mv_version_linux);

#ifdef RAID_DRIVER
	hba = __mv_get_ext_from_host(pSHost);
	if (!hba->desc->hba_desc->RunAsNonRAID)
	{
		if (mv_proc_ld_info(pSHost) == -1) {
			len = sprintf(pBuffer,
				      "Driver is busy, please try later.\n");
			goto out;
		} else {
			for (i = 0; i < MAX_LD_SUPPORTED_PERFORMANCE; i++) {
				if (ldinfo[i].Status != LD_STATUS_INVALID) {
					if (ldinfo[i].Status == LD_STATUS_OFFLINE
					        && ldinfo[i].BGAStatus == LD_BGA_STATE_RUNNING) {
						ldinfo[i].BGAStatus = LD_BGA_STATE_AUTOPAUSED;
					}
					if (ldinfo[i].Status == LD_STATUS_MISSING) {
						ldinfo[i].Status = LD_STATUS_OFFLINE;
					}
				} else {
					break;
				}
			}
		}

		len += sprintf(pBuffer+len,"Index RAID\tStatus  \t\tBGA Status\n");
		for (i = 0 ; i < LDINFO_NUM ; i++) {
			if (ldinfo[i].Size.value == 0) {
				if (i == 0) {
					len += sprintf(pBuffer+len,"NO Logical Disk\n");
				}
				break;
			}
			/* Fixed issue -- An unknown item shown in /proc/scsi/mv64xx/. */
			if (ldinfo[i].Status == LD_STATUS_INVALID)
				continue;

			len += sprintf(pBuffer+len,
				"%-5d %s\t%s",
				ldinfo[i].ID,
				mv_ld_raid_mode(ldinfo[i].RaidMode),
				mv_ld_status(ldinfo[i].Status)
				);

			tmplen = 24 -strlen(mv_ld_status(ldinfo[i].Status));
			while (j < tmplen) {
				len += sprintf(pBuffer+len, "%s", " ");
				j++;
			}
			j = 0;

			len += sprintf(pBuffer+len, "%s", mv_ld_bga_status(ldinfo[i].BGAStatus));

			if (ldinfo[i].BGAStatus != LD_BGA_STATE_NONE) {
				ret = mv_ld_get_status(pSHost,ldinfo[i].ID,&ld_status);
				if (ret == 0) {
					if (ld_status.Status != LD_STATUS_INVALID) {
						if (ld_status.Status == LD_STATUS_MISSING)
							ld_status.Status = LD_STATUS_OFFLINE;
						ld_status.BgaState = ldinfo[i].BGAStatus;
					}
					len += mv_ld_show_status(pBuffer+len,&ld_status);
					ret = -1;
				}
			}

			tmp = NULL;
			tmplen = 0;
			len += sprintf(pBuffer+len,"\n");
		}
	}
	out:
#endif
#if 0
	if (mv_get_register_mode() != 0)
		len += mv_proc_hba_info(pSHost, pBuffer + len);

#endif
	
	return 0;
}
#else
#ifndef MV_VMK_ESX35
int mv_linux_proc_info(struct Scsi_Host *pSHost, char *pBuffer,
		       char **ppStart,off_t offset, int length, int inout)
#else
int mv_linux_proc_info(char *pBuffer, char **ppStart,off_t offset,
                        int length, int host_no, int inout)

#endif
{
	int len = 0;
	int datalen = 0;/*use as a temp flag.*/
#ifndef MV_VMK_ESX35
#ifdef RAID_DRIVER
	int i = 0;
	int j = 0;
	int ret = -1;
	LD_Status ld_status;
	char *tmp = NULL;
	int tmplen = 0;
	struct hba_extension *hba;
#endif
#endif
	if (
#ifndef MV_VMK_ESX35
		!pSHost ||
#endif
		 !pBuffer)
		return (-ENOSYS);

	if (inout == 1) {
		/* User write is not supported. */
		return (-ENOSYS);
	}

	len = sprintf(pBuffer,"Marvell %s Driver , Version %s\n",
		      mv_product_name, mv_version_linux);

#ifndef MV_VMK_ESX35
#ifdef RAID_DRIVER
	hba = __mv_get_ext_from_host(pSHost);
	if (!hba->desc->hba_desc->RunAsNonRAID)
	{
		if (mv_proc_ld_info(pSHost) == -1) {
			len = sprintf(pBuffer,
				      "Driver is busy, please try later.\n");
			goto out;
		} else {
			for (i = 0; i < MAX_LD_SUPPORTED_PERFORMANCE; i++) {
				if (ldinfo[i].Status != LD_STATUS_INVALID) {
					if (ldinfo[i].Status == LD_STATUS_OFFLINE
					        && ldinfo[i].BGAStatus == LD_BGA_STATE_RUNNING) {
						ldinfo[i].BGAStatus = LD_BGA_STATE_AUTOPAUSED;
					}
					if (ldinfo[i].Status == LD_STATUS_MISSING) {
						ldinfo[i].Status = LD_STATUS_OFFLINE;
					}
				} else {
					break;
				}
			}
		}

		len += sprintf(pBuffer+len,"Index RAID\tStatus  \t\tBGA Status\n");
		for (i = 0 ; i < LDINFO_NUM ; i++) {
			if (ldinfo[i].Size.value == 0) {
				if (i == 0) {
					len += sprintf(pBuffer+len,"NO Logical Disk\n");
				}
				break;
			}
			/* Fixed issue -- An unknown item shown in /proc/scsi/mv64xx/. */
			if (ldinfo[i].Status == LD_STATUS_INVALID)
				continue;

			len += sprintf(pBuffer+len,
				"%-5d %s\t%s",
				ldinfo[i].ID,
				mv_ld_raid_mode(ldinfo[i].RaidMode),
				mv_ld_status(ldinfo[i].Status)
				);

			tmplen = 24 -strlen(mv_ld_status(ldinfo[i].Status));
			while (j < tmplen) {
				len += sprintf(pBuffer+len, "%s", " ");
				j++;
			}
			j = 0;

			len += sprintf(pBuffer+len, "%s", mv_ld_bga_status(ldinfo[i].BGAStatus));

			if (ldinfo[i].BGAStatus != LD_BGA_STATE_NONE) {
				ret = mv_ld_get_status(pSHost,ldinfo[i].ID,&ld_status);
				if (ret == 0) {
					if (ld_status.Status != LD_STATUS_INVALID) {
						if (ld_status.Status == LD_STATUS_MISSING)
							ld_status.Status = LD_STATUS_OFFLINE;
						ld_status.BgaState = ldinfo[i].BGAStatus;
					}
					len += mv_ld_show_status(pBuffer+len,&ld_status);
					ret = -1;
				}
			}

			tmp = NULL;
			tmplen = 0;
			len += sprintf(pBuffer+len,"\n");
		}
	}
	out:
#endif
	if (mv_get_register_mode() != 0)
		len += mv_proc_hba_info(pSHost, pBuffer + len);
#endif
	datalen = len - offset;
	if (datalen < 0) {
		datalen = 0;
		*ppStart = pBuffer + len;
	} else {
		*ppStart = pBuffer + offset;
	}
	return datalen;
}
#endif
#ifdef MV_BLK_IOCTL

/*DEFINE_MUTEX in gcc 4.3 failed to initialize.*/
static spinlock_t ioctl_index_spin;


void * kbuf_array[512] = {NULL,};
unsigned char mvcdb[512][16];
unsigned long kbuf_index[512/8] = {0,};

static inline int mv_is_api_cmd(int cmd)
{
	return (cmd >= API_BLOCK_IOCTL_DEFAULT_FUN) && \
		(cmd < API_BLOCK_IOCTL_DEFAULT_FUN + API_IOCTL_MAX );
}

/* ATA_16(0x85) SAT command and 0xec for Identify */
char ata_ident[] = {0x85, 0x08, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x00};

#ifdef AMCC_CLI_WORKROUND
unsigned char page_buffer[0x4000];
#endif

/**
 *	id_to_string - Convert IDENTIFY DEVICE page into string
 *	@iden: IDENTIFY DEVICE results we will examine
 *	@s: string into which data is output
 *	@ofs: offset into identify device page
 *	@len: length of string to return. must be an even number.
 *
 *	The strings in the IDENTIFY DEVICE page are broken up into
 *	16-bit chunks.  Run through the string, and output each
 *	8-bit chunk linearly, regardless of platform.
 *
 */
static void id_to_string(const u16 *iden, unsigned char *s,
		   unsigned int ofs, unsigned int len)
{
	unsigned int c;
	while (len > 0) {
		c = iden[ofs] >> 8;
		*s = c;
		s++;

		c = iden[ofs] & 0xff;
		*s = c;
		s++;

		ofs++;
		len -= 2;
	}
}
static int io_get_identity(void *kbuf)
{
	char buf[40];
	u16 *dst = kbuf;
	unsigned int i,j;
	
	if (!dst)
		return -1;

#ifdef __MV_BIG_ENDIAN_BITFIELD__
        for (i = 0; i < 256; i++)
                dst[i] = MV_LE16_TO_CPU(dst[i]);
#endif

	/*identify data -> model number: word 27-46*/
	id_to_string(dst, buf, 27, 40);
	memcpy(&dst[27], buf, 40);

	/*identify data -> firmware revision: word 23-26*/
	id_to_string(dst, buf, 23, 8);
	memcpy(&dst[23], buf, 8);
	
	/*identify data -> serial number: word 10-19*/
	id_to_string(dst, buf, 10, 20);
	memcpy(&dst[10], buf, 20);

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 12)

/**
 * scsi_normalize_sense - normalize main elements from either fixed or
 *			descriptor sense data format into a common format.
 *
 * @sense_buffer:	byte array containing sense data returned by device
 * @sb_len:		number of valid bytes in sense_buffer
 * @sshdr:		pointer to instance of structure that common
 *			elements are written to.
 *
 * Notes:
 *	The "main elements" from sense data are: response_code, sense_key,
 *	asc, ascq and additional_length (only for descriptor format).
 *
 *	Typically this function can be called after a device has
 *	responded to a SCSI command with the CHECK_CONDITION status.
 *
 * Return value:
 *	1 if valid sense data information found, else 0;
 **/
int scsi_normalize_sense(const u8 *sense_buffer, int sb_len,
                         struct scsi_sense_hdr *sshdr)
{
	if (!sense_buffer || !sb_len)
		return 0;

	memset(sshdr, 0, sizeof(struct scsi_sense_hdr));

	sshdr->response_code = (sense_buffer[0] & 0x7f);

	if (!scsi_sense_valid(sshdr))
		return 0;

	if (sshdr->response_code >= 0x72) {
		/*
		 * descriptor format
		 */
		if (sb_len > 1)
			sshdr->sense_key = (sense_buffer[1] & 0xf);
		if (sb_len > 2)
			sshdr->asc = sense_buffer[2];
		if (sb_len > 3)
			sshdr->ascq = sense_buffer[3];
		if (sb_len > 7)
			sshdr->additional_length = sense_buffer[7];
	} else {
		/* 
		 * fixed format
		 */
		if (sb_len > 2)
			sshdr->sense_key = (sense_buffer[2] & 0xf);
		if (sb_len > 7) {
			sb_len = (sb_len < (sense_buffer[7] + 8)) ?
					 sb_len : (sense_buffer[7] + 8);
			if (sb_len > 12)
				sshdr->asc = sense_buffer[12];
			if (sb_len > 13)
				sshdr->ascq = sense_buffer[13];
		}
	}

	return 1;
}
#endif	
/**
 *	mv_ata_task_ioctl - Handler for HDIO_DRIVE_TASK ioctl
 *	@scsidev: Device to which we are issuing command
 *	@arg: User provided data for issuing command
 *
 *	LOCKING:
 *	Defined by the SCSI layer.  We don't really care.
 *
 *	RETURNS:
 *	Zero on success, negative errno on error.
 */
int mv_ata_task_ioctl(struct scsi_device *scsidev, void __user *arg)
{
	int rc = 0;
	u8 scsi_cmd[MAX_COMMAND_SIZE];
	u8 args[7];
	struct scsi_sense_hdr sshdr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
	if (arg == NULL)
		return -EINVAL;

	if (copy_from_user(args, arg, sizeof(args)))
		return -EFAULT;

	memset(scsi_cmd, 0, sizeof(scsi_cmd));
	scsi_cmd[0]  = ATA_16;
	scsi_cmd[1]  = (3 << 1); /* Non-data */
	/* scsi_cmd[2] is already 0 -- no off.line, cc, or data xfer */
	scsi_cmd[4]  = args[1];
	scsi_cmd[6]  = args[2];
	scsi_cmd[8]  = args[3];
	scsi_cmd[10] = args[4];
	scsi_cmd[12] = args[5];
	scsi_cmd[14] = args[0];
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(scsidev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		scsi_release_request(sreq);
		return rc;
	}
	sreq->sr_data_direction = DMA_NONE;
	scsi_wait_req(sreq, scsi_cmd, NULL, 0, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	rc = sreq->sr_result;
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
	if (scsi_execute_req(scsidev, scsi_cmd, DMA_NONE, NULL, 0, &sshdr,
				     (10*HZ), 5,0))
			rc = -EIO;


#else
	if (scsi_execute_req(scsidev, scsi_cmd, DMA_NONE, NULL, 0, &sshdr,
			     (10*HZ), 5))
		rc = -EIO;
#endif
	/* Need code to retrieve data from check condition? */
	return rc;
}
/****************************************************************
*  Name:   mv_ial_ht_ata_cmd
*
*  Description:    handles mv_sata ata IOCTL special drive command (HDIO_DRIVE_CMD)
*
*  Parameters:     scsidev - Device to which we are issuing command
*                  arg     - User provided data for issuing command
*
*  Returns:        0 on success, otherwise of failure.
*
****************************************************************/
static int mv_ial_ht_ata_cmd(struct scsi_device *scsidev, void __user *arg)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
      	u8 args[4] , *argbuf = NULL;
	u8 sensebuf[SCSI_SENSE_BUFFERSIZE];
        struct scsi_sense_hdr sshdr;
      	int argsize = 0;
      	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	if (arg == NULL)
          	return -EINVAL;
  
      	if (copy_from_user(args, arg, sizeof(args)))
          	return -EFAULT;
      	
	memset(sensebuf, 0, sizeof(sensebuf));
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));
      	if (args[3]) {
          	argsize = SECTOR_SIZE * args[3];
          	argbuf = kmalloc(argsize, GFP_KERNEL);
          	if (argbuf == NULL) {
              		rc = -ENOMEM;
              		goto error;
     	}
  
     	scsi_cmd[1]  = (4 << 1); /* PIO Data-in */
     	scsi_cmd[2]  = 0x0e;     /* no off.line or cc, read from dev,
                                             block count in sector count field */
      	data_dir = DMA_FROM_DEVICE;
      	} else {
       		scsi_cmd[1]  = (3 << 1); /* Non-data */
       		scsi_cmd[2]  = 0x20;     /* cc but no off.line or data xfer */
      		data_dir = DMA_NONE;
	}
  
	scsi_cmd[0] = ATA_16;
  
   	scsi_cmd[4] = args[2];
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    	if (args[0] == WIN_SMART) { /* hack -- ide driver does this too... */
#else
	if (args[0] == ATA_CMD_SMART) { /* hack -- ide driver does this too... */	
#endif	
        	scsi_cmd[6]  = args[3];
       		scsi_cmd[8]  = args[1];
     		scsi_cmd[10] = 0x4f;
    		scsi_cmd[12] = 0xc2;
      	} else {
          	scsi_cmd[6]  = args[1];
      	}
      	scsi_cmd[14] = args[0];
      	
      	/* Good values for timeout and retries?  Values below
         	from scsi_ioctl_send_command() for default case... */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(scsidev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, &sshdr, (10*HZ), 5, 0, 0, NULL);
#else
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, &sshdr, (10*HZ), 5, 0, 0, NULL);
#endif


      	if (scsi_sense_valid(&sshdr)) {/* sense data available */
         	u8 *desc = sensebuf + 8;
  
          	/* If we set cc then ATA pass-through will cause a
          	* check condition even if no error. Filter that. */
          	if (scsi_status_is_check_condition(cmd_result)) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
  
          	/* Send userspace a few ATA registers (same as drivers/ide) */
          	if (sensebuf[0] == 0x72 &&     /* format is "descriptor" */
              		desc[0] == 0x09 ) {        /* code is "ATA Descriptor" */
              		args[0] = desc[13];    /* status */
              		args[1] = desc[3];     /* error */
              		args[2] = desc[5];     /* sector count (0:7) */
              		if (copy_to_user(arg, args, sizeof(args)))
                  		rc = -EFAULT;
          	}
      	}
  
      	if (cmd_result) {
          	rc = -EIO;
          	goto free_req;
      	}
  
      	if ((argbuf) && copy_to_user(arg + sizeof(args), argbuf, argsize))
          	rc = -EFAULT;
      	
free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif		
error:
      	if (argbuf) kfree(argbuf);
      	return rc;
}

static int check_dma (__u8 ata_op)
{
	switch (ata_op) {
		case ATA_CMD_READ_DMA_EXT:
		case ATA_CMD_READ_FPDMA_QUEUED:
		case ATA_CMD_WRITE_DMA_EXT:
		case ATA_CMD_WRITE_FPDMA_QUEUED:
		case ATA_CMD_READ_DMA:
		case ATA_CMD_WRITE_DMA:
			return SG_DMA;
		default:
			return SG_PIO;
	}
}
unsigned char excute_taskfile(struct scsi_device *dev,ide_task_request_t *req_task,u8 
 rw,char *argbuf,unsigned int buff_size)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
	u8 sensebuf[SCSI_SENSE_BUFFERSIZE];
        struct scsi_sense_hdr sshdr;
      	int argsize=0;
	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	argsize=buff_size;
	memset(sensebuf, 0, sizeof(sensebuf));
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));

  	data_dir = DMA_FROM_DEVICE;   // need to fixed
	scsi_cmd[0] = ATA_16;
	scsi_cmd[13] = 0x40;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
      	scsi_cmd[14] = ((task_struct_t *)(&req_task->io_ports))->command;
#else
	scsi_cmd[14] = ((char *)(&req_task->io_ports))[7];
#endif
	if(check_dma(scsi_cmd[14])){
		scsi_cmd[1] = argbuf ? SG_ATA_PROTO_DMA : SG_ATA_PROTO_NON_DATA;
	} else {
		scsi_cmd[1] = argbuf ? (rw ? SG_ATA_PROTO_PIO_OUT : SG_ATA_PROTO_PIO_IN) : SG_ATA_PROTO_NON_DATA;
	}
	scsi_cmd[ 2] = SG_CDB2_CHECK_COND;
	if (argbuf) {
		scsi_cmd[2] |= SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS;
		scsi_cmd[2] |= rw ? SG_CDB2_TDIR_TO_DEV : SG_CDB2_TDIR_FROM_DEV;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(dev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, &sshdr, (10*HZ), 5, 0, 0, NULL);
#else
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0);
#endif
  
      	if (scsi_sense_valid(&sshdr)) {/* sense data available */
         	u8 *desc = sensebuf + 8;
  		
          	/* If we set cc then ATA pass-through will cause a
          	* check condition even if no error. Filter that. */
          	if (cmd_result & SAM_STAT_CHECK_CONDITION) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
      	}
  
      	if (cmd_result) {
          	rc = EIO;
		MV_PRINT("EIO=%d\n",-EIO);
          	goto free_req;
      	}
#if 0
      	if ( copy_to_user(argbuf,sensebuf, argsize))
       		rc = -EFAULT;
#endif
free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif
      	return rc;
}
u8 mv_do_taskfile_ioctl(struct scsi_device *dev,void __user *arg){
	ide_task_request_t *req_task=NULL;
	char __user *buf = (char __user *)arg;
	u8 *outbuf	= NULL;
	u8 *inbuf	= NULL;
	int err		= 0;
	int tasksize	= sizeof(ide_task_request_t);
	int taskin	= 0;
	int taskout	= 0;
	int rw = SG_READ;
	
	req_task = kzalloc(tasksize, GFP_KERNEL);
	if (req_task == NULL) return -ENOMEM;
	if (copy_from_user(req_task, buf, tasksize)) {
		kfree(req_task);
		return -EFAULT;
	}

	switch (req_task->req_cmd) {
		case TASKFILE_CMD_REQ_OUT:
		case TASKFILE_CMD_REQ_RAW_OUT:
			rw         = SG_WRITE;
			break;
		case TASKFILE_CMD_REQ_IN:
			break;
	}
	taskout = (int) req_task->out_size;
	taskin  = (int) req_task->in_size;


	if (taskout) {
		int outtotal = tasksize;
		outbuf = kzalloc(taskout, GFP_KERNEL);
		if (outbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(outbuf, buf + outtotal, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}

	if (taskin) {
		int intotal = tasksize + taskout;
		inbuf = kzalloc(taskin, GFP_KERNEL);
		if (inbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(inbuf, buf + intotal, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
	
	switch(req_task->data_phase) {
		case TASKFILE_DPHASE_PIO_OUT:
			err = excute_taskfile(dev,req_task,rw,outbuf,taskout);
			break;
		default:
			err = -EFAULT;
			goto abort;
	}
	if (copy_to_user(buf, req_task, tasksize)) {
		err = -EFAULT;
		goto abort;
	}
	if (taskout) {
		int outtotal = tasksize;
		if (copy_to_user(buf + outtotal, outbuf, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}
	if (taskin) {
		int intotal = tasksize + taskout;
		if (copy_to_user(buf + intotal, inbuf, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
abort:
	kfree(req_task);
	kfree(outbuf);
	kfree(inbuf);

	return err;
}

int mv_new_ioctl(struct scsi_device *dev, unsigned int cmd, void __user *arg)
{
	int error,writing = 0, length;
	int console,nr_hba;
	int nbit;
	static int mutex_flag = 0;
	void * kbuf = NULL;
	int val = 0;
	struct scsi_idlun idlun;
	struct request *rq;
	struct request_queue * q = dev->request_queue;
	PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER psptdwb = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
	struct scsi_request *sreq;
#endif
   
	/*Workaround for GCC 4.3 */
	if(mutex_flag == 0){		
		spin_lock_init(&ioctl_index_spin);
		mutex_flag = 1;
	}
	switch (cmd){

	case HDIO_GET_IDENTITY:
		psptdwb = hba_mem_alloc(sizeof(*psptdwb),MV_FALSE);
		if (!psptdwb)
			return -ENOMEM;
		/* extract with a function */
		psptdwb->sptd.DataTransferLength = 512;
		psptdwb->sptd.DataBuffer = arg;
		psptdwb->sptd.CdbLength = 16;
		memcpy((void*)psptdwb->sptd.Cdb, ata_ident, 16);
		goto fetch_data;

	case HDIO_GET_32BIT:
		if (copy_to_user(arg, &val, 1))
			return -EFAULT;
		return 0;
	case HDIO_SET_32BIT:
		val = (unsigned long)arg;
		if (val != 0)
			return -EINVAL;
		return 0;
	case HDIO_DRIVE_CMD:
             	if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
                 	return -EACCES;
 
             	return mv_ial_ht_ata_cmd(dev, arg);

	case HDIO_DRIVE_TASK:
		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
			return -EACCES;
		return mv_ata_task_ioctl(dev, arg);
	case HDIO_DRIVE_TASKFILE:
		return mv_do_taskfile_ioctl(dev,arg);

	default:
		break;
	}

	switch(cmd - API_BLOCK_IOCTL_DEFAULT_FUN){
	case API_IOCTL_GET_VIRTURL_ID:
		console = VIRTUAL_DEVICE_ID;
		if (copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			(void *)&console,sizeof(int)))
			return -EIO;
		return 0;
	case API_IOCTL_GET_HBA_COUNT:
		nr_hba = __mv_get_adapter_count();
		if (copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			(void *)&nr_hba,sizeof(unsigned int)))
			return -EIO;
		return 0;
	case API_IOCTL_LOOKUP_DEV:
		if(copy_from_user(&idlun, ((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			sizeof(struct scsi_idlun)))
			return -EIO;
		if(dev->host->host_no != ((idlun.dev_id) >> 24))
			return EFAULT;
		return 0;
	case API_IOCTL_CHECK_VIRT_DEV:
		if( dev->id != VIRTUAL_DEVICE_ID)
			return -EFAULT;
		return 0;
#ifdef API_GET_ENC_ID
	case API_IOCTL_GET_ENC_ID:
		return mv_get_enc_scsi_id((MV_PVOID)dev,(MV_PVOID)arg);
#endif
	case API_IOCTL_DEFAULT_FUN:
		break;
	default:
		/*  set default with no such terminal, not -ENOPERM to cater to Linux */
		return -ENOTTY;
	}

	psptdwb = hba_mem_alloc(sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
	if (!psptdwb)
		return -ENOMEM;
	error = copy_from_user(psptdwb, (void *)arg, sizeof(*psptdwb));
	if (error) {
		hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
		return -EIO;
	}
fetch_data:
	length = psptdwb->sptd.DataTransferLength;

	if (length){
		if(length > IOCTL_BUF_LEN || (kbuf = hba_mem_alloc(length,MV_TRUE)) == NULL ){
			hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
			return -ENOMEM;
		}
		if(copy_from_user(kbuf, psptdwb->sptd.DataBuffer, length)){
			hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
			hba_mem_free(kbuf,length,MV_TRUE);
			return -EIO;
		}

#ifdef AMCC_CLI_WORKROUND
		if (psptdwb->sptd.Cdb[0] == APICDB0_FLASH && psptdwb->sptd.Cdb[1] == APICDB1_FLASH_BIN && \
			psptdwb->sptd.Cdb[2] == FLASH_UPLOAD && psptdwb->sptd.Cdb[3] == FLASH_TYPE_CONFIG){
			memcpy(page_buffer,&((unsigned char *)kbuf)[8], 0x4000);
		}
#endif
		if ((SCSI_IS_WRITE(psptdwb->sptd.Cdb[0])) || (psptdwb->sptd.DataIn == 0))
			writing = 1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    rq = blk_get_request(q, writing ? REQ_OP_DRV_OUT : REQ_OP_DRV_IN, 0);
    if (IS_ERR(rq))
    {
		hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
		return -ENOMEM;

    }
#else
    rq = blk_get_request(q, writing ? WRITE : READ, GFP_KERNEL);
	if (!rq) {
		hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
		return -ENOMEM;
	}
#endif
#else
	sreq = scsi_allocate_request(dev, GFP_KERNEL);
	if (!sreq) {
		MV_PRINT("SCSI internal ioctl failed, no memory\n");
		return -ENOMEM;
	}
	rq = sreq->sr_request;
#endif

	scsi_req(rq)->cmd_len = psptdwb->sptd.CdbLength;
	psptdwb->sptd.ScsiStatus = REQ_STATUS_PENDING;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	//rq->cmd_type = REQ_TYPE_BLOCK_PC;
	//rq->tag = psptdwb->sptd.ScsiStatus;
#else
  #if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
	rq->flags |= REQ_BLOCK_PC;
	rq->rq_status = psptdwb->sptd.ScsiStatus;
  #else
  	//rq->tag = psptdwb->sptd.ScsiStatus;
  #endif
#endif
	rq->timeout = msecs_to_jiffies(psptdwb->sptd.TimeOutValue);
	if(!rq->timeout)
		rq->timeout = 60 * HZ;

        memcpy(scsi_req(rq)->cmd, psptdwb->sptd.Cdb, psptdwb->sptd.CdbLength);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
	if(length && blk_rq_map_kern(q, rq,kbuf, length, __GFP_WAIT)){
		error = -EIO;
		goto out;
	}

	scsi_req(rq)->retries = 1;

#else
	rq->data = kbuf;
	rq->data_len = length;
#endif

	spin_lock(&ioctl_index_spin);
	nbit = find_first_zero_bit((unsigned long*)kbuf_index,512);
	if(nbit >= 512){		
		spin_unlock(&ioctl_index_spin);
		error = -ENOSPC;
		goto out;
	}
	__set_bit(nbit, kbuf_index);
	spin_unlock(&ioctl_index_spin);

	/*make sure till queuecommand we will not reuse the field.*/
	kbuf_array[nbit] = kbuf;
	memcpy((void*)mvcdb[nbit],scsi_req(rq)->cmd,16);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,12,0)
	blk_execute_rq(NULL, rq, 0);
#else
	blk_execute_rq(q, NULL, rq, 0);
#endif
#else
	error = blk_execute_rq(q, NULL, rq);
#endif

#else
	sreq->sr_data_direction = writing ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	scsi_wait_req(sreq, rq->cmd, kbuf, length,  rq->timeout, 1);
	memcpy(rq->sense,sreq->sr_sense_buffer,rq->sense_len);
	if(sreq->sr_result)
		error = -EIO;
	else
		error = sreq->sr_result;
#endif	//#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)

	spin_lock(&ioctl_index_spin);
	__clear_bit(nbit,kbuf_index);
	spin_unlock(&ioctl_index_spin);
	kbuf_array[nbit] = NULL;
	memset((void*)mvcdb[nbit],0x00,16);

	/* Fix hdparm -i get inverse param info.*/
	if (length && (cmd == HDIO_GET_IDENTITY))
		io_get_identity(kbuf);
	
	if (error) {
		/* REQ_STATUS_HAS_SENSE is Used by SCSI command ;REQ_STATUS_ERROR_WITH_SENSE
		is used by RAID API and Sense_Info_Buffer[0] is used to pass error code to RAID API . */
                if ( scsi_req(rq)->sense_len && scsi_req(rq)->sense && ((MV_PU8)scsi_req(rq)->sense)[0]) {
                        MV_DPRINT(("%s has sense,rq->sense[0]=0x%x\n", __func__,((MV_PU8)scsi_req(rq)->sense)[0]));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19) || LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
			if (rq->tag == REQ_STATUS_ERROR_WITH_SENSE)
				error = rq->tag;
#else
			if(rq->rq_status == REQ_STATUS_ERROR_WITH_SENSE)
				error = rq->rq_status;
#endif
		}
	} else if (length && copy_to_user(psptdwb->sptd.DataBuffer,kbuf,length)) {
                        error = -EIO;
                        goto out;
	}
	if (mv_is_api_cmd(cmd) && copy_to_user((void*)arg,psptdwb,
		sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER))){
		error = -EIO;
		goto out;
	}

out:
	hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
	hba_mem_free(kbuf, psptdwb->sptd.DataTransferLength,MV_TRUE);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
	blk_put_request(rq);
#else
	scsi_release_request(sreq);
#endif
	return error;
}
#endif
/*
 *Character Device Interface.
 */
#ifdef RAID_DRIVER
static int mv_open(struct inode *inode, struct file *file)
{
	unsigned int minor_number;
	int retval = -ENODEV;
	unsigned long flags = 0;
#ifndef MV_VMK_ESX35
	spin_lock_irqsave(&inode->i_lock, flags);
	minor_number = MINOR(inode->i_rdev);
	if (minor_number >= __mv_get_adapter_count()) {
		MV_PRINT("MV : No such device.\n");
		goto out;
	}
	retval = 0;
out:
	spin_unlock_irqrestore(&inode->i_lock, flags);
	return retval;
#else
	return 0;
#endif
}

#ifndef MV_BLK_IOCTL
#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
static int _mv_major;
int mv_register_chdev(struct hba_extension *hba)
{
#ifndef MV_VMK_ESX35
	apidev_class = class_create(THIS_MODULE, MV_DEVFS_NAME);
	if (IS_ERR(apidev_class)) {
		printk("mv ioctl: %s: Unable to sysfs class\n", __func__);
		apidev_class = NULL;
		return 1;
	}
	printk("mv ioctl: %s: apidev_class=%p.\n", __func__, apidev_class);
#endif

	_mv_major = register_chrdev(0,
        	        MV_DEVFS_NAME,
                	&mv_fops);
	if (_mv_major < 0){
		printk("mv ioctl: %s: Unable to register CHAR device (%d)\n",
		    __func__, _mv_major);
#ifndef MV_VMK_ESX35
		class_destroy(apidev_class);
		apidev_class = NULL;
#endif
		return _mv_major;
	}

else
	printk("mv ioctl: %s: Successfully to register CHAR device (%d)\n",
		__func__, _mv_major);

#ifndef MV_VMK_ESX35
        class_device_create(apidev_class, NULL, MKDEV(_mv_major, 0), NULL,
            		MV_DEVFS_NAME);
#endif
}

void mv_unregister_chdev(struct hba_extension *hba)
{
#ifndef MV_VMK_ESX35
	if (!apidev_class)
		return;
	class_device_destroy(apidev_class, MKDEV(_mv_major, 0));
#endif
	unregister_chrdev(_mv_major, MV_DEVFS_NAME);

#ifndef MV_VMK_ESX35
	class_destroy(apidev_class);
	apidev_class = NULL;
#endif
}

#else
static dev_t _mv_major = 0;

int mv_register_chdev(struct hba_extension *hba)
{
	int ret = 0;
	dev_t num;

	/*
	 * look for already-allocated major number first, if not found or
	 * fail to register, try allocate one .
	 *
	 */
	if (_mv_major) {
		ret = register_chrdev_region(MKDEV(_mv_major,
						   hba->desc->hba_desc->id),
					     1,
					     MV_DEVFS_NAME);
		num = MKDEV(_mv_major, hba->desc->hba_desc->id);
	}
	if (ret) {
		MV_DPRINT(("registered chrdev (%d, %d) failed.\n",
	       	_mv_major, hba->desc->hba_desc->id));
		return	ret;
	}
	if (ret || !_mv_major) {
		ret = alloc_chrdev_region(&num,
					  hba->desc->hba_desc->id,
					  1,
					  MV_DEVFS_NAME);
		if (ret) {
			MV_DPRINT(( "allocate chrdev (%d, %d) failed.\n",
		       	MAJOR(num), hba->desc->hba_desc->id));
			return ret;
		} else
			_mv_major = MAJOR(num);
	}

	memset(&hba->desc->hba_desc->cdev, 0, sizeof(struct cdev));
	cdev_init(&hba->desc->hba_desc->cdev, &mv_fops);
	hba->desc->hba_desc->cdev.owner = THIS_MODULE;
	hba->desc->hba_desc->dev_no = num;
	MV_DPRINT((  "registered chrdev (%d, %d).\n",
	       MAJOR(num), MINOR(num)));
	ret = cdev_add(&hba->desc->hba_desc->cdev, num, 1);

	return ret;
}

void mv_unregister_chdev(struct hba_extension *hba)
{
	cdev_del(&hba->desc->hba_desc->cdev);
	unregister_chrdev_region(hba->desc->hba_desc->dev_no, 1);
}

#endif
#endif

MV_U8 mv_check_ioctl_req(PMV_Request    req)
{
	switch(req->Cdb[0]){
	case APICDB0_ADAPTER:
	case APICDB0_LD:
	case APICDB0_PD:
	case APICDB0_BLOCK:
	case APICDB0_EVENT:
	case APICDB0_DBG:
	case API_SCSI_CMD_RCV_DIAG_RSLT:
	case API_SCSI_CMD_SND_DIAG:
#ifdef SUPPORT_PASS_THROUGH_DIRECT
	case APICDB0_PASS_THRU_CMD_SCSI:
	case APICDB0_PASS_THRU_CMD_ATA:
#endif
		return	MV_TRUE;

	default:
		return	MV_FALSE;
	}
	return	MV_FALSE;
}

#ifdef API_GET_ENC_ID
 int mv_get_enc_scsi_id(MV_PVOID hba_dev,MV_PVOID arg)
{
	struct Scsi_Host  *host;
	struct scsi_device *sdev = NULL;
	penc_hctl_info enc_hctl = NULL;
	MV_U16 target_id = 0,i=0;
	MV_U8 num = 0,max_enc_num = 0;
	unsigned long data_transfer_length = 0;
#ifdef MV_BLK_IOCTL
	struct scsi_device *dev = (struct scsi_device * )hba_dev;
	host = dev->host;
#else
	struct hba_extension *hba =(struct hba_extension *)hba_dev;
	host = hba->desc->hba_desc->hba_host;
#endif
	data_transfer_length = ((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataTransferLength;
	max_enc_num = data_transfer_length/sizeof(enc_hctl_info);
	enc_hctl = (penc_hctl_info)hba_mem_alloc(data_transfer_length,MV_FALSE);
	if(!enc_hctl){
		hba_mem_free(enc_hctl, data_transfer_length,MV_FALSE);
		return -ENOMEM;
	}
	memset(enc_hctl,0,data_transfer_length);

	for(target_id=0;target_id<0xFF;target_id++) {
		sdev = scsi_device_lookup(host,0 ,target_id,0);
		if(sdev) {
			if((sdev->type == enclosure)&&(num < max_enc_num)){
				enc_hctl[num].host_no=sdev->host->host_no;
				enc_hctl[num].channel = sdev->channel;
				enc_hctl[num].target_id = sdev->id;
				enc_hctl[num].lun = sdev->lun;
				//enc_hctl[num].dev_id = MIN_ENC_ID+num;
				enc_hctl[num].enc_num = num+1;
				num++;
			}
			scsi_device_put(sdev);
		}
	}
#ifdef MV_DEBUG
	for(i=0;i<num;i++)
		MV_DPRINT(("max_enc_num: %d enc_num:%d dev_id:%02d Host:%d Channel:%02d Id:%02d Lun:%02d\n", \
			max_enc_num,enc_hctl[i].enc_num ,enc_hctl[i].dev_id,enc_hctl[i].host_no,\
			enc_hctl[i].channel ,enc_hctl[i].target_id,enc_hctl[i].lun ));
#endif
	if(num){
		if (copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			enc_hctl,data_transfer_length)){
			hba_mem_free(enc_hctl, data_transfer_length,MV_FALSE);
              	return -EIO;
      	 	}
	}
	hba_mem_free(enc_hctl, data_transfer_length,MV_FALSE);
	return 0;
}
#endif


#if defined (HAVE_UNLOCKED_IOCTL) && !defined (__VMKLNX__)
static long mv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int mv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER karg;
	struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER  __user *uarg = (void __user *) arg;
	struct hba_extension	*hba;
	struct mv_adp_desc *hba_desc;
	PMV_Request    req = NULL;
	int error = 0;
	int ret   = 0;
	int sptdwb_size = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	int console_id  = VIRTUAL_DEVICE_ID;
	MV_U8 Scsi_Status = REQ_STATUS_PENDING;
	unsigned long flags;
	PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER psptdwb = NULL;
	int mv_device_count;
	struct scsi_idlun idlun;

#ifdef SUPPORT_SES
	BUS_ADDRESS busaddr = 0;
	dma_addr_t      dma_addr;
	MV_PVOID        vir_addr = NULL;
#endif
	void *ioctl_buf = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	DECLARE_MUTEX(sem_lock);
#else
    DEFINE_SEMAPHORE(sem_lock);
#endif


#if defined(__VMKLNX__) || defined(MV_VMK_ESX35) 
	hba_desc = gl_hba_desc;
#else
#ifdef HAVE_UNLOCKED_IOCTL
	hba_desc = container_of(file->f_dentry->d_inode->i_cdev,struct mv_adp_desc, cdev);
#else
	hba_desc = container_of(inode->i_cdev, struct mv_adp_desc, cdev);
#endif
#endif

	if (!hba_desc) {
		MV_DPRINT(( "Marvell : Driver is not installed ,Invalid operation.\n"));
		return -EPERM;

	}
	hba = (struct hba_extension	*)mv_get_hba_extension(hba_desc);

	/*if the driver is shutdown ,any process shouldn't call mv_ioctl*/
	if(hba != NULL) {
		if( DRIVER_STATUS_SHUTDOWN == hba->State ) {
			MV_DPRINT((  "Marvell : Driver had been rmmoded ,Invalid operation.\n"));
			return -EPERM;
		}
	} else {
		MV_DPRINT(( "Marvell : Driver is not installed ,Invalid operation.\n"));
		return -EPERM;
	}

	mv_device_count =  __mv_get_adapter_count();

	if (cmd >= API_IOCTL_MAX) {
			MV_DPRINT(( "Marvell : Invalid ioctl command.\n"));
			return -EBADF;
	}

	if (copy_from_user(&karg, uarg, sizeof(struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER))) {
		MV_PRINT(( "Marvell : Clone User Data Error.\n"));
		return -EFAULT;
	}	

	if (cmd == API_IOCTL_GET_VIRTURL_ID) {
		if (copy_to_user((void __user *)karg.sptd.DataBuffer,
			(void *)&console_id,sizeof(int)) != 0 ) {
			MV_DPRINT(( "Marvell : Get VIRTUAL_DEVICE_ID Error.\n"));
			return -EIO;
		}
		return 0;
	}

	if (cmd == API_IOCTL_GET_HBA_COUNT) {
		if (copy_to_user((void __user *)karg.sptd.DataBuffer,
			(void *)&mv_device_count,sizeof(unsigned int)) != 0) {
			MV_DPRINT(( "Marvell : Get Device Number Error.\n"));
			return -EIO;
		}
		return 0;
	}

	if (cmd == API_IOCTL_LOOKUP_DEV) {
		if( copy_from_user(&idlun, (void __user *)karg.sptd.DataBuffer,
			sizeof(struct scsi_idlun)) !=0) {
			MV_DPRINT(( "Marvell : Get Device idlun Error.\n"));
			return -EIO;
		}

		/*To check host no, if fail, return EFAULT (Bad address) */
		if (hba_desc->hba_host->host_no != ((idlun.dev_id) >> 24)) {
			MV_DPRINT(( "Marvell : lookup device host number error .\n"));
			return EFAULT;
		}
		/*
		if (hba->host->host_no == ((idlun.dev_id) >> 24)) {
			sdev = scsi_device_lookup(hba->host, (idlun.dev_id >> 16)&0xff,(idlun.dev_id)&0xff,(idlun.dev_id >> 8)&0xff);
			if (!sdev) {
				MV_DBG(DMSG_IOCTL, "Marvell : lookup device .\n");
				return -EIO;
			}
			scsi_device_put(sdev);
		}
		*/
		return 0;
	}

#ifdef API_GET_ENC_ID
		if (cmd==API_IOCTL_GET_ENC_ID){
			return mv_get_enc_scsi_id((MV_PVOID)hba,(MV_PVOID)&karg;
		}
#endif
	if (down_trylock(&sem_lock)) {
		up(&sem_lock);
		return -EINTR;
	}
	psptdwb = hba_mem_alloc(sptdwb_size,MV_FALSE);
	if ( NULL == psptdwb ) {
		MV_DPRINT(( "__MV__ alloc psptwb buffer fail .\n"));
		ret = -ENOMEM;
		goto free_lock;
	}

	error = copy_from_user(psptdwb, uarg, sptdwb_size);
	if (error) {
		ret = -EIO;
		goto clean_pspbuf;
	}

	if (psptdwb->sptd.DataTransferLength) {
#ifdef SUPPORT_SES
		/*receive dialog info,cdb[1]=0x01;send dialog info ,cdb[1]=0x10; */
		if( __is_scsi_cmd_rcv_snd_diag(psptdwb->sptd.Cdb[0])) {
			if((psptdwb->sptd.Cdb[1] ==0x01) ||(psptdwb->sptd.Cdb[1]==0x10)) {
				psptdwb->sptd.DataTransferLength = ROUNDING(psptdwb->sptd.DataTransferLength, 8);
				psptdwb->sptd.DataBuffer = (MV_PVOID) pci_alloc_consistent(hba_desc->dev,
					psptdwb->sptd.DataTransferLength,&dma_addr);
				if (NULL == psptdwb->sptd.DataBuffer ) {
					MV_DPRINT(( "Ioctl unable to alloc 0x%lx consistent mem.\n",
						psptdwb->sptd.DataTransferLength));
					pci_free_consistent(hba_desc->dev,  psptdwb->sptd.DataTransferLength,
						psptdwb->sptd.DataBuffer,dma_addr);
					ret = -ENOMEM;
					goto clean_pspbuf;
				}
				memset(psptdwb->sptd.DataBuffer, 0, psptdwb->sptd.DataTransferLength);
				vir_addr = psptdwb->sptd.DataBuffer;
			} else {
				ioctl_buf = hba_mem_alloc(psptdwb->sptd.DataTransferLength,MV_FALSE);
				if ( NULL == ioctl_buf ) {
					MV_DPRINT((  "__MV__ alloc ioctl_buf fail .\n"));
					ret = -ENOMEM;
					goto clean_pspbuf;
				}
				psptdwb->sptd.DataBuffer = ioctl_buf;
				memset(ioctl_buf, 0, psptdwb->sptd.DataTransferLength);
			}

		}else
#endif
		{
			ioctl_buf = hba_mem_alloc(psptdwb->sptd.DataTransferLength,MV_FALSE);
			if ( NULL == ioctl_buf ) {
				MV_DPRINT(( "__MV__ alloc ioctl_buf fail .\n"));
				ret = -ENOMEM;
				goto clean_pspbuf;
			}
			psptdwb->sptd.DataBuffer = ioctl_buf;
			memset(ioctl_buf, 0, psptdwb->sptd.DataTransferLength);
		}
		error = copy_from_user( psptdwb->sptd.DataBuffer,
					(void __user *)karg.sptd.DataBuffer,psptdwb->sptd.DataTransferLength);
		if (error) {
			ret = -EIO;
			goto clean_dmabuf;
		}
	} else {
		psptdwb->sptd.DataBuffer = NULL;
	}
	memset(psptdwb->Sense_Buffer, 0, SENSE_INFO_BUFFER_SIZE);

	#ifdef USE_REQ_POOL
		req = hba_req_cache_alloc(hba);
	#else
		req = res_get_req_from_pool(hba->req_pool);
	#endif

	if (NULL == req) {
		ret = -ENOMEM;
		goto clean_dmabuf;
	}

	req->Cmd_Initiator = hba;
	req->Org_Req = req;
	req->Device_Id = psptdwb->sptd.TargetId;
	req->Cmd_Flag = 0;
	req->Req_Type = REQ_TYPE_INTERNAL;
	psptdwb->sptd.ScsiStatus = REQ_STATUS_PENDING;
	req->Scsi_Status = psptdwb->sptd.ScsiStatus;

	if (psptdwb->sptd.DataTransferLength == 0) {
		//req->Cmd_Flag |= CMD_FLAG_NON_DATA;
	} else {
		if (SCSI_IS_READ(psptdwb->sptd.Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DATA_IN;
		if (SCSI_IS_READ(psptdwb->sptd.Cdb[0]) || SCSI_IS_WRITE(psptdwb->sptd.Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DMA;
	}

	req->Data_Transfer_Length = psptdwb->sptd.DataTransferLength;
	req->Data_Buffer = psptdwb->sptd.DataBuffer;
	req->Sense_Info_Buffer = psptdwb->Sense_Buffer;
	memcpy(req->Cdb, psptdwb->sptd.Cdb, MAX_CDB_SIZE);
	memset(req->Context, 0, sizeof(MV_PVOID)*MAX_POSSIBLE_MODULE_NUMBER);
	req->LBA.value = 0;
	req->Sector_Count = 0;
	req->Completion = ioctlcallback;
	SGTable_Init(&req->SG_Table, 0);

#ifdef SUPPORT_SES
	/*receive dialog info,cdb[1]=0x01; send dialog info , cdb[1]=0x10; */
	if( __is_scsi_cmd_rcv_snd_diag(req->Cdb[0])) {
		if((req->Cdb[1] ==0x01) ||(req->Cdb[1]==0x10)) {
			if(req->Cdb[0] ==API_SCSI_CMD_RCV_DIAG_RSLT)
				req->Cmd_Flag = CMD_FLAG_DATA_IN;
			if(req->Cdb[0] ==API_SCSI_CMD_SND_DIAG	)
				req->Cmd_Flag &= ~CMD_FLAG_DATA_IN;
			busaddr = (BUS_ADDRESS)dma_addr;
			SGTable_Append(
				&req->SG_Table,
				LO_BUSADDR(busaddr),
				HI_BUSADDR(busaddr),
				req->Data_Transfer_Length);
		}
	}
#endif
	if( atomic_read(&hba->Ioctl_Io_Count) > MV_MAX_IOCTL_REQUEST){
			MV_DPRINT(( "__MV__  ioctl Io Count = 0x%x req->Cdb[0-2](%x-%x-%x).\n",
				atomic_read(&hba->Ioctl_Io_Count),req->Cdb[0],req->Cdb[1],req->Cdb[2]));
			ret = -EBUSY;
#ifdef USE_REQ_POOL
                hba_req_cache_free(hba,req);
#else
                res_free_req_to_pool(hba->req_pool, req);
#endif
			goto clean_reqpool;
	}

#ifdef SUPPORT_TASKLET
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_disable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif
#endif	
	spin_lock_irqsave(&hba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 1);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	atomic_inc(&hba->Ioctl_Io_Count);
	hba->desc->hba_desc->ioctl_cmpl_Scsi_Status = req->Scsi_Status;
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);

#ifdef CORE_NO_RECURSIVE_CALL
	{
		MV_PVOID core = (MV_PVOID)HBA_GetModuleExtension(hba, MODULE_CORE);
		core_send_mv_request(core, req);
	}
#endif

	spin_unlock_irqrestore(&hba->desc->hba_desc->global_lock, flags);

#ifdef SUPPORT_TASKLET
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet);
#if defined(HARDWARE_XOR)
	tasklet_enable(&hba->desc->hba_desc->mv_tasklet_xor);
#endif
#endif

#ifdef CORE_NO_RECURSIVE_CALL
	{
		MV_PVOID core = (MV_PVOID)HBA_GetModuleExtension(hba, MODULE_CORE);
		core_send_mv_request(core, req);
	}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (!__hba_wait_for_atomic_timeout(&hba->desc->hba_desc->hba_ioctl_sync,HBA_REQ_TIMER_IOCTL*HZ))
#else
	if (!wait_for_completion_timeout(&hba->desc->hba_desc->ioctl_cmpl,HBA_REQ_TIMER_IOCTL*HZ))
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
	{
		MV_DPRINT((  "__MV__  ioctl Io timerout req->Cdb[0-2](%x-%x-%x).\n",
		req->Cdb[0],req->Cdb[1],req->Cdb[2]));
		if (atomic_read(&hba->Ioctl_Io_Count)) {
			atomic_dec(&hba->Ioctl_Io_Count);
		}
#ifdef USE_REQ_POOL
		hba_req_cache_free(hba,req);
#else
		res_free_req_to_pool(hba->req_pool, req);
#endif
	        ret = -EIO;
	        goto clean_reqpool;
	}
	Scsi_Status = hba->desc->hba_desc->ioctl_cmpl_Scsi_Status;
	if (psptdwb->sptd.DataTransferLength) {
		error = copy_to_user((void __user *)karg.sptd.DataBuffer,
				     psptdwb->sptd.DataBuffer, psptdwb->sptd.DataTransferLength);
		if (error) {
			ret = -EIO;
			goto clean_reqpool;
		}
	}
	psptdwb->sptd.ScsiStatus = Scsi_Status;//req->Scsi_Status ;
	error = copy_to_user(uarg, (void *)psptdwb, sptdwb_size);
	if (error) {
		ret = -EIO;
		goto clean_reqpool;
	}

	if (Scsi_Status/*req->Scsi_Status*/)
		ret = Scsi_Status; //req->Scsi_Status;

	if (/*req->Scsi_Status*/Scsi_Status == REQ_STATUS_INVALID_PARAMETER)
		ret = -EPERM;

clean_reqpool:

clean_dmabuf:
	#ifdef SUPPORT_SES
	if( __is_scsi_cmd_rcv_snd_diag(psptdwb->sptd.Cdb[0])) {
		if(( psptdwb->sptd.Cdb[1] == 0x01) || (psptdwb->sptd.Cdb[1] == 0x10)) {
			if (NULL != vir_addr)
				pci_free_consistent(hba_desc->dev,  psptdwb->sptd.DataTransferLength,vir_addr,dma_addr);
		} else {
			if(ioctl_buf)
				hba_mem_free(ioctl_buf, psptdwb->sptd.DataTransferLength,MV_FALSE);
		}
	} else {
		if(ioctl_buf)
			hba_mem_free(ioctl_buf, psptdwb->sptd.DataTransferLength,MV_FALSE);
	}
	#endif
clean_pspbuf:
	hba_mem_free(psptdwb,sptdwb_size,MV_FALSE);

free_lock:
	up(&sem_lock);
	if (atomic_read(&hba->Ioctl_Io_Count)) {
		atomic_dec(&hba->Ioctl_Io_Count);
	}
	return ret;
}

static long
mv_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        long ret;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
     DECLARE_MUTEX(sem_ioctl_lock);
#else
     DEFINE_SEMAPHORE(sem_ioctl_lock);
#endif
        up(&sem_ioctl_lock);
#if defined (HAVE_UNLOCKED_IOCTL) && !defined (__VMKLNX__)
        ret = mv_ioctl(file, cmd, arg);
#else
		ret = mv_ioctl(NULL, file, cmd, arg);
#endif
        down(&sem_ioctl_lock);

        return ret;
}

#else  /* RAID_DRIVER */
inline int mv_register_chdev(struct hba_extension *hba) {return 0;}
inline void mv_unregister_chdev(struct hba_extension *hba) {}
#endif /* RAID_DRIVER */
