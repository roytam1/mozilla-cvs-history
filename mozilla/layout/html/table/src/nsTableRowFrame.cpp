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
#include "nsTableRowFrame.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIHTMLAttributes.h"
#include "nsHTMLAtoms.h"
#include "nsIContent.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsIView.h"
#include "nsIPtr.h"
#include "nsIReflowCommand.h"
#include "nsCSSRendering.h"
#include "nsHTMLIIDs.h"
// the following header files are required for style optimizations that work only when the child content is really a cell
#include "nsIHTMLTableCellElement.h"
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);
// end includes for style optimizations that require real content knowledge

NS_DEF_PTR(nsIStyleContext);

#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
//#define NOISY
//#define NOISY_FLOW
#else
static const PRBool gsDebug = PR_FALSE;
#endif

/* ----------- RowReflowState ---------- */

struct RowReflowState {
  // Our reflow state
  const nsHTMLReflowState& reflowState;

  // The body's available size (computed from the body's parent)
  nsSize availSize;

  // the running x-offset
  nscoord x;

  // Height of tallest cell (excluding cells with rowspan > 1)
  nscoord maxCellHeight;    // just the height of the cell frame
  nscoord maxCellVertSpace; // the maximum MAX(cellheight + topMargin + bottomMargin)
  
  nsTableFrame *tableFrame;
   

  RowReflowState(const nsHTMLReflowState& aReflowState,
                 nsTableFrame*            aTableFrame)
    : reflowState(aReflowState)
  {
    availSize.width = reflowState.maxSize.width;
    availSize.height = reflowState.maxSize.height;
    maxCellHeight = 0;
    maxCellVertSpace = 0;
    tableFrame = aTableFrame;
    x=0;
  }
};




/* ----------- nsTableRowpFrame ---------- */

nsTableRowFrame::nsTableRowFrame(nsIContent* aContent,
                                 nsIFrame*   aParentFrame)
  : nsContainerFrame(aContent, aParentFrame),
    mTallestCell(0),
    mCellMaxTopMargin(0),
    mCellMaxBottomMargin(0),
    mMinRowSpan(1)
{
}

nsTableRowFrame::~nsTableRowFrame()
{
}

NS_IMETHODIMP
nsTableRowFrame::Init(nsIPresContext& aPresContext, nsIFrame* aChildList)
{
  mFirstChild = aChildList;
  nsTableFrame* table = nsnull;
  nsresult  result;

  result = nsTableFrame::GetTableFrame(this, table);
  if ((NS_OK==result) && (table != nsnull))
  {
    SetRowIndex(table->GetNextAvailRowIndex());
    PRInt32   colIndex = 0;
    for (nsIFrame* kidFrame = mFirstChild; nsnull != kidFrame; kidFrame->GetNextSibling(kidFrame)) 
    {
      const nsStyleDisplay *kidDisplay;
      kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
      if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
      {
        // what column does this cell belong to?
        colIndex = table->GetNextAvailColIndex(mRowIndex, colIndex);
        /* for style context optimization, set the content's column index if possible.
         * this can only be done if we really have an nsTableCell.  
         * other tags mapped to table cell display won't benefit from this optimization
         * see nsHTMLStyleSheet::RulesMatching
         */
        nsIContent* cell;
        kidFrame->GetContent(cell);
        nsIHTMLTableCellElement *cellContent = nsnull;
        nsresult rv = cell->QueryInterface(kIHTMLTableCellElementIID, 
                                           (void **)&cellContent);  // cellContent: REFCNT++
        NS_RELEASE(cell);
        if (NS_SUCCEEDED(rv))
        { // we know it's a table cell
          cellContent->SetColIndex(colIndex);
          if (gsDebug) printf("%p : set cell content %p to col index = %d\n", this, cellContent, colIndex);
          NS_RELEASE(cellContent);
        }
      
        // this sets the frame's notion of it's column index
        ((nsTableCellFrame *)kidFrame)->InitCellFrame(colIndex);
        if (gsDebug) printf("%p : set cell frame %p to col index = %d\n", this, kidFrame, colIndex);
        // add the cell frame to the table's cell map
        table->AddCellToTable(this, (nsTableCellFrame *)kidFrame, kidFrame == mFirstChild);
      }
    }
  }
  return NS_OK;
}

/**
 * Post-reflow hook. This is where the table row does its post-processing
 */
NS_METHOD
nsTableRowFrame::DidReflow(nsIPresContext& aPresContext,
                           nsDidReflowStatus aStatus)
{
  if (NS_FRAME_REFLOW_FINISHED == aStatus) {
    // Resize and re-align the cell frames based on our row height
    nscoord           cellHeight = mRect.height - mCellMaxTopMargin - mCellMaxBottomMargin;
    nsIFrame *cellFrame = mFirstChild;
    nsTableFrame* tableFrame;
    nsTableFrame::GetTableFrame(this, tableFrame);
    while (nsnull != cellFrame)
    {
      const nsStyleDisplay *kidDisplay;
      cellFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
      if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
      {
        PRInt32 rowSpan = tableFrame->GetEffectiveRowSpan(mRowIndex, (nsTableCellFrame *)cellFrame);
        if (1==rowSpan)
        {
          // resize the cell's height
          nsSize  cellFrameSize;
          cellFrame->GetSize(cellFrameSize);
          cellFrame->SizeTo(cellFrameSize.width, cellHeight);
  
          // realign cell content based on the new height
          ((nsTableCellFrame *)cellFrame)->VerticallyAlignChild(&aPresContext);
        }
      }
        // Get the next cell
      cellFrame->GetNextSibling(cellFrame);
    }
  }

  // Let our base class do the usual work
  return nsContainerFrame::DidReflow(aPresContext, aStatus);
}

void nsTableRowFrame::ResetMaxChildHeight()
{
  mTallestCell=0;
  mCellMaxTopMargin=0;
  mCellMaxBottomMargin=0;
}

void nsTableRowFrame::SetMaxChildHeight(nscoord aChildHeight, nscoord aTopMargin, nscoord aBottomMargin)
{
  if (mTallestCell<aChildHeight)
    mTallestCell = aChildHeight;

  if (mCellMaxTopMargin<aTopMargin)
    mCellMaxTopMargin = aTopMargin;

  if (mCellMaxBottomMargin<aBottomMargin)
    mCellMaxBottomMargin = aBottomMargin;
}

NS_METHOD nsTableRowFrame::Paint(nsIPresContext& aPresContext,
                                 nsIRenderingContext& aRenderingContext,
                                 const nsRect& aDirtyRect)
{
  /*
  const nsStyleColor* myColor =
    (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
  if (nsnull != myColor) {
    nsRect  rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *myColor);
  }
  */

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect);
  return NS_OK;
}

/** overloaded method from nsContainerFrame.  The difference is that 
  * we don't want to clip our children, so a cell can do a rowspan
  */
void nsTableRowFrame::PaintChildren(nsIPresContext&      aPresContext,
                                     nsIRenderingContext& aRenderingContext,
                                     const nsRect&        aDirtyRect)
{
  nsIFrame* kid = mFirstChild;
  while (nsnull != kid) {
    nsIView *pView;
     
    kid->GetView(pView);
    if (nsnull == pView) {
      nsRect kidRect;
      kid->GetRect(kidRect);
      nsRect damageArea;
      PRBool overlap = damageArea.IntersectRect(aDirtyRect, kidRect);
      if (overlap) {
        // Translate damage area into kid's coordinate system
        nsRect kidDamageArea(damageArea.x - kidRect.x,
                             damageArea.y - kidRect.y,
                             damageArea.width, damageArea.height);
        aRenderingContext.PushState();
        aRenderingContext.Translate(kidRect.x, kidRect.y);
        kid->Paint(aPresContext, aRenderingContext, kidDamageArea);
        if (nsIFrame::GetShowFrameBorders()) {
          aRenderingContext.SetColor(NS_RGB(255,0,0));
          aRenderingContext.DrawRect(0, 0, kidRect.width, kidRect.height);
        }
        aRenderingContext.PopState();
      }
    }
    kid->GetNextSibling(kid);
  }
}

void nsTableRowFrame::SetRowIndex (int aRowIndex)
{
  mRowIndex = aRowIndex;
}

/** returns the height of the tallest child in this row (ignoring any cell with rowspans) */
nscoord nsTableRowFrame::GetTallestChild() const
{
  return mTallestCell;
}

nscoord nsTableRowFrame::GetChildMaxTopMargin() const
{
  return mCellMaxTopMargin;
}

nscoord nsTableRowFrame::GetChildMaxBottomMargin() const
{
  return mCellMaxBottomMargin;
}

PRInt32 nsTableRowFrame::GetMaxColumns() const
{
  int sum = 0;
  nsIFrame *cell=mFirstChild;
  while (nsnull!=cell) 
  {
    const nsStyleDisplay *kidDisplay;
    cell->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      sum += ((nsTableCellFrame *)cell)->GetColSpan();
    }
    cell->GetNextSibling(cell);
  }
  return sum;
}

/* GetMinRowSpan is needed for deviant cases where every cell in a row has a rowspan > 1.
 * It sets mMinRowSpan, which is used in FixMinCellHeight and PlaceChild
 */
void nsTableRowFrame::GetMinRowSpan(nsTableFrame *aTableFrame)
{
  PRInt32 minRowSpan=-1;
  nsIFrame *frame=mFirstChild;
  while (nsnull!=frame)
  {
    const nsStyleDisplay *kidDisplay;
    frame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      PRInt32 rowSpan = aTableFrame->GetEffectiveRowSpan(mRowIndex, ((nsTableCellFrame *)frame));
      if (-1==minRowSpan)
        minRowSpan = rowSpan;
      else if (minRowSpan>rowSpan)
        minRowSpan = rowSpan;
    }
    frame->GetNextSibling(frame);
  }
  mMinRowSpan = minRowSpan;
}

void nsTableRowFrame::FixMinCellHeight(nsTableFrame *aTableFrame)
{
  nsIFrame *frame=mFirstChild;
  while (nsnull!=frame)
  {
    const nsStyleDisplay *kidDisplay;
    frame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      PRInt32 rowSpan = aTableFrame->GetEffectiveRowSpan(mRowIndex, ((nsTableCellFrame *)frame));
      if (mMinRowSpan==rowSpan)
      {
        nsRect rect;
        frame->GetRect(rect);
        if (rect.height > mTallestCell)
          mTallestCell = rect.height;
      }
    }
    frame->GetNextSibling(frame);
  }
}

// Position and size aKidFrame and update our reflow state. The origin of
// aKidRect is relative to the upper-left origin of our frame, and includes
// any left/top margin.
void nsTableRowFrame::PlaceChild(nsIPresContext&    aPresContext,
																 RowReflowState&    aState,
																 nsIFrame*          aKidFrame,
																 const nsRect&      aKidRect,
																 nsSize*            aMaxElementSize,
																 nsSize*            aKidMaxElementSize)
{
  if (PR_TRUE==gsDebug)
    printf ("row: placing cell at %d, %d, %d, %d\n",
           aKidRect.x, aKidRect.y, aKidRect.width, aKidRect.height);

  // Place and size the child
  aKidFrame->SetRect(aKidRect);

  // update the running total for the row width
  aState.x += aKidRect.width;

  // Update the maximum element size
  PRInt32 rowSpan = aState.tableFrame->GetEffectiveRowSpan(mRowIndex, 
                    ((nsTableCellFrame*)aKidFrame));
  if (nsnull != aMaxElementSize) 
  {
    aMaxElementSize->width += aKidMaxElementSize->width;
    if ((mMinRowSpan==rowSpan) && (aKidMaxElementSize->height>aMaxElementSize->height))
    {
      aMaxElementSize->height = aKidMaxElementSize->height;
    }
  }

  if (mMinRowSpan == rowSpan)
  {
    // Update maxCellHeight
    if (aKidRect.height > aState.maxCellHeight)
      aState.maxCellHeight = aKidRect.height;

    // Update maxCellVertSpace
    nsMargin margin;

    if (aState.tableFrame->GetCellMarginData((nsTableCellFrame *)aKidFrame, margin) == NS_OK)
    {
      nscoord height = aKidRect.height + margin.top + margin.bottom;
  
      if (height > aState.maxCellVertSpace)
        aState.maxCellVertSpace = height;
    }
  }
}

/**
 * Called for a resize reflow. Typically because the column widths have
 * changed. Reflows all the existing table cell frames
 */
nsresult nsTableRowFrame::ResizeReflow(nsIPresContext&  aPresContext,
                                       RowReflowState&  aState,
                                       nsHTMLReflowMetrics& aDesiredSize)
{
  if (nsnull == mFirstChild)
    return NS_OK;

  nsSize      kidMaxElementSize;
  PRInt32     prevColIndex = -1;       // remember the col index of the previous cell to handle rowspans into this row
  nsSize*     pKidMaxElementSize = (nsnull != aDesiredSize.maxElementSize) ?
                                     &kidMaxElementSize : nsnull;
  nscoord     maxCellTopMargin = 0;
  nscoord     maxCellBottomMargin = 0;
  nscoord cellSpacing = aState.tableFrame->GetCellSpacing();
  PRInt32 cellColSpan=1;  // must be defined here so it's set properly for non-cell kids
  if (PR_TRUE==gsDebug) printf("%p: RR\n", this);
  // Reflow each of our existing cell frames
  for (nsIFrame*  kidFrame = mFirstChild; nsnull != kidFrame; ) 
  {
    const nsStyleDisplay *kidDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      nsMargin kidMargin;
      aState.tableFrame->GetCellMarginData((nsTableCellFrame *)kidFrame,kidMargin);
      if (kidMargin.top > maxCellTopMargin)
        maxCellTopMargin = kidMargin.top;
      if (kidMargin.bottom > maxCellBottomMargin)
        maxCellBottomMargin = kidMargin.bottom;
 
      // Compute the x-origin for the child, taking into account straddlers (cells from prior
      // rows with rowspans > 1)
      PRInt32 cellColIndex = ((nsTableCellFrame *)kidFrame)->GetColIndex();
      if (prevColIndex != (cellColIndex-1))
      { // if this cell is not immediately adjacent to the previous cell, factor in missing col info
        for (PRInt32 colIndex=prevColIndex+1; colIndex<cellColIndex; colIndex++)
        {
          aState.x += aState.tableFrame->GetColumnWidth(colIndex);
          aState.x += cellSpacing;
          if (PR_TRUE==gsDebug)
            printf("  in loop, aState.x set to %d from cellSpacing %d and col width\n", 
                    aState.x, aState.tableFrame->GetColumnWidth(colIndex), cellSpacing);
        }
      }
      aState.x += cellSpacing;
      if (PR_TRUE==gsDebug) printf("  past loop, aState.x set to %d\n", aState.x);

      // at this point, we know the column widths.  
      // so we get the avail width from the known column widths
      cellColSpan = aState.tableFrame->GetEffectiveColSpan(((nsTableCellFrame *)kidFrame)->GetColIndex(),
                                                                   ((nsTableCellFrame *)kidFrame));
      nscoord availWidth = 0;
      for (PRInt32 numColSpan=0; numColSpan<cellColSpan; numColSpan++)
      {
        availWidth += aState.tableFrame->GetColumnWidth(cellColIndex+numColSpan);
        if (numColSpan != 0)
        {
          availWidth += cellSpacing;
        }
        if (PR_TRUE==gsDebug) 
          printf("  in loop, availWidth set to %d from colIndex %d width %d and cellSpacing\n", 
                  availWidth, cellColIndex, aState.tableFrame->GetColumnWidth(cellColIndex+numColSpan), cellSpacing);
      }
      if (PR_TRUE==gsDebug) printf("  availWidth for this cell is %d\n", availWidth);

      prevColIndex = cellColIndex + (cellColSpan-1);  // remember the rightmost column this cell spans into
      nsHTMLReflowMetrics desiredSize(pKidMaxElementSize);

      // If the available width is the same as last time we reflowed the cell,
      // then just use the previous desired size and max element size.
      // if we need the max-element-size we don't need to reflow.
      // we just grab it from the cell frame which remembers it (see the else clause below)
      if (availWidth != ((nsTableCellFrame *)kidFrame)->GetPriorAvailWidth())
      {
        // Always let the cell be as high as it wants. We ignore the height that's
        // passed in and always place the entire row. Let the row group decide
        // whether we fit or wehther the entire row is pushed
        nsSize  kidAvailSize(availWidth, NS_UNCONSTRAINEDSIZE);

        // Reflow the child
        nsHTMLReflowState kidReflowState(aPresContext, kidFrame,
                                         aState.reflowState, kidAvailSize,
                                         eReflowReason_Resize);
        if (gsDebug) printf ("%p RR: avail=%d\n", this, availWidth);
        nsReflowStatus status;
        ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, status);
        if (gsDebug) printf ("%p RR: desired=%d\n", this, desiredSize.width);
#ifdef NS_DEBUG
        if (desiredSize.width > availWidth)
        {
          printf("WARNING: cell returned desired width %d given avail width %d\n",
                  desiredSize.width, availWidth);
        }
#endif
        NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "unexpected reflow status");

        if (gsDebug)
        {
          if (nsnull!=pKidMaxElementSize)
            printf("reflow of cell returned result = %s with desired=%d,%d, min = %d,%d\n",
                    NS_FRAME_IS_COMPLETE(status)?"complete":"NOT complete", 
                    desiredSize.width, desiredSize.height, 
                    pKidMaxElementSize->width, pKidMaxElementSize->height);
          else
            printf("reflow of cell returned result = %s with desired=%d,%d, min = nsnull\n",
                    NS_FRAME_IS_COMPLETE(status)?"complete":"NOT complete", 
                    desiredSize.width, desiredSize.height);
        }
      }
      else
      {
        nsSize priorSize = ((nsTableCellFrame *)kidFrame)->GetDesiredSize();
        desiredSize.width = priorSize.width;
        desiredSize.height = priorSize.height;
        if (nsnull != pKidMaxElementSize) 
          *pKidMaxElementSize = ((nsTableCellFrame *)kidFrame)->GetPass1MaxElementSize();
      }

      // Place the child after taking into account its margin and attributes
      nscoord specifiedHeight = 0;
      nscoord cellHeight = desiredSize.height;
      nsIStyleContextPtr kidSC;
      kidFrame->GetStyleContext(&aPresContext, kidSC.AssignRef());
      const nsStylePosition* kidPosition = (const nsStylePosition*)
        kidSC->GetStyleData(eStyleStruct_Position);
      switch (kidPosition->mHeight.GetUnit()) {
      case eStyleUnit_Coord:
        specifiedHeight = kidPosition->mHeight.GetCoordValue();
        break;

      case eStyleUnit_Inherit:
        // XXX for now, do nothing
      default:
      case eStyleUnit_Auto:
        break;
      }
      if (specifiedHeight>cellHeight)
        cellHeight = specifiedHeight;

      nscoord cellWidth = desiredSize.width;
      // begin special Nav4 compatibility code
      if (0==cellWidth)
      {
        cellWidth = availWidth;
      }
      // end special Nav4 compatibility code

      // Place the child
      nsRect kidRect (aState.x, kidMargin.top, cellWidth, cellHeight);

      PlaceChild(aPresContext, aState, kidFrame, kidRect, aDesiredSize.maxElementSize,
                 pKidMaxElementSize);

      if (PR_TRUE==gsDebug) printf("  past PlaceChild, aState.x set to %d\n", aState.x);
    }

    // Get the next child
    kidFrame->GetNextSibling(kidFrame);
    // if this was the last child, and it had a colspan>1, add in the cellSpacing for the colspan
    // if the last kid wasn't a colspan, then we still have the colspan of the last real cell
    if ((nsnull==kidFrame) && (cellColSpan>1))
      aState.x += cellSpacing;
  }

  SetMaxChildHeight(aState.maxCellHeight,maxCellTopMargin, maxCellBottomMargin);  // remember height of tallest child who doesn't have a row span

  // Return our desired size. Note that our desired width is just whatever width
  // we were given by the row group frame
  aDesiredSize.width = aState.x;
  aDesiredSize.height = aState.maxCellVertSpace;  

  if (gsDebug)
    printf("rr -- row %p width = %d from maxSize %d\n", 
           this, aDesiredSize.width, aState.reflowState.maxSize.width);
  
  if (aDesiredSize.width > aState.reflowState.maxSize.width) 
  {
    printf ("%p error case, desired width = %d, maxSize=%d\n",
            this, aDesiredSize.width, aState.reflowState.maxSize.width);
    fflush (stdout);
  }
  NS_ASSERTION(aDesiredSize.width <= aState.reflowState.maxSize.width, "row calculated to be too wide.");
  return NS_OK;
}

/**
 * Called for the initial reflow. Creates each table cell frame, and
 * reflows it to gets its minimum and maximum sizes
 */
nsresult
nsTableRowFrame::InitialReflow(nsIPresContext&  aPresContext,
                               RowReflowState&  aState,
                               nsHTMLReflowMetrics& aDesiredSize)
{
  // Place our children, one at a time, until we are out of children
  nsSize    kidMaxElementSize(0,0);
  PRInt32   kidIndex = 0;
  PRInt32   colIndex = 0;
  nsIFrame* prevKidFrame = nsnull;
  nscoord   maxTopMargin = 0;
  nscoord   maxBottomMargin = 0;
  nscoord   x = 0;
  PRBool    isFirst=PR_TRUE;
  PRBool    tableLayoutStrategy=NS_STYLE_TABLE_LAYOUT_AUTO; 
  nsTableFrame* table = nsnull;
  nsresult  result = nsTableFrame::GetTableFrame(this, table);
  if ((NS_OK==result) && (table != nsnull))
  {
    nsStyleTable* tableStyle;
    table->GetStyleData(eStyleStruct_Table, (nsStyleStruct *&)tableStyle);
    tableLayoutStrategy = tableStyle->mLayoutStrategy;
  }
  else
    return NS_ERROR_UNEXPECTED;

  for (nsIFrame* kidFrame = mFirstChild; nsnull != kidFrame; kidFrame->GetNextSibling(kidFrame)) 
  {
    const nsStyleDisplay *kidDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      // Get the child's margins
      nsMargin  margin;
      nscoord   topMargin = 0;

      if (aState.tableFrame->GetCellMarginData((nsTableCellFrame *)kidFrame, margin) == NS_OK)
      {
        topMargin = margin.top;
      }
   
      maxTopMargin = PR_MAX(margin.top, maxTopMargin);
      maxBottomMargin = PR_MAX(margin.bottom, maxBottomMargin);

      // Because we're not splittable always allow the child to be as high as
      // it wants. The default available width is also unconstrained so we can
      // get the child's maximum width
      nsSize  kidAvailSize;
      nsHTMLReflowMetrics kidSize(nsnull);
      if (NS_STYLE_TABLE_LAYOUT_AUTO==tableLayoutStrategy)
      {
        kidAvailSize.SizeTo(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
        kidSize.maxElementSize=&kidMaxElementSize;
      }
      else
      {
        PRInt32 colIndex = ((nsTableCellFrame *)kidFrame)->GetColIndex();
        kidAvailSize.SizeTo(table->GetColumnWidth(colIndex), NS_UNCONSTRAINEDSIZE); 
      }

      nsHTMLReflowState kidReflowState(aPresContext, kidFrame,
                                       aState.reflowState, kidAvailSize,
                                       eReflowReason_Initial);

      if (gsDebug) printf ("%p InitR: avail=%d\n", this, kidAvailSize.width);
      nsresult status;
      ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, status);
      if (gsDebug) 
        printf ("TR %p for cell %p Initial Reflow: desired=%d, MES=%d\n", 
               this, kidFrame, kidSize.width, kidMaxElementSize.width);
      //XXX: this is a hack, shouldn't it be the case that a min size is 
      //     never larger than a desired size?
      if (kidMaxElementSize.width>kidSize.width)
        kidSize.width = kidMaxElementSize.width;
      if (kidMaxElementSize.height>kidSize.height)
        kidSize.height = kidMaxElementSize.height;
      ((nsTableCellFrame *)kidFrame)->SetPass1DesiredSize(kidSize);
      ((nsTableCellFrame *)kidFrame)->SetPass1MaxElementSize(kidMaxElementSize);
      NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "unexpected child reflow status");

      if (gsDebug) 
      {
        printf("reflow of cell returned result = %s with desired=%d,%d, min = %d,%d\n",
               NS_FRAME_IS_COMPLETE(status)?"complete":"NOT complete", 
               kidSize.width, kidSize.height, 
               kidMaxElementSize.width, kidMaxElementSize.height);
      }

      // Place the child
      x += margin.left;
      nsRect kidRect(x, topMargin, kidSize.width, kidSize.height);
      PlaceChild(aPresContext, aState, kidFrame, kidRect, aDesiredSize.maxElementSize,
                 &kidMaxElementSize);
      x += kidSize.width + margin.right;
    }
  }

  SetMaxChildHeight(aState.maxCellHeight, maxTopMargin, maxBottomMargin);  // remember height of tallest child who doesn't have a row span

  // Return our desired size
  aDesiredSize.width = x;
  aDesiredSize.height = aState.maxCellVertSpace;   

  return result;
}

// Recover the reflow state to what it should be if aKidFrame is about
// to be reflowed
//
// The things in the RowReflowState object we need to restore are:
// - maxCellHeight
// - maxVertCellSpace
// - x
nsresult nsTableRowFrame::RecoverState(nsIPresContext& aPresContext,
                                       RowReflowState& aState,
                                       nsIFrame*       aKidFrame,
                                       nscoord&        aMaxCellTopMargin,
                                       nscoord&        aMaxCellBottomMargin)
{
  aMaxCellTopMargin = aMaxCellBottomMargin = 0;

  // Walk the list of children looking for aKidFrame. While we're at
  // it get the maxCellHeight and maxVertCellSpace for all the
  // frames except aKidFrame
//  nsIFrame* prevKidFrame = nsnull;
  for (nsIFrame* frame = mFirstChild; nsnull != frame;) {
    const nsStyleDisplay *kidDisplay;
    frame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)kidDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CELL == kidDisplay->mDisplay)
    {
      if (frame != aKidFrame) {
        // Update the max top and bottom margins
        nsMargin       kidMargin;
        aState.tableFrame->GetCellMarginData((nsTableCellFrame *)frame, kidMargin);
        if (kidMargin.top > aMaxCellTopMargin)
          aMaxCellTopMargin = kidMargin.top;
        if (kidMargin.bottom > aMaxCellBottomMargin)
          aMaxCellBottomMargin = kidMargin.bottom;

        PRInt32 rowSpan = aState.tableFrame->GetEffectiveRowSpan(mRowIndex, ((nsTableCellFrame *)frame));
        if (mMinRowSpan == rowSpan) {
          // Get the cell's desired height the last time it was reflowed
          nsSize  desiredSize = ((nsTableCellFrame *)frame)->GetDesiredSize();

          // See if it has a specified height that overrides the desired size
          nscoord specifiedHeight = 0;
          nsIStyleContextPtr kidSC;
          frame->GetStyleContext(&aPresContext, kidSC.AssignRef());
          const nsStylePosition* kidPosition = (const nsStylePosition*)
            kidSC->GetStyleData(eStyleStruct_Position);
          switch (kidPosition->mHeight.GetUnit()) {
          case eStyleUnit_Coord:
            specifiedHeight = kidPosition->mHeight.GetCoordValue();
            break;
      
          case eStyleUnit_Inherit:
            // XXX for now, do nothing
          default:
          case eStyleUnit_Auto:
            break;
          }
          if (specifiedHeight > desiredSize.height)
            desiredSize.height = specifiedHeight;
        
          // Update maxCellHeight
          if (desiredSize.height > aState.maxCellHeight) {
            aState.maxCellHeight = desiredSize.height;
          }
  
          // Update maxCellVertHeight
          nsMargin margin;
    
          if (aState.tableFrame->GetCellMarginData((nsTableCellFrame *)frame, margin) == NS_OK)
          {
            nscoord height = desiredSize.height + margin.top + margin.bottom;
            if (height > aState.maxCellVertSpace) {
              aState.maxCellVertSpace = height;
            }
          }
        }

        // XXX We also need to recover the max element size if requested by the
        // caller...
        //
        // We should be using GetReflowMetrics() to get information from the
        // table cell, and that will include the max element size...
      }
    }

    // Remember the frame that precedes aKidFrame
//    prevKidFrame = frame;
    frame->GetNextSibling(frame);
  }

  // Update the running x-offset based on the frame's current x-origin
  nsPoint origin;
  aKidFrame->GetOrigin(origin);
  aState.x = origin.x;

  return NS_OK;
}

nsresult nsTableRowFrame::IncrementalReflow(nsIPresContext&  aPresContext,
                                            RowReflowState&  aState,
                                            nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult  status;

  // XXX Deal with the case where the reflow command is targeted at us
  nsIFrame* target;
  aState.reflowState.reflowCommand->GetTarget(target);
  if (this == target) {
    NS_NOTYETIMPLEMENTED("unexpected reflow command");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  // Get the next frame in the reflow chain
  nsIFrame* kidFrame;
  aState.reflowState.reflowCommand->GetNext(kidFrame);

  // Recover our reflow state
  nscoord maxCellTopMargin, maxCellBottomMargin;
  RecoverState(aPresContext, aState, kidFrame, maxCellTopMargin, maxCellBottomMargin);

  // Get the frame's margins
  nsMargin  kidMargin;
  aState.tableFrame->GetCellMarginData((nsTableCellFrame *)kidFrame, kidMargin);
  if (kidMargin.top > maxCellTopMargin)
    maxCellTopMargin = kidMargin.top;
  if (kidMargin.bottom > maxCellBottomMargin)
    maxCellBottomMargin = kidMargin.bottom;

  // At this point, we know the column widths. Get the available width
  // from the known column widths
  PRInt32 cellColIndex = ((nsTableCellFrame *)kidFrame)->GetColIndex();
  PRInt32 cellColSpan = aState.tableFrame->GetEffectiveColSpan(((nsTableCellFrame *)kidFrame)->GetColIndex(),
                                                                 ((nsTableCellFrame *)kidFrame));
  nscoord availWidth = 0;
  for (PRInt32 numColSpan = 0; numColSpan < cellColSpan; numColSpan++)
  {
    availWidth += aState.tableFrame->GetColumnWidth(cellColIndex+numColSpan);
    if (0<numColSpan)
    {
      availWidth += kidMargin.right;
      if (0!=cellColIndex)
        availWidth += kidMargin.left;
    }
  }

  // Always let the cell be as high as it wants. We ignore the height that's
  // passed in and always place the entire row. Let the row group decide
  // whether we fit or wehther the entire row is pushed
  nsSize  kidAvailSize(availWidth, NS_UNCONSTRAINEDSIZE);

  // Pass along the reflow command
  nsSize          kidMaxElementSize;
  nsHTMLReflowMetrics desiredSize(&kidMaxElementSize);
  nsHTMLReflowState kidReflowState(aPresContext, kidFrame, aState.reflowState,
                                   kidAvailSize);

  // XXX Unfortunately we need to reflow the child several times.
  // The first time is for the incremental reflow command. We can't pass in
  // a max width of NS_UNCONSTRAINEDSIZE, because the max width must match
  // the width of the previous reflow...
  ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, status);

  // Now do the regular pass 1 reflow and gather the max width and max element
  // size.
  // XXX It would be nice if we could skip this step and the next step if the
  // column width isn't dependent on the max cell width...
  kidReflowState.reason = eReflowReason_Resize;
  kidReflowState.reflowCommand = nsnull;
  kidReflowState.maxSize.width = NS_UNCONSTRAINEDSIZE;
  ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, status);
  if (gsDebug) 
      printf ("TR %p for cell %p Incremental Reflow: desired=%d, MES=%d\n", 
             this, kidFrame, desiredSize.width, kidMaxElementSize.width);
  // Update the cell layout data.
  //XXX: this is a hack, shouldn't it be the case that a min size is 
  //     never larger than a desired size?
  if (kidMaxElementSize.width>desiredSize.width)
    desiredSize.width = kidMaxElementSize.width;
  if (kidMaxElementSize.height>desiredSize.height)
    desiredSize.height = kidMaxElementSize.height;
  ((nsTableCellFrame *)kidFrame)->SetPass1DesiredSize(desiredSize);
  ((nsTableCellFrame *)kidFrame)->SetPass1MaxElementSize(kidMaxElementSize);
  
  // Now reflow the cell again this time constraining the width
  // XXX Ignore for now the possibility that the column width has changed...
  kidReflowState.maxSize.width = availWidth;
  ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, status);
  
  // Place the child after taking into account it's margin and attributes
  // XXX We need to ask the table (or the table layout strategy) if the column
  // widths have changed. If so, we just bail and return a status indicating
  // what happened and let the table reflow all the table cells...
  nscoord specifiedHeight = 0;
  nscoord cellHeight = desiredSize.height;
  nsIStyleContextPtr kidSC;
  kidFrame->GetStyleContext(&aPresContext, kidSC.AssignRef());
  const nsStylePosition* kidPosition = (const nsStylePosition*)
    kidSC->GetStyleData(eStyleStruct_Position);
  switch (kidPosition->mHeight.GetUnit()) {
  case eStyleUnit_Coord:
    specifiedHeight = kidPosition->mHeight.GetCoordValue();
    break;

  case eStyleUnit_Inherit:
    // XXX for now, do nothing
  default:
  case eStyleUnit_Auto:
    break;
  }
  if (specifiedHeight>cellHeight)
    cellHeight = specifiedHeight;

  nscoord cellWidth = desiredSize.width;
  // begin special Nav4 compatibility code
  if (0==cellWidth)
  {
    cellWidth = aState.tableFrame->GetColumnWidth(cellColIndex);
  }
  // end special Nav4 compatibility code

  // Now place the child
  nsRect kidRect (aState.x, kidMargin.top, cellWidth, cellHeight);

  PlaceChild(aPresContext, aState, kidFrame, kidRect, aDesiredSize.maxElementSize,
             &kidMaxElementSize);

  SetMaxChildHeight(aState.maxCellHeight, maxCellTopMargin, maxCellBottomMargin);

  // Return our desired size. Note that our desired width is just whatever width
  // we were given by the row group frame
  aDesiredSize.width = aState.availSize.width;
  aDesiredSize.height = aState.maxCellVertSpace;   

  if (gsDebug)
    printf("incr -- row %p width = %d MES=%d from maxSize %d\n", 
           this, aDesiredSize.width, aDesiredSize.maxElementSize->width,
           aState.reflowState.maxSize.width);

  return status;
}

/** Layout the entire row.
  * This method stacks cells horizontally according to HTML 4.0 rules.
  */
NS_METHOD
nsTableRowFrame::Reflow(nsIPresContext&          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  if (gsDebug==PR_TRUE)
    printf("nsTableRowFrame::Reflow - aMaxSize = %d, %d\n",
            aReflowState.maxSize.width, aReflowState.maxSize.height);

  // Initialize 'out' parameters
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }
  aStatus = NS_FRAME_COMPLETE;  // we're never continued

  // Initialize our internal data
  ResetMaxChildHeight();

  // Initialize our automatic state object
  nsTableFrame* tableFrame;
  mContentParent->GetContentParent((nsIFrame*&)tableFrame);
  RowReflowState state(aReflowState, tableFrame);

  // Do the reflow
  nsresult  result;

  switch (aReflowState.reason) {
  case eReflowReason_Initial:
    result = InitialReflow(aPresContext, state, aDesiredSize);
    GetMinRowSpan(tableFrame);
    FixMinCellHeight(tableFrame);
    break;

  case eReflowReason_Resize:
    result = ResizeReflow(aPresContext, state, aDesiredSize);
    break;

  case eReflowReason_Incremental:
    result = IncrementalReflow(aPresContext, state, aDesiredSize);
    break;

  }

  if (gsDebug==PR_TRUE) 
  {
    if (nsnull!=aDesiredSize.maxElementSize)
      printf("%p: Row::RR returning: %s with aDesiredSize=%d,%d, aMES=%d,%d\n",
              this, NS_FRAME_IS_COMPLETE(aStatus)?"Complete":"Not Complete",
              aDesiredSize.width, aDesiredSize.height,
              aDesiredSize.maxElementSize->width, aDesiredSize.maxElementSize->height);
    else
      printf("%p: Row::RR returning: %s with aDesiredSize=%d,%d, aMES=NSNULL\n", 
             this, NS_FRAME_IS_COMPLETE(aStatus)?"Complete":"Not Complete",
             aDesiredSize.width, aDesiredSize.height);
  }

  return result;
}

NS_METHOD
nsTableRowFrame::CreateContinuingFrame(nsIPresContext&  aPresContext,
                                       nsIFrame*        aParent,
                                       nsIStyleContext* aStyleContext,
                                       nsIFrame*&       aContinuingFrame)
{
  // Because rows are always complete we should never be asked to create
  // a continuing frame
  NS_ERROR("Unexpected request");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* ----- global methods ----- */

nsresult 
NS_NewTableRowFrame(nsIContent* aContent,
                    nsIFrame*   aParentFrame,
                    nsIFrame*&  aResult)
{
  nsIFrame* it = new nsTableRowFrame(aContent, aParentFrame);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = it;
  return NS_OK;
}

/* ----- debugging methods ----- */
NS_METHOD nsTableRowFrame::List(FILE* out, PRInt32 aIndent, nsIListFilter *aFilter) const
{
  // if a filter is present, only output this frame if the filter says we should
  // since this could be any "tag" with the right display type, we'll
  // just pretend it's a row
  if (nsnull==aFilter)
    return nsContainerFrame::List(out, aIndent, aFilter);

  nsAutoString tagString("tr");
  PRBool outputMe = aFilter->OutputTag(&tagString);
  if (PR_TRUE==outputMe)
  {
    // Indent
    for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);

    // Output the tag and rect
    nsIAtom* tag;
    mContent->GetTag(tag);
    if (tag != nsnull) {
      nsAutoString buf;
      tag->ToString(buf);
      fputs(buf, out);
      NS_RELEASE(tag);
    }

    fprintf(out, "(%d)", ContentIndexInContainer(this));
    out << mRect;
    if (0 != mState) {
      fprintf(out, " [state=%08x]", mState);
    }
    fputs("\n", out);
  }
  // Output the children
  if (nsnull != mFirstChild) {
    if (PR_TRUE==outputMe)
    {
      if (0 != mState) {
        fprintf(out, " [state=%08x]\n", mState);
      }
    }
    for (nsIFrame* child = mFirstChild; child; child->GetNextSibling(child)) {
      child->List(out, aIndent + 1, aFilter);
    }
  } else {
    if (PR_TRUE==outputMe)
    {
      if (0 != mState) {
        fprintf(out, " [state=%08x]\n", mState);
      }
    }
  }
  return NS_OK;
}


