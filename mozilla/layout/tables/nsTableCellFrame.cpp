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
#include "nsTableCellFrame.h"
#include "nsTableColFrame.h"
#include "nsTableFrame.h"
#include "nsIReflowCommand.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLParts.h"
#include "nsHTMLValue.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsVoidArray.h"
#include "nsIPtr.h"
#include "nsIView.h"
#include "nsStyleUtil.h"
#include "nsLayoutAtoms.h"
#include "nsCOMPtr.h"
#include "nsIHTMLTableCellElement.h"

NS_DEF_PTR(nsIStyleContext);
static NS_DEFINE_IID(kIHTMLTableCellElementIID, NS_IHTMLTABLECELLELEMENT_IID);

NS_IMETHODIMP
nsTableCellFrame::Init(nsIPresContext&  aPresContext,
                       nsIContent*      aContent,
                       nsIFrame*        aParent,
                       nsIStyleContext* aContext,
                       nsIFrame*        aPrevInFlow)
{
  nsresult  rv;
  
  // Let the base class do its initialization
  rv = nsHTMLContainerFrame::Init(aPresContext, aContent, aParent, aContext,
                                  aPrevInFlow);

  if (aPrevInFlow) {
    // Set the column index
    nsTableCellFrame* cellFrame = (nsTableCellFrame*)aPrevInFlow;
    PRInt32           baseColIndex;
    
    cellFrame->GetColIndex(baseColIndex);
    InitCellFrame(baseColIndex);
  }

  return rv;
}

NS_IMETHODIMP
nsTableCellFrame::AppendFrames(nsIPresContext& aPresContext,
                               nsIPresShell&   aPresShell,
                               nsIAtom*        aListName,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::InsertFrames(nsIPresContext& aPresContext,
                               nsIPresShell&   aPresShell,
                               nsIAtom*        aListName,
                               nsIFrame*       aPrevFrame,
                               nsIFrame*       aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsTableCellFrame::RemoveFrame(nsIPresContext& aPresContext,
                              nsIPresShell&   aPresShell,
                              nsIAtom*        aListName,
                              nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "unsupported operation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void nsTableCellFrame::InitCellFrame(PRInt32 aColIndex)
{
  NS_PRECONDITION(0<=aColIndex, "bad col index arg");
  SetColIndex(aColIndex); // this also sets the contents col index
  mBorderEdges.mOutsideEdge=PR_FALSE;
  nsTableFrame* tableFrame=nsnull;  // I should be checking my own style context, but border-collapse isn't inheriting correctly
  nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
  if ((NS_SUCCEEDED(rv)) && (nsnull!=tableFrame))
  {
    if (NS_STYLE_BORDER_COLLAPSE == tableFrame->GetBorderCollapseStyle())
    {
      PRInt32 rowspan = GetRowSpan();
      PRInt32 i;
      for (i=0; i<rowspan; i++)
      {
        nsBorderEdge *borderToAdd = new nsBorderEdge();
        mBorderEdges.mEdges[NS_SIDE_LEFT].AppendElement(borderToAdd);
        borderToAdd = new nsBorderEdge();
        mBorderEdges.mEdges[NS_SIDE_RIGHT].AppendElement(borderToAdd);
      }
      PRInt32 colspan = GetColSpan();
      for (i=0; i<colspan; i++)
      {
        nsBorderEdge *borderToAdd = new nsBorderEdge();
        mBorderEdges.mEdges[NS_SIDE_TOP].AppendElement(borderToAdd);
        borderToAdd = new nsBorderEdge();
        mBorderEdges.mEdges[NS_SIDE_BOTTOM].AppendElement(borderToAdd);
      }
    }
    mCollapseOffset = nsPoint(0,0);
  }
}

nsresult nsTableCellFrame::SetColIndex(PRInt32 aColIndex)
{  
  mColIndex = aColIndex;
  // for style context optimization, set the content's column index if possible.
  // this can only be done if we really have an nsTableCell.  
  // other tags mapped to table cell display won't benefit from this optimization
  // see nsHTMLStyleSheet::RulesMatching

  //nsIContent* cell;
  //kidFrame->GetContent(&cell);
  nsCOMPtr<nsIContent> cell;
  nsresult rv = GetContent(getter_AddRefs(cell));
  if (NS_FAILED(rv) || !cell)
    return rv;

  nsIHTMLTableCellElement* cellContent = nsnull;
  rv = cell->QueryInterface(kIHTMLTableCellElementIID, 
                            (void **)&cellContent);  // cellContent: REFCNT++
  if (cellContent && NS_SUCCEEDED(rv)) { // it's a table cell
    cellContent->SetColIndex(aColIndex);
    NS_RELEASE(cellContent);
  }
  return rv;
}

      
void nsTableCellFrame::SetBorderEdgeLength(PRUint8 aSide, 
                                           PRInt32 aIndex, 
                                           nscoord aLength)
{
  if ((NS_SIDE_LEFT==aSide) || (NS_SIDE_RIGHT==aSide))
  {
    PRInt32 baseRowIndex;
    GetRowIndex(baseRowIndex);
    PRInt32 rowIndex = aIndex-baseRowIndex;
    nsBorderEdge *border = (nsBorderEdge *)(mBorderEdges.mEdges[aSide].ElementAt(rowIndex));
    border->mLength = aLength;
  }
  else {
    NS_ASSERTION(PR_FALSE, "bad arg aSide passed to SetBorderEdgeLength");
  }
}


NS_METHOD nsTableCellFrame::Paint(nsIPresContext& aPresContext,
                                  nsIRenderingContext& aRenderingContext,
                                  const nsRect& aDirtyRect,
                                  nsFramePaintLayer aWhichLayer)
{
  const nsStyleDisplay* disp =
    (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);

  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    if (disp->mVisible) {
      const nsStyleColor* myColor =
        (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
      const nsStyleSpacing* mySpacing =
        (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
      NS_ASSERTION(nsnull!=myColor, "bad style color");
      NS_ASSERTION(nsnull!=mySpacing, "bad style spacing");

      const nsStyleTable* cellTableStyle;
      GetStyleData(eStyleStruct_Table, ((const nsStyleStruct *&)cellTableStyle)); 
      nsRect  rect(0, 0, mRect.width, mRect.height);

      // only non empty cells render their background
      if (PR_FALSE == GetContentEmpty()) {
        nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                        aDirtyRect, rect, *myColor, *mySpacing, 0, 0);
      }
    
      // empty cells do not render their border
      PRBool renderBorder = PR_TRUE;
      if (PR_TRUE==GetContentEmpty())
      {
        if (NS_STYLE_TABLE_EMPTY_CELLS_HIDE==cellTableStyle->mEmptyCells)
          renderBorder=PR_FALSE;
      }
      if (PR_TRUE==renderBorder)
      {
        PRIntn skipSides = GetSkipSides();
        nsTableFrame* tableFrame=nsnull;  // I should be checking my own style context, but border-collapse isn't inheriting correctly
        nsresult rv = nsTableFrame::GetTableFrame(this, tableFrame);
        if ((NS_SUCCEEDED(rv)) && (nsnull!=tableFrame))
        {
          const nsStyleTable* tableStyle;
          tableFrame->GetStyleData(eStyleStruct_Table, ((const nsStyleStruct *&)tableStyle)); 
          if (NS_STYLE_BORDER_SEPARATE == tableFrame->GetBorderCollapseStyle())
          {
            nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                        aDirtyRect, rect, *mySpacing, mStyleContext, skipSides);
          }
          else
          {
            nsCSSRendering::PaintBorderEdges(aPresContext, aRenderingContext, this,
                                             aDirtyRect, rect, &mBorderEdges, mStyleContext, skipSides);
          }
        }
      }
    }
  }

  // for debug...
  if ((NS_FRAME_PAINT_LAYER_DEBUG == aWhichLayer) && GetShowFrameBorders()) {
    aRenderingContext.SetColor(NS_RGB(0, 0, 128));
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }

  // if the cell originates in a row and/or col that is collapsed, the
  // bottom and/or right portion of the cell is painted by translating
  // the rendering context.
  PRBool clipState;
  aRenderingContext.PushState();
  nsPoint offset = mCollapseOffset;
  if ((0 != offset.x) || (0 != offset.y)) {
    aRenderingContext.Translate(offset.x, offset.y);
  }
  // Bug 6674 is fixed by uncommenting the following if statement, but sample 4 resizing 
  // of the collapsed table rows then paints incorrectly
  //if (NS_STYLE_OVERFLOW_HIDDEN == disp->mOverflow) {
    aRenderingContext.SetClipRect(nsRect(-offset.x, -offset.y, mRect.width, mRect.height),
                                nsClipCombine_kIntersect, clipState);
  //}
  PaintChildren(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
  aRenderingContext.PopState(clipState);
  
  return nsFrame::Paint(aPresContext,
                        aRenderingContext,
                        aDirtyRect,
                        aWhichLayer);
}

//null range means the whole thing
NS_IMETHODIMP
nsTableCellFrame::SetSelected(nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread)
{
  //traverse through children unselect tables
  if ((aSpread == eSpreadDown) && aSelected){
    nsIFrame* kid;
    FirstChild(nsnull, &kid);
    while (nsnull != kid) {
      kid->SetSelected(nsnull,PR_FALSE,eSpreadDown);

      kid->GetNextSibling(&kid);
    }
  }
  return nsFrame::SetSelected(aRange,aSelected,eSpreadNone);
}

PRIntn
nsTableCellFrame::GetSkipSides() const
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

PRBool nsTableCellFrame::ParentDisablesSelection() const //override default behavior
{
  PRBool returnval;
  if (NS_FAILED(GetSelected(&returnval)))
    return PR_FALSE;
  if (returnval)
    return PR_TRUE;
  return nsFrame::ParentDisablesSelection();
}

void nsTableCellFrame::SetBorderEdge(PRUint8       aSide, 
                                     PRInt32       aRowIndex, 
                                     PRInt32       aColIndex, 
                                     nsBorderEdge *aBorder,
                                     nscoord       aOddAmountToAdd)
{
  nsBorderEdge *border = nsnull;
  switch (aSide)
  {
    case NS_SIDE_TOP:
    {
      PRInt32 baseColIndex;
      GetColIndex(baseColIndex);
      PRInt32 colIndex = aColIndex-baseColIndex;
      border = (nsBorderEdge *)(mBorderEdges.mEdges[aSide].ElementAt(colIndex));
      mBorderEdges.mMaxBorderWidth.top = PR_MAX(aBorder->mWidth+aOddAmountToAdd, mBorderEdges.mMaxBorderWidth.top);
      break;
    }

    case NS_SIDE_BOTTOM:
    {
      PRInt32 baseColIndex;
      GetColIndex(baseColIndex);
      PRInt32 colIndex = aColIndex-baseColIndex;
      border = (nsBorderEdge *)(mBorderEdges.mEdges[aSide].ElementAt(colIndex));
      mBorderEdges.mMaxBorderWidth.bottom = PR_MAX(aBorder->mWidth+aOddAmountToAdd, mBorderEdges.mMaxBorderWidth.bottom);
      break;
    }
  
    case NS_SIDE_LEFT:
    {
      PRInt32 baseRowIndex;
      GetRowIndex(baseRowIndex);
      PRInt32 rowIndex = aRowIndex-baseRowIndex;
      border = (nsBorderEdge *)(mBorderEdges.mEdges[aSide].ElementAt(rowIndex));
      mBorderEdges.mMaxBorderWidth.left = PR_MAX(aBorder->mWidth+aOddAmountToAdd, mBorderEdges.mMaxBorderWidth.left);
      break;
    }
      
    case NS_SIDE_RIGHT:  
    {
      PRInt32 baseRowIndex;
      GetRowIndex(baseRowIndex);
      PRInt32 rowIndex = aRowIndex-baseRowIndex;
      border = (nsBorderEdge *)(mBorderEdges.mEdges[aSide].ElementAt(rowIndex));
      mBorderEdges.mMaxBorderWidth.right = PR_MAX(aBorder->mWidth+aOddAmountToAdd, mBorderEdges.mMaxBorderWidth.right);
      break;
    }
  }
  if (nsnull!=border) {
    *border=*aBorder;
    border->mWidth += aOddAmountToAdd;
  }
  else {
    NS_ASSERTION(PR_FALSE, "bad border edge state");
  }
}

/**
  *
  * Align the cell's child frame within the cell
  *
  */
void  nsTableCellFrame::VerticallyAlignChild()
{
  const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  const nsStyleText* textStyle =
      (const nsStyleText*)mStyleContext->GetStyleData(eStyleStruct_Text);
  /* XXX: remove tableFrame when border-collapse inherits */
  nsTableFrame* tableFrame=nsnull;
  (void) nsTableFrame::GetTableFrame(this, tableFrame);
  nsMargin borderPadding;
  GetCellBorder (borderPadding, tableFrame);
  nsMargin padding;
  spacing->GetPadding(padding);
  borderPadding += padding;
  
  nscoord topInset = borderPadding.top;
  nscoord bottomInset = borderPadding.bottom;
  PRUint8 verticalAlignFlags = NS_STYLE_VERTICAL_ALIGN_MIDDLE;
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    verticalAlignFlags = textStyle->mVerticalAlign.GetIntValue();
  }
  nscoord height = mRect.height;
  nsRect kidRect;  
  nsIFrame* firstKid = mFrames.FirstChild();
  firstKid->GetRect(kidRect);
  nscoord childHeight = kidRect.height;
  

  // Vertically align the child
  nscoord kidYTop = 0;
  switch (verticalAlignFlags) 
  {
    case NS_STYLE_VERTICAL_ALIGN_BASELINE:
      // Align the baselines of the child frame with the baselines of 
      // other children in the same row which have align = baseline 
      // XXX Not yet implemented, so fall through to top

    case NS_STYLE_VERTICAL_ALIGN_TOP:
      // Align the top of the child frame with the top of the content area, 
      kidYTop = topInset;
    break;

    case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
      // Align the bottom of the child frame with the bottom of the content area, 
      kidYTop = height - childHeight - bottomInset;
    break;

    default:
    case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
      // Align the middle of the child frame with the middle of the content area, 
      kidYTop = (height - childHeight - bottomInset + topInset) / 2;
  }
  firstKid->MoveTo(kidRect.x, kidYTop);
}

PRInt32 nsTableCellFrame::GetRowSpan()
{  
  PRInt32 rowSpan=1;
  nsIHTMLContent *hc=nsnull;
  nsresult rv = mContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (NS_OK==rv)
  {
    nsHTMLValue val;
    hc->GetHTMLAttribute(nsHTMLAtoms::rowspan, val); 
    if (eHTMLUnit_Integer == val.GetUnit()) { 
       rowSpan=val.GetIntValue(); 
    }
    NS_RELEASE(hc);
  }
  return rowSpan;
}

PRInt32 nsTableCellFrame::GetColSpan()
{  
  PRInt32 colSpan=1;
  nsIHTMLContent *hc=nsnull;
  nsresult rv = mContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (NS_OK==rv)
  {
    nsHTMLValue val;
    hc->GetHTMLAttribute(nsHTMLAtoms::colspan, val); 
    if (eHTMLUnit_Integer == val.GetUnit()) { 
       colSpan=val.GetIntValue(); 
    }
    NS_RELEASE(hc);
  }
  return colSpan;
}


static
void DebugCheckChildSize(nsIFrame*            aChild, 
                         nsHTMLReflowMetrics& aMet, 
                         nsSize&              aAvailSize,
                         PRBool               aIsPass2Reflow)
{

/* approved for commenting out by rickg
  if (aMet.width > aAvailSize.width) {
    nsAutoString tmp;
    aChild->GetFrameName(tmp);
    printf("WARNING: cell ");
    fputs(tmp, stdout);
    printf(" content has desired width %d given avail width %d\n",
            aMet.width, aAvailSize.width);
  }
*/
  if (aIsPass2Reflow) {
    if ((aMet.width < 0) || (aMet.width > 60000)) {
      printf("WARNING: cell content %p has large width %d \n", aChild, aMet.width);
    }
    if ((aMet.height < 0) || (aMet.height > 60000)) {
      printf("WARNING: cell content %p has large height %d \n", aChild, aMet.height);
    }
  }
  if (aMet.maxElementSize) {
    nscoord tmp = aMet.maxElementSize->width;
    if ((tmp < 0) || (tmp > 60000)) {
      printf("WARNING: cell content %p has large max element width %d \n", aChild, tmp);
    }
    tmp = aMet.maxElementSize->height;
    if ((tmp < 0) || (tmp > 60000)) {
      printf("WARNING: cell content %p has large max element height %d \n", aChild, tmp);
    }
  }
}

/**
  */
NS_METHOD nsTableCellFrame::Reflow(nsIPresContext&          aPresContext,
                                   nsHTMLReflowMetrics&     aDesiredSize,
                                   const nsHTMLReflowState& aReflowState,
                                   nsReflowStatus&          aStatus)
{
  if (nsDebugTable::gRflCell) nsTableFrame::DebugReflow("TC::Rfl", this, &aReflowState, nsnull);

  nsresult rv = NS_OK;
  // this should probably be cached somewhere
  nsCompatibility compatMode;
  aPresContext.GetCompatibilityMode(&compatMode);

  // Initialize out parameter
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }

  aStatus = NS_FRAME_COMPLETE;
  nsSize availSize(aReflowState.availableWidth, aReflowState.availableHeight);
  nsSize maxElementSize;
  nsSize *pMaxElementSize = aDesiredSize.maxElementSize;
  if (NS_UNCONSTRAINEDSIZE==aReflowState.availableWidth)
    pMaxElementSize = &maxElementSize;

  // SEC: what about ascent and decent???

  // Compute the insets (sum of border and padding)
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);

  /* XXX: remove tableFrame when border-collapse inherits */
  nsTableFrame* tableFrame=nsnull;
  rv = nsTableFrame::GetTableFrame(this, tableFrame);
  nsMargin padding;
  spacing->GetPadding(padding);
  nsMargin borderPadding = padding;
  nsMargin border;
  GetCellBorder(border, tableFrame);
  if ((NS_UNCONSTRAINEDSIZE == availSize.width) || !GetContentEmpty()) {
    borderPadding += border;
  }
  
  nscoord topInset    = borderPadding.top;
  nscoord rightInset  = borderPadding.right;
  nscoord bottomInset = borderPadding.bottom;
  nscoord leftInset   = borderPadding.left;

  // reduce available space by insets, if we're in a constrained situation
  if (NS_UNCONSTRAINEDSIZE!=availSize.width)
    availSize.width -= leftInset+rightInset;
  if (NS_UNCONSTRAINEDSIZE!=availSize.height)
    availSize.height -= topInset+bottomInset;

  PRBool  isStyleChanged = PR_FALSE;
  if (eReflowReason_Incremental == aReflowState.reason) 
  {
    // We *must* do this otherwise incremental reflow that's
    // passing through will not work right.
    nsIFrame* next;
    aReflowState.reflowCommand->GetNext(next);

    // if it is a StyleChanged reflow targeted at this cell frame,
    // handle that here
    // first determine if this frame is the target or not
    nsIFrame *target=nsnull;
    rv = aReflowState.reflowCommand->GetTarget(target);
    if ((PR_TRUE==NS_SUCCEEDED(rv)) && (nsnull!=target))
    {
      if (this==target)
      {
        nsIReflowCommand::ReflowType type;
        aReflowState.reflowCommand->GetType(type);
        if (nsIReflowCommand::StyleChanged==type)
        {
          isStyleChanged = PR_TRUE;
        }
        else {
          NS_ASSERTION(PR_FALSE, "table cell target of illegal incremental reflow type");
        }
      }
    }
    // if any of these conditions are not true, we just pass the reflow command down
  }

  // Try to reflow the child into the available space. It might not
  // fit or might need continuing.
  if (availSize.height < 0)
    availSize.height = 1;

  nsHTMLReflowMetrics kidSize(pMaxElementSize);
  kidSize.width=kidSize.height=kidSize.ascent=kidSize.descent=0;
  SetPriorAvailWidth(aReflowState.availableWidth);
  nsIFrame* firstKid = mFrames.FirstChild();
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, firstKid,
                                   availSize);

  // If it was a style change targeted at us, then reflow the child using
  // the special reflow reason
  if (isStyleChanged) {
    kidReflowState.reason = eReflowReason_StyleChange;
    kidReflowState.reflowCommand = nsnull;
  }

  if (nsDebugTable::gRflArea) nsTableFrame::DebugReflow("Area::Rfl en", firstKid, &kidReflowState, nsnull);
  ReflowChild(firstKid, aPresContext, kidSize, kidReflowState, aStatus);
  if (nsDebugTable::gRflArea) nsTableFrame::DebugReflow("Area::Rfl ex", firstKid, nsnull, &kidSize);

#ifdef NS_DEBUG
  DebugCheckChildSize(firstKid, kidSize, availSize, (NS_UNCONSTRAINEDSIZE != aReflowState.availableWidth));
#endif

  PRBool useInsets = PR_TRUE;
  // 0 dimensioned cells need to be treated specially in Standard/NavQuirks mode 
  // see testcase "emptyCells.html"
  if (NS_UNCONSTRAINEDSIZE == kidReflowState.availableWidth) {
    if ((0 == kidSize.width) || (0 == kidSize.height)) {
    //if ((0 == kidSize.width) && (0 == kidSize.height)) { // XXX why was this &&
      SetContentEmpty(PR_TRUE);
      useInsets = PR_FALSE;
      // need to reduce the insets by border if the cell is empty
      leftInset   -= border.left;
      rightInset  -= border.right;
      topInset    -= border.top;
      bottomInset -= border.bottom;
    }
    else 
      SetContentEmpty(PR_FALSE);
  }

  const nsStylePosition* pos;
  GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)pos));

  // calculate the min cell width
  float p2t;
  aPresContext.GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t); 
  nscoord smallestMinWidth = onePixel;
  if (eCompatibility_NavQuirks == compatMode) {
    if ((pos->mWidth.GetUnit() != eStyleUnit_Coord)   &&
        (pos->mWidth.GetUnit() != eStyleUnit_Percent)) {
      if (border.left > 0) 
        smallestMinWidth += onePixel;
      if (border.right > 0) 
        smallestMinWidth += onePixel;
    }
    PRInt32 colspan = GetColSpan();
    if (colspan > 1) {
      smallestMinWidth = PR_MAX(smallestMinWidth, colspan * onePixel);
      nscoord spacingX = tableFrame->GetCellSpacingX();
      nscoord spacingExtra = spacingX * (colspan - 1);
      smallestMinWidth += spacingExtra;
      if (padding.left > 0) {
        smallestMinWidth -= onePixel;
      }
    }
  }

  if (0 == kidSize.width) {
    if (NS_UNCONSTRAINEDSIZE == kidReflowState.availableWidth) {
//    const nsStylePosition* pos;
//    GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)pos));
//      if ((pos->mWidth.GetUnit() != eStyleUnit_Coord)   &&
//          (pos->mWidth.GetUnit() != eStyleUnit_Percent)) {
//      kidSize.width = minCellWidth;
//      if ((nsnull != aDesiredSize.maxElementSize) && (0 == pMaxElementSize->width))
//        pMaxElementSize->width = kidSize.width;
//      }
    }
    else  // empty content has to be forced to the assigned width for resize or incremental reflow
      kidSize.width = kidReflowState.availableWidth;
  }
  if (0 == kidSize.height) {
    const nsStylePosition* pos;
    GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)pos));
    if ((pos->mHeight.GetUnit() != eStyleUnit_Coord) &&
        (pos->mHeight.GetUnit() != eStyleUnit_Percent)) {
      // Standard mode should probably be 0 pixels high instead of 1
      PRInt32 pixHeight = (eCompatibility_Standard == compatMode) ? 1 : 2;
      kidSize.height = NSIntPixelsToTwips(pixHeight, p2t);
      if ((nsnull != aDesiredSize.maxElementSize) && (0 == pMaxElementSize->height))
        pMaxElementSize->height = kidSize.height;
    }
  }
  // end 0 dimensioned cells

  kidSize.width = PR_MAX(kidSize.width, smallestMinWidth); 

  // Place the child
  //////////////////////////////// HACK //////////////////////////////
  kidSize.width = PR_MIN(kidSize.width, availSize.width);
  ///////////////////////////// END HACK /////////////////////////////
  firstKid->SetRect(nsRect(leftInset, topInset,
                           kidSize.width, kidSize.height));  
    
  // Return our size and our result

  // first, compute the height which can be set w/o being restricted by aMaxSize.height
  nscoord cellHeight = kidSize.height + topInset + bottomInset;

  // next determine the cell's width
  nscoord cellWidth = kidSize.width;      // at this point, we've factored in the cell's style attributes

  // add the insets into the cell width (in both the constrained-width and unconstrained-width cases
  cellWidth += leftInset + rightInset;    // factor in border and padding

  // set the cell's desired size and max element size
  aDesiredSize.width = cellWidth;
  aDesiredSize.height = cellHeight;
  aDesiredSize.ascent = topInset;
  aDesiredSize.descent = bottomInset;
  if (nsnull!=aDesiredSize.maxElementSize)
  {
    *aDesiredSize.maxElementSize = *pMaxElementSize;
    if (0!=pMaxElementSize->height)
      aDesiredSize.maxElementSize->height += topInset + bottomInset;
    //if (0!=pMaxElementSize->width)
    aDesiredSize.maxElementSize->width = PR_MAX(smallestMinWidth, aDesiredSize.maxElementSize->width); 
    aDesiredSize.maxElementSize->width += leftInset + rightInset;
  }
  // remember my desired size for this reflow
  SetDesiredSize(aDesiredSize);

  aDesiredSize.ascent=aDesiredSize.height;
  aDesiredSize.descent=0;

  if (nsDebugTable::gRflCell) nsTableFrame::DebugReflow("TC::Rfl ex", this, nsnull, &aDesiredSize);

  return NS_OK;
}

/**
  *
  * Update the border style to map to the HTML border style
  *
  */
void nsTableCellFrame::MapHTMLBorderStyle(nsIPresContext* aPresContext, 
                                          nsStyleSpacing& aSpacingStyle,
                                          nsTableFrame*   aTableFrame)
{
  //adjust the border style based on the table rules attribute
  const nsStyleTable* tableStyle;
  aTableFrame->GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);

  switch (tableStyle->mRules)
  {
  case NS_STYLE_TABLE_RULES_NONE:
    aSpacingStyle.SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_NONE);
    aSpacingStyle.SetBorderStyle(NS_SIDE_LEFT, NS_STYLE_BORDER_STYLE_NONE);
    aSpacingStyle.SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_NONE);
    aSpacingStyle.SetBorderStyle(NS_SIDE_RIGHT, NS_STYLE_BORDER_STYLE_NONE);
    break;

  case NS_STYLE_TABLE_RULES_COLS:
	  aSpacingStyle.SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_NONE);
    aSpacingStyle.SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_NONE);
    break;

  case NS_STYLE_TABLE_RULES_ROWS:
    aSpacingStyle.SetBorderStyle(NS_SIDE_LEFT, NS_STYLE_BORDER_STYLE_NONE);
    aSpacingStyle.SetBorderStyle(NS_SIDE_RIGHT, NS_STYLE_BORDER_STYLE_NONE);
    break;

  default:
    // do nothing for "GROUPS" or "ALL" or for any illegal value
    // "GROUPS" will be handled in nsTableFrame::ProcessGroupRules
    break;
  }
}


PRBool nsTableCellFrame::ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult)
{
  if (aValue.GetUnit() == eHTMLUnit_Pixel)
    aResult = aValue.GetPixelValue();
  else if (aValue.GetUnit() == eHTMLUnit_Empty)
    aResult = aDefault;
  else
  {
    NS_ERROR("Unit must be pixel or empty");
    return PR_FALSE;
  }
  return PR_TRUE;
}

void nsTableCellFrame::MapBorderMarginPadding(nsIPresContext* aPresContext)
{
  // Check to see if the table has either cell padding or 
  // Cell spacing defined for the table. If true, then
  // this setting overrides any specific border, margin or 
  // padding information in the cell. If these attributes
  // are not defined, the the cells attributes are used

  nsTableFrame* tableFrame;
  nsTableFrame::GetTableFrame(this, tableFrame);
  NS_ASSERTION(tableFrame,"Table must not be null");
  if (!tableFrame)
    return;

  nscoord   spacingX = tableFrame->GetCellSpacingX();
  nscoord   spacingY = tableFrame->GetCellSpacingY();

  // get the table frame style context, and from it get cellpadding, cellspacing, and border info
  const nsStyleTable* tableStyle;
  tableFrame->GetStyleData(eStyleStruct_Table, (const nsStyleStruct *&)tableStyle);
  const nsStyleSpacing* tableSpacingStyle;
  tableFrame->GetStyleData(eStyleStruct_Spacing,(const nsStyleStruct *&)tableSpacingStyle);
  nsStyleSpacing* spacingData = (nsStyleSpacing*)mStyleContext->GetMutableStyleData(eStyleStruct_Spacing);

  // Cache the border-spacing into margin and wipe out any previous 
  // margins, since CSS doesn't allow margins to be set on cells
  spacingData->mMargin.SetTop(spacingY);
  spacingData->mMargin.SetRight(spacingX);
  spacingData->mMargin.SetBottom(spacingY);
  spacingData->mMargin.SetLeft(spacingX);

  float p2t;
  aPresContext->GetPixelsToTwips(&p2t);

  // Get the table's cellpadding or use 2 pixels as the default if it is not set.
  // This assumes that ua.css does not set padding for the cell.
  nscoord defaultPadding = tableFrame->GetCellPadding();
  if (-1 == defaultPadding) { // not set in table
    defaultPadding = NSIntPixelsToTwips(1, p2t);
  }

  // if the padding is not already set, set it to the table's cellpadding
  if (eStyleUnit_Null == spacingData->mPadding.GetTopUnit()) 
    spacingData->mPadding.SetTop(defaultPadding);
  if (eStyleUnit_Null == spacingData->mPadding.GetRightUnit()) 
    spacingData->mPadding.SetRight(defaultPadding); 
  if (eStyleUnit_Null == spacingData->mPadding.GetBottomUnit())
    spacingData->mPadding.SetBottom(defaultPadding);
  if (eStyleUnit_Null == spacingData->mPadding.GetLeftUnit()) 
    spacingData->mPadding.SetLeft(defaultPadding);

  MapHTMLBorderStyle(aPresContext, *spacingData, tableFrame);

  MapVAlignAttribute(aPresContext, tableFrame);
  MapHAlignAttribute(aPresContext, tableFrame);
  
}

/* XXX: this code will not work properly until the style and layout code has been updated 
 * as outlined in Bugzilla bug report 1802 and 915 */
void nsTableCellFrame::MapVAlignAttribute(nsIPresContext* aPresContext, nsTableFrame *aTableFrame)
{
  const nsStyleText* textStyle;
  GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)textStyle);
  // check if valign is set on the cell
  //   this condition will also be true if we inherited valign from the row or rowgroup
  if (textStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated)
  {
    return; // valign is already set on this cell
  }

  // check if valign is set on the cell's COL (or COLGROUP by inheritance)
  nsTableColFrame *colFrame;
  PRInt32 colIndex;
  GetColIndex(colIndex);
  aTableFrame->GetColumnFrame(colIndex, colFrame);
  if (colFrame) {
    const nsStyleText* colTextStyle;
    colFrame->GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)colTextStyle);
    if (colTextStyle->mVerticalAlign.GetUnit() == eStyleUnit_Enumerated) {
      nsStyleText* mutableTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
      mutableTextStyle->mVerticalAlign.SetIntValue(colTextStyle->mVerticalAlign.GetIntValue(), eStyleUnit_Enumerated);
      return; // valign set from COL info
	}
  }

  // otherwise, set the vertical align attribute to the HTML default
  nsStyleText* mutableTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
  mutableTextStyle->mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_MIDDLE, eStyleUnit_Enumerated);
}

/* XXX: this code will not work properly until the style and layout code has been updated 
 * as outlined in Bugzilla bug report 1802 and 915.
 * In particular, mTextAlign has to be an nsStyleCoord, not just an int */
void nsTableCellFrame::MapHAlignAttribute(nsIPresContext* aPresContext, nsTableFrame *aTableFrame)
{
#if 0
  const nsStyleText* textStyle;
  GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)textStyle);
  // check if halign is set on the cell
  //   cells do not inherited halign from the row or rowgroup
  if (textStyle->mTextAlign.GetUnit() == eStyleUnit_Enumerated)
  {
    if (textStyle->mTextAlign.GetIntValue() != NS_STYLE_TEXT_ALIGN_DEFAULT)
      return; // valign is already set on this cell
  }

  // check if halign is set on the cell's ROW (or ROWGROUP by inheritance)
  nsIFrame *rowFrame;
  aTableFrame->GetContentParent(rowFrame);
  const nsStyleText* rowTextStyle;
  rowFrame->GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)rowTextStyle);
  if (rowTextStyle->mTextAlign.GetUnit() == eStyleUnit_Enumerated)
  {
    nsStyleText* mutableTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
    mutableTextStyle->mTextAlign.SetIntValue(rowTextStyle->mTextAlign.GetIntValue(), eStyleUnit_Enumerated);
    return; // halign set from ROW info
  }

  // check if halign is set on the cell's COL (or COLGROUP by inheritance)
  nsTableColFrame *colFrame;
  PRInt32 colIndex;
  GetColIndex(colIndex);
  aTableFrame->GetColumnFrame(colIndex, colFrame);
  if (colFrame) {
    const nsStyleText* colTextStyle;
    colFrame->GetStyleData(eStyleStruct_Text,(const nsStyleStruct *&)colTextStyle);
    if (colTextStyle->mTextAlign.GetUnit() == eStyleUnit_Enumerated)
	{
      nsStyleText* mutableTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
      mutableTextStyle->mTextAlign.SetIntValue(colTextStyle->mTextAlign.GetIntValue(), eStyleUnit_Enumerated);
      return; // halign set from COL info
	}
  }

  // otherwise, set the vertical align attribute to the HTML default (center for TH, left for TD and all others)
  nsStyleText* mutableTextStyle = (nsStyleText*)mStyleContext->GetMutableStyleData(eStyleStruct_Text);
  nsIAtom *tag=nsnull;
  if (nsnull!=mContent)
    mContent->GetTag(tag);
  if (nsHTMLAtoms::th==tag)
    mutableTextStyle->mVerticalAlign.SetIntValue(NS_STYLE_TEXT_ALIGN_CENTER, eStyleUnit_Enumerated);
  else
    mutableTextStyle->mVerticalAlign.SetIntValue(NS_STYLE_TEXT_ALIGN_LEFT, eStyleUnit_Enumerated);
#endif
}  

// Subclass hook for style post processing
NS_METHOD nsTableCellFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
#ifdef NOISY_STYLE
  printf("nsTableCellFrame::DidSetStyleContext \n");
#endif

  MapBorderMarginPadding(aPresContext);
  mStyleContext->RecalcAutomaticData(aPresContext);
  return NS_OK;
}

/* ----- global methods ----- */

NS_IMPL_ADDREF_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableCellFrame, nsHTMLContainerFrame)

nsresult nsTableCellFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(nsITableCellLayout::GetIID())) {
    *aInstancePtr = (void*) (nsITableCellLayout *)this;
    return NS_OK;
  } else {
    return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
  }
}

/* This is primarily for editor access via nsITableLayout */
NS_IMETHODIMP
nsTableCellFrame::GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex)
{
  nsresult res = GetRowIndex(aRowIndex);
  if (NS_FAILED(res))
  {
    aColIndex = 0;
    return res;
  }
  aColIndex = mColIndex;
  return  NS_OK;
}

nsresult 
NS_NewTableCellFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTableCellFrame* it = new nsTableCellFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}



/* ----- methods from CellLayoutData ----- */

/**
  * Given a frame and an edge, find the margin
  *
  **/
nscoord nsTableCellFrame::GetMargin(nsIFrame* aFrame, PRUint8 aEdge) const
{
  nscoord result = 0;

  if (aFrame)
  {
    const nsStyleSpacing* spacing;
    aFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
    nsMargin  margin;
    spacing->CalcMarginFor(aFrame, margin);
    switch (aEdge)
    {
      case NS_SIDE_TOP:
        result = margin.top;
      break;

      case NS_SIDE_RIGHT:
        result = margin.right;
      break;

      case NS_SIDE_BOTTOM:
        result = margin.bottom;
      break;

      case NS_SIDE_LEFT:
        result = margin.left;
      break;

    }
  }
  return result;
}


/**
  * Given an Edge, find the opposing edge (top<-->bottom, left<-->right)
  *
  **/
PRUint8 nsTableCellFrame::GetOpposingEdge(PRUint8 aEdge)
{
   PRUint8 result;

   switch (aEdge)
    {
   case NS_SIDE_LEFT:
        result = NS_SIDE_RIGHT;
        break;

   case NS_SIDE_RIGHT:
        result = NS_SIDE_LEFT;
        break;

   case NS_SIDE_TOP:
        result = NS_SIDE_BOTTOM;
        break;

   case NS_SIDE_BOTTOM:
        result = NS_SIDE_TOP;
        break;

      default:
        result = NS_SIDE_TOP;
     }
  return result;
}

void nsTableCellFrame::GetCellBorder(nsMargin &aBorder, nsTableFrame *aTableFrame)
{
  aBorder.left = aBorder.right = aBorder.top = aBorder.bottom = 0;
  if (nsnull==aTableFrame) {
    return;
  }

  if (NS_STYLE_BORDER_COLLAPSE==aTableFrame->GetBorderCollapseStyle()) {
    aBorder = mBorderEdges.mMaxBorderWidth;
  } else {
    const nsStyleSpacing* spacing;
    GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct*&)spacing);
    spacing->GetBorder(aBorder);
  }
}

void nsTableCellFrame::RecalcLayoutData(nsMargin& aMargin)
{
  mMargin.left   = aMargin.left;
  mMargin.top    = aMargin.top;
  mMargin.right  = aMargin.right;
  mMargin.bottom = aMargin.bottom;

  mCalculated = NS_OK;
}


NS_IMETHODIMP
nsTableCellFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::tableCellFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

NS_IMETHODIMP
nsTableCellFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("TableCell", aResult);
}

void nsTableCellFrame::SetCollapseOffsetX(nscoord aXOffset)
{
  mCollapseOffset.x = aXOffset;
}

void nsTableCellFrame::SetCollapseOffsetY(nscoord aYOffset)
{
  mCollapseOffset.y = aYOffset;
}

nsPoint nsTableCellFrame::GetCollapseOffset()
{
  return mCollapseOffset;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableCellFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  PRUint32 sum = sizeof(*this);
  // XXX write me
  *aResult = sum;
  return NS_OK;
}
#endif
