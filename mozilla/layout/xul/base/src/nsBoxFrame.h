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
#include "nsContainerBox.h"
class nsBoxLayoutState;
class nsBoxFrameInner;
class nsBoxDebugInner;

class nsHTMLReflowCommand;
class nsHTMLInfo;

// flags for box info
#define NS_FRAME_BOX_SIZE_VALID    0x0001
#define NS_FRAME_BOX_IS_COLLAPSED  0x0002
#define NS_FRAME_BOX_NEEDS_RECALC  0x0004
#define NS_FRAME_IS_BOX            0x0008


// flags from box
#define NS_STATE_IS_HORIZONTAL           0x00400000
#define NS_STATE_AUTO_STRETCH            0x00800000
#define NS_STATE_IS_ROOT                 0x01000000
#define NS_STATE_CURRENTLY_IN_DEBUG      0x02000000
#define NS_STATE_SET_TO_DEBUG            0x04000000
#define NS_STATE_DEBUG_WAS_SET           0x08000000
#define NS_STATE_IS_COLLAPSED            0x10000000
#define NS_STATE_DEFAULT_HORIZONTAL      0x20000000
#define NS_STATE_STYLE_CHANGE            0x40000000

class nsBoxFrame : public nsHTMLContainerFrame, public nsContainerBox
{
public:

  friend nsresult NS_NewBoxFrame(nsIPresShell* aPresShell, 
                                 nsIFrame** aNewFrame, 
                                 PRBool aIsRoot = PR_FALSE,
                                 nsIBoxLayout* aLayoutManager = nsnull,
                                 PRBool aDefaultHorizontal = PR_TRUE);

  // gets the rect inside our border and debug border. If you wish to paint inside a box
  // call this method to get the rect so you don't draw on the debug border or outer border.

  // ------ nsISupports --------

  NS_DECL_ISUPPORTS_INHERITED

  // ------ nsIBox -------------

  NS_IMETHOD GetPrefSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetFlex(nsBoxLayoutState& aBoxLayoutState, nscoord& aFlex);
  NS_IMETHOD GetAscent(nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);
  NS_IMETHOD SetDebug(nsBoxLayoutState& aBoxLayoutState, PRBool aDebug);
  NS_IMETHOD GetFrame(nsIFrame** aFrame);
  NS_IMETHOD GetVAlign(Valignment& aAlign);
  NS_IMETHOD GetHAlign(Halignment& aAlign);
  NS_IMETHOD NeedsRecalc();
  NS_IMETHOD GetInset(nsMargin& aInset);
  NS_IMETHOD Layout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD GetDebug(PRBool& aDebug);

  //NS_IMETHOD GetMouseThrough(PRBool& aMouseThrough);

  // ----- child and sibling operations ---

  // ----- public methods -------
  
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint,
                              nsFramePaintLayer aWhichLayer,    
                              nsIFrame**     aFrame);

  NS_IMETHOD GetCursor(nsIPresContext* aPresContext,
                                     nsPoint&        aPoint,
                                     PRInt32&        aCursor);


  NS_IMETHOD ReflowDirtyChild(nsIPresShell* aPresShell, nsIFrame* aChild);

  NS_IMETHOD  Init(nsIPresContext*  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        asPrevInFlow);

 
  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);

  NS_IMETHOD Paint ( nsIPresContext* aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect& aDirtyRect,
                      nsFramePaintLayer aWhichLayer);



  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  InsertFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

  NS_IMETHOD  RemoveFrame(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  NS_IMETHOD  SetInitialChildList(nsIPresContext* aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList);

  NS_IMETHOD GetFrameName(nsString& aResult) const;

  NS_IMETHOD DidReflow(nsIPresContext* aPresContext,
                   nsDidReflowStatus aStatus);

  virtual PRBool IsHorizontal() const;

  virtual ~nsBoxFrame();

  virtual nsresult GetContentOf(nsIContent** aContent);
  virtual nsresult SyncLayout(nsBoxLayoutState& aBoxLayoutState);

  nsBoxFrame(nsIPresShell* aPresShell, PRBool aIsRoot = nsnull, nsIBoxLayout* aLayoutManager = nsnull, PRBool aDefaultHorizontal = PR_TRUE);
 
protected:
    virtual void GetBoxName(nsAutoString& aName);

    virtual PRBool HasStyleChange();
    virtual void SetStyleChangeFlag(PRBool aDirty);

    virtual void PropagateDebug(nsBoxLayoutState& aState);



    // Paint one child frame
    virtual void PaintChild(nsIPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nsIFrame*            aFrame,
                             nsFramePaintLayer    aWhichLayer);

    virtual void PaintChildren(nsIPresContext*      aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect,
                             nsFramePaintLayer    aWhichLayer);

    virtual PRIntn GetSkipSides() const { return 0; }


    virtual PRBool GetInitialOrientation(PRBool& aIsHorizontal); 
    virtual PRBool GetInitialHAlignment(Halignment& aHalign); 
    virtual PRBool GetInitialVAlignment(Valignment& aValign); 
    virtual PRBool GetInitialAutoStretch(PRBool& aStretch); 
  
    NS_IMETHOD  Destroy(nsIPresContext* aPresContext);

    virtual void GetInsertionPoint(nsIPresShell* aShell, nsIFrame* aChild, nsIFrame** aResult);

private: 
  
    friend class nsBoxFrameInner;
    friend class nsBoxDebug;
    nsBoxFrameInner* mInner;
}; // class nsBoxFrame

#endif

