/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#ifndef nsNetService_h___
#define nsNetService_h___

#include "nspr.h"
#include "plevent.h"
#include "nsIPref.h"
#include "nsINetService.h"
#include "nsNetThread.h"
#include "net.h"

class nsITimer;

class nsNetlibService : public nsINetService {

public:
    NS_DECL_ISUPPORTS

    nsNetlibService();

    /* Implementation of the nsINetService interface */
    NS_IMETHOD OpenStream(nsIURL *aUrl, nsIStreamListener *aConsumer);
    NS_IMETHOD OpenBlockingStream(nsIURL *aUrl, 
                                  nsIStreamListener *aConsumer,
                                  nsIInputStream **aNewStream);
    
    NS_IMETHOD InterruptStream(nsIURL* aURL);

    NS_IMETHOD GetCookieString(nsIURL *aURL, nsString& aCookie);
    NS_IMETHOD SetCookieString(nsIURL *aURL, const nsString& aCookie);

#ifdef SingleSignon
    NS_IMETHOD SI_RememberSignonData
        (char* URLName, LO_FormSubmitData *submit);
    NS_IMETHOD SI_RestoreSignonData
        (char* URLNAME, char* name, char** value);
#endif

    NS_IMETHOD GetProxyHTTP(nsString& aProxyHTTP);
    NS_IMETHOD SetProxyHTTP(nsString& aProxyHTTP);
    NS_IMETHOD GetHTTPOneOne(PRBool& aOneOne);
    NS_IMETHOD SetHTTPOneOne(PRBool aSendOneOne);

    NS_IMETHOD GetAppCodeName(nsString& aAppCodeName);
    NS_IMETHOD GetAppVersion(nsString& aAppVersion);
    NS_IMETHOD GetAppName(nsString& aAppName);
    NS_IMETHOD GetLanguage(nsString& aLanguage);
    NS_IMETHOD GetPlatform(nsString& aPlatform);
    NS_IMETHOD GetUserAgent(nsString& aUA);
    NS_IMETHOD SetCustomUserAgent(nsString aCustom);

protected:
    virtual ~nsNetlibService();

    nsresult StartNetlibThread(void);
    nsresult StopNetlibThread(void);

    void SchedulePollingTimer();
    void CleanupPollingTimer(nsITimer* aTimer);
    static void NetPollSocketsCallback(nsITimer* aTimer, void* aClosure);

#if defined(NETLIB_THREAD)
    static void NetlibThreadMain(void *aParam);
#endif /* NETLIB_THREAD */

private:
    void SetupURLStruct(nsIURL *aURL, URL_Struct *aURL_s);
    /* XXX: This is temporary until bamwrap.cpp is removed... */
    void *m_stubContext;
    nsIPref *mPref;

    nsITimer* mPollingTimer;

    nsNetlibThread* mNetlibThread;
};


#endif /* nsNetService_h___ */
