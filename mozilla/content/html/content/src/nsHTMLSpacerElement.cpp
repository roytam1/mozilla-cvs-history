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
 * The Original Code is Mozilla Communicator client code.
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
#include "nsIDOMHTMLElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIHTMLAttributes.h"
#include "nsSize.h"
#include "nsIDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMDocument.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsIWebShell.h"
#include "nsIFrame.h"
#include "nsLayoutAtoms.h"
#include "nsIRuleNode.h"

// XXX nav attrs: suppress

class nsHTMLSpacerElement : public nsGenericHTMLLeafElement,
                            public nsIDOMHTMLElement
{
public:
  nsHTMLSpacerElement();
  virtual ~nsHTMLSpacerElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLLeafElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLLeafElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLLeafElement::)

  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD AttributeToString(nsIAtom* aAttribute,
                               const nsHTMLValue& aValue,
                               nsAWritableString& aResult) const;
  NS_IMETHOD GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const;
  NS_IMETHOD GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                      PRInt32& aHint) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLSpacerElement(nsIHTMLContent** aInstancePtrResult,
                        nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLSpacerElement* it = new nsHTMLSpacerElement();

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


nsHTMLSpacerElement::nsHTMLSpacerElement()
{
}

nsHTMLSpacerElement::~nsHTMLSpacerElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLSpacerElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLSpacerElement, nsGenericElement);


// QueryInterface implementation for nsHTMLSpacerElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLSpacerElement,
                                    nsGenericHTMLLeafElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLSpacerElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLSpacerElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLSpacerElement* it = new nsHTMLSpacerElement();

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
nsHTMLSpacerElement::StringToAttribute(nsIAtom* aAttribute,
                                       const nsAReadableString& aValue,
                                       nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::size) {
    if (ParseValue(aValue, 0, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    if (ParseAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if ((aAttribute == nsHTMLAtoms::width) ||
           (aAttribute == nsHTMLAtoms::height)) {
    if (ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLSpacerElement::AttributeToString(nsIAtom* aAttribute,
                                       const nsHTMLValue& aValue,
                                       nsAWritableString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      AlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return nsGenericHTMLLeafElement::AttributeToString(aAttribute, aValue,
                                                     aResult);
}

static void
MapAttributesIntoRule(const nsIHTMLMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (!aAttributes || !aData)
    return;

  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImagePositionAttributeInto(aAttributes, aData);

  if (aData->mPositionData) {
    nsHTMLValue value;
    const nsStyleDisplay* display = (const nsStyleDisplay*)
      aData->mStyleContext->GetStyleData(eStyleStruct_Display);
    
    PRBool typeIsBlock = (display->mDisplay == NS_STYLE_DISPLAY_BLOCK);
    if (typeIsBlock) {
      // width: value
      if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::width, value);
        if (value.GetUnit() == eHTMLUnit_Pixel)
          aData->mPositionData->mWidth.SetFloatValue((float)value.GetPixelValue(), eCSSUnit_Pixel);
        else if (value.GetUnit() == eHTMLUnit_Percent)
          aData->mPositionData->mWidth.SetPercentValue(value.GetPercentValue());
      }

      // height: value
      if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::height, value);
        if (value.GetUnit() == eHTMLUnit_Pixel)
          aData->mPositionData->mHeight.SetFloatValue((float)value.GetPixelValue(), eCSSUnit_Pixel);   
        else if (value.GetUnit() == eHTMLUnit_Percent)
          aData->mPositionData->mHeight.SetPercentValue(value.GetPercentValue());
      }
    }
    else
    {
      // size: value
      if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
        aAttributes->GetAttribute(nsHTMLAtoms::size, value);
        if (value.GetUnit() == eHTMLUnit_Pixel) 
          aData->mPositionData->mWidth.SetFloatValue((float)value.GetPixelValue(), eCSSUnit_Pixel);  
      }
    }
  }
  else if (aData->mDisplayData) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRUint8 align = (PRUint8)(value.GetIntValue());
      if (aData->mDisplayData && aData->mDisplayData->mFloat.GetUnit() == eCSSUnit_Null) {
        if (align == NS_STYLE_TEXT_ALIGN_LEFT)
          aData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_LEFT, eCSSUnit_Enumerated);
        else if (align == NS_STYLE_TEXT_ALIGN_RIGHT)
          aData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_RIGHT, eCSSUnit_Enumerated);
      }
    }

    if (aData->mDisplayData->mDisplay == eCSSUnit_Null) {
      aAttributes->GetAttribute(nsHTMLAtoms::type, value);
      if (eHTMLUnit_String == value.GetUnit()) {
        nsAutoString tmp;
        value.GetStringValue(tmp);
        if (tmp.EqualsIgnoreCase("line") ||
            tmp.EqualsIgnoreCase("vert") ||
            tmp.EqualsIgnoreCase("vertical") ||
            tmp.EqualsIgnoreCase("block")) {
          // This is not strictly 100% compatible: if the spacer is given
          // a width of zero then it is basically ignored.
          aData->mDisplayData->mDisplay = NS_STYLE_DISPLAY_BLOCK;
        }
      }
    }
  }
  
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}


NS_IMETHODIMP
nsHTMLSpacerElement::GetMappedAttributeImpact(const nsIAtom* aAttribute, PRInt32 aModType,
                                             PRInt32& aHint) const
{
  if ((aAttribute == nsHTMLAtoms::usemap) ||
      (aAttribute == nsHTMLAtoms::ismap)) {
    aHint = NS_STYLE_HINT_FRAMECHANGE;
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (!GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    if (!GetImageMappedAttributesImpact(aAttribute, aHint)) {
      if (!GetImageBorderAttributeImpact(aAttribute, aHint)) {
        aHint = NS_STYLE_HINT_CONTENT;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSpacerElement::GetAttributeMappingFunction(nsMapRuleToAttributesFunc& aMapRuleFunc) const
{
  aMapRuleFunc = &MapAttributesIntoRule;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLSpacerElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
