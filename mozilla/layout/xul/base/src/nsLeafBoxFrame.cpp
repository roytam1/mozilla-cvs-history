/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
// Eric Vaughan
// Netscape Communications
//
// See documentation in associated header file
//

#include "nsLeafBoxFrame.h"
#include "nsBoxFrame.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsHTMLAtoms.h"
#include "nsXULAtoms.h"
#include "nsIPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIStyleContext.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsBoxLayoutState.h"

//
// NS_NewToolbarFrame
//
// Creates a new Toolbar frame and returns it in |aNewFrame|
//
nsresult
NS_NewLeafBoxFrame ( nsIPresShell* aPresShell, nsIFrame** aNewFrame )
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsLeafBoxFrame* it = new (aPresShell) nsLeafBoxFrame(aPresShell);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  // it->SetFlags(aFlags);
  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewTextFrame

nsLeafBoxFrame::nsLeafBoxFrame(nsIPresShell* aShell):nsBox(aShell)
{
}

/**
 * Initialize us. This is a good time to get the alignment of the box
 */
NS_IMETHODIMP
nsLeafBoxFrame::Init(nsIPresContext*  aPresContext,
              nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIStyleContext* aContext,
              nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsLeafFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  mMouseThrough = sometimes;

  if (mContent) {
    nsAutoString value;
    if (NS_CONTENT_ATTR_HAS_VALUE == mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::mousethrough, value)) {
        if (value.EqualsIgnoreCase("never")) 
            mMouseThrough = never;
        else if (value.EqualsIgnoreCase("always")) 
            mMouseThrough = always;
      
    }
  }

  return rv;
}

NS_IMETHODIMP  
nsLeafBoxFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                             const nsPoint& aPoint, 
                             nsFramePaintLayer aWhichLayer,    
                             nsIFrame**     aFrame)
{   
  if (!mRect.Contains(aPoint))
    return NS_ERROR_FAILURE;

  if (mMouseThrough != never)
  {
     *aFrame = this;
     return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLeafBoxFrame::GetFrame(nsIFrame** aFrame)
{
  *aFrame = this;  
  return NS_OK;
}

NS_IMETHODIMP
nsLeafBoxFrame::DidReflow(nsIPresContext* aPresContext,
                   nsDidReflowStatus aStatus)
{
  PRBool isDirty = mState & NS_FRAME_IS_DIRTY;
  PRBool hasDirtyChildren = mState & NS_FRAME_HAS_DIRTY_CHILDREN;
  nsresult rv = nsFrame::DidReflow(aPresContext, aStatus);
  if (isDirty)
    mState |= NS_FRAME_IS_DIRTY;

  if (hasDirtyChildren)
    mState |= NS_FRAME_HAS_DIRTY_CHILDREN;

  return rv;

}


NS_IMETHODIMP
nsLeafBoxFrame::Reflow(nsIPresContext*   aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
   NS_ASSERTION(aReflowState.mComputedWidth >=0 && aReflowState.mComputedHeight >= 0, "Computed Size < 0");

  aStatus = NS_FRAME_COMPLETE;

  // create the layout state
  nsBoxLayoutState state(aPresContext, aReflowState);

  state.HandleReflow(this, PR_FALSE);

  nsSize computedSize(aReflowState.mComputedWidth,aReflowState.mComputedHeight);

  nsMargin m;
  GetBorderAndPadding(m);

  // this happens sometimes. So lets handle it gracefully.
  if (aReflowState.mComputedHeight == 0) {
    nsSize minSize(0,0);
    GetMinSize(state, minSize);
    computedSize.height = minSize.height - m.top - m.bottom;
  }

  // if we are told to layout intrinic then get our preferred size.
  if (computedSize.width == NS_INTRINSICSIZE || computedSize.height == NS_INTRINSICSIZE) {
     nsSize prefSize;
     nsSize minSize;
     nsSize maxSize;
     GetPrefSize(state, prefSize);
     GetMinSize(state,  minSize);
     GetMaxSize(state,  maxSize);
     BoundsCheck(minSize, prefSize, maxSize);

    // get our desiredSize
    if (aReflowState.mComputedWidth == NS_INTRINSICSIZE)
       computedSize.width = prefSize.width - m.left - m.right;

    if (aReflowState.mComputedHeight == NS_INTRINSICSIZE || aReflowState.mComputedHeight == 0)
       computedSize.height = prefSize.height - m.top - m.bottom;
  }

  nsRect r(0,0,computedSize.width, computedSize.height);
  r.Inflate(m);
  r.x = mRect.x;
  r.y = mRect.y;

  SetBounds(state, r);
 
  // layout our children
  Layout(state);
  
  // ok our child could have gotten bigger. So lets get its bounds
  GetBounds(r);
  
  // get the ascent
  nscoord ascent;
  GetAscent(state, ascent);

  aDesiredSize.width  = r.width;
  aDesiredSize.height = r.height;
  aDesiredSize.ascent = ascent;
  aDesiredSize.descent = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsLeafBoxFrame::GetFrameName(nsString& aResult) const
{
  aResult = "LeafBox";
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsLeafBoxFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsLeafBoxFrame::Release(void)
{
    return NS_OK;
}

NS_IMETHODIMP
nsLeafBoxFrame::ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent)
{
  NeedsRecalc();
  return nsLeafFrame::ContentChanged(aPresContext, aChild, aSubContent);
}


NS_INTERFACE_MAP_BEGIN(nsLeafBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsIBox)
#ifdef NS_DEBUG
  NS_INTERFACE_MAP_ENTRY(nsIFrameDebug)
#endif
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIBox)
NS_INTERFACE_MAP_END_INHERITING(nsLeafFrame)