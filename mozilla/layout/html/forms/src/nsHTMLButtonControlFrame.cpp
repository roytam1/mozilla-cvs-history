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

#include "nsHTMLButtonControlFrame.h"

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsFormControlHelper.h"
#include "nsIFormControlFrame.h"
#include "nsHTMLParts.h"
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
#include "nsStyleUtil.h"
#include "nsDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIHTMLAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIWidget.h"
#include "nsIComponentManager.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsViewsCID.h"
#include "nsColor.h"
#include "nsIDocument.h"
#include "nsButtonFrameRenderer.h"
#include "nsFormControlFrame.h"

static NS_DEFINE_IID(kIFormControlIID, NS_IFORMCONTROL_IID);
static NS_DEFINE_IID(kIFormControlFrameIID, NS_IFORMCONTROLFRAME_IID);
static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

nsresult
NS_NewHTMLButtonControlFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsHTMLButtonControlFrame* it = new (aPresShell) nsHTMLButtonControlFrame;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsHTMLButtonControlFrame::nsHTMLButtonControlFrame()
  : nsHTMLContainerFrame()
{
  mInline = PR_TRUE;
  mPreviousCursor = eCursor_standard;
  mTranslatedRect = nsRect(0,0,0,0);
  mDidInit = PR_FALSE;
  mRenderer.SetNameSpace(kNameSpaceID_None);
  mPresContext    = nsnull;

  mCacheSize.width             = -1;
  mCacheSize.height            = -1;
  mCachedMaxElementSize.width  = -1;
  mCachedMaxElementSize.height = -1;
}

nsHTMLButtonControlFrame::~nsHTMLButtonControlFrame()
{
  nsFormControlFrame::RegUnRegAccessKey(mPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);
  if (mFormFrame) {
    mFormFrame->RemoveFormControlFrame(*this);
    mFormFrame = nsnull;
  }
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsHTMLContainerFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  mRenderer.SetFrame(this,aPresContext);
   // cache our display type
  const nsStyleDisplay* styleDisplay;
  GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) styleDisplay);
  mInline = (NS_STYLE_DISPLAY_BLOCK != styleDisplay->mDisplay);

  PRUint32 flags = NS_BLOCK_SPACE_MGR;
  if (mInline) {
    flags |= NS_BLOCK_SHRINK_WRAP;
  }

  nsCOMPtr<nsIPresShell> shell;
  aPresContext->GetShell(getter_AddRefs(shell));
  nsIFrame* areaFrame;
  NS_NewAreaFrame(shell, &areaFrame, flags);
  mFrames.SetFrames(areaFrame);

  // Resolve style and initialize the frame
  nsIStyleContext* styleContext;
  aPresContext->ResolvePseudoStyleContextFor(mContent, nsHTMLAtoms::buttonContentPseudo,
                                            mStyleContext, PR_FALSE,
                                            &styleContext);
  mFrames.FirstChild()->Init(aPresContext, mContent, this, styleContext, nsnull);
  NS_RELEASE(styleContext);                                           

  return rv;
}

nsrefcnt nsHTMLButtonControlFrame::AddRef(void)
{
  NS_WARNING("not supported");
  return 1;
}

nsrefcnt nsHTMLButtonControlFrame::Release(void)
{
  NS_WARNING("not supported");
  return 1;
}

// Frames are not refcounted, no need to AddRef
NS_IMETHODIMP
nsHTMLButtonControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIFormControlFrameIID)) {
    *aInstancePtr = (void*) ((nsIFormControlFrame*) this);
    return NS_OK;
  }
  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

void
nsHTMLButtonControlFrame::GetDefaultLabel(nsString& aString) 
{
  PRInt32 type;
  GetType(&type);
  if (NS_FORM_BUTTON_BUTTON == type) {
    aString = "Button";
  } 
  else if (NS_FORM_BUTTON_RESET == type) {
    aString = "Reset";
  } 
  else if (NS_FORM_BUTTON_SUBMIT == type) {
    aString = "Submit";
  } 
}


PRInt32
nsHTMLButtonControlFrame::GetMaxNumValues() 
{
  return 1;
}


PRBool
nsHTMLButtonControlFrame::GetNamesValues(PRInt32 aMaxNumValues, PRInt32& aNumValues,
                                     nsString* aValues, nsString* aNames)
{
  nsAutoString name;
  nsresult result = GetName(&name);
  if ((aMaxNumValues <= 0) || (NS_CONTENT_ATTR_HAS_VALUE != result)) {
    return PR_FALSE;
  }

  PRInt32 type;
  GetType(&type);
  nsAutoString value;
  nsresult valResult = GetValue(&value);

  if (NS_CONTENT_ATTR_HAS_VALUE == valResult) {
    aValues[0] = value;
    aNames[0]  = name;
    aNumValues = 1;
    return PR_TRUE;
  } else {
    aNumValues = 0;
    return PR_FALSE;
  }

}


NS_IMETHODIMP
nsHTMLButtonControlFrame::GetType(PRInt32* aType) const
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIFormControl* formControl = nsnull;
    result = mContent->QueryInterface(kIFormControlIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      result = formControl->GetType(aType);
      NS_RELEASE(formControl);
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::GetName(nsString* aResult)
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* formControl = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      nsHTMLValue value;
      result = formControl->GetHTMLAttribute(nsHTMLAtoms::name, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(*aResult);
        }
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::GetValue(nsString* aResult)
{
  nsresult result = NS_FORM_NOTOK;
  if (mContent) {
    nsIHTMLContent* formControl = nsnull;
    result = mContent->QueryInterface(kIHTMLContentIID, (void**)&formControl);
    if ((NS_OK == result) && formControl) {
      nsHTMLValue value;
      result = formControl->GetHTMLAttribute(nsHTMLAtoms::value, value);
      if (NS_CONTENT_ATTR_HAS_VALUE == result) {
        if (eHTMLUnit_String == value.GetUnit()) {
          value.GetStringValue(*aResult);
        }
      }
      NS_RELEASE(formControl);
    }
  }
  return result;
}

PRBool
nsHTMLButtonControlFrame::IsSuccessful(nsIFormControlFrame* aSubmitter)
{
  if (this == (aSubmitter)) {
    nsAutoString name;
    return (NS_CONTENT_ATTR_HAS_VALUE == GetName(&name));
  } else {
    return PR_FALSE;
  }
}

PRBool
nsHTMLButtonControlFrame::IsReset(PRInt32 type)
{
  if (NS_FORM_BUTTON_RESET == type) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

PRBool
nsHTMLButtonControlFrame::IsSubmit(PRInt32 type)
{
  if (NS_FORM_BUTTON_SUBMIT == type) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}


void
nsHTMLButtonControlFrame::MouseClicked(nsIPresContext* aPresContext) 
{
  PRInt32 type;
  GetType(&type);

  if ((nsnull != mFormFrame) && !nsFormFrame::GetDisabled(this)) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsEvent event;
    event.eventStructType = NS_EVENT;
    nsIContent *formContent = nsnull;
    mFormFrame->GetContent(&formContent);

    if (IsReset(type) == PR_TRUE) {
      event.message = NS_FORM_RESET;
      if (nsnull != formContent) {
        formContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
      }
      if (nsEventStatus_eConsumeNoDefault != status) {
        mFormFrame->OnReset(aPresContext);
      }
    }
    else if (IsSubmit(type) == PR_TRUE) {
      event.message = NS_FORM_SUBMIT;
      if (nsnull != formContent) {
        formContent->HandleDOMEvent(aPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status); 
      }
      if (nsEventStatus_eConsumeNoDefault != status) {
        mFormFrame->OnSubmit(aPresContext, this);
      }
    }
 
    NS_IF_RELEASE(formContent);
  }

}

void 
nsHTMLButtonControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
}

void
nsHTMLButtonControlFrame::ScrollIntoView(nsIPresContext* aPresContext)
{
  if (aPresContext) {
    nsCOMPtr<nsIPresShell> presShell;
    aPresContext->GetShell(getter_AddRefs(presShell));
    presShell->ScrollFrameIntoView(this,
                   NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE);

  }
}

void
nsHTMLButtonControlFrame::GetTranslatedRect(nsIPresContext* aPresContext, nsRect& aRect)
{
  nsIView* view;
  nsPoint viewOffset(0,0);
  GetOffsetFromView(aPresContext, viewOffset, &view);
  while (nsnull != view) {
    nsPoint tempOffset;
    view->GetPosition(&tempOffset.x, &tempOffset.y);
    viewOffset += tempOffset;
    view->GetParent(view);
  }
  aRect = nsRect(viewOffset.x, viewOffset.y, mRect.width, mRect.height);
}

            

NS_IMETHODIMP
nsHTMLButtonControlFrame::HandleEvent(nsIPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  // if disabled do nothing
  if (mRenderer.isDisabled()) {
    return NS_OK;
  }

   // lets see if the button was clicked. -EDV
  switch (aEvent->message) {
     case NS_MOUSE_LEFT_CLICK:
        MouseClicked(aPresContext);
     break;
  }

  // we don't want our children to get any events. So just pass it to frame.
  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}


NS_IMETHODIMP
nsHTMLButtonControlFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                           const nsPoint& aPoint,
                                           nsIFrame** aFrame)
{
  *aFrame = this;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                              nsIAtom*        aListName,
                                              nsIFrame*       aChildList)
{
  // add ourself as an nsIFormControlFrame
  nsFormFrame::AddFormControlFrame(aPresContext, *NS_STATIC_CAST(nsIFrame*, this));

 
  // Set the parent for each of the child frames
  for (nsIFrame* frame = aChildList; nsnull != frame; frame->GetNextSibling(&frame)) {
    frame->SetParent(mFrames.FirstChild());
  }

  // Queue up the frames for the inline frame
  return mFrames.FirstChild()->SetInitialChildList(aPresContext, nsnull, aChildList);
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::Paint(nsIPresContext* aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                const nsRect& aDirtyRect,
                                nsFramePaintLayer aWhichLayer)
{
 	const nsStyleDisplay* disp = (const nsStyleDisplay*)
	mStyleContext->GetStyleData(eStyleStruct_Display);
	if (disp->mVisible == NS_STYLE_VISIBILITY_VISIBLE)
  {
    nsRect rect(0, 0, mRect.width, mRect.height);
    mRenderer.PaintButton(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer, rect);
  }

#if 0 // old way
  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

#else // temporary
    // XXX This is temporary
  // clips to it's size minus the border 
  // but the real problem is the FirstChild (the AreaFrame)
  // isn't being constrained properly
  // Bug #17474
  const nsStyleSpacing* spacing;
  GetStyleData(eStyleStruct_Spacing,  (const nsStyleStruct *&)spacing);
  nsMargin border;
  border.SizeTo(0, 0, 0, 0);
  spacing->CalcBorderFor(this, border);

  nsRect rect;
  GetRect(rect);
  rect.x = 0;
  rect.y = 0;
  rect.Deflate(border);
  aRenderingContext.PushState();
  PRBool clipEmpty;

  aRenderingContext.SetClipRect(rect, nsClipCombine_kIntersect, clipEmpty);

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  aRenderingContext.PopState(clipEmpty);

#endif

  return NS_OK;
}

// XXX a hack until the reflow state does this correctly
// XXX when it gets fixed, leave in the printf statements or add an assertion
static
void ButtonHack(nsHTMLReflowState& aReflowState, char* aMessage)
{
  if (aReflowState.mComputedWidth == 0) {
    aReflowState.mComputedWidth = aReflowState.availableWidth;
  }
  if ((aReflowState.mComputedWidth != NS_INTRINSICSIZE) &&
      (aReflowState.mComputedWidth > aReflowState.availableWidth) &&
      (aReflowState.availableWidth > 0)) {
//    printf("BUG - %s has a computed width = %d, available width = %d \n", 
//    aMessage, aReflowState.mComputedWidth, aReflowState.availableWidth);
    aReflowState.mComputedWidth = aReflowState.availableWidth;
  }
  if (aReflowState.mComputedHeight == 0) {
    aReflowState.mComputedHeight = aReflowState.availableHeight;
  }
  if ((aReflowState.mComputedHeight != NS_INTRINSICSIZE) &&
      (aReflowState.mComputedHeight > aReflowState.availableHeight) &&
      (aReflowState.availableHeight > 0)) {
//    printf("BUG - %s has a computed height = %d, available height = %d \n", 
//    aMessage, aReflowState.mComputedHeight, aReflowState.availableHeight);
    aReflowState.mComputedHeight = aReflowState.availableHeight;
  }
}

NS_IMETHODIMP 
nsHTMLButtonControlFrame::AddComputedBorderPaddingToDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                                                                const nsHTMLReflowState& aSuggestedReflowState)
{
  aDesiredSize.width  += aSuggestedReflowState.mComputedBorderPadding.left + aSuggestedReflowState.mComputedBorderPadding.right;
  aDesiredSize.height += aSuggestedReflowState.mComputedBorderPadding.top + aSuggestedReflowState.mComputedBorderPadding.bottom;
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLButtonControlFrame::Reflow(nsIPresContext* aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus& aStatus)
{
  if (!mFormFrame && (eReflowReason_Initial == aReflowState.reason)) {
    mPresContext = aPresContext;
    nsFormControlFrame::RegUnRegAccessKey(aPresContext, NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
    nsFormFrame::AddFormControlFrame(aPresContext, *NS_STATIC_CAST(nsIFrame*, this));
  }

#if 0
  nsresult skiprv = nsFormControlFrame::SkipResizeReflow(mCacheSize, mCachedMaxElementSize, aPresContext, 
                                                         aDesiredSize, aReflowState, aStatus);
  if (NS_SUCCEEDED(skiprv)) {
    return skiprv;
  }
#endif
  // XXX remove the following when the reflow state is fixed
  ButtonHack((nsHTMLReflowState&)aReflowState, "html4 button");

  // commenting this out for now. We need a view to do mouse grabbing but
  // it doesn't really seem to work correctly. When you press the only event
  // you can get after that is a release. You need mouse enter and exit.
  // the view also breaks the outline code. For some reason you can not reset 
  // the clip rect to draw outside you bounds if you have a view. And you need to
  // because the outline must be drawn outside of our bounds according to CSS. -EDV

  // XXX If you do decide you need a view, then create it in the Init() function
  // and not here...
#if 0
  if (!mDidInit) {
    // create our view, we need a view to grab the mouse 
    nsIView* view;
    GetView(&view);
    if (!view) {
      nsresult result = nsComponentManager::CreateInstance(kViewCID, nsnull, kIViewIID, (void **)&view);
	    nsCOMPtr<nsIPresShell> presShell;
      aPresContext->GetShell(getter_AddRefs(presShell));
	    nsCOMPtr<nsIViewManager> viewMan;
      presShell->GetViewManager(getter_AddRefs(viewMan));

      nsIFrame* parWithView;
	    nsIView *parView;
      GetParentWithView(&parWithView);
	    parWithView->GetView(&parView);
      // the view's size is not know yet, but its size will be kept in synch with our frame.
      nsRect boundBox(0, 0, 500, 500); 
      result = view->Init(viewMan, boundBox, parView, nsnull);
      viewMan->InsertChild(parView, view, 0);
      SetView(view);

      const nsStyleColor* color = (const nsStyleColor*) mStyleContext->GetStyleData(eStyleStruct_Color);
      // set the opacity
      viewMan->SetViewOpacity(view, color->mOpacity);

    }
    mDidInit = PR_TRUE;
  }
#endif

  // reflow the child
  nsIFrame* firstKid = mFrames.FirstChild();
  nsSize availSize(aReflowState.mComputedWidth, NS_INTRINSICSIZE);

  // indent the child inside us by the the focus border. We must do this separate from the
  // regular border.
  nsMargin focusPadding = mRenderer.GetAddedButtonBorderAndPadding();

  
  if (NS_INTRINSICSIZE != availSize.width) {
    availSize.width -= focusPadding.left + focusPadding.right;
    availSize.width = PR_MAX(availSize.width,0);
  }
  if (NS_AUTOHEIGHT != availSize.height) {
    availSize.height -= focusPadding.top + focusPadding.bottom;
    availSize.height = PR_MAX(availSize.height,0);
  }
  
  nsHTMLReflowState reflowState(aPresContext, aReflowState, firstKid, availSize);
  //reflowState.computedWidth = availSize;

  // XXX remove the following when the reflow state is fixed
  //ButtonHack(reflowState, "html4 button's area");

  // XXX Proper handling of incremental reflow...
  if (eReflowReason_Incremental == aReflowState.reason) {
    nsIFrame* targetFrame;
    
    // See if it's targeted at us
    aReflowState.reflowCommand->GetTarget(targetFrame);
    if (this == targetFrame) {
      Invalidate(aPresContext, nsRect(0,0,mRect.width,mRect.height), PR_FALSE);

      nsIReflowCommand::ReflowType  reflowType;
      aReflowState.reflowCommand->GetType(reflowType);
      if (nsIReflowCommand::StyleChanged == reflowType) {
        reflowState.reason = eReflowReason_StyleChange;
      }
      else {
        reflowState.reason = eReflowReason_Resize;
      }
    } else {
      nsIFrame* nextFrame;

      // Remove the next frame from the reflow path
      aReflowState.reflowCommand->GetNext(nextFrame);  
    }
  }

  ReflowChild(firstKid, aPresContext, aDesiredSize, reflowState,
              focusPadding.left + aReflowState.mComputedBorderPadding.left,
              focusPadding.top + aReflowState.mComputedBorderPadding.top,
              0, aStatus);

  // center child vertically
  nscoord yoff = 0;
  if (aReflowState.mComputedHeight != NS_INTRINSICSIZE) {
    yoff = (aReflowState.mComputedHeight - aDesiredSize.height)/2;
    if (yoff < 0)
        yoff = 0;
  }


  // Place the child
  FinishReflowChild(firstKid, aPresContext, aDesiredSize,
                    focusPadding.left + aReflowState.mComputedBorderPadding.left,
                    yoff + focusPadding.top + aReflowState.mComputedBorderPadding.top, 0);

#if 0 // old way
  // if computed use the computed values.
  if (aReflowState.mComputedWidth != NS_INTRINSICSIZE && (aDesiredSize.width < aReflowState.mComputedWidth)) 
    aDesiredSize.width = aReflowState.mComputedWidth;
  else 
    aDesiredSize.width  += focusPadding.left + focusPadding.right;

  if (aReflowState.mComputedHeight != NS_INTRINSICSIZE && (aDesiredSize.height < aReflowState.mComputedHeight)) 
    aDesiredSize.height = aReflowState.mComputedHeight;
  else
    aDesiredSize.height += focusPadding.top + focusPadding.bottom;

#else // temporary for Bug #17474
  // if computed use the computed values.
  if (aReflowState.mComputedWidth != NS_INTRINSICSIZE) 
    aDesiredSize.width = aReflowState.mComputedWidth;
  else 
    aDesiredSize.width  += focusPadding.left + focusPadding.right;

  if (aReflowState.mComputedHeight != NS_INTRINSICSIZE) 
    aDesiredSize.height = aReflowState.mComputedHeight;
  else
    aDesiredSize.height += focusPadding.top + focusPadding.bottom;
#endif

  AddComputedBorderPaddingToDesiredSize(aDesiredSize, aReflowState);
  //aDesiredSize.width  += aReflowState.mComputedBorderPadding.left + aReflowState.mComputedBorderPadding.right;
  //aDesiredSize.height += aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;

#if 0
  //adjust our max element size, if necessary
  if (aDesiredSize.maxElementSize) {
    aDesiredSize.AddBorderPaddingToMaxElementSize(aReflowState.mComputedBorderPadding);
  }
#endif

  aDesiredSize.ascent  = aDesiredSize.height;
  aDesiredSize.descent = 0;


  aStatus = NS_FRAME_COMPLETE;

  nsFormControlFrame::SetupCachedSizes(mCacheSize, mCachedMaxElementSize, aDesiredSize);
  return NS_OK;
}

PRIntn
nsHTMLButtonControlFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::GetFont(nsIPresContext* aPresContext, 
                                  const nsFont*&  aFont)
{
  return nsFormControlHelper::GetFont(this, aPresContext, mStyleContext, aFont);
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::GetFormContent(nsIContent*& aContent) const
{
  nsIContent* content;
  nsresult    rv;

  rv = GetContent(&content);
  aContent = content;
  return rv;
}

nscoord 
nsHTMLButtonControlFrame::GetVerticalInsidePadding(nsIPresContext* aPresContext,
                                                   float aPixToTwip, 
                                                   nscoord aInnerHeight) const
{
   return 0;
}

nscoord 
nsHTMLButtonControlFrame::GetHorizontalInsidePadding(nsIPresContext* aPresContext,
                                               float aPixToTwip, 
                                               nscoord aInnerWidth,
                                               nscoord aCharWidth) const
{
  return 0;
}

nsresult nsHTMLButtonControlFrame::RequiresWidget(PRBool& aRequiresWidget)
{
  aRequiresWidget = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP nsHTMLButtonControlFrame::SetProperty(nsIPresContext* aPresContext,
                                                    nsIAtom* aName, const nsString& aValue)
{
  return NS_OK;
}

NS_IMETHODIMP nsHTMLButtonControlFrame::GetProperty(nsIAtom* aName, nsString& aValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::GetAdditionalStyleContext(PRInt32 aIndex, 
                                                    nsIStyleContext** aStyleContext) const
{
  return mRenderer.GetStyleContext(aIndex, aStyleContext);
}

NS_IMETHODIMP
nsHTMLButtonControlFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                                    nsIStyleContext* aStyleContext)
{
  return mRenderer.SetStyleContext(aIndex, aStyleContext);
}


NS_IMETHODIMP nsHTMLButtonControlFrame::SetSuggestedSize(nscoord aWidth, nscoord aHeight)
{
//  mSuggestedWidth = aWidth;
//  mSuggestedHeight = aHeight;
  return NS_OK;
}




