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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef _nsHTTPChannel_h_
#define _nsHTTPChannel_h_

#include "nsIHTTPChannel.h"
#include "nsIChannel.h"
#include "nsHTTPEnums.h"
#include "nsIURI.h"
#include "nsHTTPHandler.h"
#include "nsIEventQueue.h"
#include "nsIEventSinkGetter.h"
#include "nsIHttpEventSink.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsHTTPRequest;
class nsHTTPResponse;
/* 
    The nsHTTPChannel class is an example implementation of a
    protocol instnce that is active on a per-URL basis.

    Currently this is being built with the Netlib dll. But after
    the registration stuff that DP is working on gets completed 
    this will move to the HTTP lib.

    -Gagan Saksena 02/25/99
*/
class nsHTTPChannel : public nsIHTTPChannel
{

public:

    // Constructors and Destructor
    nsHTTPChannel(nsIURI* i_URL, 
                  const char* verb,
                  nsIEventSinkGetter* i_EventSinkGetter,
                  nsIURI* originalURI,
                  nsHTTPHandler* i_Handler);

    virtual ~nsHTTPChannel();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIHTTPCHANNEL

    // nsHTTPChannel methods:
    nsresult            Authenticate(const char *iChallenge,
                                 nsIChannel **oChannel);
    nsresult            Init(nsILoadGroup *aGroup);
    nsresult            Open();
    nsresult            Redirect(const char *aURL,
                                 nsIChannel **aResult);
    nsresult            ResponseCompleted(nsIChannel* aTransport, 
                                          nsresult aStatus);
    nsresult            SetResponse(nsHTTPResponse* i_pResp);
    nsresult            GetResponseContext(nsISupports** aContext);
    nsresult            SetContentLength(PRInt32 aContentLength);
    nsresult            SetContentType(const char* aContentType);
    nsresult            SetCharset(const char *aCharset);

    // Set if this channel is using proxy to connect
    nsresult            SetUsingProxy(PRBool i_UsingProxy);

protected:
    nsCOMPtr<nsIURI>            mOriginalURI;
    nsCOMPtr<nsIURI>            mURI;
    PRBool                      mConnected; 
    nsCOMPtr<nsHTTPHandler>     mHandler;
    HTTPState                   mState;

    nsCString                    mVerb;
    nsCOMPtr<nsIHTTPEventSink>  mEventSink;
    nsCOMPtr<nsIEventSinkGetter> mEventSinkGetter;

    nsHTTPRequest*              mRequest;
    nsHTTPResponse*             mResponse;
    nsIStreamListener*          mResponseDataListener;

    PRUint32                    mLoadAttributes;

    nsCOMPtr<nsISupports>       mResponseContext;
    nsCOMPtr<nsILoadGroup>      mLoadGroup;

    PRInt32                     mContentLength;
    nsCString                   mContentType;
    nsCString                   mCharset;
    nsCOMPtr<nsISupports>       mOwner;
	// Auth related stuff-
	/* 
		If this is true then we have already tried 
		prehost as a response to the server challenge. 
		And so we need to throw a dialog box!
	*/
	PRBool 						mAuthTriedWithPrehost;
    PRBool                      mUsingProxy;
};

#endif /* _nsHTTPChannel_h_ */
