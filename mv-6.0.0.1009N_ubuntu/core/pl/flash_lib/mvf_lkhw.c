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

#define _DBG_( _X_ )  //  _X_

#undef MV_REG_R8
#undef MV_REG_W8
#undef MV_REG_R32
#undef MV_REG_W32
#define MV_REG_R8        MV_MMR8
#define MV_REG_W8        MV_MMW8
#define MV_REG_R32       MV_MMR32
#define MV_REG_W32       MV_MMW32

#if defined(SDT_TYPE_FREY)
#define RC9XXX_WIN_REG_SIZE(a)            ((MVF_ROUND(a, 0x1000) - 1) & 0xFFFFF000)
#define RC9XXX_WIN_REAL_SIZE(a)           (((a) | 0x00000FFF) + 1)
#define RC9XXX_WIN_UNIT_SIZE(n)           (((MV_U32)0x10000L) * n)
#define RC9XXX_WIN_DEF_SIZE                 RC9XXX_WIN_UNIT_SIZE(1)

#define RC9XXX_BAR2_DEFAULT_ALIGN_SIZE  (0x1000L * 1)
#define RC9XXX_BAR2_OFF(a)                ((a) & (RC9XXX_BAR2_DEFAULT_ALIGN_SIZE - 1))
#define RC9XXX_BAR2_SEG(a)                ((a) & (~(RC9XXX_BAR2_DEFAULT_ALIGN_SIZE - 1)))

#define RC9XXX_BAR2_ALIGN_SIZE            RC9XXX_WIN_REG_SIZE(RC9XXX_BAR2_DEFAULT_ALIGN_SIZE - 1)

#define RC9XXX_REG_PCIE_VEN_DEV           (0x8000L)
#define RC9XXX_REG_PCIE_WIN_CTRL(x)     (0x8420L + ((x) * 0x10))
#define RC9XXX_REG_PWC_ENA                 MV_BIT(0)
#define RC9XXX_REG_PWC_SIZE(r)            RC9XXX_WIN_REG_SIZE(r)
#define RC9XXX_REG_PWC_TID(r)             ((r>>4) & 0xF)
#define RC9XXX_REG_PWC_ATTR(r)            ((r>>8) & 0xFF)
#define RC9XXX_REG_PWC_BAR(r)             ((r>>1) & 0x01)
#define RC9XXX_REG_PCIE_WIN_BASE(x)       (RC9XXX_REG_PCIE_WIN_CTRL(x) + 4)
#define RC9XXX_REG_PWRM_ENA               MV_BIT(0)
#define RC9XXX_REG_PCIE_WIN_REMAP(x)	  (RC9XXX_REG_PCIE_WIN_CTRL(x) + 0x8)
//#define RC9XXX_REG_PCIE_WIN_REMAP_HI(x)   (RC9XXX_REG_PCIE_WIN_CTRL(x)+0x10)
#define RC9XXX_REG_CPU_CTRL_STS           0x20104L
#define RC9XXX_REG_RSTOUTn_MASK           0x20108L
#define RC9XXX_REG_SYS_SOFT_RESET         0x2010CL
#define RC9XXX_REG_BACK_DOOR              0xF1400L
#define RC9XXX_PCIE_WIN_USE               (3)

#define RC9XXX_REG_A2M_WIN_CTRL(x)        (0x10600L + (x) * 0x10)
#define RC9XXX_REG_A2M_WIN_BASE(x)        (0x10604L + (x) * 0x10)
#define RC9XXX_REG_AWC_ENA                        MV_BIT(5)
#define RC9XXX_REG_A2M_WIN_REMAP(x)       (0x10608L + (x) * 0x10 )
#define RC9XXX_REG_A2M_WIN_REMAP_HI(x)    ( 0x1060CL + (x) * 0x10 )
#else /*SDT_TYPE_FREY*/
#define RC8XXX_WIN_REG_SIZE(a)            ((a)&0xFFFF0000)
#define RC8XXX_WIN_REAL_SIZE(a)           (((a)|0x0000FFFF)+1)
#define RC8XXX_WIN_UNIT_SIZE(n)           (((MV_U32)0x10000L)*n)
#define RC8XXX_WIN_DEF_SIZE               RC8XXX_WIN_UNIT_SIZE(1)
#if 0
#define BAR2_DEFAULT_ALIGN_SIZE           (pAI->RC8XXX_REG_FLASH_SIZE)
#define RC8XXX_BAR2_OFF(a)                ((a)&mask_off)
#define RC8XXX_BAR2_SEG(a)                ((a)&mask_seg)
#else
#define BAR2_DEFAULT_ALIGN_SIZE           ((0x10000L*1))
#define RC8XXX_BAR2_OFF(a)                ((a)&(BAR2_DEFAULT_ALIGN_SIZE-1))
#define RC8XXX_BAR2_SEG(a)                ((a)&(~(BAR2_DEFAULT_ALIGN_SIZE-1)))
#endif
//_MARVELL_SDK_PACKAGE_NONRAID
#define BAR2_ALIGN_SIZE                   RC8XXX_WIN_REG_SIZE(BAR2_DEFAULT_ALIGN_SIZE-1)

#define RC8XXX_REG_PCIE_VEN_DEV           (0x30000L)
#define RC8XXX_REG_PCIE_WIN_CTRL(x)       (0x31820L+((x)*0x10)+(((x)>=5)*0x10))
#define RC8XXX_REG_PWC_ENA                 MV_BIT(0)
#define RC8XXX_REG_PWC_SIZE(r)            RC8XXX_WIN_REG_SIZE(r)
#define RC8XXX_REG_PWC_TID(r)             ((r>>4)&0xF)
#define RC8XXX_REG_PWC_ATTR(r)            ((r>>8)&0xFF)
#define RC8XXX_REG_PWC_BAR(r)             ((r>>1)&0x01)
#define RC8XXX_REG_PCIE_WIN_BASE(x)       (RC8XXX_REG_PCIE_WIN_CTRL(x)+4)
#define RC8XXX_REG_PWRM_ENA               MV_BIT(0)
#define RC8XXX_REG_PCIE_WIN_REMAP(x)	  (RC8XXX_REG_PCIE_WIN_CTRL(x)+0xC)
#define RC8XXX_REG_PCIE_WIN_REMAP_HI(x)   (RC8XXX_REG_PCIE_WIN_CTRL(x)+0x10)
#define RC8XXX_REG_CPU_CTRL_STS           0x20104L
#define RC8XXX_REG_RSTOUTn_MASK           0x20108L
#define RC8XXX_REG_SYS_SOFT_RESET         0x2010CL
#define RC8XXX_REG_BACK_DOOR              0xF1400L
#define RC8XXX_PCIE_WIN_USE               (5)

#define RC8XXX_REG_A2M_WIN_CTRL(x)        ( 0x20000L + (x)*0x10 )
#define RC8XXX_REG_A2M_WIN_BASE(x)        ( 0x20004L + (x)*0x10 )
#define RC8XXX_REG_AWC_ENA                MV_BIT(5)
#define RC8XXX_REG_A2M_WIN_REMAP(x)       ( 0x20008L + (x)*0x10 )
#define RC8XXX_REG_A2M_WIN_REMAP_HI(x)    ( 0x2000CL + (x)*0x10 )
#define RC8XXX_REG_A2M_WINS             8
#endif /*SDT_TYPE_FREY*/


#define BWI_FLAG_ENABLE         MV_BIT(0)
#define BWI_FLAG_REMAP          MV_BIT(1)
#define BWI_FLAG_INTERNALREG    MV_BIT(2)

#define MBUS_ATTR_CS_DEVICE     1

typedef struct MV_BoardWindowInfo_
{
    MV_U32      base;
    MV_U32      size;
    MV_U32      remap_lo;
    MV_U32      remap_hi;
    MV_U16      attr;
    MV_U16      target;
    MV_U32      flags;
}MV_BoardWindowInfo;

typedef struct MV_DeviceDecodeInfo_
{
    char        cs_no;
    char        tgt_no;
    char        attr;
    MV_U32        decode_addr;
    MV_U32      decode_size;
}MV_DeviceDecodeInfo;

#if defined( MV_REG_READ )
#undef MV_REG_READ
#undef MV_REG_WRITE
#endif
#define MV_REG_READ( off )              MV_MMR32( bar, off )
#define MV_REG_WRITE( off, data )       MV_MMW32( bar, off, data )
#define MV_IREG_READ                        MV_REG_READ
#define MV_IREG_WRITE                       MV_REG_WRITE


#undef MV_INLINE
#define MV_INLINE

#define MVF_DEV_IS_INTEL( pFI )   (((MV_U16)pFI->ID&0xFF00)==0x8900 )
#define MVF_DEV_ACCESS_RIGHT( pAI, pFI )
#if defined( _MVFLASH_ )
#if defined(SDT_TYPE_FREY)
#define MVF_PAR_DEV_ID_SETUP( pAI, pFI ) RC9XXX_SetPCIWinDevCS(pAI, pFI->chip_sel)
#define MVF_PAR_DEV_HW_DEP_INIT(pAI, pFI)  \
    RC9XXX_SetPCIWinBase(pAI, (MV_U32)pAI->FlashBar);
#define MVF_PAR_DEV_HW_DEP_SHUTDOWN(pAI, pFI) 
#else
#define MVF_PAR_DEV_ID_SETUP( pAI, pFI ) RC8XXX_SetPCIWinDevCS(pAI, pFI->chip_sel)
#define MVF_PAR_DEV_HW_DEP_INIT(pAI, pFI)  \
    RC8XXX_SetPCIWinBase(pAI, (MV_U32)pAI->FlashBar );
#define MVF_PAR_DEV_HW_DEP_SHUTDOWN(pAI, pFI)  
#endif
#else
#define MVF_PAR_DEV_ID_SETUP( pAI, pFI )
#define MVF_PAR_DEV_HW_DEP_INIT(pAI, pFI)
#define MVF_PAR_DEV_HW_DEP_SHUTDOWN(pAI, pFI)
#endif

#if !defined( SDT_TYPE_FREY)
int RC8XXX_BoardWindowInfoGet ( MV_PVOID bar, MV_BoardWindowInfo *pWI, int WinNo)
{
MV_U32 ctrl, base;
    if( pWI==NULL )
        return MVF_FALSE;
    MV_ZeroMemory( pWI, sizeof( MV_BoardWindowInfo ) );
    if( WinNo<8 )
    {
        ctrl = MV_REG_READ(RC8XXX_REG_A2M_WIN_CTRL(WinNo));
        base = MV_REG_READ(RC8XXX_REG_A2M_WIN_BASE(WinNo));
        if( ctrl&MV_BIT(5) )
        {
            pWI->flags|= BWI_FLAG_ENABLE;
        }    
        else
            return MVF_FALSE;
        if( WinNo<2 )
        {
            pWI->remap_lo = MV_REG_READ(RC8XXX_REG_A2M_WIN_REMAP(WinNo));
            pWI->remap_hi = MV_REG_READ(RC8XXX_REG_A2M_WIN_REMAP_HI(WinNo));
            pWI->flags|= BWI_FLAG_REMAP;
        }
        else
        {
            pWI->remap_lo = 0xFFFFFFFF;
            pWI->remap_hi = 0xFFFFFFFF;
        }
        pWI->base = base&0xFFFF0000;
        pWI->size = RC8XXX_WIN_REAL_SIZE(ctrl);
        pWI->attr = (ctrl>>8)&0xFF;
        pWI->target = (ctrl&0x1F);
        return 1;    
    }
    else if( WinNo==8 )
    {
        base = MV_REG_READ(RC8XXX_REG_A2M_WIN_BASE(WinNo));
        pWI->flags|= BWI_FLAG_INTERNALREG;
    }
    return 1;
}

static const char CSAttrMap[] =
{
    0x1E, 0x1D, 0x1B, 0x0F
};

static int RC8XXX_GetDeviceDecodeInfo( AdapterInfo *pAI, FlashInfo *pFI)
{
    int winNum;
    MV_BoardWindowInfo WinInfo;

    for( winNum=0; winNum<RC8XXX_REG_A2M_WINS; winNum++ ) {
    	if (RC8XXX_BoardWindowInfoGet( pAI->RC8XXX_REG_INTER_BASE , &WinInfo, winNum) == 1) {
    	    int i;
            if( !(WinInfo.flags&BWI_FLAG_ENABLE ) )
                continue;
            if( (WinInfo.target!=MBUS_ATTR_CS_DEVICE ) )
                continue;
            i = CSAttrMap[pFI->chip_sel];
            if( WinInfo.attr==i ) {
                pFI->DecodeAddress = ( WinInfo.base+WinInfo.size )-pFI->Size;
                pFI->DevBase = 0;
                return 1;
            }    
    	}

    }	
    return MVF_FALSE;
}

#if defined( _MVFLASH_ )
static MV_INLINE void
RC8XXX_SetPCIWinBase
(
    register AdapterInfo *pAI,
    MV_U32      Addr
)
{
    int i;
    MV_U32 reg;
    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE-1), 0 );
    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE), Addr );
    MV_RC8XXX_win_remap_setup(pAI);
}    

static MV_U32          cs[]= { 0x00001E13, 0x00001D13, 0x00001B13, 0x0000F13 };
#define RC8XXX_SetPCIWinCtrl(pAI, DevCs, pci_win_no, ena)	do {	\
	MV_U32 reg;	\
	reg = ((cs[DevCs]&(~1L))|(RC8XXX_BAR2_SEG(BAR2_ALIGN_SIZE))|((ena)?1:0));	\
    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, RC8XXX_REG_PCIE_WIN_CTRL(pci_win_no), reg ); \
} while( 0 )
#define RC8XXX_SetPCIWinEna(pAI, pci_win_no, ena)	do {	\
	MV_U32 reg;	\
	reg = MV_REG_R32( pAI->RC8XXX_REG_INTER_BASE, RC8XXX_REG_PCIE_WIN_CTRL(pci_win_no) );	\
	reg = (((reg)&(~1L))|((ena)?1:0));	\
    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, RC8XXX_REG_PCIE_WIN_CTRL(pci_win_no), reg ); \
} while( 0 )

#define RC8XXX_SetPCIWinRemap( pAI, pci_win_no, seg, ena ) do { 	\
        MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE,				\
                    RC8XXX_REG_PCIE_WIN_REMAP(pci_win_no), \
                    RC8XXX_BAR2_SEG(seg)|((ena)?1:0) );	\
} while(0);                    


static  MV_U32          last_seg = 0xFFFFFFFF, mask_seg = 0xFFFFFFFF, mask_off = 0x00000000;

void
RC8XXX_SetPCIWinDevCS
(
    register AdapterInfo *pAI,
    int     DevCs    
)
{
#if defined( _USE_2_WIN_ )
	RC8XXX_SetPCIWinCtrl( pAI, DevCs, RC8XXX_PCIE_WIN_USE-1, 1 );
#else	
	RC8XXX_SetPCIWinCtrl( pAI, DevCs, RC8XXX_PCIE_WIN_USE-1, 0 );
#endif

	RC8XXX_SetPCIWinCtrl( pAI, DevCs, RC8XXX_PCIE_WIN_USE, 1 );
}
MV_INLINE 
int MV_RC8XXX_WinRemapper( AdapterInfo *pAI, MV_U32 addr)    
{
    MV_U32  cur_seg;

	cur_seg=RC8XXX_BAR2_SEG(addr);
	#if defined( _USE_2_WIN_ )
    if(cur_seg!=last_seg ) 
    {
	    if ( cur_seg == 0 )
	    {
		    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, 
		    			RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE), 
		    			0 );
		    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, 
		    			RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE-1), 
		    			(MV_U32)pAI->FlashBar);
	    } else {
		    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, 
		    			RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE-1), 
		    			0);
		    MV_REG_W32( pAI->RC8XXX_REG_INTER_BASE, 
		    			RC8XXX_REG_PCIE_WIN_BASE(RC8XXX_PCIE_WIN_USE), 
		    			(MV_U32)pAI->FlashBar );
    		RC8XXX_SetPCIWinRemap ( pAI, RC8XXX_PCIE_WIN_USE, cur_seg, 1);
	    }
        last_seg = cur_seg;    
	}    
	#else
    if( cur_seg!=last_seg ) 
    {
		RC8XXX_SetPCIWinRemap ( pAI, RC8XXX_PCIE_WIN_USE, cur_seg, 1);
        last_seg = cur_seg;    
    }
    #endif
    return 0;
}

MV_INLINE inline
void MV_RC8XXX_win_remap_setup( AdapterInfo *pAI)    
{
    mask_off = (BAR2_DEFAULT_ALIGN_SIZE-1);
    mask_seg =~(mask_off);
#if defined( _USE_2_WIN_ )
	RC8XXX_SetPCIWinRemap ( pAI, RC8XXX_PCIE_WIN_USE-1, BAR2_ALIGN_SIZE*0, 1);
#endif
	RC8XXX_SetPCIWinRemap ( pAI, RC8XXX_PCIE_WIN_USE, BAR2_ALIGN_SIZE*1, 1);
}
#endif
#if defined( _MVFLASH_)&&!defined( _WIN_REMAP_INLINE_ )
#define FLASH_BASE			((MV_U32)pAI->FlashBar)		
MV_INLINE MV_U8 MV_RC8XXX_MEMR8( AdapterInfo *pAI, void* bar, MV_U32 addr)    
{
	MV_U8	data;
    MV_RC8XXX_WinRemapper( pAI, addr );
    data = MV_REG_R8( FLASH_BASE, RC8XXX_BAR2_OFF(addr) ); 
    return data;
}

MV_INLINE void MV_RC8XXX_MEMW8( AdapterInfo *pAI,void* bar, MV_U32 addr, MV_U8 data )
{
    MV_RC8XXX_WinRemapper( pAI, addr );
    MV_REG_W8( FLASH_BASE, RC8XXX_BAR2_OFF(addr), data ); 
}
void MV_RC8XXX_MEMBufRW( AdapterInfo *pAI, void* bar, MV_U32 addr, void *data, int count) 
{
	char *pu8 = (char*)data;
	int i;
	if (count>0) {
		for (i=0; i<count; i++)
			MV_RC8XXX_MEMW8( pAI, bar, addr++, *pu8++ );
	}
	if (count<0) {
		count =-count;
		for (i=0; i<count; i++)
			*pu8++ = MV_RC8XXX_MEMR8( pAI, bar, addr++ );
	}
}	

MV_INLINE MV_U32 MV_RC8XXX_MEMR32( AdapterInfo *pAI, void* bar, MV_U32 addr)    
{
	MV_U32 data;
    MV_RC8XXX_WinRemapper( pAI, addr );
    data = MV_REG_R32( FLASH_BASE, RC8XXX_BAR2_OFF(addr) );
    return data;
}

MV_INLINE void MV_RC8XXX_MEMW32( AdapterInfo *pAI,void* bar, MV_U32 addr, MV_U32 data)    
{
    MV_RC8XXX_WinRemapper( pAI, addr );
    MV_REG_W32( FLASH_BASE, RC8XXX_BAR2_OFF(addr), data ); 
}
#endif    
int MV_RC8XXX_ResetCpu( void *bar, int flags )
{
	MV_U32 reg;
    MV_U32 u32_tmp;
    long i;
    if (flags&MV_BIT(0)) {
	    u32_tmp = MV_REG_R32( bar, RC8XXX_REG_CPU_CTRL_STS);
        MVF_SET_BIT (u32_tmp, MV_BIT(1), MV_BIT(1) );
        MV_REG_W32 (bar, RC8XXX_REG_CPU_CTRL_STS, u32_tmp );
    } else {
	    u32_tmp = MV_REG_R32( bar, RC8XXX_REG_CPU_CTRL_STS);
        MVF_SET_BIT (u32_tmp, MV_BIT(1), 0 );
        MV_REG_W32 (bar, RC8XXX_REG_CPU_CTRL_STS, u32_tmp );
    }    
    if (flags&MV_BIT(1)){
	    reg = MV_REG_R32( bar, RC8XXX_REG_BACK_DOOR);
	    MVF_SET_BIT ( reg, MV_BIT(24), 0 );
	    MV_REG_W32( bar, RC8XXX_REG_BACK_DOOR, reg );
        u32_tmp = MV_REG_R32( bar, RC8XXX_REG_RSTOUTn_MASK);
        MVF_SET_BIT ( u32_tmp, MV_BIT(2), 0 );
        MV_REG_W32( bar, RC8XXX_REG_RSTOUTn_MASK, u32_tmp );
        MV_REG_W32( bar, RC8XXX_REG_SYS_SOFT_RESET, MV_BIT(0) );
        for( i=0; i<300; i++ ) {
            DelayMSec(10);
            if (MV_REG_R32(bar, 0xF1400)!=u32_tmp ) {
                return 0;
            }    
        }
    }    
    if (flags&MV_BIT(2)){
    	int j;
	    MV_REG_W32( bar, 0xf1480, 1 );
	    while( MV_REG_R32( bar, 0xf1480)&MV_BIT(0) );
	    reg = MV_REG_R32( bar, RC8XXX_REG_BACK_DOOR);
	    MVF_SET_BIT ( reg, MV_BIT(24), 0 );
	    MV_REG_W32( bar, RC8XXX_REG_BACK_DOOR, reg );
        
	    u32_tmp = MV_REG_R32( bar, RC8XXX_REG_CPU_CTRL_STS);
        MVF_SET_BIT (u32_tmp, 6, 4 );
        MV_REG_W32 (bar, RC8XXX_REG_CPU_CTRL_STS, u32_tmp );
	    reg = MV_REG_R32( bar, 0x20314 );
	    j = 0;
        for( i=0; i<500000; i++ ) {
		    u32_tmp = MV_REG_R32( bar, 0x20314 );
		    if (u32_tmp==reg)
		    	j++;
		    if (j>10)	
        		mvf_printf( "\r%lx %lx %d", u32_tmp, reg, j );
        }
        for (i=0; i<2000; i++) {
    		MV_REG_W32 (bar, RC8XXX_REG_CPU_CTRL_STS, MV_BIT(1) );
		    u32_tmp = MV_REG_R32( bar, RC8XXX_REG_CPU_CTRL_STS );
		    if (u32_tmp&MV_BIT(1))
		    	break;
		}    	
    }    
    return -1;
}
#endif /*SDT_TYPE_FREY*/

