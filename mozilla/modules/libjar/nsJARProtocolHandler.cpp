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

#include "nsJARProtocolHandler.h"
#include "nsIIOService.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsJARURI.h"
#include "nsIURL.h"
#include "nsJARChannel.h"
#include "nsXPIDLString.h"

static NS_DEFINE_CID(kIOServiceCID,      NS_IOSERVICE_CID);
static NS_DEFINE_CID(kJARUriCID,         NS_JARURI_CID);
static NS_DEFINE_CID(kZipReaderCacheCID, NS_ZIPREADERCACHE_CID);

#define NS_JAR_CACHE_SIZE       32

////////////////////////////////////////////////////////////////////////////////

nsJARProtocolHandler::nsJARProtocolHandler()
{
    NS_INIT_REFCNT();
}

nsresult
nsJARProtocolHandler::Init()
{
    nsresult rv;
	rv = nsComponentManager::CreateInstance(kZipReaderCacheCID,
                                            nsnull,
                                            NS_GET_IID(nsIZipReaderCache),
                                            getter_AddRefs(mJARCache));
    if (NS_FAILED(rv)) return rv;

    rv = mJARCache->Init(NS_JAR_CACHE_SIZE);
    return rv;
}

nsJARProtocolHandler::~nsJARProtocolHandler()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsJARProtocolHandler, 
                              nsIJARProtocolHandler, 
                              nsIProtocolHandler)

NS_METHOD
nsJARProtocolHandler::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsJARProtocolHandler* ph = new nsJARProtocolHandler();
    if (ph == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ph);
    nsresult rv = ph->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = ph->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(ph);
    return rv;
}

NS_IMETHODIMP
nsJARProtocolHandler::GetJARCache(nsIZipReaderCache* *result)
{
    *result = mJARCache;
    NS_ADDREF(*result);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
nsJARProtocolHandler::GetScheme(char* *result)
{
    *result = nsCRT::strdup("jar");
    if (*result == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsJARProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        // no port for JAR: URLs
    return NS_OK;
}

NS_IMETHODIMP
nsJARProtocolHandler::NewURI(const char *aSpec, nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv = NS_OK;
    nsIURI* url;

    rv = nsJARURI::Create(nsnull, NS_GET_IID(nsIJARURI), (void**)&url);
    if (NS_FAILED(rv)) return rv;

    if (aBaseURI)
    {
        nsXPIDLCString aResolvedURI;
        rv = aBaseURI->Resolve(aSpec, getter_Copies(aResolvedURI));
        if (NS_FAILED(rv)) return rv;
        rv = url->SetSpec(aResolvedURI);
    } else {
        rv = url->SetSpec((char*)aSpec);
    }

    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *result = url;
    return rv;
}

NS_IMETHODIMP
nsJARProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    nsresult rv;
    
	nsJARChannel* channel;
    rv = nsJARChannel::Create(nsnull, NS_GET_IID(nsIJARChannel), (void**)&channel);
    if (NS_FAILED(rv)) return rv;

	rv = channel->Init(this, uri);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }

    *result = channel;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
