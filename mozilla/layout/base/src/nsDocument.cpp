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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "plstr.h"

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsDocument.h"
#include "nsIArena.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsIChannel.h"
#include "nsString.h"
#include "nsIContent.h"
#include "nsIDocumentObserver.h"
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsEventListenerManager.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptEventListener.h"
#include "nsDOMEvent.h"
#include "nsDOMEventsIIDs.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEventStateManager.h"
#include "nsContentList.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDOMStyleSheetList.h"
#include "nsDOMAttribute.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsGenericElement.h"

#include "nsICSSStyleSheet.h"

#include "nsITextContent.h"
#include "nsIDocumentEncoder.h"
//#include "nsXIFConverter.h"
#include "nsIHTMLContentSink.h"
//#include "nsHTMLContentSinkStream.h"
//#include "nsHTMLToTXTSinkStream.h"
//#include "nsXIFDTD.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"

#include "nsRange.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "nsDOMDocumentType.h"

#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"

#include "nsLayoutAtoms.h"
#include "nsLayoutCID.h"
#include "nsIDOMSelection.h"
#include "nsIDOMRange.h"
#include "nsIEnumerator.h"
#include "nsDOMError.h"
#include "nsIScrollableView.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"

#include "nsNetUtil.h"     // for NS_MakeAbsoluteURI

#include "nsIScriptSecurityManager.h"
#include "nsIAggregatePrincipal.h"

static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIDOMCommentIID, NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOCUMENT_IID);

#include "nsIDOMElement.h"

static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMNSDocumentIID, NS_IDOMNSDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIDOMAttrIID, NS_IDOMATTR_IID);
static NS_DEFINE_IID(kIScriptEventListenerIID, NS_ISCRIPTEVENTLISTENER_IID);
static NS_DEFINE_IID(kIPrivateDOMEventIID, NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIEventListenerManagerIID, NS_IEVENTLISTENERMANAGER_IID);
static NS_DEFINE_IID(kIPostDataIID, NS_IPOSTDATA_IID);
static NS_DEFINE_IID(kIDOMStyleSheetIID, NS_IDOMSTYLESHEET_IID);
static NS_DEFINE_IID(kIDOMDOMImplementationIID, NS_IDOMDOMIMPLEMENTATION_IID);
static NS_DEFINE_IID(kIDocumentObserverIID, NS_IDOCUMENT_OBSERVER_IID);
static NS_DEFINE_IID(kICSSStyleSheetIID, NS_ICSS_STYLE_SHEET_IID);
static NS_DEFINE_IID(kCRangeCID, NS_RANGE_CID);
static NS_DEFINE_IID(kIDOMRange, NS_IDOMRANGE_IID);
static NS_DEFINE_IID(kIEnumeratorIID, NS_IENUMERATOR_IID);
static NS_DEFINE_IID(kIDOMScriptObjectFactoryIID, NS_IDOM_SCRIPT_OBJECT_FACTORY_IID);
static NS_DEFINE_IID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);


#include "nsILineBreakerFactory.h"
#include "nsIWordBreakerFactory.h"
#include "nsLWBrkCIID.h"
static NS_DEFINE_IID(kLWBrkCID, NS_LWBRK_CID);
static NS_DEFINE_IID(kILineBreakerFactoryIID, NS_ILINEBREAKERFACTORY_IID);
static NS_DEFINE_IID(kIWordBreakerFactoryIID, NS_IWORDBREAKERFACTORY_IID);

#include "nsIHTMLDocument.h"
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);

class nsDOMStyleSheetList : public nsIDOMStyleSheetList,
                            public nsIScriptObjectOwner,
                            public nsIDocumentObserver
{
public:
  nsDOMStyleSheetList(nsIDocument *aDocument);
  virtual ~nsDOMStyleSheetList();

  NS_DECL_ISUPPORTS
  NS_DECL_IDOMSTYLESHEETLIST
  
  NS_IMETHOD BeginUpdate(nsIDocument *aDocument) { return NS_OK; }
  NS_IMETHOD EndUpdate(nsIDocument *aDocument) { return NS_OK; }
  NS_IMETHOD BeginLoad(nsIDocument *aDocument) { return NS_OK; }
  NS_IMETHOD EndLoad(nsIDocument *aDocument) { return NS_OK; }
  NS_IMETHOD BeginReflow(nsIDocument *aDocument,
			                   nsIPresShell* aShell) { return NS_OK; }
  NS_IMETHOD EndReflow(nsIDocument *aDocument,
		                   nsIPresShell* aShell) { return NS_OK; } 
  NS_IMETHOD ContentChanged(nsIDocument *aDocument,
			                      nsIContent* aContent,
                            nsISupports* aSubContent) { return NS_OK; }
  NS_IMETHOD ContentStatesChanged(nsIDocument* aDocument,
                                  nsIContent* aContent1,
                                  nsIContent* aContent2) { return NS_OK; }
  NS_IMETHOD AttributeChanged(nsIDocument *aDocument,
                              nsIContent*  aContent,
                              PRInt32      aNameSpaceID,
                              nsIAtom*     aAttribute,
                              PRInt32      aHint) { return NS_OK; }
  NS_IMETHOD ContentAppended(nsIDocument *aDocument,
			                       nsIContent* aContainer,
                             PRInt32     aNewIndexInContainer) 
                             { return NS_OK; }
  NS_IMETHOD ContentInserted(nsIDocument *aDocument,
			                       nsIContent* aContainer,
                             nsIContent* aChild,
                             PRInt32 aIndexInContainer) { return NS_OK; }
  NS_IMETHOD ContentReplaced(nsIDocument *aDocument,
			                       nsIContent* aContainer,
                             nsIContent* aOldChild,
                             nsIContent* aNewChild,
                             PRInt32 aIndexInContainer) { return NS_OK; }
  NS_IMETHOD ContentRemoved(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer) { return NS_OK; }
  NS_IMETHOD StyleSheetAdded(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetRemoved(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet);
  NS_IMETHOD StyleSheetDisabledStateChanged(nsIDocument *aDocument,
                                        nsIStyleSheet* aStyleSheet,
                                        PRBool aDisabled) { return NS_OK; }
  NS_IMETHOD StyleRuleChanged(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule,
                              PRInt32 aHint) { return NS_OK; }
  NS_IMETHOD StyleRuleAdded(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule) { return NS_OK; }
  NS_IMETHOD StyleRuleRemoved(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) { return NS_OK; }
  NS_IMETHOD DocumentWillBeDestroyed(nsIDocument *aDocument);

  // nsIScriptObjectOwner interface
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void* aScriptObject);

protected:
  PRInt32       mLength;
  nsIDocument*  mDocument;
  void*         mScriptObject;
};

nsDOMStyleSheetList::nsDOMStyleSheetList(nsIDocument *aDocument)
{
  NS_INIT_REFCNT();
  mLength = -1;
  // Not reference counted to avoid circular references.
  // The document will tell us when its going away.
  mDocument = aDocument;
  mDocument->AddObserver(this);
  mScriptObject = nsnull;
}

nsDOMStyleSheetList::~nsDOMStyleSheetList()
{
  if (nsnull != mDocument) {
    mDocument->RemoveObserver(this);
  }
  mDocument = nsnull;
}

NS_IMPL_ADDREF(nsDOMStyleSheetList)
NS_IMPL_RELEASE(nsDOMStyleSheetList)

NS_INTERFACE_MAP_BEGIN(nsDOMStyleSheetList)
   NS_INTERFACE_MAP_ENTRY(nsIDOMStyleSheetList)
   NS_INTERFACE_MAP_ENTRY(nsIScriptObjectOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMStyleSheetList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP    
nsDOMStyleSheetList::GetLength(PRUint32* aLength)
{
  if (nsnull != mDocument) {
    // XXX Find the number and then cache it. We'll use the 
    // observer notification to figure out if new ones have
    // been added or removed.
    if (-1 == mLength) {
      PRUint32 count = 0;
      PRInt32 i, imax = mDocument->GetNumberOfStyleSheets();
      
      for (i = 0; i < imax; i++) {
        nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(i);
        nsIDOMStyleSheet *domss;

        if (NS_OK == sheet->QueryInterface(kIDOMStyleSheetIID, (void **)&domss)) {
          count++;
          NS_RELEASE(domss);
        }

        NS_RELEASE(sheet);
      }
      mLength = count;
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
  if (nsnull != mDocument) {
    PRUint32 count = 0;
    PRInt32 i, imax = mDocument->GetNumberOfStyleSheets();
  
    // XXX Not particularly efficient, but does anyone care?
    for (i = 0; (i < imax) && (nsnull == *aReturn); i++) {
      nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(i);
      nsIDOMStyleSheet *domss;
      
      if (NS_OK == sheet->QueryInterface(kIDOMStyleSheetIID, (void **)&domss)) {
        if (count++ == aIndex) {
          *aReturn = domss;
          NS_ADDREF(domss);
        }
        NS_RELEASE(domss);
      }
      
      NS_RELEASE(sheet);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsDOMStyleSheetList::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;

  if (nsnull == mScriptObject) {
    nsISupports *supports = (nsISupports *)(nsIDOMStyleSheetList *)this;
    nsISupports *parent = (nsISupports *)mDocument;

    // XXX Should be done through factory
    res = NS_NewScriptStyleSheetList(aContext, 
                                     supports,
                                     parent,
                                     (void**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;

  return res;
}

NS_IMETHODIMP 
nsDOMStyleSheetList::SetScriptObject(void* aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

NS_IMETHODIMP 
nsDOMStyleSheetList::StyleSheetAdded(nsIDocument *aDocument,
                                     nsIStyleSheet* aStyleSheet)
{
  if (-1 != mLength) {
    nsIDOMStyleSheet *domss;
    if (NS_OK == aStyleSheet->QueryInterface(kIDOMStyleSheetIID, (void **)&domss)) {
      mLength++;
      NS_RELEASE(domss);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
nsDOMStyleSheetList::StyleSheetRemoved(nsIDocument *aDocument,
                                       nsIStyleSheet* aStyleSheet)
{
  if (-1 != mLength) {
    nsIDOMStyleSheet *domss;
    if (NS_OK == aStyleSheet->QueryInterface(kIDOMStyleSheetIID, (void **)&domss)) {
      mLength--;
      NS_RELEASE(domss);
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

// ==================================================================
// =
// ==================================================================

class nsDOMImplementation : public nsIDOMDOMImplementation,
                            public nsIScriptObjectOwner
{
public:
  nsDOMImplementation(nsIDocument* aDocument = nsnull);
  virtual ~nsDOMImplementation();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD    HasFeature(const nsString& aFeature, 
                           const nsString& aVersion, 
                           PRBool* aReturn);
  NS_IMETHOD    CreateDocumentType(const nsString& aQualifiedName,
                                   const nsString& aPublicId,
                                   const nsString& aSystemId,
                                   nsIDOMDocumentType** aReturn);

  NS_IMETHOD    CreateDocument(const nsString& aNamespaceURI,
                               const nsString& aQualifiedName,
                               nsIDOMDocumentType* aDoctype,
                               nsIDOMDocument** aReturn);

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

protected:
  void *mScriptObject;
  nsCOMPtr<nsIDocument> mDocument;
};

nsDOMImplementation::nsDOMImplementation(nsIDocument* aDocument)
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
  mDocument = aDocument;
}

nsDOMImplementation::~nsDOMImplementation()
{
}

NS_IMPL_ADDREF(nsDOMImplementation)
NS_IMPL_RELEASE(nsDOMImplementation)

nsresult 
nsDOMImplementation::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMDOMImplementationIID)) {
    nsIDOMDOMImplementation* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsIDOMDOMImplementation* tmp = this;
    nsISupports* tmp2 = tmp;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP    
nsDOMImplementation::HasFeature(const nsString& aFeature, 
                                const nsString& aVersion, 
                                PRBool* aReturn)
{
  // XXX Currently this is hardcoded. In the future, we should
  // probably figure out some of this by querying the registry??
  PRInt32 result;
  float ver = aVersion.ToFloat(&result);
  if (NS_FAILED(result)) {
    return result;
  }

  if ((aFeature.EqualsIgnoreCase("HTML") ||
      aFeature.EqualsIgnoreCase("XML")) &&
      ((ver == 1.0) || (ver == 2.0) || (0 ==aVersion.Length()))) {
    *aReturn = PR_TRUE;
  }
  else {
    *aReturn = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocumentType(const nsString& aQualifiedName,
                                        const nsString& aPublicId, 
                                        const nsString& aSystemId, 
                                        nsIDOMDocumentType** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  return NS_NewDOMDocumentType(aReturn, aQualifiedName, nsnull, nsnull,
                               aPublicId, aSystemId, nsAutoString());
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocument(const nsString& aNamespaceURI, 
                                    const nsString& aQualifiedName, 
                                    nsIDOMDocumentType* aDoctype, 
                                    nsIDOMDocument** aReturn)
{  
  NS_ENSURE_ARG_POINTER(aReturn);    
  
  nsresult rv = NS_OK;
  *aReturn = nsnull;

  nsIURI* baseURI;
  rv = mDocument->GetBaseURL(baseURI);
  if (NS_FAILED(rv)) return rv;

  NS_NewDOMDocument(aReturn, aNamespaceURI, aQualifiedName, aDoctype, baseURI);

  NS_RELEASE(baseURI);
  return rv;
}

NS_IMETHODIMP 
nsDOMImplementation::GetScriptObject(nsIScriptContext *aContext, 
                                     void** aScriptObject)
{
  nsresult result = NS_OK;

  if (nsnull == mScriptObject) {
    NS_WITH_SERVICE(nsIDOMScriptObjectFactory, factory, 
                    kDOMScriptObjectFactoryCID, &result);

    if (NS_OK == result) {
      nsIScriptGlobalObject *global = aContext->GetGlobalObject();

      result = factory->NewScriptDOMImplementation(aContext, (nsISupports*)(nsIDOMDOMImplementation*)this,
                                                   global, &mScriptObject);
      NS_RELEASE(global);
    }
  }
  
  *aScriptObject = mScriptObject;
  return result;
}

NS_IMETHODIMP 
nsDOMImplementation::SetScriptObject(void *aScriptObject)
{
  mScriptObject = nsnull;
  return NS_OK;
}

// ==================================================================
// =
// ==================================================================

MOZ_DECL_CTOR_COUNTER(nsDocumentChildNodes);

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
  if (nsnull != mDocument) {
    PRInt32 count;
    mDocument->GetChildCount(count);
    *aLength = (PRUint32)count;
  }
  else {
    *aLength = 0;
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsDocumentChildNodes::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsresult result = NS_OK;
  nsIContent* content = nsnull;

  *aReturn = nsnull;
  if (nsnull != mDocument) {
    result = mDocument->ChildAt(aIndex, content);
    if ((NS_OK == result) && (nsnull != content)) {
      result = content->QueryInterface(kIDOMNodeIID, (void**)aReturn);
    }
  }

  return result;
}

void 
nsDocumentChildNodes::DropReference()
{
  mDocument = nsnull;
}


// ==================================================================
// =
// ==================================================================

nsDocument::nsDocument()
{
  NS_INIT_REFCNT();

  mArena = nsnull;
  mDocumentTitle = nsnull;
  mDocumentURL = nsnull;
  mCharacterSet.AssignWithConversion("ISO-8859-1");
  mParentDocument = nsnull;
  mRootContent = nsnull;
  mScriptObject = nsnull;
  mListenerManager = nsnull;
  mDisplaySelection = PR_FALSE;
  mInDestructor = PR_FALSE;
  mDOMStyleSheets = nsnull;
  mNameSpaceManager = nsnull;
  mHeaderData = nsnull;
  mLineBreaker = nsnull;
  mProlog = nsnull;
  mEpilog = nsnull;
  mChildNodes = nsnull;
  mWordBreaker = nsnull;
  mModCount = 0;
  mFileSpec = nsnull;
  mPrincipal = nsnull;
  mNextContentID = NS_CONTENT_ID_COUNTER_BASE;
  Init();/* XXX */
}

nsDocument::~nsDocument()
{
  // XXX Inform any remaining observers that we are going away.
  // Note that this currently contradicts the rule that all
  // observers must hold on to live references to the document.
  // This notification will occur only after the reference has
  // been dropped.
  mInDestructor = PR_TRUE;
  PRInt32 index, count;
  for (index = 0; index < mObservers.Count(); index++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
    observer->DocumentWillBeDestroyed(this);
    if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
      index--;
    }
  }

  if (nsnull != mDocumentTitle) {
    delete mDocumentTitle;
    mDocumentTitle = nsnull;
  }
  NS_IF_RELEASE(mDocumentURL);
  NS_IF_RELEASE(mPrincipal);
  mDocumentLoadGroup = null_nsCOMPtr();

  mParentDocument = nsnull;

  // Delete references to sub-documents
  index = mSubDocuments.Count();
  while (--index >= 0) {
    nsIDocument* subdoc = (nsIDocument*) mSubDocuments.ElementAt(index);
    NS_RELEASE(subdoc);
  }

  NS_IF_RELEASE(mRootContent);

  // Delete references to style sheets
  index = mStyleSheets.Count();
  while (--index >= 0) {
    nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(index);
    sheet->SetOwningDocument(nsnull);
    NS_RELEASE(sheet);
  }

  nsIContent* content;
  if (nsnull != mProlog) {
    count = mProlog->Count();
    for (index = 0; index < count; index++) {
      content = (nsIContent*)mProlog->ElementAt(index);
      NS_RELEASE(content);
    }
    delete mProlog;
  }
  if (nsnull != mEpilog) {
    count = mEpilog->Count();
    for (index = 0; index < count; index++) {
      content = (nsIContent*)mEpilog->ElementAt(index);
      NS_RELEASE(content);
    }
    delete mEpilog;
  }

  if (nsnull != mChildNodes) {
    mChildNodes->DropReference();
    NS_RELEASE(mChildNodes);
  }

  NS_IF_RELEASE(mArena);
  NS_IF_RELEASE(mListenerManager);
  NS_IF_RELEASE(mDOMStyleSheets);
  NS_IF_RELEASE(mNameSpaceManager);
  if (nsnull != mHeaderData) {
    delete mHeaderData;
    mHeaderData = nsnull;
  }
  NS_IF_RELEASE(mLineBreaker);
  NS_IF_RELEASE(mWordBreaker);
  
	delete mFileSpec;
	
}

nsresult nsDocument::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDocumentIID)) {
    nsIDocument* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMDocumentIID)) {
    nsIDOMDocument* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNSDocumentIID)) {
    nsIDOMNSDocument* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMDocumentStyle))) {
    nsIDOMDocumentStyle* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMDocumentView))) {
    nsIDOMDocumentView* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptObjectOwnerIID)) {
    nsIScriptObjectOwner* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIJSScriptObjectIID)) {
    nsIJSScriptObject* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIScriptObjectPrincipal))) {
    nsIScriptObjectPrincipal* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventReceiverIID)) {
    nsIDOMEventReceiver* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventTargetIID)) {
    nsIDOMEventTarget* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNodeIID)) {
    nsIDOMNode* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDiskDocument))) {
    nsIDiskDocument* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsISupportsWeakReference))) {
    nsISupportsWeakReference* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    nsIDocument* tmp = this;
    nsISupports* tmp2 = tmp;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsDocument)
NS_IMPL_RELEASE(nsDocument)

nsresult nsDocument::Init()
{
  nsresult rv = NS_NewHeapArena(&mArena, nsnull);
  NS_NewNameSpaceManager(&mNameSpaceManager);
  return rv;
}

nsIArena* nsDocument::GetArena()
{
  if (nsnull != mArena) {
    NS_ADDREF(mArena);
  }
  return mArena;
}

nsresult
nsDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsresult rv = NS_OK;

  if (nsnull != mDocumentTitle) {
    delete mDocumentTitle;
    mDocumentTitle = nsnull;
  }
  NS_IF_RELEASE(mDocumentURL);
  NS_IF_RELEASE(mPrincipal);
  mDocumentLoadGroup = null_nsCOMPtr();

  // Delete references to sub-documents
  PRInt32 index = mSubDocuments.Count();
  while (--index >= 0) {
    nsIDocument* subdoc = (nsIDocument*) mSubDocuments.ElementAt(index);
    NS_RELEASE(subdoc);
  }

  if (nsnull != mRootContent) {
    // Ensure that document is nsnull to allow validity checks on content
    mRootContent->SetDocument(nsnull, PR_TRUE);
    ContentRemoved(nsnull, mRootContent, 0);
    NS_IF_RELEASE(mRootContent);
  }

  // Delete references to style sheets
  index = mStyleSheets.Count();
  while (--index >= 0) {
    nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(index);
    sheet->SetOwningDocument(nsnull);

    PRInt32 pscount = mPresShells.Count();
    PRInt32 psindex;
    for (psindex = 0; psindex < pscount; psindex++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(psindex);
      nsCOMPtr<nsIStyleSet> set;
      if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
        if (set) {
          set->RemoveDocStyleSheet(sheet);
        }
      }
    }

    // XXX Tell observers?

    NS_RELEASE(sheet);
  }
  mStyleSheets.Clear();

  NS_IF_RELEASE(mListenerManager);
  NS_IF_RELEASE(mDOMStyleSheets);

  NS_IF_RELEASE(mNameSpaceManager);

  (void)aChannel->GetURI(&mDocumentURL);
  nsCOMPtr<nsISupports> owner;
  aChannel->GetOwner(getter_AddRefs(owner));
  if (owner)
    owner->QueryInterface(NS_GET_IID(nsIPrincipal), (void**)&mPrincipal);

  mDocumentLoadGroup = getter_AddRefs(NS_GetWeakReference(aLoadGroup));
  // there was an assertion here that aLoadGroup was not null.  This is no longer valid
  // nsWebShell::SetDocument does not create a load group, and it works just fine.

  if (NS_OK == rv) {
    rv = NS_NewNameSpaceManager(&mNameSpaceManager);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::StartDocumentLoad(const char* aCommand,
                              nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              nsISupports* aContainer,
                              nsIStreamListener **aDocListener,
                              PRBool aReset)
{
  nsresult rv = NS_OK;
  if (aReset)
    rv = Reset(aChannel, aLoadGroup);
  return rv;
}

NS_IMETHODIMP 
nsDocument::StopDocumentLoad()
{
  return NS_OK;
}

const nsString* nsDocument::GetDocumentTitle() const
{
  return mDocumentTitle;
}

nsIURI* nsDocument::GetDocumentURL() const
{
  NS_IF_ADDREF(mDocumentURL);
  return mDocumentURL;
}

NS_IMETHODIMP
nsDocument::GetPrincipal(nsIPrincipal **aPrincipal)
{
  if (!mPrincipal) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) 
        return rv;
    if (NS_FAILED(rv = securityManager->GetCodebasePrincipal(mDocumentURL, 
                                                             &mPrincipal)))
        return rv;
  }

  if(aPrincipal)
  {
    *aPrincipal = mPrincipal;
    NS_ADDREF(*aPrincipal);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddPrincipal(nsIPrincipal *aNewPrincipal)
{
  nsresult rv;
  if (!mPrincipal)
    GetPrincipal(nsnull);

  nsCOMPtr<nsIAggregatePrincipal> agg =
    do_QueryInterface(mPrincipal, &rv);
  if (NS_SUCCEEDED(rv))
  {
    rv = agg->Intersect(aNewPrincipal);
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::GetContentType(nsString& aContentType) const
{
  // Must be implemented by derived class.
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetDocumentLoadGroup(nsILoadGroup **aGroup) const
{
  nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

  *aGroup = group;
  NS_IF_ADDREF(*aGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBaseURL(nsIURI*& aURL) const
{
  aURL = mDocumentURL;
  NS_IF_ADDREF(mDocumentURL);
  return NS_OK;
}

NS_IMETHODIMP nsDocument::GetDocumentCharacterSet(nsString& oCharSetID) 
{
  oCharSetID = mCharacterSet;
  return NS_OK;
}

NS_IMETHODIMP nsDocument::SetDocumentCharacterSet(const nsString& aCharSetID)
{
  mCharacterSet = aCharSetID;
  return NS_OK;
}

NS_IMETHODIMP nsDocument::GetLineBreaker(nsILineBreaker** aResult) 
{
  if(nsnull == mLineBreaker ) {
     // no line breaker, find a default one
     nsILineBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          kILineBreakerFactoryIID,
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsILineBreaker *lb = nsnull ;
      nsAutoString lbarg;
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mLineBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mLineBreaker;
  NS_IF_ADDREF(mLineBreaker);
  return NS_OK; // XXX we should do error handling here
}
NS_IMETHODIMP nsDocument::SetLineBreaker(nsILineBreaker* aLineBreaker) 
{
  NS_IF_RELEASE(mLineBreaker);
  mLineBreaker = aLineBreaker;
  NS_IF_ADDREF(mLineBreaker);
  return NS_OK;
}
NS_IMETHODIMP nsDocument::GetWordBreaker(nsIWordBreaker** aResult) 
{
  if(nsnull == mWordBreaker ) {
     // no line breaker, find a default one
     nsIWordBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          kIWordBreakerFactoryIID,
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsIWordBreaker *lb = nsnull ;
      nsAutoString lbarg;
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mWordBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mWordBreaker;
  NS_IF_ADDREF(mWordBreaker);
  return NS_OK; // XXX we should do error handling here
}
NS_IMETHODIMP nsDocument::SetWordBreaker(nsIWordBreaker* aWordBreaker) 
{
  NS_IF_RELEASE(mWordBreaker);
  mWordBreaker = aWordBreaker;
  NS_IF_ADDREF(mWordBreaker);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetHeaderData(nsIAtom* aHeaderField, nsString& aData) const
{
  aData.Truncate();
  const nsDocHeaderData* data = mHeaderData;
  while (nsnull != data) {
    if (data->mField == aHeaderField) {
      aData = data->mData;
      break;
    }
    data = data->mNext;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetHeaderData(nsIAtom* aHeaderField, const nsString& aData)
{
  if (nsnull != aHeaderField) {
    if (nsnull == mHeaderData) {
      if (0 < aData.Length()) { // don't bother storing empty string
        mHeaderData = new nsDocHeaderData(aHeaderField, aData);
      }
    }
    else {
      nsDocHeaderData* data = mHeaderData;
      nsDocHeaderData** lastPtr = &mHeaderData;
      do {  // look for existing and replace
        if (data->mField == aHeaderField) {
          if (0 < aData.Length()) {
            data->mData.Assign(aData);
          }
          else {  // don't store empty string
            (*lastPtr)->mNext = data->mNext;
            data->mNext = nsnull;
            delete data;
          }
          return NS_OK;
        }
        lastPtr = &(data->mNext);
        data = data->mNext;
      } while (nsnull != data);
      // didn't find, append
      if (0 < aData.Length()) {
        *lastPtr = new nsDocHeaderData(aHeaderField, aData);
      }
    }
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

#if 0
// XXX Temp hack: moved to nsMarkupDocument
NS_IMETHODIMP
nsDocument::CreateShell(nsIPresContext* aContext,
                        nsIViewManager* aViewManager,
                        nsIStyleSet* aStyleSet,
                        nsIPresShell** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIPresShell* shell;
  nsresult rv = NS_NewPresShell(&shell);
  if (NS_OK != rv) {
    return rv;
  }

  if (NS_OK != shell->Init(this, aContext, aViewManager, aStyleSet)) {
    NS_RELEASE(shell);
    return rv;
  }

  // Note: we don't hold a ref to the shell (it holds a ref to us)
  mPresShells.AppendElement(shell);
  *aInstancePtrResult = shell;
  return NS_OK;
}
#endif

PRBool nsDocument::DeleteShell(nsIPresShell* aShell)
{
  return mPresShells.RemoveElement(aShell);
}

PRInt32 nsDocument::GetNumberOfShells()
{
  return mPresShells.Count();
}

nsIPresShell* nsDocument::GetShellAt(PRInt32 aIndex)
{
  nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(aIndex);
  if (nsnull != shell) {
    NS_ADDREF(shell);
  }
  return shell;
}

nsIDocument* nsDocument::GetParentDocument()
{
  if (nsnull != mParentDocument) {
    NS_ADDREF(mParentDocument);
  }
  return mParentDocument;
}

/**
 * Note that we do *not* AddRef our parent because that would
 * create a circular reference.
 */
void nsDocument::SetParentDocument(nsIDocument* aParent)
{
  mParentDocument = aParent;
}

void nsDocument::AddSubDocument(nsIDocument* aSubDoc)
{
  NS_ADDREF(aSubDoc);
  mSubDocuments.AppendElement(aSubDoc);
}

PRInt32 nsDocument::GetNumberOfSubDocuments()
{
  return mSubDocuments.Count();
}

nsIDocument* nsDocument::GetSubDocumentAt(PRInt32 aIndex)
{
  nsIDocument* doc = (nsIDocument*) mSubDocuments.ElementAt(aIndex);
  if (nsnull != doc) {
    NS_ADDREF(doc);
  }
  return doc;
}

nsIContent* nsDocument::GetRootContent()
{
  if (nsnull != mRootContent) {
    NS_ADDREF(mRootContent);
  }
  return mRootContent;
}

void nsDocument::SetRootContent(nsIContent* aRoot)
{
  NS_IF_RELEASE(mRootContent);
  if (nsnull != aRoot) {
    mRootContent = aRoot;
    NS_ADDREF(aRoot);
  }
}

NS_IMETHODIMP 
nsDocument::AppendToProlog(nsIContent* aContent)
{
  if (nsnull == mProlog) {
    mProlog = new nsVoidArray();
  }

  mProlog->AppendElement((void *)aContent);
  NS_ADDREF(aContent);

  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::AppendToEpilog(nsIContent* aContent)
{
  if (nsnull == mEpilog) {
    mEpilog = new nsVoidArray();
  }

  mEpilog->AppendElement((void *)aContent);
  NS_ADDREF(aContent);

  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
  nsIContent* content = nsnull;
  PRInt32 prolog = 0;

  if (nsnull != mProlog) {
    prolog = mProlog->Count();
    if (aIndex < prolog) {
      // It's in the prolog
      content = (nsIContent*)mProlog->ElementAt(aIndex);
    }
  }
  
  if (aIndex == prolog) {
    // It's the document element
    content = mRootContent;
  }
  else if ((aIndex > prolog) && (nsnull != mEpilog)) {
    // It's in the epilog
    content = (nsIContent*)mEpilog->ElementAt(aIndex-prolog-1);
  }

  NS_IF_ADDREF(content);
  aResult = content;

  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::IndexOf(nsIContent* aPossibleChild, PRInt32& aIndex) const
{
  PRInt32 index = -1;
  PRInt32 prolog = 0;

  if (nsnull != mProlog) {
    index = mProlog->IndexOf(aPossibleChild);
    prolog = mProlog->Count();
  }

  if (-1 == index) {
    if (aPossibleChild == mRootContent) {
      index = prolog;
    }
    else if (nsnull != mEpilog) {
      index = mEpilog->IndexOf(aPossibleChild);
      if (-1 != index) {
        index += (prolog+1);
      }
    }
  }
  
  aIndex = index;
  
  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::GetChildCount(PRInt32& aCount)
{
  aCount = 1;
  if (nsnull != mProlog) {
    aCount += mProlog->Count();
  }
  if (nsnull != mEpilog) {
    aCount += mEpilog->Count();
  }

  return NS_OK;
}

PRInt32 nsDocument::GetNumberOfStyleSheets()
{
  return mStyleSheets.Count();
}

nsIStyleSheet* nsDocument::GetStyleSheetAt(PRInt32 aIndex)
{
  nsIStyleSheet* sheet = (nsIStyleSheet*)mStyleSheets.ElementAt(aIndex);
  NS_IF_ADDREF(sheet);
  return sheet;
}

PRInt32 nsDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet)
{
  return mStyleSheets.IndexOf(aSheet);
}

void nsDocument::InternalAddStyleSheet(nsIStyleSheet* aSheet)  // subclass hook for sheet ordering
{
  mStyleSheets.AppendElement(aSheet);
}

void nsDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  InternalAddStyleSheet(aSheet);
  NS_ADDREF(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  if (enabled) {
    PRInt32 count = mPresShells.Count();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(index);
      nsCOMPtr<nsIStyleSet> set;
      if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
        if (set) {
          set->AddDocStyleSheet(aSheet, this);
        }
      }
    }

    // XXX should observers be notified for disabled sheets??? I think not, but I could be wrong
    for (index = 0; index < mObservers.Count(); index++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
      observer->StyleSheetAdded(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
        index--;
      }
    }
  }
}

void nsDocument::RemoveStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  mStyleSheets.RemoveElement(aSheet);
  
  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  if (enabled) {
    PRInt32 count = mPresShells.Count();
    PRInt32 index;
    for (index = 0; index < count; index++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(index);
      nsCOMPtr<nsIStyleSet> set;
      if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
        if (set) {
          set->RemoveDocStyleSheet(aSheet);
        }
      }
    }

    // XXX should observers be notified for disabled sheets??? I think not, but I could be wrong
    for (index = 0; index < mObservers.Count(); index++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
      observer->StyleSheetRemoved(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
        index--;
      }
    }
  }

  aSheet->SetOwningDocument(nsnull);
  NS_RELEASE(aSheet);
}

void 
nsDocument::InternalInsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex)
{ // subclass hook for sheet ordering
  mStyleSheets.InsertElementAt(aSheet, aIndex);
}

NS_IMETHODIMP
nsDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex, PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aSheet, "null ptr");
  InternalInsertStyleSheetAt(aSheet, aIndex);

  NS_ADDREF(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  PRInt32 count;
  PRInt32 index;
  if (enabled) {
    count = mPresShells.Count();
    for (index = 0; index < count; index++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(index);
      nsCOMPtr<nsIStyleSet> set;
      shell->GetStyleSet(getter_AddRefs(set));
      if (set) {
        set->AddDocStyleSheet(aSheet, this);
      }
    }
  }
  if (aNotify) {  // notify here even if disabled, there may have been others that weren't notified
    for (index = 0; index < mObservers.Count(); index++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
      observer->StyleSheetAdded(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
        index--;
      }
    }
  }
  return NS_OK;
}


void nsDocument::SetStyleSheetDisabledState(nsIStyleSheet* aSheet,
                                            PRBool aDisabled)
{
  NS_PRECONDITION(nsnull != aSheet, "null arg");
  PRInt32 index = mStyleSheets.IndexOf((void *)aSheet);
  PRInt32 count;
  // If we're actually in the document style sheet list
  if (-1 != index) {
    count = mPresShells.Count();
    for (index = 0; index < count; index++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(index);
      nsCOMPtr<nsIStyleSet> set;
      if (NS_SUCCEEDED(shell->GetStyleSet(getter_AddRefs(set)))) {
        if (set) {
          if (aDisabled) {
            set->RemoveDocStyleSheet(aSheet);
          }
          else {
            set->AddDocStyleSheet(aSheet, this);
          }
        }
      }
    }
  }  

  for (index = 0; index < mObservers.Count(); index++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(index);
    observer->StyleSheetDisabledStateChanged(this, aSheet, aDisabled);
    if (observer != (nsIDocumentObserver*)mObservers.ElementAt(index)) {
      index--;
    }
  }
}

NS_IMETHODIMP 
nsDocument::GetScriptGlobalObject(nsIScriptGlobalObject** aScriptGlobalObject)
{
   NS_ENSURE_ARG_POINTER(aScriptGlobalObject);

   *aScriptGlobalObject = mScriptGlobalObject;
   NS_IF_ADDREF(*aScriptGlobalObject);
   return NS_OK;
}

NS_IMETHODIMP 
nsDocument::SetScriptGlobalObject(nsIScriptGlobalObject *aScriptGlobalObject)
{
  // XXX HACK ALERT! If the script context owner is null, the document
  // will soon be going away. So tell our content that to lose its
  // reference to the document. This has to be done before we
  // actually set the script context owner to null so that the
  // content elements can remove references to their script objects.
  if ((nsnull == aScriptGlobalObject) && (nsnull != mRootContent)) {
    mRootContent->SetDocument(nsnull, PR_TRUE);
  }

  mScriptGlobalObject = aScriptGlobalObject;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNameSpaceManager(nsINameSpaceManager*& aManager)
{
  aManager = mNameSpaceManager;
  NS_IF_ADDREF(aManager);
  return NS_OK;
}


// Note: We don't hold a reference to the document observer; we assume
// that it has a live reference to the document.
void nsDocument::AddObserver(nsIDocumentObserver* aObserver)
{
  // XXX Make sure the observer isn't already in the list
  if (mObservers.IndexOf(aObserver) == -1) {
    mObservers.AppendElement(aObserver);
  }
}

PRBool nsDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
  // If we're in the process of destroying the document (and we're
  // informing the observers of the destruction), don't remove the
  // observers from the list. This is not a big deal, since we
  // don't hold a live reference to the observers.
  if (!mInDestructor)
    return mObservers.RemoveElement(aObserver);
  else
    return (mObservers.IndexOf(aObserver) != -1);
}

NS_IMETHODIMP 
nsDocument::BeginUpdate()
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->BeginUpdate(this);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsDocument::EndUpdate()
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->EndUpdate(this);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::BeginLoad()
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->BeginLoad(this);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::EndLoad()
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
    observer->EndLoad(this);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentChanged(nsIContent* aContent,
                           nsISupports* aSubContent)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentChanged(this, aContent, aSubContent);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentStatesChanged(nsIContent* aContent1,
                                 nsIContent* aContent2)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentStatesChanged(this, aContent1, aContent2);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsDocument::ContentAppended(nsIContent* aContainer,
                            PRInt32 aNewIndexInContainer)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentAppended(this, aContainer, aNewIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentInserted(nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentInserted(this, aContainer, aChild, aIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentReplaced(nsIContent* aContainer,
                            nsIContent* aOldChild,
                            nsIContent* aNewChild,
                            PRInt32 aIndexInContainer)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentReplaced(this, aContainer, aOldChild, aNewChild,
                              aIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ContentRemoved(nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->ContentRemoved(this, aContainer, 
                             aChild, aIndexInContainer);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AttributeChanged(nsIContent* aChild,
                             PRInt32 aNameSpaceID,
                             nsIAtom* aAttribute,
                             PRInt32 aHint)
{
  PRInt32 i;
  nsresult result = NS_OK;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    nsresult rv = observer->AttributeChanged(this, aChild, aNameSpaceID, aAttribute, aHint);
    if (NS_FAILED(rv) && NS_SUCCEEDED(result))
      result = rv;
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
  }
  return result;
}


NS_IMETHODIMP
nsDocument::StyleRuleChanged(nsIStyleSheet* aStyleSheet, nsIStyleRule* aStyleRule,
                             PRInt32 aHint)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->BeginUpdate(this);
    observer->StyleRuleChanged(this, aStyleSheet, aStyleRule, aHint);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::StyleRuleAdded(nsIStyleSheet* aStyleSheet, nsIStyleRule* aStyleRule)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->BeginUpdate(this);
    observer->StyleRuleAdded(this, aStyleSheet, aStyleRule);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::StyleRuleRemoved(nsIStyleSheet* aStyleSheet, nsIStyleRule* aStyleRule)
{
  PRInt32 i;
  // Get new value of count for every iteration in case
  // observers remove themselves during the loop.
  for (i = 0; i < mObservers.Count(); i++) {
    nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
    observer->BeginUpdate(this);
    observer->StyleRuleRemoved(this, aStyleSheet, aStyleRule);
    // Make sure that the observer didn't remove itself during the
    // notification. If it did, update our index and count.
    if (observer != (nsIDocumentObserver*)mObservers[i]) {
      i--;
    }
    else {
      observer->EndUpdate(this);
    }
  }
  return NS_OK;
}


nsresult nsDocument::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIScriptGlobalObject> global;

  if (nsnull == mScriptObject) {
    // XXX We make the (possibly erroneous) assumption that the first
    // presentation shell represents the "primary view" of the document
    // and that the JS parent chain should incorporate just that view.
    // This is done for lack of a better model when we have multiple
    // views.
    nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(0);
    if (shell) {
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));
      nsCOMPtr<nsISupports> container;
      
      res = cx->GetContainer(getter_AddRefs(container));
      if (NS_SUCCEEDED(res) && container) {
        global = do_GetInterface(container);
      }
    }
    // XXX If we can't find a view, parent to the calling context's
    // global object. This may not be right either, but we need
    // something.
    else {
      global = getter_AddRefs(aContext->GetGlobalObject());
    }
    
    if (NS_SUCCEEDED(res)) {
      res = NS_NewScriptDocument(aContext, 
                                 (nsISupports *)(nsIDOMDocument *)this, 
                                 (nsISupports *)global, 
                                 (void**)&mScriptObject);
    }
  }
  
  *aScriptObject = mScriptObject;

  return res;
}

nsresult nsDocument::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
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

  if (mProlog) {
    PRInt32 i, count = mProlog->Count();

    for (i = 0; i < count; i++) {
      nsIContent* content = (nsIContent *)mProlog->ElementAt(0);

      if (!content)
        continue;

      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));

      if (node) {
        PRUint16 nodeType;

        node->GetNodeType(&nodeType);

        if (nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE) {
          return node->QueryInterface(NS_GET_IID(nsIDOMDocumentType),
                                      (void **)aDoctype);
        }
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
  nsDOMImplementation* impl = new nsDOMImplementation(this);
  if (nsnull == impl) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return impl->QueryInterface(kIDOMDOMImplementationIID, (void**)aImplementation);
}

NS_IMETHODIMP    
nsDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
  if (nsnull == aDocumentElement) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult res = NS_ERROR_FAILURE;

  if (nsnull != mRootContent) {
    res = mRootContent->QueryInterface(kIDOMElementIID, (void**)aDocumentElement);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Element");
  }
  
  return res;
}

NS_IMETHODIMP    
nsDocument::CreateElement(const nsString& aTagName, 
                          nsIDOMElement** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}
 
NS_IMETHODIMP
nsDocument::CreateTextNode(const nsString& aData, nsIDOMText** aReturn)
{
  nsIContent* text = nsnull;
  nsresult        rv = NS_NewTextNode(&text);

  if (NS_OK == rv) {
    rv = text->QueryInterface(kIDOMTextIID, (void**)aReturn);
    (*aReturn)->AppendData(aData);
    NS_RELEASE(text);
  }

  return rv;
}

NS_IMETHODIMP    
nsDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
  return NS_NewDocumentFragment(aReturn, this);
}

NS_IMETHODIMP    
nsDocument::CreateComment(const nsString& aData, nsIDOMComment** aReturn)
{
  nsIContent* comment = nsnull;
  nsresult        rv = NS_NewCommentNode(&comment);

  if (NS_OK == rv) {
    rv = comment->QueryInterface(kIDOMCommentIID, (void**)aReturn);
    (*aReturn)->AppendData(aData);
    NS_RELEASE(comment);
  }

  return rv;
}

NS_IMETHODIMP 
nsDocument::CreateCDATASection(const nsString& aData, nsIDOMCDATASection** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::CreateProcessingInstruction(const nsString& aTarget, 
                                        const nsString& aData, 
                                        nsIDOMProcessingInstruction** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::CreateAttribute(const nsString& aName, 
                            nsIDOMAttr** aReturn)
{
  nsAutoString value;
  nsDOMAttribute* attribute;
  
  value.Truncate();
  attribute = new nsDOMAttribute(nsnull, aName, value);
  if (nsnull == attribute) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return attribute->QueryInterface(kIDOMAttrIID, (void**)aReturn);
}

NS_IMETHODIMP    
nsDocument::CreateEntityReference(const nsString& aName, 
                                  nsIDOMEntityReference** aReturn)
{
  // Should be implemented by subclass
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::GetElementsByTagName(const nsString& aTagname, 
                                 nsIDOMNodeList** aReturn)
{
  nsIAtom* nameAtom;
  PRInt32 nameSpaceId;
  nsresult result = NS_OK;

  if (nsnull != mRootContent) {
    result = mRootContent->ParseAttributeString(aTagname, nameAtom,
                                                nameSpaceId);
    if (NS_OK != result) {
      return result;
    }
  }
  else {
    nameAtom = NS_NewAtom(aTagname);
    nameSpaceId = kNameSpaceID_None;
  }

  nsContentList* list = new nsContentList(this, nameAtom, nameSpaceId);
  NS_IF_RELEASE(nameAtom);
  if (nsnull == list) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return list->QueryInterface(kIDOMNodeListIID, (void **)aReturn);
}

NS_IMETHODIMP    
nsDocument::GetStyleSheets(nsIDOMStyleSheetList** aStyleSheets)
{
  if (nsnull == mDOMStyleSheets) {
    mDOMStyleSheets = new nsDOMStyleSheetList(this);
    if (nsnull == mDOMStyleSheets) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mDOMStyleSheets);
  }

  *aStyleSheets = mDOMStyleSheets;
  NS_ADDREF(mDOMStyleSheets);

  return NS_OK;
}

NS_IMETHODIMP    
nsDocument::GetCharacterSet(nsString& aCharacterSet)
{
  return GetDocumentCharacterSet(aCharacterSet);
}

NS_IMETHODIMP    
nsDocument::CreateElementWithNameSpace(const nsString& aTagName, 
                                       const nsString& aNameSpace, 
                                       nsIDOMElement** aReturn)
{
  *aReturn = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::CreateRange(nsIDOMRange** aReturn)
{
  return NS_NewRange(aReturn);
}


NS_IMETHODIMP
nsDocument::GetDefaultView(nsIDOMAbstractView** aDefaultView)
{
  NS_ENSURE_ARG_POINTER(aDefaultView);

  *aDefaultView = nsnull;

  return NS_OK;
}


nsresult
nsDocument::GetPixelDimensions(nsIPresShell* aShell,
                               PRInt32* aWidth,
                               PRInt32* aHeight)
{
  nsresult result = NS_OK;
  nsSize size;
  nsIFrame* frame;
  
  result = FlushPendingNotifications();
  if (NS_FAILED(result)) {
    return result;
  }

  result = aShell->GetPrimaryFrameFor(mRootContent, &frame);
  if (NS_SUCCEEDED(result) && frame) {
    nsIView*                  view;
    nsCOMPtr<nsIPresContext>  presContext;

    aShell->GetPresContext(getter_AddRefs(presContext));
    result = frame->GetView(presContext, &view);
    if (NS_SUCCEEDED(result)) {
      // If we have a view check if it's scrollable. If not,
      // just use the view size itself
      if (view) {
        nsIScrollableView* scrollableView;
        
        if (NS_SUCCEEDED(view->QueryInterface(NS_GET_IID(nsIScrollableView), (void**)&scrollableView))) {
          scrollableView->GetScrolledView(view);
        }

        result = view->GetDimensions(&size.width, &size.height);
      }
      // If we don't have a view, use the frame size
      else {
        result = frame->GetSize(size);
      }
    }

    // Convert from twips to pixels
    if (NS_SUCCEEDED(result)) {
      nsCOMPtr<nsIPresContext> context;
      
      result = aShell->GetPresContext(getter_AddRefs(context));
      
      if (NS_SUCCEEDED(result)) {
        float scale;
        context->GetTwipsToPixels(&scale);
        
        *aWidth = NSTwipsToIntPixels(size.width, scale);
        *aHeight = NSTwipsToIntPixels(size.height, scale);
      }
    }
  }
  else {
    *aWidth = 0;
    *aHeight = 0;
  }

  return result;
}

NS_IMETHODIMP    
nsDocument::GetWidth(PRInt32* aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  nsCOMPtr<nsIPresShell> shell;
  nsresult result = NS_OK;

  // We make the assumption that the first presentation shell
  // is the one for which we need information.
  shell = getter_AddRefs(GetShellAt(0));
  if (shell) {
    PRInt32 width, height;

    result = GetPixelDimensions(shell, &width, &height);
    *aWidth = width;
  } else
    *aWidth = 0;

  return result;
}

NS_IMETHODIMP    
nsDocument::GetHeight(PRInt32* aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  nsCOMPtr<nsIPresShell> shell;
  nsresult result = NS_OK;

  // We make the assumption that the first presentation shell
  // is the one for which we need information.
  shell = getter_AddRefs(GetShellAt(0));
  if (shell) {
    PRInt32 width, height;

    result = GetPixelDimensions(shell, &width, &height);
    *aHeight = height;
  } else
    *aHeight = 0;

  return result;
}

NS_IMETHODIMP
nsDocument::Load (const nsString& aUrl, const nsString& aMimeType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//
// nsIDOMNode methods
//
NS_IMETHODIMP    
nsDocument::GetNodeName(nsString& aNodeName)
{
  aNodeName.AssignWithConversion("#document");
  return NS_OK;
}

NS_IMETHODIMP    
nsDocument::GetNodeValue(nsString& aNodeValue)
{
  aNodeValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP    
nsDocument::SetNodeValue(const nsString& aNodeValue)
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
  if (nsnull == mChildNodes) {
    mChildNodes = new nsDocumentChildNodes(this);
    if (nsnull == mChildNodes) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mChildNodes);
  }

  return mChildNodes->QueryInterface(kIDOMNodeListIID, (void**)aChildNodes);
}

NS_IMETHODIMP    
nsDocument::HasChildNodes(PRBool* aHasChildNodes)
{
  *aHasChildNodes = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP    
nsDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
  nsresult result = NS_OK;

  if ((nsnull != mProlog) && (0 != mProlog->Count())) {
    nsIContent* content;
    content = (nsIContent *)mProlog->ElementAt(0);

    if (nsnull != content) {
      result = content->QueryInterface(kIDOMNodeIID, (void**)aFirstChild);
    }
  }
  else {
    nsIDOMElement* element;
    result = GetDocumentElement(&element);
    if (NS_OK == result) {
      result = element->QueryInterface(kIDOMNodeIID, (void**)aFirstChild);
      NS_RELEASE(element);
    }
  }

  return result;
}

NS_IMETHODIMP    
nsDocument::GetLastChild(nsIDOMNode** aLastChild)
{
  nsresult result = NS_OK;

  if ((nsnull != mEpilog) && (0 != mEpilog->Count())) {
    nsIContent* content;
    content = (nsIContent *)mEpilog->ElementAt(mEpilog->Count()-1);
    if (nsnull != content) {
      result = content->QueryInterface(kIDOMNodeIID, (void**)aLastChild);
    }
  }
  else {
    nsIDOMElement* element;
    result = GetDocumentElement(&element);
    if (NS_OK == result) {
      result = element->QueryInterface(kIDOMNodeIID, (void**)aLastChild);
      NS_RELEASE(element);
    }
  }

  return result;
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
nsDocument::GetNamespaceURI(nsString& aNamespaceURI)
{ 
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetPrefix(nsString& aPrefix)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::SetPrefix(const nsString& aPrefix)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetLocalName(nsString& aLocalName)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
  NS_ASSERTION(nsnull != aNewChild, "null ptr");
  nsresult result = NS_OK;
  PRInt32 index;
  PRUint16 nodeType;
  nsIContent *content, *refContent = nsnull;

  if (nsnull == aNewChild) {
    return NS_ERROR_NULL_POINTER;
  }

  aNewChild->GetNodeType(&nodeType);
  if ((COMMENT_NODE != nodeType) &&
      (PROCESSING_INSTRUCTION_NODE != nodeType) &&
      (DOCUMENT_TYPE_NODE != nodeType)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  result = aNewChild->QueryInterface(kIContentIID, (void**)&content);
  if (NS_OK != result) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if (nsnull == aRefChild) {
    if ((!mProlog || (mProlog && mProlog->Count())) && mRootContent) {
      AppendToEpilog(content);
    } else if (nodeType != ELEMENT_NODE) {
      AppendToProlog(content);
    } else {
      return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
    }
  }
  else {
    result = aRefChild->QueryInterface(kIContentIID, (void**)&refContent);
    if (NS_OK != result) {
      NS_RELEASE(content);
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    if ((nsnull != mProlog) && (0 != mProlog->Count())) {
      index = mProlog->IndexOf(refContent);
      if (-1 != index) {
        mProlog->InsertElementAt(content, index);
        NS_ADDREF(content);
      }
    }

    if (refContent == mRootContent) {
      AppendToProlog(content);
    }
    else if ((nsnull != mEpilog) && (0 != mEpilog->Count())) {
      index = mEpilog->IndexOf(refContent);
      if (-1 != index) {
        mEpilog->InsertElementAt(content, index);
        NS_ADDREF(content);
      }
    }
    NS_RELEASE(refContent);
  }

  if (NS_OK == result) {
    content->SetDocument(this, PR_TRUE);
    *aReturn = aNewChild;
    NS_ADDREF(aNewChild);
  }
  else {
    *aReturn = nsnull;
  }

  NS_RELEASE(content);  

  return result;
}

NS_IMETHODIMP    
nsDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  NS_ASSERTION(((nsnull != aNewChild) && (nsnull != aOldChild)), "null ptr");
  nsresult result = NS_OK;
  PRInt32 index;
  PRUint16 nodeType;
  nsIContent *content, *refContent;
  
  if ((nsnull == aNewChild) || (nsnull == aOldChild)) {
    return NS_ERROR_NULL_POINTER;
  }

  aNewChild->GetNodeType(&nodeType);
  if ((COMMENT_NODE != nodeType) && (PROCESSING_INSTRUCTION_NODE != nodeType)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  result = aNewChild->QueryInterface(kIContentIID, (void**)&content);
  if (NS_OK != result) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  result = aOldChild->QueryInterface(kIContentIID, (void**)&refContent);
  if (NS_OK != result) {
    NS_RELEASE(content);
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if ((nsnull != mProlog) && (0 != mProlog->Count())) {
    index = mProlog->IndexOf(refContent);
    if (-1 != index) {
      nsIContent* oldContent;
      oldContent = (nsIContent*)mProlog->ElementAt(index);
      NS_RELEASE(oldContent);
      mProlog->ReplaceElementAt(content, index);
      NS_ADDREF(content);
    }
  }

  if (refContent == mRootContent) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  else if ((nsnull != mEpilog) && (0 != mEpilog->Count())) {
    index = mEpilog->IndexOf(refContent);
    if (-1 != index) {
      nsIContent* oldContent;
      oldContent = (nsIContent*)mEpilog->ElementAt(index);
      NS_RELEASE(oldContent);
      mEpilog->ReplaceElementAt(content, index);
      NS_ADDREF(content);
    }
  }

  if (NS_OK == result) {
    content->SetDocument(this, PR_TRUE);
    refContent->SetDocument(nsnull, PR_TRUE);
    *aReturn = aNewChild;
    NS_ADDREF(aNewChild);
  }
  else {
    *aReturn = nsnull;
  }

  NS_RELEASE(content);
  NS_RELEASE(refContent);

  return result;
}

NS_IMETHODIMP    
nsDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  NS_ASSERTION(nsnull != aOldChild, "null ptr");
  nsresult result = NS_OK;
  PRInt32 index;
  nsIContent *content;
  
  if (nsnull == aOldChild) {
    return NS_ERROR_NULL_POINTER;
  }

  result = aOldChild->QueryInterface(kIContentIID, (void**)&content);
  if (NS_OK != result) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if ((nsnull != mProlog) && (0 != mProlog->Count())) {
    index = mProlog->IndexOf(content);
    if (-1 != index) {
      // Don't drop reference count since we're going
      // to return this element anyway.
      mProlog->RemoveElementAt(index);
    }
  }

  if (content == mRootContent) {
    result = NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  else if ((nsnull != mEpilog) && (0 != mEpilog->Count())) {
    index = mEpilog->IndexOf(content);
    if (-1 != index) {
      mEpilog->RemoveElementAt(index);
    }
  }

  if (NS_OK == result) {
    content->SetDocument(nsnull, PR_TRUE);
    *aReturn = aOldChild;
  }
  else {
    *aReturn = nsnull;
  }

  NS_RELEASE(content);
  
  return result;
}

NS_IMETHODIMP    
nsDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return InsertBefore(aNewChild, nsnull, aReturn);
}

NS_IMETHODIMP    
nsDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  // We don't allow cloning of a document
  *aReturn = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::Normalize()
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::Supports(const nsString& aFeature, const nsString& aVersion,
                     PRBool* aReturn)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nsnull;
  return NS_OK;
}

nsresult nsDocument::GetListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  if (nsnull != mListenerManager) {
    return mListenerManager->QueryInterface(kIEventListenerManagerIID, (void**) aInstancePtrResult);
  }
  if (NS_OK == GetNewListenerManager(aInstancePtrResult)) {
    mListenerManager = *aInstancePtrResult;
    NS_ADDREF(mListenerManager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsDocument::GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  return NS_NewEventListenerManager(aInstancePtrResult);
} 

nsresult nsDocument::HandleEvent(nsIDOMEvent *aEvent)
{
  return NS_ERROR_FAILURE;
} 

nsresult nsDocument::HandleDOMEvent(nsIPresContext* aPresContext, 
                                    nsEvent* aEvent, 
                                    nsIDOMEvent** aDOMEvent,
                                    PRUint32 aFlags,
                                    nsEventStatus* aEventStatus)
{
  nsresult mRet = NS_OK;
  nsIDOMEvent* mDOMEvent = nsnull;

  if (NS_EVENT_FLAG_INIT == aFlags) {
    aDOMEvent = &mDOMEvent;
    aEvent->flags = NS_EVENT_FLAG_NONE;
  }
  
  //Capturing stage
  if (NS_EVENT_FLAG_BUBBLE != aFlags && nsnull != mScriptGlobalObject) {
    mScriptGlobalObject->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
  }
  
  //Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
    aEvent->flags |= aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
    aEvent->flags &= ~aFlags;
  }

  //Bubbling stage
  if (NS_EVENT_FLAG_CAPTURE != aFlags && nsnull != mScriptGlobalObject) {
    mScriptGlobalObject->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_BUBBLE, aEventStatus);
  }

  if (NS_EVENT_FLAG_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event, release here.
    if (nsnull != *aDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (0 != rc) {
      //Okay, so someone in the DOM loop (a listener, JS object) still has a ref to the DOM Event but
      //the internal data hasn't been malloc'd.  Force a copy of the data here so the DOM Event is still valid.
        nsIPrivateDOMEvent *mPrivateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(kIPrivateDOMEventIID, (void**)&mPrivateEvent)) {
          mPrivateEvent->DuplicatePrivateData();
          NS_RELEASE(mPrivateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }

  return mRet;
}

nsresult nsDocument::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsDocument::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  if (nsnull != mListenerManager) {
    mListenerManager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsDocument::AddEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                      PRBool aUseCapture)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult nsDocument::RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                         PRBool aUseCapture)
{
  if (nsnull != mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

PRBool    nsDocument::AddProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}

PRBool    nsDocument::DeleteProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}

PRBool    nsDocument::GetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  PRBool result = PR_TRUE;

  if (JSVAL_IS_STRING(aID) && 
      PL_strcmp("location", JS_GetStringBytes(JS_ValueToString(aContext, aID))) == 0) {
    if (mScriptGlobalObject) {
      nsCOMPtr<nsIJSScriptObject> window(do_QueryInterface(mScriptGlobalObject));
      if(window) {
        result = window->GetProperty(aContext, aObj, aID, aVp);
      }
      else {
        result = PR_FALSE;
      }
    }
  }

  return result;
}

PRBool    nsDocument::SetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  PRBool result = PR_TRUE;

  if (JS_TypeOfValue(aContext, *aVp) == JSTYPE_FUNCTION && JSVAL_IS_STRING(aID)) {
    nsAutoString mPropName, mPrefix;
    mPropName.Assign(JS_GetStringChars(JS_ValueToString(aContext, aID)));
    if (mPropName.Length() > 2)
      mPrefix.Assign(mPropName.GetUnicode(), 2);
    if (mPrefix.EqualsWithConversion("on")) {
      nsCOMPtr<nsIAtom> atom = getter_AddRefs(NS_NewAtom(mPropName));
      nsIEventListenerManager *mManager = nsnull;

      if (atom.get() == nsLayoutAtoms::onmousedown || atom.get() == nsLayoutAtoms::onmouseup || atom.get() ==  nsLayoutAtoms::onclick ||
         atom.get() == nsLayoutAtoms::onmouseover || atom.get() == nsLayoutAtoms::onmouseout) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMMouseListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onkeydown || atom.get() == nsLayoutAtoms::onkeyup || atom.get() == nsLayoutAtoms::onkeypress) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMKeyListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onmousemove) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMMouseMotionListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onfocus || atom.get() == nsLayoutAtoms::onblur) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMFocusListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onsubmit || atom.get() == nsLayoutAtoms::onreset || atom.get() == nsLayoutAtoms::onchange ||
               atom.get() == nsLayoutAtoms::onselect) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMFormListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onload || atom.get() == nsLayoutAtoms::onunload || atom.get() == nsLayoutAtoms::onabort ||
               atom.get() == nsLayoutAtoms::onerror) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this, atom, kIDOMLoadListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      else if (atom.get() == nsLayoutAtoms::onpaint) {
        if (NS_OK == GetListenerManager(&mManager)) {
          nsCOMPtr<nsIScriptContext> mScriptCX;
          if(NS_FAILED(nsLayoutUtils::GetStaticScriptContext(aContext, (JSObject*)mScriptObject, getter_AddRefs(mScriptCX))) ||
             NS_OK != mManager->RegisterScriptEventListener(mScriptCX, this,
                                                             atom, kIDOMPaintListenerIID)) {
            NS_RELEASE(mManager);
            return PR_FALSE;
          }
        }
      }
      NS_IF_RELEASE(mManager);
    }
  }
  else if (JSVAL_IS_STRING(aID) && 
      PL_strcmp("location", JS_GetStringBytes(JS_ValueToString(aContext, aID))) == 0) {
    if (mScriptGlobalObject) {
      nsCOMPtr<nsIJSScriptObject> window(do_QueryInterface(mScriptGlobalObject));
      if(window) {
        result = window->SetProperty(aContext, aObj, aID, aVp);
      }
      else {
        result = PR_FALSE;
      }
    }
  }

  return result;
}

PRBool    nsDocument::EnumerateProperty(JSContext *aContext, JSObject *aObj)
{
  return PR_TRUE;
}

PRBool    nsDocument::Resolve(JSContext *aContext, JSObject *aObj, jsval aID)
{
  return PR_TRUE;
}

PRBool    nsDocument::Convert(JSContext *aContext, JSObject *aObj, jsval aID)
{
  return PR_TRUE;
}

void      nsDocument::Finalize(JSContext *aContext, JSObject *aObj)
{
}

/**
  * Finds text in content
 */
NS_IMETHODIMP nsDocument::FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound)
{
  aIsFound = PR_FALSE;
  return NS_ERROR_FAILURE;
}



void nsDocument::BeginConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  nsIContent* content = nsnull;
  nsresult    isContent = aNode->QueryInterface(kIContentIID, (void**)&content);
  PRBool      isSynthetic = PR_TRUE;

  // Begin Conversion
  if (NS_OK == isContent) 
  {
    content->IsSynthetic(isSynthetic);
    if (PR_FALSE == isSynthetic)
    {
      content->BeginConvertToXIF(aConverter);
      content->ConvertContentToXIF(aConverter);
    }
    NS_RELEASE(content);
  }
}

void nsDocument::ConvertChildrenToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  // Iterate through the children, convertion child nodes
  nsresult result = NS_OK;
  nsIDOMNode* child = nsnull;
  result = aNode->GetFirstChild(&child);
    
  while ((result == NS_OK) && (child != nsnull))
  { 
    nsIDOMNode* temp = child;
    result=ToXIF(aConverter,child);    
    result = child->GetNextSibling(&child);
    NS_RELEASE(temp);
  }
}

void nsDocument::FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  nsIContent* content = nsnull;
  nsresult    isContent = aNode->QueryInterface(kIContentIID, (void**)&content);
  PRBool      isSynthetic = PR_TRUE;

  if (NS_OK == isContent) 
  {
    content->IsSynthetic(isSynthetic);
    if (PR_FALSE == isSynthetic)
      content->FinishConvertToXIF(aConverter);
    NS_RELEASE(content);
  }
}


NS_IMETHODIMP
nsDocument::ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
  nsresult result=NS_OK;
  nsIDOMSelection* sel = aConverter.GetSelection();
  if (sel != nsnull)
  {
    nsIContent* content = nsnull;
    result=aNode->QueryInterface(kIContentIID, (void**)&content);

    if (NS_SUCCEEDED(result) && content)
    {
      PRBool  isInSelection = IsInSelection(sel,content);

      if (isInSelection == PR_TRUE)
      {
        BeginConvertToXIF(aConverter,aNode);
        ConvertChildrenToXIF(aConverter,aNode);
        FinishConvertToXIF(aConverter,aNode);
      }
      else
      {
        ConvertChildrenToXIF(aConverter,aNode);
      }
      NS_RELEASE(content);
    }
  }
  else
  {
    BeginConvertToXIF(aConverter,aNode);
    ConvertChildrenToXIF(aConverter,aNode);
    FinishConvertToXIF(aConverter,aNode);
  }
  return result;
}

NS_IMETHODIMP
nsDocument::CreateXIF(nsString & aBuffer, nsIDOMSelection* aSelection)
{
    nsresult result=NS_OK;

    nsXIFConverter converter(aBuffer);

    converter.SetSelection(aSelection);

    converter.AddStartTag( NS_ConvertToString("section") ); 
    converter.AddStartTag( NS_ConvertToString("section_head") );

    converter.BeginStartTag( NS_ConvertToString("document_info") );
    converter.AddAttribute(NS_ConvertToString("charset"),mCharacterSet);
    nsCOMPtr<nsIURI> uri (getter_AddRefs(GetDocumentURL()));
    if (uri)
    {
      char* spec = 0;
      if (NS_SUCCEEDED(uri->GetSpec(&spec)) && spec)
      {
        converter.AddAttribute(NS_ConvertToString("uri"), NS_ConvertToString(spec));
        Recycle(spec);
      }
    }
    converter.FinishStartTag(NS_ConvertToString("document_info"),PR_TRUE,PR_TRUE);

    converter.AddEndTag(NS_ConvertToString("section_head"));
    converter.AddStartTag(NS_ConvertToString("section_body"));

    nsCOMPtr<nsIDOMDocumentType> doctype;
    GetDoctype(getter_AddRefs(doctype));
    if(doctype) {
      nsAutoString tmpStr, docTypeStr;

      doctype->GetName(tmpStr);

      if (tmpStr.Length()) {
        docTypeStr.AppendWithConversion("DOCTYPE ");
        docTypeStr.Append(tmpStr);

        doctype->GetPublicId(tmpStr);
        if (tmpStr.Length()) {
          docTypeStr.AppendWithConversion(" PUBLIC \"");
          docTypeStr.Append(tmpStr);
          docTypeStr.AppendWithConversion('"');
        }

        doctype->GetSystemId(tmpStr);
        if (tmpStr.Length()) {
          docTypeStr.AppendWithConversion(" SYSTEM \"");
          docTypeStr.Append(tmpStr);
          docTypeStr.AppendWithConversion('"');
        }

        doctype->GetInternalSubset(tmpStr);
        if (tmpStr.Length()) {
          docTypeStr.AppendWithConversion(" [\n");
          docTypeStr.Append(tmpStr);
          docTypeStr.AppendWithConversion("\n]");
        }
      }

      if (docTypeStr.Length())
        converter.AddMarkupDeclaration(docTypeStr);
    }

    nsIDOMElement* root = nsnull;
    result=GetDocumentElement(&root);
    if (NS_SUCCEEDED(result))
    {  
  #if 1
      result=ToXIF(converter,root);
  #else
     if(NS_SUCCEEDED(rv)) {
        // Make a content iterator over the selection:
        nsCOMPtr<nsIContentIterator> iter;
        result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                                          NS_GET_IID(nsIContentIterator), 
                                                          getter_AddRefs(iter));
        if ((NS_SUCCEEDED(result)) && iter)
        {
          nsCOMPtr<nsIContent> rootContent (do_QueryInterface(root));
          if (rootContent)
          {
            iter->Init(rootContent);
            // loop through the content iterator for each content node
            while (NS_COMFALSE == iter->IsDone())
            {
              nsCOMPtr<nsIContent> content;
              res = iter->CurrentNode(getter_AddRefs(content));
              if (NS_FAILED(res))
                break;
              //content->BeginConvertToXIF(converter);
              content->ConvertContentToXIF(converter);
              //content->FinishConvertToXIF(converter);
    #if 0
              nsCOMPtr<nsIDOMNode> node (do_QueryInterface(content));
              if (node)
                ToXIF(converter, node);
    #endif
              iter->Next();
            }
          }
        }
     }
    #endif
        NS_RELEASE(root);
     }
  converter.AddEndTag(NS_ConvertToString("section_body"));
  converter.AddEndTag(NS_ConvertToString("section"));
  return result;
}

static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

#if 0
nsresult
nsDocument::OutputDocumentAs(nsIOutputStream* aStream,
                             nsIDOMSelection* selection,
                             EOutputFormat aOutputFormat,
                             const nsString& aCharset,
                             PRUint32 aFlags)
{
  nsresult  rv = NS_OK;
  
  nsAutoString charsetStr; charsetStr.Assign(aCharset);
  if (charsetStr.Length() == 0)
  {
	  rv = GetDocumentCharacterSet(charsetStr);
	  if(NS_FAILED(rv)) {
	     charsetStr.AssignWithConversion("ISO-8859-1"); 
	  }
  }

  nsCOMPtr<nsIParser>  parser;
  rv = nsComponentManager::CreateInstance(kCParserCID, 
                                             nsnull, 
                                             kCParserIID, 
                                             getter_AddRefs(parser));
  if (NS_SUCCEEDED(rv))
  {
 	  nsString buffer;
	  rv=CreateXIF(buffer, selection);			// if selection is null, ignores the selection

    if(NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIHTMLContentSink> sink;

      switch (aOutputFormat)
      {
      case eOutputText:
        rv = NS_New_HTMLToTXT_SinkStream(getter_AddRefs(sink), aStream, &charsetStr, 0);
        break;
      case eOutputHTML:
        rv = NS_New_HTML_ContentSinkStream(getter_AddRefs(sink), aStream, &charsetStr, 0);
        break;
      default:
        rv = NS_ERROR_INVALID_ARG;
      }

      if (NS_SUCCEEDED(rv) && sink)
      {
        parser->SetContentSink(sink);
        parser->SetDocumentCharset(charsetStr, kCharsetFromPreviousLoading);
        nsCOMPtr<nsIDTD>  dtd;
        rv = NS_NewXIFDTD(getter_AddRefs(dtd));
        if (NS_SUCCEEDED(rv) && dtd)
        {
          parser->RegisterDTD(dtd);
          parser->Parse(buffer, 0, NS_ConvertToString("text/xif"), PR_FALSE, PR_TRUE);           
        }
      }
    }
  }
  return rv;
}
#endif

NS_IMETHODIMP
nsDocument::InitDiskDocument(nsFileSpec* aFileSpec)
{
  mFileSpec = nsnull;
 
  if (aFileSpec)
  {
    mFileSpec = new nsFileSpec(*aFileSpec);
    if (!mFileSpec)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  
  mModCount = 0;
	return NS_OK;
}



NS_IMETHODIMP
nsDocument::SaveFile(nsFileSpec*     aFileSpec,
                     PRBool          aReplaceExisting,
                     PRBool          aSaveCopy,
                     const nsString& aFormatType,
                     const nsString& aSaveCharset,
                     PRUint32        aFlags)
{
  if (!aFileSpec)
    return NS_ERROR_NULL_POINTER;
    
  nsresult  rv = NS_OK;

  // if we're not replacing an existing file but the file
  // exists, somethine is wrong
  if (!aReplaceExisting && aFileSpec->Exists())
    return NS_ERROR_FAILURE;				// where are the file I/O errors?
  
  nsOutputFileStream		stream(*aFileSpec);
  // if the stream didn't open, something went wrong
  if (!stream.is_open())
    return NS_BASE_STREAM_CLOSED;

  // Get a document encoder instance:
  nsCOMPtr<nsIDocumentEncoder> encoder;
  char* progid = (char *)nsAllocator::Alloc(strlen(NS_DOC_ENCODER_PROGID_BASE)
                                            + aFormatType.Length() + 1);
  if (! progid)
    return NS_ERROR_OUT_OF_MEMORY;
  strcpy(progid, NS_DOC_ENCODER_PROGID_BASE);
  char* type = aFormatType.ToNewCString();
  strcat(progid, type);
  nsCRT::free(type);
  rv = nsComponentManager::CreateInstance(progid,
                                          nsnull,
                                          NS_GET_IID(nsIDocumentEncoder),
                                          getter_AddRefs(encoder));
  nsCRT::free(progid);
  if (NS_FAILED(rv))
    return rv;

  rv = encoder->Init(this, aFormatType, aFlags);
  if (NS_FAILED(rv))
    return rv;

  nsAutoString charsetStr = aSaveCharset;
  if (charsetStr.Length() == 0)
  {
	  rv = GetDocumentCharacterSet(charsetStr);
	  if(NS_FAILED(rv)) {
	     charsetStr.AssignWithConversion("ISO-8859-1"); 
	  }
  }
  encoder->SetCharset(charsetStr);

  rv = encoder->EncodeToStream(stream.GetIStream());

  if (NS_SUCCEEDED(rv))
  {
    // if everything went OK and we're not just saving off a copy,
    // store the new fileSpec in the doc
    if (!aSaveCopy)
    {
      delete mFileSpec;
      mFileSpec = new nsFileSpec(*aFileSpec);
      if (!mFileSpec)
        return NS_ERROR_OUT_OF_MEMORY;
      
      // and mark the document as clean
      ResetModCount();
    }
  }
  
  return rv;
}

NS_IMETHODIMP
nsDocument::GetFileSpec(nsFileSpec& aFileSpec)
{
  if (mFileSpec)
  {
    aFileSpec = *mFileSpec;
    return NS_OK;
  }
  
  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP 
nsDocument::FlushPendingNotifications()
{
  PRInt32 i, count = mPresShells.Count();

  for (i = 0; i < count; i++) {
    nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);
    if (shell) {
      shell->FlushPendingNotifications();
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
  nsresult rv;
  if (!mBindingManager) {
    mBindingManager = do_CreateInstance("component://netscape/xbl/binding-manager", &rv);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
  }

  *aResult = mBindingManager;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetModCount(PRInt32 *outModCount)
{
  if (!outModCount)
  	return NS_ERROR_NULL_POINTER;
  	
 *outModCount = mModCount;
 return NS_OK;
}


NS_IMETHODIMP
nsDocument::ResetModCount()
{
  mModCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IncrementModCount(PRInt32 aNumMods)
{
  mModCount += aNumMods;
  //NS_ASSERTION(mModCount >= 0, "Modification count went negative");
  return NS_OK;
}

//
// FindContent does a depth-first search from aStartNode
// and returns the first of aTest1 or aTest2 which it finds.
// I think.
//
nsIContent* nsDocument::FindContent(const nsIContent* aStartNode,
                                    const nsIContent* aTest1, 
                                    const nsIContent* aTest2) const
{
  PRInt32       count;
  aStartNode->ChildCount(count);
  PRInt32       index;

  for(index = 0; index < count;index++)
  {
    nsIContent* child;
    aStartNode->ChildAt(index, child);
    nsIContent* content = FindContent(child,aTest1,aTest2);
    if (content != nsnull) {
      NS_IF_RELEASE(child);
      return content;
    }
    if (child == aTest1 || child == aTest2) {
      NS_IF_RELEASE(content);
      return child;
    }
    NS_IF_RELEASE(child);
    NS_IF_RELEASE(content);
  }
  return nsnull;
}


/**
 *  Determines if the content is found within the selection
 *  
 *  @update  gpk 1/8/99
 *  @param   param -- description
 *  @param   param -- description
 *  @return  PR_TRUE if the content is found within the selection
 */
PRBool
nsDocument::IsInSelection(nsIDOMSelection* aSelection, const nsIContent* aContent) const
{
  PRBool aYes = PR_FALSE;
  nsCOMPtr<nsIDOMNode> node (do_QueryInterface((nsIContent *) aContent));
  aSelection->ContainsNode(node, PR_FALSE, &aYes);
  return aYes;
}

nsIContent* nsDocument::GetPrevContent(const nsIContent *aContent) const
{
  nsIContent* result = nsnull;
 
  // Look at previous sibling

  if (nsnull != aContent)
  {
    nsIContent* parent;
    aContent->GetParent(parent);
    if (parent != nsnull && parent != mRootContent)
    {
      PRInt32  index;
      parent->IndexOf((nsIContent*)aContent, index);
      if (index > 0)
        parent->ChildAt(index-1, result);
      else
        result = GetPrevContent(parent);
    }
    NS_IF_RELEASE(parent);
  }
  return result;
}

nsIContent* nsDocument::GetNextContent(const nsIContent *aContent) const
{
  nsIContent* result = nsnull;
   
  if (nsnull != aContent)
  {
    // Look at next sibling
    nsIContent* parent;
    aContent->GetParent(parent);
    if (parent != nsnull && parent != mRootContent)
    {
      PRInt32     index;
      parent->IndexOf((nsIContent*)aContent, index);
      PRInt32     count;
      parent->ChildCount(count);
      if (index+1 < count) {
        parent->ChildAt(index+1, result);
        // Get first child down the tree
        for (;;) {
          PRInt32 n;
          result->ChildCount(n);
          if (n <= 0) {
            break;
          }
          nsIContent * old = result;
          old->ChildAt(0, result);
          NS_RELEASE(old);
          result->ChildCount(n);
        }
      } else {
        result = GetNextContent(parent);
      }
    }
    NS_IF_RELEASE(parent);
  }
  return result;
}


