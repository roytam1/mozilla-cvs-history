/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Contributor(s):
 *   Scott MacGregor <scott@scott-macgregor.org>
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

#include "nsMsgContentPolicy.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"

static const char kBlockRemoteImages[] = "mailnews.message_display.disable_remote_image";
static const char kAllowPlugins[] = "mailnews.message_display.allow.plugins";

NS_IMPL_ADDREF(nsMsgContentPolicy)
NS_IMPL_RELEASE(nsMsgContentPolicy)

NS_INTERFACE_MAP_BEGIN(nsMsgContentPolicy)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentPolicy)
   NS_INTERFACE_MAP_ENTRY(nsIContentPolicy)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsSupportsWeakReference)
NS_INTERFACE_MAP_END

nsMsgContentPolicy::nsMsgContentPolicy()
{
  mAllowPlugins = PR_FALSE;
}

nsMsgContentPolicy::~nsMsgContentPolicy()
{
  // hey, we are going away...clean up after ourself....unregister our observer
  nsresult rv = NS_OK;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIPrefBranchInternal> prefInternal = do_QueryInterface(prefBranch, &rv);
    if (NS_SUCCEEDED(rv))
    {
      prefInternal->RemoveObserver(kBlockRemoteImages, this);
      prefInternal->RemoveObserver(kAllowPlugins, this);
    }
  }
}

nsresult nsMsgContentPolicy::Init()
{
  nsresult rv;

  // register ourself as an observer on the mail preference to block remote images
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranchInternal> prefInternal = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  prefInternal->AddObserver(kBlockRemoteImages, this, PR_TRUE);
  prefInternal->AddObserver(kAllowPlugins, this, PR_TRUE);

  prefBranch->GetBoolPref(kAllowPlugins, &mAllowPlugins);
  rv = prefBranch->GetBoolPref(kBlockRemoteImages, &mBlockRemoteImages);
  
  return rv;
}

NS_IMETHODIMP
nsMsgContentPolicy::ShouldLoad(PRInt32 aContentType, nsIURI *aContentLocation, nsISupports *aContext,
                               nsIDOMWindow *aWindow, PRBool *shouldLoad)
{
  nsresult rv = NS_OK;
  *shouldLoad = PR_TRUE;

  if (!aContentLocation || !aContext) 
      return rv;

  if (aContentType == nsIContentPolicy::OBJECT)
  {
      // only allow the plugin to load if the allow plugins pref has been set
      *shouldLoad = mAllowPlugins;
  }
  else if (aContentType == nsIContentPolicy::IMAGE)
  {
    PRBool isFtp = PR_FALSE;
    rv = aContentLocation->SchemeIs("ftp", &isFtp);

    if (isFtp) 
    {
      // never allow ftp for mail messages, 
      // because we don't want to send the users email address
      // as the anonymous password
      *shouldLoad = PR_FALSE;
    }
    else // check for http and https urls...block those if necessary
    {
      PRBool needToCheck = PR_FALSE;
      rv = aContentLocation->SchemeIs("http", &needToCheck);
      NS_ENSURE_SUCCESS(rv,rv);

      if (!needToCheck) {
        rv = aContentLocation->SchemeIs("https", &needToCheck);
        NS_ENSURE_SUCCESS(rv,rv);
      }

      if (needToCheck) // http or https ? 
      {
        // check the 'disable remote images pref' and block the image if appropriate
        *shouldLoad = !mBlockRemoteImages;
      }
    }
  }

  return rv;
}

NS_IMETHODIMP nsMsgContentPolicy::ShouldProcess(PRInt32 contentType,
                                                nsIURI *contentLocation,
                                                nsISupports *context,
                                                nsIDOMWindow *window,
                                                PRBool *shouldProcess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgContentPolicy::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsresult rv;
   
  if (!nsCRT::strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) 
  {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_LossyConvertUCS2toASCII pref(aData);

    if (pref.Equals(kBlockRemoteImages))
      rv = prefBranch->GetBoolPref(kBlockRemoteImages, &mBlockRemoteImages);
  }
  
  return NS_OK;
}
