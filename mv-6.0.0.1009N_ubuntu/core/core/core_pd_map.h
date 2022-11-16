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

#ifndef _CORE_PD_MAP_H_
#define _CORE_PD_MAP_H_

/*******************************************************************************
 * Structs and defines                                                         *
 ******************************************************************************/
#ifdef PD_PAGE_PD_ID_AVAILABLE
#undef PD_PAGE_PD_ID_AVAILABLE
#endif

#ifdef PD_PAGE_DEBUG
#define CORE_PD_DPRINT(x)	CORE_DPRINT(x)
#else
#define CORE_PD_DPRINT(x)
#endif

#define PD_PAGE_PD_ID_AVAILABLE 	0xFFFF

/* ikkwong:TBD */
/* Move the max page num into lib_pd_page and initialize during init stage */
enum {
	MV_MAX_PD_BUFFER        = 1,
	MV_MAX_PD_PAGE          = 1,    /* Vanir has 128 drives = 1 page */
	MV_MAX_PD_PAGE_ENTRY    = MAX_PD_IN_PD_PAGE_FLASH,
};

enum {
	PD_FLAG_ONLINE          = (1U << 0),
	PD_FLAG_CACHE_ENABLE    = (1U << 1),
	PD_FLAG_MAPPED          = (1U << 2),
	PD_FLAG_NEW             = (1U << 3),
};

#define PD_PAGE_OFFSET		        (FLASH_PARAM_SIZE + \
					FLASH_PD_INFO_PAGE_SIZE + \
					PAGE_INTERVAL_DISTANCE)
#define PD_PAGE_SIZE                    (PD_PAGE_OFFSET * MV_MAX_PD_PAGE)

typedef struct _pd_page {
	MV_U8           page_num;
	MV_U8           avail_entry;
    MV_U8           rsvd[2];
	MV_U32          buf_addr;
} pd_page;

typedef struct _pd_page_buf {
	MV_U8           buf_page_num;
	MV_U8           page_changed;
    MV_U8           rsvd[2];
	MV_PU8          buf_ptr;
} pd_page_buf;

typedef struct _pd_page_lookup {
	MV_U8           page_num;
	MV_U8           pd_flag;
} pd_page_lookup;

typedef struct _lib_pd_page {
	pd_page         *pd;
	pd_page_buf     *pd_buf;
	pd_page_lookup  *pd_lookup;
	MV_U16          dev_init_index;         /* Index used during init */
    MV_U8           rsvd[2];
} lib_pd_page;

/*******************************************************************************
 * Function Prototypes                                                         *
 ******************************************************************************/

MV_U32 core_pd_get_cached_memory_quota(MV_U16 max_io);
MV_U32 core_pd_get_dma_memory_quota(MV_U16 max_io);
MV_U8 core_pd_init_memory(MV_PVOID core_p,
        lib_resource_mgr *rsrc, MV_U16 max_io);

#ifdef SUPPORT_PD_PAGE
MV_VOID core_pd_init(MV_PVOID core_p);
MV_VOID core_pd_flush_pd_buffer(MV_PVOID core_p);
MV_VOID core_pd_free_pd_buffer(MV_PVOID core_p);

MV_VOID core_pd_update_device_id(MV_PVOID core_p, MV_PVOID dev_p);
MV_VOID core_pd_update_all_device_id(MV_PVOID core_p);
MV_VOID core_pd_set_device_offline(MV_PVOID core_p, MV_PVOID dev_p);
MV_U8 core_pd_erase_pd_page(MV_PVOID core_p);
#ifdef SUPPORT_SAVE_CACHE_IN_FLASH
MV_VOID core_pd_set_device_cache_setting(MV_PVOID core_p, MV_PVOID dev_p,
        MV_PVOID data_entry, MV_U8 enable);
MV_VOID core_pd_update_all_device_cache_setting(MV_PVOID core_p,
        MV_U8 first_run);
MV_U8 core_pd_update_device_cache_setting(MV_PVOID core_p,
        MV_PVOID dev_p);
#endif /* SUPPORT_SAVE_CACHE_IN_FLASH */
#endif /* SUPPORT_PD_PAGE */

#endif /* _CORE_PD_MAP_H_ */
