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
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsIHTMLContent.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsITextContent.h"
#include "nsIDocument.h"
#include "nsIScriptLoader.h"
#include "nsIScriptLoaderObserver.h"
#include "nsIScriptElement.h"
#include "nsGUIEvent.h"
#include "nsIURI.h"

class nsHTMLScriptElement : public nsGenericHTMLContainerElement,
                            public nsIDOMHTMLScriptElement,
                            public nsIScriptLoaderObserver,
                            public nsIScriptElement
{
public:
  nsHTMLScriptElement();
  virtual ~nsHTMLScriptElement();

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNode
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsGenericHTMLContainerElement::)

  // nsIDOMElement
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLElement
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLContainerElement::)

  // nsIDOMHTMLScriptElement
  NS_DECL_NSIDOMHTMLSCRIPTELEMENT

  // nsIScriptLoaderObserver
  NS_DECL_NSISCRIPTLOADEROBSERVER

  // nsIScriptElement
  NS_IMETHOD SetLineNumber(PRUint32 aLineNumber);
  NS_IMETHOD GetLineNumber(PRUint32* aLineNumber);

  NS_IMETHOD SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                     const nsAString& aValue, PRBool aNotify);
  NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep,
                         PRBool aCompileEventHandlers);
  NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex,
                           PRBool aNotify, PRBool aDeepSetDocument);
  NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify,
                           PRBool aDeepSetDocument);

  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);
  virtual nsresult SetInnerHTML(const nsAString& aInnerHTML);

#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

protected:
  PRUint32 mLineNumber;
  PRPackedBool mIsEvaluated;

  /**
   * Processes the script if it's in the document-tree and links to or
   * contains a script. Once it has been evaluated there is no way to make it
   * reevaluate the script, you'll have to create a new element. This also means
   * that when adding a src attribute to an element that already contains an
   * inline script, the script referenced by the src attribute will not be
   * loaded.
   *
   * In order to be able to use multiple childNodes, or to use the
   * fallback-mechanism of using both inline script and linked script you have
   * to add all attributes and childNodes before adding the element to the
   * document-tree.
   */
  void MaybeProcessScript();
};

nsresult
NS_NewHTMLScriptElement(nsIHTMLContent** aInstancePtrResult,
                        nsINodeInfo *aNodeInfo)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  nsHTMLScriptElement* it = new nsHTMLScriptElement();

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


nsHTMLScriptElement::nsHTMLScriptElement()
{
  mLineNumber = 0;
  mIsEvaluated = PR_FALSE;
}

nsHTMLScriptElement::~nsHTMLScriptElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLScriptElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLScriptElement, nsGenericElement) 

// QueryInterface implementation for nsHTMLScriptElement
NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLScriptElement,
                                    nsGenericHTMLContainerElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLScriptElement)
  NS_INTERFACE_MAP_ENTRY(nsIScriptLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIScriptElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLScriptElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMETHODIMP 
nsHTMLScriptElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue, PRBool aNotify)
{
  nsresult rv = nsGenericHTMLContainerElement::SetAttr(aNameSpaceID,
                                                       aName,
                                                       aValue,
                                                       aNotify);
  if (NS_SUCCEEDED(rv) && aNotify && aName == nsHTMLAtoms::src &&
      aNameSpaceID == kNameSpaceID_None) {
    MaybeProcessScript();
  }

  return rv;
}

NS_IMETHODIMP 
nsHTMLScriptElement::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                 PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLContainerElement::SetDocument(aDocument, aDeep,
                                                           aCompileEventHandlers);
  if (NS_SUCCEEDED(rv) && aDocument) {
    MaybeProcessScript();
  }

  return rv;
}

NS_IMETHODIMP 
nsHTMLScriptElement::InsertChildAt(nsIContent* aKid, PRInt32 aIndex,
                                   PRBool aNotify, PRBool aDeepSetDocument)
{
  nsresult rv = nsGenericHTMLContainerElement::InsertChildAt(aKid, aIndex,
                                                             aNotify,
                                                             aDeepSetDocument);
  if (NS_SUCCEEDED(rv) && aNotify) {
    MaybeProcessScript();
  }

  return rv;
}

NS_IMETHODIMP 
nsHTMLScriptElement::AppendChildTo(nsIContent* aKid, PRBool aNotify,
                                   PRBool aDeepSetDocument)
{
  nsresult rv = nsGenericHTMLContainerElement::AppendChildTo(aKid, aNotify,
                                                             aDeepSetDocument);
  if (NS_SUCCEEDED(rv) && aNotify) {
    MaybeProcessScript();
  }

  return rv;
}

nsresult
nsHTMLScriptElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsHTMLScriptElement* it = new nsHTMLScriptElement();

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
nsHTMLScriptElement::GetText(nsAString& aValue)
{
  return GetContentsAsText(aValue);
}

NS_IMETHODIMP
nsHTMLScriptElement::SetText(const nsAString& aValue)
{
  return ReplaceContentsWithText(aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLScriptElement::GetHtmlFor(nsAString& aValue)
{
  // XXX write me
//  GetAttribute(nsHTMLAtoms::charset, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::SetHtmlFor(const nsAString& aValue)
{
  // XXX write me
//  return SetAttr(nsHTMLAtoms::charset, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::GetEvent(nsAString& aValue)
{
  // XXX write me
//  GetAttribute(nsHTMLAtoms::charset, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScriptElement::SetEvent(const nsAString& aValue)
{
  // XXX write me
//  return SetAttr(nsHTMLAtoms::charset, aValue);
  return NS_OK;
}

NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Charset, charset)
NS_IMPL_BOOL_ATTR(nsHTMLScriptElement, Defer, defer)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLScriptElement, Type, type)

nsresult
nsHTMLScriptElement::GetInnerHTML(nsAString& aInnerHTML)
{
  return GetContentsAsText(aInnerHTML);
}

nsresult
nsHTMLScriptElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  return ReplaceContentsWithText(aInnerHTML, PR_TRUE);
}


#ifdef DEBUG
NS_IMETHODIMP
nsHTMLScriptElement::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const
{
  *aResult = sizeof(*this) + BaseSizeOf(aSizer);

  return NS_OK;
}
#endif

/* void scriptAvailable (in nsresult aResult, in nsIDOMHTMLScriptElement aElement, in nsIURI aURI, in PRInt32 aLineNo, in PRUint32 aScriptLength, [size_is (aScriptLength)] in wstring aScript); */
NS_IMETHODIMP 
nsHTMLScriptElement::ScriptAvailable(nsresult aResult, 
                                     nsIDOMHTMLScriptElement *aElement, 
                                     PRBool aIsInline,
                                     PRBool aWasPending,
                                     nsIURI *aURI, 
                                     PRInt32 aLineNo,
                                     const nsAString& aScript)
{
  nsresult rv = NS_OK;
  if (!aIsInline && NS_FAILED(aResult)) {
    nsCOMPtr<nsIPresContext> presContext;
    GetPresContext(this, getter_AddRefs(presContext)); 

    nsEventStatus status = nsEventStatus_eIgnore;
    nsScriptErrorEvent event;
    event.eventStructType = NS_EVENT;

    event.message = NS_SCRIPT_ERROR;

    event.lineNr = aLineNo;

    NS_NAMED_LITERAL_STRING(errorString, "Error loading script");
    event.errorMsg = errorString.get();

    nsCAutoString spec;
    aURI->GetSpec(spec);

    NS_ConvertUTF8toUCS2 fileName(spec);

    event.fileName = fileName.get();

    rv = HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT,
                        &status);
  }

  return NS_OK;
}

/* void scriptEvaluated (in nsresult aResult, in nsIDOMHTMLScriptElement aElement); */
NS_IMETHODIMP 
nsHTMLScriptElement::ScriptEvaluated(nsresult aResult, 
                                     nsIDOMHTMLScriptElement *aElement,
                                     PRBool aIsInline,
                                     PRBool aWasPending)
{
  nsresult rv = NS_OK;
  if (!aIsInline) {
    nsCOMPtr<nsIPresContext> presContext;
    GetPresContext(this, getter_AddRefs(presContext)); 

    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;

    event.message = NS_SUCCEEDED(aResult) ? NS_SCRIPT_LOAD : NS_SCRIPT_ERROR;
    rv = HandleDOMEvent(presContext, &event, nsnull, NS_EVENT_FLAG_INIT,
                        &status);
  }

  return rv;
}

NS_IMETHODIMP 
nsHTMLScriptElement::SetLineNumber(PRUint32 aLineNumber)
{
  mLineNumber = aLineNumber;

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLScriptElement::GetLineNumber(PRUint32* aLineNumber)
{
  NS_ENSURE_ARG_POINTER(aLineNumber);

  *aLineNumber = mLineNumber;

  return NS_OK;
}

void
nsHTMLScriptElement::MaybeProcessScript()
{
  if (mIsEvaluated || !mDocument || !mParent) {
    return;
  }

  // We'll always call this to make sure that
  // ScriptAvailable/ScriptEvaluated gets called. See bug 153600
  nsCOMPtr<nsIScriptLoader> loader;
  mDocument->GetScriptLoader(getter_AddRefs(loader));
  if (loader) {
    loader->ProcessScriptElement(this, this);
  }

  // But we'll only set mIsEvaluated if we did really load or evaluate
  // something
  if (HasAttr(kNameSpaceID_None, nsHTMLAtoms::src) || mChildren.Count()) {
    mIsEvaluated = PR_TRUE;
  }
}
