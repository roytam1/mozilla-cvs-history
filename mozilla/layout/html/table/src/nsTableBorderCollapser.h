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
#ifndef nsTableBorderCollapser_h__
#define nsTableBorderCollapser_h__

#include "nsIStyleContext.h"
class nsTableFrame;

/**
  * Class for handling CSS border-style:collapse
  */
class nsTableBorderCollapser 
{
public:
  nsTableBorderCollapser(nsTableFrame& aTableFrame);

  ~nsTableBorderCollapser();

  /** notification that top and bottom borders have been computed */ 
  void DidComputeHorizontalBorders(nsIPresContext& aPresContext,
                                   PRInt32         aStartRowIndex,
                                   PRInt32         aEndRowIndex);

  /** compute the left and right collapsed borders between aStartRowIndex and aEndRowIndex, inclusive */
  void ComputeVerticalBorders(nsIPresContext& aPresContext,
                              PRInt32         aStartRowIndex, 
                              PRInt32          aEndRowIndex);

  /** compute the top and bottom collapsed borders between aStartRowIndex and aEndRowIndex, inclusive */
  void ComputeHorizontalBorders(nsIPresContext& aPresContext,
                                PRInt32         aStartRowIndex, 
                                PRInt32         aEndRowIndex);

  /** compute the left borders for the table objects intersecting at (aRowIndex, aColIndex) */
  void ComputeLeftBorderForEdgeAt(nsIPresContext& aPresContext,
                                  PRInt32         aRowIndex, 
                                  PRInt32         aColIndex);

  /** compute the right border for the table cell at (aRowIndex, aColIndex)
    * and the appropriate border for that cell's right neighbor 
    * (the left border for a neighboring cell, or the right table edge) 
    */
  void ComputeRightBorderForEdgeAt(nsIPresContext& aPresContext,
                                   PRInt32         aRowIndex, 
                                   PRInt32         aColIndex);

  /** compute the top borders for the table objects intersecting at (aRowIndex, aColIndex) */
  void ComputeTopBorderForEdgeAt(nsIPresContext& aPresContext,
                                 PRInt32         aRowIndex, 
                                 PRInt32         aColIndex);

  /** compute the bottom border for the table cell at (aRowIndex, aColIndex)
    * and the appropriate border for that cell's bottom neighbor 
    * (the top border for a neighboring cell, or the bottom table edge) 
    */
  void ComputeBottomBorderForEdgeAt(nsIPresContext& aPresContext,
                                    PRInt32         aRowIndex, 
                                    PRInt32         aColIndex);
  
  /** at the time we initially compute collapsing borders, we don't yet have the 
    * column widths.  So we set them as a post-process of the column balancing algorithm.
    */
  void SetHorizontalEdgeLengths();

  /** @return the identifier representing the edge opposite from aEdge (left-right, top-bottom) */
  PRUint8 GetOpposingEdge(PRUint8 aEdge);

  /** @return the computed width for aSide of aBorder */
  nscoord GetWidthForSide(const nsMargin& aBorder, 
	                      PRUint8         aSide);

  /** returns BORDER_PRECEDENT_LOWER if aStyle1 is lower precedent that aStyle2
    *         BORDER_PRECEDENT_HIGHER if aStyle1 is higher precedent that aStyle2
    *         BORDER_PRECEDENT_EQUAL if aStyle1 and aStyle2 have the same precedence
    *         (note, this is not necessarily the same as saying aStyle1==aStyle2)
    * according to the CSS-2 collapsing borders for tables precedent rules.
    */
  PRUint8 CompareBorderStyles(PRUint8 aStyle1, 
	                            PRUint8 aStyle2);

  /** helper to set the length of an edge for aSide border of this table frame */
  void SetBorderEdgeLength(PRUint8 aSide, 
                           PRInt32 aIndex, 
                           nscoord aLength);

  /** Compute the style, width, and color of an edge in a collapsed-border table.
    * This method is the CSS2 border conflict resolution algorithm
    * The spec says to resolve conflicts in this order:<br>
    * 1. any border with the style HIDDEN wins<br>
    * 2. the widest border with a style that is not NONE wins<br>
    * 3. the border styles are ranked in this order, highest to lowest precedence:<br>
    *       double, solid, dashed, dotted, ridge, outset, groove, inset<br>
    * 4. borders that are of equal width and style (differ only in color) have this precedence:<br>
    *       cell, row, rowgroup, col, colgroup, table<br>
    * 5. if all border styles are NONE, then that's the computed border style.<br>
    * This method assumes that the styles were added to aStyles in the reverse precedence order
    * of their frame type, so that styles that come later in the list win over style 
    * earlier in the list if the tie-breaker gets down to #4.
    * This method sets the out-param aBorder with the resolved border attributes
    *
    * @param aSide   the side that is being compared
    * @param aStyles the resolved styles of the table objects intersecting at aSide
    *                styles must be added to this list in reverse precedence order
    * @param aBorder [OUT] the border edge that we're computing.  Results of the computation
    *                      are stored in aBorder:  style, color, and width.
    * @param aFlipLastSide an indication of what the bordering object is:  another cell, or the table itself.
    */
  void ComputeBorderSegment(PRUint8       aSide, 
                            nsVoidArray*  aStyles, 
                            nsBorderEdge& aBorder,
                            PRBool        aFlipLastSide);

  void GetBorder(nsMargin& aBorder);

  void GetBorderAt(PRInt32   aRowIndex,
                   PRInt32   aColIndex,
                   nsMargin& aBorder);

  void GetMaxBorder(PRInt32  aStartRowIndex,
                    PRInt32  aEndRowIndex,
                    PRInt32  aStartColIndex,
                    PRInt32  aEndColIndex,
                    nsMargin aBorder);

  nsBorderEdges* GetEdges();

protected:
  nsBorderEdges mBorderEdges;
  nsTableFrame& mTableFrame;
};

inline nsBorderEdges* nsTableBorderCollapser::GetEdges() 
{
  return &mBorderEdges;
}

#endif






