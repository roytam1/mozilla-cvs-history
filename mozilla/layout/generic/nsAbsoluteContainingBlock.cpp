/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#include "nsCOMPtr.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsContainerFrame.h"
#include "nsIViewManager.h"
#include "nsLayoutAtoms.h"
#include "nsIPresShell.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"

#ifdef DEBUG
#include "nsBlockFrame.h"
#endif


nsresult
nsAbsoluteContainingBlock::FirstChild(const nsIFrame* aDelegatingFrame,
                                      nsIAtom*        aListName,
                                      nsIFrame**      aFirstChild) const
{
  NS_PRECONDITION(GetChildListName() == aListName, "unexpected child list name");
  *aFirstChild = mAbsoluteFrames.FirstChild();
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::SetInitialChildList(nsIFrame*       aDelegatingFrame,
                                               nsPresContext* aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  NS_PRECONDITION(GetChildListName() == aListName, "unexpected child list name");
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mAbsoluteFrames.SetFrames(aChildList);
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::AppendFrames(nsIFrame*       aDelegatingFrame,
                                        nsPresContext* aPresContext,
                                        nsIPresShell&   aPresShell,
                                        nsIAtom*        aListName,
                                        nsIFrame*       aFrameList)
{
  nsresult  rv = NS_OK;

  // Append the frames to our list of absolutely positioned frames
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.AppendFrames(nsnull, aFrameList);

  aDelegatingFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  aPresShell.FrameNeedsReflow(aDelegatingFrame, PR_FALSE);

  return rv;
}

nsresult
nsAbsoluteContainingBlock::InsertFrames(nsIFrame*       aDelegatingFrame,
                                        nsPresContext* aPresContext,
                                        nsIPresShell&   aPresShell,
                                        nsIAtom*        aListName,
                                        nsIFrame*       aPrevFrame,
                                        nsIFrame*       aFrameList)
{
  nsresult  rv = NS_OK;

  // Insert the new frames
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

  aDelegatingFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
  aPresShell.FrameNeedsReflow(aDelegatingFrame, PR_FALSE);

  return rv;
}

nsresult
nsAbsoluteContainingBlock::RemoveFrame(nsIFrame*       aDelegatingFrame,
                                       nsPresContext* aPresContext,
                                       nsIPresShell&   aPresShell,
                                       nsIAtom*        aListName,
                                       nsIFrame*       aOldFrame)
{
  PRBool result = mAbsoluteFrames.DestroyFrame(aPresContext, aOldFrame);
  NS_ASSERTION(result, "didn't find frame to delete");
  // Because positioned frames aren't part of a flow, there's no additional
  // work to do, e.g. reflowing sibling frames. And because positioned frames
  // have a view, we don't need to repaint
  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsAbsoluteContainingBlock::ReplaceFrame(nsIFrame*       aDelegatingFrame,
                                        nsPresContext* aPresContext,
                                        nsIPresShell&   aPresShell,
                                        nsIAtom*        aListName,
                                        nsIFrame*       aOldFrame,
                                        nsIFrame*       aNewFrame)
{
  PRBool result = mAbsoluteFrames.ReplaceFrame(aPresContext, aDelegatingFrame,
                                               aOldFrame, aNewFrame, PR_TRUE);
  NS_ASSERTION(result, "Problems replacing a frame");
  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsAbsoluteContainingBlock::Reflow(nsIFrame*                aDelegatingFrame,
                                  nsPresContext*          aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nscoord                  aContainingBlockWidth,
                                  nscoord                  aContainingBlockHeight,
                                  nsRect*                  aChildBounds)
{
  // Initialize OUT parameter
  if (aChildBounds)
    aChildBounds->SetRect(0, 0, 0, 0);

  PRBool parentDirty = aDelegatingFrame->GetStateBits() & NS_FRAME_IS_DIRTY;

  nsIFrame* kidFrame;
  for (kidFrame = mAbsoluteFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
    if (parentDirty || kidFrame->GetStateBits() & NS_FRAME_IS_DIRTY) {
      // Reflow the frame
      nsReflowStatus  kidStatus;
      ReflowAbsoluteFrame(aDelegatingFrame, aPresContext, aReflowState, aContainingBlockWidth,
                          aContainingBlockHeight, kidFrame, kidStatus);

    }
    if (aChildBounds) {
      // Add in the child's bounds
      nsRect  kidBounds = kidFrame->GetRect();
      aChildBounds->UnionRect(*aChildBounds, kidBounds);

      // If the frame has visible overflow, then take it into account, too.
      if (kidFrame->GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN) {
        // Get the property
        nsRect* overflowArea =  kidFrame->GetOverflowAreaProperty();

        if (overflowArea) {
          // The overflow area is in the child's coordinate space, so translate
          // it into the parent's coordinate space
          nsRect  rect(*overflowArea);

          rect.MoveBy(kidBounds.x, kidBounds.y);
          aChildBounds->UnionRect(*aChildBounds, rect);
        }
      }
    }
  }
  return NS_OK;
}

PRBool
nsAbsoluteContainingBlock::ReflowingAbsolutesOnly(nsIFrame* aDelegatingFrame,
                                                  const nsHTMLReflowState& aReflowState)
{
  // See if the reflow command is targeted at us.
  nsReflowPath *path = aReflowState.path;
  nsHTMLReflowCommand *command = path->mReflowCommand;

  if (command) {
    // It's targeted at us. See if it's for the positioned child frames
    nsCOMPtr<nsIAtom> listName;
    command->GetChildListName(*getter_AddRefs(listName));

    if (GetChildListName() != listName) {
      // A reflow command is targeted directly at this block.
      // The block will have to do a proper reflow.
      return PR_FALSE;
    }
  }

  nsReflowPath::iterator iter = path->FirstChild();
  nsReflowPath::iterator end = path->EndChildren();

  if (iter != end && mAbsoluteFrames.NotEmpty()) {
    for ( ; iter != end; ++iter) {
      // See if it's one of our absolutely positioned child frames
      if (!mAbsoluteFrames.ContainsFrame(*iter)) {
        // At least one of the frames along the reflow path wasn't
        // absolutely positioned, so we'll need to deal with it in
        // normal block reflow.
        return PR_FALSE;
      }
    }
  }

  return PR_TRUE;
}

static PRBool IsFixedBorderSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Enumerated
    || aUnit == eStyleUnit_Null;
}
static PRBool IsFixedPaddingSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Null;
}
static PRBool IsFixedMarginSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Null;
}
static PRBool IsFixedMaxSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Null || aUnit == eStyleUnit_Coord;
}

void
nsAbsoluteContainingBlock::DirtyFramesDependingOnContainer(PRBool aWidthChanged,
                                                           PRBool aHeightChanged)
{
  for (nsIFrame* f = mAbsoluteFrames.FirstChild(); f; f = f->GetNextSibling()) {
    const nsStylePosition* pos = f->GetStylePosition();
    // See if f's position might have changed because it depends on a
    // placeholder's position
    if (pos->mOffset.GetTopUnit() == eStyleUnit_Auto ||
        (f->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL
         ? pos->mOffset.GetRightUnit() : pos->mOffset.GetLeftUnit()) == eStyleUnit_Auto) {
      // Note that in CSS2.1, 'bottom:auto' can never actually induce a dependence
      // on the position of the placeholder
      f->AddStateBits(NS_FRAME_IS_DIRTY);
      continue;
    }
    if (!aWidthChanged && !aHeightChanged) {
      // skip getting style data
      continue;
    }
    const nsStyleBorder* border = f->GetStyleBorder();
    const nsStylePadding* padding = f->GetStylePadding();
    const nsStyleMargin* margin = f->GetStyleMargin();
    if (aWidthChanged) {
      // See if f's width might have changed.
      // If border-left, border-right, padding-left, padding-right,
      // width, min-width, and max-width are all lengths, 'none', or enumerated,
      // then our frame width does not depend on the parent width.
      if (pos->mWidth.GetUnit() != eStyleUnit_Coord ||
          pos->mMinWidth.GetUnit() != eStyleUnit_Coord ||
          !IsFixedMaxSize(pos->mMaxWidth.GetUnit()) ||
          !IsFixedBorderSize(border->mBorder.GetLeftUnit()) ||
          !IsFixedBorderSize(border->mBorder.GetRightUnit()) ||
          !IsFixedPaddingSize(padding->mPadding.GetLeftUnit()) ||
          !IsFixedPaddingSize(padding->mPadding.GetRightUnit())) {
        f->AddStateBits(NS_FRAME_IS_DIRTY);
        continue;
      }

      // See if f's position might have changed. If we're RTL then the
      // rules are slightly different. We'll assume percentage or auto
      // margins will always induce a dependency on the size
      if (!IsFixedMarginSize(margin->mMargin.GetLeftUnit()) ||
          !IsFixedMarginSize(margin->mMargin.GetRightUnit())) {
        f->AddStateBits(NS_FRAME_IS_DIRTY);
        continue;
      }
      if (f->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
        // Note that even if 'left' is a length, our position can
        // still depend on the containing block width, because if
        // 'right' is also a length we will discard 'left' and be
        // positioned relative to the containing block right edge.
        // 'left' length and 'right' auto is the only combination
        // we can be sure of.
        if (pos->mOffset.GetLeftUnit() != eStyleUnit_Coord ||
            pos->mOffset.GetRightUnit() != eStyleUnit_Auto) {
          f->AddStateBits(NS_FRAME_IS_DIRTY);
          continue;
        }
      } else {
        if (pos->mOffset.GetLeftUnit() != eStyleUnit_Coord) {
          f->AddStateBits(NS_FRAME_IS_DIRTY);
          continue;
        }
      }
    }
    if (aHeightChanged) {
      // See if f's height might have changed.
      // If border-top, border-bottom, padding-top, padding-bottom,
      // min-height, and max-height are all lengths or 'none',
      // and height is a length or height and bottom are auto and top is not auto,
      // then our frame height does not depend on the parent height.
      if (!(pos->mHeight.GetUnit() == eStyleUnit_Coord ||
            (pos->mHeight.GetUnit() == eStyleUnit_Auto &&
             pos->mOffset.GetBottomUnit() == eStyleUnit_Auto &&
             pos->mOffset.GetTopUnit() != eStyleUnit_Auto)) ||
          pos->mMinHeight.GetUnit() != eStyleUnit_Coord ||
          !IsFixedMaxSize(pos->mMaxHeight.GetUnit()) ||
          !IsFixedBorderSize(border->mBorder.GetTopUnit()) ||
          !IsFixedBorderSize(border->mBorder.GetBottomUnit()) ||
          !IsFixedPaddingSize(padding->mPadding.GetTopUnit()) ||
          !IsFixedPaddingSize(padding->mPadding.GetBottomUnit())) { 
        f->AddStateBits(NS_FRAME_IS_DIRTY);
        continue;
      }
      
      // See if f's position might have changed.
      if (!IsFixedMarginSize(margin->mMargin.GetTopUnit()) ||
          !IsFixedMarginSize(margin->mMargin.GetBottomUnit())) {
        f->AddStateBits(NS_FRAME_IS_DIRTY);
        continue;
      }
      if (pos->mOffset.GetTopUnit() != eStyleUnit_Coord) {
        f->AddStateBits(NS_FRAME_IS_DIRTY);
        continue;
      }
    }
  }
}

void
nsAbsoluteContainingBlock::DestroyFrames(nsIFrame*       aDelegatingFrame,
                                         nsPresContext* aPresContext)
{
  mAbsoluteFrames.DestroyFrames(aPresContext);
}

// XXX Optimize the case where it's a resize reflow and the absolutely
// positioned child has the exact same size and position and skip the
// reflow...

// When bug 154892 is checked in, make sure that when 
// GetChildListName() == nsLayoutAtoms::fixedList, the height is unconstrained.
// since we don't allow replicated frames to split.

nsresult
nsAbsoluteContainingBlock::ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                                               nsPresContext*          aPresContext,
                                               const nsHTMLReflowState& aReflowState,
                                               nscoord                  aContainingBlockWidth,
                                               nscoord                  aContainingBlockHeight,
                                               nsIFrame*                aKidFrame,
                                               nsReflowStatus&          aStatus)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent);
    printf("abs pos ");
    if (nsnull != aKidFrame) {
      nsIFrameDebug*  frameDebug;
      if (NS_SUCCEEDED(CallQueryInterface(aKidFrame, &frameDebug))) {
        nsAutoString name;
        frameDebug->GetFrameName(name);
        printf("%s ", NS_LossyConvertUCS2toASCII(name).get());
      }
    }
    printf("r=%d",aReflowState.reason);

    if (aReflowState.reason == eReflowReason_Incremental) {
      nsHTMLReflowCommand *command = aReflowState.path->mReflowCommand;

      if (command) {
        // We're the target.
        nsReflowType type;
        command->GetType(type);
        printf("(%d)", type);      
      }
    }
    char width[16];
    char height[16];
    PrettyUC(aReflowState.availableWidth, width);
    PrettyUC(aReflowState.availableHeight, height);
    printf(" a=%s,%s ", width, height);
    PrettyUC(aReflowState.mComputedWidth, width);
    PrettyUC(aReflowState.mComputedHeight, height);
    printf("c=%s,%s \n", width, height);
  }
  if (nsBlockFrame::gNoisy) {
    nsBlockFrame::gNoiseIndent++;
  }
#endif // DEBUG

  nsresult  rv;
  nsMargin  border;
  // Get the border values
  if (!aReflowState.mStyleBorder->GetBorder(border)) {
    NS_NOTYETIMPLEMENTED("percentage border");
  }

  nscoord availWidth = aReflowState.mComputedWidth;
#error "This code needs to go."
  enum { NOT_SHRINK_TO_FIT, SHRINK_TO_FIT_AVAILWIDTH, SHRINK_TO_FIT_MEW };
  PRUint32 situation = NOT_SHRINK_TO_FIT;
  while (1) {
    nsHTMLReflowMetrics kidDesiredSize(nsnull);
    if (situation == NOT_SHRINK_TO_FIT) {
      // CSS2.1 10.3.7 width:auto and at least one of left/right is auto...
      const nsStylePosition* stylePosition = aKidFrame->GetStylePosition();
      if (eStyleUnit_Auto == stylePosition->mWidth.GetUnit() &&
          (eStyleUnit_Auto == stylePosition->mOffset.GetLeftUnit() ||
           eStyleUnit_Auto == stylePosition->mOffset.GetRightUnit())) {
        situation = SHRINK_TO_FIT_AVAILWIDTH;
        if (aContainingBlockWidth != -1) {
          availWidth = aContainingBlockWidth;
        } else {
          availWidth = aReflowState.mComputedWidth;
        }
        kidDesiredSize.mComputeMEW = PR_TRUE;
      }
    }

    nsSize            availSize(availWidth, NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aKidFrame,
                                     availSize, aContainingBlockWidth,
                                     aContainingBlockHeight);

    if (situation == SHRINK_TO_FIT_MEW) {
      situation = NOT_SHRINK_TO_FIT; // This is the last reflow
      kidReflowState.mComputedWidth = PR_MIN(availWidth, kidReflowState.mComputedMaxWidth);
      if (kidReflowState.mComputedWidth < kidReflowState.mComputedMinWidth) {
        kidReflowState.mComputedWidth = kidReflowState.mComputedMinWidth;
      }
    } else if (situation == SHRINK_TO_FIT_AVAILWIDTH) {
      NS_ASSERTION(availWidth != NS_UNCONSTRAINEDSIZE,
                   "shrink-to-fit: expected a constrained available width");
      PRInt32 maxWidth = availWidth -
        (kidReflowState.mComputedMargin.left + kidReflowState.mComputedBorderPadding.left +
         kidReflowState.mComputedBorderPadding.right + kidReflowState.mComputedMargin.right);
      if (NS_AUTOOFFSET != kidReflowState.mComputedOffsets.right) {
        maxWidth -= kidReflowState.mComputedOffsets.right;
      }
      if (NS_AUTOOFFSET != kidReflowState.mComputedOffsets.left) {
        maxWidth -= kidReflowState.mComputedOffsets.left;
      }
      // The following also takes care of maxWidth<0
      if (kidReflowState.mComputedMaxWidth > maxWidth) {
        kidReflowState.mComputedMaxWidth = PR_MAX(maxWidth, kidReflowState.mComputedMinWidth);
      }
    }

    // Send the WillReflow() notification and position the frame
    aKidFrame->WillReflow(aPresContext);

    // XXXldb We can simplify this if we come up with a better way to
    // position views.
    nscoord x;
    if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) {
      // Just use the current x-offset
      x = aKidFrame->GetPosition().x;
    } else {
      x = border.left + kidReflowState.mComputedOffsets.left + kidReflowState.mComputedMargin.left;
    }
    aKidFrame->SetPosition(nsPoint(x, border.top +
                                      kidReflowState.mComputedOffsets.top +
                                      kidReflowState.mComputedMargin.top));

    // Position its view, but don't bother it doing it now if we haven't
    // yet determined the left offset
    if (NS_AUTOOFFSET != kidReflowState.mComputedOffsets.left) {
      nsContainerFrame::PositionFrameView(aKidFrame);
    }

    // Do the reflow
    rv = aKidFrame->Reflow(aPresContext, kidDesiredSize, kidReflowState, aStatus);

    if (situation == SHRINK_TO_FIT_AVAILWIDTH) {
      // ...continued CSS2.1 10.3.7 width:auto and at least one of left/right is auto
      availWidth -= kidReflowState.mComputedMargin.left + kidReflowState.mComputedMargin.right;

      if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.right) {
        NS_ASSERTION(NS_AUTOOFFSET != kidReflowState.mComputedOffsets.left,
                     "Can't solve for both left and right");
        availWidth -= kidReflowState.mComputedOffsets.left;
      } else {
        NS_ASSERTION(NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left,
                     "Expected to solve for left");
        availWidth -= kidReflowState.mComputedOffsets.right;
      }
      if (availWidth < 0) {
        availWidth = 0;
      }

#error "This code needs to go!"
      // Shrink-to-fit: min(max(preferred minimum width, available width), preferred width).
      // XXX this is not completely correct - see bug 201897 comment 56/58 and bug 268499.
      if (kidDesiredSize.mMaxElementWidth > availWidth) {
        aKidFrame->DidReflow(aPresContext, &kidReflowState, NS_FRAME_REFLOW_FINISHED);
        availWidth = PR_MAX(0, kidDesiredSize.mMaxElementWidth -
                               kidReflowState.mComputedBorderPadding.left -
                               kidReflowState.mComputedBorderPadding.right);
        situation = SHRINK_TO_FIT_MEW;
        continue; // Do a second reflow constrained to MEW.
      }
    }

    // If we're solving for 'left' or 'top', then compute it now that we know the
    // width/height
    if ((NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) ||
        (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top)) {
      if (-1 == aContainingBlockWidth) {
        // Get the containing block width/height
        kidReflowState.ComputeContainingBlockRectangle(aPresContext,
                                                       &aReflowState,
                                                       aContainingBlockWidth,
                                                       aContainingBlockHeight);
      }

      if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) {
        NS_ASSERTION(NS_AUTOOFFSET != kidReflowState.mComputedOffsets.right,
                     "Can't solve for both left and right");
        kidReflowState.mComputedOffsets.left = aContainingBlockWidth -
                                               kidReflowState.mComputedOffsets.right -
                                               kidReflowState.mComputedMargin.right -
                                               kidDesiredSize.width -
                                               kidReflowState.mComputedMargin.left;
      }
      if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top) {
        kidReflowState.mComputedOffsets.top = aContainingBlockHeight -
                                              kidReflowState.mComputedOffsets.bottom -
                                              kidReflowState.mComputedMargin.bottom -
                                              kidDesiredSize.height -
                                              kidReflowState.mComputedMargin.top;
      }
    }

    // Position the child relative to our padding edge
    nsRect  rect(border.left + kidReflowState.mComputedOffsets.left + kidReflowState.mComputedMargin.left,
                 border.top + kidReflowState.mComputedOffsets.top + kidReflowState.mComputedMargin.top,
                 kidDesiredSize.width, kidDesiredSize.height);
    aKidFrame->SetRect(rect);

    // Size and position the view and set its opacity, visibility, content
    // transparency, and clip
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aKidFrame,
                                               aKidFrame->GetView(),
                                               &kidDesiredSize.mOverflowArea);
    aKidFrame->DidReflow(aPresContext, &kidReflowState, NS_FRAME_REFLOW_FINISHED);

    // If the frame has visible overflow, then store it as a property on the
    // frame. This allows us to be able to recover it without having to reflow
    // the frame
    if (aKidFrame->GetStateBits() & NS_FRAME_OUTSIDE_CHILDREN) {
      // Get the property (creating a rect struct if necessary)
      nsRect* overflowArea = aKidFrame->GetOverflowAreaProperty(PR_TRUE);

      NS_ASSERTION(overflowArea, "should have created rect");
      if (overflowArea) {
        *overflowArea = kidDesiredSize.mOverflowArea;
      }
    }
#ifdef DEBUG
    if (nsBlockFrame::gNoisy) {
      nsBlockFrame::gNoiseIndent--;
    }
    if (nsBlockFrame::gNoisyReflow) {
      nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent);
      printf("abs pos ");
      if (nsnull != aKidFrame) {
        nsIFrameDebug*  frameDebug;
        if (NS_SUCCEEDED(CallQueryInterface(aKidFrame, &frameDebug))) {
          nsAutoString name;
          frameDebug->GetFrameName(name);
          printf("%s ", NS_LossyConvertUCS2toASCII(name).get());
        }
      }
      printf("%p rect=%d,%d,%d,%d", aKidFrame, rect.x, rect.y, rect.width, rect.height);
      printf("\n");
    }
#endif

    break;
  }
  return rv;
}

#ifdef DEBUG
 void nsAbsoluteContainingBlock::PrettyUC(nscoord aSize,
                        char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    if((PRInt32)0xdeadbeef == aSize)
    {
      strcpy(aBuf, "deadbeef");
    }
    else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}
#endif
