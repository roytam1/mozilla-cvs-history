/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Brian Ryner.
 * Portions created by Brian Ryner are Copyright (C) 2000 Brian Ryner.
 * All Rights Reserved.
 *
 * Contributor(s): 
 *  Brian Ryner <bryner@uiuc.edu>
 */

#ifndef nsFingerChannel_h___
#define nsFingerChannel_h___

#include "nsString.h"
#include "nsILoadGroup.h"
#include "nsIInputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsFingerHandler.h"
#include "nsIStreamListener.h"
#include "nsITransport.h"
#include "nsIProxyInfo.h"

class nsFingerChannel 
: public nsIChannel, 
  public nsIStreamListener {

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    // nsFingerChannel methods:
    nsFingerChannel();
    virtual ~nsFingerChannel();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    nsresult Init(nsIURI* uri, nsIProxyInfo* proxyInfo);

protected:
    nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
    nsCOMPtr<nsIURI>                    mOriginalURI;
    nsCOMPtr<nsIURI>                    mUrl;
    nsCOMPtr<nsIStreamListener>         mListener;
    PRUint32                            mLoadFlags;
    nsCOMPtr<nsILoadGroup>              mLoadGroup;
    nsCString                           mContentType;
    PRInt32                             mContentLength;
    nsCOMPtr<nsISupports>               mOwner; 
    PRUint32                            mBufferSegmentSize;
    PRUint32                            mBufferMaxSize;
    PRBool                              mActAsObserver;

    PRInt32                             mPort;
    nsCString                           mHost;
    nsCString                           mUser;

    nsXPIDLCString                      mRequest;

    nsCOMPtr<nsISupports>               mResponseContext;
    nsCOMPtr<nsITransport>              mTransport;
    nsCOMPtr<nsIRequest>                mTransportRequest;
    nsCOMPtr<nsIProxyInfo>              mProxyInfo;
    nsresult                            mStatus;

protected:
    nsresult SendRequest(nsITransport* aTransport);
};

#endif /* nsFingerChannel_h___ */
