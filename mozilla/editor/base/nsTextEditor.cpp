/* -*- Mode: C++ tab-width: 2 indent-tabs-mode: nil c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsTextEditor.h"
#include "nsEditorEventListeners.h"
#include "nsIEditProperty.h"
#include "nsEditProperty.h"  // temporary, to get html atoms

#include "nsIStreamListener.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsIDocument.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLContentSinkStream.h"
#include "nsHTMLToTXTSinkStream.h"
#include "nsXIFDTD.h"
#include "nsFileSpec.h"

#include "nsIDOMDocument.h"
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMSelection.h"
#include "nsIDOMRange.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMElement.h"
#include "nsIDOMTextListener.h"
#include "nsIDiskDocument.h"
#include "nsEditorCID.h"
#include "nsISupportsArray.h"
#include "nsIEnumerator.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsLayoutCID.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsVoidArray.h"

// transactions the text editor knows how to build itself
#include "TransactionFactory.h"
#include "PlaceholderTxn.h"
#include "InsertTextTxn.h"

#include "nsIFileStream.h"
#include "nsIStringStream.h"

#include "nsIAppShell.h"
#include "nsIToolkit.h"
#include "nsWidgetsCID.h"
#include "nsIFileWidget.h"

class nsIFrame;


#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsTextEditRules.h"


static NS_DEFINE_IID(kIDOMEventReceiverIID, NS_IDOMEVENTRECEIVER_IID);
static NS_DEFINE_IID(kIDOMMouseListenerIID, NS_IDOMMOUSELISTENER_IID);
static NS_DEFINE_IID(kIDOMKeyListenerIID,   NS_IDOMKEYLISTENER_IID);
static NS_DEFINE_IID(kIDOMTextListenerIID,  NS_IDOMTEXTLISTENER_IID);
static NS_DEFINE_IID(kIDOMCompositionListenerIID, NS_IDOMCOMPOSITIONLISTENER_IID);
static NS_DEFINE_IID(kIDOMDragListenerIID,  NS_IDOMDRAGLISTENER_IID);
static NS_DEFINE_IID(kIDOMSelectionListenerIID, NS_IDOMSELECTIONLISTENER_IID);

static NS_DEFINE_IID(kIEditPropertyIID, NS_IEDITPROPERTY_IID);
static NS_DEFINE_CID(kEditorCID,        NS_EDITOR_CID);
static NS_DEFINE_IID(kIEditorIID,       NS_IEDITOR_IID);
static NS_DEFINE_IID(kISupportsIID,     NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kITextEditorIID,   NS_ITEXTEDITOR_IID);
static NS_DEFINE_CID(kTextEditorCID,    NS_TEXTEDITOR_CID);

static NS_DEFINE_IID(kIContentIteratorIID,   NS_ICONTENTITERTOR_IID);
static NS_DEFINE_CID(kCContentIteratorCID,   NS_CONTENTITERATOR_CID);

static NS_DEFINE_IID(kIDOMRangeIID, NS_IDOMRANGE_IID);
static NS_DEFINE_CID(kCRangeCID,    NS_RANGE_CID);

static NS_DEFINE_IID(kIInputStreamIID, NS_IINPUTSTREAM_IID);
static NS_DEFINE_IID(kIOutputStreamIID, NS_IOUTPUTSTREAM_IID);

#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif

/* ---------- TypeInState implementation ---------- */
// most methods are defined inline in TypeInState.h

NS_IMPL_ADDREF(TypeInState)

NS_IMPL_RELEASE(TypeInState)

NS_IMETHODIMP
TypeInState::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMSelectionListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMSelectionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

TypeInState::~TypeInState()
{
};

NS_IMETHODIMP TypeInState::NotifySelectionChanged()
{ 
  Reset(); 
  return NS_OK;
};


/* ---------- nsTextEditor implementation ---------- */

nsTextEditor::nsTextEditor()
{
// Done in nsEditor
//  NS_INIT_REFCNT();
  mRules = nsnull;
  nsEditProperty::InstanceInit();
}

nsTextEditor::~nsTextEditor()
{
  //the autopointers will clear themselves up. 
  //but we need to also remove the listeners or we have a leak
  nsCOMPtr<nsIDOMDocument> doc;
  nsEditor::GetDocument(getter_AddRefs(doc));
  if (doc)
  {
    nsCOMPtr<nsIDOMEventReceiver> erP;
    nsresult result = doc->QueryInterface(kIDOMEventReceiverIID, getter_AddRefs(erP));
    if (NS_SUCCEEDED(result) && erP) 
    {
      if (mKeyListenerP) {
        erP->RemoveEventListenerByIID(mKeyListenerP, kIDOMKeyListenerIID);
      }
      if (mMouseListenerP) {
        erP->RemoveEventListenerByIID(mMouseListenerP, kIDOMMouseListenerIID);
      }

	  if (mTextListenerP) {
		  erP->RemoveEventListenerByIID(mTextListenerP, kIDOMTextListenerIID);
	  }

 	  if (mCompositionListenerP) {
		  erP->RemoveEventListenerByIID(mCompositionListenerP, kIDOMCompositionListenerIID);
	  }

	  if (mDragListenerP) {
        erP->RemoveEventListenerByIID(mDragListenerP, kIDOMDragListenerIID);
      }

    }
    else
      NS_NOTREACHED("~nsTextEditor");
  }
  if (mRules) {
    delete mRules;
  }
  NS_IF_RELEASE(mTypeInState);
  nsEditProperty::InstanceShutdown();
}

// Adds appropriate AddRef, Release, and QueryInterface methods for derived class
//NS_IMPL_ISUPPORTS_INHERITED(nsTextEditor, nsEditor, nsITextEditor)

//NS_IMPL_ADDREF_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsTextEditor::AddRef(void)
{
  return nsEditor::AddRef();
}

//NS_IMPL_RELEASE_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsTextEditor::Release(void)
{
  return nsEditor::Release();
}

//NS_IMPL_QUERY_INTERFACE_INHERITED(Class, Super, AdditionalInterface)
NS_IMETHODIMP nsTextEditor::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
 
  if (aIID.Equals(nsITextEditor::GetIID())) {
    *aInstancePtr = NS_STATIC_CAST(nsITextEditor*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return nsEditor::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP nsTextEditor::Init(nsIDOMDocument *aDoc, nsIPresShell *aPresShell)
{
  NS_PRECONDITION(nsnull!=aDoc && nsnull!=aPresShell, "bad arg");
  nsresult result=NS_ERROR_NULL_POINTER;
  if ((nsnull!=aDoc) && (nsnull!=aPresShell))
  {
    // Init the base editor
    result = nsEditor::Init(aDoc, aPresShell);
    if (NS_OK != result) { return result; }

    // init the type-in state
    mTypeInState = new TypeInState();
    if (!mTypeInState) {return NS_ERROR_NULL_POINTER;}
    NS_ADDREF(mTypeInState);

    nsCOMPtr<nsIDOMSelection>selection;
    result = nsEditor::GetSelection(getter_AddRefs(selection));
    if (NS_OK != result) { return result; }
    if (selection) 
    {
      nsCOMPtr<nsIDOMSelectionListener>listener;
      listener = do_QueryInterface(mTypeInState);
      if (listener) {
        selection->AddSelectionListener(listener); 
      }
    }

    // Init the rules system
    InitRules();

    result = NS_NewEditorKeyListener(getter_AddRefs(mKeyListenerP), this);
    if (NS_OK != result) {
      return result;
    }
    result = NS_NewEditorMouseListener(getter_AddRefs(mMouseListenerP), this);
    if (NS_OK != result) {
#ifdef DEBUG_akkana
      printf("Couldn't get mouse listener\n");
#endif
      // drop the key listener if we couldn't get a mouse listener.
      mKeyListenerP = do_QueryInterface(0); 
      return result;
    }

	result = NS_NewEditorTextListener(getter_AddRefs(mTextListenerP),this);
	if (NS_OK !=result) {
		// drop the key and mouse listeners
#ifdef DEBUG_TAGUE
	printf("nsTextEditor.cpp: failed to get TextEvent Listener\n");
#endif
		mMouseListenerP = do_QueryInterface(0);
		mKeyListenerP = do_QueryInterface(0);
		return result;
	}

	result = NS_NewEditorCompositionListener(getter_AddRefs(mCompositionListenerP),this);
	if (NS_OK!=result) {
		// drop the key and mouse listeners
#ifdef DEBUG_TAGUE
	printf("nsTextEditor.cpp: failed to get TextEvent Listener\n");
#endif
		mMouseListenerP = do_QueryInterface(0);
		mKeyListenerP = do_QueryInterface(0);
		mTextListenerP = do_QueryInterface(0);
		return result;
	}

    result = NS_NewEditorDragListener(getter_AddRefs(mDragListenerP), this);
    if (NS_OK != result) {
      //return result;
		mMouseListenerP = do_QueryInterface(0);
		mKeyListenerP = do_QueryInterface(0);
		mTextListenerP = do_QueryInterface(0);
		mCompositionListenerP = do_QueryInterface(0);
    }

    nsCOMPtr<nsIDOMEventReceiver> erP;
    result = aDoc->QueryInterface(kIDOMEventReceiverIID, getter_AddRefs(erP));
    if (NS_OK != result) 
    {
      mKeyListenerP = do_QueryInterface(0);
      mMouseListenerP = do_QueryInterface(0); //dont need these if we cant register them
	  mTextListenerP = do_QueryInterface(0);
      mDragListenerP = do_QueryInterface(0); //dont need these if we cant register them
	  mCompositionListenerP = do_QueryInterface(0);
      return result;
    }
    //cmanske: Shouldn't we check result from this?
    result = erP->AddEventListenerByIID(mKeyListenerP, kIDOMKeyListenerIID);
    if (!NS_SUCCEEDED(result))
    {
      printf("nsTextEditor::Init -- faile to add mKeyListenerP\n");
      return result;
    }

#ifdef NEW_DRAG_AND_DROP
    erP->AddEventListenerByIID(mDragListenerP, kIDOMDragListenerIID);
#endif
    erP->AddEventListenerByIID(mMouseListenerP, kIDOMMouseListenerIID);
	
    erP->AddEventListenerByIID(mTextListenerP,kIDOMTextListenerIID);
    erP->AddEventListenerByIID(mCompositionListenerP,kIDOMCompositionListenerIID);

    result = NS_OK;

		EnableUndo(PR_TRUE);
  }
  return result;
}

void  nsTextEditor::InitRules()
{
// instantiate the rules for this text editor
// XXX: we should be told which set of rules to instantiate
  mRules =  new nsTextEditRules();
  mRules->Init(this);
}


NS_IMETHODIMP nsTextEditor::SetTextProperty(nsIAtom        *aProperty, 
                                            const nsString *aAttribute, 
                                            const nsString *aValue)
{
  if (!aProperty)
    return NS_ERROR_NULL_POINTER;

  if (gNoisy) 
  { 
    nsAutoString propString;
    aProperty->ToString(propString);
    char *propCString = propString.ToNewCString();
    if (gNoisy) { printf("---------- nsTextEditor::SetTextProperty %s ----------\n", propCString); }
    delete [] propCString;
  }

  nsresult result=NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMSelection>selection;
  result = nsEditor::GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    PRBool isCollapsed;
    selection->GetIsCollapsed(&isCollapsed);
    if (PR_TRUE==isCollapsed)
    {
      // manipulating text attributes on a collapsed selection only sets state for the next text insertion
      SetTypeInStateForProperty(*mTypeInState, aProperty, aAttribute, aValue);
    }
    else
    {
      // set the text property for all selected ranges
      nsEditor::BeginTransaction();
      nsCOMPtr<nsIEnumerator> enumerator;
      enumerator = do_QueryInterface(selection);
      if (enumerator)
      {
        enumerator->First(); 
        nsISupports *currentItem;
        result = enumerator->CurrentItem(&currentItem);
        if ((NS_SUCCEEDED(result)) && (nsnull!=currentItem))
        {
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          nsCOMPtr<nsIDOMNode>commonParent;
          result = range->GetCommonParent(getter_AddRefs(commonParent));
          if ((NS_SUCCEEDED(result)) && commonParent)
          {
            PRInt32 startOffset, endOffset;
            range->GetStartOffset(&startOffset);
            range->GetEndOffset(&endOffset);
            nsCOMPtr<nsIDOMNode> startParent;  nsCOMPtr<nsIDOMNode> endParent;
            range->GetStartParent(getter_AddRefs(startParent));
            range->GetEndParent(getter_AddRefs(endParent));
            if (startParent.get()==endParent.get()) 
            { // the range is entirely contained within a single text node
              // commonParent==aStartParent, so get the "real" parent of the selection
              startParent->GetParentNode(getter_AddRefs(commonParent));
              result = SetTextPropertiesForNode(startParent, commonParent, 
                                                startOffset, endOffset,
                                                aProperty, aAttribute, aValue);
            }
            else
            {
              nsCOMPtr<nsIDOMNode> startGrandParent;
              startParent->GetParentNode(getter_AddRefs(startGrandParent));
              nsCOMPtr<nsIDOMNode> endGrandParent;
              endParent->GetParentNode(getter_AddRefs(endGrandParent));
              if (NS_SUCCEEDED(result))
              {
                PRBool canCollapseStyleNode = PR_FALSE;
                if (endGrandParent.get()==startGrandParent.get())
                {
                  result = IntermediateNodesAreInline(range, startParent, startOffset, 
                                                      endParent, endOffset, 
                                                      canCollapseStyleNode);
                }
                if (NS_SUCCEEDED(result)) 
                {
                  if (PR_TRUE==canCollapseStyleNode)
                  { // the range is between 2 nodes that have a common (immediate) grandparent,
                    // and any intermediate nodes are just inline style nodes
                    result = SetTextPropertiesForNodesWithSameParent(startParent,startOffset,
                                                                     endParent,  endOffset,
                                                                     commonParent,
                                                                     aProperty, aAttribute, aValue);
                  }
                  else
                  { // the range is between 2 nodes that have no simple relationship
                    result = SetTextPropertiesForNodeWithDifferentParents(range,
                                                                          startParent,startOffset, 
                                                                          endParent,  endOffset,
                                                                          commonParent,
                                                                          aProperty, aAttribute, aValue);
                  }
                }
              }
            }
            if (NS_SUCCEEDED(result))
            { // compute a range for the selection
              // don't want to actually do anything with selection, because
              // we are still iterating through it.  Just want to create and remember
              // an nsIDOMRange, and later add the range to the selection after clearing it.
              // XXX: I'm blocked here because nsIDOMSelection doesn't provide a mechanism
              //      for setting a compound selection yet.
            }
          }
        }
      }
      nsEditor::EndTransaction();
    }
    if (NS_SUCCEEDED(result))
    { // set the selection
      // XXX: can't do anything until I can create ranges
    }
  }
  if (gNoisy) {DebugDumpContent(); } // DEBUG
  //HACK TO DRAW CHANGES
  // HACKForceRedraw();
  //END HACK
  return result;
}

NS_IMETHODIMP nsTextEditor::GetTextProperty(nsIAtom *aProperty, 
                                            const nsString *aAttribute, 
                                            const nsString *aValue,
                                            PRBool &aFirst, PRBool &aAny, PRBool &aAll)
{
  if (!aProperty)
    return NS_ERROR_NULL_POINTER;

  if (gNoisy) 
  { 
    nsAutoString propString;
    aProperty->ToString(propString);
    char *propCString = propString.ToNewCString();
    if (gNoisy) { printf("nsTextEditor::GetTextProperty %s\n", propCString); }
    delete [] propCString;
  }

  nsresult result=NS_ERROR_NOT_INITIALIZED;
  aAny=PR_FALSE;
  aAll=PR_TRUE;
  aFirst=PR_FALSE;
  PRBool first=PR_TRUE;
  nsCOMPtr<nsIDOMSelection>selection;
  result = nsEditor::GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    PRBool isCollapsed;
    selection->GetIsCollapsed(&isCollapsed);
    nsCOMPtr<nsIEnumerator> enumerator;
    enumerator = do_QueryInterface(selection);
    if (enumerator)
    {
      enumerator->First(); 
      nsISupports *currentItem;
      result = enumerator->CurrentItem(&currentItem);
      // XXX: should be a while loop, to get each separate range
      if ((NS_SUCCEEDED(result)) && currentItem)
      {
        PRBool firstNodeInRange = PR_TRUE; // for each range, set a flag 
        nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
        nsCOMPtr<nsIContentIterator> iter;
        result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                                    kIContentIteratorIID, 
                                                    getter_AddRefs(iter));
        if ((NS_SUCCEEDED(result)) && iter)
        {
          iter->Init(range);
          // loop through the content iterator for each content node
          // for each text node:
          // get the frame for the content, and from it the style context
          // ask the style context about the property
          nsCOMPtr<nsIContent> content;
          result = iter->CurrentNode(getter_AddRefs(content));
          while (NS_COMFALSE == iter->IsDone())
          {
            if (gNoisy) { printf("  checking node %p\n", content.get()); }
            nsCOMPtr<nsIDOMCharacterData>text;
            text = do_QueryInterface(content);
            if (text)
            {
              PRBool skipNode = PR_FALSE;
              if (PR_FALSE==isCollapsed && PR_TRUE==first && PR_TRUE==firstNodeInRange)
              {
                firstNodeInRange = PR_FALSE;
                PRInt32 startOffset;
                range->GetStartOffset(&startOffset);
                PRUint32 count;
                text->GetLength(&count);
                if (startOffset==(PRInt32)count) 
                {
                  if (gNoisy) { printf("  skipping node %p\n", content.get()); }
                  skipNode = PR_TRUE;
                }
              }
              if (PR_FALSE==skipNode)
              {
                nsCOMPtr<nsIDOMNode>node;
                node = do_QueryInterface(content);
                if (node)
                {
                  PRBool isSet;
                  nsCOMPtr<nsIDOMNode>resultNode;
                  IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, isSet, getter_AddRefs(resultNode));
                  if (PR_TRUE==first)
                  {
                    aFirst = isSet;
                    first = PR_FALSE;
                  }
                  if (PR_TRUE==isSet) {
                    aAny = PR_TRUE;
                  }
                  else {
                    aAll = PR_FALSE;
                  }
                }
              }
            }
            iter->Next();
            iter->CurrentNode(getter_AddRefs(content));
          }
        }
      }
    }
  }
  if (PR_FALSE==aAny) { // make sure that if none of the selection is set, we don't report all is set
    aAll = PR_FALSE;
  }
  if (gNoisy) { printf("  returning first=%d any=%d all=%d\n", aFirst, aAny, aAll); }
  return result;
}

void nsTextEditor::IsTextStyleSet(nsIStyleContext *aSC, 
                                  nsIAtom *aProperty, 
                                  const nsString *aAttribute,  
                                  PRBool &aIsSet) const
{
  aIsSet = PR_FALSE;
  if (aSC && aProperty)
  {
    nsStyleFont* font = (nsStyleFont*)aSC->GetStyleData(eStyleStruct_Font);
    if (nsIEditProperty::i==aProperty)
    {
      aIsSet = PRBool(font->mFont.style & NS_FONT_STYLE_ITALIC);
    }
    else if (nsIEditProperty::b==aProperty)
    { // XXX: check this logic with Peter
      aIsSet = PRBool(font->mFont.weight > NS_FONT_WEIGHT_NORMAL);
    }
  }
}

// this will NOT find aAttribute unless aAttribute has a non-null value
// so singleton attributes like <Table border> will not be matched!
void nsTextEditor::IsTextPropertySetByContent(nsIDOMNode     *aNode,
                                              nsIAtom        *aProperty, 
                                              const nsString *aAttribute, 
                                              const nsString *aValue, 
                                              PRBool         &aIsSet,
                                              nsIDOMNode    **aStyleNode) const
{
  nsresult result;
  aIsSet = PR_FALSE;  // must be initialized to false for code below to work
  nsAutoString propName;
  aProperty->ToString(propName);
  nsCOMPtr<nsIDOMNode>parent;
  result = aNode->GetParentNode(getter_AddRefs(parent));
  while (NS_SUCCEEDED(result) && parent)
  {
    nsCOMPtr<nsIDOMElement>element;
    element = do_QueryInterface(parent);
    if (element)
    {
      nsString tag;
      element->GetTagName(tag);
      if (propName.EqualsIgnoreCase(tag))
      {
        PRBool found = PR_FALSE;
        if (aAttribute)
        {
          nsAutoString value;
          element->GetAttribute(*aAttribute, value);
          if (0!=value.Length())
          {
            if (!aValue) {
              found = PR_TRUE;
            }
            else if (aValue->EqualsIgnoreCase(value)) {
              found = PR_TRUE;
            }
            else {  // we found the prop with the attribute, but the value doesn't match
              break;
            }
          }
        }
        else { 
          found = PR_TRUE;
        }
        if (PR_TRUE==found)
        {
          aIsSet = PR_TRUE;
          break;
        }
      }
    }
    nsCOMPtr<nsIDOMNode>temp;
    result = parent->GetParentNode(getter_AddRefs(temp));
    if (NS_SUCCEEDED(result) && temp) {
      parent = do_QueryInterface(temp);
    }
    else {
      parent = do_QueryInterface(nsnull);
    }
  }
}


NS_IMETHODIMP nsTextEditor::RemoveTextProperty(nsIAtom *aProperty, const nsString *aAttribute)
{
  if (!aProperty)
    return NS_ERROR_NULL_POINTER;

  if (gNoisy) 
  { 
    nsAutoString propString;
    aProperty->ToString(propString);
    char *propCString = propString.ToNewCString();
    if (gNoisy) { printf("---------- nsTextEditor::RemoveTextProperty %s ----------\n", propCString); }
    delete [] propCString;
  }

  nsresult result=NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMSelection>selection;
  result = nsEditor::GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    PRBool isCollapsed;
    selection->GetIsCollapsed(&isCollapsed);
    if (PR_TRUE==isCollapsed)
    {
      // manipulating text attributes on a collapsed selection only sets state for the next text insertion
      SetTypeInStateForProperty(*mTypeInState, aProperty, aAttribute, nsnull);
    }
    else
    {
      nsEditor::BeginTransaction();
      nsCOMPtr<nsIDOMNode> startParent, endParent;
      PRInt32 startOffset, endOffset;
      nsCOMPtr<nsIEnumerator> enumerator;
      enumerator = do_QueryInterface(selection);
      if (enumerator)
      {
        enumerator->First(); 
        nsISupports *currentItem;
        result = enumerator->CurrentItem(&currentItem);
        if ((NS_SUCCEEDED(result)) && (nsnull!=currentItem))
        {
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          nsCOMPtr<nsIDOMNode>commonParent;
          result = range->GetCommonParent(getter_AddRefs(commonParent));
          if ((NS_SUCCEEDED(result)) && commonParent)
          {
            range->GetStartOffset(&startOffset);
            range->GetEndOffset(&endOffset);
            range->GetStartParent(getter_AddRefs(startParent));
            range->GetEndParent(getter_AddRefs(endParent));
            if (startParent.get()==endParent.get()) 
            { // the range is entirely contained within a single text node
              // commonParent==aStartParent, so get the "real" parent of the selection
              startParent->GetParentNode(getter_AddRefs(commonParent));
              result = RemoveTextPropertiesForNode(startParent, commonParent, 
                                                   startOffset, endOffset,
                                                   aProperty, nsnull);
            }
            else
            {
              nsCOMPtr<nsIDOMNode> startGrandParent;
              startParent->GetParentNode(getter_AddRefs(startGrandParent));
              nsCOMPtr<nsIDOMNode> endGrandParent;
              endParent->GetParentNode(getter_AddRefs(endGrandParent));
              if (NS_SUCCEEDED(result))
              {
                PRBool canCollapseStyleNode = PR_FALSE;
                if (endGrandParent.get()==startGrandParent.get())
                {
                  result = IntermediateNodesAreInline(range, startParent, startOffset, 
                                                      endParent, endOffset, 
                                                      canCollapseStyleNode);
                }
                if (NS_SUCCEEDED(result)) 
                {
                  if (PR_TRUE==canCollapseStyleNode)
                  { // the range is between 2 nodes that have a common (immediate) grandparent,
                    // and any intermediate nodes are just inline style nodes
                    result = RemoveTextPropertiesForNodesWithSameParent(startParent,startOffset,
                                                                        endParent,  endOffset,
                                                                        commonParent,
                                                                        aProperty, nsnull);
                  }
                  else
                  { // the range is between 2 nodes that have no simple relationship
                    result = RemoveTextPropertiesForNodeWithDifferentParents(startParent,startOffset, 
                                                                             endParent,  endOffset,
                                                                             commonParent,
                                                                             aProperty, nsnull);
                  }
                }
              }
            }
            if (NS_SUCCEEDED(result))
            { // compute a range for the selection
              // don't want to actually do anything with selection, because
              // we are still iterating through it.  Just want to create and remember
              // an nsIDOMRange, and later add the range to the selection after clearing it.
              // XXX: I'm blocked here because nsIDOMSelection doesn't provide a mechanism
              //      for setting a compound selection yet.
            }
          }
        }
      }
      nsEditor::EndTransaction();
      if (NS_SUCCEEDED(result))
      { 
        selection->Collapse(startParent, startOffset);
        selection->Extend(endParent, endOffset);
      }
    }
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::DeleteSelection(nsIEditor::ECollapsedSelectionAction aAction)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  nsresult result = nsEditor::BeginTransaction();
  if (NS_FAILED(result)) { return result; }

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kDeleteSelection);
  ruleInfo.collapsedAction = aAction;
  result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::DeleteSelection(aAction);
    // post-process 
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }

  nsresult endTxnResult = nsEditor::EndTransaction();  // don't return this result!
  NS_ASSERTION ((NS_SUCCEEDED(endTxnResult)), "bad end transaction result");


// XXXX: Horrible hack! We are doing this because
// of an error in Gecko which is not rendering the
// document after a change via the DOM - gpk 2/13/99
  // BEGIN HACK!!!
  // HACKForceRedraw();
  // END HACK

  return result;
}

NS_IMETHODIMP nsTextEditor::InsertText(const nsString& aStringToInsert)
{
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsString resultString;
  PlaceholderTxn *placeholderTxn=nsnull;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertText);
  ruleInfo.placeTxn = &placeholderTxn;
  ruleInfo.inString = &aStringToInsert;
  ruleInfo.outString = &resultString;
  ruleInfo.typeInState = *mTypeInState;

  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::InsertText(resultString);
    // post-process 
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  if (placeholderTxn)
    placeholderTxn->SetAbsorb(PR_FALSE);  // this ends the merging of txns into placeholderTxn

  // BEGIN HACK!!!
  // HACKForceRedraw();
  // END HACK
  return result;
}

NS_IMETHODIMP nsTextEditor::InsertBreak()
{
  // For plainttext just pass newlines through
  nsAutoString  key;
  key += '\n';
  return InsertText(key);
}


NS_IMETHODIMP nsTextEditor::EnableUndo(PRBool aEnable)
{
  return nsEditor::EnableUndo(aEnable);
}

NS_IMETHODIMP nsTextEditor::Undo(PRUint32 aCount)
{
  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kUndo);
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::Undo(aCount);
    nsEditor::GetSelection(getter_AddRefs(selection));
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::CanUndo(PRBool &aIsEnabled, PRBool &aCanUndo)
{
  return nsEditor::CanUndo(aIsEnabled, aCanUndo);
}

NS_IMETHODIMP nsTextEditor::Redo(PRUint32 aCount)
{
  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kRedo);
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::Redo(aCount);
    nsEditor::GetSelection(getter_AddRefs(selection));
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::CanRedo(PRBool &aIsEnabled, PRBool &aCanRedo)
{
  return nsEditor::CanRedo(aIsEnabled, aCanRedo);
}

NS_IMETHODIMP nsTextEditor::BeginTransaction()
{
  return nsEditor::BeginTransaction();
}

NS_IMETHODIMP nsTextEditor::EndTransaction()
{
  return nsEditor::EndTransaction();
}

NS_IMETHODIMP nsTextEditor::MoveSelectionUp(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionDown(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionNext(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectNext(nsIAtom *aIncrement, PRBool aExtendSelection) 
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectAll()
{
  return nsEditor::SelectAll();
}

NS_IMETHODIMP nsTextEditor::ScrollUp(nsIAtom *aIncrement)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::ScrollDown(nsIAtom *aIncrement)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::ScrollIntoView(PRBool aScrollToBegin)
{
  return nsEditor::ScrollIntoView(aScrollToBegin);
}

static NS_DEFINE_IID(kCFileWidgetCID, NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kIFileWidgetIID, NS_IFILEWIDGET_IID);

NS_IMETHODIMP nsTextEditor::SaveDocument(PRBool saveAs, PRBool saveCopy)
{
	nsresult rv = NS_OK;
	
  // get the document
  nsCOMPtr<nsIDOMDocument> doc;
  rv = GetDocument(getter_AddRefs(doc));
  if (NS_FAILED(rv) || !doc)
    return rv;
  
  nsCOMPtr<nsIDiskDocument>  diskDoc = do_QueryInterface(doc);
  if (!diskDoc)
    return NS_ERROR_NO_INTERFACE;
  
  // this should really call out to the appcore for the display of the put file
  // dialog.
  
  // find out if the doc already has a fileSpec associated with it.
  nsFileSpec		docFileSpec;
  PRBool mustShowFileDialog = saveAs || (diskDoc->GetFileSpec(docFileSpec) == NS_ERROR_NOT_INITIALIZED);
  PRBool replacing = !saveAs;
  
  if (mustShowFileDialog)
  {
  	nsCOMPtr<nsIFileWidget>	fileWidget;
    rv = nsComponentManager::CreateInstance(kCFileWidgetCID, nsnull, kIFileWidgetIID, getter_AddRefs(fileWidget));
    if (NS_SUCCEEDED(rv) && fileWidget)
    {
      nsAutoString  promptString("Save this document as:");			// XXX i18n, l10n
  	  nsFileDlgResults dialogResult;
  	  dialogResult = fileWidget->PutFile(nsnull, promptString, docFileSpec);
  	  if (dialogResult == nsFileDlgResults_Cancel)
  	    return NS_OK;
  	    
  	  replacing = (dialogResult == nsFileDlgResults_Replace);
  	}
  	else
  	{
   		NS_ASSERTION(0, "Failed to get file widget");
  		return rv;
  	}
  }

  nsAutoString  charsetStr("ISO-8859-1");
  rv = diskDoc->SaveFile(&docFileSpec, replacing, saveCopy, nsIDiskDocument::eSaveFileHTML, charsetStr);
  
  if (NS_FAILED(rv))
  {
    // show some error dialog?
    NS_WARNING("Saving file failed");
  }
  
  return rv;
}

NS_IMETHODIMP nsTextEditor::Save()
{
  return SaveDocument(PR_FALSE, PR_FALSE);
}

NS_IMETHODIMP nsTextEditor::SaveAs(PRBool aSavingCopy)
{
  return SaveDocument(PR_TRUE, aSavingCopy);
}

NS_IMETHODIMP nsTextEditor::Cut()
{
  return nsEditor::Cut();
}

NS_IMETHODIMP nsTextEditor::Copy()
{
  return nsEditor::Copy();
}

NS_IMETHODIMP nsTextEditor::Paste()
{
  return nsEditor::Paste();
}

NS_IMETHODIMP nsTextEditor::Insert(nsString& aInputString)
{
  printf("nsTextEditor::Insert not yet implemented\n");
  return NS_ERROR_NOT_IMPLEMENTED;
}

// Useful helper method for Get/SetBodyWrapWidth:
static nsCOMPtr<nsIDOMElement>
findPreElement(nsIDOMDocument* domdoc)
{
  nsCOMPtr<nsIDocument> doc (do_QueryInterface(domdoc));
  if (!doc)
    return 0;

  nsIContent* rootContent = doc->GetRootContent();
  if (!rootContent)
    return 0;

  nsCOMPtr<nsIDOMNode> rootNode (do_QueryInterface(rootContent));
  if (!rootNode)
    return 0;

  nsString prestr ("PRE");  // GetFirstNodeOfType requires capitals
  nsCOMPtr<nsIDOMNode> preNode;
  if (!NS_SUCCEEDED(nsEditor::GetFirstNodeOfType(rootNode, prestr,
                                                 getter_AddRefs(preNode))))
  {
#ifdef DEBUG_akkana
    printf("No PRE tag\n");
#endif
    return 0;
  }
  return do_QueryInterface(preNode);
}

//
// Get the wrap width for the first PRE tag in the document.
// If no PRE tag, throw an error.
//
NS_IMETHODIMP nsTextEditor::GetBodyWrapWidth(PRInt32 *aWrapColumn)
{
  nsresult res;

  if (! aWrapColumn)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMDocument> domdoc;
  nsEditor::GetDocument(getter_AddRefs(domdoc));
  if (!domdoc)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIDOMElement> preElement = findPreElement(domdoc);
  if (!preElement)
    return NS_ERROR_UNEXPECTED;
  nsString colsStr ("cols");
  nsString numCols;
  PRBool isSet;
  res = GetAttributeValue(preElement, colsStr, numCols, isSet);
  if (!NS_SUCCEEDED(res))
  {
#ifdef DEBUG_akkana
    printf("GetAttributeValue(cols) failed\n");
#endif
    return NS_ERROR_UNEXPECTED;
  }

  if (isSet)
  {
    PRInt32 errCode;
    *aWrapColumn = numCols.ToInteger(&errCode);
    if (errCode)
      return NS_ERROR_FAILURE;
    return NS_OK;
  }

  // if we get here, cols isn't set, so check whether wrap is set:
  nsString wrapStr ("wrap");
  res = GetAttributeValue(preElement, colsStr, numCols, isSet);
  if (!NS_SUCCEEDED(res))
  {
#ifdef DEBUG_akkana
    printf("GetAttributeValue(cols) failed\n");
#endif
    return NS_ERROR_UNEXPECTED;
  }

  if (isSet)
    *aWrapColumn = 0;   // wrap to window width
  else
    *aWrapColumn = -1;  // no wrap

  return NS_OK;
}

//
// Change the wrap width on the first <PRE> tag in this document.
// (Eventually want to search for more than one in case there are
// interspersed quoted text blocks.)
// 
NS_IMETHODIMP nsTextEditor::SetBodyWrapWidth(PRInt32 aWrapColumn)
{
  nsresult res;

  if (! aWrapColumn)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMDocument> domdoc;
  nsEditor::GetDocument(getter_AddRefs(domdoc));
  if (!domdoc)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIDOMElement> preElement = findPreElement(domdoc);
  if (!preElement)
    return NS_ERROR_UNEXPECTED;

  nsString wrapStr ("wrap");
  nsString colsStr ("cols");

  // If wrap col is nonpositive, then we need to remove any existing "cols":
  if (aWrapColumn <= 0)
  {
    (void)RemoveAttribute(preElement, colsStr);

    if (aWrapColumn == 0)        // Wrap to window width
    {
      nsString oneStr ("1");
      res = SetAttribute(preElement, wrapStr, oneStr);
    }
    else res = NS_OK;
    return res;
  }

  // Otherwise we're setting cols, want to remove wrap
  (void)RemoveAttribute(preElement, wrapStr);
  nsString numCols;
  numCols.Append(aWrapColumn, 10);
  res = SetAttribute(preElement, colsStr, numCols);

  // Layout doesn't detect that this attribute change requires redraw.  Sigh.
  //HACKForceRedraw();  // This doesn't do it either!

  return res;
}  

NS_IMETHODIMP nsTextEditor::OutputText(nsString& aOutputString)
{
  return OutputText(nsnull,&aOutputString,nsnull);
}

NS_IMETHODIMP nsTextEditor::OutputText(nsIOutputStream* aOutputStream, nsString* aCharsetOverride)
{
  return OutputText(aOutputStream,nsnull,aCharsetOverride);
}


NS_IMETHODIMP nsTextEditor::OutputText(nsIOutputStream* aOutputStream, nsString* aOutputString, nsString* aCharsetOverride)
{ 
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> shell;
 	GetPresShell(getter_AddRefs(shell));
  if (shell) {
    nsCOMPtr<nsIDocument> doc;
    shell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      nsString buffer;

      doc->CreateXIF(buffer);

      nsAutoString charset;
      rv = doc->GetDocumentCharacterSet(charset);
      if(NS_FAILED(rv)) {
         charset = "ISO-8859-1"; 
      }
      nsIParser* parser;

      static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
      static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

      rv = nsComponentManager::CreateInstance(kCParserCID, 
                                                 nsnull, 
                                                 kCParserIID, 
                                                 (void **)&parser);

      if (NS_SUCCEEDED(rv)) {
        nsIHTMLContentSink* sink = nsnull;
        
        if (NS_SUCCEEDED(rv))
        {
          
          if (aOutputString != nsnull)
            rv = NS_New_HTMLToTXT_SinkStream(&sink,aOutputString); 
          else if (aOutputStream != nsnull)
            rv = NS_New_HTMLToTXT_SinkStream(&sink,aOutputStream,aCharsetOverride);

          if (sink && NS_SUCCEEDED(rv))
          {
 	          parser->SetContentSink(sink);
            parser->SetDocumentCharset(charset, kCharsetFromPreviousLoading);
 	          nsIDTD* dtd = nsnull;
	          rv = NS_NewXIFDTD(&dtd);
	          if (NS_OK == rv) {
	            parser->RegisterDTD(dtd);
	            parser->Parse(buffer, 0, "text/xif",PR_FALSE,PR_TRUE);           
	          }

	          NS_IF_RELEASE(dtd);
	          NS_IF_RELEASE(sink);
            NS_IF_RELEASE(parser);
          }

        }        
      }
    }
  }
  return rv;
}


NS_IMETHODIMP nsTextEditor::OutputHTML(nsString& aOutputString)
{
  return OutputHTML(nsnull,&aOutputString,nsnull);
}

NS_IMETHODIMP nsTextEditor::OutputHTML(nsIOutputStream* aOutputStream,nsString* aCharsetOverride)
{
  return OutputHTML(aOutputStream,nsnull,aCharsetOverride);
}


NS_IMETHODIMP nsTextEditor::OutputHTML(nsIOutputStream* aOutputStream, nsString* aOutputString, nsString* aCharsetOverride)
{
  
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresShell> shell;
 	GetPresShell(getter_AddRefs(shell));
  if (shell) {
    nsCOMPtr<nsIDocument> doc;
    shell->GetDocument(getter_AddRefs(doc));
    if (doc) {
      nsString buffer;

      doc->CreateXIF(buffer);

      nsAutoString charset;
      rv = doc->GetDocumentCharacterSet(charset);
      if(NS_FAILED(rv)) {
         charset = "ISO-8859-1"; 
      }
      nsIParser* parser;

      static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
      static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

      rv = nsComponentManager::CreateInstance(kCParserCID, 
                                                 nsnull, 
                                                 kCParserIID, 
                                                 (void **)&parser);

      if (NS_OK == rv) {
        nsIHTMLContentSink* sink = nsnull;

        if (aOutputStream)
          rv = NS_New_HTML_ContentSinkStream(&sink,aOutputStream,aCharsetOverride);
        else if (aOutputString)
          rv = NS_New_HTML_ContentSinkStream(&sink,aOutputString);
  
      	if (sink && NS_SUCCEEDED(rv)) {

	        if (NS_OK == rv) {
	          parser->SetContentSink(sink);
	    
	          parser->SetDocumentCharset(charset, kCharsetFromPreviousLoading);

	          nsIDTD* dtd = nsnull;
	          rv = NS_NewXIFDTD(&dtd);
	          if (NS_OK == rv) {
	            parser->RegisterDTD(dtd);
	            parser->Parse(buffer, 0, "text/xif",PR_FALSE,PR_TRUE);           
	          }
	          NS_IF_RELEASE(dtd);
	          NS_IF_RELEASE(sink);
	        }
        }
        NS_RELEASE(parser);
      }
  	}
	}
  return rv;
}


NS_IMETHODIMP nsTextEditor::SetTextPropertiesForNode(nsIDOMNode  *aNode, 
                                                     nsIDOMNode  *aParent,
                                                     PRInt32      aStartOffset,
                                                     PRInt32      aEndOffset,
                                                     nsIAtom     *aPropName, 
                                                     const nsString *aAttribute,
                                                     const nsString *aValue)
{
  if (gNoisy) { printf("nsTextEditor::SetTextPropertyForNode\n"); }
  nsresult result=NS_OK;
  // verify that aNode is a text node
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nodeAsChar =  do_QueryInterface(aNode);
  if (!nodeAsChar)
    return NS_ERROR_FAILURE;

  PRBool textPropertySet;
  nsCOMPtr<nsIDOMNode>resultNode;
  IsTextPropertySetByContent(aNode, aPropName, aAttribute, aValue, textPropertySet, getter_AddRefs(resultNode));
  if (PR_FALSE==textPropertySet)
  {
    nsAutoString tag;
    aPropName->ToString(tag);
    if (NS_SUCCEEDED(result)) 
    {
      nsCOMPtr<nsIDOMNode>newStyleNode;
      if (NS_SUCCEEDED(result))
      {
        result = nsEditor::CreateNode(tag, aParent, 0, getter_AddRefs(newStyleNode));
        if (NS_SUCCEEDED(result) && newStyleNode)
        {
          result = MoveContentOfNodeIntoNewParent(aNode, newStyleNode, aStartOffset, aEndOffset);
          if (NS_SUCCEEDED(result) && newStyleNode) 
          {
            if (aAttribute)
            {
              nsCOMPtr<nsIDOMElement> newStyleElement;
              newStyleElement = do_QueryInterface(newStyleNode);
              nsAutoString value;
              if (aValue) {
                value = *aValue;
              }
              // XXX should be a call to editor to change attribute!
              result = newStyleElement->SetAttribute(*aAttribute, value);
            }
          }
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::MoveContentOfNodeIntoNewParent(nsIDOMNode  *aNode, 
                                                           nsIDOMNode  *aNewParentNode, 
                                                           PRInt32      aStartOffset, 
                                                           PRInt32      aEndOffset)
{
  if (!aNode || !aNewParentNode) { return NS_ERROR_NULL_POINTER; }
  if (gNoisy) { printf("nsTextEditor::MoveContentOfNodeIntoNewParent\n"); }
  nsresult result=NS_OK;

  PRUint32 count;
  result = GetLengthOfDOMNode(aNode, count);

  if (NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsIDOMNode>newChildNode;  // this will be the child node we move into the new style node
    // split the node at the start offset unless the split would create an empty node
    if (aStartOffset!=0)
    {
      result = nsEditor::SplitNode(aNode, aStartOffset, getter_AddRefs(newChildNode));
      if (gNoisy) { printf("* split created left node %p\n", newChildNode.get());}
      if (gNoisy) {DebugDumpContent(); } // DEBUG
    }
    if (NS_SUCCEEDED(result))
    {
      if (aEndOffset!=(PRInt32)count)
      {
        result = nsEditor::SplitNode(aNode, aEndOffset-aStartOffset, getter_AddRefs(newChildNode));
        if (gNoisy) { printf("* split created left node %p\n", newChildNode.get());}
        if (gNoisy) {DebugDumpContent(); } // DEBUG
      }
      else
      {
        newChildNode = do_QueryInterface(aNode);
        if (gNoisy) { printf("* second split not required, new text node set to aNode = %p\n", newChildNode.get());}
      }
      if (NS_SUCCEEDED(result))
      {
        // move aNewParentNode into the right location

        // optimization:  if all we're doing is changing a value for an existing attribute for the
        //                entire selection, then just twiddle the existing style node
        PRBool done = PR_FALSE; // set to true in optimized case if we can really do the optimization
        /*
        if (aAttribute && aValue && (0==aStartOffset) && (aEndOffset==(PRInt32)count))
        {
          // ??? can we really compute this?
        }
        */
        if (PR_FALSE==done)
        {
          // if we've ended up with an empty text node, just delete it and we're done
          nsCOMPtr<nsIDOMCharacterData>newChildNodeAsChar;
          newChildNodeAsChar =  do_QueryInterface(newChildNode);
          PRUint32 newChildNodeLength;
          if (newChildNodeAsChar)
          {
            newChildNodeAsChar->GetLength(&newChildNodeLength);
            if (0==newChildNodeLength)
            {
              result = nsEditor::DeleteNode(newChildNode); 
              done = PR_TRUE;
            }
          }
          // move the new child node into the new parent
          if (PR_FALSE==done)
          {
            // first, move the new parent into the correct location
            PRInt32 offsetInParent;
            nsCOMPtr<nsIDOMNode>parentNode;
            result = aNode->GetParentNode(getter_AddRefs(parentNode));
            if (NS_SUCCEEDED(result))
            {
              result = nsEditor::DeleteNode(aNewParentNode);
              if (NS_SUCCEEDED(result))
              { // must get child offset AFTER delete of aNewParentNode!
                result = GetChildOffset(aNode, parentNode, offsetInParent);
                if (NS_SUCCEEDED(result))
                {
                  result = nsEditor::InsertNode(aNewParentNode, parentNode, offsetInParent);
                  if (NS_SUCCEEDED(result))
                  {
                    // then move the new child into the new parent node
                    result = nsEditor::DeleteNode(newChildNode);
                    if (NS_SUCCEEDED(result)) 
                    {
                      result = nsEditor::InsertNode(newChildNode, aNewParentNode, 0);
                      if (NS_SUCCEEDED(result)) 
                      { // set the selection
                        nsCOMPtr<nsIDOMSelection>selection;
                        result = nsEditor::GetSelection(getter_AddRefs(selection));
                        if (NS_SUCCEEDED(result)) 
                        {
                          selection->Collapse(newChildNode, 0);
                          PRInt32 endOffset = aEndOffset-aStartOffset;
                          selection->Extend(newChildNode, endOffset);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return result;
}

/* this should only get called if the only intervening nodes are inline style nodes */
NS_IMETHODIMP 
nsTextEditor::SetTextPropertiesForNodesWithSameParent(nsIDOMNode  *aStartNode,
                                                      PRInt32      aStartOffset,
                                                      nsIDOMNode  *aEndNode,
                                                      PRInt32      aEndOffset,
                                                      nsIDOMNode  *aParent,
                                                      nsIAtom     *aPropName,
                                                      const nsString *aAttribute,
                                                      const nsString *aValue)
{
  if (gNoisy) { printf("nsTextEditor::SetTextPropertiesForNodesWithSameParent\n"); }
  nsresult result=NS_OK;
  PRBool textPropertySet;
  nsCOMPtr<nsIDOMNode>resultNode;
  IsTextPropertySetByContent(aStartNode, aPropName, aAttribute, aValue, textPropertySet, getter_AddRefs(resultNode));
  if (PR_FALSE==textPropertySet)
  {
    nsAutoString tag;
    aPropName->ToString(tag);
    // create the new style node, which will be the new parent for the selected nodes
    nsCOMPtr<nsIDOMNode>newStyleNode;
    result = nsEditor::CreateNode(tag, aParent, 0, getter_AddRefs(newStyleNode));
    if (NS_SUCCEEDED(result) && newStyleNode)
    {
      result = MoveContiguousContentIntoNewParent(aStartNode, aStartOffset, aEndNode, aEndOffset, aParent, newStyleNode);
      if (NS_SUCCEEDED(result) && aAttribute)
      {
        nsCOMPtr<nsIDOMElement> newStyleElement;
        newStyleElement = do_QueryInterface(newStyleNode);
        nsAutoString value;
        if (aValue) {
          value = *aValue;
        }
        result = newStyleElement->SetAttribute(*aAttribute, value);
      }
    }
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::MoveContiguousContentIntoNewParent(nsIDOMNode  *aStartNode, 
                                                               PRInt32      aStartOffset, 
                                                               nsIDOMNode  *aEndNode,
                                                               PRInt32      aEndOffset,
                                                               nsIDOMNode  *aGrandParentNode,
                                                               nsIDOMNode  *aNewParentNode)
{
  if (!aStartNode || !aEndNode || !aNewParentNode) { return NS_ERROR_NULL_POINTER; }
  if (gNoisy) { printf("nsTextEditor::MoveContiguousContentIntoNewParent\n"); }

  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode>newLeftNode;  // this will be the middle text node
  if (0!=aStartOffset) {
    result = nsEditor::SplitNode(aStartNode, aStartOffset, getter_AddRefs(newLeftNode));
  }
  if (NS_SUCCEEDED(result))
  {
    PRUint32 count;
    GetLengthOfDOMNode(aEndNode, count);
    nsCOMPtr<nsIDOMNode>newRightNode;  // this will be the middle text node
    if ((PRInt32)count!=aEndOffset) {
      result = nsEditor::SplitNode(aEndNode, aEndOffset, getter_AddRefs(newRightNode));
    }
    else {
      newRightNode = do_QueryInterface(aEndNode);
    }
    if (NS_SUCCEEDED(result))
    {
      PRInt32 offsetInParent;
      if (newLeftNode) {
        result = GetChildOffset(newLeftNode, aGrandParentNode, offsetInParent);
      }
      else {
        offsetInParent = -1; // relies on +1 below in call to CreateNode
      }
      if (NS_SUCCEEDED(result))
      {
        // wherever aNewParentNode is, delete it and insert it into aGrandParentNode
        result = nsEditor::DeleteNode(aNewParentNode);
        if (NS_SUCCEEDED(result))
        { 
          result = nsEditor::InsertNode(aNewParentNode, aGrandParentNode, offsetInParent);
          if (NS_SUCCEEDED(result))
          { // move the right half of the start node into the new parent node
            nsCOMPtr<nsIDOMNode>intermediateNode;
            result = aStartNode->GetNextSibling(getter_AddRefs(intermediateNode));
            if (NS_SUCCEEDED(result))
            {
              result = nsEditor::DeleteNode(aStartNode);
              if (NS_SUCCEEDED(result)) 
              { 
                PRInt32 childIndex=0;
                result = nsEditor::InsertNode(aStartNode, aNewParentNode, childIndex);
                childIndex++;
                if (NS_SUCCEEDED(result))
                { // move all the intermediate nodes into the new parent node
                  nsCOMPtr<nsIDOMNode>nextSibling;
                  while (intermediateNode.get() != aEndNode)
                  {
                    if (!intermediateNode)
                      result = NS_ERROR_NULL_POINTER;
                    if (NS_FAILED(result)) {
                      break;
                    }
                    // get the next sibling before moving the current child!!!
                    intermediateNode->GetNextSibling(getter_AddRefs(nextSibling));
                    result = nsEditor::DeleteNode(intermediateNode);
                    if (NS_SUCCEEDED(result)) {
                      result = nsEditor::InsertNode(intermediateNode, aNewParentNode, childIndex);
                      childIndex++;
                    }
                    intermediateNode = do_QueryInterface(nextSibling);
                  }
                  if (NS_SUCCEEDED(result))
                  { // move the left half of the end node into the new parent node
                    result = nsEditor::DeleteNode(newRightNode);
                    if (NS_SUCCEEDED(result)) 
                    {
                      result = nsEditor::InsertNode(newRightNode, aNewParentNode, childIndex);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return result;
}


/* this wraps every selected text node in a new inline style node if needed
   the text nodes are treated as being unique -- each needs it's own style node 
   if the style is not already present.
   each action has immediate effect on the content tree and resolved style, so
   doing outermost text nodes first removes the need for interior style nodes in some cases.
   XXX: need to code test to see if new style node is needed
*/
NS_IMETHODIMP
nsTextEditor::SetTextPropertiesForNodeWithDifferentParents(nsIDOMRange *aRange,
                                                           nsIDOMNode  *aStartNode,
                                                           PRInt32      aStartOffset,
                                                           nsIDOMNode  *aEndNode,
                                                           PRInt32      aEndOffset,
                                                           nsIDOMNode  *aParent,
                                                           nsIAtom     *aPropName,
                                                           const nsString *aAttribute,
                                                           const nsString *aValue)
{
  if (gNoisy) { printf("nsTextEditor::SetTextPropertiesForNodeWithDifferentParents\n"); }
  nsresult result=NS_OK;
  PRUint32 count;
  if (!aRange || !aStartNode || !aEndNode || !aParent || !aPropName)
    return NS_ERROR_NULL_POINTER;
  // create a style node for the text in the start parent
  nsCOMPtr<nsIDOMNode>parent;

  // create new parent nodes for all the content between the start and end nodes
  nsCOMPtr<nsIContentIterator>iter;
  result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                              kIContentIteratorIID, getter_AddRefs(iter));
  if ((NS_SUCCEEDED(result)) && iter)
  {
    nsCOMPtr<nsIContent>startContent;
    startContent = do_QueryInterface(aStartNode);
    nsCOMPtr<nsIContent>endContent;
    endContent = do_QueryInterface(aEndNode);
    if (startContent && endContent)
    {
      iter->Init(aRange);
      nsCOMPtr<nsIContent> content;
      iter->CurrentNode(getter_AddRefs(content));
      nsAutoString tag;
      aPropName->ToString(tag);
      while (NS_COMFALSE == iter->IsDone())
      {
        if ((content.get() != startContent.get()) &&
            (content.get() != endContent.get()))
        {
          nsCOMPtr<nsIDOMCharacterData>charNode;
          charNode = do_QueryInterface(content);
          if (charNode)
          {
            // only want to wrap the text node in a new style node if it doesn't already have that style
            nsCOMPtr<nsIDOMNode>node;
            node = do_QueryInterface(content);
            PRBool textPropertySet;
            nsCOMPtr<nsIDOMNode>resultNode;
            IsTextPropertySetByContent(node, aPropName, aAttribute, aValue, textPropertySet, getter_AddRefs(resultNode));
            if (PR_FALSE==textPropertySet)
            {
              charNode->GetParentNode(getter_AddRefs(parent));
              if (!parent) {
                return NS_ERROR_NULL_POINTER;
              }
              nsCOMPtr<nsIContent>parentContent;
              parentContent = do_QueryInterface(parent);
            
              PRInt32 offsetInParent;
              parentContent->IndexOf(content, offsetInParent);

              nsCOMPtr<nsIDOMNode>newStyleNode;
              result = nsEditor::CreateNode(tag, parent, offsetInParent, getter_AddRefs(newStyleNode));
              if (NS_SUCCEEDED(result) && newStyleNode) 
              {
                nsCOMPtr<nsIDOMNode>contentNode;
                contentNode = do_QueryInterface(content);
                if (!contentNode) { return NS_ERROR_NULL_POINTER;}
                result = GetLengthOfDOMNode(contentNode, count);
                if (NS_SUCCEEDED(result)) {
                  result = SetTextPropertiesForNode(contentNode, newStyleNode, 0, count, aPropName, aAttribute, aValue);
                }
              }
            }
          }
        }
        // note we don't check the result, we just rely on iter->IsDone
        iter->Next();
        iter->CurrentNode(getter_AddRefs(content));
      }
    }
  }

  // handle endpoints
  if (NS_SUCCEEDED(result))
  {
    // create a style node for the text in the start parent
    result = aStartNode->GetParentNode(getter_AddRefs(parent));
    if (NS_FAILED(result)) {
      return result;
    }
    nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
    nodeAsChar = do_QueryInterface(aStartNode);
    if (nodeAsChar)
    {   
      nodeAsChar->GetLength(&count);
      result = SetTextPropertiesForNode(aStartNode, parent, aStartOffset, count, aPropName, aAttribute, aValue);
    }

    if (NS_SUCCEEDED(result))
    {
      // create a style node for the text in the end parent
      result = aEndNode->GetParentNode(getter_AddRefs(parent));
      if (NS_FAILED(result)) {
        return result;
      }
      nodeAsChar = do_QueryInterface(aEndNode);
      if (nodeAsChar)
      {
        nodeAsChar->GetLength(&count);
        result = SetTextPropertiesForNode(aEndNode, parent, 0, aEndOffset, aPropName, aAttribute, aValue);
      }
    }
  }

  return result;
}

NS_IMETHODIMP nsTextEditor::RemoveTextPropertiesForNode(nsIDOMNode *aNode, 
                                                        nsIDOMNode *aParent,
                                                        PRInt32     aStartOffset,
                                                        PRInt32     aEndOffset,
                                                        nsIAtom    *aPropName,
                                                        const nsString *aAttribute)
{
  if (gNoisy) { printf("nsTextEditor::RemoveTextPropertyForNode\n"); }
  nsresult result=NS_OK;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nodeAsChar =  do_QueryInterface(aNode);
  PRBool textPropertySet;
  nsCOMPtr<nsIDOMNode>resultNode;
  IsTextPropertySetByContent(aNode, aPropName, aAttribute, nsnull, textPropertySet, getter_AddRefs(resultNode));
  if (PR_TRUE==textPropertySet)
  {
    nsCOMPtr<nsIDOMNode>parent; // initially set to first interior parent node to process
    nsCOMPtr<nsIDOMNode>newMiddleNode; // this will be the middle node after any required splits
    nsCOMPtr<nsIDOMNode>newLeftNode;   // this will be the leftmost node, 
                                       // the node being split will be rightmost
    PRUint32 count;
    // if aNode is a text node, treat is specially
    if (nodeAsChar)
    {
      nodeAsChar->GetLength(&count);
      // split the node, and all parent nodes up to the style node
      // then promote the selected content to the parent of the style node
      if (0!=aStartOffset) {
        if (gNoisy) { printf("* splitting text node %p at %d\n", aNode, aStartOffset);}
        result = nsEditor::SplitNode(aNode, aStartOffset, getter_AddRefs(newLeftNode));
        if (gNoisy) { printf("* split created left node %p\n", newLeftNode.get());}
        if (gNoisy) {DebugDumpContent(); } // DEBUG
      }
      if (NS_SUCCEEDED(result))
      {
        if ((PRInt32)count!=aEndOffset) {
          if (gNoisy) { printf("* splitting text node (right node) %p at %d\n", aNode, aEndOffset-aStartOffset);}
          result = nsEditor::SplitNode(aNode, aEndOffset-aStartOffset, getter_AddRefs(newMiddleNode));
          if (gNoisy) { printf("* split created middle node %p\n", newMiddleNode.get());}
          if (gNoisy) {DebugDumpContent(); } // DEBUG
        }
        else {
          if (gNoisy) { printf("* no need to split text node, middle to aNode\n");}
          newMiddleNode = do_QueryInterface(aNode);
        }
        NS_ASSERTION(newMiddleNode, "no middle node created");
        // now that the text node is split, split parent nodes until we get to the style node
        parent = do_QueryInterface(aParent);  // we know this has to succeed, no need to check
      }
    }
    else {
      newMiddleNode = do_QueryInterface(aNode);
      parent = do_QueryInterface(aParent); 
    }
    if (NS_SUCCEEDED(result) && newMiddleNode)
    {
      // split every ancestor until we find the node that is giving us the style we want to remove
      // then split the style node and promote the selected content to the style node's parent
      while (NS_SUCCEEDED(result) && parent)
      {
        if (gNoisy) { printf("* looking at parent %p\n", parent.get());}
        // get the tag from parent and see if we're done
        nsCOMPtr<nsIDOMNode>temp;
        nsCOMPtr<nsIDOMElement>element;
        element = do_QueryInterface(parent);
        if (element)
        {
          nsAutoString tag;
          result = element->GetTagName(tag);
          if (gNoisy) { printf("* parent has tag %s\n", tag.ToNewCString()); } // XXX leak!
          if (NS_SUCCEEDED(result))
          {
            if (PR_FALSE==tag.EqualsIgnoreCase(aPropName->GetUnicode()))
            {
              PRInt32 offsetInParent;
              result = GetChildOffset(newMiddleNode, parent, offsetInParent);
              if (NS_SUCCEEDED(result))
              {
                if (0!=offsetInParent) {
                  if (gNoisy) { printf("* splitting parent %p at offset %d\n", parent.get(), offsetInParent);}
                  result = nsEditor::SplitNode(parent, offsetInParent, getter_AddRefs(newLeftNode));
                  if (gNoisy) { printf("* split created left node %p sibling of parent\n", newLeftNode.get());}
                  if (gNoisy) {DebugDumpContent(); } // DEBUG
                }
                if (NS_SUCCEEDED(result))
                {
                  nsCOMPtr<nsIDOMNodeList>childNodes;
                  result = parent->GetChildNodes(getter_AddRefs(childNodes));
                  if (NS_SUCCEEDED(result) && childNodes)
                  {
                    childNodes->GetLength(&count);
                    NS_ASSERTION(count>0, "bad child count in newly split node");
                    if ((PRInt32)count!=1) 
                    {
                      if (gNoisy) { printf("* splitting parent %p at offset %d\n", parent.get(), 1);}
                      result = nsEditor::SplitNode(parent, 1, getter_AddRefs(newMiddleNode));
                      if (gNoisy) { printf("* split created middle node %p sibling of parent\n", newMiddleNode.get());}
                      if (gNoisy) {DebugDumpContent(); } // DEBUG
                    }
                    else {
                      if (gNoisy) { printf("* no need to split parent, newMiddleNode=parent\n");}
                      newMiddleNode = do_QueryInterface(parent);
                    }
                    NS_ASSERTION(newMiddleNode, "no middle node created");
                    parent->GetParentNode(getter_AddRefs(temp));
                    parent = do_QueryInterface(temp);
                  }
                }
              }
            }
            // else we've found the style tag (referred to by "parent")
            // newMiddleNode is the node that is an ancestor to the selection
            else
            {
              if (gNoisy) { printf("* this is the style node\n");}
              PRInt32 offsetInParent;
              result = GetChildOffset(newMiddleNode, parent, offsetInParent);
              if (NS_SUCCEEDED(result))
              {
                nsCOMPtr<nsIDOMNodeList>childNodes;
                result = parent->GetChildNodes(getter_AddRefs(childNodes));
                if (NS_SUCCEEDED(result) && childNodes)
                {
                  childNodes->GetLength(&count);
                  // if there are siblings to the right, split parent at offsetInParent+1
                  if ((PRInt32)count!=offsetInParent+1)
                  {
                    nsCOMPtr<nsIDOMNode>newRightNode;
                    //nsCOMPtr<nsIDOMNode>temp;
                    if (gNoisy) { printf("* splitting parent %p at offset %d for right side\n", parent.get(), offsetInParent+1);}
                    result = nsEditor::SplitNode(parent, offsetInParent+1, getter_AddRefs(temp));
                    if (NS_SUCCEEDED(result))
                    {
                      newRightNode = do_QueryInterface(parent);
                      parent = do_QueryInterface(temp);
                      if (gNoisy) { printf("* split created right node %p sibling of parent %p\n", newRightNode.get(), parent.get());}
                      if (gNoisy) {DebugDumpContent(); } // DEBUG
                    }
                  }
                  if (NS_SUCCEEDED(result) && 0!=offsetInParent) {
                    if (gNoisy) { printf("* splitting parent %p at offset %d for left side\n", parent.get(), offsetInParent);}
                    result = nsEditor::SplitNode(parent, offsetInParent, getter_AddRefs(newLeftNode));
                    if (gNoisy) { printf("* split created left node %p sibling of parent %p\n", newLeftNode.get(), parent.get());}
                    if (gNoisy) {DebugDumpContent(); } // DEBUG
                  }
                  if (NS_SUCCEEDED(result))
                  { // promote the selection to the grandparent
                    // first, determine the child's position in it's parent
                    PRInt32 childPositionInParent;
                    GetChildOffset(newMiddleNode, parent, childPositionInParent);
                    // compare childPositionInParent to the number of children in parent
                    //PRUint32 count=0;
                    //nsCOMPtr<nsIDOMNodeList>childNodes;
                    result = parent->GetChildNodes(getter_AddRefs(childNodes));
                    if (NS_SUCCEEDED(result) && childNodes) {
                      childNodes->GetLength(&count);
                    }
                    PRBool insertAfter = PR_FALSE;
                    // if they're equal, we'll insert newMiddleNode in grandParent after the parent
                    if ((PRInt32)count==childPositionInParent) {
                      insertAfter = PR_TRUE;
                    }
                    // now that we know where to put newMiddleNode, do it.
                    nsCOMPtr<nsIDOMNode>grandParent;
                    result = parent->GetParentNode(getter_AddRefs(grandParent));
                    if (NS_SUCCEEDED(result) && grandParent)
                    {
                      if (gNoisy) { printf("* deleting middle node %p\n", newMiddleNode.get());}
                      result = nsEditor::DeleteNode(newMiddleNode);
                      if (gNoisy) {DebugDumpContent(); } // DEBUG
                      if (NS_SUCCEEDED(result))
                      {
                        PRInt32 position;
                        result = GetChildOffset(parent, grandParent, position);
                        if (NS_SUCCEEDED(result)) 
                        {
                          if (PR_TRUE==insertAfter)
                          {
                            if (gNoisy) {printf("insertAfter=PR_TRUE, incr. position\n"); }
                            position++;
                          }
                          if (gNoisy) { 
                            printf("* inserting node %p in grandparent %p at offset %d\n", 
                                   newMiddleNode.get(), grandParent.get(), position);
                          }
                          result = nsEditor::InsertNode(newMiddleNode, grandParent, position);
                          if (gNoisy) {DebugDumpContent(); } // DEBUG
                          if (NS_SUCCEEDED(result)) 
                          {
                            PRBool hasChildren=PR_TRUE;
                            parent->HasChildNodes(&hasChildren);
                            if (PR_FALSE==hasChildren) {
                              if (gNoisy) { printf("* deleting empty style node %p\n", parent.get());}
                              result = nsEditor::DeleteNode(parent);
                              if (gNoisy) {DebugDumpContent(); } // DEBUG
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
  }
  return result;
}

/* this should only get called if the only intervening nodes are inline style nodes */
NS_IMETHODIMP 
nsTextEditor::RemoveTextPropertiesForNodesWithSameParent(nsIDOMNode *aStartNode,
                                                         PRInt32     aStartOffset,
                                                         nsIDOMNode *aEndNode,
                                                         PRInt32     aEndOffset,
                                                         nsIDOMNode *aParent,
                                                         nsIAtom    *aPropName,
                                                         const nsString *aAttribute)
{
  if (gNoisy) { printf("nsTextEditor::RemoveTextPropertiesForNodesWithSameParent\n"); }
  nsresult result=NS_OK;
  PRInt32 startOffset = aStartOffset;
  PRInt32 endOffset;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nsCOMPtr<nsIDOMNode>parentNode = do_QueryInterface(aParent);
  
  // remove aPropName from all intermediate nodes
  nsCOMPtr<nsIDOMNode>siblingNode;
  nsCOMPtr<nsIDOMNode>nextSiblingNode;  // temp to hold the next node in the list
  result = aStartNode->GetNextSibling(getter_AddRefs(siblingNode));
  while (siblingNode && NS_SUCCEEDED(result))
  {
    // get next sibling right away, before we move siblingNode!
    siblingNode->GetNextSibling(getter_AddRefs(nextSiblingNode));
    if (aEndNode==siblingNode.get()) {  // found the end node, handle that below
      break;
    }
    else
    { // found a sibling node between aStartNode and aEndNode, remove the style node
      PRUint32 childCount=0;
      nodeAsChar =  do_QueryInterface(siblingNode);
      if (nodeAsChar) {
        nodeAsChar->GetLength(&childCount);
      }
      else
      {
        nsCOMPtr<nsIDOMNodeList>grandChildNodes;
        result = siblingNode->GetChildNodes(getter_AddRefs(grandChildNodes));
        if (NS_SUCCEEDED(result) && grandChildNodes) {
          grandChildNodes->GetLength(&childCount);
        }
        if (0==childCount)
        { // node has no children
          // XXX: for now, I think that's ok.  just pass in 0
        }
      }
      if (NS_SUCCEEDED(result)) {
        siblingNode->GetParentNode(getter_AddRefs(parentNode));
        result = RemoveTextPropertiesForNode(siblingNode, parentNode, 0, childCount, aPropName, aAttribute);
      }
    }
    siblingNode = do_QueryInterface(nextSiblingNode);    
  }
  if (NS_SUCCEEDED(result))
  {
    // remove aPropName from aStartNode
    //nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
    nodeAsChar =  do_QueryInterface(aStartNode);
    if (nodeAsChar) {
      nodeAsChar->GetLength((PRUint32 *)&endOffset);
    }
    else
    {
      if (gNoisy) { printf("not yet supported\n");}
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    result = aStartNode->GetParentNode(getter_AddRefs(parentNode));
    if (NS_SUCCEEDED(result)) {
      result = RemoveTextPropertiesForNode(aStartNode, parentNode, startOffset, endOffset, aPropName, aAttribute);
    }
  }
  if (NS_SUCCEEDED(result))
  {
    // remove aPropName from the end node
    startOffset = 0;
    endOffset = aEndOffset;
    result = aEndNode->GetParentNode(getter_AddRefs(parentNode));
    if (NS_SUCCEEDED(result)) {
      result = RemoveTextPropertiesForNode(aEndNode, parentNode, startOffset, endOffset, aPropName, aAttribute);
    }
  }
  return result;
}

NS_IMETHODIMP
nsTextEditor::RemoveTextPropertiesForNodeWithDifferentParents(nsIDOMNode  *aStartNode,
                                                              PRInt32      aStartOffset,
                                                              nsIDOMNode  *aEndNode,
                                                              PRInt32      aEndOffset,
                                                              nsIDOMNode  *aParent,
                                                              nsIAtom     *aPropName,
                                                              const nsString *aAttribute)
{
  if (gNoisy) { printf("nsTextEditor::RemoveTextPropertiesForNodeWithDifferentParents\n"); }
  nsresult result=NS_OK;
  if (!aStartNode || !aEndNode || !aParent || !aPropName)
    return NS_ERROR_NULL_POINTER;

  PRInt32 rangeStartOffset = aStartOffset;  // used to construct a range for the nodes between
  PRInt32 rangeEndOffset = aEndOffset;      // aStartNode and aEndNode after we've processed those endpoints

  // delete the style node for the text in the start parent
  PRBool skippedStartNode = PR_FALSE;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  PRUint32 count;
  nsCOMPtr<nsIDOMNode>parent;
  result = aStartNode->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(result)) {
    return result;
  }
  nodeAsChar = do_QueryInterface(aStartNode);
  if (!nodeAsChar) { return NS_ERROR_FAILURE; }
  nodeAsChar->GetLength(&count);
  if ((PRUint32)aStartOffset!=count) {  // only do this if at least one child is selected
    result = RemoveTextPropertiesForNode(aStartNode, parent, aStartOffset, count, aPropName, aAttribute);
    if (0!=aStartOffset) {
      rangeStartOffset = 0; // we split aStartNode at aStartOffset and it is the right node now
    }
  }
  else 
  { 
    skippedStartNode = PR_TRUE;
    if (gNoisy) { printf("skipping start node because aStartOffset==count\n"); } 
  }

  // delete the style node for the text in the end parent
  if (NS_SUCCEEDED(result))
  {
    result = aEndNode->GetParentNode(getter_AddRefs(parent));
    if (NS_SUCCEEDED(result)) 
    {
      nodeAsChar = do_QueryInterface(aEndNode);
      if (!nodeAsChar) { return NS_ERROR_FAILURE; }
      nodeAsChar->GetLength(&count);
      if (aEndOffset!=0) {  // only do this if at least one child is selected
        result = RemoveTextPropertiesForNode(aEndNode, parent, 0, aEndOffset, aPropName, aAttribute);
        if (0!=aEndOffset) {
          rangeEndOffset = 0; // we split aEndNode at aEndOffset and it is the right node now
        }
      }
      else { if (gNoisy) { printf("skipping end node because aEndOffset==0\n"); } }
    }
  }

  // remove aPropName style nodes for all the content between the start and end nodes
  if (NS_SUCCEEDED(result))
  {
    // build our own range now, because the endpoints may have shifted during shipping
    nsCOMPtr<nsIDOMRange> range;
    result = nsComponentManager::CreateInstance(kCRangeCID, 
                                                nsnull, 
                                                kIDOMRangeIID, 
                                                getter_AddRefs(range));
    if (NS_FAILED(result)) { return result; }
    if (!range) { return NS_ERROR_NULL_POINTER; }
    // compute the start node
    nsCOMPtr<nsIDOMNode>startNode = do_QueryInterface(aStartNode);
    if (PR_TRUE==skippedStartNode) {
      nsEditor::GetNextNode(aStartNode, PR_TRUE, getter_AddRefs(startNode));
    }
    range->SetStart(startNode, rangeStartOffset);
    range->SetEnd(aEndNode, rangeEndOffset);
    if (gNoisy) 
    { 
      printf("created range [(%p,%d), (%p,%d)]\n", 
              aStartNode, rangeStartOffset,
              aEndNode, rangeEndOffset);
    }

    nsVoidArray nodeList;
    nsCOMPtr<nsIContentIterator>iter;
    result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                                kIContentIteratorIID, getter_AddRefs(iter));
    if ((NS_SUCCEEDED(result)) && iter)
    {
      nsCOMPtr<nsIContent>startContent;
      startContent = do_QueryInterface(aStartNode);
      nsCOMPtr<nsIContent>endContent;
      endContent = do_QueryInterface(aEndNode);
      if (startContent && endContent)
      {
        iter->Init(range);
        nsCOMPtr<nsIContent> content;
        iter->CurrentNode(getter_AddRefs(content));
        nsAutoString propName;  // the property we are removing
        aPropName->ToString(propName);
        while (NS_COMFALSE == iter->IsDone())
        {
          if ((content.get() != startContent.get()) &&
              (content.get() != endContent.get()))
          {
            nsCOMPtr<nsIDOMElement>element;
            element = do_QueryInterface(content);
            if (element)
            {
              nsString tag;
              element->GetTagName(tag);
              if (propName.EqualsIgnoreCase(tag))
              {
                if (-1==nodeList.IndexOf(content.get())) {
                  nodeList.AppendElement((void *)(content.get()));
                }
              }
            }
          }
          // note we don't check the result, we just rely on iter->IsDone
          iter->Next();
          iter->CurrentNode(getter_AddRefs(content));
        }
      }
    }

    // now delete all the style nodes we found
    if (NS_SUCCEEDED(result))
    {
      nsIContent *contentPtr;
      contentPtr = (nsIContent*)(nodeList.ElementAt(0));
      while (NS_SUCCEEDED(result) && contentPtr)
      {
        nsCOMPtr<nsIDOMNode>styleNode;
        styleNode = do_QueryInterface(contentPtr);
        // promote the children of styleNode
        nsCOMPtr<nsIDOMNode>parentNode;
        result = styleNode->GetParentNode(getter_AddRefs(parentNode));
        if (NS_SUCCEEDED(result) && parentNode)
        {
          PRInt32 position;
          result = GetChildOffset(styleNode, parentNode, position);
          if (NS_SUCCEEDED(result))
          {
            nsCOMPtr<nsIDOMNode>previousSiblingNode;
            nsCOMPtr<nsIDOMNode>childNode;
            result = styleNode->GetLastChild(getter_AddRefs(childNode));
            while (NS_SUCCEEDED(result) && childNode)
            {
              childNode->GetPreviousSibling(getter_AddRefs(previousSiblingNode));
              // explicitly delete of childNode from styleNode
              // can't just rely on DOM semantics of InsertNode doing the delete implicitly, doesn't undo! 
              result = nsEditor::DeleteNode(childNode); 
              if (NS_SUCCEEDED(result))
              {
                result = nsEditor::InsertNode(childNode, parentNode, position);
                if (gNoisy) 
                {
                  printf("deleted next sibling node %p\n", childNode.get());
                  DebugDumpContent(); // DEBUG
                }
              }
              childNode = do_QueryInterface(previousSiblingNode);        
            } // end while loop 
            // delete styleNode
            result = nsEditor::DeleteNode(styleNode);
            if (gNoisy) 
            {
              printf("deleted style node %p\n", styleNode.get());
              DebugDumpContent(); // DEBUG
            }
          }
        }

        // get next content ptr
        nodeList.RemoveElementAt(0);
        contentPtr = (nsIContent*)(nodeList.ElementAt(0));
      }
    }
  }

  return result;
}

TypeInState * nsTextEditor::GetTypeInState()
{
  if (mTypeInState) {
    NS_ADDREF(mTypeInState);
  }
  return mTypeInState;
}

NS_IMETHODIMP
nsTextEditor::SetTypeInStateForProperty(TypeInState    &aTypeInState, 
                                        nsIAtom        *aPropName, 
                                        const nsString *aAttribute,
                                        const nsString *aValue)
{
  if (!aPropName) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 propEnum;
  aTypeInState.GetEnumForName(aPropName, propEnum);
  if (nsIEditProperty::b==aPropName || nsIEditProperty::i==aPropName || nsIEditProperty::u==aPropName) 
  {
    if (PR_TRUE==aTypeInState.IsSet(propEnum))
    { // toggle currently set boldness
      aTypeInState.UnSet(propEnum);
    }
    else
    { // get the current style and set boldness to the opposite of the current state
      PRBool any = PR_FALSE;
      PRBool all = PR_FALSE;
      PRBool first = PR_FALSE;
      GetTextProperty(aPropName, aAttribute, nsnull, first, any, all); // operates on current selection
      aTypeInState.SetProp(propEnum, !any);
    }
  }
  else if (nsIEditProperty::font==aPropName)
  {
    if (!aAttribute) { return NS_ERROR_NULL_POINTER; }
    nsIAtom *attribute = NS_NewAtom(*aAttribute);
    if (!attribute) { return NS_ERROR_NULL_POINTER; }
    PRUint32 attrEnum;
    aTypeInState.GetEnumForName(attribute, attrEnum);
    if (nsIEditProperty::color==attribute || nsIEditProperty::face==attribute || nsIEditProperty::size==attribute)
    {
      if (PR_TRUE==aTypeInState.IsSet(attrEnum))
      { 
        if (nsnull==aValue) {
          aTypeInState.UnSet(attrEnum);
        }
        else { // we're just changing the value of color
          aTypeInState.SetPropValue(attrEnum, *aValue);
        }
      }
      else
      { // get the current style and set font color if it's needed
        if (!aValue) { return NS_ERROR_NULL_POINTER; }
        PRBool any = PR_FALSE;
        PRBool all = PR_FALSE;
        PRBool first = PR_FALSE;
        GetTextProperty(aPropName, aAttribute, aValue, first, any, all); // operates on current selection
        if (PR_FALSE==all) {
          aTypeInState.SetPropValue(attrEnum, *aValue);
        }
      }    
    }
    else { return NS_ERROR_FAILURE; }
  }
  else { return NS_ERROR_FAILURE; }
  return NS_OK;
}

// This file should be rearranged to put all methods that simply call nsEditor together
NS_IMETHODIMP
nsTextEditor::CopyAttributes(nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  return nsEditor::CopyAttributes(aDestNode, aSourceNode);
}

NS_IMETHODIMP
nsTextEditor::BeginComposition(void)
{
	return nsEditor::BeginComposition();
}

NS_IMETHODIMP
nsTextEditor::SetCompositionString(const nsString& aCompositionString)
{
	return nsEditor::SetCompositionString(aCompositionString);
}

NS_IMETHODIMP
nsTextEditor::EndComposition(void)
{
	return nsEditor::EndComposition();
}
