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
#include "nsIDOMHTMLTableColElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIRuleNode.h"

class nsHTMLTableColGroupElement : public nsGenericHTMLContainerElement,
                                   public nsIDOMHTMLTableColElement
{
public:
  nsHTMLTableColGroupElement();
  virtual ~nsHTMLTableColGroupElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLTableColElement
  NS_DECL_NSIDOMHTMLTABLECOLELEMENT

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunctions(nsMapRuleToAttributesFunc& aMapRuleFunc,
                                          nsMapAttributesFunc& aMapFunc) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                      PRInt32& aHint) const;

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLTableColGroupElement(nsIHTMLContent** aInstancePtrResult,
                               nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLTableColGroupElement* it = new nsHTMLTableColGroupElement();

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


nsHTMLTableColGroupElement::nsHTMLTableColGroupElement()
{
}

nsHTMLTableColGroupElement::~nsHTMLTableColGroupElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableColGroupElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableColGroupElement, nsGenericElement) 


// XPConnect interface list for nsHTMLTableColGroupElement
NS_CLASSINFO_MAP_BEGIN(HTMLTableColGroupElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableColElement)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsHTMLTableColGroupElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableColGroupElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableColElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableColGroupElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLTableColGroupElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLTableColGroupElement* it = new nsHTMLTableColGroupElement();

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


NS_IMPL_STRING_ATTR(nsHTMLTableColGroupElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLTableColGroupElement, Ch, ch)
NS_IMPL_STRING_ATTR(nsHTMLTableColGroupElement, ChOff, charoff)
NS_IMPL_INT_ATTR(nsHTMLTableColGroupElement, Span, span)
NS_IMPL_STRING_ATTR(nsHTMLTableColGroupElement, VAlign, valign)
NS_IMPL_STRING_ATTR(nsHTMLTableColGroupElement, Width, width)


NS_IMETHODIMP
nsHTMLTableColGroupElement::StringToAttribute(nsIAtom* aAttribute,
                                  const nsAReadableString& aValue,
                                  nsHTMLValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     ch
   */
  /* attributes that resolve to integers */
  if (aAttribute == nsHTMLAtoms::charoff) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::span) {
    if (ParseValue(aValue, 1, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    /* attributes that resolve to integers or percents or proportions */

    if (ParseValueOrPercentOrProportional(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    /* other attributes */

    if (ParseTableCellHAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (ParseTableVAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTableColGroupElement::AttributeToString(nsIAtom* aAttribute,
                                  const nsHTMLValue& aValue,
                                  nsAWritableString& aResult) const
{
  /* ignore these attributes, stored already as strings
     ch
   */
  /* ignore attributes that are of standard types
     charoff, repeat
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (TableCellHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (TableVAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    if (ValueOrPercentOrProportionalToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLContainerElement::AttributeToString(aAttribute, aValue,
                                                          aResult);
}

static 
void MapAttributesIntoRule(const nsIHTMLMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (!aAttributes || !aData)
    return;

  nsHTMLValue value;
  
  if (aData->mPositionData && aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
    // width
    aAttributes->GetAttribute(nsHTMLAtoms::width, value);
    if (value.GetUnit() != eHTMLUnit_Null) {
      switch (value.GetUnit()) {
      case eHTMLUnit_Percent: {
        nsCSSValue val; val.SetPercentValue(value.GetPercentValue());
        aData->mPositionData->mWidth = val;
        break;
      }
      case eHTMLUnit_Pixel: {
        nsCSSValue val((float)value.GetPixelValue(), eCSSUnit_Pixel);
        aData->mPositionData->mWidth = val;
        break;
      }
      case eHTMLUnit_Proportional: {
        nsCSSValue val((float)value.GetIntValue(), eCSSUnit_Proportional);
        aData->mPositionData->mWidth = val;
        break;
      }
      default:
        break;
      }
    }
  }
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIMutableStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull!=aContext, "bad style context arg");
  NS_PRECONDITION(nsnull!=aPresContext, "bad presentation context arg");

  if (nsnull != aAttributes) {
    nsHTMLValue value;
    nsStyleText* textStyle = nsnull;

    // align: enum
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) 
    {
      textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mTextAlign = value.GetIntValue();
    }
    
    // valign: enum
    aAttributes->GetAttribute(nsHTMLAtoms::valign, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      if (nsnull==textStyle)
        textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mVerticalAlign.SetIntValue(value.GetIntValue(),
                                            eStyleUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext,
                                                aPresContext);
}

NS_IMETHODIMP
nsHTMLTableColGroupElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                                     PRInt32& aHint) const
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::align) ||
      (aAttribute == nsHTMLAtoms::valign)) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableColGroupElement::GetAttributeMappingFunctions(nsMapRuleToAttributesFunc& aMapRuleFunc,
                                                         nsMapAttributesFunc& aMapFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableColGroupElement::SizeOf(nsISizeOfHandler* aSizer,
                                   PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
