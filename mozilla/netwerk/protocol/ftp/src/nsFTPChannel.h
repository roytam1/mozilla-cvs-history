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

// ftp implementation header

#ifndef nsFTPChannel_h___
#define nsFTPChannel_h___

#include "nsIFTPChannel.h"
#include "nsIStreamListener.h"
#include "nsIThread.h"
#include "nsIURI.h"
#include "nsString2.h"
#include "nsIEventQueue.h"
#include "nsIBufferOutputStream.h"
#include "nsIBufferInputStream.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsHashtable.h"

class nsIEventSinkGetter;
class nsIProgressEventSink;

class nsFTPChannel : public nsIFTPChannel,
                     public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIFTPCHANNEL
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER

    // nsFTPChannel methods:
    nsFTPChannel();
    virtual ~nsFTPChannel();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    // initializes the channel. creates the FTP connection thread
    // and returns it so the protocol handler can cache it and
    // join() it on shutdown.
    nsresult Init(const char* verb, nsIURI* uri, nsILoadGroup *aGroup,
                  nsIEventSinkGetter* getter, nsHashtable *aConnectionList,
                  nsIThread **_retval);

protected:
    nsIURI*                 mUrl;
    nsIEventQueue*          mEventQueue;
    nsIProgressEventSink*   mEventSink;

    PRBool                  mConnected;
    nsIStreamListener*      mListener;
    nsISupports*            mContext;
    PRUint32                mLoadAttributes;

    nsIBufferInputStream*   mBufferInputStream;
    nsIBufferOutputStream*  mBufferOutputStream;
    PRUint32                mSourceOffset;
    PRInt32                 mAmount;
    nsILoadGroup*           mLoadGroup;
    nsAutoString            mContentType;
    PRInt32                 mContentLength;
    nsCOMPtr<nsISupports>   mOwner;
    nsHashtable*            mConnectionList; // thread safe list of connections.
    nsIThread*              mConnectionThread; // the thread for this connection.
};

#define NS_FTP_SEGMENT_SIZE   (4*1024)
#define NS_FTP_BUFFER_SIZE    (8*1024)


#endif /* nsFTPChannel_h___ */
