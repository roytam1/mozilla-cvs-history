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
#include "nsIWebShell.h"
#include "nsIDocumentLoader.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIDeviceContext.h"
#include "nsILinkHandler.h"
#include "nsIStreamListener.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContextOwner.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsDOMEvent.h"
#include "nsIPresContext.h"
#include "nsRepository.h"
#include "nsCRT.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "nsWidgetsCID.h"
#include "nsGfxCIID.h"
#include "plevent.h"
#include "nsIPluginHost.h"
#include "nsplugin.h"
#include "nsPluginsCID.h"
#include "nsIPref.h"
#include "nsIBrowserWindow.h"

#include "prlog.h"


//XXX for nsIPostData; this is wrong; we shouldn't see the nsIDocument type
#include "nsIDocument.h"

#ifdef NS_DEBUG
/**
 * Note: the log module is created during initialization which
 * means that you cannot perform logging before then.
 */
static PRLogModuleInfo* gLogModule = PR_NewLogModule("webwidget");
#endif

#define WEB_TRACE_CALLS        0x1
#define WEB_TRACE_HISTORY      0x2

#define WEB_LOG_TEST(_lm,_bit) (PRIntn((_lm)->level) & (_bit))

#ifdef NS_DEBUG
#define WEB_TRACE(_bit,_args)            \
  PR_BEGIN_MACRO                         \
    if (WEB_LOG_TEST(gLogModule,_bit)) { \
      PR_LogPrint _args;                 \
    }                                    \
  PR_END_MACRO
#else
#define WEB_TRACE(_bit,_args)
#endif


#if XP_UNIX
  // XXX. This should be changed. 
  // Allow the event queue to be setup from outside
  // the webshell.
PLEventQueue* gWebShell_UnixEventQueue;

void nsWebShell_SetUnixEventQueue(PLEventQueue* aEventQueue)
{
  gWebShell_UnixEventQueue = aEventQueue;
}
#endif

//----------------------------------------------------------------------

class nsWebShell : public nsIWebShell,
                   public nsIWebShellContainer,
                   public nsILinkHandler,
                   public nsIScriptContextOwner,
                   public nsIDocumentLoaderObserver
{
public:
  nsWebShell();
  virtual ~nsWebShell();

  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIContentViewerContainer
  NS_IMETHOD QueryCapability(const nsIID &aIID, void** aResult);
  NS_IMETHOD Embed(nsIContentViewer* aDocViewer, 
                   const char* aCommand,
                   nsISupports* aExtraInfo);

  // nsIWebShell
  NS_IMETHOD Init(nsNativeWidget aNativeParent,
                  const nsRect& aBounds,
                  nsScrollPreference aScrolling = nsScrollPreference_kAuto,
                  PRBool aAllowPlugins = PR_TRUE);
  NS_IMETHOD Destroy(void);
  NS_IMETHOD GetBounds(nsRect& aResult);
  NS_IMETHOD SetBounds(const nsRect& aBounds);
  NS_IMETHOD MoveTo(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Show();
  NS_IMETHOD Hide();
  NS_IMETHOD SetContentViewer(nsIContentViewer* aViewer);
  NS_IMETHOD GetContentViewer(nsIContentViewer*& aResult);
  NS_IMETHOD SetContainer(nsIWebShellContainer* aContainer);
  NS_IMETHOD GetContainer(nsIWebShellContainer*& aResult);
  NS_IMETHOD SetObserver(nsIStreamObserver* anObserver);
  NS_IMETHOD GetObserver(nsIStreamObserver*& aResult);
  NS_IMETHOD GetDocumentLoader(nsIDocumentLoader*& aResult);
  NS_IMETHOD SetPrefs(nsIPref* aPrefs);
  NS_IMETHOD GetPrefs(nsIPref*& aPrefs);
  NS_IMETHOD GetRootWebShell(nsIWebShell*& aResult);
  NS_IMETHOD SetParent(nsIWebShell* aParent);
  NS_IMETHOD GetParent(nsIWebShell*& aParent);
  NS_IMETHOD GetChildCount(PRInt32& aResult);
  NS_IMETHOD AddChild(nsIWebShell* aChild);
  NS_IMETHOD ChildAt(PRInt32 aIndex, nsIWebShell*& aResult);
  NS_IMETHOD GetName(nsString& aName);
  NS_IMETHOD SetName(const nsString& aName);
  NS_IMETHOD FindChildWithName(const nsString& aName,
                               nsIWebShell*& aResult);
  NS_IMETHOD Back(void);
  NS_IMETHOD Forward(void);
  NS_IMETHOD LoadURL(const nsString& aURLSpec,
                     nsIPostData* aPostData=nsnull);
  NS_IMETHOD GoTo(PRInt32 aHistoryIndex);
  NS_IMETHOD GetHistoryIndex(PRInt32& aResult);
  NS_IMETHOD GetURL(PRInt32 aHistoryIndex, nsString& aURLResult);
  NS_IMETHOD SetTitle(const nsString& aTitle);
  NS_IMETHOD GetTitle(nsString& aResult);

  NS_IMETHOD GetMarginWidth (PRInt32& aWidth);
  NS_IMETHOD SetMarginWidth (PRInt32  aWidth);
  NS_IMETHOD GetMarginHeight(PRInt32& aWidth);
  NS_IMETHOD SetMarginHeight(PRInt32  aHeight);

  // nsIWebShellContainer
  NS_IMETHOD WillLoadURL(nsIWebShell* aShell, const nsString& aURL);
  NS_IMETHOD BeginLoadURL(nsIWebShell* aShell, const nsString& aURL);
  NS_IMETHOD EndLoadURL(nsIWebShell* aShell, const nsString& aURL);

  // nsILinkHandler
  NS_IMETHOD OnLinkClick(nsIFrame* aFrame, 
                         const nsString& aURLSpec,
                         const nsString& aTargetSpec,
                         nsIPostData* aPostData = 0);
  NS_IMETHOD OnOverLink(nsIFrame* aFrame, 
                        const nsString& aURLSpec,
                        const nsString& aTargetSpec);
  NS_IMETHOD GetLinkState(const nsString& aURLSpec, nsLinkState& aState);

  // nsIScriptContextOwner
  NS_IMETHOD GetScriptContext(nsIScriptContext **aContext);
  NS_IMETHOD GetScriptGlobalObject(nsIScriptGlobalObject **aGlobal);
  NS_IMETHOD ReleaseScriptContext(nsIScriptContext *aContext);

  // nsIDocumentLoaderObserver
  NS_IMETHOD OnConnectionsComplete();

  // nsWebShell
  void HandleLinkClickEvent(const nsString& aURLSpec,
                            const nsString& aTargetSpec,
                            nsIPostData* aPostDat = 0);

  void ShowHistory();

  nsIWebShell* GetTarget(const nsString& aName);

  static nsEventStatus PR_CALLBACK HandleEvent(nsGUIEvent *aEvent);

  static nsresult CreatePluginHost(void);

protected:
  nsIScriptGlobalObject *mScriptGlobal;
  nsIScriptContext* mScriptContext;

  nsIWebShellContainer* mContainer;
  nsIContentViewer* mContentViewer;
  nsIDeviceContext* mDeviceContext;
  nsIPref* mPrefs;
  nsIWidget* mWindow;
  nsISupports* mInnerWindow;
  nsIDocumentLoader* mDocLoader;
  nsIStreamObserver* mObserver;

  nsIWebShell* mParent;
  nsVoidArray mChildren;
  nsString mName;

  nsVoidArray mHistory;
  PRInt32 mHistoryIndex;

  nsString mTitle;

  nsString mOverURL;
  nsString mOverTarget;

  nsScrollPreference mScrollPref;
  PRInt32 mMarginWidth;
  PRInt32 mMarginHeight;

  void ReleaseChildren();

  static nsIPluginHost    *mPluginHost;
  static nsIPluginManager *mPluginManager;
  static PRBool           mPluginInited;
};

//----------------------------------------------------------------------

// Class IID's
static NS_DEFINE_IID(kChildCID, NS_CHILD_CID);
static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
static NS_DEFINE_IID(kDocumentLoaderCID, NS_DOCUMENTLOADER_CID);
static NS_DEFINE_IID(kWebShellCID, NS_WEB_SHELL_CID);

// IID's
static NS_DEFINE_IID(kIContentViewerContainerIID,
                     NS_ICONTENT_VIEWER_CONTAINER_IID);
static NS_DEFINE_IID(kIDeviceContextIID, NS_IDEVICE_CONTEXT_IID);
static NS_DEFINE_IID(kIDocumentLoaderIID, NS_IDOCUMENTLOADER_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kIScriptContextOwnerIID, NS_ISCRIPTCONTEXTOWNER_IID);
static NS_DEFINE_IID(kIStreamObserverIID, NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);
static NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
static NS_DEFINE_IID(kIPluginHostIID, NS_IPLUGINHOST_IID);
static NS_DEFINE_IID(kCPluginHostCID, NS_PLUGIN_HOST_CID);
static NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
static NS_DEFINE_IID(kIDocumentLoaderObserverIID, NS_IDOCUMENT_LOADER_OBSERVER_IID);
static NS_DEFINE_IID(kIDocumentViewerIID, NS_IDOCUMENT_VIEWER_IID);

// XXX not sure
static NS_DEFINE_IID(kILinkHandlerIID, NS_ILINKHANDLER_IID);

nsIPluginHost *nsWebShell::mPluginHost = nsnull;
nsIPluginManager *nsWebShell::mPluginManager = nsnull;
PRBool nsWebShell::mPluginInited = PR_FALSE;

nsresult nsWebShell::CreatePluginHost(void)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (nsnull == mPluginManager)
  {
    rv = NSRepository::CreateInstance(kCPluginHostCID, nsnull,
                                      kIPluginManagerIID,
                                      (void**)&mPluginManager);
    if (NS_OK == rv)
    {
      if (NS_OK == mPluginManager->QueryInterface(kIPluginHostIID,
                                                  (void **)&mPluginHost))
      {
        mPluginHost->Init();
        mPluginHost->LoadPlugins();
      }
    }
  }

  return rv;
}

//----------------------------------------------------------------------

// Note: operator new zeros our memory
nsWebShell::nsWebShell()
{
  NS_INIT_REFCNT();
  mHistoryIndex = -1;
  mScrollPref = nsScrollPreference_kAuto;
  mScriptGlobal = nsnull;
  mScriptContext = nsnull;
  mMarginWidth  = -1;  
  mMarginHeight = -1;
}

nsWebShell::~nsWebShell()
{
  // Stop any pending document loads and destroy the loader...
  if (nsnull != mDocLoader) {
    mDocLoader->Stop();
    mDocLoader->RemoveObserver((nsIDocumentLoaderObserver*)this);
    NS_RELEASE(mDocLoader);
  }

  NS_IF_RELEASE(mInnerWindow);

  NS_IF_RELEASE(mContentViewer);
  NS_IF_RELEASE(mDeviceContext);
  NS_IF_RELEASE(mPrefs);
  NS_IF_RELEASE(mContainer);
  NS_IF_RELEASE(mObserver);

  if (nsnull != mScriptGlobal) {
    mScriptGlobal->SetWebShell(nsnull);
    NS_RELEASE(mScriptGlobal);
  }
  NS_IF_RELEASE(mScriptContext);

  // Release references on our children
  ReleaseChildren();

  // Free up history memory
  PRInt32 i, n = mHistory.Count();
  for (i = 0; i < n; i++) {
    nsString* s = (nsString*) mHistory.ElementAt(i);
    delete s;
  }
}

void
nsWebShell::ReleaseChildren()
{
  PRInt32 i, n = mChildren.Count();
  for (i = 0; i < n; i++) {
    nsIWebShell* shell = (nsIWebShell*) mChildren.ElementAt(i);
    shell->SetParent(nsnull);

    //Break circular reference of webshell to contentviewer
    shell->SetContentViewer(nsnull);
    NS_RELEASE(shell);
  }
  mChildren.Clear();
}

NS_IMPL_ADDREF(nsWebShell)
NS_IMPL_RELEASE(nsWebShell)

//XXX missing nsIWebShellContainer!!!
nsresult
nsWebShell::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIWebShellIID)) {
    *aInstancePtr = (void*)(nsIWebShell*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIContentViewerContainerIID)) {
    *aInstancePtr = (void*)(nsIContentViewerContainer*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptContextOwnerIID)) {
    *aInstancePtr = (void*)(nsIScriptContextOwner*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDocumentLoaderObserverIID)) {
    *aInstancePtr = (void*)(nsIDocumentLoaderObserver*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)(nsIWebShell*)this;
    AddRef();
    return NS_OK;
  }
  if (nsnull != mInnerWindow) {
    return mInnerWindow->QueryInterface(aIID, aInstancePtr);
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsWebShell::QueryCapability(const nsIID &aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(kILinkHandlerIID)) {
    *aInstancePtr = (void*) ((nsILinkHandler*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptContextOwnerIID)) {
    *aInstancePtr = (void*) ((nsIScriptContextOwner*)this);
    AddRef();
    return NS_OK;
  }

  //XXX this seems a little wrong. MMP
  if (nsnull != mPluginManager)
    return mPluginManager->QueryInterface(aIID, aInstancePtr);

  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsWebShell::Embed(nsIContentViewer* aContentViewer, 
                  const char* aCommand, 
                  nsISupports* aExtraInfo)
{
  nsresult rv;
  nsRect bounds;

  WEB_TRACE(WEB_TRACE_CALLS,
      ("nsWebShell::Embed: this=%p aDocViewer=%p aCommand=%s aExtraInfo=%p",
       this, aContentViewer, aCommand ? aCommand : "", aExtraInfo));

  NS_IF_RELEASE(mContentViewer);
  mContentViewer = aContentViewer;
  NS_ADDREF(aContentViewer);

  mWindow->GetBounds(bounds);
  bounds.x = bounds.y = 0;
  rv = mContentViewer->Init(mWindow->GetNativeData(NS_NATIVE_WIDGET), 
                            mDeviceContext, 
                            mPrefs,
                            bounds,
                            mScrollPref);
  if (NS_OK == rv) {
    mContentViewer->Show();
  }

  // Now that we have switched documents, forget all of our children
  ReleaseChildren();

  return rv;
}

NS_IMETHODIMP
nsWebShell::Init(nsNativeWidget aNativeParent,
                 const nsRect& aBounds,
                 nsScrollPreference aScrolling,
                 PRBool aAllowPlugins)
{
  //XXX make sure plugins have started up. this really needs to
  //be associated with the nsIContentViewerContainer interfaces,
  //not the nsIWebShell interfaces. this is a hack. MMP

  if ((PR_TRUE == aAllowPlugins) && (PR_FALSE == mPluginInited))
    CreatePluginHost();

  //never attempt to initialize plugins again...
  mPluginInited = PR_TRUE;

  mScrollPref = aScrolling;

  WEB_TRACE(WEB_TRACE_CALLS,
            ("nsWebShell::Init: this=%p", this));

  nsresult rv = NS_OK;

  // Initial error checking...
  NS_PRECONDITION(nsnull != aNativeParent, "null Parent Window");
  if (nsnull == aNativeParent) {
    rv = NS_ERROR_NULL_POINTER;
    goto done;
  }

  // Create a document loader...
  if (nsnull != mParent) {
    nsIDocumentLoader* parentLoader;

    // Create a child document loader...
    mParent->GetDocumentLoader(parentLoader);
    if (NS_OK == rv) {
      rv = parentLoader->CreateDocumentLoader(&mDocLoader);
      NS_RELEASE(parentLoader);
    }
  } else {
    rv = NSRepository::CreateInstance(kDocumentLoaderCID,
                                      nsnull,
                                      kIDocumentLoaderIID,
                                      (void**)&mDocLoader);
  }
  if (NS_OK != rv) {
    goto done;
  }
  //Register ourselves as an observer for the new doc loader
  mDocLoader->AddObserver((nsIDocumentLoaderObserver*)this);

  // Create device context
  rv = NSRepository::CreateInstance(kDeviceContextCID, nsnull,
                                    kIDeviceContextIID,
                                    (void **)&mDeviceContext);
  if (NS_OK != rv) {
    goto done;
  }
  mDeviceContext->Init(aNativeParent);
  mDeviceContext->SetDevUnitsToAppUnits(mDeviceContext->GetDevUnitsToTwips());
  mDeviceContext->SetAppUnitsToDevUnits(mDeviceContext->GetTwipsToDevUnits());
//  mDeviceContext->SetGamma(1.7f);
  mDeviceContext->SetGamma(1.0f);

  // Create a Native window for the shell container...
  rv = NSRepository::CreateInstance(kChildCID,
                                    (nsISupports*)((nsIWebShell*)this),
                                    kISupportsIID,
                                    (void**)&mInnerWindow);
  if (NS_OK != rv) {
    goto done;
  }
  mInnerWindow->QueryInterface(kIWidgetIID, (void**) &mWindow);
  if (NS_OK != rv) {
    NS_RELEASE(mInnerWindow);
  }
  else {
    mWindow->Create(aNativeParent, aBounds, nsWebShell::HandleEvent,
                    mDeviceContext, nsnull);
    // Get rid of extra reference count
    mWindow->Release();
  }

done:
  return rv;
}

NS_IMETHODIMP
nsWebShell::Destroy()
{
  nsresult rv = NS_OK;

  // Stop any URLs that are currently being loaded...
  mDocLoader->Stop();

  SetContainer(nsnull);
  SetObserver(nsnull);

  NS_IF_RELEASE(mContentViewer);

  return rv;
}


NS_IMETHODIMP
nsWebShell::GetBounds(nsRect& aResult)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  aResult.SetRect(0, 0, 0, 0);
  if (nsnull != mWindow) {
    mWindow->GetBounds(aResult);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetBounds(const nsRect& aBounds)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  if (nsnull != mWindow) {
    // Don't have the widget repaint. Layout will generate repaint requests
    // during reflow
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height,
                    PR_FALSE);
  }

  if (nsnull != mContentViewer) {
    nsRect rr(0, 0, aBounds.width, aBounds.height);
    mContentViewer->SetBounds(rr);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::MoveTo(PRInt32 aX, PRInt32 aY)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  if (nsnull != mWindow) {
    mWindow->Move(aX, aY);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::Show()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  if (nsnull != mWindow) {
    mWindow->Show(PR_TRUE);
  }
  if (nsnull != mContentViewer) {
    mContentViewer->Show();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::Hide()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");

  if (nsnull != mWindow) {
    mWindow->Show(PR_FALSE);
  }
  if (nsnull != mContentViewer) {
    mContentViewer->Hide();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetContentViewer(nsIContentViewer* aViewer)
{
  NS_IF_RELEASE(mContentViewer);
  mContentViewer = aViewer;
  NS_IF_ADDREF(aViewer);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetContentViewer(nsIContentViewer*& aResult)
{
  aResult = mContentViewer;
  NS_IF_ADDREF(mContentViewer);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetContainer(nsIWebShellContainer* aContainer)
{
  NS_IF_RELEASE(mContainer);
  mContainer = aContainer;
  NS_IF_ADDREF(aContainer);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetContainer(nsIWebShellContainer*& aResult)
{
  aResult = mContainer;
  NS_IF_ADDREF(mContainer);
  return NS_OK;
}

nsEventStatus PR_CALLBACK
nsWebShell::HandleEvent(nsGUIEvent *aEvent)
{ 
  return nsEventStatus_eIgnore;
}

NS_IMETHODIMP
nsWebShell::SetObserver(nsIStreamObserver* anObserver)
{
  NS_IF_RELEASE(mObserver);
  mObserver = anObserver;
  NS_IF_ADDREF(mObserver);
  return NS_OK;
}


NS_IMETHODIMP 
nsWebShell::GetObserver(nsIStreamObserver*& aResult)
{
  aResult = mObserver;
  NS_IF_ADDREF(mObserver);
  return NS_OK;
}


NS_IMETHODIMP 
nsWebShell::GetDocumentLoader(nsIDocumentLoader*& aResult)
{
  aResult = mDocLoader;
  NS_IF_ADDREF(mDocLoader);
  return (nsnull != mDocLoader) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsWebShell::SetPrefs(nsIPref* aPrefs)
{
  NS_IF_RELEASE(mPrefs);
  mPrefs = aPrefs;
  NS_IF_ADDREF(mPrefs);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetPrefs(nsIPref*& aPrefs)
{
  aPrefs = mPrefs;
  NS_IF_ADDREF(aPrefs);
  return NS_OK;
}

nsresult
nsWebShell::GetRootWebShell(nsIWebShell*& aResult)
{
  nsIWebShell* top = this;
  NS_ADDREF(this);
  for (;;) {
    nsIWebShell* parent;
    top->GetParent(parent);
    if (nsnull == parent) {
      break;
    }
    NS_RELEASE(top);
    top = parent;
  }
  aResult = top;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetParent(nsIWebShell* aParent)
{
  NS_IF_RELEASE(mParent);
  mParent = aParent;
  NS_IF_ADDREF(aParent);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetParent(nsIWebShell*& aParent)
{
  aParent = mParent;
  NS_IF_ADDREF(mParent);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetChildCount(PRInt32& aResult)
{
  aResult = mChildren.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::AddChild(nsIWebShell* aChild)
{
  NS_PRECONDITION(nsnull != aChild, "null ptr");
  if (nsnull == aChild) {
    return NS_ERROR_NULL_POINTER;
  }
  mChildren.AppendElement(aChild);
  aChild->SetParent(this);
  NS_ADDREF(aChild);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::ChildAt(PRInt32 aIndex, nsIWebShell*& aResult)
{
  if (PRUint32(aIndex) >= PRUint32(mChildren.Count())) {
    aResult = nsnull;
  }
  else {
    aResult = (nsIWebShell*) mChildren.ElementAt(aIndex);
    NS_IF_ADDREF(aResult);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetName(nsString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetName(const nsString& aName)
{
  mName = aName;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::FindChildWithName(const nsString& aName,
                              nsIWebShell*& aResult)
{
  aResult = nsnull;

  nsAutoString childName;
  PRInt32 i, n = mChildren.Count();
  for (i = 0; i < n; i++) {
    nsIWebShell* child = (nsIWebShell*) mChildren.ElementAt(i);
    if (nsnull != child) {
      child->GetName(childName);
      if (childName.Equals(aName)) {
        aResult = child;
        NS_ADDREF(child);
        break;
      }

      // See if child contains the shell with the given name
      nsresult rv = child->FindChildWithName(aName, aResult);
      if (NS_OK != rv) {
        return rv;
      }
      if (nsnull != aResult) {
        break;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetMarginWidth(PRInt32& aWidth)
{
  aWidth = mMarginWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetMarginWidth(PRInt32 aWidth)
{
  mMarginWidth = aWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetMarginHeight(PRInt32& aHeight)
{
  aHeight = mMarginHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::SetMarginHeight(PRInt32 aHeight)
{
  mMarginHeight = aHeight;
  return NS_OK;
}

//----------------------------------------

// History methods

NS_IMETHODIMP
nsWebShell::Back(void)
{
  return GoTo(mHistoryIndex - 1);
}

NS_IMETHODIMP
nsWebShell::Forward(void)
{
  return GoTo(mHistoryIndex + 1);
}

NS_IMETHODIMP
nsWebShell::LoadURL(const nsString& aURLSpec,
                    nsIPostData* aPostData)
{
  nsresult rv;
  PRInt32 colon, fSlash;
  PRUnichar port;
  nsAutoString urlSpec(aURLSpec);

  fSlash=urlSpec.Find('/');

  // if no scheme (protocol) is found, assume http.
  if ( ((colon=urlSpec.Find(':')) == -1) // no colon at all
      || ( (fSlash > -1) && (colon > fSlash) ) // the only colon comes after the first slash
      || ( (colon < urlSpec.Length()-1) // the first char after the first colon is a digit (i.e. a port)
            && ((port=urlSpec.CharAt(colon+1)) < '9')
            && (port > '0') )
      ) {
    nsString httpDef("http://");
    urlSpec.Insert(httpDef, 0, 7);
  }
  // Give web-shell-container right of refusal
  if (nsnull != mContainer) {
    rv = mContainer->WillLoadURL(this, urlSpec);
    if (NS_OK != rv) {
      return rv;
    }
  }

  // Discard part of history that is no longer reachable
  PRInt32 i, n = mHistory.Count();
  i = mHistoryIndex + 1;
  while (--n >= i) {
    nsString* u = (nsString*) mHistory.ElementAt(n);
    delete u;
    mHistory.RemoveElementAt(n);
  }

  // Tack on new url
  nsString* url = new nsString(urlSpec);
  mHistory.AppendElement(url);
  mHistoryIndex++;
  ShowHistory();

  // Tell web-shell-container we are loading a new url
  if (nsnull != mContainer) {
    rv = mContainer->BeginLoadURL(this, urlSpec);
    if (NS_OK != rv) {
      return rv;
    }
  }

  // Stop any documents that are currently being loaded...
  mDocLoader->Stop();

  rv = mDocLoader->LoadURL(urlSpec,       // URL string
                           nsnull,         // Command
                           this,           // Container
                           aPostData,      // Post Data
                           nsnull,         // Extra Info...
                           mObserver);     // Observer
  return rv;
}

NS_IMETHODIMP
nsWebShell::GoTo(PRInt32 aHistoryIndex)
{
  nsresult rv = NS_ERROR_ILLEGAL_VALUE;
  if ((aHistoryIndex >= 0) &&
      (aHistoryIndex <= mHistory.Count() - 1)) {
    nsString* s = (nsString*) mHistory.ElementAt(aHistoryIndex);

    // Give web-shell-container right of refusal
    nsAutoString urlSpec(*s);
    if (nsnull != mContainer) {
      rv = mContainer->WillLoadURL(this, urlSpec);
      if (NS_OK != rv) {
        return rv;
      }
    }

    printf("Goto %d\n", aHistoryIndex);
    mHistoryIndex = aHistoryIndex;
    ShowHistory();

    // Tell web-shell-container we are loading a new url
    if (nsnull != mContainer) {
      rv = mContainer->BeginLoadURL(this, urlSpec);
      if (NS_OK != rv) {
        return rv;
      }
    }

    // Stop any documents that are currently being loaded...
    mDocLoader->Stop();

    rv = mDocLoader->LoadURL(urlSpec,        // URL string
                             nsnull,         // Command
                             this,           // Container
                             nsnull,         // Post Data
                             nsnull,         // Extra Info...
                             mObserver);     // Observer
  }
  return rv;
}

NS_IMETHODIMP
nsWebShell::GetHistoryIndex(PRInt32& aResult)
{
  aResult = mHistoryIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetURL(PRInt32 aHistoryIndex, nsString& aURLResult)
{
  nsresult rv = NS_ERROR_ILLEGAL_VALUE;
  if ((aHistoryIndex >= 0) &&
      (aHistoryIndex <= mHistory.Count() - 1)) {
    aURLResult.Truncate();
    nsString* s = (nsString*) mHistory.ElementAt(aHistoryIndex);
    if (nsnull != s) {
      aURLResult = *s;
    }
    rv = NS_OK;
  }
  return rv;
}

void
nsWebShell::ShowHistory()
{
#ifdef NS_DEBUG
  if (WEB_LOG_TEST(gLogModule, WEB_TRACE_HISTORY)) {
    PRInt32 i, n = mHistory.Count();
    for (i = 0; i < n; i++) {
      if (i == mHistoryIndex) {
        printf("**");
      }
      else {
        printf("  ");
      }
      nsString* u = (nsString*) mHistory.ElementAt(i);
      fputs(*u, stdout);
      printf("\n");
    }
  }
#endif
}

//----------------------------------------

// Chrome API's

NS_IMETHODIMP
nsWebShell::SetTitle(const nsString& aTitle)
{
  // Record local title
  mTitle = aTitle;

  // Title's set on the top level web-shell are passed ont to the container
  if (nsnull == mParent) {
    if (nsnull != mContainer) {
      mContainer->SetTitle(aTitle);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::GetTitle(nsString& aResult)
{
  aResult = mTitle;
  return NS_OK;
}

//----------------------------------------------------------------------

// WebShell container implementation

NS_IMETHODIMP
nsWebShell::WillLoadURL(nsIWebShell* aShell, const nsString& aURL)
{
  if (nsnull != mContainer) {
    return mContainer->WillLoadURL(aShell, aURL);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::BeginLoadURL(nsIWebShell* aShell, const nsString& aURL)
{
  if (nsnull != mContainer) {
    return mContainer->BeginLoadURL(aShell, aURL);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::EndLoadURL(nsIWebShell* aShell, const nsString& aURL)
{
  if (nsnull != mContainer) {
    return mContainer->EndLoadURL(aShell, aURL);
  }
  return NS_OK;
}

//----------------------------------------------------------------------

// WebShell link handling

struct OnLinkClickEvent : public PLEvent {
  OnLinkClickEvent(nsWebShell* aHandler, const nsString& aURLSpec,
                   const nsString& aTargetSpec, nsIPostData* aPostData = 0);
  ~OnLinkClickEvent();

  void HandleEvent() {
    mHandler->HandleLinkClickEvent(*mURLSpec, *mTargetSpec, mPostData);
  }

  nsWebShell*  mHandler;
  nsString*    mURLSpec;
  nsString*    mTargetSpec;
  nsIPostData* mPostData;
};

static void PR_CALLBACK HandlePLEvent(OnLinkClickEvent* aEvent)
{
  aEvent->HandleEvent();
}

static void PR_CALLBACK DestroyPLEvent(OnLinkClickEvent* aEvent)
{
  delete aEvent;
}

OnLinkClickEvent::OnLinkClickEvent(nsWebShell* aHandler,
                                   const nsString& aURLSpec,
                                   const nsString& aTargetSpec,
                                   nsIPostData* aPostData)
{
  mHandler = aHandler;
  NS_ADDREF(aHandler);
  mURLSpec = new nsString(aURLSpec);
  mTargetSpec = new nsString(aTargetSpec);
  mPostData = aPostData;
  NS_IF_ADDREF(mPostData);

#ifdef XP_PC
  PL_InitEvent(this, nsnull,
               (PLHandleEventProc) ::HandlePLEvent,
               (PLDestroyEventProc) ::DestroyPLEvent);

  PLEventQueue* eventQueue = PL_GetMainEventQueue();
  PL_PostEvent(eventQueue, this);
#endif

#ifdef XP_UNIX
  PL_InitEvent(this, nsnull,
               (PLHandleEventProc) ::HandlePLEvent,
               (PLDestroyEventProc) ::DestroyPLEvent);

  PL_PostEvent(gWebShell_UnixEventQueue, this);
#endif

}

OnLinkClickEvent::~OnLinkClickEvent()
{
  NS_IF_RELEASE(mHandler);
  NS_IF_RELEASE(mPostData);
  if (nsnull != mURLSpec) delete mURLSpec;
  if (nsnull != mTargetSpec) delete mTargetSpec;
}

//----------------------------------------

NS_IMETHODIMP
nsWebShell::OnLinkClick(nsIFrame* aFrame, 
                        const nsString& aURLSpec,
                        const nsString& aTargetSpec,
                        nsIPostData* aPostData)
{
  OnLinkClickEvent* ev;
  nsresult rv = NS_OK;

  ev = new OnLinkClickEvent(this, aURLSpec, aTargetSpec, aPostData);
  if (nsnull == ev) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  return rv;
}

// Find the web shell in the entire tree that we can reach that the
// link click should go to.

// XXX This doesn't yet know how to target other windows with their
// own tree
nsIWebShell*
nsWebShell::GetTarget(const nsString& aName)
{
  nsIWebShell* target = nsnull;

  if (aName.EqualsIgnoreCase("_blank")) {
    // XXX Need api in nsIWebShellContainer
    NS_ASSERTION(0, "not implemented yet");
    target = this;
    NS_ADDREF(target);
  } 
  else if (aName.EqualsIgnoreCase("_self")) {
    target = this;
    NS_ADDREF(target);
  } 
  else if (aName.EqualsIgnoreCase("_parent")) {
    if (nsnull == mParent) {
      target = this;
    }
    else {
      target = mParent;
    }
    NS_ADDREF(target);
  }
  else if (aName.EqualsIgnoreCase("_top")) {
    GetRootWebShell(target);
  }
  else {
    // Look from the top of the tree downward
    nsIWebShell* top;
    GetRootWebShell(top);
    top->FindChildWithName(aName, target);
    if (nsnull == target) {
      target = this;
      NS_ADDREF(target);
    }
    NS_RELEASE(top);
  }

  return target;
}

void
nsWebShell::HandleLinkClickEvent(const nsString& aURLSpec,
                                 const nsString& aTargetSpec,
                                 nsIPostData* aPostData)
{
  nsIWebShell* shell = GetTarget(aTargetSpec);
  if (nsnull != shell) {
    shell->LoadURL(aURLSpec, aPostData);
  }
}

NS_IMETHODIMP
nsWebShell::OnOverLink(nsIFrame* aFrame, 
                       const nsString& aURLSpec,
                       const nsString& aTargetSpec)
{
  if (!aURLSpec.Equals(mOverURL) || !aTargetSpec.Equals(mOverTarget)) {
fputs("Was '", stdout); fputs(mOverURL, stdout); fputs("' '", stdout); fputs(mOverTarget, stdout); fputs("'\n", stdout); 
    fputs("Over link '", stdout);
    fputs(aURLSpec, stdout);
    fputs("' '", stdout);
    fputs(aTargetSpec, stdout);
    fputs("'\n", stdout);
    mOverURL = aURLSpec;
    mOverTarget = aTargetSpec;

    nsIWebShell *mRootWebShell;
    GetRootWebShell(mRootWebShell);
    if (nsnull != mRootWebShell) {
      nsIWebShellContainer *mRootContainer;
      mRootWebShell->GetContainer(mRootContainer);
      if (nsnull != mRootContainer) {
        nsIBrowserWindow *mBrowser;
        if (NS_OK == mRootContainer->QueryInterface(kIBrowserWindowIID, (void**)&mBrowser)) {
          mBrowser->SetStatus(mOverURL);
          NS_RELEASE(mBrowser);
        }
        NS_RELEASE(mRootContainer);
      }
      NS_RELEASE(mRootWebShell);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell:: GetLinkState(const nsString& aURLSpec, nsLinkState& aState)
{
  aState = eLinkState_Unvisited;
#ifdef NS_DEBUG
  if (aURLSpec.Equals("http://visited/")) {
    aState = eLinkState_Visited;
  }
  else if (aURLSpec.Equals("http://out-of-date/")) {
    aState = eLinkState_OutOfDate;
  }
  else if (aURLSpec.Equals("http://active/")) {
    aState = eLinkState_Active;
  }
  else if (aURLSpec.Equals("http://hover/")) {
    aState = eLinkState_Hover;
  }
#endif
  return NS_OK;
}

//----------------------------------------------------------------------

nsresult 
nsWebShell::GetScriptContext(nsIScriptContext** aContext)
{
  NS_PRECONDITION(nsnull != aContext, "null arg");
  nsresult res = NS_OK;

  if (nsnull == mScriptGlobal) {
    res = NS_NewScriptGlobalObject(&mScriptGlobal);
    if (NS_OK != res) {
      return res;
    }
    mScriptGlobal->SetWebShell(this);
  }

  if (nsnull == mScriptContext) {
    res = NS_CreateContext(mScriptGlobal, &mScriptContext);
    if (NS_OK != res) {
      return res;
    }
  }

  *aContext = mScriptContext;
  NS_ADDREF(mScriptContext);

  return res;
}

nsresult 
nsWebShell::GetScriptGlobalObject(nsIScriptGlobalObject** aGlobal)
{
  NS_PRECONDITION(nsnull != aGlobal, "null arg");
  nsresult res = NS_OK;

  if (nsnull == mScriptGlobal) {
    res = NS_NewScriptGlobalObject(&mScriptGlobal);
    if (NS_OK != res) {
      return res;
    }
    mScriptGlobal->SetWebShell(this);
  }

  *aGlobal = mScriptGlobal;
  NS_IF_ADDREF(mScriptGlobal);

  return res;
}

nsresult 
nsWebShell::ReleaseScriptContext(nsIScriptContext *aContext)
{
  // XXX Is this right? Why are we passing in a context?
  NS_IF_RELEASE(aContext);
  return NS_OK;
}

NS_IMETHODIMP
nsWebShell::OnConnectionsComplete()
{
  nsresult ret = NS_ERROR_FAILURE;
  
  if (nsnull != mScriptGlobal) {
    nsIDocumentViewer *mDocViewer;
    if (nsnull != mContentViewer && 
        NS_OK == mContentViewer->QueryInterface(kIDocumentViewerIID, (void**)&mDocViewer)) {
      nsIPresContext *mPresContext;
      if (NS_OK == mDocViewer->GetPresContext(mPresContext)) {
        nsEventStatus mStatus = nsEventStatus_eIgnore;
        nsMouseEvent mEvent;
        mEvent.eventStructType = NS_EVENT;
        mEvent.message = NS_PAGE_LOAD;
        ret = mScriptGlobal->HandleDOMEvent(*mPresContext, &mEvent, nsnull, DOM_EVENT_INIT, mStatus);

        NS_RELEASE(mPresContext);
      }
      NS_RELEASE(mDocViewer);
    }
  }
  return ret;
}

//----------------------------------------------------------------------

// Factory code for creating nsWebShell's

class nsWebShellFactory : public nsIFactory
{
public:
  nsWebShellFactory();
  virtual ~nsWebShellFactory();

  // nsISupports methods
  NS_IMETHOD QueryInterface(const nsIID &aIID, void **aResult);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  // nsIFactory methods
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

private:
  nsrefcnt  mRefCnt;
};

nsWebShellFactory::nsWebShellFactory()
{
  mRefCnt = 0;
}

nsWebShellFactory::~nsWebShellFactory()
{
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

nsresult
nsWebShellFactory::QueryInterface(const nsIID &aIID, void **aResult)
{
  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  // Always NULL result, in case of failure
  *aResult = NULL;

  if (aIID.Equals(kISupportsIID)) {
    *aResult = (void *)(nsISupports*)this;
  } else if (aIID.Equals(kIFactoryIID)) {
    *aResult = (void *)(nsIFactory*)this;
  }

  if (*aResult == NULL) {
    return NS_NOINTERFACE;
  }

  AddRef(); // Increase reference count for caller
  return NS_OK;
}

nsrefcnt
nsWebShellFactory::AddRef()
{
  return ++mRefCnt;
}

nsrefcnt
nsWebShellFactory::Release()
{
  if (--mRefCnt == 0) {
    delete this;
    return 0; // Don't access mRefCnt after deleting!
  }
  return mRefCnt;
}

nsresult
nsWebShellFactory::CreateInstance(nsISupports *aOuter,
                                  const nsIID &aIID,
                                  void **aResult)
{
  nsresult rv;
  nsWebShell *inst;

  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = NULL;
  if (nsnull != aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    goto done;
  }

  inst = new nsWebShell();
  if (inst == NULL) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }

  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

done:
  return rv;
}

nsresult
nsWebShellFactory::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.
  return NS_OK;
}

extern "C" NS_WEB nsresult
NS_NewWebShellFactory(nsIFactory** aFactory)
{
  nsresult rv = NS_OK;
  nsIFactory* inst = new nsWebShellFactory();
  if (nsnull == inst) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    NS_ADDREF(inst);
  }
  *aFactory = inst;
  return rv;
}
