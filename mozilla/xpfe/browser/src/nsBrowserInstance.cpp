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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsBrowserInstance.h"
// Use this trick temporarily, to minimize delta to nsBrowserAppCore.cpp.
#define nsBrowserAppCore nsBrowserInstance

#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIClipboardCommands.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"

#include "nsIFileSpecWithUI.h"

#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"

#include "nsIScriptGlobalObject.h"
#include "nsIWebShell.h"
#include "nsIWebShellWindow.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#include "nsIWidget.h"
#include "plevent.h"

#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"

#include "nsIDocumentViewer.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsICmdLineService.h"
#include "nsIGlobalHistory.h"

#include "nsIDOMXULDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"


#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsFileSpec.h"  // needed for nsAutoCString

#include "nsIDocumentLoader.h"
#include "nsIObserverService.h"
#include "nsFileLocations.h"

#include "nsIFileLocator.h"
#include "nsIFileSpec.h"
#include "nsIWalletService.h"

#include "nsCURILoader.h"

static NS_DEFINE_IID(kIWalletServiceIID, NS_IWALLETSERVICE_IID);
static NS_DEFINE_IID(kWalletServiceCID, NS_WALLETSERVICE_CID);

// Interface for "unknown content type handler" component/service.
#include "nsIUnkContentTypeHandler.h"

// Stuff to implement file download dialog.
#include "nsIXULWindowCallbacks.h"
#include "nsIDocumentObserver.h"
#include "nsIContent.h"
#include "nsIContentViewerFile.h"
#include "nsINameSpaceManager.h"
#include "nsFileStream.h"

// Stuff to implement find/findnext
#include "nsIFindComponent.h"
#ifdef DEBUG_warren
#include "prlog.h"
#if defined(DEBUG) || defined(FORCE_PR_LOG)
static PRLogModuleInfo* gTimerLog = nsnull;
#endif /* DEBUG || FORCE_PR_LOG */
#endif

static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);


/* Define Class IDs */
static NS_DEFINE_IID(kAppShellServiceCID,        NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kCmdLineServiceCID,    NS_COMMANDLINE_SERVICE_CID);
static NS_DEFINE_IID(kCGlobalHistoryCID,       NS_GLOBALHISTORY_CID);
static NS_DEFINE_IID(kCSessionHistoryCID,       NS_SESSIONHISTORY_CID);

/* Define Interface IDs */
static NS_DEFINE_IID(kIAppShellServiceIID,       NS_IAPPSHELL_SERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,              NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDOMDocumentIID,           nsIDOMDocument::GetIID());
static NS_DEFINE_IID(kIDocumentIID,              nsIDocument::GetIID());
static NS_DEFINE_IID(kIStreamObserverIID,        NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kIWebShellWindowIID,        NS_IWEBSHELL_WINDOW_IID);
static NS_DEFINE_IID(kIGlobalHistoryIID,       NS_IGLOBALHISTORY_IID);
static NS_DEFINE_IID(kIWebShellIID,              NS_IWEB_SHELL_IID);

#ifdef DEBUG                                                           
static int APP_DEBUG = 0; // Set to 1 in debugger to turn on debugging.
#else                                                                  
#define APP_DEBUG 0                                                    
#endif                                                                 

#define FILE_PROTOCOL "file://"

static nsresult
FindNamedXULElement(nsIWebShell * aShell, const char *aId, nsCOMPtr<nsIDOMElement> * aResult );


static nsresult setAttribute( nsIWebShell *shell,
                              const char *id,
                              const char *name,
                              const nsString &value );
/////////////////////////////////////////////////////////////////////////
// nsBrowserAppCore
/////////////////////////////////////////////////////////////////////////

nsBrowserAppCore::nsBrowserAppCore()
{
  mContentWindow        = nsnull;
  mContentScriptContext = nsnull;
  mWebShellWin          = nsnull;
  mWebShell             = nsnull;
  mContentAreaWebShell  = nsnull;
  mContentAreaDocLoader = nsnull;
  mSHistory             = nsnull;
  mIsViewSource         = PR_FALSE;
  mIsLoadingHistory     = PR_FALSE;
  NS_INIT_REFCNT();
}

nsBrowserAppCore::~nsBrowserAppCore()
{
  NS_IF_RELEASE(mSHistory);
}

NS_IMPL_ADDREF(nsBrowserAppCore)
NS_IMPL_RELEASE(nsBrowserAppCore)


NS_IMPL_QUERY_HEAD(nsBrowserAppCore)
   NS_IMPL_QUERY_BODY(nsIBrowserInstance)
   NS_IMPL_QUERY_BODY(nsIDocumentLoaderObserver)
   NS_IMPL_QUERY_BODY(nsIObserver)
   NS_IMPL_QUERY_BODY(nsIURIContentListener)
NS_IMPL_QUERY_TAIL(nsIBrowserInstance)


static
nsIPresShell*
GetPresShellFor(nsIWebShell* aWebShell)
{
  nsIPresShell* shell = nsnull;
  if (nsnull != aWebShell) {
    nsIContentViewer* cv = nsnull;
    aWebShell->GetContentViewer(&cv);
    if (nsnull != cv) {
      nsIDocumentViewer* docv = nsnull;
      cv->QueryInterface(kIDocumentViewerIID, (void**) &docv);
      if (nsnull != docv) {
        nsIPresContext* cx;
        docv->GetPresContext(cx);
	      if (nsnull != cx) {
	        cx->GetShell(&shell);
	        NS_RELEASE(cx);
	      }
        NS_RELEASE(docv);
      }
      NS_RELEASE(cv);
    }
  }
  return shell;
}

NS_IMETHODIMP    
nsBrowserAppCore::Init()
{
  nsresult rv = NS_OK;

  // Create session history.
  
  rv = nsComponentManager::CreateInstance(kCSessionHistoryCID,
                                          nsnull,
                                          nsISessionHistory::GetIID(),
                                          (void **)&mSHistory );

  if ( NS_SUCCEEDED( rv ) ) {
#ifdef DEBUG_radha
	  printf("Successfully created instance of session history\n");
#endif
      // Add this object of observer of various events.
      BeginObserving(); 
  }

  // register ourselves as a content listener with the uri dispatcher service
  rv = NS_OK;
  NS_WITH_SERVICE(nsIURILoader, dispatcher, NS_URI_LOADER_PROGID, &rv);
  if (NS_SUCCEEDED(rv)) 
    rv = dispatcher->RegisterContentListener(this);
   
  return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::SetDocumentCharset(const PRUnichar *aCharset)
{
  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(mContentWindow) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (webShell) 
  {
    nsCOMPtr<nsIContentViewer> childCV;
    NS_ENSURE_SUCCESS(webShell->GetContentViewer(getter_AddRefs(childCV)), NS_ERROR_FAILURE);
    if (childCV) 
    {
      nsCOMPtr<nsIMarkupDocumentViewer> markupCV = do_QueryInterface(childCV);
      if (markupCV) {
        NS_ENSURE_SUCCESS(markupCV->SetDefaultCharacterSet(aCharset), NS_ERROR_FAILURE);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Back()
{
  GoBack(mContentAreaWebShell);
	return NS_OK;
}

NS_IMETHODIMP 
nsBrowserAppCore::GetSessionHistory(nsISessionHistory ** aResult)
{
	if (!aResult)
		return NS_ERROR_NULL_POINTER;

	if (mSHistory) {
	   NS_ADDREF(mSHistory);
	   *aResult = mSHistory;
	}
	else
		return NS_ERROR_NO_INTERFACE;
	return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::Reload(nsLoadFlags flags)
{
	if (mContentAreaWebShell)
	   Reload(mContentAreaWebShell, flags);
	return NS_OK;
}   

NS_IMETHODIMP
nsBrowserAppCore::Forward()
{
  GoForward(mContentAreaWebShell);
	return NS_OK;
}



NS_IMETHODIMP    
nsBrowserAppCore::Stop()
{
  mContentAreaWebShell->Stop();

  if (mIsLoadingHistory) {
	  SetLoadingFlag(PR_FALSE);
  }
  nsAutoString v( "false" );
  // XXX: The throbber should be turned off when the OnStopDocumentLoad 
  //      notification is received 
  setAttribute( mWebShell, "Browser:Throbber", "busy", v );
  return NS_OK;
}



nsresult ProfileDirectory(nsFileSpec& dirSpec) {
  nsIFileSpec* spec = NS_LocateFileOrDirectory(
  						nsSpecialFileSpec::App_UserProfileDirectory50);
  if (!spec)
  	return NS_ERROR_FAILURE;
  return spec->GetFileSpec(&dirSpec);
}



NS_IMETHODIMP
nsBrowserAppCore::GotoHistoryIndex(PRInt32 aIndex)
{
    Goto(aIndex, mContentAreaWebShell, PR_FALSE);
	return NS_OK;

}

NS_IMETHODIMP
nsBrowserAppCore::BackButtonPopup()
{
	if (!mSHistory)  {
		printf("nsBrowserAppCore::BackButtonPopup Couldn't get a handle to SessionHistory\n");
		return NS_ERROR_FAILURE;
	}

 // Get handle to the "backbuttonpopup" element
  nsCOMPtr<nsIDOMElement>   backPopupElement;
  nsresult rv = FindNamedXULElement(mWebShell, "backbuttonpopup", &backPopupElement);

  if (!NS_SUCCEEDED(rv) ||  !backPopupElement)
  {
     return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> backPopupNode(do_QueryInterface(backPopupElement)); 
  if (!backPopupNode) {
    return NS_ERROR_FAILURE;
  }

  //get handle to the Menu item under popup
  nsCOMPtr<nsIDOMNode>   menu;
  backPopupNode->GetFirstChild(getter_AddRefs(menu));
  if (!menu) {
	 printf("nsBrowserAppCore::BackButtonPopup Call to GetFirstChild failed\n");
     return NS_ERROR_FAILURE;
  }
  
  PRBool hasChildren=PR_FALSE;

  // Check if menu has children. If so, remove them.
  rv = menu->HasChildNodes(&hasChildren);
  if (NS_SUCCEEDED(rv) && hasChildren) {
	  rv = ClearHistoryPopup(menu);
	  if (!NS_SUCCEEDED(rv))
		  printf("nsBrowserAppCore::BackButtonPopup ERROR While removing old history menu items\n");
  }    // hasChildren
  else {
	 if (APP_DEBUG) printf("nsBrowserAppCore::BackButtonPopup Menu has no children\n");
  }             

  PRInt32 indix=0, i=0;
  //Get current index in Session History. We have already verified 
  // if mSHistory is null
  mSHistory->GetCurrentIndex(&indix);

  //Decide on the # of items in the popup list 
  if (indix > SHISTORY_POPUP_LIST)
     i  = indix-SHISTORY_POPUP_LIST;

  for (PRInt32 j=indix-1;j>=i;j--) {
      PRUnichar *title=nsnull;
	  char * url=nsnull;
	  
        mSHistory->GetURLForIndex(j, &url);
        nsAutoString  histURL(url);
        mSHistory->GetTitleForIndex(j, &title);
        nsAutoString  histTitle(title);
        rv = CreateMenuItem(menu, j, title);
	    if (!NS_SUCCEEDED(rv)) 
		  printf("nsBrowserAppCore:;BackButtonpopup ERROR while creating menu item\n");
		Recycle(title);
		Recycle(url);
     } 

  return NS_OK;

}



NS_IMETHODIMP nsBrowserAppCore::CreateMenuItem(
  nsIDOMNode *    aParentMenu,
  PRInt32      aIndex,
  const PRUnichar *  aName)
{
  if (APP_DEBUG) printf("In CreateMenuItem\n");
  nsresult rv=NS_OK;  
  nsCOMPtr<nsIDOMDocument>  doc;

  rv = aParentMenu->GetOwnerDocument(getter_AddRefs(doc));
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR Getting handle to the document\n");
	  return NS_ERROR_FAILURE;
  }
  nsString menuitemName(aName);
  
  // Create nsMenuItem
  nsCOMPtr<nsIDOMElement>  menuItemElement;
  nsString  tagName("menuitem");
  rv = doc->CreateElement(tagName, getter_AddRefs(menuItemElement));
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR creating the menu item element\n");
	  return NS_ERROR_FAILURE;
  }

  //Set the label for the menu item
  nsString menuitemlabel(aName);
  if (APP_DEBUG) printf("nsBrowserAppCore::CreateMenuItem Setting menu name to %s\n", menuitemlabel.ToNewCString());
  rv = menuItemElement->SetAttribute(nsString("value"), menuitemlabel);
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR Setting node value for menu item ****\n");
	  return NS_ERROR_FAILURE;
  }

  //Set the onaction attribute
  nsString menuitemCmd("gotoHistoryIndex(");
  menuitemCmd.Append(aIndex);
  menuitemCmd += ")";  
  if (APP_DEBUG) printf("nsBrowserAppCore::CreateMenuItem Setting action handler to %s\n", menuitemCmd.ToNewCString());
  nsString attrName("oncommand");
  rv = menuItemElement->SetAttribute(attrName, menuitemCmd);
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR setting onaction handler\n");
	  return NS_ERROR_FAILURE;
  }
  
  
  // Set the hist attribute to true
  rv = menuItemElement->SetAttribute(nsString("ishist"), nsString("true"));
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR setting ishist handler\n");
	  return NS_ERROR_FAILURE;
  }

    // Make a DOMNode out of it
  nsCOMPtr<nsIDOMNode>  menuItemNode = do_QueryInterface(menuItemElement);
  if (!menuItemNode) {
	  printf("nsBrowserAppCore::CreateMenuItem ERROR converting DOMElement to DOMNode *****\n");
	  return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> resultNode;
  // Make nsMenuItem a child of nsMenu
  rv = aParentMenu->AppendChild(menuItemNode, getter_AddRefs(resultNode));
  
  if (!NS_SUCCEEDED(rv)) 
  {
       printf("nsBrowserAppCore::CreateMenuItem ERROR appending menuitem to menu *****\n");
	   return NS_ERROR_FAILURE;
  }
  else
	 if (APP_DEBUG) printf("nsBrowserAppCore::CreateMenuItem Successfully appended menu item to parent\n");



  return NS_OK;
}



NS_IMETHODIMP
nsBrowserAppCore::ForwardButtonPopup()
{

	if (!mSHistory)  {
		printf("nsBrowserAppCore::ForwardButtonPopup Couldn't get a handle to SessionHistory\n");
		return NS_ERROR_FAILURE;
	}

  if (APP_DEBUG) printf("In BrowserAppCore::Forwardbuttonpopup\n");

 // Get handle to the "forwardbuttonpopup" element
  nsCOMPtr<nsIDOMElement>   forwardPopupElement;
  nsresult rv = FindNamedXULElement(mWebShell, "forwardbuttonpopup", &forwardPopupElement);

  if (!NS_SUCCEEDED(rv) ||  !forwardPopupElement)
  {
	 printf("nsBrowserAppCore::ForwardButtonPopup Couldn't get handle to forwardPopupElement\n");
     return NS_ERROR_FAILURE;
  }

  // Make a nsIDOMNode out of it
  nsCOMPtr<nsIDOMNode> forwardPopupNode(do_QueryInterface(forwardPopupElement)); 
  if (!forwardPopupNode) {
	  printf("nsBrowserAppCore::ForwardButtonPopup Couldn't make a node out of forwardpopupelement\n");
    return NS_ERROR_FAILURE;
  }
  
  //get handle to the Menu item under popup
  nsCOMPtr<nsIDOMNode>   menu;
  rv = forwardPopupNode->GetFirstChild(getter_AddRefs(menu));
  if (!NS_SUCCEEDED(rv) || !menu) {
	  printf("nsBrowserAppCore::ForwardButtonPopup Call to GetFirstChild failed\n");
     return NS_ERROR_FAILURE;
  }
  
  PRBool hasChildren=PR_FALSE;

  // Check if menu has children. If so, remove them.
  menu->HasChildNodes(&hasChildren);
  if (hasChildren) {
	  // Remove all old entries 
	  rv = ClearHistoryPopup(menu);
	  if (!NS_SUCCEEDED(rv)) {
		  printf("nsBrowserAppCore::ForwardMenuPopup Error while clearing old history entries\n");
	  }
  }    // hasChildren
  else {
	  if (APP_DEBUG) printf("nsBrowserAppCore::ForwardButtonPopup Menu has no children\n");
  }	 

  PRInt32 indix=0, i=0, length=0;
  //Get current index in Session History
  mSHistory->GetCurrentIndex(&indix);
 //Get total length of Session History
  mSHistory->GetHistoryLength(&length);

  //Decide on the # of items in the popup list 
  if ((length-indix) > SHISTORY_POPUP_LIST)
     i  = indix+SHISTORY_POPUP_LIST;
  else
	 i = length;

  for (PRInt32 j=indix+1;j<i;j++) {
      PRUnichar *title=nsnull;
	  char * url=nsnull;

      mSHistory->GetURLForIndex(j, &url);
      mSHistory->GetTitleForIndex(j, &title);
      nsAutoString  histTitle(title);      
      rv = CreateMenuItem(menu, j, title);
	  if (!NS_SUCCEEDED(rv)) 
		  printf("nsBrowserAppCore::ForwardbuttonPopup, Error while creating history menu items\n");
	  Recycle(title);
	  Recycle(url);
  } 
	 return NS_OK;

}


NS_IMETHODIMP
nsBrowserAppCore::UpdateGoMenu()
{

    if (!mSHistory)  {
		printf("nsBrowserAppCore::UpdateGoMenu Couldn't get a handle to SessionHistory\n");
		return NS_ERROR_FAILURE;
	}

  // Get handle to the "main-menubar" element
  nsCOMPtr<nsIDOMElement>   mainMenubarElement;
  nsresult rv = FindNamedXULElement(mWebShell, "main-menubar", &mainMenubarElement);

  if (!NS_SUCCEEDED(rv) ||  !mainMenubarElement)
  {
	 printf("Couldn't get handle to the Go menu\n");
     return NS_ERROR_FAILURE;
  }
  else {
	  if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu Got handle to the main-toolbox element\n");	
  }

  nsCOMPtr<nsIDOMNode> mainMenubarNode(do_QueryInterface(mainMenubarElement)); 
  if (!mainMenubarNode) {
    if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu Couldn't get Node out of Element\n");
    return NS_ERROR_FAILURE;
  }

   nsCOMPtr<nsIDOMNode> goMenuNode;
   PRBool hasChildren=PR_FALSE;
  // Check if toolbar has children.
  rv = mainMenubarNode->HasChildNodes(&hasChildren);
  if (NS_SUCCEEDED(rv) && hasChildren) {	
     nsCOMPtr<nsIDOMNodeList>   childList;

     //Get handle to the children list
     rv = mainMenubarNode->GetChildNodes(getter_AddRefs(childList));
     if (NS_SUCCEEDED(rv) && childList) {
        PRInt32 ccount=0;
        childList->GetLength((unsigned int *)&ccount);
        
		// Get the 'Go' menu
        for (PRInt32 i=0; i<ccount; i++) {
            nsCOMPtr<nsIDOMNode> child;
            rv = childList->Item(i, getter_AddRefs(child));
			if (!NS_SUCCEEDED(rv) || !child) {
               if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu Couldn't get child %d from menu bar\n", i);
			   return NS_ERROR_FAILURE;
			}
			// Get element out of the node
			nsCOMPtr<nsIDOMElement> childElement(do_QueryInterface(child));
			if (!childElement) {
				printf("nsBrowserAppCore::UpdateGoMenu Could n't get DOMElement out of DOMNode for child\n");
				return NS_ERROR_FAILURE;
			}
			nsString nodelabel;
            rv = childElement->GetAttribute(nsAutoString("value"), nodelabel);
			if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu Node Name for menu = %s\n", nodelabel.ToNewCString());
			if (!NS_SUCCEEDED(rv)) {
				printf("nsBrowserAppCore::UpdateGoMenu Couldn't get node name\n");
				return NS_ERROR_FAILURE;
			}
			nsString nodeid;
            rv = childElement->GetAttribute(nsAutoString("id"), nodeid);
			if (nodeid == "gomenu") {
				goMenuNode = child;
				break;
			}
        } //(for)	
     }   // if (childList)
  }    // hasChildren
  else {
	  if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu Menubar has no children\n");
	  return NS_ERROR_FAILURE;
  }

  if (!goMenuNode) {
     printf("nsBrowserAppCore::UpdateGoMenu Couldn't find Go Menu. returning\n");
	 return NS_ERROR_FAILURE;
 
  }

    //get handle to the menupopup under gomenu
  nsCOMPtr<nsIDOMNode>   menuPopup;
  rv = goMenuNode->GetFirstChild(getter_AddRefs(menuPopup));
  if (!NS_SUCCEEDED(rv) || !menuPopup) {
	  printf("nsBrowserAppCore::UpdateGoMenu Call to get menupopup under go menu failed\n");
     return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIDOMElement> menuPopupElement(do_QueryInterface(menuPopup));
  if (!menuPopupElement) {
	printf("nsBrowserAppCore::UpdateGoMenu Could n't get DOMElement out of DOMNode for menuPopup\n");
	return NS_ERROR_FAILURE;
  }

  // Clear all history children under Go menu
  rv = ClearHistoryPopup(menuPopup);
  if (!NS_SUCCEEDED(rv)) {
	  printf("nsBrowserAppCore::UpdateGoMenu Error while clearing old history list\n");
  }
  
  PRInt32 length=0,i=0;
  //Get total length of the  Session History
  mSHistory->GetHistoryLength(&length);

  //Decide on the # of items in the popup list 
  if (length > SHISTORY_POPUP_LIST)
     i  = length-SHISTORY_POPUP_LIST;

  for (PRInt32 j=length-1;j>=i;j--) {
      PRUnichar  *title=nsnull;
	  char * url=nsnull;

      mSHistory->GetURLForIndex(j, &url);
      nsAutoString  histURL(url);
      mSHistory->GetTitleForIndex(j, &title);
      nsAutoString  histTitle(title);
      if (APP_DEBUG) printf("nsBrowserAppCore::UpdateGoMenu URL = %s, TITLE = %s\n", histURL.ToNewCString(), histTitle.ToNewCString());
      rv = CreateMenuItem(menuPopup, j, title);
	  if (!NS_SUCCEEDED(rv)) {
        printf("nsBrowserAppCore::UpdateGoMenu Error while creating history mene item\n");
	  }
	  Recycle(title);
	  Recycle(url);
     }
  return NS_OK;

}



NS_IMETHODIMP
nsBrowserAppCore::ClearHistoryPopup(nsIDOMNode * aParent)
{

	 nsresult rv;
	 nsCOMPtr<nsIDOMNode> menu = dont_QueryInterface(aParent);

     nsCOMPtr<nsIDOMNodeList>   childList;

     //Get handle to the children list
     rv = menu->GetChildNodes(getter_AddRefs(childList));
     if (NS_SUCCEEDED(rv) && childList) {
        PRInt32 ccount=0;
        childList->GetLength((unsigned int *)&ccount);

        // Remove the children that has the 'hist' attribute set to true.
        for (PRInt32 i=0; i<ccount; i++) {
            nsCOMPtr<nsIDOMNode> child;
            rv = childList->Item(i, getter_AddRefs(child));
			if (!NS_SUCCEEDED(rv) ||  !child) {
				printf("nsBrowserAppCore::ClearHistoryPopup, Could not get child\n");
				return NS_ERROR_FAILURE;
			}
			// Get element out of the node
			nsCOMPtr<nsIDOMElement> childElement(do_QueryInterface(child));
			if (!childElement) {
				printf("nsBrowserAppCore::ClearHistorypopup Could n't get DOMElement out of DOMNode for child\n");
				return NS_ERROR_FAILURE;
			}
			nsString  attrname("ishist");
			nsString  attrvalue;
			rv = childElement->GetAttribute(attrname, attrvalue);
			if (NS_SUCCEEDED(rv) && attrvalue == "true") {
				// It is a history menu item. Remove it
                nsCOMPtr<nsIDOMNode> ret;			    
                rv = menu->RemoveChild(child, getter_AddRefs(ret));
			    if (NS_SUCCEEDED(rv)) {
				   if (ret) {
				      if (APP_DEBUG) printf("nsBrowserAppCore::ClearHistoryPopup Child %x removed from the popuplist \n", (unsigned int) child.get());			          
				   }
				   else {
				      printf("nsBrowserAppCore::ClearHistoryPopup Child %x was not removed from popuplist\n", (unsigned int) child.get());
				   }
				}  // NS_SUCCEEDED(rv)
			    else
				{
				   printf("nsBrowserAppCore::ClearHistoryPopup Child %x was not removed from popuplist\n", (unsigned int) child.get());
                   return NS_ERROR_FAILURE;
				}				  
			}  // atrrvalue == true			 
		} //(for)	
	 }   // if (childList)
	 return NS_OK;
}


static void DOMWindowToWebShellWindow(
              nsIDOMWindow *DOMWindow,
              nsCOMPtr<nsIWebShellWindow> *webWindow)
{
  if (!DOMWindow)
    return; // with webWindow unchanged -- its constructor gives it a null ptr

  nsCOMPtr<nsIScriptGlobalObject> globalScript(do_QueryInterface(DOMWindow));
  nsCOMPtr<nsIWebShell> webshell, rootWebshell;
  if (globalScript)
    globalScript->GetWebShell(getter_AddRefs(webshell));
  if(!webshell)
   return;
  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  webshell->GetTopLevelWindow(getter_AddRefs(topLevelWindow));
  *webWindow = do_QueryInterface(topLevelWindow);
}


NS_IMETHODIMP    
nsBrowserAppCore::WalletPreview(nsIDOMWindow* aWin, nsIDOMWindow* aForm)
{
  NS_PRECONDITION(aForm != nsnull, "null ptr");
  if (! aForm)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aForm); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if (NS_SUCCEEDED(res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_Prefill(shell, PR_FALSE);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    if (NS_FAILED(res)) { /* this just means that there was nothing to prefill */
      return NS_OK;
    }
  } else {
    return res;
  }


    // (code adapted from nsToolkitCore::ShowModal. yeesh.)
    nsresult           rv;
    nsIAppShellService *appShell;
    nsIWebShellWindow  *window;

    window = nsnull;

    nsCOMPtr<nsIURI> urlObj;
    char * urlstr = "chrome://wallet/content/WalletPreview.xul";
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIURI *uri = nsnull;
    rv = service->NewURI(urlstr, nsnull, &uri);
    if (NS_FAILED(rv)) return rv;

    rv = uri->QueryInterface(nsIURI::GetIID(), (void**)&urlObj);
    NS_RELEASE(uri);
    if (NS_FAILED(rv))
        return rv;

    rv = nsServiceManager::GetService(kAppShellServiceCID, kIAppShellServiceIID,
                                    (nsISupports**) &appShell);
    if (NS_FAILED(rv))
        return rv;

    // Create "save to disk" nsIXULCallbacks...
    //nsIXULWindowCallbacks *cb = new nsFindDialogCallbacks( aURL, aContentType );
    nsIXULWindowCallbacks *cb = nsnull;

    nsCOMPtr<nsIWebShellWindow> parent;
    DOMWindowToWebShellWindow(aWin, &parent);
    window = nsnull;
    appShell->CreateTopLevelWindow(parent, urlObj, PR_TRUE, PR_TRUE,
                              NS_CHROME_ALL_CHROME | NS_CHROME_OPEN_AS_DIALOG,
                              cb, 504, 436, &window);
    if (window != nsnull) {
      appShell->RunModalDialog(&window, parent, nsnull, NS_CHROME_ALL_CHROME,
                               cb, 504, 436);
      NS_RELEASE(window);
    }
    nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletChangePassword()
{
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_ChangePassword();
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
  }
  return NS_OK;
}

#include "nsIDOMHTMLDocument.h"
static NS_DEFINE_IID(kIDOMHTMLDocumentIID, NS_IDOMHTMLDOCUMENT_IID);
NS_IMETHODIMP    
nsBrowserAppCore::WalletQuickFillin(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aWin); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_Prefill(shell, PR_TRUE);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    return NS_OK;
  } else {
    return res;
  }
}

NS_IMETHODIMP    
nsBrowserAppCore::WalletRequestToCapture(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsIPresShell* shell;
  shell = nsnull;
  nsCOMPtr<nsIWebShell> webcontent; 

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobalObject; 
  scriptGlobalObject = do_QueryInterface(aWin); 
  scriptGlobalObject->GetWebShell(getter_AddRefs(webcontent)); 

  shell = GetPresShellFor(webcontent);
  nsIWalletService *walletservice;
  nsresult res;
  res = nsServiceManager::GetService(kWalletServiceCID,
                                     kIWalletServiceIID,
                                     (nsISupports **)&walletservice);
  if ((NS_OK == res) && (nsnull != walletservice)) {
    res = walletservice->WALLET_RequestToCapture(shell);
    nsServiceManager::ReleaseService(kWalletServiceCID, walletservice);
    return NS_OK;
  } else {
    return res;
  }
}

NS_IMETHODIMP    
nsBrowserAppCore::LoadUrl(const PRUnichar *aUrl)
{
  nsresult rv = NS_OK;

  if (mIsLoadingHistory) {
     SetLoadingFlag(PR_FALSE);
  }
  /* Ask nsWebShell to load the URl */
  if ( mIsViewSource ) {
    // Viewing source, load with "view-source" command.
    rv = mContentAreaWebShell->LoadURL( aUrl, "view-source", nsnull, PR_FALSE );
  } else {
    // Normal browser.
    rv = mContentAreaWebShell->LoadURL( aUrl );
  }

  return rv;
}

#ifdef DEBUG
#include "nsProxyObjectManager.h"

class PageCycler : public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  PageCycler(nsBrowserAppCore* appCore)
    : mAppCore(appCore), mBuffer(nsnull), mCursor(nsnull) { 
    NS_INIT_REFCNT();
    NS_ADDREF(mAppCore);
  }

  virtual ~PageCycler() { 
    if (mBuffer) delete[] mBuffer;
    NS_RELEASE(mAppCore);
  }

  nsresult Init(const char* nativePath) {
    nsresult rv;
    mFile = nativePath;
    if (!mFile.IsFile())
      return mFile.Error();

    nsCOMPtr<nsISupports> in;
    rv = NS_NewTypicalInputFileStream(getter_AddRefs(in), mFile);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIInputStream> inStr = do_QueryInterface(in, &rv);
    if (NS_FAILED(rv)) return rv;
    PRUint32 avail;
    rv = inStr->Available(&avail);
    if (NS_FAILED(rv)) return rv;

    mBuffer = new char[avail + 1];
    if (mBuffer == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
    PRUint32 amt;
    rv = inStr->Read(mBuffer, avail, &amt);
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION(amt == avail, "Didn't get the whole file.");
    mBuffer[avail] = '\0';
    mCursor = mBuffer;

    NS_WITH_SERVICE(nsIObserverService, obsServ, NS_OBSERVERSERVICE_PROGID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsString topic("EndDocumentLoad");
    rv = obsServ->AddObserver(this, topic.GetUnicode());
    return rv; 
  }

  nsresult GetNextURL(nsString& result) {
    result = mCursor;
    PRInt32 pos = result.Find(NS_LINEBREAK);
    if (pos > 0) {
      result.Truncate(pos);
      mLastRequest = result;
      mCursor += pos + NS_LINEBREAK_LEN;
      return NS_OK;
    }
    else if ( !result.IsEmpty() ) {
      // no more URLs after this one
      mCursor += result.Length(); // Advance cursor to terminating '\0'
      mLastRequest = result;
      return NS_OK;
    }
    else {
      // no more URLs, so quit the browser
      nsresult rv;
      NS_WITH_SERVICE(nsIAppShellService, appShellServ, kAppShellServiceCID, &rv);
      if(NS_FAILED(rv)) return rv;
      NS_WITH_SERVICE(nsIProxyObjectManager, pIProxyObjectManager, nsIProxyObjectManager::GetCID(), &rv);
      if(NS_FAILED(rv)) return rv;
      nsCOMPtr<nsIAppShellService> appShellProxy;
      rv = pIProxyObjectManager->GetProxyObject(NS_UI_THREAD_EVENTQ, nsIAppShellService::GetIID(), 
                                                appShellServ, PROXY_ASYNC | PROXY_ALWAYS,
                                                getter_AddRefs(appShellProxy));

      (void)appShellProxy->Quit();
      return NS_ERROR_FAILURE;
    }
  }

  NS_IMETHOD Observe(nsISupports* aSubject, 
                     const PRUnichar* aTopic,
                     const PRUnichar* someData) {
    nsresult rv = NS_OK;
    nsString data(someData);
    if (data.Find(mLastRequest) == 0) {
      char* dataStr = data.ToNewCString();
      printf("########## PageCycler loaded: %s\n", dataStr);
      nsCRT::free(dataStr);

      nsAutoString url;
      rv = GetNextURL(url);
      if (NS_SUCCEEDED(rv)) {
        rv = mAppCore->LoadUrl(url.GetUnicode());
      }
    }
    else {
      char* dataStr = data.ToNewCString();
      printf("########## PageCycler possible failure for: %s\n", dataStr);
      nsCRT::free(dataStr);
    }
    return rv;
  }
  
protected:
  nsBrowserAppCore*     mAppCore;
  nsFileSpec            mFile;
  char*                 mBuffer;
  char*                 mCursor;
  nsAutoString          mLastRequest;
};

NS_IMPL_ISUPPORTS1(PageCycler, nsIObserver);

#endif

NS_IMETHODIMP    
nsBrowserAppCore::LoadInitialPage(void)
{
  static PRBool cmdLineURLUsed = PR_FALSE;
  char * urlstr = nsnull;
  nsresult rv;

  if (!cmdLineURLUsed) {
    NS_WITH_SERVICE(nsICmdLineService, cmdLineArgs, kCmdLineServiceCID, &rv);
    if (NS_FAILED(rv)) {
      if (APP_DEBUG) fprintf(stderr, "Could not obtain CmdLine processing service\n");
      return NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    // First, check if there's a URL file to load (for testing), and if there 
    // is, process it instead of anything else.
    char* file = 0;
    rv = cmdLineArgs->GetCmdLineValue("-f", &file);
    if (NS_SUCCEEDED(rv) && file) {
      PageCycler* bb = new PageCycler(this);
      if (bb == nsnull) {
        nsCRT::free(file);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(bb);
      rv = bb->Init(file);
      nsCRT::free(file);
      if (NS_FAILED(rv)) return rv;

      nsAutoString str;
      rv = bb->GetNextURL(str);
      if (NS_SUCCEEDED(rv)) {
        urlstr = str.ToNewCString();
      }
      NS_RELEASE(bb);
    }
    else {
#endif
  
    // Examine content URL.
    if ( mContentAreaWebShell ) {
      const PRUnichar *url = 0;
      rv = mContentAreaWebShell->GetURL(&url );
      /* Check whether url is valid. Otherwise we compare 0x00 with 
         * "about:blank" and there by return from here with out 
         * loading the command line url or default home page.
         */
      if ( NS_SUCCEEDED( rv ) && url ) {
        if ( nsString(url) != "about:blank" ) {
          // Something has already been loaded (probably via window.open),
          // leave it be.
          return NS_OK;
        }
      }
    }

    // Get the URL to load
    rv = cmdLineArgs->GetURLToLoad(&urlstr);

#ifdef DEBUG
    }
#endif

    if (urlstr != nsnull) {
      // A url was provided. Load it
      if (APP_DEBUG) printf("Got Command line URL to load %s\n", urlstr);
      nsString url( urlstr ); // Convert to unicode.
      rv = LoadUrl( url.GetUnicode() );
      cmdLineURLUsed = PR_TRUE;
      return rv;
    }
  }

  // No URL was provided in the command line. Load the default provided
  // in the navigator.xul;

  // but first, abort if the window doesn't want a default page loaded
  if (mWebShellWin) {
    PRBool loadDefault;
    mWebShellWin->ShouldLoadDefaultPage(&loadDefault);
    if (!loadDefault)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMElement>    argsElement;

  rv = FindNamedXULElement(mWebShell, "args", &argsElement);
  if (!argsElement) {
    // Couldn't get the "args" element from the xul file. Load a blank page
    if (APP_DEBUG) printf("Couldn't find args element\n");
    nsString url("about:blank"); 
    rv = LoadUrl( url.GetUnicode() );
    return rv;
  }

  // Load the default page mentioned in the xul file.
  nsString value;
  argsElement->GetAttribute(nsString("value"), value);
  if (value.Length()) {
    rv = LoadUrl(value.GetUnicode());
    return rv;
  }

  if (APP_DEBUG) printf("Quitting LoadInitialPage\n");
  return NS_OK;
}

static
nsIScriptContext *    
GetScriptContext(nsIDOMWindow * aWin) {
  nsIScriptContext * scriptContext = nsnull;
  if (nsnull != aWin) {
    nsIDOMDocument * domDoc;
    aWin->GetDocument(&domDoc);
    if (nsnull != domDoc) {
      nsIDocument * doc;
      if (NS_OK == domDoc->QueryInterface(kIDocumentIID,(void**)&doc)) {
        nsIScriptContextOwner * owner = doc->GetScriptContextOwner();
        if (nsnull != owner) {
          owner->GetScriptContext(&scriptContext);
          NS_RELEASE(owner);
        }
        NS_RELEASE(doc);
      }
      NS_RELEASE(domDoc);
    }
  }
  return scriptContext;
}

NS_IMETHODIMP    
nsBrowserAppCore::SetContentWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  mContentWindow = aWin;
  // NS_ADDREF(aWin); WE DO NOT OWN THIS


  // we do not own the script context, so don't addref it
  nsCOMPtr<nsIScriptContext>	scriptContext = getter_AddRefs(GetScriptContext(aWin));
  mContentScriptContext = scriptContext;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(mContentWindow) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWebShell> webShell;
  globalObj->GetWebShell(getter_AddRefs(webShell));
  if (webShell) {
    mContentAreaWebShell = webShell;
    // NS_ADDREF(mContentAreaWebShell); WE DO NOT OWN THIS
    webShell->SetDocLoaderObserver((nsIDocumentLoaderObserver *)this);
	if (mSHistory)
       webShell->SetSessionHistory(mSHistory);

    // Cache the Document Loader for the content area webshell.  This is a 
    // weak reference that is *not* reference counted...
    nsCOMPtr<nsIDocumentLoader> docLoader;

    webShell->GetDocumentLoader(*getter_AddRefs(docLoader));
    mContentAreaDocLoader = docLoader.get();

    const PRUnichar * name;
    webShell->GetName( &name);
    nsAutoString str(name);

    if (APP_DEBUG) {
      printf("Attaching to Content WebShell [%s]\n", (const char *)nsAutoCString(str));
    }
  }

  return NS_OK;

}



NS_IMETHODIMP    
nsBrowserAppCore::SetWebShellWindow(nsIDOMWindow* aWin)
{
  NS_PRECONDITION(aWin != nsnull, "null ptr");
  if (! aWin)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  if (!globalObj) {
    return NS_ERROR_FAILURE;
  }

  nsIWebShell * webShell;
  globalObj->GetWebShell(&webShell);
  if (nsnull != webShell) {
    mWebShell = webShell;
    //NS_ADDREF(mWebShell); WE DO NOT OWN THIS
    // inform our top level webshell that we are its parent URI content listener...
    mWebShell->SetParentURIContentListener(this);

    const PRUnichar * name;
    webShell->GetName( &name);
    nsAutoString str(name);

    if (APP_DEBUG) {
      printf("Attaching to WebShellWindow[%s]\n", (const char *)nsAutoCString(str));
    }

    nsIWebShellContainer * webShellContainer;
    webShell->GetContainer(webShellContainer);
    if (nsnull != webShellContainer)
    {
    	nsCOMPtr<nsIWebShellWindow> webShellWin;
      if (NS_OK == webShellContainer->QueryInterface(kIWebShellWindowIID, getter_AddRefs(webShellWin)))
      {
        mWebShellWin = webShellWin;		// WE DO NOT OWN THIS
      }
      NS_RELEASE(webShellContainer);
    }
    NS_RELEASE(webShell);
  }
  return NS_OK;
}



// Utility to extract document from a webshell object.
static nsCOMPtr<nsIDocument> getDocument( nsIWebShell *aWebShell ) {
    nsCOMPtr<nsIDocument> result;

    // Get content viewer from the web shell.
    nsCOMPtr<nsIContentViewer> contentViewer;
    nsresult rv = aWebShell ? aWebShell->GetContentViewer(getter_AddRefs(contentViewer))
                            : NS_ERROR_NULL_POINTER;

    if ( contentViewer ) {
        // Up-cast to a document viewer.
        nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));
        if ( docViewer ) {
            // Get the document from the doc viewer.
            docViewer->GetDocument(*getter_AddRefs(result));
        } else {
            if (APP_DEBUG) printf( "%s %d: Upcast to nsIDocumentViewer failed\n",
                                   __FILE__, (int)__LINE__ );
        }
    } else {
        if (APP_DEBUG) printf( "%s %d: GetContentViewer failed, rv=0x%X\n",
                               __FILE__, (int)__LINE__, (int)rv );
    }
    return result;
}

// Utility to set element attribute.
static nsresult setAttribute( nsIWebShell *shell,
                              const char *id,
                              const char *name,
                              const nsString &value ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIDocument> doc = getDocument( shell );

    if ( doc ) {
        // Up-cast.
        nsCOMPtr<nsIDOMXULDocument> xulDoc( do_QueryInterface(doc) );
        if ( xulDoc ) {
            // Find specified element.
            nsCOMPtr<nsIDOMElement> elem;
            rv = xulDoc->GetElementById( id, getter_AddRefs(elem) );
            if ( elem ) {
                // Set the text attribute.
                rv = elem->SetAttribute( name, value );
                if ( rv != NS_OK ) {
                     if (APP_DEBUG) printf("SetAttribute failed, rv=0x%X\n",(int)rv);
                }
            } else {
                if (APP_DEBUG) printf("GetElementByID failed, rv=0x%X\n",(int)rv);
            }
        } else {
          if (APP_DEBUG)   printf("Upcast to nsIDOMXULDocument failed\n");
        }
    } else {
        if (APP_DEBUG) printf("getDocument failed, rv=0x%X\n",(int)rv);
    }
    return rv;
}

// nsIDocumentLoaderObserver methods

NS_IMETHODIMP
nsBrowserAppCore::OnStartDocumentLoad(nsIDocumentLoader* aLoader, nsIURI* aURL, const char* aCommand)
{
  NS_PRECONDITION(aLoader != nsnull, "null ptr");
  if (! aLoader)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(aURL != nsnull, "null ptr");
  if (! aURL)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  // Notify observers that a document load has started in the
  // content window.
  NS_WITH_SERVICE(nsIObserverService, observer, NS_OBSERVERSERVICE_PROGID, &rv);
  if (NS_FAILED(rv)) return rv;

  char* url;
  rv = aURL->GetSpec(&url);
  if (NS_FAILED(rv)) return rv;

  nsAutoString urlStr(url);

#ifdef DEBUG_warren
  char* urls;
  aURL->GetSpec(&urls);
  if (gTimerLog == nsnull)
    gTimerLog = PR_NewLogModule("Timer");
  mLoadStartTime = PR_IntervalNow();
  PR_LOG(gTimerLog, PR_LOG_DEBUG, 
         (">>>>> Starting timer for %s\n", urls));
  printf(">>>>> Starting timer for %s\n", urls);
  nsCRT::free(urls);
#endif

  // Check if this notification is for a frame
  PRBool isFrame=PR_FALSE;
  nsCOMPtr<nsISupports> container;
  aLoader->GetContainer(getter_AddRefs(container));
  if (container) {
     nsCOMPtr<nsIWebShell>   ws(do_QueryInterface(container));
	 if (ws) {
       nsCOMPtr<nsIWebShell> parent;
		 ws->GetParent(*getter_AddRefs(parent));
		 if (parent) 
			 isFrame = PR_TRUE;
     }
  }

  if (!isFrame) {
    nsAutoString kStartDocumentLoad("StartDocumentLoad");
    rv = observer->Notify(mContentWindow,
                        kStartDocumentLoad.GetUnicode(),
                        urlStr.GetUnicode());

    // XXX Ignore rv for now. They are using nsIEnumerator instead of
    // nsISimpleEnumerator.
    // set the url string in the urlbar only for toplevel pages, not for frames
	setAttribute( mWebShell, "urlbar", "value", url);
  }


  // Kick start the throbber
  nsAutoString trueStr("true");
  nsAutoString emptyStr;
  setAttribute( mWebShell, "Browser:Throbber", "busy", trueStr );

  // Enable the Stop buton
  setAttribute( mWebShell, "canStop", "disabled", emptyStr );

  //Disable the reload button
  setAttribute(mWebShell, "canReload", "disabled", trueStr);

  PRBool result=PR_TRUE;
  // Check with sessionHistory if you can go forward
  CanGoForward(&result);
  setAttribute(mWebShell, "canGoForward", "disabled", (result == PR_TRUE) ? "" : "true");


    // Check with sessionHistory if you can go back
  CanGoBack(&result);
  setAttribute(mWebShell, "canGoBack", "disabled", (result == PR_TRUE) ? "" : "true");


  nsCRT::free(url);

  return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::OnEndDocumentLoad(nsIDocumentLoader* aLoader, nsIChannel* channel, nsresult aStatus,
									nsIDocumentLoaderObserver * aObserver)
{
  NS_PRECONDITION(aLoader != nsnull, "null ptr");
  if (! aLoader)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(channel != nsnull, "null ptr");
  if (! channel)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  nsCOMPtr<nsIURI> aUrl;
  rv = channel->GetOriginalURI(getter_AddRefs(aUrl));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString url;
  rv = aUrl->GetSpec(getter_Copies(url));
  if (NS_FAILED(rv)) return rv;

  PRBool  isFrame=PR_FALSE;
  nsCOMPtr<nsIWebShell> webshell, parent;
  // Is this a frame ?
  webshell = do_QueryInterface(aObserver);
  if (webshell) {
    webshell->GetParent(*getter_AddRefs(parent));
  }
  if (parent)
	isFrame = PR_TRUE;


  if (mContentAreaDocLoader) {
    PRBool isBusy = PR_FALSE;

    mContentAreaDocLoader->IsBusy(isBusy);
    if (isBusy) {
      return NS_OK;
    }
  }

  /* Inform Session History about the status of the page load */
  if (mSHistory) {
    mSHistory->UpdateStatus(webshell, (PRInt32) aStatus); 
  }
  if (mIsLoadingHistory) {
      SetLoadingFlag(PR_FALSE);
  }

  /* If this is a frame, don't do any of the Global History
   * & observer thingy 
   */
  if (!isFrame) {
      nsAutoString urlStr(url);
      nsAutoString kEndDocumentLoad("EndDocumentLoad");
      nsAutoString kFailDocumentLoad("FailDocumentLoad");

      // Notify observers that a document load has started in the
      // content window.
      NS_WITH_SERVICE(nsIObserverService, observer, NS_OBSERVERSERVICE_PROGID, &rv);
      if (NS_FAILED(rv)) return rv;

      rv = observer->Notify(mContentWindow,
                            NS_SUCCEEDED(aStatus) ? kEndDocumentLoad.GetUnicode() : kFailDocumentLoad.GetUnicode(),
                            urlStr.GetUnicode());

      // XXX Ignore rv for now. They are using nsIEnumerator instead of
      // nsISimpleEnumerator.
	  /*
	   * Update the 'Go' menu. I know this adds discrepancy between the 'Go'
	   * menu and the Back button when it comes to Session History in frame
	   * pages. But most of these sub-frames don't have title which leads to
	   * blank menu items in the 'go' menu. So, I'm taking sub-frames
	   * totally off the go menu. This is how 4.x behaves.
	   */ 
      if (NS_SUCCEEDED(aStatus))
	    UpdateGoMenu();

      /* To satisfy a request from the QA group */
      if (aStatus == NS_OK) {
        fprintf(stdout, "Document %s loaded successfully\n", (const char*)url);
        fflush(stdout);
	  }
      else {
        fprintf(stdout, "Error loading URL %s \n", (const char*)url);
        fflush(stdout);
	  }
  } //if (!isFrame)

#ifdef DEBUG_warren
  char* urls;
  aUrl->GetSpec(&urls);
  if (gTimerLog == nsnull)
    gTimerLog = PR_NewLogModule("Timer");
  PRIntervalTime end = PR_IntervalNow();
  PRIntervalTime diff = end - mLoadStartTime;
  PR_LOG(gTimerLog, PR_LOG_DEBUG, 
         (">>>>> Stopping timer for %s. Elapsed: %.3f\n", 
          urls, PR_IntervalToMilliseconds(diff) / 1000.0));
  printf(">>>>> Stopping timer for %s. Elapsed: %.3f\n", 
         urls, PR_IntervalToMilliseconds(diff) / 1000.0);
  nsCRT::free(urls);
#endif

  setAttribute( mWebShell, "Browser:Throbber", "busy", "false" );

    //Disable the Stop button
  setAttribute( mWebShell, "canStop", "disabled", "true" );

  //Enable the reload button
  setAttribute(mWebShell, "canReload", "disabled", "");

  return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIChannel* channel,
                                           const char *aContentType,
                                           const char *aCommand )
{
    nsresult rv = NS_OK;

    // Turn off the indicators in the chrome.
    setAttribute( mWebShell, "Browser:Throbber", "busy", "false" );

    // Get "unknown content type handler" and have it handle this.
    nsIUnknownContentTypeHandler *handler;
    rv = nsServiceManager::GetService( NS_IUNKNOWNCONTENTTYPEHANDLER_PROGID,
                                       nsIUnknownContentTypeHandler::GetIID(),
                                       (nsISupports**)&handler );

    if ( NS_SUCCEEDED( rv ) ) {
        /* Have handler take care of this. */
        // Get DOM window.
        nsCOMPtr<nsIDOMWindow> domWindow;
        rv = mWebShellWin->ConvertWebShellToDOMWindow( mWebShell,
                                                       getter_AddRefs( domWindow ) );
        if ( NS_SUCCEEDED( rv ) && domWindow ) {
            rv = handler->HandleUnknownContentType( channel, aContentType, domWindow );
        } else {
            #ifdef NS_DEBUG
            printf( "%s %d: ConvertWebShellToDOMWindow failed, rv=0x%08X\n",
                    __FILE__, (int)__LINE__, (int)rv );
            #endif
        }

        // Release the unknown content type handler service object.
        nsServiceManager::ReleaseService( NS_IUNKNOWNCONTENTTYPEHANDLER_PROGID, handler );
    } else {
        #ifdef NS_DEBUG
        printf( "%s %d: GetService failed for unknown content type handler, rv=0x%08X\n",
                __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

NS_IMETHODIMP
nsBrowserAppCore::OnStartURLLoad(nsIDocumentLoader* loader, 
                                 nsIChannel* channel,
                                 nsIContentViewer* aViewer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::OnProgressURLLoad(nsIDocumentLoader* loader, 
                                    nsIChannel* channel, PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  char *urlString = 0;
  aURL->GetSpec( &urlString );
  nsCRT::free(urlString);
  return rv;
}


NS_IMETHODIMP
nsBrowserAppCore::OnStatusURLLoad(nsIDocumentLoader* loader, 
                                  nsIChannel* channel, nsString& aMsg)
{
  nsresult rv = setAttribute( mWebShell, "Browser:Status", "value", aMsg );
   return rv;
}


NS_IMETHODIMP
nsBrowserAppCore::OnEndURLLoad(nsIDocumentLoader* loader, 
                               nsIChannel* channel, nsresult aStatus)
{
  return NS_OK;
}

/////////////////////////////////////////////////////////
//             nsISessionHistory methods              //
////////////////////////////////////////////////////////


NS_IMETHODIMP    
nsBrowserAppCore::GoBack(nsIWebShell * aPrev)
{
  if (mIsLoadingHistory) {
	  SetLoadingFlag(PR_FALSE);
  }
  mIsLoadingHistory = PR_TRUE;
  if (mSHistory) {
	  //mSHistory checks for null pointers
    return mSHistory->GoBack(aPrev);
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::GoForward(nsIWebShell * aPrev)
{
  if (mIsLoadingHistory) {
     SetLoadingFlag(PR_FALSE);
  }
  mIsLoadingHistory = PR_TRUE;
  if (mSHistory) {
	  //mSHistory checks for null pointers
    return mSHistory->GoForward(aPrev);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::Reload(nsIWebShell * aPrev, nsLoadFlags aType)
{
  if (mIsLoadingHistory) {
     SetLoadingFlag(PR_FALSE);
  }
  mIsLoadingHistory = PR_TRUE;
  if (mSHistory) {
	  //mSHistory checks for null pointers
	return mSHistory->Reload(aPrev, aType);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::Add(const char * aURL, nsIWebShell * aWebShell)
{
 return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::Goto(PRInt32 aGotoIndex, nsIWebShell * aPrev, PRBool aIsReloading)
{
   nsresult rv=NS_OK;
   if (mSHistory) {
	   //mSHistory checks for null pointers
     rv = mSHistory->Goto(aGotoIndex, aPrev, PR_FALSE);
   }
   return rv;
}


NS_IMETHODIMP
nsBrowserAppCore::SetLoadingFlag(PRBool aFlag)
{
  mIsLoadingHistory = aFlag;
  if (mSHistory)
	mSHistory->SetLoadingFlag(aFlag);
  return NS_OK;
}

NS_IMETHODIMP
nsBrowserInstance::UpdateStatus(nsIWebShell * aWebShell, PRInt32 aStatus) {
	if (mSHistory) {
		//mSHistory checks for null pointers
		mSHistory->UpdateStatus(aWebShell, aStatus);
	}
	return NS_OK;
}

/* Error checks on the arguments for all the following
 * methods done in nsSessionHistory.cpp
 */

NS_IMETHODIMP
nsBrowserAppCore::GetLoadingFlag(PRBool *aFlag)
{

  if (mSHistory)
	mSHistory->GetLoadingFlag(aFlag);
  return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::CanGoForward(PRBool * aResult)
{

   if (mSHistory) {
     mSHistory->CanGoForward(aResult);
   }
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::CanGoBack(PRBool * aResult)
{

   if (mSHistory)
     mSHistory->CanGoBack(aResult);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::GetHistoryLength(PRInt32 * aResult)
{

   if (mSHistory)
     mSHistory->GetHistoryLength(aResult);
   return NS_OK;
}


NS_IMETHODIMP
nsBrowserAppCore::GetCurrentIndex(PRInt32 * aResult)
{

   if (mSHistory)
     mSHistory->GetCurrentIndex(aResult);
   return NS_OK;

}

NS_IMETHODIMP
nsBrowserAppCore::GetURLForIndex(PRInt32 aIndex,  char** aURL)
{
   if (mSHistory)
     return  mSHistory->GetURLForIndex(aIndex, aURL);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetURLForIndex(PRInt32 aIndex, const char* aURL)
{
   if (mSHistory)
      mSHistory->SetURLForIndex(aIndex, aURL);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::GetTitleForIndex(PRInt32 aIndex,  PRUnichar** aTitle)
{

   if (mSHistory)
      mSHistory->GetTitleForIndex(aIndex, aTitle);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetTitleForIndex(PRInt32 aIndex, const PRUnichar* aTitle)
{
   if (mSHistory)
      mSHistory->SetTitleForIndex(aIndex, aTitle);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::GetHistoryObjectForIndex(PRInt32 aIndex, nsISupports ** aState)
{

   if (mSHistory)
      mSHistory->GetHistoryObjectForIndex(aIndex, aState);
   return NS_OK;
}

NS_IMETHODIMP
nsBrowserAppCore::SetHistoryObjectForIndex(PRInt32 aIndex, nsISupports * aState)
{
   if (mSHistory)
      mSHistory->SetHistoryObjectForIndex(aIndex, aState);
   return NS_OK;
}


NS_IMETHODIMP    
nsBrowserAppCore::PrintPreview()
{ 

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Copy()
{ 
  nsIPresShell * presShell = GetPresShellFor(mContentAreaWebShell);
  if (nsnull != presShell) {
    presShell->DoCopy();
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Print()
{  
  if (mContentAreaWebShell) {
    nsCOMPtr<nsIContentViewer> viewer;    
    mWebShell->GetContentViewer(getter_AddRefs(viewer));    
    if (nsnull != viewer) {
      nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
      if (viewerFile) {
        NS_ENSURE_SUCCESS(viewerFile->Print(), NS_ERROR_FAILURE);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserAppCore::Close()
{ 
  EndObserving();

  // Undo other stuff we did in SetContentWindow.
  if ( mContentAreaWebShell ) {
      mContentAreaWebShell->SetDocLoaderObserver( 0 );
      mContentAreaWebShell->SetSessionHistory( 0 );
  }

  // session history is an instance, not a service
  NS_IF_RELEASE(mSHistory);

  // Release search context.
  mSearchContext = 0;

  // unregister ourselves with the uri loader because
  // we can no longer accept new content!
  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsIURILoader, dispatcher, NS_URI_LOADER_PROGID, &rv);
  if (NS_SUCCEEDED(rv)) 
    rv = dispatcher->UnRegisterContentListener(this);

  return NS_OK;
}


void
nsBrowserAppCore::InitializeSearch( nsIFindComponent *finder )
{
    nsresult rv = NS_OK;

    if ( finder && !mSearchContext ) {
        // Create the search context for this browser window.
        rv = finder->CreateContext( mContentAreaWebShell, nsnull, getter_AddRefs(mSearchContext));
        if ( NS_FAILED( rv ) ) {
            #ifdef NS_DEBUG
            printf( "%s %d CreateContext failed, rv=0x%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
            #endif
        }
    }
}

NS_IMETHODIMP    
nsBrowserAppCore::Find()
{
    nsresult rv = NS_OK;
    PRBool   found = PR_FALSE;
    
    // Get find component.
    nsIFindComponent *finder;
    rv = nsServiceManager::GetService( NS_IFINDCOMPONENT_PROGID,
                                       nsIFindComponent::GetIID(),
                                       (nsISupports**)&finder );
    if ( NS_SUCCEEDED( rv ) ) {
        // Make sure we've initialized searching for this document.
        InitializeSearch( finder );

        // Perform find via find component.
        if ( finder && mSearchContext ) {
            rv = finder->Find( mSearchContext, &found );
        }

        // Release the service.
        nsServiceManager::ReleaseService( NS_IFINDCOMPONENT_PROGID, finder );
    } else {
        #ifdef NS_DEBUG
            printf( "%s %d: GetService failed for find component, rv=0x08%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

NS_IMETHODIMP    
nsBrowserAppCore::FindNext()
{
    nsresult rv = NS_OK;
    PRBool   found = PR_FALSE;

    // Get find component.
    nsIFindComponent *finder;
    rv = nsServiceManager::GetService( NS_IFINDCOMPONENT_PROGID,
                                       nsIFindComponent::GetIID(),
                                       (nsISupports**)&finder );
    if ( NS_SUCCEEDED( rv ) ) {
        // Make sure we've initialized searching for this document.
        InitializeSearch( finder );

        // Perform find via find component.
        if ( finder && mSearchContext ) {
            rv = finder->FindNext( mSearchContext, &found );
        }

        // Release the service.
        nsServiceManager::ReleaseService( NS_IFINDCOMPONENT_PROGID, finder );
    } else {
        #ifdef NS_DEBUG
            printf( "%s %d: GetService failed for find component, rv=0x08%X\n",
                    __FILE__, (int)__LINE__, (int)rv );
        #endif
    }

    return rv;
}

// XXXbe why is this needed?  eliminate
NS_IMETHODIMP    
nsBrowserAppCore::ExecuteScript(nsIScriptContext * aContext, const nsString& aScript)
{
  if (nsnull != aContext) {
    const char* url = "";
    PRBool isUndefined = PR_FALSE;
    nsString rVal;
    if (APP_DEBUG) {
      printf("Executing [%s]\n", (const char *)nsAutoCString(aScript));
    }
    aContext->EvaluateString(aScript, url, 0, nsnull, rVal, &isUndefined);
  } 
  return NS_OK;
}




//----------------------------------------------------------------------



static nsresult
FindNamedXULElement(nsIWebShell * aShell,
                              const char *aId,
                              nsCOMPtr<nsIDOMElement> * aResult ) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIContentViewer> cv;
    rv = aShell ? aShell->GetContentViewer(getter_AddRefs(cv))
               : NS_ERROR_NULL_POINTER;
    if ( cv ) {
        // Up-cast.
        nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
        if ( docv ) {
            // Get the document from the doc viewer.
            nsCOMPtr<nsIDocument> doc;
            rv = docv->GetDocument(*getter_AddRefs(doc));
            if ( doc ) {
                // Up-cast.

                nsCOMPtr<nsIDOMXULDocument> xulDoc( do_QueryInterface(doc) );
                  if ( xulDoc ) {
                    // Find specified element.
                    nsCOMPtr<nsIDOMElement> elem;

                    rv = xulDoc->GetElementById( aId, getter_AddRefs(elem) );
                    if ( elem ) {
			*aResult =  elem;
                    } else {
                       if (APP_DEBUG) printf("GetElementByID failed, rv=0x%X\n",(int)rv);
                    }
                } else {
                  if (APP_DEBUG)   printf("Upcast to nsIDOMXULDocument failed\n");
                }

            } else {
               if (APP_DEBUG)  printf("GetDocument failed, rv=0x%X\n",(int)rv);
            }
        } else {
             if (APP_DEBUG)  printf("Upcast to nsIDocumentViewer failed\n");
        }
    } else {
       if (APP_DEBUG) printf("GetContentViewer failed, rv=0x%X\n",(int)rv);
    }
    return rv;
}

static const char *prefix = "component://netscape/appshell/component/browser/window";

void
nsBrowserAppCore::BeginObserving() {
    // Get observer service.
    nsIObserverService *svc = 0;
    nsresult rv = nsServiceManager::GetService( NS_OBSERVERSERVICE_PROGID,
                                                nsIObserverService::GetIID(),
                                                (nsISupports**)&svc );
    if ( NS_SUCCEEDED( rv ) && svc ) {
        // Add/Remove object as observer of web shell window topics.
        nsAutoString topic1(prefix);
        topic1 += ";status";
        nsAutoString topic2(prefix);
        topic2 += ";progress";
        rv = svc->AddObserver( this, topic1.GetUnicode() );
        rv = svc->AddObserver( this, topic2.GetUnicode() );
        // Release the service.
        nsServiceManager::ReleaseService( NS_OBSERVERSERVICE_PROGID, svc );
    }

    return;
}

void
nsBrowserAppCore::EndObserving() {
    // Get observer service.
    nsIObserverService *svc = 0;
    nsresult rv = nsServiceManager::GetService( NS_OBSERVERSERVICE_PROGID,
                                                nsIObserverService::GetIID(),
                                                (nsISupports**)&svc );
    if ( NS_SUCCEEDED( rv ) && svc ) {
        // Add/Remove object as observer of web shell window topics.
        nsAutoString topic1(prefix);
        topic1 += ";status";
        nsAutoString topic2(prefix);
        topic2 += ";progress";
        rv = svc->RemoveObserver( this, topic1.GetUnicode() );
        rv = svc->RemoveObserver( this, topic2.GetUnicode() );
        // Release the service.
        nsServiceManager::ReleaseService( NS_OBSERVERSERVICE_PROGID, svc );
    }

    return;
}

NS_IMETHODIMP
nsBrowserAppCore::Observe( nsISupports *aSubject,
                           const PRUnichar *aTopic,
                           const PRUnichar *someData ) {
    nsresult rv = NS_OK;

    // We only are interested if aSubject is our web shell window.
    if ( aSubject && mWebShellWin ) {
        nsIWebShellWindow *window = 0;
        rv = aSubject->QueryInterface( nsIWebShellWindow::GetIID(), (void**)&window );
        if ( NS_SUCCEEDED( rv ) && window ) {
            nsString topic1 = prefix;
            topic1 += ";status";
            nsString topic2 = prefix;
            topic2 += ";progress";
            // Compare to our window.
            if ( window == mWebShellWin ) {
                // Get topic substring.
                if ( topic1 == aTopic ) {
                    // Update status text.
                    nsAutoString v(someData);
                    rv = setAttribute( mWebShell, "Browser:Status", "value", v );
                } else if ( topic2 == aTopic ) {
                    // We don't process this, yet.
                }
            } else {
                // Not for this app core.
            }
            // Release the window.
            window->Release();
        }
    }

    return rv;
}

NS_IMETHODIMP
nsBrowserAppCore::SelectAll()
{
  nsresult rv;
  nsCOMPtr<nsIClipboardCommands> clip(do_QueryInterface(mContentAreaWebShell,&rv));
  if ( NS_SUCCEEDED(rv) ) {
      rv = clip->SelectAll();
  }

  return rv;
}

NS_IMETHODIMP
nsBrowserInstance::GetIsViewSource(PRBool *aBool) {
    nsresult rv = NS_OK;
    if ( aBool ) {
        *aBool = mIsViewSource;
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }
    return rv;
}

NS_IMETHODIMP
nsBrowserInstance::SetIsViewSource(PRBool aBool) {
    nsresult rv = NS_OK;
    mIsViewSource = aBool;
    return rv;
}

// nsIURIContentListener support

NS_IMETHODIMP 
nsBrowserInstance::GetProtocolHandler(nsIURI * /* aURI */, nsIProtocolHandler **aProtocolHandler)
{
   // we don't have any app specific protocol handlers we want to use so 
  // just use the system default by returning null.
  *aProtocolHandler = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
nsBrowserInstance::DoContent(const char *aContentType, const char *aCommand, const char *aWindowTarget, 
                             nsIChannel *aChannel, nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
  // forward the DoContent call to our content area webshell
  nsCOMPtr<nsIURIContentListener> ctnListener = do_QueryInterface(mContentAreaWebShell);
  if (ctnListener)
    return ctnListener->DoContent(aContentType, aCommand, aWindowTarget, aChannel, aContentHandler, aAbortProcess);
  return NS_OK;
}

NS_IMETHODIMP 
nsBrowserInstance::CanHandleContent(const char * aContentType,
                                    const char * aCommand,
                                    const char * aWindowTarget,
                                    char ** aDesiredContentType,
                                    PRBool * aCanHandleContent)

{
  // need to list all the content types the browser window knows how to handle here:
  if (aContentType)
  {
     if (nsCRT::strcasecmp(aContentType, "multipart/x-mixed-replace") == 0)
     {
       *aDesiredContentType = nsCRT::strdup("text/html");
       *aCanHandleContent = PR_TRUE;
     }
     if (nsCRT::strcasecmp(aContentType, "text/html") == 0)
       *aCanHandleContent = PR_TRUE;
        
  }
  else
    *aCanHandleContent = PR_FALSE;

  return NS_OK;
}


NS_DEFINE_MODULE_INSTANCE_COUNTER()

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsBrowserInstance, Init)

static nsModuleComponentInfo components[] = {
  { "nsBrowserInstance",
    NS_BROWSERINSTANCE_CID,
    NS_IBROWSERINSTANCE_PROGID, 
    nsBrowserInstanceConstructor }
};

NS_IMPL_NSGETMODULE("nsBrowserModule", components)
