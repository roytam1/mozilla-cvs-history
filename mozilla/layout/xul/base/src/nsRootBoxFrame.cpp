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
#include "nsHTMLParts.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsIDocument.h"
#include "nsIReflowCommand.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsViewsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsHTMLIIDs.h"
#include "nsPageFrame.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIViewManager.h"
#include "nsHTMLAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDeviceContext.h"
#include "nsIScrollableView.h"
#include "nsLayoutAtoms.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"

// Interface IDs
static NS_DEFINE_IID(kScrollViewIID, NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kIFrameIID, NS_IFRAME_IID);

/**
 * Root frame class.
 *
 * The root frame is the parent frame for the document element's frame.
 * It only supports having a single child frame which must be an area
 * frame
 */
class nsRootBoxFrame : public nsBoxFrame {
public:

  friend nsresult NS_NewBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame);

  nsRootBoxFrame(nsIPresShell* aShell);

  NS_IMETHOD AppendFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIPresContext* aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD HandleEvent(nsIPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                              nsFramePaintLayer aWhichLayer,
                              nsIFrame**     aFrame);
  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::rootFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsString& aResult) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif
};

//----------------------------------------------------------------------

nsresult
NS_NewRootBoxFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsRootBoxFrame* it = new (aPresShell) nsRootBoxFrame (aPresShell);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aNewFrame = it;

  return NS_OK;
}

nsRootBoxFrame::nsRootBoxFrame(nsIPresShell* aShell):nsBoxFrame(aShell, PR_TRUE)
{
  nsCOMPtr<nsIBoxLayout> layout;
  NS_NewStackLayout(aShell, layout);
  SetLayoutManager(layout);
}

NS_IMETHODIMP
nsRootBoxFrame::AppendFrames(nsIPresContext* aPresContext,
                        nsIPresShell&   aPresShell,
                        nsIAtom*        aListName,
                        nsIFrame*       aFrameList)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  NS_PRECONDITION(mFrames.IsEmpty(), "already have a child frame");
  if (aListName) {
    // We only support unnamed principal child list
    rv = NS_ERROR_INVALID_ARG;

  } else if (!mFrames.IsEmpty()) {
    // We only allow a single child frame
    rv = NS_ERROR_FAILURE;

  } else {
    rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
nsRootBoxFrame::InsertFrames(nsIPresContext* aPresContext,
                        nsIPresShell&   aPresShell,
                        nsIAtom*        aListName,
                        nsIFrame*       aPrevFrame,
                        nsIFrame*       aFrameList)
{
  nsresult  rv;

  // Because we only support a single child frame inserting is the same
  // as appending
  NS_PRECONDITION(!aPrevFrame, "unexpected previous sibling frame");
  if (aPrevFrame) {
    rv = NS_ERROR_UNEXPECTED;
  } else {
    rv = AppendFrames(aPresContext, aPresShell, aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
nsRootBoxFrame::RemoveFrame(nsIPresContext* aPresContext,
                       nsIPresShell&   aPresShell,
                       nsIAtom*        aListName,
                       nsIFrame*       aOldFrame)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  if (aListName) {
    // We only support the unnamed principal child list
    rv = NS_ERROR_INVALID_ARG;
  
  } else if (aOldFrame == mFrames.FirstChild()) {
     rv = nsBoxFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP
nsRootBoxFrame::Reflow(nsIPresContext*          aPresContext,
                  nsHTMLReflowMetrics&     aDesiredSize,
                  const nsHTMLReflowState& aReflowState,
                  nsReflowStatus&          aStatus)
{
  return nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}

NS_IMETHODIMP
nsRootBoxFrame::HandleEvent(nsIPresContext* aPresContext, 
                       nsGUIEvent* aEvent,
                       nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  if (aEvent->message == NS_MOUSE_LEFT_BUTTON_UP ||
      aEvent->message == NS_MOUSE_MIDDLE_BUTTON_UP ||
      aEvent->message == NS_MOUSE_RIGHT_BUTTON_UP) {
    nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRootBoxFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                   const nsPoint& aPoint, 
                                   nsFramePaintLayer aWhichLayer,
                                   nsIFrame**     aFrame)
{
  // this should act like a block, so we need to override
  return nsBoxFrame::GetFrameForPoint(aPresContext, aPoint, aWhichLayer, aFrame);
}

NS_IMETHODIMP
nsRootBoxFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::rootFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsRootBoxFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("RootBox", aResult);
}

NS_IMETHODIMP
nsRootBoxFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = sizeof(*this);
  return NS_OK;
}
#endif
