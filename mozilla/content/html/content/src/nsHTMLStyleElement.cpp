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
#include "nsIDOMHTMLStyleElement.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsHTMLUtils.h"

// XXX no SRC attribute


class nsHTMLStyleElement : public nsGenericHTMLContainerElement,
                           public nsIDOMHTMLStyleElement,
                           public nsStyleLinkElement
{
public:
  nsHTMLStyleElement();
  virtual ~nsHTMLStyleElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLStyleElement
  NS_DECL_NSIDOMHTMLSTYLEELEMENT

  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
                         PRBool aCompileEventHandlers) {
    nsIDocument *oldDoc = mDocument;
    nsresult rv = nsGenericHTMLContainerElement::SetDocument(aDocument, aDeep,
                                                             aCompileEventHandlers);
    UpdateStyleSheet(PR_TRUE, oldDoc);
    return rv;
  }
 
  NS_IMETHOD SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAReadableString& aValue, PRBool aNotify) {
    nsresult rv = nsGenericHTMLContainerElement::SetAttr(aNameSpaceID, aName,
                                                         aValue, aNotify);
    UpdateStyleSheet(aNotify);
    return rv;
  }
  NS_IMETHOD SetAttr(nsINodeInfo* aNodeInfo,
                     const nsAReadableString& aValue, PRBool aNotify) {
    nsresult rv = nsGenericHTMLContainerElement::SetAttr(aNodeInfo, aValue,
                                                         aNotify);

    // nsGenericHTMLContainerElement::SetAttribute(nsINodeInfo* aNodeInfo,
    //                                             const nsAReadableString& aValue,
    //                                             PRBool aNotify)
    //
    // calls
    //
    // SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
    //              const nsAReadableString& aValue, PRBool aNotify)
    //
    // which ends up calling UpdateStyleSheet so we don't call UpdateStyleSheet
    // here ourselves.

    return rv;
  }
  NS_IMETHOD UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                            PRBool aNotify) {
    nsresult rv = nsGenericHTMLContainerElement::UnsetAttr(aNameSpaceID,
                                                           aAttribute,
                                                           aNotify);
    UpdateStyleSheet(aNotify);
    return rv;
  }

  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

protected:
  virtual void GetStyleSheetInfo(nsAWritableString& aUrl,
                                 nsAWritableString& aTitle,
                                 nsAWritableString& aType,
                                 nsAWritableString& aMedia,
                                 PRBool* aDoBlock);

  nsresult GetHrefCString(char* &aBuf);
};

nsresult
NS_NewHTMLStyleElement(nsIHTMLContent** aInstancePtrResult,
                       nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLStyleElement* it = new nsHTMLStyleElement();

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


nsHTMLStyleElement::nsHTMLStyleElement()
{
  nsHTMLUtils::AddRef(); // for GetHrefCString
}

nsHTMLStyleElement::~nsHTMLStyleElement()
{
  nsHTMLUtils::Release(); // for GetHrefCString
}


NS_IMPL_ADDREF_INHERITED(nsHTMLStyleElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLStyleElement, nsGenericElement) 


// QueryInterface implementation for nsHTMLStyleElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLStyleElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLStyleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLinkStyle)
  NS_INTERFACE_MAP_ENTRY(nsIStyleSheetLinkingElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLStyleElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLStyleElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLStyleElement* it = new nsHTMLStyleElement();

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
nsHTMLStyleElement::GetDisabled(PRBool* aDisabled)
{
  nsresult result = NS_OK;
  
  if (mStyleSheet) {
    nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(mStyleSheet));

    if (ss) {
      result = ss->GetDisabled(aDisabled);
    }
  }
  else {
    *aDisabled = PR_FALSE;
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLStyleElement::SetDisabled(PRBool aDisabled)
{
  nsresult result = NS_OK;
  
  if (mStyleSheet) {
    nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(mStyleSheet));

    if (ss) {
      result = ss->SetDisabled(aDisabled);
    }
  }

  return result;
}

NS_IMPL_STRING_ATTR(nsHTMLStyleElement, Media, media)
NS_IMPL_STRING_ATTR(nsHTMLStyleElement, Type, type)

NS_IMETHODIMP
nsHTMLStyleElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}

nsresult
nsHTMLStyleElement::GetHrefCString(char* &aBuf)
{
  // Get src= attribute (relative URL).
  nsAutoString relURLSpec;

  if (NS_CONTENT_ATTR_HAS_VALUE ==
      nsGenericHTMLContainerElement::GetAttr(kNameSpaceID_HTML,
                                             nsHTMLAtoms::src, relURLSpec)) {
    // Clean up any leading or trailing whitespace
    relURLSpec.Trim(" \t\n\r");

    // Get base URL.
    nsCOMPtr<nsIURI> baseURL;
    GetBaseURL(*getter_AddRefs(baseURL));

    if (baseURL) {
      // Get absolute URL.
      NS_MakeAbsoluteURIWithCharset(&aBuf, relURLSpec, mDocument, baseURL,
                                    nsHTMLUtils::IOService,
                                    nsHTMLUtils::CharsetMgr);
    }
    else {
      // Absolute URL is same as relative URL.
      aBuf = relURLSpec.ToNewUTF8String();
    }
  }
  else {
    // Absolute URL is empty because we have no HREF.
    aBuf = nsnull;
  }

  return NS_OK;
}

void
nsHTMLStyleElement::GetStyleSheetInfo(nsAWritableString& aUrl,
                                      nsAWritableString& aTitle,
                                      nsAWritableString& aType,
                                      nsAWritableString& aMedia,
                                      PRBool* aIsAlternate)
{
  nsresult rv = NS_OK;

  aUrl.Truncate();
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  nsAutoString href, rel, title, type;

  char *buf;
  GetHrefCString(buf);
  if (buf) {
    href.Assign(NS_ConvertASCIItoUCS2(buf));
    nsCRT::free(buf);
  }
  if (href.IsEmpty()) {
    // if href is empty then just bail
    return;
  }

  GetAttribute(NS_LITERAL_STRING("title"), title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  GetAttribute(NS_LITERAL_STRING("media"), aMedia);
  ToLowerCase(aMedia); // HTML4.0 spec is inconsistent, make it case INSENSITIVE

  GetAttribute(NS_LITERAL_STRING("type"), type);
  aType.Assign(type);

  nsAutoString mimeType;
  nsAutoString notUsed;
  nsStyleLinkElement::SplitMimeType(type, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.EqualsIgnoreCase("text/css")) {
    return;
  }

  // If we get here we assume that we're loading a css file, so set the
  // type to 'text/css'
  aType.Assign(NS_LITERAL_STRING("text/css"));
  
  nsCOMPtr<nsIURI> url, base;

  nsCOMPtr<nsIURI> baseURL;
  GetBaseURL(*getter_AddRefs(baseURL));
  rv = NS_MakeAbsoluteURI(aUrl, href, baseURL);

  if (!aTitle.IsEmpty()) {
    *aIsAlternate = PR_FALSE;
    nsAutoString prefStyle;
    mDocument->GetHeaderData(nsHTMLAtoms::headerDefaultStyle,
                             prefStyle);
    if (prefStyle.IsEmpty()) { // Set as preferred
      mDocument->SetHeaderData(nsHTMLAtoms::headerDefaultStyle,
                               title);
    }
  }

  return;
}

