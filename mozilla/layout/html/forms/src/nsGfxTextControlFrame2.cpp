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
 *
 * Contributor(s): 
 */

#ifdef ENDER_LITE

#include "nsCOMPtr.h"
#include "nsGfxTextControlFrame2.h"
#include "nsIDocument.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIFormControl.h"
#include "nsIServiceManager.h"
#include "nsIFrameSelection.h"
#include "nsIHTMLEditor.h"
#include "nsEditorCID.h"
#include "nsLayoutCID.h"
#include "nsFormControlHelper.h"
#include "nsIDocumentEncoder.h"
#include "nsICaret.h"
#include "nsIDOMSelectionListener.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIEditorController.h"
#include "nsIElementFactory.h"
#include "nsIHTMLContent.h"
#include "nsFormFrame.h"
#include "nsIEditorIMESupport.h"
#include "nsIEditorObserver.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIScrollableView.h"
#include "nsIScrollableFrame.h" //to turn off scroll bars
#include "nsFormControlFrame.h" //for registering accesskeys
#include "nsIDeviceContext.h" // to measure fonts
#include "nsIPresState.h" //for saving state
#include "nsLinebreakConverter.h" //to strip out carriage returns
#include "nsIEditorMailSupport.h"

#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"
#include "nsIComponentManager.h"
#include "nsIView.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIPresShell.h"
#include "nsIComponentManager.h"

#include "nsBoxLayoutState.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h" //getframetype
//for keylistener for "return" check
#include "nsIDOMKeyListener.h" 
#include "nsIDOMKeyEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDocument.h" //observe documents to send onchangenotifications
#include "nsIStyleSheet.h"//observe documents to send onchangenotifications
#include "nsIStyleRule.h"//observe documents to send onchangenotifications
#include "nsIDOMEventListener.h"//observe documents to send onchangenotifications

#include "nsIDOMFocusListener.h" //onchange events
#include "nsIDOMCharacterData.h" //for selection setting helper func
#include "nsIDOMNodeList.h" //for selection settting helper func
#include "nsIDOMRange.h" //for selection settting helper func
#include "nsRange.h" //for selection settting helper func (i cant believe this is exported!?)
#include "nsIScriptGlobalObject.h" //needed for notify selection changed to update the menus ect.
#include "nsIDOMWindow.h" //needed for notify selection changed to update the menus ect.
#include "nsITextContent.h" //needed to create initial text control content

#include "nsITransactionManager.h"
#include "nsITransactionListener.h"

#define DEFAULT_COLUMN_WIDTH 20


static NS_DEFINE_CID(kHTMLEditorCID, NS_HTMLEDITOR_CID);
static NS_DEFINE_CID(kFrameSelectionCID, NS_FRAMESELECTION_CID);


static nsresult GetElementFactoryService(nsIElementFactory **aFactory)
{
  nsresult rv(NS_OK);
  static nsWeakPtr sElementFactory = getter_AddRefs( NS_GetWeakReference(nsCOMPtr<nsIElementFactory>(do_GetService(
                     NS_ELEMENT_FACTORY_PROGID_PREFIX"http://www.w3.org/1999/xhtml", &rv) )));
  if (sElementFactory)
  {
    nsCOMPtr<nsIElementFactory> fac(do_QueryReferent(sElementFactory));
    *aFactory = fac.get();
    if (!*aFactory)
      rv = NS_ERROR_FAILURE;
    NS_IF_ADDREF(*aFactory);
  }
  else
    return NS_ERROR_FAILURE;
  return rv;
}


class nsTextInputListener : public nsIDOMKeyListener,
                            public nsIDOMSelectionListener,
                            public nsIDOMFocusListener,
                            public nsIEditorObserver,
                            public nsITransactionListener,
                            public nsSupportsWeakReference
{
public:
  /** the default constructor
   */ 
  nsTextInputListener();
  /** the default destructor. virtual due to the possibility of derivation.
   */
  virtual ~nsTextInputListener();

  /** SetEditor gives an address to the editor that will be accessed
   *  @param aEditor the editor this listener calls for editing operations
   */
  void SetFrame(nsGfxTextControlFrame2 *aFrame){mFrame = aFrame;}

/*interfaces for addref and release and queryinterface*/
  NS_DECL_ISUPPORTS

/*BEGIN interfaces in to the keylister base interface. must be supplied to handle pure virtual interfaces
  see the nsIDOMKeyListener interface implementation for details
  */
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyUp(nsIDOMEvent* aKeyEvent);
  virtual nsresult KeyPress(nsIDOMEvent* aKeyEvent);
/*END interfaces from nsIDOMKeyListener*/
/*BEGIN nsIDOMSelectionListener Interface*/
  NS_IMETHOD    NotifySelectionChanged(nsIDOMDocument* aDoc, nsIDOMSelection* aSel, PRInt16 aReason);
/*END nsIDOMSelectionListener*/

/* BEGIN EditorObserver*/
  NS_IMETHOD EditAction();
/*END EditorObserver*/

  /** nsIDOMFocusListener interfaces 
    * used to propogate focus, blur, and change notifications
    * @see nsIDOMFocusListener
    */
  virtual nsresult Focus(nsIDOMEvent* aEvent); //must use virtual nsresult. the header does for somereason
  virtual nsresult Blur (nsIDOMEvent* aEvent);
  /* END interfaces from nsIDOMFocusListener*/


  /** nsITransactionListener interfaces
    */
  
  NS_IMETHOD WillDo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidDo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aDoResult);
  NS_IMETHOD WillUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidUndo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aUndoResult);
  NS_IMETHOD WillRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, PRBool *aInterrupt);
  NS_IMETHOD DidRedo(nsITransactionManager *aManager, nsITransaction *aTransaction, nsresult aRedoResult);
  NS_IMETHOD WillBeginBatch(nsITransactionManager *aManager, PRBool *aInterrupt);
  NS_IMETHOD DidBeginBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillEndBatch(nsITransactionManager *aManager, PRBool *aInterrupt);
  NS_IMETHOD DidEndBatch(nsITransactionManager *aManager, nsresult aResult);
  NS_IMETHOD WillMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                       nsITransaction *aTransactionToMerge, PRBool *aInterrupt);
  NS_IMETHOD DidMerge(nsITransactionManager *aManager, nsITransaction *aTopTransaction,
                      nsITransaction *aTransactionToMerge,
                      PRBool aDidMerge, nsresult aMergeResult);


protected:

  nsresult  UpdateTextInputCommands(const nsString& commandsToUpdate);

protected:

  nsGfxTextControlFrame2* mFrame;  // weak reference
  nsString mFocusedValue;
  
  PRPackedBool    mSelectionWasCollapsed;
  PRPackedBool    mKnowSelectionCollapsed;

  PRPackedBool    mFirstDoOfFirstUndo;
  
};


/*
 * nsTextEditorListener implementation
 */

NS_IMPL_ADDREF(nsTextInputListener)

NS_IMPL_RELEASE(nsTextInputListener)


nsTextInputListener::nsTextInputListener()
: mFrame(nsnull)
, mSelectionWasCollapsed(PR_TRUE)
, mKnowSelectionCollapsed(PR_FALSE)
, mFirstDoOfFirstUndo(PR_TRUE)
{
  NS_INIT_REFCNT();
}



nsTextInputListener::~nsTextInputListener() 
{
}


NS_IMPL_QUERY_INTERFACE6(nsTextInputListener,
                          nsIDOMKeyListener,
                          nsIDOMSelectionListener,
                          nsIDOMFocusListener,
                          nsIEditorObserver,
                          nsITransactionListener,
                          nsISupportsWeakReference)

nsresult
nsTextInputListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

// individual key handlers return NS_OK to indicate NOT consumed
// by default, an error is returned indicating event is consumed
// joki is fixing this interface.
nsresult
nsTextInputListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextInputListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextInputListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  if (!mFrame)
    return NS_OK;
  nsCOMPtr<nsIDOMKeyEvent>keyEvent;
  keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) 
  {
    //non-key event passed to keydown.  bad things.
    return NS_OK;
  }
  
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(aKeyEvent);
  if(privateEvent) 
  {
    PRBool dispatchStopped;
    privateEvent->IsDispatchStopped(&dispatchStopped);
    if(dispatchStopped)
      return NS_OK;
  }
  PRUint32     keyCode;
  keyEvent->GetKeyCode(&keyCode);
  if (nsIDOMKeyEvent::DOM_VK_RETURN==keyCode
      || nsIDOMKeyEvent::DOM_VK_ENTER==keyCode)
  {
    mFrame->SubmitAttempt();
  }
  return NS_OK;
}

//END KeyListener

//BEGIN NS_IDOMSELECITONLISTENER


NS_IMETHODIMP
nsTextInputListener::NotifySelectionChanged(nsIDOMDocument* aDoc, nsIDOMSelection* aSel, PRInt16 aReason)
{
  PRBool collapsed;
  if (!mFrame || !aDoc || !aSel || NS_FAILED(aSel->GetIsCollapsed(&collapsed)))
    return NS_OK;

  // if the collapsed state did not change, don't fire notifications
  if (mKnowSelectionCollapsed && collapsed == mSelectionWasCollapsed)
    return NS_OK;
    
  mSelectionWasCollapsed = collapsed;
  mKnowSelectionCollapsed = PR_TRUE;

  return UpdateTextInputCommands(NS_ConvertASCIItoUCS2("select"));
}

//EDITOR INTERFACE

NS_IMETHODIMP nsTextInputListener::EditAction()
{
  return mFrame->InternalContentChanged();
}

//END NS_IDOMSELECTIONLISTENER
//focuslistener

nsresult
nsTextInputListener::Focus(nsIDOMEvent* aEvent)
{
  if (!mFrame) return NS_OK;
  nsCOMPtr<nsIEditor> editor;
  mFrame->GetEditor(getter_AddRefs(editor));
  if (editor)
  {
    editor->AddEditorObserver(this);
  }
  
  nsresult rv = mFrame->GetText(&mFocusedValue, PR_FALSE);
  if (NS_FAILED(rv)) return rv;
  
  return NS_OK;
}

nsresult
nsTextInputListener::Blur (nsIDOMEvent* aEvent)
{
  if (!mFrame)
    return NS_OK;
    
  nsAutoString blurValue;
  mFrame->GetText(&blurValue,PR_FALSE);
  if (mFocusedValue.Compare(blurValue))//different fire onchange
  {
    mFrame->CallOnChange();
  }
  nsCOMPtr<nsIEditor> editor;
  mFrame->GetEditor(getter_AddRefs(editor));
  if (editor)
  {
    editor->RemoveEditorObserver(this);
  }

  return NS_OK;
}


//END focuslistener

NS_IMETHODIMP nsTextInputListener::WillDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidDo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aDoResult)
{
  // we only need to update if the undo count is now 1
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 1)
  {
    if (mFirstDoOfFirstUndo)
      UpdateTextInputCommands(NS_ConvertASCIItoUCS2("undo"));

    mFirstDoOfFirstUndo = PR_FALSE;
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::WillUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidUndo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, nsresult aUndoResult)
{
  PRInt32 undoCount;
  aManager->GetNumberOfUndoItems(&undoCount);
  if (undoCount == 0)
    mFirstDoOfFirstUndo = PR_TRUE;    // reset the state for the next do

  UpdateTextInputCommands(NS_ConvertASCIItoUCS2("undo"));
  
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::WillRedo(nsITransactionManager *aManager,
  nsITransaction *aTransaction, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidRedo(nsITransactionManager *aManager,  
  nsITransaction *aTransaction, nsresult aRedoResult)
{
  UpdateTextInputCommands(NS_ConvertASCIItoUCS2("undo"));
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::WillBeginBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidBeginBatch(nsITransactionManager *aManager, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::WillEndBatch(nsITransactionManager *aManager, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidEndBatch(nsITransactionManager *aManager, nsresult aResult)
{
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::WillMerge(nsITransactionManager *aManager,
        nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge, PRBool *aInterrupt)
{
  *aInterrupt = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTextInputListener::DidMerge(nsITransactionManager *aManager,
      nsITransaction *aTopTransaction, nsITransaction *aTransactionToMerge,
      PRBool aDidMerge, nsresult aMergeResult)
{
  return NS_OK;
}



nsresult nsTextInputListener::UpdateTextInputCommands(const nsString& commandsToUpdate)
{
  if (!mFrame) return NS_ERROR_NOT_INITIALIZED;
  
  nsCOMPtr<nsIContent> content;
  nsresult rv = mFrame->GetContent(getter_AddRefs(content));
  if (NS_FAILED(rv) || !content ) 
    return rv?rv:NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDocument> doc;
  rv = content->GetDocument(*getter_AddRefs(doc));
  if (NS_FAILED(rv) || !doc ) 
    return rv?rv:NS_ERROR_FAILURE;

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject;
  rv = doc->GetScriptGlobalObject(getter_AddRefs(scriptGlobalObject));
  if (NS_FAILED(rv) || !scriptGlobalObject ) 
    return rv?rv:NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(scriptGlobalObject, &rv);
  if (NS_FAILED(rv) || !domWindow ) 
    return rv?rv:NS_ERROR_FAILURE;

  return domWindow->UpdateCommands(commandsToUpdate);
}


//END NSTEXTINPUTLISTENER

#ifdef XP_MAC
#pragma mark -
#endif
  
class nsTextInputSelectionImpl : public nsSupportsWeakReference, public nsISelectionController, public nsIFrameSelection
{
public:
  NS_DECL_ISUPPORTS

  nsTextInputSelectionImpl(nsIFrameSelection *aSel, nsIPresShell *aShell, nsIContent *aLimiter);
  ~nsTextInputSelectionImpl(){}

  
  //NSISELECTIONCONTROLLER INTERFACES
  NS_IMETHOD SetDisplaySelection(PRInt16 toggle);
  NS_IMETHOD GetDisplaySelection(PRInt16 *_retval);
  NS_IMETHOD SetDisplayNonTextSelection(PRBool toggle);
  NS_IMETHOD GetDisplayNonTextSelection(PRBool *_retval);
  NS_IMETHOD GetSelection(PRInt16 type, nsIDOMSelection **_retval);
  NS_IMETHOD ScrollSelectionIntoView(PRInt16 type, PRInt16 region);
  NS_IMETHOD RepaintSelection(PRInt16 type);
  NS_IMETHOD RepaintSelection(nsIPresContext* aPresContext, SelectionType aSelectionType);
  NS_IMETHOD SetCaretEnabled(PRBool enabled);
  NS_IMETHOD SetCaretWidth(PRInt16 twips);
  NS_IMETHOD SetCaretReadOnly(PRBool aReadOnly);
  NS_IMETHOD GetCaretEnabled(PRBool *_retval);
  NS_IMETHOD CharacterMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD WordMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD LineMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD IntraLineMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD PageMove(PRBool aForward, PRBool aExtend){return NS_OK;}//*
  NS_IMETHOD CompleteScroll(PRBool aForward){return NS_OK;}//*
  NS_IMETHOD CompleteMove(PRBool aForward, PRBool aExtend){return NS_OK;}//*
  NS_IMETHOD ScrollPage(PRBool aForward){return NS_OK;}//*
  NS_IMETHOD ScrollLine(PRBool aForward){return NS_OK;}//*
  NS_IMETHOD ScrollHorizontal(PRBool aLeft){return NS_OK;}//*
  NS_IMETHOD SelectAll(void);
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, PRBool *_retval);

  //NSIFRAMSELECTION INTERFACES
  NS_IMETHOD Init(nsIFocusTracker *aTracker, nsIContent *aLimiter) ;
  NS_IMETHOD ShutDown() ;
  NS_IMETHOD HandleTextEvent(nsGUIEvent *aGuiEvent) ;
  NS_IMETHOD HandleKeyEvent(nsIPresContext* aPresContext, nsGUIEvent *aGuiEvent);
  NS_IMETHOD HandleClick(nsIContent *aNewFocus, PRUint32 aContentOffset, PRUint32 aContentEndOffset , 
                       PRBool aContinueSelection, PRBool aMultipleSelection, PRBool aHint); 
  NS_IMETHOD HandleDrag(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint);
  NS_IMETHOD HandleTableSelection(nsIContent *aParentContent, PRInt32 aContentOffset, PRUint32 aTarget, nsMouseEvent *aMouseEvent);
  NS_IMETHOD StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay);
  NS_IMETHOD StopAutoScrollTimer();
  NS_IMETHOD EnableFrameNotification(PRBool aEnable);
  NS_IMETHOD LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                             SelectionDetails **aReturnDetails, PRBool aSlowCheck);
  NS_IMETHOD SetMouseDownState(PRBool aState);
  NS_IMETHOD GetMouseDownState(PRBool *aState);
  NS_IMETHOD SetDelayCaretOverExistingSelection(PRBool aDelay);
  NS_IMETHOD GetDelayCaretOverExistingSelection(PRBool *aDelay);
  NS_IMETHOD SetDelayedCaretData(nsMouseEvent *aMouseEvent);
  NS_IMETHOD GetDelayedCaretData(nsMouseEvent **aMouseEvent);
  NS_IMETHOD GetLimiter(nsIContent **aLimiterContent);
  NS_IMETHOD GetTableCellSelection(PRBool *aState);
  NS_IMETHOD GetTableCellSelectionStyleColor(const nsStyleColor **aStyleColor);
  NS_IMETHOD GetFrameForNodeOffset(nsIContent *aNode, PRInt32 aOffset, HINT aHint, nsIFrame **aReturnFrame, PRInt32 *aReturnOffset);
  NS_IMETHOD AdjustOffsetsFromStyle(nsIFrame *aFrame, PRBool *changeSelection,
      nsIContent** outContent, PRInt32* outStartOffset, PRInt32* outEndOffset);
  NS_IMETHOD GetHint(nsIFrameSelection::HINT *aHint);
  NS_IMETHOD SetHint(nsIFrameSelection::HINT aHint);
  NS_IMETHOD SetScrollableView(nsIScrollableView *aScrollableView);
  //END INTERFACES


  nsWeakPtr &GetPresShell(){return mPresShellWeak;}
private:
  nsCOMPtr<nsIFrameSelection> mFrameSelection;
  nsCOMPtr<nsIContent>        mLimiter;
  nsWeakPtr mPresShellWeak;
};

// Implement our nsISupports methods
NS_IMPL_ISUPPORTS3(nsTextInputSelectionImpl, nsISelectionController, nsISupportsWeakReference, nsIFrameSelection)


// BEGIN nsTextInputSelectionImpl

nsTextInputSelectionImpl::nsTextInputSelectionImpl(nsIFrameSelection *aSel, nsIPresShell *aShell, nsIContent *aLimiter)
{
  NS_INIT_REFCNT();
  if (aSel && aShell)
  {
    mFrameSelection = aSel;//we are the owner now!
    nsCOMPtr<nsIFocusTracker> tracker = do_QueryInterface(aShell);
    mLimiter = aLimiter;
    mFrameSelection->Init(tracker, mLimiter);
    mPresShellWeak = getter_AddRefs( NS_GetWeakReference(aShell) );
  }
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDisplaySelection(PRInt16 aToggle)
{
  if (mFrameSelection)
    return mFrameSelection->SetDisplaySelection(aToggle);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDisplaySelection(PRInt16 *aToggle)
{
  if (mFrameSelection)
    return mFrameSelection->GetDisplaySelection(aToggle);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDisplayNonTextSelection(PRBool aToggle)
{
  return NS_OK;//stub this out. not used in input
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDisplayNonTextSelection(PRBool *aToggle)
{
  return NS_OK;//stub this out. not used in input
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetSelection(PRInt16 type, nsIDOMSelection **_retval)
{
  if (mFrameSelection)
    return mFrameSelection->GetSelection(type, _retval);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::ScrollSelectionIntoView(PRInt16 type, PRInt16 region)
{
  if (mFrameSelection)
    return mFrameSelection->ScrollSelectionIntoView(type, region);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::RepaintSelection(PRInt16 type)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(mPresShellWeak);
  if (presShell)
  {
    nsCOMPtr<nsIPresContext> context;
    if (NS_SUCCEEDED(presShell->GetPresContext(getter_AddRefs(context))) && context)
    {
      return mFrameSelection->RepaintSelection(context, type);
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::RepaintSelection(nsIPresContext* aPresContext, SelectionType aSelectionType)
{
  return RepaintSelection(aSelectionType);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretEnabled(PRBool enabled)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsCOMPtr<nsICaret> caret;
    if (NS_SUCCEEDED(result = shell->GetCaret(getter_AddRefs(caret))))
    {
      nsCOMPtr<nsIDOMSelection> domSel;
      if (NS_SUCCEEDED(result = mFrameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel))))
      {
        nsCOMPtr<nsIDOMSelection> domSel;
        if (NS_SUCCEEDED(GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel))) && domSel)
          caret->SetCaretDOMSelection(domSel);
        return caret->SetCaretVisible(enabled);
      }
    }

  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretWidth(PRInt16 pixels)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsCOMPtr<nsICaret> caret;
    if (NS_SUCCEEDED(result = shell->GetCaret(getter_AddRefs(caret))))
    {
        return caret->SetCaretWidth(pixels);
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretReadOnly(PRBool aReadOnly)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsCOMPtr<nsICaret> caret;
    if (NS_SUCCEEDED(result = shell->GetCaret(getter_AddRefs(caret))))
    {
      nsCOMPtr<nsIDOMSelection> domSel;
      if (NS_SUCCEEDED(result = mFrameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel))))
      {
        return caret->SetCaretReadOnly(aReadOnly);
      }
    }

  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetCaretEnabled(PRBool *_retval)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsCOMPtr<nsICaret> caret;
    if (NS_SUCCEEDED(result = shell->GetCaret(getter_AddRefs(caret))))
    {
      nsCOMPtr<nsIDOMSelection> domSel;
      if (NS_SUCCEEDED(result = mFrameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel))))
      {
        return caret->GetCaretVisible(_retval);
      }
    }

  }
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::CharacterMove(PRBool aForward, PRBool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->CharacterMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::WordMove(PRBool aForward, PRBool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->WordMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::LineMove(PRBool aForward, PRBool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->LineMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::IntraLineMove(PRBool aForward, PRBool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->IntraLineMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::SelectAll()
{
  if (mFrameSelection)
    return mFrameSelection->SelectAll();
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, PRBool *_retval)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsISelectionController> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    return shell->CheckVisibility(node,startOffset,EndOffset, _retval);
  }
  return NS_ERROR_FAILURE;

}


//nsTextInputSelectionImpl::FRAMESELECTIONAPIS

NS_IMETHODIMP
nsTextInputSelectionImpl::Init(nsIFocusTracker *aTracker, nsIContent *aLimiter)
{
  return mFrameSelection->Init(aTracker, aLimiter);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::ShutDown()
{
  return mFrameSelection->ShutDown();
}


NS_IMETHODIMP
nsTextInputSelectionImpl::HandleTextEvent(nsGUIEvent *aGuiEvent)
{
  return mFrameSelection->HandleTextEvent(aGuiEvent);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::HandleKeyEvent(nsIPresContext* aPresContext, nsGUIEvent *aGuiEvent)
{
  return mFrameSelection->HandleKeyEvent(aPresContext, aGuiEvent);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::HandleClick(nsIContent *aNewFocus, PRUint32 aContentOffset, PRUint32 aContentEndOffset , 
                     PRBool aContinueSelection, PRBool aMultipleSelection, PRBool aHint)
{
  return mFrameSelection->HandleClick(aNewFocus, aContentOffset, aContentEndOffset , 
                     aContinueSelection, aMultipleSelection, aHint);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::HandleDrag(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint)
{
  return mFrameSelection->HandleDrag(aPresContext, aFrame, aPoint);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::HandleTableSelection(nsIContent *aParentContent, PRInt32 aContentOffset, PRUint32 aTarget, nsMouseEvent *aMouseEvent)
{
  // We should never have a table inside a text control frame!
  NS_ASSERTION(PR_TRUE, "Calling HandleTableSelection inside nsGfxTextControlFrame!");
  return NS_OK;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::StartAutoScrollTimer(nsIPresContext *aPresContext, nsIFrame *aFrame, nsPoint& aPoint, PRUint32 aDelay)
{
  return mFrameSelection->StartAutoScrollTimer(aPresContext, aFrame, aPoint, aDelay);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::StopAutoScrollTimer()
{
  return mFrameSelection->StopAutoScrollTimer();
}


NS_IMETHODIMP
nsTextInputSelectionImpl::EnableFrameNotification(PRBool aEnable)
{
  return mFrameSelection->EnableFrameNotification(aEnable);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::LookUpSelection(nsIContent *aContent, PRInt32 aContentOffset, PRInt32 aContentLength,
                           SelectionDetails **aReturnDetails, PRBool aSlowCheck)
{
  return mFrameSelection->LookUpSelection(aContent, aContentOffset, aContentLength, aReturnDetails, aSlowCheck);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::SetMouseDownState(PRBool aState)
{
  return mFrameSelection->SetMouseDownState(aState);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::GetMouseDownState(PRBool *aState)
{
  return mFrameSelection->GetMouseDownState(aState);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDelayCaretOverExistingSelection(PRBool aDelay)
{
  return mFrameSelection->SetDelayCaretOverExistingSelection(aDelay);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDelayCaretOverExistingSelection(PRBool *aDelay)
{
  return mFrameSelection->GetDelayCaretOverExistingSelection(aDelay);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDelayedCaretData(nsMouseEvent *aMouseEvent)
{
  return mFrameSelection->SetDelayedCaretData(aMouseEvent);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDelayedCaretData(nsMouseEvent **aMouseEvent)
{
  return mFrameSelection->GetDelayedCaretData(aMouseEvent);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetLimiter(nsIContent **aLimiterContent)
{
  return mFrameSelection->GetLimiter(aLimiterContent);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetTableCellSelection(PRBool *aState)
{
  return mFrameSelection->GetTableCellSelection(aState);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::GetTableCellSelectionStyleColor(const nsStyleColor **aStyleColor)
{
  return mFrameSelection->GetTableCellSelectionStyleColor(aStyleColor);
}


NS_IMETHODIMP
nsTextInputSelectionImpl::GetFrameForNodeOffset(nsIContent *aNode, PRInt32 aOffset, HINT aHint, nsIFrame **aReturnFrame, PRInt32 *aReturnOffset)
{
  return mFrameSelection->GetFrameForNodeOffset(aNode, aOffset, aHint,aReturnFrame,aReturnOffset);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::AdjustOffsetsFromStyle(nsIFrame *aFrame, PRBool *changeSelection,
      nsIContent** outContent, PRInt32* outStartOffset, PRInt32* outEndOffset)
{
  return mFrameSelection->AdjustOffsetsFromStyle(aFrame, changeSelection, outContent, outStartOffset, outEndOffset);
}

NS_IMETHODIMP nsTextInputSelectionImpl::GetHint(nsIFrameSelection::HINT *aHint)
{
  return mFrameSelection->GetHint(aHint);
}

NS_IMETHODIMP nsTextInputSelectionImpl::SetHint(nsIFrameSelection::HINT aHint)
{
  return mFrameSelection->SetHint(aHint);
}

NS_IMETHODIMP nsTextInputSelectionImpl::SetScrollableView(nsIScrollableView *aScrollableView)
{
  if(mFrameSelection) 
    return mFrameSelection->SetScrollableView(aScrollableView);
  return NS_ERROR_FAILURE;
}



// END   nsTextInputSelectionImpl



nsresult
NS_NewGfxTextControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsGfxTextControlFrame2* it = new (aPresShell) nsGfxTextControlFrame2(aPresShell);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(nsGfxTextControlFrame2, nsBoxFrame);
NS_IMPL_RELEASE_INHERITED(nsGfxTextControlFrame2, nsBoxFrame);
 

NS_IMETHODIMP
nsGfxTextControlFrame2::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIFormControlFrame))) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = (void*)(nsIAnonymousContentCreator*) this;
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIGfxTextControlFrame2))) {
    *aInstancePtr = (void*)(nsIGfxTextControlFrame2*) this;
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIStatefulFrame))) {
    *aInstancePtr = (void*)(nsIStatefulFrame*) this;
    return NS_OK;
  }
  return nsBoxFrame::QueryInterface(aIID, aInstancePtr);
}

nsGfxTextControlFrame2::nsGfxTextControlFrame2(nsIPresShell* aShell):nsStackFrame(aShell)
{
  mIsProcessing=PR_FALSE;
  mNotifyOnInput = PR_FALSE;
  mFormFrame = nsnull;
  mCachedState = nsnull;
  mSuggestedWidth = NS_FORMSIZE_NOTSET;
  mSuggestedHeight = NS_FORMSIZE_NOTSET;
}

nsGfxTextControlFrame2::~nsGfxTextControlFrame2()
{
  //delete mTextListener;
  //delete mTextSelImpl; dont delete this since mSelCon will release it.
}

NS_IMETHODIMP
nsGfxTextControlFrame2::Destroy(nsIPresContext* aPresContext)
{
  // Clean up the controller
  nsCOMPtr<nsIControllers> controllers;
  nsCOMPtr<nsIDOMNSHTMLInputElement> inputElement = do_QueryInterface(mContent);
  if (inputElement)
    inputElement->GetControllers(getter_AddRefs(controllers));
  else
  {
    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> textAreaElement = do_QueryInterface(mContent);
    textAreaElement->GetControllers(getter_AddRefs(controllers));
  }

  if (controllers)
  {
    PRUint32 numControllers;
    nsresult rv = controllers->GetControllerCount(&numControllers);
    NS_ASSERTION((NS_SUCCEEDED(rv)), "bad result in gfx text control destructor");
    for (PRUint32 i = 0; i < numControllers; i ++)
    {
      nsCOMPtr<nsIController> controller;
      rv = controllers->GetControllerAt(i, getter_AddRefs(controller));
      if (NS_SUCCEEDED(rv) && controller)
      {
        nsCOMPtr<nsIEditorController> editController = do_QueryInterface(controller);
        if (editController)
        {
          editController->SetCommandRefCon(nsnull);
        }
      }
    }
  }

  mSelCon = 0;
  mEditor = 0;
  
  if (mCachedState)
  {
    delete mCachedState;
    mCachedState = nsnull;
  }
  nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);
  if (mFormFrame) {
    mFormFrame->RemoveFormControlFrame(*this);
    mFormFrame = nsnull;
  }
  nsCOMPtr<nsIDOMEventReceiver> erP;
  if (mTextListener)
  {
    if (NS_SUCCEEDED(mContent->QueryInterface(NS_GET_IID(nsIDOMEventReceiver), getter_AddRefs(erP))) && erP)
    {
      erP->RemoveEventListenerByIID(NS_STATIC_CAST(nsIDOMFocusListener  *,mTextListener), NS_GET_IID(nsIDOMFocusListener));
      erP->RemoveEventListenerByIID(NS_STATIC_CAST(nsIDOMKeyListener*,mTextListener), NS_GET_IID(nsIDOMKeyListener));
    }
    nsCOMPtr<nsIPresShell> shell;
    nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_FAILED(rv) || !shell)
      return rv?rv:NS_ERROR_FAILURE;

    if (mEditor)
    {
      mEditor->RemoveEditorObserver(mTextListener);
    }
  }
  return nsBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP 
nsGfxTextControlFrame2::GetFrameType(nsIAtom** aType) const 
{ 
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer"); 
  *aType = nsLayoutAtoms::textInputFrame; 
  NS_ADDREF(*aType); 
  return NS_OK; 
} 

// XXX: wouldn't it be nice to get this from the style context!
PRBool nsGfxTextControlFrame2::IsSingleLineTextControl() const
{
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT==type) || (NS_FORM_INPUT_PASSWORD==type)) {
    return PR_TRUE;
  }
  return PR_FALSE; 
}

// XXX: wouldn't it be nice to get this from the style context!
PRBool nsGfxTextControlFrame2::IsPlainTextControl() const
{
  // need to check HTML attribute of mContent and/or CSS.
  return PR_TRUE;
}

PRBool nsGfxTextControlFrame2::IsPasswordTextControl() const
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_INPUT_PASSWORD==type) {
    return PR_TRUE;
  }
  return PR_FALSE;
}


nsresult 
nsGfxTextControlFrame2::GetColRowSizeAttr(nsIFormControlFrame*  aFrame,
                                         nsIAtom *     aColSizeAttr,
                                         nsHTMLValue & aColSize,
                                         nsresult &    aColStatus,
                                         nsIAtom *     aRowSizeAttr,
                                         nsHTMLValue & aRowSize,
                                         nsresult &    aRowStatus)
{
  nsIContent* iContent = nsnull;
  aFrame->GetFormContent((nsIContent*&) iContent);
  if (!iContent) {
    return NS_ERROR_FAILURE;
  }
  nsIHTMLContent* hContent = nsnull;
  nsresult result = iContent->QueryInterface(kIHTMLContentIID, (void**)&hContent);
  if ((NS_OK != result) || !hContent) {
    NS_RELEASE(iContent);
    return NS_ERROR_FAILURE;
  }

  aColStatus = NS_CONTENT_ATTR_NOT_THERE;
  if (nsnull != aColSizeAttr) {
    aColStatus = hContent->GetHTMLAttribute(aColSizeAttr, aColSize);
  }

  aRowStatus= NS_CONTENT_ATTR_NOT_THERE;
  if (nsnull != aRowSizeAttr) {
    aRowStatus = hContent->GetHTMLAttribute(aRowSizeAttr, aRowSize);
  }

  NS_RELEASE(hContent);
  NS_RELEASE(iContent);
  
  return NS_OK;
}



NS_IMETHODIMP
nsGfxTextControlFrame2::ReflowStandard(nsIPresContext*          aPresContext,
                                       nsSize&                  aDesiredSize,
                                       const nsHTMLReflowState& aReflowState,
                                       nsReflowStatus&          aStatus,
                                       nsMargin&                aBorder,
                                       nsMargin&                aPadding)
{
  // get the css size and let the frame use or override it
  nsSize minSize;
  
  PRInt32 ignore;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    PRInt32 width = 0;
    if (NS_CONTENT_ATTR_HAS_VALUE != GetSizeFromContent(&width)) {
      width = GetDefaultColumnWidth();
    }
    nsInputDimensionSpec textSpec(nsnull, PR_FALSE, nsnull,
                                  nsnull, width, 
                                  PR_FALSE, nsnull, 1);
    CalculateSizeStandard(aPresContext, aReflowState.rendContext, this,
                          textSpec, aDesiredSize, minSize, ignore, aBorder, aPadding);
  } else {
    nsInputDimensionSpec areaSpec(nsHTMLAtoms::cols, PR_FALSE, nsnull, 
                                  nsnull, GetDefaultColumnWidth(), 
                                  PR_FALSE, nsHTMLAtoms::rows, 1);
    CalculateSizeStandard(aPresContext, aReflowState.rendContext, this, 
                          areaSpec, aDesiredSize, minSize, ignore, aBorder, aPadding);
  }

  // CalculateSize makes calls in the nsFormControlHelper that figures
  // out the entire size of the control when in NavQuirks mode. For the
  // textarea, this means the scrollbar sizes hav already been added to
  // its overall size and do not need to be added here.
  if (NS_FORM_TEXTAREA == type) {
    float   p2t;
    aPresContext->GetPixelsToTwips(&p2t);

    nscoord scrollbarWidth  = 0;
    nscoord scrollbarHeight = 0;
    float   scale;
    nsCOMPtr<nsIDeviceContext> dx;
    aPresContext->GetDeviceContext(getter_AddRefs(dx));
    if (dx) { 
      float sbWidth;
      float sbHeight;
      dx->GetCanonicalPixelScale(scale);
      dx->GetScrollBarDimensions(sbWidth, sbHeight);
      scrollbarWidth  = PRInt32(sbWidth * scale);
      scrollbarHeight = PRInt32(sbHeight * scale);
    } else {
      scrollbarWidth  = nsFormControlFrame::GetScrollbarWidth(p2t);
      scrollbarHeight = scrollbarWidth;
    }

    aDesiredSize.height += scrollbarHeight;
    minSize.height      += scrollbarHeight;
    aDesiredSize.width  += scrollbarWidth;
    minSize.width       += scrollbarWidth;
  }
  aDesiredSize.width  += aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right;
  aDesiredSize.height += aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;

  return NS_OK;

}



PRInt32
nsGfxTextControlFrame2::CalculateSizeStandard (nsIPresContext*       aPresContext, 
                                              nsIRenderingContext*  aRendContext,
                                              nsIFormControlFrame*  aFrame,
                                              nsInputDimensionSpec& aSpec, 
                                              nsSize&               aDesiredSize, 
                                              nsSize&               aMinSize, 
                                              nscoord&              aRowHeight,
                                              nsMargin&             aBorder,
                                              nsMargin&             aPadding) 
{
  nscoord charWidth   = 0; 
  aDesiredSize.width  = CSS_NOTSET;
  aDesiredSize.height = CSS_NOTSET;

  nsHTMLValue colAttr;
  nsresult    colStatus;
  nsHTMLValue rowAttr;
  nsresult    rowStatus;
  if (NS_ERROR_FAILURE == GetColRowSizeAttr(aFrame, 
                                            aSpec.mColSizeAttr, colAttr, colStatus,
                                            aSpec.mRowSizeAttr, rowAttr, rowStatus)) {
    return 0;
  }

  float p2t;
  aPresContext->GetScaledPixelsToTwips(&p2t);

  // Calculate the min size of the text control as one char
  // save the current default col size
  nscoord tmpCol = aSpec.mColDefaultSize;
  aSpec.mColDefaultSize = 1;
  charWidth = nsFormControlHelper::GetTextSize(aPresContext, aFrame, aSpec.mColDefaultSize, aDesiredSize, aRendContext); 

  // set the default col size back
  aMinSize.width  = aDesiredSize.width;
  aMinSize.height = aDesiredSize.height;
  aSpec.mColDefaultSize = tmpCol;

  // determine the width, char height, row height
  if (NS_CONTENT_ATTR_HAS_VALUE == colStatus) {  // col attr will provide width
    PRInt32 col = ((colAttr.GetUnit() == eHTMLUnit_Pixel) ? colAttr.GetPixelValue() : colAttr.GetIntValue());
    col = (col <= 0) ? 1 : col; // XXX why a default of 1 char, why hide it
    charWidth = nsFormControlHelper::GetTextSize(aPresContext, aFrame, col, aDesiredSize, aRendContext);
  } else {
    charWidth = nsFormControlHelper::GetTextSize(aPresContext, aFrame, aSpec.mColDefaultSize, aDesiredSize, aRendContext); 
  }

  nscoord fontHeight  = 0;
  // get leading
  nsCOMPtr<nsIFontMetrics> fontMet;
  nsresult res = nsFormControlHelper::GetFrameFontFM(aPresContext, aFrame, getter_AddRefs(fontMet));
  if (NS_SUCCEEDED(res) && fontMet) {
    aRendContext->SetFont(fontMet);
    fontMet->GetHeight(fontHeight);
  }
  aRowHeight      = aDesiredSize.height;
  aMinSize.height = aDesiredSize.height;
  PRInt32 numRows = 0;

  if (NS_CONTENT_ATTR_HAS_VALUE == rowStatus) { // row attr will provide height
    PRInt32 rowAttrInt = ((rowAttr.GetUnit() == eHTMLUnit_Pixel) 
                            ? rowAttr.GetPixelValue() : rowAttr.GetIntValue());
    numRows = (rowAttrInt > 0) ? rowAttrInt : 1;
    aDesiredSize.height = aDesiredSize.height * numRows;
  } else {
    aDesiredSize.height = aDesiredSize.height * aSpec.mRowDefaultSize;
  }

  numRows = (aRowHeight > 0) ? (aDesiredSize.height / aRowHeight) : 0;
  if (numRows == 1) {
    PRInt32 type;
    GetType(&type);
    if (NS_FORM_TEXTAREA == type) {
      aDesiredSize.height += fontHeight;
    }
  }

  return numRows;
}




PRInt32
nsGfxTextControlFrame2::CalculateSizeNavQuirks (nsIPresContext*       aPresContext, 
                                              nsIRenderingContext*  aRendContext,
                                              nsIFormControlFrame*  aFrame,
                                              nsInputDimensionSpec& aSpec, 
                                              nsSize&               aDesiredSize, 
                                              nsSize&               aMinSize, 
                                              nscoord&              aRowHeight,
                                              nsMargin&             aBorder,
                                              nsMargin&             aPadding)
{
  nscoord charWidth   = 0; 
  aDesiredSize.width  = CSS_NOTSET;
  aDesiredSize.height = CSS_NOTSET;

  // Quirks does not use rowAttr
  nsHTMLValue colAttr;
  nsresult    colStatus;
  nsHTMLValue rowAttr;
  nsresult    rowStatus;
  if (NS_ERROR_FAILURE == GetColRowSizeAttr(aFrame, 
                                            aSpec.mColSizeAttr, colAttr, colStatus,
                                            aSpec.mRowSizeAttr, rowAttr, rowStatus)) {
    return 0;
  }

  // Get the Font Metrics for the Control
  // without it we can't calculate  the size
  nsCOMPtr<nsIFontMetrics> fontMet;
  nsresult res = nsFormControlHelper::GetFrameFontFM(aPresContext, aFrame, getter_AddRefs(fontMet));
  if (NS_SUCCEEDED(res) && fontMet) {
    aRendContext->SetFont(fontMet);

    // Calculate the min size of the text control as one char
    // save the current default col size
    nscoord tmpCol        = aSpec.mColDefaultSize;
    aSpec.mColDefaultSize = 1;
    charWidth = nsFormControlHelper::CalcNavQuirkSizing(aPresContext, 
                                                        aRendContext, fontMet, 
                                                        aFrame, aSpec, aDesiredSize);
    // set the default col size back
    aMinSize.width        = aDesiredSize.width;
    aMinSize.height       = aDesiredSize.height;
    aSpec.mColDefaultSize = tmpCol;

    // Figure out the number of columns
    // and set that as the default col size
    if (NS_CONTENT_ATTR_HAS_VALUE == colStatus) {  // col attr will provide width
      PRInt32 col = ((colAttr.GetUnit() == eHTMLUnit_Pixel) ? colAttr.GetPixelValue() : colAttr.GetIntValue());
      col = (col <= 0) ? 1 : col; // XXX why a default of 1 char, why hide it
      aSpec.mColDefaultSize = col;
    }
    charWidth = nsFormControlHelper::CalcNavQuirkSizing(aPresContext, 
                                                        aRendContext, fontMet, 
                                                        aFrame, aSpec, aDesiredSize);
    aDesiredSize.height = aDesiredSize.height * aSpec.mRowDefaultSize;
  } else {
    NS_ASSERTION(fontMet, "Couldn't get Font Metrics"); 
    aDesiredSize.width = 300;  // arbitrary values
    aDesiredSize.width = 1500;
  }

  aRowHeight      = aDesiredSize.height;
  aMinSize.height = aDesiredSize.height;

  PRInt32 numRows = (aRowHeight > 0) ? (aDesiredSize.height / aRowHeight) : 0;

  return numRows;
}

//------------------------------------------------------------------
NS_IMETHODIMP
nsGfxTextControlFrame2::ReflowNavQuirks(nsIPresContext*          aPresContext,
                                        nsSize&                  aDesiredSize,
                                        const nsHTMLReflowState& aReflowState,
                                        nsReflowStatus&          aStatus,
                                        nsMargin&                aBorder,
                                        nsMargin&                aPadding)
{
  PRInt32 ignore;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
    PRInt32 width = 0;
    if (NS_CONTENT_ATTR_HAS_VALUE != GetSizeFromContent(&width)) {
      width = GetDefaultColumnWidth();
    }
    nsInputDimensionSpec textSpec(nsnull, PR_FALSE, nsnull,
                                  nsnull, width, 
                                  PR_FALSE, nsnull, 1);
    CalculateSizeNavQuirks(aPresContext, aReflowState.rendContext, this,  
                           textSpec, aDesiredSize, mMinSize, ignore, aBorder, aPadding);
  } else {
    nsInputDimensionSpec areaSpec(nsHTMLAtoms::cols, PR_FALSE, nsnull, 
                                  nsnull, GetDefaultColumnWidth(), 
                                  PR_FALSE, nsHTMLAtoms::rows, 1);
    CalculateSizeNavQuirks(aPresContext, aReflowState.rendContext, this,  
                           areaSpec, aDesiredSize, mMinSize, ignore, aBorder, aPadding);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsGfxTextControlFrame2::CreateFrameFor(nsIPresContext*   aPresContext,
                               nsIContent *      aContent,
                               nsIFrame**        aFrame)
{
  aContent = nsnull;
  return NS_ERROR_FAILURE;
}

#define DIV_STRING "user-focus: none; overflow:auto; border: 0px !important; padding: 0px; margin:0px"
#define DIV_STRING_SINGLELINE "user-focus: none; white-space : nowrap; overflow:auto; border: 0px !important; padding: 0px; margin:0px"

NS_IMETHODIMP
nsGfxTextControlFrame2::CreateAnonymousContent(nsIPresContext* aPresContext,
                                           nsISupportsArray& aChildList)
{
  // Get the PresShell

  mState |= NS_FRAME_INDEPENDENT_SELECTION;

  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));

  if (NS_FAILED(rv))
    return rv;

  if (!shell)
    return NS_ERROR_FAILURE;

  // Get the DOM document

  nsCOMPtr<nsIDocument> doc;
  rv = shell->GetDocument(getter_AddRefs(doc));
  if (NS_FAILED(rv))
    return rv;
  if (!doc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domdoc = do_QueryInterface(doc, &rv);
  if (NS_FAILED(rv))
    return rv;
  if (!domdoc)
    return NS_ERROR_FAILURE;
  
  // Now create a DIV and add it to the anonymous content child list.
  nsCOMPtr<nsIElementFactory> elementFactory;
  rv = GetElementFactoryService(getter_AddRefs(elementFactory));
  if (NS_FAILED(rv) || ! elementFactory)
    return rv?rv:NS_ERROR_FAILURE;

  nsCOMPtr<nsINodeInfoManager> nodeInfoManager;
  rv = doc->GetNodeInfoManager(*getter_AddRefs(nodeInfoManager));

  if (NS_FAILED(rv))
    return rv;

  NS_ENSURE_TRUE(nodeInfoManager, NS_ERROR_FAILURE);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = nodeInfoManager->GetNodeInfo(nsHTMLAtoms::div, nsnull, kNameSpaceID_HTML, *getter_AddRefs(nodeInfo));

  if (NS_FAILED(rv))
    return rv;

  if (!nodeInfo)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> divContent;

  rv = elementFactory->CreateInstanceByTag(nodeInfo, getter_AddRefs(divContent));

  if (NS_FAILED(rv))
    return rv;

  if (!divContent)
    return NS_ERROR_FAILURE;


  // Set the neccessary style attributes on the text control.

  if (IsSingleLineTextControl())
    rv = divContent->SetAttribute(kNameSpaceID_None,nsHTMLAtoms::style, NS_ConvertToString(DIV_STRING_SINGLELINE), PR_FALSE);
  else
    rv = divContent->SetAttribute(kNameSpaceID_None,nsHTMLAtoms::style, NS_ConvertToString(DIV_STRING), PR_FALSE);

  if (NS_FAILED(rv))
    return rv;

  // rv = divContent->SetAttribute(kNameSpaceID_None,nsXULAtoms::debug, NS_ConvertToString("true"), PR_FALSE);
  rv = aChildList.AppendElement(divContent);

  if (NS_FAILED(rv))
    return rv;

  // Create an editor

  rv = nsComponentManager::CreateInstance(kHTMLEditorCID,
                                              nsnull,
                                              NS_GET_IID(nsIEditor), getter_AddRefs(mEditor));
  if (NS_FAILED(rv))
    return rv;
  if (!mEditor) 
    return NS_ERROR_OUT_OF_MEMORY;

  // Create selection

  nsCOMPtr<nsIFrameSelection> frameSel;
  rv = nsComponentManager::CreateInstance(kFrameSelectionCID, nsnull,
                                                 NS_GET_IID(nsIFrameSelection),
                                                 getter_AddRefs(frameSel));

  // Create a SelectionController

  mTextSelImpl = new nsTextInputSelectionImpl(frameSel,shell,divContent);
  if (!mTextSelImpl)
    return NS_ERROR_OUT_OF_MEMORY;
  mTextListener = new nsTextInputListener();
  if (!mTextListener)
    return NS_ERROR_OUT_OF_MEMORY;
  mTextListener->SetFrame(this);
  mSelCon =  do_QueryInterface((nsISupports *)(nsISelectionController *)mTextSelImpl);//this will addref it once
  if (!mSelCon)
    return NS_ERROR_NO_INTERFACE;
  mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);

  // Setup the editor flags

  PRUint32 editorFlags = 0;
  if (IsPlainTextControl())
    editorFlags |= nsIHTMLEditor::eEditorPlaintextMask;
  if (IsSingleLineTextControl())
    editorFlags |= nsIHTMLEditor::eEditorSingleLineMask;
  if (IsPasswordTextControl())
    editorFlags |= nsIHTMLEditor::eEditorPasswordMask;

  // Now initialize the editor.
  //
  // NOTE: Conversion of '\n' to <BR> happens inside the
  //       editor's Init() call.

  rv = mEditor->Init(domdoc, shell, divContent, mSelCon, editorFlags);

  if (NS_FAILED(rv))
    return rv;

  // Initialize the controller for the editor

  nsCOMPtr<nsIControllers> controllers;
  nsCOMPtr<nsIDOMNSHTMLInputElement> inputElement = do_QueryInterface(mContent);
  if (inputElement)
    rv = inputElement->GetControllers(getter_AddRefs(controllers));
  else
  {
    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> textAreaElement = do_QueryInterface(mContent);

    if (!textAreaElement)
      return NS_ERROR_FAILURE;

    rv = textAreaElement->GetControllers(getter_AddRefs(controllers));
  }

  if (NS_FAILED(rv))
    return rv;

  if (controllers)
  {
    PRUint32 numControllers;
    PRBool found = PR_FALSE;
    rv = controllers->GetControllerCount(&numControllers);
    for (PRUint32 i = 0; i < numControllers; i ++)
    {
      nsCOMPtr<nsIController> controller;
      rv = controllers->GetControllerAt(i, getter_AddRefs(controller));
      if (NS_SUCCEEDED(rv) && controller)
      {
        nsCOMPtr<nsIEditorController> editController = do_QueryInterface(controller);
        if (editController)
        {
          editController->SetCommandRefCon(mEditor);
          found = PR_TRUE;
        }
      }
    }
    if (!found)
      rv = NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIEditorMailSupport> mailEditor(do_QueryInterface(mEditor));
  if (mailEditor)
  {
    nsHTMLValue colAttr;
    nsresult    colStatus;
    nsHTMLValue rowAttr;
    nsresult    rowStatus;
    PRInt32 type;
    GetType(&type);
    nsInputDimensionSpec *spec;
    if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) {
      PRInt32 width = 0;
      if (NS_CONTENT_ATTR_HAS_VALUE != GetSizeFromContent(&width)) {
        width = GetDefaultColumnWidth();
      }
      spec = new nsInputDimensionSpec(nsnull, PR_FALSE, nsnull,
                                    nsnull, width, 
                                    PR_FALSE, nsnull, 1);
    } else {
      spec = new nsInputDimensionSpec(nsHTMLAtoms::cols, PR_FALSE, nsnull, 
                                    nsnull, GetDefaultColumnWidth(), 
                                    PR_FALSE, nsHTMLAtoms::rows, 1);
    }
    if (spec)
    {
      if (NS_FAILED(GetColRowSizeAttr(this, 
                                              spec->mColSizeAttr, colAttr, colStatus,
                                              spec->mRowSizeAttr, rowAttr, rowStatus)))
        return NS_ERROR_FAILURE;
      PRInt32 col =-1;
      if (!(colAttr.GetUnit() == eHTMLUnit_Null))
      {
        col = ((colAttr.GetUnit() == eHTMLUnit_Pixel) ? colAttr.GetPixelValue() : colAttr.GetIntValue());
        col = (col <= 0) ? 1 : col; // XXX why a default of 1 char, why hide it
      }
      mailEditor->SetBodyWrapWidth(col);
      delete spec;
    }
  }

  // Set max text field length

  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor)
  {
    PRInt32 maxLength;
    rv = GetMaxLength(&maxLength);
    if (NS_CONTENT_ATTR_NOT_THERE != rv)
    { 
      htmlEditor->SetMaxTextLength(maxLength);
    }
  }
    
  // Get the caret and make it a selection listener.

  nsCOMPtr<nsIDOMSelection> domSelection;
  if (NS_SUCCEEDED(mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSelection))) && domSelection)
  {
    nsCOMPtr<nsICaret> caret;
    nsCOMPtr<nsIDOMSelectionListener> listener;
    if (NS_SUCCEEDED(shell->GetCaret(getter_AddRefs(caret))) && caret)
    {
      listener = do_QueryInterface(caret);
      if (listener)
      {
        domSelection->AddSelectionListener(listener);
      }
    }

    domSelection->AddSelectionListener(NS_STATIC_CAST(nsIDOMSelectionListener *, mTextListener));
  }
  
  // also set up the text listener as a transaction listener on the editor
  if (mEditor && mTextListener)
  {
    nsCOMPtr<nsITransactionManager> txMgr;
    rv = mEditor->GetTransactionManager(getter_AddRefs(txMgr));
    if (NS_FAILED(rv)) return rv;
    if (!txMgr) return NS_ERROR_NULL_POINTER;

    rv = txMgr->AddListener(NS_STATIC_CAST(nsITransactionListener*, mTextListener));
    if (NS_FAILED(rv)) return rv;
  }  
  
  // Get the default value for the textfield.

  nsAutoString defaultValue;

  if (mCachedState)
    defaultValue = mCachedState->GetUnicode();
  else
    rv = GetText(&defaultValue, PR_TRUE);

  if (NS_FAILED(rv))
    return rv;

  // If we have a default value, insert it under the div we created
  // above, but be sure to use the editor so that '*' characters get
  // displayed for password fields, etc. SetTextControlFrameState()
  // will call the editor for us.

  if (defaultValue.Length() > 0)
  {
    rv = mEditor->GetFlags(&editorFlags);

    if (NS_FAILED(rv))
      return rv;

    // Avoid causing reentrant painting and reflowing by telling the editor
    // that we don't want it to force immediate view refreshes or force
    // immediate reflows during any editor calls.

    rv = mEditor->SetFlags(editorFlags |
                           nsIHTMLEditor::eEditorDisableForcedUpdatesMask |
                           nsIHTMLEditor::eEditorDisableForcedReflowsMask);

    if (NS_FAILED(rv))
      return rv;

    // Now call SetTextControlFrameState() which will make the
    // neccessary editor calls to set the default value.
    // Make sure to turn off undo before setting the default
    // value, and turn it back on afterwards. This will make
    // sure we can't undo past the default value.

    rv = mEditor->EnableUndo(PR_FALSE);

    if (NS_FAILED(rv))
      return rv;

    SetTextControlFrameState(defaultValue);

    rv = mEditor->EnableUndo(PR_TRUE);

    if (NS_FAILED(rv))
      return rv;

    // Now restore the original editor flags.

    rv = mEditor->SetFlags(editorFlags);

    if (NS_FAILED(rv))
      return rv;
  }

  if (mContent)
  {
    rv = mEditor->GetFlags(&editorFlags);

    if (NS_FAILED(rv))
      return rv;

    PRInt32 nameSpaceID;

    rv = mContent->GetNameSpaceID(nameSpaceID);

    if (NS_FAILED(rv))
      return rv;

    nsAutoString resultValue;

    // Check if the readonly attribute is set.

    rv = mContent->GetAttribute(nameSpaceID, nsHTMLAtoms::readonly, resultValue);

    if (NS_FAILED(rv))
      return rv;

    if (NS_CONTENT_ATTR_NOT_THERE != rv)
      editorFlags |= nsIHTMLEditor::eEditorReadonlyMask;

    // Check if the disabled attribute is set.

    rv = mContent->GetAttribute(nameSpaceID, nsHTMLAtoms::disabled, resultValue);

    if (NS_FAILED(rv))
      return rv;

    if (NS_CONTENT_ATTR_NOT_THERE != rv) 
      editorFlags |= nsIHTMLEditor::eEditorDisabledMask;

    // Disable the caret and selection if neccessary.

    if (editorFlags & nsIHTMLEditor::eEditorReadonlyMask ||
        editorFlags & nsIHTMLEditor::eEditorDisabledMask)
    {
      if (mSelCon)
      {
        mSelCon->SetCaretEnabled(PR_FALSE);

        if (editorFlags & nsIHTMLEditor::eEditorDisabledMask)
          mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_OFF);
      }

      mEditor->SetFlags(editorFlags);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::SetDocumentForAnonymousContent(nsIDocument* aDocument,
                                                       PRBool aDeep,
                                                       PRBool aCompileEventHandlers)
{
  if (mEditor) {
    nsCOMPtr<nsIDOMElement> root;
    mEditor->GetRootElement(getter_AddRefs(root));
    if (root) {
      nsCOMPtr<nsIContent> rootContent( do_QueryInterface(root) );
      rootContent->SetDocument(aDocument, aDeep, aCompileEventHandlers);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::Reflow(nsIPresContext*   aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGfxTextControlFrame2", aReflowState.reason);

  // make sure the the form registers itself on the initial/first reflow
  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
    nsFormFrame::AddFormControlFrame(aPresContext, *NS_STATIC_CAST(nsIFrame*, this));
    mNotifyOnInput = PR_TRUE;//its ok to notify now. all has been prepared.
  }

  return nsStackFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}


NS_IMETHODIMP
nsGfxTextControlFrame2::GetPrefSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  if (!DoesNeedRecalc(mPrefSize)) {
     aSize = mPrefSize;
     return NS_OK;
  }

  PropagateDebug(aState);

  // navquirk can only happen if we are in the HTML namespace. It does not apply in XUL.
  PRInt32 nameSpaceID;
  mContent->GetNameSpaceID(nameSpaceID);
  
  aSize.width = 0;
  aSize.height = 0;

  PRBool collapsed = PR_FALSE;
  IsCollapsed(aState, collapsed);
  if (collapsed)
    return NS_OK;

  nsIPresContext* aPresContext = aState.GetPresContext();
  const nsHTMLReflowState* aReflowState = aState.GetReflowState();
  nsSize styleSize(CSS_NOTSET,CSS_NOTSET);
  nsFormControlFrame::GetStyleSize(aPresContext, *aReflowState, styleSize);


 if (mState & NS_FRAME_FIRST_REFLOW)
   mNotifyOnInput = PR_TRUE;//its ok to notify now. all has been prepared.

  if (!aReflowState)
    return NS_OK;

  nsCompatibility mode;
  aPresContext->GetCompatibilityMode(&mode); 
  PRBool navQuirksMode = eCompatibility_NavQuirks == mode && nameSpaceID == kNameSpaceID_HTML;

  nsReflowStatus aStatus;
  nsMargin border;
  border.SizeTo(0, 0, 0, 0);
  nsMargin padding;
  padding.SizeTo(0, 0, 0, 0);

  // Get the CSS border
  const nsStyleSpacing* spacing;
  GetStyleData(eStyleStruct_Spacing,  (const nsStyleStruct *&)spacing);
  spacing->CalcBorderFor(this, border);
  spacing->CalcPaddingFor(this, padding);

  nsresult rv;
  if (navQuirksMode) {
    rv = ReflowNavQuirks(aPresContext, aSize, *aReflowState, aStatus, border, padding);
  } else {
    rv = ReflowStandard(aPresContext, aSize, *aReflowState, aStatus, border, padding);
  }
  AddInset(aSize);

  mPrefSize = aSize;

#ifdef DEBUG_rods
  {
    nsMargin borderPadding(0,0,0,0);
    GetBorderAndPadding(borderPadding);
    nsSize size(169, 24);
    nsSize actual(aSize.width/15, 
                  aSize.height/15);
    printf("nsGfxText(field) %d,%d  %d,%d  %d,%d\n", 
           size.width, size.height, actual.width, actual.height, actual.width-size.width, actual.height-size.height);  // text field
  }
#endif

  return NS_OK;
}



NS_IMETHODIMP
nsGfxTextControlFrame2::GetMinSize(nsBoxLayoutState& aState, nsSize& aSize)
{
#define FIX_FOR_BUG_40596
#ifdef FIX_FOR_BUG_40596
  aSize = mMinSize;
  return NS_OK;
#else
  return nsBox::GetMinSize(aState, aSize);
#endif
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetMaxSize(nsBoxLayoutState& aState, nsSize& aSize)
{
  return nsBox::GetMaxSize(aState, aSize);
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetAscent(nsBoxLayoutState& aState, nscoord& aAscent)
{
  nsSize size;
  nsresult rv = GetPrefSize(aState, size);
  aAscent = size.height;
  return rv;
}

PRIntn
nsGfxTextControlFrame2::GetSkipSides() const
{
  return 0;
}

//IMPLEMENTING NS_IFORMCONTROLFRAME
NS_IMETHODIMP
nsGfxTextControlFrame2::GetName(nsString* aResult)
{
  nsresult rv = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* formControl = nsnull;
    rv = mContent->QueryInterface(NS_GET_IID(nsIHTMLContent),(void**)&formControl);
    if (NS_SUCCEEDED(rv) && formControl) {
      nsHTMLValue value;
      rv = formControl->GetHTMLAttribute(nsHTMLAtoms::name, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == rv) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(*aResult);
        }
      }
      NS_RELEASE(formControl);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetType(PRInt32* aType) const
{
  nsresult rv = NS_FORM_NOTOK;
  if (mContent) {
    nsIFormControl* formControl = nsnull;
    rv = mContent->QueryInterface(NS_GET_IID(nsIFormControl), (void**)&formControl);
    if ((NS_OK == rv) && formControl) {
      rv = formControl->GetType(aType);
      NS_RELEASE(formControl);
    }
  }
  return rv;
}

nsresult
nsGfxTextControlFrame2::GetSizeFromContent(PRInt32* aSize) const
{
  *aSize = -1;
  nsresult result = NS_CONTENT_ATTR_NOT_THERE;
  nsIHTMLContent* content = nsnull;
  mContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    result = content->GetHTMLAttribute(nsHTMLAtoms::size, value);
    if (eHTMLUnit_Integer == value.GetUnit()) { 
      *aSize = value.GetIntValue();
    }
    NS_RELEASE(content);
  }
  if (*aSize < 1) {
    // This is part of bug 46224
    // when we can get a PresContent (may be cache it) 
    // then we can check the compatibility mode
#ifdef FUTURE_ADDITIONAL_FIX_FOR_46224
    nsCompatibility mode;
    mPresContext->GetCompatibilityMode(&mode); 
    if (eCompatibility_NavQuirks == mode) {
      *aSize = 1;
    } else {
      *aSize = 20;
    }
#else
      *aSize = 1;
#endif
  }
  return result;
}

void    nsGfxTextControlFrame2::SetFocus(PRBool aOn , PRBool aRepaint){}
void    nsGfxTextControlFrame2::ScrollIntoView(nsIPresContext* aPresContext){}
void    nsGfxTextControlFrame2::MouseClicked(nsIPresContext* aPresContext){}

void    nsGfxTextControlFrame2::Reset(nsIPresContext* aPresContext)
{
  nsAutoString temp;
  GetText(&temp, PR_TRUE);
  SetTextControlFrameState(temp);
}

PRInt32 nsGfxTextControlFrame2::GetMaxNumValues(){return 1;}/**/

PRBool  nsGfxTextControlFrame2::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames)
{
  if (!aValues || !aNames) { return PR_FALSE; }

  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_NOT_THERE == result)) {
    return PR_FALSE;
  }

  aNames[0] = name;  
  aNumValues = 1;

  GetText(&(aValues[0]), PR_FALSE);
  // XXX: error checking
  return PR_TRUE;
}

nscoord 
nsGfxTextControlFrame2::GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                             float aPixToTwip, 
                                             nscoord aInnerHeight) const
{
   return NSIntPixelsToTwips(0, aPixToTwip); 
}


//---------------------------------------------------------
nscoord 
nsGfxTextControlFrame2::GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
  return GetVerticalInsidePadding(aPresContext, aPixToTwip, aInnerWidth);
}


void 
nsGfxTextControlFrame2::SetFormFrame(nsFormFrame* aFormFrame) 
{ 
  mFormFrame = aFormFrame; 
}


//---------------------------------------------------------
PRBool
nsGfxTextControlFrame2::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  nsAutoString name;
  PRBool disabled = PR_FALSE;
  nsFormControlHelper::GetDisabled(mContent, &disabled);
  return !disabled && (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
}

NS_IMETHODIMP 
nsGfxTextControlFrame2::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
  mSuggestedWidth = aWidth;
  mSuggestedHeight = aHeight;
  return NS_OK;
}

nsresult 
nsGfxTextControlFrame2::RequiresWidget(PRBool& aRequiresWidget)
{
  aRequiresWidget = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetFont(nsIPresContext* aPresContext, 
                            const nsFont*&  aFont)
{
  return nsFormControlHelper::GetFont(this, aPresContext, mStyleContext, aFont);
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetFormContent(nsIContent*& aContent) const
{
  nsIContent* content;
  nsresult    rv;

  rv = GetContent(&content);
  aContent = content;
  return rv;
}

NS_IMETHODIMP nsGfxTextControlFrame2::SetProperty(nsIPresContext* aPresContext, nsIAtom* aName, const nsAReadableString& aValue)
{
  if (!mIsProcessing)//some kind of lock.
  {
    mIsProcessing = PR_TRUE;
    
    if (nsHTMLAtoms::value == aName) 
    {
      if (mEditor) {
        mEditor->EnableUndo(PR_FALSE);    // wipe out undo info
      }
      SetTextControlFrameState(aValue);   // set new text value
      if (mEditor)  {
        mEditor->EnableUndo(PR_TRUE);     // fire up a new txn stack
      }
    }
    else if (nsHTMLAtoms::select == aName && mSelCon)
    {
      // select all the text
      mSelCon->SelectAll();
    }
    mIsProcessing = PR_FALSE;
  }
  return NS_OK;
}      

NS_IMETHODIMP nsGfxTextControlFrame2::GetProperty(nsIAtom* aName, nsAWritableString& aValue)
{
  // Return the value of the property from the widget it is not null.
  // If widget is null, assume the widget is GFX-rendered and return a member variable instead.

  if (nsHTMLAtoms::value == aName) {
    GetTextControlFrameState(aValue);
  }
  return NS_OK;
}  



NS_IMETHODIMP
nsGfxTextControlFrame2::GetEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);
  *aEditor = mEditor;
  NS_IF_ADDREF(*aEditor);
  return NS_OK;
}



NS_IMETHODIMP
nsGfxTextControlFrame2::GetTextLength(PRInt32* aTextLength)
{
  NS_ENSURE_ARG_POINTER(aTextLength);
  nsString *str = GetCachedString();
  if (str)
  {
    *aTextLength = str->Length();
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsGfxTextControlFrame2::GetFirstTextNode(nsIDOMCharacterData* *aFirstTextNode)
{
  if (!mEditor)
    return NS_ERROR_NOT_INITIALIZED;
  nsCOMPtr<nsIDOMElement> rootElement;
  mEditor->GetRootElement(getter_AddRefs(rootElement));
  *aFirstTextNode = nsnull;
  
  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootElement);
  if (!rootNode) return NS_ERROR_FAILURE;
  
  // for a text widget, the text of the document is in a single
  // text node under the body. Let's make sure that's true.
  nsCOMPtr<nsIDOMNodeList> childNodesList;
  rootNode->GetChildNodes(getter_AddRefs(childNodesList));
  if (!childNodesList)
  {
    NS_WARNING("rootNode has no text node list");
    return NS_ERROR_FAILURE;
  }

  PRUint32 numChildNodes = 0;
  childNodesList->GetLength(&numChildNodes);

  nsCOMPtr<nsIDOMNode> firstChild;
  nsresult rv = rootNode->GetFirstChild(getter_AddRefs(firstChild));
  if (NS_FAILED(rv)) return rv;
  if (!firstChild) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCharacterData> charDataNode = do_QueryInterface(firstChild, &rv);
  if (NS_FAILED(rv)) return rv;
  
  NS_ADDREF(*aFirstTextNode = charDataNode);
  return NS_OK;
}


nsresult
nsGfxTextControlFrame2::SelectAllContents()
{
  nsresult rv;
  
  if (IsSingleLineTextControl())
  {
    rv = SetSelectionRange(0, eSelectToEnd);
  }
  else
  {
    // we have to select all
    if (!mEditor)
      return NS_ERROR_NOT_INITIALIZED;
    NS_ASSERTION(mEditor, "Should have an editor here");    
    rv = mEditor->SelectAll();
  }

  return rv;
}


nsresult
nsGfxTextControlFrame2::SetSelectionEndPoints(PRInt32 aSelStart, PRInt32 aSelEnd)
{
  NS_ASSERTION(IsSingleLineTextControl(), "Should only call this on a single line input");
  NS_ASSERTION(mEditor, "Should have an editor here");
  NS_ASSERTION(mTextSelImpl,"selection not found!");

  nsCOMPtr<nsIDOMCharacterData> firstTextNode;
  nsresult rv = GetFirstTextNode(getter_AddRefs(firstTextNode));
  if (NS_FAILED(rv) || !firstTextNode)
  {
    // probably an empty document. not an error
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMNode> firstNode = do_QueryInterface(firstTextNode, &rv);
  if (!firstNode) return rv;
  
  // constrain the selection to this node
  PRUint32 nodeLengthU;
  firstTextNode->GetLength(&nodeLengthU);
  PRInt32 nodeLength = (PRInt32)nodeLengthU;
    
  nsCOMPtr<nsIDOMSelection> selection;
  mTextSelImpl->GetSelection(nsISelectionController::SELECTION_NORMAL,getter_AddRefs(selection));  
  if (!selection) return NS_ERROR_FAILURE;

  // are we setting both start and end?
  if (aSelStart != eIgnoreSelect && aSelEnd != eIgnoreSelect)
  {
    if (aSelStart == eSelectToEnd || aSelStart > nodeLength)
      aSelStart = nodeLength;
    if (aSelStart < 0)
      aSelStart = 0;

    if (aSelEnd == eSelectToEnd || aSelEnd > nodeLength)
      aSelEnd = nodeLength;
    if (aSelEnd < 0)
      aSelEnd = 0;

    // remove existing ranges
    selection->ClearSelection();  

    nsCOMPtr<nsIDOMRange> selectionRange;
    NS_NewRange(getter_AddRefs(selectionRange));
    if (!selectionRange) return NS_ERROR_OUT_OF_MEMORY;
    
    selectionRange->SetStart(firstTextNode, aSelStart);
    selectionRange->SetEnd(firstTextNode, aSelEnd);
    
    selection->AddRange(selectionRange);
  }
  else    // we're setting either start or end but not both
  {
    // does a range exist?
    nsCOMPtr<nsIDOMRange> firstRange;
    selection->GetRangeAt(0, getter_AddRefs(firstRange));
    PRBool mustAdd = PR_FALSE;
    PRInt32 selStart = 0, selEnd = 0;

    if (firstRange)
    {
     firstRange->GetStartOffset(&selStart);
     firstRange->GetEndOffset(&selEnd);
    }
    else
    {
      // no range. Make a new one.
      NS_NewRange(getter_AddRefs(firstRange));
      if (!firstRange) return NS_ERROR_OUT_OF_MEMORY;
      mustAdd = PR_TRUE;
    }
    
    if (aSelStart == eSelectToEnd)
      selStart = nodeLength;
    else if (aSelStart != eIgnoreSelect)
      selStart = aSelStart;

    if (aSelEnd == eSelectToEnd)
      selEnd = nodeLength;
    else if (aSelEnd != eIgnoreSelect)
      selEnd = aSelEnd;
    
    // swap them
    if (selEnd < selStart)
    {
      PRInt32 temp = selStart;
      selStart = selEnd;
      selEnd = temp;
    }
    
    firstRange->SetStart(firstTextNode, selStart);
    firstRange->SetEnd(firstTextNode, selEnd);
    if (mustAdd)  
      selection->AddRange(firstRange);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::SetSelectionRange(PRInt32 aSelStart, PRInt32 aSelEnd)
{
  if (!IsSingleLineTextControl()) return NS_ERROR_NOT_IMPLEMENTED;
  
  // make sure we have an editor
  if (!mEditor) 
    return NS_ERROR_NOT_INITIALIZED;
  
  return SetSelectionEndPoints(aSelStart, aSelEnd);
}


NS_IMETHODIMP
nsGfxTextControlFrame2::SetSelectionStart(PRInt32 aSelectionStart)
{
  if (!IsSingleLineTextControl()) return NS_ERROR_NOT_IMPLEMENTED;

  // make sure we have an editor
  if (!mEditor) 
    return NS_ERROR_NOT_INITIALIZED;
  
  return SetSelectionEndPoints(aSelectionStart, eIgnoreSelect);
}

NS_IMETHODIMP
nsGfxTextControlFrame2::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  if (!IsSingleLineTextControl()) return NS_ERROR_NOT_IMPLEMENTED;

  // make sure we have an editor
  if (!mEditor) 
    return NS_ERROR_NOT_INITIALIZED;
  
  return SetSelectionEndPoints(eIgnoreSelect, aSelectionEnd);
}


NS_IMETHODIMP
nsGfxTextControlFrame2::GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd)
{
  if (!IsSingleLineTextControl()) return NS_ERROR_NOT_IMPLEMENTED;

  NS_ENSURE_ARG_POINTER((aSelectionStart && aSelectionEnd));

  // make sure we have an editor
  if (!mEditor) 
    return NS_ERROR_NOT_INITIALIZED;
  
  nsCOMPtr<nsIDOMSelection> selection;
  mTextSelImpl->GetSelection(nsISelectionController::SELECTION_NORMAL,getter_AddRefs(selection));  
  if (!selection) return NS_ERROR_FAILURE;

  // we should have only zero or one range
  PRInt32 numRanges = 0;
  selection->GetRangeCount(&numRanges);
  if (numRanges > 1)
  {
    NS_ASSERTION(0, "Found more than on range in GetSelectionRange");
  }
  
  if (numRanges == 0)
  {
    *aSelectionStart = 0;
    *aSelectionEnd = 0;
  }
  else
  {
    nsCOMPtr<nsIDOMRange> firstRange;
    selection->GetRangeAt(0, getter_AddRefs(firstRange));
    if (!firstRange) 
      return NS_ERROR_FAILURE;
    firstRange->GetStartOffset(aSelectionStart);
    firstRange->GetEndOffset(aSelectionEnd);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsGfxTextControlFrame2::GetSelectionContr(nsISelectionController **aSelCon)
{
  NS_ENSURE_ARG_POINTER(aSelCon);
  NS_IF_ADDREF(*aSelCon = mSelCon);
  return NS_OK;
}


/////END INTERFACE IMPLEMENTATIONS

////NSIFRAME
NS_IMETHODIMP
nsGfxTextControlFrame2::AttributeChanged(nsIPresContext* aPresContext,
                                        nsIContent*     aChild,
                                        PRInt32         aNameSpaceID,
                                        nsIAtom*        aAttribute,
                                        PRInt32         aHint)
{
  if (!mEditor || !mSelCon) {return NS_ERROR_NOT_INITIALIZED;}
  nsresult rv = NS_OK;

  if (nsHTMLAtoms::value == aAttribute) 
  {
    if (mEditor && mContent)
    {
      nsString value;
      mContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::value, value);
      mEditor->EnableUndo(PR_FALSE);      // wipe out undo info
      SetTextControlFrameState(value);    // set new text value
      mEditor->EnableUndo(PR_TRUE);       // fire up a new txn stack
    }
    if (aHint != NS_STYLE_HINT_REFLOW)
      nsFormFrame::StyleChangeReflow(aPresContext, this);
  } 
  else if (nsHTMLAtoms::maxlength == aAttribute) 
  {
    PRInt32 maxLength;
    nsresult rv = GetMaxLength(&maxLength);
    
    nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
    if (htmlEditor)
    {
      if (NS_CONTENT_ATTR_NOT_THERE != rv) 
      {  // set the maxLength attribute
          htmlEditor->SetMaxTextLength(maxLength);
        // if maxLength>docLength, we need to truncate the doc content
      }
      else { // unset the maxLength attribute
          htmlEditor->SetMaxTextLength(-1);
      }
    }
  } 
  else if (mEditor && nsHTMLAtoms::readonly == aAttribute) 
  {
    nsresult rv = DoesAttributeExist(nsHTMLAtoms::readonly);
    PRUint32 flags;
    mEditor->GetFlags(&flags);
    if (NS_CONTENT_ATTR_NOT_THERE != rv) 
    { // set readonly
      flags |= nsIHTMLEditor::eEditorReadonlyMask;
      if (mSelCon)
        mSelCon->SetCaretEnabled(PR_FALSE);
    }
    else 
    { // unset readonly
      flags &= ~(nsIHTMLEditor::eEditorReadonlyMask);
      if (mSelCon && !(flags & nsIHTMLEditor::eEditorDisabledMask))
        mSelCon->SetCaretEnabled(PR_TRUE);
    }    
    mEditor->SetFlags(flags);
  }
  else if (mEditor && nsHTMLAtoms::disabled == aAttribute) 
  {
    nsCOMPtr<nsIPresShell> shell;
    rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_FAILED(rv) || !shell)
      return rv?rv:NS_ERROR_FAILURE;

    rv = DoesAttributeExist(nsHTMLAtoms::disabled);
    PRUint32 flags;
    mEditor->GetFlags(&flags);
    if (NS_CONTENT_ATTR_NOT_THERE != rv) 
    { // set disabled
      flags |= nsIHTMLEditor::eEditorDisabledMask;
      if (mSelCon)
      {
        mSelCon->SetCaretEnabled(PR_FALSE);
        mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_OFF);
      }
    }
    else 
    { // unset disabled
      flags &= ~(nsIHTMLEditor::eEditorDisabledMask);
      if (mSelCon)
      {
        if (! (flags & nsIHTMLEditor::eEditorReadonlyMask))
          mSelCon->SetCaretEnabled(PR_TRUE);
        mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
      }
    }    
    mEditor->SetFlags(flags);
  }
  else if ((nsHTMLAtoms::size == aAttribute ||
            nsHTMLAtoms::rows == aAttribute) && aHint != NS_STYLE_HINT_REFLOW) {
    nsFormFrame::StyleChangeReflow(aPresContext, this);
  }
  // Allow the base class to handle common attributes supported
  // by all form elements... 
  else {
    rv = nsBoxFrame::AttributeChanged(aPresContext, aChild, aNameSpaceID, aAttribute, aHint);
  }

  return rv;
}


NS_IMETHODIMP
nsGfxTextControlFrame2::GetText(nsString* aText, PRBool aInitialValue)
{
  nsresult rv = NS_CONTENT_ATTR_NOT_THERE;
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) 
  {
    if (PR_TRUE==aInitialValue)
    {
      rv = nsFormControlHelper::GetInputElementValue(mContent, aText, aInitialValue);
    }
    else
    {
      GetTextControlFrameState(*aText);
    }
    RemoveNewlines(*aText);
  } 
  else 
  {
    nsIDOMHTMLTextAreaElement* textArea = nsnull;
    rv = mContent->QueryInterface(NS_GET_IID(nsIDOMHTMLTextAreaElement), (void**)&textArea);
    if ((NS_OK == rv) && textArea) {
      if (PR_TRUE == aInitialValue) {
        rv = textArea->GetDefaultValue(*aText);
      }
      else {
        if(mEditor) {
          nsCOMPtr<nsIEditorIMESupport> imeSupport = do_QueryInterface(mEditor);
          if(imeSupport) 
            imeSupport->ForceCompositionEnd();
        }
        rv = textArea->GetValue(*aText);
      }
      NS_RELEASE(textArea);
    }
  }
  return rv;
}



///END NSIFRAME OVERLOADS
/////BEGIN PROTECTED METHODS

void nsGfxTextControlFrame2::RemoveNewlines(nsString &aString)
{
  // strip CR/LF and null
  static const char badChars[] = {10, 13, 0};
  aString.StripChars(badChars);
}


NS_IMETHODIMP
nsGfxTextControlFrame2::GetMaxLength(PRInt32* aSize)
{
  *aSize = -1;
  nsresult rv = NS_CONTENT_ATTR_NOT_THERE;
  nsIHTMLContent* content = nsnull;
  mContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) {
    nsHTMLValue value;
    rv = content->GetHTMLAttribute(nsHTMLAtoms::maxlength, value);
    if (eHTMLUnit_Integer == value.GetUnit()) { 
      *aSize = value.GetIntValue();
    }
    NS_RELEASE(content);
  }
  return rv;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::DoesAttributeExist(nsIAtom *aAtt)
{
  nsresult rv = NS_CONTENT_ATTR_NOT_THERE;
  nsIHTMLContent* content = nsnull;
  mContent->QueryInterface(kIHTMLContentIID, (void**) &content);
  if (nsnull != content) 
  {
    nsHTMLValue value;
    rv = content->GetHTMLAttribute(aAtt, value);
    NS_RELEASE(content);
  }
  return rv;
}

void
nsGfxTextControlFrame2::SubmitAttempt()
{
  // Submit the form
  if (mFormFrame && mTextSelImpl && mFormFrame->CanSubmit(this)) {
    nsIContent *formContent = nsnull;

    nsEventStatus status = nsEventStatus_eIgnore;

    nsWeakPtr &shell = mTextSelImpl->GetPresShell();
    nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(shell);
    if (!presShell) return;
    {
      nsCOMPtr<nsIPresContext> context;
      if (NS_SUCCEEDED(presShell->GetPresContext(getter_AddRefs(context))) && context)
      {
        mFormFrame->GetContent(&formContent);
        if (nsnull != formContent) {
          nsEvent event;
          event.eventStructType = NS_EVENT;
          event.message = NS_FORM_SUBMIT;

          formContent->HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
          NS_RELEASE(formContent);
        }

        if (nsEventStatus_eConsumeNoDefault != status) {
          mFormFrame->OnSubmit(context, this);
        }
      }
    }
  }
}

// this is where we propogate a content changed event
NS_IMETHODIMP
nsGfxTextControlFrame2::InternalContentChanged()
{
  NS_PRECONDITION(mContent, "illegal to call unless we map to a content node");

  if (!mContent) { return NS_ERROR_NULL_POINTER; }

  if (PR_FALSE==mNotifyOnInput) { 
    return NS_OK; // if notification is turned off, just return ok
  } 
  
  // Dispatch the change event
  nsEventStatus status = nsEventStatus_eIgnore;
  nsGUIEvent event;
  event.eventStructType = NS_GUI_EVENT;
  event.widget = nsnull;
  event.message = NS_FORM_INPUT;
  event.flags = NS_EVENT_FLAG_INIT;

  // Have the content handle the event, propogating it according to normal DOM rules.
  nsWeakPtr &shell = mTextSelImpl->GetPresShell();
  nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(shell);
  if (!presShell) 
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresContext> context;
  if (NS_SUCCEEDED(presShell->GetPresContext(getter_AddRefs(context))) && context)
    return mContent->HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsGfxTextControlFrame2::CallOnChange()
{
  // Dispatch th1e change event
  nsCOMPtr<nsIContent> content;
  if (NS_SUCCEEDED(GetFormContent(*getter_AddRefs(content))))
  {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsInputEvent event;
    event.eventStructType = NS_INPUT_EVENT;
    event.widget = nsnull;
    event.message = NS_FORM_CHANGE;
    event.flags = NS_EVENT_FLAG_INIT;
    event.isShift = PR_FALSE;
    event.isControl = PR_FALSE;
    event.isAlt = PR_FALSE;
    event.isMeta = PR_FALSE;

    // Have the content handle the event.
    nsWeakPtr &shell = mTextSelImpl->GetPresShell();
    nsCOMPtr<nsIPresShell> presShell = do_QueryReferent(shell);
    if (!presShell)
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsIPresContext> context;
    if (NS_SUCCEEDED(presShell->GetPresContext(getter_AddRefs(context))) && context)
      return mContent->HandleDOMEvent(context, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
  }
  return NS_OK;
}


//======
//privates

nsString *
nsGfxTextControlFrame2::GetCachedString()
{
  if (!mCachedState && mEditor)
  {
    mCachedState = new nsString;
    if (!mCachedState)
      return nsnull;
    GetTextControlFrameState(*mCachedState);  
  }
  return mCachedState;
}

void nsGfxTextControlFrame2::GetTextControlFrameState(nsAWritableString& aValue)
{
  aValue.SetLength(0);  // initialize out param
  
  if (mEditor) 
  {
    PRUint32 flags = nsIDocumentEncoder::OutputLFLineBreak;;

    if (PR_TRUE==IsPlainTextControl()) {
      flags |= nsIDocumentEncoder::OutputBodyOnly;   // OutputNoDoctype if head info needed
    }

    nsFormControlHelper::nsHTMLTextWrap wrapProp;
    nsresult rv = nsFormControlHelper::GetWrapPropertyEnum(mContent, wrapProp);
    flags |= nsIDocumentEncoder::OutputPreformatted;
    if (NS_CONTENT_ATTR_NOT_THERE != rv) 
    {
      if (wrapProp == nsFormControlHelper::eHTMLTextWrap_Hard)
      {
        flags |= nsIDocumentEncoder::OutputWrap;
      }
    }

    mEditor->OutputToString(aValue, NS_LITERAL_STRING("text/plain"), flags);
  }
}     


// END IMPLEMENTING NS_IFORMCONTROLFRAME

void
nsGfxTextControlFrame2::SetTextControlFrameState(const nsAReadableString& aValue)
{
  if (mEditor) 
  {
    nsAutoString currentValue;
    nsAutoString format; format.AssignWithConversion("text/plain");
    nsresult rv = mEditor->OutputToString(currentValue, format, 0);
    if (PR_TRUE==IsSingleLineTextControl()) {
      RemoveNewlines(currentValue); 
    }
    if (PR_FALSE==currentValue.Equals(aValue))  // this is necessary to avoid infinite recursion
    {
      // \r is an illegal character in the dom, but people use them,
      // so convert windows and mac platform linebreaks to \n:
      // Unfortunately aValue is declared const, so we have to copy
      // in order to do this substitution.
      currentValue.Assign(aValue);
      nsFormControlHelper::PlatformToDOMLineBreaks(currentValue);

      nsCOMPtr<nsIDOMDocument>domDoc;
      rv = mEditor->GetDocument(getter_AddRefs(domDoc));
			if (NS_FAILED(rv)) return;
			if (!domDoc) return;

      rv = mEditor->SelectAll();
      nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
			if (!htmlEditor) return;

			// get the flags, remove readonly and disabled, set the value, restore flags
			PRUint32 flags, savedFlags;
			mEditor->GetFlags(&savedFlags);
			flags = savedFlags;
			flags &= ~(nsIHTMLEditor::eEditorDisabledMask);
			flags &= ~(nsIHTMLEditor::eEditorReadonlyMask);
			mEditor->SetFlags(flags);
      mEditor->SelectAll();
      mEditor->DeleteSelection(nsIEditor::eNone);
      htmlEditor->InsertText(currentValue);
      mEditor->SetFlags(savedFlags);
    }
  }
  else
  {
    if (!mCachedState)
    {
      mCachedState = new nsString;
      if (!mCachedState)
        return;
    }
    mCachedState->Assign(aValue); //store value for later initialization;
  }
}


NS_IMETHODIMP
nsGfxTextControlFrame2::SetInitialChildList(nsIPresContext* aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  /*nsIFrame *list = aChildList;
  nsFrameState  frameState;
  while (list)
  {
    list->GetFrameState(&frameState);
    frameState |= NS_FRAME_INDEPENDENT_SELECTION;
    list->SetFrameState(frameState);
    list->GetNextSibling(&list);
  }
  */
  nsresult rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, aChildList);
  if (mEditor)
    mEditor->PostCreate();
  //look for scroll view below this frame go along first child list
  nsIFrame *first;
  FirstChild(aPresContext,nsnull, &first);

//we must turn off scrollbars for singleline text controls
  PRInt32 type;
  GetType(&type);
  if ((NS_FORM_INPUT_TEXT == type) || (NS_FORM_INPUT_PASSWORD == type)) 
  {
    nsIScrollableFrame *scrollableFrame = nsnull;
    if (first)
      first->QueryInterface(NS_GET_IID(nsIScrollableFrame), (void **) &scrollableFrame);
    if (scrollableFrame)
      scrollableFrame->SetScrollbarVisibility(aPresContext,PR_FALSE,PR_FALSE);
  }

  //register keylistener
  nsCOMPtr<nsIDOMEventReceiver> erP;
  if (NS_SUCCEEDED(mContent->QueryInterface(NS_GET_IID(nsIDOMEventReceiver), getter_AddRefs(erP))) && erP)
  {
    // register the event listeners with the DOM event reveiver
    rv = erP->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMKeyListener *,mTextListener), NS_GET_IID(nsIDOMKeyListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register key listener");
    rv = erP->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMFocusListener *,mTextListener), NS_GET_IID(nsIDOMFocusListener));
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to register focus listener");
    nsCOMPtr<nsIPresShell> shell;
    nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_FAILED(rv) || !shell)
      return rv?rv:NS_ERROR_FAILURE;
  }

  while(first)
  {
    nsIScrollableView *scrollView;
    nsIView *view;
    first->GetView(aPresContext,&view);
    if (view)
    {
      view->QueryInterface(NS_GET_IID(nsIScrollableView),(void **)&scrollView);
      if (scrollView)
      {
        mTextSelImpl->SetScrollableView(scrollView);
        break;
      }
    }
    first->FirstChild(aPresContext,nsnull, &first);
  }

  return rv;
}


PRInt32 
nsGfxTextControlFrame2::GetWidthInCharacters() const
{
  // see if there's a COL attribute, if so it wins
  nsCOMPtr<nsIHTMLContent> content;
  nsresult rv = mContent->QueryInterface(NS_GET_IID(nsIHTMLContent), getter_AddRefs(content));
  if (NS_SUCCEEDED(rv) && content)
  {
    nsHTMLValue resultValue;
    rv = content->GetHTMLAttribute(nsHTMLAtoms::cols, resultValue);
    if (NS_CONTENT_ATTR_NOT_THERE != rv) 
    {
      if (resultValue.GetUnit() == eHTMLUnit_Integer) 
      {
        return (resultValue.GetIntValue());
      }
    }
  }

  // otherwise, see if CSS has a width specified.  If so, work backwards to get the 
  // number of characters this width represents.
 
  
  // otherwise, the default is just returned.
  return DEFAULT_COLUMN_WIDTH;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::GetStateType(nsIPresContext* aPresContext, nsIStatefulFrame::StateType* aStateType)
{
  *aStateType = nsIStatefulFrame::eTextType;
  return NS_OK;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::SaveState(nsIPresContext* aPresContext, nsIPresState** aState)
{
  // Construct a pres state.
  NS_NewPresState(aState); // The addref happens here.
  
  nsString theString;
  nsresult res = GetProperty(nsHTMLAtoms::value, theString);
  if (NS_FAILED(res))
    return res;
    
  res = nsLinebreakConverter::ConvertStringLineBreaks(theString,
           nsLinebreakConverter::eLinebreakPlatform, nsLinebreakConverter::eLinebreakContent);
  NS_ASSERTION(NS_SUCCEEDED(res), "Converting linebreaks failed!");  
  
  (*aState)->SetStateProperty(NS_ConvertASCIItoUCS2("value"), theString);
  return res;
}

NS_IMETHODIMP
nsGfxTextControlFrame2::RestoreState(nsIPresContext* aPresContext, nsIPresState* aState)
{
  nsAutoString stateString;
  aState->GetStateProperty(NS_ConvertASCIItoUCS2("value"), stateString);
  nsresult res = SetProperty(aPresContext, nsHTMLAtoms::value, stateString);
  return res;
}

#endif
