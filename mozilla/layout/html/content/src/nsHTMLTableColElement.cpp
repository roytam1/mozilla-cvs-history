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
#include "nsIDOMHTMLTableColElement.h"
#include "nsIHTMLTableColElement.h"
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

static NS_DEFINE_IID(kIDOMHTMLTableColElementIID, NS_IDOMHTMLTABLECOLELEMENT_IID);
static NS_DEFINE_IID(kIHTMLTableColElementIID, NS_IHTMLTABLECOLELEMENT_IID);

class nsHTMLTableColElement :  public nsIDOMHTMLTableColElement,
                               public nsIHTMLTableColElement,
                               public nsIScriptObjectOwner,
                               public nsIDOMEventReceiver,
                               public nsIHTMLContent
{
public:
  nsHTMLTableColElement(nsIAtom* aTag);
  virtual ~nsHTMLTableColElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLTableColElement
  NS_IMETHOD GetAlign(nsString& aAlign);
  NS_IMETHOD SetAlign(const nsString& aAlign);
  NS_IMETHOD GetCh(nsString& aCh);
  NS_IMETHOD SetCh(const nsString& aCh);
  NS_IMETHOD GetChOff(nsString& aChOff);
  NS_IMETHOD SetChOff(const nsString& aChOff);
  NS_IMETHOD GetSpan(PRInt32* aSpan);
  NS_IMETHOD SetSpan(PRInt32 aSpan);
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

  // nsIHTMLTableColElement
  NS_IMETHOD GetSpanValue(PRInt32* aSpan);

protected:
  nsGenericHTMLContainerElement mInner;
};

nsresult
NS_NewHTMLTableColElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLTableColElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}


nsHTMLTableColElement::nsHTMLTableColElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLTableColElement::~nsHTMLTableColElement()
{
}

NS_IMPL_ADDREF(nsHTMLTableColElement)

NS_IMPL_RELEASE(nsHTMLTableColElement)

nsresult
nsHTMLTableColElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLTableColElementIID)) {
    nsIDOMHTMLTableColElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIHTMLTableColElementIID)) {
    nsIHTMLTableColElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLTableColElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  nsHTMLTableColElement* it = new nsHTMLTableColElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner, aDeep);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Ch, ch)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, ChOff, choff)
NS_IMPL_INT_ATTR(nsHTMLTableColElement, Span, span)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, VAlign, valign)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Width, width)

NS_IMETHODIMP
nsHTMLTableColElement::StringToAttribute(nsIAtom* aAttribute,
                                  const nsString& aValue,
                                  nsHTMLValue& aResult)
{
  /* ignore these attributes, stored simply as strings
     ch
   */
  /* attributes that resolve to integers */
  if (aAttribute == nsHTMLAtoms::choff) {
    if (nsGenericHTMLElement::ParseValue(aValue, 0, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::span) {
    if (nsGenericHTMLElement::ParseValue(aValue, 1, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  /* attributes that resolve to integers or percents or proportions */
  else if (aAttribute == nsHTMLAtoms::width) {
    if (nsGenericHTMLElement::ParseValueOrPercentOrProportional(aValue, aResult, eHTMLUnit_Pixel)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }

  /* other attributes */
  else if (aAttribute == nsHTMLAtoms::align) {
    if (mInner.ParseTableCellHAlignValue(aValue, aResult)) {
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
nsHTMLTableColElement::AttributeToString(nsIAtom* aAttribute,
                                  const nsHTMLValue& aValue,
                                  nsString& aResult) const
{
  /* ignore these attributes, stored already as strings
     ch
   */
  /* ignore attributes that are of standard types
     choff, span
   */
  if (aAttribute == nsHTMLAtoms::align) {
    if (mInner.TableCellHAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    if (nsGenericHTMLElement::TableVAlignValueToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::width) {
    if (nsGenericHTMLElement::ValueOrPercentOrProportionalToString(aValue, aResult)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

static void
MapAttributesInto(const nsIHTMLMappedAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull!=aContext, "bad style context arg");
  NS_PRECONDITION(nsnull!=aPresContext, "bad presentation context arg");
  if (nsnull != aAttributes) {

    nsHTMLValue value;
    nsStyleText* textStyle = nsnull;

    // width
    aAttributes->GetAttribute(nsHTMLAtoms::width, value);
    if (value.GetUnit() != eHTMLUnit_Null) {
      nsStylePosition* position = (nsStylePosition*)
        aContext->GetMutableStyleData(eStyleStruct_Position);
      switch (value.GetUnit()) {
      case eHTMLUnit_Percent:
        position->mWidth.SetPercentValue(value.GetPercentValue());
        break;

      case eHTMLUnit_Pixel:
        float p2t;
        aPresContext->GetScaledPixelsToTwips(&p2t);
        position->mWidth.SetCoordValue(NSIntPixelsToTwips(value.GetPixelValue(), p2t));
        break;

      case eHTMLUnit_Proportional:
        position->mWidth.SetIntValue(value.GetIntValue(), eStyleUnit_Proportional);
        break;
      default:
        break;
      }
    }

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

    // span: int
    aAttributes->GetAttribute(nsHTMLAtoms::span, value);
    if (value.GetUnit() == eHTMLUnit_Integer)
    {
      nsStyleTable *tableStyle = (nsStyleTable*)aContext->GetMutableStyleData(eStyleStruct_Table);
      tableStyle->mSpan = value.GetIntValue();
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLTableColElement::GetMappedAttributeImpact(const nsIAtom* aAttribute,
                                                PRInt32& aHint) const
{
  if ((aAttribute == nsHTMLAtoms::width) ||
      (aAttribute == nsHTMLAtoms::align) ||
      (aAttribute == nsHTMLAtoms::valign) ||
      (aAttribute == nsHTMLAtoms::span)) {
    aHint = NS_STYLE_HINT_REFLOW;
  }
  else if (! nsGenericHTMLElement::GetCommonMappedAttributesImpact(aAttribute, aHint)) {
    aHint = NS_STYLE_HINT_CONTENT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableColElement::GetAttributeMappingFunctions(nsMapAttributesFunc& aFontMapFunc,
                                                    nsMapAttributesFunc& aMapFunc) const
{
  aFontMapFunc = nsnull;
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableColElement::HandleDOMEvent(nsIPresContext& aPresContext,
                               nsEvent* aEvent,
                               nsIDOMEvent** aDOMEvent,
                               PRUint32 aFlags,
                               nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}

NS_METHOD nsHTMLTableColElement::GetSpanValue(PRInt32* aSpan)
{
  if (nsnull!=aSpan)
  {
    PRInt32 span=-1;
    GetSpan(&span);
    if (-1==span)
      span=1; // the default;
    *aSpan = span;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableColElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  return mInner.SizeOf(aSizer, aResult, sizeof(*this));
}
