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

#ifndef nsJARChannel_h__
#define nsJARChannel_h__

#include "nsIJARChannel.h"
#include "nsIStreamListener.h"
#include "nsIJARProtocolHandler.h"
#include "nsIJARURI.h"
#include "nsIFileSystem.h"
#include "nsIFileChannel.h"
#include "nsIZipReader.h"
#include "nsIFileChannel.h"
#include "nsCOMPtr.h"

class nsIFileChannel;

#define NS_JARCHANNEL_CID							 \
{ /* 0xc7e410d5-0x85f2-11d3-9f63-006008a6efe9 */     \
    0xc7e410d5,                                      \
    0x85f2,                                          \
    0x11d3,                                          \
    {0x9f, 0x63, 0x00, 0x60, 0x08, 0xa6, 0xef, 0xe9} \
}

#define JAR_DIRECTORY "jarCache"

class nsJARChannel : public nsIJARChannel, 
                     public nsIStreamListener,
                     public nsIFileSystem
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIJARCHANNEL
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIFILESYSTEM

    nsJARChannel();
    virtual ~nsJARChannel();

    // Define a Create method to be used with a factory:
    static NS_METHOD
    Create(nsISupports* aOuter, REFNSIID aIID, void **aResult);

    nsresult Init(nsIJARProtocolHandler* aHandler, const char* verb, nsIURI* uri,
                  nsILoadGroup* aGroup, nsIEventSinkGetter* getter, nsIURI* originalURI);

    nsresult ExtractJARElement(nsIFileChannel* jarFileChannel);
    nsresult GetCacheFile(nsFileSpec& cacheFile);

protected:
	char*                               mCommand;
	nsCOMPtr<nsIJARURI>                 mURI;
	nsCOMPtr<nsILoadGroup>              mLoadGroup;
	nsCOMPtr<nsIEventSinkGetter>        mEventSinkGetter;
	nsCOMPtr<nsIURI>                    mOriginalURI;
    nsLoadFlags                         mLoadAttributes;
    nsCOMPtr<nsISupports>               mOwner;

    PRUint32                            mStartPosition;
    PRInt32                             mReadCount;
    nsCOMPtr<nsISupports>               mUserContext;
    nsCOMPtr<nsIStreamListener>         mUserListener;

    char*                               mContentType;
    PRInt32                             mContentLength;
    nsCOMPtr<nsIURI>                    mJARBaseURI;
    nsCOMPtr<nsIFileChannel>            mJARBaseFile;
    char*                               mJAREntry;
    nsCOMPtr<nsIZipReader>              mJAR;
};

#endif // nsJARChannel_h__
