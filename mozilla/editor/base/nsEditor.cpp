/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
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


#include "nsIDOMDocument.h"
#include "nsEditor.h"
#include "nsIEditProperty.h"  // to be removed  XXX
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNode.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIDocument.h"
#include "nsIDiskDocument.h"
#include "nsVector.h"
#include "nsIServiceManager.h"
#include "nsEditFactory.h"
#include "nsTextEditFactory.h"
#include "nsHTMLEditFactory.h"
#include "nsEditorCID.h"
#include "nsTransactionManagerCID.h"
#include "nsITransactionManager.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIDOMSelection.h"
#include "nsIEnumerator.h"
#include "nsIAtom.h"
#include "nsVoidArray.h"
#include "nsISupportsArray.h"
#include "nsICaret.h"
#include "nsIStyleContext.h"

#include "nsIEditActionListener.h"

#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsLayoutCID.h"

// transactions the editor knows how to build
#include "TransactionFactory.h"
#include "EditAggregateTxn.h"
#include "ChangeAttributeTxn.h"
#include "CreateElementTxn.h"
#include "InsertElementTxn.h"
#include "DeleteElementTxn.h"
#include "InsertTextTxn.h"
#include "DeleteTextTxn.h"
#include "DeleteRangeTxn.h"
#include "SplitElementTxn.h"
#include "JoinElementTxn.h"
#include "nsIStringStream.h"

// #define HACK_FORCE_REDRAW 1


#ifdef HACK_FORCE_REDRAW
// INCLUDES FOR EVIL HACK TO FOR REDRAW
// BEGIN
#include "nsIViewManager.h"
#include "nsIView.h"
// END
#endif

// Drag & Drop, Clipboard
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsITransferable.h"
#include "nsIFormatConverter.h"

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kCClipboardCID,           NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
static NS_DEFINE_IID(kCXIFFormatConverterCID,  NS_XIFFORMATCONVERTER_CID);

static NS_DEFINE_CID(kCRangeCID,            NS_RANGE_CID);
static NS_DEFINE_CID(kEditorCID,            NS_EDITOR_CID);
static NS_DEFINE_CID(kTextEditorCID,        NS_TEXTEDITOR_CID);
static NS_DEFINE_CID(kHTMLEditorCID,        NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
// transaction manager
static NS_DEFINE_CID(kCTransactionManagerCID, NS_TRANSACTIONMANAGER_CID);

// transactions
static NS_DEFINE_IID(kEditAggregateTxnIID,  EDIT_AGGREGATE_TXN_IID);
static NS_DEFINE_IID(kInsertTextTxnIID,     INSERT_TEXT_TXN_IID);
static NS_DEFINE_IID(kDeleteTextTxnIID,     DELETE_TEXT_TXN_IID);
static NS_DEFINE_IID(kCreateElementTxnIID,  CREATE_ELEMENT_TXN_IID);
static NS_DEFINE_IID(kInsertElementTxnIID,  INSERT_ELEMENT_TXN_IID);
static NS_DEFINE_IID(kDeleteElementTxnIID,  DELETE_ELEMENT_TXN_IID);
static NS_DEFINE_IID(kDeleteRangeTxnIID,    DELETE_RANGE_TXN_IID);
static NS_DEFINE_IID(kChangeAttributeTxnIID,CHANGE_ATTRIBUTE_TXN_IID);
static NS_DEFINE_IID(kSplitElementTxnIID,   SPLIT_ELEMENT_TXN_IID);
static NS_DEFINE_IID(kJoinElementTxnIID,    JOIN_ELEMENT_TXN_IID);

static NS_DEFINE_CID(kComponentManagerCID,  NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kCDOMRangeCID, NS_RANGE_CID);

// factory classes
static NS_DEFINE_IID(kIEditFactoryIID, NS_IEDITORFACTORY_IID);
static NS_DEFINE_IID(kIHTMLEditFactoryIID, NS_IHTMLEDITORFACTORY_IID);
static NS_DEFINE_IID(kITextEditFactoryIID, NS_ITEXTEDITORFACTORY_IID);


#ifdef XP_PC
#define TRANSACTION_MANAGER_DLL "txmgr.dll"
#else
#ifdef XP_MAC
#define TRANSACTION_MANAGER_DLL "TRANSACTION_MANAGER_DLL"
#else // XP_UNIX
#define TRANSACTION_MANAGER_DLL "libtxmgr.so"
#endif
#endif

#define NS_ERROR_EDITOR_NO_SELECTION NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,1)
#define NS_ERROR_EDITOR_NO_TEXTNODE  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_EDITOR,2)

const char* nsEditor::kMOZEditorBogusNodeAttr="MOZ_EDITOR_BOGUS_NODE";
const char* nsEditor::kMOZEditorBogusNodeValue="TRUE";

#ifdef NS_DEBUG_EDITOR
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif



/* ----- TEST METHODS DECLARATIONS ----- */
// Methods defined here are TEMPORARY
//NS_IMETHODIMP  GetColIndexForCell(nsIPresShell *aPresShell, nsIDOMNode *aCellNode, PRInt32 &aCellIndex);
/* ----- END TEST METHOD DECLARATIONS ----- */


PRInt32 nsEditor::gInstanceCount = 0;

//monitor for the editor



PRMonitor *getEditorMonitor() //if more than one person asks for the monitor at the same time for the FIRST time, we are screwed
{
  static PRMonitor *ns_editlock = nsnull;
  if (nsnull == ns_editlock)
  {
    ns_editlock = (PRMonitor *)1; //how long will the next line take?  lets cut down on the chance of reentrancy
    ns_editlock = PR_NewMonitor();
  }
  else if ((PRMonitor *)1 == ns_editlock)
    return getEditorMonitor();
  return ns_editlock;
}

nsIComponentManager* gCompMgr = NULL;

/*
we must be good providers of factories etc. this is where to put ALL editor exports
*/
//BEGIN EXPORTS
extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports * aServMgr, 
                                           const nsCID & aClass, 
                                           const char * aClassName,
                                           const char * aProgID,
                                           nsIFactory ** aFactory)
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = nsnull;

  nsresult rv;
  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  rv = servMgr->GetService(kComponentManagerCID, nsIComponentManager::GetIID(), 
                         (nsISupports**)&gCompMgr);
  if (NS_FAILED(rv)) return rv;

  rv = NS_NOINTERFACE;
  if (aClass.Equals(kEditorCID)) {
    rv = GetEditFactory(aFactory, aClass);
    if (NS_FAILED(rv)) goto done;
  }
  else if (aClass.Equals(kTextEditorCID)) {
    rv = GetTextEditFactory(aFactory, aClass);
    if (NS_FAILED(rv)) goto done;
  }
  else if (aClass.Equals(kHTMLEditorCID)) {
    rv = GetHTMLEditFactory(aFactory, aClass);
    if (NS_FAILED(rv)) goto done;
  }
  
  done:
  (void)servMgr->ReleaseService(kComponentManagerCID, gCompMgr);

  return rv;
}



extern "C" NS_EXPORT PRBool
NSCanUnload(nsISupports* aServMgr)
{
  return nsEditor::gInstanceCount; //I have no idea. I am copying code here
}



extern "C" NS_EXPORT nsresult 
NSRegisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;
  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsIComponentManager* compMgr;
  rv = servMgr->GetService(kComponentManagerCID, 
                           nsIComponentManager::GetIID(), 
                           (nsISupports**)&compMgr);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kEditorCID, NULL, NULL, path, 
                                  PR_TRUE, PR_TRUE);
  if (NS_FAILED(rv)) goto done;
  rv = compMgr->RegisterComponent(kTextEditorCID, NULL, NULL, path, 
                                  PR_TRUE, PR_TRUE);
  if (NS_FAILED(rv)) goto done;
  rv = compMgr->RegisterComponent(kHTMLEditorCID, NULL, NULL, path, 
                                  PR_TRUE, PR_TRUE);

  done:
  (void)servMgr->ReleaseService(kComponentManagerCID, compMgr);
  return rv;
}

extern "C" NS_EXPORT nsresult 
NSUnregisterSelf(nsISupports* aServMgr, const char *path)
{
  nsresult rv;

  nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;

  nsIComponentManager* compMgr;
  rv = servMgr->GetService(kComponentManagerCID, 
                           nsIComponentManager::GetIID(), 
                           (nsISupports**)&compMgr);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->UnregisterComponent(kIEditFactoryIID, path);
  if (NS_FAILED(rv)) goto done;
  rv = compMgr->UnregisterComponent(kITextEditFactoryIID, path);
  if (NS_FAILED(rv)) goto done;
  rv = compMgr->UnregisterComponent(kIHTMLEditFactoryIID, path);

  done:
  (void)servMgr->ReleaseService(kComponentManagerCID, compMgr);
  return rv;
}

//END EXPORTS


//class implementations are in order they are declared in nsEditor.h

nsEditor::nsEditor()
:  mPresShell(nsnull)
,  mViewManager(nsnull)
,  mUpdateCount(0)
,  mActionListeners(nsnull)
,  mDoc(nsnull)
{
  //initialize member variables here
  NS_INIT_REFCNT();
  mIMEFirstTransaction=PR_FALSE;
  PR_EnterMonitor(getEditorMonitor());
  gInstanceCount++;
  mActionListeners = 0;
  PR_ExitMonitor(getEditorMonitor());

}



nsEditor::~nsEditor()
{
  if (mActionListeners)
  {
    PRInt32 i;
    nsIEditActionListener *listener;

    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      NS_IF_RELEASE(listener);
    }

    delete mActionListeners;
    mActionListeners = 0;
  }
}



// BEGIN nsEditor core implementation


NS_IMPL_ADDREF(nsEditor)

NS_IMPL_RELEASE(nsEditor)



NS_IMETHODIMP
nsEditor::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsISupports::GetIID())) {
    nsIEditor *tmp = this;
    nsISupports *tmp2 = tmp;
    *aInstancePtr = (void*)tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(nsIEditor::GetIID())) {
    *aInstancePtr = (void*)(nsIEditor*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP 
nsEditor::GetDocument(nsIDOMDocument **aDoc)
{
  if (!aDoc)
    return NS_ERROR_NULL_POINTER;
  *aDoc = nsnull; // init out param
  NS_PRECONDITION(mDoc, "bad state, null mDoc");
  if (!mDoc)
    return NS_ERROR_NOT_INITIALIZED;
  return mDoc->QueryInterface(nsIDOMDocument::GetIID(), (void **)aDoc);
}

// This seems like too much work! There should be a "nsDOMDocument::GetBody()"
NS_IMETHODIMP 
nsEditor::GetBodyElement(nsIDOMElement **aBodyElement)
{
  nsresult result;

  if (!aBodyElement)
    return NS_ERROR_NULL_POINTER;

  *aBodyElement = 0;
  
  NS_PRECONDITION(mDoc, "bad state, null mDoc");
  if (!mDoc)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMNodeList>nodeList; 
  nsString bodyTag = "body"; 

  result = mDoc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));

  if (NS_FAILED(result))
    return result;
  
  if (!nodeList)
    return NS_ERROR_NULL_POINTER;

  PRUint32 count; 
  nodeList->GetLength(&count);

  NS_ASSERTION(1==count, "More than one body found in document!"); 

  if (count < 1)
    return NS_ERROR_FAILURE;

  // Use the first body node in the list:
  nsCOMPtr<nsIDOMNode> node;
  result = nodeList->Item(0, getter_AddRefs(node)); 
  if (NS_SUCCEEDED(result) && node)
  {
    //return node->QueryInterface(nsIDOMElement::GetIID(), (void **)aBodyElement);
    // Is above equivalent to this:
    nsCOMPtr<nsIDOMElement> bodyElement = do_QueryInterface(node);
    if (bodyElement)
      *aBodyElement = bodyElement;
  }
  return result;
}

nsresult 
nsEditor::GetPresShell(nsIPresShell **aPS)
{
  if (!aPS)
    return NS_ERROR_NULL_POINTER;
  *aPS = nsnull; // init out param
  NS_PRECONDITION(mPresShell, "bad state, null mPresShell");
  if (!mPresShell)
    return NS_ERROR_NOT_INITIALIZED;
  return mPresShell->QueryInterface(nsIPresShell::GetIID(), (void **)aPS);
}


NS_IMETHODIMP
nsEditor::GetSelection(nsIDOMSelection **aSelection)
{
  if (!aSelection)
    return NS_ERROR_NULL_POINTER;
  *aSelection = nsnull;
  nsresult result = mPresShell->GetSelection(aSelection);  // does an addref
  return result;
}

NS_IMETHODIMP
nsEditor::Init(nsIDOMDocument *aDoc, nsIPresShell* aPresShell)
{
  NS_PRECONDITION(nsnull!=aDoc && nsnull!=aPresShell, "bad arg");
  if ((nsnull==aDoc) || (nsnull==aPresShell))
    return NS_ERROR_NULL_POINTER;

  mDoc = aDoc;
  mPresShell = aPresShell;		// we don't addref the pres shell
  mPresShell->GetViewManager(&mViewManager);
  mPresShell->SetDisplayNonTextSelection(PR_TRUE);//we want to see all the selection reflected to user
  if (mViewManager){
    mViewManager->Release(); //we want a weak link
  }
  mUpdateCount=0;
  InsertTextTxn::ClassInit();

  /* Show the caret */
  nsCOMPtr<nsICaret>	caret;
  if (NS_SUCCEEDED(mPresShell->GetCaret(getter_AddRefs(caret))))
  {
  	caret->SetCaretVisible(PR_TRUE);
  	caret->SetCaretReadOnly(PR_FALSE);
  }

	mPresShell->SetCaretEnabled(PR_TRUE);

  NS_POSTCONDITION(mDoc && mPresShell, "bad state");

  return NS_OK;
}

// #define DEBUG_WITH_JS_EDITOR_LOG
#ifdef DEBUG_WITH_JS_EDITOR_LOG
#include "nsJSEditorLog.h"
#endif

NS_IMETHODIMP
nsEditor::EnableUndo(PRBool aEnable)
{
  nsresult result=NS_OK;

  if (PR_TRUE==aEnable)
  {
    if (!mTxnMgr)
    {
      result = gCompMgr->CreateInstance(kCTransactionManagerCID,
                                        nsnull,
                                        nsITransactionManager::GetIID(), getter_AddRefs(mTxnMgr));
      if (NS_FAILED(result) || !mTxnMgr) {
        printf("ERROR: Failed to get TransactionManager instance.\n");
        return NS_ERROR_NOT_AVAILABLE;
      }

#ifdef DEBUG_WITH_JS_EDITOR_LOG
      nsJSEditorLog *log = new nsJSEditorLog();
      if (log)
      {
        NS_ADDREF(log);
        result = mTxnMgr->AddListener(log);
        NS_RELEASE(log);
      }
#endif
    }
  }
  else
  { // disable the transaction manager if it is enabled
    if (mTxnMgr)
    {
      mTxnMgr = do_QueryInterface(nsnull);
    }
  }

  return result;
}

NS_IMETHODIMP nsEditor::CanUndo(PRBool &aIsEnabled, PRBool &aCanUndo)
{
  aIsEnabled = ((PRBool)((nsITransactionManager *)0!=mTxnMgr.get()));
  if (aIsEnabled)
  {
    PRInt32 numTxns=0;
    mTxnMgr->GetNumberOfUndoItems(&numTxns);
    aCanUndo = ((PRBool)(0==numTxns));
  }
  else {
    aCanUndo = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsEditor::CanRedo(PRBool &aIsEnabled, PRBool &aCanRedo)
{
  aIsEnabled = ((PRBool)((nsITransactionManager *)0!=mTxnMgr.get()));
  if (aIsEnabled)
  {
    PRInt32 numTxns=0;
    mTxnMgr->GetNumberOfRedoItems(&numTxns);
    aCanRedo = ((PRBool)(0==numTxns));
  }
  else {
    aCanRedo = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetProperties(nsVoidArray * aPropList)
{
  return NS_OK;
}



NS_IMETHODIMP
nsEditor::GetProperties(nsVoidArray *aPropList)
{
  return NS_OK;
}


NS_IMETHODIMP 
nsEditor::SetAttribute(nsIDOMElement *aElement, const nsString& aAttribute, const nsString& aValue)
{
  ChangeAttributeTxn *txn;
  nsresult result = CreateTxnForSetAttribute(aElement, aAttribute, aValue, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }
  return result;
}


NS_IMETHODIMP 
nsEditor::CreateTxnForSetAttribute(nsIDOMElement *aElement, 
                                   const nsString& aAttribute, 
                                   const nsString& aValue,
                                   ChangeAttributeTxn ** aTxn)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (nsnull != aElement)
  {
    result = TransactionFactory::GetNewTransaction(kChangeAttributeTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(this, aElement, aAttribute, aValue, PR_FALSE);
    }
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::GetAttributeValue(nsIDOMElement *aElement, 
                            const nsString& aAttribute, 
                            nsString& aResultValue, 
                            PRBool&   aResultIsSet)
{
  aResultIsSet=PR_FALSE;
  nsresult result=NS_OK;
  if (nsnull!=aElement)
  {
    nsCOMPtr<nsIDOMAttr> attNode;
    result = aElement->GetAttributeNode(aAttribute, getter_AddRefs(attNode));
    if ((NS_SUCCEEDED(result)) && attNode)
    {
      attNode->GetSpecified(&aResultIsSet);
      attNode->GetValue(aResultValue);
    }
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::RemoveAttribute(nsIDOMElement *aElement, const nsString& aAttribute)
{
  ChangeAttributeTxn *txn;
  nsresult result = CreateTxnForRemoveAttribute(aElement, aAttribute, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::CreateTxnForRemoveAttribute(nsIDOMElement *aElement, 
                                      const nsString& aAttribute,
                                      ChangeAttributeTxn ** aTxn)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (nsnull != aElement)
  {
    result = TransactionFactory::GetNewTransaction(kChangeAttributeTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  
    {
      nsAutoString value;
      result = (*aTxn)->Init(this, aElement, aAttribute, value, PR_TRUE);
    }
  }
  return result;
}

// Objects must be DOM elements
NS_IMETHODIMP
nsEditor::CopyAttributes(nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  nsresult result=NS_OK;

  if (!aDestNode || !aSourceNode)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMElement> destElement = do_QueryInterface(aDestNode);
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(aSourceNode);
  if (!destElement || !sourceElement)
    return NS_ERROR_NO_INTERFACE;

  nsAutoString name;
  nsAutoString value;
  nsCOMPtr<nsIDOMNamedNodeMap> sourceAttributes;
  sourceElement->GetAttributes(getter_AddRefs(sourceAttributes));
  nsCOMPtr<nsIDOMNamedNodeMap> destAttributes;
  destElement->GetAttributes(getter_AddRefs(destAttributes));
  if (!sourceAttributes || !destAttributes)
    return NS_ERROR_FAILURE;

  PRUint32 sourceCount;
  sourceAttributes->GetLength(&sourceCount);
  PRUint32 i, destCount;
  destAttributes->GetLength(&destCount);
  nsIDOMNode* attrNode;

  // Clear existing attributes
  for (i = 0; i < destCount; i++)
  {
    if( NS_SUCCEEDED(destAttributes->Item(i, &attrNode)) && attrNode)
    {
      nsCOMPtr<nsIDOMAttr> destAttribute = do_QueryInterface(attrNode);
      if (destAttribute)
      {
        nsCOMPtr<nsIDOMAttr> resultAttribute;
        destElement->RemoveAttributeNode(destAttribute, getter_AddRefs(resultAttribute));
        // Is the resultAttribute deleted automagically?
      }
    }
  }
  // Set just the attributes that the source element has
  for (i = 0; i < sourceCount; i++)
  {
    if( NS_SUCCEEDED(sourceAttributes->Item(i, &attrNode)) && attrNode)
    {
      nsCOMPtr<nsIDOMAttr> sourceAttribute = do_QueryInterface(attrNode);
      if (sourceAttribute)
      {
        nsString sourceAttrName;
        if (NS_SUCCEEDED(sourceAttribute->GetName(sourceAttrName)))
        {
          nsString sourceAttrValue;
          if (NS_SUCCEEDED(sourceAttribute->GetValue(sourceAttrValue)) &&
              sourceAttrValue != "")
          {
            destElement->SetAttribute(sourceAttrName, sourceAttrValue);
          } else {
            // Do we ever get here?
            destElement->RemoveAttribute(sourceAttrName);
#if DEBUG_cmanske
            printf("Attribute in NamedNodeMap has empty value in nsEditor::CopyAttributes()\n");
#endif
          }
        }        
      }
    }
  }
  return result;
}

NS_IMETHODIMP
nsEditor::InsertBreak()
{
  return NS_OK;
}
 

//END nsIEditorInterfaces


//BEGIN nsEditor Private methods

NS_IMETHODIMP 
nsEditor::DoAfterDoTransaction(nsITransaction *aTxn)
{
  nsresult rv = NS_OK;
  
  PRBool  isTransientTransaction;
  rv = aTxn->GetIsTransient(&isTransientTransaction);
  if (NS_FAILED(rv))
    return rv;
  
  if (!isTransientTransaction)
  {
    rv = IncDocModCount(+1);		// don't count transient transactions
  }
  
  return rv;
}


NS_IMETHODIMP 
nsEditor::DoAfterUndoTransaction()
{
  nsresult rv = NS_OK;

  rv = IncDocModCount(-1);		// all undoable transactions are non-transient

  return rv;
}

NS_IMETHODIMP 
nsEditor::Do(nsITransaction *aTxn)
{
  if (gNoisy) { printf("Editor::Do ----------\n"); }
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMSelection>selection;
  nsresult selectionResult = GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(selectionResult) && selection) {
    selection->StartBatchChanges();
    if (aTxn)
    {
      if (mTxnMgr) {
        result = mTxnMgr->Do(aTxn);
      }
      else {
        result = aTxn->Do();
      }
      
      if (NS_SUCCEEDED(result))
        result = DoAfterDoTransaction(aTxn);
    }
    
    selection->EndBatchChanges();
  }

  return result;
}

NS_IMETHODIMP 
nsEditor::Undo(PRUint32 aCount)
{
  if (gNoisy) { printf("Editor::Undo ----------\n"); }
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMSelection>selection;
  nsresult selectionResult = GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(selectionResult) && selection) {
    selection->StartBatchChanges();
    if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
    {
      PRUint32 i=0;
      for ( ; i<aCount; i++)
      {
        result = mTxnMgr->Undo();

        if (NS_SUCCEEDED(result))
	        result = DoAfterUndoTransaction();
	        
        if (NS_FAILED(result))
          break;
      }
    }
    selection->EndBatchChanges();
  }

  return result;
}

NS_IMETHODIMP 
nsEditor::Redo(PRUint32 aCount)
{
  if (gNoisy) { printf("Editor::Redo ----------\n"); }
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMSelection>selection;
  nsresult selectionResult = GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(selectionResult) && selection) {
    selection->StartBatchChanges();
    if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
    {
      PRUint32 i=0;
      for ( ; i<aCount; i++)
      {
        result = mTxnMgr->Redo();
        if (NS_FAILED(result))
          break;
      }
    }
    selection->EndBatchChanges();
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::BeginTransaction()
{
  NS_PRECONDITION(mUpdateCount>=0, "bad state");

  nsCOMPtr<nsIDOMSelection>selection;
  nsresult selectionResult = GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(selectionResult) && selection) {
    selection->StartBatchChanges();
  }

  if (nsnull!=mViewManager)
  {
    if (0==mUpdateCount)
    {
#ifdef HACK_FORCE_REDRAW
      mViewManager->DisableRefresh();
#else
      mViewManager->BeginUpdateViewBatch();
#endif
    }
    mUpdateCount++;
  }

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    mTxnMgr->BeginBatch();
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsEditor::EndTransaction()
{
  NS_PRECONDITION(mUpdateCount>0, "bad state");

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
    mTxnMgr->EndBatch();
  }

  if (nsnull!=mViewManager)
  {
    mUpdateCount--;
    if (0==mUpdateCount)
    {
#ifdef HACK_FORCE_REDRAW
      mViewManager->EnableRefresh();
      HACKForceRedraw();
#else
      mViewManager->EndUpdateViewBatch();
#endif
    }
  }  

  nsCOMPtr<nsIDOMSelection>selection;
  nsresult selectionResult = GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(selectionResult) && selection) {
    selection->EndBatchChanges();
  }

  return NS_OK;
}

NS_IMETHODIMP nsEditor::ScrollIntoView(PRBool aScrollToBegin)
{
  return NS_OK;
}

// XXX: the rule system should tell us which node to select all on (ie, the root, or the body)
NS_IMETHODIMP nsEditor::SelectAll()
{
  if (!mDoc || !mPresShell) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMSelection> selection;
  nsresult result = mPresShell->GetSelection(getter_AddRefs(selection));
  if (NS_SUCCEEDED(result) && selection)
  {
    nsCOMPtr<nsIDOMNodeList>nodeList;
    nsAutoString bodyTag = "body";
    result = mDoc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
    if ((NS_SUCCEEDED(result)) && nodeList)
    {
      PRUint32 count;
      nodeList->GetLength(&count);
      NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
      nsCOMPtr<nsIDOMNode>bodyNode;
      result = nodeList->Item(0, getter_AddRefs(bodyNode));
      if ((NS_SUCCEEDED(result)) && bodyNode)
      {
        selection->Collapse(bodyNode, 0);
        PRInt32 numBodyChildren=0;
        nsCOMPtr<nsIDOMNode>lastChild;
        result = bodyNode->GetLastChild(getter_AddRefs(lastChild));
        if ((NS_SUCCEEDED(result)) && lastChild)
        {
          GetChildOffset(lastChild, bodyNode, numBodyChildren);
          selection->Extend(bodyNode, numBodyChildren+1);
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::Cut()
{
  nsCOMPtr<nsIDOMSelection> selection;
  nsresult res = mPresShell->GetSelection(getter_AddRefs(selection));
  if (!NS_SUCCEEDED(res))
    return res;

  PRBool isCollapsed;
  if (NS_SUCCEEDED(selection->GetIsCollapsed(&isCollapsed)) && isCollapsed)
    return NS_ERROR_NOT_AVAILABLE;

  res = Copy();
  if (NS_SUCCEEDED(res))
    res = DeleteSelection(eDoNothing);
  return res;
}


NS_IMETHODIMP nsEditor::Copy()
{
  //printf("nsEditor::Copy\n");

  return mPresShell->DoCopy();
}

NS_IMETHODIMP nsEditor::Paste()
{
  //printf("nsEditor::Paste\n");
  nsString stuffToPaste;

  // Get Clipboard Service
  nsIClipboard* clipboard;
  nsresult rv = nsServiceManager::GetService(kCClipboardCID,
                                             nsIClipboard::GetIID(),
                                             (nsISupports **)&clipboard);

  // Create generic Transferable for getting the data
  nsCOMPtr<nsITransferable> trans;
  rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                          nsITransferable::GetIID(), 
                                          (void**) getter_AddRefs(trans));
  if (NS_OK == rv) {
    // Get the nsITransferable interface for getting the data from the clipboard
    if (trans) {
      // Create the desired DataFlavor for the type of data we want to get out of the transferable
      nsAutoString flavor(kTextMime);

      trans->AddDataFlavor(&flavor);

      // Get the Data from the clipboard
      clipboard->GetData(trans);

      // Now we ask the transferable for the data
      // it still owns the data, we just have a pointer to it.
      // If it can't support a "text" output of the data the call will fail
      char *str = 0;
      PRUint32 len;
      if (NS_OK == trans->GetTransferData(&flavor, (void **)&str, &len)) {

        // Make adjustments for null terminated strings
        if (str && len > 0) {
          // stuffToPaste is ready for insertion into the content
          stuffToPaste.SetString(str, len);
        }
      }
    }
  }
  nsServiceManager::ReleaseService(kCClipboardCID, clipboard);

  //printf("Trying to insert '%s'\n", stuffToPaste.ToNewCString());

  // Now let InsertText handle the hard stuff:
  return InsertText(stuffToPaste);
}

NS_IMETHODIMP nsEditor::PasteAsQuotation()
{
#ifdef DEBUG
  printf("nsEditor::PasteAsQuotation() not meaningful, shouldn't be here\n");
#endif
  return Paste();
}

NS_IMETHODIMP nsEditor::InsertAsQuotation(const nsString& aQuotedText)
{
#ifdef DEBUG
  printf("nsEditor::PasteAsQuotation() not meaningful, shouldn't be here\n");
#endif
  return InsertText(aQuotedText);
}

NS_IMETHODIMP
nsEditor::AddEditActionListener(nsIEditActionListener *aListener)
{
  if (!aListener)
    return NS_ERROR_NULL_POINTER;

  if (!mActionListeners)
  {
    mActionListeners = new nsVoidArray();

    if (!mActionListeners)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!mActionListeners->AppendElement((void *)aListener))
    return NS_ERROR_FAILURE;

  NS_ADDREF(aListener);

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::RemoveEditActionListener(nsIEditActionListener *aListener)
{
  if (!aListener || !mActionListeners)
    return NS_ERROR_FAILURE;

  if (!mActionListeners->RemoveElement((void *)aListener))
    return NS_ERROR_FAILURE;

  NS_IF_RELEASE(aListener);

  if (mActionListeners->Count() < 1)
  {
    delete mActionListeners;
    mActionListeners = 0;
  }

  return NS_OK;
}


nsString & nsIEditor::GetTextNodeTag()
{
  static nsString gTextNodeTag("special text node tag");
  return gTextNodeTag;
}


NS_IMETHODIMP nsEditor::CreateNode(const nsString& aTag,
                                   nsIDOMNode *    aParent,
                                   PRInt32         aPosition,
                                   nsIDOMNode **   aNewNode)
{
  CreateElementTxn *txn;
  nsresult result = CreateTxnForCreateElement(aTag, aParent, aPosition, &txn);
  if (NS_SUCCEEDED(result)) 
  {
    result = Do(txn);  
    if (NS_SUCCEEDED(result)) 
    {
      result = txn->GetNewNode(aNewNode);
      NS_ASSERTION((NS_SUCCEEDED(result)), "GetNewNode can't fail if txn::Do succeeded.");
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForCreateElement(const nsString& aTag,
                                                  nsIDOMNode     *aParent,
                                                  PRInt32         aPosition,
                                                  CreateElementTxn ** aTxn)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (nsnull != aParent)
  {
    result = TransactionFactory::GetNewTransaction(kCreateElementTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(this, aTag, aParent, aPosition);
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::InsertNode(nsIDOMNode * aNode,
                                   nsIDOMNode * aParent,
                                   PRInt32      aPosition)
{
  PRInt32 i;
  nsIEditActionListener *listener;

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->WillInsertNode(aNode, aParent, aPosition);
    }
  }

  InsertElementTxn *txn;
  nsresult result = CreateTxnForInsertElement(aNode, aParent, aPosition, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->DidInsertNode(aNode, aParent, aPosition, result);
    }
  }

  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForInsertElement(nsIDOMNode * aNode,
                                                  nsIDOMNode * aParent,
                                                  PRInt32      aPosition,
                                                  InsertElementTxn ** aTxn)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (aNode && aParent && aTxn)
  {
    result = TransactionFactory::GetNewTransaction(kInsertElementTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result)) {
      result = (*aTxn)->Init(aNode, aParent, aPosition, this);
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::DeleteNode(nsIDOMNode * aElement)
{
  PRInt32 i;
  nsIEditActionListener *listener;

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->WillDeleteNode(aElement);
    }
  }

  DeleteElementTxn *txn;
  nsresult result = CreateTxnForDeleteElement(aElement, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->DidDeleteNode(aElement, result);
    }
  }

  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForDeleteElement(nsIDOMNode * aElement,
                                             DeleteElementTxn ** aTxn)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (nsnull != aElement)
  {
    result = TransactionFactory::GetNewTransaction(kDeleteElementTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result)) {
      result = (*aTxn)->Init(aElement);
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::CreateAggregateTxnForDeleteSelection(nsIAtom *aTxnName, EditAggregateTxn **aAggTxn) 
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (aAggTxn)
  {
    *aAggTxn = nsnull;
    result = TransactionFactory::GetNewTransaction(kEditAggregateTxnIID, (EditTxn**)aAggTxn); 

    if (NS_FAILED(result) || !*aAggTxn) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    // Set the name for the aggregate transaction  
    (*aAggTxn)->SetName(aTxnName);

    // Get current selection and setup txn to delete it,
    //  but only if selection exists (is not a collapsed "caret" state)
    nsCOMPtr<nsIDOMSelection> selection;
    result = mPresShell->GetSelection(getter_AddRefs(selection));
    if (NS_SUCCEEDED(result) && selection)
    {
      PRBool collapsed;
      result = selection->GetIsCollapsed(&collapsed);
      if (NS_SUCCEEDED(result) && !collapsed) {
        EditAggregateTxn *delSelTxn;
        result = CreateTxnForDeleteSelection(nsIEditor::eDoNothing,
                                             &delSelTxn);
        if (NS_SUCCEEDED(result) && delSelTxn) {
          (*aAggTxn)->AppendChild(delSelTxn);
        }
      }
    }
  }
  return result;
}


NS_IMETHODIMP 
nsEditor::InsertText(const nsString& aStringToInsert)
{
  EditAggregateTxn *aggTxn = nsnull;
  // Create the "delete current selection" txn
  nsresult result = CreateAggregateTxnForDeleteSelection(InsertTextTxn::gInsertTextTxnName, &aggTxn);
  if ((NS_FAILED(result)) || (nsnull==aggTxn)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  InsertTextTxn *txn;
  result = CreateTxnForInsertText(aStringToInsert, nsnull, &txn); // insert at the current selection
  if ((NS_SUCCEEDED(result)) && txn)  {
    aggTxn->AppendChild(txn);
    result = Do(aggTxn);  
  }
  else if (NS_ERROR_EDITOR_NO_SELECTION==result)  {
    result = DoInitialInsert(aStringToInsert);
  }
  else if (NS_ERROR_EDITOR_NO_TEXTNODE==result) 
  {
    BeginTransaction();
    nsCOMPtr<nsIDOMSelection> selection;
    result = GetSelection(getter_AddRefs(selection));
    if ((NS_SUCCEEDED(result)) && selection)
    {
      nsCOMPtr<nsIDOMNode> selectedNode;
      PRInt32 offset;
      result = selection->GetAnchorNode(getter_AddRefs(selectedNode));
      if (NS_SUCCEEDED(result) && NS_SUCCEEDED(selection->GetAnchorOffset(&offset)) && selectedNode)
      {
        nsCOMPtr<nsIDOMNode> newNode;
        result = CreateNode(GetTextNodeTag(), selectedNode, offset+1, 
                            getter_AddRefs(newNode));
        if (NS_SUCCEEDED(result) && newNode)
        {
          nsCOMPtr<nsIDOMCharacterData>newTextNode;
          newTextNode = do_QueryInterface(newNode);
          if (newTextNode)
          {
            nsAutoString placeholderText(" ");
            newTextNode->SetData(placeholderText);
            selection->Collapse(newNode, 0);
            selection->Extend(newNode, 1);
            result = InsertText(aStringToInsert);
          }
        }
      }
    }            
    EndTransaction();
  }
  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForInsertText(const nsString & aStringToInsert,
                                               nsIDOMCharacterData *aTextNode,
                                               InsertTextTxn ** aTxn)
{
  nsresult result;
  PRInt32 offset;
  nsCOMPtr<nsIDOMCharacterData> nodeAsText;

  if (aTextNode) {
    nodeAsText = do_QueryInterface(aTextNode);
    offset = 0;
    result = NS_OK;
  }
  else
  {
    nsCOMPtr<nsIDOMSelection> selection;
    result = mPresShell->GetSelection(getter_AddRefs(selection));
    if ((NS_SUCCEEDED(result)) && selection)
    {
      result = NS_ERROR_UNEXPECTED; 
      nsCOMPtr<nsIEnumerator> enumerator;
      enumerator = do_QueryInterface(selection);
      if (enumerator)
      {
        enumerator->First(); 
        nsISupports *currentItem;
        result = enumerator->CurrentItem(&currentItem);
        if ((NS_SUCCEEDED(result)) && (nsnull!=currentItem))
        {
          result = NS_ERROR_UNEXPECTED; 
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          if (range)
          {
            nsCOMPtr<nsIDOMNode> node;
            result = range->GetStartParent(getter_AddRefs(node));
            if ((NS_SUCCEEDED(result)) && (node))
            {
              nodeAsText = do_QueryInterface(node);
              range->GetStartOffset(&offset);
              if (!nodeAsText) {
                result = NS_ERROR_EDITOR_NO_TEXTNODE;
              }
            }
          }
        }
        else
        { 
          result = NS_ERROR_EDITOR_NO_SELECTION;
        }
      }
    }
  }
  if (NS_SUCCEEDED(result) && nodeAsText)
  {
    result = TransactionFactory::GetNewTransaction(kInsertTextTxnIID, (EditTxn **)aTxn);
    if (nsnull!=*aTxn) {
      result = (*aTxn)->Init(nodeAsText, offset, aStringToInsert, mPresShell);
    }
    else {
      result = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return result;
}

// we're in the special situation where there is no selection.  Insert the text
// at the beginning of the document.
// XXX: this is all logic that must be moved to the rule system
//      for HTML, we create a text node on the body.  That's what is done below
//      for XML, we would create a text node on the root element.
//      The rule system should be telling us which of these (or any other variant) to do.
/* this method should look something like
   BeginTransaction()
   mRule->GetNodeForInitialInsert(parentNode)
   mRule->CreateInitialDocumentFragment(childNode)
   InsertElement(childNode, parentNode)
   find the first text node in childNode
   insert the text there
*/

NS_IMETHODIMP nsEditor::DoInitialInsert(const nsString & aStringToInsert)
{
  if (!mDoc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  
  nsCOMPtr<nsIDOMNodeList>nodeList;
  nsAutoString bodyTag = "body";
  nsresult result = mDoc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
  if ((NS_SUCCEEDED(result)) && nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
    nsCOMPtr<nsIDOMNode>node;
    result = nodeList->Item(0, getter_AddRefs(node));
    if ((NS_SUCCEEDED(result)) && node)
    { // now we've got the body tag.
      // create transaction to insert the text node, 
      // and create a transaction to insert the text
      CreateElementTxn *txn;
      result = CreateTxnForCreateElement(GetTextNodeTag(), node, 0, &txn);
      if ((NS_SUCCEEDED(result)) && txn)
      {
        result = Do(txn);
        if (NS_SUCCEEDED(result))
        {
          nsCOMPtr<nsIDOMNode>newNode;
          txn->GetNewNode(getter_AddRefs(newNode));
          if ((NS_SUCCEEDED(result)) && newNode)
          {
            nsCOMPtr<nsIDOMCharacterData>newTextNode;
            newTextNode = do_QueryInterface(newNode);
            if (newTextNode)
            {
              InsertTextTxn *insertTxn;
              result = CreateTxnForInsertText(aStringToInsert, newTextNode, &insertTxn);
              if (NS_SUCCEEDED(result)) {
                result = Do(insertTxn);
              }
            }
            else {
              result = NS_ERROR_UNEXPECTED;
            }
          }
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP nsEditor::DeleteText(nsIDOMCharacterData *aElement,
                              PRUint32             aOffset,
                              PRUint32             aLength)
{
  DeleteTextTxn *txn;
  nsresult result = CreateTxnForDeleteText(aElement, aOffset, aLength, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
    // HACKForceRedraw();
  }
  return result;
}


NS_IMETHODIMP nsEditor::CreateTxnForDeleteText(nsIDOMCharacterData *aElement,
                                               PRUint32             aOffset,
                                               PRUint32             aLength,
                                               DeleteTextTxn      **aTxn)
{
  nsresult result=NS_ERROR_NULL_POINTER;
  if (nsnull != aElement)
  {
    result = TransactionFactory::GetNewTransaction(kDeleteTextTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(this, aElement, aOffset, aLength);
    }
  }
  return result;
}


NS_IMETHODIMP nsEditor::DeleteSelectionAndCreateNode(const nsString& aTag,
                                                     nsIDOMNode ** aNewNode)
{
  nsCOMPtr<nsIDOMNode> parentSelectedNode;
  PRInt32 offsetOfNewNode;
  nsresult result = DeleteSelectionAndPrepareToCreateNode(parentSelectedNode,
                                                          offsetOfNewNode);
  if (!NS_SUCCEEDED(result))
    return result;

  nsCOMPtr<nsIDOMNode> newNode;
  result = CreateNode(aTag, parentSelectedNode, offsetOfNewNode,
                      getter_AddRefs(newNode));
  
  *aNewNode = newNode;

  // we want the selection to be just after the new node
  nsCOMPtr<nsIDOMSelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
    selection->Collapse(parentSelectedNode, offsetOfNewNode+1);

  return result;
}

NS_IMETHODIMP nsEditor::DeleteSelectionAndPrepareToCreateNode(nsCOMPtr<nsIDOMNode> &parentSelectedNode, PRInt32& offsetOfNewNode)
{
  nsresult result=NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMSelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    PRBool collapsed;
    result = selection->GetIsCollapsed(&collapsed);
    if (NS_SUCCEEDED(result) && !collapsed) 
    {
      result = DeleteSelection(nsIEditor::eDoNothing);
      if (NS_FAILED(result)) {
        return result;
      }
      // get the new selection
      result = GetSelection(getter_AddRefs(selection));
      if (NS_FAILED(result)) {
        return result;
      }
#ifdef NS_DEBUG
      nsCOMPtr<nsIDOMNode>testSelectedNode;
      nsresult debugResult = selection->GetAnchorNode(getter_AddRefs(testSelectedNode));
      // no selection is ok.
      // if there is a selection, it must be collapsed
      if (testSelectedNode)
      {
        PRBool testCollapsed;
        debugResult = selection->GetIsCollapsed(&testCollapsed);
        NS_ASSERTION((NS_SUCCEEDED(result)), "couldn't get a selection after deletion");
        NS_ASSERTION(PR_TRUE==testCollapsed, "selection not reset after deletion");
      }
#endif
    }
    // split the selected node
    PRInt32 offsetOfSelectedNode;
    result = selection->GetAnchorNode(getter_AddRefs(parentSelectedNode));
    if (NS_SUCCEEDED(result) && NS_SUCCEEDED(selection->GetAnchorOffset(&offsetOfSelectedNode)) && parentSelectedNode)
    {
      nsCOMPtr<nsIDOMNode> selectedNode;
      PRUint32 selectedNodeContentCount=0;
      nsCOMPtr<nsIDOMCharacterData>selectedParentNodeAsText;
      selectedParentNodeAsText = do_QueryInterface(parentSelectedNode);
      /* if the selection is a text node, split the text node if necesary
         and compute where to put the new node
      */
      if (selectedParentNodeAsText) 
      { 
        PRInt32 indexOfTextNodeInParent;
        selectedNode = do_QueryInterface(parentSelectedNode);
        selectedNode->GetParentNode(getter_AddRefs(parentSelectedNode));
        selectedParentNodeAsText->GetLength(&selectedNodeContentCount);
        GetChildOffset(selectedNode, parentSelectedNode, indexOfTextNodeInParent);

        if ((offsetOfSelectedNode!=0) && (((PRUint32)offsetOfSelectedNode)!=selectedNodeContentCount))
        {
          nsCOMPtr<nsIDOMNode> newSiblingNode;
          result = SplitNode(selectedNode, offsetOfSelectedNode, getter_AddRefs(newSiblingNode));
          // now get the node's offset in it's parent, and insert the new tag there
          if (NS_SUCCEEDED(result)) {
            result = GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
          }
        }
        else 
        { // determine where to insert the new node
          if (0==offsetOfSelectedNode) {
            offsetOfNewNode = indexOfTextNodeInParent; // insert new node as previous sibling to selection parent
          }
          else {                 // insert new node as last child
            GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
            offsetOfNewNode++;    // offsets are 0-based, and we need the index of the new node
          }
        }
      }
      /* if the selection is not a text node, split the parent node if necesary
         and compute where to put the new node
      */
      else
      { // it's an interior node
        nsCOMPtr<nsIDOMNodeList>parentChildList;
        parentSelectedNode->GetChildNodes(getter_AddRefs(parentChildList));
        if ((NS_SUCCEEDED(result)) && parentChildList)
        {
          result = parentChildList->Item(offsetOfSelectedNode, getter_AddRefs(selectedNode));
          if ((NS_SUCCEEDED(result)) && selectedNode)
          {
            nsCOMPtr<nsIDOMCharacterData>selectedNodeAsText;
            selectedNodeAsText = do_QueryInterface(selectedNode);
            nsCOMPtr<nsIDOMNodeList>childList;
            selectedNode->GetChildNodes(getter_AddRefs(childList));
            if ((NS_SUCCEEDED(result)) && childList) {
              childList->GetLength(&selectedNodeContentCount);
            }
            else {
              return NS_ERROR_NULL_POINTER;
            }
            if ((offsetOfSelectedNode!=0) && (((PRUint32)offsetOfSelectedNode)!=selectedNodeContentCount))
            {
              nsCOMPtr<nsIDOMNode> newSiblingNode;
              result = SplitNode(selectedNode, offsetOfSelectedNode, getter_AddRefs(newSiblingNode));
              // now get the node's offset in it's parent, and insert the new tag there
              if (NS_SUCCEEDED(result)) {
                result = GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
              }
            }
            else 
            { // determine where to insert the new node
              if (0==offsetOfSelectedNode) {
                offsetOfNewNode = 0; // insert new node as first child
              }
              else {                 // insert new node as last child
                GetChildOffset(selectedNode, parentSelectedNode, offsetOfNewNode);
                offsetOfNewNode++;    // offsets are 0-based, and we need the index of the new node
              }
            }
          }
        }
      }

      // Here's where the new node was inserted
    }
    else {
      printf("InsertBreak into an empty document is not yet supported\n");
    }
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::DeleteSelection(nsIEditor::ECollapsedSelectionAction aAction)
{
  nsresult result;

  EditAggregateTxn *txn;
  result = CreateTxnForDeleteSelection(aAction, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }

  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForDeleteSelection(nsIEditor::ECollapsedSelectionAction aAction,
                                                    EditAggregateTxn  ** aTxn)
{
  if (!aTxn)
    return NS_ERROR_NULL_POINTER;
  *aTxn = nsnull;

  nsresult result;
  // allocate the out-param transaction
  result = TransactionFactory::GetNewTransaction(kEditAggregateTxnIID, (EditTxn **)aTxn);
  if (NS_FAILED(result)) {
    return result;
  }
  nsCOMPtr<nsIDOMSelection> selection;
  result = mPresShell->GetSelection(getter_AddRefs(selection));
  if ((NS_SUCCEEDED(result)) && selection)
  {
    nsCOMPtr<nsIEnumerator> enumerator;
    enumerator = do_QueryInterface(selection);
    if (enumerator)
    {
      for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
      {
        nsISupports *currentItem=nsnull;
        result = enumerator->CurrentItem(&currentItem);
        if ((NS_SUCCEEDED(result)) && (currentItem))
        {
          nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
          PRBool isCollapsed;
          range->GetIsCollapsed(&isCollapsed);
          if (PR_FALSE==isCollapsed)
          {
            DeleteRangeTxn *txn;
            result = TransactionFactory::GetNewTransaction(kDeleteRangeTxnIID, (EditTxn **)&txn);
            if ((NS_SUCCEEDED(result)) && (nsnull!=txn))
            {
              txn->Init(this, range);
              (*aTxn)->AppendChild(txn);
            }
            else
              result = NS_ERROR_OUT_OF_MEMORY;
          }
          else
          { // we have an insertion point.  delete the thing in front of it or behind it, depending on aAction
            result = CreateTxnForDeleteInsertionPoint(range, aAction, *aTxn);
          }
        }
      }
    }
  }

  // if we didn't build the transaction correctly, destroy the out-param transaction so we don't leak it.
  if (NS_FAILED(result))
  {
    NS_IF_RELEASE(*aTxn);
  }

  return result;
}


//XXX: currently, this doesn't handle edge conditions because GetNext/GetPrior are not implemented
NS_IMETHODIMP
nsEditor::CreateTxnForDeleteInsertionPoint(nsIDOMRange         *aRange, 
                                           nsIEditor::ECollapsedSelectionAction
                                             aAction,
                                           EditAggregateTxn    *aTxn)
{
  nsCOMPtr<nsIDOMNode> node;
  PRBool isFirst;
  PRBool isLast;
  PRInt32 offset;
  //PRInt32 length=1;

  // get the node and offset of the insertion point
  nsresult result = aRange->GetStartParent(getter_AddRefs(node));
  if (NS_FAILED(result))
    return result;
  result = aRange->GetStartOffset(&offset);
  if (NS_FAILED(result))
    return result;

  // determine if the insertion point is at the beginning, middle, or end of the node
  nsCOMPtr<nsIDOMCharacterData> nodeAsText;
  nsCOMPtr<nsIDOMNode> selectedNode;
  nodeAsText = do_QueryInterface(node);

  if (nodeAsText)
  {
    PRUint32 count;
    nodeAsText->GetLength(&count);
    isFirst = PRBool(0==offset);
    isLast  = PRBool(count==(PRUint32)offset);
  }
  else
  { 
    // get the child list and count
    nsCOMPtr<nsIDOMNodeList>childList;
    PRUint32 count=0;
    result = node->GetChildNodes(getter_AddRefs(childList));
    if ((NS_SUCCEEDED(result)) && childList)
    {
      childList->GetLength(&count);
      childList->Item(offset, getter_AddRefs(selectedNode));
    }
    isFirst = PRBool(0==offset);
    isLast  = PRBool((count-1)==(PRUint32)offset);
  }
// XXX: if isFirst && isLast, then we'll need to delete the node 
  //    as well as the 1 child

  // build a transaction for deleting the appropriate data
  // XXX: this has to come from rule section
  if ((nsIEditor::eDeleteLeft==aAction) && (PR_TRUE==isFirst))
  { // we're backspacing from the beginning of the node.  Delete the first thing to our left
    nsCOMPtr<nsIDOMNode> priorNode;
    result = GetPriorNode(node, PR_TRUE, getter_AddRefs(priorNode));
    if ((NS_SUCCEEDED(result)) && priorNode)
    { // there is a priorNode, so delete it's last child (if text content, delete the last char.)
      // if it has no children, delete it
      nsCOMPtr<nsIDOMCharacterData> priorNodeAsText;
      priorNodeAsText = do_QueryInterface(priorNode);
      if (priorNodeAsText)
      {
        PRUint32 length=0;
        priorNodeAsText->GetLength(&length);
        if (0<length)
        {
          DeleteTextTxn *txn;
          result = CreateTxnForDeleteText(priorNodeAsText, length-1, 1, &txn);
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
          }
        }
        else
        { // XXX: can you have an empty text node?  If so, what do you do?
          printf("ERROR: found a text node with 0 characters\n");
          result = NS_ERROR_UNEXPECTED;
        }
      }
      else
      { // priorNode is not text, so tell it's parent to delete it
        DeleteElementTxn *txn;
        result = CreateTxnForDeleteElement(priorNode, &txn);
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
        }
      }
    }
  }
  else if ((nsIEditor::eDeleteRight==aAction) && (PR_TRUE==isLast))
  { // we're deleting from the end of the node.  Delete the first thing to our right
    nsCOMPtr<nsIDOMNode> nextNode;
    result = GetNextNode(node, PR_TRUE, getter_AddRefs(nextNode));
    if ((NS_SUCCEEDED(result)) && nextNode)
    { // there is a priorNode, so delete it's last child (if text content, delete the last char.)
      // if it has no children, delete it
      nsCOMPtr<nsIDOMCharacterData> nextNodeAsText;
      nextNodeAsText = do_QueryInterface(nextNode);
      if (nextNodeAsText)
      {
        PRUint32 length=0;
        nextNodeAsText->GetLength(&length);
        if (0<length)
        {
          DeleteTextTxn *txn;
          result = CreateTxnForDeleteText(nextNodeAsText, 0, 1, &txn);
          if (NS_SUCCEEDED(result)) {
            aTxn->AppendChild(txn);
          }
        }
        else
        { // XXX: can you have an empty text node?  If so, what do you do?
          printf("ERROR: found a text node with 0 characters\n");
          result = NS_ERROR_UNEXPECTED;
        }
      }
      else
      { // nextNode is not text, so tell it's parent to delete it
        DeleteElementTxn *txn;
        result = CreateTxnForDeleteElement(nextNode, &txn);
        if (NS_SUCCEEDED(result)) {
          aTxn->AppendChild(txn);
        }
      }
    }
  }
  else
  {
    if (nodeAsText)
    { // we have text, so delete a char at the proper offset
      if (nsIEditor::eDeleteLeft==aAction) {
        offset --;
      }
      DeleteTextTxn *txn;
      result = CreateTxnForDeleteText(nodeAsText, offset, 1, &txn);
      if (NS_SUCCEEDED(result)) {
        aTxn->AppendChild(txn);
      }
    }
    else
    { // we're deleting a node
      DeleteElementTxn *txn;
      result = CreateTxnForDeleteElement(selectedNode, &txn);
      if (NS_SUCCEEDED(result)) {
        aTxn->AppendChild(txn);
      }
    }
  }
  return result;
}


NS_IMETHODIMP 
nsEditor::SplitNode(nsIDOMNode * aNode,
                    PRInt32      aOffset,
                    nsIDOMNode **aNewLeftNode)
{
  PRInt32 i;
  nsIEditActionListener *listener;

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->WillSplitNode(aNode, aOffset);
    }
  }

  SplitElementTxn *txn;
  nsresult result = CreateTxnForSplitNode(aNode, aOffset, &txn);
  if (NS_SUCCEEDED(result))  
  {
    result = Do(txn);
    if (NS_SUCCEEDED(result))
    {
      result = txn->GetNewNode(aNewLeftNode);
      NS_ASSERTION((NS_SUCCEEDED(result)), "result must succeeded for GetNewNode");
    }
  }

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
      {
        nsIDOMNode *ptr = (aNewLeftNode) ? *aNewLeftNode : 0;
        listener->DidSplitNode(aNode, aOffset, ptr, result);
      }
    }
  }

  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForSplitNode(nsIDOMNode *aNode,
                                         PRUint32    aOffset,
                                         SplitElementTxn **aTxn)
{
  nsresult result=NS_ERROR_NULL_POINTER;
  if (nsnull != aNode)
  {
    result = TransactionFactory::GetNewTransaction(kSplitElementTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(this, aNode, aOffset);
    }
  }
  return result;
}

NS_IMETHODIMP
nsEditor::JoinNodes(nsIDOMNode * aLeftNode,
                    nsIDOMNode * aRightNode,
                    nsIDOMNode * aParent)
{
  PRInt32 i;
  nsIEditActionListener *listener;

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->WillJoinNodes(aLeftNode, aRightNode, aParent);
    }
  }

  JoinElementTxn *txn;
  nsresult result = CreateTxnForJoinNode(aLeftNode, aRightNode, &txn);
  if (NS_SUCCEEDED(result))  {
    result = Do(txn);  
  }

  if (mActionListeners)
  {
    for (i = 0; i < mActionListeners->Count(); i++)
    {
      listener = (nsIEditActionListener *)mActionListeners->ElementAt(i);
      if (listener)
        listener->DidJoinNodes(aLeftNode, aRightNode, aParent, result);
    }
  }

  return result;
}

NS_IMETHODIMP nsEditor::CreateTxnForJoinNode(nsIDOMNode  *aLeftNode,
                                             nsIDOMNode  *aRightNode,
                                             JoinElementTxn **aTxn)
{
  nsresult result=NS_ERROR_NULL_POINTER;
  if ((nsnull != aLeftNode) && (nsnull != aRightNode))
  {
    result = TransactionFactory::GetNewTransaction(kJoinElementTxnIID, (EditTxn **)aTxn);
    if (NS_SUCCEEDED(result))  {
      result = (*aTxn)->Init(this, aLeftNode, aRightNode);
    }
  }
  return result;
}


// END nsEditor core implementation

// BEGIN nsEditor public static helper methods

nsresult
nsEditor::SplitNodeImpl(nsIDOMNode * aExistingRightNode,
                        PRInt32      aOffset,
                        nsIDOMNode*  aNewLeftNode,
                        nsIDOMNode*  aParent)
{

  if (gNoisy) { printf("SplitNodeImpl: left=%p, right=%p, offset=%d\n", aNewLeftNode, aExistingRightNode, aOffset); }
  
  nsresult result;
  NS_ASSERTION(((nsnull!=aExistingRightNode) &&
                (nsnull!=aNewLeftNode) &&
                (nsnull!=aParent)),
                "null arg");
  if ((nsnull!=aExistingRightNode) &&
      (nsnull!=aNewLeftNode) &&
      (nsnull!=aParent))
  {
    nsCOMPtr<nsIDOMNode> resultNode;
    result = aParent->InsertBefore(aNewLeftNode, aExistingRightNode, getter_AddRefs(resultNode));
    //printf("  after insert\n"); content->List();  // DEBUG
    if (NS_SUCCEEDED(result))
    {
      // split the children between the 2 nodes
      // at this point, aExistingRightNode has all the children
      // move all the children whose index is < aOffset to aNewLeftNode
      if (0<=aOffset) // don't bother unless we're going to move at least one child
      {
        // if it's a text node, just shuffle around some text
        nsCOMPtr<nsIDOMCharacterData> rightNodeAsText( do_QueryInterface(aExistingRightNode) );
        nsCOMPtr<nsIDOMCharacterData> leftNodeAsText( do_QueryInterface(aNewLeftNode) );
        if (leftNodeAsText && rightNodeAsText)
        {
          // fix right node
          nsString leftText;
          rightNodeAsText->SubstringData(0, aOffset, leftText);
          rightNodeAsText->DeleteData(0, aOffset);
          // fix left node
          leftNodeAsText->SetData(leftText);
          // moose          
        }
        else
        {  // otherwise it's an interior node, so shuffle around the children
           // go through list backwards so deletes don't interfere with the iteration
          nsCOMPtr<nsIDOMNodeList> childNodes;
          result = aExistingRightNode->GetChildNodes(getter_AddRefs(childNodes));
          if ((NS_SUCCEEDED(result)) && (childNodes))
          {
            PRInt32 i=aOffset-1;
            for ( ; ((NS_SUCCEEDED(result)) && (0<=i)); i--)
            {
              nsCOMPtr<nsIDOMNode> childNode;
              result = childNodes->Item(i, getter_AddRefs(childNode));
              if ((NS_SUCCEEDED(result)) && (childNode))
              {
                result = aExistingRightNode->RemoveChild(childNode, getter_AddRefs(resultNode));
                //printf("  after remove\n"); content->List();  // DEBUG
                if (NS_SUCCEEDED(result))
                {
                  nsCOMPtr<nsIDOMNode> firstChild;
                  aNewLeftNode->GetFirstChild(getter_AddRefs(firstChild));
                  result = aNewLeftNode->InsertBefore(childNode, firstChild, getter_AddRefs(resultNode));
                  //printf("  after append\n"); content->List();  // DEBUG
                }
              }
            }
          }        
        }
      }
    }
  }
  else
    result = NS_ERROR_INVALID_ARG;

  return result;
}

nsresult
nsEditor::JoinNodesImpl(nsIDOMNode * aNodeToKeep,
                        nsIDOMNode * aNodeToJoin,
                        nsIDOMNode * aParent,
                        PRBool       aNodeToKeepIsFirst)
{
  nsresult result = NS_OK;
  NS_ASSERTION(((nsnull!=aNodeToKeep) &&
                (nsnull!=aNodeToJoin) &&
                (nsnull!=aParent)),
                "null arg");
  if ((nsnull!=aNodeToKeep) &&
      (nsnull!=aNodeToJoin) &&
      (nsnull!=aParent))
  {
    // if it's a text node, just shuffle around some text
    nsCOMPtr<nsIDOMCharacterData> keepNodeAsText( do_QueryInterface(aNodeToKeep) );
    nsCOMPtr<nsIDOMCharacterData> joinNodeAsText( do_QueryInterface(aNodeToJoin) );
    if (keepNodeAsText && joinNodeAsText)
    {
      nsString rightText;
      nsString leftText;
      if (aNodeToKeepIsFirst)
      {
        keepNodeAsText->GetData(leftText);
        joinNodeAsText->GetData(rightText);
      }
      else
      {
        keepNodeAsText->GetData(rightText);
        joinNodeAsText->GetData(leftText);
      }
      leftText += rightText;
      keepNodeAsText->SetData(leftText);          
    }
    else
    {  // otherwise it's an interior node, so shuffle around the children
      nsCOMPtr<nsIDOMNodeList> childNodes;
      result = aNodeToJoin->GetChildNodes(getter_AddRefs(childNodes));
      if ((NS_SUCCEEDED(result)) && (childNodes))
      {
        PRInt32 i;  // must be signed int!
        PRUint32 childCount=0;
        childNodes->GetLength(&childCount);
        nsCOMPtr<nsIDOMNode> firstNode; //only used if aNodeToKeepIsFirst is false
        if (PR_FALSE==aNodeToKeepIsFirst)
        { // remember the first child in aNodeToKeep, we'll insert all the children of aNodeToJoin in front of it
          result = aNodeToKeep->GetFirstChild(getter_AddRefs(firstNode));  
          // GetFirstChild returns nsnull firstNode if aNodeToKeep has no children, that's ok.
        }
        nsCOMPtr<nsIDOMNode> resultNode;
        // have to go through the list backwards to keep deletes from interfering with iteration
        nsCOMPtr<nsIDOMNode> previousChild;
        for (i=childCount-1; ((NS_SUCCEEDED(result)) && (0<=i)); i--)
        {
          nsCOMPtr<nsIDOMNode> childNode;
          result = childNodes->Item(i, getter_AddRefs(childNode));
          if ((NS_SUCCEEDED(result)) && (childNode))
          {
            if (PR_TRUE==aNodeToKeepIsFirst)
            { // append children of aNodeToJoin
              //was result = aNodeToKeep->AppendChild(childNode, getter_AddRefs(resultNode));
              result = aNodeToKeep->InsertBefore(childNode, previousChild, getter_AddRefs(resultNode));
              previousChild = do_QueryInterface(childNode);
            }
            else
            { // prepend children of aNodeToJoin
              //was result = aNodeToKeep->InsertBefore(childNode, firstNode, getter_AddRefs(resultNode));
              result = aNodeToKeep->InsertBefore(childNode, firstNode, getter_AddRefs(resultNode));
              firstNode = do_QueryInterface(childNode);
            }
          }
        }
      }
      else if (!childNodes) {
        result = NS_ERROR_NULL_POINTER;
      }
    }
    if (NS_SUCCEEDED(result))
    { // delete the extra node
      nsCOMPtr<nsIDOMNode> resultNode;
      result = aParent->RemoveChild(aNodeToJoin, getter_AddRefs(resultNode));
    }
  }
  else
    result = NS_ERROR_INVALID_ARG;

  return result;
}

nsresult 
nsEditor::GetChildOffset(nsIDOMNode *aChild, nsIDOMNode *aParent, PRInt32 &aOffset)
{
  NS_ASSERTION((aChild && aParent), "bad args");
  nsresult result = NS_ERROR_NULL_POINTER;
  if (aChild && aParent)
  {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    result = aParent->GetChildNodes(getter_AddRefs(childNodes));
    if ((NS_SUCCEEDED(result)) && (childNodes))
    {
      PRInt32 i=0;
      for ( ; NS_SUCCEEDED(result); i++)
      {
        nsCOMPtr<nsIDOMNode> childNode;
        result = childNodes->Item(i, getter_AddRefs(childNode));
        if ((NS_SUCCEEDED(result)) && (childNode))
        {
          if (childNode.get()==aChild)
          {
            aOffset = i;
            break;
          }
        }
        else if (!childNode)
          result = NS_ERROR_NULL_POINTER;
      }
    }
    else if (!childNodes)
      result = NS_ERROR_NULL_POINTER;
  }
  return result;
}

// returns the number of things inside aNode.  
// If aNode is text, returns number of characters. If not, returns number of children nodes.
nsresult
nsEditor::GetLengthOfDOMNode(nsIDOMNode *aNode, PRUint32 &aCount) 
{
  aCount = 0;
  if (!aNode) { return NS_ERROR_NULL_POINTER; }
  nsresult result=NS_OK;
  nsCOMPtr<nsIDOMCharacterData>nodeAsChar;
  nodeAsChar = do_QueryInterface(aNode);
  if (nodeAsChar) {
    nodeAsChar->GetLength(&aCount);
  }
  else
  {
    PRBool hasChildNodes;
    aNode->HasChildNodes(&hasChildNodes);
    if (PR_TRUE==hasChildNodes)
    {
      nsCOMPtr<nsIDOMNodeList>nodeList;
      result = aNode->GetChildNodes(getter_AddRefs(nodeList));
      if (NS_SUCCEEDED(result) && nodeList) {
        nodeList->GetLength(&aCount);
      }
    }
  }
  return result;
}

// content-based inline vs. block query
nsresult
nsEditor::IsNodeInline(nsIDOMNode *aNode, PRBool &aIsInline)
{
  // this is a content-based implementation
  if (!aNode) { return NS_ERROR_NULL_POINTER; }

  nsresult result;
  aIsInline = PR_FALSE;
  nsCOMPtr<nsIDOMElement>element;
  element = do_QueryInterface(aNode);
  if (element)
  {
    nsAutoString tag;
    result = element->GetTagName(tag);
    if (NS_SUCCEEDED(result))
    {
      nsIAtom *tagAtom = NS_NewAtom(tag);
      if (!tagAtom) { return NS_ERROR_NULL_POINTER; }
      if (tagAtom==nsIEditProperty::a      ||
          tagAtom==nsIEditProperty::b      ||
          tagAtom==nsIEditProperty::big    ||
          tagAtom==nsIEditProperty::font   ||
          tagAtom==nsIEditProperty::i      ||
          tagAtom==nsIEditProperty::span   ||
          tagAtom==nsIEditProperty::small  ||
          tagAtom==nsIEditProperty::strike ||
          tagAtom==nsIEditProperty::sub    ||
          tagAtom==nsIEditProperty::sup    ||
          tagAtom==nsIEditProperty::tt     ||
          tagAtom==nsIEditProperty::u        )
      {
        aIsInline = PR_TRUE;
      }
    }
  }
  return NS_OK;
}

nsresult
nsEditor::GetBlockParent(nsIDOMNode *aNode, nsIDOMElement **aBlockParent) 
{
  nsresult result = NS_OK;
  if (!aBlockParent) {return NS_ERROR_NULL_POINTER;}
  *aBlockParent = nsnull;
  nsCOMPtr<nsIDOMNode>parent;
  nsCOMPtr<nsIDOMNode>temp;
  result = aNode->GetParentNode(getter_AddRefs(parent));
  while (NS_SUCCEEDED(result) && parent)
  {
    PRBool isInline;
    result = IsNodeInline(parent, isInline);
    if (PR_FALSE==isInline)
    {
      parent->QueryInterface(nsIDOMElement::GetIID(), (void**)aBlockParent);
      break;
    }
    result = parent->GetParentNode(getter_AddRefs(temp));
    parent = do_QueryInterface(temp);
  }
  if (gNoisy) { printf("GetBlockParent for %p returning parent %p\n", aNode, *aBlockParent); }
  return result;
}

nsresult
nsEditor::GetBlockSection(nsIDOMNode *aChild,
                          nsIDOMNode **aLeftNode, 
                          nsIDOMNode **aRightNode) 
{
  nsresult result = NS_OK;
  if (!aChild || !aLeftNode || !aRightNode) {return NS_ERROR_NULL_POINTER;}
  *aLeftNode = aChild;
  *aRightNode = aChild;

  nsCOMPtr<nsIDOMNode>sibling;
  result = aChild->GetPreviousSibling(getter_AddRefs(sibling));
  while ((NS_SUCCEEDED(result)) && sibling)
  {
    PRBool isInline;
    IsNodeInline(sibling, isInline);
    if (PR_FALSE==isInline) 
    {
      nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(sibling);
      if (!nodeAsText) {
        break;
      }
      // XXX: needs some logic to work for other leaf nodes besides text!
    }
    *aLeftNode = sibling;
    result = (*aLeftNode)->GetPreviousSibling(getter_AddRefs(sibling)); 
  }
  NS_ADDREF((*aLeftNode));
  // now do the right side
  result = aChild->GetNextSibling(getter_AddRefs(sibling));
  while ((NS_SUCCEEDED(result)) && sibling)
  {
    PRBool isInline;
    IsNodeInline(sibling, isInline);
    if (PR_FALSE==isInline) 
    {
      nsCOMPtr<nsIDOMCharacterData>nodeAsText = do_QueryInterface(sibling);
      if (!nodeAsText) {
        break;
      }
    }
    *aRightNode = sibling;
    result = (*aRightNode)->GetNextSibling(getter_AddRefs(sibling)); 
  }
  NS_ADDREF((*aRightNode));
  if (gNoisy) { printf("GetBlockSection returning %p %p\n", 
                      (*aLeftNode), (*aRightNode)); }

  return result;
}

nsresult
nsEditor::GetBlockSectionsForRange(nsIDOMRange *aRange, nsISupportsArray *aSections) 
{
  if (!aRange || !aSections) {return NS_ERROR_NULL_POINTER;}

  nsresult result;
  nsCOMPtr<nsIContentIterator>iter;
  result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                              nsIContentIterator::GetIID(), getter_AddRefs(iter));
  if ((NS_SUCCEEDED(result)) && iter)
  {
    nsCOMPtr<nsIDOMRange> lastRange;
    iter->Init(aRange);
    nsCOMPtr<nsIContent> currentContent;
    iter->CurrentNode(getter_AddRefs(currentContent));
    while (NS_COMFALSE == iter->IsDone())
    {
      nsCOMPtr<nsIDOMNode>currentNode = do_QueryInterface(currentContent);
      if (currentNode)
      {
        nsCOMPtr<nsIAtom> currentContentTag;
        currentContent->GetTag(*getter_AddRefs(currentContentTag));
        // <BR> divides block content ranges.  We can achieve this by nulling out lastRange
        if (nsIEditProperty::br==currentContentTag.get())
        {
          lastRange = do_QueryInterface(nsnull);
        }
        else
        {
          PRBool isInlineOrText;
          result = IsNodeInline(currentNode, isInlineOrText);
          if (PR_FALSE==isInlineOrText)
          {
            PRUint16 nodeType;
            currentNode->GetNodeType(&nodeType);
            if (nsIDOMNode::TEXT_NODE == nodeType) {
              isInlineOrText = PR_TRUE;
            }
          }
          if (PR_TRUE==isInlineOrText)
          {
            nsCOMPtr<nsIDOMNode>leftNode;
            nsCOMPtr<nsIDOMNode>rightNode;
            result = GetBlockSection(currentNode,
                                     getter_AddRefs(leftNode),
                                     getter_AddRefs(rightNode));
            if (gNoisy) {printf("currentNode %p has block content (%p,%p)\n", currentNode.get(), leftNode.get(), rightNode.get());}
            if ((NS_SUCCEEDED(result)) && leftNode && rightNode)
            {
              // add range to the list if it doesn't overlap with the previous range
              PRBool addRange=PR_TRUE;
              if (lastRange)
              {
                nsCOMPtr<nsIDOMNode> lastStartNode;
                nsCOMPtr<nsIDOMElement> blockParentOfLastStartNode;
                lastRange->GetStartParent(getter_AddRefs(lastStartNode));
                result = GetBlockParent(lastStartNode, getter_AddRefs(blockParentOfLastStartNode));
                if ((NS_SUCCEEDED(result)) && blockParentOfLastStartNode)
                {
                  if (gNoisy) {printf("lastStartNode %p has block parent %p\n", lastStartNode.get(), blockParentOfLastStartNode.get());}
                  nsCOMPtr<nsIDOMElement> blockParentOfLeftNode;
                  result = GetBlockParent(leftNode, getter_AddRefs(blockParentOfLeftNode));
                  if ((NS_SUCCEEDED(result)) && blockParentOfLeftNode)
                  {
                    if (gNoisy) {printf("leftNode %p has block parent %p\n", leftNode.get(), blockParentOfLeftNode.get());}
                    if (blockParentOfLastStartNode==blockParentOfLeftNode) {
                      addRange = PR_FALSE;
                    }
                  }
                }
              }
              if (PR_TRUE==addRange) 
              {
                if (gNoisy) {printf("adding range, setting lastRange with start node %p\n", leftNode.get());}
                nsCOMPtr<nsIDOMRange> range;
                result = nsComponentManager::CreateInstance(kCRangeCID, nsnull, 
                                                            nsIDOMRange::GetIID(), getter_AddRefs(range));
                if ((NS_SUCCEEDED(result)) && range)
                { // initialize the range
                  range->SetStart(leftNode, 0);
                  range->SetEnd(rightNode, 0);
                  aSections->AppendElement(range);
                  lastRange = do_QueryInterface(range);
                }
              }        
            }
          }
        }
      }
      /* do not check result here, and especially do not return the result code.
       * we rely on iter->IsDone to tell us when the iteration is complete
       */
      iter->Next();
      iter->CurrentNode(getter_AddRefs(currentContent));
    }
  }
  return result;
}

nsresult
nsEditor::IntermediateNodesAreInline(nsIDOMRange *aRange,
                                     nsIDOMNode  *aStartNode, 
                                     PRInt32      aStartOffset, 
                                     nsIDOMNode  *aEndNode,
                                     PRInt32      aEndOffset,
                                     PRBool      &aResult) 
{
  aResult = PR_TRUE; // init out param.  we assume the condition is true unless we find a node that violates it
  if (!aStartNode || !aEndNode || !aRange) { return NS_ERROR_NULL_POINTER; }

  nsCOMPtr<nsIContentIterator>iter;
  nsresult result;
  result = nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                              nsIContentIterator::GetIID(), getter_AddRefs(iter));
  //XXX: maybe CreateInstance is expensive, and I should keep around a static iter?  
  //     as long as this method can't be called recursively or re-entrantly!

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
      while (NS_COMFALSE == iter->IsDone())
      {
        if ((content.get() != startContent.get()) &&
            (content.get() != endContent.get()))
        {
          nsCOMPtr<nsIDOMNode>currentNode;
          currentNode = do_QueryInterface(content);
          PRBool isInline=PR_FALSE;
          IsNodeInline(currentNode, isInline);
          if (PR_FALSE==isInline)
          {
            nsCOMPtr<nsIDOMCharacterData>nodeAsText;
            nodeAsText = do_QueryInterface(currentNode);
            if (!nodeAsText)  // text nodes don't count in this check, so ignore them
            {
              aResult = PR_FALSE;
              break;
            }
          }
        }
        /* do not check result here, and especially do not return the result code.
         * we rely on iter->IsDone to tell us when the iteration is complete
         */
        iter->Next();
        iter->CurrentNode(getter_AddRefs(content));
      }
    }
  }

  return result;
}


nsresult 
nsEditor::GetPriorNode(nsIDOMNode  *aCurrentNode, 
                       PRBool       aEditableNode, 
                       nsIDOMNode **aResultNode)
{
  nsresult result;
  if (!aCurrentNode || !aResultNode) { return NS_ERROR_NULL_POINTER; }
  
  *aResultNode = nsnull;  // init out-param

  // if aCurrentNode has a left sibling, return that sibling's rightmost child (or itself if it has no children)
  result = aCurrentNode->GetPreviousSibling(aResultNode);
  if ((NS_SUCCEEDED(result)) && *aResultNode)
  {
    result = GetRightmostChild(*aResultNode, aResultNode);
    if (NS_FAILED(result)) { return result; }
    if (PR_FALSE==aEditableNode) {
      return result;
    }
    if (PR_TRUE==IsEditable(*aResultNode)) {
      return result;
    }
    else 
    { // restart the search from the non-editable node we just found
      nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
      return GetPriorNode(notEditableNode, aEditableNode, aResultNode);
    }
  }
  
  // otherwise, walk up the parent change until there is a child that comes before 
  // the ancestor of aCurrentNode.  Then return that node's rightmost child
  nsCOMPtr<nsIDOMNode> parent = do_QueryInterface(aCurrentNode);
  do {
    nsCOMPtr<nsIDOMNode> node(parent);
    result = node->GetParentNode(getter_AddRefs(parent));
    if ((NS_SUCCEEDED(result)) && parent)
    {
      result = parent->GetPreviousSibling(getter_AddRefs(node));
      if ((NS_SUCCEEDED(result)) && node)
      {
        result = GetRightmostChild(node, aResultNode);
        if (NS_FAILED(result)) { return result; }
        if (PR_FALSE==aEditableNode) {
          return result;
        }
        if (PR_TRUE==IsEditable(*aResultNode)) {
          return result;
        }
        else 
        { // restart the search from the non-editable node we just found
          nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
          return GetPriorNode(notEditableNode, aEditableNode, aResultNode);
        }
      }
    }
  } while ((NS_SUCCEEDED(result)) && parent);

  return result;
}

nsresult 
nsEditor::GetNextNode(nsIDOMNode  *aCurrentNode, 
                      PRBool       aEditableNode, 
                      nsIDOMNode **aResultNode)
{
  nsresult result;
  *aResultNode = nsnull;
  // if aCurrentNode has a right sibling, return that sibling's leftmost child (or itself if it has no children)
  result = aCurrentNode->GetNextSibling(aResultNode);
  if ((NS_SUCCEEDED(result)) && *aResultNode)
  {
    result = GetLeftmostChild(*aResultNode, aResultNode);
    if (NS_FAILED(result)) { return result; }
    if (PR_FALSE==aEditableNode) {
      return result;
    }
    if (PR_TRUE==IsEditable(*aResultNode)) {
      return result;
    }
    else 
    { // restart the search from the non-editable node we just found
      nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
      return GetNextNode(notEditableNode, aEditableNode, aResultNode);
    }
  }
  
  // otherwise, walk up the parent change until there is a child that comes after 
  // the ancestor of aCurrentNode.  Then return that node's leftmost child

  nsCOMPtr<nsIDOMNode> parent(do_QueryInterface(aCurrentNode));
  do {
    nsCOMPtr<nsIDOMNode> node(parent);
    result = node->GetParentNode(getter_AddRefs(parent));
    if ((NS_SUCCEEDED(result)) && parent)
    {
      result = parent->GetNextSibling(getter_AddRefs(node));
      if ((NS_SUCCEEDED(result)) && node)
      {
        result = GetLeftmostChild(node, aResultNode);
        if (NS_FAILED(result)) { return result; }
        if (PR_FALSE==aEditableNode) {
          return result;
        }
        if (PR_TRUE==IsEditable(*aResultNode)) {
          return result;
        }
        else 
        { // restart the search from the non-editable node we just found
          nsCOMPtr<nsIDOMNode> notEditableNode = do_QueryInterface(*aResultNode);
          return GetNextNode(notEditableNode, aEditableNode, aResultNode);
        }
      }
    }
  } while ((NS_SUCCEEDED(result)) && parent);

  return result;
}

nsresult
nsEditor::GetRightmostChild(nsIDOMNode *aCurrentNode, nsIDOMNode **aResultNode)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode> resultNode(do_QueryInterface(aCurrentNode));
  PRBool hasChildren;
  resultNode->HasChildNodes(&hasChildren);
  while ((NS_SUCCEEDED(result)) && (PR_TRUE==hasChildren))
  {
    nsCOMPtr<nsIDOMNode> temp(resultNode);
    temp->GetLastChild(getter_AddRefs(resultNode));
    resultNode->HasChildNodes(&hasChildren);
  }

  if (NS_SUCCEEDED(result)) {
    *aResultNode = resultNode;
    NS_ADDREF(*aResultNode);
  }
  return result;
}

nsresult
nsEditor::GetLeftmostChild(nsIDOMNode *aCurrentNode, nsIDOMNode **aResultNode)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIDOMNode> resultNode(do_QueryInterface(aCurrentNode));
  PRBool hasChildren;
  resultNode->HasChildNodes(&hasChildren);
  while ((NS_SUCCEEDED(result)) && (PR_TRUE==hasChildren))
  {
    nsCOMPtr<nsIDOMNode> temp(resultNode);
    temp->GetFirstChild(getter_AddRefs(resultNode));
    resultNode->HasChildNodes(&hasChildren);
  }

  if (NS_SUCCEEDED(result)) {
    *aResultNode = resultNode;
    NS_ADDREF(*aResultNode);
  }
  return result;
}

PRBool 
nsEditor::NodeIsType(nsIDOMNode *aNode, nsIAtom *aTag)
{
  nsCOMPtr<nsIDOMElement>element;
  element = do_QueryInterface(aNode);
  if (element)
  {
    nsAutoString tag;
    element->GetTagName(tag);
    if (tag.Equals(aTag->GetUnicode()))
    {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool 
nsEditor::IsEditable(nsIDOMNode *aNode)
{
  if (!aNode) return PR_FALSE;
  nsCOMPtr<nsIDOMElement>element;
  element = do_QueryInterface(aNode);
  if (element)
  {
    nsAutoString att(kMOZEditorBogusNodeAttr);
    nsAutoString val;
    (void)element->GetAttribute(att, val);
    if (val.Equals(kMOZEditorBogusNodeValue)) {
      return PR_FALSE;
    }
    else {
      return PR_TRUE;
    }
  }
  else
  { 
    nsCOMPtr<nsIDOMCharacterData>text;
    text = do_QueryInterface(aNode);
    if (text)
    {
      nsAutoString data;
      text->GetData(data);
      PRUint32 length = data.Length();
      if (0==length) {
        return PR_FALSE;
      }
      // if the node contains only newlines, it's not editable
      PRUint32 i;
      for (i=0; i<length; i++)
      {
        if ('\n'!=data.CharAt(0)) {
          return PR_TRUE;
        }
      }
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

//END nsEditor static utility methods


NS_IMETHODIMP nsEditor::IncDocModCount(PRInt32 inNumMods)
{
  if (!mDoc) return NS_ERROR_NOT_INITIALIZED;
  
	nsCOMPtr<nsIDiskDocument>  diskDoc = do_QueryInterface(mDoc);
	if (diskDoc)
    diskDoc->IncrementModCount(inNumMods);

  return NS_OK;
}


NS_IMETHODIMP nsEditor::GetDocModCount(PRInt32 &outModCount)
{
  if (!mDoc) return NS_ERROR_NOT_INITIALIZED;
  
	nsCOMPtr<nsIDiskDocument>  diskDoc = do_QueryInterface(mDoc);
	if (diskDoc)
	  diskDoc->GetModCount(&outModCount);

  return NS_OK;
}


NS_IMETHODIMP nsEditor::ResetDocModCount()
{
  if (!mDoc) return NS_ERROR_NOT_INITIALIZED;
  
	nsCOMPtr<nsIDiskDocument>  diskDoc = do_QueryInterface(mDoc);
	if (diskDoc)
    diskDoc->ResetModCount();

  return NS_OK;
}


void nsEditor::HACKForceRedraw()
{
#ifdef HACK_FORCE_REDRAW
// XXXX: Horrible hack! We are doing this because
// of an error in Gecko which is not rendering the
// document after a change via the DOM - gpk 2/11/99
  // BEGIN HACK!!!
  nsCOMPtr<nsIPresShell> shell;
  
 	GetPresShell(getter_AddRefs(shell));
  if (shell) {
    nsCOMPtr<nsIViewManager> viewmgr;

    shell->GetViewManager(getter_AddRefs(viewmgr));
    if (viewmgr) {
      nsIView* view;
      viewmgr->GetRootView(view);			// views are not refCounted
      if (view) {
        viewmgr->UpdateView(view,nsnull,NS_VMREFRESH_IMMEDIATE);
      }
    }
  }
  // END HACK
#endif
}


NS_IMETHODIMP nsEditor::GetLayoutObject(nsIDOMNode *aNode, nsISupports **aLayoutObject)
{
  nsresult result = NS_ERROR_FAILURE;  // we return an error unless we get the index
  if( mPresShell != nsnull )
  {
    if ((nsnull!=aNode))
    { // get the content interface
      nsCOMPtr<nsIContent> nodeAsContent( do_QueryInterface(aNode) );
      if (nodeAsContent)
      { // get the frame from the content interface
        nsISupports *layoutObject=nsnull; // frames are not ref counted, so don't use an nsCOMPtr
        *aLayoutObject = nsnull;
        return (NS_SUCCEEDED(mPresShell->GetLayoutObjectFor(nodeAsContent, &layoutObject)));
      }
    }
    else {
      result = NS_ERROR_NULL_POINTER;
    }
  }
  return result;
}

NS_IMETHODIMP
nsEditor::BeginComposition(void)
{

  if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
#ifdef DEBUG_tague
	printf("nsEditor::StartComposition -- begin batch.\n");
#endif
    mTxnMgr->BeginBatch();
  }

  if (!mIMESelectionRange)
  {
    nsresult result = nsComponentManager::CreateInstance(kCDOMRangeCID, nsnull,
                                                         nsIDOMRange::GetIID(), 
                                                         getter_AddRefs(mIMESelectionRange));
	if (NS_FAILED(result))
	{
	  mTxnMgr->EndBatch();
	  return result;
	}
  }

  mIMEFirstTransaction=PR_TRUE;
 
  return NS_OK;

}

NS_IMETHODIMP
nsEditor::EndComposition(void)
{
 if ((nsITransactionManager *)nsnull!=mTxnMgr.get())
  {
#ifdef DEBUG_tague
	printf("nsEditor::EndComposition -- end batch.\n");
#endif
	mTxnMgr->EndBatch();
	mIMEFirstTransaction=PR_TRUE;
	return NS_OK;
  }

  // mIMESelectionRange = nsCOMPtr<nsIDOMRange>();

  return NS_OK;
}

NS_IMETHODIMP
nsEditor::SetCompositionString(const nsString& aCompositionString)
{


	if (mIMEFirstTransaction==PR_TRUE) {
		mIMEFirstTransaction = PR_FALSE;
	} else {
		// printf("Undo!\n");
		// mTxnMgr->Undo();
		nsCOMPtr<nsIDOMSelection> selection;
		nsresult result;

        result = mPresShell->GetSelection(getter_AddRefs(selection));

		if (NS_FAILED(result))
		{
			EndTransaction();
			return result;
		}

		nsCOMPtr<nsIDOMNode> node;
		PRInt32 offset;

		result = mIMESelectionRange->GetStartParent(getter_AddRefs(node));
		result = mIMESelectionRange->GetStartOffset(&offset);

		result = selection->Collapse(node, offset);


		result = mIMESelectionRange->GetEndParent(getter_AddRefs(node));
		result = mIMESelectionRange->GetEndOffset(&offset);

		result = selection->Extend(node, offset);
	}
#ifdef DEBUG_tague
	printf("nsEditor::SetCompositionString: string=%s\n",aCompositionString);
#endif
	return SetPreeditText(aCompositionString);
}

NS_IMETHODIMP
nsEditor::DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed)
{
  NS_NOTREACHED("This should never get called. Overridden by subclasses");
  return NS_OK;
}

NS_IMETHODIMP
nsEditor::DebugDumpContent() const
{
  nsCOMPtr<nsIContent>content;
  nsCOMPtr<nsIDOMNodeList>nodeList;
  nsAutoString bodyTag = "body";
  mDoc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
  if (nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
    nsCOMPtr<nsIDOMNode>bodyNode;
    nodeList->Item(0, getter_AddRefs(bodyNode));
    if (bodyNode) {
      content = do_QueryInterface(bodyNode);
    }
  }
  content->List();
  return NS_OK;
}

nsresult
nsEditor::GetFirstNodeOfType(nsIDOMNode     *aStartNode, 
                             const nsString &aTag, 
                             nsIDOMNode    **aResult)
{
  nsresult result=NS_OK;

  if (!aStartNode)
    return NS_ERROR_NULL_POINTER;
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMElement> element;
  *aResult = nsnull;
  nsCOMPtr<nsIDOMNode> childNode;
  result = aStartNode->GetFirstChild(getter_AddRefs(childNode));
  while (childNode)
  {
    result = childNode->QueryInterface(nsIDOMNode::GetIID(),getter_AddRefs(element));
    nsAutoString tag;
    if (NS_SUCCEEDED(result) && (element))
    {    
      element->GetTagName(tag);
      if (PR_TRUE==aTag.Equals(tag))
      {
        return (childNode->QueryInterface(nsIDOMNode::GetIID(),(void **) aResult)); // does the addref
      }
      else
      {
        result = GetFirstNodeOfType(childNode, aTag, aResult);
        if (nsnull!=*aResult)
          return result;
      }
    }
    nsCOMPtr<nsIDOMNode> temp = childNode;
    temp->GetNextSibling(getter_AddRefs(childNode));
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsEditor::GetFirstTextNode(nsIDOMNode *aNode, nsIDOMNode **aRetNode)
{
  if (!aNode || !aRetNode)
  {
    NS_NOTREACHED("GetFirstTextNode Failed");
    return NS_ERROR_NULL_POINTER;
  }

  PRUint16 mType;
  PRBool mCNodes;

  nsCOMPtr<nsIDOMNode> answer;
  
  aNode->GetNodeType(&mType);

  if (nsIDOMNode::ELEMENT_NODE == mType) {
    if (NS_SUCCEEDED(aNode->HasChildNodes(&mCNodes)) && PR_TRUE == mCNodes) 
    {
      nsCOMPtr<nsIDOMNode> node1;
      nsCOMPtr<nsIDOMNode> node2;

      if (!NS_SUCCEEDED(aNode->GetFirstChild(getter_AddRefs(node1))))
      {
        NS_NOTREACHED("GetFirstTextNode Failed");
      }
      while(!answer && node1) 
      {
        GetFirstTextNode(node1, getter_AddRefs(answer));
        node1->GetNextSibling(getter_AddRefs(node2));
        node1 = node2;
      }
    }
  }
  else if (nsIDOMNode::TEXT_NODE == mType) {
    answer = do_QueryInterface(aNode);
  }

    // OK, now return the answer, if any
  *aRetNode = answer;
  if (*aRetNode)
    NS_IF_ADDREF(*aRetNode);
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}


//END nsEditor Private methods

NS_IMETHODIMP nsEditor::DoInitialPreeeditInsert(const nsString& aStringToInsert)
{
  if (!mDoc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  
  nsCOMPtr<nsIDOMNodeList>nodeList;
  nsAutoString bodyTag = "body";
  nsresult result = mDoc->GetElementsByTagName(bodyTag, getter_AddRefs(nodeList));
  if ((NS_SUCCEEDED(result)) && nodeList)
  {
    PRUint32 count;
    nodeList->GetLength(&count);
    NS_ASSERTION(1==count, "there is not exactly 1 body in the document!");
    nsCOMPtr<nsIDOMNode>node;
    result = nodeList->Item(0, getter_AddRefs(node));
    if ((NS_SUCCEEDED(result)) && node)
    { // now we've got the body tag.
      // create transaction to insert the text node, 
      // and create a transaction to insert the text
      CreateElementTxn *txn;
      result = CreateTxnForCreateElement(GetTextNodeTag(), node, 0, &txn);
      if ((NS_SUCCEEDED(result)) && txn)
      {
        result = Do(txn);
        if (NS_SUCCEEDED(result))
        {
          nsCOMPtr<nsIDOMNode>newNode;
          txn->GetNewNode(getter_AddRefs(newNode));
          if ((NS_SUCCEEDED(result)) && newNode)
          {
            nsCOMPtr<nsIDOMCharacterData>newTextNode;
            newTextNode = do_QueryInterface(newNode);
            if (newTextNode)
            {
              InsertTextTxn *insertTxn;
              result = CreateTxnForInsertText(aStringToInsert, newTextNode, &insertTxn);
              if (NS_SUCCEEDED(result)) {
                result = Do(insertTxn);
              }
            }
            else {
              result = NS_ERROR_UNEXPECTED;
            }
          }
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP 
nsEditor::SetPreeditText(const nsString& aStringToInsert)
{
  nsresult result;

  //EditAggregateTxn *aggTxn = nsnull;
  // Create the "delete current selection" txn
  nsCOMPtr<nsIDOMSelection> selection;

  BeginTransaction();

  result = mPresShell->GetSelection(getter_AddRefs(selection));

  if (NS_SUCCEEDED(result) && selection)
  {
    PRBool collapsed;
    result = selection->GetIsCollapsed(&collapsed);
    if (NS_SUCCEEDED(result) && !collapsed) {
      EditAggregateTxn *delSelTxn;
      // XXX should this be eDoNothing instead of eDeleteRight?
      result = CreateTxnForDeleteSelection(nsIEditor::eDeleteRight,
                                           &delSelTxn);
      if (NS_SUCCEEDED(result) && delSelTxn) {
        result = Do(delSelTxn);

		if (NS_FAILED(result)) {
			EndTransaction();
			return result;
		}
      }
    }
  }

  result = mPresShell->GetSelection(getter_AddRefs(selection));

  if (NS_FAILED(result))
  {
	  EndTransaction();
	  return result;
  }

  nsCOMPtr<nsIDOMRange> startRange;
  nsCOMPtr<nsIDOMRange> endRange;

  result = selection->GetRangeAt(0, getter_AddRefs(startRange));

  if (NS_FAILED(result))
  {
	  EndTransaction();
	  return result;
  }

  InsertTextTxn *txn;
  result = CreateTxnForInsertText(aStringToInsert, nsnull, &txn); // insert at the current selection
  if ((NS_SUCCEEDED(result)) && txn)  {
    result = Do(txn);  
  }
  else if (NS_ERROR_EDITOR_NO_SELECTION==result)  {
    result = DoInitialInsert(aStringToInsert);
  }
  else if (NS_ERROR_EDITOR_NO_TEXTNODE==result) 
  {
    result = GetSelection(getter_AddRefs(selection));
    if ((NS_SUCCEEDED(result)) && selection)
    {
      nsCOMPtr<nsIDOMNode> selectedNode;
      PRInt32 offset;
      result = selection->GetAnchorNode(getter_AddRefs(selectedNode));
      if (NS_SUCCEEDED(result) && NS_SUCCEEDED(selection->GetAnchorOffset(&offset)) && selectedNode)
      {
        nsCOMPtr<nsIDOMNode> newNode;
        result = CreateNode(GetTextNodeTag(), selectedNode, offset+1, 
                            getter_AddRefs(newNode));
        if (NS_SUCCEEDED(result) && newNode)
        {
          nsCOMPtr<nsIDOMCharacterData>newTextNode;
          newTextNode = do_QueryInterface(newNode);
          if (newTextNode)
          {
            nsAutoString placeholderText(" ");
            newTextNode->SetData(placeholderText);
            selection->Collapse(newNode, 0);
            selection->Extend(newNode, 1);
            result = SetPreeditText(aStringToInsert);
          }
        }
      }
    }

  }

  result = mPresShell->GetSelection(getter_AddRefs(selection));

  if (NS_FAILED(result))
  {
	 EndTransaction();
	 return result;
  }

  result = selection->GetRangeAt(0, getter_AddRefs(endRange));

  nsCOMPtr<nsIDOMNode> node;
  PRInt32 offset;

  startRange->GetStartParent(getter_AddRefs(node));
  startRange->GetStartOffset(&offset);
  mIMESelectionRange->SetStart(node, offset);

  endRange->GetStartParent(getter_AddRefs(node));
  endRange->GetStartOffset(&offset);
  mIMESelectionRange->SetEnd(node, offset);

  EndTransaction();
  // HACKForceRedraw();
  return result;
}


///////////////////////////////////////////////////////////////////////////
// GetTag: digs out the atom for the tag of this node
//                    
nsCOMPtr<nsIAtom> 
nsEditor::GetTag(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIAtom> atom;
  
  if (!aNode) 
  {
    NS_NOTREACHED("null node passed to nsEditor::GetTag()");
    return atom;
  }
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  content->GetTag(*getter_AddRefs(atom));

  return atom;
}


///////////////////////////////////////////////////////////////////////////
// GetTagString: digs out string for the tag of this node
//                    
nsresult 
nsEditor::GetTagString(nsIDOMNode *aNode, nsString& outString)
{
  nsCOMPtr<nsIAtom> atom;
  
  if (!aNode) 
  {
    NS_NOTREACHED("null node passed to nsEditor::GetTag()");
    return NS_ERROR_NULL_POINTER;
  }
  
  atom = GetTag(aNode);
  if (atom)
  {
    atom->ToString(outString);
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


///////////////////////////////////////////////////////////////////////////
// NodesSameType: do these nodes have the same tag?
//                    
PRBool 
nsEditor::NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2)
{
  if (!aNode1 || !aNode2) 
  {
    NS_NOTREACHED("null node passed to nsEditor::NodesSameType()");
    return PR_FALSE;
  }
  
  nsCOMPtr<nsIAtom> atom1 = GetTag(aNode1);
  nsCOMPtr<nsIAtom> atom2 = GetTag(aNode2);
  
  if (atom1.get() == atom2.get())
    return PR_TRUE;

  return PR_FALSE;
}



///////////////////////////////////////////////////////////////////////////
// IsBlockNode: true if this node is an html block node
//                    
PRBool
nsEditor::IsBlockNode(nsIDOMNode *aNode)
{
  return !IsInlineNode(aNode);
}


///////////////////////////////////////////////////////////////////////////
// IsInlineNode: true if this node is an html inline node
//                    
PRBool
nsEditor::IsInlineNode(nsIDOMNode *aNode)
{
  PRBool retVal = PR_FALSE;
  IsNodeInline(aNode, retVal);
  return retVal;
}


///////////////////////////////////////////////////////////////////////////
// GetBlockNodeParent: returns enclosing block level ancestor, if any
//
nsCOMPtr<nsIDOMNode>
nsEditor::GetBlockNodeParent(nsIDOMNode *aNode)
{
  nsCOMPtr<nsIDOMNode> tmp;
  nsCOMPtr<nsIDOMNode> p;

  if (NS_FAILED(aNode->GetParentNode(getter_AddRefs(p))))  // no parent, ran off top of tree
    return tmp;

  while (p && !IsBlockNode(p))
  {
    if (NS_FAILED(p->GetParentNode(getter_AddRefs(tmp)))) // no parent, ran off top of tree
      return p;

    p = tmp;
  }
  return p;
}


///////////////////////////////////////////////////////////////////////////
// HasSameBlockNodeParent: true if nodes have same block level ancestor
//               
PRBool
nsEditor::HasSameBlockNodeParent(nsIDOMNode *aNode1, nsIDOMNode *aNode2)
{
  if (!aNode1 || !aNode2)
  {
    NS_NOTREACHED("null node passed to HasSameBlockNodeParent()");
    return PR_FALSE;
  }
  
  if (aNode1 == aNode2)
    return PR_TRUE;
    
  nsCOMPtr<nsIDOMNode> p1 = GetBlockNodeParent(aNode1);
  nsCOMPtr<nsIDOMNode> p2 = GetBlockNodeParent(aNode2);

  return (p1 == p2);
}


///////////////////////////////////////////////////////////////////////////
// IsTextOrElementNode: true if node of dom type element or text
//               
PRBool
nsEditor::IsTextOrElementNode(nsIDOMNode *aNode)
{
  if (!aNode)
  {
    NS_NOTREACHED("null node passed to IsTextOrElementNode()");
    return PR_FALSE;
  }
  
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  if ((nodeType == nsIDOMNode::ELEMENT_NODE) || (nodeType == nsIDOMNode::TEXT_NODE))
    return PR_TRUE;
    
  return PR_FALSE;
}



///////////////////////////////////////////////////////////////////////////
// IsTextNode: true if node of dom type text
//               
PRBool
nsEditor::IsTextNode(nsIDOMNode *aNode)
{
  if (!aNode)
  {
    NS_NOTREACHED("null node passed to IsTextNode()");
    return PR_FALSE;
  }
  
  PRUint16 nodeType;
  aNode->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::TEXT_NODE)
    return PR_TRUE;
    
  return PR_FALSE;
}


///////////////////////////////////////////////////////////////////////////
// GetIndexOf: returns the position index of the node in the parent
//
PRInt32 
nsEditor::GetIndexOf(nsIDOMNode *parent, nsIDOMNode *child)
{
  PRInt32 index = 0;
  
  NS_PRECONDITION(parent, "null parent passed to nsEditor::GetIndexOf");
  NS_PRECONDITION(parent, "null child passed to nsEditor::GetIndexOf");
  nsCOMPtr<nsIContent> content = do_QueryInterface(parent);
  nsCOMPtr<nsIContent> cChild = do_QueryInterface(child);
  NS_PRECONDITION(content, "null content in nsEditor::GetIndexOf");
  NS_PRECONDITION(cChild, "null content in nsEditor::GetIndexOf");
  
  nsresult res = content->IndexOf(cChild, index);
  if (NS_FAILED(res)) 
  {
    NS_NOTREACHED("could not find child in parent - nsEditor::GetIndexOf");
  }
  return index;
}
  

///////////////////////////////////////////////////////////////////////////
// GetChildAt: returns the node at this position index in the parent
//
nsCOMPtr<nsIDOMNode> 
nsEditor::GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset)
{
  nsCOMPtr<nsIDOMNode> resultNode;
  
  if (!aParent) 
    return resultNode;
  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aParent);
  nsCOMPtr<nsIContent> cChild;
  NS_PRECONDITION(content, "null content in nsEditor::GetChildAt");
  
  if (NS_FAILED(content->ChildAt(aOffset, *getter_AddRefs(cChild))))
    return resultNode;
  
  resultNode = do_QueryInterface(cChild);
  return resultNode;
}
  


///////////////////////////////////////////////////////////////////////////
// NextNodeInBlock: gets the next/prev node in the block, if any.  Next node
//                  must be an element or text node, others are ignored
nsCOMPtr<nsIDOMNode>
nsEditor::NextNodeInBlock(nsIDOMNode *aNode, IterDirection aDir)
{
  nsCOMPtr<nsIDOMNode> nullNode;
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIContent> blockContent;
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIDOMNode> blockParent;
  
  if (!aNode)  return nullNode;

  nsCOMPtr<nsIContentIterator> iter;
  if (NS_FAILED(nsComponentManager::CreateInstance(kCContentIteratorCID, nsnull,
                                        nsIContentIterator::GetIID(), 
                                        getter_AddRefs(iter))))
    return nullNode;

  // much gnashing of teeth as we twit back and forth between content and domnode types
  content = do_QueryInterface(aNode);
  if (IsBlockNode(aNode))
  {
    blockParent = do_QueryInterface(aNode);
  }
  else
  {
    blockParent = GetBlockNodeParent(aNode);
  }
  if (!blockParent) return nullNode;
  blockContent = do_QueryInterface(blockParent);
  if (!blockContent) return nullNode;
  
  if (NS_FAILED(iter->Init(blockContent)))  return nullNode;
  if (NS_FAILED(iter->PositionAt(content)))  return nullNode;
  
  while (NS_COMFALSE == iter->IsDone())
  {
  	if (NS_FAILED(iter->CurrentNode(getter_AddRefs(content)))) return nullNode;
    // ignore nodes that aren't elements or text, or that are the block parent 
    node = do_QueryInterface(content);
    if (node && IsTextOrElementNode(node) && (node != blockParent) && (node!=nsCOMPtr<nsIDOMNode>(dont_QueryInterface(aNode))))
      return node;
    
    if (aDir == kIterForward)
      iter->Next();
    else
      iter->Prev();
  }
  
  return nullNode;
}


///////////////////////////////////////////////////////////////////////////
// GetStartNodeAndOffset: returns whatever the start parent & offset is of 
//                        the first range in the selection.
nsresult 
nsEditor::GetStartNodeAndOffset(nsIDOMSelection *aSelection,
                                       nsCOMPtr<nsIDOMNode> *outStartNode,
                                       PRInt32 *outStartOffset)
{
  if (!outStartNode || !outStartOffset) 
    return NS_ERROR_NULL_POINTER;
    
  nsCOMPtr<nsIEnumerator> enumerator;
  enumerator = do_QueryInterface(aSelection);
  if (!enumerator) 
    return NS_ERROR_FAILURE;
    
  enumerator->First(); 
  nsISupports *currentItem;
  if ((NS_FAILED(enumerator->CurrentItem(&currentItem))) || !currentItem)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
  if (!range)
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetStartParent(getter_AddRefs(*outStartNode))))
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetStartOffset(outStartOffset)))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}


///////////////////////////////////////////////////////////////////////////
// GetEndNodeAndOffset: returns whatever the end parent & offset is of 
//                        the first range in the selection.
nsresult 
nsEditor::GetEndNodeAndOffset(nsIDOMSelection *aSelection,
                                       nsCOMPtr<nsIDOMNode> *outEndNode,
                                       PRInt32 *outEndOffset)
{
  if (!outEndNode || !outEndOffset) 
    return NS_ERROR_NULL_POINTER;
    
  nsCOMPtr<nsIEnumerator> enumerator;
  enumerator = do_QueryInterface(aSelection);
  if (!enumerator) 
    return NS_ERROR_FAILURE;
    
  enumerator->First(); 
  nsISupports *currentItem;
  if ((NS_FAILED(enumerator->CurrentItem(&currentItem))) || !currentItem)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );
  if (!range)
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetEndParent(getter_AddRefs(*outEndNode))))
    return NS_ERROR_FAILURE;
    
  if (NS_FAILED(range->GetEndOffset(outEndOffset)))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}


///////////////////////////////////////////////////////////////////////////
// IsPreformatted: checks the style info for the node for the preformatted
//                 text style.
nsresult 
nsEditor::IsPreformatted(nsIDOMNode *aNode, PRBool *aResult)
{
  nsresult result;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  nsIFrame *frame;
  nsCOMPtr<nsIStyleContext> styleContext;
  const nsStyleText* styleText;
  PRBool bPreformatted;
  
  if (!aResult || !content) return NS_ERROR_NULL_POINTER;
  
  if (!mPresShell) return NS_ERROR_NULL_POINTER;
  
  result = mPresShell->GetPrimaryFrameFor(content, &frame);
  if (NS_FAILED(result)) return result;
  
  result = mPresShell->GetStyleContextFor(frame, getter_AddRefs(styleContext));
  if (NS_FAILED(result)) return result;

  styleText = (const nsStyleText*)styleContext->GetStyleData(eStyleStruct_Text);

  bPreformatted = (NS_STYLE_WHITESPACE_PRE == styleText->mWhiteSpace) ||
    (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == styleText->mWhiteSpace);

  *aResult = bPreformatted;
  return NS_OK;
}


///////////////////////////////////////////////////////////////////////////
// IsNextCharWhitespace: checks the adjacent content in the same block
//                       to see if following selection is whitespace
nsresult 
nsEditor::IsNextCharWhitespace(nsIDOMNode *aParentNode, 
                                      PRInt32 aOffset,
                                      PRBool *aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
  *aResult = PR_FALSE;
  
  nsString tempString;
  PRUint32 strLength;
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aParentNode);
  if (textNode)
  {
    textNode->GetLength(&strLength);
    if (aOffset < strLength)
    {
      // easy case: next char is in same node
      textNode->SubstringData(aOffset,aOffset+1,tempString);
      *aResult = nsString::IsSpace(tempString.First());
      return NS_OK;
    }
  }
  
  // harder case: next char in next node.
  nsCOMPtr<nsIDOMNode> node = NextNodeInBlock(aParentNode, kIterForward);
  nsCOMPtr<nsIDOMNode> tmp;
  while (node) 
  {
    if (!IsInlineNode(node))  // skip over bold, italic, link, ect nodes
    {
      if (IsTextNode(node))
      {
        textNode = do_QueryInterface(node);
        textNode->GetLength(&strLength);
        if (strLength)
        {
          textNode->SubstringData(0,1,tempString);
          *aResult = nsString::IsSpace(tempString.First());
          return NS_OK;
        }
        // else it's an empty text node, skip it.
      }
      else  // node is an image or some other thingy that doesn't count as whitespace
      {
        break;
      }
    }
    tmp = node;
    node = NextNodeInBlock(tmp, kIterForward);
  }
  
  return NS_OK;
}


///////////////////////////////////////////////////////////////////////////
// IsPrevCharWhitespace: checks the adjacent content in the same block
//                       to see if following selection is whitespace
nsresult 
nsEditor::IsPrevCharWhitespace(nsIDOMNode *aParentNode, 
                                      PRInt32 aOffset,
                                      PRBool *aResult)
{
  if (!aResult) return NS_ERROR_NULL_POINTER;
  *aResult = PR_FALSE;
  
  nsString tempString;
  PRUint32 strLength;
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aParentNode);
  if (textNode)
  {
    if (aOffset > 0)
    {
      // easy case: prev char is in same node
      textNode->SubstringData(aOffset-1,aOffset,tempString);
      *aResult = nsString::IsSpace(tempString.First());
      return NS_OK;
    }
  }
  
  // harder case: prev char in next node
  nsCOMPtr<nsIDOMNode> node = NextNodeInBlock(aParentNode, kIterBackward);
  nsCOMPtr<nsIDOMNode> tmp;
  while (node) 
  {
    if (!IsInlineNode(node))  // skip over bold, italic, link, ect nodes
    {
      if (IsTextNode(node))
      {
        textNode = do_QueryInterface(node);
        textNode->GetLength(&strLength);
        if (strLength)
        {
          textNode->SubstringData(strLength-1,strLength,tempString);
          *aResult = nsString::IsSpace(tempString.First());
          return NS_OK;
        }
        // else it's an empty text node, skip it.
      }
      else  // node is an image or some other thingy that doesn't count as whitespace
      {
        break;
      }
    }
    // otherwise we found a node we want to skip, keep going
    tmp = node;
    node = NextNodeInBlock(tmp, kIterBackward);
  }
  
  return NS_OK;
  
}


///////////////////////////////////////////////////////////////////////////
// SplitNodeDeep: this splits a node "deeply", splitting children as 
//                appropriate.  The place to split is represented by
//                a dom point at {splitPointParent, splitPointOffset}.
//                That dom point must be inside aNode, which is the node to 
//                split.
nsresult
nsEditor::SplitNodeDeep(nsIDOMNode *aNode, 
                        nsIDOMNode *aSplitPointParent, 
                        PRInt32 aSplitPointOffset)
{
  if (!aNode || !aSplitPointParent) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> nodeToSplit = do_QueryInterface(aSplitPointParent);
  nsCOMPtr<nsIDOMNode> tempNode;  
  PRInt32 offset = aSplitPointOffset;
  
  while (nodeToSplit)
  {
    nsresult res = SplitNode(nodeToSplit, offset, getter_AddRefs(tempNode));
    if (NS_FAILED(res)) return res;
    
    if (nodeToSplit.get() == aNode)  // we split all the way up to (and including) aNode; we're done
      break;
      
    tempNode = nodeToSplit;
    res = tempNode->GetParentNode(getter_AddRefs(nodeToSplit));
    offset = GetIndexOf(nodeToSplit, tempNode);
  }
  
  if (!nodeToSplit)
  {
    NS_NOTREACHED("null node obtained in nsEditor::SplitNodeDeep()");
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}


///////////////////////////////////////////////////////////////////////////
// JoinNodeDeep:  this joins two like nodes "deeply", joining children as 
//                appropriate.  
nsresult
nsEditor::JoinNodeDeep(nsIDOMNode *aLeftNode, 
                       nsIDOMNode *aRightNode,
                       nsIDOMSelection *aSelection) 
{
  if (!aLeftNode || !aRightNode) return NS_ERROR_NULL_POINTER;

  // while the rightmost children and their descendants of the left node 
  // match the leftmost children and their descendants of the right node
  // join them up.  Can you say that three times fast?
   
  nsCOMPtr<nsIDOMNode> leftNodeToJoin = do_QueryInterface(aLeftNode);
  nsCOMPtr<nsIDOMNode> rightNodeToJoin = do_QueryInterface(aRightNode);
  nsCOMPtr<nsIDOMNode> parentNode;
  PRInt32 offset;
  nsresult res = NS_OK;
  
  rightNodeToJoin->GetParentNode(getter_AddRefs(parentNode));
  
  while (leftNodeToJoin && rightNodeToJoin && parentNode &&
          NodesSameType(leftNodeToJoin, rightNodeToJoin))
  {
    res = JoinNodes(leftNodeToJoin,rightNodeToJoin,parentNode);
    if (NS_FAILED(res)) return res;
    
    res = GetStartNodeAndOffset(aSelection, &parentNode, &offset);
    if (NS_FAILED(res)) return res;
    
    if (offset == 0)  // no new left node; we're done joining
      return NS_OK;

    if (IsTextNode(parentNode)) // we've joined all the way down to text nodes, we're done!
      return NS_OK;

    else
    {
      // get new left and right nodes, and begin anew
      leftNodeToJoin = GetChildAt(parentNode, offset-1);
      rightNodeToJoin = GetChildAt(parentNode, offset);
    }
  }
  
  return res;
}

