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
#include "nsCOMPtr.h"
#include "nsIRegistry.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeDecodeHelper.h"
#include "nsIUnicodeEncodeHelper.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetAlias.h"
#include "nsITextToSubURI.h"
#include "nsIServiceManager.h"
#include "nsCharsetMenu.h"
#include "rdf.h"
#include "nsUConvDll.h"
#include "nsFileSpec.h"
#include "nsIFile.h"

#include "nsUCvMinSupport.h"
#include "nsISO88591ToUnicode.h"
#include "nsCP1252ToUnicode.h"
#include "nsMacRomanToUnicode.h"
#include "nsUTF8ToUnicode.h"
#include "nsUnicodeToISO88591.h"
#include "nsUnicodeToCP1252.h"
#include "nsUnicodeToMacRoman.h"
#include "nsUnicodeToUTF8.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kCharsetMenuCID, NS_CHARSETMENU_CID);
static NS_DEFINE_CID(kTextToSubURICID, NS_TEXTTOSUBURI_CID);
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);

// converters
NS_DEFINE_CID(kISO88591ToUnicodeCID, NS_ISO88591TOUNICODE_CID);
NS_DEFINE_CID(kCP1252ToUnicodeCID, NS_CP1252TOUNICODE_CID);
NS_DEFINE_CID(kMacRomanToUnicodeCID, NS_MACROMANTOUNICODE_CID);
NS_DEFINE_CID(kUTF8ToUnicodeCID, NS_UTF8TOUNICODE_CID);
NS_DEFINE_CID(kUnicodeToISO88591CID, NS_UNICODETOISO88591_CID);
NS_DEFINE_CID(kUnicodeToCP1252CID, NS_UNICODETOCP1252_CID);
NS_DEFINE_CID(kUnicodeToMacRomanCID, NS_UNICODETOMACROMAN_CID);
NS_DEFINE_CID(kUnicodeToUTF8CID, NS_UNICODETOUTF8_CID);

class nsUConvModule : public nsIModule {
public:
  nsUConvModule();
  virtual ~nsUConvModule();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIMODULE

  nsresult Initialize();

protected:
  void Shutdown();

  PRBool mInitialized;
};

nsUConvModule::nsUConvModule()
  : mInitialized(PR_FALSE)
{
  NS_INIT_ISUPPORTS();
}

nsUConvModule::~nsUConvModule()
{
  Shutdown();
}

NS_IMPL_ISUPPORTS1(nsUConvModule, nsIModule)

nsresult
nsUConvModule::Initialize()
{
  return NS_OK;
}

void
nsUConvModule::Shutdown()
{
}

//----------------------------------------
// This is a mid-range hack, in terms of ugliness.
// 
//     * shaver considers rewriting nsUConvModule.cpp
// <jag> shaver: don't do it, not today. Long term
//
// So blame jag.

struct Components {
  nsModuleComponentInfo info;
  const char* mCharsetSrc;
  const char* mCharsetDest;
};

// The list of components we register
static Components gComponents[] = {
  { { "Charset Conversion Manager", NS_ICHARSETCONVERTERMANAGER_CID,
      NS_CHARSETCONVERTERMANAGER_CONTRACTID, NS_NewCharsetConverterManager },
    NULL, NULL, },

  { { "Unicode Decode Helper", NS_UNICODEDECODEHELPER_CID,
      NS_UNICODEDECODEHELPER_CONTRACTID, NS_NewUnicodeDecodeHelper },
    NULL, NULL, },

  { { "Unicode Encode Helper", NS_UNICODEENCODEHELPER_CID,
      NS_UNICODEENCODEHELPER_CONTRACTID, NS_NewUnicodeEncodeHelper },
    NULL, NULL, },

  { { "Platform Charset Information", NS_PLATFORMCHARSET_CID,
      NS_PLATFORMCHARSET_CONTRACTID, NS_NewPlatformCharset },
    NULL, NULL, },

  { { "Charset Alias Information",  NS_CHARSETALIAS_CID,
      NS_CHARSETALIAS_CONTRACTID, NS_NewCharsetAlias },
    NULL, NULL, },

  { { NS_CHARSETMENU_PID, NS_CHARSETMENU_CID,
      NS_RDF_DATASOURCE_CONTRACTID_PREFIX NS_CHARSETMENU_PID,
      NS_NewCharsetMenu},
    NULL, NULL, },

  { { "Text To Sub URI Helper", NS_TEXTTOSUBURI_CID,
      NS_ITEXTTOSUBURI_CONTRACTID, NS_NewTextToSubURI },
    NULL, NULL, },

  { { "ISO-8859-1 To Unicode Converter", NS_ISO88591TOUNICODE_CID,
      NS_ISO88591TOUNICODE_CONTRACTID, NS_NewISO88591ToUnicode },
    "ISO-8859-1", "Unicode", },

  { { "windows-1252 To Unicode Converter", NS_CP1252TOUNICODE_CID,
      NS_CP1252TOUNICODE_CONTRACTID, NS_NewCP1252ToUnicode },
    "windows-1252", "Unicode", },

  { { "x-mac-roman To Unicode Converter", NS_MACROMANTOUNICODE_CID,
      NS_MACROMANTOUNICODE_CONTRACTID, NS_NewMacRomanToUnicode },
    "x-mac-roman", "Unicode", },

  { { "UTF-8 To Unicode Converter", NS_UTF8TOUNICODE_CID,
      NS_UTF8TOUNICODE_CONTRACTID, NS_NewUTF8ToUnicode },
    "UTF-8", "Unicode", },

  { { "Unicode To ISO-8859-1 Converter", NS_UNICODETOISO88591_CID,
      NS_UNICODETOISO88591_CONTRACTID, NS_NewUnicodeToISO88591 },
    "Unicode", "ISO-8859-1", },

  { { "Unicode To windows-1252 Converter", NS_UNICODETOCP1252_CID,
      NS_UNICODETOCP1252_CONTRACTID, NS_NewUnicodeToCP1252 },
    "Unicode", "windows-1252", },

  { { "Unicode To x-mac-roman Converter", NS_UNICODETOMACROMAN_CID,
      NS_UNICODETOMACROMAN_CONTRACTID, NS_NewUnicodeToMacRoman },
    "Unicode", "x-mac-roman", },

  { { "Unicode To UTF-8 Converter", NS_UNICODETOUTF8_CID,
      NS_UNICODETOUTF8_CONTRACTID, NS_NewUnicodeToUTF8 },
    "Unicode", "UTF-8", },
};
#define NUM_COMPONENTS (sizeof(gComponents) / sizeof(gComponents[0]))

NS_IMETHODIMP
nsUConvModule::GetClassObject(nsIComponentManager *aCompMgr,
                              const nsCID& aClass,
                              const nsIID& aIID,
                              void** r_classObj)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  // Defensive programming: Initialize *r_classObj in case of error below
  if (!r_classObj) {
    return NS_ERROR_INVALID_POINTER;
  }
  *r_classObj = NULL;

  if (!mInitialized) {
    rv = Initialize();
    if (NS_FAILED(rv)) {
      return rv;
    }
    mInitialized = PR_TRUE;
  }

  nsCOMPtr<nsIGenericFactory> fact;

  for (unsigned int i = 0; i < NUM_COMPONENTS; i++) {
    if (aClass.Equals(gComponents[i].info.mCID)) {
      rv = NS_NewGenericFactory(getter_AddRefs(fact), &gComponents[i].info);
      break;
    }
  }

  if (fact) {
    rv = fact->QueryInterface(aIID, r_classObj);
  }

  return rv;
}

NS_IMETHODIMP
nsUConvModule::RegisterSelf(nsIComponentManager *aCompMgr,
                            nsIFile* aPath,
                            const char* registryLocation,
                            const char* componentType)
{
  nsresult rv = NS_OK;
  char * cid_string;                                                            
  char buff[1024]; 
  nsIRegistry * registry = NULL;
  nsRegistryKey key;                                                            

#ifdef DEBUG
  printf("*** Registering uconv components\n");
#endif
 
  // get the registry                                                           
  rv = nsServiceManager::GetService(NS_REGISTRY_CONTRACTID, 
      NS_GET_IID(nsIRegistry), (nsISupports**)&registry);                       
  if (NS_FAILED(rv))   
    return rv;

  // open the registry                                                          
  rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);                               
  if (NS_FAILED(rv)) 
    return rv;

  Components* cp = gComponents;
  Components* end = cp + NUM_COMPONENTS;
  while (cp < end) {
    rv = aCompMgr->RegisterComponentWithType(cp->info.mCID, 
                                             cp->info.mDescription,
                                             cp->info.mContractID, aPath, 
                                             registryLocation, PR_TRUE,
                                             PR_TRUE, componentType);
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      printf("nsUConvModule: unable to register %s component => %x\n",
             cp->info.mDescription, rv);
#endif
      break;
    }

    // register Unicode converter component info here.
    if (NULL != cp->mCharsetSrc) {
      cid_string = cp->info.mCID.ToString();                             
      sprintf(buff, "%s/%s", "software/netscape/intl/uconv", cid_string);         
      nsCRT::free(cid_string);                                                    

      rv = registry -> AddSubtree(nsIRegistry::Common, buff, &key);              
      if (NS_FAILED(rv))
        break;

      rv = registry -> SetStringUTF8(key, "source", cp->mCharsetSrc);
      if (NS_FAILED(rv)) 
        break;

      rv = registry -> SetStringUTF8(key, "destination", cp->mCharsetDest);
      if (NS_FAILED(rv)) 
        break;
    }
    cp++;
  }

  if (registry != NULL) {                                                       
    nsServiceManager::ReleaseService(NS_REGISTRY_CONTRACTID, registry);         
  }
  else  {
#ifdef DEBUG
      printf("nsUConvModule: unable to register %s component => %x\n",
             cp->info.mDescription, rv);
#endif
  }

  // XXX also unregister this stuff when time comes
  rv = NS_RegisterConverterManagerData();

  return rv;
}

NS_IMETHODIMP
nsUConvModule::UnregisterSelf(nsIComponentManager *aCompMgr,
                             nsIFile* aPath,
                             const char* registryLocation)
{
#ifdef DEBUG
  printf("*** Unregistering uconv components\n");
#endif
  Components* cp = gComponents;
  Components* end = cp + NUM_COMPONENTS;
  while (cp < end) {
    nsresult rv = aCompMgr->UnregisterComponentSpec(cp->info.mCID, aPath);
    if (NS_FAILED(rv)) {
#ifdef DEBUG
      printf("nsUConvModule: unable to unregister %s component => %x\n",
             cp->info.mDescription, rv);
#endif
    }
    cp++;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUConvModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
  if (!okToUnload) {
    return NS_ERROR_INVALID_POINTER;
  }
  *okToUnload = PR_FALSE;
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

static nsUConvModule *gModule = NULL;

extern "C" NS_EXPORT nsresult
NSGETMODULE_ENTRY_POINT(nsUConvModule) (nsIComponentManager *servMgr,
                                        nsIFile* aPath,
                                        nsIModule** return_cobj)
{
  nsresult rv = NS_OK;

  NS_ASSERTION(return_cobj, "Null argument");
  NS_ASSERTION(gModule == NULL, "nsUConvModule: Module already created.");

  // Create an initialize the layout module instance
  nsUConvModule *m = new nsUConvModule();
  if (!m) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Increase refcnt and store away nsIModule interface to m in return_cobj
  rv = m->QueryInterface(NS_GET_IID(nsIModule), (void**)return_cobj);
  if (NS_FAILED(rv)) {
    delete m;
    m = nsnull;
  }
  gModule = m;                  // WARNING: Weak Reference
  return rv;
}
