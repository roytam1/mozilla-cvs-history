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

#ifndef nsMenu_h__
#define nsMenu_h__

#include "nsdefs.h"
#include "nsWindow.h"
#include "nsSwitchToUIThread.h"

#include "nsIMenu.h"
#include "nsVoidArray.h"
#include "nsIMenuListener.h"
#include "nsVoidArray.h"

/**
 * Native Win32 button wrapper
 */

class nsMenu : public nsIMenu, public nsIMenuListener
{

public:
  nsMenu();
  virtual ~nsMenu();

  NS_DECL_ISUPPORTS
  
  //nsIMenuListener interface
  nsEventStatus MenuSelected(const nsMenuEvent & aMenuEvent);
  
  NS_IMETHOD Create(nsIMenuBar * aParent, const nsString &aLabel);
  NS_IMETHOD Create(nsIMenu * aParent, const nsString &aLabel);

  // nsIMenu Methods
  NS_IMETHOD GetParent(nsISupports *&aParent);
  NS_IMETHOD GetLabel(nsString &aText);
  NS_IMETHOD SetLabel(nsString &aText);
  NS_IMETHOD AddItem(const nsString &aText);
  NS_IMETHOD AddMenuItem(nsIMenuItem * aMenuItem);
  NS_IMETHOD AddMenu(nsIMenu * aMenu);
  NS_IMETHOD AddSeparator();
  NS_IMETHOD GetItemCount(PRUint32 &aCount);
  NS_IMETHOD GetItemAt(const PRUint32 aCount, nsISupports *& aMenuItem);
  NS_IMETHOD InsertItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem);
  NS_IMETHOD InsertItemAt(const PRUint32 aCount, const nsString & aMenuItemName);
  NS_IMETHOD InsertSeparator(const PRUint32 aCount);
  NS_IMETHOD RemoveItem(const PRUint32 aCount);
  NS_IMETHOD RemoveAll();
  NS_IMETHOD GetNativeData(void*& aData);

  // Native Impl Methods
  nsIMenu    * GetMenuParent()    { return mMenuParent;    }
  nsIMenuBar * GetMenuBarParent() { return mMenuBarParent; }

protected:
  nsIMenuBar * GetMenuBar(nsIMenu * aMenu);
  nsIWidget *  GetParentWidget();

  nsString     mLabel;
  PRUint32     mNumMenuItems;
  HMENU        mMenu;

  nsIMenuBar * mMenuBarParent;
  nsIMenu    * mMenuParent;

  nsVoidArray mItems;

};

#endif // nsMenu_h__
