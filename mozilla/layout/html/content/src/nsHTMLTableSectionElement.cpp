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
#include "nsIDOMHTMLTableSectionElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

static NS_DEFINE_IID(kIDOMHTMLTableSectionElementIID, NS_IDOMHTMLTABLESECTIONELEMENT_IID);

// you will see the phrases "rowgroup" and "section" used interchangably

class nsHTMLTableSectionElement : public nsIDOMHTMLTableSectionElement,
                           public nsIScriptObjectOwner,
                           public nsIDOMEventReceiver,
                           public nsIHTMLContent
{
public:
  nsHTMLTableSectionElement(nsIAtom* aTag);
  ~nsHTMLTableSectionElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLTableSectionElement
  NS_IMETHOD GetCh(nsString& aCh);
  NS_IMETHOD SetCh(const nsString& aCh);
  NS_IMETHOD GetChOff(nsString& aChOff);
  NS_IMETHOD SetChOff(const nsString& aChOff);
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetVAlign(nsString& aVAlign);
  NS_IMETHOD SetVAlign(const nsString& aVAlign);
  NS_IMETHOD GetRows(nsIDOMHTMLCollection** aRows);
  NS_IMETHOD InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aReturn);
  NS_IMETHOD DeleteRow(PRInt32 aIndex);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsGenericHTMLContainerElement mInner;
};

nsresult
NS_NewHTMLTableSectionElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLTableSectionElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLTableSectionElement::nsHTMLTableSectionElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLTableSectionElement::~nsHTMLTableSectionElement()
{
}

NS_IMPL_ADDREF(nsHTMLTableSectionElement)

NS_IMPL_RELEASE(nsHTMLTableSectionElement)

nsresult
nsHTMLTableSectionElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLTableSectionElementIID)) {
    nsIDOMHTMLTableSectionElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLTableSectionElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLTableSectionElement* it = new nsHTMLTableSectionElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, Align, align, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, VAlign, valign, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, Ch, ch, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, ChOff, choff, eSetAttrNotify_Reflow)

NS_IMETHODIMP
nsHTMLTableSectionElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  *aValue = 0;
  // XXX write me
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aValue)
{
  *aValue = 0;
  // XXX write me
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::DeleteRow(PRInt32 aIndex)
{
  // XXX write me
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::StringToAttribute(nsIAtom* aAttribute,
                                      const nsString& aValue,
                                      nsHTMLValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     ch
   */
  /* attributes that resolve to integers */
  if (aAttribute == nsHTMLAtoms::choff) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  /* attributes that resolve to integers or percents */
  else if (aAttribute == nsHTMLAtoms::height) {
    nsGenericHTMLElement::ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  /* other attributes */
  else if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::ParseTableHAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::background) {
    nsAutoString href(aValue);
    href.StripWhitespace();
    aResult.SetStringValue(href);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::bgcolor) {
    nsGenericHTMLElement::ParseColor(aValue, aResult);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (nsGenericHTMLElement::ParseTableVAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::AttributeToString(nsIAtom* aAttribute,
                                      nsHTMLValue& aValue,
                                      nsString& aResult) const
{
  /* ignore these attributes, stored already as strings
     ch
   */
  /* ignore attributes that are of standard types
     choff, height, width, background, bgcolor
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::TableHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (nsGenericHTMLElement::TableVAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(nsIHTMLAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull!=aContext, "bad style context arg");
  NS_PRECONDITION(nsnull!=aPresContext, "bad presentation context arg");

  if (nsnull!=aAttributes)
  {
    nsHTMLValue value;
    nsHTMLValue widthValue;
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
    if (value.GetUnit() == eHTMLUnit_Enumerated) 
    {
      if (nsnull==textStyle)
        textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mVerticalAlign.SetIntValue(value.GetIntValue(), eStyleUnit_Enumerated);
    }

    // height: pixel
    aAttributes->GetAttribute(nsHTMLAtoms::height, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      float p2t = aPresContext->GetPixelsToTwips();
      nsStylePosition* pos = (nsStylePosition*)
        aContext->GetMutableStyleData(eStyleStruct_Position);
      nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      pos->mHeight.SetCoordValue(twips);
    }
    nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aContext, aPresContext);
    nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
  }

}

NS_IMETHODIMP
nsHTMLTableSectionElement::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableSectionElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                   nsEvent* aEvent,
                                   nsIDOMEvent** aDOMEvent,
                                   PRUint32 aFlags,
                                   nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
