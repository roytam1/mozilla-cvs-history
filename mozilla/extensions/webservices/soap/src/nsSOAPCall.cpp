/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsSOAPCall.h"
#include "nsSOAPResponse.h"
#include "nsSOAPParameter.h"
#include "nsSOAPUtils.h"
#include "nsCRT.h"
#include "jsapi.h"
#include "nsISOAPParameter.h"
#include "nsISOAPTransport.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

/////////////////////////////////////////////
//
//
/////////////////////////////////////////////

nsSOAPCall::nsSOAPCall()
{
}

nsSOAPCall::~nsSOAPCall()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsSOAPCall, 
                   nsSOAPMessage,
                   nsISOAPCall)

/* attribute DOMString transportURI; */
NS_IMETHODIMP nsSOAPCall::GetTransportURI(nsAWritableString & aTransportURI)
{
  NS_ENSURE_ARG_POINTER(&aTransportURI);
  aTransportURI.Assign(mTransportURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPCall::SetTransportURI(const nsAReadableString & aTransportURI)
{
  mTransportURI.Assign(aTransportURI);
  return NS_OK;
}

nsresult
nsSOAPCall::GetTransport(nsISOAPTransport** aTransport)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  nsXPIDLCString protocol;
  nsCString transportURI(mTransportURI.ToNewCString());

  rv = NS_NewURI(getter_AddRefs(uri), transportURI.get());
  if (NS_FAILED(rv)) return rv;

  uri->GetScheme(getter_Copies(protocol));
  
  nsCAutoString transportContractid;
  transportContractid.Assign(NS_SOAPTRANSPORT_CONTRACTID_PREFIX);
  transportContractid.Append(protocol);

  nsCOMPtr<nsISOAPTransport> transport = do_GetService(transportContractid, &rv);
  if (NS_FAILED(rv)) return rv;

  *aTransport = transport.get();
  NS_ADDREF(*aTransport);

  return NS_OK;
}

/* nsISOAPResponse invoke (); */
NS_IMETHODIMP nsSOAPCall::Invoke(nsISOAPResponse **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;
  nsCOMPtr<nsISOAPTransport> transport;

  if (mTransportURI.Length() == 0) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  rv = GetTransport(getter_AddRefs(transport));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsISOAPResponse> response;
  response = new nsSOAPResponse();
  if (!response) return NS_ERROR_OUT_OF_MEMORY;

  rv = transport->SyncCall(mTransportURI,
                           NS_STATIC_CAST(nsSOAPMessage*, this),
                           response);
  if (NS_FAILED(rv)) return rv;

  return response->QueryInterface(NS_GET_IID(nsISOAPResponse), (void**)_retval);
}

/* void asyncInvoke (in nsISOAPResponseListener listener); */
NS_IMETHODIMP nsSOAPCall::AsyncInvoke(nsISOAPResponseListener *listener)
{
  nsresult rv;
  nsCOMPtr<nsISOAPTransport> transport;

  if (mTransportURI.Length() == 0) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  rv = GetTransport(getter_AddRefs(transport));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsISOAPResponse> response;
  response = new nsSOAPResponse();
  if (!response) return NS_ERROR_OUT_OF_MEMORY;

  rv = transport->AsyncCall(mTransportURI,
                           NS_STATIC_CAST(nsSOAPMessage*, this),
                           listener,
                           response);
  return rv;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPCall::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPCall))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPCall::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPCall))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPCall::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPCall))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPCall::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPCall))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
