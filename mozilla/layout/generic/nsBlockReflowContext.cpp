/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
// vim:cindent:ts=2:et:sw=2:
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 *   David Baron <dbaron@dbaron.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsBlockReflowContext.h"
#include "nsLineLayout.h"
#include "nsSpaceManager.h"
#include "nsIFontMetrics.h"
#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsLayoutAtoms.h"
#include "nsCOMPtr.h"
#include "nsLayoutUtils.h"

#ifdef NS_DEBUG
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_VERTICAL_MARGINS
#else
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_VERTICAL_MARGINS
#endif

nsBlockReflowContext::nsBlockReflowContext(nsPresContext* aPresContext,
                                           const nsHTMLReflowState& aParentRS)
  : mPresContext(aPresContext),
    mOuterReflowState(aParentRS),
    mMetrics()
{
  mStyleBorder = nsnull;
  mStyleMargin = nsnull;
  mStylePadding = nsnull;
}

PRBool
nsBlockReflowContext::ComputeCollapsedTopMargin(const nsHTMLReflowState& aRS,
  nsCollapsingMargin* aMargin, nsIFrame* aClearanceFrame, PRBool* aMayNeedRetry)
{
  // Include frame's top margin
  aMargin->Include(aRS.mComputedMargin.top);

  // The inclusion of the bottom margin when empty is done by the caller
  // since it doesn't need to be done by the top-level (non-recursive)
  // caller.

#ifdef NOISY_VERTICAL_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": %d => %d\n", aRS.mComputedMargin.top, aMargin->get());
#endif

  PRBool dirtiedLine = PR_FALSE;

  // Calculate the frame's generational top-margin from its child
  // blocks. Note that if the frame has a non-zero top-border or
  // top-padding then this step is skipped because it will be a margin
  // root.  It is also skipped if the frame is a margin root for other
  // reasons.
  void* bf;
  if (0 == aRS.mComputedBorderPadding.top &&
      !(aRS.frame->GetStateBits() & NS_BLOCK_MARGIN_ROOT) &&
      NS_SUCCEEDED(aRS.frame->QueryInterface(kBlockFrameCID, &bf))) {
    nsBlockFrame* block = NS_STATIC_CAST(nsBlockFrame*, aRS.frame);
    
    for (nsBlockFrame::line_iterator line = block->begin_lines(),
           line_end = block->end_lines();
         line != line_end; ++line) {
      if (!aClearanceFrame && line->HasClearance()) {
        // If we don't have a clearance frame, then we're computing
        // the collapsed margin in the first pass, assuming that all
        // lines have no clearance. So clear their clearance flags.
        line->ClearHasClearance();
        line->MarkDirty();
        dirtiedLine = PR_TRUE;
      }
      
      PRBool isEmpty = line->IsEmpty();
      if (line->IsBlock()) {
        nsBlockFrame* kidBlock = NS_STATIC_CAST(nsBlockFrame*, line->mFirstChild);
        if (kidBlock == aClearanceFrame) {
          line->SetHasClearance();
          line->MarkDirty();
          dirtiedLine = PR_TRUE;
          break;
        }
        // Here is where we recur. Now that we have determined that a
        // generational collapse is required we need to compute the
        // child blocks margin and so in so that we can look into
        // it. For its margins to be computed we need to have a reflow
        // state for it.
        nsSize availSpace(aRS.mComputedWidth, aRS.mComputedHeight);
        nsHTMLReflowState reflowState(kidBlock->GetPresContext(),
                                      aRS, kidBlock, availSpace);
        // Record that we're being optimistic by assuming the kid
        // has no clearance
        if (kidBlock->GetStyleDisplay()->mBreakType != NS_STYLE_CLEAR_NONE) {
          *aMayNeedRetry = PR_TRUE;
        }
        if (ComputeCollapsedTopMargin(reflowState, aMargin, aClearanceFrame, aMayNeedRetry)) {
          line->MarkDirty();
          dirtiedLine = PR_TRUE;
        }
        if (isEmpty)
          aMargin->Include(reflowState.mComputedMargin.bottom);
      }
      if (!isEmpty)
        break;
    }
  }
  
#ifdef NOISY_VERTICAL_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": => %d\n", aMargin.get());
#endif

  return dirtiedLine;
}

struct nsBlockHorizontalAlign {
  nscoord mXOffset;  // left edge
  nscoord mLeftMargin;
  nscoord mRightMargin;
};

// Given the width of the block frame and a suggested x-offset calculate
// the actual x-offset taking into account horizontal alignment. Also returns
// the actual left and right margin
void
nsBlockReflowContext::AlignBlockHorizontally(nscoord                 aWidth,
                                             nsBlockHorizontalAlign &aAlign)
{
  // Initialize OUT parameters
  aAlign.mLeftMargin = mMargin.left;
  aAlign.mRightMargin = mMargin.right;

  // Get style unit associated with the left and right margins
  nsStyleUnit leftUnit = mStyleMargin->mMargin.GetLeftUnit();
  nsStyleUnit rightUnit = mStyleMargin->mMargin.GetRightUnit();

  // Apply post-reflow horizontal alignment. When a block element
  // doesn't use it all of the available width then we need to
  // align it using the text-align property.
  if (NS_UNCONSTRAINEDSIZE != mSpace.width &&
      NS_UNCONSTRAINEDSIZE != mOuterReflowState.mComputedWidth) {
    // It is possible that the object reflowed was given a
    // constrained width and ended up picking a different width
    // (e.g. a table width a set width that ended up larger
    // because its contents required it). When this happens we
    // need to recompute auto margins because the reflow state's
    // computations are no longer valid.
    if (aWidth != mComputedWidth) {
      if (eStyleUnit_Auto == leftUnit) {
        aAlign.mXOffset = mSpace.x;
        aAlign.mLeftMargin = 0;
      }
      if (eStyleUnit_Auto == rightUnit) {
        aAlign.mRightMargin = 0;
      }
    }

    // Compute how much remaining space there is, and in special
    // cases apply it (normally we should get zero here because of
    // the logic in nsHTMLReflowState).
    nscoord remainingSpace = mSpace.XMost() - (aAlign.mXOffset + aWidth +
                             aAlign.mRightMargin);
    if (remainingSpace > 0) {
      // The block/table frame didn't use all of the available
      // space. Synthesize margins for its horizontal placement.
      if (eStyleUnit_Auto == leftUnit) {
        if (eStyleUnit_Auto == rightUnit) {
          // When both margins are auto, we center the block
          aAlign.mXOffset += remainingSpace / 2;
        }
        else {
          // When the left margin is auto we right align the block
          aAlign.mXOffset += remainingSpace;
        }
      }
      else if (eStyleUnit_Auto != rightUnit) {
        // The block/table doesn't have auto margins.

        // For normal (non-table) blocks we don't get here because
        // nsHTMLReflowState::CalculateBlockSideMargins handles this.
        // (I think there may be an exception to that, though...)

        // We use a special value of the text-align property for
        // HTML alignment (the CENTER element and DIV ALIGN=...)
        // since it acts on blocks and tables rather than just
        // being a text-align.
        // So, check the text-align value from the parent to see if
        // it has one of these special values.
        const nsStyleText* styleText = mOuterReflowState.mStyleText;
        if (styleText->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT) {
          aAlign.mXOffset += remainingSpace;
        } else if (styleText->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER) {
          aAlign.mXOffset += remainingSpace / 2;
        } else {
          // If we don't have a special text-align value indicating
          // HTML alignment, then use the CSS rules.

          // When neither margin is auto then the block is said to
          // be over constrained, Depending on the direction, choose
          // which margin to treat as auto.
          PRUint8 direction = mOuterReflowState.mStyleVisibility->mDirection;
          if (NS_STYLE_DIRECTION_RTL == direction) {
            // The left margin becomes auto
            aAlign.mXOffset += remainingSpace;
          }
          //else {
            // The right margin becomes auto which is a no-op
          //}
        }
      }
    }
  }
}

static void
nsPointDtor(void *aFrame, nsIAtom *aPropertyName,
            void *aPropertyValue, void *aDtorData)
{
  nsPoint *point = NS_STATIC_CAST(nsPoint*, aPropertyValue);
  delete point;
}

nsresult
nsBlockReflowContext::ReflowBlock(const nsRect&       aSpace,
                                  PRBool              aApplyTopMargin,
                                  nsCollapsingMargin& aPrevMargin,
                                  nscoord             aClearance,
                                  PRBool              aIsAdjacentWithTop,
                                  nsMargin&           aComputedOffsets,
                                  nsHTMLReflowState&  aFrameRS,
                                  nsReflowStatus&     aFrameReflowStatus)
{
  nsresult rv = NS_OK;
  mFrame = aFrameRS.frame;
  mSpace = aSpace;

  const nsStyleDisplay* display = mFrame->GetStyleDisplay();

  aComputedOffsets = aFrameRS.mComputedOffsets;
  if (NS_STYLE_POSITION_RELATIVE == display->mPosition) {
    nsPropertyTable *propTable = mPresContext->PropertyTable();

    nsPoint *offsets = NS_STATIC_CAST(nsPoint*,
        propTable->GetProperty(mFrame, nsLayoutAtoms::computedOffsetProperty));

    if (offsets)
      offsets->MoveTo(aComputedOffsets.left, aComputedOffsets.top);
    else {
      offsets = new nsPoint(aComputedOffsets.left, aComputedOffsets.top);
      if (offsets)
        propTable->SetProperty(mFrame, nsLayoutAtoms::computedOffsetProperty,
                               offsets, nsPointDtor, nsnull);
    }
  }

  aFrameRS.mLineLayout = nsnull;
  if (!aIsAdjacentWithTop) {
    aFrameRS.mFlags.mIsTopOfPage = PR_FALSE;  // make sure this is cleared
  }
  mComputedWidth = aFrameRS.mComputedWidth;

  if (aApplyTopMargin) {
    mTopMargin = aPrevMargin;

#ifdef NOISY_VERTICAL_MARGINS
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": reflowing ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" margin => %d, clearance => %d\n", mTopMargin.get(), aClearance);
#endif

    // Adjust the available height if its constrained so that the
    // child frame doesn't think it can reflow into its margin area.
    if (NS_UNCONSTRAINEDSIZE != aFrameRS.availableHeight) {
      aFrameRS.availableHeight -= mTopMargin.get() + aClearance;
    }
  }

  // Compute x/y coordinate where reflow will begin. Use the rules
  // from 10.3.3 to determine what to apply. At this point in the
  // reflow auto left/right margins will have a zero value.
  mMargin = aFrameRS.mComputedMargin;
  mStyleBorder = aFrameRS.mStyleBorder;
  mStyleMargin = aFrameRS.mStyleMargin;
  mStylePadding = aFrameRS.mStylePadding;
  nscoord x;
  nscoord y = mSpace.y + mTopMargin.get() + aClearance;

  // If it's a right floated element, then calculate the x-offset
  // differently
  if (NS_STYLE_FLOAT_RIGHT == aFrameRS.mStyleDisplay->mFloats) {
    nscoord frameWidth;
     
    if (NS_UNCONSTRAINEDSIZE == aFrameRS.mComputedWidth) {
      // Use the current frame width
      frameWidth = mFrame->GetSize().width;
    } else {
      frameWidth = aFrameRS.mComputedWidth +
                   aFrameRS.mComputedBorderPadding.left +
                   aFrameRS.mComputedBorderPadding.right;
    }

    // if this is an unconstrained width reflow, then just place the float at the left margin
    if (NS_UNCONSTRAINEDSIZE == mSpace.width)
      x = mSpace.x;
    else
      x = mSpace.XMost() - mMargin.right - frameWidth;

  } else {
    x = mSpace.x + mMargin.left;
  }
  mX = x;
  mY = y;

  // XXX We should always be doing AlignBlockHorizontally here now, if we
  // need the function at all.  It should probably be replaced by code in
  // nsHTMLReflowState, though.

  // If it's an auto-width table, then it doesn't behave like other blocks
  // XXX why not for a floating table too?
  if (aFrameRS.mStyleDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE &&
      !aFrameRS.mStyleDisplay->IsFloating()) {
    // If this isn't the table's initial reflow, then use its existing
    // width to determine where it will be placed horizontally
    if (aFrameRS.reason != eReflowReason_Initial) {
      nsBlockHorizontalAlign  align;

      align.mXOffset = x;
      AlignBlockHorizontally(mFrame->GetSize().width, align);
      // Don't reset "mX". because PlaceBlock() will recompute the
      // x-offset and expects "mX" to be at the left margin edge
      x = align.mXOffset;
    }
  }

   // Compute the translation to be used for adjusting the spacemanagager
   // coordinate system for the frame.  The spacemanager coordinates are
   // <b>inside</b> the callers border+padding, but the x/y coordinates
   // are not (recall that frame coordinates are relative to the parents
   // origin and that the parents border/padding is <b>inside</b> the
   // parent frame. Therefore we have to subtract out the parents
   // border+padding before translating.
   nscoord tx = x - mOuterReflowState.mComputedBorderPadding.left;
   nscoord ty = y - mOuterReflowState.mComputedBorderPadding.top;
 
  // If the element is relatively positioned, then adjust x and y accordingly
  if (NS_STYLE_POSITION_RELATIVE == aFrameRS.mStyleDisplay->mPosition) {
    x += aFrameRS.mComputedOffsets.left;
    y += aFrameRS.mComputedOffsets.top;
  }

  // Let frame know that we are reflowing it
  mFrame->WillReflow(mPresContext);

  // Position it and its view (if it has one)
  // Note: Use "x" and "y" and not "mX" and "mY" because they more accurately
  // represents where we think the block will be placed
  mFrame->SetPosition(nsPoint(x, y));
  nsContainerFrame::PositionFrameView(mFrame);

#ifdef DEBUG
  mMetrics.width = nscoord(0xdeadbeef);
  mMetrics.height = nscoord(0xdeadbeef);
  mMetrics.ascent = nscoord(0xdeadbeef);
  mMetrics.descent = nscoord(0xdeadbeef);
#endif

  mOuterReflowState.mSpaceManager->Translate(tx, ty);
  rv = mFrame->Reflow(mPresContext, mMetrics, aFrameRS, aFrameReflowStatus);
  mOuterReflowState.mSpaceManager->Translate(-tx, -ty);

#ifdef DEBUG
  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
    if (CRAZY_WIDTH(mMetrics.width) || CRAZY_HEIGHT(mMetrics.height)) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" metrics=%d,%d!\n", mMetrics.width, mMetrics.height);
    }
    if ((mMetrics.width == nscoord(0xdeadbeef)) ||
        (mMetrics.height == nscoord(0xdeadbeef)) ||
        (mMetrics.ascent == nscoord(0xdeadbeef)) ||
        (mMetrics.descent == nscoord(0xdeadbeef))) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" didn't set whad %d,%d,%d,%d!\n",
             mMetrics.width, mMetrics.height,
             mMetrics.ascent, mMetrics.descent);
    }
  }
#endif

  if (!(NS_FRAME_OUTSIDE_CHILDREN & mFrame->GetStateBits())) {
    // Provide overflow area for child that doesn't have any
    mMetrics.mOverflowArea.x = 0;
    mMetrics.mOverflowArea.y = 0;
    mMetrics.mOverflowArea.width = mMetrics.width;
    mMetrics.mOverflowArea.height = mMetrics.height;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus) ||
      (mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    // If frame is complete and has a next-in-flow, we need to delete
    // them now. Do not do this when a break-before is signaled because
    // the frame is going to get reflowed again (and may end up wanting
    // a next-in-flow where it ends up), unless it is an out of flow frame.
    if (NS_FRAME_IS_COMPLETE(aFrameReflowStatus)) {
      nsIFrame* kidNextInFlow = mFrame->GetNextInFlow();
      if (nsnull != kidNextInFlow) {
        // Remove all of the childs next-in-flows. Make sure that we ask
        // the right parent to do the removal (it's possible that the
        // parent is not this because we are executing pullup code)
/* XXX promote DeleteChildsNextInFlow to nsIFrame to elminate this cast */
        NS_STATIC_CAST(nsHTMLContainerFrame*, kidNextInFlow->GetParent())
          ->DeleteNextInFlowChild(mPresContext, kidNextInFlow);
      }
    }
  }

  return rv;
}

/**
 * Attempt to place the block frame within the available space.  If
 * it fits, apply horizontal positioning (CSS 10.3.3), collapse
 * margins (CSS2 8.3.1). Also apply relative positioning.
 */
PRBool
nsBlockReflowContext::PlaceBlock(const nsHTMLReflowState& aReflowState,
                                 PRBool                   aForceFit,
                                 nsLineBox*               aLine,
                                 const nsMargin&          aComputedOffsets,
                                 nsCollapsingMargin&      aBottomMarginResult,
                                 nsRect&                  aInFlowBounds,
                                 nsRect&                  aCombinedRect)
{
  // Compute collapsed bottom margin value
  aBottomMarginResult = mMetrics.mCarriedOutBottomMargin;
  aBottomMarginResult.Include(mMargin.bottom);

  // See if the block will fit in the available space
  PRBool fits = PR_TRUE;
  nscoord x = mX;
  nscoord y = mY;

  // Check whether the block's bottom margin collapses with its top
  // margin. See CSS 2.1 section 8.3.1; those rules seem to match
  // nsBlockFrame::IsEmpty(). Any such block must have zero height so
  // check that first.
  if (0 == mMetrics.height && !aLine->HasClearance() &&
      aLine->CachedIsEmpty())
  {
    // Collapse the bottom margin with the top margin that was already
    // applied.
    aBottomMarginResult.Include(mTopMargin);
#ifdef NOISY_VERTICAL_MARGINS
    printf("  ");
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" -- collapsing top & bottom margin together; y=%d spaceY=%d\n",
           y, mSpace.y);
#endif

    y = mSpace.y;

    // Now place the frame and complete the reflow process
    nsContainerFrame::FinishReflowChild(mFrame, mPresContext, &aReflowState, mMetrics, x, y, 0);

    // Empty blocks do not have anything special done to them and they
    // always fit. Note: don't force the width to 0
    aInFlowBounds = nsRect(x, y, mMetrics.width, 0);

    // Retain combined area information in case we contain a float
    // and nothing else.
    aCombinedRect = mMetrics.mOverflowArea;
    aCombinedRect.x += x;
    aCombinedRect.y += y;
  }
  else {
    // See if the frame fit. If its the first frame then it always
    // fits.
    if (aForceFit || (y + mMetrics.height <= mSpace.YMost())) 
    {
      // Calculate the actual x-offset and left and right margin
      nsBlockHorizontalAlign  align;
      
      align.mXOffset = x;
      AlignBlockHorizontally(mMetrics.width, align);
      x = align.mXOffset;
      mMargin.left = align.mLeftMargin;
      mMargin.right = align.mRightMargin;

      // Update the in-flow bounds rectangle
      aInFlowBounds.SetRect(x, y,
                            mMetrics.width,
                            mMetrics.height);

      // Apply CSS relative positioning to update x,y coordinates
      const nsStyleDisplay* styleDisp = mFrame->GetStyleDisplay();
      if (NS_STYLE_POSITION_RELATIVE == styleDisp->mPosition) {
        x += aComputedOffsets.left;
        y += aComputedOffsets.top;
      }

      // Compute combined-rect in callers coordinate system. The value
      // returned in the reflow metrics is relative to the child
      // frame.  This includes relative positioning and the result should
      // be used only for painting and for 'overflow' handling.
      aCombinedRect.x = mMetrics.mOverflowArea.x + x;
      aCombinedRect.y = mMetrics.mOverflowArea.y + y;
      aCombinedRect.width = mMetrics.mOverflowArea.width;
      aCombinedRect.height = mMetrics.mOverflowArea.height;

      // Now place the frame and complete the reflow process
      nsContainerFrame::FinishReflowChild(mFrame, mPresContext, &aReflowState, mMetrics, x, y, 0);
    }
    else {
      // Send the DidReflow() notification, but don't bother placing
      // the frame
      mFrame->DidReflow(mPresContext, &aReflowState, NS_FRAME_REFLOW_FINISHED);
      fits = PR_FALSE;
    }
  }

  return fits;
}
