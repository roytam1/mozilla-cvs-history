/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// vim:cindent:ts=2:et:sw=2:
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Steve Clark <buster@netscape.com>
 *   Robert O'Callahan <roc+moz@cs.cmu.edu>
 *   L. David Baron <dbaron@dbaron.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsIPresContext.h"
#include "nsLayoutAtoms.h"
#include "nsIFrame.h"
#include "nsIFrameManager.h"

#include "nsINameSpaceManager.h"
#include "nsHTMLAtoms.h"


#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsIPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       PRBool aBlockMarginRoot)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mPrevBottomMargin(),
    mLineNumber(0),
    mFlags(0),
    mFloaterBreakType(NS_STYLE_CLEAR_NONE)
{
  const nsMargin& borderPadding = BorderPadding();

  if (aBlockMarginRoot) {
    SetFlag(BRS_ISTOPMARGINROOT, PR_TRUE);
    SetFlag(BRS_ISBOTTOMMARGINROOT, PR_TRUE);
  }
  if (0 != aReflowState.mComputedBorderPadding.top) {
    SetFlag(BRS_ISTOPMARGINROOT, PR_TRUE);
  }
  if (0 != aReflowState.mComputedBorderPadding.bottom) {
    SetFlag(BRS_ISBOTTOMMARGINROOT, PR_TRUE);
  }
  if (GetFlag(BRS_ISTOPMARGINROOT)) {
    SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
  }
  
  mSpaceManager = aReflowState.mSpaceManager;

  NS_ASSERTION(mSpaceManager,
               "SpaceManager should be set in nsBlockReflowState" );
  if (mSpaceManager) {
    // Translate into our content area and then save the 
    // coordinate system origin for later.
    mSpaceManager->Translate(borderPadding.left, borderPadding.top);
    mSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);
  }

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mBlock->GetNextInFlow(NS_REINTERPRET_CAST(nsIFrame**, &mNextInFlow));
  mKidXMost = 0;

  // Compute content area width (the content area is inside the border
  // and padding)
  if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedWidth) {
    mContentArea.width = aReflowState.mComputedWidth;
  }
  else {
    if (NS_UNCONSTRAINEDSIZE == aReflowState.availableWidth) {
      mContentArea.width = NS_UNCONSTRAINEDSIZE;
      SetFlag(BRS_UNCONSTRAINEDWIDTH, PR_TRUE);
    }
    else if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
      // Choose a width based on the content (shrink wrap width) up
      // to the maximum width
      mContentArea.width = aReflowState.mComputedMaxWidth;
      SetFlag(BRS_SHRINKWRAPWIDTH, PR_TRUE);
    }
    else {
      nscoord lr = borderPadding.left + borderPadding.right;
      mContentArea.width = PR_MAX(0, aReflowState.availableWidth - lr);
    }
  }

  // Compute content area height. Unlike the width, if we have a
  // specified style height we ignore it since extra content is
  // managed by the "overflow" property. When we don't have a
  // specified style height then we may end up limiting our height if
  // the availableHeight is constrained (this situation occurs when we
  // are paginated).
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    // We are in a paginated situation. The bottom edge is just inside
    // the bottom border and padding. The content area height doesn't
    // include either border or padding edge.
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = PR_MAX(0, mBottomEdge - borderPadding.top);
  }
  else {
    // When we are not in a paginated situation then we always use
    // an constrained height.
    SetFlag(BRS_UNCONSTRAINEDHEIGHT, PR_TRUE);
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }

  mY = borderPadding.top;
  mBand.Init(mSpaceManager, mContentArea);

  mPrevChild = nsnull;
  mCurrentLine = aFrame->end_lines();

  const nsStyleText* styleText = mBlock->GetStyleText();
  switch (styleText->mWhiteSpace) {
  case NS_STYLE_WHITESPACE_PRE:
  case NS_STYLE_WHITESPACE_NOWRAP:
    SetFlag(BRS_NOWRAP, PR_TRUE);
    break;
  default:
    SetFlag(BRS_NOWRAP, PR_FALSE);
    break;
  }

  SetFlag(BRS_COMPUTEMAXELEMENTWIDTH, aMetrics.mComputeMEW);
#ifdef DEBUG
  if (nsBlockFrame::gNoisyMaxElementWidth) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("BRS: setting compute-MEW to %d\n", aMetrics.mComputeMEW);
  }
#endif
  mMaxElementWidth = 0;
  SetFlag(BRS_COMPUTEMAXWIDTH, 
          (NS_REFLOW_CALC_MAX_WIDTH == (aMetrics.mFlags & NS_REFLOW_CALC_MAX_WIDTH)));
  mMaximumWidth = 0;

  mMinLineHeight = nsHTMLReflowState::CalcLineHeight(mPresContext,
                                                     aReflowState.rendContext,
                                                     aReflowState.frame);
}

nsBlockReflowState::~nsBlockReflowState()
{
  // Restore the coordinate system, unless the space manager is null,
  // which means it was just destroyed.
  if (mSpaceManager) {
    const nsMargin& borderPadding = BorderPadding();
    mSpaceManager->Translate(-borderPadding.left, -borderPadding.top);
  }
}

nsLineBox*
nsBlockReflowState::NewLineBox(nsIFrame* aFrame,
                               PRInt32 aCount,
                               PRBool aIsBlock)
{
  nsCOMPtr<nsIPresShell> shell;
  mPresContext->GetShell(getter_AddRefs(shell));

  return NS_NewLineBox(shell, aFrame, aCount, aIsBlock);
}

void
nsBlockReflowState::FreeLineBox(nsLineBox* aLine)
{
  if (aLine) {
    nsCOMPtr<nsIPresShell> presShell;
    mPresContext->GetShell(getter_AddRefs(presShell));
    
    aLine->Destroy(presShell);
  }
}

// Compute the amount of available space for reflowing a block frame
// at the current Y coordinate. This method assumes that
// GetAvailableSpace has already been called.
void
nsBlockReflowState::ComputeBlockAvailSpace(nsIFrame* aFrame,
                                           nsSplittableType aSplitType,
                                           const nsStyleDisplay* aDisplay,
                                           nsRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has floater count %d\n", aFrame, mBand.GetFloaterCount());
  mBand.List();
#endif
  aResult.y = mY;
  aResult.height = GetFlag(BRS_UNCONSTRAINEDHEIGHT)
    ? NS_UNCONSTRAINEDSIZE
    : mBottomEdge - mY;

  const nsMargin& borderPadding = BorderPadding();

  /* bug 18445: treat elements mapped to display: block such as text controls
   * just like normal blocks   */
  PRBool treatAsNotSplittable=PR_FALSE;
  nsCOMPtr<nsIAtom>frameType;
  aFrame->GetFrameType(getter_AddRefs(frameType));
  if (frameType) {
    // text controls are not splittable, so make a special case here
    // XXXldb Why not just set the frame state bit?
    if (nsLayoutAtoms::textInputFrame == frameType.get())
      treatAsNotSplittable = PR_TRUE;
  }

  if (NS_FRAME_SPLITTABLE_NON_RECTANGULAR == aSplitType ||    // normal blocks 
      NS_FRAME_NOT_SPLITTABLE == aSplitType ||                // things like images mapped to display: block
      PR_TRUE == treatAsNotSplittable)                        // text input controls mapped to display: block (special case)
  {
    if (mBand.GetFloaterCount()) {
      // Use the float-edge property to determine how the child block
      // will interact with the floater.
      const nsStyleBorder* borderStyle = aFrame->GetStyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  // content and only content does runaround of floaters
          // The child block will flow around the floater. Therefore
          // give it all of the available space.
          aResult.x = borderPadding.left;
          aResult.width = GetFlag(BRS_UNCONSTRAINEDWIDTH)
            ? NS_UNCONSTRAINEDSIZE
            : mContentArea.width;
           break;
        case NS_STYLE_FLOAT_EDGE_BORDER: 
        case NS_STYLE_FLOAT_EDGE_PADDING:
          {
            // The child block's border should be placed adjacent to,
            // but not overlap the floater(s).
            nsMargin m(0, 0, 0, 0);
            const nsStyleMargin* styleMargin = aFrame->GetStyleMargin();
            styleMargin->GetMargin(m); // XXX percentage margins
            if (NS_STYLE_FLOAT_EDGE_PADDING == borderStyle->mFloatEdge) {
              // Add in border too
              nsMargin b;
              borderStyle->GetBorder(b);
              m += b;
            }

            // determine left edge
            if (mBand.GetLeftFloaterCount()) {
              aResult.x = mAvailSpaceRect.x + borderPadding.left - m.left;
            }
            else {
              aResult.x = borderPadding.left;
            }

            // determine width
            if (GetFlag(BRS_UNCONSTRAINEDWIDTH)) {
              aResult.width = NS_UNCONSTRAINEDSIZE;
            }
            else {
              if (mBand.GetRightFloaterCount()) {
                if (mBand.GetLeftFloaterCount()) {
                  aResult.width = mAvailSpaceRect.width + m.left + m.right;
                }
                else {
                  aResult.width = mAvailSpaceRect.width + m.right;
                }
              }
              else {
                aResult.width = mAvailSpaceRect.width + m.left;
              }
            }
          }
          break;

        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            // The child block's margins should be placed adjacent to,
            // but not overlap the floater.
            aResult.x = mAvailSpaceRect.x + borderPadding.left;
            aResult.width = mAvailSpaceRect.width;
          }
          break;
      }
    }
    else {
      // Since there are no floaters present the float-edge property
      // doesn't matter therefore give the block element all of the
      // available space since it will flow around the floater itself.
      aResult.x = borderPadding.left;
      aResult.width = GetFlag(BRS_UNCONSTRAINEDWIDTH)
        ? NS_UNCONSTRAINEDSIZE
        : mContentArea.width;
    }
  }
  else {
    // The frame is clueless about the space manager and therefore we
    // only give it free space. An example is a table frame - the
    // tables do not flow around floaters.
    aResult.x = mAvailSpaceRect.x + borderPadding.left;
    aResult.width = mAvailSpaceRect.width;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

void
nsBlockReflowState::GetAvailableSpace(nscoord aY)
{
#ifdef DEBUG
  // Verify that the caller setup the coordinate system properly
  nscoord wx, wy;
  mSpaceManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mSpaceManagerX) && (wy == mSpaceManagerY),
               "bad coord system");
#endif

  mBand.GetAvailableSpace(aY - BorderPadding().top, mAvailSpaceRect);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d count=%d\n",
           mAvailSpaceRect.x, mAvailSpaceRect.y,
           mAvailSpaceRect.width, mAvailSpaceRect.height,
           mBand.GetTrapezoidCount());
  }
#endif
}

PRBool
nsBlockReflowState::ClearPastFloaters(PRUint8 aBreakType)
{
  nscoord saveY, deltaY;

  PRBool applyTopMargin = PR_FALSE;
  switch (aBreakType) {
  case NS_STYLE_CLEAR_LEFT:
  case NS_STYLE_CLEAR_RIGHT:
  case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
    // Apply the previous margin before clearing
    saveY = mY + mPrevBottomMargin.get();
    ClearFloaters(saveY, aBreakType);
#ifdef NOISY_FLOATER_CLEARING
    nsFrame::ListTag(stdout, mBlock);
    printf(": ClearPastFloaters: mPrevBottomMargin=%d saveY=%d oldY=%d newY=%d deltaY=%d\n",
           mPrevBottomMargin, saveY, saveY - mPrevBottomMargin, mY,
           mY - saveY);
#endif

    // Determine how far we just moved. If we didn't move then there
    // was nothing to clear to don't mess with the normal margin
    // collapsing behavior. In either case we need to restore the Y
    // coordinate to what it was before the clear.
    deltaY = mY - saveY;
    if (0 != deltaY) {
      // Pretend that the distance we just moved is a previous
      // blocks bottom margin. Note that GetAvailableSpace has been
      // done so that the available space calculations will be done
      // after clearing the appropriate floaters.
      //
      // What we are doing here is applying CSS2 section 9.5.2's
      // rules for clearing - "The top margin of the generated box
      // is increased enough that the top border edge is below the
      // bottom outer edge of the floating boxes..."
      //
      // What this will do is cause the top-margin of the block
      // frame we are about to reflow to be collapsed with that
      // distance.
      
      // XXXldb This doesn't handle collapsing with negative margins
      // correctly, although it's arguable what "correct" is.

      // XXX Are all the other margins included by this point?
      mPrevBottomMargin.Zero();
      mPrevBottomMargin.Include(deltaY);
      mY = saveY;

      // Force margin to be applied in this circumstance
      applyTopMargin = PR_TRUE;
    }
    else {
      // Put mY back to its original value since no clearing
      // happened. That way the previous blocks bottom margin will
      // be applied properly.
      mY = saveY - mPrevBottomMargin.get();
    }
    break;
  }
  return applyTopMargin;
}

/*
 * Reconstruct the vertical margin before the line |aLine| in order to
 * do an incremental reflow that begins with |aLine| without reflowing
 * the line before it.  |aLine| may point to the fencepost at the end of
 * the line list, and it is used this way since we (for now, anyway)
 * always need to recover margins at the end of a block.
 *
 * The reconstruction involves walking backward through the line list to
 * find any collapsed margins preceding the line that would have been in
 * the reflow state's |mPrevBottomMargin| when we reflowed that line in
 * a full reflow (under the rule in CSS2 that all adjacent vertical
 * margins of blocks collapse).
 */
void
nsBlockReflowState::ReconstructMarginAbove(nsLineList::iterator aLine)
{
  mPrevBottomMargin.Zero();
  nsBlockFrame *block = mBlock;

  const nsStyleText* styleText = block->GetStyleText();
  PRBool isPre = NS_STYLE_WHITESPACE_PRE == styleText->mWhiteSpace ||
                 NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == styleText->mWhiteSpace;

  nsCompatibility mode;
  mPresContext->GetCompatibilityMode(&mode);

  nsLineList::iterator firstLine = block->begin_lines();
  for (;;) {
    --aLine;
    if (aLine->IsBlock()) {
      mPrevBottomMargin = aLine->GetCarriedOutBottomMargin();
      break;
    }
    PRBool isEmpty;
    aLine->IsEmpty(mode, isPre, &isEmpty);
    if (! isEmpty) {
      break;
    }
    if (aLine == firstLine) {
      // If the top margin was carried out (and thus already applied),
      // set it to zero.  Either way, we're done.
      if ((0 == mReflowState.mComputedBorderPadding.top) &&
          !(block->mState & NS_BLOCK_MARGIN_ROOT)) {
        mPrevBottomMargin.Zero();
      }
      break;
    }
  }
}

/**
 * Restore information about floaters into the space manager for an
 * incremental reflow, and simultaneously push the floaters by
 * |aDeltaY|, which is the amount |aLine| was pushed relative to its
 * parent.  The recovery of state is one of the things that makes
 * incremental reflow O(N^2) and this state should really be kept
 * around, attached to the frame tree.
 */
void
nsBlockReflowState::RecoverFloaters(nsLineList::iterator aLine,
                                    nscoord aDeltaY)
{
  if (aLine->HasFloaters()) {
    // Place the floaters into the space-manager again. Also slide
    // them, just like the regular frames on the line.
    nsFloaterCache* fc = aLine->GetFirstFloater();
    while (fc) {
      nsIFrame* floater = fc->mPlaceholder->GetOutOfFlowFrame();
      if (aDeltaY != 0) {
        fc->mRegion.y += aDeltaY;
        fc->mCombinedArea.y += aDeltaY;
        nsPoint p;
        floater->GetOrigin(p);
        floater->MoveTo(mPresContext, p.x, p.y + aDeltaY);
      }
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisySpaceManager) {
        nscoord tx, ty;
        mSpaceManager->GetTranslation(tx, ty);
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("RecoverFloaters: txy=%d,%d (%d,%d) ",
               tx, ty, mSpaceManagerX, mSpaceManagerY);
        nsFrame::ListTag(stdout, floater);
        printf(" aDeltaY=%d region={%d,%d,%d,%d}\n",
               aDeltaY, fc->mRegion.x, fc->mRegion.y,
               fc->mRegion.width, fc->mRegion.height);
      }
#endif
      mSpaceManager->AddRectRegion(floater, fc->mRegion);
      fc = fc->Next();
    }
  } else if (aLine->IsBlock()) {
    nsBlockFrame *kid = nsnull;
    aLine->mFirstChild->QueryInterface(kBlockFrameCID, (void**)&kid);
    if (kid) {
      nscoord kidx = kid->mRect.x, kidy = kid->mRect.y;
      mSpaceManager->Translate(kidx, kidy);
      for (nsBlockFrame::line_iterator line = kid->begin_lines(),
                                   line_end = kid->end_lines();
           line != line_end;
           ++line)
        // Pass 0, not the real DeltaY, since these floaters aren't
        // moving relative to their parent block, only relative to
        // the space manager.
        RecoverFloaters(line, 0);
      mSpaceManager->Translate(-kidx, -kidy);
    }
  }
}

/**
 * Everything done in this function is done O(N) times for each pass of
 * reflow so it is O(N*M) where M is the number of incremental reflow
 * passes.  That's bad.  Don't do stuff here.
 *
 * When this function is called, |aLine| has just been slid by |aDeltaY|
 * and the purpose of RecoverStateFrom is to ensure that the
 * nsBlockReflowState is in the same state that it would have been in
 * had the line just been reflowed.
 *
 * Most of the state recovery that we have to do involves floaters.
 */
void
nsBlockReflowState::RecoverStateFrom(nsLineList::iterator aLine,
                                     nscoord aDeltaY)
{
  // Make the line being recovered the current line
  mCurrentLine = aLine;

  // Recover mKidXMost and mMaxElementWidth
  nscoord xmost = aLine->mBounds.XMost();
  if (xmost > mKidXMost) {
#ifdef DEBUG
    if (CRAZY_WIDTH(xmost)) {
      nsFrame::ListTag(stdout, mBlock);
      printf(": WARNING: xmost:%d\n", xmost);
    }
#endif
#ifdef NOISY_KIDXMOST
    printf("%p RecoverState block %p aState.mKidXMost=%d\n", this, mBlock, xmost); 
#endif
    mKidXMost = xmost;
  }
  if (GetFlag(BRS_COMPUTEMAXELEMENTWIDTH)) {
#ifdef DEBUG
    if (nsBlockFrame::gNoisyMaxElementWidth) {
      nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
      printf("nsBlockReflowState::RecoverStateFrom block %p caching max width %d\n", mBlock, aLine->mMaxElementWidth);
    }
#endif
    UpdateMaxElementWidth(aLine->mMaxElementWidth);

    // Recover the floater MEWs for floaters in this line (but not in
    // blocks within it, since their MEWs are already part of the block's
    // MEW).
    if (aLine->HasFloaters()) {
      for (nsFloaterCache* fc = aLine->GetFirstFloater(); fc; fc = fc->Next())
        UpdateMaxElementWidth(fc->mMaxElementWidth);
    }
  }

  // If computing the maximum width, then update mMaximumWidth
  if (GetFlag(BRS_COMPUTEMAXWIDTH)) {
#ifdef NOISY_MAXIMUM_WIDTH
    printf("nsBlockReflowState::RecoverStateFrom block %p caching max width %d\n", mBlock, aLine->mMaximumWidth);
#endif
    UpdateMaximumWidth(aLine->mMaximumWidth);
  }

  // Place floaters for this line into the space manager
  if (aLine->HasFloaters() || aLine->IsBlock()) {
    // Undo border/padding translation since the nsFloaterCache's
    // coordinates are relative to the frame not relative to the
    // border/padding.
    const nsMargin& bp = BorderPadding();
    mSpaceManager->Translate(-bp.left, -bp.top);

    RecoverFloaters(aLine, aDeltaY);

#ifdef DEBUG
    if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisySpaceManager) {
      mSpaceManager->List(stdout);
    }
#endif
    // And then put the translation back again
    mSpaceManager->Translate(bp.left, bp.top);
  }
}

PRBool
nsBlockReflowState::IsImpactedByFloater() const
{
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockReflowState::IsImpactedByFloater %p returned %d\n", 
         this, mBand.GetFloaterCount());
#endif
  return mBand.GetFloaterCount() > 0;
}


void
nsBlockReflowState::InitFloater(nsLineLayout&       aLineLayout,
                                nsPlaceholderFrame* aPlaceholder,
                                nsReflowStatus&     aReflowStatus)
{
  // Set the geometric parent of the floater
  nsIFrame* floater = aPlaceholder->GetOutOfFlowFrame();
  floater->SetParent(mBlock);

  // Then add the floater to the current line and place it when
  // appropriate
  AddFloater(aLineLayout, aPlaceholder, PR_TRUE, aReflowStatus);
}

// This is called by the line layout's AddFloater method when a
// place-holder frame is reflowed in a line. If the floater is a
// left-most child (it's x coordinate is at the line's left margin)
// then the floater is place immediately, otherwise the floater
// placement is deferred until the line has been reflowed.

// XXXldb This behavior doesn't quite fit with CSS1 and CSS2 --
// technically we're supposed let the current line flow around the
// float as well unless it won't fit next to what we already have.
// But nobody else implements it that way...
void
nsBlockReflowState::AddFloater(nsLineLayout&       aLineLayout,
                               nsPlaceholderFrame* aPlaceholder,
                               PRBool              aInitialReflow,
                               nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(mBlock->end_lines() != mCurrentLine, "null ptr");

  aReflowStatus = NS_FRAME_COMPLETE;
  // Allocate a nsFloaterCache for the floater
  nsFloaterCache* fc = mFloaterCacheFreeList.Alloc();
  fc->mPlaceholder = aPlaceholder;
  fc->mIsCurrentLineFloater = aLineLayout.CanPlaceFloaterNow();
  fc->mMaxElementWidth = 0;

  // Now place the floater immediately if possible. Otherwise stash it
  // away in mPendingFloaters and place it later.
  if (fc->mIsCurrentLineFloater) {
    // Record this floater in the current-line list
    mCurrentLineFloaters.Append(fc);

    // Because we are in the middle of reflowing a placeholder frame
    // within a line (and possibly nested in an inline frame or two
    // that's a child of our block) we need to restore the space
    // manager's translation to the space that the block resides in
    // before placing the floater.
    nscoord ox, oy;
    mSpaceManager->GetTranslation(ox, oy);
    nscoord dx = ox - mSpaceManagerX;
    nscoord dy = oy - mSpaceManagerY;
    mSpaceManager->Translate(-dx, -dy);

    // And then place it
    PRBool isLeftFloater;
    FlowAndPlaceFloater(fc, &isLeftFloater, aReflowStatus);    

    // Pass on updated available space to the current inline reflow engine
    GetAvailableSpace();
    aLineLayout.UpdateBand(mAvailSpaceRect.x + BorderPadding().left, mY,
                           GetFlag(BRS_UNCONSTRAINEDWIDTH) ? NS_UNCONSTRAINEDSIZE : mAvailSpaceRect.width,
                           mAvailSpaceRect.height,
                           isLeftFloater,
                           aPlaceholder->GetOutOfFlowFrame());

    // Restore coordinate system
    mSpaceManager->Translate(dx, dy);
  }
  else {
    // This floater will be placed after the line is done (it is a
    // below-current-line floater).
    mBelowCurrentLineFloaters.Append(fc);
  }
}

void
nsBlockReflowState::UpdateMaxElementWidth(nscoord aMaxElementWidth)
{
#ifdef DEBUG
  nscoord oldWidth = mMaxElementWidth;
#endif
  if (aMaxElementWidth > mMaxElementWidth) {
    mMaxElementWidth = aMaxElementWidth;
  }
#ifdef DEBUG
  if (nsBlockFrame::gNoisyMaxElementWidth) {
    if (mMaxElementWidth != oldWidth) {
      nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
      if (NS_UNCONSTRAINEDSIZE == mReflowState.availableWidth) {
        printf("PASS1 ");
      }
      nsFrame::ListTag(stdout, mBlock);
      printf(": old max-element-width=%d new=%d\n",
             oldWidth, mMaxElementWidth);
    }
  }
#endif
}

void
nsBlockReflowState::UpdateMaximumWidth(nscoord aMaximumWidth)
{
  if (aMaximumWidth > mMaximumWidth) {
#ifdef NOISY_MAXIMUM_WIDTH
    printf("nsBlockReflowState::UpdateMaximumWidth block %p caching max width %d\n", mBlock, aMaximumWidth);
#endif
    mMaximumWidth = aMaximumWidth;
  }
}

PRBool
nsBlockReflowState::CanPlaceFloater(const nsRect& aFloaterRect,
                                    PRUint8 aFloats)
{
  // If the current Y coordinate is not impacted by any floaters
  // then by definition the floater fits.
  PRBool result = PR_TRUE;
  if (0 != mBand.GetFloaterCount()) {
    // XXX We should allow overflow by up to half a pixel here (bug 21193).
    if (mAvailSpaceRect.width < aFloaterRect.width) {
      // The available width is too narrow (and its been impacted by a
      // prior floater)
      result = PR_FALSE;
    }
    else {
      // At this point we know that there is enough horizontal space for
      // the floater (somewhere). Lets see if there is enough vertical
      // space.
      if (mAvailSpaceRect.height < aFloaterRect.height) {
        // The available height is too short. However, its possible that
        // there is enough open space below which is not impacted by a
        // floater.
        //
        // Compute the X coordinate for the floater based on its float
        // type, assuming its placed on the current line. This is
        // where the floater will be placed horizontally if it can go
        // here.
        nscoord xa;
        if (NS_STYLE_FLOAT_LEFT == aFloats) {
          xa = mAvailSpaceRect.x;
        }
        else {
          xa = mAvailSpaceRect.XMost() - aFloaterRect.width;

          // In case the floater is too big, don't go past the left edge
          // XXXldb This seems wrong, but we might want to fix bug 6976
          // first.
          if (xa < mAvailSpaceRect.x) {
            xa = mAvailSpaceRect.x;
          }
        }
        nscoord xb = xa + aFloaterRect.width;

        // Calculate the top and bottom y coordinates, again assuming
        // that the floater is placed on the current line.
        const nsMargin& borderPadding = BorderPadding();
        nscoord ya = mY - borderPadding.top;
        if (ya < 0) {
          // CSS2 spec, 9.5.1 rule [4]: "A floating box's outer top may not
          // be higher than the top of its containing block."  (Since the
          // containing block is the content edge of the block box, this
          // means the margin edge of the floater can't be higher than the
          // content edge of the block that contains it.)
          ya = 0;
        }
        nscoord yb = ya + aFloaterRect.height;

        nscoord saveY = mY;
        for (;;) {
          // Get the available space at the new Y coordinate
          mY += mAvailSpaceRect.height;
          GetAvailableSpace();

          if (0 == mBand.GetFloaterCount()) {
            // Winner. This band has no floaters on it, therefore
            // there can be no overlap.
            break;
          }

          // Check and make sure the floater won't intersect any
          // floaters on this band. The floaters starting and ending
          // coordinates must be entirely in the available space.
          if ((xa < mAvailSpaceRect.x) || (xb > mAvailSpaceRect.XMost())) {
            // The floater can't go here.
            result = PR_FALSE;
            break;
          }

          // See if there is now enough height for the floater.
          if (yb < mY + mAvailSpaceRect.height) {
            // Winner. The bottom Y coordinate of the floater is in
            // this band.
            break;
          }
        }

        // Restore Y coordinate and available space information
        // regardless of the outcome.
        mY = saveY;
        GetAvailableSpace();
      }
    }
  }
  return result;
}

void
nsBlockReflowState::FlowAndPlaceFloater(nsFloaterCache* aFloaterCache,
                                        PRBool*         aIsLeftFloater,
                                        nsReflowStatus& aReflowStatus)
{
  aReflowStatus = NS_FRAME_COMPLETE;
  // Save away the Y coordinate before placing the floater. We will
  // restore mY at the end after placing the floater. This is
  // necessary because any adjustments to mY during the floater
  // placement are for the floater only, not for any non-floating
  // content.
  nscoord saveY = mY;

  nsPlaceholderFrame* placeholder = aFloaterCache->mPlaceholder;
  nsIFrame*           floater = placeholder->GetOutOfFlowFrame();

  // Grab the floater's display information
  const nsStyleDisplay* floaterDisplay = floater->GetStyleDisplay();

  // This will hold the floater's geometry when we've found a place
  // for it to live.
  nsRect region;

  // The floater's old region, so we can propagate damage.
  nsRect oldRegion;
  floater->GetRect(oldRegion);
  oldRegion.Inflate(aFloaterCache->mMargins);

  // Enforce CSS2 9.5.1 rule [2], i.e., make sure that a float isn't
  // ``above'' another float that preceded it in the flow.
  mY = NS_MAX(mSpaceManager->GetLowestRegionTop() + BorderPadding().top, mY);

  // See if the floater should clear any preceeding floaters...
  if (NS_STYLE_CLEAR_NONE != floaterDisplay->mBreakType) {
    // XXXldb Does this handle vertical margins correctly?
    ClearFloaters(mY, floaterDisplay->mBreakType);
  }
  else {
    // Get the band of available space
    GetAvailableSpace();
  }

  // Reflow the floater
  mBlock->ReflowFloater(*this, placeholder, aFloaterCache, aReflowStatus);

  // Get the floaters bounding box and margin information
  floater->GetRect(region);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("flowed floater: ");
    nsFrame::ListTag(stdout, floater);
    printf(" (%d,%d,%d,%d)\n",
	   region.x, region.y, region.width, region.height);
  }
#endif

  // Adjust the floater size by its margin. That's the area that will
  // impact the space manager.
  region.width += aFloaterCache->mMargins.left + aFloaterCache->mMargins.right;
  region.height += aFloaterCache->mMargins.top + aFloaterCache->mMargins.bottom;

  // Find a place to place the floater. The CSS2 spec doesn't want
  // floaters overlapping each other or sticking out of the containing
  // block if possible (CSS2 spec section 9.5.1, see the rule list).
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floaterDisplay->mFloats) ||
	       (NS_STYLE_FLOAT_RIGHT == floaterDisplay->mFloats),
	       "invalid float type");

  // Can the floater fit here?
  PRBool keepFloaterOnSameLine = PR_FALSE;
  nsCompatibility mode;
  mPresContext->GetCompatibilityMode(&mode);

  while (! CanPlaceFloater(region, floaterDisplay->mFloats)) {
    // Nope. try to advance to the next band.
    if (NS_STYLE_DISPLAY_TABLE != floaterDisplay->mDisplay ||
          eCompatibility_NavQuirks != mode ) {

      mY += mAvailSpaceRect.height;
      GetAvailableSpace();
    } else {
      // IE handles floater tables in a very special way

      // see if the previous floater is also a table and has "align"
      nsFloaterCache* fc = mCurrentLineFloaters.Head();
      nsIFrame* prevFrame = nsnull;
      while (fc) {
        if (fc->mPlaceholder->GetOutOfFlowFrame() == floater) {
          break;
        }
        prevFrame = fc->mPlaceholder->GetOutOfFlowFrame();
        fc = fc->Next();
      }
      
      if(prevFrame) {
        //get the frame type
        nsIAtom* atom;
        prevFrame->GetFrameType(&atom);
        if(nsLayoutAtoms::tableOuterFrame == atom) {
          //see if it has "align="
          // IE makes a difference between align and he float property
          nsCOMPtr<nsIContent> content;
          prevFrame->GetContent(getter_AddRefs(content));
          if (content) {
            nsAutoString value;
            if (NS_CONTENT_ATTR_HAS_VALUE == content->GetAttr(kNameSpaceID_None, nsHTMLAtoms::align, value)) {
              // we're interested only if previous frame is align=left
              // IE messes things up when "right" (overlapping frames) 
              if (value.EqualsIgnoreCase("left")) {
                keepFloaterOnSameLine = PR_TRUE;
                // don't advance to next line (IE quirkie behaviour)
                // it breaks rule CSS2/9.5.1/1, but what the hell
                // since we cannot evangelize the world
                break;
              }
            }
          }
        }
      }

      // the table does not fit anymore in this line so advance to next band 
      mY += mAvailSpaceRect.height;
      GetAvailableSpace();
      // reflow the floater again now since we have more space
      mBlock->ReflowFloater(*this, placeholder, aFloaterCache, aReflowStatus);
      // Get the floaters bounding box and margin information
      floater->GetRect(region);
      // Adjust the floater size by its margin. That's the area that will
      // impact the space manager.
      region.width += aFloaterCache->mMargins.left + aFloaterCache->mMargins.right;
      region.height += aFloaterCache->mMargins.top + aFloaterCache->mMargins.bottom;

    }
  }
  // If the floater is continued, it will get the same absolute x value as its prev-in-flow
  nsRect prevRect(0,0,0,0);
  nsIFrame* prevInFlow;
  floater->GetPrevInFlow(&prevInFlow);
  if (prevInFlow) {
    prevInFlow->GetRect(prevRect);

    nsCOMPtr<nsIPresShell> presShell;
    mPresContext->GetShell(getter_AddRefs(presShell));
    nsCOMPtr<nsIFrameManager> frameManager;
    presShell->GetFrameManager(getter_AddRefs(frameManager));

    nsIFrame *placeParent, *placeParentPrev, *prevPlace, *prevPlaceParent;
    // If prevInFlow's placeholder is in a block that wasn't continued, we need to adjust 
    // prevRect.x to account for the missing frame offsets.
    placeholder->GetParent(&placeParent);
    placeParent->GetPrevInFlow(&placeParentPrev);
    frameManager->GetPlaceholderFrameFor(prevInFlow, &prevPlace);
    prevPlace->GetParent(&prevPlaceParent);

    for (nsIFrame* ancestor = prevPlaceParent; 
         ancestor && (ancestor != placeParentPrev); 
         ancestor->GetParent(&ancestor)) {
      nsRect rect;
      ancestor->GetRect(rect);
      prevRect.x += rect.x;
    }        
  }
  // Assign an x and y coordinate to the floater. Note that the x,y
  // coordinates are computed <b>relative to the translation in the
  // spacemanager</b> which means that the impacted region will be
  // <b>inside</b> the border/padding area.
  PRBool isLeftFloater;
  if (NS_STYLE_FLOAT_LEFT == floaterDisplay->mFloats) {
    isLeftFloater = PR_TRUE;
    region.x = (prevInFlow) ? prevRect.x : mAvailSpaceRect.x;
  }
  else {
    isLeftFloater = PR_FALSE;
    if (NS_UNCONSTRAINEDSIZE != mAvailSpaceRect.width) {
      nsIFrame* prevInFlow;
      floater->GetPrevInFlow(&prevInFlow);
      if (prevInFlow) {
        region.x = prevRect.x;
      }
      else if (!keepFloaterOnSameLine) {
        region.x = mAvailSpaceRect.XMost() - region.width;
      } 
      else {
        // this is the IE quirk (see few lines above)
        // the table is keept in the same line: don't let it overlap the previous floater 
        region.x = mAvailSpaceRect.x;
      }
    }
    else {
      // For unconstrained reflows, pretend that a right floater is
      // instead a left floater.  This will make us end up with the
      // correct unconstrained width, and we'll place it later.
      region.x = mAvailSpaceRect.x;
    }
  }
  *aIsLeftFloater = isLeftFloater;
  const nsMargin& borderPadding = BorderPadding();
  region.y = mY - borderPadding.top;
  if (region.y < 0) {
    // CSS2 spec, 9.5.1 rule [4]: "A floating box's outer top may not
    // be higher than the top of its containing block."  (Since the
    // containing block is the content edge of the block box, this
    // means the margin edge of the floater can't be higher than the
    // content edge of the block that contains it.)
    region.y = 0;
  }

  // Place the floater in the space manager
  // if the floater split, then take up all of the vertical height 
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) && 
      (NS_UNCONSTRAINEDSIZE != mContentArea.height)) {
    region.height = PR_MAX(region.height, mContentArea.height);
  }
#ifdef DEBUG
  nsresult rv =
#endif
  mSpaceManager->AddRectRegion(floater, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad floater placement");

  // If the floater's dimensions have changed, note the damage in the
  // space manager.
  if (region != oldRegion) {
    // XXXwaterson conservative: we could probably get away with noting
    // less damage; e.g., if only height has changed, then only note the
    // area into which the float has grown or from which the float has
    // shrunk.
    nscoord top = NS_MIN(region.y, oldRegion.y);
    nscoord bottom = NS_MAX(region.YMost(), oldRegion.YMost());
    mSpaceManager->IncludeInDamage(top, bottom);
  }

  // Save away the floaters region in the spacemanager, after making
  // it relative to the containing block's frame instead of relative
  // to the spacemanager translation (which is inset by the
  // border+padding).
  aFloaterCache->mRegion.x = region.x + borderPadding.left;
  aFloaterCache->mRegion.y = region.y + borderPadding.top;
  aFloaterCache->mRegion.width = region.width;
  aFloaterCache->mRegion.height = region.height;
#ifdef NOISY_SPACEMANAGER
  nscoord tx, ty;
  mSpaceManager->GetTranslation(tx, ty);
  nsFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloater: AddRectRegion: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tx, ty, mSpaceManagerX, mSpaceManagerY,
         aFloaterCache->mRegion.x, aFloaterCache->mRegion.y,
         aFloaterCache->mRegion.width, aFloaterCache->mRegion.height);
#endif

  // Set the origin of the floater frame, in frame coordinates. These
  // coordinates are <b>not</b> relative to the spacemanager
  // translation, therefore we have to factor in our border/padding.
  nscoord x = borderPadding.left + aFloaterCache->mMargins.left + region.x;
  nscoord y = borderPadding.top + aFloaterCache->mMargins.top + region.y;

  // If floater is relatively positioned, factor that in as well
  // XXXldb Should this be done after handling the combined area
  // below?
  if (NS_STYLE_POSITION_RELATIVE == floaterDisplay->mPosition) {
    x += aFloaterCache->mOffsets.left;
    y += aFloaterCache->mOffsets.top;
  }

  // Position the floater and make sure and views are properly
  // positioned. We need to explicitly position its child views as
  // well, since we're moving the floater after flowing it.
  floater->MoveTo(mPresContext, x, y);
  nsContainerFrame::PositionFrameView(mPresContext, floater);
  nsContainerFrame::PositionChildViews(mPresContext, floater);

  // Update the floater combined area state
  nsRect combinedArea = aFloaterCache->mCombinedArea;
  combinedArea.x += x;
  combinedArea.y += y;
  // When we are placing a right floater in an unconstrained situation or
  // when shrink wrapping, we don't apply it to the floater combined area
  // immediately, since there's no need to since we're guaranteed another
  // reflow, and since there's no need to change the code that was
  // necessary back when the floater was positioned relative to
  // NS_UNCONSTRAINEDSIZE.
  if (isLeftFloater ||
      !GetFlag(BRS_UNCONSTRAINEDWIDTH) ||
      !GetFlag(BRS_SHRINKWRAPWIDTH)) {
    nsBlockFrame::CombineRects(combinedArea, mFloaterCombinedArea);
  } else if (GetFlag(BRS_SHRINKWRAPWIDTH)) {
    // Mark the line dirty so we come back and re-place the floater once
    // the shrink wrap width is determined
    mCurrentLine->MarkDirty();
    SetFlag(BRS_NEEDRESIZEREFLOW, PR_TRUE);
  }

  // Now restore mY
  mY = saveY;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect r;
    floater->GetRect(r);
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("placed floater: ");
    nsFrame::ListTag(stdout, floater);
    printf(" %d,%d,%d,%d\n", r.x, r.y, r.width, r.height);
  }
#endif
}

/**
 * Place below-current-line floaters.
 */
PRBool
nsBlockReflowState::PlaceBelowCurrentLineFloaters(nsFloaterCacheList& aList)
{
  nsFloaterCache* fc = aList.Head();
  while (fc) {
    NS_ASSERTION(!fc->mIsCurrentLineFloater,
                 "A cl floater crept into the bcl floater list.");
    if (!fc->mIsCurrentLineFloater) {
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow) {
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("placing bcl floater: ");
        nsFrame::ListTag(stdout, fc->mPlaceholder->GetOutOfFlowFrame());
        printf("\n");
      }
#endif

      // Place the floater
      PRBool isLeftFloater;
      nsReflowStatus reflowStatus;
      FlowAndPlaceFloater(fc, &isLeftFloater, reflowStatus);

      if (NS_FRAME_IS_TRUNCATED(reflowStatus)) {
        // return before processing all of the floaters, since the line will be pushed.
        return PR_FALSE;
      }
      else if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus)) {
        // Create a continuation for the incomplete floater and its placeholder.
        nsresult rv = mBlock->SplitPlaceholder(*mPresContext, *fc->mPlaceholder);
        if (NS_FAILED(rv)) 
          return PR_FALSE;
      }
    }
    fc = fc->Next();
  }
  return PR_TRUE;
}

void
nsBlockReflowState::ClearFloaters(nscoord aY, PRUint8 aBreakType)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floaters: in: mY=%d aY=%d(%d)\n",
           mY, aY, aY - BorderPadding().top);
  }
#endif

#ifdef NOISY_FLOATER_CLEARING
  printf("nsBlockReflowState::ClearFloaters: aY=%d breakType=%d\n",
         aY, aBreakType);
  mSpaceManager->List(stdout);
#endif
  const nsMargin& bp = BorderPadding();
  nscoord newY = mBand.ClearFloaters(aY - bp.top, aBreakType);
  mY = newY + bp.top;
  GetAvailableSpace();

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floaters: out: mY=%d(%d)\n", mY, mY - bp.top);
  }
#endif
}

