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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsHTMLReflowState_h___
#define nsHTMLReflowState_h___

#include "nslayout.h"

class nsIFrame;
class nsIPresContext;
class nsIReflowCommand;
class nsIRenderingContext;
class nsISpaceManager;
class nsLineLayout;

struct nsStyleDisplay;
struct nsStylePosition;
struct nsStyleSpacing;

/**
 * Constant used to indicate an unconstrained size.
 *
 * @see #Reflow()
 */
#define NS_UNCONSTRAINEDSIZE NS_MAXSIZE

/**
 * The reason the frame is being reflowed.
 *
 * XXX Should probably be a #define so it can be extended for specialized
 * reflow interfaces...
 *
 * @see nsHTMLReflowState
 */
enum nsReflowReason {
  eReflowReason_Initial = 0,     // initial reflow of a newly created frame
  eReflowReason_Incremental = 1, // an incremental change has occured. see the reflow command for details
  eReflowReason_Resize = 2,      // general request to determine a desired size
  eReflowReason_StyleChange = 3  // request to reflow because of a style change. Note: you must reflow
                                 // all your child frames
};

/**
 * CSS Frame type. Included as part of the reflow state.
 */
typedef PRUint32  nsCSSFrameType;

#define NS_CSS_FRAME_TYPE_UNKNOWN         0
#define NS_CSS_FRAME_TYPE_INLINE          1
#define NS_CSS_FRAME_TYPE_BLOCK           2  /* block-level in normal flow */
#define NS_CSS_FRAME_TYPE_FLOATING        3
#define NS_CSS_FRAME_TYPE_ABSOLUTE        4
#define NS_CSS_FRAME_TYPE_INTERNAL_TABLE  5  /* row group frame, row frame, cell frame, ... */

/**
 * Bit-flag that indicates whether the element is replaced. Applies to inline,
 * block-level, floating, and absolutely positioned elements
 */
#define NS_CSS_FRAME_TYPE_REPLACED        0x8000

/**
 * Helper macros for telling whether items are replaced
 */
#define NS_FRAME_IS_REPLACED(_ft) \
  (NS_CSS_FRAME_TYPE_REPLACED == ((_ft) & NS_CSS_FRAME_TYPE_REPLACED))

#define NS_FRAME_REPLACED(_ft) \
  (NS_CSS_FRAME_TYPE_REPLACED | (_ft))

/**
 * A macro to extract the type. Masks off the 'replaced' bit-flag
 */
#define NS_FRAME_GET_TYPE(_ft) \
  ((_ft) & ~NS_CSS_FRAME_TYPE_REPLACED)

#define NS_INTRINSICSIZE  NS_UNCONSTRAINEDSIZE
#define NS_AUTOHEIGHT     NS_UNCONSTRAINEDSIZE
#define NS_AUTOMARGIN     NS_UNCONSTRAINEDSIZE

/**
 * Reflow state passed to a frame during reflow. The reflow states are linked
 * together. The availableWidth and availableHeight represent the available
 * space in which to reflow your frame, and are computed based on the parent
 * frame's computed width and computed height. The available space is the total
 * space including margins, border, padding, and content area. A value of
 * NS_UNCONSTRAINEDSIZE means you can choose whatever size you want
 *
 * @see #Reflow()
 */
struct nsHTMLReflowState {
  // pointer to parent's reflow state
  const nsHTMLReflowState* parentReflowState;

  // the frame being reflowed
  nsIFrame*            frame;

  // the reason for the reflow
  nsReflowReason       reason;

  // the reflow command. only set for eReflowReason_Incremental
  nsIReflowCommand*    reflowCommand;

  // the available space in which to reflow
  nscoord              availableWidth, availableHeight;

  // rendering context to use for measurement
  nsIRenderingContext* rendContext;

  // is the current context at the top of a page?
  PRPackedBool         isTopOfPage;

  // The type of frame, from css's perspective. This value is
  // initialized by the Init method below.
  nsCSSFrameType   mFrameType;

  nsISpaceManager* mSpaceManager;

  // LineLayout object (only for inline reflow; set to NULL otherwise)
  nsLineLayout*    mLineLayout;

  // The computed width specifies the frame's content width, and it does not
  // apply to inline non-replaced elements
  //
  // For replaced inline frames, a value of NS_INTRINSICSIZE means you should
  // use your intrinsic width as the computed width
  //
  // For block-level frames, the computed width is based on the width of the
  // containing block, the margin/border/padding areas, and the min/max width
  nscoord          mComputedWidth; 

  // The computed height specifies the frame's content height, and it does
  // not apply to inline non-replaced elements
  //
  // For replaced inline frames, a value of NS_INTRINSICSIZE means you should
  // use your intrinsic height as the computed height
  //
  // For non-replaced block-level frames in the flow and floated, a value of
  // NS_AUTOHEIGHT means you choose a height to shrink wrap around the normal
  // flow child frames. The height must be within the limit of the min/max
  // height if there is such a limit
  //
  // For replaced block-level frames, a value of NS_INTRINSICSIZE
  // means you use your intrinsic height as the computed height
  nscoord          mComputedHeight;

  // Computed margin values
  nsMargin         mComputedMargin;

  // Cached copy of the border values
  nsMargin         mComputedBorderPadding;

  // Computed padding values
  nsMargin         mComputedPadding;

  // Computed values for 'left/top/right/bottom' offsets. Only applies to
  // 'positioned' elements
  nsMargin         mComputedOffsets;

  // Computed values for 'min-width/max-width' and 'min-height/max-height'
  nscoord          mComputedMinWidth, mComputedMaxWidth;
  nscoord          mComputedMinHeight, mComputedMaxHeight;

  // Compact margin available space
  nscoord          mCompactMarginWidth;

  // The following data members are relevant if nsStyleText.mTextAlign
  // == NS_STYLE_TEXT_ALIGN_CHAR

  // distance from reference edge (as specified in nsStyleDisplay.mDirection) 
  // to the align character (which will be specified in nsStyleTable)
  nscoord          mAlignCharOffset;

  // if true, the reflow honors alignCharOffset and does not
  // set it. if false, the reflow sets alignCharOffset
  PRPackedBool     mUseAlignCharOffset;

  // Cached pointers to the various style structs used during intialization
  const nsStyleDisplay* mStyleDisplay;
  const nsStylePosition* mStylePosition;
  const nsStyleSpacing* mStyleSpacing;

  // This value keeps track of how deeply nested a given reflow state
  // is from the top of the frame tree.
  PRInt32 mReflowDepth;

  // Note: The copy constructor is written by the compiler
  // automatically. You can use that and then override specific values
  // if you want, or you can call Init as desired...

  // Initialize a <b>root</b> reflow state with a rendering context to
  // use for measuring things.
  nsHTMLReflowState(nsIPresContext*          aPresContext,
                    nsIFrame*                aFrame,
                    nsReflowReason           aReason,
                    nsIRenderingContext*     aRenderingContext,
                    const nsSize&            aAvailableSpace);

  // Initialize a <b>root</b> reflow state for an <b>incremental</b>
  // reflow.
  nsHTMLReflowState(nsIPresContext*          aPresContext,
                    nsIFrame*                aFrame,
                    nsIReflowCommand&        aReflowCommand,
                    nsIRenderingContext*     aRenderingContext,
                    const nsSize&            aAvailableSpace);

  // Initialize a reflow state for a child frames reflow. Some state
  // is copied from the parent reflow state; the remaining state is
  // computed.
  nsHTMLReflowState(nsIPresContext*          aPresContext,
                    const nsHTMLReflowState& aParentReflowState,
                    nsIFrame*                aFrame,
                    const nsSize&            aAvailableSpace,
                    nsReflowReason           aReason);

  // Same as the previous except that the reason is taken from the
  // parent's reflow state.
  nsHTMLReflowState(nsIPresContext*          aPresContext,
                    const nsHTMLReflowState& aParentReflowState,
                    nsIFrame*                aFrame,
                    const nsSize&            aAvailableSpace);

  // Used when you want to override the default containing block
  // width and height. Used by absolute positioning code
  nsHTMLReflowState(nsIPresContext*          aPresContext,
                    const nsHTMLReflowState& aParentReflowState,
                    nsIFrame*                aFrame,
                    const nsSize&            aAvailableSpace,
                    nscoord                  aContainingBlockWidth,
                    nscoord                  aContainingBlockHeight);

  /**
   * Get the containing block reflow state, starting from a frames
   * <B>parent</B> reflow state (the parent reflow state may or may not end
   * up being the containing block reflow state)
   */
  static const nsHTMLReflowState*
    GetContainingBlockReflowState(const nsHTMLReflowState* aParentRS);

  /**
   * First find the containing block's reflow state using
   * GetContainingBlockReflowState, then ask the containing block for
   * it's content width using GetContentWidth
   */
  static nscoord
    GetContainingBlockContentWidth(const nsHTMLReflowState* aParentRS);

  /**
   * Get the page box reflow state, starting from a frames
   * <B>parent</B> reflow state (the parent reflow state may or may not end
   * up being the containing block reflow state)
   */
  static const nsHTMLReflowState*
    GetPageBoxReflowState(const nsHTMLReflowState* aParentRS);

  /**
   * Compute the border plus padding for <TT>aFrame</TT>. If a
   * percentage needs to be computed it will be computed by finding
   * the containing block, use GetContainingBlockReflowState.
   * aParentReflowState is aFrame's
   * parent's reflow state. The resulting computed border plus padding
   * is returned in aResult.
   */
  static void ComputeBorderPaddingFor(nsIFrame* aFrame,
                                      const nsHTMLReflowState* aParentRS,
                                      nsMargin& aResult);

  /**
   * Calculate the raw line-height property for the given frame. The return
   * value, if line-height was applied and is valid will be >= 0. Otherwise,
   * the return value will be <0 which is illegal (CSS2 spec: section 10.8.1).
   */
  static nscoord CalcLineHeight(nsIPresContext* aPresContext,
                                nsIRenderingContext* aRenderingContext,
                                nsIFrame* aFrame);

  static PRBool UseComputedHeight();

  static nsCSSFrameType DetermineFrameType(nsIFrame* aFrame);

  void ComputeContainingBlockRectangle(const nsHTMLReflowState* aContainingBlockRS,
                                       nscoord& aContainingBlockWidth,
                                       nscoord& aContainingBlockHeight);

protected:
  // This method initializes various data members. It is automatically
  // called by the various constructors
  void Init(nsIPresContext* aPresContext,
            nscoord         aContainingBlockWidth = -1,
            nscoord         aContainingBlockHeight = -1);

  void InitConstraints(nsIPresContext* aPresContext,
                       nscoord         aContainingBlockWidth,
                       nscoord         aContainingBlockHeight);

  void InitAbsoluteConstraints(nsIPresContext* aPresContext,
                               const nsHTMLReflowState* cbrs,
                               nscoord aContainingBlockWidth,
                               nscoord aContainingBlockHeight);

  void ComputeRelativeOffsets(const nsHTMLReflowState* cbrs,
                              nscoord aContainingBlockWidth,
                              nscoord aContainingBlockHeight);

  void ComputeBlockBoxData(nsIPresContext* aPresContext,
                           const nsHTMLReflowState* cbrs,
                           nsStyleUnit aWidthUnit,
                           nsStyleUnit aHeightUnit,
                           nscoord aContainingBlockWidth,
                           nscoord aContainingBlockHeight);

  void CalculateBlockSideMargins(const nsHTMLReflowState* aContainingBlockRS,
                                 nscoord                  aComputedWidth);

  void ComputeHorizontalValue(nscoord aContainingBlockWidth,
                                     nsStyleUnit aUnit,
                                     const nsStyleCoord& aCoord,
                                     nscoord& aResult);

  void ComputeVerticalValue(nscoord aContainingBlockHeight,
                                   nsStyleUnit aUnit,
                                   const nsStyleCoord& aCoord,
                                   nscoord& aResult);

  static nsCSSFrameType DetermineFrameType(nsIFrame* aFrame,
                                           const nsStylePosition* aPosition,
                                           const nsStyleDisplay* aDisplay);

  // Computes margin values from the specified margin style information, and
  // fills in the mComputedMargin member
  void ComputeMargin(nscoord aContainingBlockWidth,
                     const nsHTMLReflowState* aContainingBlockRS);
  
  // Computes padding values from the specified padding style information, and
  // fills in the mComputedPadding member
  void ComputePadding(nscoord aContainingBlockWidth,
                      const nsHTMLReflowState* aContainingBlockRS);

  // Calculates the computed values for the 'min-Width', 'max-Width',
  // 'min-Height', and 'max-Height' properties, and stores them in the assorted
  // data members
  void ComputeMinMaxValues(nscoord                  aContainingBlockWidth,
                           nscoord                  aContainingBlockHeight,
                           const nsHTMLReflowState* aContainingBlockRS);

};

#endif /* nsHTMLReflowState_h___ */

