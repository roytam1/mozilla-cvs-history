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

#include "nsSOAPResponse.h"
#include "nsSOAPUtils.h"
#include "nsSOAPFault.h"
#include "nsISOAPEncoder.h"
#include "jsapi.h"
#include "nsISOAPParameter.h"
#include "nsIComponentManager.h"
#include "nsMemory.h"
#include "nsReadableUtils.h"

nsSOAPResponse::nsSOAPResponse(nsIDOMDocument* aEnvelopeDocument)
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
  mEnvelopeDocument = aEnvelopeDocument;
  if (mEnvelopeDocument) {
    nsCOMPtr<nsIDOMElement> element;
    mEnvelopeDocument->GetDocumentElement(getter_AddRefs(element));
    if (!element) return;

    nsAutoString ns, name;
    element->GetNamespaceURI(ns);
    element->GetLocalName(name);
    
    if (ns.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kSOAPEnvURI)) &&
        name.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kEnvelopeTagName))) {
      mEnvelopeElement = element;
      nsSOAPUtils::GetFirstChildElement(mEnvelopeElement, 
                                        getter_AddRefs(element));
      if (!element) return;
      
      element->GetNamespaceURI(ns);
      element->GetLocalName(name);
      
      if (ns.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kSOAPEnvURI)) &&
          name.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kHeaderTagName))) {
        mHeaderElement = element;
          
        nsSOAPUtils::GetNextSiblingElement(mHeaderElement,
                                           getter_AddRefs(element));
        if (!element) return;
        
        element->GetNamespaceURI(ns);
        element->GetLocalName(name);
      }
      
      if (ns.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kSOAPEnvURI)) &&
          name.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kBodyTagName))) {
        mBodyElement = element;
        
        nsSOAPUtils::GetFirstChildElement(mBodyElement, 
                                          getter_AddRefs(element));
        if (!element) return;

        element->GetNamespaceURI(ns);
        element->GetLocalName(name);

        // XXX This assumes that the first body entry is either a fault
        // or a result and that the two are mutually exclusive 
        if (ns.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kSOAPEnvURI)) &&
            name.Equals(NS_ConvertASCIItoUCS2(nsSOAPUtils::kFaultTagName))) {
          mFaultElement = element;
        }
        else {
          mResultElement = element;
        }
      }
    }
  }
}

nsSOAPResponse::~nsSOAPResponse()
{
}

NS_IMPL_ISUPPORTS2(nsSOAPResponse, nsISOAPResponse, nsISecurityCheckedComponent)

/* readonly attribute nsIDOMElement envelope; */
NS_IMETHODIMP nsSOAPResponse::GetEnvelope(nsIDOMElement * *aEnvelope)
{
  NS_ENSURE_ARG_POINTER(aEnvelope);
  *aEnvelope = mEnvelopeElement;
  NS_IF_ADDREF(*aEnvelope);
  return NS_OK;
}

/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPResponse::GetHeader(nsIDOMElement * *aHeader)
{
  NS_ENSURE_ARG_POINTER(aHeader);
  *aHeader = mHeaderElement;
  NS_IF_ADDREF(*aHeader);
  return NS_OK;
}

/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPResponse::GetBody(nsIDOMElement * *aBody)
{
  NS_ENSURE_ARG_POINTER(aBody);
  *aBody = mBodyElement;
  NS_IF_ADDREF(*aBody);
  return NS_OK;
}

/* readonly attribute unsigned long status; */
NS_IMETHODIMP
nsSOAPResponse::GetStatus(PRUint32 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsSOAPResponse::SetStatus(PRUint32 aStatus)
{
  mStatus = aStatus;
  return NS_OK;
}

/* readonly attribute string targetObjectURI; */
NS_IMETHODIMP nsSOAPResponse::GetTargetObjectURI(char * *aTargetObjectURI)
{
  NS_ENSURE_ARG_POINTER(aTargetObjectURI);
  *aTargetObjectURI = nsnull;
  if (mResultElement) {
    nsAutoString ns;
    mResultElement->GetNamespaceURI(ns);
    if (ns.Length() > 0) {
      *aTargetObjectURI = ToNewCString(ns);
    }
  }
  return NS_OK;
}

/* readonly attribute string methodName; */
NS_IMETHODIMP nsSOAPResponse::GetMethodName(char * *aMethodName)
{
  NS_ENSURE_ARG_POINTER(aMethodName);
  *aMethodName = nsnull;
  if (mResultElement) {
    nsAutoString localName;
    mResultElement->GetLocalName(localName);
    if (localName.Length() > 0) {
      *aMethodName = ToNewCString(localName);
    }
  }
  return NS_OK;
}

/* boolean generatedFault (); */
NS_IMETHODIMP nsSOAPResponse::GeneratedFault(PRBool *_retval)
{
  NS_ENSURE_ARG(_retval);
  if (mFaultElement) {
    *_retval = PR_TRUE;
  }
  else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}

/* readonly attribute nsISOAPFault fault; */
NS_IMETHODIMP nsSOAPResponse::GetFault(nsISOAPFault * *aFault)
{
  NS_ENSURE_ARG_POINTER(aFault);
  *aFault = nsnull;
  if (mFaultElement) {
    nsSOAPFault* fault = new nsSOAPFault(mFaultElement);
    if (!fault) return NS_ERROR_OUT_OF_MEMORY;

    return fault->QueryInterface(NS_GET_IID(nsISOAPFault), (void**)aFault);
  }
  return NS_OK;
}

/* readonly attribute nsISOAPParameter returnValue; */
NS_IMETHODIMP nsSOAPResponse::GetReturnValue(nsISOAPParameter * *aReturnValue)
{
  NS_ENSURE_ARG_POINTER(aReturnValue);
  nsresult rv;

  *aReturnValue = nsnull;

  if (mResultElement) {
    // The first child element is assumed to be the returned
    // value
    nsCOMPtr<nsIDOMElement> value;
    nsSOAPUtils::GetFirstChildElement(mResultElement, getter_AddRefs(value));

    // Get the inherited encoding style starting from the
    // value
    char* encodingStyle;
    nsSOAPUtils::GetInheritedEncodingStyle(value, 
                                           &encodingStyle);
    
    if (!encodingStyle) {
      encodingStyle = nsCRT::strdup(nsSOAPUtils::kSOAPEncodingURI);
    }
    
    // Find the corresponding encoder
    nsCAutoString encoderContractid;
    encoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
    encoderContractid.Append(encodingStyle);

    nsCOMPtr<nsISOAPEncoder> encoder = do_CreateInstance(encoderContractid);
    if (!encoder) {
      nsMemory::Free(encodingStyle);
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    // Convert the result element to a parameter
    nsCOMPtr<nsISOAPParameter> param;
    rv = encoder->ElementToParameter(value,
                                     encodingStyle,
                                     nsISOAPParameter::PARAMETER_TYPE_UNKNOWN,
                                     getter_AddRefs(param));
    nsMemory::Free(encodingStyle);
    if (NS_FAILED(rv)) return rv;

    *aReturnValue = param;
    NS_IF_ADDREF(*aReturnValue);
  }

  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPResponse::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPResponse::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPResponse::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPResponse::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
