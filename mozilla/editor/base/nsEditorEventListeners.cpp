/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsEditorEventListeners.h"
#include "nsEditor.h"
#include "nsVoidArray.h"
#include "nsString.h"

#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsIDOMSelection.h"
#include "nsIDOMCharacterData.h"
#include "nsIEditProperty.h"
#include "nsISupportsArray.h"
#include "nsIStringStream.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsIEditorMailSupport.h"
#include "nsIDocumentEncoder.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIPref.h"

// for repainting hack only
#include "nsIView.h"
#include "nsIViewManager.h"
// end repainting hack only

// Drag & Drop, Clipboard
#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsITransferable.h"
#include "nsIFormatConverter.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsISupportsPrimitives.h"
#include "nsLayoutCID.h"

// Drag & Drop, Clipboard Support
static NS_DEFINE_IID(kCDataFlavorCID,          NS_DATAFLAVOR_CID);
static NS_DEFINE_IID(kContentIteratorCID,      NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCXIFConverterCID,        NS_XIFFORMATCONVERTER_CID);

static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);

//#define DEBUG_IME

static nsresult ScrollSelectionIntoView(nsIEditor *aEditor);

/*
 * nsTextEditorKeyListener implementation
 */

NS_IMPL_ADDREF(nsTextEditorKeyListener)

NS_IMPL_RELEASE(nsTextEditorKeyListener)


nsTextEditorKeyListener::nsTextEditorKeyListener()
{
  NS_INIT_REFCNT();
}



nsTextEditorKeyListener::~nsTextEditorKeyListener() 
{
}



nsresult
nsTextEditorKeyListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMEventListener))) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMKeyListener))) {
    *aInstancePtr = (void*)(nsIDOMKeyListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult
nsTextEditorKeyListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

// individual key handlers return NS_OK to indicate NOT consumed
// by default, an error is returned indicating event is consumed
// joki is fixing this interface.
nsresult
nsTextEditorKeyListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorKeyListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
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

  PRBool keyProcessed;
  // we should check a flag here to see if we should be using built-in key bindings
  // mEditor->GetFlags(&flags);
  // if (flags & ...)
  ProcessShortCutKeys(aKeyEvent, keyProcessed);
  if (PR_FALSE==keyProcessed)
  {
    PRUint32     keyCode;
    PRUint32 flags;
    keyEvent->GetKeyCode(&keyCode);

    // if we are readonly or disabled, then do nothing.
    if (NS_SUCCEEDED(mEditor->GetFlags(&flags)))
    {
      if (flags & nsIHTMLEditor::eEditorReadonlyMask || 
          flags & nsIHTMLEditor::eEditorDisabledMask) 
        return NS_OK;
    }
    else
      return NS_ERROR_FAILURE;  // Editor unable to handle this.


    nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
    if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

    // if there is no charCode, then it's a key that doesn't map to a character,
    // so look for special keys using keyCode
    if (0 != keyCode)
    {
      if (nsIDOMKeyEvent::DOM_VK_SHIFT==keyCode
          || nsIDOMKeyEvent::DOM_VK_CONTROL==keyCode
          || nsIDOMKeyEvent::DOM_VK_ALT==keyCode)
        return NS_ERROR_BASE; // consumed
      if (nsIDOMKeyEvent::DOM_VK_BACK_SPACE==keyCode) 
      {
        mEditor->DeleteSelection(nsIEditor::ePrevious);
        ScrollSelectionIntoView(mEditor);
        return NS_ERROR_BASE; // consumed
      }   
      if (nsIDOMKeyEvent::DOM_VK_DELETE==keyCode)
      {
        mEditor->DeleteSelection(nsIEditor::eNext);
        ScrollSelectionIntoView(mEditor);
        return NS_ERROR_BASE; // consumed
      }   
      if (nsIDOMKeyEvent::DOM_VK_TAB==keyCode)
      {
        if ((flags & nsIHTMLEditor::eEditorSingleLineMask))
          return NS_OK; // let it be used for focus switching

        // else we insert the tab straight through
        htmlEditor->EditorKeyPress(keyEvent);
        ScrollSelectionIntoView(mEditor);
        return NS_ERROR_BASE; // "I handled the event, don't do default processing"
      }
      if (nsIDOMKeyEvent::DOM_VK_RETURN==keyCode
          || nsIDOMKeyEvent::DOM_VK_ENTER==keyCode)
      {
        if (!(flags & nsIHTMLEditor::eEditorSingleLineMask))
        {
          //htmlEditor->InsertBreak();
          htmlEditor->EditorKeyPress(keyEvent);
          ScrollSelectionIntoView(mEditor);
          return NS_ERROR_BASE; // consumed
        }
        else 
        {
          return NS_OK;
        }
      }
    }

    if (NS_SUCCEEDED(htmlEditor->EditorKeyPress(keyEvent)))
      ScrollSelectionIntoView(mEditor);
  }
  else
    ScrollSelectionIntoView(mEditor);

  return NS_ERROR_BASE; // consumed
  
}


nsresult
nsTextEditorKeyListener::ProcessShortCutKeys(nsIDOMEvent* aKeyEvent, PRBool& aProcessed)
{
  aProcessed=PR_FALSE;

#ifdef USE_OLD_OBSOLETE_HARDWIRED_SHORTCUT_KEYS
  PRUint32 charCode;

  nsCOMPtr<nsIDOMKeyEvent>keyEvent;
  keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) {
    //non-key event passed in.  bad things.
    return NS_OK;
  }

  if (NS_SUCCEEDED(keyEvent->GetCharCode(&charCode)))
  {
    // Figure out the modifier key, different on every platform
    PRBool isOnlyPlatformModifierKey = PR_FALSE;;
    PRBool isCtrlKey, isAltKey, isMetaKey, isShiftKey;
    if (NS_SUCCEEDED(keyEvent->GetAltKey(&isAltKey)) &&
        NS_SUCCEEDED(keyEvent->GetMetaKey(&isMetaKey)) &&
        NS_SUCCEEDED(keyEvent->GetShiftKey(&isShiftKey)) &&
        NS_SUCCEEDED(keyEvent->GetCtrlKey(&isCtrlKey)) )
    {
#if defined(XP_MAC)
      isOnlyPlatformModifierKey = (isMetaKey &&
                                   !isShiftKey && !isCtrlKey && !isAltKey);
#elif defined(XP_UNIX)
      isOnlyPlatformModifierKey = (isAltKey &&
                                   !isShiftKey && !isCtrlKey && !isMetaKey);
#else /* Windows and default */
      isOnlyPlatformModifierKey = (isCtrlKey &&
                                   !isShiftKey && !isAltKey && !isMetaKey);
#endif
    }

    if (PR_TRUE == isOnlyPlatformModifierKey)
    {
      // swallow all keys with only the platform modifier
      // even if we don't process them
      aProcessed = PR_TRUE;

      // A minimal set of fallback mappings in case
      // XUL keybindings are unavailable:
      switch (charCode)
      {
        // XXX: hard-coded select all
        case (PRUint32)('a'):
          if (mEditor)
            mEditor->SelectAll();
          break;

        // XXX: hard-coded cut
        case (PRUint32)('x'):
          if (mEditor)
            mEditor->Cut();
          break;

        // XXX: hard-coded copy
        case (PRUint32)('c'):
          if (mEditor)
            mEditor->Copy();
          break;

        // XXX: hard-coded paste
        case (PRUint32)('v'):
          if (mEditor)
            mEditor->Paste(nsIClipboard::kGlobalClipboard);
          break;

        // XXX: hard-coded undo
        case (PRUint32)('z'):
          if (mEditor)
            mEditor->Undo(1);
          break;

        // XXX: hard-coded redo
        case (PRUint32)('y'):
          if (mEditor)
            mEditor->Redo(1);
          break;
      }
    }
  }
#endif
  return NS_OK;
}

nsresult
ScrollSelectionIntoView(nsIEditor *aEditor)
{
  if (! aEditor)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISelectionController> selCon;
  
  nsresult result = aEditor->GetSelectionController(getter_AddRefs(selCon));

  if (NS_FAILED(result) || ! selCon)
    return result ? result: NS_ERROR_FAILURE;

  return selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_FOCUS_REGION);
}

/*
 * nsTextEditorMouseListener implementation
 */

NS_IMPL_ADDREF(nsTextEditorMouseListener)

NS_IMPL_RELEASE(nsTextEditorMouseListener)


nsTextEditorMouseListener::nsTextEditorMouseListener() 
{
  NS_INIT_REFCNT();
}



nsTextEditorMouseListener::~nsTextEditorMouseListener() 
{
}



nsresult
nsTextEditorMouseListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMEventListener))) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMMouseListener))) {
    *aInstancePtr = (void*)(nsIDOMMouseListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult
nsTextEditorMouseListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



// XXX: evil global functions, move them to proper support module

nsresult
IsNodeInSelection(nsIDOMNode *aInNode, nsIDOMSelection *aInSelection, PRBool &aOutIsInSel)
{
   aOutIsInSel = PR_FALSE;   // init out-param
   if (!aInNode || !aInSelection) { return NS_ERROR_NULL_POINTER; }

  nsCOMPtr<nsIContentIterator>iter;
  nsresult result = nsComponentManager::CreateInstance(kContentIteratorCID, nsnull,
                                                       NS_GET_IID(nsIContentIterator), 
                                                       getter_AddRefs(iter));
   if (NS_FAILED(result)) { return result; }
   if (!iter) { return NS_ERROR_OUT_OF_MEMORY; }

   nsCOMPtr<nsIEnumerator> enumerator;
  result = aInSelection->GetEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(result)) { return result; }
   if (!enumerator) { return NS_ERROR_OUT_OF_MEMORY; }

  for (enumerator->First(); NS_OK!=enumerator->IsDone(); enumerator->Next())
  {
    nsCOMPtr<nsISupports> currentItem;
    result = enumerator->CurrentItem(getter_AddRefs(currentItem));
    if ((NS_SUCCEEDED(result)) && (currentItem))
    {
      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

         iter->Init(range);
         nsCOMPtr<nsIContent> currentContent;
         iter->CurrentNode(getter_AddRefs(currentContent));
         while (NS_ENUMERATOR_FALSE == iter->IsDone())
         {
            nsCOMPtr<nsIDOMNode>currentNode = do_QueryInterface(currentContent);
            if (currentNode.get()==aInNode)
            {
               // if it's a start or end node, need to test whether the (x,y) 
               // of the event falls within the selection

               // talk to Mike

               aOutIsInSel = PR_TRUE;
               return NS_OK;
            }
            /* do not check result here, and especially do not return the result code.
             * we rely on iter->IsDone to tell us when the iteration is complete
             */
            iter->Next();
            iter->CurrentNode(getter_AddRefs(currentContent));
         }
      }
   }
   return NS_OK;
}


nsresult
nsTextEditorMouseListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  if (!aMouseEvent)
    return NS_OK;   // NS_OK means "we didn't process the event".  Go figure.

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent) {
    //non-ui event passed in.  bad things.
    return NS_OK;
  }

  // If we got a mouse down inside the editing area, we should force the 
  // IME to commit before we change the cursor position
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if(imeEditor)
  	imeEditor->ForceCompositionEnd();

   nsCOMPtr<nsIEditor> editor (do_QueryInterface(mEditor));
  if (!editor) { return NS_OK; }

  PRUint16 button = 0;
  mouseEvent->GetButton(&button);
  // middle-mouse click (paste);
  if (button == 2)
  {
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefService, kPrefServiceCID, &rv);
    if (NS_SUCCEEDED(rv) && prefService)
    {
      PRBool doMiddleMousePaste = PR_FALSE;;
      rv = prefService->GetBoolPref("middlemouse.paste", &doMiddleMousePaste);
      if (NS_SUCCEEDED(rv) && doMiddleMousePaste)
      {
        // Set the selection to the point under the mouse cursor:
        nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aMouseEvent));

        if (!nsuiEvent)
          return NS_ERROR_BASE; // NS_ERROR_BASE means "We did process the event".
        nsCOMPtr<nsIDOMNode> parent;
        if (!NS_SUCCEEDED(nsuiEvent->GetRangeParent(getter_AddRefs(parent))))
          return NS_ERROR_BASE; // NS_ERROR_BASE means "We did process the event".
        PRInt32 offset = 0;
        if (!NS_SUCCEEDED(nsuiEvent->GetRangeOffset(&offset)))
          return NS_ERROR_BASE; // NS_ERROR_BASE means "We did process the event".

        nsCOMPtr<nsIDOMSelection> selection;
        if (NS_SUCCEEDED(editor->GetSelection(getter_AddRefs(selection))))
          (void)selection->Collapse(parent, offset);

        // If the ctrl key is pressed, we'll do paste as quotation.
        // Would've used the alt key, but the kde wmgr treats alt-middle specially. 
        nsCOMPtr<nsIEditorMailSupport> mailEditor;
        mouseEvent = do_QueryInterface(aMouseEvent);
        PRBool ctrlKey = PR_FALSE;
        mouseEvent->GetCtrlKey(&ctrlKey);
        if (ctrlKey)
          mailEditor = do_QueryInterface(mEditor);

        if (mailEditor)
          mailEditor->PasteAsQuotation(nsIClipboard::kSelectionClipboard);
        else
          editor->Paste(nsIClipboard::kSelectionClipboard);

     // Prevent the event from bubbling up to be possibly handled
        // again by the containing window:
        mouseEvent->PreventBubble();
        mouseEvent->PreventDefault();

        // We processed the event, whether paste succeeded or not:
        return NS_ERROR_BASE; // NS_ERROR_BASE means "We did process the event".
      }
    }
   }
   return NS_OK;   // did not process the event
}

nsresult
nsTextEditorMouseListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

nsresult
nsTextEditorMouseListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorMouseListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorMouseListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}


/*
 * nsTextEditorTextListener implementation
 */

NS_IMPL_ADDREF(nsTextEditorTextListener)

NS_IMPL_RELEASE(nsTextEditorTextListener)


nsTextEditorTextListener::nsTextEditorTextListener()
:   mCommitText(PR_FALSE),
   mInTransaction(PR_FALSE)
{
  NS_INIT_REFCNT();
}



nsTextEditorTextListener::~nsTextEditorTextListener() 
{
}

nsresult
nsTextEditorTextListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMEventListener))) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMTextListener))) {
    *aInstancePtr = (void*)(nsIDOMTextListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsTextEditorTextListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleEvent\n");
#endif
  return NS_OK;
}



nsresult
nsTextEditorTextListener::HandleText(nsIDOMEvent* aTextEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorTextListener::HandleText\n");
#endif
   nsAutoString            composedText;
   nsresult            result = NS_OK;
   nsCOMPtr<nsIPrivateTextEvent> textEvent;
   nsIPrivateTextRangeList      *textRangeList;
   nsTextEventReply         *textEventReply;

   textEvent = do_QueryInterface(aTextEvent);
   if (!textEvent) {
      //non-ui event passed in.  bad things.
      return NS_OK;
   }

   textEvent->GetText(composedText);
   textEvent->GetInputRange(&textRangeList);
   textEvent->GetEventReply(&textEventReply);
   textRangeList->AddRef();
   nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor, &result);
   if (imeEditor) {
    result = imeEditor->SetCompositionString(composedText,textRangeList,textEventReply);
    ScrollSelectionIntoView(mEditor);
   }
   return result;
}

/*
 * nsTextEditorDragListener implementation
 */

NS_IMPL_ADDREF(nsTextEditorDragListener)

NS_IMPL_RELEASE(nsTextEditorDragListener)


nsTextEditorDragListener::nsTextEditorDragListener() 
{
  NS_INIT_REFCNT();
}

nsTextEditorDragListener::~nsTextEditorDragListener() 
{
}

nsresult
nsTextEditorDragListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMEventListener))) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMDragListener))) {
    *aInstancePtr = (void*)(nsIDOMDragListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}



nsresult
nsTextEditorDragListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


nsresult
nsTextEditorDragListener::DragGesture(nsIDOMEvent* aDragEvent)
{
  PRBool canDrag = PR_FALSE;
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if ( !htmlEditor )
    return NS_ERROR_NULL_POINTER;
  
  // ...figure out if a drag should be started...
  nsresult rv = htmlEditor->CanDrag(aDragEvent, canDrag);
  if ( NS_SUCCEEDED(rv) && canDrag )
    rv = htmlEditor->DoDrag(aDragEvent);

  return rv;
}


nsresult
nsTextEditorDragListener::DragEnter(nsIDOMEvent* aDragEvent)
{
  nsresult rv;
  NS_WITH_SERVICE ( nsIDragService, dragService, "component://netscape/widget/dragservice", &rv );
  if ( NS_SUCCEEDED(rv) ) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
    if ( dragSession ) {
      PRUint32 flags;
      if (NS_SUCCEEDED(mEditor->GetFlags(&flags))) {
        if ((flags & nsIHTMLEditor::eEditorDisabledMask) || 
            (flags & nsIHTMLEditor::eEditorReadonlyMask)) {
          dragSession->SetCanDrop(PR_FALSE);
          return NS_OK;
        }
      }
      PRBool flavorSupported = PR_FALSE;
      dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
      if ( flavorSupported ) 
        dragSession->SetCanDrop(PR_TRUE);
    }
  }

  return NS_OK;
}


nsresult
nsTextEditorDragListener::DragOver(nsIDOMEvent* aDragEvent)
{
  nsresult rv;
  NS_WITH_SERVICE ( nsIDragService, dragService, "component://netscape/widget/dragservice", &rv );
  if ( NS_SUCCEEDED(rv) ) {
    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
    if ( dragSession ) {
      PRUint32 flags;
      if (NS_SUCCEEDED(mEditor->GetFlags(&flags))) {
        if ((flags & nsIHTMLEditor::eEditorDisabledMask) || 
            (flags & nsIHTMLEditor::eEditorReadonlyMask)) {
          dragSession->SetCanDrop(PR_FALSE);
          return NS_OK;
        }
      }
      PRBool flavorSupported = PR_FALSE;
      dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
      if ( flavorSupported ) 
        dragSession->SetCanDrop(PR_TRUE);
    } 
  }

  return NS_OK;
}


nsresult
nsTextEditorDragListener::DragExit(nsIDOMEvent* aDragEvent)
{
  return NS_OK;
}



nsresult
nsTextEditorDragListener::DragDrop(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if ( htmlEditor )
  {
    //some day we want to use another way to stop this from bubbling.
    aMouseEvent->PreventBubble();
    aMouseEvent->PreventDefault();

    return htmlEditor->InsertFromDrop(aMouseEvent);
  }

  return NS_OK;
}


nsTextEditorCompositionListener::nsTextEditorCompositionListener()
{
  NS_INIT_REFCNT();
}

nsTextEditorCompositionListener::~nsTextEditorCompositionListener() 
{
}


nsresult
nsTextEditorCompositionListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsISupports))) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMEventListener))) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIDOMCompositionListener))) {
    *aInstancePtr = (void*)(nsIDOMCompositionListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsTextEditorCompositionListener)

NS_IMPL_RELEASE(nsTextEditorCompositionListener)

nsresult
nsTextEditorCompositionListener::HandleEvent(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEvent\n");
#endif
  return NS_OK;
}

void nsTextEditorCompositionListener::SetEditor(nsIEditor *aEditor)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(aEditor);
  if (!imeEditor) return;      // should return an error here!
  
  // note that we don't hold an extra reference here.
  mEditor = imeEditor;
}

nsresult
nsTextEditorCompositionListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleStartComposition\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  nsTextEventReply* eventReply;

  if (!pCompositionEvent) return NS_ERROR_FAILURE;
  
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  return mEditor->BeginComposition(eventReply);
}
nsresult
nsTextEditorCompositionListener::HandleQueryComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleQueryComposition\n");
#endif
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  nsTextEventReply* eventReply;

  if (!pCompositionEvent) return NS_ERROR_FAILURE;
  
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  return mEditor->QueryComposition(eventReply);
}

nsresult
nsTextEditorCompositionListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
#ifdef DEBUG_IME
   printf("nsTextEditorCompositionListener::HandleEndComposition\n");
#endif
   return mEditor->EndComposition();
}



/*
 * Factory functions
 */



nsresult 
NS_NewEditorKeyListener(nsIDOMEventListener ** aInstancePtrResult, 
                        nsIEditor *aEditor)
{
  nsTextEditorKeyListener* it = new nsTextEditorKeyListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}



nsresult
NS_NewEditorMouseListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorMouseListener* it = new nsTextEditorMouseListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}


nsresult
NS_NewEditorTextListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorTextListener*   it = new nsTextEditorTextListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }

   it->SetEditor(aEditor);

   return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}



nsresult
NS_NewEditorDragListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorDragListener* it = new nsTextEditorDragListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetEditor(aEditor);

  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);   
}

nsresult
NS_NewEditorCompositionListener(nsIDOMEventListener** aInstancePtrResult, nsIEditor* aEditor)
{
   nsTextEditorCompositionListener*   it = new nsTextEditorCompositionListener();
   if (nsnull==it) {
      return NS_ERROR_OUT_OF_MEMORY;
   }
   it->SetEditor(aEditor);
  return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}

nsresult 
NS_NewEditorFocusListener(nsIDOMEventListener ** aInstancePtrResult, 
                          nsIEditor *aEditor)
{
  nsTextEditorFocusListener* it = new nsTextEditorFocusListener();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetEditor(aEditor);
   return it->QueryInterface(NS_GET_IID(nsIDOMEventListener), (void **) aInstancePtrResult);
}



/*
 * nsTextEditorFocusListener implementation
 */

NS_IMPL_ADDREF(nsTextEditorFocusListener)

NS_IMPL_RELEASE(nsTextEditorFocusListener)


nsTextEditorFocusListener::nsTextEditorFocusListener() 
{
  NS_INIT_REFCNT();
}

nsTextEditorFocusListener::~nsTextEditorFocusListener() 
{
}

nsresult
nsTextEditorFocusListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kIDOMFocusListenerIID, NS_IDOMFOCUSLISTENER_IID);
  static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMFocusListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMFocusListener*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult
nsTextEditorFocusListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsTextEditorFocusListener::Focus(nsIDOMEvent* aEvent)
{
  // turn on selection and caret
  if (mEditor)
  {
    PRUint32 flags;
    aEvent->PreventBubble();
    mEditor->GetFlags(&flags);
    if (! (flags & nsIHTMLEditor::eEditorDisabledMask))
    { // only enable caret and selection if the editor is not disabled
      nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);
      if (editor)
      {
        nsCOMPtr<nsISelectionController>selCon;
        editor->GetSelectionController(getter_AddRefs(selCon));
        if (selCon)
        {
          if (! (flags & nsIHTMLEditor::eEditorReadonlyMask))
          { // only enable caret if the editor is not readonly
            selCon->SetCaretEnabled(PR_TRUE);
          }

          selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
#ifdef USE_HACK_REPAINT
  // begin hack repaint
          nsCOMPtr<nsIViewManager> viewmgr;
          ps->GetViewManager(getter_AddRefs(viewmgr));
          if (viewmgr) {
            nsIView* view;
            viewmgr->GetRootView(view);         // views are not refCounted
            if (view) {
              viewmgr->UpdateView(view,NS_VMREFRESH_IMMEDIATE);
            }
          }
  // end hack repaint
#else
          selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
#endif
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsTextEditorFocusListener::Blur(nsIDOMEvent* aEvent)
{
  // turn off selection and caret
  if (mEditor)
  {
    PRUint32 flags;
    aEvent->PreventBubble();
    mEditor->GetFlags(&flags);
    nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);
    if (editor)
    {
      nsCOMPtr<nsISelectionController>selCon;
      editor->GetSelectionController(getter_AddRefs(selCon));
      if (selCon)
      {
        selCon->SetCaretEnabled(PR_FALSE);
        if((flags & nsIHTMLEditor::eEditorSingleLineMask) ||
          (flags & nsIHTMLEditor::eEditorPlaintextMask) ||
          (flags & nsIHTMLEditor::eEditorPasswordMask) ||
          (flags & nsIHTMLEditor::eEditorReadonlyMask) ||
          (flags & nsIHTMLEditor::eEditorDisabledMask) ||
          (flags & nsIHTMLEditor::eEditorFilterInputMask))
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);//hide but do NOT turn off
        }
        else
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
        }

#ifdef USE_HACK_REPAINT
// begin hack repaint
        nsCOMPtr<nsIViewManager> viewmgr;
        ps->GetViewManager(getter_AddRefs(viewmgr));
        if (viewmgr) 
        {
          nsIView* view;
          viewmgr->GetRootView(view);         // views are not refCounted
          if (view) {
            viewmgr->UpdateView(view,NS_VMREFRESH_IMMEDIATE);
          }
        }
// end hack repaint
#else
        selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
#endif
      }
    }
  }
  return NS_OK;
}

