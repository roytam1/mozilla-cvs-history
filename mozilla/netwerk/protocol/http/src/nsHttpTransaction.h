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

#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsIEventQueue.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"

class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpChunkedDecoder;

//-----------------------------------------------------------------------------
// nsHttpTransaction represents a single HTTP transaction.  It is thread-safe,
// intended to run on the socket thread.
//-----------------------------------------------------------------------------

class nsHttpTransaction : public nsAHttpTransaction
                        , public nsIRequest
                        , public nsIInputStream
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIINPUTSTREAM

    // A transaction is constructed from request headers.
    nsHttpTransaction(nsIStreamListener *, nsIInterfaceRequestor *, PRUint8 caps);
    virtual ~nsHttpTransaction();

    // Called to initialize the transaction
    nsresult SetupRequest(nsHttpRequestHead *requestHeaders,
                          nsIInputStream    *requestBody,
                          PRBool             requestBodyIncludesHeaders,
                          PRBool             pruneProxyHeaders);

    nsIStreamListener     *Listener()       { return mListener; }
    nsAHttpConnection     *Connection()     { return mConnection; }
    nsHttpRequestHead     *RequestHead()    { return mRequestHead; }
    nsHttpResponseHead    *ResponseHead()   { return mHaveAllHeaders ? mResponseHead : nsnull; }
    nsIInterfaceRequestor *Callbacks()      { return mCallbacks; } 
    nsIEventQueue         *ConsumerEventQ() { return mConsumerEventQ; }
    nsISupports           *SecurityInfo()   { return mSecurityInfo; }
    PRUint8                Capabilities()   { return mCapabilities; }

    // Called to take ownership of the response headers; the transaction
    // will drop any reference to the response headers after this call.
    nsHttpResponseHead *TakeResponseHead();

    // Called to find out if the transaction generated a complete response.
    PRBool ResponseIsComplete() { return mResponseIsComplete; }

    // nsAHttpTransaction methods:
    void     SetConnection(nsAHttpConnection *conn) { NS_IF_ADDREF(mConnection = conn); }
    void     SetSecurityInfo(nsISupports *info) { mSecurityInfo = info; }
    void     GetNotificationCallbacks(nsIInterfaceRequestor **cb) { NS_IF_ADDREF(*cb = mCallbacks); }
    PRUint32 GetRequestSize();
    PRUint32 GetContentRead() { return mContentRead; }
    nsresult OnDataWritable(nsIOutputStream *);
    nsresult OnDataReadable(nsIInputStream *);
    nsresult OnStopTransaction(nsresult);
    void     OnStatus(nsresult status, const PRUnichar *statusText);
    PRBool   IsDone() { return mTransactionDone; }
    nsresult Status() { return mStatus; }

private:
    nsresult Restart();
    void     ParseLine(char *line);
    nsresult ParseLineSegment(char *seg, PRUint32 len);
    nsresult ParseHead(char *, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContentStart();
    nsresult HandleContent(char *, PRUint32 count, PRUint32 *contentRead, PRUint32 *contentRemaining);
    void     DeleteSelfOnConsumerThread();

    static void *PR_CALLBACK DeleteThis_EventHandlerFunc(PLEvent *);
    static void  PR_CALLBACK DeleteThis_EventCleanupFunc(PLEvent *);

private:
    nsCOMPtr<nsIStreamListener>     mListener;
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsIProgressEventSink>  mProgressSink;
    nsCOMPtr<nsIEventQueue>         mConsumerEventQ;
    nsCOMPtr<nsISupports>           mSecurityInfo;

    nsAHttpConnection              *mConnection;      // hard ref

    nsCString                       mReqHeaderBuf;    // flattened request headers
    nsCOMPtr<nsIInputStream>        mReqHeaderStream; // header data stream
    nsCOMPtr<nsIInputStream>        mReqUploadStream; // upload data stream
    PRUint32                        mReqUploadStreamOffset;
    PRUint32                        mReqUploadStreamLength;

    nsCOMPtr<nsIInputStream>        mSource;
    nsHttpRequestHead              *mRequestHead;     // weak ref
    nsHttpResponseHead             *mResponseHead;    // hard ref

    nsCString                       mLineBuf;         // may contain a partial line

    PRInt32                         mContentLength;   // equals -1 if unknown
    PRUint32                        mContentRead;     // count of consumed content bytes

    nsHttpChunkedDecoder           *mChunkedDecoder;

    PRInt32                         mTransactionDone; // set atomically
    nsresult                        mStatus;

    PRUint16                        mRestartCount;    // the number of times this transaction has been restarted
    PRUint8                         mCapabilities;

    PRPackedBool                    mHaveStatusLine;
    PRPackedBool                    mHaveAllHeaders;
    PRPackedBool                    mResponseIsComplete;
    PRPackedBool                    mFiredOnStart;
    PRPackedBool                    mNoContent;       // expecting an empty entity body?
    PRPackedBool                    mPrematureEOF;
};

#endif // nsHttpTransaction_h__
