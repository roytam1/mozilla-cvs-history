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

#include "nsCookie.h"
#include "nsString.h"
#include "nsCRT.h"
#include "prmem.h"

// nsCookie Implementation

NS_IMPL_ISUPPORTS2(nsCookie, nsICookie, nsISupportsWeakReference);

nsCookie::nsCookie() {
  NS_INIT_ISUPPORTS();
}

nsCookie::nsCookie
  (char * name,
   char * value,
   PRBool isDomain,
   char * host,
   char * path,
   PRBool isSecure,
   PRUint64 expires,
   nsCookieStatus status,
   nsCookiePolicy policy) {
  cookieName = name;
  cookieValue = value;
  cookieIsDomain = isDomain;
  cookieHost = host;
  cookiePath = path;
  cookieIsSecure = isSecure;
  cookieExpires = expires;
  cookieStatus = status;
  cookiePolicy = policy;
  NS_INIT_ISUPPORTS();
}

nsCookie::~nsCookie(void) {
  nsCRT::free(cookieName);
  nsCRT::free(cookieValue);
  nsCRT::free(cookieHost);
  nsCRT::free(cookiePath);
}

NS_IMETHODIMP nsCookie::GetName(nsACString& aName) {
  if (cookieName) {
    aName = cookieName;
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsCookie::GetValue(nsACString& aValue) {
  if (cookieValue) {
    aValue = cookieValue;
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsCookie::GetIsDomain(PRBool *aIsDomain) {
  *aIsDomain = cookieIsDomain;
  return NS_OK;
}

NS_IMETHODIMP nsCookie::GetHost(nsACString& aHost) {
//NS_IMETHODIMP nsCookie::GetHost(nsAUTF8String& aHost) {
// using nsACString above instead of nsAUTF8String because the latter doesn't exist yet
  if (cookieHost) {
    aHost = cookieHost;
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsCookie::GetPath(nsACString& aPath) {
//NS_IMETHODIMP nsCookie::GetPath(nsAUTF8String& aPath) {
// using nsACString above instead of nsAUTF8String because the latter doesn't exist yet
  if (cookiePath) {
    aPath = cookiePath;
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsCookie::GetIsSecure(PRBool *aIsSecure) {
  *aIsSecure = cookieIsSecure;
  return NS_OK;
}

NS_IMETHODIMP nsCookie::GetExpires(PRUint64 *aExpires) {
  *aExpires = cookieExpires;
  return NS_OK;
}

NS_IMETHODIMP nsCookie::GetStatus(nsCookieStatus *aStatus) {
  *aStatus = cookieStatus;
  return NS_OK;
}

NS_IMETHODIMP nsCookie::GetPolicy(nsCookiePolicy *aPolicy) {
  *aPolicy = cookiePolicy;
  return NS_OK;
}
