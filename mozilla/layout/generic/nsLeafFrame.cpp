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
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"

nsLeafFrame::~nsLeafFrame()
{
}

NS_IMETHODIMP
nsLeafFrame::Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer)
{
  if (eFramePaintLayer_Underlay == aWhichLayer) {
    const nsStyleDisplay* disp = (const nsStyleDisplay*)
      mStyleContext->GetStyleData(eStyleStruct_Display);
    if (disp->mVisible) {
      const nsStyleColor* myColor = (const nsStyleColor*)
        mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
        mStyleContext->GetStyleData(eStyleStruct_Spacing);
      nsRect rect(0, 0, mRect.width, mRect.height);
      nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                      aDirtyRect, rect, *myColor, 0, 0);
      nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                  aDirtyRect, rect, *mySpacing, 0);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsLeafFrame::Reflow(nsIPresContext& aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsLeafFrame::Reflow: aMaxSize=%d,%d",
                  aReflowState.maxSize.width, aReflowState.maxSize.height));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  // XXX add in code to check for width/height being set via css
  // and if set use them instead of calling GetDesiredSize.


  GetDesiredSize(&aPresContext, aReflowState, aMetrics);
  nsMargin borderPadding;
  AddBordersAndPadding(&aPresContext, aReflowState, aMetrics, borderPadding);
  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.AddBorderPaddingToMaxElementSize(borderPadding);
    aMetrics.maxElementSize->width = aMetrics.width;
    aMetrics.maxElementSize->height = aMetrics.height;
  }
  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("exit nsLeafFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  return NS_OK;
}

// XXX how should border&padding effect baseline alignment?
// => descent = borderPadding.bottom for example
void
nsLeafFrame::AddBordersAndPadding(nsIPresContext* aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsHTMLReflowMetrics& aMetrics,
                                  nsMargin& aBorderPadding)
{
  nsHTMLReflowState::ComputeBorderPaddingFor(this,
                                             aReflowState.parentReflowState,
                                             aBorderPadding);
  aMetrics.width += aBorderPadding.left + aBorderPadding.right;
  aMetrics.height += aBorderPadding.top + aBorderPadding.bottom;
  aMetrics.ascent = aMetrics.height;
  aMetrics.descent = 0;
}

NS_IMETHODIMP
nsLeafFrame::ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent)
{
  // Generate a reflow command with this frame as the target frame
  nsIReflowCommand* cmd;
  nsresult          result;
                                                
  result = NS_NewHTMLReflowCommand(&cmd, this, nsIReflowCommand::ContentChanged);
  if (NS_OK == result) {
    nsIPresShell* shell = aPresContext->GetShell();
    shell->AppendReflowCommand(cmd);
    NS_RELEASE(shell);
    NS_RELEASE(cmd);
  }

  return result;
}

