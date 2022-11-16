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

/*******************************************************************************
*
*                   Copyright 2007,MARVELL SEMICONDUCTOR, LTD.
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
*
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES,
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.
* (MJKK), MARVELL SEMICONDUCTOR ISRAEL. (MSIL),  MARVELL TAIWAN, LTD. AND
* SYSKONNECT GMBH.
*
********************************************************************************/

#if defined( _MVFLASH_ )
    #include <stdio.h>
    #include <string.h>
    #include "mvf_type.h"
    #include "mv_incs.h"
    #define SPI_DBG( _X_ )      mvf_printf _X_
    #define EEPROM_DBG( _X_ )   mvf_printf _X_
#elif defined( _OS_WINDOWS )
    #include "mv_include.h"
    #define SPI_DBG( _X_ )      MV_DPRINT( _X_ )
#endif

#include "mvf_hal.h"
#include "mvf_spi.h"
#include "mvf_apif.h"

#if defined( MV_REG_R32 )
#undef MV_REG_R32
#endif
#if defined( MV_REG_W32 )
#undef MV_REG_W32
#endif

#if defined( SDT_TYPE_Thor )||defined( SDT_TYPE_Odin ) || defined( SDT_TYPE_Vili )

#define MV_REG_R32      pAI->HAL_RegR32
#define MV_REG_W32      pAI->HAL_RegW32

#define SPI_CTRL_REG            0xc0
#define SPI_CTRL_VendorEnable   MV_BIT( 29 )
#define SPI_CTRL_SpiRdy         MV_BIT( 22 )
#define SPI_CTRL_SpiStart       MV_BIT( 20 )

#define SPI_CMD_REG             0xc4
#define SPI_DATA_REG            0xc8

/* Vili only */
#define SPI_CTRL_REG_VILI            0x10000
#define SPI_ADDR_REG_VILI            0x10004
#define SPI_DATA_REG_VILI            0x10008
#define SPI_OP_CODE_VILI             0x1000c

#define SPI_VDIR_VILI           	MV_BIT(13)
#define SPI_ADDR_VLD_VILI         	MV_BIT(12)
#define SPI_CTRL_SpiRdy_VILI        MV_BIT(7)
#define SPI_CTRL_VendorEnable_VILI  MV_BIT(5)
#define SPI_OP_VLD_VILI				MV_BIT(4)
#define SPI_CTRL_SpiStart_VILI      MV_BIT(4)

#define _spi_vendor_data_bytes(bytes)   (((MV_U32)((bytes) & 0x7))  << 8)
#define SPI_CTRL_WREN			MV_BIT(18)|MV_BIT(17)
#define SPI_CTRL_WRITE			MV_BIT(17)
#define SPI_CTRL_READ			MV_BIT(17)|MV_BIT(16)
#define SPI_RDID_WINB			MV_BIT(23)|MV_BIT(20)
#define SPI_SECERASE_WINB		MV_BIT(21)
#define SPI_BLKERASE_WINB		MV_BIT(23)|MV_BIT(22)|MV_BIT(20)|MV_BIT(19)
/* END OF Vili only */


#define ODIN_SPI_CTRL_REG       0x10
#define ODIN_SPI_CMD_REG        0x14
#define ODIN_SPI_DATA_REG       0x18

enum {
     SPI_INS_WREN = 0,
     SPI_INS_WRDI = 1,
     SPI_INS_RDSR = 2,
     SPI_INS_WRSR = 3,
     SPI_INS_READ = 4,
     SPI_INS_PROG = 5,
     SPI_INS_SERASE = 6,
     SPI_INS_CERASE = 7,
     SPI_INS_RDID = 8,
};
typedef struct SPIFlashInfo_
{
    char    id_count;
    char    id[15];
    unsigned long size;
    long    sect_size;    
    char    CmdSet[16];
}SPIFlashInfo;

static
SPIFlashInfo SPIFlashInfoList[] =
{
    {/*atmel 25f2048*/
        2, { 0x1F, 0x63, 0x00,  },
        256L * 1024, 64L * 1024,
        { 0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x52, 0x62, 0x15, },
    },
    {/*winbond w25x40*/
        2, { 0xEF, 0x12, },
        512L * 1024, 4L * 1024,
        { 0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0xC7, 0x90, },
    },
    {/*mxic 25l2005*/
        3, { 0xC2, 0x20, 0x12, 0x00, },
        256L * 1024, 4L * 1024,
        { 0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x9F, },
    },
    {/*st m25p20*/
        2, { 0x12, 0x20, 0x00, },
        256L * 1024, 64L * 1024,
        { 0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x9F, },
    },
    {/*sst25lf020a/sst25lf040a */
        2, { 0xBF, 0x43, 0x44, 0x00, },
        256L * 1024, 4L * 1024,
        { 0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x90, },
    },
    { 0, },
};

static MV_U8            *SPICmd;
/**********************************************************************************/
static MV_U32
Thor_MMR32
(
    void*   addr,
    MV_U32  off
)
{
    return MV_MMR32( addr, off );
}
static void
Thor_MMW32
(
    void*   addr,
    MV_U32  off,
    MV_U32  data
)
{
    MV_MMW32( addr, off, data );
}
static MV_U32
Odin_IOR32
(
    void*   addr,
    MV_U32  off
)
{
    return MV_IOR32( addr-(0xc0-0x10), off );
}
static void
Odin_IOW32
(
    void*   addr,
    MV_U32  off,
    MV_U32  data
)
{
    MV_IOW32( addr-(0xc0-0x10), off, data );
}
/**********************************************************************************/
static MV_U32
SPI_Cmd
(
    MV_U8   cmd
)
{
    return SPICmd[cmd];
}

static int
SPI_WaitDataReady
(
    AdapterInfo     *pAI,
    MV_U32          timeout
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
    MV_U32  i, dwTmp;

    i = 0;
    while (timeout-->0)
    {
    	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
	        dwTmp = MV_REG_R32( BarAdr, SPI_CTRL_REG_VILI );

	        if( ( dwTmp & SPI_CTRL_SpiRdy_VILI ) )
	        {
	        	return 0;
	        }
    	} else {
    	    dwTmp = MV_REG_R32( BarAdr, SPI_CTRL_REG );
        	if( !( dwTmp & SPI_CTRL_SpiStart ) )
        	{
            	return 0;
        	}
    	}
        DelayUSec( 10 );
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );

    return -1;
}




static int
SPI_BuildCmd
(
    MV_U32      *dwCmd,
    MV_U8       cmd,
    MV_U8       read,
    MV_U8       length,
    MV_U32      addr
)
{
    MV_U32  dwTmp;

    dwTmp = ( ( MV_U32 )cmd << 24 )|( (MV_U32)length << 19 );
    if( read )
    {
        dwTmp|= MV_BIT( 23 );
    }
    if( addr!=-1L )
    {
        dwTmp|= MV_BIT( 22 );
        dwTmp|= ( addr & 0x0003FFFF );
    }
    *dwCmd = dwTmp;
    return 0;
}

static int
SPI_IssueCmd
(
    AdapterInfo *pAI,
    MV_U32      cmd
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
		MV_REG_W32( BarAdr, SPI_CTRL_REG_VILI, cmd );
	} else {
    	MV_REG_W32( BarAdr, SPI_CTRL_REG, SPI_CTRL_VendorEnable );
    	MV_REG_W32( BarAdr, SPI_CMD_REG, cmd );
    	MV_REG_W32( BarAdr, SPI_CTRL_REG, SPI_CTRL_VendorEnable | SPI_CTRL_SpiStart );
    }
        
    return 0;
}

static int
SPI_RDSR
(
    AdapterInfo *pAI,
    MV_U8       *sr
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
    MV_U32   dwTmp;

    if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
	    dwTmp = SPI_OP_VLD_VILI|SPI_INS_RDSR;
	} else {
	    SPI_BuildCmd( &dwTmp, 
	                  (MV_U8)SPI_Cmd( SPI_INS_RDSR ), 
	                  0, 
	                  0, 
	                  -1 );
	}
    SPI_IssueCmd( pAI, dwTmp );

    if( 0 == SPI_WaitDataReady( pAI, 10000 ) )
    {
    	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
        	dwTmp = MV_REG_R32( BarAdr, SPI_DATA_REG_VILI );
        } else {
        	dwTmp = MV_REG_R32( BarAdr, SPI_DATA_REG );
        }
        *sr = (MV_U8)dwTmp;
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}

static int
SPI_PollSR
(
    AdapterInfo *pAI,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout
)
{
    MV_U8   sr;

    timeout/= 20;
    while (timeout-->0)
    {
        if( 0 == SPI_RDSR( pAI, &sr ) )
        {
            if( ( sr & mask )==bit )
                return 0;
        }
        else
            SPI_DBG( ( "%d\n", __LINE__ ) );
        DelayUSec( 20 );
    }

    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

static int
SPI_WREN
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;

    if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
	    dwTmp = SPI_OP_VLD_VILI|SPI_INS_WREN;
	} else {
	    SPI_BuildCmd( &dwTmp, 
	                  (MV_U8)SPI_Cmd( SPI_INS_WREN ), 
	                  0, 
	                  0, 
	                  -1 );
	}
    SPI_IssueCmd(pAI, dwTmp);

    if( 0!=SPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == SPI_PollSR( pAI, 0x03, 0x02, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

static int
SPI_WRDI
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;

	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
	    dwTmp = SPI_OP_VLD_VILI|SPI_INS_WRDI;
	} else {
	    SPI_BuildCmd( &dwTmp, 
	                  (MV_U8)SPI_Cmd( SPI_INS_WRDI ), 
	                  0, 
	                  0, 
	                  -1 );
	}
    SPI_IssueCmd( pAI, dwTmp );

    SPI_WaitDataReady( pAI, 10000 );
    if( 0 == SPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

static int
SPI_SectErase
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr
)
{
    MV_U32  dwTmp;

    if( -1 == SPI_WREN( pAI ) )
        return -1;
    SPI_BuildCmd( &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_SERASE ), 
                  0, 
                  0, 
                  addr );
    SPI_IssueCmd( pAI, dwTmp );
    if( 0!=SPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == SPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

static int
SPI_SectErase_Vili
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      addr
)
{
	MV_PVOID  BarAdr = pAI->FlashBar;
	MV_U32      dwTmp=0;
	MV_U32      dwTmp2=0;

    if( -1 == SPI_WREN( pAI ) )
        return -1;

	addr &= ~(0x10000-1);

	MV_REG_W32( BarAdr, SPI_ADDR_REG_VILI, addr );
	
#if 1
	dwTmp2 = pFI->ID&0xFF;
	if(dwTmp2 == 0x1F) {
	    MV_REG_W32(BarAdr, SPI_CTRL_REG_VILI, 0x00521030);
	} else {
		dwTmp = SPI_BLKERASE_WINB | SPI_ADDR_VLD_VILI | 
   				SPI_CTRL_VendorEnable_VILI | SPI_OP_VLD_VILI;
   		SPI_IssueCmd( pAI, dwTmp );
   	}
#else
    _spi_run_op(SPI_CTRL_OP_SECTOR_ERASE);// not working right now
#endif
//_MARVELL_SDK_PACKAGE_NONRAID

	if(0!=SPI_WaitDataReady( pAI, 10000 ))
	{
		mvf_printf(  "time out waiting\n" );
		return -1;
	}
	return 0;
}

static int
SPI_Write
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    MV_U32      Data,
    int         Count
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
    MV_U32   dwTmp;

    SPI_WREN( pAI );
    MV_REG_W32( BarAdr, SPI_DATA_REG, Data );
    SPI_BuildCmd( &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_PROG ), 
                  0, 
                  Count, 
                  Addr );
    SPI_IssueCmd( pAI, dwTmp );

    if( 0 != SPI_WaitDataReady( pAI, 10000  ) ){
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
    if( 0 == SPI_PollSR( pAI, 0x01, 0, 5000 ) ){
        return 0;
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}

static int
SPI_Write_Vili
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    MV_U32      Data,
    int         Count
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
    MV_U32   dwTmp;
    MV_U32   dwTmp2=0;

    SPI_WREN( pAI );
    MV_REG_W32( BarAdr, SPI_DATA_REG_VILI, Data );
    MV_REG_W32( BarAdr, SPI_ADDR_REG_VILI, Addr );
    dwTmp2 = pFI->ID&0xFF;
    if(dwTmp2 == 0x1F) {
	    dwTmp = SPI_OP_VLD_VILI|SPI_INS_PROG;
	} else {
		dwTmp = SPI_CTRL_WRITE | SPI_ADDR_VLD_VILI |_spi_vendor_data_bytes(4)| 
				SPI_CTRL_VendorEnable_VILI | SPI_OP_VLD_VILI;
   	}

    SPI_IssueCmd( pAI, dwTmp );

    if( 0 != SPI_WaitDataReady( pAI, 10000  ) ){
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
    if( 0 == SPI_PollSR( pAI, 0x01, 0, 5000 ) ){
        return 0;
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}

int
SPI_WriteBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void       *Data,
    MV_U32      Count
)
{
    MV_U32 *u32ptr = (MV_U32*)Data;
    int per_io;
    while( Count>0 )
    {
        per_io = MIN (4, Count);
        if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
        	if( 0 != SPI_Write_Vili( pAI, pFI, Addr ,*u32ptr++, per_io ) )
            	return -1;
    	} else {
    		if( 0 != SPI_Write( pAI, pFI, Addr ,*u32ptr++, per_io ) )
            	return -1;
    	}
        Addr+=4;    
        Count-= 4;
    }
    return 0;
}

static int
SPI_Read
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void      	*Data,
    int         Count
)
{
    MV_PVOID  BarAdr = pAI->FlashBar;
    MV_U32  dwTmp;

    SPI_BuildCmd( &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_READ ), 
                  1, 
                  Count, 
                  Addr );
    SPI_IssueCmd( pAI, dwTmp );

    if( 0 == SPI_WaitDataReady( pAI, 10000 ) ) {
        *(MV_U32*)Data = MV_REG_R32( BarAdr, SPI_DATA_REG );
        return 0;
    }    
    else
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}

static int
SPI_Read_Vili
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void      	*Data,
    int         Count
)
{
    void*   BarAdr = pAI->FlashBar;
    MV_U32  dwTmp;    

	MV_REG_W32( BarAdr, SPI_ADDR_REG_VILI, Addr );
	dwTmp = SPI_CTRL_READ | SPI_VDIR_VILI  | SPI_ADDR_VLD_VILI |
			_spi_vendor_data_bytes(4)| SPI_CTRL_VendorEnable_VILI | SPI_OP_VLD_VILI;
    SPI_IssueCmd( pAI, dwTmp );

    if( 0 == SPI_WaitDataReady( pAI, 10000 ) ) {
        *(MV_U32*)Data = MV_REG_R32( BarAdr, SPI_DATA_REG_VILI );
        return 0;
    }    
    else
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}


int
SPI_ReadBuf
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void       *Data,
    MV_U32      Count
)
{
    int i, j, k;
    MV_U32 u32tmp;
    k=0;
    while( Count>0 ) {
        if (((i = MOD2( Addr, 4))>0)||(Count<4)) {
            Addr-= i;
            j = MIN (Count, 4)-i;
            if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
            	SPI_Read_Vili(pAI, pFI, Addr, Data, j);
        	} else {
        		SPI_Read(pAI, pFI, Addr, Data, j);
        	}
            Addr+= j;
        } else {    
            MV_U32 *p_u32 = (MV_U32*)Data;
            for( ;Count>=4; Count-=4, Addr+= 4 ){
            	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
            		SPI_Read_Vili(pAI, pFI, Addr, (void*)((char*)Data+k*4), 4 );
            		k++;
        		} else {
        			SPI_Read(pAI, pFI, Addr, Data, j);
        		}
            }
        }        
    }
    return 0;
}

int
SPI_Identify
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    SPIFlashInfo *pSFI    
)
{
    MV_PVOID    BarAdr = pAI->FlashBar;
    MV_U32      dwTmp;
    MV_U8       *u8ptr = (MV_U8*)&dwTmp;

    SPI_BuildCmd( &dwTmp, 
                  pSFI->CmdSet[SPI_INS_RDID], 
                  1, 
                  4, 
                  0 );
    SPI_IssueCmd( pAI, dwTmp );

    if( 0 == SPI_WaitDataReady( pAI, 10000 ) )
    {
        int i, j;
        MV_U8 m_id, d_id;
        char *id = pSFI->id;

        dwTmp = MV_REG_R32( BarAdr, SPI_DATA_REG );
        pFI->ID = 0;
        m_id = dwTmp&0xFF;
        d_id = (dwTmp>>8)&0xFF;
        if (m_id==pSFI->id[0]) {
            j = pSFI->id_count-1;
            id = pSFI->id+1;
            u8ptr++;
            for ( i=1;i<sizeof( pSFI->id); i+= j )
                if (0==memcmp( pSFI->id+i, u8ptr, j )) {
                    j = 0;
                    break;
                }    
            if (j>0)
                mvf_printf ("manufacture id is supported but device id not in list.!!\n");
            pFI->ID = m_id;
        }    
        if (m_id==pSFI->id[0]) {
            MV_U32_PUT_BITS(pFI->ID, 0, pSFI->id_count*8, dwTmp);
            pFI->DecodeAddress = 0;
            pFI->SectSize = pSFI->sect_size;
            pFI->Size     = pSFI->size;
            pFI->WriteBufSize = 4;
            return 0;
        }
    }
    return -1;
}

int
SPI_Identify_Vili
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    SPIFlashInfo *pSFI    
)
{
    MV_PVOID    BarAdr = pAI->FlashBar;
    MV_U32 BarAdr1 = 0xDFF00000;
    MV_U32      dwTmp=0, dwTmp2=0;
    MV_U8       *u8ptr = (MV_U8*)&dwTmp;
    MV_U8		num = 0;
    int i, j;
    char *id = pSFI->id;

    dwTmp = SPI_OP_VLD_VILI|SPI_INS_RDID;
retry:
    SPI_IssueCmd( pAI, dwTmp );
    if( 0 == SPI_WaitDataReady( pAI, 100000 ) )
    {
	    MV_U8 m_id=0x00000000;
	    dwTmp = 0x00000000;
        dwTmp = MV_REG_R32( BarAdr, SPI_DATA_REG_VILI );
        pFI->ID = 0;
        m_id = dwTmp&0xFF;
        if (m_id==pSFI->id[0]) {
            j = pSFI->id_count-1;
            id = pSFI->id+1;
            u8ptr++;
            for ( i=1;i<sizeof( pSFI->id); i+= j )
                if (0==memcmp( pSFI->id+i, u8ptr, j )) {
                    j = 0;
                    break;
                }
            pFI->ID = dwTmp;
        } else {
        	if(num == 0){
        		dwTmp = SPI_RDID_WINB | SPI_VDIR_VILI | SPI_ADDR_VLD_VILI |_spi_vendor_data_bytes(4)| 
        				SPI_CTRL_VendorEnable_VILI | SPI_OP_VLD_VILI;
        		num++;
        		goto retry;
        	}
        }
        if (m_id==pSFI->id[0]) {
            MV_U32_PUT_BITS(pFI->ID, 0, pSFI->id_count*8, dwTmp);
            pFI->DecodeAddress = 0;
            pFI->SectSize = pSFI->sect_size;
            pFI->Size     = pSFI->size;
            pFI->WriteBufSize = 4;
            return 0;
        }
    }
    return -1;
}

int
SPI_Init
(
    AdapterInfo     *pAI,
    FlashInfo       *pFI
)
{
    SPIFlashInfo *pSFI = SPIFlashInfoList;

    if (pFI==NULL) {
        if( Flash_IsType(pAI, Odin)){
            pAI->HAL_RegR32 = Odin_IOR32;
            pAI->HAL_RegW32 = Odin_IOW32;
        }
        else if( Flash_IsType(pAI, Thor)){
            pAI->HAL_RegR32 = Thor_MMR32;
            pAI->HAL_RegW32 = Thor_MMW32;
        } else if(Flash_IsType(pAI, Vili)){
            pAI->HAL_RegR32 = Thor_MMR32;
            pAI->HAL_RegW32 = Thor_MMW32;
        }else{
            return -1;
        }    
        return 0;
    }    
    while( pSFI->size!=0 ){
    	if( Flash_GetSupportDevType( pAI->devId )->Type == SDT_TYPE_Vili ){
        	if( 0==SPI_Identify_Vili( pAI, pFI, pSFI ) ){
            	SPICmd = pSFI->CmdSet;
            	return 0;
        	}
        } else {
            if( 0==SPI_Identify( pAI, pFI, pSFI ) ){
            	SPICmd = pSFI->CmdSet;
            	return 0;
        	}
        }
        pSFI++;
        DelayMSec(50);
    }
    return -1;
}

int
SPI_Shutdown
(
    AdapterInfo     *pAI,
    FlashInfo       *pFI
)
{
    return 0;
}

/**********************************************************************************/
int
Flash_MVF_IfReg_mvf_spi
(
    void        *OsContext
)
{
AdapterInfo *pAI = (AdapterInfo*)OsContext;

    switch( Flash_GetSupportDevType( pAI->devId )->Type )
    {
        case SDT_TYPE_Thor:
        case SDT_TYPE_Odin:
            pAI->Flash_Init = SPI_Init;
            pAI->Flash_Shutdown = SPI_Shutdown;
            pAI->Flash_ReadBuf = SPI_ReadBuf;
            pAI->Flash_WriteBuf = SPI_WriteBuf;
            pAI->Flash_SectErase = SPI_SectErase;
            break;
        case SDT_TYPE_Vili:
            pAI->Flash_Init = SPI_Init;
            pAI->Flash_Shutdown = SPI_Shutdown;
            pAI->Flash_ReadBuf = SPI_ReadBuf;
            pAI->Flash_WriteBuf = SPI_WriteBuf;
            pAI->Flash_SectErase = SPI_SectErase_Vili;
            break;
        default:
            pAI->Flash_Init = NULL;
            pAI->Flash_Shutdown = NULL;
            pAI->Flash_ReadBuf = NULL;
            pAI->Flash_WriteBuf = NULL;
            pAI->Flash_SectErase = NULL;

            pAI->FlashBar = NULL;
            return -1;
    }
    return 0;
}

#include "odin_eeprom.c"
#endif
