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
#include "nsIHTMLTableCellElement.h"
#include "nsIDOMHTMLTableCellElement.h"
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

static NS_DEFINE_IID(kIDOMHTMLTableCellElementIID, NS_IDOMHTMLTABLECELLELEMENT_IID);
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);

class nsHTMLTableCellElement :  public nsIHTMLTableCellElement,
                                public nsIDOMHTMLTableCellElement,
                                public nsIScriptObjectOwner,
                                public nsIDOMEventReceiver,
                                public nsIHTMLContent
{
public:
  nsHTMLTableCellElement(nsIAtom* aTag);
  ~nsHTMLTableCellElement();

// nsISupports
  NS_DECL_ISUPPORTS

// nsIHTMLTableCellElement

  /** @return the starting column for this cell.  Always >= 1 */
  NS_METHOD GetColIndex (PRInt32* aColIndex);

  /** set the starting column for this cell.  Always >= 1 */
  NS_METHOD SetColIndex (PRInt32 aColIndex);


// nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

// nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

// nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

// nsIDOMHTMLTableCellElement
  NS_IMETHOD GetCellIndex(PRInt32* aCellIndex);
  NS_IMETHOD SetCellIndex(PRInt32 aCellIndex);
  NS_IMETHOD GetAbbr(nsString& aAbbr);
  NS_IMETHOD SetAbbr(const nsString& aAbbr);
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetAxis(nsString& aAxis);
  NS_IMETHOD SetAxis(const nsString& aAxis);
  NS_IMETHOD GetBgColor(nsString& aBgColor);
  NS_IMETHOD SetBgColor(const nsString& aBgColor);
  NS_IMETHOD GetCh(nsString& aCh);
  NS_IMETHOD SetCh(const nsString& aCh);
  NS_IMETHOD GetChOff(nsString& aChOff);
  NS_IMETHOD SetChOff(const nsString& aChOff);
  NS_IMETHOD GetColSpan(PRInt32* aColSpan);
  NS_IMETHOD SetColSpan(PRInt32 aColSpan);
  NS_IMETHOD GetHeaders(nsString& aHeaders);
  NS_IMETHOD SetHeaders(const nsString& aHeaders);
  NS_IMETHOD GetHeight(nsString& aHeight);
  NS_IMETHOD SetHeight(const nsString& aHeight);
  NS_IMETHOD GetNoWrap(PRBool* aNoWrap);
  NS_IMETHOD SetNoWrap(PRBool aNoWrap);
  NS_IMETHOD GetRowSpan(PRInt32* aRowSpan);
  NS_IMETHOD SetRowSpan(PRInt32 aRowSpan);
  NS_IMETHOD GetScope(nsString& aScope);
  NS_IMETHOD SetScope(const nsString& aScope);
  NS_IMETHOD GetVAlign(nsString& aVAlign);
  NS_IMETHOD SetVAlign(const nsString& aVAlign);
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
  nsGenericHTMLContainerElement mInner;
  PRInt32 mColIndex;
};

nsresult
NS_NewHTMLTableCellElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLTableCellElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLTableCellElement::nsHTMLTableCellElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
  mColIndex=0;
}

nsHTMLTableCellElement::~nsHTMLTableCellElement()
{
}

NS_IMPL_ADDREF(nsHTMLTableCellElement)

NS_IMPL_RELEASE(nsHTMLTableCellElement)

nsresult
nsHTMLTableCellElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLTableCellElementIID)) {
    nsIDOMHTMLTableCellElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  else if (aIID.Equals(kIHTMLTableCellElementIID)) {
    nsIHTMLTableCellElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLTableCellElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLTableCellElement* it = new nsHTMLTableCellElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

/** @return the starting column for this cell in aColIndex.  Always >= 1 */
NS_METHOD nsHTMLTableCellElement::GetColIndex (PRInt32* aColIndex)
{ 
  *aColIndex = mColIndex;
  return NS_OK;
}

/** set the starting column for this cell.  Always >= 1 */
NS_METHOD nsHTMLTableCellElement::SetColIndex (PRInt32 aColIndex)
{ 
  mColIndex = aColIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableCellElement::GetCellIndex(PRInt32* aCellIndex)
{
  *aCellIndex = 0;/* XXX */
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableCellElement::SetCellIndex(PRInt32 aCellIndex)
{
  // XXX write me
  return NS_OK;
}

NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Abbr, abbr, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Align, align, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Axis, axis, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, BgColor, bgcolor, eSetAttrNotify_Render)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Ch, ch, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, ChOff, choff, eSetAttrNotify_Reflow)
NS_IMPL_INT_ATTR(nsHTMLTableCellElement, ColSpan, colspan, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Headers, headers, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Height, height, eSetAttrNotify_Reflow)
NS_IMPL_BOOL_ATTR(nsHTMLTableCellElement, NoWrap, nowrap, eSetAttrNotify_Reflow)
NS_IMPL_INT_ATTR(nsHTMLTableCellElement, RowSpan, rowspan, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Scope, scope, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, VAlign, valign, eSetAttrNotify_Reflow)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Width, width, eSetAttrNotify_Reflow)


static nsGenericHTMLElement::EnumTable kCellScopeTable[] = {
  { "row",      NS_STYLE_CELL_SCOPE_ROW },
  { "col",      NS_STYLE_CELL_SCOPE_COL },
  { "rowgroup", NS_STYLE_CELL_SCOPE_ROWGROUP },
  { "colgroup", NS_STYLE_CELL_SCOPE_COLGROUP },
  { 0 }
};

NS_IMETHODIMP
nsHTMLTableCellElement::StringToAttribute(nsIAtom* aAttribute,
                                   const nsString& aValue,
                                   nsHTMLValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     abbr, axis, ch, headers
   */
  /* attributes that resolve to integers with a min of 0 */
  if (aAttribute == nsHTMLAtoms::choff) {
    nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  /* attributes that resolve to integers with a min of 1 */
  if ((aAttribute == nsHTMLAtoms::colspan) ||
      (aAttribute == nsHTMLAtoms::rowspan)) {
    nsGenericHTMLElement::ParseValue(aValue, 1, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  /* attributes that resolve to integers or percents */
  else if (aAttribute == nsHTMLAtoms::height) {
    nsGenericHTMLElement::ParseValueOrPercent(aValue, aResult, eHTMLUnit_Pixel);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }

  /* attributes that resolve to integers or percents or proportions */
  else if (aAttribute == nsHTMLAtoms::width) {
    nsGenericHTMLElement::ParseValueOrPercentOrProportional(aValue, aResult, eHTMLUnit_Pixel);
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
  else if (aAttribute == nsHTMLAtoms::nowrap) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  else if (aAttribute == nsHTMLAtoms::scope) {
    if (nsGenericHTMLElement::ParseEnumValue(aValue, kCellScopeTable, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (nsGenericHTMLElement::ParseTableVAlignValue(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLTableCellElement::AttributeToString(nsIAtom* aAttribute,
                                   nsHTMLValue& aValue,
                                   nsString& aResult) const
{
  /* ignore these attributes, stored already as strings
     abbr, axis, ch, headers
   */
  /* ignore attributes that are of standard types
     choff, colspan, rowspan, height, width, nowrap, background, bgcolor
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (nsGenericHTMLElement::TableHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::scope) {
    if (nsGenericHTMLElement::EnumValueToString(aValue, kCellScopeTable, aResult)) {
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

    // width: pixel
    float p2t = aPresContext->GetPixelsToTwips();
    nsStylePosition* pos = (nsStylePosition*)
      aContext->GetMutableStyleData(eStyleStruct_Position);
    aAttributes->GetAttribute(nsHTMLAtoms::width, widthValue);
    if (widthValue.GetUnit() == eHTMLUnit_Pixel) {
      nscoord width = widthValue.GetPixelValue();
      nscoord twips = NSIntPixelsToTwips(width, p2t);
      pos->mWidth.SetCoordValue(twips);
    }
    else if (widthValue.GetUnit() == eHTMLUnit_Percent) {
      float widthPercent = widthValue.GetPercentValue();
      pos->mWidth.SetPercentValue(widthPercent);
    }

    // height: pixel
    aAttributes->GetAttribute(nsHTMLAtoms::height, value);
    if (value.GetUnit() == eHTMLUnit_Pixel) {
      nscoord height = value.GetPixelValue();
      nscoord twips = NSIntPixelsToTwips(height, p2t);
      pos->mHeight.SetCoordValue(twips);
    }

    // nowrap
    // nowrap depends on the width attribute, so be sure to handle it after width is mapped!
    aAttributes->GetAttribute(nsHTMLAtoms::nowrap, value);
    if (value.GetUnit() == eHTMLUnit_Empty)
    {
      if (widthValue.GetUnit() != eHTMLUnit_Pixel)
      {
        if (nsnull==textStyle)
          textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
        textStyle->mWhiteSpace = NS_STYLE_WHITESPACE_NOWRAP;
      }
    }

    nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aContext, aPresContext);
    nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
  }
}

NS_IMETHODIMP
nsHTMLTableCellElement::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableCellElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                nsEvent* aEvent,
                                nsIDOMEvent** aDOMEvent,
                                PRUint32 aFlags,
                                nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
