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

#ifndef COM_EVENT_DEFINE_H
#define COM_EVENT_DEFINE_H

/****************************************
 *         Perceived Severity
 ****************************************/

#define SEVERITY_UNKNOWN    0
#define SEVERITY_OTHER      1
#define SEVERITY_INFO       2
#define SEVERITY_WARNING    3  /* used when its appropriate to let the 
				  user decide if action is needed */
#define SEVERITY_MINOR      4  /* indicate action is needed, but the 
				  situation is not serious at this time */  
#define SEVERITY_MAJOR      5  /* indicate action is needed NOW */
#define SEVERITY_CRITICAL   6  /* indicate action is needed NOW and the 
				  scope is broad */
#define SEVERITY_FATAL      7  /* indicate an error occurred, but it's too
				  late to take remedial action */

/****************************************
 *             Event Classes
 ****************************************/
#define EVT_CLASS_ADAPTER   0
#define EVT_CLASS_LD        1  /* Logical Drive */
#define EVT_CLASS_HD        2  /* Hard Drive */
#define EVT_CLASS_PM        3  /* Port Multplier */
#define EVT_CLASS_EXPANDER  4
#define EVT_CLASS_MDD       5 
#define EVT_CLASS_BSL       6  /* Bad Sector Lock */

/********************************************************
 *                 Event Codes 
 *
 *  !!!  When adding an EVT_CODE, Please put its severity level
 *  !!!  and suggested mesage string as comments.  This is the 
 *  !!!  only place to document how 'Params' in 'DriverEvent' 
 *  !!!  structure is to be used.
 *
 ********************************************************/
#ifndef _MARVELL_SDK_PACKAGE_NONRAID
/* Event code for EVT_CLASS_LD (Logical Drive) */
#define EVT_CODE_LD_OFFLINE                0
#define EVT_CODE_LD_ONLINE                 1
#define EVT_CODE_LD_CREATE                 2
#define EVT_CODE_LD_DELETE                 3
#define EVT_CODE_LD_DEGRADE                4
#define EVT_CODE_LD_PARTIALLYOPTIMAL       5
#define EVT_CODE_LD_CACHE_MODE_CHANGE      6
#define EVT_CODE_LD_FIXED                  7
#define EVT_CODE_LD_FOUND_ERROR            8
#define EVT_CODE_LD_IMPORTED               9
#define EVT_CODE_LD_REPORTED                10
#define EVT_CODE_LD_CONFLICTED              11
#define EVT_CODE_LD_RESERVED4              12
#define EVT_CODE_LD_RESERVED5              13
#define EVT_CODE_LD_RESERVED6              14
#define EVT_CODE_LD_RESERVED7              15
#define EVT_CODE_LD_RESERVED8              16
#define EVT_CODE_LD_RESERVED9              17
#define EVT_CODE_LD_RESERVED10             18
#define EVT_CODE_LD_RESERVED11             19
/*
 *  NOTE: Don't change the following event code order in each event group! 
 *      See raid_get_bga_event_id() for detail. 
 */
#define EVT_CODE_LD_CHECK_START            20 
//#define EVT_CODE_LD_CHECK_RESTART          21 
#define EVT_CODE_LD_CHECK_PAUSE            22 
#define EVT_CODE_LD_CHECK_RESUME           23 
#define EVT_CODE_LD_CHECK_ABORT            24 
#define EVT_CODE_LD_CHECK_COMPLETE         25 
#define EVT_CODE_LD_CHECK_PROGRESS         26  
#define EVT_CODE_LD_CHECK_ERROR            27 
#define EVT_CODE_LD_CHECK_AUTO_PAUSED      28 
#define EVT_CODE_LD_CHECK_AUTO_RESUME      29 

#define EVT_CODE_LD_FIX_START              30 
//#define EVT_CODE_LD_FIX_RESTART            31 
#define EVT_CODE_LD_FIX_PAUSE              32 
#define EVT_CODE_LD_FIX_RESUME             33 
#define EVT_CODE_LD_FIX_ABORT              34 
#define EVT_CODE_LD_FIX_COMPLETE           35 
#define EVT_CODE_LD_FIX_PROGRESS           36 
#define EVT_CODE_LD_FIX_ERROR              37
#define EVT_CODE_LD_FIX_AUTO_PAUSED        38 
#define EVT_CODE_LD_FIX_AUTO_RESUME        39 

#define EVT_CODE_LD_INIT_QUICK_START       40
//#define EVT_CODE_LD_INIT_QUICK_RESTART     41    
#define EVT_CODE_LD_INIT_QUICK_PAUSE       42    
#define EVT_CODE_LD_INIT_QUICK_RESUME      43    
#define EVT_CODE_LD_INIT_QUICK_ABORT       44    
#define EVT_CODE_LD_INIT_QUICK_COMPLETE    45    
#define EVT_CODE_LD_INIT_QUICK_PROGRESS    46  
#define EVT_CODE_LD_INIT_QUICK_ERROR       47   
#define EVT_CODE_LD_INIT_QUICK_AUTO_PAUSED 48
#define EVT_CODE_LD_INIT_QUICK_AUTO_RESUME 49 

#define EVT_CODE_LD_INIT_BACK_START        50
//#define EVT_CODE_LD_INIT_BACK_RESTART      51
#define EVT_CODE_LD_INIT_BACK_PAUSE        52
#define EVT_CODE_LD_INIT_BACK_RESUME       53
#define EVT_CODE_LD_INIT_BACK_ABORT        54
#define EVT_CODE_LD_INIT_BACK_COMPLETE     55
#define EVT_CODE_LD_INIT_BACK_PROGRESS     56
#define EVT_CODE_LD_INIT_BACK_ERROR        57
#define EVT_CODE_LD_INIT_BACK_AUTO_PAUSED  58  
#define EVT_CODE_LD_INIT_BACK_AUTO_RESUME  59 

#define EVT_CODE_LD_INIT_FORE_START        60
//#define EVT_CODE_LD_INIT_FORE_RESTART      61  
#define EVT_CODE_LD_INIT_FORE_PAUSE        62
#define EVT_CODE_LD_INIT_FORE_RESUME       63   
#define EVT_CODE_LD_INIT_FORE_ABORT        64
#define EVT_CODE_LD_INIT_FORE_COMPLETE     65 
#define EVT_CODE_LD_INIT_FORE_PROGRESS     66 
#define EVT_CODE_LD_INIT_FORE_ERROR        67
#define EVT_CODE_LD_INIT_FORE_AUTO_PAUSED  68 
#define EVT_CODE_LD_INIT_FORE_AUTO_RESUME  69 


#define EVT_CODE_LD_REBUILD_START          70 
//#define EVT_CODE_LD_REBUILD_RESTART        71 
#define EVT_CODE_LD_REBUILD_PAUSE          72 
#define EVT_CODE_LD_REBUILD_RESUME         73 
#define EVT_CODE_LD_REBUILD_ABORT          74 
#define EVT_CODE_LD_REBUILD_COMPLETE       75 
#define EVT_CODE_LD_REBUILD_PROGRESS       76 
#define EVT_CODE_LD_REBUILD_ERROR          77 
#define EVT_CODE_LD_REBUILD_AUTO_PAUSED    78 
#define EVT_CODE_LD_REBUILD_AUTO_RESUME	   79 

#define EVT_CODE_LD_MIGRATION_START        80 
//#define EVT_CODE_LD_MIGRATION_RESTART      81 
#define EVT_CODE_LD_MIGRATION_PAUSE        82 
#define EVT_CODE_LD_MIGRATION_RESUME       83 
#define EVT_CODE_LD_MIGRATION_ABORT        84 
#define EVT_CODE_LD_MIGRATION_COMPLETE     85 
#define EVT_CODE_LD_MIGRATION_PROGRESS     86
#define EVT_CODE_LD_MIGRATION_ERROR        87 
#define EVT_CODE_LD_MIGRATION_AUTO_PAUSED  88 
#define EVT_CODE_LD_MIGRATION_AUTO_RESUME  89 

#define EVT_CODE_LD_COPYBACK_START          90 
//#define EVT_CODE_LD_COPYBACK_RESTART        91 
#define EVT_CODE_LD_COPYBACK_PAUSE          92 
#define EVT_CODE_LD_COPYBACK_RESUME         93 
#define EVT_CODE_LD_COPYBACK_ABORT          94 
#define EVT_CODE_LD_COPYBACK_COMPLETE       95 
#define EVT_CODE_LD_COPYBACK_PROGRESS       96 
#define EVT_CODE_LD_COPYBACK_ERROR          97 
#define EVT_CODE_LD_COPYBACK_AUTO_PAUSED    98 
#define EVT_CODE_LD_COPYBACK_AUTO_RESUME	99 


/* event code for logging inconsistent LBA found in consistency check or synchronization fix */
#define EVT_CODE_LD_INCONSISTENT_LBA       100
#endif
// _MARVELL_SDK_PACKAGE_NONRAID
/* only used in application */
#define EVT_CODE_EVT_ERR                   0xffff
#define EVT_CODE_SMART_FROM_OFF_TO_ON	   0  // SMART setting is changed from OFF-->ON
#define EVT_CODE_SMART_FROM_ON_TO_OFF	   1  // SMART setting is changed from ON-->OFF
#define EVT_CODE_ALARM_TURN_ON			   2			 
#define EVT_CODE_ALARM_TURN_OFF			   3
#define EVT_CODE_AUTO_REBUILD_ON		   4	
#define EVT_CODE_AUTO_REBUILD_OFF		   5 
#define EVT_CODE_HD_MP_RATE_CHANGE		   6
#define EVT_CODE_COPY_BACK_ON			   7
#define EVT_CODE_COPY_BACK_OFF			   8
#define EVT_CODE_ADAPTER_FOUND			   9
#define EVT_CODE_BBU_ECCERROR			   10
#define EVT_CODE_ADAPTER_BBU_COUNT		   11
#define EVT_CODE_ADAPTER_BBU_START		   12
#define EVT_CODE_ADAPTER_BBU_FINISH		   13
#define EVT_CODE_PAGE_ECC_ERROR			   14
#define EVT_CODE_ADAPTER_MEMORY_SIZE_LACK	   15
#define EVT_CODE_ADAPTER_REMOVED			   16
#define EVT_CODE_ADAPTER_DETECTED			   17
#define EVT_CODE_ADAPTER_EVT_SYNC			   18

/*
 * Event code for EVT_CLASS_HD (Hard Disk)
 */
#define EVT_CODE_HD_OFFLINE                0
#define EVT_CODE_HD_ONLINE                 1 
#define EVT_CODE_HD_SETDOWN                2
#define EVT_CODE_HD_TIMEOUT                3
#define EVT_CODE_HD_RW_ERROR               4
#define EVT_CODE_HD_SMART                  5
#define EVT_CODE_HD_ERROR_FIXED            6
#define EVT_CODE_HD_PLUG_IN                7
#define EVT_CODE_HD_PLUG_OUT               8
#define EVT_CODE_HD_ASSIGN_SPARE           9
#define EVT_CODE_HD_REMOVE_SPARE           10
#define EVT_CODE_HD_SMART_THRESHOLD_OVER   11
/*New events added in March 2007 from LSI event list.*/
#define EVT_CODE_HD_SMART_POLLING_FAIL	   12  // SMART polling failed on %s (Error %02x)
#define EVT_CODE_BAD_BLOCK_TBL_80_FULL	   13  // Bad block table on PD %s is 80% full
#define EVT_CODE_BAD_BLOCK_TBL_FULL	       14  // Bad block table on PD %s is full; Unable to log block %x
#define EVT_CODE_BAD_BLOCK_REASSIGNED	   15  // Bad block reassigned on %s at %lx to %lx
#define EVT_CODE_HD_CACHE_MODE_CHANGE	   16
/*New event for HD media patrol.*/
#define EVT_CODE_HD_MP_START			   17 
//#define EVT_CODE_HD_MP_RESTART			   18
#define EVT_CODE_HD_MP_PAUSE			   19
#define EVT_CODE_HD_MP_RESUME              20 
#define EVT_CODE_HD_MP_ABORT               21
#define EVT_CODE_HD_MP_COMPLETE            22
#define EVT_CODE_HD_MP_PROGRESS            23
#define EVT_CODE_HD_MP_ERROR               24
#define EVT_CODE_HD_MP_AUTO_PAUSED         25
#define EVT_CODE_HD_MP_AUTO_RESUME         26

/*New event for HD (spare) data scrubbing.*/
#define EVT_CODE_HD_DS_START			   27 
//#define EVT_CODE_HD_DS_RESTART	           28
#define EVT_CODE_HD_DS_PAUSE	           29
#define EVT_CODE_HD_DS_RESUME              30 
#define EVT_CODE_HD_DS_ABORT               31
#define EVT_CODE_HD_DS_COMPLETE            32
#define EVT_CODE_HD_DS_PROGRESS            33
#define EVT_CODE_HD_DS_AUTO_PAUSED		   34
#define EVT_CODE_HD_DS_TERMINATE_IMMEDIATE 35

/*event for sense code*/
#define EVT_CODE_HD_SC_ERROR			36

#define EVT_CODE_HD_RCT_ADD					37
#define EVT_CODE_HD_RCT_REMOVE				38


#define EVT_CODE_HD_SC_RECOVERED_ERROR   40
#define EVT_CODE_HD_SC_NOT_READY   41
#define EVT_CODE_HD_SC_MEDIUM_ERROR   42
#define EVT_CODE_HD_SC_NO_SENSE   43
#define EVT_CODE_HD_SC_ILLEGAL_REQUEST   44
#define EVT_CODE_HD_SC_UNIT_ATTENTION    45
#define EVT_CODE_HD_SC_DATA_PROTECT   46
#define EVT_CODE_HD_SC_BLANK_CHECK   47
#define EVT_CODE_HD_SC_COPY_ABORTED   48
#define EVT_CODE_HD_SC_ABORTED_COMMAND   49
#define EVT_CODE_HD_SC_VOLUME_OVERFLOW   50
#define EVT_CODE_HD_SC_MISCOMPARE   51
#define EVT_CODE_HD_SC_HARDWARE_ERROR 52
#define EVT_CODE_HD_SC_VENDOR_SPECIFIC  53
#define EVT_CODE_HD_SETFREE				54
#define EVT_CODE_HD_OFF_PLAIN_DISK		55        // disk offline caused by plain disk on AES enabled port
#define EVT_CODE_HD_OFF_CRYPTO_DISK		56        // disk offline caused by crypto disk on AES disable port
#define EVT_CODE_HD_OFF_CIPHER_MISMATCH	57       // disk offline caused by cipher is mismatch between disk and port.
#define EVT_CODE_HD_SPARE_DISK_WARNING	59       // Params[0] is spare disk id.| (1 << 16) Params[1] is disk array id.
#define EVT_CODE_HD_READ_ERROR          60       // when do bga met read error on member disk
#define EVT_CODE_HD_WRITE_ERROR         61       // when do bga met write error on member disk

/*
 * code for EVT_CLASS_MDD
 */
#define EVT_CODE_MDD_ERROR                 0

/*
 * Event code for EVT_CLASS_EXPANDER
 */
#define EVT_CODE_EXPANDER_PLUG_IN        0
#define EVT_CODE_EXPANDER_PLUG_OUT       1

/**********************************
 *                Event IDs
 **********************************/

#ifndef _MARVELL_SDK_PACKAGE_NONRAID
/*
 * Event Id for EVT_CLASS_LD
 */
#define _CLASS_LD(x)                (EVT_CLASS_LD << 16 | (x))

#define EVT_ID_LD_OFFLINE            _CLASS_LD(EVT_CODE_LD_OFFLINE)
#define EVT_ID_LD_ONLINE             _CLASS_LD(EVT_CODE_LD_ONLINE) 
#define EVT_ID_LD_CREATE             _CLASS_LD(EVT_CODE_LD_CREATE)
#define EVT_ID_LD_DELETE             _CLASS_LD(EVT_CODE_LD_DELETE)
#define EVT_ID_LD_DEGRADE            _CLASS_LD(EVT_CODE_LD_DEGRADE)
#define EVT_ID_LD_PARTIALLYOPTIMAL   _CLASS_LD(EVT_CODE_LD_PARTIALLYOPTIMAL)
#define EVT_ID_LD_CACHE_MODE_CHANGE  _CLASS_LD(EVT_CODE_LD_CACHE_MODE_CHANGE)
#define EVT_ID_LD_FIXED              _CLASS_LD(EVT_CODE_LD_FIXED)
#define EVT_ID_LD_FOUND_ERROR        _CLASS_LD(EVT_CODE_LD_FOUND_ERROR)
#define EVT_ID_LD_IMPORTED	     _CLASS_LD(EVT_CODE_LD_IMPORTED)
#define EVT_ID_LD_REPORTED	     _CLASS_LD(EVT_CODE_LD_REPORTED)
#define EVT_ID_LD_CONFLICTED	     _CLASS_LD(EVT_CODE_LD_CONFLICTED)

#define EVT_ID_LD_CHECK_START        _CLASS_LD(EVT_CODE_LD_CHECK_START)
//#define EVT_ID_LD_CHECK_RESTART      _CLASS_LD(EVT_CODE_LD_CHECK_RESTART)
#define EVT_ID_LD_CHECK_PAUSE        _CLASS_LD(EVT_CODE_LD_CHECK_PAUSE)
#define EVT_ID_LD_CHECK_RESUME       _CLASS_LD(EVT_CODE_LD_CHECK_RESUME)
#define EVT_ID_LD_CHECK_ABORT        _CLASS_LD(EVT_CODE_LD_CHECK_ABORT)
#define EVT_ID_LD_CHECK_COMPLETE     _CLASS_LD(EVT_CODE_LD_CHECK_COMPLETE)
#define EVT_ID_LD_CHECK_PROGRESS     _CLASS_LD(EVT_CODE_LD_CHECK_PROGRESS)
#define EVT_ID_LD_CHECK_ERROR        _CLASS_LD(EVT_CODE_LD_CHECK_ERROR)
#define EVT_ID_LD_CHECK_AUTO_PAUSED  _CLASS_LD(EVT_CODE_LD_CHECK_AUTO_PAUSED)
#define EVT_ID_LD_CHECK_AUTO_RESUME  _CLASS_LD(EVT_CODE_LD_CHECK_AUTO_RESUME)

#define EVT_ID_LD_FIXED_START        _CLASS_LD(EVT_CODE_LD_FIX_START)
//#define EVT_ID_LD_FIXED_RESTART      _CLASS_LD(EVT_CODE_LD_FIX_RESTART)
#define EVT_ID_LD_FIXED_PAUSE        _CLASS_LD(EVT_CODE_LD_FIX_PAUSE)
#define EVT_ID_LD_FIXED_RESUME       _CLASS_LD(EVT_CODE_LD_FIX_RESUME)
#define EVT_ID_LD_FIXED_ABORT        _CLASS_LD(EVT_CODE_LD_FIX_ABORT)
#define EVT_ID_LD_FIXED_COMPLETE     _CLASS_LD(EVT_CODE_LD_FIX_COMPLETE)
#define EVT_ID_LD_FIXED_PROGRESS     _CLASS_LD(EVT_CODE_LD_FIX_PROGRESS)
#define EVT_ID_LD_FIXED_ERROR        _CLASS_LD(EVT_CODE_LD_FIX_ERROR)
#define EVT_ID_LD_FIXED_AUTO_PAUSED  _CLASS_LD(EVT_CODE_LD_FIX_AUTO_PAUSED)
#define EVT_ID_LD_FIXED_AUTO_RESUME  _CLASS_LD(EVT_CODE_LD_FIX_AUTO_RESUME)

#define EVT_ID_LD_INIT_QUICK_START   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_START)
//#define EVT_ID_LD_INIT_QUICK_RESTART _CLASS_LD(EVT_CODE_LD_INIT_QUICK_RESTART)
#define EVT_ID_LD_INIT_QUICK_PAUSE   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_PAUSE)
#define EVT_ID_LD_INIT_QUICK_RESUME  _CLASS_LD(EVT_CODE_LD_INIT_QUICK_RESUME)
#define EVT_ID_LD_INIT_QUICK_ABORT   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_ABORT)
#define EVT_ID_LD_INIT_QUICK_COMPLETE _CLASS_LD(EVT_CODE_LD_INIT_QUICK_COMPLETE)
#define EVT_ID_LD_INIT_QUICK_PROGRESS _CLASS_LD(EVT_CODE_LD_INIT_QUICK_PROGRESS)
#define EVT_ID_LD_INIT_QUICK_ERROR   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_ERROR)
#define EVT_ID_LD_INIT_QUICK_AUTO_PAUSED   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_AUTO_PAUSED)
#define EVT_ID_LD_INIT_QUICK_AUTO_RESUME   _CLASS_LD(EVT_CODE_LD_INIT_QUICK_AUTO_RESUME)

#define EVT_ID_LD_INIT_BACK_START    _CLASS_LD(EVT_CODE_LD_INIT_BACK_START)
//#define EVT_ID_LD_INIT_BACK_RESTART  _CLASS_LD(EVT_CODE_LD_INIT_BACK_RESTART)
#define EVT_ID_LD_INIT_BACK_PAUSE    _CLASS_LD(EVT_CODE_LD_INIT_BACK_PAUSE)
#define EVT_ID_LD_INIT_BACK_RESUME   _CLASS_LD(EVT_CODE_LD_INIT_BACK_RESUME)
#define EVT_ID_LD_INIT_BACK_ABORT    _CLASS_LD(EVT_CODE_LD_INIT_BACK_ABORT)
#define EVT_ID_LD_INIT_BACK_COMPLETE _CLASS_LD(EVT_CODE_LD_INIT_BACK_COMPLETE)
#define EVT_ID_LD_INIT_BACK_PROGRESS _CLASS_LD(EVT_CODE_LD_INIT_BACK_PROGRESS)
#define EVT_ID_LD_INIT_BACK_ERROR    _CLASS_LD(EVT_CODE_LD_INIT_BACK_ERROR)
#define EVT_ID_LD_INIT_BACK_AUTO_PAUSED   _CLASS_LD(EVT_CODE_LD_INIT_BACK_AUTO_PAUSED)
#define EVT_ID_LD_INIT_BACK_AUTO_RESUME   _CLASS_LD(EVT_CODE_LD_INIT_BACK_AUTO_RESUME)


#define EVT_ID_LD_INIT_FORE_START    _CLASS_LD(EVT_CODE_LD_INIT_FORE_START)
//#define EVT_ID_LD_INIT_FORE_RESTART  _CLASS_LD(EVT_CODE_LD_INIT_FORE_RESTART)
#define EVT_ID_LD_INIT_FORE_PAUSE    _CLASS_LD(EVT_CODE_LD_INIT_FORE_PAUSE)
#define EVT_ID_LD_INIT_FORE_RESUME   _CLASS_LD(EVT_CODE_LD_INIT_FORE_RESUME)
#define EVT_ID_LD_INIT_FORE_ABORT    _CLASS_LD(EVT_CODE_LD_INIT_FORE_ABORT)
#define EVT_ID_LD_INIT_FORE_COMPLETE _CLASS_LD(EVT_CODE_LD_INIT_FORE_COMPLETE)
#define EVT_ID_LD_INIT_FORE_PROGRESS _CLASS_LD(EVT_CODE_LD_INIT_FORE_PROGRESS)
#define EVT_ID_LD_INIT_FORE_ERROR    _CLASS_LD(EVT_CODE_LD_INIT_FORE_ERROR)
#define EVT_ID_LD_INIT_FORE_AUTO_PAUSED  _CLASS_LD(EVT_CODE_LD_INIT_FORE_AUTO_PAUSED)
#define EVT_ID_LD_INIT_FORE_AUTO_RESUME  _CLASS_LD(EVT_CODE_LD_INIT_FORE_AUTO_RESUME)

#define EVT_ID_LD_REBUILD_START      _CLASS_LD(EVT_CODE_LD_REBUILD_START)
//#define EVT_ID_LD_REBUILD_RESTART    _CLASS_LD(EVT_CODE_LD_REBUILD_RESTART)
#define EVT_ID_LD_REBUILD_PAUSE      _CLASS_LD(EVT_CODE_LD_REBUILD_PAUSE)
#define EVT_ID_LD_REBUILD_RESUME     _CLASS_LD(EVT_CODE_LD_REBUILD_RESUME)
#define EVT_ID_LD_REBUILD_ABORT      _CLASS_LD(EVT_CODE_LD_REBUILD_ABORT)
#define EVT_ID_LD_REBUILD_COMPLETE   _CLASS_LD(EVT_CODE_LD_REBUILD_COMPLETE)
#define EVT_ID_LD_REBUILD_PROGRESS   _CLASS_LD(EVT_CODE_LD_REBUILD_PROGRESS)
#define EVT_ID_LD_REBUILD_ERROR      _CLASS_LD(EVT_CODE_LD_REBUILD_ERROR)
#define EVT_ID_LD_REBUILD_AUTO_PAUSED _CLASS_LD(EVT_CODE_LD_REBUILD_AUTO_PAUSED)
#define EVT_ID_LD_REBUILD_AUTO_RESUME _CLASS_LD(EVT_CODE_LD_REBUILD_AUTO_RESUME)

#define EVT_ID_LD_MIGRATION_START    _CLASS_LD(EVT_CODE_LD_MIGRATION_START)
//#define EVT_ID_LD_MIGRATION_RESTART  _CLASS_LD(EVT_CODE_LD_MIGRATION_RESTART)
#define EVT_ID_LD_MIGRATION_PAUSE    _CLASS_LD(EVT_CODE_LD_MIGRATION_PAUSE)
#define EVT_ID_LD_MIGRATION_RESUME   _CLASS_LD(EVT_CODE_LD_MIGRATION_RESUME)
#define EVT_ID_LD_MIGRATION_ABORT    _CLASS_LD(EVT_CODE_LD_MIGRATION_ABORT)
#define EVT_ID_LD_MIGRATION_COMPLETE _CLASS_LD(EVT_CODE_LD_MIGRATION_COMPLETE)
#define EVT_ID_LD_MIGRATION_PROGRESS _CLASS_LD(EVT_CODE_LD_MIGRATION_PROGRESS)
#define EVT_ID_LD_MIGRATION_ERROR    _CLASS_LD(EVT_CODE_LD_MIGRATION_ERROR)
#define EVT_ID_LD_MIGRATION_AUTO_PAUSED    _CLASS_LD(EVT_CODE_LD_MIGRATION_AUTO_PAUSED)
#define EVT_ID_LD_MIGRATION_AUTO_RESUME    _CLASS_LD(EVT_CODE_LD_MIGRATION_AUTO_RESUME)

#define EVT_ID_LD_COPYBACK_START      _CLASS_LD(EVT_CODE_LD_COPYBACK_START)
//#define EVT_ID_LD_COPYBACK_RESTART    _CLASS_LD(EVT_CODE_LD_COPYBACK_RESTART)
#define EVT_ID_LD_COPYBACK_PAUSE      _CLASS_LD(EVT_CODE_LD_COPYBACK_PAUSE)
#define EVT_ID_LD_COPYBACK_RESUME     _CLASS_LD(EVT_CODE_LD_COPYBACK_RESUME)
#define EVT_ID_LD_COPYBACK_ABORT      _CLASS_LD(EVT_CODE_LD_COPYBACK_ABORT)
#define EVT_ID_LD_COPYBACK_COMPLETE   _CLASS_LD(EVT_CODE_LD_COPYBACK_COMPLETE)
#define EVT_ID_LD_COPYBACK_PROGRESS   _CLASS_LD(EVT_CODE_LD_COPYBACK_PROGRESS)
#define EVT_ID_LD_COPYBACK_ERROR      _CLASS_LD(EVT_CODE_LD_COPYBACK_ERROR)
#define EVT_ID_LD_COPYBACK_AUTO_PAUSED _CLASS_LD(EVT_CODE_LD_COPYBACK_AUTO_PAUSED)
#define EVT_ID_LD_COPYBACK_AUTO_RESUME _CLASS_LD(EVT_CODE_LD_COPYBACK_AUTO_RESUME)


#define EVT_ID_LD_INCONSISTENT_LBA   _CLASS_LD(EVT_CODE_LD_INCONSISTENT_LBA)
#endif
// _MARVELL_SDK_PACKAGE_NONRAID
/*
 * Event Id for EVT_CLASS_HD
 */
#define _CLASS_HD(x)                    (EVT_CLASS_HD << 16 | (x))

#define EVT_ID_HD_OFFLINE               _CLASS_HD(EVT_CODE_HD_OFFLINE)
#define EVT_ID_HD_ONLINE                _CLASS_HD(EVT_CODE_HD_ONLINE)
#define EVT_ID_HD_SETDOWN               _CLASS_HD(EVT_CODE_HD_SETDOWN)
#define EVT_ID_HD_TIMEOUT               _CLASS_HD(EVT_CODE_HD_TIMEOUT)
#define EVT_ID_HD_RW_ERROR              _CLASS_HD(EVT_CODE_HD_RW_ERROR)
#define EVT_ID_HD_SMART                 _CLASS_HD(EVT_CODE_HD_SMART)
#define EVT_ID_HD_ERROR_FIXED           _CLASS_HD(EVT_CODE_HD_ERROR_FIXED)
#define EVT_ID_HD_PLUG_IN               _CLASS_HD(EVT_CODE_HD_PLUG_IN)
#define EVT_ID_HD_PLUG_OUT              _CLASS_HD(EVT_CODE_HD_PLUG_OUT)
#define EVT_ID_HD_ASSIGN_SPARE          _CLASS_HD(EVT_CODE_HD_ASSIGN_SPARE)
#define EVT_ID_HD_REMOVE_SPARE          _CLASS_HD(EVT_CODE_HD_REMOVE_SPARE)
#define EVT_ID_HD_SMART_THRESHOLD_OVER  _CLASS_HD(EVT_CODE_HD_SMART_THRESHOLD_OVER)
#define EVT_ID_HD_SMART_POLLING_FAIL    _CLASS_HD(EVT_CODE_HD_SMART_POLLING_FAIL)		
#define EVT_ID_BAD_BLOCK_TBL_80_FULL    _CLASS_HD(EVT_CODE_BAD_BLOCK_TBL_80_FULL)
#define EVT_ID_BAD_BLOCK_TBL_FULL       _CLASS_HD(EVT_CODE_BAD_BLOCK_TBL_FULL)
#define EVT_ID_BAD_BLOCK_REASSIGNED     _CLASS_HD(EVT_CODE_BAD_BLOCK_REASSIGNED)
#define EVT_ID_HD_CACHE_MODE_CHANGE		_CLASS_HD(EVT_CODE_HD_CACHE_MODE_CHANGE)

#define EVT_ID_HD_MP_START				_CLASS_HD(EVT_CODE_HD_MP_START)
//#define EVT_ID_HD_MP_RESTART			_CLASS_HD(EVT_CODE_HD_MP_RESTART)
#define EVT_ID_HD_MP_PAUSE				_CLASS_HD(EVT_CODE_HD_MP_PAUSE)
#define EVT_ID_HD_MP_RESUME				_CLASS_HD(EVT_CODE_HD_MP_RESUME)
#define EVT_ID_HD_MP_ABORT				_CLASS_HD(EVT_CODE_HD_MP_ABORT)
#define EVT_ID_HD_MP_COMPLETE			_CLASS_HD(EVT_CODE_HD_MP_COMPLETE)
#define EVT_ID_HD_MP_PROGRESS			_CLASS_HD(EVT_CODE_HD_MP_PROGRESS)
#define EVT_ID_HD_MP_ERROR				_CLASS_HD(EVT_CODE_HD_MP_ERROR)
#define EVT_ID_HD_MP_AUTO_PAUSED		_CLASS_HD(EVT_CODE_HD_MP_AUTO_PAUSED)
#define EVT_ID_HD_MP_AUTO_RESUME		_CLASS_HD(EVT_CODE_HD_MP_AUTO_RESUME)

/* Event Id for Data Scrub */
#define EVT_ID_HD_DS_START				 _CLASS_HD(EVT_CODE_HD_DS_START)
//#define EVT_ID_HD_DS_RESTART			 _CLASS_HD(EVT_CODE_HD_DS_RESTART)
#define EVT_ID_HD_DS_PAUSE				 _CLASS_HD(EVT_CODE_HD_DS_PAUSE)
#define EVT_ID_HD_DS_RESUME				 _CLASS_HD(EVT_CODE_HD_DS_RESUME)
#define EVT_ID_HD_DS_ABORT				 _CLASS_HD(EVT_CODE_HD_DS_ABORT)
#define EVT_ID_HD_DS_COMPLETE			 _CLASS_HD(EVT_CODE_HD_DS_COMPLETE)
#define EVT_ID_HD_DS_PROGRESS			 _CLASS_HD(EVT_CODE_HD_DS_PROGRESS)
#define EVT_ID_HD_DS_AUTO_PAUSED		 _CLASS_HD(EVT_CODE_HD_DS_AUTO_PAUSED)
#define EVT_ID_HD_DS_TERMINATE_IMMEDIATE _CLASS_HD(EVT_CODE_HD_DS_TERMINATE_IMMEDIATE)

/* Event ID for sense code*/
#define EVT_ID_HD_SC_ERROR				_CLASS_HD(EVT_CODE_HD_SC_ERROR)

#define EVT_ID_HD_RCT_ADD				_CLASS_HD(EVT_CODE_HD_RCT_ADD)
#define EVT_ID_HD_RCT_REMOVE			_CLASS_HD(EVT_CODE_HD_RCT_REMOVE)

#define EVT_ID_HD_SC_NO_SENSE                   	  _CLASS_HD(EVT_CODE_HD_SC_NO_SENSE)
#define EVT_ID_HD_SC_RECOVERED_ERROR  	 	_CLASS_HD(EVT_CODE_HD_SC_RECOVERED_ERROR)
#define EVT_ID_HD_SC_NOT_READY   			_CLASS_HD(EVT_CODE_HD_SC_NOT_READY)
#define EVT_ID_HD_SC_MEDIUM_ERROR  		 _CLASS_HD(EVT_CODE_HD_SC_MEDIUM_ERROR)
#define EVT_ID_HD_SC_ILLEGAL_REQUEST   	_CLASS_HD(EVT_CODE_HD_SC_ILLEGAL_REQUEST)
#define EVT_ID_HD_SC_UNIT_ATTENTION   	_CLASS_HD(EVT_CODE_HD_SC_UNIT_ATTENTION)
#define EVT_ID_HD_SC_DATA_PROTECT  	 	_CLASS_HD(EVT_CODE_HD_SC_DATA_PROTECT)
#define EVT_ID_HD_SC_BLANK_CHECK   		_CLASS_HD(EVT_CODE_HD_SC_BLANK_CHECK)
#define EVT_ID_HD_SC_COPY_ABORTED   	_CLASS_HD(EVT_CODE_HD_SC_COPY_ABORTED)
#define EVT_ID_HD_SC_ABORTED_COMMAND   _CLASS_HD(EVT_CODE_HD_SC_ABORTED_COMMAND)
#define EVT_ID_HD_SC_VOLUME_OVERFLOW  _CLASS_HD(EVT_CODE_HD_SC_VOLUME_OVERFLOW)
#define EVT_ID_HD_SC_MISCOMPARE  		 _CLASS_HD(EVT_CODE_HD_SC_MISCOMPARE)
#define EVT_ID_HD_SC_HARDWARE_ERROR 		_CLASS_HD(EVT_CODE_HD_SC_HARDWARE_ERROR)
#define EVT_ID_HD_SC_VENDOR_SPECIFIC  		_CLASS_HD(EVT_CODE_HD_SC_VENDOR_SPECIFIC)
#define EVT_ID_HD_SETFREE					_CLASS_HD(EVT_CODE_HD_SETFREE)
#define EVT_ID_HD_OFF_PLAIN_DISK					_CLASS_HD(EVT_CODE_HD_OFF_PLAIN_DISK)
#define EVT_ID_HD_OFF_CRYPTO_DISK					_CLASS_HD(EVT_CODE_HD_OFF_CRYPTO_DISK)
#define EVT_ID_HD_OFF_CIPHER_MISMATCH					_CLASS_HD(EVT_CODE_HD_OFF_CIPHER_MISMATCH)
#define EVT_ID_HD_SPARE_DISK_WARNING					_CLASS_HD(EVT_CODE_HD_SPARE_DISK_WARNING)
#define EVT_ID_HD_READ_ERROR					        _CLASS_HD(EVT_CODE_HD_READ_ERROR)
#define EVT_ID_HD_WRITE_ERROR					        _CLASS_HD(EVT_CODE_HD_WRITE_ERROR)
/*
 * Id for EVT_CLASS_MDD
 */

#define _CLASS_MDD(x)                    (EVT_CLASS_MDD << 16 | (x))
#define EVT_ID_MDD_ERROR                 _CLASS_MDD(EVT_CODE_MDD_ERROR)

/*
 * Event Id for EVT_CLASS_EXPANDER
 */
#define _CLASS_EXPANDER(x)               (EVT_CLASS_EXPANDER << 16 | (x))

#define EVT_ID_EXPANDER_PLUG_IN          _CLASS_EXPANDER(EVT_CODE_EXPANDER_PLUG_IN)
#define EVT_ID_EXPANDER_PLUG_OUT         _CLASS_EXPANDER(EVT_CODE_EXPANDER_PLUG_OUT)


/*
 * Id for EVT_CLASS_ADAPTER
 */

#define _CLASS_ADPT(x)                   (EVT_CLASS_ADAPTER << 16 | (x))
#define EVT_ID_EVT_LOST                  _CLASS_ADPT(EVT_CODE_EVT_ERR)
#define EVT_ID_SMART_FROM_OFF_TO_ON		 _CLASS_ADPT(EVT_CODE_SMART_FROM_OFF_TO_ON)
#define EVT_ID_SMART_FROM_ON_TO_OFF		 _CLASS_ADPT(EVT_CODE_SMART_FROM_ON_TO_OFF)
#define EVT_ID_ALARM_TURN_ON			 _CLASS_ADPT(EVT_CODE_ALARM_TURN_ON)
#define EVT_ID_ALARM_TURN_OFF		     _CLASS_ADPT(EVT_CODE_ALARM_TURN_OFF)
#define EVT_ID_AUTO_REBUILD_ON		     _CLASS_ADPT(EVT_CODE_AUTO_REBUILD_ON)
#define EVT_ID_AUTO_REBUILD_OFF		     _CLASS_ADPT(EVT_CODE_AUTO_REBUILD_OFF)
#define EVT_ID_HD_MP_RATE_CHANGE		 _CLASS_ADPT(EVT_CODE_HD_MP_RATE_CHANGE)
#define EVT_ID_COPY_BACK_ON			     _CLASS_ADPT(EVT_CODE_COPY_BACK_ON)
#define EVT_ID_COPY_BACK_OFF		     _CLASS_ADPT(EVT_CODE_COPY_BACK_OFF)
#define EVT_ID_ADAPTER_FOUND			 _CLASS_ADPT(EVT_CODE_ADAPTER_FOUND)
#define EVT_ID_ADAPTER_ECC_ERROR			 _CLASS_ADPT(EVT_CODE_BBU_ECCERROR)
#define EVT_ID_ADAPTER_BBU_COUNT			 _CLASS_ADPT(EVT_CODE_ADAPTER_BBU_COUNT)
#define EVT_ID_ADAPTER_BBU_START			 _CLASS_ADPT(EVT_CODE_ADAPTER_BBU_START)
#define EVT_ID_ADAPTER_BBU_FINISH			 _CLASS_ADPT(EVT_CODE_ADAPTER_BBU_FINISH)
#define EVT_ID_ADAPTER_PAGE_ECC_ERROR			 _CLASS_ADPT(EVT_CODE_PAGE_ECC_ERROR)
#define EVT_ID_ADAPTER_MEMORY_SIZE_LACK		 _CLASS_ADPT(EVT_CODE_ADAPTER_MEMORY_SIZE_LACK)
#define EVT_ID_ADAPTER_REMOVED			_CLASS_ADPT(EVT_CODE_ADAPTER_REMOVED)
#define EVT_ID_ADAPTER_DETECTED			_CLASS_ADPT(EVT_CODE_ADAPTER_DETECTED)
#define EVT_ID_ADAPTER_EVT_SYNC			_CLASS_ADPT(EVT_CODE_ADAPTER_EVT_SYNC)
#endif /*  COM_EVENT_DEFINE_H */