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

#include "BasicTableLayoutStrategy.h"
#include "nsTableFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsVoidArray.h"
#include "nsHTMLIIDs.h"

#if 1
static PRBool gsDebugAssign  = PR_FALSE;
static PRBool gsDebugBalance = PR_FALSE;
#else
static PRBool gsDebugAssign  = PR_TRUE;
static PRBool gsDebugBalance = PR_TRUE;
#endif
static PRInt32 gsDebugCount = 0;

PRBool CanAllocate(PRInt32          aTypeToAllocate,
                   PRInt32          aTypeAlreadyAllocated,
                   nsTableColFrame* aColFrame,
                   PRBool&          aConsiderAdj)
{
  aConsiderAdj = PR_TRUE;
  if ((aTypeToAllocate == FIX_ADJ) && (aTypeAlreadyAllocated == FIX)) {
    return PR_TRUE;
  }
  if ((aTypeToAllocate == DES_CON) && (aTypeAlreadyAllocated == FIX_ADJ)) {
    aConsiderAdj = PR_FALSE;
    return PR_TRUE;
  }
  if ((aTypeToAllocate == DES_CON) && (aTypeAlreadyAllocated == FIX) && 
      (aColFrame->GetWidth(DES_ADJ) > aColFrame->GetWidth(DES_CON))) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

/* ---------- BasicTableLayoutStrategy ---------- */

MOZ_DECL_CTOR_COUNTER(BasicTableLayoutStrategy);


BasicTableLayoutStrategy::BasicTableLayoutStrategy(nsTableFrame *aFrame, PRBool aIsNavQuirks)
{
  MOZ_COUNT_CTOR(BasicTableLayoutStrategy);
  NS_ASSERTION(nsnull != aFrame, "bad frame arg");

  mTableFrame            = aFrame;
  mMinTableContentWidth  = 0;
  mMaxTableContentWidth  = 0;
  mCellSpacingTotal      = 0;
  mIsNavQuirksMode       = aIsNavQuirks;
}

BasicTableLayoutStrategy::~BasicTableLayoutStrategy()
{
  MOZ_COUNT_DTOR(BasicTableLayoutStrategy);
}

PRBool BasicTableLayoutStrategy::Initialize(nsIPresContext* aPresContext,
                                            nsSize*         aMaxElementSize,
                                            nscoord         aMaxWidth)
{
  ContinuingFrameCheck();

  PRBool result = PR_TRUE;

  // re-init instance variables
  mMinTableContentWidth = 0;
  mMaxTableContentWidth = 0;
  mCellSpacingTotal     = 0;
  mCols                 = mTableFrame->GetEffectiveCOLSAttribute();
  // assign the width of all fixed-width columns
  AssignPreliminaryColumnWidths(aPresContext, aMaxWidth);

  // set aMaxElementSize here because we compute mMinTableWidth in AssignPreliminaryColumnWidths
  if (nsnull != aMaxElementSize) {
    SetMaxElementSize(aMaxElementSize);
  }

  return result;
}

void BasicTableLayoutStrategy::SetMaxElementSize(nsSize* aMaxElementSize)
{
  if (nsnull != aMaxElementSize) {
    aMaxElementSize->height = 0;
    nsMargin borderPadding;
    const nsStylePosition* tablePosition;
    const nsStyleSpacing* tableSpacing;
    mTableFrame->GetStyleData(eStyleStruct_Position, ((const nsStyleStruct *&)tablePosition));
    mTableFrame->GetStyleData(eStyleStruct_Spacing , ((const nsStyleStruct *&)tableSpacing));
    mTableFrame->GetTableBorder(borderPadding);
    nsMargin padding;
    tableSpacing->GetPadding(padding);
    borderPadding += padding;
    nscoord horBorderPadding = borderPadding.left + borderPadding.right;
    if (tablePosition->mWidth.GetUnit() == eStyleUnit_Coord) {
      aMaxElementSize->width = tablePosition->mWidth.GetCoordValue();
      if (mMinTableContentWidth + horBorderPadding > aMaxElementSize->width) {
        aMaxElementSize->width = mMinTableContentWidth + horBorderPadding;
      }
    }   
    else {
      aMaxElementSize->width = mMinTableContentWidth + horBorderPadding;
    }
  }
}

void BasicTableLayoutStrategy::ContinuingFrameCheck()
{
#ifdef NS_DEBUG
  nsIFrame* tablePIF = nsnull;
  mTableFrame->GetPrevInFlow(&tablePIF);
  NS_ASSERTION(!tablePIF, "never ever call me on a continuing frame!");
#endif
}

PRBool BCW_Wrapup(nsIPresContext*           aPresContext,
                  BasicTableLayoutStrategy* aStrategy, 
                  nsTableFrame*             aTableFrame, 
                  PRInt32*                  aAllocTypes)
{
  if (aAllocTypes)
    delete [] aAllocTypes;
  if (gsDebugBalance) {printf("BalanceColumnWidths ex \n"); aTableFrame->Dump(aPresContext, PR_FALSE, PR_TRUE, PR_FALSE);}
  return PR_TRUE;
}

// The priority of allocations for a given column are as follows
//   1) max(MIN, MIN_ADJ)
//   2) max (PCT, PCT_ADJ) go to 7
//   3) FIX 
//   4) FIX_ADJ, if FIX was not set do DES_CON, go to 7
//   5) PROportional go to 7 
//   6) max(DES_CON, DES_ADJ)
//   7) for a fixed width table, the column may get more 
//      space if the sum of the col allocations is insufficient 

PRBool 
BasicTableLayoutStrategy::BalanceColumnWidths(nsIPresContext*          aPresContext,
                                              nsIStyleContext*         aTableStyle,
                                              const nsHTMLReflowState& aReflowState,
                                              nscoord                  aMaxWidthIn)
{
  if (gsDebugBalance) {printf("BalanceColumnWidths en max=%d count=%d \n", aMaxWidthIn, gsDebugCount++); mTableFrame->Dump(aPresContext, PR_FALSE, PR_TRUE, PR_FALSE);}

  ContinuingFrameCheck();
  if (!aTableStyle) {
    NS_ASSERTION(aTableStyle, "bad style arg");
    return PR_FALSE;
  }

  PRInt32 numCols = mTableFrame->GetColCount();

  // determine if the table is auto/fixed and get the fixed width if available
  nscoord maxWidth = aMaxWidthIn; 
  nscoord specifiedTableWidth = mTableFrame->CalcBorderBoxWidth(aReflowState);
  PRBool tableIsAutoWidth = mTableFrame->IsAutoWidth();
  // a specifiedTableWidth of <= 0 indicates percentage based 
  if (!tableIsAutoWidth && (specifiedTableWidth > 0)) {
    maxWidth = PR_MIN(specifiedTableWidth, aMaxWidthIn); // specifiedWidth usually == aMaxWidthIn for fixed table
  }
  // reduce the maxWidth by border and padding, since we will be dealing with content width
  // XXX should this be done in aMaxWidthIn by the caller?
  if (maxWidth != NS_UNCONSTRAINEDSIZE) {
    const nsStyleSpacing* spacing;
    mTableFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
    nsMargin borderPadding;
    spacing->CalcBorderPaddingFor(mTableFrame, borderPadding);
    maxWidth -= borderPadding.left + borderPadding.right;
    maxWidth = PR_MAX(0, maxWidth);
  }
  // initialize the col percent and cell percent values to 0.
  PRInt32 colX;
  for (colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    colFrame->SetWidth(PCT, WIDTH_NOT_SET);
    colFrame->SetWidth(PCT_ADJ, WIDTH_NOT_SET);
  }
  // set PCT and PCT_ADJ widths on col frames. An auto table returns 
  // a new table width based on percent cells/cols if they exist
  nscoord perAdjTableWidth = 0;
  if ((NS_UNCONSTRAINEDSIZE != maxWidth) || (tableIsAutoWidth)) {
    // for an auto width table, use a large basis just so that the quirky
    // auto table sizing will get as big as it should
    nscoord basis = (NS_UNCONSTRAINEDSIZE == maxWidth) 
                    ? NS_UNCONSTRAINEDSIZE : maxWidth - mCellSpacingTotal;
    perAdjTableWidth = AssignPercentageColumnWidths(basis, tableIsAutoWidth);
  }

  // set the table's columns to the min width
  for (colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    nscoord colMinWidth = colFrame->GetMinWidth();
    mTableFrame->SetColumnWidth(colX, colMinWidth);
  }

  // if the max width available is less than the min content width for fixed table, we're done
  if (!tableIsAutoWidth && (maxWidth < mMinTableContentWidth)) {
    return BCW_Wrapup(aPresContext, this, mTableFrame, nsnull);
  }

  // if the max width available is less than the min content width for auto table
  // that had no % cells/cols, we're done
  if (tableIsAutoWidth && (maxWidth < mMinTableContentWidth) && (0 == perAdjTableWidth)) {
    return BCW_Wrapup(aPresContext, this, mTableFrame, nsnull);
  }

  PRInt32 cellSpacingTotal;
  // the following are of size NUM_WIDTHS, but only MIN_CON, DES_CON, FIX, FIX_ADJ, PCT
  // are used and they account for colspan ADJusted values
  PRInt32 totalWidths[NUM_WIDTHS]; // sum of col widths of a particular type 
  PRInt32 totalAvailWidths[NUM_WIDTHS]; 
  PRInt32 totalCounts[NUM_WIDTHS]; // num of cols of a particular type
  PRInt32 minWidths[NUM_WIDTHS];
  PRInt32 num0Proportional;

  CalculateTotals(cellSpacingTotal, totalCounts, totalWidths, totalAvailWidths, minWidths, num0Proportional);
  // auto width table's adjusted width needs cell spacing
  if (tableIsAutoWidth && perAdjTableWidth > 0) { 
    perAdjTableWidth = PR_MIN(perAdjTableWidth + cellSpacingTotal, maxWidth);
  }
  nscoord totalAllocated = totalWidths[MIN_CON] + cellSpacingTotal;
  
  // allocate and initialize arrays indicating what col gets set
  PRInt32* allocTypes = new PRInt32[numCols];
  if (!allocTypes) return PR_FALSE;
 
  for (colX = 0; colX < numCols; colX++) {
    allocTypes[colX] = -1;
  }

  // allocate percentage cols
  if (totalCounts[PCT] > 0) {
    if (totalAllocated + totalAvailWidths[PCT] - minWidths[PCT] <= maxWidth) {
      AllocateFully(totalAllocated, allocTypes, PCT);
      //NS_WARN_IF_FALSE(totalAllocated <= maxWidth, "over allocated");
    }
    else {
      AllocateConstrained(maxWidth - totalAllocated, PCT, PR_FALSE, allocTypes);
      return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
    }
  }
  // allocate fixed cols
  if (totalAllocated < maxWidth && totalCounts[FIX] > 0) {
    if (totalAllocated + totalAvailWidths[FIX] - minWidths[FIX] <= maxWidth) { 
      AllocateFully(totalAllocated, allocTypes, FIX);
      //NS_WARN_IF_FALSE(totalAllocated <= maxWidth, "over allocated");
    }
    else {
      AllocateConstrained(maxWidth - totalAllocated, FIX, PR_TRUE, allocTypes);
      return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
    }
  }
  // allocate fixed adjusted cols
  if (totalAllocated < maxWidth && totalCounts[FIX_ADJ] > 0) {
    if (totalAllocated + totalAvailWidths[FIX_ADJ] - minWidths[FIX_ADJ] <= maxWidth) { 
      AllocateFully(totalAllocated, allocTypes, FIX_ADJ);
      //NS_WARN_IF_FALSE(totalAllocated <= maxWidth, "over allocated");
    }
    else {
      AllocateConstrained(maxWidth - totalAllocated, FIX_ADJ, PR_TRUE, allocTypes);
      return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
    }
  }
  // allocate proportional cols up to their min proportional value
  if (totalAllocated < maxWidth && totalCounts[MIN_PRO] > 0) {
    if (totalAllocated + totalAvailWidths[MIN_PRO] - minWidths[MIN_PRO] <= maxWidth) { 
      AllocateFully(totalAllocated, allocTypes, MIN_PRO, PR_FALSE);
      //NS_WARN_IF_FALSE(totalAllocated <= maxWidth, "over allocated");
    }
    else {
      AllocateConstrained(maxWidth - totalAllocated, MIN_PRO, PR_FALSE, allocTypes);
      return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
    }
  }

  // allocate auto cols, considering even those that are proportional
  if (totalAllocated < maxWidth && totalCounts[DES_CON] > 0) {
    if (totalAllocated + totalAvailWidths[DES_CON] - minWidths[DES_CON]<= maxWidth) { 
      AllocateFully(totalAllocated, allocTypes, DES_CON);
      //NS_WARN_IF_FALSE(totalAllocated <= maxWidth, "over allocated");
    }
    else {
      AllocateConstrained(maxWidth - totalAllocated, DES_CON, PR_TRUE, allocTypes);
      return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
    }
  }

  // if this is a nested non auto table and pass1 reflow, we are done
  if ((maxWidth == NS_UNCONSTRAINEDSIZE) && (!tableIsAutoWidth))  {
    return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
  }

  // allocate the rest unconstrained
  PRBool skip0Proportional = totalCounts[DES_CON] > num0Proportional;
  if ( (tableIsAutoWidth && (perAdjTableWidth - totalAllocated > 0)) ||
       (!tableIsAutoWidth && (totalAllocated < maxWidth)) ) {
    if (totalCounts[PCT] != numCols) {
      //PRBool onlyAuto = (totalCounts[DES_CON] > 0) && !mIsNavQuirksMode;
      for (colX = 0; colX < numCols; colX++) {
        if (PCT == allocTypes[colX]) {
          allocTypes[colX] = -1;
        }
        //else if ((FIX == allocTypes[colX]) && onlyAuto) {
        //  allocTypes[colX] = -1;
        //}
      }
    }
    if (tableIsAutoWidth) {
      AllocateUnconstrained(perAdjTableWidth - totalAllocated, allocTypes, skip0Proportional);
    }
    else {
      AllocateUnconstrained(maxWidth - totalAllocated, allocTypes, skip0Proportional);
    }
  }
  // give the proportional cols the rest up to the max width in quirks mode
  else if (tableIsAutoWidth && mIsNavQuirksMode && (totalCounts[MIN_PRO] > 0)) {
    for (colX = 0; colX < numCols; colX++) {
      if (DES_CON != allocTypes[colX]) {
        allocTypes[colX] = -1;
      }
    }
    AllocateUnconstrained(maxWidth - totalAllocated, allocTypes, skip0Proportional);
  }


  return BCW_Wrapup(aPresContext, this, mTableFrame, allocTypes);
}


// Allocate aWidthType values to all cols available in aIsAllocated 
void BasicTableLayoutStrategy::AllocateFully(nscoord&      aTotalAllocated,
                                             PRInt32*      aAllocTypes,
                                             PRInt32       aWidthType,
                                             PRBool        aMarkAllocated)
{
  PRInt32 numCols = mTableFrame->GetColCount();
  for (PRInt32 colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    nscoord oldWidth = mTableFrame->GetColumnWidth(colX);
    nscoord newWidth = colFrame->GetWidth(aWidthType);

    PRBool useAdj;
    if (!CanAllocate(aWidthType, aAllocTypes[colX], colFrame, useAdj)) {
      if (-1 != aAllocTypes[colX]) {
        continue;
      }
    }
    // account for col span overrides with DES_CON and FIX
    if ((DES_CON == aWidthType) && useAdj) {
      newWidth = PR_MAX(newWidth, colFrame->GetWidth(DES_ADJ));
    }
    else if (PCT == aWidthType) {
      newWidth = PR_MAX(newWidth, colFrame->GetWidth(PCT_ADJ));
    }
    if (WIDTH_NOT_SET == newWidth) {
      continue;
    }
    if (newWidth > oldWidth) {
      mTableFrame->SetColumnWidth(colX, newWidth);
      aTotalAllocated += newWidth - oldWidth;
    }
    if (aMarkAllocated) {
      aAllocTypes[colX] = aWidthType;
    }
  }
}

void BasicTableLayoutStrategy::AllocateUnconstrained(PRInt32  aAllocAmount,
                                                     PRInt32* aAllocTypes,
                                                     PRBool   aSkip0Proportional)
{
  nscoord divisor          = 0;
  PRInt32 numColsAllocated = 0; 
  PRInt32 totalAllocated   = 0;
  PRInt32 colX;
  PRInt32 numCols = mTableFrame->GetColCount();
  for (colX = 0; colX < numCols; colX++) { 
    if (-1 != aAllocTypes[colX]) {
      divisor += mTableFrame->GetColumnWidth(colX);
      numColsAllocated++;
    }
  }
  for (colX = 0; colX < numCols; colX++) { 
    if (-1 != aAllocTypes[colX]) {
      if (aSkip0Proportional) {
        nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
        if (e0ProportionConstraint == colFrame->GetConstraint()) {
          continue;
        }
      }
      nscoord oldWidth = mTableFrame->GetColumnWidth(colX);
      float percent = (divisor == 0) 
        ? (1.0f / ((float)numColsAllocated))
        : ((float)oldWidth) / ((float)divisor);
      nscoord addition = NSToCoordRound(((float)aAllocAmount) * percent);
      if (addition > (aAllocAmount - totalAllocated)) {
        mTableFrame->SetColumnWidth(colX, oldWidth + (aAllocAmount - totalAllocated));
        break;
      }
      mTableFrame->SetColumnWidth(colX, oldWidth + addition);
      totalAllocated += addition;
    }
  }
}

nscoord GetConstrainedWidth(nsTableColFrame* colFrame,
                            PRBool           aConsiderPct)
                           
{
  nscoord conWidth = WIDTH_NOT_SET;
  if (aConsiderPct) {
    conWidth = colFrame->GetPctWidth();
  }
  if (conWidth <= 0 ) {
    conWidth = colFrame->GetFixWidth();
  }
  return conWidth;
}

#define LIMIT_DES  0
#define LIMIT_CON  1
#define LIMIT_NONE 2

void 
BasicTableLayoutStrategy::ComputeColspanWidths(PRInt32           aWidthIndex,
                                               nsTableCellFrame* aCellFrame,
                                               PRInt32           aColIndex,
                                               PRInt32           aColSpan,
                                               PRBool            aConsiderPct)
{
  if (!aCellFrame || (aColIndex < 0) || (aColIndex < 0) || (aColSpan < 0)) {
    NS_ASSERTION(PR_FALSE, "ComputeColspanWidths called incorrectly");
    return;
  }

  nscoord cellWidth = 0;
  if (MIN_CON == aWidthIndex) {
    cellWidth = aCellFrame->GetPass1MaxElementSize().width;
  }
  else if (DES_CON == aWidthIndex) {
    cellWidth = aCellFrame->GetMaximumWidth();
  }
  else { // FIX width
    // see if the cell has a style width specified
    const nsStylePosition* cellPosition;
    aCellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
    if (eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) {
      // need to add padding into fixed width
      const nsStyleSpacing* spacing;
      aCellFrame->GetStyleData(eStyleStruct_Spacing,(const nsStyleStruct *&)spacing);
      nsMargin paddingMargin;
      spacing->CalcPaddingFor(aCellFrame, paddingMargin); 
      nscoord padding = paddingMargin.left + paddingMargin.right;
      cellWidth = cellPosition->mWidth.GetCoordValue() + padding;
      cellWidth = PR_MAX(cellWidth, aCellFrame->GetPass1MaxElementSize().width);
    }
  }

  if (0 >= cellWidth) { 
    return;
  }

  if (MIN_CON == aWidthIndex) {
    // for min width, first allocate fixed cells up to their fixed value, then if 
    // necessary allocate auto cells up to their auto value, then if necessary
    // allocate auto cells. 
    for (PRInt32 limitX = LIMIT_CON; limitX <= LIMIT_NONE; limitX++) { 
      if (ComputeColspanWidths(aWidthIndex, aCellFrame, cellWidth, 
                               aColIndex, aColSpan, aConsiderPct, limitX)) {
        return;
      }
    }
  } else {
    // for des width, just allocate auto cells
    ComputeColspanWidths(aWidthIndex, aCellFrame, cellWidth, 
                         aColIndex, aColSpan, aConsiderPct, LIMIT_NONE);
  }
}


PRBool 
BasicTableLayoutStrategy::ComputeColspanWidths(PRInt32           aWidthIndex,
                                               nsTableCellFrame* aCellFrame,
                                               nscoord           aCellWidth,
                                               PRInt32           aColIndex,
                                               PRInt32           aColSpan,
                                               PRBool            aConsiderPct,
                                               PRInt32           aLimitType)
{
  PRBool result = PR_TRUE;
  // skip DES_CON if there is a FIX since FIX is really the desired size
  //if ((DES_CON == widthX) && (cellWidths[FIX] > 0)) 
  nscoord spanCellSpacing = 0; // total cell spacing cells being spanned
  nscoord spanTotal       = 0; // total width of type aWidthIndex of spanned cells
  nscoord divisorCon      = 0; // the sum of constrained (fix or pct if specified) widths of spanned cells
  nscoord divisorDes      = 0; // the sum of desired widths
  // the following are only used for MIN_CON calculations. Takings differences between
  // actual and target values allows target values to be reached and not exceeded. This
  // is not as accurate as the method in AllocateConstrained, but it is a lot cheaper.
  nscoord divisorConLimit = 0; // the sum of differences between constrained width and min width
  nscoord divisorDesLimit = 0; // the sum of differences between desired width and min width

  nscoord spacingX = mTableFrame->GetCellSpacingX();
  PRInt32 spanX;
  // accumulate the various divisors to be used later
  for (spanX = 0; spanX < aColSpan; spanX++) {
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(aColIndex + spanX);
    nscoord colWidth = PR_MAX(colFrame->GetWidth(aWidthIndex), 
                              colFrame->GetWidth(aWidthIndex + NUM_MAJOR_WIDTHS));
    colWidth = PR_MAX(colWidth, 0);
    if (aWidthIndex == DES_CON) {
      nscoord conWidth = GetConstrainedWidth(colFrame, aConsiderPct); 
      if (conWidth <= 0) {
        divisorDes += colWidth;
        spanTotal  += colWidth;
      }
      else {
        divisorCon += conWidth;
        spanTotal  += conWidth;
      }
    }
    else if (aWidthIndex == MIN_CON) {
      nscoord conWidth  = GetConstrainedWidth(colFrame, aConsiderPct);
      nscoord desWidth  = colFrame->GetDesWidth();
      if (conWidth > 0) {
        divisorCon += conWidth;
        divisorConLimit += PR_MAX(conWidth - colWidth, 0);
      }
      else {
        divisorDes += desWidth;
        divisorDesLimit += PR_MAX(desWidth - colWidth, 0);
      }
      spanTotal += colWidth;
    }
    else { // FIX width
      if (colWidth > 0) {
        divisorCon += colWidth;
      }
      else {
        divisorDes += colFrame->GetDesWidth();
      }
      spanTotal += colWidth;
    }
    if ((spanX > 0) && (mTableFrame->GetNumCellsOriginatingInCol(aColIndex + spanX) > 0)) {
      spanCellSpacing += spacingX;
    }
  }

  if (MIN_CON != aWidthIndex) {
    aLimitType = LIMIT_NONE; // just in case we were called incorrectly
  }
  else {
    // this method gets called up to 3 times, first to let fixed cols reach their
    // target, 2nd to let auto cols reach their target and 3rd to let auto cols
    // fill out the remainder. Below are some optimizations which can skip steps.

    // if there are no constrained cols to focus on, focus on auto cols
    if ((LIMIT_CON == aLimitType) && (0 == divisorConLimit)) {
      aLimitType = LIMIT_DES;
    }
    // if there are no auto cols to focus on, focus on nothing
    if ((LIMIT_DES == aLimitType) && (0 == divisorDesLimit)) {
      aLimitType = LIMIT_NONE;
    }
  }

  nscoord availWidth = aCellWidth - spanTotal - spanCellSpacing;
  // there are 2 cases where the target will not be reached
  if ((LIMIT_CON == aLimitType) && (divisorConLimit < availWidth)) {
    availWidth = divisorConLimit;
    result = PR_FALSE;
  }
  if ((LIMIT_DES == aLimitType) && (divisorDesLimit < availWidth)) {
    availWidth = divisorDesLimit;
    result = PR_FALSE;
  }
  if (availWidth > 0) {
    // get the correct numerator in a similar fashion to getting the divisor
    for (spanX = 0; spanX < aColSpan; spanX++) {
      nsTableColFrame* colFrame = mTableFrame->GetColFrame(aColIndex + spanX);
      nscoord colWidth = PR_MAX(colFrame->GetWidth(aWidthIndex), 
                                colFrame->GetWidth(aWidthIndex + NUM_MAJOR_WIDTHS));
      nscoord minWidth = colFrame->GetMinWidth();

      colWidth = PR_MAX(colWidth, 0);
      nscoord numeratorDes      = 0;
      nscoord numeratorCon      = 0;
      nscoord numeratorConLimit = 0;
      nscoord numeratorDesLimit = 0;
      if (aWidthIndex == DES_CON) {
        nscoord conWidth = GetConstrainedWidth(colFrame, aConsiderPct);
        if (conWidth <= 0) {
          numeratorDes = colWidth;
        }
        else {
          numeratorCon = conWidth;
        }
      }
      else if (aWidthIndex == MIN_CON) {
        nscoord conWidth  = GetConstrainedWidth(colFrame, aConsiderPct);
        nscoord desWidth  = colFrame->GetDesWidth();
        if (conWidth > 0) {
          numeratorCon = conWidth;
          numeratorConLimit = PR_MAX(conWidth - colWidth, 0);
        }
        else {
          numeratorDes = desWidth;
          numeratorDesLimit = PR_MAX(desWidth - colWidth, 0);
        }
      }
      else { // FIX width
        if (colWidth <= 0) {
          numeratorDes = colFrame->GetDesWidth();
        }
        else {
          numeratorCon = colWidth;
        }
      }

      nscoord divisor;
      nscoord numerator;
      if (divisorDes > 0) { // there were auto cols
        divisor   = divisorDes;
        numerator = numeratorDes;
      }
      else {                // there were only constrained cols
        divisor   = divisorCon;
        numerator = numeratorCon;
      }
      // let constrained cols reach their target 
      if (LIMIT_CON == aLimitType) {
        if (divisorConLimit > 0) {
          divisor   = divisorConLimit;
          numerator = numeratorConLimit;
        }
      }
      // let auto cols reach their target 
      else if (LIMIT_DES == aLimitType) {
        if (divisorDesLimit > 0) {
          divisor   = divisorDesLimit;
          numerator = numeratorDesLimit;
        }
      }

      // calculate the amount of additional width the col will get
      nscoord addition = (0 >= divisor) 
        ? NSToCoordRound( ((float)availWidth) / ((float)aColSpan) )
        : NSToCoordRound( (((float)numerator) / ((float)divisor)) * availWidth);
      if (addition > 0) {
        nscoord newColAdjWidth = colWidth + addition;
        if (newColAdjWidth > minWidth) {
          if (FIX == aWidthIndex) {
            // a colspan cannot fix a column below what a cell desires on its own
            nscoord desCon = colFrame->GetWidth(DES_CON); // do not consider DES_ADJ
            if (desCon > newColAdjWidth) {
              newColAdjWidth = desCon;
            }
          }
          colFrame->SetWidth(aWidthIndex + NUM_MAJOR_WIDTHS, newColAdjWidth);
          colFrame->SetConstrainingCell(aCellFrame); // XXX is this right?
        }
      }
    }
  }
  return result;
}

// Determine min, desired, fixed, and proportional sizes for the cols and 
// and calculate min/max table width
PRBool BasicTableLayoutStrategy::AssignPreliminaryColumnWidths(nsIPresContext* aPresContext,
                                                               nscoord         aMaxWidth)
{
  if (gsDebugAssign) {printf("AssignPrelimColWidths en max=%d count=%d \n", aMaxWidth, gsDebugCount++); mTableFrame->Dump(aPresContext, PR_FALSE, PR_TRUE, PR_FALSE);}
  PRBool rv = PR_FALSE;
  PRInt32 numRows = mTableFrame->GetRowCount();
  PRInt32 numCols = mTableFrame->GetColCount();
  nscoord spacingX = mTableFrame->GetCellSpacingX();
  PRInt32 colX, rowX; 
  mCellSpacingTotal = 0;

  PRInt32 propTotal    = 0; // total of numbers of the type 1*, 2*, etc 
  PRInt32 numColsForColsAttr = 0; // Nav Quirks cols attribute for equal width cols
  if (NS_STYLE_TABLE_COLS_NONE != mCols) {
    numColsForColsAttr = (NS_STYLE_TABLE_COLS_ALL == mCols) ? numCols : mCols;
  }

  // For every column, determine it's min and desired width based on cell style
  // base on cells which do not span cols. Also, determine mCellSpacingTotal
  for (colX = 0; colX < numCols; colX++) { 
    nscoord minWidth = 0;
    nscoord desWidth = 0;
    nscoord fixWidth = WIDTH_NOT_SET;
    
    // Get column frame and reset it
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    NS_ASSERTION(nsnull != colFrame, "bad col frame");
    colFrame->ResetSizingInfo();

    if (mTableFrame->GetNumCellsOriginatingInCol(colX) > 0) {
      mCellSpacingTotal += spacingX;
    }

    // Scan the cells in the col that have colspan = 1 and find the maximum
    // min, desired, and fixed cells.
    nsTableCellFrame* fixContributor = nsnull;
    nsTableCellFrame* desContributor = nsnull;
    for (rowX = 0; rowX < numRows; rowX++) {
      PRBool originates;
      PRInt32 colSpan;
      nsTableCellFrame* cellFrame = mTableFrame->GetCellInfoAt(rowX, colX, &originates, &colSpan);
      // skip cells that don't originate at (rowX, colX); colspans are handled in the
      // next pass, row spans don't need to be handled
      if (!cellFrame || !originates || (colSpan > 1)) { 
        continue;
      }
      // these values include borders and padding
      minWidth = PR_MAX(minWidth, cellFrame->GetPass1MaxElementSize().width);
      nscoord cellDesWidth = cellFrame->GetMaximumWidth();
      if (cellDesWidth > desWidth) {
        desContributor = cellFrame;
        desWidth = cellDesWidth;
      }
      // see if the cell has a style width specified
      const nsStylePosition* cellPosition;
      cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
      if (eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) {
        nscoord coordValue = cellPosition->mWidth.GetCoordValue();
        if (coordValue > 0) { // ignore if width == 0
          // need to add padding into fixed width
          const nsStyleSpacing* spacing;
          cellFrame->GetStyleData(eStyleStruct_Spacing,(const nsStyleStruct *&)spacing);
          nsMargin paddingMargin;
          spacing->CalcPaddingFor(cellFrame, paddingMargin); 
          nscoord newFixWidth = coordValue + paddingMargin.left + paddingMargin.right;
          // 2nd part of condition is Nav Quirk like below
          if ((newFixWidth > fixWidth) || ((newFixWidth == fixWidth) && (desContributor == cellFrame))) {
            fixWidth = newFixWidth;
            fixContributor = cellFrame;
          }
          fixWidth = PR_MAX(fixWidth, minWidth);
        }
      }
    }

    desWidth = PR_MAX(desWidth, minWidth);

    // Nav Quirk like above
    if ((fixWidth > 0) && (desWidth > fixWidth) && (fixContributor != desContributor)) {
      fixWidth = WIDTH_NOT_SET;
      fixContributor = nsnull;
    }

    // cache the computed column info
    colFrame->SetWidth(MIN_CON, minWidth);
    colFrame->SetWidth(DES_CON, desWidth);
    colFrame->SetConstrainingCell(fixContributor);
    if (fixWidth > 0) {
      colFrame->SetWidth(FIX, PR_MAX(fixWidth, minWidth));
    }

    // determine if there is a proportional column either from html4 
    // proportional width on a col or Nav Quirks cols attr
    if (fixWidth <= 0) {
      nscoord proportion = WIDTH_NOT_SET;
      nsStyleCoord colStyleWidth = colFrame->GetStyleWidth();
      if (eStyleUnit_Proportional == colStyleWidth.GetUnit()) {
        proportion = colStyleWidth.GetIntValue();
      }
      else if (colX < numColsForColsAttr) {
        proportion = 1;
        if ((eStyleUnit_Percent == colStyleWidth.GetUnit()) &&
            (colStyleWidth.GetPercentValue() > 0.0f)) {
          proportion = WIDTH_NOT_SET;
        }
      }
      if (proportion >= 0) {
        colFrame->SetWidth(MIN_PRO, proportion);
        if (proportion > 0) {
          propTotal += proportion;
          colFrame->SetConstraint(eProportionConstraint);
        }
        else {
          colFrame->SetConstraint(e0ProportionConstraint);
          // override the desired, proportional widths
          nscoord colMinWidth = colFrame->GetWidth(MIN_CON);
          colFrame->SetWidth(DES_CON, colMinWidth);
          colFrame->SetWidth(MIN_PRO, 0);
        }
      }
    }
  }
  if (mCellSpacingTotal > 0) {
    mCellSpacingTotal += spacingX; // add last cell spacing on right
  }

  // figure the proportional width for porportional cols
  if (propTotal > 0)  {
    nscoord minPropTotal = 0;
    nscoord desPropTotal = 0;
    // figure the totals of all proportional cols which support every min and desired width
    for (colX = 0; colX < numCols; colX++) { 
      nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
      nscoord colProp = colFrame->GetWidth(MIN_PRO);
      if (colProp > 0) {
        nscoord minWidth = colFrame->GetWidth(MIN_CON);
        nscoord desWidth = colFrame->GetWidth(DES_CON);
        minPropTotal = PR_MAX(minPropTotal, NSToCoordRound(((float)propTotal * minWidth) / (float)colProp));
        desPropTotal = PR_MAX(desPropTotal, NSToCoordRound(((float)propTotal * desWidth) / (float)colProp));
      }
    }
    // store a ratio to allow going from a min to a desired proportional width
    if (minPropTotal > 0) {
      mMinToDesProportionRatio = ((float)desPropTotal) / ((float)minPropTotal);
    }
    // figure the cols proportional min width based on the new totals
    for (colX = 0; colX < numCols; colX++) { 
      nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
      nscoord colProp = colFrame->GetWidth(MIN_PRO);
      if (colProp > 0) {
        nscoord minProp = NSToCoordRound(((float)colProp * minPropTotal) / (float)propTotal);
        colFrame->SetWidth(MIN_PRO, minProp);
        colFrame->SetWidth(DES_CON, NSToCoordRound(((float)minProp) * mMinToDesProportionRatio));
      }
    }
  }

  // For each col, consider the cells originating in it with colspans > 1.
  // Adjust the cols that each cell spans if necessary. Iterate backwards 
  // so that nested and/or overlaping col spans handle the inner ones first, 
  // ensuring more accurated calculations.
  for (colX = numCols - 1; colX >= 0; colX--) { 
    for (rowX = 0; rowX < numRows; rowX++) {
      PRBool originates;
      PRInt32 colSpan;
      nsTableCellFrame* cellFrame = mTableFrame->GetCellInfoAt(rowX, colX, &originates, &colSpan);
      if (!originates || (1 == colSpan)) {
        continue;
      }
      // set MIN_ADJ, DES_ADJ, FIX_ADJ
      for (PRInt32 widthX = 0; widthX < NUM_MAJOR_WIDTHS; widthX++) {
        ComputeColspanWidths(widthX, cellFrame, colX, colSpan, PR_FALSE);
      }
    }
  }

  // Set the col's fixed width if present 
  // Set the table col width for each col to the content min. 
  for (colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    nscoord fixColWidth = colFrame->GetWidth(FIX);
    // use the style width of a col only if the col hasn't gotten a fixed width from any cell
    if (fixColWidth <= 0) {
      nsStyleCoord colStyleWidth = colFrame->GetStyleWidth();
      if (eStyleUnit_Coord == colStyleWidth.GetUnit()) {
        fixColWidth = colStyleWidth.GetCoordValue();
        if (fixColWidth > 0) {
          colFrame->SetWidth(FIX, fixColWidth);
        }
      }
    }
    nscoord minWidth = colFrame->GetMinWidth();
    mTableFrame->SetColumnWidth(colX, minWidth);
  }
  SetMinAndMaxTableContentWidths();

  if (gsDebugAssign) {printf("AssignPrelimColWidths ex\n"); mTableFrame->Dump(aPresContext, PR_FALSE, PR_TRUE, PR_FALSE);}
  return rv;
}

// Determine percentage col widths for each col frame
nscoord BasicTableLayoutStrategy::AssignPercentageColumnWidths(nscoord aBasisIn,
                                                               PRBool  aTableIsAutoWidth)
{
  PRInt32 numRows = mTableFrame->GetRowCount();
  PRInt32 numCols = mTableFrame->GetColCount();
  nscoord spacingX = mTableFrame->GetCellSpacingX();
  PRInt32 colX, rowX; 
  nscoord basis; // basis to use for percentage based calculations
  if (!aTableIsAutoWidth) {
    if (NS_UNCONSTRAINEDSIZE == aBasisIn) {
      return 0; // don't do the calculations on unconstrained basis
    }
    basis = aBasisIn;
  }
  else {
    // For an auto table, determine the potentially new percent adjusted width based 
    // on percent cells/cols. This probably should only be a NavQuirks thing, since
    // a percentage based cell or column on an auto table should force the column to auto
    basis = 0;                 
    nscoord fixWidthTotal = 0; // total of fixed widths of all cols
    float perTotal = 0.0f;     // total of percentage constrained cols and/or cells in cols
    PRInt32 numPerCols = 0;    // number of colums that have percentage constraints
    nscoord prefWidthTotal = 0;// total of des/fix widths of cols that don't have percentage constraints
    for (colX = 0; colX < numCols; colX++) { 
      nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
      nscoord colBasis = -1;
      // Scan the cells in the col 
      for (rowX = 0; rowX < numRows; rowX++) {
        PRBool originates;
        PRInt32 colSpan;
        nsTableCellFrame* cellFrame = mTableFrame->GetCellInfoAt(rowX, colX, &originates, &colSpan);
        if (!originates) { // skip  cells that don't originate in the col
          continue;
        }
        // see if the cell has a style percent width specified
        const nsStylePosition* cellPosition;
        cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
        if (eStyleUnit_Percent == cellPosition->mWidth.GetUnit()) {
          float percent = cellPosition->mWidth.GetPercentValue();
          colBasis = 0;
          if (percent > 0.0f) {
            // calculate the preferred width of the cell based on fixWidth and desWidth
            nscoord cellDesWidth  = 0;
            for (PRInt32 spanX = 0; spanX < colSpan; spanX++) {
              nsTableColFrame* spanFrame = mTableFrame->GetColFrame(colX + spanX);
              cellDesWidth += spanFrame->GetDesWidth();
            }
            // figure the basis using the cell's desired width and percent
            colBasis = NSToCoordRound((float)cellDesWidth / percent);
            perTotal += percent;
          }
        }
      }
      if (-1 == colBasis) {
        // see if the col has a style percent width specified
        nsStyleCoord colStyleWidth = colFrame->GetStyleWidth();
        if (eStyleUnit_Percent == colStyleWidth.GetUnit()) {
          float percent = colStyleWidth.GetPercentValue();
          colBasis = 0;
          if (percent > 0.0f) {
            nscoord desWidth = colFrame->GetDesWidth();
            colBasis = NSToCoordRound((float)desWidth / percent);
          }
        }
      }
      basis = PR_MAX(basis, colBasis);
      nscoord fixWidth = colFrame->GetFixWidth();
      fixWidthTotal += fixWidth;
      if (colBasis >= 0) {
        numPerCols++;
      }
      else {
        prefWidthTotal += (fixWidth > 0) ? fixWidth : colFrame->GetDesWidth();
      }
    } // end for (colX ..
    // If there are no pct cells or cols, there is nothing to do.
    if (0 == numPerCols) {
      return 0;
    }
    // If there is only one col and it is % based, it won't affect anything
    if ((1 == numCols) && (numCols == numPerCols)) {
      return 0;
    }
    // compute a basis considering total percentages and the desired width of everything else
    if (perTotal < 1.0f) {
      if (perTotal > 0.0f) {
        nscoord otherBasis = NSToCoordRound((float)prefWidthTotal / (1.0f - perTotal));
        basis = PR_MAX(basis, otherBasis);
      }
    }
    else if ((prefWidthTotal > 0) && (NS_UNCONSTRAINEDSIZE != aBasisIn)) { // make the basis as big as possible 
      basis = aBasisIn;
    }

    basis = PR_MAX(basis, fixWidthTotal);
    basis = PR_MIN(basis, aBasisIn); 
  }

  nscoord colPctTotal = 0;
  nscoord* colPcts = new nscoord[numCols];
  if (!colPcts) return 0;
  if (0 == basis) return 0;
  
  // Determine the percentage contribution for cols and for cells with colspan = 1
  // Iterate backwards, similarly to the reasoning in AssignPreliminaryColumnWidths
  for (colX = numCols - 1; colX >= 0; colX--) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    nscoord maxColPctWidth = WIDTH_NOT_SET;
    float maxColPct = 0.0f;
    colPcts[colX] = 0;

    nsTableCellFrame* percentContributor = nsnull;
    // Scan the cells in the col that have colspan = 1; assign PER widths
    for (rowX = 0; rowX < numRows; rowX++) {
      PRBool originates;
      PRInt32 colSpan;
      nsTableCellFrame* cellFrame = mTableFrame->GetCellInfoAt(rowX, colX, &originates, &colSpan);
      // skip cells that don't originate at (rowX, colX); colspans are handled in the
      // next pass, row spans don't need to be handled
      if (!cellFrame || !originates || (colSpan > 1)) { 
        continue;
      }
      // see if the cell has a style percent width specified
      const nsStylePosition* cellPosition;
      cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
      if (eStyleUnit_Percent == cellPosition->mWidth.GetUnit()) {
        float percent = cellPosition->mWidth.GetPercentValue();
        if (percent > maxColPct) {
          maxColPct = percent;
          maxColPctWidth = NSToCoordRound( ((float)basis) * maxColPct );
          percentContributor = cellFrame;
          if (!mIsNavQuirksMode) { 
            // need to add padding 
            const nsStyleSpacing* spacing;
            cellFrame->GetStyleData(eStyleStruct_Spacing,(const nsStyleStruct *&)spacing);
            nsMargin paddingMargin;
            spacing->CalcPaddingFor(cellFrame, paddingMargin); 
            maxColPctWidth += paddingMargin.left + paddingMargin.right;
          }
        }
      }
    }
    if (WIDTH_NOT_SET == maxColPctWidth) {
      // see if the col has a style percent width specified
      nsStyleCoord colStyleWidth = colFrame->GetStyleWidth();
      if (eStyleUnit_Percent == colStyleWidth.GetUnit()) {
        maxColPct = colStyleWidth.GetPercentValue();
        maxColPctWidth = NSToCoordRound( ((float)basis) * maxColPct );
      }
    }
    // conflicting pct/fixed widths are recorded. Nav 4.x may be changing the
    // fixed width value if it exceeds the pct value and not recording the pct
    // value. This is not being done and IE5 doesn't do it either.
    if (maxColPctWidth > 0) {
      nscoord minWidth = colFrame->GetMinWidth();
      if (minWidth > maxColPctWidth) {
        maxColPctWidth = minWidth;
        colPcts[colX] = NSToCoordRound( 100.0f * ((float)maxColPctWidth) / ((float)basis) );
      }
      else {
        colPcts[colX] = NSToCoordRound(maxColPct * 100.0f);
      }
      colFrame->SetWidth(PCT, maxColPctWidth);
      colFrame->SetConstrainingCell(percentContributor);
      colPctTotal += colPcts[colX];
    }
  }
  
  // For each col, consider the cells originating in it with colspans > 1.
  // Adjust the cols that each cell spans if necessary.
  for (colX = 0; colX < numCols; colX++) { 
    for (rowX = 0; rowX < numRows; rowX++) {
      PRBool originates;
      PRInt32 colSpan;
      nsTableCellFrame* cellFrame = mTableFrame->GetCellInfoAt(rowX, colX, &originates, &colSpan);
      if (!originates || (1 == colSpan)) {
        continue;
      }
      nscoord cellPctWidth = WIDTH_NOT_SET;
      // see if the cell has a style percentage width specified
      const nsStylePosition* cellPosition;
      cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
      float cellPct = 0.0f;
      if (eStyleUnit_Percent == cellPosition->mWidth.GetUnit()) {
        cellPct = cellPosition->mWidth.GetPercentValue();
        cellPctWidth = NSToCoordRound( ((float)basis) * cellPct );
        if (!mIsNavQuirksMode) { 
          // need to add padding 
          const nsStyleSpacing* spacing;
          cellFrame->GetStyleData(eStyleStruct_Spacing,(const nsStyleStruct *&)spacing);
          nsMargin paddingMargin;
          spacing->CalcPaddingFor(cellFrame, paddingMargin); 
          cellPctWidth += paddingMargin.left + paddingMargin.right;
        }
      }
      if (cellPctWidth > 0) {
        nscoord spanCellSpacing = 0;
        nscoord spanTotal = 0;
        nscoord colPctWidthTotal = 0;
        // accumulate the spanTotal as the max of MIN, DES, FIX, PCT
        PRInt32 spanX;
        for (spanX = 0; spanX < colSpan; spanX++) {
          nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX + spanX);
          nscoord colPctWidth = colFrame->GetWidth(PCT);
          if (colPctWidth > 0) { // skip pct cols
            colPctWidthTotal += colPctWidth;
            continue;
          }
          nscoord colWidth = PR_MAX(colFrame->GetMinWidth(), colFrame->GetFixWidth());
          colWidth = PR_MAX(colWidth, colFrame->GetDesWidth()); // XXX check this
          //colWidth = PR_MAX(colWidth, colFrame->GetPctWidth());
          spanTotal += colWidth;
          if ((spanX > 0) && (mTableFrame->GetNumCellsOriginatingInCol(colX + spanX) > 0)) {
            spanCellSpacing += spacingX;
          }
        }
        cellPctWidth += spanCellSpacing; // add it back in since it was subtracted from aBasisIn
        if (cellPctWidth <= 0) {
          continue;
        }
        if (colPctWidthTotal < cellPctWidth) { 
          // record the percent contributions for the spanned cols
          for (spanX = 0; spanX < colSpan; spanX++) {
            nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX + spanX);
            if (colFrame->GetWidth(PCT) > 0) { // skip pct cols
              continue;
            }
            nscoord minWidth = colFrame->GetMinWidth();
            nscoord colWidth = PR_MAX(minWidth, colFrame->GetFixWidth());
            colWidth = PR_MAX(colWidth, colFrame->GetDesWidth()); // XXX check this
            //float colPctAdj = (0 == spanTotal) 
            //  ? cellPctWidth / ((float) colSpan)
            //  : cellPct * ((float)colWidth) / (float)spanTotal;
            float avail = (float)PR_MAX(cellPctWidth - colPctWidthTotal, 0);
            float colPctAdj = (0 == spanTotal) 
              ? avail / ((float) colSpan)
              : (avail / (float)basis) * (((float)colWidth) / (float)spanTotal);
            if (colPctAdj > 0) {
              nscoord colPctAdjWidth = colFrame->GetWidth(PCT_ADJ);
              nscoord newColPctAdjWidth = NSToCoordRound(colPctAdj * (float)basis);
              if (newColPctAdjWidth > colPctAdjWidth) {
                if (colPctAdjWidth > 0) { // remove its contribution
                  colPctTotal -= colPcts[colX + spanX];
                }
                if (minWidth > newColPctAdjWidth) {
                  newColPctAdjWidth = minWidth;
                  colPcts[colX + spanX] = NSToCoordRound( 100.0f * ((float)newColPctAdjWidth) / ((float)basis) );
                }
                else {
                  colPcts[colX + spanX] = NSToCoordRound( 100.0f * colPctAdj );
                }
                if (newColPctAdjWidth > colFrame->GetWidth(PCT)) {
                  colFrame->SetWidth(PCT_ADJ, newColPctAdjWidth);
                  colFrame->SetConstrainingCell(cellFrame);
                }
                colPctTotal += colPcts[colX + spanX];
              }
            }
          }
        }
      }
    } // end for (rowX ..
  } // end for (colX ..

  // if the percent total went over 100%, adjustments need to be made to right most cols
  if (colPctTotal > 100) {
    for (colX = numCols - 1; colX >= 0; colX--) {
      if (colPcts[colX] > 0) {
        nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
        nscoord newPct = colPcts[colX] - (colPctTotal - 100);
        if (newPct > 0) { // this col has enough percent alloc to handle it
          nscoord newPctWidth = NSToCoordRound( ((float)basis) * ((float)newPct) / 100.0f );
          newPctWidth = PR_MAX(newPctWidth, colFrame->GetMinWidth());
          // since we don't care which one contributed, set both
          colFrame->SetWidth(PCT, newPctWidth);
          colFrame->SetWidth(PCT_ADJ, newPctWidth);
          break;
        }
        else { // this col cannot handle all the reduction, reduce it down to zero
          colFrame->SetWidth(PCT,     WIDTH_NOT_SET);
          colFrame->SetWidth(PCT_ADJ, WIDTH_NOT_SET);
          colPctTotal -= colPcts[colX];
          if (colPctTotal <= 100) {
            break;
          }
        }
      }
    }
  }

  delete [] colPcts;
  return basis;
}

void BasicTableLayoutStrategy::SetMinAndMaxTableContentWidths()
{
  mMinTableContentWidth = 0;
  mMaxTableContentWidth = 0;

  nscoord spacingX = mTableFrame->GetCellSpacingX();
  PRInt32 numCols = mTableFrame->GetColCount();
  for (PRInt32 colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    mMinTableContentWidth += colFrame->GetMinWidth();
    mMaxTableContentWidth += PR_MAX(colFrame->GetDesWidth(), colFrame->GetFixWidth());
    if (mTableFrame->GetNumCellsOriginatingInCol(colX) > 0) {
      mMaxTableContentWidth += spacingX;
      mMinTableContentWidth += spacingX;
    }
  }
  // if it is not a degenerate table, add the last spacing on the right
  if (mMinTableContentWidth > 0) {
    mMinTableContentWidth += spacingX;
    mMaxTableContentWidth += spacingX;
  }
}

// calculate totals by width type. 
void BasicTableLayoutStrategy::CalculateTotals(PRInt32& aCellSpacing,
                                               PRInt32* aTotalCounts,
                                               PRInt32* aTotalWidths,
                                               PRInt32* aTotalAvailWidths,
                                               PRInt32* aMinWidths,
                                               PRInt32& a0ProportionalCount)
{
  //mTableFrame->Dump(PR_TRUE, PR_FALSE);
  aCellSpacing = 0;
  for (PRInt32 widthX = 0; widthX < NUM_WIDTHS; widthX++) {
    aTotalCounts[widthX]      = 0;
    aTotalWidths[widthX]      = 0;
    aTotalAvailWidths[widthX] = 0;
    aMinWidths[widthX]        = 0;
  }
  a0ProportionalCount = 0;

  nscoord spacingX = mTableFrame->GetCellSpacingX();
  PRInt32 numCols = mTableFrame->GetColCount();
  for (PRInt32 colX = 0; colX < numCols; colX++) { 
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    if (mTableFrame->GetNumCellsOriginatingInCol(colX) > 0) {
      aCellSpacing += spacingX;
    }
    nscoord minCol = colFrame->GetMinWidth();
    aTotalCounts[MIN_CON]++;
    aTotalWidths[MIN_CON] += minCol;

    if (e0ProportionConstraint == colFrame->GetConstraint()) {
      a0ProportionalCount++;
    }

    nscoord pct    = colFrame->GetPctWidth();
    nscoord fix    = colFrame->GetWidth(FIX);
    nscoord fixAdj = colFrame->GetWidth(FIX_ADJ);
    // if there is a pct width then no others are considered
    if (pct > 0) {
      aTotalCounts[PCT]++;
      aTotalWidths[PCT] += pct;
      aTotalAvailWidths[PCT] += pct;
      aMinWidths[PCT] += minCol;
      continue;
    }
    if ((fix > 0) || (fixAdj > 0)) {
      if (fix > 0) {
        aTotalCounts[FIX]++;
        aTotalWidths[FIX] += fix;
        aTotalAvailWidths[FIX] += fix;
        aMinWidths[FIX] += minCol;
      }
      nscoord desCon = colFrame->GetWidth(DES_CON);
      if (fixAdj > 0) {
        if (fixAdj > fix) {
          aTotalCounts[FIX_ADJ]++;
          aTotalWidths[FIX_ADJ] += fixAdj;
          aTotalAvailWidths[FIX_ADJ] += fixAdj;
          if (fix > 0) {
            aTotalAvailWidths[FIX_ADJ] -= fix;
          }
          else { // there was no fix
            aMinWidths[FIX_ADJ] += minCol;
          }
          // include the DES_CON of a FIX_ADJ that has no FIX
          if ((desCon > fixAdj) && (fix <= 0)) { 
            aTotalCounts[DES_CON]++;
            aTotalWidths[DES_CON] += desCon;
            aTotalAvailWidths[DES_CON] += desCon;
            if (fixAdj > 0) {
              aTotalAvailWidths[DES_CON] -= fixAdj;
            }
          }
        }
      }
      else {
        nscoord desAdj = colFrame->GetWidth(DES_ADJ); 
        if ((desAdj > desCon) && (desAdj > fix)) {
          aTotalCounts[DES_CON]++;
          aTotalWidths[DES_CON] += desAdj;
          aTotalAvailWidths[DES_CON] += desAdj - fix;
        }
      }
      continue;
    }

    if (eProportionConstraint == colFrame->GetConstraint()) {
      nscoord minProp = colFrame->GetWidth(MIN_PRO);
      aTotalCounts[MIN_PRO]++;
      aTotalWidths[MIN_PRO] += minProp;
      aTotalAvailWidths[MIN_PRO] = aTotalWidths[MIN_PRO];
      aTotalCounts[DES_CON]++;
      aTotalWidths[DES_CON] += NSToCoordRound(((float)minProp) * mMinToDesProportionRatio);
      aTotalAvailWidths[DES_CON] = aTotalWidths[DES_CON];
      aMinWidths[MIN_PRO] += minCol;
      aMinWidths[DES_CON] += minProp;
      continue;
    }

    // desired alone is lowest priority
    aTotalCounts[DES_CON]++;
    aTotalWidths[DES_CON] += colFrame->GetDesWidth();
    aTotalAvailWidths[DES_CON] = aTotalWidths[DES_CON];
    aMinWidths[DES_CON] += minCol;
  }
  // if it is not a degenerate table, add the last spacing on the right
  if (numCols > 0) {
    aCellSpacing += spacingX;
  }
}


struct nsColInfo {
  nsColInfo(nsTableColFrame* aFrame,
            PRInt32          aIndex,
            PRInt32          aMinWidth,
            PRInt32          aWidth,
            PRInt32          aMaxWidth)
    : mFrame(aFrame), mIndex(aIndex), mMinWidth(aMinWidth), 
      mWidth(aWidth), mMaxWidth(aMaxWidth), mWeight(0)
  {}
  nsTableColFrame* mFrame;
  PRInt32          mIndex;
  PRInt32          mMinWidth;
  PRInt32          mWidth;
  PRInt32          mMaxWidth;
  float            mWeight;
};

void
AC_Wrapup(nsTableFrame* aTableFrame,
          PRInt32       aNumItems, 
          nsColInfo**   aColInfo,
          PRBool        aAbort = PR_FALSE)
{
  if (aColInfo) {
    for (PRInt32 i = 0; i < aNumItems; i++) {
      if (aColInfo[i]) {
        if (!aAbort) {
          aTableFrame->SetColumnWidth(aColInfo[i]->mIndex, aColInfo[i]->mWidth);
        }
        delete aColInfo[i];
      }
    }
    delete [] aColInfo;
  }
}

void
AC_Increase(PRInt32     aNumAutoCols,
            nsColInfo** aColInfo,
            PRInt32     aDivisor,
            PRInt32&    aAvailWidth)
{
  for (PRInt32 i = 0; i < aNumAutoCols; i++) {
    if ((aAvailWidth <= 0) || (aDivisor <= 0)) {
      break;
    }
    // aDivisor represents the sum of unallocated space (diff between max and min values)
    float percent = ((float)aColInfo[i]->mMaxWidth - (float)aColInfo[i]->mMinWidth) / (float)aDivisor;
    aDivisor -= aColInfo[i]->mMaxWidth - aColInfo[i]->mMinWidth;

    nscoord addition = NSToCoordRound(((float)(aAvailWidth)) * percent);
    // if its the last col, try to give what's left to it
    if ((i == aNumAutoCols - 1) && (addition < aAvailWidth)) {
      addition = aAvailWidth;
    }
    // don't let the addition exceed what is available to add
    addition = PR_MIN(addition, aAvailWidth);
    // don't go over the col max
    addition = PR_MIN(addition, aColInfo[i]->mMaxWidth - aColInfo[i]->mWidth);
    aColInfo[i]->mWidth += addition;
    aAvailWidth -= addition;
  }
}

void
AC_Decrease(PRInt32     aNumAutoCols,
            nsColInfo** aColInfo,
            PRInt32     aDivisor,
            PRInt32&    aExcess)
{
  for (PRInt32 i = 0; i < aNumAutoCols; i++) {
    if ((aExcess <= 0) || (aDivisor <= 0)) {
      break;
    }
    float percent = ((float)aColInfo[i]->mMaxWidth) / (float)aDivisor;
    aDivisor -= aColInfo[i]->mMaxWidth;
    nscoord reduction = NSToCoordRound(((float)(aExcess)) * percent);
    // if its the last col, try to remove the remaining excess from it
    if ((i == aNumAutoCols - 1) && (reduction < aExcess)) {
      reduction = aExcess;
    }
    // don't let the reduction exceed what is available to reduce
    reduction = PR_MIN(reduction, aExcess);
    // don't go under the col min
    reduction = PR_MIN(reduction, aColInfo[i]->mWidth - aColInfo[i]->mMinWidth);
    aColInfo[i]->mWidth -= reduction;
    aExcess -= reduction;
  }
}

void 
AC_Sort(nsColInfo** aColInfo, PRInt32 aNumCols)
{
  // sort the cols based on the Weight 
  for (PRInt32 j = aNumCols - 1; j > 0; j--) {
    for (PRInt32 i = 0; i < j; i++) { 
      if (aColInfo[i]->mWeight < aColInfo[i+1]->mWeight) { // swap them
        nsColInfo* save = aColInfo[i];
        aColInfo[i]     = aColInfo[i+1];
        aColInfo[i+1]   = save;
      }
    }
  }
}


// this assumes that the table has set the width for each col to be its min
void BasicTableLayoutStrategy::AllocateConstrained(PRInt32  aAvailWidth,
                                                   PRInt32  aWidthType,
                                                   PRBool   aStartAtMin,        
                                                   PRInt32* aAllocTypes)
{
  if ((0 == aAvailWidth) || (aWidthType < 0) || (aWidthType >= NUM_WIDTHS)) {
    NS_ASSERTION(PR_TRUE, "invalid args to AllocateConstrained");
    return;
  }

  PRInt32 numCols = mTableFrame->GetColCount();
  PRInt32 numConstrainedCols = 0;
  nscoord sumMaxConstraints  = 0;
  nscoord sumMinConstraints  = 0;
  PRBool useAdj = PR_TRUE;
  PRInt32 colX;
  // find out how many constrained cols there are
  for (colX = 0; colX < numCols; colX++) {
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    if (!CanAllocate(aWidthType, aAllocTypes[colX], colFrame, useAdj)) {
      if (-1 != aAllocTypes[colX]) {
        continue;
      }
    }
    numConstrainedCols++;
    nscoord width = colFrame->GetWidth(aWidthType);
    if ((DES_CON == aWidthType) && useAdj) {
      width = PR_MAX(width, colFrame->GetWidth(DES_ADJ));
    }
    else if (PCT == aWidthType) {
      width = PR_MAX(width, colFrame->GetWidth(PCT_ADJ));
    }
  }

  // allocate storage for the constrained cols. Only they get adjusted.
  nsColInfo** colInfo = new nsColInfo*[numConstrainedCols];
  if (!colInfo) return;
  memset(colInfo, 0, numConstrainedCols * sizeof(nsColInfo *));

  PRInt32 maxMinDiff = 0;
  PRInt32 constrColX = 0;
  // set the col info entries for each constrained col
  for (colX = 0; colX < numCols; colX++) {
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(colX);
    if (!CanAllocate(aWidthType, aAllocTypes[colX], colFrame, useAdj)) {
      if (-1 != aAllocTypes[colX]) {
        continue;
      }
    }
    nscoord minWidth = mTableFrame->GetColumnWidth(colX);
    nscoord maxWidth = colFrame->GetWidth(aWidthType);
    if ((DES_CON == aWidthType) && useAdj) {
      maxWidth = PR_MAX(maxWidth, colFrame->GetWidth(DES_ADJ));
    }
    else if (PCT == aWidthType) {
      maxWidth = PR_MAX(maxWidth, colFrame->GetWidth(PCT_ADJ));
    }
    if (maxWidth <= 0) {
      continue;
    }
    sumMaxConstraints += maxWidth;
    sumMinConstraints += minWidth;

    maxWidth = PR_MAX(maxWidth, minWidth);
    maxMinDiff += maxWidth - minWidth;
    nscoord startWidth = (aStartAtMin) ? minWidth : maxWidth;
    colInfo[constrColX] = new nsColInfo(colFrame, colX, minWidth, startWidth, maxWidth);
    if (!colInfo[constrColX]) {
      AC_Wrapup(mTableFrame, numConstrainedCols, colInfo, PR_TRUE);
      return;
    }
    aAllocTypes[colX] = aWidthType;
    constrColX++;
  }

  if (constrColX < numConstrainedCols) {
    // some of the constrainted cols might have been 0 and skipped
    numConstrainedCols = constrColX;
  }

  PRInt32 i;
  if (aStartAtMin) { // allocate extra space 
    nscoord availWidth = aAvailWidth; 
    for (i = 0; i < numConstrainedCols; i++) {
      // the weight here is a relative metric for determining when cols reach their max constraint. 
      // A col with a larger weight will reach its max before one with a smaller value.
      nscoord delta = colInfo[i]->mMaxWidth - colInfo[i]->mWidth;
      colInfo[i]->mWeight = (delta <= 0) 
        ? 1000000 // cols which have already reached their max get a large value
        : ((float)colInfo[i]->mMaxWidth) / ((float)delta);
    }
      
    // sort the cols based on the weight so that in one pass cols with higher 
    // weights will get their max earlier than ones with lower weights
    // This is an innefficient bubble sort, but unless there are an unlikely 
    // large number of cols, it is not an issue.
    AC_Sort(colInfo, numConstrainedCols);
   
    // compute the proportion to be added to each column, don't go beyond the col's
    // max. This algorithm assumes that the Weight works as stated above
    AC_Increase(numConstrainedCols, colInfo, sumMaxConstraints - sumMinConstraints, availWidth);
  }
  else { // reduce each col width 
    nscoord reduceWidth = maxMinDiff - aAvailWidth;
    if (reduceWidth < 0) {
      NS_ASSERTION(PR_TRUE, "AllocateConstrained called incorrectly");
      AC_Wrapup(mTableFrame, numConstrainedCols, colInfo);
      return;
    }
    for (i = 0; i < numConstrainedCols; i++) {
      // the weight here is a relative metric for determining when cols reach their min. 
      // A col with a larger weight will reach its min before one with a smaller value.
      nscoord delta = colInfo[i]->mWidth - colInfo[i]->mMinWidth;
      colInfo[i]->mWeight = (delta <= 0) 
        ? 1000000 // cols which have already reached their min get a large value
        : ((float)colInfo[i]->mWidth) / ((float)delta);
    }
      
    // sort the cols based on the Weight 
    AC_Sort(colInfo, numConstrainedCols);
   
    // compute the proportion to be subtracted from each column, don't go beyond 
    // the col's min. This algorithm assumes that the Weight works as stated above
    AC_Decrease(numConstrainedCols, colInfo, sumMaxConstraints, reduceWidth);
  }
  AC_Wrapup(mTableFrame, numConstrainedCols, colInfo);
}

// XXX this function will be improved after the colspan algorithms have been extracted
// from AssignPreliminarColumnWidths and AssignPercentageColumnWidths. For now, pessimistic
// assumptions are made
PRBool BasicTableLayoutStrategy::ColumnsCanBeInvalidatedBy(nsStyleCoord*           aPrevStyleWidth,
                                                           const nsTableCellFrame& aCellFrame) const
{
  if (!mTableFrame) 
    return PR_TRUE;

  const nsStylePosition* cellPosition;
  aCellFrame.GetStyleData(eStyleStruct_Position, (const nsStyleStruct*&)cellPosition);
  const nsStyleCoord& styleWidth = cellPosition->mWidth;

  PRInt32 colIndex;
  aCellFrame.GetColIndex(colIndex);
  nsTableColFrame* colFrame = mTableFrame->GetColFrame(colIndex);
  nscoord colSpan = mTableFrame->GetEffectiveColSpan(aCellFrame);

  if (aPrevStyleWidth) {
    nsTableColFrame* colSpanFrame = colFrame;
    // see if this cell is responsible for setting a fixed or percentage based col
    for (PRInt32 span = 1; span <= colSpan; span++) {
      if (&aCellFrame == colSpanFrame->GetConstrainingCell()) 
        return PR_TRUE; // assume that the style change will affect cols
      if (span < colSpan) 
        colSpanFrame = mTableFrame->GetColFrame(colIndex + span);
    }
    // if we get here, the cell was not responsible for a fixed or percentage col
    switch(aPrevStyleWidth->GetUnit()) {
    case eStyleUnit_Percent: 
      if (eStyleUnit_Percent == styleWidth.GetUnit()) {
        // PCT to PCT 
        if (aPrevStyleWidth->GetPercentValue() < styleWidth.GetPercentValue()) 
          return PR_TRUE; // XXX see comments above
      }
      // PCT to FIX and PCT to AUTO changes have no effect since PCT allocations 
      // are the highest priority and the cell's previous PCT value did not 
      // cause it to be responsible for setting any cells PCT_ADJ 
    case eStyleUnit_Coord: 
      if (eStyleUnit_Percent == styleWidth.GetUnit()) {
        // FIX to PCT 
        return PR_TRUE; // XXX see comments above
      } 
      else if (eStyleUnit_Coord == styleWidth.GetUnit()) {
        // FIX to FIX
        nscoord newWidth = styleWidth.GetCoordValue();
        if (aPrevStyleWidth->GetCoordValue() < newWidth) {
          if (colSpan > 1) 
            return PR_TRUE; // XXX see comments above
          if (newWidth > colFrame->GetFixWidth()) 
            return PR_TRUE; // XXX see comments above          
        }
      }
      // FIX to AUTO results in no column changes here
    case eStyleUnit_Auto: 
      if (eStyleUnit_Percent == styleWidth.GetUnit()) {
        // AUTO to PCT 
        return PR_TRUE; // XXX see comments above
      } 
      else if (eStyleUnit_Coord == styleWidth.GetUnit()) {
        // AUTO to FIX
        return PR_TRUE; // XXX see comments above
      } 
      // AUTO to AUTO is not a style change
    default:
      break;
    }
  }
  return PR_FALSE;
}


// XXX this function will be improved after the colspan algorithms have been extracted
// from AssignPreliminarColumnWidths and AssignPercentageColumnWidths. For now, pessimistic
// assumptions are made
PRBool BasicTableLayoutStrategy::ColumnsCanBeInvalidatedBy(const nsTableCellFrame& aCellFrame,
                                                           PRBool                  aConsiderMinWidth) const
{
  if (aConsiderMinWidth || !mTableFrame) 
    return PR_TRUE;

  PRInt32 colIndex;
  aCellFrame.GetColIndex(colIndex);
  nsTableColFrame* colFrame = mTableFrame->GetColFrame(colIndex);
  nscoord colSpan = mTableFrame->GetEffectiveColSpan(aCellFrame);

  // check to see if DES_CON can affect columns
  nsTableColFrame* spanFrame = colFrame;
  for (PRInt32 span = 0; span < colSpan; span++) {
    // see if the column width is constrained
    if ((spanFrame->GetPctWidth() > 0) || (spanFrame->GetFixWidth() > 0) ||
        (spanFrame->GetWidth(MIN_PRO) > 0)) {
      if ((spanFrame->GetWidth(PCT_ADJ) > 0) && (spanFrame->GetWidth(PCT) <= 0)) {
        return PR_TRUE; 
      }
      if ((spanFrame->GetWidth(FIX_ADJ) > 0) && (spanFrame->GetWidth(FIX) <= 0)) {
        return PR_TRUE; // its unfortunate that the balancing algorithms cause this
      }
    }
    else {
      return PR_TRUE; // XXX need to add cases if table has coord width specified
    }
    if (span < colSpan - 1) 
      spanFrame = mTableFrame->GetColFrame(colIndex + span + 1);
  }
  return PR_FALSE;
}

PRBool BasicTableLayoutStrategy::ColumnsAreValidFor(const nsTableCellFrame& aCellFrame,
                                                    nscoord                 aPrevCellMin,
                                                    nscoord                 aPrevCellDes) const
{
  PRInt32 colIndex;
  aCellFrame.GetColIndex(colIndex);
  nsTableColFrame* colFrame = mTableFrame->GetColFrame(colIndex);
  nscoord colSpan = mTableFrame->GetEffectiveColSpan(aCellFrame);

  nscoord cellMin = aCellFrame.GetPass1MaxElementSize().width;
  nscoord cellDes = aCellFrame.GetMaximumWidth();
  nscoord colMin  = colFrame->GetMinWidth();
  nscoord colDes  = colFrame->GetDesWidth();

  PRBool minChanged = PR_TRUE;
  if (((cellMin > aPrevCellMin) && (cellMin <= colMin)) ||
      ((cellMin <= aPrevCellMin) && (aPrevCellMin <= colMin))) {
    minChanged = PR_FALSE;
  }
  if (minChanged) {
    return PR_FALSE; // XXX add cases where table has coord width and cell is constrained
  }

  PRBool desChanged = PR_TRUE;
  if ((cellDes > aPrevCellDes) && (cellDes <= colDes)) {
    // XXX This next check causes a problem if the cell's desired width shrinks,
    // because the comparison (aPresCellDes <= colDes) will always be TRUE and
    // so we always end up setting desChanged to PR_FALSE. That means the column
    // won't shrink like it should
#if 0
      || ((cellDes <= aPrevCellDes) && (aPrevCellDes <= colDes))) {
#endif
    desChanged = PR_FALSE;
  }
  
  if (1 == colSpan) {
    // see if the column width is constrained
    if ((colFrame->GetPctWidth() > 0) || (colFrame->GetFixWidth() > 0) ||
        (colFrame->GetWidth(MIN_PRO) > 0)) {
      if ((colFrame->GetWidth(PCT_ADJ) > 0) && (colFrame->GetWidth(PCT) <= 0)) {
        if (desChanged) {
          return PR_FALSE; // XXX add cases where table has coord width
        }
      }
      if ((colFrame->GetWidth(FIX_ADJ) > 0) && (colFrame->GetWidth(FIX) <= 0)) {
        if (desChanged) {
          return PR_FALSE; // its unfortunate that the balancing algorithms cause this
                          // XXX add cases where table has coord width
        }
      }
    }
    else { // the column width is not constrained
      if (desChanged) {
        return PR_FALSE;
      }
    }
  }
  else {
    return PR_FALSE; // XXX this needs a lot of cases
  }
  return PR_TRUE;
}
  
PRBool BasicTableLayoutStrategy::IsColumnInList(const PRInt32 colIndex, 
                                                PRInt32*      colIndexes, 
                                                PRInt32       aNumFixedColumns)
{
  PRBool result = PR_FALSE;
  for (PRInt32 i = 0; i < aNumFixedColumns; i++) {
    if (colIndex == colIndexes[i]) {
      result = PR_TRUE;
      break;
    }
    else if (colIndex<colIndexes[i]) {
      break;
    }
  }
  return result;
}

PRBool BasicTableLayoutStrategy::ColIsSpecifiedAsMinimumWidth(PRInt32 aColIndex)
{
  PRBool result = PR_FALSE;
  nsTableColFrame* colFrame;
  mTableFrame->GetColumnFrame(aColIndex, colFrame);
  nsStyleCoord colStyleWidth = colFrame->GetStyleWidth();
  switch (colStyleWidth.GetUnit()) {
  case eStyleUnit_Coord:
    if (0 == colStyleWidth.GetCoordValue()) {
      result = PR_TRUE;
    }
    break;
  case eStyleUnit_Percent:
    {
      // total hack for now for 0% and 1% specifications
      // should compare percent to available parent width and see that it is below minimum
      // for this column
      float percent = colStyleWidth.GetPercentValue();
      if (0.0f == percent || 0.01f == percent) {  
        result = PR_TRUE;
      }
      break;
    }
  case eStyleUnit_Proportional:
    if (0 == colStyleWidth.GetIntValue()) {
      result = PR_TRUE;
    }

  default:
    break;
  }

  return result;
}

void BasicTableLayoutStrategy::Dump(PRInt32 aIndent)
{
  char* indent = new char[aIndent + 1];
  for (PRInt32 i = 0; i < aIndent + 1; i++) {
    indent[i] = ' ';
  }
  indent[aIndent] = 0;

  printf("%s**START BASIC STRATEGY DUMP** table=%p cols=%X",
         indent, mTableFrame, mCols);
  printf("\n%s minConWidth=%d maxConWidth=%d cellSpacing=%d propRatio=%.2f navQuirks=%d",
    indent, mMinTableContentWidth, mMaxTableContentWidth, mCellSpacingTotal, mMinToDesProportionRatio, mIsNavQuirksMode);
  printf(" **END BASIC STRATEGY DUMP** \n");
  delete [] indent;
}

