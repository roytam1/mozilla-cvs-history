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

#ifndef FixedTableLayoutStrategy_h__
#define FixedTableLayoutStrategy_h__

#include "nscore.h"
#include "BasicTableLayoutStrategy.h"
#include "nsCoord.h"

class nsVoidArray;
class nsTableFrame;
struct nsStylePosition;


/* ---------- FixedTableLayoutStrategy ---------- */

/** Implementation of HTML4 "table-layout=fixed" table layout.
  * The input to this class is the resolved styles for both the
  * table columns and the cells in row0.
  * The output from this class is to set the column widths in
  * mTableFrame.
  */
class FixedTableLayoutStrategy : public BasicTableLayoutStrategy
{
public:

  /** Public constructor.
    * @paran aFrame           the table frame for which this delegate will do layout
    * @param aNumCols         the total number of columns in the table    
    */
  FixedTableLayoutStrategy(nsTableFrame *aFrame);

  /** destructor */
  virtual ~FixedTableLayoutStrategy();

  /** Called during resize reflow to determine the new column widths
    * @param aTableStyle - the resolved style for mTableFrame
	  * @param aReflowState - the reflow state for mTableFrame
	  * @param aMaxWidth - the computed max width for columns to fit into
	  */
  virtual PRBool  BalanceColumnWidths(nsIStyleContext*         aTableStyle,
                                      const nsHTMLReflowState& aReflowState,
                                      nscoord                  aMaxWidth);

  // see nsTableFrame::ColumnsCanBeInvalidatedBy
  PRBool ColumnsCanBeInvalidatedBy(nsStyleCoord*           aPrevStyleWidth,
                                   const nsTableCellFrame& aCellFrame) const;

  // see nsTableFrame::ColumnsCanBeInvalidatedBy
  PRBool ColumnsCanBeInvalidatedBy(const nsTableCellFrame& aCellFrame,
                                   PRBool                  aConsiderMinWidth = PR_FALSE) const;

  // see nsTableFrame::ColumnsCanBeInvalidatedBy
  PRBool ColumnsAreValidFor(const nsTableCellFrame& aCellFrame,
                            nscoord                 aPrevCellMin,
                            nscoord                 aPrevCellDes) const;

protected:
   /* assign the width of all columns
    * if there is a colframe with a width attribute, use it as the column width.
    * otherwise if there is a cell in the first row and it has a width attribute, use it.
    *  if this cell includes a colspan, width is divided equally among spanned columns
    * otherwise the cell get a proportion of the remaining space. 
    *  as determined by the table width attribute.  If no table width attribute, it gets 0 width
    * Computes the minimum and maximum table widths.
    *
    * Set column width information in each column frame and in the table frame.
    *
    * @return PR_TRUE if all is well, PR_FALSE if there was an unrecoverable error
    *
    */
  virtual PRBool AssignPreliminaryColumnWidths(nscoord aComputedWidth);


};


#endif

