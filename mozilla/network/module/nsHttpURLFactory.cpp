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

#include "nsIProtocolURLFactory.h"
#include "nsHttpUrl.h"
#include "nsINetService.h"
#include "nsNetThread.h"
#include "nsIServiceManager.h"
#include "nsString.h"

class nsHttpURLFactory : public nsIProtocolURLFactory
{
public:

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // nsIProtocolURLFactory:

    NS_IMETHOD CreateURL(nsIURI* *aResult,
                         const nsString& aSpec,
                         const nsIURI* aContextURL = nsnull,
                         nsISupports* aContainer = nsnull,
                         nsILoadGroup* aGroup = nsnull);

    ////////////////////////////////////////////////////////////////////////////
    // nsHttpURLFactory:

    nsHttpURLFactory(void);
    virtual ~nsHttpURLFactory(void);

protected:

};

nsHttpURLFactory::nsHttpURLFactory(void)
{
    NS_INIT_REFCNT();
}

nsHttpURLFactory::~nsHttpURLFactory(void)
{
}

static NS_DEFINE_IID(kIProtocolURLFactoryIID, NS_IPROTOCOLURLFACTORY_IID);
NS_IMPL_ISUPPORTS(nsHttpURLFactory, kIProtocolURLFactoryIID);

NS_IMETHODIMP
nsHttpURLFactory::CreateURL(nsIURI* *aResult,
                          const nsString& aSpec,
                          const nsIURI* aContextURL,
                          nsISupports* aContainer,
                          nsILoadGroup* aGroup)
{
    nsHttpUrlImpl* url = new nsHttpUrlImpl(aContainer, aGroup);
    if (url == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(url);
    nsresult err = url->ParseURL(aSpec, aContextURL);
    if (err != NS_OK) {
		NS_RELEASE(url);
        return err;
	}
    *aResult = url;
    return NS_OK;
}

static NS_DEFINE_IID(kINetServiceIID, NS_INETSERVICE_IID);
static NS_DEFINE_IID(kNetServiceCID, NS_NETSERVICE_CID);

// XXX temporarily, until we have pluggable protocols...
extern "C" NS_NET nsresult
NS_InitializeHttpURLFactory(nsINetService* inet)
{
    nsresult rv;
    nsHttpURLFactory* urlf = new nsHttpURLFactory();
    if (urlf == NULL)
        return NS_ERROR_OUT_OF_MEMORY;
#if 0
    nsINetService *inet = nsnull;
    rv = nsServiceManager::GetService(kNetServiceCID,
                                      kINetServiceIID,
                                      (nsISupports **)&inet);
    if (rv != NS_OK) return rv;
#endif
    rv = inet->RegisterProtocol(*new nsString("http"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("https"), urlf, NULL);
    if (rv != NS_OK) goto done;

    // XXX Hacks until we have real urlf objects for these...
    inet->RegisterProtocol(*new nsString("ftp"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("resource"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("chrome"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("file"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("javascript"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("sockstub"), urlf, NULL);
    if (rv != NS_OK) goto done;
    inet->RegisterProtocol(*new nsString("about"), urlf, NULL);
    if (rv != NS_OK) goto done;

  done:
#if 0
    nsServiceManager::ReleaseService(kNetServiceCID, inet);
#endif
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
