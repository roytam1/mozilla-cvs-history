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
#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTableCell.h"

struct nsStyleMolecule;

/**
 * nsTableCellFrame
 * data structure to maintain information about a single table cell's frame
 *
 * @author  sclark
 */
class nsTableCellFrame : public nsContainerFrame
{
public:
  static nsresult NewFrame(nsIFrame** aInstancePtrResult,
                           nsIContent* aContent,
                           PRInt32     aIndexInParent,
                           nsIFrame*   aParent);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD ResizeReflow(nsIPresContext* aPresContext,
                          nsReflowMetrics& aDesiredSize,
                          const nsSize&   aMaxSize,
                          nsSize*         aMaxElementSize,
                          ReflowStatus&   aStatus);

  NS_IMETHOD IncrementalReflow(nsIPresContext*  aPresContext,
                               nsReflowMetrics& aDesiredSize,
                               const nsSize&    aMaxSize,
                               nsReflowCommand& aReflowCommand,
                               ReflowStatus&    aStatus);

  /**
   * @see nsContainerFrame
   */
  NS_IMETHOD CreateContinuingFrame(nsIPresContext* aPresContext,
                                   nsIFrame*       aParent,
                                   nsIFrame*&      aContinuingFrame);

  void          VerticallyAlignChild(nsIPresContext* aPresContext);

  virtual PRInt32 GetRowSpan();

  virtual ~nsTableCellFrame();

protected:

  /** protected constructor.
    * @see NewFrame
    */
  nsTableCellFrame(nsIContent* aContent,
                   PRInt32 aIndexInParent,
					         nsIFrame* aParentFrame);

  /** Create a psuedo-frame for this caption.  Handles continuing frames as needed.
    */
  virtual void CreatePsuedoFrame(nsIPresContext* aPresContext);


};


#endif
