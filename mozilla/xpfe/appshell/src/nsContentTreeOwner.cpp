/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

// Local Includes
#include "nsContentTreeOwner.h"
#include "nsXULWindow.h"

// Helper Classes
#include "nsIGenericFactory.h"

// Interfaces needed to be included
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULElement.h"
#include "nsIPrompt.h"
#include "nsIWindowMediator.h"
#include "nsIXULBrowserWindow.h"
#include "nsPIDOMWindow.h"

// CIDs
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);

//*****************************************************************************
//***    nsContentTreeOwner: Object Management
//*****************************************************************************

nsContentTreeOwner::nsContentTreeOwner(PRBool fPrimary) : mXULWindow(nsnull), 
   mPrimary(fPrimary), mContentTitleSetting(PR_FALSE), 
   mChromeFlags(nsIWebBrowserChrome::CHROME_ALL)
{
	NS_INIT_REFCNT();
}

nsContentTreeOwner::~nsContentTreeOwner()
{
}

//*****************************************************************************
// nsContentTreeOwner::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(nsContentTreeOwner)
NS_IMPL_RELEASE(nsContentTreeOwner)

NS_INTERFACE_MAP_BEGIN(nsContentTreeOwner)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
NS_INTERFACE_MAP_END

//*****************************************************************************
// nsContentTreeOwner::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::GetInterface(const nsIID& aIID, void** aSink)
{
   NS_ENSURE_ARG_POINTER(aSink);

   if(aIID.Equals(NS_GET_IID(nsIWebBrowserChrome)))
      *aSink = NS_STATIC_CAST(nsIWebBrowserChrome*, this);
   else if(aIID.Equals(NS_GET_IID(nsIPrompt)))
      return mXULWindow->GetInterface(aIID, aSink);
   else
      return QueryInterface(aIID, aSink);

   NS_IF_ADDREF(((nsISupports*)*aSink));
   return NS_OK;   
}

//*****************************************************************************
// nsContentTreeOwner::nsIDocShellTreeOwner
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::FindItemWithName(const PRUnichar* aName,
   nsIDocShellTreeItem* aRequestor, nsIDocShellTreeItem** aFoundItem)
{
   NS_ENSURE_ARG_POINTER(aFoundItem);

   *aFoundItem = nsnull;

   nsAutoString   name(aName);

   PRBool fIs_Content = PR_FALSE;

   /* Special Cases */
   if(name.Length() == 0)
      return NS_OK;
   if(name.EqualsIgnoreCase("_blank"))
      return NS_OK;
   if(name.EqualsIgnoreCase("_content"))
      {
      fIs_Content = PR_TRUE;
      mXULWindow->GetPrimaryContentShell(aFoundItem);
      if(*aFoundItem)
         return NS_OK;
      }

   nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
   NS_ENSURE_TRUE(windowMediator, NS_ERROR_FAILURE);

   nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
   NS_ENSURE_SUCCESS(windowMediator->GetXULWindowEnumerator(nsnull, 
      getter_AddRefs(windowEnumerator)), NS_ERROR_FAILURE);
   
   PRBool more;
   
   windowEnumerator->HasMoreElements(&more);
   while(more)
      {
      nsCOMPtr<nsISupports> nextWindow = nsnull;
      windowEnumerator->GetNext(getter_AddRefs(nextWindow));
      nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
      NS_ENSURE_TRUE(xulWindow, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDocShellTreeItem> shellAsTreeItem;
      xulWindow->GetPrimaryContentShell(getter_AddRefs(shellAsTreeItem));

      if(shellAsTreeItem)
         {
         if(fIs_Content)
            *aFoundItem = shellAsTreeItem;
         else if(aRequestor != shellAsTreeItem.get())
            {
            // Do this so we can pass in the tree owner as the requestor so the child knows not
            // to call back up.
            nsCOMPtr<nsIDocShellTreeOwner> shellOwner;
            shellAsTreeItem->GetTreeOwner(getter_AddRefs(shellOwner));
            nsCOMPtr<nsISupports> shellOwnerSupports(do_QueryInterface(shellOwner));

            shellAsTreeItem->FindItemWithName(aName, shellOwnerSupports, aFoundItem);
            }
         if(*aFoundItem)
            return NS_OK;   
         }
      windowEnumerator->HasMoreElements(&more);
      }
   return NS_OK;      
}

NS_IMETHODIMP nsContentTreeOwner::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
   PRBool aPrimary, const PRUnichar* aID)
{
   mXULWindow->ContentShellAdded(aContentShell, aPrimary, aID);
   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::GetPrimaryContentShell(nsIDocShellTreeItem** aShell)
{
   return mXULWindow->GetPrimaryContentShell(aShell);
}

NS_IMETHODIMP nsContentTreeOwner::SizeShellTo(nsIDocShellTreeItem* aShellItem,
   PRInt32 aCX, PRInt32 aCY)
{
   return mXULWindow->SizeShellTo(aShellItem, aCX, aCY);
}

NS_IMETHODIMP nsContentTreeOwner::ShowModal()
{
   return mXULWindow->ShowModal();   
}

NS_IMETHODIMP nsContentTreeOwner::ExitModalLoop(nsresult aStatus)
{
   return mXULWindow->ExitModalLoop(aStatus);   
}

NS_IMETHODIMP nsContentTreeOwner::GetNewWindow(PRInt32 aChromeFlags,
   nsIDocShellTreeItem** aDocShellTreeItem)
{
   return mXULWindow->GetNewWindow(aChromeFlags, aDocShellTreeItem);
}

//*****************************************************************************
// nsContentTreeOwner::nsIWebBrowserChrome
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::SetStatus(PRUint32 aStatusType, const PRUnichar* aStatus)
{
   nsCOMPtr<nsIDOMWindowInternal> domWindow;
   mXULWindow->GetWindowDOMWindow(getter_AddRefs(domWindow));
   nsCOMPtr<nsPIDOMWindow> piDOMWindow(do_QueryInterface(domWindow));
   if(!piDOMWindow)
      return NS_OK;

   nsCOMPtr<nsISupports> xpConnectObj;
   nsAutoString xulBrowserWinId; xulBrowserWinId.AssignWithConversion("XULBrowserWindow");
   piDOMWindow->GetObjectProperty(xulBrowserWinId.GetUnicode(), getter_AddRefs(xpConnectObj));
   nsCOMPtr<nsIXULBrowserWindow> xulBrowserWindow(do_QueryInterface(xpConnectObj));

   if (xulBrowserWindow)
   {
     switch(aStatusType)
     {
     case STATUS_SCRIPT:
       xulBrowserWindow->SetJSStatus(aStatus);
       break;
     case STATUS_SCRIPT_DEFAULT:
       xulBrowserWindow->SetJSDefaultStatus(aStatus);
       break;
     case STATUS_LINK:
       xulBrowserWindow->SetOverLink(aStatus);
       break;
     }
   }

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SetChromeFlags(PRUint32 aChromeFlags)
{
   mChromeFlags = aChromeFlags;
   NS_ENSURE_SUCCESS(ApplyChromeFlags(), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::GetChromeFlags(PRUint32* aChromeFlags)
{
   NS_ENSURE_ARG_POINTER(aChromeFlags);

   *aChromeFlags = mChromeFlags;
   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::CreateBrowserWindow(PRUint32 aChromeFlags,
   nsIWebBrowser** aWebBrowser)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::FindNamedBrowserItem(const PRUnichar* aName,
   nsIDocShellTreeItem** aBrowserItem)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::ShowAsModal()
{
   return ShowModal();
}

NS_IMETHODIMP nsContentTreeOwner::ExitModalEventLoop(nsresult aStatus)
{
   return ExitModalLoop(aStatus);   
}

NS_IMETHODIMP
nsContentTreeOwner::SetPersistence(PRBool aPersistX, PRBool aPersistY,
                                   PRBool aPersistCX, PRBool aPersistCY,
                                   PRBool aPersistSizeMode)
{
   nsCOMPtr<nsIDOMElement> docShellElement;
   mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
   if(!docShellElement)
      return NS_ERROR_FAILURE;

   nsAutoString persistString;
   docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("persist"), persistString);

   PRBool saveString = PR_FALSE;
   PRInt32 index;

   // Set X
   index = persistString.Find("screenX");
   if(!aPersistX && (index >= 0)) {
      persistString.Cut(index, 7);
      saveString = PR_TRUE;
   } else if(aPersistX && (index < 0 )) {
     persistString.AppendWithConversion(" screenX");
     saveString = PR_TRUE;
   }
   // Set Y
   index = persistString.Find("screenY");
   if(!aPersistY && (index >= 0)) {
     persistString.Cut(index, 7);
     saveString = PR_TRUE;
   } else if(aPersistY && (index < 0 )) {
     persistString.AppendWithConversion(" screenY");
     saveString = PR_TRUE;
   }
   // Set CX
   index = persistString.Find("width");
   if(!aPersistCX && (index >= 0)) {
     persistString.Cut(index, 5);
     saveString = PR_TRUE;
   } else if(aPersistCX && (index < 0 )) {
     persistString.AppendWithConversion(" width");
     saveString = PR_TRUE;
   }
   // Set CY
   index = persistString.Find("height");
   if(!aPersistCY && (index >= 0)) {
     persistString.Cut(index, 6);
     saveString = PR_TRUE;
   } else if(aPersistCY && (index < 0 )) {
     persistString.AppendWithConversion(" height");
     saveString = PR_TRUE;
   }
   // Set SizeMode
   index = persistString.Find("sizemode");
   if (!aPersistSizeMode && (index >= 0)) {
     persistString.Cut(index, 8);
     saveString = PR_TRUE;
   } else if (aPersistSizeMode && (index < 0)) {
     persistString.AppendWithConversion(" sizemode");
     saveString = PR_TRUE;
   }

   if(saveString) 
      docShellElement->SetAttribute(NS_ConvertASCIItoUCS2("persist"), persistString);

   return NS_OK;
}

NS_IMETHODIMP
nsContentTreeOwner::GetPersistence(PRBool* aPersistX, PRBool* aPersistY,
                                   PRBool* aPersistCX, PRBool* aPersistCY,
                                   PRBool* aPersistSizeMode)
{
   nsCOMPtr<nsIDOMElement> docShellElement;
   mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
   if(!docShellElement) 
      return NS_ERROR_FAILURE;

   nsAutoString persistString;
   docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("persist"), persistString);

   if(aPersistX)
      *aPersistX = persistString.Find("screenX") >= 0 ? PR_TRUE : PR_FALSE;
   if(aPersistY)
      *aPersistY = persistString.Find("screenY") >= 0 ? PR_TRUE : PR_FALSE;
   if(aPersistCX)
      *aPersistCX = persistString.Find("width") >= 0 ? PR_TRUE : PR_FALSE;
   if(aPersistCY)
      *aPersistCY = persistString.Find("height") >= 0 ? PR_TRUE : PR_FALSE;
   if(aPersistSizeMode)
      *aPersistSizeMode = persistString.Find("sizemode") >= 0 ? PR_TRUE : PR_FALSE;

   return NS_OK;
}

//*****************************************************************************
// nsContentTreeOwner::nsIBaseWindow
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* parentWidget, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)   
{
   // Ignore wigdet parents for now.  Don't think those are a vaild thing to call.
   NS_ENSURE_SUCCESS(SetPositionAndSize(x, y, cx, cy, PR_FALSE), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::Create()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsContentTreeOwner::Destroy()
{
   return mXULWindow->Destroy();
}

NS_IMETHODIMP nsContentTreeOwner::SetPosition(PRInt32 aX, PRInt32 aY)
{
   return mXULWindow->SetPosition(aX, aY);
}

NS_IMETHODIMP nsContentTreeOwner::GetPosition(PRInt32* aX, PRInt32* aY)
{
   return mXULWindow->GetPosition(aX, aY);
}

NS_IMETHODIMP nsContentTreeOwner::SetSize(PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
   return mXULWindow->SetSize(aCX, aCY, aRepaint);
}

NS_IMETHODIMP nsContentTreeOwner::GetSize(PRInt32* aCX, PRInt32* aCY)
{
   return mXULWindow->GetSize(aCX, aCY);
}

NS_IMETHODIMP nsContentTreeOwner::SetPositionAndSize(PRInt32 aX, PRInt32 aY,
   PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
   return mXULWindow->SetPositionAndSize(aX, aY, aCX, aCY, aRepaint);
}

NS_IMETHODIMP nsContentTreeOwner::GetPositionAndSize(PRInt32* aX, PRInt32* aY,
   PRInt32* aCX, PRInt32* aCY)
{
   return mXULWindow->GetPositionAndSize(aX, aY, aCX, aCY); 
}

NS_IMETHODIMP nsContentTreeOwner::Repaint(PRBool aForce)
{
   return mXULWindow->Repaint(aForce);
}

NS_IMETHODIMP nsContentTreeOwner::GetParentWidget(nsIWidget** aParentWidget)
{
   return mXULWindow->GetParentWidget(aParentWidget);
}

NS_IMETHODIMP nsContentTreeOwner::SetParentWidget(nsIWidget* aParentWidget)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   return mXULWindow->GetParentNativeWindow(aParentNativeWindow);
}

NS_IMETHODIMP nsContentTreeOwner::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetVisibility(PRBool* aVisibility)
{
   return mXULWindow->GetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::SetVisibility(PRBool aVisibility)
{
   return mXULWindow->SetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::GetMainWidget(nsIWidget** aMainWidget)
{
   NS_ENSURE_ARG_POINTER(aMainWidget);

   *aMainWidget = mXULWindow->mWindow;
   NS_IF_ADDREF(*aMainWidget);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::SetFocus()
{
   return mXULWindow->SetFocus();
}

NS_IMETHODIMP nsContentTreeOwner::FocusAvailable(nsIBaseWindow* aCurrentFocus, 
   PRBool* aTookFocus)
{
   return mXULWindow->FocusAvailable(aCurrentFocus, aTookFocus);
}

NS_IMETHODIMP nsContentTreeOwner::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);

   //XXX First Check In
   NS_ASSERTION(PR_FALSE, "Not Yet Implemented");
   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::SetTitle(const PRUnichar* aTitle)
{
   // We only allow the title to be set from the primary content shell
   if(!mPrimary || !mContentTitleSetting)
      return NS_OK;

   nsAutoString   title;
   nsAutoString   docTitle(aTitle);

   if(docTitle.Length() > 0)
      {
      if(mTitlePreface.Length() > 0)
         {
         // Title will be: "Preface: Doc Title - Mozilla"
         title.Assign(mTitlePreface);
         title.Append(docTitle);
         }
      else 
         {
         // Title will be: "Doc Title - Mozilla"
         title = docTitle;
         }
      title += mTitleSeparator + mWindowTitleModifier;
      }
   else 
      { 
      // Title will just be plain: Mozilla
      title.Assign(mWindowTitleModifier);
      }

   // XXX Don't need to fully qualify this once I remove nsWebShellWindow::SetTitle
   // return mXULWindow->SetTitle(title.GetUnicode());
   return mXULWindow->nsXULWindow::SetTitle(title.GetUnicode());
}

//*****************************************************************************
// nsContentTreeOwner: Helpers
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::ApplyChromeFlags()
{
   if(!mXULWindow->mChromeLoaded)
      return NS_OK;  // We'll do this later when chrome is loaded
      
   nsCOMPtr<nsIDOMElement> window;
   mXULWindow->GetWindowDOMElement(getter_AddRefs(window));
   NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
   
   mXULWindow->mWindow->ShowMenuBar(mChromeFlags & 
                                    nsIWebBrowserChrome::CHROME_MENUBAR ? 
                                    PR_TRUE : PR_FALSE);

   // Construct the new value for the 'chromehidden' attribute that
   // we'll whack onto the window. We've got style rules in
   // navigator.css that trigger visibility based on the
   // 'chromehidden' attribute of the <window> tag.
   nsAutoString newvalue;

   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_MENUBAR)) {
     newvalue.AppendWithConversion("menubar ");
   } 
   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_TOOLBAR)) {
     newvalue.AppendWithConversion("toolbar ");
   }
   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_LOCATIONBAR)) {
     newvalue.AppendWithConversion("location ");
   }
   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR)) {
     newvalue.AppendWithConversion("directories ");
   }
   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_STATUSBAR)) {
     newvalue.AppendWithConversion("status ");
   }
   if (! (mChromeFlags & nsIWebBrowserChrome::CHROME_EXTRA)) {
     newvalue.AppendWithConversion("extrachrome");
   }

   // Get the old value, to avoid useless style reflows if we're just
   // setting stuff to the exact same thing.
   nsAutoString oldvalue;
   window->GetAttribute(NS_ConvertASCIItoUCS2("chromehidden"), oldvalue);

   if (oldvalue != newvalue) {
     window->SetAttribute(NS_ConvertASCIItoUCS2("chromehidden"), newvalue);
   }

   return NS_OK;
}

//*****************************************************************************
// nsContentTreeOwner: Accessors
//*****************************************************************************   

void nsContentTreeOwner::XULWindow(nsXULWindow* aXULWindow)
{
   mXULWindow = aXULWindow;
   if(mXULWindow && mPrimary)
      {
      // Get the window title modifiers
      nsCOMPtr<nsIDOMElement> docShellElement;
      mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));

      nsAutoString   contentTitleSetting;

      if(docShellElement)  
         {
         docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("contenttitlesettting"), contentTitleSetting);
         if(contentTitleSetting.EqualsWithConversion("true"))
            {
            mContentTitleSetting = PR_TRUE;
            docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("titlemodifier"), mWindowTitleModifier);
            docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("titlemenuseparator"), mTitleSeparator);
            docShellElement->GetAttribute(NS_ConvertASCIItoUCS2("titlepreface"), mTitlePreface);
            }
         }
      else
         {
         NS_ERROR("This condition should never happen.  If it does, "
            "we just won't get a modifier, but it still shouldn't happen.");
         }
      }
}

nsXULWindow* nsContentTreeOwner::XULWindow()
{
   return mXULWindow;
}

