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

#include "nsMenuListener.h"
#include "nsMenuBarFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIDOMEventReceiver.h"
#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsMenuFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsMenuPopupFrame.h"

static NS_DEFINE_IID(kIFrameIID, NS_IFRAME_IID);

//
// NS_NewMenuBarFrame
//
// Wrapper for creating a new menu Bar container
//
nsresult
NS_NewMenuBarFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsMenuBarFrame* it = new (aPresShell) nsMenuBarFrame;
  if ( !it )
    return NS_ERROR_OUT_OF_MEMORY;
  *aNewFrame = it;
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuBarFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsMenuBarFrame::Release(void)
{
    return NS_OK;
}


//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsMenuBarFrame)
  NS_INTERFACE_MAP_ENTRY(nsIMenuParent)
NS_INTERFACE_MAP_END_INHERITING(nsToolbarFrame)


//
// nsMenuBarFrame cntr
//
nsMenuBarFrame::nsMenuBarFrame()
:mIsActive(PR_FALSE), mTarget(nsnull), mKeyboardNavigator(nsnull), mMenuBarListener(nsnull)
{

} // cntr

nsMenuBarFrame::~nsMenuBarFrame()
{
}

NS_IMETHODIMP
nsMenuBarFrame::Init(nsIPresContext*  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsToolbarFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  // XXX hack
  mPresContext = aPresContext;

  // Create the menu bar listener.
  mMenuBarListener = new nsMenuBarListener(this);
  NS_IF_ADDREF(mMenuBarListener);
  if (! mMenuBarListener)
    return NS_ERROR_OUT_OF_MEMORY;

  // Hook up the menu bar as a key listener on the whole document.  It will see every
  // key press that occurs, but after everyone else does.
  nsCOMPtr<nsIDocument> doc;
  aContent->GetDocument(*getter_AddRefs(doc));
  nsCOMPtr<nsIDOMEventReceiver> target = do_QueryInterface(doc);
  
  mTarget = target;

  // Also hook up the listener to the window listening for focus events. This is so we can keep proper
  // state as the user alt-tabs through processes.
  
  target->AddEventListener("blur", (nsIDOMFocusListener*)mMenuBarListener, PR_FALSE);
  
  target->AddEventListener("mousedown", (nsIDOMMouseListener*)mMenuBarListener, PR_FALSE);

  target->AddEventListener("keypress", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE); 
  target->AddEventListener("keydown", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);  
  target->AddEventListener("keyup", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);   
  
  return rv;
}

NS_IMETHODIMP
nsMenuBarFrame::IsOpen()
{
  PRBool isOpen = PR_FALSE;
  if(mCurrentMenu) {
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      return PR_TRUE;
	}
  }
  return PR_FALSE;
}


NS_IMETHODIMP
nsMenuBarFrame::SetActive(PRBool aActiveFlag)
{
  mIsActive = aActiveFlag;
  if (mIsActive) {
    InstallKeyboardNavigator();
  }
  else RemoveKeyboardNavigator();

  return NS_OK;
}

void
nsMenuBarFrame::ToggleMenuActiveState()
{
  if (mIsActive) {
    // Deactivate the menu bar
    mIsActive = PR_FALSE;
    if (mCurrentMenu) {
      // Deactivate the menu.
      mCurrentMenu->OpenMenu(PR_FALSE);
      mCurrentMenu->SelectMenu(PR_FALSE);
      mCurrentMenu = nsnull;
      RemoveKeyboardNavigator();
    }
  }
  else {
    // Activate the menu bar
    SetActive(PR_TRUE);

    InstallKeyboardNavigator();

    // Set the active menu to be the top left item (e.g., the File menu).
    // We use an attribute called "active" to track the current active menu.
    nsCOMPtr<nsIContent> firstMenuItem;
    nsIMenuFrame* firstFrame;
    GetNextMenuItem(nsnull, &firstFrame);
    if (firstFrame) {
      firstFrame->SelectMenu(PR_TRUE);
      
      // Track this item for keyboard navigation.
      mCurrentMenu = firstFrame;
    }
  }
}

nsIMenuFrame*
nsMenuBarFrame::FindMenuWithShortcut(PRUint32 aLetter)
{
  // Enumerate over our list of frames.
  nsIFrame* currFrame = mFrames.FirstChild();
  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      // Get the shortcut attribute.
      nsString shortcutKey = "";
      current->GetAttribute(kNameSpaceID_None, nsXULAtoms::accesskey, shortcutKey);
      shortcutKey.ToUpperCase();
      if (shortcutKey.Length() > 0) {
        // We've got something.
        PRUnichar shortcutChar = shortcutKey.CharAt(0);
        if (shortcutChar == aLetter) {
          // We match!
          nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
          if (menuFrame)
            return menuFrame.get();
          return nsnull;
        }
      }
    }
    currFrame->GetNextSibling(&currFrame);
  }
  return nsnull;
}

NS_IMETHODIMP 
nsMenuBarFrame::ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag)
{
  if (mCurrentMenu) {
    PRBool isOpen = PR_FALSE;
    mCurrentMenu->MenuIsOpen(isOpen);
    if (isOpen) {
      // No way this applies to us. Give it to our child.
      mCurrentMenu->ShortcutNavigation(aLetter, aHandledFlag);
      return NS_OK;
    }
  }

  // This applies to us. Let's see if one of the shortcuts applies
  nsIMenuFrame* result = FindMenuWithShortcut(aLetter);
  if (result) {
    // We got one!
    aHandledFlag = PR_TRUE;
    mIsActive = PR_TRUE;
    SetCurrentMenuItem(result);
    result->OpenMenu(PR_TRUE);
    result->SelectFirstItem();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag)
{
  if (!mCurrentMenu)
    return NS_OK;
  
  PRBool isContainer = PR_FALSE;
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsContainer(isContainer);
  mCurrentMenu->MenuIsOpen(isOpen);

  aHandledFlag = PR_FALSE;
  
  if (isOpen) {
    // Let the child menu try to handle it.
    mCurrentMenu->KeyboardNavigation(aDirection, aHandledFlag);
  }

  if (aHandledFlag)
    return NS_OK;

  if (aDirection == NS_VK_RIGHT || aDirection == NS_VK_LEFT) {
    
    nsIMenuFrame* nextItem;
    
    if (aDirection == NS_VK_RIGHT)
      GetNextMenuItem(mCurrentMenu, &nextItem);
    else GetPreviousMenuItem(mCurrentMenu, &nextItem);

    SetCurrentMenuItem(nextItem);
    if (nextItem) {
      PRBool nextIsOpen;
      nextItem->MenuIsOpen(nextIsOpen);
      if (nextIsOpen) {
        // Select the first item.
        nextItem->SelectFirstItem();
      }
    }
  }
  else if (aDirection == NS_VK_UP || aDirection == NS_VK_DOWN) {
    // Open the menu and select its first item.
    mCurrentMenu->OpenMenu(PR_TRUE);
    mCurrentMenu->SelectFirstItem();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::GetNextMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult)
{
  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(kIFrameIID, (void**)&currFrame); 
    if (currFrame) {
      startFrame = currFrame;
      currFrame->GetNextSibling(&currFrame);
    }
  }
  else currFrame = mFrames.FirstChild();

  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));

    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }
    currFrame->GetNextSibling(&currFrame);
  }

  currFrame = mFrames.FirstChild();

  // Still don't have anything. Try cycling from the beginning.
  while (currFrame && currFrame != startFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }

    currFrame->GetNextSibling(&currFrame);
  }

  // No luck. Just return our start value.
  *aResult = aStart;
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::GetPreviousMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult)
{
  nsIFrame* currFrame = nsnull;
  nsIFrame* startFrame = nsnull;
  if (aStart) {
    aStart->QueryInterface(kIFrameIID, (void**)&currFrame);
    if (currFrame) {
      startFrame = currFrame;
      currFrame = mFrames.GetPrevSiblingFor(currFrame);
    }
  }
  else currFrame = mFrames.LastChild();

  while (currFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));

    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }
    currFrame = mFrames.GetPrevSiblingFor(currFrame);
  }

  currFrame = mFrames.LastChild();

  // Still don't have anything. Try cycling from the end.
  while (currFrame && currFrame != startFrame) {
    nsCOMPtr<nsIContent> current;
    currFrame->GetContent(getter_AddRefs(current));
    
    // See if it's a menu item.
    if (IsValidItem(current)) {
      nsCOMPtr<nsIMenuFrame> menuFrame = do_QueryInterface(currFrame);
      *aResult = menuFrame.get();
      NS_IF_ADDREF(*aResult);
      return NS_OK;
    }

    currFrame = mFrames.GetPrevSiblingFor(currFrame);
  }

  // No luck. Just return our start value.
  *aResult = aStart;
  return NS_OK;
}

NS_IMETHODIMP nsMenuBarFrame::SetCurrentMenuItem(nsIMenuFrame* aMenuItem)
{
  if (mCurrentMenu == aMenuItem)
    return NS_OK;

  PRBool wasOpen = PR_FALSE;
  
  // Unset the current child.
  if (mCurrentMenu) {
    mCurrentMenu->MenuIsOpen(wasOpen);
    mCurrentMenu->SelectMenu(PR_FALSE);
    if (wasOpen)
      mCurrentMenu->OpenMenu(PR_FALSE);
  }

  // Set the new child.
  if (aMenuItem) {
    aMenuItem->SelectMenu(PR_TRUE);
    aMenuItem->MarkAsGenerated(); // Have the menu building. Get it ready to be shown.

    if (wasOpen)
      aMenuItem->OpenMenu(PR_TRUE);
  }

  mCurrentMenu = aMenuItem;

  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarFrame::Escape(PRBool& aHandledFlag)
{
  if (!mCurrentMenu)
    return NS_OK;

  // See if our menu is open.
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    // Let the child menu handle this.
    aHandledFlag = PR_FALSE;
    mCurrentMenu->Escape(aHandledFlag);
    if (!aHandledFlag) {
      // Close up this menu but keep our current menu item
      // designation.
      mCurrentMenu->OpenMenu(PR_FALSE);
    }
	if (nsMenuFrame::mDismissalListener)
      nsMenuFrame::mDismissalListener->Unregister();
    return NS_OK;
  }

  // It's us. Just set our active flag to false.
  mIsActive = PR_FALSE;

  // Clear our current menu item if we've got one.
  SetCurrentMenuItem(nsnull);

  // Clear out our dismissal listener
  if (nsMenuFrame::mDismissalListener)
    nsMenuFrame::mDismissalListener->Unregister();

  return NS_OK;
}

NS_IMETHODIMP 
nsMenuBarFrame::Enter()
{
  if (!mCurrentMenu)
    return NS_OK;

  // See if our menu is open.
  PRBool isOpen = PR_FALSE;
  mCurrentMenu->MenuIsOpen(isOpen);
  if (isOpen) {
    // Let the child menu handle this.
    mCurrentMenu->Enter();
    return NS_OK;
  }

  // It's us. Open the current menu.
  mCurrentMenu->OpenMenu(PR_TRUE);
  mCurrentMenu->SelectFirstItem();

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::HideChain()
{
  // Stop capturing rollups
  // (must do this during Hide, which happens before the menu item is executed,
  // since this reinstates normal event handling.)
  if (nsMenuFrame::mDismissalListener)
    nsMenuFrame::mDismissalListener->Unregister();

  if (mCurrentMenu) {
    mCurrentMenu->ActivateMenu(PR_FALSE);
    mCurrentMenu->SelectMenu(PR_FALSE);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::DismissChain()
{
  // Stop capturing rollups
  if (nsMenuFrame::mDismissalListener)
    nsMenuFrame::mDismissalListener->Unregister();
  
  SetCurrentMenuItem(nsnull);
  SetActive(PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::GetWidget(nsIWidget **aWidget)
{
  // Get parent view
  nsIView * view = nsnull;
  nsMenuPopupFrame::GetNearestEnclosingView(mPresContext, this, &view);
  if (!view)
    return NS_OK;

  view->GetWidget(*aWidget);
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::CreateDismissalListener()
{
  NS_ADDREF(nsMenuFrame::mDismissalListener = new nsMenuDismissalListener());
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::InstallKeyboardNavigator()
{
  if (mKeyboardNavigator)
    return NS_OK;

  mKeyboardNavigator = new nsMenuListener(this);
  NS_IF_ADDREF(mKeyboardNavigator);

  mTarget->AddEventListener("keypress", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE); 
  mTarget->AddEventListener("keydown", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);  
  mTarget->AddEventListener("keyup", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);   
  
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarFrame::RemoveKeyboardNavigator()
{
  if (!mKeyboardNavigator)
    return NS_OK;

  mTarget->RemoveEventListener("keypress", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener("keydown", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  mTarget->RemoveEventListener("keyup", (nsIDOMKeyListener*)mKeyboardNavigator, PR_TRUE);
  
  NS_IF_RELEASE(mKeyboardNavigator);

  return NS_OK;
}

// helpers ///////////////////////////////////////////////////////////

PRBool 
nsMenuBarFrame::IsValidItem(nsIContent* aContent)
{
  nsCOMPtr<nsIAtom> tag;
  aContent->GetTag(*getter_AddRefs(tag));
  if (tag && (tag.get() == nsXULAtoms::menu ||
              tag.get() == nsXULAtoms::menuitem) &&
      !IsDisabled(aContent))
      return PR_TRUE;

  return PR_FALSE;
}

PRBool 
nsMenuBarFrame::IsDisabled(nsIContent* aContent)
{
  nsString disabled = "";
  aContent->GetAttribute(kNameSpaceID_None, nsHTMLAtoms::disabled, disabled);
  if (disabled == "true")
    return PR_TRUE;
  return PR_FALSE;
}

NS_IMETHODIMP
nsMenuBarFrame::Destroy(nsIPresContext* aPresContext)
{
  mTarget->RemoveEventListener("blur", (nsIDOMFocusListener*)mMenuBarListener, PR_FALSE); 
  
  mTarget->RemoveEventListener("mousedown", (nsIDOMMouseListener*)mMenuBarListener, PR_FALSE); 

  mTarget->RemoveEventListener("keypress", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE); 
  mTarget->RemoveEventListener("keydown", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);  
  mTarget->RemoveEventListener("keyup", (nsIDOMKeyListener*)mMenuBarListener, PR_FALSE);

  NS_IF_RELEASE(mMenuBarListener);

  return nsToolbarFrame::Destroy(aPresContext);
}
