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

#include "nsContextMenu.h"
#include "nsIContextMenu.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"
#include "nsIMenuListener.h"

#include "nsCOMPtr.h"

#include "nsIComponentManager.h"

#include "nsMenu.h" // for mMacMenuIDCount

#include "nsString.h"
#include "nsStringUtil.h"

#include <TextUtils.h>
#include <ToolUtils.h>
#include <Devices.h>
#include <Menus.h>
#include "nslog.h"

NS_IMPL_LOG(nsContextMenuLog)
#define PRINTF NS_LOG_PRINTF(nsContextMenuLog)
#define FLUSH  NS_LOG_FLUSH(nsContextMenuLog)

// CIDs
#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kMenuBarCID,  NS_MENUBAR_CID);
static NS_DEFINE_CID(kMenuCID,     NS_MENU_CID);
static NS_DEFINE_CID(kMenuItemCID, NS_MENUITEM_CID);

NS_IMPL_ISUPPORTS2(nsContextMenu, nsIContextMenu, nsIMenuListener)

//-------------------------------------------------------------------------
//
// nsContextMenu constructor
//
//-------------------------------------------------------------------------
nsContextMenu::nsContextMenu() : nsIContextMenu()
{
  NS_INIT_REFCNT();
  mNumMenuItems  = 0;
  mMenuParent    = nsnull;
  
  mMacMenuID = 0;
  mMacMenuHandle = nsnull;
  mListener      = nsnull;
  mX             = 0;
  mY             = 0;
  mDOMNode       = nsnull;
  mWebShell      = nsnull;

  //
  // create a multi-destination Unicode converter which can handle all of the installed
  //	script systems
  //
  OSErr err = ::CreateUnicodeToTextRunInfoByScriptCode(0,NULL,&mUnicodeTextRunConverter);
  NS_ASSERTION(err==noErr,"nsMenu::nsMenu: CreateUnicodeToTextRunInfoByScriptCode failed.");	


}

//-------------------------------------------------------------------------
//
// nsContextMenu destructor
//
//-------------------------------------------------------------------------
nsContextMenu::~nsContextMenu()
{
  NS_IF_RELEASE(mListener);

  while(mNumMenuItems)
  {
    --mNumMenuItems;
    
    if(mMenuItemVoidArray[mNumMenuItems]) {
      // Figure out what we're releasing
      nsIMenuItem * menuitem = nsnull;
      ((nsISupports*)mMenuItemVoidArray[mNumMenuItems])->QueryInterface(NS_GET_IID(nsIMenuItem), (void**) &menuitem);  
      if(menuitem)
      {
        // case menuitem
        menuitem->Release(); // Release our hold
        NS_IF_RELEASE(menuitem); // Balance QI
      }
      else
      {
	    nsIMenu * menu = nsnull;
	    ((nsISupports*)mMenuItemVoidArray[mNumMenuItems])->QueryInterface(NS_GET_IID(nsIMenu), (void**) &menu);
	    if(menu)
	    {
	      // case menu
	      menu->Release(); // Release our hold 
	      NS_IF_RELEASE(menu); // Balance QI
	    }
	  }
	}
  }
  
  mMacMenuIDCount--;
  OSErr err = ::DisposeUnicodeToTextRunInfo(&mUnicodeTextRunConverter);
  NS_ASSERTION(err==noErr,"nsMenu::~nsMenu: DisposeUnicodeToTextRunInfo failed.");	    
}

//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::Create(nsISupports *aParent, const nsString& anAlignment,
                                const nsString& anAnchorAlignment)
{
  mParent = aParent;
  mMacMenuHandle = ::NewMenu(mMacMenuIDCount, "\p");
  mMacMenuID = mMacMenuIDCount;
  mMacMenuIDCount++;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::GetParent(nsISupports*& aParent)
{
  aParent = nsnull;
  if (mParent) {
    return mParent->QueryInterface(NS_GET_IID(nsISupports),(void**)&aParent);
  }
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::AddItem(nsISupports* aItem)
{
  if(aItem)
  {
    // Figure out what we're adding
    nsIMenuItem * menuitem = nsnull;
    aItem->QueryInterface(NS_GET_IID(nsIMenuItem), (void**) &menuitem);  
    if(menuitem)
    {
      // case menuitem
      AddMenuItem(menuitem);
      NS_RELEASE(menuitem);
    }
    else
    {
	  nsIMenu * menu = nsnull;
	  aItem->QueryInterface(NS_GET_IID(nsIMenu), (void**) &menu);
	  if(menu)
	  {
	    // case menu
	    AddMenu(menu);
	    NS_RELEASE(menu);
	  }
	}
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::AddMenuItem(nsIMenuItem * aMenuItem)
{
  if(aMenuItem) {
    nsISupports * supports = nsnull;
    aMenuItem->QueryInterface(NS_GET_IID(nsISupports), (void**)&supports);
    if(supports) {
	  mMenuItemVoidArray.AppendElement(supports);
      
	  nsString label;
	  nsString labelHack; labelHack.AssignWithConversion(" ");
	  aMenuItem->GetLabel(label);
	    
	  mNumMenuItems++;
	  ::InsertMenuItem(mMacMenuHandle, (const unsigned char *)" ", mNumMenuItems);
	  if(label[0] == PRUnichar('-')) {
	    labelHack.Append(label);
 	    NSStringSetMenuItemText(mMacMenuHandle, mNumMenuItems, labelHack);
	  } else {
	    NSStringSetMenuItemText(mMacMenuHandle, mNumMenuItems, label);
	  }
	}
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::AddMenu(nsIMenu * aMenu)
{
  // Add a submenu
  if(aMenu) {
    nsISupports * supports = nsnull;
    aMenu->QueryInterface(NS_GET_IID(nsISupports), (void**)&supports);
    if(supports) {
      mMenuItemVoidArray.AppendElement(supports);
  
      // We have to add it as a menu item and then associate it with the item
      nsString label;
      aMenu->GetLabel(label);

      mNumMenuItems++;
      ::InsertMenuItem(mMacMenuHandle, "\p ", mNumMenuItems);
      NSStringSetMenuItemText(mMacMenuHandle, mNumMenuItems,label);
  
      MenuHandle menuHandle;
      aMenu->GetNativeData((void**)&menuHandle);
      ::InsertMenu(menuHandle, hierMenu);
      PRInt16 temp = mMacMenuIDCount;
      ::SetMenuItemHierarchicalID((MenuHandle) mMacMenuHandle, mNumMenuItems, --temp);
    }
  }
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::AddSeparator() 
{
  // HACK - We're not really appending an nsMenuItem but it 
  // needs to be here to make sure that event dispatching isn't off by one.
  mMenuItemVoidArray.AppendElement(nsnull);
  ::InsertMenuItem(mMacMenuHandle, "\p(-", mNumMenuItems );
  mNumMenuItems++;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::GetItemCount(PRUint32 &aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::GetItemAt(const PRUint32 aPos, nsISupports *& aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::InsertItemAt(const PRUint32 aPos, nsISupports * aMenuItem)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::RemoveItem(const PRUint32 aPos)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::RemoveAll()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::GetNativeData(void ** aData)
{
  *aData = mMacMenuHandle;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::AddMenuListener(nsIMenuListener * aMenuListener)
{
  mListener = aMenuListener;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsContextMenu::RemoveMenuListener(nsIMenuListener * aMenuListener)
{
  if (aMenuListener == mListener) {
  }
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// nsIMenuListener interface
//
//-------------------------------------------------------------------------
nsEventStatus nsContextMenu::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  nsEventStatus eventStatus = nsEventStatus_eIgnore;
      
  // Determine if this is the correct menu to handle the event
  //PRInt16 menuID = HiWord(((nsMenuEvent)aMenuEvent).mCommand);
  PRInt16 menuID = mSelectedMenuID;

#ifdef APPLE_MENU_HACK
  if(kAppleMenuID == menuID)
	{
    //PRInt16 menuItemID = LoWord(((nsMenuEvent)aMenuEvent).mCommand);
    PRInt16 menuItemID = mSelectedMenuItem;
		if (menuItemID > 2)			// don't handle the about or separator items yet
		{
			Str255		itemStr;
			::GetMenuItemText(GetMenuHandle(menuID), menuItemID, itemStr);
#if !TARGET_CARBON
			::OpenDeskAcc(itemStr);
#endif
			eventStatus = nsEventStatus_eConsumeNoDefault;
		}
	}
	else
#endif
  if(mMacMenuID == menuID)
  {
    // Call MenuSelected on the correct nsMenuItem
    //PRInt16 menuItemID = LoWord(((nsMenuEvent)aMenuEvent).mCommand);
    PRInt16 menuItemID = mSelectedMenuItem;
    nsIMenuListener * menuListener = nsnull;
    ((nsIMenuItem*)mMenuItemVoidArray[menuItemID-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
	if(menuListener) {
	  eventStatus = menuListener->MenuSelected(aMenuEvent);
	  NS_IF_RELEASE(menuListener);
	}
  } 
  else
  {
    // Make sure none of our submenus are the ones that should be handling this
      for (int i = mMenuItemVoidArray.Count(); i > 0; i--)
	  {
	    if(nsnull != mMenuItemVoidArray[i-1])
	    {
		    nsIMenu * submenu = nsnull;
		    ((nsISupports*)mMenuItemVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenu), (void**)&submenu);
		    if(submenu)
		    {
			    nsIMenuListener * menuListener = nsnull;
			    ((nsISupports*)mMenuItemVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
			    if(menuListener){
			      eventStatus = menuListener->MenuSelected(aMenuEvent);
			      NS_IF_RELEASE(menuListener);
			      if(nsEventStatus_eIgnore != eventStatus)
			        return eventStatus;
			    }
		    }
		}
	  }
  
  }
  return eventStatus;
}
//-------------------------------------------------------------------------
nsEventStatus nsContextMenu::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  // Put context menu in the menu list
  ::InsertMenu(mMacMenuHandle, -1);    
      
  // Call MenuConstruct
  MenuConstruct(
      aMenuEvent,
      mParentWindow, 
      mDOMNode,
	  mWebShell);
  
  // Display and track the menu
  Point location;
  location.h = mX;
  location.v = mY;
  UInt32 outUserSelectionType;
  Str255 inHelpItemString = "\p";
  ::ContextualMenuSelect (
                     mMacMenuHandle,
                     location,
                     false,
                     kCMHelpItemNoHelp,
                     inHelpItemString,
                     0, //const AEDesc* inSelection, We should really be constructing this
                     & outUserSelectionType,
                     & mSelectedMenuID,
                     & mSelectedMenuItem); 
                     //ContextualMenuSelect			(MenuHandle 			inMenu,
						//		 Point 					inGlobalLocation,
						//		 Boolean 				inReserved,
						//		 UInt32 				inHelpType,
						//		 ConstStr255Param 		inHelpItemString,
						//		 const AEDesc *			inSelection,
						//		 UInt32 *				outUserSelectionType,
						//		 SInt16 *				outMenuID,
						//		 UInt16 *				outMenuItem)						TWOWORDINLINE(0x7003, 0xAA72);

  if(outUserSelectionType != kCMNothingSelected) {
    MenuItemSelected(aMenuEvent);
  }
  
  return eventStatus;
}

//-------------------------------------------------------------------------
nsEventStatus nsContextMenu::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  // Remove the context menu from the menu list
  ::DeleteMenu(mMacMenuID);    
      
  // Call MenuDestruct
  MenuDestruct(aMenuEvent);
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
nsEventStatus nsContextMenu::MenuConstruct(
    const nsMenuEvent & aMenuEvent,
    nsIWidget         * aParentWindow, 
    void              * menuNode,
	void              * aWebShell)
{

  
  // Construct the menu
      nsCOMPtr<nsIDOMNode> menuitemNode;
    ((nsIDOMNode*)mDOMNode)->GetFirstChild(getter_AddRefs(menuitemNode));

	unsigned short menuIndex = 0;

    while (menuitemNode) {
      nsCOMPtr<nsIDOMElement> menuitemElement(do_QueryInterface(menuitemNode));
      if (menuitemElement) {
        nsString menuitemNodeType;
        nsString menuitemName;
        menuitemElement->GetNodeName(menuitemNodeType);
        if (menuitemNodeType.EqualsWithConversion("menuitem")) {
          // LoadMenuItem
          LoadMenuItem(this, menuitemElement, menuitemNode, menuIndex, (nsIWebShell*)aWebShell);
        } else if (menuitemNodeType.EqualsWithConversion("menuseparator")) {
          AddSeparator();
        } else if (menuitemNodeType.EqualsWithConversion("menu")) {
          // Load a submenu
          LoadSubMenu(this, menuitemElement, menuitemNode);
        }
      }
	  ++menuIndex;
      nsCOMPtr<nsIDOMNode> oldmenuitemNode(menuitemNode);
      oldmenuitemNode->GetNextSibling(getter_AddRefs(menuitemNode));
    } // end menu item innner loop
  

  
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
nsEventStatus nsContextMenu::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
/**
* Set Location
*
*/
NS_METHOD nsContextMenu::SetLocation(PRInt32 aX, PRInt32 aY)
{
    mX = aX;
    mY = aY;
	return NS_OK;
}

//-------------------------------------------------------------------------
/**
* Set DOMNode
*
*/
NS_METHOD nsContextMenu::SetDOMNode(nsIDOMNode * aMenuNode)
{
    mDOMNode = aMenuNode;
	return NS_OK;
}

//-------------------------------------------------------------------------
/**
* Set DOMElement
*
*/
NS_METHOD nsContextMenu::SetDOMElement(nsIDOMElement * aMenuElement)
{
	return NS_OK;
}
    
//-------------------------------------------------------------------------
/**
* Set WebShell
*
*/
NS_METHOD nsContextMenu::SetWebShell(nsIWebShell * aWebShell)
{
    mWebShell = aWebShell;
	return NS_OK;
}

//----------------------------------------
void nsContextMenu::LoadMenuItem(
  nsIContextMenu * pParentMenu,
  nsIDOMElement  * menuitemElement,
  nsIDOMNode     * menuitemNode,
  unsigned short   menuitemIndex,
  nsIWebShell    * aWebShell)
{
  static const char* NS_STRING_TRUE = "true";
  nsString disabled;
  nsString menuitemName;
  nsString menuitemCmd;

  menuitemElement->GetAttribute(NS_ConvertASCIItoUCS2("disabled"), disabled);
  menuitemElement->GetAttribute(NS_ConvertASCIItoUCS2("value"), menuitemName);
  menuitemElement->GetAttribute(NS_ConvertASCIItoUCS2("cmd"), menuitemCmd);
  // Create nsMenuItem
  nsIMenuItem * pnsMenuItem = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuItemCID, nsnull, NS_GET_IID(nsIMenuItem), (void**)&pnsMenuItem);
  if (NS_OK == rv) {
    pnsMenuItem->Create(pParentMenu, menuitemName, 0);   
	
    nsISupports * supports = nsnull;
    pnsMenuItem->QueryInterface(NS_GET_IID(nsISupports), (void**) &supports);
    pParentMenu->AddItem(supports); // Parent should now own menu item
    NS_RELEASE(supports);
          
    // Create MenuDelegate - this is the intermediator inbetween 
    // the DOM node and the nsIMenuItem
    // The nsWebShellWindow wacthes for Document changes and then notifies the 
    // the appropriate nsMenuDelegate object
    nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(menuitemNode));
    if (!domElement) {
		return;
    }
    
    nsAutoString cmdAtom; cmdAtom.AssignWithConversion("oncommand");
    nsString cmdName;

    domElement->GetAttribute(cmdAtom, cmdName);

    pnsMenuItem->SetCommand(cmdName);
	// DO NOT use passed in wehshell because of messed up windows dynamic loading
	// code. 
    pnsMenuItem->SetWebShell(mWebShell);
    pnsMenuItem->SetDOMElement(domElement);
    pnsMenuItem->SetDOMNode(menuitemNode);

	if(disabled.EqualsWithConversion(NS_STRING_TRUE) )
		::DisableMenuItem(mMacMenuHandle, menuitemIndex + 1);
    else
    	::EnableMenuItem(mMacMenuHandle, menuitemIndex + 1);
		
	NS_RELEASE(pnsMenuItem);
  } 
  return;
}

//----------------------------------------
void nsContextMenu::LoadSubMenu(
  nsIContextMenu * pParentMenu,
  nsIDOMElement * menuElement,
  nsIDOMNode    * menuNode)
{
  nsString menuName;
  menuElement->GetAttribute(NS_ConvertASCIItoUCS2("value"), menuName);
  //PRINTF("Creating Menu [%s] \n", menuName.ToNewCString()); // this leaks

  // Create nsMenu
  nsIMenu * pnsMenu = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuCID, nsnull, NS_GET_IID(nsIMenu), (void**)&pnsMenu);
  if (NS_OK == rv) {
    // Call Create
    nsISupports * supports = nsnull;
    pParentMenu->QueryInterface(NS_GET_IID(nsISupports), (void**) &supports);
    pnsMenu->Create(supports, menuName);
    NS_RELEASE(supports); // Balance QI

    // Set nsMenu Name
    pnsMenu->SetLabel(menuName); 

    // Make nsMenu a child of parent nsMenu. The parent takes ownership
    supports = nsnull;
    pnsMenu->QueryInterface(NS_GET_IID(nsISupports), (void**) &supports);
	pParentMenu->AddItem(supports);
	NS_RELEASE(supports);

	pnsMenu->SetWebShell(mWebShell);
	pnsMenu->SetDOMNode(menuNode);
	pnsMenu->SetDOMElement(menuElement);

	// We're done with the menu
	NS_RELEASE(pnsMenu);
  }     
}

void nsContextMenu::NSStringSetMenuItemText(MenuHandle macMenuHandle, short menuItem, nsString& menuString)
{
	OSErr					err;
	const PRUnichar*		unicodeText;
	char*					scriptRunText;
	size_t					unicodeTextLengthInBytes, unicdeTextReadInBytes,
							scriptRunTextSizeInBytes, scriptRunTextLengthInBytes,
							scriptCodeRunListLength;
	ScriptCodeRun			convertedTextScript;
	long					scriptMgrVariable;
	
	//
	// extract the Unicode text from the nsString and convert it into a single script run
	//
	unicodeText = menuString.GetUnicode();
	unicodeTextLengthInBytes = menuString.Length() * sizeof(PRUnichar);
	scriptRunTextSizeInBytes = unicodeTextLengthInBytes * 2;
	scriptRunText = new char[scriptRunTextSizeInBytes];
	
	err = ::ConvertFromUnicodeToScriptCodeRun(mUnicodeTextRunConverter,
				unicodeTextLengthInBytes,unicodeText,
				0, /* no flags*/
				0,NULL,NULL,NULL, /* no offset arrays */
				scriptRunTextSizeInBytes,&unicdeTextReadInBytes,&scriptRunTextLengthInBytes,
				scriptRunText,
				1 /* count of script runs*/,&scriptCodeRunListLength,&convertedTextScript);
	NS_ASSERTION(err==noErr,"nsMenu::NSStringSetMenuItemText: ConvertFromUnicodeToScriptCodeRun failed.");
	if (err!=noErr) { delete [] scriptRunText; return; }
	scriptRunText[scriptRunTextLengthInBytes] = 0;	// null terminate
	
	//
	// get a font from the script code
	//
	scriptMgrVariable = ::GetScriptVariable(convertedTextScript.script,smScriptAppFondSize);
	::SetMenuItemText(macMenuHandle,menuItem,c2pstr(scriptRunText));
	err = ::SetMenuItemFontID(macMenuHandle,menuItem,HiWord(scriptMgrVariable));
	NS_ASSERTION(err==noErr,"nsMenu::NSStringSetMenuItemText: SetMenuItemFontID failed.");
	
	//
	// clean up and exit
	//
	delete [] scriptRunText;
			
}
	
