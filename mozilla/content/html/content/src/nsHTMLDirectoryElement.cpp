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
#include "nsIDOMHTMLDirectoryElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

// XXX nav4 has type= start= (same as OL/UL)

extern nsGenericHTMLElement::EnumTable kListTypeTable[];

static NS_DEFINE_IID(kIDOMHTMLDirectoryElementIID, NS_IDOMHTMLDIRECTORYELEMENT_IID);

class nsHTMLDirectoryElement : public nsIDOMHTMLDirectoryElement,
                               public nsIScriptObjectOwner,
                               public nsIDOMEventReceiver,
                               public nsIHTMLContent
{
public:
  nsHTMLDirectoryElement(nsIAtom* aTag);
  ~nsHTMLDirectoryElement();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLDirectoryElement
  NS_IMETHOD GetCompact(PRBool* aCompact);
  NS_IMETHOD SetCompact(PRBool aCompact);

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
NS_NewHTMLDirectoryElement(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLDirectoryElement(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLDirectoryElement::nsHTMLDirectoryElement(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLDirectoryElement::~nsHTMLDirectoryElement()
{
}

NS_IMPL_ADDREF(nsHTMLDirectoryElement)

NS_IMPL_RELEASE(nsHTMLDirectoryElement)

nsresult
nsHTMLDirectoryElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLDirectoryElementIID)) {
    nsIDOMHTMLDirectoryElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    mRefCnt++;
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLDirectoryElement::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLDirectoryElement* it = new nsHTMLDirectoryElement(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_BOOL_ATTR(nsHTMLDirectoryElement, Compact, compact, eSetAttrNotify_Reflow)

NS_IMETHODIMP
nsHTMLDirectoryElement::StringToAttribute(nsIAtom* aAttribute,
                                          const nsString& aValue,
                                          nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::type) {
    if (!nsGenericHTMLElement::ParseEnumValue(aValue, kListTypeTable,
                                              aResult)) {
      aResult.SetIntValue(NS_STYLE_LIST_STYLE_BASIC, eHTMLUnit_Enumerated);
    }
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  if (aAttribute == nsHTMLAtoms::start) {
    nsGenericHTMLElement::ParseValue(aValue, 1, aResult, eHTMLUnit_Integer);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  if (aAttribute == nsHTMLAtoms::compact) {
    aResult.SetEmptyValue();
    return NS_CONTENT_ATTR_NO_VALUE;
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLDirectoryElement::AttributeToString(nsIAtom* aAttribute,
                                          nsHTMLValue& aValue,
                                          nsString& aResult) const
{
  if (aAttribute == nsHTMLAtoms::type) {
    nsGenericHTMLElement::EnumValueToString(aValue, kListTypeTable, aResult);
    return NS_CONTENT_ATTR_HAS_VALUE;
  }
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLDirectoryElement::MapAttributesInto(nsIStyleContext* aContext,
                                          nsIPresContext* aPresContext)
{
  if (nsnull != mInner.mAttributes) {
    nsHTMLValue value;
    nsStyleList* list = (nsStyleList*)
      aContext->GetMutableStyleData(eStyleStruct_List);

    // type: enum
    GetAttribute(nsHTMLAtoms::type, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      list->mListStyleType = value.GetIntValue();
    }

    // compact: empty
    GetAttribute(nsHTMLAtoms::compact, value);
    if (value.GetUnit() == eHTMLUnit_Empty) {
      // XXX set
    }
  }
  return mInner.MapAttributesInto(aContext, aPresContext);
}

NS_IMETHODIMP
nsHTMLDirectoryElement::HandleDOMEvent(nsIPresContext& aPresContext,
                                       nsEvent* aEvent,
                                       nsIDOMEvent** aDOMEvent,
                                       PRUint32 aFlags,
                                       nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
