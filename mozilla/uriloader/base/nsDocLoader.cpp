/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsIDocumentLoader.h"
#include "nsIWebShell.h"
#include "prmem.h"
#include "plstr.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsIStreamListener.h"
#include "nsIFactory.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsITimer.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsVoidArray.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"
#ifndef NECKO
#include "nsIHttpURL.h"
#include "nsIRefreshUrl.h"
#include "nsIURLGroup.h"
#include "nsILoadAttribs.h"
#include "nsINetService.h"
//#include "nsIPostToServer.h"
#else
#include "nsIIOService.h"
#include "nsILoadGroup.h"
//#include "nsILoadGroupObserver.h"
#include "nsNeckoUtil.h"
#include "nsIURL.h"
#include "nsIDNSService.h"
#include "nsIChannel.h"
#include "nsIHTTPChannel.h"
#include "nsHTTPEnums.h"
#include "nsIProgressEventSink.h"
#endif // NECKO
#include "nsIGenericFactory.h"
#include "nsIStreamLoadableDocument.h"
#include "nsCOMPtr.h"
#include "nsCom.h"
#include "prlog.h"
#include "prprf.h"

#include "nsWeakReference.h"

#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);

#include <iostream.h>

// XXX ick ick ick
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"

#ifdef DEBUG
#undef NOISY_CREATE_DOC
#else
#undef NOISY_CREATE_DOC
#endif

#if defined(DEBUG) || defined(FORCE_PR_LOG)
PRLogModuleInfo* gDocLoaderLog = nsnull;
#endif /* DEBUG || FORCE_PR_LOG */


  /* Private IIDs... */
/* eb001fa0-214f-11d2-bec0-00805f8a66dc */
#define NS_DOCUMENTBINDINFO_IID   \
{ 0xeb001fa0, 0x214f, 0x11d2, \
  {0xbe, 0xc0, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }


/* Define IIDs... */
static NS_DEFINE_IID(kIStreamObserverIID,          NS_ISTREAMOBSERVER_IID);
static NS_DEFINE_IID(kIDocumentLoaderIID,          NS_IDOCUMENTLOADER_IID);
static NS_DEFINE_IID(kIDocumentLoaderFactoryIID,   NS_IDOCUMENTLOADERFACTORY_IID);
static NS_DEFINE_IID(kDocumentBindInfoIID,         NS_DOCUMENTBINDINFO_IID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDocumentIID,                NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIStreamListenerIID,          NS_ISTREAMLISTENER_IID);
#ifndef NECKO
static NS_DEFINE_IID(kHTTPURLIID,                  NS_IHTTPURL_IID);
static NS_DEFINE_IID(kRefreshURLIID,               NS_IREFRESHURL_IID);
static NS_DEFINE_IID(kILoadGroupIID,               NS_ILOADGROUP_IID);
static NS_DEFINE_IID(kINetServiceIID,              NS_INETSERVICE_IID);
static NS_DEFINE_IID(kNetServiceCID,               NS_NETSERVICE_CID);
#else
static NS_DEFINE_CID(kIOServiceCID,                NS_IOSERVICE_CID);
#endif // NECKO
static NS_DEFINE_IID(kIContentViewerContainerIID,  NS_ICONTENT_VIEWER_CONTAINER_IID);
static NS_DEFINE_CID(kGenericFactoryCID,           NS_GENERICFACTORY_CID);

/* Forward declarations.... */
class nsDocLoaderImpl;


#ifdef NECKO
class nsChannelListener : public nsIStreamListener
{
public:
  nsChannelListener();

  nsresult Init(nsDocLoaderImpl *aDocLoader, nsIStreamListener *aListener);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMOBSERVER
  NS_DECL_NSISTREAMLISTENER

protected:
  virtual ~nsChannelListener();

  nsDocLoaderImpl *mDocLoader;
  nsCOMPtr<nsIStreamListener> mNextListener;
};

#endif // NECKO




/* 
 * The nsDocumentBindInfo contains the state required when a single document
 * is being loaded...  Each instance remains alive until its target URL has 
 * been loaded (or aborted).
 *
 * The Document Loader maintains a list of nsDocumentBindInfo instances which 
 * represents the set of documents actively being loaded...
 */
class nsDocumentBindInfo : public nsIStreamListener
                         , public nsIProgressEventSink
{
public:
    nsDocumentBindInfo();

    nsresult Init(nsDocLoaderImpl* aDocLoader,
                  const char *aCommand, 
                  nsIContentViewerContainer* aContainer,
                  nsISupports* aExtraInfo,
                  nsIStreamObserver* anObserver);

    NS_DECL_ISUPPORTS

    nsresult Bind(nsIURI* aURL, nsIStreamListener* aListener, nsIInputStream *postDataStream = nsnull, const PRUnichar* aReferrer=nsnull);

    nsresult Bind(nsIStreamListener *aListener);

    // nsIStreamObserver methods:
    NS_DECL_NSISTREAMOBSERVER
	
	// nsIStreamListener methods:
    NS_DECL_NSISTREAMLISTENER

    // nsIProgressEventSink methods:
    NS_DECL_NSIPROGRESSEVENTSINK


    nsresult GetStatus(void) { return mStatus; }

    /* nsIRefreshURL interface methods... */
    NS_IMETHOD RefreshURL(nsIURI* aURL, PRInt32 millis, PRBool repeat);
    NS_IMETHOD CancelRefreshURLTimers(void);

protected:
    virtual ~nsDocumentBindInfo();

protected:
    char*               m_Command;
    nsIContentViewerContainer* m_Container;
    nsISupports*        m_ExtraInfo;
    nsIStreamObserver*  m_Observer;
    nsIStreamListener*  m_NextStream;
    nsDocLoaderImpl*    m_DocLoader;
    nsresult            mStatus;
};




/****************************************************************************
 * nsDocLoaderImpl implementation...
 ****************************************************************************/

class nsDocLoaderImpl : public nsIDocumentLoader, 
                        public nsIStreamObserver,
                        public nsILoadGroupListenerFactory,
                        public nsSupportsWeakReference
{
public:

    nsDocLoaderImpl();

    nsresult Init();

    // for nsIGenericFactory:
    static NS_IMETHODIMP Create(nsISupports *aOuter, const nsIID &aIID, void **aResult);

    NS_DECL_ISUPPORTS

    // nsIDocumentLoader interface
    NS_IMETHOD LoadDocument(nsIURI * aUri, 
                            const char *aCommand,
                            nsIContentViewerContainer* aContainer,
                            nsIInputStream* aPostDataStream = nsnull,
                            nsISupports* aExtraInfo = nsnull,
                            nsIStreamObserver* anObserver = nsnull,
                            nsLoadFlags aType = nsIChannel::LOAD_NORMAL,
                            const PRUint32 aLocalIP = 0,
                            const PRUnichar* aReferrer = nsnull);

    NS_IMETHOD LoadSubDocument(nsIURI * aUri,
                               nsISupports* aExtraInfo = nsnull,
                               nsLoadFlags aType = nsIChannel::LOAD_NORMAL,
                               const PRUint32 aLocalIP = 0);

    NS_IMETHOD Stop(void);

    NS_IMETHOD IsBusy(PRBool& aResult);

    NS_IMETHOD CreateDocumentLoader(nsIDocumentLoader** anInstance);

    NS_IMETHOD AddObserver(nsIDocumentLoaderObserver *aObserver);
    NS_IMETHOD RemoveObserver(nsIDocumentLoaderObserver *aObserver);

    NS_IMETHOD SetContainer(nsIContentViewerContainer* aContainer);
    NS_IMETHOD GetContainer(nsIContentViewerContainer** aResult);
    NS_IMETHOD GetContentViewerContainer(PRUint32 aDocumentID, 
                                         nsIContentViewerContainer** aResult);
    NS_IMETHOD Destroy();

    // nsILoadGroup interface...
    NS_IMETHOD CreateURL(nsIURI** aInstancePtrResult, 
                         nsIURI* aBaseURL,
                         const nsString& aSpec,
                         nsISupports* aContainer);

    NS_IMETHOD OpenStream(nsIURI *aUrl, 
                          nsIStreamListener *aConsumer);

    NS_IMETHOD GetDefaultLoadAttributes(nsLoadFlags *aLoadAttribs);
    NS_IMETHOD SetDefaultLoadAttributes(nsLoadFlags aLoadAttribs);

    NS_IMETHOD AddChildGroup(nsILoadGroup* aGroup);
    NS_IMETHOD RemoveChildGroup(nsILoadGroup* aGroup);

    // nsILoadGroupListenerFactory methods...
    NS_IMETHOD CreateLoadGroupListener(nsIStreamListener *aListener,
                                       nsIStreamListener **aResult);

    // Implementation specific methods...
    void FireOnStartDocumentLoad(nsDocLoaderImpl* aLoadInitiator,
                                 nsIURI* aURL, 
                                 const char* aCommand);
    void FireOnEndDocumentLoad(nsDocLoaderImpl* aLoadInitiator,
                               nsresult aStatus);
							   

    void FireOnStartURLLoad(nsDocLoaderImpl* aLoadInitiator,
                            nsIChannel* channel, 
                            nsIContentViewer* aViewer);

    void FireOnProgressURLLoad(nsDocLoaderImpl* aLoadInitiator,
                               nsIChannel* channel, 
                               PRUint32 aProgress, 
                               PRUint32 aProgressMax);

    void FireOnStatusURLLoad(nsDocLoaderImpl* aLoadInitiator,
                             nsIChannel* channel, 
                             nsString& aMsg);

    void FireOnEndURLLoad(nsDocLoaderImpl* aLoadInitiator,
                          nsIChannel* channel, nsresult aStatus);

    nsresult LoadURLComplete(nsIChannel* channel, nsISupports* ctxt,
                             nsISupports* aLoader, nsresult aStatus,
                             const PRUnichar* aMsg);
    void SetParent(nsDocLoaderImpl* aParent);
    void SetDocumentChannel(nsIChannel* channel);

    NS_DECL_NSISTREAMOBSERVER

    nsILoadGroup* GetLoadGroup() { return mLoadGroup; } 

    nsresult CreateContentViewer(const char *aCommand,
                                 nsIChannel* channel,
                                 const char* aContentType, 
                                 nsIContentViewerContainer* aContainer,
                                 nsISupports* aExtraInfo,
                                 nsIStreamListener** aDocListener,
                                 nsIContentViewer** aDocViewer);

protected:
    virtual ~nsDocLoaderImpl();

protected:

    // IMPORTANT: The ownership implicit in the following member
    // variables has been explicitly checked and set using nsCOMPtr
    // for owning pointers and raw COM interface pointers for weak
    // (ie, non owning) references. If you add any members to this
    // class, please make the ownership explicit (pinkerton, scc).
  
    nsIChannel*                mDocumentChannel;       // [OWNER] ???compare with document
    nsVoidArray                mDocObservers;
    nsCOMPtr<nsIStreamObserver> mStreamObserver;   // ??? unclear what to do here
    nsIContentViewerContainer* mContainer;         // [WEAK] it owns me!

    nsDocLoaderImpl*           mParent;            // [OWNER] but upside down ownership model
                                                   //  needs to be fixed***
    /*
     * This flag indicates that the loader is loading a document.  It is set
     * from the call to LoadDocument(...) until the OnConnectionsComplete(...)
     * notification is fired...
     */
    PRBool mIsLoadingDocument;

    nsCOMPtr<nsILoadGroup>      mLoadGroup;
};


nsDocLoaderImpl::nsDocLoaderImpl()
{
    NS_INIT_REFCNT();

#if defined(DEBUG) || defined(FORCE_PR_LOG)
    if (nsnull == gDocLoaderLog) {
        gDocLoaderLog = PR_NewLogModule("DocLoader");
    }
#endif /* DEBUG || FORCE_PR_LOG */

#ifdef NECKO
    mDocumentChannel = nsnull;
#else
    mDocumentUrl    = nsnull;
    mForegroundURLs = 0;
    mTotalURLs      = 0;
#endif
    mParent         = nsnull;
    mContainer      = nsnull;

    mIsLoadingDocument = PR_FALSE;

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocLoader:%p: created.\n", this));
}

nsresult
nsDocLoaderImpl::Init()
{
    nsresult rv;

#ifdef NECKO
    rv = NS_NewLoadGroup(nsnull, this, nsnull, getter_AddRefs(mLoadGroup));
    if (NS_FAILED(rv)) return rv;
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocLoader:%p: load group %x.\n", this, mLoadGroup));

    rv = mLoadGroup->SetGroupListenerFactory( (nsILoadGroupListenerFactory*)this);
    if (NS_FAILED(rv)) return rv;

#else
    rv = NS_NewISupportsArray(getter_AddRefs(m_LoadingDocsList));
    if (NS_FAILED(rv)) return rv;
    rv = NS_NewLoadAttribs(getter_AddRefs(m_LoadAttrib));
    if (NS_FAILED(rv)) return rv;
#endif
    return rv;
}

nsDocLoaderImpl::~nsDocLoaderImpl()
{
    Stop();
    if (nsnull != mParent) {
#ifdef NECKO
        mParent->RemoveChildGroup(GetLoadGroup());
#else
        mParent->RemoveChildGroup(this);
#endif
        NS_RELEASE(mParent);
    }

#ifdef NECKO
    NS_IF_RELEASE(mDocumentChannel);
#else
    NS_IF_RELEASE(mDocumentUrl);
#endif

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocLoader:%p: deleted.\n", this));
#ifndef NECKO
    NS_PRECONDITION((0 == mChildGroupList.Count()), "Document loader has children...");
#endif
}


/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ADDREF(nsDocLoaderImpl);
NS_IMPL_RELEASE(nsDocLoaderImpl);


NS_IMETHODIMP
nsDocLoaderImpl::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER;
    }
    if (aIID.Equals(kIDocumentLoaderIID)) {
        *aInstancePtr = (void*)(nsIDocumentLoader*)this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
#ifndef NECKO
    if (aIID.Equals(kILoadGroupIID)) {
        *aInstancePtr = (void*)(nsILoadGroup*)this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
#else
    if (aIID.Equals(nsCOMTypeInfo<nsILoadGroupListenerFactory>::GetIID())) {
        *aInstancePtr = (void*)(nsILoadGroupListenerFactory*)this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
    if (aIID.Equals(kIStreamObserverIID)) {
        *aInstancePtr = (void*)(nsIStreamObserver*)this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
#endif // NECKO
    if (aIID.Equals(nsCOMTypeInfo<nsISupportsWeakReference>::GetIID())) {
        *aInstancePtr = NS_STATIC_CAST(nsISupportsWeakReference*,this);
        NS_ADDREF_THIS();
        return NS_OK;        
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsDocLoaderImpl::CreateDocumentLoader(nsIDocumentLoader** anInstance)
{
    nsDocLoaderImpl* newLoader = nsnull;
    nsresult rv = NS_OK;

    /* Check for initial error conditions... */
    if (nsnull == anInstance) {
        rv = NS_ERROR_NULL_POINTER;
        goto done;
    }

    NS_NEWXPCOM(newLoader, nsDocLoaderImpl);
    if (nsnull == newLoader) {
        *anInstance = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    rv = newLoader->QueryInterface(kIDocumentLoaderIID, (void**)anInstance);
    if (NS_SUCCEEDED(rv)) {
        // Initialize now that we have a reference
        rv = newLoader->Init();
        if (NS_SUCCEEDED(rv)) {
#ifdef NECKO
            AddChildGroup(newLoader->GetLoadGroup());
#else
            AddChildGroup(newLoader);
#endif
            newLoader->SetParent(this);
        }
    }

  done:
    return rv;
}

nsresult
nsDocLoaderImpl::CreateContentViewer(const char *aCommand,
#ifdef NECKO
                                     nsIChannel* channel,
#else
                                     nsIURI* aURL, 
#endif
                                     const char* aContentType, 
                                     nsIContentViewerContainer* aContainer,
                                     nsISupports* aExtraInfo,
                                     nsIStreamListener** aDocListenerResult,
                                     nsIContentViewer** aDocViewerResult)
{
    // Lookup class-id for the command plus content-type combination
    nsCID cid;
    char id[500];
    PR_snprintf(id, sizeof(id),
                NS_DOCUMENT_LOADER_FACTORY_PROGID_PREFIX "%s/%s",
                aCommand ? aCommand : "view",/* XXX bug! shouldn't b needed!*/
                aContentType);
    nsresult rv = nsComponentManager::ProgIDToCLSID(id, &cid);
    if (NS_FAILED(rv)) {
        return rv;
    }

    // Create an instance of the document-loader-factory object
    nsIDocumentLoaderFactory* factory;
    rv = nsComponentManager::CreateInstance(cid, (nsISupports *)nsnull,
                                            kIDocumentLoaderFactoryIID, 
                                            (void **)&factory);
    if (NS_FAILED(rv)) {
        return rv;
    }

    // Now create an instance of the content viewer
    rv = factory->CreateInstance(aCommand, 
#ifdef NECKO
                                 channel, mLoadGroup,
#else
                                 aURL,
#endif
                                 aContentType,
                                 aContainer,
                                 aExtraInfo, aDocListenerResult,
                                 aDocViewerResult);
    NS_RELEASE(factory);
    return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::LoadDocument(nsIURI * aUri, 
                              const char* aCommand,
                              nsIContentViewerContainer* aContainer,
                              nsIInputStream* aPostDataStream,
                              nsISupports* aExtraInfo,
                              nsIStreamObserver* anObserver,
                              nsLoadFlags aType,
                              const PRUint32 aLocalIP,
                              const PRUnichar* aReferrer)
{
  nsresult rv = NS_OK;
  nsDocumentBindInfo* loader = nsnull;
  if (!aUri)
    return NS_ERROR_NULL_POINTER;

#if defined(DEBUG)
  nsXPIDLCString urlSpec;
  aUri->GetSpec(getter_Copies(urlSpec));
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: LoadDocument(...) called for %s.", 
          this, (const char *) urlSpec));
#endif /* DEBUG */

  /* Check for initial error conditions... */
  if (nsnull == aContainer) {
      rv = NS_ERROR_NULL_POINTER;
      goto done;
  }

  NS_NEWXPCOM(loader, nsDocumentBindInfo);
  if (nsnull == loader) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto done;
  }
  NS_ADDREF(loader);
  loader->Init(this,           // DocLoader
               aCommand,       // Command
               aContainer,     // Viewer Container
               aExtraInfo,     // Extra Info
               anObserver);    // Observer

  /*
   * Set the flag indicating that the document loader is in the process of
   * loading a document.  This flag will remain set until the 
   * OnConnectionsComplete(...) notification is fired for the loader...
   */
  mIsLoadingDocument = PR_TRUE;

  mStreamObserver = dont_QueryInterface(anObserver);

  rv = loader->Bind(aUri, nsnull, aPostDataStream, aReferrer);

done:
  NS_RELEASE(loader);
  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::LoadSubDocument(nsIURI *aUri,
                                 nsISupports* aExtraInfo,
                                 nsLoadFlags aType,
                                 const PRUint32 aLocalIP)
{
  nsresult rv;
  nsDocumentBindInfo* loader = nsnull;
  if (!aUri)
    return NS_ERROR_NULL_POINTER;

#ifdef DEBUG
  char buffer[512];
  nsXPIDLCString uriSpec;
  rv = aUri->GetSpec(getter_Copies(uriSpec));
  if (NS_FAILED(rv)) return rv;

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: LoadSubDocument(...) called for %s.",
          this, (const char *) uriSpec));
#endif /* DEBUG */

  NS_NEWXPCOM(loader, nsDocumentBindInfo);
  if (nsnull == loader) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      return rv;
  }
  NS_ADDREF(loader);
  loader->Init(this,           // DocLoader
               nsnull,         // Command
               nsnull,     // Viewer Container
               aExtraInfo,     // Extra Info
               mStreamObserver);    // Observer


  rv = loader->Bind(aUri, nsnull, nsnull, nsnull);
  NS_RELEASE(loader);
  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::Stop(void)
{
  nsresult rv = NS_OK;
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Stop() called\n", this));
#ifdef NECKO
  rv = mLoadGroup->Cancel();
#else
  m_LoadingDocsList->EnumerateForwards(nsDocLoaderImpl::StopBindInfoEnumerator, nsnull);

  /* 
   * Now the only reference to each nsDocumentBindInfo instance is held by 
   * Netlib via the nsIStreamListener interface...
   * 
   * When each connection is aborted, Netlib will release its reference to 
   * the StreamListener and the DocumentBindInfo object will be deleted...
   */
  m_LoadingDocsList->Clear();

  /*
   * Now Stop() all documents being loaded by child DocumentLoaders...
   */
  mChildGroupList.EnumerateForwards(nsDocLoaderImpl::StopDocLoaderEnumerator, nsnull);

  /* Reset the URL counters... */
  mForegroundURLs = 0;
  mTotalURLs      = 0;
#endif

  /* 
   * Release the Stream Observer...  
   * It will be set on the next LoadDocument(...) 
   */
  mStreamObserver = do_QueryInterface(0);   // to be replaced with null_nsCOMPtr()

  return rv;
}       


NS_IMETHODIMP
nsDocLoaderImpl::IsBusy(PRBool& aResult)
{
#ifdef NECKO
    return mLoadGroup->IsPending(&aResult);
#else
  aResult = PR_FALSE;

  /* If this document loader is busy? */
  if (0 != mForegroundURLs) {
    aResult = PR_TRUE;
  } 
  /* Otherwise, check its child document loaders... */
  else {
    mChildGroupList.EnumerateForwards(nsDocLoaderImpl::IsBusyEnumerator, 
                                      (void*)&aResult);
  }

  return NS_OK;
#endif
}


/*
 * Do not hold refs to the objects in the observer lists.  Observers
 * are expected to remove themselves upon their destruction if they
 * have not removed themselves previously
 */
NS_IMETHODIMP
nsDocLoaderImpl::AddObserver(nsIDocumentLoaderObserver* aObserver)
{
  // Make sure the observer isn't already in the list
  if (mDocObservers.IndexOf(aObserver) == -1) {
    mDocObservers.AppendElement(aObserver);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocLoaderImpl::RemoveObserver(nsIDocumentLoaderObserver* aObserver)
{
  if (PR_TRUE == mDocObservers.RemoveElement(aObserver)) {
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocLoaderImpl::SetContainer(nsIContentViewerContainer* aContainer)
{
  mContainer = aContainer;

  return NS_OK;
}

NS_IMETHODIMP
nsDocLoaderImpl::GetContainer(nsIContentViewerContainer** aResult)
{
  nsresult rv = NS_OK;

  if (nsnull == aResult) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    *aResult = mContainer;
    NS_IF_ADDREF(*aResult);
  }
  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::GetContentViewerContainer(PRUint32 aDocumentID,
                                           nsIContentViewerContainer** aResult)
{
  nsISupports* base = (nsISupports*) aDocumentID;
  nsIDocument* doc;
  nsresult rv;

  rv = base->QueryInterface(kIDocumentIID, (void**)&doc);
  if (NS_SUCCEEDED(rv)) {
    nsIPresShell* pres;
    pres = doc->GetShellAt(0);
    if (nsnull != pres) {
      nsIPresContext* presContext;
      rv = pres->GetPresContext(&presContext);
      if (NS_SUCCEEDED(rv) && nsnull != presContext) {
        nsISupports* supp;
        rv = presContext->GetContainer(&supp);
        if (NS_SUCCEEDED(rv) && nsnull != supp) {          
          rv = supp->QueryInterface(kIContentViewerContainerIID, (void**)aResult);          
          NS_RELEASE(supp);
        }
        NS_RELEASE(presContext);
      }
      NS_RELEASE(pres);
    }
    NS_RELEASE(doc);
  }
  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::Destroy()
{
    Stop();
#if 0
    if (nsnull != mParent) {
#ifdef NECKO
        mParent->RemoveChildGroup(GetLoadGroup());
#else
        mParent->RemoveChildGroup(this);
#endif
        NS_RELEASE(mParent);
    }
#endif
    NS_IF_RELEASE(mDocumentChannel);

    return NS_OK;
}

NS_IMETHODIMP
nsDocLoaderImpl::CreateURL(nsIURI** aInstancePtrResult, 
                           nsIURI* aBaseURL,
                           const nsString& aURLSpec,
                           nsISupports* aContainer)
{
  nsresult rv;
  nsIURI* url = nsnull;

    /* Check for initial error conditions... */
  if (nsnull == aInstancePtrResult) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
#ifdef NECKO
    rv = NS_NewURI(&url, aURLSpec, aBaseURL);
#else
    rv = NS_NewURL(&url, aURLSpec, aBaseURL, aContainer, this);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILoadAttribs> loadAttributes;
      rv = url->GetLoadAttribs(getter_AddRefs(loadAttributes));
      if (loadAttributes)
        loadAttributes->Clone(m_LoadAttrib);
    }
#endif
    *aInstancePtrResult = url;
  }

  return rv;
}


NS_IMETHODIMP
nsDocLoaderImpl::OpenStream(nsIURI *aUrl, nsIStreamListener *aConsumer)
{
  nsresult rv;
  nsDocumentBindInfo* loader = nsnull;

#if defined(DEBUG)
#ifdef NECKO
  char* buffer;
#else
  const char* buffer;
#endif
  aUrl->GetSpec(&buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: OpenStream(...) called for %s.", 
          this, buffer));
#ifdef NECKO
  nsCRT::free(buffer);
#endif
#endif /* DEBUG */

#ifndef NECKO
  nsURLLoadType loadType;
#endif // NECKO

  NS_NEWXPCOM(loader, nsDocumentBindInfo);
  if (nsnull == loader) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }
  NS_ADDREF(loader);
  loader->Init(this,              // DocLoader
               nsnull,            // Command
               mContainer,        // Viewer Container
               nsnull,            // Extra Info
               mStreamObserver);  // Observer

#ifndef NECKO   // done in the load group now
  /* The DocumentBindInfo reference is only held by the Array... */
  m_LoadingDocsList->AppendElement(((nsISupports*)(nsIStreamObserver*)loader));

  /* Update the URL counters... */
  nsILoadAttribs* loadAttributes;

  rv = aUrl->GetLoadAttribs(&loadAttributes);
  loadType = nsURLLoadNormal;
  if (NS_SUCCEEDED(rv)) {
    rv = loadAttributes->GetLoadType(&loadType);
    if (NS_FAILED(rv)) {
      loadType = nsURLLoadNormal;
    }
    NS_RELEASE(loadAttributes);
  }
  if (nsURLLoadBackground != loadType) {
    mForegroundURLs += 1;
  }
  mTotalURLs += 1;
#endif

  rv = loader->Bind(aUrl, aConsumer);
done:
  NS_RELEASE(loader);
  return rv;
}


NS_IMETHODIMP
#ifdef NECKO
nsDocLoaderImpl::GetDefaultLoadAttributes(nsLoadFlags *aLoadAttribs)
#else
nsDocLoaderImpl::GetDefaultLoadAttributes(nsILoadAttribs*& aLoadAttribs)
#endif
{
#ifdef NECKO
    return mLoadGroup->GetDefaultLoadAttributes(aLoadAttribs);
#else
  aLoadAttribs = m_LoadAttrib;
  NS_IF_ADDREF(aLoadAttribs);
#endif

  return NS_OK;;
}


NS_IMETHODIMP
#ifdef NECKO
nsDocLoaderImpl::SetDefaultLoadAttributes(nsLoadFlags aLoadAttribs)
#else
nsDocLoaderImpl::SetDefaultLoadAttributes(nsILoadAttribs*  aLoadAttribs)
#endif
{
#ifdef NECKO
    return mLoadGroup->SetDefaultLoadAttributes(aLoadAttribs);
#else
  m_LoadAttrib->Clone(aLoadAttribs);

  /*
   * Now set the default attributes for all child DocumentLoaders...
   */
  PRInt32 count = mChildGroupList.Count();
  PRInt32 index;

  for (index = 0; index < count; index++) {
    nsILoadGroup* child = (nsILoadGroup*)mChildGroupList.ElementAt(index);
    child->SetDefaultLoadAttributes(m_LoadAttrib);
  }
#endif

  return NS_OK;
}

#ifdef NECKO
NS_IMETHODIMP
nsDocLoaderImpl::OnStartRequest(nsIChannel *channel, nsISupports *ctxt)
{
    // called when the group gets its first element
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    //
    // Only fire an OnStartDocumentLoad(...) if the document loader
    // has initiated a load...  Otherwise, this notification has
    // resulted from a channel being added to the load group.
    //
    if (mIsLoadingDocument) {
        FireOnStartDocumentLoad(this, uri, "load"); // XXX fix command
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocLoaderImpl::OnStopRequest(nsIChannel *channel, nsISupports *ctxt, 
                               nsresult status, const PRUnichar *errorMsg)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Is now idle...\n", this));

    //
    // Only fire the OnEndDocumentLoad(...) if the document loader 
    // has initiated a load...
    //
    if (mIsLoadingDocument) {
        mIsLoadingDocument = PR_FALSE;
        FireOnEndDocumentLoad(this, status);
    }
    return NS_OK;
}
#endif

NS_IMETHODIMP
nsDocLoaderImpl::AddChildGroup(nsILoadGroup* aGroup)
{
#ifdef NECKO
    return mLoadGroup->AddSubGroup(aGroup);
#else
  mChildGroupList.AppendElement(aGroup);
  return NS_OK;
#endif
}


NS_IMETHODIMP
nsDocLoaderImpl::RemoveChildGroup(nsILoadGroup* aGroup)
{
#ifdef NECKO
    return mLoadGroup->RemoveSubGroup(aGroup);
#else
  nsresult rv = NS_OK;

  if (PR_FALSE == mChildGroupList.RemoveElement(aGroup)) {
    rv = NS_ERROR_FAILURE;
  }
  return rv;
#endif
}


#ifdef NECKO
NS_IMETHODIMP
nsDocLoaderImpl::CreateLoadGroupListener(nsIStreamListener *aListener,
                                         nsIStreamListener **aResult)
{
  nsChannelListener *newListener;
 
  NS_NEWXPCOM(newListener, nsChannelListener);
  if (nsnull == newListener) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(newListener);
  newListener->Init(this, aListener);

  *aResult = newListener;

  return NS_OK;
}
#endif

void nsDocLoaderImpl::FireOnStartDocumentLoad(nsDocLoaderImpl* aLoadInitiator,
                                              nsIURI* aURL, 
                                              const char* aCommand)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

#if defined(DEBUG)
#ifdef NECKO
  char *buffer;
#else
  const char *buffer;
#endif

  aURL->GetSpec(&buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Firing OnStartDocumentLoad(...) called for [DocLoader:%p] %s.\n",
          this, aLoadInitiator, buffer));
#ifdef NECKO
  nsCRT::free(buffer);
#endif
#endif /* DEBUG */

  /*
   * First notify any observers that the URL load has begun...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
    observer->OnStartDocumentLoad(aLoadInitiator, aURL, aCommand);
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
    mParent->FireOnStartDocumentLoad(aLoadInitiator, aURL, aCommand);
  }
}

void nsDocLoaderImpl::FireOnEndDocumentLoad(nsDocLoaderImpl* aLoadInitiator,
                                            nsresult aStatus)
									
{
#if defined(DEBUG)
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_OK;
    if (aLoadInitiator->mDocumentChannel)
        rv = aLoadInitiator->mDocumentChannel->GetURI(getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
        char* buffer = nsnull;
        if (uri)
            rv = uri->GetSpec(&buffer);
        if (NS_SUCCEEDED(rv) && buffer != nsnull) {
            PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
                   ("DocLoader:%p: Firing OnEndDocumentLoad(...) called for [DocLoader:%p] %s\n", 
                    this, aLoadInitiator, buffer));
            nsCRT::free(buffer);
        }
    }
#endif /* DEBUG */

    /*
     * First notify any observers that the document load has finished...
     */
    PRInt32 count = mDocObservers.Count();
    PRInt32 index;

    for (index = 0; index < count; index++) {
        nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)
            mDocObservers.ElementAt(index);
        observer->OnEndDocumentLoad(aLoadInitiator, 
#ifdef NECKO
                                    aLoadInitiator->mDocumentChannel,
#else
                                    mDocumentUrl,
#endif
                                    aStatus, observer);
    }
#ifdef NECKO
    /*
     * Next notify the parent...
     */
    if (nsnull != mParent) {
        mParent->FireOnEndDocumentLoad(aLoadInitiator, aStatus);
    }

#else // !NECKO
    /*
     * Finally notify the parent...
     */
    if (nsnull != mParent) {
        mParent->ChildDocLoaderFiredEndDocumentLoad(this, aLoadInitiator, aStatus);
    }
#endif // !NECKO
}

#ifndef NECKO
void
nsDocLoaderImpl::ChildDocLoaderFiredEndDocumentLoad(nsDocLoaderImpl* aChild,
                                                    nsIDocumentLoader* aLoadInitiator,
                                                    nsresult aStatus)
{
    PRBool busy;
    IsBusy(busy);
    if (!busy) {
        // If the parent is no longer busy because a child document
        // loader finished, then its time for the parent to fire its
        // on-end-document-load notification.
        FireOnEndDocumentLoad(aLoadInitiator, aStatus);
    }
}
#endif // !NECKO

void nsDocLoaderImpl::FireOnStartURLLoad(nsDocLoaderImpl* aLoadInitiator,
#ifdef NECKO
                                         nsIChannel* channel,
#else
                                         nsIURI* aURL,
                                         const char* aContentType, 
#endif
                                         nsIContentViewer* aViewer)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that the URL load has begun...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
#ifdef NECKO
    observer->OnStartURLLoad(aLoadInitiator, channel, aViewer);
#else
    observer->OnStartURLLoad(aLoadInitiator, aURL, aContentType, aViewer);
#endif
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
#ifdef NECKO
    mParent->FireOnStartURLLoad(aLoadInitiator, channel, aViewer);
#else
    mParent->FireOnStartURLLoad(aLoadInitiator, aURL, aContentType, aViewer);
#endif
  }
}

void nsDocLoaderImpl::FireOnProgressURLLoad(nsDocLoaderImpl* aLoadInitiator,
#ifdef NECKO
                                            nsIChannel* channel, 
#else
                                            nsIURI* aURL, 
#endif
                                            PRUint32 aProgress,
                                            PRUint32 aProgressMax)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that there is progress information available...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
#ifdef NECKO
    observer->OnProgressURLLoad(aLoadInitiator, channel, aProgress, aProgressMax);
#else
    observer->OnProgressURLLoad(aLoadInitiator, aURL, aProgress, aProgressMax);
#endif
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
#ifdef NECKO
    mParent->FireOnProgressURLLoad(aLoadInitiator, channel, aProgress, aProgressMax);
#else
    mParent->FireOnProgressURLLoad(aLoadInitiator, aURL, aProgress, aProgressMax);
#endif
  }
}

void nsDocLoaderImpl::FireOnStatusURLLoad(nsDocLoaderImpl* aLoadInitiator,
#ifdef NECKO
                                          nsIChannel* channel,
#else
                                          nsIURI* aURL,
#endif
                                          nsString& aMsg)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that there is status text available...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
#ifdef NECKO
    observer->OnStatusURLLoad(aLoadInitiator, channel, aMsg);
#else
    observer->OnStatusURLLoad(aLoadInitiator, aURL, aMsg);
#endif
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
#ifdef NECKO
    mParent->FireOnStatusURLLoad(aLoadInitiator, channel, aMsg);
#else
    mParent->FireOnStatusURLLoad(aLoadInitiator, aURL, aMsg);
#endif
  }
}

#ifdef NECKO
void nsDocLoaderImpl::FireOnEndURLLoad(nsDocLoaderImpl* aLoadInitiator,
                                       nsIChannel* channel, nsresult aStatus)
#else
void nsDocLoaderImpl::FireOnEndURLLoad(nsIDocumentLoader* aLoadInitiator,
                                       nsIURI* aURL, PRInt32 aStatus)
#endif
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that the URL load has begun...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
#ifdef NECKO
    observer->OnEndURLLoad(aLoadInitiator, channel, aStatus);
#else
    observer->OnEndURLLoad(aLoadInitiator, aURL, aStatus);
#endif
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
#ifdef NECKO
    mParent->FireOnEndURLLoad(aLoadInitiator, channel, aStatus);
#else
    mParent->FireOnEndURLLoad(aLoadInitiator, aURL, aStatus);
#endif
  }
}



#ifdef NECKO
nsresult nsDocLoaderImpl::LoadURLComplete(nsIChannel* channel, nsISupports* ctxt,
                                          nsISupports* aBindInfo, nsresult aStatus,
                                          const PRUnichar* aMsg)
#else
nsresult nsDocLoaderImpl::LoadURLComplete(nsIURI* aURL, nsISupports* aBindInfo, PRInt32 aStatus)
#endif
{
    /*
     * If the entry is not found in the list, then it must have been cancelled
     * via Stop(...). So ignore just it... 
     */
#ifdef NECKO
#if defined(DEBUG)
    nsCOMPtr<nsIURI> uri;
    nsresult rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
        char* buffer;
        rv = uri->GetSpec(&buffer);
        if (NS_SUCCEEDED(rv)) {
            PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
                   ("DocLoader:%p: LoadURLComplete(...) called for %s\n", 
                    this, buffer));
            nsCRT::free(buffer);
        }
    }
#endif /* DEBUG */
#else // !NECKO
    PRBool isForegroundURL = PR_FALSE;

    PRBool removed = m_LoadingDocsList->RemoveElement(aBindInfo);
    if (removed) {
        nsILoadAttribs* loadAttributes;
        nsURLLoadType loadType = nsURLLoadNormal;

        nsresult rv = aURL->GetLoadAttribs(&loadAttributes);
        if (NS_SUCCEEDED(rv) && loadAttributes) {
            rv = loadAttributes->GetLoadType(&loadType);
            if (NS_FAILED(rv)) {
                loadType = nsURLLoadNormal;
            }
            NS_RELEASE(loadAttributes);
        }
        if (nsURLLoadBackground != loadType) {
            mForegroundURLs--;
            isForegroundURL = PR_TRUE;
        }
        mTotalURLs -= 1;

        NS_ASSERTION(mTotalURLs >= mForegroundURLs,
                     "Foreground URL count is wrong.");

#if defined(DEBUG)
        const char* buffer;

        aURL->GetSpec(&buffer);
        PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
               ("DocLoader:%p: LoadURLComplete(...) called for %s; Foreground URLs: %d; Total URLs: %d\n", 
                this, buffer, mForegroundURLs, mTotalURLs));
#endif /* DEBUG */
    }
#endif // !NECKO

    /*
     * Fire the OnEndURLLoad notification to any observers...
     */
#ifdef NECKO
///    FireOnEndURLLoad((nsIDocumentLoader *) this, channel, aStatus);
#else
    FireOnEndURLLoad((nsIDocumentLoader *) this, aURL, aStatus);
#endif

#ifdef NECKO
///    return GetLoadGroup()->RemoveChannel(channel, ctxt, aStatus, aMsg);
  return NS_OK;
#else   // !NECKO
    /*
     * Fire the OnEndDocumentLoad notification to any observers...
     */
    PRBool busy;
    IsBusy(busy);
    if (isForegroundURL && !busy) {
#if defined(DEBUG)
        const char* buffer;
        nsIURI* uri = mDocumentUrl;

        uri->GetSpec(&buffer);
        PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
               ("DocLoader:%p: OnEndDocumentLoad(...) called for %s.\n",
                this, buffer));
#endif /* DEBUG */

        FireOnEndDocumentLoad((nsIDocumentLoader *) this, aStatus);
    }
    return NS_OK;
#endif // !NECKO
}

void nsDocLoaderImpl::SetParent(nsDocLoaderImpl* aParent)
{
  NS_IF_RELEASE(mParent);
  mParent = aParent;
  NS_IF_ADDREF(mParent);
}

#ifdef NECKO
void nsDocLoaderImpl::SetDocumentChannel(nsIChannel* channel)
{
  NS_IF_RELEASE(mDocumentChannel);
  mDocumentChannel = channel;
  NS_IF_ADDREF(mDocumentChannel);
}
#else
void nsDocLoaderImpl::SetDocumentUrl(nsIURI* aUrl)
{
  NS_IF_RELEASE(mDocumentUrl);
  mDocumentUrl = aUrl;
  NS_IF_ADDREF(mDocumentUrl);
}
#endif

#ifndef NECKO
PRBool nsDocLoaderImpl::StopBindInfoEnumerator(nsISupports* aElement, void* aData)
{
    nsresult rv;
    nsDocumentBindInfo* bindInfo;

    rv = aElement->QueryInterface(kDocumentBindInfoIID, (void**)&bindInfo);
    if (NS_SUCCEEDED(rv)) {
        bindInfo->Stop();
        NS_RELEASE(bindInfo);
    }

    return PR_TRUE;
}


PRBool nsDocLoaderImpl::StopDocLoaderEnumerator(void* aElement, void* aData)
{
  nsresult rv;
  nsIDocumentLoader* docLoader;
    
  rv = ((nsISupports*)aElement)->QueryInterface(kIDocumentLoaderIID, (void**)&docLoader);
  if (NS_SUCCEEDED(rv)) {
    docLoader->Stop();
    NS_RELEASE(docLoader);
  }

  return PR_TRUE;
}


PRBool nsDocLoaderImpl::IsBusyEnumerator(void* aElement, void* aData)
{
  nsresult rv;
  nsIDocumentLoader* docLoader;
  PRBool* result = (PRBool*)aData;
    
  rv = ((nsISupports*)aElement)->QueryInterface(kIDocumentLoaderIID, (void**)&docLoader);
  if (NS_SUCCEEDED(rv)) {
    docLoader->IsBusy(*result);
    NS_RELEASE(docLoader);
  }

  return !(*result);
}
#endif

/****************************************************************************
 * nsDocumentBindInfo implementation...
 ****************************************************************************/

nsDocumentBindInfo::nsDocumentBindInfo()
{
    NS_INIT_REFCNT();

    m_Command = nsnull;
#ifndef NECKO
    m_Url = nsnull;
#endif
    m_Container = nsnull;
    m_ExtraInfo = nsnull;
    m_Observer = nsnull;
    m_NextStream = nsnull;
    m_DocLoader = nsnull;
    mStatus = NS_OK;
}

nsresult
nsDocumentBindInfo::Init(nsDocLoaderImpl* aDocLoader,
                         const char *aCommand, 
                         nsIContentViewerContainer* aContainer,
                         nsISupports* aExtraInfo,
                         nsIStreamObserver* anObserver)
{
#ifndef NECKO
    m_Url        = nsnull;
#endif
    m_NextStream = nsnull;
    m_Command    = (nsnull != aCommand) ? PL_strdup(aCommand) : nsnull;

    m_DocLoader = aDocLoader;
    NS_ADDREF(m_DocLoader);

    m_Container = aContainer;
    NS_IF_ADDREF(m_Container);

    m_Observer = anObserver;
    NS_IF_ADDREF(m_Observer);

    m_ExtraInfo = aExtraInfo;
    NS_IF_ADDREF(m_ExtraInfo);

    mStatus = NS_OK;
    return NS_OK;
}

nsDocumentBindInfo::~nsDocumentBindInfo()
{
    if (m_Command) {
        PR_Free(m_Command);
    }
    m_Command = nsnull;

    NS_RELEASE   (m_DocLoader);
#ifndef NECKO
    NS_IF_RELEASE(m_Url);
#endif
    NS_IF_RELEASE(m_NextStream);
    NS_IF_RELEASE(m_Container);
    NS_IF_RELEASE(m_Observer);
    NS_IF_RELEASE(m_ExtraInfo);
}

/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ADDREF(nsDocumentBindInfo);
NS_IMPL_RELEASE(nsDocumentBindInfo);

nsresult
nsDocumentBindInfo::QueryInterface(const nsIID& aIID,
                                   void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  *aInstancePtrResult = NULL;

  if (aIID.Equals(kIStreamObserverIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamObserver*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kIStreamListenerIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamListener*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kDocumentBindInfoIID)) {
    *aInstancePtrResult = (void*) this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
#ifndef NECKO
  if (aIID.Equals(kRefreshURLIID)) {
    *aInstancePtrResult = (void*) ((nsIRefreshUrl*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
#endif
  return NS_NOINTERFACE;
}

class nsWebShellEventSinkGetter : public nsIEventSinkGetter {
public:
  NS_DECL_ISUPPORTS

  nsWebShellEventSinkGetter(nsIWebShell *aWebShell) {
    NS_INIT_REFCNT();
    mWebShell = aWebShell;
    NS_IF_ADDREF(mWebShell);
  }

  virtual ~nsWebShellEventSinkGetter() {
    NS_IF_RELEASE(mWebShell);
  }

  NS_IMETHOD GetEventSink(const char* aVerb, const nsIID& anIID, nsISupports** aSink) {
    if (mWebShell) {
      if (nsCRT::strcmp(aVerb, "load") == 0) {
        return mWebShell->QueryInterface(anIID, (void**)aSink);
      }
    }
    return NS_ERROR_FAILURE;
  }
private:
  nsIWebShell* mWebShell;
};

// XXXbe second arg is unnecessary given first; hidden static IID in macro... scc help!
NS_IMPL_ISUPPORTS(nsWebShellEventSinkGetter, nsCOMTypeInfo<nsIEventSinkGetter>::GetIID())

nsresult nsDocumentBindInfo::Bind(nsIURI* aURL, nsIStreamListener* aListener, nsIInputStream *postDataStream, const PRUnichar* aReferrer)
{
  nsresult rv = NS_OK;

  //  m_DocLoader->FireOnStartDocumentLoad(aURL, m_Command);

  /* Set up the stream listener (if provided)... */
  if (nsnull != aListener) {
    m_NextStream = aListener;
    NS_ADDREF(m_NextStream);
  }

  nsILoadGroup* loadGroup = nsnull;
  if (m_DocLoader) {
      loadGroup = m_DocLoader->GetLoadGroup();
  }

  // XXXbe this knows that m_Container implements nsIWebShell
  nsCOMPtr<nsIWebShell> webShell = do_QueryInterface(m_Container);
  nsCOMPtr<nsIEventSinkGetter> eventSinkGetter = new nsWebShellEventSinkGetter(webShell);
  if (!eventSinkGetter)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIChannel> channel;
  rv = NS_OpenURI(getter_AddRefs(channel), aURL, loadGroup, eventSinkGetter);
  if (NS_FAILED(rv)) return rv;

  if (postDataStream || aReferrer)
  {
      nsCOMPtr<nsIHTTPChannel> httpChannel(do_QueryInterface(channel));
      if (httpChannel)
      {
          if (postDataStream) {
              httpChannel->SetRequestMethod(HM_POST);
              httpChannel->SetPostDataStream(postDataStream);
          }
          if (aReferrer) {
              // Referer - misspelled, but per the HTTP spec
              nsCAutoString str = aReferrer;
              nsCOMPtr<nsIAtom> key = NS_NewAtom("referer");
              httpChannel->SetRequestHeader(key, str.GetBuffer());
          }
      }
  }
  m_DocLoader->SetDocumentChannel(channel);

///  rv = loadGroup->AddChannel(channel, nsnull);
///  if (NS_FAILED(rv)) return rv;
  rv = channel->AsyncRead(0, -1, nsnull, this);

  return rv;
}

#ifdef NECKO
nsresult nsDocumentBindInfo::Bind(nsIStreamListener *aListener)
{
  m_NextStream = aListener;
  NS_IF_ADDREF(m_NextStream);

  return NS_OK;
}

#endif // NECKO

#ifndef NECKO
nsresult nsDocumentBindInfo::Stop(void)
{
  nsresult rv;
  if (m_Url == nsnull) return NS_OK;

#if defined(DEBUG)
#ifdef NECKO
  char* spec;
#else
  const char* spec;
#endif
  rv = m_Url->GetSpec(&spec);
  if (NS_SUCCEEDED(rv)) {
      PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
             ("DocumentBindInfo:%p: Stop(...) called for %s.\n", this, spec));
  }
#ifdef NECKO
  nsCRT::free(spec);
#endif
#endif /* DEBUG */

  /* 
   * Mark the IStreamListener as being aborted...  If more data is pushed
   * down the stream, the connection will be aborted...
   */
  mStatus = NS_BINDING_ABORTED;

  /* Stop the URL binding process... */
#ifndef NECKO
  nsINetService* inet;
  rv = nsServiceManager::GetService(kNetServiceCID,
                                    kINetServiceIID,
                                    (nsISupports **)&inet);
  if (NS_SUCCEEDED(rv)) {
    rv = inet->InterruptStream(m_Url);
    nsServiceManager::ReleaseService(kNetServiceCID, inet);
  }
#else
  // XXX NECKO
  // need to interrupt the load;
#endif // NECKO

  return rv;
}

NS_METHOD nsDocumentBindInfo::GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo)
{
    nsresult rv = NS_OK;

    NS_PRECONDITION(nsnull !=m_NextStream, "DocLoader: No stream for document");

    if (nsnull != m_NextStream) {
        rv = m_NextStream->GetBindInfo(aURL, aInfo);
    }

    return rv;
}
#endif

#ifdef NECKO
NS_METHOD nsDocumentBindInfo::OnProgress(nsIChannel* channel, nsISupports *ctxt,
                                         PRUint32 aProgress, PRUint32 aProgressMax)
#else
NS_METHOD nsDocumentBindInfo::OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax)
#endif
{
    nsresult rv = NS_OK;

#ifdef NECKO
    nsCOMPtr<nsIURI> aURL;
    rv = channel->GetURI(getter_AddRefs(aURL));
    if (NS_FAILED(rv)) return rv;
#endif

#if defined(DEBUG)
#ifdef NECKO
    char* spec;
#else
    const char* spec;
#endif
    (void)aURL->GetSpec(&spec);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnProgress(...) called for %s.  Progress: %d.  ProgressMax: %d\n", 
            this, spec, aProgress, aProgressMax));
#ifdef NECKO
    nsCRT::free(spec);
#endif
#endif /* DEBUG */

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
#ifdef NECKO
//        rv = m_NextStream->OnProgress(channel, aProgress, aProgressMax);
        NS_ASSERTION(0, "help");
#else
        rv = m_NextStream->OnProgress(aURL, aProgress, aProgressMax);
#endif
    }

    /* Pass the notification out to any observers... */
#ifdef NECKO
    m_DocLoader->FireOnProgressURLLoad(m_DocLoader, channel, aProgress, aProgressMax);
#else
    m_DocLoader->FireOnProgressURLLoad(m_DocLoader, aURL, aProgress, aProgressMax);
#endif

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
#ifdef NECKO
//        (void) m_Observer->OnProgress(channel, aProgress, aProgressMax);
        NS_ASSERTION(0, "help");
#else
        (void) m_Observer->OnProgress(aURL, aProgress, aProgressMax);
#endif
    }

    return rv;
}


#ifdef NECKO
NS_METHOD nsDocumentBindInfo::OnStatus(nsIChannel* channel, nsISupports *ctxt, const PRUnichar *aMsg)
#else
NS_METHOD nsDocumentBindInfo::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
#endif
{
    nsresult rv = NS_OK;

#ifdef NECKO
    nsCOMPtr<nsIURI> aURL;
    rv = channel->GetURI(getter_AddRefs(aURL));
    if (NS_FAILED(rv)) return rv;
#endif

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
#ifdef NECKO
//        rv = m_NextStream->OnStatus(ctxt, aMsg);
        NS_ASSERTION(0, "help");
#else
        rv = m_NextStream->OnStatus(aURL, aMsg);
#endif
    }

    /* Pass the notification out to any observers... */
    nsString msgStr(aMsg);
#ifdef NECKO
    m_DocLoader->FireOnStatusURLLoad(m_DocLoader, channel, msgStr);
#else
    m_DocLoader->FireOnStatusURLLoad(m_DocLoader, aURL, msgStr);
#endif

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
#ifdef NECKO
//        (void) m_Observer->OnStatus(ctxt, aMsg);
        NS_ASSERTION(0, "help");
#else
        (void) m_Observer->OnStatus(aURL, aMsg);
#endif
    }

    return rv;
}

NS_IMETHODIMP
#ifdef NECKO
nsDocumentBindInfo::OnStartRequest(nsIChannel* channel, nsISupports *ctxt)
#else
nsDocumentBindInfo::OnStartRequest(nsIURI* aURL, const char *aContentType)
#endif
{
    nsresult rv = NS_OK;
    nsIContentViewer* viewer = nsnull;

#ifdef NECKO
    nsCOMPtr<nsIURI> aURL;
    rv = channel->GetURI(getter_AddRefs(aURL));
    if (NS_FAILED(rv)) return rv;
    char* aContentType = nsnull;
    rv = channel->GetContentType(&aContentType);
    if (NS_FAILED(rv)) return rv;
#endif // NECKO

#if defined(DEBUG)
#ifdef NECKO
    char* spec;
#else
    const char* spec;
#endif // NECKO
    (void)aURL->GetSpec(&spec);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p OnStartRequest(...) called for %s.  Content-type is %s\n",
            this, spec, aContentType));
#ifdef NECKO
    nsCRT::free(spec);
#endif // NECKO
#endif /* DEBUG */

    /* If the binding has been canceled via Stop() then abort the load... */
    if (NS_BINDING_ABORTED == mStatus) {
        rv = NS_BINDING_ABORTED;
        goto done;
    }

    if (nsnull == m_NextStream) {
        /*
         * Now that the content type is available, create a document
         * (and viewer) of the appropriate type...
         */
        if (m_DocLoader) {

            rv = m_DocLoader->CreateContentViewer(m_Command, 
#ifdef NECKO
                                                  channel,
#else
                                                  m_Url,
#endif
                                                  aContentType, 
                                                  m_Container,
                                                  m_ExtraInfo,
                                                  &m_NextStream, 
                                                  &viewer);
        } else {
            rv = NS_ERROR_NULL_POINTER;
        }

        if (NS_FAILED(rv)) {
            printf("DocLoaderFactory: Unable to create ContentViewer for command=%s, content-type=%s\n", m_Command ? m_Command : "(null)", aContentType);
            if ( m_Container ) {
                // Give content container a chance to do something with this URL.
#ifndef NECKO
                rv = m_Container->HandleUnknownContentType( (nsIDocumentLoader*) m_DocLoader, aURL, aContentType, m_Command );
#else
                rv = m_Container->HandleUnknownContentType( (nsIDocumentLoader*) m_DocLoader, channel, aContentType, m_Command );
#endif // NECKO
            }
            // Stop the binding.
            // This crashes on Unix/Mac... Stop();
            goto done;
        }

        /*
         * Give the document container the new viewer...
         */
        if (m_Container) {
            viewer->SetContainer(m_Container);

            rv = m_Container->Embed(viewer, m_Command, m_ExtraInfo);
            if (NS_FAILED(rv)) {
                goto done;
            }
        }

    }

    /*
     * Pass the OnStartRequest(...) notification out to the document 
     * IStreamListener.
     */
    if (nsnull != m_NextStream) {
#ifdef NECKO
        rv = m_NextStream->OnStartRequest(channel, ctxt);
#else
        rv = m_NextStream->OnStartRequest(aURL, aContentType);
#endif
    }

    /*
     * Notify the DocumentLoadObserver(s) 
     */
    if ((nsnull == viewer) && (nsnull != m_Container)) {
        m_Container->GetContentViewer(&viewer);
    }
#ifdef NECKO
///    m_DocLoader->FireOnStartURLLoad((nsIDocumentLoader *)m_DocLoader, channel, viewer);
#else
    m_DocLoader->FireOnStartURLLoad((nsIDocumentLoader *)m_DocLoader, m_Url, aContentType, viewer);
#endif

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
#ifdef NECKO
        nsresult rv2 = m_Observer->OnStartRequest(channel, ctxt);
#else
        nsresult rv2 = m_Observer->OnStartRequest(aURL, aContentType);
#endif
        if (NS_SUCCEEDED(rv))
        	rv = rv2;
    }

  done:
    NS_IF_RELEASE(viewer);
#ifdef NECKO
    nsCRT::free(aContentType);
#endif
    return rv;
}


#ifdef NECKO
NS_METHOD nsDocumentBindInfo::OnDataAvailable(nsIChannel* channel, nsISupports *ctxt,
                                              nsIInputStream *aStream, 
                                              PRUint32 sourceOffset, 
                                              PRUint32 aLength)
#else
NS_METHOD nsDocumentBindInfo::OnDataAvailable(nsIURI* aURL, nsIInputStream *aStream, PRUint32 aLength)
#endif
{
    nsresult rv = NS_OK;

#ifdef NECKO
    nsCOMPtr<nsIURI> aURL;
    rv = channel->GetURI(getter_AddRefs(aURL));
    if (NS_FAILED(rv)) return rv;
#endif

#if defined(DEBUG)
#ifdef NECKO
    char* spec;
#else
    const char* spec;
#endif
    (void)aURL->GetSpec(&spec);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnDataAvailable(...) called for %s.  Bytes available: %d.\n", 
            this, spec, aLength));
#ifdef NECKO
    nsCRT::free(spec);
#endif
#endif /* DEBUG */

    /* If the binding has been canceled via Stop() then abort the load... */
    if (NS_BINDING_ABORTED == mStatus) {
        rv = NS_BINDING_ABORTED;
        goto done;
    }

    if (nsnull != m_NextStream) {
       /*
        * Bump the refcount in case the stream gets destroyed while the data
        * is being processed...  If Stop(...) is called the stream could be
        * freed prematurely :-(
        *
        * Currently this can happen if javascript loads a new URL 
        * (via nsIWebShell::LoadURL) during the parse phase... 
        */
        nsIStreamListener* listener = m_NextStream;

        NS_ADDREF(listener);
#ifdef NECKO
        rv = listener->OnDataAvailable(channel, ctxt, aStream, sourceOffset, aLength);
#else
        rv = listener->OnDataAvailable(aURL, aStream, aLength);
#endif
        NS_RELEASE(listener);
    } else {
      rv = NS_BINDING_FAILED;
    }

done:
    return rv;
}


#ifdef NECKO
NS_METHOD nsDocumentBindInfo::OnStopRequest(nsIChannel* channel, nsISupports *ctxt,
                                            nsresult aStatus, const PRUnichar *aMsg)
#else
NS_METHOD nsDocumentBindInfo::OnStopRequest(nsIURI* aURL, PRInt32 aStatus, const PRUnichar* aMsg)
#endif
{
    nsresult rv = NS_OK;

#ifdef NECKO
    nsCOMPtr<nsIURI> aURL;
    rv = channel->GetURI(getter_AddRefs(aURL));
    if (NS_FAILED(rv)) return rv;
#endif

#if defined(DEBUG)
#ifdef NECKO
    char* spec;
#else
    const char* spec;
#endif
    (void)aURL->GetSpec(&spec);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnStopRequest(...) called for %s.  Status: %d.\n", 
            this, spec, aStatus));
#ifdef NECKO
    nsCRT::free(spec);
#endif
#endif // DEBUG

    if (NS_FAILED(aStatus)) {
#ifdef NECKO
        char *url;
        if (NS_SUCCEEDED(rv)) 
            aURL->GetSpec(&url);
        else
            url = nsCRT::strdup("");
#else
        const char *url;
        if (nsnull != aURL) 
            aURL->GetSpec(&url);
        else
            url = "";      
#endif // NECKO

#ifdef NECKO
        if (aStatus == NS_ERROR_UNKNOWN_HOST)
            printf("Error: Unknown host: %s\n", url);
        else if (aStatus == NS_ERROR_MALFORMED_URI)
            printf("Error: Malformed URI: %s\n", url);
        else if (NS_FAILED(aStatus))
            printf("Error: Can't load: %s (%x)\n", url, aStatus);
        nsCRT::free(url);
#endif
    }

    if (nsnull != m_NextStream) {
#ifdef NECKO
        rv = m_NextStream->OnStopRequest(channel, ctxt, aStatus, aMsg);
#else
        rv = m_NextStream->OnStopRequest(aURL, aStatus, aMsg);
#endif
    }

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
#ifdef NECKO
        (void) m_Observer->OnStopRequest(channel, ctxt, aStatus, aMsg);
#else
        (void) m_Observer->OnStopRequest(aURL, aStatus, aMsg);
#endif
    }

    /*
     * The stream is complete...  Tell the DocumentLoader to release us...
     */
#ifdef NECKO
    rv = m_DocLoader->LoadURLComplete(channel, ctxt, (nsIStreamListener *)this,
                                      aStatus, aMsg);
#else
    rv = m_DocLoader->LoadURLComplete(aURL, (nsIStreamListener *)this, aStatus);
#endif
    NS_IF_RELEASE(m_NextStream);

    return rv;
}

NS_METHOD
nsDocumentBindInfo::RefreshURL(nsIURI* aURL, PRInt32 millis, PRBool repeat)
{
    if (nsnull != m_Container) {
        nsresult rv = NS_OK;
#ifdef NECKO
        NS_ASSERTION(0, "help");
#else
        nsIRefreshUrl* refresher = nsnull;

        /* Delegate the actual refresh call up-to the container. */
        rv = m_Container->QueryInterface(kRefreshURLIID, (void**)&refresher);

        if (NS_FAILED(rv)) {
            return PR_FALSE;
        }
        rv = refresher->RefreshURL(aURL, millis, repeat);
        NS_RELEASE(refresher);
#endif
        return rv;
    }
    return PR_FALSE;
}

NS_METHOD
nsDocumentBindInfo::CancelRefreshURLTimers(void)
{
    if (nsnull != m_Container) {
        nsresult rv = NS_OK;
#ifdef NECKO
        NS_ASSERTION(0, "help");
#else
        nsIRefreshUrl* refresher = nsnull;

        /* Delegate the actual cancel call up-to the container. */
        rv = m_Container->QueryInterface(kRefreshURLIID, (void**)&refresher);

        if (NS_FAILED(rv)) {
            return PR_FALSE;
        }
        rv = refresher->CancelRefreshURLTimers();
        NS_RELEASE(refresher);
#endif
        return rv;
    }
    return PR_FALSE;
}



#ifdef NECKO

NS_IMPL_ISUPPORTS(nsChannelListener, nsCOMTypeInfo<nsIStreamListener>::GetIID());

nsChannelListener::nsChannelListener()
{
  NS_INIT_REFCNT();

  mDocLoader = nsnull;
}

nsresult nsChannelListener::Init(nsDocLoaderImpl *aDocLoader, 
                                 nsIStreamListener *aListener)
{
  mDocLoader = aDocLoader;
  NS_ADDREF(mDocLoader);

  mNextListener = aListener;

  return NS_OK;
}

nsChannelListener::~nsChannelListener()
{
  NS_RELEASE(mDocLoader);
  mNextListener = null_nsCOMPtr();
}

NS_IMETHODIMP
nsChannelListener::OnStartRequest(nsIChannel *aChannel, nsISupports *aContext)
{
  nsresult rv;
  nsCOMPtr<nsIContentViewerContainer> container;
  nsCOMPtr<nsIContentViewer> viewer;

  ///////////////////////////////
  // STREAM CONVERTERS
  ///////////////////////////////

  nsXPIDLCString contentType;
  rv = aChannel->GetContentType(getter_Copies(contentType));
  if (NS_FAILED(rv)) return rv;

  PRBool conversionRequired = PR_FALSE;
  nsAutoString from, to;

  // Let's shanghai this channelListener's mNextListener if we want to convert the stream.
  if (!PL_strcmp(contentType, "message/rfc822"))
  {
	  from = "message/rfc822";
	  to = "text/xul";
	  conversionRequired = PR_TRUE;
  }
  else if (!PL_strcmp(contentType, "multipart/x-mixed-replace")) 
  {
	  from = "multipart/x-mixed-replace";
	  to = "text/html";
	  conversionRequired = PR_TRUE;
  }

  if (conversionRequired)
  {

	NS_WITH_SERVICE(nsIStreamConverterService, StreamConvService, kStreamConverterServiceCID, &rv);
	if (NS_FAILED(rv)) return rv;

    // The following call binds this channelListener's mNextListener (typically
    // the nsDocumentBindInfo) to the underlying stream converter, and returns
    // the underlying stream converter which we then set to be this channelListener's
    // mNextListener. This effectively nestles the stream converter down right
    // in between the raw stream and the final listener.
    nsIStreamListener *converterListener = nsnull;
    rv = StreamConvService->AsyncConvertData(from.GetUnicode(), to.GetUnicode(), mNextListener, aChannel,
                                             &converterListener);
    mNextListener = converterListener;
  }

  //////////////////////////////
  // END STREAM CONVERTERS
  //////////////////////////////
  
  // Pass the notification to the next listener...
  rv = mNextListener->OnStartRequest(aChannel, aContext);


  //
  // Notify the document loader...
  mDocLoader->GetContainer(getter_AddRefs(container));
  if (container) {
    container->GetContentViewer(getter_AddRefs(viewer));
  }

  mDocLoader->FireOnStartURLLoad(mDocLoader, 
                                 aChannel, 
                                 viewer);

  return rv;
}

NS_IMETHODIMP
nsChannelListener::OnStopRequest(nsIChannel *aChannel, nsISupports *aContext,
                                 nsresult aStatus, const PRUnichar *aMsg)
{
  nsresult rv;

  rv = mNextListener->OnStopRequest(aChannel, aContext, aStatus, aMsg);

  mDocLoader->FireOnEndURLLoad(mDocLoader, 
                               aChannel, 
                               aStatus);

  return rv;
}

NS_IMETHODIMP
nsChannelListener::OnDataAvailable(nsIChannel *aChannel, nsISupports *aContext,
                                   nsIInputStream *aInStream, PRUint32 aOffset,
                                   PRUint32 aCount)
{
  return mNextListener->OnDataAvailable(aChannel, aContext, aInStream, 
                                        aOffset, aCount);
}


#endif // NECKO



/*******************************************
 *  nsDocLoaderServiceFactory
 *******************************************/
static nsDocLoaderImpl* gServiceInstance = nsnull;

NS_IMETHODIMP
nsDocLoaderImpl::Create(nsISupports *aOuter, const nsIID &aIID, void **aResult)
{
  nsresult rv;
  nsDocLoaderImpl* inst;

  // Parameter validation...
  if (NULL == aResult) {
    rv = NS_ERROR_NULL_POINTER;
    goto done;
  }
  // Do not support aggregatable components...
  *aResult = NULL;
  if (NULL != aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    goto done;
  }

  if (NULL == gServiceInstance) {
    // Create a new instance of the component...
    NS_NEWXPCOM(gServiceInstance, nsDocLoaderImpl);
    if (NULL == gServiceInstance) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      goto done;
    }
  }

  // If the QI fails, the component will be destroyed...
  //
  // Use a local copy so the NS_RELEASE() will not null the global
  // pointer...
  inst = gServiceInstance;

  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  if (NS_SUCCEEDED(rv)) {
      rv = inst->Init();
  }
  NS_RELEASE(inst);

done:
  return rv;
}

// Entry point to create nsDocLoaderService factory instances...

nsresult NS_NewDocLoaderServiceFactory(nsIFactory** aResult)
{
  nsresult rv = NS_OK;
  nsIGenericFactory* factory;
  rv = nsComponentManager::CreateInstance(kGenericFactoryCID, nsnull, 
                                          nsIGenericFactory::GetIID(),
                                          (void**)&factory);
  if (NS_FAILED(rv)) return rv;

  rv = factory->SetConstructor(nsDocLoaderImpl::Create);
  if (NS_FAILED(rv)) {
      NS_RELEASE(factory);
      return rv;
  }

  *aResult = factory;
  return rv;
}

