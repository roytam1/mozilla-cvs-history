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
#include "nsTableFrame.h"
#include "nsIRenderingContext.h"
#include "nsIStyleContext.h"
#include "nsIContent.h"
#include "nsIContentDelegate.h"
#include "nsCellMap.h"
#include "nsTableCellFrame.h"
#include "nsHTMLParts.h"
#include "nsTableColFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsIHTMLContent.h"

#include "BasicTableLayoutStrategy.h"

#include "nsIPresContext.h"
#include "nsCSSRendering.h"
#include "nsStyleConsts.h"
#include "nsVoidArray.h"
#include "nsIPtr.h"
#include "nsIView.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsIReflowCommand.h"

#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
static PRBool gsDebugCLD = PR_FALSE;
static PRBool gsDebugNT = PR_FALSE;
//#define NOISY
//#define NOISY_FLOW
//#ifdef NOISY_STYLE
#else
static const PRBool gsDebug = PR_FALSE;
static const PRBool gsDebugCLD = PR_FALSE;
static const PRBool gsDebugNT = PR_FALSE;
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

NS_DEF_PTR(nsIStyleContext);
NS_DEF_PTR(nsIContent);

const nsIID kTableFrameCID = NS_TABLEFRAME_CID;

/* ----------- CellData ---------- */

/* CellData is the info stored in the cell map */
CellData::CellData()
{
  mCell = nsnull;
  mRealCell = nsnull;
  mOverlap = nsnull;
}

CellData::~CellData()
{}

/* ----------- InnerTableReflowState ---------- */

struct InnerTableReflowState {

  // Our reflow state
  const nsReflowState& reflowState;

  // The body's available size (computed from the body's parent)
  nsSize availSize;

  nscoord leftInset, topInset;

  // Margin tracking information
  nscoord prevMaxPosBottomMargin;
  nscoord prevMaxNegBottomMargin;

  // Flags for whether the max size is unconstrained
  PRBool  unconstrainedWidth;
  PRBool  unconstrainedHeight;

  // Running y-offset
  nscoord y;

  // a list of the footers in this table frame, for quick access when inserting bodies
  nsVoidArray *footerList;

  // cache the total height of the footers for placing body rows
  nscoord footerHeight;

  InnerTableReflowState(nsIPresContext*      aPresContext,
                        const nsReflowState& aReflowState,
                        const nsMargin&      aBorderPadding)
    : reflowState(aReflowState)
  {
    prevMaxPosBottomMargin = 0;
    prevMaxNegBottomMargin = 0;
    y=0;  // border/padding/margin???

    unconstrainedWidth = PRBool(aReflowState.maxSize.width == NS_UNCONSTRAINEDSIZE);
    availSize.width = aReflowState.maxSize.width;
    if (!unconstrainedWidth) {
      availSize.width -= aBorderPadding.left + aBorderPadding.right;
    }
    leftInset = aBorderPadding.left;

    unconstrainedHeight = PRBool(aReflowState.maxSize.height == NS_UNCONSTRAINEDSIZE);
    availSize.height = aReflowState.maxSize.height;
    if (!unconstrainedHeight) {
      availSize.height -= aBorderPadding.top + aBorderPadding.bottom;
    }
    topInset = aBorderPadding.top;

    footerHeight = 0;
    footerList = nsnull;
  }

  ~InnerTableReflowState() {
    if (nsnull!=footerList)
      delete footerList;
  }
};

/* ----------- ColumnInfoCache ---------- */

static const PRInt32 NUM_COL_WIDTH_TYPES=4;

class ColumnInfoCache
{
public:
  ColumnInfoCache(PRInt32 aNumColumns);
  ~ColumnInfoCache();

  void AddColumnInfo(const nsStyleUnit aType, 
                     PRInt32 aColumnIndex);

  void GetColumnsByType(const nsStyleUnit aType, 
                        PRInt32& aOutNumColumns,
                        PRInt32 *& aOutColumnIndexes);
  enum ColWidthType {
    eColWidthType_Auto         = 0,      // width based on contents
    eColWidthType_Percent      = 1,      // (float) 1.0 == 100%
    eColWidthType_Coord        = 2,      // (nscoord) value is twips
    eColWidthType_Proportional = 3       // (int) value has proportional meaning
  };

private:
  PRInt32  mColCounts [4];
  PRInt32 *mColIndexes[4];
  PRInt32  mNumColumns;
};

ColumnInfoCache::ColumnInfoCache(PRInt32 aNumColumns)
{
  mNumColumns = aNumColumns;
  for (PRInt32 i=0; i<NUM_COL_WIDTH_TYPES; i++)
  {
    mColCounts[i] = 0;
    mColIndexes[i] = nsnull;
  }
}

ColumnInfoCache::~ColumnInfoCache()
{
  for (PRInt32 i=0; i<NUM_COL_WIDTH_TYPES; i++)
  {
    if (nsnull!=mColIndexes[i])
    {
      delete [] mColIndexes[i];
    }
  }
}

void ColumnInfoCache::AddColumnInfo(const nsStyleUnit aType, 
                                    PRInt32 aColumnIndex)
{
  switch (aType)
  {
    case eStyleUnit_Auto:
      if (nsnull==mColIndexes[eColWidthType_Auto])
        mColIndexes[eColWidthType_Auto] = new PRInt32[mNumColumns];     // TODO : be much more efficient
      mColIndexes[eColWidthType_Auto][mColCounts[eColWidthType_Auto]] = aColumnIndex;
      mColCounts[eColWidthType_Auto]++;
      break;

    case eStyleUnit_Percent:
      if (nsnull==mColIndexes[eColWidthType_Percent])
        mColIndexes[eColWidthType_Percent] = new PRInt32[mNumColumns];     // TODO : be much more efficient
      mColIndexes[eColWidthType_Percent][mColCounts[eColWidthType_Percent]] = aColumnIndex;
      mColCounts[eColWidthType_Percent]++;
      break;

    case eStyleUnit_Coord:
      if (nsnull==mColIndexes[eColWidthType_Coord])
        mColIndexes[eColWidthType_Coord] = new PRInt32[mNumColumns];     // TODO : be much more efficient
      mColIndexes[eColWidthType_Coord][mColCounts[eColWidthType_Coord]] = aColumnIndex;
      mColCounts[eColWidthType_Coord]++;
      break;

    case eStyleUnit_Proportional:
      if (nsnull==mColIndexes[eColWidthType_Proportional])
        mColIndexes[eColWidthType_Proportional] = new PRInt32[mNumColumns];     // TODO : be much more efficient
      mColIndexes[eColWidthType_Proportional][mColCounts[eColWidthType_Proportional]] = aColumnIndex;
      mColCounts[eColWidthType_Proportional]++;
      break;
  }
}


void ColumnInfoCache::GetColumnsByType(const nsStyleUnit aType, 
                                        PRInt32& aOutNumColumns,
                                        PRInt32 *& aOutColumnIndexes)
{
  // initialize out-params
  aOutNumColumns=0;
  aOutColumnIndexes=nsnull;
  
  // fill out params with column info based on aType
  switch (aType)
  {
    case eStyleUnit_Auto:
      aOutNumColumns = mColCounts[eColWidthType_Auto];
      aOutColumnIndexes = mColIndexes[eColWidthType_Auto];
      break;
    case eStyleUnit_Percent:
      aOutNumColumns = mColCounts[eColWidthType_Percent];
      aOutColumnIndexes = mColIndexes[eColWidthType_Percent];
      break;
    case eStyleUnit_Coord:
      aOutNumColumns = mColCounts[eColWidthType_Coord];
      aOutColumnIndexes = mColIndexes[eColWidthType_Coord];
      break;
    case eStyleUnit_Proportional:
      aOutNumColumns = mColCounts[eColWidthType_Proportional];
      aOutColumnIndexes = mColIndexes[eColWidthType_Proportional];
      break;
  }
}



/* --------------------- nsTableFrame -------------------- */


nsTableFrame::nsTableFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsContainerFrame(aContent, aParentFrame),
    mCellMap(nsnull),
    mColCache(nsnull),
    mColumnWidths(nsnull),
    mTableLayoutStrategy(nsnull),
    mFirstPassValid(PR_FALSE),
    mPass(kPASS_UNDEFINED),
    mIsInvariantWidth(PR_FALSE)
{
  mEffectiveColCount = -1;  // -1 means uninitialized
}

nsTableFrame::~nsTableFrame()
{
  if (nsnull!=mCellMap)
    delete mCellMap;

  if (nsnull!=mColumnWidths)
    delete [] mColumnWidths;

  if (nsnull!=mTableLayoutStrategy)
    delete mTableLayoutStrategy;

  if (nsnull!=mColCache)
    delete mColCache;
}

/* ****** CellMap methods ******* */

/* return the next row group frame after aRowGroupFrame */
nsTableRowGroupFrame* nsTableFrame::NextRowGroupFrame(nsTableRowGroupFrame* aRowGroupFrame)
{
  if (nsnull == aRowGroupFrame)
  {
    aRowGroupFrame = (nsTableRowGroupFrame*)mFirstChild;
  }
  else
  {
    aRowGroupFrame->GetNextSibling((nsIFrame*&)aRowGroupFrame);
  }

  while (nsnull != aRowGroupFrame)
  {
    const nsStyleDisplay *display;
    aRowGroupFrame->GetStyleData(eStyleStruct_Display, (nsStyleStruct *&)display);
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == display->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay )
    {
      break;
    }

    // Get the next frame
    aRowGroupFrame->GetNextSibling((nsIFrame*&)aRowGroupFrame);
  }

  return aRowGroupFrame;
}

/* counts columns in column groups */
PRInt32 nsTableFrame::GetSpecifiedColumnCount ()
{
  mColCount=0;
  nsIFrame * colGroup = mFirstChild;
  while (nsnull!=colGroup)
  {
    const nsStyleDisplay *childDisplay;
    colGroup->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      mColCount += ((nsTableColGroupFrame *)colGroup)->GetColumnCount();
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      break;
    }
    colGroup->GetNextSibling(colGroup);
  }    
  return mColCount;
}

PRInt32 nsTableFrame::GetRowCount ()
{
  PRInt32 rowCount = 0;

  nsCellMap *cellMap = GetCellMap();
  if (nsnull != cellMap)
    return cellMap->GetRowCount();

  nsIFrame *child=mFirstChild;
  while (nsnull!=child)
  {
    const nsStyleDisplay *childDisplay;
    child->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      PRInt32 thisRowCount=0;
      ((nsTableRowGroupFrame *)child)->GetRowCount(thisRowCount);
      rowCount += thisRowCount;
    }
    child->GetNextSibling(child);
  }
  return rowCount;
}

/* return the effective col count */
PRInt32 nsTableFrame::GetColCount ()
{
  nsCellMap *cellMap = GetCellMap();
  NS_ASSERTION(nsnull!=cellMap, "GetColCount can only be called after cellmap has been created");

  if (nsnull!=cellMap)
  {
    if (-1==mEffectiveColCount)
      SetEffectiveColCount();
  }
  return mEffectiveColCount;
}

void nsTableFrame::SetEffectiveColCount()
{
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    PRInt32 colCount = cellMap->GetColCount();
    mEffectiveColCount = colCount;
    PRInt32 rowCount = cellMap->GetRowCount();
    for (PRInt32 colIndex=colCount-1; colIndex>0; colIndex--)
    {
      PRBool deleteCol=PR_TRUE;
      for (PRInt32 rowIndex=0; rowIndex<rowCount; rowIndex++)
      {
        CellData *cell = cellMap->GetCellAt(rowIndex, colIndex);
        if ((nsnull!=cell) && (cell->mCell != nsnull))
        { // found a real cell, so we're done
          deleteCol = PR_FALSE;
          break;
        }
      }
      if (PR_TRUE==deleteCol)
        mEffectiveColCount--;
      else
        break;
    }
  }
}

nsTableColFrame * nsTableFrame::GetColFrame(PRInt32 aColIndex)
{
  nsTableColFrame *result = nsnull;
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    result = cellMap->GetColumnFrame(aColIndex);
  }
  return result;
}

// can return nsnull
nsTableCellFrame * nsTableFrame::GetCellAt(PRInt32 aRowIndex, PRInt32 aColIndex)
{
  nsTableCellFrame *result = nsnull;
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    CellData * cellData = cellMap->GetCellAt(aRowIndex, aColIndex);
    if (nsnull!=cellData)
    {
      result = cellData->mCell;
      if (nsnull==result)
        result = cellData->mRealCell->mCell;
    }
  }
  return result;
}

// return the number of rows spanned by aCell starting at aRowIndex
// note that this is different from just the rowspan of aCell
// (that would be GetEffectiveRowSpan (indexOfRowThatContains_aCell, aCell)
//
// XXX This code should be in the table row group frame instead, and it
// should clip rows spans so they don't extend past a row group rather than
// clip to the table itself. Before that can happen the code that builds the
// cell map needs to take row groups into account
PRInt32 nsTableFrame::GetEffectiveRowSpan (PRInt32 aRowIndex, nsTableCellFrame *aCell)
{
  NS_PRECONDITION (nsnull!=aCell, "bad cell arg");
  NS_PRECONDITION (0<=aRowIndex && aRowIndex<GetRowCount(), "bad row index arg");

  if (!(0<=aRowIndex && aRowIndex<GetRowCount()))
    return 1;

  PRInt32 rowSpan = aCell->GetRowSpan();
  PRInt32 rowCount = GetRowCount();
  if (rowCount < (aRowIndex + rowSpan))
    return (rowCount - aRowIndex);
  return rowSpan;
}

// return the number of cols spanned by aCell starting at aColIndex
// note that this is different from just the colspan of aCell
// (that would be GetEffectiveColSpan (indexOfColThatContains_aCell, aCell)
//
// XXX Should be moved to colgroup, as GetEffectiveRowSpan should be moved to rowgroup?
PRInt32 nsTableFrame::GetEffectiveColSpan (PRInt32 aColIndex, nsTableCellFrame *aCell)
{
  NS_PRECONDITION (nsnull!=aCell, "bad cell arg");
  nsCellMap *cellMap = GetCellMap();
  NS_PRECONDITION (nsnull!=cellMap, "bad call, cellMap not yet allocated.");
  NS_PRECONDITION (0<=aColIndex && aColIndex<GetColCount(), "bad col index arg");

  PRInt32 result;
  if (cellMap->GetRowCount()==1)
    return 1;
  PRInt32 colSpan = aCell->GetColSpan();
  PRInt32 colCount = GetColCount();
  if (colCount < (aColIndex + colSpan))
    result =  colCount - aColIndex;
  else
  {
    result = colSpan;
    // check for case where all cells in a column have a colspan
    PRInt32 initialColIndex = aCell->GetColIndex();
    PRInt32 minColSpanForCol = cellMap->GetMinColSpan(initialColIndex);
    result -= (minColSpanForCol - 1); // minColSpanForCol is always at least 1
                                      // and we want to treat default as 0 (no effect)
  }

  return result;
}

PRInt32 nsTableFrame::GetEffectiveCOLSAttribute()
{
  nsCellMap *cellMap = GetCellMap();
  NS_PRECONDITION (nsnull!=cellMap, "bad call, cellMap not yet allocated.");
  PRInt32 result;
  nsIFrame *tableFrame = this;
  nsStyleTable *tableStyle=nsnull;
  GetStyleData(eStyleStruct_Table, (nsStyleStruct *&)tableStyle);
  result = tableStyle->mCols;
  PRInt32 numCols = GetColCount();
  if (result>numCols)
    result = numCols;
  return result;
}


/* call when the cell structure has changed.  mCellMap will be rebuilt on demand. */
void nsTableFrame::ResetCellMap ()
{
  // XXX: SEC should this call ResetCellMap on firstInFlow?
  if (nsnull!=mCellMap)
    delete mCellMap;
  mCellMap = nsnull; // for now, will rebuild when needed
}

/** sum the columns represented by all nsTableColGroup objects
  * if the cell map says there are more columns than this, 
  * add extra implicit columns to the content tree.
  */
void nsTableFrame::EnsureColumns(nsIPresContext*      aPresContext,
                                 nsReflowMetrics&     aDesiredSize,
                                 const nsReflowState& aReflowState,
                                 nsReflowStatus&      aStatus)
{
#ifdef XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  // XXX sec should only be called on firstInFlow
  SetMinColSpanForTable();
  if (nsnull==mCellMap)
    return; // no info yet, so nothing useful to do

  // make sure we've accounted for the COLS attribute
  AdjustColumnsForCOLSAttribute();

  PRInt32 actualColumns = 0;
  nsTableColGroupFrame *lastColGroupFrame = nsnull;
  nsIFrame * firstRowGroupFrame=nsnull;
  nsIFrame * prevSibFrame=nsnull;
  nsIFrame * childFrame=mFirstChild;
  while (nsnull!=childFrame)
  {
    const nsStyleDisplay *childDisplay;
    childFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      PRInt32 numCols = ((nsTableColGroupFrame*)childFrame)->GetColumnCount();
      actualColumns += numCols;
      lastColGroupFrame = (nsTableColGroupFrame *)childFrame;
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      firstRowGroupFrame = childFrame;
      break;
    }
    prevSibFrame = childFrame;
    childFrame->GetNextSibling(childFrame);
  }    
  if (actualColumns < GetColCount())
  {
    nsTableColGroup *lastColGroup=nsnull;
    if (nsnull==lastColGroupFrame)
    {
      lastColGroup = new nsTableColGroup (PR_TRUE);   // create an implicit colgroup
      // XXX: instead of Append, maybe this should be insertAt(0)?
      mContent->AppendChildTo(lastColGroup, PR_FALSE);  // add the implicit colgroup to my content
      // Resolve style for the child
      nsIStyleContext* colGroupStyleContext =
        aPresContext->ResolveStyleContextFor(lastColGroup, this, PR_TRUE);      // kidStyleContext: REFCNT++
      nsIContentDelegate* kidDel = nsnull;
      kidDel = lastColGroup->GetDelegate(aPresContext);                         // kidDel: REFCNT++
      nsresult rv = kidDel->CreateFrame(aPresContext, lastColGroup, this,
                                        colGroupStyleContext, (nsIFrame *&)lastColGroupFrame);
      NS_RELEASE(kidDel);                                                       // kidDel: REFCNT--
      NS_RELEASE(colGroupStyleContext);                                         // kidStyleContenxt: REFCNT--

      // hook lastColGroupFrame into child list
      if (nsnull==firstRowGroupFrame)
      { // make lastColGroupFrame the last frame
        nsIFrame *lastChild=nsnull;
        LastChild(lastChild);
        lastChild->SetNextSibling(lastColGroupFrame);
      }
      else
      { // insert lastColGroupFrame before the first row group frame
        if (nsnull!=prevSibFrame)
        { // lastColGroupFrame is inserted between prevSibFrame and lastColGroupFrame
          prevSibFrame->SetNextSibling(lastColGroupFrame);
        }
        else
        { // lastColGroupFrame is inserted as the first child of this table
          mFirstChild = lastColGroupFrame;
        }
        lastColGroupFrame->SetNextSibling(firstRowGroupFrame);
      }
      mChildCount++;
    }
    else
    {
      lastColGroupFrame->GetContent((nsIContent *&)lastColGroup);  // ADDREF b: lastColGroup++
    }

    PRInt32 excessColumns = GetColCount() - actualColumns;
    for ( ; excessColumns > 0; excessColumns--)
    { //QQQ
      // need to find the generic way to stamp out this content, and ::AppendChildTo it
      nsTableCol *col = new nsTableCol(PR_TRUE);
      lastColGroup->AppendChildTo (col, PR_FALSE);
    }
    NS_RELEASE(lastColGroup);                       // ADDREF: lastColGroup--
    lastColGroupFrame->Reflow(*aPresContext, aDesiredSize, aReflowState, aStatus);
  }
#endif
}

/** sum the columns represented by all nsTableColGroup objects
  * if the cell map says there are more columns than this, 
  * add extra implicit columns to the content tree.
  */
// XXX should be nsresult, not void
void nsTableFrame::EnsureColumnFrameAt(PRInt32              aColIndex,
                                       nsIPresContext*      aPresContext,
                                       nsReflowMetrics&     aDesiredSize,
                                       const nsReflowState& aReflowState,
                                       nsReflowStatus&      aStatus)
{
  nsresult rv;
  PRInt32 actualColumns = 0;
  nsTableColGroupFrame *lastColGroupFrame = nsnull;
  nsIFrame * firstRowGroupFrame=nsnull;
  nsIFrame * prevSibFrame=nsnull;
  nsIFrame * childFrame=mFirstChild;
  while (nsnull!=childFrame)
  {
    const nsStyleDisplay *childDisplay;
    childFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      PRInt32 numCols = ((nsTableColGroupFrame*)childFrame)->GetColumnCount();
      actualColumns += numCols;
      lastColGroupFrame = (nsTableColGroupFrame *)childFrame;
      if (actualColumns > aColIndex)
        break;  // we have enough col frames at this point
    }
    else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
             NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      firstRowGroupFrame = childFrame;
      break;
    }
    prevSibFrame = childFrame;
    childFrame->GetNextSibling(childFrame);
  }    
  if (actualColumns <= aColIndex)
  {
    nsIHTMLContent *lastColGroup=nsnull;
    if (nsnull==lastColGroupFrame)
    {
      // create an implicit colgroup
      nsAutoString colGroupTag;
      nsHTMLAtoms::colgroup->ToString(colGroupTag);
      rv = NS_CreateHTMLElement(&lastColGroup, colGroupTag);  // ADDREF a: lastColGroup++
      //XXX mark it as implicit!
      mContent->AppendChildTo(lastColGroup, PR_FALSE);  // add the implicit colgroup to my content
      // Resolve style for the child
      nsIStyleContext* colGroupStyleContext =
        aPresContext->ResolveStyleContextFor(lastColGroup, this, PR_TRUE);      // kidStyleContext: REFCNT++
      nsIContentDelegate* kidDel = nsnull;
      kidDel = lastColGroup->GetDelegate(aPresContext);                         // kidDel: REFCNT++
      rv = kidDel->CreateFrame(aPresContext, lastColGroup, this,
                                        colGroupStyleContext, (nsIFrame *&)lastColGroupFrame);
      NS_RELEASE(kidDel);                                                       // kidDel: REFCNT--
      NS_RELEASE(colGroupStyleContext);                                         // kidStyleContenxt: REFCNT--

      // hook lastColGroupFrame into child list
      if (nsnull==firstRowGroupFrame)
      { // make lastColGroupFrame the last frame
        nsIFrame *lastChild=nsnull;
        LastChild(lastChild);
        lastChild->SetNextSibling(lastColGroupFrame);
      }
      else
      { // insert lastColGroupFrame before the first row group frame
        if (nsnull!=prevSibFrame)
        { // lastColGroupFrame is inserted between prevSibFrame and lastColGroupFrame
          prevSibFrame->SetNextSibling(lastColGroupFrame);
        }
        else
        { // lastColGroupFrame is inserted as the first child of this table
          mFirstChild = lastColGroupFrame;
        }
        lastColGroupFrame->SetNextSibling(firstRowGroupFrame);
      }
      mChildCount++;
    }
    else
    {
      lastColGroupFrame->GetContent((nsIContent *&)lastColGroup);  // ADDREF b: lastColGroup++
    }

    nsAutoString colTag;
    nsHTMLAtoms::col->ToString(colTag);
    PRInt32 excessColumns = aColIndex - actualColumns;
    for ( ; excessColumns >= 0; excessColumns--)
    {
      nsIHTMLContent *col=nsnull;
      // create an implicit col
      rv = NS_CreateHTMLElement(&col, colTag);  // ADDREF: col++
      //XXX mark the col implicit
      lastColGroup->AppendChildTo((nsIContent*)col, PR_FALSE);
      NS_RELEASE(col);                          // ADDREF: col--
    }
    NS_RELEASE(lastColGroup);                       // ADDREF: lastColGroup--
    lastColGroupFrame->Reflow(*aPresContext, aDesiredSize, aReflowState, aStatus);
  }
}

void nsTableFrame::AddColumnFrame (nsTableColFrame *aColFrame)
{
  mCellMap->AppendColumnFrame(aColFrame);
}

/** return the index of the next row that is not yet assigned */
PRInt32 nsTableFrame::GetNextAvailRowIndex() const
{
  PRInt32 result=0;
  if (nsnull!=mCellMap)
  {
    result = mCellMap->GetRowCount(); // the next index is the current count
    mCellMap->GrowToRow(result+1);    // expand the cell map to include this new row
  }
  return result;
}

/** return the index of the next column in aRowIndex that does not have a cell assigned to it */
PRInt32 nsTableFrame::GetNextAvailColIndex(PRInt32 aRowIndex, PRInt32 aColIndex) const
{
  PRInt32 result=0;
  if (nsnull!=mCellMap)
    result = mCellMap->GetNextAvailColIndex(aRowIndex, aColIndex);
  return result;
}

/** Get the cell map for this table frame.  It is not always mCellMap.
  * Only the firstInFlow has a legit cell map
  */
nsCellMap * nsTableFrame::GetCellMap()
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  if (this!=firstInFlow)
  {
    return firstInFlow->GetCellMap();
  }
  return mCellMap;
}

void nsTableFrame::SetMinColSpanForTable()
{ // XXX: must be called ONLY on first-in-flow
  // set the minColSpan for each column
  PRInt32 rowCount = mCellMap->GetRowCount();
  for (PRInt32 colIndex=0; colIndex<mColCount; colIndex++)
  {
    PRInt32 minColSpan;
    for (PRInt32 rowIndex=0; rowIndex<rowCount; rowIndex++)
    {
      nsTableCellFrame *cellFrame = mCellMap->GetCellFrameAt(rowIndex, colIndex);
      if (nsnull!=cellFrame)
      {
        PRInt32 colSpan = cellFrame->GetColSpan();
        if (0==rowIndex)
          minColSpan = colSpan;
        else
          minColSpan = PR_MIN(minColSpan, colSpan);
      }
    }
    if (1<minColSpan)
      mCellMap->SetMinColSpan(colIndex, minColSpan);
  }
}

void nsTableFrame::AddCellToTable (nsTableRowFrame *aRowFrame, 
                                   nsTableCellFrame *aCellFrame,
                                   PRBool aAddRow)
{
  NS_ASSERTION(nsnull!=aRowFrame, "bad aRowFrame arg");
  NS_ASSERTION(nsnull!=aCellFrame, "bad aCellFrame arg");
  NS_PRECONDITION(nsnull!=mCellMap, "bad cellMap");

  // XXX: must be called only on first-in-flow!
  if (gsDebug==PR_TRUE) printf("Build Cell Map...\n");

  // Make an educated guess as to how many columns we have. It's
  // only a guess because we can't know exactly until we have
  // processed the last row.
  if (0 == mColCount)
    mColCount = GetSpecifiedColumnCount();
  if (0 == mColCount) // no column parts
  {
    mColCount = aRowFrame->GetMaxColumns();
  }

  PRInt32 rowIndex;
  // If we have a cell map reset it; otherwise allocate a new cell map
  // also determine the index of aRowFrame and set it if necessary
  if (0==mCellMap->GetRowCount())
  { // this is the first time we've ever been called
    rowIndex = 0;
    if (gsDebug==PR_TRUE) printf("rowFrame %p set to index %d\n", aRowFrame, rowIndex);
  }
  else
  {
    rowIndex = mCellMap->GetRowCount() - 1;   // rowIndex is 0-indexed, rowCount is 1-indexed
  }

  PRInt32 colIndex=0;
  while (PR_TRUE)
  {
    CellData *data = mCellMap->GetCellAt(rowIndex, colIndex);
    if (nsnull == data)
    {
      BuildCellIntoMap(aCellFrame, rowIndex, colIndex);
      break;
    }
    colIndex++;
  }

  if (gsDebug==PR_TRUE)
    DumpCellMap ();
}

/**
  */
void nsTableFrame::DumpCellMap () 
{
  printf("dumping CellMap:\n");
  if (nsnull != mCellMap)
  {
    PRInt32 rowCount = mCellMap->GetRowCount();
    PRInt32 cols = mCellMap->GetColCount();
    for (PRInt32 r = 0; r < rowCount; r++)
    {
      if (gsDebug==PR_TRUE)
      { printf("row %d", r);
        printf(": ");
      }
      for (PRInt32 c = 0; c < cols; c++)
      {
        CellData *cd =mCellMap->GetCellAt(r, c);
        if (cd != nsnull)
        {
          if (cd->mCell != nsnull)
          {
            printf("C%d,%d ", r, c);
            printf("     ");
          }
          else
          {
            nsTableCellFrame *cell = cd->mRealCell->mCell;
            nsTableRowFrame *row;
            cell->GetGeometricParent((nsIFrame *&)row);
            int rr = row->GetRowIndex ();
            int cc = cell->GetColIndex ();
            printf("S%d,%d ", rr, cc);
            if (cd->mOverlap != nsnull)
            {
              cell = cd->mOverlap->mCell;
              nsTableRowFrame* row2;
              cell->GetGeometricParent((nsIFrame *&)row2);
              rr = row2->GetRowIndex ();
              cc = cell->GetColIndex ();
              printf("O%d,%c ", rr, cc);
            }
            else
              printf("     ");
          }
        }
        else
          printf("----      ");
      }
      printf("\n");
    }
  }
  else
    printf ("[nsnull]");
}

void nsTableFrame::BuildCellIntoMap (nsTableCellFrame *aCell, PRInt32 aRowIndex, PRInt32 aColIndex)
{
  NS_PRECONDITION (nsnull!=aCell, "bad cell arg");
  NS_PRECONDITION (0 <= aColIndex, "bad column index arg");
  NS_PRECONDITION (0 <= aRowIndex, "bad row index arg");

  // Setup CellMap for this cell
  int rowSpan = aCell->GetRowSpan();
  int colSpan = aCell->GetColSpan();
  if (gsDebug==PR_TRUE) printf("        BuildCellIntoMap. rowSpan = %d, colSpan = %d\n", rowSpan, colSpan);

  // Grow the mCellMap array if we will end up addressing
  // some new columns.
  if (mCellMap->GetColCount() < (aColIndex + colSpan))
  {
    if (gsDebug==PR_TRUE) 
      printf("        calling GrowCellMap(%d)\n", aColIndex+colSpan);
    GrowCellMap (aColIndex + colSpan);
  }

  if (mCellMap->GetRowCount() < (aRowIndex+1))
  {
    printf("*********************************************** calling GrowToRow(%d)\n", aRowIndex+1);
    mCellMap->GrowToRow(aRowIndex+1);
  }

  // Setup CellMap for this cell in the table
  CellData *data = new CellData ();
  data->mCell = aCell;
  data->mRealCell = data;
  if (gsDebug==PR_TRUE) printf("        calling mCellMap->SetCellAt(data, %d, %d)\n", aRowIndex, aColIndex);
  mCellMap->SetCellAt(data, aRowIndex, aColIndex);

  // Create CellData objects for the rows that this cell spans. Set
  // their mCell to nsnull and their mRealCell to point to data. If
  // there were no column overlaps then we could use the same
  // CellData object for each row that we span...
  if ((1 < rowSpan) || (1 < colSpan))
  {
    if (gsDebug==PR_TRUE) printf("        spans\n");
    for (int rowIndex = 0; rowIndex < rowSpan; rowIndex++)
    {
      if (gsDebug==PR_TRUE) printf("          rowIndex = %d\n", rowIndex);
      int workRow = aRowIndex + rowIndex;
      if (gsDebug==PR_TRUE) printf("          workRow = %d\n", workRow);
      for (int colIndex = 0; colIndex < colSpan; colIndex++)
      {
        if (gsDebug==PR_TRUE) printf("            colIndex = %d\n", colIndex);
        int workCol = aColIndex + colIndex;
        if (gsDebug==PR_TRUE) printf("            workCol = %d\n", workCol);
        CellData *testData = mCellMap->GetCellAt(workRow, workCol);
        if (nsnull == testData)
        {
          CellData *spanData = new CellData ();
          spanData->mRealCell = data;
          if (gsDebug==PR_TRUE) printf("            null GetCellAt(%d, %d) so setting to spanData\n", workRow, workCol);
          mCellMap->SetCellAt(spanData, workRow, workCol);
        }
        else if ((0 < rowIndex) || (0 < colIndex))
        { // we overlap, replace existing data, it might be shared
          if (gsDebug==PR_TRUE) printf("            overlapping Cell from GetCellAt(%d, %d) so setting to spanData\n", workRow, workCol);
          CellData *overlap = new CellData ();
          overlap->mCell = testData->mCell;
          overlap->mRealCell = testData->mRealCell;
          overlap->mOverlap = data;
          mCellMap->SetCellAt(overlap, workRow, workCol);
        }
      }
    }
  }
}

void nsTableFrame::GrowCellMap (PRInt32 aColCount)
{
  if (nsnull!=mCellMap)
  {
    mCellMap->GrowToCol(aColCount);
    mColCount = aColCount;
  }
}


/* ***** Column Layout Data methods ***** */

/*
 * Lists the column layout data which turns
 * around and lists the cell layout data.
 * This is for debugging purposes only.
 */
void nsTableFrame::ListColumnLayoutData(FILE* out, PRInt32 aIndent) 
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  if (this!=firstInFlow)
  {
    firstInFlow->ListColumnLayoutData(out, aIndent);
    return;
  }

  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  {
    fprintf(out,"Column Layout Data \n");
    
    PRInt32 numCols = GetColCount();
    PRInt32 numRows = cellMap->GetRowCount();
    for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
    {
      for (PRInt32 indent = aIndent; --indent >= 0; ) 
        fputs("  ", out);
      fprintf(out,"Column Data [%d] \n",colIndex);
      for (PRInt32 rowIndex = 0; rowIndex < numRows; rowIndex++)
      {
        nsTableCellFrame *cellFrame = cellMap->GetCellFrameAt(rowIndex, colIndex);
        PRInt32 rowIndent;
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);
        fprintf(out,"Cell Data [%d] \n",rowIndex);
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);
        nsMargin margin;
        cellFrame->GetMargin(margin);
        fprintf(out,"Margin -- Top: %d Left: %d Bottom: %d Right: %d \n",  
                NSTwipsToIntPoints(margin.top),
                NSTwipsToIntPoints(margin.left),
                NSTwipsToIntPoints(margin.bottom),
                NSTwipsToIntPoints(margin.right));
      
        for (rowIndent = aIndent+2; --rowIndent >= 0; ) fputs("  ", out);

    /*
        nscoord top,left,bottom,right;
        top = (mBorderFrame[NS_SIDE_TOP] ? cellFrame->GetBorderWidth((nsIFrame*)mBorderFrame[NS_SIDE_TOP], NS_SIDE_TOP) : 0);
        left = (mBorderFrame[NS_SIDE_LEFT] ? cellFrame->GetBorderWidth((nsIFrame*)mBorderFrame[NS_SIDE_LEFT], NS_SIDE_LEFT) : 0);
        bottom = (mBorderFrame[NS_SIDE_BOTTOM] ? cellFrame->GetBorderWidth((nsIFrame*)mBorderFrame[NS_SIDE_BOTTOM], NS_SIDE_BOTTOM) : 0);
        right = (mBorderFrame[NS_SIDE_RIGHT] ? cellFrame->GetBorderWidth((nsIFrame*)mBorderFrame[NS_SIDE_RIGHT], NS_SIDE_RIGHT) : 0);

        fprintf(out,"Border -- Top: %d Left: %d Bottom: %d Right: %d \n",  
                    NSTwipsToIntPoints(top),
                    NSTwipsToIntPoints(left),
                    NSTwipsToIntPoints(bottom),
                    NSTwipsToIntPoints(right));
    */
      }
    }
  }
}

/**
  * For the TableCell in CellData, add it to the list
  */
void nsTableFrame::AppendLayoutData(nsVoidArray* aList, nsTableCellFrame* aTableCell)
{
  if (aTableCell != nsnull)
  {
    aList->AppendElement((void*)aTableCell);
  }
}

void nsTableFrame::RecalcLayoutData()
{
  nsCellMap *cellMap = GetCellMap();
  if (nsnull==cellMap)
    return; // no info yet, so nothing useful to do

  PRInt32 colCount = GetColCount();
  PRInt32 rowCount = cellMap->GetRowCount();
  PRInt32 row = 0;
  PRInt32 col = 0;

  nsTableCellFrame*  above = nsnull;
  nsTableCellFrame*  below = nsnull;
  nsTableCellFrame*  left = nsnull;
  nsTableCellFrame*  right = nsnull;


  PRInt32       edge = 0;
  nsVoidArray*  boundaryCells[4];

  for (edge = 0; edge < 4; edge++)
    boundaryCells[edge] = new nsVoidArray();


  if (colCount != 0 && rowCount != 0)
  {
    for (row = 0; row < rowCount; row++)
    {
      for (col = 0; col < colCount; col++)
      {
        nsTableCellFrame*  cell = nsnull;
        CellData*     cellData = cellMap->GetCellAt(row,col);
        
        if (cellData)
          cell = cellData->mCell;
        
        if (nsnull==cell)
          continue;

        PRInt32 colSpan = cell->GetColSpan();
        PRInt32 rowSpan = cell->GetRowSpan();

        // clear the cells for all for edges
        for (edge = 0; edge < 4; edge++)
          boundaryCells[edge]->Clear();

        // Check to see if the cell data represents the top,left
        // corner of a a table cell

        // Check to see if cell the represents a top edge cell
        if (0 == row)
          above = nsnull;
        else
        {
          cellData = cellMap->GetCellAt(row-1,col);
          if (nsnull != cellData)
            above = cellData->mRealCell->mCell;

          // Does the cell data point to the same cell?
          // If it is, then continue
          if ((nsnull != above) && (above == cell))
            continue;
        }           

        // Check to see if cell the represents a left edge cell
        if (0 == col)
          left = nsnull;
        else
        {
          cellData = cellMap->GetCellAt(row,col-1);
          if (cellData != nsnull)
            left = cellData->mRealCell->mCell;

          if ((nsnull != left) && (left == cell))
            continue;
        }

        // If this is the top,left edged cell
        // Then add the cells on the for edges to the array
        
        // Do the top and bottom edge
        PRInt32   r,c;
        PRInt32   r1,r2;
        PRInt32   c1,c2;
        PRInt32   last;
        

        r1 = row - 1;
        r2 = row + rowSpan;
        c = col;
        last = col + colSpan -1;
        last = PR_MIN(last,colCount-1);
        
        while (c <= last)
        {
          if (r1 != -1)
          {
            // Add top edge cells
            if (c != col)
            {
              cellData = cellMap->GetCellAt(r1,c);
              if ((cellData != nsnull) && (cellData->mCell != above))
              {
                above = cellData->mCell;
                if (above != nsnull)
                  AppendLayoutData(boundaryCells[NS_SIDE_TOP],above);
              }
            }
            else if (above != nsnull)
            {
              AppendLayoutData(boundaryCells[NS_SIDE_TOP],above);
            }
          }

          if (r2 < rowCount)
          {
            // Add bottom edge cells
            cellData = cellMap->GetCellAt(r2,c);
            if ((cellData != nsnull) && cellData->mCell != below)
            {
              below = cellData->mCell;
              if (below != nsnull)
                AppendLayoutData(boundaryCells[NS_SIDE_BOTTOM],below);
            }
          }
          c++;
        }

        // Do the left and right edge
        c1 = col - 1;
        c2 = col + colSpan;
        r = row ;
        last = row + rowSpan-1;
        last = PR_MIN(last,rowCount-1);
        
        while (r <= last)
        {
          // Add left edge cells
          if (c1 != -1)
          {
            if (r != row)
            {
              cellData = cellMap->GetCellAt(r,c1);
              if ((cellData != nsnull) && (cellData->mCell != left))
              {
                left = cellData->mCell;
                if (left != nsnull)
                  AppendLayoutData(boundaryCells[NS_SIDE_LEFT],left);
              }
            }
            else if (left != nsnull)
            {
              AppendLayoutData(boundaryCells[NS_SIDE_LEFT],left);
            }
          }

          if (c2 < colCount)
          {
            // Add right edge cells
            cellData = cellMap->GetCellAt(r,c2);
            if ((cellData != nsnull) && (cellData->mCell != right))
            {
              right = cellData->mCell;
              if (right != nsnull)
                AppendLayoutData(boundaryCells[NS_SIDE_RIGHT],right);
            }
          }
          r++;
        }
        
        cell->RecalcLayoutData(this,boundaryCells);
      }
    }
  }
  for (edge = 0; edge < 4; edge++)
    delete boundaryCells[edge];
}




/* SEC: TODO: adjust the rect for captions */
NS_METHOD nsTableFrame::Paint(nsIPresContext& aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              const nsRect& aDirtyRect)
{
  // table paint code is concerned primarily with borders and bg color
  const nsStyleDisplay* disp =
    (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);

  if (disp->mVisible) {
    const nsStyleSpacing* spacing =
      (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
    const nsStyleColor* color =
      (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);

    nsRect  rect(0, 0, mRect.width, mRect.height);
    nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
                                    aDirtyRect, rect, *color);
    nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
                                aDirtyRect, rect, *spacing, 0);
  }

  // for debug...
  if (nsIFrame::GetShowFrameBorders()) {
    aRenderingContext.SetColor(NS_RGB(0,128,0));
    aRenderingContext.DrawRect(0, 0, mRect.width, mRect.height);
  }

  PaintChildren(aPresContext, aRenderingContext, aDirtyRect);
  return NS_OK;
}

PRBool nsTableFrame::NeedsReflow(const nsSize& aMaxSize)
{
  PRBool result = PR_TRUE;
  if (PR_TRUE==mIsInvariantWidth)
    result = PR_FALSE;
  // TODO: other cases...
  return result;
}

nsresult nsTableFrame::AdjustSiblingsAfterReflow(nsIPresContext*         aPresContext,
                                                 InnerTableReflowState& aState,
                                                 nsIFrame*              aKidFrame,
                                                 nscoord                aDeltaY)
{
  nsIFrame* lastKidFrame = aKidFrame;

  if (aDeltaY != 0) {
    // Move the frames that follow aKidFrame by aDeltaY
    nsIFrame* kidFrame;

    aKidFrame->GetNextSibling(kidFrame);
    while (nsnull != kidFrame) {
      nsPoint origin;
  
      // XXX We can't just slide the child if it has a next-in-flow
      kidFrame->GetOrigin(origin);
      origin.y += aDeltaY;
  
      // XXX We need to send move notifications to the frame...
      kidFrame->WillReflow(*aPresContext);
      kidFrame->MoveTo(origin.x, origin.y);

      // Get the next frame
      lastKidFrame = kidFrame;
      kidFrame->GetNextSibling(kidFrame);
    }

  } else {
    // Get the last frame
    LastChild(lastKidFrame);
  }

  // Update our running y-offset to reflect the bottommost child
  nsRect  rect;
  lastKidFrame->GetRect(rect);
  aState.y = rect.YMost();

  return NS_OK;
}

// SEC: TODO need to worry about continuing frames prev/next in flow for splitting across pages.
// SEC: TODO need to keep "first pass done" state, update it when ContentChanged notifications come in

/* overview:
  if mFirstPassValid is false, this is our first time through since content was last changed
    set pass to 1
    do pass 1
      get min/max info for all cells in an infinite space
      do column balancing
    set mFirstPassValid to true
    do pass 2
  if pass is 1,
    set pass to 2
    use column widths to ResizeReflow cells
*/

/* Layout the entire inner table. */
NS_METHOD nsTableFrame::Reflow(nsIPresContext& aPresContext,
                               nsReflowMetrics& aDesiredSize,
                               const nsReflowState& aReflowState,
                               nsReflowStatus& aStatus)
{
  if (gsDebug==PR_TRUE) 
  {
    printf("-----------------------------------------------------------------\n");
    printf("nsTableFrame::Reflow: table %p reason %d given maxSize=%d,%d\n",
            this, aReflowState.reason, aReflowState.maxSize.width, aReflowState.maxSize.height);
  }

#ifdef NS_DEBUG
  PreReflowCheck();
#endif

  // Initialize out parameter
  if (nsnull != aDesiredSize.maxElementSize) {
    aDesiredSize.maxElementSize->width = 0;
    aDesiredSize.maxElementSize->height = 0;
  }

  aStatus = NS_FRAME_COMPLETE;

  if (eReflowReason_Incremental == aReflowState.reason) {
    const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
      mStyleContext->GetStyleData(eStyleStruct_Spacing);
    nsMargin myBorderPadding;
    mySpacing->CalcBorderPaddingFor(this, myBorderPadding);
  
    InnerTableReflowState state(&aPresContext, aReflowState, myBorderPadding);
  
    nsIFrame* target;
    aReflowState.reflowCommand->GetTarget(target);
    if (this == target) {
      NS_NOTYETIMPLEMENTED("unexpected reflow command");
    }

    // XXX Deal with the case where the reflow command is targeted at us
    nsIFrame* kidFrame;
    aReflowState.reflowCommand->GetNext(kidFrame);

    // Remember the old rect
    nsRect  oldKidRect;
    kidFrame->GetRect(oldKidRect);

    // Pass along the reflow command
    nsReflowMetrics desiredSize(nsnull);
    // XXX Correctly compute the available space...
    nsReflowState kidReflowState(kidFrame, aReflowState, aReflowState.maxSize);
    kidFrame->WillReflow(aPresContext);
    aStatus = ReflowChild(kidFrame, &aPresContext, desiredSize, kidReflowState);

    // Resize the row group frame
    nsRect  kidRect;
    kidFrame->GetRect(kidRect);
    kidFrame->SizeTo(desiredSize.width, desiredSize.height);

#if 1
    // XXX For the time being just fall through and treat it like a
    // pass 2 reflow...
    mPass = kPASS_SECOND;
    // calling intialize here resets all the cached info based on new table content 
    if (nsnull!=mTableLayoutStrategy)
    {
      mTableLayoutStrategy->Initialize(aDesiredSize.maxElementSize);
    }
#else
    // XXX Hack...
    AdjustSiblingsAfterReflow(&aPresContext, state, kidFrame, desiredSize.height -
                              oldKidRect.height);
    aDesiredSize.width = mRect.width;
    aDesiredSize.height = state.y + myBorderPadding.top + myBorderPadding.bottom;
    return NS_OK;
#endif
  }

  if (PR_TRUE==NeedsReflow(aReflowState.maxSize))
  {
    if (PR_FALSE==IsFirstPassValid())
    { // we treat the table as if we've never seen the layout data before
      if (mCellMap!=nsnull)
        delete mCellMap;
      mCellMap = new nsCellMap(0, 0);
      mPass = kPASS_FIRST;
      aStatus = ResizeReflowPass1(&aPresContext, aDesiredSize, aReflowState, aStatus);
      // check result
    }
    mPass = kPASS_SECOND;

    if (nsnull==mPrevInFlow)
    {
      // assign column widths, and assign aMaxElementSize->width
      BalanceColumnWidths(&aPresContext, aReflowState, aReflowState.maxSize,
                          aDesiredSize.maxElementSize);

      // assign table width
      SetTableWidth(&aPresContext);
    }

    // Constrain our reflow width to the computed table width
    nsReflowState    reflowState(aReflowState);
    reflowState.maxSize.width = mRect.width;
    aStatus = ResizeReflowPass2(&aPresContext, aDesiredSize, reflowState, 0, 0);

    mPass = kPASS_UNDEFINED;
  }
  else
  {
    // set aDesiredSize and aMaxElementSize
  }

  if (gsDebugNT==PR_TRUE) 
  {
    if (nsnull!=aDesiredSize.maxElementSize)
      printf("%p: Inner table reflow complete, returning aDesiredSize = %d,%d and aMaxElementSize=%d,%d\n",
              this, aDesiredSize.width, aDesiredSize.height, 
              aDesiredSize.maxElementSize->width, aDesiredSize.maxElementSize->height);
    else
      printf("%p: Inner table reflow complete, returning aDesiredSize = %d,%d and NSNULL aMaxElementSize\n",
              this, aDesiredSize.width, aDesiredSize.height);
  }

#ifdef NS_DEBUG
  PostReflowCheck(aStatus);
#endif  

  if (PR_TRUE==gsDebug) printf("end reflow for table %p\n", this);
  return NS_OK;
}

/** the first of 2 reflow passes
  * lay out the captions and row groups in an infinite space (NS_UNCONSTRAINEDSIZE)
  * cache the results for each caption and cell.
  * if successful, set mFirstPassValid=PR_TRUE, so we know we can skip this step 
  * next time.  mFirstPassValid is set to PR_FALSE when content is changed.
  * NOTE: should never get called on a continuing frame!  All cached pass1 state
  *       is stored in the inner table first-in-flow.
  */
nsReflowStatus nsTableFrame::ResizeReflowPass1(nsIPresContext* aPresContext,
                                               nsReflowMetrics& aDesiredSize,
                                               const nsReflowState& aReflowState,
                                               nsReflowStatus& aStatus)
{
  NS_PRECONDITION(aReflowState.frame == this, "bad reflow state");
  NS_PRECONDITION(aReflowState.parentReflowState->frame == mGeometricParent,
                  "bad parent reflow state");
  NS_ASSERTION(nsnull!=aPresContext, "bad pres context param");
  NS_ASSERTION(nsnull==mPrevInFlow, "illegal call, cannot call pass 1 on a continuing frame.");
  NS_ASSERTION(nsnull != mContent, "null content");

  if (PR_TRUE==gsDebugNT) printf("%p nsTableFrame::ResizeReflow Pass1: maxSize=%d,%d\n",
                               this, aReflowState.maxSize.width, aReflowState.maxSize.height);
  nsReflowStatus result = NS_FRAME_COMPLETE;

  mChildCount = 0;
  mFirstContentOffset = mLastContentOffset = 0;

  nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE); // availSize is the space available at any given time in the process
  nsSize maxSize(0, 0);       // maxSize is the size of the largest child so far in the process
  nsSize kidMaxSize(0,0);
  nsReflowMetrics kidSize(&kidMaxSize);
  nscoord y = 0;
  nscoord maxAscent = 0;
  nscoord maxDescent = 0;
  PRInt32 kidIndex = 0;
  PRInt32 lastIndex;
  mContent->ChildCount(lastIndex);
  PRInt32 contentOffset=0;
  nsIFrame* prevKidFrame = nsnull;/* XXX incremental reflow! */

  // Compute the insets (sum of border and padding)
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);
  nscoord topInset = borderPadding.top;
  nscoord rightInset = borderPadding.right;
  nscoord bottomInset = borderPadding.bottom;
  nscoord leftInset = borderPadding.left;
  nsReflowReason  reflowReason = aReflowState.reason;
  nsIContent * prevKid; // do NOT hold a reference for this temp pointer!

  /* assumes that Table's children are in the following order:
   *  Captions
   *  ColGroups, in order
   *  THead, in order
   *  TFoot, in order
   *  TBody, in order
   */
  for (;;) {
    nsIContentPtr kid;
    mContent->ChildAt(kidIndex, kid.AssignRef());   // kid: REFCNT++
    if (kid.IsNull()) {
      result = NS_FRAME_COMPLETE;
      break;
    }

    if (kid==prevKid)
    { // we just saw this kid, but in it's processing someone inserted something in front of it
      // (probably a colgroup).  So don't process it twice.
      kidIndex++;
      continue;
    }
    prevKid = kid;

    mLastContentIsComplete = PR_TRUE;

    // Resolve style
    nsIStyleContextPtr kidStyleContext =
      aPresContext->ResolveStyleContextFor(kid, this, PR_TRUE);
    NS_ASSERTION(kidStyleContext.IsNotNull(), "null style context for kid");
    const nsStyleDisplay *childDisplay = (nsStyleDisplay*)kidStyleContext->GetStyleData(eStyleStruct_Display);
    if (NS_STYLE_DISPLAY_TABLE_CAPTION != childDisplay->mDisplay)
    {
      // get next frame, creating one if needed
      nsIFrame* kidFrame=nsnull;
      if (nsnull!=prevKidFrame)
        prevKidFrame->GetNextSibling(kidFrame);  // no need to check for an error, just see if it returned null...
      else
        kidFrame=mFirstChild;

      // if this is the first time, allocate the frame
      if (nsnull==kidFrame)
      {
        nsIContentDelegate* kidDel;
        kidDel = kid->GetDelegate(aPresContext);
        nsresult rv = kidDel->CreateFrame(aPresContext, kid,
                                          this, kidStyleContext, kidFrame);
        reflowReason = eReflowReason_Initial;
        NS_RELEASE(kidDel);

        // Link child frame into the list of children
        if (nsnull != prevKidFrame) {
          prevKidFrame->SetNextSibling(kidFrame);
        } else {
          // Our first child
          mFirstChild = kidFrame;
          SetFirstContentOffset(kidIndex);
          if (gsDebug) printf("INNER: set first content offset to %d\n", GetFirstContentOffset()); //@@@
        }
        mChildCount++;
      }

      nsSize maxKidElementSize(0,0);
      nsReflowState kidReflowState(kidFrame, aReflowState, availSize,
                                   reflowReason);
      PRInt32 yCoord = y;
      if (NS_UNCONSTRAINEDSIZE!=yCoord)
        yCoord+= topInset;
      kidFrame->WillReflow(*aPresContext);
      kidFrame->MoveTo(leftInset, yCoord);
      result = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState);

      // Place the child since some of it's content fit in us.
      if (PR_TRUE==gsDebugNT) {
        printf ("%p: reflow of row group returned desired=%d,%d, max-element=%d,%d\n",
                this, kidSize.width, kidSize.height, kidMaxSize.width, kidMaxSize.height);
      }
      kidFrame->SetRect(nsRect(leftInset, yCoord,
                               kidSize.width, kidSize.height));
      if (NS_UNCONSTRAINEDSIZE==kidSize.height)
        y = NS_UNCONSTRAINEDSIZE;
      else
        y += kidSize.height;
      if (kidMaxSize.width > maxSize.width) {
        maxSize.width = kidMaxSize.width;
      }
      if (kidMaxSize.height > maxSize.height) {
        maxSize.height = kidMaxSize.height;
      }

      prevKidFrame = kidFrame;

      if (NS_FRAME_IS_NOT_COMPLETE(result)) {
        // If the child didn't finish layout then it means that it used
        // up all of our available space (or needs us to split).
        mLastContentIsComplete = PR_FALSE;
        break;
      }
    }
    contentOffset++;
    kidIndex++;
    if (NS_FRAME_IS_NOT_COMPLETE(result)) {
      // If the child didn't finish layout then it means that it used
      // up all of our available space (or needs us to split).
      mLastContentIsComplete = PR_FALSE;
      break;
    }
  }

  BuildColumnCache(aPresContext, aDesiredSize, aReflowState, aStatus);
  // Recalculate Layout Dependencies
  RecalcLayoutData();

  if (nsnull != prevKidFrame) {
    NS_ASSERTION(IsLastChild(prevKidFrame), "unexpected last child");
                                           // can't use SetLastContentOffset here
    mLastContentOffset = contentOffset-1;    // takes into account colGroup frame we're not using
    if (gsDebug) printf("INNER: set last content offset to %d\n", GetLastContentOffset()); //@@@
  }

  aDesiredSize.width = kidSize.width;
  mFirstPassValid = PR_TRUE;

  return result;
}

/** the second of 2 reflow passes
  */
nsReflowStatus nsTableFrame::ResizeReflowPass2(nsIPresContext* aPresContext,
                                               nsReflowMetrics& aDesiredSize,
                                               const nsReflowState& aReflowState,
                                               PRInt32 aMinCaptionWidth,
                                               PRInt32 mMaxCaptionWidth)
{
  NS_PRECONDITION(aReflowState.frame == this, "bad reflow state");
  NS_PRECONDITION(aReflowState.parentReflowState->frame == mGeometricParent,
                  "bad parent reflow state");
  if (PR_TRUE==gsDebugNT)
    printf("%p nsTableFrame::ResizeReflow Pass2: maxSize=%d,%d\n",
           this, aReflowState.maxSize.width, aReflowState.maxSize.height);

  nsReflowStatus result = NS_FRAME_COMPLETE;

  const nsStyleSpacing* mySpacing = (const nsStyleSpacing*)
    mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin myBorderPadding;
  mySpacing->CalcBorderPaddingFor(this, myBorderPadding);

  InnerTableReflowState state(aPresContext, aReflowState, myBorderPadding);

  // now that we've computed the column  width information, reflow all children
  nsIContent* c = mContent;
  NS_ASSERTION(nsnull != c, "null kid");
  nsSize kidMaxSize(0,0);

  PRInt32 kidIndex = 0;
  PRInt32 lastIndex;
  c->ChildCount(lastIndex);
  nsIFrame* prevKidFrame = nsnull;/* XXX incremental reflow! */

#ifdef NS_DEBUG
  //PreReflowCheck();
#endif

  PRBool        reflowMappedOK = PR_TRUE;
  nsReflowStatus  status = NS_FRAME_COMPLETE;

  // Check for an overflow list
  MoveOverflowToChildList();

  // Reflow the existing frames
  if (nsnull != mFirstChild) {
    reflowMappedOK = ReflowMappedChildren(aPresContext, state, aDesiredSize.maxElementSize);
    if (PR_FALSE == reflowMappedOK) {
      status = NS_FRAME_NOT_COMPLETE;
    }
  }

  // Did we successfully reflow our mapped children?
  if (PR_TRUE == reflowMappedOK) {
    // Any space left?
    PRInt32 numKids;
    mContent->ChildCount(numKids);
    if (state.availSize.height <= 0) {
      // No space left. Don't try to pull-up children or reflow unmapped
      if (NextChildOffset() < numKids) {
        status = NS_FRAME_NOT_COMPLETE;
      }
    } else if (NextChildOffset() < numKids) {
      // Try and pull-up some children from a next-in-flow
      if (PullUpChildren(aPresContext, state, aDesiredSize.maxElementSize)) {
        // If we still have unmapped children then create some new frames
        mContent->ChildCount(numKids);
        if (NextChildOffset() < numKids) {
          status = ReflowUnmappedChildren(aPresContext, state, aDesiredSize.maxElementSize);
        }
      } else {
        // We were unable to pull-up all the existing frames from the
        // next in flow
        status = NS_FRAME_NOT_COMPLETE;
      }
    }
  }

  // Return our size and our status
  aDesiredSize.width = ComputeDesiredWidth(aReflowState);
  aDesiredSize.height = state.y + myBorderPadding.top + myBorderPadding.bottom;


  if (NS_FRAME_IS_NOT_COMPLETE(status)) {
    // Don't forget to add in the bottom margin from our last child.
    // Only add it in if there's room for it.
    nscoord margin = state.prevMaxPosBottomMargin -
      state.prevMaxNegBottomMargin;
    if (state.availSize.height >= margin) {
      state.y += margin;
    }
  }

  mPass = kPASS_UNDEFINED;  // we're no longer in-process

#ifdef NS_DEBUG
  //PostReflowCheck(status);
#endif

  return status;

}

nscoord nsTableFrame::ComputeDesiredWidth(const nsReflowState& aReflowState) const
{
  nscoord desiredWidth=aReflowState.maxSize.width;
  // this is the biggest hack in the world.  But there's no other rational way to handle nested percent tables
  nsStylePosition* position;
  PRBool isNested=IsNested(aReflowState, position);
  if((eReflowReason_Initial==aReflowState.reason) && 
     (PR_TRUE==isNested) && (eStyleUnit_Percent==position->mWidth.GetUnit()))
  {
    desiredWidth =  mTableLayoutStrategy->GetTableMaxWidth();
  }
  return desiredWidth;
}

// Collapse child's top margin with previous bottom margin
nscoord nsTableFrame::GetTopMarginFor(nsIPresContext*      aCX,
                                      InnerTableReflowState& aState,
                                      const nsMargin& aKidMargin)
{
  nscoord margin;
  nscoord maxNegTopMargin = 0;
  nscoord maxPosTopMargin = 0;
  if ((margin = aKidMargin.top) < 0) {
    maxNegTopMargin = -margin;
  } else {
    maxPosTopMargin = margin;
  }

  nscoord maxPos = PR_MAX(aState.prevMaxPosBottomMargin, maxPosTopMargin);
  nscoord maxNeg = PR_MAX(aState.prevMaxNegBottomMargin, maxNegTopMargin);
  margin = maxPos - maxNeg;

  return margin;
}

// Position and size aKidFrame and update our reflow state. The origin of
// aKidRect is relative to the upper-left origin of our frame, and includes
// any left/top margin.
void nsTableFrame::PlaceChild(nsIPresContext*    aPresContext,
                              InnerTableReflowState& aState,
                              nsIFrame*          aKidFrame,
                              const nsRect&      aKidRect,
                              nsSize*            aMaxElementSize,
                              nsSize&            aKidMaxElementSize)
{
  if (PR_TRUE==gsDebug)
    printf ("table: placing row group at %d, %d, %d, %d\n",
           aKidRect.x, aKidRect.y, aKidRect.width, aKidRect.height);
  // Place and size the child
  aKidFrame->SetRect(aKidRect);

  // Adjust the running y-offset
  aState.y += aKidRect.height;

  // If our height is constrained then update the available height
  if (PR_FALSE == aState.unconstrainedHeight) {
    aState.availSize.height -= aKidRect.height;
  }

  // If this is a footer row group, add it to the list of footer row groups
  const nsStyleDisplay *childDisplay;
  aKidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
  if (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay)
  {
    if (nsnull==aState.footerList)
      aState.footerList = new nsVoidArray();
    aState.footerList->AppendElement((void *)aKidFrame);
    aState.footerHeight += aKidRect.height;
  }
  // else if this is a body row group, push down all the footer row groups
  else
  {
    // don't bother unless there are footers to push down
    if (nsnull!=aState.footerList  &&  0!=aState.footerList->Count())
    {
      nsPoint origin;
      aKidFrame->GetOrigin(origin);
      origin.y -= aState.footerHeight;
      aKidFrame->MoveTo(origin.x, origin.y);
      // XXX do we need to check for headers here also, or is that implicit?
      if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay)
      {
        PRInt32 numFooters = aState.footerList->Count();
        for (PRInt32 footerIndex = 0; footerIndex < numFooters; footerIndex++)
        {
          nsTableRowGroupFrame * footer = (nsTableRowGroupFrame *)(aState.footerList->ElementAt(footerIndex));
          NS_ASSERTION(nsnull!=footer, "bad footer list in table inner frame.");
          if (nsnull!=footer)
          {
            footer->GetOrigin(origin);
            origin.y += aKidRect.height;
            footer->MoveTo(origin.x, origin.y);
          }
        }
      }
    }
  }
  if (nsnull != aMaxElementSize) 
  {
    nsMargin borderPadding;
    const nsStyleSpacing* tableSpacing;
    // begin REMOVE_ME_WHEN_TABLE_STYLE_IS_RESOLVED!
    nsIFrame * parent = nsnull;
    GetGeometricParent(parent);
    parent->GetStyleData(eStyleStruct_Spacing , ((nsStyleStruct *&)tableSpacing));
    // end REMOVE_ME_WHEN_TABLE_STYLE_IS_RESOLVED!
    tableSpacing->CalcBorderPaddingFor(this, borderPadding);
    nscoord cellSpacing = GetCellSpacing();
    nscoord kidWidth = aKidMaxElementSize.width + borderPadding.left + borderPadding.right + cellSpacing*2;
    aMaxElementSize->width = PR_MAX(aMaxElementSize->width, kidWidth); 
    aMaxElementSize->height += aKidMaxElementSize.height;
    if (gsDebug)
      printf("%p placeChild set MES->width to %d\n", 
             this, aMaxElementSize->width);
  }
}

/**
 * Reflow the frames we've already created
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @return  true if we successfully reflowed all the mapped children and false
 *            otherwise, e.g. we pushed children to the next in flow
 */
PRBool nsTableFrame::ReflowMappedChildren( nsIPresContext*        aPresContext,
                                           InnerTableReflowState& aState,
                                           nsSize*                aMaxElementSize)
{
#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
#ifdef NOISY
  ListTag(stdout);
  printf(": reflow mapped (childCount=%d) [%d,%d,%c]\n",
         mChildCount,
         mFirstContentOffset, mLastContentOffset,
         (mLastContentIsComplete ? 'T' : 'F'));
#ifdef NOISY_FLOW
  {
    nsTableFrame* flow = (nsTableFrame*) mNextInFlow;
    while (flow != 0) {
      printf("  %p: [%d,%d,%c]\n",
             flow, flow->mFirstContentOffset, flow->mLastContentOffset,
             (flow->mLastContentIsComplete ? 'T' : 'F'));
      flow = (nsTableFrame*) flow->mNextInFlow;
    }
  }
#endif
#endif
  NS_PRECONDITION(nsnull != mFirstChild, "no children");

  PRInt32   childCount = 0;
  nsIFrame* prevKidFrame = nsnull;
  mLastContentOffset = 0;
  // Remember our original mLastContentIsComplete so that if we end up
  // having to push children, we have the correct value to hand to
  // PushChildren.
  PRBool    originalLastContentIsComplete = mLastContentIsComplete;

  nsSize    kidMaxElementSize(0,0);
  nsSize*   pKidMaxElementSize = (nsnull != aMaxElementSize) ? &kidMaxElementSize : nsnull;
  PRBool    result = PR_TRUE;

  for (nsIFrame*  kidFrame = mFirstChild; nsnull != kidFrame; ) {
    nsSize            kidAvailSize(aState.availSize);
    nsReflowMetrics   desiredSize(pKidMaxElementSize);
    desiredSize.width=desiredSize.height=desiredSize.ascent=desiredSize.descent=0;
    nsReflowStatus    status;

    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_CAPTION != childDisplay->mDisplay)
    { // for all colgroups and rowgroups...
      const nsStyleSpacing* kidSpacing;
      kidFrame->GetStyleData(eStyleStruct_Spacing, ((nsStyleStruct *&)kidSpacing));
      nsMargin kidMargin;
      kidSpacing->CalcMarginFor(kidFrame, kidMargin);

      nscoord topMargin = GetTopMarginFor(aPresContext, aState, kidMargin);
      nscoord bottomMargin = kidMargin.bottom;

      // Figure out the amount of available size for the child (subtract
      // off the top margin we are going to apply to it)
      if (PR_FALSE == aState.unconstrainedHeight) {
        kidAvailSize.height -= topMargin;
      }
      // Subtract off for left and right margin
      if (PR_FALSE == aState.unconstrainedWidth) {
        kidAvailSize.width -= kidMargin.left + kidMargin.right;
      }

      // Reflow the child into the available space
      nsReflowState kidReflowState(kidFrame, aState.reflowState, kidAvailSize,
                                   eReflowReason_Resize);
      kidFrame->WillReflow(*aPresContext);
      nscoord x = aState.leftInset + kidMargin.left;
      nscoord y = aState.topInset + aState.y + topMargin;
      kidFrame->MoveTo(x, y);
      status = ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState);
      // Did the child fit?
      if ((kidFrame != mFirstChild) && (desiredSize.height > kidAvailSize.height))
      {
        // The child is too wide to fit in the available space, and it's
        // not our first child

        // Since we are giving the next-in-flow our last child, we
        // give it our original mLastContentIsComplete too (in case we
        // are pushing into an empty next-in-flow)
        PushChildren(kidFrame, prevKidFrame, originalLastContentIsComplete);
        PRInt32 contentIndex;
        prevKidFrame->GetContentIndex(contentIndex);
        SetLastContentOffset(contentIndex);

        result = PR_FALSE;
        break;
      }

      // Place the child after taking into account it's margin
      aState.y += topMargin;
      nsRect kidRect (x, y, desiredSize.width, desiredSize.height);
      if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
          NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
          NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
      {
        // we don't want to adjust the maxElementSize if this is an initial reflow
        // it was set by the TableLayoutStrategy and shouldn't be changed.
        nsSize *requestedMaxElementSize = nsnull;
        if (eReflowReason_Initial != aState.reflowState.reason)
          requestedMaxElementSize = aMaxElementSize;
        PlaceChild(aPresContext, aState, kidFrame, kidRect,
                   requestedMaxElementSize, kidMaxElementSize);
        if (bottomMargin < 0) {
          aState.prevMaxNegBottomMargin = -bottomMargin;
        } else {
          aState.prevMaxPosBottomMargin = bottomMargin;
        }
      }
      childCount++;
      kidFrame->GetContentIndex(mLastContentOffset);

      // Remember where we just were in case we end up pushing children
      prevKidFrame = kidFrame;

      // Update mLastContentIsComplete now that this kid fits
      mLastContentIsComplete = PRBool(NS_FRAME_IS_COMPLETE(status));

      // Special handling for incomplete children
      if (NS_FRAME_IS_NOT_COMPLETE(status)) {
        nsIFrame* kidNextInFlow;
         
        kidFrame->GetNextInFlow(kidNextInFlow);
        PRBool lastContentIsComplete = mLastContentIsComplete;
        if (nsnull == kidNextInFlow) {
          // The child doesn't have a next-in-flow so create a continuing
          // frame. This hooks the child into the flow
          nsIFrame* continuingFrame;
           
          nsIStyleContext* kidSC;
          kidFrame->GetStyleContext(aPresContext, kidSC);
          kidFrame->CreateContinuingFrame(*aPresContext, this, kidSC, continuingFrame);
          NS_RELEASE(kidSC);
          NS_ASSERTION(nsnull != continuingFrame, "frame creation failed");

          // Add the continuing frame to the sibling list
          nsIFrame* nextSib;
           
          kidFrame->GetNextSibling(nextSib);
          continuingFrame->SetNextSibling(nextSib);
          kidFrame->SetNextSibling(continuingFrame);
          if (nsnull == nextSib) {
            // Assume that the continuation frame we just created is
            // complete, for now. It will get reflowed by our
            // next-in-flow (we are going to push it now)
            lastContentIsComplete = PR_TRUE;
          }
        }
        // We've used up all of our available space so push the remaining
        // children to the next-in-flow
        nsIFrame* nextSibling;
         
        kidFrame->GetNextSibling(nextSibling);
        if (nsnull != nextSibling) {
          PushChildren(nextSibling, kidFrame, lastContentIsComplete);
          PRInt32 contentIndex;
          prevKidFrame->GetContentIndex(contentIndex);
          SetLastContentOffset(contentIndex);
        }
        result = PR_FALSE;
        break;
      }
    }

    // Get the next child
    kidFrame->GetNextSibling(kidFrame);

    // XXX talk with troy about checking for available space here
  }

  // Update the child count
  mChildCount = childCount;
#ifdef NS_DEBUG
  NS_POSTCONDITION(LengthOf(mFirstChild) == mChildCount, "bad child count");

  nsIFrame* lastChild;
  PRInt32   lastIndexInParent;

  LastChild(lastChild);
  lastChild->GetContentIndex(lastIndexInParent);
  NS_POSTCONDITION(lastIndexInParent == mLastContentOffset, "bad last content offset");
#endif

#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
#ifdef NOISY
  ListTag(stdout);
  printf(": reflow mapped %sok (childCount=%d) [%d,%d,%c]\n",
         (result ? "" : "NOT "),
         mChildCount,
         mFirstContentOffset, mLastContentOffset,
         (mLastContentIsComplete ? 'T' : 'F'));
#ifdef NOISY_FLOW
  {
    nsTableFrame* flow = (nsTableFrame*) mNextInFlow;
    while (flow != 0) {
      printf("  %p: [%d,%d,%c]\n",
             flow, flow->mFirstContentOffset, flow->mLastContentOffset,
             (flow->mLastContentIsComplete ? 'T' : 'F'));
      flow = (nsTableFrame*) flow->mNextInFlow;
    }
  }
#endif
#endif
  return result;
}

/**
 * Try and pull-up frames from our next-in-flow
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @return  true if we successfully pulled-up all the children and false
 *            otherwise, e.g. child didn't fit
 */
PRBool nsTableFrame::PullUpChildren(nsIPresContext*      aPresContext,
                                    InnerTableReflowState& aState,
                                    nsSize*              aMaxElementSize)
{
#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
#ifdef NOISY
  ListTag(stdout);
  printf(": pullup (childCount=%d) [%d,%d,%c]\n",
         mChildCount,
         mFirstContentOffset, mLastContentOffset,
         (mLastContentIsComplete ? 'T' : 'F'));
#ifdef NOISY_FLOW
  {
    nsTableFrame* flow = (nsTableFrame*) mNextInFlow;
    while (flow != 0) {
      printf("  %p: [%d,%d,%c]\n",
             flow, flow->mFirstContentOffset, flow->mLastContentOffset,
             (flow->mLastContentIsComplete ? 'T' : 'F'));
      flow = (nsTableFrame*) flow->mNextInFlow;
    }
  }
#endif
#endif
  nsTableFrame* nextInFlow = (nsTableFrame*)mNextInFlow;
  nsSize         kidMaxElementSize(0,0);
  nsSize*        pKidMaxElementSize = (nsnull != aMaxElementSize) ? &kidMaxElementSize : nsnull;
#ifdef NS_DEBUG
  PRInt32        kidIndex = NextChildOffset();
#endif
  nsIFrame*      prevKidFrame;
   
  LastChild(prevKidFrame);

  // This will hold the prevKidFrame's mLastContentIsComplete
  // status. If we have to push the frame that follows prevKidFrame
  // then this will become our mLastContentIsComplete state. Since
  // prevKidFrame is initially our last frame, it's completion status
  // is our mLastContentIsComplete value.
  PRBool        prevLastContentIsComplete = mLastContentIsComplete;

  PRBool        result = PR_TRUE;

  while (nsnull != nextInFlow) {
    nsReflowMetrics kidSize(pKidMaxElementSize);
    kidSize.width=kidSize.height=kidSize.ascent=kidSize.descent=0;
    nsReflowStatus  status;

    // Get the next child
    nsIFrame* kidFrame = nextInFlow->mFirstChild;

    // Any more child frames?
    if (nsnull == kidFrame) {
      // No. Any frames on its overflow list?
      if (nsnull != nextInFlow->mOverflowList) {
        // Move the overflow list to become the child list
        nextInFlow->AppendChildren(nextInFlow->mOverflowList);
        nextInFlow->mOverflowList = nsnull;
        kidFrame = nextInFlow->mFirstChild;
      } else {
        // We've pulled up all the children, so move to the next-in-flow.
        nextInFlow->GetNextInFlow((nsIFrame*&)nextInFlow);
        continue;
      }
    }

    // See if the child fits in the available space. If it fits or
    // it's splittable then reflow it. The reason we can't just move
    // it is that we still need ascent/descent information
    nsSize            kidFrameSize(0,0);
    nsSplittableType  kidIsSplittable;

    kidFrame->GetSize(kidFrameSize);
    kidFrame->IsSplittable(kidIsSplittable);
    if ((kidFrameSize.height > aState.availSize.height) &&
        NS_FRAME_IS_NOT_SPLITTABLE(kidIsSplittable)) {
      result = PR_FALSE;
      mLastContentIsComplete = prevLastContentIsComplete;
      break;
    }
    nsReflowState kidReflowState(kidFrame, aState.reflowState, aState.availSize,
                                 eReflowReason_Resize);
    kidFrame->WillReflow(*aPresContext);
    status = ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState);

    // Did the child fit?
    if ((kidSize.height > aState.availSize.height) && (nsnull != mFirstChild)) {
      // The child is too wide to fit in the available space, and it's
      // not our first child
      result = PR_FALSE;
      mLastContentIsComplete = prevLastContentIsComplete;
      break;
    }

    // Advance y by the topMargin between children. Zero out the
    // topMargin in case this frame is continued because
    // continuations do not have a top margin. Update the prev
    // bottom margin state in the body reflow state so that we can
    // apply the bottom margin when we hit the next child (or
    // finish).
    //aState.y += topMargin;
    nsRect kidRect (0, 0, kidSize.width, kidSize.height);
    //kidRect.x += kidMol->margin.left;
    kidRect.y += aState.y;
    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      PlaceChild(aPresContext, aState, kidFrame, kidRect, aMaxElementSize, *pKidMaxElementSize);
    }

    // Remove the frame from its current parent
    kidFrame->GetNextSibling(nextInFlow->mFirstChild);
    nextInFlow->mChildCount--;
    // Update the next-in-flows first content offset
    if (nsnull != nextInFlow->mFirstChild) {
      PRInt32 contentIndex;
      nextInFlow->mFirstChild->GetContentIndex(contentIndex);
      nextInFlow->SetFirstContentOffset(contentIndex);
    }

    // Link the frame into our list of children
    kidFrame->SetGeometricParent(this);
    nsIFrame* contentParent;

    kidFrame->GetContentParent(contentParent);
    if (nextInFlow == contentParent) {
      kidFrame->SetContentParent(this);
    }
    if (nsnull == prevKidFrame) {
      mFirstChild = kidFrame;
      PRInt32 contentIndex;
      kidFrame->GetContentIndex(contentIndex);
      SetFirstContentOffset(contentIndex);
    } else {
      prevKidFrame->SetNextSibling(kidFrame);
    }
    kidFrame->SetNextSibling(nsnull);
    mChildCount++;

    // Remember where we just were in case we end up pushing children
    prevKidFrame = kidFrame;
    prevLastContentIsComplete = mLastContentIsComplete;

    // Is the child we just pulled up complete?
    mLastContentIsComplete = PRBool(NS_FRAME_IS_COMPLETE(status));
    if (NS_FRAME_IS_NOT_COMPLETE(status)) {
      // No the child isn't complete
      nsIFrame* kidNextInFlow;
       
      kidFrame->GetNextInFlow(kidNextInFlow);
      if (nsnull == kidNextInFlow) {
        // The child doesn't have a next-in-flow so create a
        // continuing frame. The creation appends it to the flow and
        // prepares it for reflow.
        nsIFrame* continuingFrame;

        nsIStyleContext* kidSC;
        kidFrame->GetStyleContext(aPresContext, kidSC);
        kidFrame->CreateContinuingFrame(*aPresContext, this, kidSC, continuingFrame);
        NS_RELEASE(kidSC);
        NS_ASSERTION(nsnull != continuingFrame, "frame creation failed");

        // Add the continuing frame to our sibling list and then push
        // it to the next-in-flow. This ensures the next-in-flow's
        // content offsets and child count are set properly. Note that
        // we can safely assume that the continuation is complete so
        // we pass PR_TRUE into PushChidren in case our next-in-flow
        // was just drained and now needs to know it's
        // mLastContentIsComplete state.
        kidFrame->SetNextSibling(continuingFrame);

        PushChildren(continuingFrame, kidFrame, PR_TRUE);

        // After we push the continuation frame we don't need to fuss
        // with mLastContentIsComplete beause the continuation frame
        // is no longer on *our* list.
      }

      // If the child isn't complete then it means that we've used up
      // all of our available space.
      result = PR_FALSE;
      break;
    }
  }

  // Update our last content offset
  if (nsnull != prevKidFrame) {
    NS_ASSERTION(IsLastChild(prevKidFrame), "bad last child");
    PRInt32 contentIndex;
    prevKidFrame->GetContentIndex(contentIndex);
    SetLastContentOffset(contentIndex);
  }

  // We need to make sure the first content offset is correct for any empty
  // next-in-flow frames (frames where we pulled up all the child frames)
  nextInFlow = (nsTableFrame*)mNextInFlow;
  if ((nsnull != nextInFlow) && (nsnull == nextInFlow->mFirstChild)) {
    // We have at least one empty frame. Did we succesfully pull up all the
    // child frames?
    if (PR_FALSE == result) {
      // No, so we need to adjust the first content offset of all the empty
      // frames
      AdjustOffsetOfEmptyNextInFlows();
#ifdef NS_DEBUG
    } else {
      // Yes, we successfully pulled up all the child frames which means all
      // the next-in-flows must be empty. Do a sanity check
      while (nsnull != nextInFlow) {
        NS_ASSERTION(nsnull == nextInFlow->mFirstChild, "non-empty next-in-flow");
        nextInFlow->GetNextInFlow((nsIFrame*&)nextInFlow);
      }
#endif
    }
  }

#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
  return result;
}

/**
 * Create new frames for content we haven't yet mapped
 *
 * @param   aPresContext presentation context to use
 * @param   aState current inline state
 * @return  frComplete if all content has been mapped and frNotComplete
 *            if we should be continued
 */
nsReflowStatus
nsTableFrame::ReflowUnmappedChildren(nsIPresContext*      aPresContext,
                                     InnerTableReflowState& aState,
                                     nsSize*              aMaxElementSize)
{
#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
  nsIFrame*    kidPrevInFlow = nsnull;
  nsReflowStatus result = NS_FRAME_NOT_COMPLETE;

  // If we have no children and we have a prev-in-flow then we need to pick
  // up where it left off. If we have children, e.g. we're being resized, then
  // our content offset should already be set correctly...
  if ((nsnull == mFirstChild) && (nsnull != mPrevInFlow)) {
    nsTableFrame* prev = (nsTableFrame*)mPrevInFlow;
    NS_ASSERTION(prev->mLastContentOffset >= prev->mFirstContentOffset, "bad prevInFlow");

    mFirstContentOffset = prev->NextChildOffset();
    if (!prev->mLastContentIsComplete) {
      // Our prev-in-flow's last child is not complete
      prev->LastChild(kidPrevInFlow);
    }
  }
  mLastContentIsComplete = PR_TRUE;

  // Place our children, one at a time until we are out of children
  nsSize    kidMaxElementSize(0,0);
  nsSize*   pKidMaxElementSize = (nsnull != aMaxElementSize) ? &kidMaxElementSize : nsnull;
  PRInt32   kidIndex = NextChildOffset();
  nsIFrame* prevKidFrame;

  LastChild(prevKidFrame);
  for (;;) {
    // Get the next content object
    nsIContentPtr kid;
    mContent->ChildAt(kidIndex, kid.AssignRef());
    if (kid.IsNull()) {
      result = NS_FRAME_COMPLETE;
      break;
    }

    // Make sure we still have room left
    if (aState.availSize.height <= 0) {
      // Note: return status was set to frNotComplete above...
      break;
    }

    // Resolve style for the child
    nsIStyleContextPtr kidStyleContext =
      aPresContext->ResolveStyleContextFor(kid, this, PR_TRUE);

    // Figure out how we should treat the child
    nsIFrame*        kidFrame;

    // Create a child frame
    if (nsnull == kidPrevInFlow) {
      nsIContentDelegate* kidDel = nsnull;
      kidDel = kid->GetDelegate(aPresContext);
      nsresult rv = kidDel->CreateFrame(aPresContext, kid, this,
                                        kidStyleContext, kidFrame);
      NS_RELEASE(kidDel);
    } else {
      kidPrevInFlow->CreateContinuingFrame(*aPresContext, this, kidStyleContext,
                                           kidFrame);
    }

    // Link child frame into the list of children
    if (nsnull != prevKidFrame) {
      prevKidFrame->SetNextSibling(kidFrame);
    } else {
      mFirstChild = kidFrame;  // our first child
      SetFirstContentOffset(kidIndex);
    }
    mChildCount++;

    // Try to reflow the child into the available space. It might not
    // fit or might need continuing.
    nsReflowMetrics kidSize(pKidMaxElementSize);
    kidSize.width=kidSize.height=kidSize.ascent=kidSize.descent=0;
    nsReflowState   kidReflowState(kidFrame, aState.reflowState, aState.availSize,
                                   eReflowReason_Initial);
    kidFrame->WillReflow(*aPresContext);
    kidFrame->MoveTo(0, aState.y);
    nsReflowStatus status = ReflowChild(kidFrame,aPresContext, kidSize, kidReflowState);

    // Did the child fit?
    if ((kidSize.height > aState.availSize.height) && (nsnull != mFirstChild)) {
      // The child is too wide to fit in the available space, and it's
      // not our first child. Add the frame to our overflow list
      NS_ASSERTION(nsnull == mOverflowList, "bad overflow list");
      mOverflowList = kidFrame;
      prevKidFrame->SetNextSibling(nsnull);
      break;
    }

    // Advance y by the topMargin between children. Zero out the
    // topMargin in case this frame is continued because
    // continuations do not have a top margin. Update the prev
    // bottom margin state in the body reflow state so that we can
    // apply the bottom margin when we hit the next child (or
    // finish).
    //aState.y += topMargin;
    nsRect kidRect (0, aState.y, kidSize.width, kidSize.height);
    const nsStyleDisplay *childDisplay;
    kidFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
    if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
        NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
    {
      PlaceChild(aPresContext, aState, kidFrame, kidRect, aMaxElementSize, *pKidMaxElementSize);
    }

    prevKidFrame = kidFrame;
    kidIndex++;

    // Did the child complete?
    if (NS_FRAME_IS_NOT_COMPLETE(status)) {
      // If the child isn't complete then it means that we've used up
      // all of our available space
      mLastContentIsComplete = PR_FALSE;
      break;
    }
    kidPrevInFlow = nsnull;
  }

  // Update the content mapping
  NS_ASSERTION(IsLastChild(prevKidFrame), "bad last child");
  PRInt32 contentIndex;
  prevKidFrame->GetContentIndex(contentIndex);
  SetLastContentOffset(contentIndex);
#ifdef NS_DEBUG
  PRInt32 len = LengthOf(mFirstChild);
  NS_ASSERTION(len == mChildCount, "bad child count");
#endif
#ifdef NS_DEBUG
  VerifyLastIsComplete();
#endif
  return result;
}


/**
  Now I've got all the cells laid out in an infinite space.
  For each column, use the min size for each cell in that column
  along with the attributes of the table, column group, and column
  to assign widths to each column.
  */
// use the cell map to determine which cell is in which column.
void nsTableFrame::BalanceColumnWidths(nsIPresContext* aPresContext, 
                                       const nsReflowState& aReflowState,
                                       const nsSize& aMaxSize, 
                                       nsSize* aMaxElementSize)
{
  NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");

  PRInt32 numCols = GetColCount();
  if (nsnull==mColumnWidths)
  {
    mColumnWidths = new PRInt32[numCols];
    nsCRT::memset (mColumnWidths, 0, numCols*sizeof(PRInt32));
  }

  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);

  // need to figure out the overall table width constraint
  // default case, get 100% of available space

  PRInt32 maxWidth;
  const nsStylePosition* position =
    (const nsStylePosition*)mStyleContext->GetStyleData(eStyleStruct_Position);
  switch (position->mWidth.GetUnit()) {
  case eStyleUnit_Coord:
    maxWidth = position->mWidth.GetCoordValue();
    break;

  case eStyleUnit_Auto:
    maxWidth = aMaxSize.width;
    break;

  case eStyleUnit_Percent:
  case eStyleUnit_Proportional:
  case eStyleUnit_Inherit:
    // XXX for now these fall through

  default:
    maxWidth = aMaxSize.width;
    break;
  }


  if (0>maxWidth)  // nonsense style specification
    maxWidth = 0;

  if (PR_TRUE==gsDebug || PR_TRUE==gsDebugNT) 
    printf ("%p: maxWidth=%d from aMaxSize=%d,%d\n", 
            this, maxWidth, aMaxSize.width, aMaxSize.height);

  // based on the compatibility mode, create a table layout strategy
  if (nsnull==mTableLayoutStrategy)
  { // TODO:  build a different strategy based on the compatibility mode
    mTableLayoutStrategy = new BasicTableLayoutStrategy(this, numCols);
    mTableLayoutStrategy->Initialize(aMaxElementSize);
  }
  mTableLayoutStrategy->BalanceColumnWidths(mStyleContext, aReflowState, maxWidth);
}

/**
  sum the width of each column
  add in table insets
  set rect
  */
void nsTableFrame::SetTableWidth(nsIPresContext* aPresContext)
{
  NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");

  nscoord cellSpacing = GetCellSpacing();
  if (gsDebug==PR_TRUE) 
    printf ("SetTableWidth with cellSpacing = %d ", cellSpacing);
  PRInt32 tableWidth = cellSpacing;

  PRInt32 numCols = GetColCount();
  for (PRInt32 colIndex = 0; colIndex<numCols; colIndex++)
  {
    nscoord totalColWidth = mColumnWidths[colIndex];
    totalColWidth += cellSpacing;
    if (gsDebug==PR_TRUE) 
      printf (" += %d ", totalColWidth);
    tableWidth += totalColWidth;
  }

  // Compute the insets (sum of border and padding)
  const nsStyleSpacing* spacing =
    (const nsStyleSpacing*)mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  spacing->CalcBorderPaddingFor(this, borderPadding);

  nscoord rightInset = borderPadding.right;
  nscoord leftInset = borderPadding.left;
  tableWidth += (leftInset + rightInset);
  nsRect tableSize = mRect;
  tableSize.width = tableWidth;
  if (PR_TRUE==gsDebug  ||  PR_TRUE==gsDebugNT)
  {
    printf ("%p: setting table rect to %d, %d after adding insets %d, %d\n", 
            this, tableSize.width, tableSize.height, rightInset, leftInset);
  }
  SetRect(tableSize);
}

void nsTableFrame::VerticallyAlignChildren(nsIPresContext* aPresContext,
                                           nscoord* aAscents,
                                           nscoord aMaxAscent,
                                           nscoord aMaxHeight)
{
}

void nsTableFrame::AdjustColumnsForCOLSAttribute()
{
  nsCellMap *cellMap = GetCellMap();
  NS_ASSERTION(nsnull!=cellMap, "bad cell map");
  
  // any specified-width column turns off COLS attribute
  nsStyleTable* tableStyle = (nsStyleTable *)mStyleContext->GetMutableStyleData(eStyleStruct_Table);
  if (tableStyle->mCols != NS_STYLE_TABLE_COLS_NONE)
  {
    PRInt32 numCols = GetColCount();
    PRInt32 numRows = GetRowCount();
    for (PRInt32 rowIndex=0; rowIndex<numRows; rowIndex++)
    {
      for (PRInt32 colIndex=0; colIndex<numCols; colIndex++)
      {
        nsTableCellFrame *cellFrame = cellMap->GetCellFrameAt(rowIndex, colIndex);
        // get the cell style info
        const nsStylePosition* cellPosition;
        if (nsnull!=cellFrame)
        {
          cellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
          if ((eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) ||
               (eStyleUnit_Percent==cellPosition->mWidth.GetUnit())) 
          {
            tableStyle->mCols = NS_STYLE_TABLE_COLS_NONE;
            break;
          }
        }
      }
    }
  }
}

NS_METHOD
nsTableFrame::SetColumnStyleFromCell(nsIPresContext  * aPresContext,
                                     nsTableCellFrame* aCellFrame,
                                     nsTableRowFrame * aRowFrame)
{
  // if this cell is the first non-col-spanning cell to have a width attribute, 
  // then the width attribute also acts as the width attribute for the entire column
  // if the cell has a colspan, the width is used provisionally, divided equally among 
  // the spanned columns
  if ((nsnull!=aPresContext) && (nsnull!=aCellFrame) && (nsnull!=aRowFrame))
  {
    // get the cell style info
    const nsStylePosition* cellPosition;
    aCellFrame->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)cellPosition);
    if ((eStyleUnit_Coord == cellPosition->mWidth.GetUnit()) ||
         (eStyleUnit_Percent==cellPosition->mWidth.GetUnit())) {
      // compute the width per column spanned
      PRInt32 colSpan = GetEffectiveColSpan(aCellFrame->GetColIndex(), aCellFrame);
      for (PRInt32 i=0; i<colSpan; i++)
      {
        // get the appropriate column frame
        nsTableColFrame *colFrame;
        GetColumnFrame(i+aCellFrame->GetColIndex(), colFrame);
        if (nsTableColFrame::eWIDTH_SOURCE_CELL != colFrame->GetWidthSource()) 
        {
          if ((1==colSpan) ||
              (nsTableColFrame::eWIDTH_SOURCE_CELL_WITH_SPAN != colFrame->GetWidthSource()))
          {
            // get the column style and set the width attribute
            nsIStyleContext *colSC;
            colFrame->GetStyleContext(aPresContext, colSC);
            nsStylePosition* colPosition = (nsStylePosition*) colSC->GetMutableStyleData(eStyleStruct_Position);
            NS_RELEASE(colSC);
            // set the column width attribute
            if (eStyleUnit_Coord == cellPosition->mWidth.GetUnit())
            {
              nscoord width = cellPosition->mWidth.GetCoordValue();
              colPosition->mWidth.SetCoordValue(width/colSpan);
            }
            else
            {
              float width = cellPosition->mWidth.GetPercentValue();
              colPosition->mWidth.SetPercentValue(width/colSpan);
            }
            // set the column width-set-type
            if (1==colSpan)
              colFrame->SetWidthSource(nsTableColFrame::eWIDTH_SOURCE_CELL);
            else
              colFrame->SetWidthSource(nsTableColFrame::eWIDTH_SOURCE_CELL_WITH_SPAN);
          }
        }
      }
    }
  }
  return NS_OK;
}

/* there's an easy way and a hard way.  The easy way is to look in our
 * cache and pull the frame from there.
 * If the cache isn't built yet, then we have to go hunting.
 */
NS_METHOD nsTableFrame::GetColumnFrame(PRInt32 aColIndex, nsTableColFrame *&aColFrame)
{
  aColFrame = nsnull; // initialize out parameter
  nsCellMap *cellMap = GetCellMap();
  if (nsnull!=cellMap)
  { // hooray, we get to do this the easy way because the info is cached
    aColFrame = cellMap->GetColumnFrame(aColIndex);
    NS_ASSERTION(nsnull!=aColFrame, "bad col frame");
  }
  else
  { // ah shucks, we have to go hunt for the column frame brute-force style
    nsIFrame *childFrame;
    FirstChild(childFrame);
    for (;;)
    {
      if (nsnull==childFrame)
      {
        NS_ASSERTION (PR_FALSE, "scanned the frame hierarchy and no column frame could be found.");
        break;
      }
      const nsStyleDisplay *childDisplay;
      childFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
      {
        PRInt32 colGroupStartingIndex = ((nsTableColGroupFrame *)childFrame)->GetStartColumnIndex();
        if (aColIndex >= colGroupStartingIndex)
        { // the cell's col might be in this col group
          PRInt32 childCount;
          childFrame->ChildCount(childCount);
          if (aColIndex < colGroupStartingIndex + childCount)
          { // yep, we've found it
            childFrame->ChildAt(aColIndex-colGroupStartingIndex, (nsIFrame *&)aColFrame);
            break;
          }
        }
      }
      childFrame->GetNextSibling(childFrame);
    }
  }
  return NS_OK;
}

/* We have to go through our child list twice.
 * The first time, we scan until we find the first row.  
 * We set column style from the cells in the first row.
 * Then we terminate that loop and start a second pass.
 * In the second pass, we build column and cell cache info.
 */
void nsTableFrame::BuildColumnCache( nsIPresContext*      aPresContext,
                                     nsReflowMetrics&     aDesiredSize,
                                     const nsReflowState& aReflowState,
                                     nsReflowStatus&      aStatus
                                    )
{
  // probably want this assertion : NS_ASSERTION(nsnull==mPrevInFlow, "never ever call me on a continuing frame!");
  NS_ASSERTION(nsnull!=mCellMap, "never ever call me until the cell map is built!");
  EnsureColumns(aPresContext, aDesiredSize, aReflowState, aStatus);
  if (nsnull==mColCache)
  {
    mColCache = new ColumnInfoCache(mColCount);
    nsIFrame * childFrame = mFirstChild;
    while (nsnull!=childFrame)
    { // in this loop, we cache column info and set column style info from cells in first row
      const nsStyleDisplay *childDisplay;
      childFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));

      if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
      { // if it's a col group then get the columns and cache them in the CellMap
        nsTableColFrame *colFrame=nsnull;
        childFrame->FirstChild((nsIFrame *&)colFrame);
        while (nsnull!=colFrame)
        { 
          mCellMap->AppendColumnFrame(colFrame);
          colFrame->GetNextSibling((nsIFrame *&)colFrame);
        }
      }
      else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
               NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
               NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
      { // if it's a row group, get the cells and set the column style if appropriate
        nsIFrame *rowFrame;
        childFrame->FirstChild(rowFrame);
        while (nsnull!=rowFrame)
        {
          nsIFrame *cellFrame;
          rowFrame->FirstChild(cellFrame);
          while (nsnull!=cellFrame)
          {
            /* this is the first time we are guaranteed to have both the cell frames
             * and the column frames, so it's a good time to 
             * set the column style from the cell's width attribute (if this is the first row)
             */
            SetColumnStyleFromCell(aPresContext, (nsTableCellFrame *)cellFrame, (nsTableRowFrame *)rowFrame);
            cellFrame->GetNextSibling(cellFrame);
          }
          rowFrame->GetNextSibling(rowFrame);
        }
      }
      childFrame->GetNextSibling(childFrame);
    }

    // second time through, set column cache info for each column
    // we can't do this until the loop above has set the column style info from the cells in the first row
    childFrame = mFirstChild;
    while (nsnull!=childFrame)
    { // for every child, if it's a col group then get the columns
      const nsStyleDisplay *childDisplay;
      childFrame->GetStyleData(eStyleStruct_Display, ((nsStyleStruct *&)childDisplay));
      if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
      {
        nsTableColFrame *colFrame=nsnull;
        childFrame->FirstChild((nsIFrame *&)colFrame);
        while (nsnull!=colFrame)
        { // for every column, create an entry in the column cache
          // assumes that the col style has been twiddled to account for first cell width attribute
          const nsStylePosition* colPosition;
          colFrame->GetStyleData(eStyleStruct_Position, ((nsStyleStruct *&)colPosition));
          mColCache->AddColumnInfo(colPosition->mWidth.GetUnit(), colFrame->GetColumnIndex());
          colFrame->GetNextSibling((nsIFrame *&)colFrame);
        }
      }
      else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay ||
               NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay ||
               NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay )
      {
        break;  // once we hit a row group, we're done
      }
      childFrame->GetNextSibling(childFrame);
    }
  }
}

PRInt32 nsTableFrame::GetReflowPass() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return firstInFlow->mPass;
}

void nsTableFrame::SetReflowPass(PRInt32 aReflowPass)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  firstInFlow->mPass = aReflowPass;
}

PRBool nsTableFrame::IsFirstPassValid() const
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  return firstInFlow->mFirstPassValid;
}

NS_METHOD
nsTableFrame::CreateContinuingFrame(nsIPresContext&  aPresContext,
                                    nsIFrame*        aParent,
                                    nsIStyleContext* aStyleContext,
                                    nsIFrame*&       aContinuingFrame)
{
  nsTableFrame* cf = new nsTableFrame(mContent, aParent);
  if (nsnull == cf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PrepareContinuingFrame(aPresContext, aParent, aStyleContext, cf);
  if (PR_TRUE==gsDebug) printf("nsTableFrame::CCF parent = %p, this=%p, cf=%p\n", aParent, this, cf);
  // set my width, because all frames in a table flow are the same width
  // code in nsTableOuterFrame depends on this being set
  cf->SetRect(nsRect(0, 0, mRect.width, 0));
  // add headers and footers to cf
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  nsIFrame * rg = nsnull;
  firstInFlow->FirstChild(rg);
  NS_ASSERTION (nsnull!=rg, "previous frame has no children");
  PRInt32 index = 0;
  nsIFrame * bodyRowGroupFromOverflow = mOverflowList;
  nsIFrame * lastSib = nsnull;
  for ( ; nsnull!=rg; index++)
  {
    nsIContent *content = nsnull;
    rg->GetContent(content);                                              // content: REFCNT++
    NS_ASSERTION(nsnull!=content, "bad frame, returned null content.");
    const nsStyleDisplay* display;
    GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&)display);
    if ((display->mDisplay == NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) || 
        (display->mDisplay == NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP)) 
    {
      printf("found a head or foot in continuing frame\n");
      // Resolve style for the child
      nsIStyleContext* kidStyleContext =
        aPresContext.ResolveStyleContextFor(content, cf);               // kidStyleContext: REFCNT++
      nsIContentDelegate* kidDel = nsnull;
      kidDel = content->GetDelegate(&aPresContext);                       // kidDel: REFCNT++
      nsIFrame* duplicateFrame;
      nsresult rv = kidDel->CreateFrame(&aPresContext, content, cf,
                                        kidStyleContext, duplicateFrame);
      NS_RELEASE(kidDel);                                                // kidDel: REFCNT--
      NS_RELEASE(kidStyleContext);                                       // kidStyleContenxt: REFCNT--
      
      if (nsnull==lastSib)
      {
        mOverflowList = duplicateFrame;
      }
      else
      {
        lastSib->SetNextSibling(duplicateFrame);
      }
      duplicateFrame->SetNextSibling(bodyRowGroupFromOverflow);
      lastSib = duplicateFrame;
    }
    NS_RELEASE(content);                                                 // content: REFCNT--
    // get the next row group
    rg->GetNextSibling(rg);
  }
  aContinuingFrame = cf;
  return NS_OK;
}

PRInt32 nsTableFrame::GetColumnWidth(PRInt32 aColIndex)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");
  PRInt32 result = 0;
  if (this!=firstInFlow)
    result = firstInFlow->GetColumnWidth(aColIndex);
  else
  {
    NS_ASSERTION(nsnull!=mColumnWidths, "illegal state");
#ifdef NS_DEBUG
    NS_ASSERTION(nsnull!=mCellMap, "no cell map");
    PRInt32 numCols = GetColCount();
    NS_ASSERTION (numCols > aColIndex, "bad arg, col index out of bounds");
#endif
    if (nsnull!=mColumnWidths)
     result = mColumnWidths[aColIndex];
  }

  //printf("GET_COL_WIDTH: %p, FIF=%p getting col %d and returning %d\n", this, firstInFlow, aColIndex, result);

  // XXX hack
  return result;
}

void  nsTableFrame::SetColumnWidth(PRInt32 aColIndex, nscoord aWidth)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(nsnull!=firstInFlow, "illegal state -- no first in flow");

  if (this!=firstInFlow)
    firstInFlow->SetColumnWidth(aColIndex, aWidth);
  else
  {
    NS_ASSERTION(nsnull!=mColumnWidths, "illegal state");
    if (nsnull!=mColumnWidths)
      mColumnWidths[aColIndex] = aWidth;
  }
}

/**
  *
  * Update the border style to map to the HTML border style
  *
  */
void nsTableFrame::MapHTMLBorderStyle(nsStyleSpacing& aSpacingStyle, nscoord aBorderWidth)
{
  nsStyleCoord  width;
  width.SetCoordValue(aBorderWidth);
  aSpacingStyle.mBorder.SetTop(width);
  aSpacingStyle.mBorder.SetLeft(width);
  aSpacingStyle.mBorder.SetBottom(width);
  aSpacingStyle.mBorder.SetRight(width);

  aSpacingStyle.mBorderStyle[NS_SIDE_TOP] = NS_STYLE_BORDER_STYLE_OUTSET; 
  aSpacingStyle.mBorderStyle[NS_SIDE_LEFT] = NS_STYLE_BORDER_STYLE_OUTSET; 
  aSpacingStyle.mBorderStyle[NS_SIDE_BOTTOM] = NS_STYLE_BORDER_STYLE_OUTSET; 
  aSpacingStyle.mBorderStyle[NS_SIDE_RIGHT] = NS_STYLE_BORDER_STYLE_OUTSET; 

  nsIStyleContext* styleContext = mStyleContext; 
  const nsStyleColor* colorData = (const nsStyleColor*)
    styleContext->GetStyleData(eStyleStruct_Color);

  // Look until we find a style context with a NON-transparent background color
  while (styleContext)
  {
    if ((colorData->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)!=0)
    {
      nsIStyleContext* temp = styleContext;
      styleContext = styleContext->GetParent();
      if (temp != mStyleContext)
        NS_RELEASE(temp);
      colorData = (const nsStyleColor*)styleContext->GetStyleData(eStyleStruct_Color);
    }
    else
    {
      break;
    }
  }

  // Yaahoo, we found a style context which has a background color 
  
  nscolor borderColor = 0xFFC0C0C0;

  if (styleContext != nsnull)
  {
    borderColor = colorData->mBackgroundColor;
    if (styleContext != mStyleContext)
      NS_RELEASE(styleContext);
  }

  // if the border color is white, then shift to grey
  if (borderColor == 0xFFFFFFFF)
    borderColor = 0xFFC0C0C0;

  aSpacingStyle.mBorderColor[NS_SIDE_TOP]     = borderColor;
  aSpacingStyle.mBorderColor[NS_SIDE_LEFT]    = borderColor;
  aSpacingStyle.mBorderColor[NS_SIDE_BOTTOM]  = borderColor;
  aSpacingStyle.mBorderColor[NS_SIDE_RIGHT]   = borderColor;

}



PRBool nsTableFrame::ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult)
{
  PRInt32 result = 0;

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

void nsTableFrame::MapBorderMarginPadding(nsIPresContext* aPresContext)
{
#if 0
  // Check to see if the table has either cell padding or 
  // Cell spacing defined for the table. If true, then
  // this setting overrides any specific border, margin or 
  // padding information in the cell. If these attributes
  // are not defined, the the cells attributes are used
  
  nsHTMLValue padding_value;
  nsHTMLValue spacing_value;
  nsHTMLValue border_value;


  nsresult border_result;

  nscoord   padding = 0;
  nscoord   spacing = 0;
  nscoord   border  = 1;

  float     p2t = aPresContext->GetPixelsToTwips();

  nsIHTMLContent*  table = (nsIHTMLContent*)mContent;

  NS_ASSERTION(table,"Table Must not be null");
  if (!table)
    return;

  nsStyleSpacing* spacingData = (nsStyleSpacing*)mStyleContext->GetMutableStyleData(eStyleStruct_Spacing);

  border_result = table->GetAttribute(nsHTMLAtoms::border,border_value);
  if (border_result == NS_CONTENT_ATTR_HAS_VALUE)
  {
    PRInt32 intValue = 0;

    if (ConvertToPixelValue(border_value,1,intValue))
      border = NSIntPixelsToTwips(intValue, p2t); 
  }
  MapHTMLBorderStyle(*spacingData,border);
#endif
}




// Subclass hook for style post processing
NS_METHOD nsTableFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
#ifdef NOISY_STYLE
  printf("nsTableFrame::DidSetStyleContext \n");
#endif
  return NS_OK;
}

NS_METHOD nsTableFrame::GetCellMarginData(nsTableCellFrame* aKidFrame, nsMargin& aMargin)
{
  nsresult result = NS_ERROR_NOT_INITIALIZED;

  if (nsnull != aKidFrame)
  {
    result = aKidFrame->GetMargin(aMargin);
  }

  return result;
}

nscoord nsTableFrame::GetCellSpacing()
{
  nsTableFrame* tableFrame = this;
  nsStyleTable* tableStyle;
  GetStyleData(eStyleStruct_Table, (nsStyleStruct *&)tableStyle);
  nscoord cellSpacing = 0;
  if (tableStyle->mCellSpacing.GetUnit() == eStyleUnit_Coord)
    cellSpacing = tableStyle->mCellSpacing.GetCoordValue();
  return cellSpacing;
}

void nsTableFrame::GetColumnsByType(const nsStyleUnit aType, 
                                    PRInt32& aOutNumColumns,
                                    PRInt32 *& aOutColumnIndexes)
{
  mColCache->GetColumnsByType(aType, aOutNumColumns, aOutColumnIndexes);
}


NS_METHOD
nsTableFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kTableFrameCID)) {
    *aInstancePtr = (void*) (this);
    return NS_OK;
  }
  return nsContainerFrame::QueryInterface(aIID, aInstancePtr);
}


/* ----- global methods ----- */

nsresult 
NS_NewTableFrame(nsIContent* aContent,
                 nsIFrame*   aParentFrame,
                 nsIFrame*&  aResult)
{
  nsIFrame* it = new nsTableFrame(aContent, aParentFrame);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = it;
  return NS_OK;
}

/* helper method for determining if this is a nested table or not */
PRBool nsTableFrame::IsNested(const nsReflowState& aReflowState, nsStylePosition *& aPosition) const
{
  nsresult rv;
  PRBool result = PR_FALSE;
#ifdef NS_DEBUG
  PRInt32 counter=0;
#endif
  // Walk up the reflow state chain until we find a cell or the root
  const nsReflowState* rs = aReflowState.parentReflowState;
  while (nsnull != rs) 
  {
#ifdef NS_DEBUG
    counter++;
    NS_ASSERTION(counter<100000, "infinite loop in IsNested");
#endif
    nsIFrame* parentTable = nsnull;
    rv = rs->frame->QueryInterface(kTableFrameCID, (void**) &parentTable);
    if (NS_OK==rv)
    {
      result = PR_TRUE;
      parentTable->GetStyleData(eStyleStruct_Position, ((nsStyleStruct *&)aPosition));
      break;
    }
    rs = rs->parentReflowState;
  }
  return result;
}

/* helper method for getting the width of the table's containing block */
nscoord nsTableFrame::GetTableContainerWidth(const nsReflowState& aReflowState)
{
  nsresult rv;
  nscoord parentWidth = aReflowState.maxSize.width;

  // Walk up the reflow state chain until we find a block
  // frame. Our width is computed relative to there.
  const nsReflowState* rs = &aReflowState;
  nsIFrame *childFrame=nsnull;
  nsIFrame *grandchildFrame=nsnull;
  nsIFrame *greatgrandchildFrame=nsnull;
  while (nsnull != rs) 
  {
    // if it's a block, use its max width
    nsIFrame* block = nsnull;
    rv = rs->frame->QueryInterface(kBlockFrameCID, (void**) &block);
    if (NS_OK==rv) 
    { // we found a block, see if it's really a table cell (which means we're a nested table)
      PRBool skipThisBlock=PR_FALSE;
      if (PR_TRUE==((nsContainerFrame*)block)->IsPseudoFrame())
      {
        const nsReflowState* parentRS = rs->parentReflowState;
        if (nsnull!=parentRS)
        {
          parentRS = parentRS->parentReflowState;
          if (nsnull!=parentRS)
          {
            nsIFrame* cell = nsnull;
            rv = parentRS->frame->QueryInterface(kTableCellFrameCID, (void**) &cell);
            if (rv == NS_OK) {
              if (PR_TRUE==gsDebugNT)
                printf("%p: found a block pframe %p in a cell, skipping it.\n", aReflowState.frame, block);
              skipThisBlock = PR_TRUE;
            }
          }
        }
      }
      // at this point, we know we have a block.  If we're sure it's not a table cell pframe,
      // then we can use it
      if (PR_FALSE==skipThisBlock)
      {
        if (NS_UNCONSTRAINEDSIZE!=rs->maxSize.width)
        {
          parentWidth = rs->maxSize.width;
          if (PR_TRUE==gsDebugNT)
            printf("%p: found a block frame %p, returning width %d\n", 
                   aReflowState.frame, block, parentWidth);
          break;
        }
      }
    }
    // or if it's another table (we're nested) use its computed width
    if (rs->frame!=aReflowState.frame)
    {
      nsMargin borderPadding;
      const nsStylePosition* tablePosition;
      const nsStyleSpacing* spacing;
      nsIFrame* cell = nsnull;
      rv = rs->frame->QueryInterface(kTableCellFrameCID, (void**) &cell);
      // if the cell has a specified width, use it
      if (NS_OK==rv) 
      {
        // Compute and subtract out the insets (sum of border and padding) for the table
        cell->GetStyleData(eStyleStruct_Position, ((nsStyleStruct *&)tablePosition));
        if (eStyleUnit_Coord == tablePosition->mWidth.GetUnit())
        {
          // first, get pointers to the table frame parents of the cell
          nsIFrame *rowParent, *rowGroupParent;
          nsTableFrame *tableParent;
          cell->GetGeometricParent(rowParent);
          rowParent->GetGeometricParent(rowGroupParent);
          rowGroupParent->GetGeometricParent((nsIFrame *&)tableParent);
          if (nsnull != tableParent->mColumnWidths)
          {
            PRInt32 colIndex = ((nsTableCellFrame *)cell)->GetColIndex();
            parentWidth = tableParent->GetColumnWidth(colIndex);
            break;
          }
          // if the column width of this cell is already computed, it overrides the attribute
          // otherwise, use the attribute becauase the actual column width has not yet been computed
          else
          {
            parentWidth = tablePosition->mWidth.GetCoordValue();
            // subtract out cell border and padding
            cell->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
            spacing->CalcBorderPaddingFor(cell, borderPadding);
            parentWidth -= (borderPadding.right + borderPadding.left);
            if (PR_TRUE==gsDebugNT)
              printf("%p: found a cell frame %p with fixed coord width %d, returning parentWidth %d\n", 
                     aReflowState.frame, cell, tablePosition->mWidth.GetCoordValue(), parentWidth);
            break;
          }
        }
      }
      else
      {
        nsIFrame* table = nsnull;
        rv = rs->frame->QueryInterface(kTableFrameCID, (void**) &table);
        if (NS_OK==rv) {
          /* We found the nearest containing table (actually, the inner table).  
             This defines what our percentage size is relative to. Use its desired width 
             as the basis for computing our width.
             **********************************************************************************
             Nav4 compatibility code:  if the inner table has a percent width and the outer
             table has an auto width, the parentWidth is the width the containing cell would be 
             without the inner table.
             **********************************************************************************
           */
          // Compute and subtract out the insets (sum of border and padding) for the table
          table->GetStyleData(eStyleStruct_Position, (const nsStyleStruct *&)tablePosition);
          table->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
          if (eStyleUnit_Auto == tablePosition->mWidth.GetUnit())
          {
            parentWidth = NS_UNCONSTRAINEDSIZE;
            if (nsnull != ((nsTableFrame*)table)->mColumnWidths)
            {
              parentWidth=0;
              PRInt32 colIndex = ((nsTableCellFrame *)greatgrandchildFrame)->GetColIndex();
              PRInt32 colSpan = ((nsTableFrame*)table)->GetEffectiveColSpan(colIndex, ((nsTableCellFrame *)greatgrandchildFrame));
              for (PRInt32 i = 0; i<colSpan; i++)
                parentWidth += ((nsTableFrame*)table)->GetColumnWidth(i+colIndex);
              // subtract out cell border and padding
              greatgrandchildFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
              spacing->CalcBorderPaddingFor(greatgrandchildFrame, borderPadding);
              parentWidth -= (borderPadding.right + borderPadding.left);
              if (PR_TRUE==gsDebugNT)
                printf("%p: found a table frame %p with auto width, returning parentWidth %d from cell in col %d with span %d\n", 
                       aReflowState.frame, table, parentWidth, colIndex, colSpan);
            }
            else
            {
              if (PR_TRUE==gsDebugNT)
                printf("%p: found a table frame %p with auto width, returning parentWidth %d because parent has no info yet.\n", 
                       aReflowState.frame, table, parentWidth);
            }
          }
          else
          {
            if (nsnull!=((nsTableFrame*)table)->mColumnWidths)
            {
              parentWidth=0;
              PRInt32 colIndex = ((nsTableCellFrame*)greatgrandchildFrame)->GetColIndex();
              PRInt32 colSpan = ((nsTableFrame*)table)->GetEffectiveColSpan(colIndex, ((nsTableCellFrame *)greatgrandchildFrame));
              for (PRInt32 i = 0; i<colSpan; i++)
                parentWidth += ((nsTableFrame*)table)->GetColumnWidth(i+colIndex);
              if (eStyleUnit_Percent == tablePosition->mWidth.GetUnit())
              {
                float percent = tablePosition->mWidth.GetPercentValue();
                parentWidth = (nscoord)(percent*((float)parentWidth));
              }
            }
            else
            {
              nsSize tableSize;
              table->GetSize(tableSize);
              parentWidth = tableSize.width;
              if (0!=tableSize.width)
              { // the table has been sized, so we can compute the available space for the child
                spacing->CalcBorderPaddingFor(rs->frame, borderPadding);
                parentWidth -= (borderPadding.right + borderPadding.left);
                // same for the row group
                childFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
                spacing->CalcBorderPaddingFor(childFrame, borderPadding);
                parentWidth -= (borderPadding.right + borderPadding.left);
                // same for the row
                grandchildFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
                spacing->CalcBorderPaddingFor(grandchildFrame, borderPadding);
                parentWidth -= (borderPadding.right + borderPadding.left);
                // same for the cell
                greatgrandchildFrame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
                spacing->CalcBorderPaddingFor(greatgrandchildFrame, borderPadding);
                parentWidth -= (borderPadding.right + borderPadding.left);
              }
              else
              {
                // the table has not yet been sized, so we need to infer the available space
                parentWidth = rs->maxSize.width;
                if (eStyleUnit_Percent == tablePosition->mWidth.GetUnit())
                {
                  float percent = tablePosition->mWidth.GetPercentValue();
                  parentWidth = (nscoord)(percent*((float)parentWidth));
                }
              }
            }
            if (PR_TRUE==gsDebugNT)
              printf("%p: found a table frame %p, returning parentWidth %d \n", 
                     aReflowState.frame, table, parentWidth);
          }
          break;
        }
      }
    }

    if (nsnull==childFrame)
      childFrame = rs->frame;
    else if (nsnull==grandchildFrame)
    {
      grandchildFrame = childFrame;
      childFrame = rs->frame;
    }
    else
    {
      greatgrandchildFrame = grandchildFrame;
      grandchildFrame = childFrame;
      childFrame = rs->frame;
    }
    rs = rs->parentReflowState;
  }

  return parentWidth;
}

// aSpecifiedTableWidth is filled if the table witdth is not auto
PRBool nsTableFrame::TableIsAutoWidth(nsTableFrame *aTableFrame,
                                      nsIStyleContext *aTableStyle, 
                                      const nsReflowState& aReflowState,
                                      nscoord& aSpecifiedTableWidth)
{
  NS_ASSERTION(nsnull!=aTableStyle, "bad arg - aTableStyle");
  PRBool result = PR_TRUE;  // the default
  if (nsnull!=aTableStyle)
  {
    nsStylePosition* tablePosition = (nsStylePosition*)aTableStyle->GetStyleData(eStyleStruct_Position);
    nsMargin borderPadding;
    const nsStyleSpacing* spacing;
    switch (tablePosition->mWidth.GetUnit()) {
    case eStyleUnit_Auto:         // specified auto width
    case eStyleUnit_Proportional: // illegal for table, so ignored
      break;

    case eStyleUnit_Inherit:
      // get width of parent and see if it is a specified value or not
      // XXX for now, just return true
      break;

    case eStyleUnit_Coord:
      // XXX: subtract out this table frame's borderpadding?
      aSpecifiedTableWidth = tablePosition->mWidth.GetCoordValue();
      aReflowState.frame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
      spacing->CalcBorderPaddingFor(aReflowState.frame, borderPadding);
      aSpecifiedTableWidth -= (borderPadding.right + borderPadding.left);
      result = PR_FALSE;
      break;

    case eStyleUnit_Percent:
      // set aSpecifiedTableWidth to be the given percent of the parent.
      // first, get the effective parent width (parent width - insets)
      nscoord parentWidth = nsTableFrame::GetTableContainerWidth(aReflowState);
      if (NS_UNCONSTRAINEDSIZE!=parentWidth)
      {
        aReflowState.frame->GetStyleData(eStyleStruct_Spacing, (const nsStyleStruct *&)spacing);
        spacing->CalcBorderPaddingFor(aReflowState.frame, borderPadding);
        parentWidth -= (borderPadding.right + borderPadding.left);

        // then set aSpecifiedTableWidth to the given percent of the computed parent width
        float percent = tablePosition->mWidth.GetPercentValue();
        aSpecifiedTableWidth = (PRInt32)(parentWidth*percent);
        if (PR_TRUE==gsDebug || PR_TRUE==gsDebugNT) 
          printf("%p: TableIsAutoWidth setting aSpecifiedTableWidth = %d with parentWidth = %d and percent = %f\n", 
                 aTableFrame, aSpecifiedTableWidth, parentWidth, percent);
      }
      else
      {
        aSpecifiedTableWidth=parentWidth;
        if (PR_TRUE==gsDebug || PR_TRUE==gsDebugNT) 
          printf("%p: TableIsAutoWidth setting aSpecifiedTableWidth = %d with parentWidth = %d\n", 
                 aTableFrame, aSpecifiedTableWidth, parentWidth);
      }
      result = PR_FALSE;
      break;
    }
  }

  return result; 
}

/* ----- debugging methods ----- */
NS_METHOD nsTableFrame::List(FILE* out, PRInt32 aIndent, nsIListFilter *aFilter) const
{
  // if a filter is present, only output this frame if the filter says we should
  // since this could be any "tag" with the right display type, we'll
  // just pretend it's a table
  if (nsnull==aFilter)
    return nsContainerFrame::List(out, aIndent, aFilter);

  nsAutoString tagString("table");
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
    PRInt32 contentIndex;

    GetContentIndex(contentIndex);
    fprintf(out, "(%d)", contentIndex);
    out << mRect;
    if (0 != mState) {
      fprintf(out, " [state=%08x]", mState);
    }
    fputs("\n", out);
    if (nsnull!=mTableLayoutStrategy)
    {
      for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
      fprintf(out, "min=%d, max=%d, fixed=%d, cols=%d, numCols=%d\n",
              mTableLayoutStrategy->GetTableMinWidth(),
              mTableLayoutStrategy->GetTableMaxWidth(),
              mTableLayoutStrategy->GetTableFixedWidth(),
              mTableLayoutStrategy->GetCOLSAttribute(),
              mTableLayoutStrategy->GetNumCols()
             );
    }
  }
  // Output the children
  if (mChildCount > 0) {
    if (PR_TRUE==outputMe)
    {
      if (0 != mState) {
        fprintf(out, " [state=%08x]\n", mState);
      }
    }
    for (nsIFrame* child = mFirstChild; child; NextChild(child, child)) {
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
