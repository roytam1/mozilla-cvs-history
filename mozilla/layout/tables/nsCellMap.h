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
#ifndef nsCellMap_h__
#define nsCellMap_h__

#include "nscore.h"
#include "celldata.h"
#include "nsVoidArray.h"
class nsTableColFrame;
class nsTableCellFrame;
class nsIPresContext;
class nsTableRowGroupFrame;
class nsTableFrame;
class nsCellMap;

struct nsColInfo
{
  PRInt32 mNumCellsOrig; // number of cells originating in the col
  PRInt32 mNumCellsSpan; // number of cells spanning into the col via colspans (not rowspans)

  nsColInfo(); 
  nsColInfo(PRInt32 aNumCellsOrig,
            PRInt32 aNumCellsSpan);
};

class nsTableCellMap
{
public:
  nsTableCellMap(nsIPresContext* aPresContext, nsTableFrame& aTableFrame);

  /** destructor
    * NOT VIRTUAL BECAUSE THIS CLASS SHOULD **NEVER** BE SUBCLASSED  
	  */
  ~nsTableCellMap();
  
  void RemoveGroupCellMap(nsTableRowGroupFrame* aRowGroup);
  void InsertGroupCellMap(PRInt32               aRowIndex, 
                          nsTableRowGroupFrame& aNewRowGroup);
  void InsertGroupCellMap(nsTableRowGroupFrame* aPrevRowGroup, 
                          nsTableRowGroupFrame& aNewRowGroup);

  nsTableCellFrame* GetCellFrame(PRInt32   aRowIndex,
                                 PRInt32   aColIndex,
                                 CellData& aData,
                                 PRBool    aUseRowIfOverlap) const;

  /** return the CellData for the cell at (aTableRowIndex, aTableColIndex) */
  CellData* GetCellAt(PRInt32 aRowIndex, 
                      PRInt32 aColIndex);

  nsColInfo* GetColInfoAt(PRInt32 aColIndex);

  /** append the cellFrame at the end of the row at aRowIndex and return the col index
    */
  PRInt32 AppendCell(nsTableCellFrame&     aCellFrame,
                     PRInt32               aRowIndex,
                     PRBool                aRebuildIfNecessary);

  void InsertCells(nsVoidArray&          aCellFrames,
                   PRInt32               aRowIndex,
                   PRInt32               aColIndexBefore);

  void RemoveCell(nsTableCellFrame* aCellFrame,
                  PRInt32           aRowIndex);

  void InsertRows(nsIPresContext*       aPresContext,
                  nsTableRowGroupFrame& aRowGroup,
                  nsVoidArray&          aRows,
                  PRInt32               aFirstRowIndex,
                  PRBool                aConsiderSpans);

  void RemoveRows(nsIPresContext* aPresContext,
                  PRInt32         aFirstRowIndex,
                  PRInt32         aNumRowsToRemove,
                  PRBool          aConsiderSpans);

  PRInt32 GetEffectiveColSpan(PRInt32                 aColIndex, 
                              const nsTableCellFrame& aCell);

  PRInt32 GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const;
  PRInt32 GetNumCellsOriginatingInCol(PRInt32 aColIndex) const;

  PRInt32 GetRowSpan(PRInt32 aRowIndex,
                     PRInt32 aColIndex);
  PRInt32 GetColSpan(PRInt32 aRowIndex,
                     PRInt32 aColIndex);

  /** return the total number of columns in the table represented by this CellMap */
  PRInt32 GetColCount() const;

  /** return the actual number of rows in the table represented by this CellMap */
  PRInt32 GetRowCount() const;

  // temporary until nsTableFrame::GetCellData uses GetCellFrameAt
  nsTableCellFrame* GetCellFrameOriginatingAt(PRInt32 aRowX, 
                                              PRInt32 aColX);

  nsTableCellFrame* GetCellInfoAt(PRInt32  aRowX, 
                                  PRInt32  aColX, 
                                  PRBool*  aOriginates = nsnull, 
                                  PRInt32* aColSpan = nsnull);

  void AddColsAtEnd(PRUint32 aNumCols);
  PRInt32 RemoveUnusedCols(PRInt32 aMaxNumToRemove);

  PRBool RowIsSpannedInto(PRInt32 aRowIndex);
  PRBool RowHasSpanningCells(PRInt32 aRowIndex);
  PRBool ColIsSpannedInto(PRInt32 aColIndex);
  PRBool ColHasSpanningCells(PRInt32 aColIndex);

  /** dump a representation of the cell map to stdout for debugging */
#ifdef NS_DEBUG
  void Dump() const;
#endif

#ifdef DEBUG
  void SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

protected:
  friend class nsCellMap;
  void InsertGroupCellMap(nsCellMap* aPrevMap,
                          nsCellMap& aNewMap);

  nsVoidArray mCols;
  nsCellMap*  mFirstMap;
};

/** nsCellMap is a support class for nsTablePart.  
  * It maintains an Rows x Columns grid onto which the cells of the table are mapped.
  * This makes processing of rowspan and colspan attributes much easier.
  * Each cell is represented by a CellData object.
  *
  * @see CellData
  * @see nsTableFrame::AddCellToMap
  * @see nsTableFrame::GrowCellMap
  * @see nsTableFrame::BuildCellIntoMap
  *
  * mRows is an array of rows.  a row cannot be null.
  * each row is an array of cells.  a cell can be null.
  */
class nsCellMap
{
public:
  /** constructor 
    * @param aNumRows - initial number of rows
	  * @param aNumColumns - initial number of columns
	  */
  nsCellMap(nsTableRowGroupFrame& aRowGroupFrame);

  /** destructor
    * NOT VIRTUAL BECAUSE THIS CLASS SHOULD **NEVER** BE SUBCLASSED  
	  */
  ~nsCellMap();

  nsCellMap* GetNextSibling() const;
  void SetNextSibling(nsCellMap* aSibling);

  nsTableRowGroupFrame* GetRowGroup() const;

  nsTableCellFrame* GetCellFrame(PRInt32   aRowIndex,
                                 PRInt32   aColIndex,
                                 CellData& aData,
                                 PRBool    aUseRowSpanIfOverlap) const;

  /** return the CellData for the cell at (aTableRowIndex, aTableColIndex) */
  CellData* GetCellAt(PRInt32 aRowIndex, 
                      PRInt32 aColIndex,
                      PRInt32 aNumColsInTable);

  /** append the cellFrame at the end of the row at aRowIndex and return the col index
    */
  PRInt32 AppendCell(nsTableCellMap&   aMap,
                     nsTableCellFrame& aCellFrame, 
                     PRInt32           aRowIndex,
                     PRBool            aRebuildIfNecessary);

  void InsertCells(nsTableCellMap& aMap,
                   nsVoidArray&    aCellFrames,
                   PRInt32         aRowIndex,
                   PRInt32         aColIndexBefore);

  void RemoveCell(nsTableCellMap&   aMap,
                  nsTableCellFrame* aCellFrame,
                  PRInt32           aRowIndex);

  void RemoveCol(PRInt32 aColIndex);

  void InsertRows(nsIPresContext* aPresContext,
                  nsTableCellMap& aMap,
                  nsVoidArray&    aRows,
                  PRInt32         aFirstRowIndex,
                  PRBool          aConsiderSpans);

  void RemoveRows(nsIPresContext* aPresContext,
                  nsTableCellMap& aMap,
                  PRInt32         aFirstRowIndex,
                  PRInt32         aNumRowsToRemove,
                  PRBool          aConsiderSpans);

  PRInt32 GetEffectiveColSpan(PRInt32                 aRowIndex,
                              PRInt32                 aColIndex,
                              PRInt32                 aNumColsInTable,
                              const nsTableCellFrame& aCell);

  PRInt32 GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const;
  PRInt32 GetNumCellsOriginatingInCol(PRInt32 aColIndex) const;

  /** return the actual number of rows in the table represented by this CellMap */
  PRInt32 GetRowCount() const;

  // temporary until nsTableFrame::GetCellData uses GetCellFrameAt
  nsTableCellFrame* GetCellFrameOriginatingAt(PRInt32 aRowX, 
                                              PRInt32 aColX,
                                              PRInt32 aNumColsInTable);

  nsTableCellFrame* GetCellInfoAt(PRInt32  aRowX, 
                                  PRInt32  aColX, 
                                  PRInt32  aNumColsInTable,
                                  PRBool*  aOriginates = nsnull, 
                                  PRInt32* aColSpan = nsnull);

  PRBool RowIsSpannedInto(PRInt32 aRowIndex,
                          PRInt32 aNumColsInTable);

  PRBool RowHasSpanningCells(PRInt32 aRowIndex,
                             PRInt32 aNumCols);

  PRBool ColIsSpannedInto(PRInt32 aColIndex,
                          PRInt32 aNumCols);

  PRBool ColHasSpanningCells(PRInt32 aColIndex,
                             PRInt32 aNumCols);

  /** dump a representation of the cell map to stdout for debugging */
#ifdef NS_DEBUG
  void Dump() const;
#endif

#ifdef DEBUG
  void SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

protected:
  friend class nsTableCellMap;

  PRBool Grow(nsTableCellMap& aMap,
              PRInt32         aNumRows,
              PRInt32         aRowIndex = -1); 

  void GrowRow(nsVoidArray& aRow,
               PRInt32      aNumCols);

  /** assign aCellData to the cell at (aRow,aColumn) */
  void SetMapCellAt(nsTableCellMap& aMap,
                    CellData&       aCellData, 
                    PRInt32         aMapRowIndex, 
                    PRInt32         aColIndex,
                    PRBool          aCountZeroSpanAsSpan);

  CellData* GetMapCellAt(PRInt32 aMapRowIndex, 
                         PRInt32 aColIndex,
                         PRInt32 aNumColsInTable);

  PRInt32 GetNumCellsIn(PRInt32 aColIndex) const;

  void ExpandWithRows(nsIPresContext* aPresContext,
                      nsTableCellMap& aMap,
                      nsVoidArray&    aRowFrames,
                      PRInt32         aStartRowIndex);

  void ExpandWithCells(nsTableCellMap& aMap,
                       nsVoidArray&    aCellFrames,
                       PRInt32         aRowIndex,
                       PRInt32         aColIndex,
                       PRInt32         aRowSpan,
                       PRBool          aRowSpanIsZero);

  void ShrinkWithoutRows(nsTableCellMap& aMap,
                         PRInt32         aFirstRowIndex,
                         PRInt32         aNumRowsToRemove);

  void ShrinkWithoutCell(nsTableCellMap&   aMap,
                         nsTableCellFrame& aCellFrame,
                         PRInt32           aRowIndex,
                         PRInt32           aColIndex);

  void RebuildConsideringRows(nsIPresContext* aPresContext,
                              nsTableCellMap& aMap,
                              PRInt32         aStartRowIndex,
                              nsVoidArray*    aRowsToInsert,
                              PRInt32         aNumRowsToRemove = 0);

  void RebuildConsideringCells(nsTableCellMap& aMap,
                               nsVoidArray*    aCellFrames,
                               PRInt32         aRowIndex,
                               PRInt32         aColIndex,
                               PRBool          aInsert);

  PRBool CellsSpanOut(nsIPresContext* aPresContext, nsVoidArray& aNewRows);

  PRBool CellsSpanInOrOut(PRInt32 aStartRowIndex, 
                          PRInt32 aEndRowIndex,
                          PRInt32 aStartColIndex, 
                          PRInt32 aEndColIndex,
                          PRInt32 aNumColsInTable);

  void ExpandForZeroSpan(nsTableCellFrame* aCellFrame,
                         PRInt32           aNumColsInTable);

  PRBool CreateEmptyRow(PRInt32 aRowIndex,
                        PRInt32 aNumCols);

  PRInt32 GetColSpan(nsTableCellFrame& aCellFrameToAdd, 
                     PRInt32           aColIndex,
                     PRInt32           aNumColsInTable,
                     PRBool&           aIsZeroColSpan);
  
  PRInt32 GetColSpan(PRInt32 aRowIndex,
                     PRInt32 aColIndex,
                     PRInt32 aNumColsInTable,
                     PRBool& aIsZeroColSpan);

  PRInt32 GetRowSpan(PRInt32 aRowIndex,
                     PRInt32 aColIndex,
                     PRBool& aIsZeroRowSpan);

  PRInt32 GetRowSpan(nsTableCellFrame& aCellFrameToAdd, 
                     PRInt32           aRowIndex,
                     PRBool&           aIsZeroRowSpan);
  
  void AdjustForZeroSpan(PRInt32 aRowIndex,
                         PRInt32 aColIndex,
                         PRInt32 aNumColsInTable);

  PRBool IsZeroColSpan(PRInt32 aRowIndex,
                       PRInt32 aColIndex) const;

  /** an array containing col array. It can be larger than mRowCount due to
    * row spans extending beyond the table */
  nsVoidArray mRows; 

  /** the number of rows in the table which is <= the number of rows in the cell map
    * due to row spans extending beyond the end of the table (dead rows) */
  PRInt32 mRowCount;

  // the row group that corresponds to this map
  nsTableRowGroupFrame* mRowGroupFrame;

  // the next row group cell map
  nsCellMap* mNextSibling;

};

/* ----- inline methods ----- */
inline PRInt32 nsTableCellMap::GetColCount() const
{
  return mCols.Count();
}

inline nsCellMap* nsCellMap::GetNextSibling() const
{
  return mNextSibling;
}

inline void nsCellMap::SetNextSibling(nsCellMap* aSibling)
{
  mNextSibling = aSibling;
}

inline nsTableRowGroupFrame* nsCellMap::GetRowGroup() const
{
  return mRowGroupFrame;
}

inline PRInt32 nsCellMap::GetRowCount() const
{ 
  return mRowCount; 
}

// nsColInfo

inline nsColInfo::nsColInfo()
 :mNumCellsOrig(0), mNumCellsSpan(0) 
{}

inline nsColInfo::nsColInfo(PRInt32 aNumCellsOrig,
                            PRInt32 aNumCellsSpan)
 :mNumCellsOrig(aNumCellsOrig), mNumCellsSpan(aNumCellsSpan) 
{}


#endif
