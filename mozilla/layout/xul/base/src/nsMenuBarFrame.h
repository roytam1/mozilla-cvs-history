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

//
// nsMenuBarFrame
//

#ifndef nsMenuBarFrame_h__
#define nsMenuBarFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsToolbarFrame.h"
#include "nsMenuBarListener.h"
#include "nsIMenuParent.h"
#include "nsIWidget.h"

class nsIContent;
class nsIMenuFrame;

nsresult NS_NewMenuBarFrame(nsIFrame** aResult) ;

class nsMenuBarFrame : public nsToolbarFrame, public nsIMenuParent
{
public:
  nsMenuBarFrame();
  ~nsMenuBarFrame();

  NS_DECL_ISUPPORTS

  // nsIMenuParentInterface
  NS_IMETHOD SetCurrentMenuItem(nsIMenuFrame* aMenuItem);
  NS_IMETHOD GetNextMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult);
  NS_IMETHOD GetPreviousMenuItem(nsIMenuFrame* aStart, nsIMenuFrame** aResult);
  NS_IMETHOD SetActive(PRBool aActiveFlag); 
  NS_IMETHOD GetIsActive(PRBool& isActive) { isActive = IsActive(); return NS_OK; };
  NS_IMETHOD IsMenuBar(PRBool& isMenuBar) { isMenuBar = PR_TRUE; return NS_OK; };

  NS_IMETHOD IsActive() { return mIsActive; };

  // Closes up the chain of open cascaded menus.
  NS_IMETHOD DismissChain();

  // Hides the chain of cascaded menus without closing them up.
  NS_IMETHOD HideChain();

  NS_IMETHOD GetWidget(nsIWidget **aWidget);
  // The dismissal listener gets created and attached to the window.
  NS_IMETHOD CreateDismissalListener();

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD Destroy(nsIPresContext& aPresContext);

// Non-interface helpers

  // Called when a menu on the menu bar is clicked on.
  void ToggleMenuActiveState();
  
  // Used to move up, down, left, and right in menus.
  void KeyboardNavigation(PRUint32 aDirection);
  
  // Used to handle ALT+key combos
  void ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag);
  nsIMenuFrame* FindMenuWithShortcut(PRUint32 aLetter);

  // Called when the ESC key is held down to close levels of menus.
  void Escape();

  // Called to execute a menu item.
  void Enter();

  PRBool IsValidItem(nsIContent* aContent);
  PRBool IsDisabled(nsIContent* aContent);

protected:
  nsMenuBarListener* mMenuBarListener; // The listener that tells us about key and mouse events.
  PRBool mIsActive; // Whether or not the menu bar is active (a menu item is highlighted or shown).
  nsIMenuFrame* mCurrentMenu; // The current menu that is active.

  nsIDOMEventReceiver* mTarget;

  // XXX Hack
  nsIPresContext* mPresContext;  // weak reference

}; // class nsMenuBarFrame

#endif
