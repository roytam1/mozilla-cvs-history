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

#ifndef nsEventStateManager_h__
#define nsEventStateManager_h__

#include "nsIEventStateManager.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIPref.h"
#include "nsIObserver.h"

class nsIDocument;
class nsIScrollableView;
class nsISelfScrollingFrame;

#undef USE_FOCUS_FOR_MOUSEWHEEL // experimental code to use only the focus
                                // system to track the mousewheel

/*
 * Event listener manager
 */

class nsEventStateManager : public nsIEventStateManager,
                            public nsIObserver
{

public:
  nsEventStateManager();
  virtual ~nsEventStateManager();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NS_IMETHOD Init();
  nsresult Shutdown();

  /* The PreHandleEvent method is called before event dispatch to either
   * the DOM or frames.  Any processing which must not be prevented or
   * cancelled should occur here.  Any processing which is intended to
   * be conditional based on either DOM or frame processing should occur in
   * PostHandleEvent.  Any centralized event processing which must occur before
   * DOM or frame event handling should occur here as well.
   */
  NS_IMETHOD PreHandleEvent(nsIPresContext* aPresContext,
                         nsGUIEvent *aEvent,
                         nsIFrame* aTargetFrame,
                         nsEventStatus* aStatus,
                         nsIView* aView);

  /* The PostHandleEvent method should contain all system processing which
   * should occur conditionally based on DOM or frame processing.  It should
   * also contain any centralized event processing which must occur after
   * DOM and frame processing.
   */
  NS_IMETHOD PostHandleEvent(nsIPresContext* aPresContext,
                         nsGUIEvent *aEvent,
                         nsIFrame* aTargetFrame,
                         nsEventStatus* aStatus,
                         nsIView* aView);

  NS_IMETHOD SetPresContext(nsIPresContext* aPresContext);
  NS_IMETHOD ClearFrameRefs(nsIFrame* aFrame);

  NS_IMETHOD GetEventTarget(nsIFrame **aFrame);
  NS_IMETHOD GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent);
  NS_IMETHOD GetEventRelatedContent(nsIContent** aContent);

  NS_IMETHOD GetContentState(nsIContent *aContent, PRInt32& aState);
  NS_IMETHOD SetContentState(nsIContent *aContent, PRInt32 aState);
  NS_IMETHOD GetFocusedContent(nsIContent **aContent);
  NS_IMETHOD SetFocusedContent(nsIContent* aContent);

  // This is an experiement and may be temporary
  NS_IMETHOD ConsumeFocusEvents(PRBool aDoConsume) { mConsumeFocusEvents = aDoConsume; return NS_OK; }

  // Access Key Registration
  NS_IMETHOD RegisterAccessKey(nsIFrame * aFrame, PRUint32 aKey);
  NS_IMETHOD UnregisterAccessKey(nsIFrame * aFrame);

protected:
  void UpdateCursor(nsIPresContext* aPresContext, nsEvent* aEvent, nsIFrame* aTargetFrame, nsEventStatus* aStatus);
  void GenerateMouseEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent);
  void GenerateDragDropEnterExit(nsIPresContext* aPresContext, nsGUIEvent* aEvent);
  NS_IMETHOD SetClickCount(nsIPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);  
  NS_IMETHOD CheckForAndDispatchClick(nsIPresContext* aPresContext, nsMouseEvent *aEvent, nsEventStatus* aStatus);  
  PRBool ChangeFocus(nsIContent* aFocus, nsIFrame* aFocusFrame, PRBool aSetFocus);
  void ShiftFocus(PRBool foward);
  NS_IMETHOD GetNextTabbableContent(nsIContent* aRootContent, nsIFrame* aFrame, PRBool foward, nsIContent** aResult);
  PRInt32 GetNextTabIndex(nsIContent* aParent, PRBool foward);
  NS_IMETHOD SendFocusBlur(nsIPresContext* aPresContext, nsIContent *aContent);
  PRBool CheckDisabled(nsIContent* aContent);

  // These functions are all for mousewheel scrolling
  nsISelfScrollingFrame* GetParentSelfScrollingFrame(nsIFrame* aFrame);
  nsIScrollableView* GetNearestScrollingView(nsIView* aView);
#ifdef USE_FOCUS_FOR_MOUSEWHEEL
  nsresult  GetScrollableFrameOrView(nsIScrollableView* &sv,
                                     nsISelfScrollingFrame* &sf,
                                     nsIView* &focusView);
#else

  // This function MAY CHANGE the PresContext that you pass into it.  It
  // will be changed to the PresContext for the main document.  If the
  // new PresContext differs from the one you passed in, you should
  // be sure to release the new one.

  nsIFrame* GetDocumentFrame(nsIPresContext* &aPresContext);

  // This function may call GetDocumentFrame, so read the warning above
  // regarding the PresContext that you pass into this function.

  nsresult  GetScrollableFrameOrView(nsIPresContext* &aPresContext,
                                     nsIFrame* aTargetFrame, nsIView* aView,
                                     nsIScrollableView* &sv,
                                     nsISelfScrollingFrame* &sf,
                                     nsIView* &focusView);
#endif
  void ForceViewUpdate(nsIView* aView);
  nsresult getPrefService();
  // end mousewheel functions

  // routines for the d&d gesture tracking state machine
  void BeginTrackingDragGesture ( nsGUIEvent* inDownEvent, nsIFrame* inDownFrame ) ;
  void StopTrackingDragGesture ( ) ;
  void GenerateDragGesture ( nsIPresContext* aPresContext, nsGUIEvent *aEvent ) ;
  PRBool IsTrackingDragGesture ( ) const { return mIsTrackingDragGesture; }

  PRBool mSuppressFocusChange; // Used only for Ender text fields to suppress a focus firing on mouse down

  //Any frames here must be checked for validity in ClearFrameRefs
  nsIFrame* mCurrentTarget;
  nsIContent* mCurrentTargetContent;
  nsIContent* mCurrentRelatedContent;
  nsIFrame* mLastMouseOverFrame;
  nsCOMPtr<nsIContent> mLastMouseOverContent;
  nsIFrame* mLastDragOverFrame;
  
  // member variables for the d&d gesture state machine
  PRBool mIsTrackingDragGesture;
  nsPoint mGestureDownPoint;
  nsIFrame* mGestureDownFrame;

  nsIContent* mLastLeftMouseDownContent;
  nsIContent* mLastMiddleMouseDownContent;
  nsIContent* mLastRightMouseDownContent;

  nsIContent* mActiveContent;
  nsIContent* mHoverContent;
  nsIContent* mDragOverContent;
  nsIContent* mCurrentFocus;
  PRInt32 mCurrentTabIndex;
  nsIWidget * mLastWindowToHaveFocus; // last native window to get focus via the evs
  PRBool      mConsumeFocusEvents;

  //Anti-recursive stack controls
  nsIContent* mFirstBlurEvent;
  nsIContent* mFirstFocusEvent;
  nsCOMPtr<nsIContent> mFirstMouseOverEventContent;
  nsCOMPtr<nsIContent> mFirstMouseOutEventContent;

  nsIPresContext* mPresContext;      // Not refcnted
  nsIDocument* mDocument;            // [OWNER], but doesn't need to be.

  PRUint32 mLClickCount;
  PRUint32 mMClickCount;
  PRUint32 mRClickCount;

  static PRUint32 mInstanceCount;

  // For mousewheel preferences handling
  nsCOMPtr<nsIPref> mPrefService;
  PRBool m_haveShutdown;
};

extern nsresult NS_NewEventStateManager(nsIEventStateManager** aInstancePtrResult);

#endif // nsEventStateManager_h__
