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

#include <gtk/gtk.h>

#include "nsMenu.h"
#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIMenuItem.h"

#include "nsString.h"
#include "nsStringUtil.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIMenuIID, NS_IMENU_IID);
//NS_IMPL_ISUPPORTS(nsMenu, kMenuIID)

nsresult nsMenu::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      
                                                                         
  *aInstancePtr = NULL;                                                  
                                                                                        
  if (aIID.Equals(kIMenuIID)) {                                         
    *aInstancePtr = (void*)(nsIMenu*) this;                                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }                                                                      
  if (aIID.Equals(kISupportsIID)) {                                      
    *aInstancePtr = (void*)(nsISupports*)(nsIMenu*)this;                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }
  if (aIID.Equals(kIMenuListenerIID)) {                                      
    *aInstancePtr = (void*)(nsIMenuListener*)this;                        
    NS_ADDREF_THIS();                                                    
    return NS_OK;                                                        
  }                                                     
  return NS_NOINTERFACE;                                                 
}

NS_IMPL_ADDREF(nsMenu)
NS_IMPL_RELEASE(nsMenu)

//-------------------------------------------------------------------------
//
// nsMenu constructor
//
//-------------------------------------------------------------------------
nsMenu::nsMenu() : nsIMenu()
{
  NS_INIT_REFCNT();
  mNumMenuItems  = 0;
  mMenu          = nsnull;
  mMenuParent    = nsnull;
  mMenuBarParent = nsnull;
}

//-------------------------------------------------------------------------
//
// nsMenu destructor
//
//-------------------------------------------------------------------------
nsMenu::~nsMenu()
{
  NS_IF_RELEASE(mMenuBarParent);
  NS_IF_RELEASE(mMenuParent);
}

//-------------------------------------------------------------------------
//
// Create the proper widget
// add menu to menubar
//-------------------------------------------------------------------------
NS_METHOD nsMenu::Create(nsIMenuBar *aParent, const nsString &aLabel)
{
  mMenuBarParent = aParent;
  NS_ADDREF(mMenuBarParent);

  mLabel = aLabel;
  mMenu = gtk_menu_new();
//aParent->AddMenu(this);

  return NS_OK;
}

//add submenu
//-------------------------------------------------------------------------
NS_METHOD nsMenu::Create(nsIMenu *aParent, const nsString &aLabel)
{
  mMenuParent = aParent;
  NS_ADDREF(mMenuParent);

  mLabel = aLabel;
  mMenu = gtk_menu_new();
//  aParent->AddMenu(this);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::GetParent(nsISupports*& aParent)
{
  aParent = nsnull;
  if (nsnull != mMenuParent) {
    return mMenuParent->QueryInterface(kISupportsIID,(void**)&aParent);
  } else if (nsnull != mMenuBarParent) {
    return mMenuBarParent->QueryInterface(kISupportsIID,(void**)&aParent);
  }

  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::GetLabel(nsString &aText)
{
  aText = mLabel;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::SetLabel(nsString &aText)
{
  /* we Do GetLabel() when we are adding the menu...
   *  but we might want to redo this.
   */
  mLabel = aText;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::AddItem(const nsString &aText)
{
  char * labelStr = mLabel.ToNewCString();
  GtkWidget *widget;

  widget = gtk_menu_item_new_with_label (labelStr);
  gtk_widget_show(widget);
  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);

  delete[] labelStr;

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::AddMenuItem(nsIMenuItem * aMenuItem)
{
  GtkWidget *widget;
  void *voidData;
  
  aMenuItem->GetNativeData(voidData);
  widget = GTK_WIDGET(voidData);

  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);

  // XXX add aMenuItem to internal data structor list
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::AddMenu(nsIMenu * aMenu)
{
  nsString Label;
  GtkWidget *item=nsnull, *newmenu=nsnull;
  char *labelStr;
  void *voidData=NULL;
  
  aMenu->GetLabel(Label);

  labelStr = Label.ToNewCString();

  item = gtk_menu_item_new_with_label (labelStr);
  gtk_widget_show(item);
  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), item);

  delete[] labelStr;

  voidData = NULL;

  aMenu->GetNativeData(voidData);
  newmenu = GTK_WIDGET(voidData);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), newmenu);

  return NS_OK;

}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::AddSeparator() 
{
  GtkWidget *widget;
  widget = gtk_menu_item_new ();
  gtk_widget_show(widget);
  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::GetItemCount(PRUint32 &aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::InsertSeparator(const PRUint32 aPos)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::RemoveItem(const PRUint32 aPos)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::RemoveAll()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenu::GetNativeData(void *& aData)
{
  aData = (void *)mMenu;
  return NS_OK;
}

GtkWidget *nsMenu::GetNativeParent()
{
  void * voidData; 
  if (nsnull != mMenuParent) {
    mMenuParent->GetNativeData(voidData);
  } else if (nsnull != mMenuBarParent) {
    mMenuBarParent->GetNativeData(voidData);
  } else {
    return nsnull;
  }
  return GTK_WIDGET(voidData);
}

//-------------------------------------------------------------------------
nsEventStatus nsMenu::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

