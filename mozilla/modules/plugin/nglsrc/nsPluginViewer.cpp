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
#ifdef NECKO
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsIChannel.h"
#include "nsNeckoUtil.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
#include "nsIComponentManager.h"
#include "nsWidgetsCID.h"
#include "nsILinkHandler.h"
#include "nsIWebShell.h"
#include "nsIBrowserWindow.h"
#include "nsIContent.h"
#include "nsIDocument.h"

// Class IDs
static NS_DEFINE_IID(kChildWindowCID, NS_CHILD_CID);
static NS_DEFINE_IID(kIWidgetIID, NS_IWIDGET_IID);

// Interface IDs
static NS_DEFINE_IID(kIContentViewerIID, NS_ICONTENT_VIEWER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginHostIID, NS_IPLUGINHOST_IID);
static NS_DEFINE_IID(kIPluginInstanceOwnerIID, NS_IPLUGININSTANCEOWNER_IID);
static NS_DEFINE_IID(kILinkHandlerIID, NS_ILINKHANDLER_IID);
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIBrowserWindowIID, NS_IBROWSER_WINDOW_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOCUMENT_IID);


class PluginViewerImpl;

class PluginListener : public nsIStreamListener {
public:
  PluginListener(PluginViewerImpl* aViewer);
  virtual ~PluginListener();

  // nsISupports
  NS_DECL_ISUPPORTS

#ifdef NECKO
  // nsIStreamObserver methods:
  NS_IMETHOD OnStartBinding(nsISupports *ctxt);
  NS_IMETHOD OnStopBinding(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg);
  NS_IMETHOD OnStartRequest(nsISupports *ctxt) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD OnStopRequest(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg) { return NS_ERROR_NOT_IMPLEMENTED; }
  // nsIStreamListener methods:
  NS_IMETHOD OnDataAvailable(nsISupports *ctxt, nsIBufferInputStream *inStr, PRUint32 sourceOffset, PRUint32 count);
#else
  // nsIStreamListener
  NS_IMETHOD OnStartBinding(nsIURI* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
  NS_IMETHOD OnStopBinding(nsIURI* aURL, nsresult aStatus,
                           const PRUnichar* aMsg);
  NS_IMETHOD GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo);
  NS_IMETHOD OnDataAvailable(nsIURI* aURL, nsIInputStream* aStream,
                             PRUint32 aCount);
#endif

  PluginViewerImpl* mViewer;
  nsIStreamListener* mNextStream;
};

class pluginInstanceOwner : public nsIPluginInstanceOwner {
public:
  pluginInstanceOwner();
  virtual ~pluginInstanceOwner();

  NS_DECL_ISUPPORTS

  //nsIPluginInstanceOwner interface

  NS_IMETHOD SetInstance(nsIPluginInstance *aInstance);

  NS_IMETHOD GetInstance(nsIPluginInstance *&aInstance);

  NS_IMETHOD GetWindow(nsPluginWindow *&aWindow);

  NS_IMETHOD GetMode(nsPluginMode *aMode);

  NS_IMETHOD CreateWidget(void);

  NS_IMETHOD GetURL(const char *aURL, const char *aTarget, void *aPostData);

  NS_IMETHOD ShowStatus(const char *aStatusMsg);

  NS_IMETHOD GetDocument(nsIDocument* *aDocument);

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
    
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

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
  NS_IMETHOD Stop(void);
  NS_IMETHOD GetBounds(nsRect& aResult);
  NS_IMETHOD SetBounds(const nsRect& aBounds);
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Show();
  NS_IMETHOD Hide();
  NS_IMETHOD Print();
  NS_IMETHOD PrintContent(nsIWebShell  *aParent,nsIDeviceContext *aDContext);
  NS_IMETHOD SetEnableRendering(PRBool aOn);
  NS_IMETHOD GetEnableRendering(PRBool* aResult);

  virtual ~PluginViewerImpl();

  nsresult CreatePlugin(nsIPluginHost* aHost, const nsRect& aBounds,
                        nsIStreamListener*& aResult);

  nsresult MakeWindow(nsNativeWidget aParent,
                      nsIDeviceContext* aDeviceContext,
                      const nsRect& aBounds);

#ifdef NECKO
  nsresult StartLoad(nsIChannel* channel, nsIStreamListener*& aResult);
#else
  nsresult StartLoad(nsIURI* aURL, const char* aContentType,
                     nsIStreamListener*& aResult);
#endif

  void ForceRefresh(void);

#ifdef NECKO
  nsresult GetURI(nsIURI* *aURI);
#else
  nsresult GetURL(nsIURI *&aURL);
#endif

  nsresult GetDocument(nsIDocument* *aDocument);

  nsIWidget* mWindow;
  nsIDocument* mDocument;
  nsIContentViewerContainer* mContainer;
#ifdef NECKO
  nsIChannel* mChannel;
#else
  nsIURI* mURL;
  nsString mContentType;
#endif
  pluginInstanceOwner *mOwner;
  PRBool mEnableRendering;
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
  mEnableRendering = PR_TRUE;
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
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    nsISupports* tmp = this;
    *aInstancePtr = (void*)tmp;
    NS_ADDREF_THIS();
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
  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mContainer);
#ifdef NECKO
  NS_IF_RELEASE(mChannel);
#else
  NS_IF_RELEASE(mURL);
#endif
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
  return aDoc->QueryInterface(kIDocumentIID, (void**)&mDocument);
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
    mOwner = new pluginInstanceOwner();
    if (nsnull != mOwner) {
      NS_ADDREF(mOwner);
      rv = mOwner->Init(this, mWindow);
    }
  }
  return rv;
}

nsresult
#ifdef NECKO
PluginViewerImpl::StartLoad(nsIChannel* channel, nsIStreamListener*& aResult)
#else
PluginViewerImpl::StartLoad(nsIURI* aURL, const char* aContentType,
                            nsIStreamListener*& aResult)
#endif
{
#ifdef NECKO
  NS_IF_RELEASE(mChannel);
  mChannel = channel;
  NS_ADDREF(mChannel);

#ifdef DEBUG
  char* contentType;
  mChannel->GetContentType(&contentType);
  printf("PluginViewerImpl::StartLoad: content-type=%s\n", contentType);
  nsCRT::free(contentType);
#endif

#else
  printf("PluginViewerImpl::StartLoad: content-type=%s\n", aContentType);

  NS_IF_RELEASE(mURL);
  mURL = aURL;
  NS_IF_ADDREF(aURL);
  mContentType = aContentType;
#endif

  aResult = nsnull;

  // Only instantiate the plugin if our container can host it
  nsIPluginHost* host;
  nsresult rv = mContainer->QueryCapability(kIPluginHostIID, (void **)&host);
  if (NS_OK == rv) {
    nsRect r;
    mWindow->GetClientBounds(r);
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

#ifdef NECKO
    nsIURI* uri;
    rv = mChannel->GetURI(&uri);
    if (NS_FAILED(rv)) return rv;

    char* spec;
    rv = uri->GetSpec(&spec);
    NS_RELEASE(uri);
    if (NS_FAILED(rv)) return rv;
    nsAutoString str(spec);
    nsCRT::free(spec);

    char* ct;
    rv = mChannel->GetContentType(&ct);
    if (NS_FAILED(rv)) return rv;
    rv = aHost->InstantiateFullPagePlugin(ct, str, aResult, mOwner);
#else
    PRUnichar* fullurl;
    mURL->ToString(&fullurl);
    char* ct = mContentType.ToNewCString();
    nsAutoString str = fullurl;
    rv = aHost->InstantiateFullPagePlugin(ct, str, aResult, mOwner);
    delete fullurl;
#endif
    delete[] ct;
  }

  return rv;
}

NS_IMETHODIMP
PluginViewerImpl::Stop(void)
{
  // XXX write this
  return NS_OK;
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
    nsComponentManager::CreateInstance(kChildWindowCID, nsnull, kIWidgetIID,
                                 (void**)&mWindow);
  if (NS_OK != rv) {
    return rv;
  }
  mWindow->Create(aParent, aBounds, HandlePluginEvent, aDeviceContext);
  return rv;
}

NS_IMETHODIMP
PluginViewerImpl::GetBounds(nsRect& aResult)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    mWindow->GetBounds(aResult);
  }
  else {
    aResult.SetRect(0, 0, 0, 0);
  }
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::SetBounds(const nsRect& aBounds)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    // Don't have the widget repaint. Layout will generate repaint requests
    // during reflow
    nsIPluginInstance *inst;
    mWindow->Resize(aBounds.x, aBounds.y, aBounds.width, aBounds.height, PR_FALSE);
    if ((nsnull != mOwner) && (NS_OK == mOwner->GetInstance(inst)) && (nsnull != inst)) {
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
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::Move(PRInt32 aX, PRInt32 aY)
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    nsIPluginInstance *inst;
    mWindow->Move(aX, aY);
    if ((nsnull != mOwner) && (NS_OK == mOwner->GetInstance(inst)) && (nsnull != inst)) {
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
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::Show()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    mWindow->Show(PR_TRUE);
  }

  // XXX should we call SetWindow here?

  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::Hide()
{
  NS_PRECONDITION(nsnull != mWindow, "null window");
  if (nsnull != mWindow) {
    mWindow->Show(PR_FALSE);
  }

  // should we call SetWindow(nsnull) here?

  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::Print(void)
{
  // need to call the plugin from here somehow

  return NS_OK;
}


NS_IMETHODIMP
PluginViewerImpl::PrintContent(nsIWebShell  *aParent,nsIDeviceContext *aDContext)
{

  return NS_OK;
}


NS_IMETHODIMP
PluginViewerImpl::SetEnableRendering(PRBool aOn)
{
  mEnableRendering = aOn;
  return NS_OK;
}

NS_IMETHODIMP
PluginViewerImpl::GetEnableRendering(PRBool* aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null OUT ptr");
  if (aResult) {
    *aResult = mEnableRendering;
  }
  return NS_OK;
}

void
PluginViewerImpl::ForceRefresh()
{
  mWindow->Invalidate(PR_TRUE);
}

#ifdef NECKO
nsresult PluginViewerImpl::GetURI(nsIURI* *aURI)
{
  return mChannel->GetURI(aURI);
}
#else
nsresult PluginViewerImpl::GetURL(nsIURI *&aURL)
{
  NS_IF_ADDREF(mURL);
  aURL = mURL;
  return NS_OK;
}
#endif

nsresult PluginViewerImpl::GetDocument(nsIDocument* *aDocument)
{
  NS_IF_ADDREF(mDocument);
  *aDocument = mDocument;
  return NS_OK;
}

//----------------------------------------------------------------------

PluginListener::PluginListener(PluginViewerImpl* aViewer)
{
  NS_INIT_REFCNT();
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
#ifdef NECKO
PluginListener::OnStartBinding(nsISupports *ctxt)
#else
PluginListener::OnStartBinding(nsIURI* aURL, const char *contentType)
#endif
{
#ifdef NECKO
  nsresult rv;
  nsIChannel* channel;
  char* contentType = nsnull;

  rv = ctxt->QueryInterface(nsIChannel::GetIID(), (void**)&channel);
  if (NS_FAILED(rv)) return rv;
  rv = channel->GetContentType(&contentType);
  if (NS_FAILED(rv)) {
    NS_RELEASE(channel);
    return rv;
  }
  rv = mViewer->StartLoad(channel, mNextStream);
  NS_RELEASE(channel);
  if (NS_FAILED(rv)) return rv;
#else
  mViewer->StartLoad(aURL, contentType, mNextStream);
#endif
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
#ifdef NECKO
  return mNextStream->OnStartBinding(ctxt);
#else
  return mNextStream->OnStartBinding(aURL, contentType);
#endif
}

#ifndef NECKO
NS_IMETHODIMP
PluginListener::OnProgress(nsIURI* aURL, PRUint32 aProgress,
                           PRUint32 aProgressMax)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnProgress(aURL, aProgress, aProgressMax);
}

NS_IMETHODIMP
PluginListener::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->OnStatus(aURL, aMsg);
}
#endif

NS_IMETHODIMP
#ifdef NECKO
PluginListener::OnStopBinding(nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg)
#else
PluginListener::OnStopBinding(nsIURI* aURL, nsresult aStatus,
                              const PRUnichar* aMsg)
#endif
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
#ifdef NECKO
  return mNextStream->OnStopBinding(ctxt, status, errorMsg);
#else
  return mNextStream->OnStopBinding(aURL, aStatus, aMsg);
#endif
}

#ifndef NECKO
NS_IMETHODIMP
PluginListener::GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo)
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
  return mNextStream->GetBindInfo(aURL, aInfo);
}
#endif

NS_IMETHODIMP
#ifdef NECKO
PluginListener::OnDataAvailable(nsISupports *ctxt, nsIBufferInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
#else
PluginListener::OnDataAvailable(nsIURI* aURL, nsIInputStream* aStream,
                                PRUint32 aCount)
#endif
{
  if (nsnull == mNextStream) {
    return NS_ERROR_FAILURE;
  }
#ifdef NECKO
  return mNextStream->OnDataAvailable(ctxt, inStr, sourceOffset, count);
#else
  return mNextStream->OnDataAvailable(aURL, aStream, aCount);
#endif
}

//----------------------------------------------------------------------

pluginInstanceOwner :: pluginInstanceOwner()
{
  NS_INIT_REFCNT();

  memset(&mPluginWindow, 0, sizeof(mPluginWindow));
  mInstance = nsnull;
  mWindow = nsnull;
  mViewer = nsnull;
}

pluginInstanceOwner :: ~pluginInstanceOwner()
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

NS_IMPL_ISUPPORTS(pluginInstanceOwner, kIPluginInstanceOwnerIID);

NS_IMETHODIMP pluginInstanceOwner :: SetInstance(nsIPluginInstance *aInstance)
{
  NS_IF_RELEASE(mInstance);
  mInstance = aInstance;
  NS_IF_ADDREF(mInstance);

  return NS_OK;
}

NS_IMETHODIMP pluginInstanceOwner :: GetInstance(nsIPluginInstance *&aInstance)
{
  NS_IF_ADDREF(mInstance);
  aInstance = mInstance;

  return NS_OK;
}

NS_IMETHODIMP pluginInstanceOwner :: GetWindow(nsPluginWindow *&aWindow)
{
  aWindow = &mPluginWindow;
  return NS_OK;
}

NS_IMETHODIMP pluginInstanceOwner :: GetMode(nsPluginMode *aMode)
{
  *aMode = nsPluginMode_Full;
  return NS_OK;
}

NS_IMETHODIMP pluginInstanceOwner :: CreateWidget(void)
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

NS_IMETHODIMP pluginInstanceOwner :: GetURL(const char *aURL, const char *aTarget, void *aPostData)
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
#ifdef NECKO
        nsIURI  *uri;
        rv = mViewer->GetURI(&uri);
#else
        nsIURI  *url;
        rv = mViewer->GetURL(url);
#endif

        if (NS_OK == rv)
        {
          // Create an absolute URL
#ifdef NECKO
          char* absURIStr;
          rv = NS_MakeAbsoluteURI(aURL, uri, &absURIStr);
          NS_RELEASE(uri);
          nsAutoString fullurl(absURIStr);
          nsCRT::free(absURIStr);
#else
          nsAutoString  uniurl = nsAutoString(aURL);
          const char* spec;
          (void)url->GetSpec(&spec);
          nsAutoString  base = nsAutoString(spec);
          nsAutoString  fullurl;
          rv = NS_MakeAbsoluteURL(url, base, uniurl, fullurl);
          NS_RELEASE(url);
#endif

          if (NS_OK == rv) {
            nsAutoString  unitarget = nsAutoString(aTarget);
            rv = lh->OnLinkClick(nsnull, eLinkVerb_Replace, fullurl.GetUnicode(), unitarget.GetUnicode(), nsnull);
          }
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

NS_IMETHODIMP pluginInstanceOwner :: ShowStatus(const char *aStatusMsg)
{
  nsresult  rv = NS_ERROR_FAILURE;

  if (nsnull != mViewer)
  {
    nsIContentViewerContainer *cont;

    rv = mViewer->GetContainer(cont);

    if ((NS_OK == rv) && (nsnull != cont))
    {
      nsIWebShell *ws;

      rv = cont->QueryInterface(kIWebShellIID, (void **)&ws);

      if (NS_OK == rv)
      {
        nsIWebShell *rootWebShell;

        ws->GetRootWebShell(rootWebShell);

        if (nsnull != rootWebShell)
        {
          nsIWebShellContainer *rootContainer;

          rv = rootWebShell->GetContainer(rootContainer);

          if (nsnull != rootContainer)
          {
            nsIBrowserWindow *browserWindow;

            if (NS_OK == rootContainer->QueryInterface(kIBrowserWindowIID, (void**)&browserWindow))
            {
              nsAutoString  msg = nsAutoString(aStatusMsg);

              rv = browserWindow->SetStatus(msg.GetUnicode());
              NS_RELEASE(browserWindow);
            }

            NS_RELEASE(rootContainer);
          }

          NS_RELEASE(rootWebShell);
        }

        NS_RELEASE(ws);
      }

      NS_RELEASE(cont);
    }
  }

  return rv;
}

NS_IMETHODIMP pluginInstanceOwner :: GetDocument(nsIDocument* *aDocument)
{
	return mViewer->GetDocument(aDocument);
}

NS_IMETHODIMP pluginInstanceOwner :: Init(PluginViewerImpl *aViewer, nsIWidget *aWindow)
{
  //do not addref
  mWindow = aWindow;
  mViewer = aViewer;

  return NS_OK;
}
