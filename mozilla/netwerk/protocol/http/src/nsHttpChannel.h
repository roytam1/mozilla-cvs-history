/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Darin Fisher <darin@netscape.com> (original author)
 */

#ifndef nsHttpChannel_h__
#define nsHttpChannel_h__

#include "nsHttpRequestHead.h"
#include "nsIHttpChannel.h"
#include "nsIHttpEventSink.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInputStream.h"
#include "nsIProgressEventSink.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsICacheListener.h"
#include "nsITransport.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

class nsHttpTransaction;
class nsHttpConnectionInfo;
class nsHttpResponseHead;
class nsIHttpAuthenticator;

//-----------------------------------------------------------------------------
// nsHttpChannel
//-----------------------------------------------------------------------------

class nsHttpChannel : public nsIHttpChannel
                    , public nsIStreamListener
                    , public nsIInterfaceRequestor
                    , public nsIProgressEventSink
                    , public nsICachingChannel
                    , public nsICacheListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIHTTPCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK
    NS_DECL_NSICACHINGCHANNEL
    NS_DECL_NSICACHELISTENER

    nsHttpChannel();
    virtual ~nsHttpChannel();

    nsresult Init(nsIURI *uri,
                  PRUint32 capabilities,
                  const char *proxyHost=0,
                  PRInt32 proxyPort=-1,
                  const char *proxyType=0);

private:
    nsresult Connect(PRBool firstTime = PR_TRUE);
    nsresult AsyncAbort(nsresult status);
    nsresult SetupTransaction();
    nsresult ApplyContentConversions();
    nsresult ProcessResponse();
    nsresult ProcessNormal();
    nsresult ProcessNotModified();
    nsresult ProcessRedirection(PRUint32 httpStatus);
    nsresult ProcessAuthentication(PRUint32 httpStatus);

    // cache specific methods
    nsresult OpenCacheEntry(PRBool *delayed);
    nsresult GenerateCacheKey(nsACString &key);
    nsresult UpdateExpirationTime();
    nsresult CheckCache();
    nsresult ReadFromCache();
    nsresult CloseCacheEntry(nsresult status);
    nsresult CacheReceivedResponse();

    // auth specific methods
    nsresult GetCredentials(const char *challenges, PRBool proxyAuth, nsAFlatCString &creds);
    nsresult SelectChallenge(const char *challenges, nsAFlatCString &challenge, nsIHttpAuthenticator **); 
    nsresult GetAuthenticator(const char *scheme, nsIHttpAuthenticator **);
    nsresult GetUserPassFromURI(nsAString &user, nsAString &pass);
    nsresult ParseRealm(const char *challenge, nsACString &realm);
    nsresult PromptForUserPass(const char *host, PRInt32 port, PRBool proxyAuth, const char *realm, nsAString &user, nsAString &pass);
    nsresult AddAuthorizationHeaders();
    nsresult GetCurrentPath(char **);

private:
    nsCOMPtr<nsIURI>                  mOriginalURI;
    nsCOMPtr<nsIURI>                  mURI;
    nsCOMPtr<nsIStreamListener>       mListener;
    nsCOMPtr<nsISupports>             mListenerContext;
    nsCOMPtr<nsILoadGroup>            mLoadGroup;
    nsCOMPtr<nsISupports>             mOwner;
    nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
    nsCOMPtr<nsIProgressEventSink>    mProgressSink;
    nsCOMPtr<nsIHttpEventSink>        mHttpEventSink;
    nsCOMPtr<nsIInputStream>          mUploadStream;
    nsCOMPtr<nsIURI>                  mReferrer;
    nsCOMPtr<nsISupports>             mSecurityInfo;

    nsHttpRequestHead                 mRequestHead;
    nsHttpResponseHead               *mResponseHead;

    nsHttpTransaction                *mTransaction;     // hard ref
    nsHttpTransaction                *mPrevTransaction; // hard ref
    nsHttpConnectionInfo             *mConnectionInfo;  // hard ref

    nsXPIDLCString                    mSpec;

    PRUint32                          mLoadFlags;
    PRUint32                          mCapabilities;
    PRUint32                          mStatus;
    PRUint32                          mReferrerType;

    // cache specific data
    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    nsCOMPtr<nsITransport>            mCacheTransport;
    nsCOMPtr<nsIRequest>              mCacheReadRequest;
    nsHttpResponseHead               *mCachedResponseHead;
    nsCacheAccessMode                 mCacheAccess;
    PRUint32                          mPostID;
    PRUint32                          mRequestTime;

    PRPackedBool                      mIsPending;
    PRPackedBool                      mApplyConversion;
    PRPackedBool                      mTriedCredentialsFromPrehost;
    PRPackedBool                      mFromCacheOnly;
    PRPackedBool                      mCachedContentIsValid;
};

#endif
