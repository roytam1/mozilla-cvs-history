/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications Corporation.
 * Portions created by Netscape Communications Corporation are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the LGPL or the GPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsPluginProxyImpl.h"
#include "nsCRT.h"

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPluginProxyImpl, nsIProxy)

  nsPluginProxyImpl::nsPluginProxyImpl() : 
    mProxyHost(nsnull), mProxyType(nsnull), mProxyPort(-1)
{
  NS_INIT_ISUPPORTS();
  
}

nsPluginProxyImpl::~nsPluginProxyImpl()
{
  if (mProxyHost) {
    nsCRT::free(mProxyHost);
  }
  if (mProxyType) {
    nsCRT::free(mProxyType);
  }
  mProxyPort = -1;
}

/* attribute string proxyHost; */
NS_IMETHODIMP nsPluginProxyImpl::GetProxyHost(char * *aProxyHost)
{
  if (!aProxyHost) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_ERROR_OUT_OF_MEMORY;

  if (mProxyHost) {
    if ((*aProxyHost = nsCRT::strdup(mProxyHost))) {
      rv = NS_OK;
    }
  }
  return rv;
}

NS_IMETHODIMP nsPluginProxyImpl::SetProxyHost(const char * aProxyHost)
{
  nsresult rv = NS_OK;
  char *newProxyHost;

  if (aProxyHost) {
    if ((newProxyHost = nsCRT::strdup(aProxyHost))) {
      nsCRT::free(mProxyHost);
      mProxyHost = newProxyHost;
    }
    else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  else {
    // setting our mProxyHost to nsnull
    nsCRT::free(mProxyHost);
    mProxyHost = nsnull;
  }

  return rv;
}

/* attribute long proxyPort; */
NS_IMETHODIMP nsPluginProxyImpl::GetProxyPort(PRInt32 *aProxyPort)
{
  if (!aProxyPort) {
    return NS_ERROR_NULL_POINTER;
  }
  *aProxyPort = mProxyPort;
  return NS_OK;
}
NS_IMETHODIMP nsPluginProxyImpl::SetProxyPort(PRInt32 aProxyPort)
{
  mProxyPort = aProxyPort;
  return NS_OK;
}

/* attribute string proxyType; */
NS_IMETHODIMP nsPluginProxyImpl::GetProxyType(char * *aProxyType)
{
  if (!aProxyType) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_ERROR_OUT_OF_MEMORY;

  if (mProxyType) {
    if ((*aProxyType = nsCRT::strdup(mProxyType))) {
      rv = NS_OK;
    }
  }
  return rv;
}
NS_IMETHODIMP nsPluginProxyImpl::SetProxyType(const char * aProxyType)
{
  nsresult rv = NS_OK;
  char *newProxyType;

  if (aProxyType) {
    if ((newProxyType = nsCRT::strdup(aProxyType))) {
      nsCRT::free(mProxyType);
      mProxyType = newProxyType;
    }
    else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  else {
    // setting our mProxyType to nsnull
    nsCRT::free(mProxyType);
    mProxyType = nsnull;
  }

  return rv;
}
