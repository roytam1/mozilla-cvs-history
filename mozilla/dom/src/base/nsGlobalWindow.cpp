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
 *   Travis Bogard <travis@netscape.com>
 *   Brendan Eich <brendan@mozilla.org>
 *   David Hyatt (hyatt@netscape.com)
 *   Johnny Stenback <jst@netscape.com>
 */

// Local Includes
#include "nsGlobalWindow.h"
#include "nsScreen.h"
#include "nsHistory.h"
#include "nsBarProps.h"

// Helper Classes
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsJSUtils.h"
#include "prmem.h"
#include "jsdbgapi.h"           // for JS_ClearWatchPointsForObject
#include "nsReadableUtils.h"
#include "nsDOMClassInfo.h"

// Other Classes
#include "nsIEventListenerManager.h"
#include "nsEscape.h"
#include "nsDOMPropEnums.h"
#include "nsStyleCoord.h"
#include "nsHTMLReflowState.h"
#include "nsMimeTypeArray.h"
#include "nsNetUtil.h"
#include "nsPluginArray.h"
#include "nsContentCID.h"

// Interfaces Needed
#include "nsIBaseWindow.h"
#include "nsICharsetConverterManager.h"
#include "nsICodebasePrincipal.h"
#include "nsIContent.h"
#include "nsIContentViewerFile.h"
#include "nsIContentViewerEdit.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocument.h"
#include "nsIDOMCrypto.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMPkcs11.h"
#include "nsIEventQueueService.h"
#include "nsIEventStateManager.h"
#include "nsIHTTPProtocolHandler.h"
#include "nsIInterfaceRequestor.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIPref.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIPrompt.h"
#include "nsIServiceManager.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptExternalNameSet.h"
#include "nsIScrollableView.h"
#include "nsISelectionController.h"
#include "nsISelection.h"
#include "nsIFrameSelection.h"
#include "nsISidebar.h"         // XXX for sidebar HACK, see bug 20721
#include "nsIPrompt.h"
#include "nsIStyleContext.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebShell.h"
#include "nsIComputedDOMStyle.h"
#include "nsIEntropyCollector.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsIWindowWatcher.h"
#include "nsPIWindowWatcher.h"
#include "nsISupportsPrimitives.h"
#include "nsDOMClassInfo.h"
#include "nsIJSNativeInitializer.h"

#include "nsWindowRoot.h"

// XXX An unfortunate dependency exists here (two XUL files).
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULCommandDispatcher.h"

#include "nsIBindingManager.h"
#include "nsIXBLService.h"


static nsIEntropyCollector* gEntropyCollector = nsnull;
static PRInt32              gRefCnt           = 0;
nsIXPConnect *GlobalWindowImpl::sXPConnect    = nsnull;

// CIDs
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kHTTPHandlerCID, NS_IHTTPHANDLER_CID);
static NS_DEFINE_CID(kXULControllersCID, NS_XULCONTROLLERS_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static char *sWindowWatcherContractID = "@mozilla.org/embedcomp/window-watcher;1";

static const char *kCryptoContractID = NS_CRYPTO_CONTRACTID;
static const char *kPkcs11ContractID = NS_PKCS11_CONTRACTID;

//*****************************************************************************
//***    GlobalWindowImpl: Object Management
//*****************************************************************************

GlobalWindowImpl::GlobalWindowImpl() :
  mScriptObject(nsnull),
  mNavigator(nsnull), mScreen(nsnull), mHistory(nsnull), mFrames(nsnull),
  mLocation(nsnull), mMenubar(nsnull), mToolbar(nsnull), mLocationbar(nsnull),
  mPersonalbar(nsnull), mStatusbar(nsnull), mScrollbars(nsnull),
  mTimeouts(nsnull), mTimeoutInsertionPoint(&mTimeouts), mRunningTimeout(nsnull),
  mTimeoutPublicIdCounter(1), mTimeoutFiringDepth(0),
  mFirstDocumentLoad(PR_TRUE), mGlobalObjectOwner(nsnull), mDocShell(nsnull),
  mMutationBits(0), mChromeEventHandler(nsnull)
{
  NS_INIT_REFCNT();
  if (gRefCnt++ == 0) {
    nsCOMPtr<nsIEntropyCollector> enCol(do_GetService(NS_ENTROPYCOLLECTOR_CONTRACTID));
    if (enCol) {
      gEntropyCollector = enCol;
      NS_ADDREF(gEntropyCollector);
    }
  }

  if (!sXPConnect) {
    nsServiceManager::GetService(nsIXPConnect::GetCID(),
                                 nsIXPConnect::GetIID(),
                                 (nsISupports **)&sXPConnect);
  }
}

GlobalWindowImpl::~GlobalWindowImpl()
{
  if (!--gRefCnt) {
    NS_IF_RELEASE(gEntropyCollector);

    NS_IF_RELEASE(sXPConnect);
  }

  CleanUp();
}

// static
void
GlobalWindowImpl::ShutDown()
{
  NS_IF_RELEASE(sXPConnect);
}

void GlobalWindowImpl::CleanUp()
{
  if (mContext)
    mContext->RemoveReference(&mScriptObject, mScriptObject);
  mContext = nsnull;            // Forces Release
  mDocument = nsnull;           // Forces Release
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
  mOpener = nsnull;             // Forces Release
  mControllers = nsnull;        // Forces Release
}

//*****************************************************************************
// GlobalWindowImpl::nsISupports
//*****************************************************************************


// XPConnect interface list for GlobalWindowImpl
NS_CLASSINFO_MAP_BEGIN(Window)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMWindow)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMEventReceiver)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMViewCSS)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMAbstractView)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for GlobalWindowImpl
NS_INTERFACE_MAP_BEGIN(GlobalWindowImpl)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindowInternal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMJSWindow)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventReceiver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMWindow)
  NS_INTERFACE_MAP_ENTRY(nsIDOMViewCSS)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAbstractView)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_DOM_CLASSINFO(Window)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(GlobalWindowImpl)
NS_IMPL_RELEASE(GlobalWindowImpl)


#if 0
//*****************************************************************************
// GlobalWindowImpl::nsIScriptObjectOwner
//*****************************************************************************
NS_IMETHODIMP GlobalWindowImpl::GetScriptObject(nsIScriptContext* aContext,
                                                void **aScriptObject)
{
  NS_ENSURE_ARG_POINTER(aScriptObject);
  nsresult res = NS_OK;
  if (!mScriptObject) {
    res = NS_NewScriptWindow(aContext, NS_STATIC_CAST(nsIDOMWindow *, this),
                             nsnull, &mScriptObject);
    aContext->AddNamedReference(&mScriptObject, mScriptObject,
                                "window_object");
#if 0
    // This code is useful for debugging leaks of global window objects
    // that are caused by leaked GC roots.  To use it, #define
    // GC_MARK_DEBUG at the top of js/src/jsgc.c and uncomment this
    // block.  The JS engine will then show why a global window is
    // not being garbage collected.
    extern void *js_LiveThingToFind;
    static int count = 0;
    if (++count == 2) {
      js_LiveThingToFind = mScriptObject;
    }
#endif
  }
  *aScriptObject = mScriptObject;
  return res;
}

NS_IMETHODIMP GlobalWindowImpl::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}
#endif

//*****************************************************************************
// GlobalWindowImpl::nsIScriptGlobalObject
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::SetContext(nsIScriptContext* aContext)
{
  // if setting the context to null, then we won't get to clean up the
  // named reference, so do it now
  if (!aContext) {
    NS_WARNING("Possibly early removal of script object, see bug #41608");
    mContext->RemoveReference(&mScriptObject, mScriptObject);
  } else {
    mScriptObject = ::JS_GetGlobalObject((JSContext *)aContext->GetNativeContext());
  }

  mContext = aContext;

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetContext(nsIScriptContext ** aContext)
{
  *aContext = mContext;
  NS_IF_ADDREF(*aContext);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetNewDocument(nsIDOMDocument* aDocument)
{
  if (!aDocument) {
    if (mDocument) {
      // Cache the old principal now that the document is being removed.
      nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

      doc->GetPrincipal(getter_AddRefs(mDocumentPrincipal));
    }
  } else {
    // let go of the old cached principal
    mDocumentPrincipal = nsnull;
  }

  // Always clear watchpoints, to deal with two cases:
  // 1.  The first document for this window is loading, and a miscreant has
  //     preset watchpoints on the window object in order to attack the new
  //     document's privileged information.
  // 2.  A document loaded and used watchpoints on its own window, leaving
  //     them set until the next document loads.  We must clean up window
  //     watchpoints here.
  // Watchpoints set on document and subordinate objects are all cleared
  // when those sub-window objects are finalized, after JS_ClearScope and
  // a GC run that finds them to be garbage.

  if (mContext && mScriptObject)
    ::JS_ClearWatchPointsForObject((JSContext *)mContext->GetNativeContext(),
                                   mScriptObject);

  if (mFirstDocumentLoad) {
    if (aDocument) {
      mFirstDocumentLoad = PR_FALSE;
    }

    mDocument = aDocument;

    if (mDocument) {
      // Get our private root. If it is equal to us, then we
      // need to attach our global key bindings that handle 
      // browser scrolling and other browser commands.
      nsCOMPtr<nsIDOMWindowInternal> internal;
      GetPrivateRoot(getter_AddRefs(internal));
      nsCOMPtr<nsIDOMWindowInternal> us(do_QueryInterface(NS_STATIC_CAST(nsIDOMWindow*,this)));
      if (internal == us) {
        nsresult rv;
        NS_WITH_SERVICE(nsIXBLService, xblService, "@mozilla.org/xbl;1", &rv);
        if (xblService) {
          nsCOMPtr<nsIDOMEventReceiver> rec(do_QueryInterface(mChromeEventHandler));
          xblService->AttachGlobalKeyHandler(rec);
          
          // for now, the only way to get drag/drop is to use the XUL wrapper. There
          // are just too many things that need to be added to hookup DnD with XBL (pinkerton)
          //xblService->AttachGlobalDragHandler(rec);
        }
      }
    }

    return NS_OK;
  }

  /* No mDocShell means we've already been partially closed down.
     When that happens, setting status isn't a big requirement,
     so don't. (Doesn't happen under normal circumstances, but
     bug 49615 describes a case.) */
  /* We only want to do this when we're setting a new document rather
     than going away.  See bug 61840.  */
  if (mDocShell && aDocument) {
    SetStatus(nsString());
    SetDefaultStatus(nsString());
  }

  if (mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    nsCOMPtr<nsIURI> docURL;

    if (doc) {
      docURL = dont_AddRef(doc->GetDocumentURL());
      doc = nsnull;             // Forces release now
    }

    if (docURL) {
      char *str;
      docURL->GetSpec(&str);

      nsAutoString url;
      url.AssignWithConversion(str);

      //about:blank URL's do not have ClearScope called on page change.
      if (!url.EqualsWithConversion("about:blank")) {
        ClearAllTimeouts();

        if (mSidebar) {
          mSidebar->SetWindow(nsnull);
          mSidebar = nsnull;
        }

        if (mListenerManager)
          mListenerManager->RemoveAllListeners(PR_FALSE);

        if (mContext && mScriptObject) {
//      if (mContext && mScriptObject && aDocument) {
//      not doing this unless there's a new document prevents a closed window's
//      JS properties from going away (that's good) and causes everything,
//      and I mean everything, to be leaked (that's bad)
          ::JS_ClearScope((JSContext *)mContext->GetNativeContext(),
                          mScriptObject);
        }
      }
      nsCRT::free(str);
    }

    //XXX Should this be outside the about:blank clearscope exception?
    mDocument = nsnull;         // Forces Release
  }

  if (mContext && aDocument) {
    // Add an extra ref in case we release mContext during GC.
    nsCOMPtr<nsIScriptContext> kungFuDeathGrip = mContext;
    kungFuDeathGrip->GC();
  }

  mDocument = aDocument;

  if (mDocument && mContext) {
    mContext->InitContext(this);

    JSContext *cx = (JSContext *)mContext->GetNativeContext();

    mScriptObject = ::JS_GetGlobalObject(cx);
  }

  // Clear our mutation bitfield.
  mMutationBits = 0;

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetDocShell(nsIDocShell* aDocShell)
{
  if (aDocShell == mDocShell)
    return NS_OK;

  /* SetDocShell(nsnull) means the window is being torn down. Set the
     "closed" JS property, Drop our reference to the script context,
     allowing it to be deleted later, and hand off our reference
     to the script object (held via a named JS root) to the context
     so it will be unrooted later. Meanwhile, keep our weak reference
     to the script object so it can be retrieved later, as the JS glue
     is wont to do. */
  if (!aDocShell && mContext) {
    ClearAllTimeouts();
    if (mScriptObject) {
      // Indicate that the window is now closed. Since we've
      // cleared scope, we have to explicitly set a property.
      jsval val = BOOLEAN_TO_JSVAL(JS_TRUE);
      ::JS_SetProperty((JSContext *)mContext->GetNativeContext(),
                       mScriptObject, "closed", &val);
      // hand off our reference to mContext
      mContext->SetRootedScriptObject(mScriptObject);
      mContext->RemoveReference(&mScriptObject, mScriptObject);
    }
    mContext = nsnull;          // force release now
    mControllers = nsnull;      // force release now
  }
  mDocShell = aDocShell;        // Weak Reference

  if (mLocation)
    mLocation->SetDocShell(aDocShell);
  if (mNavigator)
    mNavigator->SetDocShell(aDocShell);
  if (mHistory)
    mHistory->SetDocShell(aDocShell);
  if (mFrames)
    mFrames->SetDocShell(aDocShell);
  if (mScreen)
    mScreen->SetDocShell(aDocShell);

  if (mDocShell) {
    // tell our member elements about the new browserwindow
    if (mMenubar) {
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      GetWebBrowserChrome(getter_AddRefs(browserChrome));
      mMenubar->SetWebBrowserChrome(browserChrome);
    }

    // Get our enclosing chrome shell and retrieve its global window impl, so
    // that we can do some forwarding to the chrome document.
    mDocShell->GetChromeEventHandler(getter_AddRefs(mChromeEventHandler));
    if (!mChromeEventHandler) {
      // We have no chrome event handler. If we have a parent,
      // get our chrome event handler from the parent.  If
      // we don't have a parent, then we need to make a new
      // window root object that will function as a chrome event
      // handler and receive all events that occur anywhere inside
      // our window.
      nsCOMPtr<nsIDOMWindow> parentWindow;
      GetParent(getter_AddRefs(parentWindow));
      if (parentWindow.get() != NS_STATIC_CAST(nsIDOMWindow*,this)) {
        nsCOMPtr<nsPIDOMWindow> piWindow(do_QueryInterface(parentWindow));
        nsCOMPtr<nsIChromeEventHandler> handler;
        piWindow->GetChromeEventHandler(getter_AddRefs(mChromeEventHandler));
      }
      else NS_NewWindowRoot(this, getter_AddRefs(mChromeEventHandler));
    }
  }
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetDocShell(nsIDocShell ** aDocShell)
{
  *aDocShell = mDocShell;
  NS_IF_ADDREF(*aDocShell);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetOpenerWindow(nsIDOMWindowInternal* aOpener)
{
  mOpener = aOpener;
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetGlobalObjectOwner(nsIScriptGlobalObjectOwner* aOwner)
{
  mGlobalObjectOwner = aOwner;  // Note this is supposed to be a weak ref.
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetGlobalObjectOwner(nsIScriptGlobalObjectOwner ** aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);

  *aOwner = mGlobalObjectOwner;
  NS_IF_ADDREF(*aOwner);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::HandleDOMEvent(nsIPresContext* aPresContext,
                                               nsEvent* aEvent,
                                               nsIDOMEvent** aDOMEvent,
                                               PRUint32 aFlags,
                                               nsEventStatus* aEventStatus)
{
  nsresult ret = NS_OK;
  nsIDOMEvent *domEvent = nsnull;

  /* mChromeEventHandler and mContext go dangling in the middle of this
     function under some circumstances (events that destroy the window)
     without this addref. */
  nsCOMPtr<nsIChromeEventHandler> kungFuDeathGrip1(mChromeEventHandler);
  nsCOMPtr<nsIScriptContext> kungFuDeathGrip2(mContext);

  /* If this is a mouse event, use the struct to provide entropy for 
   * the system.
   */
  if (gEntropyCollector && !mChromeEventHandler &&
      (NS_EVENT_FLAG_BUBBLE != aFlags) && 
      (aEvent->message == NS_MOUSE_LEFT_BUTTON_DOWN)) {
    //Since the high bits seem to be zero's most of the time, 
    //let's only take the lowest half of the point structure.
    PRInt16 myCoord[4];

    myCoord[0] = aEvent->point.x;
    myCoord[1] = aEvent->point.y;
    myCoord[2] = aEvent->refPoint.x;
    myCoord[3] = aEvent->refPoint.y;
    gEntropyCollector->RandomUpdate((void*)myCoord, sizeof(myCoord));
    gEntropyCollector->RandomUpdate((void*)&aEvent->time, sizeof(PRUint32));
  }

  if (NS_EVENT_FLAG_INIT & aFlags) {
    if (!aDOMEvent) {
      aDOMEvent = &domEvent;
    }
    aEvent->flags = aFlags;
    aFlags &= ~(NS_EVENT_FLAG_CANT_BUBBLE | NS_EVENT_FLAG_CANT_CANCEL);

    // Execute bindingdetached handlers before we tear ourselves
    // down.
    if (aEvent->message == NS_PAGE_UNLOAD && mDocument) {
      nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
      nsCOMPtr<nsIBindingManager> bindingManager;
      doc->GetBindingManager(getter_AddRefs(bindingManager));
      if (bindingManager)
        bindingManager->ExecuteDetachedHandlers();
    }
  }

  // Capturing stage
  if ((NS_EVENT_FLAG_BUBBLE != aFlags) && mChromeEventHandler) {
    // Check chrome document capture here.
    // XXX The chrome can not handle this, see bug 51211
    if (aEvent->message != NS_IMAGE_LOAD) {
      mChromeEventHandler->HandleChromeEvent(aPresContext, aEvent, aDOMEvent,
                                             NS_EVENT_FLAG_CAPTURE,
                                             aEventStatus);
    }
  }

  // Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) &&
      !((NS_EVENT_FLAG_BUBBLE & aFlags) &&
        (NS_EVENT_FLAG_CANT_BUBBLE & aEvent->flags))) {
    aEvent->flags |= aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, this,
                                  aFlags, aEventStatus);
    aEvent->flags &= ~aFlags;
  }

  // Bubbling stage
  if ((NS_EVENT_FLAG_CAPTURE != aFlags) && mChromeEventHandler) {
    // Bubble to a chrome document if it exists
    // XXX Need a way to know if an event should really bubble or not.
    // For now filter out load and unload, since they cause problems.
    if ((aEvent->message != NS_PAGE_LOAD) &&
        (aEvent->message != NS_PAGE_UNLOAD) &&
        (aEvent->message != NS_IMAGE_LOAD) &&
        (aEvent->message != NS_FOCUS_CONTENT) &&
        (aEvent->message != NS_BLUR_CONTENT)) {
      mChromeEventHandler->HandleChromeEvent(aPresContext, aEvent,
                                             aDOMEvent, NS_EVENT_FLAG_BUBBLE,
                                             aEventStatus);
    }
  }

  if (NS_EVENT_FLAG_INIT & aFlags) {
    // We're leaving the DOM event loop so if we created an event, release here.
    if (*aDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (rc) {
        // Okay, so someone in the DOM loop (a listener, JS object) still has
        // a ref to the DOM Event but the internal data hasn't been malloc'd.
        // Force a copy of the data here so the DOM Event is still valid.
        nsCOMPtr<nsIPrivateDOMEvent>
          privateEvent(do_QueryInterface(*aDOMEvent));
        if (privateEvent)
          privateEvent->DuplicatePrivateData();
      }
    }
    aDOMEvent = nsnull;
  }

  return ret;
}

JSObject *
GlobalWindowImpl::GetGlobalJSObject()
{
  return mScriptObject;
}


//*****************************************************************************
// GlobalWindowImpl::nsIScriptObjectPrincipal
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::GetPrincipal(nsIPrincipal** result)
{
  NS_ENSURE_ARG_POINTER(result);

  if (mDocument) {
    // If we have a document, get the principal from the document
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

    return doc->GetPrincipal(result);
  }

  if (mDocumentPrincipal) {
    *result = mDocumentPrincipal;
    NS_ADDREF(*result);

    return NS_OK;
  }

  // If we don't have a principal and we don't have a document we
  // ask the parent window for the principal. This can happen when
  // loading a frameset that has a <frame src="javascript:xxx">, in
  // that case the global window is used in JS before we've loaded
  // a document into the window.
  nsCOMPtr<nsIDOMWindow> parent;

  GetParent(getter_AddRefs(parent));

  if (parent && (parent.get() != NS_STATIC_CAST(nsIDOMWindow *, this))) {
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal(do_QueryInterface(parent));

    if (objPrincipal) {
      return objPrincipal->GetPrincipal(result);
    }
  }

  return NS_ERROR_FAILURE;
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMWindow
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::GetDocument(nsIDOMDocument** aDocument)
{
  *aDocument = mDocument;
  NS_IF_ADDREF(*aDocument);
  return NS_OK;
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMWindowInternal
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::GetWindow(nsIDOMWindowInternal** aWindow)
{
  *aWindow = NS_STATIC_CAST(nsIDOMWindowInternal *, this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetSelf(nsIDOMWindowInternal** aWindow)
{
  *aWindow = NS_STATIC_CAST(nsIDOMWindowInternal *, this);
  NS_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetNavigator(nsIDOMNavigator** aNavigator)
{
  if (!mNavigator) {
    mNavigator = new NavigatorImpl(mDocShell);
    NS_ENSURE_TRUE(mNavigator, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(mNavigator);
  }

  *aNavigator = mNavigator;
  NS_ADDREF(*aNavigator);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetScreen(nsIDOMScreen** aScreen)
{
  if (!mScreen && mDocShell) {
    mScreen = new ScreenImpl(mDocShell);
    NS_ENSURE_TRUE(mScreen, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(mScreen);
  }

  *aScreen = mScreen;
  NS_ADDREF(*aScreen);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetHistory(nsIDOMHistory** aHistory)
{
  if (!mHistory && mDocShell) {
    mHistory = new HistoryImpl(mDocShell);
    NS_ENSURE_TRUE(mHistory, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(mHistory);
  }
  *aHistory = mHistory;
  NS_ADDREF(*aHistory);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetParent(nsIDOMWindow** aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);
  *aParent = nsnull;
  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> parent;
  docShellAsItem->GetSameTypeParent(getter_AddRefs(parent));

  if (parent) {
    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(parent));
    NS_ENSURE_SUCCESS(CallQueryInterface(globalObject.get(), aParent),
                      NS_ERROR_FAILURE);
  }
  else {
    *aParent = NS_STATIC_CAST(nsIDOMWindowInternal *, this);
    NS_ADDREF(*aParent);
  }
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetTop(nsIDOMWindow** aTop)
{
  nsresult ret = NS_OK;

  *aTop = nsnull;
  if (mDocShell) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
    nsCOMPtr<nsIDocShellTreeItem> root;
    docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(root));

    if (root) {
      nsCOMPtr<nsIScriptGlobalObject> globalObject(do_GetInterface(root));
      CallQueryInterface(globalObject.get(), aTop);
    }
  }

  return ret;
}

NS_IMETHODIMP GlobalWindowImpl::GetContent(nsIDOMWindow** aContent)
{
  *aContent = nsnull;

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> primaryContent;
  treeOwner->GetPrimaryContentShell(getter_AddRefs(primaryContent));

  nsCOMPtr<nsIDOMWindowInternal> domWindow(do_GetInterface(primaryContent));
  *aContent = domWindow;
  NS_IF_ADDREF(*aContent);

  return NS_OK;
}

// XXX for sidebar HACK, see bug 20721
NS_IMETHODIMP GlobalWindowImpl::GetSidebar(nsISidebar** aSidebar)
{
  nsresult rv = NS_OK;

  if (!mSidebar) {
    mSidebar = do_CreateInstance(NS_SIDEBAR_CONTRACTID, &rv);

    if (mSidebar) {
      nsIDOMWindowInternal *win = NS_STATIC_CAST(nsIDOMWindowInternal *, this);
      /* no addref */
      mSidebar->SetWindow(win);
    }
  }

  *aSidebar = mSidebar;
  NS_IF_ADDREF(*aSidebar);

  return rv;

}

NS_IMETHODIMP GlobalWindowImpl::GetPrompter(nsIPrompt** aPrompt)
{
  if (!mDocShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_NO_INTERFACE);

  NS_ADDREF(*aPrompt = prompter);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetMenubar(nsIDOMBarProp** aMenubar)
{
  if (!mMenubar) {
    mMenubar = new MenubarPropImpl();
    if (mMenubar) {
      NS_ADDREF(mMenubar);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mMenubar-> SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aMenubar = mMenubar;
  NS_IF_ADDREF(mMenubar);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetToolbar(nsIDOMBarProp** aToolbar)
{
  if (!mToolbar) {
    mToolbar = new ToolbarPropImpl();
    if (mToolbar) {
      NS_ADDREF(mToolbar);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mToolbar->SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aToolbar = mToolbar;
  NS_IF_ADDREF(mToolbar);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetLocationbar(nsIDOMBarProp** aLocationbar)
{
  if (!mLocationbar) {
    mLocationbar = new LocationbarPropImpl();
    if (mLocationbar) {
      NS_ADDREF(mLocationbar);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mLocationbar->SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aLocationbar = mLocationbar;
  NS_IF_ADDREF(mLocationbar);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetPersonalbar(nsIDOMBarProp** aPersonalbar)
{
  if (!mPersonalbar) {
    mPersonalbar = new PersonalbarPropImpl();
    if (mPersonalbar) {
      NS_ADDREF(mPersonalbar);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mPersonalbar->SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aPersonalbar = mPersonalbar;
  NS_IF_ADDREF(mPersonalbar);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetStatusbar(nsIDOMBarProp** aStatusbar)
{
  if (!mStatusbar) {
    mStatusbar = new StatusbarPropImpl();
    if (mStatusbar) {
      NS_ADDREF(mStatusbar);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mStatusbar->SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aStatusbar = mStatusbar;
  NS_IF_ADDREF(mStatusbar);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetScrollbars(nsIDOMBarProp** aScrollbars)
{
  if (!mScrollbars) {
    mScrollbars = new ScrollbarsPropImpl();
    if (mScrollbars) {
      NS_ADDREF(mScrollbars);
      nsCOMPtr<nsIWebBrowserChrome> browserChrome;
      if (mDocShell &&
          NS_SUCCEEDED(GetWebBrowserChrome(getter_AddRefs(browserChrome)))) {
        mScrollbars->SetWebBrowserChrome(browserChrome);
      }
    }
  }

  *aScrollbars = mScrollbars;
  NS_IF_ADDREF(mScrollbars);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetDirectories(nsIDOMBarProp** aDirectories)
{
  return GetPersonalbar(aDirectories);
}

NS_IMETHODIMP GlobalWindowImpl::GetClosed(PRBool* aClosed)
{
  *aClosed = !mDocShell;
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetFrames(nsIDOMWindowCollection** aFrames)
{
  if (!mFrames && mDocShell) {
    mFrames = new nsDOMWindowList(mDocShell);
    NS_ENSURE_TRUE(mFrames, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(mFrames);
  }

  *aFrames = NS_STATIC_CAST(nsIDOMWindowCollection *, mFrames);
  NS_IF_ADDREF(mFrames);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetCrypto(nsIDOMCrypto** aCrypto)
{
  nsresult rv;

  if (!mCrypto) {
    mCrypto = do_CreateInstance(kCryptoContractID, &rv);
  }
  *aCrypto = mCrypto;
  NS_IF_ADDREF(*aCrypto);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetPkcs11(nsIDOMPkcs11** aPkcs11)
{
  nsresult rv;

  if (!mPkcs11) {
    mPkcs11 = do_CreateInstance(kPkcs11ContractID, &rv);
  }
  *aPkcs11 = mPkcs11;
  NS_IF_ADDREF(*aPkcs11);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetControllers(nsIControllers** aResult)
{
  if (!mControllers) {
    mControllers = do_CreateInstance(kXULControllersCID);
    NS_ENSURE_TRUE(mControllers, NS_ERROR_FAILURE);
#ifdef DOM_CONTROLLER
    // Add in the default controller
    nsDOMWindowController *domController = new nsDOMWindowController(this);
    if (domController) {
      nsCOMPtr<nsIController> controller(domController);
      mControllers->AppendController(controller);
    }
#endif // DOM_CONTROLLER
  }
  *aResult = mControllers;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetOpener(nsIDOMWindowInternal** aOpener)
{
  *aOpener = mOpener;
  NS_IF_ADDREF(*aOpener);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetOpener(nsIDOMWindowInternal* aOpener)
{
  mOpener = aOpener;
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetStatus(nsAWritableString& aStatus)
{
  aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetStatus(const nsAReadableString& aStatus)
{
  mStatus = aStatus;

   nsCOMPtr<nsIWebBrowserChrome> browserChrome;
   GetWebBrowserChrome(getter_AddRefs(browserChrome));
   if(browserChrome)
      browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_SCRIPT, nsPromiseFlatString(aStatus).get());

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetDefaultStatus(nsAWritableString& aDefaultStatus)
{
  aDefaultStatus = mDefaultStatus;
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetDefaultStatus(const nsAReadableString& aDefaultStatus)
{
  mDefaultStatus = aDefaultStatus;

   nsCOMPtr<nsIWebBrowserChrome> browserChrome;
   GetWebBrowserChrome(getter_AddRefs(browserChrome));
   if(browserChrome)
      browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_SCRIPT_DEFAULT, nsPromiseFlatString(aDefaultStatus).get());

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetName(nsAWritableString& aName)
{
  nsXPIDLString name;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  if (docShellAsItem)
    docShellAsItem->GetName(getter_Copies(name));

  aName.Assign(name);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetName(const nsAReadableString& aName)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  if (docShellAsItem)
    result = docShellAsItem->SetName(nsPromiseFlatString(aName).get());
  return result;
}

/*
NS_IMETHODIMP    
GlobalWindowImpl::SetLocation(jsval aLocation)
{
  nsresult result;

  if (!mLocation && mDocShell) {
    mLocation = new LocationImpl(mDocShell);
    NS_IF_ADDREF(mLocation);
  }

  if (mLocation) {
    nsCOMPtr<nsIThreadJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
    NS_ENSURE_TRUE(stack, NS_ERROR_FAILURE);

    JSContext* cx;
    NS_ENSURE_SUCCESS(stack->Peek(&cx), NS_ERROR_FAILURE);

    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);
     
    result = mLocation->SetHrefWithContext(cx, aLocation);
    if (NS_FAILED(result)) return result;
  }

  return NS_OK;
}
*/

NS_IMETHODIMP    
GlobalWindowImpl::GetTitle(nsAWritableString& aTitle)
{
  aTitle = mTitle;
  return NS_OK;
}

NS_IMETHODIMP    
GlobalWindowImpl::SetTitle(const nsAReadableString& aTitle)
{
  mTitle = aTitle;
  if(mDocShell) {
    // See if we're a chrome shell.
    PRInt32 type;
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
    docShellAsItem->GetItemType(&type);
    if(type == nsIDocShellTreeItem::typeChrome) {
      nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
      if(docShellAsWin) {
        docShellAsWin->SetTitle(nsPromiseFlatString(mTitle).get());
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetInnerWidth(PRInt32* aInnerWidth)
{
  nsCOMPtr<nsIBaseWindow> docShellWin(do_QueryInterface(mDocShell));
  *aInnerWidth = 0;
  if (docShellWin)
    docShellWin->GetSize(aInnerWidth, nsnull);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetInnerWidth(PRInt32 aInnerWidth)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> docShellParent;
  docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

  // It's only valid to access this from a top window.  Doesn't work from 
  // sub-frames.
  if (docShellParent)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aInnerWidth, nsnull),
                    NS_ERROR_FAILURE);

  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  PRInt32 cy = 0;
  docShellAsWin->GetSize(nsnull, &cy);
  NS_ENSURE_SUCCESS(treeOwner->SizeShellTo(docShellAsItem, aInnerWidth, cy),
                    NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetInnerHeight(PRInt32* aInnerHeight)
{
  FlushPendingNotifications();

  nsCOMPtr<nsIBaseWindow> docShellWin(do_QueryInterface(mDocShell));
  *aInnerHeight = 0;
  if (docShellWin)
    docShellWin->GetSize(nsnull, aInnerHeight);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetInnerHeight(PRInt32 aInnerHeight)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> docShellParent;
  docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

  // It's only valid to access this from a top window.  Doesn't work from 
  // sub-frames.
  if (docShellParent)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_ENSURE_TRUE(treeOwner, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(nsnull, &aInnerHeight),
                    NS_ERROR_FAILURE);

  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  PRInt32 cx = 0;
  docShellAsWin->GetSize(&cx, nsnull);
  NS_ENSURE_SUCCESS(treeOwner->
                    SizeShellTo(docShellAsItem, cx, aInnerHeight),
                    NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetOuterWidth(PRInt32* aOuterWidth)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  FlushPendingNotifications();

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(aOuterWidth, nsnull),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetOuterWidth(PRInt32 aOuterWidth)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aOuterWidth, nsnull),
                    NS_ERROR_FAILURE);

  PRInt32 cy;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(nsnull, &cy), NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(aOuterWidth, cy, PR_TRUE),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetOuterHeight(PRInt32* aOuterHeight)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  FlushPendingNotifications();

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(nsnull, aOuterHeight),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetOuterHeight(PRInt32 aOuterHeight)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(nsnull, &aOuterHeight),
                    NS_ERROR_FAILURE);

  PRInt32 cx;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&cx, nsnull), NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(cx, aOuterHeight, PR_TRUE),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetScreenX(PRInt32* aScreenX)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  FlushPendingNotifications();

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(aScreenX, nsnull),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetScreenX(PRInt32 aScreenX)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&aScreenX, nsnull),
                    NS_ERROR_FAILURE);

  PRInt32 y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(nsnull, &y),
                    NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(aScreenX, y),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetScreenY(PRInt32* aScreenY)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  FlushPendingNotifications();

  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(nsnull, aScreenY),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SetScreenY(PRInt32 aScreenY)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(nsnull, &aScreenY),
                    NS_ERROR_FAILURE);

  PRInt32 x;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, nsnull),
                    NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(x, aScreenY),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
GlobalWindowImpl::CheckSecurityWidthAndHeight(PRInt32* aWidth,
                                              PRInt32* aHeight)
{
  // This one is easy.  Just ensure the variable is greater than 100;
  if ((aWidth && *aWidth < 100) || (aHeight && *aHeight < 100)) {
    // Check security state for use in determing window dimensions
    nsCOMPtr<nsIScriptSecurityManager>
      securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
    NS_ENSURE_TRUE(securityManager, NS_ERROR_FAILURE);

    PRBool enabled;
    nsresult res =
      securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);

    if (NS_FAILED(res) || !enabled) {
      //sec check failed
      if (aWidth && *aWidth < 100) {
        *aWidth = 100;
      }
      if (aHeight && *aHeight < 100) {
        *aHeight = 100;
      }
    }
  }

  return NS_OK;
}

nsresult
GlobalWindowImpl::CheckSecurityLeftAndTop(PRInt32* aLeft, PRInt32* aTop)
{
  // This one is harder.  We have to get the screen size and window dimensions.

  // Check security state for use in determing window dimensions
  nsCOMPtr<nsIScriptSecurityManager>
    securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(securityManager, NS_ERROR_FAILURE);

  PRBool enabled;
  nsresult res =
    securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);
  if (NS_FAILED(res)) {
    enabled = PR_FALSE;
  }

  if (!enabled) {
    PRInt32 screenLeft, screenTop, screenWidth, screenHeight;
    PRInt32 winLeft, winTop, winWidth, winHeight;

    FlushPendingNotifications();

    // Get the window size
    nsCOMPtr<nsIBaseWindow> treeOwner;
    GetTreeOwner(getter_AddRefs(treeOwner));
    if (treeOwner)
      treeOwner->GetPositionAndSize(&winLeft, &winTop, &winWidth, &winHeight);

    // Get the screen dimensions
    // XXX This should use nsIScreenManager once it's fully fleshed out.
    nsCOMPtr<nsIDOMScreen> screen;
    GetScreen(getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailLeft(&screenLeft);
      screen->GetAvailWidth(&screenWidth);
      screen->GetAvailHeight(&screenHeight);
#ifdef XP_MAC
      /* The mac's coordinate system is different from the assumed Windows'
         system. It offsets by the height of the menubar so that a window
         placed at (0,0) will be entirely visible. Unfortunately that
         correction is made elsewhere (in Widget) and the meaning of
         the Avail... coordinates is overloaded. Here we allow a window
         to be placed at (0,0) because it does make sense to do so.
      */
      screen->GetTop(&screenTop);
#else
      screen->GetAvailTop(&screenTop);
#endif
    }

    if (screen && treeOwner) {
      if (aLeft) {
        if (screenLeft+screenWidth < *aLeft+winWidth)
          *aLeft = screenLeft+screenWidth - winWidth;
        if (screenLeft > *aLeft)
          *aLeft = screenLeft;
      }
      if (aTop) {
        if (screenTop+screenHeight < *aTop+winHeight)
          *aTop = screenTop+screenHeight - winHeight;
        if (screenTop > *aTop)
          *aTop = screenTop;
      }
    } else {
      if (aLeft)
        *aLeft = 0;
      if (aTop)
        *aTop = 0;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetPageXOffset(PRInt32* aPageXOffset)
{
  return GetScrollX(aPageXOffset);
}

NS_IMETHODIMP GlobalWindowImpl::GetPageYOffset(PRInt32* aPageYOffset)
{
  return GetScrollY(aPageYOffset);
}

NS_IMETHODIMP GlobalWindowImpl::GetScrollX(PRInt32* aScrollX)
{
  NS_ENSURE_ARG_POINTER(aScrollX);
  nsresult result = NS_OK;
  nsIScrollableView *view;      // no addref/release for views
  float p2t, t2p;

  *aScrollX = 0;

  GetScrollInfo(&view, &p2t, &t2p);
  if (view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    *aScrollX = NSTwipsToIntPixels(xPos, t2p);
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::GetScrollY(PRInt32* aScrollY)
{
  NS_ENSURE_ARG_POINTER(aScrollY);
  nsresult result = NS_OK;
  nsIScrollableView *view;      // no addref/release for views
  float p2t, t2p;

  *aScrollY = 0;

  GetScrollInfo(&view, &p2t, &t2p);
  if (view) {
    nscoord xPos, yPos;
    result = view->GetScrollPosition(xPos, yPos);
    *aScrollY = NSTwipsToIntPixels(yPos, t2p);
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::GetLength(PRUint32* aLength)
{
  nsCOMPtr<nsIDOMWindowCollection> frames;
  if (NS_SUCCEEDED(GetFrames(getter_AddRefs(frames))) && frames) {
    return frames->GetLength(aLength);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::Dump(const nsAReadableString& aStr)
{
  char *cstr = ToNewUTF8String(aStr);

#ifdef XP_MAC
  // have to convert \r to \n so that printing to the console works
  char *c = cstr, *cEnd = cstr + aStr.Length();
  while (c < cEnd) {
    if (*c == '\r')
      *c = '\n';
    c++;
  }
#endif

  if (cstr) {
    printf("%s", cstr);
    nsCRT::free(cstr);
  }

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Alert(const nsAReadableString& aString)
{
  NS_ENSURE_STATE(mDocShell);

  nsAutoString str;

  str.Assign(aString);

  // XXX: Concatenation of optional args?

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  return prompter->Alert(nsnull, str.GetUnicode());
}

NS_IMETHODIMP
GlobalWindowImpl::Confirm(const nsAReadableString& aString, PRBool* aReturn)
{
  NS_ENSURE_STATE(mDocShell);

  nsAutoString str;

  *aReturn = PR_FALSE;

  str.Assign(aString);

  // XXX: Concatenation of optional args?

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));
  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  return prompter->Confirm(nsnull, str.GetUnicode(), aReturn);
}

NS_IMETHODIMP
GlobalWindowImpl::Prompt(const nsAReadableString& aMessage,
                         const nsAReadableString& aInitial,
                         const nsAReadableString& aTitle,
                         const PRUint32 aSavePassword,
                         nsAWritableString& aReturn)
{
  NS_ENSURE_STATE(mDocShell);

  aReturn.Truncate(); // XXX Null string!!!

  nsresult rv = NS_OK;

  nsCOMPtr<nsIPrompt> prompter(do_GetInterface(mDocShell));

  NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

  PRBool b;
  nsXPIDLString uniResult;

  rv = prompter->Prompt(nsPromiseFlatString(aTitle).get(),
                        nsPromiseFlatString(aMessage).get(), nsnull,
                        aSavePassword,
                        nsPromiseFlatString(aInitial).get(),
                        getter_Copies(uniResult), &b);
  NS_ENSURE_SUCCESS(rv, rv);

  if (uniResult && b) {
    aReturn.Assign(uniResult);
  }
  else {
    // XXX: Null out the return string!!!
  }

  return rv;
}

static JSContext * GetCurrentContext()
{
  // Get JSContext from stack.



  // Cache stack!!!

  nsresult rv;
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);

  if (NS_FAILED(rv))
    return nsnull;

  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)))
    return nsnull;

  return cx;
}

NS_IMETHODIMP
GlobalWindowImpl::Prompt(nsAWritableString& aReturn)
{
  NS_ENSURE_STATE(mDocShell);
  NS_ENSURE_STATE(sXPConnect);

  JSContext *cx = GetCurrentContext();

  if (!cx) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIXPCNativeCallContext> ncc;

  rv = sXPConnect->GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoString message, initial, title;
  PRUint32 savePassword = nsIPrompt::SAVE_PASSWORD_NEVER;

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  if (argc > 0) {
    nsJSUtils::nsConvertJSValToString(message, cx, argv[0]);

    if (argc > 1) {
      nsJSUtils::nsConvertJSValToString(initial, cx, argv[1]);

      if (argc > 2) {
        nsJSUtils::nsConvertJSValToString(title, cx, argv[2]);

        if (argc > 3) {
          nsJSUtils::nsConvertJSValToUint32(&savePassword, cx, argv[3]);
        }
      }
    }
  }

  return Prompt(message, initial, title, savePassword, aReturn);
}

NS_IMETHODIMP GlobalWindowImpl::Focus()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  if (treeOwnerAsWin)
    treeOwnerAsWin->SetVisibility(PR_TRUE);

  nsCOMPtr<nsIPresShell> presShell;
  if (mDocShell)
    mDocShell->GetPresShell(getter_AddRefs(presShell));

  nsresult result = NS_OK;
  if (presShell) {
    nsCOMPtr<nsIViewManager> vm;
    presShell->GetViewManager(getter_AddRefs(vm));
    if (vm) {
      nsCOMPtr<nsIWidget> widget;
      vm->GetWidget(getter_AddRefs(widget));
      if (widget)
        // raise the window since this was a focus call on the window.
        result = widget->SetFocus(PR_TRUE);
    }
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::Blur()
{
  nsCOMPtr<nsIDocShellTreeItem>
    docShellAsItem(do_QueryInterface(mDocShell));
  if (docShellAsItem) {
    nsCOMPtr<nsIDocShellTreeItem> parent;
    // Parent regardless of chrome or content boundary
    docShellAsItem->GetParent(getter_AddRefs(parent));

    nsCOMPtr<nsIBaseWindow> newFocusWin;

    if (parent)
      newFocusWin = do_QueryInterface(parent);
    else {
      nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
      docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
      newFocusWin = do_QueryInterface(treeOwner);
    }

    if (newFocusWin)
      newFocusWin->SetFocus();
  }

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Back()
{
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  return webNav->GoBack();
}

NS_IMETHODIMP GlobalWindowImpl::Forward()
{
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

  return webNav->GoForward();
}

NS_IMETHODIMP GlobalWindowImpl::Home()
{
  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
  NS_ENSURE_TRUE(prefs, NS_ERROR_FAILURE);

  // if we get here, we know prefs is not null
  nsXPIDLString url;
  prefs->GetLocalizedUnicharPref(PREF_BROWSER_STARTUP_HOMEPAGE,
                                 getter_Copies(url));
  nsString homeURL;
  if (!url) {
    // if all else fails, use this
#ifdef DEBUG_seth
    printf("all else failed.  using %s as the home page\n", DEFAULT_HOME_PAGE);
#endif
    homeURL.AssignWithConversion(DEFAULT_HOME_PAGE);
  }
  else
    homeURL = url;
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(webNav->LoadURI(homeURL.GetUnicode(), nsIWebNavigation::LOAD_FLAGS_NONE), NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Stop()
{
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
  return webNav->Stop();
}

NS_IMETHODIMP GlobalWindowImpl::Print()
{
  if (mDocShell) {
    nsCOMPtr<nsIContentViewer> viewer;
    mDocShell->GetContentViewer(getter_AddRefs(viewer));
    if (viewer) {
      nsCOMPtr<nsIContentViewerFile> viewerFile(do_QueryInterface(viewer));
      if (viewerFile)
        return viewerFile->Print(PR_FALSE, nsnull);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::MoveTo(PRInt32 aXPos, PRInt32 aYPos)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&aXPos, &aYPos),
                    NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(aXPos, aYPos),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::MoveBy(PRInt32 aXDif, PRInt32 aYDif)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  PRInt32 x, y;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetPosition(&x, &y), NS_ERROR_FAILURE);

  PRInt32 newX = x + aXDif;
  PRInt32 newY = y + aYDif;
  NS_ENSURE_SUCCESS(CheckSecurityLeftAndTop(&newX, &newY), NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetPosition(newX, newY),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::ResizeTo(PRInt32 aWidth, PRInt32 aHeight)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&aWidth, &aHeight),
                    NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(aWidth, aHeight, PR_TRUE),
                    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::ResizeBy(PRInt32 aWidthDif, PRInt32 aHeightDif)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  PRInt32 cx, cy;
  NS_ENSURE_SUCCESS(treeOwnerAsWin->GetSize(&cx, &cy), NS_ERROR_FAILURE);

  PRInt32 newCX = cx + aWidthDif;
  PRInt32 newCY = cy + aHeightDif;
  NS_ENSURE_SUCCESS(CheckSecurityWidthAndHeight(&newCX, &newCY),
                    NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(treeOwnerAsWin->SetSize(newCX, newCY,
                                            PR_TRUE), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::SizeToContent()
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> docShellParent;
  docShellAsItem->GetSameTypeParent(getter_AddRefs(docShellParent));

  // It's only valid to access this from a top window.  Doesn't work from 
  // sub-frames.
  if (docShellParent)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  nsCOMPtr<nsIMarkupDocumentViewer> markupViewer(do_QueryInterface(cv));
  NS_ENSURE_TRUE(markupViewer, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(markupViewer->SizeToContent(), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetAttention()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  nsCOMPtr<nsIWidget> widget;
  treeOwnerAsWin->GetMainWidget(getter_AddRefs(widget));
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(widget->GetAttention(), NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Scroll(PRInt32 aXScroll, PRInt32 aYScroll)
{
  return ScrollTo(aXScroll, aYScroll);
}

NS_IMETHODIMP GlobalWindowImpl::ScrollTo(PRInt32 aXScroll, PRInt32 aYScroll)
{
  nsresult result;
  nsIScrollableView *view;      // no addref/release for views
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
    result = view->ScrollTo(NSIntPixelsToTwips(aXScroll, p2t),
                            NSIntPixelsToTwips(aYScroll, p2t),
                            NS_VMREFRESH_IMMEDIATE);
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::ScrollBy(PRInt32 aXScrollDif,
                                         PRInt32 aYScrollDif)
{
  nsresult result;
  nsIScrollableView *view;      // no addref/release for views
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);

  if (view) {
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

NS_IMETHODIMP GlobalWindowImpl::ScrollByLines(PRInt32 numLines)
{
  nsresult result;
  nsIScrollableView *view = nsnull;   // no addref/release for views
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (view)
  {
    result = view->ScrollByLines(0, numLines);
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::ScrollByPages(PRInt32 numPages)
{
  nsresult result;
  nsIScrollableView *view = nsnull;   // no addref/release for views
  float p2t, t2p;

  result = GetScrollInfo(&view, &p2t, &t2p);
  if (view)
  {
    result = view->ScrollByPages(numPages);
  }

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::ClearTimeout(PRInt32 aTimerID)
{
  return ClearTimeoutOrInterval(aTimerID);
}

NS_IMETHODIMP GlobalWindowImpl::ClearInterval(PRInt32 aTimerID)
{
  return ClearTimeoutOrInterval(aTimerID);
}

NS_IMETHODIMP
GlobalWindowImpl::SetTimeout(PRBool *_retval)
{
  return SetTimeoutOrInterval(PR_FALSE, _retval);
}

NS_IMETHODIMP
GlobalWindowImpl::SetInterval(PRBool *_retval)
{
  return SetTimeoutOrInterval(PR_TRUE, _retval);
}

NS_IMETHODIMP
GlobalWindowImpl::SetResizable(PRBool aResizable)
{
  // nop

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::CaptureEvents(PRInt32 aEventFlags)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  if (NS_SUCCEEDED(GetListenerManager(getter_AddRefs(manager)))) {
    manager->CaptureEvent(aEventFlags);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::ReleaseEvents(PRInt32 aEventFlags)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  if (NS_SUCCEEDED(GetListenerManager(getter_AddRefs(manager)))) {
    manager->ReleaseEvent(aEventFlags);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::RouteEvent(nsIDOMEvent* aEvt)
{
  //XXX Not the best solution -joki
  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::EnableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::DisableExternalCapture()
{
  return NS_ERROR_FAILURE;
}

//Note: This call will lock the cursor, it will not change as it moves.
//To unlock, the cursor must be set back to CURSOR_AUTO.
NS_IMETHODIMP GlobalWindowImpl::SetCursor(const nsAReadableString& aCursor)
{
  nsresult ret = NS_OK;
  PRInt32 cursor;

  if (aCursor.Equals(NS_LITERAL_STRING("auto")))
    cursor = NS_STYLE_CURSOR_AUTO;
  else if (aCursor.Equals(NS_LITERAL_STRING("default")))
    cursor = NS_STYLE_CURSOR_DEFAULT;
  else if (aCursor.Equals(NS_LITERAL_STRING("pointer")))
    cursor = NS_STYLE_CURSOR_POINTER;
  else if (aCursor.Equals(NS_LITERAL_STRING("crosshair")))
    cursor = NS_STYLE_CURSOR_CROSSHAIR;
  else if (aCursor.Equals(NS_LITERAL_STRING("move")))
    cursor = NS_STYLE_CURSOR_MOVE;
  else if (aCursor.Equals(NS_LITERAL_STRING("text")))
    cursor = NS_STYLE_CURSOR_TEXT;
  else if (aCursor.Equals(NS_LITERAL_STRING("wait")))
    cursor = NS_STYLE_CURSOR_WAIT;
  else if (aCursor.Equals(NS_LITERAL_STRING("help")))
    cursor = NS_STYLE_CURSOR_HELP;
  else if (aCursor.Equals(NS_LITERAL_STRING("n-resize")))
    cursor = NS_STYLE_CURSOR_N_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("s-resize")))
    cursor = NS_STYLE_CURSOR_S_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("w-resize")))
    cursor = NS_STYLE_CURSOR_W_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("e-resize")))
    cursor = NS_STYLE_CURSOR_E_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("ne-resize")))
    cursor = NS_STYLE_CURSOR_NE_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("nw-resize")))
    cursor = NS_STYLE_CURSOR_NW_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("se-resize")))
    cursor = NS_STYLE_CURSOR_SE_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("sw-resize")))
    cursor = NS_STYLE_CURSOR_SW_RESIZE;
  else if (aCursor.Equals(NS_LITERAL_STRING("copy")))
    cursor = NS_STYLE_CURSOR_COPY;      // CSS3
  else if (aCursor.Equals(NS_LITERAL_STRING("alias")))
    cursor = NS_STYLE_CURSOR_ALIAS;
  else if (aCursor.Equals(NS_LITERAL_STRING("context-menu")))
    cursor = NS_STYLE_CURSOR_CONTEXT_MENU;
  else if (aCursor.Equals(NS_LITERAL_STRING("cell")))
    cursor = NS_STYLE_CURSOR_CELL;
  else if (aCursor.Equals(NS_LITERAL_STRING("grab")))
    cursor = NS_STYLE_CURSOR_GRAB;
  else if (aCursor.Equals(NS_LITERAL_STRING("grabbing")))
    cursor = NS_STYLE_CURSOR_GRABBING;
  else if (aCursor.Equals(NS_LITERAL_STRING("spinning")))
    cursor = NS_STYLE_CURSOR_SPINNING;
  else if (aCursor.Equals(NS_LITERAL_STRING("count-up")))
    cursor = NS_STYLE_CURSOR_COUNT_UP;
  else if (aCursor.Equals(NS_LITERAL_STRING("count-down")))
    cursor = NS_STYLE_CURSOR_COUNT_DOWN;
  else if (aCursor.Equals(NS_LITERAL_STRING("count-up-down")))
    cursor = NS_STYLE_CURSOR_COUNT_UP_DOWN;
  else
    return NS_OK;

  nsCOMPtr<nsIPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (presContext) {
    nsCOMPtr<nsIEventStateManager> esm;
    if (NS_SUCCEEDED(presContext->GetEventStateManager(getter_AddRefs(esm)))) {
      // Need root widget.
      nsCOMPtr<nsIPresShell> presShell;
      mDocShell->GetPresShell(getter_AddRefs(presShell));
      NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

      nsCOMPtr<nsIViewManager> vm;
      presShell->GetViewManager(getter_AddRefs(vm));
      NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

      nsIView *rootView;
      vm->GetRootView(rootView);
      NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);

      nsCOMPtr<nsIWidget> widget;
      rootView->GetWidget(*getter_AddRefs(widget));
      NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

      // Call esm and set cursor.
      ret = esm->SetCursor(cursor, widget, PR_TRUE);
    }
  }

  return ret;
}

NS_IMETHODIMP
GlobalWindowImpl::Open(const nsAReadableString& aUrl,
                       const nsAReadableString& aName,
                       const nsAReadableString& aOptions,
                       nsIDOMWindow **_retval)
{
  return OpenInternal(aUrl, aName, aOptions, PR_FALSE, nsnull, 0, nsnull,
                      _retval);
}

NS_IMETHODIMP
GlobalWindowImpl::Open(nsIDOMWindow **_retval)
{
  NS_ENSURE_STATE(sXPConnect);

  JSContext *cx = GetCurrentContext();

  if (!cx) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIXPCNativeCallContext> ncc;

  rv = sXPConnect->GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoString url, name, options;

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  if (argc > 0) {
    nsJSUtils::nsConvertJSValToString(url, cx, argv[0]);

    if (argc > 1) {
      nsJSUtils::nsConvertJSValToString(name, cx, argv[1]);

      if (argc > 2) {
        nsJSUtils::nsConvertJSValToString(options, cx, argv[2]);
      }
    }
  }

  return OpenInternal(url, name, options, PR_FALSE, nsnull, 0, nsnull,
                      _retval);
}

// like Open, but attaches to the new window any extra parameters past 
// [features] as a JS property named "arguments"
NS_IMETHODIMP
GlobalWindowImpl::OpenDialog(const nsAReadableString& aUrl,
                             const nsAReadableString& aName,
                             const nsAReadableString& aOptions,
                             nsISupports* aExtraArgument,
                             nsIDOMWindow** _retval)
{
  return OpenInternal(aUrl, aName, aOptions, PR_TRUE, nsnull, 0,
                      aExtraArgument, _retval);
}

NS_IMETHODIMP
GlobalWindowImpl::OpenDialog(nsIDOMWindow** _retval)
{
  NS_ENSURE_STATE(sXPConnect);

  JSContext *cx = GetCurrentContext();

  if (!cx) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIXPCNativeCallContext> ncc;

  rv = sXPConnect->GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoString url, name, options;

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  if (argc > 0) {
    nsJSUtils::nsConvertJSValToString(url, cx, argv[0]);

    if (argc > 1) {
      nsJSUtils::nsConvertJSValToString(name, cx, argv[1]);

      if (argc > 2) {
        nsJSUtils::nsConvertJSValToString(options, cx, argv[2]);
      }
    }
  }

  return OpenInternal(url, name, options, PR_TRUE, argv, argc, nsnull,
                      _retval);
}

NS_IMETHODIMP
GlobalWindowImpl::GetFrames(nsIDOMWindow** aFrames)
{
  *aFrames = this;
  NS_ADDREF(*aFrames);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::Close()
{
  // Note: the basic security check, rejecting windows not opened through JS,
  // has been removed. This was approved long ago by ...you're going to call me
  // on this, aren't you... well it was. And anyway, a better means is coming.
  // In the new world of application-level interfaces being written in JS, this
  // security check was causing problems.

  JSContext *cx = GetCurrentContext();

  if (cx) {
    nsCOMPtr<nsIScriptContext> currentCX = 
      NS_STATIC_CAST(nsIScriptContext *, JS_GetContextPrivate(cx));

    if (currentCX && currentCX == mContext) {
      return currentCX->SetTerminationFunction(CloseWindow, NS_STATIC_CAST(nsIScriptGlobalObject *, this));
    }
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  treeOwnerAsWin->Destroy();
  CleanUp();

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::UpdateCommands(const nsAReadableString& anAction)
{
  nsCOMPtr<nsIDOMWindowInternal> rootWindow;
  GetPrivateRoot(getter_AddRefs(rootWindow));
  if (!rootWindow)
    return NS_OK;

  nsCOMPtr<nsIDOMDocument> document;
  rootWindow->GetDocument(getter_AddRefs(document));

  if (document) {
    // See if we contain a XUL document.
    nsCOMPtr<nsIDOMXULDocument> xulDoc = do_QueryInterface(document);
    if (xulDoc) {
      // Retrieve the command dispatcher and call updateCommands on it.
      nsCOMPtr<nsIDOMXULCommandDispatcher> xulCommandDispatcher;
      xulDoc->GetCommandDispatcher(getter_AddRefs(xulCommandDispatcher));
      xulCommandDispatcher->UpdateCommands(anAction);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Escape(const nsAReadableString& aStr, nsAWritableString& aReturn)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIUnicodeEncoder> encoder;
  nsAutoString charset;

  nsCOMPtr<nsICharsetConverterManager>
    ccm(do_GetService(kCharsetConverterManagerCID));
  NS_ENSURE_TRUE(ccm, NS_ERROR_FAILURE);

  // Get the document character set
  charset.AssignWithConversion("UTF-8");        // default to utf-8
  if (mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));

    if (doc)
      result = doc->GetDocumentCharacterSet(charset);
  }
  if (NS_FAILED(result))
    return result;

  // Get an encoder for the character set
  result = ccm->GetUnicodeEncoder(&charset, getter_AddRefs(encoder));
  if (NS_FAILED(result))
    return result;

  result = encoder->Reset();
  if (NS_FAILED(result))
    return result;

  PRInt32 maxByteLen, srcLen;
  srcLen = aStr.Length();

  nsPromiseFlatString flatSrc(aStr);
  const PRUnichar* src = flatSrc.get();

  // Get the expected length of result string
  result = encoder->GetMaxLength(src, srcLen, &maxByteLen);
  if (NS_FAILED(result))
    return result;

  // Allocate a buffer of the maximum length
  char* dest = (char *) nsMemory::Alloc(maxByteLen + 1);
  PRInt32 destLen2, destLen = maxByteLen;
  if (!dest)
    return NS_ERROR_OUT_OF_MEMORY;
  
  // Convert from unicode to the character set
  result = encoder->Convert(src, &srcLen, dest, &destLen);
  if (NS_FAILED(result)) {
    nsMemory::Free(dest);
    return result;
  }

  // Allow the encoder to finish the conversion
  destLen2 = maxByteLen - destLen;
  encoder->Finish(dest + destLen, &destLen2);
  dest[destLen + destLen2] = '\0';

  // Escape the string
  char *outBuf =
    nsEscape(dest, nsEscapeMask(url_XAlphas | url_XPAlphas | url_Path));
  CopyASCIItoUCS2(nsLiteralCString(outBuf), aReturn);

  nsMemory::Free(outBuf);
  nsMemory::Free(dest);

  return result;
}

NS_IMETHODIMP GlobalWindowImpl::Unescape(const nsAReadableString& aStr,
                                         nsAWritableString& aReturn)
{
  nsresult result = NS_OK;
  nsCOMPtr<nsIUnicodeDecoder> decoder;
  nsAutoString charset;

  nsCOMPtr<nsICharsetConverterManager>
    ccm(do_GetService(kCharsetConverterManagerCID));
  NS_ENSURE_TRUE(ccm, NS_ERROR_FAILURE);

  // Get the document character set
  charset.AssignWithConversion("UTF-8");        // default to utf-8
  if (mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));

    if (doc)
      result = doc->GetDocumentCharacterSet(charset);
  }
  if (NS_FAILED(result))
    return result;

  // Get an decoder for the character set
  result = ccm->GetUnicodeDecoder(&charset, getter_AddRefs(decoder));
  if (NS_FAILED(result))
    return result;

  result = decoder->Reset();
  if (NS_FAILED(result))
    return result;

  // Need to copy to do the two-byte to one-byte deflation
  char *inBuf = ToNewCString(aStr );
  if (!inBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  // Unescape the string
  char *src = nsUnescape(inBuf);

  PRInt32 maxLength, srcLen;
  srcLen = nsCRT::strlen(src);

  // Get the expected length of the result string
  result = decoder->GetMaxLength(src, srcLen, &maxLength);
  if (NS_FAILED(result)) {
    nsMemory::Free(src);
    return result;
  }

  // Allocate a buffer of the maximum length
  PRUnichar *dest = (PRUnichar*) nsMemory::Alloc(sizeof(PRUnichar) * maxLength);
  PRInt32 destLen = maxLength;
  if (!dest) {
    nsMemory::Free(src);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Convert from character set to unicode
  result = decoder->Convert(src, &srcLen, dest, &destLen);
  nsMemory::Free(src);
  if (NS_FAILED(result)) {
    nsMemory::Free(dest);
    return result;
  }

  aReturn.Assign(dest, destLen);
  nsMemory::Free(dest);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetSelection(nsISelection** aSelection)
{
  NS_ENSURE_ARG_POINTER(aSelection);
  *aSelection = nsnull;

  if (!mDocShell)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));

  if (!presShell)
    return NS_OK;

  nsCOMPtr<nsIFrameSelection> selection;
  presShell->GetFrameSelection(getter_AddRefs(selection));

  if (!selection)
    return NS_OK;

  return selection->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                 aSelection);
}


//*****************************************************************************
// GlobalWindowImpl::nsIJSScriptObject
//*****************************************************************************

PRBool GlobalWindowImpl::AddProperty(JSContext* aContext, JSObject* aObj,
                                     jsval aID, jsval* aVp)
{
  return PR_TRUE;
}

PRBool GlobalWindowImpl::DeleteProperty(JSContext* aContext, JSObject* aObj,
                                        jsval aID, jsval* aVp)
{
  return PR_TRUE;
}

PRBool GlobalWindowImpl::GetProperty(JSContext* aContext, JSObject* aObj,
                                     jsval aID, jsval* aVp)
{
  return PR_TRUE;
}

PRBool GlobalWindowImpl::SetProperty(JSContext* aContext, JSObject* aObj,
                                     jsval aID, jsval* aVp)
{
  return PR_TRUE;
}

PRBool GlobalWindowImpl::EnumerateProperty(JSContext* aContext, JSObject* aObj)
{
  return ::JS_EnumerateStandardClasses(aContext, aObj);
}

PRBool GlobalWindowImpl::Resolve(JSContext* aContext, JSObject* aObj,
                                 jsval aID, PRBool* aDidDefineProperty)
{
  *aDidDefineProperty = PR_FALSE;

  if (JSVAL_IS_STRING(aID)) {
    JSBool resolved;
    JSString *str;

    if (!::JS_ResolveStandardClass(aContext, aObj, aID, &resolved))
      return PR_FALSE;

    if (resolved) {
      *aDidDefineProperty = PR_TRUE;

      return PR_TRUE;
    }

    str = JSVAL_TO_STRING(aID);
    if (mDocShell) {
      nsCOMPtr<nsIDocShellTreeNode>
        docShellAsNode(do_QueryInterface(mDocShell));
      PRInt32 count;

      if (NS_SUCCEEDED(docShellAsNode->GetChildCount(&count)) && count) {
        nsCOMPtr<nsIDocShellTreeItem> child;

        const jschar *chars = ::JS_GetStringChars(str);
        const PRUnichar *unichars = NS_REINTERPRET_CAST(const PRUnichar*, chars);
        nsresult rv =
          docShellAsNode->FindChildWithName(unichars,
                                            PR_FALSE,
                                            PR_FALSE,
                                            nsnull,
                                            getter_AddRefs(child));
        if (NS_SUCCEEDED(rv)) {
          if (child) {
            JSObject *childObj = nsnull;

            // We found a subframe of the right name.  The rest of this code
            // is to get its script object.
            nsCOMPtr<nsIScriptGlobalObject>
              childGlobalObject(do_GetInterface(child));
            if (childGlobalObject) {
              //              nsCOMPtr<nsIScriptObjectOwner>
              //                objOwner(do_QueryInterface(childGlobalObject));
              //              if (objOwner) {
              //                nsCOMPtr<nsIScriptContext> scriptContext;
              //
              //                childGlobalObject->GetContext(getter_AddRefs(scriptContext));
              //                if (scriptContext) {
              //                  objOwner->GetScriptObject(scriptContext,
              //                                            (void **) &childObj);
              //                }
              //              }
            }
            // Okay, if we now have a childObj, we can define it and proceed.
            if (childObj) {
              if (!::JS_DefineUCProperty(aContext, mScriptObject,
                                         chars, ::JS_GetStringLength(str),
                                         OBJECT_TO_JSVAL(childObj), nsnull,
                                         nsnull, 0)) {
                return PR_FALSE;
              }

              *aDidDefineProperty = PR_TRUE;
            }
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool GlobalWindowImpl::Convert(JSContext* aContext, JSObject* aObj, jsval aID)
{
  return PR_TRUE;
}

void GlobalWindowImpl::Finalize(JSContext* aContext, JSObject* aObj)
{
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMEventTarget
//*****************************************************************************

NS_IMETHODIMP
GlobalWindowImpl::AddEventListener(const nsAReadableString& aType, 
                                   nsIDOMEventListener* aListener,
                                   PRBool aUseCapture)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  if (NS_SUCCEEDED(GetListenerManager(getter_AddRefs(manager)))) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GlobalWindowImpl::RemoveEventListener(const nsAReadableString& aType,
                                      nsIDOMEventListener* aListener,
                                      PRBool aUseCapture)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::DispatchEvent(nsIDOMEvent* aEvent)
{
  if (mDocument) {
    nsCOMPtr<nsIDocument> idoc(do_QueryInterface(mDocument));
    if (idoc) {
      // Obtain a presentation context
      PRInt32 count = idoc->GetNumberOfShells();
      if (count == 0)
        return NS_OK;

      nsCOMPtr<nsIPresShell> shell = getter_AddRefs(idoc->GetShellAt(0));

      // Retrieve the context
      nsCOMPtr<nsIPresContext> aPresContext;
      shell->GetPresContext(getter_AddRefs(aPresContext));

      nsCOMPtr<nsIEventStateManager> esm;
      if (NS_SUCCEEDED(aPresContext->GetEventStateManager(getter_AddRefs(esm)))) {
        return esm->DispatchNewEvent(NS_STATIC_CAST(nsIScriptGlobalObject *, this),
                                     aEvent);
      }
    }
  }
  return NS_ERROR_FAILURE;
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMEventReceiver
//*****************************************************************************

NS_IMETHODIMP
GlobalWindowImpl::AddEventListenerByIID(nsIDOMEventListener* aListener,
                                        const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  if (NS_OK == GetListenerManager(getter_AddRefs(manager))) {
    manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GlobalWindowImpl::RemoveEventListenerByIID(nsIDOMEventListener* aListener,
                                           const nsIID& aIID)
{
  if (mListenerManager) {
    mListenerManager->RemoveEventListenerByIID(aListener, aIID,
                                               NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GlobalWindowImpl::GetListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  if (mListenerManager)
    return CallQueryInterface(mListenerManager, aInstancePtrResult);

  // This is gonna get ugly.  Can't use NS_NewEventListenerManager because of
  // a circular link problem.
  nsCOMPtr<nsIDOMEventReceiver> doc(do_QueryInterface(mDocument));

  if (doc) {
    if (NS_OK == doc->GetNewListenerManager(aInstancePtrResult)) {
      mListenerManager = *aInstancePtrResult;
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

//XXX I need another way around the circular link problem.
NS_IMETHODIMP
GlobalWindowImpl::GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP GlobalWindowImpl::HandleEvent(nsIDOMEvent *aEvent)
{
  return DispatchEvent(aEvent);
}

//*****************************************************************************
// GlobalWindowImpl::nsPIDOMWindow
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::GetPrivateParent(nsPIDOMWindow ** aParent)
{
  nsCOMPtr<nsIDOMWindow> parent;
  *aParent = nsnull;            // Set to null so we can bail out later

  GetParent(getter_AddRefs(parent));

  if (NS_STATIC_CAST(nsIDOMWindow *, this) == parent.get()) {
    nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
    if (!chromeElement)
      return NS_OK;             // This is ok, just means a null parent.

    nsCOMPtr<nsIDocument> doc;
    chromeElement->GetDocument(*getter_AddRefs(doc));
    if (!doc)
      return NS_OK;             // This is ok, just means a null parent.

    nsCOMPtr<nsIScriptGlobalObject> globalObject;
    doc->GetScriptGlobalObject(getter_AddRefs(globalObject));
    if (!globalObject)
      return NS_OK;             // This is ok, just means a null parent.

    parent = do_QueryInterface(globalObject);
  }

  if (parent)
    CallQueryInterface(parent.get(), aParent);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetPrivateRoot(nsIDOMWindowInternal ** aParent)
{
  *aParent = nsnull;            // Set to null so we can bail out later

  nsCOMPtr<nsIDOMWindow> parent;
  GetTop(getter_AddRefs(parent));

  nsCOMPtr<nsIScriptGlobalObject> parentTop = do_QueryInterface(parent);
  nsCOMPtr<nsIDocShell> docShell;
  NS_ASSERTION(parentTop, "cannot get parentTop");
  if(parentTop == nsnull)
    return NS_ERROR_FAILURE;
  parentTop->GetDocShell(getter_AddRefs(docShell));

  // Get the chrome event handler from the doc shell, since we only
  // want to deal with XUL chrome handlers and not the new kind of
  // window root handler.
  nsCOMPtr<nsIChromeEventHandler> chromeEventHandler;
  docShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));

  nsCOMPtr<nsIContent> chromeElement(do_QueryInterface(mChromeEventHandler));
  if (chromeElement) {
    nsCOMPtr<nsIDocument> doc;
    chromeElement->GetDocument(*getter_AddRefs(doc));
    if (doc) {
      nsCOMPtr<nsIScriptGlobalObject> globalObject;
      doc->GetScriptGlobalObject(getter_AddRefs(globalObject));

      parent = do_QueryInterface(globalObject);
      nsCOMPtr<nsIDOMWindow> tempParent;
      parent->GetTop(getter_AddRefs(tempParent));
      CallQueryInterface(tempParent, aParent);
      return NS_OK;
    }
  }

  if (parent) {
    CallQueryInterface(parent, aParent);
  }

  return NS_OK;
}


NS_IMETHODIMP GlobalWindowImpl::GetLocation(nsIDOMLocation ** aLocation)
{
  if (!mLocation && mDocShell) {
    mLocation = new LocationImpl(mDocShell);
    NS_IF_ADDREF(mLocation);
  }

  *aLocation = mLocation;
  NS_IF_ADDREF(mLocation);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::GetObjectProperty(const PRUnichar *aProperty,
                                                  nsISupports ** aObject)
{
  // Get JSContext from stack.
  nsCOMPtr<nsIThreadJSContextStack>
    stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
  NS_ENSURE_TRUE(stack, NS_ERROR_FAILURE);

  JSContext *cx;
  NS_ENSURE_SUCCESS(stack->Peek(&cx), NS_ERROR_FAILURE);

  if (!cx) {
    stack->GetSafeJSContext(&cx);
    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);
  }

  jsval propertyVal;

  if (!::JS_LookupUCProperty(cx, mScriptObject,
                             NS_REINTERPRET_CAST(const jschar *, aProperty),
                             nsCRT::strlen(aProperty), &propertyVal)) {
    return NS_ERROR_FAILURE;
  }

  if (!nsJSUtils::nsConvertJSValToXPCObject(aObject, NS_GET_IID(nsISupports),
                                            cx, propertyVal)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Activate()
{
/*
   nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
   GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
   if(treeOwnerAsWin)
      treeOwnerAsWin->SetVisibility(PR_TRUE);

   nsCOMPtr<nsIPresShell> presShell;
   mDocShell->GetPresShell(getter_AddRefs(presShell));
   NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

   nsCOMPtr<nsIViewManager> vm;
   presShell->GetViewManager(getter_AddRefs(vm));
   NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

   nsIView* rootView;
   vm->GetRootView(rootView);
   NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);
   
   nsCOMPtr<nsIWidget> widget;
   rootView->GetWidget(*getter_AddRefs(widget));
   NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

   return widget->SetFocus();
   
 */
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  if (treeOwnerAsWin)
    treeOwnerAsWin->SetVisibility(PR_TRUE);

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIViewManager> vm;
  presShell->GetViewManager(getter_AddRefs(vm));
  NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

  nsIView *rootView;
  vm->GetRootView(rootView);
  NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);

  nsCOMPtr<nsIWidget> widget;
  rootView->GetWidget(*getter_AddRefs(widget));
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  nsEventStatus status;
  nsGUIEvent guiEvent;

  guiEvent.eventStructType = NS_GUI_EVENT;
  guiEvent.point.x = 0;
  guiEvent.point.y = 0;
  guiEvent.time = PR_IntervalNow();
  guiEvent.nativeMsg = nsnull;
  guiEvent.message = NS_ACTIVATE;
  guiEvent.widget = widget;

  vm->DispatchEvent(&guiEvent, &status);

  return NS_OK;
}

NS_IMETHODIMP GlobalWindowImpl::Deactivate()
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  if (treeOwnerAsWin)
    treeOwnerAsWin->SetVisibility(PR_TRUE);

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIViewManager> vm;
  presShell->GetViewManager(getter_AddRefs(vm));
  NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

  nsIView *rootView;
  vm->GetRootView(rootView);
  NS_ENSURE_TRUE(rootView, NS_ERROR_FAILURE);

  nsCOMPtr<nsIWidget> widget;
  rootView->GetWidget(*getter_AddRefs(widget));
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  nsEventStatus status;
  nsGUIEvent guiEvent;

  guiEvent.eventStructType = NS_GUI_EVENT;
  guiEvent.point.x = 0;
  guiEvent.point.y = 0;
  guiEvent.time = PR_IntervalNow();
  guiEvent.nativeMsg = nsnull;
  guiEvent.message = NS_DEACTIVATE;
  guiEvent.widget = widget;

  vm->DispatchEvent(&guiEvent, &status);

  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetChromeEventHandler(nsIChromeEventHandler** aHandler)
{
  *aHandler = mChromeEventHandler;
  NS_IF_ADDREF(*aHandler);
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::GetRootFocusController(nsIFocusController** aController)
{
  *aController = nsnull;

  nsCOMPtr<nsIDOMWindowInternal> rootWindow;
  GetPrivateRoot(getter_AddRefs(rootWindow));
  if (rootWindow) {
    // Obtain the chrome event handler.
    nsCOMPtr<nsIChromeEventHandler> chromeHandler;
    nsCOMPtr<nsPIDOMWindow> piWin(do_QueryInterface(rootWindow));
    piWin->GetChromeEventHandler(getter_AddRefs(chromeHandler));
    if (chromeHandler) {
      nsCOMPtr<nsPIWindowRoot> windowRoot(do_QueryInterface(chromeHandler));
      if (windowRoot) {
        windowRoot->GetFocusController(getter_AddRefs(aController));
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::HasMutationListeners(PRUint32 aMutationEventType, PRBool* aResult)
{
  *aResult = (mMutationBits & aMutationEventType) != 0;
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetMutationListeners(PRUint32 aType)
{
  mMutationBits |= aType;
  return NS_OK;
}

NS_IMETHODIMP
GlobalWindowImpl::SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx,
                                     PRInt32 cy, PRBool fRepaint)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  return treeOwnerAsWin->SetPositionAndSize(x, y, cx, cy, fRepaint);
}

NS_IMETHODIMP
GlobalWindowImpl::GetPositionAndSize(PRInt32 *x, PRInt32 *y, PRInt32 *cx,
                                     PRInt32 *cy)
{
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin;
  GetTreeOwner(getter_AddRefs(treeOwnerAsWin));
  NS_ENSURE_TRUE(treeOwnerAsWin, NS_ERROR_FAILURE);

  return treeOwnerAsWin->GetPositionAndSize(x, y, cx, cy);
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMViewCSS
//*****************************************************************************

NS_IMETHODIMP
GlobalWindowImpl::GetComputedStyle(nsIDOMElement* aElt,
                                   const nsAReadableString& aPseudoElt,
                                   nsIDOMCSSStyleDeclaration** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  NS_ENSURE_ARG_POINTER(aElt);
  *aReturn = nsnull;

  NS_ENSURE_TRUE(mDocShell, NS_OK);

  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_TRUE(presShell, NS_OK);

  nsresult rv = NS_OK;
  nsCOMPtr<nsIComputedDOMStyle> compStyle;

  compStyle =
    do_CreateInstance("@mozilla.org/DOM/Level2/CSS/computedStyleDeclaration;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = compStyle->Init(aElt, aPseudoElt, presShell);
  NS_ENSURE_SUCCESS(rv, rv);

  return compStyle->QueryInterface(NS_GET_IID(nsIDOMCSSStyleDeclaration),
                                   (void **) aReturn);
}

//*****************************************************************************
// GlobalWindowImpl::nsIDOMAbstractView
//*****************************************************************************

NS_IMETHODIMP GlobalWindowImpl::GetDocument(nsIDOMDocumentView ** aDocumentView)
{
  NS_ENSURE_ARG_POINTER(aDocumentView);

  nsresult rv = NS_OK;

  if (mDocument) {
    rv = mDocument->QueryInterface(NS_GET_IID(nsIDOMDocumentView),
                                   (void **) aDocumentView);
  }
  else {
    *aDocumentView = nsnull;
  }

  return rv;
}

//*****************************************************************************
// GlobalWindowImpl: Window Control Functions
//*****************************************************************************

NS_IMETHODIMP
GlobalWindowImpl::OpenInternal(const nsAReadableString& aUrl,
                               const nsAReadableString& aName,
                               const nsAReadableString& aOptions,
                               PRBool aDialog, jsval *argv, PRUint32 argc,
                               nsISupports *aExtraArgument,
                               nsIDOMWindow **aReturn)
{
  nsCOMPtr<nsIDOMWindow> domReturn;
  char *features = nsnull;
  char *name = nsnull;
  char *url = nsnull;
  nsresult rv;

  *aReturn = nsnull;
  rv = NS_ERROR_FAILURE;

  if (!aUrl.IsEmpty()) {
    // fix bug 35076
    // if the URL contains non ASCII, escape from the first non ASCII char
    nsAutoString unescapedURL(aUrl);
    nsAutoString escapedURL;

    if (unescapedURL.IsASCII())
      escapedURL = unescapedURL;
    else {
      // const PRUnichar *pt = unescapedURL.GetUnicode();
      PRUint32 len = unescapedURL.Length();
      PRUint32 nonAsciiPos;
      for (nonAsciiPos = 0; nonAsciiPos < len; nonAsciiPos++)
        if ((0xFF80 & aUrl.CharAt(nonAsciiPos)) != 0)
          break;

      nsAutoString right, escapedRight;
      unescapedURL.Left(escapedURL, nonAsciiPos);
      unescapedURL.Right(right, len - nonAsciiPos);
      if (NS_SUCCEEDED(Escape(right, escapedRight)))
        escapedURL.Append(escapedRight);
      else
        escapedURL = unescapedURL;
    }

    if (!escapedURL.IsEmpty())
      url = ToNewCString(escapedURL);
  }

  if (!aName.IsEmpty()) {
    name = ToNewUTF8String(aName);
  }

  if (1 && !aOptions.IsEmpty() /* IsNullDOMString(aOptions) */) {
    features = ToNewUTF8String(aOptions);
  }

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(sWindowWatcherContractID));
  if (wwatch) {
    if (argc) {
      nsCOMPtr<nsPIWindowWatcher> pwwatch(do_QueryInterface(wwatch));
      NS_ENSURE_TRUE(pwwatch, NS_ERROR_UNEXPECTED);

      PRUint32 extraArgc = argc >= 3 ? argc-3 : 0;
      rv = pwwatch->OpenWindowJS(this, url, name, features, aDialog,
                                 extraArgc, argv+3,
                                 getter_AddRefs(domReturn));
    } else {
      rv = wwatch->OpenWindow(this, url, name, features, aExtraArgument,
                              getter_AddRefs(domReturn));
    }

    if (domReturn)
      CallQueryInterface(domReturn, aReturn);
  }

  nsMemory::Free(url);
  return rv;

}

void GlobalWindowImpl::CloseWindow(nsISupports *aWindow)
{
  nsCOMPtr<nsIDOMWindowInternal> win(do_QueryInterface(aWindow));

  win->Close();
}

//*****************************************************************************
// GlobalWindowImpl: Timeout Functions
//*****************************************************************************

static const char *kSetIntervalStr = "setInterval";
static const char *kSetTimeoutStr = "setTimeout";

nsresult
GlobalWindowImpl::SetTimeoutOrInterval(PRBool aIsInterval, PRInt32 *aReturn)
{
  NS_ENSURE_STATE(sXPConnect);

  JSContext *cx = GetCurrentContext();

  if (!cx) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv = NS_OK;
  nsCOMPtr<nsIXPCNativeCallContext> ncc;

  rv = sXPConnect->GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  JSString *expr = nsnull;
  JSObject *funobj = nsnull;
  nsTimeoutImpl *timeout;
  jsdouble interval;
  PRInt64 now, delta;

  if (argc < 2) {
    ::JS_ReportError(cx, "Function %s requires at least 2 parameters",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_FAILURE;
  }

  if (!::JS_ValueToNumber(cx, argv[1], &interval)) {
    ::JS_ReportError(cx,
                     "Second argument to %s must be a millisecond interval",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  switch (::JS_TypeOfValue(cx, argv[0])) {
  case JSTYPE_FUNCTION:
    funobj = JSVAL_TO_OBJECT(argv[0]);
    break;

  case JSTYPE_STRING:
  case JSTYPE_OBJECT:
    if (!(expr = ::JS_ValueToString(cx, argv[0])))
      return NS_ERROR_FAILURE;
    if (!expr)
      return NS_ERROR_OUT_OF_MEMORY;
    argv[0] = STRING_TO_JSVAL(expr);
    break;

  default:
    ::JS_ReportError(cx, "useless %s call (missing quotes around argument?)",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_FAILURE;
  }

  timeout = PR_NEWZAP(nsTimeoutImpl);
  if (!timeout)
    return NS_ERROR_OUT_OF_MEMORY;

  // Initial ref_count to indicate that this timeout struct will
  // be held as the closure of a timer.
  timeout->ref_count = 1;
  if (aIsInterval)
    timeout->interval = (PRInt32) interval;
  if (expr) {
    if (!::JS_AddNamedRoot(cx, &timeout->expr, "timeout.expr")) {
      PR_DELETE(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    timeout->expr = expr;
  }
  else if (funobj) {
    int i;

    /* Leave an extra slot for a secret final argument that
       indicates to the called function how "late" the timeout is. */
    timeout->argv = (jsval *) PR_MALLOC((argc - 1) * sizeof(jsval));
    if (!timeout->argv) {
      DropTimeout(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!::JS_AddNamedRoot(cx, &timeout->funobj, "timeout.funobj")) {
      DropTimeout(timeout);
      return NS_ERROR_FAILURE;
    }
    timeout->funobj = funobj;

    timeout->argc = 0;
    for (i = 2; (PRUint32) i < argc; i++) {
      timeout->argv[i - 2] = argv[i];
      if (!::JS_AddNamedRoot(cx, &timeout->argv[i - 2], "timeout.argv[i]")) {
        DropTimeout(timeout);
        return NS_ERROR_FAILURE;
      }
      timeout->argc++;
    }
  }

  const char *filename;
  if (nsJSUtils::nsGetCallingLocation(cx, &filename, &timeout->lineno)) {
    timeout->filename = PL_strdup(filename);
    if (!timeout->filename) {
      DropTimeout(timeout);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  timeout->version = ::JS_VersionToString(::JS_GetVersion(cx));

  // Get principal of currently executing code, save for execution of timeout
  nsCOMPtr<nsIScriptSecurityManager>
    securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  if (NS_FAILED(securityManager->GetSubjectPrincipal(&timeout->principal)))
    return NS_ERROR_FAILURE;

  LL_I2L(now, PR_IntervalNow());
  LL_D2L(delta, PR_MillisecondsToInterval((PRUint32) interval));
  LL_ADD(timeout->when, now, delta);
  nsresult err;
  timeout->timer = do_CreateInstance("@mozilla.org/timer;1", &err);
  if (NS_OK != err) {
    DropTimeout(timeout);
    return err;
  }

  err = timeout->timer->Init(nsGlobalWindow_RunTimeout, timeout,
                             (PRInt32) interval, NS_PRIORITY_LOWEST);
  if (NS_OK != err) {
    DropTimeout(timeout);
    return err;
  }
  timeout->window = this;
  NS_ADDREF(this);

  InsertTimeoutIntoList(mTimeoutInsertionPoint, timeout);
  timeout->public_id = ++mTimeoutPublicIdCounter;
  *aReturn = timeout->public_id;

  return NS_OK;
}

PRBool GlobalWindowImpl::RunTimeout(nsTimeoutImpl *aTimeout)
{
  nsTimeoutImpl *next, *prev, *timeout;
  nsTimeoutImpl *last_expired_timeout, **last_insertion_point;
  nsTimeoutImpl dummy_timeout;
  JSContext *cx;
  PRInt64 now;
  nsCOMPtr<nsITimer> timer;
  nsresult rv;
  PRUint32 firingDepth = mTimeoutFiringDepth + 1;

  if (nsnull == mContext) {
    return PR_TRUE;
  }

  /* Make sure that the window or the script context don't go away as 
     a result of running timeouts */
  GlobalWindowImpl *temp = this;
  NS_ADDREF(temp);
  nsIScriptContext *tempContext = mContext;
  NS_ADDREF(tempContext);

  timer = aTimeout->timer;
  cx = (JSContext *) mContext->GetNativeContext();

  /* A native timer has gone off.  See which of our timeouts need
     servicing */
  LL_I2L(now, PR_IntervalNow());

  /* The timeout list is kept in deadline order.  Discover the
     latest timeout whose deadline has expired. On some platforms,
     native timeout events fire "early", so we need to test the
     timer as well as the deadline. */
  last_expired_timeout = nsnull;
  for (timeout = mTimeouts; timeout; timeout = timeout->next) {
    if (((timeout == aTimeout) || !LL_CMP(timeout->when, >, now)) &&
        (0 == timeout->firingDepth)) {
      /*
       * Mark any timeouts that are on the list to be fired with the
       * firing depth so that we can reentrantly run timeouts
       */
      timeout->firingDepth = firingDepth;
      last_expired_timeout = timeout;
    }
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
  dummy_timeout.firingDepth = firingDepth;
  dummy_timeout.next = last_expired_timeout->next;
  last_expired_timeout->next = &dummy_timeout;

  /* Don't let ClearWindowTimeouts throw away our stack-allocated
     dummy timeout. */
  dummy_timeout.ref_count = 2;

  last_insertion_point = mTimeoutInsertionPoint;
  mTimeoutInsertionPoint = &dummy_timeout.next;

  prev = nsnull;
  for (timeout = mTimeouts; timeout != &dummy_timeout; timeout = next) {
    next = timeout->next;

    /*
     * Check to see if it should fire at this depth. If it shouldn't, we'll 
     * ignore it 
     */
    if (timeout->firingDepth == firingDepth) {
      nsTimeoutImpl *last_running_timeout;

      /* Hold the timeout in case expr or funobj releases its doc. */
      HoldTimeout(timeout);
      last_running_timeout = mRunningTimeout;
      mRunningTimeout = timeout;
      ++mTimeoutFiringDepth;

      if (timeout->expr) {
        /* Evaluate the timeout expression. */
        nsAutoString script(NS_REINTERPRET_CAST(const PRUnichar *,
                                                ::JS_GetStringChars(timeout->expr)));
        nsAutoString blank;
        PRBool isUndefined;
        rv = mContext->EvaluateString(script, mScriptObject,
                                      timeout->principal,
                                      timeout->filename,
                                      timeout->lineno,
                                      timeout->version, blank, &isUndefined);
      }
      else {
        PRInt64 lateness64;
        PRInt32 lateness;

        /* Add "secret" final argument that indicates timeout
           lateness in milliseconds */
        LL_SUB(lateness64, now, timeout->when);
        LL_L2I(lateness, lateness64);
        lateness = PR_IntervalToMilliseconds(lateness);
        timeout->argv[timeout->argc] = INT_TO_JSVAL((jsint) lateness);
        PRBool aBoolResult;
        rv = mContext->CallEventHandler(mScriptObject, timeout->funobj,
                                        timeout->argc + 1, timeout->argv,
                                        &aBoolResult, PR_FALSE);
      }

      --mTimeoutFiringDepth;
      mRunningTimeout = last_running_timeout;

      if (NS_FAILED(rv)) {
        mTimeoutInsertionPoint = last_insertion_point;
        NS_RELEASE(temp);
        NS_RELEASE(tempContext);
        return PR_TRUE;
      }

      /* If the temporary reference is the only one that is keeping
         the timeout around, the document was released and we should
         restart this function. */
      if (timeout->ref_count == 1) {
        mTimeoutInsertionPoint = last_insertion_point;
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
        if (delay32 < 0)
          delay32 = 0;
        delay32 = PR_IntervalToMilliseconds(delay32);

        /* Reschedule timeout.  Account for possible error return in
           code below that checks for zero toid. */
        nsresult err;
        timeout->timer =
          do_CreateInstance("@mozilla.org/timer;1", &err);
        if (NS_OK != err) {
          mTimeoutInsertionPoint = last_insertion_point;
          NS_RELEASE(temp);
          NS_RELEASE(tempContext);
          return PR_TRUE;
        }

        err = timeout->timer->Init(nsGlobalWindow_RunTimeout, timeout,
                                   delay32, NS_PRIORITY_LOWEST);
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
      if (nsnull == prev) {
        mTimeouts = next;
      }
      else {
        prev->next = next;
      }

      PRBool isInterval = (timeout->interval && timeout->timer);

      // Drop timeout struct since it's out of the list
      DropTimeout(timeout, tempContext);

      /* Free the timeout if this is not a repeating interval
       *  timeout (or if it was an interval timeout, but we were
       *  unsuccessful at rescheduling it.)
       */
      if (isInterval) {
        /* Reschedule an interval timeout */
        /* Insert interval timeout onto list sorted in deadline order. */
        InsertTimeoutIntoList(mTimeoutInsertionPoint, timeout);
      }
    }
    else {
      /*
       * We skip the timeout since it's on the list to run at another
       * depth.
       */
      prev = timeout;
    }
  }

  /* Take the dummy timeout off the head of the list */
  if (nsnull == prev) {
    mTimeouts = dummy_timeout.next;
  }
  else {
    prev->next = dummy_timeout.next;
  }

  mTimeoutInsertionPoint = last_insertion_point;

  /* Get rid of our temporary references to ourselves and the script context */
  NS_RELEASE(temp);
  NS_RELEASE(tempContext);
  return PR_TRUE;
}

void GlobalWindowImpl::DropTimeout(nsTimeoutImpl *aTimeout,
                                   nsIScriptContext *aContext)
{
  JSRuntime *rt = nsnull;

  if (--aTimeout->ref_count > 0)
    return;

  if (!aContext)
    aContext = mContext;
  if (aContext) {
    JSContext *cx;
    cx = (JSContext *) aContext->GetNativeContext();
    rt = ::JS_GetRuntime(cx);
  } else {
    /* XXX The timeout *must* be unrooted, even if !aContext. This can be
       done without a JS context using the JSRuntime. This is safe enough,
       but it would be better to drop all a window's timeouts before its
       context is cleared. Bug 50705 describes a situation where we're not.
       In that case, at the time the context is cleared, a timeout (actually
       an Interval) is still active, but temporarily removed from the window's
       list of timers (placed instead on the timer manager's list). This makes
       the nearly handy ClearAllTimeouts routine useless, so we we settled on
       using the JSRuntime rather than relying on the window having a context.
       It would be good to remedy this workable but clumsy situation someday.
     */
    NS_WARNING("DropTimeout proceeding without context.");
    nsCOMPtr<nsIJSRuntimeService> rtsvc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    if (rtsvc)
      rtsvc->GetRuntime(&rt);
  }

  NS_ASSERTION(rt, "DropTimeout with no JSRuntime. eek!");
  if (!rt) // most unexpected. not much choice but to bail.
    return;

  if (aTimeout->expr)
    ::JS_RemoveRootRT(rt, &aTimeout->expr);
  else if (aTimeout->funobj) {
    ::JS_RemoveRootRT(rt, &aTimeout->funobj);
    if (aTimeout->argv) {
      int i;
      for (i = 0; i < aTimeout->argc; i++)
        ::JS_RemoveRootRT(rt, &aTimeout->argv[i]);
      PR_FREEIF(aTimeout->argv);
    }
  }

  if (aTimeout->timer) {
    aTimeout->timer->Cancel();
    aTimeout->timer = nsnull;
  }
  PR_FREEIF(aTimeout->filename);
  NS_IF_RELEASE(aTimeout->window);
  NS_IF_RELEASE(aTimeout->principal);
  PR_DELETE(aTimeout);
}

void GlobalWindowImpl::HoldTimeout(nsTimeoutImpl *aTimeout)
{
  aTimeout->ref_count++;
}

nsresult
GlobalWindowImpl::ClearTimeoutOrInterval(PRInt32 aTimerID)
{
  PRUint32 public_id;
  nsTimeoutImpl **top, *timeout;

  public_id = (PRUint32) aTimerID;
  if (!public_id) {             /* id of zero is reserved for internal use */
    /* return silently for compatibility (see bug 30700) */
    return NS_OK;
  }
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
          DropTimeout(timeout);
        }
        DropTimeout(timeout);
      }
      break;
    }
  }
  return NS_OK;
}

void GlobalWindowImpl::ClearAllTimeouts()
{
  nsTimeoutImpl *timeout, *next;

  for (timeout = mTimeouts; timeout; timeout = next) {
    /* If RunTimeout() is higher up on the stack for this
       window, e.g. as a result of document.write from a timeout,
       then we need to reset the list insertion point for
       newly-created timeouts in case the user adds a timeout,
       before we pop the stack back to RunTimeout. */
    if (mRunningTimeout == timeout)
      mTimeoutInsertionPoint = &mTimeouts;

    next = timeout->next;
    if (timeout->timer) {
      timeout->timer->Cancel();
      // Drop the count since the timer isn't going to hold on
      // anymore.
      DropTimeout(timeout);
    }
    // Drop the count since we're removing it from the list.
    DropTimeout(timeout);
  }
  mTimeouts = NULL;
}

void GlobalWindowImpl::InsertTimeoutIntoList(nsTimeoutImpl **aList,
                                             nsTimeoutImpl *aTimeout)
{
  nsTimeoutImpl *to;

  NS_ASSERTION(aList, "GlobalWindowImpl::InsertTimeoutIntoList null timeoutList");
  while ((to = *aList) != nsnull) {
    if (LL_CMP(to->when, >, aTimeout->when))
      break;
    aList = &to->next;
  }
  aTimeout->firingDepth = 0;
  aTimeout->next = to;
  *aList = aTimeout;
  // Increment the ref_count since we're in the list
  HoldTimeout(aTimeout);
}

void nsGlobalWindow_RunTimeout(nsITimer *aTimer, void *aClosure)
{
  nsTimeoutImpl *timeout = (nsTimeoutImpl *) aClosure;

  if (timeout->window->RunTimeout(timeout)) {
    // Drop the ref_count since the timer won't be holding on to the
    // timeout struct anymore
    timeout->window->DropTimeout(timeout);
  }
}

//*****************************************************************************
// GlobalWindowImpl: Helper Functions
//*****************************************************************************

nsresult
GlobalWindowImpl::GetTreeOwner(nsIDocShellTreeOwner **aTreeOwner)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  return docShellAsItem->GetTreeOwner(aTreeOwner);
}

nsresult
GlobalWindowImpl::GetTreeOwner(nsIBaseWindow **aTreeOwner)
{
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  docShellAsItem->GetTreeOwner(getter_AddRefs(treeOwner));
  if (!treeOwner) {
    *aTreeOwner = nsnull;
    return NS_OK;
  }

  return CallQueryInterface(treeOwner, aTreeOwner);
}

nsresult
GlobalWindowImpl::GetWebBrowserChrome(nsIWebBrowserChrome **aBrowserChrome)
{
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  GetTreeOwner(getter_AddRefs(treeOwner));

  nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(treeOwner));
  *aBrowserChrome = browserChrome;
  NS_IF_ADDREF(*aBrowserChrome);
  return NS_OK;
}

nsresult
GlobalWindowImpl::GetScrollInfo(nsIScrollableView **aScrollableView,
                                float *aP2T, float *aT2P)
{
  // Flush pending notifications so that the presentation is up to
  // date.
  FlushPendingNotifications();

  nsCOMPtr<nsIPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (presContext) {
    presContext->GetPixelsToTwips(aP2T);
    presContext->GetTwipsToPixels(aT2P);

    nsCOMPtr<nsIPresShell> presShell;
    presContext->GetShell(getter_AddRefs(presShell));
    if (presShell) {
      nsCOMPtr<nsIViewManager> vm;
      presShell->GetViewManager(getter_AddRefs(vm));
      if (vm)
        return vm->GetRootScrollableView(aScrollableView);
    }
  }
  return NS_OK;
}

void GlobalWindowImpl::FlushPendingNotifications()
{
  if (mDocument) {
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDocument));
    if (doc)
      doc->FlushPendingNotifications();
  }
}

//*****************************************************************************
// GlobalWindowImpl: Creator Function (This should go away)
//*****************************************************************************

nsresult NS_NewScriptGlobalObject(nsIScriptGlobalObject **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = nsnull;

  GlobalWindowImpl *global =
    new GlobalWindowImpl();
  NS_ENSURE_TRUE(global, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(NS_STATIC_CAST(nsIScriptGlobalObject *, global),
                            aResult);
}

//*****************************************************************************
//***    NavigatorImpl: Object Management
//*****************************************************************************

NavigatorImpl::NavigatorImpl(nsIDocShell *aDocShell) : 
  mMimeTypes(nsnull),                                                       
  mPlugins(nsnull),                                                       
  mDocShell(aDocShell)
{
  NS_INIT_REFCNT();
}

NavigatorImpl::~NavigatorImpl()
{
  NS_IF_RELEASE(mMimeTypes);
  NS_IF_RELEASE(mPlugins);
}

//*****************************************************************************
//    NavigatorImpl::nsISupports
//*****************************************************************************

NS_IMPL_ADDREF(NavigatorImpl)
NS_IMPL_RELEASE(NavigatorImpl)

NS_INTERFACE_MAP_BEGIN(NavigatorImpl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNavigator)
NS_INTERFACE_MAP_END

void NavigatorImpl::SetDocShell(nsIDocShell *aDocShell)
{
  mDocShell = aDocShell;
  if (mPlugins)
    mPlugins->SetDocShell(aDocShell);
}

//*****************************************************************************
//    NavigatorImpl::nsIDOMNavigator
//*****************************************************************************

NS_IMETHODIMP NavigatorImpl::GetUserAgent(nsAWritableString& aUserAgent)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *ua = nsnull;
    res = service->GetUserAgent(&ua);
    aUserAgent = ua;
    Recycle(ua);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetAppCodeName(nsAWritableString& aAppCodeName)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *appName = nsnull;
    res = service->GetAppName(&appName);
    aAppCodeName = appName;
    Recycle(appName);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetAppVersion(nsAWritableString& aAppVersion)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *str = nsnull;
    res = service->GetAppVersion(&str);
    aAppVersion = str;
    Recycle(str);

    aAppVersion.Append(NS_LITERAL_STRING(" (")); 
    res = service->GetPlatform(&str);
    if (NS_FAILED(res))
      return res;

    aAppVersion += str;
    Recycle(str);

    aAppVersion.Append(NS_LITERAL_STRING("; "));                      
    res = service->GetLanguage(&str);
    if (NS_FAILED(res))
      return res;

    aAppVersion += str;
    Recycle(str);

    aAppVersion.Append(PRUnichar(')'));
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetAppName(nsAWritableString& aAppName)
{
  aAppName.Assign(NS_LITERAL_STRING("Netscape"));
  return NS_OK;
}

NS_IMETHODIMP NavigatorImpl::GetLanguage(nsAWritableString& aLanguage)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *lang = nsnull;
    res = service->GetLanguage(&lang);
    aLanguage = lang;
    Recycle(lang);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetPlatform(nsAWritableString& aPlatform)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    // sorry for the #if platform ugliness, but Communicator is likewise hardcoded
    // and we're seeking backward compatibility here (bug 47080)
#if defined(WIN32)
    aPlatform = NS_LITERAL_STRING("Win32");
#elif defined(XP_MAC)
    // XXX not sure what to do about Mac OS X on non-PPC, but since Comm 4.x
    // doesn't know about it this will actually be backward compatible
    aPlatform = NS_LITERAL_STRING("MacPPC");
#else
    // XXX Communicator uses compiled-in build-time string defines
    // to indicate the platform it was compiled *for*, not what it is
    // currently running *on* which is what this does.
    PRUnichar *plat = nsnull;
    res = service->GetOscpu(&plat);
    aPlatform = plat;
    Recycle(plat);
#endif
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetOscpu(nsAWritableString& aOSCPU)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *oscpu = nsnull;
    res = service->GetOscpu(&oscpu);
    aOSCPU = oscpu;
    Recycle(oscpu);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetVendor(nsAWritableString& aVendor)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *vendor = nsnull;
    res = service->GetVendor(&vendor);
    aVendor = vendor;
    Recycle(vendor);
  }

  return res;
}


NS_IMETHODIMP NavigatorImpl::GetVendorSub(nsAWritableString& aVendorSub)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *vendor = nsnull;
    res = service->GetVendorSub(&vendor);
    aVendorSub = vendor;
    Recycle(vendor);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetProduct(nsAWritableString& aProduct)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *product = nsnull;
    res = service->GetProduct(&product);
    aProduct = product;
    Recycle(product);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetProductSub(nsAWritableString& aProductSub)
{
  nsresult res;
  nsCOMPtr<nsIHTTPProtocolHandler>
    service(do_GetService(kHTTPHandlerCID, &res));
  if (NS_SUCCEEDED(res) && (nsnull != service)) {
    PRUnichar *productSub = nsnull;
    res = service->GetProductSub(&productSub);
    aProductSub = productSub;
    Recycle(productSub);
  }

  return res;
}

NS_IMETHODIMP NavigatorImpl::GetSecurityPolicy(nsAWritableString& aSecurityPolicy)
{
  return NS_OK;
}

NS_IMETHODIMP NavigatorImpl::GetMimeTypes(nsIDOMMimeTypeArray **aMimeTypes)
{
  if (!mMimeTypes) {
    mMimeTypes = new MimeTypeArrayImpl(this);
    NS_IF_ADDREF(mMimeTypes);
  }

  *aMimeTypes = mMimeTypes;
  NS_IF_ADDREF(mMimeTypes);

  return NS_OK;
}

NS_IMETHODIMP NavigatorImpl::GetPlugins(nsIDOMPluginArray **aPlugins)
{
  if (!mPlugins) {
    mPlugins = new PluginArrayImpl(this, mDocShell);
    NS_IF_ADDREF(mPlugins);
  }

  *aPlugins = mPlugins;
  NS_IF_ADDREF(mPlugins);

  return NS_OK;
}

NS_IMETHODIMP NavigatorImpl::GetCookieEnabled(PRBool *aCookieEnabled)
{
  nsresult rv = NS_OK;
  *aCookieEnabled = PR_FALSE;

  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (NS_FAILED(rv) || prefs == nsnull)
    return rv;

  PRInt32 cookieBehaviorPref;
  rv = prefs->GetIntPref("network.cookie.cookieBehavior", &cookieBehaviorPref);

  if (NS_FAILED(rv))
    return rv;

  const PRInt32 DONT_USE = 2;
  *aCookieEnabled = (cookieBehaviorPref != DONT_USE);
  return rv;
}

NS_IMETHODIMP NavigatorImpl::JavaEnabled(PRBool *aReturn)
{
  nsresult rv = NS_OK;
  *aReturn = PR_FALSE;

  // determine whether user has enabled java.
  NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
  if (NS_FAILED(rv) || prefs == nsnull)
    return rv;

  // if pref doesn't exist, map result to false.
  if (prefs->GetBoolPref("security.enable_java", aReturn) != NS_OK)
    *aReturn = PR_FALSE;

  return rv;
}

NS_IMETHODIMP NavigatorImpl::TaintEnabled(PRBool *aReturn)
{
  *aReturn = PR_FALSE;
  return NS_OK;
}

#if 0
NS_IMETHODIMP NavigatorImpl::Preference(nsISupports** aReturn)
{
  nsresult result = NS_OK;

  *aReturn = JSVAL_NULL;

  JSObject *self = mScriptObject;
  if (!self)
    return NS_ERROR_FAILURE;

  NS_WITH_SERVICE(nsIScriptSecurityManager, secMan,
                  NS_SCRIPTSECURITYMANAGER_CONTRACTID, &result);
  if (NS_FAILED(result))
    return result;

  result =
    secMan->CheckScriptAccess(cx, self, NS_DOM_PROP_NAVIGATOR_PREFERENCE,
                              argc != 1);
  if (NS_FAILED(result)) {
    // Need to throw error here
    return NS_ERROR_FAILURE;
  }

  NS_WITH_SERVICE(nsIPref, pref, kPrefServiceCID, &result);
  if (NS_FAILED(result))
    return result;

  if (argc > 0) {
    JSString *str = ::JS_ValueToString(cx, argv[0]);
    if (str) {
      char *prefStr = ::JS_GetStringBytes(str);
      if (argc == 1) {
        PRInt32 prefType;
        pref->GetPrefType(prefStr, &prefType);
        switch (prefType & nsIPref::ePrefValuetypeMask) {
        case nsIPref::ePrefString:
          {
            char *prefCharVal;
            result = pref->CopyCharPref(prefStr, &prefCharVal);
            if (NS_SUCCEEDED(result)) {
              JSString *retStr = ::JS_NewStringCopyZ(cx, prefCharVal);
              if (retStr)
                *aReturn = STRING_TO_JSVAL(retStr);
              PL_strfree(prefCharVal);
            }
            break;
          }

        case nsIPref::ePrefInt:
          {
            PRInt32 prefIntVal;
            result = pref->GetIntPref(prefStr, &prefIntVal);
            if (NS_SUCCEEDED(result))
              *aReturn = INT_TO_JSVAL(prefIntVal);
            break;
          }

        case nsIPref::ePrefBool:
          {
            PRBool prefBoolVal;
            result = pref->GetBoolPref(prefStr, &prefBoolVal);
            if (NS_SUCCEEDED(result))
              *aReturn = BOOLEAN_TO_JSVAL(prefBoolVal);
            break;
          }
        }
      }
      else {
        if (JSVAL_IS_STRING(argv[1])) {
          JSString *valueJSStr = ::JS_ValueToString(cx, argv[1]);
          if (valueJSStr)
            result = pref->SetCharPref(prefStr, ::JS_GetStringBytes(valueJSStr));
        }
        else if (JSVAL_IS_INT(argv[1])) {
          jsint valueInt = JSVAL_TO_INT(argv[1]);
          result = pref->SetIntPref(prefStr, (PRInt32) valueInt);
        }
        else if (JSVAL_IS_BOOLEAN(argv[1])) {
          JSBool valueBool = JSVAL_TO_BOOLEAN(argv[1]);
          result = pref->SetBoolPref(prefStr, (PRBool) valueBool);
        }
        else if (JSVAL_IS_NULL(argv[1])) {
          result = pref->DeleteBranch(prefStr);
        }
      }
    }
  }

  return result;
}
#endif

#ifdef XP_MAC
#pragma mark -
#endif

//*****************************************************************************
//***  DOM Controller Stuff
//*****************************************************************************

#ifdef DOM_CONTROLLER
// nsDOMWindowController
const char *sCopyString = "cmd_copy";
const char *sCutString = "cmd_cut";
const char *sPasteString = "cmd_paste";
const char *sSelectAllString = "cmd_selectAll";

const char *sScrollTopString = "cmd_scrollTop";
const char *sScrollBottomString = "cmd_scrollBottom";
const char *sScrollPageUpString = "cmd_scrollPageUp";
const char *sScrollPageDownString = "cmd_scrollPageDown";
const char *sScrollLineUpString = "cmd_scrollLineUp";
const char *sScrollLineDownString = "cmd_scrollLineDown";
const char *sScrollLeftString = "cmd_scrollLeft";
const char *sScrollRightString = "cmd_scrollRight";

// These are so the browser can use editor navigation key bindings
// helps with accessibility (boolean pref accessibility.browsewithcaret)

const char *sSelectCharPreviousString = "cmd_selectCharPrevious";
const char *sSelectCharNextString = "cmd_selectCharNext";

const char *sWordPreviousString = "cmd_wordPrevious";
const char *sWordNextString = "cmd_wordNext";
const char *sSelectWordPreviousString = "cmd_selectWordPrevious";
const char *sSelectWordNextString = "cmd_selectWordNext";

const char *sBeginLineString = "cmd_beginLine";
const char *sEndLineString = "cmd_endLine";
const char *sSelectBeginLineString = "cmd_selectBeginLine";
const char *sSelectEndLineString = "cmd_selectEndLine";

const char *sSelectLinePreviousString = "cmd_selectLinePrevious";
const char *sSelectLineNextString = "cmd_selectLineNext";

const char *sSelectPagePreviousString = "cmd_selectPagePrevious";
const char *sSelectPageNextString = "cmd_selectPageNext";

const char *sSelectMoveTopString = "cmd_selectMoveTop";
const char *sSelectMoveBottomString = "cmd_selectMoveBottom";

NS_IMPL_ADDREF(nsDOMWindowController)
NS_IMPL_RELEASE(nsDOMWindowController)

NS_INTERFACE_MAP_BEGIN(nsDOMWindowController)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIController)
  NS_INTERFACE_MAP_ENTRY(nsIController)
NS_INTERFACE_MAP_END
//NS_IMPL_QUERY_INTERFACE1(nsDOMWindowController, nsIController)

nsDOMWindowController::nsDOMWindowController(nsIDOMWindowInternal *aWindow)
{
  NS_INIT_REFCNT();
  mWindow = aWindow;

  // Set browse with caret flag so we don't need to every time
  mBrowseWithCaret = PR_FALSE;
  nsCOMPtr<nsIPref> prefs(do_GetService(kPrefServiceCID));
  if (prefs)
    prefs->GetBoolPref("accessibility.browsewithcaret", &mBrowseWithCaret);
}

nsresult
nsDOMWindowController::GetEditInterface(nsIContentViewerEdit **aEditInterface)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(mWindow));
  NS_ENSURE_TRUE(sgo, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell;
  sgo->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContentViewer> viewer;
  docShell->GetContentViewer(getter_AddRefs(viewer));
  nsCOMPtr<nsIContentViewerEdit> edit(do_QueryInterface(viewer));
  NS_ENSURE_TRUE(edit, NS_ERROR_FAILURE);

  *aEditInterface = edit;
  NS_ADDREF(*aEditInterface);
  return NS_OK;
}

nsresult
nsDOMWindowController::GetPresShell(nsIPresShell **aPresShell)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(mWindow));
  NS_ENSURE_TRUE(sgo, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell;
  sgo->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(docShell->GetPresShell(aPresShell), NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult
nsDOMWindowController::GetSelectionController(nsISelectionController **aSelCon)
{
  nsCOMPtr<nsIPresShell> presShell;
  nsresult result = GetPresShell(getter_AddRefs(presShell));
  if (presShell && NS_SUCCEEDED(result)) {
    nsCOMPtr<nsISelectionController> selController =
      do_QueryInterface(presShell);
    if (selController) {
      *aSelCon = selController;
      NS_ADDREF(*aSelCon);
      return NS_OK;
    }
  }
  return NS_FAILED(result) ? result : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsDOMWindowController::IsCommandEnabled(const PRUnichar *aCommand,
                                                      PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIContentViewerEdit> editInterface;
  rv = GetEditInterface(getter_AddRefs(editInterface));
  if (NS_FAILED(rv))
    return rv;

  if (nsCAutoString(sCopyString).EqualsWithConversion(aCommand)) {
    rv = editInterface->GetCopyable(aResult);
  }
  else if (nsCAutoString(sCutString).EqualsWithConversion(aCommand)) {
    rv = editInterface->GetCutable(aResult);
  }
  else if (nsCAutoString(sPasteString).EqualsWithConversion(aCommand)) {
    rv = editInterface->GetPasteable(aResult);
  }
  else if (nsCAutoString(sSelectAllString).EqualsWithConversion(aCommand)) {
    *aResult = PR_TRUE;
    rv = NS_OK;
  }
  return rv;

}

NS_IMETHODIMP nsDOMWindowController::SupportsCommand(const PRUnichar *aCommand,
                                                     PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;

  if (nsCAutoString(sCopyString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectAllString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sCutString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sPasteString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollTopString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollBottomString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollPageUpString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollPageDownString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollLineUpString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollLineDownString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollLeftString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sScrollRightString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectCharPreviousString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectCharNextString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sWordPreviousString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sWordNextString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectWordPreviousString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectWordNextString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sBeginLineString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sEndLineString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectBeginLineString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectEndLineString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectLinePreviousString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectLineNextString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectPagePreviousString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectPageNextString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectMoveTopString).EqualsWithConversion(aCommand) ||
      nsCAutoString(sSelectMoveBottomString).EqualsWithConversion(aCommand)
      ) {
      *aResult=PR_TRUE;
      }

  return NS_OK;
}

NS_IMETHODIMP nsDOMWindowController::DoCommand(const PRUnichar *aCommand)
{
  NS_ENSURE_ARG_POINTER(aCommand);
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIContentViewerEdit> editInterface;
  rv = GetEditInterface(getter_AddRefs(editInterface));
  if (!editInterface)
    return rv;

  if (nsCAutoString(sCopyString).EqualsWithConversion(aCommand))
    return editInterface->CopySelection();
  if (nsCAutoString(sSelectAllString).EqualsWithConversion(aCommand))
    return editInterface->SelectAll();
  if (nsCAutoString(sCutString).EqualsWithConversion(aCommand))
    return editInterface->CutSelection();

  nsCOMPtr<nsISelectionController> selCont;
  rv = GetSelectionController(getter_AddRefs(selCont));
  if (!selCont)
    return rv;

  if (nsCAutoString(sPasteString).EqualsWithConversion(aCommand))
    rv = editInterface->Paste();
  else if (nsCAutoString(sScrollTopString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->CompleteMove(PR_FALSE, PR_FALSE): selCont->CompleteScroll(PR_FALSE));
  else if (nsCAutoString(sScrollBottomString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->CompleteMove(PR_TRUE, PR_FALSE): selCont->CompleteScroll(PR_TRUE));
  else if (nsCAutoString(sScrollPageUpString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->PageMove(PR_FALSE, PR_FALSE): selCont->ScrollPage(PR_FALSE));
  else if (nsCAutoString(sScrollPageDownString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->PageMove(PR_TRUE, PR_FALSE): selCont->ScrollPage(PR_TRUE));
  else if (nsCAutoString(sScrollLineUpString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->LineMove(PR_FALSE, PR_FALSE): selCont->ScrollLine(PR_FALSE));
  else if (nsCAutoString(sScrollLineDownString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->LineMove(PR_TRUE, PR_FALSE): selCont->ScrollLine(PR_TRUE));
  else if (nsCAutoString(sScrollLeftString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->CharacterMove(PR_FALSE, PR_FALSE): selCont->ScrollHorizontal(PR_TRUE));
  else if (nsCAutoString(sScrollRightString).EqualsWithConversion(aCommand))
    rv = (mBrowseWithCaret? selCont->CharacterMove(PR_TRUE, PR_FALSE): selCont->ScrollHorizontal(PR_FALSE));
  // These commands are so the browser can use editor navigation key bindings -
  // Helps with accessibility - aaronl@chorus.net
  else if (nsCAutoString(sSelectCharPreviousString).EqualsWithConversion(aCommand))
    rv = selCont->CharacterMove(PR_FALSE, PR_TRUE);
  else if (nsCAutoString(sSelectCharNextString).EqualsWithConversion(aCommand))
    rv = selCont->CharacterMove(PR_TRUE, PR_TRUE);
  else if (nsCAutoString(sWordPreviousString).EqualsWithConversion(aCommand))
    rv = selCont->WordMove(PR_FALSE, PR_FALSE);
  else if (nsCAutoString(sWordNextString).EqualsWithConversion(aCommand))
    rv = selCont->WordMove(PR_TRUE, PR_FALSE);
  else if (nsCAutoString(sSelectWordPreviousString).EqualsWithConversion(aCommand))
    rv = selCont->WordMove(PR_FALSE, PR_TRUE);
  else if (nsCAutoString(sSelectWordNextString).EqualsWithConversion(aCommand))
    rv = selCont->WordMove(PR_TRUE, PR_TRUE);
  else if (nsCAutoString(sBeginLineString).EqualsWithConversion(aCommand))
    rv = selCont->IntraLineMove(PR_FALSE, PR_FALSE);
  else if (nsCAutoString(sEndLineString).EqualsWithConversion(aCommand))
    rv = selCont->IntraLineMove(PR_TRUE, PR_FALSE);
  else if (nsCAutoString(sSelectBeginLineString).EqualsWithConversion(aCommand))
    rv = selCont->IntraLineMove(PR_FALSE, PR_TRUE);
  else if (nsCAutoString(sSelectEndLineString).EqualsWithConversion(aCommand))
    rv = selCont->IntraLineMove(PR_TRUE, PR_TRUE);
  else if (nsCAutoString(sSelectLinePreviousString).EqualsWithConversion(aCommand))
    rv = selCont->LineMove(PR_FALSE, PR_TRUE);
  else if (nsCAutoString(sSelectLineNextString).EqualsWithConversion(aCommand))
    rv = selCont->LineMove(PR_TRUE, PR_TRUE);
  else if (nsCAutoString(sSelectMoveTopString).EqualsWithConversion(aCommand))
    rv = selCont->CompleteMove(PR_FALSE, PR_TRUE);
  else if (nsCAutoString(sSelectMoveBottomString).EqualsWithConversion(aCommand))
    rv = selCont->CompleteMove(PR_TRUE, PR_TRUE);
  return rv;
}


NS_IMETHODIMP nsDOMWindowController::OnEvent(const PRUnichar *aEventName)
{
  return NS_OK;
}
#endif
