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
#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "nsITableCellLayout.h"
#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsTableRowFrame.h"  // need to actually include this here to inline GetRowIndex
#include "nsIStyleContext.h"

struct nsStyleSpacing;
class nsTableFrame;
class nsHTMLValue;

/**
 * Additional frame-state bits
 */
#define NS_TABLE_CELL_FRAME_CONTENT_EMPTY 0x80000000

/**
 * nsTableCellFrame
 * data structure to maintain information about a single table cell's frame
 *
 * NOTE:  frames are not ref counted.  We expose addref and release here
 * so we can change that decsion in the future.  Users of nsITableCellLayout
 * should refcount correctly as if this object is being ref counted, though
 * no actual support is under the hood.
 *
 * @author  sclark
 */
class nsTableCellFrame : public nsHTMLContainerFrame, public nsITableCellLayout
{
public:

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  // default constructor supplied by the compiler

  nsTableCellFrame();
  ~nsTableCellFrame();

  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aHint);

  // table cells contain an area frame which does most of the work, and
  // so these functions should never be called. They assert and return
  // NS_ERROR_NOT_IMPLEMENTED
  NS_IMETHOD AppendFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  void InitCellFrame(PRInt32 aColIndex);

  void SetBorderEdge(PRUint8 aSide, 
                     PRInt32 aRowIndex, 
                     PRInt32 aColIndex, 
                     nsBorderEdge *border,
                     nscoord aOddAmountToAdd);

  void SetBorderEdgeLength(PRUint8 aSide, 
                           PRInt32 aIndex, 
                           nscoord aLength);


  /** instantiate a new instance of nsTableCellFrame.
    * @param aResult    the new object is returned in this out-param
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  friend nsresult 
  NS_NewTableCellFrame(nsIPresShell* aPresShell, nsIFrame** aResult);

  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                              nsFramePaintLayer aWhichLayer,
                              nsIFrame**     aFrame);

  NS_IMETHOD SetSelected(nsIPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);

  NS_IMETHOD Reflow(nsIPresContext*      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::tableCellFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

  virtual void VerticallyAlignChild(nsIPresContext*          aPresContext,
                                    const nsHTMLReflowState& aReflowState,
                                    nscoord                  aMaxAscent);

  PRBool HasVerticalAlignBaseline();

  /**
   * return the cell's specified row span. this is what was specified in the
   * content model or in the style info, and is always >= 1.
   * to get the effective row span (the actual value that applies), use GetEffectiveRowSpan()
   * @see nsTableFrame::GetEffectiveRowSpan()
   */
  virtual PRInt32 GetRowSpan();

  // there is no set row index because row index depends on the cell's parent row only

  /*---------------- nsITableCellLayout methods ------------------------*/

  /**
   * return the cell's starting row index (starting at 0 for the first row).
   * for continued cell frames the row index is that of the cell's first-in-flow
   * and the column index (starting at 0 for the first column
   */
  NS_IMETHOD GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex);

  /** return the mapped cell's row index (starting at 0 for the first row) */
  virtual nsresult GetRowIndex(PRInt32 &aRowIndex) const;

  /** return the previous cell having the same column index as current cell
    * returns null if no cell is present (but nsresult is still NS_OK)
    * (When used within layout, you can QI aCellLayout to get an nsIFrame*)
    */
  NS_IMETHOD GetPreviousCellInColumn(nsITableCellLayout **aCellLayout);

  /** return the next cell having the same column index
    * returns null if no cell is present (but nsresult is still NS_OK)
    * (When used within layout, you can QI aCellLayout to get an nsIFrame*)
    */
  NS_IMETHOD GetNextCellInColumn(nsITableCellLayout **aCellLayout);

  /**
   * return the cell's specified col span. this is what was specified in the
   * content model or in the style info, and is always >= 1.
   * to get the effective col span (the actual value that applies), use GetEffectiveColSpan()
   * @see nsTableFrame::GetEffectiveColSpan()
   */
  virtual PRInt32 GetColSpan();
  
  /** return the cell's column index (starting at 0 for the first column) */
  virtual nsresult GetColIndex(PRInt32 &aColIndex) const;
  virtual nsresult SetColIndex(PRInt32 aColIndex);

  /** return the available width given to this frame during its last reflow */
  virtual nscoord GetPriorAvailWidth();
  
  /** set the available width given to this frame during its last reflow */
  virtual void SetPriorAvailWidth(nscoord aPriorAvailWidth);

  /** return the desired size returned by this frame during its last reflow */
  virtual nsSize GetDesiredSize();
  virtual nscoord GetDesiredAscent();

  /** set the desired size returned by this frame during its last reflow */
  virtual void SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize);

  /** return the maximum width of the cell */
  virtual nscoord GetMaximumWidth() const;

  /** set the maximum width of the cell */
  virtual void SetMaximumWidth(nscoord aMaximumWidth);

  /** return the MaxElement size returned by this frame during its last reflow 
    * not counting reflows where MaxElementSize is not requested.  
    * That is, the cell frame will always remember the last non-null MaxElementSize
    */
  virtual nsSize GetPass1MaxElementSize() const;

  /** set the MaxElement size returned by this frame during its last reflow.
    * should never be called with a null MaxElementSize
    */
  virtual void SetPass1MaxElementSize(const nsSize & aMaxElementSize);

  PRBool GetContentEmpty();
  void SetContentEmpty(PRBool aContentEmpty);

  // The collapse offset is (0,0) except for cells originating in a row/col which is collapsed
  void    SetCollapseOffsetX(nsIPresContext* aPresContext, nscoord aXOffset);
  void    SetCollapseOffsetY(nsIPresContext* aPresContext, nscoord aYOffset);
  void    GetCollapseOffset(nsIPresContext* aPresContext, nsPoint& aOffset);

protected:
  /** implement abstract method on nsHTMLContainerFrame */
  virtual PRIntn GetSkipSides() const;

  virtual PRBool ParentDisablesSelection() const; //override default behavior

private:  

  // All these methods are support methods for RecalcLayoutData
  nsIFrame* GetFrameAt(nsVoidArray* aList,  PRInt32 aIndex);

  //XXX: aTableFrame can be removed as soon as border-collapse inherits correctly
  void GetCellBorder(nsMargin &aBorder, nsTableFrame *aTableFrame);

  PRUint8 GetOpposingEdge(PRUint8 aEdge);

protected:

  // Subclass hook for style post processing
  NS_IMETHOD DidSetStyleContext(nsIPresContext* aPresContext);

  void      MapBorderPadding(nsIPresContext* aPresContext);

  void      MapHTMLBorderStyle(nsIPresContext* aPresContext,
                               nsStyleSpacing& aSpacingStyle,
                               nsTableFrame*   aTableFrame);

  void      MapVAlignAttribute(nsIPresContext* aPresContext, nsTableFrame *aTableFrame);
  void      MapHAlignAttribute(nsIPresContext* aPresContext, nsTableFrame *aTableFrame);

  PRBool    ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult);

protected:

  /** the starting column for this cell */
  int          mColIndex;

  /** the available width we were given in our previous reflow */
  nscoord      mPriorAvailWidth;

  /** these are the last computed desired and max element sizes */
  nsSize       mDesiredSize;
  nscoord      mDesiredAscent;

  /** these are the Pass 1 maximum width and max element sizes */
  nscoord      mMaximumWidth;
  nsSize       mPass1MaxElementSize;

public:
  nsBorderEdges *mBorderEdges;      // one list of border segments for each side of the table frame
                                    // used only for the collapsing border model

};

inline nsresult nsTableCellFrame::GetRowIndex(PRInt32 &aRowIndex) const
{
  nsTableCellFrame* cell = (nsTableCellFrame*)GetFirstInFlow();
  nsresult result;
  nsTableRowFrame * row;
  cell->GetParent((nsIFrame **)&row);
  if (nsnull!=row)
  {
    aRowIndex = row->GetRowIndex();
    result = NS_OK;
  }
  else
  {
    aRowIndex = 0;
    result = NS_ERROR_NOT_INITIALIZED;
  }
  return result;
}

inline nsresult nsTableCellFrame::GetColIndex(PRInt32 &aColIndex) const
{  
  aColIndex = mColIndex;
  return  NS_OK;
}

inline nscoord nsTableCellFrame::GetPriorAvailWidth()
{ return mPriorAvailWidth;}

inline void nsTableCellFrame::SetPriorAvailWidth(nscoord aPriorAvailWidth)
{ mPriorAvailWidth = aPriorAvailWidth;}

inline nsSize nsTableCellFrame::GetDesiredSize()
{ return mDesiredSize; }

inline nscoord nsTableCellFrame::GetDesiredAscent()
{ return mDesiredAscent; }

inline void nsTableCellFrame::SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize)
{ 
  mDesiredSize.width = aDesiredSize.width;
  mDesiredSize.height = aDesiredSize.height;
  mDesiredAscent = aDesiredSize.ascent;
}

inline nscoord nsTableCellFrame::GetMaximumWidth() const
{ return mMaximumWidth; }

inline void nsTableCellFrame::SetMaximumWidth(nscoord aMaximumWidth)
{ 
  mMaximumWidth = aMaximumWidth;
}

inline nsSize nsTableCellFrame::GetPass1MaxElementSize() const
{ return mPass1MaxElementSize; }

inline PRBool nsTableCellFrame::GetContentEmpty()
{
  return (mState & NS_TABLE_CELL_FRAME_CONTENT_EMPTY) ==
         NS_TABLE_CELL_FRAME_CONTENT_EMPTY;
}

inline void nsTableCellFrame::SetContentEmpty(PRBool aContentEmpty)
{
  if (aContentEmpty) {
    mState |= NS_TABLE_CELL_FRAME_CONTENT_EMPTY;
  } else {
    mState &= ~NS_TABLE_CELL_FRAME_CONTENT_EMPTY;
  }
}

#endif



