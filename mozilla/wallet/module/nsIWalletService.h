/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.	Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsIWalletService_h___
#define nsIWalletService_h___

//#include "nscore.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsIPresShell.h"
#include "lo_ele.h"
#include "nsIURL.h"

// {738CFD51-ABCF-11d2-AB4B-0080C787AD96}
#define NS_IWALLETSERVICE_IID	 \
{ 0x738cfd51, 0xabcf, 0x11d2, { 0xab, 0x4b, 0x0, 0x80, 0xc7, 0x87, 0xad, 0x96 } };


// {738CFD52-ABCF-11d2-AB4B-0080C787AD96}
#define NS_WALLETSERVICE_CID \
{ 0x738cfd52, 0xabcf, 0x11d2, { 0xab, 0x4b, 0x0, 0x80, 0xc7, 0x87, 0xad, 0x96 } };

/**
 * The nsIWalletService interface provides an API to the wallet service.
 * This is a preliminary interface which <B>will</B> change over time!
 *
 */
struct nsIWalletService : public nsISupports
{
    NS_IMETHOD WALLET_PreEdit(nsIURL* url) = 0;
    NS_IMETHOD WALLET_Prefill(nsIPresShell* shell, PRBool quick) = 0;
    NS_IMETHOD WALLET_Capture(nsIDocument* doc, nsString name, nsString value) = 0;
    NS_IMETHOD WALLET_OKToCapture(PRBool* result, PRInt32 count, char* URLName) = 0;
#ifdef junk
    NS_IMETHOD SI_DisplaySignonInfoAsHTML()=0;
    NS_IMETHOD SI_RememberSignonData
        (char* URLName, LO_FormSubmitData *submit)=0;
    NS_IMETHOD SI_RestoreSignonData
        (char* URLNAME, char* name, char** value)=0;
#endif
};

#endif /* nsIWalletService_h___ */
