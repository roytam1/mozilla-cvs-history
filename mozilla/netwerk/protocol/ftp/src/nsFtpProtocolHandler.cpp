/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsFtpProtocolHandler.h"
#include "nsFTPChannel.h"
#include "nsIURL.h"
#include "nsIURLParser.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsConnectionCacheObj.h"
#include "prlog.h"
#include "nsIProtocolProxyService.h"
#include "nsIHTTPProtocolHandler.h"
#include "nsIHTTPChannel.h"
#include "nsNetUtil.h"


#if defined(PR_LOGGING)
//
// Log module for FTP Protocol logging...
//
// To enable logging (see prlog.h for full details):
//
//    set NSPR_LOG_MODULES=nsFTPProtocol:5
//    set NSPR_LOG_FILE=nspr.log
//
// this enables PR_LOG_DEBUG level information and places all output in
// the file nspr.log
//
PRLogModuleInfo* gFTPLog = nsnull;

#endif /* PR_LOGGING */

static NS_DEFINE_CID(kStandardURLCID,       NS_STANDARDURL_CID);
static NS_DEFINE_CID(kAuthUrlParserCID,     NS_AUTHORITYURLPARSER_CID);
static NS_DEFINE_CID(kProtocolProxyServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
static NS_DEFINE_CID(kHTTPHandlerCID, NS_IHTTPHANDLER_CID);

////////////////////////////////////////////////////////////////////////////////

nsFtpProtocolHandler::~nsFtpProtocolHandler() {
    PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("~nsFtpProtocolHandler() called"));
}

NS_IMPL_ISUPPORTS4(nsFtpProtocolHandler, 
                   nsIProtocolHandler, 
                   nsIConnectionCache, 
                   nsIObserver,
                   nsIProxy)

nsresult
nsFtpProtocolHandler::Init() {
    nsresult rv = NS_OK;

#if defined(PR_LOGGING)
    if (!gFTPLog) gFTPLog = PR_NewLogModule("nsFTPProtocol");
#endif /* PR_LOGGING */

    mProxySvc = do_GetService(kProtocolProxyServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    NS_NEWXPCOM(mRootConnectionList, nsHashtable);
    if (!mRootConnectionList) return NS_ERROR_OUT_OF_MEMORY;
    rv = NS_NewThreadPool(getter_AddRefs(mPool), 
                          NS_FTP_MIN_CONNECTION_COUNT,
                          NS_FTP_MAX_CONNECTION_COUNT,
                          NS_FTP_CONNECTION_STACK_SIZE);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIObserverService> obsServ = do_GetService(NS_OBSERVERSERVICE_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
        nsAutoString topic(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        obsServ->AddObserver(this, topic.GetUnicode());
    }
    return rv;
}


    
////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsFtpProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("ftp");
    if (*result == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = 21; 
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::GetUritype(PRInt16 *result)
{
    *result = url_std;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv;
    PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("FTP attempt at %s ", aSpec));

    nsCOMPtr<nsIURI> url;
    nsCOMPtr<nsIURLParser> urlparser;
    if (aBaseURI) {
        rv = aBaseURI->Clone(getter_AddRefs(url));
        if (NS_FAILED(rv)) return rv;
        rv = url->SetRelativePath(aSpec);
    }
    else {
        rv = nsComponentManager::CreateInstance(kAuthUrlParserCID, 
                                    nsnull, NS_GET_IID(nsIURLParser),
                                    getter_AddRefs(urlparser));
        if (NS_FAILED(rv)) return rv;
        rv = nsComponentManager::CreateInstance(kStandardURLCID, 
                                    nsnull, NS_GET_IID(nsIURI),
                                    getter_AddRefs(url));
        if (NS_FAILED(rv)) return rv;

        rv = url->SetURLParser(urlparser);
        if (NS_FAILED(rv)) return rv;

        rv = url->SetSpec((char*)aSpec);
    }
    if (NS_FAILED(rv)) {
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("FAILED\n"));
        return rv;
    }

    *result = url.get();
    NS_ADDREF(*result);
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("SUCCEEDED\n"));
    return rv;
}

NS_IMETHODIMP
nsFtpProtocolHandler::NewChannel(const char* verb, nsIURI* url,
                                 nsILoadGroup* aLoadGroup,
                                 nsIInterfaceRequestor* notificationCallbacks,
                                 nsLoadFlags loadAttributes,
                                 nsIURI* originalURI,
                                 PRUint32 bufferSegmentSize,
                                 PRUint32 bufferMaxSize,
                                 nsIChannel* *result)
{
    nsresult rv = NS_OK;

    PRBool useProxy = PR_FALSE;
    
    if (NS_SUCCEEDED(mProxySvc->GetProxyEnabled(&useProxy)) && useProxy)
    {
        rv = mProxySvc->ExamineForProxy(url, this);
        if (NS_FAILED(rv)) return rv;
    }

    nsFTPChannel* channel;
    rv = nsFTPChannel::Create(nsnull, NS_GET_IID(nsPIFTPChannel), (void**)&channel);
    if (NS_FAILED(rv)) return rv;

    rv = channel->Init(verb, url, aLoadGroup, notificationCallbacks, loadAttributes,
                       originalURI, bufferSegmentSize, bufferMaxSize, this, mPool);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("nsFtpProtocolHandler::NewChannel() FAILED\n"));
        return rv;
    }

    if (!mProxyHost.IsEmpty() || mProxyPort > 0) {
        nsCOMPtr<nsIChannel> proxyChannel;
        // if an FTP proxy is enabled, push things off to HTTP.

        nsCOMPtr<nsIHTTPProtocolHandler> httpHandler = do_GetService(kHTTPHandlerCID, &rv);
        if (NS_FAILED(rv)) return rv;

        // Some dummy URI for the HTTP layer.
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), "http://");
        if (NS_FAILED(rv)) return rv;

        rv = httpHandler->NewChannel(verb, uri, aLoadGroup,
                                     NS_STATIC_CAST(nsIInterfaceRequestor*, channel),
                                     loadAttributes,
                                     url,
                                     bufferSegmentSize,
                                     bufferMaxSize,
                                     getter_AddRefs(proxyChannel));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIHTTPChannel> httpChannel = do_QueryInterface(proxyChannel, &rv);
        if (NS_FAILED(rv)) return rv;

        nsXPIDLCString spec;
        rv = url->GetSpec(getter_Copies(spec));
        if (NS_FAILED(rv)) return rv;

        rv = httpChannel->SetProxyRequestURI((const char*)spec);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIProxy> proxyHTTP = do_QueryInterface(httpChannel, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = proxyHTTP->SetProxyHost(mProxyHost.GetBuffer());
        if (NS_FAILED(rv)) return rv;

        rv = proxyHTTP->SetProxyPort(mProxyPort);
        if (NS_FAILED(rv)) return rv;

        rv = channel->SetProxyChannel(proxyChannel);
    }

    *result = channel;
    return rv;
}

// nsIConnectionCache methods
NS_IMETHODIMP
nsFtpProtocolHandler::RemoveConn(const char *aKey, nsConnectionCacheObj* *_retval) {
    NS_ASSERTION(_retval, "null pointer");
    nsStringKey key(aKey);
    *_retval = (nsConnectionCacheObj*)mRootConnectionList->Remove(&key);
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::InsertConn(const char *aKey, nsConnectionCacheObj *aConn) {
    NS_ASSERTION(aConn, "null pointer");
    nsStringKey key(aKey);
    mRootConnectionList->Put(&key, aConn);
    return NS_OK;
}

// cleans up a connection list entry
PRBool CleanupConnEntry(nsHashKey *aKey, void *aData, void *closure) {
    delete (nsConnectionCacheObj*)aData;
    return PR_TRUE;
}

// nsIObserver method
NS_IMETHODIMP
nsFtpProtocolHandler::Observe(nsISupports     *aSubject,
                              const PRUnichar *aTopic,
                              const PRUnichar *someData ) {
    nsresult rv;
    if (mRootConnectionList) {
        mRootConnectionList->Reset(CleanupConnEntry);
        NS_DELETEXPCOM(mRootConnectionList);
        mRootConnectionList = nsnull;
    }
    // remove ourself from the observer service.
    NS_WITH_SERVICE(nsIObserverService, obsServ, NS_OBSERVERSERVICE_PROGID, &rv);
    if (NS_SUCCEEDED(rv)) {
        nsAutoString topic(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        obsServ->RemoveObserver(this, topic.GetUnicode());
    }
    return NS_OK;
}


// nsIProxy methods
NS_IMETHODIMP
nsFtpProtocolHandler::GetProxyHost(char* *_retval) {
    *_retval = mProxyHost.ToNewCString();
    if (!*_retval) return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::SetProxyHost(const char *aProxyHost) {
    mProxyHost = aProxyHost;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::GetProxyPort(PRInt32 *_retval) {
    *_retval = mProxyPort;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::SetProxyPort(PRInt32 aProxyPort) {
    mProxyPort = aProxyPort;
    return NS_OK;
}
////////////////////////////////////////////////////////////////////////////////
