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
#ifndef nsTableFrame_h__
#define nsTableFrame_h__

#include "nscore.h"
#include "nsVoidArray.h"
#include "nsHTMLContainerFrame.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsIStyleContext.h"
#include "nsITableLayout.h"

class nsCellMap;
class nsTableCellFrame;
class nsTableColFrame;
class nsTableRowGroupFrame;
class nsTableRowFrame;
class nsTableColGroupFrame;
class nsTableBorderCollapser;
class nsITableLayoutStrategy;
class nsHTMLValue;

struct InnerTableReflowState;
struct nsStylePosition;
struct nsStyleSpacing;

/**
 * Child list name indices
 * @see #GetAdditionalChildListName()
 */
#define NS_TABLE_FRAME_COLGROUP_LIST_INDEX 0
#define NS_TABLE_FRAME_LAST_LIST_INDEX    NS_TABLE_FRAME_COLGROUP_LIST_INDEX

struct nsDebugTable
{
  static PRBool gRflTableOuter;
  static PRBool gRflTable;
  static PRBool gRflRowGrp;
  static PRBool gRflRow;
  static PRBool gRflCell;
  static PRBool gRflArea;
};
/* ============================================================================ */

/** nsTableFrame maps the inner portion of a table (everything except captions.)
  * Used as a pseudo-frame within nsTableOuterFrame, it may also be used
  * stand-alone as the top-level frame.
  *
  * The flowed child list contains row group frames. There is also an additional
  * named child list:
  * - "ColGroup-list" which contains the col group frames
  *
  * @see nsLayoutAtoms::colGroupList
  *
  * TODO: make methods virtual so nsTableFrame can be used as a base class in the future.
  */
class nsTableFrame : public nsHTMLContainerFrame, public nsITableLayout
{
public:

  // nsISupports
  NS_DECL_ISUPPORTS_INHERITED

  /** nsTableOuterFrame has intimate knowledge of the inner table frame */
  friend class nsTableOuterFrame;

  /** instantiate a new instance of nsTableFrame.
    * @param aResult    the new object is returned in this out-param
    * @param aContent   the table object to map
    * @param aParent    the parent of the new frame
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  friend nsresult 
  NS_NewTableFrame(nsIFrame** aResult);

  /** sets defaults for table-specific style.
    * @see nsIFrame::Init 
    */
  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);


  /** @see nsIFrame::Destroy */
  NS_IMETHOD Destroy(nsIPresContext& aPresContext);

  NS_IMETHOD AppendFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIPresContext& aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  /** helper method to find the table parent of any table frame object */
  // TODO: today, this depends on display types.  This should be changed to rely
  // on stronger criteria, like an inner table frame atom
  static NS_METHOD GetTableFrame(nsIFrame *aSourceFrame, nsTableFrame *& aTableFrame);

  /**
    * @param aReflowState  the context within which we're to determine the table width info
    * @param aSpecifiedTableWidth [OUT] if the table is not auto-width,
    *                                   aSpecifiedTableWidth iw set to the resolved width.
    * @return PR_TRUE if the table is auto-width.  value of aSpecifiedTableWidth is undefined.
    *         PR_FALSE if the table is not auto-width, 
    *         and aSpecifiedTableWidth is set to the resolved width in twips.
    */
  PRBool IsAutoWidth(const nsHTMLReflowState& aReflowState,
                     nscoord&                 aSpecifiedTableWidth);
  
  /** @return PR_TRUE if aDisplayType represents a rowgroup of any sort
    * (header, footer, or body)
    */
  PRBool IsRowGroup(PRInt32 aDisplayType) const;

  /** Initialize the table frame with a set of children.
    * Calls DidAppendRowGroup which calls nsTableRowFrame::InitChildren
    * which finally calls back into this table frame to build the cell map.
    * Also ensures we have the right number of column frames for the child list.
    * 
    * @see nsIFrame::SetInitialChildList 
    */
  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  /** return the first child belonging to the list aListName. 
    * @see nsIFrame::FirstChild
    */
  NS_IMETHOD FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const;

  /** @see nsIFrame::GetAdditionalChildListName */
  NS_IMETHOD  GetAdditionalChildListName(PRInt32   aIndex,
                                         nsIAtom** aListName) const;

  /** complete the append of aRowGroupFrame to the table
    * this builds the cell map by calling nsTableRowFrame::InitChildren
    * which calls back into this table frame to build the cell map.
    * @param aRowGroupFrame the row group that was appended.
    * note that this method is optimized for content appended, and doesn't
    * work for random insertion of row groups.  Random insertion must go
    * through incremental reflow notifications.
    */
  NS_IMETHOD DidAppendRowGroup(nsTableRowGroupFrame *aRowGroupFrame);

  /** @see nsIFrame::Paint */
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  /** nsIFrame method overridden to handle table specifics
  */
  NS_IMETHOD SetSelected(nsIPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);

  /** inner tables are reflowed in two steps.
    * <pre>
    * if mFirstPassValid is false, this is our first time through since content was last changed
    *   set pass to 1
    *   do pass 1
    *     get min/max info for all cells in an infinite space
    *   do column balancing
    *   set mFirstPassValid to true
    *   do pass 2
    *     use column widths to Reflow cells
    * </pre>
    *
    * @see ResizeReflowPass1
    * @see ResizeReflowPass2
    * @see BalanceColumnWidths
    * @see nsIFrameReflow::Reflow
    */
  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);



  /** the COLS attribute can be modified by any cell's width attribute.
    * deal with it here.  Must be called before any call to 
    * ColumnInfoCache::AddColumnInfo
    */
  virtual void AdjustColumnsForCOLSAttribute();

  /** return the column frame corresponding to the given column index
    * there are two ways to do this, depending on whether we have cached
    * column information yet.
    */
  NS_METHOD GetColumnFrame(PRInt32 aColIndex, nsTableColFrame *&aColFrame);

  /** return PR_TRUE if the column width information has been set */
  PRBool IsColumnWidthsSet();

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::tableFrame
   */
  NS_IMETHOD  GetFrameType(nsIAtom** aType) const;

  /** @see nsIFrame::GetFrameName */
  NS_IMETHOD GetFrameName(nsString& aResult) const;

#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const;
#endif

  /** get the max border thickness for each edge */
  void GetTableBorder(nsMargin &aBorder);

  void SetBorderEdgeLength(PRUint8 aSide, 
                           PRInt32 aIndex, 
                           nscoord aLength);

  /** get the border values for the row and column */
  void GetTableBorderAt(PRInt32  aRowIndex, 
                        PRInt32  aColIndex,
                        nsMargin &aBorder);

  /** get the max border thickness for each edge encompassed by the row group */
  void GetTableBorderForRowGroup(nsTableRowGroupFrame * aRowGroupFrame, nsMargin &aBorder);

  /** return the width of the column at aColIndex    */
  virtual PRInt32 GetColumnWidth(PRInt32 aColIndex);

  /** set the width of the column at aColIndex to aWidth    */
  virtual void SetColumnWidth(PRInt32 aColIndex, nscoord aWidth);

  /** helper to get the border collapse style value */
  virtual PRUint8 GetBorderCollapseStyle();

  /** helper to get the cell spacing X style value */
  virtual nscoord GetCellSpacingX();

  /** helper to get the cell spacing Y style value */
  virtual nscoord GetCellSpacingY();

  /** helper to get the cell padding style value */
  virtual nscoord GetCellPadding();
          

  // Get cell margin information
  NS_IMETHOD GetCellMarginData(nsTableCellFrame* aKidFrame, nsMargin& aMargin);

  /** return the row span of a cell, taking into account row span magic at the bottom
    * of a table.
    *
    * @param aRowIndex  the row from which to measure effective row span
    * @param aCell      the cell
    *
    * @return  the row span, correcting for row spans that extend beyond the bottom
    *          of the table.
    */
  virtual PRInt32  GetEffectiveRowSpan(PRInt32 aRowIndex, nsTableCellFrame *aCell);
  virtual PRInt32  GetEffectiveRowSpan(nsTableCellFrame *aCell);

  /** return the col span of a cell, taking into account col span magic at the edge
    * of a table.
    *
    * @param aColIndex  the column from which to measure effective col span
    * @param aCell      the cell
    *
    * @return  the col span, correcting for col spans that extend beyond the edge
    *          of the table.
    */
  virtual PRInt32  GetEffectiveColSpan(PRInt32 aColIndex, const nsTableCellFrame *aCell) const;
  virtual PRInt32  GetEffectiveColSpan(const nsTableCellFrame *aCell) const;

  /** return the value of the COLS attribute, adjusted for the 
    * actual number of columns in the table
    */
  PRInt32 GetEffectiveCOLSAttribute();

  /** return the index of the next row that is not yet assigned.
    * If no row is initialized, 0 is returned.
    */
  PRInt32 GetNextAvailRowIndex() const;

  /** build as much of the CellMap as possible from the info we have so far 
    */
  virtual PRInt32 AddCellToTable (nsTableCellFrame* aCellFrame,
                                  PRInt32           aRowIndex);
  virtual void RemoveCellFromTable (nsTableCellFrame* aCellFrame,
                                    PRInt32           aRowIndex);

  virtual void AddColumnFrame (nsTableColFrame *aColFrame);

  static PRBool IsFinalPass(const nsHTMLReflowState& aReflowState);

  nsTableCellFrame* GetCellInfoAt(PRInt32            aRowX, 
                                  PRInt32            aColX, 
                                  PRBool*            aOriginates = nsnull, 
                                  PRInt32*           aColSpan = nsnull);
  PRInt32 GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const;
  PRInt32 GetNumCellsOriginatingInCol(PRInt32 aColIndex) const;

  NS_METHOD GetBorderPlusMarginPadding(nsMargin& aResult);

  PRBool HasNonPercentSpanningPercent() const;
  void SetHasNonPercentSpanningPercent(PRBool aValue);

  static void DebugReflow(char*                     aMessage,
                         const nsIFrame*            aFrame,
                         const nsHTMLReflowState*   aState, 
                         const nsHTMLReflowMetrics* aMetrics);

  static void DebugGetIndent(const nsIFrame* aFrame, 
                             char*           aBuf);

protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableFrame();

  /** destructor, responsible for mColumnLayoutData and mColumnWidths */
  virtual ~nsTableFrame();

  /** implement abstract method on nsHTMLContainerFrame */
  virtual PRIntn GetSkipSides() const;

  virtual PRBool ParentDisablesSelection() const; //override default behavior

  /** helper method for determining if this is a nested table or not 
    * @param aReflowState The reflow state for this inner table frame
    * @param aPosition    [OUT] The position style struct for the parent table, if nested.
    *                     If not nested, undefined.
    * @return  PR_TRUE if this table is nested inside another table.
    */
  PRBool IsNested(const nsHTMLReflowState& aReflowState, const nsStylePosition *& aPosition) const;
  
  // Sets the starting column index for aColGroupFrame and the siblings frames that
  // follow
  void SetStartingColumnIndexFor(nsTableColGroupFrame* aColGroupFrame,
                                 PRInt32 aIndex);

  // Calculate the starting column index to use for the specified col group frame
  PRInt32 CalculateStartingColumnIndexFor(nsTableColGroupFrame* aColGroupFrame);

public:
  /** first pass of ResizeReflow.  
    * lays out all table content with aMaxSize(NS_UNCONSTRAINEDSIZE,NS_UNCONSTRAINEDSIZE) and
    * a non-null aMaxElementSize so we get all the metrics we need to do column balancing.
    * Pass 1 only needs to be executed once no matter how many times the table is resized, 
    * as long as content and style don't change.  This is managed in the member variable mFirstPassIsValid.
    * The layout information for each cell is cached in mColumLayoutData.
    * Incremental layout can take advantage of aStartingFrame to pick up where a previous
    * ResizeReflowPass1 left off.
    *
    * @see nsIFrameReflow::Reflow
    */
  NS_IMETHOD ResizeReflowPass1(nsIPresContext&          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus,
                               nsTableRowGroupFrame *   aStartingFrame,
                               nsReflowReason           aReason,
                               PRBool                   aDoSiblings);

  NS_IMETHOD GetTableSpecifiedHeight(nscoord& aHeight, const nsHTMLReflowState& aReflowState);
  virtual PRBool RowGroupsShouldBeConstrained() { return PR_FALSE; }
  
  /** do I need to do a reflow? */
  virtual PRBool NeedsReflow(const nsHTMLReflowState& aReflowState);
  
protected:
  /** second pass of ResizeReflow.
    * lays out all table content with aMaxSize(computed_table_width, given_table_height) 
    * Pass 2 is executed every time the table needs to resize.  An optimization is included
    * so that if the table doesn't need to actually be resized, no work is done (see NeedsReflow).
    * 
    * @see nsIFrameReflow::Reflow
    */
  NS_IMETHOD ResizeReflowPass2(nsIPresContext&          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus);

// begin incremental reflow methods
  
  /** Incremental Reflow attempts to do column balancing with the minimum number of reflow
    * commands to child elements.  This is done by processing the reflow command,
    * rebalancing column widths (if necessary), then comparing the resulting column widths
    * to the prior column widths and reflowing only those cells that require a reflow.
    * All incremental reflows go through this method.
    *
    * @see Reflow
    */
  NS_IMETHOD IncrementalReflow(nsIPresContext&          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus);

  /** process an incremental reflow command targeted at a child of this frame. 
    * @param aNextFrame  the next frame in the reflow target chain
    * @see nsIFrameReflow::Reflow
    */
  NS_IMETHOD IR_TargetIsChild(nsIPresContext&        aPresContext,
                              nsHTMLReflowMetrics&   aDesiredSize,
                              InnerTableReflowState& aReflowState,
                              nsReflowStatus&        aStatus,
                              nsIFrame *             aNextFrame);

  /** process an incremental reflow command targeted at this frame. 
    * @see nsIFrameReflow::Reflow
    */
  NS_IMETHOD IR_TargetIsMe(nsIPresContext&        aPresContext,
                           nsHTMLReflowMetrics&   aDesiredSize,
                           InnerTableReflowState& aReflowState,
                           nsReflowStatus&        aStatus);

  /** process a style chnaged notification.
    * @see nsIFrameReflow::Reflow
    * TODO: needs to be optimized for which attribute was actually changed.
    */
  NS_IMETHOD IR_StyleChanged(nsIPresContext&        aPresContext,
                             nsHTMLReflowMetrics&   aDesiredSize,
                             InnerTableReflowState& aReflowState,
                             nsReflowStatus&        aStatus);
  
  NS_IMETHOD AdjustSiblingsAfterReflow(nsIPresContext&        aPresContext,
                                       InnerTableReflowState& aReflowState,
                                       nsIFrame*              aKidFrame,
                                       nsSize*                aMaxElementSize,
                                       nscoord                aDeltaY);
  
  nsresult RecoverState(InnerTableReflowState& aReflowState,
                        nsIFrame*              aKidFrame,
                        nsSize*                aMaxElementSize);

  NS_METHOD AdjustForCollapsingRowGroup(nsIFrame* aRowGroupFrame, 
                                        PRInt32& aRowX);

  NS_METHOD CollapseRowGroup(nsIPresContext* aPresContext,
                             nsIFrame* aRowGroupFrame,
                             const nscoord& aYTotalOffset,
                             nscoord& aYGroupOffset, PRInt32& aRowX);

  NS_METHOD AdjustForCollapsingRows(nsIPresContext& aPresContext, 
                                    nscoord&        aHeight);

  NS_METHOD AdjustForCollapsingCols(nsIPresContext& aPresContext, 
                                    nscoord&        aWidth);
// end incremental reflow methods

  /** return the desired width of this table accounting for the current
    * reflow state, and for the table attributes and parent
    */
  nscoord ComputeDesiredWidth(const nsHTMLReflowState& aReflowState) const;

  /** return the desired height of this table accounting for the current
    * reflow state, and for the table attributes and parent
    */
  nscoord ComputeDesiredHeight(nsIPresContext&          aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nscoord                  aDefaultHeight);

  /** The following two functions are helpers for ComputeDesiredHeight 
    */
  void DistributeSpaceToCells(nsIPresContext& aPresContext, 
                              const nsHTMLReflowState& aReflowState,
                              nsIFrame* aRowGroupFrame);
  void DistributeSpaceToRows(nsIPresContext& aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsIFrame* aRowGroupFrame, const nscoord& aSumOfRowHeights,
                                  const nscoord& aExcess, const nsStyleTable* aTableStyle, 
                                  nscoord& aExcessForRowGroup, 
                                  nscoord& aRowGroupYPos);

  nscoord GetEffectiveContainerHeight(const nsHTMLReflowState& aReflowState);

  void PlaceChild(nsIPresContext&        aPresContext,
                  InnerTableReflowState& aReflowState,
                  nsIFrame*              aKidFrame,
                  const nsRect&          aKidRect,
                  nsSize*                aMaxElementSize,
                  nsSize&                aKidMaxElementSize);

  /**
   * Reflow the frames we've already created
   *
   * @param   aPresContext presentation context to use
   * @param   aReflowState current inline state
   * @return  true if we successfully reflowed all the mapped children and false
   *            otherwise, e.g. we pushed children to the next in flow
   */
  NS_IMETHOD ReflowMappedChildren(nsIPresContext&        aPresContext,
                                  nsHTMLReflowMetrics&   aDesiredSize,
                                  InnerTableReflowState& aReflowState,
                                  nsReflowStatus&        aStatus);
  /**
   * Try and pull-up frames from our next-in-flow
   *
   * @param   aPresContext presentation context to use
   * @param   aReflowState current reflow state
   * @return  true if we successfully pulled-up all the children and false
   *            otherwise, e.g. child didn't fit
   */
  NS_IMETHOD PullUpChildren(nsIPresContext&        aPresContext,
                            nsHTMLReflowMetrics&   aDesiredSize,
                            InnerTableReflowState& aReflowState,
                            nsReflowStatus&        aStatus);

  /** assign widths for each column, taking into account the table content, the effective style, 
    * the layout constraints, and the compatibility mode.  Sets mColumnWidths as a side effect.
    * @param aPresContext     the presentation context
    * @param aTableStyle      the resolved style for the table
    * @param aMaxSize         the height and width constraints
    * @param aMaxElementSize  the min size of the largest indivisible object
    */
  virtual void BalanceColumnWidths(nsIPresContext&          aPresContext, 
                                   const nsHTMLReflowState& aReflowState,
                                   const nsSize&            aMaxSize, 
                                   nsSize*                  aMaxElementSize);

  /** sets the width of the table according to the computed widths of each column. */
  virtual void SetTableWidth(nsIPresContext&  aPresContext);

  /** returns PR_TRUE if the cached pass 1 data is still valid */
  virtual PRBool IsFirstPassValid() const;

  /** returns PR_TRUE if the cached column info is still valid */
  virtual PRBool IsColumnCacheValid() const;

  /** returns PR_TRUE if the cached column info is still valid */
  virtual PRBool IsColumnWidthsValid() const;

  nsIFrame* GetFirstBodyRowGroupFrame();
  PRBool MoveOverflowToChildList(nsIPresContext* aPresContext);
  void PushChildren(nsIPresContext *aPresContext,
                    nsIFrame*       aFromChild,
                    nsIFrame*       aPrevSibling);

public:
  // Returns PR_TRUE if there are any cells above the row at
  // aRowIndex and spanning into the row at aRowIndex     
  PRBool RowIsSpannedInto(PRInt32 aRowIndex);

  // Returns PR_TRUE if there is a cell originating in aRowIndex
  // which spans into the next row
  PRBool RowHasSpanningCells(PRInt32 aRowIndex);

  // Returns PR_TRUE if there are any cells to the left of the column at
  // aColIndex and spanning into the column at aColIndex     
  PRBool ColIsSpannedInto(PRInt32 aColIndex);

  // Returns PR_TRUE if there is a cell originating in aColIndex
  // which spans into the next col
  PRBool ColHasSpanningCells(PRInt32 aColIndex);

  // Returns PR_TRUE if the style width change, aPrevStyleWidth, for aCellFrame 
  // could require the columns to be rebalanced. This method can be used to 
  // determine if an incremental reflow on aCellFrame is necessary as the result
  // of the style width change. If aConsiderMinWidth is PR_TRUE, then potential
  // changes to aCellFrame's min width is considered (however, if considered, 
  // the function will always return PR_TRUE if the layout strategy is Basic).
  PRBool ColumnsCanBeInvalidatedBy(nsStyleCoord*           aPrevStyleWidth,
                                   const nsTableCellFrame& aCellFrame) const;

  // Returns PR_TRUE if potential width changes to aCellFrame could require the 
  // columns to be rebalanced. This method can be used after an incremental reflow
  // of aCellFrame to determine if a pass1 reflow on aCellFrame is necessary. If 
  // aConsiderMinWidth is PR_TRUE, then potential changes to aCellFrame's min width 
  // is considered (however, if considered, the function will always return PR_TRUE 
  // if the layout strategy is Basic).
  PRBool ColumnsCanBeInvalidatedBy(const nsTableCellFrame& aCellFrame,
                                   PRBool                  aConsiderMinWidth = PR_FALSE) const;

  // Returns PR_TRUE if changes to aCellFrame's pass1 min and desired (max) sizes
  // don't require the columns to be rebalanced. This method can be used after a 
  // pass1 reflow of aCellFrame to determine if the columns need rebalancing. 
  // aPrevCellMin and aPrevCellDes are the values aCellFrame had before the last 
  // pass1 reflow.
  PRBool ColumnsAreValidFor(const nsTableCellFrame& aCellFrame,
                            nscoord                 aPrevCellMin,
                            nscoord                 aPrevCellDes) const;
  
  virtual void InvalidateFirstPassCache();

  virtual void InvalidateColumnCache();

  virtual void InvalidateColumnWidths();

protected:

  /** Support methods for DidSetStyleContext */
  void      MapBorderMarginPadding(nsIPresContext& aPresContext);
  void      MapHTMLBorderStyle(nsStyleSpacing& aSpacingStyle, nscoord aBorderWidth);
  PRBool    ConvertToPixelValue(nsHTMLValue& aValue, PRInt32 aDefault, PRInt32& aResult);

  /** returns PR_TRUE if the cached pass 1 data is still valid */
  virtual PRBool IsCellMapValid() const;

public:
  /** Get the cell map for this table frame.  It is not always mCellMap.
    * Only the firstInFlow has a legit cell map
    */
  virtual nsCellMap *GetCellMap() const;

  /** ResetCellMap is called when the cell structure of the table is changed.
    * Call with caution, only when changing the structure of the table such as 
    * inserting or removing rows, changing the rowspan or colspan attribute of a cell, etc.
    */
  virtual void InvalidateCellMap();
    
  /** sum the columns represented by all nsTableColGroup objects. 
    * if the cell map says there are more columns than this, 
    * add extra implicit columns to the content tree.
    *
    * returns whether any implicit column frames were created
    */ 
  virtual void EnsureColumns (nsIPresContext& aPresContext,
                              PRBool&         aCreatedColFrames);

  // These methods are used to incrementally insert and remove rows
  // from the cell map without having to invalidate the entire map.
  NS_IMETHOD InsertRowIntoMap(nsTableRowFrame* aRow, PRInt32 aRowIndex);
  NS_IMETHOD RemoveRowFromMap(nsTableRowFrame* aRow, PRInt32 aRowIndex);

  NS_IMETHOD AdjustRowIndices(nsIFrame* aRowGroup, 
                                             PRInt32 aRowIndex,
                                             PRInt32 anAdjustment);

  // Return PR_TRUE if rules=groups is set for the table content 
  PRBool HasGroupRules() const;

  // Remove cell borders which aren't bordering row and/or col groups 
  void ProcessGroupRules(nsIPresContext& aPresContext);

protected:
  /** iterates all child frames and creates a new cell map */
  NS_IMETHOD ReBuildCellMap();

  void SetColumnDimensions(nsIPresContext* aPresContext, nscoord aHeight);

#ifdef NS_DEBUG
  /** for debugging only
    * prints out info about the table layout state, printing columns and their cells
    */
  void ListColumnLayoutData(FILE* out, PRInt32 aIndent);
#endif

  /** return the number of columns as specified by the input. 
    * has 2 side effects:<br>
    * calls SetStartColumnIndex on each nsTableColumn<br>
    * sets mSpecifiedColCount.<br>
    */
  virtual PRInt32 GetSpecifiedColumnCount ();

	/** 
	  * Return aFrame's child if aFrame is an nsScrollFrame, otherwise return aFrame
	  */
  nsTableRowGroupFrame* GetRowGroupFrameFor(nsIFrame* aFrame, const nsStyleDisplay* aDisplay);

public: /* ----- Cell Map public methods ----- */

  /** returns the number of rows in this table.
    * if mCellMap has been created, it is asked for the number of rows.<br>
    * otherwise, the content is enumerated and the rows are counted.
    */
  virtual PRInt32 GetRowCount() const;

  /** returns the number of columns in this table after redundant columns have been removed 
    */
  virtual PRInt32 GetColCount();

  /** return the column frame at colIndex.
    * returns nsnull if the col frame has not yet been allocated, or if aColIndex is out of range
    */
  nsTableColFrame * GetColFrame(PRInt32 aColIndex);

  /** return the cell frame at aRowIndex, aColIndex.
    * returns nsnull if the cell frame has not yet been allocated, 
    * or if aRowIndex or aColIndex is out of range
    */
  nsTableCellFrame * GetCellFrameAt(PRInt32 aRowIndex, PRInt32 aColIndex);

  /** return the minimum width of the table caption.  Return 0 if there is no caption. */
  nscoord GetMinCaptionWidth();

  /** return the minimum contend width of the table (excludes borders and padding).  
      Return 0 if the min width is unknown. */
  nscoord GetMinTableContentWidth();

  /** return the maximum content width of the table (excludes borders and padding). 
      Return 0 if the max width is unknown. */
  nscoord GetMaxTableContentWidth();

  /** compute the max-element-size for the table
    * @param aMaxElementSize  [OUT] width field set to the min legal width of the table
    */
  void SetMaxElementSize(nsSize* aMaxElementSize);

  /** returns PR_TRUE if table layout requires a preliminary pass over the content */
  PRBool RequiresPass1Layout();

  // compute the height of the table to be used as the basis for 
  // percentage height cells
  void ComputePercentBasisForRows(const nsHTMLReflowState& aReflowState);

  nscoord GetPercentBasisForRows();

  /*---------------- nsITableLayout methods ------------------------*/
  
  /** Get the cell and associated data for a table cell from the frame's cellmap */
  NS_IMETHOD GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex, 
                           nsIDOMElement* &aCell,   //out params
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                           PRInt32& aRowSpan, PRInt32& aColSpan,
                           PRBool& aIsSelected);

  /** Get the number of rows and column for a table from the frame's cellmap 
    *  Some rows may not have enough cells (the number returned is the maximum possible),
    *  which displays as a ragged-right edge table
    */
  NS_IMETHOD GetTableSize(PRInt32& aRowCount, PRInt32& aColCount);

  /*------------end of nsITableLayout methods -----------------------*/


  virtual void CacheColFrames(nsIPresContext& aPresContext,
                              PRBool          aReset = PR_FALSE);
public:
  static nsIAtom* gColGroupAtom;
  void Dump(PRBool aDumpCols, PRBool aDumpCellMap);

protected:
  void DebugPrintCount() const; // Debugging routine

// data members
  PRInt32     *mColumnWidths;       // widths of each column
  PRInt32      mColumnWidthsLength; // the number of column lengths this frame has allocated

  struct TableBits {
    unsigned mColumnWidthsSet:1;       // PR_TRUE if column widths have been set at least once
    unsigned mColumnWidthsValid:1;     // PR_TRUE if column width data is still legit, PR_FALSE if it needs to be recalculated
    unsigned mFirstPassValid:1;        // PR_TRUE if first pass data is still legit, PR_FALSE if it needs to be recalculated
    unsigned mColumnCacheValid:1;      // PR_TRUE if column cache info is still legit, PR_FALSE if it needs to be recalculated
    unsigned mCellMapValid:1;          // PR_TRUE if cell map data is still legit, PR_FALSE if it needs to be recalculated
    unsigned mIsInvariantWidth:1;      // PR_TRUE if table width cannot change
    unsigned mHasScrollableRowGroup:1; // PR_TRUE if any section has overflow == "auto" or "scroll"
    unsigned mNonPercentSpansPercent:1;
    int : 24;                          // unused
  } mBits;

  PRInt32      mColCount;           // the number of columns in this table
  nsCellMap*   mCellMap;            // maintains the relationships between rows, cols, and cells
  nsITableLayoutStrategy * mTableLayoutStrategy; // the layout strategy for this frame
  nsFrameList  mColGroups;          // the list of colgroup frames

  nsTableBorderCollapser* mBorderCollapser;      // one list of border segments for each side of the table frame
                                    // used only for the collapsing border model
  nscoord      mPercentBasisForRows;
};


inline PRBool nsTableFrame::IsRowGroup(PRInt32 aDisplayType) const
{
  return PRBool((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aDisplayType));
}

inline nscoord nsTableFrame::GetPercentBasisForRows()
{
  return mPercentBasisForRows;
}

inline PRBool nsTableFrame::HasNonPercentSpanningPercent() const
{
  return (PRBool)mBits.mNonPercentSpansPercent;
}

inline void nsTableFrame::SetHasNonPercentSpanningPercent(PRBool aValue)
{
  mBits.mNonPercentSpansPercent = (unsigned)aValue;
}

enum nsTableIteration {
  eTableLTR = 0,
  eTableRTL = 1,
  eTableDIR = 2
};

class nsTableIterator
{
public:
  nsTableIterator(nsIFrame&        aSource, 
                  nsTableIteration aType);
  nsTableIterator(nsFrameList&     aSource, 
                  nsTableIteration aType);
  nsIFrame* First();
  nsIFrame* Next();
  PRBool    IsLeftToRight();
  PRInt32   Count();

protected:
  void Init(nsIFrame*        aFirstChild,
            nsTableIteration aType);
  PRBool    mLeftToRight;
  nsIFrame* mFirstListChild;
  nsIFrame* mFirstChild;
  nsIFrame* mCurrentChild;
  PRInt32   mCount;
};

#endif































