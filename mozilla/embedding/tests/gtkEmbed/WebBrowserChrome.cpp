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
 */

// Local Includes

#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsIDocShellTreeItem.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsIDOMWindow.h"
#include "nsCWebBrowser.h"
#include "nsWidgetsCID.h"
#include "nsIWebBrowserSetup.h"
#include "gtkEmbed.h"
#include "WebBrowserChrome.h"

nsVoidArray WebBrowserChrome::sBrowserList;

WebBrowserChrome::WebBrowserChrome()
{
	NS_INIT_REFCNT();
    mNativeWindow = nsnull;
}

WebBrowserChrome::~WebBrowserChrome()
{
}

//*****************************************************************************
// WebBrowserChrome::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(WebBrowserChrome)
NS_IMPL_RELEASE(WebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(WebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)  //optional
//   NS_INTERFACE_MAP_ENTRY(nsIPrompt)
NS_INTERFACE_MAP_END

//*****************************************************************************
// WebBrowserChrome::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
  *aInstancePtr = 0;
  if (aIID.Equals(NS_GET_IID(nsIDOMWindow))) {
    if (mWebBrowser)
      return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
    return NS_ERROR_NOT_INITIALIZED;
  }
  return QueryInterface(aIID, aInstancePtr);
}

//*****************************************************************************
// WebBrowserChrome::nsIWebBrowserChrome
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);
   NS_ENSURE_TRUE(mWebBrowser, NS_ERROR_NOT_INITIALIZED);
   *aWebBrowser = mWebBrowser;
   NS_IF_ADDREF(*aWebBrowser);

   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ENSURE_ARG(aWebBrowser);   // Passing nsnull is NOT OK
   NS_ENSURE_TRUE(mWebBrowser, NS_ERROR_NOT_INITIALIZED);
   NS_ERROR("Who be calling me");
   mWebBrowser = aWebBrowser;
   return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}


NS_IMETHODIMP WebBrowserChrome::CreateBrowser(PRInt32 aX, PRInt32 aY,
                                              PRInt32 aCX, PRInt32 aCY,
                                              nsIWebBrowser **aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);
   *aWebBrowser = nsnull;

    mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);

    if (!mWebBrowser)
        return NS_ERROR_FAILURE;

    mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, this));

    nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser);
    dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

    
    mBaseWindow = do_QueryInterface(mWebBrowser);
    mNativeWindow = ::CreateNativeWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, this));

    if (!mNativeWindow)
        return NS_ERROR_FAILURE;

    mBaseWindow->InitWindow( mNativeWindow,
                             nsnull, 
                             0, 0, 450, 450);
    mBaseWindow->Create();
    
    // Configure what the web browser can and cannot do
    nsCOMPtr<nsIWebBrowserSetup> webBrowserAsSetup(do_QueryInterface(mWebBrowser));
    webBrowserAsSetup->SetProperty(nsIWebBrowserSetup::SETUP_ALLOW_PLUGINS, PR_FALSE);

    *aWebBrowser = mWebBrowser;
    NS_ADDREF(*aWebBrowser);
    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::CreateBrowserWindow(PRUint32 chromeMask,
    PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, nsIWebBrowser **aWebBrowser)
{
  NS_ENSURE_ARG_POINTER(aWebBrowser);
  *aWebBrowser = nsnull;

  nsresult rv;
  nsCOMPtr<nsIWebBrowserChrome> parent;
  nsCOMPtr<nsIWebBrowserChrome> newChrome;

  if (chromeMask & nsIWebBrowserChrome::CHROME_DEPENDENT)
    parent = dont_QueryInterface(NS_STATIC_CAST(nsIWebBrowserChrome*, this));
  rv = ::CreateBrowserWindow(chromeMask, parent, getter_AddRefs(newChrome));

  if (NS_SUCCEEDED(rv) && newChrome)
    newChrome->GetWebBrowser(aWebBrowser);
  return rv;
}


NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebBrowserChrome::ShowAsModal(void)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


//*****************************************************************************
// WebBrowserChrome::nsIWebProgressListener
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                                  PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                                  PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                               PRInt32 progressStateFlags, PRUint32 status)
{

    if ((progressStateFlags & STATE_STOP) && (progressStateFlags & STATE_IS_REQUEST))
    {
    }
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                                 nsIRequest* aRequest,
                                                 nsIURI *location)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
WebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
    return NS_OK;
}



NS_IMETHODIMP 
WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRInt32 state)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


//*****************************************************************************
// WebBrowserChrome::nsIWebBrowserSiteWindow
//*****************************************************************************   

NS_IMETHODIMP WebBrowserChrome::Destroy()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP WebBrowserChrome::SetPosition(PRInt32 x, PRInt32 y)
{
    return mBaseWindow->SetPosition(x, y);
}

NS_IMETHODIMP WebBrowserChrome::GetPosition(PRInt32* x, PRInt32* y)
{
    return mBaseWindow->GetPosition(x, y);
}

NS_IMETHODIMP WebBrowserChrome::SetSize(PRInt32 cx, PRInt32 cy, PRBool fRepaint)
{
    return mBaseWindow->SetSize(cx, cy, fRepaint);
}

NS_IMETHODIMP WebBrowserChrome::GetSize(PRInt32* cx, PRInt32* cy)
{
    return mBaseWindow->GetSize(cx, cy);
}

NS_IMETHODIMP WebBrowserChrome::SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy, PRBool fRepaint)
{
    return mBaseWindow->SetPositionAndSize(x, y, cx, cy, fRepaint);
}

NS_IMETHODIMP WebBrowserChrome::GetPositionAndSize(PRInt32* x, PRInt32* y, PRInt32* cx, PRInt32* cy)
{
    return mBaseWindow->GetPositionAndSize(x, y, cx, cy);
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
   return mBaseWindow->SetFocus();
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);

   *aTitle = nsnull;
   
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar* aTitle)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void ** aSiteWindow)
{
   NS_ENSURE_ARG_POINTER(aSiteWindow);
   *aSiteWindow = mNativeWindow;
   return NS_OK;
}

