/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsIDOMHTMLImageElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIHTMLAttributes.h"

// XXX nav attrs: suppress

static NS_DEFINE_IID(kIDOMHTMLImageElementIID, NS_IDOMHTMLIMAGEELEMENT_IID);

class nsHTMLImageElement : public nsIDOMHTMLImageElement,
                            public nsIScriptObjectOwner,
                            public nsIDOMEventReceiver,
                            public nsIHTMLContent
{
public:
  nsHTMLImageElement(nsIAtom* aTag);
  ~nsHTMLImageElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLImageElement
  NS_IMETHOD GetLowSrc(nsString& aLowSrc);
  NS_IMETHOD SetLowSrc(const nsString& aLowSrc);
  NS_IMETHOD GetName(nsString& aName);
  NS_IMETHOD SetName(const nsString& aName);
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetAlt(nsString& aAlt);
  NS_IMETHOD SetAlt(const nsString& aAlt);
  NS_IMETHOD GetBorder(nsString& aBorder);
  NS_IMETHOD SetBorder(const nsString& aBorder);
  NS_IMETHOD GetHeight(nsString& aHeight);
  NS_IMETHOD SetHeight(const nsString& aHeight);
  NS_IMETHOD GetHspace(nsString& aHspace);
  NS_IMETHOD SetHspace(const nsString& aHspace);
  NS_IMETHOD GetIsMap(PRBool* aIsMap);
  NS_IMETHOD SetIsMap(PRBool aIsMap);
  NS_IMETHOD GetLongDesc(nsString& aLongDesc);
  NS_IMETHOD SetLongDesc(const nsString& aLongDesc);
  NS_IMETHOD GetSrc(nsString& aSrc);
  NS_IMETHOD SetSrc(const nsString& aSrc);
  NS_IMETHOD GetUseMap(nsString& aUseMap);
  NS_IMETHOD SetUseMap(const nsString& aUseMap);
  NS_IMETHOD GetVspace(nsString& aVspace);
  NS_IMETHOD SetVspace(const nsString& aVspace);
  NS_IMETHOD GetWidth(nsString& aWidth);
  NS_IMETHOD SetWidth(const nsString& aWidth);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsGenericHTMLLeafElement mInner;
};

nsresult
NS_NewHTMLImageElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLImageElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLImageElement::nsHTMLImageElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLImageElement::~nsHTMLImageElement()
{
}

NS_IMPL_ADDREF(nsHTMLImageElement)

NS_IMPL_RELEASE(nsHTMLImageElement)

nsresult
nsHTMLImageElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLImageElementIID)) {
    nsIDOMHTMLImageElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLImageElement::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLImageElement* it = new nsHTMLImageElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLImageElement, LowSrc, lowsrc, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Name, name, eSetAttrNotify_Restart)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Align, align, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Alt, alt, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Border, border, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Height, height, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Hspace, hspace, eSetAttrNotify_Reflow)
NS_IMPL_BOOL_ATTR(nsHTMLImageElement, IsMap, ismap, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, LongDesc, longdesc, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Src, src, eSetAttrNotify_Render)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, UseMap, usemap, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Vspace, vspace, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLImageElement, Width, width, eSetAttrNotify_Reflow)

NS_IMETHODIMP
nsHTMLImageElement::StringToAttribute(nsIAtom* aAttribute,
                                      const nsString& aValue,
                                      nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::ParseAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::ismap) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  if ((aAttribute == nsHTMLAtoms::usemap) ||
      (aAttribute == nsHTMLAtoms::src) ||
      (aAttribute == nsHTMLAtoms::lowsrc)) {
    nsAutoString tmp(aValue);
    tmp.StripWhitespace();
    aResult.SetStringValue(tmp);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (nsGenericHTMLElement::ParseImageAttribute(aAttribute,
                                                     aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLImageElement::AttributeToString(nsIAtom* aAttribute,
                                      nsHTMLValue& aValue,
                                      nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      nsGenericHTMLElement::AlignValueToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (nsGenericHTMLElement::ImageAttributeToString(aAttribute,
                                                        aValue, aResult)) {
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(nsIHTMLAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  if (nsnull != aAttributes) {
    nsHTMLValue value;
    aAttributes->GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      PRUint8 align = value.GetIntValue();
      nsStyleDisplay* display = (nsStyleDisplay*)
        aContext->GetMutableStyleData(eStyleStruct_Display);
      nsStyleText* text = (nsStyleText*)
        aContext->GetMutableStyleData(eStyleStruct_Text);
      nsStyleSpacing* spacing = (nsStyleSpacing*)
        aContext->GetMutableStyleData(eStyleStruct_Spacing);
      float p2t = aPresContext->GetPixelsToTwips();
      nsStyleCoord three(NSIntPixelsToTwips(3, p2t));
      switch (align) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        display->mFloats = NS_STYLE_FLOAT_LEFT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        display->mFloats = NS_STYLE_FLOAT_RIGHT;
        spacing->mMargin.SetLeft(three);
        spacing->mMargin.SetRight(three);
        break;
      default:
        text->mVerticalAlign.SetIntValue(align, eStyleUnit_Enumerated);
        break;
      }
    }
  }
  nsGenericHTMLElement::MapImageAttributesInto(aAttributes, aContext, aPresContext);
  nsGenericHTMLElement::MapImageBorderAttributesInto(aAttributes, aContext, aPresContext, nsnull);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLImageElement::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLImageElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                   nsEvent* aEvent,
                                   nsIDOMEvent** aDOMEvent,
                                   PRUint32 aFlags,
                                   nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
