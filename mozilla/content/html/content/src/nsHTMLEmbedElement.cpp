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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIDOMHTMLEmbedElement.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"


class nsHTMLEmbedElement : public nsGenericHTMLLeafElement,
                           public nsIDOMHTMLEmbedElement
{
public:
  nsHTMLEmbedElement();
  virtual ~nsHTMLEmbedElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLLeafElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLLeafElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLLeafElement::)

  // nsIDOMHTMLEmbedElement
  NS_DECL_NSIDOMHTMLEMBEDELEMENT

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;
  NS_IMETHOD GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                          nsMapAttributesFunc& aMapFunc) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLEmbedElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLEmbedElement* it = new nsHTMLEmbedElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    delete it;

    return rv;
  }

  *aInstancePtrResult = NS_STATIC_CAST(nsIHTMLContent *, it);
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsHTMLEmbedElement::nsHTMLEmbedElement()
{
}

nsHTMLEmbedElement::~nsHTMLEmbedElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLEmbedElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLEmbedElement, nsGenericElement);


// XPConnect interface list for nsHTMLEmbedElement
NS_CLASINFO_MAP_BEGIN(HTMLEmbedElement)
  NS_CLASINFO_MAP_ENTRY(nsIDOMHTMLEmbedElement)
  NS_CLASINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASINFO_MAP_END


// QueryInterface implementation for nsHTMLEmbedElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLEmbedElement,
                                    nsGenericHTMLLeafElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLEmbedElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLEmbedElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLEmbedElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLEmbedElement* it = new nsHTMLEmbedElement();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> kungFuDeathGrip(it);

  nsresult rv = it->Init(mNodeInfo);

  if (NS_FAILED(rv))
    return rv;

  CopyInnerTo(this, it, aDeep);

  *aReturn = NS_STATIC_CAST(nsIDOMNode *, it);

  NS_ADDREF(*aReturn);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEmbedElement::StringToAttribute(nsIAtom* aAttribute,
                                       const nsAReadableString& aValue,
                                       nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (ParseAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (ParseImageAttribute(aAttribute, aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLEmbedElement::AttributeToString(nsIAtom* aAttribute,
                                       const nsHTMLValue& aValue,
                                       nsAWritableString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      AlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (ImageAttributeToString(aAttribute, aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  return nsGenericHTMLLeafElement::AttributeToString(aAttribute, aValue,
                                                     aResult);
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIMutableStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  nsGenericHTMLElement::MapImageAlignAttributeInto(aAttributes, aContext,
                                                   aPresContext);
  nsGenericHTMLElement::MapImageAttributesInto(aAttributes, aContext,
                                               aPresContext);
  nsGenericHTMLElement::MapImageBorderAttributeInto(aAttributes, aContext,
                                                    aPresContext, nsnull);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext,
                                                aPresContext);
}

NS_IMETHODIMP
nsHTMLEmbedElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                             PRInt32& aHint) const
{
  if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    if (!GetImageMappedAttributesImpact(aAttribute, aHint)) {
      if (!GetImageAlignAttributeImpact(aAttribute, aHint)) {
        if (!GetImageBorderAttributeImpact(aAttribute, aHint)) {
          aHint = NS_STYLE_HINT_CONTENT;
        }
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEmbedElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                 nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEmbedElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}


/***************************************************************************/

#if 0
NS_IMETHODIMP
nsHTMLEmbedElement::GetScriptObject(nsIScriptContext* aContext,
                                    void** aScriptObject)
{
  return GetPluginScriptObject(aContext, aScriptObject);
}

PRBool
nsHTMLEmbedElement::GetProperty(JSContext *aContext, JSObject *aObj,
                                jsval aID, jsval *aVp)
{
  return GetPluginProperty(aContext, aObj, aID, aVp);
}
#endif

/////////////////////////////////////////////
// Implement nsIDOMHTMLEmbedElement interface
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Height, height)
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Width, width)
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLEmbedElement, Src, src)

