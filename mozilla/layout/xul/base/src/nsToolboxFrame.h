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

//
// Mike Pinkerton
// Netscape Communications
//
// nsToolboxFrame is a layout object that contains one or more toolbar frames
// (specified as children in the DOM). These toolbars are laid out one on top
// of the other, and can be of varying heights but are all of the same width
// (the full width of the toolbox). Each toolbar is associated with a "grippy"
// which can be used to either collapse a particular toolbar or as a handle to
// pick up and move a toolbar to a new position within the toolbox. When toolbars
// are collapsed, it's grippy is moved to the bottom of the box and laid on
// its side. Clicking again on the grippy will reinstate the toolbar to its previous
// position in the toolbox.
//
// As mentioned above, the toolbox expects its toolbars to be its children in
// the DOM. The exact structure of the children is documented on:
//   http://www.mozilla.org/xpfe/DMWSpecNew.html
//

#ifndef nsToolBoxFrame_h___
#define nsToolBoxFrame_h___

#include "nsIDOMDragListener.h"
#include "nsHTMLContainerFrame.h"
#include "nsIStyleContext.h"
#include "nsIContent.h"
#include "nsXULAtoms.h"
#include "nsCOMPtr.h"
#include "nsBoxFrame.h"

#define NS_TOOLBOX_GRIPPY_NORMAL_CONTEXT_INDEX    0
#define NS_TOOLBOX_GRIPPY_ROLLOVER_CONTEXT_INDEX  1

class nsToolboxFrame : public nsBoxFrame
{
public:
  friend nsresult NS_NewToolboxFrame(nsIFrame** aNewFrame);

    // nsIHTMLReflow overrides
  NS_IMETHOD Reflow(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD GetBoxInfo(nsIPresContext& aPresContext, const nsHTMLReflowState& aReflowState, nsBoxInfo& aSize);

  NS_IMETHOD  Paint(nsIPresContext& aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect& aDirtyRect,
                    nsFramePaintLayer aWhichLayer);
  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext, 
                          nsGUIEvent*     aEvent,
                          nsEventStatus&  aEventStatus);

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow); 

  NS_IMETHOD  GetAdditionalStyleContext(PRInt32 aIndex, 
                                        nsIStyleContext** aStyleContext) const;
  NS_IMETHOD  SetAdditionalStyleContext(PRInt32 aIndex, 
                                        nsIStyleContext* aStyleContext);
 
    // Overridden to capture events
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint,
                              nsIFrame**     aFrame);

/*BEGIN implementations of dragevent handler interface*/
  virtual nsresult HandleEvent(nsIDOMEvent* aEvent);
  virtual nsresult DragEnter(nsIDOMEvent* aDragEvent);
  virtual nsresult DragOver(nsIDOMEvent* aDragEvent);
  virtual nsresult DragExit(nsIDOMEvent* aDragEvent);
  virtual nsresult DragDrop(nsIDOMEvent* aDragEvent);
  virtual nsresult DragGesture(nsIDOMEvent* aDragEvent) { return NS_OK; } 
/*END implementations of dragevent handler interface*/

protected:
  enum { kGrippyWidthInPixels = 10, kCollapsedGrippyHeightInPixels = 10, kCollapsedGrippyWidthInPixels = 50 } ;
  enum { kNoGrippyHilighted = -1 } ;

  struct TabInfo {
    TabInfo( nsIContent * inContent, PRBool inCollapsed, 
               const nsRect &inBounds = nsRect(0,0,0,0)) 
       : mToolbar(inContent),
         mBoundingRect(inBounds),
         mCollapsed(inCollapsed)
      {
      } 

    void SetBounds(const nsRect &inBounds) { mBoundingRect = inBounds; }

    nsIContent*    mToolbar;       // content object associated w/ toolbar frame. We don't own it.
    nsRect         mBoundingRect;
    PRBool         mCollapsed;
  };

  nsToolboxFrame();
  virtual ~nsToolboxFrame();

  virtual void UpdateStyles(nsIPresContext* aPresContext);
  virtual void CalculateGrippies(nsIPresContext& aPresContext);
  virtual nsresult ReflowGrippies(nsIPresContext&          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

 
  void RefreshStyleContext(nsIPresContext* aPresContext,
                            nsIAtom *         aNewContentPseudo,
                            nsCOMPtr<nsIStyleContext>* aCurrentStyle,
                            nsIContent *      aContent,
                            nsIStyleContext*  aParentStyle) ;

  void DrawGrippies ( nsIPresContext& aPresContext, nsIRenderingContext & aContext ) const ;
  void DrawGrippy ( nsIPresContext& aPresContext, nsIRenderingContext & aContext, 
                      const nsRect & aBoundingRect, PRBool aDrawHilighted ) const ;
  void CollapseToolbar ( TabInfo & inTab ) ; 
  void ExpandToolbar ( TabInfo & inTab ) ; 

  void ConvertToLocalPoint ( nsIPresContext* aPresContext, nsPoint & ioPoint ) ;
  void OnMouseMove ( nsIPresContext* aPresContext, nsPoint & aMouseLoc ) ;
  void OnMouseExit ( nsIPresContext* aPresContext ) ;
  void OnMouseLeftClick ( nsIPresContext* aPresContext, nsPoint & aMouseLoc ) ;

    // utility routines
  TabInfo* FindGrippyForToolbar ( nsVoidArray & inList, const nsIContent* inContent ) const ;
  void ClearGrippyList ( nsVoidArray & inList ) ;

    // style context for the normal state and rollover state of grippies
  nsCOMPtr<nsIStyleContext>    mGrippyNormalStyle;
  nsCOMPtr<nsIStyleContext>    mGrippyRolloverStyle;
  
  nsMargin mInset;
  virtual void GetInset(nsMargin& margin);

  unsigned long mSumOfToolbarHeights;
  nsVoidArray  mGrippies;          // list of all active grippies
  unsigned short mNumToolbars;
  short mGrippyHilighted;          // used to indicate which grippy the mouse is inside

  const nsCOMPtr<nsIAtom> kCollapsedAtom ;
  const nsCOMPtr<nsIAtom> kHiddenAtom ;
  
  class DragListenerDelegate : public nsIDOMDragListener
  {
  protected:
    nsToolboxFrame* mFrame;

  public:
    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIDOMEventListener interface
    virtual nsresult HandleEvent(nsIDOMEvent* aEvent)
    {
      return mFrame ? mFrame->HandleEvent(aEvent) : NS_OK;
    }

    virtual nsresult DragGesture(nsIDOMEvent* aEvent)
    {
      return mFrame ? mFrame->DragGesture(aEvent) : NS_OK;
    }

    // nsIDOMDragListener interface
    virtual nsresult DragEnter(nsIDOMEvent* aMouseEvent)
    {
      return mFrame ? mFrame->DragEnter(aMouseEvent) : NS_OK;
    }

    virtual nsresult DragOver(nsIDOMEvent* aMouseEvent)
    {
      return mFrame ? mFrame->DragOver(aMouseEvent) : NS_OK;
    }

    virtual nsresult DragExit(nsIDOMEvent* aMouseEvent)
    {
      return mFrame ? mFrame->DragExit(aMouseEvent) : NS_OK;
    }

    virtual nsresult DragDrop(nsIDOMEvent* aMouseEvent)
    {
      return mFrame ? mFrame->DragDrop(aMouseEvent) : NS_OK;
    }

    // Implementation methods
    DragListenerDelegate(nsToolboxFrame* aFrame) : mFrame(aFrame)
    {
      NS_INIT_REFCNT();
    }

    virtual ~DragListenerDelegate() {}

    void NotifyFrameDestroyed() { mFrame = nsnull; }
  };
  DragListenerDelegate* mDragListenerDelegate;
  
    // pass-by-value not allowed for a toolbox because it corresponds 1-to-1
    // with an element in the UI.
  nsToolboxFrame ( const nsToolboxFrame& aFrame ) ;	            // DO NOT IMPLEMENT
  nsToolboxFrame& operator= ( const nsToolboxFrame& aFrame ) ;  // DO NOT IMPLEMENT
  
}; // class nsToolboxFrame

#endif
