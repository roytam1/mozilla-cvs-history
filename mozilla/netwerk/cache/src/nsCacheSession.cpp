/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is nsCacheSession.h, released February 23, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan <gordon@netscape.com>
 *    Patrick Beard   <beard@netscape.com>
 *    Darin Fisher    <darin@netscape.com>
 */

#include "nsCacheSession.h"
#include "nsCacheService.h"



NS_IMPL_ISUPPORTS1(nsCacheSession, nsICacheSession)

nsCacheSession::nsCacheSession(const char * clientID,
                               PRInt32 storagePolicy,
                               PRBool streamBased)
    : mClientID(clientID),
      mStoragePolicy(storagePolicy),
      mStreamBased(streamBased)
{
  NS_INIT_ISUPPORTS();
}

nsCacheSession::~nsCacheSession()
{
  /* destructor code */
    // notify service we are going away?
}


NS_IMETHODIMP
nsCacheSession::OpenCacheEntry(const char *               key, 
                               nsCacheAccessMode          accessRequested, 
                               nsICacheEntryDescriptor ** result)
{
    return nsCacheService::GlobalInstance()->OpenCacheEntry(this,
                                                            key,
                                                            accessRequested,
                                                            result);
}


NS_IMETHODIMP nsCacheSession::AsyncOpenCacheEntry(const char *key,
                                                  nsCacheAccessMode accessRequested,
                                                  nsICacheListener *listener)
{
    return nsCacheService::GlobalInstance()->AsyncOpenCacheEntry(this,
                                                                 key,
                                                                 accessRequested,
                                                                 listener);
}
