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

#include "nsHTMLContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsHTMLParts.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsFormFrame.h"

#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
#include "nsISupports.h"
#include "nsHTMLAtoms.h"
#include "nsIImage.h"
#include "nsHTMLImage.h"
#include "nsStyleUtil.h"
#include "nsDOMEvent.h"
#include "nsIDOMHTMLCollection.h"
#include "nsStyleConsts.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIWidget.h"
#include "nsRepository.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsViewsCID.h"
#include "nsColor.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"

//Enumeration of possible mouse states used to detect mouse clicks
enum nsMouseState {
  eMouseNone,
  eMouseEnter,
  eMouseDown,
  eMouseExit
};

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID);
static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kIFormIID, NS_IFORM_IID);

class nsLabelFrame : public nsHTMLContainerFrame
{
public:
  nsLabelFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD Init(nsIPresContext& aPresContext, nsIFrame* aChildList);

  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus& aEventStatus);

  NS_IMETHOD GetFor(nsString& aFor);

protected:
  PRBool FindFirstControl(nsIFrame* aParentFrame, nsIFormControlFrame*& aResultFrame);
  PRBool FindForControl(nsIFormControlFrame*& aResultFrame);
  void GetTranslatedRect(nsRect& aRect);

  virtual  ~nsLabelFrame();
  PRIntn GetSkipSides() const;
  PRBool mInline;
  nsCursor mPreviousCursor;
  nsMouseState mLastMouseState;
  PRBool mControlIsInside;
  nsIFormControlFrame* mControlFrame;
  nsRect mTranslatedRect;
};

nsresult
NS_NewLabelFrame(nsIContent* aContent,
                 nsIFrame*   aParent,
                 nsIFrame*&  aResult)
{
  aResult = new nsLabelFrame(aContent, aParent);
  if (nsnull == aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsLabelFrame::nsLabelFrame(nsIContent* aContent,
                                           nsIFrame* aParentFrame)
  : nsHTMLContainerFrame(aContent, aParentFrame)
{
  mInline          = PR_TRUE;
  mLastMouseState  = eMouseNone;
  mPreviousCursor  = eCursor_standard;
  mControlIsInside = PR_FALSE;
  mControlFrame    = nsnull;
  mTranslatedRect = nsRect(0,0,0,0);
}

nsLabelFrame::~nsLabelFrame()
{
}


void
nsLabelFrame::GetTranslatedRect(nsRect& aRect)
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

            
NS_METHOD 
nsLabelFrame::HandleEvent(nsIPresContext& aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus& aEventStatus)
{
  if (!mControlFrame) {
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
          mControlFrame->SetFocus(PR_TRUE);
 	        mLastMouseState = eMouseDown;
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
            //printf("%d exit\n", foo++);
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
	        if (eMouseDown == mLastMouseState) {
            nsEventStatus status = nsEventStatus_eIgnore;
            nsMouseEvent event;
            event.eventStructType = NS_MOUSE_EVENT;
            event.message = NS_MOUSE_LEFT_CLICK;
            mContent->HandleDOMEvent(aPresContext, &event, nsnull, DOM_EVENT_INIT, status);
        
            if (nsEventStatus_eConsumeNoDefault != status) {
              mControlFrame->MouseClicked(&aPresContext);
            }
	          mLastMouseState = eMouseEnter;
	        } 
	        break;
        case NS_MOUSE_EXIT: // doesn't work for frames, yet
	        break;
        }
        aEventStatus = nsEventStatus_eConsumeNoDefault;
        NS_RELEASE(viewMan);
        return NS_OK;
      }
    }
  }
  if (nsnull == mFirstChild) { // XXX see corresponding hack in nsHTMLContainerFrame::DeleteFrame
    aEventStatus = nsEventStatus_eConsumeNoDefault;
    return NS_OK;
  } else {
    return nsHTMLContainerFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  }
}


NS_IMETHODIMP
nsLabelFrame::GetFor(nsString& aResult)
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* htmlContent = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
    if ((NS_OK == result) && htmlContent) {
      nsHTMLValue value;
      result = htmlContent->GetAttribute(nsHTMLAtoms::_for, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(aResult);
        }
      }
      NS_RELEASE(htmlContent);
    }
  }
  return result;
}

PRBool IsButton(PRInt32 aType)
{
  if ((NS_FORM_INPUT_BUTTON == aType) || (NS_FORM_INPUT_RESET   == aType)   ||
      (NS_FORM_INPUT_SUBMIT == aType) || (NS_FORM_BUTTON_BUTTON == aType)   ||
      (NS_FORM_BUTTON_RESET == aType) || (NS_FORM_BUTTON_SUBMIT == aType))  {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}


PRBool 
nsLabelFrame::FindForControl(nsIFormControlFrame*& aResultFrame)
{
  static char whitespace[] = " \r\n\t";

  nsAutoString forId;
  if (NS_CONTENT_ATTR_HAS_VALUE != GetFor(forId)) {
    return PR_FALSE;
  }

  nsIDocument* iDoc = nsnull;
  nsresult result = mContent->GetDocument(iDoc);
  if ((NS_OK != result) || (nsnull == iDoc)) {
    return PR_FALSE;
  }

  nsIHTMLDocument* htmlDoc = nsnull;
  result = iDoc->QueryInterface(kIHTMLDocumentIID, (void**)&htmlDoc);
  if ((NS_OK != result) || (nsnull == htmlDoc)) {
    NS_RELEASE(iDoc);
    return PR_FALSE;
  }

  nsIPresShell *shell = iDoc->GetShellAt(0);
  if (nsnull == shell) {
    NS_RELEASE(iDoc);
    NS_RELEASE(htmlDoc);
    return PR_FALSE;
  }

  nsIDOMHTMLCollection* forms = nsnull;
  htmlDoc->GetForms(&forms);
  PRUint32 numForms;
  forms->GetLength(&numForms);

  PRBool returnValue = PR_FALSE;

  for (PRUint32 formX = 0; formX < numForms; formX++) {
    nsIContent* iContent = nsnull;
    nsIDOMNode* node = nsnull;
    forms->Item(formX, &node);
    if (nsnull == node) {
      continue;
    }
    nsIForm* form = nsnull;
    result = node->QueryInterface(kIFormIID, (void**)&form);
    if ((NS_OK != result) || (nsnull == form)) {
      continue;
    }
    PRUint32 numControls;
    form->GetElementCount(&numControls);
    for (PRUint32 controlX = 0; controlX < numControls; controlX++) {
      nsIFormControl* control = nsnull;
      form->GetElementAt(controlX, &control);
      if (nsnull == control) {
        continue;
      }
      // buttons have implicit labels and we don't allow them to have explicit ones
      PRInt32 type;
      control->GetType(&type);
      if (!IsButton(type)) {
        nsIHTMLContent* htmlContent = nsnull;
        result = control->QueryInterface(kIHTMLContentIID, (void**)&htmlContent);
        if ((NS_OK == result) && (nsnull != htmlContent)) {
          nsHTMLValue value;
          nsAutoString id;
          result = htmlContent->GetAttribute(nsHTMLAtoms::id, value);
          if ((NS_CONTENT_ATTR_HAS_VALUE == result) && (eHTMLUnit_String == value.GetUnit())) {
            value.GetStringValue(id);
            id.Trim(whitespace, PR_TRUE, PR_TRUE);    
            if (id.Equals(forId)) {
              nsIFrame* frame = shell->FindFrameWithContent(htmlContent);
              if (nsnull != frame) {
                nsIFormControlFrame* fcFrame = nsnull;
                result = frame->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
                if ((NS_OK == result) && (nsnull != fcFrame)) {
                  aResultFrame = fcFrame;
                  NS_RELEASE(fcFrame);
                  returnValue = PR_TRUE;
                }
              }
            }
          }
          NS_RELEASE(htmlContent);
        }
      }
      NS_RELEASE(control);
    } 
  }
  NS_RELEASE(iDoc);
  NS_RELEASE(htmlDoc);
  NS_RELEASE(forms);
  NS_RELEASE(shell);

  return returnValue;
}

PRBool 
nsLabelFrame::FindFirstControl(nsIFrame* aParentFrame, nsIFormControlFrame*& aResultFrame)
{
  nsIFrame* child = nsnull;
  aParentFrame->FirstChild(child);
  while (nsnull != child) {
    nsIFormControlFrame* fcFrame = nsnull;
    nsresult result = child->QueryInterface(kIFormControlFrameIID, (void**)&fcFrame);
    if ((NS_OK == result) && fcFrame) {
      PRInt32 type;
      fcFrame->GetType(&type);
      // buttons have implicit labels and we don't allow them to have explicit ones
      if (!IsButton(type)) {
        aResultFrame = fcFrame;
        return PR_TRUE;
      }
      NS_RELEASE(fcFrame);
    } else if (FindFirstControl(child, aResultFrame)) {
      return PR_TRUE;
    }
    child->GetNextSibling(child);
  }
  return PR_FALSE;
}


NS_IMETHODIMP
nsLabelFrame::Init(nsIPresContext& aPresContext, nsIFrame* aChildList)
{
  // create our view, we need a view to grab the mouse 
  nsIView* view;
  GetView(view);
  if (!view) {
    nsresult result = nsRepository::CreateInstance(kViewCID, nsnull, kIViewIID,
                                                  (void **)&view);
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
    NS_RELEASE(viewMan);
  }

  // cache our display type
  const nsStyleDisplay* styleDisplay;
  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) styleDisplay);
  mInline = (NS_STYLE_DISPLAY_BLOCK != styleDisplay->mDisplay);

  PRUint8 flags = (mInline) ? NS_BODY_SHRINK_WRAP : 0;
  NS_NewBodyFrame(mContent, this, mFirstChild, flags);

  // Resolve style and set the style context
  nsIStyleContext* styleContext =
    aPresContext.ResolvePseudoStyleContextFor(mContent, nsHTMLAtoms::labelContentPseudo, mStyleContext);
  mFirstChild->SetStyleContext(&aPresContext, styleContext);
  NS_RELEASE(styleContext);                                           

  // Set the geometric and content parent for each of the child frames
  for (nsIFrame* frame = aChildList; nsnull != frame; frame->GetNextSibling(frame)) {
    frame->SetGeometricParent(mFirstChild);
    frame->SetContentParent(mFirstChild);
  }

  // Queue up the frames for the body frame
  return mFirstChild->Init(aPresContext, aChildList);
}

NS_IMETHODIMP
nsLabelFrame::Paint(nsIPresContext& aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect)
{
  return nsHTMLContainerFrame::Paint(aPresContext, aRenderingContext, aDirtyRect);
}

NS_IMETHODIMP 
nsLabelFrame::Reflow(nsIPresContext& aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus& aStatus)
{
  if (nsnull == mControlFrame) {
    // check to see if a form control is referenced via the "for" attribute
    if (FindForControl(mControlFrame)) {
      mControlIsInside = PR_FALSE;
    } else {
      // find the 1st (and should be only) form control contained within if there is no "for"
      mControlIsInside = FindFirstControl(this, mControlFrame);
    }
  }

  nsSize availSize(aReflowState.maxSize);

  // reflow the child
  nsHTMLReflowState reflowState(aPresContext, mFirstChild, aReflowState, availSize);
  ReflowChild(mFirstChild, aPresContext, aDesiredSize, reflowState, aStatus);

  // get border and padding
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);

  // Place the child
  nsRect rect = nsRect(borderPadding.left, borderPadding.top, aDesiredSize.width, aDesiredSize.height);
  mFirstChild->SetRect(rect);

  // add in our border and padding to the size of the child
  aDesiredSize.width  += borderPadding.left + borderPadding.right;
  aDesiredSize.height += borderPadding.top + borderPadding.bottom;

  // adjust our max element size, if necessary
  if (aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width  += borderPadding.left + borderPadding.right;
    aDesiredSize.maxElementSize->height += borderPadding.top + borderPadding.bottom;
  }

  // if we are constrained and the child is smaller, use the constrained values
  if (aReflowState.HaveFixedContentWidth() && (aDesiredSize.width < aReflowState.minWidth)) {
    aDesiredSize.width = aReflowState.minWidth;
  }
  if (aReflowState.HaveFixedContentHeight() && (aDesiredSize.height < aReflowState.minHeight)) {
    aDesiredSize.height = aReflowState.minHeight;
  }

  aDesiredSize.ascent  = aDesiredSize.height;
  aDesiredSize.descent = 0;

  aStatus = NS_FRAME_COMPLETE;
  return NS_OK;
}


PRIntn
nsLabelFrame::GetSkipSides() const
{
  return 0;
}


