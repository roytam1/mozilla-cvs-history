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
#ifdef XP_MAC
#include "nsIDocumentLoader.h"
#define NS_IMPL_IDS
#else
#define NS_IMPL_IDS
#include "nsIDocumentLoader.h"
#endif
#include "prmem.h"
#include "plstr.h"
#include "nsString.h"
#include "nsISupportsArray.h"
#include "nsIURL.h"
#include "nsIStreamListener.h"
#include "nsINetSupport.h"
#include "nsIPostToServer.h"
#include "nsIFactory.h"
#include "nsIContentViewerContainer.h"
#include "nsIRefreshUrl.h"
#include "nsITimer.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsVoidArray.h"
#include "nsIHttpUrl.h"

// XXX: Only needed for dummy factory...
#include "nsIDocument.h"
#include "nsIDocumentViewer.h"
#include "nsICSSParser.h"

/* Forward declarations.... */
class nsDocLoaderImpl;


  /* Private IIDs... */
/* eb001fa0-214f-11d2-bec0-00805f8a66dc */
#define NS_DOCUMENTBINDINFO_IID   \
{ 0xeb001fa0, 0x214f, 0x11d2, \
  {0xbe, 0xc0, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }

/* 28475a80-2150-11d2-bec0-00805f8a66dc */
#define NS_DOCLOADERIMPL_IID   \
{ 0x28475a80, 0x2150, 0x11d2, \
  {0xbe, 0xc0, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }

/* Define IIDs... */
NS_DEFINE_IID(kIStreamObserverIID,        NS_ISTREAMOBSERVER_IID);
NS_DEFINE_IID(kIDocumentLoaderIID,        NS_IDOCUMENTLOADER_IID);
NS_DEFINE_IID(kIDocumentLoaderFactoryIID, NS_IDOCUMENTLOADERFACTORY_IID);
NS_DEFINE_IID(kDocLoaderImplIID,          NS_DOCLOADERIMPL_IID);
NS_DEFINE_IID(kDocumentBindInfoIID,       NS_DOCUMENTBINDINFO_IID);
NS_DEFINE_IID(kRefreshURLIID,       NS_IREFRESHURL_IID);
NS_DEFINE_IID(kHTTPURLIID,          NS_IHTTPURL_IID);
NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);



/* 
 * The nsDocumentBindInfo contains the state required when a single document
 * is being loaded...  Each instance remains alive until its target URL has 
 * been loaded (or aborted).
 *
 * The Document Loader maintains a list of nsDocumentBindInfo instances which 
 * represents the set of documents actively being loaded...
 */
class nsDocumentBindInfo : public nsIStreamListener, 
                           public nsINetSupport,
                           public nsIRefreshUrl
{
public:
    nsDocumentBindInfo(nsDocLoaderImpl* aDocLoader,
                       const char *aCommand, 
                       nsIContentViewerContainer* aContainer,
                       nsISupports* aExtraInfo,
                       nsIStreamObserver* anObserver);

    NS_DECL_ISUPPORTS

    nsresult Bind(const nsString& aURLSpec, 
                  nsIPostData* aPostData, 
                  nsIStreamListener* aListener);
    nsresult Stop(void);

    /* nsIStreamListener interface methods... */
    NS_IMETHOD GetBindInfo(nsIURL* aURL);
    NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
    NS_IMETHOD OnStatus(nsIURL* aURL, const nsString& aMsg);
    NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
    NS_IMETHOD OnDataAvailable(nsIURL* aURL, nsIInputStream *aStream, PRInt32 aLength);
    NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 aStatus, const nsString& aMsg);

	/* nsINetSupport interface methods */
    NS_IMETHOD_(void) Alert(const nsString &aText);
    NS_IMETHOD_(PRBool) Confirm(const nsString &aText);
    NS_IMETHOD_(PRBool) Prompt(const nsString &aText,
                               const nsString &aDefault,
                               nsString &aResult);
    NS_IMETHOD_(PRBool) PromptUserAndPassword(const nsString &aText,
                                              nsString &aUser,
                                              nsString &aPassword);
    NS_IMETHOD_(PRBool) PromptPassword(const nsString &aText,
                                       nsString &aPassword);

    /* nsIRefreshURL interface methods... */
    NS_IMETHOD RefreshURL(nsIURL* aURL, PRInt32 millis, PRBool repeat);
    NS_IMETHOD CancelRefreshURLTimers(void);

    nsresult GetStatus(void) { return mStatus; }

protected:
    virtual ~nsDocumentBindInfo();

protected:
    char*               m_Command;
    nsIURL*             m_Url;
    nsIContentViewerContainer* m_Container;
    nsISupports*        m_ExtraInfo;
    nsIStreamObserver*  m_Observer;
    nsINetSupport*      m_NetSupport;
    nsIStreamListener*  m_NextStream;
    nsDocLoaderImpl*    m_DocLoader;

    nsresult            mStatus;
};




/****************************************************************************
 * nsDocFactoryImpl implementation...
 ****************************************************************************/

class nsDocFactoryImpl : public nsIDocumentLoaderFactory
{
public:
    nsDocFactoryImpl();

    NS_DECL_ISUPPORTS

    NS_IMETHOD CreateInstance(nsIURL* aURL,
                              const char* aContentType, 
                              const char* aCommand,
                              nsIContentViewerContainer* aContainer,
                              nsIStreamListener** aDocListener,
                              nsIContentViewer** aDocViewer);

    nsresult InitUAStyleSheet();

    nsresult CreateImageDocument(nsIURL* aURL, 
                                 const char* aCommand,
                                 nsIContentViewerContainer* aContainer,
                                 nsIStreamListener** aDocListener,
                                 nsIContentViewer** aDocViewer);
};

static nsIStyleSheet* gUAStyleSheet;

nsDocFactoryImpl::nsDocFactoryImpl()
{
    NS_INIT_REFCNT();
}

/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ISUPPORTS(nsDocFactoryImpl,kIDocumentLoaderFactoryIID);

static char* gValidTypes[] = {"text/html","text/xml","application/rtf",0};

static char* gImageTypes[] = {"image/gif", "image/jpeg", 0 };

NS_IMETHODIMP
nsDocFactoryImpl::CreateInstance(nsIURL* aURL, 
                                 const char* aContentType, 
                                 const char *aCommand,
                                 nsIContentViewerContainer* aContainer,
                                 nsIStreamListener** aDocListener,
                                 nsIContentViewer** aDocViewer)
{
    nsresult rv = NS_ERROR_FAILURE;
    nsIDocument* doc = nsnull;
    nsIDocumentViewer* docv = nsnull;

    int typeIndex=0;
    while(gValidTypes[typeIndex]) {
      if (0== PL_strcmp(gValidTypes[typeIndex++], aContentType)) {
        goto nextstep;
      }
    }

    // Try secondary types
    typeIndex = 0;
    while(gImageTypes[typeIndex]) {
      if (0== PL_strcmp(gImageTypes[typeIndex++], aContentType)) {
          return CreateImageDocument(aURL, aCommand,
                                     aContainer,
                                     aDocListener,
                                     aDocViewer);
      }
    }
    goto done;

nextstep:
    // Load the UA style sheet if we haven't already done that
    if (nsnull == gUAStyleSheet) {
        InitUAStyleSheet();
    }

    /*
     * Create the HTML document...
     */
    rv = NS_NewHTMLDocument(&doc);
    if (NS_OK != rv) {
        goto done;
    }

    /*
     * Create the HTML Content Viewer...
     */
    rv = NS_NewDocumentViewer(docv);
    if (NS_OK != rv) {
        goto done;
    }
    docv->SetUAStyleSheet(gUAStyleSheet);

    /* 
     * Initialize the document to begin loading the data...
     *
     * An nsIStreamListener connected to the parser is returned in
     * aDocListener.
     */
    rv = doc->StartDocumentLoad(aURL, aContainer, aDocListener);
    if (NS_OK != rv) {
        NS_IF_RELEASE(docv);
        goto done;
    }

    /*
     * Bind the document to the Content Viewer...
     */
    rv = docv->BindToDocument(doc, aCommand);
    *aDocViewer = docv;

done:
    NS_IF_RELEASE(doc);
    return rv;
}

nsresult
nsDocFactoryImpl::CreateImageDocument(nsIURL* aURL, 
                                      const char* aCommand,
                                      nsIContentViewerContainer* aContainer,
                                      nsIStreamListener** aDocListener,
                                      nsIContentViewer** aDocViewer)
{
    nsresult rv = NS_ERROR_FAILURE;
    nsIDocument* doc = nsnull;
    nsIDocumentViewer* docv = nsnull;

    // Load the UA style sheet if we haven't already done that
    if (nsnull == gUAStyleSheet) {
        InitUAStyleSheet();
    }

    /*
     * Create the image document...
     */
    rv = NS_NewImageDocument(&doc);
    if (NS_OK != rv) {
        goto done;
    }

    /*
     * Create the image content viewer...
     */
    rv = NS_NewDocumentViewer(docv);
    if (NS_OK != rv) {
        goto done;
    }
    docv->SetUAStyleSheet(gUAStyleSheet);

    /* 
     * Initialize the document to begin loading the data...
     *
     * An nsIStreamListener connected to the parser is returned in
     * aDocListener.
     */
    rv = doc->StartDocumentLoad(aURL, aContainer, aDocListener);
    if (NS_OK != rv) {
        NS_IF_RELEASE(docv);
        goto done;
    }

    /*
     * Bind the document to the Content Viewer...
     */
    rv = docv->BindToDocument(doc, aCommand);
    *aDocViewer = docv;

done:
    NS_IF_RELEASE(doc);
    return rv;
}

#define UA_CSS_URL "resource:/res/ua.css"

nsresult nsDocFactoryImpl::InitUAStyleSheet()
{
  nsresult rv = NS_OK;

  if (nsnull == gUAStyleSheet) {  // snarf one
    nsIURL* uaURL;
    rv = NS_NewURL(&uaURL, nsnull, nsString(UA_CSS_URL)); // XXX this bites, fix it
    if (NS_OK == rv) {
      // Get an input stream from the url
      PRInt32 ec;
      nsIInputStream* in = uaURL->Open(&ec);
      if (nsnull != in) {
        // Translate the input using the argument character set id into unicode
        nsIUnicharInputStream* uin;
        rv = NS_NewConverterStream(&uin, nsnull, in);
        if (NS_OK == rv) {
          // Create parser and set it up to process the input file
          nsICSSParser* css;
          rv = NS_NewCSSParser(&css);
          if (NS_OK == rv) {
            // Parse the input and produce a style set
            // XXX note: we are ignoring rv until the error code stuff in the
            // input routines is converted to use nsresult's
            css->Parse(uin, uaURL, gUAStyleSheet);
            NS_RELEASE(css);
          }
          NS_RELEASE(uin);
        }
        NS_RELEASE(in);
      }
      else {
//        printf("open of %s failed: error=%x\n", UA_CSS_URL, ec);
        rv = NS_ERROR_ILLEGAL_VALUE;  // XXX need a better error code here
      }

      NS_RELEASE(uaURL);
    }
  }
  return rv;
}


/****************************************************************************
 * nsDocLoaderImpl implementation...
 ****************************************************************************/

class nsDocLoaderImpl : public nsIDocumentLoader
{
public:

    nsDocLoaderImpl(nsDocLoaderImpl* aParent);

    NS_DECL_ISUPPORTS

    NS_IMETHOD LoadURL(const nsString& aURLSpec, 
                       const char *aCommand,
                       nsIContentViewerContainer* aContainer,
                       nsIPostData* aPostData = nsnull,
                       nsISupports* aExtraInfo = nsnull,
                       nsIStreamObserver* anObserver = nsnull);

    NS_IMETHOD LoadURL(const nsString& aURLSpec,
                       nsIStreamListener* aListener);

    NS_IMETHOD Stop(void);

    NS_IMETHOD CreateDocumentLoader(nsIDocumentLoader** anInstance);
    NS_IMETHOD SetDocumentFactory(nsIDocumentLoaderFactory* aFactory);

    void LoadURLComplete(nsISupports* loader);

    NS_IMETHOD AddObserver(nsIDocumentLoaderObserver *aObserver);
    NS_IMETHOD RemoveObserver(nsIDocumentLoaderObserver *aObserver);

protected:
    virtual ~nsDocLoaderImpl();

private:
    static PRBool StopBindInfoEnumerator (nsISupports* aElement, void* aData);
    static PRBool StopDocLoaderEnumerator(nsISupports* aElement, void* aData);

public:
    nsIDocumentLoaderFactory* m_DocFactory;

protected:
    nsISupportsArray* m_LoadingDocsList;

    nsDocLoaderImpl* mParent;
    nsISupportsArray* mChildDocLoaderList;
    nsVoidArray mObservers;
};


nsDocLoaderImpl::nsDocLoaderImpl(nsDocLoaderImpl* aParent)
{
    NS_INIT_REFCNT();

    NS_NewISupportsArray(&m_LoadingDocsList);
    NS_NewISupportsArray(&mChildDocLoaderList);

    mParent = aParent;
    NS_IF_ADDREF(mParent);

    m_DocFactory = new nsDocFactoryImpl();
    NS_ADDREF(m_DocFactory);
}


nsDocLoaderImpl::~nsDocLoaderImpl()
{
    Stop();

    NS_IF_RELEASE(mParent);
    NS_IF_RELEASE(mChildDocLoaderList);
    NS_IF_RELEASE(m_LoadingDocsList);
    NS_IF_RELEASE(m_DocFactory);
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
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kDocLoaderImplIID)) {
    *aInstancePtr = (void*)this;
    AddRef();
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

    newLoader = new nsDocLoaderImpl(this);
    if (nsnull == newLoader) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    rv = newLoader->QueryInterface(kIDocumentLoaderIID, (void**)anInstance);

    mChildDocLoaderList->AppendElement(newLoader);

done:
    return rv;
}



NS_IMETHODIMP
nsDocLoaderImpl::SetDocumentFactory(nsIDocumentLoaderFactory* aFactory)
{
    NS_IF_RELEASE(m_DocFactory);
    m_DocFactory = aFactory;
    NS_IF_ADDREF(m_DocFactory);

    return NS_OK;
}


NS_IMETHODIMP
nsDocLoaderImpl::LoadURL(const nsString& aURLSpec, 
                         const char* aCommand,
                         nsIContentViewerContainer* aContainer,
                         nsIPostData* aPostData,
                         nsISupports* aExtraInfo,
                         nsIStreamObserver* anObserver)
{
    nsresult rv;
    nsDocumentBindInfo* loader = nsnull;

    /* Check for initial error conditions... */
    if (nsnull == aContainer) {
        rv = NS_ERROR_NULL_POINTER;
        goto done;
    }

    loader = new nsDocumentBindInfo(this,           // DocLoader
                                    aCommand,       // Command
                                    aContainer,     // Viewer Container
                                    aExtraInfo,     // Extra Info
                                    anObserver);    // Observer
    if (nsnull == loader) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    /* The DocumentBindInfo reference is only held by the Array... */
    m_LoadingDocsList->AppendElement((nsIStreamListener *)loader);

    rv = loader->Bind(aURLSpec, aPostData, nsnull);

done:
    return rv;
}


NS_IMETHODIMP
nsDocLoaderImpl::LoadURL(const nsString& aURLSpec,
                         nsIStreamListener* aListener)
{
    nsresult rv;
    nsDocumentBindInfo* loader = nsnull;

    /* Check for initial error conditions... */
    if (nsnull == aListener) {
        rv = NS_ERROR_NULL_POINTER;
        goto done;
    }

    loader = new nsDocumentBindInfo(this,           // DocLoader
                                    nsnull,         // Command
                                    nsnull,         // Viewer Container
                                    nsnull,         // Extra Info
                                    nsnull);        // Observer
    if (nsnull == loader) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }

    /* The DocumentBindInfo reference is only held by the Array... */
    m_LoadingDocsList->AppendElement(((nsISupports*)(nsIStreamObserver*)loader));

    rv = loader->Bind(aURLSpec, nsnull, aListener);
done:
    return rv;
}

NS_IMETHODIMP
nsDocLoaderImpl::Stop(void)
{
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
    mChildDocLoaderList->EnumerateForwards(nsDocLoaderImpl::StopDocLoaderEnumerator, nsnull);

    return NS_OK;
}       


void nsDocLoaderImpl::LoadURLComplete(nsISupports* aBindInfo)
{
    PRBool rv;

    rv = m_LoadingDocsList->RemoveElement(aBindInfo);

    if (0 == m_LoadingDocsList->Count()) {
      PRInt32 count = mObservers.Count();
      PRInt32 index;

      for (index = 0; index < count; index++) {
        nsIDocumentLoaderObserver* observer = (nsIDocumentLoaderObserver*)mObservers.ElementAt(index);
        observer->OnConnectionsComplete();
      }
    }

    /*
     * If the entry was not found in the list, then it must have been cancelled
     * via Stop(...).  
     */
#if defined(NS_DEBUG)
    if (PR_TRUE != rv) {
        nsDocumentBindInfo* docInfo;

        rv = aBindInfo->QueryInterface(kDocumentBindInfoIID, (void**)&docInfo);
        NS_ASSERTION(((NS_OK == rv) && (docInfo->GetStatus() == NS_BINDING_ABORTED)), 
                     "Entry was not Aborted!");
    }
#endif /* NS_DEBUG */
}


PRBool nsDocLoaderImpl::StopBindInfoEnumerator(nsISupports* aElement, void* aData)
{
    nsresult rv;
    nsDocumentBindInfo* bindInfo;

    rv = aElement->QueryInterface(kDocumentBindInfoIID, (void**)&bindInfo);
    if (NS_OK == rv) {
        bindInfo->Stop();
    }

    return PR_TRUE;
}


PRBool nsDocLoaderImpl::StopDocLoaderEnumerator(nsISupports* aElement, void* aData)
{
    nsresult rv;
    nsDocLoaderImpl* docLoader;
    
    rv = aElement->QueryInterface(kDocLoaderImplIID, (void**)&docLoader);
    if (NS_OK == rv) {
        docLoader->Stop();
    }

    return PR_TRUE;
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
  if (mObservers.IndexOf(aObserver) == -1) {
    mObservers.AppendElement(aObserver);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocLoaderImpl::RemoveObserver(nsIDocumentLoaderObserver* aObserver)
{
  if (PR_TRUE == mObservers.RemoveElement(aObserver)) {
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/****************************************************************************
 * nsDocumentBindInfo implementation...
 ****************************************************************************/

nsDocumentBindInfo::nsDocumentBindInfo(nsDocLoaderImpl* aDocLoader,
                                       const char *aCommand, 
                                       nsIContentViewerContainer* aContainer,
                                       nsISupports* aExtraInfo,
                                       nsIStreamObserver* anObserver)
{
    NS_INIT_REFCNT();

    m_Url        = nsnull;
    m_NextStream = nsnull;
    m_Command    = (nsnull != aCommand) ? PL_strdup(aCommand) : nsnull;
    m_ExtraInfo  = aExtraInfo;

    m_DocLoader = aDocLoader;
    NS_ADDREF(m_DocLoader);

    m_Container = aContainer;
    NS_IF_ADDREF(m_Container);

    m_Observer = anObserver;
    NS_IF_ADDREF(m_Observer);
	m_NetSupport = NULL;
    if (m_Observer) {
        m_Observer->QueryInterface(kINetSupportIID, (void **) &m_NetSupport);
    }

    m_ExtraInfo = aExtraInfo;
    NS_IF_ADDREF(m_ExtraInfo);
    mStatus = NS_OK;
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
    NS_IF_RELEASE(m_NetSupport);
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
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIStreamListenerIID)) {
    *aInstancePtrResult = (void*) ((nsIStreamListener*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kDocumentBindInfoIID)) {
    *aInstancePtrResult = (void*) this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kINetSupportIID)) {
    *aInstancePtrResult = (void*) ((nsINetSupport*)this);
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kRefreshURLIID)) {
    *aInstancePtrResult = (void*) ((nsIRefreshUrl*)this);
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsresult nsDocumentBindInfo::Bind(const nsString& aURLSpec, 
                                  nsIPostData* aPostData,
                                  nsIStreamListener* aListener)
{
    nsresult rv;

    /* If this nsDocumentBindInfo was created with a container pointer.
     * extract the nsISupports iface from it and create the url with 
     * the nsISupports pointer so the backend can have access to the front
     * end nsIContentViewerContainer for refreshing urls.
     */
    if (m_Container) {
        nsISupports* container = nsnull;
        rv = m_Container->QueryInterface(kISupportsIID, (void**)&container);
        if (rv != NS_OK) {
            return rv;
        }
        rv = NS_NewURL(&m_Url, aURLSpec, container);
        NS_RELEASE(container);
        container = nsnull;
        if (NS_OK != rv) {
            return rv;
        }
    } else {
        rv = NS_NewURL(&m_Url, aURLSpec);
        if (NS_OK != rv) {
            return rv;
        }
    }

    /* Store any POST data into the URL */
    if (nsnull != aPostData) {
        static NS_DEFINE_IID(kPostToServerIID, NS_IPOSTTOSERVER_IID);
        nsIPostToServer* pts;

        rv = m_Url->QueryInterface(kPostToServerIID, (void **)&pts);
        if (NS_OK == rv) {
            const char* data = aPostData->GetData();

            if (aPostData->IsFile()) {
                pts->SendDataFromFile(data);
            }
            else {
                pts->SendData(data, aPostData->GetDataLength());
            }
        }
    }

    /* Set up the stream listener (if provided)... */
    if (nsnull != aListener) {
        m_NextStream = aListener;
        NS_ADDREF(m_NextStream);
    }

    /* Start the URL binding process... */
    rv = m_Url->Open(this);

    return rv;
}

nsresult nsDocumentBindInfo::Stop(void)
{
    mStatus = NS_BINDING_ABORTED;

    return NS_OK;
}


NS_METHOD nsDocumentBindInfo::GetBindInfo(nsIURL* aURL)
{
    nsresult rv = NS_OK;

    NS_PRECONDITION(nsnull !=m_NextStream, "DocLoader: No stream for document");

    if (nsnull != m_NextStream) {
        rv = m_NextStream->GetBindInfo(aURL);
    }

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnProgress(nsIURL* aURL, PRInt32 aProgress, 
                                         PRInt32 aProgressMax)
{
    nsresult rv = NS_OK;

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnProgress(aURL, aProgress, aProgressMax);
    }

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnProgress(aURL, aProgress, aProgressMax);
    }

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnStatus(nsIURL* aURL, const nsString& aMsg)
{
    nsresult rv = NS_OK;

    /* Pass the notification out to the next stream listener... */
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnStatus(aURL, aMsg);
    }

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnStatus(aURL, aMsg);
    }

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnStartBinding(nsIURL* aURL, const char *aContentType)
{
    nsresult rv = NS_OK;
    nsIContentViewer* viewer = nsnull;

    /* If the binding has been canceled via Stop() then abort the load... */
    if (NS_BINDING_ABORTED == mStatus) {
        rv = NS_BINDING_ABORTED;
        goto done;
    }

    if (nsnull == m_NextStream) {

        /*
         * Now that the content type is available, create a document (and viewer)
         * of the appropriate type...
         */
        if (m_DocLoader->m_DocFactory) {
            rv = m_DocLoader->m_DocFactory->CreateInstance(m_Url,
                                                           aContentType, 
                                                           m_Command, 
                                                           m_Container,
                                                           &m_NextStream, 
                                                           &viewer);
        } else {
            rv = NS_ERROR_NULL_POINTER;
        }

        if (NS_OK != rv) {
            goto done;
        }

        /*
         * Give the document container the new viewer...
         */
        viewer->SetContainer(m_Container);

        rv = m_Container->Embed(viewer, m_Command, m_ExtraInfo);
        if (NS_OK != rv) {
            goto done;
        }
    }

    /*
     * Pass the OnStartBinding(...) notification out to the document 
     * IStreamListener.
     */
    NS_ASSERTION((nsnull != m_NextStream), "No stream was created!");

    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnStartBinding(aURL, aContentType);
    }

    /* Pass the notification out to the Observer... */
    if (nsnull != m_Observer) {
        /* XXX: Should we ignore the return value? */
        (void) m_Observer->OnStartBinding(aURL, aContentType);
    }

done:
    NS_IF_RELEASE(viewer);

    return rv;
}


NS_METHOD nsDocumentBindInfo::OnDataAvailable(nsIURL* aURL, 
                                              nsIInputStream *aStream, PRInt32 aLength)
{
    nsresult rv = NS_OK;

    /* If the binding has been canceled via Stop() then abort the load... */
    if (NS_BINDING_ABORTED == mStatus) {
        rv = NS_BINDING_ABORTED;
        goto done;
    }

    NS_PRECONDITION(nsnull !=m_NextStream, "DocLoader: No stream for document");
    if (nsnull != m_NextStream) {
        rv = m_NextStream->OnDataAvailable(aURL, aStream, aLength);
    }

done:
    return rv;
}


NS_METHOD nsDocumentBindInfo::OnStopBinding(nsIURL* aURL, PRInt32 aStatus, 
                                            const nsString& aMsg)
{
    nsresult rv = NS_OK;

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
    m_DocLoader->LoadURLComplete((nsIStreamListener *)this);

    return rv;
}

NS_IMETHODIMP_(void)
nsDocumentBindInfo::Alert(const nsString &aText)
{
    if (nsnull != m_NetSupport) {
        m_NetSupport->Alert(aText);
    }
}

NS_IMETHODIMP_(PRBool)
nsDocumentBindInfo::Confirm(const nsString &aText)
{
    if (nsnull != m_NetSupport) {
        return m_NetSupport->Confirm(aText);
    }
    return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsDocumentBindInfo::Prompt(const nsString &aText,
                           const nsString &aDefault,
                           nsString &aResult)
{
    if (nsnull != m_NetSupport) {
        return m_NetSupport->Prompt(aText, aDefault, aResult);
    }
    return PR_FALSE;
}

NS_IMETHODIMP_(PRBool) 
nsDocumentBindInfo::PromptUserAndPassword(const nsString &aText,
                                          nsString &aUser,
                                          nsString &aPassword)
{
    if (nsnull != m_NetSupport) {
        return m_NetSupport->PromptUserAndPassword(aText, aUser, aPassword);
    }
    return PR_FALSE;
}

NS_IMETHODIMP_(PRBool) 
nsDocumentBindInfo::PromptPassword(const nsString &aText,
                                   nsString &aPassword)
{
    if (nsnull != m_NetSupport) {
        return m_NetSupport->PromptPassword(aText, aPassword);
    }
    return PR_FALSE;
}

NS_METHOD
nsDocumentBindInfo::RefreshURL(nsIURL* aURL, PRInt32 millis, PRBool repeat)
{
    if (nsnull != m_Container) {
        nsresult rv;
        nsIRefreshUrl* refresher = nsnull;

        /* Delegate the actual refresh call up-to the container. */
        rv = m_Container->QueryInterface(kRefreshURLIID, (void**)&refresher);

        if (rv != NS_OK) {
            PR_FALSE;
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

        if (rv != NS_OK) {
            PR_FALSE;
        }
        rv = refresher->CancelRefreshURLTimers();
        NS_RELEASE(refresher);
        return rv;
    }
    return PR_FALSE;
}

/*******************************************
 *  nsDocLoaderFactory
 *******************************************/

static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);
static NS_DEFINE_IID(kCDocumentLoader, NS_DOCUMENTLOADER_CID);

class nsDocumentLoaderFactory : public nsIFactory
{
public:
    nsDocumentLoaderFactory();

    NS_DECL_ISUPPORTS

    // nsIFactory methods
    NS_IMETHOD CreateInstance(nsISupports *aOuter,
                              const nsIID &aIID,
                              void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);


protected:
    virtual ~nsDocumentLoaderFactory();
};

nsDocumentLoaderFactory::nsDocumentLoaderFactory()
{
    NS_INIT_REFCNT();
}

nsDocumentLoaderFactory::~nsDocumentLoaderFactory()
{
}

/*
 * Implementation of ISupports methods...
 */
NS_IMPL_ISUPPORTS(nsDocumentLoaderFactory,kIFactoryIID);


NS_IMETHODIMP
nsDocumentLoaderFactory::CreateInstance(nsISupports* aOuter,
                                        const nsIID& aIID,
                                        void** aResult)
{
    nsresult rv;
    nsIDocumentLoader* inst;
    static nsDocLoaderImpl* gGlobalDocLoader = nsnull;


    if (nsnull == aResult) {
        rv = NS_ERROR_NULL_POINTER;
        goto done;
    }
    *aResult = nsnull;

    if (nsnull != aOuter) {
        rv = NS_ERROR_NO_AGGREGATION;
        goto done;
    }

    if (nsnull == gGlobalDocLoader) {
        gGlobalDocLoader = new nsDocLoaderImpl(nsnull);
        if (nsnull == gGlobalDocLoader) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto done;
        }

        NS_ADDREF(gGlobalDocLoader);    // RefCount = 1
    }

    rv = gGlobalDocLoader->CreateDocumentLoader(&inst);
    if (NS_OK != rv) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    *aResult = inst;

done:
    return rv;
}

NS_IMETHODIMP
nsDocumentLoaderFactory::LockFactory(PRBool aLock)
{
    // Not implemented in simplest case.
    return NS_OK;
}

extern "C" NS_WEB nsresult
NS_NewDocumentLoaderFactory(nsIFactory** aFactory)
{
    if (nsnull == aFactory) {
        return NS_ERROR_NULL_POINTER;
    }

    *aFactory = new nsDocumentLoaderFactory();
    if (nsnull == *aFactory) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*aFactory);

    return NS_OK;
}


