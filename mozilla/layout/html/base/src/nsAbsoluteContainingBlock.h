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
#ifndef nsAbsoluteContainingBlock_h___
#define nsAbsoluteContainingBlock_h___

#include "nslayout.h"
#include "nsFrameList.h"

class nsIAtom;
class nsIFrame;
class nsIPresContext;

/**
 * This class contains the logic for being an absolute containing block.
 *
 * There is no principal child list, just a named child list which contains
 * the absolutely positioned frames
 *
 * All functions include as the first argument the frame that is delegating
 * the request
 *
 * @see nsLayoutAtoms::absoluteList
 */
class nsAbsoluteContainingBlock
{
public:
  nsresult FirstChild(const nsIFrame* aDelegatingFrame,
                      nsIAtom*        aListName,
                      nsIFrame**      aFirstChild) const;
  
  nsresult SetInitialChildList(nsIFrame*       aDelegatingFrame,
                               nsIPresContext* aPresContext,
                               nsIAtom*        aListName,
                               nsIFrame*       aChildList);
  nsresult AppendFrames(nsIFrame*       aDelegatingFrame,
                        nsIPresContext* aPresContext,
                        nsIPresShell&   aPresShell,
                        nsIAtom*        aListName,
                        nsIFrame*       aFrameList);
  nsresult InsertFrames(nsIFrame*       aDelegatingFrame,
                        nsIPresContext* aPresContext,
                        nsIPresShell&   aPresShell,
                        nsIAtom*        aListName,
                        nsIFrame*       aPrevFrame,
                        nsIFrame*       aFrameList);
  nsresult RemoveFrame(nsIFrame*       aDelegatingFrame,
                       nsIPresContext* aPresContext,
                       nsIPresShell&   aPresShell,
                       nsIAtom*        aListName,
                       nsIFrame*       aOldFrame);
  
  // Called by the delegating frame after it has done its reflow first. This
  // function will reflow any absolutely positioned child frames that need to
  // be reflowed, e.g., because the absolutely positioned child frame has
  // 'auto' for an offset, or a percentage based width or height.
  // Returns (in the local coordinate space) the bounding rect of the absolutely
  // positioned child elements taking into account their overflow area (if it
  // is visible)
  nsresult Reflow(nsIFrame*                aDelegatingFrame,
                  nsIPresContext*          aPresContext,
                  const nsHTMLReflowState& aReflowState,
                  nscoord                  aContainingBlockWidth,
                  nscoord                  aContainingBlockHeight,
                  nsRect&                  aChildBounds);

  // Called only for a reflow reason of eReflowReason_Incremental. The
  // aWasHandled return value indicates whether the reflow command was
  // handled (i.e., the reflow command involved an absolutely positioned
  // child element), or whether the caller should handle it.
  // Returns (in the local coordinate space) the bounding rect of the absolutely
  // positioned child elements taking into account their overflow area (if it
  // is visible). This is only set if the reflow command was handled
  nsresult IncrementalReflow(nsIFrame*                aDelegatingFrame,
                             nsIPresContext*          aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nscoord                  aContainingBlockWidth,
                             nscoord                  aContainingBlockHeight,
                             PRBool&                  aWasHandled,
                             nsRect&                  aChildBounds);

  void DestroyFrames(nsIFrame*       aDelegatingFrame,
                     nsIPresContext* aPresContext);

  PRBool  HasAbsoluteFrames() {return mAbsoluteFrames.NotEmpty();}

protected:
  nsresult ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                               nsIPresContext*          aPresContext,
                               const nsHTMLReflowState& aReflowState,
                               nscoord                  aContainingBlockWidth,
                               nscoord                  aContainingBlockHeight,
                               nsIFrame*                aKidFrame,
                               nsReflowReason           aReason,
                               nsReflowStatus&          aStatus);

  void CalculateChildBounds(nsIPresContext* aPresContext, nsRect& aChildBounds);

protected:
  nsFrameList mAbsoluteFrames;  // additional named child list
};

#endif /* nsnsAbsoluteContainingBlock_h___ */

