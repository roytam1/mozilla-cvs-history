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
#include "prmem.h"
#include "plstr.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsIStreamListener.h"
#include "nsIPostToServer.h"
#include "nsIFactory.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIRefreshUrl.h"
#include "nsITimer.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsVoidArray.h"
#include "nsIHttpURL.h"
#include "nsIServiceManager.h"
#ifndef NECKO
#include "nsIURLGroup.h"
#include "nsILoadAttribs.h"
#include "nsINetService.h"
#else
#include "nsIURL.h"
#include "nsIEventQueueService.h"
#include "nsIIOService.h"
#include "nsIChannel.h"
#endif // NECKO
#include "nsIGenericFactory.h"
#include "nsIStreamLoadableDocument.h"
#include "nsCOMPtr.h"
#include "nsCom.h"

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
static NS_DEFINE_IID(kRefreshURLIID,               NS_IREFRESHURL_IID);
static NS_DEFINE_IID(kHTTPURLIID,                  NS_IHTTPURL_IID);
static NS_DEFINE_IID(kISupportsIID,                NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIDocumentIID,                NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIStreamListenerIID,          NS_ISTREAMLISTENER_IID);
#ifndef NECKO
static NS_DEFINE_IID(kILoadGroupIID,                NS_ILOADGROUP_IID);
static NS_DEFINE_IID(kINetServiceIID,              NS_INETSERVICE_IID);
static NS_DEFINE_IID(kNetServiceCID,             NS_NETSERVICE_CID);
#else
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIIOServiceIID, NS_IIOSERVICE_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#endif // NECKO
static NS_DEFINE_IID(kIContentViewerContainerIID,  NS_ICONTENT_VIEWER_CONTAINER_IID);
static NS_DEFINE_CID(kGenericFactoryCID,           NS_GENERICFACTORY_CID);

/* Forward declarations.... */
class nsDocLoaderImpl;

/* 
 * The nsDocumentBindInfo contains the state required when a single document
 * is being loaded...  Each instance remains alive until its target URL has 
 * been loaded (or aborted).
 *
 * The Document Loader maintains a list of nsDocumentBindInfo instances which 
 * represents the set of documents actively being loaded...
 */
class nsDocumentBindInfo : public nsIStreamListener, 
                           public nsIRefreshUrl
{
public:
    nsDocumentBindInfo();

    nsresult Init(nsDocLoaderImpl* aDocLoader,
                  const char *aCommand, 
                  nsIContentViewerContainer* aContainer,
                  nsISupports* aExtraInfo,
                  nsIStreamObserver* anObserver);

    NS_DECL_ISUPPORTS

    nsresult Bind(const nsString& aURLSpec, 
                  nsIPostData* aPostData,
                  nsIStreamListener* aListener);

    nsresult Bind(nsIURI* aURL, nsIStreamListener* aListener);

    nsresult Stop(void);

    /* nsIStreamListener interface methods... */
    NS_IMETHOD GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* aInfo);
    NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax);
    NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
    NS_IMETHOD OnStartBinding(nsIURI* aURL, const char *aContentType);
    NS_IMETHOD OnDataAvailable(nsIURI* aURL, nsIInputStream *aStream, PRUint32 aLength);
    NS_IMETHOD OnStopBinding(nsIURI* aURL, nsresult aStatus, const PRUnichar* aMsg);

    nsresult GetStatus(void) { return mStatus; }

    /* nsIRefreshURL interface methods... */
    NS_IMETHOD RefreshURL(nsIURI* aURL, PRInt32 millis, PRBool repeat);
    NS_IMETHOD CancelRefreshURLTimers(void);

protected:
    virtual ~nsDocumentBindInfo();

protected:
    char*               m_Command;
    nsIURI*             m_Url;
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

#ifndef NECKO
class nsDocLoaderImpl : public nsIDocumentLoader, public nsILoadGroup
#else
class nsDocLoaderImpl : public nsIDocumentLoader
#endif // NECKO
{
public:

    nsDocLoaderImpl();

    // for nsIGenericFactory:
    static NS_IMETHODIMP Create(nsISupports *aOuter, const nsIID &aIID, void **aResult);

    NS_DECL_ISUPPORTS

    // nsIDocumentLoader interface
    NS_IMETHOD LoadDocument(const nsString& aURLSpec, 
                            const char *aCommand,
                            nsIContentViewerContainer* aContainer,
                            nsIPostData* aPostData = nsnull,
                            nsISupports* aExtraInfo = nsnull,
                            nsIStreamObserver* anObserver = nsnull,
                            nsURLReloadType aType = nsURLReload,
                            const PRUint32 aLocalIP = 0);

    NS_IMETHOD LoadSubDocument(const nsString& aURLSpec,
                               nsISupports* aExtraInfo = nsnull,
                               nsURLReloadType type = nsURLReload,
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

#ifndef NECKO
    // nsILoadGroup interface...
    NS_IMETHOD CreateURL(nsIURI** aInstancePtrResult, 
                         nsIURI* aBaseURL,
                         const nsString& aSpec,
                         nsISupports* aContainer);

    NS_IMETHOD OpenStream(nsIURI *aUrl, 
                          nsIStreamListener *aConsumer);

    NS_IMETHOD GetDefaultLoadAttributes(nsILoadAttribs*& aLoadAttribs);
    NS_IMETHOD SetDefaultLoadAttributes(nsILoadAttribs*  aLoadAttribs);

    NS_IMETHOD AddChildGroup(nsILoadGroup* aGroup);
    NS_IMETHOD RemoveChildGroup(nsILoadGroup* aGroup);
#endif // NECKO

    // Implementation specific methods...
    void FireOnStartDocumentLoad(nsIDocumentLoader* aLoadInitiator,
                                 nsIURI* aURL, 
                                 const char* aCommand);
    void FireOnEndDocumentLoad(nsIDocumentLoader* aLoadInitiator,
                               PRInt32 aStatus);
							   

    void FireOnStartURLLoad(nsIDocumentLoader* aLoadInitiator,
                            nsIURI* aURL, 
                            const char* aContentType, 
                            nsIContentViewer* aViewer);

    void FireOnProgressURLLoad(nsIDocumentLoader* aLoadInitiator,
                               nsIURI* aURL, PRUint32 aProgress, 
                               PRUint32 aProgressMax);

    void FireOnStatusURLLoad(nsIDocumentLoader* aLoadInitiator,
                             nsIURI* aURL, nsString& aMsg);

    void FireOnEndURLLoad(nsIDocumentLoader* aLoadInitiator,
                          nsIURI* aURL, PRInt32 aStatus);

    void LoadURLComplete(nsIURI* aURL, nsISupports* aLoader, PRInt32 aStatus);
    void SetParent(nsDocLoaderImpl* aParent);
    void SetDocumentUrl(nsIURI* aUrl);

    nsresult CreateContentViewer(nsIURI* aURL, 
                                 const char* aContentType, 
                                 const char *aCommand,
                                 nsIContentViewerContainer* aContainer,
                                 nsISupports* aExtraInfo,
                                 nsIStreamListener** aDocListener,
                                 nsIContentViewer** aDocViewer);

protected:
    virtual ~nsDocLoaderImpl();

    void ChildDocLoaderFiredEndDocumentLoad(nsDocLoaderImpl* aChild,
                                            nsIDocumentLoader* aLoadInitiator,
                                            PRInt32 aStatus);


private:
    static PRBool StopBindInfoEnumerator (nsISupports* aElement, void* aData);
    static PRBool StopDocLoaderEnumerator(void* aElement, void* aData);
    static PRBool IsBusyEnumerator(void* aElement, void* aData);

protected:

    // IMPORTANT: The ownership implicit in the following member
    // variables has been explicitly checked and set using nsCOMPtr
    // for owning pointers and raw COM interface pointers for weak
    // (ie, non owning) references. If you add any members to this
    // class, please make the ownership explicit (pinkerton, scc).
  
    nsIURI*                    mDocumentUrl;       // [OWNER] ???compare with document
    nsCOMPtr<nsISupportsArray> m_LoadingDocsList;

    nsVoidArray                mChildGroupList;
    nsVoidArray                mDocObservers;
    nsCOMPtr<nsILoadAttribs>   m_LoadAttrib;
    nsCOMPtr<nsIStreamObserver> mStreamObserver;   // ??? unclear what to do here
    nsIContentViewerContainer* mContainer;         // [WEAK] it owns me!

    nsDocLoaderImpl*           mParent;            // [OWNER] but upside down ownership model
                                                   //  needs to be fixed***
    /*
     * The following counts are for the current document loader only.  They
     * do not take into account URLs being loaded by child document loaders.
     */
    PRInt32 mForegroundURLs;
    PRInt32 mTotalURLs;
    /*
     * This flag indicates that the loader is loading a document.  It is set
     * from the call to LoadDocument(...) until the OnConnectionsComplete(...)
     * notification is fired...
     */
    PRBool mIsLoadingDocument;
};


nsDocLoaderImpl::nsDocLoaderImpl()
{
    NS_INIT_REFCNT();

#if defined(DEBUG) || defined(FORCE_PR_LOG)
    if (nsnull == gDocLoaderLog) {
        gDocLoaderLog = PR_NewLogModule("DocLoader");
    }
#endif /* DEBUG || FORCE_PR_LOG */

    mDocumentUrl    = nsnull;
    mParent         = nsnull;
    mContainer      = nsnull;
    mForegroundURLs = 0;
    mTotalURLs      = 0;

    mIsLoadingDocument = PR_FALSE;

    NS_NewISupportsArray(getter_AddRefs(m_LoadingDocsList));
    NS_NewLoadAttribs(getter_AddRefs(m_LoadAttrib));

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocLoader:%p: created.\n", this));
}


nsDocLoaderImpl::~nsDocLoaderImpl()
{
    Stop();
    if (nsnull != mParent) {
#ifndef NECKO
        mParent->RemoveChildGroup(this);
#endif // NECKO
        NS_RELEASE(mParent);
    }

    NS_IF_RELEASE(mDocumentUrl);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocLoader:%p: deleted.\n", this));

    NS_PRECONDITION((0 == mChildGroupList.Count()), "Document loader has children...");
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
#endif // NECKO
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
#ifndef NECKO
        AddChildGroup(newLoader);
#endif // NECKO
        newLoader->SetParent(this);
    }

  done:
    return rv;
}

nsresult
nsDocLoaderImpl::CreateContentViewer(nsIURI* aURL, 
                                     const char* aContentType, 
                                     const char *aCommand,
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
    rv = factory->CreateInstance(aURL, aContentType, aCommand, aContainer,
                                 aExtraInfo, aDocListenerResult,
                                 aDocViewerResult);
    NS_RELEASE(factory);
    return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::LoadDocument(const nsString& aURLSpec, 
                              const char* aCommand,
                              nsIContentViewerContainer* aContainer,
                              nsIPostData* aPostData,
                              nsISupports* aExtraInfo,
                              nsIStreamObserver* anObserver,
                              nsURLReloadType aType,
                              const PRUint32 aLocalIP)
{
  nsresult rv;
  nsURLLoadType loadType;
  nsDocumentBindInfo* loader = nsnull;

#if defined(DEBUG)
  char buffer[256];

  aURLSpec.ToCString(buffer, sizeof(buffer));
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: LoadDocument(...) called for %s.", 
          this, buffer));
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
  loader->Init(this,           // DocLoader
               aCommand,       // Command
               aContainer,     // Viewer Container
               aExtraInfo,     // Extra Info
               anObserver);    // Observer

  /* The DocumentBindInfo reference is only held by the Array... */
  m_LoadingDocsList->AppendElement((nsIStreamListener *)loader);

  /* Initialize the URL counters... */
  NS_PRECONDITION(((mTotalURLs == 0) && (mForegroundURLs == 0)), "DocuemntLoader is busy...");
  rv = m_LoadAttrib->GetLoadType(&loadType);
  if (NS_FAILED(rv)) {
    loadType = nsURLLoadNormal;
  }
  if (nsURLLoadBackground != loadType) {
    mForegroundURLs = 1;
  }
  mTotalURLs = 1;
  /*
   * Set the flag indicating that the document loader is in the process of
   * loading a document.  This flag will remain set until the 
   * OnConnectionsComplete(...) notification is fired for the loader...
   */
  mIsLoadingDocument = PR_TRUE;

  m_LoadAttrib->SetReloadType(aType);
  // If we've got special loading instructions, mind them.
  if ((aType == nsURLReloadBypassProxy) || 
      (aType == nsURLReloadBypassCacheAndProxy)) {
      m_LoadAttrib->SetBypassProxy(PR_TRUE);
  }
  if ( aLocalIP ) {
      m_LoadAttrib->SetLocalIP(aLocalIP);
  }

  mStreamObserver = dont_QueryInterface(anObserver);

  rv = loader->Bind(aURLSpec, aPostData, nsnull);

done:
  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::LoadSubDocument(const nsString& aURLSpec,
                                 nsISupports* aExtraInfo,
                                 nsURLReloadType type,
                                 const PRUint32 aLocalIP)
{
  nsresult rv;
  nsDocumentBindInfo* loader = nsnull;

#ifdef DEBUG
  char buffer[256];

  aURLSpec.ToCString(buffer, sizeof(buffer));
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: LoadSubDocument(...) called for %s.",
          this, buffer));
#endif /* DEBUG */

  NS_NEWXPCOM(loader, nsDocumentBindInfo);
  if (nsnull == loader) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      return rv;
  }
  loader->Init(this,           // DocLoader
               nsnull,         // Command
               nsnull,     // Viewer Container
               aExtraInfo,     // Extra Info
               mStreamObserver);    // Observer

  /* The DocumentBindInfo reference is only held by the Array... */
  m_LoadingDocsList->AppendElement((nsIStreamListener *)loader);

  /* Increment the URL counters... */
  mForegroundURLs++;
  mTotalURLs++;
  
  rv = loader->Bind(aURLSpec, nsnull, nsnull);

  return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::Stop(void)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Stop() called\n", this));

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

  /* 
   * Release the Stream Observer...  
   * It will be set on the next LoadDocument(...) 
   */
  mStreamObserver = do_QueryInterface(0);   // to be replaced with null_nsCOMPtr()

  return NS_OK;
}       


NS_IMETHODIMP
nsDocLoaderImpl::IsBusy(PRBool& aResult)
{
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

#ifndef NECKO
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
    rv = NS_NewURL(&url, aURLSpec, aBaseURL, aContainer, this);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILoadAttribs> loadAttributes;
      rv = url->GetLoadAttribs(getter_AddRefs(loadAttributes));
      if (loadAttributes)
        loadAttributes->Clone(m_LoadAttrib);
    }
    *aInstancePtrResult = url;
  }

  return rv;
}


NS_IMETHODIMP
nsDocLoaderImpl::OpenStream(nsIURI *aUrl, nsIStreamListener *aConsumer)
{
  nsresult rv;
  nsDocumentBindInfo* loader = nsnull;
  nsURLLoadType loadType = nsURLLoadNormal;

#if defined(DEBUG)
  const char* buffer;
  aUrl->GetSpec(&buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: OpenStream(...) called for %s.", 
          this, buffer));
#endif /* DEBUG */


  NS_NEWXPCOM(loader, nsDocumentBindInfo);
  if (nsnull == loader) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }
  loader->Init(this,              // DocLoader
               nsnull,            // Command
               mContainer,        // Viewer Container
               nsnull,            // Extra Info
               mStreamObserver);  // Observer

  /* The DocumentBindInfo reference is only held by the Array... */
  m_LoadingDocsList->AppendElement(((nsISupports*)(nsIStreamObserver*)loader));

  /* Update the URL counters... */
  nsILoadAttribs* loadAttributes;

  rv = aUrl->GetLoadAttribs(&loadAttributes);
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

  rv = loader->Bind(aUrl, aConsumer);
done:
  return rv;
}


NS_IMETHODIMP
nsDocLoaderImpl::GetDefaultLoadAttributes(nsILoadAttribs*& aLoadAttribs)
{
  aLoadAttribs = m_LoadAttrib;
  NS_IF_ADDREF(aLoadAttribs);

  return NS_OK;;
}


NS_IMETHODIMP
nsDocLoaderImpl::SetDefaultLoadAttributes(nsILoadAttribs*  aLoadAttribs)
{
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

  return NS_OK;
}


NS_IMETHODIMP
nsDocLoaderImpl::AddChildGroup(nsILoadGroup* aGroup)
{
  mChildGroupList.AppendElement(aGroup);
  return NS_OK;
}


NS_IMETHODIMP
nsDocLoaderImpl::RemoveChildGroup(nsILoadGroup* aGroup)
{
  nsresult rv = NS_OK;

  if (PR_FALSE == mChildGroupList.RemoveElement(aGroup)) {
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}
#endif // NECKO


void nsDocLoaderImpl::FireOnStartDocumentLoad(nsIDocumentLoader* aLoadInitiator,
                                              nsIURI* aURL, 
                                              const char* aCommand)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

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

void nsDocLoaderImpl::FireOnEndDocumentLoad(nsIDocumentLoader* aLoadInitiator,
                                            PRInt32 aStatus)
									
{
    PRInt32 count = mDocObservers.Count();
    PRInt32 index;

    /*
     * First notify any observers that the document load has finished...
     */
    for (index = 0; index < count; index++) {
        nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)
            mDocObservers.ElementAt(index);
        observer->OnEndDocumentLoad(aLoadInitiator, mDocumentUrl,
                                    aStatus, observer);
    }

    /*
     * Finally notify the parent...
     */
    if (nsnull != mParent) {
        mParent->ChildDocLoaderFiredEndDocumentLoad(this, aLoadInitiator, aStatus);
    }
}

void
nsDocLoaderImpl::ChildDocLoaderFiredEndDocumentLoad(nsDocLoaderImpl* aChild,
                                                    nsIDocumentLoader* aLoadInitiator,
                                                    PRInt32 aStatus)
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

void nsDocLoaderImpl::FireOnStartURLLoad(nsIDocumentLoader* aLoadInitiator,
                                         nsIURI* aURL, const char* aContentType, 
                                         nsIContentViewer* aViewer)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that the URL load has begun...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
    observer->OnStartURLLoad(aLoadInitiator, aURL, aContentType, aViewer);
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
    mParent->FireOnStartURLLoad(aLoadInitiator, aURL, aContentType, aViewer);
  }
}

void nsDocLoaderImpl::FireOnProgressURLLoad(nsIDocumentLoader* aLoadInitiator,
                                            nsIURI* aURL, PRUint32 aProgress,
                                            PRUint32 aProgressMax)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that there is progress information available...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
    observer->OnProgressURLLoad(aLoadInitiator, aURL, aProgress, aProgressMax);
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
    mParent->FireOnProgressURLLoad(aLoadInitiator, aURL, aProgress, aProgressMax);
  }
}

void nsDocLoaderImpl::FireOnStatusURLLoad(nsIDocumentLoader* aLoadInitiator,
                                          nsIURI* aURL, nsString& aMsg)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that there is status text available...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
    observer->OnStatusURLLoad(aLoadInitiator, aURL, aMsg);
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
    mParent->FireOnStatusURLLoad(aLoadInitiator, aURL, aMsg);
  }
}

void nsDocLoaderImpl::FireOnEndURLLoad(nsIDocumentLoader* aLoadInitiator,
                                       nsIURI* aURL, PRInt32 aStatus)
{
  PRInt32 count = mDocObservers.Count();
  PRInt32 index;

  /*
   * First notify any observers that the URL load has begun...
   */
  for (index = 0; index < count; index++) {
    nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mDocObservers.ElementAt(index);
    observer->OnEndURLLoad(aLoadInitiator, aURL, aStatus);
  }

  /*
   * Finally notify the parent...
   */
  if (nsnull != mParent) {
    mParent->FireOnEndURLLoad(aLoadInitiator, aURL, aStatus);
  }
}



void nsDocLoaderImpl::LoadURLComplete(nsIURI* aURL, nsISupports* aBindInfo, PRInt32 aStatus)
{
    PRBool isForegroundURL = PR_FALSE;

    /*
     * If the entry is not found in the list, then it must have been cancelled
     * via Stop(...). So ignore just it... 
     */
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

    /*
     * Fire the OnEndURLLoad notification to any observers...
     */
    FireOnEndURLLoad((nsIDocumentLoader *) this, aURL, aStatus);

    /*
     * Fire the OnEndDocumentLoad notification to any observers...
     */
    PRBool busy;
    IsBusy(busy);
    if (isForegroundURL && !busy) {
#if defined(DEBUG)
        const char* buffer;

        mDocumentUrl->GetSpec(&buffer);
        PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
               ("DocLoader:%p: OnEndDocumentLoad(...) called for %s.\n",
                this, buffer));
#endif /* DEBUG */

        FireOnEndDocumentLoad((nsIDocumentLoader *) this, aStatus);
    }
}

void nsDocLoaderImpl::SetParent(nsDocLoaderImpl* aParent)
{
  NS_IF_RELEASE(mParent);
  mParent = aParent;
  NS_IF_ADDREF(mParent);
}

void nsDocLoaderImpl::SetDocumentUrl(nsIURI* aUrl)
{
  NS_IF_RELEASE(mDocumentUrl);
  mDocumentUrl = aUrl;
  NS_IF_ADDREF(mDocumentUrl);
}

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

/****************************************************************************
 * nsDocumentBindInfo implementation...
 ****************************************************************************/

nsDocumentBindInfo::nsDocumentBindInfo()
{
    NS_INIT_REFCNT();

    m_Command = nsnull;
    m_Url = nsnull;
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

    m_Url        = nsnull;
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
    NS_IF_RELEASE(m_Url);
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
  if (aIID.Equals(kRefreshURLIID)) {
    *aInstancePtrResult = (void*) ((nsIRefreshUrl*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult nsDocumentBindInfo::Bind(const nsString& aURLSpec, 
                                  nsIPostData* aPostData,
                                  nsIStreamListener* aListener)
{
    nsresult rv;
    nsIURI* url = nsnull;

    /* If this nsDocumentBindInfo was created with a container pointer.
     * extract the nsISupports iface from it and create the url with 
     * the nsISupports pointer so the backend can have access to the front
     * end nsIContentViewerContainer for refreshing urls.
     */
#ifndef NECKO
    rv = m_DocLoader->CreateURL(&url, nsnull, aURLSpec, m_Container);
#else
    rv = NS_ERROR_NULL_POINTER;
#endif // NECKO
    if (NS_FAILED(rv)) {
      return rv;
    }

    /* Store any POST data into the URL */
    if (nsnull != aPostData) {
        static NS_DEFINE_IID(kPostToServerIID, NS_IPOSTTOSERVER_IID);
        nsIPostToServer* pts;

        rv = url->QueryInterface(kPostToServerIID, (void **)&pts);
        if (NS_SUCCEEDED(rv)) {
            const char* data = aPostData->GetData();

            if (aPostData->IsFile()) {
                pts->SendDataFromFile(data);
            }
            else {
                pts->SendData(data, aPostData->GetDataLength());
            }
            NS_RELEASE(pts);
        }
    }

    /*
     * Set the URL has the current "document" being loaded...
     */
    m_DocLoader->SetDocumentUrl(url);
    /*
     * Fire the OnStarDocumentLoad interfaces 
     */
    m_DocLoader->FireOnStartDocumentLoad((nsIDocumentLoader *) m_DocLoader, url, m_Command);

    /*
     * Initiate the network request...
     */
    rv = Bind(url, aListener);
    NS_RELEASE(url);

    return rv;
}


nsresult nsDocumentBindInfo::Bind(nsIURI* aURL, nsIStreamListener* aListener)
{
  nsresult rv = NS_OK;

  m_Url = aURL;
  NS_ADDREF(m_Url);

#if defined(DEBUG)
  const char *buffer;

  aURL->GetSpec(&buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocumentBindInfo:%p: OnStartDocumentLoad(...) called for %s.\n",
          this, buffer));
#endif /* DEBUG */

  //  m_DocLoader->FireOnStartDocumentLoad(aURL, m_Command);

  /* Set up the stream listener (if provided)... */
  if (nsnull != aListener) {
    m_NextStream = aListener;
    NS_ADDREF(m_NextStream);
  }

#ifndef NECKO
  /* Start the URL binding process... */
  nsINetService *inet = nsnull;
  rv = nsServiceManager::GetService(kNetServiceCID,
                                    kINetServiceIID,
                                    (nsISupports **)&inet);
  if (NS_SUCCEEDED(rv)) {
    rv = inet->OpenStream(m_Url, this);
    nsServiceManager::ReleaseService(kNetServiceCID, inet);
  }
#else
  NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsIURI *uri = nsnull;
  rv = m_Url->QueryInterface(nsIURI::GetIID(), (void**)&uri);
  if (NS_FAILED(rv)) return rv;
  
  nsIChannel *channel = nsnull;
  // XXX NECKO verb? sinkGetter?
  rv = service->NewChannelFromURI("load", uri, nsnull, &channel);
  NS_RELEASE(uri);
  if (NS_FAILED(rv)) return rv;

  nsIEventQueue *eventQ = nsnull;
  NS_WITH_SERVICE(nsIEventQueueService, eqService, kEventQueueServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = eqService->GetThreadEventQueue(PR_CurrentThread(), &eventQ);
  if (NS_FAILED(rv)) return rv;

  rv = channel->AsyncRead(0, -1, nsnull, eventQ, this);
  NS_RELEASE(eventQ);
#endif // NECKO

  return rv;
}


nsresult nsDocumentBindInfo::Stop(void)
{
  nsresult rv;
  if (m_Url == nsnull) return NS_OK;

#if defined(DEBUG)
  const char* spec;
  rv = m_Url->GetSpec(&spec);
  if (NS_SUCCEEDED(rv)) {
      PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
             ("DocumentBindInfo:%p: Stop(...) called for %s.\n", this, spec));
  }
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


NS_METHOD nsDocumentBindInfo::OnProgress(nsIURI* aURL, PRUint32 aProgress, 
                                         PRUint32 aProgressMax)
{
    nsresult rv = NS_OK;

#if defined(DEBUG)
    const char* spec;
    (void)aURL->GetSpec(&spec);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnProgress(...) called for %s.  Progress: %d.  ProgressMax: %d\n", 
            this, spec, aProgress, aProgressMax));
#endif /* DEBUG */

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnProgress(aURL, aProgress, aProgressMax);
    }

    /* Pass the notification out to any observers... */
    m_DocLoader->FireOnProgressURLLoad((nsIDocumentLoader *) m_DocLoader, aURL, aProgress, aProgressMax);

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnProgress(aURL, aProgress, aProgressMax);
    }

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
    nsresult rv = NS_OK;

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnStatus(aURL, aMsg);
    }

    /* Pass the notification out to any observers... */
    nsString msgStr(aMsg);
    m_DocLoader->FireOnStatusURLLoad((nsIDocumentLoader *) m_DocLoader, aURL, msgStr);

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnStatus(aURL, aMsg);
    }

    return rv;
}


NS_IMETHODIMP
nsDocumentBindInfo::OnStartBinding(nsIURI* aURL, const char *aContentType)
{
    nsresult rv = NS_OK;
    nsIContentViewer* viewer = nsnull;

#if defined(DEBUG)
    const char* spec;
    (void)aURL->GetSpec(&spec);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p OnStartBinding(...) called for %s.  Content-type is %s\n",
            this, spec, aContentType));
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
            rv = m_DocLoader->CreateContentViewer(m_Url,
                                                  aContentType, 
                                                  m_Command, 
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
                rv = m_Container->HandleUnknownContentType( (nsIDocumentLoader*) m_DocLoader, aURL, aContentType, m_Command );
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
     * Pass the OnStartBinding(...) notification out to the document 
     * IStreamListener.
     */
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnStartBinding(aURL, aContentType);
    }

    /*
     * Notify the DocumentLoadObserver(s) 
     */
    if ((nsnull == viewer) && (nsnull != m_Container)) {
        m_Container->GetContentViewer(&viewer);
    }
    m_DocLoader->FireOnStartURLLoad((nsIDocumentLoader *)m_DocLoader, m_Url, aContentType, viewer);

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        nsresult rv2 = m_Observer->OnStartBinding(aURL, aContentType);
        if (NS_SUCCEEDED(rv))
        	rv = rv2;
    }

  done:
    NS_IF_RELEASE(viewer);

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnDataAvailable(nsIURI* aURL, 
                                              nsIInputStream *aStream, PRUint32 aLength)
{
    nsresult rv = NS_OK;

#if defined(DEBUG)
    const char* spec;
    (void)aURL->GetSpec(&spec);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnDataAvailable(...) called for %s.  Bytes available: %d.\n", 
            this, spec, aLength));
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
        rv = listener->OnDataAvailable(aURL, aStream, aLength);
        NS_RELEASE(listener);
    } else {
      rv = NS_BINDING_FAILED;
    }

done:
    return rv;
}


NS_METHOD nsDocumentBindInfo::OnStopBinding(nsIURI* aURL, nsresult aStatus, 
                                            const PRUnichar* aMsg)
{
    nsresult rv = NS_OK;

#if defined(DEBUG)
    const char* spec;
    (void)aURL->GetSpec(&spec);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
           ("DocumentBindInfo:%p: OnStopBinding(...) called for %s.  Status: %d.\n", 
            this, spec, aStatus));
#endif /* DEBUG */

    if (NS_FAILED(aStatus)) {
      const char *url;
      if (nsnull != aURL) 
        aURL->GetSpec(&url);
      else
        url = "";      
      cerr << "nsDocumentBindInfo::OnStopBinding: Load of URL '" << url << "' failed.  Error code: " 
        << NS_ERROR_GET_CODE(aStatus) << "\n";
    }

    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnStopBinding(aURL, aStatus, aMsg);
    }

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnStopBinding(aURL, aStatus, aMsg);
    }

    /*
     * The stream is complete...  Tell the DocumentLoader to release us...
     */
    m_DocLoader->LoadURLComplete(aURL, (nsIStreamListener *)this, aStatus);
    NS_IF_RELEASE(m_NextStream);

    return rv;
}

NS_METHOD
nsDocumentBindInfo::RefreshURL(nsIURI* aURL, PRInt32 millis, PRBool repeat)
{
    if (nsnull != m_Container) {
        nsresult rv;
        nsIRefreshUrl* refresher = nsnull;

        /* Delegate the actual refresh call up-to the container. */
        rv = m_Container->QueryInterface(kRefreshURLIID, (void**)&refresher);

        if (NS_FAILED(rv)) {
            return PR_FALSE;
        }
        rv = refresher->RefreshURL(aURL, millis, repeat);
        NS_RELEASE(refresher);
        return rv;
    }
    return PR_FALSE;
}

NS_METHOD
nsDocumentBindInfo::CancelRefreshURLTimers(void)
{
    if (nsnull != m_Container) {
        nsresult rv;
        nsIRefreshUrl* refresher = nsnull;

        /* Delegate the actual cancel call up-to the container. */
        rv = m_Container->QueryInterface(kRefreshURLIID, (void**)&refresher);

        if (NS_FAILED(rv)) {
            return PR_FALSE;
        }
        rv = refresher->CancelRefreshURLTimers();
        NS_RELEASE(refresher);
        return rv;
    }
    return PR_FALSE;
}

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
