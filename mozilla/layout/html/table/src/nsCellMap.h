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
#ifndef nsCellMap_h__
#define nsCellMap_h__

#include "nscore.h"
#include "celldata.h"

class nsVoidArray;
class nsTableColFrame;
class nsTableCellFrame;

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
protected:
  /** storage for rows */
  nsVoidArray *mRows;       

  /** storage for CellData pointers */
  PRInt32 *mMinColSpans;

  /** a cache of the column frames, by col index */
  nsVoidArray * mColFrames;

  /** the number of rows.  mRows[0] - mRows[mRowCount-1] are non-null. */
  PRInt32 mRowCount;

  /** the number of rows allocated (due to cells having rowspans extending beyond the end of the table */
  PRInt32 mTotalRowCount;

  /** the number of columns (the max of all row lengths) */
  PRInt32 mColCount;

public:
  /** constructor 
    * @param aRows - initial number of rows
	* @param aColumns - initial number of columns
	*/
  nsCellMap(PRInt32 aRows, PRInt32 aColumns);

  /** destructor
    * NOT VIRTUAL BECAUSE THIS CLASS SHOULD **NEVER** BE SUBCLASSED  
	*/
  ~nsCellMap();

  /** set the CellMap to (aRows x aColumns) */
  void Reset(PRInt32 aRows, PRInt32 aColumns);

  /** return the CellData for the cell at (aRowIndex,aColIndex) */
  CellData * GetCellAt(PRInt32 aRowIndex, PRInt32 aColIndex) const;

  /** return the nsTableCellFrame for the cell at (aRowIndex, aColIndex) */
  nsTableCellFrame * GetCellFrameAt(PRInt32 aRowIndex, PRInt32 aColIndex) const;

  /** assign aCellData to the cell at (aRow,aColumn) */
  void SetCellAt(CellData *aCellData, PRInt32 aRow, PRInt32 aColumn);

  /** expand the CellMap to have aRowCount rows.  The number of columns remains the same */
  void GrowToRow(PRInt32 aRowCount);

  /** expand the CellMap to have aColCount columns.  The number of rows remains the same */
  void GrowToCol(PRInt32 aColCount);

  /** return the total number of columns in the table represented by this CellMap */
  PRInt32 GetColCount() const;

  /** return the actual number of rows in the table represented by this CellMap */
  PRInt32 GetRowCount() const;

  /** return the column frame associated with aColIndex */
  nsTableColFrame * GetColumnFrame(PRInt32 aColIndex) const;

  /** return the index of the next column in aRowIndex after aColIndex 
    * that does not have a cell assigned to it.
    * If aColIndex is past the end of the row, it is returned.
    * If the row is not initialized in the cell map, 0 is returned.
    */
  PRInt32 GetNextAvailColIndex(PRInt32 aRowIndex, PRInt32 aColIndex) const;

  /** cache the min col span for all cells in aColIndex */ 
  void    SetMinColSpan(PRInt32 aColIndex, PRBool  aColSpan);

  /** get the cached min col span for aColIndex */
  PRInt32 GetMinColSpan(PRInt32 aColIndex) const;

  /** add a column frame to the list of column frames
    * column frames must be added in order
	*/
  void AppendColumnFrame(nsTableColFrame *aColFrame);

  /** return PR_TRUE if aRowIndex has any cells with rowspan>1 contained
    * within it (not just cells that are in the row, but cells that span
	* into the row as well.
	*/
  PRBool RowImpactedBySpanningCell(PRInt32 aRowIndex);

  /** return PR_TRUE if aColIndex has any cells with colspan>1 contained
    * within it (not just cells that are in the col, but cells that span
	* into the col as well.
	*/
  PRBool ColumnImpactedBySpanningCell(PRInt32 aColIndex);

  /** for debugging */
  void DumpCellMap() const;

};

/* ----- inline methods ----- */

inline CellData * nsCellMap::GetCellAt(PRInt32 aRowIndex, PRInt32 aColIndex) const
{
  NS_PRECONDITION(0<=aRowIndex, "bad aRowIndex arg");
  NS_PRECONDITION(0<=aColIndex, "bad aColIndex arg");
  // don't check index vs. count for row or col, because it's ok to ask for a cell that doesn't yet exist
  NS_PRECONDITION(nsnull!=mRows, "bad mRows");

  CellData *result = nsnull;
  nsVoidArray *row = (nsVoidArray *)(mRows->ElementAt(aRowIndex));
  if (nsnull!=row)
    result = (CellData *)(row->ElementAt(aColIndex));
  return result;
}

inline nsTableCellFrame * nsCellMap::GetCellFrameAt(PRInt32 aRowIndex, PRInt32 aColIndex) const
{
  NS_PRECONDITION(0<=aRowIndex, "bad aRowIndex arg");
  // don't check aRowIndex vs. mRowCount, because it's ok to ask for a cell in a row that doesn't yet exist
  NS_PRECONDITION(0<=aColIndex && aColIndex < mColCount, "bad aColIndex arg");

  nsTableCellFrame *result = nsnull;
  CellData * cellData = GetCellAt(aRowIndex, aColIndex);
  if (nsnull!=cellData)
    result = cellData->mCell;
  return result;
}

inline PRInt32 nsCellMap::GetColCount() const
{ 
  return mColCount; 
}

inline PRInt32 nsCellMap::GetRowCount() const
{ 
  return mRowCount; 
}

inline void nsCellMap::AppendColumnFrame(nsTableColFrame *aColFrame)
{
  mColFrames->AppendElement(aColFrame);
}


#endif
