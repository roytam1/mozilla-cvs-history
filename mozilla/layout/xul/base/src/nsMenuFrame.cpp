/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *   Michael Lowe <michael.lowe@bigfoot.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Dean Tessman <dean_tessman@hotmail.com>
 */

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
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
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsILookAndFeel.h"
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"

#define NS_MENU_POPUP_LIST_INDEX   (NS_AREA_FRAME_ABSOLUTE_LIST_INDEX + 1)

static PRInt32 gEatMouseMove = PR_FALSE;

nsMenuDismissalListener* nsMenuFrame::mDismissalListener = nsnull;

static NS_DEFINE_IID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kILookAndFeelIID, NS_ILOOKANDFEEL_IID);


//
// NS_NewMenuFrame
//
// Wrapper for creating a new menu popup container
//
nsresult
NS_NewMenuFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame, PRUint32 aFlags)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMenuFrame* it = new (aPresShell) nsMenuFrame;
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  if (aFlags)
    it->SetIsMenu(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsMenuFrame::Release(void)
{
    return NS_OK;
}

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIMenuFrame)
  NS_INTERFACE_MAP_ENTRY(nsIAnonymousContentCreator)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

//
// nsMenuFrame cntr
//
nsMenuFrame::nsMenuFrame()
  : mIsMenu(PR_FALSE),
    mMenuOpen(PR_FALSE),
    mChecked(PR_FALSE),
    mType(eMenuType_Normal),
    mMenuParent(nsnull),
    mPresContext(nsnull),
    mGroupName("")
{

} // cntr

NS_IMETHODIMP
nsMenuFrame::Init(nsIPresContext*  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  mPresContext = aPresContext; // Don't addref it.  Our lifetime is shorter.

  nsresult  rv = nsBoxFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  // Set our menu parent.
  nsCOMPtr<nsIMenuParent> menuparent = do_QueryInterface(aParent);
  mMenuParent = menuparent.get();

  // Do the type="checkbox" magic
  UpdateMenuType(aPresContext);

  nsAutoString accelString;
  BuildAcceleratorText(accelString);
  
  return rv;
}

// The following methods are all overridden to ensure that the menupopup frame
// is placed in the appropriate list.
NS_IMETHODIMP
nsMenuFrame::FirstChild(nsIPresContext* aPresContext,
                        nsIAtom*        aListName,
                        nsIFrame**      aFirstChild) const
{
  if (nsLayoutAtoms::popupList == aListName) {
    *aFirstChild = mPopupFrames.FirstChild();
  } else {
    nsBoxFrame::FirstChild(aPresContext, aListName, aFirstChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SetInitialChildList(nsIPresContext* aPresContext,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsLayoutAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {

    nsFrameList frames(aChildList);

    // We may have a menupopup in here. Get it out, and move it into
    // the popup frame list.
    nsIFrame* frame = frames.FirstChild();
    while (frame) {
      nsCOMPtr<nsIContent> content;
      frame->GetContent(getter_AddRefs(content));
      nsCOMPtr<nsIAtom> tag;
      content->GetTag(*getter_AddRefs(tag));
      if (tag.get() == nsXULAtoms::menupopup) {
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
    
    nsCOMPtr<nsIDOMXULMenuListElement> list(do_QueryInterface(mContent));
    if (list) {
      nsCOMPtr<nsIDOMElement> element;
      list->GetSelectedItem(getter_AddRefs(element));
      if (!element) {
        nsAutoString value;
        list->GetValue(value);
        if (value == "") {
          nsCOMPtr<nsIContent> child;
          GetMenuChildrenElement(getter_AddRefs(child));
          if (child) {
            PRInt32 count;
            child->ChildCount(count);
            if (count > 0) {
              nsCOMPtr<nsIContent> item;
              child->ChildAt(0, *getter_AddRefs(item));
              nsCOMPtr<nsIDOMElement> selectedElement(do_QueryInterface(item));
              if (selectedElement) 
                list->SetSelectedItem(selectedElement);
            }
          }
        }
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsMenuFrame::GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const
{
  return nsBoxFrame::GetAdditionalChildListName(aIndex, aListName);
}

NS_IMETHODIMP
nsMenuFrame::Destroy(nsIPresContext* aPresContext)
{
   // Cleanup frames in popup child list
  mPopupFrames.DestroyFrames(aPresContext);
  return nsBoxFrame::Destroy(aPresContext);
}

// Called to prevent events from going to anything inside the menu.
NS_IMETHODIMP
nsMenuFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                              nsIFrame**     aFrame)
{
  nsresult result = nsBoxFrame::GetFrameForPoint(aPresContext, aPoint, aFrame);
  nsCOMPtr<nsIContent> content;
  if (*aFrame) {
    (*aFrame)->GetContent(getter_AddRefs(content));
    if (content) {
      // This allows selective overriding for subcontent.
      nsAutoString value;
      content->GetAttribute(kNameSpaceID_None, nsXULAtoms::allowevents, value);
      if (value == "true")
        return result;
    }
  }
  *aFrame = this; // Capture all events so that we can perform selection
  return NS_OK;
}

NS_IMETHODIMP 
nsMenuFrame::HandleEvent(nsIPresContext* aPresContext, 
                             nsGUIEvent*     aEvent,
                             nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  *aEventStatus = nsEventStatus_eConsumeDoDefault;
  
  if (IsDisabled()) // Disabled menus process no events.
    return NS_OK;

  if (aEvent->message == NS_KEY_PRESS) {
    nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
    PRUint32 keyCode = keyEvent->keyCode;
    if (keyCode == NS_VK_UP || keyCode == NS_VK_DOWN) {
      if (!IsOpen()) 
        OpenMenu(PR_TRUE);
    }
  }
  else if (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN) {
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(isMenuBar);

    // The menu item was selected. Bring up the menu.
    // We have children.
    if (mIsMenu)
      ToggleMenuState();

    if (isMenuBar && mIsMenu) {

      if (!IsOpen()) {
        // We closed up. The menu bar should always be
        // deactivated when this happens.
        mMenuParent->SetActive(PR_FALSE);
      }
    }
  }
  else if ( aEvent->message == NS_MOUSE_RIGHT_BUTTON_UP && mMenuParent ) {
    // if this menu is a context menu it accepts right-clicks...fire away!
    PRBool isContextMenu = PR_FALSE;
    mMenuParent->GetIsContextMenu(isContextMenu);
    if ( isContextMenu )
      Execute();
  }
  else if (aEvent->message == NS_MOUSE_LEFT_BUTTON_UP && !IsMenu() && mMenuParent) {
    // First, flip "checked" state if we're a checkbox menu, or
    // an un-checked radio menu
    if (mType == eMenuType_Checkbox ||
        (mType == eMenuType_Radio && !mChecked)) {
      nsAutoString checked;

      if (mChecked)
        checked = "false";
      else
        checked = "true";
      mContent->SetAttribute(kNameSpaceID_None, nsHTMLAtoms::checked, checked,
                             PR_TRUE);
      /* the AttributeChanged code will update all the internal state */
    }

    // Execute the execute event handler.
    Execute();
  }
  else if (aEvent->message == NS_MOUSE_EXIT) {
    // Kill our timer if one is active.
    if (mOpenTimer) {
      mOpenTimer->Cancel();
      mOpenTimer = nsnull;
    }

    // Deactivate the menu.
    PRBool isActive = PR_FALSE;
    PRBool isMenuBar = PR_FALSE;
    if (mMenuParent) {
      mMenuParent->IsMenuBar(isMenuBar);
      PRBool cancel = PR_TRUE;
      if (isMenuBar) {
        mMenuParent->GetIsActive(isActive);
        if (isActive) cancel = PR_FALSE;
      }
      
      if (cancel) {
        if (IsMenu() && !isMenuBar && mMenuOpen) {
          // Submenus don't get closed up immediately.
        }
        else mMenuParent->SetCurrentMenuItem(nsnull);
      }
    }
  }
  else if (aEvent->message == NS_MOUSE_MOVE && mMenuParent) {
    if (gEatMouseMove) {
      gEatMouseMove = PR_FALSE;
      return NS_OK;
    }

    // we checked for mMenuParent right above

    PRBool isMenuBar = PR_FALSE;
    mMenuParent->IsMenuBar(isMenuBar);

    // Let the menu parent know we're the new item.
    mMenuParent->SetCurrentMenuItem(this);
    
    // If we're a menu (and not a menu item),
    // kick off the timer.
    if (!isMenuBar && IsMenu() && !mMenuOpen && !mOpenTimer) {

      PRInt32 menuDelay = 300;   // ms

      nsILookAndFeel * lookAndFeel;
      if (NS_OK == nsComponentManager::CreateInstance(kLookAndFeelCID, nsnull, 
                      kILookAndFeelIID, (void**)&lookAndFeel)) {
        lookAndFeel->GetMetric(nsILookAndFeel::eMetric_SubmenuDelay, menuDelay);
       NS_RELEASE(lookAndFeel);
      }

      // We're a menu, we're built, we're closed, and no timer has been kicked off.
      NS_NewTimer(getter_AddRefs(mOpenTimer));
      mOpenTimer->Init(this, menuDelay, NS_PRIORITY_HIGHEST);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ToggleMenuState()
{  
  if (mMenuOpen) {
    OpenMenu(PR_FALSE);
  }
  else {
    OpenMenu(PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectMenu(PRBool aActivateFlag)
{
  if (aActivateFlag) {
    // Highlight the menu.
    mContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, "true", PR_TRUE);
  }
  else {
    // Unhighlight the menu.
    mContent->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
  }

  return NS_OK;
}

PRBool nsMenuFrame::IsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsString genVal;
    child->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal == "")
      return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::MarkAsGenerated()
{
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  // Generate the menu if it hasn't been generated already.  This
  // takes it from display: none to display: block and gives us
  // a menu forevermore.
  if (child) {
    nsAutoString genVal;
    child->GetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, genVal);
    if (genVal == "")
      child->SetAttribute(kNameSpaceID_None, nsXULAtoms::menugenerated, "true", PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::ActivateMenu(PRBool aActivateFlag)
{
  // Activate the menu without opening it.
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));

  // We've got some children for real.
  if (child) {
    // When we sync the popup view with the frame, we'll show the popup if |menutobedisplayed|
    // is set by setting the |menuactive| attribute. This trips CSS to make the view visible.
    // We wait until the last possible moment to show to avoid flashing, but we can just go
    // ahead and hide it here if we're told to (no additional stages necessary).
    if (aActivateFlag)
      child->SetAttribute(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, "true", PR_TRUE);
    else {
      child->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
      child->UnsetAttribute(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, PR_TRUE);
    }
  }

  return NS_OK;
}  

NS_IMETHODIMP
nsMenuFrame::AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint)
{
  nsAutoString value;

  if (aAttribute == nsXULAtoms::open) {
    aChild->GetAttribute(kNameSpaceID_None, aAttribute, value);
    if (value == "true")
      OpenMenuInternal(PR_TRUE);
    else
      OpenMenuInternal(PR_FALSE);
  } else if (aAttribute == nsHTMLAtoms::checked) {
    if (mType != eMenuType_Normal)
        UpdateMenuSpecialState(aPresContext);
  } else if ( aAttribute == nsHTMLAtoms::type || aAttribute == nsHTMLAtoms::name )
    UpdateMenuType(aPresContext);

  /* we need to reflow, if these change */
  if (aAttribute == nsHTMLAtoms::value ||
      aAttribute == nsXULAtoms::acceltext ||
      aAttribute == nsHTMLAtoms::type ||
      aAttribute == nsHTMLAtoms::checked) {

    nsCOMPtr<nsIPresShell> shell;
    nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIReflowCommand> reflowCmd;
    rv = NS_NewHTMLReflowCommand(getter_AddRefs(reflowCmd), this,
                                 nsIReflowCommand::StyleChanged);
    if (NS_FAILED(rv))
      return rv;

    shell->AppendReflowCommand(reflowCmd);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::OpenMenu(PRBool aActivateFlag)
{
  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
  if (aActivateFlag) {
    // Now that the menu is opened, we should have a menupopup child built.
    // Mark it as generated, which ensures a frame gets built.
    MarkAsGenerated();

    domElement->SetAttribute("open", "true");
  }
  else domElement->RemoveAttribute("open");

  return NS_OK;
}

void 
nsMenuFrame::OpenMenuInternal(PRBool aActivateFlag) 
{
  gEatMouseMove = PR_TRUE;

  if (!mIsMenu)
    return;

  if (aActivateFlag) {
    // Execute the oncreate handler
    if (!OnCreate())
      return;

    // Set the focus back to our view's widget.
    if (nsMenuFrame::mDismissalListener)
      nsMenuFrame::mDismissalListener->EnableListener(PR_FALSE);
    
    // XXX Only have this here because of RDF-generated content.
    MarkAsGenerated();

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    ActivateMenu(PR_TRUE);
  
    if (menuPopup) {
      // Install a keyboard navigation listener if we're the root of the menu chain.
      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->InstallKeyboardNavigator();
      else if (!mMenuParent)
        menuPopup->InstallKeyboardNavigator();
      
      // Tell the menu bar we're active.
      if (mMenuParent)
        mMenuParent->SetActive(PR_TRUE);

      nsCOMPtr<nsIContent> menuPopupContent;
      menuPopup->GetContent(getter_AddRefs(menuPopupContent));

      // See if we're a menulist and set our selection accordingly.
      nsCOMPtr<nsIDOMXULMenuListElement> list = do_QueryInterface(mContent);
      if (list) {
        nsCOMPtr<nsIDOMElement> element;
        list->GetSelectedItem(getter_AddRefs(element));
        if (element) {
          nsCOMPtr<nsIContent> selectedContent = do_QueryInterface(element);
          nsIFrame* curr;
          menuPopup->FirstChild(mPresContext, nsnull, &curr);
          while (curr) {
            nsCOMPtr<nsIContent> child;
            curr->GetContent(getter_AddRefs(child));
            if (selectedContent == child) {
              nsCOMPtr<nsIMenuFrame> menuframe(do_QueryInterface(curr));
              if (menuframe)
                menuPopup->SetCurrentMenuItem(menuframe);
            }
            curr->GetNextSibling(&curr);
          }
        }
      }
      // End of menulist stuff.  We now return to our regularly scheduled
      // programming.

      // Sync up the view.
      nsAutoString popupAnchor, popupAlign;
      
      menuPopupContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::popupanchor, popupAnchor);
      menuPopupContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::popupalign, popupAlign);

      
      if (onMenuBar) {
        if (popupAnchor == "")
          popupAnchor = "bottomleft";
        if (popupAlign == "")
          popupAlign = "topleft";
      }
      else {
        if (popupAnchor == "")
          popupAnchor = "topright";
        if (popupAlign == "")
          popupAlign = "topleft";
      }

      menuPopup->SyncViewWithFrame(mPresContext, popupAnchor, popupAlign, this, -1, -1);
    }

    nsCOMPtr<nsIMenuParent> childPopup = do_QueryInterface(frame);
    UpdateDismissalListener(childPopup);

    mMenuOpen = PR_TRUE;

    // Set the focus back to our view's widget.
    if (nsMenuFrame::mDismissalListener)
      nsMenuFrame::mDismissalListener->EnableListener(PR_TRUE);
    
  }
  else {
    // Close the menu. 
    // Execute the ondestroy handler
    if (!OnDestroy())
      return;

    // Set the focus back to our view's widget.
    if (nsMenuFrame::mDismissalListener) {
      nsMenuFrame::mDismissalListener->EnableListener(PR_FALSE);
      nsMenuFrame::mDismissalListener->SetCurrentMenuParent(mMenuParent);
    }

    nsIFrame* frame = mPopupFrames.FirstChild();
    nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  
    // Make sure we clear out our own items.
    if (menuPopup) {
      menuPopup->SetCurrentMenuItem(nsnull);
      menuPopup->KillCloseTimer();

      PRBool onMenuBar = PR_TRUE;
      if (mMenuParent)
        mMenuParent->IsMenuBar(onMenuBar);

      if (mMenuParent && onMenuBar)
        mMenuParent->RemoveKeyboardNavigator();
      else if (!mMenuParent)
        menuPopup->RemoveKeyboardNavigator();
    }

    ActivateMenu(PR_FALSE);

    mMenuOpen = PR_FALSE;
/*
    nsIView*  view;
    GetView(&view);
    if (!view) {
      nsPoint offset;
      GetOffsetFromView(offset, &view);
    }
    nsCOMPtr<nsIWidget> widget;
    view->GetWidget(*getter_AddRefs(widget));
    widget->SetFocus();
*/
    if (nsMenuFrame::mDismissalListener)
      nsMenuFrame::mDismissalListener->EnableListener(PR_TRUE);
  }
}

void
nsMenuFrame::GetMenuChildrenElement(nsIContent** aResult)
{
  PRInt32 count;
  mContent->ChildCount(count);

  for (PRInt32 i = 0; i < count; i++) {
    nsCOMPtr<nsIContent> child;
    mContent->ChildAt(i, *getter_AddRefs(child));
    nsCOMPtr<nsIAtom> tag;
    child->GetTag(*getter_AddRefs(tag));
    if (tag && tag.get() == nsXULAtoms::menupopup) {
      *aResult = child.get();
      NS_ADDREF(*aResult);
      return;
    }
  }
}

NS_IMETHODIMP
nsMenuFrame::Reflow(nsIPresContext*   aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  //NS_ASSERTION(aReflowState.reason != eReflowReason_Incremental,"Incremental Reflow not supported!");

  nsIFrame* popupChild = mPopupFrames.FirstChild();

  nsHTMLReflowState boxState(aReflowState);

  if (aReflowState.reason == eReflowReason_Incremental) {
    nsIFrame* incrementalChild;

    // get the child but don't pull it off
    aReflowState.reflowCommand->GetNext(incrementalChild, PR_FALSE);
    
    // see if it is in the mPopupFrames list
    nsIFrame* child = mPopupFrames.FirstChild();
    popupChild = nsnull;

    while (nsnull != child) 
    { 
      // if it is then flow the popup incrementally then flow
      // us with a resize just to get our correct desired size.
      if (child == incrementalChild) {
        // pull it off now
        aReflowState.reflowCommand->GetNext(incrementalChild);

        // we know what child
        popupChild = child;

        // relow the box with resize just to get the
        // aDesiredSize set correctly
        boxState.reason = eReflowReason_Resize;
        break;
      }

      nsresult rv = child->GetNextSibling(&child);
      NS_ASSERTION(rv == NS_OK,"failed to get next child");
    }   
  } 

  // If we're a menulist AND if we're intrinsically sized, then
  // we need to flow our popup and use its width as our own width.
  PRBool constrainMenuWidth = PR_FALSE;
  nsCOMPtr<nsIDOMXULMenuListElement> menulist = do_QueryInterface(mContent);
  if (menulist && (aReflowState.mComputedWidth == NS_UNCONSTRAINEDSIZE)) {
    constrainMenuWidth = PR_TRUE;
    MarkAsGenerated();
  }

  // Handle reflowing our subordinate popup
  nsHTMLReflowMetrics kidDesiredSize(aDesiredSize);  
  if (popupChild) {         
      // Constrain the child's width and height to aAvailableWidth and aAvailableHeight
      nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState, popupChild,
                                       availSize);
      kidReflowState.mComputedWidth = NS_UNCONSTRAINEDSIZE;
      kidReflowState.mComputedHeight = NS_UNCONSTRAINEDSIZE;
      
      nsRect rect;
      popupChild->GetRect(rect);
      nsresult rv = ReflowChild(popupChild, aPresContext, kidDesiredSize, kidReflowState,
                       rect.x, rect.y, NS_FRAME_NO_MOVE_VIEW, aStatus);

      // Set the child's width and height to its desired size
      // Note: don't position or size the view now, we'll do that in the
      // DidReflow() function
      FinishReflowChild(popupChild, aPresContext, kidDesiredSize, rect.x, rect.y, NS_FRAME_NO_MOVE_VIEW);
  }

  if (constrainMenuWidth) {
    boxState.mComputedWidth = kidDesiredSize.width;
  }

  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, boxState, aStatus);

  // If we're a menulist, then we might potentially flow the popup a second time
  // (its width may be too small).
  if (menulist && popupChild) {
    if (kidDesiredSize.width < aDesiredSize.width) {
      // Flow the popup again.
      // Constrain the child's width and height to aAvailableWidth and aAvailableHeight
      nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState, popupChild,
                                       availSize);
      kidReflowState.mComputedWidth = aDesiredSize.width;
      kidReflowState.mComputedHeight = NS_UNCONSTRAINEDSIZE;
    
      nsRect rect;
      popupChild->GetRect(rect);
      nsresult rv = ReflowChild(popupChild, aPresContext, kidDesiredSize, kidReflowState,
                       rect.x, rect.y, NS_FRAME_NO_MOVE_VIEW, aStatus);

      // Set the child's width and height to its desired size
      // Note: don't position or size the view now, we'll do that in the
      // DidReflow() function
      FinishReflowChild(popupChild, aPresContext, kidDesiredSize, rect.x, rect.y, NS_FRAME_NO_MOVE_VIEW);
    }
  }
 
  return rv;
}


NS_IMETHODIMP
nsMenuFrame::DidReflow(nsIPresContext* aPresContext,
                            nsDidReflowStatus aStatus)
{
  nsresult rv;
  rv = nsBoxFrame::DidReflow(aPresContext, aStatus);

  // Sync up the view.
  nsIFrame* frame = mPopupFrames.FirstChild();
  nsMenuPopupFrame* menuPopup = (nsMenuPopupFrame*)frame;
  if (mMenuOpen && menuPopup) {
    nsCOMPtr<nsIContent> menuPopupContent;
    menuPopup->GetContent(getter_AddRefs(menuPopupContent));
    nsAutoString popupAnchor, popupAlign;
      
    menuPopupContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::popupanchor, popupAnchor);
    menuPopupContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::popupalign, popupAlign);

    PRBool onMenuBar = PR_TRUE;
    if (mMenuParent)
      mMenuParent->IsMenuBar(onMenuBar);

    if (onMenuBar) {
      if (popupAnchor == "")
          popupAnchor = "bottomleft";
      if (popupAlign == "")
          popupAlign = "topleft";
    }
    else {
      if (popupAnchor == "")
        popupAnchor = "topright";
      if (popupAlign == "")
        popupAlign = "topleft";
    }

    menuPopup->SyncViewWithFrame(aPresContext, popupAnchor, popupAlign, this, -1, -1);
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->ShortcutNavigation(aLetter, aHandledFlag);
  } 

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->KeyboardNavigation(aDirection, aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Escape(PRBool& aHandledFlag)
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Escape(aHandledFlag);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::Enter()
{
  if (!mMenuOpen) {
    // The enter key press applies to us.
    if (!IsMenu() && mMenuParent) {
      // Execute our event handler
      Execute();
    }
    else {
      OpenMenu(PR_TRUE);
      SelectFirstItem();
    }

    return NS_OK;
  }

  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    popup->Enter();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuFrame::SelectFirstItem()
{
  nsIFrame* frame = mPopupFrames.FirstChild();
  if (frame) {
    nsMenuPopupFrame* popup = (nsMenuPopupFrame*)frame;
    nsIMenuFrame* result;
    popup->GetNextMenuItem(nsnull, &result);
    popup->SetCurrentMenuItem(result);
  }

  return NS_OK;
}

PRBool
nsMenuFrame::IsMenu()
{
  nsCOMPtr<nsIAtom> tag;
  mContent->GetTag(*getter_AddRefs(tag));
  if (tag.get() == nsXULAtoms::menu)
    return PR_TRUE;
  return PR_FALSE;
}

NS_IMETHODIMP_(void)
nsMenuFrame::Notify(nsITimer* aTimer)
{
  // Our timer has fired.
  if (aTimer == mOpenTimer.get()) {
    if (!mMenuOpen && mMenuParent) {
      nsAutoString active = "";
      mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::menuactive, active);
      if (active == "true") {
        // We're still the active menu.
        OpenMenu(PR_TRUE);
      }
    }
    mOpenTimer->Cancel();
    mOpenTimer = nsnull;
  }
  
  mOpenTimer = nsnull;
}

PRBool 
nsMenuFrame::IsDisabled()
{
  nsAutoString disabled;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled == "true")
    return PR_TRUE;
  return PR_FALSE;
}

void
nsMenuFrame::UpdateMenuType(nsIPresContext* aPresContext)
{
  nsAutoString value;
  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::type, value);
  if (value == "checkbox")
    mType = eMenuType_Checkbox;
  else if (value == "radio") {
    mType = eMenuType_Radio;

    nsAutoString value;
    mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::name, value);
    if ( mGroupName != value )
      mGroupName = value;
  } 
  else {
    if (mType != eMenuType_Normal)
      mContent->UnsetAttribute(kNameSpaceID_None, nsHTMLAtoms::checked,
                               PR_TRUE);
    mType = eMenuType_Normal;
  }
  UpdateMenuSpecialState(aPresContext);
}

/* update checked-ness for type="checkbox" and type="radio" */
void
nsMenuFrame::UpdateMenuSpecialState(nsIPresContext* aPresContext) {
  nsAutoString value;
  PRBool newChecked;

  mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::checked,
                         value);
  newChecked = (value == "true");

  if (newChecked == mChecked) {
    /* checked state didn't change */

    if (mType != eMenuType_Radio)
      return; // only Radio possibly cares about other kinds of change

    mContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::name, value);
    if (value == mGroupName)
      return;                   // no interesting change
  } else { 
    mChecked = newChecked;
    if (mType != eMenuType_Radio || !mChecked)
      /*
       * Unchecking something requires no further changes, and only
       * menuRadio has to do additional work when checked.
       */
      return;
  }

  /*
   * If we get this far, we're type=radio, and:
   * - our name= changed, or
   * - we went from checked="false" to checked="true"
   */

  if (!mChecked)
    /*
     * If we're not checked, then it must have been a name change, and a name
     * change on an unchecked item doesn't require any magic.
     */
    return;
  
  /*
   * Behavioural note:
   * If we're checked and renamed _into_ an existing radio group, we are
   * made the new checked item, and we unselect the previous one.
   *
   * The only other reasonable behaviour would be to check for another selected
   * item in that group.  If found, unselect ourselves, otherwise we're the
   * selected item.  That, however, would be a lot more work, and I don't think
   * it's better at all.
   */

  /* walk siblings, looking for the other checked item with the same name */
  nsIFrame *parent, *sib;
  nsIMenuFrame *sibMenu;
  nsMenuType sibType;
  nsAutoString sibGroup;
  PRBool sibChecked;
  
  nsresult rv = GetParent(&parent);
  NS_ASSERTION(NS_SUCCEEDED(rv), "couldn't get parent of radio menu frame\n");
  if (NS_FAILED(rv)) return;
  
  // get the first sibling in this menu popup. This frame may be it, and if we're
  // being called at creation time, this frame isn't yet in the parent's child list.
  // All I'm saying is that this may fail, but it's most likely alright.
  rv = parent->FirstChild(aPresContext, NULL, &sib);
  if ( NS_FAILED(rv) || !sib )
    return;

  do {
    if (NS_FAILED(sib->QueryInterface(NS_GET_IID(nsIMenuFrame),
                                      (void **)&sibMenu)))
        continue;
        
    if (sibMenu != (nsIMenuFrame *)this &&        // correct way to check?
        (sibMenu->GetMenuType(sibType), sibType == eMenuType_Radio) &&
        (sibMenu->MenuIsChecked(sibChecked), sibChecked) &&
        (sibMenu->GetRadioGroupName(sibGroup), sibGroup == mGroupName)) {
      
      nsCOMPtr<nsIContent> content;
      if (NS_FAILED(sib->GetContent(getter_AddRefs(content))))
        continue;             // break?
      
      /* uncheck the old item */
      content->UnsetAttribute(kNameSpaceID_None, nsHTMLAtoms::checked,
                              PR_TRUE);

      /* XXX in DEBUG, check to make sure that there aren't two checked items */
      return;
    }

  } while(NS_SUCCEEDED(sib->GetNextSibling(&sib)) && sib);

}

NS_IMETHODIMP
nsMenuFrame::CreateAnonymousContent(nsIPresContext* aPresContext, nsISupportsArray& aAnonymousChildren)
{
  return NS_OK;
}


void 
nsMenuFrame::BuildAcceleratorText(nsString& aAccelString)
{
  nsString accelText;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::acceltext, accelText);
  if (accelText != "") {
    // Just use this.
    aAccelString = accelText;
    return;
  }

  // See if we have a key node and use that instead.
  nsString keyValue;
  mContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::key, keyValue);

  nsCOMPtr<nsIDocument> document;
  mContent->GetDocument(*getter_AddRefs(document));

  // Turn the document into a XUL document so we can use getElementById
  nsCOMPtr<nsIDOMXULDocument> xulDocument = do_QueryInterface(document);
  if (!xulDocument)
    return;

  nsCOMPtr<nsIDOMElement> keyElement;
  xulDocument->GetElementById(keyValue, getter_AddRefs(keyElement));
  
  if (!keyElement)
    return;
  
  nsAutoString keyAtom("key");
  nsAutoString shiftAtom("shift");
	nsAutoString altAtom("alt");
	nsAutoString commandAtom("command");
  nsAutoString controlAtom("control");

	nsString shiftValue;
	nsString altValue;
	nsString commandValue;
  nsString controlValue;
	nsString keyChar = " ";
	
	keyElement->GetAttribute(keyAtom, keyChar);
	keyElement->GetAttribute(shiftAtom, shiftValue);
	keyElement->GetAttribute(altAtom, altValue);
	keyElement->GetAttribute(commandAtom, commandValue);
  keyElement->GetAttribute(controlAtom, controlValue);
	  
  nsAutoString xulkey;
  keyElement->GetAttribute(nsAutoString("xulkey"), xulkey);
  if (xulkey == "true") {
      // Set the default for the xul key modifier
#ifdef XP_MAC
      commandValue = "true";
#elif defined(XP_PC) || defined(NTO)
      controlValue = "true";
#else 
      altValue = "true";
#endif
  }
  
  PRBool prependPlus = PR_FALSE;

  if(commandValue != "" && commandValue != "false") {
    prependPlus = PR_TRUE;
	  aAccelString += "Ctrl"; // Hmmm. Kinda defeats the point of having an abstraction.
  }

  if(controlValue != "" && controlValue != "false") {
    prependPlus = PR_TRUE;
	  aAccelString += "Ctrl";
  }

  if(shiftValue != "" && shiftValue != "false") {
    if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += "Shift";
  }

  if (altValue != "" && altValue != "false") {
	  if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += "Alt";
  }

  keyChar.ToUpperCase();
  if (keyChar != "") {
    if (prependPlus)
      aAccelString += "+";
    prependPlus = PR_TRUE;
    aAccelString += keyChar;
  }

  if (aAccelString != "")
    mContent->SetAttribute(kNameSpaceID_None, nsXULAtoms::acceltext, aAccelString, PR_FALSE);
}

void
nsMenuFrame::Execute()
{
  // Temporarily disable rollup events on this menu.  This is
  // to suppress this menu getting removed in the case where
  // the oncommand handler opens a dialog, etc.
  if ( nsMenuFrame::mDismissalListener ) {
    nsMenuFrame::mDismissalListener->EnableListener(PR_FALSE);
  }

  // Get our own content node and hold on to it to keep it from going away.
  nsCOMPtr<nsIContent> content = dont_QueryInterface(mContent);

  // First hide all of the open menus.
  if (mMenuParent)
    mMenuParent->HideChain();

  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_ACTION;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  mContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);

  // XXX HACK. Just gracefully exit if the node has been removed, e.g., window.close()
  // was executed.
  nsCOMPtr<nsIDocument> doc;
  content->GetDocument(*getter_AddRefs(doc));

  // Now properly close them all up.
  if (doc && mMenuParent)
    mMenuParent->DismissChain();

  // Re-enable rollup events on this menu.
  if ( nsMenuFrame::mDismissalListener ) {
	nsMenuFrame::mDismissalListener->EnableListener(PR_TRUE);
  }
}

PRBool
nsMenuFrame::OnCreate()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_CREATE;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  if (child) 
    rv = child->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  else rv = mContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

PRBool
nsMenuFrame::OnDestroy()
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event;
  event.eventStructType = NS_EVENT;
  event.message = NS_MENU_DESTROY;
  event.isShift = PR_FALSE;
  event.isControl = PR_FALSE;
  event.isAlt = PR_FALSE;
  event.isMeta = PR_FALSE;
  event.clickCount = 0;
  event.widget = nsnull;
  
  nsCOMPtr<nsIContent> child;
  GetMenuChildrenElement(getter_AddRefs(child));
  
  nsresult rv;
  if (child) 
    rv = child->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);
  else rv = mContent->HandleDOMEvent(mPresContext, &event, nsnull, NS_EVENT_FLAG_INIT, &status);

  if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
    return PR_FALSE;
  return PR_TRUE;
}

NS_IMETHODIMP
nsMenuFrame::RemoveFrame(nsIPresContext* aPresContext,
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
nsMenuFrame::InsertFrames(nsIPresContext* aPresContext,
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
  if (tag && tag.get() == nsXULAtoms::menupopup) {
    mPopupFrames.InsertFrames(nsnull, nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::InsertFrames(aPresContext, aPresShell, aListName, aPrevFrame, aFrameList);  
  }

  return rv;
}

NS_IMETHODIMP
nsMenuFrame::AppendFrames(nsIPresContext* aPresContext,
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
  if (tag && tag.get() == nsXULAtoms::menupopup) {
    mPopupFrames.AppendFrames(nsnull, aFrameList);
    rv = GenerateDirtyReflowCommand(aPresContext, aPresShell);
  } else {
    rv = nsBoxFrame::AppendFrames(aPresContext, aPresShell, aListName, aFrameList); 
  }

  return rv;
}

void
nsMenuFrame::UpdateDismissalListener(nsIMenuParent* aMenuParent)
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

NS_IMETHODIMP
nsMenuFrame::GetBoxInfo(nsIPresContext* aPresContext, const nsHTMLReflowState& aReflowState, nsBoxInfo& aSize)
{
  nsresult rv = nsBoxFrame::GetBoxInfo(aPresContext, aReflowState, aSize);
  nsCOMPtr<nsIDOMXULMenuListElement> menulist(do_QueryInterface(mContent));
  if (menulist) {
    nsCalculatedBoxInfo boxInfo;
    boxInfo.frame = this;
    boxInfo.prefSize.width = NS_UNCONSTRAINEDSIZE;
    boxInfo.prefSize.height = NS_UNCONSTRAINEDSIZE;
    boxInfo.flex = 0;
    GetRedefinedMinPrefMax(aPresContext, this, boxInfo);
    if (boxInfo.prefSize.width == NS_UNCONSTRAINEDSIZE &&
        boxInfo.prefSize.height == NS_UNCONSTRAINEDSIZE &&
        boxInfo.flex == 0) {
      nsIFrame* frame = mPopupFrames.FirstChild();
      if (!frame) {
        MarkAsGenerated();
        frame = mPopupFrames.FirstChild();
      }
      
      nsCOMPtr<nsIBox> box(do_QueryInterface(frame));
      nsCalculatedBoxInfo childInfo;
      childInfo.frame = frame;
      box->GetBoxInfo(aPresContext, aReflowState, childInfo);
      GetRedefinedMinPrefMax(aPresContext, this, childInfo);
      aSize.prefSize.width = childInfo.prefSize.width;
    }
  }
  return rv;
}

