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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsCOMPtr.h"
#include "nsBlockFrame.h"
#include "nsBlockReflowContext.h"
#include "nsBlockBandData.h"
#include "nsBulletFrame.h"
#include "nsLineBox.h"
#include "nsInlineFrame.h"
#include "nsLineLayout.h"
#include "nsPlaceholderFrame.h"
#include "nsStyleConsts.h"
#include "nsHTMLIIDs.h"
#include "nsCSSRendering.h"
#include "nsIFrameManager.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIReflowCommand.h"
#include "nsISpaceManager.h"
#include "nsIStyleContext.h"
#include "nsIView.h"
#include "nsIFontMetrics.h"
#include "nsHTMLParts.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLValue.h"
#include "nsDOMEvent.h"
#include "nsIHTMLContent.h"
#include "prprf.h"
#include "nsLayoutAtoms.h"
#include "nsITextContent.h"
#include "nsStyleChangeList.h"
#include "nsISizeOfHandler.h"
#include "nsIFocusTracker.h"
#include "nsIFrameSelection.h"
#include "nsSpaceManager.h"
#include "prenv.h"
#include "plstr.h"

// XXX HTML:P's that are empty yet have style indicating they should
// clear floaters - we need to ignore the clear behavior.

#ifdef DEBUG

static PRBool gLamePaintMetrics;
static PRBool gLameReflowMetrics;
static PRBool gNoisy;
static PRBool gNoisyDamageRepair;
static PRBool gNoisyMaxElementSize;
static PRBool gNoisyReflow;
static PRBool gReallyNoisyReflow;
static PRBool gNoisySpaceManager;
static PRBool gVerifyLines;
static PRBool gDisableResizeOpt;
static PRBool gListTextRuns;

struct BlockDebugFlags {
  const char* name;
  PRBool* on;
};

static BlockDebugFlags gFlags[] = {
  { "reflow", &gNoisyReflow },
  { "really-noisy-reflow", &gReallyNoisyReflow },
  { "max-element-size", &gNoisyMaxElementSize },
  { "space-manager", &gNoisySpaceManager },
  { "verify-lines", &gVerifyLines },
  { "damage-repair", &gNoisyDamageRepair },
  { "lame-paint-metrics", &gLamePaintMetrics },
  { "lame-reflow-metrics", &gLameReflowMetrics },
  { "disable-resize-opt", &gDisableResizeOpt },
  { "list-text-runs", &gListTextRuns },
};
#define NUM_DEBUG_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))

static void
ShowDebugFlags()
{
  printf("Here are the available GECKO_BLOCK_DEBUG_FLAGS:\n");
  BlockDebugFlags* bdf = gFlags;
  BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
  for (; bdf < end; bdf++) {
    printf("  %s\n", bdf->name);
  }
  printf("Note: GECKO_BLOCK_DEBUG_FLAGS is a comma seperated list of flag\n");
  printf("names (no whitespace)\n");
}

static void
InitDebugFlags()
{
  static PRBool firstTime = PR_TRUE;
  if (firstTime) {
    firstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_BLOCK_DEBUG_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;
      for (;;) {
        char* cm = PL_strchr(flags, ',');
        if (cm) *cm = '\0';

        PRBool found = PR_FALSE;
        BlockDebugFlags* bdf = gFlags;
        BlockDebugFlags* end = gFlags + NUM_DEBUG_FLAGS;
        for (; bdf < end; bdf++) {
          if (PL_strcasecmp(bdf->name, flags) == 0) {
            *(bdf->on) = PR_TRUE;
            printf("nsBlockFrame: setting %s debug flag on\n", bdf->name);
            gNoisy = PR_TRUE;
            found = PR_TRUE;
            break;
          }
        }
        if (!found) {
          error = PR_TRUE;
        }

        if (!cm) break;
        *cm = ',';
        flags = cm + 1;
      }
      if (error) {
        ShowDebugFlags();
      }
    }
  }
}

#undef NOISY_FIRST_LINE
#undef  REALLY_NOISY_FIRST_LINE
#undef NOISY_FIRST_LETTER
#undef NOISY_MAX_ELEMENT_SIZE
#undef NOISY_FLOATER_CLEARING
#undef NOISY_FINAL_SIZE
#undef NOISY_REMOVE_FRAME
#undef NOISY_COMBINED_AREA
#undef NOISY_VERTICAL_MARGINS
#undef NOISY_REFLOW_REASON
#undef REFLOW_STATUS_COVERAGE
#undef NOISY_SPACEMANAGER

#endif

//----------------------------------------------------------------------

// Debugging support code

#ifdef DEBUG
static PRInt32 gNoiseIndent;
static const char* kReflowCommandType[] = {
  "ContentChanged",
  "StyleChanged",
  "PullupReflow",
  "PushReflow",
  "CheckPullupReflow",
  "ReflowDirty",
  "UserDefined",
};
#endif

#ifdef REALLY_NOISY_FIRST_LINE
static void
DumpStyleGeneaology(nsIFrame* aFrame, const char* gap)
{
  fputs(gap, stdout);
  nsFrame::ListTag(stdout, aFrame);
  printf(": ");
  nsIStyleContext* sc;
  aFrame->GetStyleContext(&sc);
  while (nsnull != sc) {
    nsIStyleContext* psc;
    printf("%p ", sc);
    psc = sc->GetParent();
    NS_RELEASE(sc);
    sc = psc;
  }
  printf("\n");
}
#endif

#ifdef REFLOW_STATUS_COVERAGE
static void
RecordReflowStatus(PRBool aChildIsBlock, nsReflowStatus aFrameReflowStatus)
{
  static PRUint32 record[2];

  // 0: child-is-block
  // 1: child-is-inline
  PRIntn index = 0;
  if (!aChildIsBlock) index |= 1;

  // Compute new status
  PRUint32 newS = record[index];
  if (NS_INLINE_IS_BREAK(aFrameReflowStatus)) {
    if (NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
      newS |= 1;
    }
    else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
      newS |= 2;
    }
    else {
      newS |= 4;
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(aFrameReflowStatus)) {
    newS |= 8;
  }
  else {
    newS |= 16;
  }

  // Log updates to the status that yield different values
  if (record[index] != newS) {
    record[index] = newS;
    printf("record(%d): %02x %02x\n", index, record[0], record[1]);
  }
}
#endif

//----------------------------------------------------------------------

inline void CombineRects(const nsRect& r1, nsRect& r2)
{
  nscoord xa = r2.x;
  nscoord ya = r2.y;
  nscoord xb = xa + r2.width;
  nscoord yb = ya + r2.height;
  nscoord x = r1.x;
  nscoord y = r1.y;
  nscoord xmost = x + r1.width;
  nscoord ymost = y + r1.height;
  if (x < xa) {
    xa = x;
  }
  if (xmost > xb) {
    xb = xmost;
  }
  if (y < ya) {
    ya = y;
  }
  if (ymost > yb) {
    yb = ymost;
  }
  r2.x = xa;
  r2.y = ya;
  r2.width = xb - xa;
  r2.height = yb - ya;
}

//----------------------------------------------------------------------

class nsBlockReflowState {
public:
  nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                     nsIPresContext* aPresContext,
                     nsBlockFrame* aFrame,
                     const nsHTMLReflowMetrics& aMetrics,
                     PRBool aBlockMarginRoot);

  ~nsBlockReflowState();

  /**
   * Update our state when aLine is skipped over during incremental
   * reflow.
   */
  void RecoverStateFrom(nsLineBox* aLine, PRBool aPrevLineWasClean);

  /**
   * Get the available reflow space for the current y coordinate. The
   * available space is relative to our coordinate system (0,0) is our
   * upper left corner.
   */
  void GetAvailableSpace() {
#ifdef DEBUG
    // Verify that the caller setup the coordinate system properly
    nscoord wx, wy;
    mSpaceManager->GetTranslation(wx, wy);
    NS_ASSERTION((wx == mSpaceManagerX) && (wy == mSpaceManagerY),
                 "bad coord system");
#endif
    mBand.GetAvailableSpace(mY - BorderPadding().top, mAvailSpaceRect);

#ifdef DEBUG
    if (gNoisyReflow) {
      nsFrame::IndentBy(stdout, gNoiseIndent);
      printf("GetAvailableSpace: band=%d,%d,%d,%d count=%d\n",
             mAvailSpaceRect.x, mAvailSpaceRect.y,
             mAvailSpaceRect.width, mAvailSpaceRect.height,
             mBand.GetTrapezoidCount());
    }
#endif
  }

  void GetAvailableSpace(nscoord aY) {
#ifdef DEBUG
    // Verify that the caller setup the coordinate system properly
    nscoord wx, wy;
    mSpaceManager->GetTranslation(wx, wy);
    NS_ASSERTION((wx == mSpaceManagerX) && (wy == mSpaceManagerY),
                 "bad coord system");
#endif
    mBand.GetAvailableSpace(aY - BorderPadding().top, mAvailSpaceRect);

#ifdef DEBUG
    if (gNoisyReflow) {
      nsFrame::IndentBy(stdout, gNoiseIndent);
      printf("GetAvailableSpace: band=%d,%d,%d,%d count=%d\n",
             mAvailSpaceRect.x, mAvailSpaceRect.y,
             mAvailSpaceRect.width, mAvailSpaceRect.height,
             mBand.GetTrapezoidCount());
    }
#endif
  }

  void InitFloater(nsLineLayout& aLineLayout,
                   nsPlaceholderFrame* aPlaceholderFrame);

  void AddFloater(nsLineLayout& aLineLayout,
                  nsPlaceholderFrame* aPlaceholderFrame,
                  PRBool aInitialReflow);

  PRBool CanPlaceFloater(const nsRect& aFloaterRect, PRUint8 aFloats);

  void PlaceFloater(nsFloaterCache* aFloaterCache,
                    PRBool* aIsLeftFloater);

  void PlaceBelowCurrentLineFloaters(nsFloaterCacheList& aFloaters);

  void ClearFloaters(nscoord aY, PRUint8 aBreakType);

  PRBool ClearPastFloaters(PRUint8 aBreakType);

  PRBool IsLeftMostChild(nsIFrame* aFrame);

  PRBool IsAdjacentWithTop() const {
    return mY == mReflowState.mComputedBorderPadding.top;
  }

  const nsMargin& BorderPadding() const {
    return mReflowState.mComputedBorderPadding;
  }

  void UpdateMaxElementSize(const nsSize& aMaxElementSize) {
#ifdef NOISY_MAX_ELEMENT_SIZE
    nsSize oldSize = mMaxElementSize;
#endif
    if (aMaxElementSize.width > mMaxElementSize.width) {
      mMaxElementSize.width = aMaxElementSize.width;
    }
    if (aMaxElementSize.height > mMaxElementSize.height) {
      mMaxElementSize.height = aMaxElementSize.height;
    }
#ifdef NOISY_MAX_ELEMENT_SIZE
    if ((mMaxElementSize.width != oldSize.width) ||
        (mMaxElementSize.height != oldSize.height)) {
      nsFrame::IndentBy(stdout, mBlock->GetDepth());
      if (NS_UNCONSTRAINEDSIZE == mReflowState.availableWidth) {
        printf("PASS1 ");
      }
      nsFrame::ListTag(stdout, mBlock);
      printf(": old max-element-size=%d,%d new=%d,%d\n",
             oldSize.width, oldSize.height,
             mMaxElementSize.width, mMaxElementSize.height);
    }
#endif
  }

  void UpdateMaximumWidth(nscoord aMaximumWidth) {
    if (aMaximumWidth > mMaximumWidth) {
      mMaximumWidth = aMaximumWidth;
    }
  }

  void RecoverVerticalMargins(nsLineBox* aLine,
                              PRBool aApplyTopMargin,
                              nscoord* aTopMarginResult,
                              nscoord* aBottomMarginResult);

  void ComputeBlockAvailSpace(nsIFrame* aFrame,
                              nsSplittableType aSplitType,
                              const nsStyleDisplay* aDisplay,
                              nsRect& aResult);

  void RecoverStateFrom(nsLineBox* aLine,
                        PRBool aApplyTopMargin,
                        nscoord aDeltaY,
                        nsRect* aDamageRect);

  void AdvanceToNextLine() {
    mLineNumber++;
  }

  PRBool IsImpactedByFloater() {
    return mBand.GetFloaterCount();
  }

  nsLineBox* NewLineBox(nsIFrame* aFrame, PRInt32 aCount, PRBool aIsBlock);

  void FreeLineBox(nsLineBox* aLine);

  void StoreMaxElementSize(nsIFrame* aFloater, const nsSize& aSize) {
    mBand.StoreMaxElementSize(mPresContext, aFloater, aSize);
  }

  //----------------------------------------

  // This state is the "global" state computed once for the reflow of
  // the block.

  // The block frame that is using this object
  nsBlockFrame* mBlock;

  nsIPresContext* mPresContext;

  const nsHTMLReflowState& mReflowState;

  nsISpaceManager* mSpaceManager;

  // The coordinates within the spacemanager where the block is being
  // placed <b>after</b> taking into account the blocks border and
  // padding. This, therefore, represents the inner "content area" (in
  // spacemanager coordinates) where child frames will be placed,
  // including child blocks and floaters.
  nscoord mSpaceManagerX, mSpaceManagerY;

  // XXX get rid of this
  nsReflowStatus mReflowStatus;

  nscoord mBottomEdge;

  PRPackedBool mUnconstrainedWidth;
  PRPackedBool mUnconstrainedHeight;
  PRPackedBool mShrinkWrapWidth;
  PRPackedBool mNeedResizeReflow;

  // The content area to reflow child frames within. The x/y
  // coordinates are known to be mBorderPadding.left and
  // mBorderPadding.top. The width/height may be NS_UNCONSTRAINEDSIZE
  // if the container reflowing this frame has given the frame an
  // unconstrained area.
  nsSize mContentArea;

  // Our wrapping behavior
  PRPackedBool mNoWrap;

  // Is this frame a root for top/bottom margin collapsing?
  PRPackedBool mIsTopMarginRoot, mIsBottomMarginRoot;

  // See ShouldApplyTopMargin
  PRPackedBool mApplyTopMargin;

  //----------------------------------------

  // This state is "running" state updated by the reflow of each line
  // in the block. This same state is "recovered" when a line is not
  // dirty and is passed over during incremental reflow.

  // The current line being reflowed
  nsLineBox* mCurrentLine;

  // The previous line just reflowed
  nsLineBox* mPrevLine;

  // The current Y coordinate in the block
  nscoord mY;

  // The available space within the current band.
  nsRect mAvailSpaceRect;

  // The maximum x-most of each line
  nscoord mKidXMost;

  // The combined area of all floaters placed so far
  nsRect mFloaterCombinedArea;

  // For unconstained-width reflow, we keep the right floaters
  // combined area stored seperately.
  PRBool mHaveRightFloaters;
  nsRect mRightFloaterCombinedArea;

  nsFloaterCacheFreeList mFloaterCacheFreeList;

  // Previous child. This is used when pulling up a frame to update
  // the sibling list.
  nsIFrame* mPrevChild;

  // The next immediate child frame that is the target of an
  // incremental reflow command. Once that child has been reflowed we
  // null this slot out.
  nsIFrame* mNextRCFrame;

  // The previous child frames collapsed bottom margin value.
  nscoord mPrevBottomMargin;

  // The current next-in-flow for the block. When lines are pulled
  // from a next-in-flow, this is used to know which next-in-flow to
  // pull from. When a next-in-flow is emptied of lines, we advance
  // this to the next next-in-flow.
  nsBlockFrame* mNextInFlow;

  // The current band data for the current Y coordinate
  nsBlockBandData mBand;

  // List of free nsLineBox's
  nsLineBox* mFreeLineList;

  //----------------------------------------

  // Temporary line-reflow state. This state is used during the reflow
  // of a given line, but doesn't have meaning before or after.

  // The list of floaters that are "current-line" floaters. These are
  // added to the line after the line has been reflowed, to keep the
  // list fiddling from being N^2.
  nsFloaterCacheFreeList mCurrentLineFloaters;

  // The list of floaters which are "below current-line"
  // floaters. These are reflowed/placed after the line is reflowed
  // and placed. Again, this is done to keep the list fiddling from
  // being N^2.
  nsFloaterCacheFreeList mBelowCurrentLineFloaters;

  PRPackedBool mComputeMaxElementSize;
  PRPackedBool mComputeMaximumWidth;

  nsSize mMaxElementSize;
  nscoord mMaximumWidth;

  nscoord mMinLineHeight;

  PRInt32 mLineNumber;
};

// XXX This is vile. Make it go away
void
nsLineLayout::InitFloater(nsPlaceholderFrame* aFrame)
{
  mBlockRS->InitFloater(*this, aFrame);
}
void
nsLineLayout::AddFloater(nsPlaceholderFrame* aFrame)
{
  mBlockRS->AddFloater(*this, aFrame, PR_FALSE);
}

//----------------------------------------------------------------------

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsIPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       PRBool aBlockMarginRoot)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mIsTopMarginRoot(PR_FALSE),
    mIsBottomMarginRoot(PR_FALSE),
    mApplyTopMargin(PR_FALSE),
    mNextRCFrame(nsnull),
    mPrevBottomMargin(0),
    mFreeLineList(nsnull),
    mLineNumber(0),
    mNeedResizeReflow(PR_FALSE)
{
  if (aBlockMarginRoot) {
    mIsTopMarginRoot = PR_TRUE;
    mIsBottomMarginRoot = PR_TRUE;
  }
  if (mIsTopMarginRoot) {
    mApplyTopMargin = PR_TRUE;
  }
  
  mSpaceManager = aReflowState.mSpaceManager;

  // Translate into our content area and then save the 
  // coordinate system origin for later.
  const nsMargin& borderPadding = BorderPadding();
  mSpaceManager->Translate(borderPadding.left, borderPadding.top);
  mSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mBlock->GetNextInFlow((nsIFrame**)&mNextInFlow);
  mKidXMost = 0;

  // Compute content area width (the content area is inside the border
  // and padding)
  mUnconstrainedWidth = PR_FALSE;
  mShrinkWrapWidth = PR_FALSE;
  if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedWidth) {
    mContentArea.width = aReflowState.mComputedWidth;
  }
  else {
    if (NS_UNCONSTRAINEDSIZE == aReflowState.availableWidth) {
      mContentArea.width = NS_UNCONSTRAINEDSIZE;
      mUnconstrainedWidth = PR_TRUE;
    }
    else if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
      // Choose a width based on the content (shrink wrap width) up
      // to the maximum width
      mContentArea.width = aReflowState.mComputedMaxWidth;
      mShrinkWrapWidth = PR_TRUE;
    }
    else {
      nscoord lr = borderPadding.left + borderPadding.right;
      mContentArea.width = aReflowState.availableWidth - lr;
    }
  }
  mHaveRightFloaters = PR_FALSE;

  // Compute content area height. Unlike the width, if we have a
  // specified style height we ignore it since extra content is
  // managed by the "overflow" property. When we don't have a
  // specified style height then we may end up limiting our height if
  // the availableHeight is constrained (this situation occurs when we
  // are paginated).
  mUnconstrainedHeight = PR_FALSE;
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    // We are in a paginated situation. The bottom edge is just inside
    // the bottom border and padding. The content area height doesn't
    // include either border or padding edge.
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = mBottomEdge - borderPadding.top;
  }
  else {
    // When we are not in a paginated situation then we always use
    // an constrained height.
    mUnconstrainedHeight = PR_TRUE;
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }

  mY = borderPadding.top;
  mBand.Init(mSpaceManager, mContentArea);

  mPrevChild = nsnull;
  mCurrentLine = nsnull;
  mPrevLine = nsnull;

  const nsStyleText* styleText;
  mBlock->GetStyleData(eStyleStruct_Text,
                       (const nsStyleStruct*&) styleText);
  switch (styleText->mWhiteSpace) {
  case NS_STYLE_WHITESPACE_PRE:
  case NS_STYLE_WHITESPACE_NOWRAP:
    mNoWrap = PR_TRUE;
    break;
  default:
    mNoWrap = PR_FALSE;
    break;
  }

  mComputeMaxElementSize = nsnull != aMetrics.maxElementSize;
  mMaxElementSize.SizeTo(0, 0);
  mComputeMaximumWidth = NS_REFLOW_CALC_MAX_WIDTH == (aMetrics.mFlags & NS_REFLOW_CALC_MAX_WIDTH);
  mMaximumWidth = 0;

  if (0 != borderPadding.top) {
    mIsTopMarginRoot = PR_TRUE;
  }
  if (0 != borderPadding.bottom) {
    mIsBottomMarginRoot = PR_TRUE;
  }

  mMinLineHeight = nsHTMLReflowState::CalcLineHeight(mPresContext,
                                                     aReflowState.rendContext,
                                                     aReflowState.frame);
}

nsBlockReflowState::~nsBlockReflowState()
{
  // Restore the coordinate system
  const nsMargin& borderPadding = BorderPadding();
  mSpaceManager->Translate(-borderPadding.left, -borderPadding.top);

  // Release any line box's that are laying around
  nsLineBox* line = mFreeLineList;
  while (line) {
    nsLineBox* next = line->mNext;
    delete line;
    line = next;
  }
}

nsLineBox*
nsBlockReflowState::NewLineBox(nsIFrame* aFrame,
                               PRInt32 aCount,
                               PRBool aIsBlock)
{
  nsLineBox* newLine;
  if (mFreeLineList) {
    newLine = mFreeLineList;
    mFreeLineList = newLine->mNext;
    newLine->Reset(aFrame, aCount, aIsBlock);
  }
  else {
    newLine = new nsLineBox(aFrame, aCount, aIsBlock);
  }
  return newLine;
}

void
nsBlockReflowState::FreeLineBox(nsLineBox* aLine)
{
  if (aLine) {
    aLine->mNext = mFreeLineList;
    mFreeLineList = aLine;
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
  aResult.y = mY;
  aResult.height = mUnconstrainedHeight
    ? NS_UNCONSTRAINEDSIZE
    : mBottomEdge - mY;

  const nsMargin& borderPadding = BorderPadding();

  if (NS_FRAME_SPLITTABLE_NON_RECTANGULAR == aSplitType) {
    if (mBand.GetFloaterCount()) {
      // Use the float-edge property to determine how the child block
      // will interact with the floater.
      const nsStyleSpacing* spacing;
      aFrame->GetStyleData(eStyleStruct_Spacing,
                           (const nsStyleStruct*&) spacing);
      switch (spacing->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:
          // The child block will flow around the floater. Therefore
          // give it all of the available space.
          aResult.x = borderPadding.left;
          aResult.width = mUnconstrainedWidth
            ? NS_UNCONSTRAINEDSIZE
            : mContentArea.width;
          break;

        case NS_STYLE_FLOAT_EDGE_BORDER:
        case NS_STYLE_FLOAT_EDGE_PADDING:
          {
            // The child block's border should be placed adjacent to,
            // but not overlap the floater(s).
            nsMargin m(0, 0, 0, 0);
            spacing->GetMargin(m); // XXX percentage margins
            if (NS_STYLE_FLOAT_EDGE_PADDING == spacing->mFloatEdge) {
              // Add in border too
              nsMargin b;
              spacing->GetBorder(b);
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
            if (mUnconstrainedWidth) {
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
      aResult.width = mUnconstrainedWidth
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
    saveY = mY + mPrevBottomMargin;
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
      mPrevBottomMargin = deltaY;
      mY = saveY;

      // Force margin to be applied in this circumstance
      applyTopMargin = PR_TRUE;
    }
    else {
      // Put aState.mY back to its original value since no clearing
      // happened. That way the previous blocks bottom margin will
      // be applied properly.
      mY = saveY - mPrevBottomMargin;
    }
    break;
  }
  return applyTopMargin;
}

// Recover the collapsed vertical margin values for aLine. Note that
// the values are not collapsed with aState.mPrevBottomMargin, nor are
// they collapsed with each other when the line height is zero.
void
nsBlockReflowState::RecoverVerticalMargins(nsLineBox* aLine,
                                           PRBool aApplyTopMargin,
                                           nscoord* aTopMarginResult,
                                           nscoord* aBottomMarginResult)
{
  if (aLine->IsBlock()) {
    // Update band data
    GetAvailableSpace();

    // Setup reflow state to compute the block childs top and bottom
    // margins
    nsIFrame* frame = aLine->mFirstChild;
    nsRect availSpaceRect;
    const nsStyleDisplay* display;
    frame->GetStyleData(eStyleStruct_Display,
                        (const nsStyleStruct*&) display);
    nsSplittableType splitType = NS_FRAME_NOT_SPLITTABLE;
    frame->IsSplittable(splitType);
    ComputeBlockAvailSpace(frame, splitType, display, availSpaceRect);
    nsSize availSpace(availSpaceRect.width, availSpaceRect.height);
    nsHTMLReflowState reflowState(mPresContext, mReflowState,
                                  frame, availSpace);

    // Compute collapsed top margin
    nscoord topMargin = 0;
    if (aApplyTopMargin) {
      topMargin =
        nsBlockReflowContext::ComputeCollapsedTopMargin(mPresContext,
                                                        reflowState);
    }

    // Compute collapsed bottom margin
    nscoord bottomMargin = reflowState.mComputedMargin.bottom;
    bottomMargin =
      nsBlockReflowContext::MaxMargin(bottomMargin,
                                      aLine->GetCarriedOutBottomMargin());
    *aTopMarginResult = topMargin;
    *aBottomMarginResult = bottomMargin;
  }
  else {
    // XXX_ib
    *aTopMarginResult = 0;
    *aBottomMarginResult = 0;
  }
}

void
nsBlockReflowState::RecoverStateFrom(nsLineBox* aLine,
                                     PRBool aApplyTopMargin,
                                     nscoord aDeltaY,
                                     nsRect* aDamageRect)
{
  // Make the line being recovered the current line
  mCurrentLine = aLine;

  // Update aState.mPrevChild as if we had reflowed all of the frames
  // in this line.
  mPrevChild = aLine->LastChild();

  // Recover mKidXMost and mMaxElementSize
  nscoord xmost = aLine->mBounds.XMost();
  if (xmost > mKidXMost) {
#ifdef DEBUG
    if (CRAZY_WIDTH(xmost)) {
      nsFrame::ListTag(stdout, mBlock);
      printf(": WARNING: xmost:%d\n", xmost);
    }
#endif
    mKidXMost = xmost;
  }
  if (mComputeMaxElementSize) {
    UpdateMaxElementSize(nsSize(aLine->mMaxElementWidth, aLine->mBounds.height));
  }

  // If computing the maximum width, then update mMaximumWidth
  if (mComputeMaximumWidth) {
    UpdateMaximumWidth(aLine->mMaximumWidth);
  }

  // The line may have clear before semantics.
  if (aLine->IsBlock() && aLine->HasBreak()) {
    // Clear past floaters before the block if the clear style is not none
    aApplyTopMargin = ClearPastFloaters(aLine->GetBreakType());
#ifdef NOISY_VERTICAL_MARGINS
    nsFrame::ListTag(stdout, mBlock);
    printf(": RecoverStateFrom: y=%d child ", mY);
    nsFrame::ListTag(stdout, aLine->mFirstChild);
    printf(" has clear of %d => %s, mPrevBottomMargin=%d\n", aLine->mBreakType,
           aApplyTopMargin ? "applyTopMargin" : "nope", mPrevBottomMargin);
#endif
  }

  // Recover mPrevBottomMargin and calculate the line's new Y
  // coordinate (newLineY)
  nscoord newLineY = mY;
  nsRect lineCombinedArea;
  aLine->GetCombinedArea(&lineCombinedArea);
  if (aLine->IsBlock()) {
    if ((0 == aLine->mBounds.height) && (0 == lineCombinedArea.height)) {
      if (nsBlockReflowContext::IsHTMLParagraph(aLine->mFirstChild)) {
        // Empty HTML paragraphs disappear entirely - their margins go
        // to zero. Therefore we leave mPrevBottomMargin alone.
      }
      else {
        // The line's top and bottom margin values need to be collapsed
        // with the mPrevBottomMargin to determine a new
        // mPrevBottomMargin value.
        nscoord topMargin, bottomMargin;
        RecoverVerticalMargins(aLine, aApplyTopMargin,
                               &topMargin, &bottomMargin);
        nscoord m = nsBlockReflowContext::MaxMargin(bottomMargin,
                                                    mPrevBottomMargin);
        m = nsBlockReflowContext::MaxMargin(m, topMargin);
        mPrevBottomMargin = m;
      }
    }
    else {
      // Recover the top and bottom margins for this line
      nscoord topMargin, bottomMargin;
      RecoverVerticalMargins(aLine, aApplyTopMargin,
                             &topMargin, &bottomMargin);

      // Compute the collapsed top margin value
      nscoord collapsedTopMargin =
        nsBlockReflowContext::MaxMargin(topMargin, mPrevBottomMargin);

      // The lineY is just below the collapsed top margin value. The
      // mPrevBottomMargin gets set to the bottom margin value for the
      // line.
      newLineY += collapsedTopMargin;
      mPrevBottomMargin = bottomMargin;
    }
  }
  else if (0 == aLine->GetHeight()) {
    // For empty inline lines we leave the previous bottom margin
    // alone so that it's collpased with the next line.
  }
  else {
    // For non-empty inline lines the previous margin is applied
    // before the line. Therefore apply it now and zero it out.
    newLineY += mPrevBottomMargin;
    mPrevBottomMargin = 0;
  }

  // Save away the old combined area for later
  nsRect oldCombinedArea = lineCombinedArea;

  // Slide the frames in the line by the computed delta. This also
  // updates the lines Y coordinate and the combined area's Y
  // coordinate.
  nscoord finalDeltaY = newLineY - aLine->mBounds.y;
  mBlock->SlideLine(*this, aLine, finalDeltaY);

  // Place floaters for this line into the space manager
  if (aLine->HasFloaters()) {
    // Undo border/padding translation since the nsFloaterCache's
    // coordinates are relative to the frame not relative to the
    // border/padding.
    const nsMargin& bp = BorderPadding();
    mSpaceManager->Translate(-bp.left, -bp.top);

    // Place the floaters into the space-manager again. Also slide
    // them, just like the regular frames on the line.
    nsRect r;
    nsFloaterCache* fc = aLine->GetFirstFloater();
    while (fc) {
      fc->mRegion.y += finalDeltaY;
      fc->mCombinedArea.y += finalDeltaY;
      nsIFrame* floater = fc->mPlaceholder->GetOutOfFlowFrame();
      floater->GetRect(r);
      floater->MoveTo(mPresContext, r.x, r.y + finalDeltaY);
#ifdef DEBUG
      if (gNoisyReflow || gNoisySpaceManager) {
        nscoord tx, ty;
        mSpaceManager->GetTranslation(tx, ty);
        nsFrame::IndentBy(stdout, gNoiseIndent);
        printf("RecoverState: txy=%d,%d (%d,%d) ",
               tx, ty, mSpaceManagerX, mSpaceManagerY);
        nsFrame::ListTag(stdout, floater);
        printf(" r.y=%d finalDeltaY=%d (sum=%d) region={%d,%d,%d,%d}\n",
               r.y, finalDeltaY, r.y + finalDeltaY,
               fc->mRegion.x, fc->mRegion.y,
               fc->mRegion.width, fc->mRegion.height);
      }
#endif
      mSpaceManager->AddRectRegion(floater, fc->mRegion);
      fc = fc->Next();
    }

    // And then put the translation back again
    mSpaceManager->Translate(bp.left, bp.top);
  }

  // Recover mY
  mY = aLine->mBounds.YMost();

  // Compute the damage area
  if (aDamageRect) {
    if (0 == finalDeltaY) {
      aDamageRect->Empty();
    } else {
      aLine->GetCombinedArea(&lineCombinedArea);
      aDamageRect->UnionRect(oldCombinedArea, lineCombinedArea);
    }
  }

  // It's possible that the line has clear after semantics
  if (!aLine->IsBlock() && aLine->HasBreak()) {
    PRUint8 breakType = aLine->GetBreakType();
    switch (breakType) {
    case NS_STYLE_CLEAR_LEFT:
    case NS_STYLE_CLEAR_RIGHT:
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      ClearFloaters(mY, breakType);
      break;
    }
  }
}

//----------------------------------------------------------------------

const nsIID kBlockFrameCID = NS_BLOCK_FRAME_CID;

nsresult
NS_NewBlockFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsBlockFrame* it = new (aPresShell) nsBlockFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetFlags(aFlags);
  *aNewFrame = it;
  return NS_OK;
}

nsBlockFrame::nsBlockFrame()
{
#ifdef DEBUG
  InitDebugFlags();
#endif
}

nsBlockFrame::~nsBlockFrame()
{
  nsTextRun::DeleteTextRuns(mTextRuns);
}

NS_IMETHODIMP
nsBlockFrame::Destroy(nsIPresContext* aPresContext)
{
  // Outside bullets are not in our child-list so check for them here
  // and delete them when present.
  if (HaveOutsideBullet()) {
    mBullet->Destroy(aPresContext);
    mBullet = nsnull;
  }

  mFloaters.DestroyFrames(aPresContext);

  nsLineBox::DeleteLineList(aPresContext, mLines);

  return nsBlockFrameSuper::Destroy(aPresContext);
}

NS_IMETHODIMP
nsBlockFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kBlockFrameCID)) {
    nsBlockFrame* tmp = this;
    *aInstancePtr = (void*) tmp;
    return NS_OK;
  }
  if (aIID.Equals(nsILineIterator::GetIID())) {
    nsLineIterator* it = new nsLineIterator;
    if (!it) {
      *aInstancePtr = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    const nsStyleDisplay* display;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
    nsresult rv = it->Init(mLines,
                           display->mDirection == NS_STYLE_DIRECTION_RTL);
    if (NS_FAILED(rv)) {
      delete it;
      return rv;
    }
    return it->QueryInterface(aIID, aInstancePtr);
  }
  return nsBlockFrameSuper::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
nsBlockFrame::IsSplittable(nsSplittableType& aIsSplittable) const
{
  aIsSplittable = NS_FRAME_SPLITTABLE_NON_RECTANGULAR;
  return NS_OK;
}

#ifdef DEBUG
static void
ListTextRuns(FILE* out, PRInt32 aIndent, nsTextRun* aRuns)
{
  while (nsnull != aRuns) {
    aRuns->List(out, aIndent);
    aRuns = aRuns->GetNext();
  }
}

NS_METHOD
nsBlockFrame::List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
  nsIView* view;
  GetView(aPresContext, &view);
  if (nsnull != view) {
    fprintf(out, " [view=%p]", view);
  }
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", mNextSibling);
  }

  // Output the flow linkage
  if (nsnull != mPrevInFlow) {
    fprintf(out, " prev-in-flow=%p", mPrevInFlow);
  }
  if (nsnull != mNextInFlow) {
    fprintf(out, " next-in-flow=%p", mNextInFlow);
  }

  // Output the rect and state
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  PRInt32 numInlineLines = 0;
  PRInt32 numBlockLines = 0;
  if (nsnull != mLines) {
    nsLineBox* line = mLines;
    while (nsnull != line) {
      if (line->IsBlock()) {
        numBlockLines++;
      }
      else {
        numInlineLines++;
      }
      line = line->mNext;
    }
  }
  fprintf(out, " sc=%p(i=%d,b=%d)<\n", mStyleContext, numInlineLines, numBlockLines);
  aIndent++;

  // Output the lines
  if (nsnull != mLines) {
    nsLineBox* line = mLines;
    while (nsnull != line) {
      line->List(aPresContext, out, aIndent);
      line = line->mNext;
    }
  }

  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  for (;;) {
    nsIFrame* kid;
    GetAdditionalChildListName(listIndex++, &listName);
    if (nsnull == listName) {
      break;
    }
    FirstChild(listName, &kid);
    if (nsnull != kid) {
      IndentBy(out, aIndent);
      nsAutoString tmp;
      if (nsnull != listName) {
        listName->ToString(tmp);
        fputs(tmp, out);
      }
      fputs("<\n", out);
      while (nsnull != kid) {
        nsIFrameDebug*  frameDebug;

        if (NS_SUCCEEDED(kid->QueryInterface(nsIFrameDebug::GetIID(), (void**)&frameDebug))) {
          frameDebug->List(aPresContext, out, aIndent + 1);
        }
        kid->GetNextSibling(&kid);
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
    NS_IF_RELEASE(listName);
  }

  // Output the text-runs
  if (gListTextRuns && mTextRuns) {
    IndentBy(out, aIndent);
    fputs("text-runs <\n", out);

    ListTextRuns(out, aIndent + 1, mTextRuns);

    IndentBy(out, aIndent);
    fputs(">\n", out);
  }

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Block", aResult);
}
#endif

NS_IMETHODIMP
nsBlockFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::blockFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Child frame enumeration

NS_IMETHODIMP
nsBlockFrame::FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const
{
  NS_PRECONDITION(nsnull != aFirstChild, "null OUT parameter pointer");
  if (nsnull == aListName) {
    *aFirstChild = (nsnull != mLines) ? mLines->mFirstChild : nsnull;
    return NS_OK;
  }
  else if (aListName == nsLayoutAtoms::floaterList) {
    *aFirstChild = mFloaters.FirstChild();
    return NS_OK;
  }
  else if (aListName == nsLayoutAtoms::bulletList) {
    if (HaveOutsideBullet()) {
      *aFirstChild = mBullet;
    }
    else {
      *aFirstChild = nsnull;
    }
    return NS_OK;
  }
  *aFirstChild = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsBlockFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                         nsIAtom** aListName) const
{
  NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");
  if (aIndex < 0) {
    return NS_ERROR_INVALID_ARG;
  }
  *aListName = nsnull;
  switch (aIndex) {
  case NS_BLOCK_FRAME_FLOATER_LIST_INDEX:
    *aListName = nsLayoutAtoms::floaterList;
    NS_ADDREF(*aListName);
    break;
  case NS_BLOCK_FRAME_BULLET_LIST_INDEX:
    *aListName = nsLayoutAtoms::bulletList;
    NS_ADDREF(*aListName);
    break;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::IsPercentageBase(PRBool& aBase) const
{
  aBase = PR_TRUE;
  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
// Frame structure methods

//////////////////////////////////////////////////////////////////////
// Reflow methods

NS_IMETHODIMP
nsBlockFrame::Reflow(nsIPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
#ifdef DEBUG
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": begin reflow availSize=%d,%d computedSize=%d,%d\n",
           aReflowState.availableWidth, aReflowState.availableHeight,
           aReflowState.mComputedWidth, aReflowState.mComputedHeight);
  }
  if (gNoisy) {
    gNoiseIndent++;
  }
  PRTime start;
  PRInt32 ctc;
  if (gLameReflowMetrics) {
    start = PR_Now();
    ctc = nsLineBox::GetCtorCount();
  }
#endif

  if (IsFrameTreeTooDeep(aReflowState, aMetrics)) {
#ifdef DEBUG_kipp
    {
      extern char* nsPresShell_ReflowStackPointerTop;
      char marker;
      char* newsp = (char*) &marker;
      printf("XXX: frame tree is too deep; approx stack size = %d\n",
             nsPresShell_ReflowStackPointerTop - newsp);
    }
#endif
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  // Should we create a space manager?
  nsCOMPtr<nsISpaceManager> spaceManager;
  nsISpaceManager*          oldSpaceManager = aReflowState.mSpaceManager;
  if (NS_BLOCK_SPACE_MGR & mState) {
    nsSpaceManager* rawPtr = new nsSpaceManager(this);
    if (!rawPtr) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    spaceManager = do_QueryInterface(rawPtr);

    // Set the space manager in the existing reflow state
    nsHTMLReflowState&  reflowState = (nsHTMLReflowState&)aReflowState;
    reflowState.mSpaceManager = spaceManager.get();
  }

  nsBlockReflowState state(aReflowState, aPresContext, this, aMetrics,
                           NS_BLOCK_MARGIN_ROOT & mState);

  if (eReflowReason_Resize != aReflowState.reason) {
    RenumberLists();
    ComputeTextRuns(aPresContext);
  }

  nsresult rv = NS_OK;
  PRBool isStyleChange = PR_FALSE;
  nsIFrame* target;
  switch (aReflowState.reason) {
  case eReflowReason_Initial:
#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": reflow=initial\n");
#endif
    DrainOverflowLines(aPresContext);
    rv = PrepareInitialReflow(state);
    mState &= ~NS_FRAME_FIRST_REFLOW;
    break;  

  case eReflowReason_Dirty:    
    break;

  case eReflowReason_Incremental:
    aReflowState.reflowCommand->GetTarget(target);
    if (this == target) {
      nsIReflowCommand::ReflowType type;
      aReflowState.reflowCommand->GetType(type);
#ifdef NOISY_REFLOW_REASON
      ListTag(stdout);
      printf(": reflow=incremental type=%d\n", type);
#endif
      switch (type) {
      case nsIReflowCommand::StyleChanged:
        rv = PrepareStyleChangedReflow(state);
        isStyleChange = PR_TRUE;
        break;
      case nsIReflowCommand::ReflowDirty:
        break;
      default:
        // Map any other incremental operations into full reflows
        rv = PrepareResizeReflow(state);
        break;
      }
    }
    else {
      // Get next frame in reflow command chain
      aReflowState.reflowCommand->GetNext(state.mNextRCFrame);
#ifdef NOISY_REFLOW_REASON
      ListTag(stdout);
      printf(": reflow=incremental");
      if (state.mNextRCFrame) {
        printf(" next=");
        nsFrame::ListTag(stdout, state.mNextRCFrame);
      }
      printf("\n");
#endif

      // Now do the reflow
      rv = PrepareChildIncrementalReflow(state);
    }
    break;

  case eReflowReason_Resize:
  default:
#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": reflow=resize (%d)\n", aReflowState.reason);
#endif
    DrainOverflowLines(aPresContext);
    rv = PrepareResizeReflow(state);
    break;
  }

  // Now reflow...
  rv = ReflowDirtyLines(state);
  aStatus = state.mReflowStatus;
  if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
    if (NS_STYLE_OVERFLOW_HIDDEN == aReflowState.mStyleDisplay->mOverflow) {
      aStatus = NS_FRAME_COMPLETE;
    }
    else {
#ifdef DEBUG_kipp
      ListTag(stdout); printf(": block is not complete\n");
#endif
    }
  }

  // XXX_pref get rid of this!
  BuildFloaterList();
  
  // Compute our final size
  ComputeFinalSize(aReflowState, state, aMetrics);

  if (NS_BLOCK_WRAP_SIZE & mState) {
    // When the area frame is supposed to wrap around all in-flow
    // children, make sure its big enough to include those that stick
    // outside the box.
    if (NS_FRAME_OUTSIDE_CHILDREN & mState) {
      nscoord xMost = aMetrics.mOverflowArea.XMost();
      if (xMost > aMetrics.width) {
#ifdef NOISY_FINAL_SIZE
        ListTag(stdout);
        printf(": changing desired width from %d to %d\n", aMetrics.width, xMost);
#endif
        aMetrics.width = xMost;
      }
      nscoord yMost = aMetrics.mOverflowArea.YMost();
      if (yMost > aMetrics.height) {
#ifdef NOISY_FINAL_SIZE
        ListTag(stdout);
        printf(": changing desired height from %d to %d\n", aMetrics.height, yMost);
#endif
        aMetrics.height = yMost;
      }
    }
  }

  // If we set the space manager, then restore the old space manager now that we're
  // going out of scope
  if (NS_BLOCK_SPACE_MGR & mState) {
    nsHTMLReflowState&  reflowState = (nsHTMLReflowState&)aReflowState;
    reflowState.mSpaceManager = oldSpaceManager;
  }

#if 0
#ifdef NOISY_SPACEMANAGER
  if (eReflowReason_Incremental == aReflowState.reason) {
    if (mSpaceManager) {
      ListTag(stdout);
      printf(": space-manager after reflow\n");
      mSpaceManager->List(stdout);
    }
  }
#endif
#endif
  
  // If this is an incremental reflow and we changed size, then make sure our
  // border is repainted if necessary
  if (eReflowReason_Incremental == aReflowState.reason ||
      eReflowReason_Dirty == aReflowState.reason) {
    if (isStyleChange) {
      // Lots of things could have changed so damage our entire
      // bounds
      Invalidate(aPresContext, nsRect(0, 0, mRect.width, mRect.height));

    } else {
      nsMargin  border = aReflowState.mComputedBorderPadding -
                         aReflowState.mComputedPadding;
  
      // See if our width changed
      if ((aMetrics.width != mRect.width) && (border.right > 0)) {
        nsRect  damageRect;
  
        if (aMetrics.width < mRect.width) {
          // Our new width is smaller, so we need to make sure that
          // we paint our border in its new position
          damageRect.x = aMetrics.width - border.right;
          damageRect.width = border.right;
          damageRect.y = 0;
          damageRect.height = aMetrics.height;
  
        } else {
          // Our new width is larger, so we need to erase our border in its
          // old position
          damageRect.x = mRect.width - border.right;
          damageRect.width = border.right;
          damageRect.y = 0;
          damageRect.height = mRect.height;
        }
        Invalidate(aPresContext, damageRect);
      }
  
      // See if our height changed
      if ((aMetrics.height != mRect.height) && (border.bottom > 0)) {
        nsRect  damageRect;
        
        if (aMetrics.height < mRect.height) {
          // Our new height is smaller, so we need to make sure that
          // we paint our border in its new position
          damageRect.x = 0;
          damageRect.width = aMetrics.width;
          damageRect.y = aMetrics.height - border.bottom;
          damageRect.height = border.bottom;
  
        } else {
          // Our new height is larger, so we need to erase our border in its
          // old position
          damageRect.x = 0;
          damageRect.width = mRect.width;
          damageRect.y = mRect.height - border.bottom;
          damageRect.height = border.bottom;
        }
        Invalidate(aPresContext, damageRect);
      }
    }
  }

#ifdef DEBUG
  if (gNoisy) {
    gNoiseIndent--;
  }
  if (gNoisyReflow) {
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": status=%x (%scomplete) metrics=%d,%d carriedMargin=%d",
           aStatus, NS_FRAME_IS_COMPLETE(aStatus) ? "" : "not ",
           aMetrics.width, aMetrics.height,
           aMetrics.mCarriedOutBottomMargin);
    if (mState & NS_FRAME_OUTSIDE_CHILDREN) {
      printf(" combinedArea={%d,%d,%d,%d}",
             aMetrics.mOverflowArea.x,
             aMetrics.mOverflowArea.y,
             aMetrics.mOverflowArea.width,
             aMetrics.mOverflowArea.height);
    }
    if (aMetrics.maxElementSize) {
      printf(" maxElementSize=%d,%d",
             aMetrics.maxElementSize->width,
             aMetrics.maxElementSize->height);
    }
    printf("\n");
  }

  if (gLameReflowMetrics) {
    PRTime end = PR_Now();

    PRInt32 ectc = nsLineBox::GetCtorCount();
    PRInt32 numLines = nsLineBox::ListLength(mLines);
    if (!numLines) numLines = 1;
    PRTime delta, perLineDelta, lines;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(perLineDelta, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) (%d lines; %d new lines)",
                delta, perLineDelta, numLines, ectc - ctc);
    printf("%s\n", buf);
  }
#endif
  return rv;
}

static PRBool
HaveAutoWidth(const nsHTMLReflowState& aReflowState)
{
  const nsHTMLReflowState* rs = &aReflowState;
  if (NS_UNCONSTRAINEDSIZE == rs->mComputedWidth) {
    return PR_TRUE;
  }
  const nsStylePosition* pos = rs->mStylePosition;

  for (;;) {
    if (!pos) {
      return PR_TRUE;
    }
    nsStyleUnit widthUnit = pos->mWidth.GetUnit();
    if (eStyleUnit_Auto == widthUnit) {
      return PR_TRUE;
    }
    if (eStyleUnit_Inherit != widthUnit) {
      break;
    }
    const nsHTMLReflowState* prs = (const nsHTMLReflowState*)
      rs->parentReflowState;
    if (!prs) {
      return PR_TRUE;
    }
    rs = prs;
    pos = prs->mStylePosition;
  }

  return PR_FALSE;
}
        
void
nsBlockFrame::ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                               nsBlockReflowState& aState,
                               nsHTMLReflowMetrics& aMetrics)
{
  const nsMargin& borderPadding = aState.BorderPadding();
#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": mY=%d mIsBottomMarginRoot=%s mPrevBottomMargin=%d bp=%d,%d\n",
         aState.mY, aState.mIsBottomMarginRoot ? "yes" : "no",
         aState.mPrevBottomMargin,
         borderPadding.top, borderPadding.bottom);
#endif

  // Special check for zero sized content: If our content is zero
  // sized then we collapse into nothingness.
  //
  // Consensus after discussion with a few CSS folks is that html's
  // notion of collapsing <P>'s should take precedence over non
  // auto-sided block elements. Therefore we don't honor the width,
  // height, border or padding attributes (the parent has to not apply
  // a margin for us also).
  //
  // Note that this is <b>only</b> done for html paragraphs. Its not
  // appropriate to apply it to other containers, especially XML
  // content!
  PRBool isHTMLParagraph = 0 != (mState & NS_BLOCK_IS_HTML_PARAGRAPH);
  if (isHTMLParagraph &&
      (aReflowState.mStyleDisplay->mDisplay == NS_STYLE_DISPLAY_BLOCK) &&
      (((0 == aState.mKidXMost) ||
        (0 == aState.mKidXMost - borderPadding.left)) &&
       (0 == aState.mY - borderPadding.top))) {
    // Zero out most everything
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
    aMetrics.mCarriedOutBottomMargin = 0;

    // Note: Don't zero out the max-element-sizes: they will be zero
    // if this is truly empty, otherwise they won't because of a
    // floater.
    if (nsnull != aMetrics.maxElementSize) {
      aMetrics.maxElementSize->width = aState.mMaxElementSize.width;
      aMetrics.maxElementSize->height = aState.mMaxElementSize.height;
    }
  }
  else {
    // Compute final width
    nscoord maxWidth = 0, maxHeight = 0;
    nscoord minWidth = aState.mKidXMost + borderPadding.right;
    if (!HaveAutoWidth(aReflowState)) {
      // Use style defined width
      aMetrics.width = borderPadding.left + aReflowState.mComputedWidth +
        borderPadding.right;
      // XXX quote css1 section here
      if ((0 == aReflowState.mComputedWidth) && (aMetrics.width < minWidth)) {
        aMetrics.width = minWidth;
      }

      // When style defines the width use it for the max-element-size
      // because we can't shrink any smaller.
      maxWidth = aMetrics.width;
    }
    else {
      nscoord computedWidth = minWidth;
      PRBool compact = PR_FALSE;
#if 0
      if (NS_STYLE_DISPLAY_COMPACT == aReflowState.mStyleDisplay->mDisplay) {
        // If we are display: compact AND we have no lines or we have
        // exactly one line and that line is not a block line AND that
        // line doesn't end in a BR of any sort THEN we remain a compact
        // frame.
        if ((nsnull == mLines) ||
            ((nsnull == mLines->mNext) && !mLines->IsBlock() &&
             (NS_STYLE_CLEAR_NONE == mLines->GetBreakType())
             /*XXX && (computedWidth <= aState.mCompactMarginWidth) */
              )) {
          compact = PR_TRUE;
        }
      }
#endif

      // There are two options here. We either shrink wrap around our
      // contents or we fluff out to the maximum block width. Note:
      // We always shrink wrap when given an unconstrained width.
      if ((0 == (NS_BLOCK_SHRINK_WRAP & mState)) &&
          !aState.mUnconstrainedWidth && !aState.mShrinkWrapWidth &&
          !compact) {
        // Set our width to the max width if we aren't already that
        // wide. Note that the max-width has nothing to do with our
        // contents (CSS2 section XXX)
        computedWidth = borderPadding.left + aState.mContentArea.width +
          borderPadding.right;
      }

      // See if we should compute our max element size
      if (aState.mComputeMaxElementSize) {
        // Adjust the computedWidth
        if (aState.mNoWrap) {
          // When no-wrap is true the max-element-size.width is the
          // width of the widest line plus the right border. Note that
          // aState.mKidXMost already has the left border factored in
          maxWidth = aState.mKidXMost + borderPadding.right;
        }
        else {
          // Add in border and padding dimensions to already computed
          // max-element-size values.
          maxWidth = aState.mMaxElementSize.width +
            borderPadding.left + borderPadding.right;
        }
        if (computedWidth < maxWidth) {
          computedWidth = maxWidth;
        }
      }

      // Apply min/max values
      if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxWidth) {
        nscoord computedMaxWidth = aReflowState.mComputedMaxWidth +
          borderPadding.left + borderPadding.right;
        if (computedWidth > computedMaxWidth) {
          computedWidth = computedMaxWidth;
        }
      }
      if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinWidth) {
        nscoord computedMinWidth = aReflowState.mComputedMinWidth +
          borderPadding.left + borderPadding.right;
        if (computedWidth < computedMinWidth) {
          computedWidth = computedMinWidth;
        }
      }
      aMetrics.width = computedWidth;

      // If we're shrink wrapping, then now that we know our final width we
      // need to do horizontal alignment of the inline lines and make sure
      // blocks are correctly sized and positioned. Any lines that need
      // final adjustment will have been marked as dirty
      if (aState.mShrinkWrapWidth && aState.mNeedResizeReflow) {
        // If the parent reflow state is also shrink wrap width, then
        // we don't need to do this, because it will reflow us after it
        // calculates the final width
        PRBool  parentIsShrinkWrapWidth = PR_FALSE;
        if (aReflowState.parentReflowState) {
          if (NS_SHRINKWRAPWIDTH == aReflowState.parentReflowState->mComputedWidth) {
            parentIsShrinkWrapWidth = PR_TRUE;
          }
        }

        if (!parentIsShrinkWrapWidth) {
          nsHTMLReflowState reflowState(aReflowState);
  
          reflowState.mComputedWidth = aMetrics.width - borderPadding.left -
                                       borderPadding.right;
          reflowState.reason = eReflowReason_Resize;
          reflowState.mSpaceManager->ClearRegions();
  
          nscoord oldDesiredWidth = aMetrics.width;
          nsBlockReflowState state(reflowState, aState.mPresContext, this, aMetrics,
                                   NS_BLOCK_MARGIN_ROOT & mState);
          ReflowDirtyLines(state);
          aState.mY = state.mY;
          NS_ASSERTION(oldDesiredWidth == aMetrics.width, "bad desired width");
        }
      }
    }

    if (aState.mShrinkWrapWidth) {
      PRBool  parentIsShrinkWrapWidth = PR_FALSE;
      if (aReflowState.parentReflowState) {
        if (NS_SHRINKWRAPWIDTH == aReflowState.parentReflowState->mComputedWidth) {
          parentIsShrinkWrapWidth = PR_TRUE;
        }
      }
    }

    // Compute final height
    if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedHeight) {
      // Use style defined height
      aMetrics.height = borderPadding.top + aReflowState.mComputedHeight +
        borderPadding.bottom;

      // When style defines the height use it for the max-element-size
      // because we can't shrink any smaller.
      maxHeight = aMetrics.height;

      // Don't carry out a bottom margin when our height is fixed
      // unless the bottom of the last line adjoins the bottom of our
      // content area.
      if (!aState.mIsBottomMarginRoot) {
        if (aState.mY + aState.mPrevBottomMargin != aMetrics.height) {
          aState.mPrevBottomMargin = 0;
        }
      }
    }
    else {
      nscoord autoHeight = aState.mY;

      // Shrink wrap our height around our contents.
      if (aState.mIsBottomMarginRoot) {
        // When we are a bottom-margin root make sure that our last
        // childs bottom margin is fully applied.
        // XXX check for a fit
        autoHeight += aState.mPrevBottomMargin;
      }
      autoHeight += borderPadding.bottom;

      // Apply min/max values
      if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMaxHeight) {
        nscoord computedMaxHeight = aReflowState.mComputedMaxHeight +
          borderPadding.top + borderPadding.bottom;
        if (autoHeight > computedMaxHeight) {
          autoHeight = computedMaxHeight;
        }
      }
      if (NS_UNCONSTRAINEDSIZE != aReflowState.mComputedMinHeight) {
        nscoord computedMinHeight = aReflowState.mComputedMinHeight +
          borderPadding.top + borderPadding.bottom;
        if (autoHeight < computedMinHeight) {
          autoHeight = computedMinHeight;
        }
      }
      aMetrics.height = autoHeight;

      if (aState.mComputeMaxElementSize) {
        maxHeight = aState.mMaxElementSize.height +
          borderPadding.top + borderPadding.bottom;
      }
    }

    aMetrics.ascent = aMetrics.height;
    aMetrics.descent = 0;
    if (aState.mComputeMaxElementSize) {
      // Store away the final value
      aMetrics.maxElementSize->width = maxWidth;
      aMetrics.maxElementSize->height = maxHeight;
    }

    // Return bottom margin information
    aMetrics.mCarriedOutBottomMargin =
      aState.mIsBottomMarginRoot ? 0 : aState.mPrevBottomMargin;

#ifdef DEBUG
    if (CRAZY_WIDTH(aMetrics.width) || CRAZY_HEIGHT(aMetrics.height)) {
      ListTag(stdout);
      printf(": WARNING: desired:%d,%d\n", aMetrics.width, aMetrics.height);
    }
    if (aState.mComputeMaxElementSize &&
        ((maxWidth > aMetrics.width) || (maxHeight > aMetrics.height))) {
      ListTag(stdout);
      printf(": WARNING: max-element-size:%d,%d desired:%d,%d maxSize:%d,%d\n",
             maxWidth, maxHeight, aMetrics.width, aMetrics.height,
             aState.mReflowState.availableWidth,
             aState.mReflowState.availableHeight);
    }
#endif
#ifdef NOISY_MAX_ELEMENT_SIZE
    if (aState.mComputeMaxElementSize) {
      IndentBy(stdout, GetDepth());
      if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableWidth) {
        printf("PASS1 ");
      }
      ListTag(stdout);
      printf(": max-element-size:%d,%d desired:%d,%d maxSize:%d,%d\n",
             maxWidth, maxHeight, aMetrics.width, aMetrics.height,
             aState.mReflowState.availableWidth,
             aState.mReflowState.availableHeight);
    }
#endif
  }

  // If we're requested to update our maximum width, then compute it
  if (aState.mComputeMaximumWidth) {
    // We need to add in for the right border/padding
    aMetrics.mMaximumWidth = aState.mMaximumWidth + borderPadding.right;
  }

  // Compute the combined area of our children
// XXX_perf: This can be done incrementally
  nscoord xa = 0, ya = 0, xb = aMetrics.width, yb = aMetrics.height;
  if (NS_STYLE_OVERFLOW_HIDDEN != aReflowState.mStyleDisplay->mOverflow) {
    nsLineBox* line = mLines;
    while (nsnull != line) {
      // Compute min and max x/y values for the reflowed frame's
      // combined areas
      nsRect lineCombinedArea;
      line->GetCombinedArea(&lineCombinedArea);
      nscoord x = lineCombinedArea.x;
      nscoord y = lineCombinedArea.y;
      nscoord xmost = x + lineCombinedArea.width;
      nscoord ymost = y + lineCombinedArea.height;
      if (x < xa) {
        xa = x;
      }
      if (xmost > xb) {
        xb = xmost;
      }
      if (y < ya) {
        ya = y;
      }
      if (ymost > yb) {
        yb = ymost;
      }
      line = line->mNext;
    }

    // Factor the bullet in; normally the bullet will be factored into
    // the line-box's combined area. However, if the line is a block
    // line then it won't; if there are no lines, it won't. So just
    // factor it in anyway (it can't hurt if it was already done).
    if (mBullet) {
      nsRect r;
      mBullet->GetRect(r);
      if (r.x < xa) xa = r.x;
      if (r.y < ya) ya = r.y;
      nscoord xmost = r.XMost();
      if (xmost > xb) xb = xmost;
      nscoord ymost = r.YMost();
      if (ymost > yb) yb = ymost;
    }
  }
#ifdef NOISY_COMBINED_AREA
  ListTag(stdout);
  printf(": ca=%d,%d,%d,%d\n", xa, ya, xb-xa, yb-ya);
#endif

  // If the combined area of our children exceeds our bounding box
  // then set the NS_FRAME_OUTSIDE_CHILDREN flag, otherwise clear it.
  aMetrics.mOverflowArea.x = xa;
  aMetrics.mOverflowArea.y = ya;
  aMetrics.mOverflowArea.width = xb - xa;
  aMetrics.mOverflowArea.height = yb - ya;
  if ((aMetrics.mOverflowArea.x < 0) ||
      (aMetrics.mOverflowArea.y < 0) ||
      (aMetrics.mOverflowArea.XMost() > aMetrics.width) ||
      (aMetrics.mOverflowArea.YMost() > aMetrics.height)) {
    mState |= NS_FRAME_OUTSIDE_CHILDREN;
  }
  else {
    mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
  }
}

nsresult
nsBlockFrame::PrepareInitialReflow(nsBlockReflowState& aState)
{
  PrepareResizeReflow(aState);
  return NS_OK;
}

nsresult
nsBlockFrame::PrepareChildIncrementalReflow(nsBlockReflowState& aState)
{
  // Determine the line being impacted
  PRBool isFloater;
  nsLineBox* prevLine;
  nsLineBox* line = FindLineFor(aState.mNextRCFrame, &prevLine, &isFloater);
  if (nsnull == line) {
    // This can't happen, but just in case it does...
    return PrepareResizeReflow(aState);
  }

  // XXX: temporary: If the child frame is a floater then punt
  if (isFloater) {
    return PrepareResizeReflow(aState);
  }

  // Figure out which line to mark dirty.
  MarkLineDirty(line, prevLine);

  return NS_OK;
}

nsresult
nsBlockFrame::MarkLineDirty(nsLineBox* aLine, nsLineBox* aPrevLine)
{
  // If the line that was affected is a block then just mark it dirty
  // so that we reflow it.
  if (aLine->IsBlock()) {
    aLine->MarkDirty();
#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": mark line %p dirty\n", aLine);
    }
#endif
  }
  else {
    // Mark previous line dirty if its an inline line so that it can
    // maybe pullup something from the line just affected.
    if (aPrevLine && !aPrevLine->IsBlock()) {
      aPrevLine->MarkDirty();
#ifdef DEBUG
      if (gNoisyReflow) {
        IndentBy(stdout, gNoiseIndent);
        ListTag(stdout);
        printf(": mark prev-line %p dirty\n", aPrevLine);
      }
#endif
    }
    else {
      aLine->MarkDirty();
#ifdef DEBUG
      if (gNoisyReflow) {
        IndentBy(stdout, gNoiseIndent);
        ListTag(stdout);
        printf(": mark line %p dirty\n", aLine);
      }
#endif
    }
  }

  return NS_OK;
}

nsresult
nsBlockFrame::UpdateBulletPosition(nsBlockReflowState& aState)
{
  if (nsnull == mBullet) {
    // Don't bother if there is no bullet
    return NS_OK;
  }
  const nsStyleList* styleList;
  GetStyleData(eStyleStruct_List, (const nsStyleStruct*&) styleList);
  if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == styleList->mListStylePosition) {
    if (HaveOutsideBullet()) {
      // We now have an inside bullet, but used to have an outside
      // bullet.  Adjust the frame line list
      nsLineBox* line = aState.NewLineBox(mBullet, 1, PR_FALSE);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      line->mNext = mLines;
      mLines = line;
    }
    mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
  }
  else {
    if (!HaveOutsideBullet()) {
      // We now have an outside bullet, but used to have an inside
      // bullet. Take the bullet frame out of the first lines frame
      // list.
      if ((nsnull != mLines) && (mBullet == mLines->mFirstChild)) {
        nsIFrame* next;
        mBullet->GetNextSibling(&next);
        mBullet->SetNextSibling(nsnull);
        PRInt32 count = mLines->GetChildCount() - 1;
        NS_ASSERTION(count >= 0, "empty line w/o bullet");
        mLines->SetChildCount(count);
        if (0 == count) {
          nsLineBox* nextLine = mLines->mNext;
          aState.FreeLineBox(mLines);
          mLines = nextLine;
          if (nsnull != nextLine) {
            nextLine->MarkDirty();
          }
        }
        else {
          mLines->mFirstChild = next;
          mLines->MarkDirty();
        }
      }
    }
    mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
  }
#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}

nsresult
nsBlockFrame::PrepareStyleChangedReflow(nsBlockReflowState& aState)
{
  nsresult rv = UpdateBulletPosition(aState);

  // Mark everything dirty
  nsLineBox* line = mLines;
  while (nsnull != line) {
    line->MarkDirty();
    line = line->mNext;
  }
  return rv;
}

nsresult
nsBlockFrame::PrepareResizeReflow(nsBlockReflowState& aState)
{
  // See if we can try and avoid marking all the lines as dirty
  PRBool  tryAndSkipLines = PR_FALSE;

  // See if this is this a constrained resize reflow
  if ((aState.mReflowState.reason == eReflowReason_Resize) &&
      (NS_UNCONSTRAINEDSIZE != aState.mReflowState.availableWidth)) {

    // If the text is left-aligned, then we try and avoid reflowing the lines
    const nsStyleText* styleText = (const nsStyleText*)
      mStyleContext->GetStyleData(eStyleStruct_Text);

    if ((NS_STYLE_TEXT_ALIGN_LEFT == styleText->mTextAlign) ||
        ((NS_STYLE_TEXT_ALIGN_DEFAULT == styleText->mTextAlign) &&
         (NS_STYLE_DIRECTION_LTR == aState.mReflowState.mStyleDisplay->mDirection))) {
      tryAndSkipLines = PR_TRUE;
    }
  }

#ifdef DEBUG
  if (gDisableResizeOpt) {
    tryAndSkipLines = PR_FALSE;
  }
  if (gNoisyReflow) {
    if (!tryAndSkipLines) {
      const nsStyleText* mStyleText = (const nsStyleText*)
        mStyleContext->GetStyleData(eStyleStruct_Text);
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": marking all lines dirty: reason=%d availWidth=%d textAlign=%d\n",
             aState.mReflowState.reason,
             aState.mReflowState.availableWidth,
             mStyleText->mTextAlign);
    }
  }
#endif

  nsLineBox* line = mLines;
  if (tryAndSkipLines) {
    // The line's bounds are relative to the border edge of the frame
    nscoord newAvailWidth = aState.mReflowState.mComputedBorderPadding.left;
     
    if (NS_SHRINKWRAPWIDTH == aState.mReflowState.mComputedWidth) {
      newAvailWidth += aState.mReflowState.mComputedMaxWidth;
    } else {
      newAvailWidth += aState.mReflowState.mComputedWidth;
    }

#ifdef DEBUG
    if (gNoisyReflow) {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": trying to avoid marking all lines dirty\n");
    }
#endif
    
    PRBool notWrapping = aState.mNoWrap;
    while (nsnull != line) {

      if (line->IsBlock()) {
        // We have to let child blocks make their own decisions.
        line->MarkDirty();
      }
      else {
        // We can avoid reflowing *some* inline lines in some cases.
        if (notWrapping) {
          // When no-wrap is set then the only line-breaking that
          // occurs for inline lines is triggered by BR elements or by
          // newlines. Therefore, we don't need to reflow the line.
        }
        else if ((line->mNext && !line->HasBreak()) ||
                 line->HasFloaters() || line->IsImpactedByFloater() ||
                 line->HasPercentageChild() ||
                 (line->mBounds.XMost() > newAvailWidth)) {
          // When an inline line has:
          //
          //  - a next line and it doesn't end in a break, or
          //  - floaters, or
          //  - is impacted by a floater, or
          //  - is wider than the new available space
          //
          // Then we must reflow it.
          line->MarkDirty();
        }

#ifdef DEBUG
        if (gNoisyReflow && !line->IsDirty() && !notWrapping) {
          IndentBy(stdout, gNoiseIndent + 1);
          printf("skipped: line=%p next=%p %s %s %s%s%s breakType=%d xmost=%d\n",
                 line, line->mNext,
                 line->IsBlock() ? "block" : "inline",
                 aState.mNoWrap ? "no-wrap" : "wrapping",
                 line->HasBreak() ? "has-break " : "",
                 line->HasFloaters() ? "has-floaters " : "",
                 line->IsImpactedByFloater() ? "impacted " : "",
                 line->GetBreakType(),
                 line->mBounds.XMost());
        }
#endif
      }
      line = line->mNext;
    }
  }
  else {
    // Mark everything dirty
    while (nsnull != line) {
      line->MarkDirty();
      line = line->mNext;
    }
  }
  return NS_OK;
}

//----------------------------------------

nsLineBox*
nsBlockFrame::FindLineFor(nsIFrame* aFrame,
                          nsLineBox** aPrevLineResult,
                          PRBool* aIsFloaterResult)
{
  nsLineBox* prevLine = nsnull;
  nsLineBox* line = mLines;
  PRBool isFloater = PR_FALSE;
  while (nsnull != line) {
    if (line->Contains(aFrame)) {
      break;
    }
    if (line->HasFloaters()) {
      nsFloaterCache* fc = line->GetFirstFloater();
      while (fc) {
        if (aFrame == fc->mPlaceholder->GetOutOfFlowFrame()) {
          isFloater = PR_TRUE;
          goto done;
        }
        fc = fc->Next();
      }
    }
    prevLine = line;
    line = line->mNext;
  }

 done:
  *aIsFloaterResult = isFloater;
  *aPrevLineResult = prevLine;
  return line;
}

void
nsBlockFrame::RecoverStateFrom(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               nscoord aDeltaY,
                               nsRect* aDamageRect)
{
  PRBool applyTopMargin = PR_FALSE;
  if (aLine->IsBlock()) {
    nsIFrame* framePrevInFlow;
    aLine->mFirstChild->GetPrevInFlow(&framePrevInFlow);
    if (nsnull == framePrevInFlow) {
      applyTopMargin = ShouldApplyTopMargin(aState, aLine);
    }
  }

  aState.RecoverStateFrom(aLine, applyTopMargin, aDeltaY, aDamageRect);
}

/**
 * Propogate reflow "damage" from the just reflowed line (aLine) to
 * any subsequent lines that were affected. The only thing that causes
 * damage is a change to the impact that floaters make.
 */
void
nsBlockFrame::PropogateReflowDamage(nsBlockReflowState& aState,
                                    nsLineBox* aLine,
                                    const nsRect& aOldCombinedArea,
                                    nscoord aDeltaY)
{
  // See if the line has a relevant combined area, and if it does if
  // the combined area has changed.
  nsRect lineCombinedArea;
  aLine->GetCombinedArea(&lineCombinedArea);
  if (lineCombinedArea != aLine->mBounds) {
    if (lineCombinedArea != aOldCombinedArea) {
      // The line's combined-area changed. Therefore we need to damage
      // the lines below that were previously (or are now) impacted by
      // the change. It's possible that a floater shrunk or grew so
      // use the larger of the impacted area.
      nscoord newYMost = lineCombinedArea.YMost();
      nscoord oldYMost = aOldCombinedArea.YMost();
      nscoord impactYB = newYMost < oldYMost ? oldYMost : newYMost;
      nscoord impactYA = lineCombinedArea.y;

      // Loop over each subsequent line and mark them dirty if they
      // intersect the impacted area. Note: we cannot stop after the
      // first non-intersecting line because lines might be
      // overlapping because of negative margins.
      nsLineBox* next = aLine->mNext;
      while (nsnull != next) {
        nscoord lineYA = next->mBounds.y + aDeltaY;
        nscoord lineYB = lineYA + next->mBounds.height;
        if ((lineYB >= impactYA) && (lineYA < impactYB)) {
          next->MarkDirty();
        }
        next = next->mNext;
      }
    }
    else {
      // The line's combined area didn't change from last
      // time. Therefore just sliding subsequent lines will work.
      return;
    }
  }

  if (aDeltaY) {
    nsLineBox* next = aLine->mNext;
    while (nsnull != next) {
      if (!next->IsDirty()) {
        // Cases we need to find:
        //
        // 1. the line was impacted by a floater and now isn't
        // 2. the line wasn't impacted by a floater and now is
        //
        //XXXPerf: An optimization: if the line was and is completely
        //impacted by a floater and the floater hasn't changed size,
        //then we don't need to mark the line dirty.
        aState.GetAvailableSpace(next->mBounds.y + aDeltaY);
        PRBool wasImpactedByFloater = next->IsImpactedByFloater();
        PRBool isImpactedByFloater = aState.IsImpactedByFloater();
        if (wasImpactedByFloater != isImpactedByFloater) {
          next->MarkDirty();
        }
        else if (isImpactedByFloater) {
          //XXX: Maybe the floater itself changed size?
          if (next->IsBlock()) {
            //XXXPerf
            // Case:
            // It's possible that more/less of the line is impacted by
            // the floater than last time. So reflow.
            next->MarkDirty();
          }
        }
      }
      next = next->mNext;
    }
  }
}

static PRBool
WrappedLinesAreDirty(nsLineBox* aLine)
{
  if (aLine->IsInline()) {
    while (aLine->IsLineWrapped()) {
      aLine = aLine->mNext;
      if (!aLine) {
        break;
      }

      NS_ASSERTION(!aLine->IsBlock(), "didn't expect a block line");
      if (aLine->IsDirty()) {
        // we found a continuing line that is dirty
        return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}

/**
 * Reflow the dirty lines
 */
nsresult
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  PRBool keepGoing = PR_TRUE;

#ifdef DEBUG
  if (gNoisyReflow) {
    if (aState.mReflowState.reason == eReflowReason_Incremental) {
      nsIReflowCommand::ReflowType type;
      aState.mReflowState.reflowCommand->GetType(type);
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": incrementally reflowing dirty lines: type=%s(%d)",
             kReflowCommandType[type], type);
    }
    else {
      IndentBy(stdout, gNoiseIndent);
      ListTag(stdout);
      printf(": reflowing dirty lines");
    }
    printf(" computedWidth=%d\n", aState.mReflowState.mComputedWidth);
    gNoiseIndent++;
  }
#endif

  // Check whether this is an incremental reflow
  PRBool  incrementalReflow = aState.mReflowState.reason ==
                              eReflowReason_Incremental ||
                              aState.mReflowState.reason ==
                              eReflowReason_Dirty;
  
  // Reflow the lines that are already ours
  aState.mPrevLine = nsnull;
  nsLineBox* line = mLines;
  nscoord deltaY = 0;
  while (nsnull != line) {
#ifdef DEBUG
    if (gNoisyReflow) {
      nsRect lca;
      line->GetCombinedArea(&lca);
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d dirty=%s oldBounds={%d,%d,%d,%d} oldCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d\n",
             line, aState.mY, line->IsDirty() ? "yes" : "no",
             line->mBounds.x, line->mBounds.y,
             line->mBounds.width, line->mBounds.height,
             lca.x, lca.y, lca.width, lca.height,
             deltaY, aState.mPrevBottomMargin);
      gNoiseIndent++;
    }
#endif

    // If we're supposed to update our maximum width, then we'll also need to
    // reflow this line if it's line wrapped and any of the continuing lines
    // are dirty
    if (line->IsDirty() || (aState.mComputeMaximumWidth && ::WrappedLinesAreDirty(line))) {
      // Compute the dirty lines "before" YMost, after factoring in
      // the running deltaY value - the running value is implicit in
      // aState.mY.
      nscoord oldHeight = line->mBounds.height;
      nsRect oldCombinedArea;
      line->GetCombinedArea(&oldCombinedArea);

      // Reflow the dirty line. If it's an incremental reflow, then have
      // it invalidate the dirty area
      rv = ReflowLine(aState, line, &keepGoing, incrementalReflow);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (!keepGoing) {
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line);
        }
        break;
      }
      nscoord newHeight = line->mBounds.height;
      deltaY += newHeight - oldHeight;

      // If the next line is clean then check and see if reflowing the
      // current line "damaged" the next line. Damage occurs when the
      // current line contains floaters that intrude upon the
      // subsequent lines.
      nsLineBox* next = line->mNext;
      if ((nsnull != next) && !next->IsDirty()) {
        PropogateReflowDamage(aState, line, oldCombinedArea, deltaY);
      }
    }
    else {
      // XXX what if the slid line doesn't fit because we are in a
      // vertically constrained situation?
      // Recover state as if we reflowed this line
      nsRect  damageRect;
      RecoverStateFrom(aState, line, deltaY, incrementalReflow ?
                       &damageRect : 0);
      if (incrementalReflow && !damageRect.IsEmpty()) {
        Invalidate(aState.mPresContext, damageRect);
      }
    }

#ifdef DEBUG
    if (gNoisyReflow) {
      gNoiseIndent--;
      nsRect lca;
      line->GetCombinedArea(&lca);
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d newBounds={%d,%d,%d,%d} newCombinedArea={%d,%d,%d,%d} deltaY=%d mPrevBottomMargin=%d\n",
             line, aState.mY,
             line->mBounds.x, line->mBounds.y,
             line->mBounds.width, line->mBounds.height,
             lca.x, lca.y, lca.width, lca.height,
             deltaY, aState.mPrevBottomMargin);
    }
#endif

    // If this is an inline frame then its time to stop
    aState.mPrevLine = line;
    line = line->mNext;
    aState.AdvanceToNextLine();
  }

  // Pull data from a next-in-flow if we can
  while (keepGoing && (nsnull != aState.mNextInFlow)) {
    // Grab first line from our next-in-flow
    line = aState.mNextInFlow->mLines;
    if (nsnull == line) {
      aState.mNextInFlow = (nsBlockFrame*) aState.mNextInFlow->mNextInFlow;
      continue;
    }
    // XXX See if the line is not dirty; if it's not maybe we can
    // avoid the pullup if it can't fit?
    aState.mNextInFlow->mLines = line->mNext;
    line->mNext = nsnull;
    if (0 == line->GetChildCount()) {
      // The line is empty. Try the next one.
      NS_ASSERTION(nsnull == line->mFirstChild, "bad empty line");
      aState.FreeLineBox(line);
      continue;
    }

    // XXX move to a subroutine: run-in, overflow, pullframe and this do this
    // Make the children in the line ours.
    nsIFrame* frame = line->mFirstChild;
    nsIFrame* lastFrame = nsnull;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      frame->SetParent(this);
      // When pushing and pulling frames we need to check for whether any
      // views need to be reparented
      nsHTMLContainerFrame::ReparentFrameView(aState.mPresContext, frame, mNextInFlow, this);
      lastFrame = frame;
      frame->GetNextSibling(&frame);
    }
    lastFrame->SetNextSibling(nsnull);

    // Add line to our line list
    if (nsnull == aState.mPrevLine) {
      NS_ASSERTION(nsnull == mLines, "bad aState.mPrevLine");
      mLines = line;
    }
    else {
      NS_ASSERTION(nsnull == aState.mPrevLine->mNext, "bad aState.mPrevLine");
      aState.mPrevLine->mNext = line;
      aState.mPrevChild->SetNextSibling(line->mFirstChild);
    }

    // Now reflow it and any lines that it makes during it's reflow
    // (we have to loop here because reflowing the line may case a new
    // line to be created; see SplitLine's callers for examples of
    // when this happens).
    while (nsnull != line) {
      rv = ReflowLine(aState, line, &keepGoing, incrementalReflow ?
                      PR_TRUE : PR_FALSE);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (!keepGoing) {
        if (0 == line->GetChildCount()) {
          DeleteLine(aState, line);
        }
        break;
      }

      // If this is an inline frame then its time to stop
      aState.mPrevLine = line;
      line = line->mNext;
      aState.AdvanceToNextLine();
    }
  }

  // Handle an odd-ball case: a list-item with no lines
  if (mBullet && HaveOutsideBullet() && !mLines) {
    nsHTMLReflowMetrics metrics(nsnull);
    ReflowBullet(aState, metrics);

    // There are no lines so we have to fake up some y motion so that
    // we end up with *some* height.
    aState.mY += metrics.height;
  }

#ifdef DEBUG
  if (gNoisyReflow) {
    gNoiseIndent--;
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": done reflowing dirty lines (status=%x)\n",
           aState.mReflowStatus);
  }
#endif

  return rv;
}

void
nsBlockFrame::DeleteLine(nsBlockReflowState& aState,
                         nsLineBox* aLine)
{
  NS_PRECONDITION(0 == aLine->GetChildCount(), "can't delete !empty line");
  if (0 == aLine->GetChildCount()) {
    if (nsnull == aState.mPrevLine) {
      NS_ASSERTION(aLine == mLines, "huh");
      mLines = nsnull;
    }
    else {
      NS_ASSERTION(aState.mPrevLine->mNext == aLine, "bad prev-line");
      aState.mPrevLine->mNext = aLine->mNext;
    }
    aState.FreeLineBox(aLine);
  }
}

/**
 * Reflow a line. The line will either contain a single block frame
 * or contain 1 or more inline frames. aLineReflowStatus indicates
 * whether or not the caller should continue to reflow more lines.
 */
nsresult
nsBlockFrame::ReflowLine(nsBlockReflowState& aState,
                         nsLineBox* aLine,
                         PRBool* aKeepReflowGoing,
                         PRBool aDamageDirtyArea)
{
  nsresult rv = NS_OK;

  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "reflowing empty line");

  // Setup the line-layout for the new line
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();

  // Now that we know what kind of line we have, reflow it
  nsRect oldCombinedArea;
  aLine->GetCombinedArea(&oldCombinedArea);

  if (aLine->IsBlock()) {
    rv = ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
    
    // We expect blocks to damage any area inside their bounds that is
    // dirty; however, if the frame changes size or position then we
    // need to do some repainting
    if (aDamageDirtyArea) {
      nsRect lineCombinedArea;
      aLine->GetCombinedArea(&lineCombinedArea);
      if ((oldCombinedArea.x != lineCombinedArea.x) ||
          (oldCombinedArea.y != lineCombinedArea.y)) {
        // The block has moved, and so do be safe we need to repaint
        // XXX We need to improve on this...
        nsRect  dirtyRect;
        dirtyRect.UnionRect(oldCombinedArea, lineCombinedArea);
        Invalidate(aState.mPresContext, dirtyRect);

      } else {
        if (oldCombinedArea.width != lineCombinedArea.width) {
          nsRect  dirtyRect;

          // Just damage the vertical strip that was either added or went
          // away
          dirtyRect.x = PR_MIN(oldCombinedArea.XMost(),
                               lineCombinedArea.XMost());
          dirtyRect.y = lineCombinedArea.y;
          dirtyRect.width = PR_MAX(oldCombinedArea.XMost(),
                                   lineCombinedArea.XMost()) -
                            dirtyRect.x;
          dirtyRect.height = PR_MAX(oldCombinedArea.height,
                                    lineCombinedArea.height);
          Invalidate(aState.mPresContext, dirtyRect);
        }
        if (oldCombinedArea.height != lineCombinedArea.height) {
          nsRect  dirtyRect;
          
          // Just damage the horizontal strip that was either added or went
          // away
          dirtyRect.x = lineCombinedArea.x;
          dirtyRect.y = PR_MIN(oldCombinedArea.YMost(),
                               lineCombinedArea.YMost());
          dirtyRect.width = PR_MAX(oldCombinedArea.width,
                                   lineCombinedArea.width);
          dirtyRect.height = PR_MAX(oldCombinedArea.YMost(),
                                    lineCombinedArea.YMost()) -
                             dirtyRect.y;
          Invalidate(aState.mPresContext, dirtyRect);
        }
      }
    }
  }
  else {
    aLine->SetLineWrapped(PR_FALSE);

    // If we're supposed to update the maximum width, then we'll need to reflow
    // the line with an unconstrained width (which will give us the new maximum
    // width), then we'll reflow it again with the constrained width.
    // We only do this if this is a beginning line, i.e., don't do this for
    // lines associated with content that line wrapped (see ReflowDirtyLines()
    // for details).
    // XXX This approach doesn't work when floaters are involved in which case
    // we'll either need to recover the floater state that applies to the
    // unconstrained reflow or keep it around in a separate space manager...
    PRBool  isBeginningLine = !aState.mPrevLine || !aState.mPrevLine->IsLineWrapped();
    if (aState.mComputeMaximumWidth && isBeginningLine) {
      nscoord oldY = aState.mY;
      nscoord oldPrevBottomMargin = aState.mPrevBottomMargin;

      // First reflow the line with an unconstrained width
      ReflowInlineFrames(aState, aLine, aKeepReflowGoing, PR_TRUE);

      // Update the line's maximum width
      aLine->mMaximumWidth = aLine->mBounds.XMost();
      aState.UpdateMaximumWidth(aLine->mMaximumWidth);

      // Remove any floaters associated with the line from the space
      // manager
      aLine->RemoveFloatersFromSpaceManager(aState.mSpaceManager);

      // Now reflow the line again this time without having it compute
      // the maximum width
      aState.mY = oldY;
      aState.mPrevBottomMargin = oldPrevBottomMargin;
      rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing);

    } else {
      rv = ReflowInlineFrames(aState, aLine, aKeepReflowGoing);
    }

    // We don't really know what changed in the line, so use the union
    // of the old and new combined areas
    if (aDamageDirtyArea) {
      nsRect combinedArea;
      aLine->GetCombinedArea(&combinedArea);

      nsRect dirtyRect;
      dirtyRect.UnionRect(oldCombinedArea, combinedArea);
      Invalidate(aState.mPresContext, dirtyRect);
    }
  }

  return rv;
}

/**
 * Pull frame from the next available location (one of our lines or
 * one of our next-in-flows lines).
 */
nsresult
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        nsLineBox* aLine,
                        nsIFrame*& aFrameResult)
{
  nsresult rv = NS_OK;
  PRBool stopPulling;
  aFrameResult = nsnull;

  // First check our remaining lines
  while (nsnull != aLine->mNext) {
    rv = PullFrame(aState, aLine, &aLine->mNext, PR_FALSE,
                   aFrameResult, stopPulling);
    if (NS_FAILED(rv) || stopPulling) {
      return rv;
    }
  }

  // Pull frames from the next-in-flow(s) until we can't
  nsBlockFrame* nextInFlow = aState.mNextInFlow;
  while (nsnull != nextInFlow) {
    nsLineBox* line = nextInFlow->mLines;
    if (nsnull == line) {
      nextInFlow = (nsBlockFrame*) nextInFlow->mNextInFlow;
      aState.mNextInFlow = nextInFlow;
      continue;
    }
    rv = PullFrame(aState, aLine, &nextInFlow->mLines, PR_TRUE,
                   aFrameResult, stopPulling);
    if (NS_FAILED(rv) || stopPulling) {
      return rv;
    }
  }
  return rv;
}

/**
 * Try to pull a frame out of a line pointed at by aFromList. If a
 * frame is pulled then aPulled will be set to PR_TRUE. In addition,
 * if aUpdateGeometricParent is set then the pulled frames geometric
 * parent will be updated (e.g. when pulling from a next-in-flows line
 * list).
 *
 * Note: pulling a frame from a line that is a place-holder frame
 * doesn't automatically remove the corresponding floater from the
 * line's floater array. This happens indirectly: either the line gets
 * emptied (and destroyed) or the line gets reflowed (because we mark
 * it dirty) and the code at the top of ReflowLine empties the
 * array. So eventually, it will be removed, just not right away.
 */
nsresult
nsBlockFrame::PullFrame(nsBlockReflowState& aState,
                        nsLineBox* aLine,
                        nsLineBox** aFromList,
                        PRBool aUpdateGeometricParent,
                        nsIFrame*& aFrameResult,
                        PRBool& aStopPulling)
{
  nsLineBox* fromLine = *aFromList;
  NS_ABORT_IF_FALSE(fromLine, "bad line to pull from");
  NS_ABORT_IF_FALSE(fromLine->GetChildCount(), "empty line");
  NS_ABORT_IF_FALSE(aLine->GetChildCount(), "empty line");

  if (fromLine->IsBlock()) {
    // If our line is not empty and the child in aFromLine is a block
    // then we cannot pull up the frame into this line. In this case
    // we stop pulling.
    aStopPulling = PR_TRUE;
    aFrameResult = nsnull;
  }
  else {
    // Take frame from fromLine
    nsIFrame* frame = fromLine->mFirstChild;
    aLine->SetChildCount(aLine->GetChildCount() + 1);

    PRInt32 fromLineChildCount = fromLine->GetChildCount();
    if (0 != --fromLineChildCount) {
      // Mark line dirty now that we pulled a child
      fromLine->SetChildCount(fromLineChildCount);
      fromLine->MarkDirty();
      frame->GetNextSibling(&fromLine->mFirstChild);
    }
    else {
      // Free up the fromLine now that it's empty
      *aFromList = fromLine->mNext;
      aState.FreeLineBox(fromLine);
    }

    // Change geometric parents
    if (aUpdateGeometricParent) {
      // Before we set the new parent frame get the current parent
      nsIFrame* oldParentFrame;
      frame->GetParent(&oldParentFrame);
      frame->SetParent(this);

      // When pushing and pulling frames we need to check for whether any
      // views need to be reparented
      NS_ASSERTION(oldParentFrame != this, "unexpected parent frame");
      nsHTMLContainerFrame::ReparentFrameView(aState.mPresContext, frame, oldParentFrame, this);
      
      // The frame is being pulled from a next-in-flow; therefore we
      // need to add it to our sibling list.
      if (nsnull != aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(frame);
      }
      frame->SetNextSibling(nsnull);
    }

    // Stop pulling because we found a frame to pull
    aStopPulling = PR_TRUE;
    aFrameResult = frame;
#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return NS_OK;
}

static void
PlaceFrameView(nsIPresContext* aPresContext,
               nsIFrame*       aFrame)
{
  nsIView*  view;
  aFrame->GetView(aPresContext, &view);
  if (view) {
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aFrame, view, nsnull);

  } else {
    nsContainerFrame::PositionChildViews(aPresContext, aFrame);
  }
}

void
nsBlockFrame::SlideLine(nsBlockReflowState& aState,
                        nsLineBox* aLine, nscoord aDY)
{
  // Adjust line state
  aLine->SlideBy(aDY);

  // Adjust the frames in the line
  nsIFrame* kid = aLine->mFirstChild;
  if (!kid) {
    return;
  }

  if (aLine->IsBlock()) {
    nsRect r;
    kid->GetRect(r);
    if (aDY) {
      r.y += aDY;
      kid->SetRect(aState.mPresContext, r);
    }

    // Make sure the frame's view and any child views are updated
    ::PlaceFrameView(aState.mPresContext, kid);

    // If the child has any floaters that impact the space-manager,
    // place them now so that they are present in the space-manager
    // again (they were removed by the space-manager's frame when
    // the reflow began).
    nsBlockFrame* bf;
    nsresult rv = kid->QueryInterface(kBlockFrameCID, (void**) &bf);
    if (NS_SUCCEEDED(rv)) {
      // Translate spacemanager to the child blocks upper-left corner
      // so that when it places its floaters (which are relative to
      // it) the right coordinates are used. Note that we have already
      // been translated by our border+padding so factor that in to
      // get the right translation.
      const nsMargin& bp = aState.BorderPadding();
      nscoord dx = r.x - bp.left;
      nscoord dy = r.y - bp.top;
      aState.mSpaceManager->Translate(dx, dy);
      bf->UpdateSpaceManager(aState.mPresContext, aState.mSpaceManager);
      aState.mSpaceManager->Translate(-dx, -dy);
    }
  }
  else {
    // Adjust the Y coordinate of the frames in the line.
    // Note: we need to re-position views even if aDY is 0, because
    // one of our parent frames may have moved and so the view's position
    // relative to its parent may have changed
    nsRect r;
    PRInt32 n = aLine->GetChildCount();
    while (--n >= 0) {
      if (aDY) {
        kid->GetRect(r);
        r.y += aDY;
        kid->SetRect(aState.mPresContext, r);
      }
      // Make sure the frame's view and any child views are updated
      ::PlaceFrameView(aState.mPresContext, kid);
      kid->GetNextSibling(&kid);
    }
  }
}

nsresult
nsBlockFrame::UpdateSpaceManager(nsIPresContext* aPresContext,
                                 nsISpaceManager* aSpaceManager)
{
  nsLineBox* line = mLines;
  while (nsnull != line) {
    // Place the floaters in the spacemanager
    if (line->HasFloaters()) {
      nsFloaterCache* fc = line->GetFirstFloater();
      while (fc) {
        nsIFrame* floater = fc->mPlaceholder->GetOutOfFlowFrame();
        aSpaceManager->AddRectRegion(floater, fc->mRegion);
#ifdef NOISY_SPACEMANAGER
        nscoord tx, ty;
        aSpaceManager->GetTranslation(tx, ty);
        nsFrame::ListTag(stdout, this);
        printf(": UpdateSpaceManager: AddRectRegion: txy=%d,%d {%d,%d,%d,%d}\n",
               tx, ty,
               fc->mRegion.x, fc->mRegion.y,
               fc->mRegion.width, fc->mRegion.height);
#endif
        fc = fc->Next();
      }
    }

    // Tell kids about the move too
    if (line->mFirstChild && line->IsBlock()) {
      // If the child has any floaters that impact the space-manager,
      // place them now so that they are present in the space-manager
      // again (they were removed by the space-manager's frame when
      // the reflow began).
      nsBlockFrame* bf;
      nsresult rv = line->mFirstChild->QueryInterface(kBlockFrameCID,
                                                      (void**) &bf);
      if (NS_SUCCEEDED(rv)) {
        nsPoint origin;
        bf->GetOrigin(origin);

        // Translate spacemanager to the child blocks upper-left
        // corner so that when it places its floaters (which are
        // relative to it) the right coordinates are used.
        aSpaceManager->Translate(origin.x, origin.y);
        bf->UpdateSpaceManager(aPresContext, aSpaceManager);
        aSpaceManager->Translate(-origin.x, -origin.y);
      }
    }
    
    line = line->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsBlockFrame::AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aHint)
{
  nsresult rv = nsBlockFrameSuper::AttributeChanged(aPresContext, aChild,
                                                    aNameSpaceID, aAttribute, aHint);

  if (NS_OK != rv) {
    return rv;
  }
  if (nsHTMLAtoms::start == aAttribute) {
    // XXX Not sure if this is necessary anymore
    RenumberLists();

    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    
    nsIReflowCommand* reflowCmd;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::ContentChanged,
                                 nsnull,
                                 aAttribute);
    if (NS_SUCCEEDED(rv)) {
      shell->AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }
  else if (nsHTMLAtoms::value == aAttribute) {
    const nsStyleDisplay* styleDisplay;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) styleDisplay);
    if (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) {
      nsIFrame* nextAncestor = mParent;
      nsBlockFrame* blockParent = nsnull;
      
      // Search for the closest ancestor that's a block frame. We
      // make the assumption that all related list items share a
      // common block parent.
      while (nextAncestor != nsnull) {
        if (NS_OK == nextAncestor->QueryInterface(kBlockFrameCID, 
                                                  (void**)&blockParent)) {
          break;
        }
        nextAncestor->GetParent(&nextAncestor);
      }

      // Tell the enclosing block frame to renumber list items within
      // itself
      if (nsnull != blockParent) {
        // XXX Not sure if this is necessary anymore
        blockParent->RenumberLists();

        nsCOMPtr<nsIPresShell> shell;
        aPresContext->GetShell(getter_AddRefs(shell));
        
        nsIReflowCommand* reflowCmd;
        rv = NS_NewHTMLReflowCommand(&reflowCmd, blockParent,
                                     nsIReflowCommand::ContentChanged,
                                     nsnull,
                                     aAttribute);
        if (NS_SUCCEEDED(rv)) {
          shell->AppendReflowCommand(reflowCmd);
          NS_RELEASE(reflowCmd);
        }
      }
    }
  }

  return rv;
}

nsBlockFrame*
nsBlockFrame::FindFollowingBlockFrame(nsIFrame* aFrame)
{
  nsBlockFrame* followingBlockFrame = nsnull;
  nsIFrame* frame = aFrame;
  for (;;) {
    nsIFrame* nextFrame;
    frame->GetNextSibling(&nextFrame);
    if (nsnull != nextFrame) {
      const nsStyleDisplay* display;
      nextFrame->GetStyleData(eStyleStruct_Display,
                              (const nsStyleStruct*&) display);
      if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
        followingBlockFrame = (nsBlockFrame*) nextFrame;
        break;
      }
      else if (NS_STYLE_DISPLAY_INLINE == display->mDisplay) {
        // If it's a text-frame and it's just whitespace and we are
        // in a normal whitespace situation THEN skip it and keep
        // going...
        // XXX WRITE ME!
      }
      frame = nextFrame;
    }
    else
      break;
  }
  return followingBlockFrame;
}

PRBool
nsBlockFrame::ShouldApplyTopMargin(nsBlockReflowState& aState,
                                   nsLineBox* aLine)
{
  if (aState.mApplyTopMargin) {
    // Apply short-circuit check to avoid searching the line list
    return PR_TRUE;
  }

  if (!aState.IsAdjacentWithTop()) {
    // If we aren't at the top Y coordinate then something of non-zero
    // height must have been placed. Therefore the childs top-margin
    // applies.
    aState.mApplyTopMargin = PR_TRUE;
    return PR_TRUE;
  }

  // Determine if this line is "essentially" the first line
  nsLineBox* line = mLines;
  while (line != aLine) {
    if (line->IsBlock()) {
      // A line which preceeds aLine contains a block; therefore the
      // top margin applies.
      aState.mApplyTopMargin = PR_TRUE;
      return PR_TRUE;
    }
    else if (line->HasFloaters()) {
      // A line which preceeds aLine is not empty therefore the top
      // margin applies.
      aState.mApplyTopMargin = PR_TRUE;
      return PR_TRUE;
    }
    line = line->mNext;
  }

  // The line being reflowed is "essentially" the first line in the
  // block. Therefore its top-margin will be collapsed by the
  // generational collapsing logic with its parent (us).
  return PR_FALSE;
}

nsIFrame*
nsBlockFrame::GetTopBlockChild()
{
  nsIFrame* firstChild = mLines ? mLines->mFirstChild : nsnull;
  if (firstChild) {
    if (mLines->IsBlock()) {
      // Winner
      return firstChild;
    }

    // If the first line is not a block line then the second line must
    // be a block line otherwise the top child can't be a block.
    nsLineBox* next = mLines->mNext;
    if ((nsnull == next) || !next->IsBlock()) {
      // There is no line after the first line or its not a block so
      // don't bother trying to skip over the first line.
      return nsnull;
    }

    // The only time we can skip over the first line and pretend its
    // not there is if the line contains only compressed
    // whitespace. If white-space is significant to this frame then we
    // can't skip over the line.
    const nsStyleText* styleText;
    GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&) styleText);
    if ((NS_STYLE_WHITESPACE_PRE == styleText->mWhiteSpace) ||
        (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == styleText->mWhiteSpace)) {
      // Whitespace is significant
      return nsnull;
    }

    // See if each frame is a text frame that contains nothing but
    // whitespace.
    PRInt32 n = mLines->GetChildCount();
    while (--n >= 0) {
      nsIContent* content;
      nsresult rv = firstChild->GetContent(&content);
      if (NS_FAILED(rv) || (nsnull == content)) {
        return nsnull;
      }
      nsITextContent* tc;
      rv = content->QueryInterface(kITextContentIID, (void**) &tc);
      NS_RELEASE(content);
      if (NS_FAILED(rv) || (nsnull == tc)) {
        return nsnull;
      }
      PRBool isws = PR_FALSE;
      tc->IsOnlyWhitespace(&isws);
      NS_RELEASE(tc);
      if (!isws) {
        return nsnull;
      }
      firstChild->GetNextSibling(&firstChild);
    }

    // If we make it to this point then every frame on the first line
    // was compressible white-space. Since we already know that the
    // second line contains a block, that block is the
    // top-block-child.
    return next->mFirstChild;
  }

  return nsnull;
}

nsresult
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               PRBool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");

  nsresult rv = NS_OK;

  nsIFrame* frame = aLine->mFirstChild;

  // Prepare the block reflow engine
  const nsStyleDisplay* display;
  frame->GetStyleData(eStyleStruct_Display,
                      (const nsStyleStruct*&) display);
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState,
                           aState.mComputeMaxElementSize,
                           aState.mComputeMaximumWidth);
  brc.SetNextRCFrame(aState.mNextRCFrame);

  // See if we should apply the top margin. If the block frame being
  // reflowed is a continuation (non-null prev-in-flow) then we don't
  // apply its top margin because its not significant. Otherwise, dig
  // deeper.
  PRBool applyTopMargin = PR_FALSE;
  nsIFrame* framePrevInFlow;
  frame->GetPrevInFlow(&framePrevInFlow);
  if (nsnull == framePrevInFlow) {
    applyTopMargin = ShouldApplyTopMargin(aState, aLine);
  }

  // Clear past floaters before the block if the clear style is not none
  PRUint8 breakType = display->mBreakType;
  aLine->SetBreakType(breakType);
  if (NS_STYLE_CLEAR_NONE != breakType) {
    PRBool alsoApplyTopMargin = aState.ClearPastFloaters(breakType);
    if (alsoApplyTopMargin) {
      applyTopMargin = PR_TRUE;
    }
#ifdef NOISY_VERTICAL_MARGINS
    ListTag(stdout);
    printf(": y=%d child ", aState.mY);
    ListTag(stdout, frame);
    printf(" has clear of %d => %s, mPrevBottomMargin=%d\n",
           breakType,
           applyTopMargin ? "applyTopMargin" : "nope",
           aState.mPrevBottomMargin);
#endif
  }

  nscoord topMargin = 0;
  if (applyTopMargin) {
    // Precompute the blocks top margin value so that we can get the
    // correct available space (there might be a floater that's
    // already been placed below the aState.mPrevBottomMargin

    // Setup a reflowState to get the style computed margin-top value

    // The availSpace here is irrelevant to our needs - all we want
    // out if this setup is the margin-top value which doesn't depend
    // on the childs available space.
    nsSize availSpace(aState.mContentArea.width, NS_UNCONSTRAINEDSIZE);
    nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                  frame, availSpace);

    // Now compute the collapsed margin-top value
    topMargin =
      nsBlockReflowContext::ComputeCollapsedTopMargin(aState.mPresContext,
                                                      reflowState);

    // And collapse it with the previous bottom margin to get the final value
    topMargin =
      nsBlockReflowContext::MaxMargin(topMargin, aState.mPrevBottomMargin);

    // Temporarily advance the running Y value so that the
    // GetAvailableSpace method will return the right available
    // space. This undone as soon as the margin is computed.
    aState.mY += topMargin;
  }

  // Compute the available space for the block
  aState.GetAvailableSpace();
  aLine->SetLineIsImpactedByFloater(aState.IsImpactedByFloater());
  nsSplittableType splitType = NS_FRAME_NOT_SPLITTABLE;
  frame->IsSplittable(splitType);
  nsRect availSpace;
  aState.ComputeBlockAvailSpace(frame, splitType, display, availSpace);

  // Now put the Y coordinate back and flow the block letting the
  // block reflow context compute the same top margin value we just
  // computed (sigh).
  if (topMargin) {
    aState.mY -= topMargin;
    availSpace.y -= topMargin;
    if (NS_UNCONSTRAINEDSIZE != availSpace.height) {
      availSpace.height += topMargin;
    }
  }

  // Reflow the block into the available space
  nsReflowStatus frameReflowStatus=NS_FRAME_COMPLETE;
  nsMargin computedOffsets;
  rv = brc.ReflowBlock(frame, availSpace,
                       applyTopMargin, aState.mPrevBottomMargin,
                       aState.IsAdjacentWithTop(),
                       computedOffsets, frameReflowStatus);
  if (NS_FAILED(rv)) {
    return rv;
  }
  aState.mPrevChild = frame;

#if defined(REFLOW_STATUS_COVERAGE)
  RecordReflowStatus(PR_TRUE, frameReflowStatus);
#endif

  if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
    // None of the child block fits.
    PushLines(aState);
    *aKeepReflowGoing = PR_FALSE;
    aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
  }
  else {
    // Note: line-break-after a block is a nop

    // Try to place the child block
    PRBool isAdjacentWithTop = aState.IsAdjacentWithTop();
    nscoord collapsedBottomMargin;
    nsRect combinedArea;
    *aKeepReflowGoing = brc.PlaceBlock(isAdjacentWithTop, computedOffsets,
                                       &collapsedBottomMargin,
                                       aLine->mBounds, combinedArea);
    if (aState.mShrinkWrapWidth) {
      // Mark the line as block so once we known the final shrink wrap width
      // we can reflow the block to the correct size
      // XXX We don't always need to do this...
      aLine->MarkDirty();
      aState.mNeedResizeReflow = PR_TRUE;

      // Add the right margin to the line's bounnds. That way it will be taken into
      // account when we compute our shrink wrap size
      nscoord marginRight = brc.GetMargin().right;
      if (marginRight != NS_UNCONSTRAINEDSIZE) {
        aLine->mBounds.width += marginRight;
      }

    }
    aLine->SetCombinedArea(combinedArea);
    if (*aKeepReflowGoing) {
      // Some of the child block fit

      // Advance to new Y position
      nscoord newY = aLine->mBounds.YMost();
      aState.mY = newY;
      aLine->SetCarriedOutBottomMargin(brc.GetCarriedOutBottomMargin());

      // Continue the block frame now if it didn't completely fit in
      // the available space.
      if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
        PRBool madeContinuation;
        rv = CreateContinuationFor(aState, aLine, frame, madeContinuation);
        if (NS_FAILED(rv)) {
          return rv;
        }

        // Push continuation to a new line, but only if we actually
        // made one.
        if (madeContinuation) {
          frame->GetNextSibling(&frame);
          nsLineBox* line = aState.NewLineBox(frame, 1, PR_TRUE);
          if (nsnull == line) {
            return NS_ERROR_OUT_OF_MEMORY;
          }
          line->mNext = aLine->mNext;
          aLine->mNext = line;

          // Do not count the continuation child on the line it used
          // to be on
          aLine->SetChildCount(aLine->GetChildCount() - 1);
       }

        // Advance to next line since some of the block fit. That way
        // only the following lines will be pushed.
        aState.mPrevLine = aLine;
        PushLines(aState);
        aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
        *aKeepReflowGoing = PR_FALSE;

        // The bottom margin for a block is only applied on the last
        // flow block. Since we just continued the child block frame,
        // we know that line->mFirstChild is not the last flow block
        // therefore zero out the running margin value.
#ifdef NOISY_VERTICAL_MARGINS
        ListTag(stdout);
        printf(": reflow incomplete, frame=");
        nsFrame::ListTag(stdout, frame);
        printf(" prevBottomMargin=%d, setting to zero\n",
               aState.mPrevBottomMargin);
#endif
        aState.mPrevBottomMargin = 0;
      }
      else {
#ifdef NOISY_VERTICAL_MARGINS
        ListTag(stdout);
        printf(": reflow complete for ");
        nsFrame::ListTag(stdout, frame);
        printf(" prevBottomMargin=%d collapsedBottomMargin=%d\n",
               aState.mPrevBottomMargin, collapsedBottomMargin);
#endif
        if (collapsedBottomMargin >= 0) {
          aState.mPrevBottomMargin = collapsedBottomMargin;
        }
        else {
          // Leave margin alone: it was a collapsed paragraph that
          // must not interfere with the running margin calculations
          // (in other words it should act like an empty line of
          // whitespace).
        }
      }
#ifdef NOISY_VERTICAL_MARGINS
      ListTag(stdout);
      printf(": frame=");
      nsFrame::ListTag(stdout, frame);
      printf(" carriedOutBottomMargin=%d collapsedBottomMargin=%d => %d\n",
             aLine->GetCarriedOutBottomMargin(), collapsedBottomMargin,
             aState.mPrevBottomMargin);
#endif

      // Post-process the "line"
      nsSize maxElementSize(0, 0);
      if (aState.mComputeMaxElementSize) {
        maxElementSize = brc.GetMaxElementSize();
        if (aState.IsImpactedByFloater() &&
            (NS_FRAME_SPLITTABLE_NON_RECTANGULAR != splitType)) {
          // Add in floater impacts to the lines max-element-size, but
          // only if the block element isn't one of us (otherwise the
          // floater impacts will be counted twice).
          ComputeLineMaxElementSize(aState, aLine, &maxElementSize);
        }
      }
      // If we asked the block to update its maximum width, then record the
      // updated value in the line, and update the current maximum width
      if (aState.mComputeMaximumWidth) {
        aLine->mMaximumWidth = brc.GetMaximumWidth();
        aState.UpdateMaximumWidth(aLine->mMaximumWidth);

      }
      PostPlaceLine(aState, aLine, maxElementSize);

      // Place the "marker" (bullet) frame.
      //
      // According to the CSS2 spec, section 12.6.1, the "marker" box
      // participates in the height calculation of the list-item box's
      // first line box.
      //
      // There are exactly two places a bullet can be placed: near the
      // first or second line. Its only placed on the second line in a
      // rare case: an empty first line followed by a second line that
      // contains a block (example: <LI>\n<P>... ). This is where
      // the second case can happen.
      if (HaveOutsideBullet() &&
          ((aLine == mLines) ||
           ((0 == mLines->mBounds.height) && (aLine == mLines->mNext)))) {
        // Reflow the bullet
        nsHTMLReflowMetrics metrics(nsnull);
        ReflowBullet(aState, metrics);

        // For bullets that are placed next to a child block, there will
        // be no correct ascent value. Therefore, make one up...
        nscoord ascent = 0;
        const nsStyleFont* font;
        frame->GetStyleData(eStyleStruct_Font,
                            (const nsStyleStruct*&) font);
        nsIRenderingContext& rc = *aState.mReflowState.rendContext;
        rc.SetFont(font->mFont);
        nsIFontMetrics* fm;
        rv = rc.GetFontMetrics(fm);
        if (NS_SUCCEEDED(rv) && (nsnull != fm)) {
          fm->GetMaxAscent(ascent);
          NS_RELEASE(fm);
        }
        rv = NS_OK;

        // Tall bullets won't look particularly nice here...
        nsRect bbox;
        mBullet->GetRect(bbox);
        nscoord bulletTopMargin = applyTopMargin ? collapsedBottomMargin : 0;
        bbox.y = aState.BorderPadding().top + ascent -
          metrics.ascent + bulletTopMargin;
        mBullet->SetRect(aState.mPresContext, bbox);
      }
    }
    else {
      // None of the block fits. Determine the correct reflow status.
      if (aLine == mLines) {
        // If it's our very first line then we need to be pushed to
        // our parents next-in-flow. Therefore, return break-before
        // status for our reflow status.
        aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      }
      else {
        // Push the line that didn't fit and any lines that follow it
        // to our next-in-flow.
        PushLines(aState);
        aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
  }
#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return rv;
}

#define LINE_REFLOW_OK   0
#define LINE_REFLOW_STOP 1
#define LINE_REFLOW_REDO 2

nsresult
nsBlockFrame::ReflowInlineFrames(nsBlockReflowState& aState,
                                 nsLineBox* aLine,
                                 PRBool* aKeepReflowGoing,
                                 PRBool aUpdateMaximumWidth)
{
  nsresult rv = NS_OK;
  *aKeepReflowGoing = PR_TRUE;

#ifdef DEBUG
  PRInt32 spins = 0;
#endif
  PRUint8 lineReflowStatus = LINE_REFLOW_REDO;
  while (LINE_REFLOW_REDO == lineReflowStatus) {
    // Prevent overflowing limited thread stacks by creating
    // nsLineLayout from the heap when the frame tree depth gets
    // large.
    if (aState.mReflowState.mReflowDepth > 30) {//XXX layout-tune.h?
      rv = DoReflowInlineFramesMalloc(aState, aLine, aKeepReflowGoing,
                                      &lineReflowStatus,
                                      aUpdateMaximumWidth);
    }
    else {
      rv = DoReflowInlineFramesAuto(aState, aLine, aKeepReflowGoing,
                                    &lineReflowStatus,
                                    aUpdateMaximumWidth);
    }
    if (NS_FAILED(rv)) {
      break;
    }
#ifdef DEBUG
    spins++;
    if (1000 == spins) {
      ListTag(stdout);
      printf(": yikes! spinning on a line over 1000 times!\n");
      NS_ABORT();
    }
#endif
  }
  return rv;
}

nsresult
nsBlockFrame::DoReflowInlineFramesMalloc(nsBlockReflowState& aState,
                                         nsLineBox* aLine,
                                         PRBool* aKeepReflowGoing,
                                         PRUint8* aLineReflowStatus,
                                         PRBool aUpdateMaximumWidth)
{
  nsLineLayout* ll = new nsLineLayout(aState.mPresContext,
                                      aState.mReflowState.mSpaceManager,
                                      &aState.mReflowState,
                                      aState.mComputeMaxElementSize);
  if (!ll) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  ll->Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
  ll->SetReflowTextRuns(mTextRuns);
  nsresult rv = DoReflowInlineFrames(aState, *ll, aLine, aKeepReflowGoing,
                                     aLineReflowStatus, aUpdateMaximumWidth);
  ll->EndLineReflow();
  delete ll;
  return rv;
}

nsresult
nsBlockFrame::DoReflowInlineFramesAuto(nsBlockReflowState& aState,
                                       nsLineBox* aLine,
                                       PRBool* aKeepReflowGoing,
                                       PRUint8* aLineReflowStatus,
                                       PRBool aUpdateMaximumWidth)
{
  nsLineLayout lineLayout(aState.mPresContext,
                          aState.mReflowState.mSpaceManager,
                          &aState.mReflowState,
                          aState.mComputeMaxElementSize);
  lineLayout.Init(&aState, aState.mMinLineHeight, aState.mLineNumber);
  lineLayout.SetReflowTextRuns(mTextRuns);
  nsresult rv = DoReflowInlineFrames(aState, lineLayout, aLine,
                                     aKeepReflowGoing, aLineReflowStatus,
                                     aUpdateMaximumWidth);
  lineLayout.EndLineReflow();
  return rv;
}

nsresult
nsBlockFrame::DoReflowInlineFrames(nsBlockReflowState& aState,
                                   nsLineLayout& aLineLayout,
                                   nsLineBox* aLine,
                                   PRBool* aKeepReflowGoing,
                                   PRUint8* aLineReflowStatus,
                                   PRBool aUpdateMaximumWidth)
{
  // Forget all of the floaters on the line
  aLine->FreeFloaters(aState.mFloaterCacheFreeList);
  aState.mFloaterCombinedArea.SetRect(0, 0, 0, 0);
  aState.mRightFloaterCombinedArea.SetRect(0, 0, 0, 0);

  // Setup initial coordinate system for reflowing the inline frames
  // into. Apply a previous block frame's bottom margin first.
  aState.mY += aState.mPrevBottomMargin;
  aState.GetAvailableSpace();
  PRBool impactedByFloaters = aState.IsImpactedByFloater();
  aLine->SetLineIsImpactedByFloater(impactedByFloaters);

  const nsMargin& borderPadding = aState.BorderPadding();
  nscoord x = aState.mAvailSpaceRect.x + borderPadding.left;
  nscoord availWidth = aState.mAvailSpaceRect.width;
  nscoord availHeight;
  if (aState.mUnconstrainedHeight) {
    availHeight = NS_UNCONSTRAINEDSIZE;
  }
  else {
    /* XXX get the height right! */
    availHeight = aState.mAvailSpaceRect.height;
  }
  if (aUpdateMaximumWidth) {
    availWidth = NS_UNCONSTRAINEDSIZE;
  }
  aLineLayout.BeginLineReflow(x, aState.mY,
                              availWidth, availHeight,
                              impactedByFloaters,
                              PR_FALSE /*XXX isTopOfPage*/);

  // XXX Unfortunately we need to know this before reflowing the first
  // inline frame in the line. FIX ME.
  if ((0 == aLineLayout.GetLineNumber()) &&
      (NS_BLOCK_HAS_FIRST_LETTER_STYLE & mState)) {
    aLineLayout.SetFirstLetterStyleOK(PR_TRUE);
  }

  // Reflow the frames that are already on the line first
  nsresult rv = NS_OK;
  PRUint8 lineReflowStatus = LINE_REFLOW_OK;
  PRInt32 i;
  nsIFrame* frame = aLine->mFirstChild;
  for (i = 0; i < aLine->GetChildCount(); i++) {      
    rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                           &lineReflowStatus);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (LINE_REFLOW_OK != lineReflowStatus) {
      // It is possible that one or more of next lines are empty
      // (because of DeleteChildsNextInFlow). If so, delete them now
      // in case we are finished.
      nsLineBox* nextLine = aLine->mNext;
      while ((nsnull != nextLine) && (0 == nextLine->GetChildCount())) {
        // XXX Is this still necessary now that DeleteChildsNextInFlow
        // uses DoRemoveFrame?
        aLine->mNext = nextLine->mNext;
        NS_ASSERTION(nsnull == nextLine->mFirstChild, "bad empty line");
        aState.FreeLineBox(nextLine);
        nextLine = aLine->mNext;
      }
      break;
    }
    frame->GetNextSibling(&frame);
  }

  // Pull frames and reflow them until we can't
  while (LINE_REFLOW_OK == lineReflowStatus) {
    rv = PullFrame(aState, aLine, frame);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (nsnull == frame) {
      break;
    }
    while (LINE_REFLOW_OK == lineReflowStatus) {
      PRInt32 oldCount = aLine->GetChildCount();
      rv = ReflowInlineFrame(aState, aLineLayout, aLine, frame,
                             &lineReflowStatus);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (aLine->GetChildCount() != oldCount) {
        // We just created a continuation for aFrame AND its going
        // to end up on this line (e.g. :first-letter
        // situation). Therefore we have to loop here before trying
        // to pull another frame.
        frame->GetNextSibling(&frame);
      }
      else {
        break;
      }
    }
  }
  if (LINE_REFLOW_REDO == lineReflowStatus) {
    // This happens only when we have a line that is impacted by
    // floaters and the first element in the line doesn't fit with
    // the floaters.
    //
    // What we do is to advance past the first floater we find and
    // then reflow the line all over again.
    NS_ASSERTION(aState.IsImpactedByFloater(),
                 "redo line on totally empty line");
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aState.mAvailSpaceRect.height,
                 "unconstrained height on totally empty line");

    aState.mY += aState.mAvailSpaceRect.height;
    // XXX: a small optimization can be done here when paginating:
    // if the new Y coordinate is past the end of the block then
    // push the line and return now instead of later on after we are
    // past the floater.
  }
  else {
    // If we are propogating out a break-before status then there is
    // no point in placing the line.
    if (!NS_INLINE_IS_BREAK_BEFORE(aState.mReflowStatus)) {
      rv = PlaceLine(aState, aLineLayout, aLine, aKeepReflowGoing, aUpdateMaximumWidth);
    }
  }
  *aLineReflowStatus = lineReflowStatus;

  return rv;
}

/**
 * Reflow an inline frame. The reflow status is mapped from the frames
 * reflow status to the lines reflow status (not to our reflow status).
 * The line reflow status is simple: PR_TRUE means keep placing frames
 * on the line; PR_FALSE means don't (the line is done). If the line
 * has some sort of breaking affect then aLine's break-type will be set
 * to something other than NS_STYLE_CLEAR_NONE.
 */
nsresult
nsBlockFrame::ReflowInlineFrame(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                nsLineBox* aLine,
                                nsIFrame* aFrame,
                                PRUint8* aLineReflowStatus)
{
  *aLineReflowStatus = LINE_REFLOW_OK;

  // If it's currently ok to be reflowing in first-letter style then
  // we must be about to reflow a frame that has first-letter style.
  PRBool reflowingFirstLetter = aLineLayout.GetFirstLetterStyleOK();
#ifdef NOISY_FIRST_LETTER
  ListTag(stdout);
  printf(": reflowing ");
  nsFrame::ListTag(stdout, aFrame);
  printf(" reflowingFirstLetter=%s\n", reflowingFirstLetter ? "on" : "off");
#endif

  // Reflow the inline frame
  nsReflowStatus frameReflowStatus;
  PRBool         pushedFrame;
  nsresult rv = aLineLayout.ReflowFrame(aFrame, &aState.mNextRCFrame,
                                        frameReflowStatus, nsnull, pushedFrame);
  if (NS_FAILED(rv)) {
    return rv;
  }
#ifdef REALLY_NOISY_REFLOW_CHILD
  nsFrame::ListTag(stdout, aFrame);
  printf(": status=%x\n", frameReflowStatus);
#endif

#if defined(REFLOW_STATUS_COVERAGE)
  RecordReflowStatus(PR_FALSE, frameReflowStatus);
#endif

  // Send post-reflow notification
  aState.mPrevChild = aFrame;

  // Process the child frames reflow status. There are 5 cases:
  // complete, not-complete, break-before, break-after-complete,
  // break-after-not-complete. There are two situations: we are a
  // block or we are an inline. This makes a total of 10 cases
  // (fortunately, there is some overlap).
  aLine->SetBreakType(NS_STYLE_CLEAR_NONE);
  if (NS_INLINE_IS_BREAK(frameReflowStatus)) {
    // Always abort the line reflow (because a line break is the
    // minimal amount of break we do).
    *aLineReflowStatus = LINE_REFLOW_STOP;

    // XXX what should aLine's break-type be set to in all these cases?
    PRUint8 breakType = NS_INLINE_GET_BREAK_TYPE(frameReflowStatus);
    NS_ASSERTION(breakType != NS_STYLE_CLEAR_NONE, "bad break type");
    NS_ASSERTION(NS_STYLE_CLEAR_PAGE != breakType, "no page breaks yet");

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      // Break-before cases.
      if (aFrame == aLine->mFirstChild) {
        // If we break before the first frame on the line then we must
        // be trying to place content where theres no room (e.g. on a
        // line with wide floaters). Inform the caller to reflow the
        // line after skipping past a floater.
        *aLineReflowStatus = LINE_REFLOW_REDO;
      }
      else {
        // It's not the first child on this line so go ahead and split
        // the line. We will see the frame again on the next-line.
        rv = SplitLine(aState, aLineLayout, aLine, aFrame);
        if (NS_FAILED(rv)) {
          return rv;
        }

        // If we're splitting the line because the frame didn't fit and it
        // was pushed, then mark the line as having word wrapped. We need to
        // know that if we're shrink wrapping our width
        if (pushedFrame) {
          aLine->SetLineWrapped(PR_TRUE);
        }
      }
    }
    else {
      // Break-after cases
      if (breakType == NS_STYLE_CLEAR_LINE) {
        if (!aLineLayout.GetLineEndsInBR()) {
          breakType = NS_STYLE_CLEAR_NONE;
        }
      }
      aLine->SetBreakType(breakType);
      if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
        // Create a continuation for the incomplete frame. Note that the
        // frame may already have a continuation.
        PRBool madeContinuation;
        rv = CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
        if (NS_FAILED(rv)) {
          return rv;
        }
        // Remember that the line has wrapped
        aLine->SetLineWrapped(PR_TRUE);
      }

      // Split line, but after the frame just reflowed
      nsIFrame* nextFrame;
      aFrame->GetNextSibling(&nextFrame);
      rv = SplitLine(aState, aLineLayout, aLine, nextFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }

      // Mark next line dirty in case SplitLine didn't end up
      // pushing any frames.
      nsLineBox* next = aLine->mNext;
      if ((nsnull != next) && !next->IsBlock()) {
        next->MarkDirty();
      }
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
    // Frame is not-complete, no special breaking status

    // Create a continuation for the incomplete frame. Note that the
    // frame may already have a continuation.
    PRBool madeContinuation;
    rv = CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
    if (NS_FAILED(rv)) {
      return rv;
    }

    // Remember that the line has wrapped
    aLine->SetLineWrapped(PR_TRUE);
    
    // If we are reflowing the first letter frame then don't split the
    // line and don't stop the line reflow...
    PRBool splitLine = !reflowingFirstLetter;
    if (reflowingFirstLetter) {
      nsCOMPtr<nsIAtom> frameType;
      aFrame->GetFrameType(getter_AddRefs(frameType));
      if ((nsLayoutAtoms::inlineFrame == frameType.get()) ||
          (nsLayoutAtoms::lineFrame == frameType.get())) {
        splitLine = PR_TRUE;
      }
    }

    if (splitLine) {
      // Split line after the current frame
      *aLineReflowStatus = LINE_REFLOW_STOP;
      aFrame->GetNextSibling(&aFrame);
      rv = SplitLine(aState, aLineLayout, aLine, aFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }

      // Mark next line dirty in case SplitLine didn't end up
      // pushing any frames.
      nsLineBox* next = aLine->mNext;
      if ((nsnull != next) && !next->IsBlock()) {
        next->MarkDirty();
      }
    }
  }

  return NS_OK;
}

/**
 * Create a continuation, if necessary, for aFrame. Place it on the
 * same line that aFrame is on. Set aMadeNewFrame to PR_TRUE if a
 * new frame is created.
 */
nsresult
nsBlockFrame::CreateContinuationFor(nsBlockReflowState& aState,
                                    nsLineBox* aLine,
                                    nsIFrame* aFrame,
                                    PRBool& aMadeNewFrame)
{
  aMadeNewFrame = PR_FALSE;
  nsresult rv;
  nsIFrame* nextInFlow;
  rv = CreateNextInFlow(aState.mPresContext, this, aFrame, nextInFlow);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsnull != nextInFlow) {
    aMadeNewFrame = PR_TRUE;
    aLine->SetChildCount(aLine->GetChildCount() + 1);
  }
#ifdef DEBUG
  VerifyLines(PR_FALSE);
#endif
  return rv;
}

nsresult
nsBlockFrame::SplitLine(nsBlockReflowState& aState,
                        nsLineLayout& aLineLayout,
                        nsLineBox* aLine,
                        nsIFrame* aFrame)
{
  NS_ABORT_IF_FALSE(aLine->IsInline(), "illegal SplitLine on block line");

  PRInt32 pushCount = aLine->GetChildCount() - aLineLayout.GetCurrentSpanCount();
  NS_ABORT_IF_FALSE(pushCount >= 0, "bad push count"); 

#ifdef DEBUG
  if (gNoisyReflow) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("split line: from line=%p pushCount=%d aFrame=", aLine, pushCount);
    if (aFrame) {
      nsFrame::ListTag(stdout, aFrame);
    }
    else {
      printf("(null)");
    }
    printf("\n");
    if (gReallyNoisyReflow) {
      aLine->List(aState.mPresContext, stdout, gNoiseIndent+1);
    }
  }
#endif

  if (0 != pushCount) {
    NS_ABORT_IF_FALSE(aLine->GetChildCount() > pushCount, "bad push");
    NS_ABORT_IF_FALSE(nsnull != aFrame, "whoops");

    // Put frames being split out into their own line
    nsLineBox* newLine = aState.NewLineBox(aFrame, pushCount, PR_FALSE);
    if (!newLine) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    newLine->mNext = aLine->mNext;
    aLine->mNext = newLine;
    aLine->SetChildCount(aLine->GetChildCount() - pushCount);
#ifdef DEBUG
    if (gReallyNoisyReflow) {
      newLine->List(aState.mPresContext, stdout, gNoiseIndent+1);
    }
#endif

    // Let line layout know that some frames are no longer part of its
    // state.
    aLineLayout.SplitLineTo(aLine->GetChildCount());
#ifdef DEBUG
    VerifyLines(PR_TRUE);
#endif
  }
  return NS_OK;
}

PRBool
nsBlockFrame::ShouldJustifyLine(nsBlockReflowState& aState, nsLineBox* aLine)
{
  nsLineBox* next = aLine->mNext;
  while (nsnull != next) {
    // There is another line
    if (0 != next->GetChildCount()) {
      // If the next line is a block line then we must not justify
      // this line because it means that this line is the last in a
      // group of inline lines.
      return !next->IsBlock();
    }

    // The next line is empty, try the next one
    next = next->mNext;
  }

  // XXX Not sure about this part
  // Try our next-in-flows lines to answer the question
  nsBlockFrame* nextInFlow = (nsBlockFrame*) mNextInFlow;
  while (nsnull != nextInFlow) {
    nsLineBox* line = nextInFlow->mLines;
    while (nsnull != line) {
      if (0 != line->GetChildCount()) {
        return !line->IsBlock();
      }
      line = line->mNext;
    }
    nextInFlow = (nsBlockFrame*) nextInFlow->mNextInFlow;
  }

  // This is the last line - so don't allow justification
  return PR_FALSE;
}

nsresult
nsBlockFrame::PlaceLine(nsBlockReflowState& aState,
                        nsLineLayout& aLineLayout,
                        nsLineBox* aLine,
                        PRBool* aKeepReflowGoing,
                        PRBool aUpdateMaximumWidth)
{
  nsresult rv = NS_OK;

  // Trim extra white-space from the line before placing the frames
  PRBool trimmed = aLineLayout.TrimTrailingWhiteSpace();
  aLine->SetTrimmed(trimmed);

  // Vertically align the frames on this line.
  //
  // According to the CSS2 spec, section 12.6.1, the "marker" box
  // participates in the height calculation of the list-item box's
  // first line box.
  //
  // There are exactly two places a bullet can be placed: near the
  // first or second line. Its only placed on the second line in a
  // rare case: an empty first line followed by a second line that
  // contains a block (example: <LI>\n<P>... ).
  //
  // For this code, only the first case is possible because this
  // method is used for placing a line of inline frames. If the rare
  // case is happening then the worst that will happen is that the
  // bullet frame will be reflowed twice.
  PRBool addedBullet = PR_FALSE;
  if (HaveOutsideBullet() && (aLine == mLines) &&
      (!aLineLayout.IsZeroHeight() || !aLine->mNext)) {
    nsHTMLReflowMetrics metrics(nsnull);
    ReflowBullet(aState, metrics);
    aLineLayout.AddBulletFrame(mBullet, metrics);
    addedBullet = PR_TRUE;
  }
  nsSize maxElementSize;
  aLineLayout.VerticalAlignFrames(aLine->mBounds, maxElementSize);
  // See if we're shrink wrapping the width
  if (aState.mShrinkWrapWidth) {
    // When determining the line's width we also need to include any
    // right floaters that impact us. This represents the shrink wrap
    // width of the line
    if (aState.IsImpactedByFloater() && !aLine->IsLineWrapped()) {
      NS_ASSERTION(aState.mContentArea.width >= aState.mAvailSpaceRect.XMost(), "bad state");
      aLine->mBounds.width += aState.mContentArea.width - aState.mAvailSpaceRect.XMost();
    }
  }
#ifdef DEBUG
	{
		static nscoord lastHeight = 0;
	  if (CRAZY_HEIGHT(aLine->mBounds.y)) {
	  	lastHeight = aLine->mBounds.y;
	  	if (abs(aLine->mBounds.y - lastHeight) > CRAZY_H/10) {
		    nsFrame::ListTag(stdout);
		    printf(": line=%p y=%d line.bounds.height=%d\n",
		           aLine, aLine->mBounds.y, aLine->mBounds.height);
	    }
	  }
	  else {
	  	lastHeight = 0;
	  }
  }
#endif

  // Only block frames horizontally align their children because
  // inline frames "shrink-wrap" around their children (therefore
  // there is no extra horizontal space).
#if XXX_fix_me
  PRBool allowJustify = PR_TRUE;
  if (NS_STYLE_TEXT_ALIGN_JUSTIFY == aState.mStyleText->mTextAlign) {
    allowJustify = ShouldJustifyLine(aState, aLine);
  }
#else
  PRBool allowJustify = PR_FALSE;
#endif

  PRBool successful;
  nsRect combinedArea;
  successful = aLineLayout.HorizontalAlignFrames(aLine->mBounds, allowJustify,
                                                 aState.mShrinkWrapWidth);
  if (!successful) {
    // Mark the line dirty and then later once we've determined the width
    // we can do the horizontal alignment
    aLine->MarkDirty();
    aState.mNeedResizeReflow = PR_TRUE;
  }
  aLineLayout.RelativePositionFrames(combinedArea);
  aLine->SetCombinedArea(combinedArea);
  if (addedBullet) {
    aLineLayout.RemoveBulletFrame(mBullet);
  }

  // Inline lines do not have margins themselves; however they are
  // impacted by prior block margins. If this line ends up having some
  // height then we zero out the previous bottom margin value that was
  // already applied to the line's starting Y coordinate. Otherwise we
  // leave it be so that the previous blocks bottom margin can be
  // collapsed with a block that follows.
  nscoord newY;
  if (aLine->mBounds.height > 0) {
    // This line has some height. Therefore the application of the
    // previous-bottom-margin should stick.
    aState.mPrevBottomMargin = 0;
    newY = aLine->mBounds.YMost();
  }
  else {
    // Don't let the previous-bottom-margin value affect the newY
    // coordinate (it was applied in ReflowInlineFrames speculatively)
    // since the line is empty.
    nscoord dy = -aState.mPrevBottomMargin;
    newY = aState.mY + dy;
    aLine->SlideBy(dy);
  }

  // See if the line fit. If it doesn't we need to push it. Our first
  // line will always fit.
  if ((mLines != aLine) && (newY > aState.mBottomEdge)) {
    // Push this line and all of it's children and anything else that
    // follows to our next-in-flow
    PushLines(aState);

    // Stop reflow and whack the reflow status if reflow hasn't
    // already been stopped.
    if (*aKeepReflowGoing) {
      NS_ASSERTION(NS_FRAME_COMPLETE == aState.mReflowStatus,
                   "lost reflow status");
      aState.mReflowStatus = NS_FRAME_NOT_COMPLETE;
      *aKeepReflowGoing = PR_FALSE;
    }
    return rv;
  }

  aState.mY = newY;
  if (aState.mComputeMaxElementSize) {
#ifdef NOISY_MAX_ELEMENT_SIZE
    IndentBy(stdout, GetDepth());
    if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableWidth) {
      printf("PASS1 ");
    }
    ListTag(stdout);
    printf(": line.floaters=%s%s band.floaterCount=%d\n",
           aLine->mFloaters.NotEmpty() ? "yes" : "no",
           aState.mHaveRightFloaters ? "(have right floaters)" : "",
           aState.mBand.GetFloaterCount());
#endif
    if (0 != aState.mBand.GetFloaterCount()) {
      // Add in floater impacts to the lines max-element-size
      ComputeLineMaxElementSize(aState, aLine, &maxElementSize);
    }
  }
  
  // If we're reflowing the line just to get incrementally update the
  // maximum width, then don't post-place the line. It's doing work we
  // don't need, and it will update things like aState.mKidXMost that
  // we don't want updated...
  if (!aUpdateMaximumWidth) {
    PostPlaceLine(aState, aLine, maxElementSize);
  }

  // Add the already placed current-line floaters to the line
  aLine->AppendFloaters(aState.mCurrentLineFloaters);

  // Any below current line floaters to place?
  if (aState.mBelowCurrentLineFloaters.NotEmpty()) {
    // Reflow the below-current-line floaters, then add them to the
    // lines floater list.
    aState.PlaceBelowCurrentLineFloaters(aState.mBelowCurrentLineFloaters);
    aLine->AppendFloaters(aState.mBelowCurrentLineFloaters);
  }

  // When a line has floaters, factor them into the combined-area
  // computations.
  if (aLine->HasFloaters()) {
    // Combine the floater combined area (stored in aState) and the
    // value computed by the line layout code.
    nsRect lineCombinedArea;
    aLine->GetCombinedArea(&lineCombinedArea);
#ifdef NOISY_COMBINED_AREA
    ListTag(stdout);
    printf(": lineCA=%d,%d,%d,%d floaterCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height,
           aState.mFloaterCombinedArea.x, aState.mFloaterCombinedArea.y,
           aState.mFloaterCombinedArea.width,
           aState.mFloaterCombinedArea.height);
#endif
    CombineRects(aState.mFloaterCombinedArea, lineCombinedArea);

    if (aState.mHaveRightFloaters &&
        (aState.mUnconstrainedWidth || aState.mShrinkWrapWidth)) {
      // We are reflowing in an unconstrained situation or shrink wrapping and
      // have some right floaters. They were placed at the infinite right edge
      // which will cause the combined area to be unusable.
      //
      // To solve this issue, we pretend that the right floaters ended up just
      // past the end of the line. Note that the right floater combined area
      // we computed as we were going will have as its X coordinate the left
      // most edge of all the right floaters. Therefore, to accomplish our goal
      // all we do is set that X value to the lines XMost value.
#ifdef NOISY_COMBINED_AREA
      printf("  ==> rightFloaterCA=%d,%d,%d,%d lineXMost=%d\n",
             aState.mRightFloaterCombinedArea.x,
             aState.mRightFloaterCombinedArea.y,
             aState.mRightFloaterCombinedArea.width,
             aState.mRightFloaterCombinedArea.height,
             aLine->mBounds.XMost());
#endif
      aState.mRightFloaterCombinedArea.x = aLine->mBounds.XMost();
      CombineRects(aState.mRightFloaterCombinedArea, lineCombinedArea);

      if (aState.mShrinkWrapWidth) {
        // Mark the line dirty so we come back and re-place the floater once
        // the shrink wrap width is determined
        aLine->MarkDirty();
        aState.mNeedResizeReflow = PR_TRUE;
      }
    }
    aLine->SetCombinedArea(lineCombinedArea);
#ifdef NOISY_COMBINED_AREA
    printf("  ==> final lineCA=%d,%d,%d,%d\n",
           lineCombinedArea.x, lineCombinedArea.y,
           lineCombinedArea.width, lineCombinedArea.height);
#endif
    aState.mHaveRightFloaters = PR_FALSE;
  }

  // Apply break-after clearing if necessary
  PRUint8 breakType = aLine->GetBreakType();
  switch (breakType) {
  case NS_STYLE_CLEAR_LEFT:
  case NS_STYLE_CLEAR_RIGHT:
  case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
    aState.ClearFloaters(aState.mY, breakType);
    break;
  }

  return rv;
}

// Compute the line's max-element-size by adding into the raw value
// computed by reflowing the contents of the line (aMaxElementSize)
// the impact of floaters on this line or the preceeding lines.
void
nsBlockFrame::ComputeLineMaxElementSize(nsBlockReflowState& aState,
                                        nsLineBox* aLine,
                                        nsSize* aMaxElementSize)
{
  nscoord maxWidth, maxHeight;
  aState.mBand.GetMaxElementSize(aState.mPresContext, &maxWidth, &maxHeight);
#ifdef NOISY_MAX_ELEMENT_SIZE
  IndentBy(stdout, GetDepth());
  if (NS_UNCONSTRAINEDSIZE == aState.mReflowState.availableWidth) {
    printf("PASS1 ");
  }
  ListTag(stdout);
  printf(": maxFloaterSize=%d,%d\n", maxWidth, maxHeight);
#endif

  // If the floaters are wider than the content, then use the maximum
  // floater width as the maximum width.
  //
  // It used to be the case that we would always place some content
  // next to a floater, regardless of the amount of available space
  // after subtracing off the floaters sizes. This can lead to content
  // overlapping floaters, so we no longer do this (and pass CSS2's
  // conformance tests). This is not how navigator 4-1 used to do
  // things.
  if (maxWidth > aMaxElementSize->width) {
    aMaxElementSize->width = maxWidth;
  }

  // Only update the max-element-size's height value if the floater is
  // part of the current line.
  if (aLine->HasFloaters()) {
    // If the maximum-height of the tallest floater is larger than the
    // maximum-height of the content then update the max-element-size
    // height
    if (maxHeight > aMaxElementSize->height) {
      aMaxElementSize->height = maxHeight;
    }
  }
}

void
nsBlockFrame::PostPlaceLine(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            const nsSize& aMaxElementSize)
{
  // If it's inline elements, then make sure the views are correctly
  // positioned and sized
  if (aLine->IsInline()) {
    nsIFrame* frame = aLine->mFirstChild;
    for (PRInt32 i = 0; i < aLine->GetChildCount(); i++) {
      ::PlaceFrameView(aState.mPresContext, frame);
      frame->GetNextSibling(&frame);
    }
  }

  // Update max-element-size
  if (aState.mComputeMaxElementSize) {
    aState.UpdateMaxElementSize(aMaxElementSize);
    // We also cache the max element width in the line. This is needed for
    // incremental reflow
    aLine->mMaxElementWidth = aMaxElementSize.width;
  }

  // If this is an unconstrained reflow, then cache the line width in the
  // line. We'll need this during incremental reflow if we're asked to
  // calculate the maximum width
  if (aState.mUnconstrainedWidth) {
    aLine->mMaximumWidth = aLine->mBounds.XMost();
  }

  // Update xmost
  nscoord xmost = aLine->mBounds.XMost();
#ifdef DEBUG
  if (CRAZY_WIDTH(xmost)) {
    ListTag(stdout);
    printf(": line=%p xmost=%d\n", aLine, xmost);
  }
#endif
  // If we're shrink wrapping our width and the line was wrapped,
  // then make sure we take up all of the available width
  if (aState.mShrinkWrapWidth && aLine->IsLineWrapped()) {
    aState.mKidXMost = aState.BorderPadding().left + aState.mContentArea.width;

  }
  else if (xmost > aState.mKidXMost) {
    aState.mKidXMost = xmost;
  }
}

void
nsBlockFrame::PushLines(nsBlockReflowState& aState)
{
  NS_ASSERTION(nsnull != aState.mPrevLine, "bad push");

  nsLineBox* lastLine = aState.mPrevLine;
  nsLineBox* nextLine = lastLine->mNext;

  if (nextLine) {
    lastLine->mNext = nsnull;
    SetOverflowLines(aState.mPresContext, nextLine);
  
    // Mark all the overflow lines dirty so that they get reflowed when
    // they are pulled up by our next-in-flow.
    while (nsnull != nextLine) {
      nextLine->MarkDirty();
      nextLine = nextLine->mNext;
    }
  }

  // Break frame sibling list
  nsIFrame* lastFrame = lastLine->LastChild();
  lastFrame->SetNextSibling(nsnull);

#ifdef DEBUG
  VerifyOverflowSituation(aState.mPresContext);
#endif
}

PRBool
nsBlockFrame::DrainOverflowLines(nsIPresContext* aPresContext)
{
#ifdef DEBUG
  VerifyOverflowSituation(aPresContext);
#endif
  PRBool drained = PR_FALSE;
  nsLineBox* overflowLines;

  // First grab the prev-in-flows overflow lines
  nsBlockFrame* prevBlock = (nsBlockFrame*) mPrevInFlow;
  if (nsnull != prevBlock) {
    overflowLines = prevBlock->GetOverflowLines(aPresContext, PR_TRUE);
    if (nsnull != overflowLines) {
      drained = PR_TRUE;

      // Make all the frames on the overflow line list mine
      nsIFrame* lastFrame = nsnull;
      nsIFrame* frame = overflowLines->mFirstChild;
      while (nsnull != frame) {
        frame->SetParent(this);

        // When pushing and pulling frames we need to check for whether any
        // views need to be reparented
        nsHTMLContainerFrame::ReparentFrameView(aPresContext, frame, prevBlock, this);

        // Get the next frame
        lastFrame = frame;
        frame->GetNextSibling(&frame);
      }

      // Join the line lists
      if (nsnull == mLines) {
        mLines = overflowLines;
      }
      else {
        // Join the sibling lists together
        lastFrame->SetNextSibling(mLines->mFirstChild);

        // Place overflow lines at the front of our line list
        nsLineBox* lastLine = nsLineBox::LastLine(overflowLines);
        lastLine->mNext = mLines;
        mLines = overflowLines;
      }
    }
  }

  // Now grab our own overflow lines
  overflowLines = GetOverflowLines(aPresContext, PR_TRUE);
  if (overflowLines) {
    // This can happen when we reflow and not everything fits and then
    // we are told to reflow again before a next-in-flow is created
    // and reflows.
    nsLineBox* lastLine = nsLineBox::LastLine(mLines);
    if (nsnull == lastLine) {
      mLines = overflowLines;
    }
    else {
      lastLine->mNext = overflowLines;
      nsIFrame* lastFrame = lastLine->LastChild();
      lastFrame->SetNextSibling(overflowLines->mFirstChild);
    }
    drained = PR_TRUE;
  }
  return drained;
}

nsLineBox*
nsBlockFrame::GetOverflowLines(nsIPresContext* aPresContext,
                               PRBool          aRemoveProperty)
{
  nsCOMPtr<nsIPresShell>     presShell;
  aPresContext->GetShell(getter_AddRefs(presShell));

  if (presShell) {
    nsCOMPtr<nsIFrameManager>  frameManager;
    presShell->GetFrameManager(getter_AddRefs(frameManager));
  
    if (frameManager) {
      PRUint32  options = 0;
      void*     value;
  
      if (aRemoveProperty) {
        options |= NS_IFRAME_MGR_REMOVE_PROP;
      }
      frameManager->GetFrameProperty(this, nsLayoutAtoms::overflowLinesProperty,
                                     options, &value);
      return (nsLineBox*)value;
    }
  }

  return nsnull;
}

// Destructor function for the overflowLines frame property
static void
DestroyOverflowLines(nsIPresContext* aPresContext,
                     nsIFrame*       aFrame,
                     nsIAtom*        aPropertyName,
                     void*           aPropertyValue)
{
  if (aPropertyValue) {
    nsLineBox* lines = (nsLineBox*) aPropertyValue;
    nsLineBox::DeleteLineList(aPresContext, lines);
  }
}

nsresult
nsBlockFrame::SetOverflowLines(nsIPresContext* aPresContext,
                               nsLineBox*      aOverflowFrames)
{
  nsCOMPtr<nsIPresShell>     presShell;
  nsresult                   rv = NS_ERROR_FAILURE;

  aPresContext->GetShell(getter_AddRefs(presShell));
  if (presShell) {
    nsCOMPtr<nsIFrameManager>  frameManager;
    presShell->GetFrameManager(getter_AddRefs(frameManager));
  
    if (frameManager) {
      rv = frameManager->SetFrameProperty(this, nsLayoutAtoms::overflowLinesProperty,
                                          aOverflowFrames, DestroyOverflowLines);

      // Verify that we didn't overwrite an existing overflow list
      NS_ASSERTION(rv != NS_IFRAME_MGR_PROP_OVERWRITTEN, "existing overflow list");
    }
  }
  return rv;
}

//////////////////////////////////////////////////////////////////////
// Frame list manipulation routines

nsIFrame*
nsBlockFrame::LastChild()
{
  if (mLines) {
    nsLineBox* line = nsLineBox::LastLine(mLines);
    return line->LastChild();
  }
  return nsnull;
}

NS_IMETHODIMP
nsBlockFrame::AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  if (nsnull == aFrameList) {
    return NS_OK;
  }
  if (nsLayoutAtoms::floaterList == aListName) {
    // XXX we don't *really* care about this right now because we are
    // BuildFloaterList ing still
    mFloaters.AppendFrames(nsnull, aFrameList);
    return NS_OK;
  }
  else if (nsnull != aListName) {
    return NS_ERROR_INVALID_ARG;
  }

  // Find the proper last-child for where the append should go
  nsIFrame* lastKid = nsnull;
  nsLineBox* lastLine = nsLineBox::LastLine(mLines);
  if (lastLine) {
    lastKid = lastLine->LastChild();
  }

  // Add frames after the last child
#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": append ");
  nsFrame::ListTag(stdout, aFrameList);
  if (lastKid) {
    printf(" after ");
    nsFrame::ListTag(stdout, lastKid);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aPresContext, aFrameList, lastKid);
  if (NS_SUCCEEDED(rv)) {
    // Generate reflow command to reflow the dirty lines
    nsIReflowCommand* reflowCmd = nsnull;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::ReflowDirty,
                                 nsnull);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != aListName) {
        reflowCmd->SetChildListName(aListName);
      }
      aPresShell.AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::InsertFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList)
{
  if (nsLayoutAtoms::floaterList == aListName) {
    // XXX we don't *really* care about this right now because we are
    // BuildFloaterList'ing still
    mFloaters.AppendFrames(nsnull, aFrameList);
    return NS_OK;
  }
  else if (nsnull != aListName) {
    return NS_ERROR_INVALID_ARG;
  }

#ifdef NOISY_REFLOW_REASON
  ListTag(stdout);
  printf(": insert ");
  nsFrame::ListTag(stdout, aFrameList);
  if (aPrevFrame) {
    printf(" after ");
    nsFrame::ListTag(stdout, aPrevFrame);
  }
  printf("\n");
#endif
  nsresult rv = AddFrames(aPresContext, aFrameList, aPrevFrame);
  if (NS_SUCCEEDED(rv)) {
    // Generate reflow command to reflow the dirty lines
    nsIReflowCommand* reflowCmd = nsnull;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::ReflowDirty,
                                 nsnull);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != aListName) {
        reflowCmd->SetChildListName(aListName);
      }
      aPresShell.AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }
  return rv;
}

nsresult
nsBlockFrame::AddFrames(nsIPresContext* aPresContext,
                        nsIFrame* aFrameList,
                        nsIFrame* aPrevSibling)
{
  if (nsnull == aFrameList) {
    return NS_OK;
  }

  // Attempt to find the line that contains the previous sibling
  nsLineBox* prevSibLine = nsnull;
  PRInt32 prevSiblingIndex = -1;
  if (aPrevSibling) {
    // Find the line that contains the previous sibling
    prevSibLine = nsLineBox::FindLineContaining(mLines, aPrevSibling,
                                                &prevSiblingIndex);
    NS_ASSERTION(nsnull != prevSibLine, "prev sibling not in line list");
    if (nsnull == prevSibLine) {
      // Note: defensive code! FindLineContaining must not return
      // null in this case, so if it does...
      aPrevSibling = nsnull;
    }
  }

  // Find the frame following aPrevSibling so that we can join up the
  // two lists of frames.
  nsIFrame* prevSiblingNextFrame = nsnull;
  if (aPrevSibling) {
    aPrevSibling->GetNextSibling(&prevSiblingNextFrame);

    // Split line containing aPrevSibling in two if the insertion
    // point is somewhere in the middle of the line.
    PRInt32 rem = prevSibLine->GetChildCount() - prevSiblingIndex - 1;
    if (rem) {
      // Split the line in two where the frame(s) are being inserted.
      nsLineBox* line = new nsLineBox(prevSiblingNextFrame, rem, PR_FALSE);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      line->mNext = prevSibLine->mNext;
      prevSibLine->mNext = line;
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() - rem);
      prevSibLine->MarkDirty();
    }

    // Now (partially) join the sibling lists together
    aPrevSibling->SetNextSibling(aFrameList);
  }
  else if (mLines) {
    prevSiblingNextFrame = mLines->mFirstChild;
  }

  // Walk through the new frames being added and update the line data
  // structures to fit.
  nsIFrame* newFrame = aFrameList;
  while (newFrame) {
    PRBool isBlock = nsLineLayout::TreatFrameAsBlock(newFrame);

    // If the frame is a block frame, or if there is no previous line
    // or if the previous line is a block line then make a new line.
    if (isBlock || !prevSibLine || prevSibLine->IsBlock()) {
      // Create a new line for the frame and add its line to the line
      // list.
      nsLineBox* line = new nsLineBox(newFrame, 1, isBlock);
      if (!line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (prevSibLine) {
        // Append new line after prevSibLine
        line->mNext = prevSibLine->mNext;
        prevSibLine->mNext = line;
      }
      else {
        // New line is going before the other lines
        line->mNext = mLines;
        mLines = line;
      }
      prevSibLine = line;
    }
    else {
      prevSibLine->SetChildCount(prevSibLine->GetChildCount() + 1);
      prevSibLine->MarkDirty();
    }

    aPrevSibling = newFrame;
    newFrame->GetNextSibling(&newFrame);
  }
  if (prevSiblingNextFrame) {
    // Connect the last new frame to the remainder of the sibling list
    aPrevSibling->SetNextSibling(prevSiblingNextFrame);
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}


void
nsBlockFrame::FixParentAndView(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  while (aFrame) {
    nsIFrame* oldParent;
    aFrame->GetParent(&oldParent);
    aFrame->SetParent(this);
    if (this != oldParent) {
      nsHTMLContainerFrame::ReparentFrameView(aPresContext, aFrame, oldParent, this);
      aPresContext->ReParentStyleContext(aFrame, mStyleContext);
    }
    aFrame->GetNextSibling(&aFrame);
  }
}

NS_IMETHODIMP
nsBlockFrame::RemoveFrame(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame)
{
  nsresult rv = NS_OK;

#ifdef NOISY_REFLOW_REASON
    ListTag(stdout);
    printf(": remove ");
    nsFrame::ListTag(stdout, aOldFrame);
    printf("\n");
#endif

  if (nsLayoutAtoms::floaterList == aListName) {
    // Remove floater from the floater list first
    mFloaters.RemoveFrame(aOldFrame);

    // Find which line contains the floater
    nsLineBox* line = mLines;
    while (nsnull != line) {
      if (line->IsInline() && line->RemoveFloater(aOldFrame)) {
        aOldFrame->Destroy(aPresContext);
        goto found_it;
      }
      line = line->mNext;
    }
   found_it:

    // Mark every line at and below the line where the floater was dirty
    while (nsnull != line) {
      line->MarkDirty();
      line = line->mNext;
    }
  }
  else if (nsnull != aListName) {
    rv = NS_ERROR_INVALID_ARG;
  }
  else {
    rv = DoRemoveFrame(aPresContext, aOldFrame);
  }

  if (NS_SUCCEEDED(rv)) {
    // Generate reflow command to reflow the dirty lines
    nsIReflowCommand* reflowCmd = nsnull;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::ReflowDirty,
                                 nsnull);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != aListName) {
        reflowCmd->SetChildListName(aListName);
      }
      aPresShell.AppendReflowCommand(reflowCmd);
      NS_RELEASE(reflowCmd);
    }
  }
  return rv;
}

nsresult
nsBlockFrame::DoRemoveFrame(nsIPresContext* aPresContext,
                            nsIFrame* aDeletedFrame)
{
  // Find the line and the previous sibling that contains
  // deletedFrame; we also find the pointer to the line.
  nsBlockFrame* flow = this;
  nsLineBox** linep = &flow->mLines;
  nsLineBox* line = flow->mLines;
  nsLineBox* prevLine = nsnull;
  nsIFrame* prevSibling = nsnull;
  while (nsnull != line) {
    nsIFrame* frame = line->mFirstChild;
    PRInt32 n = line->GetChildCount();
    while (--n >= 0) {
      if (frame == aDeletedFrame) {
        goto found_frame;
      }
      prevSibling = frame;
      frame->GetNextSibling(&frame);
    }
    linep = &line->mNext;
    prevLine = line;
    line = line->mNext;
  }
 found_frame:;
#ifdef NS_DEBUG
  NS_ASSERTION(nsnull != line, "can't find deleted frame in lines");
  if (nsnull != prevSibling) {
    nsIFrame* tmp;
    prevSibling->GetNextSibling(&tmp);
    NS_ASSERTION(tmp == aDeletedFrame, "bad prevSibling");
  }
#endif

  // Remove frame and all of its continuations
  while (nsnull != aDeletedFrame) {
    while ((nsnull != line) && (nsnull != aDeletedFrame)) {
#ifdef NS_DEBUG
      nsIFrame* parent;
      aDeletedFrame->GetParent(&parent);
      NS_ASSERTION(flow == parent, "messed up delete code");
      NS_ASSERTION(line->Contains(aDeletedFrame), "frame not in line");
#endif

      // See if the frame being deleted is the last one on the line
      PRBool isLastFrameOnLine = PR_FALSE;
      if (1 == line->GetChildCount()) {
        isLastFrameOnLine = PR_TRUE;
      }
      else if (line->LastChild() == aDeletedFrame) {
        isLastFrameOnLine = PR_TRUE;
      }

      // Remove aDeletedFrame from the line
      nsIFrame* nextFrame;
      aDeletedFrame->GetNextSibling(&nextFrame);
      if (line->mFirstChild == aDeletedFrame) {
        line->mFirstChild = nextFrame;
      }
      if (prevLine && !prevLine->IsBlock()) {
        // Since we just removed a frame that follows some inline
        // frames, we need to reflow the previous line.
        prevLine->MarkDirty();
      }

      // Take aDeletedFrame out of the sibling list. Note that
      // prevSibling will only be nsnull when we are deleting the very
      // first frame.
      if (nsnull != prevSibling) {
        prevSibling->SetNextSibling(nextFrame);
      }

      // Destroy frame; capture its next-in-flow first in case we need
      // to destroy that too.
      nsIFrame* nextInFlow;
      aDeletedFrame->GetNextInFlow(&nextInFlow);
      nsSplittableType st;
      aDeletedFrame->IsSplittable(st);
      if (NS_FRAME_NOT_SPLITTABLE != st) {
        nsSplittableFrame::RemoveFromFlow(aDeletedFrame);
      }
#ifdef NOISY_REMOVE_FRAME
      printf("DoRemoveFrame: prevLine=%p line=%p frame=",
             prevLine, line);
      nsFrame::ListTag(stdout, aDeletedFrame);
      printf(" prevSibling=%p nextInFlow=%p\n", prevSibling, nextInFlow);
#endif
      aDeletedFrame->Destroy(aPresContext);
      aDeletedFrame = nextInFlow;

      // If line is empty, remove it now
      nsLineBox* next = line->mNext;
      PRInt32 lineChildCount = line->GetChildCount();
      if (0 == --lineChildCount) {
        *linep = next;
        line->mNext = nsnull;
        // Invalidate the space taken up by the line.
        // XXX We need to do this if we're removing a frame as a result of
        // a call to RemoveFrame(), but we may not need to do this in all
        // cases...
        nsRect lineCombinedArea;
        line->GetCombinedArea(&lineCombinedArea);
        Invalidate(aPresContext, lineCombinedArea);
        delete line;
        line = next;
      }
      else {
        // Make the line that just lost a frame dirty
        line->SetChildCount(lineChildCount);
        line->MarkDirty();

        // If we just removed the last frame on the line then we need
        // to advance to the next line.
        if (isLastFrameOnLine) {
          prevLine = line;
          linep = &line->mNext;
          line = next;
        }
      }

      // See if we should keep looking in the current flow's line list.
      if (nsnull != aDeletedFrame) {
        if (aDeletedFrame != nextFrame) {
          // The deceased frames continuation is not the next frame in
          // the current flow's frame list. Therefore we know that the
          // continuation is in a different parent. So break out of
          // the loop so that we advance to the next parent.
#ifdef NS_DEBUG
          nsIFrame* checkParent;
          aDeletedFrame->GetParent(&checkParent);
          NS_ASSERTION(checkParent != flow, "strange continuation");
#endif
          break;
        }
      }
    }

    // Advance to next flow block if the frame has more continuations
    if (nsnull != aDeletedFrame) {
      flow = (nsBlockFrame*) flow->mNextInFlow;
      NS_ASSERTION(nsnull != flow, "whoops, continuation without a parent");
      prevLine = nsnull;
      line = flow->mLines;
      linep = &flow->mLines;
      prevSibling = nsnull;
    }
  }

#ifdef DEBUG
  VerifyLines(PR_TRUE);
#endif
  return NS_OK;
}

void
nsBlockFrame::DeleteChildsNextInFlow(nsIPresContext* aPresContext,
                                     nsIFrame* aChild)
{
  NS_PRECONDITION(IsChild(aPresContext, aChild), "bad geometric parent");
  nsIFrame* nextInFlow;
  aChild->GetNextInFlow(&nextInFlow);
  NS_PRECONDITION(nsnull != nextInFlow, "null next-in-flow");
  nsBlockFrame* parent;
  nextInFlow->GetParent((nsIFrame**)&parent);
  NS_PRECONDITION(nsnull != parent, "next-in-flow with no parent");
  NS_PRECONDITION(nsnull != parent->mLines, "next-in-flow with weird parent");
//  NS_PRECONDITION(nsnull == parent->mOverflowLines, "parent with overflow");
  parent->DoRemoveFrame(aPresContext, nextInFlow);
}

////////////////////////////////////////////////////////////////////////
// Floater support

nsresult
nsBlockFrame::ReflowFloater(nsBlockReflowState& aState,
                            nsPlaceholderFrame* aPlaceholder,
                            nsRect& aCombinedRect,
                            nsMargin& aMarginResult,
                            nsMargin& aComputedOffsetsResult)
{
  // XXX update this just
  aState.GetAvailableSpace();

  // Reflow the floater. Since floaters are continued we given them an
  // unbounded height. Floaters with an auto width are sized to zero
  // according to the css2 spec.
  nsRect availSpace(0, 0, aState.mAvailSpaceRect.width, NS_UNCONSTRAINEDSIZE);
  nsIFrame* floater = aPlaceholder->GetOutOfFlowFrame();
  PRBool isAdjacentWithTop = aState.IsAdjacentWithTop();

  // Setup block reflow state to reflow the floater
  nsBlockReflowContext brc(aState.mPresContext, aState.mReflowState,
                           aState.mComputeMaxElementSize,
                           aState.mComputeMaximumWidth);
  brc.SetNextRCFrame(aState.mNextRCFrame);

  // Reflow the floater
  nsReflowStatus frameReflowStatus;
  nsresult rv = brc.ReflowBlock(floater, availSpace, PR_TRUE, 0,
                                isAdjacentWithTop,
                                aComputedOffsetsResult, frameReflowStatus);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Capture the margin information for the caller
  const nsMargin& m = brc.GetMargin();
  aMarginResult.top = brc.GetTopMargin();
  aMarginResult.right = m.right;
  aMarginResult.bottom = 
    nsBlockReflowContext::MaxMargin(brc.GetCarriedOutBottomMargin(),
                                    m.bottom);
  aMarginResult.left = m.left;

  const nsHTMLReflowMetrics& metrics = brc.GetMetrics();
  aCombinedRect = metrics.mOverflowArea;
  // Set the rect, make sure the view is properly sized and positioned,
  // and tell the frame we're done reflowing it
  floater->SizeTo(aState.mPresContext, metrics.width, metrics.height);
  nsIView*  view;
  floater->GetView(aState.mPresContext, &view);
  if (view) {
    nsContainerFrame::SyncFrameViewAfterReflow(aState.mPresContext, floater, view,
                                               &metrics.mOverflowArea,
                                               NS_FRAME_NO_MOVE_VIEW);
  }
  floater->DidReflow(aState.mPresContext, NS_FRAME_REFLOW_FINISHED);

  // Stash away the max-element-size for later
  aState.StoreMaxElementSize(floater, brc.GetMaxElementSize());

  return NS_OK;
}

void
nsBlockReflowState::InitFloater(nsLineLayout& aLineLayout,
                                nsPlaceholderFrame* aPlaceholder)
{
  // Set the geometric parent of the floater
  nsIFrame* floater = aPlaceholder->GetOutOfFlowFrame();
  floater->SetParent(mBlock);

  // Then add the floater to the current line and place it when
  // appropriate
  AddFloater(aLineLayout, aPlaceholder, PR_TRUE);
}

// This is called by the line layout's AddFloater method when a
// place-holder frame is reflowed in a line. If the floater is a
// left-most child (it's x coordinate is at the line's left margin)
// then the floater is place immediately, otherwise the floater
// placement is deferred until the line has been reflowed.
void
nsBlockReflowState::AddFloater(nsLineLayout& aLineLayout,
                               nsPlaceholderFrame* aPlaceholder,
                               PRBool aInitialReflow)
{
  NS_PRECONDITION(nsnull != mCurrentLine, "null ptr");

  // Allocate a nsFloaterCache for the floater
  nsFloaterCache* fc = mFloaterCacheFreeList.Alloc();
  fc->mPlaceholder = aPlaceholder;
  fc->mIsCurrentLineFloater = aLineLayout.CanPlaceFloaterNow();

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

    // Reflow the floater
    mBlock->ReflowFloater(*this, aPlaceholder, fc->mCombinedArea,
                          fc->mMargins, fc->mOffsets);

    // And then place it
    PRBool isLeftFloater;
    PlaceFloater(fc, &isLeftFloater);

    // Pass on updated available space to the current inline reflow engine
    GetAvailableSpace();
    aLineLayout.UpdateBand(mAvailSpaceRect.x + BorderPadding().left, mY,
                           mAvailSpaceRect.width,
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

PRBool
nsBlockReflowState::IsLeftMostChild(nsIFrame* aFrame)
{
  for (;;) {
    nsIFrame* parent;
    aFrame->GetParent(&parent);
    if (parent == mBlock) {
      nsIFrame* child = mCurrentLine->mFirstChild;
      PRInt32 n = mCurrentLine->GetChildCount();
      while ((nsnull != child) && (aFrame != child) && (--n >= 0)) {
        nsSize  size;

        // Is the child zero-sized?
        child->GetSize(size);
        if (size.width > 0) {
          // We found a non-zero sized child frame that precedes aFrame
          return PR_FALSE;
        }
        child->GetNextSibling(&child);
      }
      break;
    }
    else {
      // See if there are any non-zero sized child frames that precede
      // aFrame in the child list
      nsIFrame* child;
      parent->FirstChild(nsnull, &child);
      while ((nsnull != child) && (aFrame != child)) {
        nsSize  size;

        // Is the child zero-sized?
        child->GetSize(size);
        if (size.width > 0) {
          // We found a non-zero sized child frame that precedes aFrame
          return PR_FALSE;
        }
        child->GetNextSibling(&child);
      }
    }
  
    // aFrame is the left-most non-zero sized frame in its geometric parent.
    // Walk up one level and check that its parent is left-most as well
    aFrame = parent;
  }
  return PR_TRUE;
}

PRBool
nsBlockReflowState::CanPlaceFloater(const nsRect& aFloaterRect,
                                    PRUint8 aFloats)
{
  // If the current Y coordinate is not impacted by any floaters
  // then by definition the floater fits.
  PRBool result = PR_TRUE;
  if (0 != mBand.GetFloaterCount()) {
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
          // CSS2 spec, 9.5.1 rule [4]: A floating box's outer top may not
          // be higher than the top of its containing block.

          // XXX It's not clear if it means the higher than the outer edge
          // or the border edge or the inner edge?
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
nsBlockReflowState::PlaceFloater(nsFloaterCache* aFloaterCache,
                                 PRBool* aIsLeftFloater)
{
  // Save away the Y coordinate before placing the floater. We will
  // restore mY at the end after placing the floater. This is
  // necessary because any adjustments to mY during the floater
  // placement are for the floater only, not for any non-floating
  // content.
  nscoord saveY = mY;
  nsIFrame* floater = aFloaterCache->mPlaceholder->GetOutOfFlowFrame();

  // Get the type of floater
  const nsStyleDisplay* floaterDisplay;
  const nsStyleSpacing* floaterSpacing;
  const nsStylePosition* floaterPosition;
  floater->GetStyleData(eStyleStruct_Display,
                        (const nsStyleStruct*&)floaterDisplay);
  floater->GetStyleData(eStyleStruct_Spacing,
                        (const nsStyleStruct*&)floaterSpacing);
  floater->GetStyleData(eStyleStruct_Position,
                        (const nsStyleStruct*&)floaterPosition);

  // See if the floater should clear any preceeding floaters...
  if (NS_STYLE_CLEAR_NONE != floaterDisplay->mBreakType) {
    ClearFloaters(mY, floaterDisplay->mBreakType);
  }
  else {
    // Get the band of available space
    GetAvailableSpace();
  }

  // Get the floaters bounding box and margin information
  nsRect region;
  floater->GetRect(region);

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

  // While there is not enough room for the floater, clear past other
  // floaters until there is room (or the band is not impacted by a
  // floater).
  // 
  // Note: The CSS2 spec says that floaters should be placed as high
  // as possible.
  while (!CanPlaceFloater(region, floaterDisplay->mFloats)) {
    mY += mAvailSpaceRect.height;
    GetAvailableSpace();
  }

  // Assign an x and y coordinate to the floater. Note that the x,y
  // coordinates are computed <b>relative to the translation in the
  // spacemanager</b> which means that the impacted region will be
  // <b>inside</b> the border/padding area.
  PRBool okToAddRectRegion = PR_TRUE;
  PRBool isLeftFloater;
  if (NS_STYLE_FLOAT_LEFT == floaterDisplay->mFloats) {
    isLeftFloater = PR_TRUE;
    region.x = mAvailSpaceRect.x;
  }
  else {
    isLeftFloater = PR_FALSE;
    region.x = mAvailSpaceRect.XMost() - region.width;
  }
  *aIsLeftFloater = isLeftFloater;
  const nsMargin& borderPadding = BorderPadding();
  region.y = mY - borderPadding.top;
  if (region.y < 0) {
    // CSS2 spec, 9.5.1 rule [4]: A floating box's outer top may not
    // be higher than the top of its containing block.

    // XXX It's not clear if it means the higher than the outer edge
    // or the border edge or the inner edge?
    region.y = 0;
  }

  // Place the floater in the space manager
  if (okToAddRectRegion) {
#ifdef DEBUG
    nsresult rv =
#endif
    mSpaceManager->AddRectRegion(floater, region);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad floater placement");
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
  printf(": PlaceFloater: AddRectRegion: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
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
  if (NS_STYLE_POSITION_RELATIVE == floaterPosition->mPosition) {
    x += aFloaterCache->mOffsets.left;
    y += aFloaterCache->mOffsets.top;
  }

  // Position the floater and make sure and views are properly positioned
  nsIView*  view;
  floater->MoveTo(mPresContext, x, y);
  floater->GetView(mPresContext, &view);
  if (view) {
    nsContainerFrame::PositionFrameView(mPresContext, floater, view);
  } else {
    nsContainerFrame::PositionChildViews(mPresContext, floater);
  }

  // Update the floater combined area state
  nsRect combinedArea = aFloaterCache->mCombinedArea;
  combinedArea.x += x;
  combinedArea.y += y;
  if (!isLeftFloater && (mUnconstrainedWidth || mShrinkWrapWidth)) {
    // When we are placing a right floater in an unconstrained situation or
    // when shrink wrapping, we don't apply it to the floater combined area
    // immediately. Otherwise we end up with an infinitely wide combined
    // area. Instead, we save it away in mRightFloaterCombinedArea so that
    // later on when we know the width of a line we can compute a better value.
    if (!mHaveRightFloaters) {
      mRightFloaterCombinedArea = combinedArea;
      mHaveRightFloaters = PR_TRUE;
    }
    else {
      CombineRects(combinedArea, mRightFloaterCombinedArea);
    }
  }
  else {
    CombineRects(combinedArea, mFloaterCombinedArea);
  }

  // Now restore mY
  mY = saveY;

#ifdef DEBUG
  if (gNoisyReflow) {
    nsRect r;
    floater->GetRect(r);
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("placed floater: ");
    ((nsFrame*)floater)->ListTag(stdout);
    printf(" %d,%d,%d,%d\n", r.x, r.y, r.width, r.height);
  }
#endif
}

/**
 * Place below-current-line floaters.
 */
void
nsBlockReflowState::PlaceBelowCurrentLineFloaters(nsFloaterCacheList& aList)
{
  nsFloaterCache* fc = aList.Head();
  while (fc) {
    if (!fc->mIsCurrentLineFloater) {
#ifdef DEBUG
      if (gNoisyReflow) {
        nsFrame::IndentBy(stdout, gNoiseIndent);
        printf("placing bcl floater: ");
        nsFrame::ListTag(stdout, fc->mPlaceholder->GetOutOfFlowFrame());
        printf("\n");
      }
#endif
      mBlock->ReflowFloater(*this, fc->mPlaceholder, fc->mCombinedArea,
                            fc->mMargins, fc->mOffsets);

      // Place the floater
      PRBool isLeftFloater;
      PlaceFloater(fc, &isLeftFloater);
    }
    fc = fc->Next();
  }
}

void
nsBlockReflowState::ClearFloaters(nscoord aY, PRUint8 aBreakType)
{
#ifdef DEBUG
  if (gNoisyReflow) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("clear floaters: in: mY=%d aY=%d(%d)\n",
           mY, aY, aY - BorderPadding().top);
  }
#endif

#ifdef NOISY_FLOATER_CLEARING
  printf("nsBlockReflowState::ClearFloaters: aY=%d breakType=%dn",
         aY, aBreakType);
  mSpaceManager->List(stdout);
#endif
  const nsMargin& bp = BorderPadding();
  nscoord newY = mBand.ClearFloaters(aY - bp.top, aBreakType);
  mY = newY + bp.top;
  GetAvailableSpace();

#ifdef DEBUG
  if (gNoisyReflow) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("clear floaters: out: mY=%d(%d)\n", mY, mY - bp.top);
  }
#endif
}

//////////////////////////////////////////////////////////////////////
// Painting, event handling

PRIntn
nsBlockFrame::GetSkipSides() const
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

#ifdef DEBUG
static void ComputeCombinedArea(nsLineBox* aLine,
                                nscoord aWidth, nscoord aHeight,
                                nsRect& aResult)
{
  nscoord xa = 0, ya = 0, xb = aWidth, yb = aHeight;
  while (nsnull != aLine) {
    // Compute min and max x/y values for the reflowed frame's
    // combined areas
    nsRect lineCombinedArea;
    aLine->GetCombinedArea(&lineCombinedArea);
    nscoord x = lineCombinedArea.x;
    nscoord y = lineCombinedArea.y;
    nscoord xmost = x + lineCombinedArea.width;
    nscoord ymost = y + lineCombinedArea.height;
    if (x < xa) {
      xa = x;
    }
    if (xmost > xb) {
      xb = xmost;
    }
    if (y < ya) {
      ya = y;
    }
    if (ymost > yb) {
      yb = ymost;
    }
    aLine = aLine->mNext;
  }

  aResult.x = xa;
  aResult.y = ya;
  aResult.width = xb - xa;
  aResult.height = yb - ya;
}
#endif

NS_IMETHODIMP
nsBlockFrame::Paint(nsIPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer)
{
  if (NS_FRAME_IS_UNFLOWABLE & mState) {
    return NS_OK;
  }

#ifdef DEBUG
  if (gNoisyDamageRepair) {
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
      PRInt32 depth = GetDepth();
      nsRect ca;
      ComputeCombinedArea(mLines, mRect.width, mRect.height, ca);
      nsFrame::IndentBy(stdout, depth);
      ListTag(stdout);
      printf(": bounds=%d,%d,%d,%d dirty=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
             mRect.x, mRect.y, mRect.width, mRect.height,
             aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height,
             ca.x, ca.y, ca.width, ca.height);
    }
  }
#endif  

  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    mStyleContext->GetStyleData(eStyleStruct_Display);

  // If overflow is hidden then set the clip rect so that children
  // don't leak out of us
  if (NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) {
    aRenderingContext.PushState();
    SetClipRect(aRenderingContext);
  }

  // Only paint the border and background if we're visible
  if (disp->mVisible && (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) &&
      (0 != mRect.width) && (0 != mRect.height)) {
    PRIntn skipSides = GetSkipSides();
    const nsStyleColor* color = (const nsStyleColor*)
      mStyleContext->GetStyleData(eStyleStruct_Color);
    const nsStyleSpacing* spacing = (const nsStyleSpacing*)
      mStyleContext->GetStyleData(eStyleStruct_Spacing);

    // Paint background, border and outline
    nsRect rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *color, *spacing, 0, 0);
    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                aDirtyRect, rect, *spacing, mStyleContext,
                                skipSides);
    nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                                 aDirtyRect, rect, *spacing, mStyleContext, 0);
  }

  // Child elements have the opportunity to override the visibility
  // property and display even if the parent is hidden
  if (NS_FRAME_PAINT_LAYER_FLOATERS == aWhichLayer) {
    PaintFloaters(aPresContext, aRenderingContext, aDirtyRect);
  }
  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  if (NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) {
    PRBool clipState;
    aRenderingContext.PopState(clipState);
  }

#if 0
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    // Render the bands in the spacemanager
    nsISpaceManager* sm = mSpaceManager;

    if (nsnull != sm) {
      nsBlockBandData band;
      band.Init(sm, nsSize(mRect.width, mRect.height));
      nscoord y = 0;
      while (y < mRect.height) {
        nsRect availArea;
        band.GetAvailableSpace(y, availArea);
  
        // Render a box and a diagonal line through the band
        aRenderingContext.SetColor(NS_RGB(0,255,0));
        aRenderingContext.DrawRect(0, availArea.y,
                                   mRect.width, availArea.height);
        aRenderingContext.DrawLine(0, availArea.y,
                                   mRect.width, availArea.YMost());
  
        // Render boxes and opposite diagonal lines around the
        // unavailable parts of the band.
        PRInt32 i;
        for (i = 0; i < band.GetTrapezoidCount(); i++) {
          const nsBandTrapezoid* trapezoid = band.GetTrapezoid(i);
          if (nsBandTrapezoid::Available != trapezoid->mState) {
            nsRect r;
            trapezoid->GetRect(r);
            if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
              aRenderingContext.SetColor(NS_RGB(0,255,128));
            }
            else {
              aRenderingContext.SetColor(NS_RGB(128,255,0));
            }
            aRenderingContext.DrawRect(r);
            aRenderingContext.DrawLine(r.x, r.YMost(), r.XMost(), r.y);
          }
        }
        y = availArea.YMost();
      }
    }
  }
#endif

  return NS_OK;
}

void
nsBlockFrame::PaintFloaters(nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect)
{
  for (nsLineBox* line = mLines; nsnull != line; line = line->mNext) {
    if (!line->HasFloaters()) {
      continue;
    }
    nsFloaterCache* fc = line->GetFirstFloater();
    while (fc) {
      nsIFrame* floater = fc->mPlaceholder->GetOutOfFlowFrame();
      PaintChild(aPresContext, aRenderingContext, aDirtyRect,
                 floater, NS_FRAME_PAINT_LAYER_BACKGROUND);
      PaintChild(aPresContext, aRenderingContext, aDirtyRect,
                 floater, NS_FRAME_PAINT_LAYER_FLOATERS);
      PaintChild(aPresContext, aRenderingContext, aDirtyRect,
                 floater, NS_FRAME_PAINT_LAYER_FOREGROUND);
      fc = fc->Next();
    }
  }
}

void
nsBlockFrame::PaintChildren(nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect,
                            nsFramePaintLayer aWhichLayer)
{
#ifdef DEBUG
  PRInt32 depth = 0;
  if (gNoisyDamageRepair) {
    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
      depth = GetDepth();
    }
  }
  PRTime start;
  PRInt32 drawnLines;
  if (gLamePaintMetrics) {
    start = PR_Now();
    drawnLines = 0;
  }
#endif

  for (nsLineBox* line = mLines; nsnull != line; line = line->mNext) {
    // If the line's combined area (which includes child frames that
    // stick outside of the line's bounding box or our bounding box)
    // intersects the dirty rect then paint the line.
    if (line->CombinedAreaIntersects(aDirtyRect)) {
#ifdef DEBUG
      if (gNoisyDamageRepair &&
          (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)) {
        nsRect lineCombinedArea;
        line->GetCombinedArea(&lineCombinedArea);
        nsFrame::IndentBy(stdout, depth+1);
        printf("draw line=%p bounds=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
               line, line->mBounds.x, line->mBounds.y,
               line->mBounds.width, line->mBounds.height,
               lineCombinedArea.x, lineCombinedArea.y,
               lineCombinedArea.width, lineCombinedArea.height);
      }
      if (gLamePaintMetrics) {
        drawnLines++;
      }
#endif
      nsIFrame* kid = line->mFirstChild;
      PRInt32 n = line->GetChildCount();
      while (--n >= 0) {
        PaintChild(aPresContext, aRenderingContext, aDirtyRect, kid,
                   aWhichLayer);
        kid->GetNextSibling(&kid);
      }
    }
#ifdef DEBUG
    else {
      if (gNoisyDamageRepair &&
          (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)) {
        nsRect lineCombinedArea;
        line->GetCombinedArea(&lineCombinedArea);
        nsFrame::IndentBy(stdout, depth+1);
        printf("skip line=%p bounds=%d,%d,%d,%d ca=%d,%d,%d,%d\n",
               line, line->mBounds.x, line->mBounds.y,
               line->mBounds.width, line->mBounds.height,
               lineCombinedArea.x, lineCombinedArea.y,
               lineCombinedArea.width, lineCombinedArea.height);
      }
    }
#endif  
  }

  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
    if ((nsnull != mBullet) && HaveOutsideBullet()) {
      // Paint outside bullets manually
      PaintChild(aPresContext, aRenderingContext, aDirtyRect, mBullet,
                 aWhichLayer);
    }
  }

#ifdef DEBUG
  if (gLamePaintMetrics) {
    PRTime end = PR_Now();

    PRInt32 numLines = nsLineBox::ListLength(mLines);
    if (!numLines) numLines = 1;
    PRTime lines, deltaPerLine, delta;
    LL_I2L(lines, numLines);
    LL_SUB(delta, end, start);
    LL_DIV(deltaPerLine, delta, lines);

    ListTag(stdout);
    char buf[400];
    PR_snprintf(buf, sizeof(buf),
                ": %lld elapsed (%lld per line) lines=%d drawn=%d skip=%d",
                delta, deltaPerLine,
                numLines, drawnLines, numLines - drawnLines);
    printf("%s\n", buf);
  }
#endif
}


NS_IMETHODIMP
nsBlockFrame::HandleEvent(nsIPresContext* aPresContext, 
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus)
{
  if (aEvent->message == NS_MOUSE_MOVE) {
    nsCOMPtr<nsIPresShell> shell;
    nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_SUCCEEDED(rv)){
      nsCOMPtr<nsIFrameSelection> frameselection;
      if (NS_SUCCEEDED(shell->GetFrameSelection(getter_AddRefs(frameselection))) && frameselection){
          PRBool mouseDown = PR_FALSE;
          if (NS_FAILED(frameselection->GetMouseDownState(&mouseDown)) || !mouseDown) 
            return NS_OK;//do not handle
      }
    }
  }

  if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN || aEvent->message == NS_MOUSE_MOVE ||
    aEvent->message == NS_MOUSE_LEFT_DOUBLECLICK ) {
    nsresult result;
    nsIFrame *resultFrame = nsnull;//this will be passed the handle event when we 
                                   //can tell who to pass it to
    nsCOMPtr<nsILineIterator> it; 
    nsIFrame *mainframe = this;
    nsCOMPtr<nsIPresShell> shell;
    aPresContext->GetShell(getter_AddRefs(shell));
    if (!shell)
      return NS_OK;
    nsCOMPtr<nsIFocusTracker> tracker;
    result = shell->QueryInterface(nsIFocusTracker::GetIID(),getter_AddRefs(tracker));

    result = mainframe->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));
    nsIView* parentWithView;
    nsPoint origin;

    while(NS_SUCCEEDED(result))
    { //we are starting aloop to allow us to "drill down to the one we want" 
      mainframe->GetOffsetFromView(aPresContext, origin, &parentWithView);

      if (NS_FAILED(result))
        return NS_OK;//do not handle
      PRInt32 countLines;
      result = it->GetNumLines(&countLines);
      if (NS_FAILED(result))
        return NS_OK;//do not handle
      PRInt32 i;
      PRInt32 lineFrameCount;
      nsIFrame *firstFrame;
      nsRect  rect;
      PRInt32 closestLine = 0;
      PRInt32 closestDistance = 999999; //some HUGE number that will always fail first comparison
      //incase we hit another block frame.
      for (i = 0; i< countLines;i++)
      {
        PRUint32 flags;
        result = it->GetLine(i, &firstFrame, &lineFrameCount,rect,&flags);
        if (NS_FAILED(result))
          continue;//do not handle
        rect+=origin;
        rect.width = aEvent->point.x - rect.x+1;//EXTEND RECT TO REACH POINT
        if (rect.Contains(aEvent->point.x, aEvent->point.y))
        {
          closestLine = i;
          break;
        }
        else
        {
          PRInt32 distance = PR_MIN(abs(rect.y - aEvent->point.y),abs((rect.y + rect.height) - aEvent->point.y));
          if (distance < closestDistance)
          {
            closestDistance = distance;
            closestLine = i;
          }
          else if (distance > closestDistance)
            break;//done
        }
      }
      //we will now ask where to go. if we cant find what we want"aka another block frame" 
      //we drill down again
      nsPeekOffsetStruct pos;
      pos.mTracker = tracker;
      pos.mDirection = eDirNext;
      pos.mDesiredX = aEvent->point.x;
      
      result = nsFrame::GetNextPrevLineFromeBlockFrame(aPresContext,
                                          &pos,
                                          mainframe, 
                                          closestLine-1, 
                                          0
                                          );
      
      if (NS_SUCCEEDED(result) && pos.mResultFrame){
        result = pos.mResultFrame->QueryInterface(nsILineIterator::GetIID(),getter_AddRefs(it));//if this fails thats ok
        resultFrame = pos.mResultFrame;
        mainframe = resultFrame;
      }
      else
        break;//time to go nothing was found
    }
    //end while loop. if nssucceeded resutl then keep going that means
    //we have successfully hit another block frame and we should keep going.


    if (resultFrame)
      return resultFrame->HandleEvent(aPresContext, aEvent, aEventStatus);
    else
      return NS_OK; //just stop it
  }
  return NS_OK;
}


NS_IMETHODIMP
nsBlockFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                               const nsPoint& aPoint,
                               nsIFrame** aFrame)
{
  nsresult rv = GetFrameForPointUsing(aPresContext, aPoint, nsnull, aFrame);
  if (NS_OK == rv) {
    return NS_OK;
  }
  if (nsnull != mBullet) {
    rv = GetFrameForPointUsing(aPresContext, aPoint, nsLayoutAtoms::bulletList, aFrame);
    if (NS_OK == rv) {
      return NS_OK;
    }
  }
  if (mFloaters.NotEmpty()) {
    rv = GetFrameForPointUsing(aPresContext, aPoint, nsLayoutAtoms::floaterList, aFrame);
    if (NS_OK == rv) {
      return NS_OK;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::ReflowDirtyChild(nsIPresShell* aPresShell, nsIFrame* aChild)
{  
  // Mark the line containing the child frame dirty.
  if (aChild) {    
    PRBool isFloater;
    nsLineBox* prevLine;
    nsLineBox* line = FindLineFor(aChild, &prevLine, &isFloater);
    
    if (!isFloater) {
      if (line) MarkLineDirty(line, prevLine);
    }
    else {
      line = mLines;
      while (nsnull != line) {
        line->MarkDirty();
        line = line->mNext;
      }
#ifdef DEBUG_nisheeth      
      NS_ASSERTION(0, "nsBlockFrame::ReflowDirtyChild: Marked all lines dirty.");
#endif
    }
  }

  // Either generate a reflow command to reflow the dirty child or 
  // coalesce this reflow request with an existing reflow command    
  if (!(mState & NS_FRAME_HAS_DIRTY_CHILDREN)) {
    // If this is the first dirty child, 
    // post a dirty children reflow command targeted at yourself
    mState |= NS_FRAME_HAS_DIRTY_CHILDREN;

    nsFrame::CreateAndPostReflowCommand(aPresShell, this, 
      nsIReflowCommand::ReflowDirty, nsnull, nsnull, nsnull);
  }
  else {
    if (!(mState & NS_FRAME_IS_DIRTY)) {
      // If the parent frame is not an area frame
      nsCOMPtr<nsIAtom> frameType;
      mParent->GetFrameType(getter_AddRefs(frameType));      
      if (frameType.get() != nsLayoutAtoms::areaFrame) {
        // Mark yourself as dirty
        mState |= NS_FRAME_IS_DIRTY;

        // Cancel the dirty children reflow command you posted earlier
        nsIReflowCommand::ReflowType type = nsIReflowCommand::ReflowDirty;
        aPresShell->CancelReflowCommand(this, &type);

        // Pass up the reflow request to the parent frame.
        mParent->ReflowDirtyChild(aPresShell, this);
      }
    }
  }
  
  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
// Debugging

#ifdef NS_DEBUG
static PRBool
InLineList(nsLineBox* aLines, nsIFrame* aFrame)
{
  while (nsnull != aLines) {
    nsIFrame* frame = aLines->mFirstChild;
    PRInt32 n = aLines->GetChildCount();
    while (--n >= 0) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame->GetNextSibling(&frame);
    }
    aLines = aLines->mNext;
  }
  return PR_FALSE;
}

static PRBool
InSiblingList(nsLineBox* aLine, nsIFrame* aFrame)
{
  if (nsnull != aLine) {
    nsIFrame* frame = aLine->mFirstChild;
    while (nsnull != frame) {
      if (frame == aFrame) {
        return PR_TRUE;
      }
      frame->GetNextSibling(&frame);
    }
  }
  return PR_FALSE;
}

PRBool
nsBlockFrame::IsChild(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  nsIFrame* parent;
  aFrame->GetParent(&parent);
  if (parent != (nsIFrame*)this) {
    return PR_FALSE;
  }
  if (InLineList(mLines, aFrame) && InSiblingList(mLines, aFrame)) {
    return PR_TRUE;
  }
  nsLineBox* overflowLines = GetOverflowLines(aPresContext, PR_FALSE);
  if (InLineList(overflowLines, aFrame) && InSiblingList(overflowLines, aFrame)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsBlockFrame::VerifyTree() const
{
  // XXX rewrite this
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aHandler || !aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  PRUint32 sum = sizeof(*this);

  // Add in size of each line object
  nsLineBox* line = mLines;
  while (line) {
    PRUint32  lineBoxSize;
    nsIAtom* atom = line->SizeOf(aHandler, &lineBoxSize);
    aHandler->AddSize(atom, lineBoxSize);
    line = line->mNext;
  }

  // Add in text-run data
  nsTextRun* runs = mTextRuns;
  while (runs) {
    PRUint32 runSize;
    runs->SizeOf(aHandler, &runSize);
    aHandler->AddSize(nsLayoutAtoms::textRun, runSize);
    runs = runs->GetNext();
  }

  *aResult = sum;
  return NS_OK;
}
#endif

//----------------------------------------------------------------------

NS_IMETHODIMP
nsBlockFrame::Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow)
{
  if (aPrevInFlow) {
    // Copy over the block/area frame type flags
    nsBlockFrame*  blockFrame = (nsBlockFrame*)aPrevInFlow;

    SetFlags(blockFrame->mState & NS_BLOCK_FLAGS_MASK);
  }

  nsresult rv = nsBlockFrameSuper::Init(aPresContext, aContent, aParent,
                                        aContext, aPrevInFlow);
  if (nsBlockReflowContext::IsHTMLParagraph(this)) {
    mState |= NS_BLOCK_IS_HTML_PARAGRAPH;
  }
  return rv;
}

nsIStyleContext*
nsBlockFrame::GetFirstLetterStyle(nsIPresContext* aPresContext)
{
  nsIStyleContext* fls;
  aPresContext->ProbePseudoStyleContextFor(mContent,
                                           nsHTMLAtoms::firstLetterPseudo,
                                           mStyleContext, PR_FALSE, &fls);
  return fls;
}

NS_IMETHODIMP
nsBlockFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;

  if (nsLayoutAtoms::floaterList == aListName) {
    mFloaters.SetFrames(aChildList);
  }
  else {

    // Lookup up the two pseudo style contexts
    if (nsnull == mPrevInFlow) {
      nsIStyleContext* firstLetterStyle = GetFirstLetterStyle(aPresContext);
      if (nsnull != firstLetterStyle) {
        mState |= NS_BLOCK_HAS_FIRST_LETTER_STYLE;
#ifdef NOISY_FIRST_LETTER
        ListTag(stdout);
        printf(": first-letter style found\n");
#endif
        NS_RELEASE(firstLetterStyle);
      }
    }

    rv = AddFrames(aPresContext, aChildList, nsnull);
    if (NS_FAILED(rv)) {
      return rv;
    }

    // Create list bullet if this is a list-item. Note that this is done
    // here so that RenumberLists will work (it needs the bullets to
    // store the bullet numbers).
    const nsStyleDisplay* styleDisplay;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) styleDisplay);
    if ((nsnull == mPrevInFlow) &&
        (NS_STYLE_DISPLAY_LIST_ITEM == styleDisplay->mDisplay) &&
        (nsnull == mBullet)) {
      // Resolve style for the bullet frame
      nsIStyleContext* kidSC;
      aPresContext->ResolvePseudoStyleContextFor(mContent, 
                                             nsHTMLAtoms::mozListBulletPseudo,
                                             mStyleContext, PR_FALSE, &kidSC);

      // Create bullet frame
      nsCOMPtr<nsIPresShell> shell;
      aPresContext->GetShell(getter_AddRefs(shell));
      mBullet = new (shell.get()) nsBulletFrame;

      if (nsnull == mBullet) {
        NS_RELEASE(kidSC);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mBullet->Init(aPresContext, mContent, this, kidSC, nsnull);
      NS_RELEASE(kidSC);

      // If the list bullet frame should be positioned inside then add
      // it to the flow now.
      const nsStyleList* styleList;
      GetStyleData(eStyleStruct_List, (const nsStyleStruct*&) styleList);
      if (NS_STYLE_LIST_STYLE_POSITION_INSIDE ==
          styleList->mListStylePosition) {
        AddFrames(aPresContext, mBullet, nsnull);
        mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }
      else {
        mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
      }
    }
  }

  return NS_OK;
}

PRBool
nsBlockFrame::FrameStartsCounterScope(nsIFrame* aFrame)
{
  const nsStyleContent* styleContent;
  aFrame->GetStyleData(eStyleStruct_Content,
                       (const nsStyleStruct*&) styleContent);
  if (0 != styleContent->CounterResetCount()) {
    // Winner
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsBlockFrame::RenumberLists()
{
  if (!FrameStartsCounterScope(this)) {
    // If this frame doesn't start a counter scope then we don't need
    // to renumber child list items.
    return;
  }

  // Setup initial list ordinal value
  // XXX Map html's start property to counter-reset style
  PRInt32 ordinal = 1;
  nsIHTMLContent* hc;
  if (mContent && (NS_OK == mContent->QueryInterface(kIHTMLContentIID, (void**) &hc))) {
    nsHTMLValue value;
    if (NS_CONTENT_ATTR_HAS_VALUE ==
        hc->GetHTMLAttribute(nsHTMLAtoms::start, value)) {
      if (eHTMLUnit_Integer == value.GetUnit()) {
        ordinal = value.GetIntValue();
        if (ordinal <= 0) {
          ordinal = 1;
        }
      }
    }
    NS_RELEASE(hc);
  }

  // Get to first-in-flow
  nsBlockFrame* block = (nsBlockFrame*) GetFirstInFlow();
  RenumberListsInBlock(block, &ordinal);
}

PRBool
nsBlockFrame::RenumberListsInBlock(nsBlockFrame* aBlockFrame,
                                   PRInt32* aOrdinal)
{
  PRBool renumberedABullet = PR_FALSE;

  while (nsnull != aBlockFrame) {
    // Examine each line in the block
    nsLineBox* line = aBlockFrame->mLines;
    while (line) {
      nsIFrame* kid = line->mFirstChild;
      PRInt32 n = line->GetChildCount();
      while (--n >= 0) {
        PRBool kidRenumberedABullet = RenumberListsFor(kid, aOrdinal);
        if (kidRenumberedABullet) {
          line->MarkDirty();
          renumberedABullet = PR_TRUE;
        }
        kid->GetNextSibling(&kid);
      }
      line = line->mNext;
    }

    // Advance to the next continuation
    aBlockFrame->GetNextInFlow((nsIFrame**) &aBlockFrame);
  }

  return renumberedABullet;
}

// XXX temporary code: after ib work is done in frame construction
// code this can be removed.
PRBool
nsBlockFrame::RenumberListsIn(nsIFrame* aContainerFrame, PRInt32* aOrdinal)
{
  PRBool renumberedABullet = PR_FALSE;

  // For each flow-block...
  while (nsnull != aContainerFrame) {
    // For each frame in the flow-block...
    nsIFrame* kid;
    aContainerFrame->FirstChild(nsnull, &kid);
    while (nsnull != kid) {
      PRBool kidRenumberedABullet = RenumberListsFor(kid, aOrdinal);
      if (kidRenumberedABullet) {
        renumberedABullet = PR_TRUE;
      }
      kid->GetNextSibling(&kid);
    }
    aContainerFrame->GetNextInFlow(&aContainerFrame);
  }
  return renumberedABullet;
}

PRBool
nsBlockFrame::RenumberListsFor(nsIFrame* aKid, PRInt32* aOrdinal)
{
  PRBool kidRenumberedABullet = PR_FALSE;

  // If the frame is a list-item and the frame implements our
  // block frame API then get it's bullet and set the list item
  // ordinal.
  const nsStyleDisplay* display;
  aKid->GetStyleData(eStyleStruct_Display,
                    (const nsStyleStruct*&) display);
  if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
    // Make certain that the frame isa block-frame in case
    // something foreign has crept in.
    nsBlockFrame* listItem;
    nsresult rv = aKid->QueryInterface(kBlockFrameCID, (void**)&listItem);
    if (NS_SUCCEEDED(rv)) {
      if (nsnull != listItem->mBullet) {
        PRBool changed;
        *aOrdinal = listItem->mBullet->SetListItemOrdinal(*aOrdinal,
                                                          &changed);
        if (changed) {
          kidRenumberedABullet = PR_TRUE;
        }
      }

      // XXX temporary? if the list-item has child list-items they
      // should be numbered too; especially since the list-item is
      // itself (ASSUMED!) not to be a counter-reseter.
      PRBool meToo = RenumberListsInBlock(listItem, aOrdinal);
      if (meToo) {
        kidRenumberedABullet = PR_TRUE;
      }
    }
  }
  else if (NS_STYLE_DISPLAY_BLOCK == display->mDisplay) {
    if (FrameStartsCounterScope(aKid)) {
      // Don't bother recursing into a block frame that is a new
      // counter scope. Any list-items in there will be handled by
      // it.
    }
    else {
      // If the display=block element ISA block-frame then go
      // ahead and recurse into it as it might have child
      // list-items.
      nsBlockFrame* kidBlock;
      nsresult rv = aKid->QueryInterface(kBlockFrameCID, (void**) &kidBlock);
      if (NS_SUCCEEDED(rv)) {
        kidRenumberedABullet = RenumberListsInBlock(kidBlock, aOrdinal);
      }
    }
  } else if (NS_STYLE_DISPLAY_INLINE == display->mDisplay) {
    // XXX temporary code: after ib work is done in frame construction
    // code this can be removed.

    // If the display=inline element ISA nsInlineFrame then go
    // ahead and recurse into it as it might have child
    // list-items.
    nsInlineFrame* kidInline;
    nsresult rv = aKid->QueryInterface(nsInlineFrame::kInlineFrameCID,
                                       (void**) &kidInline);
    if (NS_SUCCEEDED(rv)) {
      kidRenumberedABullet = RenumberListsIn(aKid, aOrdinal);
    }
  }
  return kidRenumberedABullet;
}

void
nsBlockFrame::ReflowBullet(nsBlockReflowState& aState,
                           nsHTMLReflowMetrics& aMetrics)
{
  // Reflow the bullet now
  nsSize availSize;
  availSize.width = NS_UNCONSTRAINEDSIZE;
  availSize.height = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState reflowState(aState.mPresContext, aState.mReflowState,
                                mBullet, availSize);
  nsReflowStatus  status;
  mBullet->WillReflow(aState.mPresContext);
  mBullet->Reflow(aState.mPresContext, aMetrics, reflowState, status);

  // Place the bullet now; use its right margin to distance it
  // from the rest of the frames in the line
  nscoord x = - reflowState.mComputedMargin.right - aMetrics.width;

  // Approximate the bullets position; vertical alignment will provide
  // the final vertical location.
  const nsMargin& bp = aState.BorderPadding();
  nscoord y = bp.top;
  mBullet->SetRect(aState.mPresContext, nsRect(x, y, aMetrics.width, aMetrics.height));
  mBullet->DidReflow(aState.mPresContext, NS_FRAME_REFLOW_FINISHED);
}

//XXX get rid of this -- its slow
void
nsBlockFrame::BuildFloaterList()
{
  nsIFrame* head = nsnull;
  nsIFrame* current = nsnull;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (line->HasFloaters()) {
      nsFloaterCache* fc = line->GetFirstFloater();
      while (fc) {
        nsIFrame* floater = fc->mPlaceholder->GetOutOfFlowFrame();
        if (nsnull == head) {
          current = head = floater;
        }
        else {
          current->SetNextSibling(floater);
          current = floater;
        }
        fc = fc->Next();
      }
    }
    line = line->mNext;
  }

  // Terminate end of floater list just in case a floater was removed
  if (nsnull != current) {
    current->SetNextSibling(nsnull);
  }
  mFloaters.SetFrames(head);
}

// XXX keep the text-run data in the first-in-flow of the block

// XXX Switch to an interface to pass to child frames -or- do the
// grovelling directly ourselves?
nsresult
nsBlockFrame::ComputeTextRuns(nsIPresContext* aPresContext)
{
  // Destroy old run information first
  nsTextRun::DeleteTextRuns(mTextRuns);
  mTextRuns = nsnull;

  nsLineLayout textRunThingy(aPresContext);

  // Ask each child to find its text runs
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (!line->IsBlock()) {
      nsIFrame* frame = line->mFirstChild;
      PRInt32 n = line->GetChildCount();
      while (--n >= 0) {
        frame->FindTextRuns(textRunThingy);
        frame->GetNextSibling(&frame);
      }
    }
    else {
      // A block frame isn't text therefore it will end an open text
      // run.
      textRunThingy.EndTextRun();
    }
    line = line->mNext;
  }
  textRunThingy.EndTextRun();

  // Now take the text-runs away from the line layout engine.
  mTextRuns = textRunThingy.TakeTextRuns();
  return NS_OK;
}

#ifdef DEBUG
void
nsBlockFrame::VerifyLines(PRBool aFinalCheckOK)
{
  if (!gVerifyLines) {
    return;
  }
  nsLineBox* line = mLines;
  if (!line) {
    return;
  }

  // Add up the counts on each line. Also validate that IsFirstLine is
  // set properly.
  PRInt32 count = 0;
  PRBool seenBlock = PR_FALSE;
  while (nsnull != line) {
    if (aFinalCheckOK) {
      NS_ABORT_IF_FALSE(line->GetChildCount(), "empty line");
      if (line->IsBlock()) {
        seenBlock = PR_TRUE;
      }
      if (line->IsBlock()) {
        NS_ASSERTION(1 == line->GetChildCount(), "bad first line");
      }
    }
    count += line->GetChildCount();
    line = line->mNext;
  }

  // Then count the frames
  PRInt32 frameCount = 0;
  nsIFrame* frame = mLines->mFirstChild;
  while (nsnull != frame) {
    frameCount++;
    frame->GetNextSibling(&frame);
  }
  NS_ASSERTION(count == frameCount, "bad line list");

  // Next: test that each line has right number of frames on it
  line = mLines;
  nsLineBox* prevLine = nsnull;
  while (nsnull != line) {
    count = line->GetChildCount();
    frame = line->mFirstChild;
    while (--count >= 0) {
      frame->GetNextSibling(&frame);
    }
    prevLine = line;
    line = line->mNext;
    if ((nsnull != line) && (0 != line->GetChildCount())) {
      NS_ASSERTION(frame == line->mFirstChild, "bad line list");
    }
  }
}

// Its possible that a frame can have some frames on an overflow
// list. But its never possible for multiple frames to have overflow
// lists. Check that this fact is actually true.
void
nsBlockFrame::VerifyOverflowSituation(nsIPresContext* aPresContext)
{
  nsBlockFrame* flow = (nsBlockFrame*) GetFirstInFlow();
  while (nsnull != flow) {
    nsLineBox* overflowLines = GetOverflowLines(aPresContext, PR_FALSE);
    if (nsnull != overflowLines) {
      NS_ASSERTION(nsnull != overflowLines->mFirstChild,
                   "bad overflow list");
    }
    flow = (nsBlockFrame*) flow->mNextInFlow;
  }
}

PRInt32
nsBlockFrame::GetDepth() const
{
  PRInt32 depth = 0;
  nsIFrame* parent = mParent;
  while (nsnull != parent) {
    parent->GetParent(&parent);
    depth++;
  }
  return depth;
}
#endif
