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
#include "nsPopUpMenu.h"
#include "nsIMenu.h"
#include "nsIWidget.h"

#include "nsString.h"
#include "nsFileSpec.h" // XXX: For nsAutoCString

NS_IMPL_ISUPPORTS(nsPopUpMenu, nsIPopUpMenu::GetIID())

//-------------------------------------------------------------------------
//
// nsPopUpMenu constructor
//
//-------------------------------------------------------------------------
nsPopUpMenu::nsPopUpMenu() : nsIPopUpMenu()
{
  NS_INIT_REFCNT();
  mNumMenuItems = 0;
  mParent       = nsnull;
  mMenu         = nsnull;
}

//-------------------------------------------------------------------------
//
// nsPopUpMenu destructor
//
//-------------------------------------------------------------------------
nsPopUpMenu::~nsPopUpMenu()
{
  NS_IF_RELEASE(mParent);
}


//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::Create(nsIWidget *aParent)
{

  mParent = aParent;
  NS_ADDREF(mParent);

  GtkWidget *parentWidget = GTK_WIDGET(mParent->GetNativeData(NS_NATIVE_WINDOW));
  mMenu = gtk_menu_new();
  /*mMenu = XmCreatePopupMenu(parentWidget, "_popup", NULL, 0); */
  return NS_OK;
}


//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddItem(const nsString &aText)
{
  GtkWidget *widget;

  widget = gtk_menu_item_new_with_label ((const char*)nsAutoCString(mLabel));
  gtk_widget_show(widget);
  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddItem(nsIMenuItem * aMenuItem)
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
NS_METHOD nsPopUpMenu::AddMenu(nsIMenu * aMenu)
{
  nsString Label;
  GtkWidget *item=NULL, *parentmenu=NULL, *newmenu=NULL;
  void *voidData=NULL;
  
  aMenu->GetLabel(Label);

  GetNativeData(voidData);
  parentmenu = GTK_WIDGET(voidData);

  item = gtk_menu_item_new_with_label ((const char*)nsAutoCString(Label));
  gtk_widget_show(item);
  gtk_menu_shell_append (GTK_MENU_SHELL (parentmenu), item);

  voidData = NULL;

  aMenu->GetNativeData(&voidData);
  newmenu = GTK_WIDGET(voidData);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), newmenu);

  // XXX add aMenu to internal data structor list
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddSeparator() 
{
  GtkWidget *widget;
  widget = gtk_menu_item_new ();
  gtk_widget_show(widget);
  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::GetItemCount(PRUint32 &aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::GetItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::InsertItemAt(const PRUint32 aCount, nsIMenuItem *& aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::InsertItemAt(const PRUint32 aCount, const nsString & aMenuItemName)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::InsertSeparator(const PRUint32 aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::RemoveItem(const PRUint32 aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::RemoveAll()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
void nsPopUpMenu::GetXY(GtkMenu *menu, gint *x, gint *y, gpointer user_data)
{
  *x = ((nsPopUpMenu *)(user_data))->mX;
  *y = ((nsPopUpMenu *)(user_data))->mY;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::ShowMenu(PRInt32 aX, PRInt32 aY)
{
  mX = aX;
  mY = aY;

  gtk_menu_popup (GTK_MENU(mMenu),
		  NULL,
		  NULL,
                  GetXY,
		  this,
		  0,
		  GDK_CURRENT_TIME);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::GetNativeData(void *& aData)
{
  aData = (void *)mMenu;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::GetParent(nsIWidget *& aParent)
{
  aParent = mParent;
  return NS_OK;
}


