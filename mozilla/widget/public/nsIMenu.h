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

#ifndef nsIMenu_h__
#define nsIMenu_h__

#include "nsISupports.h"
#include "nsString.h"

class nsIMenuBar;
class nsIMenu;
class nsIMenuItem;

// {35A3DEC1-4992-11d2-8DBA-00609703C14E}
#define NS_IMENU_IID      \
{ 0x35a3dec1, 0x4992, 0x11d2, \
  { 0x8d, 0xba, 0x0, 0x60, 0x97, 0x3, 0xc1, 0x4e } }


/**
 * Menu widget
 */
class nsIMenu : public nsISupports {

  public:
 
  /**
    * Creates the Menu and adds it the MenuBar
    *
    */
    NS_IMETHOD Create(nsIMenuBar * aParent, const nsString &aLabel) = 0;
    
  /**
    * Creates the Menu and adds it another Menu
    *
    */
    NS_IMETHOD Create(nsIMenu * aParent, const nsString &aLabel) = 0;

   /**
    * Get the Menu's Parent
    *
    */
    NS_IMETHOD GetParent(nsISupports *&aParent) = 0;

   /**
    * Get the Menu label
    *
    */
    NS_IMETHOD GetLabel(nsString &aText) = 0;

   /**
    * Set the Menu label
    *
    */
    NS_IMETHOD SetLabel(nsString &aText) = 0;

   /**
    * Adds a Menu Item
    *
    */
    NS_IMETHOD AddItem(const nsString &aText) = 0;
    
   /**
    * Adds a Menu Item
    *
    */
    NS_IMETHOD AddMenuItem(nsIMenuItem * aMenuItem) = 0;
    
   /**
    * Adds a Cascading Menu 
    *
    */
    NS_IMETHOD AddMenu(nsIMenu * aMenu) = 0;
    
   /**
    * Adds Separator
    *
    */
    NS_IMETHOD AddSeparator() = 0;

   /**
    * Returns the number of menu items
    *
    */
    NS_IMETHOD GetItemCount(PRUint32 &aCount) = 0;

   /**
    * Returns a Menu Item at a specified Index
    *
    */
    NS_IMETHOD GetItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem) = 0;

   /**
    * Inserts a Menu Item at a specified Index
    *
    */
    NS_IMETHOD InsertItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem) = 0;

   /**
    * Creates and inserts a Menu Item at a specified Index
    *
    */
    NS_IMETHOD InsertItemAt(const PRUint32 aCount, const nsString & aMenuItemName) = 0;

   /**
    * Creates and inserts a Separator at a specified Index
    *
    */
    NS_IMETHOD InsertSeparator(const PRUint32 aCount) = 0;

   /**
    * Removes an Menu Item from a specified Index
    *
    */
    NS_IMETHOD RemoveItem(const PRUint32 aCount) = 0;

   /**
    * Removes all the Menu Items
    *
    */
    NS_IMETHOD RemoveAll() = 0;

   /**
    * Gets Native MenuHandle
    *
    */
    NS_IMETHOD  GetNativeData(void*& aData) = 0;
};

#endif
