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
#include "nsIDOMHTMLModElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"


class nsHTMLInsElement : public nsGenericHTMLContainerElement,
                         public nsIDOMHTMLModElement
{
public:
  nsHTMLInsElement();
  virtual ~nsHTMLInsElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLModElement
  NS_DECL_NSIDOMHTMLMODELEMENT

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
};

nsresult
NS_NewHTMLInsElement(nsIHTMLContent** aInstancePtrResult,
                     nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLInsElement* it = new nsHTMLInsElement();

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


nsHTMLInsElement::nsHTMLInsElement()
{
}

nsHTMLInsElement::~nsHTMLInsElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLInsElement, nsGenericElement);
NS_IMPL_RELEASE_INHERITED(nsHTMLInsElement, nsGenericElement);


// XPConnect interface list for nsHTMLInsElement
NS_CLASSINFO_MAP_BEGIN(HTMLInsElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLModElement)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsHTMLInsElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLInsElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLModElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLInsElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLInsElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLInsElement* it = new nsHTMLInsElement();

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


NS_IMPL_STRING_ATTR(nsHTMLInsElement, Cite, cite)
NS_IMPL_STRING_ATTR(nsHTMLInsElement, DateTime, datetime)


NS_IMETHODIMP
nsHTMLInsElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
