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
 */

// Local Includes
#include "nsWebBrowser.h"
#include "nsWebBrowserPersist.h"
#include "nsWebBrowserFind.h"

// Helper Classes
#include "nsGfxCIID.h"
#include "nsWidgetsCID.h"

//Interfaces Needed
#include "nsIComponentManager.h"
#include "nsIDeviceContext.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebShell.h"
#include "nsPIDOMWindow.h"
#include "nsIFocusController.h"
#include "nsIDOMWindowInternal.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserFocus.h"
#include "nsIPresShell.h"

// Printing Includes
#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"
// Print Options
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);

static NS_DEFINE_CID(kWebShellCID,         NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kChildCID,               NS_CHILD_CID);
static NS_DEFINE_IID(kDeviceContextCID,       NS_DEVICE_CONTEXT_CID);

//*****************************************************************************
//***    nsWebBrowser: Object Management
//*****************************************************************************

nsWebBrowser::nsWebBrowser() : mDocShellTreeOwner(nsnull), 
   mInitInfo(nsnull), mContentType(typeContentWrapper),
   mParentNativeWindow(nsnull), mParentWidget(nsnull), mParent(nsnull),
   mProgressListener(nsnull), mListenerArray(nsnull), mFindImpl(nsnull)
{
    NS_INIT_REFCNT();
    mInitInfo = new nsWebBrowserInitInfo();
}

nsWebBrowser::~nsWebBrowser()
{
   InternalDestroy();
}

PRBool PR_CALLBACK deleteListener(void *aElement, void *aData) {
    nsWebBrowserListenerState *state = (nsWebBrowserListenerState*)aElement;
    NS_DELETEXPCOM(state);
    return PR_TRUE;
}

NS_IMETHODIMP nsWebBrowser::InternalDestroy()
{

  if (mInternalWidget)
    mInternalWidget->SetClientData(0);

   SetDocShell(nsnull);

   if(mDocShellTreeOwner)
      {
      mDocShellTreeOwner->WebBrowser(nsnull);
      NS_RELEASE(mDocShellTreeOwner);
      }
   if(mInitInfo)
      {
      delete mInitInfo;
      mInitInfo = nsnull;
      }

   if (mListenerArray) {
      (void)mListenerArray->EnumerateForwards(deleteListener, nsnull);
      delete mListenerArray;
      mListenerArray = nsnull;
   }
   if (mFindImpl) {
      delete mFindImpl;
      mFindImpl = nsnull;
   }

   return NS_OK;
}


//*****************************************************************************
// nsWebBrowser::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(nsWebBrowser)
NS_IMPL_RELEASE(nsWebBrowser)

NS_INTERFACE_MAP_BEGIN(nsWebBrowser)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowser)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowser)
    NS_INTERFACE_MAP_ENTRY(nsIWebNavigation)
    NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
    NS_INTERFACE_MAP_ENTRY(nsIScrollable)
    NS_INTERFACE_MAP_ENTRY(nsITextScroll)
    NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeItem)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserSetup)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserPersist)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserFocus)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserFind)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserPrint)
NS_INTERFACE_MAP_END

///*****************************************************************************
// nsWebBrowser::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP nsWebBrowser::GetInterface(const nsIID& aIID, void** aSink)
{
   NS_ENSURE_ARG_POINTER(aSink);

   if(NS_SUCCEEDED(QueryInterface(aIID, aSink)))
      return NS_OK;

   if(mDocShell)
      return mDocShellAsReq->GetInterface(aIID, aSink);

   return NS_NOINTERFACE;
}

//*****************************************************************************
// nsWebBrowser::nsIWebBrowser
//*****************************************************************************   

// listeners that currently support registration through AddWebBrowserListener:
//  - nsIWebProgressListener
NS_IMETHODIMP nsWebBrowser::AddWebBrowserListener(nsIWeakReference *aListener, const nsIID& aIID)
{           
    nsresult rv = NS_ERROR_INVALID_ARG;
    NS_ENSURE_ARG_POINTER(aListener);

    if (!mWebProgress) {
        // The window hasn't been created yet, so queue up the listener. They'll be
        // registered when the window gets created.
        nsWebBrowserListenerState *state = nsnull;
        NS_NEWXPCOM(state, nsWebBrowserListenerState);
        if (!state) return NS_ERROR_OUT_OF_MEMORY;

        state->mWeakPtr = aListener;
        state->mID = aIID;

        if (!mListenerArray) {
            NS_NEWXPCOM(mListenerArray, nsVoidArray);
            if (!mListenerArray) return NS_ERROR_OUT_OF_MEMORY;
        }

        if (!mListenerArray->AppendElement(state)) return NS_ERROR_OUT_OF_MEMORY;
    } else {
        nsCOMPtr<nsISupports> supports(do_QueryReferent(aListener));
        if (!supports) return NS_ERROR_INVALID_ARG;
        rv = BindListener(supports, aIID);
    }
    
    return rv;
}

NS_IMETHODIMP nsWebBrowser::BindListener(nsISupports *aListener, const nsIID& aIID) {
    NS_ASSERTION(aListener, "invalid args");
    NS_ASSERTION(mWebProgress, "this should only be called after we've retrieved a progress iface");
    nsresult rv = NS_OK;

    // register this listener for the specified interface id
    if (aIID.Equals(NS_GET_IID(nsIWebProgressListener))) {
        nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(aListener, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = mWebProgress->AddProgressListener(listener);
    }
    else if (aIID.Equals(NS_GET_IID(nsISHistoryListener))) {      
      nsCOMPtr<nsISHistory> shistory(do_GetInterface(mDocShell, &rv));
      if (NS_FAILED(rv)) return rv;
      nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(aListener, &rv));
      if (NS_FAILED(rv)) return rv;
      rv = shistory->AddSHistoryListener(listener);
    }
    return rv;
}

NS_IMETHODIMP nsWebBrowser::RemoveWebBrowserListener(nsIWeakReference *aListener, const nsIID& aIID)
{
    nsresult rv = NS_ERROR_INVALID_ARG;
    NS_ENSURE_ARG_POINTER(aListener);

    if (!mWebProgress) {
        // if there's no-one to register the listener w/, and we don't have a queue going,
        // the the called is calling Remove before an Add which doesn't make sense.
        if (!mListenerArray) return NS_ERROR_FAILURE;

        // iterate the array and remove the queued listener
        PRInt32 count = mListenerArray->Count();
        while (count > 0) {
            nsWebBrowserListenerState *state = (nsWebBrowserListenerState*)mListenerArray->ElementAt(count);
            NS_ASSERTION(state, "list construction problem");

            if (state->Equals(aListener, aIID)) {
                // this is the one, pull it out.
                mListenerArray->RemoveElementAt(count);
                break;
            }
            count--; 
        }

        // if we've emptied the array, get rid of it.
        if (0 >= mListenerArray->Count()) {
            (void)mListenerArray->EnumerateForwards(deleteListener, nsnull);
            NS_DELETEXPCOM(mListenerArray);
            mListenerArray = nsnull;
        }

    } else {
        nsCOMPtr<nsISupports> supports(do_QueryReferent(aListener));
        if (!supports) return NS_ERROR_INVALID_ARG;
        rv = UnBindListener(aListener, aIID);
    }
    
    return rv;
}

NS_IMETHODIMP nsWebBrowser::UnBindListener(nsISupports *aListener, const nsIID& aIID) {
    NS_ASSERTION(aListener, "invalid args");
    NS_ASSERTION(mWebProgress, "this should only be called after we've retrieved a progress iface");
    nsresult rv = NS_OK;

    // remove the listener for the specified interface id
    if (aIID.Equals(NS_GET_IID(nsIWebProgressListener))) {
        nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(aListener, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = mWebProgress->RemoveProgressListener(listener);
    }
    else if (aIID.Equals(NS_GET_IID(nsISHistoryListener))) {
      nsCOMPtr<nsISHistory> shistory(do_GetInterface(mDocShell, &rv));
      if (NS_FAILED(rv)) return rv;
      nsCOMPtr<nsISHistoryListener> listener(do_QueryInterface(aListener, &rv));
      if (NS_FAILED(rv)) return rv;
      rv = shistory->RemoveSHistoryListener(listener);
    }
    return rv;
}

NS_IMETHODIMP nsWebBrowser::GetContainerWindow(nsIWebBrowserChrome** aTopWindow)
{
   NS_ENSURE_ARG_POINTER(aTopWindow);

   if(mDocShellTreeOwner)
      *aTopWindow = mDocShellTreeOwner->mWebBrowserChrome;
   else
      *aTopWindow = nsnull;
   NS_IF_ADDREF(*aTopWindow);

   return NS_OK;   
}

NS_IMETHODIMP nsWebBrowser::SetContainerWindow(nsIWebBrowserChrome* aTopWindow)
{
   NS_ENSURE_SUCCESS(EnsureDocShellTreeOwner(), NS_ERROR_FAILURE);
   return mDocShellTreeOwner->SetWebBrowserChrome(aTopWindow);
}

NS_IMETHODIMP nsWebBrowser::GetParentURIContentListener(nsIURIContentListener**
   aParentContentListener)
{
   NS_ENSURE_ARG_POINTER(aParentContentListener);

   nsCOMPtr<nsIURIContentListener> listener(do_QueryInterface(mDocShell));
   NS_ASSERTION(listener, "docshell is no longer a content listner");

   return listener->GetParentContentListener(aParentContentListener);
}

NS_IMETHODIMP nsWebBrowser::SetParentURIContentListener(nsIURIContentListener*
   aParentContentListener)
{
   nsCOMPtr<nsIURIContentListener> listener(do_QueryInterface(mDocShell));
   NS_ASSERTION(listener, "docshell is no longer a content listner");

   return listener->SetParentContentListener(aParentContentListener);
}

NS_IMETHODIMP nsWebBrowser::GetContentDOMWindow(nsIDOMWindow **_retval)
{
    NS_ENSURE_STATE(mDocShell);
    nsresult rv = NS_OK;
    nsCOMPtr<nsIDOMWindow> retval = do_GetInterface(mDocShell, &rv);
    if (NS_FAILED(rv)) return rv;

    *_retval = retval;
    NS_ADDREF(*_retval);
    return rv;
}

//*****************************************************************************
// nsWebBrowser::nsIDocShellTreeItem
//*****************************************************************************   

NS_IMETHODIMP nsWebBrowser::GetName(PRUnichar** aName)
{
   NS_ENSURE_ARG_POINTER(aName);

   if(mDocShell)  
      mDocShellAsItem->GetName(aName);
   else
      *aName = mInitInfo->name.ToNewUnicode();

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetName(const PRUnichar* aName)
{
   if(mDocShell)
      {
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
      NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

      return docShellAsItem->SetName(aName);
      }
   else
      mInitInfo->name = aName;

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetItemType(PRInt32* aItemType)
{
   NS_ENSURE_ARG_POINTER(aItemType);

   *aItemType = mContentType;
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetItemType(PRInt32 aItemType)
{
    NS_ENSURE_TRUE((aItemType == typeContentWrapper || aItemType == typeChromeWrapper), NS_ERROR_FAILURE);
    mContentType = aItemType;
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetParent(nsIDocShellTreeItem** aParent)
{
   NS_ENSURE_ARG_POINTER(aParent);

   *aParent = mParent;
   NS_IF_ADDREF(*aParent);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetParent(nsIDocShellTreeItem* aParent)
{
  // null aParent is ok
   
  /* Note this doesn't do an addref on purpose.  This is because the parent
   is an implied lifetime.  We don't want to create a cycle by refcounting
   the parent.*/
  mParent = aParent;
  return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetSameTypeParent(nsIDocShellTreeItem** aParent)
{
   NS_ENSURE_ARG_POINTER(aParent);
   *aParent = nsnull;

   if(!mParent)
      return NS_OK;
      
   PRInt32  parentType;
   NS_ENSURE_SUCCESS(mParent->GetItemType(&parentType), NS_ERROR_FAILURE);

   if(typeContentWrapper == parentType)
      {
      *aParent = mParent;
      NS_ADDREF(*aParent);
      }                   
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetRootTreeItem(nsIDocShellTreeItem** aRootTreeItem)
{
   NS_ENSURE_ARG_POINTER(aRootTreeItem);
   *aRootTreeItem = NS_STATIC_CAST(nsIDocShellTreeItem*, this);

   nsCOMPtr<nsIDocShellTreeItem> parent;
   NS_ENSURE_SUCCESS(GetParent(getter_AddRefs(parent)), NS_ERROR_FAILURE);
   while(parent)
      {
      *aRootTreeItem = parent;
      NS_ENSURE_SUCCESS((*aRootTreeItem)->GetParent(getter_AddRefs(parent)), NS_ERROR_FAILURE);
      }
   NS_ADDREF(*aRootTreeItem);
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetSameTypeRootTreeItem(nsIDocShellTreeItem** aRootTreeItem)
{
   NS_ENSURE_ARG_POINTER(aRootTreeItem);
   *aRootTreeItem = NS_STATIC_CAST(nsIDocShellTreeItem*, this);

   nsCOMPtr<nsIDocShellTreeItem> parent;
   NS_ENSURE_SUCCESS(GetSameTypeParent(getter_AddRefs(parent)), NS_ERROR_FAILURE);
   while(parent)
      {
      *aRootTreeItem = parent;
      NS_ENSURE_SUCCESS((*aRootTreeItem)->GetSameTypeParent(getter_AddRefs(parent)), 
         NS_ERROR_FAILURE);
      }
   NS_ADDREF(*aRootTreeItem);
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::FindItemWithName(const PRUnichar *aName, 
   nsISupports* aRequestor, nsIDocShellTreeItem **_retval)
{
   NS_ENSURE_STATE(mDocShell);
   NS_ASSERTION(mDocShellTreeOwner, "This should always be set when in this situation");

   return mDocShellAsItem->FindItemWithName(aName, 
      NS_STATIC_CAST(nsIDocShellTreeOwner*, mDocShellTreeOwner), _retval);
}

NS_IMETHODIMP nsWebBrowser::GetTreeOwner(nsIDocShellTreeOwner** aTreeOwner)
{  
    NS_ENSURE_ARG_POINTER(aTreeOwner);
    *aTreeOwner = nsnull;
    if(mDocShellTreeOwner)
    {
        if (mDocShellTreeOwner->mTreeOwner)
        {
            *aTreeOwner = mDocShellTreeOwner->mTreeOwner;
        }
        else
        {
            *aTreeOwner = mDocShellTreeOwner;
        }
    }
    NS_IF_ADDREF(*aTreeOwner);
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetTreeOwner(nsIDocShellTreeOwner* aTreeOwner)
{
   NS_ENSURE_SUCCESS(EnsureDocShellTreeOwner(), NS_ERROR_FAILURE);
   return mDocShellTreeOwner->SetTreeOwner(aTreeOwner);
}

NS_IMETHODIMP nsWebBrowser::SetChildOffset(PRInt32 aChildOffset)
{
  // Not implemented
  return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetChildOffset(PRInt32 *aChildOffset)
{
  // Not implemented
  return NS_OK;
}

//*****************************************************************************
// nsWebBrowser::nsIWebNavigation
//*****************************************************************************

NS_IMETHODIMP nsWebBrowser::GetCanGoBack(PRBool* aCanGoBack)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GetCanGoBack(aCanGoBack);
}

NS_IMETHODIMP nsWebBrowser::GetCanGoForward(PRBool* aCanGoForward)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GetCanGoForward(aCanGoForward);
}

NS_IMETHODIMP nsWebBrowser::GoBack()
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GoBack();
}

NS_IMETHODIMP nsWebBrowser::GoForward()
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GoForward();
}

NS_IMETHODIMP nsWebBrowser::LoadURI(const PRUnichar* aURI, PRUint32 aLoadFlags)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->LoadURI(aURI, aLoadFlags);
}

NS_IMETHODIMP nsWebBrowser::Reload(PRUint32 aReloadFlags)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->Reload(aReloadFlags);
}

NS_IMETHODIMP nsWebBrowser::GotoIndex(PRInt32 aIndex)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GotoIndex(aIndex);
}

NS_IMETHODIMP nsWebBrowser::Stop()
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->Stop();
}

NS_IMETHODIMP nsWebBrowser::GetCurrentURI(nsIURI** aURI)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GetCurrentURI(aURI);
}

NS_IMETHODIMP nsWebBrowser::SetSessionHistory(nsISHistory* aSessionHistory)
{
   if(mDocShell)
      return mDocShellAsNav->SetSessionHistory(aSessionHistory);
   else
      mInitInfo->sessionHistory = aSessionHistory;

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetSessionHistory(nsISHistory** aSessionHistory)
{
   NS_ENSURE_ARG_POINTER(aSessionHistory);
   if(mDocShell)
      return mDocShellAsNav->GetSessionHistory(aSessionHistory);
   else
      *aSessionHistory = mInitInfo->sessionHistory;

   NS_IF_ADDREF(*aSessionHistory);

   return NS_OK;
}


NS_IMETHODIMP nsWebBrowser::GetDocument(nsIDOMDocument** aDocument)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsNav->GetDocument(aDocument);
}


//*****************************************************************************
// nsWebBrowser::nsIWebBrowserSetup
//*****************************************************************************

/* void setProperty (in unsigned long aId, in unsigned long aValue); */
NS_IMETHODIMP nsWebBrowser::SetProperty(PRUint32 aId, PRUint32 aValue)
{
    switch (aId)
    {
    case nsIWebBrowserSetup::SETUP_ALLOW_PLUGINS:
        {
           NS_ENSURE_STATE(mDocShell);
           NS_ENSURE_TRUE((aValue == PR_TRUE || aValue == PR_FALSE), NS_ERROR_INVALID_ARG);
           mDocShell->SetAllowPlugins(aValue);
        }
        break;
    case nsIWebBrowserSetup::SETUP_ALLOW_JAVASCRIPT:
        {
           NS_ENSURE_STATE(mDocShell);
           NS_ENSURE_TRUE((aValue == PR_TRUE || aValue == PR_FALSE), NS_ERROR_INVALID_ARG);
           mDocShell->SetAllowJavascript(aValue);
        }
        break;
    case nsIWebBrowserSetup::SETUP_ALLOW_META_REDIRECTS:
        {
           NS_ENSURE_STATE(mDocShell);
           NS_ENSURE_TRUE((aValue == PR_TRUE || aValue == PR_FALSE), NS_ERROR_INVALID_ARG);
           mDocShell->SetAllowMetaRedirects(aValue);
        }
        break;
    default:
        return NS_ERROR_INVALID_ARG;
  
    }
    return NS_OK;
}

//*****************************************************************************
// nsWebBrowser::nsIWebBrowserPersist
//*****************************************************************************

/* attribute nsIWebBrowserPersistProgress progressListener; */
NS_IMETHODIMP nsWebBrowser::GetProgressListener(nsIWebBrowserPersistProgress * *aProgressListener)
{
    NS_ENSURE_ARG_POINTER(aProgressListener);
    *aProgressListener = mProgressListener;
    NS_IF_ADDREF(*aProgressListener);
    return NS_OK;
}
  
NS_IMETHODIMP nsWebBrowser::SetProgressListener(nsIWebBrowserPersistProgress * aProgressListener)
{
    mProgressListener = aProgressListener;
    return NS_OK;
}

/* void saveURI (in nsIURI aURI, in string aFileName); */
NS_IMETHODIMP nsWebBrowser::SaveURI(nsIURI *aURI, nsIInputStream *aPostData, const char *aFileName)
{
    // Create a throwaway persistence object to do the work
    nsWebBrowserPersist *persist = new nsWebBrowserPersist();
    if (persist == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    persist->AddRef();
    persist->SetProgressListener(mProgressListener);
    nsresult rv = persist->SaveURI(aURI, aPostData, aFileName);
    persist->Release();
    return rv;
}

/* void saveCurrentURI (in string aFileName); */
NS_IMETHODIMP nsWebBrowser::SaveCurrentURI(const char *aFileName)
{
    // Get the current URI
    nsCOMPtr<nsIURI> uri;
    nsresult rv = GetCurrentURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
    {
        return NS_ERROR_FAILURE;
    }

    return SaveURI(uri, nsnull, aFileName);
}

/* void saveDocument (in nsIDOMDocument document); */
NS_IMETHODIMP nsWebBrowser::SaveDocument(nsIDOMDocument *aDocument, const char *aFileName, const char *aDataPath)
{
    // Use the specified DOM document, or if none is specified, the one
    // attached to the web browser.

    nsCOMPtr<nsIDOMDocument> doc;
    if (aDocument)
    {
        doc = do_QueryInterface(aDocument);
    }
    else
    {
        GetDocument(getter_AddRefs(doc));
    }
    if (!doc)
    {
        return NS_ERROR_FAILURE;
    }

    // Create a throwaway persistence object to do the work
    nsWebBrowserPersist *persist = new nsWebBrowserPersist();
    if (persist == nsnull)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    persist->AddRef();
    persist->SetProgressListener(mProgressListener);
    nsresult rv = persist->SaveDocument(doc, aFileName, aDataPath);
    persist->Release();
    return rv;
}

//*****************************************************************************
// nsWebBrowser::nsIWebBrowserFind
//*****************************************************************************

/* boolean findNext (); */
NS_IMETHODIMP nsWebBrowser::FindNext(PRBool *didFind)
{
    NS_ENSURE_ARG_POINTER(didFind);
    *didFind = PR_FALSE;

    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
        
    NS_ENSURE_TRUE(mFindImpl->CanFindNext(), NS_ERROR_NOT_INITIALIZED);
    
    nsCOMPtr<nsIDOMWindow> windowToSearch;
    rv = GetFocusedWindow(getter_AddRefs(windowToSearch));
    if (!windowToSearch) {
        nsCOMPtr<nsIDOMWindowInternal> contentDomWindow;
        rv = GetPrimaryContentWindow(getter_AddRefs(contentDomWindow));
        windowToSearch = contentDomWindow;
    }
    
    NS_ENSURE_TRUE(windowToSearch, NS_ERROR_FAILURE);
    rv = mFindImpl->DoFind(windowToSearch, didFind);
    
    return rv;
}

/* attribute wstring searchString; */
NS_IMETHODIMP nsWebBrowser::GetSearchString(PRUnichar * *aSearchString)
{
    NS_ENSURE_ARG_POINTER(aSearchString);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    return mFindImpl->GetSearchString(aSearchString);
}

NS_IMETHODIMP nsWebBrowser::SetSearchString(const PRUnichar * aSearchString)
{
    NS_ENSURE_ARG(aSearchString);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    return mFindImpl->SetSearchString(aSearchString);
}

/* attribute boolean findBackwards; */
NS_IMETHODIMP nsWebBrowser::GetFindBackwards(PRBool *aFindBackwards)
{
    NS_ENSURE_ARG_POINTER(aFindBackwards);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    *aFindBackwards = mFindImpl->GetFindBackwards();
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetFindBackwards(PRBool aFindBackwards)
{
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    mFindImpl->SetFindBackwards(aFindBackwards);
    return NS_OK;
}

/* attribute boolean wrapFind; */
NS_IMETHODIMP nsWebBrowser::GetWrapFind(PRBool *aWrapFind)
{
    NS_ENSURE_ARG_POINTER(aWrapFind);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    *aWrapFind = mFindImpl->GetWrapFind();
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetWrapFind(PRBool aWrapFind)
{
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    mFindImpl->SetWrapFind(aWrapFind);
    return NS_OK;
}

/* attribute boolean entireWord; */
NS_IMETHODIMP nsWebBrowser::GetEntireWord(PRBool *aEntireWord)
{
    NS_ENSURE_ARG_POINTER(aEntireWord);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    *aEntireWord = mFindImpl->GetEntireWord();
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetEntireWord(PRBool aEntireWord)
{
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    mFindImpl->SetEntireWord(aEntireWord);
    return NS_OK;
}

/* attribute boolean matchCase; */
NS_IMETHODIMP nsWebBrowser::GetMatchCase(PRBool *aMatchCase)
{
    NS_ENSURE_ARG_POINTER(aMatchCase);
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    *aMatchCase = mFindImpl->GetMatchCase();
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetMatchCase(PRBool aMatchCase)
{
    nsresult rv = EnsureFindImpl();
    if (NS_FAILED(rv)) return rv;
    mFindImpl->SetMatchCase(aMatchCase);
    return NS_OK;
}


//*****************************************************************************
// nsWebBrowser::nsIBaseWindow
//*****************************************************************************

NS_IMETHODIMP nsWebBrowser::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* aParentWidget, PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY)   
{
   NS_ENSURE_ARG(aParentNativeWindow || aParentWidget);
   NS_ENSURE_STATE(!mDocShell || mInitInfo);

   if(aParentWidget)
      NS_ENSURE_SUCCESS(SetParentWidget(aParentWidget), NS_ERROR_FAILURE);
   else
      NS_ENSURE_SUCCESS(SetParentNativeWindow(aParentNativeWindow),
         NS_ERROR_FAILURE);

   NS_ENSURE_SUCCESS(SetPositionAndSize(aX, aY, aCX, aCY, PR_FALSE),
      NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::Create()
{
   NS_ENSURE_STATE(!mDocShell && (mParentNativeWindow || mParentWidget));

   NS_ENSURE_SUCCESS(EnsureDocShellTreeOwner(), NS_ERROR_FAILURE);

   nsCOMPtr<nsIWidget> docShellParentWidget(mParentWidget);
   if(!mParentWidget) // We need to create a widget
      {
      nsCOMPtr<nsIDeviceContext> deviceContext = 
                                          do_CreateInstance(kDeviceContextCID);
      NS_ENSURE_TRUE(deviceContext, NS_ERROR_FAILURE);

      deviceContext->Init(mParentNativeWindow);
      float dev2twip;
      deviceContext->GetDevUnitsToTwips(dev2twip);
      deviceContext->SetDevUnitsToAppUnits(dev2twip);
      float twip2dev;
      deviceContext->GetTwipsToDevUnits(twip2dev);
      deviceContext->SetAppUnitsToDevUnits(twip2dev);
      deviceContext->SetGamma(1.0f);

      // Create the widget
      NS_ENSURE_TRUE(mInternalWidget = do_CreateInstance(kChildCID), NS_ERROR_FAILURE);

      docShellParentWidget = mInternalWidget;
      nsWidgetInitData  widgetInit;

      widgetInit.clipChildren = PR_FALSE;
      widgetInit.mWindowType = eWindowType_child;
      nsRect bounds(mInitInfo->x, mInitInfo->y, mInitInfo->cx, mInitInfo->cy);
      
      mInternalWidget->SetClientData(NS_STATIC_CAST(nsWebBrowser *, this));
      mInternalWidget->Create(mParentNativeWindow, bounds, nsWebBrowser::HandleEvent,
         deviceContext, nsnull, nsnull, &widgetInit);  
      }

   nsCOMPtr<nsIDocShell> docShell(do_CreateInstance(kWebShellCID));
   NS_ENSURE_SUCCESS(SetDocShell(docShell), NS_ERROR_FAILURE);

   // the docshell has been set so we now have our listener registrars.
   if (mListenerArray) {
      // we had queued up some listeners, let's register them now.
      PRInt32 count = mListenerArray->Count();
      PRInt32 i = 0;
      NS_ASSERTION(count > 0, "array construction problem");
      while (i < count) {
          nsWebBrowserListenerState *state = (nsWebBrowserListenerState*)mListenerArray->ElementAt(i);
          NS_ASSERTION(state, "array construction problem");
          nsCOMPtr<nsISupports> listener = do_QueryReferent(state->mWeakPtr);
          NS_ASSERTION(listener, "bad listener");
          (void)BindListener(listener, state->mID);
          i++;
      }
      (void)mListenerArray->EnumerateForwards(deleteListener, nsnull);
      NS_DELETEXPCOM(mListenerArray);
      mListenerArray = nsnull;
   }

   // HACK ALERT - this registration registers the nsDocShellTreeOwner as a 
   // nsIWebBrowserListener so it can setup it's MouseListener in one of the 
   // progress callbacks. If we can register the MouseListener another way, this 
   // registration can go away, and nsDocShellTreeOwner can stop implementing
   // nsIWebProgressListener.
   nsCOMPtr<nsISupports> supports = nsnull;
   (void)mDocShellTreeOwner->QueryInterface(NS_GET_IID(nsIWebProgressListener),
                             NS_STATIC_CAST(void**, getter_AddRefs(supports)));
   (void)BindListener(supports, NS_GET_IID(nsIWebProgressListener));

   NS_ENSURE_SUCCESS(mDocShellAsWin->InitWindow(nsnull,
      docShellParentWidget, mInitInfo->x, mInitInfo->y, mInitInfo->cx,
      mInitInfo->cy), NS_ERROR_FAILURE);

   mDocShellAsItem->SetName(mInitInfo->name.GetUnicode());
   if (mContentType == typeChromeWrapper)
   {
       mDocShellAsItem->SetItemType(nsIDocShellTreeItem::typeChrome);
   }
   else
   {
       mDocShellAsItem->SetItemType(nsIDocShellTreeItem::typeContent);
   }
   mDocShellAsItem->SetTreeOwner(mDocShellTreeOwner);
   
   // If the webbrowser is a content docshell item then we won't hear any
   // events from subframes. To solve that we install our own chrome event handler
   // that always gets called (even for subframes) for any bubbling event.

   if(!mInitInfo->sessionHistory)
      mInitInfo->sessionHistory = do_CreateInstance(NS_SHISTORY_CONTRACTID);
   NS_ENSURE_TRUE(mInitInfo->sessionHistory, NS_ERROR_FAILURE);
   mDocShellAsNav->SetSessionHistory(mInitInfo->sessionHistory);
      
   NS_ENSURE_SUCCESS(mDocShellAsWin->Create(), NS_ERROR_FAILURE);

   mDocShellTreeOwner->AddToWatcher(); // evil twin of Remove in SetDocShell(0)

   delete mInitInfo;
   mInitInfo = nsnull;

   return NS_OK; 
}

NS_IMETHODIMP nsWebBrowser::Destroy()
{
   InternalDestroy();

   if(!mInitInfo)
      mInitInfo = new nsWebBrowserInitInfo();

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetPosition(PRInt32 aX, PRInt32 aY)
{
   PRInt32 cx = 0;
   PRInt32 cy = 0;

   GetSize(&cx, &cy);

   return SetPositionAndSize(aX, aY, cx, cy, PR_FALSE);
}

NS_IMETHODIMP nsWebBrowser::GetPosition(PRInt32* aX, PRInt32* aY)
{
   return GetPositionAndSize(aX, aY, nsnull, nsnull);
}

NS_IMETHODIMP nsWebBrowser::SetSize(PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
   PRInt32 x = 0;
   PRInt32 y = 0;

   GetPosition(&x, &y);

   return SetPositionAndSize(x, y, aCX, aCY, aRepaint);
}

NS_IMETHODIMP nsWebBrowser::GetSize(PRInt32* aCX, PRInt32* aCY)
{
   return GetPositionAndSize(nsnull, nsnull, aCX, aCY);
}

NS_IMETHODIMP nsWebBrowser::SetPositionAndSize(PRInt32 aX, PRInt32 aY,
   PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
   if(!mDocShell)
      {
      mInitInfo->x = aX;
      mInitInfo->y = aY;
      mInitInfo->cx = aCX;
      mInitInfo->cy = aCY;
      }
   else
      {
      PRInt32 doc_x = aX;
      PRInt32 doc_y = aY;

      // If there is an internal widget we need to make the docShell coordinates
      // relative to the internal widget rather than the calling app's parent.
      // We also need to resize our widget then.
      if(mInternalWidget)
         {
         doc_x = doc_y = 0;
         NS_ENSURE_SUCCESS(mInternalWidget->Resize(aX, aY, aCX, aCY, aRepaint),
            NS_ERROR_FAILURE);
         }
      // Now reposition/ resize the doc
      NS_ENSURE_SUCCESS(mDocShellAsWin->SetPositionAndSize(doc_x, doc_y, aCX, aCY, 
         aRepaint), NS_ERROR_FAILURE);
      }

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetPositionAndSize(PRInt32* aX, PRInt32* aY, 
   PRInt32* aCX, PRInt32* aCY)
{
   if(!mDocShell)
      {
      if(aX)
         *aX = mInitInfo->x;
      if(aY)
         *aY = mInitInfo->y;
      if(aCX)
         *aCX = mInitInfo->cx;
      if(aCY)
         *aCY = mInitInfo->cy;
      }
   else
      {
      if(mInternalWidget)
         {
         nsRect bounds;
         NS_ENSURE_SUCCESS(mInternalWidget->GetBounds(bounds), NS_ERROR_FAILURE);

         if(aX)
            *aX = bounds.x;
         if(aY)
            *aY = bounds.y;
         if(aCX)
            *aCX = bounds.width;
         if(aCY)
            *aCY = bounds.height;
         return NS_OK;
         }
      else
         return mDocShellAsWin->GetPositionAndSize(aX, aY, aCX, aCY); // Can directly return this as it is the
      }
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::Repaint(PRBool aForce)
{
   NS_ENSURE_STATE(mDocShell);
   return mDocShellAsWin->Repaint(aForce); // Can directly return this as it is the
}                                     // same interface, thus same returns.

NS_IMETHODIMP nsWebBrowser::GetParentWidget(nsIWidget** aParentWidget)
{
   NS_ENSURE_ARG_POINTER(aParentWidget);

   *aParentWidget = mParentWidget;

   NS_IF_ADDREF(*aParentWidget);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetParentWidget(nsIWidget* aParentWidget)
{
   NS_ENSURE_STATE(!mDocShell);

   mParentWidget = aParentWidget;
   if(mParentWidget)
      mParentNativeWindow = mParentWidget->GetNativeData(NS_NATIVE_WIDGET);
   else
      mParentNativeWindow = nsnull;

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   NS_ENSURE_ARG_POINTER(aParentNativeWindow);
   
   *aParentNativeWindow = mParentNativeWindow;

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   NS_ENSURE_STATE(!mDocShell);

   mParentNativeWindow = aParentNativeWindow;

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetVisibility(PRBool* visibility)
{
   NS_ENSURE_ARG_POINTER(visibility);

   if(!mDocShell)
      *visibility = mInitInfo->visible;
   else
      NS_ENSURE_SUCCESS(mDocShellAsWin->GetVisibility(visibility), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetVisibility(PRBool aVisibility)
{
   if(!mDocShell)
      mInitInfo->visible = aVisibility;
   else
      {
      NS_ENSURE_SUCCESS(mDocShellAsWin->SetVisibility(aVisibility), NS_ERROR_FAILURE);
      if(mInternalWidget)
         mInternalWidget->Show(aVisibility);
      }

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetMainWidget(nsIWidget** mainWidget)
{
   NS_ENSURE_ARG_POINTER(mainWidget);

   if(mInternalWidget)
      *mainWidget = mInternalWidget;
   else
      *mainWidget = mParentWidget;

   NS_IF_ADDREF(*mainWidget);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetFocus()
{
   NS_ENSURE_STATE(mDocShell);

   NS_ENSURE_SUCCESS(mDocShellAsWin->SetFocus(), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::FocusAvailable(nsIBaseWindow* aCurrentFocus,
   PRBool* aTookFocus)
{
   NS_ENSURE_ARG_POINTER(aTookFocus);

   // Next person we should call is first the parent otherwise the 
   // docshell tree owner.
   nsCOMPtr<nsIBaseWindow> nextCallWin(do_QueryInterface(mParent));
   if(!nextCallWin)
      nextCallWin = do_QueryInterface(nsnull /*mTreeOwner*/);

   //If the current focus is us, offer it to the next owner.
   if(aCurrentFocus == NS_STATIC_CAST(nsIBaseWindow*, this))
      {
      if(nextCallWin)
         return nextCallWin->FocusAvailable(aCurrentFocus, aTookFocus);
      return NS_OK;
      }

   //Otherwise, check the chilren and offer it to the next sibling.
   if((mDocShellAsWin.get() != aCurrentFocus) &&
      NS_SUCCEEDED(mDocShellAsWin->SetFocus()))
      {
      *aTookFocus = PR_TRUE;
      return NS_OK;
      }

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);
   NS_ENSURE_STATE(mDocShell);

   NS_ENSURE_SUCCESS(mDocShellAsWin->GetTitle(aTitle), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::SetTitle(const PRUnichar* aTitle)
{
   NS_ENSURE_STATE(mDocShell);

   NS_ENSURE_SUCCESS(mDocShellAsWin->SetTitle(aTitle), NS_ERROR_FAILURE);

   return NS_OK;
}

//*****************************************************************************
// nsWebBrowser::nsIScrollable
//*****************************************************************************

NS_IMETHODIMP nsWebBrowser::GetCurScrollPos(PRInt32 aScrollOrientation, 
   PRInt32* aCurPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->GetCurScrollPos(aScrollOrientation, aCurPos);
}

NS_IMETHODIMP nsWebBrowser::SetCurScrollPos(PRInt32 aScrollOrientation, 
   PRInt32 aCurPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetCurScrollPos(aScrollOrientation, aCurPos);
}

NS_IMETHODIMP nsWebBrowser::SetCurScrollPosEx(PRInt32 aCurHorizontalPos, 
   PRInt32 aCurVerticalPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetCurScrollPosEx(aCurHorizontalPos, 
      aCurVerticalPos);
}

NS_IMETHODIMP nsWebBrowser::GetScrollRange(PRInt32 aScrollOrientation,
   PRInt32* aMinPos, PRInt32* aMaxPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->GetScrollRange(aScrollOrientation, aMinPos,
      aMaxPos);
}

NS_IMETHODIMP nsWebBrowser::SetScrollRange(PRInt32 aScrollOrientation,
   PRInt32 aMinPos, PRInt32 aMaxPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetScrollRange(aScrollOrientation, aMinPos,
      aMaxPos);
}

NS_IMETHODIMP nsWebBrowser::SetScrollRangeEx(PRInt32 aMinHorizontalPos,
   PRInt32 aMaxHorizontalPos, PRInt32 aMinVerticalPos, PRInt32 aMaxVerticalPos)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetScrollRangeEx(aMinHorizontalPos,
      aMaxHorizontalPos, aMinVerticalPos, aMaxVerticalPos);
}

NS_IMETHODIMP nsWebBrowser::GetCurrentScrollbarPreferences(PRInt32 aScrollOrientation,
   PRInt32* aScrollbarPref)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->GetCurrentScrollbarPreferences(aScrollOrientation,
      aScrollbarPref);
}

NS_IMETHODIMP nsWebBrowser::GetDefaultScrollbarPreferences(PRInt32 aScrollOrientation,
   PRInt32* aScrollbarPref)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->GetDefaultScrollbarPreferences(aScrollOrientation,
      aScrollbarPref);
}

NS_IMETHODIMP nsWebBrowser::SetCurrentScrollbarPreferences(PRInt32 aScrollOrientation,
   PRInt32 aScrollbarPref)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetCurrentScrollbarPreferences(aScrollOrientation,
      aScrollbarPref);

}

NS_IMETHODIMP nsWebBrowser::SetDefaultScrollbarPreferences(PRInt32 aScrollOrientation,
   PRInt32 aScrollbarPref)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->SetDefaultScrollbarPreferences(aScrollOrientation,
      aScrollbarPref);
}

NS_IMETHODIMP nsWebBrowser::ResetScrollbarPreferences()
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->ResetScrollbarPreferences();
}

NS_IMETHODIMP nsWebBrowser::GetScrollbarVisibility(PRBool* aVerticalVisible,
   PRBool* aHorizontalVisible)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsScrollable->GetScrollbarVisibility(aVerticalVisible,
      aHorizontalVisible);
}

//*****************************************************************************
// nsWebBrowser::nsITextScroll
//*****************************************************************************   

NS_IMETHODIMP nsWebBrowser::ScrollByLines(PRInt32 aNumLines)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsTextScroll->ScrollByLines(aNumLines);
}

NS_IMETHODIMP nsWebBrowser::ScrollByPages(PRInt32 aNumPages)
{
   NS_ENSURE_STATE(mDocShell);

   return mDocShellAsTextScroll->ScrollByPages(aNumPages);
}


//*****************************************************************************
// nsWebBrowser: Listener Helpers
//*****************************************************************************   

NS_IMETHODIMP nsWebBrowser::SetDocShell(nsIDocShell* aDocShell)
{
     if(aDocShell)
     {
         NS_ENSURE_TRUE(!mDocShell, NS_ERROR_FAILURE);
 
         nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(aDocShell));
         nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(aDocShell));
         nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(aDocShell));
         nsCOMPtr<nsIWebNavigation> nav(do_QueryInterface(aDocShell));
         nsCOMPtr<nsIScrollable> scrollable(do_QueryInterface(aDocShell));
         nsCOMPtr<nsITextScroll> textScroll(do_QueryInterface(aDocShell));
         nsCOMPtr<nsIWebProgress> progress(do_GetInterface(aDocShell));
         NS_ENSURE_TRUE(req && baseWin && item && nav && scrollable && textScroll && progress,
             NS_ERROR_FAILURE);
 
         mDocShell = aDocShell;
         mDocShellAsReq = req;
         mDocShellAsWin = baseWin;
         mDocShellAsItem = item;
         mDocShellAsNav = nav;
         mDocShellAsScrollable = scrollable;
         mDocShellAsTextScroll = textScroll;
         mWebProgress = progress;
     }
     else
     {
         if (mDocShellTreeOwner)
           mDocShellTreeOwner->RemoveFromWatcher(); // evil twin of Add in Create()
         if (mDocShellAsWin)
           mDocShellAsWin->Destroy();

         mDocShell = nsnull;
         mDocShellAsReq = nsnull;
         mDocShellAsWin = nsnull;
         mDocShellAsItem = nsnull;
         mDocShellAsNav = nsnull;
         mDocShellAsScrollable = nsnull;
         mDocShellAsTextScroll = nsnull;
         mWebProgress = nsnull;
     }

     return NS_OK; 
}

NS_IMETHODIMP nsWebBrowser::EnsureDocShellTreeOwner()
{
   if(mDocShellTreeOwner)
      return NS_OK;

   mDocShellTreeOwner = new nsDocShellTreeOwner();
   NS_ENSURE_TRUE(mDocShellTreeOwner, NS_ERROR_OUT_OF_MEMORY);

   NS_ADDREF(mDocShellTreeOwner);
   mDocShellTreeOwner->WebBrowser(this);
   
   return NS_OK;
}

NS_IMETHODIMP nsWebBrowser::EnsureFindImpl()
{
   if (mFindImpl)
      return NS_OK;

   mFindImpl = new nsWebBrowserFindImpl;
   NS_ENSURE_TRUE(mFindImpl, NS_ERROR_OUT_OF_MEMORY);
   return mFindImpl->Init();
}

/* static */
nsEventStatus PR_CALLBACK nsWebBrowser::HandleEvent(nsGUIEvent *aEvent)
{
  nsEventStatus  result = nsEventStatus_eIgnore;
  nsWebBrowser  *browser = nsnull;
  void          *data = nsnull;

  if (!aEvent->widget)
    return result;

  aEvent->widget->GetClientData(data);
  if (!data)
    return result;

  browser = NS_STATIC_CAST(nsWebBrowser *, data);

  switch(aEvent->message) {

  case NS_ACTIVATE: {
    browser->Activate();
    break;
  }

  case NS_DEACTIVATE: {
    browser->Deactivate();
    break;
  }

  case NS_GOTFOCUS: {
    // try to set focus on the last focused window as stored in the
    // focus controller object.
    nsCOMPtr<nsIDOMWindow> domWindowExternal;
    browser->GetContentDOMWindow(getter_AddRefs(domWindowExternal));
    nsCOMPtr<nsIDOMWindowInternal> domWindow;
    domWindow = do_QueryInterface(domWindowExternal);
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWindow));
    nsCOMPtr<nsIFocusController> focusController;
    piWin->GetRootFocusController(getter_AddRefs(focusController));
    if (focusController) {
      nsCOMPtr<nsIDOMWindowInternal> focusedWindow;
      focusController->GetFocusedWindow(getter_AddRefs(focusedWindow));
      if (focusedWindow) {
        focusController->SetSuppressFocus(PR_TRUE);
        domWindow->Focus(); // This sets focus, but we'll ignore it.  
                           // A subsequent activate will cause us to stop suppressing.
        break;
      }
    }

    // If there wasn't a focus controller and focused window just set
    // focus on the primary content shell.  If that wasn't focused,
    // try and just set it on the toplevel DOM window.
    nsCOMPtr<nsIDOMWindowInternal> contentDomWindow;
    browser->GetPrimaryContentWindow(getter_AddRefs(contentDomWindow));
    if (contentDomWindow)
      contentDomWindow->Focus();
    else if (domWindow)
      domWindow->Focus();
    break;
  }

  default:
    break;
  }

  return result;
    
  
}

NS_IMETHODIMP nsWebBrowser::GetPrimaryContentWindow(nsIDOMWindowInternal **aDOMWindow)
{
  *aDOMWindow = 0;

  nsCOMPtr<nsIDocShellTreeItem> item;
  mDocShellTreeOwner->GetPrimaryContentShell(getter_AddRefs(item));
  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell;
  docShell = do_QueryInterface(item);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDOMWindowInternal> domWindow;
  domWindow = do_GetInterface(docShell);
  NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);

  *aDOMWindow = domWindow;
  NS_ADDREF(*aDOMWindow);
  return NS_OK;
  
}
//*****************************************************************************
// nsWebBrowser::nsIWebBrowserFocus
//*****************************************************************************   

/* void activate (); */
NS_IMETHODIMP nsWebBrowser::Activate(void)
{
    // Make sure we can get the to a valid nsIPresShell
    // Without this check we assert when the embeddor calls
    // us and the presShell is not yet created
    // For ex, under MSFT Windows embeddors can trap
    // the WM_ACTIVATE message in response to which
    // can call nsIWebBrowser->Activate(). During the initial
    // window creation the presShell will not be created when
    // the call to the initial Activate() is made

    NS_ENSURE_STATE(mDocShell);
    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    if(!presShell)
        return NS_OK;

    nsCOMPtr<nsIDOMWindow> domWindow;
    GetContentDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
        nsCOMPtr<nsPIDOMWindow> privateDOMWindow = do_QueryInterface(domWindow);
        if(privateDOMWindow)
          privateDOMWindow->Activate();
    }

    return NS_OK;
}

/* void deactivate (); */
NS_IMETHODIMP nsWebBrowser::Deactivate(void)
{
    NS_ENSURE_STATE(mDocShell);
    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    if(!presShell)
        return NS_OK;

    nsCOMPtr<nsIDOMWindow> domWindow;
    GetContentDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) {
        nsCOMPtr<nsPIDOMWindow> privateDOMWindow = do_QueryInterface(domWindow);
        if(privateDOMWindow)
          privateDOMWindow->Deactivate();
    }

    return NS_OK;
}

/* void setFocusAtFirstElement (); */
NS_IMETHODIMP nsWebBrowser::SetFocusAtFirstElement(void)
{
  return NS_OK;
}

/* void setFocusAtLastElement (); */
NS_IMETHODIMP nsWebBrowser::SetFocusAtLastElement(void)
{
  return NS_OK;
}

/* attribute nsIDOMWindow focusedWindow; */
NS_IMETHODIMP nsWebBrowser::GetFocusedWindow(nsIDOMWindow * *aFocusedWindow)
{
    NS_ENSURE_ARG_POINTER(aFocusedWindow);
    *aFocusedWindow = nsnull;

    nsresult rv;
    nsCOMPtr<nsIDOMWindowInternal> focusedWindow;

    nsCOMPtr<nsIDOMWindow> domWindowExternal;
    rv = GetContentDOMWindow(getter_AddRefs(domWindowExternal));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(domWindowExternal /*domWindow*/, &rv));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFocusController> focusController;
    piWin->GetRootFocusController(getter_AddRefs(focusController));
    if (focusController)
      rv = focusController->GetFocusedWindow(getter_AddRefs(focusedWindow));
    
    *aFocusedWindow = focusedWindow;
    NS_IF_ADDREF(*aFocusedWindow);
    
    return *aFocusedWindow ? NS_OK : NS_ERROR_FAILURE;
}
NS_IMETHODIMP nsWebBrowser::SetFocusedWindow(nsIDOMWindow * aFocusedWindow)
{
  return NS_OK;
}

/* attribute nsIDOMElement focusedElement; */
NS_IMETHODIMP nsWebBrowser::GetFocusedElement(nsIDOMElement * *aFocusedElement)
{
  *aFocusedElement = nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsWebBrowser::SetFocusedElement(nsIDOMElement * aFocusedElement)
{
  return NS_OK;
}

//------------------------------------------------------
/* void Print (in nsIDOMWindow aDOMWindow, in nsIPrintOptions aThePrintOptions); */
NS_IMETHODIMP nsWebBrowser::Print(nsIDOMWindow *aDOMWindow, 
                                  nsIPrintOptions *aThePrintOptions,
                                  nsIPrintListener *aPrintListener)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMWindow> thisDOMWin;
  // XXX this next line may need to be changed
  // it is unclear what the correct way is to get the document.
  GetContentDOMWindow(getter_AddRefs(thisDOMWin));
  if (aDOMWindow == thisDOMWin.get()) {
     nsCOMPtr<nsIContentViewer> contentViewer;
     mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
     if (contentViewer) {
       nsCOMPtr<nsIContentViewerFile> contentViewerFile(do_QueryInterface(contentViewer));
       if (contentViewerFile) {
         rv = contentViewerFile->Print(PR_FALSE, nsnull, aPrintListener);
       }
     }
  }
  return rv;
}

  /* void Cancel (); */
NS_IMETHODIMP nsWebBrowser::Cancel(void)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIPrintOptions, printService, kPrintOptionsCID, &rv);
  if (NS_SUCCEEDED(rv) && printService) {
    return printService->SetIsCancelled(PR_TRUE);
  }
  return NS_OK;
}
