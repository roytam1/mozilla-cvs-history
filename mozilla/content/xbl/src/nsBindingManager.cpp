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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsIXBLService.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "nsHTMLAtoms.h"
#include "nsSupportsArray.h"
#include "nsITextContent.h"
#include "nsIStreamListener.h"
#include "nsIStyleRuleSupplier.h"
#include "nsIDocumentObserver.h"

#include "nsIXBLBinding.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIXBLBindingAttachedHandler.h"
#include "nsIXBLInsertionPoint.h"

#include "nsIStyleSheet.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsIHTMLContentContainer.h"

#include "nsIStyleRuleProcessor.h"
#include "nsIStyleSet.h"
#include "nsIXBLPrototypeBinding.h"
#include "nsIWeakReference.h"

#include "nsIXPConnect.h"

// Static IIDs/CIDs. Try to minimize these.
static NS_DEFINE_CID(kNameSpaceManagerCID,        NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kXMLDocumentCID,             NS_XMLDOCUMENT_CID);
static NS_DEFINE_CID(kParserCID,                  NS_PARSER_IID); // XXX What's up with this???

class nsXBLDocumentInfo : public nsIXBLDocumentInfo, public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  
  nsXBLDocumentInfo(const char* aDocURI, nsIDocument* aDocument);
  virtual ~nsXBLDocumentInfo();
  
  NS_IMETHOD GetDocument(nsIDocument** aResult) { *aResult = mDocument; NS_IF_ADDREF(*aResult); return NS_OK; };
  NS_IMETHOD GetRuleProcessors(nsISupportsArray** aResult);
  
  NS_IMETHOD GetScriptAccess(PRBool* aResult) { *aResult = mScriptAccess; return NS_OK; };
  NS_IMETHOD SetScriptAccess(PRBool aAccess) { mScriptAccess = aAccess; return NS_OK; };

  NS_IMETHOD GetDocumentURI(nsCString& aDocURI) { aDocURI = mDocURI; return NS_OK; };

  NS_IMETHOD GetPrototypeBinding(const nsAReadableCString& aRef, nsIXBLPrototypeBinding** aResult);
  NS_IMETHOD SetPrototypeBinding(const nsAReadableCString& aRef, nsIXBLPrototypeBinding* aBinding);

private:
  nsCOMPtr<nsIDocument> mDocument;
  nsCString mDocURI;
  nsCOMPtr<nsISupportsArray> mRuleProcessors;
  PRBool mScriptAccess;
  nsSupportsHashtable* mBindingTable;
};

/* Implementation file */
NS_IMPL_ISUPPORTS2(nsXBLDocumentInfo, nsIXBLDocumentInfo, nsISupportsWeakReference)

nsXBLDocumentInfo::nsXBLDocumentInfo(const char* aDocURI, nsIDocument* aDocument)
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
  mDocURI = aDocURI;
  mDocument = aDocument;
  mScriptAccess = PR_TRUE;
  mBindingTable = nsnull;
}

nsXBLDocumentInfo::~nsXBLDocumentInfo()
{
  /* destructor code */
  delete mBindingTable;
}

NS_IMETHODIMP
nsXBLDocumentInfo::GetRuleProcessors(nsISupportsArray** aResult)
{
  if (!mRuleProcessors) {
    // Gather the rule processors.
    PRInt32 count = mDocument->GetNumberOfStyleSheets();
    if (count > 2) {
      nsCOMPtr<nsIHTMLContentContainer> container(do_QueryInterface(mDocument));
      nsCOMPtr<nsIHTMLCSSStyleSheet> inlineSheet;
      container->GetInlineStyleSheet(getter_AddRefs(inlineSheet));
      nsCOMPtr<nsIHTMLStyleSheet> attrSheet;
      container->GetAttributeStyleSheet(getter_AddRefs(attrSheet));
      nsCOMPtr<nsIStyleSheet> inlineCSS(do_QueryInterface(inlineSheet));
      nsCOMPtr<nsIStyleSheet> attrCSS(do_QueryInterface(attrSheet));
      NS_NewISupportsArray(getter_AddRefs(mRuleProcessors));
      nsCOMPtr<nsIStyleRuleProcessor> prevProcessor;
      for (PRInt32 i = 0; i < count; i++) {
        nsCOMPtr<nsIStyleSheet> sheet = getter_AddRefs(mDocument->GetStyleSheetAt(i));
        if (sheet == inlineCSS || sheet == attrCSS)
          continue;

        nsCOMPtr<nsIStyleRuleProcessor> processor;
        sheet->GetStyleRuleProcessor(*getter_AddRefs(processor), prevProcessor);
        if (processor != prevProcessor) {
          mRuleProcessors->AppendElement(processor);
          prevProcessor = processor;
        }
      }
    }
  }
  
  *aResult = mRuleProcessors;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXBLDocumentInfo::GetPrototypeBinding(const nsAReadableCString& aRef, nsIXBLPrototypeBinding** aResult)
{
  *aResult = nsnull;
  if (!mBindingTable)
    return NS_OK;

  const char* str = nsPromiseFlatCString(aRef);
  nsCStringKey key(str);
  *aResult = NS_STATIC_CAST(nsIXBLPrototypeBinding*, mBindingTable->Get(&key)); // Addref happens here.

  return NS_OK;
}

NS_IMETHODIMP
nsXBLDocumentInfo::SetPrototypeBinding(const nsAReadableCString& aRef, nsIXBLPrototypeBinding* aBinding)
{
  if (!mBindingTable)
    mBindingTable = new nsSupportsHashtable();

  const char* str = nsPromiseFlatCString(aRef);
  nsCStringKey key(str);
  mBindingTable->Put(&key, aBinding);

  return NS_OK;
}

nsresult NS_NewXBLDocumentInfo(nsIDocument* aDocument, nsIXBLDocumentInfo** aResult)
{
  nsCOMPtr<nsIURI> url = getter_AddRefs(aDocument->GetDocumentURL());
  
  nsXPIDLCString str;
  url->GetSpec(getter_Copies(str));

  *aResult = new nsXBLDocumentInfo((const char*)str, aDocument);
  
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

// ==================================================================
// = nsAnonymousContentList 
// ==================================================================

// {CC949466-626E-4e3b-88EE-967DC5CEF7BF}
#define NS_IANONYMOUSCONTENTLIST_IID \
{ 0xcc949466, 0x626e, 0x4e3b, { 0x88, 0xee, 0x96, 0x7d, 0xc5, 0xce, 0xf7, 0xbf } }

class nsIAnonymousContentList : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IANONYMOUSCONTENTLIST_IID; return iid; }

   // nsIAnonymousContentList
  NS_IMETHOD GetInsertionPointCount(PRUint32* aCount)=0;
  NS_IMETHOD GetInsertionPointAt(PRUint32 i, nsIXBLInsertionPoint** aResult)=0;
};

class nsAnonymousContentList : public nsGenericDOMNodeList, public nsIAnonymousContentList
{
public:
  nsAnonymousContentList(nsISupportsArray* aElements);
  virtual ~nsAnonymousContentList();

  NS_DECL_ISUPPORTS_INHERITED

  // nsIDOMNodeList interface
  NS_DECL_IDOMNODELIST

  // nsIAnonymousContentList
  NS_IMETHOD GetInsertionPointCount(PRUint32* aCount);
  NS_IMETHOD GetInsertionPointAt(PRUint32 i, nsIXBLInsertionPoint** aResult);
  
private:
  nsISupportsArray* mElements;
};

MOZ_DECL_CTOR_COUNTER(nsAnonymousContentList);

NS_IMPL_ISUPPORTS_INHERITED(nsAnonymousContentList, nsGenericDOMNodeList, nsIAnonymousContentList)

nsAnonymousContentList::nsAnonymousContentList(nsISupportsArray* aElements)
{
  MOZ_COUNT_CTOR(nsAnonymousContentList);

  // We don't reference count our Anonymous reference (to avoid circular
  // references). We'll be told when the Anonymous goes away.
  mElements = aElements;
  NS_IF_ADDREF(mElements);
}
 
nsAnonymousContentList::~nsAnonymousContentList()
{
  MOZ_COUNT_DTOR(nsAnonymousContentList);
  NS_IF_RELEASE(mElements);
}

NS_IMETHODIMP
nsAnonymousContentList::GetLength(PRUint32* aLength)
{
  NS_ASSERTION(aLength != nsnull, "null ptr");
  if (! aLength)
      return NS_ERROR_NULL_POINTER;

  PRUint32 cnt;
  mElements->Count(&cnt);

  *aLength = 0;
  nsCOMPtr<nsIXBLInsertionPoint> point;
  PRUint32 l;
  for (PRUint32 i = 0; i < cnt; i++) {
    point = getter_AddRefs((nsIXBLInsertionPoint*)(mElements->ElementAt(i)));
    point->ChildCount(&l);
    *aLength += l;
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsAnonymousContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  PRUint32 cnt;
  nsresult rv = mElements->Count(&cnt);
  if (NS_FAILED(rv)) return rv;

  PRUint32 pointCount = 0;
  nsCOMPtr<nsIXBLInsertionPoint> point;
  for (PRUint32 i = 0; i < cnt; i++) {
    aIndex -= pointCount;
    
    point = getter_AddRefs((nsIXBLInsertionPoint*)(mElements->ElementAt(i)));
    point->ChildCount(&pointCount);

    if (aIndex < pointCount) {
      nsCOMPtr<nsIContent> result;
      rv = point->ChildAt(aIndex, getter_AddRefs(result));
      if (result && NS_SUCCEEDED(rv))
        return result->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)aReturn);
      else return rv;
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsAnonymousContentList::GetInsertionPointCount(PRUint32* aCount)
{
  *aCount = 0;
  if (mElements)
    mElements->Count(aCount);
  return NS_OK;
}

NS_IMETHODIMP
nsAnonymousContentList::GetInsertionPointAt(PRUint32 i, nsIXBLInsertionPoint** aResult)
{
  *aResult = nsnull;
  if (mElements)
    *aResult = (nsIXBLInsertionPoint*)mElements->ElementAt(i);
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////

class nsBindingManager : public nsIBindingManager, public nsIStyleRuleSupplier, public nsIDocumentObserver
{
  NS_DECL_ISUPPORTS

public:
  nsBindingManager();
  virtual ~nsBindingManager();

  NS_IMETHOD GetBinding(nsIContent* aContent, nsIXBLBinding** aResult);
  NS_IMETHOD SetBinding(nsIContent* aContent, nsIXBLBinding* aBinding);

  NS_IMETHOD GetInsertionParent(nsIContent* aContent, nsIContent** aResult);
  NS_IMETHOD SetInsertionParent(nsIContent* aContent, nsIContent* aResult);

  NS_IMETHOD GetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS** aResult);
  NS_IMETHOD SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aResult);

  NS_IMETHOD ChangeDocumentFor(nsIContent* aContent, nsIDocument* aOldDocument,
                               nsIDocument* aNewDocument);

  NS_IMETHOD ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult);

  NS_IMETHOD GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult);
  NS_IMETHOD SetContentListFor(nsIContent* aContent, nsISupportsArray* aList);

  NS_IMETHOD GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);
  NS_IMETHOD SetAnonymousNodesFor(nsIContent* aContent, nsISupportsArray* aList);

  NS_IMETHOD GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult);

  NS_IMETHOD GetInsertionPoint(nsIContent* aParent, nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex);
  NS_IMETHOD GetSingleInsertionPoint(nsIContent* aParent, nsIContent** aResult, PRUint32* aIndex,  
                                     PRBool* aMultipleInsertionPoints);

  NS_IMETHOD AddLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL);
  NS_IMETHOD RemoveLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL);
  NS_IMETHOD LoadBindingDocument(nsIDocument* aBoundDoc, const nsAReadableString& aURL,
                                 nsIDocument** aResult);

  NS_IMETHOD AddToAttachedQueue(nsIXBLBinding* aBinding);
  NS_IMETHOD AddHandlerToAttachedQueue(nsIXBLBindingAttachedHandler* aHandler);
  NS_IMETHOD ClearAttachedQueue();
  NS_IMETHOD ProcessAttachedQueue();

  NS_IMETHOD ExecuteDetachedHandlers();

  NS_IMETHOD PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo);
  NS_IMETHOD GetXBLDocumentInfo(const nsCString& aURL, nsIXBLDocumentInfo** aResult);

  NS_IMETHOD PutLoadingDocListener(const nsCString& aURL, nsIStreamListener* aListener);
  NS_IMETHOD GetLoadingDocListener(const nsCString& aURL, nsIStreamListener** aResult);
  NS_IMETHOD RemoveLoadingDocListener(const nsCString& aURL);

  NS_IMETHOD InheritsStyle(nsIContent* aContent, PRBool* aResult);
  NS_IMETHOD FlushChromeBindings();

  NS_IMETHOD GetBindingImplementation(nsIContent* aContent, void* aScriptObject, REFNSIID aIID, void** aResult);

  NS_IMETHOD ShouldBuildChildFrames(nsIContent* aContent, PRBool* aResult);

  // nsIStyleRuleSupplier
  NS_IMETHOD UseDocumentRules(nsIContent* aContent, PRBool* aResult);
  NS_IMETHOD WalkRules(nsIStyleSet* aStyleSet, 
                       nsISupportsArrayEnumFunc aFunc, void* aData,
                       nsIContent* aContent);

  // nsIDocumentObserver
  NS_IMETHOD BeginUpdate(nsIDocument* aDocument) { return NS_OK; }
  NS_IMETHOD EndUpdate(nsIDocument* aDocument) { return NS_OK; }
  NS_IMETHOD BeginLoad(nsIDocument* aDocument) { return NS_OK; }
  NS_IMETHOD EndLoad(nsIDocument* aDocument) { return NS_OK; }
  NS_IMETHOD BeginReflow(nsIDocument* aDocument,
			                   nsIPresShell* aShell) { return NS_OK; }
  NS_IMETHOD EndReflow(nsIDocument* aDocument,
		                   nsIPresShell* aShell) { return NS_OK; } 
  NS_IMETHOD ContentChanged(nsIDocument* aDoc, 
                            nsIContent* aContent,
                            nsISupports* aSubContent) { return NS_OK; }
  NS_IMETHOD ContentStatesChanged(nsIDocument* aDocument,
                                  nsIContent* aContent1,
                                  nsIContent* aContent2) { return NS_OK; }
  NS_IMETHOD AttributeChanged(nsIDocument* aDocument,
                              nsIContent*  aContent,
                              PRInt32      aNameSpaceID,
                              nsIAtom*     aAttribute,
                              PRInt32      aHint) { return NS_OK; }
  NS_IMETHOD ContentAppended(nsIDocument* aDocument,
			                       nsIContent* aContainer,
                             PRInt32     aNewIndexInContainer);
  NS_IMETHOD ContentInserted(nsIDocument* aDocument,
			                       nsIContent* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer);
  NS_IMETHOD ContentReplaced(nsIDocument* aDocument,
			                       nsIContent* aContainer,
                             nsIContent* aOldChild,
                             nsIContent* aNewChild,
                             PRInt32 aIndexInContainer) { return NS_OK; }
  NS_IMETHOD ContentRemoved(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer);
  NS_IMETHOD StyleSheetAdded(nsIDocument* aDocument,
                             nsIStyleSheet* aStyleSheet) { return NS_OK; }
  NS_IMETHOD StyleSheetRemoved(nsIDocument* aDocument,
                               nsIStyleSheet* aStyleSheet) { return NS_OK; }
  NS_IMETHOD StyleSheetDisabledStateChanged(nsIDocument* aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            PRBool aDisabled) { return NS_OK; }
  NS_IMETHOD StyleRuleChanged(nsIDocument* aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint) { return NS_OK; }
  NS_IMETHOD StyleRuleAdded(nsIDocument* aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule) { return NS_OK; }
  NS_IMETHOD StyleRuleRemoved(nsIDocument* aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) { return NS_OK; }
  NS_IMETHOD DocumentWillBeDestroyed(nsIDocument* aDocument) { return NS_OK; }

protected:
  void GetEnclosingScope(nsIContent* aContent, nsIContent** aParent);
  void GetOutermostStyleScope(nsIContent* aContent, nsIContent** aParent);

  void WalkRules(nsISupportsArrayEnumFunc aFunc, void* aData,
                 nsIContent* aParent, nsIContent* aCurrContent);

  nsresult GetNestedInsertionPoint(nsIContent* aParent, nsIContent* aChild, nsIContent** aResult);

// MEMBER VARIABLES
protected: 
  // A mapping from nsIContent* to the nsIXBLBinding* that is installed on that element.
  nsSupportsHashtable* mBindingTable;

  // A mapping from nsIContent* to an nsIDOMNodeList* (nsAnonymousContentList*).  
  // This list contains an accurate reflection of our *explicit* children (once intermingled with
  // insertion points) in the altered DOM.
  nsSupportsHashtable* mContentListTable;

  // A mapping from nsIContent* to an nsIDOMNodeList* (nsAnonymousContentList*).  
  // This list contains an accurate reflection of our *anonymous* children (if and only if they are
  // intermingled with insertion points) in the altered DOM.  This table is not used
  // if no insertion points were defined directly underneath a <content> tag in a
  // binding.  The NodeList from the <content> is used instead as a performance
  // optimization.
  nsSupportsHashtable* mAnonymousNodesTable;

  // A mapping from nsIContent* to nsIContent*.  The insertion parent is our one true
  // parent in the transformed DOM.  This gives us a more-or-less O(1) way of obtaining
  // our transformed parent.
  nsSupportsHashtable* mInsertionParentTable;

  // A mapping from nsIContent* to nsIXPWrappedJS* (an XPConnect wrapper for JS objects).
  // For XBL bindings that implement XPIDL interfaces, and that get referred to from C++,
  // this table caches the XPConnect wrapper for the binding.  By caching it, I control
  // its lifetime, and I prevent a re-wrap of the same script object (in the case where
  // multiple bindings in an XBL inheritance chain both implement an XPIDL interface).
  nsSupportsHashtable* mWrapperTable;

  // A mapping from nsIDocument* to nsIXBLDocumentInfo*.  This table is the cache of
  // all binding documents that have been loaded by a given bound document.
  nsSupportsHashtable* mDocumentTable;

  // The currently loading binding docs.  If they're in this table, they have not yet
  // finished loading.
  nsSupportsHashtable* mLoadingDocTable;

  // A queue of binding attached event handlers that are awaiting execution.
  nsCOMPtr<nsISupportsArray> mAttachedQueue;
};

// Implementation /////////////////////////////////////////////////////////////////

// Static member variable initialization

// Implement our nsISupports methods
NS_IMPL_ISUPPORTS3(nsBindingManager, nsIBindingManager, nsIStyleRuleSupplier, nsIDocumentObserver)

// Constructors/Destructors
nsBindingManager::nsBindingManager(void)
{
  NS_INIT_REFCNT();

  mBindingTable = nsnull;
  mContentListTable = nsnull;
  mAnonymousNodesTable = nsnull;
  mInsertionParentTable = nsnull;
  mWrapperTable = nsnull;
  mDocumentTable = nsnull;
  mLoadingDocTable = nsnull;

  mAttachedQueue = nsnull;
}

nsBindingManager::~nsBindingManager(void)
{
  delete mBindingTable;
  delete mContentListTable;
  delete mAnonymousNodesTable;
  delete mInsertionParentTable;
  delete mWrapperTable;
  delete mDocumentTable;
  delete mLoadingDocTable;
}

NS_IMETHODIMP
nsBindingManager::GetBinding(nsIContent* aContent, nsIXBLBinding** aResult) 
{ 
  if (mBindingTable) {
    nsISupportsKey key(aContent);
    *aResult = NS_STATIC_CAST(nsIXBLBinding*, mBindingTable->Get(&key));
  }
  else {
    *aResult = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::SetBinding(nsIContent* aContent, nsIXBLBinding* aBinding)
{
  if (!mBindingTable)
    mBindingTable = new nsSupportsHashtable;

  nsISupportsKey key(aContent);

  nsCOMPtr<nsISupports> old = getter_AddRefs(mBindingTable->Get(&key));
  if (old && aBinding)
    NS_ERROR("Binding already installed!");

  if (aBinding) {
    mBindingTable->Put(&key, aBinding);
  }
  else {
    mBindingTable->Remove(&key);

    // The death of the bindings means the death of the JS wrapper, and the flushing
    // of our explicit and anonymous insertion point lists.
    SetWrappedJS(aContent, nsnull);
    SetContentListFor(aContent, nsnull);
    SetAnonymousNodesFor(aContent, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetInsertionParent(nsIContent* aContent, nsIContent** aResult) 
{ 
  if (mInsertionParentTable) {
    nsISupportsKey key(aContent);
    *aResult = NS_STATIC_CAST(nsIContent*, mInsertionParentTable->Get(&key));
  }
  else {
    *aResult = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::SetInsertionParent(nsIContent* aContent, nsIContent* aParent)
{
  if (!mInsertionParentTable)
    mInsertionParentTable = new nsSupportsHashtable;

  nsISupportsKey key(aContent);
  if (aParent) {
    mInsertionParentTable->Put(&key, aParent);
  }
  else
    mInsertionParentTable->Remove(&key);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS** aResult) 
{ 
  if (mWrapperTable) {
    nsISupportsKey key(aContent);
    *aResult = NS_STATIC_CAST(nsIXPConnectWrappedJS*, mWrapperTable->Get(&key));
  }
  else {
    *aResult = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aWrappedJS)
{
  if (!mWrapperTable)
    mWrapperTable = new nsSupportsHashtable;

  nsISupportsKey key(aContent);
  if (aWrappedJS) {
    mWrapperTable->Put(&key, aWrappedJS);
  }
  else
    mWrapperTable->Remove(&key);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ChangeDocumentFor(nsIContent* aContent, nsIDocument* aOldDocument,
                                    nsIDocument* aNewDocument)
{
  NS_PRECONDITION(aOldDocument != nsnull, "no old document");
  if (! aOldDocument)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aContent, getter_AddRefs(binding));
  if (binding) {
    binding->ChangeDocument(aOldDocument, aNewDocument);
    SetBinding(aContent, nsnull);
    if (aNewDocument) {
      nsCOMPtr<nsIBindingManager> otherManager;
      aNewDocument->GetBindingManager(getter_AddRefs(otherManager));
      otherManager->SetBinding(aContent, binding);
    }
  }

  // Clear out insertion parents and content lists.
  SetInsertionParent(aContent, nsnull);
  SetContentListFor(aContent, nsnull);
  SetAnonymousNodesFor(aContent, nsnull);

  for (PRInt32 i = aOldDocument->GetNumberOfShells() - 1; i >= 0; --i) {
    nsCOMPtr<nsIPresShell> shell = dont_AddRef( aOldDocument->GetShellAt(i) );
    NS_ASSERTION(shell != nsnull, "Zoiks! nsIPresShell::ShellAt() broke");

    // See if the element has nsIAnonymousContentCreator-created
    // anonymous content...
    nsCOMPtr<nsISupportsArray> anonymousElements;
    shell->GetAnonymousContentFor(aContent, getter_AddRefs(anonymousElements));

    if (anonymousElements) {
      // ...yep, so be sure to update the doc pointer in those
      // elements, too.
      PRUint32 count;
      anonymousElements->Count(&count);

      while (PRInt32(--count) >= 0) {
        nsCOMPtr<nsISupports> isupports( getter_AddRefs(anonymousElements->ElementAt(count)) );
        nsCOMPtr<nsIContent> content( do_QueryInterface(isupports) );
        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
          continue;

        content->SetDocument(aNewDocument, PR_TRUE, PR_TRUE);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID, nsIAtom** aResult)
{
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aContent, getter_AddRefs(binding));
  
  if (binding) {
    nsCOMPtr<nsIAtom> tag;
    binding->GetBaseTag(aNameSpaceID, getter_AddRefs(tag));
    if (tag) {
      *aResult = tag;
      NS_ADDREF(*aResult);
      return NS_OK;
    }
  }

  aContent->GetNameSpaceID(*aNameSpaceID);
  return aContent->GetTag(*aResult);
}

NS_IMETHODIMP
nsBindingManager::GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{ 
  // Locate the primary binding and get its node list of anonymous children.
  *aResult = nsnull;
  
  if (mContentListTable) {
    nsISupportsKey key(aContent);
    *aResult = NS_STATIC_CAST(nsIDOMNodeList*, mContentListTable->Get(&key));
  }
  
  if (!*aResult) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aContent));
    return node->GetChildNodes(aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::SetContentListFor(nsIContent* aContent, nsISupportsArray* aList)
{ 
  if (!mContentListTable) {
    if (!aList) 
      return NS_OK;
    mContentListTable = new nsSupportsHashtable;
  }

  nsISupportsKey key(aContent);
  if (aList) {
    nsAnonymousContentList* contentList = new nsAnonymousContentList(aList);
    mContentListTable->Put(&key, (nsIDOMNodeList*)contentList);
  }
  else
    mContentListTable->Remove(&key);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{ 
  // Locate the primary binding and get its node list of anonymous children.
  *aResult = nsnull;
  if (mAnonymousNodesTable) {
    nsISupportsKey key(aContent);
    *aResult = NS_STATIC_CAST(nsIDOMNodeList*, mAnonymousNodesTable->Get(&key));
  }

  if (!*aResult) {
    nsCOMPtr<nsIXBLBinding> binding;
    GetBinding(aContent, getter_AddRefs(binding));
    if (binding)
      return binding->GetAnonymousNodes(aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::SetAnonymousNodesFor(nsIContent* aContent, nsISupportsArray* aList)
{ 
  if (!mAnonymousNodesTable) {
    if (!aList) 
      return NS_OK;
    mAnonymousNodesTable = new nsSupportsHashtable;
  }

  nsISupportsKey key(aContent);
  if (aList) {
    nsAnonymousContentList* contentList = new nsAnonymousContentList(aList);
    mAnonymousNodesTable->Put(&key, (nsIDOMNodeList*)contentList);
  }
  else
    mAnonymousNodesTable->Remove(&key);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  *aResult = nsnull;

  PRUint32 length;

  // Retrieve the anonymous content that we should build.
  GetAnonymousNodesFor(aContent, aResult);
  if (*aResult) {
    (*aResult)->GetLength(&length);
    if (length == 0)
      *aResult = nsnull;
  }
    
  // We may have an altered list of children from XBL insertion points.
  // If we don't have any anonymous kids, we next check to see if we have 
  // insertion points.
  if (! *aResult) {
    if (mContentListTable) {
      nsISupportsKey key(aContent);
      *aResult = NS_STATIC_CAST(nsIDOMNodeList*, mContentListTable->Get(&key));
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetInsertionPoint(nsIContent* aParent, nsIContent* aChild, nsIContent** aResult, PRUint32* aIndex)
{
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aParent, getter_AddRefs(binding));
  
  if (binding)
    return binding->GetInsertionPoint(aChild, aResult, aIndex);
  
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetSingleInsertionPoint(nsIContent* aParent, nsIContent** aResult, PRUint32* aIndex,
                                          PRBool* aMultipleInsertionPoints)
{
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aParent, getter_AddRefs(binding));
  
  if (binding)
    return binding->GetSingleInsertionPoint(aResult, aIndex, aMultipleInsertionPoints);
  
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::AddLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL)
{
  // First we need to load our binding.
  nsresult rv;
  NS_WITH_SERVICE(nsIXBLService, xblService, "@mozilla.org/xbl;1", &rv);
  if (!xblService)
    return rv;

  // Load the bindings.
  nsCOMPtr<nsIXBLBinding> binding;
  PRBool dummy;
  xblService->LoadBindings(aContent, aURL, PR_TRUE, getter_AddRefs(binding), &dummy);
  if (binding) {
    AddToAttachedQueue(binding);
    ProcessAttachedQueue();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::RemoveLayeredBinding(nsIContent* aContent, const nsAReadableString& aURL)
{
  /*
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aParent, getter_AddRefs(binding));
  
  nsCOMPtr<nsIXBLBinding> prevBinding;
    
  while (binding) {
    nsCOMPtr<nsIXBLBinding> nextBinding;
    binding->GetBaseBinding(getter_AddRefs(nextBinding));

    PRBool style;
    binding->IsStyleBinding(&style);
    if (!style) {
       // Remove only our binding.
      if (prevBinding) {
        prevBinding->SetBaseBinding(nextBinding);

        // XXX Unhooking the binding should kill event handlers and
        // fix up the prototype chain.
        // e.g., binding->UnhookEventHandlers(); 
        //       binding->FixupPrototypeChain();
        // or maybe just binding->Unhook();

      }
      else SetBinding(aContent, nextBinding);
    }

    prevBinding = binding;
    binding = nextBinding;
  }
*/
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::LoadBindingDocument(nsIDocument* aBoundDoc, const nsAReadableString& aURL,
                                      nsIDocument** aResult)
{
  nsCAutoString url; url.AssignWithConversion((const PRUnichar*)nsPromiseFlatString(aURL).get());
  
  nsCOMPtr<nsIURL> uri;
  nsComponentManager::CreateInstance("@mozilla.org/network/standard-url;1",
                                     nsnull,
                                     NS_GET_IID(nsIURL),
                                     getter_AddRefs(uri));
  uri->SetSpec(url);
  

  nsCOMPtr<nsIURI> docURL = getter_AddRefs(aBoundDoc->GetDocumentURL());
  nsXPIDLCString scheme;
  docURL->GetScheme(getter_Copies(scheme));

  nsXPIDLCString otherScheme;
  uri->GetScheme(getter_Copies(otherScheme));

  // First we need to load our binding.
  *aResult = nsnull;
  nsresult rv;
  NS_WITH_SERVICE(nsIXBLService, xblService, "@mozilla.org/xbl;1", &rv);
  if (!xblService)
    return rv;

  // Load the binding doc.
  nsCOMPtr<nsIXBLDocumentInfo> info;
  xblService->LoadBindingDocumentInfo(nsnull, aBoundDoc, url, nsCAutoString(), PR_TRUE, getter_AddRefs(info));
  if (!info)
    return NS_ERROR_FAILURE;

  if (!PL_strcmp(scheme, otherScheme))
    info->GetDocument(aResult); // Addref happens here.
    
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::AddToAttachedQueue(nsIXBLBinding* aBinding)
{
  if (!mAttachedQueue)
    NS_NewISupportsArray(getter_AddRefs(mAttachedQueue)); // This call addrefs the array.

  mAttachedQueue->AppendElement(aBinding);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::AddHandlerToAttachedQueue(nsIXBLBindingAttachedHandler* aBinding)
{
  if (!mAttachedQueue)
    NS_NewISupportsArray(getter_AddRefs(mAttachedQueue)); // This call addrefs the array.

  mAttachedQueue->AppendElement(aBinding);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ClearAttachedQueue()
{
  if (mAttachedQueue)
    mAttachedQueue->Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ProcessAttachedQueue()
{
  if (!mAttachedQueue)
    return NS_OK;

  PRUint32 count;
  mAttachedQueue->Count(&count);
  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsISupports> supp;
    mAttachedQueue->GetElementAt(0, getter_AddRefs(supp));
    mAttachedQueue->RemoveElementAt(0);

    nsCOMPtr<nsIXBLBinding> binding(do_QueryInterface(supp));
    if (binding)
      binding->ExecuteAttachedHandler();
    else {
      nsCOMPtr<nsIXBLBindingAttachedHandler> handler(do_QueryInterface(supp));
      if (handler)
        handler->OnBindingAttached();
    }
  }

  ClearAttachedQueue();
  return NS_OK;
}

PRBool PR_CALLBACK ExecuteDetachedHandler(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsIXBLBinding* binding = (nsIXBLBinding*)aData;
  binding->ExecuteDetachedHandler();
  return PR_TRUE;
}


NS_IMETHODIMP
nsBindingManager::ExecuteDetachedHandlers()
{
  // Walk our hashtable of bindings.
  if (mBindingTable)
    mBindingTable->Enumerate(ExecuteDetachedHandler);
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo)
{
  if (!mDocumentTable)
    mDocumentTable = new nsSupportsHashtable();

  nsCOMPtr<nsIDocument> doc;
  aDocumentInfo->GetDocument(getter_AddRefs(doc));

  nsCOMPtr<nsIURI> uri(getter_AddRefs(doc->GetDocumentURL()));
  nsXPIDLCString str;
  uri->GetSpec(getter_Copies(str));
  
  nsCStringKey key((const char*)str);
  mDocumentTable->Put(&key, aDocumentInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetXBLDocumentInfo(const nsCString& aURL, nsIXBLDocumentInfo** aResult)
{
  *aResult = nsnull;
  if (!mDocumentTable)
    return NS_OK;

  nsCStringKey key(aURL);
  *aResult = NS_STATIC_CAST(nsIXBLDocumentInfo*, mDocumentTable->Get(&key)); // Addref happens here.

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::PutLoadingDocListener(const nsCString& aURL, nsIStreamListener* aListener)
{
  if (!mLoadingDocTable)
    mLoadingDocTable = new nsSupportsHashtable();

  nsCStringKey key(aURL);
  mLoadingDocTable->Put(&key, aListener);

  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetLoadingDocListener(const nsCString& aURL, nsIStreamListener** aResult)
{
  *aResult = nsnull;
  if (!mLoadingDocTable)
    return NS_OK;

  nsCStringKey key(aURL);
  *aResult = NS_STATIC_CAST(nsIStreamListener*, mLoadingDocTable->Get(&key)); // Addref happens here.
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::RemoveLoadingDocListener(const nsCString& aURL)
{
  if (!mLoadingDocTable)
    return NS_OK;

  nsCStringKey key(aURL);
  mLoadingDocTable->Remove(&key);

  return NS_OK;
}

PRBool PR_CALLBACK MarkForDeath(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsIXBLBinding* binding = (nsIXBLBinding*)aData;
  binding->MarkForDeath();
  return PR_TRUE;
}

NS_IMETHODIMP
nsBindingManager::FlushChromeBindings()
{
  mBindingTable->Enumerate(MarkForDeath);

  mDocumentTable = nsnull;
  
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::GetBindingImplementation(nsIContent* aContent, void* aScriptObject, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aContent, getter_AddRefs(binding));
  if (binding) {
    PRBool supportsInterface;
    binding->ImplementsInterface(aIID, &supportsInterface);
    if (supportsInterface) {
      nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS;
      GetWrappedJS(aContent, getter_AddRefs(wrappedJS));

      if (wrappedJS)
        return wrappedJS->AggregatedQueryInterface(aIID, aResult);

      // We have never made a wrapper for this implementation.
      // Create an XPC wrapper for the script object and hand it back.
      JSObject* jsobj = (JSObject*)aScriptObject;

      nsCOMPtr<nsIDocument> doc;
      aContent->GetDocument(*getter_AddRefs(doc));
      if (!doc)
        return NS_NOINTERFACE;

      nsCOMPtr<nsIScriptGlobalObject> global;
      doc->GetScriptGlobalObject(getter_AddRefs(global));
      if (!global)
        return NS_NOINTERFACE;

      nsCOMPtr<nsIScriptContext> context;
      global->GetContext(getter_AddRefs(context));
      if (!context)
        return NS_NOINTERFACE;

      JSContext* jscontext = (JSContext*)context->GetNativeContext();
      if (!jscontext)
        return NS_NOINTERFACE;

      nsCOMPtr<nsIXPConnect> xpConnect = do_GetService("@mozilla.org/js/xpc/XPConnect;1");
      if (!xpConnect)
        return NS_NOINTERFACE;

      nsISupports* nativeThis = (nsISupports*)JS_GetPrivate(jscontext, jsobj);
      nsresult rv = xpConnect->WrapJSAggregatedToNative(nativeThis, jscontext, jsobj, aIID, aResult);
      if (NS_FAILED(rv))
        return rv;

      // We successfully created a wrapper.  We will own this wrapper for as long as the binding remains
      // alive.  At the time the binding is cleared out of the bindingManager, we will remove the wrapper
      // from the bindingManager as well.
      nsISupports* supp = NS_STATIC_CAST(nsISupports*, *aResult);
      wrappedJS = do_QueryInterface(supp);
      SetWrappedJS(aContent, wrappedJS);

      return rv;
    }
  }
  
  *aResult = nsnull;
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsBindingManager::InheritsStyle(nsIContent* aContent, PRBool* aResult)
{
  // Get our enclosing parent.
  *aResult = PR_TRUE;
  nsCOMPtr<nsIContent> parent;
  GetEnclosingScope(aContent, getter_AddRefs(parent));
  if (parent) {
    // See if the parent is our parent.
    nsCOMPtr<nsIContent> ourParent;
    aContent->GetParent(*getter_AddRefs(ourParent));
    if (ourParent == parent) {
      // Yes. Check the binding and see if it wants to allow us
      // to inherit styles.
      nsCOMPtr<nsIXBLBinding> binding;
      GetBinding(parent, getter_AddRefs(binding));
      if (binding)
        binding->InheritsStyle(aResult);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::UseDocumentRules(nsIContent* aContent, PRBool* aResult)
{
  if (!aContent)
    return NS_OK;

  nsCOMPtr<nsIContent> parent;
  GetOutermostStyleScope(aContent, getter_AddRefs(parent));
  *aResult = !parent;
  return NS_OK;
}

void
nsBindingManager::GetEnclosingScope(nsIContent* aContent,
                                    nsIContent** aParent)
{
  // Look up the enclosing parent.
  aContent->GetBindingParent(aParent);
}

void
nsBindingManager::GetOutermostStyleScope(nsIContent* aContent,
                                         nsIContent** aParent)
{
  nsCOMPtr<nsIContent> parent;
  GetEnclosingScope(aContent, getter_AddRefs(parent));
  while (parent) {
    PRBool inheritsStyle = PR_TRUE;
    nsCOMPtr<nsIXBLBinding> binding;
    GetBinding(parent, getter_AddRefs(binding));
    if (binding) {
      binding->InheritsStyle(&inheritsStyle);
    }
    if (!inheritsStyle)
      break;
    nsCOMPtr<nsIContent> child = parent;
    GetEnclosingScope(child, getter_AddRefs(parent));
    if (parent == child)
      break; // The scrollbar case only is deliberately hacked to return itself
             // (see GetBindingParent in nsXULElement.cpp).
  }
  *aParent = parent;
  NS_IF_ADDREF(*aParent);
}

void
nsBindingManager::WalkRules(nsISupportsArrayEnumFunc aFunc, void* aData,
                            nsIContent* aParent, nsIContent* aCurrContent)
{
  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aCurrContent, getter_AddRefs(binding));
  if (binding) {
    binding->WalkRules(aFunc, aData);
  }
  if (aParent != aCurrContent) {
    nsCOMPtr<nsIContent> par;
    GetEnclosingScope(aCurrContent, getter_AddRefs(par));
    if (par)
      WalkRules(aFunc, aData, aParent, par);
  }
}

NS_IMETHODIMP
nsBindingManager::WalkRules(nsIStyleSet* aStyleSet,
                            nsISupportsArrayEnumFunc aFunc, void* aData,
                            nsIContent* aContent)
{
  if (!aContent)
    return NS_OK;

  nsCOMPtr<nsIContent> parent;
  GetOutermostStyleScope(aContent, getter_AddRefs(parent));

  WalkRules(aFunc, aData, parent, aContent);

  if (parent) {
    // We cut ourselves off, but we still need to walk the document's attribute sheet
    // so that inline style continues to work on anonymous content.
    nsCOMPtr<nsIDocument> document;
    aContent->GetDocument(*getter_AddRefs(document));
    nsCOMPtr<nsIHTMLContentContainer> container(do_QueryInterface(document));
    nsCOMPtr<nsIHTMLCSSStyleSheet> inlineSheet;
    container->GetInlineStyleSheet(getter_AddRefs(inlineSheet));  
    nsCOMPtr<nsIStyleRuleProcessor> inlineCSS(do_QueryInterface(inlineSheet));
    (*aFunc)((nsISupports*)(inlineCSS.get()), aData);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ShouldBuildChildFrames(nsIContent* aContent, PRBool* aResult)
{
  *aResult = PR_TRUE;

  nsCOMPtr<nsIXBLBinding> binding;
  GetBinding(aContent, getter_AddRefs(binding));

  if (binding)
    return binding->ShouldBuildChildFrames(aResult);

  return NS_OK;
}

nsresult
nsBindingManager::GetNestedInsertionPoint(nsIContent* aParent, nsIContent* aChild, nsIContent** aResult)
{
  *aResult = nsnull;

  // Check to see if the content is anonymous.
  nsCOMPtr<nsIContent> bindingParent;
  aChild->GetBindingParent(getter_AddRefs(bindingParent));
  if (bindingParent == aParent)
    return NS_OK; // It is anonymous. Don't use the insertion point, since that's only
                  // for the explicit kids.

  nsCOMPtr<nsIContent> insertionElement;
  PRUint32 index;
  GetInsertionPoint(aParent, aChild, getter_AddRefs(insertionElement), &index);
  if (insertionElement != aParent) {
    // See if we nest even further in.
    nsCOMPtr<nsIContent> nestedPoint;
    GetNestedInsertionPoint(insertionElement, aChild, getter_AddRefs(nestedPoint));
    if (nestedPoint)
      insertionElement = nestedPoint;
  }

  *aResult = insertionElement;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ContentAppended(nsIDocument* aDocument,
			                            nsIContent* aContainer,
                                  PRInt32     aNewIndexInContainer)
{
  // XXX This is hacked and not quite correct. See below.
  if (aNewIndexInContainer == -1 || !mContentListTable)
    // It's anonymous.
    return NS_OK;

  PRInt32 childCount;
  nsCOMPtr<nsIContent> child;
  aContainer->ChildCount(childCount);
  aContainer->ChildAt(aNewIndexInContainer, *getter_AddRefs(child));
  
  nsCOMPtr<nsIContent> ins;
  GetNestedInsertionPoint(aContainer, child, getter_AddRefs(ins));

  if (ins) {
    nsISupportsKey key(ins);
    nsCOMPtr<nsIDOMNodeList> nodeList = getter_AddRefs(NS_STATIC_CAST(nsIDOMNodeList*, mContentListTable->Get(&key)));
    if (nodeList) {
      nsCOMPtr<nsIAnonymousContentList> contentList(do_QueryInterface(nodeList));
      if (contentList) {
        // Find a non-pseudo-insertion point and just jam ourselves in.
        // This is not 100% correct.  Hack city, baby.
        PRUint32 count;
        contentList->GetInsertionPointCount(&count);
        for (PRUint32 i =0; i < count; i++) {
          nsCOMPtr<nsIXBLInsertionPoint> point;
          contentList->GetInsertionPointAt(i, getter_AddRefs(point));
          PRInt32 index;
          point->GetInsertionIndex(&index);
          if (index != -1) {
            // We're real. Jam all the kids in.
            // XXX Check the filters to find the correct points.
            for (PRInt32 j = aNewIndexInContainer; j < childCount; j++) {
              aContainer->ChildAt(j, *getter_AddRefs(child));
              point->AddChild(child);
              SetInsertionParent(child, ins);
            }
            break;
          }
        }
      }
    }
  }
 
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ContentInserted(nsIDocument* aDocument,
			                     nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
// XXX This is hacked just to make menus work again.
  if (aIndexInContainer == -1 || !mContentListTable)
    // It's anonymous.
    return NS_OK;
 
  nsCOMPtr<nsIContent> ins;
  GetNestedInsertionPoint(aContainer, aChild, getter_AddRefs(ins));

  if (ins) {
    nsISupportsKey key(ins);
    nsCOMPtr<nsIDOMNodeList> nodeList = getter_AddRefs(NS_STATIC_CAST(nsIDOMNodeList*, mContentListTable->Get(&key)));
    if (nodeList) {
      nsCOMPtr<nsIAnonymousContentList> contentList(do_QueryInterface(nodeList));
      if (contentList) {
        // Find a non-pseudo-insertion point and just jam ourselves in.
        // This is not 100% correct.  Hack city, baby.
        PRUint32 count;
        contentList->GetInsertionPointCount(&count);
        for (PRUint32 i =0; i < count; i++) {
          nsCOMPtr<nsIXBLInsertionPoint> point;
          contentList->GetInsertionPointAt(i, getter_AddRefs(point));
          PRInt32 index;
          point->GetInsertionIndex(&index);
          if (index != -1) {
            // We're real. Jam the kid in.
            // XXX Check the filters to find the correct points.
            point->InsertChildAt(aIndexInContainer, aChild);
            SetInsertionParent(aChild, ins);
            break;
          }
        }
      }
    }
  }
 
  return NS_OK;
}

NS_IMETHODIMP
nsBindingManager::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer)

{
  if (aIndexInContainer == -1 || !mContentListTable)
    // It's anonymous.
    return NS_OK;
 
  nsCOMPtr<nsIContent> point;
  GetNestedInsertionPoint(aContainer, aChild, getter_AddRefs(point));

  if (point) {
    nsISupportsKey key(point);
    nsCOMPtr<nsIDOMNodeList> nodeList = getter_AddRefs(NS_STATIC_CAST(nsIDOMNodeList*, mContentListTable->Get(&key)));
    if (nodeList) {
      nsCOMPtr<nsIAnonymousContentList> contentList(do_QueryInterface(nodeList));
      if (contentList) {
        // Find a non-pseudo-insertion point and remove ourselves.
        PRUint32 count;
        contentList->GetInsertionPointCount(&count);
        for (PRUint32 i =0; i < count; i++) {
          nsCOMPtr<nsIXBLInsertionPoint> point;
          contentList->GetInsertionPointAt(i, getter_AddRefs(point));
          PRInt32 index;
          point->GetInsertionIndex(&index);
          if (index != -1) {
            point->RemoveChild(aChild);
          }
        }
      }
    }
  }
 
  return NS_OK;
}

// Creation Routine ///////////////////////////////////////////////////////////////////////

nsresult
NS_NewBindingManager(nsIBindingManager** aResult)
{
  *aResult = new nsBindingManager;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

