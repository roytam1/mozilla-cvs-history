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

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsPopupSetFrame.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"

#define NS_MENU_POPUP_LIST_INDEX   (NS_AREA_FRAME_ABSOLUTE_LIST_INDEX + 1)

//
// NS_NewPopupSetFrame
//
// Wrapper for creating a new menu popup container
//
nsresult
NS_NewPopupSetFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsPopupSetFrame* it = new nsPopupSetFrame;
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::Release(void)
{
    return NS_OK;
}

NS_IMETHODIMP nsPopupSetFrame::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{           
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }   
  
  *aInstancePtr = NULL;                                                  
                                                                                        
  if (aIID.Equals(nsIPopupSetFrame::GetIID())) {                                         
    *aInstancePtr = (void*)(nsIPopupSetFrame*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }   

  return nsBoxFrame::QueryInterface(aIID, aInstancePtr);                                     
}

//
// nsPopupSetFrame cntr
//
nsPopupSetFrame::nsPopupSetFrame()
:mPresContext(nsnull), mElementFrame(nsnull)
{

} // cntr

nsIFrame*
nsPopupSetFrame::GetActiveChild()
{
  return mPopupFrames.FirstChild();
}

NS_IMETHODIMP
nsPopupSetFrame::Init(nsIPresContext&  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  mPresContext = &aPresContext; // Don't addref it.  Our lifetime is shorter.
  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);
  return rv;
}

// The following methods are all overridden to ensure that the menupopup frames
// are placed in the appropriate list.
NS_IMETHODIMP
nsPopupSetFrame::FirstChild(nsIAtom*   aListName,
                        nsIFrame** aFirstChild) const
{
  if (nsLayoutAtoms::popupList == aListName) {
    *aFirstChild = mPopupFrames.FirstChild();
  } else {
    nsBoxFrame::FirstChild(aListName, aFirstChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::SetInitialChildList(nsIPresContext& aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsLayoutAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {

    nsFrameList frames(aChildList);

    // We may have menupopups in here. Get them out, and move them into
    // the popup frame list.
    nsIFrame* frame = frames.FirstChild();
    while (frame) {
      nsCOMPtr<nsIContent> content;
      frame->GetContent(getter_AddRefs(content));
      nsCOMPtr<nsIAtom> tag;
      content->GetTag(*getter_AddRefs(tag));
      if (tag.get() == nsXULAtoms::popup) {
        // Remove this frame from the list and place it in the other list.
        frames.RemoveFrame(frame);
        mPopupFrames.AppendFrame(this, frame);
        nsIFrame* first = frames.FirstChild();
        rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, first);
        return rv;
      }
      frame->GetNextSibling(&frame);
    }

    // Didn't find it.
    rv = nsBoxFrame::SetInitialChildList(aPresContext, aListName, aChildList);
  }
  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const
{
   // Maintain a separate child list for the menu contents.
   // This is necessary because we don't want the menu contents to be included in the layout
   // of the menu's single item because it would take up space, when it is supposed to
   // be floating above the display.
  /*NS_PRECONDITION(nsnull != aListName, "null OUT parameter pointer");
  
  *aListName = nsnull;
  if (NS_MENU_POPUP_LIST_INDEX == aIndex) {
    *aListName = nsLayoutAtoms::popupList;
    NS_ADDREF(*aListName);
    return NS_OK;
  }*/
  return nsBoxFrame::GetAdditionalChildListName(aIndex, aListName);
}

NS_IMETHODIMP
nsPopupSetFrame::Destroy(nsIPresContext& aPresContext)
{
   // Cleanup frames in popup child list
  mPopupFrames.DestroyFrames(aPresContext);
  return nsBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsPopupSetFrame::Reflow(nsIPresContext&   aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  nsIFrame* frame = GetActiveChild();
    
  if (!frame || (rv != NS_OK))
    return rv;

  // Constrain the child's width and height to aAvailableWidth and aAvailableHeight
  nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame,
                                   availSize);
  kidReflowState.mComputedWidth = NS_UNCONSTRAINEDSIZE;
  kidReflowState.mComputedHeight = NS_UNCONSTRAINEDSIZE;
    
   // Reflow child
  nscoord w = aDesiredSize.width;
  nscoord h = aDesiredSize.height;
  
  nsRect rect;
  frame->GetRect(rect);
  rv = ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState,
                   rect.x, rect.y, 0, aStatus);

   // Set the child's width and height to its desired size
  FinishReflowChild(frame, aPresContext, aDesiredSize, rect.x, rect.y, 0);

  // Don't let it affect our size.
  aDesiredSize.width = w;
  aDesiredSize.height = h;
  
  return rv;
}


NS_IMETHODIMP
nsPopupSetFrame::DidReflow(nsIPresContext& aPresContext,
                            nsDidReflowStatus aStatus)
{
  // Sync up the view.
  nsIFrame* activeChild = GetActiveChild();
  if (activeChild) {
    ((nsMenuPopupFrame*)activeChild)->SyncViewWithFrame(aPresContext, PR_TRUE, mElementFrame, mXPos, mYPos);
  }

  return nsBoxFrame::DidReflow(aPresContext, aStatus);
}

// Overridden Box method.
NS_IMETHODIMP
nsPopupSetFrame::Dirty(nsIPresContext& aPresContext,  const nsHTMLReflowState& aReflowState, nsIFrame*& incrementalChild)
{
  incrementalChild = nsnull;
  nsresult rv = NS_OK;

  // Dirty any children that need it.
  nsIFrame* frame;
  aReflowState.reflowCommand->GetNext(frame, PR_FALSE);
  if (frame == nsnull) {
    incrementalChild = this;
    return rv;
  }

  // Now call our original box frame method
  rv = nsBoxFrame::Dirty(aPresContext, aReflowState, incrementalChild);
  if (rv != NS_OK || incrementalChild)
    return rv;

  nsIFrame* popup = GetActiveChild();
  if (popup && (frame == popup)) {
    // An incremental reflow command is targeting something inside our
    // hidden popup view.  We can't actually return the child, since it
    // won't ever be found by box.  Instead return ourselves, so that box
    // will later send us an incremental reflow command.
    incrementalChild = this;

    // In order for the child box to know what it needs to reflow, we need
    // to call its Dirty method...
    nsIFrame* ignore;
    nsIBox* ibox;
    if (NS_SUCCEEDED(popup->QueryInterface(nsIBox::GetIID(), (void**)&ibox)) && ibox)
      ibox->Dirty(aPresContext, aReflowState, ignore);
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::RemoveFrame(nsIPresContext& aPresContext,
                           nsIPresShell& aPresShell,
                           nsIAtom* aListName,
                           nsIFrame* aOldFrame)
{
  nsresult  rv;
  
  if (mPopupFrames.ContainsFrame(aOldFrame)) {
    // Go ahead and remove this frame.
    mPopupFrames.DestroyFrame(aPresContext, aOldFrame);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::RemoveFrame(aPresContext, aPresShell, aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::InsertFrames(nsIPresContext& aPresContext,
                            nsIPresShell& aPresShell,
                            nsIAtom* aListName,
                            nsIFrame* aPrevFrame,
                            nsIFrame* aFrameList)
{

  nsCOMPtr<nsIContent> frameChild;
  aFrameList->GetContent(getter_AddRefs(frameChild));
  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;
  frameChild->GetTag(*getter_AddRefs(tag));
  if (tag && tag.get() == nsXULAtoms::popup) {
    mPopupFrames.InsertFrames(nsnull, nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);  
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  if (!aFrameList)
    return NS_OK;

  nsCOMPtr<nsIContent> frameChild;
  aFrameList->GetContent(getter_AddRefs(frameChild));

  nsCOMPtr<nsIAtom> tag;
  nsresult          rv;
  
  frameChild->GetTag(*getter_AddRefs(tag));
  if (tag && tag.get() == nsXULAtoms::popup) {
    mPopupFrames.AppendFrames(nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList); 
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::CreatePopup(nsIFrame* aElementFrame, nsIContent* aPopupContent, 
                             PRInt32 aXPos, PRInt32 aYPos, 
                             const nsString& aPopupType, const nsString& anAnchorAlignment,
                             const nsString& aPopupAlignment)
{
  // Cache the element frame.
  mElementFrame = aElementFrame;

  // Show the popup at the specified position.
  mXPos = aXPos;
  mYPos = aYPos;

  printf("X Pos: %d\n", mXPos);
  printf("Y Pos: %d\n", mYPos);

  if (!OnCreate(aPopupContent))
    return NS_OK;

  // Generate the popup.
  MarkAsGenerated(aPopupContent);

  // Now we'll have it in our child frame list.
  
  // Now open the popup.
  OpenPopup(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::HidePopup()
{
  ActivatePopup(PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::DestroyPopup()
{
  OpenPopup(PR_FALSE);
  return NS_OK;
}

void
nsPopupSetFrame::MarkAsGenerated(nsIContent* aPopupContent)
{
  // Ungenerate all other popups in the set. No more than one can exist
  // at any point in time.
  PRInt32 childCount;
  mContent->ChildCount(childCount);
  for (PRInt32 i = 0; i < childCount; i++) {
    nsCOMPtr<nsIContent> childContent;
    mContent->ChildAt(i, *getter_AddRefs(childContent));

    // Retrieve the menugenerated attribute.
    nsAutoString value;
    childContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, 
                               value);
    if (value == "true") {
      // Ungenerate this element.
      childContent->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated,
                                   PR_TRUE);
    }
  }

  // Set our attribute, but only if we aren't already generated.
  // Retrieve the menugenerated attribute.
  nsAutoString value;
  aPopupContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, 
                              value);
  if (value != "true") {
    // Generate this element.
    aPopupContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, "true",
                                PR_TRUE);
  }
}

void
nsPopupSetFrame::OpenPopup(PRBool aActivateFlag)
{
  if (aActivateFlag) {
    ActivatePopup(PR_TRUE);
    
    nsIFrame* activeChild = GetActiveChild();

    nsCOMPtr<nsIMenuParent> childPopup = do_QueryInterface(activeChild);
    UpdateDismissalListener(childPopup);
  }
  else {
    if (!OnDestroy())
      return;

    // Unregister.
    if (nsMenuFrame::mDismissalListener) {
      nsMenuFrame::mDismissalListener->Unregister();
    }

    ActivatePopup(PR_FALSE);
  }
}

void
nsPopupSetFrame::ActivatePopup(PRBool aActivateFlag)
{
  nsCOMPtr<nsIContent> content;
  GetActiveChildElement(getter_AddRefs(content));
  if (content) {
    if (aActivateFlag)
      content->SetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, "true", PR_TRUE);
    else content->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
  }
}

PRBool
nsPopupSetFrame::OnCreate(nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_CREATE;

  if (aPopupContent) {
    nsresult rv = aPopupContent->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnDestroy()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_DESTROY;

  nsCOMPtr<nsIContent> content;
  GetActiveChildElement(getter_AddRefs(content));
  
  if (content) {
    nsresult rv = content->HandleDOMEvent(*mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, status);
    if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
      return PR_FALSE;
  }
  return PR_TRUE;
}

void
nsPopupSetFrame::GetActiveChildElement(nsIContent** aResult)
{
  *aResult = nsnull;
  nsIFrame* child = GetActiveChild();
  if (child) {
    child->GetContent(aResult);
  }
}

void
nsPopupSetFrame::UpdateDismissalListener(nsIMenuParent* aMenuParent)
{
  if (!nsMenuFrame::mDismissalListener) {
    if (!aMenuParent)
       return;
    // Create the listener and attach it to the outermost window.
    aMenuParent->CreateDismissalListener();
  }
  
  // Make sure the menu dismissal listener knows what the current
  // innermost menu popup frame is.
  nsMenuFrame::mDismissalListener->SetCurrentMenuParent(aMenuParent);
}


