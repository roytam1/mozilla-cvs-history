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
#ifndef nsIHTMLReflow_h___
#define nsIHTMLReflow_h___

#include "nsIFrameReflow.h"
#include "nsStyleConsts.h"
#include "nsStyleCoord.h"
#include "nsRect.h"
class nsISpaceManager;
class nsBlockFrame;
class nsLineLayout;
struct nsStylePosition;
struct nsStyleSpacing;

// IID for the nsIHTMLFrame interface 
// a6cf9069-15b3-11d2-932e-00805f8add32
#define NS_IHTMLREFLOW_IID \
 { 0xa6cf9069, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

//----------------------------------------------------------------------

/**
 * HTML/CSS specific reflow metrics
 */
struct nsHTMLReflowMetrics : nsReflowMetrics {
  // Carried out top/bottom margin values. This is the top and bottom
  // margin values from a frames first/last child.
  nscoord mCarriedOutTopMargin;
  nscoord mCarriedOutBottomMargin;

  // For frames that have children that stick outside their rect
  // (NS_FRAME_OUTSIDE_CHILDREN) this rectangle will contain the
  // absolute bounds of the frame. Since the frame doesn't know where
  // it is going to be positioned in its parent, the assumption is
  // that it is placed at 0,0 when computing this area.
  nsRect mCombinedArea;

  nsHTMLReflowMetrics(nsSize* aMaxElementSize)
    : nsReflowMetrics(aMaxElementSize)
  {
    mCarriedOutTopMargin = 0;
    mCarriedOutBottomMargin = 0;
    mCombinedArea.x = 0;
    mCombinedArea.y = 0;
    mCombinedArea.width = 0;
    mCombinedArea.height = 0;
  }

  void AddBorderPaddingToMaxElementSize(const nsMargin& aBorderPadding) {
    maxElementSize->width += aBorderPadding.left + aBorderPadding.right;
    maxElementSize->height += aBorderPadding.top + aBorderPadding.bottom;
  }
};

// Carried out margin flags
#define NS_CARRIED_TOP_MARGIN_IS_AUTO    0x1
#define NS_CARRIED_BOTTOM_MARGIN_IS_AUTO 0x2

//----------------------------------------------------------------------

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

//----------------------------------------------------------------------

#define NS_INTRINSICSIZE  NS_UNCONSTRAINEDSIZE
#define NS_AUTOHEIGHT     NS_UNCONSTRAINEDSIZE

/**
 * HTML version of the reflow state.
 *
 * Note: the constructors are implemented inline later on in this file
 */
struct nsHTMLReflowState : nsReflowState {
  // The type of frame, from css's perspective. This value is
  // initialized by the Init method below.
  nsCSSFrameType   frameType;

  nsISpaceManager* spaceManager;

  // LineLayout object (only for inline reflow; set to NULL otherwise)
  nsLineLayout*    lineLayout;

  // Computed width and margins. The computed width specifies the frame's
  // content width, and it does not apply to inline non-replaced elements
  //
  // For replaced inline frames, a value of NS_INTRINSICSIZE means you should
  // use your intrinsic width as the computed width
  //
  // For block-level frames, the computed width is based on the width of the
  // containing block and the margin/border/padding areas and the min/max
  // width
  nscoord          computedWidth; 
  nscoord          computedLeftMargin, computedRightMargin; 

  // Computed height and margins. The computed height specifies the frame's
  // content height, and it does not apply to inline non-replaced elements
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
  nscoord          computedHeight;
  nscoord          computedTopMargin, computedBottomMargin;

  // Computed values for 'left/top/right/bottom' offsets. Only applies to
  // 'positioned' elements
  nsMargin         computedOffsets;

  // Run-in frame made available for reflow
  nsBlockFrame*    mRunInFrame;

  // Compact margin available space
  nscoord          mCompactMarginWidth;

  // The line-height for the frame. If the frame has no specific
  // line-height value then this field will be "-1".
  nscoord          mLineHeight;

  // the following data members are relevant if nsStyleText.mTextAlign == NS_STYLE_TEXT_ALIGN_CHAR
  nscoord          mAlignCharOffset;   // distance from reference edge (as specified in nsStyleDisplay.mDirection) 
                                       // to the align character (which will be specified in nsStyleTable)
  PRPackedBool     mUseAlignCharOffset;// if true, the reflow honors alignCharOffset and does not
                                       // set it. if false, the reflow sets alignCharOffset

  // Constructs an initial reflow state (no parent reflow state) for a
  // non-incremental reflow command. Sets reflowType to eReflowType_Block
  nsHTMLReflowState(nsIPresContext&      aPresContext,
                    nsIFrame*            aFrame,
                    nsReflowReason       aReason, 
                    const nsSize&        aAvailableSpace,
                    nsIRenderingContext* aContext,
                    nsISpaceManager*     aSpaceManager = nsnull);

  // Constructs an initial reflow state (no parent reflow state) for an
  // incremental reflow command. Sets reflowType to eReflowType_Block
  nsHTMLReflowState(nsIPresContext&      aPresContext,
                    nsIFrame*            aFrame,
                    nsIReflowCommand&    aReflowCommand,
                    const nsSize&        aAvailableSpace,
                    nsIRenderingContext* aContext,
                    nsISpaceManager*     aSpaceManager = nsnull);

  // Construct a reflow state for the given frame, parent reflow state, and
  // max size. Uses the reflow reason, space manager, reflow command, and
  // line layout from the parent's reflow state.  Defaults to a reflow
  // frame type of eReflowType_Block
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aAvailableSpace);

  // Construct a reflow state for the given inline frame, parent
  // reflow state, and max size. Uses the reflow reason, space
  // manager, and reflow command from the parent's reflow state. Sets
  // the reflow frame type to eReflowType_Inline
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aAvailableSpace,
                    nsLineLayout*            aLineLayout);

  // Constructs a reflow state that overrides the reflow reason of the parent
  // reflow state. Uses the space manager from the parent's reflow state and
  // sets the reflow command to NULL. Sets lineLayout to NULL, and defaults to
  // a reflow frame type of eReflowType_Block
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aAvailableSpace,
                    nsReflowReason           aReflowReason);

  /**
   * Returns PR_TRUE if the specified width or height has an value other
   * than 'auto'
   */
  PRBool HaveFixedContentWidth() const;
  PRBool HaveFixedContentHeight() const;

  /**
   * Get the containing block reflow state, starting from a frames
   * <B>parent</B> reflow state (the parent reflow state may or may not end
   * up being the containing block reflow state)
   */
  static const nsHTMLReflowState*
    GetContainingBlockReflowState(const nsReflowState* aParentRS);

  /**
   * First find the containing block's reflow state using
   * GetContainingBlockReflowState, then ask the containing block for
   * it's content width using GetContentWidth
   */
  static nscoord
    GetContainingBlockContentWidth(const nsReflowState* aParentRS);

  /**
   * Get the page box reflow state, starting from a frames
   * <B>parent</B> reflow state (the parent reflow state may or may not end
   * up being the containing block reflow state)
   */
  static const nsHTMLReflowState*
    GetPageBoxReflowState(const nsReflowState* aParentRS);

  /**
   * Compute the margin for <TT>aFrame</TT>. If a percentage needs to
   * be computed it will be computed by finding the containing block,
   * use GetContainingBlockReflowState. aParentReflowState is aFrame's
   * parent's reflow state. The resulting computed margin is returned
   * in aResult.
   */ 
  static void ComputeMarginFor(nsIFrame* aFrame,
                               const nsReflowState* aParentReflowState,
                               nsMargin& aResult);

  /**
   * Compute the padding for <TT>aFrame</TT>. If a percentage needs to
   * be computed it will be computed by finding the containing block,
   * use GetContainingBlockReflowState. aParentReflowState is aFrame's
   * parent's reflow state. The resulting computed padding is returned
   * in aResult.
   */ 
  static void ComputePaddingFor(nsIFrame* aFrame,
                                const nsReflowState* aParentReflowState,
                                nsMargin& aResult);

  /**
   * Compute the border for <TT>aFrame</TT>.The resulting computed
   * padding is returned in aResult.
   */
  static void ComputeBorderFor(nsIFrame* aFrame,
                               nsMargin& aResult);

  /**
   * Compute the border plus padding for <TT>aFrame</TT>. If a
   * percentage needs to be computed it will be computed by finding
   * the containing block, use GetContainingBlockReflowState.
   * aParentReflowState is aFrame's
   * parent's reflow state. The resulting computed border plus padding
   * is returned in aResult.
   */
  static void ComputeBorderPaddingFor(nsIFrame* aFrame,
                                      const nsReflowState* aParentRS,
                                      nsMargin& aResult);

  /**
   * Calculate the line-height property for the given frame. The return
   * value, if line-height was applied and is valid will be >= 0. Otherwise,
   * the return value will be <0 which is illegal (CSS2 spec: section 10.8.1).
   */
  static nscoord CalcLineHeight(nsIPresContext& aPresContext,
                                nsIFrame* aFrame);

protected:
  // This method initializes various data members. It is automatically
  // called by the various constructors
  void Init(nsIPresContext& aPresContext) {
    mRunInFrame = nsnull;
    mCompactMarginWidth = 0;
    computedWidth = computedHeight = 0;
    DetermineFrameType(aPresContext);
    InitConstraints(aPresContext);
  }

  void DetermineFrameType(nsIPresContext& aPresContext);

  void InitConstraints(nsIPresContext& aPresContext);
  void InitAbsoluteConstraints(nsIPresContext& aPresContext,
                               const nsHTMLReflowState* cbrs,
                               const nsStylePosition* aPosition,
                               nscoord containingBlockWidth,
                               nscoord containingBlockHeight);

  void CalculateLeftRightMargin(const nsHTMLReflowState* aContainingBlockRS,
                                const nsStyleSpacing*    aSpacing,
                                nscoord                  aComputedWidth,
                                const nsMargin&          aBorderPadding,
                                nscoord&                 aComputedLeftMargin,
                                nscoord&                 aComputedRightMargin);

  static void ComputeHorizontalValue(nscoord aContainingBlockWidth,
                                     nsStyleUnit aUnit,
                                     const nsStyleCoord& aCoord,
                                     nscoord& aResult);

  static void ComputeVerticalValue(nscoord aContainingBlockHeight,
                                   nsStyleUnit aUnit,
                                   const nsStyleCoord& aCoord,
                                   nscoord& aResult);
};

//----------------------------------------------------------------------

/**
 * Extensions to the reflow status bits defined by nsIFrameReflow
 */

// This bit is set, when a break is requested. This bit is orthogonal
// to the nsIFrame::nsReflowStatus completion bits.
#define NS_INLINE_BREAK              0x0100

// When a break is requested, this bit when set indicates that the
// break should occur after the frame just reflowed; when the bit is
// clear the break should occur before the frame just reflowed.
#define NS_INLINE_BREAK_BEFORE       0x0000
#define NS_INLINE_BREAK_AFTER        0x0200

// The type of break requested can be found in these bits.
#define NS_INLINE_BREAK_TYPE_MASK    0xF000

//----------------------------------------
// Macros that use those bits

#define NS_INLINE_IS_BREAK(_status) \
  (0 != ((_status) & NS_INLINE_BREAK))

#define NS_INLINE_IS_BREAK_AFTER(_status) \
  (0 != ((_status) & NS_INLINE_BREAK_AFTER))

#define NS_INLINE_IS_BREAK_BEFORE(_status) \
  (NS_INLINE_BREAK == ((_status) & (NS_INLINE_BREAK|NS_INLINE_BREAK_AFTER)))

#define NS_INLINE_GET_BREAK_TYPE(_status) (((_status) >> 12) & 0xF)

#define NS_INLINE_MAKE_BREAK_TYPE(_type)  ((_type) << 12)

// Construct a line-break-before status. Note that there is no
// completion status for a line-break before because we *know* that
// the frame will be reflowed later and hence it's current completion
// status doesn't matter.
#define NS_INLINE_LINE_BREAK_BEFORE()                                   \
  (NS_INLINE_BREAK | NS_INLINE_BREAK_BEFORE |                           \
   NS_INLINE_MAKE_BREAK_TYPE(NS_STYLE_CLEAR_LINE))

// Take a completion status and add to it the desire to have a
// line-break after. For this macro we do need the completion status
// because the user of the status will need to know whether to
// continue the frame or not.
#define NS_INLINE_LINE_BREAK_AFTER(_completionStatus)                   \
  ((_completionStatus) | NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |      \
   NS_INLINE_MAKE_BREAK_TYPE(NS_STYLE_CLEAR_LINE))

//----------------------------------------------------------------------

/**
 * Generate a reflow interface specific to HTML/CSS frame objects
 */
class nsIHTMLReflow : public nsIFrameReflow<nsHTMLReflowState, nsHTMLReflowMetrics>
{
public:
  // Helper method used by block reflow to identify runs of text so that
  // proper word-breaking can be done.
  NS_IMETHOD FindTextRuns(nsLineLayout& aLineLayout) = 0;

  // Justification helper method used to distribute extra space in a
  // line to leaf frames. aUsedSpace is filled in with the amount of
  // space actually used.
  NS_IMETHOD AdjustFrameSize(nscoord aExtraSpace, nscoord& aUsedSpace) = 0;

  // Justification helper method that is used to remove trailing
  // whitespace before justification.
  NS_IMETHOD TrimTrailingWhiteSpace(nsIPresContext& aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth) = 0;

  // Any objects in the frame that impact the spacemanager (e.g. a
  // floater) are to be moved in the spacemanager by the given delta
  // values.
  NS_IMETHOD MoveInSpaceManager(nsIPresContext& aPresContext,
                                nsISpaceManager* aSpaceManager,
                                nscoord aDeltaX, nscoord aDeltaY) = 0;
};

//----------------------------------------------------------------------

// Constructs an initial reflow state (no parent reflow state) for a
// non-incremental reflow command. Sets reflowType to eReflowType_Block
inline
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext&      aPresContext,
                                     nsIFrame*            aFrame,
                                     nsReflowReason       aReason, 
                                     const nsSize&        aAvailableSpace,
                                     nsIRenderingContext* aContext,
                                     nsISpaceManager*     aSpaceManager)
  : nsReflowState(aFrame, aReason, aAvailableSpace, aContext)
{
  spaceManager = aSpaceManager;
  lineLayout = nsnull;
  Init(aPresContext);
}

// Constructs an initial reflow state (no parent reflow state) for an
// incremental reflow command. Sets reflowType to eReflowType_Block
inline
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext&      aPresContext,
                                     nsIFrame*            aFrame,
                                     nsIReflowCommand&    aReflowCommand,
                                     const nsSize&        aAvailableSpace,
                                     nsIRenderingContext* aContext,
                                     nsISpaceManager*     aSpaceManager)
  : nsReflowState(aFrame, aReflowCommand, aAvailableSpace, aContext)
{
  spaceManager = aSpaceManager;
  lineLayout = nsnull;
  Init(aPresContext);
}

// Construct a reflow state for the given frame, parent reflow state, and
// max size. Uses the reflow reason, space manager, reflow command, and
// line layout from the parent's reflow state.  Defaults to a reflow
// frame type of eReflowType_Block
inline
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext&          aPresContext,
                                     nsIFrame*                aFrame,
                                     const nsHTMLReflowState& aParentState,
                                     const nsSize&            aAvailableSpace)
  : nsReflowState(aFrame, aParentState, aAvailableSpace)
{
  spaceManager = aParentState.spaceManager;
  lineLayout = aParentState.lineLayout;
  Init(aPresContext);
}

// Construct a reflow state for the given inline frame, parent reflow state,
// and max size. Uses the reflow reason, space manager, and reflow command from
// the parent's reflow state. Sets the reflow frame type to eReflowType_Inline
inline
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext&          aPresContext,
                                     nsIFrame*                aFrame,
                                     const nsHTMLReflowState& aParentState,
                                     const nsSize&            aAvailableSpace,
                                     nsLineLayout*            aLineLayout)
  : nsReflowState(aFrame, aParentState, aAvailableSpace)
{
  spaceManager = aParentState.spaceManager;
  lineLayout = aLineLayout;
  Init(aPresContext);
}

// Constructs a reflow state that overrides the reflow reason of the parent
// reflow state. Uses the space manager from the parent's reflow state and
// sets the reflow command to NULL. Sets lineLayout to NULL, and defaults to
// a reflow frame type of eReflowType_Block
inline
nsHTMLReflowState::nsHTMLReflowState(nsIPresContext&          aPresContext,
                                     nsIFrame*                aFrame,
                                     const nsHTMLReflowState& aParentState,
                                     const nsSize&            aAvailableSpace,
                                     nsReflowReason           aReflowReason)
  : nsReflowState(aFrame, aParentState, aAvailableSpace, aReflowReason)
{
  spaceManager = aParentState.spaceManager;
  lineLayout = nsnull;
  Init(aPresContext);
}

#endif /* nsIHTMLReflow_h___ */
