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
 *   Steve Clark <buster@netscape.com>
 *   Robert O'Callahan <roc+moz@cs.cmu.edu>
 *   L. David Baron <dbaron@fas.harvard.edu>
 */

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
    GetAvailableSpace(mY);
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

  PRBool IsLeftMostChild(nsIPresContext* aPresContext, nsIFrame* aFrame);

  PRBool IsAdjacentWithTop() const {
    return mY == mReflowState.mComputedBorderPadding.top;
  }

  const nsMargin& BorderPadding() const {
    return mReflowState.mComputedBorderPadding;
  }

  const nsMargin& Margin() const {
    return mReflowState.mComputedMargin;
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
#ifdef NOISY_MAXIMUM_WIDTH
      printf("nsBlockReflowState::UpdateMaximumWidth block %p caching max width %d\n", mBlock, aMaximumWidth);
#endif
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
                        nsRect* aDamageRect);

  void AdvanceToNextLine() {
    mLineNumber++;
  }

  PRBool IsImpactedByFloater() {
#ifdef REALLY_NOISY_REFLOW
    printf("nsBlockReflowState::IsImpactedByFloater %p returned %d\n", 
           this, mBand.GetFloaterCount());
#endif
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

  // The content area to reflow child frames within. The x/y
  // coordinates are known to be mBorderPadding.left and
  // mBorderPadding.top. The width/height may be NS_UNCONSTRAINEDSIZE
  // if the container reflowing this frame has given the frame an
  // unconstrained area.
  nsSize mContentArea;

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

  nsSize mMaxElementSize;
  nscoord mMaximumWidth;

  nscoord mMinLineHeight;

  PRInt32 mLineNumber;

  // block reflow state flags
#define BRS_UNCONSTRAINEDWIDTH    0x00000001
#define BRS_UNCONSTRAINEDHEIGHT   0x00000002
#define BRS_SHRINKWRAPWIDTH       0x00000004
#define BRS_NEEDRESIZEREFLOW      0x00000008
#define BRS_ISINLINEINCRREFLOW    0x00000010
#define BRS_NOWRAP                0x00000020
#define BRS_ISTOPMARGINROOT       0x00000040  // Is this frame a root for top/bottom margin collapsing?
#define BRS_ISBOTTOMMARGINROOT    0x00000080
#define BRS_APPLYTOPMARGIN        0x00000100  // See ShouldApplyTopMargin
#define BRS_COMPUTEMAXELEMENTSIZE 0x00000200
#define BRS_COMPUTEMAXWIDTH       0x00000400
#define BRS_LASTFLAG              BRS_COMPUTEMAXWIDTH

  PRInt16 mFlags;

  void SetFlag(PRUint32 aFlag, PRBool aValue)
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    NS_ASSERTION(aValue==PR_FALSE || aValue==PR_TRUE, "bad value");
    if (aValue) { // set flag
      mFlags |= aFlag;
    }
    else {        // unset flag
      mFlags &= ~aFlag;
    }
  }

  PRBool GetFlag(PRUint32 aFlag) const
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    PRBool result = (mFlags & aFlag);
    if (result) return PR_TRUE;
    return PR_FALSE;
  }
};
