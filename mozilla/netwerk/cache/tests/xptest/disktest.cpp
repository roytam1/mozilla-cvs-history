/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 */

/* 
    The TestProtocols tests the basic protocols architecture and can 
    be used to test individual protocols as well. If this grows too
    big then we should split it to individual protocols. 
    
    -Gagan Saksena 04/29/99
*/

/*
    This program would actually create a cache file in your /tmp
	directory. It would try to use the default name from URI. If
	that name can not be found, it will use nucache. Also, a database
	file (for indexing, I guess) named nufat.db would also be generated 
	in the same directory.

	This is only a temporary solution. We are still waiting for the final
	decision on which database to use.

	-yixiong 08/10/99
*/

#include <stdio.h>
#ifdef WIN32 
#include <windows.h>
#endif
#include "nspr.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIEventQueueService.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIBufferInputStream.h"
#include "nsCRT.h"
#include "nsIChannel.h"
#include "nsIURL.h"
#include "nsIHTTPChannel.h"
#include "nsIHttpEventSink.h" 
#include "nsICapabilities.h" 
#include "nsIDNSService.h" 

#include "nsISimpleEnumerator.h"
#include "nsIHTTPHeader.h"
#include "nsXPIDLString.h"

#ifdef CACHE
#include "nsICacheManager.h"
#include "nsICacheObject.h"
#endif

// this test app handles cookies.
#include "nsICookieService.h"
static NS_DEFINE_CID(nsCookieServiceCID, NS_COOKIESERVICE_CID);

static NS_DEFINE_CID(kEventQueueServiceCID,      NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID,              NS_IOSERVICE_CID);

#ifdef CACHE
static NS_DEFINE_IID(kICacheManagerIID, NS_ICACHEMANAGER_IID) ;
static NS_DEFINE_IID(kCacheManagerCID, NS_CACHEMANAGER_CID) ;

static NS_DEFINE_IID(kICacheObjectIID, NS_ICACHEOBJECT_IID) ;
static NS_DEFINE_CID(kCacheObjectCID, NS_CACHEOBJECT_CID);
#endif

//static PRTime gElapsedTime; // enable when we time it...
static int gKeepRunning = 0;
static PRBool gVerbose = PR_FALSE;
static nsIEventQueue* gEventQ = nsnull;

class URLLoadInfo : public nsISupports
{
public:

  URLLoadInfo(const char* aUrl);
  virtual ~URLLoadInfo();

  // ISupports interface...
  NS_DECL_ISUPPORTS

  const char* Name() { return mURLString.GetBuffer(); }
  PRInt32   mBytesRead;
  PRTime    mTotalTime;
  PRTime    mConnectTime;
  nsString  mURLString;
};

URLLoadInfo::URLLoadInfo(const char *aUrl) : mURLString(aUrl)
{
  NS_INIT_REFCNT();

  mBytesRead = 0;
  mConnectTime = mTotalTime = PR_Now();
}

URLLoadInfo::~URLLoadInfo()
{
}


NS_IMPL_ISUPPORTS(URLLoadInfo,nsCOMTypeInfo<nsISupports>::GetIID());


class TestHTTPEventSink : public nsIHTTPEventSink
{
public:

  TestHTTPEventSink();
  virtual ~TestHTTPEventSink();

  // ISupports interface...
  NS_DECL_ISUPPORTS

  // nsIHTTPEventSink interface...
  NS_IMETHOD      OnAwaitingInput(nsISupports* i_Context);

  NS_IMETHOD      OnHeadersAvailable(nsISupports* i_Context);

  NS_IMETHOD      OnProgress(nsISupports* i_Context, 
                             PRUint32 i_Progress, 
                             PRUint32 i_ProgressMax);

  // OnRedirect gets fired only if you have set FollowRedirects on the handler!
  NS_IMETHOD      OnRedirect(nsISupports* i_Context, 
                             nsIURI* i_NewLocation);
};

TestHTTPEventSink::TestHTTPEventSink()
{
  NS_INIT_REFCNT();
}

TestHTTPEventSink::~TestHTTPEventSink()
{
}


NS_IMPL_ISUPPORTS(TestHTTPEventSink,nsCOMTypeInfo<nsIHTTPEventSink>::GetIID());

NS_IMETHODIMP
TestHTTPEventSink::OnAwaitingInput(nsISupports* context)
{
    printf("\n+++ TestHTTPEventSink::OnAwaitingInput +++\n");
    return NS_OK;
}

NS_IMETHODIMP
TestHTTPEventSink::OnHeadersAvailable(nsISupports* context)
{
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    nsCOMPtr<nsIHTTPChannel> pHTTPCon(do_QueryInterface(context));
    PRBool bMoreHeaders;

	printf("cachetest: 162, OnHeadersAvailable, \n") ;

    if (pHTTPCon) {
        pHTTPCon->GetRequestHeaderEnumerator(getter_AddRefs(enumerator));

        printf("Request headers:\n");
        enumerator->HasMoreElements(&bMoreHeaders);
        while (bMoreHeaders) {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIHTTPHeader> header;

            enumerator->GetNext(getter_AddRefs(item));
            header = do_QueryInterface(item);

            if (header) {
                nsCOMPtr<nsIAtom> key;
                nsAutoString field(eOneByte);
                nsXPIDLCString value;

                header->GetField(getter_AddRefs(key));
                key->ToString(field);
                printf("\t%s: ", field.GetBuffer());

                header->GetValue(getter_Copies(value));
                printf("%s\n", (const char*)value);
            }
    
            enumerator->HasMoreElements(&bMoreHeaders);
        }

        pHTTPCon->GetResponseHeaderEnumerator(getter_AddRefs(enumerator));

        printf("Response headers:\n");
        enumerator->HasMoreElements(&bMoreHeaders);
        while (bMoreHeaders) {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIHTTPHeader> header;

            enumerator->GetNext(getter_AddRefs(item));
            header = do_QueryInterface(item);

            if (header) {
                nsCOMPtr<nsIAtom> key;
                nsAutoString field(eOneByte);
                nsXPIDLCString value;

                header->GetField(getter_AddRefs(key));
                key->ToString(field);
                printf("\t%s: ", field.GetBuffer());

                header->GetValue(getter_Copies(value));
                printf("%s\n", (const char*)value);
            }
    
            enumerator->HasMoreElements(&bMoreHeaders);
        }

    }


    if (gVerbose) {
        printf("\n+++ TestHTTPEventSink::OnHeadersAvailable +++\n");
        nsCOMPtr<nsIHTTPChannel> pHTTPCon(do_QueryInterface(context));
        if (pHTTPCon) {
            char* type;
            //optimize later TODO allow atoms here...! intead of just the header strings
            pHTTPCon->GetContentType(&type);
            if (type) {
                printf("\nReceiving ... %s\n", type);
                nsCRT::free(type);
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
TestHTTPEventSink::OnProgress(nsISupports* context, PRUint32 i_Progress, PRUint32 i_ProgressMax)
{
    printf("\n+++ TestHTTPEventSink::OnProgress +++\n");
    return NS_OK;
}

NS_IMETHODIMP
TestHTTPEventSink::OnRedirect(nsISupports* context, nsIURI* i_NewLocation)
{
    printf("\n+++ TestHTTPEventSink::OnRedirect +++\n");
    return NS_OK;
}


class InputTestConsumer : public nsIStreamListener
{
public:

  InputTestConsumer();
  virtual ~InputTestConsumer();

  // ISupports interface...
  NS_DECL_ISUPPORTS

  // IStreamListener interface...
  NS_IMETHOD OnStartRequest(nsIChannel* channel, nsISupports* context);

  NS_IMETHOD OnDataAvailable(nsIChannel* channel, nsISupports* context,
                             nsIInputStream *aIStream, 
                             PRUint32 aSourceOffset,
                             PRUint32 aLength);

  NS_IMETHOD OnStopRequest(nsIChannel* channel, nsISupports* context,
                           nsresult aStatus,
                           const PRUnichar* aMsg);

#ifdef CACHE
  private:

  nsICacheObject* mCacheFile ;
#endif 
};


InputTestConsumer::InputTestConsumer()
{
  NS_INIT_REFCNT();

#ifdef CACHE
  mCacheFile = nsnull ;
#endif
}

InputTestConsumer::~InputTestConsumer()
{
#ifdef CACHE
  NS_IF_RELEASE(mCacheFile) ;
#endif
}


NS_IMPL_ISUPPORTS(InputTestConsumer,nsCOMTypeInfo<nsIStreamListener>::GetIID());


NS_IMETHODIMP
InputTestConsumer::OnStartRequest(nsIChannel* channel, nsISupports* context)
{
#ifdef CACHE
  /* Stuff taken from nsPluginHostImpl.cpp */

  char * aContentType = nsnull ;
  nsIURI* aURL = nsnull ;
  nsICacheModule* diskCache ;
  nsICacheManager* cacheManager ;

  nsresult rv = NS_OK, r1, r2 ;

  rv = channel->GetContentType(&aContentType) ;
  if (NS_FAILED(rv)) return rv ;

  rv = channel ->GetURI(&aURL) ;
  if (NS_FAILED(rv)) return rv ;

  char * cString;
  char * fileName ;

  nsCOMPtr<nsIURL> pURL(do_QueryInterface(aURL)) ;

  if (pURL) {
    pURL -> GetSpec(&cString) ;

  /* Todo, use PROGID instead */
    rv = nsComponentManager::CreateInstance(kCacheObjectCID,
                                          nsnull,
										  kICacheObjectIID,
										  (void**)&mCacheFile) ;

    if (NS_FAILED(rv)) {
      return rv ;
    }

	r1 = NS_ERROR_FAILURE ;
    r1 = mCacheFile->SetAddress(cString) ;

  /* use the actual filname of the net-based file as the cache filename */
    r1 = pURL->GetFileName(&fileName) ;

	if(!fileName) {
	  fileName=new char[50] ;
	  strcpy(fileName, "nucache") ;
	}

    r2 = mCacheFile ->SetFilename(fileName) ;

	delete [] fileName ;
  }

  /* todo: use PROGID instead */
  rv = nsServiceManager::GetService(kCacheManagerCID,
										  kICacheManagerIID,
										  (nsISupports **)&cacheManager) ;
  if (NS_FAILED(rv)) return rv ;

  // cacheManager->Init( ) ;

  rv = cacheManager->GetDiskModule(&diskCache) ;
  if (NS_FAILED(rv)) return rv ;

  rv = diskCache->AddObject(mCacheFile) ;

#endif

  URLLoadInfo* info = (URLLoadInfo*)context;
  if (info) {
    info->mConnectTime = PR_Now() - info->mConnectTime;
  }

  if (gVerbose) {
    printf("\nStarted loading: %s\n", info ? info->Name() : "UNKNOWN URL");
  }
/*
  nsCOMPtr<nsIURI> pURI(do_QueryInterface(context));
  char* location = nsnull;

  if (pURI) {
    pURI->GetSpec(&location);
  }

  printf("\nStarted loading: %s\n", location ? location : "UNKNOWN URL");
  if (location) {
    nsCRT::free(location);
  }
*/
  return NS_OK;
}


NS_IMETHODIMP
InputTestConsumer::OnDataAvailable(nsIChannel* channel, 
                                   nsISupports* context,
                                   nsIInputStream *aIStream, 
                                   PRUint32 aSourceOffset,
                                   PRUint32 aLength)
{
  char buf[1025];
  PRUint32 amt, amt_wrote;
  nsresult rv;
  URLLoadInfo* info = (URLLoadInfo*)context;

  do {
    rv = aIStream->Read(buf, 1024, &amt);

#ifdef CACHE
	if (nsnull != mCacheFile ) {
	  mCacheFile->Write((char*)buf, amt, &amt_wrote) ;
	}
#endif

    if (rv == NS_BASE_STREAM_EOF) break;
    if (NS_FAILED(rv)) return rv;
    if (gVerbose) {
      buf[amt] = '\0';
      puts(buf);
    }
    if (info) {
      info->mBytesRead += amt;
    }
  } while (amt);

  return NS_OK;
}


NS_IMETHODIMP
InputTestConsumer::OnStopRequest(nsIChannel* channel, 
                                 nsISupports* context,
                                 nsresult aStatus,
                                 const PRUnichar* aMsg)
{
  URLLoadInfo* info = (URLLoadInfo*)context;

  if (info) {
    double connectTime;
    double readTime;
    PRUint32 httpStatus, o_amt=0, l ;
	PRInt16 m ;
    PRBool bHTTPURL = PR_FALSE, b;
	char buf[2000], *s ;
	PRIntervalTime t ;

	if (mCacheFile) {
	  mCacheFile->GetLastModified(&t) ;
	  printf("\nlast modified: %d\n", t) ;

	  mCacheFile->GetEtag(&s) ;
	  printf("etag: %s\n", s) ;

	  mCacheFile->GetContentType(&s) ;
	  printf("content type: %s\n", s) ;

	  mCacheFile->GetCharset(&s) ;
	  printf("character set: %s\n", s) ;

	  mCacheFile->GetContentEncoding(&s) ;
	  printf("Content encoding: %s\n", s) ;
	  
	  mCacheFile->GetContentLength(&l) ; 
	  printf("Content Length: %d\n", l) ;

	  mCacheFile->GetExpires(&t) ;
	  printf("Expires: %d\n", t) ;

	  mCacheFile->GetFilename(&s) ;
	  printf("Filename: %s\n", s) ;

      mCacheFile->GetState(&l) ;
	  printf("State: %d\n", l) ;

	  mCacheFile->GetInfoSize(&l) ;
	  printf("Info size: %d\n", l) ;

      mCacheFile->IsPartial(&b) ;
	  printf("Is completed: %s\n", b?"True":"False") ;

	  mCacheFile->GetLastAccessed(&t) ;
	  printf("Last Access time: %d\n", t) ;

	  mCacheFile->GetLastModified(&t) ;
	  printf("Last Modified time: %d\n", t) ;

	  mCacheFile->GetModuleIndex(&m) ;
	  printf("Module: %d\n", m) ;

	  mCacheFile->GetPageServicesURL(&s) ;
	  printf("Page Service URL: %s\n", s) ;

	  mCacheFile->GetPostData(&s) ;
	  printf("Post Data: %s\n", s) ;

	  mCacheFile->GetPostDataLen(&l) ;
	  printf("Post Data Length: %d\n", l) ;

	  mCacheFile->GetSize(&l) ;
	  printf("Size: %d\n", l) ;

	  mCacheFile->GetAddress(&s) ;
	  printf("URL Address: %s \n", s) ;

	  /* I know my test webpage is less than 2000 bytes. yixiong */
	  mCacheFile->Read (buf, (PRUint32) 2000, &o_amt) ;
	  buf[o_amt] = '\0' ;
//	  printf("%s\n   o_amt = %d \n", buf, o_amt) ;
	}

    info->mTotalTime = PR_Now() - info->mTotalTime;

    connectTime = (info->mConnectTime/1000.0)/1000.0;
    readTime    = ((info->mTotalTime-info->mConnectTime)/1000.0)/1000.0;

    nsCOMPtr<nsIHTTPChannel> pHTTPCon(do_QueryInterface(channel));
    if (pHTTPCon) {
        pHTTPCon->GetResponseStatus(&httpStatus);
        bHTTPURL = PR_TRUE;
    }

    printf("\nFinished loading: %s  Status Code: %x\n", info->Name(), aStatus);
    if (bHTTPURL)
        printf("\tHTTP Status: %u\n", httpStatus);
     if (NS_ERROR_UNKNOWN_HOST == aStatus) {
         printf("\tDNS lookup failed.\n");
     }
    printf("\tRead: %d bytes.\n", info->mBytesRead);
    printf("\tTime to connect: %.3f seconds\n", connectTime);
    printf("\tTime to read: %.3f seconds.\n", readTime);
    if (readTime > 0.0) {
      printf("\tThroughput: %.0f bps.\n", (info->mBytesRead*8)/readTime);
    } else {
      printf("\tThroughput: REAL FAST!!\n");
    }
  } else {
    printf("\nFinished loading: UNKNOWN URL. Status Code: %x\n", aStatus);
  }
/*
  nsCOMPtr<nsIURI> pURI(do_QueryInterface(context));
  char* location = nsnull;

  if (pURI) {
    pURI->GetSpec(&location);
  }

  printf("\nFinished loading: %s  Status Code: %x\n", location ? location : "UNKNOWN URL", aStatus);

  if (location) {
    nsCRT::free(location);
  }
*/
  gKeepRunning -= 1;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

class nsNotificationCallbacks : public nsICapabilities {
public:
    NS_DECL_ISUPPORTS

    nsNotificationCallbacks() {
        NS_INIT_REFCNT();
    }

    NS_IMETHOD QueryCapability(const nsIID& eventSinkIID, nsISupports* *result) {
        nsresult rv = NS_ERROR_FAILURE;

        if (eventSinkIID.Equals(NS_GET_IID(nsIHTTPEventSink))) {
            TestHTTPEventSink *sink;

            sink = new TestHTTPEventSink();
            if (sink == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;
            NS_ADDREF(sink);
            rv = sink->QueryInterface(eventSinkIID, (void**)result);
            NS_RELEASE(sink);
        }
        return rv;
    }
};

NS_IMPL_ISUPPORTS1(nsNotificationCallbacks, nsICapabilities)

////////////////////////////////////////////////////////////////////////////////


nsresult StartLoadingURL(const char* aUrlString)
{
    nsresult rv;

    NS_WITH_SERVICE(nsIIOService, pService, kIOServiceCID, &rv);
    if (pService) {
        nsCOMPtr<nsIURI> pURL;

        rv = pService->NewURI(aUrlString, nsnull, getter_AddRefs(pURL));
        if (NS_FAILED(rv)) {
            printf("ERROR: NewURI failed for %s\n", aUrlString);
            return rv;
        }
        nsCOMPtr<nsIChannel> pChannel;
        nsNotificationCallbacks* callbacks;

        callbacks = new nsNotificationCallbacks();
        if (!callbacks) {
            NS_ERROR("Failed to create a new consumer!");
            return NS_ERROR_OUT_OF_MEMORY;;
        }
        NS_ADDREF(callbacks);

        // Async reading thru the calls of the event sink interface
        rv = pService->NewChannelFromURI("load", pURL, nsnull, callbacks, 
                                         getter_AddRefs(pChannel));
        NS_RELEASE(callbacks);
        if (NS_FAILED(rv)) {
            printf("ERROR: NewChannelFromURI failed for %s\n", aUrlString);
            return rv;
        }

        /* 
           You may optionally add/set other headers on this
           request object. This is done by QI for the specific
           protocolConnection.
        */
        nsCOMPtr<nsIHTTPChannel> pHTTPCon(do_QueryInterface(pChannel));

        if (pHTTPCon) {
            // Setting a sample user agent string.
            nsCOMPtr<nsIAtom> userAgent;

            userAgent = NS_NewAtom("user-agent");
            rv = pHTTPCon->SetRequestHeader(userAgent, "Mozilla/5.0 [en] (Win98; U)");
            if (NS_FAILED(rv)) return rv;
        }
            
        InputTestConsumer* listener;

        listener = new InputTestConsumer;
        NS_IF_ADDREF(listener);
        if (!listener) {
            NS_ERROR("Failed to create a new stream listener!");
            return NS_ERROR_OUT_OF_MEMORY;;
        }

        URLLoadInfo* info;
        info = new URLLoadInfo(aUrlString);
        NS_IF_ADDREF(info);
        if (!info) {
            NS_ERROR("Failed to create a load info!");
            return NS_ERROR_OUT_OF_MEMORY;;
        }
        

        rv = pChannel->AsyncRead(0,         // staring position
                                 -1,        // number of bytes to read
                                 info,      // ISupports context
                                 listener); // IStreamListener consumer
        if (NS_SUCCEEDED(rv)) {
            gKeepRunning += 1;
        }
        NS_RELEASE(listener);
        NS_RELEASE(info);
    }

    return rv;
}


nsresult LoadURLsFromFile(char *aFileName)
{
    nsresult rv = NS_OK;
    PRInt32 len, offset;
    PRFileDesc* fd;
    char buffer[1024];
    nsString fileBuffer;
    nsAutoString urlString(eOneByte);

    fd = PR_Open(aFileName, PR_RDONLY, 777);
    if (!fd) {
        return NS_ERROR_FAILURE;
    }

    // Keep reading the file until EOF (or an error) is reached...        
    do {
        len = PR_Read(fd, buffer, sizeof(buffer));
        if (len>0) {
            fileBuffer.Append(buffer, len);
            // Treat each line as a URL...
            while ((offset = fileBuffer.FindChar('\n')) != -1) {
                fileBuffer.Left(urlString, offset);
                fileBuffer.Cut(0, offset+1);

                urlString.StripChars("\r");
                if (urlString.Length()) {
                    printf("\t%s\n", urlString.GetBuffer());
                    rv = StartLoadingURL(urlString.GetBuffer());
                }
            }
        }
    } while (len>0);

    // If anything is left in the fileBuffer, treat it as a URL...
    fileBuffer.StripChars("\r");
    if (fileBuffer.Length()) {
        printf("\t%s\n", fileBuffer.GetBuffer());
        StartLoadingURL(fileBuffer.GetBuffer());
    }
    
    PR_Close(fd);
    return NS_OK;
}


nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL /* default */);
  return rv;
}

int
main(int argc, char* argv[])
{
    nsresult rv= (nsresult)-1;
    if (argc < 2) {
        printf("usage: %s [-verbose] [-file <name>] <url> <url> ... \n", argv[0]);
        return -1;
    }

    /* 
      The following code only deals with XPCOM registration stuff. and setting
      up the event queues. Copied from TestSocketIO.cpp
    */

    rv = NS_AutoregisterComponents();
    if (NS_FAILED(rv)) return rv;

    
    // Create the Event Queue for this thread...
    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = eventQService->CreateThreadEventQueue();
    if (NS_FAILED(rv)) return rv;

    eventQService->GetThreadEventQueue(PR_CurrentThread(), &gEventQ);

#if 0 // Jud sez 
    // fire up an instance of the cookie manager.
    // I'm doing this using the serviceManager for convenience's sake.
    // Presumably an application will init it's own cookie service a 
    // different way (this way works too though).
    NS_WITH_SERVICE(nsICookieService, cookieService, nsCookieServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
#endif // NECKO

    int i;
    printf("\nTrying to load:\n");
    for (i=1; i<argc; i++) {
        // Turn on verbose printing...
        if (PL_strcasecmp(argv[i], "-verbose") == 0) {
            gVerbose = PR_TRUE;
            continue;
        } 

        // Turn on netlib tracing...
        if (PL_strcasecmp(argv[i], "-file") == 0) {
            LoadURLsFromFile(argv[++i]);
            continue;
        } 

        printf("\t%s\n", argv[i]);
        rv = StartLoadingURL(argv[i]);
    }

  // Enter the message pump to allow the URL load to proceed.
    while ( gKeepRunning ) {
#ifdef WIN32
        MSG msg;

        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            gKeepRunning = 0;
    }
#else
#ifdef XP_MAC
    /* Mac stuff is missing here! */
#else
    PLEvent *gEvent;
    rv = gEventQ->GetEvent(&gEvent);
    rv = gEventQ->HandleEvent(gEvent);
#endif /* XP_UNIX */
#endif /* !WIN32 */
    }

    return rv;
}
