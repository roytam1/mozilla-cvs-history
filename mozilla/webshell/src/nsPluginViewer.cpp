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
#include "nscore.h"
#include "nsCRT.h"
#include "nsIContentViewer.h"
#include "nsIContentViewerContainer.h"
#include "nsIPluginHost.h"
#include "nsIPluginInstance.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsRepository.h"
#include "nsWidgetsCID.h"
#include "nsILinkHandler.h"

// Class IDs
static NS_DEFINE_IID(kChildWindowCID, NS_CHILD_CID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

// Interface IDs
static NS_DEFINE_IID(kIContentViewerIID, NS_ICONTENT_VIEWER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginHostIID, NS_IPLUGINHOST_IID);
static NS_DEFINE_IID(kIPluginInstanceOwnerIID, NS_IPLUGININSTANCEOWNER_IID);
static NS_DEFINE_IID(kILinkHandlerIID, NS_ILINKHANDLER_IID);

class PluginViewerImpl;

class PluginListener : public nsIStreamListener {
public:
  PluginListener(PluginViewerImpl* aViewer);
  ~PluginListener();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIStreamListener
  NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURL* aURL, const nsString &aMsg);
  NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 aStatus,
                           const nsString& aMsg);
  NS_IMETHOD GetBindInfo(nsIURL* aURL);
  NS_IMETHOD OnDataAvailable(nsIURL* aURL, nsIInputStream* aStream,
                             PRInt32 aCount);

  PluginViewerImpl* mViewer;
  nsIStreamListener* mNextStream;
};

class nsPluginInstanceOwner : public nsIPluginInstanceOwner {
public:
  nsPluginInstanceOwner();
  ~nsPluginInstanceOwner();

  NS_DECL_ISUPPORTS

  //nsIPluginInstanceOwner interface

  NS_IMETHOD SetInstance(nsIPluginInstance *aInstance);

  NS_IMETHOD GetInstance(nsIPluginInstance *&aInstance);

  NS_IMETHOD GetWindow(nsPluginWindow *&aWindow);

  NS_IMETHOD GetMode(nsPluginMode *aMode);

  NS_IMETHOD GetAttributes(PRUint16& n, const char*const*& names,
                           const char*const*& values);

  NS_IMETHOD GetAttribute(const char* name, const char* *result);

  NS_IMETHOD CreateWidget(void);

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget, void *aPostData);

  //locals

  NS_IMETHOD Init(PluginViewerImpl *aViewer, nsIWidget *aWindow);

private:
  nsPluginWindow    mPluginWindow;
  nsIPluginInstance *mInstance;
  nsIWidget         *mWindow;       //we do not addref this...
  PluginViewerImpl  *mViewer;       //we do not addref this...
};

class PluginViewerImpl : public nsIContentViewer
{
public:
  PluginViewerImpl(const char* aCommand, nsIStreamListener** aDocListener);
    
  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIContentViewer
  NS_IMETHOD Init(nsNativeWidget aParent,
                  nsIDeviceContext* aDeviceContext,
                  nsIPref* aPrefs,
                  const nsRect& aBounds,
                  nsScrollPreference aScrolling = nsScrollPreference_kAuto);
    
  NS_IMETHOD BindToDocument(nsISupports* aDoc, const char* aCommand);
  NS_IMETHOD SetContainer(nsIContentViewerContainer* aContainer);
  NS_IMETHOD GetContainer(nsIContentViewerContainer*& aContainerResult);

  virtual nsRect GetBounds();
  virtual void SetBounds(const nsRect& aBounds);
  virtual void Move(PRInt32 aX, PRInt32 aY);
  virtual void Show();
  virtual void Hide();

  ~PluginViewerImpl();

  nsresult CreatePlugin(nsIPluginHost* aHost, const nsRect& aBounds,
                        nsIStreamListener*& aResult);

  nsresult MakeWindow(nsNativeWidget aParent,
                      nsIDeviceContext* aDeviceContext,
                      const nsRect& aBounds);

  nsresult StartLoad(nsIURL* aURL, const char* aContentType,
                     nsIStreamListener*& aResult);

  void ForceRefresh(void);

  nsresult GetURL(nsIURL *&aURL);

  nsIWidget* mWindow;
  nsIContentViewerContainer* mContainer;
  nsIURL* mURL;
  nsString mContentType;
  nsPluginInstanceOwner *mOwner;
};

//----------------------------------------------------------------------

nsresult
NS_NewPluginContentViewer(const char* aCommand,
                          nsIStreamListener** aDocListener,
                          nsIContentViewer** aDocViewer)
{
  PluginViewerImpl* it = new PluginViewerImpl(aCommand, aDocListener);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIContentViewerIID, (void**) aDocViewer);
}

// Note: operator new zeros our memory
PluginViewerImpl::PluginViewerImpl(const char* aCommand,
                                   nsIStreamListener** aDocListener)
{
  NS_INIT_REFCNT();
  nsIStreamListener* it = new PluginListener(this);
  *aDocListener = it;
}

// ISupports implementation...
NS_IMPL_ADDREF(PluginViewerImpl)
NS_IMPL_RELEASE(PluginViewerImpl)

nsresult
PluginViewerImpl::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(kIContentViewerIID)) {
    nsIContentViewer* tmp = this;
    *aInstancePtr = (void*)tmp;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsISupports* tmp = this;
    *aInstancePtr = (void*)tmp;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

PluginViewerImpl::~PluginViewerImpl()
{
  NS_IF_RELEASE(mOwner);
  if (nsnull != mWindow) {
    mWindow->Destroy();
    NS_RELEASE(mWindow);
  }
  NS_IF_RELEASE(mContainer);
  NS_IF_RELEASE(mURL);
}

/*
 * This method is called by the Document Loader once a document has
 * been created for a particular data stream...  The content viewer
 * must cache this document for later use when Init(...) is called.
 */
NS_IMETHODIMP
PluginViewerImpl::BindToDocument(nsISupports *aDoc, const char *aCommand)
{
#ifdef NS_DEBUG
  printf("PluginViewerImpl::BindToDocument\n");
#endif
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::SetContainer(nsIContentViewerContainer* aContainer)
{
  NS_IF_RELEASE(mContainer);
  mContainer = aContainer;
  NS_IF_ADDREF(mContainer);
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::GetContainer(nsIContentViewerContainer*& aResult)
{
  aResult = mContainer;
  NS_IF_ADDREF(mContainer);
  return NS_OK;
}


NS_IMETHODIMP
PluginViewerImpl::Init(nsNativeWidget aNativeParent,
                       nsIDeviceContext* aDeviceContext,
                       nsIPref* aPrefs,
                       const nsRect& aBounds,
                       nsScrollPreference aScrolling)
{
  nsresult rv = MakeWindow(aNativeParent, aDeviceContext, aBounds);
  if (NS_OK == rv) {
    mOwner = new nsPluginInstanceOwner();
    if (nsnull != mOwner) {
      NS_ADDREF(mOwner);
      rv = mOwner->Init(this, mWindow);
    }
  }
  return rv;
}

nsresult
PluginViewerImpl::StartLoad(nsIURL* aURL, const char* aContentType,
                            nsIStreamListener*& aResult)
{
  printf("PluginViewerImpl::StartLoad: content-type=%s\n", aContentType);

  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_IF_ADDREF(aURL);
  mContentType = aContentType;

  aResult = nsnull;

  // Only instantiate the plugin if our container can host it
  nsIPluginHost* host;
  nsresult rv = mContainer->QueryCapability(kIPluginHostIID, (void **)&host);
  if (NS_OK == rv) {
    nsRect r;
    mWindow->GetBounds(r);
    rv = CreatePlugin(host, nsRect(0, 0, r.width, r.height), aResult);
    NS_RELEASE(host);
  }

  return rv;
}

nsresult
PluginViewerImpl::CreatePlugin(nsIPluginHost* aHost, const nsRect& aBounds,
                               nsIStreamListener*& aResult)
{
  nsresult rv = NS_OK;

  if (nsnull != mOwner) {
    nsPluginWindow  *win;

    mOwner->GetWindow(win);

    win->x = aBounds.x;
    win->y = aBounds.y;
    win->width = aBounds.width;
    win->height = aBounds.height;
    win->clipRect.top = aBounds.y;
    win->clipRect.left = aBounds.x;
    win->clipRect.bottom = aBounds.YMost();
    win->clipRect.right = aBounds.XMost();
  #ifdef XP_UNIX
    win->ws_info = nsnull;   //XXX need to figure out what this is. MMP
  #endif

    nsAutoString fullurl;
    mURL->ToString(fullurl);

    char* ct = mContentType.ToNewCString();
    rv = aHost->InstantiatePlugin(ct, fullurl, aResult, mOwner);
    delete ct;
  }

  return rv;
}

static nsEventStatus PR_CALLBACK
HandlePluginEvent(nsGUIEvent *aEvent)
{
  return nsEventStatus_eIgnore;
}

nsresult
PluginViewerImpl::MakeWindow(nsNativeWidget aParent,
                             nsIDeviceContext* aDeviceContext,
                             const nsRect& aBounds)
{
  nsresult rv =
    nsRepository::CreateInstance(kChildWindowCID, nsnull, kIWidgetIID,
                                 (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }
  mWindow->Create(aParent, aBounds, HandlePluginEvent, aDeviceContext);
  return rv;
}

nsRect
PluginViewerImpl::GetBounds()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  nsRect zr(0, 0, 0, 0);
  if (nsnull != mWindow) {
    mWindow->GetBounds(zr);
  }
  return zr;
}

void
PluginViewerImpl::SetBounds(const nsRect& aBounds)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    // Don't have the widget repaint. Layout will generate repaint requests
    // during reflow
    nsIPluginInstance *inst;
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height, PR_FALSE);
    if ((nsnull != mOwner) && (NS_OK == mOwner->GetInstance(inst))) {
      nsPluginWindow  *win;
      if (NS_OK == mOwner->GetWindow(win)) {
        win->x = aBounds.x;
        win->y = aBounds.y;
        win->width = aBounds.width;
        win->height = aBounds.height;
        win->clipRect.top = aBounds.y;
        win->clipRect.left = aBounds.x;
        win->clipRect.bottom = aBounds.YMost();
        win->clipRect.right = aBounds.XMost();

        inst->SetWindow(win);
      }
      NS_RELEASE(inst);
    }
  }
}

void
PluginViewerImpl::Move(PRInt32 aX, PRInt32 aY)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    nsIPluginInstance *inst;
    mWindow->Move(aX, aY);
    if ((nsnull != mOwner) && (NS_OK == mOwner->GetInstance(inst))) {
      nsPluginWindow  *win;
      if (NS_OK == mOwner->GetWindow(win)) {
        win->x = aX;
        win->y = aY;
        win->clipRect.bottom = (win->clipRect.bottom - win->clipRect.top) + aY;
        win->clipRect.right = (win->clipRect.right - win->clipRect.left) + aX;
        win->clipRect.top = aY;
        win->clipRect.left = aX;

        inst->SetWindow(win);
      }
      NS_RELEASE(inst);
    }
  }
}

void
PluginViewerImpl::Show()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    mWindow->Show(PR_TRUE);
  }
}

void
PluginViewerImpl::Hide()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    mWindow->Show(PR_FALSE);
  }
}

void
PluginViewerImpl::ForceRefresh()
{
  mWindow->Invalidate(PR_TRUE);
}

nsresult PluginViewerImpl::GetURL(nsIURL *&aURL)
{
  NS_IF_ADDREF(mURL);
  aURL = mURL;
  return NS_OK;
}

//----------------------------------------------------------------------

PluginListener::PluginListener(PluginViewerImpl* aViewer)
{
  mViewer = aViewer;
  NS_ADDREF(aViewer);
  mRefCnt = 1;
}

PluginListener::~PluginListener()
{
  NS_RELEASE(mViewer);
  NS_IF_RELEASE(mNextStream);
}

NS_IMPL_ISUPPORTS(PluginListener, kIStreamListenerIID)

NS_IMETHODIMP
PluginListener::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
  mViewer->StartLoad(aURL, aContentType, mNextStream);
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnStartBinding(aURL, aContentType);
}

NS_IMETHODIMP
PluginListener::OnProgress(nsIURL* aURL, PRInt32 aProgress,
                           PRInt32 aProgressMax)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnProgress(aURL, aProgress, aProgressMax);
}

NS_IMETHODIMP
PluginListener::OnStatus(nsIURL* aURL, const nsString &aMsg)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnStatus(aURL, aMsg);
}

NS_IMETHODIMP
PluginListener::OnStopBinding(nsIURL* aURL, PRInt32 aStatus,
                              const nsString& aMsg)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnStopBinding(aURL, aStatus, aMsg);
}

NS_IMETHODIMP
PluginListener::GetBindInfo(nsIURL* aURL)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->GetBindInfo(aURL);
}

NS_IMETHODIMP
PluginListener::OnDataAvailable(nsIURL* aURL, nsIInputStream* aStream,
                                PRInt32 aCount)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnDataAvailable(aURL, aStream, aCount);
}

//----------------------------------------------------------------------

nsPluginInstanceOwner :: nsPluginInstanceOwner()
{
  memset(&mPluginWindow, 0, sizeof(mPluginWindow));
  mInstance = nsnull;
  mWindow = nsnull;
  mViewer = nsnull;
}

nsPluginInstanceOwner :: ~nsPluginInstanceOwner()
{
  if (nsnull != mInstance)
  {
    mInstance->Stop();
    mInstance->Destroy();
    NS_RELEASE(mInstance);
  }

  mWindow = nsnull;
  mViewer = nsnull;
}

NS_IMPL_ISUPPORTS(nsPluginInstanceOwner, kIPluginInstanceOwnerIID)

NS_IMETHODIMP nsPluginInstanceOwner :: SetInstance(nsIPluginInstance *aInstance)
{
  NS_IF_RELEASE(mInstance);
  mInstance = aInstance;
  NS_IF_ADDREF(mInstance);

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetInstance(nsIPluginInstance *&aInstance)
{
  NS_IF_ADDREF(mInstance);
  aInstance = mInstance;

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetWindow(nsPluginWindow *&aWindow)
{
  aWindow = &mPluginWindow;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetMode(nsPluginMode *aMode)
{
  *aMode = nsPluginMode_Full;
  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetAttributes(PRUint16& n, const char*const*& names,
                                                     const char*const*& values)
{
  n = 0;
  names = nsnull;
  values = nsnull;

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetAttribute(const char* name, const char* *result)
{
  name = nsnull;
  result = nsnull;

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: CreateWidget(void)
{
  PRBool    windowless;

  if (nsnull != mInstance)
  {
    mInstance->GetValue(nsPluginInstanceVariable_WindowlessBool, (void *)&windowless);

    if (PR_TRUE == windowless)
    {
      mPluginWindow.window = nsnull;    //XXX this needs to be a HDC
      mPluginWindow.type = nsPluginWindowType_Drawable;
    }
    else if (nsnull != mWindow)
    {
      mPluginWindow.window = (nsPluginPort *)mWindow->GetNativeData(NS_NATIVE_WINDOW);
      mPluginWindow.type = nsPluginWindowType_Window;
    }
    else
      return NS_ERROR_FAILURE;
  }
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP nsPluginInstanceOwner :: GetURL(const char *aURL, const char *aTarget, void *aPostData)
{
  nsresult  rv;

  if (nsnull != mViewer)
  {
    nsIContentViewerContainer *cont;

    rv = mViewer->GetContainer(cont);

    if (NS_OK == rv)
    {
      nsILinkHandler  *lh;

      rv = cont->QueryInterface(kILinkHandlerIID, (void **)&lh);

      if (NS_OK == rv)
      {
        nsIURL  *url;

        rv = mViewer->GetURL(url);

        if (NS_OK == rv)
        {
          nsAutoString  uniurl = nsAutoString(aURL);
          nsAutoString  unitarget = nsAutoString(aTarget);
          nsAutoString  base = nsAutoString(url->GetSpec());
          nsAutoString  fullurl;

          // Create an absolute URL
          rv = NS_MakeAbsoluteURL(url, base, uniurl, fullurl);

          if (NS_OK == rv)
            rv = lh->OnLinkClick(nsnull, fullurl.GetUnicode(), unitarget.GetUnicode(), nsnull);

          NS_RELEASE(url);
        }

        NS_RELEASE(lh);
      }

      NS_RELEASE(cont);
    }
  }
  else
    rv = NS_ERROR_FAILURE;

  return rv;
}

NS_IMETHODIMP nsPluginInstanceOwner :: Init(PluginViewerImpl *aViewer, nsIWidget *aWindow)
{
  //do not addref
  mWindow = aWindow;
  mViewer = aViewer;

  return NS_OK;
}
