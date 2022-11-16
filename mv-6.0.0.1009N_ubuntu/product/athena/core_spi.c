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

#include "core_header.h"
//#include "com_type.h"
#include "spi_hal.h"
#include "core_spi.h"
#include "core_util.h"

#define IDENTIFY_SPI

static MV_U8           SPICmd[16];

#ifndef IDENTIFY_SPI
MV_U8   default_spi_cmd[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x52, 0x62, 0x15
};
#else
MV_U8   ATMEL_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x52, 0x62, 0x15
};
MV_U8   MXIC_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x90
};
MV_U8   WINBOND_SPI_CMD[16] = 
{
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0xD8, 0xC7, 0xAB
};

MV_U8   ATMEL_SPI_CMD_41A_021[16] = 
{
/* 0 		1	2	 3	   4      5 	    6	    7   	8      9      10       11*/
    0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0xD8, 0x60, 0x9F, 0x36, 0x39, 0x3C
};

MV_U8	EON_F20_SPI_CMD[16] = 
{
	0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20, 0x60, 0x90
};

MV_U8   SST_SPI_CMD[16] = 
{
/* 0 		1	2	 3	   4      5 	    6	    7   	8      9      10       11 */
    0x06, 0x04, 0x05, 0x01, 0x03, 0xAD,  0xD8 , 0xC7, 0xAB, 0x01, 0x50, 0x01 
};

MV_U8 MICRON_SPI_CMD_M25PX[16] =
{
	0x06, 0x04, 0x05, 0x01, 0x03, 0x02, 0x20,0xC7,0x9F, 0xD8
};

#endif

#if defined( SPIRegR32 )
#undef SPIRegR32
#endif
#if defined( SPIRegW32 )
#undef SPIRegW32
#endif

#define SPIRegR32       FMemR32
#define SPIRegW32       FMemW32

/**********************************************************************************/
MV_U32
SPI_Cmd
(
    MV_U8   cmd
)
{
    if( cmd >= sizeof( SPICmd ) )
        return (MV_U32)(-1);
    return SPICmd[cmd];
}
/**********************************************************************************/
int
OdinSPI_WaitDataReady
(
    AdapterInfo *pAI,
    MV_U32         timeout
)
{

    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;
//    MV_U32  timer;

    for( i=0; i<timeout; i++ )
//    timer = Timer_GetTime();
//    while( !Timer_CheckTime( timer, 3 ) )
    {
		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_CTRL_REG );
		//        if( dwTmp & SPI_CTRL_SpiRdy )
        //spi rdy bit will lost some times.
        //check spi start bit clear
        if( !( dwTmp & SPI_CTRL_SpiStart ) )
        {
//            SPI_DBG( ( "WaitDataReady %ld\n", i ) );
            return 0;
        }
        DelayUSec( 10 );
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );

    return -1;
}




int
OdinSPI_WaitCmdIssued
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;
//    MV_U32  timer;

    for( i=0; i<3000; i++ )
//    timer = Timer_GetTime();
//    while( !Timer_CheckTime( timer, 0.5 ) )
    {
		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_CTRL_REG );
		if( !( dwTmp & SPI_CTRL_SpiStart ) )
        {
            return 0;
        }
        DelayUSec( 20 );
    }

    return -1;
}

#define OdinSPI_BuildCmd SPI_BuildCmd

int
SPI_BuildCmd
(
	MV_PVOID	BarAdr,
    MV_U32      *dwCmd,
    MV_U8       cmd,
    MV_U8       read,
    MV_U8       length,
    MV_U32      addr
)

{
    MV_U32  dwTmp;

    SPIRegW32( BarAdr, ODIN_SPI_CTRL_REG, 0 );
    dwTmp = ( ( MV_U32 )cmd << 8 )|( (MV_U32)length << 4 );
	dwTmp |=  0x09130000L;
    if( read )
    {
        dwTmp|= SPI_CTRL_READ;
    }
    if (addr != MV_MAX_U32)
	{
		SPIRegW32( BarAdr, ODIN_SPI_ADDR_REG, ( addr & 0x0003FFFF ) );
		dwTmp|=SPI_CTRL_AddrValid;
	}

	*dwCmd = dwTmp;
    return 0;
}

int
OdinSPI_IssueCmd
(
    AdapterInfo *pAI,
    MV_U32      cmd
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    int     retry;

    for( retry=0; retry<1; retry++ )
    {
        SPIRegW32( BarAdr, ODIN_SPI_CTRL_REG, cmd | SPI_CTRL_SpiStart );
	}    
        
    return 0;
}
int
OdinSPI_RDSR
(
    AdapterInfo *pAI,
    MV_U8       *sr
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
//    int     retry;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
				(MV_U8)SPI_Cmd( SPI_INS_RDSR ), 
                  1, 
                  1, 
                  MV_MAX_U32 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

        dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		*sr = (MV_U8)dwTmp;
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}


int
OdinSPI_EWSR
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];


		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_UPTSEC ), 
                  0, 
                  0, 
                  MV_MAX_U32 );
//   SPI_DBG(("start SPI_EWSR.\n"));
//   SPI_DBG(("command=0x%lx.\n",dwTmp));

    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 ==OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        	return 0;
    }
    SPI_DBG( ( "line: %d\n", __LINE__ ) );
    return -1;
}


int
OdinSPI_WRSR
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32 dwTmp;
    MV_U8 status=0xFF;
   
    if (0!=OdinSPI_RDSR( pAI, &status ))
        return -1;
    
//	SPI_DBG(("flash status=0x%x.\n", status));

    if((status & 0x1C) == 0){	/* Block protection are disabled*/		
		//SPI_DBG(("Block protection are disabled.\n"));
		return 0;
    }

    if (0  !=OdinSPI_EWSR( pAI )){
	SPI_DBG(("Can not enable write satatus.\n"));
        return -1;
    }

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WRSR), 
                  0, 
                  1, 
                  MV_MAX_U32);

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, (MV_U32)(status & (~ 0x1C)));

//   SPI_DBG(("start SPI_INS_WRSR.\n"));
//   SPI_DBG(("command=0x%lx.\n",dwTmp));

    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}





int
OdinSPI_PollSR
(
    AdapterInfo *pAI,
    MV_U8       mask,
    MV_U8       bit,
    MV_U32      timeout
)
{
    MV_U32  i;
    MV_U8   sr;
//    MV_U32  timer;

    for( i=0; i<timeout; i++ )
//    timer = Timer_GetTime();
//    while( !Timer_CheckTime( timer, 3 ) )
    {
        if( 0 == OdinSPI_RDSR( pAI, &sr ) )
        {
//            SPI_DBG( ( "sr:%2.2X\n", sr ) );
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

int
OdinSPI_WREN
(
    AdapterInfo *pAI
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WREN ), 
                  0, 
                  0, 
                  MV_MAX_U32 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0x02, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_WRDI
(
    AdapterInfo *pAI
)
{
    MV_U32 dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_WRDI ), 
                  0, 
                  0, 
                  MV_MAX_U32);
    OdinSPI_IssueCmd( pAI, dwTmp );

    OdinSPI_WaitDataReady( pAI, 10000 );
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_RDPT
(
    AdapterInfo *pAI,
    MV_U32      addr,
    MV_U8       *Data
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32   dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_RDPT ), 
                  1, 
                  1, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );
		*Data = (MV_U8)dwTmp;
        return 0;
    }
    else
    {
        SPI_DBG( ( " SPI_RDPT timeout\n"  ) );
    }
    return -1;
}



int
OdinSPI_SectUnprotect
(
    AdapterInfo *pAI,
    MV_U32      addr
)
{
    MV_U32 dwTmp;
    MV_U8 protect_sect=0xFF;	
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];

    if(-1 ==OdinSPI_RDPT(pAI, addr, &protect_sect))
    {
	return -1;
    }
        //SPI_DBG( ( "%d protect_sect=0x%x ,add=0x%lx\n", __LINE__,protect_sect,addr ) );

    if(protect_sect==0)
    	return 0;
    
    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_UPTSEC), 
                  0, 
                  0, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "error SPI_SectUnprotect \n" ) );
    return -1;
}



int
OdinSPI_SectErase
(
    AdapterInfo *pAI,
    MV_U32      addr
)
{
    MV_U32  dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
	
    if(pAI->FlashID==SST25VF040B )
    {
//	  SPI_DBG(("start  SPI_WRSR.\n"));
      if(-1 == OdinSPI_WRSR(pAI))
      {
        SPI_DBG(("SPI_WRSR error.\n"));
        return -1;
      }
    }
   
   if((pAI->FlashID==AT25DF041A) || (pAI->FlashID==AT25DF021)) {

	   if(-1 == OdinSPI_SectUnprotect(pAI, addr))
	   {
			SPI_DBG(("Un protect error.\n"));
			return -1;
	    }
   }

    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
	                  (MV_U8)SPI_Cmd( SPI_INS_SERASE ), 
                  0, 
                  0, 
                  addr );
    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "error OdinSPI_SectErase\n" ) );
    return -1;
}

int
OdinSPI_ChipErase
(
    AdapterInfo *pAI
)
{
    MV_U32 dwTmp;
	MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
	
    if( -1 == OdinSPI_WREN( pAI ) )
        return -1;

		OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_CERASE ), 
                  0, 
                  0, 
                  MV_MAX_U32 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0!=OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x03, 0, 300000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d\n", __LINE__ ) );
    return -1;
}
int
OdinSPI_Write_autoinc
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U16      Data, 
    MV_U32      Count
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;
   
    if (Count>4)
        Count = 4;

   //SPI_DBG(("autodata=0x%lx.\n",Data));

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, Data );

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_PROG ), 
                  0, 
                  (MV_U8)Count, 
                 Addr );

   //SPI_DBG(("autocommand=0x%lx.\n",dwTmp));
    OdinSPI_IssueCmd( pAI, dwTmp );
    if( 0 != OdinSPI_WaitDataReady( pAI, 10000  ) )
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }


    if( 0 != OdinSPI_PollSR( pAI, 0x01, 0, 5000 ) )
    {
       SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
  
    return 0;
}


int
OdinSPI_Write_SST
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U8*      Data, 
    MV_U32      Count
)
{
	MV_U32 i=0;
	MV_U16 dwTmp;
    //	MV_U8 status=0xFF;

    	//if (0!=SPI_RDSR( pAI, &status ))
        	//	return -1;
	//SPI_DBG(("1 status=0x%x.\n", status));
//	SPI_DBG(("start SPI_WriteTest, count=0x%lx, data=0x%lx.\n",Count, (MV_U32)(*(MV_PU32)Data)));

    	if (0  !=OdinSPI_WREN( pAI ))
        		return -1;

	MV_CopyMemory( &dwTmp, &Data[0], 2);
	if(OdinSPI_Write_autoinc(pAI, Addr, dwTmp, 2)){
	       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
		return -1;
	}

    	//if (0!=SPI_RDSR( pAI, &status ))
        	//	return -1;
    	
	//SPI_DBG(("2 status=0x%x.\n", status));

	for(i=2; i<Count; i+=2){
		MV_CopyMemory( &dwTmp, &Data[i], 2);
//		if((i % 1024) == 0)
//			SPI_DBG(("offset =0x%lx.\n", i));
		if(OdinSPI_Write_autoinc(pAI, -1L, dwTmp, 2)){
		       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
			return -1;
		}
	}

	//SPI_DBG(("start SPI_Write_SST3.\n"));
	
	if(0 != OdinSPI_WRDI(pAI)){
	       	SPI_DBG( ( "%d timeout\n", __LINE__ ) );
		return -1;
	}

	return 0;

	
}

int
OdinSPI_Write
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      Data
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;
    if(pAI->FlashID	==	SST25VF040B ){
	return OdinSPI_Write_SST(pAI, Addr, (MV_U8 *)(&Data), 4);
    }

    OdinSPI_WREN( pAI );

	SPIRegW32( BarAdr, ODIN_SPI_WR_DATA_REG, Data );

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_PROG ), 
                  0, 
                  4, 
                  Addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 != OdinSPI_WaitDataReady( pAI, 10000  ) )
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
        return -1;
    }
    if( 0 == OdinSPI_PollSR( pAI, 0x01, 0, 5000 ) )
    {
        return 0;
    }
    SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    return -1;
}

int
OdinSPI_WriteBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
)
{
    MV_U32  i;

    for( i=0; i<Count; i+=4 )
    {
        if( -1 == OdinSPI_Write( pAI, Addr + i, *(MV_U32*)&Data[i] ) )
        {
            SPI_DBG( ( "Write failed at %5.5x\n", Addr+i ) );
            return -1;
        }
    }
    return 0;
}

int
OdinSPI_Read
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U8       Size
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  i, dwTmp;

    if( Size > 4 )
        Size = 4;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)SPI_Cmd( SPI_INS_READ ), 
                  1, 
                  Size, 
                  Addr );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		for( i=0; i<Size; i++ )
        {
            Data[i] = ((MV_U8*)&dwTmp)[i];
        }
        return 0;
    }
    else
    {
        SPI_DBG( ( "%d timeout\n", __LINE__ ) );
    }
    return -1;
}

int
OdinSPI_ReadBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
)
{
    MV_U32      i, j;
    MV_U32      tmpAddr, tmpData, AddrEnd;
    MV_U8       *u8 = (MV_U8*)Data;

    AddrEnd = Addr + Count;
    tmpAddr = ALIGN( Addr, 4 );
    j = ( Addr & ( MV_BIT( 2 ) - 1 ) );
    if( j > 0 )
    {
        OdinSPI_Read( pAI, tmpAddr, (MV_U8*)&tmpData, 4 );

        for( i=j; i<4; i++ )
        {
            *u8++ = ((MV_U8*)&tmpData)[i];
        }

        tmpAddr+= 4;
    }
    j = ALIGN( AddrEnd, 4 );    
    for(; tmpAddr < j; tmpAddr+=4 )
    {
        if (OdinSPI_Read(pAI, tmpAddr, (MV_U8*)&tmpData, 4) == -1) {
            SPI_DBG( ( "Read failed at %5.5x\n", tmpAddr ) );
            return -1;
        }
        *((MV_U32*)u8) = tmpData;
        u8+= 4;
    }
    if( tmpAddr < AddrEnd )
    {
        OdinSPI_Read( pAI, tmpAddr, (MV_U8*)&tmpData, 4 );
        Count = AddrEnd - tmpAddr;
        for( i=0; i<Count; i++ )
        {
            *u8++ = ( (MV_U8*)&tmpData )[i];
        }
    }
    
    return 0;
}

int
OdinSPI_RMWBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count,
    MV_U8       *Buf
)
{
MV_U32  tmpAddr;
MV_U32  SecSize;
MV_U32  i, StartOfSec;

    SecSize = pAI->FlashSectSize;
    tmpAddr = ALIGN( Addr, SecSize );
    while( Count > 0 )
    {
        StartOfSec = ( Addr - tmpAddr );
        if( ( StartOfSec>0 ) || ( Count<SecSize ) )
            OdinSPI_ReadBuf( pAI, tmpAddr, Buf, SecSize );
        if( 0!=OdinSPI_SectErase( pAI, tmpAddr ) )
            return -1;
        for( i=StartOfSec; Count>0 && i<SecSize; i++ )
        {
            Buf[i] = *Data++;
            Count--;
        }
        if( 0!=OdinSPI_WriteBuf( pAI, tmpAddr, Buf, SecSize ) )
            return -1;
        Addr+= (SecSize - StartOfSec);
        tmpAddr+= SecSize;
    }    

    return 0;
}

#ifdef IDENTIFY_SPI
int
OdinSPI_SSTIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp=0;
  //  MV_U8 status;

    //SST identify

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  SST_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );
//   SPI_DBG(("start SPI_SSTIdentify.\n"));
//   SPI_DBG(("command=0x%lx.\n",dwTmp));

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

//	 SPI_DBG(("start dwTmp=0x%lx.\n",dwTmp));
//  	SPI_RDSR(pAI, &status);
//	 SPI_DBG(("status =0x%x.\n",status));

        switch( dwTmp )
        {
            case 0x8DBF:
               pAI->FlashID = SST25VF040B;
                pAI->FlashSize = 256L * 1024;	/* HBA data just fixed at 256KB, no matter acture flash size */
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}


int
OdinSPI_AtmelIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
    //atmel identify

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  ATMEL_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x631f:
                pAI->FlashID = AT25F2048;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}

int
OdinSPI_AtmelIdentify_41A_021
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
    //atmel identify

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  (MV_U8)ATMEL_SPI_CMD_41A_021[SPI_INS_RDID], 
                  1, 
                  2, 
                  MV_MAX_U32 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
       {
            case 0x441f:
                pAI->FlashID = AT25DF041A;
                pAI->FlashSize = 256L * 1024;	/* HBA data just saved at 256KB, the flash size acture 512KB */
                pAI->FlashSectSize = 64L * 1024;
		    return 0;	
            case 0x431f:
                pAI->FlashID = AT25DF021;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
				return 0;
 	    	case 0x9d7f:
	    		pAI->FlashID = PM25LD010;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}


int
OdinSPI_WinbondIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;
    
    //winbond identify

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  WINBOND_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x1212:
                pAI->FlashID = W25X40;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 64L * 1024;
                return 0;
        }            
    }

    return -1;
}

int
OdinSPI_MxicIdentify
(
    AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;

    //mxic identify

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  MXIC_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );
		switch( dwTmp )
        {
            case 0x11C2:
                pAI->FlashID = MX25L2005;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 4L * 1024;
                return 0;
            case 0x13C2:
                pAI->FlashID = MX25V8006;
                pAI->FlashSize =  1024L * 1024;
                pAI->FlashSectSize = 4L * 1024;
                return 0;
        }
    }

    return -1;
}

int
OdinSPI_EONIdentify_F20
(
	AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];
    MV_U32  dwTmp;

	//EON F20 identify (EN25F20)

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  EON_F20_SPI_CMD[SPI_INS_RDID], 
                  1, 
                  2, 
                  0 );
    OdinSPI_IssueCmd( pAI, dwTmp );
    
    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {

		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );

		switch( dwTmp )
        {
            case 0x111C:
                pAI->FlashID = EN25F20;
                pAI->FlashSize = 256L * 1024;
                pAI->FlashSectSize = 4L * 1024;
                return 0;
        }
    }

    return -1;

}

int
OdinSPI_MicronIdentify_M25PX
(
	AdapterInfo *pAI
)
{
    MV_PVOID  BarAdr = pAI->bar[FLASH_BAR_NUMBER];	
    MV_U32  dwTmp;

	OdinSPI_BuildCmd( BarAdr, &dwTmp, 
                  MICRON_SPI_CMD_M25PX[SPI_INS_RDID], 
                  1, 
                  3, 
                  MV_MAX_U32  );
	
	OdinSPI_IssueCmd( pAI, dwTmp );

    if( 0 == OdinSPI_WaitDataReady( pAI, 10000 ) )
    {
		dwTmp = SPIRegR32( BarAdr, ODIN_SPI_RD_DATA_REG );		

		switch( dwTmp )
        {       
		case 0x147120:
            pAI->FlashID = M25PX80;
            pAI->FlashSize = 1024L * 1024;
            pAI->FlashSectSize = 4L * 1024;
            return 0;
        }
    } 
	
	return -1;
}

#endif

int
OdinSPI_Init
(
    AdapterInfo *pAI
)
{
	MV_U32  i;
#ifndef IDENTIFY_SPI
    for( i=0; i<sizeof( SPICmd ); i++ )
    {
        SPICmd[i] = default_spi_cmd[i];
    }    

    pAI->FlashID = 0x11ab;
    pAI->FlashSize = DEFAULT_SPI_FLASH_SIZE;
    pAI->FlashSectSize = DEFAULT_SPI_FLASH_SECT_SIZE;
    return 0;
#else
    MV_U8   *pSPIVendor;

    pSPIVendor = NULL;
	/* Identify Atmel first. Suppose it's popular. 
	 * Don't identify Mxic since it can use the same instruction set as Atmel. 
	 * If cannot identify, by default use Atmel instruction set. */
    if( 0 == OdinSPI_AtmelIdentify( pAI ) )
    {
        pSPIVendor = ATMEL_SPI_CMD;
    }
    else if( 0 == OdinSPI_MxicIdentify( pAI ) )
    {
        pSPIVendor = MXIC_SPI_CMD;
    }
    	else if ( 0 == OdinSPI_AtmelIdentify_41A_021( pAI) ) {
		  pSPIVendor = ATMEL_SPI_CMD_41A_021;
	}
	else if( 0 == OdinSPI_WinbondIdentify( pAI ) )
	{
		pSPIVendor = WINBOND_SPI_CMD;
	} 
	else if( 0 == OdinSPI_SSTIdentify( pAI ) )
	{
//	 	  SPI_DBG(("find SST flash.\n"));
		pSPIVendor = SST_SPI_CMD;
	}
	else if( 0 == OdinSPI_EONIdentify_F20( pAI ) )
	{
		pSPIVendor = EON_F20_SPI_CMD;
	}
	else if (0 == OdinSPI_MicronIdentify_M25PX(pAI))
	{
	    pSPIVendor = MICRON_SPI_CMD_M25PX;
	}
	else
	{
		pSPIVendor = ATMEL_SPI_CMD;
	}

    if( pSPIVendor )
    {
        for( i=0; i<sizeof( SPICmd ); i++ )
        {
            SPICmd[i] = pSPIVendor[i];
        }    
        return 0;
    }        

    return -1;
#endif
}

int
InitHBAInfo
(
    HBA_Info_Main *pHBAInfo
)
{
    int i;
    MV_U8 *pU8;

    pU8 = (MV_U8*)pHBAInfo;
    for( i=0; i<sizeof( HBA_Info_Main ); i++ )
        pU8[i] = 0xFF;
    MV_CopyMemory( pHBAInfo->signature, "MRVL", 4 );

    return 0;
}

int
GetHBAInfoChksum
(
    HBA_Info_Main *pHBAInfo
)
{
    MV_U32  i;
    MV_U8   *pU8, cs;

    cs = 0;

    pU8 = (MV_U8*)pHBAInfo;
    for( i=0; i<sizeof( HBA_Info_Main ); i++ )
    {
        cs+= pU8[i];
    }
    return cs;
}

int
LoadHBAInfo
(
    AdapterInfo *pAI,
    HBA_Info_Main *pHBAInfo
)
{
    if( 0!=OdinSPI_ReadBuf( pAI, 
                            pAI->FlashSize-sizeof( HBA_Info_Main ), 
                            (MV_U8*)pHBAInfo, 
                            sizeof( HBA_Info_Main ) ) )
    {
        return -1;
    }
    if( 0!=GetHBAInfoChksum( pHBAInfo ) )
    {
        InitHBAInfo( pHBAInfo );
        return -1;
    }
    return 0;
}

#include <core_eeprom.c>
