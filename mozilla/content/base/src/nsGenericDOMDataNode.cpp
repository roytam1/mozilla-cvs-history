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
#include "nsGenericDOMDataNode.h"
#include "nsGenericElement.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIDocument.h"
#include "nsIDOMRange.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsXIFConverter.h"
#include "nsRange.h"
#include "nsTextContentChangeData.h"
#include "nsIDOMSelection.h"
#include "nsIEnumerator.h"


#include "nsCRT.h"
#include "nsIEventStateManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsISizeOfHandler.h"
#include "nsDOMEvent.h"
#include "nsIDOMText.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIScriptGlobalObject.h"
#include "prprf.h"
#include "nsCOMPtr.h"

// XXX share all id's in this dir

NS_DEFINE_IID(kIDOMCharacterDataIID, NS_IDOMCHARACTERDATA_IID);

static NS_DEFINE_IID(kIPrivateDOMEventIID, NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIEnumeratorIID, NS_IENUMERATOR_IID);
static NS_DEFINE_IID(kIDOMDocumentIID, NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kITextContentIID, NS_ITEXT_CONTENT_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);

//----------------------------------------------------------------------

nsGenericDOMDataNode::nsGenericDOMDataNode()
  : mText()
{
  mDocument = nsnull;
  mParent = nsnull;
  mContent = nsnull;
  mScriptObject = nsnull;
  mListenerManager = nsnull;
  mRangeList = nsnull;
  mCapturer = nsnull;
}

nsGenericDOMDataNode::~nsGenericDOMDataNode()
{
  NS_IF_RELEASE(mListenerManager);
  delete mRangeList;
}

void
nsGenericDOMDataNode::Init(nsIContent* aOuterContentObject)
{
  NS_ASSERTION((nsnull == mContent) && (nsnull != aOuterContentObject),
               "null ptr");
  mContent = aOuterContentObject;
}

nsresult
nsGenericDOMDataNode::GetNodeValue(nsString& aNodeValue)
{
  return GetData(aNodeValue);
}

nsresult
nsGenericDOMDataNode::SetNodeValue(const nsString& aNodeValue)
{
  return SetData(aNodeValue);
}

nsresult
nsGenericDOMDataNode::GetParentNode(nsIDOMNode** aParentNode)
{
  nsresult res = NS_OK;

  if (nsnull != mParent) {
    res = mParent->QueryInterface(kIDOMNodeIID, (void**)aParentNode);
    NS_ASSERTION(NS_OK == res, "Must be a DOM Node");
  }
  else if (nsnull == mDocument) {
    *aParentNode = nsnull;
  } 
  else {
    // If we don't have a parent, but we're in the document, we must
    // be the root node of the document. The DOM says that the root
    // is the document.
    res = mDocument->QueryInterface(kIDOMNodeIID, (void**)aParentNode);
  }

  return res;
}

nsresult
nsGenericDOMDataNode::GetPreviousSibling(nsIDOMNode** aPrevSibling)
{
  nsIContent* sibling = nsnull;
  nsresult result = NS_OK;

  if (nsnull != mParent) {
    PRInt32 pos;
    mParent->IndexOf(mContent, pos);
    if (pos > -1 ) {
      mParent->ChildAt(--pos, sibling);
    }
  }
  else if (nsnull != mDocument) {
    // Nodes that are just below the document (their parent is the
    // document) need to go to the document to find their next sibling.
    PRInt32 pos;
    mDocument->IndexOf(mContent, pos);
    if (pos > -1 ) {
      mDocument->ChildAt(--pos, sibling);
    }    
  }

  if (nsnull != sibling) {
    result = sibling->QueryInterface(kIDOMNodeIID,(void**)aPrevSibling);
    NS_ASSERTION(NS_OK == result, "Must be a DOM Node");
    NS_RELEASE(sibling); // balance the AddRef in ChildAt()
  }
  else {
    *aPrevSibling = nsnull;
  }
  
  return result;
}

nsresult
nsGenericDOMDataNode::GetNextSibling(nsIDOMNode** aNextSibling)
{
  nsIContent* sibling = nsnull;
  nsresult result = NS_OK;

  if (nsnull != mParent) {
    PRInt32 pos;
    mParent->IndexOf(mContent, pos);
    if (pos > -1 ) {
      mParent->ChildAt(++pos, sibling);
    }
  }
  else if (nsnull != mDocument) {
    // Nodes that are just below the document (their parent is the
    // document) need to go to the document to find their next sibling.
    PRInt32 pos;
    mDocument->IndexOf(mContent, pos);
    if (pos > -1 ) {
      mDocument->ChildAt(++pos, sibling);
    }    
  }

  if (nsnull != sibling) {
    result = sibling->QueryInterface(kIDOMNodeIID,(void**)aNextSibling);
    NS_ASSERTION(NS_OK == result, "Must be a DOM Node");
    NS_RELEASE(sibling); // balance the AddRef in ChildAt()
  }
  else {
    *aNextSibling = nsnull;
  }
  
  return result;
}

nsresult    
nsGenericDOMDataNode::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  // XXX Since we believe this won't be done very often, we won't
  // burn another slot in the data node and just create a new
  // (empty) childNodes list every time we're asked.
  nsChildContentList* list = new nsChildContentList(nsnull);
  if (nsnull == list) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return list->QueryInterface(kIDOMNodeListIID, (void**)aChildNodes);
}

nsresult    
nsGenericDOMDataNode::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  // XXX Actually the owner document is the document in whose context
  // the node has been created. We should be able to get at it
  // whether or not we are attached to the document.
  if (nsnull != mDocument) {
    return mDocument->QueryInterface(kIDOMDocumentIID, (void **)aOwnerDocument);
  }
  else {
    *aOwnerDocument = nsnull;
    return NS_OK;
  }
}

#if 0
nsresult
nsGenericDOMDataNode::Equals(nsIDOMNode* aNode, PRBool aDeep, PRBool* aReturn)
{
  *aReturn = PR_FALSE;
  PRInt32 nt1, nt2;
  GetNodeType(&nt1);
  aNode->GetNodeType(&nt2);
  if (nt1 != nt2) {
    return NS_OK;
  }
  return NS_OK;
}
#endif

//----------------------------------------------------------------------

// Implementation of nsIDOMCharacterData

nsresult    
nsGenericDOMDataNode::GetData(nsString& aData)
{
  if (mText.Is2b()) {
    aData.Assign(mText.Get2b(), mText.GetLength());
  }
  else {
    aData.Assign(mText.Get1b(), mText.GetLength());
  }
  return NS_OK;
}

nsresult    
nsGenericDOMDataNode::SetData(const nsString& aData)
{
  // inform any enclosed ranges of change
  // we can lie and say we are deleting all the text, since in a total
  // text replacement we should just collapse all the ranges.
  if (mRangeList) nsRange::TextOwnerChanged(mContent, 0, mText.GetLength(), 0);

  nsresult result;
  nsCOMPtr<nsITextContent> textContent = do_QueryInterface(mContent, &result);

  // If possible, let the container content object have a go at it.
  if (NS_SUCCEEDED(result)) {
    result = textContent->SetText(aData.GetUnicode(), 
                                  aData.Length(), 
                                  PR_TRUE); 
  }
  else {
    result = SetText(aData.GetUnicode(), aData.Length(), PR_TRUE); 
  }

  return result;
}

nsresult    
nsGenericDOMDataNode::GetLength(PRUint32* aLength)
{
  *aLength = mText.GetLength();
  return NS_OK;
}

nsresult    
nsGenericDOMDataNode::SubstringData(PRUint32 aStart,
                                    PRUint32 aCount,
                                    nsString& aReturn)
{
  aReturn.Truncate();

  // XXX add <0 checks if types change
  PRUint32 textLength = PRUint32( mText.GetLength() );
  if (aStart > textLength) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRUint32 amount = aCount;
  if (aStart + amount > textLength) {
    amount = textLength - aStart;
  }
  if (mText.Is2b()) {
    aReturn.Assign(mText.Get2b() + aStart, amount);
  }
  else {
    aReturn.Assign(mText.Get1b() + aStart, amount);
  }

  return NS_OK;
}

//----------------------------------------------------------------------

nsresult    
nsGenericDOMDataNode::AppendData(const nsString& aData)
{
#if 1
  // Allocate new buffer
  nsresult result = NS_OK;
  PRInt32 dataLength = aData.Length();
  PRInt32 textLength = mText.GetLength();
  PRInt32 newSize = textLength + dataLength;
  PRUnichar* to = new PRUnichar[newSize + 1];
  if (nsnull == to) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // XXX This is slow...

  // Fill new buffer with old data and new data
  if (textLength) {
    mText.CopyTo(to, 0, textLength);
  }
  nsCRT::memcpy(to + textLength, aData.GetUnicode(),
                sizeof(PRUnichar) * dataLength);

  // Null terminate the new buffer...
  to[newSize] = (PRUnichar)0;

  nsCOMPtr<nsITextContent> textContent = do_QueryInterface(mContent, &result);

  // Switch to new buffer
  // Dont do notification in SetText, since we will do it later
  if (NS_SUCCEEDED(result)) {
    result = textContent->SetText(to, newSize, PR_FALSE);
  }
  else {
    result = SetText(to, newSize, PR_FALSE);
  }

  delete [] to;

  // Trigger a reflow
  if (nsnull != mDocument) {
    nsTextContentChangeData* tccd = nsnull;
    result = NS_NewTextContentChangeData(&tccd);
    if (NS_SUCCEEDED(result)) {
      tccd->SetData(nsITextContentChangeData::Append, textLength, dataLength);
      result = mDocument->ContentChanged(mContent, tccd);
      NS_RELEASE(tccd);
    }
    else {
      result = mDocument->ContentChanged(mContent, nsnull);
    }
  }

  return result;
#else
  return ReplaceData(mText.GetLength(), 0, aData);
#endif
}

nsresult    
nsGenericDOMDataNode::InsertData(PRUint32 aOffset, const nsString& aData)
{
  return ReplaceData(aOffset, 0, aData);
}

nsresult    
nsGenericDOMDataNode::DeleteData(PRUint32 aOffset, PRUint32 aCount)
{
  nsAutoString empty;
  return ReplaceData(aOffset, aCount, empty);
}

nsresult    
nsGenericDOMDataNode::ReplaceData(PRUint32 aOffset, PRUint32 aCount,
                                  const nsString& aData)
{
  nsresult result = NS_OK;

  // sanitize arguments
  PRUint32 textLength = mText.GetLength();
  if ((aOffset > textLength) || (aOffset < 0) || (aCount < 0)) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  // Allocate new buffer
  PRUint32 endOffset = aOffset + aCount;
  if (endOffset > textLength) {
    aCount = textLength - aOffset;
    endOffset = textLength;
  }
  PRInt32 dataLength = aData.Length();
  PRInt32 newLength = textLength - aCount + dataLength;
  PRUnichar* to = new PRUnichar[newLength ? newLength+1 : 1];
  if (nsnull == to) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // inform any enclosed ranges of change
  if (mRangeList) nsRange::TextOwnerChanged(mContent, aOffset, endOffset, dataLength);
  
  // Copy over appropriate data
  if (0 != aOffset) {
    mText.CopyTo(to, 0, aOffset);
  }
  if (0 != dataLength) {
    nsCRT::memcpy(to + aOffset, aData.GetUnicode(),
                  sizeof(PRUnichar) * dataLength);
  }
  if (endOffset != textLength) {
    mText.CopyTo(to + aOffset + dataLength, endOffset, textLength - endOffset);
  }

  // Null terminate the new buffer...
  to[newLength] = (PRUnichar)0;

  // Switch to new buffer
  nsCOMPtr<nsITextContent> textContent = do_QueryInterface(mContent, &result);

  // If possible, let the container content object have a go at it.
  if (NS_SUCCEEDED(result)) {
    result = textContent->SetText(to, newLength, PR_TRUE);
  }
  else {
    result = SetText(to, newLength, PR_TRUE);
  }
  delete [] to;

  return result;
}

//----------------------------------------------------------------------

// nsIScriptObjectOwner implementation

nsresult
nsGenericDOMDataNode::GetScriptObject(nsIScriptContext* aContext,
                                      void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    nsIDOMScriptObjectFactory *factory;
    
    res = nsGenericElement::GetScriptObjectFactory(&factory);
    if (NS_OK != res) {
      return res;
    }
    
    nsIDOMNode* node;
    PRUint16 nodeType;

    res = mContent->QueryInterface(kIDOMNodeIID, (void**)&node);
    if (NS_OK != res) {
      return res;
    }

    node->GetNodeType(&nodeType);
    res = factory->NewScriptCharacterData(nodeType,
                                          aContext, mContent,
                                          mParent, (void**)&mScriptObject);
    if (nsnull != mDocument) {
      nsAutoString nodeName;
      char nameBuf[128];
      
      node->GetNodeName(nodeName);
      nodeName.ToCString(nameBuf, sizeof(nameBuf));

      aContext->AddNamedReference((void *)&mScriptObject,
                                  mScriptObject,
                                  nameBuf);
    }
    NS_RELEASE(node);
    NS_RELEASE(factory);
  }
  *aScriptObject = mScriptObject;
  return res;
}

nsresult
nsGenericDOMDataNode::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

//----------------------------------------------------------------------

nsresult
nsGenericDOMDataNode::GetListenerManager(nsIEventListenerManager** aResult)
{
  if (nsnull != mListenerManager) {
    NS_ADDREF(mListenerManager);
    *aResult = mListenerManager;
    return NS_OK;
  }
  nsresult rv = NS_NewEventListenerManager(aResult);
  if (NS_OK == rv) {
    mListenerManager = *aResult;
    NS_ADDREF(mListenerManager);
  }
  return rv;
}

//----------------------------------------------------------------------

// Implementation of nsIContent


nsresult
nsGenericDOMDataNode::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
  return NS_OK;
}

/**
 * Translate the content object into the (XIF) XML Interchange Format
 * XIF is an intermediate form of the content model, the buffer
 * will then be parsed into any number of formats including HTML, TXT, etc.
 */
nsresult
nsGenericDOMDataNode::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
  const nsIContent* content = mContent;
  nsIDOMSelection* sel = aConverter.GetSelection();

  if (sel != nsnull && mDocument->IsInSelection(sel,content))
  {
    nsCOMPtr<nsIEnumerator> enumerator;
    if (NS_SUCCEEDED(sel->GetEnumerator(getter_AddRefs(enumerator)))) {
      for (enumerator->First();NS_OK != enumerator->IsDone(); enumerator->Next()) {
        nsIDOMRange* range = nsnull;
        if (NS_SUCCEEDED(enumerator->CurrentItem((nsISupports**)&range))) {
      
          nsCOMPtr<nsIDOMNode> startNode;
          nsCOMPtr<nsIDOMNode> endNode;
          PRInt32 startOffset = 0;
          PRInt32 endOffset = 0;

          range->GetStartParent(getter_AddRefs(startNode));
          range->GetEndParent(getter_AddRefs(endNode));

          range->GetStartOffset(&startOffset);
          range->GetEndOffset(&endOffset);

          nsCOMPtr<nsIContent> startContent;
          nsCOMPtr<nsIContent> endContent;
          startContent = do_QueryInterface(startNode);
          endContent = do_QueryInterface(endNode);


          nsString  buffer;
          mText.AppendTo(buffer);
          if (startContent.get() == content || endContent.get() == content)
          { 
            // NOTE: ORDER MATTERS!
            // This must go before the Cut
            if (endContent.get() == content)
              buffer.Truncate(endOffset);            
      
            // This must go after the Trunctate
            if (startContent.get() == content)
             buffer.Cut(0,startOffset); 
          }
          aConverter.AddContent(buffer);
        }
      }
    }
  }
  else  
  {
    nsString  buffer;
    mText.AppendTo(buffer);
    aConverter.AddContent(buffer);
  }
  return NS_OK;
}

void
nsGenericDOMDataNode::ToCString(nsString& aBuf, PRInt32 aOffset,
                                PRInt32 aLen) const
{
  if (mText.Is2b()) {
    const PRUnichar* cp = mText.Get2b() + aOffset;
    const PRUnichar* end = cp + aLen;
    while (cp < end) {
      PRUnichar ch = *cp++;
      if (ch == '\r') {
        aBuf.Append("\\r");
      } else if (ch == '\n') {
        aBuf.Append("\\n");
      } else if (ch == '\t') {
        aBuf.Append("\\t");
      } else if ((ch < ' ') || (ch >= 127)) {
        char buf[10];
        PR_snprintf(buf, sizeof(buf), "\\u%04x", ch);
        aBuf.Append(buf);
      } else {
        aBuf.Append(ch);
      }
    }
  }
  else {
    unsigned char* cp = (unsigned char*)mText.Get1b() + aOffset;
    const unsigned char* end = cp + aLen;
    while (cp < end) {
      PRUnichar ch = *cp++;
      if (ch == '\r') {
        aBuf.Append("\\r");
      } else if (ch == '\n') {
        aBuf.Append("\\n");
      } else if (ch == '\t') {
        aBuf.Append("\\t");
      } else if ((ch < ' ') || (ch >= 127)) {
        char buf[10];
        PR_snprintf(buf, sizeof(buf), "\\u%04x", ch);
        aBuf.Append(buf);
      } else {
        aBuf.Append(ch);
      }
    }
  }
}

nsresult
nsGenericDOMDataNode::GetDocument(nsIDocument*& aResult) const
{
  aResult = mDocument;
  NS_IF_ADDREF(mDocument);
  return NS_OK;
}


nsresult
nsGenericDOMDataNode::SetDocument(nsIDocument* aDocument, PRBool aDeep)
{
  // If we were part of a document, make sure we get rid of the
  // script context reference to our script object so that our
  // script object can be freed (or collected).
  if ((nsnull != mDocument) && (nsnull != mScriptObject)) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
    if (globalObject) {
      nsCOMPtr<nsIScriptContext> context;
      if (NS_OK == globalObject->GetContext(getter_AddRefs(context)) && context) {
        context->RemoveReference((void *)&mScriptObject,
                                 mScriptObject);
      }
    }
  }

  mDocument = aDocument;

  // If we already have a script object and now we're being added
  // to a document, make sure that the script context adds a 
  // reference to our script object. This will ensure that it
  // won't be freed (or collected) out from under us.
  if ((nsnull != mDocument) && (nsnull != mScriptObject)) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
    if (globalObject) {
      nsCOMPtr<nsIScriptContext> context;
      if (NS_OK == globalObject->GetContext(getter_AddRefs(context)) && context) {
        context->AddNamedReference((void *)&mScriptObject,
                                   mScriptObject,
                                   "Text");
      }
    }
  }

  return NS_OK;
}

nsresult
nsGenericDOMDataNode::GetParent(nsIContent*& aResult) const
{
  NS_IF_ADDREF(mParent);
  aResult = mParent;
  return NS_OK;;
}

nsresult
nsGenericDOMDataNode::SetParent(nsIContent* aParent)
{
  mParent = aParent;
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::HandleDOMEvent(nsIPresContext* aPresContext,
                                     nsEvent* aEvent,
                                     nsIDOMEvent** aDOMEvent,
                                     PRUint32 aFlags,
                                     nsEventStatus* aEventStatus)
{
  nsresult ret = NS_OK;
  nsIDOMEvent* domEvent = nsnull;

  if (NS_EVENT_FLAG_INIT == aFlags) {
    aDOMEvent = &domEvent;
    aEvent->flags = NS_EVENT_FLAG_NONE;

    //Initiate capturing phase.  Special case first call to document
    if (nsnull != mDocument) {
      mDocument->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
    }
  }
  
  //Capturing stage evaluation
  //Always pass capturing up the tree before local evaulation
  if (NS_EVENT_FLAG_BUBBLE != aFlags && nsnull != mCapturer) {
    mCapturer->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
  }
  
  //Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
    aEvent->flags |= aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
    aEvent->flags &= ~aFlags;
  }

  //Bubbling stage
  if (NS_EVENT_FLAG_CAPTURE != aFlags && mParent != nsnull) {
    ret = mParent->HandleDOMEvent(aPresContext, aEvent, aDOMEvent,
                                  NS_EVENT_FLAG_BUBBLE, aEventStatus);
  }

  if (NS_EVENT_FLAG_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event,
    // release here.
    if (nsnull != *aDOMEvent) {
      if (0 != (*aDOMEvent)->Release()) {
        // Okay, so someone in the DOM loop (a listener, JS object)
        // still has a ref to the DOM Event but the internal data
        // hasn't been malloc'd.  Force a copy of the data here so the
        // DOM Event is still valid.
        nsIPrivateDOMEvent *privateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(kIPrivateDOMEventIID, (void**)&privateEvent)) {
          privateEvent->DuplicatePrivateData();
          NS_RELEASE(privateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }
  return ret;
}


nsresult 
nsGenericDOMDataNode::RangeAdd(nsIDOMRange& aRange)
{
  // lazy allocation of range list
  if (nsnull == mRangeList) {
    mRangeList = new nsVoidArray();
  }
  if (nsnull == mRangeList) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Make sure we don't add a range that is already
  // in the list!
  PRInt32 i = mRangeList->IndexOf(&aRange);
  if (i >= 0) {
    // Range is already in the list, so there
    // is nothing to do!
    return NS_OK;
  }
  
  // dont need to addref - this call is made by the range object itself
  PRBool rv = mRangeList->AppendElement(&aRange);
  if (rv)  return NS_OK;
  return NS_ERROR_FAILURE;
}


nsresult 
nsGenericDOMDataNode::RangeRemove(nsIDOMRange& aRange)
{
  if (mRangeList) {
    // dont need to release - this call is made by the range object itself
    PRBool rv = mRangeList->RemoveElement(&aRange);
    if (rv) {
      if (mRangeList->Count() == 0) {
        delete mRangeList;
        mRangeList = nsnull;
      }
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}


nsresult 
nsGenericDOMDataNode::GetRangeList(nsVoidArray*& aResult) const
{
  aResult = mRangeList;
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult,
                             size_t aInstanceSize) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 sum = 0;
#ifdef DEBUG
  sum += (PRUint32) aInstanceSize;
  sum += mText.GetLength() *
    (mText.Is2b() ? sizeof(PRUnichar) : sizeof(char));
#endif
  *aResult = sum;
  return NS_OK;
}

//----------------------------------------------------------------------

// Implementation of the nsIDOMText interface

nsresult
nsGenericDOMDataNode::SplitText(PRUint32 aOffset, nsIDOMText** aReturn)
{
  nsresult rv = NS_OK;
  nsAutoString cutText;
  PRUint32 length;

  GetLength(&length);
  if (aOffset > length) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  rv = SubstringData(aOffset, length-aOffset, cutText);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = DeleteData(aOffset, length-aOffset);
  if (NS_FAILED(rv)) {
    return rv;
  }

  /*
   * Use CloneContent() for creating the new node so that the new node is of
   * same class as this node!
   */

  nsCOMPtr<nsITextContent> tmpContent(do_QueryInterface(mContent, &rv));
  nsCOMPtr<nsITextContent> newContent;
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  rv = tmpContent->CloneContent(PR_FALSE, getter_AddRefs(newContent));
  if (NS_FAILED(rv)) {
    return rv;
  }


  nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(newContent, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = newNode->SetNodeValue(cutText);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIContent> parentNode;
  GetParent(*getter_AddRefs(parentNode));

  if (parentNode) {
    PRInt32 index;
    
    rv = parentNode->IndexOf(mContent, index);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIContent> content(do_QueryInterface(newNode));
      
      rv = parentNode->InsertChildAt(content, index+1, PR_TRUE);
    }
  }
  
  return newNode->QueryInterface(kIDOMTextIID, (void**)aReturn);
}

//----------------------------------------------------------------------

// Implementation of the nsITextContent interface

nsresult
nsGenericDOMDataNode::GetText(const nsTextFragment** aFragmentsResult)
{
  *aFragmentsResult = &mText;
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::GetTextLength(PRInt32* aLengthResult)
{
  if (!aLengthResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aLengthResult = mText.GetLength();
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::CopyText(nsString& aResult)
{
  if (mText.Is2b()) {
    aResult.Assign(mText.Get2b(), mText.GetLength());
  }
  else {
    aResult.Assign(mText.Get1b(), mText.GetLength());
  }
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::SetText(const PRUnichar* aBuffer, PRInt32 aLength,
                              PRBool aNotify)
{
  NS_PRECONDITION((aLength >= 0) && (nsnull != aBuffer), "bad args");
  if (aLength < 0) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  if (nsnull == aBuffer) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aNotify && (nsnull != mDocument)) {
    mDocument->BeginUpdate();
  }
  mText.SetTo(aBuffer, aLength);

  // Trigger a reflow
  if (aNotify && (nsnull != mDocument)) {
    mDocument->ContentChanged(mContent, nsnull);
    mDocument->EndUpdate();
  }
  return NS_OK;
}

nsresult
nsGenericDOMDataNode::SetText(const char* aBuffer, 
                              PRInt32 aLength,
                              PRBool aNotify)
{
  NS_PRECONDITION((aLength >= 0) && (nsnull != aBuffer), "bad args");
  if (aLength < 0) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  if (nsnull == aBuffer) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aNotify && (nsnull != mDocument)) {
    mDocument->BeginUpdate();
  }
  mText.SetTo(aBuffer, aLength);

  // Trigger a reflow
  if (aNotify && (nsnull != mDocument)) {
    mDocument->ContentChanged(mContent, nsnull);
    mDocument->EndUpdate();
  }
  return NS_OK;
}


nsresult
nsGenericDOMDataNode::IsOnlyWhitespace(PRBool* aResult)
{
  nsTextFragment& frag = mText;
  if (frag.Is2b()) {
    const PRUnichar* cp = frag.Get2b();
    const PRUnichar* end = cp + frag.GetLength();
    while (cp < end) {
      PRUnichar ch = *cp++;
      if (!XP_IS_SPACE(ch)) {
        *aResult = PR_FALSE;
        return NS_OK;
      }
    }
  }
  else {
    const char* cp = frag.Get1b();
    const char* end = cp + frag.GetLength();
    while (cp < end) {
      PRUnichar ch = PRUnichar(*(unsigned char*)cp);
      cp++;
      if (!XP_IS_SPACE(ch)) {
        *aResult = PR_FALSE;
        return NS_OK;
      }
    }
  }

  *aResult = PR_TRUE;
  return NS_OK;
}
