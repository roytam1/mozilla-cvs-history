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
#include "nsPopUpMenu.h"
#include "nsIPopUpMenu.h"
#include "nsIMenu.h"
#include "nsIWidget.h"

#include "nsString.h"
#include "nsStringUtil.h"
#include "nsPhWidgetLog.h"

NS_IMPL_ISUPPORTS1(nsPopUpMenu, nsIPopUpMenu)

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

  return NS_OK;
}


//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddItem(const nsString &aText)
{
  char * labelStr = mLabel.ToNewCString();
  delete[] labelStr;

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddItem(nsIMenuItem * aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddMenu(nsIMenu * aMenu)
{
  nsString Label;
//  GtkWidget *item=NULL, *parentmenu=NULL, *newmenu=NULL;
  char *labelStr;
  void *voidData=NULL;
  
  aMenu->GetLabel(Label);

  labelStr = Label.ToNewCString();

  GetNativeData(voidData);
//  parentmenu = GTK_WIDGET(voidData);

//  item = gtk_menu_item_new_with_label (labelStr);
//  gtk_widget_show(item);
//  gtk_menu_shell_append (GTK_MENU_SHELL (parentmenu), item);

  delete[] labelStr;

  voidData = NULL;

//  aMenu->GetNativeData(voidData);
//  newmenu = GTK_WIDGET(voidData);

//  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), newmenu);

  // XXX add aMenu to internal data structor list
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::AddSeparator() 
{
//  GtkWidget *widget;

//  widget = gtk_menu_item_new ();
//  gtk_widget_show(widget);
//  gtk_menu_shell_append (GTK_MENU_SHELL (mMenu), widget);

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

#if 0
//-------------------------------------------------------------------------
void nsPopUpMenu::GetXY(GtkMenu *menu, gint *x, gint *y, gpointer user_data)
{
//  *x = ((nsPopUpMenu *)(user_data))->mX;
//  *y = ((nsPopUpMenu *)(user_data))->mY;
}
#endif

//-------------------------------------------------------------------------
NS_METHOD nsPopUpMenu::ShowMenu(PRInt32 aX, PRInt32 aY)
{
#if 0
  mX = aX;
  mY = aY;

  gtk_menu_popup (GTK_MENU(mMenu),
		  NULL,
		  NULL,
                  GetXY,
		  this,
		  0,
		  GDK_CURRENT_TIME);
#endif

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


