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

#include "nsSOAPUtils.h"
#include "nsIDOMText.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsCOMPtr.h"

NS_NAMED_LITERAL_STRING(kSOAPEnvURI,"http://schemas.xmlsoap.org/soap/envelope/");
NS_NAMED_LITERAL_STRING(kSOAPEncodingURI,"http://schemas.xmlsoap.org/soap/encoding/");
NS_NAMED_LITERAL_STRING(kSOAPEnvPrefix,"SOAP-ENV");
NS_NAMED_LITERAL_STRING(kSOAPEncodingPrefix,"SOAP-ENC");
NS_NAMED_LITERAL_STRING(kXSIURI,"http://www.w3.org/1999/XMLSchema-instance");
NS_NAMED_LITERAL_STRING(kXSDURI,"http://www.w3.org/1999/XMLSchema");
NS_NAMED_LITERAL_STRING(kXSIPrefix,"xsi");
NS_NAMED_LITERAL_STRING(kXSITypeAttribute,"type");
NS_NAMED_LITERAL_STRING(kXSDPrefix,"xsd");
NS_NAMED_LITERAL_STRING(kEncodingStyleAttribute,"encodingStyle");
NS_NAMED_LITERAL_STRING(kEnvelopeTagName,"Envelope");
NS_NAMED_LITERAL_STRING(kHeaderTagName,"Header");
NS_NAMED_LITERAL_STRING(kBodyTagName,"Body");
NS_NAMED_LITERAL_STRING(kFaultTagName,"Fault");
NS_NAMED_LITERAL_STRING(kFaultCodeTagName,"faultcode");
NS_NAMED_LITERAL_STRING(kFaultStringTagName,"faultstring");
NS_NAMED_LITERAL_STRING(kFaultActorTagName,"faultactor");
NS_NAMED_LITERAL_STRING(kFaultDetailTagName,"detail");

NS_NAMED_LITERAL_STRING(kEmpty,"");
NS_NAMED_LITERAL_STRING(kTypeSeparator,"#");

NS_NAMED_LITERAL_STRING(kStringType,"#string");
NS_NAMED_LITERAL_STRING(kBooleanType,"#boolean");
NS_NAMED_LITERAL_STRING(kDoubleType,"#double");
NS_NAMED_LITERAL_STRING(kFloatType,"#float");
NS_NAMED_LITERAL_STRING(kLongType,"#long");
NS_NAMED_LITERAL_STRING(kIntType,"#int");
NS_NAMED_LITERAL_STRING(kShortType,"#short");
NS_NAMED_LITERAL_STRING(kByteType,"#byte");
NS_NAMED_LITERAL_STRING(kArrayType,"#array");
NS_NAMED_LITERAL_STRING(kStructType,"#struct");
NS_NAMED_LITERAL_STRING(kLiteralType,"#literal");
NS_NAMED_LITERAL_STRING(kNullType,"#null");
NS_NAMED_LITERAL_STRING(kVoidType,"#void");
NS_NAMED_LITERAL_STRING(kUnknownType,"#unknown");

NS_NAMED_LITERAL_STRING(kStringSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#string");
NS_NAMED_LITERAL_STRING(kBooleanSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#boolean");
NS_NAMED_LITERAL_STRING(kDoubleSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#double");
NS_NAMED_LITERAL_STRING(kFloatSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#float");
NS_NAMED_LITERAL_STRING(kLongSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#long");
NS_NAMED_LITERAL_STRING(kIntSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#int");
NS_NAMED_LITERAL_STRING(kShortSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#short");
NS_NAMED_LITERAL_STRING(kByteSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#byte");
NS_NAMED_LITERAL_STRING(kArraySchemaType,"#schema#http://www.w3.org/1999/XMLSchema#array");
NS_NAMED_LITERAL_STRING(kStructSchemaType,"#schema#http://www.w3.org/1999/XMLSchema#struct");
NS_NAMED_LITERAL_STRING(kLiteralSchemaType,"#literal");
NS_NAMED_LITERAL_STRING(kNullSchemaType,"#null");
NS_NAMED_LITERAL_STRING(kVoidSchemaType,"#void");
NS_NAMED_LITERAL_STRING(kUnknownSchemaType,"#unknown");

NS_NAMED_LITERAL_STRING(kStructTypePrefix,"#struct#");
NS_NAMED_LITERAL_STRING(kXMLSchemaSchemaTypePrefix, "#schema#");
NS_NAMED_LITERAL_STRING(kXMLNameSchemaTypePrefix, "#name#");

NS_NAMED_LITERAL_STRING(kXMLNamespaceNamespaceURI, "htp://www.w3.org/2000/xmlns/");
NS_NAMED_LITERAL_STRING(kXMLNamespaceURI, "htp://www.w3.org/XML/1998/namespace");
NS_NAMED_LITERAL_STRING(kXMLPrefix, "xml:");
NS_NAMED_LITERAL_STRING(kXMLNamespacePrefix, "xmlns:");

void 
nsSOAPUtils::GetSpecificChildElement(
  nsIDOMElement *aParent, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  GetFirstChildElement(aParent, getter_AddRefs(sibling));
  if (sibling)
  {
    GetSpecificSiblingElement(sibling,
      aNamespace, aType, aElement);
  }
}

void 
nsSOAPUtils::GetSpecificSiblingElement(
  nsIDOMElement *aSibling, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  sibling = aSibling;
  do {
    nsAutoString name, namespaceURI;
    sibling->GetLocalName(name);
    sibling->GetNamespaceURI(namespaceURI);
    if (name.Equals(aType)
      && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
    {
      *aElement = sibling;
      NS_ADDREF(*aElement);
      return;
    }
    nsCOMPtr<nsIDOMElement> temp = sibling;
    GetNextSiblingElement(temp, getter_AddRefs(sibling));
  } while (sibling);
}

void
nsSOAPUtils::GetFirstChildElement(nsIDOMElement* aParent, 
                                  nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  *aElement = nsnull;
  aParent->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      child->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
}

void
nsSOAPUtils::GetNextSiblingElement(nsIDOMElement* aStart, 
                                   nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> sibling;

  *aElement = nsnull;
  GetNextSibling(aStart, getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      sibling->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = sibling;
    GetNextSibling(temp, getter_AddRefs(sibling));
  }
}

void 
nsSOAPUtils::GetElementTextContent(nsIDOMElement* aElement, 
                                   nsAWritableString& aText)
{
  nsCOMPtr<nsIDOMNode> child;
  nsAutoString rtext;
  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::TEXT_NODE == type
        || nsIDOMNode::CDATA_SECTION_NODE == type) {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(child);
      nsAutoString data;
      text->GetData(data);
      rtext.Append(data);
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
  aText.Assign(rtext);
}

PRBool
nsSOAPUtils::HasChildElements(nsIDOMElement* aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      return PR_TRUE;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }

  return PR_FALSE;
}

void
nsSOAPUtils::GetNextSibling(nsIDOMNode* aSibling, nsIDOMNode **aNext)
{
  nsCOMPtr<nsIDOMNode> last;
  nsCOMPtr<nsIDOMNode> current;
  PRUint16 type;

  *aNext = nsnull;
  last = aSibling;

  last->GetNodeType(&type);
  if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
    last->GetFirstChild(getter_AddRefs(current));
    if (!last)
    {
      last->GetNextSibling(getter_AddRefs(current));
    }
  }
  else {
    last->GetNextSibling(getter_AddRefs(current));
  }
  while (!current)
  {
    last->GetParentNode(getter_AddRefs(current));
    current->GetNodeType(&type);
    if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
      last = current;
      last->GetNextSibling(getter_AddRefs(current));
    }
    else {
      current = nsnull;
      break;
    }
  }
  *aNext = current;
  NS_IF_ADDREF(*aNext);
}
nsresult nsSOAPUtils::GetNamespaceURI(nsIDOMElement* aScope,
                                  const nsAReadableString & aQName, 
                                  nsAWritableString & aURI)
{
  aURI.Truncate(0);
  PRInt32 i = aQName.FindChar(':');
  if (i < 0) {
    return NS_OK;
  }
  nsAutoString prefix;
  aQName.Left(prefix, i);

  if (prefix.Equals(kXMLPrefix)) {
    aURI.Assign(kXMLNamespaceURI);
    return NS_OK;
  }

  nsresult rc;
  nsCOMPtr<nsIDOMNode> current = aScope;
  nsCOMPtr<nsIDOMNamedNodeMap> attrs;
  nsCOMPtr<nsIDOMNode> temp;
  nsAutoString value;
  while (current != nsnull) {
    rc = current->GetAttributes(getter_AddRefs(attrs));
    if (NS_FAILED(rc)) return rc;
    if (attrs) {
      rc = attrs->GetNamedItemNS(kXMLNamespaceNamespaceURI, prefix, getter_AddRefs(temp));
      if (NS_FAILED(rc)) return rc;
      if (temp != nsnull)
	return temp->GetNodeValue(aURI);
    }
    rc = current->GetParentNode(getter_AddRefs(temp));
    if (NS_FAILED(rc)) return rc;
    current = temp;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsSOAPUtils::GetLocalName(const nsAReadableString & aQName, 
                                  nsAWritableString & aLocalName)
{
  PRInt32 i = aQName.FindChar(':');
  if (i < 0)
    aLocalName = aQName;
  else
    aQName.Mid(aLocalName, i, aQName.Length() - i);
  return NS_OK;
}

nsresult 
MakeNamespacePrefix(nsIDOMElement* aScope,
                   const nsAReadableString & aURI,
                   nsAWritableString & aPrefix)
{
//  This may change for level 3 serialization, so be sure to gut this
//  and call the standardized level 3 method when it is available.
  aPrefix.Truncate(0);
  if (aURI.IsEmpty())
    return NS_OK;
  if (aURI.Equals(kXMLNamespaceURI)) {
    aPrefix.Assign(kXMLPrefix);
    return NS_OK;
  }
  nsCOMPtr<nsIDOMNode> current = aScope;
  nsCOMPtr<nsIDOMNamedNodeMap> attrs;
  nsCOMPtr<nsIDOMNode> temp;
  nsAutoString tstr;
  nsresult rc;
  PRUint32 maxns = 0;	//  Keep track of max generated NS
  for (;;) {
    rc = current->GetAttributes(getter_AddRefs(attrs));
    if (NS_FAILED(rc)) return rc;
    if (attrs) {
      PRUint32 i = 0;
      for (;;)
      {
        attrs->Item(i++, getter_AddRefs(temp));
        if (!temp)
          break;
        temp->GetNamespaceURI(tstr);
        if (!tstr.Equals(kXMLNamespaceNamespaceURI))
          continue;
        temp->GetNodeValue(tstr);
        if (tstr.Equals(aURI)) {
          nsAutoString prefix;
          rc = temp->GetLocalName(prefix);
          if (NS_FAILED(rc)) return rc;
          nsCOMPtr<nsIDOMNode> check = aScope;
          PRBool hasDecl;
	  nsCOMPtr<nsIDOMElement> echeck;
          while (check != current) { // Make sure prefix is not overridden
	    echeck = do_QueryInterface(check);
	    if (echeck) {
	      rc = echeck->HasAttributeNS(kXMLNamespaceNamespaceURI, prefix, &hasDecl);
              if (NS_FAILED(rc)) return rc;
	      if (hasDecl)
	        break;
              current->GetParentNode(getter_AddRefs(temp));
	      current = temp;
	    }
          }
          if (check == current) {
	    aPrefix.Assign(prefix);
	    return NS_OK;
          }
	}
	rc = temp->GetLocalName(tstr);
        if (NS_FAILED(rc)) 
	  return rc;
	else {	//  Decode the generated namespace into a number
          nsReadingIterator<PRUnichar> i1;
          nsReadingIterator<PRUnichar> i2;
	  tstr.BeginReading(i1);
	  tstr.EndReading(i2);
	  if (i1 == i2 || *i1 != 'n') 
	    continue;
	  i1++;
	  if (i1 == i2 || *i1 != 's') 
	    continue;
	  i1++;
	  PRUint32 n = 0;
	  while (i1 != i2) {
	    PRUnichar c = *i1;
	    i1++;
	    if (c < '0' || c > '9') {
	      n = 0;
	      break;
	    }
	    n = n * 10 + (c - '0');
	  }
	  if (n > maxns)
            maxns = n;
	}
      }
    }
    current->GetParentNode(getter_AddRefs(temp));
    if (temp)
      current = temp;
    else
      break;
  }
// Create a unique prefix...
  PRUint32 len = 3;
  PRUint32 c = maxns + 1;
  while (c >= 10) {
    c = c / 10;
    len++;
  }
// Set the length and write it backwards since that's the easiest way..
  aPrefix.SetLength(len);
  nsWritingIterator<PRUnichar> i2;
  aPrefix.EndWriting(i2);
  c = maxns + 1;
  while (c > 0) {
    PRUint32 r = c % 10;
    c = c / 10;
    i2--;
    *i2 = (PRUnichar)(r + '0');
  }
  i2--;
  *i2 = 's';
  i2--;
  *i2 = 'n';
  return NS_OK;
}

nsresult 
nsSOAPUtils::MakeNamespacePrefixFixed(nsIDOMElement* aScope,
		                      nsAReadableString & aURI,
				      nsAWritableString & aPrefix)
{
  if (aURI.Equals(kSOAPEncodingURI))
    aPrefix = kSOAPEncodingPrefix;
  else if (aURI.Equals(kSOAPEnvURI))
    aPrefix = kSOAPEnvPrefix;
  else if (aURI.Equals(kXSIURI))
    aPrefix = kXSIPrefix;
  else if (aURI.Equals(kXSDURI))
    aPrefix = kXSDPrefix;
  else
    return MakeNamespacePrefix(aScope, aURI, aPrefix);

  return NS_OK;
}

PRBool nsSOAPUtils::StartsWith(nsAReadableString& aSuper,
		           nsAReadableString& aSub)
{
  PRUint32 c1 = aSuper.Length();
  PRUint32 c2 = aSub.Length();
  if (c1 < c2) return PR_FALSE;
  if (c1 == c2) return aSuper.Equals(aSub);
  nsReadingIterator<PRUnichar> i1;
  nsReadingIterator<PRUnichar> i2;
  aSuper.BeginReading(i1);
  aSub.BeginReading(i2);
  while (c2--) {
    if (*i1 != *i2) return PR_FALSE;
    i1++;
    i2++;
  }
  return PR_TRUE;
}
