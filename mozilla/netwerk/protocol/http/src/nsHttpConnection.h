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
#include "nsIEventQueue.h"
#include "nsIInputStream.h"
#include "nsIProxyInfo.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "prclist.h"
#include "nsCRT.h"
#include "prlock.h"

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
    nsresult OnHeadersAvailable(nsHttpTransaction *, PRBool *reset);

    // called by the transaction to inform the connection that it is done.
    nsresult OnTransactionComplete(nsHttpTransaction *, nsresult status);

    // called by the transaction to suspend/resume a read-in-progress
    nsresult Suspend();
    nsresult Resume();

    // called to cause the underlying socket to start speaking SSL
    nsresult ProxyStepUp();

    PRBool   CanReuse(); // can this connection be reused?
    PRBool   IsAlive();
    PRBool   IsKeepAlive()   { return mKeepAlive; }
    PRUint16 IdleTimeout()   { return mIdleTimeout; }

    void     DontReuse()     { mKeepAlive = PR_FALSE;
                               mIdleTimeout = 0; }

    void DropTransaction();
    void ReportProgress(PRUint32 progress, PRInt32 progressMax);

    nsHttpTransaction    *Transaction()    { return mTransaction; }
    nsHttpConnectionInfo *ConnectionInfo() { return mConnectionInfo; }
    nsIEventQueue        *ConsumerEventQ() { return mConsumerEventQ; }

private:
    nsresult ActivateConnection();
    nsresult CreateTransport();

    // proxy the release of the transaction
    nsresult ProxyReleaseTransaction(nsHttpTransaction *);

    nsresult SetupSSLProxyConnect();

private:
    nsCOMPtr<nsISocketTransport>    mSocketTransport;
    nsCOMPtr<nsIRequest>            mWriteRequest;
    nsCOMPtr<nsIRequest>            mReadRequest;

    nsCOMPtr<nsIProgressEventSink>  mProgressSink;
    nsCOMPtr<nsIEventQueue>         mConsumerEventQ;

    nsCOMPtr<nsIInputStream>        mSSLProxyConnectStream;

    nsHttpTransaction              *mTransaction;    // hard ref
    nsHttpConnectionInfo           *mConnectionInfo; // hard ref

    PRLock                         *mLock;

    PRUint32                        mLastActiveTime;
    PRUint16                        mIdleTimeout;   // value of keep-alive: timeout=

    PRPackedBool                    mKeepAlive;
    PRPackedBool                    mWriteDone;
    PRPackedBool                    mReadDone;
};

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo - holds the properties of a connection
//-----------------------------------------------------------------------------

class nsHttpConnectionInfo : public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    nsHttpConnectionInfo(const char *host, PRInt32 port,
                         nsIProxyInfo* proxyInfo,
                         PRBool usingSSL=PR_FALSE)
        : mProxyInfo(proxyInfo)
        , mUsingSSL(usingSSL) 
    {
        LOG(("Creating nsHttpConnectionInfo @%x\n", this));

        NS_INIT_ISUPPORTS();

        mUsingHttpProxy = (proxyInfo && !nsCRT::strcmp(proxyInfo->Type(), "http"));

        SetOriginServer(host, port);
    }

    nsresult SetOriginServer(const char* host, PRInt32 port)
    {
        if (host)
            mHost.Adopt(nsCRT::strdup(host));
        mPort = port == -1 ? DefaultPort() : port;
        
        return NS_OK;
    }
    
    virtual ~nsHttpConnectionInfo()
    {
        LOG(("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    const char *ProxyHost() const { return mProxyInfo ? mProxyInfo->Host() : nsnull; }
    PRInt32     ProxyPort() const { return mProxyInfo ? mProxyInfo->Port() : -1; }
    const char *ProxyType() const { return mProxyInfo ? mProxyInfo->Type() : nsnull; }

    // Compare this connection info to another...
    // Two connections are 'equal' if they end up talking the same
    // protocol to the same server. This is needed to properly manage
    // persistent connections to proxies
    // Note that we don't care about transparent proxies - 
    // it doesn't matter if we're talking via socks or not, since
    // a request will end up at the same host.
    PRBool Equals(const nsHttpConnectionInfo *info)
    {
        // Strictly speaking, we could talk to a proxy on the same port
        // and reuse the connection. Its not worth the extra strcmp.
        if ((info->mUsingHttpProxy != mUsingHttpProxy) ||
            (info->mUsingSSL != mUsingSSL))
            return PR_FALSE;

        // if its a proxy, then compare the proxy servers.
        if (mUsingHttpProxy && !mUsingSSL)
            return (!PL_strcasecmp(info->ProxyHost(), ProxyHost()) &&
                    info->ProxyPort() == ProxyPort());

        // otherwise, just check the hosts
        return (!PL_strcasecmp(info->mHost, mHost) &&
                info->mPort == mPort);

    }

    const char *Host()      { return mHost; }
    PRInt32     Port()      { return mPort; }
    nsIProxyInfo *ProxyInfo() { return mProxyInfo; }
    PRBool      UsingHttpProxy() { return mUsingHttpProxy; }
    PRBool      UsingSSL()  { return mUsingSSL; }

    PRInt32     DefaultPort() { return mUsingSSL ? 443 : 80; }
            
private:
    nsXPIDLCString     mHost;
    PRInt32            mPort;
    nsCOMPtr<nsIProxyInfo> mProxyInfo;
    PRPackedBool       mUsingHttpProxy;
    PRPackedBool       mUsingSSL;
};

#endif // nsHttpConnection_h__
