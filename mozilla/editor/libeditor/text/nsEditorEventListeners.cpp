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
#include "nsISelection.h"
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
#include "nsIDOMNSUIEvent.h"
#include "nsIPref.h"
#include "nsILookAndFeel.h"
#include "nsIPresContext.h"
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
#include "nsIDOMNSRange.h"
#include "nslog.h"

#ifdef DEBUG_IME
NS_IMPL_LOG_ENABLED(nsEditorEventListenersLog)
#else
NS_IMPL_LOG(nsEditorEventListenersLog)
#endif
#define PRINTF NS_LOG_PRINTF(nsEditorEventListenersLog)
#define FLUSH  NS_LOG_FLUSH(nsEditorEventListenersLog)

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kCDataFlavorCID,          NS_DATAFLAVOR_CID);
static NS_DEFINE_CID(kContentIteratorCID,      NS_CONTENTITERATOR_CID);
static NS_DEFINE_CID(kLookAndFeelCID,          NS_LOOKANDFEEL_CID);
static NS_DEFINE_CID(kPrefServiceCID,          NS_PREF_CID);

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
  
  nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent = do_QueryInterface(aKeyEvent);
  if(nsUIEvent) 
  {
    PRBool defaultPrevented;
    nsUIEvent->GetPreventDefault(&defaultPrevented);
    if(defaultPrevented)
      return NS_OK;
  }

  // we should check a flag here to see if we should be using built-in key bindings
  // mEditor->GetFlags(&flags);
  // if (flags & ...)

  PRUint32 keyCode;
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
    PRBool isAnyModifierKeyButShift;
    nsresult rv;
    rv = keyEvent->GetAltKey(&isAnyModifierKeyButShift);
    if (NS_FAILED(rv)) return rv;
    
    if (!isAnyModifierKeyButShift)
    {
      rv = keyEvent->GetMetaKey(&isAnyModifierKeyButShift);
      if (NS_FAILED(rv)) return rv;
      
      if (!isAnyModifierKeyButShift)
      {
        rv = keyEvent->GetCtrlKey(&isAnyModifierKeyButShift);
        if (NS_FAILED(rv)) return rv;
      }
    }

    switch (keyCode)
    {
      // we should be handling DOM_VK_META here too but it doesn't exist at the moment
      case nsIDOMKeyEvent::DOM_VK_SHIFT:
      case nsIDOMKeyEvent::DOM_VK_CONTROL:
      case nsIDOMKeyEvent::DOM_VK_ALT:
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;

      case nsIDOMKeyEvent::DOM_VK_BACK_SPACE: 
        if (isAnyModifierKeyButShift)
          return NS_OK;

        mEditor->DeleteSelection(nsIEditor::ePrevious);
        ScrollSelectionIntoView(mEditor);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;
 
      case nsIDOMKeyEvent::DOM_VK_DELETE:
        if (isAnyModifierKeyButShift)
          return NS_OK;

        mEditor->DeleteSelection(nsIEditor::eNext);
        ScrollSelectionIntoView(mEditor);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK; 
        break;
 
      case nsIDOMKeyEvent::DOM_VK_TAB:
        if ((flags & nsIHTMLEditor::eEditorSingleLineMask))
          return NS_OK; // let it be used for focus switching

        // else we insert the tab straight through
        htmlEditor->EditorKeyPress(keyEvent);
        ScrollSelectionIntoView(mEditor);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK; 

      case nsIDOMKeyEvent::DOM_VK_RETURN:
      case nsIDOMKeyEvent::DOM_VK_ENTER:
        if (!(flags & nsIHTMLEditor::eEditorSingleLineMask))
        {
          //htmlEditor->InsertBreak();
          htmlEditor->EditorKeyPress(keyEvent);
          ScrollSelectionIntoView(mEditor);
          aKeyEvent->PreventDefault(); // consumed
        }
        return NS_OK;
    }
  }

  if (NS_SUCCEEDED(htmlEditor->EditorKeyPress(keyEvent)))
    ScrollSelectionIntoView(mEditor);

  return NS_OK; // we don't PreventDefault() here or keybindings like control-x won't work 
}


nsresult
nsTextEditorKeyListener::ProcessShortCutKeys(nsIDOMEvent* aKeyEvent, PRBool& aProcessed)
{
  aProcessed=PR_FALSE;
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
          return NS_ERROR_NULL_POINTER;
        nsCOMPtr<nsIDOMNode> parent;
        if (!NS_SUCCEEDED(nsuiEvent->GetRangeParent(getter_AddRefs(parent))))
          return NS_ERROR_NULL_POINTER;
        PRInt32 offset = 0;
        if (!NS_SUCCEEDED(nsuiEvent->GetRangeOffset(&offset)))
          return NS_ERROR_NULL_POINTER;

        nsCOMPtr<nsISelection> selection;
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

        // We processed the event, whether drop/paste succeeded or not
        return NS_OK;
      }
    }
   }
   return NS_OK;
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
  PRINTF("nsTextEditorTextListener::HandleEvent\n");
  return NS_OK;
}



nsresult
nsTextEditorTextListener::HandleText(nsIDOMEvent* aTextEvent)
{
  PRINTF("nsTextEditorTextListener::HandleText\n");
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
  return DragOver(aDragEvent);
}


nsresult
nsTextEditorDragListener::DragOver(nsIDOMEvent* aDragEvent)
{
  nsresult rv;
  NS_WITH_SERVICE ( nsIDragService, dragService, "@mozilla.org/widget/dragservice;1", &rv );
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
      if ( flavorSupported ) {
        dragSession->SetCanDrop(PR_TRUE);
        aDragEvent->PreventBubble();
      }
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
    nsresult rv;
    NS_WITH_SERVICE(nsIDragService, dragService, "@mozilla.org/widget/dragservice;1", &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDragSession> dragSession(do_QueryInterface(dragService));
    if (dragSession)
    {
       PRBool flavorSupported = PR_FALSE;
      dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
      if ( !flavorSupported ) 
        dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
      if (! flavorSupported ) 
        return NS_OK;     
    }

    nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aMouseEvent));
    if (!nsuiEvent) return NS_OK;

    //some day we want to use another way to stop this from bubbling.
    aMouseEvent->PreventBubble();
    aMouseEvent->PreventDefault();

    /* for bug 47399, when dropping a drag session, if you are over your original
       selection, nothing should happen. 
       cmanske: But do this only if drag source is not the same as target (current) document!
    */
    nsCOMPtr<nsISelection> selection;
    rv = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(rv) || !selection) 
      return rv?rv:NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIEditor> editor = do_QueryInterface(htmlEditor);
    nsCOMPtr<nsIDOMDocument> domdoc;
    rv = editor->GetDocument(getter_AddRefs(domdoc));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMDocument> sourceDoc;
    rv = dragSession->GetSourceDocument(getter_AddRefs(sourceDoc));
    if (NS_FAILED(rv)) return rv;
    if (domdoc == sourceDoc)
    {
      PRBool isCollapsed;
      rv = selection->GetIsCollapsed(&isCollapsed);
      if (NS_FAILED(rv)) return rv;
  
      // Don't bother if collapsed - can always drop
      if (!isCollapsed)
      {
        nsCOMPtr<nsIDOMNode> parent;
        rv = nsuiEvent->GetRangeParent(getter_AddRefs(parent));
        if (NS_FAILED(rv)) return rv;
        if (!parent) return NS_ERROR_FAILURE;

        PRInt32 offset = 0;
        rv = nsuiEvent->GetRangeOffset(&offset);
        if (NS_FAILED(rv)) return rv;

        PRInt32 rangeCount;
        rv = selection->GetRangeCount(&rangeCount);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < rangeCount; i++)
        {
          nsCOMPtr<nsIDOMRange> range;

          rv = selection->GetRangeAt(i, getter_AddRefs(range));
          if (NS_FAILED(rv) || !range) 
            continue;//dont bail yet, iterate through them all

          nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
          if (NS_FAILED(rv) || !nsrange) 
            continue;//dont bail yet, iterate through them all

          PRBool inrange;
          rv = nsrange->IsPointInRange(parent, offset, &inrange);
          if(inrange)
            return NS_ERROR_FAILURE;//okay, now you can bail, we are over the orginal selection
        }
      }
    }
    // if we are not over orginal selection, drop that baby!
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
  PRINTF("nsTextEditorCompositionListener::HandleEvent\n");
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
  PRINTF("nsTextEditorCompositionListener::HandleStartComposition\n");
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
  PRINTF("nsTextEditorCompositionListener::HandleQueryComposition\n");
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
  PRINTF("nsTextEditorCompositionListener::HandleEndComposition\n");
   return mEditor->EndComposition();
}


nsresult
nsTextEditorCompositionListener::HandleQueryReconversion(nsIDOMEvent* aReconversionEvent)
{
  PRINTF("nsTextEditorCompositionListener::HandleQueryReconversion\n");
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aReconversionEvent);
  nsReconversionEventReply* eventReply;

  if (!pCompositionEvent)
    return NS_ERROR_FAILURE;

  nsresult rv = pCompositionEvent->GetReconversionReply(&eventReply);
  if (NS_FAILED(rv))
    return rv;

  return mEditor->GetReconversionString(eventReply);
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
            PRInt32 pixelWidth;
            nsresult result;

            NS_WITH_SERVICE(nsILookAndFeel, look, kLookAndFeelCID, &result);

            if (NS_SUCCEEDED(result) && look)
            {
              if(flags & nsIHTMLEditor::eEditorSingleLineMask)
                look->GetMetric(nsILookAndFeel::eMetric_SingleLineCaretWidth, pixelWidth);
              else
                look->GetMetric(nsILookAndFeel::eMetric_MultiLineCaretWidth, pixelWidth);
            }

            selCon->SetCaretWidth(pixelWidth);
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
        if((flags & nsIHTMLEditor::eEditorWidgetMask)  ||
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

