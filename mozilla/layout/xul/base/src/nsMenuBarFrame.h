/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
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

class nsIContent;

nsresult NS_NewMenuBarFrame(nsIFrame** aResult) ;

class nsMenuBarFrame : public nsToolbarFrame, public nsIMenuParent
{
public:
  nsMenuBarFrame();

  NS_DECL_ISUPPORTS

  // nsIMenuParentInterface
  NS_IMETHOD SetCurrentMenuItem(nsIFrame* aMenuItem);
  NS_IMETHOD GetNextMenuItem(nsIFrame* aStart, nsIFrame** aResult);
  NS_IMETHOD GetPreviousMenuItem(nsIFrame* aStart, nsIFrame** aResult);
  NS_IMETHOD SetActive(); 

  NS_IMETHOD IsActive() { return mIsActive; };

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

// Non-interface helpers
  void ToggleMenuActiveState();
  void KeyboardNavigation(PRUint32 aDirection);
  void ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag);
  nsIFrame* FindMenuWithShortcut(PRUint32 aLetter);

protected:
  nsMenuBarListener* mMenuBarListener; // The listener that tells us about key and mouse events.
  PRBool mIsActive; // Whether or not the menu bar is active (a menu item is highlighted or shown).
  nsIFrame* mCurrentMenu; // The current menu that is active.
}; // class nsMenuBarFrame

#endif
