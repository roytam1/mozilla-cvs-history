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
#ifndef nsTableRowGroupFrame_h__
#define nsTableRowGroupFrame_h__

#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsIAtom.h"

class nsTableRowFrame;
struct RowGroupReflowState;

/**
 * nsTableRowGroupFrame is the frame that maps row groups 
 * (HTML tags THEAD, TFOOT, and TBODY). This class cannot be reused
 * outside of an nsTableFrame.  It assumes that its parent is an nsTableFrame, and 
 * its children are nsTableRowFrames.
 * 
 * @see nsTableFrame
 * @see nsTableRowFrame
 *
 * @author  sclark
 */
class nsTableRowGroupFrame : public nsHTMLContainerFrame
{
public:

  /** instantiate a new instance of nsTableRowGroupFrame.
    * @param aResult    the new object is returned in this out-param
    * @param aContent   the table object to map
    * @param aParent    the parent of the new frame
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  friend nsresult 
  NS_NewTableRowGroupFrame(nsIContent* aContent,
                           nsIFrame*   aParentFrame,
                           nsIFrame*&  aResult);

  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  /** @see nsIFrame::Paint */
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  /** ask all children to paint themselves, without clipping (for cells with rowspan>1)
    * @see nsIFrame::Paint 
    */
  virtual void PaintChildren(nsIPresContext&      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect);

  /**
   * Find the correct descendant frame.
   * Return PR_TRUE if a frame containing the point is found.
   * @see nsContainerFrame::GetFrameForPoint
   */
  NS_IMETHOD GetFrameForPoint(const nsPoint& aPoint, nsIFrame** aFrame);

   /** calls Reflow for all of its child rows.
    * Rows are all set to the same width and stacked vertically.
    * <P> rows are not split unless absolutely necessary.
    *
    * @param aDesiredSize width set to width of rows, height set to 
    *                     sum of height of rows that fit in aMaxSize.height.
    *
    * @see nsIFrame::Reflow
    */
  NS_IMETHOD Reflow(nsIPresContext& aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  NS_IMETHOD GetFrameName(nsString& aResult) const;

  /** returns the type of the mapped row group content in aType.
    * caller MUST call release on the returned object if it is not null.
    *
    * @param  aType out param filled with the type of the mapped content, or null if none.
    *
    * @return NS_OK
    */ 
  NS_IMETHOD GetRowGroupType(nsIAtom *& aType);

  /** set aCount to the number of child rows (not necessarily == number of child frames) */
  NS_METHOD GetRowCount(PRInt32 &aCount);

  /** get the maximum number of columns taken up by any row in this rowgroup */
  NS_METHOD GetMaxColumns(PRInt32 &aMaxColumns) const;

  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0, nsIListFilter *aFilter = nsnull) const;


protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableRowGroupFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  /** protected destructor */
  ~nsTableRowGroupFrame();

  /** implement abstract method on nsHTMLContainerFrame */
  virtual PRIntn GetSkipSides() const;

  nscoord GetTopMarginFor(nsIPresContext*      aCX,
                          RowGroupReflowState& aReflowState,
                          const nsMargin&      aKidMargin);

  void          PlaceChild( nsIPresContext&      aPresContext,
                            RowGroupReflowState& aReflowState,
                            nsIFrame*            aKidFrame,
                            const nsRect&        aKidRect,
                            nsSize*              aMaxElementSize,
                            nsSize&              aKidMaxElementSize);

  void ShrinkWrapChildren(nsIPresContext* aPresContext, 
                          nsHTMLReflowMetrics& aDesiredSize);


  /** Incremental Reflow attempts to do column balancing with the minimum number of reflow
    * commands to child elements.  This is done by processing the reflow command,
    * rebalancing column widths (if necessary), then comparing the resulting column widths
    * to the prior column widths and reflowing only those cells that require a reflow.
    *
    * @see Reflow
    */
  NS_IMETHOD IncrementalReflow(nsIPresContext&      aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               RowGroupReflowState& aReflowState,
                               nsReflowStatus&      aStatus);

  NS_IMETHOD IR_TargetIsChild(nsIPresContext&      aPresContext,
                              nsHTMLReflowMetrics& aDesiredSize,
                              RowGroupReflowState& aReflowState,
                              nsReflowStatus&      aStatus,
                              nsIFrame *           aNextFrame);

  NS_IMETHOD IR_TargetIsMe(nsIPresContext&      aPresContext,
                           nsHTMLReflowMetrics& aDesiredSize,
                           RowGroupReflowState& aReflowState,
                           nsReflowStatus&      aStatus);

  NS_IMETHOD IR_RowInserted(nsIPresContext&      aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            RowGroupReflowState& aReflowState,
                            nsReflowStatus&      aStatus,
                            nsTableRowFrame *    aInsertedFrame,
                            PRBool               aReplace);

  NS_IMETHOD IR_RowAppended(nsIPresContext&      aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            RowGroupReflowState& aReflowState,
                            nsReflowStatus&      aStatus,
                            nsTableRowFrame *    aAppendedFrame);

  NS_IMETHOD IR_RowRemoved(nsIPresContext&      aPresContext,
                           nsHTMLReflowMetrics& aDesiredSize,
                           RowGroupReflowState& aReflowState,
                           nsReflowStatus&      aStatus,
                           nsTableRowFrame *    aDeletedFrame);

  NS_IMETHOD IR_StyleChanged(nsIPresContext&      aPresContext,
                             nsHTMLReflowMetrics& aDesiredSize,
                             RowGroupReflowState& aReflowState,
                             nsReflowStatus&      aStatus);

  NS_IMETHOD DidAppendRow(nsTableRowFrame *aRowFrame);

  PRBool NoRowsFollow();

  /** @see nsContainerFrame::CreateContinuingFrame */
  NS_IMETHOD CreateContinuingFrame(nsIPresContext&  aPresContext,
                                   nsIFrame*        aParent,
                                   nsIStyleContext* aStyleContext,
                                   nsIFrame*&       aContinuingFrame);

  nsresult AdjustSiblingsAfterReflow(nsIPresContext&      aPresContext,
                                     RowGroupReflowState& aReflowState,
                                     nsIFrame*            aKidFrame,
                                     nscoord              aDeltaY);

  /**
   * Reflow the frames we've already created
   *
   * @param   aPresContext presentation context to use
   * @param   aReflowState current inline state
   * @return  true if we successfully reflowed all the mapped children and false
   *            otherwise, e.g. we pushed children to the next in flow
   */
  NS_METHOD     ReflowMappedChildren(nsIPresContext&      aPresContext,
                                     nsHTMLReflowMetrics& aDesiredSize,
                                     RowGroupReflowState& aReflowState,
                                     nsReflowStatus&      aStatus,
                                     nsTableRowFrame *    aStartFrame,
                                     nsReflowReason       aReason,
                                     PRBool               aDoSiblings);

  /**
   * Try and pull-up frames from our next-in-flow
   *
   * @param   aPresContext presentation context to use
   * @param   aReflowState current inline state
   * @return  true if we successfully pulled-up all the children and false
   *            otherwise, e.g. child didn't fit
   */
  NS_METHOD     PullUpChildren(nsIPresContext&      aPresContext,
                               nsHTMLReflowMetrics& aDesiredSize,
                               RowGroupReflowState& aReflowState,
                               nsReflowStatus&      aStatus);

private:
  nsIAtom *mType;

};


#endif
