/* -*- Mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef DEBUG_dp
#include <stdio.h>
#endif 

#include "nsCookieService.h" /* don't remove -- needed for mac build */
#include "nsCookieHTTPNotify.h"
#include "nsIGenericFactory.h"
#include "nsIHttpChannel.h"
#include "nsCookie.h"
#include "nsIURL.h"
#include "nsCRT.h"
#include "nsLiteralString.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsINetModuleMgr.h" 
#include "nsILoadGroup.h"
#include "nsICategoryManager.h"
#include "nsIHttpProtocolHandler.h"		// for NS_HTTP_STARTUP_CATEGORY
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrompt.h"

// we want to explore making the document own the load group
// so we can associate the document URI with the load group.
// until this point, we have an evil hack:
#include "nsIHttpChannelInternal.h"  

static NS_DEFINE_CID(kINetModuleMgrCID, NS_NETMODULEMGR_CID);

///////////////////////////////////
// nsISupports

NS_IMPL_ISUPPORTS2(nsCookieHTTPNotify, nsIHttpNotify, nsINetNotify);

///////////////////////////////////
// nsCookieHTTPNotify Implementation

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsCookieHTTPNotify, Init)

nsresult nsCookieHTTPNotify::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    return nsCookieHTTPNotifyConstructor(aOuter, aIID, aResult);
}

NS_METHOD nsCookieHTTPNotify::RegisterProc(nsIComponentManager *aCompMgr,
                                           nsIFile *aPath,
                                           const char *registryLocation,
                                           const char *componentType,
                                           const nsModuleComponentInfo *info)
{
    // Register ourselves into the NS_CATEGORY_HTTP_STARTUP
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString prevEntry;
    rv = catman->AddCategoryEntry(NS_HTTP_STARTUP_CATEGORY, "Http Cookie Notify", NS_COOKIEHTTPNOTIFY_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(prevEntry));

    return NS_OK;

}

NS_METHOD nsCookieHTTPNotify::UnregisterProc(nsIComponentManager *aCompMgr,
                                             nsIFile *aPath,
                                             const char *registryLocation,
                                             const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = catman->DeleteCategoryEntry(NS_HTTP_STARTUP_CATEGORY, 
                                     NS_COOKIEHTTPNOTIFY_CONTRACTID, PR_TRUE);

    // Return value is not used from this function.
    return NS_OK;
}

NS_IMETHODIMP
nsCookieHTTPNotify::Init()
{
    // Register to handing http requests and responses
    nsresult rv = NS_OK;
    nsCOMPtr<nsINetModuleMgr> pNetModuleMgr = do_GetService(kINetModuleMgrCID, &rv); 
    if (NS_FAILED(rv)) return rv;
    rv = pNetModuleMgr->RegisterModule(NS_NETWORK_MODULE_MANAGER_HTTP_REQUEST_CONTRACTID,
                                       (nsIHttpNotify *)this);
    if (NS_FAILED(rv)) return rv;

    rv = pNetModuleMgr->RegisterModule(NS_NETWORK_MODULE_MANAGER_HTTP_RESPONSE_CONTRACTID,
                                       (nsIHttpNotify *)this);
    return rv;
}

nsCookieHTTPNotify::nsCookieHTTPNotify()
{
    NS_INIT_ISUPPORTS();
    mCookieService = nsnull;
#ifdef DEBUG_dp
    printf("CookieHTTPNotify Created.\n");
#endif /* DEBUG_dp */
}

nsCookieHTTPNotify::~nsCookieHTTPNotify()
{
}

NS_IMETHODIMP
nsCookieHTTPNotify::SetupCookieService()
{
    nsresult rv = NS_OK;
    if (!mCookieService)
    {
      mCookieService = do_GetService(NS_COOKIESERVICE_CONTRACTID, &rv);
    }
    return rv;
}

///////////////////////////////////
// nsIHttpNotify

NS_IMETHODIMP
nsCookieHTTPNotify::OnModifyRequest(nsIHttpChannel *aHttpChannel)
{
    nsresult rv;
    // Preconditions
    NS_ENSURE_ARG_POINTER(aHttpChannel);

    // Get the url
    nsCOMPtr<nsIURI> pURL;
    rv = aHttpChannel->GetURI(getter_AddRefs(pURL));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aHttpChannel);
    NS_ENSURE_TRUE(httpInternal, NS_ERROR_UNEXPECTED);

    // Get the original url that the user either typed in or clicked on
    nsCOMPtr<nsIURI> pFirstURL;
    rv = httpInternal->GetDocumentURI(getter_AddRefs(pFirstURL));
    if (NS_FAILED(rv)) return rv;
    if (!pFirstURL) {
      rv = aHttpChannel->GetOriginalURI(getter_AddRefs(pFirstURL));
      if (NS_FAILED(rv)) return rv;
    }

    // Ensure that the cookie service exists
    rv = SetupCookieService();
    if (NS_FAILED(rv)) return rv;

    // Get the cookies
    char * cookie;
    rv = mCookieService->GetCookieStringFromHttp(pURL, pFirstURL, &cookie);
    if (NS_FAILED(rv)) return rv;

    // Clear any existing Cookie request header
    rv = aHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Cookie"), NS_LITERAL_CSTRING(""));
    if (NS_FAILED(rv)) return rv;

    // Set the cookie into the request headers
    if (cookie && *cookie)
        rv = aHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Cookie"), nsDependentCString(cookie));
    nsMemory::Free((void *)cookie);

    return rv;
}

NS_IMETHODIMP
nsCookieHTTPNotify::OnExamineResponse(nsIHttpChannel *aHttpChannel)
{
    nsresult rv;
    // Preconditions
    NS_ENSURE_ARG_POINTER(aHttpChannel);

    // Get the Cookie header
    nsCAutoString cookieHeader;
    rv = aHttpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Set-Cookie"), cookieHeader);
    if (NS_FAILED(rv)) return rv;
    if (cookieHeader.IsEmpty()) return NS_OK; // not an error, there's just no header.

    // Get the url
    nsCOMPtr<nsIURI> pURL;
    rv = aHttpChannel->GetURI(getter_AddRefs(pURL));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aHttpChannel);
    NS_ENSURE_TRUE(httpInternal, NS_ERROR_UNEXPECTED);

    // Get the original url that the user either typed in or clicked on
    nsCOMPtr<nsIURI> pFirstURL;
    rv = httpInternal->GetDocumentURI(getter_AddRefs(pFirstURL));
    if (NS_FAILED(rv)) return rv;

    // Get the prompter
    nsCOMPtr<nsILoadGroup> pLoadGroup;
    rv = aHttpChannel->GetLoadGroup(getter_AddRefs(pLoadGroup));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIChannel> pChannel;
    if (pLoadGroup) {
      nsCOMPtr<nsIRequest> pRequest;
      rv = pLoadGroup->GetDefaultLoadRequest(getter_AddRefs(pRequest));
      if (NS_FAILED(rv)) return rv;
      pChannel = do_QueryInterface(pRequest);
    }
    nsCOMPtr<nsIInterfaceRequestor> pInterfaces;
    nsCOMPtr<nsIPrompt> pPrompter;
    if (pChannel) {
      pChannel->GetNotificationCallbacks(getter_AddRefs(pInterfaces));
    } else {
      aHttpChannel->GetNotificationCallbacks(getter_AddRefs(pInterfaces));
    }
    if (pInterfaces)
      pInterfaces->GetInterface(NS_GET_IID(nsIPrompt), getter_AddRefs(pPrompter));

    // Get the expires
    nsCAutoString dateHeader;
    rv = aHttpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Date"), dateHeader);
    // NS_ERROR_NOT_AVAILABLE is not a fatal error, other errors are
    if (NS_FAILED(rv) && rv != NS_ERROR_NOT_AVAILABLE) return rv;

    // Ensure that we have the cookie service
    rv = SetupCookieService();
    if (NS_FAILED(rv)) return rv;

    // Save the cookie
    rv = mCookieService->SetCookieStringFromHttp(pURL, pFirstURL, pPrompter, cookieHeader.get(), dateHeader.get(), aHttpChannel);

    return rv;
}

