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
#include "nsINameSpaceManager.h"  // for kNameSpaceID_HTML
#include "nsIWebShell.h"
#include "nsIFocusableContent.h"
#include "nsIScrollableView.h"
#include "nsIDOMSelection.h"
#include "nsIFrameSelection.h"
#include "nsIDeviceContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsISelfScrollingFrame.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIGfxTextControlFrame.h"
#include "nsPIDOMWindow.h"

#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "prefapi.h"
#include "nsISessionHistory.h"

#undef DEBUG_scroll     // define to see ugly mousewheel messages

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDOMHTMLAnchorElementIID, NS_IDOMHTMLANCHORELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLSelectElementIID, NS_IDOMHTMLSELECTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLTextAreaElementIID, NS_IDOMHTMLTEXTAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLAreaElementIID, NS_IDOMHTMLAREAELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLObjectElementIID, NS_IDOMHTMLOBJECTELEMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLButtonElementIID, NS_IDOMHTMLBUTTONELEMENT_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIFocusableContentIID, NS_IFOCUSABLECONTENT_IID);
static NS_DEFINE_IID(kIScrollableViewIID, NS_ISCROLLABLEVIEW_IID);

//we will use key binding by default now. this wil lbreak viewer for now
#define NON_KEYBINDING 0  

nsIFrame * gCurrentlyFocusedTargetFrame = 0; 
nsIContent * gCurrentlyFocusedContent = 0; // Weak because it mirrors the strong mCurrentFocus

nsIContent * gLastFocusedContent = 0; // Strong reference
nsIDocument * gLastFocusedDocument = 0; // Strong reference
nsIPresContext* gLastFocusedPresContext = 0; // Weak reference

PRUint32 nsEventStateManager::mInstanceCount = 0;

// For caching mousewheel prefs
struct IntPrefPair
{
  char* name;
  PRInt32 value;
};

struct BoolPrefPair
{
  char* name;
  PRBool value;
};

enum {
 MOUSE_SCROLL_N_LINES,
 MOUSE_SCROLL_PAGE,
 MOUSE_SCROLL_HISTORY
};

static IntPrefPair	mwIntPrefValues[] = 
{
  { "mousewheel.withnokey.action", MOUSE_SCROLL_N_LINES },
  { "mousewheel.withnokey.numlines", 3 },
  { "mousewheel.withcontrolkey.action", MOUSE_SCROLL_N_LINES },
  { "mousewheel.withcontrolkey.numlines", 3 },
  { "mousewheel.withshiftkey.action", MOUSE_SCROLL_N_LINES },
  { "mousewheel.withshiftkey.numlines", 3 },
  { "mousewheel.withaltkey.action", MOUSE_SCROLL_N_LINES },
  { "mousewheel.withaltkey.numlines", 3 }
};

static BoolPrefPair mwBoolPrefValues[] =
{
  { "mousewheel.withnokey.sysnumlines", PR_FALSE },
  { "mousewheel.withcontrolkey.sysnumlines", PR_FALSE },
  { "mousewheel.withshiftkey.sysnumlines", PR_FALSE },
  { "mousewheel.withaltkey.sysnumlines", PR_FALSE }
};

static PRUint32 numIntPrefValues =
  (sizeof(mwIntPrefValues) / sizeof(mwIntPrefValues[0]));

static PRUint32 numBoolPrefValues =
  (sizeof(mwBoolPrefValues) / sizeof(mwBoolPrefValues[0]));

static PRInt32 getCachedIntPref (const char* aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName, "aPrefName = null");

  for (PRUint32 i = 0; i < numIntPrefValues; i++)
  {
    if (nsAutoString(mwIntPrefValues[i].name) == aPrefName)
    {
      return mwIntPrefValues[i].value;
    }
  }

  // Hopefully we never reach this code
  NS_ASSERTION(PR_FALSE,
               "Attempt to get a cached pref that is not in the cache.");
  return 0;
}

static PRBool getCachedBoolPref(const char* aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName, "aPrefName = null");

  for (PRUint32 i = 0; i < numBoolPrefValues; i++)
  {
    if (nsAutoString(mwBoolPrefValues[i].name) == aPrefName)
    {
      return mwBoolPrefValues[i].value;
    }
  }

  // Hopefully we never reach this code
  NS_ASSERTION(PR_FALSE,
               "Attempt to get a cached pref that is not in the cache.");
  return 0;
}

static void setCachedIntPref (const char* aPrefName, PRInt32 aValue)
{
  NS_ASSERTION(nsnull != aPrefName, "aPrefName = null");

  for (PRUint32 i = 0; i < numIntPrefValues; i++)
  {
    if (nsAutoString(mwIntPrefValues[i].name) == aPrefName)
    {
      mwIntPrefValues[i].value = aValue;
      return;
    }
  }

  NS_ASSERTION(PR_FALSE,
               "Attempt to set a cached pref that is not in the cache.");
}

static void setCachedBoolPref (const char* aPrefName, PRBool aValue)
{
  NS_ASSERTION(nsnull != aPrefName, "aPrefName = null");

  for (PRUint32 i = 0; i < numBoolPrefValues; i++)
  {
    if (nsAutoString(mwBoolPrefValues[i].name) == aPrefName)
    {
      mwBoolPrefValues[i].value = aValue;
      return;
    }
  }

  NS_ASSERTION(PR_FALSE,
               "Attempt to set a cached pref that is not in the cache.");
}

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

int mwIntPrefChangedCallback(const char* name, void * closure)
{
  nsIPref* prefs = nsnull;
  nsresult rv = nsServiceManager::GetService(kPrefCID, NS_GET_IID(nsIPref),
                                              (nsISupports**) &prefs);
  NS_ASSERTION(NS_SUCCEEDED(rv),"Could not get prefs service.");
  NS_ASSERTION(nsnull != prefs,"Prefs service is null.");

  if (NS_SUCCEEDED(rv))
  {
    PRInt32 value;
    prefs->GetIntPref(name, &value);
    setCachedIntPref(name, value);
    NS_RELEASE(prefs);
  }

  return PREF_NOERROR;
}

int mwBoolPrefChangedCallback(const char* name, void * closure)
{
  nsIPref* prefs = nsnull;
  nsresult rv = nsServiceManager::GetService(kPrefCID, NS_GET_IID(nsIPref),
                                              (nsISupports**) &prefs);
  NS_ASSERTION(NS_SUCCEEDED(rv),"Could not get prefs service.");
  NS_ASSERTION(nsnull != prefs,"Prefs service is null.");

  if (NS_SUCCEEDED(rv))
  {
    PRBool value;
    prefs->GetBoolPref(name, &value);
    setCachedBoolPref(name, value);
    NS_RELEASE(prefs);
  }

  return PREF_NOERROR;
}

PRInt32 getIntPref(nsIPref* aPrefs, const char* aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName,"aPrefName = null.");
  NS_ASSERTION(nsnull != aPrefs,"aPrefs = null.");

  PRInt32 value = 0;
  if (aPrefs)
  {
    aPrefs->GetIntPref(aPrefName, &value);
  }

  return value;
}

PRBool getBoolPref(nsIPref* aPrefs, const char* aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName, "aPrefName = null.");
  NS_ASSERTION(nsnull != aPrefs, "aPrefs = null.");

  PRBool value = PR_FALSE;
  if (aPrefs)
  {
    aPrefs->GetBoolPref(aPrefName, &value);
  }

  return value;
}
      
void mwRegisterPrefCallbacks()
{
  static PRBool once = PR_TRUE;

  if (once) {
    once = PR_FALSE;
    nsIPref* prefs = nsnull;
    nsresult rv = nsServiceManager::GetService(kPrefCID,
                                 NS_GET_IID(nsIPref), (nsISupports**) &prefs);
    NS_ASSERTION(NS_SUCCEEDED(rv),"Could not get prefs service.");
    NS_ASSERTION(nsnull != prefs,"Prefs services is null.");

    if (NS_SUCCEEDED(rv))
    {
     PRUint32 i;
 
     for (i = 0; i < numIntPrefValues; i++)
      {
        mwIntPrefValues[i].value = getIntPref(prefs, mwIntPrefValues[i].name);
        prefs->RegisterCallback(mwIntPrefValues[i].name,
                                mwIntPrefChangedCallback, NULL);
      }

      for (i = 0; i < numBoolPrefValues; i++)
      {
        mwBoolPrefValues[i].value = getBoolPref(prefs,
                                                mwBoolPrefValues[i].name);
        prefs->RegisterCallback(mwBoolPrefValues[i].name,
                                mwBoolPrefChangedCallback, NULL);
      }

      NS_RELEASE(prefs);
    }
  }
}


nsEventStateManager::nsEventStateManager()
  : mGestureDownPoint(0,0)
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
  NS_INIT_REFCNT();
  
  ++mInstanceCount;
  mwRegisterPrefCallbacks();
}

nsEventStateManager::~nsEventStateManager()
{
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
  
  --mInstanceCount;
  if(mInstanceCount == 0) {
    NS_IF_RELEASE(gLastFocusedContent);
    NS_IF_RELEASE(gLastFocusedDocument);
  }
}

NS_IMPL_ISUPPORTS1(nsEventStateManager, nsIEventStateManager)

NS_IMETHODIMP
nsEventStateManager::PreHandleEvent(nsIPresContext* aPresContext, 
                                 nsGUIEvent *aEvent,
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
    BeginTrackingDragGesture ( aEvent, aTargetFrame );
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
    StopTrackingDragGesture();
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
    break;
  case NS_MOUSE_MOVE:
    GenerateDragGesture(aPresContext, aEvent);
    UpdateCursor(aPresContext, aEvent->point, aTargetFrame, aStatus);
    GenerateMouseEnterExit(aPresContext, aEvent);
    break;
  case NS_MOUSE_EXIT:
    GenerateMouseEnterExit(aPresContext, aEvent);
    break;
  case NS_DRAGDROP_OVER:
    GenerateDragDropEnterExit(aPresContext, aEvent);
    break;
  case NS_GOTFOCUS:
    {
      nsCOMPtr<nsIPresShell> presShell;
      aPresContext->GetShell(getter_AddRefs(presShell));
      
      if (!mDocument) {  
        if (presShell) {
          presShell->GetDocument(&mDocument);
        }
      }

      if (gLastFocusedDocument == mDocument)
         break;
        
      //fire focus
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent focusevent;
      focusevent.eventStructType = NS_EVENT;
      focusevent.message = NS_FOCUS_CONTENT;

      if (mDocument) {
        // fire focus on window, not document
        nsCOMPtr<nsIScriptGlobalObject> globalObject;
        mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
        if (globalObject) {
          nsIContent* currentFocus = mCurrentFocus;
          mCurrentFocus = nsnull;
          mDocument->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status);
          globalObject->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status); 
          mCurrentFocus = currentFocus;
        }

        
        NS_IF_RELEASE(gLastFocusedDocument);
        gLastFocusedDocument = mDocument;
        gLastFocusedPresContext = aPresContext;
        NS_IF_ADDREF(gLastFocusedDocument);
      }
    }
    break;
    
  case NS_LOSTFOCUS:
    {
      // Hold the blur, wait for the focus so we can query the style of the focus
      // target as to what to do with the event. If appropriate we fire the blur
      // at that time.
    }
    break;
    
 case NS_ACTIVATE:
    {
      nsIContent* newFocus;
      mCurrentTarget->GetContent(&newFocus);
      if (newFocus) {
        nsIFocusableContent *focusChange;
        if (NS_SUCCEEDED(newFocus->QueryInterface(kIFocusableContentIID,
                                                  (void **)&focusChange))) {
          NS_RELEASE(focusChange);
          NS_RELEASE(newFocus);
          break;
        }
        NS_RELEASE(newFocus);
      }
      
      //fire focus
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent focusevent;
      focusevent.eventStructType = NS_EVENT;
      focusevent.message = NS_FOCUS_CONTENT;

      if (!mDocument) {
        nsCOMPtr<nsIPresShell> presShell;
        aPresContext->GetShell(getter_AddRefs(presShell));
        if (presShell) {
          presShell->GetDocument(&mDocument);
        }
      }

      if (mDocument) {
        // NS_IF_RELEASE(gLastFocusedContent);
        // mCurrentTarget->GetContent(&gLastFocusedContent);
                
        mCurrentTarget = nsnull;

        // fire focus on window, not document
        nsCOMPtr<nsIScriptGlobalObject> globalObject;
        mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
        if(!globalObject) break;
                     
        globalObject->HandleDOMEvent(aPresContext, &focusevent, nsnull, NS_EVENT_FLAG_INIT, &status); 
      }
          
    }
    break;
    
 case NS_DEACTIVATE:
    {
      nsIContent* newFocus;
      mCurrentTarget->GetContent(&newFocus);
      if (newFocus) {
        nsIFocusableContent *focusChange;
        if (NS_SUCCEEDED(newFocus->QueryInterface(kIFocusableContentIID,
                                                  (void **)&focusChange))) {
          NS_RELEASE(focusChange);
          NS_RELEASE(newFocus);
          break;
        }
        NS_RELEASE(newFocus);
      }
          
              //fire blur
              nsEventStatus status = nsEventStatus_eIgnore;
              nsEvent event;
              event.eventStructType = NS_EVENT;
              event.message = NS_BLUR_CONTENT;

              if (!mDocument) {
                nsCOMPtr<nsIPresShell> presShell;
                aPresContext->GetShell(getter_AddRefs(presShell));
                if (presShell) {
                  presShell->GetDocument(&mDocument);
                }
              }
                          
              if (mDocument) {
                mCurrentTarget = nsnull;

                // fire focus on window, not document
                nsCOMPtr<nsIScriptGlobalObject> globalObject;
                mDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
                if(!globalObject) break;
                     
                globalObject->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
              }
                          
    } 
    break;
    
  case NS_KEY_PRESS:
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


// 
// BeginTrackingDragGesture
//
// Record that the mouse has gone down and that we should move to TRACKING state
// of d&d gesture tracker.
//
void
nsEventStateManager :: BeginTrackingDragGesture ( nsGUIEvent* inDownEvent, nsIFrame* inDownFrame )
{
  mIsTrackingDragGesture = PR_TRUE;
  mGestureDownPoint = inDownEvent->point;
  mGestureDownFrame = inDownFrame;
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
      nsEventStatus status = nsEventStatus_eIgnore;
      nsMouseEvent event;
      event.eventStructType = NS_DRAGDROP_EVENT;
      event.message = NS_DRAGDROP_GESTURE;
      event.widget = aEvent->widget;
      event.clickCount = 0;
      event.point = aEvent->point;
      event.refPoint = aEvent->refPoint;

      // dispatch to the DOM
      nsCOMPtr<nsIContent> lastContent;
      if ( mGestureDownFrame ) {
        mGestureDownFrame->GetContent(getter_AddRefs(lastContent));
        if ( lastContent )
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
      }
      
      // dispatch to the frame
      if ( mGestureDownFrame )
        mGestureDownFrame->HandleEvent(aPresContext, &event, &status);   

      StopTrackingDragGesture();              
    }
  }
} // GenerateDragGesture


NS_IMETHODIMP
nsEventStateManager::PostHandleEvent(nsIPresContext* aPresContext, 
                                     nsGUIEvent *aEvent,
                                     nsIFrame* aTargetFrame,
                                     nsEventStatus* aStatus,
                                     nsIView* aView)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG_POINTER(aStatus);
  mCurrentTarget = aTargetFrame;
  NS_IF_RELEASE(mCurrentTargetContent);  
  nsresult ret = NS_OK;

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
        mCurrentTarget->GetContent(getter_AddRefs(newFocus));
        nsCOMPtr<nsIFocusableContent> focusable;
  
        if (newFocus) {
          // Look for the nearest enclosing focusable content.
          nsCOMPtr<nsIContent> current = newFocus;
          while (current) {
            focusable = do_QueryInterface(current);
            if (focusable)
              break;

            nsCOMPtr<nsIContent> parent;
            current->GetParent(*getter_AddRefs(parent));
            current = parent;
          }

          // if a focusable piece of content is an anonymous node
          // weed to find the parent and have the parent be the 
          // new focusable node
          //
          // We fist to check here to see if the focused content 
          // is anonymous content
          if (focusable) {
            nsCOMPtr<nsIContent> parent;
            current->GetParent(*getter_AddRefs(parent));
            NS_ASSERTION(parent.get(), "parent is null.  this should not happen.");
            PRInt32 numChilds;
            parent->ChildCount(numChilds);
            PRInt32 i;
            PRBool isChild = PR_FALSE;
            for (i=0;i<numChilds;i++) {
              nsCOMPtr<nsIContent> child;
              parent->ChildAt(i, *getter_AddRefs(child));
              if (child.get() == current.get()) {
                isChild = PR_TRUE;
                break;
              }
            }
            // if it isn't a child of the parent, then it is anonynous content
            // Now, go up the parent list to find a focusable content node
            if (!isChild) {
              current = parent;
              while (current) {
                focusable = do_QueryInterface(current);
                if (focusable)
                  break;

                nsCOMPtr<nsIContent> tempParent;
                current->GetParent(*getter_AddRefs(tempParent));
                current = tempParent;
              }
            }
            // now we ned to get the content node's frame and reset 
            // the mCurrentTarget target
            nsCOMPtr<nsIPresShell> shell;
            if (mPresContext) {
              nsresult rv = mPresContext->GetShell(getter_AddRefs(shell));
              if (NS_SUCCEEDED(rv) && shell){
                shell->GetPrimaryFrameFor(current, &mCurrentTarget);
              }
            }
            // now adjust the value for the "newFocus"
            newFocus = current;
          }

          nsCOMPtr<nsIContent> content = do_QueryInterface(focusable);
          ChangeFocus(content, mCurrentTarget, PR_TRUE);
        }
        SetContentState(newFocus, NS_EVENT_STATE_ACTIVE);
      }
    }
    break;
  case NS_MOUSE_LEFT_BUTTON_UP:
  case NS_MOUSE_MIDDLE_BUTTON_UP:
  case NS_MOUSE_RIGHT_BUTTON_UP:
    {
      ret = CheckForAndDispatchClick(aPresContext, (nsMouseEvent*)aEvent, aStatus);

      SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
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
      nsMouseScrollEvent *msEvent = (nsMouseScrollEvent*) aEvent;
      PRInt32 action = 0;
      PRInt32 numLines = 0;

      if (msEvent->isShift) {
        action = getCachedIntPref("mousewheel.withshiftkey.action");
        if (getCachedBoolPref("mousewheel.withshiftkey.sysnumlines"))
          numLines = msEvent->deltaLines;
        else
          numLines = getCachedIntPref("mousewheel.withshiftkey.numlines");
      } else if (msEvent->isControl) {
        action = getCachedIntPref("mousewheel.withcontrolkey.action");
        if (getCachedBoolPref("mousewheel.withcontrolkey.sysnumlines"))
          numLines = msEvent->deltaLines;
        else
          numLines = getCachedIntPref("mousewheel.withcontrolkey.numlines");
      } else if (msEvent->isAlt) {
        action = getCachedIntPref("mousewheel.withaltkey.action");
        if (getCachedBoolPref("mousewheel.withaltkey.sysnumlines"))
          numLines = msEvent->deltaLines;
        else
          numLines = getCachedIntPref("mousewheel.withaltkey.numlines");
      } else {
        action = getCachedIntPref("mousewheel.withnokey.action");
        if (getCachedBoolPref("mousewheel.withnokey.sysnumlines"))
          numLines = msEvent->deltaLines;
        else
          numLines = getCachedIntPref("mousewheel.withnokey.numlines");
      }

      if ((msEvent->deltaLines < 0) && (numLines > 0))
        numLines = -numLines;

      switch (action) {
      case MOUSE_SCROLL_N_LINES:
        {
          nsIView* focusView = nsnull;
          nsIScrollableView* sv = nsnull;
          nsISelfScrollingFrame* sf = nsnull;

          if (NS_SUCCEEDED(GetScrollableFrameOrView(aPresContext, aTargetFrame, aView, sv, sf, focusView)))
            {
              if (sv)
                {
                  sv->ScrollByLines(numLines);
                  ForceViewUpdate(focusView);
                }
              else if (sf)
                sf->ScrollByLines(aPresContext, numLines);
            }
          
          break;
        }
      case MOUSE_SCROLL_PAGE:
        if (!mCurrentFocus) {
          nsIScrollableView* sv = GetNearestScrollingView(aView);
          if (sv) {
            sv->ScrollByPages((numLines > 0) ? 1 : -1);
          }
        }
        break;
      case MOUSE_SCROLL_HISTORY:
        {
          nsCOMPtr<nsIWebShell> webShell;
          mPresContext->GetContainer(getter_AddRefs(webShell));
          if (nsnull != webShell) {
            nsCOMPtr<nsIWebShell> root;
              webShell->GetRootWebShell(*getter_AddRefs(root));
              if (nsnull != root) {
                nsCOMPtr<nsISessionHistory> sHist;
                root->GetSessionHistory(*getter_AddRefs(sHist));
                if (sHist) {
                  if (numLines > 0)
                  {
                    sHist->GoBack(root);
                  }
                  else
                  {
                    sHist->GoForward(root);
                  }
                }
              }
          }
        }
        break;
        
      }
    }

    break;
  
  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    // clean up after ourselves. make sure we do this _after_ the event, else we'll
    // clean up too early!
    GenerateDragDropEnterExit(aPresContext, aEvent);
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
            //Shift focus forward or back depending on shift key
            ShiftFocus(!((nsInputEvent*)aEvent)->isShift);
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
                sv->ScrollByLines((keyEvent->keyCode == NS_VK_DOWN) ? 1 : -1);
              
                // force the update to happen now, otherwise multiple scrolls can
                // occur before the update is processed. (bug #7354)
                nsIViewManager* vm = nsnull;
             	  if (NS_OK == aView->GetViewManager(vm) && nsnull != vm) {
             	    // I'd use Composite here, but it doesn't always work.
                  // vm->Composite();
                  nsIView* rootView = nsnull;
                  if (NS_OK == vm->GetRootView(rootView) && nsnull != rootView) {
              	    nsIWidget* rootWidget = nsnull;
              		  if (NS_OK == rootView->GetWidget(rootWidget) && nsnull != rootWidget) {
                      rootWidget->Update();
                      NS_RELEASE(rootWidget);
                    }
              	  }
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
  if (aFrame == mGestureDownFrame)
    mGestureDownFrame = nsnull;
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
  if (NS_OK == aView->QueryInterface(kIScrollableViewIID, (void**)&sv)) {
    return sv;
  }

  nsIView* parent;
  aView->GetParent(parent);

  if (nsnull != parent) {
    return GetNearestScrollingView(parent);
  }

  return nsnull;
}

nsISelfScrollingFrame*
nsEventStateManager::GetParentSelfScrollingFrame(nsIFrame* aFrame)
{
  nsISelfScrollingFrame *sf;
  if (NS_OK == aFrame->QueryInterface(NS_GET_IID(nsISelfScrollingFrame),
                                      (void**)&sf)) {
    return sf;
  }

  nsIFrame* parent;
  aFrame->GetParent(&parent);

  if (nsnull != parent) {
    return GetParentSelfScrollingFrame(parent);
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
nsEventStateManager::UpdateCursor(nsIPresContext* aPresContext, nsPoint& aPoint, nsIFrame* aTargetFrame, 
                                  nsEventStatus* aStatus)
{
  PRInt32 cursor;
  nsCursor c;
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
    aTargetFrame->GetCursor(aPresContext, aPoint, cursor);
  }

  switch (cursor) {
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
  //These aren't in the CSS2 spec. Don't know what to do with them.
  case NS_STYLE_CURSOR_NE_RESIZE:
  case NS_STYLE_CURSOR_NW_RESIZE:
  case NS_STYLE_CURSOR_SE_RESIZE:
  case NS_STYLE_CURSOR_SW_RESIZE:
    c = eCursor_select;
    break;
  }

  if (NS_STYLE_CURSOR_AUTO != cursor) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }

  nsCOMPtr<nsIWidget> window;
  aTargetFrame->GetWindow(aPresContext, getter_AddRefs(window));
  window->SetCursor(c);
}

void
nsEventStateManager::GenerateMouseEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent)
{
  //Hold onto old target content through the event and reset after.
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_MOUSE_MOVE:
    {
      if (mLastMouseOverFrame != mCurrentTarget) {
        //We'll need the content, too, to check if it changed separately from the frames.
        nsCOMPtr<nsIContent> lastContent;
        nsCOMPtr<nsIContent> targetContent;

        if ( mCurrentTarget )
          mCurrentTarget->GetContent(getter_AddRefs(targetContent));

        if (mLastMouseOverFrame) {
          //fire mouseout
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_MOUSE_EVENT;
          event.message = NS_MOUSE_EXIT;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.isShift = ((nsMouseEvent*)aEvent)->isShift;
          event.isControl = ((nsMouseEvent*)aEvent)->isControl;
          event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
          event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

          //The frame has change but the content may not have.  Check before dispatching to content
          mLastMouseOverFrame->GetContent(getter_AddRefs(lastContent));

          mCurrentTargetContent = lastContent;
          NS_IF_ADDREF(mCurrentTargetContent);
          mCurrentRelatedContent = targetContent;
          NS_IF_ADDREF(mCurrentRelatedContent);

          if (lastContent != targetContent) {
            //XXX This event should still go somewhere!!
            if (nsnull != lastContent) {
              lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
            }
          }

          //Now dispatch to the frame
          if (mLastMouseOverFrame) {
            //XXX Get the new frame
            mLastMouseOverFrame->HandleEvent(aPresContext, &event, &status);   
          }

          NS_IF_RELEASE(mCurrentTargetContent);
          NS_IF_RELEASE(mCurrentRelatedContent);
        }

        //fire mouseover
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_MOUSE_EVENT;
        event.message = NS_MOUSE_ENTER;
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
        if (lastContent != targetContent) {
          //XXX This event should still go somewhere!!
          if (targetContent) {
            targetContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
          }

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(targetContent, NS_EVENT_STATE_HOVER);
          }
        }

        //Now dispatch to the frame
        if (mCurrentTarget) {
          //XXX Get the new frame
          mCurrentTarget->HandleEvent(aPresContext, &event, &status);
        }

        NS_IF_RELEASE(mCurrentTargetContent);
        NS_IF_RELEASE(mCurrentRelatedContent);
        mLastMouseOverFrame = mCurrentTarget;
      }
    }
    break;
  case NS_MOUSE_EXIT:
    {
      //This is actually the window mouse exit event.
      if (nsnull != mLastMouseOverFrame) {
        //fire mouseout
        nsEventStatus status = nsEventStatus_eIgnore;
        nsMouseEvent event;
        event.eventStructType = NS_MOUSE_EVENT;
        event.message = NS_MOUSE_EXIT;
        event.widget = aEvent->widget;
        event.clickCount = 0;
        event.point = aEvent->point;
        event.refPoint = aEvent->refPoint;
        event.isShift = ((nsMouseEvent*)aEvent)->isShift;
        event.isControl = ((nsMouseEvent*)aEvent)->isControl;
        event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
        event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

        nsCOMPtr<nsIContent> lastContent;
        mLastMouseOverFrame->GetContent(getter_AddRefs(lastContent));

        mCurrentTargetContent = lastContent;
        NS_IF_ADDREF(mCurrentTargetContent);
        mCurrentRelatedContent = nsnull;

        if (lastContent) {
          lastContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 

          if (nsEventStatus_eConsumeNoDefault != status) {
            SetContentState(nsnull, NS_EVENT_STATE_HOVER);
          }
        }


        //Now dispatch to the frame
        if (nsnull != mLastMouseOverFrame) {
          //XXX Get the new frame
          mLastMouseOverFrame->HandleEvent(aPresContext, &event, &status);   
          mLastMouseOverFrame = nsnull;
        }

        NS_IF_RELEASE(mCurrentTargetContent);
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
        mCurrentTarget->GetContent(getter_AddRefs(targetContent));

        if ( mLastDragOverFrame ) {
          //fire drag exit
          nsEventStatus status = nsEventStatus_eIgnore;
          nsMouseEvent event;
          event.eventStructType = NS_DRAGDROP_EVENT;
          event.message = NS_DRAGDROP_EXIT;
          event.widget = aEvent->widget;
          event.clickCount = 0;
          event.point = aEvent->point;
          event.refPoint = aEvent->refPoint;
          event.isShift = ((nsMouseEvent*)aEvent)->isShift;
          event.isControl = ((nsMouseEvent*)aEvent)->isControl;
          event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
          event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;

          //The frame has change but the content may not have.  Check before dispatching to content
          mLastDragOverFrame->GetContent(getter_AddRefs(lastContent));

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
        event.message = NS_DRAGDROP_EXIT;
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
        mLastDragOverFrame->GetContent(getter_AddRefs(lastContent));

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
}

NS_IMETHODIMP
nsEventStateManager::SetClickCount(nsIPresContext* aPresContext, 
                                   nsMouseEvent *aEvent,
                                   nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;
  nsCOMPtr<nsIContent> mouseContent;

  mCurrentTarget->GetContent(getter_AddRefs(mouseContent));

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

  mCurrentTarget->GetContent(getter_AddRefs(mouseContent));

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
    event.point.x = aEvent->point.x;
    event.point.y = aEvent->point.y;
    event.refPoint.x = aEvent->refPoint.x;
    event.refPoint.y = aEvent->refPoint.y;
    event.clickCount = aEvent->clickCount;
    event.isShift = aEvent->isShift;
    event.isControl = aEvent->isControl;
    event.isAlt = aEvent->isAlt;
    event.isMeta = aEvent->isMeta;

    if (mouseContent) {
      ret = mouseContent->HandleDOMEvent(aPresContext, &event, nsnull,
                                         NS_EVENT_FLAG_INIT, aStatus); 
    }

    if (nsnull != mCurrentTarget) {
      ret = mCurrentTarget->HandleEvent(aPresContext, &event, aStatus);
    }
  }
  return ret;
}

NS_IMETHODIMP
nsEventStateManager::DispatchKeyPressEvent(nsIPresContext* aPresContext, 
                                           nsKeyEvent *aEvent,
                                           nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;

  //fire keypress
  nsKeyEvent event;
  event.eventStructType = NS_KEY_EVENT;
  event.message = NS_KEY_PRESS;
  event.widget = nsnull;
  event.keyCode = aEvent->keyCode;
  event.isShift = aEvent->isShift;
  event.isControl = aEvent->isControl;
  event.isAlt = aEvent->isAlt;
  event.isMeta = aEvent->isMeta;

  nsIContent *content;
  mCurrentTarget->GetContent(&content);

  if (nsnull != content) {
    ret = content->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, aStatus); 
    NS_RELEASE(content);
  }

  //Now dispatch to the frame
  if (nsnull != mCurrentTarget) {
    mCurrentTarget->HandleEvent(aPresContext, &event, aStatus);   
  }

  return ret;
}

PRBool
nsEventStateManager::ChangeFocus(nsIContent* aFocusContent, nsIFrame* aTargetFrame, PRBool aSetFocus)
{
  nsCOMPtr<nsIFocusableContent> focusChange = do_QueryInterface(aFocusContent);
  
  if (aTargetFrame) {
 
    PRBool suppressBlurAndFocus = PR_FALSE;
    nsCOMPtr<nsIStyleContext> context;
    aTargetFrame->GetStyleContext(getter_AddRefs(context));
  
    const nsStyleUserInterface* styleStruct = (const nsStyleUserInterface*)context->GetStyleData(eStyleStruct_UserInterface);
    if (NS_STYLE_USER_FOCUS_IGNORE == styleStruct->mUserFocus) {
      // we want to surpress the blur and the following focus
      suppressBlurAndFocus = PR_TRUE;
    }
  
    if(!suppressBlurAndFocus) {

      if (focusChange) {
        focusChange->SetFocus(mPresContext);
      }
      else {
        SendFocusBlur(mPresContext, nsnull);
      }
    }
  } 
  else {
    printf("OH MY GOD!\n");
  }
  
  return PR_FALSE;
} 

void
nsEventStateManager::ShiftFocus(PRBool forward)
{
  PRBool topOfDoc = PR_FALSE;

  if (nsnull == mPresContext) {
    return;
  }

  if (nsnull == mCurrentFocus) {
    if (nsnull == mDocument) {
      nsCOMPtr<nsIPresShell> presShell;
      mPresContext->GetShell(getter_AddRefs(presShell));
      if (presShell) {
        presShell->GetDocument(&mDocument);
        if (nsnull == mDocument) {
          return;
        }
      }
    }
    mCurrentFocus = mDocument->GetRootContent();
    if (nsnull == mCurrentFocus) {  
      return;
    }
    mCurrentTabIndex = forward ? 1 : 0;
    topOfDoc = PR_TRUE;
  }

  nsIContent* next = GetNextTabbableContent(mCurrentFocus, nsnull, mCurrentFocus, forward);

  if (nsnull == next) {
    PRBool focusTaken = PR_FALSE;

    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

    //Pass focus up to nsIWebShellContainer FocusAvailable
    nsISupports* container;
    mPresContext->GetContainer(&container);
    if (nsnull != container) {
      nsIWebShell* webShell;
      if (NS_OK == container->QueryInterface(kIWebShellIID, (void**)&webShell)) {
        nsIWebShellContainer* webShellContainer;
        webShell->GetContainer(webShellContainer);
        if (nsnull != webShellContainer) {
          webShellContainer->FocusAvailable(webShell, focusTaken);
          NS_RELEASE(webShellContainer);
        }
        NS_RELEASE(webShell);
      }
      NS_RELEASE(container);
    }
    if (!focusTaken && !topOfDoc) {
      ShiftFocus(forward);
    }
    return;
  }

  // Now we need to get the content node's frame and reset 
  // the mCurrentTarget target.  Otherwise the focus
  // code will be slightly out of sync (with regards to
  // focusing widgets within the current target).
  nsCOMPtr<nsIPresShell> shell;
  if (mPresContext) {
    nsresult rv = mPresContext->GetShell(getter_AddRefs(shell));
    if (NS_SUCCEEDED(rv) && shell){
      shell->GetPrimaryFrameFor(next, &mCurrentTarget);
    }
  }
  ChangeFocus(next, mCurrentTarget, PR_TRUE);

  NS_IF_RELEASE(mCurrentFocus);
  mCurrentFocus = next;
}

/*
 * At some point this will need to be linked into HTML 4.0 tabindex
 */
nsIContent*
nsEventStateManager::GetNextTabbableContent(nsIContent* aParent, nsIContent* aChild, nsIContent* aTop, PRBool forward)
{
  PRInt32 count, index;
  aParent->ChildCount(count);

  if (nsnull != aChild) {
    aParent->IndexOf(aChild, index);
    index += forward ? 1 : -1;
  }
  else {
    index = forward ? 0 : count-1;
  }

  for (;index < count && index >= 0;index += forward ? 1 : -1) {
    nsIContent* child;

    aParent->ChildAt(index, child);
    nsIContent* content = GetNextTabbableContent(child, nsnull, aTop, forward);
    if (content != nsnull) {
      NS_IF_RELEASE(child);
      return content;
    }
    if (nsnull != child) {
      nsCOMPtr<nsIAtom> tag;
      PRInt32 tabIndex = -1;
      PRBool disabled = PR_TRUE;
      PRBool hidden = PR_FALSE;

      child->GetTag(*getter_AddRefs(tag));
      if (nsHTMLAtoms::input==tag.get()) {
        nsIDOMHTMLInputElement *nextInput;
        if (NS_OK == child->QueryInterface(kIDOMHTMLInputElementIID,(void **)&nextInput)) {
          nextInput->GetDisabled(&disabled);
          nextInput->GetTabIndex(&tabIndex);

          nsAutoString type;
          nextInput->GetType(type);
          if (type.EqualsIgnoreCase("hidden")) {
            hidden = PR_TRUE;
          }
          NS_RELEASE(nextInput);
        }
      }
      else if (nsHTMLAtoms::select==tag.get()) {
        nsIDOMHTMLSelectElement *nextSelect;
        if (NS_OK == child->QueryInterface(kIDOMHTMLSelectElementIID,(void **)&nextSelect)) {
          nextSelect->GetDisabled(&disabled);
          nextSelect->GetTabIndex(&tabIndex);
          NS_RELEASE(nextSelect);
        }
      }
      else if (nsHTMLAtoms::textarea==tag.get()) {
        nsIDOMHTMLTextAreaElement *nextTextArea;
        if (NS_OK == child->QueryInterface(kIDOMHTMLTextAreaElementIID,(void **)&nextTextArea)) {
          nextTextArea->GetDisabled(&disabled);
          nextTextArea->GetTabIndex(&tabIndex);
          NS_RELEASE(nextTextArea);
        }

      }
      else if(nsHTMLAtoms::a==tag.get()) {
        nsIDOMHTMLAnchorElement *nextAnchor;
        if (NS_OK == child->QueryInterface(kIDOMHTMLAnchorElementIID,(void **)&nextAnchor)) {
          nextAnchor->GetTabIndex(&tabIndex);
          NS_RELEASE(nextAnchor);
        }
        disabled = PR_FALSE;
      }
      else if(nsHTMLAtoms::button==tag.get()) {
        nsIDOMHTMLButtonElement *nextButton;
        if (NS_OK == child->QueryInterface(kIDOMHTMLButtonElementIID,(void **)&nextButton)) {
          nextButton->GetTabIndex(&tabIndex);
          nextButton->GetDisabled(&disabled);
          NS_RELEASE(nextButton);
        }
      }
      else if(nsHTMLAtoms::area==tag.get()) {
        nsIDOMHTMLAreaElement *nextArea;
        if (NS_OK == child->QueryInterface(kIDOMHTMLAreaElementIID,(void **)&nextArea)) {
          nextArea->GetTabIndex(&tabIndex);
          NS_RELEASE(nextArea);
        }
        disabled = PR_FALSE;
      }
      else if(nsHTMLAtoms::object==tag.get()) {
        nsIDOMHTMLObjectElement *nextObject;
        if (NS_OK == child->QueryInterface(kIDOMHTMLObjectElementIID,(void **)&nextObject)) {
          nextObject->GetTabIndex(&tabIndex);
          NS_RELEASE(nextObject);
        }
        disabled = PR_FALSE;
      }

      //TabIndex not set (-1) treated at same level as set to 0
      tabIndex = tabIndex < 0 ? 0 : tabIndex;

      if (!disabled && !hidden && mCurrentTabIndex == tabIndex) {
        return child;
      }
      NS_RELEASE(child);
    }
  }

  if (aParent == aTop) {
    nsIContent* nextParent;
    aParent->GetParent(nextParent);
    if (nsnull != nextParent) {
      nsIContent* content = GetNextTabbableContent(nextParent, aParent, nextParent, forward);
      NS_RELEASE(nextParent);
      return content;
    }
    //Reached end of document
    else {
      //If already at lowest priority tab (0), end
      if (((forward) && (0 == mCurrentTabIndex)) ||
          ((!forward) && (1 == mCurrentTabIndex))) {
        return nsnull;
      }
      //else continue looking for next highest priority tab
      mCurrentTabIndex = GetNextTabIndex(aParent, forward);
      nsIContent* content = GetNextTabbableContent(aParent, nsnull, aParent, forward);
      return content;
    }
  }

  return nsnull;
}

PRInt32
nsEventStateManager::GetNextTabIndex(nsIContent* aParent, PRBool forward)
{
  PRInt32 count, tabIndex, childTabIndex;
  nsIContent* child;
  
  aParent->ChildCount(count);
 
  if (forward) {
    tabIndex = 0;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, child);
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
      NS_RELEASE(child);
    }
  } 
  else { /* !forward */
    tabIndex = 1;
    for (PRInt32 index = 0; index < count; index++) {
      aParent->ChildAt(index, child);
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
      NS_RELEASE(child);
    }
  }
  return tabIndex;
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

  *aFrame = mCurrentTarget;
  return NS_OK;
}

// This API crosses ESM instances to give the currently focused frame, no matter what ESM instace
// you call it from.
NS_IMETHODIMP
nsEventStateManager::GetFocusedEventTarget(nsIFrame **aFrame)
{
  if (!gCurrentlyFocusedTargetFrame && gCurrentlyFocusedContent && gLastFocusedPresContext) {
	nsCOMPtr<nsIPresShell> shell;
    if (gLastFocusedPresContext) {
      nsresult rv = gLastFocusedPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        shell->GetPrimaryFrameFor(gCurrentlyFocusedContent, &gCurrentlyFocusedTargetFrame);
      }
    }
  }

  *aFrame = gCurrentlyFocusedTargetFrame;
  return NS_OK;
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
  
  if (mCurrentTarget) {
    mCurrentTarget->GetContent(aContent);
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
  if (aContent == mHoverContent) {
    aState |= NS_EVENT_STATE_HOVER;
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

  if ((aState & NS_EVENT_STATE_HOVER) && (aContent != mHoverContent)) {
    //transferring ref to notifyContent from mHoverContent
    notifyContent[1] = mHoverContent; // notify hover first, since more common case
    mHoverContent = aContent;
    NS_IF_ADDREF(mHoverContent);
  }

  if ((aState & NS_EVENT_STATE_FOCUS)) {
    if (aContent == mCurrentFocus) {
      //printf("NOOOOOOOOO!\n");
    } else {
      SendFocusBlur(mPresContext, aContent);

      //transferring ref to notifyContent from mCurrentFocus
      notifyContent[3] = mCurrentFocus;
      mCurrentFocus = aContent;
      NS_IF_ADDREF(mCurrentFocus);

	    gCurrentlyFocusedContent = mCurrentFocus;
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

  if (notifyContent[0]) { // have at least one to notify about
    nsIDocument *document;  // this presumes content can't get/lose state if not connected to doc
    notifyContent[0]->GetDocument(document);
    if (document) {
      document->ContentStatesChanged(notifyContent[0], notifyContent[1]);
      if (notifyContent[2]) {  // more that two notifications are needed (should be rare)
        // XXX a further optimization here would be to group the notification pairs
        // together by parent/child, only needed if more than two content changed
        // (ie: if [0] and [2] are parent/child, then notify (0,2) (1,3))
        document->ContentStatesChanged(notifyContent[2], notifyContent[3]);
        if (notifyContent[4]) {  // more that two notifications are needed (should be rare)
          document->ContentStatesChanged(notifyContent[4], nsnull);
        }
      }
      NS_RELEASE(document);
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
  //PRBool suppressBlurAndFocus = PR_FALSE;

  if(mCurrentTarget) {
    nsCOMPtr<nsIStyleContext> context;
    mCurrentTarget->GetStyleContext(getter_AddRefs(context));
    if(!context) return NS_OK;

 
    const nsStyleUserInterface* styleStruct = (const nsStyleUserInterface*)context->GetStyleData(eStyleStruct_UserInterface);
    if (NS_STYLE_USER_FOCUS_IGNORE == styleStruct->mUserFocus) {
      // we want to suppress the blur and the following focus
      return NS_OK;
	  }
  }

  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
    
  if (nsnull != gLastFocusedPresContext) {
    
    if (gLastFocusedContent) { 

      // Retrieve this content node's pres context. it can be out of sync in
      // the Ender widget case.
      nsCOMPtr<nsIDocument> doc;
      gLastFocusedContent->GetDocument(*getter_AddRefs(doc));
      if (doc) {
        nsCOMPtr<nsIPresShell> shell = getter_AddRefs(doc->GetShellAt(0));
        nsCOMPtr<nsIPresContext> oldPresContext;
        shell->GetPresContext(getter_AddRefs(oldPresContext));

        //fire blur
        nsEventStatus status = nsEventStatus_eIgnore;
        nsEvent event;
        event.eventStructType = NS_EVENT;
        event.message = NS_BLUR_CONTENT;

        nsCOMPtr<nsIEventStateManager> esm;
        oldPresContext->GetEventStateManager(getter_AddRefs(esm));
        esm->SetFocusedContent(gLastFocusedContent);
        gLastFocusedContent->HandleDOMEvent(oldPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
        esm->SetFocusedContent(nsnull);
      }
    }

    // Go ahead and fire a blur on the window.
    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    gLastFocusedDocument->GetScriptGlobalObject(getter_AddRefs(globalObject));
  
    if (!mDocument) {  
      if (presShell) {
        presShell->GetDocument(&mDocument);
      }
    }

    if ((gLastFocusedDocument != mDocument) && globalObject) {  
      nsEventStatus status = nsEventStatus_eIgnore;
      nsEvent event;
      event.eventStructType = NS_EVENT;
      event.message = NS_BLUR_CONTENT;

      nsCOMPtr<nsIEventStateManager> esm;
      gLastFocusedPresContext->GetEventStateManager(getter_AddRefs(esm));
      esm->SetFocusedContent(nsnull);
      gLastFocusedDocument->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
      globalObject->HandleDOMEvent(gLastFocusedPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
    }
  }
  
  NS_IF_RELEASE(gLastFocusedContent);
  NS_IF_RELEASE(mCurrentFocus);
  gLastFocusedContent = aContent;
  mCurrentFocus = aContent;
  NS_IF_ADDREF(aContent);
  NS_IF_ADDREF(aContent);
 
  if (nsnull != aContent) {
    //fire focus
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FOCUS_CONTENT;

    NS_IF_RELEASE(mCurrentFocus);
    mCurrentFocus = aContent;
    NS_IF_ADDREF(mCurrentFocus);

    if (nsnull != mPresContext) {
      aContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
    }
    
    nsAutoString tabIndex;
    aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::tabindex, tabIndex);
    PRInt32 ec, val = tabIndex.ToInteger(&ec);
    if (NS_OK == ec) {
      mCurrentTabIndex = val;
    }
  }

  // XXx The following line could be bad in the case where a
  // parent element is really the one with the focus.
  nsIFrame * currentFocusFrame = mCurrentTarget;

  // Find the window that this frame is in and
  // make sure it has focus
  // GFX Text Control frames handle their own focus
  // and must be special-cased.
  nsCOMPtr<nsIGfxTextControlFrame> gfxFrame = do_QueryInterface(currentFocusFrame);
  if (gfxFrame) {
    gfxFrame->SetInnerFocus();
  } 
  else if (currentFocusFrame) {
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
	          break;
	        }
	        ancestor->GetParent(ancestor);
        }
      }
    }
  }
  
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

nsIFrame*
nsEventStateManager::GetDocumentFrame(nsIPresContext* aPresContext)
{
  nsCOMPtr<nsIPresShell> presShell;
  nsIDocument* aDocument;
  nsIFrame* aFrame;
  nsIView* aView;

  aPresContext->GetShell(getter_AddRefs(presShell));
  if (nsnull == presShell) {
#ifdef DEBUG_scroll
    printf("Got a null PresShell\n");
#endif
    return nsnull;
  }
  presShell->GetDocument(&aDocument);
  presShell->GetPrimaryFrameFor(aDocument->GetRootContent(), &aFrame);

  aFrame->GetView(aPresContext, &aView);
  if (!aView) {
#ifdef DEBUG_scroll
    printf("looking for a parent with a view\n");
#endif
    aFrame->GetParentWithView(aPresContext, &aFrame);
  }
  return aFrame;
}

// There are three posibilities for what this function returns:
//  sv and focusView non-null, sf null  (a view should be scrolled)
//  sv and focusView null, sf non-null  (a frame should be scrolled)
//  sv, focusView, and sf all null (nothing to scroll)
// The location works like this:
//  First, check for a focused frame and try to get its view.
//    If we can, and can get an nsIScrollableView for it, use that.
//  If there is a focused frame but it does not have a view, or it has
//   a view but no nsIScrollableView, check for an nsISelfScrollingFrame.
//    If we find one, use that.
//  If there is no focused frame, we first look for an nsISelfScrollingFrame
//   as an ancestor of the event target.  If there isn't one, we try to get
//   an nsIView corresponding to the main document.
// Confused yet?

nsresult nsEventStateManager::GetScrollableFrameOrView(nsIPresContext* aPresContext,
                                                       nsIFrame* aTargetFrame,
                                                       nsIView* aView,
                                                       nsIScrollableView* &sv,
                                                       nsISelfScrollingFrame* &sf,
                                                       nsIView* &focusView)
{
  nsIFrame* focusFrame = nsnull;

  nsCOMPtr<nsIPresShell> presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));
  if (!presShell)    // this is bad
    {
      sv = nsnull;
      sf = nsnull;
      focusView = nsnull;
      return NS_OK;
    }

#ifdef DEBUG_scroll
  printf("GetScrollableFrameOrView: aTargetFrame = %p, aView = %p\n",
         aTargetFrame, aView);
#endif
          
  if (mCurrentFocus) {
#ifdef DEBUG_scroll
    printf("GetScrollableFrameOrView: mCurrentFocus = %p\n", mCurrentFocus);
#endif
    
    presShell->GetPrimaryFrameFor(mCurrentFocus, &focusFrame);
    if (focusFrame)
      focusFrame->GetView(aPresContext, &focusView);
  }
          
  if (focusFrame) {
    if (!focusView) {
      // The focused frame doesn't have a view
      // Revert to the parameters passed in
      // XXX this might not be right
      
#ifdef DEBUG_scroll
      printf("Could not get a view for the focused frame\n");
#endif
      
      focusFrame = aTargetFrame;
      focusView = aView;
    }
  } else {
    // Focus is null.  This means the main document has the focus,
    // and we should scroll that.
    
#ifdef DEBUG_scroll
    printf("GetScrollableFrameOrView: mCurrentFocus = NULL\n");
#endif
    
    // If we can get an nsISelfScrollingFrame, that is preferable to getting
    // the document view

    sf = GetParentSelfScrollingFrame(aTargetFrame);
    if (sf) {
#ifdef DEBUG_scroll
      printf("Found a SelfScrollingFrame: sf = %p\n", sf);
#endif
      focusView = aView;
    } else {
      focusFrame = GetDocumentFrame(aPresContext);
      focusFrame->GetView(aPresContext, &focusView);
      
#ifdef DEBUG_scroll
      if (focusView)
        printf("Got view for document frame!\n");
      else      // hopefully we won't be here
        printf("Couldn't get view for document frame\n");
#endif
      
    }
  }

#ifdef DEBUG_scroll
  printf("GetScrollableFrameOrView: focusFrame = %p, focusView = %p\n",
         focusFrame, focusView);
#endif
  
  sv = GetNearestScrollingView(focusView);

  if (sv) {
#ifdef DEBUG_scroll
    printf("Found a ScrollingView\n");
#endif

   // We can stop now
    sf = nsnull;
    return NS_OK;
  }
  else {
#ifdef DEBUG_scroll
    printf("No scrolling view, looking for a scrolling frame\n");
#endif
    if (!sf)
      sf = GetParentSelfScrollingFrame(aTargetFrame);

    if (sf) {
#ifdef DEBUG_scroll
      printf("Found a scrolling frame\n");
#endif

      sv = nsnull;
      focusView = nsnull;
      return NS_OK;

    }
    else {
#ifdef DEBUG_scroll
      printf("Could not find a scrolling frame\n");
#endif
      sf = nsnull;
      sv = nsnull;
      focusView = nsnull;
      return NS_OK;
    }
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
    nsIView* rootView = nsnull;
    if (NS_OK == vm->GetRootView(rootView) && nsnull != rootView) {
      nsIWidget* rootWidget = nsnull;
      if (NS_OK == rootView->GetWidget(rootWidget) &&
          nsnull != rootWidget) {
        rootWidget->Update();
        NS_RELEASE(rootWidget);
      }
    }
    NS_RELEASE(vm);
  }
}

nsresult NS_NewEventStateManager(nsIEventStateManager** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIEventStateManager* manager = new nsEventStateManager();
  if (nsnull == manager) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return manager->QueryInterface(NS_GET_IID(nsIEventStateManager), (void **) aInstancePtrResult);
}

