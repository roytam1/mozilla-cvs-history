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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/**

  Eric D Vaughan
  nsBoxFrame is a frame that can lay its children out either vertically or horizontally.
  It lays them out according to a min max or preferred size.
 
**/

#ifndef nsBoxFrame_h___
#define nsBoxFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsIBox.h"
#include "nsISpaceManager.h"
class nsBoxFrameInner;
class nsBoxDebugInner;

class nsHTMLReflowCommand;

class nsCalculatedBoxInfo : public nsBoxInfo {
public:
    nsSize calculatedSize;
    PRBool prefWidthIntrinsic;
    PRBool prefHeightIntrinsic;
    PRBool sizeValid;
    PRBool needsReflow;
    PRBool needsRecalc;
    PRBool collapsed;
    PRBool isIncremental;

    nsCalculatedBoxInfo();
    nsCalculatedBoxInfo(const nsBoxInfo& aInfo);
    virtual void clear();

};

class nsBoxFrame : public nsHTMLContainerFrame, public nsIBox
{
public:

  friend nsresult NS_NewBoxFrame(nsIFrame** aNewFrame, PRUint32 aFlags = 0);
  // gets the rect inside our border and debug border. If you wish to paint inside a box
  // call this method to get the rect so you don't draw on the debug border or outer border.
  virtual void GetInnerRect(nsRect& aInner);

  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                             nsIFrame**     aFrame);

  NS_IMETHOD GetCursor(nsIPresContext& aPresContext,
                                     nsPoint&        aPoint,
                                     PRInt32&        aCursor);



  // nsIBox methods
  NS_IMETHOD GetBoxInfo(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsBoxInfo& aSize);
  NS_IMETHOD Dirty(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsIFrame*& aIncrementalChild);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr); 

  NS_IMETHOD DidReflow(nsIPresContext& aPresContext,
                      nsDidReflowStatus aStatus);


  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);

 
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);

  NS_IMETHOD Paint ( nsIPresContext& aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect& aDirtyRect,
                      nsFramePaintLayer aWhichLayer);

  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  InsertFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  RemoveFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  NS_IMETHOD GetFrameName(nsString& aResult) const;

  virtual PRBool IsHorizontal() const { return mHorizontal; }

  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual ~nsBoxFrame();

  virtual void GetChildBoxInfo(PRInt32 aIndex, nsBoxInfo& aSize);
  virtual void SetChildNeedsRecalc(PRInt32 aIndex, PRBool aRecalc);

  // Paint one child frame
  virtual void PaintChild(nsIPresContext&      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nsIFrame*            aFrame,
                             nsFramePaintLayer    aWhichLayer);


protected:
    nsBoxFrame(PRUint32 aFlags = 0);

    virtual void GetRedefinedMinPrefMax(nsIPresContext& aPresContext, nsIFrame* aFrame, nsCalculatedBoxInfo& aSize);
    virtual nsresult GetChildBoxInfo(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsIFrame* aFrame, nsCalculatedBoxInfo& aSize);
    virtual nsresult FlowChildren(nsIPresContext&   aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus,
                     nsRect& availableSize); 

    virtual nsresult FlowChildAt(nsIFrame* frame, 
                     nsIPresContext& aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus,
                     nsCalculatedBoxInfo&     aInfo,
                     PRBool& needsRedraw,
                     nsString& aReason);

    virtual nsresult PlaceChildren(nsIPresContext& aPresContext, nsRect& boxRect);
    virtual void ChildResized(nsIFrame* aFrame, nsHTMLReflowMetrics& aDesiredSize, nsRect& aRect, nsCalculatedBoxInfo& aInfo, PRBool* aResized, nscoord& aChangedIndex, PRBool& aFinished, nscoord aIndex, nsString& aReason);
    virtual void LayoutChildrenInRect(nsRect& size);
    virtual void AddChildSize(nsBoxInfo& aInfo, nsBoxInfo& aChildInfo);
    virtual void BoundsCheck(const nsBoxInfo& aBoxInfo, nsRect& aRect);
    virtual void InvalidateChildren();
    virtual void AddSize(const nsSize& a, nsSize& b, PRBool largest);

    virtual PRIntn GetSkipSides() const { return 0; }

    virtual void GetInset(nsMargin& margin); 
    virtual void CollapseChild(nsIPresContext& aPresContext, nsIFrame* frame, PRBool hide);

    nsresult GenerateDirtyReflowCommand(nsIPresContext& aPresContext,
                                        nsIPresShell&   aPresShell);

    PRBool mHorizontal;
    nsCalculatedBoxInfo mSprings[100];
    nscoord mSpringCount;

private: 
  
    friend class nsBoxFrameInner;
    friend class nsBoxDebugInner;
    nsBoxFrameInner* mInner;
}; // class nsBoxFrame



#endif

