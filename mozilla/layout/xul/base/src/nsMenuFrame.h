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
// nsMenuFrame
//

#ifndef nsMenuFrame_h__
#define nsMenuFrame_h__

#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

#include "nsIAnonymousContentCreator.h"
#include "nsBoxFrame.h"
#include "nsFrameList.h"
#include "nsIMenuParent.h"
#include "nsIMenuFrame.h"
#include "nsMenuDismissalListener.h"
#include "nsITimer.h"
#include "nsITimerCallback.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsIContent.h"

nsresult NS_NewMenuFrame(nsIFrame** aResult, PRUint32 aFlags) ;

class nsMenuBarFrame;
class nsMenuPopupFrame;

class nsMenuFrame : public nsBoxFrame, 
                    public nsIMenuFrame,
                    public nsITimerCallback, 
                    public nsIAnonymousContentCreator
{
public:
  nsMenuFrame();

  NS_DECL_ISUPPORTS
  
  // The nsIAnonymousContentCreator interface
  NS_IMETHOD CreateAnonymousContent(nsISupportsArray& aAnonymousItems);
  
  // The nsITimerCallback interface
  virtual void Notify(nsITimer *timer);

  NS_IMETHOD Init(nsIPresContext&  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD IsActive(PRBool& aResult) { aResult = PR_TRUE; return NS_OK; };

  // The following four methods are all overridden so that the menu children
  // can be stored in a separate list (so that they don't impact reflow of the
  // actual menu item at all).
  NS_IMETHOD FirstChild(nsIAtom*   aListName,
                        nsIFrame** aFirstChild) const;
  NS_IMETHOD SetInitialChildList(nsIPresContext& aPresContext,
                                 nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  NS_IMETHOD GetAdditionalChildListName(PRInt32   aIndex,
                                        nsIAtom** aListName) const;
  NS_IMETHOD Destroy(nsIPresContext& aPresContext);

  // Overridden to prevent events from ever going to children of the menu.
  NS_IMETHOD GetFrameForPoint(nsIPresContext* aPresContext,
                              const nsPoint& aPoint, 
                              nsIFrame**     aFrame);

  NS_IMETHOD HandleEvent(nsIPresContext& aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus&  aEventStatus);

  // Reflow methods
  NS_IMETHOD Reflow(nsIPresContext&   aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD DidReflow(nsIPresContext& aPresContext,
                            nsDidReflowStatus aStatus);

  NS_IMETHOD Dirty(const nsHTMLReflowState& aReflowState, nsIFrame*& incrementalChild);

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

  // nsIMenuFrame Interface

  NS_IMETHOD ActivateMenu(PRBool aActivateFlag);
  NS_IMETHOD SelectMenu(PRBool aActivateFlag);
  NS_IMETHOD OpenMenu(PRBool aActivateFlag);

  NS_IMETHOD MenuIsOpen(PRBool& aResult) { aResult = IsOpen(); return NS_OK; };
  NS_IMETHOD MenuIsContainer(PRBool& aResult) { aResult = IsMenu(); return NS_OK; };
  NS_IMETHOD MenuIsChecked(PRBool& aResult) { aResult = mChecked; return NS_OK; };
  
  NS_IMETHOD SelectFirstItem();

  NS_IMETHOD Escape(PRBool& aHandledFlag);
  NS_IMETHOD Enter();
  NS_IMETHOD ShortcutNavigation(PRUint32 aLetter, PRBool& aHandledFlag);
  NS_IMETHOD KeyboardNavigation(PRUint32 aDirection, PRBool& aHandledFlag);

  NS_IMETHOD GetMenuParent(nsIMenuParent** aResult) { NS_IF_ADDREF(mMenuParent); *aResult = mMenuParent; return NS_OK; };
  NS_IMETHOD GetRadioGroupName(nsString &aName) { aName = mGroupName; return NS_OK; };
  NS_IMETHOD GetMenuType(nsMenuType &aType) { aType = mType; return NS_OK; };

  NS_IMETHOD MarkAsGenerated();

  // nsMenuFrame methods 

  PRBool IsOpen() { return mMenuOpen; };
  PRBool IsMenu();
  PRBool IsDisabled();
  PRBool IsGenerated();
  NS_IMETHOD ToggleMenuState();

  void SetIsMenu(PRBool aIsMenu) { mIsMenu = aIsMenu; };

protected:
  static void UpdateDismissalListener(nsIMenuParent* aMenuParent);
  void UpdateMenuType();
  void UpdateMenuSpecialState();

  void OpenMenuInternal(PRBool aActivateFlag);
  void GetMenuChildrenElement(nsIContent** aResult);

  // Called to split the accesskey attribute up based on the specified string.
  void SplitOnShortcut(nsString& aBeforeString, nsString& aAccessString, nsString& aAfterString);

  // Examines the key node and builds the accelerator.
  void BuildAcceleratorText(nsString& aAccelString);

  // Called to execute our command handler.
  void Execute();

  // Called as a hook just before the menu gets opened.
  PRBool OnCreate();

  // Called as a hook just before the menu goes away.
  PRBool OnDestroy();

  NS_IMETHOD AttributeChanged(nsIPresContext* aPresContext,
                              nsIContent* aChild,
                              PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aHint);

protected:
  nsFrameList mPopupFrames;
  PRPackedBool mIsMenu; // Whether or not we can even have children or not.
  PRPackedBool mMenuOpen;
  PRPackedBool mHasAnonymousContent;  // Do we have anonymous content frames?
  PRPackedBool mChecked;              // if so, are we checked?
  nsMenuType mType;

  nsCOMPtr<nsIContent> mMenuText;
  nsCOMPtr<nsIContent> mAccelText;
  nsIMenuParent* mMenuParent; // Our parent menu.
  nsCOMPtr<nsITimer> mOpenTimer;
  nsIPresContext* mPresContext; // Our pres context.
  nsString mGroupName;

public:
  static nsMenuDismissalListener* mDismissalListener; // The listener that dismisses menus.
}; // class nsMenuFrame

#endif
