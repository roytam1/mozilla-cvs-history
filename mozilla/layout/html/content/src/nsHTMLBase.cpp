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
#include "nsIDOMHTMLBaseElement.h"
#include "nsIScriptObjectOwner.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsHTMLGenericContent.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"

static NS_DEFINE_IID(kIDOMHTMLBaseElementIID, NS_IDOMHTMLBASEELEMENT_IID);

class nsHTMLBase : public nsIDOMHTMLBaseElement,
                   public nsIScriptObjectOwner,
                   public nsIDOMEventReceiver,
                   public nsIHTMLContent
{
public:
  nsHTMLBase(nsIAtom* aTag);
  ~nsHTMLBase();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMNode
  NS_IMPL_IDOMNODE_USING_GENERIC(mInner)

  // nsIDOMElement
  NS_IMPL_IDOMELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLElement
  NS_IMPL_IDOMHTMLELEMENT_USING_GENERIC(mInner)

  // nsIDOMHTMLBaseElement
  NS_IMETHOD GetHref(nsString& aHref);
  NS_IMETHOD SetHref(const nsString& aHref);
  NS_IMETHOD GetTarget(nsString& aTarget);
  NS_IMETHOD SetTarget(const nsString& aTarget);

  // nsIScriptObjectOwner
  NS_IMPL_ISCRIPTOBJECTOWNER_USING_GENERIC(mInner)

  // nsIDOMEventReceiver
  NS_IMPL_IDOMEVENTRECEIVER_USING_GENERIC(mInner)

  // nsIContent
  NS_IMPL_ICONTENT_USING_GENERIC(mInner)

  // nsIHTMLContent
  NS_IMPL_IHTMLCONTENT_USING_GENERIC(mInner)

protected:
  nsHTMLGenericLeafContent mInner;
};

nsresult
NS_NewHTMLBase(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new nsHTMLBase(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsHTMLBase::nsHTMLBase(nsIAtom* aTag)
{
  NS_INIT_REFCNT();
  mInner.Init(this, aTag);
}

nsHTMLBase::~nsHTMLBase()
{
}

NS_IMPL_ADDREF(nsHTMLBase)

NS_IMPL_RELEASE(nsHTMLBase)

nsresult
nsHTMLBase::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_IMPL_HTML_CONTENT_QUERY_INTERFACE(aIID, aInstancePtr, this)
  if (aIID.Equals(kIDOMHTMLBaseElementIID)) {
    nsIDOMHTMLBaseElement* tmp = this;
    *aInstancePtr = (void*) tmp;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsHTMLBase::CloneNode(nsIDOMNode** aReturn)
{
  nsHTMLBase* it = new nsHTMLBase(mInner.mTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mInner.CopyInnerTo(this, &it->mInner);
  return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}

NS_IMPL_STRING_ATTR(nsHTMLBase, Href, href, eSetAttrNotify_None)
NS_IMPL_STRING_ATTR(nsHTMLBase, Target, target, eSetAttrNotify_None)

NS_IMETHODIMP
nsHTMLBase::StringToAttribute(nsIAtom* aAttribute,
                              const nsString& aValue,
                              nsHTMLValue& aResult)
{
  return NS_CONTENT_ATTR_NOT_THERE;
}

NS_IMETHODIMP
nsHTMLBase::AttributeToString(nsIAtom* aAttribute,
                              nsHTMLValue& aValue,
                              nsString& aResult) const
{
  return mInner.AttributeToString(aAttribute, aValue, aResult);
}

NS_IMETHODIMP
nsHTMLBase::MapAttributesInto(nsIStyleContext* aContext,
                              nsIPresContext* aPresContext)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLBase::HandleDOMEvent(nsIPresContext& aPresContext,
                           nsEvent* aEvent,
                           nsIDOMEvent** aDOMEvent,
                           PRUint32 aFlags,
                           nsEventStatus& aEventStatus)
{
  return mInner.HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                               aFlags, aEventStatus);
}
