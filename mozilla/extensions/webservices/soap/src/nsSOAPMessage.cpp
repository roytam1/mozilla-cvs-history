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

#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsIComponentManager.h"
#include "nsSOAPUtils.h"
#include "nsSOAPMessage.h"
#include "nsSOAPParameter.h"
#include "nsSOAPHeaderBlock.h"
#include "nsISOAPEncoding.h"
#include "nsIDOMDocument.h"
#include "nsIDOMParser.h"
#include "nsIDOMElement.h"
#include "nsIDOMNamedNodeMap.h"

static NS_DEFINE_CID(kDOMParserCID, NS_DOMPARSER_CID);
/////////////////////////////////////////////
//
//
/////////////////////////////////////////////
  
nsSOAPMessage::nsSOAPMessage()
{
  NS_INIT_ISUPPORTS();
}

nsSOAPMessage::~nsSOAPMessage()
{
}

NS_IMPL_ISUPPORTS3(nsSOAPMessage, 
                   nsISOAPMessage, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable)

/* attribute nsIDOMDocument message; */
NS_IMETHODIMP nsSOAPMessage::GetMessage(nsIDOMDocument * *aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  *aMessage = mMessage;
  NS_IF_ADDREF(*aMessage);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMessage(nsIDOMDocument * aMessage)
{
  mMessage = aMessage;
  return NS_OK;
}

/* readonly attribute nsIDOMElement envelope; */
NS_IMETHODIMP nsSOAPMessage::GetEnvelope(nsIDOMElement * *aEnvelope)
{
  NS_ENSURE_ARG_POINTER(aEnvelope);

  if (mMessage) {
    nsCOMPtr<nsIDOMElement> root;
    mMessage->GetDocumentElement(getter_AddRefs(root));
    if (root) {
      nsAutoString name, namespaceURI;
      root->GetLocalName(name);
      root->GetNamespaceURI(namespaceURI);
      if (name.Equals(nsSOAPUtils::kEnvelopeTagName)
        && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
      {
        *aEnvelope = root;
        NS_ADDREF(*aEnvelope);
        return NS_OK;
      }
    } 

  }
  *aEnvelope = nsnull;
  return NS_OK;
}

/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPMessage::GetHeader(nsIDOMElement * *aHeader)
{
  NS_ENSURE_ARG_POINTER(aHeader);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kHeaderTagName, 
      aHeader);
  }
  else {
    *aHeader = nsnull;
  }
  return NS_OK;
}

/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPMessage::GetBody(nsIDOMElement * *aBody)
{
  NS_ENSURE_ARG_POINTER(aBody);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kBodyTagName, 
      aBody);
  }
  else {
    *aBody = nsnull;
  }
  return NS_OK;
}

/* attribute DOMString actionURI; */
NS_IMETHODIMP nsSOAPMessage::GetActionURI(nsAWritableString & aActionURI)
{
  NS_ENSURE_ARG_POINTER(&aActionURI);
  aActionURI.Assign(mActionURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetActionURI(const nsAReadableString & aActionURI)
{
  mActionURI.Assign(aActionURI);
  return NS_OK;
}

/* readonly attribute AString methodName; */
NS_IMETHODIMP nsSOAPMessage::GetMethodName(nsAString & aMethodName)
{
  nsCOMPtr<nsIDOMElement> body;
  GetBody(getter_AddRefs(body));
  if (body) {
    nsCOMPtr<nsIDOMElement> method;
    nsSOAPUtils::GetFirstChildElement(body, getter_AddRefs(method));
    if (method) {
      body->GetLocalName(aMethodName);
      return NS_OK;
    }
  }
  aMethodName.SetLength(0);
  return NS_OK;
}

/* readonly attribute AString targetObjectURI; */
NS_IMETHODIMP nsSOAPMessage::GetTargetObjectURI(nsAString & aTargetObjectURI)
{
  nsCOMPtr<nsIDOMElement> body;
  GetBody(getter_AddRefs(body));
  if (body) {
    nsCOMPtr<nsIDOMElement> method;
    nsSOAPUtils::GetFirstChildElement(body, getter_AddRefs(method));
    if (method) {
      body->GetNamespaceURI(aTargetObjectURI);
      return NS_OK;
    }
  }
  aTargetObjectURI.SetLength(0);
  return NS_OK;
}

NS_NAMED_LITERAL_STRING(kEmptySOAPDocStr, "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\">"
"<SOAP-ENV:Header>"
"</SOAP-ENV:Header>"
"<SOAP-ENV:Body>"
"</SOAP-ENV:Body>"
"</SOAP-ENV:Envelope>");

/* void encode (in AString aMethodName, in AString aTargetObjectURI, in PRUint32 aHeaderBlockCount, [array, size_is (aHeaderBlockCount)] in nsISOAPHeaderBlock aHeaderBlocks, in PRUint32 aParameterCount, [array, size_is (aParameterCount)] in nsISOAPParameter aParameters); */
NS_IMETHODIMP nsSOAPMessage::Encode(const nsAString & aMethodName, const nsAString & aTargetObjectURI, PRUint32 aHeaderBlockCount, nsISOAPHeaderBlock **aHeaderBlocks, PRUint32 aParameterCount, nsISOAPParameter **aParameters)
{

//  Construct the message skeleton

  nsresult rv;
  nsCOMPtr<nsIDOMNode> ignored;
  nsCOMPtr<nsIDOMParser> parser = do_CreateInstance(kDOMParserCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsAutoString docstr;
  rv = parser->ParseFromString(kEmptySOAPDocStr.get(), "text/xml", 
		               getter_AddRefs(mMessage));
  if (NS_FAILED(rv)) return rv;

//  Declare the default encoding if one exists

  if (mEncoding) {
    nsCOMPtr<nsIDOMElement> envelope;
    rv = GetEnvelope(getter_AddRefs(envelope));
    if (NS_FAILED(rv)) return rv;
    if (envelope) {
      nsAutoString enc;
      mEncoding->GetStyleURI(enc);
      envelope->SetAttributeNS(nsSOAPUtils::kSOAPEncodingURI, nsSOAPUtils::kEncodingStyleAttribute, enc);
    }
  }

//  Encode and add headers, if any were specified 

  if (aHeaderBlockCount) {
    nsCOMPtr<nsIDOMElement> parent;
    rv = GetHeader(getter_AddRefs(parent));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsISupports> next;
    nsCOMPtr<nsISOAPHeaderBlock> header;
    nsCOMPtr<nsIDOMElement> element;
    nsCOMPtr<nsISOAPEncoding> encoding;
    nsCOMPtr<nsISchemaType> schemaType;
    nsCOMPtr<nsIVariant> value;
    nsAutoString name;
    nsAutoString namespaceURI;
    nsAutoString actorURI;
    for (PRUint32 i = 0; i < aHeaderBlockCount; i++) {
      header = aHeaderBlocks[i];
      if (!header) return NS_ERROR_FAILURE;
      rv = header->GetElement(getter_AddRefs(element));
      if (element) {
        nsCOMPtr<nsIDOMNode> node1 = (nsIDOMElement*)element;
        nsCOMPtr<nsIDOMNode> node2;
        rv = mMessage->ImportNode(node1, PR_TRUE, getter_AddRefs(node1));
        if (NS_FAILED(rv)) return rv;
        rv = parent->AppendChild(node2, getter_AddRefs(node1));
        if (NS_FAILED(rv)) return rv;
	element = do_QueryInterface(node1);
      }
      else {
        rv = header->GetNamespaceURI(namespaceURI);
        if (NS_FAILED(rv)) return rv;
        rv = header->GetName(name);
        if (NS_FAILED(rv)) return rv;
        rv = header->GetActorURI(actorURI);
        if (NS_FAILED(rv)) return rv;
        rv = header->GetEncoding(getter_AddRefs(encoding));
        if (NS_FAILED(rv)) return rv;
        if (!encoding) {
          encoding = mEncoding;
        }
        rv = header->GetSchemaType(getter_AddRefs(schemaType));
        if (NS_FAILED(rv)) return rv;
	rv = header->GetValue(getter_AddRefs(value));
        if (NS_FAILED(rv)) return rv;
        rv = encoding->Encode(value, namespaceURI, name,
		        schemaType, nsnull, parent, getter_AddRefs(element));
        if (NS_FAILED(rv)) return rv;
        if (!actorURI.IsEmpty()) {
	  element->SetAttributeNS(nsSOAPUtils::kSOAPEnvPrefix, nsSOAPUtils::kActorAttribute, actorURI);
        }
        if (mEncoding != encoding) {
          nsAutoString enc;
          encoding->GetStyleURI(enc);
          element->SetAttributeNS(nsSOAPUtils::kSOAPEncodingURI, nsSOAPUtils::kEncodingStyleAttribute, enc);
        }
      }
    }
  }
  nsCOMPtr<nsIDOMElement> body;
  rv = GetBody(getter_AddRefs(body));
  if (NS_FAILED(rv)) return rv;

//  Only produce a call element if mMethodName was non-empty

  if (!aMethodName.IsEmpty()) {
    nsCOMPtr<nsIDOMElement> call;
    rv = mMessage->CreateElementNS(aTargetObjectURI, aMethodName, getter_AddRefs(call));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDOMNode> ignored;
    rv = body->AppendChild(call, getter_AddRefs(ignored));
    if (NS_FAILED(rv)) return rv;
    body = call;
    nsAutoString prefix;
    rv = nsSOAPUtils::MakeNamespacePrefixFixed(body, aTargetObjectURI, prefix);
    if (NS_FAILED(rv)) return rv;
    if (!prefix.IsEmpty()) {
      rv = body->SetPrefix(prefix);
      if (NS_FAILED(rv)) return rv;
    }
  }

//  Encode and add all of the parameters into the body

  nsCOMPtr<nsISupports> next;
  nsCOMPtr<nsISOAPParameter> param;
  nsCOMPtr<nsIDOMElement> element;
  nsCOMPtr<nsISOAPEncoding> encoding;
  nsCOMPtr<nsISchemaType> schemaType;
  nsCOMPtr<nsIVariant> value;
  nsAutoString name;
  nsAutoString namespaceURI;
  for (PRUint32 i = 0; i < aParameterCount; i++) {
    param = aParameters[i];
    if (!param) return NS_ERROR_FAILURE;
    rv = param->GetElement(getter_AddRefs(element));
    if (element) {
      nsCOMPtr<nsIDOMNode> node1 = (nsIDOMElement*)element;
      nsCOMPtr<nsIDOMNode> node2;
      rv = mMessage->ImportNode(node1, PR_TRUE, getter_AddRefs(node1));
      if (NS_FAILED(rv)) return rv;
      rv = body->AppendChild(node2, getter_AddRefs(node1));
      if (NS_FAILED(rv)) return rv;
      element = do_QueryInterface(node1);
    }
    else {
      rv = param->GetNamespaceURI(namespaceURI);
      if (NS_FAILED(rv)) return rv;
      rv = param->GetName(name);
      if (NS_FAILED(rv)) return rv;
      rv = param->GetEncoding(getter_AddRefs(encoding));
      if (NS_FAILED(rv)) return rv;
      if (!encoding) {
        encoding = mEncoding;
      }
      rv = param->GetSchemaType(getter_AddRefs(schemaType));
      if (NS_FAILED(rv)) return rv;
      rv = param->GetValue(getter_AddRefs(value));
      if (NS_FAILED(rv)) return rv;
      rv = encoding->Encode(value, namespaceURI, name,
		      schemaType, nsnull, body, getter_AddRefs(element));
      if (NS_FAILED(rv)) return rv;
      if (mEncoding != encoding) {
        nsAutoString enc;
        encoding->GetStyleURI(enc);
        element->SetAttributeNS(nsSOAPUtils::kSOAPEncodingURI, nsSOAPUtils::kEncodingStyleAttribute, enc);
      }
    }
  }
  return NS_OK;
}

static NS_DEFINE_CID(kMemoryCID, NS_MEMORY_CID);

/* void getHeaderBlocks (out PRUint32 aCount, [array, size_is (aCount), retval] out nsISOAPHeaderBlock aHeaderBlocks); */
NS_IMETHODIMP nsSOAPMessage::GetHeaderBlocks(PRUint32 *aCount, nsISOAPHeaderBlock ***aHeaderBlocks)
{
  nsCOMPtr<nsIMemory> memory = do_GetService(kMemoryCID);
  *aCount = 0;
  *aHeaderBlocks = nsnull;
  int count = 0;
  int length = 0;
  nsCOMPtr<nsIDOMElement> element;
  nsresult rv = GetHeader(getter_AddRefs(element));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDOMElement> next;
  nsCOMPtr<nsISOAPHeaderBlock> header;
  nsSOAPUtils::GetFirstChildElement(element, getter_AddRefs(next));
  while (next) {
    if (length == count) {
      length = length ? 2 * length : 10;
      *aHeaderBlocks = (nsISOAPHeaderBlock* *)memory->Realloc(*aHeaderBlocks, length * sizeof(**aHeaderBlocks));
    }
    element = next;
    header = new nsSOAPHeaderBlock();
    if (NS_FAILED(rv)) return rv;
    NS_ADDREF(header);
    (*aHeaderBlocks)[(*aCount)++] = header;
    rv = header->SetElement(element);
    if (NS_FAILED(rv)) return rv;
    nsSOAPUtils::GetNextSiblingElement(element, getter_AddRefs(next));
  }
  if (*aCount) {
    *aHeaderBlocks = (nsISOAPHeaderBlock* *)memory->Realloc(*aHeaderBlocks, (*aCount) * sizeof(**aHeaderBlocks));
  }
  return NS_OK;
}

/* void getParameters (in boolean aDocumentStyle, out PRUint32 aCount, [array, size_is (aCount), retval] out nsISOAPParameter aParameters); */
NS_IMETHODIMP nsSOAPMessage::GetParameters(PRBool aDocumentStyle, PRUint32 *aCount, nsISOAPParameter ***aParameters)
{
  nsCOMPtr<nsIMemory> memory = do_GetService(kMemoryCID);
  *aCount = 0;
  *aParameters = nsnull;
  int count = 0;
  int length = 0;
  nsCOMPtr<nsIDOMElement> element;
  nsresult rv = GetHeader(getter_AddRefs(element));
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<nsIDOMElement> next;
  nsCOMPtr<nsISOAPParameter> param;
  nsSOAPUtils::GetFirstChildElement(element, getter_AddRefs(next));
  while (next) {
    if (length == count) {
      length = length ? 2 * length : 10;
      *aParameters = (nsISOAPParameter* *)memory->Realloc(*aParameters, length * sizeof(**aParameters));
    }
    element = next;
    param = new nsSOAPParameter();
    if (NS_FAILED(rv)) return rv;
    NS_ADDREF(param);
    (*aParameters)[(*aCount)++] = param;
    rv = param->SetElement(element);
    if (NS_FAILED(rv)) return rv;
    nsSOAPUtils::GetNextSiblingElement(element, getter_AddRefs(next));
  }
  if (*aCount) {
    *aParameters = (nsISOAPParameter* *)memory->Realloc(*aParameters, (*aCount) * sizeof(**aParameters));
  }
  return NS_OK;
}

/* attribute nsISOAPEncoding encoding; */
NS_IMETHODIMP nsSOAPMessage::GetEncoding(nsISOAPEncoding* * aEncoding)
{
  NS_ENSURE_ARG_POINTER(aEncoding);
  if (!mEncoding) {
    nsresult rv;
    mEncoding = do_CreateInstance(NS_SOAPENCODING_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return rv;
  }
  *aEncoding = mEncoding;
  NS_IF_ADDREF(*aEncoding);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetEncoding(nsISOAPEncoding* aEncoding)
{
  NS_ENSURE_ARG_POINTER(aEncoding);
  mEncoding = aEncoding;
  return NS_OK;
}

static const char*kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPMessage::CanCreateWrapper(const nsIID * iid, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPMessage::CanCallMethod(const nsIID * iid, const PRUnichar*methodName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanGetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanSetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
