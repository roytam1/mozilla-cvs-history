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

#include "nsAbout.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

NS_IMPL_ISUPPORTS(nsAbout, NS_GET_IID(nsIAboutModule));

static const char kURI[] = "chrome://global/content/about.html";

NS_IMETHODIMP
nsAbout::NewChannel(const char *verb,
                    nsIURI *aURI,
                    nsILoadGroup* aLoadGroup,
                    nsIInterfaceRequestor* notificationCallbacks,
                    nsLoadFlags loadAttributes,
                    nsIURI* originalURI,
                    PRUint32 bufferSegmentSize,
                    PRUint32 bufferMaxSize,
                    nsIChannel **result)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, ioService, kIOServiceCID, &rv);
    if ( NS_FAILED(rv) )
        return rv;
   	rv = ioService->NewChannel(verb, kURI, NULL, aLoadGroup,
                               notificationCallbacks, loadAttributes,
                               originalURI, bufferSegmentSize, bufferMaxSize,
                               result);
    return rv;
}

NS_METHOD
nsAbout::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAbout* about = new nsAbout();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
