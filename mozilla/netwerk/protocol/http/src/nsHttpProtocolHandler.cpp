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

#include "nsHttpProtocolHandler.h"
#include "nsHttpProtocolConnection.h"
#include "nsIUrl.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsISocketTransportService.h"
#include "nsIChannel.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kStandardUrlCID,            NS_STANDARDURL_CID);

////////////////////////////////////////////////////////////////////////////////

nsHttpProtocolHandler::nsHttpProtocolHandler()
    : mConnectionPool(nsnull)
{
    NS_INIT_REFCNT();
}

nsresult
nsHttpProtocolHandler::Init(void)
{
    mConnectionPool = new nsHashtable();
    if (mConnectionPool == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

nsHttpProtocolHandler::~nsHttpProtocolHandler()
{
    if (mConnectionPool) delete mConnectionPool;
}

NS_IMPL_ISUPPORTS(nsHttpProtocolHandler, nsIProtocolHandler::GetIID());

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsHttpProtocolHandler::GetScheme(const char* *result)
{
    *result = "http";
    return NS_OK;
}

NS_IMETHODIMP
nsHttpProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = 80;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpProtocolHandler::MakeAbsoluteUrl(const char* aSpec,
                                       nsIUrl* aBaseUrl,
                                       char* *result)
{
    // XXX optimize this to not needlessly construct the URL

    nsresult rv;
    nsIUrl* url;
    rv = NewUrl(aSpec, aBaseUrl, &url);
    if (NS_FAILED(rv)) return rv;

    rv = url->ToNewCString(result);
    NS_RELEASE(url);
    return rv;
}

NS_IMETHODIMP
nsHttpProtocolHandler::NewUrl(const char* aSpec,
                              nsIUrl* aBaseUrl,
                              nsIUrl* *result)
{
    nsresult rv;

    // http URLs (currently) have no additional structure beyond that provided by standard
    // URLs, so there is no "outer" given to CreateInstance 

    nsIUrl* url;
    rv = nsComponentManager::CreateInstance(kStandardUrlCID, nsnull,
                                            nsIUrl::GetIID(),
                                            (void**)&url);
    if (NS_FAILED(rv)) return rv;

    rv = url->Init(aSpec, aBaseUrl);

    nsIUrl* realUrl = nsnull;
    
    rv = url->QueryInterface(nsIUrl::GetIID(), (void**)&realUrl);
    if (NS_FAILED(rv)) return rv;

    // XXX this is the default port for http. we need to strip out the actual
    // XXX requested port.
    realUrl->SetPort(80);

    *result = realUrl;
    NS_ADDREF(*result);

    return rv;
}

NS_IMETHODIMP
nsHttpProtocolHandler::NewConnection(nsIUrl* url,
                                     nsISupports* eventSink,
                                     nsIEventQueue* eventQueue,
                                     nsIProtocolConnection* *result)
{
    nsresult rv;
    nsHttpProtocolConnection* connection = new nsHttpProtocolConnection();
    if (connection == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = connection->Init(url, eventSink, this, eventQueue);
    if (NS_FAILED(rv)) {
        delete connection;
        return rv;
    }
    NS_ADDREF(connection);
    *result = connection;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
nsHttpProtocolHandler::GetTransport(const char* host, PRInt32 port,
                                    nsIChannel* *result)
{
    nsresult rv;
    nsSocketTransportKey key(host, port);
    nsIChannel* trans;
    
    trans = (nsIChannel*)mConnectionPool->Get(&key);
    if (trans) {
        *result = trans;
        NS_ADDREF(trans);
        return NS_OK;
    }
    
    NS_WITH_SERVICE(nsISocketTransportService, sts, kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = sts->CreateTransport(host, port, &trans);
    if (NS_FAILED(rv)) return rv;

    void* oldValue = mConnectionPool->Put(&key, trans);
    NS_ASSERTION(oldValue == nsnull, "race?");
    NS_ADDREF(trans);   // released in ReleaseTransport
    *result = trans;
        
    return NS_OK;
}

nsresult
nsHttpProtocolHandler::ReleaseTransport(const char* host, PRInt32 port,
                                        nsIChannel* trans)
{
    nsSocketTransportKey key(host, port);
    nsIChannel* value = (nsIChannel*)mConnectionPool->Remove(&key);

    NS_ASSERTION(trans == value, "mConnectionPool out of sync");
    if (!value) {
        return NS_ERROR_FAILURE;
    }

    NS_RELEASE(value);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

