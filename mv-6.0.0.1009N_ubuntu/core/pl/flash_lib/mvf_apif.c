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

#include "mvf_type.h"
#include "mv_incs.h"
#include "mvf_hal.h"
#include "mvf_apif.h"
#if defined( SDT_TYPE_Odin )||defined( SDT_TYPE_Thor )
#include "mvf_spi.h"
#endif
#include "mvstdio.h"
/**********************************************************************************/
/**********************************************************************************/
#if defined( _MVF_DBG_ )
#define MCF_DBG_LV1( X )  X
#else
#define MCF_DBG_LV1( X )
#endif

int
Flash_CheckSkipWrite
(
    void       *pData,
    void       *pBuf,
    MV_U32      Count
)
{
    register MV_U32 i;
    char *Data = pData, *Buf = pBuf;
    for( i=0; i<Count; i++ ){
#if defined(SDT_TYPE_FREY)
        if(Buf[i] == 0xFF)
            continue;
        if((Buf[i] & Data[i]) == Data[i])
            continue;
        return -1;
#else
        if((Buf[i] & Data[i]) != Data[i])
        	return -1;
#endif
    }
    return 0;
}

int
Flash_CheckAreaErased
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *pData, 
    MV_U32      Count
)
{
    char    Buf[2048];
    MV_U32  per_size;
    char *Data = pData;
    while( Count > 0 ){
        per_size = MIN( sizeof( Buf ), Count );
        if( 0!=pAI->Flash_ReadBuf(pAI, pFI, Addr, Buf, per_size ) )
            return -1;
        if( 0!=Flash_CheckSkipWrite(Data, Buf, per_size) )
            return -1;
        Count-= per_size;
        Addr+= per_size;
        Data+= per_size;
    }
    return 0;
}

int
Flash_SearchWritableArea
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      *Addr,
    char        *Data,
    MV_U32      SizeOfArea,
    MV_U32      SizeOfReq
)
{
    char    Buf[2048];
    MV_U32  per_size;
    MV_U32  addr, StartAddr;
    long    i, j;    

    addr = StartAddr = *Addr;
    while( SizeOfArea > 0 )
    {
        per_size = MIN( sizeof( Buf ), SizeOfReq );
        if( 0!=pAI->Flash_ReadBuf(pAI, pFI, addr, Buf, per_size) )
            return -1;
        for( i=0, j=i; i<sizeof( Buf ); i++ )
        {
            if( Buf[i]==0xFF )
                continue;
            if( Data==NULL||(Data[i]&Buf[i])==Data[i] )
                continue;
            j = i;
        }
        if( j!=0 )
            StartAddr = addr+j+1;
        addr+= per_size;
        Data+= per_size;
        SizeOfArea-= per_size;
        if( ( addr-StartAddr ) >= SizeOfReq )
        {
            *Addr = StartAddr;
            return 0;
        }
    }
    return -1;    
}

static int
Flash_Sect2Sect
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      SAddr,
    MV_U32      TAddr,
    MV_U32      Count
)
{
    char    Buf[2048];
    MV_U32  per_size;

    while( Count > 0 )
    {
        per_size = MIN( sizeof( Buf ), Count );
        if( 0!=pAI->Flash_ReadBuf(pAI, pFI, TAddr, Buf, per_size ) )
            return -1;
        if( 0!=Flash_CheckSkipWrite(NULL, Buf, per_size) )
        {
            if( 0!=pAI->Flash_Erase(pAI, pFI, TAddr, 1))
                return -1;
                
        }        
        if( 0!=pAI->Flash_ReadBuf(pAI, pFI, SAddr, Buf, per_size ) )
            return -1;
        if( 0!=pAI->Flash_WriteBuf(pAI, pFI, TAddr, Buf, per_size ) )
            return -1;
        Count-= per_size;
        SAddr+= per_size;
        TAddr+= per_size;
    }
    return 0;
}

int
Flash_SectRMW
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *pData,
    MV_U32      Count,
    MV_U32      SectorAddr
)
{
    MV_U32  StartOfSec, CountOfSector, tmpAddr, task_size, addr_src;
    MV_U32  /*BufSectorAddr,*/ StartOfBufSect;
    char *Data = pData;

    SectorAddr = Flash_GetSectAddr( pFI, SectorAddr );
    if( Flash_GetSectAddr(pFI, Addr)==SectorAddr )
        return -1;
    while( Count > 0 )
    {
        tmpAddr = Flash_GetSectAddr( pFI, Addr );
        StartOfSec = Addr - tmpAddr;
        CountOfSector = pFI->SectSize-StartOfSec;
        CountOfSector = MIN( Count, CountOfSector);
        //BufSectorAddr = ((Count-CountOfSector)>pFI->SectSize)?(tmpAddr+pFI->SectSize):SectorAddr;

        StartOfBufSect = SectorAddr;
        if( 0!=Flash_SearchWritableArea(pAI, pFI, &StartOfBufSect, NULL, pFI->SectSize, pFI->SectSize-CountOfSector) )
        if( 0!=Flash_CheckAreaErased(pAI, pFI, Addr, Data, CountOfSector) )
        {
            if( 0!=pAI->Flash_Erase(pAI, pFI, SectorAddr, 1))
                return -1;
        }        
        
        if( StartOfSec > 0)
            if( 0!=Flash_Sect2Sect(pAI, pFI, tmpAddr, StartOfBufSect, StartOfSec) )
                return -1;
        addr_src = StartOfSec+CountOfSector;
        task_size = pFI->SectSize-addr_src;
        if( task_size>0 )
            if( 0!=Flash_Sect2Sect(pAI, pFI, tmpAddr+addr_src, StartOfBufSect+StartOfSec, task_size) )
                return -1;

        if( 0!=pAI->Flash_Erase(pAI, pFI, tmpAddr, 1))
            return -1;

        if( StartOfSec > 0 )
            if( 0!=Flash_Sect2Sect(pAI, pFI, StartOfBufSect, tmpAddr, StartOfSec) )
                return -1;
        if( task_size>0 )
            if( 0!=Flash_Sect2Sect(pAI, pFI, StartOfBufSect+StartOfSec, tmpAddr+addr_src, task_size) )
                return -1;

        if( 0!=pAI->Flash_WriteBuf(pAI, pFI, Addr, Data, CountOfSector) )
            return -1;
        
        Count-= CountOfSector;
        Addr+= CountOfSector;
        Data+= CountOfSector;
    }    

    return 0;
}
#if defined( _DISABLE_SMART_RMW_ )
static int
Flash_BufRMW
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count,
    MV_U8       *Buf
)
{
MV_U32  StartOfSec, CountOfSector, tmpAddr;
MV_I32  i;

    if (Buf==NULL)
        if(pAI->buf_size<pFI->SectSize)
            return -1;
        else
            Buf = pAI->buf;
    
    while( Count > 0 )
    {
        tmpAddr = Flash_GetSectAddr( pFI, Addr );
        StartOfSec = Addr - tmpAddr;
        CountOfSector = pFI->SectSize-StartOfSec;
        CountOfSector = (Count > CountOfSector)?CountOfSector:Count;
        if( 0!=pAI->Flash_ReadBuf( pAI, pFI, Addr, Buf+StartOfSec, CountOfSector ) )
            return -2;
        for( i=0; i<CountOfSector; i++ )
        {
            if( Data[i]!=Buf[StartOfSec+i]&&
                Buf[StartOfSec+i]!=0xFF )
                break;
        }
        if( i!=CountOfSector )
        {
            if( StartOfSec > 0 )
            {
                if( 0!=pAI->Flash_ReadBuf( pAI, pFI, tmpAddr, Buf, StartOfSec ) )
                    return -2;
            }        
            i = ( StartOfSec+CountOfSector );
            if( i )
            {
                if( 0!=pAI->Flash_ReadBuf( pAI, pFI, Addr, Buf+i, pFI->SectSize - i ) )
                    return -2;
            }        
            for( i=0; i<CountOfSector; i++ )
            {
                Buf[StartOfSec+i] = Data[i];
            }
            if( 0!=pAI->Flash_Erase( pAI, pFI, Addr, 1 ) )
                return -1;
            if( 0!=pAI->Flash_WriteBuf( pAI, pFI, tmpAddr, Buf, StartOfSec+CountOfSector) )
                return -3;
        }
        else
        {
            if( 0!=pAI->Flash_WriteBuf( pAI, pFI, Addr, Data, CountOfSector) )
                return -3;
        }        
        Count-= CountOfSector;
        Addr+= CountOfSector;
        Data+= CountOfSector;
    }    

    return 0;
}
#endif
static int
Flash_BufRMWSmart
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *pData,
    MV_U32      Count,
    void        *pBuf
)
{
    char *Data = (char*)pData;
    char *Buf = (char*)pBuf;
#if !defined( _DISABLE_SMART_RMW_ )
    MV_U32  StartOfSec, CountOfSector, SectAddr;
    long  i;

    if (Buf==NULL) {
        if(pAI->buf_size<pFI->SectSize) {
            return -1;
        } else {
            Buf = pAI->buf;
        }    
    }    
    while( Count > 0 ){
        SectAddr = Flash_GetSectAddr( pFI, Addr);
        StartOfSec = MOD2( Addr, pFI->SectSize);
        CountOfSector = MIN(Count, pFI->SectSize-StartOfSec);
        if( 0!=Flash_CheckAreaErased(pAI, pFI, Addr, Data, CountOfSector) ){
            if( StartOfSec > 0 ){
                if( 0!=pAI->Flash_ReadBuf( pAI, pFI, SectAddr, Buf, StartOfSec ) )
                    return -2;
            }        
            for( i=0; i<CountOfSector; i++ )
                Buf[StartOfSec+i] = Data[i];
            i+= StartOfSec;
            if( (i<pFI->SectSize)&&0!=pAI->Flash_ReadBuf( pAI, pFI, SectAddr+i, Buf+i, pFI->SectSize-i ) )
                return -2;
            if( 0!=pAI->Flash_Erase( pAI, pFI, SectAddr, 1 ) )
                return -1;
            if( 0!=pAI->Flash_WriteBuf( pAI, pFI, SectAddr, Buf, pFI->SectSize) )
                return -3;
        }
        else
        {
            if( 0!=pAI->Flash_WriteBuf( pAI, pFI, Addr, Data, CountOfSector) )
                return -3;
        }        
        Count-= CountOfSector;
        Addr+= CountOfSector;
        Data+= CountOfSector;
    }    

    return 0;
#else
    return Flash_BufRMW(pAI, pFI, Addr, Data, Count, Buf);
#endif    
}
static int Flash_WriteVerify(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void       *pData,
    MV_U32      Count,
    void       *pBuf
)
{
MV_U32  per_size;
char *Data = pData, *Buf = pBuf;

    if (Buf==NULL) {
        if(pAI->buf_size<pFI->SectSize)
            return -1;
        else
            Buf = pAI->buf;
    }    
    while( Count > 0 ){
        per_size = MIN( Count, pAI->buf_size );
        pAI->Flash_ReadBuf (pAI, pFI, Addr, Buf, per_size);
        if (0!=MV_CompareMemory (Buf, Data, per_size))
            return -1;
        Count-= per_size;
        Addr+= per_size;
    }    

    return 0;
}
/**********************************************************************************/
static int Flash_Erase ( AdapterInfo *pAI, FlashInfo *pFI, MV_U32 blkaddr, MV_U32 count)
{
#if defined(SDT_TYPE_FREY)
   int blk_id = -1;
   count+= blkaddr;
   if(count > pFI->Size)
       count = pFI->Size;
   while(blkaddr < count) {
       if (blk_id != pAI->Flash_GetBlkBase(pAI, pFI, blkaddr)) {
           if(0 != pAI->Flash_SectErase(pAI, pFI, blkaddr)) {
               return -1;
           }
           blk_id = pAI->Flash_GetBlkBase(pAI, pFI, blkaddr);
	    mvf_printf("\nErase Block%d address 0x%lx", blk_id, blkaddr);
       }
       blkaddr ++;
   }
#else
	count+= blkaddr;
	if (count>pFI->Size)
		count = pFI->Size;
    while( blkaddr<count ) {
		mvf_printf( "Erase block %lx\n",  blkaddr );
        if( 0!=pAI->Flash_SectErase( pAI, pFI, blkaddr ) ) {
            return -1;
        }
        blkaddr+= pFI->SectSize;
    }
#endif
    return 0;
}
/**********************************************************************************/
static const SupportDevType DevList[] =
{
#if !defined( SDT_TYPE_FREY )
#if defined( SDT_TYPE_Thor )
    { 0x6145, SDT_TYPE_Thor },
    { 0x6122, SDT_TYPE_Thor },
#endif    
#if defined( SDT_TYPE_Odin )
    { 0x6440, SDT_TYPE_Odin },
    { 0x6480, SDT_TYPE_Odin },
#endif    
#if defined( SDT_TYPE_Loki )
    { 0x8480, SDT_TYPE_Loki },
    { 0x8180, SDT_TYPE_Loki },
#endif    
#else
    { 0x9180, SDT_TYPE_FREY },
    { 0x9480, SDT_TYPE_FREY },
    { 0x9580, SDT_TYPE_FREY },
    { 0x9548, SDT_TYPE_FREY },
    { 0x9588, SDT_TYPE_FREY },
#endif    
    { 0x0000, SDT_TYPE_Unknown },
};

SupportDevType*
Flash_GetSupportDevType
(
    MV_U16 DevId
)
{
SupportDevType *pDT;

    pDT = (SupportDevType*)&DevList[0];
    while( pDT->Type!=SDT_TYPE_Unknown )
    {
        if( pDT->DevId == DevId )
            break;
        pDT++;
    }
    return pDT;
}

SupportDevType const *Flash_GetSupportDevTypeBySeq(int Seq)
{
    if( Seq < sizeof( DevList)/sizeof( DevList[0]) )
        return &DevList[Seq];
    return NULL;    
}
/**********************************************************************************/
typedef struct MVF_IF_REG_INFO_
{
    int     ven_id, key;
    int     (*IfRegFunc)(void*);
}MVF_IF_REG_INFO;

extern int  Flash_MVF_IfReg_mvf_loki(void*);
#if defined( SDT_TYPE_Thor )||defined( SDT_TYPE_Odin )
extern int  Flash_MVF_IfReg_mvf_spi(void*);
#endif

static MVF_IF_REG_INFO g_MVFIF_ModuleList[] = 
{
#if !defined(SDT_TYPE_FREY)
#if defined( SDT_TYPE_Thor )||defined( SDT_TYPE_Odin )
    {    0x11ab, SDT_TYPE_Thor,     Flash_MVF_IfReg_mvf_spi,  },
    {    0x11ab, SDT_TYPE_Odin,     Flash_MVF_IfReg_mvf_spi,  },
#endif
    {    0x11ab, SDT_TYPE_Loki,     Flash_MVF_IfReg_mvf_loki, },
#else
    {    0x11ab, SDT_TYPE_FREY,     flash_mvf_ifreg_mvf_frey, },
#endif
    {    0x0000, SDT_TYPE_Unknown,  NULL                    , },
};
static int
Flash_MVFIF_DoesAnyOneWannaSupportMe
(
    __IO__ AdapterInfo     *pAI
)
{
    MVF_IF_REG_INFO *pMIRI = &g_MVFIF_ModuleList[0];
    int type;

    if( pAI==NULL||pAI->devId==0 )
        return -1;
    type = Flash_GetSupportDevType( pAI->devId )->Type;
    if( type==SDT_TYPE_Unknown )
        return -1;
    while( pMIRI->ven_id!=0 )
    {
        if( (pMIRI->key==type)&&(pMIRI->IfRegFunc!=NULL) )
            if( 0==pMIRI->IfRegFunc(pAI) )
                return 0;
        pMIRI++;        
    }
    return -1;
}    
/**********************************************************************************/
static int
DefReadBufHandler
(
    AdapterInfo *pAI,
    FlashInfo   *pFI,
    MV_U32      Addr,
    void        *Data,
    MV_U32      Count
)
{
    if (pFI->flags&FI_FLAG_MEM_MAPPED) {
        MV_CopyMemory(Data, (void*)((char*)pFI->DevBase+Addr), Count);
    } else {
        return -1;
    }    
    return 0;
}

int
Flash_AssignAdapterIf
(
    AdapterInfo *pAI
)
{
    if( pAI==NULL||pAI->devId==0 )
        return -1;
    pAI->FlashBar = NULL;
    pAI->Flash_RMWBuf = NULL;
    pAI->Flash_Sect2Sect = NULL;
    pAI->Flash_SectRMW = NULL;
    pAI->Flash_Ioctl = NULL;
    pAI->Flash_ReadBuf = DefReadBufHandler;
    switch( Flash_GetSupportDevType( pAI->devId )->Type )
    {
#if !defined( SDT_TYPE_FREY)
    #if defined( SDT_TYPE_Thor )
        case SDT_TYPE_Thor:
            pAI->FlashBar = pAI->bar[5];
            break;
    #endif        
    #if defined( SDT_TYPE_Odin)
        case SDT_TYPE_Odin:
            pAI->FlashBar = pAI->bar[2];
            break;
    #endif
    #if defined( SDT_TYPE_Loki )
        case SDT_TYPE_Loki:
        	if( pAI->bar[4] )
	            pAI->FlashBar = pAI->bar[4];
        	else if( pAI->bar[2] )
	            pAI->FlashBar = pAI->bar[2];
            break;
    #endif        
#else
        case SDT_TYPE_FREY:
	     pAI->FlashBar = (void *)0xFC000000;
            break;
#endif        
        default:
            pAI->Flash_Init = NULL;
            pAI->Flash_Shutdown = NULL;
            pAI->Flash_ReadBuf = NULL;
            pAI->Flash_WriteBuf = NULL;
            pAI->Flash_RMWBuf = NULL;
            pAI->Flash_Erase = NULL;
            pAI->Flash_SectErase = NULL;

            pAI->FlashBar = NULL;
            return -1;
    }
    if( 0!=Flash_MVFIF_DoesAnyOneWannaSupportMe(pAI) )
        return -1;
    pAI->Flash_RMWBuf = Flash_BufRMWSmart;
    pAI->Flash_Sect2Sect = Flash_Sect2Sect;
    pAI->Flash_SectRMW = Flash_SectRMW;
    pAI->Flash_Erase = Flash_Erase;
    return 0;
}
/**********************************************************************************/
FlashInfo* Flash_CheckDecodeArea( AdapterInfo *pAI, MV_U32 addr, MV_U32 count)
{
    int i;
    FlashInfo *pFI;
    for( i=0; i<pAI->FlashCount; i++ ) {
        pFI = pAI->pFI[i];
        if( SEG_CHECK_INSIDE(addr, count, pFI->DecodeAddress, pFI->Size) )
            goto ret_good;
        if (count==0) {
            if (pFI->DecodeAddress>addr)
            goto ret_good;
        }        
    }    
    return NULL;    
ret_good:    
    return pFI;
}

