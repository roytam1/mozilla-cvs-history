/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/**

  Eric D Vaughan
  nsBoxFrame is a frame that can lay its children out either vertically or horizontally.
  It lays them out according to a min max or preferred size.
 
**/

#ifndef nsBoxFrame_h___
#define nsBoxFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIBox.h"
class nsHTMLReflowCommand;

class nsCalculatedBoxInfo : public nsBoxInfo {
public:
    nsSize calculatedSize;
    PRBool sizeValid;
    PRBool needsReflow;
    PRBool needsRecalc;
    PRBool collapsed;

    nsCalculatedBoxInfo();
    virtual void clear();

};

class nsBoxFrame : public nsHTMLContainerFrame, public nsIBox
{
public:

  friend nsresult NS_NewBoxFrame(nsIFrame** aNewFrame);

  // nsIBox methods
  NS_IMETHOD GetBoxInfo(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsBoxInfo& aSize);
  NS_IMETHOD Dirty(const nsHTMLReflowState& aReflowState, nsIFrame*& incrementalChild);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr); 


  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);

 
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);


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

  PRBool IsHorizontal() const { return mHorizontal; }

  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);


protected:
    nsBoxFrame();

    virtual void GetRedefinedMinPrefMax(nsIFrame* aFrame, nsBoxInfo& aSize);
    virtual nsresult GetChildBoxInfo(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsIFrame* aFrame, nsBoxInfo& aSize);
    virtual nsresult FlowChildren(nsIPresContext&   aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus,
                     nsRect& availableSize,
                     nsIFrame*& incrementalChild); 

    virtual nsresult FlowChildAt(nsIFrame* frame, 
                     nsIPresContext& aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus,
                     nscoord spring,
                     nsIFrame*& incrementalChild);

    virtual nsresult PlaceChildren(nsRect& boxRect);


    virtual void BoundsCheck(const nsBoxInfo& aBoxInfo, nsRect& aRect);

    /*
	  virtual void GetDesiredSize(nsIPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aDesiredSize);
    */

    virtual PRIntn GetSkipSides() const { return 0; }

    virtual void GetInset(nsMargin& margin);
  
    virtual void LayoutChildrenInRect(nsRect& size);

    virtual void InvalidateChildren();

    virtual void AddSize(const nsSize& a, nsSize& b, PRBool largest);


    PRBool mHorizontal;

private: 
  
    // XXX for the moment we can only handle 100 children.
    // Should use a dynamic array.
    nsCalculatedBoxInfo mSprings[100];
    nscoord mSpringCount;
}; // class nsBoxFrame



#endif

