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
#include "nsReadableUtils.h"
#include "nsSOAPBlock.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsISOAPAttachments.h"

nsSOAPBlock::nsSOAPBlock()
{
  NS_INIT_ISUPPORTS();
}

nsSOAPBlock::nsSOAPBlock(nsISOAPAttachments* aAttachments)
{
  NS_INIT_ISUPPORTS();
  mAttachments = aAttachments;
}

nsSOAPBlock::~nsSOAPBlock()
{
}

NS_IMPL_ISUPPORTS3(nsSOAPBlock, 
                      nsISOAPBlock, 
                      nsISecurityCheckedComponent,
                      nsIJSNativeInitializer)

/* attribute AString namespaceURI; */
NS_IMETHODIMP nsSOAPBlock::GetNamespaceURI(nsAWritableString & aNamespaceURI)
{
  NS_ENSURE_ARG_POINTER(&aNamespaceURI);
  if (mElement) {
    return mElement->GetNamespaceURI(aNamespaceURI);
  }
  else {
    aNamespaceURI.Assign(mNamespaceURI);
  }
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetNamespaceURI(const nsAReadableString & aNamespaceURI)
{
  nsresult rc = SetElement(nsnull);
  if (NS_FAILED(rc)) return rc;
  mNamespaceURI.Assign(aNamespaceURI);
  return NS_OK;
}

/* attribute AString name; */
NS_IMETHODIMP nsSOAPBlock::GetName(nsAWritableString & aName)
{
  NS_ENSURE_ARG_POINTER(&aName);
  if (mElement) {
    return mElement->GetLocalName(aName);
  }
  else {
    aName.Assign(mName);
  }
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetName(const nsAReadableString & aName)
{
  nsresult rc = SetElement(nsnull);
  if (NS_FAILED(rc)) return rc;
  mName.Assign(aName);
  return NS_OK;
}

/* attribute nsISOAPEncoding encoding; */
NS_IMETHODIMP nsSOAPBlock::GetEncoding(nsISOAPEncoding* * aEncoding)
{
  NS_ENSURE_ARG_POINTER(aEncoding);
  *aEncoding = mEncoding;
  NS_IF_ADDREF(*aEncoding);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetEncoding(nsISOAPEncoding* aEncoding)
{
  mEncoding = aEncoding;
  mComputeValue = PR_TRUE;
  return NS_OK;
}

/* attribute nsISchemaType schemaType; */
NS_IMETHODIMP nsSOAPBlock::GetSchemaType(nsISchemaType* * aSchemaType)
{
  NS_ENSURE_ARG_POINTER(aSchemaType);
  *aSchemaType = mSchemaType;
  NS_IF_ADDREF(*aSchemaType);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetSchemaType(nsISchemaType* aSchemaType)
{
  mSchemaType = aSchemaType;
  mComputeValue = PR_TRUE;
  return NS_OK;
}

/* attribute nsIDOMElement element; */
NS_IMETHODIMP nsSOAPBlock::GetElement(nsIDOMElement* * aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  *aElement = mElement;
  NS_IF_ADDREF(*aElement);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetElement(nsIDOMElement* aElement)
{
  mElement = aElement;
  mComputeValue = PR_TRUE;
  return NS_OK;
}

/* attribute nsIVariant value; */
NS_IMETHODIMP nsSOAPBlock::GetValue(nsIVariant* * aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  if (mElement) {    //  Check for auto-computation
    if (mComputeValue) {
      mComputeValue = PR_FALSE;
      if (mEncoding) {
        mStatus = mEncoding->Decode(mElement, mSchemaType, mAttachments, getter_AddRefs(mValue));
      }
      else {
	mStatus = NS_ERROR_NOT_INITIALIZED;
      }
    }
  }
  *aValue = mValue;
  NS_IF_ADDREF(*aValue);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPBlock::SetValue(nsIVariant* aValue)
{
  nsresult rc = SetElement(nsnull);
  if (NS_FAILED(rc)) return rc;
  mValue = aValue;
  return NS_OK;
}

NS_IMETHODIMP 
nsSOAPBlock::Initialize(JSContext *cx, JSObject *obj, 
                            PRUint32 argc, jsval *argv)
{

//  Get the arguments.

  nsCOMPtr<nsIVariant>  value;
  nsAutoString          name;
  nsAutoString          namespaceURI;
  nsCOMPtr<nsISupports> schemaType;
  nsCOMPtr<nsISupports> encoding;

  if (!JS_ConvertArguments(cx, argc, argv, "/%iv %is %is %ip %ip", 
    getter_AddRefs(value), 
    NS_STATIC_CAST(nsAString*, &name), 
    NS_STATIC_CAST(nsAString*, &namespaceURI), 
    getter_AddRefs(schemaType),
    getter_AddRefs(encoding))) return NS_ERROR_ILLEGAL_VALUE;

  nsresult rc = SetValue(value);
  if (NS_FAILED(rc)) return rc;
  rc = SetName(name);
  if (NS_FAILED(rc)) return rc;
  rc = SetNamespaceURI(namespaceURI);
  if (NS_FAILED(rc)) return rc;
  if (schemaType) {
    nsCOMPtr<nsISchemaType> v = do_QueryInterface(schemaType, &rc);
    if (NS_FAILED(rc)) return rc;
    rc = SetSchemaType(v);
    if (NS_FAILED(rc)) return rc;
  }
  if (encoding) {
    nsCOMPtr<nsISOAPEncoding> v = do_QueryInterface(encoding, &rc);
    if (NS_FAILED(rc)) return rc;
    rc = SetEncoding(v);
    if (NS_FAILED(rc)) return rc;
  }

  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPBlock::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPBlock))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPBlock::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPBlock))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPBlock::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPBlock))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPBlock::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPBlock))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
