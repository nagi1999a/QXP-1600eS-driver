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

#define TWSI_RDY            MV_BIT( 7 )
#define TWSI_RD             MV_BIT( 4 )
#define TWSI_WR             MV_BIT( 3 )
#define TWSI_SLV_ADRS       ( 0 )

static int
Odin_PollTWSICtrlReg
(
    AdapterInfo *pAI,
    MV_U32      mask,
    MV_U32      bit,
    MV_U32      timeout
)
{
    MV_PVOID  BarAdr = pAI->bar[2];
    MV_U32  i;
    MV_U32  sr;

    for( i=0; i<timeout; i++ )
    {
        sr = MV_IOR32( BarAdr, ODIN_SPI_CTRL_REG ); 
        if( ( sr & mask )==bit )
            return 0;
        DelayUSec( 1000 );
    }

    return -1;
}

static int
Odin_EEPRomW32
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      Data
)   
{
    MV_PVOID  BarAdr = pAI->bar[2];

    MV_IOW32( BarAdr, ODIN_SPI_DATA_REG, Data );
    MV_IOW32( BarAdr, ODIN_SPI_CMD_REG, Addr & ( MV_BIT( 18 ) - 1 ) );
    MV_IOW32( BarAdr, ODIN_SPI_CTRL_REG, TWSI_WR );

    if( 0==Odin_PollTWSICtrlReg( pAI, TWSI_WR, 0, 100000  ) )
    {
        DelayMSec( 5 );
        return 0;
    }        

    return -1;
}

static int
Odin_EEPRomR32
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      *Data
)   
{
    MV_PVOID  BarAdr = pAI->bar[2];
    MV_U32    tmpData;

    MV_IOW32( BarAdr, ODIN_SPI_CMD_REG, Addr & ( MV_BIT( 18 ) - 1 ) );
    MV_IOW32( BarAdr, ODIN_SPI_CTRL_REG, TWSI_RD );

    if( 0==Odin_PollTWSICtrlReg( pAI, TWSI_RD, 0, 100000  ) )
    {
        tmpData = MV_IOR32( BarAdr, ODIN_SPI_DATA_REG );
        *Data = tmpData;
        return 0;
    }    

    return -1;
}

int
Odin_EEPRomWriteBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    void*       Data,
    MV_U32      Count
)   
{
    MV_U32      i, j;
    MV_U32      tmpAddr, tmpData, AddrEnd;
    MV_U8       *u8 = (MV_U8*)Data;

    AddrEnd = Addr + Count;
    tmpAddr = MVF_ALIGN( Addr, 4 );
    if( Addr > 0xFF )
        return -2;
    j = ( Addr & ( MV_BIT( 2 ) - 1 ) );
    if( j > 0 )
    {
        Odin_EEPRomR32( pAI, tmpAddr, &tmpData );

        for( i=j; i<4; i++ )
        {
            ( (MV_U8*)&tmpData )[i] = *u8++;
        }
        Odin_EEPRomW32( pAI, tmpAddr, tmpData );

        tmpAddr+= 4;
    }

    j = MVF_ALIGN( AddrEnd, 4 );    
    for( ; tmpAddr < j; tmpAddr+=4 )
    {
        Odin_EEPRomW32( pAI, tmpAddr, *((MV_U32*)u8) );
        u8+= 4;
    }
    if( tmpAddr < AddrEnd )
    {
        Odin_EEPRomR32( pAI, tmpAddr, &tmpData );
        Count = AddrEnd - tmpAddr;
        for( i=0; i<Count; i++ )
        {
            ( (MV_U8*)&tmpData )[i] = *u8++;
        }
        Odin_EEPRomW32( pAI, tmpAddr, tmpData );
    }
    return 0;
}

int
Odin_EEPRomReadBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    void*       Data,
    MV_U32      Count
)   
{
    MV_U32      i, j;
    MV_U32      tmpAddr, tmpData, AddrEnd;
    MV_U8       *u8 = (MV_U8*)Data;

    AddrEnd = Addr + Count;
    tmpAddr = MVF_ALIGN( Addr, 4 );
    if( Addr > 0xFF )
        return -2;
    j = ( Addr & ( MV_BIT( 2 ) - 1 ) );
    if( j > 0 )
    {
        Odin_EEPRomR32( pAI, tmpAddr, &tmpData );

        for( i=j; i<4; i++ )
        {
            *u8++ = ((MV_U8*)&tmpData)[i];
        }

        tmpAddr+= 4;
    }
    j = MVF_ALIGN( AddrEnd, 4 );    
    for(; tmpAddr < j; tmpAddr+=4 )
    {
        Odin_EEPRomR32( pAI, tmpAddr, &tmpData );
        *((MV_U32*)u8) = tmpData;
        u8+= 4;
    }
    if( tmpAddr < AddrEnd )
    {
        Odin_EEPRomR32( pAI, tmpAddr, &tmpData );
        Count = AddrEnd - tmpAddr;
        for( i=0; i<Count; i++ )
        {
            *u8++ = ( (MV_U8*)&tmpData )[i];
        }
    }
    return 0;
}

int
Odin_NVRamWrite
(
    AdapterInfo *pAI,
    MV_U8       ItemPos,
    void*       Data,
    MV_U16      Count
)   
{
    MV_U32      i;
    MV_U8       Hdr[2];
    MV_U8       *u8 = (MV_U8*)Data, cs;

    cs = NVRAM_ENTRY_ID;
    for( i=0; i<Count; i++ )    
        cs+= u8[i];
    Hdr[0] = NVRAM_ENTRY_ID;
    Hdr[1] = 0-cs;

    Odin_EEPRomWriteBuf( pAI, ItemPos, Hdr, 2 );
    Odin_EEPRomWriteBuf( pAI, ItemPos+2, Data, Count );

    return 0;
}

int
Odin_NVRamRead
(
    AdapterInfo *pAI,
    MV_U8       ItemPos,
    void*       Data,
    MV_U16      Count
)   
{
    MV_U32      i;
    MV_U8       Hdr[2];
    MV_U8       *u8 = (MV_U8*)Data, cs;

    Odin_EEPRomReadBuf( pAI, ItemPos, Hdr, 2 );
    Odin_EEPRomReadBuf( pAI, ItemPos+2, Data, Count );
    if( Hdr[0] != NVRAM_ENTRY_ID )
        return -1;
    cs = Hdr[0] + Hdr[1];
    for( i=0; i<Count; i++ )    
        cs+= u8[i];
    if( cs )
        return -2;
    return 0;        
}

