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
#include "nsPageFrame.h"
#include "nsHTMLParts.h"
#include "nsIContent.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsIRenderingContext.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsLayoutAtoms.h"
#include "nsIStyleSet.h"
#include "nsIPresShell.h"

nsPageFrame::nsPageFrame()
{
}

NS_METHOD nsPageFrame::Reflow(nsIPresContext&          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  aStatus = NS_FRAME_COMPLETE;  // initialize out parameter

  if (eReflowReason_Incremental == aReflowState.reason) {
    // We don't expect the target of the reflow command to be page frame
#ifdef NS_DEUG
    NS_ASSERTION(nsnull != aReflowState.reflowCommand, "null reflow command");

    nsIFrame* target;
    aReflowState.reflowCommand->GetTarget(target);
    NS_ASSERTION(target != this, "page frame is reflow command target");
#endif
  
    // Verify the next reflow command frame is our one and only child frame
    nsIFrame* next;
    aReflowState.reflowCommand->GetNext(next);
    NS_ASSERTION(next == mFrames.FirstChild(), "bad reflow frame");

    // Dispatch the reflow command to our content child. Allow it to be as high
    // as it wants
    nsSize            maxSize(aReflowState.availableWidth, NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState kidReflowState(aPresContext, mFrames.FirstChild(),
                                     aReflowState,
                                     maxSize);
  
    kidReflowState.isTopOfPage = PR_TRUE;
    ReflowChild(mFrames.FirstChild(), aPresContext, aDesiredSize,
                kidReflowState, aStatus);
  
    // Place and size the child. Make sure the child is at least as
    // tall as our max size (the containing window)
    if (aDesiredSize.height < aReflowState.availableHeight) {
      aDesiredSize.height = aReflowState.availableHeight;
    }

    nsRect  rect(0, 0, aDesiredSize.width, aDesiredSize.height);
    mFrames.FirstChild()->SetRect(rect);

  } else {
    // Do we have any children?
    // XXX We should use the overflow list instead...
    if (mFrames.IsEmpty() && (nsnull != mPrevInFlow)) {
      nsPageFrame*  prevPage = (nsPageFrame*)mPrevInFlow;

      nsIFrame* prevLastChild = prevPage->mFrames.LastChild();

      // Create a continuing child of the previous page's last child
      nsIPresShell* presShell;
      nsIStyleSet*  styleSet;
      nsIFrame*     newFrame;

      aPresContext.GetShell(&presShell);
      presShell->GetStyleSet(&styleSet);
      NS_RELEASE(presShell);
      styleSet->CreateContinuingFrame(&aPresContext, prevLastChild, this, &newFrame);
      NS_RELEASE(styleSet);
      mFrames.SetFrames(newFrame);
    }

    // Resize our frame allowing it only to be as big as we are
    // XXX Pay attention to the page's border and padding...
    if (mFrames.NotEmpty()) {
      nsIFrame* frame = mFrames.FirstChild();
      nsSize  maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
      nsHTMLReflowState kidReflowState(aPresContext, frame, aReflowState,
                                       maxSize);
      kidReflowState.isTopOfPage = PR_TRUE;

      nsIHTMLReflow*    htmlReflow;
      if (NS_OK == frame->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow)) {
        // Get the child's desired size
        ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, aStatus);
  
        // Make sure the child is at least as tall as our max size (the containing window)
        if (aDesiredSize.height < aReflowState.availableHeight) {
          aDesiredSize.height = aReflowState.availableHeight;
        }
  
        // Place and size the child
        nsRect  rect(0, 0, aDesiredSize.width, aDesiredSize.height);
        frame->SetRect(rect);
        // XXX Should we be sending the DidReflow?
        htmlReflow->DidReflow(aPresContext, NS_FRAME_REFLOW_FINISHED);
  
        // Is the frame complete?
        if (NS_FRAME_IS_COMPLETE(aStatus)) {
          nsIFrame* childNextInFlow;
  
          frame->GetNextInFlow(childNextInFlow);
          NS_ASSERTION(nsnull == childNextInFlow, "bad child flow list");
        }
      }
    }

    // Return our desired size
    aDesiredSize.width = aReflowState.availableWidth;
    aDesiredSize.height = aReflowState.availableHeight;
  }

  return NS_OK;
}

NS_METHOD
nsPageFrame::CreateContinuingFrame(nsIPresContext&  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame)
{
  nsPageFrame* cf = new nsPageFrame;
  if (nsnull == cf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  cf->Init(aPresContext, mContent, aParent, aStyleContext);
  cf->AppendToFlow(this);
  aContinuingFrame = cf;
  return NS_OK;
}

NS_IMETHODIMP
nsPageFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::pageFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

NS_IMETHODIMP
nsPageFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Page", aResult);
}

//----------------------------------------------------------------------

nsresult
NS_NewPageFrame(nsIFrame*& aResult)
{
  nsPageFrame*  it = new nsPageFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = it;
  return NS_OK;
}

