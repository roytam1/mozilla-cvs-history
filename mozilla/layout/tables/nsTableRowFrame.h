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
#ifndef nsTableRowFrame_h__
#define nsTableRowFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"

class  nsTableFrame;
class  nsTableCellFrame;
struct RowReflowState;

/**
 * nsTableRowFrame is the frame that maps table rows 
 * (HTML tag TR). This class cannot be reused
 * outside of an nsTableRowGroupFrame.  It assumes that its parent is an nsTableRowGroupFrame,  
 * and its children are nsTableCellFrames.
 * 
 * @see nsTableFrame
 * @see nsTableRowGroupFrame
 * @see nsTableCellFrame
 */
class nsTableRowFrame : public nsContainerFrame
{
public:
  /** Initialization of frame as a row */
  void InitRowData(PRInt32 aRowIndex);

  /** Initialization of data */
  NS_IMETHOD InitChildren(PRInt32 aRowIndex=-1);

  void ResetInitChildren();

  /** instantiate a new instance of nsTableRowFrame.
    * @param aResult    the new object is returned in this out-param
    * @param aContent   the table object to map
    * @param aParent    the parent of the new frame
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  friend nsresult 
  NS_NewTableRowFrame(nsIContent* aContent,
                      nsIFrame*   aParentFrame,
                      nsIFrame*&  aResult);

  NS_IMETHOD Init(nsIPresContext& aPresContext, nsIFrame* aChildList);

  /** @see nsIFrame::Paint */
  NS_IMETHOD Paint(nsIPresContext&      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);


  /** ask all children to paint themselves, without clipping (for cells with rowspan>1)
    * @see nsIFrame::Paint 
    */
  virtual void PaintChildren(nsIPresContext&      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect);

  /** calls Reflow for all of its child cells.
    * Cells with rowspan=1 are all set to the same height and stacked horizontally.
    * <P> Cells are not split unless absolutely necessary.
    * <P> Cells are resized in nsTableFrame::BalanceColumnWidths 
    * and nsTableFrame::ShrinkWrapChildren
    *
    * @param aDesiredSize width set to width of the sum of the cells, height set to 
    *                     height of cells with rowspan=1.
    *
    * @see nsIFrame::Reflow
    * @see nsTableFrame::BalanceColumnWidths
    * @see nsTableFrame::ShrinkWrapChildren
    */
  NS_IMETHOD Reflow(nsIPresContext&      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  NS_IMETHOD DidReflow(nsIPresContext& aPresContext,
                       nsDidReflowStatus aStatus);

  /** @see nsContainerFrame::CreateContinuingFrame */
  NS_IMETHOD CreateContinuingFrame(nsIPresContext&  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame);
  
  /** set mTallestCell to 0 in anticipation of recalculating it */
  void ResetMaxChildHeight();

  /** set mTallestCell to max(mTallestCell, aChildHeight) */ 
  void SetMaxChildHeight(nscoord aChildHeight, nscoord aCellTopMargin, nscoord aCellBottomMargin);

  /** returns the tallest child in this row (ignoring any cell with rowspans) */
  nscoord GetTallestChild() const;
  nscoord GetChildMaxTopMargin() const;
  nscoord GetChildMaxBottomMargin() const;

  PRInt32 GetMaxColumns() const;

  /** returns the ordinal position of this row in its table */
  virtual PRInt32 GetRowIndex() const;

  /** set this row's starting row index */
  virtual void SetRowIndex (int aRowIndex);

  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0, nsIListFilter *aFilter = nsnull) const;

protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableRowFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  /** destructor */
  virtual ~nsTableRowFrame();

  /** Incremental Reflow attempts to do column balancing with the minimum number of reflow
    * commands to child elements.  This is done by processing the reflow command,
    * rebalancing column widths (if necessary), then comparing the resulting column widths
    * to the prior column widths and reflowing only those cells that require a reflow.
    *
    * @see Reflow
    */
  NS_IMETHOD IncrementalReflow(nsIPresContext&       aPresContext,
                               nsHTMLReflowMetrics&  aDesiredSize,
                               RowReflowState&       aReflowState,
                               nsReflowStatus&       aStatus);

  NS_IMETHOD IR_TargetIsChild(nsIPresContext&      aPresContext,
                              nsHTMLReflowMetrics& aDesiredSize,
                              RowReflowState&      aReflowState,
                              nsReflowStatus&      aStatus,
                              nsIFrame *           aNextFrame);

  NS_IMETHOD IR_TargetIsMe(nsIPresContext&      aPresContext,
                           nsHTMLReflowMetrics& aDesiredSize,
                           RowReflowState&      aReflowState,
                           nsReflowStatus&      aStatus);

  NS_IMETHOD IR_CellInserted(nsIPresContext&     aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            RowReflowState&      aReflowState,
                            nsReflowStatus&      aStatus,
                            nsIFrame *           aInsertedFrame,
                            PRBool               aReplace);

  NS_IMETHOD IR_CellAppended(nsIPresContext&     aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            RowReflowState&      aReflowState,
                            nsReflowStatus&      aStatus,
                            nsIFrame *           aAppendedFrame);

  NS_IMETHOD IR_DidAppendCell(nsTableRowFrame *aRowFrame);

  NS_IMETHOD IR_CellRemoved(nsIPresContext&     aPresContext,
                           nsHTMLReflowMetrics& aDesiredSize,
                           RowReflowState&      aReflowState,
                           nsReflowStatus&      aStatus,
                           nsIFrame *           aDeletedFrame);
  
  NS_IMETHOD IR_UnknownFrameInserted(nsIPresContext&      aPresContext,
                                     nsHTMLReflowMetrics& aDesiredSize,
                                     RowReflowState&      aReflowState,
                                     nsReflowStatus&      aStatus,
                                     nsIFrame *           aInsertedFrame,
                                     PRBool               aReplace);

  NS_IMETHOD IR_UnknownFrameRemoved(nsIPresContext&      aPresContext,
                                    nsHTMLReflowMetrics& aDesiredSize,
                                    RowReflowState&      aReflowState,
                                    nsReflowStatus&      aStatus,
                                    nsIFrame *           aDeletedFrame);


  // row-specific methods

  void GetMinRowSpan(nsTableFrame *aTableFrame);

  void FixMinCellHeight(nsTableFrame *aTableFrame);

  nsresult RecoverState(nsIPresContext& aPresContext,
                        RowReflowState& aState,
                        nsIFrame*       aKidFrame,
                        nscoord&        aMaxCellTopMargin,
                        nscoord&        aMaxCellBottomMargin);

  void PlaceChild(nsIPresContext& aPresContext,
                  RowReflowState& aState,
                  nsIFrame*       aKidFrame,
                  const nsRect&   aKidRect,
                  nsSize*         aMaxElementSize,
                  nsSize*         aKidMaxElementSize);

  nscoord ComputeCellXOffset(const RowReflowState& aState,
                             nsIFrame*             aKidFrame,
                             const nsMargin&       aKidMargin) const;
  nscoord ComputeCellAvailWidth(const RowReflowState& aState,
                                nsIFrame*             aKidFrame) const;

  /**
   * Called for a resize reflow. Typically because the column widths have
   * changed. Reflows all the existing table cell frames
   */
  nsresult ResizeReflow(nsIPresContext&  aPresContext,
                        RowReflowState&  aState,
                        nsHTMLReflowMetrics& aDesiredSize);

  /**
   * Called for the initial reflow. Creates each table cell frame, and
   * reflows the cell frame to gets its minimum and maximum sizes
   */
  nsresult InitialReflow(nsIPresContext&  aPresContext,
                         RowReflowState&  aState,
                         nsHTMLReflowMetrics& aDesiredSize);

  /**
   * Called for incremental reflow
   */
  nsresult IncrementalReflow(nsIPresContext&  aPresContext,
                             RowReflowState&  aState,
                             nsHTMLReflowMetrics& aDesiredSize);

private:
  PRInt32  mRowIndex;
  nscoord  mTallestCell;          // not my height, but the height of my tallest child
  nscoord  mCellMaxTopMargin;
  nscoord  mCellMaxBottomMargin;
  PRInt32  mMinRowSpan;           // the smallest row span among all my child cells
  PRBool   mInitializedChildren;  // PR_TRUE if child cells have been initialized 
                                  // (for now, that means "added to the table", and
                                  // is NOT the same as having nsIFrame::Init() called.)

};


inline void nsTableRowFrame::InitRowData(PRInt32 aRowIndex)
{
  NS_ASSERTION(0<=aRowIndex, "bad param row index");
  mRowIndex = aRowIndex;
}

inline PRInt32 nsTableRowFrame::GetRowIndex() const
{
  NS_ASSERTION(0<=mRowIndex, "bad state: row index");
  return (mRowIndex);
}

inline void nsTableRowFrame::ResetInitChildren()
{ mInitializedChildren=PR_FALSE; } 

#endif
