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
#include "nsIDOMHTMLPreElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"
#include "nsIRuleNode.h"
#include "nsICSSDeclaration.h"

// XXX wrap, variable, cols, tabstop


class nsHTMLPreElement : public nsGenericHTMLContainerElement,
                         public nsIDOMHTMLPreElement
{
public:
  nsHTMLPreElement();
  virtual ~nsHTMLPreElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLPreElement
  NS_IMETHOD GetWidth(PRInt32* aWidth);
  NS_IMETHOD SetWidth(PRInt32 aWidth);

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLPreElement(nsIHTMLContent** aInstancePtrResult,
                     nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLPreElement* it = new nsHTMLPreElement();

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


nsHTMLPreElement::nsHTMLPreElement()
{
}

nsHTMLPreElement::~nsHTMLPreElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLPreElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLPreElement, nsGenericElement);


// QueryInterface implementation for nsHTMLPreElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLPreElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLPreElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLPreElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLPreElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLPreElement* it = new nsHTMLPreElement();

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


NS_IMPL_INT_ATTR(nsHTMLPreElement, Width, width)


NS_IMETHODIMP
nsHTMLPreElement::StringToAttribute(nsIAtom* aAttribute,
                                    const nsAReadableString& aValue,
                                    nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::cols) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::tabstop) {
    nsAutoString val(aValue);

    PRInt32 ec, tabstop = val.ToInteger(&ec);

    if (tabstop <= 0) {
      tabstop = 8;
    }

    aResult.SetIntValue(tabstop, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

static void
MapAttributesIntoRule(const nsIHTMLMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (!aData)
    return;

  if (aData->mFontData) {
    nsCSSFont& font = *(aData->mFontData);

    if (nsnull != aAttributes) {
      nsHTMLValue value;

      // variable: empty
      aAttributes->GetAttribute(nsHTMLAtoms::variable, value);
      if (value.GetUnit() == eHTMLUnit_Empty)
        font.mFamily.SetStringValue(NS_LITERAL_STRING("serif"), eCSSUnit_String);
    }
  }
  else if (aData->mPositionData) {
    // cols: int (nav4 attribute)
    nsHTMLValue value;
    if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::cols, value);
      if (value.GetUnit() == eHTMLUnit_Integer)
        aData->mPositionData->mWidth.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Char);

      // width: int (html4 attribute == nav4 cols)
      aAttributes->GetAttribute(nsHTMLAtoms::width, value);
      if (value.GetUnit() == eHTMLUnit_Integer)
        aData->mPositionData->mWidth.SetFloatValue((float)value.GetIntValue(), eCSSUnit_Char);
    }
  }
  else if (aData->mTextData && aData->mSID == eStyleStruct_Text) {
    if (aData->mTextData->mWhiteSpace.GetUnit() == eCSSUnit_Null) {
      nsHTMLValue value;
      // wrap: empty
      aAttributes->GetAttribute(nsHTMLAtoms::wrap, value);
      if (value.GetUnit() != eHTMLUnit_Null)
        aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_MOZ_PRE_WRAP, eCSSUnit_Enumerated);
      
      // cols: int (nav4 attribute)
      aAttributes->GetAttribute(nsHTMLAtoms::cols, value);
      if (value.GetUnit() == eHTMLUnit_Integer)
        // Force wrap property on since we want to wrap at a width
        // boundary not just a newline.
        aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_MOZ_PRE_WRAP, eCSSUnit_Enumerated);
      
      // width: int (html4 attribute == nav4 cols)
      aAttributes->GetAttribute(nsHTMLAtoms::width, value);
      if (value.GetUnit() == eHTMLUnit_Integer)
        // Force wrap property on since we want to wrap at a width
        // boundary not just a newline.
        aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_MOZ_PRE_WRAP, eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP
nsHTMLPreElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                           PRInt32& aHint) const
{
  if ((aAttribute == nsHTMLAtoms::variable) || 
      (aAttribute == nsHTMLAtoms::wrap) ||
      (aAttribute == nsHTMLAtoms::cols) ||
      (aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::tabstop)) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}



NS_IMETHODIMP
nsHTMLPreElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLPreElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
