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
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#define NS_IMPL_IDS

#include "nsCOMPtr.h"
#include "nsIPersistentProperties2.h"
#include "nsIStringBundle.h"
#include "nsIEventQueueService.h"
#include "nsILocale.h"
#include <iostream.h>

#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
//
#include "nsFileLocations.h"
#include "nsIFileLocator.h"
#include "nsIFileSpec.h"

#define TEST_URL "resource:/res/strres.properties"

#ifdef XP_PC
#define NETLIB_DLL "netlib.dll"
#define RAPTORBASE_DLL "raptorbase.dll"
#define XPCOM_DLL "xpcom32.dll"
#else /* else XP_PC */
#ifdef XP_MAC
#define NETLIB_DLL "NETLIB_DLL"
#define RAPTORBASE_DLL "base.shlb"
#define XPCOM_DLL "XPCOM_DLL"
#else /* else XP_MAC */
#define NETLIB_DLL "libnetlib"MOZ_DLL_SUFFIX
#define RAPTORBASE_DLL "libraptorbase"MOZ_DLL_SUFFIX
#define XPCOM_DLL "libxpcom"MOZ_DLL_SUFFIX
#endif /* XP_MAC */
#endif /* XP_PC */

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_IID(kIEventQueueServiceIID, NS_IEVENTQUEUESERVICE_IID);

static NS_DEFINE_IID(kIIOServiceIID, NS_IIOSERVICE_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_IID(kIStringBundleServiceIID, NS_ISTRINGBUNDLESERVICE_IID);

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// let add some locale stuff
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsLocaleCID.h"

//
//
//
nsILocale*
get_applocale(void)
{
	nsresult			result;
	nsILocale*			locale;
	nsString*			category;
	nsString*			value;
	PRUnichar *lc_name_unichar;

	// get a locale service 
	nsCOMPtr<nsILocaleService> localeService = do_GetService(NS_LOCALESERVICE_PROGID, &result);
	NS_ASSERTION(NS_SUCCEEDED(result),"nsLocaleTest: get locale service failed");

	//
	// test GetApplicationLocale
	//
	result = localeService->GetApplicationLocale(&locale);
	NS_ASSERTION(NS_SUCCEEDED(result),"nsLocaleTest: factory_get_locale failed");
	NS_ASSERTION(locale!=NULL,"nsLocaleTest: factory_get_locale failed");

	//
	// test and make sure the locale is a valid Interface
	//
	locale->AddRef();

	category = new nsString();
	category->AssignWithConversion("NSILOCALE_MESSAGES");
	value = new nsString();

	result = locale->GetCategory(category->GetUnicode(),&lc_name_unichar);
	value->Assign(lc_name_unichar);
	NS_ASSERTION(NS_SUCCEEDED(result),"nsLocaleTest: factory_get_locale failed");
	NS_ASSERTION(value->Length()>0,"nsLocaleTest: factory_get_locale failed");

	locale->Release();
	delete category;
	delete value;

    return locale;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// end of locale stuff
//
////////////////////////////////////////////////////////////////////////////////////////////////////
nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, 
                                                 NULL /* default */);

	// startup netlib:	
	nsComponentManager::RegisterComponent(kEventQueueServiceCID, 
                                        NULL, NULL, 
                                        XPCOM_DLL, 
                                        PR_FALSE, PR_FALSE);
  nsComponentManager::RegisterComponent(kIOServiceCID, NULL, NULL, NETLIB_DLL, PR_FALSE, PR_FALSE);

  // Create the Event Queue for this thread...
  nsIEventQueueService* pEventQService;
  
  pEventQService = nsnull;
  nsresult result = nsServiceManager::GetService(kEventQueueServiceCID,
                                                 kIEventQueueServiceIID,
                                                 (nsISupports **)&pEventQService);
  if (NS_SUCCEEDED(result)) {
    // XXX: What if this fails?
    result = pEventQService->CreateThreadEventQueue();
  }
  
  nsComponentManager::RegisterComponent(kPersistentPropertiesCID, 
                                        NULL,
                                        NULL, 
                                        RAPTORBASE_DLL, 
                                        PR_FALSE, 
                                        PR_FALSE);
  return rv;
}

int
main(int argc, char *argv[])
{
  nsresult ret;

  NS_AutoregisterComponents();

  nsIStringBundleService* service = nsnull;
  ret = nsServiceManager::GetService(kStringBundleServiceCID,
                                     kIStringBundleServiceIID, (nsISupports**) &service);
  if (NS_FAILED(ret)) {
    printf("cannot create service\n");
    return 1;
  }

  nsILocale* locale = get_applocale();

  nsIStringBundle* bundle = nsnull;

  ret = service->CreateBundle(TEST_URL, locale, &bundle);

  /* free it
  */
  locale->Release();

  if (NS_FAILED(ret)) {
    printf("cannot create instance\n");
    return 1;
  }

  nsAutoString v;
  v.AssignWithConversion("");
  PRUnichar *ptrv = nsnull;
  char *value = nsnull;

  // 123
  ret = bundle->GetStringFromID(123, &ptrv);
  if (NS_FAILED(ret)) {
    printf("cannot get string from ID 123, ret=%d\n", ret);
    return 1;
  }
  v = ptrv;
  value = v.ToNewCString();
  cout << "123=\"" << value << "\"" << endl;

  // file
  nsString strfile;
  strfile.AssignWithConversion("file");
  const PRUnichar *ptrFile = strfile.GetUnicode();
  ret = bundle->GetStringFromName(ptrFile, &ptrv);
  if (NS_FAILED(ret)) {
    printf("cannot get string from name\n");
    return 1;
  }
  v = ptrv;
  value = v.ToNewCString();
  cout << "file=\"" << value << "\"" << endl;

 nsIBidirectionalEnumerator* propEnum = nsnull;
  ret = bundle->GetEnumeration(&propEnum);
  if (NS_FAILED(ret)) {
    printf("cannot get enumeration\n");
    return 1;
  }
  ret = propEnum->First();
  if (NS_FAILED(ret))
  {
  printf("enumerator is empty\n");
  return 1;
  }

  cout << endl << "Key" << "\t" << "Value" << endl;
  cout <<		  "---" << "\t" << "-----" << endl;
  while (NS_SUCCEEDED(ret))
  {
    nsIPropertyElement* propElem = nsnull;
    ret = propEnum->CurrentItem((nsISupports**)&propElem);
    if (NS_FAILED(ret)) {
    printf("failed to get current item\n");
    return 1;
    }

    PRUnichar *pKey = nsnull;
    PRUnichar *pVal = nsnull;

    ret = propElem->GetKey(&pKey);
    if (NS_FAILED(ret)) {
      printf("failed to get current element's key\n");
      return 1;
    }
    ret = propElem->GetValue(&pVal);
    if (NS_FAILED(ret)) {
      printf("failed to get current element's value\n");
      return 1;
    }

    nsAutoString keyAdjustedLengthBuff(pKey);
    nsAutoString valAdjustedLengthBuff(pVal);

    char* keyCStr = keyAdjustedLengthBuff.ToNewCString();
    char* valCStr = valAdjustedLengthBuff.ToNewCString();
    if (keyCStr && valCStr) 
    cout << keyCStr << "\t" << valCStr << endl;
    delete[] keyCStr;
    delete[] valCStr;
    delete[] pKey;
    delete[] pVal;

    ret = propEnum->Next();
  }

  return 0;
}
