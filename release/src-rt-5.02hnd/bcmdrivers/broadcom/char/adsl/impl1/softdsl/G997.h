/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

/****************************************************************************
 *
 * G997.h 
 *
 * Description:
 *	This file contains the exported functions and definitions for G97Framer
 *
 *
 * Copyright (c) 1993-1998 AltoCom, Inc. All rights reserved.
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.3 $
 *
 * $Id: G997.h,v 1.3 2003/07/18 18:56:59 ilyas Exp $
 *
 * $Log: G997.h,v $
 * Revision 1.3  2003/07/18 18:56:59  ilyas
 * Added support for shared TX buffer (for ADSL driver)
 *
 * Revision 1.2  2002/07/20 00:51:41  ilyas
 * Merged witchanges made for VxWorks/Linux driver.
 *
 * Revision 1.1  2001/12/13 02:28:27  ilyas
 * Added common framer (DslPacket and G997) and G997 module
 *
 *
 *
 *****************************************************************************/

#ifndef	G997FramerHeader
#define	G997FramerHeader

#define kG997NoHdlc		0x1

extern Boolean  G997Init(
		void					*gDslVars, 
		bitMap					setup, 
		ulong					rxBufNum,
		ulong					rxBufSize,
		ulong					rxPacketNum,
		upperLayerFunctions		*pUpperLayerFunctions,
		dslCommandHandlerType	g997PhyCommandHandler);

extern void		G997Close(void *gDslVars);
extern void		G997Timer(void *gDslVars, unsigned long timeQ24ms);
extern Boolean	G997CommandHandler (void *gDslVars, dslCommandStruct *cmd);
extern void		G997StatusSnooper (void *gDslVars, dslStatusStruct *status);

extern	int		G997SendFrame(void *gDslVars, void *pVc, ulong mid, dslFrame *pFrame);
extern  int		G997ReturnFrame(void *gDslVars, void *pVc, ulong mid, dslFrame *pFrame);

extern  Boolean G997SetTxBuffer(void *gDslVars, ulong len, void *bufPtr);
extern	void *	G997GetFramePoolHandler(void *gDslVars);

#endif	/* G997FramerHeader */