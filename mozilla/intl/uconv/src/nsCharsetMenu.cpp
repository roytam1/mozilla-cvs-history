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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIServiceManager.h"
#include "nsIComponentManager.h" 
#include "rdf.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFContainerUtils.h"
#include "nsRDFCID.h"
#include "nsXPIDLString.h"
#include "nsCharsetMenu.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsUConvDll.h"
#include "nsICollation.h"
#include "nsCollationCID.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsIPref.h"
#include "nsICurrentCharsetListener.h"
#include "nsQuickSort.h"

//----------------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID, NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFContainerCID, NS_RDFCONTAINER_CID);
static NS_DEFINE_CID(kCollationFactoryCID, NS_COLLATIONFACTORY_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 

static const char * kURINC_BrowserAutodetMenuRoot = "NC:BrowserAutodetMenuRoot";
static const char * kURINC_BrowserCharsetMenuRoot = "NC:BrowserCharsetMenuRoot";
static const char * kURINC_BrowserMoreCharsetMenuRoot = "NC:BrowserMoreCharsetMenuRoot";
static const char * kURINC_BrowserMore1CharsetMenuRoot = "NC:BrowserMore1CharsetMenuRoot";
static const char * kURINC_BrowserMore2CharsetMenuRoot = "NC:BrowserMore2CharsetMenuRoot";
static const char * kURINC_BrowserMore3CharsetMenuRoot = "NC:BrowserMore3CharsetMenuRoot";
static const char * kURINC_BrowserMore4CharsetMenuRoot = "NC:BrowserMore4CharsetMenuRoot";
static const char * kURINC_BrowserMore5CharsetMenuRoot = "NC:BrowserMore5CharsetMenuRoot";
static const char * kURINC_MaileditCharsetMenuRoot = "NC:MaileditCharsetMenuRoot";
static const char * kURINC_MailviewCharsetMenuRoot = "NC:MailviewCharsetMenuRoot";
static const char * kURINC_ComposerCharsetMenuRoot = "NC:ComposerCharsetMenuRoot";
static const char * kURINC_DecodersRoot = "NC:DecodersRoot";
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Checked);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, BookmarkSeparator);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, CharsetDetector);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, NC, type);

//----------------------------------------------------------------------------
// Class nsMenuEntry [declaration]

/**
 * A little class holding all data needed for a menu item.
 *
 * @created         18/Apr/2000
 * @author  Catalin Rotaru [CATA]
 */
class nsMenuEntry
{
public: 
  // memory & ref counting & leak prevention stuff
  nsMenuEntry() { MOZ_COUNT_CTOR(nsMenuEntry); }
  ~nsMenuEntry() { MOZ_COUNT_DTOR(nsMenuEntry); }

  nsCOMPtr<nsIAtom> mCharset;
  nsAutoString      mTitle;
};

MOZ_DECL_CTOR_COUNTER(nsMenuEntry)

//----------------------------------------------------------------------------
// Class nsCharsetMenu [declaration]

/**
 * The Charset Converter menu.
 *
 * God, our GUI programming disgusts me.
 *
 * @created         23/Nov/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsCharsetMenu : public nsIRDFDataSource, public nsICurrentCharsetListener
{
  NS_DECL_ISUPPORTS

private:
  static nsIRDFResource * kNC_BrowserAutodetMenuRoot;
  static nsIRDFResource * kNC_BrowserCharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMoreCharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMore1CharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMore2CharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMore3CharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMore4CharsetMenuRoot;
  static nsIRDFResource * kNC_BrowserMore5CharsetMenuRoot;
  static nsIRDFResource * kNC_MaileditCharsetMenuRoot;
  static nsIRDFResource * kNC_MailviewCharsetMenuRoot;
  static nsIRDFResource * kNC_ComposerCharsetMenuRoot;
  static nsIRDFResource * kNC_DecodersRoot;
  static nsIRDFResource * kNC_Name;
  static nsIRDFResource * kNC_Checked;
  static nsIRDFResource * kNC_CharsetDetector;
  static nsIRDFResource * kNC_BookmarkSeparator;
  static nsIRDFResource * kRDF_type;

  static nsIRDFDataSource * mInner;

  static const char *   kBrowserStaticPrefKey;
  static const char *   kBrowserCachePrefKey;
  static const char *   kBrowserCacheSizePrefKey;
  static const char *   kMailviewStaticPrefKey;
  static const char *   kMailviewCachePrefKey;
  static const char *   kMailviewCacheSizePrefKey;
  static const char *   kComposerStaticPrefKey;
  static const char *   kComposerCachePrefKey;
  static const char *   kComposerCacheSizePrefKey;
  static const char *   kMaileditPrefKey;

  nsVoidArray   mBrowserMenu;
  PRInt32       mBrowserCacheStart;
  PRInt32       mBrowserCacheSize;
  PRInt32       mBrowserMenuRDFPosition;

  nsVoidArray   mMailviewMenu;
  PRInt32       mMailviewCacheStart;
  PRInt32       mMailviewCacheSize;
  PRInt32       mMailviewMenuRDFPosition;

  nsVoidArray   mComposerMenu;
  PRInt32       mComposerCacheStart;
  PRInt32       mComposerCacheSize;
  PRInt32       mComposerMenuRDFPosition;

  nsCOMPtr<nsIRDFService>               mRDFService;
  nsCOMPtr<nsICharsetConverterManager2> mCCManager;
  nsCOMPtr<nsIPref>                     mPrefService;

  nsresult Init();
  nsresult Done();
  nsresult SetCharsetCheckmark(nsString * aCharset, PRBool aValue);

  nsresult InitResources();
  nsresult FreeResources();

  nsresult InitBrowserMenu();
  nsresult InitMaileditMenu();
  nsresult InitMailviewMenu();
  nsresult InitComposerMenu();
  nsresult InitOthers();
  nsresult InitSecodaryTiers();

  nsresult InitStaticMenu(nsISupportsArray * aDecs, 
    nsIRDFResource * aResource, const char * aKey, nsVoidArray * aArray);
  nsresult InitCacheMenu(nsISupportsArray * aDecs, nsIRDFResource * aResource,
    const char * aKey, nsVoidArray * aArray);
  nsresult InitAutodetMenu(nsIRDFResource * aResource);
  nsresult InitMoreMenu(nsISupportsArray * aDecs, nsIRDFResource * aResource, 
    char * aFlag);
  nsresult InitMoreSubmenus(nsISupportsArray * aDecs);

  nsresult AddCharsetToItemArray(nsVoidArray * aArray, nsIAtom * aCharset, 
    nsMenuEntry ** aResult, PRInt32 aPlace);
  nsresult AddCharsetArrayToItemArray(nsVoidArray * aArray, 
    nsISupportsArray * aCharsets);
  nsresult AddMenuItemToContainer(nsIRDFContainer * aContainer, 
    nsMenuEntry * aItem, nsIRDFResource * aType, char * aIDPrefix, 
    PRInt32 aPlace);
  nsresult AddMenuItemArrayToContainer(nsIRDFContainer * aContainer, 
    nsVoidArray * aArray, nsIRDFResource * aType);
  nsresult AddCharsetToContainer(nsVoidArray * aArray, 
    nsIRDFContainer * aContainer, nsIAtom * aCharset, char * aIDPrefix, 
    PRInt32 aPlace, PRInt32 aRDFPlace);

  nsresult AddFromPrefsToMenu(nsVoidArray * aArray, 
    nsIRDFContainer * aContainer, const char * aKey, nsISupportsArray * aDecs,
    char * aIDPrefix);
  nsresult AddFromNolocPrefsToMenu(nsVoidArray * aArray, 
    nsIRDFContainer * aContainer, const char * aKey, nsISupportsArray * aDecs,
    char * aIDPrefix);
  nsresult AddFromStringToMenu(char * aCharsetList, nsVoidArray * aArray, 
    nsIRDFContainer * aContainer, nsISupportsArray * aDecs, char * aIDPrefix);

  nsresult AddSeparatorToContainer(nsIRDFContainer * aContainer);
  nsresult AddCharsetToCache(nsIAtom * aCharset, nsVoidArray * aArray, 
    nsIRDFResource * aRDFResource, PRInt32 aCacheStart, PRInt32 aCacheSize, 
    PRInt32 aRDFPlace);

  nsresult WriteCacheToPrefs(nsVoidArray * aArray, PRInt32 aCacheStart, 
    const char * aKey);
  nsresult ClearMenu(nsIRDFContainer * aContainer, nsVoidArray * aArray);
  nsresult RemoveLastMenuItem(nsIRDFContainer * aContainer, 
    nsVoidArray * aArray);

  nsresult RemoveFlaggedCharsets(nsISupportsArray * aList, nsString * aProp);
  nsresult NewRDFContainer(nsIRDFDataSource * aDataSource, 
    nsIRDFResource * aResource, nsIRDFContainer ** aResult);
  void FreeMenuItemArray(nsVoidArray * aArray);
  PRInt32 FindMenuItemInArray(nsVoidArray * aArray, nsIAtom * aCharset, 
      nsMenuEntry ** aResult);
  nsresult ReorderMenuItemArray(nsVoidArray * aArray);
  nsresult GetCollation(nsICollation ** aCollation);

public:
  nsCharsetMenu();
  virtual ~nsCharsetMenu();

  nsresult RefreshBroserMenu();
  nsresult RefreshMailviewMenu();
  nsresult RefreshComposerMenu();

  //--------------------------------------------------------------------------
  // Interface nsICurrentCharsetListener [declaration]

  NS_IMETHOD SetCurrentCharset(const PRUnichar * aCharset);
  NS_IMETHOD SetCurrentMailCharset(const PRUnichar * aCharset);
  NS_IMETHOD SetCurrentComposerCharset(const PRUnichar * aCharset);

  //--------------------------------------------------------------------------
  // Interface nsIRDFDataSource [declaration]

  NS_DECL_NSIRDFDATASOURCE
};

//----------------------------------------------------------------------------
// Global functions and data [implementation]

NS_IMETHODIMP NS_NewCharsetMenu(nsISupports * aOuter, const nsIID & aIID, 
                                void ** aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsCharsetMenu* inst = new nsCharsetMenu();
  if (!inst) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult res = inst->QueryInterface(aIID, aResult);
  if (NS_FAILED(res)) {
    *aResult = nsnull;
    delete inst;
  }
  return res;
}

static int PR_CALLBACK CompareMenuItems(const void* aArg1, const void* aArg2, void *data)
{
  PRInt32 res; 
  nsMenuEntry * aItem1 = *((nsMenuEntry **) aArg1);
  nsMenuEntry * aItem2 = *((nsMenuEntry **) aArg2);
  nsICollation * aCollation = (nsICollation *) data;

  aCollation->CompareString(kCollationCaseInSensitive, aItem1->mTitle, 
    aItem2->mTitle, &res);

  return res;
}

static int PR_CALLBACK BrowserStaticChanged(const char * aPrefName, 
                                            void * aInstanceData)
{
  nsresult res;
  res = ((nsCharsetMenu *) aInstanceData)->RefreshBroserMenu();
  NS_ASSERTION(NS_SUCCEEDED(res), "error refreshing the browser menu");

  return 0;
}

static int PR_CALLBACK MailviewStaticChanged(const char * aPrefName, 
                                             void * aInstanceData)
{
  nsresult res;
  res = ((nsCharsetMenu *) aInstanceData)->RefreshMailviewMenu();
  NS_ASSERTION(NS_SUCCEEDED(res), "error refreshing the mailview menu");

  return 0;
}

static int PR_CALLBACK ComposerStaticChanged(const char * aPrefName, 
                                             void * aInstanceData)
{
  nsresult res;
  res = ((nsCharsetMenu *) aInstanceData)->RefreshComposerMenu();
  NS_ASSERTION(NS_SUCCEEDED(res), "error refreshing the composer menu");

  return 0;
}

//----------------------------------------------------------------------------
// Class nsCharsetMenu [implementation]

NS_IMPL_ISUPPORTS2(nsCharsetMenu, nsIRDFDataSource, nsICurrentCharsetListener)

nsIRDFDataSource * nsCharsetMenu::mInner = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserAutodetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserCharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMoreCharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMore1CharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMore2CharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMore3CharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMore4CharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BrowserMore5CharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_MaileditCharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_MailviewCharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_ComposerCharsetMenuRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_DecodersRoot = NULL;
nsIRDFResource * nsCharsetMenu::kNC_Name = NULL;
nsIRDFResource * nsCharsetMenu::kNC_Checked = NULL;
nsIRDFResource * nsCharsetMenu::kNC_CharsetDetector = NULL;
nsIRDFResource * nsCharsetMenu::kNC_BookmarkSeparator = NULL;
nsIRDFResource * nsCharsetMenu::kRDF_type = NULL;

// Note here that browser and mailview have the same static area and cache 
// size but the cache itself is separate.

const char * nsCharsetMenu::kBrowserStaticPrefKey		= "intl.charsetmenu.browser.static";
const char * nsCharsetMenu::kBrowserCachePrefKey		= "intl.charsetmenu.browser.cache";
const char * nsCharsetMenu::kBrowserCacheSizePrefKey	= "intl.charsetmenu.browser.cache.size";

const char * nsCharsetMenu::kMailviewStaticPrefKey		= "intl.charsetmenu.browser.static";
const char * nsCharsetMenu::kMailviewCachePrefKey		= "intl.charsetmenu.mailview.cache";
const char * nsCharsetMenu::kMailviewCacheSizePrefKey	= "intl.charsetmenu.browser.cache.size";

const char * nsCharsetMenu::kComposerStaticPrefKey		= "intl.charsetmenu.browser.static";
const char * nsCharsetMenu::kComposerCachePrefKey		= "intl.charsetmenu.composer.cache";
const char * nsCharsetMenu::kComposerCacheSizePrefKey	= "intl.charsetmenu.browser.cache.size";

const char * nsCharsetMenu::kMaileditPrefKey			= "intl.charsetmenu.mailedit";

nsCharsetMenu::nsCharsetMenu() 
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);

  nsresult res = NS_OK;

  res = InitResources();
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing resources");

  Init();
  InitBrowserMenu();
  InitMaileditMenu();
  InitMailviewMenu();
  InitComposerMenu();
  InitSecodaryTiers();
  InitOthers();
}

nsCharsetMenu::~nsCharsetMenu() 
{
  Done();

  FreeMenuItemArray(&mBrowserMenu);
  FreeMenuItemArray(&mMailviewMenu);
  FreeMenuItemArray(&mComposerMenu);

  FreeResources();

  PR_AtomicDecrement(&g_InstanceCount);
}

// XXX collapse these 2 in one

nsresult nsCharsetMenu::RefreshBroserMenu()
{
  nsresult res = NS_OK;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_BrowserCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // clean the menu
  res = ClearMenu(container, &mBrowserMenu);
  if (NS_FAILED(res)) return res;

  // rebuild the menu
  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  res = AddFromPrefsToMenu(&mBrowserMenu, container, kBrowserStaticPrefKey, 
    decs, "charset.");
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing static charset menu from prefs");

  // mark the end of the static area, the rest is cache
  mBrowserCacheStart = mBrowserMenu.Count();

  res = InitCacheMenu(decs, kNC_BrowserCharsetMenuRoot, kBrowserCachePrefKey, 
    &mBrowserMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing browser cache charset menu");

  return res;
}

nsresult nsCharsetMenu::RefreshMailviewMenu()
{
  nsresult res = NS_OK;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_MailviewCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // clean the menu
  res = ClearMenu(container, &mMailviewMenu);
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  res = AddFromPrefsToMenu(&mMailviewMenu, container, kMailviewStaticPrefKey, 
    decs, "charset.");
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing static charset menu from prefs");

  // mark the end of the static area, the rest is cache
  mMailviewCacheStart = mMailviewMenu.Count();

  res = InitCacheMenu(decs, kNC_MailviewCharsetMenuRoot, 
    kMailviewCachePrefKey, &mMailviewMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing mailview cache charset menu");

  return res;
}

nsresult nsCharsetMenu::RefreshComposerMenu()
{
  nsresult res = NS_OK;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_ComposerCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // clean the menu
  res = ClearMenu(container, &mComposerMenu);
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  res = AddFromPrefsToMenu(&mComposerMenu, container, kComposerStaticPrefKey, 
    decs, "charset.");
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing static charset menu from prefs");

  // mark the end of the static area, the rest is cache
  mComposerCacheStart = mComposerMenu.Count();

  res = InitCacheMenu(decs, kNC_ComposerCharsetMenuRoot, 
    kComposerCachePrefKey, &mComposerMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing composer cache charset menu");

  return res;
}

nsresult nsCharsetMenu::Init() 
{
  nsresult res = NS_OK;
  nsIRDFContainerUtils * rdfUtil = NULL;

  mRDFService->GetResource(kURINC_BrowserAutodetMenuRoot, &kNC_BrowserAutodetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserCharsetMenuRoot, &kNC_BrowserCharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMoreCharsetMenuRoot, &kNC_BrowserMoreCharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMore1CharsetMenuRoot, &kNC_BrowserMore1CharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMore2CharsetMenuRoot, &kNC_BrowserMore2CharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMore3CharsetMenuRoot, &kNC_BrowserMore3CharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMore4CharsetMenuRoot, &kNC_BrowserMore4CharsetMenuRoot);
  mRDFService->GetResource(kURINC_BrowserMore5CharsetMenuRoot, &kNC_BrowserMore5CharsetMenuRoot);
  mRDFService->GetResource(kURINC_MaileditCharsetMenuRoot, &kNC_MaileditCharsetMenuRoot);
  mRDFService->GetResource(kURINC_MailviewCharsetMenuRoot, &kNC_MailviewCharsetMenuRoot);
  mRDFService->GetResource(kURINC_ComposerCharsetMenuRoot, &kNC_ComposerCharsetMenuRoot);
  mRDFService->GetResource(kURINC_DecodersRoot, &kNC_DecodersRoot);
  mRDFService->GetResource(kURINC_Name, &kNC_Name);
  mRDFService->GetResource(kURINC_Checked, &kNC_Checked);
  mRDFService->GetResource(kURINC_CharsetDetector, &kNC_CharsetDetector);
  mRDFService->GetResource(kURINC_BookmarkSeparator, &kNC_BookmarkSeparator);
  mRDFService->GetResource(kURINC_type, &kRDF_type);

  res = nsComponentManager::CreateInstance(kRDFInMemoryDataSourceCID, nsnull, 
    NS_GET_IID(nsIRDFDataSource), (void**) &mInner);
  if (NS_FAILED(res)) goto done;

  res = nsServiceManager::GetService(kRDFContainerUtilsCID, 
    NS_GET_IID(nsIRDFContainerUtils), (nsISupports **)&rdfUtil);
  if (NS_FAILED(res)) goto done;

  res = rdfUtil->MakeSeq(mInner, kNC_BrowserAutodetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserCharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMoreCharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMore1CharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMore2CharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMore3CharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMore4CharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_BrowserMore5CharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_MaileditCharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_MailviewCharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_ComposerCharsetMenuRoot, NULL);
  if (NS_FAILED(res)) goto done;
  res = rdfUtil->MakeSeq(mInner, kNC_DecodersRoot, NULL);
  if (NS_FAILED(res)) goto done;

  res = mRDFService->RegisterDataSource(this, PR_FALSE);

done:
  if (rdfUtil != NULL) nsServiceManager::ReleaseService(kRDFContainerUtilsCID, 
      rdfUtil);

  return res;
}

nsresult nsCharsetMenu::Done() 
{
  nsresult res = NS_OK;
  res = mRDFService->UnregisterDataSource(this);

  NS_IF_RELEASE(kNC_BrowserAutodetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserCharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMoreCharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMore1CharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMore2CharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMore3CharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMore4CharsetMenuRoot);
  NS_IF_RELEASE(kNC_BrowserMore5CharsetMenuRoot);
  NS_IF_RELEASE(kNC_MaileditCharsetMenuRoot);
  NS_IF_RELEASE(kNC_MailviewCharsetMenuRoot);
  NS_IF_RELEASE(kNC_ComposerCharsetMenuRoot);
  NS_IF_RELEASE(kNC_DecodersRoot);
  NS_IF_RELEASE(kNC_Name);
  NS_IF_RELEASE(kNC_Checked);
  NS_IF_RELEASE(kNC_CharsetDetector);
  NS_IF_RELEASE(kNC_BookmarkSeparator);
  NS_IF_RELEASE(kRDF_type);
  NS_IF_RELEASE(mInner);

  return res;
}

nsresult nsCharsetMenu::SetCharsetCheckmark(nsString * aCharset, 
                                            PRBool aValue)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIRDFContainer> container;
  nsCOMPtr<nsIRDFResource> node;

  res = NewRDFContainer(mInner, kNC_BrowserCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // find RDF node for given charset
  char csID[256];
  aCharset->ToCString(csID, sizeof(csID));
  res = mRDFService->GetResource(csID, getter_AddRefs(node));
  if (NS_FAILED(res)) return res;

  // set checkmark value
  nsCOMPtr<nsIRDFLiteral> checkedLiteral;
  nsAutoString checked; checked.AssignWithConversion((aValue == PR_TRUE) ? "true" : "false");
  res = mRDFService->GetLiteral(checked.GetUnicode(), getter_AddRefs(checkedLiteral));
  if (NS_FAILED(res)) return res;
  res = Assert(node, kNC_Checked, checkedLiteral, PR_TRUE);
  if (NS_FAILED(res)) return res;

  return res;
}

/**
 * Init the resources needed by the component.
 */
nsresult nsCharsetMenu::InitResources()
{
  nsresult res = NS_OK;

  mRDFService = do_GetService(kRDFServiceCID, &res);
  if (NS_FAILED(res)) return res;

  mCCManager = do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) return res;

  mPrefService = do_GetService(NS_PREF_CONTRACTID, &res);
  if (NS_FAILED(res)) return res;

  return res;
}

/**
 * Free the resources no longer needed by the component.
 */
nsresult nsCharsetMenu::FreeResources()
{
  nsresult res = NS_OK;

  mRDFService   = NULL;
  mCCManager    = NULL;
  mPrefService  = NULL;

  return res;
}

// XXX collapse these initAAAMenu()'s in one

nsresult nsCharsetMenu::InitBrowserMenu() 
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_BrowserCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // even if we fail, the show must go on
  res = InitStaticMenu(decs, kNC_BrowserCharsetMenuRoot, 
    kBrowserStaticPrefKey, &mBrowserMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing browser static charset menu");

  // mark the end of the static area, the rest is cache
  mBrowserCacheStart = mBrowserMenu.Count();
  mPrefService->GetIntPref(kBrowserCacheSizePrefKey, &mBrowserCacheSize);

  // compute the position of the menu in the RDF container
  res = container->GetCount(&mBrowserMenuRDFPosition);
  if (NS_FAILED(res)) return res;
  // this "1" here is a correction necessary because the RDF container 
  // elements are numbered from 1 (why god, WHY?!?!?!)
  mBrowserMenuRDFPosition -= mBrowserCacheStart - 1;

  res = InitCacheMenu(decs, kNC_BrowserCharsetMenuRoot, kBrowserCachePrefKey, 
    &mBrowserMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing browser cache charset menu");

  // register prefs callback
  mPrefService->RegisterCallback(kBrowserStaticPrefKey, BrowserStaticChanged, 
    this);

  return res;
}

nsresult nsCharsetMenu::InitMaileditMenu() 
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> encs;
  res = mCCManager->GetEncoderList(getter_AddRefs(encs));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRDFContainer> container;

  res = NewRDFContainer(mInner, kNC_MaileditCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  res = AddFromPrefsToMenu(NULL, container, kMaileditPrefKey, encs, NULL);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing mailedit charset menu from prefs");

  return res;
}

nsresult nsCharsetMenu::InitMailviewMenu() 
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_MailviewCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // even if we fail, the show must go on
  res = InitStaticMenu(decs, kNC_MailviewCharsetMenuRoot, 
    kMailviewStaticPrefKey, &mMailviewMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing mailview static charset menu");

  // mark the end of the static area, the rest is cache
  mMailviewCacheStart = mMailviewMenu.Count();
  mPrefService->GetIntPref(kMailviewCacheSizePrefKey, &mMailviewCacheSize);

  // compute the position of the menu in the RDF container
  res = container->GetCount(&mMailviewMenuRDFPosition);
  if (NS_FAILED(res)) return res;
  // this "1" here is a correction necessary because the RDF container 
  // elements are numbered from 1 (why god, WHY?!?!?!)
  mMailviewMenuRDFPosition -= mMailviewCacheStart - 1;

  res = InitCacheMenu(decs, kNC_MailviewCharsetMenuRoot, 
    kMailviewCachePrefKey, &mMailviewMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing mailview cache charset menu");

  // register prefs callback
  mPrefService->RegisterCallback(kMailviewStaticPrefKey, MailviewStaticChanged,
    this);

  return res;
}

nsresult nsCharsetMenu::InitComposerMenu() 
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, kNC_ComposerCharsetMenuRoot, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // even if we fail, the show must go on
  res = InitStaticMenu(decs, kNC_ComposerCharsetMenuRoot, 
    kComposerStaticPrefKey, &mComposerMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing composer static charset menu");

  // mark the end of the static area, the rest is cache
  mComposerCacheStart = mComposerMenu.Count();
  mPrefService->GetIntPref(kComposerCacheSizePrefKey, &mComposerCacheSize);

  // compute the position of the menu in the RDF container
  res = container->GetCount(&mComposerMenuRDFPosition);
  if (NS_FAILED(res)) return res;
  // this "1" here is a correction necessary because the RDF container 
  // elements are numbered from 1 (why god, WHY?!?!?!)
  mComposerMenuRDFPosition -= mComposerCacheStart - 1;

  res = InitCacheMenu(decs, kNC_ComposerCharsetMenuRoot, 
    kComposerCachePrefKey, &mComposerMenu);
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing composer cache charset menu");

  // register prefs callback
  mPrefService->RegisterCallback(kComposerStaticPrefKey, ComposerStaticChanged,
    this);

  return res;
}

nsresult nsCharsetMenu::InitOthers() 
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  res = InitMoreMenu(decs, kNC_DecodersRoot, ".notForBrowser");
  if (NS_FAILED(res)) return res;

  return res;
}

/**
 * Inits the secondary tiers of the charset menu. Because currently all the CS 
 * menus are sharing the secondary tiers, one should call this method only 
 * once for all of them.
 */
nsresult nsCharsetMenu::InitSecodaryTiers()
{
  nsresult res = NS_OK;

  nsCOMPtr<nsISupportsArray> decs;
  res = mCCManager->GetDecoderList(getter_AddRefs(decs));
  if (NS_FAILED(res)) return res;

  res = InitMoreSubmenus(decs);
  NS_ASSERTION(NS_SUCCEEDED(res), "err init browser charset more submenus");

  res = InitMoreMenu(decs, kNC_BrowserMoreCharsetMenuRoot, ".notForBrowser");
  NS_ASSERTION(NS_SUCCEEDED(res), "err init browser charset more menu");

  res = InitAutodetMenu(kNC_BrowserAutodetMenuRoot);
  NS_ASSERTION(NS_SUCCEEDED(res), "err init chardet menu");

  return res;
}

nsresult nsCharsetMenu::InitStaticMenu(
                        nsISupportsArray * aDecs,
                        nsIRDFResource * aResource, 
                        const char * aKey, 
                        nsVoidArray * aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIRDFContainer> container;

  res = NewRDFContainer(mInner, aResource, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // XXX work around bug that causes the submenus to be first instead of last
  res = AddSeparatorToContainer(container);
  NS_ASSERTION(NS_SUCCEEDED(res), "error adding separator to container");

  res = AddFromPrefsToMenu(aArray, container, aKey, aDecs, "charset.");
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing static charset menu from prefs");

  return res;
}

nsresult nsCharsetMenu::InitCacheMenu(
                        nsISupportsArray * aDecs,
                        nsIRDFResource * aResource, 
                        const char * aKey, 
                        nsVoidArray * aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIRDFContainer> container;

  res = NewRDFContainer(mInner, aResource, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  res = AddFromNolocPrefsToMenu(aArray, container, aKey, aDecs, "charset.");
  NS_ASSERTION(NS_SUCCEEDED(res), "error initializing cache charset menu from prefs");

  return res;
}

nsresult nsCharsetMenu::InitAutodetMenu(nsIRDFResource * aResource)
{
  nsresult res = NS_OK;
  nsVoidArray chardetArray;
  nsCOMPtr<nsIRDFContainer> container;

  res = NewRDFContainer(mInner, aResource, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsISupportsArray> array;
  res = mCCManager->GetCharsetDetectorList(getter_AddRefs(array));
  if (NS_FAILED(res)) goto done;

  res = AddCharsetArrayToItemArray(&chardetArray, array);
  if (NS_FAILED(res)) goto done;

  // reorder the array
  res = ReorderMenuItemArray(&chardetArray);
  if (NS_FAILED(res)) goto done;

  res = AddMenuItemArrayToContainer(container, &chardetArray, 
    kNC_CharsetDetector);
  if (NS_FAILED(res)) goto done;

done:
  // free the elements in the VoidArray
  FreeMenuItemArray(&chardetArray);

  return res;
}

nsresult nsCharsetMenu::InitMoreMenu(
                        nsISupportsArray * aDecs, 
                        nsIRDFResource * aResource, 
                        char * aFlag)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIRDFContainer> container;
  nsVoidArray moreMenu;
  nsAutoString prop; prop.AssignWithConversion(aFlag);

  res = NewRDFContainer(mInner, aResource, getter_AddRefs(container));
  if (NS_FAILED(res)) goto done;

  // remove charsets "not for browser"
  res = RemoveFlaggedCharsets(aDecs, &prop);
  if (NS_FAILED(res)) goto done;

  res = AddCharsetArrayToItemArray(&moreMenu, aDecs);
  if (NS_FAILED(res)) goto done;

  // reorder the array
  res = ReorderMenuItemArray(&moreMenu);
  if (NS_FAILED(res)) goto done;

  res = AddMenuItemArrayToContainer(container, &moreMenu, NULL);
  if (NS_FAILED(res)) goto done;

done:
  // free the elements in the VoidArray
  FreeMenuItemArray(&moreMenu);

  return res;
}

// XXX please make this method more general; the cut&pasted code is laughable
nsresult nsCharsetMenu::InitMoreSubmenus(nsISupportsArray * aDecs)
{
  nsresult res = NS_OK;

  nsCOMPtr<nsIRDFContainer> container1;
  nsCOMPtr<nsIRDFContainer> container2;
  nsCOMPtr<nsIRDFContainer> container3;
  nsCOMPtr<nsIRDFContainer> container4;
  nsCOMPtr<nsIRDFContainer> container5;
  char * key1 = "intl.charsetmenu.browser.more1";
  char * key2 = "intl.charsetmenu.browser.more2";
  char * key3 = "intl.charsetmenu.browser.more3";
  char * key4 = "intl.charsetmenu.browser.more4";
  char * key5 = "intl.charsetmenu.browser.more5";

  res = NewRDFContainer(mInner, kNC_BrowserMore1CharsetMenuRoot, 
    getter_AddRefs(container1));
  if (NS_FAILED(res)) return res;
  AddFromPrefsToMenu(NULL, container1, key1, aDecs, NULL);

  res = NewRDFContainer(mInner, kNC_BrowserMore2CharsetMenuRoot, 
    getter_AddRefs(container2));
  if (NS_FAILED(res)) return res;
  AddFromPrefsToMenu(NULL, container2, key2, aDecs, NULL);

  res = NewRDFContainer(mInner, kNC_BrowserMore3CharsetMenuRoot, 
    getter_AddRefs(container3));
  if (NS_FAILED(res)) return res;
  AddFromPrefsToMenu(NULL, container3, key3, aDecs, NULL);

  res = NewRDFContainer(mInner, kNC_BrowserMore4CharsetMenuRoot, 
    getter_AddRefs(container4));
  if (NS_FAILED(res)) return res;
  AddFromPrefsToMenu(NULL, container4, key4, aDecs, NULL);

  res = NewRDFContainer(mInner, kNC_BrowserMore5CharsetMenuRoot, 
    getter_AddRefs(container5));
  if (NS_FAILED(res)) return res;
  AddFromPrefsToMenu(NULL, container5, key5, aDecs, NULL);

  return res;
}

nsresult nsCharsetMenu::AddCharsetToItemArray(
                        nsVoidArray * aArray, 
                        nsIAtom * aCharset, 
                        nsMenuEntry ** aResult,
                        PRInt32 aPlace) 
{
  nsresult res = NS_OK;
  nsMenuEntry * item = NULL; 

  if (aResult != NULL) *aResult = NULL;
  
  item = new nsMenuEntry();
  if (item == NULL) {
    res = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }

  item->mCharset = aCharset;

  res = mCCManager->GetCharsetTitle2(aCharset, &item->mTitle);
  if (NS_FAILED(res)) {
    res = aCharset->ToString(item->mTitle);
    if (NS_FAILED(res)) goto done;
  }

  if (aArray != NULL) {
    if (aPlace < 0) {
      res = aArray->AppendElement(item);
      if (NS_FAILED(res)) goto done;
    } else {
      res = aArray->InsertElementAt(item, aPlace);
      if (NS_FAILED(res)) goto done;
    }
  }

  if (aResult != NULL) *aResult = item;

  // if we have made another reference to "item", do not delete it 
  if ((aArray != NULL) || (aResult != NULL)) item = NULL; 

done:
  if (item != NULL) delete item;

  return res;
}

nsresult nsCharsetMenu::AddCharsetArrayToItemArray(
                        nsVoidArray * aArray, 
                        nsISupportsArray * aCharsets) 
{
  PRUint32 count;
  nsresult res = aCharsets->Count(&count);
  if (NS_FAILED(res)) return res;

  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> cs;
    res = aCharsets->GetElementAt(i, getter_AddRefs(cs));
    if (NS_FAILED(res)) return res;

    res = AddCharsetToItemArray(aArray, cs, NULL, -1);
    if (NS_FAILED(res)) return res;
  }

  return NS_OK;
}

// aPlace < -1 for Remove
// aPlace < 0 for Append
nsresult nsCharsetMenu::AddMenuItemToContainer(
                        nsIRDFContainer * aContainer,
                        nsMenuEntry * aItem,
                        nsIRDFResource * aType,
                        char * aIDPrefix,
                        PRInt32 aPlace) 
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIRDFResource> node;

  nsAutoString cs;
  res = aItem->mCharset->ToString(cs);
  if (NS_FAILED(res)) return res;

  nsAutoString id;
  if (aIDPrefix != NULL) id.AssignWithConversion(aIDPrefix);
  id.Append(cs);

  // Make up a unique ID and create the RDF NODE
  char csID[256];
  id.ToCString(csID, sizeof(csID));
  res = mRDFService->GetResource(csID, getter_AddRefs(node));
  if (NS_FAILED(res)) return res;

  const PRUnichar * title = aItem->mTitle.GetUnicode();

  // set node's title
  nsCOMPtr<nsIRDFLiteral> titleLiteral;
  res = mRDFService->GetLiteral(title, getter_AddRefs(titleLiteral));
  if (NS_FAILED(res)) return res;

  if (aPlace < -1) {
    res = Unassert(node, kNC_Name, titleLiteral);
    if (NS_FAILED(res)) return res;
  } else {
    res = Assert(node, kNC_Name, titleLiteral, PR_TRUE);
    if (NS_FAILED(res)) return res;
  }

  if (aType != NULL) {
    if (aPlace < -1) {
      res = Unassert(node, kRDF_type, aType);
      if (NS_FAILED(res)) return res;
    } else {
      res = Assert(node, kRDF_type, aType, PR_TRUE);
      if (NS_FAILED(res)) return res;
    }
  }

  // Add the element to the container
  if (aPlace < -1) {
    res = aContainer->RemoveElement(node, PR_TRUE);
    if (NS_FAILED(res)) return res;
  } else if (aPlace < 0) {
    res = aContainer->AppendElement(node);
    if (NS_FAILED(res)) return res;
  } else {
    res = aContainer->InsertElementAt(node, aPlace, PR_TRUE);
    if (NS_FAILED(res)) return res;
  } 

  return res;
}

nsresult nsCharsetMenu::AddMenuItemArrayToContainer(
                        nsIRDFContainer * aContainer,
                        nsVoidArray * aArray,
                        nsIRDFResource * aType) 
{
  PRUint32 count = aArray->Count();
  nsresult res = NS_OK;

  for (PRUint32 i = 0; i < count; i++) {
    nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(i);
    if (item == NULL) return NS_ERROR_UNEXPECTED;

    res = AddMenuItemToContainer(aContainer, item, aType, NULL, -1);
    if (NS_FAILED(res)) return res;
  }

  return NS_OK;
}

nsresult nsCharsetMenu::AddCharsetToContainer(
                        nsVoidArray * aArray, 
                        nsIRDFContainer * aContainer, 
                        nsIAtom * aCharset, 
                        char * aIDPrefix,
                        PRInt32 aPlace,
						PRInt32 aRDFPlace)
{
  nsresult res = NS_OK;
  nsMenuEntry * item = NULL; 
  
  res = AddCharsetToItemArray(aArray, aCharset, &item, aPlace);
  if (NS_FAILED(res)) goto done;

  res = AddMenuItemToContainer(aContainer, item, NULL, aIDPrefix, 
    aPlace + aRDFPlace);
  if (NS_FAILED(res)) goto done;

  // if we have made another reference to "item", do not delete it 
  if (aArray != NULL) item = NULL; 

done:
  if (item != NULL) delete item;

  return res;
}

nsresult nsCharsetMenu::AddFromPrefsToMenu(
                        nsVoidArray * aArray, 
                        nsIRDFContainer * aContainer, 
                        const char * aKey, 
                        nsISupportsArray * aDecs, 
                        char * aIDPrefix)
{
  nsresult res = NS_OK;

  PRUnichar * value = NULL;
  res = mPrefService->GetLocalizedUnicharPref(aKey, &value);
  if (NS_FAILED(res)) return res;

  if (value != NULL) {
    res = AddFromStringToMenu(NS_ConvertUCS2toUTF8(value), aArray, aContainer, 
      aDecs, aIDPrefix);
    nsMemory::Free(value);
  }

  return res;
}

nsresult nsCharsetMenu::AddFromNolocPrefsToMenu(
                        nsVoidArray * aArray, 
                        nsIRDFContainer * aContainer, 
                        const char * aKey, 
                        nsISupportsArray * aDecs, 
                        char * aIDPrefix)
{
  nsresult res = NS_OK;

  char * value = NULL;
  res = mPrefService->CopyCharPref(aKey, &value);
  if (NS_FAILED(res)) return res;

  if (value != NULL) {
    res = AddFromStringToMenu(value, aArray, aContainer, aDecs, aIDPrefix);
    nsMemory::Free(value);
  }

  return res;
}

nsresult nsCharsetMenu::AddFromStringToMenu(
                        char * aCharsetList, 
                        nsVoidArray * aArray, 
                        nsIRDFContainer * aContainer, 
                        nsISupportsArray * aDecs, 
                        char * aIDPrefix)
{
  nsresult res = NS_OK;
  char * p = aCharsetList;
  char * q = p;
  while (*p != 0) {
	  for (; (*q != ',') && (*q != ' ') && (*q != 0); q++) {;}
    char temp = *q;
    *q = 0;

    nsCOMPtr<nsIAtom> atom;
    res = mCCManager->GetCharsetAtom2(p, getter_AddRefs(atom));
    NS_ASSERTION(NS_SUCCEEDED(res), "cannot get charset atom");
    if (NS_FAILED(res)) break;

    // if this charset is not on the accepted list of charsets, ignore it
    PRInt32 index;
    res = aDecs->GetIndexOf(atom, &index);
    if (NS_SUCCEEDED(res) && (index >= 0)) {

      // else, add it to the menu
      res = AddCharsetToContainer(aArray, aContainer, atom, aIDPrefix, -1, 0);
      NS_ASSERTION(NS_SUCCEEDED(res), "cannot add charset to menu");
      if (NS_FAILED(res)) break;

      res = aDecs->RemoveElement(atom);
      NS_ASSERTION(NS_SUCCEEDED(res), "cannot remove atom from array");
    }

    *q = temp;
    for (; (*q == ',') || (*q == ' '); q++) {;}
    p=q;
  }

  return NS_OK;
}

nsresult nsCharsetMenu::AddSeparatorToContainer(nsIRDFContainer * aContainer)
{
  nsAutoString str;
  str.AssignWithConversion("----");

  // hack to generate unique id's for separators
  static PRInt32 u = 0;
  u++;
  str.AppendInt(u);

  nsMenuEntry item;
  item.mCharset = getter_AddRefs(NS_NewAtom(str));
  item.mTitle.Assign(str);

  return AddMenuItemToContainer(aContainer, &item, kNC_BookmarkSeparator, 
    NULL, -1);
}

nsresult nsCharsetMenu::AddCharsetToCache(
                        nsIAtom * aCharset,
                        nsVoidArray * aArray,
                        nsIRDFResource * aRDFResource, 
                        PRInt32 aCacheStart, 
						PRInt32 aCacheSize,
						PRInt32 aRDFPlace)
{
  PRInt32 i;
  nsresult res = NS_OK;

  i = FindMenuItemInArray(aArray, aCharset, NULL);
  if (i >= 0) return res;

  nsCOMPtr<nsIRDFContainer> container;
  res = NewRDFContainer(mInner, aRDFResource, getter_AddRefs(container));
  if (NS_FAILED(res)) return res;

  // iff too many items, remove last one
  if (aArray->Count() - aCacheStart >= aCacheSize){
    res = RemoveLastMenuItem(container, aArray);
    if (NS_FAILED(res)) return res;
  }

  res = AddCharsetToContainer(aArray, container, aCharset, "charset.", 
    aCacheStart, aRDFPlace);

  return res;
}

nsresult nsCharsetMenu::WriteCacheToPrefs(nsVoidArray * aArray, 
                                          PRInt32 aCacheStart, 
                                          const char * aKey)
{
  nsresult res = NS_OK;

  // create together the cache string
  nsAutoString cache;
  nsAutoString sep; sep.AppendWithConversion(", ");
  PRInt32 count = aArray->Count();

  for (PRInt32 i = aCacheStart; i < count; i++) {
    nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(i);
    if (item != NULL) {    
      nsAutoString cs;
      res = item->mCharset->ToString(cs);
      if (NS_SUCCEEDED(res)) {
        cache.Append(cs);
        if (i < count - 1) {
          cache.Append(sep);
        }
      }
    }
  }

  // write the pref
  res = mPrefService->SetCharPref(aKey, NS_ConvertUCS2toUTF8(cache.GetUnicode()));

  return res;
}

nsresult nsCharsetMenu::ClearMenu(nsIRDFContainer * aContainer,  
                                  nsVoidArray * aArray)
{
  nsresult res = NS_OK;

  // clean the RDF data source
  PRInt32 count = aArray->Count();
  for (PRInt32 i = 0; i < count; i++) {
    nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(i);
    if (item != NULL) {    
      res = AddMenuItemToContainer(aContainer, item, NULL, "charset.", -2);
      if (NS_FAILED(res)) return res;
    }
  }

  // clean the internal data structures
  FreeMenuItemArray(aArray);

  return res;
}

nsresult nsCharsetMenu::RemoveLastMenuItem(nsIRDFContainer * aContainer,
                                           nsVoidArray * aArray)
{
  nsresult res = NS_OK;

  PRInt32 last = aArray->Count() - 1;
  nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(last);
  if (item != NULL) {    
    res = AddMenuItemToContainer(aContainer, item, NULL, "charset.", -2);
    if (NS_FAILED(res)) return res;

    res = aArray->RemoveElementAt(last);
    if (NS_FAILED(res)) return res;
  }

  return res;
}

nsresult nsCharsetMenu::RemoveFlaggedCharsets(
                        nsISupportsArray * aList, 
                        nsString * aProp)
{
  nsresult res = NS_OK;
  PRUint32 count;

  res = aList->Count(&count);
  if (NS_FAILED(res)) return res;

  for (PRUint32 i = 0; i < count; i++) {
    nsCOMPtr<nsIAtom> atom;
    res = aList->GetElementAt(i, getter_AddRefs(atom));
    if (NS_FAILED(res)) continue;

    nsAutoString str;
    res = mCCManager->GetCharsetData2(atom, aProp->GetUnicode(), &str);
    if (NS_FAILED(res)) continue;

    res = aList->RemoveElement(atom);
    if (NS_FAILED(res)) continue;

    i--; 
    count--;
  }

  return NS_OK;
}

nsresult nsCharsetMenu::NewRDFContainer(nsIRDFDataSource * aDataSource, 
                                        nsIRDFResource * aResource, 
                                        nsIRDFContainer ** aResult)
{
  nsresult res;

  res = nsComponentManager::CreateInstance(kRDFContainerCID, NULL, 
    NS_GET_IID(nsIRDFContainer), (void**)aResult);
  if (NS_FAILED(res)) return res;

  res = (*aResult)->Init(aDataSource, aResource);
  if (NS_FAILED(res)) NS_RELEASE(*aResult);

  return res;
}

void nsCharsetMenu::FreeMenuItemArray(nsVoidArray * aArray)
{
  PRUint32 count = aArray->Count();
  for (PRUint32 i = 0; i < count; i++) {
    nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(i);
    if (item != NULL) {
      delete item;
    }
  }
  aArray->Clear();
}

PRInt32 nsCharsetMenu::FindMenuItemInArray(nsVoidArray * aArray, 
                                           nsIAtom * aCharset, 
                                           nsMenuEntry ** aResult)
{
  PRUint32 count = aArray->Count();

  for (PRUint32 i=0; i < count; i++) {
    nsMenuEntry * item = (nsMenuEntry *) aArray->ElementAt(i);
    if ((item->mCharset).get() == aCharset) {
      if (aResult != NULL) *aResult = item;
      return i;
    }
  }

  if (aResult != NULL) *aResult = NULL;
  return -1;
}

nsresult nsCharsetMenu::ReorderMenuItemArray(nsVoidArray * aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsICollation> collation;
  PRUint32 count = aArray->Count();
  PRUint32 i;

  // we need to use a temporary array
  nsMenuEntry ** array = new nsMenuEntry * [count];
  if (array == NULL) {
    res = NS_ERROR_OUT_OF_MEMORY;
    goto done;
  }

  for (i = 0; i < count; i++) {
    array[i] = (nsMenuEntry *)aArray->ElementAt(i);
  }

  // reorder the array
  res = GetCollation(getter_AddRefs(collation));
  if (NS_SUCCEEDED(res)) 
    NS_QuickSort(array, count, sizeof(*array), CompareMenuItems, collation);

  // move the elements from the temporary array into the the real one
  aArray->Clear();
  for (i = 0; i < count; i++) {
    aArray->AppendElement(array[i]);
  }

done:
  delete [] array;
  return res;
}

nsresult nsCharsetMenu::GetCollation(nsICollation ** aCollation)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsILocale> locale = nsnull;
  nsICollationFactory * collationFactory = nsnull;
  
  NS_WITH_SERVICE(nsILocaleService, localeServ, kLocaleServiceCID, &res);
  if (NS_FAILED(res)) return res;

  res = localeServ->GetApplicationLocale(getter_AddRefs(locale));
  if (NS_FAILED(res)) return res;

  res = nsComponentManager::CreateInstance(kCollationFactoryCID, NULL, 
      NS_GET_IID(nsICollationFactory), (void**) &collationFactory);
  if (NS_FAILED(res)) return res;

  res = collationFactory->CreateCollation(locale, aCollation);
  NS_RELEASE(collationFactory);
  return res;
}

//----------------------------------------------------------------------------
// Interface nsICurrentCharsetListener [implementation]

NS_IMETHODIMP nsCharsetMenu::SetCurrentCharset(const PRUnichar * aCharset)
{
  nsresult res;

  nsCOMPtr<nsIAtom> atom;
  res = mCCManager->GetCharsetAtom(aCharset, getter_AddRefs(atom));
  if (NS_FAILED(res)) return res;

  res = AddCharsetToCache(atom, &mBrowserMenu, kNC_BrowserCharsetMenuRoot, 
    mBrowserCacheStart, mBrowserCacheSize, mBrowserMenuRDFPosition);
  if (NS_FAILED(res)) return res;

  res = WriteCacheToPrefs(&mBrowserMenu, mBrowserCacheStart, 
    kBrowserCachePrefKey);
  return res;
}

NS_IMETHODIMP nsCharsetMenu::SetCurrentMailCharset(const PRUnichar * aCharset)
{
  nsresult res;

  nsCOMPtr<nsIAtom> atom;
  res = mCCManager->GetCharsetAtom(aCharset, getter_AddRefs(atom));
  if (NS_FAILED(res)) return res;

  res = AddCharsetToCache(atom, &mMailviewMenu, kNC_MailviewCharsetMenuRoot, 
    mMailviewCacheStart, mMailviewCacheSize, mMailviewMenuRDFPosition);
  if (NS_FAILED(res)) return res;

  res = WriteCacheToPrefs(&mMailviewMenu, mMailviewCacheStart, 
    kMailviewCachePrefKey);
  return res;
}

NS_IMETHODIMP nsCharsetMenu::SetCurrentComposerCharset(const PRUnichar * aCharset)
{
  nsresult res;

  nsCOMPtr<nsIAtom> atom;
  res = mCCManager->GetCharsetAtom(aCharset, getter_AddRefs(atom));
  if (NS_FAILED(res)) return res;

  res = AddCharsetToCache(atom, &mComposerMenu, kNC_ComposerCharsetMenuRoot, 
    mComposerCacheStart, mComposerCacheSize, mComposerMenuRDFPosition);
  if (NS_FAILED(res)) return res;

  res = WriteCacheToPrefs(&mComposerMenu, mComposerCacheStart, 
    kComposerCachePrefKey);
  return res;
}

//----------------------------------------------------------------------------
// Interface nsIRDFDataSource [implementation]

NS_IMETHODIMP nsCharsetMenu::GetURI(char ** uri)
{
  if (!uri) return NS_ERROR_NULL_POINTER;

  *uri = nsXPIDLCString::Copy("rdf:charset-menu");
  if (!(*uri)) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP nsCharsetMenu::GetSource(nsIRDFResource* property,
                                       nsIRDFNode* target,
                                       PRBool tv,
                                       nsIRDFResource** source)
{
  return mInner->GetSource(property, target, tv, source);
}

NS_IMETHODIMP nsCharsetMenu::GetSources(nsIRDFResource* property,
                                        nsIRDFNode* target,
                                        PRBool tv,
                                        nsISimpleEnumerator** sources)
{
  return mInner->GetSources(property, target, tv, sources);
}

NS_IMETHODIMP nsCharsetMenu::GetTarget(nsIRDFResource* source,
                                       nsIRDFResource* property,
                                       PRBool tv,
                                       nsIRDFNode** target)
{
  return mInner->GetTarget(source, property, tv, target);
}

NS_IMETHODIMP nsCharsetMenu::GetTargets(nsIRDFResource* source,
                                        nsIRDFResource* property,
                                        PRBool tv,
                                        nsISimpleEnumerator** targets)
{
  return mInner->GetTargets(source, property, tv, targets);
}

NS_IMETHODIMP nsCharsetMenu::Assert(nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget,
                                    PRBool aTruthValue)
{
  // TODO: filter out asserts we don't care about
  return mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
}

NS_IMETHODIMP nsCharsetMenu::Unassert(nsIRDFResource* aSource,
                                      nsIRDFResource* aProperty,
                                      nsIRDFNode* aTarget)
{
  // TODO: filter out unasserts we don't care about
  return mInner->Unassert(aSource, aProperty, aTarget);
}


NS_IMETHODIMP nsCharsetMenu::Change(nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aOldTarget,
                                    nsIRDFNode* aNewTarget)
{
  // TODO: filter out changes we don't care about
  return mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
}

NS_IMETHODIMP nsCharsetMenu::Move(nsIRDFResource* aOldSource,
                                  nsIRDFResource* aNewSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget)
{
  // TODO: filter out changes we don't care about
  return mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
}


NS_IMETHODIMP nsCharsetMenu::HasAssertion(nsIRDFResource* source, 
                                          nsIRDFResource* property, 
                                          nsIRDFNode* target, PRBool tv, 
                                          PRBool* hasAssertion)
{
  return mInner->HasAssertion(source, property, target, tv, hasAssertion);
}

NS_IMETHODIMP nsCharsetMenu::AddObserver(nsIRDFObserver* n)
{
  return mInner->AddObserver(n);
}

NS_IMETHODIMP nsCharsetMenu::RemoveObserver(nsIRDFObserver* n)
{
  return mInner->RemoveObserver(n);
}

NS_IMETHODIMP 
nsCharsetMenu::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
  return mInner->HasArcIn(aNode, aArc, result);
}

NS_IMETHODIMP 
nsCharsetMenu::HasArcOut(nsIRDFResource *source, nsIRDFResource *aArc, PRBool *result)
{
  return mInner->HasArcOut(source, aArc, result);
}

NS_IMETHODIMP nsCharsetMenu::ArcLabelsIn(nsIRDFNode* node, 
                                         nsISimpleEnumerator** labels)
{
  return mInner->ArcLabelsIn(node, labels);
}

NS_IMETHODIMP nsCharsetMenu::ArcLabelsOut(nsIRDFResource* source, 
                                          nsISimpleEnumerator** labels)
{
  return mInner->ArcLabelsOut(source, labels);
}

NS_IMETHODIMP nsCharsetMenu::GetAllResources(nsISimpleEnumerator** aCursor)
{
  return mInner->GetAllResources(aCursor);
}

NS_IMETHODIMP nsCharsetMenu::GetAllCommands(
                             nsIRDFResource* source,
                             nsIEnumerator/*<nsIRDFResource>*/** commands)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCharsetMenu::GetAllCmds(
                             nsIRDFResource* source,
                             nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCharsetMenu::IsCommandEnabled(
                             nsISupportsArray/*<nsIRDFResource>*/* aSources,
                             nsIRDFResource*   aCommand,
                             nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                             PRBool* aResult)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsCharsetMenu::DoCommand(nsISupportsArray* aSources,
                                       nsIRDFResource*   aCommand,
                                       nsISupportsArray* aArguments)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

