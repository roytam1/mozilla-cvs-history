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

#ifndef nsHTTPEncodeStream_h__
#define nsHTTPEncodeStream_h__

#include "nsIInputStream.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIFileStreams.h"

class nsHTTPEncodeStream : public nsIInputStream,
                           public nsIRandomAccessStore
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIBASESTREAM
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIRANDOMACCESSSTORE

#if 0
    NS_IMETHOD Seek(PRSeekWhence whence, PRInt32 offset);
    NS_IMETHOD Tell(PRIntn* outWhere);
    // XXX supposedly "protected":
    NS_IMETHOD GetAtEOF(PRBool* outAtEOF);
    NS_IMETHOD SetAtEOF(PRBool inAtEOF);
#endif

    // nsHTTPEncodeStream methods:
    nsHTTPEncodeStream();
    virtual ~nsHTTPEncodeStream();

    static NS_METHOD
    Create(nsIInputStream *rawStream, PRUint32 flags, 
           nsIInputStream **result);

    nsresult Init(nsIInputStream *rawStream, PRUint32 flags);
    nsresult GetData(char* buf, PRUint32 bufLen, PRUint32 *readCount);
    nsresult PushBack(nsString& data);

protected:
    nsCOMPtr<nsIInputStream>    mInput;
    PRUint32                    mFlags;
    nsString                    mPushBackBuffer;
    PRBool                      mLastLineComplete;
};

#endif // nsHTTPEncodeStream_h__
