/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsDNSService_h__
#define nsDNSService_h__

#include "nsIDNSService.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#if defined(XP_MAC)
#include <OSUtils.h>
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#elif defined (XP_PC)
#include <windows.h>
#endif

class nsIDNSListener;
class nsICancelable;
class nsDNSLookup;

class nsDNSService : public nsIDNSService,
                     public nsIRunnable

{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    // nsDNSService methods:
    nsDNSService();
    virtual ~nsDNSService();
    nsresult Init();
 
    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    // nsIDNSService methods:
    NS_DECL_NSIDNSSERVICE

protected:
    friend class nsDNSLookup;
    friend pascal void  nsDnsServiceNotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);

    nsIThread *   mThread;
    PRBool        mThreadRunning;
    // nsDNSLookup cache? - list of nsDNSLookups, hash table (nsHashTable, nsStringKey)
    // list of nsDNSLookups in order of expiration (PRCList?)

#if defined(XP_MAC)

    InetSvcRef  mServiceRef;
	QHdr		mCompletionQueue;

#if TARGET_CARBON
    OTClientContextPtr  mClientContext;
    OTNotifyUPP         nsDnsServiceNotifierRoutineUPP;
#endif /* TARGET_CARBON */
	
#elif defined(XP_UNIX)
    //XXX - to be defined

#elif defined(_WIN32)
    WNDCLASS wc;
    HWND     DNSWindow;
    UINT     msgAsyncSelect;
    UINT     msgFoundDNS;
#endif
};

#endif /* nsDNSService_h__ */
