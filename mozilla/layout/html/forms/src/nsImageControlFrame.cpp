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

#include "nsHTMLImage.h"
#include "nsIFormControlFrame.h"
#include "nsIFormControl.h"
#include "nsHTMLParts.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsIViewManager.h"
#include "nsISupports.h"
#include "nsHTMLAtoms.h"
#include "nsIView.h"
#include "nsViewsCID.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIImage.h"
#include "nsStyleUtil.h"
#include "nsDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsFormFrame.h"

//Enumeration of possible mouse states used to detect mouse clicks
enum nsMouseState {
  eMouseNone,
  eMouseEnter,
  eMouseDown,
  eMouseExit
};

static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID);
static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

class nsImageControlFrame : public ImageFrame,
                            public nsIFormControlFrame 
{
public:
  nsImageControlFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  NS_IMETHOD Init(nsIPresContext& aPresContext, nsIFrame* aChildList);

  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus& aEventStatus);

  virtual void MouseClicked(nsIPresContext* aPresContext);

  virtual void SetFormFrame(nsFormFrame* aFormFrame) { mFormFrame = aFormFrame; }

  PRBool IsSuccessful(nsIFormControlFrame* aSubmitter);

  virtual PRInt32 GetMaxNumValues();

  virtual PRBool GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                nsString* aValues, nsString* aNames);

  NS_IMETHOD GetType(PRInt32* aType) const;

  NS_IMETHOD GetName(nsString* aName);

  virtual void Reset() {};

  void SetFocus(PRBool aOn, PRBool aRepaint);
protected:
  virtual  ~nsImageControlFrame();
  void GetTranslatedRect(nsRect& aRect); // XXX this implementation is a copy of nsHTMLButtonControlFrame
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  nsFormFrame* mFormFrame;
  nsMouseState mLastMouseState;
  nsPoint mLastClickPoint; 
  nsCursor mPreviousCursor;
  nsRect mTranslatedRect;
  PRBool mGotFocus;
};


nsImageControlFrame::nsImageControlFrame(nsIContent* aContent,
                                           nsIFrame* aParentFrame)
  : ImageFrame(aContent, aParentFrame)
{
  mLastMouseState = eMouseNone;
  mLastClickPoint = nsPoint(0,0);
  mPreviousCursor = eCursor_standard;
  mTranslatedRect = nsRect(0,0,0,0);
  mGotFocus = PR_FALSE;
}

nsImageControlFrame::~nsImageControlFrame()
{
}

nsresult
NS_NewImageControlFrame(nsIContent* aContent,
                        nsIFrame*   aParent,
                        nsIFrame*&  aResult)
{
  aResult = new nsImageControlFrame(aContent, aParent);
  if (nsnull == aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult
nsImageControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIFormControlFrameIID)) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  return ImageFrame::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt nsImageControlFrame::AddRef(void)
{
  NS_WARNING("not supported");
  return 1;
}

nsrefcnt nsImageControlFrame::Release(void)
{
  NS_WARNING("not supported");
  return 1;
}

NS_IMETHODIMP
nsImageControlFrame::Init(nsIPresContext& aPresContext, nsIFrame* aChildList)
{
  nsFormFrame::AddFormControlFrame(aPresContext, *this);
  if (nsnull == mFormFrame) {
    return NS_OK;
  }
  // add ourself as an nsIFormControlFrame
  nsFormFrame::AddFormControlFrame(aPresContext, *this);

  // create our view, we need a view to grab the mouse 
  nsIView* view;
  GetView(view);
  if (!view) {
    nsresult result = nsRepository::CreateInstance(kViewCID, nsnull, kIViewIID, (void **)&view);
	  nsIPresShell   *presShell = aPresContext.GetShell();     
	  nsIViewManager *viewMan   = presShell->GetViewManager();  
    NS_RELEASE(presShell);

    nsIFrame* parWithView;
	  nsIView *parView;
    GetParentWithView(parWithView);
	  parWithView->GetView(parView);
    // the view's size is not know yet, but its size will be kept in synch with our frame.
    nsRect boundBox(0, 0, 500, 500); 
    result = view->Init(viewMan, boundBox, parView, nsnull);
    viewMan->InsertChild(parView, view, 0);
    SetView(view);

    const nsStyleColor* color = (const nsStyleColor*) mStyleContext->GetStyleData(eStyleStruct_Color);
    // set the opacity
    viewMan->SetViewOpacity(view, color->mOpacity);

    NS_RELEASE(viewMan);
  }
  return NS_OK;
}

NS_METHOD 
nsImageControlFrame::HandleEvent(nsIPresContext& aPresContext, 
                                 nsGUIEvent* aEvent,
                                 nsEventStatus& aEventStatus)
{
  if (nsFormFrame::GetDisabled(this)) { // XXX cache disabled
    return NS_OK;
  }

  aEventStatus = nsEventStatus_eIgnore;
  nsresult result = NS_OK;

  nsIView* view;
  GetView(view);
  if (view) {
    nsIViewManager* viewMan;
    view->GetViewManager(viewMan);
    if (viewMan) {
      nsIView* grabber;
      viewMan->GetMouseEventGrabber(grabber);
      if ((grabber == view) || (nsnull == grabber)) {
        nsIWidget* window;
        PRBool ignore;

        switch (aEvent->message) {
        case NS_MOUSE_ENTER: // not implemented yet on frames, 1st mouse move simulates it
	        mLastMouseState = eMouseEnter;
	        break;
        case NS_MOUSE_LEFT_BUTTON_DOWN:
	        mLastMouseState = eMouseDown;
          mGotFocus = PR_TRUE;
	        break;
        case NS_MOUSE_MOVE:
          //printf ("%d mRect=(%d,%d,%d,%d), x=%d, y=%d \n", foo++, mRect.x, mRect.y, mRect.width, mRect.height, aEvent->point.x, aEvent->point.y);
          
          if (nsnull == grabber) { // simulated mouse enter
            GetTranslatedRect(mTranslatedRect);
            //printf("%d enter\n", foo++);
            viewMan->GrabMouseEvents(view, ignore);
            GetWindow(window);
            if (window) {
              mPreviousCursor = window->GetCursor(); 
              window->SetCursor(eCursor_select); // set it to something else to work around bug 
              window->SetCursor(eCursor_standard); 
              NS_RELEASE(window);
            }
            mLastMouseState = eMouseEnter;
          // simulated mouse exit
          } else if (!mTranslatedRect.Contains(aEvent->point)) {
            viewMan->GrabMouseEvents(nsnull, ignore); 
            GetWindow(window);
            if (window) {
              window->SetCursor(mPreviousCursor);  
              NS_RELEASE(window);
            }
            mLastMouseState = eMouseExit;
          }
          break;
        case NS_MOUSE_LEFT_BUTTON_UP: 
          {
            if (eMouseDown == mLastMouseState) {
              nsEventStatus status = nsEventStatus_eIgnore;
              nsMouseEvent event;
              event.eventStructType = NS_MOUSE_EVENT;
              event.message = NS_MOUSE_LEFT_CLICK;
              mContent->HandleDOMEvent(aPresContext, &event, nsnull, DOM_EVENT_INIT, status);
        
              float t2p = aPresContext.GetTwipsToPixels();
              mLastClickPoint.x = NSTwipsToIntPixels(aEvent->point.x - mTranslatedRect.x, t2p);
              mLastClickPoint.y = NSTwipsToIntPixels(aEvent->point.y - mTranslatedRect.y, t2p);

              if (nsEventStatus_eConsumeNoDefault != status) {
                MouseClicked(&aPresContext);
              }
	            mLastMouseState = eMouseEnter;
	          } 
	          break;
          }
        case NS_MOUSE_EXIT: // doesn't work for frames, yet
	        break;
        }
        aEventStatus = nsEventStatus_eConsumeNoDefault;
        NS_RELEASE(viewMan);
        return NS_OK;
      }
    }
  }
  return ImageFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void 
nsImageControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  mGotFocus = aOn;
  if (aRepaint) {
    nsRect rect(0, 0, mRect.width, mRect.height);
    Invalidate(rect, PR_TRUE);
  }
}

void
nsImageControlFrame::GetTranslatedRect(nsRect& aRect)
{
  nsIView* view;
  nsPoint viewOffset(0,0);
  GetOffsetFromView(viewOffset, view);
  while (nsnull != view) {
    nsPoint tempOffset;
    view->GetPosition(&tempOffset.x, &tempOffset.y);
    viewOffset += tempOffset;
    view->GetParent(view);
  }
  aRect = nsRect(viewOffset.x, viewOffset.y, mRect.width, mRect.height);
}

NS_IMETHODIMP
nsImageControlFrame::GetType(PRInt32* aType) const
{
  *aType = NS_FORM_INPUT_IMAGE;
  return NS_OK;
}

NS_IMETHODIMP
nsImageControlFrame::GetName(nsString* aResult)
{
  if (nsnull == aResult) {
    return NS_OK;
  } else {
    return nsFormFrame::GetName(this, *aResult);
  }
}

PRBool
nsImageControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  if (this == (aSubmitter)) {
    nsAutoString name;
    return (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
  }
  return PR_FALSE;
}

PRInt32
nsImageControlFrame::GetMaxNumValues() 
{
  return 2;
}


PRBool
nsImageControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                     nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != result)) {
    return PR_FALSE;
  }

  nsAutoString value;
  nsresult valResult = nsFormFrame::GetValue(this, value);

  char buf[20];
  aNumValues = 2;

  aValues[0].SetLength(0);
  sprintf(&buf[0], "%d", mLastClickPoint.x);
  aValues[0].Append(&buf[0]);

  aNames[0] = name;
  aNames[0].Append(".x");

  aValues[1].SetLength(0);
  sprintf(&buf[0], "%d", mLastClickPoint.y);
  aValues[1].Append(&buf[0]);

  aNames[1] = name;
  aNames[1].Append(".y");

  return PR_TRUE;
}


void
nsImageControlFrame::MouseClicked(nsIPresContext* aPresContext) 
{
  PRInt32 type;
  GetType(&type);

  if ((nsnull != mFormFrame) && !nsFormFrame::GetDisabled(this)) {
    nsEventStatus status;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    event.message = NS_FORM_SUBMIT;
    mContent->HandleDOMEvent(*aPresContext, &event, nsnull, DOM_EVENT_INIT, status); 
    if (nsEventStatus_eConsumeNoDefault != status) {
      mFormFrame->OnSubmit(aPresContext, this);
    }
  } 
}

