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

#include "nsLayoutAtoms.h"
#include "nsVoidArray.h"
#include "nsCellMap.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsTableRowGroupFrame.h"

// CellData 

MOZ_DECL_CTOR_COUNTER(CellData);

CellData::CellData(nsTableCellFrame* aOrigCell)
{
  MOZ_COUNT_CTOR(CellData);
  mOrigCell = aOrigCell;
}

CellData::~CellData()
{
  MOZ_COUNT_DTOR(CellData);
}

MOZ_DECL_CTOR_COUNTER(nsCellMap);

// nsTableCellMap

nsTableCellMap::nsTableCellMap(nsTableFrame& aTableFrame)
{
  MOZ_COUNT_CTOR(nsTableCellMap);
  mFirstMap = nsnull;
  nsTableRowGroupFrame* prior = nsnull;
  nsIFrame* child;
  aTableFrame.FirstChild(nsnull, &child);
  while(child) {
    nsTableRowGroupFrame* groupFrame = aTableFrame.GetRowGroupFrame(child);
    if (groupFrame) {
      InsertGroupCellMap(prior, *groupFrame);
      prior = groupFrame;
    }
    child->GetNextSibling(&child);
  }
}

nsTableCellMap::~nsTableCellMap()
{
  MOZ_COUNT_DTOR(nsTableCellMap);
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    nsCellMap* next = cellMap->GetNextSibling();
    delete cellMap;
    cellMap = next;
  }

  PRInt32 colCount    = mCols.Count();
  for (PRInt32 colX = 0; colX < colCount; colX++) {
    nsColInfo* colInfo = (nsColInfo *)mCols.ElementAt(colX);
    if (colInfo) {
      delete colInfo;
    }
  }
}

void 
nsTableCellMap::InsertGroupCellMap(nsCellMap* aPrevMap,
                                   nsCellMap& aNewMap)
{
  nsCellMap* next;
  if (aPrevMap) {
    next = aPrevMap->GetNextSibling();
    aPrevMap->SetNextSibling(&aNewMap);
  }
  else {
    next = mFirstMap;
    mFirstMap = &aNewMap;
  }
  aNewMap.SetNextSibling(next);
}

void nsTableCellMap::InsertGroupCellMap(nsTableRowGroupFrame* aPrevGroup,
                                        nsTableRowGroupFrame& aNewGroup)
{
  nsCellMap* newMap = new nsCellMap(aNewGroup);
  if (newMap) {
    nsCellMap* prevMap = nsnull;
    if (aPrevGroup) {
      nsCellMap* map = mFirstMap;
      while (map) {
        if (map->GetRowGroup() == aPrevGroup) {
          prevMap = map;
          break;
        }
        map = map->GetNextSibling();
      }
    }
    InsertGroupCellMap(prevMap, *newMap);
  }
}

void nsTableCellMap::InsertGroupCellMap(PRInt32               aRowIndex,
                                        nsTableRowGroupFrame& aNewGroup)
{
  nsCellMap* newMap = new nsCellMap(aNewGroup);
  if (newMap) {
    PRInt32 rowIndex = aRowIndex;
    nsCellMap* prevMap = nsnull;
    nsCellMap* map = mFirstMap;
    while (map) {
      if (map->GetRowCount() > rowIndex) {
        break;
      }
      prevMap = map;
      map = map->GetNextSibling();
    }
    InsertGroupCellMap(prevMap, *newMap);
  }
}

void nsTableCellMap::RemoveGroupCellMap(nsTableRowGroupFrame* aGroup)
{
  nsCellMap* map = mFirstMap;
  nsCellMap* prior = nsnull;
  while (map) {
    if (map->GetRowGroup() == aGroup) {
      nsCellMap* next = map->GetNextSibling();
      if (mFirstMap == map) {
        mFirstMap = next;
      }
      else {
        prior->SetNextSibling(next);
      }
      delete map;
      break;
    }
    prior = map;
    map = map->GetNextSibling();
  }
}

PRInt32 
nsTableCellMap::GetRowSpan(PRInt32 aRowIndex,
                           PRInt32 aColIndex)
{
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > aRowIndex) {
      PRBool zeroRowSpan;
      return map->GetRowSpan(aRowIndex, aColIndex, zeroRowSpan);
    }
  }
  return nsnull;
}

PRInt32 
nsTableCellMap::GetColSpan(PRInt32 aRowIndex,
                           PRInt32 aColIndex)
{
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > aRowIndex) {
      PRBool zeroColSpan;
      return map->GetColSpan(aRowIndex, aColIndex, GetColCount(), zeroColSpan);
    }
  }
  return nsnull;
}

nsTableCellFrame* nsTableCellMap::GetCellFrame(PRInt32   aRowIndex,
                                               PRInt32   aColIndex,
                                               CellData& aData,
                                               PRBool    aUseRowIfOverlap) const
{
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > aRowIndex) {
      return map->GetCellFrame(aRowIndex, aColIndex, aData, aUseRowIfOverlap);
    }
  }
  return nsnull;
}

nsColInfo* nsTableCellMap::GetColInfoAt(PRInt32 aColIndex)
{
  PRInt32 numColsToAdd = aColIndex + 1 - mCols.Count();
  if (numColsToAdd > 0) {
    AddColsAtEnd(numColsToAdd);
  }
  return (nsColInfo*)mCols.ElementAt(aColIndex);
}

PRInt32 
nsTableCellMap::GetRowCount() const
{
  PRInt32 numRows = 0;
  nsCellMap* map = mFirstMap;
  while (map) {
    numRows += map->GetRowCount();
    map = map->GetNextSibling();
  }
  return numRows;
}

CellData* 
nsTableCellMap::GetCellAt(PRInt32 aRowIndex, 
                          PRInt32 aColIndex)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->GetCellAt(rowIndex, aColIndex, GetColCount());
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  return nsnull;
}

void 
nsTableCellMap::AddColsAtEnd(PRUint32 aNumCols)
{
  for (PRUint32 colX = 1; colX <= aNumCols; colX++) {
    nsColInfo* colInfo = new nsColInfo();
    if (colInfo) {
      mCols.AppendElement(colInfo);
    }
  }
}

void
nsTableCellMap::InsertRows(nsTableRowGroupFrame& aParent,
                           nsVoidArray&          aRows,
                           PRInt32               aFirstRowIndex,
                           PRBool                aConsiderSpans)
{
  NS_ASSERTION((aRows.Count() > 0) && (aFirstRowIndex >= 0), "nsTableCellMap::InsertRows called incorrectly");
  PRInt32 rowIndex = aFirstRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowGroup() == &aParent) {
      cellMap->InsertRows(*this, aRows, rowIndex, aConsiderSpans);
      return;
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  // if we get here it is because of a nested tree row group
  if (mFirstMap) {
    mFirstMap->InsertRows(*this, aRows, aFirstRowIndex, aConsiderSpans);
  }
}

void
nsTableCellMap::RemoveRows(PRInt32 aFirstRowIndex,
                           PRInt32 aNumRowsToRemove,
                           PRBool  aConsiderSpans)
{
  PRInt32 rowIndex = aFirstRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      cellMap->RemoveRows(*this, rowIndex, aNumRowsToRemove, aConsiderSpans);
      break;
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
}

PRInt32 
nsTableCellMap::AppendCell(nsTableCellFrame&     aCellFrame,
                           PRInt32               aRowIndex,
                           PRBool                aRebuildIfNecessary)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->AppendCell(*this, aCellFrame, rowIndex, aRebuildIfNecessary);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return 0;
}


void 
nsTableCellMap::InsertCells(nsVoidArray&          aCellFrames,
                            PRInt32               aRowIndex,
                            PRInt32               aColIndexBefore)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      cellMap->InsertCells(*this, aCellFrames, rowIndex, aColIndexBefore);
      break;
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
}

PRInt32
nsTableCellMap::RemoveUnusedCols(PRInt32 aMaxToRemove)
{
  PRInt32 numColsRemoved = 0;
  for (PRInt32 colX = mCols.Count() - 1; colX >= 0; colX--) {
    nsColInfo* colInfo = (nsColInfo *) mCols.ElementAt(colX);
    if (!colInfo || (colInfo->mNumCellsOrig > 0) || (colInfo->mNumCellsSpan > 0)) {
      return numColsRemoved;
    }
    else { 
      // remove the col from the cols array
      colInfo = (nsColInfo *) mCols.ElementAt(colX);
      mCols.RemoveElementAt(colX);
      delete colInfo;

      // remove the col entry from each of the maps
      nsCellMap* cellMap = mFirstMap;
      while (cellMap) {
        cellMap->RemoveCol(colX);
        cellMap = cellMap->GetNextSibling();
      }

      numColsRemoved++;
      if (numColsRemoved >= aMaxToRemove) {
        return numColsRemoved;
      }
    }
  }
  return numColsRemoved;
}

void 
nsTableCellMap::RemoveCell(nsTableCellFrame* aCellFrame,
                           PRInt32           aRowIndex)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      cellMap->RemoveCell(*this, aCellFrame, rowIndex);
      break;
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
}

PRInt32 
nsTableCellMap::GetNumCellsOriginatingInCol(PRInt32 aColIndex) const
{
  PRInt32 colCount = mCols.Count();
  if ((aColIndex >= 0) && (aColIndex < colCount)) {
    return ((nsColInfo *)mCols.ElementAt(aColIndex))->mNumCellsOrig;
  }
  else {
    NS_ASSERTION(PR_FALSE, "nsCellMap::GetNumCellsOriginatingInCol - bad col index");
    return 0;
  }
}

#ifdef NS_DEBUG
void 
nsTableCellMap::Dump() const
{
  printf("***** START TABLE CELL MAP DUMP *****\n");
  // output col info
  PRInt32 colCount = mCols.Count();
	printf ("\ncols array orig/span-> ");
	for (PRInt32 colX = 0; colX < colCount; colX++) {
    nsColInfo* colInfo = (nsColInfo *)mCols.ElementAt(colX);
		printf ("%d=%d/%d ", colX, colInfo->mNumCellsOrig, colInfo->mNumCellsSpan);
	}
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    cellMap->Dump();
    cellMap = cellMap->GetNextSibling();
  }
  printf("\n***** END TABLE CELL MAP DUMP *****\n");
}
#endif

PRInt32 
nsTableCellMap::GetEffectiveColSpan(PRInt32                 aColIndex, 
                                    const nsTableCellFrame* aCell)
{
  PRInt32 rowIndex;
  aCell->GetRowIndex(rowIndex);
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->GetEffectiveColSpan(rowIndex, aColIndex, mCols.Count(), aCell);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  NS_ASSERTION(PR_FALSE, "col span was 0");
  return 0;
}

nsTableCellFrame* 
nsTableCellMap::GetCellFrameOriginatingAt(PRInt32 aRowIndex, 
                                          PRInt32 aColIndex)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->GetCellFrameOriginatingAt(rowIndex, aColIndex, GetColCount());
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return nsnull;
}

nsTableCellFrame* 
nsTableCellMap::GetCellInfoAt(PRInt32  aRowIndex, 
                              PRInt32  aColIndex, 
                              PRBool*  aOriginates, 
                              PRInt32* aColSpan)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->GetCellInfoAt(rowIndex, aColIndex, mCols.Count(), aOriginates, aColSpan);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return nsnull;
}
  

PRBool nsTableCellMap::RowIsSpannedInto(PRInt32 aRowIndex)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->RowIsSpannedInto(rowIndex, mCols.Count());
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return PR_FALSE;
}

PRBool nsTableCellMap::RowHasSpanningCells(PRInt32 aRowIndex)
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->RowHasSpanningCells(rowIndex, mCols.Count());
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return PR_FALSE;
}

PRBool nsTableCellMap::ColIsSpannedInto(PRInt32 aColIndex)
{
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->ColIsSpannedInto(aColIndex, mCols.Count())) {
      return PR_TRUE;
    }
    cellMap = cellMap->GetNextSibling();
  }
  return PR_FALSE;
}

PRBool nsTableCellMap::ColHasSpanningCells(PRInt32 aColIndex)
{
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->ColHasSpanningCells(aColIndex, mCols.Count())) {
      return PR_TRUE;
    }
    cellMap = cellMap->GetNextSibling();
  }
  return PR_FALSE;
}

#ifdef DEBUG
void nsTableCellMap::SizeOf(nsISizeOfHandler* aHandler, 
                            PRUint32*         aResult) const
{
  NS_PRECONDITION(aResult, "null OUT parameter pointer");
  PRUint32 sum = sizeof(*this);

  // Add in the size of the void arrays. Because we have emnbedded objects
  // and not pointers to void arrays, we need to subtract out the size of the
  // embedded object so it isn't double counted
  PRUint32 voidArraySize;

  mCols.SizeOf(aHandler, &voidArraySize);
  sum += voidArraySize - sizeof(mCols);

  *aResult = sum;
}
#endif

// nsCellMap

nsCellMap::nsCellMap(nsTableRowGroupFrame& aRowGroup)
  : mRowCount(0), mNextSibling(nsnull), mRowGroupFrame(&aRowGroup)
{
}

nsCellMap::~nsCellMap()
{
  MOZ_COUNT_DTOR(nsCellMap);

  PRInt32 mapRowCount = mRows.Count();
  for (PRInt32 rowX = 0; rowX < mapRowCount; rowX++) {
    nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(rowX));
    PRInt32 colCount = row->Count();
    for (PRInt32 colX = 0; colX < colCount; colX++) {
      CellData* data = (CellData *)(row->ElementAt(colX));
      if (data) {
        delete data;
      } 
    }
    delete row;
  }
}

nsTableCellFrame* nsCellMap::GetCellFrame(PRInt32   aRowIndexIn,
                                          PRInt32   aColIndexIn,
                                          CellData& aData,
                                          PRBool    aUseRowIfOverlap) const
{
  PRInt32 rowIndex = aRowIndexIn - aData.GetRowSpanOffset();
  PRInt32 colIndex = aColIndexIn - aData.GetColSpanOffset();
  if (aData.IsOverlap()) {
    if (aUseRowIfOverlap) {
      colIndex = aColIndexIn;
    }
    else {
      rowIndex = aRowIndexIn;
    }
  }

  nsVoidArray* row = (nsVoidArray*) mRows.ElementAt(rowIndex);
  if (row) {
    CellData* data = (CellData*)row->ElementAt(colIndex);
    if (data) {
      return data->GetCellFrame();
    }
  }
  return nsnull;
}

PRBool nsCellMap::Grow(nsTableCellMap& aMap,
                       PRInt32         aNumRows,
                       PRInt32         aRowIndex)
{
  PRInt32 numCols = aMap.GetColCount();
  PRInt32 startRowIndex = (aRowIndex >= 0) ? aRowIndex : mRows.Count();
  PRInt32 endRowIndex = startRowIndex + aNumRows - 1;
  for (PRInt32 rowX = startRowIndex; rowX <= endRowIndex; rowX++) {
    nsVoidArray* row;
    row = (0 == numCols) ? new nsVoidArray() : new nsVoidArray(numCols);
    if (row) {
      mRows.InsertElementAt(row, rowX);
    }
    else return PR_FALSE;
  }
  return PR_TRUE;
}

void nsCellMap::GrowRow(nsVoidArray& aRow,
                        PRInt32      aNumCols)
                     
{
  const PRInt32 colsInRow = aRow.Count();
  for (PRInt32 colX = colsInRow; colX < aNumCols; colX++) {
    aRow.AppendElement(nsnull);
  }
}

void
nsCellMap::InsertRows(nsTableCellMap& aMap,
                      nsVoidArray&    aRows,
                      PRInt32         aFirstRowIndex,
                      PRBool          aConsiderSpans)
{
  PRInt32 numCols = aMap.GetColCount();

  if (aFirstRowIndex > mRowCount) {
    NS_ASSERTION(PR_FALSE, "nsCellMap::InsertRows bad first row index");
    return;
  }

  if (!aConsiderSpans) {
    ExpandWithRows(aMap, aRows, aFirstRowIndex);
    return;
  }

  // if any cells span into or out of the row being inserted, then rebuild
  PRBool spansCauseRebuild = CellsSpanInOrOut(aFirstRowIndex, aFirstRowIndex,
                                              0, numCols - 1, numCols);

  // if any of the new cells span out of the new rows being added, then rebuild
  // XXX it would be better to only rebuild the portion of the map that follows the new rows
  if (!spansCauseRebuild && (aFirstRowIndex < mRows.Count())) {
    spansCauseRebuild = CellsSpanOut(aRows);
  }

  if (spansCauseRebuild) {
    RebuildConsideringRows(aMap, aFirstRowIndex, &aRows);
  }
  else {
    ExpandWithRows(aMap, aRows, aFirstRowIndex);
  }
}

void
nsCellMap::RemoveRows(nsTableCellMap& aMap,
                      PRInt32         aFirstRowIndex,
                      PRInt32         aNumRowsToRemove,
                      PRBool          aConsiderSpans)
{
  PRInt32 numRows = mRows.Count();
  PRInt32 numCols = aMap.GetColCount();

  if (aFirstRowIndex >= numRows) {
    return;
  }
  if (!aConsiderSpans) {
    ShrinkWithoutRows(aMap, aFirstRowIndex, aNumRowsToRemove);
    return;
  }
  PRInt32 endRowIndex = aFirstRowIndex + aNumRowsToRemove - 1;
  if (endRowIndex >= numRows) {
    NS_ASSERTION(PR_FALSE, "nsCellMap::RemoveRows tried to remove too many rows");
    endRowIndex = numRows - 1;
  }
  PRBool spansCauseRebuild = CellsSpanInOrOut(aFirstRowIndex, endRowIndex,
                                              0, numCols - 1, numCols);

  if (spansCauseRebuild) {
    RebuildConsideringRows(aMap, aFirstRowIndex, nsnull, aNumRowsToRemove);
  }
  else {
    ShrinkWithoutRows(aMap, aFirstRowIndex, aNumRowsToRemove);
  }
}

PRInt32 nsCellMap::AppendCell(nsTableCellMap&   aMap,
                              nsTableCellFrame& aCellFrame, 
                              PRInt32           aRowIndex,
                              PRBool            aRebuildIfNecessary)
{
  PRInt32 origNumMapRows = mRows.Count();
  PRInt32 origNumCols    = aMap.GetColCount();

  // get the first null CellData in the desired row. It may be 1 past the end if there are none
  PRInt32 startColIndex;
  if (aRowIndex < origNumMapRows) {
    for (startColIndex = 0; startColIndex < origNumCols; startColIndex++) {
      CellData* data = GetMapCellAt(aRowIndex, startColIndex, aMap.GetColCount());
      if (!data) {
        break; // we found the col
      }
    }
  }
  else {
    startColIndex = 0;
  }

  PRBool zeroRowSpan, zeroColSpan;
  PRInt32 rowSpan = GetRowSpan(aCellFrame, aRowIndex, zeroRowSpan);
  PRInt32 colSpan = GetColSpan(aCellFrame, startColIndex, origNumCols, zeroColSpan);

  // if the new cell could potentially span into other rows and collide with 
  // originating cells there, we will play it safe and just rebuild the map
  if (aRebuildIfNecessary && (aRowIndex < mRowCount - 1) && (rowSpan > 1)) {
    nsVoidArray newCellArray;
    newCellArray.AppendElement(&aCellFrame);
    RebuildConsideringCells(aMap, &newCellArray, aRowIndex, startColIndex, PR_TRUE);
    return startColIndex;
  }

  mRowCount = PR_MAX(mRowCount, aRowIndex + 1);

  // add new rows if necessary
  PRInt32 endRowIndex = aRowIndex + rowSpan - 1;
  if (endRowIndex >= origNumMapRows) {
    Grow(aMap, 1 + endRowIndex - origNumMapRows);
  }
  // add new cols to the table map if necessary
  PRInt32 endColIndex = startColIndex + colSpan - 1;
  if (endColIndex >= origNumCols) {
    aMap.AddColsAtEnd(1 + endColIndex - origNumCols);
  }

  // Setup CellData for this cell
  CellData* origData = new CellData(&aCellFrame);
  SetMapCellAt(aMap, *origData, aRowIndex, startColIndex, PR_TRUE);

  // initialize the cell frame
  aCellFrame.InitCellFrame(startColIndex);

  // Create CellData objects for the rows that this cell spans. Set
  // their mOrigCell to nsnull and their mSpanData to point to data.
  for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    for (PRInt32 colX = startColIndex; colX <= endColIndex; colX++) {
      if ((rowX != aRowIndex) || (colX != startColIndex)) { // skip orig cell data done above
        CellData* cellData = GetMapCellAt(rowX, colX, aMap.GetColCount());
        if (cellData) {
          if (cellData->IsOrig()) {
            NS_ASSERTION(PR_FALSE, "cannot overlap originating cell");
            continue;
          }
          if (rowX > aRowIndex) { // row spanning into cell
            if (cellData->IsRowSpan()) {
              NS_ASSERTION(PR_FALSE, "invalid overlap");
            }
            else {
              cellData->SetRowSpanOffset(rowX - aRowIndex);
              if (zeroRowSpan) {
                cellData->SetZeroRowSpan(PR_TRUE);
              }
            }
          }
          if (colX > startColIndex) { // col spanning into cell
            if (!cellData->IsColSpan()) { 
              if (cellData->IsRowSpan()) {
                cellData->SetOverlap(PR_TRUE);
              }
              cellData->SetColSpanOffset(colX - startColIndex);
              if (zeroColSpan) {
                cellData->SetZeroColSpan(PR_TRUE);
              }
              // only count the 1st spanned col of a zero col span
              if (!zeroColSpan || (colX == startColIndex + 1)) {
                nsColInfo* colInfo = aMap.GetColInfoAt(colX);
                colInfo->mNumCellsSpan++;
              }
            }
          }
        }
        else { 
          cellData = new CellData(nsnull);
          if (rowX > aRowIndex) {
            cellData->SetRowSpanOffset(rowX - aRowIndex);
          }
          if (zeroRowSpan) {
            cellData->SetZeroRowSpan(PR_TRUE);
          }
          if (colX > startColIndex) {
            cellData->SetColSpanOffset(colX - startColIndex);
          }
          if (zeroColSpan) {
            cellData->SetZeroColSpan(PR_TRUE);
          }
          SetMapCellAt(aMap, *cellData, rowX, colX, zeroRowSpan);
        }
      }
    }
  }
  //printf("appended cell=%p row=%d \n", aCellFrame, aRowIndex);
  //aMap.Dump();
  return startColIndex;
}

PRBool nsCellMap::CellsSpanOut(nsVoidArray& aRows)
{ 
  PRInt32 numNewRows = aRows.Count();
  for (PRInt32 rowX = 0; rowX < numNewRows; rowX++) {
    nsIFrame* rowFrame = (nsIFrame *) aRows.ElementAt(rowX);
    nsIFrame* cellFrame = nsnull;
    rowFrame->FirstChild(nsnull, &cellFrame);
    while (cellFrame) {
      nsIAtom* frameType;
      cellFrame->GetFrameType(&frameType);
      if (nsLayoutAtoms::tableCellFrame == frameType) {
        PRBool zeroSpan;
        PRInt32 rowSpan = GetRowSpan((nsTableCellFrame &)*cellFrame, rowX, zeroSpan);
        if (rowX + rowSpan > numNewRows) {
          NS_RELEASE(frameType);
          return PR_TRUE;
        }
      }
      NS_IF_RELEASE(frameType);
      cellFrame->GetNextSibling(&cellFrame);
    }
  }
  return PR_FALSE;
}
    
// return PR_TRUE if any cells have rows spans into or out of the region 
// defined by the row and col indices or any cells have colspans into the region
PRBool nsCellMap::CellsSpanInOrOut(PRInt32 aStartRowIndex, 
                                   PRInt32 aEndRowIndex,
                                   PRInt32 aStartColIndex, 
                                   PRInt32 aEndColIndex,
                                   PRInt32 aNumColsInTable)
{
  for (PRInt32 colX = aStartColIndex; colX <= aEndColIndex; colX++) {
    CellData* cellData;
    if (aStartRowIndex > 0) {
      cellData = GetMapCellAt(aStartRowIndex, colX, aNumColsInTable);
      if (cellData && (cellData->IsRowSpan())) {
        return PR_TRUE; // a cell row spans into
      }
    }
    if (aEndRowIndex < mRowCount - 1) {
      cellData = GetMapCellAt(aEndRowIndex + 1, colX, aNumColsInTable);
      if ((cellData) && (cellData->IsRowSpan())) {
        return PR_TRUE; // a cell row spans out
      }
    }
  }
  if (aStartColIndex > 0) {
    for (PRInt32 rowX = aStartRowIndex; rowX <= aEndRowIndex; rowX++) {
      CellData* cellData = GetMapCellAt(rowX, aStartColIndex, aNumColsInTable);
      if (cellData && (cellData->IsColSpan())) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

void nsCellMap::InsertCells(nsTableCellMap& aMap,
                            nsVoidArray&    aCellFrames, 
                            PRInt32         aRowIndex,
                            PRInt32         aColIndexBefore)
{
  if (aCellFrames.Count() == 0) return;
  PRInt32 numCols = aMap.GetColCount();
  if (aColIndexBefore >= numCols) {
    NS_ASSERTION(PR_FALSE, "bad arg in nsCellMap::InsertCellAt");
    return;
  }

  // get the starting col index of the 1st new cells 
  PRInt32 startColIndex;
  for (startColIndex = aColIndexBefore + 1; startColIndex < numCols; startColIndex++) {
    CellData* data = GetMapCellAt(aRowIndex, startColIndex, numCols);
    if (data && data->IsOrig()) {
      break; // we found the col index
    }
  }

  // record whether inserted cells are going to cause complications due 
  // to existing row spans, col spans or table sizing. 
  PRBool spansCauseRebuild = PR_FALSE;

  // check that all cells have the same row span
  PRInt32 numNewCells = aCellFrames.Count();
  PRBool zeroRowSpan = PR_FALSE;
  PRInt32 rowSpan = 0;
  for (PRInt32 cellX = 0; cellX < numNewCells; cellX++) {
    nsTableCellFrame* cell = (nsTableCellFrame*) aCellFrames.ElementAt(cellX);
    PRInt32 rowSpan2 = GetRowSpan(*cell, aRowIndex, zeroRowSpan);
    if (rowSpan == 0) {
      rowSpan = rowSpan2;
    }
    else if (rowSpan != rowSpan2) {
      spansCauseRebuild = PR_TRUE;
      break;
    }
  }

  // check if the new cells will cause the table to add more rows
  if (!spansCauseRebuild) {
    if (mRows.Count() < aRowIndex + rowSpan) {
      spansCauseRebuild = PR_TRUE;
    }
  }

  if (!spansCauseRebuild) {
    spansCauseRebuild = CellsSpanInOrOut(aRowIndex, aRowIndex + rowSpan - 1, 
                                         startColIndex, numCols - 1, numCols);
  }

  if (spansCauseRebuild) {
    RebuildConsideringCells(aMap, &aCellFrames, aRowIndex, startColIndex, PR_TRUE);
  }
  else {
    ExpandWithCells(aMap, aCellFrames, aRowIndex, startColIndex, rowSpan, zeroRowSpan);
  }
}
 
void
nsCellMap::ExpandWithRows(nsTableCellMap& aMap,
                          nsVoidArray&    aRowFrames,
                          PRInt32         aStartRowIndex)
{
  PRInt32 numNewRows  = aRowFrames.Count();
  PRInt32 origNumCols = aMap.GetColCount();
  PRInt32 endRowIndex = aStartRowIndex + numNewRows - 1;

  // create the new rows first
  if (!Grow(aMap, numNewRows, aStartRowIndex)) {
    return;
  }
  mRowCount += numNewRows;

  PRInt32 newRowIndex = 0;
  for (PRInt32 rowX = aStartRowIndex; rowX <= endRowIndex; rowX++) {
    nsTableRowFrame* rFrame = (nsTableRowFrame *)aRowFrames.ElementAt(newRowIndex);
    // append cells 
    nsIFrame* cFrame = nsnull;
    rFrame->FirstChild(nsnull, &cFrame);
    while (cFrame) {
      nsIAtom* cFrameType;
      cFrame->GetFrameType(&cFrameType);
      if (nsLayoutAtoms::tableCellFrame == cFrameType) {
        AppendCell(aMap, (nsTableCellFrame &)*cFrame, rowX, PR_FALSE);
      }
      NS_IF_RELEASE(cFrameType);
      cFrame->GetNextSibling(&cFrame);
    }
    newRowIndex++;
  }
}

void nsCellMap::ExpandWithCells(nsTableCellMap& aMap,
                                nsVoidArray&    aCellFrames,
                                PRInt32         aRowIndex,
                                PRInt32         aColIndex,
                                PRInt32         aRowSpan,
                                PRBool          aRowSpanIsZero)
{
  PRInt32 endRowIndex = aRowIndex + aRowSpan - 1;
  PRInt32 startColIndex = aColIndex;
  PRInt32 endColIndex;
  PRInt32 numCells = aCellFrames.Count();
  PRInt32 totalColSpan = 0;

  // add cellData entries for the space taken up by the new cells
  for (PRInt32 cellX = 0; cellX < numCells; cellX++) {
    nsTableCellFrame* cellFrame = (nsTableCellFrame*) aCellFrames.ElementAt(cellX);
    CellData* origData = new CellData(cellFrame); // the originating cell
    if (!origData) return;

    // set the starting and ending col index for the new cell
    PRBool zeroColSpan = PR_FALSE;
    PRInt32 colSpan = GetColSpan(*cellFrame, aColIndex, aMap.GetColCount(), zeroColSpan);
    totalColSpan += colSpan;
    if (cellX == 0) {
      endColIndex = aColIndex + colSpan - 1;
    }
    else {
      startColIndex = endColIndex + 1;
      endColIndex   = startColIndex + colSpan - 1;
    }
 
    // add the originating cell data and any cell data corresponding to row/col spans
    for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
      nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
      for (PRInt32 colX = aColIndex; colX <= endColIndex; colX++) {
        row->InsertElementAt(nsnull, colX);
        CellData* data = origData;
        if ((rowX != aRowIndex) || (colX != startColIndex)) {
          data = new CellData(nsnull);
          if (!data) return;
          if (rowX > aRowIndex) {
            data->SetRowSpanOffset(rowX - aRowIndex);
            if (aRowSpanIsZero) {
              data->SetZeroRowSpan(PR_TRUE);
            }
          }
          if (colX > startColIndex) {
            data->SetColSpanOffset(colX - startColIndex);
            if (zeroColSpan) {
              data->SetZeroColSpan(PR_TRUE);
            }
          }
        }
        SetMapCellAt(aMap, *data, rowX, colX, (rowX != aRowIndex)); // this increments the mNumColsOrigInCol array
      }
    }
    cellFrame->InitCellFrame(startColIndex); 
  }

  PRInt32 rowX;

  // update the row and col info due to shifting
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    PRInt32 numCols = row->Count();
    PRInt32 colX;
    for (colX = aColIndex + totalColSpan; colX < numCols; colX++) {
      CellData* data = (CellData*) row->ElementAt(colX);
      if (data) {
        // increase the origin and span counts beyond the spanned cols
        if (data->IsOrig()) {
          // a cell that gets moved needs adjustment as well as it new orignating col
          data->GetCellFrame()->SetColIndex(colX);
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig++;
        }
        // if the colspan is 0 only count the 1st spanned col
        PRBool countAsSpan = PR_FALSE;
        if (data->IsColSpan()) {
          if ( (!data->IsZeroColSpan()) ||
               ((data->IsZeroColSpan()) && (colX > aColIndex + totalColSpan) &&
                (!IsZeroColSpan(rowX, colX - 1)))) {
            nsColInfo* colInfo = aMap.GetColInfoAt(colX);
            colInfo->mNumCellsSpan++;
            countAsSpan = PR_TRUE;
          }
        }
        
        // decrease the origin and span counts within the spanned cols
        PRInt32 colX2 = colX - totalColSpan;
        nsColInfo* colInfo2 = aMap.GetColInfoAt(colX2);
        if (data->IsOrig()) {
          // the old originating col of a moved cell needs adjustment
          colInfo2->mNumCellsOrig--;
        }
        else if (countAsSpan) {
          colInfo2->mNumCellsSpan--;
        }
      }
    }
  }
  // remove any unused cols
  aMap.RemoveUnusedCols(aMap.GetColCount());
}

void nsCellMap::ShrinkWithoutRows(nsTableCellMap& aMap,
                                  PRInt32         aStartRowIndex,
                                  PRInt32         aNumRowsToRemove)
{
  PRInt32 endRowIndex = aStartRowIndex + aNumRowsToRemove - 1;
  PRInt32 colCount = aMap.GetColCount();
  for (PRInt32 rowX = endRowIndex; rowX >= aStartRowIndex; rowX--) {
    nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(rowX));
    PRInt32 colX;
    for (colX = 0; colX < colCount; colX++) {
      CellData* data = (CellData *) row->ElementAt(colX);
      if (data) {
        // Adjust the column counts.
        if (data->IsOrig()) {
          // Decrement the column count.
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig--;
        }
        else if (data->IsColSpan()) {
          if ( (!data->IsZeroColSpan()) ||
               ((data->IsZeroColSpan()) && (!IsZeroColSpan(rowX, colX - 1)))) {
            nsColInfo* colInfo = aMap.GetColInfoAt(colX);
            colInfo->mNumCellsSpan--;
          }
        }
      }
    }

    // Delete our row information.
    for (colX = 0; colX < colCount; colX++) {
      CellData* data = (CellData *)(row->ElementAt(colX));
      if (data) {
        delete data;
      }
    }

    mRows.RemoveElementAt(rowX);
    delete row;

    // Decrement our row and next available index counts.
    mRowCount--;

    // remove cols that may not be needed any more due to the removal of the rows
    aMap.RemoveUnusedCols(colCount);
  }
}

PRInt32 nsCellMap::GetColSpan(nsTableCellFrame& aCellFrameToAdd, 
                              PRInt32           aColIndex,
                              PRInt32           aNumColsInTable,
                              PRBool&           aIsZeroColSpan)
{
  aIsZeroColSpan = PR_FALSE;
  PRInt32 colSpan = aCellFrameToAdd.GetColSpan();
  if (0 == colSpan) {
    // use a min value of 2 for a zero colspan to make computations easier elsewhere 
    colSpan = PR_MAX(2, aNumColsInTable - aColIndex);
    aIsZeroColSpan = PR_TRUE;
  }
  return colSpan;
}

PRInt32 nsCellMap::GetColSpan(PRInt32 aRowIndex,
                              PRInt32 aColIndex,
                              PRInt32 aNumColsInTable,
                              PRBool& aZeroColSpan)
{
  aZeroColSpan = PR_FALSE;
  PRInt32 colSpan = 1;
  nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(aRowIndex));
  if (row) {
    PRInt32 numRowCols = row->Count();
    PRBool zeroSpan = PR_FALSE; 
    PRInt32 colX;
    CellData* data;
    for (colX = aColIndex + 1; colX < numRowCols; colX++) {
      data = (CellData *)(row->ElementAt(colX));
      if (data) {
        if (data->IsColSpan()) {
          colSpan++;
          if (data->IsZeroColSpan()) {
            zeroSpan = PR_TRUE;
            aZeroColSpan = PR_TRUE;
          }
        }
        else {
          break;
        }
      }
      else break;
    }
    if (zeroSpan && (colX < aNumColsInTable)) {
      colSpan += aNumColsInTable - colX;
    }
  }
  return colSpan;
}

PRInt32 
nsCellMap::GetRowSpan(nsTableCellFrame& aCellFrameToAdd, 
                      PRInt32           aRowIndex,
                      PRBool&           aIsZeroRowSpan)
{
  aIsZeroRowSpan = PR_FALSE;
  PRInt32 rowSpan = aCellFrameToAdd.GetRowSpan();
  if (0 == rowSpan) {
    // use a min value of 2 for a zero rowspan to make computations easier elsewhere 
    rowSpan = PR_MAX(2, mRows.Count() - aRowIndex);
    aIsZeroRowSpan = PR_TRUE;
  }
  return rowSpan;
}

PRInt32 nsCellMap::GetRowSpan(PRInt32 aRowIndex,
                              PRInt32 aColIndex,
                              PRBool& aZeroRowSpan)
{
  aZeroRowSpan = PR_FALSE;
  PRBool zeroSpan = PR_FALSE; 
  PRInt32 rowSpan = 1;
  PRInt32 rowCount = mRows.Count();
  PRInt32 rowX;
  for (rowX = aRowIndex + 1; rowX < rowCount; rowX++) {
    nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(rowX));
    CellData* data = (CellData *)(row->ElementAt(aColIndex));
    if (data) {
      if (data->IsRowSpan()) {
        rowSpan++;
        if (data->IsZeroRowSpan()) {
          zeroSpan = PR_TRUE;
          aZeroRowSpan = PR_TRUE;
        }
      }
      else {
        break;
      }
    } 
    else break;
  }
  if (zeroSpan && (rowX < rowCount)) {
    rowSpan += rowCount - rowX;
  }
  return rowSpan;
}

void nsCellMap::ShrinkWithoutCell(nsTableCellMap&   aMap,
                                  nsTableCellFrame& aCellFrame,
                                  PRInt32           aRowIndex,
                                  PRInt32           aColIndex)
{
  PRInt32 colX, rowX;

  // get the rowspan and colspan from the cell map since the content may have changed
  PRBool  zeroRowSpan, zeroColSpan;
  PRInt32 rowSpan = GetRowSpan(aRowIndex, aColIndex, zeroRowSpan);
  PRInt32 colSpan = GetColSpan(aRowIndex, aColIndex, aMap.GetColCount(), zeroColSpan);
  PRInt32 endRowIndex = aRowIndex + rowSpan - 1;
  PRInt32 endColIndex = aColIndex + colSpan - 1;

  // adjust the col counts due to the deleted cell before removing it
  for (colX = aColIndex; colX <= endColIndex; colX++) {
    nsColInfo* colInfo = aMap.GetColInfoAt(colX);
    if (colX == aColIndex) {
      colInfo->mNumCellsOrig--;
    }
    else if (!zeroColSpan || (zeroColSpan && (colX > aColIndex + 1))) {
      colInfo->mNumCellsSpan--;
    }
  }

  // remove the deleted cell and cellData entries for it
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    for (colX = endColIndex; colX >= aColIndex; colX--) {
      row->RemoveElementAt(colX);
    }
    // put back null entries in the row to make it the right size
    for (colX = 0; colX < colSpan; colX++) {
      row->AppendElement(nsnull);
    }
  }

  PRInt32 numCols = aMap.GetColCount();

  // update the row and col info due to shifting
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    for (colX = aColIndex; colX < numCols - colSpan; colX++) {
      CellData* data = (CellData*) row->ElementAt(colX);
      if (data) {
        if (data->IsOrig()) {
          // a cell that gets moved to the left needs adjustment in its new location 
          data->GetCellFrame()->SetColIndex(colX);
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig++;
          // a cell that gets moved to the left needs adjustment in its old location 
          colInfo = aMap.GetColInfoAt(colX + colSpan);
          if (colInfo) {
            colInfo->mNumCellsOrig--;
          }
        }
        else if (data->IsColSpan()) {
          if ( (!data->IsZeroColSpan()) ||
               ((data->IsZeroColSpan()) && (colX > aColIndex) && (!IsZeroColSpan(rowX, colX - 1)))) {
            // a cell that gets moved to the left needs adjustment in its new location 
            nsColInfo* colInfo = aMap.GetColInfoAt(colX);
            colInfo->mNumCellsSpan++;
            // a cell that gets moved to the left needs adjustment in its old location 
            colInfo = aMap.GetColInfoAt(colX + colSpan);
            if (colInfo) {
              colInfo->mNumCellsSpan--;
            }
          }
        }
      }
    }
  }

  // remove cols that may not be needed any more due to the removal of the cell
  aMap.RemoveUnusedCols(aMap.GetColCount());
}

void
nsCellMap::RemoveCol(PRInt32 aColIndex)
{
  PRInt32 numMapRows = mRows.Count();
  // remove the col from each of the rows
  for (PRInt32 rowX = 0; rowX < numMapRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    CellData* data = (CellData*) row->ElementAt(aColIndex);
    row->RemoveElementAt(aColIndex);
    delete data;
  }
}

void
nsCellMap::RebuildConsideringRows(nsTableCellMap& aMap,
                                  PRInt32         aStartRowIndex,
                                  nsVoidArray*    aRowsToInsert,
                                  PRBool          aNumRowsToRemove)
{
  // copy the old cell map into a new array
  PRInt32 numOrigRows = mRows.Count();
  PRInt32 numOrigCols = aMap.GetColCount();
  void** origRows = new void*[numOrigRows];
  if (!origRows) return;
  PRInt32 rowX, colX;
  // copy the orig rows
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    origRows[rowX] = row;
  }
  for (colX = 0; colX < numOrigCols; colX++) {
    nsColInfo* colInfo = aMap.GetColInfoAt(colX);
    colInfo->mNumCellsOrig = 0;
  }
  mRows.Clear();
  mRowCount = 0;
  if (aRowsToInsert) { 
    Grow(aMap, numOrigRows); 
  }

  // put back the rows before the affected ones just as before
  for (rowX = 0; rowX < aStartRowIndex; rowX++) {
    nsVoidArray* row = (nsVoidArray *)origRows[rowX];
    PRInt32 numCols = row->Count();
    for (colX = 0; colX < numCols; colX++) {
      // put in the original cell from the cell map
      CellData* data = (CellData*) row->ElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, *data->GetCellFrame(), rowX, PR_FALSE);
      }
    }
  }
  PRInt32 copyStartRowIndex;
  if (aRowsToInsert) {
    // add in the new cells and create rows if necessary
    PRInt32 numNewRows = aRowsToInsert->Count();
    rowX = aStartRowIndex;
    for (PRInt32 newRowX = 0; newRowX < numNewRows; newRowX++) {
      nsTableRowFrame* rFrame = (nsTableRowFrame *)aRowsToInsert->ElementAt(newRowX);
      nsIFrame* cFrame = nsnull;
      rFrame->FirstChild(nsnull, &cFrame);
      while (cFrame) {
        nsIAtom* cFrameType;
        cFrame->GetFrameType(&cFrameType);
        if (nsLayoutAtoms::tableCellFrame == cFrameType) {
          AppendCell(aMap, (nsTableCellFrame &)*cFrame, rowX, PR_FALSE);
        }
        NS_IF_RELEASE(cFrameType);
        cFrame->GetNextSibling(&cFrame);
      }
      rowX++;
    }
    copyStartRowIndex = aStartRowIndex;
  }
  else {
    rowX = aStartRowIndex;
    copyStartRowIndex = aStartRowIndex + aNumRowsToRemove;
  }
  // put back the rows after the affected ones just as before
  PRInt32 copyEndRowIndex = numOrigRows - 1;
  for (PRInt32 copyRowX = copyStartRowIndex; copyRowX <= copyEndRowIndex; copyRowX++) {
    nsVoidArray* row = (nsVoidArray *)origRows[copyRowX];
    PRInt32 numCols = row->Count();
    for (colX = 0; colX < numCols; colX++) {
      // put in the original cell from the cell map
      CellData* data = (CellData*) row->ElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, *data->GetCellFrame(), rowX, PR_FALSE);
      }
    }
    rowX++;
  }

  // delete the old cell map
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)origRows[rowX];
    PRInt32 len = row->Count();
    for (colX = 0; colX < len; colX++) {
      CellData* data = (CellData*) row->ElementAt(colX);
      delete data;
    }
    delete row;
  }
  delete [] origRows;
}

void nsCellMap::RebuildConsideringCells(nsTableCellMap& aMap,
                                        nsVoidArray*    aCellFrames,
                                        PRInt32         aRowIndex,
                                        PRInt32         aColIndex,
                                        PRBool          aInsert)
{
  // copy the old cell map into a new array
  PRInt32 numOrigRows = mRows.Count();
  PRInt32 numOrigCols = aMap.GetColCount();
  void** origRows = new void*[numOrigRows];
  if (!origRows) return;
  PRInt32 rowX;
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowX);
    origRows[rowX] = row;
  }
  // reinitialize data members
  mRows.Clear();
  mRowCount = 0;
  Grow(aMap, numOrigRows);

  PRInt32 numNewCells = (aCellFrames) ? aCellFrames->Count() : 0;
  // build the new cell map 
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)origRows[rowX];
    for (PRInt32 colX = 0; colX < numOrigCols; colX++) {
      if ((rowX == aRowIndex) && (colX == aColIndex)) { 
        if (aInsert) { // put in the new cells
          for (PRInt32 cellX = 0; cellX < numNewCells; cellX++) {
            nsTableCellFrame* cell = (nsTableCellFrame*)aCellFrames->ElementAt(cellX);
            if (cell) {
              AppendCell(aMap, *cell, rowX, PR_FALSE);
            }
          }
        }
        else {
          continue; // do not put the deleted cell back
        }
      }
      // put in the original cell from the cell map
      CellData* data = (CellData*) row->ElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, *data->GetCellFrame(), rowX, PR_FALSE);
      }
    }
  }

  // delete the old cell map
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    nsVoidArray* row = (nsVoidArray *)origRows[rowX];
    PRInt32 len = row->Count();
    for (PRInt32 colX = 0; colX < len; colX++) {
      CellData* data = (CellData*) row->ElementAt(colX);
      delete data;
    }
    delete row;
  }
  delete [] origRows;
}

void nsCellMap::RemoveCell(nsTableCellMap&   aMap,
                           nsTableCellFrame* aCellFrame,
                           PRInt32           aRowIndex)
{
  PRInt32 numRows = mRows.Count();
  if ((aRowIndex < 0) || (aRowIndex >= numRows)) {
    NS_ASSERTION(PR_FALSE, "bad arg in nsCellMap::RemoveCell");
    return;
  }
  PRInt32 numCols = aMap.GetColCount();

  // get the starting col index of the cell to remove
  PRInt32 startColIndex;
  for (startColIndex = 0; startColIndex < numCols; startColIndex++) {
    CellData* data = GetMapCellAt(aRowIndex, startColIndex, aMap.GetColCount());
    if (data && (data->IsOrig()) && (aCellFrame == data->GetCellFrame())) {
      break; // we found the col index
    }
  }

  PRBool isZeroRowSpan;
  PRInt32 rowSpan = GetRowSpan(aRowIndex, startColIndex, isZeroRowSpan);
  PRInt32 endRowIndex = aRowIndex + rowSpan - 1;
  PRBool isZeroColSpan;
  PRInt32 endColIndex = startColIndex + GetColSpan(aRowIndex, startColIndex, numCols, isZeroColSpan) - 1;
  // record whether removing the cells is going to cause complications due 
  // to existing row spans, col spans or table sizing. 
  PRBool spansCauseRebuild = PR_FALSE;

  // check if removing the cell will cause the table to reduce the number of rows
  if (endRowIndex == numRows) {
    spansCauseRebuild = PR_TRUE;
    for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
      for (PRInt32 colX = 0; colX < numCols; colX++) {
        if ((colX < startColIndex) || (colX > endColIndex)) {
          CellData* data = GetMapCellAt(rowX, colX, aMap.GetColCount());
          if (data) {
            // there is either an originating or spanned cell in a row of the cell 
            spansCauseRebuild = PR_FALSE;
            break;
          }
        }
      }
    }
  }

  if (!spansCauseRebuild) {
    spansCauseRebuild = CellsSpanInOrOut(aRowIndex, aRowIndex + rowSpan - 1, 
                                         startColIndex, numCols - 1, numCols);
  }

  if (spansCauseRebuild) {
    RebuildConsideringCells(aMap, nsnull, aRowIndex, startColIndex, PR_FALSE);
  }
  else {
    ShrinkWithoutCell(aMap, *aCellFrame, aRowIndex, startColIndex);
  }
}

#ifdef NS_DEBUG
void nsCellMap::Dump() const
{
  printf("***** START GROUP CELL MAP DUMP *****\n");
  PRInt32 mapRowCount = mRows.Count();
  printf("mapRowCount=%d tableRowCount=%d \n", mapRowCount, mRowCount);

  PRInt32 rowIndex, colIndex;
  for (rowIndex = 0; rowIndex < mapRowCount; rowIndex++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rowIndex);
    printf("row %d : ", rowIndex);
    PRInt32 colCount = row->Count();
    for (colIndex = 0; colIndex < colCount; colIndex++) {
      CellData* cd = (CellData *)row->ElementAt(colIndex);
      if (cd) {
        if (cd->IsOrig()) {
          printf("C%d,%d  ", rowIndex, colIndex);
        } else {
          nsTableCellFrame* cell = nsnull;
          if (cd->IsRowSpan()) {
            cell = GetCellFrame(rowIndex, colIndex, *cd, PR_TRUE);
            // check the validity of the cell map
            nsVoidArray* rowAbove = (nsVoidArray *)mRows.ElementAt(rowIndex - 1);
            CellData* cdAbove = (CellData *)rowAbove->ElementAt(colIndex);
            if (GetCellFrame(rowIndex - 1, colIndex, *cdAbove, PR_TRUE) != cell) {
              NS_ASSERTION(PR_FALSE, "bad row span data");
            }
            printf("R ");
          }
          if (cd->IsColSpan()) {
            cell = GetCellFrame(rowIndex, colIndex, *cd, PR_FALSE);
            // check the validity of the cell map
            CellData* cdBefore = (CellData *)row->ElementAt(colIndex - 1);
            if (GetCellFrame(rowIndex, colIndex - 1, *cdBefore, PR_FALSE) != cell) {
              NS_ASSERTION(PR_FALSE, "bad col span data");
            }
            printf("C ");
          }
          if (!(cd->IsRowSpan() && cd->IsColSpan())) {
            printf("  ");
          } 
          printf("  ");  
        }
      } else {
        printf("----  ");
      }
    }
    printf("\n");
  }

  // output info mapping Ci,j to cell address
  PRInt32 cellCount = 0;
  for (PRInt32 rIndex = 0; rIndex < mapRowCount; rIndex++) {
    nsVoidArray* row = (nsVoidArray *)mRows.ElementAt(rIndex);
    PRInt32 colCount = row->Count();
    for (colIndex = 0; colIndex < colCount; colIndex++) {
      CellData* cd = (CellData *)row->ElementAt(colIndex);
      if (cd) {
        if (cd->IsOrig()) {
          nsTableCellFrame* cellFrame = cd->GetCellFrame();
          PRInt32 cellFrameColIndex;
          cellFrame->GetColIndex(cellFrameColIndex);
          printf("C%d,%d=%p(%d)  ", rIndex, colIndex, cellFrame, cellFrameColIndex);
          cellCount++;
        }
      }
    }
    printf("\n");
  }

  printf("\n***** END GROUP CELL MAP DUMP *****\n");
}
#endif

PRBool 
nsCellMap::IsZeroColSpan(PRInt32 aRowIndex,
                         PRInt32 aColIndex) const
{
  nsVoidArray* row = (nsVoidArray*)mRows.ElementAt(aRowIndex);
  if (row) {
    CellData* data = (CellData*)row->ElementAt(aColIndex);
    if (data && data->IsZeroColSpan()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void 
nsCellMap::AdjustForZeroSpan(PRInt32 aRowIndex,
                             PRInt32 aColIndex,
                             PRInt32 aNumColsInTable)
{
  nsVoidArray* row = (nsVoidArray*)mRows.ElementAt(aRowIndex);
  if (!row) return;
  
  CellData* data = (CellData*)row->ElementAt(aColIndex);
  if (!data) return;

  nsTableCellFrame* cell = (data->IsOrig()) ? data->GetCellFrame() : nsnull;
  if (!cell) return;

  PRInt32 endRowIndex = (0 == cell->GetRowSpan()) ? mRows.Count() - 1 : aRowIndex;
  PRInt32 endColIndex = (0 == cell->GetColSpan()) ? aNumColsInTable - 1 : aColIndex;

  // Create span CellData objects filling out the rows to the end of the
  // map if the rowspan is 0, and/or filling out the cols to the end of 
  // table if the colspan is 0. 
  for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    nsVoidArray* row = (nsVoidArray*)mRows.ElementAt(rowX);
    for (PRInt32 colX = aColIndex; colX <= endColIndex; colX++) {
      if ((colX > aColIndex + 1) || (rowX > aRowIndex + 1)) {
        CellData* oldData = (CellData*)row->ElementAt(colX);
        if (!oldData) {
          CellData* newData = new CellData(nsnull);
          if (!newData) return;
          if (colX > aColIndex) {
            newData->SetColSpanOffset(colX - aColIndex);
            newData->SetZeroColSpan(PR_TRUE);
          }
          if (rowX > aRowIndex) {
            newData->SetRowSpanOffset(rowX - aRowIndex);
            newData->SetZeroRowSpan(PR_TRUE);
          }
          else {
            row->InsertElementAt(newData, colX);
          }
        }
      }
    }
  }
}

CellData* 
nsCellMap::GetMapCellAt(PRInt32 aMapRowIndex, 
                        PRInt32 aColIndex,
                        PRInt32 aNumColsInTable)
{
  if (0 > aMapRowIndex) return nsnull;

  CellData* data = nsnull;
  nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(aMapRowIndex));
  if (row) {
    data = (CellData *)(row->ElementAt(aColIndex));
    if (!data) {
      PRBool didZeroExpand = PR_FALSE;
      // check for special zero row span
      if ((aMapRowIndex > 0) && (aMapRowIndex < mRows.Count())) {
        PRInt32 prevRowX = aMapRowIndex - 1;
        // find the last non null data in the same col
        for ( ; prevRowX > 0; prevRowX--) {
          nsVoidArray* prevRow = (nsVoidArray *)(mRows.ElementAt(prevRowX));
          CellData* prevData = (CellData *)(prevRow->ElementAt(aColIndex));
          if (prevData) {
            nsTableCellFrame* origCell = nsnull;
            if (prevData->IsZeroRowSpan()) {
              PRInt32 rowIndex = prevRowX - prevData->GetRowSpanOffset();
              AdjustForZeroSpan(rowIndex, aColIndex, aNumColsInTable);
              didZeroExpand = PR_TRUE;
            }
            break;
          }
        }
      }
      // check for special zero col span
      if (!didZeroExpand && (aColIndex > 0) && (aColIndex < aNumColsInTable)) { 
        PRInt32 prevColX = aColIndex - 1;
        // find the last non null data in the same row
        for ( ; prevColX > 0; prevColX--) {
          CellData* prevData = (CellData *)(row->ElementAt(prevColX));
          if (prevData) {
            if (prevData->IsZeroColSpan()) {
              PRInt32 colIndex = prevColX - prevData->GetColSpanOffset();
              AdjustForZeroSpan(aMapRowIndex, prevColX, aNumColsInTable);
            }
            break;
          }
        }
      }
    }
  }
  return data;
}

CellData* 
nsCellMap::GetCellAt(PRInt32 aRowIndex, 
                     PRInt32 aColIndex,
                     PRInt32 aNumColsInTable)
{
  if ((0 > aRowIndex) || (aRowIndex >= mRowCount)) {
    return nsnull;
  }
  return GetMapCellAt(aRowIndex, aColIndex, aNumColsInTable);
}

// only called if the cell at aMapRowIndex, aColIndex is null
void nsCellMap::SetMapCellAt(nsTableCellMap& aMap,
                             CellData&       aNewCell, 
                             PRInt32         aMapRowIndex, 
                             PRInt32         aColIndex,
                             PRBool          aCountZeroSpanAsSpan)
{
  nsVoidArray* row = (nsVoidArray *)(mRows.ElementAt(aMapRowIndex));
  if (row) {
    // the table map may need cols added
    PRInt32 numColsToAdd = aColIndex + 1 - aMap.GetColCount();
    if (numColsToAdd > 0) {
      aMap.AddColsAtEnd(numColsToAdd);
    }
    // the row may need cols added
    numColsToAdd = aColIndex + 1 - row->Count();
    if (numColsToAdd > 0) {
      GrowRow(*row, numColsToAdd);
    }
    row->ReplaceElementAt(&aNewCell, aColIndex);
    // update the originating cell counts if cell originates in this row, col
    nsColInfo* colInfo = aMap.GetColInfoAt(aColIndex);
    if (colInfo) {
      if (aNewCell.IsOrig()) { 
        colInfo->mNumCellsOrig++;
      }
      else if ((aNewCell.IsColSpan()) && 
               (!aNewCell.IsZeroColSpan() || aCountZeroSpanAsSpan)) {
        colInfo->mNumCellsSpan++;
      }
    }
    else NS_ASSERTION(PR_FALSE, "SetMapCellAt called with col index > table map num cols");
  }
  else NS_ASSERTION(PR_FALSE, "SetMapCellAt called with row index > num rows");
}

PRInt32 nsCellMap::GetEffectiveColSpan(PRInt32                 aRowIndex,
                                       PRInt32                 aColIndex,
                                       PRInt32                 aNumColsInTable,
                                       const nsTableCellFrame* aCell)
{
  PRInt32 effColSpan = 0;
  for (PRInt32 colX = aColIndex; colX < aNumColsInTable; colX++) {
    PRBool found = PR_FALSE;
    CellData* cellData = GetCellAt(aRowIndex, colX, aNumColsInTable);
    if (cellData) {
      if (cellData->IsOrig()) {
        if (cellData->GetCellFrame() == aCell) {
          found = PR_TRUE;
        }
      }
      else if (cellData->IsColSpan() && 
              (GetCellFrame(aRowIndex, colX, *cellData, PR_FALSE) == aCell)) {
        found = PR_TRUE;
      }
    }
    if (found) {
      effColSpan++;
    }
    else {
      break;
    }
  }

  NS_ASSERTION(effColSpan > 0, "invalid col span or col index");
  return effColSpan;
}

nsTableCellFrame* 
nsCellMap::GetCellFrameOriginatingAt(PRInt32 aRowX, 
                                     PRInt32 aColX,
                                     PRInt32 aNumColsInTable)
{
  CellData* data = GetCellAt(aRowX, aColX, aNumColsInTable);
  if (data && data->IsOrig()) {
    return data->GetCellFrame();
  }
  return nsnull;
}

nsTableCellFrame* 
nsCellMap::GetCellInfoAt(PRInt32  aRowX, 
                         PRInt32  aColX,
                         PRInt32  aNumColsInTable, 
                         PRBool*  aOriginates, 
                         PRInt32* aColSpan)
{
  if (aOriginates) {
    *aOriginates = PR_FALSE;
  }
  CellData* data = GetCellAt(aRowX, aColX, aNumColsInTable);
  nsTableCellFrame* cellFrame = nsnull;  
  if (data) {
    if (data->IsOrig()) {
      cellFrame = data->GetCellFrame();
      if (aOriginates)
        *aOriginates = PR_TRUE;
      if (aColSpan) {
        PRInt32 initialColIndex;
        cellFrame->GetColIndex(initialColIndex);
        *aColSpan = GetEffectiveColSpan(aRowX, initialColIndex, aNumColsInTable, cellFrame);
      }
    }
    else {
      cellFrame = GetCellFrame(aRowX, aColX, *data, PR_TRUE);
      if (aColSpan)
        *aColSpan = 0;
    }
  }
  return cellFrame;
}
  

PRBool nsCellMap::RowIsSpannedInto(PRInt32 aRowIndex,
                                   PRInt32 aNumColsInTable)
{
  if ((0 > aRowIndex) || (aRowIndex >= mRowCount)) {
    return PR_FALSE;
  }
	for (PRInt32 colIndex = 0; colIndex < aNumColsInTable; colIndex++) {
		CellData* cd = GetCellAt(aRowIndex, colIndex, aNumColsInTable);
		if (cd) { // there's really a cell at (aRowIndex, colIndex)
			if (cd->IsSpan()) { // the cell at (aRowIndex, colIndex) is the result of a span
        if (cd->IsRowSpan() && GetCellFrame(aRowIndex, colIndex, *cd, PR_TRUE)) { // XXX why the last check
          return PR_TRUE;
        }
			}
		}
  }
  return PR_FALSE;
}

PRBool nsCellMap::RowHasSpanningCells(PRInt32 aRowIndex,
                                      PRInt32 aNumColsInTable)
{
  if ((0 > aRowIndex) || (aRowIndex >= mRowCount)) {
    return PR_FALSE;
  }
  if (aRowIndex != mRowCount - 1) {
    // aRowIndex is not the last row, so we check the next row after aRowIndex for spanners
    for (PRInt32 colIndex = 0; colIndex < aNumColsInTable; colIndex++) {
      CellData* cd = GetCellAt(aRowIndex, colIndex, aNumColsInTable);
      if (cd && (cd->IsOrig())) { // cell originates 
        CellData* cd2 = GetCellAt(aRowIndex + 1, colIndex, aNumColsInTable);
        if (cd2 && cd2->IsRowSpan()) { // cd2 is spanned by a row
          if (cd->GetCellFrame() == GetCellFrame(aRowIndex + 1, colIndex, *cd2, PR_TRUE)) {
            return PR_TRUE;
          }
        }
      }
    }
  }
  return PR_FALSE;
}

PRBool nsCellMap::ColIsSpannedInto(PRInt32 aColIndex,
                                   PRInt32 aNumColsInTable)
{
  if ((0 > aColIndex) || (aColIndex >= aNumColsInTable)) {
    return PR_FALSE;
  }
	for (PRInt32 rowIndex = 0; rowIndex < mRowCount; rowIndex++) {
		CellData* cd = GetCellAt(rowIndex, aColIndex, aNumColsInTable);
		if (cd) { // there's really a cell at (aRowIndex, colIndex)
			if (cd->IsSpan()) { // the cell at (aRowIndex, colIndex) is the result of a span
        if (cd->IsColSpan() && GetCellFrame(rowIndex, aColIndex, *cd, PR_FALSE)) { // XXX why the last check
          return PR_TRUE;
        }
			}
		}
  }
  return PR_FALSE;
}

PRBool nsCellMap::ColHasSpanningCells(PRInt32 aColIndex,
                                      PRInt32 aNumColsInTable)
{
  NS_PRECONDITION (aColIndex < aNumColsInTable, "bad col index arg");
  if ((0 > aColIndex) || (aColIndex >= aNumColsInTable - 1)) 
    return PR_FALSE;
 
  for (PRInt32 rowIndex = 0; rowIndex < mRowCount; rowIndex++) {
    CellData* cd = GetCellAt(rowIndex, aColIndex, aNumColsInTable);
    if (cd && (cd->IsOrig())) { // cell originates 
      CellData* cd2 = GetCellAt(rowIndex + 1, aColIndex, aNumColsInTable);
      if (cd2 && cd2->IsColSpan()) { // cd2 is spanned by a col
        if (cd->GetCellFrame() == GetCellFrame(rowIndex + 1, aColIndex, *cd2, PR_FALSE)) {
          return PR_TRUE;
        }
      }
    }
  }
  return PR_FALSE;
}

#ifdef DEBUG
void nsCellMap::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  NS_PRECONDITION(aResult, "null OUT parameter pointer");
  PRUint32 sum = sizeof(*this);

  // Add in the size of the void arrays. Because we have emnbedded objects
  // and not pointers to void arrays, we need to subtract out the size of the
  // embedded object so it isn't double counted
  PRUint32 voidArraySize;

  mRows.SizeOf(aHandler, &voidArraySize);
  sum += voidArraySize - sizeof(mRows);

  *aResult = sum;
}
#endif
