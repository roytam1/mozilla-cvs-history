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

#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"

#include "nsMenuBar.h"
#include "nsDynamicMDEF.h"

#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsString.h"
#include "nsStringUtil.h"

#include <Menus.h>
#include <TextUtils.h>
#include <Balloons.h>
#include <Traps.h>
#include <Resources.h>
#include "nsMacResources.h"

#pragma options align=mac68k
typedef struct {
	short 	jmpInstr;
	Ptr 	jmpAddr;
} JmpRecord, *JmpPtr, **JmpHandle;
#pragma options align=reset

Handle       gMDEF = nsnull;
Handle       gSystemMDEFHandle = nsnull;
nsIMenuBar * gMacMenubar = nsnull;
bool         gFirstMenuBar = true; 

// The four Golden Hierarchical Child Menus
MenuHandle gLevel2HierMenu = nsnull;
MenuHandle gLevel3HierMenu = nsnull;
MenuHandle gLevel4HierMenu = nsnull;
MenuHandle gLevel5HierMenu = nsnull;

#if !TARGET_CARBON
extern nsVoidArray gPreviousMenuStack;
#endif
extern PRInt16 gCurrentMenuDepth;

// #if APPLE_MENU_HACK
#include "nsMenu.h"				// need to get APPLE_MENU_HACK macro
// #endif

// CIDs
#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kMenuBarCID, NS_MENUBAR_CID);
static NS_DEFINE_CID(kMenuCID, NS_MENU_CID);
static NS_DEFINE_CID(kMenuItemCID, NS_MENUITEM_CID);

void InstallDefProc(
  short dpPath, 
  ResType dpType, 
  short dpID, 
  Ptr dpAddr);
  
//-------------------------------------------------------------------------
NS_IMPL_ISUPPORTS2(nsMenuBar, nsIMenuBar, nsIMenuListener)

//-------------------------------------------------------------------------
//
// nsMenuListener interface
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  // Dispatch menu event
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  for (int i = mMenuVoidArray.Count(); i > 0; --i)
  {
    nsIMenuListener * menuListener = nsnull;
    ((nsISupports*)mMenuVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
    if(menuListener){
      eventStatus = menuListener->MenuItemSelected(aMenuEvent);
      NS_RELEASE(menuListener);
      if(nsEventStatus_eIgnore != eventStatus)
        return eventStatus;
    }
  }
  return eventStatus;
}

//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  // Dispatch event
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  nsIMenuListener * menuListener = nsnull;
  //((nsISupports*)mMenuVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
  //printf("gPreviousMenuStack.Count() = %d \n", gPreviousMenuStack.Count());
#if !TARGET_CARBON
  if (gPreviousMenuStack[gPreviousMenuStack.Count() - 1])
    ((nsIMenu*)gPreviousMenuStack[gPreviousMenuStack.Count() - 1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
#endif
  if (menuListener) {
    //TODO: MenuSelected is the right thing to call...
    //eventStatus = menuListener->MenuSelected(aMenuEvent);
    eventStatus = menuListener->MenuItemSelected(aMenuEvent);
    NS_RELEASE(menuListener);
    if (nsEventStatus_eIgnore != eventStatus)
      return eventStatus;
  } else {
    // If it's the help menu, gPreviousMenuStack won't be accurate so we need to get the listener a different way 
    // We'll do it the old fashioned way of looping through and finding it
    for (int i = mMenuVoidArray.Count(); i > 0; --i) {
      ((nsISupports*)mMenuVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
	  if (menuListener) {
        //TODO: MenuSelected is the right thing to call...
	    //eventStatus = menuListener->MenuSelected(aMenuEvent);
	    eventStatus = menuListener->MenuItemSelected(aMenuEvent);
	    NS_RELEASE(menuListener);
	    if(nsEventStatus_eIgnore != eventStatus)
	      return eventStatus;
      }
    }
  }
  return eventStatus;
}

//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuConstruct(
    const nsMenuEvent & aMenuEvent,
    nsIWidget         * aParentWindow, 
    void              * menubarNode,
	void              * aWebShell)
{
    mWebShell = (nsIWebShell*) aWebShell;
    NS_ADDREF(mWebShell);
	mDOMNode  = (nsIDOMNode*)menubarNode;
	NS_ADDREF(mDOMNode);
    
    nsIMenuBar * pnsMenuBar = this;
    
    if(gFirstMenuBar){
	        gFirstMenuBar = false;
	        // Add the 4 Golden Hierarchical Menus to the MenuList        
				
	        MenuHandle macMenuHandle = ::NewMenu(2, "\psubmenu");
	        if(macMenuHandle) { 
	          gLevel2HierMenu = macMenuHandle;
#if !TARGET_CARBON
	          SInt8 state = ::HGetState((Handle)macMenuHandle);
	          ::HLock((Handle)macMenuHandle);
			  gSystemMDEFHandle = (**macMenuHandle).menuProc;
			  (**macMenuHandle).menuProc = gMDEF;
			  (**macMenuHandle).menuWidth = -1;
			  (**macMenuHandle).menuHeight = -1;
			  ::HSetState((Handle)macMenuHandle, state);
#endif
			  ::InsertMenu(macMenuHandle, hierMenu);
			}
			
	        macMenuHandle = ::NewMenu(3, "\psubmenu");
	        if(macMenuHandle) { 
	          gLevel3HierMenu = macMenuHandle;
#if !TARGET_CARBON
	          SInt8 state = ::HGetState((Handle)macMenuHandle);
	          ::HLock((Handle)macMenuHandle);
			  gSystemMDEFHandle = (**macMenuHandle).menuProc;
			  (**macMenuHandle).menuProc = gMDEF;
			  (**macMenuHandle).menuWidth = -1;
			  (**macMenuHandle).menuHeight = -1;
			  ::HSetState((Handle)macMenuHandle, state);
#endif
			  ::InsertMenu(macMenuHandle, hierMenu);
			}
			
	        macMenuHandle = ::NewMenu(4, "\psubmenu");
	        if(macMenuHandle) { 
	          gLevel4HierMenu = macMenuHandle;
#if !TARGET_CARBON
	          SInt8 state = ::HGetState((Handle)macMenuHandle);
	          ::HLock((Handle)macMenuHandle);
			  gSystemMDEFHandle = (**macMenuHandle).menuProc;
			  (**macMenuHandle).menuProc = gMDEF;
			  (**macMenuHandle).menuWidth = -1;
			  (**macMenuHandle).menuHeight = -1;
			  ::HSetState((Handle)macMenuHandle, state);
#endif
			  ::InsertMenu(macMenuHandle, hierMenu);
			}
			
	        macMenuHandle = ::NewMenu(5, "\psubmenu");
	        if(macMenuHandle) { 
	          gLevel5HierMenu = macMenuHandle; 
#if !TARGET_CARBON
	          SInt8 state = ::HGetState((Handle)macMenuHandle);
	          ::HLock((Handle)macMenuHandle);
			  gSystemMDEFHandle = (**macMenuHandle).menuProc;
			  (**macMenuHandle).menuProc = gMDEF;
			  (**macMenuHandle).menuWidth = -1;
			  (**macMenuHandle).menuHeight = -1;
			  ::HSetState((Handle)macMenuHandle, state);
#endif
			  ::InsertMenu(macMenuHandle, hierMenu);
			}
		} else {
		  ::InsertMenu(gLevel2HierMenu, hierMenu);
		  ::InsertMenu(gLevel3HierMenu, hierMenu);
		  ::InsertMenu(gLevel4HierMenu, hierMenu);
		  ::InsertMenu(gLevel5HierMenu, hierMenu);
		}
    
    nsresult rv;
    //nsresult rv = nsComponentManager::CreateInstance(kMenuBarCID, nsnull, NS_GET_IID(nsIMenuBar), (void**)&pnsMenuBar);
    //if (NS_OK == rv) {
      //if (nsnull != pnsMenuBar) {
        pnsMenuBar->Create(aParentWindow);
      
        // set pnsMenuBar as a nsMenuListener on aParentWindow
        nsCOMPtr<nsIMenuListener> menuListener;
        pnsMenuBar->QueryInterface(NS_GET_IID(nsIMenuListener), getter_AddRefs(menuListener));
        aParentWindow->AddMenuListener(menuListener);
         
        //nsCOMPtr<nsIDOMNode> menuNode;
        //((nsIDOMNode*)menubarNode)->GetFirstChild(getter_AddRefs(menuNode));
        nsIDOMNode * menuNode = nsnull;
        ((nsIDOMNode*)menubarNode)->GetFirstChild(&menuNode);
        while (menuNode) {
          NS_ADDREF(menuNode);
          
          nsCOMPtr<nsIDOMElement> menuElement(do_QueryInterface(menuNode));
          if (menuElement) {
            nsString menuNodeType;
            nsString menuName;
			nsString menuAccessKey = " ";
            menuElement->GetNodeName(menuNodeType);
            if (menuNodeType.Equals("menu")) {
              menuElement->GetAttribute(nsAutoString("value"), menuName);
			  menuElement->GetAttribute(nsAutoString("accesskey"), menuAccessKey);
              // Don't create the whole menu yet, just add in the top level names
              
                // Create nsMenu
                nsIMenu * pnsMenu = nsnull;
                rv = nsComponentManager::CreateInstance(kMenuCID, nsnull, NS_GET_IID(nsIMenu), (void**)&pnsMenu);
                if (NS_OK == rv) {
                  // Call Create
                  nsISupports * supports = nsnull;
                  pnsMenuBar->QueryInterface(NS_GET_IID(nsISupports), (void**) &supports);
                  pnsMenu->Create(supports, menuName);
                  NS_RELEASE(supports);

				  // Set JavaScript execution parameters
				  pnsMenu->SetDOMNode(menuNode);
				  pnsMenu->SetDOMElement(menuElement);
				  pnsMenu->SetWebShell((nsIWebShell*)aWebShell);

                  // Set nsMenu Name
                  pnsMenu->SetLabel(menuName); 
				  // Set the access key
				  pnsMenu->SetAccessKey(menuAccessKey);
                  // Make nsMenu a child of nsMenuBar. nsMenuBar takes ownership
                  pnsMenuBar->AddMenu(pnsMenu); 
                  
                  nsString menuIDstring;
                  menuElement->GetAttribute(nsAutoString("id"), menuIDstring);
                  if(menuIDstring == "menu_Help") {
                    nsMenuEvent event;
                    MenuHandle handle;
#ifndef RHAPSODY
                    ::HMGetHelpMenuHandle(&handle);
#endif
                    event.mCommand = (unsigned int) handle;
                    nsCOMPtr<nsIMenuListener> listener(do_QueryInterface(pnsMenu));
                    listener->MenuSelected(event);
                  }

                  // Release the menu now that the menubar owns it
                  NS_RELEASE(pnsMenu);
                }
             } 

          }
          //nsCOMPtr<nsIDOMNode> oldmenuNode(menuNode);  
          //oldmenuNode->GetNextSibling(getter_AddRefs(menuNode));
          nsCOMPtr<nsIDOMNode> oldmenuNode(do_QueryInterface(menuNode));  
          oldmenuNode->GetNextSibling(&menuNode);
        } // end while (nsnull != menuNode)
        
        // Give the aParentWindow this nsMenuBar to hold onto.
		// The parent takes ownership
        aParentWindow->SetMenuBar(pnsMenuBar);
      
        // HACK: force a paint for now
        //pnsMenuBar->Paint();
		
        #ifdef XP_MAC
        Handle tempMenuBar = ::GetMenuBar(); // Get a copy of the menu list
		pnsMenuBar->SetNativeData((void*)tempMenuBar);
		#endif
		
        // We are done with the menubar
		//NS_RELEASE(pnsMenuBar);
    //} // end if ( nsnull != pnsMenuBar )
  //}
  
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
nsEventStatus nsMenuBar::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

//-------------------------------------------------------------------------
//
// nsMenuBar constructor
//
//-------------------------------------------------------------------------
nsMenuBar::nsMenuBar() : nsIMenuBar(), nsIMenuListener()
{
  gCurrentMenuDepth = 1;
#if !TARGET_CARBON
  nsPreviousMenuStackUnwind(nsnull, nsnull);
#endif
  NS_INIT_REFCNT();
  mNumMenus       = 0;
  mParent         = nsnull;
  mIsMenuBarAdded = PR_FALSE;
  mWebShell       = nsnull;
  mDOMNode        = nsnull;
  
#if !TARGET_CARBON
  MenuDefUPP mdef = NewMenuDefProc( nsDynamicMDEFMain );
  InstallDefProc((short)nsMacResources::GetLocalResourceFile(), (ResType)'MDEF', (short)128, (Ptr) mdef );
#endif

  mOriginalMacMBarHandle = nsnull;
  mMacMBarHandle = nsnull;
  
  mOriginalMacMBarHandle = ::GetMenuBar();
  Handle tmp = ::GetMenuBar();
  ::SetMenuBar(tmp);
  this->SetNativeData((void*)tmp);
  
  ::ClearMenuBar(); 
  gMacMenubar = this;
}

//-------------------------------------------------------------------------
//
// nsMenuBar destructor
//
//-------------------------------------------------------------------------
nsMenuBar::~nsMenuBar()
{
  //NS_IF_RELEASE(mParent);

  while(mNumMenus)
  {
    --mNumMenus;
    nsISupports* menu = (nsISupports*)mMenuVoidArray[mNumMenus];
    NS_IF_RELEASE( menu );
  }
  ::SetMenuBar(mOriginalMacMBarHandle);
  ::DisposeHandle(mMacMBarHandle);
  ::DisposeHandle(mOriginalMacMBarHandle);
}

//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::Create(nsIWidget *aParent)
{
  SetParent(aParent);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetParent(nsIWidget *&aParent)
{
  aParent = mParent;

  return NS_OK;
}


//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::SetParent(nsIWidget *aParent)
{
  mParent = aParent;
  
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::AddMenu(nsIMenu * aMenu)
{
  // XXX add to internal data structure
  nsISupports * supports = nsnull;
  aMenu->QueryInterface(NS_GET_IID(nsISupports), (void**)&supports);
  if(supports){
    mMenuVoidArray.AppendElement( supports );
  }

#ifdef APPLE_MENU_HACK
  if (mNumMenus == 0)
  {
  	Str32					menuStr = { 1, 0x14 };
  	MenuHandle		appleMenu = ::NewMenu(kAppleMenuID, menuStr);

		if (appleMenu)
		{
			// XXX This should come from a string bundle
		  ::AppendMenu(appleMenu, "\pAbout Mozilla�");
		  ::AppendMenu(appleMenu, "\p-");
		  ::AppendResMenu(appleMenu, 'DRVR');
      ::InsertMenu(appleMenu, 0);
    }
  }
#endif
  
  MenuHandle menuHandle = nsnull;
  aMenu->GetNativeData((void**)&menuHandle);
  
  mNumMenus++;
  PRBool helpMenu;
  aMenu->IsHelpMenu(&helpMenu);
  if(!helpMenu)
    ::InsertMenu(menuHandle, 0);
  
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetMenuCount(PRUint32 &aCount)
{
  aCount = mNumMenus;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{ 
  nsISupports * supports = nsnull;
  supports = (nsISupports*) mMenuVoidArray.ElementAt(aCount);
  if(!supports) { 
    return NS_OK;
  } 
  
  nsIMenu * menu = nsnull;
  supports->QueryInterface(NS_GET_IID(nsISupports), (void**)&menu);
  aMenu = menu;

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::RemoveMenu(const PRUint32 aCount)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::RemoveAll()
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::GetNativeData(void *& aData)
{
  aData = (void *) mMacMBarHandle;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::SetNativeData(void* aData)
{
  if(mMacMBarHandle) ::DisposeHandle(mMacMBarHandle);
  mMacMBarHandle = (Handle) aData;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBar::Paint()
{
  gMacMenubar = this;
  PRBool isHelpMenu;
  
  ::SetMenuBar(mMacMBarHandle);
  // Now we have blown away the merged Help menu, so we have to rebuild it
  for(int i = mMenuVoidArray.Count()-1; i>=0; --i) {
    ((nsIMenu*)mMenuVoidArray[i])->IsHelpMenu(&isHelpMenu);
    if(isHelpMenu){
      MenuHandle helpMenuHandle;
#if !TARGET_CARBON
      ::HMGetHelpMenuHandle(&helpMenuHandle);
      ((nsIMenu*)mMenuVoidArray[i])->SetNativeData((void*)helpMenuHandle);
#endif 
      nsMenuEvent event;
      event.mCommand = (unsigned int) helpMenuHandle;
      nsCOMPtr<nsIMenuListener> listener(do_QueryInterface((nsIMenu*)mMenuVoidArray[i]));
      listener->MenuSelected(event);
    }
  }
  ::DrawMenuBar();
  return NS_OK;
}
//-------------------------------------------------------------------------
void InstallDefProc(
  short dpPath, 
  ResType dpType, 
  short dpID, 
  Ptr dpAddr)
{
	JmpHandle 	jH;
	short 	savePath;

	savePath = CurResFile();
	UseResFile(dpPath);

	jH = (JmpHandle)GetResource(dpType, dpID);
	DetachResource((Handle)jH);
	gMDEF = (Handle) jH;
	UseResFile(savePath);

	if (!jH)				/* is there no defproc resource? */
			DebugStr("\pStub Defproc Not Found!");

	HNoPurge((Handle)jH);	/* make this resource nonpurgeable */
	(**jH).jmpAddr = dpAddr;
	(**jH).jmpInstr = 0x4EF9;
	//FlushCache();

	HLockHi((Handle)jH);
}
