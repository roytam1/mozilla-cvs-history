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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsResChannel_h__
#define nsResChannel_h__

#include "nsIResChannel.h"
#include "nsIStreamListener.h"
#include "nsIResProtocolHandler.h"
#include "nsIURI.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadGroup.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"
#include "nsAutoLock.h"
#include "nsIIOService.h"
#include "prthread.h"

class nsResChannel : public nsIResChannel,
                     public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIRESCHANNEL
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER

    nsResChannel();
    virtual ~nsResChannel();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
    
    nsresult Init(nsIResProtocolHandler* handler,
                  const char* command, 
                  nsIURI* uri,
                  nsILoadGroup* aLoadGroup, 
                  nsIInterfaceRequestor* notificationCallbacks, 
                  nsLoadFlags loadAttributes,
                  nsIURI* originalURI,
                  PRUint32 bufferSegmentSize,
                  PRUint32 bufferMaxSize);

protected:
    class Substitutions {
    public:
        Substitutions() : mSubstitutions(nsnull) {}
        ~Substitutions(){
            if (mSubstitutions) {
                delete mSubstitutions;
                mSubstitutions = nsnull;
            }
        }

        nsresult Init();
        nsresult Next(nsIURI* *result, nsIIOService* serv);
    protected:
        nsCOMPtr<nsIURI>                mResourceURI;
        nsCStringArray*                 mSubstitutions;
    };
    friend class Substitutions;

#define GET_SUBSTITUTIONS_CHANNEL(_this) \
    ((nsResChannel*)((char*)(_this) - offsetof(nsResChannel, mSubstitutions)))

    enum State {
        QUIESCENT,
        ASYNC_OPEN,
        ASYNC_READ,
        ASYNC_WRITE
    };

    nsIStreamListener* GetUserListener() {
        // this method doesn't addref the listener
        NS_ASSERTION(mState == ASYNC_READ, "wrong state");
        // this cast is safe because we set mUserObserver in AsyncRead
        nsIStreamObserver* obs = mUserObserver;
        nsIStreamListener* listener = NS_STATIC_CAST(nsIStreamListener*, obs);
        return listener;
    }

protected:
    nsCOMPtr<nsIURI>                    mOriginalURI;
    nsCOMPtr<nsIURI>                    mResourceURI;
    nsCOMPtr<nsIURI>                    mResolvedURI;
    nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
    char*                               mCommand;
    PRUint32                            mLoadAttributes;
    nsCOMPtr<nsILoadGroup>              mLoadGroup;
    nsCOMPtr<nsISupports>               mOwner;

    nsCOMPtr<nsIResProtocolHandler>     mHandler;
    nsCOMPtr<nsIChannel>                mResolvedChannel;
    State                               mState;
    Substitutions                       mSubstitutions;
    nsCOMPtr<nsIStreamObserver>         mUserObserver;
    nsCOMPtr<nsISupports>               mUserContext;
    nsCOMPtr<nsIInputStream>            mFromStream;
    PRUint32                            mStartPosition;
    PRInt32                             mCount;
    PRUint32                            mBufferSegmentSize;
    PRUint32                            mBufferMaxSize;
#ifdef DEBUG
    PRThread*                           mInitiator;
#endif
};

#endif // nsResChannel_h__
