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

#ifndef COM_EVENT_DEFINE_EXT_H
#define COM_EVENT_DEFINE_EXT_H

//===================================================================================
//===================================================================================
//        All these events are new ones but which are listed in LSI product.
//    Pay attention: All suggested display messages are from from LSI event list.
//  We may make some little change later, especially for new events in Loki.
//===================================================================================
//===================================================================================

//=======================================
//=======================================
//                Event Classes
//=======================================
//=======================================

#define    EVT_CLASS_SAS                7        // SAS, mainly for SAS topology
#define    EVT_CLASS_ENCL               8        // Enclosure
#define    EVT_CLASS_BAT                9       // Battery
#define    EVT_CLASS_FLASH              10      // Flash memory
#define    EVT_CLASS_CACHE              11      // Cache related
#define    EVT_CLASS_MISC               12      // For other miscellenous events
#define    EVT_CLASS_ARRAY              13      // Array
#define    EVT_CLASS_SSD                14      // SSD
#define    EVT_CLASS_OEM                15      // OEM event

//=============================================================
//                    Event Codes 
//
//    !!!  When adding an EVT_ID, Please put its severity level
//  !!!  and suggested mesage string as comments.  This is the 
//  !!!  only place to document how 'Params' in 'DriverEvent' 
//  !!!  structure is to be used.
//  !!!  Please refer to the EventMessages.doc to get details.
//=============================================================

//
// Event code for EVT_CLASS_SAS (sas)
//

#define EVT_CODE_SAS_LOOP_DETECTED                0  //SAS Topology error: Loop detected
#define EVT_CODE_SAS_UNADDR_DEVICE                1  //SAS Topology error: Unaddressable device
#define EVT_CODE_SAS_MULTIPORT_SAME_ADDR          2  //SAS Topology error: Multiple ports to the same SAS address
#define EVT_CODE_SAS_EXPANDER_ERR                 3  //SAS Topology error: Expander error
#define EVT_CODE_SAS_SMP_TIMEOUT                  4  //SAS Topology error: SMP timeout
#define EVT_CODE_SAS_OUT_OF_ROUTE_ENTRIES         5  //SAS Topology error: Out of route entries
#define EVT_CODE_SAS_INDEX_NOT_FOUND              6  //SAS Topology error: Index not found
#define EVT_CODE_SAS_SMP_FUNC_FAILED              7  //SAS Topology error: SMP function failed    
#define EVT_CODE_SAS_SMP_CRC_ERR                  8  //SAS Topology error: SMP CRC error
#define EVT_CODE_SAS_MULTI_SUBTRACTIVE            9  //SAS Topology error: Multiple subtractive
#define EVT_CODE_SAS_TABEL_TO_TABLE               10 //SAS Topology error: Table to Table
#define EVT_CODE_SAS_MULTI_PATHS                  11 //SAS Topology error: Multiple paths
#define EVT_CODE_SAS_WIDE_PORT_LOST_LINK_ON_PHY   12 //SAS wide port %d lost link on PHY %d 
#define EVT_CODE_SAS_WIDE_PORT_REST_LINK_ON_PHY   13 //SAS wide port %d restored link on PHY %d 
#define EVT_CODE_SAS_PHY_EXCEED_ERR_RATE          14 //SAS port %d, PHY %d has exceeded the allowed error rate
#define EVT_CODE_SAS_SATA_MIX_NOT_SUPPORTED       15 //SAS/SATA mixing not supported in enclosure: PD %d disabled

//
// Event code for EVT_CLASS_ENCL (enclosure)
//

#define    EVT_CODE_ENCL_SES_DISCOVERED            0   // Enclosure(SES) discovered on %d
#define    EVT_CODE_ENCL_SAFTE_DISCOVERED          1   // Enclosure(SAFTE) discovered on %d
#define    EVT_CODE_ENCL_COMMUNICATION_LOST        2   // Enclosure %d communication lost
#define    EVT_CODE_ENCL_COMMUNICATION_RESTORED    3   // Enclosure %d communication restored
#define    EVT_CODE_ENCL_FAN_FAILED                4   // Enclosure %d fan %d failed
#define    EVT_CODE_ENCL_FAN_INSERTED              5   // Enclosure %d fan %d inserted
#define    EVT_CODE_ENCL_FAN_REMOVED               6   // Enclosure %d fan %d removed
#define    EVT_CODE_ENCL_PS_FAILED                 7   // Enclosure %d power supply %d failed
#define    EVT_CODE_ENCL_PS_INSERTED               8   // Enclosure %d power supply %d inserted
#define    EVT_CODE_ENCL_PS_REMOVED                9   // Enclosure %d power supply %d removed
#define    EVT_CODE_ENCL_SIM_FAILED                10  // Enclosure %d SIM %d failed
#define    EVT_CODE_ENCL_SIM_INSERTED              11  // Enclosure %d SIM %d inserted
#define    EVT_CODE_ENCL_SIM_REMOVED               12  // Enclosure %d SIM %d removed
#define    EVT_CODE_ENCL_TEMP_SENSOR_BELOW_WARNING 13  // Enclosure %d temperature sensor %d below warning threshold
#define    EVT_CODE_ENCL_TEMP_SENSOR_BELOW_ERR     14  // Enclosure %d temperature sensor %d below error threshold
#define    EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_WARNING 15  // Enclosure %d temperature sensor %d above warning threshold
#define    EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_ERR     16  // Enclosure %d temperature sensor %d above error threshold 
#define EVT_CODE_ENCL_SHUTDOWN                     17  // Enclosure %d shutdown
#define EVT_CODE_ENCL_NOT_SUPPORTED                18  // Enclosure %d not supported; too many enclosures connected to port
#define    EVT_CODE_ENCL_FW_MISMATCH               19  // Enclosure %d firmware mismatch 
#define    EVT_CODE_ENCL_SENSOR_BAD                20  // Enclosure %d sensor %d bad
#define    EVT_CODE_ENCL_PHY_BAD                   21  // Enclosure %d phy %d bad
#define    EVT_CODE_ENCL_IS_UNSTABLE               22  // Enclosure %d is unstable
#define    EVT_CODE_ENCL_HW_ERR                    23  // Enclosure %d hardware error
#define    EVT_CODE_ENCL_NOT_RESPONDING            24  // Enclosure %d not responding
#define    EVT_CODE_ENCL_HOTPLUG_DETECTED          25  // Enclosure(SES) hotplug on %d was detected, but is not supported 
#define    EVT_CODE_ENCL_PS_SWITCHED_OFF           26  // Enclosure %d Power supply %d switched off
#define    EVT_CODE_ENCL_PS_SWITCHED_ON            27  // Enclosure %d Power supply %d switched on
#define    EVT_CODE_ENCL_PS_CABLE_REMOVED          28  // Enclosure %d Power supply %d cable removed
#define    EVT_CODE_ENCL_PS_CABLE_INSERTED         29  // Enclosure %d Power supply %d cable inserted
#define    EVT_CODE_ENCL_FAN_RETURN_TO_NORMAL      30  // Enclosure %d Fan %d returned to normal
#define    EVT_CODE_ENCL_TEMP_RETURN_TO_NORMAL     31  // Enclosure %d Temperature %d returned to normal
#define    EVT_CODE_ENCL_FW_DWLD_IN_PRGS           32  // Enclosure %d Firmware download in progress
#define    EVT_CODE_ENCL_FW_DWLD_FAILED            33  // Enclosure %d Firmware download failed
#define    EVT_CODE_ENCL_TEMP_SENSOR_DIFF_DETECTED 34  // Enclosure %d Temperature sensor %d differential detected
#define    EVT_CODE_ENCL_FAN_SPEED_CHANGED         35  // Enclosure %d fan %d speed changed


//
// Event code for EVT_CLASS_BAT
//

#define EVT_CODE_BAT_PRESENT                                0   // Battery present
#define EVT_CODE_BAT_NOT_PRESENT                            1   // Battery not present
#define EVT_CODE_BAT_NEW_BAT_DETECTED                       2   // New battery detected
#define EVT_CODE_BAT_NEED_REPLACE                           3   // Battery needs to be replaced
#define EVT_CODE_BAT_REPLACED                               4   // Battery has been replaced 
#define EVT_CODE_BAT_REMOVED                                5   // Battery is removed
#define EVT_CODE_BAT_RELEARN_STARTED                        6   // Battery relearn started
#define EVT_CODE_BAT_RELEARN_IN_PGRS                        7   // Battery relearn in progress
#define EVT_CODE_BAT_RELEARN_COMPLETED                      8   // Battery relearn completed
#define EVT_CODE_BAT_RELEARN_TIMED_OUT                      9   // Battery relearn timed out
#define EVT_CODE_BAT_RELEARN_PENDING                        10  // Battery relearn pending: Battery is under charge
#define EVT_CODE_BAT_RELEARN_POSTPONED                      11  // Battery relearn postponed
#define EVT_CODE_BAT_START_IN_4_DAYS                        12  // Battery relearn will start in 4 days
#define EVT_CODE_BAT_START_IN_2_DAYS                        13  // Battery relearn will start in 2 days
#define EVT_CODE_BAT_START_IN_1_DAY                         14  // Battery relearn will start in 1 days
#define EVT_CODE_BAT_START_IN_5_HOURS                       15  // Battery relearn will start in 5 hours
#define EVT_CODE_BAT_DISCHARGING                            16  // Battery is discharging
#define EVT_CODE_BAT_STARTED_CHARGING                       17  // Battery started charging
#define EVT_CODE_BAT_CHARGE_CMPLT                           18  // Battery completed charging
#define EVT_CODE_BAT_CHARGER_PROBLEM_DETECTED               19  // Battery/charger problems detected
#define EVT_CODE_BAT_CAPACITY_BELOW_THRESHOLD               20  // Battery capacity is below threshold 
#define EVT_CODE_BAT_CAPACITY_ABOVE_THRESHOLD               21  // Battery capacity is above threshold
#define EVT_CODE_BAT_TEMP_IS_LOW                            22  // Battery temperature is low
#define EVT_CODE_BAT_TEMP_IS_NORMAL                         23  // Battery temperature back to normal
#define EVT_CODE_BAT_TEMP_IS_HIGH                           24  // Battery temperature is high
#define EVT_CODE_BAT_VOLTAGE_LOW                            25  // Battery voltage is low
#define EVT_CODE_BAT_VOLTAGE_NORMAL                         26  // Battery voltage back to normal
#define EVT_CODE_BAT_VOLTAGE_HIGH                           27  // Battery voltage is high
#define EVT_CODE_BAT_FORCE_WRITE_THROUGH                    28  // Battery capacity is too low, force all VD to use write through mode
#define EVT_CODE_BAT_SAFE_TO_USE_WRITE_BACK                 29  // Battery capacity back to normal, VD is safe to use write back mode.
#define EVT_CODE_BAT_STOP_CHARGING                          30  // Battery stop charging.
#define EVT_CODE_BAT_STOP_DISCHARGING                       31  // Battery stop discharging.
#define EVT_CODE_BAT_CHANGE_CHARGE_THRESHOLD                32  // Battery change charge threshold.
#define EVT_CODE_BAT_CHANGE_VOLTAGE_WARNING_THRESHOLD       33  // Battery change voltage warning threshold.
#define EVT_CODE_BAT_CHANGE_TEMPERATURE_WARNING_THRESHOLD   34  // Battery change temperature warning threshold.


//
// Event code for EVT_CLASS_FLASH
//

#define EVT_CODE_FLASH_DWLDED_IMAGE_CORRUPTED       0    // Flash downloaded image corrupt
#define EVT_CODE_FLASH_ERASE_ERR                    1   // Flash erase error
#define EVT_CODE_FLASH_ERASE_TIMEOUT                2   // Flash timeout during erase
#define EVT_CODE_FLASH_FLASH_ERR                    3    // Flash error
#define EVT_CODE_FLASHING_IMAGE                     4    // Flashing image: %d
#define EVT_CODE_FLASHING_NEW_IMAGE_DONE            5   // Flash of new firmware images complete
#define EVT_CODE_FLASH_PROGRAMMING_ERR              6   // Flash programming error
#define EVT_CODE_FLASH_PROGRAMMING_TIMEOUT          7   // Flash timeout during programming
#define EVT_CODE_FLASH_UNKNOWN_CHIP_TYPE            8   // Flash chip type unknown 
#define EVT_CODE_FLASH_UNKNOWN_CMD_SET              9   // Flash command set unknown
#define EVT_CODE_FLASH_VERIFY_FAILURE               10  // Flash verify failure
#define EVT_CODE_NVRAM_CORRUPT                      11    // NVRAM is corrupt; reinitializing
#define EVT_CODE_NVRAM_MISMACTH_OCCURED             12  // NVRAM mismatch occured
#define EVT_CODE_NVRAM_RECONSTRUCTION_STARTED       13  // NVRAM reconstruction started    (for vili)
#define EVT_CODE_NVRAM_RECONSTRUCTION_DONE          14  // NVRAM reconstruction done    (for vili)

//
// Event code for new Flash command (Loki and Frey)
//
#define EVT_CODE_FLASH_WRITE_ERR                    15 // Flash operation failed (for Loki and Frey)
#define EVT_CODE_FLASH_READ_ERR                     16 // Flash operation failed (for Loki and Frey)
#define EVT_CODE_FLASH_WRITE_IMAGE_SUCCEEDED        17
#define EVT_CODE_FLASH_READ_IMAGE_SUCCEEDED         18
#define EVT_CODE_FLASH_WRITE_IMAGE_FAILED           19
#define EVT_CODE_FLASH_READ_IMAGE_FAILED            20
#define EVT_CODE_FLASH_GENERATION_ERR               21 // generation error (for Loki and Frey)
#define  EVT_CODE_FLASH_FATAL_GENERATION_ERR        22
#define EVT_CODE_FLASH_HEADER_ERR                   23 // Header error in flash (for Loki and Frey)
#define EVT_CODE_FLASH_DATA_ERR                     24 // Header error in flash (for Loki and Frey)
#define EVT_CODE_FLASH_DATA_AND_HEADER_ERR          25 // Header error in flash (for Loki and Frey)

//
// Event code for EVT_CLASS_CACHE(Cache)
//

#define EVT_CODE_CACHE_NOT_RECV_FROM_TBBU           0    // Unable to recover cache data from TBBU
#define EVT_CODE_CACHE_RECVD_FROM_TBBU              1   // Cache data recovered from TBBU successfully
#define EVT_CODE_CACHE_CTRLER_CACHE_DISCARDED       2   // Controller cache discarded due to memory/battery problems
#define EVT_CODE_CACHE_FAIL_RECV_DUETO_MISMATCH     3   // Unable to recover cache data due to configuration mismatch 
#define EVT_CODE_CACHE_DIRTY_DATA_DISCARDED         4    // Dirty cache data discarded by user
#define EVT_CODE_CACHE_FLUSH_RATE_CHANGED           5   // Flush rate changed to %d seconds.


//
// Event code for EVT_CLASS_MISC
//

#define EVT_CODE_MISC_CONFIG_CLEARED                0    // Configuration cleared
#define EVT_CODE_MISC_CHANGE_BACK_ACTIVITY_RATE     1    // Background activity rate changed to %d%%
#define EVT_CODE_MISC_FATAL_FW_ERR                  2   // Fatal firmware error: %d
#define EVT_CODE_MISC_FACTORY_DEFAULTS_RESTORED     3   // Factory defaults restored
#define EVT_CODE_MISC_GET_HIBER_CMD                 4   // Hibernation command received from host
#define EVT_CODE_MISC_MUTLI_BIT_ECC_ERR             5    // Multi-bit ECC error: count = %d, address = 0x%X, module ID = %d
#define EVT_CODE_MISC_SINGLE_BIT_ECC_ERR            6   // Single-bit ECC error: count = %d
#define EVT_CODE_MISC_GET_SHUTDOWN_CMD              7    // Shutdown command received from host
#define EVT_CODE_MISC_TIME_ESTABLISHED              8    // Time established as %d; (%d seconds since power on)
#define EVT_CODE_MISC_USER_ENTERED_DEBUGGER         9   // User entered firmware debugger
#define EVT_CODE_MISC_FORMAT_COMPLETE               10    // Format complete on %d
#define EVT_CODE_MISC_FORMAT_STARTED                11    // Format started on %d 
#define EVT_CODE_MISC_REASSIGN_WRITE_OP             12    // Reassign write operation on %d is %d
#define EVT_CODE_MISC_UNEXPECTED_SENSE              13    // Unexpected sense: %d, CDB%d, Sense: %d
#define EVT_CODE_MISC_REPLACED_MISSING              14    // Replaced missing as %d on array %d row %d
#define EVT_CODE_MISC_NOT_A_CERTIFIED_DRIVE         15  // %d is not a certificated derive

/* May put into other group???*/
#define EVT_CODE_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT    16    // PDs missing from configuration on boot    
#define EVT_CODE_MISC_VD_MISSING_DRIVES                 17  // VDs missing drives and will go offline at boot: %d
#define EVT_CODE_MISC_VD_MISSING_AT_BOOT                18  // VDs missing at boot: %d
#define EVT_CODE_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT   19  // Previous configuration completely missing at boot
#define EVT_CODE_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD      20  // PD too small to be used for auto-rebuild on %d.
#define EVT_CODE_MISC_PD_TOO_SMALL_FOR_COPYBACK         21
#define EVT_CODE_MISC_CHANGE_SYNC_RATE                  22    // Synchronize rate changed to %d%%
#define EVT_CODE_MISC_CHANGE_INIT_RATE                  23    // Init rate changed to %d%%
#define EVT_CODE_MISC_CHANGE_REBUILD_RATE               24    // Rebuild rate changed to %d%%
#define EVT_CODE_MISC_CHANGE_MIGRATION_RATE             25    // Migration rate changed to %d%%
#define EVT_CODE_MISC_CHANGE_COPYBACK_RATE              26    // Copyback rate changed to %d%%
#define EVT_CODE_MISC_CHANGE_MP_RATE                    27    // Media Patrol rate changed to %d%%
#define EVT_CODE_MISC_ALARM_MUTE                        28    // Adapter alarm set to mute
//
// Event code for EVT_CLASS_ARRAY
//

#define EVT_CODE_ARRAY_CREATE                            0
#define EVT_CODE_ARRAY_DELETE                            1

//
// Event code for EVT_CLASS_OEM
//

#define _CLASS_OEM(x)                (EVT_CLASS_OEM << 16 | (x))

#define EVT_CODE_ADAPTER_TEMP_OPTIMAL                   0  // temperature check pass
#define EVT_CODE_ADAPTER_TEMP_WARN                      1  // adapter is high temperature
#define EVT_CODE_ADAPTER_TEMP_CRITICAL                  2  // adapter is critical temperature, immediate action requested
#define EVT_CODE_ADAPTER_FAN_OPTIMAL                    3  // fan check pass
#define EVT_CODE_ADAPTER_FAN_FAILURE                    4  // defective fan %d found
#define EVT_CODE_ADAPTER_FAN_ABSENT                     5  // absent fan %d found
#define EVT_CODE_ADAPTER_PSU_ALERT                      6  // power supply alert
#define EVT_CODE_ADAPTER_PSU_OPTIMAL                    7  // power supply is optimal
#define EVT_CODE_ADAPTER_MUTE_PRESS                     8  // mute is pressed
#define EVT_CODE_ADAPTER_IDENTFICATION_PRESS            9  // identification is pressed
#define EVT_CODE_ADAPTER_IDENTFICATION_ON               10 // identification is pressed
#define EVT_CODE_ADAPTER_IDENTFICATION_OFF              11 // identification is pressed
#define EVT_CODE_ADAPTER_INTRUSION_OPEN                 12 // intrusion is open
#define EVT_CODE_ADAPTER_INTRUSION_CLOSE                13 // intrusion is close
#define EVT_CODE_ADAPTER_STATUS_LOW_POWER               14 // adapter become low power status
#define EVT_CODE_ADAPTER_STATUS_IDLE                    15 // adapter become idle status

//=======================================
//=======================================
//                Event IDs
//=======================================
//=======================================

//
// Event id for EVT_CLASS_SAS
//

#define _CLASS_SAS(x)                (EVT_CLASS_SAS << 16 | (x))

#define EVT_ID_SAS_LOOP_DETECTED                       _CLASS_SAS(EVT_CODESAS_LOOP_DETECTED)
#define EVT_ID_SAS_UNADDR_DEVICE                       _CLASS_SAS(EVT_CODESAS_UNADDR_DEVICE)            
#define EVT_ID_SAS_MULTIPORT_SAME_ADDR                 _CLASS_SAS(EVT_CODESAS_MULTIPORT_SAME_ADDR)
#define EVT_ID_SAS_EXPANDER_ERR                        _CLASS_SAS(EVT_CODESAS_EXPANDER_ERR)
#define EVT_ID_SAS_SMP_TIMEOUT                         _CLASS_SAS(EVT_CODESAS_SMP_TIMEOUT)
#define EVT_ID_SAS_OUT_OF_ROUTE_ENTRIES                _CLASS_SAS(EVT_CODESAS_OUT_OF_ROUTE_ENTRIES)
#define EVT_ID_SAS_INDEX_NOT_FOUND                     _CLASS_SAS(EVT_CODESAS_INDEX_NOT_FOUND)
#define EVT_ID_SAS_SMP_FUNC_FAILED                     _CLASS_SAS(EVT_CODESAS_SMP_FUNC_FAILED)
#define EVT_ID_SAS_SMP_CRC_ERR                         _CLASS_SAS(EVT_CODESAS_SMP_CRC_ERR)
#define EVT_ID_SAS_MULTI_SUBTRACTIVE                   _CLASS_SAS(EVT_CODESAS_MULTI_SUBTRACTIVE)
#define EVT_ID_SAS_TABEL_TO_TABLE                      _CLASS_SAS(EVT_CODESAS_TABEL_TO_TABLE)
#define EVT_ID_SAS_MULTI_PATHS                         _CLASS_SAS(EVT_CODESAS_MULTI_PATHS)
#define EVT_ID_SAS_WIDE_PORT_LOST_LINK_ON_PHY          _CLASS_SAS(EVT_CODESAS_WIDE_PORT_LOST_LINK_ON_PHY)
#define EVT_ID_SAS_WIDE_PORT_REST_LINK_ON_PHY          _CLASS_SAS(EVT_CODESAS_WIDE_PORT_REST_LINK_ON_PHY)
#define EVT_ID_SAS_PHY_EXCEED_ERR_RATE                 _CLASS_SAS(EVT_CODESAS_PHY_EXCEED_ERR_RATE)
#define EVT_ID_SAS_SATA_MIX_NOT_SUPPORTED              _CLASS_SAS(EVT_CODESAS_SATA_MIX_NOT_SUPPORTED)

//
// Event id for EVT_CLASS_ENCL (enclosure)
//

#define _CLASS_ENCL(x)                (EVT_CLASS_ENCL << 16 | (x))

#define    EVT_ID_ENCL_SES_DISCOVERED                   _CLASS_ENCL(EVT_CODE_ENCL_SES_DISCOVERED)                
#define    EVT_ID_ENCL_SAFTE_DISCOVERED                 _CLASS_ENCL(EVT_CODE_ENCL_SAFTE_DISCOVERED)
#define    EVT_ID_ENCL_COMMUNICATION_LOST               _CLASS_ENCL(EVT_CODE_ENCL_COMMUNICATION_LOST)
#define    EVT_ID_ENCL_COMMUNICATION_RESTORED           _CLASS_ENCL(EVT_CODE_ENCL_COMMUNICATION_RESTORED)
#define    EVT_ID_ENCL_FAN_FAILED                       _CLASS_ENCL(EVT_CODE_ENCL_FAN_FAILED)
#define    EVT_ID_ENCL_FAN_INSERTED                     _CLASS_ENCL(EVT_CODE_ENCL_FAN_INSERTED)
#define    EVT_ID_ENCL_FAN_REMOVED                      _CLASS_ENCL(EVT_CODE_ENCL_FAN_REMOVED)
#define    EVT_ID_ENCL_PS_FAILED                        _CLASS_ENCL(EVT_CODE_ENCL_PS_FAILED)
#define    EVT_ID_ENCL_PS_INSERTED                      _CLASS_ENCL(EVT_CODE_ENCL_PS_INSERTED)
#define    EVT_ID_ENCL_PS_REMOVED                       _CLASS_ENCL(EVT_CODE_ENCL_PS_REMOVED)
#define    EVT_ID_ENCL_SIM_FAILED                       _CLASS_ENCL(EVT_CODE_ENCL_SIM_FAILED)
#define    EVT_ID_ENCL_SIM_INSERTED                     _CLASS_ENCL(EVT_CODE_ENCL_SIM_INSERTED)
#define    EVT_ID_ENCL_SIM_REMOVED                      _CLASS_ENCL(EVT_CODE_ENCL_SIM_REMOVED)
#define    EVT_ID_ENCL_TEMP_SENSOR_BELOW_WARNING        _CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_BELOW_WARNING)
#define    EVT_ID_ENCL_TEMP_SENSOR_BELOW_ERR            _CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_BELOW_ERR)
#define    EVT_ID_ENCL_TEMP_SENSOR_ABOVE_WARNING        _CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_WARNING)
#define    EVT_ID_ENCL_TEMP_SENSOR_ABOVE_ERR            _CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_ERR)
#define    EVT_ID_ENCL_SHUTDOWN                         _CLASS_ENCL(EVT_CODE_ENCL_SHUTDOWN)
#define    EVT_ID_ENCL_NOT_SUPPORTED                    _CLASS_ENCL(EVT_CODE_ENCL_NOT_SUPPORTED)
#define    EVT_ID_ENCL_FW_MISMATCH                      _CLASS_ENCL(EVT_CODE_ENCL_FW_MISMATCH)
#define    EVT_ID_ENCL_SENSOR_BAD                       _CLASS_ENCL(EVT_CODE_ENCL_SENSOR_BAD)
#define    EVT_ID_ENCL_PHY_BAD                          _CLASS_ENCL(EVT_CODE_ENCL_PHY_BAD)
#define    EVT_ID_ENCL_IS_UNSTABLE                      _CLASS_ENCL(EVT_CODE_ENCL_IS_UNSTABLE)
#define    EVT_ID_ENCL_HW_ERR                           _CLASS_ENCL(EVT_CODE_ENCL_HW_ERR)
#define    EVT_ID_ENCL_NOT_RESPONDING                   _CLASS_ENCL(EVT_CODE_ENCL_NOT_RESPONDING)
#define    EVT_ID_ENCL_HOTPLUG_DETECTED                 _CLASS_ENCL(EVT_CODE_ENCL_HOTPLUG_DETECTED)
#define    EVT_ID_ENCL_PS_SWITCHED_OFF                  _CLASS_ENCL(EVT_CODE_ENCL_PS_SWITCHED_OFF    )    
#define    EVT_ID_ENCL_PS_SWITCHED_ON                   _CLASS_ENCL(EVT_CODE_ENCL_PS_SWITCHED_ON)
#define    EVT_ID_ENCL_PS_CABLE_REMOVED                 _CLASS_ENCL(EVT_CODE_ENCL_PS_CABLE_REMOVED)
#define    EVT_ID_ENCL_PS_CABLE_INSERTED                _CLASS_ENCL(EVT_CODE_ENCL_PS_CABLE_INSERTED)
#define    EVT_ID_ENCL_FAN_RETURN_TO_NORMAL             _CLASS_ENCL(EVT_CODE_ENCL_FAN_RETURN_TO_NORMAL)    
#define    EVT_ID_ENCL_TEMP_RETURN_TO_NORMAL            _CLASS_ENCL(EVT_CODE_ENCL_TEMP_RETURN_TO_NORMAL)        
#define    EVT_ID_ENCL_FW_DWLD_IN_PRGS                  _CLASS_ENCL(EVT_CODE_ENCL_FW_DWLD_IN_PRGS    )
#define    EVT_ID_ENCL_FW_DWLD_FAILED                   _CLASS_ENCL(EVT_CODE_ENCL_FW_DWLD_FAILED)    
#define    EVT_ID_ENCL_TEMP_SENSOR_DIFF_DETECTED        _CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_DIFF_DETECTED)
#define    EVT_ID_ENCL_FAN_SPEED_CHANGED                _CLASS_ENCL(EVT_CODE_ENCL_FAN_SPEED_CHANGED)

//
// Event id for EVT_CLASS_BAT
//

#define _CLASS_BAT(x)                (EVT_CLASS_BAT << 16 | (x))

#define EVT_ID_BAT_PRESENT                               _CLASS_BAT(EVT_CODE_BAT_PRESENT)
#define EVT_ID_BAT_NOT_PRESENT                           _CLASS_BAT(EVT_CODE_BAT_NOT_PRESENT)
#define EVT_ID_BAT_NEW_BAT_DETECTED                      _CLASS_BAT(EVT_CODE_BAT_NEW_BAT_DETECTED)
#define EVT_ID_BAT_REPLACED                              _CLASS_BAT(EVT_CODE_BAT_REPLACED)
#define EVT_ID_BAT_TEMP_IS_HIGH                          _CLASS_BAT(EVT_CODE_BAT_TEMP_IS_HIGH)
#define EVT_ID_BAT_VOLTAGE_LOW                           _CLASS_BAT(EVT_CODE_BAT_VOLTAGE_LOW)
#define EVT_ID_BAT_STARTED_CHARGING                      _CLASS_BAT(EVT_CODE_BAT_STARTED_CHARGING)
#define EVT_ID_BAT_DISCHARGING                           _CLASS_BAT(EVT_CODE_BAT_DISCHARGING)
#define EVT_ID_BAT_TEMP_IS_NORMAL                        _CLASS_BAT(EVT_CODE_BAT_TEMP_IS_NORMAL)
#define EVT_ID_BAT_NEED_REPLACE                          _CLASS_BAT(EVT_CODE_BAT_NEED_REPLACE)
#define EVT_ID_BAT_RELEARN_STARTED                       _CLASS_BAT(EVT_CODE_BAT_RELEARN_STARTED)
#define EVT_ID_BAT_RELEARN_IN_PGRS                       _CLASS_BAT(EVT_CODE_BAT_RELEARN_IN_PGRS)
#define EVT_ID_BAT_RELEARN_COMPLETED                     _CLASS_BAT(EVT_CODE_BAT_RELEARN_COMPLETED)
#define EVT_ID_BAT_RELEARN_TIMED_OUT                     _CLASS_BAT(EVT_CODE_BAT_RELEARN_TIMED_OUT)
#define EVT_ID_BAT_RELEARN_PENDING                       _CLASS_BAT(EVT_CODE_BAT_RELEARN_PENDING)
#define EVT_ID_BAT_RELEARN_POSTPONED                     _CLASS_BAT(EVT_CODE_BAT_RELEARN_POSTPONED)
#define EVT_ID_BAT_START_IN_4_DAYS                       _CLASS_BAT(EVT_CODE_BAT_START_IN_4_DAYS)
#define EVT_ID_BAT_START_IN_2_DAYS                       _CLASS_BAT(EVT_CODE_BAT_START_IN_2_DAYS)
#define EVT_ID_BAT_START_IN_1_DAY                        _CLASS_BAT(EVT_CODE_BAT_START_IN_1_DAY)
#define EVT_ID_BAT_START_IN_5_HOURS                      _CLASS_BAT(EVT_CODE_BAT_START_IN_5_HOURS)
#define EVT_ID_BAT_REMOVED                               _CLASS_BAT(EVT_CODE_BAT_REMOVED)
#define EVT_ID_BAT_CHARGE_CMPLT                          _CLASS_BAT(EVT_CODE_BAT_CHARGE_CMPLT)
#define EVT_ID_BAT_CHARGER_PROBLEM_DETECTED              _CLASS_BAT(EVT_CODE_BAT_CHARGER_PROBLEM_DETECTED)
#define EVT_ID_BAT_CAPACITY_BELOW_THRESHOLD              _CLASS_BAT(EVT_CODE_BAT_CAPACITY_BELOW_THRESHOLD)
#define EVT_ID_BAT_CAPACITY_ABOVE_THRESHOLD              _CLASS_BAT(EVT_CODE_BAT_CAPACITY_ABOVE_THRESHOLD)
#define EVT_ID_BAT_TEMP_IS_LOW                           _CLASS_BAT(EVT_CODE_BAT_TEMP_IS_LOW)
#define EVT_ID_BAT_TEMP_IS_NORMAL                        _CLASS_BAT(EVT_CODE_BAT_TEMP_IS_NORMAL)
#define EVT_ID_BAT_TEMP_IS_HIGH                          _CLASS_BAT(EVT_CODE_BAT_TEMP_IS_HIGH)
#define EVT_ID_BAT_VOLTAGE_LOW                           _CLASS_BAT(EVT_CODE_BAT_VOLTAGE_LOW)
#define EVT_ID_BAT_VOLTAGE_NORMAL                        _CLASS_BAT(EVT_CODE_BAT_VOLTAGE_NORMAL)
#define EVT_ID_BAT_VOLTAGE_HIGH                          _CLASS_BAT(EVT_CODE_BAT_VOLTAGE_HIGH)
#define EVT_ID_BAT_FORCE_WRITE_THROUGH                   _CLASS_BAT(EVT_CODE_BAT_FORCE_WRITE_THROUGH)
#define EVT_ID_BAT_SAFE_TO_USE_WRITE_BACK                _CLASS_BAT(EVT_CODE_BAT_SAFE_TO_USE_WRITE_BACK)
#define EVT_ID_BAT_STOP_CHARGING                         _CLASS_BAT(EVT_CODE_BAT_STOP_CHARGING)
#define EVT_ID_BAT_STOP_DISCHARGING                      _CLASS_BAT(EVT_CODE_BAT_STOP_DISCHARGING)
#define EVT_ID_BAT_CHANGE_CHARGE_THRESHOLD               _CLASS_BAT(EVT_CODE_BAT_CHANGE_CHARGE_THRESHOLD)
#define EVT_ID_BAT_CHANGE_VOLTAGE_WARNING_THRESHOLD      _CLASS_BAT(EVT_CODE_BAT_CHANGE_VOLTAGE_WARNING_THRESHOLD)
#define EVT_ID_BAT_CHANGE_TEMPERATURE_WARNING_THRESHOLD  _CLASS_BAT(EVT_CODE_BAT_CHANGE_TEMPERATURE_WARNING_THRESHOLD)

//
// Event id for EVT_CLASS_FLASH
//
#define _CLASS_FLASH(x)                (EVT_CLASS_FLASH << 16 | (x))

#define EVT_ID_FLASH_DWLDED_IMAGE_CORRUPTED            _CLASS_FLASH(EVT_CODE_FLASH_DWLDED_IMAGE_CORRUPTED)
#define EVT_ID_FLASH_ERASE_ERR                         _CLASS_FLASH(EVT_CODE_FLASH_ERASE_ERR)
#define EVT_ID_FLASH_ERASE_TIMEOUT                     _CLASS_FLASH(EVT_CODE_FLASH_ERASE_TIMEOUT)         
#define EVT_ID_FLASH_FLASH_ERR                         _CLASS_FLASH(EVT_CODE_FLASH_FLASH_ERR)             
#define EVT_ID_FLASHING_IMAGE                          _CLASS_FLASH(EVT_CODE_FLASHING_IMAGE)              
#define EVT_ID_FLASHING_NEW_IMAGE_DONE                 _CLASS_FLASH(EVT_CODE_FLASHING_NEW_IMAGE_DONE)     
#define EVT_ID_FLASH_PROGRAMMING_ERR                   _CLASS_FLASH(EVT_CODE_FLASH_PROGRAMMING_ERR)       
#define EVT_ID_FLASH_PROGRAMMING_TIMEOUT               _CLASS_FLASH(EVT_CODE_FLASH_PROGRAMMING_TIMEOUT)   
#define EVT_ID_FLASH_UNKNOWN_CHIP_TYPE                 _CLASS_FLASH(EVT_CODE_FLASH_UNKNOWN_CHIP_TYPE)     
#define EVT_ID_FLASH_UNKNOWN_CMD_SET                   _CLASS_FLASH(EVT_CODE_FLASH_UNKNOWN_CMD_SET)       
#define EVT_ID_FLASH_VERIFY_FAILURE                    _CLASS_FLASH(EVT_CODE_FLASH_VERIFY_FAILURE)        
#define EVT_ID_NVRAM_CORRUPT                           _CLASS_FLASH(EVT_CODE_NVRAM_CORRUPT)               
#define EVT_ID_NVRAM_MISMACTH_OCCURED                  _CLASS_FLASH(EVT_CODE_NVRAM_MISMACTH_OCCURED)      
#define EVT_ID_NVRAM_RECONSTRUCTION_STARTED            _CLASS_FLASH(EVT_CODE_NVRAM_RECONSTRUCTION_STARTED)
#define EVT_ID_NVRAM_RECONSTRUCTION_DONE               _CLASS_FLASH(EVT_CODE_NVRAM_RECONSTRUCTION_DONE)     

//
// Event code for new Flash command (Loki and Frey)
//
#define EVT_ID_FLASH_WRITE_ERR                         _CLASS_FLASH(EVT_CODE_FLASH_WRITE_ERR)
#define EVT_ID_FLASH_READ_ERR                          _CLASS_FLASH(EVT_CODE_FLASH_READ_ERR)     
#define EVT_ID_FLASH_WRITE_IMAGE_SUCCEEDED             _CLASS_FLASH(EVT_CODE_FLASH_WRITE_IMAGE_SUCCEEDED)
#define EVT_ID_FLASH_READ_IMAGE_SUCCEEDED              _CLASS_FLASH(EVT_CODE_FLASH_READ_IMAGE_SUCCEEDED)
#define EVT_ID_FLASH_WRITE_IMAGE_FAILED                _CLASS_FLASH(EVT_CODE_FLASH_WRITE_IMAGE_FAILED)
#define EVT_ID_FLASH_READ_IMAGE_FAILED                 _CLASS_FLASH(EVT_CODE_FLASH_READ_IMAGE_FAILED)
#define EVT_ID_FLASH_GENERATION_ERR                    _CLASS_FLASH(EVT_CODE_FLASH_GENERATION_ERR)
#define EVT_ID_FLASH_FATAL_GENERATION_ERR              _CLASS_FLASH(EVT_CODE_FLASH_FATAL_GENERATION_ERR)
#define EVT_ID_FLASH_HEADER_ERR                        _CLASS_FLASH(EVT_CODE_FLASH_HEADER_ERR)   
#define EVT_ID_FLASH_DATA_ERR                          _CLASS_FLASH(EVT_CODE_FLASH_DATA_ERR)     
#define EVT_ID_FLASH_DATA_AND_HEADER_ERR               _CLASS_FLASH(EVT_CODE_FLASH_DATA_AND_HEADER_ERR)

// Event code for EVT_CLASS_CACHE(Cache)
//

#define _CLASS_CACHE(x)                (EVT_CLASS_CACHE << 16 | (x))

#define EVT_ID_CACHE_NOT_RECV_FROM_TBBU            _CLASS_CACHE(EVT_CODE_CACHE_NOT_RECV_FROM_TBBU)
#define EVT_ID_CACHE_RECVD_FROM_TBBU               _CLASS_CACHE(EVT_CODE_CACHE_RECVD_FROM_TBBU)
#define EVT_ID_CACHE_CTRLER_CACHE_DISCARDED        _CLASS_CACHE(EVT_CODE_CACHE_CTRLER_CACHE_DISCARDED)
#define EVT_ID_CACHE_FAIL_RECV_DUETO_MISMATCH      _CLASS_CACHE(EVT_CODE_CACHE_FAIL_RECV_DUETO_MISMATCH)
#define EVT_ID_CACHE_DIRTY_DATA_DISCARDED          _CLASS_CACHE(EVT_CODE_CACHE_DIRTY_DATA_DISCARDED)
#define EVT_ID_CACHE_FLUSH_RATE_CHANGED            _CLASS_CACHE(EVT_CODE_CACHE_FLUSH_RATE_CHANGED)


//
// Event code for EVT_CLASS_MISC
//

#define _CLASS_MISC(x)                (EVT_CLASS_MISC << 16 | (x))

#define EVT_ID_MISC_CONFIG_CLEARED                   _CLASS_MISC(EVT_CODE_MISC_CONFIG_CLEARED)        
#define EVT_ID_MISC_CHANGE_BACK_ACTIVITY_RATE        _CLASS_MISC(EVT_CODE_MISC_CHANGE_BACK_ACTIVITY_RATE)
#define EVT_ID_MISC_FATAL_FW_ERR                     _CLASS_MISC(EVT_CODE_MISC_FATAL_FW_ERR)
#define EVT_ID_MISC_FACTORY_DEFAULTS_RESTORED        _CLASS_MISC(EVT_CODE_MISC_FACTORY_DEFAULTS_RESTORED)
#define EVT_ID_MISC_GET_HIBER_CMD                    _CLASS_MISC(EVT_CODE_MISC_GET_HIBER_CMD)
#define EVT_ID_MISC_MUTLI_BIT_ECC_ERR                _CLASS_MISC(EVT_CODE_MISC_MUTLI_BIT_ECC_ERR)
#define EVT_ID_MISC_SINGLE_BIT_ECC_ERR               _CLASS_MISC(EVT_CODE_MISC_SINGLE_BIT_ECC_ERR)
#define EVT_ID_MISC_GET_SHUTDOWN_CMD                 _CLASS_MISC(EVT_CODE_MISC_GET_SHUTDOWN_CMD)
#define EVT_ID_MISC_TIME_ESTABLISHED                 _CLASS_MISC(EVT_CODE_MISC_TIME_ESTABLISHED)
#define EVT_ID_MISC_USER_ENTERED_DEBUGGER            _CLASS_MISC(EVT_CODE_MISC_USER_ENTERED_DEBUGGER)
#define EVT_ID_MISC_FORMAT_COMPLETE                  _CLASS_MISC(EVT_CODE_MISC_FORMAT_COMPLETE)
#define EVT_ID_MISC_FORMAT_STARTED                   _CLASS_MISC(EVT_CODE_MISC_FORMAT_STARTED)
#define EVT_ID_MISC_REASSIGN_WRITE_OP                _CLASS_MISC(EVT_CODE_MISC_REASSIGN_WRITE_OP)
#define EVT_ID_MISC_UNEXPECTED_SENSE                 _CLASS_MISC(EVT_CODE_MISC_UNEXPECTED_SENSE)
#define EVT_ID_MISC_REPLACED_MISSING                 _CLASS_MISC(EVT_CODE_MISC_REPLACED_MISSING)
#define EVT_ID_MISC_NOT_A_CERTIFIED_DRIVE            _CLASS_MISC(EVT_CODE_MISC_NOT_A_CERTIFIED_DRIVE)

/* May put into other group???*/
#define EVT_ID_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT   _CLASS_MISC(EVT_CODE_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT)
#define EVT_ID_MISC_VD_MISSING_DRIVES                _CLASS_MISC(EVT_CODE_MISC_VD_MISSING_DRIVES)
#define EVT_ID_MISC_VD_MISSING_AT_BOOT               _CLASS_MISC(EVT_CODE_MISC_VD_MISSING_AT_BOOT)
#define EVT_ID_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT  _CLASS_MISC(EVT_CODE_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT)
#define EVT_ID_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD     _CLASS_MISC(EVT_CODE_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD)
#define EVT_ID_MISC_PD_TOO_SMALL_FOR_COPYBACK        _CLASS_MISC(EVT_CODE_MISC_PD_TOO_SMALL_FOR_COPYBACK)

#define EVT_ID_MISC_CHANGE_SYNC_RATE                 _CLASS_MISC(EVT_CODE_MISC_CHANGE_SYNC_RATE)
#define EVT_ID_MISC_CHANGE_INIT_RATE                 _CLASS_MISC(EVT_CODE_MISC_CHANGE_INIT_RATE)
#define EVT_ID_MISC_CHANGE_REBUILD_RATE              _CLASS_MISC(EVT_CODE_MISC_CHANGE_REBUILD_RATE)
#define EVT_ID_MISC_CHANGE_MIGRATION_RATE            _CLASS_MISC(EVT_CODE_MISC_CHANGE_MIGRATION_RATE)
#define EVT_ID_MISC_CHANGE_COPYBACK_RATE             _CLASS_MISC(EVT_CODE_MISC_CHANGE_COPYBACK_RATE)
#define EVT_ID_MISC_CHANGE_MP_RATE                   _CLASS_MISC(EVT_CODE_MISC_CHANGE_MP_RATE)
#define EVT_ID_MISC_ALARM_MUTE                       _CLASS_MISC(EVT_CODE_MISC_ALARM_MUTE)

#ifndef _MARVELL_SDK_PACKAGE_NONRAID
//
// Event code for EVT_CLASS_ARRAY
//

#define _CLASS_ARRAY(x)                (EVT_CLASS_ARRAY << 16 | (x))

#define EVT_ID_ARRAY_CREATE                    _CLASS_ARRAY(EVT_CODE_ARRAY_CREATE)        
#define EVT_ID_ARRAY_DELETE                    _CLASS_ARRAY(EVT_CODE_ARRAY_DELETE)

#define EVT_ID_ADAPTER_TEMP_OPTIMAL            _CLASS_OEM(EVT_CODE_ADAPTER_TEMP_OPTIMAL)
#define EVT_ID_ADAPTER_TEMP_WARN               _CLASS_OEM(EVT_CODE_ADAPTER_TEMP_WARN)
#define EVT_ID_ADAPTER_TEMP_CRITICAL           _CLASS_OEM(EVT_CODE_ADAPTER_TEMP_CRITICAL)
#define EVT_ID_ADAPTER_FAN_OPTIMAL             _CLASS_OEM(EVT_CODE_ADAPTER_FAN_OPTIMAL)
#define EVT_ID_ADAPTER_FAN_FAILURE             _CLASS_OEM(EVT_CODE_ADAPTER_FAN_FAILURE)
#define EVT_ID_ADAPTER_FAN_ABSENT              _CLASS_OEM(EVT_CODE_ADAPTER_FAN_ABSENT)
#define EVT_ID_ADAPTER_PSU_ALERT               _CLASS_OEM(EVT_CODE_ADAPTER_PSU_ALERT)
#define EVT_ID_ADAPTER_PSU_OPTIMAL             _CLASS_OEM(EVT_CODE_ADAPTER_PSU_OPTIMAL)
#define EVT_ID_ADAPTER_MUTE_PRESS              _CLASS_OEM(EVT_CODE_ADAPTER_MUTE_PRESS)
#define EVT_ID_ADAPTER_IDENTFICATION_PRESS     _CLASS_OEM(EVT_CODE_ADAPTER_IDENTFICATION_PRESS)
#define EVT_ID_ADAPTER_IDENTFICATION_ON        _CLASS_OEM(EVT_CODE_ADAPTER_IDENTFICATION_ON)
#define EVT_ID_ADAPTER_IDENTFICATION_OFF       _CLASS_OEM(EVT_CODE_ADAPTER_IDENTFICATION_OFF)
#define EVT_ID_ADAPTER_INTRUSION_OPEN          _CLASS_OEM(EVT_CODE_ADAPTER_INTRUSION_OPEN)
#define EVT_ID_ADAPTER_INTRUSION_CLOSE         _CLASS_OEM(EVT_CODE_ADAPTER_INTRUSION_CLOSE)
#define EVT_ID_ADAPTER_STATUS_LOW_POWER        _CLASS_OEM(EVT_CODE_ADAPTER_STATUS_LOW_POWER)
#define EVT_ID_ADAPTER_STATUS_IDLE             _CLASS_OEM(EVT_CODE_ADAPTER_STATUS_IDLE)

#endif
// _MARVELL_SDK_PACKAGE_NONRAID

#endif


