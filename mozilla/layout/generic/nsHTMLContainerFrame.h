/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsHTMLContainerFrame_h___
#define nsHTMLContainerFrame_h___

#include "nsContainerFrame.h"
class nsString;
class nsAbsoluteFrame;
class nsPlaceholderFrame;
struct nsStyleDisplay;
struct nsStylePosition;
struct nsHTMLReflowMetrics;
struct nsHTMLReflowState;

// Some macros for container classes to do sanity checking on
// width/height/x/y values computed during reflow.
#ifdef DEBUG
#define CRAZY_W 500000

// 100000 lines, approximately. Assumes p2t is 15 and 15 pixels per line
#define CRAZY_H 22500000

#define CRAZY_WIDTH(_x) (((_x) < -CRAZY_W) || ((_x) > CRAZY_W))
#define CRAZY_HEIGHT(_y) (((_y) < -CRAZY_H) || ((_y) > CRAZY_H))
#endif

// Base class for html container frames that provides common
// functionality.
class nsHTMLContainerFrame : public nsContainerFrame {
public:
  NS_IMETHOD  Paint(nsIPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer,
                    PRUint32             aFlags = 0);

  /**
   * Helper method to create next-in-flows if necessary. If aFrame
   * already has a next-in-flow then this method does
   * nothing. Otherwise, a new continuation frame is created and
   * linked into the flow. In addition, the new frame becomes the
   * next-sibling of aFrame. If aPlaceholderResult is not null and
   * aFrame is a float or positioned, then *aPlaceholderResult holds
   * a placeholder.
   */
  static nsresult CreateNextInFlow(nsIPresContext* aPresContext,
                                   nsIFrame*       aOuterFrame,
                                   nsIFrame*       aFrame,
                                   nsIFrame*&      aNextInFlowResult);

  /**
   * Helper method to wrap views around frames. Used by containers
   * under special circumstances (can be used by leaf frames as well)
   * @param aContentParentFrame
   *         if non-null, this is the frame 
   *         which would have held aFrame except that aFrame was reparented
   *         to an alternative geometric parent. This is necessary
   *         so that aFrame can remember to get its Z-order from 
   *         aContentParentFrame.
   */
  static nsresult CreateViewForFrame(nsIPresContext* aPresContext,
                                     nsIFrame* aFrame,
                                     nsIStyleContext* aStyleContext,
                                     nsIFrame* aContentParentFrame,
                                     PRBool aForce);

  static nsresult ReparentFrameView(nsIPresContext* aPresContext,
                                    nsIFrame*       aChildFrame,
                                    nsIFrame*       aOldParentFrame,
                                    nsIFrame*       aNewParentFrame);

  static nsresult ReparentFrameViewList(nsIPresContext* aPresContext,
                                        nsIFrame*       aChildFrameList,
                                        nsIFrame*       aOldParentFrame,
                                        nsIFrame*       aNewParentFrame);

  /**
   * Helper method to invalidate portions of a standard container frame if the
   * reflow state indicates that they have changed (specifically border and
   * padding).
   * @param aPresContext the presentation context
   * @param aDesiredSize the new size of the frame
   * @param aReflowState the reflow that was just done on this frame
   */
  void CheckInvalidateBorder(nsIPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState);

protected:
  virtual PRIntn GetSkipSides() const = 0;
};

#endif /* nsHTMLContainerFrame_h___ */

