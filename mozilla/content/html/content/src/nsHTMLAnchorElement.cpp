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
#include "nsCOMPtr.h"
#include "nsHTMLUtils.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNSHTMLAnchorElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLDocument.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIEventStateManager.h"
#include "nsIURL.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsNetUtil.h"

#include "nsCOMPtr.h"
#include "nsIFrameManager.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIHTMLAttributes.h"

// XXX suppress

// XXX either suppress is handled in the event code below OR we need a
// custom frame

class nsHTMLAnchorElement : public nsGenericHTMLContainerElement,
                            public nsIDOMHTMLAnchorElement,
                            public nsIDOMNSHTMLAnchorElement,
                            public nsILink
{
public:
  nsHTMLAnchorElement();
  virtual ~nsHTMLAnchorElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLAnchorElement
  NS_DECL_NSIDOMHTMLANCHORELEMENT  

  // nsIDOMNSHTMLAnchorElement
  NS_DECL_NSIDOMNSHTMLANCHORELEMENT

  // nsILink
  NS_IMETHOD GetLinkState(nsLinkState &aState);
  NS_IMETHOD SetLinkState(nsLinkState aState);
  NS_IMETHOD GetHrefCString(char* &aBuf);

  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
                         PRBool aCompileEventHandlers);
  NS_IMETHOD SetFocus(nsIPresContext* aPresContext);
  NS_IMETHOD RemoveFocus(nsIPresContext* aPresContext);
  NS_IMETHOD StringToAttribute(nsIAtom* aAttribute,
                               const nsAReadableString& aValue,
                               nsHTMLValue& aResult);
  NS_IMETHOD HandleDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent, PRUint32 aFlags,
                            nsEventStatus* aEventStatus);
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;

protected:
  nsresult RegUnRegAccessKey(PRBool aDoReg);

  // The cached visited state
  nsLinkState mLinkState;

};


nsresult
NS_NewHTMLAnchorElement(nsIHTMLContent** aInstancePtrResult,
                        nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLAnchorElement* it = new nsHTMLAnchorElement();

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


nsHTMLAnchorElement::nsHTMLAnchorElement() : mLinkState(eLinkState_Unknown)
{
  nsHTMLUtils::AddRef(); // for GetHrefCString
}

nsHTMLAnchorElement::~nsHTMLAnchorElement()
{
  nsHTMLUtils::Release(); // for GetHrefCString
}


NS_IMPL_ADDREF_INHERITED(nsHTMLAnchorElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLAnchorElement, nsGenericElement) 


// XPConnect interface list for nsHTMLAnchorElement
NS_CLASSINFO_MAP_BEGIN(HTMLAnchorElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMHTMLAnchorElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLAnchorElement)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetGenericHTMLElementIIDs)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for nsHTMLAnchorElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLAnchorElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLAnchorElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLAnchorElement)
  NS_INTERFACE_MAP_ENTRY(nsILink)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLAnchorElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLAnchorElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLAnchorElement* it = new nsHTMLAnchorElement();

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


NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Charset, charset)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Coords, coords)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Rel, rel)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Rev, rev)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Shape, shape)
NS_IMPL_INT_ATTR(nsHTMLAnchorElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLAnchorElement, Type, type)


// This goes and gets the proper PresContext in order
// for it to get the EventStateManager so it can register 
// the access key
nsresult nsHTMLAnchorElement::RegUnRegAccessKey(PRBool aDoReg)
{
  // first check to see if it even has an acess key
  nsAutoString accessKey;
  PRInt32 nameSpaceID;
  GetNameSpaceID(nameSpaceID);
  nsresult rv;

  rv = NS_STATIC_CAST(nsIContent *, this)->GetAttribute(nameSpaceID,
                                                        nsHTMLAtoms::accesskey,
                                                        accessKey);

  if (NS_CONTENT_ATTR_NOT_THERE != rv) {
    nsCOMPtr<nsIPresContext> presContext;
    GetPresContext(this, getter_AddRefs(presContext));

    // With a valid PresContext we can get the EVM 
    // and register the access key
    if (presContext) {
      nsCOMPtr<nsIEventStateManager> stateManager;
      presContext->GetEventStateManager(getter_AddRefs(stateManager));

      if (stateManager) {
        if (aDoReg) {
          return stateManager->RegisterAccessKey(nsnull, this,
                                                 (PRUint32)accessKey.First());
        } else {
          return stateManager->UnregisterAccessKey(nsnull, this,
                                                   (PRUint32)accessKey.First());
        }
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                 PRBool aCompileEventHandlers)
{
  // The document gets set to null before it is destroyed,
  // so we unregister the the access key here (if it has one)
  // before setting it to null
  if (aDocument == nsnull) {
    RegUnRegAccessKey(PR_FALSE);
  }

  nsresult rv;
  rv = nsGenericHTMLContainerElement::SetDocument(aDocument,
                                                  aDeep,
                                                  aCompileEventHandlers);

  // Register the access key here (if it has one) 
  // if the document isn't null
  if (aDocument != nsnull) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLAnchorElement::Blur()
{
  return SetElementFocus(PR_FALSE);
}

NS_IMETHODIMP
nsHTMLAnchorElement::Focus()
{
  return SetElementFocus(PR_TRUE);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetFocus(nsIPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  // don't make the link grab the focus if there is no link handler
  nsCOMPtr<nsILinkHandler> handler;
  nsresult rv = aPresContext->GetLinkHandler(getter_AddRefs(handler));
  if (NS_SUCCEEDED(rv) && (nsnull != handler)) {
    nsCOMPtr<nsIEventStateManager> stateManager;

    aPresContext->GetEventStateManager(getter_AddRefs(stateManager));

    if (stateManager) {
      stateManager->SetContentState(this, NS_EVENT_STATE_FOCUS);

      // Make sure the presentation is up-to-date
      if (mDocument) {
        mDocument->FlushPendingNotifications();
      }

      nsCOMPtr<nsIPresShell> presShell;
      aPresContext->GetShell(getter_AddRefs(presShell));

      if (presShell) {
        nsIFrame* frame = nsnull;
        presShell->GetPrimaryFrameFor(this, &frame);
        if (frame) {
          presShell->ScrollFrameIntoView(frame, NS_PRESSHELL_SCROLL_ANYWHERE,
                                         NS_PRESSHELL_SCROLL_ANYWHERE);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::RemoveFocus(nsIPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  // If we are disabled, we probably shouldn't have focus in the
  // first place, so allow it to be removed.
  nsresult rv = NS_OK;

  nsCOMPtr<nsIEventStateManager> esm;
  aPresContext->GetEventStateManager(getter_AddRefs(esm));

  if (esm) {
    nsCOMPtr<nsIDocument> doc;
    GetDocument(*getter_AddRefs(doc));
    if (!doc)
      return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIContent> rootContent;
    doc->GetRootContent(getter_AddRefs(rootContent));
    rv = esm->SetContentState(rootContent, NS_EVENT_STATE_FOCUS);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLAnchorElement::StringToAttribute(nsIAtom* aAttribute,
                                       const nsAReadableString& aValue,
                                       nsHTMLValue& aResult)
{
  if (aAttribute == nsHTMLAtoms::tabindex) {
    if (ParseValue(aValue, 0, 32767, aResult, eHTMLUnit_Integer)) {
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::suppress) {
    if (nsCRT::strcasecmp(PromiseFlatString(aValue).get(),
                          NS_LITERAL_STRING("true").get())) {
      aResult.SetEmptyValue();  // XXX? shouldn't just leave "true"
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

// XXX support suppress in here
NS_IMETHODIMP
nsHTMLAnchorElement::HandleDOMEvent(nsIPresContext* aPresContext,
                                    nsEvent* aEvent,
                                    nsIDOMEvent** aDOMEvent,
                                    PRUint32 aFlags,
                                    nsEventStatus* aEventStatus)
{
  return HandleDOMEventForAnchors(this, aPresContext, aEvent, aDOMEvent, 
                                  aFlags, aEventStatus);
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetHref(nsAWritableString& aValue)
{
  char *buf;
  nsresult rv = GetHrefCString(buf);
  if (NS_FAILED(rv)) return rv;
  if (buf) {
    aValue.Assign(NS_ConvertUTF8toUCS2(buf));
    nsCRT::free(buf);
  }
  // NS_IMPL_STRING_ATTR does nothing where we have (buf == null)
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetHref(const nsAReadableString& aValue)
{
  // Clobber our "cache", so we'll recompute it the next time
  // somebody asks for it.
  mLinkState = eLinkState_Unknown;

  return NS_STATIC_CAST(nsIContent *, this)->SetAttribute(kNameSpaceID_HTML,
                                                          nsHTMLAtoms::href,
                                                          aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetTarget(nsAWritableString& aValue)
{
  aValue.Truncate();

  nsresult rv;
  rv = NS_STATIC_CAST(nsIContent *, this)->GetAttribute(kNameSpaceID_HTML,
                                                        nsHTMLAtoms::target,
                                                        aValue);
  if (rv == NS_CONTENT_ATTR_NOT_THERE && mDocument) {
    rv = mDocument->GetBaseTarget(aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetTarget(const nsAReadableString& aValue)
{
  return NS_STATIC_CAST(nsIContent *, this)->SetAttribute(kNameSpaceID_HTML,
                                                          nsHTMLAtoms::target,
                                                          aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLAnchorElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}


NS_IMETHODIMP    
nsHTMLAnchorElement::GetProtocol(nsAWritableString& aProtocol)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> url;
  nsresult result = NS_OK;
  
  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(getter_AddRefs(url), href);
    if (NS_FAILED(result))
      return result;

    char* protocol;
    result = url->GetScheme(&protocol);
    if (NS_FAILED(result))
      return result;

    aProtocol.Assign(NS_ConvertASCIItoUCS2(protocol));
    aProtocol.Append(PRUnichar(':'));

    nsCRT::free(protocol);
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHost(nsAWritableString& aHost)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> url;
  nsresult result = NS_OK;
  
  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(getter_AddRefs(url), href);
    if (NS_OK == result) {
      char* host;
      result = url->GetHost(&host);
      if (result == NS_OK) {
        aHost.Assign(NS_ConvertASCIItoUCS2(host));
        nsCRT::free(host);
        PRInt32 port;
        (void)url->GetPort(&port);
        if (-1 != port) {
          aHost.Append(NS_LITERAL_STRING(":"));
          nsAutoString portStr;
          portStr.AppendInt(port);
          aHost.Append(portStr);
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHostname(nsAWritableString& aHostname)
{
  nsAutoString href;
  nsIURI *url;
  nsresult result = NS_OK;
  
  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(&url, href);
    if (NS_OK == result) {
      char* host;
      result = url->GetHost(&host);
      if (result == NS_OK) {
        aHostname.Assign(NS_ConvertASCIItoUCS2(host));
        nsCRT::free(host);
      }
      NS_RELEASE(url);
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetPathname(nsAWritableString& aPathname)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  aPathname.Truncate();
  
  result = GetHref(href);
  if (NS_FAILED(result)) {
    return result;
  }

  result = NS_NewURI(getter_AddRefs(uri), href);
  if (NS_FAILED(result)) {
    return result;
  }

  nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

  if (!url) {
    return NS_OK;
  }

  char* file;
  result = url->GetFilePath(&file);
  if (NS_FAILED(result)) {
    return result;
  }

  aPathname.Assign(NS_ConvertASCIItoUCS2(file));
  nsCRT::free(file);

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetSearch(nsAWritableString& aSearch)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(getter_AddRefs(uri), href);
    if (NS_OK == result) {
      char *search;
      nsIURL* url;
      result = uri->QueryInterface(NS_GET_IID(nsIURL), (void**)&url);

      if (NS_SUCCEEDED(result)) {
        result = url->GetEscapedQuery(&search);
        NS_RELEASE(url);
      }

      if (NS_SUCCEEDED(result) && search && (*search)) {
        aSearch.Assign(PRUnichar('?'));
        aSearch.Append(NS_ConvertASCIItoUCS2(search));
        nsCRT::free(search);
      }
      else {
        aSearch.SetLength(0);
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetPort(nsAWritableString& aPort)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> url;
  nsresult result = NS_OK;

  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(getter_AddRefs(url), href);
    if (NS_OK == result) {
      aPort.Truncate(0);
      PRInt32 port;
      (void)url->GetPort(&port);
      if (-1 != port) {
        nsAutoString portStr;
        portStr.AppendInt(port);
        aPort.Append(portStr);
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetHash(nsAWritableString& aHash)
{
  nsAutoString href;
  nsCOMPtr<nsIURI> uri;
  nsresult result = NS_OK;

  result = GetHref(href);
  if (NS_OK == result) {
    result = NS_NewURI(getter_AddRefs(uri), href);

    if (NS_OK == result) {
      char *ref;
      nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
      if (url) {
        result = url->GetRef(&ref);
      }

      if (result == NS_OK && (nsnull != ref) && ('\0' != *ref)) {
        aHash.Assign(PRUnichar('#'));
        aHash.Append(NS_ConvertASCIItoUCS2(ref));
        nsCRT::free(ref);
      }
      else {
        aHash.SetLength(0);
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLAnchorElement::GetText(nsAWritableString& aText)
{
  aText.Truncate();

  nsCOMPtr<nsIDOMNode> child;

  GetFirstChild(getter_AddRefs(child));

  if (child) {
    child->GetNodeValue(aText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::ToString(nsAWritableString& aSource)
{
  return GetHref(aSource);
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetLinkState(nsLinkState &aState)
{
  aState = mLinkState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::SetLinkState(nsLinkState aState)
{
  mLinkState = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAnchorElement::GetHrefCString(char* &aBuf)
{
  // Get href= attribute (relative URL).
  nsAutoString relURLSpec;

  if (NS_CONTENT_ATTR_HAS_VALUE ==
      NS_STATIC_CAST(nsIContent *, this)->GetAttribute(kNameSpaceID_HTML,
                                                       nsHTMLAtoms::href,
                                                       relURLSpec)) {
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
    // Absolute URL is null to say we have no HREF.
    aBuf = nsnull;
  }

  return NS_OK;
}
