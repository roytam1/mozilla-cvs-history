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
#include "nsRect.h"
class nsISpaceManager;
class nsBlockFrame;
class nsLineLayout;

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

  // Carried out margin values. If the top/bottom margin values were
  // computed auto values then the corresponding bits are set in this
  // value.
  PRUintn mCarriedOutMarginFlags;

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
    mCarriedOutMarginFlags = 0;
    mCombinedArea.x = 0;
    mCombinedArea.y = 0;
    mCombinedArea.width = 0;
    mCombinedArea.height = 0;
  }
};

// Carried out margin flags
#define NS_CARRIED_TOP_MARGIN_IS_AUTO    0x1
#define NS_CARRIED_BOTTOM_MARGIN_IS_AUTO 0x2

//----------------------------------------------------------------------

/**
 * The type of size constraint that applies to a particular
 * dimension.  For the fixed and fixed content cases the min/max
 * width/height in the reflow state structure is ignored and you
 * should use the max size value when reflowing the frame.
 *
 * @see nsHTMLReflowState
 */
enum nsHTMLFrameConstraint {
  // Choose whatever frame size you want (there is no stylistic limitation
  // on the size of the frame)
  eHTMLFrameConstraint_Unconstrained,

  // Choose a frame size between the min and max sizes
  eHTMLFrameConstraint_Constrained,

  // Frame size is fixed. The nsReflowState::maxSize.width value
  // determines the fixed width.
  eHTMLFrameConstraint_Fixed,

  // Content size is fixed. The nsReflowState::maxSize.width value
  // determines the fixed width.
  eHTMLFrameConstraint_FixedContent
};

//----------------------------------------------------------------------

/**
 * CSS Frame type. Included as part of the reflow state.
 *
 * @see nsHTMLReflowState
 *
 * XXX This requires some more thought. Are these the correct set?
 * XXX Should we treat 'replaced' as a bit flag instead of doubling the
 *     number of enumerators?
 */
enum nsCSSFrameType {
  // unknown frame type
  eCSSFrameType_Unknown,

  // inline, non-replaced elements
  eCSSFrameType_Inline,

  // inline, replaced elements (e.g., image)
  eCSSFrameType_InlineReplaced,

  // block-level, non-replaced elements in normal flow
  eCSSFrameType_Block,

  // block-level, replaced elements in normal flow
  eCSSFrameType_BlockReplaced,

  // floating, non-replaced elements
  eCSSFrameType_Floating,

  // floating, replaced elements
  eCSSFrameType_FloatingReplaced,

  // absolutely positioned, non-replaced elements
  eCSSFrameType_Absolute,

  // absolutely positioned, replaced elements
  eCSSFrameType_AbsoluteReplaced
};

//----------------------------------------------------------------------

/**
 * HTML version of the reflow state.
 *
 * Note: the constructors are implemented inline later on in this file
 */
struct nsHTMLReflowState : nsReflowState {
  // The type of frame, from css's perspective. This value is
  // initialized by the Init method below.
  nsCSSFrameType        frameType;

  nsISpaceManager*      spaceManager;

  // LineLayout object (only for inline reflow; set to NULL otherwise)
  nsLineLayout*         lineLayout;

  // Constraint that applies to width dimension
  nsHTMLFrameConstraint widthConstraint;
  nscoord               minWidth;

  // Constraint that applies to height dimension
  nsHTMLFrameConstraint heightConstraint;
  nscoord               minHeight;

  // Run-in frame made available for reflow
  nsBlockFrame*         mRunInFrame;

  // Constructs an initial reflow state (no parent reflow state) for a
  // non-incremental reflow command. Sets reflowType to eReflowType_Block
  nsHTMLReflowState(nsIPresContext&      aPresContext,
                    nsIFrame*            aFrame,
                    nsReflowReason       aReason, 
                    const nsSize&        aMaxSize,
                    nsIRenderingContext* aContext,
                    nsISpaceManager*     aSpaceManager = nsnull);

  // Constructs an initial reflow state (no parent reflow state) for an
  // incremental reflow command. Sets reflowType to eReflowType_Block
  nsHTMLReflowState(nsIPresContext&      aPresContext,
                    nsIFrame*            aFrame,
                    nsIReflowCommand&    aReflowCommand,
                    const nsSize&        aMaxSize,
                    nsIRenderingContext* aContext,
                    nsISpaceManager*     aSpaceManager = nsnull);

  // Construct a reflow state for the given frame, parent reflow state, and
  // max size. Uses the reflow reason, space manager, reflow command, and
  // line layout from the parent's reflow state.  Defaults to a reflow
  // frame type of eReflowType_Block
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aMaxSize);

  // Construct a reflow state for the given inline frame, parent
  // reflow state, and max size. Uses the reflow reason, space
  // manager, and reflow command from the parent's reflow state. Sets
  // the reflow frame type to eReflowType_Inline
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aMaxSize,
                    nsLineLayout*            aLineLayout);

  // Constructs a reflow state that overrides the reflow reason of the parent
  // reflow state. Uses the space manager from the parent's reflow state and
  // sets the reflow command to NULL. Sets lineLayout to NULL, and defaults to
  // a reflow frame type of eReflowType_Block
  nsHTMLReflowState(nsIPresContext&          aPresContext,
                    nsIFrame*                aFrame,
                    const nsHTMLReflowState& aParentReflowState,
                    const nsSize&            aMaxSize,
                    nsReflowReason           aReflowReason);

  PRBool HaveFixedContentWidth() const {
    return eHTMLFrameConstraint_FixedContent == widthConstraint;
  }

  PRBool HaveFixedContentHeight() const {
    return eHTMLFrameConstraint_FixedContent == heightConstraint;
  }

protected:
  // This method initializes the widthConstraint, heightConstraint and
  // minSize values appropriately. It also initializes the frameType
  // value as well. This method is automatically called by the various
  // constructors.
  void Init(nsIPresContext& aPresContext) {
    mRunInFrame = nsnull;
    DetermineFrameType(aPresContext);
    InitConstraints(aPresContext);
  }

  void DetermineFrameType(nsIPresContext& aPresContext);

  void InitConstraints(nsIPresContext& aPresContext);
};

//----------------------------------------------------------------------

/**
 * Extensions to the reflow status bits defined by nsIFrameReflow
 */

// This bit is set, when a break is requested. This bit is orthogonal
// to the nsIFrame::nsReflowStatus completion bits.
#define NS_INLINE_BREAK             0x0100

#define NS_INLINE_IS_BREAK(_status) \
  (0 != ((_status) & NS_INLINE_BREAK))

// When a break is requested, this bit when set indicates that the
// break should occur after the frame just reflowed; when the bit is
// clear the break should occur before the frame just reflowed.
#define NS_INLINE_BREAK_BEFORE      0x0000
#define NS_INLINE_BREAK_AFTER       0x0200

#define NS_INLINE_IS_BREAK_AFTER(_status) \
  (0 != ((_status) & NS_INLINE_BREAK_AFTER))

#define NS_INLINE_IS_BREAK_BEFORE(_status) \
  (NS_INLINE_BREAK == ((_status) & (NS_INLINE_BREAK|NS_INLINE_BREAK_AFTER)))

// The type of break requested can be found in these bits.
#define NS_INLINE_BREAK_TYPE_MASK   0xF000

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
                                     const nsSize&        aMaxSize,
                                     nsIRenderingContext* aContext,
                                     nsISpaceManager*     aSpaceManager)
  : nsReflowState(aFrame, aReason, aMaxSize, aContext)
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
                                     const nsSize&        aMaxSize,
                                     nsIRenderingContext* aContext,
                                     nsISpaceManager*     aSpaceManager)
  : nsReflowState(aFrame, aReflowCommand, aMaxSize, aContext)
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
                                     const nsSize&            aMaxSize)
  : nsReflowState(aFrame, aParentState, aMaxSize)
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
                                     const nsSize&            aMaxSize,
                                     nsLineLayout*            aLineLayout)
  : nsReflowState(aFrame, aParentState, aMaxSize)
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
                                     const nsSize&            aMaxSize,
                                     nsReflowReason           aReflowReason)
  : nsReflowState(aFrame, aParentState, aMaxSize, aReflowReason)
{
  spaceManager = aParentState.spaceManager;
  lineLayout = nsnull;
  Init(aPresContext);
}

#endif /* nsIHTMLReflow_h___ */
