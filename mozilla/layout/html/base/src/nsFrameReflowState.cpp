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
#include "nsFrameReflowState.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsIHTMLReflow.h"
#include "nsIContent.h"
#include "nsHTMLAtoms.h"

// XXX there is no CLEAN way to detect the "replaced" attribute (yet)
void
nsHTMLReflowState::DetermineFrameType(nsIPresContext& aPresContext)
{
  nsIAtom* tag = nsnull;
  nsIContent* content;
  if ((NS_OK == frame->GetContent(content)) && (nsnull != content)) {
    content->GetTag(tag);
    NS_RELEASE(content);
  }

  // Section 9.7 indicates that absolute position takes precedence
  // over float which takes precedence over display.
  const nsStyleDisplay* display;
  frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
  const nsStylePosition* pos;
  frame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)pos);
  if ((nsnull != pos) && (NS_STYLE_POSITION_ABSOLUTE == pos->mPosition)) {
    // XXX replaced?
    frameType = eCSSFrameType_Absolute;
  }
  else if (NS_STYLE_FLOAT_NONE != display->mFloats) {
    // XXX replaced?
    frameType = eCSSFrameType_Floating;
  }
  else {
    switch (display->mDisplay) {
    case NS_STYLE_DISPLAY_BLOCK:
    case NS_STYLE_DISPLAY_LIST_ITEM:
    case NS_STYLE_DISPLAY_TABLE:
    case NS_STYLE_DISPLAY_TABLE_CELL:
    case NS_STYLE_DISPLAY_TABLE_CAPTION:
      frameType = eCSSFrameType_Block;
      break;

    case NS_STYLE_DISPLAY_INLINE:
    case NS_STYLE_DISPLAY_MARKER:
    case NS_STYLE_DISPLAY_INLINE_TABLE:
      if ((nsHTMLAtoms::img == tag) ||
          (nsHTMLAtoms::applet == tag) ||
          (nsHTMLAtoms::object == tag) ||
          (nsHTMLAtoms::input == tag) ||
          (nsHTMLAtoms::select == tag) ||
          (nsHTMLAtoms::textarea == tag) ||
          (nsHTMLAtoms::button == tag) ||
          (nsHTMLAtoms::legend == tag) ||
          (nsHTMLAtoms::fieldset == tag) ||
          (nsHTMLAtoms::iframe == tag)) {
        frameType = eCSSFrameType_InlineReplaced;
      }
      else {
        frameType = eCSSFrameType_Inline;
      }
      break;

    case NS_STYLE_DISPLAY_RUN_IN:
    case NS_STYLE_DISPLAY_COMPACT:
      // XXX need to look ahead at the frame's sibling
      frameType = eCSSFrameType_Block;
      break;

    case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    case NS_STYLE_DISPLAY_TABLE_COLUMN:
    case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
    case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    case NS_STYLE_DISPLAY_TABLE_ROW:
      // XXX I don't know what to do about these yet...later
      frameType = eCSSFrameType_Inline;
      break;

    case NS_STYLE_DISPLAY_NONE:
    default:
      frameType = eCSSFrameType_Unknown;
      break;
    }
    NS_IF_RELEASE(tag);
  }
}

void
nsHTMLReflowState::InitConstraints(nsIPresContext& aPresContext)
{
  // Determine whether the values are constrained or unconstrained
  // by looking at the maxSize
  widthConstraint = NS_UNCONSTRAINEDSIZE == maxSize.width ?
                      eHTMLFrameConstraint_Unconstrained :
                      eHTMLFrameConstraint_Constrained;
  heightConstraint = NS_UNCONSTRAINEDSIZE == maxSize.height ?
                      eHTMLFrameConstraint_Unconstrained :
                      eHTMLFrameConstraint_Constrained;
  minWidth = 0;
  minHeight = 0;

  // Some frame types are not constrained by width/height style
  // attributes. Return if the frame is one of those types.
  switch (frameType) {
  case eCSSFrameType_Unknown:
  case eCSSFrameType_Inline:
    return;

  default:
    break;
  }

  // Look for stylistic constraints on the width/height
  const nsStylePosition* pos;
  nsresult result = frame->GetStyleData(eStyleStruct_Position,
                                        (const nsStyleStruct*&)pos);
  if (NS_OK == result) {
    nscoord containingBlockWidth, containingBlockHeight;
    nscoord width = -1, height = -1;
    PRIntn widthUnit = pos->mWidth.GetUnit();
    PRIntn heightUnit = pos->mHeight.GetUnit();

    // When a percentage is specified we need to find the containing
    // block to use as the basis for the percentage computation.
    if ((eStyleUnit_Percent == widthUnit) ||
        (eStyleUnit_Percent == heightUnit)) {
      // Find the containing block for this frame
      nsIFrame* containingBlock = nsnull;
      const nsReflowState* rs = parentReflowState;
      while (nsnull != rs) {
        if (nsnull != rs->frame) {
          PRBool isContainingBlock;
          if (NS_OK == rs->frame->IsPercentageBase(isContainingBlock)) {
            if (isContainingBlock) {
              containingBlock = rs->frame;
              break;
            }
          }
        }
        rs = rs->parentReflowState;
      }

      // If there is no containing block then pretend the width or
      // height units are auto.
      if (nsnull == containingBlock) {
        if (eStyleUnit_Percent == widthUnit) {
          widthUnit = eStyleUnit_Auto;
        }
        if (eStyleUnit_Percent == heightUnit) {
          heightUnit = eStyleUnit_Auto;
        }
      }
      else {
        if (eStyleUnit_Percent == widthUnit) {
          if (NS_UNCONSTRAINEDSIZE == rs->maxSize.width) {
            // When we don't know the width (yet) of the containing
            // block we use a dummy value, assuming that the frame
            // depending on the percentage value will be reflowed a
            // second time.
            containingBlockWidth = 1;
          }
          else {
            containingBlockWidth = rs->maxSize.width;
          }
        }
        if (eStyleUnit_Percent == heightUnit) {
          if (NS_UNCONSTRAINEDSIZE == rs->maxSize.height) {
            // CSS2 spec, 10.5: if the height of the containing block
            // is not specified explicitly then the value is
            // interpreted like auto.
            heightUnit = eStyleUnit_Auto;
          }
          else {
            containingBlockHeight = rs->maxSize.height;
          }
        }
      }
    }

    switch (widthUnit) {
    case eStyleUnit_Coord:
      width = pos->mWidth.GetCoordValue();
      break;
    case eStyleUnit_Percent:
      width = nscoord(pos->mWidth.GetPercentValue() * containingBlockWidth);
      break;
    case eStyleUnit_Auto:
      // XXX See section 10.3 of the css2 spec and then write this code!
      break;
    }
    switch (heightUnit) {
    case eStyleUnit_Coord:
      height = pos->mHeight.GetCoordValue();
      break;
    case eStyleUnit_Percent:
      height = nscoord(pos->mHeight.GetPercentValue() * containingBlockHeight);
      break;
    case eStyleUnit_Auto:
      // XXX See section 10.6 of the css2 spec and then write this code!
      break;
    }

    if (width > 0) {
      // XXX If the size constraint is a fixed content width then we also
      // need to set the max width as well, but then we bust tables...
#if 0
      minWidth = maxSize.width = width;
#else
      minWidth = width;
#endif
      widthConstraint = eHTMLFrameConstraint_FixedContent;
    }
    if (height > 0) {
      // XXX If the size constraint is a fixed content width then we also
      // need to set the max height as well...
#if 0
      minHeight = maxSize.height = height;
#else
      minHeight = height;
#endif
      heightConstraint = eHTMLFrameConstraint_FixedContent;
    }
  }

  // XXX this is probably a good place to calculate auto margins too
  // (section 10.3/10.6 of the spec)
}

//----------------------------------------------------------------------

nsFrameReflowState::nsFrameReflowState(nsIPresContext& aPresContext,
                                       const nsHTMLReflowState& aReflowState,
                                       const nsHTMLReflowMetrics& aMetrics)
  : nsHTMLReflowState(aReflowState),
    mPresContext(aPresContext)
{
  // While we skip around the reflow state that our parent gave us so
  // that the parentReflowState is linked properly, we don't want to
  // skip over it's reason.
  reason = aReflowState.reason;
  mNextRCFrame = nsnull;

  // Initialize max-element-size
  mComputeMaxElementSize = nsnull != aMetrics.maxElementSize;
  mMaxElementSize.width = 0;
  mMaxElementSize.height = 0;

  // Get style data that we need
  frame->GetStyleData(eStyleStruct_Text,
                      (const nsStyleStruct*&) mStyleText);
  frame->GetStyleData(eStyleStruct_Display,
                      (const nsStyleStruct*&) mStyleDisplay);
  frame->GetStyleData(eStyleStruct_Spacing,
                      (const nsStyleStruct*&) mStyleSpacing);

  // Calculate our border and padding value
  mStyleSpacing->CalcBorderPaddingFor(frame, mBorderPadding);

  // Set mNoWrap flag
  switch (mStyleText->mWhiteSpace) {
  case NS_STYLE_WHITESPACE_PRE:
  case NS_STYLE_WHITESPACE_NOWRAP:
    mNoWrap = PR_TRUE;
    break;
  default:
    mNoWrap = PR_FALSE;
    break;
  }

  // Set mDirection value
  mDirection = mStyleDisplay->mDirection;

  // See if this container frame will act as a root for margin
  // collapsing behavior.
  mIsMarginRoot = PR_FALSE;
  if ((0 != mBorderPadding.top) || (0 != mBorderPadding.bottom)) {
    mIsMarginRoot = PR_TRUE;
  }
  mCollapsedTopMargin = 0;
  mPrevBottomMargin = 0;
  mCarriedOutMarginFlags = 0;
}

nsFrameReflowState::~nsFrameReflowState()
{
}
