/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */


#include "nsWebShellWindow.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIPref.h"

#include "nsINameSpaceManager.h"
#include "nsVoidArray.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"

#include "nsGUIEvent.h"
#include "nsWidgetsCID.h"
#include "nsIWidget.h"
#include "nsIAppShell.h"
#include "nsIXULWindowCallbacks.h"

#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"

#include "nsXULCommand.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMNodeList.h"

#include "nsIMenuBar.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"
#include "nsIMenuListener.h"
#include "nsIContextMenu.h"

// For JS Execution
#include "nsIScriptContextOwner.h"

#include "nsXPComCIID.h"
#include "nsIEventQueueService.h"
#include "plevent.h"
#include "prmem.h"

#include "nsIDocumentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDocumentLoader.h"
//#include "nsIDOMHTMLInputElement.h"
//#include "nsIDOMHTMLImageElement.h"

#include "nsIContent.h" // for menus

// For calculating size
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"

// HACK for M4, should be removed by M5
#ifdef XP_MAC
#include <Menus.h>
#endif
		

/* Define Class IDs */
static NS_DEFINE_IID(kWindowCID,           NS_WINDOW_CID);
static NS_DEFINE_IID(kWebShellCID,         NS_WEB_SHELL_CID);
static NS_DEFINE_IID(kAppShellServiceCID,  NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kAppShellCID,         NS_APPSHELL_CID);

#include "nsWidgetsCID.h"
static NS_DEFINE_IID(kMenuBarCID,          NS_MENUBAR_CID);
static NS_DEFINE_IID(kMenuCID,             NS_MENU_CID);
static NS_DEFINE_IID(kMenuItemCID,         NS_MENUITEM_CID);
static NS_DEFINE_IID(kContextMenuCID,      NS_CONTEXTMENU_CID);

static NS_DEFINE_CID(kPrefCID,             NS_PREF_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);


/* Define Interface IDs */
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIWebShellWindowIID,     NS_IWEBSHELL_WINDOW_IID);
static NS_DEFINE_IID(kIWidgetIID,             NS_IWIDGET_IID);
static NS_DEFINE_IID(kIWebShellIID,           NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIWebShellContainerIID,  NS_IWEB_SHELL_CONTAINER_IID);
static NS_DEFINE_IID(kIBrowserWindowIID,		  NS_IBROWSER_WINDOW_IID);

static NS_DEFINE_IID(kIAppShellServiceIID,    NS_IAPPSHELL_SERVICE_IID);
static NS_DEFINE_IID(kIAppShellIID,           NS_IAPPSHELL_IID);
static NS_DEFINE_IID(kIDocumentLoaderObserverIID, NS_IDOCUMENT_LOADER_OBSERVER_IID);
static NS_DEFINE_IID(kIDocumentViewerIID,     NS_IDOCUMENT_VIEWER_IID);
static NS_DEFINE_IID(kIDOMDocumentIID,        NS_IDOMDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMNodeIID,            NS_IDOMNODE_IID);
static NS_DEFINE_IID(kIDOMElementIID,         NS_IDOMELEMENT_IID);
static NS_DEFINE_IID(kIDOMCharacterDataIID,   NS_IDOMCHARACTERDATA_IID);
//static NS_DEFINE_IID(kIDOMHTMLInputElementIID, NS_IDOMHTMLINPUTELEMENT_IID);
//static NS_DEFINE_IID(kIDOMHTMLImageElementIID, NS_IDOMHTMLIMAGEELEMENT_IID);

static NS_DEFINE_IID(kIMenuIID,        NS_IMENU_IID);
static NS_DEFINE_IID(kIMenuBarIID,     NS_IMENUBAR_IID);
static NS_DEFINE_IID(kIMenuItemIID,    NS_IMENUITEM_IID);
static NS_DEFINE_IID(kIContextMenuIID, NS_ICONTEXTMENU_IID);
static NS_DEFINE_IID(kIXULCommandIID,  NS_IXULCOMMAND_IID);
static NS_DEFINE_IID(kIContentIID,     NS_ICONTENT_IID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);

#ifdef DEBUG_rods
#define DEBUG_MENUSDEL 1
#endif

#include "nsIWebShell.h"

const char * kThrobberOnStr  = "resource:/res/throbber/anims07.gif";
const char * kThrobberOffStr = "resource:/res/throbber/anims00.gif";

struct ThreadedWindowEvent {
  PLEvent           event;
  nsWebShellWindow  *window;
};

// The web shell info object is used to hold information about content areas that will
// subsequently be filled in when we receive a webshell added notification.
struct nsWebShellInfo {
  nsString id; // The identifier of the iframe or frame node in the XUL tree.
  nsIWebShell* child; // The child web shell that will end up being used for the content area.

  nsWebShellInfo(const nsString& anID, nsIWebShell* aChildShell)
  {
    id = anID; 
    child = aChildShell;
    NS_IF_ADDREF(aChildShell);
  }

  ~nsWebShellInfo()
  {
    NS_IF_RELEASE(child);
  }
};

nsWebShellWindow::nsWebShellWindow()
{
  NS_INIT_REFCNT();

  mWebShell = nsnull;
  mWindow   = nsnull;
  mCallbacks = nsnull;
  mContinueModalLoop = PR_FALSE;
  mChromeInitialized = PR_FALSE;
	mLockedUntilChromeLoad = PR_FALSE;
  mContentShells = nsnull;
}


nsWebShellWindow::~nsWebShellWindow()
{
  if (nsnull != mWebShell) {
    mWebShell->Destroy();
    NS_RELEASE(mWebShell);
  }

  NS_IF_RELEASE(mWindow);
  NS_IF_RELEASE(mCallbacks);

  // Delete any remaining content shells.
  PRInt32 count;
  if (mContentShells && ((count = mContentShells->Count()) > 0)) {
    for (PRInt32 i = 0; i < count; i++)
    {
      nsWebShellInfo* webInfo = (nsWebShellInfo*)(mContentShells->ElementAt(i));
      delete webInfo;
    }

    delete mContentShells;
  }

}


NS_IMPL_ADDREF(nsWebShellWindow);
NS_IMPL_RELEASE(nsWebShellWindow);

nsresult
nsWebShellWindow::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv = NS_NOINTERFACE;

  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if ( aIID.Equals(kIWebShellWindowIID) ) {
    *aInstancePtr = (void*) ((nsIWebShellWindow*)this);
    NS_ADDREF_THIS();  
    return NS_OK;
  }
  if (aIID.Equals(kIWebShellContainerIID)) {
    *aInstancePtr = (void*)(nsIWebShellContainer*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIDocumentLoaderObserverIID)) {
    *aInstancePtr = (void*) ((nsIDocumentLoaderObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
	if ( aIID.Equals(kIBrowserWindowIID) ) {
    *aInstancePtr = (void*) (nsIBrowserWindow*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)(nsIWebShellContainer*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return rv;
}


nsresult nsWebShellWindow::Initialize(nsIWebShellWindow* aParent,
                                      nsIAppShell* aShell, nsIURL* aUrl, 
                                      nsIStreamObserver* anObserver,
                                      nsIXULWindowCallbacks *aCallbacks,
                                      PRInt32 aInitialWidth, PRInt32 aInitialHeight)
{
  nsresult rv;

  nsString urlString;
  nsIWidget *parentWidget;
  const char *tmpStr = NULL;
  
  // XXX: need to get the default window size from prefs...
	// Doesn't come from prefs... will come from CSS/XUL/RDF
  nsRect r(0, 0, aInitialWidth, aInitialHeight);
  
  nsWidgetInitData initData;


  //if (nsnull == aUrl) {
  //  rv = NS_ERROR_NULL_POINTER;
  //  goto done;
  //}
  if (nsnull != aUrl)  {
    aUrl->GetSpec(&tmpStr);
    urlString = tmpStr;
  }

  // Create top level window
  rv = nsComponentManager::CreateInstance(kWindowCID, nsnull, kIWidgetIID,
                                    (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }

  initData.mBorderStyle = eBorderStyle_window;

  if (!aParent || NS_FAILED(aParent->GetWidget(parentWidget)))
    parentWidget = nsnull;

  mWindow->SetClientData(this);
  mWindow->Create(parentWidget,                       // Parent nsIWidget
                  r,                                  // Widget dimensions
                  nsWebShellWindow::HandleEvent,      // Event handler function
                  nsnull,                             // Device context
                  aShell,                             // Application shell
                  nsnull,                             // nsIToolkit
                  &initData);                         // Widget initialization data
  mWindow->GetClientBounds(r);
  mWindow->SetBackgroundColor(NS_RGB(192,192,192));

  // Create web shell
  rv = nsComponentManager::CreateInstance(kWebShellCID, nsnull,
                                    kIWebShellIID,
                                    (void**)&mWebShell);
  if (NS_OK != rv) {
    return rv;
  }

  r.x = r.y = 0;
  rv = mWebShell->Init(mWindow->GetNativeData(NS_NATIVE_WIDGET), 
                       r.x, r.y, r.width, r.height,
                       nsScrollPreference_kNeverScroll, 
                       PR_TRUE,                     // Allow Plugins 
                       PR_TRUE);
  mWebShell->SetContainer(this);
  mWebShell->SetObserver((nsIStreamObserver*)anObserver);
  mWebShell->SetDocLoaderObserver(this);

	// The outermost web shell is always considered to be chrome.
	mWebShell->SetWebShellType(nsWebShellChrome);

  /*
   * XXX:  How should preferences be supplied to the nsWebShellWindow?
   *       Should there be the notion of a global preferences service?
   *       Or should there be many preferences components based on 
   *       the user profile...
   */
  // Initialize the webshell with the preferences service
  nsIPref *prefs;

  rv = nsServiceManager::GetService(kPrefCID, 
                                    nsIPref::GetIID(), 
                                    (nsISupports **)&prefs);
  if (NS_SUCCEEDED(rv)) {
    mWebShell->SetPrefs(prefs);
    nsServiceManager::ReleaseService(kPrefCID, prefs);
  }

  NS_IF_RELEASE(mCallbacks);
  mCallbacks = aCallbacks;
  NS_IF_ADDREF(mCallbacks);

  if (nsnull != aUrl)  {
    mWebShell->LoadURL(urlString.GetUnicode());
  }
                     
  return rv;
}


/*
 * Close the window
 */
NS_METHOD
nsWebShellWindow::Close()
{
  ExitModalLoop();
  if ( mWebShell ) {
    mWebShell->Destroy();
    NS_IF_RELEASE(mWebShell);
  }

  NS_IF_RELEASE(mWindow);
  nsIAppShellService* appShell;
  nsresult rv = nsServiceManager::GetService(kAppShellServiceCID,
                                  kIAppShellServiceIID,
                                  (nsISupports**)&appShell);
  if (NS_FAILED(rv))
    return rv;
  
  rv = appShell->UnregisterTopLevelWindow(this);
  nsServiceManager::ReleaseService(kAppShellServiceCID, appShell);

  return rv;
}


/*
 * Event handler function...
 *
 * This function is called to process events for the nsIWidget of the 
 * nsWebShellWindow...
 */
nsEventStatus PR_CALLBACK
nsWebShellWindow::HandleEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  nsIWebShell* webShell = nsnull;

  // Get the WebShell instance...
  if (nsnull != aEvent->widget) {
    void* data;

    aEvent->widget->GetClientData(data);
    if (nsnull != data) {
      webShell = ((nsWebShellWindow*)data)->mWebShell;
    }
  }

  if (nsnull != webShell) {
    switch(aEvent->message) {
      /*
       * For size events, the WebShell must be resized to fill the entire
       * client area of the window...
       */
      case NS_SIZE: {
        nsSizeEvent* sizeEvent = (nsSizeEvent*)aEvent;  
        webShell->SetBounds(0, 0, sizeEvent->windowSize->width, sizeEvent->windowSize->height);
        result = nsEventStatus_eConsumeNoDefault;
        break;
      }
      /*
       * Notify the ApplicationShellService that the window is being closed...
       */
      case NS_DESTROY: {
        void* data;
        aEvent->widget->GetClientData(data);
        if (data)
           ((nsWebShellWindow *)data)->Close();
        break;
      }
    }
  }
  return nsEventStatus_eIgnore;
}


NS_IMETHODIMP 
nsWebShellWindow::WillLoadURL(nsIWebShell* aShell, const PRUnichar* aURL,
                              nsLoadType aReason)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::BeginLoadURL(nsIWebShell* aShell, const PRUnichar* aURL)
{
  // If loading a new root .xul document, then redo chrome.
  if ( aShell == mWebShell ) {
      mChromeInitialized = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::ProgressLoadURL(nsIWebShell* aShell, const PRUnichar* aURL,
                                  PRInt32 aProgress, PRInt32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::EndLoadURL(nsIWebShell* aWebShell, const PRUnichar* aURL,
                             PRInt32 aStatus)
{
  return NS_OK;
}



//----------------------------------------
nsCOMPtr<nsIDOMNode> nsWebShellWindow::FindNamedParentFromDoc(nsIDOMDocument * aDomDoc, const nsString &aName) 
{
  nsCOMPtr<nsIDOMNode> node; // result.
  nsCOMPtr<nsIDOMElement> element;
  aDomDoc->GetDocumentElement(getter_AddRefs(element));
  if (!element)
    return node;

  nsCOMPtr<nsIDOMNode> parent(do_QueryInterface(element));
  if (!parent)
    return node;

  parent->GetFirstChild(getter_AddRefs(node));
  while (node) {
    nsString name;
    node->GetNodeName(name);

#ifdef DEBUG_rods
    printf("Looking for [%s] [%s]\n", aName.ToNewCString(), name.ToNewCString()); // this leaks
#endif

    if (name.Equals(aName))
      return node;
    nsCOMPtr<nsIDOMNode> oldNode(node);
    oldNode->GetNextSibling(getter_AddRefs(node));
  }
  node = do_QueryInterface(nsnull);
  return node;
}


//----------------------------------------
NS_IMETHODIMP nsWebShellWindow::CreateMenu(nsIMenuBar * aMenuBar, 
                                           nsIDOMNode * aMenuNode, 
                                           nsString   & aMenuName) 
{
  // Create nsMenu
  nsIMenu * pnsMenu = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuCID, nsnull, kIMenuIID, (void**)&pnsMenu);
  if (NS_OK == rv) {
    // Call Create
    nsISupports * supports = nsnull;
    aMenuBar->QueryInterface(kISupportsIID, (void**) &supports);
    pnsMenu->Create(supports, aMenuName);
    NS_RELEASE(supports);

    // Set nsMenu Name
    pnsMenu->SetLabel(aMenuName); 
    // Make nsMenu a child of nsMenuBar
    aMenuBar->AddMenu(pnsMenu); 

    // Begin menuitem inner loop
    nsCOMPtr<nsIDOMNode> menuitemNode;
    aMenuNode->GetFirstChild(getter_AddRefs(menuitemNode));
    while (menuitemNode) {
      nsCOMPtr<nsIDOMElement> menuitemElement(do_QueryInterface(menuitemNode));
      if (menuitemElement) {
        nsString menuitemNodeType;
        nsString menuitemName;
        menuitemElement->GetNodeName(menuitemNodeType);
        if (menuitemNodeType.Equals("menuitem")) {
          // LoadMenuItem
          LoadMenuItem(pnsMenu, menuitemElement, menuitemNode);
        } else if (menuitemNodeType.Equals("separator")) {
          pnsMenu->AddSeparator();
        } else if (menuitemNodeType.Equals("menu")) {
          // Load a submenu
          LoadSubMenu(pnsMenu, menuitemElement, menuitemNode);
        }
      }
      nsCOMPtr<nsIDOMNode> oldmenuitemNode(menuitemNode);
      oldmenuitemNode->GetNextSibling(getter_AddRefs(menuitemNode));
    } // end menu item innner loop
    // The parent owns us, so we can release
    NS_RELEASE(pnsMenu);
  }

  return NS_OK;
}

//----------------------------------------
NS_IMETHODIMP nsWebShellWindow::LoadMenuItem(
  nsIMenu *    pParentMenu,
  nsIDOMElement * menuitemElement,
  nsIDOMNode *    menuitemNode)
{
  nsString menuitemName;
  nsString menuitemCmd;

  menuitemElement->GetAttribute(nsAutoString("name"), menuitemName);
  menuitemElement->GetAttribute(nsAutoString("cmd"), menuitemCmd);
  // Create nsMenuItem
  nsIMenuItem * pnsMenuItem = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuItemCID, nsnull, kIMenuItemIID, (void**)&pnsMenuItem);
  if (NS_OK == rv) {
    pnsMenuItem->Create(pParentMenu, menuitemName, 0);                 
    // Set nsMenuItem Name
    //pnsMenuItem->SetLabel(menuitemName);
    // Make nsMenuItem a child of nsMenu
    //pParentMenu->AddMenuItem(pnsMenuItem);
    nsISupports * supports = nsnull;
    pnsMenuItem->QueryInterface(kISupportsIID, (void**) &supports);
    pParentMenu->AddItem(supports);
    NS_RELEASE(supports);
          
    // Create MenuDelegate - this is the intermediator inbetween 
    // the DOM node and the nsIMenuItem
    // The nsWebShellWindow wacthes for Document changes and then notifies the 
    // the appropriate nsMenuDelegate object
    nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(menuitemNode));
    if (!domElement) {
      return NS_ERROR_FAILURE;
    }

    nsAutoString cmdAtom("onclick");
    nsString cmdName;

    domElement->GetAttribute(cmdAtom, cmdName);

    nsXULCommand * menuDelegate = new nsXULCommand();
    menuDelegate->SetCommand(cmdName);
    menuDelegate->SetWebShell(mWebShell);
    menuDelegate->SetDOMElement(domElement);
    menuDelegate->SetMenuItem(pnsMenuItem);
    nsIXULCommand * icmd;
    if (NS_OK == menuDelegate->QueryInterface(kIXULCommandIID, (void**) &icmd)) {
      mMenuDelegates.AppendElement(icmd);
      nsCOMPtr<nsIMenuListener> listener(do_QueryInterface(menuDelegate));

      if (listener) 
      {
        pnsMenuItem->AddMenuListener(listener);
        
#ifdef DEBUG_MENUSDEL
        printf("Adding menu listener to [%s]\n", menuitemName.ToNewCString());
#endif
      } 
#ifdef DEBUG_MENUSDEL
      else 
      {
        printf("*** NOT Adding menu listener to [%s]\n", menuitemName.ToNewCString());
      }
#endif
      NS_RELEASE(icmd);
    }
    
    // The parent owns us, so we can release
    NS_RELEASE(pnsMenuItem);
  } 
  return NS_OK;
}

//----------------------------------------
void nsWebShellWindow::LoadSubMenu(
  nsIMenu *       pParentMenu,
  nsIDOMElement * menuElement,
  nsIDOMNode *    menuNode)
{
  nsString menuName;
  menuElement->GetAttribute(nsAutoString("name"), menuName);
  //printf("Creating Menu [%s] \n", menuName.ToNewCString()); // this leaks

  // Create nsMenu
  nsIMenu * pnsMenu = nsnull;
  nsresult rv = nsComponentManager::CreateInstance(kMenuCID, nsnull, kIMenuIID, (void**)&pnsMenu);
  if (NS_OK == rv) {
    // Call Create
    nsISupports * supports = nsnull;
    pParentMenu->QueryInterface(kISupportsIID, (void**) &supports);
    pnsMenu->Create(supports, menuName);
    NS_RELEASE(supports); // Balance QI

    // Set nsMenu Name
    pnsMenu->SetLabel(menuName); 
    // Make nsMenu a child of parent nsMenu
    //pParentMenu->AddMenu(pnsMenu);
    supports = nsnull;
    pnsMenu->QueryInterface(kISupportsIID, (void**) &supports);
	pParentMenu->AddItem(supports);
	NS_RELEASE(supports);

    // Begin menuitem inner loop
    nsCOMPtr<nsIDOMNode> menuitemNode;
    menuNode->GetFirstChild(getter_AddRefs(menuitemNode));
    while (menuitemNode) {
      nsCOMPtr<nsIDOMElement> menuitemElement(do_QueryInterface(menuitemNode));
      if (menuitemElement) {
        nsString menuitemNodeType;
        menuitemElement->GetNodeName(menuitemNodeType);

#ifdef DEBUG_saari
        printf("Type [%s] %d\n", menuitemNodeType.ToNewCString(), menuitemNodeType.Equals("separator"));
#endif

        if (menuitemNodeType.Equals("menuitem")) {
          // Load a menuitem
          LoadMenuItem(pnsMenu, menuitemElement, menuitemNode);
        } else if (menuitemNodeType.Equals("separator")) {
          pnsMenu->AddSeparator();
        } else if (menuitemNodeType.Equals("menu")) {
          // Add a submenu
          LoadSubMenu(pnsMenu, menuitemElement, menuitemNode);
        }
      }
      nsCOMPtr<nsIDOMNode> oldmenuitemNode(menuitemNode);
      oldmenuitemNode->GetNextSibling(getter_AddRefs(menuitemNode));
    } // end menu item innner loop
    
    // The parent owns us, so we can release
    NS_RELEASE(pnsMenu);
  }     
}

//----------------------------------------
void nsWebShellWindow::DynamicLoadMenus(nsIDOMDocument * aDOMDoc, nsIWidget * aParentWindow) 
{
  // locate the window element which holds toolbars and menus and commands
  nsCOMPtr<nsIDOMElement> element;
  aDOMDoc->GetDocumentElement(getter_AddRefs(element));
  if (!element) {
    return;
  }
  nsCOMPtr<nsIDOMNode> window(do_QueryInterface(element));

  nsresult rv;
  int endCount = 0;
  nsCOMPtr<nsIDOMNode> menubarNode(FindNamedDOMNode(nsAutoString("menubar"), window, endCount, 1));
  if (menubarNode) {
    nsIMenuBar * pnsMenuBar = nsnull;
    rv = nsComponentManager::CreateInstance(kMenuBarCID, nsnull, kIMenuBarIID, (void**)&pnsMenuBar);
    if (NS_OK == rv) {
      if (nsnull != pnsMenuBar) {      
        // set pnsMenuBar as a nsMenuListener on aParentWindow
        nsCOMPtr<nsIMenuListener> menuListener;
        pnsMenuBar->QueryInterface(kIMenuListenerIID, getter_AddRefs(menuListener));

        //fake event
        nsMenuEvent fake;
        menuListener->MenuConstruct(fake, aParentWindow, menubarNode, mWebShell);

      // XXX ok this is somewhat of a kludge but it is needed. When the menu bar is added the client area got smaller
      // unfortunately the document will already have been flowed. So we need to reflow it to a smaller size. -EDV
      // BEGIN REFLOW CODE
      nsresult rv = NS_ERROR_FAILURE;

      // do a reflow
      nsCOMPtr<nsIContentViewerContainer> contentViewerContainer;
      contentViewerContainer = do_QueryInterface(mWebShell);
      if (!contentViewerContainer) {
          NS_ERROR("Webshell doesn't support the content viewer container interface");
          return;
      }

      nsCOMPtr<nsIContentViewer> contentViewer;
      if (NS_FAILED(rv = contentViewerContainer->GetContentViewer(getter_AddRefs(contentViewer)))) {
          NS_ERROR("Unable to retrieve content viewer.");
          return;
      }

      nsCOMPtr<nsIDocumentViewer> docViewer;
      docViewer = do_QueryInterface(contentViewer);
      if (!docViewer) {
          NS_ERROR("Document viewer interface not supported by the content viewer.");
          return;
      }

      nsCOMPtr<nsIPresContext> presContext;
      if (NS_FAILED(rv = docViewer->GetPresContext(*getter_AddRefs(presContext)))) {
          NS_ERROR("Unable to retrieve the doc viewer's presentation context.");
          return;
      }

      nsCOMPtr<nsIPresShell> presShell;
      if (NS_FAILED(rv = presContext->GetShell(getter_AddRefs(presShell)))) {
          NS_ERROR("Unable to retrieve the shell from the presentation context.");
          return;
      }


      nsRect rect;

      if (NS_FAILED(rv = mWindow->GetClientBounds(rect))) {
          NS_ERROR("Failed to get web shells bounds");
          return;
      }

      // convert to twips
      float p2t;
      presContext->GetScaledPixelsToTwips(&p2t);
      rect.width = NSIntPixelsToTwips(rect.width, p2t);
      rect.height = NSIntPixelsToTwips(rect.height, p2t);
    
      if (NS_FAILED(rv = presShell->ResizeReflow(rect.width,rect.height))) {
          NS_ERROR("Failed to reflow the document after the menu was added");
          return;
      }
      // END REFLOW CODE
                  
      } // end if ( nsnull != pnsMenuBar )
    }
  } // end if (menuBar)
} // nsWebShellWindow::LoadMenus

//----------------------------------------
void nsWebShellWindow::LoadMenus(nsIDOMDocument * aDOMDoc, nsIWidget * aParentWindow) 
{
  // locate the window element which holds toolbars and menus and commands
  nsCOMPtr<nsIDOMElement> element;
  aDOMDoc->GetDocumentElement(getter_AddRefs(element));
  if (!element) {
    return;
  }
  nsCOMPtr<nsIDOMNode> window(do_QueryInterface(element));

  nsresult rv;
  int endCount = 0;
  nsCOMPtr<nsIDOMNode> menubarNode(FindNamedDOMNode(nsAutoString("menubar"), window, endCount, 1));
  if (menubarNode) {
    nsIMenuBar * pnsMenuBar = nsnull;
    rv = nsComponentManager::CreateInstance(kMenuBarCID, nsnull, kIMenuBarIID, (void**)&pnsMenuBar);
    if (NS_OK == rv) {
      if (nsnull != pnsMenuBar) {
        pnsMenuBar->Create(aParentWindow);
      
        // set pnsMenuBar as a nsMenuListener on aParentWindow
        nsCOMPtr<nsIMenuListener> menuListener;
        pnsMenuBar->QueryInterface(kIMenuListenerIID, getter_AddRefs(menuListener));
        aParentWindow->AddMenuListener(menuListener);

        nsCOMPtr<nsIDOMNode> menuNode;
        menubarNode->GetFirstChild(getter_AddRefs(menuNode));
        while (menuNode) {
          nsCOMPtr<nsIDOMElement> menuElement(do_QueryInterface(menuNode));
          if (menuElement) {
            nsString menuNodeType;
            nsString menuName;
            menuElement->GetNodeName(menuNodeType);
            if (menuNodeType.Equals("menu")) {
              menuElement->GetAttribute(nsAutoString("name"), menuName);

#ifdef DEBUG_rods
              printf("Creating Menu [%s] \n", menuName.ToNewCString()); // this leaks
#endif
              CreateMenu(pnsMenuBar, menuNode, menuName);
            } 

          }
          nsCOMPtr<nsIDOMNode> oldmenuNode(menuNode);  
          oldmenuNode->GetNextSibling(getter_AddRefs(menuNode));
        } // end while (nsnull != menuNode)
          
        // Give the aParentWindow this nsMenuBar to own.
        aParentWindow->SetMenuBar(pnsMenuBar);
      
        // HACK: force a paint for now
        pnsMenuBar->Paint();
        
        // HACK for M4, should be removed by M5
        #ifdef XP_MAC
        Handle tempMenuBar = ::GetMenuBar(); // Get a copy of the menu list
		pnsMenuBar->SetNativeData((void*)tempMenuBar);
		#endif

        // The parent owns the menubar, so we can release it		
		NS_RELEASE(pnsMenuBar);
    } // end if ( nsnull != pnsMenuBar )
    }
  } // end if (menuBar)

} // nsWebShellWindow::LoadMenus

//------------------------------------------------------------------------------
void nsWebShellWindow::DoContextMenu(
  nsMenuEvent * aMenuEvent,
  nsIDOMNode  * aMenuNode, 
  nsIWidget   * aParentWindow,
  PRInt32       aX,
  PRInt32       aY) 
{
  if (aMenuNode) {
    nsIContextMenu * pnsContextMenu;
    nsresult rv = nsComponentManager::CreateInstance(kContextMenuCID, nsnull, kIContextMenuIID, (void**)&pnsContextMenu);
    if (NS_SUCCEEDED(rv) && pnsContextMenu) {
        nsISupports * supports;
        aParentWindow->QueryInterface(kISupportsIID, (void**) &supports);
        pnsContextMenu->Create(supports);
        NS_RELEASE(supports);
        pnsContextMenu->SetLocation(aX,aY);
        // Set webshell
        pnsContextMenu->SetWebShell( mWebShell );
        
        // Set DOM node
        pnsContextMenu->SetDOMNode( aMenuNode );
        
        // Construct and show menu
        nsIMenuListener * listener;
        pnsContextMenu->QueryInterface(kIMenuListenerIID, (void**) &listener);
        
        // Dynamically construct and track the menu
        listener->MenuSelected(*aMenuEvent);
        
        // Destroy the menu
        listener->MenuDeselected(*aMenuEvent); 
        
        // The parent owns the context menu, so we can release it	
        NS_RELEASE(listener);	
		NS_RELEASE(pnsContextMenu);
    }
  } // end if (aMenuNode)
}

//------------------------------------------------------------------------------
NS_IMETHODIMP
nsWebShellWindow::GetContentShellById(const nsString& aID, nsIWebShell** aChildShell)
{
	// Set to null just to be certain
  *aChildShell = nsnull;

  // If we don't have a content array, we just don't care.
  if (mContentShells == nsnull)
    return NS_ERROR_FAILURE;

  // Find out if the id in question is one that we have web shell info for.
  PRInt32 count = mContentShells->Count();
  for (PRInt32 i = 0; i < count; i++)
  {
    nsWebShellInfo* webInfo = (nsWebShellInfo*)(mContentShells->ElementAt(i));
    if (webInfo->id == aID)
    {
      // We have a match!
      *aChildShell = webInfo->child;
      NS_ADDREF(*aChildShell);

      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

//------------------------------------------------------------------------------
NS_IMETHODIMP
nsWebShellWindow::AddWebShellInfo(const nsString& aID,
                                  nsIWebShell* aChildShell)
{

  nsWebShellInfo* webShellInfo = new nsWebShellInfo(aID, 
                                                    aChildShell);
  
  if (mContentShells == nsnull)
    mContentShells = new nsVoidArray();

  mContentShells->AppendElement((void*)webShellInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ConvertWebShellToDOMWindow(nsIWebShell* aShell, nsIDOMWindow** aDOMWindow)
{
  nsresult rv;
  nsCOMPtr<nsIScriptContextOwner> newContextOwner;
  nsCOMPtr<nsIScriptGlobalObject> newGlobalObject;
  nsCOMPtr<nsIDOMWindow> newDOMWindow;

  newContextOwner = do_QueryInterface(aShell);
  if (newContextOwner)
  {
    if (NS_FAILED(rv = newContextOwner->GetScriptGlobalObject(getter_AddRefs(newGlobalObject)))) {
      NS_ERROR("Unable to retrieve global object.");
      return rv;
    }
    
    if (newGlobalObject) {
      newDOMWindow = do_QueryInterface(newGlobalObject);
      *aDOMWindow = newDOMWindow.get();
      NS_ADDREF(*aDOMWindow);
    }
    else return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::CreatePopup(nsIDOMElement* aElement, nsIDOMElement* aPopupContent, 
                              PRInt32 aXPos, PRInt32 aYPos, 
                              const nsString& aPopupType, const nsString& aPopupAlignment)
{
  nsresult rv = NS_OK;

  // Find out if we're a menu.
  nsCOMPtr<nsIDOMNodeList> menuNodes;
  if (NS_FAILED(rv = aElement->GetElementsByTagName("menu", getter_AddRefs(menuNodes)))) {
    NS_ERROR("Error occurred looking for nodes.");
    return rv;
  }

  // We got something.
  PRUint32 length;
  menuNodes->GetLength(&length);
  if (length > 0) {
    nsCOMPtr<nsIDOMNode> menuItem;
    menuNodes->Item(0, getter_AddRefs(menuItem));
    if (menuItem) {
      //nsCOMPtr<nsIDOMElement> menuElement = do_QueryInterface(menuItem);

      // XXX Call the context menu creation method
      DoContextMenu(
        nsnull,
        menuItem, 
        mWindow,
        aXPos,
        aYPos);
  
    }
    return NS_OK;
  }
  
  // XXX Handle the arbitrary popup XUL case.
  return rv;
}

NS_IMETHODIMP
nsWebShellWindow::ContentShellAdded(nsIWebShell* aChildShell, nsIContent* frameNode)
{
  // Find out the id of the frameNode in question 
  nsIAtom* idAtom = NS_NewAtom("id");
  nsAutoString value;
	frameNode->GetAttribute(kNameSpaceID_None, idAtom, value);
	AddWebShellInfo(value, aChildShell);
  NS_RELEASE(idAtom);
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::NewWebShell(PRUint32 aChromeMask, PRBool aVisible,
                              nsIWebShell *&aNewWebShell)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIWebShellWindow> newWindow;

  if ((aChromeMask & NS_CHROME_OPEN_AS_CHROME) != 0) {
    // Just do a nice normal create of a web shell and
    // return it immediately.  
    appShell->CreateTopLevelWindow(nsnull, nsnull, PR_FALSE, *getter_AddRefs(newWindow),
                                   nsnull, nsnull, 615, 480);
    newWindow->GetWebShell(aNewWebShell); // GetWebShell does the addref.
    return NS_OK;
  }

#ifdef XP_PC // XXX: Won't work on any other platforms yet. Sigh.
  // We need to create a new top level window and then enter a nested
  // loop. Eventually the new window will be told that it has loaded,
  // at which time we know it is safe to spin out of the nested loop
  // and allow the opening code to proceed.

  // First push a nested event queue for event processing from netlib
  // onto our UI thread queue stack.
  NS_WITH_SERVICE(nsIEventQueueService, eQueueService, kEventQueueServiceCID, &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to obtain queue service.");
    return rv;
  }
  eQueueService->PushThreadEventQueue();

  nsCOMPtr<nsIURL> urlObj;
  rv = NS_NewURL(getter_AddRefs(urlObj), "chrome://navigator/content/");
  if (NS_FAILED(rv))
    return rv;

  appShell->CreateTopLevelWindow(nsnull, urlObj, PR_FALSE, *getter_AddRefs(newWindow),
                                 nsnull, nsnull, 615, 480);

  // Spin into the modal loop.
  nsIAppShell *subshell;
  rv = nsComponentManager::CreateInstance(kAppShellCID, nsnull, kIAppShellIID, (void**)&subshell);
  if (NS_FAILED(rv))
    return rv;

  subshell->Create(0, nsnull);
  subshell->Spinup(); // Spin up 

  // Specify that we want the window to remain locked until the chrome has loaded.
  newWindow->LockUntilChromeLoad();

  PRBool locked = PR_FALSE;
  newWindow->GetLockedState(locked);
  while (NS_SUCCEEDED(rv) && locked) {
    void      *data;
    PRBool    isRealEvent;
    
    rv = subshell->GetNativeEvent(isRealEvent, data);
    subshell->DispatchNativeEvent(isRealEvent, data);

    newWindow->GetLockedState(locked);
  }

  // Get rid of the nested UI thread queue used for netlib, and release the event queue service.
  eQueueService->PopThreadEventQueue();

  subshell->Spindown();
  NS_RELEASE(subshell);

  // We're out of the nested loop.
  // During the layout of the new window, all content shells were located and placed
  // into the new window's content shell array.  Locate the "content area" content
  // shell.
  if (NS_FAILED(rv = newWindow->GetContentShellById("content", &aNewWebShell))) {
    NS_ERROR("Unable to obtain a browser content shell.");
    return rv;
  }
#endif // XP_PC

  return NS_OK;
}


NS_IMETHODIMP nsWebShellWindow::FindWebShellWithName(const PRUnichar* aName,
                                                     nsIWebShell*& aResult)
{
  nsresult rv = NS_OK;

  // Zero result (in case we fail).
  aResult = nsnull;

  // Search for named frame within our root webshell.  This will
  // bypass the .xul document (and rightfully so).  We need to be
  // careful not to give that documents child frames names!
  //
  // This will need to be enhanced to search for (owned?) named
  // windows at some point.
  if ( mWebShell ) {
      rv = mWebShell->FindChildWithName( aName, aResult );
  }

  return rv;
}

NS_IMETHODIMP 
nsWebShellWindow::FocusAvailable(nsIWebShell* aFocusedWebShell, PRBool& aFocusTaken)
{
  return NS_OK;
}

//----------------------------------------
// nsIWebShellWindow methods...
//----------------------------------------
NS_IMETHODIMP
nsWebShellWindow::Show(PRBool aShow)
{
  mWebShell->Show(); // crimminy -- it doesn't take a parameter!
  mWindow->Show(aShow);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ShowModal()
{
  nsresult  rv;
  rv = ShowModalInternal();
  return rv;
}


NS_IMETHODIMP
nsWebShellWindow::ShowModalInternal()
{
  nsresult    rv;
  nsIAppShell *subshell;

  // spin up a new application shell: event loops live there
  rv = nsComponentManager::CreateInstance(kAppShellCID, nsnull, kIAppShellIID, (void**)&subshell);
  if (NS_FAILED(rv))
    return rv;

  subshell->Create(0, nsnull);
  subshell->Spinup();

  nsIWidget *window = GetWidget();
  NS_ADDREF(window);
  mContinueModalLoop = PR_TRUE;
  while (NS_SUCCEEDED(rv) && mContinueModalLoop == PR_TRUE) {
    void      *data;
    PRBool    isRealEvent,
              processEvent;

    rv = subshell->GetNativeEvent(isRealEvent, data);
    if (NS_SUCCEEDED(rv)) {
      subshell->EventIsForModalWindow(isRealEvent, data, window, &processEvent);
      if (processEvent == PR_TRUE)
        subshell->DispatchNativeEvent(isRealEvent, data);
    }
  }

  subshell->Spindown();
  NS_RELEASE(window);
  NS_RELEASE(subshell);

  return rv;
}


NS_IMETHODIMP 
nsWebShellWindow::GetWebShell(nsIWebShell *& aWebShell)
{
  aWebShell = mWebShell;
  NS_ADDREF(mWebShell);
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::GetWidget(nsIWidget *& aWidget)
{
  aWidget = mWindow;
  NS_ADDREF(mWindow);
  return NS_OK;
}

void *
nsWebShellWindow::HandleModalDialogEvent(PLEvent *aEvent)
{
  ThreadedWindowEvent *event = (ThreadedWindowEvent *) aEvent;

  event->window->ShowModalInternal();
  return 0;
}

void
nsWebShellWindow::DestroyModalDialogEvent(PLEvent *aEvent)
{
  PR_Free(aEvent);
}



//----------------------------------------
// nsIDocumentLoaderObserver implementation
//----------------------------------------
NS_IMETHODIMP
nsWebShellWindow::OnStartDocumentLoad(nsIDocumentLoader* loader, 
                                      nsIURL* aURL, const char* aCommand)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnEndDocumentLoad(nsIDocumentLoader* loader, 
                                    nsIURL* aURL, PRInt32 aStatus)
{
#ifdef DEBUG_MENUSDEL
  printf("OnEndDocumentLoad\n");
#endif

  /* We get notified every time a page/Frame is loaded. But we need to
   * Load the menus, run the startup script etc.. only once. So, Use
   * the mChrome Initialized  member to check whether chrome should be 
   * initialized or not - Radha
   */
  if (mChromeInitialized)
    return NS_OK;

  mChromeInitialized = PR_TRUE;

	if (mLockedUntilChromeLoad) {
		mLockedUntilChromeLoad = PR_FALSE;
  }

  // register as document listener
  // this is needed for menus
  nsCOMPtr<nsIContentViewer> cv;
  mWebShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
   
    nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
    if (!docv)
      return NS_OK;

    nsCOMPtr<nsIDocument> doc;
    docv->GetDocument(*getter_AddRefs(doc));
    if (!doc)
      return NS_OK;

    doc->AddObserver(NS_STATIC_CAST(nsIDocumentObserver*, this));
  }

  ExecuteStartupCode();

  ///////////////////////////////
  // Find the Menubar DOM  and Load the menus, hooking them up to the loaded commands
  ///////////////////////////////
  nsCOMPtr<nsIDOMDocument> menubarDOMDoc(GetNamedDOMDoc(nsAutoString("this"))); // XXX "this" is a small kludge for code reused
  if (menubarDOMDoc)
  {
    #ifdef XP_MAC
    LoadMenus(menubarDOMDoc, mWindow);
    // Context Menu test
      	  nsCOMPtr<nsIDOMElement> element;
		  menubarDOMDoc->GetDocumentElement(getter_AddRefs(element));
		  nsCOMPtr<nsIDOMNode> window(do_QueryInterface(element));

		  int endCount = 0;
          contextMenuTest = FindNamedDOMNode(nsAutoString("contextmenu"), window, endCount, 1);
    // End Context Menu test
    #else
    DynamicLoadMenus(menubarDOMDoc, mWindow);
    #endif
  }

  SetSizeFromXUL();
  SetTitleFromXUL();

#if 0
  nsCOMPtr<nsIDOMDocument> toolbarDOMDoc(GetNamedDOMDoc(nsAutoString("browser.toolbar")));
  nsCOMPtr<nsIDOMDocument> contentDOMDoc(GetNamedDOMDoc(nsAutoString("browser.webwindow")));
  nsCOMPtr<nsIDocument> contentDoc(do_QueryInterface(contentDOMDoc));
  nsCOMPtr<nsIDocument> statusDoc(do_QueryInterface(statusDOMDoc));
  nsCOMPtr<nsIDocument> toolbarDoc(do_QueryInterface(toolbarDOMDoc));

  nsIWebShell* statusWebShell = nsnull;
  mWebShell->FindChildWithName(nsAutoString("browser.status"), statusWebShell);

  PRInt32 actualStatusHeight  = GetDocHeight(statusDoc);
  PRInt32 actualToolbarHeight = GetDocHeight(toolbarDoc);


  PRInt32 height = 0;
  PRInt32 x,y,w,h;
  PRInt32 contentHeight;
  PRInt32 toolbarHeight;
  PRInt32 statusHeight;

  mWebShell->GetBounds(x, y, w, h);
  toolbarWebShell->GetBounds(x, y, w, toolbarHeight);
  contentWebShell->GetBounds(x, y, w, contentHeight);
  statusWebShell->GetBounds(x, y, w, statusHeight); 

  //h = toolbarHeight + contentHeight + statusHeight;
  contentHeight = h - actualStatusHeight - actualToolbarHeight;

  toolbarWebShell->GetBounds(x, y, w, h);
  toolbarWebShell->SetBounds(x, y, w, actualToolbarHeight);

  contentWebShell->GetBounds(x, y, w, h);
  contentWebShell->SetBounds(x, y, w, contentHeight);

  statusWebShell->GetBounds(x, y, w, h);
  statusWebShell->SetBounds(x, y, w, actualStatusHeight);
#endif

  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::OnStartURLLoad(nsIDocumentLoader* loader, 
                                 nsIURL* aURL, 
                                 const char* aContentType, 
                                 nsIContentViewer* aViewer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnProgressURLLoad(nsIDocumentLoader* loader, 
                                    nsIURL* aURL, 
                                    PRUint32 aProgress, 
                                    PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnStatusURLLoad(nsIDocumentLoader* loader, 
                                  nsIURL* aURL, nsString& aMsg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnEndURLLoad(nsIDocumentLoader* loader, 
                               nsIURL* aURL, PRInt32 aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::HandleUnknownContentType(nsIDocumentLoader* loader, 
                                           nsIURL* aURL,
                                           const char *aContentType,
                                           const char *aCommand )
{
  return NS_OK;
}


//----------------------------------------
nsCOMPtr<nsIDOMNode> nsWebShellWindow::FindNamedDOMNode(const nsString &aName, nsIDOMNode * aParent, PRInt32 & aCount, PRInt32 aEndCount)
{
  nsCOMPtr<nsIDOMNode> node;
  aParent->GetFirstChild(getter_AddRefs(node));
  while (node) {
    nsString name;
    node->GetNodeName(name);
    //printf("FindNamedDOMNode[%s]==[%s] %d == %d\n", aName.ToNewCString(), name.ToNewCString(), aCount+1, aEndCount); //this leaks
    if (name.Equals(aName)) {
      aCount++;
      if (aCount == aEndCount)
        return node;
    }
    PRBool hasChildren;
    node->HasChildNodes(&hasChildren);
    if (hasChildren) {
      nsCOMPtr<nsIDOMNode> found(FindNamedDOMNode(aName, node, aCount, aEndCount));
      if (found)
        return found;
    }
    nsCOMPtr<nsIDOMNode> oldNode = node;
    oldNode->GetNextSibling(getter_AddRefs(node));
  }
  node = do_QueryInterface(nsnull);
  return node;

} // nsWebShellWindow::FindNamedDOMNode

//----------------------------------------
nsCOMPtr<nsIDOMNode> nsWebShellWindow::GetParentNodeFromDOMDoc(nsIDOMDocument * aDOMDoc)
{
  nsCOMPtr<nsIDOMNode> node; // null

  if (nsnull == aDOMDoc) {
    return node;
  }

  nsCOMPtr<nsIDOMElement> element;
  aDOMDoc->GetDocumentElement(getter_AddRefs(element));
  if (element)
    return nsCOMPtr<nsIDOMNode>(do_QueryInterface(element));
  return node;
} // nsWebShellWindow::GetParentNodeFromDOMDoc

//----------------------------------------
nsCOMPtr<nsIDOMDocument> nsWebShellWindow::GetNamedDOMDoc(const nsString & aWebShellName)
{
  nsCOMPtr<nsIDOMDocument> domDoc; // result == nsnull;

  // first get the toolbar child WebShell
  nsCOMPtr<nsIWebShell> childWebShell;
  if (aWebShellName.Equals("this")) { // XXX small kludge for code reused
    childWebShell = do_QueryInterface(mWebShell);
  } else {
    mWebShell->FindChildWithName(aWebShellName.GetUnicode(), *getter_AddRefs(childWebShell));
    if (!childWebShell)
      return domDoc;
  }
  
  nsCOMPtr<nsIContentViewer> cv;
  childWebShell->GetContentViewer(getter_AddRefs(cv));
  if (!cv)
    return domDoc;
   
  nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
  if (!docv)
    return domDoc;

  nsCOMPtr<nsIDocument> doc;
  docv->GetDocument(*getter_AddRefs(doc));
  if (doc)
    return nsCOMPtr<nsIDOMDocument>(do_QueryInterface(doc));

  return domDoc;
} // nsWebShellWindow::GetNamedDOMDoc

//----------------------------------------
PRInt32 nsWebShellWindow::GetDocHeight(nsIDocument * aDoc)
{
  nsIPresShell * presShell = aDoc->GetShellAt(0);
  if (!presShell)
    return 0;

  nsCOMPtr<nsIPresContext> presContext;
  presShell->GetPresContext(getter_AddRefs(presContext));
  if (presContext) {
    nsRect rect;
    presContext->GetVisibleArea(rect);
    nsIFrame * rootFrame;
    nsSize size;
    presShell->GetRootFrame(&rootFrame);
    if (rootFrame) {
      rootFrame->GetSize(size);
      float t2p;
      presContext->GetTwipsToPixels(&t2p);
      printf("Doc size %d,%d\n", PRInt32((float)size.width*t2p), 
                                 PRInt32((float)size.height*t2p));
      //return rect.height;
      return PRInt32((float)rect.height*t2p);
      //return PRInt32((float)size.height*presContext->GetTwipsToPixels());
    }
  }
  NS_RELEASE(presShell);
  return 0;
}

//----------------------------------------
#if 0
NS_IMETHODIMP nsWebShellWindow::OnConnectionsComplete()
{
#ifdef DEBUG_MENUSDEL
  printf("OnConnectionsComplete\n");
#endif

  // register as document listener
  // this is needed for menus
  nsCOMPtr<nsIContentViewer> cv;
  mWebShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
   
    nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
    if (!docv)
      return NS_OK;

    nsCOMPtr<nsIDocument> doc;
    docv->GetDocument(*getter_AddRefs(doc));
    if (!doc)
      return NS_OK;

    doc->AddObserver(NS_STATIC_CAST(nsIDocumentObserver*, this));
  }

  ExecuteStartupCode();

  ///////////////////////////////
  // Find the Menubar DOM  and Load the menus, hooking them up to the loaded commands
  ///////////////////////////////
  nsCOMPtr<nsIDOMDocument> menubarDOMDoc(GetNamedDOMDoc(nsAutoString("this"))); // XXX "this" is a small kludge for code reused
  if (menubarDOMDoc) {
    #ifdef XP_MAC
    LoadMenus(menubarDOMDoc, mWindow);
    #else
    DynamicLoadMenus(menubarDOMDoc, mWindow);
    #endif
  }

  SetSizeFromXUL();
  SetTitleFromXUL();

#if 0
  nsCOMPtr<nsIDOMDocument> toolbarDOMDoc(GetNamedDOMDoc(nsAutoString("browser.toolbar")));
  nsCOMPtr<nsIDOMDocument> contentDOMDoc(GetNamedDOMDoc(nsAutoString("browser.webwindow")));
  nsCOMPtr<nsIDocument> contentDoc(do_QueryInterface(contentDOMDoc));
  nsCOMPtr<nsIDocument> statusDoc(do_QueryInterface(statusDOMDoc));
  nsCOMPtr<nsIDocument> toolbarDoc(do_QueryInterface(toolbarDOMDoc));

  nsIWebShell* statusWebShell = nsnull;
  mWebShell->FindChildWithName(nsAutoString("browser.status"), statusWebShell);

  PRInt32 actualStatusHeight  = GetDocHeight(statusDoc);
  PRInt32 actualToolbarHeight = GetDocHeight(toolbarDoc);


  PRInt32 height = 0;
  PRInt32 x,y,w,h;
  PRInt32 contentHeight;
  PRInt32 toolbarHeight;
  PRInt32 statusHeight;

  mWebShell->GetBounds(x, y, w, h);
  toolbarWebShell->GetBounds(x, y, w, toolbarHeight);
  contentWebShell->GetBounds(x, y, w, contentHeight);
  statusWebShell->GetBounds(x, y, w, statusHeight); 

  //h = toolbarHeight + contentHeight + statusHeight;
  contentHeight = h - actualStatusHeight - actualToolbarHeight;

  toolbarWebShell->GetBounds(x, y, w, h);
  toolbarWebShell->SetBounds(x, y, w, actualToolbarHeight);

  contentWebShell->GetBounds(x, y, w, h);
  contentWebShell->SetBounds(x, y, w, contentHeight);

  statusWebShell->GetBounds(x, y, w, h);
  statusWebShell->SetBounds(x, y, w, actualStatusHeight);
#endif

  return NS_OK;
} // nsWebShellWindow::OnConnectionsComplete 
#endif  /* 0 */



/**
 * Get nsIDOMNode corresponding to a given webshell
 * @param aShell the given webshell
 * @return the corresponding DOM element, null if for some reason there is none
 */
nsCOMPtr<nsIDOMNode>
nsWebShellWindow::GetDOMNodeFromWebShell(nsIWebShell *aShell)
{
  nsCOMPtr<nsIDOMNode> node;

  nsCOMPtr<nsIContentViewer> cv;
  aShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
    if (docv) {
      nsCOMPtr<nsIDocument> doc;
      docv->GetDocument(*getter_AddRefs(doc));
      if (doc) {
        nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(doc));
        if (domdoc) {
          nsCOMPtr<nsIDOMElement> element;
          domdoc->GetDocumentElement(getter_AddRefs(element));
          if (element)
            node = do_QueryInterface(element);
        }
      }
    }
  }

  return node;
}

void nsWebShellWindow::ExecuteJavaScriptString(nsString& aJavaScript)
{
  if (aJavaScript.Length() == 0) {
    return;
  }
  
  // Get nsIScriptContextOwner
  nsCOMPtr<nsIScriptContextOwner> scriptContextOwner ( do_QueryInterface(mWebShell) );
  if ( scriptContextOwner ) {
    const char* url = "";
      // Get nsIScriptContext
    nsCOMPtr<nsIScriptContext> scriptContext;
    nsresult status = scriptContextOwner->GetScriptContext(getter_AddRefs(scriptContext));
    if (NS_OK == status) {
      // Ask the script context to evalute the javascript string
      PRBool isUndefined = PR_FALSE;
      nsString rVal("xxx");
      scriptContext->EvaluateString(aJavaScript, url, 0, rVal, &isUndefined);

#ifdef DEBUG_MENUSDEL
      printf("EvaluateString - %d [%s]\n", isUndefined, rVal.ToNewCString());
#endif
    }

  }
}


/**
 * Execute window onLoad handler
 */
void nsWebShellWindow::ExecuteStartupCode()
{
  nsCOMPtr<nsIDOMNode> webshellNode = GetDOMNodeFromWebShell(mWebShell);
  nsCOMPtr<nsIDOMElement> webshellElement;

  if (webshellNode)
    webshellElement = do_QueryInterface(webshellNode);

  if (mCallbacks)
    mCallbacks->ConstructBeforeJavaScript(mWebShell);

  // Execute the string in the onLoad attribute of the webshellElement.
  nsString startupCode;
  if (webshellElement && NS_SUCCEEDED(webshellElement->GetAttribute("onload", startupCode)))
    ExecuteJavaScriptString(startupCode);

  if (mCallbacks)
    mCallbacks->ConstructAfterJavaScript(mWebShell);

}


/* A somewhat early version of window sizing code.  This simply reads attributes
   from the window tag and blindly sets the size to whatever it finds within.
*/
void nsWebShellWindow::SetSizeFromXUL()
{
  nsCOMPtr<nsIDOMNode> webshellNode = GetDOMNodeFromWebShell(mWebShell);
  nsIWidget *windowWidget = GetWidget();
  nsCOMPtr<nsIDOMElement> webshellElement;
  nsString sizeString;
  PRInt32 errorCode,
          specWidth, specHeight,
          specSize;
  nsRect  currentSize;

  if (webshellNode)
    webshellElement = do_QueryInterface(webshellNode);
  if (!webshellElement || !windowWidget) // it's hopeless
    return;

  // first guess: use current size
  mWindow->GetBounds(currentSize);
  specWidth = currentSize.width;
  specHeight = currentSize.height;

  // read "height" attribute
  if (NS_SUCCEEDED(webshellElement->GetAttribute("height", sizeString))) {
    specSize = sizeString.ToInteger(&errorCode);
    if (NS_SUCCEEDED(errorCode) && specSize > 0)
      specHeight = specSize;
  }

  // read "width" attribute
  if (NS_SUCCEEDED(webshellElement->GetAttribute("width", sizeString))) {
    specSize = sizeString.ToInteger(&errorCode);
    if (NS_SUCCEEDED(errorCode) || specSize > 0)
      specWidth = specSize;
  }

  if (specWidth != currentSize.width || specHeight != currentSize.height)
    windowWidget->Resize(specWidth, specHeight, PR_TRUE);

#if 0
  // adjust height to fit contents?
  if (fitHeight == PR_TRUE) {
    nsCOMPtr<nsIContentViewer> cv;
    mWebShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
      nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
      if (docv) {
        nsCOMPtr<nsIDocument> doc;
        docv->GetDocument(*getter_AddRefs(doc));
        if (doc)
          specHeight = GetDocHeight(doc);
      }
    }
    mWindow->GetBounds(currentSize);
    windowWidget->Resize(currentSize.width, specHeight, PR_TRUE);
  }
#endif
} // SetSizeFromXUL


void nsWebShellWindow::SetTitleFromXUL()
{
  nsCOMPtr<nsIDOMNode> webshellNode = GetDOMNodeFromWebShell(mWebShell);
  nsIWidget *windowWidget = GetWidget();
  nsCOMPtr<nsIDOMElement> webshellElement;
  nsString windowTitle;

  if (webshellNode)
    webshellElement = do_QueryInterface(webshellNode);
  if (webshellElement && windowWidget &&
      NS_SUCCEEDED(webshellElement->GetAttribute("title", windowTitle)))
    windowWidget->SetTitle(windowTitle);
} // SetTitleFromXUL


//----------------------------------------------------------------
//-- nsIDocumentObserver
//----------------------------------------------------------------
NS_IMETHODIMP
nsWebShellWindow::BeginUpdate(nsIDocument *aDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::EndUpdate(nsIDocument *aDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::BeginLoad(nsIDocument *aDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::EndLoad(nsIDocument *aDocument)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::BeginReflow(nsIDocument *aDocument, nsIPresShell* aShell)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::EndReflow(nsIDocument *aDocument, nsIPresShell* aShell)
{
  return NS_OK;
}

///////////////////////////////////////////////////////////////
// nsIDocumentObserver
// this is needed for menu changes
///////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsWebShellWindow::ContentChanged(nsIDocument *aDocument,
                                 nsIContent* aContent,
                                 nsISupports* aSubContent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ContentStatesChanged(nsIDocument *aDocument,
                                       nsIContent* aContent1,
                                       nsIContent* aContent2)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::AttributeChanged(nsIDocument *aDocument,
                                   nsIContent*  aContent,
                                   nsIAtom*     aAttribute,
                                   PRInt32      aHint)
{
  //printf("AttributeChanged\n");
  PRInt32 i;
  for (i=0;i<mMenuDelegates.Count();i++) {
    nsIXULCommand * cmd  = (nsIXULCommand *)mMenuDelegates[i];
    nsIDOMElement * node;
    cmd->GetDOMElement(&node);
    //nsCOMPtr<nsIContent> content(do_QueryInterface(node));
    // Doing this for the must speed
    nsIContent * content;
    if (NS_OK == node->QueryInterface(kIContentIID, (void**) &content)) {
      if (content == aContent) {
        nsAutoString attr;
        aAttribute->ToString(attr);
        cmd->AttributeHasBeenSet(attr);
      }
      NS_RELEASE(content);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ContentAppended(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            PRInt32     aNewIndexInContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ContentInserted(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            PRInt32 aIndexInContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ContentReplaced(nsIDocument *aDocument,
                            nsIContent* aContainer,
                            nsIContent* aOldChild,
                            nsIContent* aNewChild,
                            PRInt32 aIndexInContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::ContentRemoved(nsIDocument *aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleSheetAdded(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleSheetRemoved(nsIDocument *aDocument,
                              nsIStyleSheet* aStyleSheet)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleSheetDisabledStateChanged(nsIDocument *aDocument,
                                           nsIStyleSheet* aStyleSheet,
                                           PRBool aDisabled)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleRuleChanged(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule,
                             PRInt32 aHint)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleRuleAdded(nsIDocument *aDocument,
                           nsIStyleSheet* aStyleSheet,
                           nsIStyleRule* aStyleRule)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::StyleRuleRemoved(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::DocumentWillBeDestroyed(nsIDocument *aDocument)
{
  return NS_OK;
}

/**************** nsIBrowserWindow interface ********************/
NS_IMETHODIMP nsWebShellWindow::Init(nsIAppShell* aAppShell,
                   nsIPref* aPrefs,
                   const nsRect& aBounds,
                   PRUint32 aChromeMask,
                   PRBool aAllowPlugins)
{
   nsresult rv;
   nsCOMPtr<nsIURL> urlObj;
 
   // (temporary)
   rv = NS_NewURL(getter_AddRefs(urlObj), "chrome://navigator/content/");
   if (NS_FAILED(rv))
     return rv;
 
   // Note: null nsIStreamObserver means this window won't be able to answer FE_callback-type
   // questions from netlib.  Observers are generally appcores.  We'll have to supply
   // a generic browser appcore here someday.
   rv = Initialize(nsnull, aAppShell, urlObj,
       nsnull, nsnull, aBounds.width, aBounds.height);
   if (NS_SUCCEEDED(rv))
     MoveTo(aBounds.x, aBounds.y);
   return rv;
}
 
NS_IMETHODIMP nsWebShellWindow::MoveTo(PRInt32 aX, PRInt32 aY)
{
   mWindow->Move(aX, aY);
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::SizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
   mWindow->Resize(aWidth, aHeight, PR_TRUE);
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::GetBounds(nsRect& aResult)
{
   mWindow->GetClientBounds(aResult);
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::GetWindowBounds(nsRect& aResult)
{
   mWindow->GetBounds(aResult);
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::SetChrome(PRUint32 aNewChromeMask)
{
   // yeah sure. we got your chrome changes.
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::GetChrome(PRUint32& aChromeMaskResult)
{
   // Do our best to fake an interesting, rather inapplicable concept from the old world.
   // We can't tell what sort of chrome features we have until we invent a standard
   // for XUL chrome elements, and then go looking for them.  For now, take a guess...
   aChromeMaskResult = NS_CHROME_WINDOW_BORDERS_ON | NS_CHROME_WINDOW_CLOSE_ON |
                       NS_CHROME_WINDOW_RESIZE_ON | NS_CHROME_MENU_BAR_ON |
                       NS_CHROME_TOOL_BAR_ON | NS_CHROME_LOCATION_BAR_ON |
                       NS_CHROME_STATUS_BAR_ON | NS_CHROME_PERSONAL_TOOLBAR_ON |
                       NS_CHROME_SCROLLBARS_ON | NS_CHROME_TITLEBAR_ON;
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::SetTitle(const PRUnichar* aTitle)
{
   nsIWidget *windowWidget = GetWidget();
 
   if (windowWidget)
     windowWidget->SetTitle(aTitle);
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::GetTitle(const PRUnichar** aResult)
{
   // no, we didn't store the title for you. why so nosy?
   return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP nsWebShellWindow::SetStatus(const PRUnichar* aStatus)
{
   // yup. got it.
   return NS_OK;
}
 
NS_IMETHODIMP nsWebShellWindow::GetStatus(const PRUnichar** aResult)
{
   // didn't store the status string, either.
   return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP nsWebShellWindow::SetProgress(PRInt32 aProgress, PRInt32 aProgressMax)
{
   // oh yeah. we're on it.
	 return NS_OK;
}
