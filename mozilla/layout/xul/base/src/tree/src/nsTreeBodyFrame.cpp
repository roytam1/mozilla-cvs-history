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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsIPresContext.h"
#include "nsINameSpaceManager.h"
#include "nsIScrollbarFrame.h"

#include "nsOutlinerBodyFrame.h"
#include "nsOutlinerSelection.h"

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"

#include "nsIContent.h"
#include "nsIStyleContext.h"
#include "nsIBoxObject.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNSDocument.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsICSSStyleRule.h"
#include "nsCSSRendering.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsXPIDLString.h"
#include "nsHTMLContainerFrame.h"
#include "nsIView.h"
#include "nsWidgetsCID.h"
#include "nsBoxFrame.h"

#define ELLIPSIS "..."

// The style context cache impl
nsresult 
nsOutlinerStyleCache::GetStyleContext(nsICSSPseudoComparator* aComparator,
                                      nsIPresContext* aPresContext, nsIContent* aContent, 
                                      nsIStyleContext* aContext, nsIAtom* aPseudoElement,
                                      nsISupportsArray* aInputWord,
                                      nsIStyleContext** aResult)
{
  *aResult = nsnull;
  
  PRUint32 count;
  aInputWord->Count(&count);
  nsDFAState startState(0);
  nsDFAState* currState = &startState;

  // Go ahead and init the transition table.
  if (!mTransitionTable) {
    // Automatic miss. Build the table
    mTransitionTable = new nsHashtable;
  }

  // The first transition is always made off the supplied pseudo-element.
  nsTransitionKey key(currState->GetStateID(), aPseudoElement);
  currState = NS_STATIC_CAST(nsDFAState*, mTransitionTable->Get(&key));

  if (!currState) {
    // We had a miss. Make a new state and add it to our hash.
    currState = new nsDFAState(mNextState);
    mNextState++;
    mTransitionTable->Put(&key, currState);
  }

  for (PRUint32 i = 0; i < count; i++)
  {
    nsCOMPtr<nsIAtom> pseudo = getter_AddRefs(NS_STATIC_CAST(nsIAtom*, aInputWord->ElementAt(i)));
    nsTransitionKey key(currState->GetStateID(), pseudo);
    currState = NS_STATIC_CAST(nsDFAState*, mTransitionTable->Get(&key));

    if (!currState) {
      // We had a miss. Make a new state and add it to our hash.
      currState = new nsDFAState(mNextState);
      mNextState++;
      mTransitionTable->Put(&key, currState);
    }
  }

  // We're in a final state.
  // Look up our style context for this state.
   if (mCache)
    *aResult = NS_STATIC_CAST(nsIStyleContext*, mCache->Get(currState)); // Addref occurs on *aResult.
  if (!*aResult) {
    // We missed the cache. Resolve this pseudo-style.
    aPresContext->ResolvePseudoStyleWithComparator(aContent, aPseudoElement,
                                                   aContext, PR_FALSE,
                                                   aComparator,
                                                   aResult); // Addref occurs on *aResult.
    // Put it in our table.
    if (!mCache)
      mCache = new nsSupportsHashtable;
    mCache->Put(currState, *aResult);
  }

  return NS_OK;
}

// Column class that caches all the info about our column.
nsOutlinerColumn::nsOutlinerColumn(nsIContent* aColElement, nsIFrame* aFrame)
:mNext(nsnull)
{
  mColFrame = aFrame;
  mColElement = aColElement; 

  // Fetch the ID.
  mColElement->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::id, mID);

  // Fetch the crop style.
  mCropStyle = 0;
  nsAutoString crop;
  mColElement->GetAttribute(kNameSpaceID_None, nsXULAtoms::crop, crop);
  if (crop.EqualsIgnoreCase("center"))
    mCropStyle = 1;
  else if (crop.EqualsIgnoreCase("left"))
    mCropStyle = 2;

  // Cache our text alignment policy.
  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetStyleContext(getter_AddRefs(styleContext));

  const nsStyleText* textStyle =
        (const nsStyleText*)styleContext->GetStyleData(eStyleStruct_Text);

  mTextAlignment = textStyle->mTextAlign;

  // Figure out if we're the primary column (that has to have indentation
  // and twisties drawn.
  mIsPrimaryCol = PR_FALSE;
  nsAutoString primary;
  mColElement->GetAttribute(kNameSpaceID_None, nsXULAtoms::primary, primary);
  if (primary.EqualsIgnoreCase("true"))
    mIsPrimaryCol = PR_TRUE;

  // Figure out if we're a cycling column (one that doesn't cause a selection
  // to happen).
  mIsCyclerCol = PR_FALSE;
  nsAutoString cycler;
  mColElement->GetAttribute(kNameSpaceID_None, nsXULAtoms::cycler, cycler);
  if (cycler.EqualsIgnoreCase("true"))
    mIsCyclerCol = PR_TRUE;
}

inline nscoord nsOutlinerColumn::GetWidth()
{
  if (mColFrame) {
    nsRect rect;
    mColFrame->GetRect(rect);
    return rect.width;
  }
  return 0;
}

//
// NS_NewOutlinerFrame
//
// Creates a new outliner frame
//
nsresult
NS_NewOutlinerBodyFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsOutlinerBodyFrame* it = new (aPresShell) nsOutlinerBodyFrame(aPresShell);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewOutlinerFrame


// Constructor
nsOutlinerBodyFrame::nsOutlinerBodyFrame(nsIPresShell* aPresShell)
:nsLeafBoxFrame(aPresShell), mPresContext(nsnull),
 mTopRowIndex(0), mRowHeight(0), mIndentation(0), mColumns(nsnull), mScrollbar(nsnull)
{
  NS_NewISupportsArray(getter_AddRefs(mScratchArray));
}

// Destructor
nsOutlinerBodyFrame::~nsOutlinerBodyFrame()
{
}

NS_IMETHODIMP_(nsrefcnt) 
nsOutlinerBodyFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsOutlinerBodyFrame::Release(void)
{
  return NS_OK;
}

static nsIFrame* InitScrollbarFrame(nsIPresContext* aPresContext, nsIFrame* aCurrFrame, nsIScrollbarMediator* aSM)
{
  // Check ourselves
  nsCOMPtr<nsIScrollbarFrame> sf(do_QueryInterface(aCurrFrame));
  if (sf) {
    sf->SetScrollbarMediator(aSM);
    return aCurrFrame;
  }

  nsIFrame* child;
  aCurrFrame->FirstChild(aPresContext, nsnull, &child);
  while (child) {
    nsIFrame* result = InitScrollbarFrame(aPresContext, child, aSM);
    if (result)
      return result;
    child->GetNextSibling(&child);
  }

  return nsnull;
}

NS_IMETHODIMP
nsOutlinerBodyFrame::Init(nsIPresContext* aPresContext, nsIContent* aContent,
                          nsIFrame* aParent, nsIStyleContext* aContext, nsIFrame* aPrevInFlow)
{
  mPresContext = aPresContext;
  nsresult rv = nsLeafBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  nsBoxFrame::CreateViewForFrame(aPresContext, this, aContext, PR_TRUE);
  nsIView* ourView;
  nsLeafBoxFrame::GetView(aPresContext, &ourView);

static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);

  ourView->CreateWidget(kWidgetCID);
  ourView->GetWidget(*getter_AddRefs(mOutlinerWidget));

  return rv;
}

NS_IMETHODIMP
nsOutlinerBodyFrame::Destroy(nsIPresContext* aPresContext)
{
  // Delete our column structures.
  delete mColumns;
  mColumns = nsnull;

  // Drop our ref to the view.
  if (mView)
    mView->SetOutliner(nsnull);
  mView = nsnull;

  return nsLeafBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP nsOutlinerBodyFrame::GetView(nsIOutlinerView * *aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::SetView(nsIOutlinerView * aView)
{
  // Outliner, meet the view.
  mView = aView;
 
  // Changing the view causes us to refetch our data.  This will
  // necessarily entail a full invalidation of the outliner.
  mTopRowIndex = 0;
  delete mColumns;
  mColumns = nsnull;
  Invalidate();
 
  if (mView) {
    // View, meet the outliner.
    mView->SetOutliner(this);
    
    // Give the view a new empty selection object to play with.
    nsCOMPtr<nsIOutlinerSelection> sel;
    NS_NewOutlinerSelection(this, getter_AddRefs(sel));

    mView->SetSelection(sel);

    // The scrollbar will need to be updated.
    InvalidateScrollbar();
  }
 
  return NS_OK;
}

NS_IMETHODIMP 
nsOutlinerBodyFrame::GetOutlinerBody(nsIDOMElement** aElement)
{
  return mContent->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
}

NS_IMETHODIMP nsOutlinerBodyFrame::GetSelection(nsIOutlinerSelection** aSelection)
{
  if (mView)
    return mView->GetSelection(aSelection);

  *aSelection = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::GetIndexOfVisibleRow(PRInt32 *_retval)
{
  *_retval = mTopRowIndex;
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::GetPageCount(PRInt32 *_retval)
{
  *_retval = mPageCount;
  return NS_OK;
}


NS_IMETHODIMP nsOutlinerBodyFrame::Invalidate()
{
  nsLeafBoxFrame::Invalidate(mPresContext, mRect, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::InvalidateRow(PRInt32 aIndex)
{
  if (aIndex < mTopRowIndex || aIndex > mTopRowIndex + mPageCount + 1)
    return NS_OK;

  nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*(aIndex-mTopRowIndex), mInnerBox.width, mRowHeight);
  nsLeafBoxFrame::Invalidate(mPresContext, rowRect, PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::InvalidateCell(PRInt32 aRow, const PRUnichar *aColID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsOutlinerBodyFrame::InvalidateRange(PRInt32 aStart, PRInt32 aEnd)
{
  if (aStart == aEnd)
    return InvalidateRow(aStart);

  
  if (aEnd < mTopRowIndex || aStart > mTopRowIndex + mPageCount + 1)
    return NS_OK;

  if (aStart < mTopRowIndex)
    aStart = mTopRowIndex;

  if (aEnd > mTopRowIndex + mPageCount + 1)
    aEnd = mTopRowIndex + mPageCount + 1;

  nsRect rangeRect(mInnerBox.x, mInnerBox.y+mRowHeight*(aStart-mTopRowIndex), mInnerBox.width, mRowHeight*(aEnd-aStart+1));
  nsLeafBoxFrame::Invalidate(mPresContext, rangeRect, PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::InvalidateScrollbar()
{
  if (!mScrollbar)
    // Try to find it.
    mScrollbar = InitScrollbarFrame(mPresContext, mParent, this);

  if (!mScrollbar || !mView)
    return NS_OK;

  PRInt32 rowCount = 0;
  mView->GetRowCount(&rowCount);
  
  nsCOMPtr<nsIContent> scrollbar;
  mScrollbar->GetContent(getter_AddRefs(scrollbar));

  nsAutoString maxposStr;

  float t2p;
  mPresContext->GetTwipsToPixels(&t2p);
  nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

  PRInt32 size = rowHeightAsPixels*(rowCount-mPageCount+1);
  maxposStr.AppendInt(size);
  scrollbar->SetAttribute(kNameSpaceID_None, nsXULAtoms::maxpos, maxposStr, PR_TRUE);

  // Also set our page increment and decrement.
  nscoord pageincrement = mPageCount*rowHeightAsPixels;
  nsAutoString pageStr;
  pageStr.AppendInt(pageincrement);
  scrollbar->SetAttribute(kNameSpaceID_None, nsXULAtoms::pageincrement, pageStr, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::GetCellAt(PRInt32 aX, PRInt32 aY, PRInt32* aRow, PRUnichar** aColID)
{
  // Ensure we have a row height.
  if (mRowHeight == 0)
    mRowHeight = GetRowHeight();

  // Convert our x and y coords to twips.
  float pixelsToTwips = 0.0;
  mPresContext->GetPixelsToTwips(&pixelsToTwips);
  aX = NSToIntRound(aX * pixelsToTwips);
  aY = NSToIntRound(aY * pixelsToTwips);
  
  // Get our box object.
  nsCOMPtr<nsIDocument> doc;
  mContent->GetDocument(*getter_AddRefs(doc));
  nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(mContent));
  
  nsCOMPtr<nsIBoxObject> boxObject;
  nsDoc->GetBoxObjectFor(elt, getter_AddRefs(boxObject));
  
  PRInt32 x;
  PRInt32 y;
  boxObject->GetX(&x);
  boxObject->GetY(&y);

  x = NSToIntRound(x * pixelsToTwips);
  y = NSToIntRound(y * pixelsToTwips);

  // Adjust into our coordinate space.
  x = aX-x;
  y = aY-y;

  // Adjust y by the inner box y, so that we're in the inner box's
  // coordinate space.
  y += mInnerBox.y;

  // Now just mod by our total inner box height and add to our top row index.
  *aRow = (y/mRowHeight)+mTopRowIndex;

  // Determine the column hit.
  nscoord currX = mInnerBox.x;
  for (nsOutlinerColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {
    nsRect colRect(currX, mInnerBox.y, currCol->GetWidth(), mInnerBox.height);
    PRInt32 overflow = colRect.x+colRect.width-(mInnerBox.x+mInnerBox.width);
    if (overflow > 0)
      colRect.width -= overflow;

    if (x >= colRect.x && x < colRect.x + colRect.width)
      *aColID = nsXPIDLString::Copy(currCol->GetID());

    currX += colRect.width;
  }
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::RowsAppended(PRInt32 count)
{
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::RowsInserted(PRInt32 index, PRInt32 count)
{
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::RowsRemoved(PRInt32 index, PRInt32 count)
{
  return NS_OK;
}

void
nsOutlinerBodyFrame::PrefillPropertyArray(PRInt32 aRowIndex, const PRUnichar* aColID)
{
  // XXX Automatically fill in the following props: container, open, selected, focused
  // And colID too, if it is non-empty.
  mScratchArray->Clear();
  
  if (aRowIndex != -1) {
    nsCOMPtr<nsIOutlinerSelection> selection;
    mView->GetSelection(getter_AddRefs(selection));
  
    PRBool isSelected;
    selection->IsSelected(aRowIndex, &isSelected);
    if (isSelected)
      mScratchArray->AppendElement(nsHTMLAtoms::selected);

    PRInt32 currentIndex;
    selection->GetCurrentIndex(&currentIndex);
    if (aRowIndex == currentIndex)
      mScratchArray->AppendElement(nsXULAtoms::current);
  }
}


PRInt32 nsOutlinerBodyFrame::GetRowHeight()
{
  // Look up the correct height.  It is equal to the specified height
  // + the specified margins.
  nsCOMPtr<nsIStyleContext> rowContext;
  mScratchArray->Clear();
  GetPseudoStyleContext(mPresContext, nsXULAtoms::mozoutlinerrow, getter_AddRefs(rowContext));
  if (rowContext) {
    const nsStylePosition* myPosition = (const nsStylePosition*)
          rowContext->GetStyleData(eStyleStruct_Position);
    if (myPosition->mHeight.GetUnit() == eStyleUnit_Coord)  {
      PRInt32 val = myPosition->mHeight.GetCoordValue();
      if (val > 0) {
        // XXX Check box-sizing to determine if border/padding should augment the height
        // Inflate the height by our margins.
        nsRect rowRect(0,0,0,val);
        const nsStyleMargin* rowMarginData = (const nsStyleMargin*)rowContext->GetStyleData(eStyleStruct_Margin);
        nsMargin rowMargin;
        rowMarginData->GetMargin(rowMargin);
        rowRect.Inflate(rowMargin);
        val = rowRect.height;
      }
      return val;
    }
  }
  return 19*15; // As good a default as any.
}

PRInt32 nsOutlinerBodyFrame::GetIndentation()
{
  // Look up the correct indentation.  It is equal to the specified indentation width.
  nsCOMPtr<nsIStyleContext> indentContext;
  mScratchArray->Clear();
  GetPseudoStyleContext(mPresContext, nsXULAtoms::mozoutlinerindentation, getter_AddRefs(indentContext));
  if (indentContext) {
    const nsStylePosition* myPosition = (const nsStylePosition*)
          indentContext->GetStyleData(eStyleStruct_Position);
    if (myPosition->mWidth.GetUnit() == eStyleUnit_Coord)  {
      PRInt32 val = myPosition->mWidth.GetCoordValue();
      return val;
    }
  }
  return 16*15; // As good a default as any.
}

nsRect nsOutlinerBodyFrame::GetInnerBox()
{
  nsRect r(0,0,mRect.width, mRect.height);
  nsMargin m(0,0,0,0);
  nsStyleBorderPadding  bPad;
  mStyleContext->GetStyle(eStyleStruct_BorderPaddingShortcut, (nsStyleStruct&)bPad);
  bPad.GetBorderPadding(m);
  r.Deflate(m);
  return r;
}

static void 
AdjustForBorderPadding(nsIStyleContext* aContext, nsRect& aRect)
{
  nsMargin m(0,0,0,0);
  nsStyleBorderPadding  bPad;
  aContext->GetStyle(eStyleStruct_BorderPaddingShortcut, (nsStyleStruct&)bPad);
  bPad.GetBorderPadding(m);
  aRect.Deflate(m);
}

// Painting routines
NS_IMETHODIMP nsOutlinerBodyFrame::Paint(nsIPresContext*      aPresContext,
                                         nsIRenderingContext& aRenderingContext,
                                         const nsRect&        aDirtyRect,
                                         nsFramePaintLayer    aWhichLayer)
{
  // XXX This trap handles an odd bogus 1 pixel invalidation that we keep getting 
  // when scrolling.
  if (aDirtyRect.width == 1)
    return NS_OK;

  const nsStyleDisplay* disp = (const nsStyleDisplay*)
      mStyleContext->GetStyleData(eStyleStruct_Display);
  if (!disp->IsVisibleOrCollapsed())
    return NS_OK; // We're invisible.  Don't paint.

  // Handles painting our background, border, and outline.
  nsresult rv = nsLeafFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  if (NS_FAILED(rv)) return rv;

  if (!mView)
    return NS_OK;

  PRBool clipState = PR_FALSE;
  
  // Update our page count, our available height and our row height.
  PRInt32 oldRowHeight = mRowHeight;
  mRowHeight = GetRowHeight();
  mIndentation = GetIndentation();
  mInnerBox = GetInnerBox();
  mPageCount = mInnerBox.height/mRowHeight;

  if (mRowHeight != oldRowHeight)
    InvalidateScrollbar();

  PRInt32 rowCount = 0;
  mView->GetRowCount(&rowCount);
  
  // Ensure our column info is built.
  EnsureColumns();

  // Loop through our columns and paint them (e.g., for sorting).  This is only
  // relevant when painting backgrounds, since columns contain no content.  Content
  // is contained in the rows.
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    nscoord currX = mInnerBox.x;
    for (nsOutlinerColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
         currCol = currCol->GetNext()) {
      nsRect colRect(currX, mInnerBox.y, currCol->GetWidth(), mInnerBox.height);
      PRInt32 overflow = colRect.x+colRect.width-(mInnerBox.x+mInnerBox.width);
      if (overflow > 0)
        colRect.width -= overflow;
      nsRect dirtyRect;
      if (dirtyRect.IntersectRect(aDirtyRect, colRect)) {
        PaintColumn(currCol, colRect, aPresContext, aRenderingContext, aDirtyRect, aWhichLayer); 
      }
      currX += currCol->GetWidth();
    }
  }

  // Loop through our on-screen rows.
  for (PRInt32 i = mTopRowIndex; i < rowCount && i < mTopRowIndex+mPageCount+1; i++) {
    nsRect rowRect(mInnerBox.x, mInnerBox.y+mRowHeight*(i-mTopRowIndex), mInnerBox.width, mRowHeight);
    nsRect dirtyRect;
    if (dirtyRect.IntersectRect(aDirtyRect, rowRect) && rowRect.y < (mInnerBox.y+mInnerBox.height)) {
      PRBool clip = (rowRect.y + rowRect.height > mInnerBox.y + mInnerBox.height);
      if (clip) {
        // We need to clip the last row, since it extends outside our inner box.  Push
        // a clip rect down.
        PRInt32 overflow = (rowRect.y+rowRect.height) - (mInnerBox.y+mInnerBox.height);
        nsRect clipRect(rowRect.x, rowRect.y, mInnerBox.width, mRowHeight-overflow);
        aRenderingContext.PushState();
        aRenderingContext.SetClipRect(clipRect, nsClipCombine_kReplace, clipState);
      }

      PaintRow(i, rowRect, aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

      if (clip)
        aRenderingContext.PopState(clipState);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::PaintColumn(nsOutlinerColumn*    aColumn,
                                               const nsRect& aColRect,
                                               nsIPresContext*      aPresContext,
                                               nsIRenderingContext& aRenderingContext,
                                               const nsRect&        aDirtyRect,
                                               nsFramePaintLayer    aWhichLayer)
{
  // Now obtain the properties for our cell.
  // XXX Automatically fill in the following props: open, container, selected, focused, and the col ID.
  PrefillPropertyArray(-1, aColumn->GetID());
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(aColumn->GetElement()));
  mView->GetColumnProperties(aColumn->GetID(), elt, mScratchArray);

  // Resolve style for the column.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsCOMPtr<nsIStyleContext> colContext;
  GetPseudoStyleContext(aPresContext, nsXULAtoms::mozoutlinercolumn, getter_AddRefs(colContext));

  // Obtain the margins for the cell and then deflate our rect by that 
  // amount.  The cell is assumed to be contained within the deflated rect.
  nsRect colRect(aColRect);
  const nsStyleMargin* colMarginData = (const nsStyleMargin*)colContext->GetStyleData(eStyleStruct_Margin);
  nsMargin colMargin;
  colMarginData->GetMargin(colMargin);
  colRect.Deflate(colMargin);

  PaintBackgroundLayer(colContext, aPresContext, aRenderingContext, colRect, aDirtyRect);

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::PaintRow(int aRowIndex, const nsRect& aRowRect,
                                            nsIPresContext*      aPresContext,
                                            nsIRenderingContext& aRenderingContext,
                                            const nsRect&        aDirtyRect,
                                            nsFramePaintLayer    aWhichLayer)
{
  // We have been given a rect for our row.  We treat this row like a full-blown
  // frame, meaning that it can have borders, margins, padding, and a background.
  
  // Without a view, we have no data. Check for this up front.
  if (!mView)
    return NS_OK;

  // Now obtain the properties for our row.
  // XXX Automatically fill in the following props: open, container, selected, focused
  PrefillPropertyArray(aRowIndex, NS_LITERAL_STRING(""));
  mView->GetRowProperties(aRowIndex, mScratchArray);

  // Resolve style for the row.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsCOMPtr<nsIStyleContext> rowContext;
  GetPseudoStyleContext(aPresContext, nsXULAtoms::mozoutlinerrow, getter_AddRefs(rowContext));

  // Obtain the margins for the row and then deflate our rect by that 
  // amount.  The row is assumed to be contained within the deflated rect.
  nsRect rowRect(aRowRect);
  const nsStyleMargin* rowMarginData = (const nsStyleMargin*)rowContext->GetStyleData(eStyleStruct_Margin);
  nsMargin rowMargin;
  rowMarginData->GetMargin(rowMargin);
  rowRect.Deflate(rowMargin);

  // If the layer is the background layer, we must paint our borders and background for our
  // row rect.
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)
    PaintBackgroundLayer(rowContext, aPresContext, aRenderingContext, rowRect, aDirtyRect);

  // Adjust the rect for its border and padding.
  AdjustForBorderPadding(rowContext, rowRect);

  // Now loop over our cells. Only paint a cell if it intersects with our dirty rect.
  nscoord currX = rowRect.x;
  for (nsOutlinerColumn* currCol = mColumns; currCol && currX < mInnerBox.x+mInnerBox.width; 
       currCol = currCol->GetNext()) {
    nsRect cellRect(currX, rowRect.y, currCol->GetWidth(), rowRect.height);
    PRInt32 overflow = cellRect.x+cellRect.width-(mInnerBox.x+mInnerBox.width);
    if (overflow > 0)
      cellRect.width -= overflow;
    nsRect dirtyRect;
    if (dirtyRect.IntersectRect(aDirtyRect, cellRect)) {
      PaintCell(aRowIndex, currCol, cellRect, aPresContext, aRenderingContext, aDirtyRect, aWhichLayer); 
    }
    currX += currCol->GetWidth();
  }

  return NS_OK;
}
  
NS_IMETHODIMP nsOutlinerBodyFrame::PaintCell(int aRowIndex, 
                                             nsOutlinerColumn*    aColumn,
                                             const nsRect& aCellRect,
                                             nsIPresContext*      aPresContext,
                                             nsIRenderingContext& aRenderingContext,
                                             const nsRect&        aDirtyRect,
                                             nsFramePaintLayer    aWhichLayer)
{
  // Now obtain the properties for our cell.
  // XXX Automatically fill in the following props: open, container, selected, focused, and the col ID.
  PrefillPropertyArray(aRowIndex, NS_LITERAL_STRING(""));
  mView->GetCellProperties(aRowIndex, aColumn->GetID(), mScratchArray);

  // Resolve style for the cell.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsCOMPtr<nsIStyleContext> cellContext;
  GetPseudoStyleContext(aPresContext, nsXULAtoms::mozoutlinercell, getter_AddRefs(cellContext));

  // Obtain the margins for the cell and then deflate our rect by that 
  // amount.  The cell is assumed to be contained within the deflated rect.
  nsRect cellRect(aCellRect);
  const nsStyleMargin* cellMarginData = (const nsStyleMargin*)cellContext->GetStyleData(eStyleStruct_Margin);
  nsMargin cellMargin;
  cellMarginData->GetMargin(cellMargin);
  cellRect.Deflate(cellMargin);

  // If the layer is the background layer, we must paint our borders and background for our
  // row rect.
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)
    PaintBackgroundLayer(cellContext, aPresContext, aRenderingContext, cellRect, aDirtyRect);

  // Adjust the rect for its border and padding.
  AdjustForBorderPadding(cellContext, cellRect);

  nscoord currX = cellRect.x;
  nscoord remainingWidth = cellRect.width;

  // Now we paint the contents of the cells.
  // Text alignment determines the order in which we paint.  
  // LEFT means paint from left to right.
  // RIGHT means paint from right to left.
  // XXX Implement RIGHT alignment!

  if (aColumn->IsPrimary()) {
    // If we're the primary column, we need to indent and paint the twisty.
    // First we indent
    PRInt32 level;
    mView->GetLevel(aRowIndex, &level);

    currX += mIndentation*level;
    remainingWidth -= mIndentation*level;

    // XXX Paint the twisty.
  }

  // XXX Now paint the various images.

  // Now paint our text.
  nsRect textRect(currX, cellRect.y, remainingWidth, cellRect.height);
  nsRect dirtyRect;
  if (dirtyRect.IntersectRect(aDirtyRect, textRect))
    PaintText(aRowIndex, aColumn, textRect, aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::PaintText(int aRowIndex, 
                                             nsOutlinerColumn*    aColumn,
                                             const nsRect& aTextRect,
                                             nsIPresContext*      aPresContext,
                                             nsIRenderingContext& aRenderingContext,
                                             const nsRect&        aDirtyRect,
                                             nsFramePaintLayer    aWhichLayer)
{
  // Now obtain the text for our cell.
  nsXPIDLString text;
  mView->GetCellText(aRowIndex, aColumn->GetID(), getter_Copies(text));

  nsAutoString realText(text);

  // Resolve style for the text.  It contains all the info we need to lay ourselves
  // out and to paint.
  nsCOMPtr<nsIStyleContext> textContext;
  GetPseudoStyleContext(aPresContext, nsXULAtoms::mozoutlinercelltext, getter_AddRefs(textContext));

  // Obtain the margins for the text and then deflate our rect by that 
  // amount.  The text is assumed to be contained within the deflated rect.
  nsRect textRect(aTextRect);
  const nsStyleMargin* textMarginData = (const nsStyleMargin*)textContext->GetStyleData(eStyleStruct_Margin);
  nsMargin textMargin;
  textMarginData->GetMargin(textMargin);
  textRect.Deflate(textMargin);

  // If the layer is the background layer, we must paint our borders and background for our
  // text rect.
  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer)
    PaintBackgroundLayer(textContext, aPresContext, aRenderingContext, textRect, aDirtyRect);
  else {
    // Time to paint our text. 
    // Adjust the rect for its border and padding.
    AdjustForBorderPadding(textContext, textRect);
    
    // Compute our text size.
    const nsStyleFont* fontStyle = (const nsStyleFont*)textContext->GetStyleData(eStyleStruct_Font);

    nsCOMPtr<nsIDeviceContext> deviceContext;
    aPresContext->GetDeviceContext(getter_AddRefs(deviceContext));

    nsCOMPtr<nsIFontMetrics> fontMet;
    deviceContext->GetMetricsFor(fontStyle->mFont, *getter_AddRefs(fontMet));
    nscoord height;
    fontMet->GetHeight(height);

    // Center the text. XXX Obey vertical-align style prop?
    if (height < textRect.height) {
      textRect.y += (textRect.height - height)/2;
      textRect.height = height;
    }

    // Set our font.
    aRenderingContext.SetFont(fontMet);

    nscoord width;
    aRenderingContext.GetWidth(realText, width);

    if (width > textRect.width) {
      // See if the width is even smaller than the ellipsis
      // If so, clear the text completely.
      nscoord ellipsisWidth;
      aRenderingContext.GetWidth(ELLIPSIS, ellipsisWidth);

      nsAutoString ellipsis; ellipsis.AssignWithConversion(ELLIPSIS);

      nscoord width = textRect.width;
      if (ellipsisWidth > width)
        realText.SetLength(0);
      else if (ellipsisWidth == width)
        realText = ellipsis;
      else {
        // We will be drawing an ellipsis, thank you very much.
        // Subtract out the required width of the ellipsis.
        // This is the total remaining width we have to play with.
        width -= ellipsisWidth;

        // Now we crop.
        switch (aColumn->GetCropStyle()) {
          default:
          case 0: {
            // Crop right. 
            nscoord cwidth;
            nscoord twidth = 0;
            int length = realText.Length();
            int i;
            for (i = 0; i < length; ++i) {
              PRUnichar ch = realText.CharAt(i);
              aRenderingContext.GetWidth(ch,cwidth);
              if (twidth + cwidth > width)
                break;
              twidth += cwidth;
            }

            realText.Truncate(i);
            realText += ellipsis;
          }
          break;

          case 2: {
            // Crop left.
            nscoord cwidth;
            nscoord twidth = 0;
            int length = realText.Length();
            int i;
            for (i=length-1; i >= 0; --i) {
              PRUnichar ch = realText.CharAt(i);
              aRenderingContext.GetWidth(ch,cwidth);
              if (twidth + cwidth > width)
                  break;

              twidth += cwidth;
            }

            nsAutoString copy;
            realText.Right(copy, length-1-i);
            realText = ellipsis;
            realText += copy;
          }
          break;

          case 1:
          {
            // XXX Not yet implemented.
          }
          break;
        }
      }
    }

    // Set our color.
    const nsStyleColor* colorStyle = (const nsStyleColor*)textContext->GetStyleData(eStyleStruct_Color);
    aRenderingContext.SetColor(colorStyle->mColor);

    aRenderingContext.DrawString(realText, textRect.x, textRect.y);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsOutlinerBodyFrame::PaintBackgroundLayer(nsIStyleContext* aStyleContext, nsIPresContext* aPresContext, 
                                          nsIRenderingContext& aRenderingContext, 
                                          const nsRect& aRect, const nsRect& aDirtyRect)
{

  const nsStyleDisplay* disp = (const nsStyleDisplay*)
      aStyleContext->GetStyleData(eStyleStruct_Display);
  const nsStyleColor* myColor = (const nsStyleColor*)
      aStyleContext->GetStyleData(eStyleStruct_Color);
  const nsStyleBorder* myBorder = (const nsStyleBorder*)
      aStyleContext->GetStyleData(eStyleStruct_Border);
  const nsStyleOutline* myOutline = (const nsStyleOutline*)
      aStyleContext->GetStyleData(eStyleStruct_Outline);
  
  nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                  aDirtyRect, aRect, *myColor, *myBorder, 0, 0);

  nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                              aDirtyRect, aRect, *myBorder, mStyleContext, 0);

  nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
                              aDirtyRect, aRect, *myBorder, *myOutline, aStyleContext, 0);

  return NS_OK;
}

// Scrolling
NS_IMETHODIMP nsOutlinerBodyFrame::EnsureRowIsVisible(PRInt32 aRow)
{
  if (!mView)
    return NS_OK;

  if (mTopRowIndex <= aRow && mTopRowIndex+mPageCount > aRow)
    return NS_OK;

  if (aRow < mTopRowIndex)
    ScrollToRow(aRow);
  else {
    // Bring it just on-screen.
    PRInt32 distance = aRow - (mTopRowIndex+mPageCount)+1;
    ScrollToRow(mTopRowIndex+distance);
  }

  return NS_OK;
}

NS_IMETHODIMP nsOutlinerBodyFrame::ScrollToRow(PRInt32 aRow)
{
  return ScrollInternal(aRow, PR_TRUE);
}

nsresult
nsOutlinerBodyFrame::ScrollInternal(PRInt32 aRow, PRBool aUpdateScrollbar)
{
  if (!mView)
    return NS_OK;

  PRInt32 rowCount;
  mView->GetRowCount(&rowCount);

  PRInt32 delta = aRow - mTopRowIndex;

  if (delta > 0) {
    if (mTopRowIndex == (rowCount - mPageCount + 1))
      return NS_OK;
  }
  else {
    if (mTopRowIndex == 0)
      return NS_OK;
  }

  mTopRowIndex += delta;

  float t2p;
  mPresContext->GetTwipsToPixels(&t2p);
  nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

  PRInt32 absDelta = delta > 0 ? delta : -delta;
  if (absDelta*mRowHeight >= mRect.height)
    Invalidate();
  else if (mOutlinerWidget)
    mOutlinerWidget->Scroll(0, -delta*rowHeightAsPixels, nsnull);
 
  if (aUpdateScrollbar) {
    // Update the scrollbar.
    nsCOMPtr<nsIContent> scrollbarContent;
    mScrollbar->GetContent(getter_AddRefs(scrollbarContent));
    float t2p;
    mPresContext->GetTwipsToPixels(&t2p);
    nscoord rowHeightAsPixels = NSToCoordRound((float)mRowHeight*t2p);

    nsAutoString curPos;
    curPos.AppendInt(mTopRowIndex*rowHeightAsPixels);
    scrollbarContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::curpos, curPos, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsOutlinerBodyFrame::ScrollbarButtonPressed(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  if (aNewIndex > aOldIndex)
    ScrollToRow(mTopRowIndex+1);
  else ScrollToRow(mTopRowIndex-1);
  return NS_OK;
}
  
NS_IMETHODIMP
nsOutlinerBodyFrame::PositionChanged(PRInt32 aOldIndex, PRInt32& aNewIndex)
{
  float t2p;
  mPresContext->GetTwipsToPixels(&t2p);
  nscoord rh = NSToCoordRound((float)mRowHeight*t2p);

  nscoord oldrow = aOldIndex/rh;
  nscoord newrow = aNewIndex/rh;

  if (oldrow != newrow)
    ScrollInternal(newrow, PR_FALSE);

  // Go exactly where we're supposed to
  // Update the scrollbar.
  nsCOMPtr<nsIContent> scrollbarContent;
  mScrollbar->GetContent(getter_AddRefs(scrollbarContent));
  nsAutoString curPos;
  curPos.AppendInt(aNewIndex);
  scrollbarContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::curpos, curPos, PR_TRUE);

  return NS_OK;
}

// The style cache.
nsresult 
nsOutlinerBodyFrame::GetPseudoStyleContext(nsIPresContext* aPresContext, nsIAtom* aPseudoElement, 
                                           nsIStyleContext** aResult)
{
  return mStyleCache.GetStyleContext(this, aPresContext, mContent, mStyleContext, aPseudoElement,
                                     mScratchArray, aResult);
}

// Our comparator for resolving our complex pseudos
NS_IMETHODIMP
nsOutlinerBodyFrame::PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult)
{
  if (aSelector->mTag == aTag) {
    // Iterate the pseudoclass list.  For each item in the list, see if
    // it is contained in our scratch array.  If we have a miss, then
    // we aren't a match.  If all items in the pseudoclass list are
    // present in the scratch array, then we have a match.
    nsAtomList* curr = aSelector->mPseudoClassList;
    while (curr) {
      PRInt32 index;
      mScratchArray->GetIndexOf(curr->mAtom, &index);
      if (index == -1) {
        *aResult = PR_FALSE;
        return NS_OK;
      }
      curr = curr->mNext;
    }
    *aResult = PR_TRUE;
  }
  else 
    *aResult = PR_FALSE;

  return NS_OK;
}

void
nsOutlinerBodyFrame::EnsureColumns()
{
  if (!mColumns) {
    nsCOMPtr<nsIContent> parent;
    mContent->GetParent(*getter_AddRefs(parent));
    nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(parent));

    nsCOMPtr<nsIDOMNodeList> cols;
    elt->GetElementsByTagName(NS_LITERAL_STRING("outlinercol"), getter_AddRefs(cols));

    nsCOMPtr<nsIPresShell> shell; 
    mPresContext->GetShell(getter_AddRefs(shell));

    PRUint32 count;
    cols->GetLength(&count);

    nsOutlinerColumn* currCol = nsnull;
    for (PRUint32 i = 0; i < count; i++) {
      nsCOMPtr<nsIDOMNode> node;
      cols->Item(i, getter_AddRefs(node));
      nsCOMPtr<nsIContent> child(do_QueryInterface(node));
      
      // Get the frame for this column.
      nsIFrame* frame;
      shell->GetPrimaryFrameFor(child, &frame);
      
      // Create a new column structure.
      nsOutlinerColumn* col = new nsOutlinerColumn(child, frame);
      if (currCol)
        currCol->SetNext(col);
      else mColumns = col;
      currCol = col;
    }
  }
}

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsOutlinerBodyFrame)
  NS_INTERFACE_MAP_ENTRY(nsIOutlinerBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsICSSPseudoComparator)
  NS_INTERFACE_MAP_ENTRY(nsIScrollbarMediator)
NS_INTERFACE_MAP_END_INHERITING(nsLeafFrame)

