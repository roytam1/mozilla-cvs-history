/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsIContent.h"
#include "nsLineLayout.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsLayoutAtoms.h"

#define nsFirstLetterFrameSuper nsHTMLContainerFrame

class nsFirstLetterFrame : public nsFirstLetterFrameSuper {
public:
  nsFirstLetterFrame();

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  NS_IMETHOD GetFrameName(nsString& aResult) const;
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD FindTextRuns(nsLineLayout& aLineLayout);

protected:
  virtual PRIntn GetSkipSides() const;

  void DrainOverflowFrames(nsIPresContext* aPresContext);
};

nsresult
NS_NewFirstLetterFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsFirstLetterFrame* it = new nsFirstLetterFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsFirstLetterFrame::nsFirstLetterFrame()
{
}

NS_IMETHODIMP
nsFirstLetterFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Letter", aResult);
}

NS_IMETHODIMP
nsFirstLetterFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::letterFrame;
  NS_ADDREF(*aType);
  return NS_OK;
}

PRIntn
nsFirstLetterFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsFirstLetterFrame::Init(nsIPresContext&  aPresContext,
                         nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIStyleContext* aContext,
                         nsIFrame*        aPrevInFlow)
{
  nsresult rv;
  nsIStyleContext* newSC = nsnull;
  if (aPrevInFlow) {
    // Get proper style context for ourselves
    nsIStyleContext* parentStyleContext;
    parentStyleContext = aContext->GetParent();
    if (parentStyleContext) {
      nsIAtom* atom = aPrevInFlow
        ? nsHTMLAtoms::mozLetterFrame
        : nsHTMLAtoms::firstLetterPseudo;
      rv = aPresContext.ResolvePseudoStyleContextFor(aContent, atom,
                                                     parentStyleContext,
                                                     PR_FALSE, &newSC);
      NS_RELEASE(parentStyleContext);
      if (NS_FAILED(rv)) {
        return rv;
      }

      if (nsnull != newSC) {
        if (newSC == aContext) {
          NS_RELEASE(newSC);
        }
        else {
          aContext = newSC;
        }
      }
    }
  }
  rv = nsFirstLetterFrameSuper::Init(aPresContext, aContent, aParent,
                                     aContext, aPrevInFlow);
  NS_IF_RELEASE(newSC);
  return rv;
}

NS_IMETHODIMP
nsFirstLetterFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                        nsIAtom*        aListName,
                                        nsIFrame*       aChildList)
{
  mFrames.SetFrames(aChildList);
  if (aChildList) {
    aPresContext.ReParentStyleContext(aChildList, mStyleContext);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFirstLetterFrame::FindTextRuns(nsLineLayout& aLineLayout)
{
  nsIFrame* frame = mFrames.FirstChild();
  while (nsnull != frame) {
    frame->FindTextRuns(aLineLayout);
    frame->GetNextSibling(&frame);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFirstLetterFrame::Reflow(nsIPresContext&          aPresContext,
                           nsHTMLReflowMetrics&     aMetrics,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aReflowStatus)
{
  nsresult rv = NS_OK;

  // Grab overflow list
  DrainOverflowFrames(&aPresContext);

  nsIFrame* kid = mFrames.FirstChild();
  nsIFrame* nextRCFrame = nsnull;
  if (aReflowState.reason == eReflowReason_Incremental) {
    nsIFrame* target;
    aReflowState.reflowCommand->GetTarget(target);
    if (this != target) {
      aReflowState.reflowCommand->GetNext(nextRCFrame);
    }
  }

  // Setup reflow state for our child
  nsSize availSize(aReflowState.availableWidth, aReflowState.availableHeight);
  const nsMargin& bp = aReflowState.mComputedBorderPadding;
  nscoord lr = bp.left + bp.right;
  nscoord tb = bp.top + bp.bottom;
  if (NS_UNCONSTRAINEDSIZE != availSize.width) {
    availSize.width -= lr;
  }
  if (NS_UNCONSTRAINEDSIZE != availSize.height) {
    availSize.height -= tb;
  }

  // Reflow the child
  if (!aReflowState.mLineLayout) {
    // When there is no lineLayout provided, we provide our own. The
    // only time that the first-letter-frame is not reflowing in a
    // line context is when its floating.
    nsHTMLReflowState rs(aPresContext, aReflowState, kid, availSize);
    nsLineLayout ll(aPresContext, nsnull, &aReflowState,
                    nsnull != aMetrics.maxElementSize);
    ll.BeginLineReflow(0, 0, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE,
                       PR_FALSE, PR_TRUE);
    rs.mLineLayout = &ll;
    ll.SetFirstLetterStyleOK(PR_TRUE);

    kid->WillReflow(aPresContext);
    kid->Reflow(aPresContext, aMetrics, rs, aReflowStatus);

    ll.EndLineReflow();
  }
  else {
    // Pretend we are a span and reflow the child frame
    nsLineLayout* ll = aReflowState.mLineLayout;
    ll->BeginSpan(this, &aReflowState, bp.left, availSize.width);
    ll->ReflowFrame(kid, &nextRCFrame, aReflowStatus, &aMetrics);
    nsSize size;
    ll->EndSpan(this, size, aMetrics.maxElementSize);
  }

  // Place and size the child and update the output metrics
  kid->MoveTo(&aPresContext, bp.left, bp.top);
  kid->SizeTo(&aPresContext, aMetrics.width, aMetrics.height);
  aMetrics.width += lr;
  aMetrics.height += tb;
  aMetrics.ascent += bp.top;
  aMetrics.descent += bp.bottom;
  if (aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width += lr;
    aMetrics.maxElementSize->height += tb;
  }

  // Create a continuation or remove existing continuations based on
  // the reflow completion status.
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    nsIFrame* kidNextInFlow;
    kid->GetNextInFlow(&kidNextInFlow);
    if (nsnull != kidNextInFlow) {
      // Remove all of the childs next-in-flows
      DeleteChildsNextInFlow(aPresContext, kid);
    }
  }
  else {
    // Create a continuation for the child frame if it doesn't already
    // have one.
    nsIFrame* nextInFlow;
    rv = CreateNextInFlow(aPresContext, this, kid, nextInFlow);
    if (NS_FAILED(rv)) {
      return rv;
    }

    // And then push it to our overflow list
    if (nextInFlow) {
      kid->SetNextSibling(nsnull);
      SetOverflowFrames(&aPresContext, nextInFlow);
    }
    else {
      nsIFrame* nextSib;
      kid->GetNextSibling(&nextSib);
      if (nextSib) {
        kid->SetNextSibling(nsnull);
        SetOverflowFrames(&aPresContext, nextSib);
      }
    }
  }

  return rv;
}

void
nsFirstLetterFrame::DrainOverflowFrames(nsIPresContext* aPresContext)
{
  nsIFrame* overflowFrames;

  // Check for an overflow list with our prev-in-flow
  nsFirstLetterFrame* prevInFlow = (nsFirstLetterFrame*)mPrevInFlow;
  if (nsnull != prevInFlow) {
    overflowFrames = prevInFlow->GetOverflowFrames(aPresContext, PR_TRUE);
    if (overflowFrames) {
      NS_ASSERTION(mFrames.IsEmpty(), "bad overflow list");

      // When pushing and pulling frames we need to check for whether any
      // views need to be reparented.
      nsIFrame* f = overflowFrames;
      while (f) {
        nsHTMLContainerFrame::ReparentFrameView(aPresContext, f, prevInFlow, this);
        f->GetNextSibling(&f);
      }
      mFrames.InsertFrames(this, nsnull, overflowFrames);
    }
  }

  // It's also possible that we have an overflow list for ourselves
  overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nsnull, overflowFrames);
  }

  // Now repair our first frames style context (since we only reflow
  // one frame there is no point in doing any other ones until they
  // are reflowed)
  nsIFrame* kid = mFrames.FirstChild();
  if (kid) {
    nsCOMPtr<nsIStyleContext> sc;
    nsCOMPtr<nsIContent> kidContent;
    kid->GetContent(getter_AddRefs(kidContent));
    if (kidContent) {
      aPresContext->ResolveStyleContextFor(kidContent, mStyleContext,
                                           PR_FALSE, getter_AddRefs(sc));
      if (sc) {
        kid->SetStyleContext(aPresContext, sc);
      }
    }
  }
}
