/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

// ftp implementation header

#ifndef nsFTPChannel_h___
#define nsFTPChannel_h___

#include "nsIURI.h"
#include "nsString.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsIProtocolHandler.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIThreadPool.h"
#include "nsFtpConnectionThread.h"
#include "netCore.h"
#include "nsXPIDLString.h"
#include "nsIStreamListener.h"
#include "nsAutoLock.h"
#include "nsIPrompt.h"
#include "nsIFTPChannel.h"
#include "nsIProxy.h"

#define FTP_COMMAND_CHANNEL_SEG_SIZE 64
#define FTP_COMMAND_CHANNEL_MAX_SIZE 512

#define FTP_DATA_CHANNEL_SEG_SIZE (32*1024)
#define FTP_DATA_CHANNEL_MAX_SIZE (256*1024)

#define NS_FTP_BUFFER_READ_SIZE             (8*1024)
#define NS_FTP_BUFFER_WRITE_SIZE            (8*1024)

#define FTP_CACHE_CONTROL_CONNECTION 1
//#define FTP_SIMULATE_DROPPED_CONTROL_CONNECTION



class nsFTPChannel : public nsIFTPChannel,
                     public nsIProxy,
                     public nsIInterfaceRequestor,
                     public nsIProgressEventSink,
                     public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIFTPCHANNEL
    NS_DECL_NSIPROXY	  
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMOBSERVER

    // nsFTPChannel methods:
    nsFTPChannel();
    virtual ~nsFTPChannel();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    // initializes the channel. 
    nsresult Init(nsIURI* uri);
    
    nsresult SetProxyChannel(nsIChannel *aChannel);

protected:
    nsCOMPtr<nsIURI>                mOriginalURI;
    nsCOMPtr<nsIURI>                mURL;
    nsCOMPtr<nsIProgressEventSink>  mEventSink;
    nsCOMPtr<nsIPrompt>             mPrompter;
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;

    PRBool                          mConnected;
    PRUint32                        mLoadAttributes;

    PRUint32                        mSourceOffset;
    PRInt32                         mAmount;
    nsCOMPtr<nsILoadGroup>          mLoadGroup;
    nsCAutoString                   mContentType;
    PRInt32                         mContentLength;
    nsCOMPtr<nsISupports>           mOwner;

    nsCOMPtr<nsIStreamListener>     mListener;
    nsCOMPtr<nsIStreamObserver>     mObserver;

    nsFtpState*                     mFTPState;   

    nsXPIDLCString                  mHost;
    PRLock*                         mLock;
    nsCOMPtr<nsISupports>           mUserContext;
    nsresult                        mStatus;
    PRPackedBool                    mCanceled;

    nsCOMPtr<nsIChannel>            mProxyChannel; // a proxy channel
    nsCAutoString                   mProxyHost;
    PRInt32                         mProxyPort;
    nsCAutoString                   mProxyType;
    PRBool                          mProxyTransparent;
};

#endif /* nsFTPChannel_h___ */
