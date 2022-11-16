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

#ifndef __MV_COM_REQUEST_HEADER_H__
#define __MV_COM_REQUEST_HEADER_H__

#define NO_MORE_DATA                            0xFFFF

#ifndef _OS_BIOS
#pragma pack(8)
#endif /* _OS_BIOS */

#ifndef _OS_LINUX
#include "string.h"
#endif

//
// Start of variable size data request structures
//

#define REQUEST_BY_RANGE	1	// get a range of data by a given starting index and total number desired
#define REQUEST_BY_ID		2	// get specific data by a given device ID

typedef struct _RequestHeader
{
    MV_U8  version;				// Request header version; 0 for now.
    MV_U8  requestType;			// REQUEST_BY_ID or REQUEST_BY_RANGE
    MV_U16 startingIndexOrId;	// Starting index (in driver) of the data to be retrieved if requestType is REQUEST_BY_RANGE; 
								// otherwise this is the device ID of which its data is to be retrieved. 
    MV_U16 numRequested;		// Max number of data entries application expected driver to return starting from 
								// startingIndexOrId, Application based on this value to allocate data space. 
								// If requestType is REQUEST_BY_ID, numRequested should set to 1.
    MV_U16 numReturned;			// Actual number of data entries returned by driver. API might reduce this number 
								// by filtering out un-wanted entries.
    MV_U16 nextStartingIndex;	// Driver suggested next starting index.  If requestType is REQUEST_BY_RANGE and  
								// if there is no more data available after this one, set it to NO_MORE_DATA. 
								// If requestType is REQUEST_BY_ID, always set it to NO_MORE_DATA.
    MV_U8  reserved1[6];
} RequestHeader, *PRequestHeader;

typedef struct _Info_Request
{
    RequestHeader header;
    MV_U8	      data[1];
} Info_Request, *PInfo_Request;
/*
void  FillRequestHeader(PRequestHeader pReq, MV_U8 requestType, MV_U16 startingIndexOrId,  MV_U16 numRequested) ;

 void inline  FillRequestHeader(PRequestHeader pReq, MV_U8 requestType, MV_U16 startingIndexOrId,  MV_U16 numRequested) 
{ 
	memset(pReq, 0,sizeof(RequestHeader));  
	pReq->requestType = (requestType);   
	pReq->startingIndexOrId= (startingIndexOrId);  
	pReq->numRequested = (numRequested);  
}
*/


#define FillRequestHeader( pReq,  m_requestType,  m_startingIndexOrId,  m_numRequested)  \
do \
{  \
	memset((pReq), 0,sizeof(RequestHeader));  \
	(pReq)->requestType = (m_requestType);   \
	(pReq)->startingIndexOrId= (m_startingIndexOrId);  \
	(pReq)->numRequested = (m_numRequested);  \
}while(0)


#ifndef _OS_BIOS
#pragma pack()
#endif /* _OS_BIOS */
#endif
