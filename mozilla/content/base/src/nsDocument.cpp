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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Johnny Stenback <jst@netscape.com>
 *   L. David Baron  <dbaron@dbaron.org>
 *   Pierre Phaneuf  <pp@ludusdesign.com>
 *   Pete Collins    <petejc@collab.net>
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
#include "plstr.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsDocument.h"
#include "nsIArena.h"
#include "nsUnicharUtils.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEventStateManager.h"
#include "nsContentList.h"
#include "nsIObserver.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"

#include "nsGUIEvent.h"

#include "nsIDOMStyleSheet.h"
#include "nsDOMAttribute.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocumentXBL.h"
#include "nsGenericElement.h"
#include "nsIDOMEventGroup.h"

#include "nsRange.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "nsDOMDocumentType.h"
#include "nsTreeWalker.h"

#include "nsIServiceManager.h"

#include "nsContentCID.h"
#include "nsDOMError.h"
#include "nsIScrollableView.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleSet.h"
#include "nsContentUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIXBLService.h"
#include "nsIXPointer.h"
#include "nsIFileChannel.h"

#include "nsNetUtil.h"     // for NS_MakeAbsoluteURI

#include "nsIScriptSecurityManager.h"
#include "nsIAggregatePrincipal.h"
#include "nsIPrivateDOMImplementation.h"

#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"

#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsXULAtoms.h"

// for radio group stuff
#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioVisitor.h"
#include "nsIFormControl.h"

#include "nsBidiUtils.h"

static NS_DEFINE_CID(kDOMEventGroupCID, NS_DOMEVENTGROUP_CID);

#include "nsILineBreakerFactory.h"
#include "nsIWordBreakerFactory.h"
#include "nsLWBrkCIID.h"

#include "nsHTMLAtoms.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "nsScriptEventManager.h"
#include "nsIXPathEvaluatorInternal.h"

// Helper structs for the content->subdoc map

class SubDocMapEntry : public PLDHashEntryHdr
{
public:
  // Both of these are strong references
  nsIContent *mKey; // must be first, to look like PLDHashEntryStub
  nsIDocument *mSubDocument;
};

struct FindContentData
{
  FindContentData(nsIDocument *aSubDoc)
    : mSubDocument(aSubDoc), mResult(nsnull)
  {
  }

  nsISupports *mSubDocument;
  nsIContent *mResult;
};


/**
 * A struct that holds all the information about a radio group.
 */
struct nsRadioGroupStruct
{
  /**
   * A strong pointer to the currently selected radio button.
   */
  nsCOMPtr<nsIDOMHTMLInputElement> mSelectedRadioButton;
  nsSmallVoidArray mRadioButtons;
};


nsDOMStyleSheetList::nsDOMStyleSheetList(nsIDocument *aDocument)
{
  mLength = -1;
  // Not reference counted to avoid circular references.
  // The document will tell us when its going away.
  mDocument = aDocument;
  mDocument->AddObserver(this);
}

nsDOMStyleSheetList::~nsDOMStyleSheetList()
{
  if (mDocument) {
    mDocument->RemoveObserver(this);
  }
}


// XXX couldn't we use the GetIIDs method from CSSStyleSheetList here?
// QueryInterface implementation for nsDOMStyleSheetList
NS_INTERFACE_MAP_BEGIN(nsDOMStyleSheetList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStyleSheetList)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStyleSheetList)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DocumentStyleSheetList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMStyleSheetList)
NS_IMPL_RELEASE(nsDOMStyleSheetList)


NS_IMETHODIMP
nsDOMStyleSheetList::GetLength(PRUint32* aLength)
{
  if (mDocument) {
    // XXX Find the number and then cache it. We'll use the
    // observer notification to figure out if new ones have
    // been added or removed.
    if (-1 == mLength) {
      mDocument->GetNumberOfStyleSheets(PR_FALSE, &mLength);

#ifdef DEBUG
      PRInt32 i;
      for (i = 0; i < mLength; i++) {
        nsCOMPtr<nsIStyleSheet> sheet;
        mDocument->GetStyleSheetAt(i, PR_FALSE, getter_AddRefs(sheet));
        nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(sheet));
        NS_ASSERTION(domss, "All \"normal\" sheets implement nsIDOMStyleSheet");
      }
#endif
    }
    *aLength = mLength;
  }
  else {
    *aLength = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetList::Item(PRUint32 aIndex, nsIDOMStyleSheet** aReturn)
{
  *aReturn = nsnull;
  if (mDocument) {
    PRInt32 count = 0;
    mDocument->GetNumberOfStyleSheets(PR_FALSE, &count);
    if (aIndex < (PRUint32)count) {
      nsCOMPtr<nsIStyleSheet> sheet;
      mDocument->GetStyleSheetAt(aIndex, PR_FALSE, getter_AddRefs(sheet));
      NS_ASSERTION(sheet, "Must have a sheet");
      return CallQueryInterface(sheet, aReturn);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetList::StyleSheetAdded(nsIDocument *aDocument,
                                     nsIStyleSheet* aStyleSheet)
{
  if (-1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength++;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetList::StyleSheetRemoved(nsIDocument *aDocument,
                                       nsIStyleSheet* aStyleSheet)
{
  if (-1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetList::DocumentWillBeDestroyed(nsIDocument *aDocument)
{
  if (nsnull != mDocument) {
    aDocument->RemoveObserver(this);
    mDocument = nsnull;
  }

  return NS_OK;
}

class nsDOMImplementation : public nsIDOMDOMImplementation,
                            public nsIPrivateDOMImplementation
{
public:
  nsDOMImplementation(nsIURI* aBaseURI = nsnull);
  virtual ~nsDOMImplementation();

  NS_DECL_ISUPPORTS

  // nsIDOMDOMImplementation
  NS_DECL_NSIDOMDOMIMPLEMENTATION

  // nsIPrivateDOMImplementation
  NS_IMETHOD Init(nsIURI* aBaseURI);

protected:
  nsCOMPtr<nsIURI> mBaseURI;
};


nsresult
NS_NewDOMImplementation(nsIDOMDOMImplementation** aInstancePtrResult)
{
  *aInstancePtrResult = new nsDOMImplementation();
  if (!*aInstancePtrResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsDOMImplementation::nsDOMImplementation(nsIURI* aBaseURI)
{
  mBaseURI = aBaseURI;
}

nsDOMImplementation::~nsDOMImplementation()
{
}

// QueryInterface implementation for nsDOMImplementation
NS_INTERFACE_MAP_BEGIN(nsDOMImplementation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMImplementation)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMImplementation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDOMImplementation)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMImplementation)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMImplementation);
NS_IMPL_RELEASE(nsDOMImplementation);


NS_IMETHODIMP
nsDOMImplementation::HasFeature(const nsAString& aFeature,
                                const nsAString& aVersion,
                                PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocumentType(const nsAString& aQualifiedName,
                                        const nsAString& aPublicId,
                                        const nsAString& aSystemId,
                                        nsIDOMDocumentType** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  return NS_NewDOMDocumentType(aReturn, aQualifiedName, nsnull, nsnull,
                               aPublicId, aSystemId, nsString());
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocument(const nsAString& aNamespaceURI,
                                    const nsAString& aQualifiedName,
                                    nsIDOMDocumentType* aDoctype,
                                    nsIDOMDocument** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = nsnull;

  if (aDoctype) {
    nsCOMPtr<nsIDOMDocument> owner;
    aDoctype->GetOwnerDocument(getter_AddRefs(owner));
    if (owner) {
      return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;
    }
  }

  nsresult rv = NS_NewDOMDocument(aReturn, aNamespaceURI, aQualifiedName,
                                  aDoctype, mBaseURI);

  nsCOMPtr<nsIDocShell> docShell;
  nsContentUtils::GetDocShellFromCaller(getter_AddRefs(docShell));
  if (docShell) {
    nsCOMPtr<nsIPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
      nsCOMPtr<nsISupports> container;
      presContext->GetContainer(getter_AddRefs(container));
      nsCOMPtr<nsIDocument> document = do_QueryInterface(*aReturn);
      if (document) {
        document->SetContainer(container);
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsDOMImplementation::Init(nsIURI* aBaseURI)
{
  mBaseURI = aBaseURI;
  return NS_OK;
}

// ==================================================================
// =
// ==================================================================

nsDocumentChildNodes::nsDocumentChildNodes(nsIDocument* aDocument)
{
  MOZ_COUNT_CTOR(nsDocumentChildNodes);

  // We don't reference count our document reference (to avoid circular
  // references). We'll be told when the document goes away.
  mDocument = aDocument;
}

nsDocumentChildNodes::~nsDocumentChildNodes()
{
  MOZ_COUNT_DTOR(nsDocumentChildNodes);
}

NS_IMETHODIMP
nsDocumentChildNodes::GetLength(PRUint32* aLength)
{
  PRInt32 count = 0;

  if (mDocument) {
    mDocument->GetChildCount(count);
  }

  *aLength = (PRUint32)count;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentChildNodes::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  *aReturn = nsnull;

  if (mDocument) {
    nsCOMPtr<nsIContent> content;
    mDocument->ChildAt(aIndex, *getter_AddRefs(content));
    if (content) {
      return CallQueryInterface(content, aReturn);
    }
  }

  return NS_OK;
}

void
nsDocumentChildNodes::DropReference()
{
  mDocument = nsnull;
}

class nsXPathDocumentTearoff : public nsIDOMXPathEvaluator
{
public:
  nsXPathDocumentTearoff(nsIDOMXPathEvaluator* aEvaluator,
                         nsIDocument* aDocument)
    : mEvaluator(aEvaluator), mDocument(aDocument)
  {
  }
  virtual ~nsXPathDocumentTearoff()
  {
  }

  NS_DECL_ISUPPORTS_INHERITED

  NS_FORWARD_NSIDOMXPATHEVALUATOR(mEvaluator->)

private:
  nsCOMPtr<nsIDOMXPathEvaluator> mEvaluator;
  nsIDocument* mDocument;
};

NS_INTERFACE_MAP_BEGIN(nsXPathDocumentTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathEvaluator)
NS_INTERFACE_MAP_END_AGGREGATED(mDocument)

NS_IMPL_ADDREF_USING_AGGREGATOR(nsXPathDocumentTearoff, mDocument)
NS_IMPL_RELEASE_USING_AGGREGATOR(nsXPathDocumentTearoff, mDocument)


// ==================================================================
// =
// ==================================================================

  // NOTE! nsDocument::operator new() zeroes out all members, so don't
  // bother initializing members to 0.

nsDocument::nsDocument()
  : mCharacterSet(NS_LITERAL_STRING("ISO-8859-1")),
    mNextContentID(NS_CONTENT_ID_COUNTER_BASE)
{

  // NOTE! nsDocument::operator new() zeroes out all members, so don't
  // bother initializing members to 0.

  // Force initialization.
  mBindingManager = do_CreateInstance("@mozilla.org/xbl/binding-manager;1");

  nsCOMPtr<nsIDocumentObserver> observer(do_QueryInterface(mBindingManager));
  if (observer) {
    // The binding manager must always be the first observer of the
    // document.

    mObservers.InsertElementAt(observer, 0);
  }
}

nsDocument::~nsDocument()
{
  mInDestructor = PR_TRUE;

  // XXX Inform any remaining observers that we are going away.
  // Note that this currently contradicts the rule that all
  // observers must hold on to live references to the document.
  // This notification will occur only after the reference has
  // been dropped.

  // if an observer removes itself, we're ok (not if it removes others though)
  PRInt32 indx;
  for (indx = mObservers.Count() - 1; indx >= 0; --indx) {
    // XXX Should this be a kungfudeathgrip?!!!!
    nsIDocumentObserver* observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(indx));

    observer->DocumentWillBeDestroyed(this);
  }

  mParentDocument = nsnull;

  // Kill the subdocument map, doing this will release its strong
  // references, if any.
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nsnull;
  }

  if (mRootContent) {
    nsCOMPtr<nsIDocument> doc;

    mRootContent->GetDocument(*getter_AddRefs(doc));

    if (doc) {
      // The root content still has a pointer back to the document,
      // clear the document pointer in all children.

      PRInt32 count = mChildren.Count();
      for (indx = 0; indx < count; ++indx) {
        mChildren[indx]->SetDocument(nsnull, PR_TRUE, PR_FALSE);
      }
    }
  }

  mRootContent = nsnull;
  mChildren.Clear();

  // Let the stylesheets know we're going away
  indx = mStyleSheets.Count();
  while (--indx >= 0) {
    mStyleSheets[indx]->SetOwningDocument(nsnull);
  }

  if (mChildNodes) {
    mChildNodes->DropReference();
  }

  if (mListenerManager) {
    mListenerManager->SetListenerTarget(nsnull);
  }

  if (mScriptLoader) {
    mScriptLoader->DropDocumentReference();
  }

  if (mCSSLoader) {
    mCSSLoader->DropDocumentReference();
  }

  if (mNodeInfoManager) {
    mNodeInfoManager->DropDocumentReference();
  }

  delete mHeaderData;
  delete mBoxObjectTable;
  delete mXPathDocument;
}

PRBool gCheckedForXPathDOM = PR_FALSE;
PRBool gHaveXPathDOM = PR_FALSE;

NS_INTERFACE_MAP_BEGIN(nsDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSDocument)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3DocumentEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentStyle)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentView)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentRange)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentTraversal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDocumentXBL)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventReceiver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3Node)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIRadioGroupContainer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocument)
  if (aIID.Equals(NS_GET_IID(nsIDOMXPathEvaluator)) &&
      (!gCheckedForXPathDOM || gHaveXPathDOM)) {
    if (!mXPathDocument) {
      nsCOMPtr<nsIDOMXPathEvaluator> evaluator;
      nsresult rv;
      evaluator = do_CreateInstance(NS_XPATH_EVALUATOR_CONTRACTID, &rv);
      gCheckedForXPathDOM = PR_TRUE;
      gHaveXPathDOM = (evaluator != nsnull);
      if (rv == NS_ERROR_FACTORY_NOT_REGISTERED) {
          return NS_ERROR_NO_INTERFACE;
      }
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIXPathEvaluatorInternal> internal =
          do_QueryInterface(evaluator);
      if (internal) {
          internal->SetDocument(this);
      }

      mXPathDocument = new nsXPathDocumentTearoff(evaluator, this);
      NS_ENSURE_TRUE(mXPathDocument, NS_ERROR_OUT_OF_MEMORY);
    }
    foundInterface = mXPathDocument;
  }
  else
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDocument)
NS_IMPL_RELEASE(nsDocument)

nsresult
nsDocument::Init()
{
  if (mArena) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  nsresult rv = NS_NewHeapArena(getter_AddRefs(mArena), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  mNodeInfoManager = new nsNodeInfoManager();
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_OUT_OF_MEMORY);

  mNodeInfoManager->Init(this);

  return rv;
}

NS_IMETHODIMP
nsDocument::GetArena(nsIArena** aArena)
{
  *aArena = mArena;
  NS_IF_ADDREF(*aArena);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsCOMPtr<nsIURI> uri;
  if (aChannel) {
    aChannel->GetOriginalURI(getter_AddRefs(uri));

    PRBool isChrome = PR_FALSE;
    PRBool isRes = PR_FALSE;
    uri->SchemeIs("chrome", &isChrome);
    uri->SchemeIs("resource", &isRes);

    if (!isChrome && !isRes)
      aChannel->GetURI(getter_AddRefs(uri));
  }

  nsresult rv = ResetToURI(uri, aLoadGroup);

  if (aChannel) {
    nsCOMPtr<nsISupports> owner;
    aChannel->GetOwner(getter_AddRefs(owner));

    mPrincipal = do_QueryInterface(owner);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup)
{
  mDocumentTitle.Truncate();

  mPrincipal = nsnull;
  mDocumentLoadGroup = nsnull;

  // Delete references to sub-documents and kill the subdocument map,
  // if any. It holds strong references
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nsnull;
  }

  mRootContent = nsnull;
  PRInt32 count, i;
  count = mChildren.Count();
  for (i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> content = mChildren[i];

    content->SetDocument(nsnull, PR_TRUE, PR_TRUE);
    ContentRemoved(nsnull, content, i);
  }
  mChildren.Clear();

  // The stylesheets should forget us
  PRInt32 indx = mStyleSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mStyleSheets[indx];
    sheet->SetOwningDocument(nsnull);

    PRBool applicable;
    sheet->GetApplicable(applicable);
    if (applicable) {
      RemoveStyleSheetFromStyleSets(sheet);
    }

    // XXX Tell observers?
  }

  // Release all the sheets
  mStyleSheets.Clear();

  // Release the listener manager
  mListenerManager = nsnull;

  // Release the stylesheets list.
  mDOMStyleSheets = nsnull;

  mDocumentURL = aURI;
  mDocumentBaseURL = mDocumentURL;

  if (aLoadGroup) {
    mDocumentLoadGroup = getter_AddRefs(NS_GetWeakReference(aLoadGroup));
    // there was an assertion here that aLoadGroup was not null.  This
    // is no longer valid nsWebShell::SetDocument does not create a
    // load group, and it works just fine.
  }

  mLastModified.Truncate();
  mContentType.Truncate();
  mContentLanguage.Truncate();

  mXMLDeclarationBits = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetDocumentURL(nsIURI* aURI)
{
  mDocumentURL = aURI;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              nsISupports* aContainer,
                              nsIStreamListener **aDocListener,
                              PRBool aReset, nsIContentSink* aSink)
{
  nsresult rv = NS_OK;
  if (aReset) {
    rv = Reset(aChannel, aLoadGroup);
  }

  nsCAutoString contentType;
  if (NS_SUCCEEDED(aChannel->GetContentType(contentType))) {
    // XXX this is only necessary for viewsource:
    nsACString::const_iterator start, end, semicolon;
    contentType.BeginReading(start);
    contentType.EndReading(end);
    semicolon = start;
    FindCharInReadable(';', semicolon, end);
    mContentType = Substring(start, semicolon);
  }

  RetrieveRelevantHeaders(aChannel);

  return rv;
}

NS_IMETHODIMP
nsDocument::StopDocumentLoad()
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentTitle(nsAString& aTitle) const
{
  aTitle = mDocumentTitle;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentURL(nsIURI** aURI) const
{
  *aURI = mDocumentURL;
  NS_IF_ADDREF(*aURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLastModified(nsAString& aLastModified)
{
  if (!mLastModified.IsEmpty()) {
    CopyASCIItoUCS2(mLastModified, aLastModified);
  } else {
    // If we for whatever reason failed to find the last modified time
    // (or even the current time), fall back to what NS4.x returned.
    CopyASCIItoUCS2(NS_LITERAL_CSTRING("January 1, 1970 GMT"), aLastModified);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPrincipal(nsIPrincipal **aPrincipal)
{
  *aPrincipal = nsnull;

  if (!mPrincipal) {
    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> securityManager =
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;
    NS_WARN_IF_FALSE(mDocumentURL, "no URL!");
    rv = securityManager->GetCodebasePrincipal(mDocumentURL,
                                               getter_AddRefs(mPrincipal));

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  *aPrincipal = mPrincipal;
  NS_ADDREF(*aPrincipal);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddPrincipal(nsIPrincipal *aNewPrincipal)
{
  nsresult rv;
  if (!mPrincipal) {
    nsCOMPtr<nsIPrincipal> principal;
    rv = GetPrincipal(getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIAggregatePrincipal> agg(do_QueryInterface(mPrincipal, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = agg->Intersect(aNewPrincipal);
    if (NS_FAILED(rv))
      return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetContentType(nsAString& aContentType)
{
  aContentType = NS_ConvertUTF8toUCS2(mContentType);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetContentType(const nsAString& aContentType)
{
  NS_ASSERTION(mContentType.IsEmpty() ||
               mContentType.Equals(NS_ConvertUCS2toUTF8(aContentType)),
               "Do you really want to change the content-type?");

  mContentType = NS_ConvertUCS2toUTF8(aContentType);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetContentLanguage(nsAString& aContentLanguage) const
{
  CopyASCIItoUCS2(mContentLanguage, aContentLanguage);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentLoadGroup(nsILoadGroup **aGroup) const
{
  nsCOMPtr<nsILoadGroup> group(do_QueryReferent(mDocumentLoadGroup));

  *aGroup = group;
  NS_IF_ADDREF(*aGroup);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBaseURL(nsIURI*& aURL) const
{
  aURL = mDocumentBaseURL;

  if (!aURL) {
    aURL = mDocumentURL;
  }

  NS_IF_ADDREF(aURL);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetBaseURL(nsIURI* aURL)
{
  nsresult rv = NS_OK;

  if (aURL) {
    nsCOMPtr<nsIScriptSecurityManager> securityManager =
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = securityManager->CheckLoadURI(mDocumentURL, aURL,
                                         nsIScriptSecurityManager::STANDARD);
      if (NS_SUCCEEDED(rv)) {
        mDocumentBaseURL = aURL;
      }
    }
  } else {
    mDocumentBaseURL = nsnull;
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::GetBaseTarget(nsAString &aBaseTarget)
{
  aBaseTarget.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetBaseTarget(const nsAString &aBaseTarget)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentCharacterSet(nsAString& aCharSetID)
{
  aCharSetID = mCharacterSet;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetDocumentCharacterSet(const nsAString& aCharSetID)
{
  if (!mCharacterSet.Equals(aCharSetID)) {
    mCharacterSet = aCharSetID;

    PRInt32 n = mCharSetObservers.Count();

    for (PRInt32 i = 0; i < n; i++) {
      nsIObserver* observer =
        NS_STATIC_CAST(nsIObserver *, mCharSetObservers.ElementAt(i));

      observer->Observe(NS_STATIC_CAST(nsIDocument *, this), "charset",
                        PromiseFlatString(aCharSetID).get());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentCharacterSetSource(PRInt32* aCharsetSource)
{
  *aCharsetSource = mCharacterSetSource;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetDocumentCharacterSetSource(PRInt32 aCharsetSource)
{
  mCharacterSetSource = aCharsetSource;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddCharSetObserver(nsIObserver* aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  NS_ENSURE_TRUE(mCharSetObservers.AppendElement(aObserver), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::RemoveCharSetObserver(nsIObserver* aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  NS_ENSURE_TRUE(mCharSetObservers.RemoveElement(aObserver), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLineBreaker(nsILineBreaker** aResult)
{
  if (!mLineBreaker) {
    // no line breaker, find a default one
    nsresult rv;
    nsCOMPtr<nsILineBreakerFactory> lbf =
      do_GetService(NS_LWBRK_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    lbf->GetBreaker(nsString(), getter_AddRefs(mLineBreaker));
    NS_ENSURE_TRUE(mLineBreaker, NS_ERROR_UNEXPECTED);
  }

  *aResult = mLineBreaker;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetLineBreaker(nsILineBreaker* aLineBreaker)
{
  mLineBreaker = aLineBreaker;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetWordBreaker(nsIWordBreaker** aResult)
{
  if (!mWordBreaker) {
    // no word breaker, find a default one
    nsresult rv;
    nsCOMPtr<nsIWordBreakerFactory> wbf =
      do_GetService(NS_LWBRK_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    wbf->GetBreaker(nsString(), getter_AddRefs(mWordBreaker));
    NS_ENSURE_TRUE(wbf, NS_ERROR_UNEXPECTED);
  }

  *aResult = mWordBreaker;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetWordBreaker(nsIWordBreaker* aWordBreaker)
{
  mWordBreaker = aWordBreaker;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const
{
  aData.Truncate();
  const nsDocHeaderData* data = mHeaderData;
  while (data) {
    if (data->mField == aHeaderField) {
      aData = data->mData;

      break;
    }
    data = data->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetHeaderData(nsIAtom* aHeaderField, const nsAString& aData)
{
  NS_ENSURE_ARG_POINTER(aHeaderField);

  if (!mHeaderData) {
    if (!aData.IsEmpty()) { // don't bother storing empty string
      mHeaderData = new nsDocHeaderData(aHeaderField, aData);
    }
  }
  else {
    nsDocHeaderData* data = mHeaderData;
    nsDocHeaderData** lastPtr = &mHeaderData;
    PRBool found = PR_FALSE;
    do {  // look for existing and replace
      if (data->mField == aHeaderField) {
        if (!aData.IsEmpty()) {
          data->mData.Assign(aData);
        }
        else {  // don't store empty string
          *lastPtr = data->mNext;
          data->mNext = nsnull;
          delete data;
        }
        found = PR_TRUE;

        break;
      }
      lastPtr = &(data->mNext);
      data = *lastPtr;
    } while (data);

    if (!aData.IsEmpty() && !found) {
      // didn't find, append
      *lastPtr = new nsDocHeaderData(aHeaderField, aData);
    }
  }

  if (aHeaderField == nsHTMLAtoms::headerDefaultStyle) {
    // switch alternate style sheets based on default
    nsAutoString type;
    nsAutoString title;
    PRInt32 index;

    mCSSLoader->SetPreferredSheet(aData);

    PRInt32 count = mStyleSheets.Count();
    for (index = 0; index < count; index++) {
      nsIStyleSheet* sheet = mStyleSheets[index];
      sheet->GetType(type);
      if (!type.Equals(NS_LITERAL_STRING("text/html"))) {
        sheet->GetTitle(title);
        if (!title.IsEmpty()) {  // if sheet has title
          PRBool enabled =
            (!aData.IsEmpty() &&
             title.Equals(aData, nsCaseInsensitiveStringComparator()));

          sheet->SetEnabled(enabled);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateShell(nsIPresContext* aContext, nsIViewManager* aViewManager,
                        nsIStyleSet* aStyleSet,
                        nsIPresShell** aInstancePtrResult)
{
  // Don't add anything here.  Add it to |doCreateShell| instead.
  // This exists so that subclasses can pass other values for the 4th
  // parameter some of the time.
  return doCreateShell(aContext, aViewManager, aStyleSet,
                       eCompatibility_FullStandards, aInstancePtrResult);
}

nsresult
nsDocument::doCreateShell(nsIPresContext* aContext,
                          nsIViewManager* aViewManager, nsIStyleSet* aStyleSet,
                          nsCompatibility aCompatMode,
                          nsIPresShell** aInstancePtrResult)
{
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = NS_NewPresShell(getter_AddRefs(shell));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = shell->Init(this, aContext, aViewManager, aStyleSet, aCompatMode);
  NS_ENSURE_SUCCESS(rv, rv);

  // Note: we don't hold a ref to the shell (it holds a ref to us)
  mPresShells.AppendElement(shell);
  *aInstancePtrResult = shell.get();
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

PRBool
nsDocument::DeleteShell(nsIPresShell* aShell)
{
  return mPresShells.RemoveElement(aShell);
}

PRInt32
nsDocument::GetNumberOfShells()
{
  return mPresShells.Count();
}

NS_IMETHODIMP
nsDocument::GetShellAt(PRInt32 aIndex, nsIPresShell** aShell)
{
  *aShell = (nsIPresShell*) mPresShells.SafeElementAt(aIndex);
  NS_IF_ADDREF(*aShell);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetParentDocument(nsIDocument** aParent)
{
  *aParent = mParentDocument;
  NS_IF_ADDREF(*aParent);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetParentDocument(nsIDocument* aParent)
{
  // Note that we do *not* AddRef our parent because that would create
  // a circular reference.
  mParentDocument = aParent;

  return NS_OK;
}

PR_STATIC_CALLBACK(void)
SubDocClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  SubDocMapEntry *e = NS_STATIC_CAST(SubDocMapEntry *, entry);

  NS_RELEASE(e->mKey);
  NS_IF_RELEASE(e->mSubDocument);
}

PR_STATIC_CALLBACK(void)
SubDocInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry, const void *key)
{
  SubDocMapEntry *e =
    NS_CONST_CAST(SubDocMapEntry *,
                  NS_STATIC_CAST(const SubDocMapEntry *, entry));

  e->mKey = NS_CONST_CAST(nsIContent *,
                          NS_STATIC_CAST(const nsIContent *, key));
  NS_ADDREF(e->mKey);

  e->mSubDocument = nsnull;
}

NS_IMETHODIMP
nsDocument::SetSubDocumentFor(nsIContent *aContent, nsIDocument* aSubDoc)
{
  NS_ENSURE_TRUE(aContent, NS_ERROR_UNEXPECTED);

  if (!aSubDoc) {
    // aSubDoc is nsnull, remove the mapping

    if (mSubDocuments) {
      SubDocMapEntry *entry =
        NS_STATIC_CAST(SubDocMapEntry*,
                       PL_DHashTableOperate(mSubDocuments, aContent,
                                            PL_DHASH_LOOKUP));

      if (PL_DHASH_ENTRY_IS_LIVE(entry)) {
        entry->mSubDocument->SetParentDocument(nsnull);

        PL_DHashTableRawRemove(mSubDocuments, entry);
      }
    }
  } else {
    if (!mSubDocuments) {
      // Create a new hashtable

      static PLDHashTableOps hash_table_ops =
      {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        PL_DHashGetKeyStub,
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        SubDocClearEntry,
        PL_DHashFinalizeStub,
        SubDocInitEntry
      };

      mSubDocuments = PL_NewDHashTable(&hash_table_ops, nsnull,
                                       sizeof(SubDocMapEntry), 16);
      if (!mSubDocuments) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    // Add a mapping to the hash table
    SubDocMapEntry *entry =
      NS_STATIC_CAST(SubDocMapEntry*,
                     PL_DHashTableOperate(mSubDocuments, aContent,
                                          PL_DHASH_ADD));

    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (entry->mSubDocument) {
      entry->mSubDocument->SetParentDocument(nsnull);

      // Release the old sub document
      NS_RELEASE(entry->mSubDocument);
    }

    entry->mSubDocument = aSubDoc;
    NS_ADDREF(entry->mSubDocument);

    aSubDoc->SetParentDocument(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetSubDocumentFor(nsIContent *aContent, nsIDocument** aSubDoc)
{
  *aSubDoc = nsnull;

  if (mSubDocuments) {
    SubDocMapEntry *entry =
      NS_STATIC_CAST(SubDocMapEntry*,
                     PL_DHashTableOperate(mSubDocuments, aContent,
                                          PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_LIVE(entry)) {
      *aSubDoc = entry->mSubDocument;
      NS_ADDREF(*aSubDoc);
    }
  }

  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
FindContentEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      PRUint32 number, void *arg)
{
  SubDocMapEntry *entry = NS_STATIC_CAST(SubDocMapEntry*, hdr);
  FindContentData *data = NS_STATIC_CAST(FindContentData*, arg);

  if (entry->mSubDocument == data->mSubDocument) {
    data->mResult = entry->mKey;

    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsDocument::FindContentForSubDocument(nsIDocument *aDocument,
                                      nsIContent **aContent)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  if (!mSubDocuments) {
    *aContent = nsnull;
    return NS_OK;
  }

  FindContentData data(aDocument);
  PL_DHashTableEnumerate(mSubDocuments, FindContentEnumerator, &data);

  *aContent = data.mResult;
  NS_IF_ADDREF(*aContent);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetRootContent(nsIContent** aRoot)
{
  *aRoot = mRootContent;

  NS_IF_ADDREF(*aRoot);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetRootContent(nsIContent* aRoot)
{
  if (mRootContent) {
    PRInt32 indx = mChildren.IndexOf(mRootContent);
    if (aRoot) {
      mChildren.ReplaceObjectAt(aRoot, indx);
    } else {
      mChildren.RemoveObjectAt(indx);
    }
  } else if (aRoot) {
    mChildren.AppendObject(aRoot);
  }

  mRootContent = aRoot;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
  NS_PRECONDITION(aIndex >= 0, "Negative indices are bad");
  if (aIndex < 0 || aIndex >= mChildren.Count()) {
    aResult = nsnull;
  } else {
    NS_IF_ADDREF(aResult = mChildren[aIndex]);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IndexOf(nsIContent* aPossibleChild, PRInt32& aIndex) const
{
  aIndex = mChildren.IndexOf(aPossibleChild);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetChildCount(PRInt32& aCount)
{
  aCount = mChildren.Count();
  return NS_OK;
}

PRInt32
nsDocument::InternalGetNumberOfStyleSheets()
{
  return mStyleSheets.Count();
}

NS_IMETHODIMP
nsDocument::GetNumberOfStyleSheets(PRBool aIncludeSpecialSheets,
                                   PRInt32* aCount)
{
  if (aIncludeSpecialSheets) {
    *aCount = mStyleSheets.Count();
  } else {
    *aCount = InternalGetNumberOfStyleSheets();
  }
  return NS_OK;
}

already_AddRefed<nsIStyleSheet>
nsDocument::InternalGetStyleSheetAt(PRInt32 aIndex)
{
  if (aIndex < 0 || aIndex >= mStyleSheets.Count()) {
    NS_ERROR("Index out of range");
    return nsnull;
  }

  nsIStyleSheet* sheet = mStyleSheets[aIndex];
  NS_ADDREF(sheet);

  return sheet;
}

NS_IMETHODIMP
nsDocument::GetStyleSheetAt(PRInt32 aIndex, PRBool aIncludeSpecialSheets,
                            nsIStyleSheet** aSheet)
{
  if (aIncludeSpecialSheets) {
    if (aIndex >= 0 && aIndex < mStyleSheets.Count()) {
      *aSheet = mStyleSheets[aIndex];
      NS_ADDREF(*aSheet);
    } else {
      NS_ERROR("Index out of range");
      *aSheet = nsnull;
    }
  } else {
    *aSheet = InternalGetStyleSheetAt(aIndex).get();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet, PRInt32* aIndex)
{
  *aIndex = mStyleSheets.IndexOf(aSheet);
  return NS_OK;
}

// subclass hooks for sheet ordering
void
nsDocument::InternalAddStyleSheet(nsIStyleSheet* aSheet, PRUint32 aFlags)
{
  mStyleSheets.AppendObject(aSheet);
}

void
nsDocument::AddStyleSheetToStyleSets(nsIStyleSheet* aSheet)
{
  PRInt32 count = mPresShells.Count();
  PRInt32 indx;
  for (indx = 0; indx < count; ++indx) {
    nsCOMPtr<nsIPresShell> shell = (nsIPresShell *)mPresShells.ElementAt(indx);
    nsCOMPtr<nsIStyleSet> set;
    if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
      if (set) {
        set->AddDocStyleSheet(aSheet, this);
      }
    }
  }
}

void
nsDocument::AddStyleSheet(nsIStyleSheet* aSheet, PRUint32 aFlags)
{
  NS_PRECONDITION(aSheet, "null arg");
  InternalAddStyleSheet(aSheet, aFlags);
  aSheet->SetOwningDocument(this);

  PRBool applicable;
  aSheet->GetApplicable(applicable);

  if (applicable) {
    AddStyleSheetToStyleSets(aSheet);
  }

  // if an observer removes itself, we're ok (not if it removes others though)
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->StyleSheetAdded(this, aSheet);
  }
}

void
nsDocument::RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet)
{
  PRInt32 count = mPresShells.Count();
  PRInt32 indx;
  for (indx = 0; indx < count; ++indx) {
    nsCOMPtr<nsIPresShell> shell = (nsIPresShell *)mPresShells.ElementAt(indx);
    nsCOMPtr<nsIStyleSet> set;
    shell->GetStyleSet(getter_AddRefs(set));

    if (set) {
      set->RemoveDocStyleSheet(aSheet);
    }
  }
}

void
nsDocument::RemoveStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  nsCOMPtr<nsIStyleSheet> sheet = aSheet; // hold ref so it won't die too soon

  if (!mStyleSheets.RemoveObject(aSheet)) {
    NS_NOTREACHED("stylesheet not found");
    return;
  }

  if (!mIsGoingAway) {
    PRBool applicable = PR_TRUE;
    aSheet->GetApplicable(applicable);
    if (applicable) {
      RemoveStyleSheetFromStyleSets(aSheet);
    }

    // if an observer removes itself, we're ok (not if it removes
    // others though)

    for (PRInt32 indx = mObservers.Count() - 1; indx >= 0; --indx) {
      nsIDocumentObserver *observer =
        NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(indx));

      observer->StyleSheetRemoved(this, aSheet);
    }
  }

  aSheet->SetOwningDocument(nsnull);
}

NS_IMETHODIMP
nsDocument::UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                              nsCOMArray<nsIStyleSheet>& aNewSheets)
{
  // XXX Need to set the sheet on the ownernode, if any
  NS_PRECONDITION(aOldSheets.Count() == aNewSheets.Count(),
                  "The lists must be the same length!");
  PRInt32 count = aOldSheets.Count();

  nsCOMPtr<nsIStyleSheet> oldSheet;
  PRInt32 i;
  for (i = 0; i < count; ++i) {
    oldSheet = aOldSheets[i];

    // First remove the old sheet.
    NS_ASSERTION(oldSheet, "None of the old sheets should be null");
    PRInt32 oldIndex = mStyleSheets.IndexOf(oldSheet);
    NS_ASSERTION(oldIndex != -1, "stylesheet not found");
    mStyleSheets.RemoveObjectAt(oldIndex);

    PRBool applicable = PR_TRUE;
    oldSheet->GetApplicable(applicable);
    if (applicable) {
      RemoveStyleSheetFromStyleSets(oldSheet);
    }
    // XXX we should really notify here, but right now that would
    // force things like a full reframe on every sheet.  We really
    // need a way to batch this sucker...

    oldSheet->SetOwningDocument(nsnull);

    // Now put the new one in its place.  If it's null, just ignore it.
    nsIStyleSheet* newSheet = aNewSheets[i];
    if (newSheet) {
      mStyleSheets.InsertObjectAt(newSheet, oldIndex);
      newSheet->SetOwningDocument(this);
      PRBool applicable = PR_TRUE;
      newSheet->GetApplicable(applicable);
      if (applicable) {
        AddStyleSheetToStyleSets(newSheet);
      }

      // XXX we should be notifying here too.
    }
  }

  // Now notify so _something_ happens, assuming we did anything
  if (oldSheet) {
    // if an observer removes itself, we're ok (not if it removes
    // others though)
    // XXXldb Hopefully the observer doesn't care which sheet you use.
    for (PRInt32 indx = mObservers.Count() - 1; indx >= 0; --indx) {
      nsIDocumentObserver *observer =
        NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(indx));

      observer->StyleSheetRemoved(this, oldSheet);
    }
  }

  return NS_OK;
}


void
nsDocument::InternalInsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex)
{
  // subclass hook for sheet ordering
  mStyleSheets.InsertObjectAt(aSheet, aIndex);
}

NS_IMETHODIMP
nsDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex)
{
  NS_PRECONDITION(aSheet, "null ptr");
  InternalInsertStyleSheetAt(aSheet, aIndex);

  aSheet->SetOwningDocument(this);

  PRBool applicable;
  aSheet->GetApplicable(applicable);

  if (applicable) {
    AddStyleSheetToStyleSets(aSheet);
  }

  // if an observer removes itself, we're ok (not if it removes others
  // though)
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->StyleSheetAdded(this, aSheet);
  }
  return NS_OK;
}


void
nsDocument::SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                         PRBool aApplicable)
{
  NS_PRECONDITION(aSheet, "null arg");

  // If we're actually in the document style sheet list
  if (-1 != mStyleSheets.IndexOf(aSheet)) {
    if (aApplicable) {
      AddStyleSheetToStyleSets(aSheet);
    } else {
      RemoveStyleSheetFromStyleSets(aSheet);
    }
  }

  // We have to always notify, since this will be called for sheets
  // that are children of sheets in our style set, as well as some
  // sheets for nsHTMLEditor.

  PRInt32 indx;
  // if an observer removes itself, we're ok (not if it removes others though)
  for (indx = mObservers.Count() - 1; indx >= 0; --indx) {
    nsIDocumentObserver* observer =
      (nsIDocumentObserver*)mObservers.ElementAt(indx);
    observer->StyleSheetApplicableStateChanged(this, aSheet, aApplicable);
  }
}

NS_IMETHODIMP
nsDocument::GetScriptGlobalObject(nsIScriptGlobalObject** aScriptGlobalObject)
{
   NS_ENSURE_ARG_POINTER(aScriptGlobalObject);

   // If we're going away, we've already released the reference to our
   // ScriptGlobalObject.  We can, however, try to obtain it for the
   // caller through our docshell.

   if (mIsGoingAway) {
     nsCOMPtr<nsIInterfaceRequestor> requestor =
       do_QueryReferent(mDocumentContainer);
     if (requestor)
       return CallGetInterface(requestor.get(), aScriptGlobalObject);
   }

   *aScriptGlobalObject = mScriptGlobalObject;
   NS_IF_ADDREF(*aScriptGlobalObject);

   return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetScriptGlobalObject(nsIScriptGlobalObject *aScriptGlobalObject)
{
  // XXX HACK ALERT! If the script context owner is null, the document
  // will soon be going away. So tell our content that to lose its
  // reference to the document. This has to be done before we actually
  // set the script context owner to null so that the content elements
  // can remove references to their script objects.
  if (!aScriptGlobalObject) {
    PRInt32 count, indx;

    count = mChildren.Count();

    mIsGoingAway = PR_TRUE;

    for (indx = 0; indx < count; ++indx) {
      mChildren[indx]->SetDocument(nsnull, PR_TRUE, PR_TRUE);
    }

    // Propagate the out-of-band notification to each PresShell's
    // anonymous content as well. This ensures that there aren't any
    // accidental script references left in anonymous content keeping
    // the document alive. (While not strictly necessary -- the
    // PresShell owns us -- it's tidy.)
    for (count = mPresShells.Count() - 1; count >= 0; --count) {
      nsCOMPtr<nsIPresShell> shell =
        NS_STATIC_CAST(nsIPresShell*, mPresShells[count]);
      if (!shell)
        continue;

      shell->ReleaseAnonymousContent();
    }

#ifdef DEBUG_jst
    printf ("Content wrapper hash had %d entries.\n",
            mContentWrapperHash.Count());
#endif

    mContentWrapperHash.Reset();
  }

  mScriptGlobalObject = aScriptGlobalObject;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetScriptLoader(nsIScriptLoader** aScriptLoader)
{
  NS_ENSURE_ARG_POINTER(aScriptLoader);

  if (!mScriptLoader) {
    nsScriptLoader* loader = new nsScriptLoader();
    if (!loader) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mScriptLoader = loader;
    mScriptLoader->Init(this);
  }

  *aScriptLoader = mScriptLoader;
  NS_IF_ADDREF(*aScriptLoader);

  return NS_OK;
}

// Note: We don't hold a reference to the document observer; we assume
// that it has a live reference to the document.
void
nsDocument::AddObserver(nsIDocumentObserver* aObserver)
{
  // XXX Make sure the observer isn't already in the list
  if (mObservers.IndexOf(aObserver) == -1) {
    mObservers.AppendElement(aObserver);
  }
}

PRBool
nsDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
  // If we're in the process of destroying the document (and we're
  // informing the observers of the destruction), don't remove the
  // observers from the list. This is not a big deal, since we
  // don't hold a live reference to the observers.
  if (!mInDestructor) {
    return mObservers.RemoveElement(aObserver);
  }

  return (mObservers.IndexOf(aObserver) != -1);
}

NS_IMETHODIMP
nsDocument::BeginUpdate()
{
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->BeginUpdate(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::EndUpdate()
{
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->EndUpdate(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::BeginLoad()
{
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->BeginLoad(this);
  }

  return NS_OK;
}

static void
GetDocumentFromDocShellTreeItem(nsIDocShellTreeItem *aDocShell,
                                nsIDocument **aDocument)
{
  *aDocument = nsnull;

  nsCOMPtr<nsIDOMWindow> window(do_GetInterface(aDocShell));

  if (window) {
    nsCOMPtr<nsIDOMDocument> dom_doc;
    window->GetDocument(getter_AddRefs(dom_doc));

    if (dom_doc) {
      CallQueryInterface(dom_doc, aDocument);
    }
  }
}

NS_IMETHODIMP
nsDocument::EndLoad()
{
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*)mObservers[i];
    observer->EndLoad(this);
  }

  // Fire a DOM event notifying listeners that this document has been
  // loaded (excluding images and other loads initiated by this
  // document).
  nsCOMPtr<nsIDOMEvent> event;
  CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));

  if (event) {
    event->InitEvent(NS_LITERAL_STRING("DOMContentLoaded"), PR_TRUE, PR_TRUE);
    PRBool noDefault;
    DispatchEvent(event, &noDefault);
  }

  // If this document is a [i]frame, fire a DOMFrameContentLoaded
  // event on all parent documents notifying that the HTML (excluding
  // other external files such as images and stylesheets) in a frame
  // has finished loading.

  nsCOMPtr<nsIDocShellTreeItem> docShellParent;

  // target_frame is the [i]frame element that will be used as the
  // target for the event. It's the [i]frame whose content is done
  // loading.
  nsCOMPtr<nsIDOMEventTarget> target_frame;

  if (mScriptGlobalObject) {
    nsCOMPtr<nsIDocShell> docShell;
    mScriptGlobalObject->GetDocShell(getter_AddRefs(docShell));

    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
      do_QueryInterface(docShell);

    if (docShellAsItem) {
      docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

      nsCOMPtr<nsIDocument> parent_doc;

      GetDocumentFromDocShellTreeItem(docShellParent,
                                      getter_AddRefs(parent_doc));

      if (parent_doc) {
        nsCOMPtr<nsIContent> target_content;

        parent_doc->FindContentForSubDocument(this,
                                              getter_AddRefs(target_content));

        target_frame = do_QueryInterface(target_content);
      }
    }
  }

  if (target_frame) {
    while (docShellParent) {
      nsCOMPtr<nsIDocument> ancestor_doc;

      GetDocumentFromDocShellTreeItem(docShellParent,
                                      getter_AddRefs(ancestor_doc));

      if (!ancestor_doc) {
        break;
      }

      nsCOMPtr<nsIPrivateDOMEvent> private_event;

      nsCOMPtr<nsIDOMDocumentEvent> document_event =
        do_QueryInterface(ancestor_doc);

      if (document_event) {
        document_event->CreateEvent(NS_LITERAL_STRING("Events"),
                                    getter_AddRefs(event));

        private_event = do_QueryInterface(event);
      }

      if (event && private_event) {
        event->InitEvent(NS_LITERAL_STRING("DOMFrameContentLoaded"), PR_TRUE,
                         PR_TRUE);

        private_event->SetTarget(target_frame);

        // To dispatch this event we must manually call
        // HandleDOMEvent() on the ancestor document since the target
        // is not in the same document, so the event would never reach
        // the ancestor document if we used the normal event
        // dispatching code.

        nsEvent* innerEvent;
        private_event->GetInternalNSEvent(&innerEvent);
        if (innerEvent) {
          nsEventStatus status = nsEventStatus_eIgnore;

          nsCOMPtr<nsIPresShell> shell;
          ancestor_doc->GetShellAt(0, getter_AddRefs(shell));

          if (shell) {
            nsCOMPtr<nsIPresContext> context;
            shell->GetPresContext(getter_AddRefs(context));

            if (context) {
              // The event argument to HandleDOMEvent() is inout, and
              // that doesn't mix well with nsCOMPtr's. We'll need to
              // perform some refcounting magic here.
              nsIDOMEvent *tmp_event = event;
              NS_ADDREF(tmp_event);

              ancestor_doc->HandleDOMEvent(context, innerEvent, &tmp_event,
                                           NS_EVENT_FLAG_INIT, &status);

              NS_IF_RELEASE(tmp_event);
            }
          }
        }
      }

      nsCOMPtr<nsIDocShellTreeItem> tmp(docShellParent);
      tmp->GetSameTypeParent(getter_AddRefs(docShellParent));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentChanged(nsIContent* aContent, nsISupports* aSubContent)
{
  NS_ABORT_IF_FALSE(aContent, "Null content!");

  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->ContentChanged(this, aContent, aSubContent);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentStatesChanged(nsIContent* aContent1, nsIContent* aContent2,
                                 PRInt32 aStateMask)
{
  PRInt32 i;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->ContentStatesChanged(this, aContent1, aContent2, aStateMask);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::ContentAppended(nsIContent* aContainer,
                            PRInt32 aNewIndexInContainer)
{
  NS_ABORT_IF_FALSE(aContainer, "Null container!");

#ifdef XP_OS2_VACPP
  volatile
#endif
  PRInt32 i;

  // XXXdwh There is a hacky ordering dependency between the binding
  // manager and the frame constructor that forces us to walk the
  // observer list in a forward order
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->ContentAppended(this, aContainer, aNewIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index
    if (i < mObservers.Count() &&
        observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentInserted(nsIContent* aContainer, nsIContent* aChild,
                            PRInt32 aIndexInContainer)
{
  NS_ABORT_IF_FALSE(aChild, "Null child!");

  PRInt32 i;
  // XXXdwh There is a hacky ordering dependency between the binding manager
  // and the frame constructor that forces us to walk the observer list
  // in a forward order
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    observer->ContentInserted(this, aContainer, aChild, aIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index.
    if (i < mObservers.Count() &&
        observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentReplaced(nsIContent* aContainer, nsIContent* aOldChild,
                            nsIContent* aNewChild, PRInt32 aIndexInContainer)
{
  NS_ABORT_IF_FALSE(aOldChild && aNewChild, "Null old or new child child!");

  PRInt32 i;
  // XXXdwh There is a hacky ordering dependency between the binding
  // manager and the frame constructor that forces us to walk the
  // observer list in a reverse order
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentReplaced(this, aContainer, aOldChild, aNewChild,
                              aIndexInContainer);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentRemoved(nsIContent* aContainer, nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  NS_ABORT_IF_FALSE(aChild, "Null child!");

  PRInt32 i;
  // XXXdwh There is a hacky ordering dependency between the binding
  // manager and the frame constructor that forces us to walk the
  // observer list in a reverse order
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentRemoved(this, aContainer,
                             aChild, aIndexInContainer);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AttributeWillChange(nsIContent* aChild, PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute)
{
  NS_ASSERTION(aChild, "Null child!");

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AttributeChanged(nsIContent* aChild, PRInt32 aNameSpaceID,
                             nsIAtom* aAttribute, PRInt32 aModType,
                             nsChangeHint aHint)
{
  NS_ABORT_IF_FALSE(aChild, "Null child!");

  PRInt32 i;
  nsresult rv = NS_OK;
  for (i = mObservers.Count() - 1; i >= 0; --i) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    nsresult rv2 = observer->AttributeChanged(this, aChild, aNameSpaceID,
                                              aAttribute, aModType, aHint);
    if (NS_FAILED(rv2) && NS_SUCCEEDED(rv))
      rv = rv2;
  }

  return rv;
}


NS_IMETHODIMP
nsDocument::StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule, nsChangeHint aHint)
{
  PRInt32 i;

  // Get new value of count for every iteration in case observers
  // remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    // XXXbz We should _not_ be calling BeginUpdate from in here! The
    // caller of StyleRuleChanged should do that!
    observer->BeginUpdate(this);
    observer->StyleRuleChanged(this, aStyleSheet, aStyleRule, aHint);

    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (i < mObservers.Count() &&
        observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                           nsIStyleRule* aStyleRule)
{
  PRInt32 i;

  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    // XXXbz We should _not_ be calling BeginUpdate from in here! The
    // caller of StyleRuleAdded should do that!
    observer->BeginUpdate(this);
    observer->StyleRuleAdded(this, aStyleSheet, aStyleRule);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (i < mObservers.Count() &&
        observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule)
{
  PRInt32 i;

  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver *observer =
      NS_STATIC_CAST(nsIDocumentObserver *, mObservers.ElementAt(i));

    // XXXbz We should _not_ be calling BeginUpdate from in here! The
    // caller of StyleRuleRemoved should do that!
    observer->BeginUpdate(this);
    observer->StyleRuleRemoved(this, aStyleSheet, aStyleRule);

    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (i < mObservers.Count() &&
        observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }

  return NS_OK;
}


//
// nsIDOMDocument interface
//
NS_IMETHODIMP
nsDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
  NS_ENSURE_ARG_POINTER(aDoctype);

  *aDoctype = nsnull;
  PRInt32 i, count;
  count = mChildren.Count();
  nsCOMPtr<nsIDOMNode> rootContentNode(do_QueryInterface(mRootContent) );
  nsCOMPtr<nsIDOMNode> node;

  for (i = 0; i < count; i++) {
    node = do_QueryInterface(mChildren[i]);

    NS_ASSERTION(node, "null element of mChildren");

    // doctype can't be after the root
    // XXX Do we really want to enforce this when we don't enforce
    // anything else?
    if (node == rootContentNode)
      return NS_OK;

    if (node) {
      PRUint16 nodeType;

      node->GetNodeType(&nodeType);

      if (nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) {
        return CallQueryInterface(node, aDoctype);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
  // For now, create a new implementation every time. This shouldn't
  // be a high bandwidth operation
  *aImplementation = new nsDOMImplementation(mDocumentURL);
  if (!*aImplementation) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aImplementation);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
  NS_ENSURE_ARG_POINTER(aDocumentElement);

  nsresult rv = NS_OK;

  if (mRootContent) {
    rv = CallQueryInterface(mRootContent, aDocumentElement);
    NS_ASSERTION(NS_OK == rv, "Must be a DOM Element");
  } else {
    *aDocumentElement = nsnull;
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateElement(const nsAString& aTagName,
                          nsIDOMElement** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateElementNS(const nsAString & namespaceURI,
                            const nsAString & qualifiedName,
                            nsIDOMElement **_retval)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateTextNode(const nsAString& aData, nsIDOMText** aReturn)
{
  *aReturn = nsnull;

  nsCOMPtr<nsITextContent> text;
  nsresult rv = NS_NewTextNode(getter_AddRefs(text));

  if (NS_SUCCEEDED(rv)) {
    rv = CallQueryInterface(text, aReturn);
    (*aReturn)->AppendData(aData);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
  return NS_NewDocumentFragment(aReturn, this);
}

NS_IMETHODIMP
nsDocument::CreateComment(const nsAString& aData, nsIDOMComment** aReturn)
{
  *aReturn = nsnull;

  nsCOMPtr<nsIContent> comment;
  nsresult rv = NS_NewCommentNode(getter_AddRefs(comment));

  if (NS_SUCCEEDED(rv)) {
    rv = CallQueryInterface(comment, aReturn);
    (*aReturn)->AppendData(aData);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateCDATASection(const nsAString& aData,
                               nsIDOMCDATASection** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateProcessingInstruction(const nsAString& aTarget,
                                        const nsAString& aData,
                                        nsIDOMProcessingInstruction** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateAttribute(const nsAString& aName,
                            nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  nsAutoString value;
  nsDOMAttribute* attribute;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = mNodeInfoManager->GetNodeInfo(aName, nsnull, kNameSpaceID_None,
                                              *getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  attribute = new nsDOMAttribute(nsnull, nodeInfo, value);
  NS_ENSURE_TRUE(attribute, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(attribute, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateAttributeNS(const nsAString & aNamespaceURI,
                              const nsAString & aQualifiedName,
                              nsIDOMAttr **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = mNodeInfoManager->GetNodeInfo(aQualifiedName, aNamespaceURI,
                                              *getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsDOMAttribute* attribute = new nsDOMAttribute(nsnull, nodeInfo, value);
  NS_ENSURE_TRUE(attribute, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(attribute, aResult);
}

NS_IMETHODIMP
nsDocument::CreateEntityReference(const nsAString& aName,
                                  nsIDOMEntityReference** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetElementsByTagName(const nsAString& aTagname,
                                 nsIDOMNodeList** aReturn)
{
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aTagname);
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIContentList> list;
  NS_GetContentList(this, nameAtom, kNameSpaceID_Unknown, nsnull,
                    getter_AddRefs(list));
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(list, aReturn);
}

NS_IMETHODIMP
nsDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName,
                                   nsIDOMNodeList** aReturn)
{
  PRInt32 nameSpaceId = kNameSpaceID_Unknown;

  nsCOMPtr<nsIContentList> list;

  if (!aNamespaceURI.Equals(NS_LITERAL_STRING("*"))) {
    nsContentUtils::GetNSManagerWeakRef()->GetNameSpaceID(aNamespaceURI,
                                                          nameSpaceId);

    if (nameSpaceId == kNameSpaceID_Unknown) {
      // Unknown namespace means no matches, we create an empty list...
      NS_GetContentList(this, nsnull, kNameSpaceID_None, nsnull,
                        getter_AddRefs(list));
      NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  if (!list) {
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aLocalName);
    NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

    NS_GetContentList(this, nameAtom, nameSpaceId, nsnull,
                      getter_AddRefs(list));
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);
  }

  return CallQueryInterface(list, aReturn);
}

NS_IMETHODIMP
nsDocument::GetElementById(const nsAString & elementId,
                           nsIDOMElement **_retval)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetAsync(PRBool *aAsync)
{
  NS_ERROR("nsDocument::GetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::SetAsync(PRBool aAsync)
{
  NS_ERROR("nsDocument::SetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::Load(const nsAString& aUrl, PRBool *aReturn)
{
  NS_ERROR("nsDocument::Load() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::EvaluateFIXptr(const nsAString& aExpression, nsIDOMRange **aRange)
{
  NS_ERROR("nsDocument::EvaluateFIXptr() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::EvaluateXPointer(const nsAString& aExpression,
                             nsIXPointerResult **aResult)
{
  NS_ERROR("nsDocument::EvaluateXPointer() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetStyleSheets(nsIDOMStyleSheetList** aStyleSheets)
{
  if (!mDOMStyleSheets) {
    mDOMStyleSheets = new nsDOMStyleSheetList(this);
    if (!mDOMStyleSheets) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aStyleSheets = mDOMStyleSheets;
  NS_ADDREF(*aStyleSheets);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetCharacterSet(nsAString& aCharacterSet)
{
  return GetDocumentCharacterSet(aCharacterSet);
}

NS_IMETHODIMP
nsDocument::ImportNode(nsIDOMNode* aImportedNode,
                       PRBool aDeep,
                       nsIDOMNode** aReturn)
{
  NS_ENSURE_ARG(aImportedNode);
  NS_ENSURE_ARG_POINTER(aReturn);

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aImportedNode);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return aImportedNode->CloneNode(aDeep, aReturn);
}

NS_IMETHODIMP
nsDocument::AddBinding(nsIDOMElement* aContent, const nsAString& aURL)
{
  nsresult rv = nsContentUtils::CheckSameOrigin(this, aContent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIBindingManager> bm;
  GetBindingManager(getter_AddRefs(bm));
  nsCOMPtr<nsIContent> content(do_QueryInterface(aContent));

  return bm->AddLayeredBinding(content, aURL);
}

NS_IMETHODIMP
nsDocument::RemoveBinding(nsIDOMElement* aContent, const nsAString& aURL)
{
  nsresult rv = nsContentUtils::CheckSameOrigin(this, aContent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mBindingManager) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aContent));
    return mBindingManager->RemoveLayeredBinding(content, aURL);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocument::LoadBindingDocument(const nsAString& aURL,
                                nsIDOMDocument** aResult)
{
  if (!mBindingManager) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> doc;
  mBindingManager->LoadBindingDocument(this, aURL, getter_AddRefs(doc));

  nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
  *aResult = domDoc;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBindingParent(nsIDOMNode* aNode, nsIDOMElement** aResult)
{
  *aResult = nsnull;
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> result;
  content->GetBindingParent(getter_AddRefs(result));
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(result));
  *aResult = elt;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

static nsresult
GetElementByAttribute(nsIContent* aContent, nsIAtom* aAttrName,
                      const nsAString& aAttrValue, PRBool aUniversalMatch,
                      nsIDOMElement** aResult)
{
  nsAutoString value;
  nsresult rv = aContent->GetAttr(kNameSpaceID_None, aAttrName, value);
  if (rv == NS_CONTENT_ATTR_HAS_VALUE &&
      (aUniversalMatch || value.Equals(aAttrValue))) {
    return CallQueryInterface(aContent, aResult);
  }

  PRInt32 childCount;
  aContent->ChildCount(childCount);

  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIContent> current;
    aContent->ChildAt(i, *getter_AddRefs(current));

    GetElementByAttribute(current, aAttrName, aAttrValue, aUniversalMatch,
                          aResult);

    if (*aResult)
      return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAnonymousElementByAttribute(nsIDOMElement* aElement,
                                           const nsAString& aAttrName,
                                           const nsAString& aAttrValue,
                                           nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIDOMNodeList> nodeList;
  GetAnonymousNodes(aElement, getter_AddRefs(nodeList));

  if (!nodeList)
    return NS_OK;

  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttrName);

  PRUint32 length;
  nodeList->GetLength(&length);

  PRBool universalMatch = aAttrValue.Equals(NS_LITERAL_STRING("*"));

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<nsIDOMNode> current;
    nodeList->Item(i, getter_AddRefs(current));

    nsCOMPtr<nsIContent> content(do_QueryInterface(current));

    GetElementByAttribute(content, attribute, aAttrValue, universalMatch,
                          aResult);
    if (*aResult)
      return NS_OK;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetAnonymousNodes(nsIDOMElement* aElement,
                              nsIDOMNodeList** aResult)
{
  *aResult = nsnull;
  if (!mBindingManager) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  return mBindingManager->GetAnonymousNodesFor(content, aResult);
}

NS_IMETHODIMP
nsDocument::CreateRange(nsIDOMRange** aReturn)
{
  return NS_NewRange(aReturn);
}

NS_IMETHODIMP
nsDocument::CreateNodeIterator(nsIDOMNode *aRoot,
                               PRUint32 aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               PRBool aEntityReferenceExpansion,
                               nsIDOMNodeIterator **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateTreeWalker(nsIDOMNode *aRoot,
                             PRUint32 aWhatToShow,
                             nsIDOMNodeFilter *aFilter,
                             PRBool aEntityReferenceExpansion,
                             nsIDOMTreeWalker **_retval)
{
  *_retval = nsnull;

  if (!aRoot) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aRoot);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_NewTreeWalker(aRoot, aWhatToShow, aFilter,
                          aEntityReferenceExpansion, _retval);
}


NS_IMETHODIMP
nsDocument::GetDefaultView(nsIDOMAbstractView** aDefaultView)
{
  NS_ENSURE_ARG_POINTER(aDefaultView);
  *aDefaultView = nsnull;

  NS_ENSURE_TRUE(mPresShells.Count() != 0, NS_OK);
  nsCOMPtr<nsIPresShell> shell = NS_STATIC_CAST(nsIPresShell *,
                                                mPresShells.ElementAt(0));
  NS_ENSURE_TRUE(shell, NS_OK);

  nsCOMPtr<nsIPresContext> ctx;
  nsresult rv = shell->GetPresContext(getter_AddRefs(ctx));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && ctx, rv);

  nsCOMPtr<nsISupports> container;
  rv = ctx->GetContainer(getter_AddRefs(container));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && container, rv);

  nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(container);
  NS_ENSURE_TRUE(window, NS_OK);

  CallQueryInterface(window, aDefaultView);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLocation(nsIDOMLocation **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  nsCOMPtr<nsIDOMWindowInternal> w(do_QueryInterface(mScriptGlobalObject));

  if (!w) {
    return NS_OK;
  }

  return w->GetLocation(_retval);
}

NS_IMETHODIMP
nsDocument::GetTitle(nsAString& aTitle)
{
  aTitle.Assign(mDocumentTitle);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetTitle(const nsAString& aTitle)
{
  for (PRInt32 i = mPresShells.Count() - 1; i >= 0; --i) {
    nsCOMPtr<nsIPresShell> shell =
      NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);

    nsCOMPtr<nsIPresContext> context;
    nsresult rv = shell->GetPresContext(getter_AddRefs(context));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> container;
    rv = context->GetContainer(getter_AddRefs(container));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!container)
      continue;

    nsCOMPtr<nsIBaseWindow> docShellWin = do_QueryInterface(container);
    if (!docShellWin)
      continue;

    rv = docShellWin->SetTitle(PromiseFlatString(aTitle).get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mDocumentTitle.Assign(aTitle);

  // Fire a DOM event for the title change.
  nsCOMPtr<nsIDOMEvent> event;
  CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  if (event) {
    event->InitEvent(NS_LITERAL_STRING("DOMTitleChanged"), PR_TRUE, PR_TRUE);
    PRBool noDefault;
    DispatchEvent(event, &noDefault);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult)
{
  nsresult rv;

  *aResult = nsnull;

  if (!mBoxObjectTable) {
    mBoxObjectTable = new nsSupportsHashtable;
  } else {
    nsISupportsKey key(aElement);
    nsCOMPtr<nsISupports> supports = dont_AddRef(mBoxObjectTable->Get(&key));

    nsCOMPtr<nsIBoxObject> boxObject(do_QueryInterface(supports));
    if (boxObject) {
      *aResult = boxObject;
      NS_ADDREF(*aResult);

      return NS_OK;
    }
  }

  nsCOMPtr<nsIPresShell> shell;
  GetShellAt(0, getter_AddRefs(shell));
  if (!shell)
    return NS_ERROR_FAILURE;

  PRInt32 namespaceID;
  nsCOMPtr<nsIAtom> tag;
  nsCOMPtr<nsIXBLService> xblService =
           do_GetService("@mozilla.org/xbl;1", &rv);
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  xblService->ResolveTag(content, &namespaceID, getter_AddRefs(tag));

  nsCAutoString contractID("@mozilla.org/layout/xul-boxobject");
  if (namespaceID == kNameSpaceID_XUL) {
    if (tag == nsXULAtoms::browser)
      contractID += "-browser";
    else if (tag == nsXULAtoms::editor)
      contractID += "-editor";
    else if (tag == nsXULAtoms::iframe)
      contractID += "-iframe";
    else if (tag == nsXULAtoms::menu)
      contractID += "-menu";
    else if (tag == nsXULAtoms::popup ||
             tag == nsXULAtoms::menupopup ||
             tag == nsXULAtoms::tooltip)
      contractID += "-popup";
    else if (tag == nsXULAtoms::tree)
      contractID += "-tree";
    else if (tag == nsXULAtoms::listbox)
      contractID += "-listbox";
    else if (tag == nsXULAtoms::scrollbox)
      contractID += "-scrollbox";
  }
  contractID += ";1";

  nsCOMPtr<nsIBoxObject> boxObject(do_CreateInstance(contractID.get()));
  if (!boxObject)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsPIBoxObject> privateBox(do_QueryInterface(boxObject));
  rv = privateBox->Init(content, shell);

  if (NS_FAILED(rv)) {
    return rv;
  }

  SetBoxObjectFor(aElement, boxObject);

  *aResult = boxObject;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject* aBoxObject)
{
  if (!mBoxObjectTable) {
    if (!aBoxObject)
      return NS_OK;
    mBoxObjectTable = new nsSupportsHashtable(12);
  }

  nsISupportsKey key(aElement);

  if (aBoxObject) {
    mBoxObjectTable->Put(&key, aBoxObject);
  } else {
    nsCOMPtr<nsISupports> supp;
    mBoxObjectTable->Remove(&key, getter_AddRefs(supp));
    nsCOMPtr<nsPIBoxObject> boxObject(do_QueryInterface(supp));
    if (boxObject) {
      boxObject->SetDocument(nsnull);
    }
  }

  return NS_OK;
}

struct DirTable {
  const char* mName;
  PRUint8     mValue;
};

static const DirTable dirAttributes[] = {
  {"ltr", IBMBIDI_TEXTDIRECTION_LTR},
  {"rtl", IBMBIDI_TEXTDIRECTION_RTL},
  {0}
};

/**
 * Retrieve the "direction" property of the document.
 *
 * @lina 01/09/2001
 */
NS_IMETHODIMP
nsDocument::GetDir(nsAString& aDirection)
{
  nsCOMPtr<nsIPresShell> shell = (nsIPresShell*)mPresShells.SafeElementAt(0);
  if (shell) {
    nsCOMPtr<nsIPresContext> context;
    shell->GetPresContext(getter_AddRefs(context));
    if (context) {
      PRUint32 options;
      context->GetBidi(&options);
      for (const DirTable* elt = dirAttributes; elt->mName; elt++) {
        if (GET_BIDI_OPTION_DIRECTION(options) == elt->mValue) {
          aDirection.Assign(NS_ConvertASCIItoUCS2(elt->mName));
          break;
        }
      }
    }
  }

  return NS_OK;
}

/**
 * Set the "direction" property of the document.
 *
 * @lina 01/09/2001
 */
NS_IMETHODIMP
nsDocument::SetDir(const nsAString& aDirection)
{
  nsIPresShell *shell =
    NS_STATIC_CAST(nsIPresShell *, mPresShells.SafeElementAt(0));

  if (!shell) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresContext> context;
  shell->GetPresContext(getter_AddRefs(context));
  NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

  PRUint32 options;
  context->GetBidi(&options);

  for (const DirTable* elt = dirAttributes; elt->mName; elt++) {
    if (aDirection == NS_ConvertASCIItoUCS2(elt->mName)) {
      if (GET_BIDI_OPTION_DIRECTION(options) != elt->mValue) {
        SET_BIDI_OPTION_DIRECTION(options, elt->mValue);
        context->SetBidi(options, PR_TRUE);
      }

      break;
    }
  }

  return NS_OK;
}


//
// nsIDOMNode methods
//
NS_IMETHODIMP
nsDocument::GetNodeName(nsAString& aNodeName)
{
  aNodeName.Assign(NS_LITERAL_STRING("#document"));

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNodeValue(nsAString& aNodeValue)
{
  SetDOMStringToNull(aNodeValue);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetNodeValue(const nsAString& aNodeValue)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}

NS_IMETHODIMP
nsDocument::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = nsIDOMNode::DOCUMENT_NODE;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetParentNode(nsIDOMNode** aParentNode)
{
  *aParentNode = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  if (!mChildNodes) {
    mChildNodes = new nsDocumentChildNodes(this);
    if (!mChildNodes) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return CallQueryInterface(mChildNodes.get(), aChildNodes);
}

NS_IMETHODIMP
nsDocument::HasChildNodes(PRBool* aHasChildNodes)
{
  NS_ENSURE_ARG(aHasChildNodes);

  *aHasChildNodes = (mChildren.Count() != 0);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::HasAttributes(PRBool* aHasAttributes)
{
  NS_ENSURE_ARG(aHasAttributes);

  *aHasAttributes = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
  if (mChildren.Count()) {
    return CallQueryInterface(mChildren[0], aFirstChild);
  }

  *aFirstChild = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLastChild(nsIDOMNode** aLastChild)
{
  PRInt32 count = mChildren.Count();
  if (count) {
    return CallQueryInterface(mChildren[count-1], aLastChild);
  }

  *aLastChild = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
  *aPreviousSibling = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  *aAttributes = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNamespaceURI(nsAString& aNamespaceURI)
{
  SetDOMStringToNull(aNamespaceURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPrefix(nsAString& aPrefix)
{
  SetDOMStringToNull(aPrefix);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetPrefix(const nsAString& aPrefix)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}

NS_IMETHODIMP
nsDocument::GetLocalName(nsAString& aLocalName)
{
  SetDOMStringToNull(aLocalName);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                         nsIDOMNode** aReturn)
{
  NS_ASSERTION(aNewChild, "null ptr");
  PRInt32 indx;
  PRUint16 nodeType;

  *aReturn = nsnull; // Do we need to do this?

  if (!aNewChild) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aNewChild);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // If it's a child type we can't handle (per DOM spec), or if it's an
  // element and we already have a root (our addition to DOM spec), throw
  // HIERARCHY_REQUEST_ERR.
  aNewChild->GetNodeType(&nodeType);
  if (((nodeType != COMMENT_NODE) &&
       (nodeType != TEXT_NODE) &&
       (nodeType != PROCESSING_INSTRUCTION_NODE) &&
       (nodeType != DOCUMENT_TYPE_NODE) &&
       (nodeType != ELEMENT_NODE)) ||
      ((nodeType == ELEMENT_NODE) && mRootContent)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNewChild));
  if (!content) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if (!aRefChild) {
    indx = mChildren.Count();
    mChildren.AppendObject(content);
  }
  else {
    nsCOMPtr<nsIContent> refContent(do_QueryInterface(aRefChild));

    if (!refContent) {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    indx = mChildren.IndexOf(refContent);
    if (indx != -1) {
      mChildren.InsertObjectAt(content, indx);
    } else {
      // couldn't find refChild
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }
  }

  // If we get here, we've succesfully inserted content into the
  // index-th spot in mChildren.
  if (nodeType == ELEMENT_NODE) {
    mRootContent = content;
  }

  content->SetDocument(this, PR_TRUE, PR_TRUE);

  ContentInserted(nsnull, content, indx);

  *aReturn = aNewChild;
  NS_ADDREF(aNewChild);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                         nsIDOMNode** aReturn)
{
  *aReturn = nsnull;

  NS_ENSURE_TRUE(aNewChild && aOldChild, NS_ERROR_NULL_POINTER);

  nsresult rv = NS_OK;
  PRInt32 indx;
  PRUint16 nodeType;

  rv = nsContentUtils::CheckSameOrigin(this, aNewChild);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aNewChild->GetNodeType(&nodeType);

  if ((COMMENT_NODE != nodeType) &&
      (TEXT_NODE != nodeType) &&
      (PROCESSING_INSTRUCTION_NODE != nodeType) &&
      (DOCUMENT_TYPE_NODE != nodeType) &&
      (ELEMENT_NODE != nodeType)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNewChild));
  nsCOMPtr<nsIContent> refContent(do_QueryInterface(aOldChild));
  if (!content || !refContent) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if (nodeType == ELEMENT_NODE && mRootContent &&
      mRootContent != refContent) {
    // Caller attempted to add a second element as a child.
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  indx = mChildren.IndexOf(refContent);
  if (indx == -1) {
    // The reference child is not a child of the document.
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  ContentRemoved(nsnull, refContent, indx);
  refContent->SetDocument(nsnull, PR_TRUE, PR_TRUE);

  mChildren.ReplaceObjectAt(content, indx);
  // This is OK because we checked above.
  if (nodeType == ELEMENT_NODE) {
    mRootContent = content;
  }

  content->SetDocument(this, PR_TRUE, PR_TRUE);
  ContentInserted(nsnull, content, indx);

  *aReturn = aOldChild;
  NS_ADDREF(aOldChild);

  return rv;
}

NS_IMETHODIMP
nsDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  *aReturn = nsnull; // do we need to do this?

  NS_ENSURE_TRUE(aOldChild, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContent> content(do_QueryInterface(aOldChild));
  if (!content) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  PRInt32 indx = mChildren.IndexOf(content);
  if (indx == -1) {
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  ContentRemoved(nsnull, content, indx);

  mChildren.RemoveObjectAt(indx);
  if (content == mRootContent)
    mRootContent = nsnull;

  content->SetDocument(nsnull, PR_TRUE, PR_TRUE);

  *aReturn = aOldChild;
  NS_ADDREF(aOldChild);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return InsertBefore(aNewChild, nsnull, aReturn);
}

NS_IMETHODIMP
nsDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  // XXX should be implemented by subclass
  *aReturn = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::Normalize()
{
  // XXX Not completely correct, since you can still have unnormalized
  // text nodes as immediate children of the document.
  if (mRootContent) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mRootContent));

    if (node) {
      return node->Normalize();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsSupported(const nsAString& aFeature, const nsAString& aVersion,
                        PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDocument::GetBaseURI(nsAString &aURI)
{
  aURI.Truncate();
  if (mDocumentBaseURL) {
    nsCAutoString spec;
    mDocumentBaseURL->GetSpec(spec);
    aURI = NS_ConvertUTF8toUCS2(spec);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CompareDocumentPosition(nsIDOMNode* aOther, PRUint16* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);
  NS_PRECONDITION(aReturn, "Must have an out parameter");

  if (this == aOther) {
    // If the two nodes being compared are the same node,
    // then no flags are set on the return.
    *aReturn = 0;

    return NS_OK;
  }

  PRUint16 mask = 0;

  nsCOMPtr<nsIContent> otherContent(do_QueryInterface(aOther));
  if (!otherContent) {
    PRUint16 otherNodeType = 0;
    aOther->GetNodeType(&otherNodeType);
    NS_ASSERTION(otherNodeType == nsIDOMNode::DOCUMENT_NODE ||
                 otherNodeType == nsIDOMNode::ATTRIBUTE_NODE,
                 "Hmm, this really _should_ support nsIContent...");
    if (otherNodeType == nsIDOMNode::ATTRIBUTE_NODE) {
      nsCOMPtr<nsIDOMAttr> otherAttr(do_QueryInterface(aOther));
      NS_ASSERTION(otherAttr, "Attributes really should be supporting "
                              "nsIDOMAttr you know...");

      nsCOMPtr<nsIDOMElement> otherOwnerEl;
      otherAttr->GetOwnerElement(getter_AddRefs(otherOwnerEl));
      if (otherOwnerEl) {
        // Documents have no direct relationship to attribute
        // nodes.  So we'll look at our relationship in relation
        // to its owner element, since that is also our relation
        // to the attribute.
        return CompareDocumentPosition(otherOwnerEl, aReturn);
      }
    }

    // If there is no common container node, then the order
    // is based upon order between the root container of each
    // node that is in no container. In this case, the result
    // is disconnected and implementation-dependent.
    mask |= (nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
             nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);

    *aReturn = mask;
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> otherDoc;
  otherContent->GetDocument(*getter_AddRefs(otherDoc));
  if (this == otherDoc) {
    // If the node being compared is contained by our node,
    // then it follows it.
    mask |= (nsIDOMNode::DOCUMENT_POSITION_IS_CONTAINED |
             nsIDOMNode::DOCUMENT_POSITION_FOLLOWING);
  }
  else {
    // If there is no common container node, then the order
    // is based upon order between the root container of each
    // node that is in no container. In this case, the result
    // is disconnected and implementation-dependent.
    mask |= (nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
             nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
  }

  *aReturn = mask;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsSameNode(nsIDOMNode* aOther, PRBool* aReturn)
{
  PRBool sameNode = PR_FALSE;

  if (this == aOther) {
    sameNode = PR_TRUE;
  }

  *aReturn = sameNode;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::LookupNamespacePrefix(const nsAString& aNamespaceURI,
                                  nsAString& aPrefix)
{
  aPrefix.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                               nsAString& aNamespaceURI)
{
  aNamespaceURI.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nsnull;

  return NS_OK;
}

nsresult
nsDocument::GetListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  if (mListenerManager) {
    *aInstancePtrResult = mListenerManager;
    NS_ADDREF(*aInstancePtrResult);

    return NS_OK;
  }

  nsresult rv = NS_NewEventListenerManager(getter_AddRefs(mListenerManager));
  NS_ENSURE_SUCCESS(rv, rv);

  mListenerManager->SetListenerTarget(NS_STATIC_CAST(nsIDocument *,this));

  *aInstancePtrResult = mListenerManager;
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsresult
nsDocument::HandleEvent(nsIDOMEvent *aEvent)
{
  PRBool noDefault;

  return DispatchEvent(aEvent, &noDefault);
}

NS_IMETHODIMP
nsDocument::GetSystemEventGroup(nsIDOMEventGroup **aGroup)
{
  nsCOMPtr<nsIEventListenerManager> manager;
  if (NS_SUCCEEDED(GetListenerManager(getter_AddRefs(manager))) && manager) {
    return manager->GetSystemEventGroupLM(aGroup);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsDocument::HandleDOMEvent(nsIPresContext* aPresContext, nsEvent* aEvent,
                           nsIDOMEvent** aDOMEvent, PRUint32 aFlags,
                           nsEventStatus* aEventStatus)
{
  nsresult mRet = NS_OK;
  PRBool externalDOMEvent = PR_FALSE;

  nsIDOMEvent* domEvent = nsnull;

  if (NS_EVENT_FLAG_INIT & aFlags) {
    if (aDOMEvent) {
      if (*aDOMEvent) {
        externalDOMEvent = PR_TRUE;
      }
    }
    else {
      aDOMEvent = &domEvent;
    }
    aEvent->flags |= aFlags;
    aFlags &= ~(NS_EVENT_FLAG_CANT_BUBBLE | NS_EVENT_FLAG_CANT_CANCEL);
    aFlags |= NS_EVENT_FLAG_BUBBLE | NS_EVENT_FLAG_CAPTURE;
  }

  // Capturing stage
  if (NS_EVENT_FLAG_CAPTURE & aFlags && nsnull != mScriptGlobalObject) {
    mScriptGlobalObject->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                        aFlags & NS_EVENT_CAPTURE_MASK,
                                        aEventStatus);
  }

  // Local handling stage
  // Check for null mDOMSlots or ELM, check if we're a non-bubbling
  // event in the bubbling state (bubbling state is indicated by the
  // presence of the NS_EVENT_FLAG_BUBBLE flag and not the
  // NS_EVENT_FLAG_INIT).
  if (mListenerManager &&
      !(NS_EVENT_FLAG_CANT_BUBBLE & aEvent->flags &&
        NS_EVENT_FLAG_BUBBLE & aFlags && !(NS_EVENT_FLAG_INIT & aFlags))) {
    aEvent->flags |= aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, this,
                                  aFlags, aEventStatus);
    aEvent->flags &= ~aFlags;
  }

  // Bubbling stage
  if (NS_EVENT_FLAG_BUBBLE & aFlags && nsnull != mScriptGlobalObject) {
    mScriptGlobalObject->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                        aFlags & NS_EVENT_BUBBLE_MASK,
                                        aEventStatus);
  }

  if (NS_EVENT_FLAG_INIT & aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event,
    // release here.
    if (*aDOMEvent && !externalDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (0 != rc) {
        // Okay, so someone in the DOM loop (a listener, JS object)
        // still has a ref to the DOM Event but the internal data
        // hasn't been malloc'd.  Force a copy of the data here so the
        // DOM Event is still valid.
        nsCOMPtr<nsIPrivateDOMEvent> privateEvent =
          do_QueryInterface(*aDOMEvent);
        if (privateEvent) {
          privateEvent->DuplicatePrivateData();
        }
      }
      aDOMEvent = nsnull;
    }
  }

  return mRet;
}

nsresult
nsDocument::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                  const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  GetListenerManager(getter_AddRefs(manager));
  if (manager) {
    manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsDocument::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                     const nsIID& aIID)
{
  if (!mListenerManager) {
    return NS_ERROR_FAILURE;
  }

  mListenerManager->RemoveEventListenerByIID(aListener, aIID,
                                             NS_EVENT_FLAG_BUBBLE);
  return NS_OK;
}

nsresult
nsDocument::AddEventListener(const nsAString& aType,
                             nsIDOMEventListener* aListener,
                             PRBool aUseCapture)
{
  return AddGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

nsresult
nsDocument::RemoveEventListener(const nsAString& aType,
                                nsIDOMEventListener* aListener,
                                PRBool aUseCapture)
{
  return RemoveGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsDocument::DispatchEvent(nsIDOMEvent* aEvent, PRBool *_retval)
{
  // Obtain a presentation context
  PRInt32 count = GetNumberOfShells();
  if (count == 0)
    return NS_OK;

  nsCOMPtr<nsIPresShell> shell;
  GetShellAt(0, getter_AddRefs(shell));
  if (!shell)
    return NS_ERROR_FAILURE;

  // Retrieve the context
  nsCOMPtr<nsIPresContext> presContext;
  shell->GetPresContext(getter_AddRefs(presContext));

  nsCOMPtr<nsIEventStateManager> esm;
  nsresult rv = presContext->GetEventStateManager(getter_AddRefs(esm));
  NS_ENSURE_SUCCESS(rv, rv);

  return esm->DispatchNewEvent(NS_STATIC_CAST(nsIDOMDocument *, this), aEvent,
                               _retval);
}

NS_IMETHODIMP
nsDocument::AddGroupedEventListener(const nsAString& aType,
                                    nsIDOMEventListener *aListener,
                                    PRBool aUseCapture,
                                    nsIDOMEventGroup *aEvtGrp)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  nsresult rv = GetListenerManager(getter_AddRefs(manager));
  if (NS_SUCCEEDED(rv) && manager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags, aEvtGrp);
    return NS_OK;
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::RemoveGroupedEventListener(const nsAString& aType,
                                       nsIDOMEventListener *aListener,
                                       PRBool aUseCapture,
                                       nsIDOMEventGroup *aEvtGrp)
{
  if (!mListenerManager) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  mListenerManager->RemoveEventListenerByType(aListener, aType, flags,
                                              aEvtGrp);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::CreateEvent(const nsAString& aEventType, nsIDOMEvent** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  // Obtain a presentation context

  nsCOMPtr<nsIPresShell> shell;
  GetShellAt(0, getter_AddRefs(shell));

  nsCOMPtr<nsIPresContext> presContext;

  if (shell) {
    // Retrieve the context
    shell->GetPresContext(getter_AddRefs(presContext));
  }

  nsCOMPtr<nsIEventListenerManager> manager;
  GetListenerManager(getter_AddRefs(manager));
  if (manager) {
    return manager->CreateEvent(presContext, nsnull, aEventType, aReturn);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocument::CreateEventGroup(nsIDOMEventGroup **aInstancePtrResult)
{
  nsresult rv;
  nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  *aInstancePtrResult = group;
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::FlushPendingNotifications(PRBool aFlushReflows,
                                      PRBool aUpdateViews)
{
  if (!aFlushReflows || !mScriptGlobalObject) {
    return NS_OK;
  }

  // We should be able to replace all this nsIDocShell* code with code
  // that uses mParentDocument, but mParentDocument is never set in
  // the current code!

  nsCOMPtr<nsIDocShell> docShell;
  mScriptGlobalObject->GetDocShell(getter_AddRefs(docShell));

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));

  if (docShellAsItem) {
    nsCOMPtr<nsIDocShellTreeItem> docShellParent;
    docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

    nsCOMPtr<nsIDOMWindow> win(do_GetInterface(docShellParent));

    if (win) {
      nsCOMPtr<nsIDOMDocument> dom_doc;
      win->GetDocument(getter_AddRefs(dom_doc));

      nsCOMPtr<nsIDocument> doc(do_QueryInterface(dom_doc));

      if (doc) {
        // If we have a parent we must flush the parent too to ensure
        // that our container is reflown if its size was changed.

        doc->FlushPendingNotifications(aFlushReflows, aUpdateViews);
      }
    }
  }

  PRInt32 i, count = mPresShells.Count();

  for (i = 0; i < count; i++) {
    nsCOMPtr<nsIPresShell> shell =
      NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);

    if (shell) {
      shell->FlushPendingNotifications(aUpdateViews);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAndIncrementContentID(PRInt32* aID)
{
  *aID = mNextContentID++;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBindingManager(nsIBindingManager** aResult)
{
  *aResult = mBindingManager;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetNodeInfoManager(nsINodeInfoManager*& aNodeInfoManager)
{
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  aNodeInfoManager = mNodeInfoManager;
  NS_ADDREF(aNodeInfoManager);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddReference(void *aKey, nsISupports *aReference)
{
  nsVoidKey key(aKey);

  if (mScriptGlobalObject) {
    mContentWrapperHash.Put(&key, aReference);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::RemoveReference(void *aKey, nsISupports **aOldReference)
{
  nsVoidKey key(aKey);

  mContentWrapperHash.Remove(&key, aOldReference);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetContainer(nsISupports *aContainer)
{
  mDocumentContainer = dont_AddRef(NS_GetWeakReference(aContainer));

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetContainer(nsISupports **aContainer)
{
  nsCOMPtr<nsISupports> container = do_QueryReferent(mDocumentContainer);

  *aContainer = container;
  NS_IF_ADDREF(*aContainer);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetScriptEventManager(nsIScriptEventManager **aResult)
{
  if (!mScriptEventManager) {
    mScriptEventManager = new nsScriptEventManager(this);
    // automatically AddRefs
  }

  *aResult = mScriptEventManager;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetXMLDeclaration(const nsAString& aVersion,
                              const nsAString& aEncoding,
                              const nsAString& aStandalone)
{
  if (aVersion.IsEmpty()) {
    mXMLDeclarationBits = 0;
    return NS_OK;
  }

  mXMLDeclarationBits = XML_DECLARATION_BITS_DECLARATION_EXISTS;

  if (!aEncoding.IsEmpty()) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_ENCODING_EXISTS;
  }

  if (aStandalone.Equals(NS_LITERAL_STRING("yes"))) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS |
                           XML_DECLARATION_BITS_STANDALONE_YES;
  } else if (aStandalone.Equals(NS_LITERAL_STRING("no"))) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetXMLDeclaration(nsAString& aVersion, nsAString& aEncoding,
                              nsAString& aStandalone)
{
  aVersion.Truncate();
  aEncoding.Truncate();
  aStandalone.Truncate();

  if (!(mXMLDeclarationBits & XML_DECLARATION_BITS_DECLARATION_EXISTS)) {
    return NS_OK;
  }

  // always until we start supporting 1.1 etc.
  aVersion.Assign(NS_LITERAL_STRING("1.0"));

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_ENCODING_EXISTS) {
    // This is what we have stored, not necessarily what was written
    // in the original
    GetDocumentCharacterSet(aEncoding);
  }

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_EXISTS) {
    if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_YES) {
      aStandalone.Assign(NS_LITERAL_STRING("yes"));
    } else {
      aStandalone.Assign(NS_LITERAL_STRING("no"));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsDocument::IsCaseSensitive()
{
  return PR_TRUE;
}

nsresult
nsDocument::GetRadioGroup(const nsAString& aName,
                          nsRadioGroupStruct **aRadioGroup)
{
  nsStringKey key(aName);
  nsRadioGroupStruct *radioGroup =
    NS_STATIC_CAST(nsRadioGroupStruct *, mRadioGroups.Get(&key));

  if (!radioGroup) {
    radioGroup = new nsRadioGroupStruct();
    NS_ENSURE_TRUE(radioGroup, NS_ERROR_OUT_OF_MEMORY);
    mRadioGroups.Put(&key, radioGroup);
  }

  *aRadioGroup = radioGroup;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    radioGroup->mSelectedRadioButton = aRadio;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement** aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    *aRadio = radioGroup->mSelectedRadioButton;
    NS_IF_ADDREF(*aRadio);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddToRadioGroup(const nsAString& aName,
                            nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    radioGroup->mRadioButtons.AppendElement(aRadio);
    NS_IF_ADDREF(aRadio);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::RemoveFromRadioGroup(const nsAString& aName,
                                 nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    if (radioGroup->mRadioButtons.RemoveElement(aRadio)) {
      NS_IF_RELEASE(aRadio);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::WalkRadioGroup(const nsAString& aName,
                           nsIRadioVisitor* aVisitor)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (!radioGroup) {
    return NS_OK;
  }

  PRBool stop = PR_FALSE;
  for (int i = 0; i < radioGroup->mRadioButtons.Count(); i++) {
    aVisitor->Visit(NS_STATIC_CAST(nsIFormControl *,
                                   radioGroup->mRadioButtons.ElementAt(i)),
                    &stop);
    if (stop) {
      return NS_OK;
    }
  }

  return NS_OK;
}

/**
 * Check if bidi enabled (set depending on the presence of RTL
 * characters). If enabled, we should apply the Unicode Bidi Algorithm
 *
 * @lina 07/12/2000
 */
NS_IMETHODIMP
nsDocument::GetBidiEnabled(PRBool* aBidiEnabled) const
{
  NS_ENSURE_ARG_POINTER(aBidiEnabled);
  *aBidiEnabled = mBidiEnabled;

  return NS_OK;
}

/**
 * Indicate the document contains RTL characters.
 *
 * @lina 07/12/2000
 */
NS_IMETHODIMP
nsDocument::SetBidiEnabled(PRBool aBidiEnabled)
{
  mBidiEnabled = aBidiEnabled;

  return NS_OK;
}

void
nsDocument::RetrieveRelevantHeaders(nsIChannel *aChannel)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  PRBool have_contentLanguage = PR_FALSE;
  PRTime modDate = LL_ZERO;
  nsresult rv;

  if (httpChannel) {
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"),
                                        mLastModified);

    if (NS_FAILED(rv)) {
      mLastModified.Truncate();
    }

    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Language"),
                                        mContentLanguage);

    if (NS_SUCCEEDED(rv)) {
      have_contentLanguage = PR_TRUE;
    }
  } else {
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(aChannel);
    if (fileChannel) {
      nsCOMPtr<nsIFile> file;
      fileChannel->GetFile(getter_AddRefs(file));
      if (file) {
        PRTime msecs;
        rv = file->GetLastModifiedTime(&msecs);

        if (NS_SUCCEEDED(rv)) {
          PRInt64 intermediateValue;
          LL_I2L(intermediateValue, PR_USEC_PER_MSEC);
          LL_MUL(modDate, msecs, intermediateValue);
        }
      }
    }
  }

  if (mLastModified.IsEmpty()) {
    modDate = PR_Now();
  }

  if (LL_NE(modDate, LL_ZERO)) {
    PRExplodedTime prtime;
    char buf[100];

    PR_ExplodeTime(modDate, PR_LocalTimeParameters, &prtime);

    // Use '%#c' for windows, because '%c' is backward-compatible and
    // non-y2k with msvc; '%#c' requests that a full year be used in the
    // result string.  Other OSes just use "%c".
    PR_FormatTime(buf, sizeof buf,
#ifdef XP_WIN
                  "%#c",
#else
                  "%c",
#endif
                  &prtime);
    mLastModified.Assign(buf);
  }

  if (!have_contentLanguage) {
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID);

    if (prefBranch) {
      prefBranch->GetCharPref("intl.accept_languages",
                              getter_Copies(mContentLanguage));
    }
  }
}
