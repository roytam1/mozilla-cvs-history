/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include "nsIServiceManager.h"
#include "nsCookieService.h"
#include "nsCookieHTTPNotify.h"
#include "nsCRT.h"
#include "nsCookies.h"
#include "nsIGenericFactory.h"
#include "nsXPIDLString.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindowInternal.h"
#include "nsIPrompt.h"
#include "nsIObserverService.h"
#include "nsIDocumentLoader.h"
#include "nsIWebProgress.h"
#include "nsCURILoader.h"
#include "nsNetCID.h"
#include "nsAppDirectoryServiceDefs.h"

static NS_DEFINE_IID(kDocLoaderServiceCID, NS_DOCUMENTLOADER_SERVICE_CID);

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// nsCookieService Implementation

NS_IMPL_ISUPPORTS4(nsCookieService, nsICookieService,
                   nsIObserver, nsIWebProgressListener, nsISupportsWeakReference);

PRBool gCookieIconVisible = PR_FALSE;

nsCookieService::nsCookieService()
{
  NS_INIT_ISUPPORTS();
}

nsCookieService::~nsCookieService(void)
{
  COOKIE_RemoveAll();
}

nsresult nsCookieService::Init()
{
  COOKIE_RegisterPrefCallbacks();
  nsresult rv;

  // cache mDir
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mDir));
  if (NS_FAILED(rv)) {
    return rv;
  }

  COOKIE_Read();

  nsCOMPtr<nsIObserverService> observerService = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
  if (observerService) {
    observerService->AddObserver(this, "profile-before-change", PR_TRUE);
    observerService->AddObserver(this, "profile-do-change", PR_TRUE);
    observerService->AddObserver(this, "xpcom-shutdown", PR_TRUE);
    observerService->AddObserver(this, "cookieIcon", PR_FALSE);
  }

  // Register as an observer for the document loader  
  nsCOMPtr<nsIDocumentLoader> docLoaderService = 
           do_GetService(kDocLoaderServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && docLoaderService) {
    nsCOMPtr<nsIWebProgress> progress(do_QueryInterface(docLoaderService));
    if (progress)
        (void) progress->AddProgressListener((nsIWebProgressListener*)this,
                                             nsIWebProgress::NOTIFY_STATE_DOCUMENT);
  } else {
    NS_ASSERTION(PR_FALSE, "Could not get nsIDocumentLoader");
  }

  return NS_OK;
}

// nsIWebProgressListener implementation
NS_IMETHODIMP
nsCookieService::OnProgressChange(nsIWebProgress *aProgress,
                                  nsIRequest *aRequest, 
                                  PRInt32 curSelfProgress, 
                                  PRInt32 maxSelfProgress, 
                                  PRInt32 curTotalProgress, 
                                  PRInt32 maxTotalProgress)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsCookieService::OnStateChange(nsIWebProgress* aWebProgress, 
                               nsIRequest *aRequest, 
                               PRUint32 progressStateFlags, 
                               nsresult aStatus)
{
    if ((progressStateFlags & STATE_IS_DOCUMENT) && (progressStateFlags & STATE_STOP)) {
      COOKIE_Notify();
    }
    return NS_OK;
}


/* void onLocationChange (in nsIURI location); */
NS_IMETHODIMP nsCookieService::OnLocationChange(nsIWebProgress* aWebProgress,
                                                     nsIRequest* aRequest,
                                                     nsIURI *location)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsCookieService::OnStatusChange(nsIWebProgress* aWebProgress,
                                     nsIRequest* aRequest,
                                     nsresult aStatus,
                                     const PRUnichar* aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsCookieService::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                       nsIRequest *aRequest, 
                                       PRUint32 state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}


NS_IMETHODIMP
nsCookieService::GetCookieString(nsIURI *aURL, char ** aCookie) {
  *aCookie = COOKIE_GetCookie(aURL);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieStringFromHttp(nsIURI *aURL, nsIURI *aFirstURL, char ** aCookie) {
  if (!aURL) {
    return NS_ERROR_FAILURE;
  }

  *aCookie = COOKIE_GetCookieFromHttp(aURL, aFirstURL);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::SetCookieString(nsIURI *aURL, nsIPrompt* aPrompt, const char * aCookie, nsIHttpChannel* aHttpChannel) {

  COOKIE_SetCookieString(aURL, aPrompt, aCookie, aHttpChannel);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::SetCookieStringFromHttp(nsIURI *aURL, nsIURI *aFirstURL, nsIPrompt *aPrompter, const char *aCookie, const char *aExpires, nsIHttpChannel* aHttpChannel) 
{
  nsCOMPtr<nsIURI> firstURL = aFirstURL;
  if (!firstURL) {
    firstURL = aURL;
  }

  COOKIE_SetCookieStringFromHttp(
    aURL, firstURL, aPrompter, aCookie, (char *)aExpires, aHttpChannel);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieIconIsVisible(PRBool *aIsVisible) {
  *aIsVisible = gCookieIconVisible;
  return NS_OK;
}

NS_IMETHODIMP nsCookieService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  nsresult rv = NS_OK;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    // The profile is about to change.
    
    // Dump current cookies.  This will be done by calling 
    // COOKIE_RemoveAll which clears the memory-resident
    // cookie table.  The reason the cookie file does not
    // need to be updated is because the file was updated every time
    // the memory-resident table changed (i.e., whenever a new cookie
    // was accepted).  If this condition ever changes, the cookie
    // file would need to be updated here.

    COOKIE_RemoveAll();
    if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get()))
      COOKIE_DeletePersistentUserData();
  } else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    // The profile has aleady changed.    
    // Now just read them from the new profile location.
    COOKIE_Read();
  } else if (!nsCRT::strcmp(aTopic, "xpcom-shutdown")) {
    // Leaving browser, need to save cookies
    COOKIE_Write(mDir);
  } else if (!nsCRT::strcmp(aTopic, "cookieIcon")) {
    gCookieIconVisible = (!nsCRT::strcmp(someData, NS_LITERAL_STRING("on").get()));
  }

  return rv;
}
