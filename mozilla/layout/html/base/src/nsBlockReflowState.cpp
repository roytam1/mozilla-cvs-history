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
#include "nsBlockFrame.h"
#include "nsBlockReflowContext.h"
#include "nsBlockBandData.h"
#include "nsBulletFrame.h"
#include "nsLineBox.h"

#include "nsFrameReflowState.h"
#include "nsLineLayout.h"
#include "nsInlineReflow.h"
#include "nsPlaceholderFrame.h"
#include "nsStyleConsts.h"
#include "nsHTMLIIDs.h"
#include "nsCSSRendering.h"
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

// XXX temporary for :first-letter support
#include "nsITextContent.h"
static NS_DEFINE_IID(kITextContentIID, NS_ITEXT_CONTENT_IID);/* XXX */

// XXX for IsEmptyLine
#include "nsTextFragment.h"

// XXX TODO:

// If I can add in IsAPlaceHolder then we can remove the mFloaters
// void array from the nsLineBox

#ifdef NS_DEBUG
#undef NOISY_FIRST_LINE
#undef REALLY_NOISY_FIRST_LINE
#undef NOISY_FIRST_LETTER
#undef NOISY_MAX_ELEMENT_SIZE
#undef NOISY_RUNIN
#undef NOISY_FLOATER_CLEARING
#undef NOISY_INCREMENTAL_REFLOW
#undef REFLOW_STATUS_COVERAGE
#else
#undef NOISY_FIRST_LINE
#undef REALLY_NOISY_FIRST_LINE
#undef NOISY_FIRST_LETTER
#undef NOISY_MAX_ELEMENT_SIZE
#undef NOISY_RUNIN
#undef NOISY_FLOATER_CLEARING
#undef NOISY_INCREMENTAL_REFLOW
#undef REFLOW_STATUS_COVERAGE
#endif

//----------------------------------------------------------------------

// Debugging support code

#ifdef NOISY_INCREMENTAL_REFLOW
static PRInt32 gNoiseIndent;
static const char* kReflowCommandType[] = {
  "FrameAppended",
  "FrameInserted",
  "FrameRemoved",
  "ContentChanged",
  "StyleChanged",
  "PullupReflow",
  "PushReflow",
  "CheckPullupReflow",
  "UserDefined",
};
#endif

#ifdef REALLY_NOISY_FIRST_LINE
static void
DumpStyleGeneaology(nsIFrame* aFrame, const char* gap)
{
  fputs(gap, stdout);
  aFrame->ListTag(stdout);
  fputs(name, out);
  printf(": ");
  nsIStyleContext* sc;
  aFrame->GetStyleContext(sc);
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

#ifdef NS_DEBUG
static void
VerifyLineLength(nsLineBox* aLine)
{
  nsIFrame* frame = aLine->mFirstChild;
  PRInt32 n = aLine->mChildCount;
  while (--n >= 0) {
    frame->GetNextSibling(&frame);
  }
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

class nsBlockReflowState : public nsFrameReflowState {
public:
  nsBlockReflowState(nsIPresContext& aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     const nsHTMLReflowMetrics& aMetrics,
                     nsLineLayout* aLineLayout);

  ~nsBlockReflowState();

  /**
   * Update the mCurrentBand data based on the current mY position.
   */
  void GetAvailableSpace();

  void InitFloater(nsPlaceholderFrame* aPlaceholderFrame);

  void AddFloater(nsPlaceholderFrame* aPlaceholderFrame,
                  PRBool aInitialReflow);

  void PlaceFloater(nsPlaceholderFrame* aFloater, PRBool& aIsLeftFloater);

  void PlaceBelowCurrentLineFloaters(nsVoidArray* aFloaters);

  void PlaceCurrentLineFloaters(nsVoidArray* aFloaters);

  void ClearFloaters(nscoord aY, PRUint8 aBreakType);

  PRBool IsLeftMostChild(nsIFrame* aFrame);

  PRBool IsAdjacentWithTop() const {
    return mY == mBorderPadding.top;
  }

  PRBool ShouldApplyTopMargin() const {
    return mIsMarginRoot || !IsAdjacentWithTop();
  }

  nsLineLayout* mLineLayout;
  nsInlineReflow* mInlineReflow;

  nsISpaceManager* mSpaceManager;
  nscoord mSpaceManagerX, mSpaceManagerY;
  nsBlockFrame* mBlock;
  nsBlockFrame* mNextInFlow;

  nsReflowStatus mReflowStatus;

  nsBlockFrame* mRunInFromFrame;
  nsBlockFrame* mRunInToFrame;

  PRUint8 mTextAlign;

  PRUintn mPrevMarginFlags;

  nscoord mBottomEdge;          // maximum Y

  PRBool mUnconstrainedWidth;
  PRBool mUnconstrainedHeight;
  nscoord mY;
  nscoord mKidXMost;
  nscoord mAscent, mDescent;

  // Previous child. This is used when pulling up a frame to update
  // the sibling list.
  nsIFrame* mPrevChild;

  nsVoidArray mPendingFloaters;

  nsLineBox* mCurrentLine;
  nsLineBox* mPrevLine;

  // The next list ordinal for counting list bullets
  PRInt32 mNextListOrdinal;

  nsBlockBandData mCurrentBand;
  nsRect mAvailSpaceRect;
};

// XXX This is vile. Make it go away
void
nsLineLayout::InitFloater(nsPlaceholderFrame* aFrame)
{
  mBlockReflowState->InitFloater(aFrame);
}
void
nsLineLayout::AddFloater(nsPlaceholderFrame* aFrame)
{
  mBlockReflowState->AddFloater(aFrame, PR_FALSE);
}

//----------------------------------------------------------------------

nsBlockReflowState::nsBlockReflowState(nsIPresContext& aPresContext,
                                       const nsHTMLReflowState& aReflowState,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       nsLineLayout* aLineLayout)
  : nsFrameReflowState(aPresContext, aReflowState, aMetrics)
{
  mInlineReflow = nsnull;

  mLineLayout = aLineLayout;

  mSpaceManager = aReflowState.spaceManager;

  // Translate into our content area and then save the 
  // coordinate system origin for later.
  mSpaceManager->Translate(mBorderPadding.left, mBorderPadding.top);
  mSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mBlock = (nsBlockFrame*) frame;
  mBlock->GetNextInFlow((nsIFrame*&)mNextInFlow);
  mKidXMost = 0;

  mRunInFromFrame = nsnull;
  mRunInToFrame = nsnull;

  mY = mAscent = mDescent = 0;
  mUnconstrainedWidth = availableWidth == NS_UNCONSTRAINEDSIZE;
  mUnconstrainedHeight = availableHeight == NS_UNCONSTRAINEDSIZE;
#ifdef NS_DEBUG
  if (!mUnconstrainedWidth && (availableWidth > 100000)) {
    mBlock->ListTag(stdout);
    printf(": bad parent: maxSize WAS %d,%d\n", availableWidth, availableHeight);
    if (availableWidth > 100000) {
      availableWidth = NS_UNCONSTRAINEDSIZE;
      mUnconstrainedWidth = PR_TRUE;
    }
  }
  if (!mUnconstrainedHeight && (availableHeight > 100000)) {
    mBlock->ListTag(stdout);
    printf(": bad parent: maxSize WAS %d,%d\n", availableWidth, availableHeight);
    if (availableHeight > 100000) {
      availableHeight = NS_UNCONSTRAINEDSIZE;
      mUnconstrainedHeight = PR_TRUE;
    }
  }
#endif

  mTextAlign = mStyleText->mTextAlign;

  nscoord lr = mBorderPadding.left + mBorderPadding.right;
  mY = mBorderPadding.top;

  if (HaveFixedContentWidth()) {
    // The CSS2 spec says that the width attribute defines the width
    // of the "content area" which does not include the border
    // padding. So we add those back in.
    mBorderArea.width = computedWidth + lr;
    mContentArea.width = computedWidth;
  }
  else {
    if (mUnconstrainedWidth) {
      mBorderArea.width = NS_UNCONSTRAINEDSIZE;
      mContentArea.width = NS_UNCONSTRAINEDSIZE;
    }
    else {
      mBorderArea.width = availableWidth;
      mContentArea.width = availableWidth - lr;
    }
  }

  mBorderArea.height = availableHeight;
  mContentArea.height = availableHeight;
  mBottomEdge = availableHeight;
  if (!mUnconstrainedHeight) {
    mBottomEdge -= mBorderPadding.bottom;
  }
  mCurrentBand.Init(mSpaceManager, mContentArea);

  mPrevChild = nsnull;
  mCurrentLine = nsnull;
  mPrevLine = nsnull;
}

nsBlockReflowState::~nsBlockReflowState()
{
  // Restore the coordinate system
  mSpaceManager->Translate(-mBorderPadding.left, -mBorderPadding.top);
}

// Get the available reflow space for the current y coordinate. The
// available space is relative to our coordinate system (0,0) is our
// upper left corner.
void
nsBlockReflowState::GetAvailableSpace()
{
  nsISpaceManager* sm = mSpaceManager;

#ifdef NS_DEBUG
  // Verify that the caller setup the coordinate system properly
  nscoord wx, wy;
  sm->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mSpaceManagerX) && (wy == mSpaceManagerY),
               "bad coord system");
#endif

  mCurrentBand.GetAvailableSpace(mY - mBorderPadding.top, mAvailSpaceRect);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("nsBlockReflowState::GetAvailableSpace: band={%d,%d,%d,%d} count=%d",
      mAvailSpaceRect.x, mAvailSpaceRect.y,
      mAvailSpaceRect.width, mAvailSpaceRect.height,
      mCurrentBand.GetTrapezoidCount()));
#ifdef NOISY_INCREMENTAL_REFLOW
  if (reason == eReflowReason_Incremental) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d count=%d\n",
           mAvailSpaceRect.x, mAvailSpaceRect.y,
           mAvailSpaceRect.width, mAvailSpaceRect.height,
           mCurrentBand.GetTrapezoidCount());
  }
#endif
}

//----------------------------------------------------------------------

#define NS_BLOCK_FRAME_CID \
 { 0xa6cf90df, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

static const nsIID kBlockFrameCID = NS_BLOCK_FRAME_CID;

nsresult
NS_NewBlockFrame(nsIFrame*& aNewFrame, PRUint32 aFlags)
{
  nsBlockFrame* it = new nsBlockFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  it->SetFlags(aFlags);
  aNewFrame = it;
  return NS_OK;
}

nsBlockFrame::nsBlockFrame()
{
}

nsBlockFrame::~nsBlockFrame()
{
  NS_IF_RELEASE(mFirstLineStyle);
  NS_IF_RELEASE(mFirstLetterStyle);
  nsTextRun::DeleteTextRuns(mTextRuns);
}

NS_IMETHODIMP
nsBlockFrame::DeleteFrame(nsIPresContext& aPresContext)
{
  // Outside bullets are not in our child-list so check for them here
  // and delete them when present.
  if (HaveOutsideBullet()) {
    mBullet->DeleteFrame(aPresContext);
    mBullet = nsnull;
  }

  nsLineBox::DeleteLineList(aPresContext, mLines);
  nsLineBox::DeleteLineList(aPresContext, mOverflowLines);

  mFloaters.DeleteFrames(aPresContext);

  return nsBlockFrameSuper::DeleteFrame(aPresContext);
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
  return nsBlockFrameSuper::QueryInterface(aIID, aInstancePtr);
}

static nsresult
ReResolveLineList(nsIPresContext* aPresContext,
                  nsLineBox* aLine,
                  nsIStyleContext* aStyleContext)
{
  nsresult rv = NS_OK;
  while (nsnull != aLine) {
    nsIFrame* child = aLine->mFirstChild;
    PRInt32 n = aLine->mChildCount;
    while ((--n >= 0) && NS_SUCCEEDED(rv)) {
      rv = child->ReResolveStyleContext(aPresContext, aStyleContext);
      child->GetNextSibling(&child);
    }
    aLine = aLine->mNext;
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::ReResolveStyleContext(nsIPresContext* aPresContext,
                                    nsIStyleContext* aParentContext)
{
  nsIStyleContext* oldContext = mStyleContext;

  // NOTE: using nsFrame's ReResolveStyleContext method to avoid
  // useless version in base classes.
  nsresult rv = nsFrame::ReResolveStyleContext(aPresContext, aParentContext);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (oldContext != mStyleContext) {
    // Re-resolve the :first-line pseudo style context
    if (nsnull == mPrevInFlow) {
      nsIStyleContext* newFirstLineStyle;
      aPresContext->ProbePseudoStyleContextFor(mContent,
                                               nsHTMLAtoms::firstLinePseudo,
                                               mStyleContext,
                                               PR_FALSE,
                                               &newFirstLineStyle);
      if (newFirstLineStyle != mFirstLineStyle) {
        NS_IF_RELEASE(mFirstLineStyle);
        mFirstLineStyle = newFirstLineStyle;
      }
      else {
        NS_IF_RELEASE(newFirstLineStyle);
      }

      // Re-resolve the :first-letter pseudo style context
      nsIStyleContext* newFirstLetterStyle;
      aPresContext->ProbePseudoStyleContextFor(mContent,
                                               nsHTMLAtoms::firstLetterPseudo,
                                               (nsnull != mFirstLineStyle
                                                ? mFirstLineStyle
                                                : mStyleContext),
                                               PR_FALSE,
                                               &newFirstLetterStyle);
      if (newFirstLetterStyle != mFirstLetterStyle) {
        NS_IF_RELEASE(mFirstLetterStyle);
        mFirstLetterStyle = newFirstLetterStyle;
      }
      else {
        NS_IF_RELEASE(newFirstLetterStyle);
      }
    }

    // Update the child frames on each line
    nsLineBox* line = mLines;
    while (nsnull != line) {
      nsIFrame* child = line->mFirstChild;
      PRInt32 n = line->mChildCount;
      while ((--n >= 0) && NS_SUCCEEDED(rv)) {
        if (line == mLines) {
          rv = child->ReResolveStyleContext(aPresContext,
                                            (nsnull != mFirstLineStyle
                                             ? mFirstLineStyle
                                             : mStyleContext));
        }
        else {
          rv = child->ReResolveStyleContext(aPresContext, mStyleContext);
        }
        child->GetNextSibling(&child);
      }
      line = line->mNext;
    }

    if (NS_SUCCEEDED(rv) && (nsnull != mOverflowLines)) {
      rv = ReResolveLineList(aPresContext, mOverflowLines, mStyleContext);
    }
    if (NS_SUCCEEDED(rv) && (nsnull != mPrevInFlow)) {
      nsLineBox* lines = ((nsBlockFrame*)mPrevInFlow)->mOverflowLines;
      if (nsnull != lines) {
        rv = ReResolveLineList(aPresContext, lines, mStyleContext);
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsBlockFrame::IsSplittable(nsSplittableType& aIsSplittable) const
{
  aIsSplittable = NS_FRAME_SPLITTABLE_NON_RECTANGULAR;
  return NS_OK;
}

static void
ListTextRuns(FILE* out, PRInt32 aIndent, nsTextRun* aRuns)
{
  while (nsnull != aRuns) {
    aRuns->List(out, aIndent);
    aRuns = aRuns->GetNext();
  }
}

NS_METHOD
nsBlockFrame::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;

  nsAutoString tagString;
  if (nsnull != mContent) {
    nsIAtom* tag;
    mContent->GetTag(tag);
    if (tag != nsnull)  {
      tag->ToString(tagString);
      NS_RELEASE(tag);
    }
  }

  // Indent
  for (i = aIndent; --i >= 0; ) fputs("  ", out);

  // Output the tag
  ListTag(out);
  nsIView* view;
  GetView(&view);
  if (nsnull != view) {
    fprintf(out, " [view=%p]", view);
  }

  // Output the flow linkage
  if (nsnull != mPrevInFlow) {
    fprintf(out, " prev-in-flow=%p", mPrevInFlow);
  }
  if (nsnull != mNextInFlow) {
    fprintf(out, " next-in-flow=%p", mNextInFlow);
  }

  // Output the rect and state
  out << mRect;
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fputs("<\n", out);
  aIndent++;

  // Output the lines
  if (nsnull != mLines) {
    nsLineBox* line = mLines;
    while (nsnull != line) {
      line->List(out, aIndent);
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
        kid->List(out, aIndent + 1);
        kid->GetNextSibling(&kid);
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
    NS_IF_RELEASE(listName);
  }

  // Output the text-runs
  if (nsnull != mTextRuns) {
    for (i = aIndent; --i >= 0; ) fputs("  ", out);
    fputs("text-runs <\n", out);

    ListTextRuns(out, aIndent + 1, mTextRuns);

    for (i = aIndent; --i >= 0; ) fputs("  ", out);
    fputs(">\n", out);
  }

  aIndent--;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);

  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Block", aResult);
}

NS_IMETHODIMP
nsBlockFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsHTMLAtoms::blockFrame; 
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
    *aFirstChild = mBullet;
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

#if 0
NS_IMETHODIMP
nsBlockFrame::ComputeCollapsedMargins(nsIPresContext& aPresContext,
                                       const nsReflowState* aParentReflowState,
                                       nscoord* aTopMarginResult,
                                       nscoord* aBottomMarginResult)
{
  NS_PRECONDITION((nsnull != aTopMarginResult) &&
                  (nsnull != aBottomMarginResult), "null ptr");
  if ((nsnull != aTopMarginResult) || (nsnull != aBottomMarginResult)) {
    nsMargin myMargin(0, 0, 0, 0);
    if (0 == (BLOCK_IS_INLINE & mFlags)) {
      nsHTMLReflowState::ComputeMarginFor(this, aParentReflowState, myMargin);
    }

    // Find the first appropriate line (skipping over an empty line if
    // present) that contains a block that can be used for a
    // parent-child margin collapse.
    nsLineBox* firstLine = nsnull;
    nscoord firstLineTopMargin = 0;
    if (nsnull != aTopMarginResult) {
      nsLineBox* line = mLines;
      if (nsnull != line) {
        if (line->IsEmptyLine()) {
          line = line->mNext;
        }
        if ((nsnull != line) && line->IsBlock()) {
          firstLine = line;
        }
      }
    }

    // Find the last appropriate line (skipping over an empty line)...
    nsLineBox* lastLine = nsnull;
    nscoord lastLineBottomMargin = 0;
    if (nsnull != aBottomMarginResult) {
      // Find the last line (line) and the previous to the last line
      // (prevLine)
      nsLineBox* line = mLines;
      nsLineBox* prevLine = nsnull;
      while (nsnull != line) {
        nsLineBox* next = line->mNext;
        if (nsnull == next) {
          break;
        }
        prevLine = line;
        line = next;
      }
      if (nsnull != line) {
        if (line->IsEmptyLine()) {
          line = prevLine;
        }
        if ((nsnull != line) && line->IsBlock()) {
          lastLine = line;
        }
      }
    }

    if (nsnull != firstLine) {
      if (lastLine == firstLine) {
        ComputeCollapsedMargins(aPresContext, nsnull,
                                &firstLineTopMargin,
                                &lastLineBottomMargin);
      }
      else {
        ComputeCollapsedMargins(aPresContext, nsnull,
                                &firstLineTopMargin,
                                nsnull);
      }
      firstLineTopMargin
    }
    else if (nsnull != lastLine) {
      ComputeCollapsedMargins(aPresContext, nsnull,
                              nsnull,
                              &lastLineBottomMargin);
    }

    if ((nsnull != aTopMarginResult) && (nsnull != aBottomMarginResult)) {
      nscoord topMargin =
        nsBlockReflowContext::MaxMargin(myMargin.top, firstLineTopMargin);
      *aTopMarginResult = topMargin;
      nscoord bottomMargin =
        nsBlockReflowContext::MaxMargin(myMargin.bottom, lastLineBottomMargin);
      *aBottomMarginResult = bottomMargin;
    }
    else if (nsnull != aTopMarginResult) {
      nscoord topMargin =
        nsBlockReflowContext::MaxMargin(myMargin.top, firstLineTopMargin);
      *aTopMarginResult = topMargin;
    }
    else {
      nscoord bottomMargin =
        nsBlockReflowContext::MaxMargin(myMargin.bottom, lastLineBottomMargin);
      *aBottomMarginResult = bottomMargin;
    }
  }
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsBlockFrame::Reflow(nsIPresContext&          aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsBlockFrame::Reflow: maxSize=%d,%d reason=%d",
                  aReflowState.availableWidth,
                  aReflowState.availableHeight,
                  aReflowState.reason));

  // Replace parent provided reflow state with our own significantly
  // more extensive version.
  nsLineLayout ll(aPresContext, aReflowState.spaceManager);
  nsLineLayout* lineLayout = &ll;
  nsBlockReflowState state(aPresContext, aReflowState, aMetrics, lineLayout);
  if (NS_BLOCK_MARGIN_ROOT & mFlags) {
    state.mIsMarginRoot = PR_TRUE;
  }
  lineLayout->Init(&state);

  // Prepare inline-reflow engine
  nsInlineReflow inlineReflow(*lineLayout, state, this, PR_TRUE);
  state.mInlineReflow = &inlineReflow;
  lineLayout->PushInline(&inlineReflow);

  nsresult rv = NS_OK;
  nsIFrame* target;
  switch (state.reason) {
  case eReflowReason_Initial:
    rv = PrepareInitialReflow(state);
    mState &= ~NS_FRAME_FIRST_REFLOW;
    break;

  case eReflowReason_Incremental:
    state.reflowCommand->GetTarget(target);
    if (this == target) {
      nsIReflowCommand::ReflowType type;
      state.reflowCommand->GetType(type);
      switch (type) {
      case nsIReflowCommand::FrameAppended:
        rv = PrepareFrameAppendedReflow(state);
        break;
      case nsIReflowCommand::FrameInserted:
        rv = PrepareFrameInsertedReflow(state);
        break;
      case nsIReflowCommand::FrameRemoved:
        rv = PrepareFrameRemovedReflow(state);
        break;
      case nsIReflowCommand::StyleChanged:
        rv = PrepareStyleChangedReflow(state);
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
      state.reflowCommand->GetNext(state.mNextRCFrame);

      // Now do the reflow
      rv = PrepareChildIncrementalReflow(state);
    }
    break;

  case eReflowReason_Resize:
  default:
    DrainOverflowLines();
    rv = PrepareResizeReflow(state);
    break;
  }

  // Now reflow...
  rv = ReflowDirtyLines(state);
  aStatus = state.mReflowStatus;
  if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
    printf("XXX: block is not complete\n");
  }

  // XXX get rid of this!
  BuildFloaterList();

  // Compute our final size
  ComputeFinalSize(state, aMetrics);
  lineLayout->PopInline();

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
               ("exit nsBlockFrame::Reflow: size=%d,%d reflowStatus=%d rv=%x",
                aMetrics.width, aMetrics.height, aStatus, rv));
  return rv;
}

// XXX make this virtual
// XXX factor into its component pieces
void
nsBlockFrame::ComputeFinalSize(nsBlockReflowState& aState,
                                nsHTMLReflowMetrics& aMetrics)
{
  // XXX handle floater problems this way...
  PRBool isFixedWidth = aState.HaveFixedContentWidth();
  PRBool isFixedHeight = aState.HaveFixedContentHeight();

#if 0
  if (NS_BODY_SHRINK_WRAP & mFlags) {
    isFixedWidth = PR_FALSE;
    isFixedHeight = PR_FALSE;
  }
#endif

  // Compute final width
  if (isFixedWidth) {
    // Use style defined width
    aMetrics.width = aState.mBorderPadding.left + aState.computedWidth +
      aState.mBorderPadding.right;
  }
  else {
    nscoord computedWidth = aState.mKidXMost + aState.mBorderPadding.right;
    PRBool compact = PR_FALSE;
    if (NS_STYLE_DISPLAY_COMPACT == aState.mStyleDisplay->mDisplay) {
      // If we are display: compact AND we have no lines or we have
      // exactly one line and that line is not a block line AND that
      // line doesn't end in a BR of any sort THEN we remain a compact
      // frame.
      if ((nsnull == mLines) ||
          ((nsnull == mLines->mNext) && !mLines->IsBlock() &&
           (NS_STYLE_CLEAR_NONE == mLines->mBreakType) &&
           (computedWidth <= aState.mCompactMarginWidth))) {
        compact = PR_TRUE;
      }
    }

    // There are two options here. We either shrink wrap around our
    // contents or we fluff out to the maximum available width. Note:
    // We always shrink wrap when given an unconstrained width.
    if ((0 == (NS_BLOCK_SHRINK_WRAP & mFlags)) &&
        !aState.mUnconstrainedWidth &&
        !compact) {
      // Fluff out to the max width if we aren't already that wide
      if (computedWidth < aState.availableWidth) {
        computedWidth = aState.availableWidth;
      }
    }
    aMetrics.width = computedWidth;
  }

  // Compute final height
  if (isFixedHeight) {
    // Use style defined height
    aMetrics.height = aState.mBorderPadding.top + aState.computedHeight +
      aState.mBorderPadding.bottom;
  }
  else {
    // Shrink wrap our height around our contents.
    if (aState.mIsMarginRoot) {
      // When we are a margin root make sure that our last childs
      // bottom margin is fully applied.
      // XXX check for a fit
      aState.mY += aState.mPrevBottomMargin;
    }
    aState.mY += aState.mBorderPadding.bottom;
    aMetrics.height = aState.mY;
  }

  // Return top and bottom margin information
  if (aState.mIsMarginRoot) {
    aMetrics.mCarriedOutTopMargin = 0;
    aMetrics.mCarriedOutBottomMargin = 0;
  }
  else {
    aMetrics.mCarriedOutTopMargin = aState.mCarriedOutTopMargin;
    aMetrics.mCarriedOutBottomMargin = aState.mPrevBottomMargin;
  }

  // Special check for zero sized content: If our content is zero
  // sized then we collapse into nothingness.
  PRBool emptyFrame = PR_FALSE;
  // We need to check the specified width and see if it's 'auto'
  const nsStylePosition* position;
  aState.frame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&) position);
  PRIntn specifiedWidthUnit = position->mWidth.GetUnit();
  if ((eStyleUnit_Auto == specifiedWidthUnit) &&
      (NS_AUTOHEIGHT == aState.computedHeight) &&
      ((0 == aState.mKidXMost - aState.mBorderPadding.left) &&
       (0 == aState.mY - aState.mBorderPadding.top))) {
    aMetrics.width = 0;
    aMetrics.height = 0;
    aState.mAscent = 0;
    aState.mDescent = 0;
    emptyFrame = PR_TRUE;
  }

  aMetrics.ascent = aMetrics.height;
  aMetrics.descent = 0;

  if (aState.mComputeMaxElementSize) {
    nscoord maxWidth, maxHeight;
    if (emptyFrame) {
      // When a frame is empty it must not provide any
      // max-element-size information.
      maxWidth = maxHeight = 0;
    }
    else {
      if (aState.mNoWrap) {
        // When no-wrap is true the max-element-size.width is the
        // width of the widest line plus the right border. Note that
        // aState.mKidXMost already has the left border factored into
        // it
        maxWidth = aState.mKidXMost + aState.mBorderPadding.right;
      }
      else {
        // Add in border and padding dimensions to already computed
        // max-element-size values.
        maxWidth = aState.mMaxElementSize.width +
          aState.mBorderPadding.left + aState.mBorderPadding.right;
      }
      maxHeight = aState.mMaxElementSize.height +
        aState.mBorderPadding.top + aState.mBorderPadding.bottom;
    }

    // Store away the final value
    aMetrics.maxElementSize->width = maxWidth;
    aMetrics.maxElementSize->height = maxHeight;
#ifdef NOISY_MAX_ELEMENT_SIZE
    ListTag(stdout);
    printf(": max-element-size:%d,%d desired:%d,%d maxSize:%d,%d\n",
           maxWidth, maxHeight, aMetrics.width, aMetrics.height,
           aState.availableWidth, aState.availableHeight);
#endif
  }

  // Compute the combined area of our children
  // XXX take into account the overflow->clip property!
  nscoord x0 = 0, y0 = 0, x1 = aMetrics.width, y1 = aMetrics.height;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    // Compute min and max x/y values for the reflowed frame's
    // combined areas
    nscoord x = line->mCombinedArea.x;
    nscoord y = line->mCombinedArea.y;
    nscoord xmost = x + line->mCombinedArea.width;
    nscoord ymost = y + line->mCombinedArea.height;
    if (x < x0) x0 = x;
    if (xmost > x1) x1 = xmost;
    if (y < y0) y0 = y;
    if (ymost > y1) y1 = ymost;

    // If the line has floaters, factor those in as well
    nsVoidArray* floaters = line->mFloaters;
    if (nsnull != floaters) {
      PRInt32 i, n = floaters->Count();
      for (i = 0; i < n; i++) {
        nsPlaceholderFrame* ph = (nsPlaceholderFrame*) floaters->ElementAt(i);
        nsIFrame* frame = ph->GetAnchoredItem();
        // XXX This is wrong! The floater may have a combined area
        // that exceeds its bounding box!
        nsRect r;
        frame->GetRect(r);
        if (r.x < x0) x0 = r.x;
        if (r.XMost() > x1) x1 = r.XMost();
        if (r.y < y0) y0 = r.y;
        if (r.YMost() > y1) y1 = r.YMost();
      }
    }
    line = line->mNext;
  }
  aMetrics.mCombinedArea.x = x0;
  aMetrics.mCombinedArea.y = y0;
  aMetrics.mCombinedArea.width = x1 - x0;
  aMetrics.mCombinedArea.height = y1 - y0;

  if (nsnull != mBullet) {
    nsRect r;
    mBullet->GetRect(r);
    nscoord x0 = aMetrics.mCombinedArea.x;
    nscoord y0 = aMetrics.mCombinedArea.y;
    nscoord x1 = x0 + aMetrics.mCombinedArea.width;
    nscoord y1 = y0 + aMetrics.mCombinedArea.height;
    if (r.x < x0) x0 = r.x;
    if (r.XMost() > x1) x1 = r.XMost();
    if (r.y < y0) y0 = r.y;
    if (r.YMost() > y1) y1 = r.YMost();
    aMetrics.mCombinedArea.x = x0;
    aMetrics.mCombinedArea.y = y0;
    aMetrics.mCombinedArea.width = x1 - x0;
    aMetrics.mCombinedArea.height = y1 - y0;

    // If the combined area of our children exceeds our bounding box
    // then set the NS_FRAME_OUTSIDE_CHILDREN flag, otherwise clear it.
    if ((aMetrics.mCombinedArea.x < 0) ||
        (aMetrics.mCombinedArea.y < 0) ||
        (aMetrics.mCombinedArea.XMost() > aMetrics.width) ||
        (aMetrics.mCombinedArea.YMost() > aMetrics.height)) {
      mState |= NS_FRAME_OUTSIDE_CHILDREN;
    }
    else {
      mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
    }
  }

  // If the combined area of our children exceeds our bounding box
  // then set the NS_FRAME_OUTSIDE_CHILDREN flag, otherwise clear it.
  if ((aMetrics.mCombinedArea.x < 0) ||
      (aMetrics.mCombinedArea.y < 0) ||
      (aMetrics.mCombinedArea.XMost() > aMetrics.width) ||
      (aMetrics.mCombinedArea.YMost() > aMetrics.height)) {
    mState |= NS_FRAME_OUTSIDE_CHILDREN;
  }
  else {
    mState &= ~NS_FRAME_OUTSIDE_CHILDREN;
  }
#if XXX
ListTag(stdout);
printf(": => carried=%d,%d\n", aMetrics.carriedOutTopMargin, aMetrics.carriedOutBottomMargin);
#endif
}

nsresult
nsBlockFrame::PrepareInitialReflow(nsBlockReflowState& aState)
{
  if ((nsnull == mPrevInFlow) && (nsnull != aState.mRunInFrame)) {
#ifdef NOISY_RUNIN
    ListTag(stdout);
    printf(": run-in from: ");
    aReflowState.mRunInFrame->ListTag(stdout);
    printf("\n");
#endif
    // Take frames away from the run-in frame
    TakeRunInFrames(aState.mRunInFrame);
  }

  DrainOverflowLines();
  PrepareResizeReflow(aState);
  ComputeTextRuns(aState);
  RenumberLists(aState);
  return NS_OK;
}

//XXX gotta go
nsresult
nsBlockFrame::PrepareFrameAppendedReflow(nsBlockReflowState& aState)
{
  RenumberLists(aState);
  nsresult rv = ComputeTextRuns(aState);
  return rv;
}

//XXX gotta go
nsresult
nsBlockFrame::PrepareFrameInsertedReflow(nsBlockReflowState& aState)
{
  nsresult rv = PrepareResizeReflow(aState);
  RenumberLists(aState);
  rv = ComputeTextRuns(aState);
  return rv;
}

//XXX gotta go
nsresult
nsBlockFrame::PrepareFrameRemovedReflow(nsBlockReflowState& aState)
{
  nsresult rv = PrepareResizeReflow(aState);
  RenumberLists(aState);
  rv = ComputeTextRuns(aState);
  return rv;
}

nsresult
nsBlockFrame::PrepareChildIncrementalReflow(nsBlockReflowState& aState)
{
  // If by chance we are inside a table, then give up and reflow
  // everything because we don't cache max-element-size information in
  // the lines.
  if (aState.mComputeMaxElementSize) {
    return PrepareResizeReflow(aState);
  }

  // Determine the line being impacted
  PRBool isFloater;
  nsLineBox* line = FindLineFor(aState.mNextRCFrame, isFloater);
  if (nsnull == line) {
    // This can't happen, but just in case it does...
    return PrepareResizeReflow(aState);
  }

  // XXX: temporary: If the child frame is a floater then punt
  if (isFloater) {
    return PrepareResizeReflow(aState);
  }

  // XXX need code for run-in/compact

  // Mark (at least) the affected line dirty.
  line->MarkDirty();
  if (aState.mNoWrap || line->IsBlock()) {
    // If we aren't wrapping then we know for certain that any changes
    // to a childs reflow can't affect the line that follows. This is
    // also true if the line is a block line.
  }
  else {
    // XXX: temporary: For now we are conservative and mark this line
    // and any inline lines that follow it dirty.
    line = line->mNext;
    while (nsnull != line) {
      if (line->IsBlock()) {
        break;
      }
      line->MarkDirty();
      line = line->mNext;
    }
  }
  return NS_OK;
}

nsresult
nsBlockFrame::PrepareStyleChangedReflow(nsBlockReflowState& aState)
{
  // XXX temporary
  return PrepareResizeReflow(aState);
}

nsresult
nsBlockFrame::PrepareResizeReflow(nsBlockReflowState& aState)
{
  // Mark everything dirty
  nsLineBox* line = mLines;
  while (nsnull != line) {
    line->MarkDirty();
    line = line->mNext;
  }
  return NS_OK;
}

//----------------------------------------

nsLineBox*
nsBlockFrame::FindLineFor(nsIFrame* aFrame, PRBool& aIsFloaterResult)
{
  aIsFloaterResult = PR_FALSE;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (line->Contains(aFrame)) {
      return line;
    }
    if (nsnull != line->mFloaters) {
      nsVoidArray& a = *line->mFloaters;
      PRInt32 i, n = a.Count();
      for (i = 0; i < n; i++) {
        nsPlaceholderFrame* ph = (nsPlaceholderFrame*) a[i];
        if (aFrame == ph->GetAnchoredItem()) {
          aIsFloaterResult = PR_TRUE;
          return line;
        }
      }
    }
    line = line->mNext;
  }
  return line;
}

nsresult
nsBlockFrame::RecoverStateFrom(nsBlockReflowState& aState,
                                nsLineBox* aLine,
                                nscoord aDeltaY)
{
  aState.mCurrentLine = aLine;

  // Recover xmost
  nscoord xmost = aLine->mBounds.XMost();
  if (xmost > aState.mKidXMost) {
    aState.mKidXMost = xmost;
  }

  // Recover the natural (un-collapsed margins) for the child
  nsMargin childMargins(0, 0, 0, 0);
  if (aLine->IsBlock()) {
    nsIFrame* frame = aLine->mFirstChild;
    const nsStyleSpacing* spacing;
    frame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
    nsBlockReflowContext::ComputeMarginsFor(aState.mPresContext, frame,
                                            spacing, aState, childMargins);
  }

  // Recompute running margin value (aState.mPrevBottomMargin). Also
  // recover the aState.carriedOutTopMargin, when appropriate.
  nscoord topMargin, bottomMargin;
  nsBlockReflowContext::CollapseMargins(childMargins,
                                        aLine->GetCarriedOutTopMargin(),
                                        aLine->GetCarriedOutBottomMargin(),
                                        aLine->GetHeight(),
                                        aState.mPrevBottomMargin,
                                        topMargin, bottomMargin);
  aState.mPrevBottomMargin = bottomMargin;
  if (!aState.ShouldApplyTopMargin()) {
    aState.mCarriedOutTopMargin = topMargin;
  }

  if (0 != aDeltaY) {
    // Move this lines frames by the current delta value
    SlideFrames(aState.mPresContext, aState.mSpaceManager, aLine, aDeltaY);
    SlideFloaters(aState.mPresContext, aState.mSpaceManager, aLine, aDeltaY,
                  PR_FALSE);
  }
  if (nsnull != aLine->mFloaters) {
    aState.mY = aLine->mBounds.y;
    aState.PlaceCurrentLineFloaters(aLine->mFloaters);
    aState.mY = aLine->mBounds.YMost();
    aState.PlaceBelowCurrentLineFloaters(aLine->mFloaters);
  }

  // Advance Y to be below the line.
  aState.mY = aLine->mBounds.YMost();

  // XXX_fix_me: if the line has clear-before semantics then figure out
  // if we need to do anything here or not!

  // Apply any clear before/after semantics the line might have
  if (!aLine->IsBlock() && (NS_STYLE_CLEAR_NONE != aLine->mBreakType)) {
    // Apply clear
    switch (aLine->mBreakType) {
    case NS_STYLE_CLEAR_LEFT:
    case NS_STYLE_CLEAR_RIGHT:
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      // XXX_fix_me is this the right y value to use? or should we use
      // the previous aState.mY?
      aState.ClearFloaters(aState.mY, aLine->mBreakType);
      break;
    }
  }

  return NS_OK;
}

/**
 * Propogate reflow "damage" from the just reflowed line (aLine) to
 * any subsequent lines that were affected. The only thing that causes
 * damage is a change to the impact that floaters make.
 */
void
nsBlockFrame::PropogateReflowDamage(nsBlockReflowState& aState,
                                    nsLineBox* aLine,
                                    nscoord aDeltaY)
{
  if (aLine->mCombinedArea.YMost() > aLine->mBounds.YMost()) {
    // The line has an object that extends outside of its bounding box.
    nscoord impactY0 = aLine->mCombinedArea.y;
    nscoord impactY1 = aLine->mCombinedArea.YMost();
#ifdef NOISY_INCREMENTAL_REFLOW
    if (aState.reason == eReflowReason_Incremental) {
      IndentBy(stdout, gNoiseIndent);
      printf("impactY0=%d impactY1=%d deltaY=%d\n",
             impactY0, impactY1, aDeltaY);
    }
#endif

    // XXX Because we don't know what it is (it might be a floater; it
    // might be something that is just relatively positioned) we
    // *assume* that it's a floater and that lines that follow will
    // need reflowing.

    // Note: we cannot stop after the first non-intersecting line
    // because lines might be overlapping because of negative margins.
    nsLineBox* next = aLine->mNext;
    while (nsnull != next) {
      nscoord lineY0 = next->mBounds.y + aDeltaY;
      nscoord lineY1 = lineY0 + next->mBounds.height;
      if ((lineY0 < impactY1) && (impactY0 < lineY1)) {
#ifdef NOISY_INCREMENTAL_REFLOW
        if (aState.reason == eReflowReason_Incremental) {
          IndentBy(stdout, gNoiseIndent);
          printf("line=%p setting dirty\n", next);
        }
#endif
        next->MarkDirty();
      }
      next = next->mNext;
    }
  }
}

/**
 * Reflow the dirty lines
 */
nsresult
nsBlockFrame::ReflowDirtyLines(nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  PRBool keepGoing = PR_TRUE;

  // Inform line layout of where the text runs are
  aState.mLineLayout->SetReflowTextRuns(mTextRuns);

#ifdef NOISY_INCREMENTAL_REFLOW
  if (aState.reason == eReflowReason_Incremental) {
    nsIReflowCommand::ReflowType type;
    aState.reflowCommand->GetType(type);
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": incrementally reflowing dirty lines: type=%s(%d)\n",
           kReflowCommandType[type], type);
    gNoiseIndent++;
  }
#endif

  // Reflow the lines that are already ours
  aState.mPrevLine = nsnull;
  nsLineBox* line = mLines;
  nscoord deltaY = 0;
  while (nsnull != line) {
#ifdef NOISY_INCREMENTAL_REFLOW
    if (aState.reason == eReflowReason_Incremental) {
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d dirty=%s oldBounds=%d,%d,%d,%d deltaY=%d\n",
             line, aState.mY, line->IsDirty() ? "yes" : "no",
             line->mBounds, deltaY);
      gNoiseIndent++;
    }
#endif
    if (line->IsDirty()) {
      // Compute the dirty lines "before" YMost, after factoring in
      // the running deltaY value - the running value is implicit in
      // aState.mY.
      nscoord oldHeight = line->mBounds.height;

      // Reflow the dirty line
      rv = ReflowLine(aState, line, &keepGoing);
      if (NS_FAILED(rv)) {
        return rv;
      }
      DidReflowLine(aState, line, keepGoing);
      if (!keepGoing) {
        if (0 == line->ChildCount()) {
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
        PropogateReflowDamage(aState, line, deltaY);
      }
    }
    else {
      // XXX what if the slid line doesn't fit because we are in a
      // vertically constrained situation?
      // Recover state as if we reflowed this line
      RecoverStateFrom(aState, line, deltaY);
    }
#ifdef NOISY_INCREMENTAL_REFLOW
    if (aState.reason == eReflowReason_Incremental) {
      gNoiseIndent--;
      IndentBy(stdout, gNoiseIndent);
      printf("line=%p mY=%d newBounds=%d,%d,%d,%d deltaY=%d\n",
             line, aState.mY, line->mBounds, deltaY);
    }
#endif

    // If this is an inline frame then its time to stop
    aState.mPrevLine = line;
    line = line->mNext;
    aState.mLineLayout->NextLine();
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
    if (0 == line->ChildCount()) {
      // The line is empty. Try the next one.
      NS_ASSERTION(nsnull == line->mFirstChild, "bad empty line");
      delete line;
      continue;
    }

    // XXX move to a subroutine: run-in, overflow, pullframe and this do this
    // Make the children in the line ours.
    nsIFrame* frame = line->mFirstChild;
    nsIFrame* lastFrame = nsnull;
    PRInt32 n = line->ChildCount();
    while (--n >= 0) {
      frame->SetParent(this);
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
      rv = ReflowLine(aState, line, &keepGoing);
      if (NS_FAILED(rv)) {
        return rv;
      }
      DidReflowLine(aState, line, keepGoing);
      if (!keepGoing) {
        if (0 == line->ChildCount()) {
          DeleteLine(aState, line);
        }
        break;
      }

      // If this is an inline frame then its time to stop
      aState.mPrevLine = line;
      line = line->mNext;
      aState.mLineLayout->NextLine();
    }
  }

#ifdef NOISY_INCREMENTAL_REFLOW
  if (aState.reason == eReflowReason_Incremental) {
    gNoiseIndent--;
    IndentBy(stdout, gNoiseIndent);
    ListTag(stdout);
    printf(": done reflowing dirty lines (status=%x, mLimitToOneLine=%d)\n",
           aState.mReflowStatus, aState.mLimitToOneLine);
  }
#endif

  return rv;
}

void
nsBlockFrame::DeleteLine(nsBlockReflowState& aState,
                         nsLineBox* aLine)
{
  NS_PRECONDITION(0 == aLine->ChildCount(), "can't delete !empty line");
  if (0 == aLine->ChildCount()) {
    if (nsnull == aState.mPrevLine) {
      NS_ASSERTION(aLine == mLines, "huh");
      mLines = nsnull;
    }
    else {
      NS_ASSERTION(aState.mPrevLine->mNext == aLine, "bad prev-line");
      aState.mPrevLine->mNext = aLine->mNext;
    }
    delete aLine;
  }
}

void
nsBlockFrame::WillReflowLine(nsBlockReflowState& aState,
                             nsLineBox* aLine)
{
  // Setup the first-letter-style-ok flag
  nsLineLayout& lineLayout = *aState.mLineLayout;
  if (mFirstLetterStyle && (0 == lineLayout.GetLineNumber())) {
    lineLayout.SetFirstLetterStyleOK(PR_TRUE);
  }
  else {
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
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
                          PRBool* aKeepReflowGoing)
{
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("nsBlockFrame::ReflowLine: line=%p", aLine));

  nsresult rv = NS_OK;

  // If the line already has floaters on it from last time, remove
  // them from the spacemanager now.
  if (nsnull != aLine->mFloaters) {
    aLine->mFloaters->Clear();
  }

  // If the line is empty then first pull a frame into it so that we
  // know what kind of line it is (block or inline).
  if (0 == aLine->ChildCount()) {
    nsIFrame* frame;
    rv = PullFrame(aState, aLine, frame);
    if (NS_FAILED(rv)) {
      return rv;
    }
    if (nsnull == frame) {
      *aKeepReflowGoing = PR_FALSE;
      return rv;
    }
  }

  // Setup the line-layout for the new line
  aState.mLineLayout->Reset();
  aState.mCurrentLine = aLine;
  aLine->ClearDirty();
  aLine->SetNeedDidReflow();

  // Now that we know what kind of line we have, reflow it
  if (aLine->IsBlock()) {
    // When reflowing a block frame we always get the available space
    aState.GetAvailableSpace();

    if ((nsnull != aState.lineLayout) &&
        (0 != aState.lineLayout->GetPlacedFrames())) {
      // Blocks are not allowed on the same line as anything else
      aState.mReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      *aKeepReflowGoing = PR_FALSE;
    }
    else {
      // Notify observers that we are about to reflow the line
      WillReflowLine(aState, aLine);

      rv = ReflowBlockFrame(aState, aLine, aKeepReflowGoing);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }
  else {
    // When this class is an inline frame and we are reflowing inline
    // frames then there is no point in getting available space.
    nscoord x, availWidth, availHeight;
    aState.GetAvailableSpace();

    // Setup initial coordinate system for reflowing the inline frames
    // into.
    x = aState.mAvailSpaceRect.x + aState.mBorderPadding.left;
    availWidth = aState.mAvailSpaceRect.width;

    if (aState.mUnconstrainedHeight) {
      availHeight = NS_UNCONSTRAINEDSIZE;
    }
    else {
      /* XXX get the height right! */
      availHeight = aState.mAvailSpaceRect.height;
    }
    aState.mInlineReflow->Init(x, aState.mY, availWidth, availHeight);

    // Notify observers that we are about to reflow the line
    WillReflowLine(aState, aLine);

    // Reflow the frames that are already on the line first
    PRBool keepLineGoing = PR_TRUE;
    PRInt32 i;
    nsIFrame* frame = aLine->mFirstChild;
    for (i = 0; i < aLine->ChildCount(); i++) {
      rv = ReflowInlineFrame(aState, aLine, frame, &keepLineGoing);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (!keepLineGoing) {
        // It is possible that one or more of next lines are empty
        // (because of DeleteNextInFlowsFor). If so, delete them now
        // in case we are finished.
        nsLineBox* nextLine = aLine->mNext;
        while ((nsnull != nextLine) && (0 == nextLine->ChildCount())) {
          // Discard empty lines immediately. Empty lines can happen
          // here because of DeleteNextInFlowsFor not being able to
          // delete lines.
          aLine->mNext = nextLine->mNext;
          NS_ASSERTION(nsnull == nextLine->mFirstChild, "bad empty line");
          delete nextLine;
          nextLine = aLine->mNext;
        }
        break;
      }
      frame->GetNextSibling(&frame);
    }

    // Pull frames and reflow them until we can't
    while (keepLineGoing) {
      nsIFrame* frame;
      rv = PullFrame(aState, aLine, frame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (nsnull == frame) {
        break;
      }
      rv = ReflowInlineFrame(aState, aLine, frame, &keepLineGoing);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }

    // If we are propogating out a break-before status then there is
    // no point in placing the line.
    if (!NS_INLINE_IS_BREAK_BEFORE(aState.mReflowStatus)) {
      rv = PlaceLine(aState, aLine, aKeepReflowGoing);
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
 * Try to pull a frame out a line pointed at by aFromList. If a frame
 * is pulled then aPulled will be set to PR_TRUE. In addition, if
 * aUpdateGeometricParent is set then the pulled frames geometric
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
  NS_ASSERTION(nsnull != fromLine, "bad line to pull from");
  if (0 == fromLine->ChildCount()) {
    // Discard empty lines immediately. Empty lines can happen here
    // because of DeleteChildsNextInFlow not being able to delete
    // lines. Don't stop pulling - there may be more frames around.
    *aFromList = fromLine->mNext;
    NS_ASSERTION(nsnull == fromLine->mFirstChild, "bad empty line");
    delete fromLine;
    aStopPulling = PR_FALSE;
    aFrameResult = nsnull;
  }
  else if ((0 != aLine->ChildCount()) && fromLine->IsBlock()) {
    // If our line is not empty and the child in aFromLine is a block
    // then we cannot pull up the frame into this line. In this case
    // we stop pulling.
    aStopPulling = PR_TRUE;
    aFrameResult = nsnull;
  }
  else {
    // Take frame from fromLine
    nsIFrame* frame = fromLine->mFirstChild;
    if (0 == aLine->mChildCount++) {
      aLine->mFirstChild = frame;
      aLine->SetIsBlock(fromLine->IsBlock());
      NS_ASSERTION(aLine->CheckIsBlock(), "bad line isBlock");
    }
    if (0 != --fromLine->mChildCount) {
      // Mark line dirty now that we pulled a child
      fromLine->MarkDirty();
      frame->GetNextSibling(&fromLine->mFirstChild);
    }
    else {
      // Free up the fromLine now that it's empty
      *aFromList = fromLine->mNext;
      delete fromLine;
    }

    // Change geometric parents
    if (aUpdateGeometricParent) {
      frame->SetParent(this);

      // The frame is being pulled from a next-in-flow; therefore we
      // need to add it to our sibling list.
      if (nsnull != aState.mPrevChild) {
        aState.mPrevChild->SetNextSibling(frame);
      }
      frame->SetNextSibling(nsnull);
#ifdef NS_DEBUG
      VerifyLineLength(aLine);
#endif
    }

    // Stop pulling because we found a frame to pull
    aStopPulling = PR_TRUE;
    aFrameResult = frame;
  }
  return NS_OK;
}

void
nsBlockFrame::DidReflowLine(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            PRBool aLineReflowStatus)
{
  // If the line no longer needs a floater array, get rid of it and
  // save some memory
  nsVoidArray* array = aLine->mFloaters;
  if (nsnull != array) {
    if (0 == array->Count()) {
      delete array;
      aLine->mFloaters = nsnull;
    }
    else {
      array->Compact();
    }
  }
}

void
nsBlockFrame::SlideFrames(nsIPresContext& aPresContext,
                          nsISpaceManager* aSpaceManager,
                          nsLineBox* aLine, nscoord aDY)
{
#if 0
ListTag(stdout); printf(": SlideFrames: line=%p dy=%d\n", aDY);
#endif
  // Adjust the Y coordinate of the frames in the line
  nsRect r;
  nsIFrame* kid = aLine->mFirstChild;
  PRInt32 n = aLine->ChildCount();
  while (--n >= 0) {
    kid->GetRect(r);
    r.y += aDY;
    kid->SetRect(r);

    // If the child has any floaters that impact the space manager,
    // slide them now.
    nsIHTMLReflow* ihr;
    if (NS_OK == kid->QueryInterface(kIHTMLReflowIID, (void**)&ihr)) {
      ihr->MoveInSpaceManager(aPresContext, aSpaceManager, 0, aDY);
    }

    kid->GetNextSibling(&kid);
  }

  // Adjust line state
  aLine->mBounds.y += aDY;
  aLine->mCombinedArea.y += aDY;
}

void
nsBlockFrame::SlideFloaters(nsIPresContext& aPresContext,
                            nsISpaceManager* aSpaceManager,
                            nsLineBox* aLine, nscoord aDY,
                            PRBool aUpdateSpaceManager)
{
  nsVoidArray* floaters = aLine->mFloaters;
  if (nsnull != floaters) {
    nsRect r;
    PRInt32 i, n = floaters->Count();
    for (i = 0; i < n; i++) {
      nsPlaceholderFrame* ph = (nsPlaceholderFrame*) floaters->ElementAt(i);
      nsIFrame* floater = ph->GetAnchoredItem();
      floater->GetRect(r);
      r.y += aDY;
      floater->SetRect(r);

      if (aUpdateSpaceManager) {
        // Adjust placement in space manager by the same amount
        aSpaceManager->OffsetRegion(floater, 0, aDY);
      }
    }
  }
}

NS_IMETHODIMP
nsBlockFrame::MoveInSpaceManager(nsIPresContext& aPresContext,
                                 nsISpaceManager* aSpaceManager,
                                 nscoord aDeltaX, nscoord aDeltaY)
{
#if 0
ListTag(stdout); printf(": MoveInSpaceManager: d=%d,%d\n", aDeltaX, aDeltaY);
#endif
  nsLineBox* line = mLines;
  while (nsnull != line) {
    // Move the floaters in the spacemanager
    nsVoidArray* floaters = line->mFloaters;
    if (nsnull != floaters) {
      PRInt32 i, n = floaters->Count();
      for (i = 0; i < n; i++) {
        nsPlaceholderFrame* ph = (nsPlaceholderFrame*) floaters->ElementAt(i);
        nsIFrame* floater = ph->GetAnchoredItem();
        aSpaceManager->OffsetRegion(floater, aDeltaX, aDeltaY);
#if 0
((nsFrame*)kid)->ListTag(stdout); printf(": offset=%d,%d\n", aDeltaX, aDeltaY);
#endif
      }
    }

    // Tell kids about the move too
    PRInt32 n = line->ChildCount();
    nsIFrame* kid = line->mFirstChild;
    while (--n >= 0) {
      nsIHTMLReflow* ihr;
      if (NS_OK == kid->QueryInterface(kIHTMLReflowIID, (void**)&ihr)) {
        ihr->MoveInSpaceManager(aPresContext, aSpaceManager, aDeltaX, aDeltaY);
      }
      kid->GetNextSibling(&kid);
    }
    
    line = line->mNext;
  }

  return NS_OK;
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
#ifdef NOISY_RUNIN
        ListTag(stdout);
        printf(": frame: ");
        aFrame->ListTag(stdout);
        printf(" followed by: ");
        nextFrame->ListTag(stdout);
        printf("\n");
#endif
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

#if XXX
void
nsBlockFrame::WillReflowFrame(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               nsIFrame* aFrame)
{
  nsIStyleContext* kidSC;
  aFrame->GetStyleContext(kidSC);
  if (nsnull != kidSC) {
    nsIStyleContext* kidParentSC;
    kidParentSC = kidSC->GetParent();
    if (nsnull != kidParentSC) {
      if (kidParentSC != mStyleContext) {
        // The frame has changed situations so re-resolve its style
        // context in the new situation.
        aFrame->ReResolveStyleContext(&aState.mPresContext, mStyleContext);
      }
      NS_RELEASE(kidParentSC);
    }
    NS_RELEASE(kidSC);
  }
}
#endif

// XXX This should be a no-op when there is no first-line/letter style
// in force!
void
nsBlockFrame::WillReflowFrame(nsBlockReflowState& aState,
                              nsLineBox* aLine,
                              nsIFrame* aFrame)
{
  PRBool repairStyleContext = PR_TRUE;

  // When reflowing a frame that is on the first-line, check and see
  // if a special style context should be placed in the context chain.
  if ((nsnull == mPrevInFlow) &&
      (0 == aState.mLineLayout->GetLineNumber())) {
    if (nsnull != mFirstLineStyle) {
      // Update the child frames style to inherit from the first-line
      // style.

      // XXX add code to check first and only do it if it needs doing!
#ifdef REALLY_NOISY_FIRST_LINE
      DumpStyleGeneaology(aFrame, "");
#endif
#ifdef NOISY_FIRST_LINE
      ListTag(stdout);
      printf(": ");
      ((nsFrame*)aFrame)->ListTag(stdout);
      printf(" adding in first-line style\n");
#endif
      aFrame->ReResolveStyleContext(&aState.mPresContext, mFirstLineStyle);
      repairStyleContext = PR_FALSE;
#ifdef REALLY_NOISY_FIRST_LINE
      DumpStyleGeneaology(aFrame, "  ");
#endif
    }
    if ((nsnull != mFirstLetterStyle) &&
        aState.mLineLayout->GetFirstLetterStyleOK()) {
      aFrame->ReResolveStyleContext(&aState.mPresContext, mFirstLetterStyle);
      repairStyleContext = PR_FALSE;
    }
  }

  if (repairStyleContext) {
    // Update style context when appropriate
    nsIStyleContext* kidSC;
    aFrame->GetStyleContext(&kidSC);
    if (nsnull != kidSC) {
      nsIStyleContext* kidParentSC;
      kidParentSC = kidSC->GetParent();
      if (nsnull != kidParentSC) {
        if (kidParentSC != mStyleContext) {
          aFrame->ReResolveStyleContext(&aState.mPresContext, mStyleContext);
        }
        NS_RELEASE(kidParentSC);
      }
      NS_RELEASE(kidSC);
    }
  }
}

nsresult
nsBlockFrame::ReflowBlockFrame(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               PRBool* aKeepReflowGoing)
{
  NS_PRECONDITION(*aKeepReflowGoing, "bad caller");
  NS_PRECONDITION(0 == aState.mLineLayout->GetPlacedFrames(),
                  "non-empty line with a block");

  nsresult rv = NS_OK;

  nsIFrame* frame = aLine->mFirstChild;

  // Prepare the inline reflow engine
  nsBlockFrame* runInToFrame;
  nsBlockFrame* compactWithFrame;
  nscoord compactMarginWidth = 0;
  PRBool isCompactFrame = PR_FALSE;
  const nsStyleDisplay* display;
  frame->GetStyleData(eStyleStruct_Display,
                      (const nsStyleStruct*&) display);
  switch (display->mDisplay) {
  case NS_STYLE_DISPLAY_RUN_IN:
#ifdef NOISY_RUNIN
    ListTag(stdout);
    printf(": trying to see if ");
    aFrame->ListTag(stdout);
    printf(" is a run-in candidate\n");
#endif
    runInToFrame = FindFollowingBlockFrame(frame);
    if (nsnull != runInToFrame) {
// XXX run-in frame should be pushed to the next-in-flow too if the
// run-in-to frame is pushed.
      nsRect r(0, aState.mY, 0, 0);
      aLine->mBounds = r;
      aLine->mCombinedArea = r;
      aLine->mCarriedOutTopMargin = 0;
      aLine->mCarriedOutBottomMargin = 0;
      aLine->SetMarginFlags(0);
#if XXX_need_line_outside_children
      aLine->ClearOutsideChildren();
#endif
      aLine->mBreakType = NS_STYLE_CLEAR_NONE;
//XXX        aFrame->WillReflow(aState.mPresContext);
      frame->SetRect(r);
      aState.mPrevChild = frame;
      aState.mRunInToFrame = runInToFrame;
      aState.mRunInFrame = (nsBlockFrame*) frame;
      return rv;
    }
    break;

  case NS_STYLE_DISPLAY_COMPACT:
    compactWithFrame = FindFollowingBlockFrame(frame);
    if (nsnull != compactWithFrame) {
      const nsStyleSpacing* spacing;
      nsMargin margin;
      nsresult rv;
      rv = compactWithFrame->GetStyleData(eStyleStruct_Spacing,
                                          (const nsStyleStruct*&) spacing);
      if (NS_SUCCEEDED(rv) && (nsnull != spacing)) {
        nsHTMLReflowState::ComputeMarginFor(compactWithFrame, &aState,
                                            margin);
        compactMarginWidth = margin.left;
      }
      isCompactFrame = PR_TRUE;
    }
    break;
  }

  nsBlockReflowContext brc(aState.mPresContext, *aState.mLineLayout, aState);
  brc.SetCompactMarginWidth(compactMarginWidth);

  // Clear floaters before the block if the clear style is not none
  aLine->mBreakType = display->mBreakType;
  if (NS_STYLE_CLEAR_NONE != display->mBreakType) {
    switch (display->mBreakType) {
    case NS_STYLE_CLEAR_LEFT:
    case NS_STYLE_CLEAR_RIGHT:
    case NS_STYLE_CLEAR_LEFT_AND_RIGHT:
      aState.ClearFloaters(aState.mY, display->mBreakType);
      // XXX: ?If we just advanced Y then we need to factor that amount
      // into the next margin calculation and reduce the amount of Y
      // margin applied by the amount just moved.
      break;
    }
  }

  // Set run-in frame if this is the run-in-to frame. That way the
  // target block frame knows to pick up the children from the run-in
  // frame.
  if (frame == aState.mRunInToFrame) {
    brc.SetRunInFrame(aState.mRunInFrame);
  }

  // Compute the available space for the block
  nscoord availHeight = aState.mUnconstrainedHeight
    ? NS_UNCONSTRAINEDSIZE
    : aState.mBottomEdge - aState.mY;

  // Now setup the availSpace rect. If the block frame we are
  // reflowing is one of "ours" (block, run-in, compact, list-item)
  // then we get it an available space that is *NOT* affected by
  // floaters.  Otherwise we position the block outside of the
  // floaters.
  nscoord availX, availWidth;
  nsSplittableType splitType;
  switch (display->mDisplay) {
  case NS_STYLE_DISPLAY_BLOCK:
  case NS_STYLE_DISPLAY_RUN_IN:
  case NS_STYLE_DISPLAY_COMPACT:
  case NS_STYLE_DISPLAY_LIST_ITEM:
    if (NS_SUCCEEDED(frame->IsSplittable(splitType)) &&
        (NS_FRAME_SPLITTABLE_NON_RECTANGULAR == splitType)) {
      availX = aState.mBorderPadding.left;
      availWidth = aState.mUnconstrainedWidth
        ? NS_UNCONSTRAINEDSIZE
        : aState.mContentArea.width;
      break;
    }
    // Assume the frame is clueless about the space manager
    // FALLTHROUGH

  default:
    availX = aState.mAvailSpaceRect.x + aState.mBorderPadding.left;
    availWidth = aState.mAvailSpaceRect.width;
    break;
  }

  // Reflow the block into the available space
  nsRect availSpace(availX, aState.mY, availWidth, availHeight);
  WillReflowFrame(aState, aLine, frame);
  nsReflowStatus frameReflowStatus;
  nsMargin computedOffsets;
  PRBool applyTopMargin = aState.ShouldApplyTopMargin();
  rv = brc.ReflowBlock(frame, availSpace,
#ifdef SPECULATIVE_TOP_MARGIN
                       applyTopMargin, aState.mPrevBottomMargin,
#endif
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
    *aKeepReflowGoing = brc.PlaceBlock(isAdjacentWithTop,
#ifndef SPECULATIVE_TOP_MARGIN
                                       applyTopMargin,
                                       aState.mPrevBottomMargin,
#endif
                                       computedOffsets,
                                       aLine->mBounds, aLine->mCombinedArea);
    if (*aKeepReflowGoing) {
      // Some of the child block fit

      // Set carry out top margin value when margin is not being applied
      if (!applyTopMargin) {
        aState.mCarriedOutTopMargin = brc.GetCollapsedTopMargin();
      }

      // Advance to new Y position
      nscoord newY = aLine->mBounds.YMost();
      if (isCompactFrame) {
        // For compact frames, we don't adjust the Y coordinate at all IF
        // the compact frame ended up fitting in the margin space
        // allocated for it.
        nsRect r;
        frame->GetRect(r);
        if (r.width <= compactMarginWidth) {
          // XXX margins will be wrong
          // XXX ltr/rtl for horizontal placement within the margin area
          // XXX vertical alignment with the compactWith frame's *first line*
          newY = aState.mY;
        }
      }
      aState.mY = newY;
      aLine->mCarriedOutTopMargin = brc.GetCarriedOutTopMargin();
      aLine->mCarriedOutBottomMargin = brc.GetCarriedOutBottomMargin();

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
          nsLineBox* line = new nsLineBox(frame, 1, LINE_IS_BLOCK);
          if (nsnull == line) {
            return NS_ERROR_OUT_OF_MEMORY;
          }
          line->mNext = aLine->mNext;
          aLine->mNext = line;

          // Do not count the continuation child on the line it used
          // to be on
          aLine->mChildCount--;
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
        aState.mPrevBottomMargin = 0;
      }
      else {
        aState.mPrevBottomMargin = brc.GetCollapsedBottomMargin();
      }

      // Post-process the "line"
      PostPlaceLine(aState, aLine, brc.GetMaxElementSize());

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
        nsresult rv;
        rv = frame->GetStyleData(eStyleStruct_Font,
                                 (const nsStyleStruct*&) font);
        if (NS_SUCCEEDED(rv) && (nsnull != font)) {
          nsIRenderingContext& rc = *aState.rendContext;
          rc.SetFont(font->mFont);
          nsIFontMetrics* fm;
          rv = rc.GetFontMetrics(fm);
          if (NS_SUCCEEDED(rv) && (nsnull != fm)) {
            fm->GetMaxAscent(ascent);
            NS_RELEASE(fm);
          }
        }

        // Tall bullets won't look particularly nice here...
        nsRect bbox;
        mBullet->GetRect(bbox);
        nscoord topMargin = applyTopMargin ? brc.GetCollapsedTopMargin() : 0;
        bbox.y = aState.mBorderPadding.top + ascent - metrics.ascent +
          topMargin;
        mBullet->SetRect(bbox);
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
  return rv;
}

/**
 * Reflow an inline frame. The reflow status is mapped from the frames
 * reflow status to the lines reflow status (not to our reflow status).
 * The line reflow status is simple: PR_TRUE means keep placing frames
 * on the line; PR_FALSE means don't (the line is done). If the line
 * has some sort of breaking affect then aLine->mBreakType will be set
 * to something other than NS_STYLE_CLEAR_NONE.
 */
nsresult
nsBlockFrame::ReflowInlineFrame(nsBlockReflowState& aState,
                                nsLineBox* aLine,
                                nsIFrame* aFrame,
                                PRBool* aKeepLineGoing)
{
  NS_PRECONDITION(*aKeepLineGoing, "bad caller");

  // Send pre-reflow notification
  WillReflowFrame(aState, aLine, aFrame);

  // If it's currently ok to be reflowing in first-letter style then
  // we must be about to reflow a frame that has first-letter style.
  PRBool reflowingFirstLetter = aState.mLineLayout->GetFirstLetterStyleOK();

  // Reflow the inline frame
  nsReflowStatus frameReflowStatus;
  nsresult rv = aState.mInlineReflow->ReflowFrame(aFrame, aState.IsAdjacentWithTop(),
                                                  frameReflowStatus);
  if (NS_FAILED(rv)) {
    return rv;
  }

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
  aLine->mBreakType = NS_STYLE_CLEAR_NONE;
  if (NS_INLINE_IS_BREAK(frameReflowStatus)) {
    // Always abort the line reflow (because a line break is the
    // minimal amount of break we do).
    *aKeepLineGoing = PR_FALSE;

    // XXX what should aLine->mBreakType be set to in all these cases?
    PRUint8 breakType = NS_INLINE_GET_BREAK_TYPE(frameReflowStatus);
    NS_ASSERTION(breakType != NS_STYLE_CLEAR_NONE, "bad break type");
    NS_ASSERTION(NS_STYLE_CLEAR_PAGE != breakType, "no page breaks yet");

    if (NS_INLINE_IS_BREAK_BEFORE(frameReflowStatus)) {
      // Break-before cases.
      if (aFrame == aLine->mFirstChild) {
        NS_NOTREACHED("can't get here, can we?");
#if 0
        // All break-before's that occur at the first child on a
        // line stop the overall reflow.
        if (mLines == aLine) {
          // If it's our first child on our first line then propogate
          // outward the break-before reflow status unmodified.
          aState.mReflowStatus = frameReflowStatus;
        }
        else {
          // Its not our first line; push the remaining lines to a
          // next-in-flow
          PushLines(aState);

          // Adjust the reflow status to indicate a
          // break-after-not-complete; because we need to be continued
          // and we need to force a line break (or something stronger)
          // upstream.
          aState.mReflowStatus = NS_FRAME_NOT_COMPLETE | NS_INLINE_BREAK |
            NS_INLINE_BREAK_AFTER | NS_INLINE_MAKE_BREAK_TYPE(breakType);
        }
#endif
      }
      else {
        // It's not the first child on this line so go ahead and split
        // the line. We will see the frame again on the next-line.
        rv = SplitLine(aState, aLine, aFrame);
        if (NS_FAILED(rv)) {
          return rv;
        }
      }
    }
    else {
      // Break-after cases
      aLine->mBreakType = breakType;
      if (NS_FRAME_IS_NOT_COMPLETE(frameReflowStatus)) {
        // Create a continuation for the incomplete frame. Note that the
        // frame may already have a continuation.
        PRBool madeContinuation;
        rv = CreateContinuationFor(aState, aLine, aFrame, madeContinuation);
        if (NS_FAILED(rv)) {
          return rv;
        }
      }

      // Split line, but after the frame just reflowed
      aFrame->GetNextSibling(&aFrame);
      rv = SplitLine(aState, aLine, aFrame);
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

    PRBool needSplit = PR_FALSE;
    if (!reflowingFirstLetter) {
      needSplit = PR_TRUE;
    }

    if (needSplit) {
      // Split line after the current frame
      *aKeepLineGoing = PR_FALSE;
      aFrame->GetNextSibling(&aFrame);
      rv = SplitLine(aState, aLine, aFrame);
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
    aLine->mChildCount++;
#ifdef NS_DEBUG
    VerifyLineLength(aLine);
#endif
  }
  return rv;
}

nsresult
nsBlockFrame::SplitLine(nsBlockReflowState& aState,
                         nsLineBox* aLine,
                         nsIFrame* aFrame)
{
  PRInt32 pushCount = aLine->ChildCount() -
    aState.mInlineReflow->GetCurrentFrameNum();
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("nsBlockFrame::SplitLine: pushing %d frames",
                  pushCount));
  if (0 != pushCount) {
    NS_ASSERTION(aLine->ChildCount() > pushCount, "bad push");
    NS_ASSERTION(nsnull != aFrame, "whoops");
    nsLineBox* to = aLine->mNext;
    if (nsnull != to) {
      // Only push into the next line if it's empty; otherwise we can
      // end up pushing a frame which is continued into the same frame
      // as it's continuation. This causes all sorts of bad side
      // effects so we don't allow it.
      if (0 != to->ChildCount()) {
        nsLineBox* insertedLine = new nsLineBox(aFrame, pushCount, 0);
        if (nsnull == insertedLine) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        aLine->mNext = insertedLine;
        insertedLine->mNext = to;
        to = insertedLine;
      } else {
        to->mFirstChild = aFrame;
        to->mChildCount += pushCount;
        to->MarkDirty();
      }
    } else {
      to = new nsLineBox(aFrame, pushCount, 0);
      if (nsnull == to) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      aLine->mNext = to;
    }
    to->SetIsBlock(aLine->IsBlock());
    aLine->mChildCount -= pushCount;
#ifdef NS_DEBUG
    VerifyLineLength(aLine);
#endif

    // Let inline reflow know that some frames are no longer part of
    // its state.
    if (!aLine->IsBlock()) {
      aState.mInlineReflow->ChangeFrameCount(aLine->ChildCount());
    }
  }
  return NS_OK;
}

PRBool
nsBlockFrame::ShouldJustifyLine(nsBlockReflowState& aState, nsLineBox* aLine)
{
  nsLineBox* next = aLine->mNext;
  while (nsnull != next) {
    // There is another line
    if (0 != next->ChildCount()) {
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
      if (0 != line->ChildCount()) {
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
                         nsLineBox* aLine,
                         PRBool* aKeepReflowGoing)
{
  nsresult rv = NS_OK;

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
  nsInlineReflow& ir = *aState.mInlineReflow;
  PRBool addedBullet = PR_FALSE;
  if (HaveOutsideBullet() && (aLine == mLines) && !ir.IsZeroHeight()) {
    nsHTMLReflowMetrics metrics(nsnull);
    ReflowBullet(aState, metrics);
    ir.AddFrame(mBullet, metrics);
    addedBullet = PR_TRUE;
  }
  ir.VerticalAlignFrames(aLine->mBounds, aState.mAscent, aState.mDescent);
  if (addedBullet) {
    ir.RemoveFrame(mBullet);
  }

  // Only block frames horizontally align their children because
  // inline frames "shrink-wrap" around their children (therefore
  // there is no extra horizontal space).
  PRBool allowJustify = PR_TRUE;
  if (NS_STYLE_TEXT_ALIGN_JUSTIFY == aState.mStyleText->mTextAlign) {
    allowJustify = ShouldJustifyLine(aState, aLine);
  }
  ir.TrimTrailingWhiteSpace(aLine->mBounds);
  ir.HorizontalAlignFrames(aLine->mBounds, allowJustify);
  ir.RelativePositionFrames(aLine->mCombinedArea);

  // Calculate the bottom margin for the line.
  nscoord lineBottomMargin = 0;
  if (0 == aLine->mBounds.height) {
    nsIFrame* brFrame = aState.mLineLayout->GetBRFrame();
    if (nsnull != brFrame) {
      // If a line ends in a BR, and the line is empty of height, then
      // we make sure that the line ends up with some height
      // anyway. Note that the height looks like vertical margin so
      // that it can compress with other block margins.
      nsIStyleContext* brSC;
      nsIPresContext& px = aState.mPresContext;
      nsresult rv = brFrame->GetStyleContext(&brSC);
      if ((NS_OK == rv) && (nsnull != brSC)) {
        const nsStyleFont* font = (const nsStyleFont*)
          brSC->GetStyleData(eStyleStruct_Font);
        nsIFontMetrics* fm = nsnull;
        px.GetMetricsFor(font->mFont, &fm);
        if (nsnull != fm) {
          fm->GetHeight(lineBottomMargin);
          NS_RELEASE(fm);
        }
        NS_RELEASE(brSC);
      }
    }
  }
  else {
    aState.mRunInFromFrame = nsnull;
    aState.mRunInToFrame = nsnull;
  }

  // Calculate the lines top and bottom margin values. The margin will
  // come from an embedded block frame, not from inline
  // frames. Because this is an "inline" line, the child margins are
  // all effectively zero so we pass in nsMargin(0, 0, 0, 0).
  nscoord topMargin, bottomMargin;
  nsBlockReflowContext::CollapseMargins(nsMargin(0, 0, 0, 0),
                                        ir.GetCarriedOutTopMargin(),
                                        ir.GetCarriedOutBottomMargin(),
                                        aLine->mBounds.height,
                                        aState.mPrevBottomMargin,
                                        topMargin, bottomMargin);

#if XXX
ListTag(stdout);
printf(": ");
((nsFrame*)(aLine->mFirstChild))->ListTag(stdout);
printf(" mY=%d carried=%d,%d top=%d bottom=%d prev=%d shouldApply=%s\n",
       aState.mY, ir.GetCarriedOutTopMargin(), ir.GetCarriedOutBottomMargin(),
       topMargin, bottomMargin, aState.mPrevBottomMargin,
       aState.ShouldApplyTopMargin() ? "yes" : "no");
#endif
  if (!aState.ShouldApplyTopMargin()) {
    aState.mCarriedOutTopMargin = topMargin;
    topMargin = 0;
  }

  // See if the line fit. If it doesn't we need to push it. Our first
  // line will always fit.
  nscoord newY = aLine->mBounds.YMost() + topMargin + lineBottomMargin;
  NS_FRAME_TRACE(NS_FRAME_TRACE_CHILD_REFLOW,
     ("nsBlockFrame::PlaceLine: newY=%d limit=%d lineHeight=%d",
      newY, aState.mBottomEdge, aLine->mBounds.height));
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

  aLine->mCarriedOutTopMargin = ir.GetCarriedOutTopMargin();
  aLine->mCarriedOutBottomMargin = ir.GetCarriedOutBottomMargin();
  aState.mPrevBottomMargin = bottomMargin;
  if (0 != topMargin) {
    // Apply collapsed top-margin value
    SlideFrames(aState.mPresContext, aState.mSpaceManager, aLine, topMargin);
    SlideFloaters(aState.mPresContext, aState.mSpaceManager, aLine, topMargin,
                  PR_TRUE);
  }
  aState.mY = newY;

  PostPlaceLine(aState, aLine, ir.GetMaxElementSize());

  // Any below current line floaters to place?
  if (0 != aState.mPendingFloaters.Count()) {
    aState.PlaceBelowCurrentLineFloaters(&aState.mPendingFloaters);
    aState.mPendingFloaters.Clear();
  }

  // Apply break-after clearing if necessary
  PRUint8 breakType = aLine->mBreakType;
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
  aState.mCurrentBand.GetMaxElementSize(&maxWidth, &maxHeight);

  // Add in the maximum width of any floaters in the band because we
  // always place some non-floating content with a floater.
  aMaxElementSize->width += maxWidth;

  // If the maximum-height of the tallest floater is larger than the
  // maximum-height of the content then update the max-element-size
  // height
  if (maxHeight > aMaxElementSize->height) {
    aMaxElementSize->height = maxHeight;
  }
}

void
nsBlockFrame::PostPlaceLine(nsBlockReflowState& aState,
                             nsLineBox* aLine,
                             const nsSize& aMaxElementSize)
{
  // Update max-element-size
  if (aState.mComputeMaxElementSize) {
    nsSize lineMaxElementSize(aMaxElementSize);
    if (0 != aState.mCurrentBand.GetFloaterCount()) {
      // Add in floater impacts to the lines max-element-size
      ComputeLineMaxElementSize(aState, aLine, &lineMaxElementSize);
    }
    if (lineMaxElementSize.width > aState.mMaxElementSize.width) {
      aState.mMaxElementSize.width = lineMaxElementSize.width;
    }
    if (lineMaxElementSize.height > aState.mMaxElementSize.height) {
      aState.mMaxElementSize.height = lineMaxElementSize.height;
    }
  }

#if XXX_need_line_outside_children
  // Compute LINE_OUTSIDE_CHILDREN state for this line. The bit is set
  // if any child frame has outside children.
  if ((aLine->mCombinedArea.x < aLine->mBounds.x) ||
      (aLine->mCombinedArea.XMost() > aLine->mBounds.XMost()) ||
      (aLine->mCombinedArea.y < aLine->mBounds.y) ||
      (aLine->mCombinedArea.YMost() > aLine->mBounds.YMost())) {
    aLine->SetOutsideChildren();
  }
  else {
    aLine->ClearOutsideChildren();
  }
#endif

  // Update xmost
  nscoord xmost = aLine->mBounds.XMost();
  if (xmost > aState.mKidXMost) {
    aState.mKidXMost = xmost;
  }
}

static nsresult
FindFloatersIn(nsIFrame* aFrame, nsVoidArray*& aArray)
{
  const nsStyleDisplay* display;
  aFrame->GetStyleData(eStyleStruct_Display,
                       (const nsStyleStruct*&) display);
  if (NS_STYLE_FLOAT_NONE != display->mFloats) {
    if (nsnull == aArray) {
      aArray = new nsVoidArray();
      if (nsnull == aArray) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    aArray->AppendElement(aFrame);
  }

  if (NS_STYLE_DISPLAY_INLINE == display->mDisplay) {
    nsIFrame* kid;
    aFrame->FirstChild(nsnull, &kid);
    while (nsnull != kid) {
      nsresult rv = FindFloatersIn(kid, aArray);
      if (NS_OK != rv) {
        return rv;
      }
      kid->GetNextSibling(&kid);
    }
  }
  return NS_OK;
}

void
nsBlockFrame::FindFloaters(nsLineBox* aLine)
{
  nsVoidArray* floaters = aLine->mFloaters;
  if (nsnull != floaters) {
    // Empty floater array before proceeding
    floaters->Clear();
  }

  nsIFrame* frame = aLine->mFirstChild;
  PRInt32 n = aLine->ChildCount();
  while (--n >= 0) {
    FindFloatersIn(frame, floaters);
    frame->GetNextSibling(&frame);
  }

  aLine->mFloaters = floaters;

  // Get rid of floater array if we don't need it
  if (nsnull != floaters) {
    if (0 == floaters->Count()) {
      delete floaters;
      aLine->mFloaters = nsnull;
    }
  }
}

void
nsBlockFrame::PushLines(nsBlockReflowState& aState)
{
  NS_ASSERTION(nsnull != aState.mPrevLine, "bad push");

  nsLineBox* lastLine = aState.mPrevLine;
  nsLineBox* nextLine = lastLine->mNext;

  lastLine->mNext = nsnull;
  mOverflowLines = nextLine;

  // Mark all the overflow lines dirty so that they get reflowed when
  // they are pulled up by our next-in-flow.
  while (nsnull != nextLine) {
    nextLine->MarkDirty();
    nextLine = nextLine->mNext;
  }

  // Break frame sibling list
  nsIFrame* lastFrame = lastLine->LastChild();
  lastFrame->SetNextSibling(nsnull);

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("nsBlockFrame::PushLines: line=%p prevInFlow=%p nextInFlow=%p",
      mOverflowLines, mPrevInFlow, mNextInFlow));
#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
//XXX    VerifyChildCount(mLines);
//XXX    VerifyChildCount(mOverflowLines, PR_TRUE);
  }
#endif
}

PRBool
nsBlockFrame::DrainOverflowLines()
{
  PRBool drained = PR_FALSE;

  // First grab the prev-in-flows overflow lines
  nsBlockFrame* prevBlock = (nsBlockFrame*) mPrevInFlow;
  if (nsnull != prevBlock) {
    nsLineBox* line = prevBlock->mOverflowLines;
    if (nsnull != line) {
      drained = PR_TRUE;
      NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
         ("nsBlockFrame::DrainOverflowLines: line=%p prevInFlow=%p",
          line, prevBlock));
      prevBlock->mOverflowLines = nsnull;

      // Make all the frames on the mOverflowLines list mine
      nsIFrame* lastFrame = nsnull;
      nsIFrame* frame = line->mFirstChild;
      while (nsnull != frame) {
        frame->SetParent(this);
        lastFrame = frame;
        frame->GetNextSibling(&frame);
      }

      // Join the line lists
      if (nsnull == mLines) {
        mLines = line;
      }
      else {
        // Join the sibling lists together
        lastFrame->SetNextSibling(mLines->mFirstChild);

        // Place overflow lines at the front of our line list
        nsLineBox* lastLine = nsLineBox::LastLine(line);
        lastLine->mNext = mLines;
        mLines = line;
      }
    }
  }

  // Now grab our own overflow lines
  if (nsnull != mOverflowLines) {
    // This can happen when we reflow and not everything fits and then
    // we are told to reflow again before a next-in-flow is created
    // and reflows.
    NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
       ("nsBlockFrame::DrainOverflowLines: from me, line=%p",
        mOverflowLines));
    nsLineBox* lastLine = nsLineBox::LastLine(mLines);
    if (nsnull == lastLine) {
      mLines = mOverflowLines;
    }
    else {
      lastLine->mNext = mOverflowLines;
      nsIFrame* lastFrame = lastLine->LastChild();
      lastFrame->SetNextSibling(mOverflowLines->mFirstChild);

      // Update our last-content-index now that we have a new last child
      lastLine = nsLineBox::LastLine(mOverflowLines);
    }
    mOverflowLines = nsnull;
    drained = PR_TRUE;
  }

#ifdef NS_DEBUG
  if (GetVerifyTreeEnable()) {
//XXX    VerifyChildCount(mLines, PR_TRUE);
  }
#endif
  return drained;
}

//////////////////////////////////////////////////////////////////////
// Frame list manipulation routines

NS_IMETHODIMP
nsBlockFrame::AppendFrames(nsIPresContext& aPresContext,
                            nsIPresShell&   aPresShell,
                            nsIAtom*        aListName,
                            nsIFrame*       aFrameList)
{
  if (nsnull != aListName) {
    // XXX temporary until area frame code is updated
    return nsFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList);
  }
  nsresult rv = AppendNewFrames(aPresContext, aFrameList);
  if (NS_SUCCEEDED(rv)) {
//    RenumberLists(aState);
//    rv = ComputeTextRuns(aState);

    nsIReflowCommand* reflowCmd = nsnull;
    nsresult rv;
    rv = NS_NewHTMLReflowCommand(&reflowCmd, this,
                                 nsIReflowCommand::FrameAppended,
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
nsBlockFrame::AppendNewFrames(nsIPresContext& aPresContext,
                               nsIFrame* aNewFrame)
{
  // Get our last line and then get its last child
  nsIFrame* lastFrame;
  nsLineBox* lastLine = nsLineBox::LastLine(mLines);
  if (nsnull != lastLine) {
    lastFrame = lastLine->LastChild();
  } else {
    lastFrame = nsnull;
  }

  // Add the new frames to the sibling list; wrap any frames that
  // require wrapping
  if (nsnull != lastFrame) {
    lastFrame->SetNextSibling(aNewFrame);
  }
  nsresult rv;

  // Make sure that new inlines go onto the end of the lastLine when
  // the lastLine is mapping inline frames.
  PRInt32 pendingInlines = 0;
  if (nsnull != lastLine) {
    if (!lastLine->IsBlock()) {
      pendingInlines = 1;
    }
  }

  // Now create some lines for the new frames
  nsIFrame* prevFrame = lastFrame;
  for (nsIFrame* frame = aNewFrame; nsnull != frame;
       frame->GetNextSibling(&frame)) {
    // See if the child is a block or non-block
    const nsStyleDisplay* kidDisplay;
    rv = frame->GetStyleData(eStyleStruct_Display,
                             (const nsStyleStruct*&) kidDisplay);
    if (NS_OK != rv) {
      return rv;
    }
    const nsStylePosition* kidPosition;
    rv = frame->GetStyleData(eStyleStruct_Position,
                             (const nsStyleStruct*&) kidPosition);
    if (NS_OK != rv) {
      return rv;
    }
    PRBool isBlock = nsLineLayout::TreatFrameAsBlock(kidDisplay, kidPosition);

    // See if we need to move the frame outside of the flow, and insert a
    // placeholder frame in its place
    nsIFrame* placeholder;
    if (MoveFrameOutOfFlow(aPresContext, frame, kidDisplay, kidPosition,
                           placeholder)) {
      // Reset the previous frame's next sibling pointer
      if (nsnull != prevFrame) {
        prevFrame->SetNextSibling(placeholder);
      }

      // The placeholder frame is always inline
      frame = placeholder;
      isBlock = PR_FALSE;
    }
    else {
      // Wrap the frame in a view if necessary
      nsIStyleContext* kidSC;
      frame->GetStyleContext(&kidSC);
      rv = CreateViewForFrame(aPresContext, frame, kidSC, PR_FALSE);
      NS_RELEASE(kidSC);
      if (NS_OK != rv) {
        return rv;
      }
    }

    // If the child is an inline then add it to the lastLine (if it's
    // an inline line, otherwise make a new line). If the child is a
    // block then make a new line and put the child in that line.
    if (isBlock) {
      // If the previous line has pending inline data to be reflowed,
      // do so now.
      if (0 != pendingInlines) {
        // Set this to true in case we don't end up reflowing all of the
        // frames on the line (because they end up being pushed).
        lastLine->MarkDirty();
        pendingInlines = 0;
      }

      // Create a line for the block
      nsLineBox* line = new nsLineBox(frame, 1, LINE_IS_BLOCK);
      if (nsnull == line) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (nsnull == lastLine) {
        mLines = line;
      }
      else {
        lastLine->mNext = line;
      }
      lastLine = line;
    }
    else {
      if (0 == pendingInlines) {
        nsLineBox* line = new nsLineBox(frame, 0, 0);
        if (nsnull == line) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        if (nsnull == lastLine) {
          mLines = line;
        }
        else {
          lastLine->mNext = line;
        }
        lastLine = line;
      }
      lastLine->mChildCount++;
      pendingInlines++;
    }

    // Remember the previous frame
    prevFrame = frame;
  }

  if (0 != pendingInlines) {
    // Set this to true in case we don't end up reflowing all of the
    // frames on the line (because they end up being pushed).
    lastLine->MarkDirty();
  }

  MarkEmptyLines(aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::InsertFrames(nsIPresContext& aPresContext,
                            nsIPresShell&   aPresShell,
                            nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsIFrame*       aFrameList)
{
  if (nsnull != aListName) {
    // XXX temporary until area frame code is updated
    return nsFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);
  }
  nsresult rv = InsertNewFrames(aPresContext, aFrameList, aPrevFrame);
  if (NS_SUCCEEDED(rv)) {
    nsIReflowCommand* reflowCmd = nsnull;
    nsresult rv;
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

// XXX rewrite to deal with a list of frames
nsresult
nsBlockFrame::InsertNewFrames(nsIPresContext& aPresContext,
                               nsIFrame*       aFrameList,
                               nsIFrame*       aPrevSibling)
{
  if (nsnull == mLines) {
    NS_ASSERTION(nsnull == aPrevSibling, "prev-sibling and empty line list!");
    return AppendNewFrames(aPresContext, aFrameList);
  }

  nsIFrame* newFrame = aFrameList;
  while (nsnull != newFrame) {
    nsIFrame* next;
    newFrame->GetNextSibling(&next);
    newFrame->SetNextSibling(nsnull);

    const nsStyleDisplay* display;
    newFrame->GetStyleData(eStyleStruct_Display,
                           (const nsStyleStruct*&) display);
    const nsStylePosition* position;
    newFrame->GetStyleData(eStyleStruct_Position,
                           (const nsStyleStruct*&) position);
    PRUint16 newFrameIsBlock =
      nsLineLayout::TreatFrameAsBlock(display, position)
      ? LINE_IS_BLOCK
      : 0;

    // See if we need to move the frame outside of the flow, and insert a
    // placeholder frame in its place
    nsIFrame* placeholder;
    if (MoveFrameOutOfFlow(aPresContext, newFrame, display, position,
                           placeholder)) {
      // Add the placeholder frame to the flow
      newFrame = placeholder;
      newFrameIsBlock = PR_FALSE;  // placeholder frame is always inline
    }
    else {
      // Wrap the frame in a view if necessary
      nsIStyleContext* kidSC;
      newFrame->GetStyleContext(&kidSC);
      nsresult rv = CreateViewForFrame(aPresContext, newFrame, kidSC, PR_FALSE);    
      NS_RELEASE(kidSC);
      if (NS_OK != rv) {
        return rv;
      }
    }

    // Insert/append the frame into flows line list at the right spot
    nsLineBox* newLine;
    nsLineBox* line = mLines;
    if (nsnull == aPrevSibling) {
      // Insert new frame into the sibling list
      newFrame->SetNextSibling(line->mFirstChild);

      if (line->IsBlock() || newFrameIsBlock) {
        // Create a new line
        newLine = new nsLineBox(newFrame, 1, newFrameIsBlock);
        if (nsnull == newLine) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        newLine->mNext = mLines;
        mLines = newLine;
      } else {
        // Insert frame at the front of the line
        line->mFirstChild = newFrame;
        line->mChildCount++;
        line->MarkDirty();
      }
    }
    else {
      // Find line containing the previous sibling to the new frame
      line = nsLineBox::FindLineContaining(line, aPrevSibling);
      NS_ASSERTION(nsnull != line, "no line contains the previous sibling");
      if (nsnull != line) {
        if (line->IsBlock()) {
          // Create a new line just after line
          newLine = new nsLineBox(newFrame, 1, newFrameIsBlock);
          if (nsnull == newLine) {
            return NS_ERROR_OUT_OF_MEMORY;
          }
          newLine->mNext = line->mNext;
          line->mNext = newLine;
        }
        else if (newFrameIsBlock) {
          // Split line in two, if necessary. We can't allow a block to
          // end up in an inline line.
          if (line->IsLastChild(aPrevSibling)) {
            // The new frame goes after prevSibling and prevSibling is
            // the last frame on the line. Therefore we don't need to
            // split the line, just create a new line.
            newLine = new nsLineBox(newFrame, 1, newFrameIsBlock);
            if (nsnull == newLine) {
              return NS_ERROR_OUT_OF_MEMORY;
            }
            newLine->mNext = line->mNext;
            line->mNext = newLine;
          }
          else {
            // The new frame goes after prevSibling and prevSibling is
            // somewhere in the line, but not at the end. Split the line
            // just after prevSibling.
            PRInt32 i, n = line->ChildCount();
            nsIFrame* frame = line->mFirstChild;
            for (i = 0; i < n; i++) {
              if (frame == aPrevSibling) {
                nsIFrame* nextSibling;
                aPrevSibling->GetNextSibling(&nextSibling);

                // Create new line to hold the remaining frames
                NS_ASSERTION(n - i - 1 > 0, "bad line count");
                newLine = new nsLineBox(nextSibling, n - i - 1, 0);
                if (nsnull == newLine) {
                  return NS_ERROR_OUT_OF_MEMORY;
                }
                newLine->mNext = line->mNext;
                line->mNext = newLine;
                line->MarkDirty();
                line->mChildCount = i + 1;
                break;
              }
              frame->GetNextSibling(&frame);
            }

            // Now create a new line to hold the block
            newLine = new nsLineBox(newFrame, 1, newFrameIsBlock);
            if (nsnull == newLine) {
              return NS_ERROR_OUT_OF_MEMORY;
            }
            newLine->mNext = line->mNext;
            line->mNext = newLine;
          }
        }
        else {
          // Insert frame into the line.
          //XXX NS_ASSERTION(line->GetLastContentIsComplete(), "bad line LCIC");
          line->mChildCount++;
          line->MarkDirty();
        }
      }

      // Insert new frame into the sibling list; note: this must be done
      // after the above logic because the above logic depends on the
      // sibling list being in the "before insertion" state.
      nsIFrame* nextSibling;
      aPrevSibling->GetNextSibling(&nextSibling);
      newFrame->SetNextSibling(nextSibling);
      aPrevSibling->SetNextSibling(newFrame);
    }
    aPrevSibling = newFrame;
    newFrame = next;
  }

  MarkEmptyLines(aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::RemoveFrame(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  if (nsnull != aListName) {
    // XXX temporary until area frame code is updated
    return nsFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
  }
  nsresult rv = DoRemoveFrame(aPresContext, aOldFrame);
  if (NS_SUCCEEDED(rv)) {
    nsIReflowCommand* reflowCmd = nsnull;
    nsresult rv;
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

// XXX simplify this by using RemoveChild and DeleteChildsNextInFlow

nsresult
nsBlockFrame::DoRemoveFrame(nsIPresContext& aPresContext,
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
    PRInt32 n = line->ChildCount();
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
#endif

      // See if the frame is a floater (actually, the floaters
      // placeholder). If it is, then destroy the floated frame too.
      const nsStyleDisplay* display;
      nsresult rv = aDeletedFrame->GetStyleData(eStyleStruct_Display,
                                                (const nsStyleStruct*&)display);
      if (NS_SUCCEEDED(rv) && (nsnull != display)) {
        // XXX Sanitize "IsFloating" question *everywhere* (add a
        // static method on nsFrame?)
        if (NS_STYLE_FLOAT_NONE != display->mFloats) {
          nsPlaceholderFrame* ph = (nsPlaceholderFrame*) aDeletedFrame;
          nsIFrame* floater = ph->GetAnchoredItem();
          if (nsnull != floater) {
            floater->DeleteFrame(aPresContext);
            if (nsnull != line->mFloaters) {
              // Wipe out the floater array for this line. It will get
              // recomputed during reflow anyway.
              delete line->mFloaters;
              line->mFloaters = nsnull;
            }
          }
        }
      }

      // Get the deleted frames next sibling
      nsIFrame* nextFrame;
      aDeletedFrame->GetNextSibling(&nextFrame);

      // Remove aDeletedFrame from the line
      if (line->mFirstChild == aDeletedFrame) {
        line->mFirstChild = nextFrame;
      }
      if (!line->IsBlock() && (nsnull != prevLine) && !prevLine->IsBlock()) {
        // Make sure the previous line (if it's an inline line) gets
        // a reflow too so that it can pullup from the line where we
        // just removed the frame.
        prevLine->MarkDirty();
        // XXX Note: prevLine may be in a prev-in-flow
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
      aDeletedFrame->GetNextInFlow(nextInFlow);
      if (nsnull != nextInFlow) {
        aDeletedFrame->BreakFromNextFlow();
      }
      aDeletedFrame->DeleteFrame(aPresContext);
      aDeletedFrame = nextInFlow;

      // If line is empty, remove it now
      nsLineBox* next = line->mNext;
      if (0 == --line->mChildCount) {
        *linep = next;
        line->mNext = nsnull;
        delete line;
      }
      else {
        line->MarkDirty();
        linep = &line->mNext;
      }
      prevLine = line;
      line = next;

      // See if we should keep looking in the current flow's line list.
      if (nsnull != aDeletedFrame) {
        if (aDeletedFrame != nextFrame) {
          // The deceased frames continuation is not the next frame in
          // the current flow's frame list. Therefore we know that the
          // continuation is in a different parent. So break out of
          // the loop so that we advance to the next parent.
#ifdef NS_DEBUG
          nsIFrame* parent;
          aDeletedFrame->GetParent(&parent);
          NS_ASSERTION(parent != flow, "strange continuation");
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
      prevSibling = nsnull;
    }
  }
  MarkEmptyLines(aPresContext);
  return NS_OK;
}

static PRBool
IsEmptyLine(nsIPresContext& aPresContext, nsLineBox* aLine)
{
  PRInt32 i, n = aLine->ChildCount();
  nsIFrame* frame = aLine->mFirstChild;
  for (i = 0; i < n; i++) {
    nsIContent* content;
    nsresult rv = frame->GetContent(&content);
    if (NS_FAILED(rv) || (nsnull == content)) {
      // If it doesn't have any content then this can't be an empty line
      return PR_FALSE;
    }
    nsITextContent* tc;
    rv = content->QueryInterface(kITextContentIID, (void**) &tc);
    if (NS_FAILED(rv) || (nsnull == tc)) {
      // If it's not text content then this can't be an empty line
      NS_RELEASE(content);
      return PR_FALSE;
    }

    const nsTextFragment* frag;
    PRInt32 numFrags;
    rv = tc->GetText(frag, numFrags);
    if (NS_FAILED(rv)) {
      NS_RELEASE(content);
      NS_RELEASE(tc);
      return PR_FALSE;
    }

    // If the text has any non-whitespace characters in it then the
    // line is not an empty line.
    while (--numFrags >= 0) {
      PRInt32 len = frag->GetLength();
      if (frag->Is2b()) {
        const PRUnichar* cp = frag->Get2b();
        const PRUnichar* end = cp + len;
        while (cp < end) {
          PRUnichar ch = *cp++;
          if (!XP_IS_SPACE(ch)) {
            NS_RELEASE(tc);
            NS_RELEASE(content);
            return PR_FALSE;
          }
        }
      }
      else {
        const char* cp = frag->Get1b();
        const char* end = cp + len;
        while (cp < end) {
          char ch = *cp++;
          if (!XP_IS_SPACE(ch)) {
            NS_RELEASE(tc);
            NS_RELEASE(content);
            return PR_FALSE;
          }
        }
      }
      frag++;
    }

    NS_RELEASE(tc);
    NS_RELEASE(content);
    frame->GetNextSibling(&frame);
  }
  return PR_TRUE;
}

void
nsBlockFrame::MarkEmptyLines(nsIPresContext& aPresContext)
{
  // PRE-formatted content considers whitespace significant
  const nsStyleText* text;
  GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&) text);
  if (NS_STYLE_WHITESPACE_PRE == text->mWhiteSpace) {
    return;
  }

  PRBool afterBlock = PR_TRUE;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (line->IsBlock()) {
      afterBlock = PR_TRUE;
    }
    else if (afterBlock) {
      afterBlock = PR_FALSE;

      // This is an inline line and it is immediately after a block
      // (or its our first line). See if it contains nothing but
      // collapsible text.
      PRBool isEmpty = IsEmptyLine(aPresContext, line);
      line->SetIsEmptyLine(isEmpty);
    }
    else {
      line->SetIsEmptyLine(PR_FALSE);
    }
    line = line->mNext;
  }
}

void
nsBlockFrame::DeleteChildsNextInFlow(nsIPresContext& aPresContext,
                                     nsIFrame* aChild)
{
  NS_PRECONDITION(IsChild(aChild), "bad geometric parent");

  nsIFrame* nextInFlow;
  nsBlockFrame* parent;
   
  aChild->GetNextInFlow(nextInFlow);
  NS_PRECONDITION(nsnull != nextInFlow, "null next-in-flow");
  nextInFlow->GetParent((nsIFrame**)&parent);

  // If the next-in-flow has a next-in-flow then delete it, too (and
  // delete it first).
  nsIFrame* nextNextInFlow;
  nextInFlow->GetNextInFlow(nextNextInFlow);
  if (nsnull != nextNextInFlow) {
    parent->DeleteChildsNextInFlow(aPresContext, nextInFlow);
  }

  // Disconnect the next-in-flow from the flow list
  nextInFlow->BreakFromPrevFlow();

  // Remove nextInFlow from the parents line list. Also remove it from
  // the sibling list.
  if (RemoveChild(parent->mLines, nextInFlow)) {
    NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
       ("nsBlockFrame::DeleteNextInFlowsFor: frame=%p (from mLines)",
        nextInFlow));
    goto done;
  }

  // If we get here then we didn't find the child on the line list. If
  // it's not there then it has to be on the overflow lines list.
  if (nsnull != mOverflowLines) {
    if (RemoveChild(parent->mOverflowLines, nextInFlow)) {
      NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
         ("nsBlockFrame::DeleteNextInFlowsFor: frame=%p (from overflow)",
          nextInFlow));
      goto done;
    }
  }
  NS_NOTREACHED("can't find next-in-flow in overflow list");

 done:;
  // If the parent is us then we will finish reflowing and update the
  // content offsets of our parents when we are a pseudo-frame; if the
  // parent is not us then it's a next-in-flow which means it will get
  // reflowed by our parent and fix its content offsets. So there.

  // Delete the next-in-flow frame and adjust its parents child count
  nextInFlow->DeleteFrame(aPresContext);

#ifdef NS_DEBUG
  aChild->GetNextInFlow(nextInFlow);
  NS_POSTCONDITION(nsnull == nextInFlow, "non null next-in-flow");
#endif
}

PRBool
nsBlockFrame::RemoveChild(nsLineBox* aLines, nsIFrame* aChild)
{
  nsLineBox* line = aLines;
  nsIFrame* prevChild = nsnull;
  while (nsnull != line) {
    nsIFrame* child = line->mFirstChild;
    PRInt32 n = line->ChildCount();
    while (--n >= 0) {
      nsIFrame* nextChild;
      child->GetNextSibling(&nextChild);
      if (child == aChild) {
        if (child == line->mFirstChild) {
          line->mFirstChild = nextChild;
        }
        if (0 == --line->mChildCount) {
          line->mFirstChild = nsnull;
        }
        if (nsnull != prevChild) {
          // Take child out of sibling list too
          prevChild->SetNextSibling(nextChild);
        }
        return PR_TRUE;
      }
      prevChild = child;
      child = nextChild;
    }
    line = line->mNext;
  }
  return PR_FALSE;
}

////////////////////////////////////////////////////////////////////////
// Floater support

void
nsBlockFrame::ReflowFloater(nsIPresContext& aPresContext,
                            nsBlockReflowState& aState,
                            nsIFrame* aFloaterFrame,
                            nsHTMLReflowState& aFloaterReflowState)
{
  // If either dimension is constrained then get the border and
  // padding values in advance.
  nsMargin bp(0, 0, 0, 0);
  if (aFloaterReflowState.HaveFixedContentWidth() ||
      aFloaterReflowState.HaveFixedContentHeight()) {
    nsHTMLReflowState::ComputeBorderPaddingFor(aFloaterFrame, &aState, bp);
  }

  // Compute the available width for the floater
  if (aFloaterReflowState.HaveFixedContentWidth()) {
    // When the floater has a contrained width, give it just enough
    // space for its styled width plus its borders and paddings.
    aFloaterReflowState.availableWidth = aFloaterReflowState.computedWidth + bp.left + bp.right;
  }
  else {
    // CSS2 section 10.3.5: Floating non-replaced elements with an
    // auto width have the computed value of zero. Therefore, don't
    // bother reflowing them.
    if (!NS_FRAME_IS_REPLACED(aFloaterReflowState.frameType)) {
      // XXX Tables are weird and special, so check for them here...
      const nsStyleDisplay* floaterDisplay;
      aFloaterFrame->GetStyleData(eStyleStruct_Display,
                                  (const nsStyleStruct*&)floaterDisplay);
      if (NS_STYLE_DISPLAY_TABLE != floaterDisplay->mDisplay) {
        return;
      }
    }
    aFloaterReflowState.availableWidth = NS_UNCONSTRAINEDSIZE;
  }

  // Compute the available height for the floater
  if (aFloaterReflowState.HaveFixedContentHeight()) {
    aFloaterReflowState.availableHeight = aFloaterReflowState.computedHeight + bp.top + bp.bottom;
  }
  else {
    aFloaterReflowState.availableHeight = NS_UNCONSTRAINEDSIZE;
  }

  // Resize reflow the anchored item into the available space
  nsIHTMLReflow*  floaterReflow;
  if (NS_OK == aFloaterFrame->QueryInterface(kIHTMLReflowIID,
                                             (void**)&floaterReflow)) {
    nsHTMLReflowMetrics desiredSize(nsnull);
    nsReflowStatus status;
    floaterReflow->WillReflow(aPresContext);
    floaterReflow->Reflow(aPresContext, desiredSize, aFloaterReflowState,
                          status);
    aFloaterFrame->SizeTo(desiredSize.width, desiredSize.height);
  }
}

void
nsBlockReflowState::InitFloater(nsPlaceholderFrame* aPlaceholder)
{
  // Set the geometric parent of the floater
  nsIFrame* floater = aPlaceholder->GetAnchoredItem();
  floater->SetParent(mBlock);

  // Then add the floater to the current line and place it when
  // appropriate
  AddFloater(aPlaceholder, PR_TRUE);
}

// This is called by the line layout's AddFloater method when a
// place-holder frame is reflowed in a line. If the floater is a
// left-most child (it's x coordinate is at the line's left margin)
// then the floater is place immediately, otherwise the floater
// placement is deferred until the line has been reflowed.
void
nsBlockReflowState::AddFloater(nsPlaceholderFrame* aPlaceholder,
                               PRBool aInitialReflow)
{
  // Update the current line's floater array
  NS_ASSERTION(nsnull != mCurrentLine, "null ptr");
  if (nsnull == mCurrentLine->mFloaters) {
    mCurrentLine->mFloaters = new nsVoidArray();
  }
  mCurrentLine->mFloaters->AppendElement(aPlaceholder);

  // Reflow the floater
  nsIFrame* floater = aPlaceholder->GetAnchoredItem();
  nsSize kidAvailSize(0, 0);
  nsHTMLReflowState reflowState(mPresContext, floater, *this, kidAvailSize);
  reflowState.lineLayout = nsnull;
  if ((nsnull == reflowCommand) || (floater != mNextRCFrame)) {
    // Stub out reflowCommand and repair reason in the reflowState
    // when incremental reflow doesn't apply to the floater.
    reflowState.reflowCommand = nsnull;
    reflowState.reason = ((reason == eReflowReason_Initial) || aInitialReflow)
      ? eReflowReason_Initial
      : eReflowReason_Resize;
  }
  mBlock->ReflowFloater(mPresContext, *this, floater, reflowState);

  // Now place the floater immediately if possible. Otherwise stash it
  // away in mPendingFloaters and place it later.
  if (0 == mLineLayout->GetPlacedFrames()) {
    NS_FRAME_LOG(NS_FRAME_TRACE_CHILD_REFLOW,
       ("nsBlockReflowState::AddFloater: IsLeftMostChild, placeHolder=%p",
        aPlaceholder));

    // Flush out pending bottom margin before placing floater
    if (0 != mPrevBottomMargin) {
      mY += mPrevBottomMargin;
      mPrevBottomMargin = 0;
    }

    // Because we are in the middle of reflowing a placeholder frame
    // within a line (and possibly nested in an inline frame or two
    // that's a child of our block) we need to restore the space
    // manager's translation to the space that the block resides in
    // before placing the floater.
    PRBool isLeftFloater;
    nscoord ox, oy;
    mSpaceManager->GetTranslation(ox, oy);
    nscoord dx = ox - mSpaceManagerX;
    nscoord dy = oy - mSpaceManagerY;
    mSpaceManager->Translate(-dx, -dy);
    PlaceFloater(aPlaceholder, isLeftFloater);

    // Pass on updated available space to the current inline reflow engine
    GetAvailableSpace();
    mLineLayout->UpdateInlines(mAvailSpaceRect.x + mBorderPadding.left,
                               mY,
                               mAvailSpaceRect.width,
                               mAvailSpaceRect.height,
                               isLeftFloater);

    // Restore coordinate system
    mSpaceManager->Translate(dx, dy);
  }
  else {
    // This floater will be placed after the line is done (it is a
    // below current line floater).
    NS_FRAME_LOG(NS_FRAME_TRACE_CHILD_REFLOW,
       ("nsBlockReflowState::AddFloater: pending, placeHolder=%p",
        aPlaceholder));
    mPendingFloaters.AppendElement(aPlaceholder);
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
      PRInt32 n = mCurrentLine->ChildCount();
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

void
nsBlockReflowState::PlaceFloater(nsPlaceholderFrame* aPlaceholder,
                                 PRBool& aIsLeftFloater)
{
  // Save away the Y coordinate before placing the floater. We will
  // restore mY at the end after placing the floater. This is
  // necessary because any adjustments to mY during the floater
  // placement are for the floater only, not for any non-floating
  // content.
  nscoord saveY = mY;
  nsIFrame* floater = aPlaceholder->GetAnchoredItem();

  // Get the type of floater
  const nsStyleDisplay* floaterDisplay;
  const nsStyleSpacing* floaterSpacing;
  floater->GetStyleData(eStyleStruct_Display,
                        (const nsStyleStruct*&)floaterDisplay);
  floater->GetStyleData(eStyleStruct_Spacing,
                        (const nsStyleStruct*&)floaterSpacing);

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
  nsMargin floaterMargin;
  ComputeMarginFor(floater, this, floaterMargin);

  // Adjust the floater size by its margin. That's the area that will
  // impact the space manager.
  region.width += floaterMargin.left + floaterMargin.right;
  region.height += floaterMargin.top + floaterMargin.bottom;

  // Find a place to place the floater. The CSS2 spec doesn't want
  // floaters overlapping each other or sticking out of the containing
  // block (CSS2 spec section 9.5.1, see the rule list).
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floaterDisplay->mFloats) ||
               (NS_STYLE_FLOAT_RIGHT == floaterDisplay->mFloats),
               "invalid float type");

  // While there is not enough room for the floater, clear past
  // other floaters until there is room (or the band is not impacted
  // by a floater).
  while ((mAvailSpaceRect.width < region.width) &&
         (mAvailSpaceRect.width < mContentArea.width)) {
    // The CSS2 spec says that floaters should be placed as high as
    // possible. We accomodate this easily by noting that if the band
    // is not the full width of the content area then it must have
    // been impacted by a floater. And we know that the height of the
    // band will be the height of the shortest floater, therefore we
    // adjust mY by that distance and keep trying until we have enough
    // space for this floater.
#ifdef NOISY_FLOATER_CLEARING
    mBlock->ListTag(stdout);
    printf(": clearing floater during floater placement: ");
    printf("availWidth=%d regionWidth=%d,%d(w/o margins) contentWidth=%d\n",
           mAvailSpaceRect.width, region.width,
           region.width - floaterMargin.left - floaterMargin.right,
           mContentArea.width);
#endif
    mY += mAvailSpaceRect.height;
    GetAvailableSpace();
  }

  // Assign an x and y coordinate to the floater. Note that the x,y
  // coordinates are computed <b>relative to the translation in the
  // spacemanager</b> which means that the impacted region will be
  // <b>inside</b> the border/padding area.
  if (NS_STYLE_FLOAT_LEFT == floaterDisplay->mFloats) {
    aIsLeftFloater = PR_TRUE;
    region.x = mAvailSpaceRect.x;
  }
  else {
    aIsLeftFloater = PR_FALSE;
    region.x = mAvailSpaceRect.XMost() - region.width;

    // In case the floater is too big, don't go past the left edge
    if (region.x < mAvailSpaceRect.x) {
      region.x = mAvailSpaceRect.x;
    }
  }
  region.y = mY - mBorderPadding.top;
  if (region.y < 0) {
    // CSS2 spec, 9.5.1 rule [4]: A floating box's outer top may not
    // be higher than the top of its containing block.

    // XXX It's not clear if it means the higher than the outer edge
    // or the border edge or the inner edge?
    region.y = 0;
  }

  // Place the floater in the space manager
  mSpaceManager->AddRectRegion(floater, region);

  // Set the origin of the floater frame, in frame coordinates. These
  // coordinates are <b>not</b> relative to the spacemanager
  // translation, therefore we have to factor in our border/padding.
  floater->MoveTo(mBorderPadding.left + floaterMargin.left + region.x,
                  mBorderPadding.top + floaterMargin.top + region.y);

  // Now restore mY
  mY = saveY;

#ifdef NOISY_INCREMENTAL_REFLOW
  if (reason == eReflowReason_Incremental) {
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
nsBlockReflowState::PlaceBelowCurrentLineFloaters(nsVoidArray* aFloaters)
{
  NS_PRECONDITION(aFloaters->Count() > 0, "no floaters");

  PRInt32 numFloaters = aFloaters->Count();
  for (PRInt32 i = 0; i < numFloaters; i++) {
    nsPlaceholderFrame* placeholderFrame = (nsPlaceholderFrame*)
      aFloaters->ElementAt(i);
    if (!IsLeftMostChild(placeholderFrame)) {
      PRBool isLeftFloater;
      PlaceFloater(placeholderFrame, isLeftFloater);
    }
  }
}

/**
 * Place current-line floaters.
 */
void
nsBlockReflowState::PlaceCurrentLineFloaters(nsVoidArray* aFloaters)
{
  NS_PRECONDITION(aFloaters->Count() > 0, "no floaters");

  PRInt32 numFloaters = aFloaters->Count();
  for (PRInt32 i = 0; i < numFloaters; i++) {
    nsPlaceholderFrame* placeholderFrame = (nsPlaceholderFrame*)
      aFloaters->ElementAt(i);
    if (IsLeftMostChild(placeholderFrame)) {
      PRBool isLeftFloater;
      PlaceFloater(placeholderFrame, isLeftFloater);
    }
  }
}

void
nsBlockReflowState::ClearFloaters(nscoord aY, PRUint8 aBreakType)
{
#ifdef NOISY_INCREMENTAL_REFLOW
  if (reason == eReflowReason_Incremental) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("clear floaters: in: mY=%d aY=%d(%d)\n",
           mY, aY, aY - mBorderPadding.top);
  }
#endif

  nscoord newY = mCurrentBand.ClearFloaters(aY - mBorderPadding.top,
                                            aBreakType);
  mY = newY + mBorderPadding.top;
  GetAvailableSpace();

#ifdef NOISY_INCREMENTAL_REFLOW
  if (reason == eReflowReason_Incremental) {
    nsFrame::IndentBy(stdout, gNoiseIndent);
    printf("clear floaters: out: mY=%d(%d)\n",
           mY, mY - mBorderPadding.top);
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

NS_IMETHODIMP
nsBlockFrame::Paint(nsIPresContext&      aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect,
                     nsFramePaintLayer    aWhichLayer)
{
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    mStyleContext->GetStyleData(eStyleStruct_Display);

  // Only paint the border and background if we're visible
  if (disp->mVisible && (eFramePaintLayer_Underlay == aWhichLayer)) {
    PRIntn skipSides = GetSkipSides();
    const nsStyleColor* color = (const nsStyleColor*)
      mStyleContext->GetStyleData(eStyleStruct_Color);
    const nsStyleSpacing* spacing = (const nsStyleSpacing*)
      mStyleContext->GetStyleData(eStyleStruct_Spacing);

    // Paint background and border
    nsRect rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *color, *spacing, 0, 0);
    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                aDirtyRect, rect, *spacing, mStyleContext,
                                skipSides);
  }

  // If overflow is hidden then set the clip rect so that children
  // don't leak out of us
  if (NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) {
    PRBool clipState;
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(nsRect(0, 0, mRect.width, mRect.height),
                                  nsClipCombine_kIntersect, clipState);
  }

  // Child elements have the opportunity to override the visibility
  // property and display even if the parent is hidden
  PaintFloaters(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

  if (NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) {
    PRBool clipState;
    aRenderingContext.PopState(clipState);
  }

  if (eFramePaintLayer_Overlay == aWhichLayer) {
    // XXX CSS2's outline handling goes here
  }
  return NS_OK;
}

void
nsBlockFrame::PaintFloaters(nsIPresContext& aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect,
                             nsFramePaintLayer aWhichLayer)
{
  for (nsLineBox* line = mLines; nsnull != line; line = line->mNext) {
    nsVoidArray* floaters = line->mFloaters;
    if (nsnull == floaters) {
      continue;
    }
    PRInt32 i, n = floaters->Count();
    for (i = 0; i < n; i++) {
      nsPlaceholderFrame* ph = (nsPlaceholderFrame*) floaters->ElementAt(i);
      PaintChild(aPresContext, aRenderingContext, aDirtyRect,
                 ph->GetAnchoredItem(), aWhichLayer);
    }
  }
}

void
nsBlockFrame::PaintChildren(nsIPresContext& aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect,
                             nsFramePaintLayer aWhichLayer)
{
  for (nsLineBox* line = mLines; nsnull != line; line = line->mNext) {
    // If the line has outside children or if the line intersects the
    // dirty rect then paint the children in the line.
    if (!((line->mCombinedArea.YMost() <= aDirtyRect.y) ||
          (line->mCombinedArea.y >= aDirtyRect.YMost()))) {
      nsIFrame* kid = line->mFirstChild;
      PRInt32 n = line->ChildCount();
      while (--n >= 0) {
        PaintChild(aPresContext, aRenderingContext, aDirtyRect, kid,
                   aWhichLayer);
        kid->GetNextSibling(&kid);
      }
    }
  }

  if (eFramePaintLayer_Content == aWhichLayer) {
    if ((nsnull != mBullet) && HaveOutsideBullet()) {
      // Paint outside bullets manually
      PaintChild(aPresContext, aRenderingContext, aDirtyRect, mBullet,
                 aWhichLayer);
    }
  }
}

NS_IMETHODIMP
nsBlockFrame::GetFrameForPoint(const nsPoint& aPoint, nsIFrame** aFrame)
{
  nsresult rv = GetFrameForPointUsing(aPoint, nsnull, aFrame);
  if (NS_OK == rv) {
    return NS_OK;
  }
  if (nsnull != mBullet) {
    rv = GetFrameForPointUsing(aPoint, nsLayoutAtoms::bulletList, aFrame);
    if (NS_OK == rv) {
      return NS_OK;
    }
  }
  if (mFloaters.NotEmpty()) {
    rv = GetFrameForPointUsing(aPoint, nsLayoutAtoms::floaterList, aFrame);
    if (NS_OK == rv) {
      return NS_OK;
    }
  }
  *aFrame = this;
  return NS_ERROR_FAILURE;
}

//////////////////////////////////////////////////////////////////////
// Debugging

#ifdef NS_DEBUG
static PRBool
InLineList(nsLineBox* aLines, nsIFrame* aFrame)
{
  while (nsnull != aLines) {
    nsIFrame* frame = aLines->mFirstChild;
    PRInt32 n = aLines->ChildCount();
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
nsBlockFrame::IsChild(nsIFrame* aFrame)
{
  nsIFrame* parent;
  aFrame->GetParent(&parent);
  if (parent != (nsIFrame*)this) {
    return PR_FALSE;
  }
  if (InLineList(mLines, aFrame) && InSiblingList(mLines, aFrame)) {
    return PR_TRUE;
  }
  if (InLineList(mOverflowLines, aFrame) &&
      InSiblingList(mOverflowLines, aFrame)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}
#endif

NS_IMETHODIMP
nsBlockFrame::VerifyTree() const
{
  // XXX rewrite this
  return NS_OK;
}

//----------------------------------------------------------------------

NS_IMETHODIMP
nsBlockFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList)
{
  nsresult rv = AppendNewFrames(aPresContext, aChildList);
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
    aPresContext.ResolvePseudoStyleContextFor(mContent, 
                                              nsHTMLAtoms::mozListBulletPseudo,
                                              mStyleContext, PR_FALSE, &kidSC);

    // Create bullet frame
    mBullet = new nsBulletFrame;
    if (nsnull == mBullet) {
      NS_RELEASE(kidSC);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mBullet->Init(aPresContext, mContent, this, kidSC);
    NS_RELEASE(kidSC);

    // If the list bullet frame should be positioned inside then add
    // it to the flow now.
    const nsStyleList* styleList;
    GetStyleData(eStyleStruct_List, (const nsStyleStruct*&) styleList);
    if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == styleList->mListStylePosition) {
      InsertNewFrames(aPresContext, mBullet, nsnull);
      mState &= ~NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
    }
    else {
      mState |= NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET;
    }
  }

  // Lookup up the two pseudo style contexts
  if (nsnull == mPrevInFlow) {
    aPresContext.
      ProbePseudoStyleContextFor(mContent, nsHTMLAtoms::firstLinePseudo,
                                 mStyleContext, PR_FALSE, &mFirstLineStyle);
    aPresContext.
      ProbePseudoStyleContextFor(mContent, nsHTMLAtoms::firstLetterPseudo,
                                 (nsnull != mFirstLineStyle
                                  ? mFirstLineStyle
                                  : mStyleContext),
                                 PR_FALSE, &mFirstLetterStyle);
#ifdef NOISY_FIRST_LETTER
    if (nsnull != mFirstLetterStyle) {
      printf("block(%d)@%p: first-letter style found\n",
             ContentIndexInContainer(this), this);
    }
#endif
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBlockFrame::CreateContinuingFrame(nsIPresContext& aPresContext,
                                    nsIFrame* aParent,
                                    nsIStyleContext* aStyleContext,
                                    nsIFrame*& aContinuingFrame)
{
  nsBlockFrame* cf = new nsBlockFrame;
  if (nsnull == cf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  cf->Init(aPresContext, mContent, aParent, aStyleContext);
  cf->SetFlags(mFlags);
  cf->AppendToFlow(this);
  aContinuingFrame = cf;
  return NS_OK;
}

void
nsBlockFrame::RenumberLists(nsBlockReflowState& aState)
{
  // Setup initial list ordinal value
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
  aState.mNextListOrdinal = ordinal;

  // Get to first-in-flow
  nsBlockFrame* block = this;
  while (nsnull != block->mPrevInFlow) {
    block = (nsBlockFrame*) block->mPrevInFlow;
  }

  // For each flow-block...
  while (nsnull != block) {
    // For each frame in the flow-block...
    nsIFrame* frame = block->mLines ? block->mLines->mFirstChild : nsnull;
    while (nsnull != frame) {
      // If the frame is a list-item and the frame implements our
      // block frame API then get it's bullet and set the list item
      // ordinal.
      const nsStyleDisplay* display;
      frame->GetStyleData(eStyleStruct_Display,
                          (const nsStyleStruct*&) display);
      if (NS_STYLE_DISPLAY_LIST_ITEM == display->mDisplay) {
        // Make certain that the frame isa block-frame in case
        // something foriegn has crept in.
        nsBlockFrame* listItem;
        if (NS_OK == frame->QueryInterface(kBlockFrameCID,
                                           (void**) &listItem)) {
          if (nsnull != listItem->mBullet) {
            aState.mNextListOrdinal =
              listItem->mBullet->SetListItemOrdinal(aState.mNextListOrdinal);
          }
        }
      }
      frame->GetNextSibling(&frame);
    }
    block = (nsBlockFrame*) block->mNextInFlow;
  }
}

void
nsBlockFrame::ReflowBullet(nsBlockReflowState& aState,
                           nsHTMLReflowMetrics& aMetrics)
{
  // Reflow the bullet now
  nsSize availSize;
  availSize.width = NS_UNCONSTRAINEDSIZE;
  availSize.height = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState reflowState(aState.mPresContext, mBullet, aState,
                                availSize, aState.mLineLayout);
  nsIHTMLReflow* htmlReflow;
  nsresult rv = mBullet->QueryInterface(kIHTMLReflowIID, (void**)&htmlReflow);
  if (NS_SUCCEEDED(rv)) {
    nsReflowStatus  status;
    htmlReflow->WillReflow(aState.mPresContext);
    htmlReflow->Reflow(aState.mPresContext, aMetrics, reflowState, status);
    htmlReflow->DidReflow(aState.mPresContext, NS_FRAME_REFLOW_FINISHED);
  }

  // Place the bullet now; use its right margin to distance it
  // from the rest of the frames in the line
  nsMargin margin;
  nsHTMLReflowState::ComputeMarginFor(mBullet, &aState, margin);
  nscoord x = aState.mBorderPadding.left - margin.right - aMetrics.width;

  // Approximate the bullets position; vertical alignment will provide
  // the final vertical location.
  nscoord y = aState.mBorderPadding.top;
  mBullet->SetRect(nsRect(x, y, aMetrics.width, aMetrics.height));
}

void
nsBlockFrame::BuildFloaterList()
{
  nsIFrame* head = nsnull;
  nsIFrame* current = nsnull;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (nsnull != line->mFloaters) {
      nsVoidArray& array = *line->mFloaters;
      PRInt32 i, n = array.Count();
      for (i = 0; i < n; i++) {
        nsPlaceholderFrame* ph = (nsPlaceholderFrame*) array[i];
        nsIFrame* floater = ph->GetAnchoredItem();
        if (nsnull == head) {
          current = head = floater;
        }
        else {
          current->SetNextSibling(floater);
          current = floater;
        }
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
nsresult
nsBlockFrame::ComputeTextRuns(nsBlockReflowState& aState)
{
  // Destroy old run information first
  nsTextRun::DeleteTextRuns(mTextRuns);
  mTextRuns = nsnull;
  aState.mLineLayout->ResetTextRuns();

  // Ask each child that implements nsIInlineReflow to find its text runs
  nsLineLayout& ll = *aState.mLineLayout;
  nsLineBox* line = mLines;
  while (nsnull != line) {
    if (!line->IsBlock()) {
      nsIFrame* frame = line->mFirstChild;
      PRInt32 n = line->ChildCount();
      while (--n >= 0) {
        nsIHTMLReflow* hr;
        if (NS_OK == frame->QueryInterface(kIHTMLReflowIID, (void**)&hr)) {
          nsresult rv = hr->FindTextRuns(ll);
          if (NS_OK != rv) {
            return rv;
          }
        }
        else {
          // A frame that doesn't implement nsIHTMLReflow isn't text
          // therefore it will end an open text run.
          ll.EndTextRun();
        }
        frame->GetNextSibling(&frame);
      }
    }
    else {
      // A frame that doesn't implement nsIInlineReflow isn't text
      // therefore it will end an open text run.
      ll.EndTextRun();
    }
    line = line->mNext;
  }
  ll.EndTextRun();

  // Now take the text-runs away from the line layout engine.
  mTextRuns = ll.TakeTextRuns();
  return NS_OK;
}

void
nsBlockFrame::TakeRunInFrames(nsBlockFrame* aRunInFrame)
{
  // Simply steal the run-in-frame's line list and make it our
  // own. XXX Very similar to the logic in DrainOverflowLines...
  nsLineBox* line = aRunInFrame->mLines;

  // Make all the frames on the mOverflowLines list mine
  nsIFrame* lastFrame = nsnull;
  nsIFrame* frame = line->mFirstChild;
  while (nsnull != frame) {
    frame->SetParent(this);
    lastFrame = frame;
    frame->GetNextSibling(&frame);
  }

  // Join the line lists
  if (nsnull == mLines) {
    mLines = line;
  }
  else {
    // Join the sibling lists together
    lastFrame->SetNextSibling(mLines->mFirstChild);

    // Place overflow lines at the front of our line list
    nsLineBox* lastLine = nsLineBox::LastLine(line);
    lastLine->mNext = mLines;
    mLines = line;
  }

  aRunInFrame->mLines = nsnull;
}

//----------------------------------------------------------------------

nsresult
NS_NewAnonymousBlockFrame(nsIFrame*& aNewFrame)
{
  nsBlockFrame* it = new nsAnonymousBlockFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aNewFrame = it;
  return NS_OK;
}

nsAnonymousBlockFrame::nsAnonymousBlockFrame()
{
}

nsAnonymousBlockFrame::~nsAnonymousBlockFrame()
{
}

NS_IMETHODIMP
nsAnonymousBlockFrame::AppendFrames(nsIPresContext& aPresContext,
                                    nsIPresShell&   aPresShell,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aFrameList)
{
  return mParent->AppendFrames(aPresContext, aPresShell, aListName,
                               aFrameList);
}

NS_IMETHODIMP
nsAnonymousBlockFrame::InsertFrames(nsIPresContext& aPresContext,
                                    nsIPresShell&   aPresShell,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aPrevFrame,
                                    nsIFrame*       aFrameList)
{
  return mParent->InsertFrames(aPresContext, aPresShell, aListName,
                               aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsAnonymousBlockFrame::RemoveFrame(nsIPresContext& aPresContext,
                                   nsIPresShell&   aPresShell,
                                   nsIAtom*        aListName,
                                   nsIFrame*       aOldFrame)
{
  return mParent->RemoveFrame(aPresContext, aPresShell, aListName,
                              aOldFrame);
}

nsresult
nsAnonymousBlockFrame::AppendFrames2(nsIPresContext& aPresContext,
                                     nsIPresShell&   aPresShell,
                                     nsIAtom*        aListName,
                                     nsIFrame*       aFrameList)
{
  return nsAnonymousBlockFrameSuper::AppendFrames(aPresContext, aPresShell,
                                                  aListName, aFrameList);
}

nsresult
nsAnonymousBlockFrame::InsertFrames2(nsIPresContext& aPresContext,
                                     nsIPresShell&   aPresShell,
                                     nsIAtom*        aListName,
                                     nsIFrame*       aPrevFrame,
                                     nsIFrame*       aFrameList)
{
  return nsAnonymousBlockFrameSuper::InsertFrames(aPresContext, aPresShell,
                                                  aListName, aPrevFrame,
                                                  aFrameList);
}

nsresult
nsAnonymousBlockFrame::RemoveFrame2(nsIPresContext& aPresContext,
                                    nsIPresShell&   aPresShell,
                                    nsIAtom*        aListName,
                                    nsIFrame*       aOldFrame)
{
  return nsAnonymousBlockFrameSuper::RemoveFrame(aPresContext, aPresShell,
                                                 aListName, aOldFrame);
}

void
nsAnonymousBlockFrame::RemoveFirstFrame()
{
  nsLineBox* line = mLines;
  if (nsnull != line) {
    nsIFrame* firstChild = line->mFirstChild;

    // If the line has floaters on it, see if the frame being removed
    // is a placeholder frame. If it is, then remove it from the lines
    // floater array and from the block frames floater child list.
    if (nsnull != line->mFloaters) {
      // XXX UNTESTED!
      nsPlaceholderFrame* placeholderFrame;
      nsVoidArray& floaters = *line->mFloaters;
      PRInt32 i, n = floaters.Count();
      for (i = 0; i < n; i++) {
        placeholderFrame = (nsPlaceholderFrame*) floaters[i];
        if (firstChild == placeholderFrame) {
          // Remove placeholder from the line's floater array
          floaters.RemoveElementAt(i);
          if (0 == floaters.Count()) {
            delete line->mFloaters;
            line->mFloaters = nsnull;
          }

          // Remove the floater from the block frames mFloaters list too
          mFloaters.RemoveFrame(placeholderFrame->GetAnchoredItem());
          break;
        }
      }
    }

    if (1 == line->mChildCount) {
      // Remove line when last frame goes away
      mLines = line->mNext;
      delete line;
    }
    else {
      // Remove frame from line and mark the line dirty
      --line->mChildCount;
      line->MarkDirty();
      firstChild->GetNextSibling(&line->mFirstChild);
    }

    // Break linkage to next child after stolen frame
    firstChild->SetNextSibling(nsnull);
  }
}

void
nsAnonymousBlockFrame::RemoveFramesFrom(nsIFrame* aFrame)
{
  nsLineBox* line = mLines;
  if (nsnull != line) {
    // Chop the child sibling list into two pieces
    nsFrameList tmp(line->mFirstChild);
    nsIFrame* prevSibling = tmp.GetPrevSiblingFor(aFrame);
    if (nsnull != prevSibling) {
      // Chop the sibling list into two pieces
      prevSibling->SetNextSibling(nsnull);

      nsLineBox* prevLine = nsnull;
      while (nsnull != line) {
        nsIFrame* frame = line->mFirstChild;
        PRInt32 i, n = line->mChildCount;
        PRBool done = PR_FALSE;
        for (i = 0; i < n; i++) {
          if (frame == aFrame) {
            // We just found the target frame (and the line its in and
            // the previous line)
            if (frame == line->mFirstChild) {
              // No more children on this line, so let it get removed
              prevLine->mNext = nsnull;
            }
            else {
              // The only frames that remain on this line are the
              // frames preceeding aFrame. Adjust the count to
              // indicate that fact.
              line->mChildCount = i;

              // Remove the lines that follow this line
              prevLine = line;
              line = line->mNext;
              prevLine->mNext = nsnull;
            }
            done = PR_TRUE;
            break;
          }
          frame->GetNextSibling(&frame);
        }
        if (done) {
          break;
        }
        prevLine = line;
        line = line->mNext;
      }
    }

    // Remove all of the remaining lines
    while (nsnull != line) {
      nsLineBox* next = line->mNext;
      delete line;
      line = next;
    }
  }
}

#if 0
nscoord
nsBlockFrame::ComputeCollapsedTopMargin(const nsHTMLReflowState& aReflowState)
{
  nscoord myTopMargin = aReflowState.computedMargin.top;
  nsLineBox* line = mLines;
  if (nsnull != line) {
    if (line->IsEmptyLine()) {
      line = line->mNext;
      if (nsnull == line) {
        return myTopMargin;
      }
    }
    if (1 == line->ChildCount()) {
      nsHTMLReflowState rs();
      nscoord topMargin = line->mFirstChild->ComputeCollapsedTopMargin(rs);
      return nsBlockReflowContext::MaxMargin(topMargin, myTopMargin);
    }
  }
  return myTopMargin;
}
#endif
