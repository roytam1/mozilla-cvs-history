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
 *   Daniel Glazman <glazman@netscape.com>
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
#include "nsIDOMHTMLStyleElement.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsHTMLUtils.h"
#include "nsUnicharUtils.h"
#include "nsParserUtils.h"


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

  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify, PRBool aDeepSetDocument)
  {
    nsresult rv = nsGenericHTMLContainerElement::InsertChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }

  NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify, PRBool aDeepSetDocument)
  {
    nsresult rv = nsGenericHTMLContainerElement::ReplaceChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }

  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify, PRBool aDeepSetDocument)
  {
    nsresult rv = nsGenericHTMLContainerElement::AppendChildTo(aKid, aNotify, aDeepSetDocument);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }

  NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
  {
    nsresult rv = nsGenericHTMLContainerElement::RemoveChildAt(aIndex, aNotify);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }

  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
                         PRBool aCompileEventHandlers) {
    nsCOMPtr<nsIDocument> oldDoc = mDocument;
    nsresult rv = nsGenericHTMLContainerElement::SetDocument(aDocument, aDeep,
                                                             aCompileEventHandlers);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet(oldDoc);
    }
    return rv;
  }
 
  NS_IMETHOD SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAString& aValue, PRBool aNotify) {
    nsresult rv = nsGenericHTMLContainerElement::SetAttr(aNameSpaceID, aName,
                                                         aValue, aNotify);
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }
  NS_IMETHOD SetAttr(nsINodeInfo* aNodeInfo,
                     const nsAString& aValue, PRBool aNotify) {
    nsresult rv = nsGenericHTMLContainerElement::SetAttr(aNodeInfo, aValue,
                                                         aNotify);

    // nsGenericHTMLContainerElement::SetAttribute(nsINodeInfo* aNodeInfo,
    //                                             const nsAString& aValue,
    //                                             PRBool aNotify)
    //
    // calls
    //
    // SetAttribute(PRInt32 aNameSpaceID, nsIAtom* aName,
    //              const nsAString& aValue, PRBool aNotify)
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
    if (NS_SUCCEEDED(rv)) {
      UpdateStyleSheet();
    }
    return rv;
  }

  nsresult GetInnerHTML(nsAString& aInnerHTML);
  nsresult SetInnerHTML(const nsAString& aInnerHTML);
  
#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

protected:
  nsresult GetHrefCString(char* &aBuf);
  void GetStyleSheetURL(PRBool* aIsInline,
                        nsAString& aUrl);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
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

nsresult
nsHTMLStyleElement::GetInnerHTML(nsAString& aInnerHTML)
{
  return GetContentsAsText(aInnerHTML);
}

nsresult
nsHTMLStyleElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  SetEnableUpdates(PR_FALSE);

  nsresult rv = ReplaceContentsWithText(aInnerHTML, PR_TRUE);
  
  SetEnableUpdates(PR_TRUE);
  
  UpdateStyleSheet();
  return rv;
}
  
#ifdef DEBUG
NS_IMETHODIMP
nsHTMLStyleElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
#endif

nsresult
nsHTMLStyleElement::GetHrefCString(char* &aBuf)
{
  // Get src= attribute (relative URL).
  nsAutoString relURLSpec;

  if (NS_CONTENT_ATTR_HAS_VALUE ==
      nsGenericHTMLContainerElement::GetAttr(kNameSpaceID_None,
                                             nsHTMLAtoms::src, relURLSpec)) {
    // Clean up any leading or trailing whitespace
    relURLSpec.Trim(" \t\n\r");

    // Get base URL.
    nsCOMPtr<nsIURI> baseURL;
    GetBaseURL(*getter_AddRefs(baseURL));

    if (baseURL) {
      // Get absolute URL.
      nsCAutoString buf;
      NS_MakeAbsoluteURIWithCharset(buf, relURLSpec, mDocument, baseURL,
                                    nsHTMLUtils::IOService,
                                    nsHTMLUtils::CharsetMgr);
      aBuf = ToNewCString(buf);
    }
    else {
      // Absolute URL is same as relative URL.
      aBuf = ToNewUTF8String(relURLSpec);
    }
  }
  else {
    // Absolute URL is empty because we have no HREF.
    aBuf = nsnull;
  }

  return NS_OK;
}

void
nsHTMLStyleElement::GetStyleSheetURL(PRBool* aIsInline,
                                     nsAString& aUrl)
{
  aUrl.Truncate();
  *aIsInline = !HasAttr(kNameSpaceID_None, nsHTMLAtoms::src);
  if (*aIsInline) {
    return;
  }
  if (mNodeInfo->NamespaceEquals(kNameSpaceID_XHTML)) {
    // We stopped supporting <style src="..."> for XHTML as it is
    // non-standard.
    *aIsInline = PR_TRUE;
    return;
  }

  char *buf;
  GetHrefCString(buf);
  if (buf) {
    aUrl.Assign(NS_ConvertASCIItoUCS2(buf));
    nsCRT::free(buf);
  }
  return;
}

void
nsHTMLStyleElement::GetStyleSheetInfo(nsAString& aTitle,
                                      nsAString& aType,
                                      nsAString& aMedia,
                                      PRBool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsHTMLAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  GetAttr(kNameSpaceID_None, nsHTMLAtoms::media, aMedia);
  ToLowerCase(aMedia); // HTML4.0 spec is inconsistent, make it case INSENSITIVE

  GetAttr(kNameSpaceID_None, nsHTMLAtoms::type, aType);

  nsAutoString mimeType;
  nsAutoString notUsed;
  nsParserUtils::SplitMimeType(aType, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.EqualsIgnoreCase("text/css")) {
    return;
  }

  // If we get here we assume that we're loading a css file, so set the
  // type to 'text/css'
  aType.Assign(NS_LITERAL_STRING("text/css"));

  return;
}
