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
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsINameSpaceManager.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"
#include "nsIContent.h"

#include "nsMenuBarX.h"
#include "nsMenuX.h"
#include "nsDynamicMDEF.h"

#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsString.h"
#include "nsStringUtil.h"
#include "nsIStringBundle.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentObserver.h"

#include "nsIDOMXULDocument.h"
#include "nsWidgetAtoms.h"

#include <Menus.h>
#include <TextUtils.h>
#include <Balloons.h>
#include <Traps.h>
#include <Resources.h>
#include <Appearance.h>
#include "nsMacResources.h"

// CIDs
#include "nsWidgetsCID.h"
static NS_DEFINE_CID(kMenuBarCID, NS_MENUBAR_CID);
static NS_DEFINE_CID(kMenuCID, NS_MENU_CID);
static NS_DEFINE_CID(kMenuItemCID, NS_MENUITEM_CID);

NS_IMPL_ISUPPORTS5(nsMenuBarX, nsIMenuBar, nsIMenuListener, nsIDocumentObserver, nsIChangeManager, nsISupportsWeakReference)

static MenuRef gDefaultRootMenu = nsnull;

//
// nsMenuBarX constructor
//
nsMenuBarX::nsMenuBarX()
{
  NS_INIT_REFCNT();
  mNumMenus       = 0;
  mParent         = nsnull;
  mIsMenuBarAdded = PR_FALSE;
  mDocument       = nsnull;

  OSStatus status = ::CreateNewMenu(0, 0, &mRootMenu);
  NS_ASSERTION(status == noErr, "nsMenuBarX::nsMenuBarX:  creation of root menu failed.");

  if (gDefaultRootMenu == nsnull) {
    gDefaultRootMenu = ::AcquireRootMenu();
    NS_ASSERTION(gDefaultRootMenu != nsnull, "nsMenuBarX::nsMenuBarX:  no default root menu!.");
  }
}

//
// nsMenuBarX destructor
//
nsMenuBarX::~nsMenuBarX()
{
    mMenusArray.Clear();    // release all menus

    // make sure we unregister ourselves as a document observer
    if ( mDocument ) {
        nsCOMPtr<nsIDocumentObserver> observer ( do_QueryInterface(NS_STATIC_CAST(nsIMenuBar*,this)) );
        mDocument->RemoveObserver(observer);
    }

    if (mRootMenu != NULL) {
        NS_ASSERTION(gDefaultRootMenu != nsnull, "nsMenuBarX::~nsMenuBarX:  no default root menu!.");
        ::SetRootMenu(gDefaultRootMenu);
        ::ReleaseMenu(mRootMenu);
    }
}

nsEventStatus 
nsMenuBarX::MenuItemSelected(const nsMenuEvent & aMenuEvent)
{
  // Dispatch menu event
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  PRUint32  numItems;
  mMenusArray.Count(&numItems);
  
  for (PRUint32 i = numItems; i > 0; --i)
  {
    nsCOMPtr<nsISupports>     menuSupports = getter_AddRefs(mMenusArray.ElementAt(i - 1));
    nsCOMPtr<nsIMenuListener> menuListener = do_QueryInterface(menuSupports);
    if(menuListener)
    {
      eventStatus = menuListener->MenuItemSelected(aMenuEvent);
      if (nsEventStatus_eIgnore != eventStatus)
        return eventStatus;
    }
  }
  return eventStatus;
}


nsEventStatus 
nsMenuBarX::MenuSelected(const nsMenuEvent & aMenuEvent)
{
  // Dispatch event
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

  nsCOMPtr<nsIMenuListener> menuListener;
  //((nsISupports*)mMenuVoidArray[i-1])->QueryInterface(NS_GET_IID(nsIMenuListener), (void**)&menuListener);
  //printf("gPreviousMenuStack.Count() = %d \n", gPreviousMenuStack.Count());
#if !TARGET_CARBON
  nsCOMPtr<nsIMenu> theMenu;
  gPreviousMenuStack.GetMenuAt(gPreviousMenuStack.Count() - 1, getter_AddRefs(theMenu));
  menuListener = do_QueryInterface(theMenu);
#endif
  if (menuListener) {
    //TODO: MenuSelected is the right thing to call...
    //eventStatus = menuListener->MenuSelected(aMenuEvent);
    eventStatus = menuListener->MenuItemSelected(aMenuEvent);
    if (nsEventStatus_eIgnore != eventStatus)
      return eventStatus;
  } else {
    // If it's the help menu, gPreviousMenuStack won't be accurate so we need to get the listener a different way 
    // We'll do it the old fashioned way of looping through and finding it
    PRUint32  numItems;
    mMenusArray.Count(&numItems);
    for (PRUint32 i = numItems; i > 0; --i)
    {
      nsCOMPtr<nsISupports>     menuSupports = getter_AddRefs(mMenusArray.ElementAt(i - 1));
      nsCOMPtr<nsIMenuListener> thisListener = do_QueryInterface(menuSupports);
	    if (thisListener)
	    {
        //TODO: MenuSelected is the right thing to call...
  	    //eventStatus = menuListener->MenuSelected(aMenuEvent);
  	    eventStatus = thisListener->MenuItemSelected(aMenuEvent);
  	    if(nsEventStatus_eIgnore != eventStatus)
  	      return eventStatus;
      }
    }
  }
  return eventStatus;
}


nsEventStatus 
nsMenuBarX::MenuDeselected(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}

nsEventStatus 
nsMenuBarX::CheckRebuild(PRBool & aNeedsRebuild)
{
  aNeedsRebuild = PR_TRUE;
  return nsEventStatus_eIgnore;
}

nsEventStatus
nsMenuBarX::SetRebuild(PRBool aNeedsRebuild)
{
  return nsEventStatus_eIgnore;
}

void
nsMenuBarX :: GetDocument ( nsIWebShell* inWebShell, nsIDocument** outDocument )
{
  *outDocument = nsnull;
  
  nsCOMPtr<nsIDocShell> docShell ( do_QueryInterface(inWebShell) );
  nsCOMPtr<nsIContentViewer> cv;
  if ( docShell ) {
    docShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      // get the document
      nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
      if (!docv)
        return;
      docv->GetDocument(*outDocument);    // addrefs
    }
  }
}


//
// RegisterAsDocumentObserver
//
// Name says it all.
//
void
nsMenuBarX :: RegisterAsDocumentObserver ( nsIWebShell* inWebShell )
{
  nsCOMPtr<nsIDocument> doc;
  GetDocument(inWebShell, getter_AddRefs(doc));
  if (!doc)
    return;

  // register ourselves
  nsCOMPtr<nsIDocumentObserver> observer ( do_QueryInterface(NS_STATIC_CAST(nsIMenuBar*,this)) );
  doc->AddObserver(observer);
  // also get pointer to doc, just in case webshell goes away
  // we can still remove ourself as doc observer directly from doc
  mDocument = doc;
} // RegisterAsDocumentObesrver


nsEventStatus
nsMenuBarX::MenuConstruct( const nsMenuEvent & aMenuEvent, nsIWidget* aParentWindow, 
                            void * menubarNode, void * aWebShell )
{
  mWebShellWeakRef = getter_AddRefs(NS_GetWeakReference(NS_STATIC_CAST(nsIWebShell*, aWebShell)));
  nsIDOMNode* aDOMNode  = NS_STATIC_CAST(nsIDOMNode*, menubarNode);
  mMenuBarContent = do_QueryInterface(aDOMNode);           // strong ref

  Create(aParentWindow);

  nsCOMPtr<nsIWebShell> webShell = do_QueryReferent(mWebShellWeakRef);
  if (webShell) RegisterAsDocumentObserver(webShell);

  // set this as a nsMenuListener on aParentWindow
  aParentWindow->AddMenuListener((nsIMenuListener *)this);

  PRInt32 count;
  mMenuBarContent->ChildCount(count);
  for ( int i = 0; i < count; ++i ) { 
    nsCOMPtr<nsIContent> menu;
    mMenuBarContent->ChildAt ( i, *getter_AddRefs(menu) );
    if ( menu ) {
      nsCOMPtr<nsIAtom> tag;
      menu->GetTag ( *getter_AddRefs(tag) );
      if (tag == nsWidgetAtoms::menu) {
        nsAutoString menuName;
        nsAutoString menuAccessKey(NS_LITERAL_STRING(" "));
        menu->GetAttribute(kNameSpaceID_None, nsWidgetAtoms::label, menuName);
        menu->GetAttribute(kNameSpaceID_None, nsWidgetAtoms::accesskey, menuAccessKey);
			  
        // Don't create the whole menu yet, just add in the top level names
              
        // Create nsMenu, the menubar will own it
        nsCOMPtr<nsIMenu> pnsMenu ( do_CreateInstance(kMenuCID) );
        if ( pnsMenu ) {
          pnsMenu->Create(NS_STATIC_CAST(nsIMenuBar*, this), menuName, menuAccessKey, 
                          NS_STATIC_CAST(nsIChangeManager *, this), 
                          NS_REINTERPRET_CAST(nsIWebShell*, aWebShell), menu);

          // Make nsMenu a child of nsMenuBar. nsMenuBar takes ownership
          AddMenu(pnsMenu); 
                  
          nsAutoString menuIDstring;
          menu->GetAttribute(kNameSpaceID_None, nsWidgetAtoms::id, menuIDstring);
          if ( menuIDstring == NS_LITERAL_STRING("menu_Help") ) {
            nsMenuEvent event;
            MenuHandle handle = nsnull;
#if !TARGET_CARBON
            ::HMGetHelpMenuHandle(&handle);
#endif
            event.mCommand = (unsigned int) handle;
            nsCOMPtr<nsIMenuListener> listener(do_QueryInterface(pnsMenu));
            listener->MenuSelected(event);
          }          
        }
      } 
    }
  } // for each menu

  // Give the aParentWindow this nsMenuBarX to hold onto.
  // The parent takes ownership
  aParentWindow->SetMenuBar(this);

  return nsEventStatus_eIgnore;
}


nsEventStatus 
nsMenuBarX::MenuDestruct(const nsMenuEvent & aMenuEvent)
{
  return nsEventStatus_eIgnore;
}


//-------------------------------------------------------------------------
//
// Create the proper widget
//
//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::Create(nsIWidget *aParent)
{
  SetParent(aParent);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::GetParent(nsIWidget *&aParent)
{
  NS_IF_ADDREF(aParent = mParent);
  return NS_OK;
}


//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::SetParent(nsIWidget *aParent)
{
  mParent = aParent;    // weak ref  
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::AddMenu(nsIMenu * aMenu)
{
  // keep track of all added menus.
  mMenusArray.AppendElement(aMenu);    // owner

  if (mNumMenus == 0)
  {
    Str32 menuStr = { 1, kMenuAppleLogoFilledGlyph };
    MenuHandle appleMenu;
    OSStatus s = ::CreateNewMenu(kAppleMenuID, 0, &appleMenu);
    ::SetMenuTitle(appleMenu, menuStr);

    if (appleMenu)
    {
      // this code reads the "label" attribute from the <menuitem/> with
      // id="aboutName" and puts its label in the Apple Menu
      nsAutoString label;
      nsCOMPtr<nsIContent> menu;
      aMenu->GetMenuContent(getter_AddRefs(menu));
      if (menu) {
        nsCOMPtr<nsIDocument> doc;
        menu->GetDocument(*getter_AddRefs(doc));
        if (doc) {
          nsCOMPtr<nsIDOMDocument> domdoc ( do_QueryInterface(doc) );
          if ( domdoc ) {
            nsCOMPtr<nsIDOMElement> aboutMenuItem;
            domdoc->GetElementById(NS_LITERAL_STRING("aboutName"), getter_AddRefs(aboutMenuItem));
            if (aboutMenuItem)
              aboutMenuItem->GetAttribute(NS_LITERAL_STRING("label"), label);
          }
        }
      }

      CFStringRef labelRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)label.get(), label.Length());
      ::InsertMenuItemTextWithCFString(appleMenu, labelRef, 1, 0, 0);
      ::CFRelease(labelRef);

      // InsertMenuItem() is 1-based, so the apple/application menu needs to
      // be at index 1. |mNumMenus| will be incremented below, so the following menu (File)
      // won't overwrite the apple menu by reusing the ID.
      mNumMenus = 1;
      ::AppendMenu(appleMenu, "\p-");
      ::InsertMenuItem(mRootMenu, menuStr, mNumMenus);
      OSStatus status = ::SetMenuItemHierarchicalMenu(mRootMenu, mNumMenus, appleMenu);
    }
  }

  MenuRef menuRef = nsnull;
  aMenu->GetNativeData((void**)&menuRef);

  PRBool helpMenu;
  aMenu->IsHelpMenu(&helpMenu);
  if(!helpMenu) {
    nsCOMPtr<nsIContent> menu;
    aMenu->GetMenuContent(getter_AddRefs(menu));
    nsAutoString menuHidden;
    menu->GetAttribute(kNameSpaceID_None, nsWidgetAtoms::hidden, menuHidden);
    if( menuHidden != NS_LITERAL_STRING("true")) {
    	// make sure we only increment |mNumMenus| if the menu is visible, since
    	// we use it as an index of where to insert the next menu.
      mNumMenus++;
      
      Str255 title;
      ::InsertMenuItem(mRootMenu, ::GetMenuTitle(menuRef, title), mNumMenus);
      OSStatus status = ::SetMenuItemHierarchicalMenu(mRootMenu, mNumMenus, menuRef);
      NS_ASSERTION(status == noErr, "nsMenuBarX::AddMenu: SetMenuItemHierarchicalMenu failed.");
    }
  }

  return NS_OK;
}

                                
//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::GetMenuCount(PRUint32 &aCount)
{
  aCount = mNumMenus;
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::GetMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{ 
  aMenu = NULL;
  nsCOMPtr<nsISupports> supports = getter_AddRefs(mMenusArray.ElementAt(aCount));
  if (!supports) return NS_OK;
  
  return CallQueryInterface(supports, &aMenu); // addref
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::InsertMenuAt(const PRUint32 aCount, nsIMenu *& aMenu)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::RemoveMenu(const PRUint32 aCount)
{
  mMenusArray.RemoveElementAt(aCount);
  ::DrawMenuBar();
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::RemoveAll()
{
  NS_ASSERTION(0, "Not implemented!");
  // mMenusArray.Clear();    // maybe?
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::GetNativeData(void *& aData)
{
    aData = (void *) mRootMenu;
    return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::SetNativeData(void* aData)
{
#if 0
    Handle menubarHandle = (Handle)aData;
    if (mMacMBarHandle && mMacMBarHandle != menubarHandle)
        ::DisposeHandle(mMacMBarHandle);
    mMacMBarHandle = menubarHandle;
#endif
    return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsMenuBarX::Paint()
{
    // hack to correctly swap menu bars.
    // hopefully this is fast enough.
    ::SetRootMenu(mRootMenu);
    ::DrawMenuBar();
    return NS_OK;
}

#pragma mark -

//
// nsIDocumentObserver
// this is needed for menubar changes
//


NS_IMETHODIMP
nsMenuBarX::BeginUpdate( nsIDocument * aDocument )
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::EndUpdate( nsIDocument * aDocument )
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::BeginLoad( nsIDocument * aDocument )
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::EndLoad( nsIDocument * aDocument )
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::BeginReflow(  nsIDocument * aDocument, nsIPresShell * aShell)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::EndReflow( nsIDocument * aDocument, nsIPresShell * aShell)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentChanged( nsIDocument * aDocument, nsIContent * aContent, nsISupports * aSubContent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentStatesChanged( nsIDocument * aDocument, nsIContent  * aContent1, nsIContent  * aContent2)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentAppended( nsIDocument * aDocument, nsIContent  * aContainer,
                              PRInt32 aNewIndexInContainer)
{
  if ( aContainer == mMenuBarContent ) {
    //Register(aContainer, );
    //InsertMenu ( aNewIndexInContainer );
  }
  else {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup ( aContainer, getter_AddRefs(obs) );
    if ( obs )
      obs->ContentInserted ( aDocument, aContainer, aNewIndexInContainer );
    else {
      nsCOMPtr<nsIContent> parent;
      aContainer->GetParent(*getter_AddRefs(parent));
      if(parent) {
        Lookup ( parent, getter_AddRefs(obs) );
        if ( obs )
          obs->ContentInserted ( aDocument, aContainer, aNewIndexInContainer );
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentReplaced( nsIDocument * aDocument, nsIContent * aContainer, nsIContent * aOldChild,
                          nsIContent * aNewChild, PRInt32 aIndexInContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleSheetAdded( nsIDocument * aDocument, nsIStyleSheet * aStyleSheet)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleSheetRemoved(nsIDocument * aDocument, nsIStyleSheet * aStyleSheet)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleSheetDisabledStateChanged(nsIDocument * aDocument, nsIStyleSheet * aStyleSheet,
                                            PRBool aDisabled)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleRuleChanged( nsIDocument * aDocument, nsIStyleSheet * aStyleSheet,
                              nsIStyleRule * aStyleRule, PRInt32 aHint)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleRuleAdded( nsIDocument * aDocument, nsIStyleSheet * aStyleSheet,
                            nsIStyleRule * aStyleRule)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::StyleRuleRemoved(nsIDocument * aDocument, nsIStyleSheet * aStyleSheet,
                              nsIStyleRule  * aStyleRule)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::DocumentWillBeDestroyed( nsIDocument * aDocument )
{
  mDocument = nsnull;
  return NS_OK;
}


NS_IMETHODIMP
nsMenuBarX::AttributeChanged( nsIDocument * aDocument, nsIContent * aContent, PRInt32 aNameSpaceID,
                              nsIAtom * aAttribute, PRInt32 aHint)
{
  // lookup and dispatch to registered thang.
  nsCOMPtr<nsIChangeObserver> obs;
  Lookup ( aContent, getter_AddRefs(obs) );
  if ( obs )
    obs->AttributeChanged ( aDocument, aNameSpaceID, aAttribute, aHint );

  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentRemoved( nsIDocument * aDocument, nsIContent * aContainer,
                            nsIContent * aChild, PRInt32 aIndexInContainer )
{  
  if ( aContainer == mMenuBarContent ) {
    Unregister(aChild);
    RemoveMenu ( aIndexInContainer );
  }
  else {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup ( aContainer, getter_AddRefs(obs) );
    if ( obs )
      obs->ContentRemoved ( aDocument, aChild, aIndexInContainer );
    else {
      nsCOMPtr<nsIContent> parent;
      aContainer->GetParent(*getter_AddRefs(parent));
      if(parent) {
        Lookup ( parent, getter_AddRefs(obs) );
        if ( obs )
          obs->ContentRemoved ( aDocument, aChild, aIndexInContainer );
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMenuBarX::ContentInserted( nsIDocument * aDocument, nsIContent * aContainer,
                            nsIContent * aChild, PRInt32 aIndexInContainer )
{  
  if ( aContainer == mMenuBarContent ) {
    //Register(aChild, );
    //InsertMenu ( aIndexInContainer );
  }
  else {
    nsCOMPtr<nsIChangeObserver> obs;
    Lookup ( aContainer, getter_AddRefs(obs) );
    if ( obs )
      obs->ContentInserted ( aDocument, aChild, aIndexInContainer );
    else {
      nsCOMPtr<nsIContent> parent;
      aContainer->GetParent(*getter_AddRefs(parent));
      if(parent) {
        Lookup ( parent, getter_AddRefs(obs) );
        if ( obs )
          obs->ContentInserted ( aDocument, aChild, aIndexInContainer );
      }
    }
  }
  return NS_OK;
}

#pragma mark - 

//
// nsIChangeManager
//
// We don't use a |nsSupportsHashtable| because we know that the lifetime of all these items
// is bouded by the lifetime of the menubar. No need to add any more strong refs to the
// picture because the containment hierarchy already uses strong refs.
//

NS_IMETHODIMP 
nsMenuBarX :: Register ( nsIContent *aContent, nsIChangeObserver *aMenuObject )
{
  nsVoidKey key ( aContent );
  mObserverTable.Put ( &key, aMenuObject );
  
  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarX :: Unregister ( nsIContent *aContent )
{
  nsVoidKey key ( aContent );
  mObserverTable.Remove ( &key );
  
  return NS_OK;
}


NS_IMETHODIMP 
nsMenuBarX :: Lookup ( nsIContent *aContent, nsIChangeObserver **_retval )
{
  *_retval = nsnull;
  
  nsVoidKey key ( aContent );
  *_retval = NS_REINTERPRET_CAST(nsIChangeObserver*, mObserverTable.Get(&key));
  NS_IF_ADDREF ( *_retval );
  
  return NS_OK;
}


#pragma mark -


//
// WebShellToPresContext
//
// Helper to dig out a pres context from a webshell. A common thing to do before
// sending an event into the dom.
//
nsresult
MenuHelpersX::WebShellToPresContext (nsIWebShell* inWebShell, nsIPresContext** outContext )
{
  NS_ENSURE_ARG_POINTER(outContext);
  *outContext = nsnull;
  if (!inWebShell)
    return NS_ERROR_INVALID_ARG;
  
  nsresult retval = NS_OK;
  
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(inWebShell));

  nsCOMPtr<nsIContentViewer> contentViewer;
  docShell->GetContentViewer(getter_AddRefs(contentViewer));
  if ( contentViewer ) {
    nsCOMPtr<nsIDocumentViewer> docViewer ( do_QueryInterface(contentViewer) );
    if ( docViewer )
      docViewer->GetPresContext(*outContext);     // AddRefs for us
    else
      retval = NS_ERROR_FAILURE;
  }
  else
    retval = NS_ERROR_FAILURE;
  
  return retval;
  
} // WebShellToPresContext



