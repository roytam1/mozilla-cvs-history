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

#include "nsSOAPUtils.h"
#include "nsIDOMText.h"
#include "nsISOAPMessage.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsCOMPtr.h"

NS_NAMED_LITERAL_STRING(realSOAPEnvURI1,
			"http://schemas.xmlsoap.org/soap/envelope/");
NS_NAMED_LITERAL_STRING(realSOAPEnvURI2,
			"http://www.w3.org/2001/09/soap-envelope");
const nsAString *nsSOAPUtils::kSOAPEnvURI[] =
    { &realSOAPEnvURI1, &realSOAPEnvURI2
};

NS_NAMED_LITERAL_STRING(realSOAPEncURI1,
			"http://schemas.xmlsoap.org/soap/encoding/");
NS_NAMED_LITERAL_STRING(realSOAPEncURI2,
			"http://www.w3.org/2001/09/soap-encoding");
const nsAString *nsSOAPUtils::kSOAPEncURI[] =
    { &realSOAPEncURI1, &realSOAPEncURI2
};

NS_NAMED_LITERAL_STRING(realXSIURI1,
			"http://www.w3.org/1999/XMLSchema-instance");
NS_NAMED_LITERAL_STRING(realXSIURI2,
			"http://www.w3.org/2001/XMLSchema-instance");
const nsAString *nsSOAPUtils::kXSIURI[] = { &realXSIURI1, &realXSIURI2
};

NS_NAMED_LITERAL_STRING(realXSURI1, "http://www.w3.org/1999/XMLSchema");
NS_NAMED_LITERAL_STRING(realXSURI2, "http://www.w3.org/2001/XMLSchema");
const nsAString *nsSOAPUtils::kXSURI[] = { &realXSURI1, &realXSURI2
};

NS_NAMED_LITERAL_STRING(realSOAPEnvPrefix, "env");
const nsAString & nsSOAPUtils::kSOAPEnvPrefix = realSOAPEnvPrefix;

NS_NAMED_LITERAL_STRING(realSOAPEncPrefix, "enc");
const nsAString & nsSOAPUtils::kSOAPEncPrefix = realSOAPEncPrefix;

NS_NAMED_LITERAL_STRING(realXSIPrefix, "xsi");
const nsAString & nsSOAPUtils::kXSIPrefix = realXSIPrefix;

NS_NAMED_LITERAL_STRING(realXSITypeAttribute, "type");
const nsAString & nsSOAPUtils::kXSITypeAttribute = realXSITypeAttribute;

NS_NAMED_LITERAL_STRING(realXSPrefix, "xs");
const nsAString & nsSOAPUtils::kXSPrefix = realXSPrefix;

NS_NAMED_LITERAL_STRING(realEncodingStyleAttribute, "encodingStyle");
const
    nsAString &
    nsSOAPUtils::kEncodingStyleAttribute = realEncodingStyleAttribute;

NS_NAMED_LITERAL_STRING(realActorAttribute, "actor");
const nsAString & nsSOAPUtils::kActorAttribute = realActorAttribute;

NS_NAMED_LITERAL_STRING(realMustUnderstandAttribute, "mustUnderstand");
const
    nsAString &
    nsSOAPUtils::kMustUnderstandAttribute = realMustUnderstandAttribute;

NS_NAMED_LITERAL_STRING(realEnvelopeTagName, "Envelope");
const nsAString & nsSOAPUtils::kEnvelopeTagName = realEnvelopeTagName;

NS_NAMED_LITERAL_STRING(realHeaderTagName, "Header");
const nsAString & nsSOAPUtils::kHeaderTagName = realHeaderTagName;

NS_NAMED_LITERAL_STRING(realBodyTagName, "Body");
const nsAString & nsSOAPUtils::kBodyTagName = realBodyTagName;

NS_NAMED_LITERAL_STRING(realFaultTagName, "Fault");
const nsAString & nsSOAPUtils::kFaultTagName = realFaultTagName;

NS_NAMED_LITERAL_STRING(realFaultCodeTagName, "faultcode");
const nsAString & nsSOAPUtils::kFaultCodeTagName = realFaultCodeTagName;

NS_NAMED_LITERAL_STRING(realFaultStringTagName, "faultstring");
const
    nsAString & nsSOAPUtils::kFaultStringTagName = realFaultStringTagName;

NS_NAMED_LITERAL_STRING(realFaultActorTagName, "faultactor");
const nsAString & nsSOAPUtils::kFaultActorTagName = realFaultActorTagName;

NS_NAMED_LITERAL_STRING(realFaultDetailTagName, "detail");
const
    nsAString & nsSOAPUtils::kFaultDetailTagName = realFaultDetailTagName;

NS_NAMED_LITERAL_STRING(realEncodingSeparator, "#");
const nsAString & nsSOAPUtils::kEncodingSeparator = realEncodingSeparator;

NS_NAMED_LITERAL_STRING(realQualifiedSeparator, ":");
const
    nsAString & nsSOAPUtils::kQualifiedSeparator = realQualifiedSeparator;

NS_NAMED_LITERAL_STRING(realXMLNamespaceNamespaceURI,
			"http://www.w3.org/2000/xmlns/");
const nsAString &
    nsSOAPUtils::kXMLNamespaceNamespaceURI = realXMLNamespaceNamespaceURI;

NS_NAMED_LITERAL_STRING(realXMLNamespaceURI,
			"http://www.w3.org/XML/1998/namespace");
const
 nsAString & nsSOAPUtils::kXMLNamespaceURI = realXMLNamespaceURI;

NS_NAMED_LITERAL_STRING(realXMLPrefix, "xml:");
const nsAString & nsSOAPUtils::kXMLPrefix = realXMLPrefix;

NS_NAMED_LITERAL_STRING(realXMLNamespacePrefix, "xmlns:");
const
    nsAString & nsSOAPUtils::kXMLNamespacePrefix = realXMLNamespacePrefix;

NS_NAMED_LITERAL_STRING(realTrue, "true");
const nsAString & nsSOAPUtils::kTrue = realTrue;

NS_NAMED_LITERAL_STRING(realFalse, "false");
const nsAString & nsSOAPUtils::kFalse = realFalse;

NS_NAMED_LITERAL_STRING(realTrueA, "1");
const nsAString & nsSOAPUtils::kTrueA = realTrueA;

NS_NAMED_LITERAL_STRING(realFalseA, "0");
const nsAString & nsSOAPUtils::kFalseA = realFalseA;

void nsSOAPUtils::GetSpecificChildElement(nsIDOMElement * aParent,
					  const nsAString & aNamespace,
					  const nsAString & aType,
					  nsIDOMElement * *aElement)
{
  nsCOMPtr < nsIDOMElement > sibling;

  *aElement = nsnull;
  GetFirstChildElement(aParent, getter_AddRefs(sibling));
  if (sibling) {
    GetSpecificSiblingElement(sibling, aNamespace, aType, aElement);
  }
}

void nsSOAPUtils::GetSpecificSiblingElement(nsIDOMElement * aSibling,
					    const nsAString & aNamespace,
					    const nsAString & aType,
					    nsIDOMElement * *aElement)
{
  nsCOMPtr < nsIDOMElement > sibling;

  *aElement = nsnull;
  sibling = aSibling;
  do {
    nsAutoString name, namespaceURI;
    sibling->GetLocalName(name);
    sibling->GetNamespaceURI(namespaceURI);
    if (name.Equals(aType) && namespaceURI.Equals(aNamespace)) {
      *aElement = sibling;
      NS_ADDREF(*aElement);
      return;
    }
    nsCOMPtr < nsIDOMElement > temp = sibling;
    GetNextSiblingElement(temp, getter_AddRefs(sibling));
  }
  while (sibling);
}

void nsSOAPUtils::GetFirstChildElement(nsIDOMElement * aParent,
				       nsIDOMElement ** aElement)
{
  nsCOMPtr < nsIDOMNode > child;

  *aElement = nsnull;
  aParent->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      child->QueryInterface(NS_GET_IID(nsIDOMElement), (void **) aElement);
      break;
    }
    nsCOMPtr < nsIDOMNode > temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
}

void nsSOAPUtils::GetNextSiblingElement(nsIDOMElement * aStart,
					nsIDOMElement ** aElement)
{
  nsCOMPtr < nsIDOMNode > sibling;

  *aElement = nsnull;
  GetNextSibling(aStart, getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      sibling->QueryInterface(NS_GET_IID(nsIDOMElement),
			      (void **) aElement);
      break;
    }
    nsCOMPtr < nsIDOMNode > temp = sibling;
    GetNextSibling(temp, getter_AddRefs(sibling));
  }
}

nsresult
    nsSOAPUtils::GetElementTextContent(nsIDOMElement * aElement,
				       nsAString & aText)
{
  nsCOMPtr < nsIDOMNode > child;
  nsAutoString rtext;
  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::TEXT_NODE == type
	|| nsIDOMNode::CDATA_SECTION_NODE == type) {
      nsCOMPtr < nsIDOMText > text = do_QueryInterface(child);
      nsAutoString data;
      text->GetData(data);
      rtext.Append(data);
    } else if (nsIDOMNode::ELEMENT_NODE == type) {
      return NS_ERROR_ILLEGAL_VALUE;	//  This was interpreted as a simple value, yet had complex content in it.
    }
    nsCOMPtr < nsIDOMNode > temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
  aText.Assign(rtext);
  return NS_OK;
}

PRBool nsSOAPUtils::HasChildElements(nsIDOMElement * aElement)
{
  nsCOMPtr < nsIDOMNode > child;

  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      return PR_TRUE;
    }
    nsCOMPtr < nsIDOMNode > temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }

  return PR_FALSE;
}

void nsSOAPUtils::GetNextSibling(nsIDOMNode * aSibling,
				 nsIDOMNode ** aNext)
{
  nsCOMPtr < nsIDOMNode > last;
  nsCOMPtr < nsIDOMNode > current;
  PRUint16 type;

  *aNext = nsnull;
  last = aSibling;

  last->GetNodeType(&type);
  if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
    last->GetFirstChild(getter_AddRefs(current));
    if (!last) {
      last->GetNextSibling(getter_AddRefs(current));
    }
  } else {
    last->GetNextSibling(getter_AddRefs(current));
  }
  while (!current) {
    last->GetParentNode(getter_AddRefs(current));
    current->GetNodeType(&type);
    if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
      last = current;
      last->GetNextSibling(getter_AddRefs(current));
    } else {
      current = nsnull;
      break;
    }
  }
  *aNext = current;
  NS_IF_ADDREF(*aNext);
}

nsresult
    nsSOAPUtils::GetNamespaceURI(nsIDOMElement * aScope,
				 const nsAString & aQName,
				 nsAString & aURI)
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
  nsCOMPtr < nsIDOMNode > current = aScope;
  nsCOMPtr < nsIDOMNamedNodeMap > attrs;
  nsCOMPtr < nsIDOMNode > temp;
  nsAutoString value;
  while (current != nsnull) {
    rc = current->GetAttributes(getter_AddRefs(attrs));
    if (NS_FAILED(rc))
      return rc;
    if (attrs) {
      rc = attrs->GetNamedItemNS(kXMLNamespaceNamespaceURI, prefix,
				 getter_AddRefs(temp));
      if (NS_FAILED(rc))
	return rc;
      if (temp != nsnull)
	return temp->GetNodeValue(aURI);
    }
    rc = current->GetParentNode(getter_AddRefs(temp));
    if (NS_FAILED(rc))
      return rc;
    current = temp;
  }
  return NS_ERROR_FAILURE;
}

nsresult
    nsSOAPUtils::GetLocalName(const nsAString & aQName,
			      nsAString & aLocalName)
{
  PRInt32 i = aQName.FindChar(':');
  if (i < 0)
    aLocalName = aQName;
  else
    aQName.Mid(aLocalName, i + 1, aQName.Length() - i);
  return NS_OK;
}

nsresult
    nsSOAPUtils::MakeNamespacePrefix(nsIDOMElement * aScope,
				     const nsAString & aURI,
				     nsAString & aPrefix)
{
//  This may change for level 3 serialization, so be sure to gut this
//  and call the standardized level 3 method when it is available.
  aPrefix.Truncate(0);
  if (aURI.IsEmpty())
    return NS_OK;
  if (aURI.Equals(nsSOAPUtils::kXMLNamespaceURI)) {
    aPrefix.Assign(nsSOAPUtils::kXMLPrefix);
    return NS_OK;
  }
  nsCOMPtr < nsIDOMNode > current = aScope;
  nsCOMPtr < nsIDOMNamedNodeMap > attrs;
  nsCOMPtr < nsIDOMNode > temp;
  nsAutoString tstr;
  nsresult rc;
  PRUint32 maxns = 0;		//  Keep track of max generated NS
  for (;;) {
    rc = current->GetAttributes(getter_AddRefs(attrs));
    if (NS_FAILED(rc))
      return rc;
    if (attrs) {
      PRUint32 i = 0;
      for (;;) {
	attrs->Item(i++, getter_AddRefs(temp));
	if (!temp)
	  break;
	temp->GetNamespaceURI(tstr);
	if (!tstr.Equals(nsSOAPUtils::kXMLNamespaceNamespaceURI))
	  continue;
	temp->GetNodeValue(tstr);
	if (tstr.Equals(aURI)) {
	  nsAutoString prefix;
	  rc = temp->GetLocalName(prefix);
	  if (NS_FAILED(rc))
	    return rc;
	  nsCOMPtr < nsIDOMNode > check = aScope;
	  PRBool hasDecl;
	  nsCOMPtr < nsIDOMElement > echeck;
	  while (check != current) {	// Make sure prefix is not overridden
	    echeck = do_QueryInterface(check);
	    if (echeck) {
	      rc = echeck->
		  HasAttributeNS(nsSOAPUtils::
				 kXMLNamespaceNamespaceURI, prefix,
				 &hasDecl);
	      if (NS_FAILED(rc))
		return rc;
	      if (hasDecl)
		break;
	      echeck->GetParentNode(getter_AddRefs(check));
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
	else {			//  Decode the generated namespace into a number
	  nsReadingIterator < PRUnichar > i1;
	  nsReadingIterator < PRUnichar > i2;
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
  nsWritingIterator < PRUnichar > i2;
  aPrefix.EndWriting(i2);
  c = maxns + 1;
  while (c > 0) {
    PRUint32 r = c % 10;
    c = c / 10;
    i2--;
    *i2 = (PRUnichar) (r + '0');
  }
  i2--;
  *i2 = 's';
  i2--;
  *i2 = 'n';
  return NS_OK;
}

PRBool nsSOAPUtils::StartsWith(nsAString & aSuper, nsAString & aSub)
{
  PRUint32 c1 = aSuper.Length();
  PRUint32 c2 = aSub.Length();
  if (c1 < c2)
    return PR_FALSE;
  if (c1 == c2)
    return aSuper.Equals(aSub);
  nsReadingIterator < PRUnichar > i1;
  nsReadingIterator < PRUnichar > i2;
  aSuper.BeginReading(i1);
  aSub.BeginReading(i2);
  while (c2--) {
    if (*i1 != *i2)
      return PR_FALSE;
    i1++;
    i2++;
  }
  return PR_TRUE;
}

