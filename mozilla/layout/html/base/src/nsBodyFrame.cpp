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
#include "nsBodyFrame.h"
#include "nsIContent.h"
#include "nsIHTMLContent.h"
#include "nsIReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIDeviceContext.h"
#include "nsSpaceManager.h"
#include "nsHTMLAtoms.h"
#include "nsIView.h"
#include "nsViewsCID.h"
#include "nsAbsoluteFrame.h"
#include "nsHTMLIIDs.h"
#include "nsIWebShell.h"
#include "nsHTMLValue.h"
#include "nsHTMLParts.h"

// XXX TEMP. See HandleEvent()...
#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"

static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);

nsresult
NS_NewBodyFrame(nsIContent* aContent, nsIFrame* aParent, nsIFrame*& aResult,
                PRUint32 aFlags)
{
  nsBodyFrame* it = new nsBodyFrame(aContent, aParent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetFlags(aFlags);
  aResult = it;
  return NS_OK;
}

nsBodyFrame::nsBodyFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsHTMLContainerFrame(aContent, aParentFrame)
{
  mSpaceManager = new nsSpaceManager(this);
  NS_ADDREF(mSpaceManager);
}

nsBodyFrame::~nsBodyFrame()
{
  NS_RELEASE(mSpaceManager);
}

/////////////////////////////////////////////////////////////////////////////
// nsIUnknown

nsresult
nsBodyFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIAbsoluteItemsIID)) {
    *aInstancePtr = (void*) ((nsIAbsoluteItems*) this);
    return NS_OK;
  }
  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

/////////////////////////////////////////////////////////////////////////////
// nsIFrame

NS_IMETHODIMP
nsBodyFrame::Init(nsIPresContext& aPresContext, nsIFrame* aChildList)
{
  // Create a block frame and set its style context
  nsresult rv = NS_NewBlockFrame(mContent, this, mFirstChild, mFlags);
  if (NS_OK != rv) {
    return rv;
  }
  mChildCount = 1;
  nsIStyleContext* pseudoStyleContext =
   aPresContext.ResolvePseudoStyleContextFor(nsHTMLAtoms::columnPseudo, this);
  mFirstChild->SetStyleContext(&aPresContext, pseudoStyleContext);
  NS_RELEASE(pseudoStyleContext);

  // Set the geometric and content parent for each of the child frames
  for (nsIFrame* frame = aChildList; nsnull != frame; frame->GetNextSibling(frame)) {
    frame->SetGeometricParent(mFirstChild);
    frame->SetContentParent(mFirstChild);
  }

  // Queue up the frames for the block frame
  return mFirstChild->Init(aPresContext, aChildList);
}

#ifdef NS_DEBUG
void
nsBodyFrame::BandData::ComputeAvailSpaceRect()
{
  nsBandTrapezoid*  trapezoid = data;

  if (count > 1) {
    // If there's more than one trapezoid that means there are floaters
    PRInt32 i;

    // Stop when we get to space occupied by a right floater, or when we've
    // looked at every trapezoid and none are right floaters
    for (i = 0; i < count; i++) {
      nsBandTrapezoid*  trapezoid = &data[i];
      if (trapezoid->state != nsBandTrapezoid::Available) {
        const nsStyleDisplay* display;
        if (nsBandTrapezoid::OccupiedMultiple == trapezoid->state) {
          PRInt32 j, numFrames = trapezoid->frames->Count();
          NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
          for (j = 0; j < numFrames; j++) {
            nsIFrame* f = (nsIFrame*)trapezoid->frames->ElementAt(j);
            f->GetStyleData(eStyleStruct_Display,
                            (const nsStyleStruct*&)display);
            if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
              goto foundRightFloater;
            }
          }
        } else {
          trapezoid->frame->GetStyleData(eStyleStruct_Display,
                                         (const nsStyleStruct*&)display);
          if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
            break;
          }
        }
      }
    }
  foundRightFloater:

    if (i > 0) {
      trapezoid = &data[i - 1];
    }
  }

  if (nsBandTrapezoid::Available == trapezoid->state) {
    // The trapezoid is available
    trapezoid->GetRect(availSpace);
  } else {
    const nsStyleDisplay* display;

    // The trapezoid is occupied. That means there's no available space
    trapezoid->GetRect(availSpace);

    // XXX Better handle the case of multiple frames
    if (nsBandTrapezoid::Occupied == trapezoid->state) {
      trapezoid->frame->GetStyleData(eStyleStruct_Display,
                                     (const nsStyleStruct*&)display);
      if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
        availSpace.x = availSpace.XMost();
      }
    }
    availSpace.width = 0;
  }
}
#endif

NS_IMETHODIMP
nsBodyFrame::Paint(nsIPresContext&      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect)
{
  nsresult rv = nsHTMLContainerFrame::Paint(aPresContext, aRenderingContext,
                                            aDirtyRect);

#ifdef NS_DEBUG
  if (nsIFrame::GetShowFrameBorders()) {
    // Render the bands in the spacemanager
    BandData band;
    nsISpaceManager* sm = mSpaceManager;
    nscoord y = 0;
    while (y < mRect.height) {
      sm->GetBandData(y, nsSize(mRect.width, mRect.height - y), band);
      band.ComputeAvailSpaceRect();

      // Render a box and a diagonal line through the band
      aRenderingContext.SetColor(NS_RGB(0,255,0));
      aRenderingContext.DrawRect(0, band.availSpace.y,
                                 mRect.width, band.availSpace.height);
      aRenderingContext.DrawLine(0, band.availSpace.y,
                                 mRect.width, band.availSpace.YMost());

      // Render boxes and opposite diagonal lines around the
      // unavailable parts of the band.
      PRInt32 i;
      for (i = 0; i < band.count; i++) {
        nsBandTrapezoid* trapezoid = &band.data[i];
        if (nsBandTrapezoid::Available != trapezoid->state) {
          nsRect r;
          trapezoid->GetRect(r);
          if (nsBandTrapezoid::OccupiedMultiple == trapezoid->state) {
            aRenderingContext.SetColor(NS_RGB(0,255,128));
          }
          else {
            aRenderingContext.SetColor(NS_RGB(128,255,0));
          }
          aRenderingContext.DrawRect(r);
          aRenderingContext.DrawLine(r.x, r.YMost(), r.XMost(), r.y);
        }
      }
      y = band.availSpace.YMost();
    }
  }
#endif

  return rv;
}

NS_IMETHODIMP
nsBodyFrame::Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsBodyFrame::Reflow: maxSize=%d,%d reason=%d",
                  aReflowState.maxSize.width,
                  aReflowState.maxSize.height,
                  aReflowState.reason));

  const nsHTMLReflowState* rsp = &aReflowState;
  nsHTMLReflowState resizeReflowState(aReflowState);
  resizeReflowState.spaceManager = mSpaceManager;

  aStatus = NS_FRAME_COMPLETE;  // initialize out parameter

#if 0
  else {
    NS_ASSERTION(eReflowReason_Initial != aReflowState.reason, "bad reason");
  }
#endif

  nsIFrame*                    reflowCmdTarget;
  nsIReflowCommand::ReflowType reflowCmdType;

  if (eReflowReason_Incremental == aReflowState.reason) {
    NS_ASSERTION(nsnull != aReflowState.reflowCommand, "null reflow command");

    // Get the target and the type of reflow command
    aReflowState.reflowCommand->GetTarget(reflowCmdTarget);
    aReflowState.reflowCommand->GetType(reflowCmdType);
    if (nsIReflowCommand::StyleChanged != reflowCmdType) {
      // XXX CONSTRUCTION
      if (this == reflowCmdTarget) {
        NS_ASSERTION(nsIReflowCommand::FrameAppended == reflowCmdType,
                     "unexpected reflow command");

        // Append reflow commands will be targeted at us. Reset the target and
        // send the reflow command.
        // XXX Would it be better to have the frame generate the reflow command
        // that way it could correctly set the target?
        reflowCmdTarget = mFirstChild;
        aReflowState.reflowCommand->SetTarget(mFirstChild);

        // Reset the geometric and content parent for each of the child frames
        nsIFrame* childList;
        aReflowState.reflowCommand->GetChildFrame(childList);
        for (nsIFrame* frame = childList; nsnull != frame; frame->GetNextSibling(frame)) {
          frame->SetGeometricParent(mFirstChild);
          frame->SetContentParent(mFirstChild);
        }
      }

      // The reflow command should never be target for us
#ifdef NS_DEBUG
      NS_ASSERTION(this != reflowCmdTarget, "bad reflow command target");
#endif

      // Is the next frame in the reflow chain the pseudo block-frame or an
      // absolutely positioned frame?
      //
      // If the next frame is the pseudo block-frame then fall thru to the main
      // code below. The only thing that should be handled below is absolutely
      // positioned elements...
      nsIFrame* nextFrame;
      aReflowState.reflowCommand->GetNext(nextFrame);
      if ((nsnull != nextFrame) && (mFirstChild != nextFrame)) {
        NS_ASSERTION(this != nextFrame, "huh?");
        NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                       ("nsBodyFrame::Reflow: reflowing frame=%p",
                        nextFrame));
        // It's an absolutely positioned frame that's the target.
        // XXX FIX ME. For an absolutely positioned item we need to properly
        // compute the available space and compute the origin...
        nsIHTMLReflow*  reflow;
        if (NS_OK == nextFrame->QueryInterface(kIHTMLReflowIID, (void**)&reflow)) {
          nsHTMLReflowState reflowState(aPresContext, nextFrame, aReflowState,
                                        aReflowState.maxSize);
          reflowState.spaceManager = mSpaceManager;
          reflow->WillReflow(aPresContext);
          nsresult rv = reflow->Reflow(aPresContext, aDesiredSize, reflowState, aStatus);
          if (NS_OK != rv) {
            return rv;
          }
          nextFrame->SizeTo(aDesiredSize.width, aDesiredSize.height);
        }

        // XXX Commented this out because the absolute positioning code
        // above doesn't check if it needs to position the absolute frame.
#if 0
        // XXX Temporary code: if the frame we just reflowed is a
        // floating frame then fall through into the main reflow pathway
        // after clearing out our incremental reflow status. This forces
        // our child to adjust to the new size of the floater.
        //
        // XXXX We shouldn't be here at all for floating frames, just for absolutely
        // positioned frames. What's happening is that if a child of the body is
        // floated then the reflow state path isn't getting set up correctly. The
        // body's block pseudo-frame isn't getting included in the reflow path like
        // it shoudld and that's why we end up here
        const nsStyleDisplay* display;
        nextFrame->GetStyleData(eStyleStruct_Display,
                                (const nsStyleStruct*&) display);
        if (NS_STYLE_FLOAT_NONE == display->mFloats) {
          return NS_OK;
        }
#endif

        // Switch over to a reflow-state that is called resize instead
        // of an incremental reflow state like we were passed in.
        resizeReflowState.reason = eReflowReason_Resize;
        resizeReflowState.reflowCommand = nsnull;
        rsp = &resizeReflowState;

        // XXX End temporary code
      }
    }
  }

  // The area that needs to be repainted. Depends on the reflow type.
  nsRect  damageArea(0, 0, 0, 0);

  // Reflow the child frame
  if (nsnull != mFirstChild) {
    // Get our border/padding info
    const nsStyleSpacing* mySpacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    nsMargin  borderPadding;
    mySpacing->CalcBorderPaddingFor(this, borderPadding);
  
    // Compute the child frame's max size
    nsSize  kidMaxSize = GetColumnAvailSpace(aPresContext, borderPadding,
                                             *rsp);
    mSpaceManager->Translate(borderPadding.left, borderPadding.top);
  
    if (eReflowReason_Resize == rsp->reason) {
      // Clear any regions that are marked as unavailable
      // XXX Temporary hack until everything is incremental...
      mSpaceManager->ClearRegions();
    }

    // Get the child's current rect
    nsRect  kidOldRect;
    mFirstChild->GetRect(kidOldRect);

    // Get the column's desired size
    nsHTMLReflowState reflowState(aPresContext, mFirstChild, *rsp, kidMaxSize);
    reflowState.spaceManager = mSpaceManager;
    nsIHTMLReflow*    htmlReflow;

    if (NS_OK == mFirstChild->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow)) {
      htmlReflow->WillReflow(aPresContext);
      htmlReflow->Reflow(aPresContext, aDesiredSize, reflowState, aStatus);
    }

    // If the frame is complete, then check whether there's a next-in-flow that
    // needs to be deleted
    if (NS_FRAME_IS_COMPLETE(aStatus)) {
      nsIFrame* kidNextInFlow;
       
      mFirstChild->GetNextInFlow(kidNextInFlow);
      if (nsnull != kidNextInFlow) {
        // Remove all of the childs next-in-flows
        DeleteChildsNextInFlow(aPresContext, mFirstChild);
      }
    }
    else {
      printf("XXX: incomplete body frame\n");
    }

    mSpaceManager->Translate(-borderPadding.left, -borderPadding.top);
  
    // Place and size the frame
    nsRect  desiredRect(borderPadding.left, borderPadding.top,
                        aDesiredSize.width, aDesiredSize.height);
    mFirstChild->SetRect(desiredRect);
  
    // Reflow any absolutely positioned frames that need reflowing
    ReflowAbsoluteItems(aPresContext, *rsp);

    // Return our desired size
    ComputeDesiredSize(aPresContext, aReflowState, desiredRect,
                       rsp->maxSize, borderPadding, aDesiredSize);

    // XXX This code is really temporary; the lower level frame
    // classes need to contribute to the area that needs damage
    // repair. This class should only worry about damage repairing
    // it's border+padding area.

    // Decide how much to repaint based on the reflow type.
    // Note: we don't have to handle the initial reflow case and the
    // resize reflow case, because they're handled by the root content
    // frame
    if (eReflowReason_Incremental == rsp->reason) {
      // For append reflow commands that target the body just repaint the newly
      // added part of the frame.
      if ((nsIReflowCommand::FrameAppended == reflowCmdType) &&
          (reflowCmdTarget == mFirstChild)) {
        // It's an append reflow command targeted at us
        damageArea.y = kidOldRect.YMost();
        damageArea.width = aDesiredSize.width;
        damageArea.height = aDesiredSize.height - kidOldRect.height;
        if (desiredRect.height == kidOldRect.height) {
          // Since we don't know what changed, assume it all changed.
          damageArea.y = 0;
          damageArea.height = aDesiredSize.height;
        }
      } else {
        // Ideally the frame that is the target of the reflow command
        // (or its parent frame) would generate a damage rect, but
        // since none of the frame classes know how to do this then
        // for the time being just repaint the entire frame
        damageArea.width = aDesiredSize.width;
        damageArea.height = aDesiredSize.height;
      }
    }

  }
  else {
    aDesiredSize.width = 0;
    aDesiredSize.height = 0;
    aDesiredSize.ascent = 0;
    aDesiredSize.descent = 0;
    if (nsnull != aDesiredSize.maxElementSize) {
      aDesiredSize.maxElementSize->width = 0;
      aDesiredSize.maxElementSize->height = 0;
    }
  }

  // If this is really The Body, we force a repaint of the damage area
  if ((NS_BODY_THE_BODY & mFlags) && !damageArea.IsEmpty()) {
    Invalidate(damageArea);
  }
  
  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("exit nsBodyFrame::Reflow: status=%d width=%d height=%d",
                      aStatus, aDesiredSize.width, aDesiredSize.height));
  return NS_OK;
}

NS_METHOD
nsBodyFrame::CreateContinuingFrame(nsIPresContext&  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame)
{
  nsBodyFrame* cf = new nsBodyFrame(mContent, aParent);
  if (nsnull == cf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PrepareContinuingFrame(aPresContext, aParent, aStyleContext, cf);
  aContinuingFrame = cf;
  return NS_OK;
}

static void
AddToPadding(nsIPresContext* aPresContext,
             nsStyleUnit aStyleUnit,
             nscoord aValue, 
             PRBool aVertical,
             nsStyleCoord& aStyleCoord)
{
  if (eStyleUnit_Coord == aStyleUnit) {
    nscoord coord;
    coord = aStyleCoord.GetCoordValue();
    coord += aValue;
    aStyleCoord.SetCoordValue(coord);
  } 
  else if (eStyleUnit_Percent == aStyleUnit) {
    nsRect visibleArea;
    aPresContext->GetVisibleArea(visibleArea);
    float increment = (aVertical)
      ? ((float)aValue) / ((float)visibleArea.height)
      : ((float)aValue) / ((float)visibleArea.width);
    float percent = aStyleCoord.GetPercentValue();
    percent += increment;
    aStyleCoord.SetPercentValue(percent);
  }
}

// XXX ick. pfuii. gotta go!
NS_METHOD 
nsBodyFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
  if (0 == (NS_BODY_THE_BODY & mFlags)) {
    return NS_OK;
  }

  // marginwidth/marginheight set in the body cancels
  // marginwidth/marginheight set in the web shell. However, if
  // marginwidth is set in the web shell but marginheight is not set
  // it is as if marginheight were set to 0. The same logic applies
  // when marginheight is set and marginwidth is not set.
      
  PRInt32 marginWidth = -1, marginHeight = -1;
  
  nsISupports* container;
  aPresContext->GetContainer(&container);
  if (nsnull != container) {
    nsIWebShell* webShell = nsnull;
    container->QueryInterface(kIWebShellIID, (void**) &webShell);
    if (nsnull != webShell) {
      webShell->GetMarginWidth(marginWidth);   // -1 indicates not set
      webShell->GetMarginHeight(marginHeight); // -1 indicates not set
      if ((marginWidth >= 0) && (marginHeight < 0)) { // nav quirk 
        marginHeight = 0;
      }
      if ((marginHeight >= 0) && (marginWidth < 0)) { // nav quirk
        marginWidth = 0;
      }
      NS_RELEASE(webShell);
    }
    NS_RELEASE(container);
  }

  nsHTMLValue value;
  float p2t = aPresContext->GetPixelsToTwips();
  nsIHTMLContent* hc;
  if (NS_OK == mContent->QueryInterface(kIHTMLContentIID, (void**) &hc)) {
    hc->GetAttribute(nsHTMLAtoms::marginwidth, value);
    if (eHTMLUnit_Pixel == value.GetUnit()) { // marginwidth is set in body 
      marginWidth = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      if (marginWidth < 0) {
        marginWidth = 0;
      }
    }
    hc->GetAttribute(nsHTMLAtoms::marginheight, value);
    if (eHTMLUnit_Pixel == value.GetUnit()) { // marginheight is set in body
      marginHeight = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
      if (marginHeight < 0) {
        marginHeight = 0;
      }
    }
    NS_RELEASE(hc);
  }

  nsStyleSpacing* spacing = nsnull;
  nsStyleCoord styleCoord;
  if (marginWidth > 0) {  // add marginwidth to padding
    spacing = (nsStyleSpacing*)mStyleContext->GetMutableStyleData(eStyleStruct_Spacing);
    AddToPadding(aPresContext, spacing->mPadding.GetLeftUnit(), 
                 marginWidth, PR_FALSE, spacing->mPadding.GetLeft(styleCoord));
    spacing->mPadding.SetLeft(styleCoord);
    AddToPadding(aPresContext, spacing->mPadding.GetRightUnit(), 
                 marginWidth, PR_FALSE, spacing->mPadding.GetRight(styleCoord));
    spacing->mPadding.SetRight(styleCoord);
  }
  if (marginHeight > 0) { // add marginheight to padding
    if (nsnull == spacing) {
      spacing = (nsStyleSpacing*)mStyleContext->GetMutableStyleData(eStyleStruct_Spacing);
    }
    AddToPadding(aPresContext, spacing->mPadding.GetTopUnit(), 
                 marginHeight, PR_TRUE, spacing->mPadding.GetTop(styleCoord));
    spacing->mPadding.SetTop(styleCoord);
    AddToPadding(aPresContext, spacing->mPadding.GetBottomUnit(), 
                 marginHeight, PR_TRUE, spacing->mPadding.GetBottom(styleCoord));
    spacing->mPadding.SetBottom(styleCoord);
  }

  if (nsnull != spacing) {
    mStyleContext->RecalcAutomaticData(aPresContext);
  }
  return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions

nsSize
nsBodyFrame::GetColumnAvailSpace(nsIPresContext& aPresContext,
                                 const nsMargin& aBorderPadding,
                                 const nsHTMLReflowState& aReflowState)
{
  nsSize  result(aReflowState.maxSize);

  // If we are being used as a top-level frame then make adjustments
  // for border/padding and a vertical scrollbar
  if (NS_BODY_THE_BODY & mFlags) {
    // If our width is constrained then subtract for the border/padding
    if (aReflowState.maxSize.width != NS_UNCONSTRAINEDSIZE) {
      result.width -= aBorderPadding.left + aBorderPadding.right;
    }
    // If our height is constrained then subtract for the border/padding
    if (aReflowState.maxSize.height != NS_UNCONSTRAINEDSIZE) {
      result.height -= aBorderPadding.top + aBorderPadding.bottom;
    }
  }
  else {
    if (aReflowState.HaveFixedContentWidth()) {
      result.width = aReflowState.minWidth +
        aBorderPadding.left + aBorderPadding.right;
    }
    if (aReflowState.HaveFixedContentHeight()) {
      result.height -= aBorderPadding.top + aBorderPadding.bottom;
    }
  }

  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     (": nsBodyFrame: columnAvailSpace=%d,%d [%s,%s]\n",
                      result.width, result.height,
                      eHTMLFrameConstraint_Unconstrained == aReflowState.widthConstraint
                      ? "not-constrained" : "constrained",
                      eHTMLFrameConstraint_Unconstrained == aReflowState.heightConstraint
                      ? "not-constrained" : "constrained"));
  return result;
}

void
nsBodyFrame::ComputeDesiredSize(nsIPresContext& aPresContext,
                                const nsHTMLReflowState& aReflowState,
                                const nsRect& aDesiredRect,
                                const nsSize& aMaxSize,
                                const nsMargin& aBorderPadding,
                                nsHTMLReflowMetrics& aMetrics)
{
  // Note: Body used as a pseudo-frame shrink wraps (unless of course
  // style says otherwise)
  nscoord height = PR_MAX(aDesiredRect.YMost(), mSpaceManager->YMost());
  nscoord width = aDesiredRect.XMost();

  // Take into account absolutely positioned elements when computing the
  // desired size
  for (PRInt32 i = 0; i < mAbsoluteItems.Count(); i++) {
    // Get the anchor frame
    nsAbsoluteFrame*  anchorFrame = (nsAbsoluteFrame*)mAbsoluteItems[i];
    nsIFrame*         absoluteFrame = anchorFrame->GetAbsoluteFrame();
    nsRect            rect;

    absoluteFrame->GetRect(rect);
    nscoord xmost = rect.XMost();
    nscoord ymost = rect.YMost();
    if (xmost > width) {
      width = xmost;
    }
    if (ymost > height) {
      height = ymost;
    }
  }

  // Apply style size if present; XXX note the inner value (style-size -
  // border+padding) should be given to the child as a max-size
  if (aReflowState.HaveFixedContentWidth()) {
    width = aReflowState.minWidth + aBorderPadding.left + aBorderPadding.right;
  }
  else {
    if ((0 == (NS_BODY_SHRINK_WRAP & mFlags)) &&
        (NS_UNCONSTRAINEDSIZE != aMaxSize.width)) {
      // Make sure we're at least as wide as our available width
      if (aMaxSize.width > width) {
        width = aMaxSize.width;
      }
    }
  }
  if (aReflowState.HaveFixedContentHeight()) {
    height = aReflowState.minHeight +
      aBorderPadding.top + aBorderPadding.bottom;
  }
  else {
    // aBorderPadding.top is already reflected in the
    // aDesiredRect.YMost() value.
    height += aBorderPadding.bottom;
  }

  aMetrics.width = width;
  aMetrics.height = height;
  aMetrics.ascent = height;
  aMetrics.descent = 0;
}

// Add the frame to the end of the child list
void nsBodyFrame::AddFrame(nsIFrame* aFrame)
{
  nsIFrame* lastChild = LastFrame(mFirstChild);

  lastChild->SetNextSibling(aFrame);
  aFrame->SetNextSibling(nsnull);
  mChildCount++;
}

/////////////////////////////////////////////////////////////////////////////
// Event handling

NS_IMETHODIMP
nsBodyFrame::HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus&  aEventStatus)
{
  aEventStatus = nsEventStatus_eIgnore;

  // Pass event down to our children. Give it to the children after
  // our first-child first (children after the first-child are either
  // absolute positioned frames or are floating frames, both of which
  // are on top (in the z order) of the first-child).
  PRInt32 n = mChildCount;
  nsIFrame* kid = mFirstChild;
  kid->GetNextSibling(kid);
  while (--n >= 0) {
    if (nsnull == kid) {
      kid = mFirstChild;
    }
    nsRect kidRect;
    kid->GetRect(kidRect);
    if (kidRect.Contains(aEvent->point)) {
      aEvent->point.MoveBy(-kidRect.x, -kidRect.y);
      kid->HandleEvent(aPresContext, aEvent, aEventStatus);
      aEvent->point.MoveBy(kidRect.x, kidRect.y);
      break;
    }
    kid->GetNextSibling(kid);
  }

  // XXX Hack mouse enter/exit and cursor code. THIS DOESN'T BELONG HERE!
#if 1
  if (aEventStatus != nsEventStatus_eConsumeNoDefault) {
    switch (aEvent->message) {
    case NS_MOUSE_MOVE:
    case NS_MOUSE_ENTER:
      {
        nsIFrame* target = this;
        nsIContent* mTargetContent = mContent;
        PRInt32 cursor;
       
        GetCursorAndContentAt(aPresContext, aEvent->point, &target, &mTargetContent, cursor);
        if (cursor == NS_STYLE_CURSOR_INHERIT) {
          cursor = NS_STYLE_CURSOR_DEFAULT;
        }
        nsCursor c;
        switch (cursor) {
        default:
        case NS_STYLE_CURSOR_DEFAULT:
          c = eCursor_standard;
          break;
        case NS_STYLE_CURSOR_POINTER:
          c = eCursor_hyperlink;
          break;
        case NS_STYLE_CURSOR_TEXT:
          c = eCursor_select;
          break;
        }
        nsIWidget* window;
        target->GetWindow(window);
        window->SetCursor(c);
        NS_RELEASE(window);

        //If the content object under the cursor has changed, fire a mouseover/out
        nsIEventStateManager *mStateManager;
        nsIContent *mLastContent;
        if (NS_OK == aPresContext.GetEventStateManager(&mStateManager)) {
          mStateManager->GetLastMouseOverContent(&mLastContent);
          if (mLastContent != mTargetContent) {
            if (nsnull != mLastContent) {
              //fire mouseout
              nsEventStatus mStatus = nsEventStatus_eIgnore;
              nsMouseEvent mEvent;
              mEvent.eventStructType = NS_MOUSE_EVENT;
              mEvent.message = NS_MOUSE_EXIT;
              mLastContent->HandleDOMEvent(aPresContext, &mEvent, nsnull, DOM_EVENT_INIT, mStatus); 
            }
            //fire mouseover
            nsEventStatus mStatus = nsEventStatus_eIgnore;
            nsMouseEvent mEvent;
            mEvent.eventStructType = NS_MOUSE_EVENT;
            mEvent.message = NS_MOUSE_ENTER;
            mTargetContent->HandleDOMEvent(aPresContext, &mEvent, nsnull, DOM_EVENT_INIT, mStatus);
            mStateManager->SetLastMouseOverContent(mTargetContent);
          }
          NS_RELEASE(mStateManager);
          NS_IF_RELEASE(mLastContent);
        }
      }
      break;
    case NS_MOUSE_EXIT:
      //Don't know if this is actually hooked up.
      {
        //Fire of mouseout to the last content object.
        nsIEventStateManager *mStateManager;
        nsIContent *mLastContent;
        if (NS_OK == aPresContext.GetEventStateManager(&mStateManager)) {
          mStateManager->GetLastMouseOverContent(&mLastContent);
          if (nsnull != mLastContent) {
            //fire mouseout
            nsEventStatus mStatus = nsEventStatus_eIgnore;
            nsMouseEvent mEvent;
            mEvent.eventStructType = NS_MOUSE_EVENT;
            mEvent.message = NS_MOUSE_EXIT;
            mLastContent->HandleDOMEvent(aPresContext, &mEvent, nsnull, DOM_EVENT_INIT, mStatus);
            mStateManager->SetLastMouseOverContent(nsnull);

            NS_RELEASE(mLastContent);
          }
          NS_RELEASE(mStateManager);
        }
      }
      break;
    case NS_MOUSE_LEFT_BUTTON_UP:
      {
        nsIEventStateManager *mStateManager;
        if (NS_OK == aPresContext.GetEventStateManager(&mStateManager)) {
          mStateManager->SetActiveLink(nsnull);
          NS_RELEASE(mStateManager);
        }
      }
      break;
    }
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsBodyFrame::GetCursorAndContentAt(nsIPresContext& aPresContext,
                                   const nsPoint&  aPoint,
                                   nsIFrame**      aFrame,
                                   nsIContent**    aContent,
                                   PRInt32&        aCursor)
{
  aCursor = NS_STYLE_CURSOR_INHERIT;
  *aContent = mContent;

  nsPoint tmp;
  PRInt32 n = mChildCount;
  nsIFrame* kid = mFirstChild;
  kid->GetNextSibling(kid);
  while (--n >= 0) {
    if (nsnull == kid) {
      kid = mFirstChild;
    }
    nsRect kidRect;
    kid->GetRect(kidRect);
    if (kidRect.Contains(aPoint)) {
      tmp.MoveTo(aPoint.x - kidRect.x, aPoint.y - kidRect.y);
      kid->GetCursorAndContentAt(aPresContext, tmp, aFrame, aContent, aCursor);
      break;
    }
    kid->GetNextSibling(kid);
  }
  return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////
// nsIAbsoluteItems

NS_METHOD nsBodyFrame::AddAbsoluteItem(nsAbsoluteFrame* aAnchorFrame)
{
  // Add the absolute anchor frame to our list of absolutely positioned
  // items.
  mAbsoluteItems.AppendElement(aAnchorFrame);
  return NS_OK;
}

NS_METHOD nsBodyFrame::RemoveAbsoluteItem(nsAbsoluteFrame* aAnchorFrame)
{
  NS_NOTYETIMPLEMENTED("removing an absolutely positioned frame");
  return NS_ERROR_NOT_IMPLEMENTED;
}

// Called at the end of the Reflow() member function so we can process
// any abolutely positioned items that need to be reflowed
void
nsBodyFrame::ReflowAbsoluteItems(nsIPresContext& aPresContext,
                                 const nsHTMLReflowState& aReflowState)
{
  for (PRInt32 i = 0; i < mAbsoluteItems.Count(); i++) {
    // Get the anchor frame and its absolutely positioned frame
    nsAbsoluteFrame*  anchorFrame = (nsAbsoluteFrame*)mAbsoluteItems[i];
    nsIFrame*         absoluteFrame = anchorFrame->GetAbsoluteFrame();
    PRBool            placeFrame = PR_FALSE;
    PRBool            reflowFrame = PR_FALSE;
    nsReflowReason    reflowReason = eReflowReason_Resize;

    // Get its style information
    const nsStyleDisplay*  display;
    const nsStylePosition* position;
     
    absoluteFrame->GetStyleData(eStyleStruct_Display, (nsStyleStruct*&)display);
    absoluteFrame->GetStyleData(eStyleStruct_Position, (nsStyleStruct*&)position);

    // See whether the frame is a newly added frame
    nsIFrame* parent;
    absoluteFrame->GetGeometricParent(parent);
    if (parent != this) {
      // The absolutely position item hasn't yet been added to our child list
      absoluteFrame->SetGeometricParent(this);

      // Add the absolutely positioned frame to the end of the child list
      AddFrame(absoluteFrame);

      // Make sure the frame has a view
      nsIView*  view;
      absoluteFrame->GetView(view);
      if (nsnull == view) {
        nsIStyleContext* absSC;
        absoluteFrame->GetStyleContext(&aPresContext, absSC);
        view = CreateAbsoluteView(absSC);
        NS_RELEASE(absSC);
        absoluteFrame->SetView(view);  
      }

      // See whether this is the frame's initial reflow
      nsFrameState  frameState;
      absoluteFrame->GetFrameState(frameState);
      if (frameState & NS_FRAME_FIRST_REFLOW) {
        reflowReason = eReflowReason_Initial;
      }

      // We need to place and reflow the absolutely positioned frame
      placeFrame = reflowFrame = PR_TRUE;

    } else {
      // We need to place the frame if the left-offset or the top-offset are
      // auto or a percentage
      if ((eStyleUnit_Coord != position->mLeftOffset.GetUnit()) ||
          (eStyleUnit_Coord != position->mTopOffset.GetUnit())) {
        placeFrame = PR_TRUE;
      }

      // We need to reflow the frame if its width or its height is auto or
      // a percentage
      if ((eStyleUnit_Coord != position->mWidth.GetUnit()) ||
          (eStyleUnit_Coord != position->mHeight.GetUnit())) {
        reflowFrame = PR_TRUE;
      }
    }

    if (placeFrame || reflowFrame) {
      // Get the rect for the absolutely positioned element
      nsRect  rect;
      ComputeAbsoluteFrameBounds(anchorFrame, aReflowState, position, rect);
  
      nsIHTMLReflow*  htmlReflow;
      if (NS_OK == absoluteFrame->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow)) {
        htmlReflow->WillReflow(aPresContext);
        absoluteFrame->MoveTo(rect.x, rect.y);

        if (reflowFrame) {
          // Resize reflow the absolutely positioned element
          nsSize  availSize(rect.width, rect.height);
      
          if (NS_STYLE_OVERFLOW_VISIBLE == display->mOverflow) {
            // Don't constrain the height since the container should be enlarged
            // to contain overflowing frames
            availSize.height = NS_UNCONSTRAINEDSIZE;
          }
      
          nsHTMLReflowMetrics desiredSize(nsnull);
          nsHTMLReflowState   reflowState(aPresContext, absoluteFrame,
                                          aReflowState, availSize,
                                          reflowReason);
          nsReflowStatus      status;
          htmlReflow->Reflow(aPresContext, desiredSize, reflowState, status);
        
          // Figure out what size to actually use. If we let the child choose its
          // size, then use what the child requested. Otherwise, use the value
          // specified in the style information
          if ((eStyleUnit_Auto == position->mWidth.GetUnit()) ||
              ((desiredSize.width > availSize.width) &&
               (NS_STYLE_OVERFLOW_VISIBLE == display->mOverflow))) {
            rect.width = desiredSize.width;
          }
          if ((eStyleUnit_Auto == position->mHeight.GetUnit()) ||
              (NS_UNCONSTRAINEDSIZE == rect.height) ||
              ((desiredSize.height > rect.height) &&
               (NS_STYLE_OVERFLOW_VISIBLE == display->mOverflow))) {
            rect.height = desiredSize.height;
          }
          absoluteFrame->SetRect(rect);
        }
      }
    }
  }
}

nsIView*
nsBodyFrame::CreateAbsoluteView(nsIStyleContext* aStyleContext) const
{
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    aStyleContext->GetStyleData(eStyleStruct_Display);
  const nsStylePosition* position = (const nsStylePosition*)
    aStyleContext->GetStyleData(eStyleStruct_Position);
  const nsStyleColor* color = (const nsStyleColor*)
    aStyleContext->GetStyleData(eStyleStruct_Color);

  nsIView*  view;

  static NS_DEFINE_IID(kViewCID, NS_VIEW_CID);
  static NS_DEFINE_IID(kIViewIID, NS_IVIEW_IID);

  // Create the view
  nsresult result = nsRepository::CreateInstance(kViewCID, 
                                                 nsnull, 
                                                 kIViewIID, 
                                                 (void **)&view);
  if (NS_OK == result) {
    // XXX Even though since we're absolutely positioned we should have a view,
    // we might not...
    nsIView*  containingView;
    GetView(containingView);
    if (nsnull == containingView) {
      nsPoint offset;
      GetOffsetFromView(offset, containingView);
    }
  
    // Initialize the view
    nsIViewManager* viewManager;
    containingView->GetViewManager(viewManager);
    NS_ASSERTION(nsnull != viewManager, "null view manager");

    // Is there a clip rect specified?
    nsViewClip    clip = {0, 0, 0, 0};
    PRUint8       clipType = (display->mClipFlags & NS_STYLE_CLIP_TYPE_MASK);
    nsViewClip*   pClip = nsnull;

    if (NS_STYLE_CLIP_RECT == clipType) {
      if ((NS_STYLE_CLIP_LEFT_AUTO & display->mClipFlags) == 0) {
        clip.mLeft = display->mClip.left;
      }
      if ((NS_STYLE_CLIP_RIGHT_AUTO & display->mClipFlags) == 0) {
        clip.mRight = display->mClip.right;
      }
      if ((NS_STYLE_CLIP_TOP_AUTO & display->mClipFlags) == 0) {
        clip.mTop = display->mClip.top;
      }
      if ((NS_STYLE_CLIP_BOTTOM_AUTO & display->mClipFlags) == 0) {
        clip.mBottom = display->mClip.bottom;
      }
      pClip = &clip;
    }
    else if (NS_STYLE_CLIP_INHERIT == clipType) {
      // XXX need to handle clip inherit (get from parent style context)
      NS_NOTYETIMPLEMENTED("clip inherit");
    }

    // Get the z-index to use
    PRInt32 zIndex = 0;
    if (position->mZIndex.GetUnit() == eStyleUnit_Integer) {
      zIndex = position->mZIndex.GetIntValue();
    } else if (position->mZIndex.GetUnit() == eStyleUnit_Auto) {
      zIndex = 0;
    } else if (position->mZIndex.GetUnit() == eStyleUnit_Inherit) {
      // XXX need to handle z-index "inherit"
      NS_NOTYETIMPLEMENTED("zIndex: inherit");
    }
     
    // Initialize the view with a size of (0, 0). When we're done reflowing
    // the frame the view will be sized and positioned
    view->Init(viewManager, nsRect(0, 0, 0, 0), containingView, nsnull,
               nsnull, nsnull, pClip);
    viewManager->InsertChild(containingView, view, zIndex);
    // If the background color is transparent then mark the view as having
    // transparent content.
    // XXX We could try and be smarter about this and check whether there's
    // a background image. If there is a background image and the image is
    // fully opaque then we don't need to mark the view as having transparent
    // content...
    if (NS_STYLE_BG_COLOR_TRANSPARENT & color->mBackgroundFlags) {
      viewManager->SetViewContentTransparency(view, PR_TRUE);
    }
    viewManager->SetViewOpacity(view, color->mOpacity);
    NS_RELEASE(viewManager);
  }

  return view;
}

// Translate aPoint from aFrameFrom's coordinate space to our coordinate space
void nsBodyFrame::TranslatePoint(nsIFrame* aFrameFrom, nsPoint& aPoint) const
{
  nsIFrame* parent;

  aFrameFrom->GetGeometricParent(parent);
  while ((nsnull != parent) && (parent != (nsIFrame*)this)) {
    nsPoint origin;

    parent->GetOrigin(origin);
    aPoint += origin;
    parent->GetGeometricParent(parent);
  }
}

void nsBodyFrame::ComputeAbsoluteFrameBounds(nsIFrame*                aAnchorFrame,
                                             const nsHTMLReflowState& aReflowState,
                                             const nsStylePosition*   aPosition,
                                             nsRect&                  aRect) const
{
  // Compute the offset and size of the view based on the position properties,
  // and the inner rect of the containing block (which we get from the reflow
  // state)
  //
  // If either the left or top are 'auto' then get the offset of the anchor
  // frame from this frame
  nsPoint offset;
  if ((eStyleUnit_Auto == aPosition->mLeftOffset.GetUnit()) ||
      (eStyleUnit_Auto == aPosition->mTopOffset.GetUnit())) {
    aAnchorFrame->GetOrigin(offset);
    TranslatePoint(aAnchorFrame, offset);
  }

  // left-offset
  if (eStyleUnit_Auto == aPosition->mLeftOffset.GetUnit()) {
    // Use the current x-offset of the anchor frame translated into our
    // coordinate space
    aRect.x = offset.x;
  } else if (eStyleUnit_Coord == aPosition->mLeftOffset.GetUnit()) {
    aRect.x = aPosition->mLeftOffset.GetCoordValue();
  } else {
    NS_ASSERTION(eStyleUnit_Percent == aPosition->mLeftOffset.GetUnit(),
                 "unexpected offset type");
    // Percentage values refer to the width of the containing block. If the
    // width is unconstrained then just use 0
    if (NS_UNCONSTRAINEDSIZE == aReflowState.maxSize.width) {
      aRect.x = 0;
    } else {
      aRect.x = (nscoord)((float)aReflowState.maxSize.width *
                          aPosition->mLeftOffset.GetPercentValue());
    }
  }

  // top-offset
  if (eStyleUnit_Auto == aPosition->mTopOffset.GetUnit()) {
    // Use the current y-offset of the anchor frame translated into our
    // coordinate space
    aRect.y = offset.y;
  } else if (eStyleUnit_Coord == aPosition->mTopOffset.GetUnit()) {
    aRect.y = aPosition->mTopOffset.GetCoordValue();
  } else {
    NS_ASSERTION(eStyleUnit_Percent == aPosition->mTopOffset.GetUnit(),
                 "unexpected offset type");
    // Percentage values refer to the height of the containing block. If the
    // height is unconstrained then interpret it like 'auto'
    if (NS_UNCONSTRAINEDSIZE == aReflowState.maxSize.height) {
      aRect.y = offset.y;
    } else {
      aRect.y = (nscoord)((float)aReflowState.maxSize.height *
                          aPosition->mTopOffset.GetPercentValue());
    }
  }

  // XXX We aren't properly handling 'auto' width and height
  // XXX The width/height represent the size of the content area only, and not
  // the frame size...

  // width
  if (eStyleUnit_Auto == aPosition->mWidth.GetUnit()) {
    // Use the right-edge of the containing block
    aRect.width = aReflowState.maxSize.width;
  } else if (eStyleUnit_Coord == aPosition->mWidth.GetUnit()) {
    aRect.width = aPosition->mWidth.GetCoordValue();
  } else {
    NS_ASSERTION(eStyleUnit_Percent == aPosition->mWidth.GetUnit(),
                 "unexpected width type");
    // Percentage values refer to the width of the containing block
    if (NS_UNCONSTRAINEDSIZE == aReflowState.maxSize.width) {
      aRect.width = NS_UNCONSTRAINEDSIZE;
    } else {
      aRect.width = (nscoord)((float)aReflowState.maxSize.width *
                              aPosition->mWidth.GetPercentValue());
    }
  }

  // height
  if (eStyleUnit_Auto == aPosition->mHeight.GetUnit()) {
    // Allow it to be as high as it wants
    aRect.height = NS_UNCONSTRAINEDSIZE;
  } else if (eStyleUnit_Coord == aPosition->mHeight.GetUnit()) {
    aRect.height = aPosition->mHeight.GetCoordValue();
  } else {
    NS_ASSERTION(eStyleUnit_Percent == aPosition->mHeight.GetUnit(),
                 "unexpected height type");
    // Percentage values refer to the height of the containing block. If the
    // height is unconstrained, then interpret it like 'auto' and make the
    // height unconstrained
    if (NS_UNCONSTRAINEDSIZE == aReflowState.maxSize.height) {
      aRect.height = NS_UNCONSTRAINEDSIZE;
    } else {
      aRect.height = (nscoord)((float)aReflowState.maxSize.height * 
                               aPosition->mHeight.GetPercentValue());
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// nsHTMLContainerFrame

// XXX use same logic as block frame?
PRIntn nsBodyFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != mPrevInFlow) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != mNextInFlow) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostics

NS_IMETHODIMP
nsBodyFrame::ListTag(FILE* out) const
{
  fprintf(out, "Body<");
  nsIAtom* atom;
  mContent->GetTag(atom);
  if (nsnull != atom) {
    nsAutoString tmp;
    atom->ToString(tmp);
    fputs(tmp, out);
  }
  fprintf(out, ">(%d)@%p", ContentIndexInContainer(this), this);
  return NS_OK;
}

NS_METHOD nsBodyFrame::VerifyTree() const
{
#ifdef NS_DEBUG
  NS_ASSERTION(0 == (mState & NS_FRAME_IN_REFLOW), "frame is in reflow");

  // Check our child count
  PRInt32 len = LengthOf(mFirstChild);
  NS_ASSERTION(len == mChildCount, "bad child count");
  nsIFrame* lastChild = LastFrame(mFirstChild);
   
  if (len != 0) {
    NS_ASSERTION(nsnull != lastChild, "bad last child");
  }
  NS_ASSERTION(nsnull == mOverflowList, "bad overflow list");

  // Verify the child's tree is valid
  nsIFrame* child = mFirstChild;
  while (nsnull != child) {
    // Make sure that the child's tree is valid
    child->VerifyTree();
    child->GetNextSibling(child);
  }

#endif
  return NS_OK;
}


