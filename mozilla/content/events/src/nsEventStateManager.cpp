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
 */

#include "nsCOMPtr.h"
#include "nsIEventStateManager.h"
#include "nsEventStateManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIWidget.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsDOMEvent.h"
#include "nsHTMLAtoms.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsImageMapUtils.h"
#include "nsIHTMLDocument.h"
#include "nsINameSpaceManager.h"  // for kNameSpaceID_HTML
#include "nsIWebShell.h"
#include "nsIBaseWindow.h"
#include "nsIScrollableView.h"
#include "nsISelection.h"
#include "nsIFrameSelection.h"
#include "nsIDeviceContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIEnumerator.h"
#include "nsFrameTraversal.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"
#include "nsIContentViewer.h"

#include "nsIServiceManager.h"
#include "nsIPref.h"

#include "nsIChromeEventHandler.h"
#include "nsIFocusController.h"

#include "nsXULAtoms.h"
#include "nsIDOMXULDocument.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsITreeFrame.h"
#include "nsIOutlinerBoxObject.h"
#include "nsIScrollableViewProvider.h"
#include "nsIDOMDocumentRange.h"

#include "nsIDOMRange.h"
#include "nsICaret.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"
#include "nsIFormControl.h"

#include "nsIFrameTraversal.h"
#include "nsLayoutCID.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"

#ifdef DEBUG_rods
//#define DEBUG_DOCSHELL_FOCUS
#endif

#ifdef DEBUG_DOCSHELL_FOCUS
static char* gDocTypeNames[] = {"eChrome", "eGenericContent", "eFrameSet", "eFrame", "eIFrame"};
#endif

static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);


//we will use key binding by default now. this wil lbreak viewer for now
#define NON_KEYBINDING 0

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsIContent * gLastFocusedContent = 0; // Strong reference
nsIDocument * gLastFocusedDocument = 0; // Strong reference
nsIPresContext* gLastFocusedPresContext = 0; // Weak reference

PRUint32 nsEventStateManager::mInstanceCount = 0;

enum {
 MOUSE_SCROLL_N_LINES,
 MOUSE_SCROLL_PAGE,
 MOUSE_SCROLL_HISTORY,
 MOUSE_SCROLL_TEXTSIZE
};

nsEventStateManager::nsEventStateManager()
  : mGestureDownPoint(0,0),
    m_haveShutdown(PR_FALSE)
{
  mLastMouseOverFrame = nsnull;
  mLastDragOverFrame = nsnull;
  mCurrentTarget = nsnull;
  mCurrentTargetContent = nsnull;
  mCurrentRelatedContent = nsnull;
  mLastLeftMouseDownContent = nsnull;
  mLastMiddleMouseDownContent = nsnull;
  mLastRightMouseDownContent = nsnull;

  mConsumeFocusEvents = PR_FALSE;
  mLockCursor = 0;

  // init d&d gesture state machine variables
  mIsTrackingDragGesture = PR_FALSE;
  mGestureDownFrame = nsnull;

  mLClickCount = 0;
  mMClickCount = 0;
  mRClickCount = 0;
  mActiveContent = nsnull;
  mHoverContent = nsnull;
  mDragOverContent = nsnull;
  mCurrentFocus = nsnull;
  mDocument = nsnull;
  mPresContext = nsnull;
  mCurrentTabIndex = 0;
  mLastWindowToHaveFocus = nsnull;
  mFirstBlurEvent = nsnull;
  mFirstFocusEvent = nsnull;
  mAccessKeys = nsnull;
  mBrowseWithCaret = PR_FALSE;
  hHover = PR_FALSE;

  NS_INIT_REFCNT();

#ifdef CLICK_HOLD_CONTEXT_MENUS
  mEventDownWidget = nsnull;
#endif
  
  ++mInstanceCount;
}

NS_IMETHODIMP
nsEventStateManager::Init()
{
  nsresult rv;
  NS_WITH_SERVICE(nsIObserverService, observerService,
                  NS_OBSERVERSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
  {
    nsAutoString topic; topic.AssignWithConversion(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    observerService->AddObserver(this, topic.get());
  }

  rv = getPrefService();

  if (NS_SUCCEEDED(rv)) {
    mPrefService->GetBoolPref("nglayout.events.showHierarchicalHover", &hHover);
  }

  return rv;
}

nsEventStateManager::~nsEventStateManager()
{
#if CLICK_HOLD_CONTEXT_MENUS
  if ( mClickHoldTimer ) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nsnull;
  }
#endif

  NS_IF_RELEASE(mCurrentTargetContent);
  NS_IF_RELEASE(mCurrentRelatedContent);

  NS_IF_RELEASE(mLastLeftMouseDownContent);
  NS_IF_RELEASE(mLastMiddleMouseDownContent);
  NS_IF_RELEASE(mLastRightMouseDownContent);

  NS_IF_RELEASE(mActiveContent);
  NS_IF_RELEASE(mHoverContent);
  NS_IF_RELEASE(mDragOverContent);
  NS_IF_RELEASE(mCurrentFocus);

  NS_IF_RELEASE(mDocument);

  NS_IF_RELEASE(mLastWindowToHaveFocus);
  NS_IF_RELEASE(mFirstBlurEvent);
  NS_IF_RELEASE(mFirstFocusEvent);
  
  --mInstanceCount;
  if(mInstanceCount == 0) {
    NS_IF_RELEASE(gLastFocusedContent);
    NS_IF_RELEASE(gLastFocusedDocument);
  }

  if (mAccessKeys) {
    delete mAccessKeys;
  }

  if (!m_haveShutdown) {
    Shutdown();

    // Don't remove from Observer service in Shutdown because Shutdown also
    // gets called from xpcom shutdown observer.  And we don't want to remove
    // from the service in that case.

    nsresult rv;

    NS_WITH_SERVICE (nsIObserverService, observerService,
                     NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      {
        nsAutoString topic; topic.AssignWithConversion(NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        observerService->RemoveObserver(this, topic.get());
      }
  }
  
}

nsresult
nsEventStateManager::Shutdown()
{
  if (mPrefService) {
    mPrefService = null_nsCOMPtr();
  }

  m_haveShutdown = PR_TRUE;
  return NS_OK;
}

nsresult
nsEventStateManager::getPrefService()
{
  nsresult rv = NS_OK;

  if (!mPrefService) {
    mPrefService = do_GetService(kPrefCID, &rv);
  }

  if (NS_FAILED(rv)) return rv;

  if (!mPrefService) return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::Observe(nsISupports *aSubject, const PRUnichar *aTopic,
                             const PRUnichar *someData) {
  nsAutoString topicString(aTopic);
  nsAutoString shutdownString; shutdownString.AssignWithConversion(NS_XPCOM_SHUTDOWN_OBSERVER_ID);

  if (topicString == shutdownString)
    Shutdown();

  return NS_OK;
}


NS_IMPL_ISUPPORTS3(nsEventStateManager, nsIEventStateManager, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
nsEventStateManager::PreHandleEvent(nsIPresContext* aPresContext, 
                                 nsEvent *aEvent,
                                 nsIFrame* aTargetFrame,
                                 nsEventStatus* aStatus,
                                 nsIView* aView)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  NS_ENSURE_ARG(aPresContext);

  mCurrentTarget = aTargetFrame;
  NS_IF_RELEASE(mCurrentTargetContent);

  nsFrameState state;

  NS_ASSERTION(mCurrentTarget, "mCurrentTarget is null.  this should not happen.  see bug #13007");
  if (!mCurrentTarget) return NS_ERROR_NULL_POINTER;

  mCurrentTarget->GetFrameState(&state);
  state |= NS_FRAME_EXTERNAL_REFERENCE;
  mCurrentTarget->SetFrameState(state);

  *aStatus = nsEventStatus_eIgnore;

  
  NS_ASSERTION(aEvent, "aEvent is null.  this should never happen");
  if (!aEvent) return NS_ERROR_NULL_POINTER;

  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    BeginTrackingDragGesture ( aPresContext, (nsGUIEvent*)aEvent, aTargetFrame );
    mLClickCount = ((nsMouseEvent*)aEvent)->clickCount;
    SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
    break;
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    mMClickCount = ((nsMouseEvent*)aEvent)->clickCount;
    SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
    break;
  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    mRClickCount = ((nsMouseEvent*)aEvent)->clickCount;
    SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
#ifdef CLICK_HOLD_CONTEXT_MENUS
    KillClickHoldTimer();
#endif
    StopTrackingDragGesture();
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
    break;
  case NS_MOUSE_MOVE:
    // on the Mac, GenerateDragGesture() may not return until the drag has completed
    // and so |aTargetFrame| may have been deleted (moving a bookmark, for example).
    // If this is the case, however, we know that ClearFrameRefs() has been called
    // and it cleared out |mCurrentTarget|. As a result, we should pass |mCurrentTarget|
    // into UpdateCursor().
    GenerateDragGesture(aPresContext, (nsGUIEvent*)aEvent);
    UpdateCursor(aPresContext, aEvent, mCurrentTarget, aStatus);
    GenerateMouseEnterExit(aPresContext, (nsGUIEvent*)aEvent);
    // Flush reflows and invalidates to eliminate flicker when both a reflow
    // and visual change occur in an event callback. See bug  #36849
    FlushPendingEvents(aPresContext); 
    break;
  case NS_MOUSE_EXIT:
    GenerateMouseEnterExit(aPresContext, (nsGUIEvent*)aEvent);
    //This is a window level mouseenter event and should stop here
    aEvent->message = 0;
    break;
  case NS_DRAGDROP_OVER:
    GenerateDragDropEnterExit(aPresContext, (nsGUIEvent*)aEvent);
    break;
  case NS_GOTFOCUS:
    {
#ifdef DEBUG_hyatt
      printf("Got focus.\n");
#endif

      EnsureDocument(aPresContext);

      if (gLastFocusedDocument == mDocument)
        break;

      //fire focus
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent focusevent;
      focusevent.eventStructType = NS_EVENT;
      focusevent.message = NS_FOCUS_CONTENT;

      if (mDocument) {
      
        if(gLastFocusedDocument) {
          // fire a blur, on the document only to keep ender happy
          nsEventStatus blurstatus = nsEventStatus_eIgnore;
          nsEvent blurevent;
          blurevent.eventStructType = NS_EVENT;
          blurevent.message = NS_BLUR_CONTENT;
      
          if(gLastFocusedPresContext) {
            nsCOMPtr<nsIFocusController> focusController;

            nsCOMPtr<nsIScriptGlobalObject> ourGlobal;
            gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(ourGlobal));
            nsCOMPtr<nsIDOMWindowInternal> rootWindow;
            nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(ourGlobal);
            if(ourWindow) {
              ourWindow->GetRootFocusController(getter_AddRefs(focusController));
              if (focusController)
                focusController->SetSuppressFocus(PR_TRUE, "NS_GOTFOCUS ESM Suppression");
            }
            
            gLastFocusedDocument->HandleDOMEvent(gLastFocusedPresContext, &blurevent, nsnull, NS_EVENT_FLAG_INIT, &blurstatus);
            if (!mCurrentFocus && gLastFocusedContent) {
              // must send it to the element that is losing focus,
              // since SendFocusBlur wont be called
              gLastFocusedContent->HandleDOMEvent(gLastFocusedPresContext, &blurevent, nsnull, NS_EVENT_FLAG_INIT, &blurstatus);

              nsCOMPtr<nsIDocument> doc;
              gLastFocusedContent->GetDocument(*getter_AddRefs(doc));
              if (doc) {
                nsCOMPtr<nsIPresShell> shell;
                doc->GetShellAt(0, getter_AddRefs(shell));
                if (shell) {
                  nsCOMPtr<nsIPresContext> oldPresContext;
                  shell->GetPresContext(getter_AddRefs(oldPresContext));

                  nsEventStatus status = nsEventStatus_eIgnore;
                  nsEvent event;
                  event.eventStructType = NS_EVENT;
                  event.message = NS_BLUR_CONTENT;
                  nsCOMPtr<nsIEventStateManager> esm;
                  oldPresContext->GetEventStateManager(getter_AddRefs(esm));
                  esm->SetFocusedContent(gLastFocusedContent);
                  gLastFocusedContent->HandleDOMEvent(oldPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
                  esm->SetFocusedContent(nsnull);
                  NS_IF_RELEASE(gLastFocusedContent);
                }
              }
            }
            if (focusController) {
              focusController->SetSuppressFocus(PR_FALSE, "NS_GOTFOCUS ESM Suppression");
            }
          }
        }
        
        // fire focus on window, not document
        nsCOMPtr<nsIScriptGlobalObject> globalObject;
        mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
        if (globalObject) {
          nsIContent* currentFocus = mCurrentFocus;
          mCurrentFocus = nsnull; // keep the owning reference in currentFocus
          if(gLastFocusedDocument != mDocument) {
            mDocument->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status);
            if (currentFocus && currentFocus != gLastFocusedContent)
              currentFocus->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status);
          }
          
          globalObject->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status);
          NS_IF_RELEASE(mCurrentFocus);
          mCurrentFocus = currentFocus; // we kept this reference above
          NS_IF_RELEASE(gLastFocusedContent);
          gLastFocusedContent = mCurrentFocus;
          NS_IF_ADDREF(gLastFocusedContent);
        }

        
        NS_IF_RELEASE(gLastFocusedDocument);
        gLastFocusedDocument = mDocument;
        gLastFocusedPresContext = aPresContext;
        NS_IF_ADDREF(gLastFocusedDocument);
      }
    }

    // Set mBrowseWithCaret to value of preference if we're in HTML (don't use caret in XUL)
    if (mDocument) {
      mBrowseWithCaret = PR_FALSE;
      nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(mDocument));
      if (htmlDoc)
        mPrefService->GetBoolPref("accessibility.browsewithcaret", &mBrowseWithCaret);
    }

    break;

  case NS_LOSTFOCUS:
    {
      // Hold the blur, wait for the focus so we can query the style of the focus
      // target as to what to do with the event. If appropriate we fire the blur
      // at that time.
#ifdef XP_WIN
      if(! NS_STATIC_CAST(nsFocusEvent*, aEvent)->isMozWindowTakingFocus) {
        EnsureDocument(aPresContext);

        // We can get a deactivate on an Ender widget.  In this
        // case, we would like to obtain the DOM Window to start
        // with by looking at gLastFocusedContent.
        nsCOMPtr<nsIScriptGlobalObject> ourGlobal;
        if (gLastFocusedContent) {
          nsCOMPtr<nsIDocument> doc;
          gLastFocusedContent->GetDocument(*getter_AddRefs(doc));
		      if(doc)
            doc->GetScriptGlobalObject(getter_AddRefs(ourGlobal));
          else {
		        mDocument->GetScriptGlobalObject(getter_AddRefs(ourGlobal));
		        NS_RELEASE(gLastFocusedContent);
          }
        }
        else mDocument->GetScriptGlobalObject(getter_AddRefs(ourGlobal));

        // Now fire blurs.  We have to fire a blur on the focused window
        // and on the focused element if there is one.
        if (gLastFocusedDocument && gLastFocusedPresContext) {
          // Blur the element.
          if (gLastFocusedContent) {
            // Retrieve this content node's pres context. it can be out of sync in
            // the Ender widget case.
            nsCOMPtr<nsIDocument> doc;
            gLastFocusedContent->GetDocument(*getter_AddRefs(doc));
            if (doc) {
              nsCOMPtr<nsIPresShell> shell;
              doc->GetShellAt(0, getter_AddRefs(shell));
              if (shell) {
                nsCOMPtr<nsIPresContext> oldPresContext;
                shell->GetPresContext(getter_AddRefs(oldPresContext));

                nsEventStatus status = nsEventStatus_eIgnore;
                nsEvent event;
                event.eventStructType = NS_EVENT;
                event.message = NS_BLUR_CONTENT;
                nsCOMPtr<nsIEventStateManager> esm;
                oldPresContext->GetEventStateManager(getter_AddRefs(esm));
                esm->SetFocusedContent(gLastFocusedContent);
                gLastFocusedContent->HandleDOMEvent(oldPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
                esm->SetFocusedContent(nsnull);
                NS_IF_RELEASE(gLastFocusedContent);
              }
            }
          }

          nsEventStatus status = nsEventStatus_eIgnore;
          nsEvent event;
          event.eventStructType = NS_EVENT;
          event.message = NS_BLUR_CONTENT;
    
          // fire blur on document and window
          nsCOMPtr<nsIScriptGlobalObject> globalObject;
          if(gLastFocusedDocument) {
            gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
            gLastFocusedDocument->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
            if(globalObject)
              globalObject->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
          }
          // Now clear our our global variables
          mCurrentTarget = nsnull;
          NS_IF_RELEASE(gLastFocusedDocument);
          gLastFocusedPresContext = nsnull;
        }             
      } 
#endif
    }
    break;

 case NS_ACTIVATE:
    {
      // If we have a command dispatcher, and if it has a focused window and a
      // focused element in its focus memory, then restore the focus to those
      // objects.
      EnsureDocument(aPresContext);
#ifdef DEBUG_hyatt
      printf("ESM: GOT ACTIVATE.\n");
#endif
      nsCOMPtr<nsIFocusController> focusController;
      nsCOMPtr<nsIDOMElement> focusedElement;
      nsCOMPtr<nsIDOMWindowInternal> focusedWindow;
      nsCOMPtr<nsIDOMXULDocument> xulDoc = do_QueryInterface(mDocument);

      nsCOMPtr<nsIScriptGlobalObject> globalObj;
      mDocument->GetScriptGlobalObject(getter_AddRefs(globalObj));
      nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(globalObj));
      NS_ASSERTION(win, "win is null.  this happens [often on xlib builds].  see bug #79213");
      if (!win) return NS_ERROR_NULL_POINTER;
      win->GetRootFocusController(getter_AddRefs(focusController));

      if (focusController) {
        // Obtain focus info from the command dispatcher.
        focusController->GetFocusedWindow(getter_AddRefs(focusedWindow));
        focusController->GetFocusedElement(getter_AddRefs(focusedElement));

        focusController->SetSuppressFocusScroll(PR_TRUE);
        focusController->SetActive(PR_TRUE);
      }

	    if (!focusedWindow) {
        nsCOMPtr<nsIScriptGlobalObject> globalObject;
        mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
        focusedWindow = do_QueryInterface(globalObject);
      }

      // Focus the DOM window.
      NS_WARN_IF_FALSE(focusedWindow,"check why focusedWindow is null!!!");
      if(focusedWindow) {
        focusedWindow->Focus();

        if (focusedElement) {
          nsCOMPtr<nsIContent> focusContent = do_QueryInterface(focusedElement);
          nsCOMPtr<nsIDOMDocument> domDoc;
          nsCOMPtr<nsIDocument> document;
          focusedWindow->GetDocument(getter_AddRefs(domDoc));
          if (domDoc) {
            document = do_QueryInterface(domDoc);
            nsCOMPtr<nsIPresShell> shell;
            nsCOMPtr<nsIPresContext> context;
            document->GetShellAt(0, getter_AddRefs(shell));
            shell->GetPresContext(getter_AddRefs(context));
            focusContent->SetFocus(context);
          }
        }  
      }

      if (focusController) {
        PRBool isSuppressed;
        focusController->GetSuppressFocus(&isSuppressed);
        while(isSuppressed){
          focusController->SetSuppressFocus(PR_FALSE, "Activation Suppression"); // Unsuppress and let the command dispatcher listen again.
          focusController->GetSuppressFocus(&isSuppressed);
        }
        focusController->SetSuppressFocusScroll(PR_FALSE);
      }  
    }
  break;
    
 case NS_DEACTIVATE:
    {
      EnsureDocument(aPresContext);

      if (gLastFocusedDocument != mDocument)
        break;

      nsCOMPtr<nsIScriptGlobalObject> ourGlobal;
      mDocument->GetScriptGlobalObject(getter_AddRefs(ourGlobal));

      // Suppress the command dispatcher for the duration of the
      // de-activation.  This will cause it to remember the last
      // focused sub-window and sub-element for this top-level
      // window.
      nsCOMPtr<nsIFocusController> focusController;
      nsCOMPtr<nsIDOMWindowInternal> rootWindow;
      nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(ourGlobal);
      if(ourWindow) {
        ourWindow->GetRootFocusController(getter_AddRefs(focusController));
        if (focusController) {
          // Suppress the command dispatcher.
          focusController->SetSuppressFocus(PR_TRUE, "Deactivate Suppression");
        }
      }

      // Now fire blurs.  We have to fire a blur on the focused window
      // and on the focused element if there is one.
      if (gLastFocusedDocument && gLastFocusedPresContext) {
        if (gLastFocusedContent) {
          // Blur the element.
          nsCOMPtr<nsIPresShell> shell;
          gLastFocusedDocument->GetShellAt(0, getter_AddRefs(shell));
          if (shell) {
            nsCOMPtr<nsIPresContext> oldPresContext;
            shell->GetPresContext(getter_AddRefs(oldPresContext));
            
            nsEventStatus status = nsEventStatus_eIgnore;
            nsEvent event;
            event.eventStructType = NS_EVENT;
            event.message = NS_BLUR_CONTENT;
            nsCOMPtr<nsIEventStateManager> esm;
            oldPresContext->GetEventStateManager(getter_AddRefs(esm));
            esm->SetFocusedContent(gLastFocusedContent);
            gLastFocusedContent->HandleDOMEvent(oldPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
            esm->SetFocusedContent(nsnull);
            NS_IF_RELEASE(gLastFocusedContent);
          }
        }

        nsEventStatus status = nsEventStatus_eIgnore;
        nsEvent event;
        event.eventStructType = NS_EVENT;
        event.message = NS_BLUR_CONTENT;
    
        // fire blur on document and window
        mDocument->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
        if (ourGlobal)
          ourGlobal->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);

        // Now clear our our global variables
        mCurrentTarget = nsnull;
        NS_IF_RELEASE(gLastFocusedDocument);
        gLastFocusedPresContext = nsnull;
      }             

      if (focusController) {
        focusController->SetActive(PR_FALSE);
        focusController->SetSuppressFocus(PR_FALSE, "Deactivate Suppression");
      }
    } 
    
    break;
    
  case NS_KEY_PRESS:
    {

      nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
#ifdef XP_MAC
    // (pinkerton, joki, saari) IE5 for mac uses Control for access keys. The HTML4 spec
    // suggests to use command on mac, but this really sucks (imagine someone having a "q"
    // as an access key and not letting you quit the app!). As a result, we've made a 
    // command decision 1 day before tree lockdown to change it to the control key.
    PRBool isSpecialAccessKeyDown = keyEvent->isControl;
#else
    PRBool isSpecialAccessKeyDown = keyEvent->isAlt;
#endif
      //This is to prevent keyboard scrolling while alt modifier in use.
      if (isSpecialAccessKeyDown) {
        //Alt key is down, we may need to do an accesskey
        if (mAccessKeys) {
          //Someone registered an accesskey.  Find and activate it.
          nsAutoString accKey((char)keyEvent->charCode);
          accKey.ToLowerCase();

          nsVoidKey key((void*)accKey.First());
          if (mAccessKeys->Exists(&key)) {
            nsCOMPtr<nsIContent> content = getter_AddRefs(NS_STATIC_CAST(nsIContent*, mAccessKeys->Get(&key)));

            //Its hard to say what HTML4 wants us to do in all cases.  So for now we'll settle for
            //A) Set focus
            ChangeFocus(content, nsnull, PR_TRUE);

            nsresult rv = getPrefService();
            PRBool activate = PR_TRUE;

            if (NS_SUCCEEDED(rv)) {
              mPrefService->GetBoolPref("accessibility.accesskeycausesactivation", &activate);
            }

            if (activate) {
              //B) Click on it if the users prefs indicate to do so.
              nsEventStatus status = nsEventStatus_eIgnore;
              nsMouseEvent event;
              event.eventStructType = NS_MOUSE_EVENT;
              event.message = NS_MOUSE_LEFT_CLICK;
              event.isShift = PR_FALSE;
              event.isControl = PR_FALSE;
              event.isAlt = PR_FALSE;
              event.isMeta = PR_FALSE;
              event.clickCount = 0;
              event.widget = nsnull;
              content->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
            }

            *aStatus = nsEventStatus_eConsumeNoDefault;
          }
        }
      }
    }
  case NS_KEY_DOWN:
  case NS_KEY_UP:
  case NS_MOUSE_SCROLL:
    {
      if (mCurrentFocus) {
        mCurrentTargetContent = mCurrentFocus;
        NS_ADDREF(mCurrentTargetContent);
      }
    }
    break;
  }
  return NS_OK;
}


#ifdef CLICK_HOLD_CONTEXT_MENUS


//
// CreateClickHoldTimer
//
// Fire off a timer for determining if the user wants click-hold. This timer
// is a one-shot that will be cancelled when the user moves enough to fire
// a drag.
//
void
nsEventStateManager :: CreateClickHoldTimer ( nsIPresContext* inPresContext, nsGUIEvent* inMouseDownEvent )
{
  // just to be anal (er, safe)
  if ( mClickHoldTimer ) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nsnull;
  }
  
  // if content clicked on has a popup, don't even start the timer
  // since we'll end up conflicting and both will show.
  nsCOMPtr<nsIContent> clickedContent;
  if ( mGestureDownFrame ) {
    mGestureDownFrame->GetContent(getter_AddRefs(clickedContent));
    if ( clickedContent ) {
      // check for the |popup| attribute
      nsAutoString popup;
      clickedContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::popup, popup);
      if ( popup != NS_LITERAL_STRING("") )
        return;
      
      // check for a <menubutton> like bookmarks
      nsCOMPtr<nsIAtom> tag;
      clickedContent->GetTag ( *getter_AddRefs(tag) );
      if ( tag == nsXULAtoms::menubutton )
        return;
    }
  }

  mClickHoldTimer = do_CreateInstance("@mozilla.org/timer;1");
  if ( mClickHoldTimer )
    mClickHoldTimer->Init(sClickHoldCallback, this, kClickHoldDelay, NS_PRIORITY_HIGH);

  mEventPoint = inMouseDownEvent->point;
  mEventRefPoint = inMouseDownEvent->refPoint;
  mEventDownWidget = inMouseDownEvent->widget;
  
  mEventPresContext = inPresContext;
  
} // CreateClickHoldTimer


//
// KillClickHoldTimer
//
// Stop the timer that would show the context menu dead in its tracks
//
void
nsEventStateManager :: KillClickHoldTimer ( )
{
  if ( mClickHoldTimer ) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nsnull;
  }

  mEventDownWidget = nsnull;
  mEventPresContext = nsnull;

} // KillTooltipTimer


//
// sClickHoldCallback
//
// This fires after the mouse has been down for a certain length of time. 
//
void
nsEventStateManager :: sClickHoldCallback ( nsITimer *aTimer, void* aESM )
{
  nsEventStateManager* self = NS_STATIC_CAST(nsEventStateManager*, aESM);
  if ( self )
    self->FireContextClick();

  // NOTE: |aTimer| and |self->mAutoHideTimer| are invalid after calling ClosePopup();
  
} // sAutoHideCallback


//
// FireContextClick
//
// If we're this far, our timer has fired, which means the mouse has been down
// for a certain period of time and has not moved enough to generate a dragGesture.
// We can be certain the user wants a context-click at this stage, so generate
// a dom event and fire it in.
//
// After the event fires, check if PreventDefault() has been set on the event which
// means that someone either ate the event or put up a context menu. This is our cue
// to stop tracking the drag gesture. If we always did this, draggable items w/out
// a context menu wouldn't be draggable after a certain length of time, which is
// _not_ what we want.
//
void
nsEventStateManager :: FireContextClick ( )
{
  if ( !mEventDownWidget || !mEventPresContext )
    return;

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_MOUSE_EVENT;
  event.message = NS_CONTEXTMENU;
  event.widget = mEventDownWidget;
  event.clickCount = 1;
  event.point = mEventPoint;
  event.refPoint = mEventRefPoint;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;

  // Dispatch to the DOM. We have to fake out the ESM and tell it that the
  // current target frame is actually where the mouseDown occurred, otherwise it
  // will use the frame the mouse is currently over which may or may not be
  // the same. (Note: saari and I have decided that we don't have to reset |mCurrentTarget|
  // when we're through because no one else is doing anything more with this
  // event and it will get reset on the very next event to the correct frame).
  mCurrentTarget = mGestureDownFrame;
  nsCOMPtr<nsIContent> lastContent;
  if ( mGestureDownFrame ) {
    mGestureDownFrame->GetContent(getter_AddRefs(lastContent));
    
    if ( lastContent ) {
      // before dispatching, check that we're not on something that doesn't get a context menu
      PRBool allowedToDispatch = PR_TRUE;

      nsCOMPtr<nsIAtom> tag;
      lastContent->GetTag ( *getter_AddRefs(tag) );
      nsCOMPtr<nsIDOMHTMLInputElement> inputElm ( do_QueryInterface(lastContent) );
      nsCOMPtr<nsIFormControl> formControl ( do_QueryInterface(lastContent) );
      if ( inputElm ) {
        // of all input elements, only ones dealing with text are allowed to have context menus
        if ( tag == nsHTMLAtoms::input ) {
          nsAutoString type;
          lastContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::type, type);
          if ( type != NS_LITERAL_STRING("") && type != NS_LITERAL_STRING("text") &&
                type != NS_LITERAL_STRING("password") && type != NS_LITERAL_STRING("file") )
            allowedToDispatch = PR_FALSE;
        }
      }
      else if ( formControl )           // catches combo-boxes
        allowedToDispatch = PR_FALSE;
      else if ( tag == nsXULAtoms::scrollbar || tag == nsXULAtoms::scrollbarbutton || tag == nsXULAtoms::button )
        allowedToDispatch = PR_FALSE;
      else if ( tag == nsHTMLAtoms::applet || tag == nsHTMLAtoms::object || tag == nsHTMLAtoms::embed )
        allowedToDispatch = PR_FALSE;
    
      if ( allowedToDispatch ) {
        // stop selection tracking, we're in control now
        nsCOMPtr<nsIFrameSelection> frameSel;
        GetSelection ( mGestureDownFrame, mEventPresContext, getter_AddRefs(frameSel) );
        if ( frameSel ) {
          PRBool mouseDownState = PR_TRUE;
          frameSel->GetMouseDownState(&mouseDownState);
          if (mouseDownState)
            frameSel->SetMouseDownState(PR_FALSE);
        }
        
        // dispatch to DOM
        lastContent->HandleDOMEvent(mEventPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
        
        // dispatch to the frame
        mGestureDownFrame->HandleEvent(mEventPresContext, &event, &status);   
      }
    }
  }
  
  // now check if the event has been handled. If so, stop tracking a drag
  if ( status == nsEventStatus_eConsumeNoDefault )
    StopTrackingDragGesture();

  KillClickHoldTimer();
  
} // FireContextClick

#endif


// 
// BeginTrackingDragGesture
//
// Record that the mouse has gone down and that we should move to TRACKING state
// of d&d gesture tracker.
//
// We also use this to track click-hold context menus on mac. When the mouse goes down,
// fire off a short timer. If the timer goes off and we have yet to fire the
// drag gesture (ie, the mouse hasn't moved a certain distance), then we can
// assume the user wants a click-hold, so fire a context-click event. We only
// want to cancel the drag gesture if the context-click event is handled.
//
void
nsEventStateManager :: BeginTrackingDragGesture ( nsIPresContext* aPresContext, nsGUIEvent* inDownEvent, nsIFrame* inDownFrame )
{
  mIsTrackingDragGesture = PR_TRUE;
  mGestureDownPoint = inDownEvent->point;
  mGestureDownFrame = inDownFrame;
  
#ifdef CLICK_HOLD_CONTEXT_MENUS
  // fire off a timer to track click-hold
  CreateClickHoldTimer ( aPresContext, inDownEvent );
#endif

}


//
// StopTrackingDragGesture
//
// Record that the mouse has gone back up so that we should leave the TRACKING 
// state of d&d gesture tracker and return to the START state.
//
void
nsEventStateManager :: StopTrackingDragGesture ( )
{
  mIsTrackingDragGesture = PR_FALSE;
  mGestureDownPoint = nsPoint(0,0);
  mGestureDownFrame = nsnull;
}


//
// GetSelection
//
// Helper routine to get an nsIFrameSelection from the given frame
//
void
nsEventStateManager :: GetSelection ( nsIFrame* inFrame, nsIPresContext* inPresContext, nsIFrameSelection** outSelection )
{
  *outSelection = nsnull;
  
  if (inFrame) {
    nsCOMPtr<nsISelectionController> selCon;
    nsresult rv = inFrame->GetSelectionController(inPresContext, getter_AddRefs(selCon));

    if (NS_SUCCEEDED(rv) && selCon) {
      nsCOMPtr<nsIFrameSelection> frameSel;

      frameSel = do_QueryInterface(selCon);

      if (! frameSel) {
        nsCOMPtr<nsIPresShell> shell;
        rv = inPresContext->GetShell(getter_AddRefs(shell));

        if (NS_SUCCEEDED(rv) && shell)
          rv = shell->GetFrameSelection(getter_AddRefs(frameSel));
      }
      
      *outSelection = frameSel.get();
      NS_IF_ADDREF(*outSelection);
    }
  }

} // GetSelection


//
// GenerateDragGesture
//
// If we're in the TRACKING state of the d&d gesture tracker, check the current position
// of the mouse in relation to the old one. If we've moved a sufficient amount from
// the mouse down, then fire off a drag gesture event.
//
// Note that when the mouse enters a new child window with its own view, the event's
// coordinates will be in relation to the origin of the inner child window, which could
// either be very different from that of the mouse coords of the mouse down and trigger
// a drag too early, or very similiar which might not trigger a drag. 
//
// Do we need to do anything about this? Let's wait and see.
//
void
nsEventStateManager :: GenerateDragGesture ( nsIPresContext* aPresContext, nsGUIEvent *aEvent )
{
  NS_WARN_IF_FALSE(aPresContext, "This shouldn't happen.");
  if ( IsTrackingDragGesture() ) {

    // Check if selection is tracking drag gestures, if so
    // don't interfere!
    nsCOMPtr<nsIFrameSelection> frameSel;
    GetSelection ( mGestureDownFrame, aPresContext, getter_AddRefs(frameSel) );
    if ( frameSel ) {
      PRBool mouseDownState = PR_TRUE;
      frameSel->GetMouseDownState(&mouseDownState);
      if (mouseDownState) {
        StopTrackingDragGesture();
        return;
      }
    }

    // figure out the delta in twips, since that is how it is in the event.
    // Do we need to do this conversion every time? Will the pres context really change on
    // us or can we cache it?
    long twipDeltaToStartDrag = 0;
    const long pixelDeltaToStartDrag = 5;
    nsCOMPtr<nsIDeviceContext> devContext;
    aPresContext->GetDeviceContext ( getter_AddRefs(devContext) );
    if ( devContext ) {
      float pixelsToTwips = 0.0;
      devContext->GetDevUnitsToTwips(pixelsToTwips);
      twipDeltaToStartDrag =  (long)(pixelDeltaToStartDrag * pixelsToTwips);
    }
 
    // fire drag gesture if mouse has moved enough
    if ( abs(aEvent->point.x - mGestureDownPoint.x) > twipDeltaToStartDrag ||
          abs(aEvent->point.y - mGestureDownPoint.y) > twipDeltaToStartDrag ) {
#ifdef CLICK_HOLD_CONTEXT_MENUS
      // stop the click-hold before we fire off the drag gesture, in case
      // it takes a long time
      KillClickHoldTimer();
#endif

      nsEventStatus status = nsEventStatus_eIgnore;
      nsMouseEvent event;
      event.eventStructType = NS_DRAGDROP_EVENT;
      event.message = NS_DRAGDROP_GESTURE;
      event.widget = aEvent->widget;
      event.clickCount = 0;
      event.point = aEvent->point;
      event.refPoint = aEvent->refPoint;
      event.isShift = ((nsMouseEvent*)aEvent)->isShift;
      event.isControl = ((nsMouseEvent*)aEvent)->isControl;
      event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
      event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

      // Dispatch to the DOM. We have to fake out the ESM and tell it that the
      // current target frame is actually where the mouseDown occurred, otherwise it
      // will use the frame the mouse is currently over which may or may not be
      // the same. (Note: saari and I have decided that we don't have to reset |mCurrentTarget|
      // when we're through because no one else is doing anything more with this
      // event and it will get reset on the very next event to the correct frame).
      mCurrentTarget = mGestureDownFrame;
      nsCOMPtr<nsIContent> lastContent;
      if ( mGestureDownFrame ) {
        mGestureDownFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));
        if ( lastContent )
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
      }
      
      // dispatch to the frame
      if ( mGestureDownFrame ) 
        mGestureDownFrame->HandleEvent(aPresContext, &event, &status);   
      
      StopTrackingDragGesture();
    }
  }

  // Now flush all pending notifications.
  FlushPendingEvents(aPresContext);
} // GenerateDragGesture

nsresult
nsEventStateManager::ChangeTextSize(PRInt32 change)
{
  if(!gLastFocusedDocument) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIScriptGlobalObject> ourGlobal;
  gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(ourGlobal));
  if(!ourGlobal) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(ourGlobal);
  if(!ourWindow) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindowInternal> rootWindow;
  ourWindow->GetPrivateRoot(getter_AddRefs(rootWindow));
  if(!rootWindow) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMWindow> windowContent;
  rootWindow->GetContent(getter_AddRefs(windowContent));
  if(!windowContent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMDocument> domDoc;
  windowContent->GetDocument(getter_AddRefs(domDoc));
  if(!domDoc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  if(!doc) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell;
  doc->GetShellAt(0, getter_AddRefs(presShell));
  if(!presShell) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if(!presContext) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> pcContainer;
  presContext->GetContainer(getter_AddRefs(pcContainer));
  if(!pcContainer) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docshell(do_QueryInterface(pcContainer));
  if(!docshell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContentViewer> cv;
  docshell->GetContentViewer(getter_AddRefs(cv));
  if(!cv) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIMarkupDocumentViewer> mv(do_QueryInterface(cv));
  if(!mv) return NS_ERROR_FAILURE;

  float textzoom;
  mv->GetTextZoom(&textzoom);
  textzoom += ((float)change) / 10;
  if (textzoom > 0 && textzoom <= 20)
    mv->SetTextZoom(textzoom);

  return NS_OK;
}


//
// DoTreeScroll
//
// Trees know best how to deal with scrolling, so use the tree scroll api's instead
// of just blindly scrolling the view.
//
nsresult
nsEventStateManager::DoTreeScroll(nsIPresContext* inPresContext, PRInt32 inNumLines,
                                    PRBool inScrollPage, nsITreeFrame* inTreeFrame)
{
  PRInt32 scrollIndex, visibleRows;
  inTreeFrame->GetIndexOfFirstVisibleRow(&scrollIndex);
  inTreeFrame->GetNumberOfVisibleRows(&visibleRows);

  if (inScrollPage)
    scrollIndex += ((inNumLines > 0) ? visibleRows : -visibleRows);
  else
    scrollIndex += inNumLines;
  
  if (scrollIndex < 0)
    scrollIndex = 0;
  else {
    PRInt32 numRows;
    inTreeFrame->GetRowCount(&numRows);
    PRInt32 lastPageTopRow = numRows - visibleRows;
    if (scrollIndex > lastPageTopRow)
      scrollIndex = lastPageTopRow;
  }
  
  inTreeFrame->ScrollToIndex(scrollIndex);

  // we have to do a sync update for mac because if we scroll too quickly
  // w/out going back to the main event loop we can easily scroll the wrong
  // bits and it looks like garbage (bug 63465).
  nsIFrame* frame = nsnull;
  if ( NS_SUCCEEDED(inTreeFrame->QueryInterface(NS_GET_IID(nsIFrame),
                                                (void **)&frame)) ) {
    nsIView* treeView = nsnull;
    frame->GetView(inPresContext, &treeView);
    if (!treeView) {
      nsIFrame* frameWithView;
      frame->GetParentWithView(inPresContext, &frameWithView);
      if (frameWithView)
        frameWithView->GetView(inPresContext, &treeView);
      else
        return NS_ERROR_FAILURE;
    }
    if (treeView)
      ForceViewUpdate(treeView);
  }
  else
    return NS_ERROR_FAILURE;
     
  return NS_OK;
      
} // DoTreeScroll


nsresult
nsEventStateManager::DoWheelScroll(nsIPresContext* aPresContext,
                                   nsIFrame* aTargetFrame,
                                   nsMouseScrollEvent* msEvent,
                                   PRInt32 numLines, PRBool scrollPage,
                                   PRBool aUseTargetFrame)
{
  nsIView* focusView = nsnull;
  nsIScrollableView* sv = nsnull;
  nsIFrame* focusFrame = nsnull;
  
  // Special case for tree/list frames - they handle their own scrolling
  nsITreeFrame* treeFrame = nsnull;
  nsCOMPtr<nsIOutlinerBoxObject> outlinerBoxObject;
  nsIFrame* curFrame = aTargetFrame;
  
  while (curFrame) {
    if (NS_OK == curFrame->QueryInterface(NS_GET_IID(nsITreeFrame),
                                          (void**) &treeFrame))
      break;

    outlinerBoxObject = do_QueryInterface(curFrame);
    if (outlinerBoxObject)
      break;

    curFrame->GetParent(&curFrame);
  }
  
  // Create a mouseout event that we fire to the content before
  // scrolling, to allow tooltips to disappear, etc.

  nsMouseEvent mouseOutEvent;
  mouseOutEvent.eventStructType = NS_MOUSE_EVENT;
  mouseOutEvent.message = NS_MOUSE_EXIT_SYNTH;
  mouseOutEvent.widget = msEvent->widget;
  mouseOutEvent.clickCount = 0;
  mouseOutEvent.point = nsPoint(0,0);
  mouseOutEvent.refPoint = nsPoint(0,0);
  mouseOutEvent.isShift = PR_FALSE;
  mouseOutEvent.isControl = PR_FALSE;
  mouseOutEvent.isAlt = PR_FALSE;
  mouseOutEvent.isMeta = PR_FALSE;

  nsCOMPtr<nsIContent> targetContent;
  aTargetFrame->GetContent(getter_AddRefs(targetContent));

  nsEventStatus mouseoutStatus = nsEventStatus_eIgnore;

  if (treeFrame) {
    if (targetContent)
      targetContent->HandleDOMEvent(aPresContext, &mouseOutEvent, nsnull,
                                    NS_EVENT_FLAG_INIT, &mouseoutStatus);
    return DoTreeScroll(aPresContext, numLines, scrollPage, treeFrame);
  } else if (outlinerBoxObject) {
    if (targetContent)
      targetContent->HandleDOMEvent(aPresContext, &mouseOutEvent, nsnull,
                                    NS_EVENT_FLAG_INIT, &mouseoutStatus);
    if (scrollPage)
      outlinerBoxObject->ScrollByPages((numLines > 0) ? 1 : -1);
    else
      outlinerBoxObject->ScrollByLines(numLines);
    return NS_OK;
  }
  
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  // Otherwise, check for a focused content element
  nsCOMPtr<nsIContent> focusContent;
  if (mCurrentFocus) {
    focusContent = mCurrentFocus;
  }
  else {
    // If there is no focused content, get the document content
    EnsureDocument(presShell);
    mDocument->GetRootContent(getter_AddRefs(focusContent));
  }
  
  if (!focusContent)
    return NS_ERROR_FAILURE;

  if (aUseTargetFrame)
    focusFrame = aTargetFrame;
  else
    presShell->GetPrimaryFrameFor(focusContent, &focusFrame);

  if (!focusFrame)
    return NS_ERROR_FAILURE;

  // Now check whether this frame wants to provide us with an
  // nsIScrollableView to use for scrolling.

  nsCOMPtr<nsIScrollableViewProvider> svp = do_QueryInterface(focusFrame);
  if (svp) {
    svp->GetScrollableView(&sv);
    if (sv)
      sv->QueryInterface(NS_GET_IID(nsIView), (void**) &focusView);
  } else {
    focusFrame->GetView(aPresContext, &focusView);
    if (!focusView) {
      nsIFrame* frameWithView;
      focusFrame->GetParentWithView(aPresContext, &frameWithView);
      if (frameWithView)
        frameWithView->GetView(aPresContext, &focusView);
      else
        return NS_ERROR_FAILURE;
    }
    
    sv = GetNearestScrollingView(focusView);
  }

  if (sv) {
    if (targetContent)
      targetContent->HandleDOMEvent(aPresContext, &mouseOutEvent, nsnull,
                                    NS_EVENT_FLAG_INIT, &mouseoutStatus);

    if (scrollPage)
      sv->ScrollByPages((numLines > 0) ? 1 : -1);
    else
      sv->ScrollByLines(0, numLines);

    if (focusView)
      ForceViewUpdate(focusView);
  } else {
    nsresult rv;
    nsIFrame* newFrame = nsnull;
    nsCOMPtr<nsIPresContext> newPresContext;

    rv = GetParentScrollingView(msEvent, aPresContext, newFrame,
                                *getter_AddRefs(newPresContext));
    if (NS_SUCCEEDED(rv))
      return DoWheelScroll(newPresContext, newFrame, msEvent, numLines,
                           scrollPage, PR_TRUE);
    else
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsEventStateManager::GetParentScrollingView(nsMouseScrollEvent *aEvent,
                                            nsIPresContext* aPresContext,
                                            nsIFrame* &targetOuterFrame,
                                            nsIPresContext* &presCtxOuter)
{
  if (!aEvent) return NS_ERROR_FAILURE;
  if (!aPresContext) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsISupports> shell;
  aPresContext->GetContainer(getter_AddRefs(shell));
  if (!shell) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(shell);
  if (!treeItem) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShellTreeItem> parent;
  treeItem->GetParent(getter_AddRefs(parent));
  if (!parent) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDocShell> pDocShell = do_QueryInterface(parent);
  if (!pDocShell) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIPresShell> presShell;
  pDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell) return NS_ERROR_FAILURE;
  
  nsIFrame* rootFrame = nsnull;
  presShell->GetRootFrame(&rootFrame);   
  if (!rootFrame) return NS_ERROR_FAILURE;
  
  presShell->GetPresContext(&presCtxOuter); //addrefs

  nsPoint eventPoint;
  rootFrame->GetOrigin(eventPoint);
  eventPoint += aEvent->point;

  nsresult rv;
  rv = rootFrame->GetFrameForPoint(presCtxOuter, eventPoint,
                                   NS_FRAME_PAINT_LAYER_FOREGROUND,
                                   &targetOuterFrame);
  if (rv != NS_OK) {
    rv = rootFrame->GetFrameForPoint(presCtxOuter, eventPoint,
                                     NS_FRAME_PAINT_LAYER_FLOATERS,
                                     &targetOuterFrame);
    if (rv != NS_OK) {
      rv = rootFrame->GetFrameForPoint(presCtxOuter, eventPoint,
                                       NS_FRAME_PAINT_LAYER_BACKGROUND,
                                       &targetOuterFrame);
      if (rv != NS_OK) return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::PostHandleEvent(nsIPresContext* aPresContext,
                                     nsEvent *aEvent,
                                     nsIFrame* aTargetFrame,
                                     nsEventStatus* aStatus,
                                     nsIView* aView)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG_POINTER(aStatus);
  mCurrentTarget = aTargetFrame;
  NS_IF_RELEASE(mCurrentTargetContent);
  nsresult ret = NS_OK;

  NS_ASSERTION(mCurrentTarget, "mCurrentTarget is null");
  if (!mCurrentTarget) return NS_ERROR_NULL_POINTER;

  nsFrameState state;
  mCurrentTarget->GetFrameState(&state);
  state |= NS_FRAME_EXTERNAL_REFERENCE;
  mCurrentTarget->SetFrameState(state);

  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
  case NS_MOUSE_RIGHT_BUTTON_DOWN: 
    {
      if (mConsumeFocusEvents) {
        mConsumeFocusEvents = PR_FALSE;
        break;
      }

      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;
        PRBool suppressBlur = PR_FALSE;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(mPresContext, aEvent, getter_AddRefs(newFocus));
          const nsStyleUserInterface* ui;
          mCurrentTarget->GetStyleData(eStyleStruct_UserInterface, ((const nsStyleStruct*&)ui));
          suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);
        }

        nsIFrame* currFrame = mCurrentTarget;
        nsCOMPtr<nsIContent> activeContent;
        if (mCurrentTarget)
          mCurrentTarget->GetContent(getter_AddRefs(activeContent));

        // Look for the nearest enclosing focusable frame.
        while (currFrame) {
          const nsStyleUserInterface* ui;
          currFrame->GetStyleData(eStyleStruct_UserInterface, ((const nsStyleStruct*&)ui));
          if ((ui->mUserFocus != NS_STYLE_USER_FOCUS_IGNORE) &&
              (ui->mUserFocus != NS_STYLE_USER_FOCUS_NONE)) {
            currFrame->GetContent(getter_AddRefs(newFocus));
            nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(newFocus));
            if (domElement)
              break;
          }
          currFrame->GetParent(&currFrame);
        }

        if (newFocus && currFrame)
          ChangeFocus(newFocus, currFrame, PR_TRUE);
        else if (!suppressBlur) {
          SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
        }

        // The rest is left button-specific.
        if (aEvent->message != NS_MOUSE_LEFT_BUTTON_DOWN)
          break;

        if (activeContent) {
          // The nearest enclosing element goes into the
          // :active state.  If we fail the QI to DOMElement,
          // then we know we're only a node, and that we need
          // to obtain our parent element and put it into :active
          // instead.
          nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(activeContent));
          if (!elt) {
            nsCOMPtr<nsIContent> par;
            activeContent->GetParent(*getter_AddRefs(par));
            if (par)
              activeContent = par;
          }
          SetContentState(activeContent, NS_EVENT_STATE_ACTIVE);
        }
      }
      else {
        // if we're here, the event handler returned false, so stop
        // any of our own processing of a drag. Workaround for bug 43258.
        StopTrackingDragGesture();      
      }
    }
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    {
      SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
      if (!mCurrentTarget) {
        nsIFrame* targ;
        GetEventTarget(&targ);
        if (!targ) return NS_ERROR_FAILURE;
      }
      ret = CheckForAndDispatchClick(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      nsCOMPtr<nsIPresShell> shell;
      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        nsCOMPtr<nsIFrameSelection> frameSel;
        rv = shell->GetFrameSelection(getter_AddRefs(frameSel));
        if (NS_SUCCEEDED(rv) && frameSel){
            frameSel->SetMouseDownState(PR_FALSE);
        }
      }
    }
    break;
  case NS_MOUSE_SCROLL:
    if (nsEventStatus_eConsumeNoDefault != *aStatus) {
      nsresult rv;
      rv = getPrefService();
      if (NS_FAILED(rv)) return rv;

      nsMouseScrollEvent *msEvent = (nsMouseScrollEvent*) aEvent;
      PRInt32 action = 0;
      PRInt32 numLines = 0;
      PRBool aBool;
      if (msEvent->isShift) {
        mPrefService->GetIntPref("mousewheel.withshiftkey.action", &action);
        mPrefService->GetBoolPref("mousewheel.withshiftkey.sysnumlines",
                                  &aBool);
        if (aBool) {
          numLines = msEvent->delta;
          if (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)
            action = MOUSE_SCROLL_PAGE;
        }
        else
          mPrefService->GetIntPref("mousewheel.withshiftkey.numlines",
                                   &numLines);
      } else if (msEvent->isControl) {
        mPrefService->GetIntPref("mousewheel.withcontrolkey.action", &action);
        mPrefService->GetBoolPref("mousewheel.withcontrolkey.sysnumlines",
                                  &aBool);
        if (aBool) {
          numLines = msEvent->delta;
          if (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)
            action = MOUSE_SCROLL_PAGE;          
        }
        else
          mPrefService->GetIntPref("mousewheel.withcontrolkey.numlines",
                                   &numLines);
      } else if (msEvent->isAlt) {
        mPrefService->GetIntPref("mousewheel.withaltkey.action", &action);
        mPrefService->GetBoolPref("mousewheel.withaltkey.sysnumlines", &aBool);
        if (aBool) {
          numLines = msEvent->delta;
          if (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)
            action = MOUSE_SCROLL_PAGE;
        }
        else
          mPrefService->GetIntPref("mousewheel.withaltkey.numlines",
                                   &numLines);
      } else {
        mPrefService->GetIntPref("mousewheel.withnokey.action", &action);
        mPrefService->GetBoolPref("mousewheel.withnokey.sysnumlines", &aBool);
        if (aBool) {
          numLines = msEvent->delta;
          if (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)
            action = MOUSE_SCROLL_PAGE;
        }
        else
          mPrefService->GetIntPref("mousewheel.withnokey.numlines", &numLines);
      }

      if ((msEvent->delta < 0) && (numLines > 0))
        numLines = -numLines;

      switch (action) {
      case MOUSE_SCROLL_N_LINES:
      case MOUSE_SCROLL_PAGE:
        {
          DoWheelScroll(aPresContext, aTargetFrame, msEvent, numLines,
                        (action == MOUSE_SCROLL_PAGE), PR_FALSE);

        }

        break;

      case MOUSE_SCROLL_HISTORY:
        {
          nsCOMPtr<nsISupports> pcContainer;
          mPresContext->GetContainer(getter_AddRefs(pcContainer));
          if (pcContainer) {
            nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(pcContainer));
            if (webNav) {
              if (msEvent->delta > 0)
                 webNav->GoBack();
              else
                 webNav->GoForward();
            }
          }
        }
        break;

      case MOUSE_SCROLL_TEXTSIZE:
        ChangeTextSize((msEvent->delta > 0) ? 1 : -1);
        break;
      }
      *aStatus = nsEventStatus_eConsumeNoDefault;

    }

    break;

  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    // clean up after ourselves. make sure we do this _after_ the event, else we'll
    // clean up too early!
    GenerateDragDropEnterExit(aPresContext, (nsGUIEvent*)aEvent);
    break;

  case NS_KEY_UP:
    if (mBrowseWithCaret) 
      MoveFocusToCaret();
    break;

  case NS_KEY_PRESS:
    if (nsEventStatus_eConsumeNoDefault != *aStatus) {
      nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
      //This is to prevent keyboard scrolling while alt modifier in use.
      if (!keyEvent->isAlt) {
        switch(keyEvent->keyCode) {
          case NS_VK_TAB:
            if (mConsumeFocusEvents) {
              mConsumeFocusEvents = PR_FALSE;
              break;
            }
            if (!((nsInputEvent*)aEvent)->isControl) {
              //Shift focus forward or back depending on shift key
              ShiftFocus(!((nsInputEvent*)aEvent)->isShift);
            } else {
              ShiftFocusByDoc(!((nsInputEvent*)aEvent)->isShift);
            }
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;

          case NS_VK_F6:
            if (mConsumeFocusEvents) {
              mConsumeFocusEvents = PR_FALSE;
              break;
            }
            //Shift focus forward or back depending on shift key
            ShiftFocusByDoc(!((nsInputEvent*)aEvent)->isShift);
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;

//the problem is that viewer does not have xul so we cannot completely eliminate these
#if NON_KEYBINDING
          case NS_VK_PAGE_DOWN:
          case NS_VK_PAGE_UP:
            if (!mCurrentFocus) {
              nsIScrollableView* sv = GetNearestScrollingView(aView);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByPages((keyEvent->keyCode != NS_VK_PAGE_UP) ? 1 : -1);
              }
            }
            break;
          case NS_VK_HOME: 
          case NS_VK_END:
            if (!mCurrentFocus) {
              nsIScrollableView* sv = GetNearestScrollingView(aView);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByWhole((keyEvent->keyCode != NS_VK_HOME) ? PR_FALSE : PR_TRUE);
              }
            }
            break;
          case NS_VK_DOWN: 
          case NS_VK_UP:
            if (!mCurrentFocus) {
              nsIScrollableView* sv = GetNearestScrollingView(aView);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByLines(0, (keyEvent->keyCode == NS_VK_DOWN) ? 1 : -1);
              
                // force the update to happen now, otherwise multiple scrolls can
                // occur before the update is processed. (bug #7354)
                nsIViewManager* vm = nsnull;
             	  if (NS_OK == aView->GetViewManager(vm) && nsnull != vm) {
             	    // I'd use Composite here, but it doesn't always work.
                  // vm->Composite();
                  vm->ForceUpdate();
                  NS_RELEASE(vm);
                }
              }
            }
            break;
          case NS_VK_LEFT: 
          case NS_VK_RIGHT:
            if (!mCurrentFocus) {
              nsIScrollableView* sv = GetNearestScrollingView(aView);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByLines((keyEvent->keyCode == NS_VK_RIGHT) ? 1 : -1, 0);
              
                // force the update to happen now, otherwise multiple scrolls can
                // occur before the update is processed. (bug #7354)
                nsIViewManager* vm = nsnull;
             	  if (NS_OK == aView->GetViewManager(vm) && nsnull != vm) {
             	    // I'd use Composite here, but it doesn't always work.
                  // vm->Composite();
                  vm->ForceUpdate();
                  NS_RELEASE(vm);
                }
              }
            }
            break;
        case 0: /* check charcode since keycode is 0 */
          {
          //Spacebar
            nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
            if (keyEvent->charCode == 0x20) {
              if (!mCurrentFocus) {
                nsIScrollableView* sv = GetNearestScrollingView(aView);
                if (sv) {
                  sv->ScrollByPages(1);
                }
              }
            }
          }
          break;
#endif //NON_KEYBINDING
        }
      }
    }
  case NS_MOUSE_ENTER:
    nsCOMPtr<nsIContent> targetContent;
    if (mCurrentTarget) {
      mCurrentTarget->GetContentForEvent(aPresContext, aEvent,
                                         getter_AddRefs(targetContent));
      SetContentState(targetContent, NS_EVENT_STATE_HOVER);
    }
    break;

  }

  //Reset target frame to null to avoid mistargetting after reentrant event
  mCurrentTarget = nsnull;

  return ret;
}

NS_IMETHODIMP
nsEventStateManager::SetPresContext(nsIPresContext* aPresContext)
{
  if (aPresContext == nsnull) {
    // A pres context is going away. Make sure we do cleanup.
    if (mPresContext == gLastFocusedPresContext) {
      gLastFocusedPresContext = nsnull;
      NS_IF_RELEASE(gLastFocusedDocument);
      NS_IF_RELEASE(gLastFocusedContent);
    }
  }

  mPresContext = aPresContext;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::ClearFrameRefs(nsIFrame* aFrame)
{
  if (aFrame == mLastMouseOverFrame)
    mLastMouseOverFrame = nsnull;
  if (aFrame == mLastDragOverFrame)
    mLastDragOverFrame = nsnull;
  if (aFrame == mGestureDownFrame) {
    mGestureDownFrame = nsnull;
 #if CLICK_HOLD_CONTEXT_MENUS
    mEventDownWidget = nsnull;
    mEventPresContext = nsnull;
#endif
  }
  if (aFrame == mCurrentTarget) {
    if (aFrame) {
      aFrame->GetContent(&mCurrentTargetContent);
    }
    mCurrentTarget = nsnull;
  }
  

  return NS_OK;
}

nsIScrollableView*
nsEventStateManager::GetNearestScrollingView(nsIView* aView)
{
  nsIScrollableView* sv;
  if (NS_OK == aView->QueryInterface(NS_GET_IID(nsIScrollableView),
                                     (void**)&sv)) {
    return sv;
  }

  nsIView* parent;
  aView->GetParent(parent);

  if (nsnull != parent) {
    return GetNearestScrollingView(parent);
  }

  return nsnull;
}

PRBool
nsEventStateManager::CheckDisabled(nsIContent* aContent)
{
  PRBool disabled = PR_FALSE;

  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));

  if (nsHTMLAtoms::input == tag.get() ||
      nsHTMLAtoms::select == tag.get() ||
      nsHTMLAtoms::textarea == tag.get() ||
      nsHTMLAtoms::button == tag.get()) {
    nsAutoString empty;
    if (NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, 
                                                           nsHTMLAtoms::disabled,
                                                           empty)) {
      disabled = PR_TRUE;
    }
  }
  
  return disabled;
}

void
nsEventStateManager::UpdateCursor(nsIPresContext* aPresContext, nsEvent* aEvent, nsIFrame* aTargetFrame, 
                                  nsEventStatus* aStatus)
{
  PRInt32 cursor;

  //If cursor is locked just use the locked one
  if (mLockCursor) {
    cursor = mLockCursor;
  }
  //If not locked, look for correct cursor
  else {
    nsCOMPtr<nsIContent> targetContent;
    if (mCurrentTarget) {
      mCurrentTarget->GetContent(getter_AddRefs(targetContent));
    }

    //Check if the current target is disabled.  If so use the default pointer.
    if (targetContent && CheckDisabled(targetContent)) {
      cursor = NS_STYLE_CURSOR_DEFAULT;
    }
    //If not disabled, check for the right cursor.
    else {
      if (aTargetFrame) {
        aTargetFrame->GetCursor(aPresContext, aEvent->point, cursor);
      }
    }
  }

  // Check whether or not to show the busy cursor
  nsCOMPtr<nsISupports> pcContainer;
  aPresContext->GetContainer(getter_AddRefs(pcContainer));    
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(pcContainer));    
  if (!docShell) return;
  PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
  docShell->GetBusyFlags(&busyFlags);

  // Show busy cursor everywhere before page loads
  // and just replace the arrow cursor after page starts loading
  if (busyFlags & nsIDocShell::BUSY_FLAGS_BUSY &&
        (cursor == NS_STYLE_CURSOR_AUTO || cursor == NS_STYLE_CURSOR_DEFAULT))
  {
    cursor = NS_STYLE_CURSOR_SPINNING;
  }
 
  if (aTargetFrame) {
    nsCOMPtr<nsIWidget> window;
    aTargetFrame->GetWindow(aPresContext, getter_AddRefs(window));
    SetCursor(cursor, window, PR_FALSE);
  }

  if (mLockCursor || NS_STYLE_CURSOR_AUTO != cursor) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }
}

NS_IMETHODIMP
nsEventStateManager::SetCursor(PRInt32 aCursor, nsIWidget* aWidget, PRBool aLockCursor)
{
  nsCursor c;

  NS_ENSURE_TRUE(aWidget, NS_ERROR_FAILURE);
  if (aLockCursor) {
    if (NS_STYLE_CURSOR_AUTO != aCursor) {
      mLockCursor = aCursor;
    }
    else {
      //If cursor style is set to auto we unlock the cursor again.
      mLockCursor = 0;
    }
  }
  switch (aCursor) {
  default:
  case NS_STYLE_CURSOR_AUTO:
  case NS_STYLE_CURSOR_DEFAULT:
    c = eCursor_standard;
    break;
  case NS_STYLE_CURSOR_POINTER:
    c = eCursor_hyperlink;
    break;
  case NS_STYLE_CURSOR_CROSSHAIR:
    c = eCursor_crosshair;
    break;
  case NS_STYLE_CURSOR_MOVE:
    c = eCursor_move;
    break;
  case NS_STYLE_CURSOR_TEXT:
    c = eCursor_select;
    break;
  case NS_STYLE_CURSOR_WAIT:
    c = eCursor_wait;
    break;
  case NS_STYLE_CURSOR_HELP:
    c = eCursor_help;
    break;
  case NS_STYLE_CURSOR_N_RESIZE:
  case NS_STYLE_CURSOR_S_RESIZE:
    c = eCursor_sizeNS;
    break;
  case NS_STYLE_CURSOR_W_RESIZE:
  case NS_STYLE_CURSOR_E_RESIZE:
    c = eCursor_sizeWE;
    break;
  case NS_STYLE_CURSOR_NW_RESIZE:
    c = eCursor_sizeNW;
    break;
  case NS_STYLE_CURSOR_SE_RESIZE:
    c = eCursor_sizeSE;
    break;
  case NS_STYLE_CURSOR_NE_RESIZE:
    c = eCursor_sizeNE;
    break;
  case NS_STYLE_CURSOR_SW_RESIZE:
    c = eCursor_sizeSW;
    break;
  case NS_STYLE_CURSOR_COPY: // CSS3
    c = eCursor_copy;
    break;
  case NS_STYLE_CURSOR_ALIAS:
    c = eCursor_alias;
    break;
  case NS_STYLE_CURSOR_CONTEXT_MENU:
    c = eCursor_context_menu;
    break;
  case NS_STYLE_CURSOR_CELL:
    c = eCursor_cell;
    break;
  case NS_STYLE_CURSOR_GRAB:
    c = eCursor_grab;
    break;
  case NS_STYLE_CURSOR_GRABBING:
    c = eCursor_grabbing;
    break;
  case NS_STYLE_CURSOR_SPINNING:
    c = eCursor_spinning;
    break;
  case NS_STYLE_CURSOR_COUNT_UP:
    c = eCursor_count_up;
    break;
  case NS_STYLE_CURSOR_COUNT_DOWN:
    c = eCursor_count_down;
    break;
  case NS_STYLE_CURSOR_COUNT_UP_DOWN:
    c = eCursor_count_up_down;
    break;
  }

  aWidget->SetCursor(c);

  return NS_OK;
}

void
nsEventStateManager::GenerateMouseEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent)
{
  //Hold onto old target content through the event and reset after.
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_MOUSE_MOVE:
    {
      nsCOMPtr<nsIContent> targetContent;
      if (mCurrentTarget) {
        mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(targetContent));
      }

      if (mLastMouseOverContent.get() != targetContent.get()) {

        //Before firing mouseout, check for recursion
        if (mLastMouseOverContent.get() != mFirstMouseOutEventContent.get()) {
        
          //Store the first mouseOut event we fire and don't refire mouseOut
          //to that element while the first mouseOut is still ongoing.
          mFirstMouseOutEventContent = mLastMouseOverContent;

          if (mLastMouseOverFrame) {
            //fire mouseout
            nsEventStatus status = nsEventStatus_eIgnore;
            nsMouseEvent event;
            event.eventStructType = NS_MOUSE_EVENT;
            event.message = NS_MOUSE_EXIT_SYNTH;
            event.widget = aEvent->widget;
            event.clickCount = 0;
            event.point = aEvent->point;
            event.refPoint = aEvent->refPoint;
            event.isShift = ((nsMouseEvent*)aEvent)->isShift;
            event.isControl = ((nsMouseEvent*)aEvent)->isControl;
            event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
            event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;
            event.nativeMsg = ((nsMouseEvent*)aEvent)->nativeMsg;

            mCurrentTargetContent = mLastMouseOverContent;
            NS_IF_ADDREF(mCurrentTargetContent);
            mCurrentRelatedContent = targetContent;
            NS_IF_ADDREF(mCurrentRelatedContent);

            //XXX This event should still go somewhere!!
            if (mLastMouseOverContent) {
              mLastMouseOverContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
            }

            //Now dispatch to the frame
            if (mLastMouseOverFrame) {
              //XXX Get the new frame
              mLastMouseOverFrame->HandleEvent(aPresContext, &event, &status);   
            }

            NS_IF_RELEASE(mCurrentTargetContent);
            NS_IF_RELEASE(mCurrentRelatedContent);

            mFirstMouseOutEventContent = nsnull;
          }
        }

        //Before firing mouseover, check for recursion
        if (targetContent.get() != mFirstMouseOverEventContent.get()) {
        
          //Store the first mouseOver event we fire and don't refire mouseOver
          //to that element while the first mouseOver is still ongoing.
          mFirstMouseOverEventContent = targetContent;

          //fire mouseover
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_MOUSE_EVENT;
          event.message = NS_MOUSE_ENTER_SYNTH;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.isShift = ((nsMouseEvent*)aEvent)->isShift;
          event.isControl = ((nsMouseEvent*)aEvent)->isControl;
          event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
          event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;
          event.nativeMsg = ((nsMouseEvent*)aEvent)->nativeMsg;

          mCurrentTargetContent = targetContent;
          NS_IF_ADDREF(mCurrentTargetContent);
          mCurrentRelatedContent = mLastMouseOverContent;
          NS_IF_ADDREF(mCurrentRelatedContent);

          //XXX This event should still go somewhere!!
          if (targetContent) {
            targetContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
          }
        
          if ( status != nsEventStatus_eConsumeNoDefault )
            SetContentState(targetContent, NS_EVENT_STATE_HOVER);
          
          //Now dispatch to the frame
          if (mCurrentTarget) {
            //XXX Get the new frame
            mCurrentTarget->HandleEvent(aPresContext, &event, &status);
          }

          NS_IF_RELEASE(mCurrentTargetContent);
          NS_IF_RELEASE(mCurrentRelatedContent);
          mLastMouseOverFrame = mCurrentTarget;
          mLastMouseOverContent = targetContent;

          mFirstMouseOverEventContent = nsnull;
        }
      }
    }
    break;
  case NS_MOUSE_EXIT:
    {
      //This is actually the window mouse exit event.
      if (nsnull != mLastMouseOverFrame) {
        //Before firing mouseout, check for recursion
        if (mLastMouseOverContent.get() != mFirstMouseOutEventContent.get()) {
    
          //Store the first mouseOut event we fire and don't refire mouseOut
          //to that element while the first mouseOut is still ongoing.
          mFirstMouseOutEventContent = mLastMouseOverContent;

          //fire mouseout
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_MOUSE_EVENT;
          event.message = NS_MOUSE_EXIT_SYNTH;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.isShift = ((nsMouseEvent*)aEvent)->isShift;
          event.isControl = ((nsMouseEvent*)aEvent)->isControl;
          event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
          event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;
          event.nativeMsg = ((nsMouseEvent*)aEvent)->nativeMsg;

          mCurrentTargetContent = mLastMouseOverContent;
          NS_IF_ADDREF(mCurrentTargetContent);
          mCurrentRelatedContent = nsnull;

          if (mLastMouseOverContent) {
            mLastMouseOverContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 

            if ( status != nsEventStatus_eConsumeNoDefault )
              SetContentState(nsnull, NS_EVENT_STATE_HOVER);
          }

          //Now dispatch to the frame
          if (nsnull != mLastMouseOverFrame) {
            //XXX Get the new frame
            mLastMouseOverFrame->HandleEvent(aPresContext, &event, &status);   
            mLastMouseOverFrame = nsnull;
            mLastMouseOverContent = nsnull;
          }

          NS_IF_RELEASE(mCurrentTargetContent);

          mFirstMouseOutEventContent = nsnull;
        }
      }
    }
    break;
  }

  //reset mCurretTargetContent to what it was
  mCurrentTargetContent = targetBeforeEvent;
}

void
nsEventStateManager::GenerateDragDropEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent)
{
  //Hold onto old target content through the event and reset after.
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_DRAGDROP_OVER:
    {
      if (mLastDragOverFrame != mCurrentTarget) {
        //We'll need the content, too, to check if it changed separately from the frames.
        nsCOMPtr<nsIContent> lastContent;
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(targetContent));

        if ( mLastDragOverFrame ) {
          //fire drag exit
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_DRAGDROP_EVENT;
          event.message = NS_DRAGDROP_EXIT_SYNTH;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.isShift = ((nsMouseEvent*)aEvent)->isShift;
          event.isControl = ((nsMouseEvent*)aEvent)->isControl;
          event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
          event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

          //The frame has change but the content may not have.  Check before dispatching to content
          mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

          mCurrentTargetContent = lastContent;
          NS_IF_ADDREF(mCurrentTargetContent);
          mCurrentRelatedContent = targetContent;
          NS_IF_ADDREF(mCurrentRelatedContent);

          if ( lastContent != targetContent ) {
            //XXX This event should still go somewhere!!
            if (lastContent)
              lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 

            // clear the drag hover
            if (status != nsEventStatus_eConsumeNoDefault )
              SetContentState(nsnull, NS_EVENT_STATE_DRAGOVER);
          }

          // Finally dispatch exit to the frame
          if ( mLastDragOverFrame ) {
            mLastDragOverFrame->HandleEvent(aPresContext, &event, &status);   

          NS_IF_RELEASE(mCurrentTargetContent);
          NS_IF_RELEASE(mCurrentRelatedContent);

          }
        }

        //fire drag enter
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_DRAGDROP_EVENT;
        event.message = NS_DRAGDROP_ENTER;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;
        event.isShift = ((nsMouseEvent*)aEvent)->isShift;
        event.isControl = ((nsMouseEvent*)aEvent)->isControl;
        event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
        event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

        mCurrentTargetContent = targetContent;
        NS_IF_ADDREF(mCurrentTargetContent);
        mCurrentRelatedContent = lastContent;
        NS_IF_ADDREF(mCurrentRelatedContent);

        //The frame has change but the content may not have.  Check before dispatching to content
        if ( lastContent != targetContent ) {
          //XXX This event should still go somewhere!!
          if ( targetContent ) 
            targetContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 

          // set drag hover on this frame
          if ( status != nsEventStatus_eConsumeNoDefault )
            SetContentState(targetContent, NS_EVENT_STATE_DRAGOVER);
        }

        // Finally dispatch to the frame
        if (mCurrentTarget) {
          //XXX Get the new frame
          mCurrentTarget->HandleEvent(aPresContext, &event, &status);
        }

        NS_IF_RELEASE(mCurrentTargetContent);
        NS_IF_RELEASE(mCurrentRelatedContent);
        mLastDragOverFrame = mCurrentTarget;
      }
    }
    break;
    
  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    {
      //This is actually the window mouse exit event.
      if ( mLastDragOverFrame ) {

        // fire mouseout
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_DRAGDROP_EVENT;
        event.message = NS_DRAGDROP_EXIT_SYNTH;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;
        event.isShift = ((nsMouseEvent*)aEvent)->isShift;
        event.isControl = ((nsMouseEvent*)aEvent)->isControl;
        event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
        event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

        // dispatch to content via DOM
        nsCOMPtr<nsIContent> lastContent;
        mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

        mCurrentTargetContent = lastContent;
        NS_IF_ADDREF(mCurrentTargetContent);
        mCurrentRelatedContent = nsnull;

        if ( lastContent ) {
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
          if ( status != nsEventStatus_eConsumeNoDefault )
            SetContentState(nsnull, NS_EVENT_STATE_DRAGOVER);
        }

        // Finally dispatch to the frame
        if ( mLastDragOverFrame ) {
          //XXX Get the new frame
          mLastDragOverFrame->HandleEvent(aPresContext, &event, &status);   
          mLastDragOverFrame = nsnull;
        }

        NS_IF_RELEASE(mCurrentTargetContent);
     }
    }
    break;
  }

  //reset mCurretTargetContent to what it was
  mCurrentTargetContent = targetBeforeEvent;

  // Now flush all pending notifications.
  FlushPendingEvents(aPresContext);
}

NS_IMETHODIMP
nsEventStateManager::SetClickCount(nsIPresContext* aPresContext, 
                                   nsMouseEvent *aEvent,
                                   nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;
  nsCOMPtr<nsIContent> mouseContent;

  mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(mouseContent));

  switch (aEvent->message) {
  case NS_MOUSE_LEFT_BUTTON_DOWN:
    NS_IF_RELEASE(mLastLeftMouseDownContent);
    mLastLeftMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastLeftMouseDownContent);
    break;

  case NS_MOUSE_LEFT_BUTTON_UP:
    if (mLastLeftMouseDownContent == mouseContent.get()) {
      aEvent->clickCount = mLClickCount;
      mLClickCount = 0;
    }
    else {
      aEvent->clickCount = 0;
    }
    NS_IF_RELEASE(mLastLeftMouseDownContent);
    break;

  case NS_MOUSE_MIDDLE_BUTTON_DOWN:
    NS_IF_RELEASE(mLastMiddleMouseDownContent);
    mLastMiddleMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastMiddleMouseDownContent);
    break;

  case NS_MOUSE_MIDDLE_BUTTON_UP:
    if (mLastMiddleMouseDownContent == mouseContent.get()) {
      aEvent->clickCount = mMClickCount;
      mMClickCount = 0;
    }
    else {
      aEvent->clickCount = 0;
    }
    NS_IF_RELEASE(mLastMiddleMouseDownContent);
    break;

  case NS_MOUSE_RIGHT_BUTTON_DOWN:
    NS_IF_RELEASE(mLastRightMouseDownContent);
    mLastRightMouseDownContent = mouseContent;
    NS_IF_ADDREF(mLastRightMouseDownContent);
    break;

  case NS_MOUSE_RIGHT_BUTTON_UP:
    if (mLastRightMouseDownContent == mouseContent.get()) {
      aEvent->clickCount = mRClickCount;
      mRClickCount = 0;
    }
    else {
      aEvent->clickCount = 0;
    }
    NS_IF_RELEASE(mLastRightMouseDownContent);
    break;
  }

  return ret;
}

NS_IMETHODIMP
nsEventStateManager::CheckForAndDispatchClick(nsIPresContext* aPresContext, 
                                              nsMouseEvent *aEvent,
                                              nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;
  nsMouseEvent event;
  nsCOMPtr<nsIContent> mouseContent;

  mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(mouseContent));

  //If mouse is still over same element, clickcount will be > 1.
  //If it has moved it will be zero, so no click.
  if (0 != aEvent->clickCount) {
    //fire click
    switch (aEvent->message) {
    case NS_MOUSE_LEFT_BUTTON_UP:
      event.message = NS_MOUSE_LEFT_CLICK;
      break;
    case NS_MOUSE_MIDDLE_BUTTON_UP:
      event.message = NS_MOUSE_MIDDLE_CLICK;
      break;
    case NS_MOUSE_RIGHT_BUTTON_UP:
      event.message = NS_MOUSE_RIGHT_CLICK;
      break;
    }

    event.eventStructType = NS_MOUSE_EVENT;
    event.widget = aEvent->widget;
    event.point = aEvent->point;
    event.refPoint = aEvent->refPoint;
    event.clickCount = aEvent->clickCount;
    event.isShift = aEvent->isShift;
    event.isControl = aEvent->isControl;
    event.isAlt = aEvent->isAlt;
    event.isMeta = aEvent->isMeta;

    nsCOMPtr<nsIPresShell> presShell;
    mPresContext->GetShell(getter_AddRefs(presShell));
    if (presShell) {
      ret = presShell->HandleEventWithTarget(&event, mCurrentTarget, mouseContent, NS_EVENT_FLAG_INIT, aStatus);
      if (NS_SUCCEEDED(ret) && aEvent->clickCount == 2) {
        nsMouseEvent event2;
        //fire double click
        switch (aEvent->message) {
        case NS_MOUSE_LEFT_BUTTON_UP:
          event2.message = NS_MOUSE_LEFT_DOUBLECLICK;
          break;
        case NS_MOUSE_MIDDLE_BUTTON_UP:
          event2.message = NS_MOUSE_MIDDLE_DOUBLECLICK;
          break;
        case NS_MOUSE_RIGHT_BUTTON_UP:
          event2.message = NS_MOUSE_RIGHT_DOUBLECLICK;
          break;
        }
        
        event2.eventStructType = NS_MOUSE_EVENT;
        event2.widget = aEvent->widget;
        event2.point = aEvent->point;
        event2.refPoint = aEvent->refPoint;
        event2.clickCount = aEvent->clickCount;
        event2.isShift = aEvent->isShift;
        event2.isControl = aEvent->isControl;
        event2.isAlt = aEvent->isAlt;
        event2.isMeta = aEvent->isMeta;

        ret = presShell->HandleEventWithTarget(&event2, mCurrentTarget, mouseContent, NS_EVENT_FLAG_INIT, aStatus);
      }
    }
  }
  return ret;
}

PRBool
nsEventStateManager::ChangeFocus(nsIContent* aFocusContent, nsIFrame* aTargetFrame, PRBool aSetFocus)
{
  aFocusContent->SetFocus(mPresContext);
  MoveCaretToFocus();

  return PR_FALSE;
}

NS_IMETHODIMP
nsEventStateManager::MoveFocus(PRBool aDirection, nsIContent* aRoot)
{
  ShiftFocus(aDirection, aRoot);
  return NS_OK;
}

//---------------------------------------------------------
// Debug Helpers
#ifdef DEBUG_DOCSHELL_FOCUS
static void 
PrintDocTree(nsIDocShellTreeNode * aParentNode, int aLevel)
{
  for (PRInt32 i=0;i<aLevel;i++) printf("  ");

  PRInt32 childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentNode));
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(aParentNode));
  PRInt32 type;
  parentAsItem->GetItemType(&type);
  nsCOMPtr<nsIPresShell> presShell;
  parentAsDocShell->GetPresShell(getter_AddRefs(presShell));
  nsCOMPtr<nsIPresContext> presContext;
  parentAsDocShell->GetPresContext(getter_AddRefs(presContext));
  nsCOMPtr<nsIDocument> doc;
  presShell->GetDocument(getter_AddRefs(doc));

  nsCOMPtr<nsIScriptGlobalObject> sgo;
  doc->GetScriptGlobalObject(getter_AddRefs(sgo));
  nsCOMPtr<nsIDOMWindowInternal> domwin(do_QueryInterface(sgo));

  nsCOMPtr<nsIWidget> widget;
  nsCOMPtr<nsIViewManager> vm;
  presShell->GetViewManager(getter_AddRefs(vm));
  if (vm) {
    vm->GetWidget(getter_AddRefs(widget));
  }
  nsCOMPtr<nsIEventStateManager> esm;
  presContext->GetEventStateManager(getter_AddRefs(esm));

  printf("DS %p  Type %s  Cnt %d  Doc %p  DW %p  EM %p\n", 
    parentAsDocShell.get(), 
    type==nsIDocShellTreeItem::typeChrome?"Chrome":"Content", 
    childWebshellCount, doc.get(), domwin.get(), esm.get());

  if (childWebshellCount > 0) {
    for (PRInt32 i=0;i<childWebshellCount;i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentNode->GetChildAt(i, getter_AddRefs(child));
      nsCOMPtr<nsIDocShellTreeNode> childAsNode(do_QueryInterface(child));
      PrintDocTree(childAsNode, aLevel+1);
    }
  }
}

static void 
PrintDocTree(nsIDocShellTreeNode * aParentNode)
{
  NS_ASSERTION(aParentNode, "Pointer is null!");

  nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(aParentNode));
  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  item->GetParent(getter_AddRefs(parentItem));
  while (parentItem) {
    nsCOMPtr<nsIDocShellTreeItem>tmp;
    parentItem->GetParent(getter_AddRefs(tmp));
    if (!tmp) {
      break;
    }
    parentItem = tmp;
  }

  if (!parentItem) {
    parentItem = do_QueryInterface(aParentNode);
  }

  if (parentItem) {
    nsCOMPtr<nsIDocShellTreeNode> parentAsNode(do_QueryInterface(parentItem));
    PrintDocTree(parentAsNode, 0);
  }
}
#endif // end debug helpers

void
nsEventStateManager::ShiftFocus(PRBool forward, nsIContent* aRoot)
{
  nsCOMPtr<nsIDocShell> docShell;
  if (mPresContext) {
    nsCOMPtr<nsISupports> pcContainer;
    mPresContext->GetContainer(getter_AddRefs(pcContainer));
    if (pcContainer)
      docShell = do_QueryInterface(pcContainer);
  }

#ifdef DEBUG_DOCSHELL_FOCUS
  printf("ShiftFocus DocShell %p   Doc: %p\n", docshell.get(), mDocument);
#endif

  // Indicates whether the document itself (i.e. no content) has focus
  PRBool docHasFocus = PR_FALSE;

  if (nsnull == mPresContext) {
    return;
  }

  EnsureDocument(mPresContext);
  if (nsnull == mDocument) {
    return;
  }

  // Figure out what type of document we are
  eDocType docType;
  FigureOutKindOfDoc(mDocument, &docType);
#ifdef DEBUG_DOCSHELL_FOCUS
  printf("Doc Type is [%s]\n", gDocTypeNames[docType]);
#endif
  
  if (aRoot) {
    NS_IF_RELEASE(mCurrentFocus);
    mCurrentFocus = aRoot;
    NS_ADDREF(mCurrentFocus);
    //mCurrentTabIndex = forward ? 1 : 0;
    // for frames this needs to be set to 0
    // otherwise it doesn't look for the next content correctly
    //if (docType == eFrame) {
      mCurrentTabIndex = 0;
    //}
  }
  else if (nsnull == mCurrentFocus) {
    mDocument->GetRootContent(&mCurrentFocus);
    if (nsnull == mCurrentFocus) {
      return;
    }
    mCurrentTabIndex = forward ? 1 : 0;
    docHasFocus = PR_TRUE;
  }

  nsIFrame* primaryFrame = nsnull;
  nsCOMPtr<nsIPresShell> shell;
  if (mPresContext) {
    nsresult rv = mPresContext->GetShell(getter_AddRefs(shell));
    if (NS_SUCCEEDED(rv) && shell){
      if (mBrowseWithCaret) {
        nsCOMPtr<nsIContent> caretFocus;
        PRUint32 caretOffset;
        GetCaretLocation(getter_AddRefs(caretFocus), &primaryFrame, &caretOffset);
      }
      else {
        shell->GetPrimaryFrameFor(mCurrentFocus, &primaryFrame);
        if (docHasFocus) {
          // so if we are going backwards then find the last piece of content
          // and and look backwards for the last focusable content
          if (!forward) {
            nsCOMPtr<nsIContent> lastContent = getter_AddRefs(GetLastContent(docShell));
            nsCOMPtr<nsIDOMHTMLIFrameElement> iframe(do_QueryInterface(lastContent));
            if (iframe) {
              if (FocusWithinHTMLIFrameDoc(lastContent, forward)) {
                return;
              }
            }
            shell->GetPrimaryFrameFor(lastContent, &primaryFrame);
          }
        }
      }
    }
  }

  PRBool doFocusAvailDocShells = PR_FALSE;
  nsCOMPtr<nsIContent> next;
  PRBool hadDocFocus = docHasFocus;

  nsCOMPtr<nsIContent> rootContent;
  mDocument->GetRootContent(getter_AddRefs(rootContent));

  // Check here to see if the HTMLFrame wants to do anything special
  // with the focus. i.e. like focusing a docshell child, or if it
  // is the last frame in the frameset, it will pass it on to the Chrome
  if (docType == eFrame && aRoot == nsnull) {
    if (FocusWithinHTMLFrameDoc(aRoot, shell, forward, doFocusAvailDocShells)) {
      return;
    } else if (docHasFocus) {
      // If we are starting at the top of the document
      // first check to see if there is a tabbable index to go to
      //
      // GetNextTabbableIndexContent may change the value mCurrentTabIndex
      // so we remember it here and reset it back if it didn't find anything
      PRInt32 currentTabIndex = mCurrentTabIndex;
      GetNextTabbableIndexContent(rootContent, forward, PR_TRUE, getter_AddRefs(next));
      if (!next) {
        mCurrentTabIndex = currentTabIndex;
      }

      // frames are document first, then content
      // so this allows us to call GetNextTabbableContent below
      docHasFocus = PR_FALSE; 
    }
  }


  PRBool docFocusFirst = PR_FALSE;
  if (docShell)
    docShell->GetFocusDocBeforeContent(&docFocusFirst);

  //Get the next tab item.  This takes tabIndex into account
  if ((docFocusFirst || !docHasFocus) && !next)
    GetNextTabbableContent(rootContent, primaryFrame, forward, getter_AddRefs(next));
  
  //Either no tabbable items or the end of the document
  if (!next) {

    // If we've reached the end of the content in this document, we
    // focus the document itself before leaving.

    // Only do this if we're not in document-before-content focus mode

    if (!docHasFocus && !doFocusAvailDocShells && !docFocusFirst) {
      PRBool focusDoc = PR_TRUE;

      nsCOMPtr<nsIDocShell> docShell;
      nsCOMPtr<nsIDocShell> parentDS;
      if (NS_SUCCEEDED(GetDocShellsFromDoc(mDocument, getter_AddRefs(docShell), getter_AddRefs(parentDS)))) {
        if (docType == eFrame) {
          if (FocusAfterHTMLFrameDoc(docShell, parentDS, forward)) {
            return;
          }
        } else if (docType == eIFrame) {
          if (FocusAfterHTMLIFrameDoc(docShell, parentDS, forward, focusDoc)) {
            return;
          }
        }
      }

      // IFrame may or may not want the current document focused
      if (focusDoc) {
        nsCOMPtr<nsIScriptGlobalObject> sgo;
        mDocument->GetScriptGlobalObject(getter_AddRefs(sgo));
        nsCOMPtr<nsIDOMWindowInternal> domwin(do_QueryInterface(sgo));
        if (domwin) {
          SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
          if (NS_SUCCEEDED(domwin->Focus()))
            return;
        }
      }
    }

    PRBool focusTaken = PR_FALSE;
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    //Offer focus upwards to allow shifting focus to UI controls
    nsCOMPtr<nsISupports> container;
    mPresContext->GetContainer(getter_AddRefs(container));
    nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(container));
    if (docShellAsWin) {
      docShellAsWin->FocusAvailable(docShellAsWin, forward, &focusTaken);
#ifdef DEBUG_DOCSHELL_FOCUS
      nsCOMPtr<nsIDocShellTreeNode> node = do_QueryInterface(container);
      PrintDocTree(node);
#endif
    }
      
    //No one took focus and we're not already at the top of the doc
    //so calling ShiftFocus will start at the top of the doc again.
    if (!focusTaken && !docHasFocus) {
      ShiftFocus(forward);
    }

    return;
  } else {
    // The "next" piece of content to be focused is an IFrame,
    // so we need to "walk" into the new HTMLIFrame docShell
    nsCOMPtr<nsIDOMHTMLIFrameElement> iframe(do_QueryInterface(next));
    if (iframe) {
      if (FocusWithinHTMLIFrameDoc(next, forward)) {
        return;
      }
    }
  }

#ifdef DEBUG_DOCSHELL_FOCUS
  printf("Shift Focus to Content: %p\n", next.get());
#endif

  // Now we need to get the content node's frame and reset 
  // the mCurrentTarget target.  Otherwise the focus
  // code will be slightly out of sync (with regards to
  // focusing widgets within the current target)
  if (shell)
    shell->GetPrimaryFrameFor(next, &mCurrentTarget);
  
  ChangeFocus(next, mCurrentTarget, PR_TRUE);

  NS_IF_RELEASE(mCurrentFocus);
  mCurrentFocus = next;
  NS_IF_ADDREF(mCurrentFocus);

  if (docShell)
    docShell->SetCanvasHasFocus(PR_FALSE);

  if (hadDocFocus)
    ForceUpdate(docShell);
}

NS_IMETHODIMP
nsEventStateManager::GetNextTabbableContent(nsIContent* aRootContent, nsIFrame* aFrame, PRBool forward,
                                            nsIContent** aResult)
{
  *aResult = nsnull;
  PRBool keepFirstFrame = PR_FALSE;

  nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;

  if (!aFrame) {
    //No frame means we need to start with the root content again.
    nsCOMPtr<nsIPresShell> presShell;
    if (mPresContext) {
      nsIFrame* result = nsnull;
      if (NS_SUCCEEDED(mPresContext->GetShell(getter_AddRefs(presShell))) && presShell) {
        presShell->GetPrimaryFrameFor(aRootContent, &result);
      }
      if (result) {
        while(NS_SUCCEEDED(result->FirstChild(mPresContext, nsnull, &result)) && result) {
          aFrame = result;
        }
      }
    }
    if (!aFrame) {
      return NS_ERROR_FAILURE;
    }
    keepFirstFrame = PR_TRUE;
  }

  //Need to do special check in case we're in an imagemap which has multiple content per frame
  if (mCurrentFocus) {
    nsCOMPtr<nsIAtom> tag;
    mCurrentFocus->GetTag(*getter_AddRefs(tag));
    if(nsHTMLAtoms::area==tag.get()) {
      //Focus is in an imagemap area
      nsCOMPtr<nsIPresShell> presShell;
      if (mPresContext) {
        nsIFrame* result = nsnull;
        if (NS_SUCCEEDED(mPresContext->GetShell(getter_AddRefs(presShell))) && presShell) {
          presShell->GetPrimaryFrameFor(mCurrentFocus, &result);
        }
        if (result == aFrame) {
          //The current focus map area is in the current frame, don't skip over it.
          keepFirstFrame = PR_TRUE;
        }
      }
    }
  }

  nsresult result;
  nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID,&result));
  if (NS_FAILED(result))
    return result;

  result = trav->NewFrameTraversal(getter_AddRefs(frameTraversal), EXTENSIVE,
                                   mPresContext, aFrame);
  if (NS_FAILED(result))
    return NS_OK;

  if (!keepFirstFrame) {
    if (forward)
      frameTraversal->Next();
    else frameTraversal->Prev();
  }

  nsISupports* currentItem;
  frameTraversal->CurrentItem(&currentItem);
  nsIFrame* currentFrame = (nsIFrame*)currentItem;

  while (currentFrame) {
    nsCOMPtr<nsIContent> child;
    currentFrame->GetContent(getter_AddRefs(child));

    const nsStyleVisibility* vis;
    currentFrame->GetStyleData(eStyleStruct_Visibility, ((const nsStyleStruct *&)vis));

    const nsStyleUserInterface* ui;
    currentFrame->GetStyleData(eStyleStruct_UserInterface, ((const nsStyleStruct*&)ui));

    PRBool viewShown = PR_TRUE;

    nsIView* frameView = nsnull;
    currentFrame->GetView(mPresContext, &frameView);
    if (!frameView) {
      nsIFrame* parentWithView = nsnull;
      currentFrame->GetParentWithView(mPresContext, &parentWithView);
      if (parentWithView)
        parentWithView->GetView(mPresContext, &frameView);
    }
    while (frameView) {
      nsViewVisibility visib;
      frameView->GetVisibility(visib);
      if (visib == nsViewVisibility_kHide) {
        viewShown = PR_FALSE;
        break;
      }
      frameView->GetParent(frameView);
    }

    nsCOMPtr<nsIDOMElement> element(do_QueryInterface(child));

    // if collapsed or hidden, we don't get tabbed into.
    if (viewShown &&
        (vis->mVisible != NS_STYLE_VISIBILITY_COLLAPSE) &&
        (vis->mVisible != NS_STYLE_VISIBILITY_HIDDEN) && 
        (ui->mUserFocus != NS_STYLE_USER_FOCUS_IGNORE) &&
        (ui->mUserFocus != NS_STYLE_USER_FOCUS_NONE) && element) {
      nsCOMPtr<nsIAtom> tag;
      PRInt32 tabIndex = -1;
      PRBool disabled = PR_TRUE;
      PRBool hidden = PR_FALSE;

      child->GetTag(*getter_AddRefs(tag));
      nsCOMPtr<nsIDOMHTMLElement> htmlElement(do_QueryInterface(child));
      if (htmlElement) {
        if (nsHTMLAtoms::input==tag.get()) {
          nsCOMPtr<nsIDOMHTMLInputElement> nextInput(do_QueryInterface(child));
          if (nextInput) {
            nextInput->GetDisabled(&disabled);
            nextInput->GetTabIndex(&tabIndex);

            nsAutoString type;
            nextInput->GetType(type);
            if (type.EqualsIgnoreCase("hidden")) {
              hidden = PR_TRUE;
            }
          }
        }
        else if (nsHTMLAtoms::select==tag.get()) {
          nsCOMPtr<nsIDOMHTMLSelectElement> nextSelect(do_QueryInterface(child));
          if (nextSelect) {
            nextSelect->GetDisabled(&disabled);
            nextSelect->GetTabIndex(&tabIndex);
          }
        }
        else if (nsHTMLAtoms::textarea==tag.get()) {
          nsCOMPtr<nsIDOMHTMLTextAreaElement> nextTextArea(do_QueryInterface(child));
          if (nextTextArea) {
            nextTextArea->GetDisabled(&disabled);
            nextTextArea->GetTabIndex(&tabIndex);
          }
        }
        else if(nsHTMLAtoms::a==tag.get()) {
          nsCOMPtr<nsIDOMHTMLAnchorElement> nextAnchor(do_QueryInterface(child));
          if (nextAnchor)
            nextAnchor->GetTabIndex(&tabIndex);
          nsAutoString href;
          nextAnchor->GetAttribute(NS_LITERAL_STRING("href"), href);
          if (!href.Length()) {
            disabled = PR_TRUE; // Don't tab unless href, bug 17605
          } else {
            disabled = PR_FALSE;
          }
        }
        else if(nsHTMLAtoms::button==tag.get()) {
          nsCOMPtr<nsIDOMHTMLButtonElement> nextButton(do_QueryInterface(child));
          if (nextButton) {
            nextButton->GetTabIndex(&tabIndex);
            nextButton->GetDisabled(&disabled);
          }
        }
        else if(nsHTMLAtoms::img==tag.get()) {
          nsCOMPtr<nsIDOMHTMLImageElement> nextImage(do_QueryInterface(child));
          nsAutoString usemap;
          if (nextImage) {
            nsCOMPtr<nsIDocument> doc;
            if (NS_SUCCEEDED(child->GetDocument(*getter_AddRefs(doc))) && doc) {
              nextImage->GetAttribute(NS_LITERAL_STRING("usemap"), usemap);
              nsCOMPtr<nsIDOMHTMLMapElement> imageMap;
              if (NS_SUCCEEDED(nsImageMapUtils::FindImageMap(doc,usemap,getter_AddRefs(imageMap))) && imageMap) {
                nsCOMPtr<nsIContent> map(do_QueryInterface(imageMap));
                if (map) {
                  nsCOMPtr<nsIContent> childArea;
                  PRInt32 count, index;
                  map->ChildCount(count);
                  //First see if mCurrentFocus is in this map
                  for (index = 0; index < count; index++) {
                    map->ChildAt(index, *getter_AddRefs(childArea));
                    if (childArea.get() == mCurrentFocus) {
                      nsAutoString tabIndexStr;
                      childArea->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
                      PRInt32 val = 0;
                      if (!tabIndexStr.IsEmpty()) {
                        PRInt32 ec, tabIndexVal = tabIndexStr.ToInteger(&ec);
                        if (NS_SUCCEEDED(ec)) {
                          val = tabIndexVal;
                        }
                      }
                      if (mCurrentTabIndex == val) {
                        //mCurrentFocus is in this map so we must start iterating past it.
                        //We skip the case where mCurrentFocus has the same tab index
                        //as mCurrentTabIndex since the next tab ordered element might
                        //be before it (or after for backwards) in the child list.
                        break;
                      }
                    }
                  }
                  PRInt32 increment = forward ? 1 : - 1;
                  PRInt32 start = index < count ? index + increment : (forward ? 0 : count - 1);
                  for (index = start; index < count && index >= 0; index += increment) {
                    //Iterate over the children.
                    map->ChildAt(index, *getter_AddRefs(childArea));

                    //Got the map area, check its tabindex.
                    nsAutoString tabIndexStr;
                    childArea->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
                    PRInt32 val = 0;
                    if (!tabIndexStr.IsEmpty()) {
                      PRInt32 ec, tabIndexVal = tabIndexStr.ToInteger(&ec);
                      if (NS_SUCCEEDED(ec)) {
                        val = tabIndexVal;
                      }
                    }
                    if (mCurrentTabIndex == val) {
                      //tabindex == the current one, use it.
                      *aResult = childArea;
                      NS_IF_ADDREF(*aResult);
                      return NS_OK;
                    }
                  }
                }
              }
            }
          }
        }
        else if(nsHTMLAtoms::object==tag.get()) {
          nsCOMPtr<nsIDOMHTMLObjectElement> nextObject(do_QueryInterface(child));
          if (nextObject) 
            nextObject->GetTabIndex(&tabIndex);
          disabled = PR_FALSE;
        }
        else if (nsHTMLAtoms::iframe==tag.get()) {
          nsCOMPtr<nsIDOMHTMLIFrameElement> iframe(do_QueryInterface(child));
          if (iframe) {
            disabled = PR_FALSE;
          }
        }
      }
      else {
        nsAutoString value;
        child->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, value);
        nsAutoString tabStr;
        child->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::tabindex, tabStr);
        if (!tabStr.IsEmpty()) {
          PRInt32 errorCode;
          tabIndex = tabStr.ToInteger(&errorCode);
        }
        if (!value.EqualsWithConversion("true"))
          disabled = PR_FALSE;
      }
      
      //TabIndex not set (-1) treated at same level as set to 0
      tabIndex = tabIndex < 0 ? 0 : tabIndex;

      if (!disabled && !hidden && mCurrentTabIndex == tabIndex && child.get() != mCurrentFocus) {
        *aResult = child;
        NS_IF_ADDREF(*aResult);
        return NS_OK;
      }
    }

    if (forward)
      frameTraversal->Next();
    else frameTraversal->Prev();

    frameTraversal->CurrentItem(&currentItem);
    currentFrame = (nsIFrame*)currentItem;
  }

  // Reached end or beginning of document
  //If already at lowest priority tab (0), end
  if (((forward) && (0 == mCurrentTabIndex)) ||
      ((!forward) && (1 == mCurrentTabIndex))) {
    return NS_OK;
  }
  //else continue looking for next highest priority tab
  mCurrentTabIndex = GetNextTabIndex(aRootContent, forward);
  return GetNextTabbableContent(aRootContent, nsnull, forward, aResult);
}

PRInt32
nsEventStateManager::GetNextTabIndex(nsIContent* aParent, PRBool forward)
{
  PRInt32 count, tabIndex, childTabIndex;
  nsCOMPtr<nsIContent> child;
  
  aParent->ChildCount(count);
 
  if (forward) {
    tabIndex = 0;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, *getter_AddRefs(child));
      childTabIndex = GetNextTabIndex(child, forward);
      if (childTabIndex > mCurrentTabIndex && childTabIndex != tabIndex) {
        tabIndex = (tabIndex == 0 || childTabIndex < tabIndex) ? childTabIndex : tabIndex; 
      }
      
      nsAutoString tabIndexStr;
      child->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_OK == ec && val > mCurrentTabIndex && val != tabIndex) {
        tabIndex = (tabIndex == 0 || val < tabIndex) ? val : tabIndex; 
      }
    }
  } 
  else { /* !forward */
    tabIndex = 1;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, *getter_AddRefs(child));
      childTabIndex = GetNextTabIndex(child, forward);
      if ((mCurrentTabIndex==0 && childTabIndex > tabIndex) ||
          (childTabIndex < mCurrentTabIndex && childTabIndex > tabIndex)) {
        tabIndex = childTabIndex;
      }
      
      nsAutoString tabIndexStr;
      child->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_OK == ec) {
        if ((mCurrentTabIndex==0 && val > tabIndex) ||
            (val < mCurrentTabIndex && val > tabIndex) ) {
          tabIndex = val;
        }
      }
    }
  }
  return tabIndex;
}

NS_IMETHODIMP 
nsEventStateManager::GetNextTabbableIndexContent(nsIContent* aRootContent, 
                                                 PRBool forward, 
                                                 PRBool aStartOver,
                                                 nsIContent** aResult)
{
  if (forward) {
    if (aStartOver) {
      mCurrentTabIndex = 0;
    }
    mCurrentTabIndex = GetNextTabIndex(aRootContent, forward);
    if (mCurrentTabIndex > 0) {
      return GetNextTabbableContent(aRootContent, nsnull, forward, aResult);
    }
  } else {
    nsCOMPtr<nsIContent> lastContent = getter_AddRefs(GetLastContent(aRootContent));
    if (lastContent) {
      MoveFocus(PR_FALSE, lastContent);
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTarget(nsIFrame **aFrame)
{
  if (!mCurrentTarget && mCurrentTargetContent) {
    nsCOMPtr<nsIPresShell> shell;
    if (mPresContext) {
      nsresult rv = mPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        shell->GetPrimaryFrameFor(mCurrentTargetContent, &mCurrentTarget);
      }
    }
  }

  if (!mCurrentTarget) {
	  nsCOMPtr<nsIPresShell> presShell;
	  mPresContext->GetShell(getter_AddRefs(presShell));
	  if (presShell) {
	    presShell->GetEventTargetFrame(&mCurrentTarget);
	  }
  }

  *aFrame = mCurrentTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::HasPositiveTabIndex(nsIContent* aContent, 
                                         PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aContent);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;

  nsAutoString tabIndexStr;
  aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndexStr);
  if (!tabIndexStr.IsEmpty()) {
    PRInt32 ec, tabIndexVal = tabIndexStr.ToInteger(&ec);
    if (NS_SUCCEEDED(ec)) {
      if (tabIndexVal > 0) {
        *aResult = PR_TRUE;
        return NS_OK;
      }
    }
  }

  PRInt32 count;
  aContent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsIContent* child;
    aContent->ChildAt(i, child);
    if (NS_SUCCEEDED(HasPositiveTabIndex(child, aResult))) {
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent)
{
  if (aEvent &&
      (aEvent->message == NS_FOCUS_CONTENT ||
       aEvent->message == NS_BLUR_CONTENT)) {
    *aContent = mCurrentFocus;
    NS_IF_ADDREF(*aContent);
    return NS_OK;
  }

  if (mCurrentTargetContent) {
    *aContent = mCurrentTargetContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;
  }

  if (!mCurrentTarget) {
	  nsCOMPtr<nsIPresShell> presShell;
	  mPresContext->GetShell(getter_AddRefs(presShell));
	  if (presShell) {
	    presShell->GetEventTargetFrame(&mCurrentTarget);
	  }
  }

  if (mCurrentTarget) {
    mCurrentTarget->GetContentForEvent(mPresContext, aEvent, aContent);
    return NS_OK;
  }

  *aContent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetEventRelatedContent(nsIContent** aContent)
{
  if (mCurrentRelatedContent) {
    *aContent = mCurrentRelatedContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;      
  }
  
  *aContent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetContentState(nsIContent *aContent, PRInt32& aState)
{
  aState = NS_EVENT_STATE_UNSPECIFIED;

  if (aContent == mActiveContent) {
    aState |= NS_EVENT_STATE_ACTIVE;
  }
  if (hHover) {
    //If using hierchical hover check the ancestor chain of mHoverContent
    //to see if we are on it.
    nsCOMPtr<nsIContent> parent = mHoverContent;
    nsIContent* child;
    while (parent) {
	    if (aContent == parent.get()) {
	      aState |= NS_EVENT_STATE_HOVER;
        break;
	    }
      child = parent;
      child->GetParent(*getter_AddRefs(parent));
    }
  }
  else {
    if (aContent == mHoverContent) {
      aState |= NS_EVENT_STATE_HOVER;
    }
  }

  if (aContent == mCurrentFocus) {
    aState |= NS_EVENT_STATE_FOCUS;
  }
  if (aContent == mDragOverContent) {
    aState |= NS_EVENT_STATE_DRAGOVER;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SetContentState(nsIContent *aContent, PRInt32 aState)
{
  const PRInt32 maxNotify = 5;
  nsIContent  *notifyContent[maxNotify] = {nsnull, nsnull, nsnull, nsnull, nsnull};

  // check to see that this state is allowed by style. Check dragover too?
  if (mCurrentTarget && (aState == NS_EVENT_STATE_ACTIVE || aState == NS_EVENT_STATE_HOVER))
  {
    const nsStyleUserInterface* ui;
    mCurrentTarget->GetStyleData(eStyleStruct_UserInterface, ((const nsStyleStruct*&)ui));
    if (ui->mUserInput == NS_STYLE_USER_INPUT_NONE)
      return NS_OK;
  }
  
  if ((aState & NS_EVENT_STATE_DRAGOVER) && (aContent != mDragOverContent)) {
    //transferring ref to notifyContent from mDragOverContent
    notifyContent[4] = mDragOverContent; // notify dragover first, since more common case
    mDragOverContent = aContent;
    NS_IF_ADDREF(mDragOverContent);
  }

  if ((aState & NS_EVENT_STATE_ACTIVE) && (aContent != mActiveContent)) {
    //transferring ref to notifyContent from mActiveContent
    notifyContent[2] = mActiveContent;
    mActiveContent = aContent;
    NS_IF_ADDREF(mActiveContent);
  }

  nsCOMPtr<nsIContent> newHover = nsnull;
  nsCOMPtr<nsIContent> oldHover = nsnull;
  nsCOMPtr<nsIContent> commonHoverParent = nsnull;
  if ((aState & NS_EVENT_STATE_HOVER) && (aContent != mHoverContent)) {
    if (hHover) {
      nsCOMPtr<nsIDocument> doc;
      if (aContent && NS_SUCCEEDED(aContent->GetDocument(*getter_AddRefs(doc))) && doc) {
        newHover = aContent;
      }
      if (mHoverContent && NS_SUCCEEDED(mHoverContent->GetDocument(*getter_AddRefs(doc))) && doc) {
        oldHover = mHoverContent;
      }

      //Find closest common parent
      nsCOMPtr<nsIContent> parent1 = mHoverContent;
      nsCOMPtr<nsIContent> parent2;
      if (mHoverContent && aContent) {
        while (parent1) {
          parent2 = aContent;
          while (parent2) {
	          if (parent1 == parent2) {
              commonHoverParent = parent1;
              break;
            }
            nsIContent* child2 = parent2;
            child2->GetParent(*getter_AddRefs(parent2));
	        }
          if (commonHoverParent) {
            break;
          }
          nsIContent* child1 = parent1;
          child1->GetParent(*getter_AddRefs(parent1));
        }
      }
      //If new hover content is null we get the top parent of mHoverContent
      else if (mHoverContent) {
        nsCOMPtr<nsIContent> parent = mHoverContent;
        nsIContent* child = nsnull;
        while (parent) {
          child = parent;
          child->GetParent(*getter_AddRefs(parent));
        }
        commonHoverParent = child;
      }
      //Else if old hover content is null we get the top parent of aContent
      else {
        nsCOMPtr<nsIContent> parent = aContent;
        nsIContent* child = nsnull;
        while (parent) {
          child = parent;
          child->GetParent(*getter_AddRefs(parent));
        }
        commonHoverParent = child;
      }
      NS_IF_RELEASE(mHoverContent);
    }
    else {
      //transferring ref to notifyContent from mHoverContent
      notifyContent[1] = mHoverContent; // notify hover first, since more common case
    }
    mHoverContent = aContent;
    NS_IF_ADDREF(mHoverContent);
  }

  if ((aState & NS_EVENT_STATE_FOCUS)) {
    if (aContent && (aContent == mCurrentFocus) && gLastFocusedDocument == mDocument) {
      // gLastFocusedDocument appears to always be correct, that is why
      // I'm not setting it here. This is to catch an edge case.
      NS_IF_RELEASE(gLastFocusedContent);
      gLastFocusedContent = mCurrentFocus;
      NS_IF_ADDREF(gLastFocusedContent);
      //If this notification was for focus alone then get rid of aContent
      //ref to avoid unnecessary notification.
      if (!(aState & ~NS_EVENT_STATE_FOCUS)) {
        aContent = nsnull;
      }
    } else {
      notifyContent[3] = gLastFocusedContent;
      NS_IF_ADDREF(gLastFocusedContent);
      SendFocusBlur(mPresContext, aContent);
    }
  }

  if (aContent) { // notify about new content too
    notifyContent[0] = aContent;
    NS_ADDREF(aContent);  // everything in notify array has a ref
  }

  // remove duplicates
  if ((notifyContent[4] == notifyContent[3]) || (notifyContent[4] == notifyContent[2]) || (notifyContent[4] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[4]);
  }
  // remove duplicates
  if ((notifyContent[3] == notifyContent[2]) || (notifyContent[3] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[3]);
  }
  if (notifyContent[2] == notifyContent[1]) {
    NS_IF_RELEASE(notifyContent[2]);
  }

  // remove notifications for content not in document.
  // we may decide this is possible later but right now it has problems.
  nsIDocument* doc = nsnull;
  for  (int i = 0; i < maxNotify; i++) {
    if (notifyContent[i] && NS_SUCCEEDED(notifyContent[i]->GetDocument(doc)) && !doc) {
      NS_RELEASE(notifyContent[i]);
    }
    NS_IF_RELEASE(doc);
  }

  // compress the notify array to group notifications tighter
  nsIContent** from = &(notifyContent[0]);
  nsIContent** to   = &(notifyContent[0]);
  nsIContent** end  = &(notifyContent[maxNotify]);

  while (from < end) {
    if (! *from) {
      while (++from < end) {
        if (*from) {
          *to++ = *from;
          *from = nsnull;
          break;
        }
      }
    }
    else {
      if (from == to) {
        to++;
        from++;
      }
      else {
        *to++ = *from;
        *from++ = nsnull;
      }
    }
  }

  if (notifyContent[0] || newHover || oldHover) { // have at least one to notify about
    nsCOMPtr<nsIDocument> doc1, doc2;  // this presumes content can't get/lose state if not connected to doc
    if (notifyContent[0]) {
      notifyContent[0]->GetDocument(*getter_AddRefs(doc1));
      if (notifyContent[1]) {
        //For :focus this might be a different doc so check
        notifyContent[1]->GetDocument(*getter_AddRefs(doc2));
        if (doc1 == doc2) {
          doc2 = nsnull;
        }
      }
    }
    else if (newHover) {
      newHover->GetDocument(*getter_AddRefs(doc1));
    }
    else {
      oldHover->GetDocument(*getter_AddRefs(doc2));
    }
    if (doc1) {
      doc1->BeginUpdate();

      //Notify all content from newHover to the commonHoverParent
      if (newHover) {
        nsCOMPtr<nsIContent> parent;
        newHover->GetParent(*getter_AddRefs(parent));
        doc1->ContentStatesChanged(newHover, parent);
        while (parent && parent != commonHoverParent) {
          parent->GetParent(*getter_AddRefs(newHover));
          if (newHover && newHover != commonHoverParent) {
            newHover->GetParent(*getter_AddRefs(parent));
            if (parent == commonHoverParent) {
              doc1->ContentStatesChanged(newHover, nsnull);
            }
            else {
              doc1->ContentStatesChanged(newHover, parent);
            }
          }
          else {
            break;
          }
        }
      }
      //Notify all content from oldHover to the commonHoverParent
      if (oldHover) {
        nsCOMPtr<nsIContent> parent;
        oldHover->GetParent(*getter_AddRefs(parent));
        doc1->ContentStatesChanged(oldHover, parent);
        while (parent && parent != commonHoverParent) {
          parent->GetParent(*getter_AddRefs(oldHover));
          if (oldHover && oldHover != commonHoverParent) {
            oldHover->GetParent(*getter_AddRefs(parent));
            if (parent == commonHoverParent) {
              doc1->ContentStatesChanged(oldHover, nsnull);
            }
            else {
              doc1->ContentStatesChanged(oldHover, parent);
            }
          }
          else {
            break;
          }
        }
      }

      doc1->ContentStatesChanged(notifyContent[0], notifyContent[1]);
      if (notifyContent[2]) {  // more that two notifications are needed (should be rare)
        // XXX a further optimization here would be to group the notification pairs
        // together by parent/child, only needed if more than two content changed
        // (ie: if [0] and [2] are parent/child, then notify (0,2) (1,3))
        doc1->ContentStatesChanged(notifyContent[2], notifyContent[3]);
        if (notifyContent[4]) {  // more that two notifications are needed (should be rare)
          doc1->ContentStatesChanged(notifyContent[4], nsnull);
        }
      }
      doc1->EndUpdate();

      if (doc2) {
        doc2->BeginUpdate();
        doc2->ContentStatesChanged(notifyContent[1], notifyContent[2]);
        if (notifyContent[3]) {
          doc1->ContentStatesChanged(notifyContent[3], notifyContent[4]);
        }
        doc2->EndUpdate();
      }
    }

    from = &(notifyContent[0]);
    while (from < to) {  // release old refs now that we are through
      nsIContent* notify = *from++;
      NS_RELEASE(notify);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SendFocusBlur(nsIPresContext* aPresContext, nsIContent *aContent)
{
  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  
  if (nsnull != gLastFocusedPresContext) {
    
    if (gLastFocusedContent && gLastFocusedContent != mFirstBlurEvent) {

      //Store the first blur event we fire and don't refire blur
      //to that element while the first blur is still ongoing.
      PRBool clearFirstBlurEvent = PR_FALSE;
      if (!mFirstBlurEvent) {
        mFirstBlurEvent = gLastFocusedContent;
        NS_ADDREF(mFirstBlurEvent);
        clearFirstBlurEvent = PR_TRUE;
      }

      // Retrieve this content node's pres context. it can be out of sync in
      // the Ender widget case.
      nsCOMPtr<nsIDocument> doc;
      gLastFocusedContent->GetDocument(*getter_AddRefs(doc));
      if (doc) {
        // The order of the nsIViewManager and nsIPresShell COM pointers is
        // important below.  We want the pres shell to get released before the
        // associated view manager on exit from this function.
        // See bug 53763.
        nsCOMPtr<nsIViewManager> kungFuDeathGrip;
        nsCOMPtr<nsIPresShell> shell;
        doc->GetShellAt(0, getter_AddRefs(shell));
        if (shell) {
          shell->GetViewManager(getter_AddRefs(kungFuDeathGrip));

          nsCOMPtr<nsIPresContext> oldPresContext;
          shell->GetPresContext(getter_AddRefs(oldPresContext));

          //fire blur
          nsEventStatus status = nsEventStatus_eIgnore;
          nsEvent event;
          event.eventStructType = NS_EVENT;
          event.message = NS_BLUR_CONTENT;
          
          EnsureDocument(presShell);
          
          // Make sure we're not switching command dispatchers, if so, surpress the blurred one
          if(gLastFocusedDocument && mDocument) {
            nsCOMPtr<nsIFocusController> newFocusController;
            nsCOMPtr<nsIFocusController> oldFocusController;
            nsCOMPtr<nsPIDOMWindow> oldPIDOMWindow;
            nsCOMPtr<nsPIDOMWindow> newPIDOMWindow;
            nsCOMPtr<nsIScriptGlobalObject> oldGlobal;
            nsCOMPtr<nsIScriptGlobalObject> newGlobal;
            gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(oldGlobal));
            mDocument->GetScriptGlobalObject(getter_AddRefs(newGlobal));
            nsCOMPtr<nsPIDOMWindow> newWindow = do_QueryInterface(newGlobal);
            nsCOMPtr<nsPIDOMWindow> oldWindow = do_QueryInterface(oldGlobal);
            if(newWindow)
              newWindow->GetRootFocusController(getter_AddRefs(newFocusController));
            if(oldWindow)
			  oldWindow->GetRootFocusController(getter_AddRefs(oldFocusController));
            if(oldFocusController && oldFocusController != newFocusController)
              oldFocusController->SetSuppressFocus(PR_TRUE, "SendFocusBlur Window Switch");
          }
          
          nsCOMPtr<nsIEventStateManager> esm;
          oldPresContext->GetEventStateManager(getter_AddRefs(esm));
          esm->SetFocusedContent(gLastFocusedContent);
          nsCOMPtr<nsIContent> temp = gLastFocusedContent;
          NS_RELEASE(gLastFocusedContent);
          gLastFocusedContent = nsnull;
          temp->HandleDOMEvent(oldPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
          esm->SetFocusedContent(nsnull);
        }
      }

      if (clearFirstBlurEvent) {
        NS_RELEASE(mFirstBlurEvent);
      }
    }

    // Go ahead and fire a blur on the window.
    nsCOMPtr<nsIScriptGlobalObject> globalObject;

    if(gLastFocusedDocument)
      gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));

    EnsureDocument(presShell);

    if (gLastFocusedDocument && (gLastFocusedDocument != mDocument) && globalObject) {  
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_BLUR_CONTENT;

	  	  // Make sure we're not switching command dispatchers, if so, surpress the blurred one
		  if(mDocument) {
		    nsCOMPtr<nsIFocusController> newFocusController;
            nsCOMPtr<nsIFocusController> oldFocusController;
		    nsCOMPtr<nsPIDOMWindow> oldPIDOMWindow;
		    nsCOMPtr<nsPIDOMWindow> newPIDOMWindow;
		    nsCOMPtr<nsIScriptGlobalObject> oldGlobal;
		    nsCOMPtr<nsIScriptGlobalObject> newGlobal;
		    gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(oldGlobal));
		    mDocument->GetScriptGlobalObject(getter_AddRefs(newGlobal));
            nsCOMPtr<nsPIDOMWindow> newWindow = do_QueryInterface(newGlobal);
		    nsCOMPtr<nsPIDOMWindow> oldWindow = do_QueryInterface(oldGlobal);

        if (newWindow)
          newWindow->GetRootFocusController(getter_AddRefs(newFocusController));
		    oldWindow->GetRootFocusController(getter_AddRefs(oldFocusController));
            if(oldFocusController && oldFocusController != newFocusController)
			  oldFocusController->SetSuppressFocus(PR_TRUE, "SendFocusBlur Window Switch #2");
		  }

      nsCOMPtr<nsIEventStateManager> esm;
      gLastFocusedPresContext->GetEventStateManager(getter_AddRefs(esm));
      esm->SetFocusedContent(nsnull);
      nsCOMPtr<nsIDocument> temp = gLastFocusedDocument;
      NS_RELEASE(gLastFocusedDocument);
      gLastFocusedDocument = nsnull;
      temp->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
      globalObject->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
    }
  }
  
  NS_IF_RELEASE(gLastFocusedContent);
  NS_IF_RELEASE(mCurrentFocus);
  gLastFocusedContent = aContent;
  mCurrentFocus = aContent;
  NS_IF_ADDREF(aContent);
  NS_IF_ADDREF(aContent);
 
  if (nsnull != aContent && aContent != mFirstFocusEvent) {

    //Store the first focus event we fire and don't refire focus
    //to that element while the first focus is still ongoing.
    PRBool clearFirstFocusEvent = PR_FALSE;
    if (!mFirstFocusEvent) {
      mFirstFocusEvent = aContent;
      NS_ADDREF(mFirstFocusEvent);
      clearFirstFocusEvent = PR_TRUE;
    }

    //fire focus
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FOCUS_CONTENT;

    if (nsnull != mPresContext) {
      aContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
    }
    
    nsAutoString tabIndex;
    aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndex);
    PRInt32 ec, val = tabIndex.ToInteger(&ec);
    if (NS_OK == ec) {
      mCurrentTabIndex = val;
    }

    if (clearFirstFocusEvent) {
      NS_RELEASE(mFirstFocusEvent);
    }
  }

  nsIFrame * currentFocusFrame = nsnull;
  if (mCurrentFocus)
    presShell->GetPrimaryFrameFor(mCurrentFocus, &currentFocusFrame);
  if (!currentFocusFrame)
    currentFocusFrame = mCurrentTarget;

  // Find the window that this frame is in and
  // make sure it has focus
  if (currentFocusFrame) {
    nsIFrame * parentFrame;
    currentFocusFrame->GetParentWithView(aPresContext, &parentFrame);
    if (nsnull != parentFrame) {
      nsIView * pView;
      parentFrame->GetView(aPresContext, &pView);
      if (nsnull != pView) {
        nsIWidget *window = nsnull;

        nsIView *ancestor = pView;
        while (nsnull != ancestor) {
          ancestor->GetWidget(window); // addrefs
          if (nsnull != window) {
              window->SetFocus();
              NS_RELEASE(window); // releases. Duh.
	          break;
	        }
	        ancestor->GetParent(ancestor);
        }
      }
    }
  }

  // For accessibility.browsewithcaret option, we must make the caret visible in text nodes
  if (presShell && mBrowseWithCaret) 
    EnsureCaretVisible(presShell, aContent);

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetFocusedContent(nsIContent** aContent)
{
  *aContent = mCurrentFocus;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SetFocusedContent(nsIContent* aContent)
{
  NS_IF_RELEASE(mCurrentFocus);
  mCurrentFocus = aContent;
  NS_IF_ADDREF(mCurrentFocus);
  return NS_OK;
}

//-------------------------------------------
// Access Key Registration
//-------------------------------------------
NS_IMETHODIMP
nsEventStateManager::RegisterAccessKey(nsIFrame * aFrame, nsIContent* aContent, PRUint32 aKey)
{
  if (!mAccessKeys) {
    mAccessKeys = new nsSupportsHashtable();
    if (!mAccessKeys) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIContent> content;
  if (!aContent) {
    aFrame->GetContent(getter_AddRefs(content));
  }
  else {
    content = aContent;
  }

  if (content) {
    nsAutoString accKey((char)aKey);
    accKey.ToLowerCase();

    nsVoidKey key((void*)accKey.First());

    mAccessKeys->Put(&key, content);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::UnregisterAccessKey(nsIFrame * aFrame, nsIContent* aContent, PRUint32 aKey)
{
  if (!mAccessKeys) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIContent> content;
  if (!aContent) {
    aFrame->GetContent(getter_AddRefs(content));
  }
  else {
    content = aContent;
  }
  if (content) {
    nsAutoString accKey((char)aKey);
    accKey.ToLowerCase();

    nsVoidKey key((void*)accKey.First());

    nsCOMPtr<nsIContent> oldContent = getter_AddRefs(NS_STATIC_CAST(nsIContent*, mAccessKeys->Get(&key)));
    if (oldContent != content) {
      return NS_OK;
    }
    mAccessKeys->Remove(&key);
  }
  return NS_OK;
}

void nsEventStateManager::ForceViewUpdate(nsIView* aView)
{
  // force the update to happen now, otherwise multiple scrolls can
  // occur before the update is processed. (bug #7354)

  nsIViewManager* vm = nsnull;
  if (NS_OK == aView->GetViewManager(vm) && nsnull != vm) {
    // I'd use Composite here, but it doesn't always work.
    // vm->Composite();
    vm->ForceUpdate();
    NS_RELEASE(vm);
  }
}

NS_IMETHODIMP
nsEventStateManager::DispatchNewEvent(nsISupports* aTarget, nsIDOMEvent* aEvent)
{
  nsresult ret = NS_OK;

  nsCOMPtr<nsIPrivateDOMEvent> privEvt(do_QueryInterface(aEvent));
  if (aEvent) {
    nsEvent* innerEvent;
    privEvt->GetInternalNSEvent(&innerEvent);
    if (innerEvent) {
      nsEventStatus status = nsEventStatus_eIgnore;
      nsCOMPtr<nsIScriptGlobalObject> target(do_QueryInterface(aTarget));
      if (target) {
        ret = target->HandleDOMEvent(mPresContext, innerEvent, &aEvent, NS_EVENT_FLAG_INIT, &status);
      }
      else {
        nsCOMPtr<nsIDocument> target(do_QueryInterface(aTarget));
        if (target) {
          ret = target->HandleDOMEvent(mPresContext, innerEvent, &aEvent, NS_EVENT_FLAG_INIT, &status);
        }
        else {
          nsCOMPtr<nsIContent> target(do_QueryInterface(aTarget));
          if (target) {
            ret = target->HandleDOMEvent(mPresContext, innerEvent, &aEvent, NS_EVENT_FLAG_INIT, &status);
          }
          else {
            nsCOMPtr<nsIChromeEventHandler> target(do_QueryInterface(aTarget));
            if (target) {
              ret = target->HandleChromeEvent(mPresContext, innerEvent, &aEvent, NS_EVENT_FLAG_INIT, &status);
            }
          }
        }
      }
    }
  }
  return ret;
}

void nsEventStateManager::EnsureDocument(nsIPresContext* aPresContext) {
  if (!mDocument) {
    nsCOMPtr<nsIPresShell> presShell;
    aPresContext->GetShell(getter_AddRefs(presShell));
    EnsureDocument(presShell);
  }
}

void nsEventStateManager::EnsureDocument(nsIPresShell* aPresShell) {
  if (!mDocument && aPresShell)
    aPresShell->GetDocument(&mDocument);
}

void nsEventStateManager::FlushPendingEvents(nsIPresContext* aPresContext) {
  NS_PRECONDITION(nsnull != aPresContext, "nsnull ptr");
  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  if (nsnull != shell) {
    shell->FlushPendingNotifications();
    nsCOMPtr<nsIViewManager> viewManager;
    shell->GetViewManager(getter_AddRefs(viewManager));
    if (viewManager) {
      viewManager->FlushPendingInvalidates();
    }
  }
}

nsresult NS_NewEventStateManager(nsIEventStateManager** aInstancePtrResult)
{
  nsresult rv;

  NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIEventStateManager* manager = new nsEventStateManager();
  if (nsnull == manager) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  rv =  manager->QueryInterface(NS_GET_IID(nsIEventStateManager),
                                 (void **) aInstancePtrResult);
  if (NS_FAILED(rv)) return rv;

  return manager->Init();
}


nsresult nsEventStateManager::GetCaretLocation(nsIContent **aCaretContent,
    nsIFrame **aCaretFrame, PRUint32* aCaretOffset)
{
  // In order to return the nsIContent and nsIFrame of the caret's position,
  // we need to get a pres shell, and then get the selection from it

  *aCaretFrame = nsnull; 
  *aCaretContent = nsnull;
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIPresShell> shell;
  if (mPresContext)
    rv = mPresContext->GetShell(getter_AddRefs(shell));

  nsCOMPtr<nsIFrameSelection> selectionFrame;
  if (shell) 
    rv = shell->GetFrameSelection(getter_AddRefs(selectionFrame));

  nsCOMPtr<nsISelection> domSelection;
  if (selectionFrame)
    rv = selectionFrame->GetSelection(nsISelectionController::SELECTION_NORMAL,
      getter_AddRefs(domSelection));

  nsCOMPtr<nsIDOMNode> focusDomNode;
  if (domSelection) {
    rv = domSelection->GetAnchorNode(getter_AddRefs(focusDomNode));
    typedef PRInt32* PRInt32_ptr;
    domSelection->GetAnchorOffset(PRInt32_ptr(aCaretOffset));
    }

  nsCOMPtr<nsIContent> selectionContent(do_QueryInterface(focusDomNode));

  nsIFrame *primaryFrame = nsnull;
  if (selectionContent) 
    rv = shell->GetPrimaryFrameFor(selectionContent, &primaryFrame);

  *aCaretFrame = primaryFrame;
  *aCaretContent = selectionContent;
  NS_ADDREF(*aCaretContent);

  return rv;
}


nsresult nsEventStateManager::MoveFocusToCaret()
{
  // mBrowseWithCaret equals the pref accessibility.browsewithcaret
  // When it's true, the user can arrow around the browser as if it's a
  // read only text editor.
  // If they cursor over a focusable element, then send focus to it
  nsCOMPtr<nsIContent> caretContent;
  nsIFrame *caretFrame;
  PRUint32 caretOffset;
  nsresult rv = GetCaretLocation(getter_AddRefs(caretContent),
    &caretFrame, &caretOffset);

  if (caretContent) {
    // First check to see if our caret is at the very end of a node
    // If so, the caret is actually sitting in front of the next
    // logical frame's primary node - so for this case we need to
    // change caretContent to that node.
    nsCOMPtr<nsIContent> origCaretContent(caretContent);
    nsAutoString nodeValue;
    nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(caretContent));
    domNode->GetNodeValue(nodeValue);

    if (nodeValue.Length() == caretOffset) {
      // Yes, indeed we were at the end of the last node
      nsCOMPtr<nsIBidirectionalEnumerator> frameTraversal;

      nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID,
                                                         &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = trav->NewFrameTraversal(getter_AddRefs(frameTraversal), EXTENSIVE,
                                   mPresContext, caretFrame);
      NS_ENSURE_SUCCESS(rv, rv);

      do {   
        // Get the next logical frame, and set the start of
        // focusable elements. Search for focusable elements from there.
        // Continue getting next frame until the primary node for the frame
        // we are on changes - we don't want to be stuck in the same place
        frameTraversal->Next();
        nsISupports* currentItem;
        frameTraversal->CurrentItem(&currentItem);
        caretFrame = NS_STATIC_CAST(nsIFrame*, currentItem);
        if (caretFrame)
          caretFrame->GetContent(getter_AddRefs(caretContent));
        else break;
      }
      while (caretContent && caretContent == origCaretContent);
    }

    // We now have the correct start node in caretContent!
    // Search for focusable elements, starting with caretContent
    // Keep going up while we look - an ancestory might be focusable

    // jst: We could end the loop earlier, such as when we're no longer
    // in the same frame, by comparing getPrimaryFrameFor(caretContent)
    // with a variable holding the starting caretContent
    nsCOMPtr<nsIAtom> tag;
    while (caretContent && NS_SUCCEEDED(rv)) {
      // Keep testing while caretContent is equal to something,
      // eventually we'll run out of ancestors

      domNode = do_QueryInterface(caretContent);
      if (!domNode) 
        break;

      domNode->GetNodeValue(nodeValue);
      caretContent->GetTag(*getter_AddRefs(tag));
      if (caretContent.get() == mCurrentFocus)
        break;  // already focused on this node, this whole thing's moot

      // For now, all we're going to focus on during this move by
      // cursor is ordinary links
      // Add more focusable tags here later if necessary ... 
      if (nsHTMLAtoms::a == tag.get()) {
        // We are on a link, so change focus to it.
        ChangeFocus(caretContent, caretFrame, PR_TRUE);
        break;
      }
      // Get the parent
      nsIContent* parent;
      caretContent->GetParent(parent);
      caretContent = dont_AddRef(parent);
    }
  }
  return NS_OK;
}



nsresult nsEventStateManager::MoveCaretToFocus()
{
  // If in HTML content and the pref accessibility.browsewithcaret is TRUE,
  // then always move the caret to beginning of a new focus

  if (mBrowseWithCaret && mPresContext) {
    nsCOMPtr<nsIPresShell> shell;
    mPresContext->GetShell(getter_AddRefs(shell));
    if (shell) {
      // rangeDoc is a document interface we can create a range with
      nsCOMPtr<nsIDOMDocumentRange> rangeDoc(do_QueryInterface(mDocument));
      nsCOMPtr<nsIDOMNode> currentFocusNode(do_QueryInterface(mCurrentFocus));
      nsCOMPtr<nsIFrameSelection> frameSelection;
      shell->GetFrameSelection(getter_AddRefs(frameSelection));
      nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(currentFocusNode));

      if (frameSelection && currentFocusNode && rangeDoc && !formControl) {
        nsCOMPtr<nsISelection> domSelection;
        frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL, 
          getter_AddRefs(domSelection));
        if (domSelection) {
          // First clear the selection
          domSelection->RemoveAllRanges();
          nsCOMPtr<nsIDOMRange> newRange;
          nsresult rv = rangeDoc->CreateRange(getter_AddRefs(newRange));
          if (NS_SUCCEEDED(rv)) {
            // If we could create a new range, then set it to the current focus node
            // And then collapse the selection
            newRange->SelectNodeContents(currentFocusNode);
            domSelection->AddRange(newRange);
            domSelection->CollapseToStart();
          }
        }
      }
    }
  }
  return NS_OK;
}


nsresult nsEventStateManager::EnsureCaretVisible(nsIPresShell* aPresShell, nsIContent *aContent)
{ 
  // When browsing with caret, make sure caret is visible after new focus
  nsCOMPtr<nsIFrameSelection> frameSelection;
  aPresShell->GetFrameSelection(getter_AddRefs(frameSelection));
  nsCOMPtr<nsICaret> caret;
  aPresShell->GetCaret(getter_AddRefs(caret));

  if (frameSelection && caret) {
    nsCOMPtr<nsISelection> domSelection;
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL,
      getter_AddRefs(domSelection));

    if (domSelection) {
      nsCOMPtr<nsIDOMNode> focusDomNode;
      domSelection->GetAnchorNode(getter_AddRefs(focusDomNode));
      // first, tell the caret which selection to use
      if (!aContent) {
        caret->SetCaretDOMSelection(domSelection);
        // otherwise, aContent == nsnull, so item is focusable
        // in that case, let focus handlers take care of setting the caret's dom selection
      }
      nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(aPresShell));
      if (!selCon)
        return NS_ERROR_NO_INTERFACE;
      selCon->SetCaretEnabled(PR_TRUE);
      caret->SetCaretVisible(PR_TRUE);

      PRInt32 pixelWidth = 1;
      nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
      if (lookNFeel)
        lookNFeel->GetMetric(nsILookAndFeel::eMetric_MultiLineCaretWidth, pixelWidth);
      caret->SetCaretWidth(pixelWidth);
    }
  }
  return NS_OK;
}

//--------------------------------------------------------------------------------
//-- DocShell Focus Traversal Methods
//--------------------------------------------------------------------------------

//----------------------------------------
// Returns PR_TRUE if this doc contains a frameset
PRBool 
nsEventStateManager::IsFrameSetDoc(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "Pointer is null!");

  nsCOMPtr<nsIDOMHTMLFrameSetElement> frameSet = do_QueryInterface(aContent);
  if (frameSet) {
    return PR_TRUE;
  }

  PRInt32 count;
  aContent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsIContent* child;
    aContent->ChildAt(i, child);
    if (IsFrameSetDoc(child)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

//----------------------------------------
// This method finds the content node in the parent document
// corresponds to the docshell
nsIContent* 
nsEventStateManager::FindContentForDocShell(nsIPresShell* aPresShell,
                                            nsIContent*   aContent,
                                            nsIDocShell*  aDocShell)
{
  NS_ASSERTION(aPresShell, "Pointer is null!");
  NS_ASSERTION(aDocShell,  "Pointer is null!");
  NS_ASSERTION(aContent,   "Pointer is null!");

  nsCOMPtr<nsISupports> supps;
  aPresShell->GetSubShellFor(aContent, getter_AddRefs(supps));
  if (supps) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(supps));
    if (docShell.get() == aDocShell) {
      NS_ADDREF(aContent);
      return aContent;
    }
  }

  // walk children content
  PRInt32 count;
  aContent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsCOMPtr<nsIContent> child;
    aContent->ChildAt(i, *getter_AddRefs(child));
    nsIContent* foundContent = FindContentForDocShell(aPresShell, child, aDocShell);
    if (foundContent != nsnull) {
      return foundContent;
    }
  }
  return nsnull;
}

//----------------------------------------
// This method checks to see if the Content node
// (that is a HTMLFrame, is the last one
PRBool 
nsEventStateManager::IsLastFrameInFrameSet(nsIContent* aLastFrameContent)
{
  NS_ASSERTION(aLastFrameContent, "Pointer is null!");

  nsCOMPtr<nsIContent> parent;
  aLastFrameContent->GetParent(*getter_AddRefs(parent));
  NS_ASSERTION(parent, "parent is null!");

  PRInt32 count;
  parent->ChildCount(count);
  for (PRInt32 i=0;i<count;i++) {
    nsCOMPtr<nsIContent> child;
    parent->ChildAt(i, *getter_AddRefs(child));
    if (child.get() == aLastFrameContent && i == count-1) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


//----------------------------------------
// Identifies the type of document it is
NS_IMETHODIMP 
nsEventStateManager::FigureOutKindOfDoc(nsIDocument* aDoc, eDocType* aDocType)
{
  NS_ENSURE_ARG_POINTER(aDoc);
  NS_ENSURE_ARG_POINTER(aDocType);

  *aDocType = eChrome;

  nsCOMPtr<nsIPresShell> presShell;
  aDoc->GetShellAt(0, getter_AddRefs(presShell));
  if (!presShell) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISupports> pcContainer;
  presContext->GetContainer(getter_AddRefs(pcContainer));
  if (!pcContainer) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(pcContainer));

#ifdef DEBUG_DOCSHELL_FOCUS
  printf("FigureOutKindOfDoc DocShell %p   Doc: %p\n", docShell.get(), aDoc);
#endif

  // Make sure this document is "content" and not "chrome"
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(docShell));
  if (!treeItem) return NS_ERROR_FAILURE;

  PRInt32 type;
  treeItem->GetItemType(&type);
  if (type != nsIDocShellTreeItem::typeContent) {
    return NS_OK;
  }
  *aDocType = eGenericContent;

  // See if we are a frame set
  nsCOMPtr<nsIContent> rootContent;
  aDoc->GetRootContent(getter_AddRefs(rootContent));
  if (IsFrameSetDoc(rootContent)) {
    *aDocType = eFrameSet;
    return NS_OK;
  }

  // So we know we the docShell is content and it isn't
  // a FrameSet, so let's get the parent's doc to see if we 
  // are a FrameSet-Frame or an IFrame

  // Get Parent Doc
  nsCOMPtr<nsIDocShellTreeItem> treeItemParent;
  treeItem->GetParent(getter_AddRefs(treeItemParent));
  if (!treeItemParent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(treeItemParent);
  if (!parentDS) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> parentPresShell;
  parentDS->GetPresShell(getter_AddRefs(parentPresShell));
  if (!parentPresShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> parentDoc;
  parentPresShell->GetDocument(getter_AddRefs(parentDoc));
  if (!parentDoc) return NS_ERROR_FAILURE;

  parentDoc->GetRootContent(getter_AddRefs(rootContent));

  nsCOMPtr<nsIContent> content = getter_AddRefs(FindContentForDocShell(parentPresShell, rootContent, docShell));

  // Is it a FrameSet Frame?
  nsCOMPtr<nsIDOMHTMLFrameElement> frameSetFrame = do_QueryInterface(content);
  if (frameSetFrame) {
    *aDocType = eFrame;
    return NS_OK;
  }

  // Is it an IFrame/
  nsCOMPtr<nsIDOMHTMLIFrameElement> iframe = do_QueryInterface(content);
  if (iframe) {
    *aDocType = eIFrame;
    return NS_OK;
  }
  // Generic Content gets here
  return NS_OK;
}

//-------------------------------------------------------
// This method gets the DocShells for the document and 
// its parent
nsresult 
nsEventStateManager::GetDocShellsFromDoc(nsIDocument* aDocument, 
                                         nsIDocShell** aDocShell, 
                                         nsIDocShell** aParentDS)
{
  NS_ASSERTION(aDocument, "Pointer is null!");
  NS_ASSERTION(aDocShell,  "Pointer is null!");
  NS_ASSERTION(aParentDS,  "Pointer is null!");

  nsCOMPtr<nsIPresShell> presShell;
  aDocument->GetShellAt(0, getter_AddRefs(presShell));
  if (!presShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> pcContainer;
  presContext->GetContainer(getter_AddRefs(pcContainer));
  if (!pcContainer) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docshell(do_QueryInterface(pcContainer));
  if (!docshell) return NS_ERROR_FAILURE;

  *aDocShell = docshell.get();
  NS_ADDREF(*aDocShell);
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(docshell));
  if (!treeItem) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShellTreeItem> treeItemParent;
  treeItem->GetParent(getter_AddRefs(treeItemParent));
  if (!treeItemParent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> parentDS(do_QueryInterface(treeItemParent));
  if (!parentDS) return NS_ERROR_FAILURE;

  *aParentDS = parentDS.get();
  NS_ADDREF(*aParentDS);
  return NS_OK;
}

//-------------------------------------------------------
// This method refreshes a docshell
void 
nsEventStateManager::ForceUpdate(nsIDocShell* aDocShell)
{
  NS_ASSERTION(aDocShell, "Pointer is null!");

  nsCOMPtr<nsIPresShell> presShell;
  aDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell) return;

  nsCOMPtr<nsIPresContext> presContext;
  aDocShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) return;

  nsCOMPtr<nsIViewManager> vm;
  presShell->GetViewManager(getter_AddRefs(vm));
  if (!vm) return;

  vm->UpdateAllViews(NS_VMREFRESH_NO_SYNC);

#ifdef DEBUG_DOCSHELL_FOCUS
  printf("ForceUpdate - DS: %p ********************************************\n", aDocShell);
#endif
}

//-------------------------------------------------------
// This method will find the next DocShell within Parent
nsIDocShell* 
nsEventStateManager::GetNextDocShell(nsIDocShell* aParentDS, nsIDocShell* aCurrentDS, PRBool aForward)
{
  NS_ASSERTION(aParentDS, "Pointer is null!");
  NS_ASSERTION(aCurrentDS, "Pointer is null!");

  nsCOMPtr<nsIDocShellTreeNode> parentTreeNode(do_QueryInterface(aParentDS));
  if (!parentTreeNode) return nsnull;

  PRInt32 count;
  parentTreeNode->GetChildCount(&count);
  for (PRInt32 i=0;i<count;i++) {
    nsCOMPtr<nsIDocShellTreeItem> child;
    parentTreeNode->GetChildAt(i, getter_AddRefs(child));
    nsCOMPtr<nsIDocShell> ds = do_QueryInterface(child);
    if (ds.get() == aCurrentDS) {
      i += aForward?1:-1;
      if (i > -1 && i < count) {
        parentTreeNode->GetChildAt(i, getter_AddRefs(child));
        ds = do_QueryInterface(child);
        nsIDocShell* fndDocShell = ds.get();
        NS_ADDREF(fndDocShell);
        return fndDocShell;
      }
      return nsnull;
    }
  }
  return nsnull;
}

//-------------------------------------------------------
// This method will find a DocShell that corresponds to 
// a content node in a parent document
nsIDocShell* 
nsEventStateManager::GetDocShellFromContent(nsIDocShell* aParentDocShell, nsIContent* aContent)
{
  NS_ASSERTION(aParentDocShell, "Pointer is null!");
  NS_ASSERTION(aContent, "Pointer is null!");

  nsCOMPtr<nsIPresShell> parentPresShell;
  aParentDocShell->GetPresShell(getter_AddRefs(parentPresShell));
  nsCOMPtr<nsISupports> supports;
  parentPresShell->GetSubShellFor(aContent, getter_AddRefs(supports));
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(supports);
  if (docShell) {
    nsIDocShell* ds = docShell.get();
    NS_ADDREF(ds);
    return ds;
  }
  return nsnull;
}

//-------------------------------------------------------
// This method will get the very last content object 
// when given aRootContent node
nsIContent*
nsEventStateManager::GetLastContent(nsIContent* aRootContent)
{
  if (aRootContent == nsnull) {
    return nsnull;
  }

  PRInt32 count;
  aRootContent->ChildCount(count);
  nsCOMPtr<nsIContent> child;
  aRootContent->ChildAt(count-1, *getter_AddRefs(child));
  nsIContent* last = GetLastContent(child);
  return last;
}


//-------------------------------------------------------
// This method will get the very last content object in
// in a DocShell
nsIContent*
nsEventStateManager::GetLastContent(nsIDocShell* aDocShell)
{
  NS_ASSERTION(aDocShell, "Pointer is null!");

  nsCOMPtr<nsIPresShell> presShell;
  aDocShell->GetPresShell(getter_AddRefs(presShell));
  if (!presShell) return nsnull;

  nsCOMPtr<nsIDocument> doc;
  presShell->GetDocument(getter_AddRefs(doc));
  if (!doc) return nsnull;

  nsCOMPtr<nsIContent> rootContent;
  doc->GetRootContent(getter_AddRefs(rootContent));
  if (!rootContent) return nsnull;

  nsIContent* last = nsnull;
  nsCOMPtr<nsIContent> parent = rootContent;
  PRInt32 count;
  parent->ChildCount(count);
  while (count > 0) {
    nsCOMPtr<nsIContent> child;
    parent->ChildAt(count-1, *getter_AddRefs(child));
    child->ChildCount(count);
    parent = child;
    last = child.get();
  }
  NS_IF_ADDREF(last);
  return last;
}

//-------------------------------------------------------
// This method will get the "next" Frame in a FrameSet (forward or backwards)
// and if one doesn't exist, then focus the next docshell AFTER
// the FrameSet
PRBool
nsEventStateManager::FocusAfterHTMLFrameDoc(nsIDocShell* aDocShell, 
                                           nsIDocShell* aParentDocShell, 
                                           PRBool       aForward)
{
  // neither of should ever be null, but just in case
  if (aDocShell == nsnull || aParentDocShell == nsnull) {
    return PR_FALSE;
  }

  // Try to get the next DocShell
  nsCOMPtr<nsIDocShell> nextDocShell = getter_AddRefs(GetNextDocShell(aParentDocShell, aDocShell, aForward));
  if (nextDocShell) {
    // unset focus in current doc
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(nextDocShell));
    if (NS_SUCCEEDED(baseWin->SetFocus())) {
#ifdef DEBUG_DOCSHELL_FOCUS
      printf("Focused DS: %p ********************************************\n", nextDocShell.get());
#endif
      return PR_TRUE;
    }
  } else {
    // ok, since the we are a HTMLFrame and parent is a FrameSet
    // then we will get the parent of the FrameSet and try to 
    // find the next aDocShell
    nsCOMPtr<nsIDocShell> oldDocShell = aDocShell;

    // unset focus in current doc
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    nsCOMPtr<nsIDocShellTreeItem> parentItem(do_QueryInterface(aParentDocShell));
    if (!parentItem) return PR_FALSE;

    nsCOMPtr<nsIDocShell> ds  = aDocShell;
    nsCOMPtr<nsIDocShell> pDS = aParentDocShell;
    nsCOMPtr<nsIDocShellTreeItem> parentParentItem;
    parentItem->GetParent(getter_AddRefs(parentParentItem));
    if (parentParentItem) {
      ds = aParentDocShell;
      pDS = do_QueryInterface(parentParentItem);
    }
    PRBool focusTaken;
    // it was the last one in the Frame
    //Offer focus upwards to allow shifting focus to UI controls
    nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(ds));
    nsCOMPtr<nsIBaseWindow> parentDocShellAsWin(do_QueryInterface(pDS));
    if (docShellAsWin && parentDocShellAsWin) {
      parentDocShellAsWin->FocusAvailable(docShellAsWin, aForward, &focusTaken);
      if (focusTaken) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

//-------------------------------------------------------
// This methods finds the content object in the parent document that 
// corresponds to the aDocShell.
// Then the parent doc is asked to find the next "focusable" item
// AFTER this IFrame.
PRBool
nsEventStateManager::FocusAfterHTMLIFrameDoc(nsIDocShell* aDocShell, 
                                             nsIDocShell* aParentDocShell, 
                                             PRBool       aForward,
                                             PRBool&      aFocusDoc)
{
  NS_ASSERTION(aDocShell, "Pointer is null!");
  NS_ASSERTION(aParentDocShell, "Pointer is null!");

  // Make sure this IFrame is inside a Content Doc
  nsCOMPtr<nsIDocShellTreeItem> parentItem(do_QueryInterface(aParentDocShell));
  if (!parentItem) return PR_FALSE;

  PRInt32 type;
  parentItem->GetItemType(&type);
  if (type == nsIDocShellTreeItem::typeContent) {

    // remove focus from any items in doc
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    // Go find the root content object for the parent document
    // so we can find the content that corresponds to this docshell
    nsCOMPtr<nsIPresShell> parentPresShell;
    aParentDocShell->GetPresShell(getter_AddRefs(parentPresShell));
    if (!parentPresShell) return PR_FALSE;

    nsCOMPtr<nsIDocument> parentDoc;
    parentPresShell->GetDocument(getter_AddRefs(parentDoc));
    if (!parentDoc) return PR_FALSE;

    nsCOMPtr<nsIPresContext> parentPresContext;
    aParentDocShell->GetPresContext(getter_AddRefs(parentPresContext));
    if (!parentPresContext) return PR_FALSE;

    nsCOMPtr<nsIContent> rootContent;
    parentDoc->GetRootContent(getter_AddRefs(rootContent));
    if (!rootContent) return PR_FALSE;

    // Now that we have the Root Content for the Parent Doc
    // we can look for the the content object that corresponds to this DocShell
    nsCOMPtr<nsIContent> focusedContent = getter_AddRefs(FindContentForDocShell(parentPresShell, rootContent, aDocShell));

#ifdef DEBUG_DOCSHELL_FOCUS
    printf("For Child DocShell %p shifting focus to content %p in parent DocShell %p\n", 
            aDocShell, focusedContent.get(), aParentDocShell);
#endif

    nsCOMPtr<nsIEventStateManager> esm;
    parentPresContext->GetEventStateManager(getter_AddRefs(esm));
    if (!esm) return PR_FALSE;

#ifdef DEBUG_DOCSHELL_FOCUS
    printf("PDS: %p  Pres %p  EM %p  Doc %p\n", aParentDocShell, 
           parentPresShell.get(), esm.get(), parentDoc.get());
#endif

    // So we use that Content node in the Parent Doc 
    // to focus the very next object
    if (focusedContent) {
      esm->MoveFocus(aForward, focusedContent);
    }
    nsCOMPtr<nsIContent> newFocusedContent;
    esm->GetFocusedContent(getter_AddRefs(newFocusedContent));

    /*nsCOMPtr<nsIContent> next;
    nsIFrame* primaryFrame = nsnull;
    parentPresShell->GetPrimaryFrameFor(focusedContent, &primaryFrame);
    if (primaryFrame) {
      esm->GetNextTabbableContent(rootContent, primaryFrame, aForward, getter_AddRefs(next));
    }*/

    if (newFocusedContent) {
      // Now, give focus to the DOM Window
      nsCOMPtr<nsIScriptGlobalObject> sgo;
      parentDoc->GetScriptGlobalObject(getter_AddRefs(sgo));
      if (!sgo) return PR_FALSE;

      nsCOMPtr<nsIDOMWindowInternal> domwin(do_QueryInterface(sgo));
      if (domwin && NS_SUCCEEDED(domwin->Focus())) {
        aParentDocShell->SetCanvasHasFocus(PR_FALSE);
        return PR_TRUE;
      }
    } else {
      return PR_TRUE;
    }

    // tell the outside NOT to focus the document
    // because it found something to focus
    aFocusDoc = PR_FALSE;
  }
  return PR_FALSE;
}

//------------------------------------------------------------------
// This method is checking for two things:
//
// 1) To see if the currently focused content has a docshell
//    and if it does we focus it
//
// 2) Check to see if this frame is the last frame in the frameset
//    and if it is then we want the parent to focus the next piece of
//    content, which means it should move on to the Chrome
PRBool
nsEventStateManager::FocusWithinHTMLFrameDoc(nsIContent*   aRootContent,
                                             nsIPresShell* aPresShell,
                                             PRBool        aForward,
                                             PRBool&       aDoFocusAvailDocShells)
{
  NS_ASSERTION(aPresShell, "Pointer is null!");

  if (mCurrentFocus == nsnull) return PR_FALSE;

  if (aRootContent == nsnull) {
    if (aPresShell) {
      // See if the currently focused item has a DocShell (like an IFrame)
      //  and then give it focus
      nsCOMPtr<nsISupports> supports;
      aPresShell->GetSubShellFor(mCurrentFocus, getter_AddRefs(supports));
      nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(supports);
      if (docShell) {
        nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(docShell));
        if (NS_SUCCEEDED(baseWin->SetFocus())) {
#ifdef DEBUG_DOCSHELL_FOCUS
          printf("Focused DS: %p ********************************************\n", docShell.get());
#endif
          return PR_TRUE;
        }
      }
    }
  } else {
    // Ok, the root content has been set so we need to go find the content
    // in the parent's doc that represents this DocShell
    nsCOMPtr<nsIDocShell> docShell;
    nsCOMPtr<nsIDocShell> parentDS;

    if (NS_SUCCEEDED(GetDocShellsFromDoc(mDocument, getter_AddRefs(docShell), getter_AddRefs(parentDS)))) {
      nsCOMPtr<nsIPresShell> parentPresShell;
      parentDS->GetPresShell(getter_AddRefs(parentPresShell));
      if (!parentPresShell) return PR_FALSE;

      nsCOMPtr<nsIDocument> parentDoc;
      parentPresShell->GetDocument(getter_AddRefs(parentDoc));
      if (!parentDoc) return PR_FALSE;

      nsCOMPtr<nsIContent> rootContent;
      parentDoc->GetRootContent(getter_AddRefs(rootContent));
      if (!rootContent) return PR_FALSE;

      // Look up the content
      nsCOMPtr<nsIContent> focusedContent = getter_AddRefs(FindContentForDocShell(parentPresShell, rootContent, docShell));

      // Now check to see if the focused piece of content that represents
      // the current DocShell is the last frame in the frameset
      if (focusedContent && IsLastFrameInFrameSet(focusedContent)) {
        aDoFocusAvailDocShells = PR_TRUE;

        PRBool focusTaken = PR_FALSE;
        SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

        // Now let the parent doc find the next focusable DocShell
        // and in this case the focus will move on out of the frameset
        nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(docShell));
        nsCOMPtr<nsIBaseWindow> parentDocShellAsWin(do_QueryInterface(parentDS));
        if (docShellAsWin) {
          parentDocShellAsWin->FocusAvailable(docShellAsWin, aForward, &focusTaken);
          if (focusTaken) {
            return PR_TRUE;
          }
        }
      }
    }
  }
  return PR_FALSE;
}

//------------------------------------------------------------------
// This method finds the 
PRBool
nsEventStateManager::FocusWithinHTMLIFrameDoc(nsIContent* aNextContent, PRBool aForward)
{
  NS_ASSERTION(aNextContent, "Pointer is null!");

  // Begin by finding the DocShell for this document
  // and the DocShell for the IFrame content
  nsCOMPtr<nsISupports> container;
  mPresContext->GetContainer(getter_AddRefs(container));
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  if (!docShell) return PR_FALSE;

  // Here we find the DocShell that correpsonds to the IFrame
  nsCOMPtr<nsIDocShell> newDocShell = getter_AddRefs(GetDocShellFromContent(docShell, aNextContent));
  if (newDocShell) {
    nsCOMPtr<nsIBaseWindow> newDocShellAsWin(do_QueryInterface(newDocShell));
    if (newDocShellAsWin) {
      // Clear focus in current document
      SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

      // set focus in IFrame
      if (NS_SUCCEEDED(newDocShellAsWin->SetFocus())) {
        // Now if we are reversing through the document then
        // we need to find the last piece of content in the IFrame document
        if (!aForward) {
          nsCOMPtr<nsIPresContext> presContext;
          newDocShell->GetPresContext(getter_AddRefs(presContext));
          if (!presContext) return PR_FALSE;

          nsCOMPtr<nsIEventStateManager> esm;
          presContext->GetEventStateManager(getter_AddRefs(esm));
          if (!esm) return PR_FALSE;

          nsCOMPtr<nsIContent> lastContent = getter_AddRefs(GetLastContent(newDocShell));
          if (lastContent) {
            esm->MoveFocus(PR_FALSE, lastContent);
          }
        }
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

//-------------------------------------------------
// Traversal by document/DocShell only
// this does not inclide any content inside the doc
// or IFrames
void
nsEventStateManager::ShiftFocusByDoc(PRBool aForward, nsIContent* aRoot)
{
  // Figure out what type of document we are
  eDocType docType;
  FigureOutKindOfDoc(mDocument, &docType);

#ifdef DEBUG_DOCSHELL_FOCUS
  printf("Doc Type is [%s]\n", gDocTypeNames[docType]);
#endif

  if (docType == eChrome) {
    ShiftFocus(aForward, aRoot);
    return;
  }

  if (docType == eFrame) {
    nsCOMPtr<nsIDocShell> docShell;
    nsCOMPtr<nsIDocShell> parentDS;
    if (NS_SUCCEEDED(GetDocShellsFromDoc(mDocument, getter_AddRefs(docShell), getter_AddRefs(parentDS)))) {
      // Try to focus the Next Frame in the FrameSet
      // if there is none then focus the next Chrome Doc
      FocusAfterHTMLFrameDoc(docShell, parentDS, aForward);
    } 
  }

}

