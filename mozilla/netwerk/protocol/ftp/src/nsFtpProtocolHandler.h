/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsFtpProtocolHandler_h___
#define nsFtpProtocolHandler_h___

#include "nsIServiceManager.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsVoidArray.h"
#include "nsIIOService.h"
#include "nsITimer.h"
#include "nsIObserverService.h"
#include "nsICacheSession.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCRT.h"

class nsITimer;
class nsIStreamListener;

// {25029490-F132-11d2-9588-00805F369F95}
#define NS_FTPPROTOCOLHANDLER_CID \
    { 0x25029490, 0xf132, 0x11d2, { 0x95, 0x88, 0x0, 0x80, 0x5f, 0x36, 0x9f, 0x95 } }

class nsFtpProtocolHandler : public nsIProxiedProtocolHandler,
                             public nsIObserver,
                             public nsSupportsWeakReference {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER
    
    // nsFtpProtocolHandler methods:
    nsFtpProtocolHandler();
    virtual ~nsFtpProtocolHandler();
    
    nsresult Init();

    // FTP Connection list access
    static nsresult InsertConnection(nsIURI *aKey, nsISupports *aConn);
    static nsresult RemoveConnection(nsIURI *aKey, nsISupports **_retval);

    static nsresult BuildStreamConverter(nsIStreamListener* in, nsIStreamListener** out);
protected:
    // Stuff for the timer callback function
    struct timerStruct {
        nsCOMPtr<nsITimer> timer;
        nsCOMPtr<nsISupports> conn;
        char* key;
        
        timerStruct() : key(nsnull) {};
        
        ~timerStruct() {
            if (timer)
                timer->Cancel();
            CRTFREEIF(key);
        }
    };

    static void Timeout(nsITimer *aTimer, void *aClosure);
    static nsVoidArray* mRootConnectionList;

    nsCOMPtr<nsIIOService> mIOSvc;
    nsCOMPtr<nsICacheSession> mCacheSession;
    static PRInt32 mIdleTimeout;
};

#endif /* nsFtpProtocolHandler_h___ */
