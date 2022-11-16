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

#include "mv_incs.h"
#include "mvf_hal.h"
#include "mvf_apif.h"
#include "proj_dep.h"
#include "mvf_loki.h"

//#define _USE_2_WIN_			1

#include "mvf_lkhw.c"
#if defined( _MVFLASH_ )
#undef MV_REG_R8
#undef MV_REG_W8
#undef MV_REG_R16
#undef MV_REG_W16
#undef MV_REG_R32
#undef MV_REG_W32
#if defined( _WIN_REMAP_INLINE_ )
#define MV_REG_R8( a, o)     ((MV_RC8XXX_WinRemapper( pAI, (MV_U32)a )|1)?MV_MMR8( (MV_U32)a, RC8XXX_BAR2_ALIGN(o) ):0)
#define MV_REG_W8( a, o, d)  do{ MV_RC8XXX_WinRemapper( pAI, (MV_U32)a );MV_MMW8( (MV_U32)a, RC8XXX_BAR2_ALIGN(o), d ); }while( 0 );
#define MV_REG_R32( a, o)    ((MV_RC8XXX_WinRemapper( pAI, (MV_U32)a )|1)?MV_MMR32( (MV_U32)a, RC8XXX_BAR2_ALIGN(o) ):0)
#define MV_REG_W32( a, o, d) do{ MV_RC8XXX_WinRemapper( pAI, (MV_U32)a );MV_MMW32( (MV_U32)a, RC8XXX_BAR2_ALIGN(o), d ); }while( 0 );
#else
#if defined(SDT_TYPE_FREY)
#define MV_REG_R8( a, o)     MV_RC9XXX_MEMR8(pAI, a, o)
#define MV_REG_W8( a, o, d)  MV_RC9XXX_MEMW8(pAI, a, o, d)
#define MV_REG_R32( a, o)    MV_RC9XXX_MEMR32(pAI, a, o)
#define MV_REG_W32( a, o, d) MV_RC9XXX_MEMW32(pAI, a, o, d)
#else
#define MV_REG_R8( a, o)     MV_RC8XXX_MEMR8( pAI, a, o )
#define MV_REG_W8( a, o, d)  MV_RC8XXX_MEMW8( pAI, a, o, d )
#define MV_REG_R32( a, o)    MV_RC8XXX_MEMR32( pAI, a, o )
#define MV_REG_W32( a, o, d) MV_RC8XXX_MEMW32( pAI, a, o, d )
#endif
#endif
#endif

#if defined( _MVF_DBG_ )
#define LKF_DBG_LV1( X )  X
#else
#define LKF_DBG_LV1( X )
#endif

#define MVF_GetDeviceDecodeInfo    RC8XXX_GetDeviceDecodeInfo

#if 0
#define MFP_SIZE                    sizeof (MVFlashProperty)
#define MFP_CMD_TYPE_INTEL          0x89
#define MFP_CMD_TYPE_AMD            0x01
typedef struct MVFlashProperty_
{
    char    mfp_size;
    char    type;
    char    id_count;
    char    id[15];
    unsigned long size;
    long    sect_size;  
    char    hw_wr_size;
    char    bus_width;
}MVFlashProperty;

static const
MVFlashProperty MVFlashPropertyList[] =
{
    {/*intel 0x8918 16M*/
        MFP_SIZE, MFP_CMD_TYPE_INTEL, 
        2, { 0x89, 0x18, 0x00,  },
        UniT (16L, 'M'), UniT (128L, 'K'), 32, 1
    },
    {/*intel 0x8918 16M*/
        MFP_SIZE, MFP_CMD_TYPE_INTEL, 
        2, { 0x89, 0x1D, 0x00,  },
        UniT (32L, 'M'), UniT (128L, 'K'), 32, 1
    },
    { 0, },
};

/*
    pFI->ID -> Id to search
    pFI->bus_width -> size of id
*/
int mvf_api_flash_propery_assign_eq (MVFlashProperty *pFP, __IO__ FlashInfo *pFI)
{
int i, j;
char *id, id1[sizeof( pFI->ID )] = { 0, };

    id = pFP->id;
    for (i=0, j=(pFP->id_count-1); j>=0; i+=8, j--)
        id1[j] = MV_U32_GET_BITS(pFI->ID, i, 8);
    if (id[0]!=id1[0])
        return -1;
    j = pFP->id_count-1;
    if (j>0) {
        for ( i=1;i<sizeof( pFP->id); i+= j )
            if (0==memcmp( id+i, id1, j )) {
                j = 0;
                break;
            }    
    }        
    if (j>0)
        mvf_printf ("manufacture id is supported but device id not in list.!!\n");
    pFI->DecodeAddress = 0;
    pFI->Size     = pFP->size;
    pFI->SectSize = pFP->sect_size;
    pFI->WriteBufSize = pFP->hw_wr_size;
    pFI->bus_width = pFP->bus_width;
}
#endif
//_MARVELL_SDK_PACKAGE_NONRAID

/**********************************************************************************/
#ifdef SUPPORT_AMD
static inline void
MVF_AmdReset
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    MV_REG_W8( pAI->FlashBar, pFI->DevBase, 0xF0 );
}

static int
MVF_AmdPollSR
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout
)
{
    MV_U8   sr;
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;

    timeout/=10;
    addr+= DevBase;
    do {
        sr = MV_REG_R8( base_addr, addr );
        if( ( sr & mask )==bit )
            break;
        if( sr & MV_BIT(5) )  {
            sr = MV_REG_R8( base_addr, addr );
            if( ( sr & mask )!=bit ) {
                LKF_DBG_LV1( ( "%d\n", __LINE__ ) );
                break;
            }    
        }
        DelayUSec( 10 );
    } while(timeout-->0);  
    MVF_AmdReset(pAI, pFI);
    if (timeout>0)
        return 0;
    LKF_DBG_LV1( ( "%d\n", __LINE__ ) );
    return -1;
}

#define ST_OFF( a )             a
static MV_INLINE int
MVF_AmdSectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;

    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x555 ), 0xAA );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x2AA ), 0x55 );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x555 ), 0x80 );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x555 ), 0xAA );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x2AA ), 0x55 );
    
    blkaddr = Flash_GetSectAddr( pFI, DevBase + blkaddr );
    MV_REG_W8( base_addr, blkaddr, 0x30 );
    if( 0 != MVF_AmdPollSR( pAI, pFI, blkaddr, 0x80, 0x80, 300000 ) )
        return -1;
    return 0;
}

static int
MVF_AmdWriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    MV_U8       *data,
    MV_U32      count
)
{
    MV_U8   tmp;
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;

    addr+= DevBase;
    count+= addr;
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x555 ), 0xAA );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x2AA ), 0x55 );
    MV_REG_W8( base_addr, DevBase + ST_OFF( 0x555 ), 0x20 );
    while( addr<count) {
        tmp = *data++;
        if( tmp!=0xFF ) {
            MV_REG_W8( base_addr, addr, 0xA0 );
            MV_REG_W8( base_addr, addr, tmp );
            if( -1 == MVF_AmdPollSR( pAI, pFI, addr, 0xFF, tmp, 30000 ) ) {
                return -1;
            }
        }        
        addr++;
    }
    MV_REG_W8( base_addr, DevBase, 0x90 );
    MV_REG_W8( base_addr, DevBase, 0x00 );
    return 0;
}

static int
MVF_AmdIdentify
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    MV_U16  ID;
    MV_U32  dwTmp;
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;

    MVF_AmdReset( pAI, pFI );
    MV_REG_W8( base_addr, DevBase+ST_OFF( 0x555 ), 0xAA );
    MV_REG_W8( base_addr, DevBase+ST_OFF( 0x2AA ), 0x55 );
    MV_REG_W8( base_addr, DevBase+ST_OFF( 0x555 ), 0x90 );
    dwTmp = MV_REG_R8( base_addr, DevBase );
    ID = ( dwTmp&0xFF );
    dwTmp = MV_REG_R8( base_addr, DevBase+1 );
    ID = ( ID << 8 )|( dwTmp&0xFF );

    pFI->ID = ID;
    switch( ID )
    {
        case 0x014F:
        case 0x20E3: /*st m29w040b */
            pFI->Size = (MV_U32)UniT (512L, 'K');
            pFI->SectSize = (MV_U32)UniT (64L, 'K');
            /* this write buf is software simulated */
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            break;
        case 0x205E: /*st M29DW323DT */
            pFI->Size = (MV_U32)UniT (4L, 'M');
            pFI->SectSize = (MV_U32)UniT (64L, 'K');
            /* this write buf is software simulated */
            pFI->WriteBufSize = 32;
            pFI->bus_width = 2;
            break;
        case 0x205F: /*st M29DW323DB */
            pFI->Size = (MV_U32)UniT (4L, 'M');
            pFI->SectSize = (MV_U32)UniT (64L, 'K');
            /* this write buf is software simulated */
            pFI->WriteBufSize = 32;
            pFI->bus_width = 2;
            break;
        default:
            ID = 0;
            break;
    }            
    MVF_AmdReset( pAI, pFI );
    if (ID)
        return 0;
    return -1;
}
#endif

#ifdef SUPPORT_SAMSUNG
#if 1
static int
MVF_SamsungEraseResumeStatus
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
    MV_U32 delay = 30000;
    MV_U32 sr;
    MV_U16 step = 0;
     /*Reset Flash*/
    do {
	if (step == 1) {
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC944, 0xFFFFFFFF);

            MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC910, blkaddr >> 1);
            MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x030);
            MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        }
        sr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0xC944);
        if((sr & MV_BIT(0)) && (sr != 0xFFFFFFFFL)) {
            sr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0xC954);
            if((sr & MV_BIT(17)) && (sr != 0xFFFFFFFFL)) {
                  if (step <= 2) {
//                      FM_PRINT("\nreturn <<0xC954>>=0x%lx, %ld", sr, delay);
//                      if ((sr & 0x80) == 0x80) {
                          break;
//                      }
                  }
            }
        }
//        FM_PRINT("\n<<0xC954>>=0x%lx, %ld", sr, delay);
        DelayMSec(10);
    } while(delay-- > 0);
    DelayMSec(10);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC944, 0xFFFFFFFF);

    if (delay > 0)
        return 0;
    return -1;
}
#endif

static int
MVF_SamsungPollSR
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout
)
{
    MV_U32   sr;
//    void    *base_addr = pAI->FlashBar;
//    MV_U32  DevBase = pFI->DevBase;

//    DelayMSec(100);
    timeout /= 10;
    do {
        sr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0xC954);
        if((sr & MV_BIT(17)) && (sr != 0xFFFFFFFFL)) {
            sr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0xC944);
            if((sr & MV_BIT(0)) && (sr != 0xFFFFFFFFL)) {
                 break;
            }
        }
        DelayUSec(10);
    } while(timeout--> 0);  
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC944, 0xFFFFFFFF);
//    MVF_SamsungReset(pAI, pFI);

    if (timeout > 0)
        return 0;
//    LKF_DBG_LV1( ( "%d\n", __LINE__ ) );
    return -1;
}

static inline void
MVF_SamsungReset
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    /*Reset Flash*/
    MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x000);
    MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0F0);
    MV_MMW32( pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
    if(-1 == MVF_SamsungPollSR( pAI, pFI, 0, 0x00, 0, 30000)) {
           return;
    }

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C910, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C914, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C918, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C928, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C92C, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C930, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C91C, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C920, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C924, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C934, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C938, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C93C, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C940, 0x000L);
}

//#define ST_OFF( a )           (((a)<<1)>>pFI->bus_width)
#define ST_OFF( a )             a
static MV_INLINE int
MVF_SamsungSectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
#if 0
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x2AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x055L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x080L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x2AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x055L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
        printf("\nblkaddr=0x%lx 0x%lx", blkaddr, blkaddr >> 1);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, blkaddr >> 1);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 30);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
        if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
            return -1;
        }
#else
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C910, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C914, 0x2AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C918, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C928, 0x555L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C92C, 0x2AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C930, blkaddr >> 1);
    
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C91C, 0x0AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C920, 0x055L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C924, 0x080L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C934, 0x0AAL);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C938, 0x055L);
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C93C, 0x030L);
    
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C940, 0x06010000L);
#if 0
        if(-1 == MVF_SamsungPollSR( pAI, pFI, blkaddr, 0xFF, 0, 30000)) {
               return -1;
        }
#else
        if(-1 == MVF_SamsungEraseResumeStatus( pAI, pFI, blkaddr)) {
               return -1;
        }
#endif
#endif
    DelayMSec(50);

    MVF_SamsungReset(pAI, pFI);
    return 0;
}

static int
MVF_SamsungWriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    MV_U8       *data,
    MV_U32      count
)
{
    MV_U16   tmp16, buf16;
//    void    *base_addr = pAI->FlashBar;
//    MV_U32  DevBase = pFI->DevBase;

//    addr+= (MV_U32)base_addr;
    count += addr;
#if 1
    while(addr < count) {
        tmp16 = *(MV_PU16)data;
        if(tmp16 != 0xFFFF) {
#if 1
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0AAL);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
            if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
                return -1;
            }
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x2AAL);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x055L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
            if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
                return -1;
            }
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0A0L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
            if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
                return -1;
            }
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, addr >> 1);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, tmp16);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x01010000);
            if( -1 == MVF_SamsungPollSR( pAI, pFI, addr, 0x00, 0x00, 30000)) {
                return -1;
            }
            pAI->Flash_ReadBuf( pAI, pFI, addr, &buf16, 2);
	     if (buf16 != tmp16) {
//                mvf_printf("\naddr=0x%lx, data0=0x%x, data1=0x%x", addr, buf16, tmp16);
                MV_DPRINT(("\naddr=0x%lx, data0=0x%x, data1=0x%x", addr, buf16, tmp16));
                return -1;
	     }
#else
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0x555L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC914, 0x2AAL);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC918, 0x555L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC934, addr);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC92C, 0x000L); 
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC930, 0x000L);

            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0AAL);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC920, 0x055L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC924, 0x0A0L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC928, tmp16);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC938, 0x000L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC93C, 0x000L);
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC940, 0x04010000);
            printf("\naddr=0x%lx data=0x%x", addr, tmp16);
            DelayMSec( 100 );
            if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
                return -1;
            }
#endif
        }        
        data += 2;
        addr += 2;
    }
#else
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8420, 0x00001000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8424, (MV_U32)base_addr & 0x000fffff);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, 0xFC000001L);
    win_size = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0x8420) << 1; 
    remap_addr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0x8428) & 0xFFFFF000;
    printf("\npAI->FlashBar=0x%lx, pFI->DevBase=0x%lx", (MV_U32)pAI->FlashBar, pFI->DevBase);
    Count += Addr;
    printf("\naddr=0x%lx, count=0x%lx", Addr, Count);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC910, 0xAAAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC914, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC918, 0xAAAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC928, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC92C, 0x2AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC930, 0x555L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC91C, 0x0AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC920, 0x055L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC924, 0x0A0L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC934, 0x0AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC938, 0x055L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xC93C, 0x0A0L);
    while(addr < count) {
        tmp32 = *(MV_PU32)data;
        if(tmp32 != 0xFFFFFFFF) {
            MV_MMW32(base_addr, (addr & (win_size - 1)), tmp32);
            printf("\n0x%lx, 0x%lx", (addr & (win_size - 1)), remap_addr + (addr & ~(win_size - 1)));
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, remap_addr + (addr & ~(win_size - 1))  | MV_BIT(0)); /*Switch Remp Win address*/
            if( -1 == MVF_SamsungPollSR( pAI, pFI, 0x00, 0x00, 0x00, 30000)) {
                return -1;
            }
        }        
        data += 4;
        addr += 4;
    }
#endif
//    DelayMSec(500);

    MVF_SamsungReset(pAI, pFI);
    return 0;
}

static int
MVF_MX29LV640_SectorBase
(
    AdapterInfo *pAI,
    FlashInfo *pFI,
    MV_U32 addr
){
    MV_U32 block_id = addr >> (12 + 1); 
    switch (block_id) {
        case 0x000: case 0x001: case 0x002: case 0x003: case 0x004: case 0x005: case 0x006: case 0x007:/*Block 0 ~ 7:*/
            return block_id;
        default:
            block_id >>= 3;
            return (block_id + 7);
    }
}

static int
MVF_MX29LV640_Identify
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    MV_U32  dwTmp;
    void    *base_addr = pAI->FlashBar;

#if defined( _MVFLASH_ )
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8420, 0x00001000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8424, (MV_U32)base_addr & 0x000fffff);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, 0xFC000001L);
#endif
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C910, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C914, 0x2AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C918, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C928, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C92C, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C930, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C91C, 0x0AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C920, 0x055L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C924, 0x090L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C934, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C938, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C93C, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C940, 0x03010000L);
    if(-1 == MVF_SamsungPollSR( pAI, pFI, 0, 0x00, 0, 30000)) {
           return -1;
    }

    dwTmp = MV_MMR32((MV_U32)base_addr, 0x0000);
    pFI->ID = dwTmp;
    switch(pFI->ID) {
        case 0x22C900C2: /*MX29LV640*/
        case 0x22CB00C2: /*MX29LV640*/
            pFI->Size = (MV_U32)UniT(8L, 'M');
            pFI->SectSize = (MV_U32)UniT(8L, 'K');
            /* this write buf is software simulated */
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            FM_PRINT("\nThis is MXIC Flash(MX29LV640)(0x%lx).\n", pFI->ID);
            break;
       default:
            FM_PRINT("\n(0x%lx)\n", pFI->ID);
            pFI->ID = 0x00000000;
            break;
    }            
    MVF_SamsungReset( pAI, pFI );
    if (pFI->ID)
        return 0;
    return -1;
}

static int
MVF_Samsung_GetBlkBase
(
    AdapterInfo *pAI,
    FlashInfo *pFI,
    MV_U32 addr
){
    MV_U32 block_id = addr >> (12 + 1); 
    switch (block_id) {
        case 0x000: case 0x001: case 0x002: case 0x003: case 0x004: case 0x005: case 0x006: case 0x007:/*Block 0 ~ 7:*/
            return block_id;
        case 0x1F8: case 0x1F9: case 0x1FA: case 0x1FB: case 0x1FC: case 0x1FD: case 0x1FE: case 0x1FF:/*Block 70 ~ 77:*/
            return ((block_id & 0x07) + 0x070);
        default:
            block_id >>= 3;
            return (block_id + 7);
    }
}

static int
MVF_SamsungIdentify
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    MV_U32  dwTmp;
    void    *base_addr = pAI->FlashBar;
//    MV_U32  DevBase = pFI->DevBase;

#if defined( _MVFLASH_ )
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8420, 0x00001000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8424, (MV_U32)base_addr & 0x000fffff);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, 0xFC000001L);
#endif
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C910, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C914, 0x2AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C918, 0x555L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C928, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C92C, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C930, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C91C, 0x0AAL);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C920, 0x055L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C924, 0x090L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C934, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C938, 0x000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C93C, 0x000L);

    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x0000C940, 0x03010000L);
    if(-1 == MVF_SamsungPollSR( pAI, pFI, 0, 0x00, 0, 30000)) {
           return -1;
    }

    dwTmp = MV_MMR32((MV_U32)base_addr, 0x0000);
//    mvf_printf("\n0x%lx", dwTmp);
//    mvf_printf("\nID 0x%lx", ID);
    pFI->ID = dwTmp;
    switch(pFI->ID) {
        case 0x257E00EC: /*K8P3215UQB*/
            pFI->Size = (MV_U32)UniT(4L, 'M');
            pFI->SectSize = (MV_U32)UniT(4L, 'K');
            /* this write buf is software simulated */
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            FM_PRINT("\nThis is Samsung Flash(K8P3215UQB)(0x%lx).\n", pFI->ID);
            break;
       default:
            FM_PRINT("\n(0x%lx)\n", pFI->ID);
            pFI->ID = 0x00000000;
           break;
    }            
    MVF_SamsungReset( pAI, pFI );
    if (pFI->ID)
        return 0;
    return -1;
}
#endif /*SUPPORT_SAMSUNG*/

/**********************************************************************************/
static inline void
MVF_IntelReset
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32		addr
)
{
    MV_REG_W8( pAI->FlashBar, addr, 0xFF );
}

static inline void
MVF_IntelClrSts
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32		addr
)
{
    MV_REG_W8( pAI->FlashBar, addr, 0x50 );
}

#define MVF_SR_LOCK		MV_BIT(1)
static int
MVF_IntelPollSR
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32       flags,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout,
    MV_U32		cur_adr
)
{
    MV_U8   sr;
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;
    int		state = 0;

   	DevBase = cur_adr;

    if( flags & 1 )
    {
        MV_REG_W8( base_addr, DevBase, 0x70 );
    }   
    /* do not remove this delay */
    DelayUSec( 30 );

    mask&= 0x7F;
    bit&= 0x7F;
    timeout = timeout/10+1;
    do
    {
        sr = MV_REG_R8( base_addr, DevBase );
        if ( ( sr&0x80 ) ) {
	    	if (state==0) {
	    		state = 1;
		    }	
	        if (sr&0x7E) {
	        	char *str;
	            mvf_printf ("Error: Addr %8.8lX, Status: %2.2X!!\n", DevBase, sr );

				if (sr&MV_BIT(6)) {
		            mvf_printf ( "Erase suspending," );
				}
				str = NULL;
				switch((sr>>4)&3) {
					case 1:
						str = "Program error,";break;
					case 2:
						str = "Erase error,";break;
					case 3:
						str = "Cmd Sequence error,";break;
				}
				if (str) {
		            mvf_printf ( str );
				}
				if (sr&MV_BIT(3)) {
		            mvf_printf ( "!!!!!!Vpen error, error, error......!!!!!!\a\a\a, please contact hardware engineer," );
				}
				if (sr&MV_BIT(2)) {
		            mvf_printf ( "Program suspending," );
				}
				if (sr&MVF_SR_LOCK) {
		            mvf_printf ( "Block locked," );
				}
	            MVF_IntelClrSts(pAI, pFI, DevBase);
	            return sr;
	        }    
	    	if (state==1) {
	            if( ( sr & mask )==( bit & mask ) )
	            	return 0;
		    }	
		}    
    }while( (--timeout)>0 );

	if (flags&1)
    	MVF_IntelReset( pAI, pFI, cur_adr );
    if (timeout)
        return 0;
    return -1;
}

#define MVF_IntelWaitReady( ai, fi, f, to, adr ) MVF_IntelPollSR(ai, fi, f, 0x80, 0x80, to, adr )

static int MV_INLINE
MVF_IntelLock
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr,
    MV_U32      count,
    int         op    
)
{
    int ret_val=0;
    blkaddr+= pFI->DevBase;
    count+= blkaddr;
    blkaddr = Flash_GetSectAddr (pFI, blkaddr);
    if (op==1) {
        op = 0x01;
        mvf_printf( "Lock block %8.8lx\n!!", blkaddr );
    }    
    else {
        op = 0xD0;
        mvf_printf( "Unlock block %8.8lx!!\n", blkaddr );
    }    
    while (blkaddr<count) {
        MV_REG_W8( pAI->FlashBar, blkaddr, 0x60 );
        MV_REG_W8( pAI->FlashBar, blkaddr, op );
        ret_val = MVF_IntelWaitReady( pAI, pFI, 0, 7000000, blkaddr );
        if (ret_val!=0)
        	return -1;
        blkaddr+= pFI->SectSize;
    }    
    MVF_IntelReset( pAI, pFI, blkaddr );
    return 0;
}

static int MV_INLINE
MVF_IntelSectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
    unsigned int ret_val;
    unsigned int flag;

    if (blkaddr>=pFI->Size)
    	return -1;
    blkaddr = MVF_ALIGN(blkaddr, pFI->SectSize);
    blkaddr+= pFI->DevBase;
    flag = 0xDEAD;    
    while (flag) {    
	    MV_REG_W8( pAI->FlashBar, blkaddr, 0x20 );
	    MV_REG_W8( pAI->FlashBar, blkaddr, 0xD0 );
    	ret_val = MVF_IntelWaitReady( pAI, pFI, 0, 5000000, blkaddr );
	    if (flag==0xDEAD) {
	    	if (ret_val&MVF_SR_LOCK) {
		        if (MVF_IntelLock(pAI, pFI, blkaddr, 1, 0));
	    	}
	    	flag = 0xBEEF;
	    } else if(flag==0xBEEF) {
	    	flag = 0;
	    }	
	}    
    MVF_IntelReset( pAI, pFI, blkaddr );
    return ret_val;
}

static int
MVF_IntelWriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    MV_U8       *data,
    MV_U32      count
)
{
    long    i;
    int     bufCount, bufSize = 1;
    void    *base_addr = pAI->FlashBar;

    addr+= pFI->DevBase;
    bufSize = pFI->WriteBufSize;
    bufCount = MIN(bufSize - MOD2(addr, bufSize), count);
    while(count > 0) {
#if !defined( _HW_BUF_WRITE_DISABLED_ )
        for(i = 0; i < bufCount; i++) {
            if(data[i] != 0xFF)
                break;
        }        
        if(i != bufCount) {
            for( i=50000L; i>0; i-- ) {
                MV_REG_W8( base_addr, addr, 0xE8 );
                if( 0 == MVF_IntelWaitReady( pAI, pFI, 0, 10, addr ) )
                    break;
            }
            if( i==0 )
                break;
            MV_REG_W8( base_addr, addr, bufCount-1 );
            for( i=0; i<bufCount; i++ )
                MV_REG_W8( base_addr, addr+i, data[i] );
            MV_REG_W8( base_addr, addr, 0xD0 );
            if( 0 != MVF_IntelWaitReady( pAI, pFI, 0, 2400000, addr ) )
                break;
        }    
#else
        if( ( i = *data )!=0xFF ) {
            MV_REG_W8( base_addr, addr, 0x40 );
            MV_REG_W8( base_addr, addr, i );
        }    
        if( 0 != MVF_IntelWaitReady( pAI, pFI, 0, 630, addr ) )
            break;
#endif
        count -= bufCount;
        addr += bufCount;
        data += bufCount;
        bufCount = MIN(bufSize, count);
    }
    MVF_IntelReset (pAI, pFI, addr - bufCount);
    if (count > 0)
        return -1;
    return 0;    
}

static int
MVF_IntelIdentify
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    MV_U16  ID;
    MV_U32  dwTmp;
    void    *base_addr = pAI->FlashBar;
    MV_U32  DevBase = pFI->DevBase;

    MVF_IntelClrSts( pAI, pFI, DevBase );
    MV_REG_W8( base_addr, DevBase, 0x90 );
    dwTmp = MV_REG_R8( base_addr, DevBase );
    ID = ( dwTmp&0xFF );
    dwTmp = MV_REG_R8( base_addr, DevBase+2 );
    ID = ( ID << 8 )|( dwTmp&0xFF );

    pFI->pAI = pAI;
    pFI->ID = ID;
    switch( ID )
    {
        case 0x8916:
            pFI->Size = 4L * 1024 * 1024;
            pFI->SectSize = 128L * 1024;
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            break;
        case 0x8917:
            pFI->Size = 8L * 1024 * 1024;
            pFI->SectSize = 128L * 1024;
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            break;
        case 0x8918:
            pFI->Size = 16L * 1024 * 1024;
            pFI->SectSize = 128L * 1024;
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            break;
        case 0x891D:
            pFI->Size = 32L * 1024 * 1024;
            pFI->SectSize = 128L * 1024;
            pFI->WriteBufSize = 32;
            pFI->bus_width = 1;
            break;
        default:
            pFI->pAI = NULL;
            ID = 0;
            break;
    }            
    MVF_IntelReset( pAI, pFI, DevBase );
    if (ID)
        return 0;
    return -1;    
}

static int
MVF_Shutdown
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    if (pFI==NULL) {
        MVF_PAR_DEV_HW_DEP_SHUTDOWN(pAI, NULL);
        return 0;
    }    
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
#ifdef SUPPORT_AMD
    if( !MVF_DEV_IS_INTEL( pFI ) )  {
        MVF_AmdReset(pAI, pFI);
        return 0;
    } 
#endif    
    MVF_IntelReset(pAI, pFI, pFI->DevBase);
    return 0;
}


static int
MVF_SectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
#ifdef SUPPORT_AMD
    if( !MVF_DEV_IS_INTEL( pFI ) )
        return MVF_AmdSectErase ( pAI, pFI, blkaddr );
#endif        
    return MVF_IntelSectErase ( pAI, pFI, blkaddr );
}

static int
MVF_ReadBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *Data,
    MV_U32      Count
)
{
	MV_PTR_X	p;
    MV_U32		i;

    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    MVF_IntelReset(pAI, pFI, Addr);
    p.p = Data;
	i=((int)Data&3);
	if (i!=(Addr&3)) {
		i = Count;
	} else {
		i = 4-i;
	}
	if (i>Count)
		i = Count;
	Count-= i;
	for ( ; i!=0; i-- )
        *p.pu8++ = MV_REG_R8( pAI->FlashBar, Addr++ );
    if (Count>=4) {
    	i = Count/4;
        for (; i!=0; i--, Addr+= 4 )
            *p.pu32++ = MV_REG_R32( pAI->FlashBar, Addr );
		i = Count&3;	    
    } 
    for ( ;i!=0; i-- ) {
        *p.pu8++ = MV_REG_R8( pAI->FlashBar, Addr++ );
    }    
    LKF_DBG_LV1(( "%s %8.8lX, %ld\r", __FUNCTION__, Addr, Count ));
    return 0;
}

static int
MVF_WriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    void        *data,
    MV_U32      count
)
{
    LKF_DBG_LV1 (( "%s %8.8lX, %ld\r", __FUNCTION__, addr, count ));
    if( ( addr+count )>=pFI->Size )
        count = pFI->Size-addr;
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
#ifdef SUPPORT_AMD
    if( !MVF_DEV_IS_INTEL( pFI ) )
        return MVF_AmdWriteBuf( pAI, pFI, addr, (MV_U8*)data, count );
#endif        
    return MVF_IntelWriteBuf( pAI, pFI, addr, (MV_U8*)data, count );
}

static int
MVF_Init
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
#if defined(SDT_TYPE_FREY)
    return 0;
#else
    if (pFI==NULL) {
        MVF_PAR_DEV_HW_DEP_INIT(pAI, pFI);
        return 0;
    }    
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    
    pFI->ID = 0;
    pFI->Size = 0;
    
    do{
        if( 0==MVF_IntelIdentify( pAI, pFI ) )
            break;
#ifdef SUPPORT_AMD
        if( 0==MVF_AmdIdentify( pAI, pFI ) )
            break;
#endif            
        return -1;
    }while(0);
    MVF_GetDeviceDecodeInfo(pAI, pFI);
    if (!(pFI->flags&FI_FLAG_MEM_MAPPED)) {
        pAI->Flash_ReadBuf = MVF_ReadBuf;
    }

    return 0;
#endif
}

int
MVF_Ioctl
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    int         code,
    void        *OsContext
)
{ 
#if defined(SDT_TYPE_FREY)
    return 0;
#else
    switch (code) {
    case MVF_IOCTL_CODE_START_UNIT:
    case MVF_IOCTL_CODE_STOP_UNIT:
        MV_RC8XXX_ResetCpu(pAI->RC8XXX_REG_INTER_BASE, (code==MVF_IOCTL_CODE_STOP_UNIT)?MV_BIT(2):0);
        break;
    case MVF_IOCTL_CODE_MOUNT_UNIT:
        MVF_PAR_DEV_ID_SETUP( pAI, pFI );
        break;
    case MVF_IOCTL_CODE_UNMOUNT_UNIT:
        break;
    case MVF_IOCTL_CODE_LOCK_UNIT:
    case MVF_IOCTL_CODE_UNLOCK_UNIT:
    {
        MV_U32 *params = (MV_U32*)OsContext;
        MVF_IntelLock(pAI, pFI, params[0], params[1], code==MVF_IOCTL_CODE_LOCK_UNIT);
        break;
    }    
    default:
        return -1;
    }
    return 0;
#endif
}


int
Flash_MVF_IfReg_mvf_loki
(
    void        *OsContext
)
{
#if defined(SDT_TYPE_FREY)
    return 0;
#else
AdapterInfo *pAI = (AdapterInfo*)OsContext;

    switch( Flash_GetSupportDevType( pAI->devId )->Type )
    {
    #if defined( SDT_TYPE_Loki )
        case SDT_TYPE_Loki:
            pAI->Flash_Init = MVF_Init;
            pAI->Flash_Shutdown = MVF_Shutdown;
            pAI->Flash_WriteBuf = MVF_WriteBuf;
            pAI->Flash_SectErase = MVF_SectErase;
            pAI->Flash_Ioctl = MVF_Ioctl;
            break;
    #endif        
        default:
            pAI->Flash_Init = NULL;
            pAI->Flash_Shutdown = NULL;
            pAI->Flash_WriteBuf = NULL;
            pAI->Flash_SectErase = NULL;
            return -1;
    }
    return 0;
#endif
}    

#if defined(SDT_TYPE_FREY)
static int
MVF_Frey_ReadBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *Data,
    MV_U32      Count
)
{
    MV_U16   tmp16;
    void    *base_addr = pAI->FlashBar;
#if defined( _MVFLASH_ )
    MV_U32 win_size;
    MV_U32 remap_addr;
#endif

    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    MVF_SamsungReset(pAI, pFI);

#if defined( _MVFLASH_ )
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8420, 0x00001000L);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8424, (MV_U32)base_addr & 0x000fffff);
    MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, 0xFC000001L);
#endif

#if defined( _MVFLASH_ )
    win_size = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0x8420) << 1; 
    remap_addr = MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0x8428) & 0xFFFFF000;
#endif
//    printf("\npAI->FlashBar=0x%lx, pFI->DevBase=0x%lx", (MV_U32)pAI->FlashBar, pFI->DevBase);
    Count += Addr;
//    printf("\naddr=0x%lx, count=0x%lx", Addr, Count);

    while(Addr < Count) {
        {
#if defined( _MVFLASH_ )
            MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0x8428, remap_addr + (Addr & ~(win_size - 1))  | MV_BIT(0)); /*Switch Remp Win address*/
            tmp16 = MV_MMR16(base_addr, (Addr & (win_size - 1)));
#else
            tmp16 = MV_MMR16(base_addr, Addr);
#endif
            *(MV_PU16)Data = tmp16;
//            printf("\n0x%lx, 0x%lx", (Addr & (win_size - 1)), remap_addr + (Addr & ~(win_size - 1)));
        }        
        Data = (MV_PU8)Data+2;
        Addr += 2;
    }

    MVF_SamsungReset(pAI, pFI);

    return 0;
}

static int
MVF_Frey_Init
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    if (pFI == NULL) {
        MVF_PAR_DEV_HW_DEP_INIT(pAI, pFI);
        return 0;
    }    
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    
    pFI->ID = 0;
    pFI->Size = 0;
    
    { /*Enable Flash moudle ,Enable 16 bits mode*/
        MV_MMW32(pAI->RC9XXX_REG_INTER_BASE, 0xc950,  MV_MMR32(pAI->RC9XXX_REG_INTER_BASE, 0xc950) | (MV_BIT(1) | MV_BIT(0)));
    }
    do{
//        if( 0==MVF_IntelIdentify( pAI, pFI ) )
//            break;
//#ifdef SUPPORT_AMD
//        if( 0==MVF_AmdIdentify( pAI, pFI ) )
//            break;
//#endif            
#ifdef SUPPORT_SAMSUNG
        if(0 == MVF_SamsungIdentify(pAI, pFI))
            break;
	 else if(0 == MVF_MX29LV640_Identify(pAI, pFI))
           break;
#endif            
        return -1;
    } while (0);

//    MVF_GetDeviceDecodeInfo(pAI, pFI);
//    if (!(pFI->flags&FI_FLAG_MEM_MAPPED)) {
        pAI->Flash_ReadBuf = MVF_Frey_ReadBuf;
//    }

    return 0;
}

static int
MVF_Frey_Shutdown
(
    AdapterInfo *pAI,
    FlashInfo   *pFI
)
{
    if (pFI==NULL) {
        MVF_PAR_DEV_HW_DEP_SHUTDOWN(pAI, NULL);
        return 0;
    }    
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    MVF_SamsungReset(pAI, pFI);
    return 0;
}

static int
MVF_Frey_WriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr,
    void        *data,
    MV_U32      count
)
{
    LKF_DBG_LV1 (( "%s %8.8lX, %ld\r", __FUNCTION__, addr, count ));
    if( ( addr+count )>=pFI->Size )
        count = pFI->Size-addr;
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    return MVF_SamsungWriteBuf(pAI, pFI, addr, (MV_U8 *)data, count);
}

static int
MVF_Frey_SectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      blkaddr
)
{
    MVF_DEV_ACCESS_RIGHT(pAI, pFI);
    MVF_PAR_DEV_ID_SETUP(pAI, pFI);
    return MVF_SamsungSectErase(pAI, pFI, blkaddr);
}

static int
MVF_Frey_GetBlkBase
(
    AdapterInfo *pAI,
    FlashInfo *pFI,
    MV_U32 addr
){
     if (pFI->ID == 0x257E00EC) /*K8P3215UQB*/
         return MVF_Samsung_GetBlkBase(pAI, pFI, addr);
     else
         return MVF_MX29LV640_SectorBase(pAI, pFI, addr);
}

int
flash_mvf_ifreg_mvf_frey
(
    void        *OsContext
)
{
AdapterInfo *pAI = (AdapterInfo*)OsContext;

    switch( Flash_GetSupportDevType( pAI->devId )->Type )
    {
         case SDT_TYPE_FREY:
            pAI->Flash_Init = MVF_Frey_Init;
            pAI->Flash_Shutdown = MVF_Frey_Shutdown;
//            pAI->Flash_ReadBuf = MVF_ReadBuf;
            pAI->Flash_WriteBuf = MVF_Frey_WriteBuf;
            pAI->Flash_SectErase = MVF_Frey_SectErase;
            pAI->Flash_Ioctl = MVF_Ioctl;
            pAI->Flash_GetBlkBase = MVF_Frey_GetBlkBase;
            break;
        default:
            pAI->Flash_Init = NULL;
            pAI->Flash_Shutdown = NULL;
//            pAI->Flash_ReadBuf = NULL;
            pAI->Flash_WriteBuf = NULL;
            pAI->Flash_SectErase = NULL;
            return -1;
    }
    return 0;
}    
#endif        

