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
#include "nsContainerFrame.h"

struct RowGroupReflowState;
struct nsStyleMolecule;

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
class nsTableRowGroupFrame : public nsContainerFrame
{
public:

  /** instantiate a new instance of nsTableRowGroupFrame.
    * @param aInstancePtrResult  the new object is returned in this out-param
    * @param aContent            the table object to map
    * @param aIndexInParent      which child is the new frame?
    * @param aParent             the parent of the new frame
    *
    * @return  NS_OK if the frame was properly allocated, otherwise an error code
    */
  static nsresult NewFrame(nsIFrame** aInstancePtrResult,
                           nsIContent* aContent,
                           PRInt32     aIndexInParent,
                           nsIFrame*   aParent);

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

  /** calls ResizeReflow for all of its child rows.
    * Rows are all set to the same width and stacked vertically.
    * <P> rows are not split unless absolutely necessary.
    *
    * @param aDesiredSize width set to width of rows, height set to 
    *                     sum of height of rows that fit in aMaxSize.height.
    *
    * @see nsIFrame::ResizeReflow
    */
  NS_IMETHOD ResizeReflow(nsIPresContext* aPresContext,
                          nsReflowMetrics& aDesiredSize,
                          const nsSize&   aMaxSize,
                          nsSize*         aMaxElementSize,
                          ReflowStatus&   aStatus);
  /** @see nsIFrame::IncrementalReflow */
  NS_IMETHOD IncrementalReflow(nsIPresContext*  aPresContext,
                               nsReflowMetrics& aDesiredSize,
                               const nsSize&    aMaxSize,
                               nsReflowCommand& aReflowCommand,
                               ReflowStatus&    aStatus);

  /** @see nsContainerFrame::CreateContinuingFrame */
  NS_IMETHOD CreateContinuingFrame(nsIPresContext* aPresContext,
                                   nsIFrame*       aParent,
                                   nsIFrame*&      aContinuingFrame);

protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableRowGroupFrame(nsIContent* aContent,
                       PRInt32 aIndexInParent,
					             nsIFrame* aParentFrame);

  /** protected destructor */
  ~nsTableRowGroupFrame();

  nscoord GetTopMarginFor(nsIPresContext*      aCX,
                          RowGroupReflowState& aState,
                          nsStyleMolecule*     aKidMol);

  void          PlaceChild( nsIPresContext*      aPresContext,
                            RowGroupReflowState& aState,
                            nsIFrame*            aKidFrame,
                            const nsRect&        aKidRect,
                            nsSize*              aMaxElementSize,
                            nsSize&              aKidMaxElementSize);

  /**
   * Reflow the frames we've already created
   *
   * @param   aPresContext presentation context to use
   * @param   aState current inline state
   * @return  true if we successfully reflowed all the mapped children and false
   *            otherwise, e.g. we pushed children to the next in flow
   */
  PRBool        ReflowMappedChildren(nsIPresContext*      aPresContext,
                                     RowGroupReflowState& aState,
                                     nsSize*              aMaxElementSize);

  /**
   * Try and pull-up frames from our next-in-flow
   *
   * @param   aPresContext presentation context to use
   * @param   aState current inline state
   * @return  true if we successfully pulled-up all the children and false
   *            otherwise, e.g. child didn't fit
   */
  PRBool        PullUpChildren(nsIPresContext*      aPresContext,
                               RowGroupReflowState& aState,
                               nsSize*              aMaxElementSize);

  /**
   * Create new frames for content we haven't yet mapped
   *
   * @param   aPresContext presentation context to use
   * @param   aState current inline state
   * @return  frComplete if all content has been mapped and frNotComplete
   *            if we should be continued
   */
  ReflowStatus  ReflowUnmappedChildren(nsIPresContext*      aPresContext,
                                       RowGroupReflowState& aState,
                                       nsSize*              aMaxElementSize);

};


#endif
