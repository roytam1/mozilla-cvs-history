/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsFtpProtocolHandler_h___
#define nsFtpProtocolHandler_h___

#include "nsIServiceManager.h"
#include "nsIProtocolHandler.h"
#include "nsHashtable.h"
#include "nsIIOService.h"
#include "nsIThreadPool.h"
#include "nsIObserverService.h"
#include "nsIProtocolProxyService.h"
#include "nsAutoLock.h"

// {25029490-F132-11d2-9588-00805F369F95}
#define NS_FTPPROTOCOLHANDLER_CID \
    { 0x25029490, 0xf132, 0x11d2, { 0x95, 0x88, 0x0, 0x80, 0x5f, 0x36, 0x9f, 0x95 } }

class nsFtpProtocolHandler : public nsIProtocolHandler{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    
    // nsFtpProtocolHandler methods:
    nsFtpProtocolHandler();
    virtual ~nsFtpProtocolHandler();
    
    // Define a Create method to be used with a factory:
    static NS_METHOD Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    nsresult Init();

    // FTP Connection list access
    static nsresult InsertConnection(nsIURI *aKey, nsISupports *aConn);
    static nsresult RemoveConnection(nsIURI *aKey, nsISupports **_retval);
    
protected:
    static nsSupportsHashtable* mRootConnectionList;
    static PRBool DisconnectConnection(nsHashKey *aKey, void *aData, void* closure);
    
    nsCOMPtr<nsIIOService> mIOSvc;
    nsCOMPtr<nsIProtocolProxyService>   mProxySvc;
};

#endif /* nsFtpProtocolHandler_h___ */
