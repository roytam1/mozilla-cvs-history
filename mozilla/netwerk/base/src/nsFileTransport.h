/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL. You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All Rights
 * Reserved.
 */

#ifndef nsFileTransport_h__
#define nsFileTransport_h__

#include "nsIChannel.h"
#include "nsIRunnable.h"
#include "nsFileSpec.h"
#include "prlock.h"
#include "nsIEventQueueService.h"
#include "nsIPipe.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIProgressEventSink.h"
#include "nsIBufferInputStream.h"
#include "nsIBufferOutputStream.h"
#include "nsIFileSystem.h"

class nsIEventSinkGetter;

class nsFileTransport : public nsIChannel, 
                        public nsIRunnable,
                        public nsIPipeObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIPIPEOBSERVER
    NS_DECL_NSIRUNNABLE

    nsFileTransport();
    // Always make the destructor virtual: 
    virtual ~nsFileTransport();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    nsresult Init(nsFileSpec& spec, 
                  const char* command,
                  nsIEventSinkGetter* getter);
    nsresult Init(nsIInputStream* fromStream, 
                  const char* contentType,
                  PRInt32 contentLength,
                  const char* command,
                  nsIEventSinkGetter* getter);

    void Process(void);

    enum State {
        CLOSED,
        OPENING,
        OPENED,
        START_READ,
        READING,
        END_READ,
        START_WRITE,
        WRITING,
        END_WRITE,
        CLOSING
    };

    enum Command {
        NONE,
        INITIATE_READ,
        INITIATE_WRITE
    };

protected:
    nsCOMPtr<nsIProgressEventSink>      mProgress;
    nsFileSpec                          mSpec;      // eliminate?
    nsCOMPtr<nsIFileSystem>             mFileObject;
    char*                               mContentType;

    nsCOMPtr<nsIStreamObserver>         mOpenObserver;
    nsCOMPtr<nsISupports>               mOpenContext;

    nsCOMPtr<nsISupports>               mContext;
    State                               mState;
    Command                             mCommand;
    PRBool                              mSuspended;
    PRMonitor*                          mMonitor;

    // state variables:
    nsresult                            mStatus;
    PRUint32                            mOffset;
    PRInt32                             mTotalAmount;
    PRInt32                             mTransferAmount;

    // reading state varialbles:
    nsCOMPtr<nsIStreamListener>         mListener;
    nsCOMPtr<nsIInputStream>            mSource;
    nsCOMPtr<nsIBufferInputStream>      mBufferInputStream;
    nsCOMPtr<nsIBufferOutputStream>     mBufferOutputStream;

    // writing state variables:
    nsCOMPtr<nsIStreamObserver>         mObserver;
    nsCOMPtr<nsIOutputStream>           mSink;
    char*                               mBuffer;
};

#define NS_FILE_TRANSPORT_SEGMENT_SIZE   (2*1024)
#define NS_FILE_TRANSPORT_BUFFER_SIZE    (8*1024)

#endif // nsFileTransport_h__
