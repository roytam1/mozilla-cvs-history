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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsCOMPtr.h"
#include "nsGlobalWindow.h"
#include "nsIContent.h"
#include "nscore.h"
#include "nsRect.h"
#include "nslayout.h"
#include "prmem.h"
#include "prtime.h"
#include "plstr.h"
#include "prinrval.h"
#include "nsIFactory.h"
#include "nsIScriptContext.h"
#include "nsIDOMDocument.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"
#include "nsEventListenerManager.h"
#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"
#include "nsIDOMBarProp.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMPaintListener.h"
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsJSUtils.h"
#include "nsIScriptEventListener.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIBrowserWindow.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIDocument.h"
#include "nsIURL.h"
#include "nsIPref.h"
#include "nsCRT.h"
#include "nsRect.h"
#include "nsIPrompt.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"
#include "nsIContentViewerEdit.h"
#include "nsIDocumentViewer.h"
#include "nsIPresShell.h"
#include "nsISelectionController.h"
#include "nsIScrollableView.h"
#include "nsIDeviceContext.h"
#include "nsScreen.h"
#include "nsHistory.h"
#include "nsBarProps.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsRDFCID.h"
#include "nsDOMPropEnums.h"
#include "nsXPIDLString.h"
#include "nsICookieService.h"
#include "nsICharsetConverterManager.h"
#include "nsEscape.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#include "nsIJVMManager.h"

#include "nsMimeTypeArray.h"
#include "nsPluginArray.h"

#include "jsapi.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIScriptEventListenerIID, NS_ISCRIPTEVENTLISTENER_IID);
static NS_DEFINE_IID(kIDOMWindowIID, NS_IDOMWINDOW_IID);
static NS_DEFINE_IID(kIDOMNavigatorIID, NS_IDOMNAVIGATOR_IID);
static NS_DEFINE_IID(kIJSScriptObjectIID, NS_IJSSCRIPTOBJECT_IID);
static NS_DEFINE_IID(kIDOMMouseListenerIID, NS_IDOMMOUSELISTENER_IID);
static NS_DEFINE_IID(kIDOMKeyListenerIID, NS_IDOMKEYLISTENER_IID);
static NS_DEFINE_IID(kIDOMMouseMotionListenerIID, NS_IDOMMOUSEMOTIONLISTENER_IID);
static NS_DEFINE_IID(kIDOMFocusListenerIID, NS_IDOMFOCUSLISTENER_IID);
static NS_DEFINE_IID(kIDOMFormListenerIID, NS_IDOMFORMLISTENER_IID);
static NS_DEFINE_IID(kIDOMLoadListenerIID, NS_IDOMLOADLISTENER_IID);
static NS_DEFINE_IID(kIDOMDragListenerIID, NS_IDOMDRAGLISTENER_IID);
static NS_DEFINE_IID(kIDOMPaintListenerIID, NS_IDOMPAINTLISTENER_IID);
static NS_DEFINE_IID(kIEventListenerManagerIID, NS_IEVENTLISTENERMANAGER_IID);
static NS_DEFINE_IID(kIPrivateDOMEventIID, NS_IPRIVATEDOMEVENT_IID);
static NS_DEFINE_IID(kIDOMEventReceiverIID, NS_IDOMEVENTRECEIVER_IID);
static NS_DEFINE_IID(kIDOMEventTargetIID, NS_IDOMEVENTTARGET_IID);
static NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIDOMAbstractViewIID, NS_IDOMABSTRACTVIEW_IID);

static NS_DEFINE_IID(kIPrefIID, NS_IPREF_IID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_IID(kCookieServiceCID, NS_COOKIESERVICE_CID);

static NS_DEFINE_CID(kXULControllersCID,          NS_XULCONTROLLERS_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

//STATIC METHOD
//one MORE implementation of this function
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
//END STATICS


GlobalWindowImpl::GlobalWindowImpl()
{
  NS_INIT_REFCNT();
  mContext = nsnull;
  mScriptObject = nsnull;
  mDocument = nsnull;
  mNavigator = nsnull;
  mScreen = nsnull;
  mHistory = nsnull;
  mMenubar = nsnull;
  mToolbar = nsnull;
  mLocationbar = nsnull;
  mPersonalbar = nsnull;
  mStatusbar = nsnull;
  mScrollbars = nsnull;
  mLocation = nsnull;
  mFrames = nsnull;
  mOpener = nsnull;
  mGlobalObjectOwner = nsnull;

  mTimeouts = nsnull;
  mTimeoutInsertionPoint = nsnull;
  mRunningTimeout = nsnull;
  mTimeoutPublicIdCounter = 1;
  mListenerManager = nsnull;

  mFirstDocumentLoad = PR_TRUE;

  mChromeEventHandler = nsnull;
  mWebShell = nsnull;
}

void GlobalWindowImpl::CleanUp()
{
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mNavigator);
  NS_IF_RELEASE(mScreen);
  NS_IF_RELEASE(mHistory);
  NS_IF_RELEASE(mMenubar);
  NS_IF_RELEASE(mToolbar);
  NS_IF_RELEASE(mLocationbar);
  NS_IF_RELEASE(mPersonalbar);
  NS_IF_RELEASE(mStatusbar);
  NS_IF_RELEASE(mScrollbars);
  NS_IF_RELEASE(mLocation);
  NS_IF_RELEASE(mFrames);
  NS_IF_RELEASE(mOpener);
  NS_IF_RELEASE(mListenerManager);
}

GlobalWindowImpl::~GlobalWindowImpl() 
{  
  CleanUp();
}

NS_IMPL_ADDREF(GlobalWindowImpl)
NS_IMPL_RELEASE(GlobalWindowImpl)

nsresult 
GlobalWindowImpl::QueryInterface(const nsIID& aIID,
                                 void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIScriptObjectOwner))) {
    *aInstancePtrResult = (void*) ((nsIScriptObjectOwner*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIScriptGlobalObject))) {
    *aInstancePtrResult = (void*) ((nsIScriptGlobalObject*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIScriptGlobalObjectData))) {
    *aInstancePtrResult = (void*) ((nsIScriptGlobalObjectData*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMWindowIID)) {
    *aInstancePtrResult = (void*) ((nsIDOMWindow*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIScriptGlobalObject*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIJSScriptObjectIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIJSScriptObject*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventReceiverIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIDOMEventReceiver*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMEventTargetIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIDOMEventTarget*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsPIDOMWindow))) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsPIDOMWindow*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMAbstractViewIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIDOMAbstractView*)this;
    AddRef();
    return NS_OK;
  }
 
  return NS_NOINTERFACE;
}

nsresult 
GlobalWindowImpl::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

nsresult 
GlobalWindowImpl::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    res = NS_NewScriptWindow(aContext, (nsIDOMWindow*)this,
                             nsnull, &mScriptObject);
    aContext->AddNamedReference(&mScriptObject, mScriptObject, 
                                "window_object");
  }
  
  *aScriptObject = mScriptObject;
  return res;
}

NS_IMETHODIMP
GlobalWindowImpl::SetContext(nsIScriptContext *aContext)
{
  if (mContext) {
    NS_RELEASE(mContext);
  }

  mContext = aContext;
  NS_ADDREF(mContext);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetContext(nsIScriptContext **aContext)
{
  *aContext = mContext;
  NS_IF_ADDREF(*aContext);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetNewDocument(nsIDOMDocument *aDocument)
{
  if (mFirstDocumentLoad) {
    mFirstDocumentLoad = PR_FALSE;
    mDocument = aDocument;
    NS_IF_ADDREF(mDocument);
    return NS_OK;
  }
  
  if (mDocument) {
    nsIDocument* doc;
    nsCOMPtr<nsIURI> docURL;

    if (mDocument && NS_SUCCEEDED(mDocument->QueryInterface(kIDocumentIID, (void**)&doc))) {
      docURL = dont_AddRef(doc->GetDocumentURL());
      NS_RELEASE(doc);
    }

    if (docURL) {
      char* str;
      docURL->GetSpec(&str);

      nsAutoString url = str;

      //about:blank URL's do not have ClearScope called on page change.
      if (url != "about:blank") {
        ClearAllTimeouts();
  
        if (mListenerManager) {
          mListenerManager->RemoveAllListeners(PR_FALSE);
        }

        if ((nsnull != mScriptObject) && 
            (nsnull != mContext) /* &&
            (nsnull != aDocument) XXXbe why commented out? */ ) {
          JS_ClearScope((JSContext *)mContext->GetNativeContext(),
                        (JSObject *)mScriptObject);
        }
      }
      nsCRT::free(str);
    }
  }

  //XXX Should this be outside the about:blank clearscope exception?
  if (nsnull != mDocument) {
    NS_RELEASE(mDocument);
  }
    
  if (nsnull != mContext) {
    mContext->GC();
  }

  mDocument = aDocument;
  
  if (nsnull != mDocument) {
    NS_ADDREF(mDocument);

    if (nsnull != mContext) {
      mContext->InitContext(this);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetWebShell(nsIWebShell *aWebShell)
{
   nsCOMPtr<nsIDocShell> aDocShell(do_QueryInterface(aWebShell));
  //mWebShell isn't refcnt'd here.  WebShell calls SetWebShell(nsnull) when deleted.

  // When SetWebShell(nsnull) is called, drop our references to the
  // script object (held via a named JS root) and the script context
  // itself.
  if ((nsnull == aWebShell) && (nsnull != mContext)) {

    if (nsnull != mScriptObject) {
      // Indicate that the window is now closed. Since we've
      // cleared scope, we have to explicitly set a property.
      jsval val = BOOLEAN_TO_JSVAL(JS_TRUE);
      JS_SetProperty((JSContext*)mContext->GetNativeContext(),
                     (JSObject*)mScriptObject,
                     "closed",
                     &val);
      mContext->RemoveReference(&mScriptObject, mScriptObject);
      mScriptObject = nsnull;
    }
      
    NS_RELEASE(mContext);
  }
  mWebShell = aWebShell;
  if (nsnull != mLocation) {
    mLocation->SetWebShell(aWebShell);
  }
  if (nsnull != mHistory) {
    mHistory->SetWebShell(aWebShell);
  }
  if (nsnull != mFrames) {
    mFrames->SetDocShell(aDocShell);
  }

  if (mWebShell) {
    // tell our member elements about the new browserwindow

    if (nsnull != mMenubar)  {
      nsCOMPtr<nsIBrowserWindow> browser;
      GetBrowserWindowInterface(*getter_AddRefs(browser));
      mMenubar->SetBrowserWindow(browser);
    }
    // Get our enclosing chrome shell and retrieve its global window impl, so that we can
    // do some forwarding to the chrome document.
    nsCOMPtr<nsIChromeEventHandler> chromeEventHandler;
    mWebShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));
    if(chromeEventHandler)
      mChromeEventHandler = chromeEventHandler.get(); //   ref
  }
   
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetWebShell(nsIWebShell **aWebShell)
{
   *aWebShell = mWebShell;
   NS_IF_ADDREF(*aWebShell);
   return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetOpenerWindow(nsIDOMWindow *aOpener)
{
  NS_IF_RELEASE(mOpener);
  mOpener = aOpener;
  NS_IF_ADDREF(mOpener);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetGlobalObjectOwner(nsIScriptGlobalObjectOwner* aOwner)
{
   mGlobalObjectOwner = aOwner; // Note this is supposed to be a weak ref.
   return NS_OK;   
}

NS_IMETHODIMP
GlobalWindowImpl::GetGlobalObjectOwner(nsIScriptGlobalObjectOwner** aOwner)
{
   NS_ENSURE_ARG_POINTER(aOwner);

   *aOwner = mGlobalObjectOwner;
   NS_IF_ADDREF(*aOwner);
   return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::GetWindow(nsIDOMWindow** aWindow)
{
  *aWindow = this;
  NS_ADDREF(this);

  return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::GetSelf(nsIDOMWindow** aWindow)
{
  *aWindow = this;
  NS_ADDREF(this);

  return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::GetDocument(nsIDOMDocument** aDocument)
{
  *aDocument = mDocument;
  NS_IF_ADDREF(mDocument);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetNavigator(nsIDOMNavigator** aNavigator)
{
  if (nsnull == mNavigator) {
    mNavigator = new NavigatorImpl();
    NS_IF_ADDREF(mNavigator);
  }

  *aNavigator = mNavigator;
  NS_IF_ADDREF(mNavigator);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetScreen(nsIDOMScreen** aScreen)
{
  if ((nsnull == mScreen) && (nsnull != mWebShell)) {
    mScreen = new ScreenImpl( mWebShell );
    NS_IF_ADDREF(mScreen);
  }
    
  *aScreen = mScreen;
  NS_IF_ADDREF(mScreen);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetHistory(nsIDOMHistory** aHistory)
{
  if ((nsnull == mHistory) && (nsnull != mWebShell)) {
    mHistory = new HistoryImpl();
    if (nsnull != mHistory) {
      NS_ADDREF(mHistory);
      mHistory->SetWebShell(mWebShell);
    }
  }
  
  *aHistory = mHistory;
  NS_IF_ADDREF(mHistory);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetMenubar(nsIDOMBarProp** aMenubar)
{
  if (nsnull == mMenubar) {
    mMenubar = new MenubarPropImpl();
    if (nsnull != mMenubar) {
      NS_ADDREF(mMenubar);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mMenubar->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aMenubar = mMenubar;
  NS_IF_ADDREF(mMenubar);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetToolbar(nsIDOMBarProp** aToolbar)
{
  if (nsnull == mToolbar) {
    mToolbar = new ToolbarPropImpl();
    if (nsnull != mToolbar) {
      NS_ADDREF(mToolbar);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mToolbar->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aToolbar = mToolbar;
  NS_IF_ADDREF(mToolbar);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetLocationbar(nsIDOMBarProp** aLocationbar)
{
  if (nsnull == mLocationbar) {
    mLocationbar = new LocationbarPropImpl();
    if (nsnull != mLocationbar) {
      NS_ADDREF(mLocationbar);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mLocationbar->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aLocationbar = mLocationbar;
  NS_IF_ADDREF(mLocationbar);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetPersonalbar(nsIDOMBarProp** aPersonalbar)
{
  if (nsnull == mPersonalbar) {
    mPersonalbar = new PersonalbarPropImpl();
    if (nsnull != mPersonalbar) {
      NS_ADDREF(mPersonalbar);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mPersonalbar->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aPersonalbar = mPersonalbar;
  NS_IF_ADDREF(mPersonalbar);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetStatusbar(nsIDOMBarProp** aStatusbar)
{
  if (nsnull == mStatusbar) {
    mStatusbar = new StatusbarPropImpl();
    if (nsnull != mStatusbar) {
      NS_ADDREF(mStatusbar);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mStatusbar->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aStatusbar = mStatusbar;
  NS_IF_ADDREF(mStatusbar);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetScrollbars(nsIDOMBarProp** aScrollbars)
{
  if (nsnull == mScrollbars) {
    mScrollbars = new ScrollbarsPropImpl();
    if (nsnull != mScrollbars) {
      NS_ADDREF(mScrollbars);
      nsIBrowserWindow *browser = nsnull;
      if ((nsnull != mWebShell) && NS_OK == GetBrowserWindowInterface(browser)) {
        mScrollbars->SetBrowserWindow(browser);
        NS_IF_RELEASE(browser);
      }
    }
  }

  *aScrollbars = mScrollbars;
  NS_IF_ADDREF(mScrollbars);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetDirectories(nsIDOMBarProp** aDirectories)
{
  return GetPersonalbar(aDirectories);
}

NS_IMETHODIMP
GlobalWindowImpl::GetOpener(nsIDOMWindow** aOpener)
{
  *aOpener = mOpener;
  NS_IF_ADDREF(*aOpener);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetOpener(nsIDOMWindow* aOpener)
{
  NS_IF_RELEASE(mOpener);
  mOpener = aOpener;
  NS_IF_ADDREF(mOpener);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetParent(nsIDOMWindow** aParent)
{
   NS_ENSURE_ARG_POINTER(aParent);
   *aParent = nsnull;
   if(!mWebShell)
      return NS_OK;

   nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
   NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

   nsCOMPtr<nsIDocShellTreeItem> parent;
   docShellAsItem->GetSameTypeParent(getter_AddRefs(parent));

   if(parent)
      {
      nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(parent));
      NS_ENSURE_SUCCESS(CallQueryInterface(globalObject.get(), aParent),
         NS_ERROR_FAILURE);
      }
   else
      {
      *aParent = NS_STATIC_CAST(nsIDOMWindow*, this);
      NS_ADDREF(*aParent);
      }
   return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::GetLocation(nsIDOMLocation** aLocation)
{
  if ((nsnull == mLocation) && (nsnull != mWebShell)) {
    mLocation = new LocationImpl(mWebShell);
    NS_IF_ADDREF(mLocation);
  }

  *aLocation = mLocation;
  NS_IF_ADDREF(mLocation);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetTop(nsIDOMWindow** aTop)
{
  nsresult ret = NS_OK;

  *aTop = nsnull;
  if (nsnull != mWebShell) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));

    if (root) {
       nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(root));
       CallQueryInterface(globalObject.get(), aTop);
    }
  }

  return ret;
}

NS_IMETHODIMP
GlobalWindowImpl::GetContent(nsIDOMWindow** aContent)
{
  nsresult rv;

  *aContent = nsnull;

  nsCOMPtr<nsIBrowserWindow> browser;
  rv = GetBrowserWindowInterface(*getter_AddRefs(browser));
  if (NS_SUCCEEDED(rv) && browser) {
    nsCOMPtr<nsIWebShell> contentShell;
    rv = browser->GetContentWebShell(getter_AddRefs(contentShell));
    if (NS_SUCCEEDED(rv) && contentShell)
      rv = WebShellToDOMWindow(contentShell, aContent);
  }
  return rv;
}

NS_IMETHODIMP
GlobalWindowImpl::GetClosed(PRBool* aClosed)
{
  if (nsnull == mWebShell) {
    *aClosed = PR_TRUE;
  }
  else {
    *aClosed = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetFrames(nsIDOMWindowCollection** aFrames)
{
  nsCOMPtr<nsIDocShell> mDocShell(do_QueryInterface(mWebShell));
  if ((nsnull == mFrames) && (mDocShell)) {
    mFrames = new nsDOMWindowList(mDocShell);
    if (nsnull == mFrames) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mFrames);
  }

  *aFrames = (nsIDOMWindowCollection *)mFrames;
  NS_IF_ADDREF(mFrames);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetStatus(nsString& aStatus)
{
  nsIBrowserWindow *mBrowser;
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    const PRUnichar *status;
    mBrowser->GetStatus(&status);
    aStatus = status;
    NS_RELEASE(mBrowser);
  }
  else {
    aStatus.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetStatus(const nsString& aStatus)
{
  nsIBrowserWindow *mBrowser;
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    mBrowser->SetStatus(aStatus.GetUnicode());
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetDefaultStatus(nsString& aDefaultStatus)
{
  nsIBrowserWindow *mBrowser;
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    const PRUnichar *status;
    mBrowser->GetDefaultStatus(&status);
    aDefaultStatus = status;
    NS_RELEASE(mBrowser);
  }
  else {
    aDefaultStatus.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetDefaultStatus(const nsString& aDefaultStatus)
{
  nsIBrowserWindow *mBrowser;
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    mBrowser->SetDefaultStatus(aDefaultStatus.GetUnicode());
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetName(nsString& aName)
{
  nsXPIDLString name;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
  if (docShellAsItem) {
    docShellAsItem->GetName(getter_Copies(name));
  }
  aName = name;
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetName(const nsString& aName)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
  if (docShellAsItem) {
    result = docShellAsItem->SetName(aName.GetUnicode());
  }
  return result;
}


NS_IMETHODIMP
GlobalWindowImpl::GetInnerWidth(PRInt32* aInnerWidth)
{
  nsIBrowserWindow *mBrowser;
  nsIDOMWindow* parent = nsnull;
  
  GetParent(&parent);
  if (parent == this) {
    // We are in a top level window.  Use browser window's bounds.
    if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
      nsRect r;
      mBrowser->GetContentBounds(r);
      *aInnerWidth = r.width;
      NS_RELEASE(mBrowser);
    }
    else {
      *aInnerWidth = 0;
    }
  }
  else {
    // We are in an (i)frame.  Use webshell bounds.
    nsCOMPtr<nsIBaseWindow> webShellWin(do_QueryInterface(mWebShell));
    *aInnerWidth = 0;
    if(webShellWin)
      {
      PRInt32 cy;
      webShellWin->GetSize(aInnerWidth, &cy);
      }    
  }
  
  NS_RELEASE(parent);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetInnerWidth(PRInt32 aInnerWidth)
{
  nsresult         rv = NS_OK;
  nsIBrowserWindow *browser;
  nsIDOMWindow     *parent = nsnull;

  GetParent(&parent);
  if (parent == this) {
    if (NS_OK == (rv = GetBrowserWindowInterface(browser))) {
      nsRect r;
      browser->GetContentBounds(r);
      rv = browser->SizeContentTo(aInnerWidth, r.height);
      NS_RELEASE(browser);
    }
  } else {
    rv = NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIBaseWindow> webShellWin(do_QueryInterface(mWebShell));
    if(webShellWin)
      {
      PRInt32 cx = 0, cy = 0;
      webShellWin->GetSize(&cx, &cy);
      rv = webShellWin->SetSize(aInnerWidth, cy, PR_FALSE);
      }
  }
  return rv;
}

NS_IMETHODIMP
GlobalWindowImpl::GetInnerHeight(PRInt32* aInnerHeight)
{
  nsIBrowserWindow *mBrowser;
  nsIDOMWindow* parent = nsnull;
  
  GetParent(&parent);
  if (parent == this) {
    // We are in a top level window.  Use browser window's bounds.    
    if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
      nsRect r;
      mBrowser->GetContentBounds(r);
      *aInnerHeight = r.height;
      NS_RELEASE(mBrowser);
    }
    else {
      *aInnerHeight = 0;
    }
  }
  else {
    // We are in an (i)frame.  Use webshell bounds.
    nsCOMPtr<nsIBaseWindow> webShellWin(do_QueryInterface(mWebShell));
    *aInnerHeight = 0;
    if(webShellWin)
      {
      PRInt32 cx;
      webShellWin->GetSize(&cx, aInnerHeight);
      }    
  }

  NS_RELEASE(parent);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetInnerHeight(PRInt32 aInnerHeight)
{
  nsresult         rv;
  nsIBrowserWindow *browser;
  nsIDOMWindow     *parent = nsnull;

  GetParent(&parent);
  if (parent == this) {
    if (NS_OK == (rv = GetBrowserWindowInterface(browser))) {
      nsRect r;
      browser->GetContentBounds(r);
      rv = browser->SizeContentTo(r.width, aInnerHeight);
      NS_RELEASE(browser);
    }
  } else {
    rv = NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIBaseWindow> webShellWin(do_QueryInterface(mWebShell));
    if(webShellWin)
      {
      PRInt32 cx = 0, cy = 0;
      webShellWin->GetSize(&cx, &cy);
      webShellWin->SetSize(cx, aInnerHeight, PR_FALSE);
      }
  }
  return rv;
}

NS_IMETHODIMP
GlobalWindowImpl::GetOuterWidth(PRInt32* aOuterWidth)
{
  nsIBrowserWindow *mBrowser;  
  
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);
    *aOuterWidth = r.width;
    NS_RELEASE(mBrowser);
  }
  else {
    *aOuterWidth = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetOuterWidth(PRInt32 aOuterWidth)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->SizeWindowTo(aOuterWidth, r.height, PR_FALSE, PR_FALSE);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetOuterHeight(PRInt32* aOuterHeight)
{
  nsIBrowserWindow *mBrowser;
    
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);
    *aOuterHeight = r.height;
    NS_RELEASE(mBrowser);
  }
  else {
    *aOuterHeight = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetOuterHeight(PRInt32 aOuterHeight)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->SizeWindowTo(r.width, aOuterHeight, PR_FALSE, PR_FALSE);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetScreenX(PRInt32* aScreenX)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);
    *aScreenX = r.x;
    NS_RELEASE(mBrowser);
  }
  else {
    *aScreenX = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetScreenX(PRInt32 aScreenX)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->MoveTo(aScreenX, r.y);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetScreenY(PRInt32* aScreenY)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);
    *aScreenY = r.y;
    NS_RELEASE(mBrowser);
  }
  else {
    *aScreenY = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetScreenY(PRInt32 aScreenY)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->MoveTo(r.x, aScreenY);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetPageXOffset(PRInt32* aPageXOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetPageXOffset(PRInt32 aPageXOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetPageYOffset(PRInt32* aPageYOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetPageYOffset(PRInt32 aPageYOffset)
{
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::Dump(const nsString& aStr)
{
  char *cstr = aStr.ToNewCString();
  
#ifdef XP_MAC
  // have to convert \r to \n so that printing to the console works
  char *c = cstr, *cEnd = cstr + aStr.Length();
  while (c < cEnd)
  {
    if (*c == '\r')
      *c = '\n';
    c++;
  }
#endif

  if (nsnull != cstr) {
    printf("%s", cstr);
    nsCRT::free(cstr);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
GlobalWindowImpl::Alert(JSContext *cx, jsval *argv, PRUint32 argc)
{
  nsAutoString str;

  NS_ENSURE_STATE(mWebShell);
  if (argc > 0) {
  nsJSUtils::nsConvertJSValToString(str, cx, argv[0]);
  }
  else {
  str.SetString("undefined");
  }

  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  mWebShell->GetTopLevelWindow(getter_AddRefs(topLevelWindow));
  NS_ENSURE_TRUE(topLevelWindow, NS_ERROR_FAILURE);

  //XXXWEBSHELL this should be a request off nsIInterfaceRequestor
  nsCOMPtr<nsIPrompt> prompter(do_QueryInterface(topLevelWindow));
  /*XXXEMBEDDING If it fails to get a prompter, it should use an internal
  // one
  if(!prompter)
  {                           //XXX This create instance should be the progid
  prompter = do_CreatInstance(NS_PROMPT_PROGID);
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);
  }
  */
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  return prompter->Alert(str.GetUnicode());
}

NS_IMETHODIMP    
GlobalWindowImpl::Confirm(JSContext *cx, jsval *argv, PRUint32 argc, PRBool* aReturn)
{
  nsAutoString str;

  *aReturn = PR_FALSE;
  NS_ENSURE_STATE(mWebShell);
  if (argc > 0) {
  nsJSUtils::nsConvertJSValToString(str, cx, argv[0]);
  }
  else {
  str.SetString("undefined");
  }

  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  mWebShell->GetTopLevelWindow(getter_AddRefs(topLevelWindow));
  NS_ENSURE_TRUE(topLevelWindow, NS_ERROR_FAILURE);

  //XXXWEBSHELL this should be a request off nsIInterfaceRequestor
  nsCOMPtr<nsIPrompt> prompter(do_QueryInterface(topLevelWindow));
  /*XXXEMBEDDING If it fails to get a prompter, it should use an internal
  // one
  if(!prompter)
  {                           //XXX This create instance should be the progid
  prompter = do_CreatInstance(NS_PROMPT_PROGID);
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);
  }
  */
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  return prompter->Confirm(str.GetUnicode(), aReturn);
}

NS_IMETHODIMP    
GlobalWindowImpl::Prompt(JSContext *cx, jsval *argv, PRUint32 argc, nsString& aReturn)
{
  nsresult ret = NS_OK;
  nsAutoString str, initial;

  aReturn.Truncate();
  NS_ENSURE_STATE(mWebShell);
  if (argc > 0) {
   nsJSUtils::nsConvertJSValToString(str, cx, argv[0]);
   
   if (argc > 1) {
     nsJSUtils::nsConvertJSValToString(initial, cx, argv[1]);
   }
   else {
     initial.SetString("undefined");
   }
  }

  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  mWebShell->GetTopLevelWindow(getter_AddRefs(topLevelWindow));
  NS_ENSURE_TRUE(topLevelWindow, NS_ERROR_FAILURE);

  //XXXWEBSHELL this should be a request off nsIInterfaceRequestor
  nsCOMPtr<nsIPrompt> prompter(do_QueryInterface(topLevelWindow));
  /*XXXEMBEDDING If it fails to get a prompter, it should use an internal
  // one
  if(!prompter)
  {                           //XXX This create instance should be the progid
  prompter = do_CreatInstance(NS_PROMPT_PROGID);
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);
  }
  */
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  PRBool b;
  PRUnichar* uniResult = nsnull;
  ret = prompter->Prompt(str.GetUnicode(), initial.GetUnicode(), &uniResult, &b);
  aReturn = uniResult;
  if (uniResult) {
    nsAllocator::Free(uniResult);
  }
  if (NS_FAILED(ret) || !b) {
    // XXX Need to check return value and return null if the
    // user hits cancel. Currently, we can only return a 
    // string reference.
    aReturn.SetString("");
  }

  return ret;
}

NS_IMETHODIMP
GlobalWindowImpl::Focus()
{
  nsIBrowserWindow *browser;
  if (NS_OK == GetBrowserWindowInterface(browser)) {
    browser->Show();
    NS_RELEASE( browser);
  }

  nsresult result = NS_OK;

  nsIContentViewer *viewer = nsnull;
  mWebShell->GetContentViewer(&viewer);
  if (viewer) {
    nsIDocumentViewer* docv = nsnull;
    viewer->QueryInterface(kIDocumentViewerIID, (void**) &docv);
    if (nsnull != docv) {
      nsIPresContext* cx = nsnull;
      docv->GetPresContext(cx);
      if (nsnull != cx) {
        nsIPresShell  *shell = nsnull;
        cx->GetShell(&shell);
        if (nsnull != shell) {
          nsIViewManager  *vm = nsnull;
          shell->GetViewManager(&vm);
          if (nsnull != vm) {
            nsIView *rootview = nsnull;
            vm->GetRootView(rootview);
            if (rootview) {
              nsIWidget* widget;
              rootview->GetWidget(widget);
              if (widget) {
                result = widget->SetFocus();
                NS_RELEASE(widget);
              }
            }
            NS_RELEASE(vm);
          }
          NS_RELEASE(shell);
        }
        NS_RELEASE(cx);
      }
      NS_RELEASE(docv);
    }
    NS_RELEASE(viewer);
  }

  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::Blur()
{
  nsresult result = NS_OK;
  if (nsnull != mWebShell) {
    result = mWebShell->RemoveFocus();
  }
  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::Close()
{
  // Note: the basic security check, rejecting windows not opened through JS,
  // has been removed. This was approved long ago by ...you're going to call me
  // on this, aren't you... well it was. And anyway, a better means is coming.
  // In the new world of application-level interfaces being written in JS, this
  // security check was causing problems.
  nsIBrowserWindow *mBrowser;
  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    mBrowser->Close();
    NS_RELEASE(mBrowser);
  }

  CleanUp();
  return NS_OK;
}

void
GlobalWindowImpl::CloseWindow(nsISupports* aWindow)
{
  nsCOMPtr<nsIDOMWindow> win = do_QueryInterface(aWindow);

  win->Close();
}

NS_IMETHODIMP    
GlobalWindowImpl::Close(JSContext* cx, jsval* argv, PRUint32 argc)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIScriptContext> callingContext;
  nsCOMPtr<nsIScriptContext> winContext;
  nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(mWebShell));

  nsJSUtils::nsGetDynamicScriptContext(cx, getter_AddRefs(callingContext));
  if (globalObject && callingContext) {
    if (NS_SUCCEEDED(result)) {
      result = globalObject->GetContext(getter_AddRefs(winContext));
      if (NS_SUCCEEDED(result)) {
        if (winContext == callingContext) {
          result = callingContext->SetTerminationFunction(CloseWindow, (nsISupports*)(nsIScriptGlobalObject*)this);
        }
        else {
          result = Close();
        }
      }
    }
  }
  return result;
}

NS_IMETHODIMP    
GlobalWindowImpl::Escape(const nsString& aStr, nsString& aReturn)
{
  nsresult result;
  nsCOMPtr<nsIUnicodeEncoder> encoder;
  nsAutoString charset;

  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &result);
  if (NS_FAILED(result) || !ccm) {
    return result;
  }

  // Get the document character set
  charset = "UTF-8"; // default to utf-8
  if (mDocument) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);

    if (doc) {
      result = doc->GetDocumentCharacterSet(charset);
    }
  }
  if (NS_FAILED(result)) {
    return result;
  }

  // Get an encoder for the character set
  result = ccm->GetUnicodeEncoder(&charset, getter_AddRefs(encoder));
  if (NS_FAILED(result)) {
    return result;
  }

  result = encoder->Reset();
  if (NS_FAILED(result)) {
    return result;
  }

  PRInt32 maxByteLen, srcLen;
  srcLen = aStr.Length();
  const PRUnichar* src = aStr.GetUnicode();

  // Get the expected length of result string
  result = encoder->GetMaxLength(src, srcLen, &maxByteLen);
  if (NS_FAILED(result)) {
    return result;
  }

  // Allocate a buffer of the maximum length
  char* dest = (char*)nsAllocator::Alloc(maxByteLen+1);
  PRInt32 destLen2, destLen = maxByteLen;
  if (nsnull == dest) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  // Convert from unicode to the character set
  result = encoder->Convert(src, &srcLen, dest, &destLen);
  if (NS_FAILED(result)) {
    nsAllocator::Free(dest);
    return result;
  }

  // Allow the encoder to finish the conversion
  destLen2 = maxByteLen - destLen;
  encoder->Finish(dest+destLen, &destLen2);
  dest[destLen+destLen2] = '\0';

  // Escape the string
  char* outBuf = nsEscape(dest, nsEscapeMask(url_XAlphas | url_XPAlphas | url_Path)); 
  aReturn.SetString(outBuf);

  nsAllocator::Free(outBuf);
  nsAllocator::Free(dest);

  return result;
}
 
NS_IMETHODIMP    
GlobalWindowImpl::Unescape(const nsString& aStr, nsString& aReturn)
{
  nsresult result;
  nsCOMPtr<nsIUnicodeDecoder> decoder;
  nsAutoString charset;

  NS_WITH_SERVICE(nsICharsetConverterManager, ccm, kCharsetConverterManagerCID, &result);
  if (NS_FAILED(result) || !ccm) {
    return result;
  }

  // Get the document character set
  charset = "UTF-8"; // default to utf-8
  if (mDocument) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mDocument);

    if (doc) {
      result = doc->GetDocumentCharacterSet(charset);
    }
  }
  if (NS_FAILED(result)) {
    return result;
  }

  // Get an decoder for the character set
  result = ccm->GetUnicodeDecoder(&charset, getter_AddRefs(decoder));
  if (NS_FAILED(result)) {
    return result;
  }

  result = decoder->Reset();
  if (NS_FAILED(result)) {
    return result;
  }

  // Need to copy to do the two-byte to one-byte deflation
  char* inBuf = aStr.ToNewCString();
  if (nsnull == inBuf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Unescape the string
  char* src = nsUnescape(inBuf);
//  nsAllocator::Free(inBuf);
  if (nsnull == src) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 maxLength, srcLen;
  srcLen = aStr.Length();

  // Get the expected length of the result string
  result = decoder->GetMaxLength(src, srcLen, &maxLength);
  if (NS_FAILED(result)) {
    nsAllocator::Free(src);
    return result;
  }

  // Allocate a buffer of the maximum length
  PRUnichar* dest = (PRUnichar*)nsAllocator::Alloc(sizeof(PRUnichar)*maxLength);
  PRInt32 destLen = maxLength;
  if (nsnull == dest) {
    nsAllocator::Free(src);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Convert from character set to unicode
  result = decoder->Convert(src, &srcLen, dest, &destLen);
  nsAllocator::Free(src);
  if (NS_FAILED(result)) {
    nsAllocator::Free(dest);
    return result;
  }

  aReturn.SetString(dest, destLen);
  nsAllocator::Free(dest);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::Forward()
{
  nsresult result = NS_OK;
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    //XXX tbi
    //mBrowser->Forward();
    NS_RELEASE(mBrowser);
  } else if (nsnull != mWebShell) {
    result = mWebShell->Forward(); // I added this - rods
  }
  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::Back()
{
  nsresult result = NS_OK;
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    //XXX tbi
    //mBrowser->Back();
    NS_RELEASE(mBrowser);
  }  else if (nsnull != mWebShell) {
    result = mWebShell->Back();// I added this - rods
  }
  return result;  
}

NS_IMETHODIMP
GlobalWindowImpl::Home()
{
  char *url = nsnull;
  nsresult rv = NS_OK;
  nsString homeURL;

  if (nsnull == mWebShell) {
    return rv;
  }

  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (NS_FAILED(rv) || !prefs) {
    return rv;
  }

  // if we get here, we know prefs is not null
  rv = prefs->CopyCharPref(PREF_BROWSER_STARTUP_HOMEPAGE, &url);
  if (NS_FAILED(rv) || (!url)) {
    // if all else fails, use this
#ifdef DEBUG_seth
    printf("all else failed.  using %s as the home page\n",DEFAULT_HOME_PAGE);
#endif
    homeURL = DEFAULT_HOME_PAGE;
  }
  else {
    homeURL = url;
  }
  PR_FREEIF(url);
  PRUnichar* urlToLoad = homeURL.ToNewUnicode();
  rv = mWebShell->LoadURL(urlToLoad);
  delete[] urlToLoad;
  return rv;
}

NS_IMETHODIMP
GlobalWindowImpl::Stop()
{
  nsresult result = NS_OK;
  if (nsnull != mWebShell) {
    result = mWebShell->Stop();
  }
  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::Print()
{
  nsresult result = NS_OK;
  if (nsnull != mWebShell) {
    nsCOMPtr<nsIContentViewer> viewer;    
    mWebShell->GetContentViewer(getter_AddRefs(viewer));    
    if (nsnull != viewer) {
      nsCOMPtr<nsIContentViewerFile> viewerFile = do_QueryInterface(viewer);
      if (viewerFile) {
        result = viewerFile->Print();
      }
    }
  }

  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::MoveTo(PRInt32 aXPos, PRInt32 aYPos)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    mBrowser->MoveTo(aXPos, aYPos);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::MoveBy(PRInt32 aXDif, PRInt32 aYDif)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->MoveTo(r.x + aXDif, r.y + aYDif);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::ResizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    mBrowser->SizeWindowTo(aWidth, aHeight, PR_FALSE, PR_FALSE);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::ResizeBy(PRInt32 aWidthDif, PRInt32 aHeightDif)
{
  nsIBrowserWindow *mBrowser;

  if (NS_OK == GetBrowserWindowInterface(mBrowser)) {
    nsRect r;
    mBrowser->GetWindowBounds(r);

    mBrowser->SizeWindowTo(r.width + aWidthDif, r.height + aHeightDif,
                           PR_FALSE, PR_FALSE);
    NS_RELEASE(mBrowser);
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SizeToContent()
{
  if (mWebShell)
    return mWebShell->SizeToContent();
  return NS_ERROR_NOT_INITIALIZED;
}

NS_IMETHODIMP
GlobalWindowImpl::GetAttention()
{
#if 0
  if (mWebShell)
    return mWebShell->GetAttention();
#endif
  return NS_OK;
}

nsresult
GlobalWindowImpl::GetScrollInfo(nsIScrollableView** aScrollableView,
                                float* aP2T, float* aT2P)
{
  nsresult result = NS_OK;

  nsCOMPtr<nsIContentViewer> viewer;
  result = mWebShell->GetContentViewer(getter_AddRefs(viewer));
  if (NS_SUCCEEDED(result) && viewer) {
    nsCOMPtr<nsIDocumentViewer> docv = do_QueryInterface(viewer, &result);
    if (NS_SUCCEEDED(result)) {
      nsCOMPtr<nsIPresContext> cx;
      result = docv->GetPresContext(*getter_AddRefs(cx));
      if (NS_SUCCEEDED(result)) {

        cx->GetPixelsToTwips(aP2T);
        cx->GetTwipsToPixels(aT2P);

        nsCOMPtr<nsIPresShell> shell;
        result = cx->GetShell(getter_AddRefs(shell));
        if (NS_SUCCEEDED(result)) {
          nsCOMPtr<nsIViewManager> vm;
          result = shell->GetViewManager(getter_AddRefs(vm));
          if (NS_SUCCEEDED(result)) {
            result = vm->GetRootScrollableView(aScrollableView);
          }
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
GlobalWindowImpl::Scroll(PRInt32 aXScroll, PRInt32 aYScroll)
{
  return ScrollTo(aXScroll, aYScroll);
}

NS_IMETHODIMP
GlobalWindowImpl::ScrollTo(PRInt32 aXScroll, PRInt32 aYScroll)
{
  nsresult result;
  nsIScrollableView* view;
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (NS_SUCCEEDED(result) && view) {
    result = view->ScrollTo(NSIntPixelsToTwips(aXScroll, p2t),
                            NSIntPixelsToTwips(aYScroll, p2t), 
                            NS_VMREFRESH_IMMEDIATE);
  }
  
  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::ScrollBy(PRInt32 aXScrollDif, PRInt32 aYScrollDif)
{
  nsresult result;
  nsIScrollableView* view;
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (NS_SUCCEEDED(result) && view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    if (NS_SUCCEEDED(result)) {
      result = view->ScrollTo(xPos + NSIntPixelsToTwips(aXScrollDif, p2t),
                              yPos + NSIntPixelsToTwips(aYScrollDif, p2t), 
                              NS_VMREFRESH_IMMEDIATE);
    }
  }

  return result;
}

NS_IMETHODIMP
GlobalWindowImpl::GetScrollX(PRInt32* aScrollX)
{
  nsresult result;
  nsIScrollableView* view;
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (NS_SUCCEEDED(result) && view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    *aScrollX = NSTwipsToIntPixels(xPos, t2p);
  }

  return result;
}

NS_IMETHODIMP    
GlobalWindowImpl::GetScrollY(PRInt32* aScrollY)
{
  nsresult result;
  nsIScrollableView* view;
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (NS_SUCCEEDED(result) && view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    *aScrollY = NSTwipsToIntPixels(yPos, t2p);
  }

  return result;
}


nsresult
GlobalWindowImpl::ClearTimeoutOrInterval(PRInt32 aTimerID)
{
  PRUint32 public_id;
  nsTimeoutImpl **top, *timeout;

  public_id = (PRUint32)aTimerID;
  if (!public_id)    /* id of zero is reserved for internal use */
    return NS_ERROR_FAILURE;
  for (top = &mTimeouts; (timeout = *top) != NULL; top = &timeout->next) {
    if (timeout->public_id == public_id) {
      if (mRunningTimeout == timeout) {
        /* We're running from inside the timeout.  Mark this
           timeout for deferred deletion by the code in
           win_run_timeout() */
        timeout->interval = 0;
      } 
      else {
        /* Delete the timeout from the pending timeout list */
        *top = timeout->next;
        if (timeout->timer) {
          timeout->timer->Cancel();
          NS_RELEASE(timeout->timer);
          DropTimeout(timeout);
        }
        DropTimeout(timeout);
      }
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::ClearTimeout(PRInt32 aTimerID)
{
  return ClearTimeoutOrInterval(aTimerID);
}

NS_IMETHODIMP    
GlobalWindowImpl::ClearInterval(PRInt32 aTimerID)
{
  return ClearTimeoutOrInterval(aTimerID);
}

void
GlobalWindowImpl::ClearAllTimeouts()
{
  nsTimeoutImpl *timeout, *next;

  for (timeout = mTimeouts; timeout; timeout = next) {
    /* If RunTimeout() is higher up on the stack for this
       window, e.g. as a result of document.write from a timeout,
       then we need to reset the list insertion point for
       newly-created timeouts in case the user adds a timeout,
       before we pop the stack back to RunTimeout. */
    if (mRunningTimeout == timeout) {
      mTimeoutInsertionPoint = nsnull;
    }
        
    next = timeout->next;
    if (timeout->timer) {
      timeout->timer->Cancel();
      NS_RELEASE(timeout->timer);
      // Drop the count since the timer isn't going to hold on
      // anymore.
      DropTimeout(timeout);
    }
    // Drop the count since we're removing it from the list.
    DropTimeout(timeout);
  }
   
  mTimeouts = NULL;
}

void
GlobalWindowImpl::HoldTimeout(nsTimeoutImpl *aTimeout)
{
  aTimeout->ref_count++;
}

void
GlobalWindowImpl::DropTimeout(nsTimeoutImpl *aTimeout,
                              nsIScriptContext* aContext)
{
  JSContext *cx;
  
  if (--aTimeout->ref_count > 0) {
    return;
  }
  
  if (!aContext)
    aContext = mContext;
  if (aContext) {
    cx = (JSContext *) aContext->GetNativeContext();
 
    if (aTimeout->expr) {
      JS_RemoveRoot(cx, &aTimeout->expr);
    }
    else if (aTimeout->funobj) {
      JS_RemoveRoot(cx, &aTimeout->funobj);
      if (aTimeout->argv) {
        int i;
        for (i = 0; i < aTimeout->argc; i++)
          JS_RemoveRoot(cx, &aTimeout->argv[i]);
        PR_FREEIF(aTimeout->argv);
      }
    }
  }
  NS_IF_RELEASE(aTimeout->timer);
  PR_FREEIF(aTimeout->filename);
  NS_IF_RELEASE(aTimeout->window);
  NS_IF_RELEASE(aTimeout->principal);
  PR_DELETE(aTimeout);
}

void
GlobalWindowImpl::InsertTimeoutIntoList(nsTimeoutImpl **aList, 
                                        nsTimeoutImpl *aTimeout)
{
  nsTimeoutImpl *to;
  
  while ((to = *aList) != nsnull) {
    if (LL_CMP(to->when, >, aTimeout->when))
      break;
    aList = &to->next;
  }
  aTimeout->next = to;
  *aList = aTimeout;
  // Increment the ref_count since we're in the list
  HoldTimeout(aTimeout);
}

void
nsGlobalWindow_RunTimeout(nsITimer *aTimer, void *aClosure)
{
  nsTimeoutImpl *timeout = (nsTimeoutImpl *)aClosure;

  if (timeout->window->RunTimeout(timeout)) {
    // Drop the ref_count since the timer won't be holding on to the
    // timeout struct anymore
    timeout->window->DropTimeout(timeout);
  }
}

PRBool
GlobalWindowImpl::RunTimeout(nsTimeoutImpl *aTimeout)
{
    nsTimeoutImpl *next, *timeout;
    nsTimeoutImpl *last_expired_timeout;
    nsTimeoutImpl dummy_timeout;
    JSContext *cx;
    PRInt64 now;
    nsITimer *timer;
    nsresult rv;

    /* Make sure that the window or the script context don't go away as 
       a result of running timeouts */
    GlobalWindowImpl* temp = this;
    NS_ADDREF(temp);
    nsIScriptContext* tempContext = mContext;
    NS_ADDREF(tempContext);

    timer = aTimeout->timer;
    cx = (JSContext *)mContext->GetNativeContext();

    /* A native timer has gone off.  See which of our timeouts need
       servicing */
    LL_I2L(now, PR_IntervalNow());

    /* The timeout list is kept in deadline order.  Discover the
       latest timeout whose deadline has expired. On some platforms,
       native timeout events fire "early", so we need to test the
       timer as well as the deadline. */
    last_expired_timeout = nsnull;
    for (timeout = mTimeouts; timeout; timeout = timeout->next) {
        if ((timeout == aTimeout) || !LL_CMP(timeout->when, >, now))
            last_expired_timeout = timeout;
    }

    /* Maybe the timeout that the event was fired for has been deleted
       and there are no others timeouts with deadlines that make them
       eligible for execution yet.  Go away. */
    if (!last_expired_timeout) {
      NS_RELEASE(temp);
      NS_RELEASE(tempContext);
      return PR_TRUE;
    }

    /* Insert a dummy timeout into the list of timeouts between the portion
       of the list that we are about to process now and those timeouts that
       will be processed in a future call to win_run_timeout().  This dummy
       timeout serves as the head of the list for any timeouts inserted as
       a result of running a timeout. */
    dummy_timeout.timer = NULL;
    dummy_timeout.public_id = 0;
    dummy_timeout.next = last_expired_timeout->next;
    last_expired_timeout->next = &dummy_timeout;

    /* Don't let ClearWindowTimeouts throw away our stack-allocated
       dummy timeout. */
    dummy_timeout.ref_count = 2;
   
    mTimeoutInsertionPoint = &dummy_timeout.next;
    
    for (timeout = mTimeouts; timeout != &dummy_timeout; timeout = next) {
      next = timeout->next;
      /* Hold the timeout in case expr or funobj releases its doc. */
      HoldTimeout(timeout);
      mRunningTimeout = timeout;

      if (timeout->expr) {
        /* Evaluate the timeout expression. */
        nsAutoString script = JS_GetStringChars(timeout->expr);
        nsAutoString blank = "";
        PRBool isUndefined;
        rv = mContext->EvaluateString(script,
                                      mScriptObject,
                                      timeout->principal,
                                      timeout->filename,
                                      timeout->lineno,
                                      timeout->version,
                                      blank,
                                      &isUndefined);
      } else {
        PRInt64 lateness64;
        PRInt32 lateness;

        /* Add "secret" final argument that indicates timeout
           lateness in milliseconds */
        LL_SUB(lateness64, now, timeout->when);
        LL_L2I(lateness, lateness64);
        lateness = PR_IntervalToMilliseconds(lateness);
        timeout->argv[timeout->argc] = INT_TO_JSVAL((jsint)lateness);
        PRBool aBoolResult;
        rv = mContext->CallEventHandler(mScriptObject, timeout->funobj,
                                        timeout->argc + 1, timeout->argv,
                                        &aBoolResult);
      }
      if (NS_FAILED(rv)) {
        NS_RELEASE(temp);
        NS_RELEASE(tempContext);
        return PR_TRUE;
      }

      mRunningTimeout = nsnull;
      /* If the temporary reference is the only one that is keeping
         the timeout around, the document was released and we should
         restart this function. */
      if (timeout->ref_count == 1) {
        DropTimeout(timeout, tempContext);
        NS_RELEASE(temp);
        NS_RELEASE(tempContext);
        return PR_FALSE;
      }
      DropTimeout(timeout, tempContext);

      /* If we have a regular interval timer, we re-fire the
       *  timeout, accounting for clock drift.
       */
      if (timeout->interval) {
        /* Compute time to next timeout for interval timer. */
        PRInt32 delay32;
        PRInt64 interval, delay;
        LL_I2L(interval, PR_MillisecondsToInterval(timeout->interval));
        LL_ADD(timeout->when, timeout->when, interval);
        LL_I2L(now, PR_IntervalNow());
        LL_SUB(delay, timeout->when, now);
        LL_L2I(delay32, delay);

        /* If the next interval timeout is already supposed to
         *  have happened then run the timeout immediately.
         */
        if (delay32 < 0) {
          delay32 = 0;
        }
        delay32 = PR_IntervalToMilliseconds(delay32);

        NS_IF_RELEASE(timeout->timer);

        /* Reschedule timeout.  Account for possible error return in
           code below that checks for zero toid. */
        nsresult err = NS_NewTimer(&timeout->timer);
        if (NS_OK != err) {
          NS_RELEASE(temp);
          NS_RELEASE(tempContext);
          return PR_TRUE;
        } 
        
        err = timeout->timer->Init(nsGlobalWindow_RunTimeout, timeout, 
                                   delay32);
        if (NS_OK != err) {
          NS_RELEASE(temp);
          NS_RELEASE(tempContext);
          return PR_TRUE;
        } 
        // Increment ref_count to indicate that this timer is holding
        // on to the timeout struct.
        HoldTimeout(timeout);
      }

      /* Running a timeout can cause another timeout to be deleted,
         so we need to reset the pointer to the following timeout. */
      next = timeout->next;
      mTimeouts = next;
      // Drop timeout struct since it's out of the list
      DropTimeout(timeout, tempContext);

      /* Free the timeout if this is not a repeating interval
       *  timeout (or if it was an interval timeout, but we were
       *  unsuccessful at rescheduling it.)
       */
      if (timeout->interval && timeout->timer) {
        /* Reschedule an interval timeout */
        /* Insert interval timeout onto list sorted in deadline order. */
        InsertTimeoutIntoList(mTimeoutInsertionPoint, timeout);
      }
    }
    /* Take the dummy timeout off the head of the list */
    mTimeouts = dummy_timeout.next;
    mTimeoutInsertionPoint = nsnull;

    /* Get rid of our temporary reference to ourselves and the 
       script context */
    NS_RELEASE(temp);
    NS_RELEASE(tempContext);
    return PR_TRUE;
}

static const char *kSetIntervalStr = "setInterval";
static const char *kSetTimeoutStr = "setTimeout";

nsresult
GlobalWindowImpl::SetTimeoutOrInterval(JSContext *cx,
                                       jsval *argv, 
                                       PRUint32 argc, 
                                       PRInt32* aReturn,
                                       PRBool aIsInterval)
{
  JSString *expr = nsnull;
  JSObject *funobj = nsnull;
  nsTimeoutImpl *timeout, **insertion_point;
  jsdouble interval;
  PRInt64 now, delta;

  if (argc < 2) {
    JS_ReportError(cx, "Function %s requires at least 2 parameters",
                   aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_FAILURE;
  }

  if (!JS_ValueToNumber(cx, argv[1], &interval)) {
    JS_ReportError(cx,
                   "Second argument to %s must be a millisecond interval",
                   aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  switch (JS_TypeOfValue(cx, argv[0])) {
    case JSTYPE_FUNCTION:
      funobj = JSVAL_TO_OBJECT(argv[0]);
      break;
    case JSTYPE_STRING:
    case JSTYPE_OBJECT:
      if (!(expr = JS_ValueToString(cx, argv[0])))
        return NS_ERROR_FAILURE;
      if (nsnull == expr)
        return NS_ERROR_OUT_OF_MEMORY;
      argv[0] = STRING_TO_JSVAL(expr);
      break;
    default:
      JS_ReportError(cx, "useless %s call (missing quotes around argument?)",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
      return NS_ERROR_FAILURE;
  }

  timeout = PR_NEWZAP(nsTimeoutImpl);
  if (nsnull == timeout)
    return NS_ERROR_OUT_OF_MEMORY;

  // Initial ref_count to indicate that this timeout struct will
  // be held as the closure of a timer.
  timeout->ref_count = 1;
  if (aIsInterval)
    timeout->interval = (PRInt32)interval;
  if (expr) {
    if (!JS_AddNamedRoot(cx, &timeout->expr, "timeout.expr")) {
      PR_DELETE(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    timeout->expr = expr;
  }
  else if (funobj) {
    int i;

    /* Leave an extra slot for a secret final argument that
       indicates to the called function how "late" the timeout is. */
    timeout->argv = (jsval *)PR_MALLOC((argc - 1) * sizeof(jsval));
    if (nsnull == timeout->argv) {
      DropTimeout(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!JS_AddNamedRoot(cx, &timeout->funobj, "timeout.funobj")) {
      DropTimeout(timeout);
      return NS_ERROR_FAILURE;
    }
    timeout->funobj = funobj;
          
    timeout->argc = 0;
    for (i = 2; (PRUint32)i < argc; i++) {
      timeout->argv[i - 2] = argv[i];
      if (!JS_AddNamedRoot(cx, &timeout->argv[i - 2], "timeout.argv[i]")) {
        DropTimeout(timeout);
        return NS_ERROR_FAILURE;
      }
      timeout->argc++;
    }
  }

  const char* filename;
  if (nsJSUtils::nsGetCallingLocation(cx, &filename, &timeout->lineno)) {
    timeout->filename = PL_strdup(filename);
    if (!timeout->filename) {
      DropTimeout(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  timeout->version = JS_VersionToString(JS_GetVersion(cx));

  // Get principal of currently executing code, save for execution of timeout
  nsresult rv;  
  NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager,
                  NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  if (NS_FAILED(securityManager->GetSubjectPrincipal(&timeout->principal)))
    return NS_ERROR_FAILURE;

  LL_I2L(now, PR_IntervalNow());
  LL_D2L(delta, PR_MillisecondsToInterval((PRUint32)interval));
  LL_ADD(timeout->when, now, delta);
  nsresult err = NS_NewTimer(&timeout->timer);
  if (NS_OK != err) {
    DropTimeout(timeout);
    return err;
  } 
  
  err = timeout->timer->Init(nsGlobalWindow_RunTimeout, timeout, 
                             (PRInt32)interval);
  if (NS_OK != err) {
    DropTimeout(timeout);
    return err;
  } 
  timeout->window = this;
  NS_ADDREF(this);
  insertion_point = (mTimeoutInsertionPoint == NULL)
                    ? &mTimeouts
                    : mTimeoutInsertionPoint;

  InsertTimeoutIntoList(insertion_point, timeout);
  timeout->public_id = ++mTimeoutPublicIdCounter;
  *aReturn = timeout->public_id;
  
  return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::SetTimeout(JSContext *cx,
                             jsval *argv, 
                             PRUint32 argc, 
                             PRInt32* aReturn)
{
  return SetTimeoutOrInterval(cx, argv, argc, aReturn, PR_FALSE);
}

NS_IMETHODIMP    
GlobalWindowImpl::SetInterval(JSContext *cx,
                              jsval *argv, 
                              PRUint32 argc, 
                              PRInt32* aReturn)
{
  return SetTimeoutOrInterval(cx, argv, argc, aReturn, PR_TRUE);
}

NS_IMETHODIMP    
GlobalWindowImpl::Open(JSContext *cx,
                       jsval *argv, 
                       PRUint32 argc, 
                       nsIDOMWindow** aReturn)
{
  return OpenInternal(cx, argv, argc, PR_FALSE, aReturn);
}

// like Open, but attaches to the new window any extra parameters past [features]
// as a JS property named "arguments"
NS_IMETHODIMP    
GlobalWindowImpl::OpenDialog(JSContext *cx,
                             jsval *argv, 
                             PRUint32 argc, 
                             nsIDOMWindow** aReturn)
{
  return OpenInternal(cx, argv, argc, PR_TRUE, aReturn);
}

nsresult    
GlobalWindowImpl::OpenInternal(JSContext *cx,
                               jsval *argv, 
                               PRUint32 argc, 
                               PRBool aDialog,
                               nsIDOMWindow** aReturn)
{
  nsresult rv;
  PRUint32 chromeFlags;
  nsAutoString mAbsURL, name;
  JSString* str;
  char* options;
  *aReturn = nsnull;
  PRBool nameSpecified = PR_FALSE;
  PRBool loadURL = PR_TRUE;

  if (argc == 0) {
    loadURL = PR_FALSE;
    
  } else if (argc > 0) {
    JSString *mJSStrURL = JS_ValueToString(cx, argv[0]);
    if (nsnull == mJSStrURL) {
      return NS_ERROR_FAILURE;
    }

    nsAutoString mURL;
    mURL.SetString(JS_GetStringChars(mJSStrURL));
    
    if (mURL.Equals("")) {
      loadURL = PR_FALSE;

    } else {
      if (mDocument) {
        // Build absolute URL relative to this document.
        nsIURI* mDocURL = 0;
        nsIDocument* mDoc;
    
        if (NS_OK == mDocument->QueryInterface(kIDocumentIID, (void**)&mDoc)) {
          mDocURL = mDoc->GetDocumentURL();
          NS_RELEASE(mDoc);
        }
    
        nsIURI *baseUri = nsnull;
        rv = mDocURL->QueryInterface(NS_GET_IID(nsIURI), (void**)&baseUri);
        if (NS_FAILED(rv)) return rv;
    
        rv = NS_MakeAbsoluteURI(mURL, baseUri, mAbsURL);
        NS_RELEASE(baseUri);
      } else {
        // No document.  Probably because this window's URL hasn't finished
        // loading.  All we can do is hope the URL we've been given is absolute.
        mAbsURL.SetString(JS_GetStringChars(mJSStrURL));
        nsCOMPtr<nsIURI> test;
        // Make URI; if mAbsURL is relative (or otherwise bogus) this will fail.
        rv = NS_NewURI( getter_AddRefs(test), mAbsURL );
      }
      if (NS_FAILED(rv)) return rv;
    }
  }
  
  /* Sanity-check the optional window_name argument. */
  if (argc > 1) {
    JSString *mJSStrName = JS_ValueToString(cx, argv[1]);
    if (nsnull == mJSStrName) {
      return NS_ERROR_FAILURE;
    }
    name.SetString(JS_GetStringChars(mJSStrName));
    nameSpecified = PR_TRUE;

    if (NS_OK != CheckWindowName(cx, name)) {
      return NS_ERROR_FAILURE;
    }
  } 

  options = nsnull;
  if (argc > 2) {
    if (!(str = JS_ValueToString(cx, argv[2]))) {
      return NS_ERROR_FAILURE;
    }
    options = JS_GetStringBytes(str);
  }
  chromeFlags = CalculateChromeFlags(options, aDialog);

  nsIWebShell *newOuterShell = nsnull;
  nsIWebShellContainer *webShellContainer;

  /* XXX check for existing window of same name.  If exists, set url and 
   * update chrome */
  if ((nsnull != mWebShell) && 
      (NS_OK == mWebShell->GetContainer(webShellContainer)) && 
      (nsnull != webShellContainer)) {

    PRBool windowIsNew;
    PRBool windowIsModal;
    nsIEventQueue *modalEventQueue = nsnull;

    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);

    // Check for existing window of same name.
    windowIsNew = PR_FALSE;
    windowIsModal = PR_FALSE;
    if (nameSpecified) {
      webShellContainer->FindWebShellWithName(name.GetUnicode(), newOuterShell);
    }
    if (nsnull == newOuterShell) {
      windowIsNew = PR_TRUE;
      if (chromeFlags & NS_CHROME_MODAL) {
        if (eventQService && NS_SUCCEEDED(eventQService->PushThreadEventQueue(&modalEventQueue)))
          windowIsModal = PR_TRUE;
      }

      // No window of that name, and we are allowed to create a new one now.
      webShellContainer->NewWebShell(chromeFlags, PR_FALSE, newOuterShell);
    }

    if (nsnull != newOuterShell) {
      if (NS_SUCCEEDED(ReadyOpenedWebShell(newOuterShell, aReturn))) {
        if (aDialog && argc > 3)
          AttachArguments(*aReturn, argv+3, argc-3);

        if (loadURL) {
          // Get security manager, check to see if URI is allowed.
          nsCOMPtr<nsIURI> newUrl;
          nsCOMPtr<nsIScriptSecurityManager> secMan;
          nsCOMPtr<nsIScriptContext> scriptCX;
          nsJSUtils::nsGetStaticScriptContext(cx, (JSObject*) mScriptObject, 
                                              getter_AddRefs(scriptCX));
          if (!scriptCX ||
              NS_FAILED(scriptCX->GetSecurityManager(getter_AddRefs(secMan))) ||
              NS_FAILED(NS_NewURI(getter_AddRefs(newUrl), mAbsURL)) ||
              NS_FAILED(secMan->CheckLoadURIFromScript(cx, newUrl))) 
            {
              NS_RELEASE(newOuterShell);
              NS_RELEASE(webShellContainer);
              return NS_ERROR_FAILURE;
            }
        }
        
        nsCOMPtr<nsIDocShellTreeItem> newShellAsItem(do_QueryInterface(newOuterShell));
        NS_ASSERTION(newShellAsItem, "We got a shell that isn't an item!"); 
        if (nameSpecified) {
          newShellAsItem->SetName(name.GetUnicode());
        }
        else {
          newShellAsItem->SetName(nsnull);
        }

        if (loadURL) {
          newOuterShell->LoadURL(mAbsURL.GetUnicode());
        }
        
        SizeAndShowOpenedWebShell(newOuterShell, options, windowIsNew, aDialog);
        if (windowIsModal) {
          nsIBrowserWindow *newWindow;
          GetBrowserWindowInterface(newWindow, newOuterShell);
          if (nsnull != newWindow) {
            newWindow->ShowModally(PR_FALSE);
            NS_RELEASE(newWindow);
          }
          eventQService->PopThreadEventQueue(modalEventQueue);
        }
      }
      NS_RELEASE(newOuterShell);
    }
    NS_RELEASE(webShellContainer);
  }

  return NS_OK;
}

// attach the given array of JS values to the given window, as a property array
// named "arguments"
nsresult
GlobalWindowImpl::AttachArguments(nsIDOMWindow *aWindow, jsval *argv, PRUint32 argc)
{
  if (argc == 0)
    return NS_OK;

  // copy the extra parameters into a JS Array and attach it
  JSObject *args;
  JSObject *scriptObject;
  nsIScriptGlobalObject *scriptGlobal;
  nsIScriptObjectOwner *owner;
  JSContext *jsContext;
  nsIScriptContext *scriptContext;

  if (NS_SUCCEEDED(aWindow->QueryInterface(NS_GET_IID(nsIScriptGlobalObject), (void **)&scriptGlobal))) {
    scriptGlobal->GetContext(&scriptContext);
    if (scriptContext) {
      jsContext = (JSContext *) scriptContext->GetNativeContext();
      if (NS_SUCCEEDED(aWindow->QueryInterface(NS_GET_IID(nsIScriptObjectOwner), (void**)&owner))) {
        owner->GetScriptObject(scriptContext, (void **) &scriptObject);
        args = JS_NewArrayObject(jsContext, argc, argv);
        if (args) {
          jsval argsVal = OBJECT_TO_JSVAL(args);
//        JS_DefineProperty(jsContext, scriptObject, "arguments",
//          argsVal, NULL, NULL, JSPROP_PERMANENT);
          JS_SetProperty(jsContext, scriptObject, "arguments", &argsVal);
        }
        NS_RELEASE(owner);
      }
      NS_RELEASE(scriptContext);
    }
    NS_RELEASE(scriptGlobal);
  }
  return NS_OK;
}


/**
 * Calculate the chrome bitmask from a string list of features.
 * @param aFeatures a string containing a list of named chrome features
 * @param aDialog affects the assumptions made about unnamed features
 * @return the chrome bitmask
 */
PRUint32
GlobalWindowImpl::CalculateChromeFlags(char *aFeatures, PRBool aDialog) {

  PRUint32  chromeFlags;

  if (nsnull == aFeatures)
    if (aDialog)
      return NS_CHROME_ALL_CHROME |
             NS_CHROME_OPEN_AS_DIALOG | NS_CHROME_OPEN_AS_CHROME;
    else
      return NS_CHROME_ALL_CHROME;

  PRBool presenceFlag = PR_FALSE;

  /* This function has become complicated since browser windows and
     dialogs diverged. The difference is, browser windows assume all
     chrome not explicitly mentioned is off, if the features string
     is not null. Exceptions are some OS border chrome new with Mozilla.
     Dialogs interpret a (mostly) empty features string to mean
     "OS's choice," and also support an "all" flag explicitly disallowed
     in the standards-compliant window.(normal)open. */

  chromeFlags = NS_CHROME_WINDOW_BORDERS_ON;
  if (aDialog && WinHasOption(aFeatures, "all", presenceFlag))
    chromeFlags = NS_CHROME_ALL_CHROME;

  /* Next, allow explicitly named options to override the initial settings */

  chromeFlags |= WinHasOption(aFeatures, "titlebar", presenceFlag) ? NS_CHROME_TITLEBAR_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "close", presenceFlag) ? NS_CHROME_WINDOW_CLOSE_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "toolbar", presenceFlag) ? NS_CHROME_TOOL_BAR_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "location", presenceFlag) ? NS_CHROME_LOCATION_BAR_ON : 0;
  chromeFlags |= (WinHasOption(aFeatures, "directories", presenceFlag) | WinHasOption(aFeatures, "personalbar", presenceFlag))
    ? NS_CHROME_PERSONAL_TOOLBAR_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "status", presenceFlag) ? NS_CHROME_STATUS_BAR_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "menubar", presenceFlag) ? NS_CHROME_MENU_BAR_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "scrollbars", presenceFlag) ? NS_CHROME_SCROLLBARS_ON : 0;
  chromeFlags |= WinHasOption(aFeatures, "resizable", presenceFlag) ? NS_CHROME_WINDOW_RESIZE_ON : 0;

  /* OK.
       Normal browser windows, in spite of a stated pattern of turning off
     all chrome not mentioned explicitly, will want the new OS chrome (window
     borders, titlebars, closebox) on, unless explicitly turned off.
       Dialogs, on the other hand, take the absence of any explicit settings
     to mean "OS' choice." */

  // default titlebar and closebox to "on," if not mentioned at all
  if (!PL_strcasestr(aFeatures, "titlebar"))
    chromeFlags |= NS_CHROME_TITLEBAR_ON;
  if (!PL_strcasestr(aFeatures, "close"))
    chromeFlags |= NS_CHROME_WINDOW_CLOSE_ON;

  if (aDialog && !presenceFlag)
    chromeFlags = NS_CHROME_DEFAULT_CHROME;

  /* Finally, once all the above normal chrome has been divined, deal
     with the features that are more operating hints than appearance
     instructions. (Note modality implies dependence.) */

  chromeFlags |= WinHasOption(aFeatures, "chrome", presenceFlag) ? NS_CHROME_OPEN_AS_CHROME : 0;
  chromeFlags |= WinHasOption(aFeatures, "dependent", presenceFlag) ? NS_CHROME_DEPENDENT : 0;
  chromeFlags |= WinHasOption(aFeatures, "modal", presenceFlag) ? (NS_CHROME_MODAL | NS_CHROME_DEPENDENT) : 0;
  chromeFlags |= WinHasOption(aFeatures, "dialog", presenceFlag) ? NS_CHROME_OPEN_AS_DIALOG : 0;

  /* and dialogs need to have the last word. assume dialogs are dialogs,
     and opened as chrome, unless explicitly told otherwise. */
  if (aDialog) {
    if (!PL_strcasestr(aFeatures, "dialog"))
      chromeFlags |= NS_CHROME_OPEN_AS_DIALOG;
    if (!PL_strcasestr(aFeatures, "chrome"))
      chromeFlags |= NS_CHROME_OPEN_AS_CHROME;
  }

  /*z-ordering, history, dependent
  chromeFlags->topmost         = WinHasOption(aFeatures, "alwaysRaised");
  chromeFlags->bottommost      = WinHasOption(aFeatures, "alwaysLowered");
  chromeFlags->z_lock          = WinHasOption(aFeatures, "z-lock");
  chromeFlags->is_modal        = WinHasOption(aFeatures, "modal");
  chromeFlags->hide_title_bar  = !(WinHasOption(aFeatures, "titlebar"));
  chromeFlags->dependent       = WinHasOption(aFeatures, "dependent");
  chromeFlags->copy_history    = FALSE;
  */

  /* Allow disabling of commands only if there is no menubar */
  /*if (!chromeFlags & NS_CHROME_MENU_BAR_ON) {
      chromeFlags->disable_commands = !WinHasOption(aFeatures, "hotkeys");
      if (XP_STRCASESTR(aFeatures,"hotkeys")==NULL)
          chromeFlags->disable_commands = FALSE;
  }*/

  /* XXX Add security check on z-ordering, modal, hide title, disable hotkeys */

  /* XXX Add security check for sizing and positioning.  
   * See mozilla/lib/libmocha/lm_win.c for current constraints */

  return chromeFlags;
}

// set the newly opened webshell's (window) size, and show it
nsresult
GlobalWindowImpl::SizeAndShowOpenedWebShell(nsIWebShell *aOuterShell,
                  char *aFeatures, PRBool aNewWindow, PRBool aDialog)
{
  if (nsnull == aOuterShell)
    return NS_ERROR_NULL_POINTER;

  nsRect           defaultBounds;
  PRInt32          top = 0, left = 0, height = 0, width = 0;
  nsIBrowserWindow *thisWindow,
                   *openedWindow = nsnull;

  // use this window's size as the default
  if (aNewWindow && NS_SUCCEEDED(GetBrowserWindowInterface(thisWindow))) {
    thisWindow->GetWindowBounds(defaultBounds);
    NS_RELEASE(thisWindow);
  }

  // get the nsIBrowserWindow corresponding to the given aOuterShell
  GetBrowserWindowInterface(openedWindow, aOuterShell);

  // set size
  if (nsnull != openedWindow) {

    nsRect   contentOffsets; // constructor sets all values to 0
    PRBool   sizeSpecified = PR_FALSE;
    PRUint32 chromeFlags = CalculateChromeFlags(aFeatures, aDialog);
    PRBool   openAsContent = (chromeFlags & NS_CHROME_OPEN_AS_CHROME) == 0;

    // if it's an extant window, we are already our default size
    if (!aNewWindow)
      if (openAsContent) {
        // defaultBounds are the content rect. also, save window size diffs
        openedWindow->GetWindowBounds(contentOffsets);
        openedWindow->GetContentBounds(defaultBounds);
        contentOffsets.x -= defaultBounds.x;
        contentOffsets.y -= defaultBounds.y;
        contentOffsets.width -= defaultBounds.width;
        contentOffsets.height -= defaultBounds.height;
      } else
        openedWindow->GetWindowBounds(defaultBounds);

    if (nsnull != aFeatures) {
      PRBool   presenceFlag = PR_FALSE; // Unused. Yuck.
      if (openAsContent) {
        width = WinHasOption(aFeatures, "innerWidth", presenceFlag) | WinHasOption(aFeatures, "width", presenceFlag);
        height = WinHasOption(aFeatures, "innerHeight", presenceFlag) | WinHasOption(aFeatures, "height", presenceFlag);
      }
      else {
        // Chrome. Look for outerWidth, outerHeight, or width/height
        width = WinHasOption(aFeatures, "outerWidth", presenceFlag) | WinHasOption(aFeatures, "width", presenceFlag);
        height = WinHasOption(aFeatures, "outerHeight", presenceFlag) | WinHasOption(aFeatures, "height", presenceFlag);
      }

      left = WinHasOption(aFeatures, "left", presenceFlag) | WinHasOption(aFeatures, "screenX", presenceFlag);
      top = WinHasOption(aFeatures, "top", presenceFlag) | WinHasOption(aFeatures, "screenY", presenceFlag);

      if (left)
        defaultBounds.x = left;
      if (top)
        defaultBounds.y = top;
      if (width) {
        sizeSpecified = PR_TRUE;
        defaultBounds.width = width;
      }
      if (height) {
        sizeSpecified = PR_TRUE;
        defaultBounds.height = height;
      }
    }

    // beard: don't resize/reposition the window if it is the same web shell.
    if (aOuterShell != mWebShell) {

      // whimper. special treatment for windows which will be intrinsically sized.
      // we can count on a Show() coming through at EndDocumentLoad time, and we
      // can count on their size being wrong at this point, and flashing.  so
      // delay some things, if this is true.
      PRBool sizeLater = PR_FALSE;
      openedWindow->IsIntrinsicallySized(sizeLater);

      if (openAsContent) {
        openedWindow->SizeWindowTo(defaultBounds.width + contentOffsets.width,
                                   defaultBounds.height + contentOffsets.height,
                                   sizeSpecified, sizeSpecified);
        // oy. sizing the content makes sense: that's what the user asked for,
        // however, it doesn't have the desired effect because all sizing
        // functions eventually end up sizing the window, and the subwindows
        // used to calculate the appropriate deltas are different. here we use
        // the HTML content area; the window uses the main webshell.  maybe
        // i could calculate a triple offset, but commenting it out is easier,
        // and probably effectively the same thing.
//      openedWindow->SizeContentTo(defaultBounds.width, defaultBounds.height);
      } else if (sizeSpecified)
        openedWindow->SizeWindowTo(defaultBounds.width, defaultBounds.height,
                                   PR_FALSE, PR_FALSE);

      openedWindow->MoveTo(defaultBounds.x + contentOffsets.x,
                      defaultBounds.y + contentOffsets.y);

      if (sizeLater)
        openedWindow->ShowAfterCreation();
      else
        openedWindow->Show();
    }

    NS_RELEASE(openedWindow);
  }
  return NS_OK;
}

// return the nsIDOMWindow corresponding to the given nsIWebShell
// note this forces the creation of a script context, if one has not already been created
// note it also sets the window's opener to this -- because it's just convenient, that's all
nsresult
GlobalWindowImpl::ReadyOpenedWebShell(nsIWebShell *aWebShell, nsIDOMWindow **aDOMWindow)
{
  nsresult              res;

  *aDOMWindow = nsnull;
  nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(aWebShell));
  NS_ENSURE_TRUE(globalObject, NS_ERROR_FAILURE);
  res = CallQueryInterface(globalObject.get(), aDOMWindow);
  globalObject->SetOpenerWindow(this); // damnit
  return res;
}

// simple utility conversion routine
nsresult
GlobalWindowImpl::WebShellToDOMWindow(nsIWebShell *aWebShell, nsIDOMWindow **aDOMWindow)
{
   NS_ENSURE_ARG_POINTER(aDOMWindow);
   NS_ENSURE_TRUE(aWebShell, NS_ERROR_FAILURE);

   nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(aWebShell));
   return CallQueryInterface(globalObject.get(), aDOMWindow);
}

NS_IMETHODIMP
GlobalWindowImpl::CreatePopup(nsIDOMElement* aElement, nsIDOMElement* aPopupContent, 
                              PRInt32 aXPos, PRInt32 aYPos, 
                              const nsString& aPopupType, 
                              const nsString& anAnchorAlignment, const nsString& aPopupAlignment,
                              nsIDOMWindow** outPopup)
{
  if (nsnull != mWebShell) {
    // Pass this off to the parent.
    nsCOMPtr<nsIWebShellContainer> webShellContainer = do_QueryInterface(mWebShell);
    if (webShellContainer) {
      webShellContainer->CreatePopup(aElement, aPopupContent, aXPos, aYPos, aPopupType,
                                     anAnchorAlignment, aPopupAlignment, this, outPopup);
    }
  }
  return NS_OK;
}

nsresult 
GlobalWindowImpl::CheckWindowName(JSContext *cx, nsString& aName)
{
  PRInt32 strIndex;
  PRUnichar mChar;

  for (strIndex = 0; strIndex < aName.Length(); strIndex++) {
    mChar = aName.CharAt(strIndex);
    if (!nsString::IsAlpha(mChar) && !nsString::IsDigit(mChar) && mChar != '_') {
      char* cp = aName.ToNewCString();
      JS_ReportError(cx,
        "illegal character '%c' ('\\%o') in window name %s",
        mChar, mChar, cp);
      nsCRT::free(cp);
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

PRInt32 
GlobalWindowImpl::WinHasOption(char *options, char *name, PRBool& aPresenceFlag)
{
  char *comma, *equal;
  PRInt32 found = 0;

  for (;;) {
    comma = strchr(options, ',');
    if (comma) *comma = '\0';
    equal = strchr(options, '=');
    if (equal) *equal = '\0';
    if (nsCRT::strcasecmp(options, name) == 0) {
      aPresenceFlag = PR_TRUE;
      if (!equal || nsCRT::strcasecmp(equal + 1, "yes") == 0)
        found = 1;
      else
        found = atoi(equal + 1);
    }
    if (equal) *equal = '=';
    if (comma) *comma = ',';
    if (found || !comma)
      break;
    options = comma + 1;
  }
  return found;
}

nsresult 
GlobalWindowImpl::GetBrowserWindowInterface(
                    nsIBrowserWindow*& aBrowser,
                    nsIWebShell *aWebShell)
{
  aBrowser = nsnull;

  if (nsnull == aWebShell)
    aWebShell = mWebShell;
  NS_ENSURE_TRUE(aWebShell, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIWebShellContainer> topLevelWindow;
  NS_ENSURE_SUCCESS(aWebShell->GetTopLevelWindow(
      getter_AddRefs(topLevelWindow)), NS_ERROR_FAILURE);

  if(!topLevelWindow)
    return NS_ERROR_FAILURE;
  return topLevelWindow->QueryInterface(NS_GET_IID(nsIBrowserWindow), (void**)&aBrowser);
}

PRBool
GlobalWindowImpl::CheckForEventListener(JSContext *aContext, nsString& aPropName)
{
  nsIEventListenerManager *mManager = nsnull;
  nsCOMPtr<nsIAtom> atom = getter_AddRefs(NS_NewAtom(aPropName));
  
  // XXX Comparisons should really be atom based

  if (aPropName == "onmousedown" || aPropName == "onmouseup" || aPropName ==  "onclick" ||
     aPropName == "onmouseover" || aPropName == "onmouseout") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMMouseListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onkeydown" || aPropName == "onkeyup" || aPropName == "onkeypress") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMKeyListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onmousemove") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMMouseMotionListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onfocus" || aPropName == "onblur") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMFocusListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onsubmit" || aPropName == "onreset" || aPropName == "onchange" ||
           aPropName == "onselect") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMFormListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onload" || aPropName == "onunload" || aPropName == "onclose" ||
           aPropName == "onabort" || aPropName == "onerror") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this, atom, kIDOMLoadListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  else if (aPropName == "onpaint") {
    if (NS_OK == GetListenerManager(&mManager)) {
      nsCOMPtr<nsIScriptContext> scriptCX;
      nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
      if (!scriptCX ||
          NS_OK != mManager->RegisterScriptEventListener(scriptCX, this,
                                                         atom, kIDOMPaintListenerIID)) {
        NS_RELEASE(mManager);
        return PR_FALSE;
      }
    }
  }
  NS_IF_RELEASE(mManager);

  return PR_TRUE;
}

PRBool
GlobalWindowImpl::AddProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  if (JS_TypeOfValue(aContext, *aVp) == JSTYPE_FUNCTION && JSVAL_IS_STRING(aID)) {
    nsString mPropName;
    nsAutoString mPrefix;
    mPropName.SetString(JS_GetStringChars(JS_ValueToString(aContext, aID)));
    mPrefix.SetString(mPropName.GetUnicode(), 2);
    if (mPrefix == "on") {
      return CheckForEventListener(aContext, mPropName);
    }
  }
  return PR_TRUE;
}

PRBool    
GlobalWindowImpl::DeleteProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  return PR_TRUE;
}

PRBool    
GlobalWindowImpl::GetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  if (JSVAL_IS_STRING(aID)) {
    char* cString = JS_GetStringBytes(JS_ValueToString(aContext, aID));
    if (PL_strcmp("location", cString) == 0) {
      nsIDOMLocation *location;
    
      if (NS_OK == GetLocation(&location)) {
        if (location != nsnull) {
          nsIScriptObjectOwner *owner = nsnull;
          if (NS_OK == location->QueryInterface(NS_GET_IID(nsIScriptObjectOwner), (void**)&owner)) {
            JSObject *object = nsnull;
            nsCOMPtr<nsIScriptContext> scriptCX;
            nsJSUtils::nsGetDynamicScriptContext(aContext, getter_AddRefs(scriptCX));
            if (scriptCX &&
                NS_OK == owner->GetScriptObject(scriptCX, (void**)&object)) {
              // set the return value
              *aVp = OBJECT_TO_JSVAL(object);
            }
            NS_RELEASE(owner);
          }
          NS_RELEASE(location);
        }
        else {
          *aVp = JSVAL_NULL;
        }
      }
      else {
        return PR_FALSE;
      }
    }
    else if (PL_strcmp("title", cString) == 0) {
      if (mWebShell) {
        // See if we're a chrome shell.
        PRInt32 type;
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
        docShellAsItem->GetItemType(&type);
        if (type == nsIDocShellTreeItem::typeChrome) {
          nsCOMPtr<nsIBrowserWindow> browser;
          if (NS_OK == GetBrowserWindowInterface(*getter_AddRefs(browser)) && browser) {
            // We got a browser window interface
            nsXPIDLString title;
            browser->GetTitle(getter_Copies(title));

            JSString* jsString = JS_NewUCStringCopyZ(aContext, (const jschar*)title);
            if (!jsString)
              return PR_FALSE;
              
            *aVp = STRING_TO_JSVAL(jsString);
          }
        }
      }
    }
  }
  return PR_TRUE;
}

PRBool
GlobalWindowImpl::SetProperty(JSContext *aContext, JSObject *aObj, jsval aID, jsval *aVp)
{
  PRBool result = PR_TRUE;
  if (JS_TypeOfValue(aContext, *aVp) == JSTYPE_FUNCTION && JSVAL_IS_STRING(aID)) {
    nsAutoString propName;
    nsAutoString prefix;
    propName.SetString(JS_GetStringChars(JS_ValueToString(aContext, aID)));
    if (propName.Length() > 2) {
      prefix.SetString(propName.GetUnicode(), 2);
      if (prefix.EqualsIgnoreCase("on")) {
        result = CheckForEventListener(aContext, propName);
      }
    }
  }
  else if (JSVAL_IS_STRING(aID)) {
    char* cString = JS_GetStringBytes(JS_ValueToString(aContext, aID));
    
    if (PL_strcmp("location", cString) == 0) {
      nsCOMPtr<nsIDOMLocation> location;
      
      if (NS_OK == GetLocation(getter_AddRefs(location))) {
        nsCOMPtr<nsIJSScriptObject> scriptObj = do_QueryInterface(location);
        JSString* str = JS_NewStringCopyZ(aContext, "href");
        
        if (scriptObj && str) {
          result = scriptObj->SetProperty(aContext, aObj, STRING_TO_JSVAL(str), aVp);
        }
      }
      else {
        result = PR_FALSE;
      }
    }
    else if (PL_strcmp("title", cString) == 0) {
      if (mWebShell) {
        // See if we're a chrome shell.
        PRInt32 type;
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mWebShell));
        docShellAsItem->GetItemType(&type);
        if (type == nsIDocShellTreeItem::typeChrome) {
          nsCOMPtr<nsIBrowserWindow> browser;
          if (NS_OK == GetBrowserWindowInterface(*getter_AddRefs(browser)) && browser) {
            // We got a browser window interface
            JSString *jsString = JS_ValueToString(aContext, *aVp);
            if (!jsString) {
              result = PR_FALSE;
            }
            else {
              const PRUnichar* uniTitle = JS_GetStringChars(jsString);
              browser->SetTitle(uniTitle);
            }
          }
        }
      }
    }
  }

  return result;
}

PRBool    
GlobalWindowImpl::EnumerateProperty(JSContext *aContext, JSObject *aObj)
{
  return PR_TRUE;
}

PRBool    
GlobalWindowImpl::Resolve(JSContext *aContext, JSObject *aObj, jsval aID)
{
  if (JSVAL_IS_STRING(aID)) {
    if (PL_strcmp("location", JS_GetStringBytes(JS_ValueToString(aContext, aID))) == 0) {
      ::JS_DefineProperty(aContext, (JSObject *)mScriptObject, "location",
                          JSVAL_NULL, nsnull, nsnull, 0);
    }
    else if (nsnull != mWebShell) {
      nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(mWebShell));
      PRInt32 count;
      if (NS_SUCCEEDED(docShellAsNode->GetChildCount(&count)) && count) {
        nsCOMPtr<nsIDocShellTreeItem> child;
        nsAutoString name(JS_GetStringBytes(JS_ValueToString(aContext, aID)));
        if (NS_SUCCEEDED(docShellAsNode->FindChildWithName(name.GetUnicode(),
          PR_FALSE, nsnull, getter_AddRefs(child)))) {
          if (child) {
            JSObject *childObj;
            //We found a subframe of the right name.  The rest of this is to get its script object.
            nsCOMPtr<nsIScriptGlobalObject> childGlobalObject(do_GetInterface(child));
            if(childGlobalObject) {
               nsCOMPtr<nsIScriptObjectOwner> objOwner(do_QueryInterface(childGlobalObject));
               if(objOwner) {
                  nsCOMPtr<nsIScriptContext> scriptContext;
               
                  childGlobalObject->GetContext(getter_AddRefs(scriptContext));
                  if(scriptContext) {
                     objOwner->GetScriptObject(scriptContext, (void**)&childObj);
                  }
               } 
            }
            //Okay, if we now have a childObj, we can define it and proceed.
            if (childObj) {
              ::JS_DefineProperty(aContext, (JSObject *)mScriptObject, 
                                  JS_GetStringBytes(JS_ValueToString(aContext, aID)),
                                  OBJECT_TO_JSVAL(childObj), nsnull, nsnull, 0);
            }
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool    
GlobalWindowImpl::Convert(JSContext *aContext, JSObject *aObj, jsval aID)
{
  return PR_TRUE;
}

void      
GlobalWindowImpl::Finalize(JSContext *aContext, JSObject *aObj)
{
}

nsresult 
GlobalWindowImpl::GetListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  if (nsnull != mListenerManager) {
    return mListenerManager->QueryInterface(kIEventListenerManagerIID, (void**) aInstancePtrResult);;
  }
  //This is gonna get ugly.  Can't use NS_NewEventListenerManager because of a circular link problem.
  nsIDOMEventReceiver *doc = nsnull;
  if (nsnull != mDocument && NS_OK == mDocument->QueryInterface(kIDOMEventReceiverIID, (void**)&doc)) {
    if (NS_OK == doc->GetNewListenerManager(aInstancePtrResult)) {
      mListenerManager = *aInstancePtrResult;
      NS_ADDREF(mListenerManager);
      NS_RELEASE(doc);
      return NS_OK;
    }
    NS_IF_RELEASE(doc);
  }
  return NS_ERROR_FAILURE;
}

//XXX I need another way around the circular link problem.
nsresult 
GlobalWindowImpl::GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  return NS_ERROR_FAILURE;
}

nsresult 
GlobalWindowImpl::HandleDOMEvent(nsIPresContext* aPresContext, 
                                 nsEvent* aEvent, 
                                 nsIDOMEvent** aDOMEvent,
                                 PRUint32 aFlags,
                                 nsEventStatus* aEventStatus)
{
  nsresult ret = NS_OK;
  nsIDOMEvent* domEvent = nsnull;

  /* mChromeEventHandler and mContext go dangling in the middle of this
     function under some circumstances (events that destroy the window)
     without this addref. */
  nsCOMPtr<nsIChromeEventHandler> kungFuDeathGrip1(mChromeEventHandler);
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip2(mContext);

  if (NS_EVENT_FLAG_INIT == aFlags) {
    aDOMEvent = &domEvent;
    aEvent->flags = NS_EVENT_FLAG_NONE;
  }
  
  //Capturing stage
  if (NS_EVENT_FLAG_BUBBLE != aFlags && mChromeEventHandler) {
    // Check chrome document capture here.
    mChromeEventHandler->HandleChromeEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
  }

  //Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
    aEvent->flags = aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
  }

  //Bubbling stage
  if (NS_EVENT_FLAG_CAPTURE != aFlags && mChromeEventHandler) {
    // Bubble to a chrome document if it exists
    // XXX Need a way to know if an event should really bubble or not.
    // For now filter out load and unload, since they cause problems.
    if (aEvent->message != NS_PAGE_LOAD && aEvent->message != NS_PAGE_UNLOAD && aEvent->message != NS_FOCUS_CONTENT
                && aEvent->message != NS_BLUR_CONTENT) {
      mChromeEventHandler->HandleChromeEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_BUBBLE, aEventStatus);
    }
  }

  if (NS_EVENT_FLAG_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event, release here.
    if (nsnull != *aDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (0 != rc) {
      //Okay, so someone in the DOM loop (a listener, JS object) still has a ref to the DOM Event but
      //the internal data hasn't been malloc'd.  Force a copy of the data here so the DOM Event is still valid.
        nsIPrivateDOMEvent *mPrivateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(kIPrivateDOMEventIID, (void**)&mPrivateEvent)) {
          mPrivateEvent->DuplicatePrivateData();
          NS_RELEASE(mPrivateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }

  return ret;
}

nsresult 
GlobalWindowImpl::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  nsIEventListenerManager *mManager;

  if (NS_OK == GetListenerManager(&mManager)) {
    mManager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    NS_RELEASE(mManager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult 
GlobalWindowImpl::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  if (nsnull != mListenerManager) {
    mListenerManager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
GlobalWindowImpl::AddEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                   PRBool aUseCapture)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
GlobalWindowImpl::RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                      PRBool aUseCapture)
{
  if (nsnull != mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult 
GlobalWindowImpl::CaptureEvents(PRInt32 aEventFlags)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    //mManager->CaptureEvent(aListener);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult 
GlobalWindowImpl::ReleaseEvents(PRInt32 aEventFlags)
{
  if (nsnull != mListenerManager) {
    //mListenerManager->ReleaseEvent(aListener);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult 
GlobalWindowImpl::RouteEvent(nsIDOMEvent* aEvt)
{
  //XXX Not the best solution -joki
  return NS_OK;
}

nsresult
GlobalWindowImpl::EnableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

nsresult
GlobalWindowImpl::DisableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GlobalWindowImpl::GetPrincipal(nsIPrincipal **result) 
{
  nsCOMPtr<nsIDocument> doc;
  if (!mDocument || NS_FAILED(mDocument->QueryInterface(kIDocumentIID, 
                                                        (void **) getter_AddRefs(doc))))
  {
    return NS_ERROR_FAILURE;
  }
  *result = doc->GetDocumentPrincipal();
  if (!*result)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

extern "C" NS_DOM nsresult
NS_NewScriptGlobalObject(nsIScriptGlobalObject **aResult)
{
  if (nsnull == aResult) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  GlobalWindowImpl *global = new GlobalWindowImpl();
  if (nsnull == global) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return global->QueryInterface(NS_GET_IID(nsIScriptGlobalObject), (void **)aResult);
}



//
//  Navigator class implementation 
//
NavigatorImpl::NavigatorImpl()
{
  NS_INIT_REFCNT();
  mScriptObject = nsnull;
  mMimeTypes = nsnull;
  mPlugins = nsnull;
}

NavigatorImpl::~NavigatorImpl()
{
  NS_IF_RELEASE(mMimeTypes);
  NS_IF_RELEASE(mPlugins);
}

NS_IMPL_ADDREF(NavigatorImpl)
NS_IMPL_RELEASE(NavigatorImpl)

nsresult 
NavigatorImpl::QueryInterface(const nsIID& aIID,
                              void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(NS_GET_IID(nsIScriptObjectOwner))) {
    *aInstancePtrResult = (void*) ((nsIScriptObjectOwner*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNavigatorIID)) {
    *aInstancePtrResult = (void*) ((nsIDOMNavigator*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtrResult = (void*)(nsISupports*)(nsIScriptObjectOwner*)this;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult 
NavigatorImpl::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}

nsresult 
NavigatorImpl::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  NS_PRECONDITION(nsnull != aScriptObject, "null arg");
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    nsIScriptGlobalObject *global = aContext->GetGlobalObject();
    res = NS_NewScriptNavigator(aContext, (nsISupports *)(nsIDOMNavigator *)this, global, &mScriptObject);
    NS_IF_RELEASE(global);
  }
  
  *aScriptObject = mScriptObject;
  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetUserAgent(nsString& aUserAgent)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *ua = nsnull;
    res = service->GetUserAgent(&ua);
    aUserAgent = ua;
    Recycle(ua);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetAppCodeName(nsString& aAppCodeName)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *appName = nsnull;
    res = service->GetAppCodeName(&appName);
    aAppCodeName = appName;
    Recycle(appName);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetAppVersion(nsString& aAppVersion)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *appVer = nsnull;
    res = service->GetAppVersion(&appVer);
    aAppVersion = appVer;
    Recycle(appVer);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetAppName(nsString& aAppName)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *appName = nsnull;
    res = service->GetAppName(&appName);
    aAppName = appName;
    Recycle(appName);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetLanguage(nsString& aLanguage)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *lang = nsnull;
    res = service->GetLanguage(&lang);
    aLanguage = lang;
    Recycle(lang);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetPlatform(nsString& aPlatform)
{
  nsresult res;
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &res);
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *plat = nsnull;
    res = service->GetPlatform(&plat);
    aPlatform = plat;
    Recycle(plat);
  }

  return res;
}

NS_IMETHODIMP
NavigatorImpl::GetSecurityPolicy(nsString& aSecurityPolicy)
{
  return NS_OK;
}

NS_IMETHODIMP
NavigatorImpl::GetMimeTypes(nsIDOMMimeTypeArray** aMimeTypes)
{
  if (nsnull == mMimeTypes) {
    mMimeTypes = new MimeTypeArrayImpl(this);
    NS_IF_ADDREF(mMimeTypes);
  }

  *aMimeTypes = mMimeTypes;
  NS_IF_ADDREF(mMimeTypes);

  return NS_OK;
}

NS_IMETHODIMP
NavigatorImpl::GetPlugins(nsIDOMPluginArray** aPlugins)
{
  if (nsnull == mPlugins) {
    mPlugins = new PluginArrayImpl(this);
    NS_IF_ADDREF(mPlugins);
  }

  *aPlugins = mPlugins;
  NS_IF_ADDREF(mPlugins);

  return NS_OK;
}

NS_IMETHODIMP    
NavigatorImpl::GetCookieEnabled(PRBool* aCookieEnabled)
{
  nsresult rv = NS_OK;
  *aCookieEnabled = PR_FALSE;

  NS_WITH_SERVICE(nsICookieService, service, kCookieServiceCID, &rv);
  if (NS_FAILED(rv) || service == nsnull) {
    return rv;
  }

  return service->CookieEnabled(aCookieEnabled);
}


NS_IMETHODIMP
NavigatorImpl::JavaEnabled(PRBool* aReturn)
{
  nsresult rv = NS_OK;
  *aReturn = PR_FALSE;

  // determine whether user has enabled java.
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (NS_FAILED(rv) || prefs == nsnull) {
    return rv;
  }
  
  // if pref doesn't exist, map result to false.
  if (prefs->GetBoolPref("security.enable_java", aReturn) != NS_OK)
    *aReturn = PR_FALSE;

  return rv;
}

NS_IMETHODIMP
NavigatorImpl::TaintEnabled(PRBool* aReturn)
{
  *aReturn = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP    
NavigatorImpl::Preference(JSContext* cx, 
                          jsval* argv, 
                          PRUint32 argc, 
                          jsval* aReturn)
{
  nsresult result = NS_OK;

  *aReturn = JSVAL_NULL;

  JSObject* self = (JSObject*)mScriptObject;
  if (!self) {
    return NS_ERROR_FAILURE;
  }

  PRBool ok;
  NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                  NS_SCRIPTSECURITYMANAGER_PROGID, &result);
  if (NS_FAILED(result)) {
    return result;
  }

  secMan->CheckScriptAccess(cx, self, NS_DOM_PROP_NAVIGATOR_PREFERENCE, 
                            (argc == 1 ? PR_FALSE : PR_TRUE), &ok);
  if (!ok) {
    //Need to throw error here
    return NS_ERROR_FAILURE;
  }

  NS_WITH_SERVICE(nsIPref, pref, kPrefServiceCID, &result);
  if (NS_FAILED(result)) {
    return result;
  }

  if (argc > 0) {
    JSString* str = JS_ValueToString(cx, argv[0]);
    if (str) {
      char* prefStr = JS_GetStringBytes(str);
      if (argc == 1) {
        PRInt32 prefType;
        pref->GetPrefType(prefStr, &prefType);
        switch (prefType & nsIPref::ePrefValuetypeMask) {
          case nsIPref::ePrefString:
          {
            char* prefCharVal;
            result = pref->CopyCharPref(prefStr, &prefCharVal);
            if (NS_SUCCEEDED(result)) {
              JSString* retStr = JS_NewStringCopyZ(cx, prefCharVal);
              if (retStr) {
                *aReturn = STRING_TO_JSVAL(retStr);
              }
              PL_strfree(prefCharVal);
            }
            break;
          }
          case nsIPref::ePrefInt:
          {
            PRInt32 prefIntVal;
            result = pref->GetIntPref(prefStr, &prefIntVal);
            if (NS_SUCCEEDED(result)) {
              *aReturn = INT_TO_JSVAL(prefIntVal);
            }
            break;
          }
          case nsIPref::ePrefBool:
          {
            PRBool prefBoolVal;
            result = pref->GetBoolPref(prefStr, &prefBoolVal);
            if (NS_SUCCEEDED(result)) {
              *aReturn = BOOLEAN_TO_JSVAL(prefBoolVal);
            }
            break;
          }
        }
      }
      else {
        if (JSVAL_IS_STRING(argv[1])) {
          JSString* valueJSStr = JS_ValueToString(cx, argv[1]);
          if (valueJSStr) {
            result = pref->SetCharPref(prefStr, JS_GetStringBytes(valueJSStr));
          }
        }
        else if (JSVAL_IS_INT(argv[1])) {
          jsint valueInt = JSVAL_TO_INT(argv[1]);
          result = pref->SetIntPref(prefStr, (PRInt32)valueInt);
        }
        else if (JSVAL_IS_BOOLEAN(argv[1])) {
          JSBool valueBool = JSVAL_TO_BOOLEAN(argv[1]);
          result = pref->SetBoolPref(prefStr, (PRBool)valueBool);
        }
        else if (JSVAL_IS_NULL(argv[1])) {
          result = pref->DeleteBranch(prefStr);
        }
      }
    }
  }

  return result;
}

// Controllers Methods
NS_IMETHODIMP
GlobalWindowImpl::GetControllers(nsIControllers** aResult)
{
  if(!mControllers){
    nsresult rv = nsComponentManager::CreateInstance(kXULControllersCID,
                                            nsnull,
                                            NS_GET_IID(nsIControllers),
                                            getter_AddRefs(mControllers));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a controllers");
    if (NS_FAILED(rv)) return rv;
    #ifdef DOM_CONTROLLER
    // Add in the default controller
    	nsDOMWindowController* domController = new nsDOMWindowController( this );
    	if ( domController )
    	{
			nsCOMPtr<nsIController> controller =  domController ;
			mControllers->AppendController( controller );
		}
	#endif // DOM_CONTROLLER
  }
  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

// nsPIDOMWindow methods

NS_IMETHODIMP
GlobalWindowImpl::GetPrivateParent(nsPIDOMWindow** aParent)
{
   nsCOMPtr<nsIDOMWindow> parent;
   *aParent = nsnull; // Set to null so we can bail out later

   GetParent(getter_AddRefs(parent));

   if(NS_STATIC_CAST(nsIDOMWindow*, this) == parent.get())
      {
      nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
      if(!chromeElement)
         return NS_OK; // This is ok, just means a null parent.

      nsCOMPtr<nsIDocument> doc;
      chromeElement->GetDocument(*getter_AddRefs(doc));
      if(!doc)
         return NS_OK; // This is ok, just means a null parent.

      nsCOMPtr<nsIScriptGlobalObject> globalObject;
      doc->GetScriptGlobalObject(getter_AddRefs(globalObject));
      if(!globalObject)
         return NS_OK; // This is ok, just means a null parent.

      parent = do_QueryInterface(globalObject);
      }

   if(parent)
      CallQueryInterface(parent.get(), aParent);

   return NS_OK;
}

#ifdef DOM_CONTROLLER
// nsDOMWindowController
const   char* sCopyString = "cmd_copy";
const   char* sCutString = "cmd_cut";
const   char* sPasteString = "cmd_paste";
const   char* sSelectAllString = "cmd_selectAll";

const   char* sBeginLineString = "cmd_beginLine";
const   char* sEndLineString   = "cmd_endLine";
const   char* sSelectBeginLineString = "cmd_selectBeginLine";
const   char* sSelectEndLineString   = "cmd_selectEndLine";

const   char* sScrollTopString = "cmd_scrollTop";
const   char* sScrollBottomString = "cmd_scrollBottom";

const   char* sMoveTopString   = "cmd_moveTop";
const   char* sMoveBottomString= "cmd_moveBottom";
const   char* sSelectMoveTopString   = "cmd_selectTop";
const   char* sSelectMoveBottomString= "cmd_selectBottom";

const   char* sDownString      = "cmd_linedown";
const   char* sUpString        = "cmd_lineup";
const   char* sSelectDownString = "cmd_selectLineDown";
const   char* sSelectUpString   = "cmd_selectLineUp";

const   char* sLeftString      = "cmd_charPrevious";
const   char* sRightString     = "cmd_charNext";
const   char* sSelectLeftString = "cmd_selectCharPrevious";
const   char* sSelectRightString= "cmd_selectCharNext";


const   char* sWordLeftString      = "cmd_wordPrevious";
const   char* sWordRightString     = "cmd_wordNext";
const   char* sSelectWordLeftString = "cmd_selectWordPrevious";
const   char* sSelectWordRightString= "cmd_selectWordNext";

const   char* sScrollPageUp    = "cmd_scrollPageUp";
const   char* sScrollPageDown  = "cmd_scrollPageDown";

const   char* sScrollLineUp    = "cmd_scrollLineUp";
const   char* sScrollLineDown  = "cmd_scrollLineDown";

const   char* sMovePageUp    = "cmd_scrollPageUp";
const   char* sMovePageDown  = "cmd_scrollPageDown";
const   char* sSelectMovePageUp     = "cmd_selectPageUp";
const   char* sSelectMovePageDown   = "cmd_selectPageDown";

NS_IMPL_ADDREF( nsDOMWindowController )
NS_IMPL_RELEASE( nsDOMWindowController )


NS_INTERFACE_MAP_BEGIN(nsDOMWindowController)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIController)
   NS_INTERFACE_MAP_ENTRY(nsIController)
NS_INTERFACE_MAP_END


//NS_IMPL_QUERY_INTERFACE1(nsDOMWindowController, nsIController)

nsDOMWindowController::nsDOMWindowController( nsIDOMWindow* aWindow)
{
	NS_INIT_REFCNT();
	mWindow =  aWindow;
}

nsresult nsDOMWindowController::GetEditInterface( nsIContentViewerEdit** aEditInterface)
{	 
	nsresult result = NS_ERROR_FAILURE;
	nsCOMPtr<nsPIDOMWindow> PIWindow = do_QueryInterface(mWindow );
	if ( PIWindow.get() == NULL ) 
		return result;
	nsCOMPtr<nsIWebShell> webshell;
	PIWindow->GetWebShell( getter_AddRefs( webshell ) );
	if (nsnull != webshell)
  	{
    	nsCOMPtr<nsIContentViewer> viewer;    
    	webshell->GetContentViewer(getter_AddRefs(viewer));    
    	if (nsnull != viewer)
    	{
    		nsCOMPtr<nsIContentViewerEdit> edit = do_QueryInterface( viewer );
    		if ( edit.get() != NULL )
			{
				*aEditInterface = edit;
        NS_ADDREF((*aEditInterface));
				return NS_OK;
			}
    	}
    }
   
  return result;
}

nsresult
nsDOMWindowController::GetPresShell(nsIPresShell** aPresShell)
{
	nsresult result = NS_ERROR_FAILURE;
	nsCOMPtr<nsPIDOMWindow> PIWindow = do_QueryInterface(mWindow );
	if ( PIWindow.get() == NULL ) 
		return NS_ERROR_FAILURE;
	nsCOMPtr<nsIWebShell> webshell;
	result = PIWindow->GetWebShell( getter_AddRefs( webshell ) );
  if (NS_SUCCEEDED(result) && webshell)
  {
    nsCOMPtr<nsIPresShell> presShell = getter_AddRefs(GetPresShellFor(webshell));
    if (presShell)
    {
      *aPresShell = presShell;
      NS_ADDREF((*aPresShell));
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsDOMWindowController::GetSelectionController(nsISelectionController **aSelCon)
{
  nsCOMPtr<nsIPresShell> presShell;
  nsresult result = GetPresShell(getter_AddRefs(presShell));
  if (presShell && NS_SUCCEEDED(result))
  {
    nsCOMPtr<nsISelectionController> selController = do_QueryInterface(presShell); 
    if (selController)
    {
      *aSelCon = selController;
      NS_ADDREF(*aSelCon);
      return NS_OK;
    }
  }
  return result ? result : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsDOMWindowController::IsCommandEnabled(const PRUnichar *aCommand, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr< nsIContentViewerEdit> editInterface;
  rv = GetEditInterface( getter_AddRefs( editInterface) );
  if ( NS_FAILED (rv ) )
  	return rv;
  	
  if (PR_TRUE== nsAutoString(sCopyString).Equals(aCommand))
  { 
    //1rv = editInterface->GetCopyable( aResult );
    rv = NS_OK;
    *aResult = PR_TRUE;//hack for now
  }
#if 0
  else if (PR_TRUE==nsAutoString(sCutString).Equals(aCommand))    
  { 
    rv =  editInterface->GetCutable( aResult);
  }
  else if (PR_TRUE==nsAutoString(sPasteString).Equals(aCommand))    
  { 
    rv = editInterface->GetPasteable( aResult );
  }
  else if (PR_TRUE==nsAutoString(sSelectAllString).Equals(aCommand))    
  { 
    *aResult = PR_TRUE;
    rv = NS_OK;
  }
#endif 
  return rv;

}

NS_IMETHODIMP nsDOMWindowController::SupportsCommand(const PRUnichar *aCommand, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = PR_FALSE;
  if (
  	(PR_TRUE== nsAutoString(sCopyString).Equals(aCommand)) ||
  	  (PR_TRUE== nsAutoString(sSelectAllString).Equals(aCommand)) ||
  	  (PR_TRUE== nsAutoString(sCutString).Equals(aCommand)) ||
  	  (PR_TRUE== nsAutoString(sPasteString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sBeginLineString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sEndLineString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectBeginLineString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectEndLineString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollTopString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollBottomString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sMoveTopString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sMoveBottomString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectMoveTopString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectMoveBottomString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sDownString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sUpString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sLeftString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sRightString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectDownString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectUpString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectLeftString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectRightString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sWordLeftString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sWordRightString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectWordLeftString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectWordRightString).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollPageUp).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollPageDown).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollLineUp).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sScrollLineDown).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sMovePageUp).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sMovePageDown).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectMovePageUp).Equals(aCommand)) ||
      (PR_TRUE== nsAutoString(sSelectMovePageDown).Equals(aCommand))
      )
  {
    *aResult = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDOMWindowController::DoCommand(const PRUnichar *aCommand)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr< nsIContentViewerEdit> editInterface;
  nsCOMPtr<nsISelectionController> selCont;
  rv = GetEditInterface( getter_AddRefs( editInterface ) );
  nsCOMPtr<nsIPresShell> presShell;
  if ( NS_FAILED ( rv ) )
  	return rv;
  	
  if (PR_TRUE== nsAutoString(sCopyString).Equals(aCommand))
  { 
    //rv = editInterface->CopySelection();
    if (NS_SUCCEEDED(GetPresShell(getter_AddRefs(presShell))) && presShell)
    {
      rv = presShell->DoCopy();
    }
  }
  else if (PR_TRUE== nsAutoString(sSelectAllString).Equals(aCommand))    
  { 
    rv = editInterface->CutSelection();
  }
  else if (PR_TRUE== nsAutoString( sSelectAllString ).Equals(aCommand))    
  { 
    rv = editInterface->Paste();
  }
  else if (PR_TRUE== nsAutoString( sSelectAllString ).Equals(aCommand))    
  { 
    rv = editInterface->SelectAll();
  }
  else if (PR_TRUE==nsAutoString(sScrollPageUp).Equals(aCommand))  //ScrollPageUp
  { 
    NS_ENSURE_SUCCESS(GetSelectionController(getter_AddRefs(selCont)),NS_ERROR_FAILURE);
    return selCont->ScrollPage(PR_FALSE);
  }
  else if (PR_TRUE==nsAutoString(sScrollPageDown).Equals(aCommand))  //ScrollPageDown
  { 
    NS_ENSURE_SUCCESS(GetSelectionController(getter_AddRefs(selCont)),NS_ERROR_FAILURE);
    return selCont->ScrollPage(PR_TRUE);
  }
  else if (PR_TRUE==nsAutoString(sScrollLineUp).Equals(aCommand))  //ScrollLineUp
  { 
    NS_ENSURE_SUCCESS(GetSelectionController(getter_AddRefs(selCont)),NS_ERROR_FAILURE);
    return selCont->ScrollLine(PR_FALSE);
  }
  else if (PR_TRUE==nsAutoString(sScrollLineDown).Equals(aCommand))  //ScrollLineDown
  { 
    NS_ENSURE_SUCCESS(GetSelectionController(getter_AddRefs(selCont)),NS_ERROR_FAILURE);
    return selCont->ScrollLine(PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP nsDOMWindowController::OnEvent(const PRUnichar *aEventName)
{
  return NS_OK;
}
#endif

