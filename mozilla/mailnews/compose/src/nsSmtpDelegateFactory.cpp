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
 * Alec Flett <alecf@netscape.com>
 */

#include "nsSmtpDelegateFactory.h"

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "nsIURL.h"
#include "nsNetCID.h"

#include "nsIRDFResource.h"

#include "nsISmtpServer.h"
#include "nsISmtpService.h"
#include "nsMsgCompCID.h"

static NS_DEFINE_CID(kSmtpServiceCID, NS_SMTPSERVICE_CID);
static NS_DEFINE_CID(kStandardUrlCID, NS_STANDARDURL_CID);

NS_IMPL_ISUPPORTS1(nsSmtpDelegateFactory, nsIRDFDelegateFactory)

nsSmtpDelegateFactory::nsSmtpDelegateFactory()
{
    NS_INIT_REFCNT();
}

nsSmtpDelegateFactory::~nsSmtpDelegateFactory()
{
}
    
NS_IMETHODIMP
nsSmtpDelegateFactory::CreateDelegate(nsIRDFResource *aOuter,
                                      const char *aKey,
                                      const nsIID & aIID, void * *aResult)
{

  nsresult rv;
  nsXPIDLCString uri;
  aOuter->GetValueConst(getter_Shares(uri));

  nsCOMPtr<nsIURL> url;
  rv = nsComponentManager::CreateInstance(kStandardUrlCID, nsnull,
                                          NS_GET_IID(nsIURL),
                                          (void **)getter_AddRefs(url));
  if (NS_FAILED(rv)) return rv;
  
  rv = url->SetSpec(uri);

  // seperate out username, hostname
  nsXPIDLCString hostname;
  nsXPIDLCString username;

  rv = url->GetPreHost(getter_Copies(username));
  NS_ENSURE_SUCCESS(rv, rv);
  
  url->GetHost(getter_Copies(hostname));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISmtpService> smtpService = do_GetService(kSmtpServiceCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISmtpServer> smtpServer;
  rv = smtpService->FindServer(username, hostname, getter_AddRefs(smtpServer));

  // no server, it's a failure!
  if (NS_FAILED(rv)) return rv;
  if (!smtpServer) return NS_ERROR_FAILURE;

  return smtpServer->QueryInterface(aIID, aResult);
}
