/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/*
  This program simply goes through the network service to grab a URL
  and write it to standard out...

  The program takes a single parameter: url to fetch
 */
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsIEventQueueService.h"
#include "nsINetService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsFileStream.h"
#include "nsFileSpec.h"
#include "nsMimeTypes.h"
#include "nsIPref.h"
#include "nsICharsetConverterManager.h"
#include "prprf.h"
#include "nsIAllocator.h" // for the CID
#include "nsURLFetcher.h"

#ifdef WIN32
#include "windows.h"
#endif

////////////////////////////////////////////////////////////////////////////////////
// THIS IS THE STUFF TO GET THE TEST HARNESS OFF THE GROUND
////////////////////////////////////////////////////////////////////////////////////
#if defined(XP_PC)
#define NETLIB_DLL "netlib.dll"
#define XPCOM_DLL  "xpcom32.dll"
#define MIME_DLL  "mime.dll"
#define PREF_DLL  "xppref32.dll"
#define UNICHAR_DLL  "uconv.dll"
#elif defined(XP_UNIX)
#define NETLIB_DLL "libnetlib"MOZ_DLL_SUFFIX
#define XPCOM_DLL  "libxpcom"MOZ_DLL_SUFFIX
#define MIME_DLL  "libmime.dll"MOZ_DLL_SUFFIX
#define PREF_DLL  "libxppref32"MOZ_DLL_SUFFIX
#define UNICHAR_DLL  "libunicharutil"MOZ_DLL_SUFFIX
#elif defined(XP_MAC)
#define NETLIB_DLL "NETLIB_DLL"
#define XPCOM_DLL  "XPCOM_DLL"
#define MIME_DLL   "MIME_DLL"
#define PREF_DLL  "XPPREF32_DLL"
#define UNICHAR_DLL  "UNICHARUTIL_DLL"
#endif

// {588595CB-2012-11d3-8EF0-00A024A7D144}
#define CONV_CID  { 0x1e3f79f1, 0x6b6b, 0x11d2, { 0x8a, 0x86, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36 } };
#define CONV_IID  { 0x1e3f79f0, 0x6b6b, 0x11d2, {  0x8a, 0x86, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36 } }

// CIDs

// I18N
static NS_DEFINE_CID(charsetCID,  CONV_CID);
NS_DEFINE_IID(kConvMeIID,             CONV_IID);

// prefs
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

// xpcom
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kGenericFactoryCID,    NS_GENERICFACTORY_CID);

// netlib definitions....
static NS_DEFINE_CID(kNetServiceCID, NS_NETSERVICE_CID);

nsICharsetConverterManager *ccMan = nsnull;

static nsresult
SetupRegistry(void)
{
  // i18n
  nsComponentManager::RegisterComponent(charsetCID, NULL, NULL, UNICHAR_DLL,  PR_FALSE, PR_FALSE);
  nsresult res = nsServiceManager::GetService(charsetCID, kConvMeIID, (nsISupports **)&ccMan);
  if (NS_FAILED(res)) 
  {
    printf("ERROR at GetService() code=0x%x.\n",res);
    return NS_ERROR_FAILURE;
  }

  // netlib
  nsComponentManager::RegisterComponent(kNetServiceCID,     NULL, NULL, NETLIB_DLL,  PR_FALSE, PR_FALSE);
  
  // xpcom
  static NS_DEFINE_CID(kAllocatorCID,  NS_ALLOCATOR_CID);
  static NS_DEFINE_CID(kEventQueueCID, NS_EVENTQUEUE_CID);
  nsComponentManager::RegisterComponent(kEventQueueServiceCID, NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);
  nsComponentManager::RegisterComponent(kEventQueueCID,        NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);
  nsComponentManager::RegisterComponent(kGenericFactoryCID,    NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);
  nsComponentManager::RegisterComponent(kAllocatorCID,         NULL, NULL, XPCOM_DLL,  PR_FALSE, PR_FALSE);

  // prefs
  nsComponentManager::RegisterComponent(kPrefCID,              NULL, NULL, PREF_DLL,  PR_FALSE, PR_FALSE);

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// END OF STUFF TO GET THE TEST HARNESS OFF THE GROUND
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
// THIS IS THE CLASS THAT WOULD BE IMPLEMENTED BY THE CONSUMER OF THE HTML OUPUT
// FROM LIBMIME. THIS EXAMPLE SIMPLY WRITES THE OUTPUT TO STDOUT()
////////////////////////////////////////////////////////////////////////////////////
class ConsoleOutputStreamImpl : public nsIOutputStream
{
public:
    ConsoleOutputStreamImpl(void) { NS_INIT_REFCNT(); }
    virtual ~ConsoleOutputStreamImpl(void) {}

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIBaseStream interface
    NS_IMETHOD Close(void) 
    {
      return NS_OK;
    }

    // nsIOutputStream interface
    NS_IMETHOD Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount) {
        PR_Write(PR_GetSpecialFD(PR_StandardOutput), aBuf, aCount);
        *aWriteCount = aCount;
        return NS_OK;
    }

    NS_IMETHOD Flush(void) {
        PR_Sync(PR_GetSpecialFD(PR_StandardOutput));
        return NS_OK;
    }
};
NS_IMPL_ISUPPORTS(ConsoleOutputStreamImpl, nsCOMTypeInfo<nsIOutputStream>::GetIID());
////////////////////////////////////////////////////////////////////////////////////
// END OF CONSUMER STREAM
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////

// Utility to create a nsIURI object...
nsresult 
nsMsgNewURL(nsIURI** aInstancePtrResult, const nsString& aSpec)
{  
  if (nsnull == aInstancePtrResult) 
    return NS_ERROR_NULL_POINTER;
  
  nsINetService *inet = nsnull;
  nsresult rv = nsServiceManager::GetService(kNetServiceCID, nsCOMTypeInfo<nsINetService>::GetIID(),
                                             (nsISupports **)&inet);
  if (rv != NS_OK) 
    return rv;

  rv = inet->CreateURL(aInstancePtrResult, aSpec, nsnull, nsnull, nsnull);
  nsServiceManager::ReleaseService(kNetServiceCID, inet);
  return rv;
}

void
FlushEvents(void)
{
#ifdef WIN32
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
#endif
}

nsresult 
MyCallback(nsIURI* aURL, nsresult aStatus, PRInt32 totalSize, const PRUnichar* aMsg, void *tagData)
{
  printf("Done with URL Request. Exit Code = [%d] - Received [%d] bytes\n", aStatus, totalSize);
  return NS_OK;
}

int
main(int argc, char** argv)
{
  nsresult  rv;
  nsIURI    *aURL;
  
  // Do some sanity checking...
  if (argc < 3) 
  {
    fprintf(stderr, "usage: %s <url> <output_file>\n", argv[0]);
    return 1;
  }
  
  // Setup the registry...
  SetupRegistry();

  // Get an event queue and get it started...
  NS_WITH_SERVICE(nsIEventQueueService, theEventQueueService, kEventQueueServiceCID, &rv);
  if (NS_FAILED(rv)) 
    return rv;
  rv = theEventQueueService->CreateThreadEventQueue();
  NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create thread event queue");
  if (NS_FAILED(rv)) 
    return rv;

  // Create an nsIURI object needed for the URL operation IO...
  if (NS_FAILED(nsMsgNewURL(&aURL, nsString(argv[1]))))
  {
    printf("Unable to create URL\n");
    return NS_ERROR_FAILURE;
  }

  // Create a fetcher for the URL attachment...
  nsURLFetcher *sFetcher = new nsURLFetcher();
  if (!sFetcher)
  {
    printf("Failed to create an attachment stream listener...\n");
    return rv;
  }

  nsFileSpec  fSpec(argv[2]);
  nsOutputFileStream dstFile(fSpec, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE );
  if (!dstFile.is_open() )
  {
    printf("Failed to open the output file!\n");
    return rv;
  }

  rv = sFetcher->FireURLRequest(aURL, &dstFile, MyCallback, sFetcher);
  if (NS_FAILED(rv)) 
  {
    printf("Failed to fire the URL request\n");
    return rv;
  }

  PRBool running = PR_TRUE; 
  while (running)
  {
    sFetcher->StillRunning(&running);
    FlushEvents();
  }

  NS_RELEASE(sFetcher);

  // Cleanup stuff necessary...
  NS_RELEASE(ccMan);
  return NS_OK;
}

