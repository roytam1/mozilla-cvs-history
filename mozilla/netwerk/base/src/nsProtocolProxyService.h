/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 
#ifndef __nsprotocolproxyservice___h___
#define __nsprotocolproxyservice___h___

#include "plevent.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIPref.h"
#include "nsVoidArray.h"
#include "nsIProtocolProxyService.h"
#include "nsIProxyAutoConfig.h"
#include "nsIProxyInfo.h"
#include "nsIIOService.h"
#include "prmem.h"

class nsProtocolProxyService : public nsIProtocolProxyService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLPROXYSERVICE

    nsProtocolProxyService();
    virtual ~nsProtocolProxyService();

    NS_IMETHOD Init();

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    void PrefsChanged(const char* pref);

    class nsProxyInfo : public nsIProxyInfo
    {
    public:
        NS_DECL_ISUPPORTS

        NS_IMETHOD_(const char*) Host() {
            return mHost;
        }

        NS_IMETHOD_(PRInt32) Port() {
            return mPort;
        }

        NS_IMETHOD_(const char*) Type() {
            return mType;
        }

        virtual ~nsProxyInfo() {
            if (mHost) nsMemory::Free(mHost);
        }

        nsProxyInfo() : mType(nsnull), mHost(nsnull), mPort(-1) {
            NS_INIT_ISUPPORTS();
        }

        const char* mType;
        char* mHost;
        PRInt32 mPort;
    };

protected:

    nsresult       GetProtocolFlags(const char *scheme, PRUint32 *flags);
    nsresult       NewProxyInfo_Internal(const char *type, char *host, PRInt32 port, nsIProxyInfo **);
    void           LoadStringPref(const char *pref, nsCString &result);
    void           LoadIntPref(const char *pref, PRInt32 &result);
    void           LoadFilters(const char* filters);
    static PRBool  CleanupFilterArray(void* aElement, void* aData);

    // simplified array of filters defined by this struct
    struct host_port {
        nsCString*  host;
        PRInt32     port;
    };

    PRLock                  *mArrayLock;
    nsVoidArray             mFiltersArray;

    PRBool CanUseProxy(nsIURI* aURI);

    nsCOMPtr<nsIIOService>  mIOService;

    nsCOMPtr<nsIPref>       mPrefs;
    PRUint16                mUseProxy;

    nsCString               mHTTPProxyHost;
    PRInt32                 mHTTPProxyPort;

    nsCString               mFTPProxyHost;
    PRInt32                 mFTPProxyPort;

    nsCString               mGopherProxyHost;
    PRInt32                 mGopherProxyPort;

    nsCString               mHTTPSProxyHost;
    PRInt32                 mHTTPSProxyPort;
    
    nsCString               mSOCKSProxyHost;
    PRInt32                 mSOCKSProxyPort;
    PRInt32                 mSOCKSProxyVersion;

    nsCOMPtr<nsIProxyAutoConfig> mPAC;
    nsCString                    mPACURL;

    static void PR_CALLBACK HandlePACLoadEvent(PLEvent* aEvent);
    static void PR_CALLBACK DestroyPACLoadEvent(PLEvent* aEvent);
};

#endif // __nsprotocolproxyservice___h___

