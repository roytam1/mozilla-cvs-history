/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chak Nanga <chak@netscape.com>
 *   David Epstein <depstein@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// File Overview....
// This is the class which implements all the interfaces
// required(and optional) by the mozilla embeddable browser engine
//
// Note that this obj gets passed in the IBrowserFrameGlue* using the 
// Init() method. Many of the interface implementations use this
// to get the actual task done. Ex: to update the status bar
//
// Look at the INTERFACE_MAP_ENTRY's below for the list of 
// the currently implemented interfaces
//
// This file currently has the implementation for all the interfaces
// which are required of an app embedding Gecko
// Implementation of other optional interfaces are in separate files
// to avoid cluttering this one and to demonstrate other embedding
// principles. For example, nsIPrompt is implemented in a separate DLL.
// 
//	nsIWebBrowserChrome	- This is a required interface to be implemented
//						  by embeddors
//
//	nsIEmbeddingSiteWindow - This is a required interface to be implemented
//						 by embedders			
//					   - SetTitle() gets called after a document
//						 load giving us the chance to update our
//						 titlebar
//
// Some points to note...
//
// nsIWebBrowserChrome interface's SetStatus() gets called when a user 
// mouses over the links on a page
//
// nsIWebProgressListener interface's OnStatusChange() gets called
// to indicate progress when a page is being loaded
//
// Next suggested file(s) to look at : 
//			Any of the BrowserImpl*.cpp files for other interface
//			implementation details
//

#ifdef _WINDOWS
  #include "stdafx.h"
#endif

#include "nsIDOMWindow.h"
#include "BrowserImpl.h"

#include "QaUtils.h"

#include "nsirequest.h"
#include "Tests.h"
#include "prmem.h"

CBrowserImpl::CBrowserImpl()
{
  NS_INIT_ISUPPORTS();

  m_pBrowserFrameGlue = NULL;
  mWebBrowser = nsnull;
}


CBrowserImpl::~CBrowserImpl()
{
}

// It's very important that the creator of this CBrowserImpl object
// Call on this Init() method with proper values after creation
//
NS_METHOD CBrowserImpl::Init(PBROWSERFRAMEGLUE pBrowserFrameGlue, 
                             nsIWebBrowser* aWebBrowser)
{
	  m_pBrowserFrameGlue = pBrowserFrameGlue;
	  
	  SetWebBrowser(aWebBrowser);

	  return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsISupports
//*****************************************************************************   

NS_IMPL_ADDREF(CBrowserImpl)
NS_IMPL_RELEASE(CBrowserImpl)

NS_INTERFACE_MAP_BEGIN(CBrowserImpl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY(nsISHistoryListener) // de: added 5/11/01
   NS_INTERFACE_MAP_ENTRY(nsIStreamListener) // de: added 6/29/01
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver) // de: added 6/29/01
   NS_INTERFACE_MAP_ENTRY(nsITooltipListener) // de: added 7/25/01
   NS_INTERFACE_MAP_ENTRY(nsIURIContentListener) // de: added 8/8/02
//   NS_INTERFACE_MAP_ENTRY(nsITooltipTextProvider) // de: added 7/26/01
NS_INTERFACE_MAP_END

//*****************************************************************************
// CBrowserImpl::nsIInterfaceRequestor
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
	if(aIID.Equals(NS_GET_IID(nsIDOMWindow)))
	{
		if (mWebBrowser)
			return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
		return NS_ERROR_NOT_INITIALIZED;
	}

	return QueryInterface(aIID, aInstancePtr);
}

//*****************************************************************************
// CBrowserImpl::nsIWebBrowserChrome
//*****************************************************************************   

//Gets called when you mouseover links etc. in a web page
//
NS_IMETHODIMP CBrowserImpl::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
//  QAOutput("nsIWebBrowserChrome::SetStatus().", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->UpdateStatusBarText(aStatus);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   QAOutput("nsIWebBrowserChrome::GetWebBrowser().", 1);

   NS_ENSURE_ARG_POINTER(aWebBrowser);

   *aWebBrowser = mWebBrowser;
   if (!aWebBrowser)
      QAOutput("aWebBrowser is null", 1);

   NS_IF_ADDREF(*aWebBrowser);

   return NS_OK;
}

// Currently called from Init(). I'm not sure who else
// calls this
//
NS_IMETHODIMP CBrowserImpl::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   QAOutput("nsIWebBrowserChrome::SetWebBrowser().", 1);

   NS_ENSURE_ARG_POINTER(aWebBrowser);

   if (!aWebBrowser)
      QAOutput("aWebBrowser is null", 1);

   mWebBrowser = aWebBrowser;

   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetChromeFlags(PRUint32* aChromeMask)
{
    QAOutput("nsIWebBrowserChrome::GetChromeFlags().", 1);

	*aChromeMask = nsIWebBrowserChrome::CHROME_ALL;
    if (!aChromeMask)
      QAOutput("aChromeMask is null", 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetChromeFlags(PRUint32 aChromeMask)
{
    QAOutput("nsIWebBrowserChrome::SetChromeFlags().", 1);

    if (!aChromeMask)
      QAOutput("aChromeMask is null", 1);

	mChromeMask = aChromeMask;

	return NS_OK;
}

// Gets called in response to create a new browser window. 
// Ex: In response to a JavaScript Window.Open() call
//
//
/*NS_IMETHODIMP CBrowserImpl::CreateBrowserWindow(PRUint32 chromeMask, PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, nsIWebBrowser **aWebBrowser)
{
	if(! m_pBrowserFrameGlue)
	{
		QAOutput("nsIWebBrowserChrome::CreateBrowserWindow(). Browser Window not created.", 1);
		return NS_ERROR_FAILURE;
	}

	if(m_pBrowserFrameGlue->CreateNewBrowserFrame(chromeMask, 
								aX, aY, aCX, aCY, aWebBrowser))
	{
		QAOutput("nsIWebBrowserChrome::CreateBrowserWindow(): Browser Window created.", 1);
		return NS_OK;
	}
	else
	{
		QAOutput("nsIWebBrowserChrome::CreateBrowserWindow(): Browser Window not created.", 1);
	    return NS_ERROR_FAILURE;
	}
}
*/

// Will get called in response to JavaScript window.close()
//
NS_IMETHODIMP CBrowserImpl::DestroyBrowserWindow()
{
	if(! m_pBrowserFrameGlue)
	{
		QAOutput("nsIWebBrowserChrome::DestroyBrowserWindow(): Browser Window not destroyed.", 1);
		return NS_ERROR_FAILURE;
	}

	m_pBrowserFrameGlue->DestroyBrowserFrame();
	QAOutput("nsIWebBrowserChrome::DestroyBrowserWindow(): Browser Window destroyed.", 1);

	return NS_OK;
}

// Gets called in response to set the size of a window
// Ex: In response to a JavaScript Window.Open() call of
// the form 
//
//		window.open("http://www.mozilla.org", "theWin", "width=200, height=400");
//
// First the CreateBrowserWindow() call will be made followed by a 
// call to this method to resize the window
//
NS_IMETHODIMP CBrowserImpl::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
	QAOutput("nsIWebBrowserChrome::SizeBrowserTo(): Browser sized.", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetBrowserFrameSize(aCX, aCY);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ShowAsModal(void)
{
	QAOutput("inside nsIWebBrowserChrome::ShowAsModal()", 2);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsWindowModal(PRBool *retval)
{
  QAOutput("inside nsIWebBrowserChrome::IsWindowModal()", 1);

  // We're not modal
  *retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ExitModalEventLoop(nsresult aStatus)
{
  QAOutput("inside nsIWebBrowserChrome::ExitModalEventLoop()", 1);

  return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIWebBrowserChromeFocus
//*****************************************************************************

NS_IMETHODIMP CBrowserImpl::FocusNextElement()
{
	QAOutput("inside nsIWebBrowserChromeFocus::FocusNextElement()", 1);

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::FocusPrevElement()
{
	QAOutput("inside nsIWebBrowserChromeFocus::FocusPrevElement()", 1);

    return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIEmbeddingSiteWindow
//*****************************************************************************   

NS_IMETHODIMP CBrowserImpl::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
	QAOutput("inside nsIEmbeddingSiteWindow::SetDimensions()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION &&
        (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
         aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
    {
    	m_pBrowserFrameGlue->SetBrowserFramePositionAndSize(x, y, cx, cy, PR_TRUE);
    }
    else
    {
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
        {
    	    m_pBrowserFrameGlue->SetBrowserFramePosition(x, y);
        }
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
            aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
        {
    	    m_pBrowserFrameGlue->SetBrowserFrameSize(cx, cy);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
	QAOutput("inside nsIEmbeddingSiteWindow::GetDimensions()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;
    
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
    	m_pBrowserFrameGlue->GetBrowserFramePosition(x, y);
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
        aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
    	m_pBrowserFrameGlue->GetBrowserFrameSize(cx, cy);
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetSiteWindow(void** aSiteWindow)
{
  QAOutput("inside nsIEmbeddingSiteWindow::GetSiteWindow()", 1);

  if (!aSiteWindow)
    return NS_ERROR_NULL_POINTER;

  *aSiteWindow = 0;
  if (m_pBrowserFrameGlue) {
    HWND w = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
    *aSiteWindow = NS_REINTERPRET_CAST(void *, w);
  }
  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetFocus()
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetFocus()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetFocus();

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetTitle(PRUnichar** aTitle)
{
    QAOutput("inside nsIEmbeddingSiteWindow::GetTitle()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->GetBrowserFrameTitle(aTitle);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetTitle(const PRUnichar* aTitle)
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetTitle()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetBrowserFrameTitle(aTitle);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetVisibility(PRBool *aVisibility)
{
    QAOutput("inside nsIEmbeddingSiteWindow::GetVisibility()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->GetBrowserFrameVisibility(aVisibility);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetVisibility(PRBool aVisibility)
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetVisibility()", 1);

    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->ShowBrowserFrame(aVisibility);

    return NS_OK;
}

//*****************************************************************************
// CBrowserImpl::nsIStreamListener (used for nsIRequest & UriContentListener)

NS_IMETHODIMP CBrowserImpl::OnDataAvailable(nsIRequest *request,
				nsISupports *ctxt, nsIInputStream *input,
				PRUint32 offset, PRUint32 count)
{
	nsCString stringMsg;
	PRUint32 readLen;

	QAOutput("##### inside nsIStreamListener::OnDataAvailable(). #####");

	RequestName(request, stringMsg, 1);
	readLen = count;
		// from prmem.h: PR_Malloc()
	char *buf = (char *)PR_Malloc(count);
	if (!input)
		QAOutput("We didn't get the nsIInputStream object.", 1);
	else {
		// consumer of input stream
		rv = input->Read(buf, count, &readLen);
		RvTestResult(rv, "nsIInputStream->Read() consumer", 1);
	}

	FormatAndPrintOutput("OnDataAvailable() offset = ", offset, 1);
	FormatAndPrintOutput("OnDataAvailable() count = ", count, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnStartRequest(nsIRequest *request,
				nsISupports *ctxt)
{
	nsCString stringMsg;

	QAOutput("##### BEGIN: nsIStreamListener::OnStartRequest() #####");
	RequestName(request, stringMsg, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnStopRequest(nsIRequest *request,
				nsISupports *ctxt, nsresult rv)
{
	nsCString stringMsg;

	RvTestResult(rv, "nsIStreamListener::OnStopRequest rv input", 1);
	RequestName(request, stringMsg, 1);
	QAOutput("##### END: nsIStreamListener::OnStopRequest() #####");
		
	return NS_OK;
}

//*****************************************************************************   
//  Tool Tip Listener

NS_IMETHODIMP CBrowserImpl::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords,
										  const PRUnichar *aTipText)
{
	QAOutput("nsITooltipListener->OnShowTooltip()",1);
	FormatAndPrintOutput("OnShowTooltip() aXCoords = ", aXCoords, 1);
	FormatAndPrintOutput("OnShowTooltip() aYCoords = ", aYCoords, 1);
	FormatAndPrintOutput("OnShowTooltip() aTipText = ", *aTipText, 1);	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHideTooltip() 
{
	QAOutput("nsITooltipListener->OnHideTooltip()",1);
	return NS_OK;
}


//*****************************************************************************   
//  UriContentListener

NS_IMETHODIMP CBrowserImpl::OnStartURIOpen(nsIURI *aURI, PRBool *_retval)
{
	QAOutput("nsIURIContentListener->OnStartURIOpen()",1);

	GetTheUri(aURI, 1);
	// set return boolean to false so uriOpen doesn't abort
	*_retval = PR_FALSE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::DoContent(const char *aContentType, PRBool aIsContentPreferred, nsIRequest *aRequest, nsIStreamListener **aContentHandler, PRBool *_retval)
{
	nsCString stringMsg;

	QAOutput("nsIURIContentListener->DoContent()",1);

	FormatAndPrintOutput("DoContent() content type = ", *aContentType, 1);
	FormatAndPrintOutput("DoContent() aIsContentPreferred = ", aIsContentPreferred, 1);
	RequestName(aRequest, stringMsg);	// nsIRequest::GetName() test
//	*aContentHandler = nsnull;
	QueryInterface(NS_GET_IID(nsIStreamListener), (void **) aContentHandler);

	*_retval = PR_FALSE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsPreferred(const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
	nsCAutoString contentStr;

	QAOutput("nsIURIContentListener->IsPreferred()",1);

	FormatAndPrintOutput("IsPreferred() content type = ", *aContentType, 1);
	*aDesiredContentType = nsCRT::strdup("text/html");
	FormatAndPrintOutput("aDesiredContentType set to = ", *aDesiredContentType, 1);
	*_retval = PR_TRUE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::CanHandleContent(const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType, PRBool *_retval)
{
	QAOutput("nsIURIContentListener->CanHandleContent()",1);

	FormatAndPrintOutput("CanHandleContent() content type = ", *aContentType, 1);
	FormatAndPrintOutput("CanHandleContent() preferred content type = ", aIsContentPreferred, 1);
	*aDesiredContentType = nsCRT::strdup("text/html");
	FormatAndPrintOutput("aDesiredContentType set to = ", *aDesiredContentType, 1);
	*_retval = PR_TRUE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetLoadCookie(nsISupports **aLoadCookie)
{
	QAOutput("nsIURIContentListener->GetLoadCookie()",1);

	if (!aLoadCookie)
		QAOutput("aLoadCookie object is null",1);
	*aLoadCookie = mLoadCookie;
	NS_IF_ADDREF(*aLoadCookie);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetLoadCookie(nsISupports * aLoadCookie)
{
	QAOutput("nsIURIContentListener->SetLoadCookie()",1);

	if (!aLoadCookie)
		QAOutput("aLoadCookie object is null",1);
	mLoadCookie = aLoadCookie;

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetParentContentListener(nsIURIContentListener **aParentContentListener)
{
	QAOutput("nsIURIContentListener->GetParentContentListener()",1);

	if (!aParentContentListener)
		QAOutput("aParentContentListener object is null",1);
	*aParentContentListener = mParentContentListener;
	NS_IF_ADDREF(*aParentContentListener);

//	QueryInterface(NS_GET_IID(nsIURIContentListener), (void **) aParentContentListener);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
	QAOutput("nsIURIContentListener->SetParentContentListener()",1);

	if (!aParentContentListener)
		QAOutput("aParentContentListener object is null",1);
	mParentContentListener = aParentContentListener;

	return NS_OK;	
}
