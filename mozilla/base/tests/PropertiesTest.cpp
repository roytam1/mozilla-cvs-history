/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL. You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All Rights
 * Reserved.
 */

#define NS_IMPL_IDS

#include "nsXPComCIID.h"
#include "nsIEventQueueService.h"
#include "nsINetService.h"
#include "nsIProperties.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsRepository.h"
#ifdef XP_PC
#include "plevent.h"
#endif

#define TEST_URL "resource:/res/test.properties"

#define NETLIB_DLL "netlib.dll"
#define RAPTORBASE_DLL "raptorbase.dll"
#define XPCOM_DLL "xpcom32.dll"

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);
static NS_DEFINE_IID(kINetServiceIID, NS_INETSERVICE_IID);
static NS_DEFINE_IID(kIPropertiesIID, NS_IPROPERTIES_IID);
static NS_DEFINE_IID(kNetServiceCID, NS_NETSERVICE_CID);

int
main(int argc, char *argv[])
{
  nsRepository::RegisterFactory(kNetServiceCID, NETLIB_DLL, PR_FALSE,
    PR_FALSE);
  nsRepository::RegisterFactory(kEventQueueServiceCID, XPCOM_DLL,
    PR_FALSE, PR_FALSE);
  nsresult ret;
  nsIEventQueueService* pEventQueueService = nsnull;
  ret = nsServiceManager::GetService(kEventQueueServiceCID,
    kIEventQueueServiceIID, (nsISupports**) &pEventQueueService);
  if (NS_FAILED(ret)) {
    printf("cannot get event queue service\n");
    return 1;
  }
  ret = pEventQueueService->CreateThreadEventQueue();
  if (NS_FAILED(ret)) {
    printf("CreateThreadEventQueue failed\n");
    return 1;
  }
  nsINetService* pNetService = nsnull;
  ret = nsServiceManager::GetService(kNetServiceCID, kINetServiceIID,
    (nsISupports**) &pNetService);
  if (NS_FAILED(ret)) {
    printf("cannot get net service\n");
    return 1;
  }
  nsIURL *url = nsnull;
  ret = pNetService->CreateURL(&url, nsString(TEST_URL), nsnull, nsnull,
    nsnull);
  if (NS_FAILED(ret)) {
    printf("cannot create URL\n");
    return 1;
  }
  nsIInputStream *in = nsnull;
  ret = pNetService->OpenBlockingStream(url, nsnull, &in);
  if (NS_FAILED(ret)) {
    printf("cannot open stream\n");
    return 1;
  }
  nsIProperties *props = nsnull;
  ret = nsRepository::CreateInstance(kPropertiesCID, NULL,
    kIPropertiesIID, (void**) &props);
  if (NS_FAILED(ret)) {
    printf("create nsIProperties failed\n");
    return 1;
  }
  props->Load(in);
  int i = 1;
  while (1) {
    char name[16];
    sprintf(name, "%d", i);
    nsAutoString v("");
    props->GetProperty(name, v);
    if (!v.Length()) {
      break;
    }
    char *value = v.ToNewCString();
    cout << "\"" << i << "\"=\"" << value << "\"" << endl;
    i++;
  }

  return 0;
}
