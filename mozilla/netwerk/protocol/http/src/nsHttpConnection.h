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

#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsHttp.h"
#include "nsIStreamListener.h"
#include "nsIStreamProvider.h"
#include "nsISocketTransport.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "prclist.h"

class nsHttpHandler;
class nsHttpConnectionInfo;
class nsHttpTransaction;

//-----------------------------------------------------------------------------
// nsHttpConnection - represents a connection to a HTTP server (or proxy)
//-----------------------------------------------------------------------------

class nsHttpConnection : public nsIStreamListener
                       , public nsIStreamProvider
                       , public nsIProgressEventSink
                       , public nsIInterfaceRequestor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMPROVIDER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK

    nsHttpConnection();
    virtual ~nsHttpConnection();

    nsresult Init(nsHttpConnectionInfo *);

    // SetTransaction causes the given transaction to be processed on this
    // connection.  It fails if there is already an existing transaction.
    nsresult SetTransaction(nsHttpTransaction *);

    // called by the transaction to inform the connection that all of the
    // headers are available.
    nsresult OnHeadersAvailable(nsHttpTransaction *);

    // called by the transaction to inform the connection that it is done.
    nsresult OnTransactionComplete(nsHttpTransaction *, nsresult status);

    // called by the transaction to resume a read-in-progress
    nsresult Resume();

    PRBool   CanReuse(); // can this connection be reused?
    PRBool   IsAlive();
    PRUint32 ReuseCount()    { return mReuseCount; }
    PRUint32 MaxReuseCount() { return mMaxReuseCount; }
    PRUint32 IdleTimeout()   { return mIdleTimeout; }

    void ReportProgress(PRUint32 progress, PRInt32 progressMax);

    nsHttpTransaction    *Transaction()     { return mTransaction; }
    nsHttpConnectionInfo *ConnectionInfo()  { return mConnectionInfo; }

private:
    enum {
        IDLE,
        WAITING_FOR_WRITE,
        WRITING,
        WAITING_FOR_READ,
        READING
    };

    nsresult ActivateConnection();
    nsresult CreateTransport();

private:
    nsCOMPtr<nsISocketTransport>    mSocketTransport;
    nsCOMPtr<nsIRequest>            mWriteRequest;
    nsCOMPtr<nsIRequest>            mReadRequest;

    nsCOMPtr<nsIProgressEventSink>  mProgressSink;

    nsHttpTransaction              *mTransaction;    // hard ref
    nsHttpConnectionInfo           *mConnectionInfo; // hard ref

    PRUint32                        mState;
    PRUint32                        mReuseCount;
    PRUint32                        mMaxReuseCount; // value of keep-alive: max=
    PRUint32                        mIdleTimeout;   // value of keep-alive: timeout=
    PRUint32                        mLastActiveTime;

    PRPackedBool                    mKeepAlive;
};

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo - holds the properties of a connection
//-----------------------------------------------------------------------------

class nsHttpConnectionInfo : public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    nsHttpConnectionInfo(const char *host, PRInt32 port,
                         const char *proxyHost=0, PRInt32 proxyPort=-1,
                         const char *proxyType=0, PRBool usingSSL=0)
        : mPort(port)
        , mProxyPort(proxyPort)
        , mUsingSSL(usingSSL)
    {
        LOG(("Creating nsHttpConnectionInfo @%x\n", this));

        NS_INIT_ISUPPORTS();

        mHost = host;
        mProxyHost = proxyHost;
        mProxyType = proxyType;
    }
    
    virtual ~nsHttpConnectionInfo()
    {
        LOG(("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    // Compare this connection info to another...
    PRBool Equals(const nsHttpConnectionInfo *info)
    {
        return !PL_strcasecmp(info->mHost, mHost) &&
               !PL_strcasecmp(info->mProxyHost, mProxyHost) &&
               !PL_strcasecmp(info->mProxyType, mProxyType) &&
                info->mPort == mPort &&
                info->mProxyPort == mProxyPort &&
                info->mUsingSSL == mUsingSSL;
    }

    const char *Host()      { return mHost; }
    PRInt32     Port()      { return mPort; }
    const char *ProxyHost() { return mProxyHost; }
    PRInt32     ProxyPort() { return mProxyPort; }
    const char *ProxyType() { return mProxyType; }
    PRBool      UsingSSL()  { return mUsingSSL; }

private:
    nsXPIDLCString     mHost;
    PRInt32            mPort;
    nsXPIDLCString     mProxyHost;
    PRInt32            mProxyPort;
    nsXPIDLCString     mProxyType;
    PRPackedBool       mUsingSSL;
};

#endif // nsHttpConnection_h__
