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

#include "nsMenuBar.h"
#include "nsMenuItem.h"
#include "nsIComponentManager.h"
#include "nsIDOMNode.h"
#include "nsIMenu.h"
#include "nsIWebShell.h"
#include "nsIWidget.h"

#include "nsString.h"

#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include <MenuItem.h>

static NS_DEFINE_CID(kMenuBarCID, NS_MENUBAR_CID);
static NS_DEFINE_CID(kMenuCID, NS_MENU_CID);

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

nsresult nsMenuBar::QueryInterface(REFNSIID aIID, void** aInstancePtr)      
{                                                                        
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtr = NULL;

  if (aIID.Equals(nsIMenuBar::GetIID())) {
    *aInstancePtr = (void*) ((nsIMenuBar*) this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) ((nsISupports*)(nsIMenuBar*) this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(nsIMenuListener::GetIID())) {
    *aInstancePtr = (void*) ((nsIMenuListener*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsMenuBar)
NS_IMPL_RELEASE(nsMenuBar)

//-------------------------------------------------------------------------
//
// nsMenuBar constructor
//
//-------------------------------------------------------------------------
nsMenuBar::nsMenuBar() : nsIMenuBar(), nsIMenuListener()
{
  NS_INIT_REFCNT();
  mMenu     = nsnull;
  mParent   = nsnull;
  mItems = new nsVoidArray();
}

//-------------------------------------------------------------------------
//
// nsMenuBar destructor
//
//-------------------------------------------------------------------------
nsMenuBar::~nsMenuBar()
{
  NS_IF_RELEASE(mParent);

  // Remove all references to the menus
  mItems->Clear();
}

//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::Create(nsIWidget *aParent)
{
	SetParent(aParent);
	mMenu = new BMenuBar(BRect(0, 0, 0, 0), B_EMPTY_STRING);
//	mParent->SetMenuBar(this);
	return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetParent(nsIWidget *&aParent)
{
  aParent = mParent;
  NS_ADDREF(mParent);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::SetParent(nsIWidget *aParent)
{
  NS_IF_RELEASE(mParent);
  mParent = aParent;
  NS_IF_ADDREF(mParent);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::AddMenu(nsIMenu * aMenu)
{
  InsertMenuAt(mItems->Count(), aMenu);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetMenuCount(PRUint32 &aCount)
{
  aCount = mItems->Count();
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetMenuAt(const PRUint32 aPos, nsIMenu *& aMenu)
{
  aMenu = (nsIMenu *)mItems->ElementAt(aPos);
  NS_ADDREF(aMenu);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::InsertMenuAt(const PRUint32 aPos, nsIMenu *& aMenu)
{
  nsString name;
  aMenu->GetLabel(name);
  char * nameStr = name.ToNewCString();

  mItems->InsertElementAt(aMenu, (PRInt32)aPos);
  NS_ADDREF(aMenu);

  BMenu *nativeMenu;
  void *data;
  aMenu->GetNativeData(&data);
  nativeMenu = (BMenu *)data;
  mMenu->AddItem(nativeMenu, aPos);
  delete[] nameStr;

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::RemoveMenu(const PRUint32 aPos)
{
  nsIMenu * menu = (nsIMenu *)mItems->ElementAt(aPos);
  NS_RELEASE(menu);
  mItems->RemoveElementAt(aPos);
  delete mMenu->RemoveItem(aPos);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::RemoveAll()
{
printf("nsMenuBar::RemoveAll - FIXME: not implemented\n");
#if 0
  while (mItems->Count()) {
    nsISupports * supports = (nsISupports *)mItems->ElementAt(0);
    NS_RELEASE(supports);
    mItems->RemoveElementAt(0);

    ::RemoveMenu(mMenu, 0, MF_BYPOSITION);
  }
#endif
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetNativeData(void *& aData)
{
  aData = (void *)mMenu;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::SetNativeData(void * aData)
{
  // Temporary hack for MacOS. Will go away when nsMenuBar handles it's own
  // construction
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::Paint()
{
  mParent->Invalidate(PR_TRUE);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsMenuListener interface
//
//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuBar::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuBar::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuBar::MenuConstruct(
    const nsMenuEvent & aMenuEvent,
    nsIWidget         * aParentWindow, 
    void              * menubarNode,
    void              * aWebShell)
{
  mWebShell = (nsIWebShell*) aWebShell;
  mDOMNode  = (nsIDOMNode*)menubarNode;
  
  nsIMenuBar * pnsMenuBar = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuBarCID,
                                                   nsnull,
                                                   nsIMenuBar::GetIID(),
                                                   (void**)&pnsMenuBar);
  if (NS_OK == rv) {
    if (nsnull != pnsMenuBar) {
      pnsMenuBar->Create(aParentWindow);

      // set pnsMenuBar as a nsMenuListener on aParentWindow
      nsCOMPtr<nsIMenuListener> menuListener;
      pnsMenuBar->QueryInterface(nsIMenuListener::GetIID(), getter_AddRefs(menuListener));
      aParentWindow->AddMenuListener(menuListener);

      nsCOMPtr<nsIDOMNode> menuNode;
      ((nsIDOMNode*)menubarNode)->GetFirstChild(getter_AddRefs(menuNode));
      while (menuNode) {
        nsCOMPtr<nsIDOMElement> menuElement(do_QueryInterface(menuNode));
        if (menuElement) {
          nsString menuNodeType;
          nsString menuName;
          menuElement->GetNodeName(menuNodeType);
          if (menuNodeType.Equals("menu")) {
            menuElement->GetAttribute(nsAutoString("value"), menuName);
            // Don't create the menu yet, just add in the top level names

            // Create nsMenu
            nsIMenu * pnsMenu = nsnull;
            rv = nsComponentManager::CreateInstance(kMenuCID, nsnull, nsIMenu::GetIID(), (void**)&pnsMenu);
            if (NS_OK == rv) {
              // Call Create
              nsISupports * supports = nsnull;
              pnsMenuBar->QueryInterface(kISupportsIID, (void**) &supports);
              pnsMenu->Create(supports, menuName);
              NS_RELEASE(supports);

              pnsMenu->SetLabel(menuName); 
              pnsMenu->SetDOMNode(menuNode);
              pnsMenu->SetDOMElement(menuElement);
              pnsMenu->SetWebShell(mWebShell);

              // Make nsMenu a child of nsMenuBar
              // nsMenuBar takes ownership of the nsMenu
              pnsMenuBar->AddMenu(pnsMenu); 

              // Release the menu now that the menubar owns it
              NS_RELEASE(pnsMenu);
            }
          } 

        }
        nsCOMPtr<nsIDOMNode> oldmenuNode(menuNode);  
        oldmenuNode->GetNextSibling(getter_AddRefs(menuNode));
      } // end while (nsnull != menuNode)

      // Give the aParentWindow this nsMenuBar to hold onto.
      // The parent window should take ownership at this point
      aParentWindow->SetMenuBar(pnsMenuBar);

      // HACK: force a paint for now
      pnsMenuBar->Paint();

      NS_RELEASE(pnsMenuBar);
    } // end if ( nsnull != pnsMenuBar )
  }

  return nsEventStatus_eIgnore;
  return nsEventStatus_eIgnore;
}

nsEventStatus nsMenuBar::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}
