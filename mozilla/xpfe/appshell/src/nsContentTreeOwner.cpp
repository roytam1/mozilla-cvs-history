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
#include "nsIDOMXULElement.h"
#include "nsIPrompt.h"
#include "nsIWindowMediator.h"

// CIDs
static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);

//*****************************************************************************
//***    nsContentTreeOwner: Object Management
//*****************************************************************************

nsContentTreeOwner::nsContentTreeOwner(PRBool fPrimary) : mXULWindow(nsnull), 
   mPrimary(fPrimary), mChromeMask(0)
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
      return mXULWindow->QueryInterface(aIID, aSink);
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

   /* Special Cases */
   if(name.Length() == 0)
      return NS_OK;
   if(name.EqualsIgnoreCase("_blank"))
      return NS_OK;
   if(name.EqualsIgnoreCase("_content"))
      return mXULWindow->GetPrimaryContentShell(aFoundItem);

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
      if(shellAsTreeItem && (aRequestor != shellAsTreeItem.get()))
         {
         // Do this so we can pass in the tree owner as the requestor so the child knows not
         // to call back up.
         nsCOMPtr<nsIDocShellTreeOwner> shellOwner;
         shellAsTreeItem->GetTreeOwner(getter_AddRefs(shellOwner));
         nsCOMPtr<nsISupports> shellOwnerSupports(do_QueryInterface(shellOwner));

         shellAsTreeItem->FindItemWithName(aName, shellOwnerSupports, aFoundItem);
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

NS_IMETHODIMP nsContentTreeOwner::GetNewWindow(PRInt32 aChromeFlags,
   nsIDocShellTreeItem** aDocShellTreeItem)
{
   return mXULWindow->GetNewWindow(aChromeFlags, aDocShellTreeItem);
}

//*****************************************************************************
// nsContentTreeOwner::nsIWebBrowserChrome
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::SetJSStatus(const PRUnichar* aStatus)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SetJSDefaultStatus(const PRUnichar* aStatus)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SetOverLink(const PRUnichar* aLink)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
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

NS_IMETHODIMP nsContentTreeOwner::SetChromeMask(PRUint32 aChromeMask)
{
   mChromeMask = aChromeMask;
   NS_ENSURE_SUCCESS(ApplyChromeMask(), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::GetChromeMask(PRUint32* aChromeMask)
{
   NS_ENSURE_ARG_POINTER(aChromeMask);

   *aChromeMask = mChromeMask;
   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::GetNewBrowserChrome(nsIWebBrowserChrome**
   aWebBrowserChrome)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::FindNamedBrowserChrome(const PRUnichar* aName,
   nsIWebBrowserChrome** aWebBrowserChrome)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
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
   if(!mPrimary)
      return NS_OK;

   // Get the window title modifiers
   nsCOMPtr<nsIDOMElement> docShellElement;
   mXULWindow->GetDOMElementFromDocShell(mXULWindow->mDocShell, 
      getter_AddRefs(docShellElement));

   nsAutoString   windowTitleModifier;
   nsAutoString   titleSeparator;
   nsAutoString   titlePreface;

   if(docShellElement)
      {
      docShellElement->GetAttribute("titlemodifier", windowTitleModifier);
      docShellElement->GetAttribute("titlemenuseparator", titleSeparator);
      docShellElement->GetAttribute("titlepreface", titlePreface);
      }
   else
      {
      NS_ERROR("This condition should never happen.  If it does, "
         "we just won't get a modifier, but it still shouldn't happen.");
      }

   nsAutoString   title;
   nsAutoString   docTitle(aTitle);

   if(docTitle.Length() > 0)
      {
      if(titlePreface.Length() > 0)
         {
         // Title will be: "Preface: Doc Title - Mozilla"
         title = titlePreface + docTitle;
         }
      else 
         {
         // Title will be: "Doc Title - Mozilla"
         title = docTitle;
         }
      title += titleSeparator + windowTitleModifier;
      }
   else 
      { 
      // Title will just be plain: Mozilla
      title = windowTitleModifier;
      }

   // XXX Don't need to fully qualify this once I remove nsWebShellWindow::SetTitle
   // return mXULWindow->SetTitle(title.GetUnicode());
   return mXULWindow->nsXULWindow::SetTitle(title.GetUnicode());
}

//*****************************************************************************
// nsContentTreeOwner: Helpers
//*****************************************************************************   

NS_IMETHODIMP nsContentTreeOwner::ApplyChromeMask()
{
   if(!mXULWindow->mChromeLoaded)
      return NS_OK;  // We'll do this later when chrome is loaded
      
   nsCOMPtr<nsIDOMElement> domElement;
   mXULWindow->GetDOMElementFromDocShell(mXULWindow->mDocShell, 
      getter_AddRefs(domElement));
   NS_ENSURE_TRUE(domElement, NS_ERROR_FAILURE);
   
   mXULWindow->mWindow->ShowMenuBar(mChromeMask & 
                                    nsIWebBrowserChrome::menuBarOn ? 
                                    PR_TRUE : PR_FALSE);

   // get a list of this document's elements with the chromeclass attribute specified
   nsCOMPtr<nsIDOMXULElement> xulRoot(do_QueryInterface(domElement));
   NS_ENSURE_TRUE(xulRoot, NS_ERROR_FAILURE);

   // todo (maybe) the longer, straight DOM (not RDF) version?
   nsCOMPtr<nsIDOMNodeList> chromeNodes;
   xulRoot->GetElementsByAttribute("chromeclass", "*", 
      getter_AddRefs(chromeNodes));
   NS_ENSURE_TRUE(chromeNodes, NS_ERROR_FAILURE);

   PRUint32 nodeCtr;
   PRUint32 nodeCount;

   chromeNodes->GetLength(&nodeCount);

   for(nodeCtr = 0; nodeCtr < nodeCount; nodeCtr++)
      {
      nsCOMPtr<nsIDOMNode> domNode;
      chromeNodes->Item(nodeCtr, getter_AddRefs(domNode));
      nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(domNode));
      if(domElement)
         {
         nsAutoString chromeClass;
         PRBool       makeChange;
         PRUint32     flag;
         // show or hide the element according to its chromeclass and the chromemask
         domElement->GetAttribute("chromeclass", chromeClass);
         makeChange = PR_FALSE;
         if(chromeClass == "menubar")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::menuBarOn;
            } 
         else if(chromeClass == "toolbar")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::toolBarOn;
            }
         else if(chromeClass == "location")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::locationBarOn;
            } 
         else if(chromeClass == "directories")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::personalToolBarOn;
            } 
         else if(chromeClass == "status")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::statusBarOn;
            }
         else if(chromeClass == "extrachrome")
            {
            makeChange = PR_TRUE;
            flag = mChromeMask & nsIWebBrowserChrome::extraChromeOn;
            }

         if(makeChange)
            {
            if(flag)
               domElement->RemoveAttribute("chromehidden");
            else
               domElement->SetAttribute("chromehidden", "T");
            }
         }
      }
   
   return NS_OK;
}

//*****************************************************************************
// nsContentTreeOwner: Accessors
//*****************************************************************************   

void nsContentTreeOwner::XULWindow(nsXULWindow* aXULWindow)
{
   mXULWindow = aXULWindow;
}

nsXULWindow* nsContentTreeOwner::XULWindow()
{
   return mXULWindow;
}

