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
#ifndef nsBlockReflowContext_h___
#define nsBlockReflowContext_h___

#include "nsIFrame.h"

class nsBlockFrame;
class nsIFrame;
class nsIPresContext;
class nsLineLayout;
struct nsStylePosition;
struct nsStyleSpacing;

/**
 * An encapsulation of the state and algorithm for reflowing block frames.
 */
class nsBlockReflowContext {
public:
  nsBlockReflowContext(nsIPresContext* aPresContext,
                       const nsHTMLReflowState& aParentRS,
                       PRBool aComputeMaxElementSize);
  ~nsBlockReflowContext() { }

  void SetNextRCFrame(nsIFrame* aNextRCFrame) {
    mNextRCFrame = aNextRCFrame;
  }

  nsIFrame* GetNextRCFrame() const {
    return mNextRCFrame;
  }

  nsresult ReflowBlock(nsIFrame* aFrame,
                       const nsRect& aSpace,
                       PRBool aApplyTopMargin,
                       nscoord aPrevBottomMargin,
                       PRBool aIsAdjacentWithTop,
                       nsMargin& aComputedOffsets,
                       nsReflowStatus& aReflowStatus);

  PRBool PlaceBlock(PRBool aForceFit,
                    const nsMargin& aComputedOffsets,
                    nscoord* aBottomMarginResult,
                    nsRect& aInFlowBounds,
                    nsRect& aCombinedRect);

  nscoord GetCarriedOutBottomMargin() const {
    return mMetrics.mCarriedOutBottomMargin;
  }

  nscoord GetTopMargin() const {
    return mTopMargin;
  }

  const nsMargin& GetMargin() const {
    return mMargin;
  }

  const nsHTMLReflowMetrics& GetMetrics() const {
    return mMetrics;
  }

  const nsSize& GetMaxElementSize() const {
    return mMaxElementSize;
  }

  // Compute the largest of two adjacent vertical margins, as per the
  // CSS2 spec section 8.3.1
  static nscoord MaxMargin(nscoord a, nscoord b) {
    if (a < 0) {
      if (b < 0) {
        if (a < b) return a;
        return b;
      }
      return b + a;
    }
    else if (b < 0) {
      return a + b;
    }
    if (a > b) return a;
    return b;
  }

  static PRBool IsHTMLParagraph(nsIFrame* aFrame);

  static nscoord ComputeCollapsedTopMargin(nsIPresContext* aPresContext,
                                           nsHTMLReflowState& aRS);

protected:
  nsStyleUnit GetRealMarginLeftUnit();
  nsStyleUnit GetRealMarginRightUnit();

  nsIPresContext* mPresContext;
  const nsHTMLReflowState& mOuterReflowState;

  nsIFrame* mFrame;
  nsRect mSpace;
  nsIFrame* mNextRCFrame;

  // Spacing style for the frame we are reflowing; only valid after reflow
  const nsStyleSpacing* mStyleSpacing;

  nscoord mComputedWidth;               // copy of reflowstate's computedWidth
  nsMargin mMargin;
  nscoord mX, mY;
  nsHTMLReflowMetrics mMetrics;
  nscoord mTopMargin;
  nsSize mMaxElementSize;
  PRBool mIsTable;
};

#endif /* nsBlockReflowContext_h___ */
