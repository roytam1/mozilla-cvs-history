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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsString.h"
#include "nsISOAPParameter.h"
#include "nsSOAPMessage.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "nsSOAPEncoding.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIDOMNodeList.h"
#include "nsISchema.h"
#include "nsISchemaLoader.h"
#include "nsSOAPUtils.h"

//  First comes the registry which shares between associated encodings but is never seen by xpconnect.

NS_IMPL_ISUPPORTS1(nsSOAPEncodingRegistry,nsISOAPEncoding)

nsSOAPEncodingRegistry::nsSOAPEncodingRegistry(nsISOAPEncoding* aEncoding): mEncodings(new nsSupportsHashtable) 
{
  NS_INIT_ISUPPORTS();

  nsAutoString style;
  aEncoding->GetStyleURI(style);
  nsStringKey styleKey(style);
  mEncodings->Put(&styleKey, aEncoding);
  /* member initializers and constructor code */
}

nsSOAPEncodingRegistry::~nsSOAPEncodingRegistry()
{
  /* destructor code */
  delete mEncodings;
}

nsresult nsSOAPEncodingRegistry::GetAssociatedEncoding(const nsAString& aStyleURI, PRBool aCreateIf, nsISOAPEncoding* * aEncoding)
{
  NS_SOAP_ENSURE_ARG_STRING(aStyleURI);
  NS_ENSURE_ARG_POINTER(aEncoding);
  nsStringKey styleKey(aStyleURI);
  *aEncoding = (nsISOAPEncoding*)mEncodings->Get(&styleKey);
  if (!*aEncoding)
  {
    nsCOMPtr<nsISOAPEncoding> defaultEncoding;
    nsCAutoString encodingContractid;
    encodingContractid.Assign(NS_SOAPENCODING_CONTRACTID_PREFIX);
    encodingContractid.Append(NS_ConvertUCS2toUTF8(aStyleURI));
    defaultEncoding = do_GetService(encodingContractid.get());
    if (defaultEncoding || aCreateIf) {
      *aEncoding = new nsSOAPEncoding(aStyleURI, this, defaultEncoding);
      mEncodings->Put(&styleKey, *aEncoding);
    }
  }
  return NS_OK;
}
nsresult nsSOAPEncodingRegistry::SetSchemaCollection(nsISchemaCollection* aSchemaCollection)
{
  NS_ENSURE_ARG(aSchemaCollection);
  mSchemaCollection = aSchemaCollection;
  return NS_OK;
}
nsresult nsSOAPEncodingRegistry::GetSchemaCollection(nsISchemaCollection** aSchemaCollection)
{
  NS_ENSURE_ARG_POINTER(aSchemaCollection);
  if (!mSchemaCollection) {
    nsresult rv;
    nsCOMPtr<nsISchemaLoader>loader = do_CreateInstance(NS_SCHEMALOADER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    mSchemaCollection = do_QueryInterface(loader);
    if (!mSchemaCollection) return NS_ERROR_FAILURE;
  }
  *aSchemaCollection = mSchemaCollection;
  NS_ADDREF(*aSchemaCollection);
  return NS_OK;
}

/* readonly attribute AString styleURI; */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetStyleURI(nsAString & aStyleURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPEncoder setEncoder (in AString aKey, in nsISOAPEncoder aEncoder); */
NS_IMETHODIMP nsSOAPEncodingRegistry::SetEncoder(const nsAString & aKey, nsISOAPEncoder *aEncoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPEncoder getEncoder (in AString aKey); */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetEncoder(const nsAString & aKey, nsISOAPEncoder **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPDecoder setDecoder (in AString aKey, in nsISOAPDecoder aDecoder); */
NS_IMETHODIMP nsSOAPEncodingRegistry::SetDecoder(const nsAString & aKey, nsISOAPDecoder *aDecoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISOAPDecoder getDecoder (in AString aKey); */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetDecoder(const nsAString & aKey, nsISOAPDecoder **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISOAPEncoder defaultEncoder; */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetDefaultEncoder(nsISOAPEncoder * *aDefaultEncoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsSOAPEncodingRegistry::SetDefaultEncoder(nsISOAPEncoder * aDefaultEncoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISOAPDecoder defaultDecoder; */
NS_IMETHODIMP nsSOAPEncodingRegistry::GetDefaultDecoder(nsISOAPDecoder * *aDefaultDecoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsSOAPEncodingRegistry::SetDefaultDecoder(nsISOAPDecoder * aDefaultDecoder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMElement encode (in nsIVariant aSource, in AString aNamespaceURI, in AString aName, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments, in nsIDOMElement aDestination); */
NS_IMETHODIMP nsSOAPEncodingRegistry::Encode(nsIVariant *aSource, const nsAString & aNamespaceURI, const nsAString & aName, nsISchemaType *aSchemaType, nsISOAPAttachments *aAttachments, nsIDOMElement *aDestination, nsIDOMElement **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant decode (in nsIDOMElement aSource, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments); */
NS_IMETHODIMP nsSOAPEncodingRegistry::Decode(nsIDOMElement *aSource, nsISchemaType *aSchemaType, nsISOAPAttachments *aAttachments, nsIVariant **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//  Second, we create the encodings themselves.

NS_IMPL_ISUPPORTS2_CI(nsSOAPEncoding, nsISOAPEncoding, nsISecurityCheckedComponent)

nsSOAPEncoding::nsSOAPEncoding(): mEncoders(new nsSupportsHashtable),
  mDecoders(new nsSupportsHashtable)
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */

  mStyleURI.Assign(nsSOAPUtils::kSOAPEncodingURI);
  mDefaultEncoding = do_GetService(NS_DEFAULTSOAPENCODER_CONTRACTID);
  mRegistry = new nsSOAPEncodingRegistry(this);
}

nsSOAPEncoding::nsSOAPEncoding(const nsAString& aStyleURI, nsSOAPEncodingRegistry* aRegistry, nsISOAPEncoding* aDefaultEncoding)
	: mEncoders(new nsSupportsHashtable), mDecoders(new nsSupportsHashtable)
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */

  mStyleURI.Assign(aStyleURI);
  mRegistry = aRegistry;
  mDefaultEncoding = aDefaultEncoding;
}

nsSOAPEncoding::~nsSOAPEncoding()
{
  /* destructor code */
  delete mEncoders;
  delete mDecoders;
}

nsresult nsSOAPEncoding::SetSchemaCollection(nsISchemaCollection* aSchemaCollection)
{
  NS_ENSURE_ARG(aSchemaCollection);
  return mRegistry->SetSchemaCollection(aSchemaCollection);
}
nsresult nsSOAPEncoding::GetSchemaCollection(nsISchemaCollection** aSchemaCollection)
{
  NS_ENSURE_ARG_POINTER(aSchemaCollection);
  return mRegistry->GetSchemaCollection(aSchemaCollection);
}

/* readonly attribute AString styleURI; */
NS_IMETHODIMP nsSOAPEncoding::GetStyleURI(nsAString & aStyleURI)
{
  NS_SOAP_ENSURE_ARG_STRING(aStyleURI);
  aStyleURI.Assign(mStyleURI);
  return NS_OK;
}

/* nsISOAPEncoding getAssociatedEncoding (in AString aStyleURI, in boolean aCreateIf); */
NS_IMETHODIMP nsSOAPEncoding::GetAssociatedEncoding(const nsAString & aStyleURI, PRBool aCreateIf, nsISOAPEncoding **_retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aStyleURI);
  NS_ENSURE_ARG_POINTER(_retval);
  return mRegistry->GetAssociatedEncoding(aStyleURI, aCreateIf, _retval);
}

/* nsISOAPEncoder setEncoder (in AString aKey, in nsISOAPEncoder aEncoder); */
NS_IMETHODIMP nsSOAPEncoding::SetEncoder(const nsAString & aKey, nsISOAPEncoder *aEncoder)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG(aEncoder);
  nsStringKey nameKey(aKey);
  if (aEncoder) {
    mEncoders->Put(&nameKey, aEncoder, nsnull);
  }
  else {
    mEncoders->Remove(&nameKey, nsnull);
  }
  return NS_OK;
}

/* nsISOAPEncoder getEncoder (in AString aKey); */
NS_IMETHODIMP nsSOAPEncoding::GetEncoder(const nsAString & aKey, nsISOAPEncoder **_retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG_POINTER(_retval);
  nsStringKey nameKey(aKey);
  *_retval = (nsISOAPEncoder*)mEncoders->Get(&nameKey);
  if (*_retval == nsnull && mDefaultEncoding != nsnull) {
    return mDefaultEncoding->GetEncoder(aKey, _retval);
  }
  return NS_OK;
}

/* nsISOAPDecoder setDecoder (in AString aKey, in nsISOAPDecoder aDecoder); */
NS_IMETHODIMP nsSOAPEncoding::SetDecoder(const nsAString & aKey, nsISOAPDecoder *aDecoder)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG(aDecoder);
  nsStringKey nameKey(aKey);
  if (aDecoder) {
    mDecoders->Put(&nameKey, aDecoder, nsnull);
  }
  else {
    mDecoders->Remove(&nameKey, nsnull);
  }
  return NS_OK;
}

/* nsISOAPDecoder getDecoder (in AString aKey); */
NS_IMETHODIMP nsSOAPEncoding::GetDecoder(const nsAString & aKey, nsISOAPDecoder **_retval)
{
  NS_SOAP_ENSURE_ARG_STRING(aKey);
  NS_ENSURE_ARG_POINTER(_retval);
  nsStringKey nameKey(aKey);
  *_retval = (nsISOAPDecoder*)mDecoders->Get(&nameKey);
  if (*_retval == nsnull && mDefaultEncoding != nsnull) {
    return mDefaultEncoding->GetDecoder(aKey, _retval);
  }
  return NS_OK;
}

/* nsIDOMElement encode (in nsIVariant aSource, in AString aNamespaceURI, in AString aName, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments, in nsIDOMElement aDestination); */
NS_IMETHODIMP nsSOAPEncoding::Encode(nsIVariant *aSource, const nsAString & aNamespaceURI, const nsAString & aName, nsISchemaType *aSchemaType, nsISOAPAttachments *aAttachments, nsIDOMElement *aDestination, nsIDOMElement **_retval)
{
  NS_ENSURE_ARG(aSource);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsISOAPEncoder> encoder;
  nsresult rv = GetDefaultEncoder(getter_AddRefs(encoder));
  if (NS_FAILED(rv)) return rv;
  if (encoder) {
    return encoder->Encode(this, aSource, aNamespaceURI, aName, aSchemaType, aAttachments, aDestination,
		    _retval);
  }
  *_retval = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant decode (in nsIDOMElement aSource, in nsISchemaType aSchemaType, in nsISOAPAttachments aAttachments); */
NS_IMETHODIMP nsSOAPEncoding::Decode(nsIDOMElement *aSource, nsISchemaType *aSchemaType, nsISOAPAttachments *aAttachments, nsIVariant **_retval)
{
  NS_ENSURE_ARG(aSource);
  NS_ENSURE_ARG_POINTER(_retval);
  nsCOMPtr<nsISOAPDecoder> decoder;
  nsresult rv = GetDefaultDecoder(getter_AddRefs(decoder));
  if (NS_FAILED(rv)) return rv;
  if (decoder) {
    return decoder->Decode(this, aSource, aSchemaType, aAttachments, _retval);
  }
  *_retval = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISOAPEncoder defaultEncoder; */
NS_IMETHODIMP nsSOAPEncoding::GetDefaultEncoder(nsISOAPEncoder * *aDefaultEncoder)
{
  NS_ENSURE_ARG_POINTER(aDefaultEncoder);
  if (mDefaultEncoding && !mDefaultEncoder) {
    return mDefaultEncoding->GetDefaultEncoder(aDefaultEncoder);
  }
  *aDefaultEncoder = mDefaultEncoder;
  NS_IF_ADDREF(*aDefaultEncoder);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPEncoding::SetDefaultEncoder(nsISOAPEncoder * aDefaultEncoder)
{
  mDefaultEncoder = aDefaultEncoder;
  return NS_OK;
}

/* attribute nsISOAPDecoder defaultDecoder; */
NS_IMETHODIMP nsSOAPEncoding::GetDefaultDecoder(nsISOAPDecoder * *aDefaultDecoder)
{
  NS_ENSURE_ARG_POINTER(aDefaultDecoder);
  if (mDefaultEncoding && !mDefaultDecoder) {
    return mDefaultEncoding->GetDefaultDecoder(aDefaultDecoder);
  }
  *aDefaultDecoder = mDefaultDecoder;
  NS_IF_ADDREF(*aDefaultDecoder);
  return NS_OK;
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsSOAPEncoding::SetDefaultDecoder(nsISOAPDecoder * aDefaultDecoder)
{
  mDefaultDecoder = aDefaultDecoder;
  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPEncoding::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncoding))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPEncoding::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncoding))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPEncoding::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncoding))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPEncoding::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncoding))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
