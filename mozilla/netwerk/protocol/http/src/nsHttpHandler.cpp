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
 *   Gagan Saksena <gagan@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Adrian Havill <havill@redhat.com>
 *   Gervase Markham <gerv@gerv.net>
 *   Bradley Baetz <bbaetz@netscape.com>
 */

#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpChannel.h"
#include "nsHttpConnection.h"
#include "nsHttpResponseHead.h"
#include "nsHttpTransaction.h"
#include "nsHttpAuthCache.h"
#include "nsIHttpChannel.h"
#include "nsIHttpNotify.h"
#include "nsIURL.h"
#include "nsICacheService.h"
#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsINetModRegEntry.h"
#include "nsICacheService.h"
#include "nsPrintfCString.h"
#include "nsCOMPtr.h"
#include "nsNetCID.h"
#include "nsAutoLock.h"
#include "prprf.h"
#include "nsReadableUtils.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <sys/utsname.h>
#endif

#if defined(XP_PC) && !defined(XP_OS2)
#include <windows.h>
#endif

static const char NETWORK_PREFS[] = "network.";
static const char INTL_ACCEPT_LANGUAGES[] = "intl.accept_languages";
static const char INTL_ACCEPT_CHARSET[] = "intl.charset.default";

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);
static NS_DEFINE_CID(kNetModuleMgrCID, NS_NETMODULEMGR_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kCacheServiceCID, NS_CACHESERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kMimeServiceCID, NS_MIMESERVICE_CID);

#define UA_PREF_PREFIX "general.useragent."
#define UA_APPNAME "Mozilla"
#define UA_APPVERSION "5.0"
#define UA_APPSECURITY_FALLBACK "N"

//-----------------------------------------------------------------------------
// nsHttpHandler <public>
//-----------------------------------------------------------------------------

nsHttpHandler *nsHttpHandler::mGlobalInstance = 0;

nsHttpHandler::nsHttpHandler()
    : mAuthCache(nsnull)
    , mHttpVersion(NS_HTTP_VERSION_1_1)
    , mReferrerLevel(0xff) // by default we always send a referrer
    , mCapabilities(NS_HTTP_ALLOW_KEEPALIVE)
    , mProxyCapabilities(NS_HTTP_ALLOW_KEEPALIVE)
    , mIdleTimeout(10)
    , mMaxRequestAttempts(10)
    , mMaxConnections(16)
    , mMaxConnectionsPerServer(8)
    , mMaxIdleConnectionsPerServer(4)
    , mActiveConnections(0)
    , mIdleConnections(0)
    , mTransactionQ(0)
    , mUserAgentIsDirty(PR_TRUE)
{
    NS_INIT_ISUPPORTS();

#if defined(PR_LOGGING)
    gHttpLog = PR_NewLogModule("nsHttp");
#endif

    LOG(("Creating nsHttpHandler [this=%x].\n", this));

    NS_ASSERTION(!mGlobalInstance, "HTTP handler already created!");
    mGlobalInstance = this;
}

nsHttpHandler::~nsHttpHandler()
{
    LOG(("Deleting nsHttpHandler [this=%x]\n", this));

    nsHttp::DestroyAtomTable();

    if (mPrefs) {
        mPrefs->UnregisterCallback(NETWORK_PREFS, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(INTL_ACCEPT_LANGUAGES, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "override", 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(INTL_ACCEPT_CHARSET, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "locale", 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "misc",
                nsHttpHandler::PrefsCallback, (void *)this);
    }

    LOG(("dropping active connections...\n"));
    DropConnections(mActiveConnections);

    LOG(("dropping idle connections...\n"));
    DropConnections(mIdleConnections);

    if (mAuthCache) {
        delete mAuthCache;
        mAuthCache = nsnull;
    }

    if (mConnectionLock) {
        PR_DestroyLock(mConnectionLock);
        mConnectionLock = nsnull;
    }

    mGlobalInstance = nsnull;
}

NS_METHOD
nsHttpHandler::Create(nsISupports *outer, REFNSIID iid, void **result)
{
    nsresult rv;
    if (outer)
        return NS_ERROR_NO_AGGREGATION;
    nsHttpHandler *handler = get();
    if (!handler) {
        // create the one any only instance of nsHttpHandler
        NS_NEWXPCOM(handler, nsHttpHandler);
        if (!handler)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(handler);
        rv = handler->Init();
        if (NS_FAILED(rv)) {
            LOG(("nsHttpHandler::Init failed [rv=%x]\n", rv));
            NS_RELEASE(handler);
            return rv;
        }
    }
    else
        NS_ADDREF(handler);
    rv = handler->QueryInterface(iid, result);
    NS_RELEASE(handler);
    return rv;
}

nsresult
nsHttpHandler::Init()
{
    nsresult rv = NS_OK;

    LOG(("nsHttpHandler::Init\n"));

    mIOService = do_GetService(kIOServiceCID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without io service");
        return rv;
    }

    mConnectionLock = PR_NewLock();
    if (!mConnectionLock)
        return NS_ERROR_OUT_OF_MEMORY;

    mPrefs = do_GetService(kPrefServiceCID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without prefs service");
        return rv;
    }

    InitUserAgentComponents();

    mPrefs->RegisterCallback(NETWORK_PREFS, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(INTL_ACCEPT_LANGUAGES, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "override", 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(INTL_ACCEPT_CHARSET, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "locale", 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "misc",
            nsHttpHandler::PrefsCallback, (void *)this);

    PrefsChanged();

    mSessionStartTime = NowInSeconds();

    mAuthCache = new nsHttpAuthCache();
    if (!mAuthCache)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = mAuthCache->Init();
    if (NS_FAILED(rv)) return rv;

    // Startup the http category
    // Bring alive the objects in the http-protocol-startup category
    NS_CreateServicesFromCategory(NS_HTTP_STARTUP_CATEGORY,
                                  NS_STATIC_CAST(nsISupports*,NS_STATIC_CAST(void*,this)),
                                  NS_HTTP_STARTUP_TOPIC);
    
    nsCOMPtr<nsIObserverService> observerSvc =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (observerSvc)
        observerSvc->AddObserver(this, NS_LITERAL_STRING("profile-before-change").get());

    return NS_OK;
}

nsresult
nsHttpHandler::AddStandardRequestHeaders(nsHttpHeaderArray *request,
                                         PRUint8 caps,
                                         PRBool useProxy)
{
    nsresult rv;

    LOG(("nsHttpHandler::AddStandardRequestHeaders\n"));

    // Add the "User-Agent" header
    rv = request->SetHeader(nsHttp::User_Agent, UserAgent());
    if (NS_FAILED(rv)) return rv;

    // MIME based content negotiation lives!
    // Add the "Accept" header
    rv = request->SetHeader(nsHttp::Accept, mAccept.get());
    if (NS_FAILED(rv)) return rv;

    // Add the "Accept-Language" header
    if (!mAcceptLanguages.IsEmpty()) {
        // Add the "Accept-Language" header
        rv = request->SetHeader(nsHttp::Accept_Language, mAcceptLanguages.get());
        if (NS_FAILED(rv)) return rv;
    }

    // Add the "Accept-Encoding" header
    rv = request->SetHeader(nsHttp::Accept_Encoding, mAcceptEncodings.get());
    if (NS_FAILED(rv)) return rv;

    // Add the "Accept-Charset" header
    rv = request->SetHeader(nsHttp::Accept_Charset, mAcceptCharsets.get());
    if (NS_FAILED(rv)) return rv;

    // RFC2616 section 19.6.2 states that the "Connection: keep-alive"
    // and "Keep-alive" request headers should not be sent by HTTP/1.1
    // user-agents.  Otherwise, problems with proxy servers (especially
    // transparent proxies) can result.
    //
    // However, we need to send something so that we can use keepalive
    // with HTTP/1.0 servers/proxies. We use "Proxy-Connection:" when 
    // we're talking to an http proxy, and "Connection:" otherwise
    
    const char* connectionType = "close";
    if (caps & NS_HTTP_ALLOW_KEEPALIVE) {
        char buf[32];
        
        PR_snprintf(buf, sizeof(buf), "%u", (PRUintn) mIdleTimeout);
        
        rv = request->SetHeader(nsHttp::Keep_Alive, buf);
        if (NS_FAILED(rv)) return rv;
        
        connectionType = "keep-alive";
    } else if (useProxy) {
        // Bug 92006
        request->SetHeader(nsHttp::Connection, "close");
    }

    const nsHttpAtom &header =
        useProxy ? nsHttp::Proxy_Connection : nsHttp::Connection;
    return request->SetHeader(header, connectionType);
}

PRBool
nsHttpHandler::IsAcceptableEncoding(const char *enc)
{
    if (!enc)
        return PR_FALSE;

    // HTTP 1.1 allows servers to send x-gzip and x-compress instead
    // of gzip and compress, for example.  So, we'll always strip off
    // an "x-" prefix before matching the encoding to one we claim
    // to accept.
    if (!PL_strncasecmp(enc, "x-", 2))
        enc += 2;
    
    return PL_strcasestr(mAcceptEncodings, enc) != nsnull;
}

nsresult
nsHttpHandler::GetCacheSession(nsCacheStoragePolicy storagePolicy,
                               nsICacheSession **result)
{
    static PRBool checkedPref = PR_FALSE;
    static PRBool useCache = PR_TRUE;
    nsresult rv;

    if (!checkedPref) {
        // XXX should register a prefs changed callback for this
        nsCOMPtr<nsIPref> prefs = do_GetService(kPrefServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        prefs->GetBoolPref("browser.cache.enable", &useCache);

        checkedPref = PR_TRUE;
    }

    // Skip cache if disabled in preferences
    if (!useCache)
        return NS_ERROR_NOT_AVAILABLE;

    if (!mCacheSession_ANY) {
        nsCOMPtr<nsICacheService> serv = do_GetService(kCacheServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = serv->CreateSession("HTTP",
                                 nsICache::STORE_ANYWHERE,
                                 nsICache::STREAM_BASED,
                                 getter_AddRefs(mCacheSession_ANY));
        if (NS_FAILED(rv)) return rv;

        rv = mCacheSession_ANY->SetDoomEntriesIfExpired(PR_FALSE);
        if (NS_FAILED(rv)) return rv;

        rv = serv->CreateSession("HTTP-memory-only",
                                 nsICache::STORE_IN_MEMORY,
                                 nsICache::STREAM_BASED,
                                 getter_AddRefs(mCacheSession_MEM));
        if (NS_FAILED(rv)) return rv;

        rv = mCacheSession_MEM->SetDoomEntriesIfExpired(PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    if (storagePolicy == nsICache::STORE_IN_MEMORY)
        NS_ADDREF(*result = mCacheSession_MEM);
    else
        NS_ADDREF(*result = mCacheSession_ANY);

    return NS_OK;
}

// may be called from any thread
nsresult
nsHttpHandler::InitiateTransaction(nsHttpTransaction *trans,
                                   nsHttpConnectionInfo *ci,
                                   PRBool failIfBusy)
{
    LOG(("nsHttpHandler::InitiateTransaction\n"));

    NS_ENSURE_ARG_POINTER(trans);
    NS_ENSURE_ARG_POINTER(ci);

    nsAutoLock lock(mConnectionLock);

    return InitiateTransaction_Locked(trans, ci, failIfBusy);
}

// may be called from any thread
nsresult
nsHttpHandler::ReclaimConnection(nsHttpConnection *conn)
{
    NS_ENSURE_ARG_POINTER(conn);

    PRBool reusable = conn->CanReuse();

    LOG(("nsHttpHandler::ReclaimConnection [conn=%x keep-alive=%d]\n",
        conn, reusable));

    nsAutoLock lock(mConnectionLock);

    // remove connection from the active connection list
    mActiveConnections.RemoveElement(conn);

    if (reusable) {
        // verify that we aren't already maxed out on the number of
        // keep-alives we can have for this server.
        PRUint32 count = CountIdleConnections(conn->ConnectionInfo());
        if (count == PRUint32(mMaxIdleConnectionsPerServer)) {
            LOG(("not caching keep-alive connection: "
                 "would exceed max allowed per server\n"));
            NS_RELEASE(conn);
        }
        else {
            LOG(("adding connection to idle list [conn=%x]\n", conn));
            // hold onto this connection in the idle list.  we push it
            // to the end of the list so as to ensure that we'll visit
            // older connections first before getting to this one.
            mIdleConnections.AppendElement(conn);
        }
    }
    else {
        LOG(("closing connection: connection can't be reused\n"));
        NS_RELEASE(conn);
    }

    LOG(("active connection count is now %u\n", mActiveConnections.Count()));

    // process the pending transaction queue...
    if (mTransactionQ.Count() > 0)
        ProcessTransactionQ();

    return NS_OK;
}

// called from any thread, by the implementation of nsHttpTransaction::Cancel
nsresult
nsHttpHandler::CancelTransaction(nsHttpTransaction *trans, nsresult status)
{
    nsHttpConnection *conn;

    LOG(("nsHttpHandler::CancelTransaction [trans=%x status=%x]\n",
        trans, status));

    NS_ENSURE_ARG_POINTER(trans);

    // we need to be inside the connection lock in order to know whether
    // or not this transaction has an associated connection.  otherwise,
    // we'd have a race condition (see bug 85822).
    {
        nsAutoLock lock(mConnectionLock);

        conn = trans->Connection();
        if (conn)
            NS_ADDREF(conn); // make sure the connection stays around.
        else
            RemovePendingTransaction(trans);
    }

    if (conn) {
        conn->OnTransactionComplete(trans, status);
        NS_RELEASE(conn);
    }
    else
        trans->OnStopTransaction(status);

    return NS_OK;
}

nsresult
nsHttpHandler::GetProxyObjectManager(nsIProxyObjectManager **result)
{
    if (!mProxyMgr) {
        nsresult rv;
        mProxyMgr = do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    *result = mProxyMgr;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsHttpHandler::GetEventQueueService(nsIEventQueueService **result)
{
    if (!mEventQueueService) {
        nsresult rv;
        mEventQueueService = do_GetService(kEventQueueServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    *result = mEventQueueService;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsHttpHandler::GetStreamConverterService(nsIStreamConverterService **result)
{
    if (!mStreamConvSvc) {
        nsresult rv;
        mStreamConvSvc = do_GetService(kStreamConverterServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    *result = mStreamConvSvc;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult
nsHttpHandler::GetMimeService(nsIMIMEService **result)
{
    if (!mMimeService) {
        nsresult rv;
        mMimeService = do_GetService(kMimeServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    *result = mMimeService;
    NS_ADDREF(*result);
    return NS_OK;
}

nsresult 
nsHttpHandler::GetIOService(nsIIOService** result)
{
    NS_ADDREF(*result = mIOService);
    return NS_OK;
}


nsresult
nsHttpHandler::OnModifyRequest(nsIHttpChannel *chan)
{
    nsresult rv;

    LOG(("nsHttpHandler::OnModifyRequest [chan=%x]\n", chan));

    if (!mNetModuleMgr) {
        mNetModuleMgr = do_GetService(kNetModuleMgrCID, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsISimpleEnumerator> modules;
    rv = mNetModuleMgr->EnumerateModules(
            NS_NETWORK_MODULE_MANAGER_HTTP_REQUEST_CONTRACTID,
            getter_AddRefs(modules));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> sup;

    // notify each module...
    while (NS_SUCCEEDED(modules->GetNext(getter_AddRefs(sup)))) {
        nsCOMPtr<nsINetModRegEntry> entry = do_QueryInterface(sup, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsINetNotify> netNotify;
        rv = entry->GetSyncProxy(getter_AddRefs(netNotify));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIHttpNotify> httpNotify = do_QueryInterface(netNotify, &rv);
        if (NS_FAILED(rv)) return rv;

        // fire off the notification, ignore the return code.
        httpNotify->OnModifyRequest(chan);
    }
    
    return NS_OK;
}

nsresult
nsHttpHandler::OnExamineResponse(nsIHttpChannel *chan)
{
    nsresult rv;

    LOG(("nsHttpHandler::OnExamineResponse [chan=%x]\n", chan));

    if (!mNetModuleMgr) {
        mNetModuleMgr = do_GetService(kNetModuleMgrCID, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsISimpleEnumerator> modules;
    rv = mNetModuleMgr->EnumerateModules(
            NS_NETWORK_MODULE_MANAGER_HTTP_RESPONSE_CONTRACTID,
            getter_AddRefs(modules));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> sup;
    nsCOMPtr<nsINetModRegEntry> entry;
    nsCOMPtr<nsINetNotify> netNotify;
    nsCOMPtr<nsIHttpNotify> httpNotify;

    // notify each module...
    while (NS_SUCCEEDED(modules->GetNext(getter_AddRefs(sup)))) {
        entry = do_QueryInterface(sup, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = entry->GetSyncProxy(getter_AddRefs(netNotify));
        if (NS_FAILED(rv)) return rv;

        httpNotify = do_QueryInterface(netNotify, &rv);
        if (NS_FAILED(rv)) return rv;

        // fire off the notification, ignore the return code.
        httpNotify->OnExamineResponse(chan);
    }
    
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler <private>
//-----------------------------------------------------------------------------

const char *
nsHttpHandler::UserAgent()
{
    if (mUserAgentOverride)
        return mUserAgentOverride.get();

    if (mUserAgentIsDirty) {
        BuildUserAgent();
        mUserAgentIsDirty = PR_FALSE;
    }

    return mUserAgent.get();
}

// called with the connection lock held
void
nsHttpHandler::ProcessTransactionQ()
{
    LOG(("nsHttpHandler::ProcessTransactionQ\n"));

    nsPendingTransaction *pt = nsnull;

    PRInt32 i;
    for (i=0; (i < mTransactionQ.Count()) &&
              (mActiveConnections.Count() < PRInt32(mMaxConnections)); ++i) {

        pt = (nsPendingTransaction *) mTransactionQ[i];

        // try to initiate this transaction... if it fails
        // then we'll just skip over this pending transaction
        // and try the next.
        nsresult rv = InitiateTransaction_Locked(pt->Transaction(),
                                                 pt->ConnectionInfo(),
                                                 PR_TRUE);
        if (NS_SUCCEEDED(rv)) {
            mTransactionQ.RemoveElementAt(i);
            delete pt;
            i--;
        }
    }
}

// called with the connection lock held
nsresult
nsHttpHandler::EnqueueTransaction(nsHttpTransaction *trans,
                                  nsHttpConnectionInfo *ci)
{
    LOG(("nsHttpHandler::EnqueueTransaction [trans=%x]\n", trans));

    nsPendingTransaction *pt = new nsPendingTransaction(trans, ci);
    if (!pt)
        return NS_ERROR_OUT_OF_MEMORY;

    mTransactionQ.AppendElement(pt);

    LOG(("transaction queue contains %u elements\n", mTransactionQ.Count()));
    return NS_OK;
}

// called with the connection lock held
nsresult
nsHttpHandler::InitiateTransaction_Locked(nsHttpTransaction *trans,
                                          nsHttpConnectionInfo *ci,
                                          PRBool failIfBusy)
{
    nsresult rv;

    LOG(("nsHttpHandler::InitiateTransaction_Locked [failIfBusy=%d]\n", failIfBusy));

    if ((mActiveConnections.Count() == PRInt32(mMaxConnections)) || 
        (CountActiveConnections(ci) == mMaxConnectionsPerServer)) {
        LOG(("unable to perform the transaction at this time [trans=%x]\n", trans));
        if (failIfBusy) return NS_ERROR_FAILURE;
        return EnqueueTransaction(trans, ci);
    }

    nsHttpConnection *conn = nsnull;

    // search the idle connection list
    PRInt32 i;
    for (i=0; i<mIdleConnections.Count(); ++i) {
        conn = (nsHttpConnection *) mIdleConnections[i];

        LOG(("comparing against idle connection [host=%s:%d]\n",
            conn->ConnectionInfo()->Host(), conn->ConnectionInfo()->Port()));

        // we check if the connection can be reused before even checking if it
        // is a "matching" connection.  this is how we keep the idle connection
        // list fresh.  we could alternatively use some sort of timer for this.
        if (!conn->CanReuse()) {
            LOG(("dropping stale connection: [conn=%x]\n", conn));
            mIdleConnections.RemoveElementAt(i);
            i--;
            NS_RELEASE(conn);
        }
        else if (conn->ConnectionInfo()->Equals(ci)) {
            LOG(("reusing connection [conn=%x]\n", conn));
            mIdleConnections.RemoveElementAt(i);
            i--;
            break;
        }
        conn = nsnull;
    }

    if (!conn) {
        LOG(("creating new connection...\n"));
        NS_NEWXPCOM(conn, nsHttpConnection);
        if (!conn)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(conn);

        rv = conn->Init(ci);
        if (NS_FAILED(rv)) {
            NS_RELEASE(conn);
            return rv;
        }
    } else {
        // Update the connectionInfo (bug 94038)
        conn->ConnectionInfo()->SetOriginServer(ci->Host(), ci->Port());
    }

    // assign the connection to the transaction.
    trans->SetConnection(conn);

    // consider this connection active, even though it may fail.
    mActiveConnections.AppendElement(conn);
    
    // we must not hold the connection lock while making this call
    // as it could lead to deadlocks.
    PR_Unlock(mConnectionLock);
    rv = conn->SetTransaction(trans);
    PR_Lock(mConnectionLock);

    if (NS_FAILED(rv)) {
        // the connection may already have been removed from the 
        // active connection list.
        if (mActiveConnections.RemoveElement(conn))
            NS_RELEASE(conn);
    }

    return rv;
}

// called with the connection lock held
nsresult
nsHttpHandler::RemovePendingTransaction(nsHttpTransaction *trans)
{
    LOG(("nsHttpHandler::RemovePendingTransaction [trans=%x]\n", trans));

    NS_ENSURE_ARG_POINTER(trans);

    nsPendingTransaction *pt = nsnull;
    PRInt32 i;
    for (i=0; i<mTransactionQ.Count(); ++i) {
        pt = (nsPendingTransaction *) mTransactionQ[i];

        if (pt->Transaction() == trans) {
            mTransactionQ.RemoveElementAt(i);
            delete pt;
            return NS_OK;
        }
    }

    NS_WARNING("transaction not in pending queue");
    return NS_ERROR_NOT_AVAILABLE;
}

PRUint8
nsHttpHandler::CountActiveConnections(nsHttpConnectionInfo *ci)
{
    PRUint8 count = 0;
    nsHttpConnection *conn = 0;

    LOG(("nsHttpHandler::CountActiveConnections [host=%s:%d]\n",
        ci->Host(), ci->Port()));

    PRInt32 i;
    for (i=0; i<mActiveConnections.Count(); ++i) {
        conn = NS_STATIC_CAST(nsHttpConnection *, mActiveConnections[i]); 
        // only include a matching connection in the count...
        if (conn->ConnectionInfo()->Equals(ci))
            count++;
    }

    LOG(("found count=%u\n", (PRUintn) count));
    return count;
}

PRUint8
nsHttpHandler::CountIdleConnections(nsHttpConnectionInfo *ci)
{
    PRUint8 count = 0;
    nsHttpConnection *conn = 0;

    if (!ci)
        return 0;

    LOG(("nsHttpHandler::CountIdleConnections [host=%s:%d]\n",
        ci->Host(), ci->Port()));

    PRInt32 i;
    for (i=0; i<mIdleConnections.Count(); ++i) {
        conn = NS_STATIC_CAST(nsHttpConnection *, mIdleConnections[i]);
        // only include a matching connection in the count if it
        // can still be reused.
        if (conn->ConnectionInfo()->Equals(ci)) {
            if (conn->CanReuse())
                count++;
            else {
                mIdleConnections.RemoveElementAt(i);
                NS_RELEASE(conn);
                i--;
            }
        }
    }

    LOG(("found count=%u\n", (PRUintn) count));
    return count;
}

void
nsHttpHandler::DropConnections(nsVoidArray &connections)
{
    nsHttpConnection *conn;
    PRInt32 i;
    for (i=0; i<connections.Count(); ++i) {
        conn = (nsHttpConnection *) connections[i];
        NS_RELEASE(conn);
    }
    connections.Clear();
}

void
nsHttpHandler::BuildUserAgent()
{
    LOG(("nsHttpHandler::BuildUserAgent\n"));

    NS_ASSERTION(mAppName &&
                 mAppVersion &&
                 mPlatform &&
                 mSecurity &&
                 mOscpu,
                 "HTTP cannot send practical requests without this much");

    // Application portion
    mUserAgent.Assign(mAppName);
    mUserAgent += '/';
    mUserAgent += mAppVersion;
    mUserAgent += ' ';

    // Application comment
    mUserAgent += '(';
    mUserAgent += mPlatform;
    mUserAgent += "; ";
    mUserAgent += mSecurity;
    mUserAgent += "; ";
    mUserAgent += mOscpu;
    if (mLanguage) {
        mUserAgent += "; ";
        mUserAgent += mLanguage;
    }
    if (mMisc) {
        mUserAgent += "; ";
        mUserAgent += mMisc;
    }
    mUserAgent += ')';

    // Product portion
    if (mProduct) {
        mUserAgent += ' ';
        mUserAgent += mProduct;
        if (mProductSub) {
            mUserAgent += '/';
            mUserAgent += mProductSub;
        }
        if (mProductComment) {
            mUserAgent += " (";
            mUserAgent += mProductComment;
            mUserAgent += ')';
        }
    }

    // Vendor portion
    if (mVendor) {
        mUserAgent += ' ';
        mUserAgent += mVendor;
        if (mVendorSub) {
            mUserAgent += '/';
            mUserAgent += mVendorSub;
        }
        if (mVendorComment) {
            mUserAgent += " (";
            mUserAgent += mVendorComment;
            mUserAgent += ')';
        }
    }
}

void
nsHttpHandler::InitUserAgentComponents()
{
    // User-specified override
    mPrefs->CopyCharPref(UA_PREF_PREFIX "override",
        getter_Copies(mUserAgentOverride));

    // Gather vendor values.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendor",
        getter_Copies(mVendor));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendorSub",
        getter_Copies(mVendorSub));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendorComment",
        getter_Copies(mVendorComment));

    // Gather product values.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "product",
        getter_Copies(mProduct));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "productSub",
        getter_Copies(mProductSub));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "productComment",
        getter_Copies(mProductComment));

    // Gather misc value.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "misc",
        getter_Copies(mMisc));

    // Gather Application name and Version.
    mAppName.Adopt(nsCRT::strdup(UA_APPNAME));
    mAppVersion.Adopt(nsCRT::strdup(UA_APPVERSION));

    // Get Security level supported
    mPrefs->CopyCharPref(UA_PREF_PREFIX "security",
        getter_Copies(mSecurity));
    if (!mSecurity)
        mSecurity.Adopt(nsCRT::strdup(UA_APPSECURITY_FALLBACK));

    // Gather locale.
    nsXPIDLString uval;
    mPrefs->GetLocalizedUnicharPref(UA_PREF_PREFIX "locale", 
        getter_Copies(uval));
    if (uval)
        mLanguage.Adopt(ToNewUTF8String(nsDependentString(uval)));

      // Gather platform.
    mPlatform.Adopt(nsCRT::strdup(
#if defined(XP_OS2)
    "OS/2"
#elif defined(XP_PC)
    "Windows"
#elif defined(RHAPSODY)
    "Macintosh"
#elif defined (XP_UNIX)
    "X11"
#elif defined(XP_BEOS)
    "BeOS"
#elif defined(XP_MAC)
    "Macintosh"
#endif
    ));

    // Gather OS/CPU.
#if defined(XP_OS2)
    ULONG os2ver = 0;
    DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR,
                    &os2ver, sizeof(os2ver));
    if (os2ver == 11)
        mOscpu.Adopt(nsCRT::strdup("2.11"));
    else if (os2ver == 30)
        mOscpu.Adopt(nsCRT::strdup("Warp 3"));
    else if (os2ver == 40)
        mOscpu.Adopt(nsCRT::strdup("Warp 4"));
    else if (os2ver == 45)
        mOscpu.Adopt(nsCRT::strdup("Warp 4.5"));

#elif defined(XP_PC)
    OSVERSIONINFO info = { sizeof OSVERSIONINFO };
    if (GetVersionEx(&info)) {
        if (info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (info.dwMajorVersion      == 3)
                mOscpu.Adopt(nsCRT::strdup("WinNT3.51"));
            else if (info.dwMajorVersion == 4)
                mOscpu.Adopt(nsCRT::strdup("WinNT4.0"));
            else {
                char *buf = PR_smprintf("Windows NT %ld.%ld",
                                        info.dwMajorVersion,
                                        info.dwMinorVersion);
                if (buf) {
                    mOscpu.Adopt(nsCRT::strdup(buf));
                    PR_smprintf_free(buf);
                }
            }
        } else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
                   info.dwMajorVersion == 4) {
            if (info.dwMinorVersion == 90)
                mOscpu.Adopt(nsCRT::strdup("Win 9x 4.90"));  // Windows Me
            else if (info.dwMinorVersion > 0)
                mOscpu.Adopt(nsCRT::strdup("Win98"));
            else
                mOscpu.Adopt(nsCRT::strdup("Win95"));
        } else {
            char *buf = PR_smprintf("Windows %ld.%ld",
                                    info.dwMajorVersion,
                                    info.dwMinorVersion);
            if (buf) {
                mOscpu.Adopt(nsCRT::strdup(buf));
                PR_smprintf_free(buf);
            }
        }
    }
#elif defined (XP_UNIX) || defined (XP_BEOS)
    struct utsname name;
    
    int ret = uname(&name);
    if (ret >= 0) {
        nsCString buf;  
        buf =  (char*)name.sysname;
        buf += ' ';
        buf += (char*)name.machine;
        mOscpu.Adopt(ToNewCString(buf));
    }
#elif defined (XP_MAC)
    mOscpu.Adopt(nsCRT::strdup("PPC"));
#endif

    mUserAgentIsDirty = PR_TRUE;
}

void
nsHttpHandler::PrefsChanged(const char *pref)
{
    PRBool bChangedAll = pref ? PR_FALSE : PR_TRUE;

    if (!mPrefs) {
        NS_NOTREACHED("No preference service available!");
        return;
    }

    nsresult rv = NS_OK;
    PRInt32 val;

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.timeout") == 0) {
        rv = mPrefs->GetIntPref("network.http.keep-alive.timeout", &val);
        if (NS_SUCCEEDED(rv))
            mIdleTimeout = (PRUint16) CLAMP(val, 1, 0xffff);
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.request.max-attempts") == 0) {
        rv = mPrefs->GetIntPref("network.http.request.max-attempts", &val);
        if (NS_SUCCEEDED(rv))
            mMaxRequestAttempts = (PRUint16) CLAMP(val, 1, 0xffff);
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.max-connections") == 0) {
        rv = mPrefs->GetIntPref("network.http.max-connections", &val);
        if (NS_SUCCEEDED(rv))
            mMaxConnections = (PRUint16) CLAMP(val, 1, 0xffff);
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.max-connections-per-server") == 0) {
        rv = mPrefs->GetIntPref("network.http.max-connections-per-server", &val);
        if (NS_SUCCEEDED(rv))
            mMaxConnectionsPerServer = (PRUint8) CLAMP(val, 1, 0xff);
    }

    /*
    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.max-connections") == 0) {
        rv = mPrefs->GetIntPref("network.http.keep-alive.max-connections", &val);
        if (NS_SUCCEEDED(rv))
            mMaxIdleConnections = (PRUint16) CLAMP(val, 1, 0xffff);
    }
    */

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.max-connections-per-server") == 0) {
        rv = mPrefs->GetIntPref("network.http.keep-alive.max-connections-per-server", &val);
        if (NS_SUCCEEDED(rv))
            mMaxIdleConnectionsPerServer = (PRUint8) CLAMP(val, 1, 0xff);
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.sendRefererHeader") == 0) {
        rv = mPrefs->GetIntPref("network.http.sendRefererHeader", (PRInt32 *) &val);
        if (NS_SUCCEEDED(rv))
            mReferrerLevel = (PRUint8) CLAMP(val, 0, 0xff);
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.version") == 0) {
        nsXPIDLCString httpVersion;
        mPrefs->CopyCharPref("network.http.version", getter_Copies(httpVersion));
	
        if (httpVersion) {
            if (PL_strcmp(httpVersion, "1.1") == 0)
                mHttpVersion = NS_HTTP_VERSION_1_1;
            else if (PL_strcmp(httpVersion, "0.9") == 0)
                mHttpVersion = NS_HTTP_VERSION_0_9;
            else
                mHttpVersion = NS_HTTP_VERSION_1_0;
        }

        if (mHttpVersion == NS_HTTP_VERSION_1_1) {
            mCapabilities = NS_HTTP_ALLOW_KEEPALIVE;
            mProxyCapabilities = NS_HTTP_ALLOW_KEEPALIVE;
        }
        else {
            mCapabilities = 0;
            mProxyCapabilities = 0;
        }
    }

    PRBool cVar = PR_FALSE;

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive") == 0) {
        rv = mPrefs->GetBoolPref("network.http.keep-alive", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |= NS_HTTP_ALLOW_KEEPALIVE;
            else
                mCapabilities &= ~NS_HTTP_ALLOW_KEEPALIVE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.keep-alive") == 0) {
        rv = mPrefs->GetBoolPref("network.http.proxy.keep-alive", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |= NS_HTTP_ALLOW_KEEPALIVE;
            else
                mProxyCapabilities &= ~NS_HTTP_ALLOW_KEEPALIVE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.pipelining") == 0) {
        rv = mPrefs->GetBoolPref("network.http.pipelining", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |=  NS_HTTP_ALLOW_PIPELINING;
            else
                mCapabilities &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    /*
    mPipelineFirstRequest = PR_FALSE;
    rv = mPrefs->GetBoolPref("network.http.pipelining.firstrequest", &mPipelineFirstRequest);

    mPipelineMaxRequests  = DEFAULT_PIPELINE_MAX_REQUESTS;
    rv = mPrefs->GetIntPref("network.http.pipelining.maxrequests", &mPipelineMaxRequests );
    */

    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.pipelining") == 0) {
        rv = mPrefs->GetBoolPref("network.http.proxy.pipelining", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |=  NS_HTTP_ALLOW_PIPELINING;
            else
                mProxyCapabilities &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    /*
    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.ssl.connect") == 0) {
        rv = mPrefs->GetBoolPref("network.http.proxy.ssl.connect", &cVar);
        if (NS_SUCCEEDED(rv))
            mProxySSLConnectAllowed = (cVar != 0);
    }
    */

    /*
    if (bChangedAll || PL_strcmp(pref, "network.http.connect.timeout") == 0)
        mPrefs->GetIntPref("network.http.connect.timeout", &mConnectTimeout);

    if (bChangedAll || PL_strcmp(pref, "network.http.request.timeout") == 0)
        mPrefs->GetIntPref("network.http.request.timeout", &mRequestTimeout);
    */

    if (bChangedAll || PL_strcmp(pref, INTL_ACCEPT_LANGUAGES) == 0) {
        nsXPIDLString acceptLanguages;
        rv = mPrefs->GetLocalizedUnicharPref(INTL_ACCEPT_LANGUAGES, 
                getter_Copies(acceptLanguages));
        if (NS_SUCCEEDED(rv))
            SetAcceptLanguages(NS_ConvertUCS2toUTF8(acceptLanguages).get());
    }

    if (bChangedAll || PL_strcmp(pref, INTL_ACCEPT_CHARSET) == 0) {
        nsXPIDLString acceptCharset;
        rv = mPrefs->GetLocalizedUnicharPref(INTL_ACCEPT_CHARSET, 
                getter_Copies(acceptCharset));
        if (NS_SUCCEEDED(rv))
            SetAcceptCharsets(NS_ConvertUCS2toUTF8(acceptCharset).get());
    }

    // general.useragent.override
    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "override") == 0) {
        char *temp = 0;
        rv = mPrefs->CopyCharPref(UA_PREF_PREFIX "override", &temp);
        if (NS_SUCCEEDED(rv)) {
            mUserAgentOverride.Adopt(temp);
            temp = 0;
            mUserAgentIsDirty = PR_TRUE;
        }
        NS_ASSERTION(!temp, "trouble: |CopyCharPref| failed, but returned a string anyway!");
    }

    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "locale") == 0) {
        // 55156: re-Gather locale.
        nsXPIDLString uval;
        rv = mPrefs->GetLocalizedUnicharPref(UA_PREF_PREFIX "locale", 
                                             getter_Copies(uval));
        if (NS_SUCCEEDED(rv)) {
            mLanguage.Adopt(ToNewUTF8String(nsDependentString(uval)));
            mUserAgentIsDirty = PR_TRUE;
        }
    }

    // general.useragent.misc
    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "misc") == 0) {
        char* temp;
        rv = mPrefs->CopyCharPref(UA_PREF_PREFIX "misc",
                                  &temp);
        if (NS_SUCCEEDED(rv)) {
            mMisc.Adopt(temp);
            temp = 0;
            mUserAgentIsDirty = PR_TRUE;
        }
        NS_ASSERTION(!temp, "trouble: |CopyCharPref| failed, but returned a string anyway!");
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.accept.default") == 0) {
        nsXPIDLCString accept;
        rv = mPrefs->CopyCharPref("network.http.accept.default",
                                  getter_Copies(accept));
        if (NS_SUCCEEDED(rv))
            SetAccept(accept);
    }
    
    if (bChangedAll || PL_strcmp(pref, "network.http.accept-encoding") == 0) {
        nsXPIDLCString acceptEncodings;
        rv = mPrefs->CopyCharPref("network.http.accept-encoding",
                                  getter_Copies(acceptEncodings));
        if (NS_SUCCEEDED(rv))
            SetAcceptEncodings(acceptEncodings);
    }
}

PRInt32 PR_CALLBACK
nsHttpHandler::PrefsCallback(const char *pref, void *self)
{
    nsHttpHandler *handler = (nsHttpHandler *) self;
    NS_ASSERTION(handler, "bad instance data");
    if (handler)
        handler->PrefsChanged(pref);
    return 0;
}

/**
 *  Allocates a C string into that contains a ISO 639 language list
 *  notated with HTTP "q" values for output with a HTTP Accept-Language
 *  header. Previous q values will be stripped because the order of
 *  the langs imply the q value. The q values are calculated by dividing
 *  1.0 amongst the number of languages present.
 *
 *  Ex: passing: "en, ja"
 *      returns: "en, ja;q=0.50"
 *
 *      passing: "en, ja, fr_CA"
 *      returns: "en, ja;q=0.66, fr_CA;q=0.33"
 */
static nsresult
PrepareAcceptLanguages(const char *i_AcceptLanguages, nsACString &o_AcceptLanguages)
{
    if (!i_AcceptLanguages)
        return NS_OK;

    PRUint32 n, size, wrote;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *comma;
    PRInt32 available;


    o_Accept = nsCRT::strdup(i_AcceptLanguages);
    if (nsnull == o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if ((char *) 0 == q_Accept)
        return nsnull;
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p))
    {
        while (*token == ' ' || *token == '\x9') token++;
        char* trim;
        trim = PL_strpbrk(token, "; \x9");
        if (trim != (char*)0)  // remove "; q=..." if present
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? ", " : ""; // delimiter if not first item
            if (q < 0.9995)
                wrote = PR_snprintf(p2, available, "%s%s;q=0.%02u", comma, token, (unsigned) (q * 100.0));
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    nsCRT::free(o_Accept);

    o_AcceptLanguages.Assign((const char *) q_Accept);
    delete [] q_Accept;

    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptLanguages(const char *aAcceptLanguages) 
{
    nsCString buf;
    nsresult rv = PrepareAcceptLanguages(aAcceptLanguages, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptLanguages.Assign(buf.get());
    return rv;
}

/**
 *  Allocates a C string into that contains a character set/encoding list
 *  notated with HTTP "q" values for output with a HTTP Accept-Charset
 *  header. If the UTF-8 character set is not present, it will be added.
 *  If a wildcard catch-all is not present, it will be added. If more than
 *  one charset is set (as of 2001-02-07, only one is used), they will be
 *  comma delimited and with q values set for each charset in decending order.
 *
 *  Ex: passing: "euc-jp"
 *      returns: "euc-jp, utf-8;q=0.667, *;q=0.667"
 *
 *      passing: "UTF-8"
 *      returns: "UTF-8, *"
 */
static nsresult
PrepareAcceptCharsets(const char *i_AcceptCharset, nsACString &o_AcceptCharset)
{
    PRUint32 n, size, wrote;
    PRInt32 available;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *acceptable, *comma;
    PRBool add_utf = PR_FALSE;
    PRBool add_asterick = PR_FALSE;

    if (!i_AcceptCharset)
        acceptable = "";
    else
        acceptable = i_AcceptCharset;
    o_Accept = nsCRT::strdup(acceptable);
    if (nsnull == o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    // only add "utf-8" and "*" to the list if they aren't
    // already specified.

    if (PL_strcasestr(acceptable, "utf-8") == NULL) {
        n++;
        add_utf = PR_TRUE;
    }
    if (PL_strstr(acceptable, "*") == NULL) {
        n++;
        add_asterick = PR_TRUE;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if ((char *) 0 == q_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p)) {
        while (*token == ' ' || *token == '\x9') token++;
        char* trim;
        trim = PL_strpbrk(token, "; \x9");
        if (trim != (char*)0)  // remove "; q=..." if present
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? ", " : ""; // delimiter if not first item
            if (q < 0.9995)
                wrote = PR_snprintf(p2, available, "%s%s;q=0.%02u", comma, token, (unsigned) (q * 100.0));
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    if (add_utf) {
        comma = n++ != 0 ? ", " : ""; // delimiter if not first item
        if (q < 0.9995)
            wrote = PR_snprintf(p2, available, "%sutf-8;q=0.%02u", comma, (unsigned) (q * 100.0));
        else
            wrote = PR_snprintf(p2, available, "%sutf-8", comma);
        q -= dec;
        p2 += wrote;
        available -= wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    if (add_asterick) {
        comma = n++ != 0 ? ", " : ""; // delimiter if not first item

        // keep q of "*" equal to the lowest q value
        // in the event of a tie between the q of "*" and a non-wildcard
        // the non-wildcard always receives preference.

        q += dec;
        if (q < 0.9995) {
            wrote = PR_snprintf(p2, available, "%s*;q=0.%02u", comma, (unsigned) (q * 100.0));
        }
        else
            wrote = PR_snprintf(p2, available, "%s*", comma);
        available -= wrote;
        p2 += wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    nsCRT::free(o_Accept);

    // change alloc from C++ new/delete to nsCRT::strdup's way
    o_AcceptCharset.Assign(q_Accept);
#if defined DEBUG_havill
    printf("Accept-Charset: %s\n", q_Accept);
#endif
    delete [] q_Accept;
    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptCharsets(const char *aAcceptCharsets) 
{
    nsCString buf;
    nsresult rv = PrepareAcceptCharsets(aAcceptCharsets, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptCharsets.Assign(buf.get());
    return rv;
}

nsresult
nsHttpHandler::SetAccept(const char *aAccept) 
{
    mAccept = aAccept;
    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptEncodings(const char *aAcceptEncodings) 
{
    mAcceptEncodings = aAcceptEncodings;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS4(nsHttpHandler,
                              nsIHttpProtocolHandler,
                              nsIProtocolHandler,
                              nsIObserver,
                              nsISupportsWeakReference)

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIProtocolHandler
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpHandler::GetScheme(char **aScheme)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpHandler::GetURIType(PRInt16 *result)
{
    *result = URI_STD;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **aURI)
{
    nsresult rv = NS_OK;

    LOG(("nsHttpHandler::NewURI\n"));

    nsCOMPtr<nsIStandardURL> url = do_CreateInstance(kStandardURLCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // XXX need to choose the default port based on the scheme
    rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY, 80, aSpec, aBaseURI);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, aURI);
}

NS_IMETHODIMP
nsHttpHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    LOG(("nsHttpHandler::NewChannel\n"));

    NS_ENSURE_ARG_POINTER(uri);
    NS_ENSURE_ARG_POINTER(result);

    PRBool isHttp = PR_FALSE, isHttps = PR_FALSE;

    // Verify that we have been given a valid scheme
    nsresult rv = uri->SchemeIs("http", &isHttp);
    if (NS_FAILED(rv)) return rv;
    if (!isHttp) {
        rv = uri->SchemeIs("https", &isHttps);
        if (NS_FAILED(rv)) return rv;
        if (!isHttps) {
            NS_WARNING("Invalid URI scheme");
            return NS_ERROR_UNEXPECTED;
        }
    }
    
    rv = NewProxyChannel(uri,
                         nsnull,
                         -1,
                         nsnull,
                         result);
    return rv;
}

NS_IMETHODIMP 
nsHttpHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    // don't override anything.  
    *_retval = PR_FALSE;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIHttpProtocolHandler
//-----------------------------------------------------------------------------
 
NS_IMETHODIMP
nsHttpHandler::NewProxyChannel(nsIURI *uri,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               const char *proxyType,
                               nsIChannel **result)
{
    nsHttpChannel *httpChannel = nsnull;

    LOG(("nsHttpHandler::NewProxyChannel [proxy=%s:%d type=%s]\n",
        proxyHost, proxyPort, proxyType));

    NS_NEWXPCOM(httpChannel, nsHttpChannel);
    if (!httpChannel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(httpChannel);

    nsresult rv = httpChannel->Init(uri,
                                    mCapabilities,
                                    proxyHost,
                                    proxyPort,
                                    proxyType);

    if (NS_SUCCEEDED(rv))
        rv = httpChannel->
                QueryInterface(NS_GET_IID(nsIChannel), (void **) result);

    NS_RELEASE(httpChannel);
    return rv;
}

NS_IMETHODIMP
nsHttpHandler::GetUserAgent(char **aUserAgent)
{
    return DupString(UserAgent(), aUserAgent);
}

NS_IMETHODIMP
nsHttpHandler::GetAppName(char **aAppName)
{
    return DupString(mAppName, aAppName);
}

NS_IMETHODIMP
nsHttpHandler::GetAppVersion(char **aAppVersion)
{
    return DupString(mAppVersion, aAppVersion);
}

NS_IMETHODIMP
nsHttpHandler::GetVendor(char **aVendor)
{
    return DupString(mVendor, aVendor);
}
NS_IMETHODIMP
nsHttpHandler::SetVendor(const char *aVendor)
{
    mVendor.Adopt(aVendor ? nsCRT::strdup(aVendor) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorSub(char **aVendorSub)
{
    return DupString(mVendorSub, aVendorSub);
}
NS_IMETHODIMP
nsHttpHandler::SetVendorSub(const char *aVendorSub)
{
    mVendorSub.Adopt(aVendorSub ? nsCRT::strdup(aVendorSub) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorComment(char **aVendorComment)
{
    return DupString(mVendorComment, aVendorComment);
}
NS_IMETHODIMP
nsHttpHandler::SetVendorComment(const char *aVendorComment)
{
    mVendorComment.Adopt(aVendorComment ? nsCRT::strdup(aVendorComment) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProduct(char **aProduct)
{
    return DupString(mProduct, aProduct);
}
NS_IMETHODIMP
nsHttpHandler::SetProduct(const char *aProduct)
{
    mProduct.Adopt(aProduct ? nsCRT::strdup(aProduct) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductSub(char **aProductSub)
{
    return DupString(mProductSub, aProductSub);
}
NS_IMETHODIMP
nsHttpHandler::SetProductSub(const char *aProductSub)
{
    mProductSub.Adopt(aProductSub ? nsCRT::strdup(aProductSub) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductComment(char **aProductComment)
{
    return DupString(mProductComment, aProductComment);
}
NS_IMETHODIMP
nsHttpHandler::SetProductComment(const char *aProductComment)
{
    mProductComment.Adopt(aProductComment ? nsCRT::strdup(aProductComment) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetPlatform(char **aPlatform)
{
    return DupString(mPlatform, aPlatform);
}

NS_IMETHODIMP
nsHttpHandler::GetOscpu(char **aOscpu)
{
    return DupString(mOscpu, aOscpu);
}

NS_IMETHODIMP
nsHttpHandler::GetLanguage(char **aLanguage)
{
    return DupString(mLanguage, aLanguage);
}
NS_IMETHODIMP
nsHttpHandler::SetLanguage(const char *aLanguage)
{
    mLanguage.Adopt(aLanguage ? nsCRT::strdup(aLanguage) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetMisc(char **aMisc)
{
    return DupString(mMisc, aMisc);
}
NS_IMETHODIMP
nsHttpHandler::SetMisc(const char *aMisc)
{
    mMisc.Adopt(aMisc ? nsCRT::strdup(aMisc) : 0);
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpHandler::Observe(nsISupports *subject,
                       const PRUnichar *topic,
                       const PRUnichar *data)
{
    if (!nsCRT::strcmp(topic, NS_LITERAL_STRING("profile-before-change").get())) {
        // clear cache of all authentication credentials.
        if (mAuthCache)
            mAuthCache->ClearAll();

        // need to reset the session start time since cache validation may
        // depend on this value.
        mSessionStartTime = NowInSeconds();
    }
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsPendingTransaction
//-----------------------------------------------------------------------------

nsHttpHandler::
nsPendingTransaction::nsPendingTransaction(nsHttpTransaction *trans,
                                           nsHttpConnectionInfo *ci)
    : mTransaction(trans)
    , mConnectionInfo(ci)
{
    LOG(("Creating nsPendingTransaction @%x\n", this));

    NS_PRECONDITION(mTransaction, "null transaction");
    NS_PRECONDITION(mConnectionInfo, "null connection info");

    NS_ADDREF(mTransaction);
    NS_ADDREF(mConnectionInfo);
}

nsHttpHandler::
nsPendingTransaction::~nsPendingTransaction()
{
    LOG(("Destroying nsPendingTransaction @%x\n", this));
 
    NS_RELEASE(mTransaction);
    NS_RELEASE(mConnectionInfo);
}
