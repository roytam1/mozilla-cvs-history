/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 *   Adam Lock <adamlock@netscape.com>
 *   Mike Pinkerton <pinkerton@netscape.com>
 */

// Local Includes
#include "nsDocShellTreeOwner.h"
#include "nsWebBrowser.h"

// Helper Classes
#include "nsIGenericFactory.h"
#include "nsStyleCoord.h"
#include "nsHTMLReflowState.h"

// Interfaces needed to be included
#include "nsIContextMenuListener.h"
#include "nsITooltipListener.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMHTMLElement.h"
#include "nsIPresShell.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIFocusController.h"
#include "nsIDOMWindowInternal.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWindowWatcher.h"
#include "nsPIWindowWatcher.h"

static char *sWindowWatcherContractID = "@mozilla.org/embedcomp/window-watcher;1";



//*****************************************************************************
//***    nsDocShellTreeOwner: Object Management
//*****************************************************************************

nsDocShellTreeOwner::nsDocShellTreeOwner() :
   mWebBrowser(nsnull), 
   mTreeOwner(nsnull),
   mPrimaryContentShell(nsnull),
   mWebBrowserChrome(nsnull),
   mOwnerWin(nsnull),
   mOwnerRequestor(nsnull),
   mChromeListener(nsnull)
{
  NS_INIT_REFCNT();
}

nsDocShellTreeOwner::~nsDocShellTreeOwner()
{
  if ( mChromeListener ) {
    mChromeListener->RemoveChromeListeners();
    NS_RELEASE(mChromeListener);
  }
}

//*****************************************************************************
// nsDocShellTreeOwner::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(nsDocShellTreeOwner)
NS_IMPL_RELEASE(nsDocShellTreeOwner)

NS_INTERFACE_MAP_BEGIN(nsDocShellTreeOwner)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocShellTreeOwner)
    NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner)
    NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
    NS_INTERFACE_MAP_ENTRY(nsICDocShellTreeOwner)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

//*****************************************************************************
// nsDocShellTreeOwner::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP nsDocShellTreeOwner::GetInterface(const nsIID& aIID, void** aSink)
{
   NS_ENSURE_ARG_POINTER(aSink);

   if(NS_SUCCEEDED(QueryInterface(aIID, aSink)))
      return NS_OK;

   if(mOwnerRequestor)
      return mOwnerRequestor->GetInterface(aIID, aSink);

   return NS_NOINTERFACE;
}

//*****************************************************************************
// nsDocShellTreeOwner::nsIDocShellTreeOwner
//*****************************************************************************   

NS_IMETHODIMP nsDocShellTreeOwner::FindItemWithName(const PRUnichar* aName,
   nsIDocShellTreeItem* aRequestor, nsIDocShellTreeItem** aFoundItem)
{
  NS_ENSURE_ARG(aName);
  NS_ENSURE_ARG_POINTER(aFoundItem);
  *aFoundItem = nsnull; // if we don't find one, we return NS_OK and a null result 
  nsresult rv;

  nsAutoString name(aName);

  if (!mWebBrowser)
    return NS_OK; // stymied

  /* special cases */
  if(name.IsEmpty())
    return NS_OK;
  if(name.EqualsIgnoreCase("_blank"))
    return NS_OK;
  if(name.EqualsIgnoreCase("_content")) {
    *aFoundItem = mWebBrowser->mDocShellAsItem;
    NS_IF_ADDREF(*aFoundItem);
    return NS_OK;
  }

  // first, is it us?
  {
    nsCOMPtr<nsIDOMWindow> domWindow;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
      nsAutoString ourName;
      domWindow->GetName(ourName);
      if (name.EqualsIgnoreCase(ourName)) {
        *aFoundItem = mWebBrowser->mDocShellAsItem;
        NS_IF_ADDREF(*aFoundItem);
        return NS_OK;
      }
    }
  }

  // next, check our children
  rv = FindChildWithName(aName, PR_TRUE, aRequestor, aFoundItem);
  if(NS_FAILED(rv) || *aFoundItem)
    return rv;

  // next, if we have a parent and it isn't the requestor, ask it
  nsCOMPtr<nsIDocShellTreeOwner> reqAsTreeOwner(do_QueryInterface(aRequestor));

  if(mTreeOwner) {
    if (mTreeOwner != reqAsTreeOwner.get())
      return mTreeOwner->FindItemWithName(aName, mWebBrowser->mDocShellAsItem.get(),
                                          aFoundItem);
    return NS_OK;
  }

  // finally, failing everything else, search all windows, if we're not already
  if (mWebBrowser->mDocShellAsItem.get() != aRequestor)
    return FindItemWithNameAcrossWindows(aName, aFoundItem);

  return NS_OK; // failed
}

nsresult nsDocShellTreeOwner::FindChildWithName(const PRUnichar *aName, 
   PRBool aRecurse, nsIDocShellTreeItem* aRequestor, 
   nsIDocShellTreeItem **aFoundItem)
{
  if (!mWebBrowser)
    return NS_OK;

  nsresult rv = NS_OK;

  nsCOMPtr<nsIDOMWindow> domWindow;
  mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
  if (!domWindow)
    return NS_OK;

  nsCOMPtr<nsIDOMWindowCollection> frames;
  domWindow->GetFrames(getter_AddRefs(frames));
  if (!frames)
    return NS_OK;

  PRUint32 ctr, count;
  frames->GetLength(&count);
  for (ctr = 0; ctr < count; ctr++) {
    nsCOMPtr<nsIDOMWindow> frame;
    frames->Item(ctr, getter_AddRefs(frame));
    if (frame) {
      nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(frame));
      if (sgo) {
        nsCOMPtr<nsIDocShell> docshell;
        sgo->GetDocShell(getter_AddRefs(docshell));
        if (docshell) {
          nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(docshell));
          if (item && item.get() != aRequestor) {
            rv = item->FindItemWithName(aName, mWebBrowser->mDocShellAsItem, aFoundItem);
            if (NS_FAILED(rv) || *aFoundItem)
              break;
          }
        }
      }
    }
  }
  return rv;
}

nsresult nsDocShellTreeOwner::FindItemWithNameAcrossWindows(
                const PRUnichar* aName,
                nsIDocShellTreeItem** aFoundItem)
{
  // search for the item across the list of top-level windows
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(sWindowWatcherContractID));
  if (!wwatch)
    return NS_OK;

  PRBool   more;
  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> windows;
  wwatch->GetWindowEnumerator(getter_AddRefs(windows));

  rv = NS_OK;
  do {
    windows->HasMoreElements(&more);
    if (!more)
      break;
    nsCOMPtr<nsISupports> nextSupWindow;
    windows->GetNext(getter_AddRefs(nextSupWindow));
    if (nextSupWindow) {
      // it's a DOM Window. cut straight to the ScriptGlobalObject.
      nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nextSupWindow));
      if (sgo) {
        nsCOMPtr<nsIDocShell> docshell;
        sgo->GetDocShell(getter_AddRefs(docshell));
        if (docshell) {
          nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(docshell));
          if (item) {
            rv = item->FindItemWithName(aName, item, aFoundItem);
            if (NS_FAILED(rv) || *aFoundItem)
              break;
          }
        }
      }
    }
  } while(1);

  return rv;
}

void nsDocShellTreeOwner::AddToWatcher() {

  if (mWebBrowser) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
      nsCOMPtr<nsPIWindowWatcher> wwatch(do_GetService(sWindowWatcherContractID));
      if (wwatch)
        wwatch->AddWindow(domWindow);
    }
  }
}

void nsDocShellTreeOwner::RemoveFromWatcher() {

  if (mWebBrowser) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
      nsCOMPtr<nsPIWindowWatcher> wwatch(do_GetService(sWindowWatcherContractID));
      if (wwatch)
        wwatch->RemoveWindow(domWindow);
    }
  }
}


NS_IMETHODIMP nsDocShellTreeOwner::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
   PRBool aPrimary, const PRUnichar* aID)
{
   if(mTreeOwner)
      return mTreeOwner->ContentShellAdded(aContentShell, aPrimary, aID);

   if (aPrimary)
      mPrimaryContentShell = aContentShell;
   return NS_OK;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetPrimaryContentShell(nsIDocShellTreeItem** aShell)
{
   NS_ENSURE_ARG_POINTER(aShell);

   if(mTreeOwner)
       return mTreeOwner->GetPrimaryContentShell(aShell);

   *aShell = (mPrimaryContentShell ? mPrimaryContentShell : mWebBrowser->mDocShellAsItem.get());
   NS_IF_ADDREF(*aShell);

   return NS_OK;
}

NS_IMETHODIMP nsDocShellTreeOwner::SizeShellTo(nsIDocShellTreeItem* aShellItem,
   PRInt32 aCX, PRInt32 aCY)
{
   NS_ENSURE_STATE(mTreeOwner || mWebBrowserChrome);

   if(mTreeOwner)
      return mTreeOwner->SizeShellTo(aShellItem, aCX, aCY);

   if(aShellItem == mWebBrowser->mDocShellAsItem.get())
      return mWebBrowserChrome->SizeBrowserTo(aCX, aCY);

   nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(aShellItem));
   NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDOMDocument> domDocument;
   webNav->GetDocument(getter_AddRefs(domDocument));
   NS_ENSURE_TRUE(domDocument, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDOMElement> domElement;
   domDocument->GetDocumentElement(getter_AddRefs(domElement));
   NS_ENSURE_TRUE(domElement, NS_ERROR_FAILURE);

   // Set the preferred Size
   //XXX
   NS_ERROR("Implement this");
   /*
   Set the preferred size on the aShellItem.
   */

   nsCOMPtr<nsIPresContext> presContext;
   mWebBrowser->mDocShell->GetPresContext(getter_AddRefs(presContext));
   NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

   nsCOMPtr<nsIPresShell> presShell;
   presContext->GetShell(getter_AddRefs(presShell));
   NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

   NS_ENSURE_SUCCESS(presShell->ResizeReflow(NS_UNCONSTRAINEDSIZE,
      NS_UNCONSTRAINEDSIZE), NS_ERROR_FAILURE);
   
   nsRect shellArea;

   presContext->GetVisibleArea(shellArea);
   float pixelScale;
   presContext->GetTwipsToPixels(&pixelScale);
   PRInt32 browserCX = PRInt32((float)shellArea.width*pixelScale);
   PRInt32 browserCY = PRInt32((float)shellArea.height*pixelScale);

   return mWebBrowserChrome->SizeBrowserTo(browserCX, browserCY);
}

NS_IMETHODIMP nsDocShellTreeOwner::GetNewWindow(PRInt32 aChromeFlags,
   nsIDocShellTreeItem** aDocShellTreeItem)
{
  nsresult rv;

  if(mTreeOwner)
    return mTreeOwner->GetNewWindow(aChromeFlags, aDocShellTreeItem);

  *aDocShellTreeItem = nsnull;

  NS_ENSURE_TRUE(mWebBrowserChrome, NS_ERROR_FAILURE);
  aChromeFlags &= ~(nsIWebBrowserChrome::CHROME_WITH_SIZE | nsIWebBrowserChrome::CHROME_WITH_POSITION);

  nsCOMPtr<nsIWebBrowser> webBrowser;
  rv = mWebBrowserChrome->CreateBrowserWindow(PRUint32(aChromeFlags),
                            -1, -1, -1, -1, // this is kind of a problem
                            getter_AddRefs(webBrowser));
  if (NS_SUCCEEDED(rv) && webBrowser) {
    nsCOMPtr<nsIInterfaceRequestor> asreq(do_QueryInterface(webBrowser));
    if (asreq) {
      nsCOMPtr<nsIDocShell> asshell(do_GetInterface(asreq));
      if (asshell)
        rv = CallQueryInterface(asshell, aDocShellTreeItem);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsDocShellTreeOwner::SetPersistence(PRBool aPersistPosition,
                                    PRBool aPersistSize,
                                    PRBool aPersistSizeMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShellTreeOwner::GetPersistence(PRBool* aPersistPosition,
                                    PRBool* aPersistSize,
                                    PRBool* aPersistSizeMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//*****************************************************************************
// nsDocShellTreeOwner::nsIBaseWindow
//*****************************************************************************   


NS_IMETHODIMP nsDocShellTreeOwner::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* aParentWidget, PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY)   
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::Create()
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::Destroy()
{
    if (mOwnerWin)
    {
        return mOwnerWin->Destroy();
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetPosition(PRInt32 aX, PRInt32 aY)
{
    if (mOwnerWin)
    {
        return mOwnerWin->SetPosition(aX, aY);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetPosition(PRInt32* aX, PRInt32* aY)
{
    if (mOwnerWin)
    {
        return mOwnerWin->GetPosition(aX, aY);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetSize(PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
    if (mOwnerWin)
    {
        return mOwnerWin->SetSize(aCX, aCY, aRepaint);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetSize(PRInt32* aCX, PRInt32* aCY)
{
    if (mOwnerWin)
    {
        return mOwnerWin->GetSize(aCX, aCY);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetPositionAndSize(PRInt32 aX, PRInt32 aY,
   PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
    if (mOwnerWin)
    {
        return mOwnerWin->SetPositionAndSize(aX, aY, aCX, aCY, aRepaint);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetPositionAndSize(PRInt32* aX, PRInt32* aY,
   PRInt32* aCX, PRInt32* aCY)
{
    if (mOwnerWin)
    {
        return mOwnerWin->GetPositionAndSize(aX, aY, aCX, aCY);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::Repaint(PRBool aForce)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetParentWidget(nsIWidget** aParentWidget)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetParentWidget(nsIWidget* aParentWidget)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
    if (mOwnerWin)
    {
        return mOwnerWin->GetSiteWindow(aParentNativeWindow);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetVisibility(PRBool* aVisibility)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetVisibility(PRBool aVisibility)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetMainWidget(nsIWidget** aMainWidget)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetFocus()
{
    if (mOwnerWin)
    {
        return mOwnerWin->SetFocus();
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::FocusAvailable(nsIBaseWindow* aCurrentFocus, 
   PRBool* aTookFocus)
{
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::GetTitle(PRUnichar** aTitle)
{
    if (mOwnerWin)
    {
        return mOwnerWin->GetTitle(aTitle);
    }
    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetTitle(const PRUnichar* aTitle)
{
    if (mOwnerWin)
    {
        return mOwnerWin->SetTitle(aTitle);
    }
    return NS_ERROR_NULL_POINTER;
}


//*****************************************************************************
// nsDocShellTreeOwner::nsIWebProgressListener
//*****************************************************************************   


NS_IMETHODIMP
nsDocShellTreeOwner::OnProgressChange(nsIWebProgress* aProgress,
                                      nsIRequest* aRequest,
                                      PRInt32 aCurSelfProgress,
                                      PRInt32 aMaxSelfProgress, 
                                      PRInt32 aCurTotalProgress,
                                      PRInt32 aMaxTotalProgress)
{
    // In the absence of DOM document creation event, this method is the
    // most convenient place to install the mouse listener on the
    // DOM document.
    return AddChromeListeners();
}

NS_IMETHODIMP
nsDocShellTreeOwner::OnStateChange(nsIWebProgress* aProgress,
                                   nsIRequest* aRequest,
                                   PRInt32 aProgressStateFlags,
                                   nsresult aStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsDocShellTreeOwner::OnLocationChange(nsIWebProgress* aWebProgress,
                                                    nsIRequest* aRequest,
                                                    nsIURI* aURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsDocShellTreeOwner::OnStatusChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsresult aStatus,
                                    const PRUnichar* aMessage)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsDocShellTreeOwner::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                      nsIRequest *aRequest, 
                                      PRInt32 state)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


//*****************************************************************************
// nsDocShellTreeOwner: Helpers
//*****************************************************************************   

//*****************************************************************************
// nsDocShellTreeOwner: Accessors
//*****************************************************************************   

void nsDocShellTreeOwner::WebBrowser(nsWebBrowser* aWebBrowser)
{
  if ( !aWebBrowser ) {
    if ( mChromeListener ) {
      mChromeListener->RemoveChromeListeners();
      NS_RELEASE(mChromeListener);
    }
  }
  mWebBrowser = aWebBrowser;
}

nsWebBrowser* nsDocShellTreeOwner::WebBrowser()
{
   return mWebBrowser;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetTreeOwner(nsIDocShellTreeOwner* aTreeOwner)
{ 
   if(aTreeOwner) {
      nsCOMPtr<nsIWebBrowserChrome> webBrowserChrome(do_GetInterface(aTreeOwner));
      NS_ENSURE_TRUE(webBrowserChrome, NS_ERROR_INVALID_ARG);
      NS_ENSURE_SUCCESS(SetWebBrowserChrome(webBrowserChrome), NS_ERROR_INVALID_ARG);
      mTreeOwner = aTreeOwner;
   }
   else if(mWebBrowserChrome)
      mTreeOwner = nsnull;
   else {
      mTreeOwner = nsnull;
      NS_ENSURE_SUCCESS(SetWebBrowserChrome(nsnull), NS_ERROR_FAILURE);
   }

   return NS_OK;
}

NS_IMETHODIMP nsDocShellTreeOwner::SetWebBrowserChrome(nsIWebBrowserChrome* aWebBrowserChrome)
{
   if(!aWebBrowserChrome) {
      mWebBrowserChrome = nsnull;
      mOwnerWin = nsnull;
      mOwnerRequestor = nsnull;
      }
   else
      {
      nsCOMPtr<nsIWebBrowserSiteWindow> ownerWin(do_QueryInterface(aWebBrowserChrome));
      nsCOMPtr<nsIInterfaceRequestor> requestor(do_QueryInterface(aWebBrowserChrome));

      // it's ok for ownerWin or requestor to be null.
      mWebBrowserChrome = aWebBrowserChrome;
      mOwnerWin = ownerWin;
      mOwnerRequestor = requestor;
   }
   return NS_OK;
}


//
// AddChromeListeners
//
// Hook up things to the chrome like context menus and tooltips, if the chrome
// has implemented the right interfaces.
//
NS_IMETHODIMP
nsDocShellTreeOwner :: AddChromeListeners ( )
{
  nsresult rv = NS_OK;
  
  if ( !mChromeListener ) {
    nsCOMPtr<nsIContextMenuListener> contextListener ( do_QueryInterface(mWebBrowserChrome) );
    nsCOMPtr<nsITooltipListener> tooltipListener ( do_QueryInterface(mWebBrowserChrome) );
    if ( contextListener || tooltipListener ) {
      mChromeListener = new ChromeListener ( mWebBrowser, mWebBrowserChrome );
      if ( mChromeListener ) {
        NS_ADDREF(mChromeListener);
        rv = mChromeListener->AddChromeListeners();
      }
      else
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }
   
  return rv;
  
} // AddChromeListeners


#ifdef XP_MAC
#pragma mark -
#endif


NS_IMPL_ADDREF(ChromeListener)
NS_IMPL_RELEASE(ChromeListener)

NS_INTERFACE_MAP_BEGIN(ChromeListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
    NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
NS_INTERFACE_MAP_END


//
// ChromeListener ctor
//
ChromeListener :: ChromeListener ( nsWebBrowser* inBrowser, nsIWebBrowserChrome* inChrome ) 
  : mWebBrowser(inBrowser), mWebBrowserChrome(inChrome),
     mContextMenuListenerInstalled(PR_FALSE), mTooltipListenerInstalled(PR_FALSE),
     mShowingTooltip(PR_FALSE), mMouseClientX(0), mMouseClientY(0)
{
    NS_INIT_REFCNT();
} // ctor


//
// ChromeListener dtor
//
ChromeListener :: ~ChromeListener ( )
{
  
} // dtor


//
// AddChromeListeners
//
// Hook up things to the chrome like context menus and tooltips, if the chrome
// has implemented the right interfaces.
//
NS_IMETHODIMP
ChromeListener :: AddChromeListeners ( )
{  
  if ( !mEventReceiver ) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);

    nsCOMPtr<nsPIDOMWindow> domWindowPrivate = do_QueryInterface(domWindow);
    NS_ENSURE_TRUE(domWindowPrivate, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMWindowInternal> rootWindow;
    domWindowPrivate->GetPrivateRoot(getter_AddRefs(rootWindow));
    NS_ENSURE_TRUE(rootWindow, NS_ERROR_FAILURE);
    nsCOMPtr<nsIChromeEventHandler> chromeHandler;
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
    piWin->GetChromeEventHandler(getter_AddRefs(chromeHandler));
    NS_ENSURE_TRUE(chromeHandler, NS_ERROR_FAILURE);

    mEventReceiver = do_QueryInterface(chromeHandler);
  }
  
  // Register the appropriate events for context menus, but only if
  // the embedding chrome cares.
  nsresult rv = NS_OK;
  nsCOMPtr<nsIContextMenuListener> contextListener ( do_QueryInterface(mWebBrowserChrome) );
  if ( contextListener && !mContextMenuListenerInstalled ) {
    rv = AddContextMenuListener();
    if ( NS_FAILED(rv) )
      return rv;
  }
    
  // Register the appropriate events for tooltips, but only if
  // the embedding chrome cares.
  nsCOMPtr<nsITooltipListener> tooltipListener ( do_QueryInterface(mWebBrowserChrome) );
  if ( tooltipListener && !mTooltipListenerInstalled ) {
    rv = AddTooltipListener();
    if ( NS_FAILED(rv) )
      return rv;
  }
  
  return rv;
  
} // AddChromeListeners


//
// AddContextMenuListener
//
// Subscribe to the events that will allow us to track context menus. Bascially, this
// is just the mouseDown event.
//
NS_IMETHODIMP
ChromeListener :: AddContextMenuListener()
{
  if (mEventReceiver) {
    nsIDOMMouseListener *pListener = NS_STATIC_CAST(nsIDOMMouseListener *, this);
    nsresult rv = mEventReceiver->AddEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseListener));
    if (NS_SUCCEEDED(rv))
      mContextMenuListenerInstalled = PR_TRUE;
  }

  return NS_OK;
}


//
// AddTooltipListener
//
// Subscribe to the events that will allow us to track tooltips. We need "mouse" for mouseExit,
// "mouse motion" for mouseMove, and "key" for keyDown. As we add the listeners, keep track
// of how many succeed so we can clean up correctly in Release().
//
NS_IMETHODIMP
ChromeListener :: AddTooltipListener()
{
  if (mEventReceiver) {
    nsIDOMMouseListener *pListener = NS_STATIC_CAST(nsIDOMMouseListener *, this);
    nsresult rv = mEventReceiver->AddEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseListener));
    nsresult rv2 = mEventReceiver->AddEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseMotionListener));
    nsresult rv3 = mEventReceiver->AddEventListenerByIID(pListener, NS_GET_IID(nsIDOMKeyListener));
    
    // if all 3 succeed, we're a go!
    if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv2) && NS_SUCCEEDED(rv3)) 
      mTooltipListenerInstalled = PR_TRUE;
  }

  return NS_OK;
}


//
// RemoveChromeListeners
//
// Unsubscribe from the various things we've hooked up to the window root, including
// context menus and tooltips for starters.
//
NS_IMETHODIMP
ChromeListener :: RemoveChromeListeners ( )
{
  HideTooltip();

  if ( mContextMenuListenerInstalled )
    RemoveContextMenuListener();
    
  if ( mTooltipListenerInstalled )
    RemoveTooltipListener();
  
  mEventReceiver = nsnull;
  
  // it really doesn't matter if these fail...
  return NS_OK;
  
} // RemoveChromeListeners


//
// RemoveContextMenuListener
//
// Unsubscribe from all the various context menu events that we were listening to. 
// Bascially, this is just the mouseDown event.
//
NS_IMETHODIMP 
ChromeListener :: RemoveContextMenuListener()
{
  if (mEventReceiver) {
    nsIDOMMouseListener *pListener = NS_STATIC_CAST(nsIDOMMouseListener *, this);
    nsresult rv = mEventReceiver->RemoveEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseListener));
    if (NS_SUCCEEDED(rv))
      mContextMenuListenerInstalled = PR_FALSE;
  }

  return NS_OK;
}


//
// RemoveTooltipListener
//
// Unsubscribe from all the various tooltip events that we were listening to
//
NS_IMETHODIMP 
ChromeListener :: RemoveTooltipListener()
{
  if (mEventReceiver) {
    nsIDOMMouseListener *pListener = NS_STATIC_CAST(nsIDOMMouseListener *, this);
    nsresult rv = mEventReceiver->RemoveEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseListener));
    nsresult rv2 = mEventReceiver->RemoveEventListenerByIID(pListener, NS_GET_IID(nsIDOMMouseMotionListener));
    nsresult rv3 = mEventReceiver->RemoveEventListenerByIID(pListener, NS_GET_IID(nsIDOMKeyListener));
    if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv2) && NS_SUCCEEDED(rv3))
      mTooltipListenerInstalled = PR_FALSE;
  }

  return NS_OK;
}


//
// KeyDown
//
// When the user starts typing, they generaly don't want to see any messy wax
// builup. Hide the tooltip.
//
nsresult
ChromeListener::KeyDown(nsIDOMEvent* aMouseEvent)
{
  // make sure we're a tooltip. if not, bail. Just to be safe.
  if ( ! mTooltipListenerInstalled )
    return NS_OK;
  
  return HideTooltip();
    
} // KeyDown


//
// KeyUp
// KeyPress
//
// We can ignore these as they are already handled by KeyDown
//
nsresult
ChromeListener::KeyUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
    
} // KeyUp

nsresult
ChromeListener::KeyPress(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
    
} // KeyPress


//
// MouseDown
//
// Do the processing for context menus, if this is a right-mouse click
//
nsresult 
ChromeListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
    nsCOMPtr<nsIDOMMouseEvent> mouseEvent (do_QueryInterface(aMouseEvent));
    if (!mouseEvent)
        return NS_OK;
 
    // if the mouse goes down for any reason when a tooltip is showing,
    // we want to hide it, but continue processing the event.
    if ( mShowingTooltip ) 
      HideTooltip();
     
    // Test for right mouse button click
    PRUint16 buttonNumber;
    nsresult res = mouseEvent->GetButton(&buttonNumber);
    if (NS_FAILED(res))
        return res;
    if (buttonNumber != 2) // 2 is the magic number
        return NS_OK;

    nsCOMPtr<nsIDOMEventTarget> targetNode;
    res = aMouseEvent->GetTarget(getter_AddRefs(targetNode));
    if (NS_FAILED(res))
        return res;
    if (!targetNode)
        return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIDOMNode> targetDOMnode;
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(targetNode);
    if (!node)
        return NS_OK;

    // Find the first node to be an element starting with this node and
    // working up through its parents.

    PRUint32 flags = nsIContextMenuListener::CONTEXT_NONE;
    nsCOMPtr<nsIDOMHTMLElement> element;
    do {
        //PRUint16 type;
        //node->GetNodeType(&type);

        // XXX test for selected text
        element = do_QueryInterface(node);
        if (element)
        {
            nsAutoString tag;
            element->GetTagName(tag);

            // Test what kind of element we're dealing with here
            if (tag.EqualsWithConversion("img", PR_TRUE))
            {
                flags |= nsIContextMenuListener::CONTEXT_IMAGE;
                targetDOMnode = node;
                // if we see an image, keep searching for a possible anchor
            }
            else if (tag.EqualsWithConversion("input", PR_TRUE))
            {
                // INPUT element - button, combo, checkbox, text etc.
                flags |= nsIContextMenuListener::CONTEXT_INPUT;
                targetDOMnode = node;
                break; // exit do-while
            }
            else if (tag.EqualsWithConversion("textarea", PR_TRUE))
            {
                // text area
                flags |= nsIContextMenuListener::CONTEXT_TEXT;
                targetDOMnode = node;
                break; // exit do-while
            }
            else if (tag.EqualsWithConversion("html", PR_TRUE))
            {
                // only care about this if no other context was found.
                if (!flags) {
                    flags |= nsIContextMenuListener::CONTEXT_DOCUMENT;
                    targetDOMnode = node;
                }
                break; // exit do-while
            }

            // Test if the element has an associated link
            nsCOMPtr<nsIDOMNamedNodeMap> attributes;
            node->GetAttributes(getter_AddRefs(attributes));
            if (attributes)
            {
                nsCOMPtr<nsIDOMNode> hrefNode;
                nsAutoString href; href.AssignWithConversion("href");
                attributes->GetNamedItem(href, getter_AddRefs(hrefNode));
                if (hrefNode)
                {
                    flags |= nsIContextMenuListener::CONTEXT_LINK;
                    if (!targetDOMnode)
                        targetDOMnode = node;
                    break; // exit do-while
                }
            }
        }

        // walk-up-the-tree
        nsCOMPtr<nsIDOMNode> parentNode;
        node->GetParentNode(getter_AddRefs(parentNode));
        node = parentNode;
    } while (node);

    // Tell the listener all about the event
    nsCOMPtr<nsIContextMenuListener> menuListener(do_QueryInterface(mWebBrowserChrome));
    if ( menuListener )
      menuListener->OnShowContextMenu(flags, aMouseEvent, targetDOMnode);

    return NS_OK;

} // MouseDown


nsresult 
ChromeListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
    return NS_OK; 
}

nsresult 
ChromeListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
    return NS_OK; 
}

nsresult 
ChromeListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
    return NS_OK; 
}

nsresult 
ChromeListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
    return NS_OK; 
}


//
// MouseOut
//
// If we're responding to tooltips, hide the tip whenever the mouse leaves
// the area it was in.
nsresult 
ChromeListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  // make sure we're a tooltip. if not, bail. Just to be safe.
  if ( ! mTooltipListenerInstalled )
    return NS_OK;
  
  return HideTooltip();
}


//
// MouseMove
//
// If we're a tooltip, fire off a timer to see if a tooltip should be shown. If the
// timer fires, we cache the node in |mPossibleTooltipNode|.
//
nsresult
ChromeListener::MouseMove(nsIDOMEvent* aMouseEvent)
{
  // make sure we're a tooltip. if not, bail. Just to be safe.
  if ( ! mTooltipListenerInstalled )
    return NS_OK;
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent ( do_QueryInterface(aMouseEvent) );
  if (!mouseEvent)
    return NS_OK;

  // stash the coordinates of the event so that we can still get back to it from within the 
  // timer callback. On win32, we'll get a MouseMove event even when a popup goes away --
  // even when the mouse doesn't change position! To get around this, we make sure the
  // mouse has really moved before proceeding.
  PRInt32 newMouseX, newMouseY;
  mouseEvent->GetClientX(&newMouseX);
  mouseEvent->GetClientY(&newMouseY);
  if ( mMouseClientX == newMouseX && mMouseClientY == newMouseY )
    return NS_OK;
  mMouseClientX = newMouseX; mMouseClientY = newMouseY;

  // We want to close the tip if it is being displayed and the mouse moves. Recall 
  // that |mShowingTooltip| is set when the popup is showing. Furthermore, as the mouse
  // moves, we want to make sure we reset the timer to show it, so that the delay
  // is from when the mouse stops moving, not when it enters the element.
  if ( mShowingTooltip )
    return HideTooltip();
  if ( mTooltipTimer )
    mTooltipTimer->Cancel();

  mTooltipTimer = do_CreateInstance("@mozilla.org/timer;1");
  if ( mTooltipTimer ) {
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aMouseEvent->GetTarget(getter_AddRefs(eventTarget));
    if ( eventTarget )
      mPossibleTooltipNode = do_QueryInterface(eventTarget);
    if ( mPossibleTooltipNode ) {
      nsresult rv = mTooltipTimer->Init(sTooltipCallback, this, kTooltipShowTime, NS_PRIORITY_HIGH);
      if (NS_FAILED(rv))
        mPossibleTooltipNode = nsnull;
    }
  }
  else
    NS_WARNING ( "Could not create a timer for tooltip tracking" );
    
  return NS_OK;
  
} // MouseMove


//
// ShowTooltip
//
// Tell the registered chrome that they should show the tooltip
//
NS_IMETHODIMP
ChromeListener :: ShowTooltip ( PRInt32 inXCoords, PRInt32 inYCoords, const nsAReadableString & inTipText )
{
  nsresult rv = NS_OK;
  
  // do the work to call the client
  nsCOMPtr<nsITooltipListener> tooltipListener ( do_QueryInterface(mWebBrowserChrome) );
  if ( tooltipListener ) {
    rv = tooltipListener->OnShowTooltip ( inXCoords, inYCoords, nsPromiseFlatString(inTipText).get() ); 
    if ( NS_SUCCEEDED(rv) )
      mShowingTooltip = PR_TRUE;
  }

  return rv;
  
} // ShowTooltip


//
// HideTooltip
//
// Tell the registered chrome that they should rollup the tooltip
// NOTE: This routine is safe to call even if the popup is already closed.
//
NS_IMETHODIMP
ChromeListener :: HideTooltip ( )
{
  nsresult rv = NS_OK;
  
  // shut down the relevant timers
  if ( mTooltipTimer ) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    // release tooltip target
    mPossibleTooltipNode = nsnull;
  }
  if ( mAutoHideTimer ) {
    mAutoHideTimer->Cancel();
    mAutoHideTimer = nsnull;
  }

  // if we're showing the tip, tell the chrome to hide it
  if ( mShowingTooltip ) {
    nsCOMPtr<nsITooltipListener> tooltipListener ( do_QueryInterface(mWebBrowserChrome) );
    if ( tooltipListener ) {
      rv = tooltipListener->OnHideTooltip ( );
      if ( NS_SUCCEEDED(rv) )
        mShowingTooltip = PR_FALSE;
    }
  }

  return rv;
  
} // HideTooltip


//
// FindTitleText
//
// Determine if there is a TITLE attribute. Checks both the XLINK namespace, and no
// namespace. Returns |PR_TRUE| if there is, and sets the text in |outText|.
//
PRBool
ChromeListener :: FindTitleText ( nsIDOMNode* inNode, nsAWritableString & outText )
{
  PRBool found = PR_FALSE;

  nsCOMPtr<nsIDOMNode> current ( inNode );
  while ( !found && current ) {
    nsCOMPtr<nsIDOMElement> currElement ( do_QueryInterface(current) );
    if ( currElement ) {
      // first try the normal title attribute...
      currElement->GetAttribute(NS_LITERAL_STRING("title"), outText);
      if ( outText.Length() )
        found = PR_TRUE;
      else {
        // ...ok, that didn't work, try it in the XLink namespace
        currElement->GetAttributeNS(NS_LITERAL_STRING("http://www.w3.org/1999/xlink"), NS_LITERAL_STRING("title"), outText);
        if ( outText.Length() )
          found = PR_TRUE;
      }
    }
    
    // not found here, walk up to the parent and keep trying
    if ( !found ) {
      nsCOMPtr<nsIDOMNode> temp ( current );
      temp->GetParentNode(getter_AddRefs(current));
    }
  } // while not found

  return found;
  
} // FindTitleText


//
// sTooltipCallback
//
// A timer callback, fired when the mouse has hovered inside of a frame for the 
// appropriate amount of time. Getting to this point means that we should show the
// tooltip, but only after we determine there is an appropriate TITLE element.
//
// This relies on certain things being cached into the |aChromeListener| object passed to
// us by the timer:
//   -- the x/y coordinates of the mouse      (mMouseClientY, mMouseClientX)
//   -- the dom node the user hovered over    (mPossibleTooltipNode)
//
void
ChromeListener :: sTooltipCallback (nsITimer *aTimer, void *aChromeListener)
{
  ChromeListener* self = NS_STATIC_CAST(ChromeListener*, aChromeListener);
  if ( self && self->mPossibleTooltipNode ) {
    // if there is a TITLE tag, show the tip and fire off a timer to auto-hide it
    nsAutoString tooltipText;
    if ( self->FindTitleText(self->mPossibleTooltipNode, tooltipText) ) {
      self->CreateAutoHideTimer ( );  
      self->ShowTooltip ( self->mMouseClientX, self->mMouseClientY, tooltipText );
    }
    
    // release tooltip target if there is one, NO MATTER WHAT
    self->mPossibleTooltipNode = nsnull;
  } // if "self" data valid
  
} // sTooltipCallback


//
// CreateAutoHideTimer
//
// Create a new timer to see if we should auto-hide. It's ok if this fails.
//
void
ChromeListener :: CreateAutoHideTimer ( )
{
  // just to be anal (er, safe)
  if ( mAutoHideTimer ) {
    mAutoHideTimer->Cancel();
    mAutoHideTimer = nsnull;
  }
  
  mAutoHideTimer = do_CreateInstance("@mozilla.org/timer;1");
  if ( mAutoHideTimer )
    mAutoHideTimer->Init(sAutoHideCallback, this, kTooltipAutoHideTime, NS_PRIORITY_HIGH);

} // CreateAutoHideTimer


//
// sAutoHideCallback
//
// This fires after a tooltip has been open for a certain length of time. Just tell
// the listener to close the popup. We don't have to worry, because HideTooltip() can
// be called multiple times, even if the tip has already been closed.
//
void
ChromeListener :: sAutoHideCallback ( nsITimer *aTimer, void* aListener )
{
  ChromeListener* self = NS_STATIC_CAST(ChromeListener*, aListener);
  if ( self )
    self->HideTooltip();

  // NOTE: |aTimer| and |self->mAutoHideTimer| are invalid after calling ClosePopup();
  
} // sAutoHideCallback


