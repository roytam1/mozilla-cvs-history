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

#define NS_IMPL_IDS

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsICharsetAlias.h"
#include "nsIRegistry.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterManager2.h"
#include "nsIStringBundle.h"
#include "nsICharsetDetector.h"
#include "nsILocaleService.h"
#include "nsUConvDll.h"
#include "prmem.h"

// just for CIDs
#include "nsIUnicodeDecodeHelper.h"
#include "nsIUnicodeEncodeHelper.h"

static NS_DEFINE_IID(kRegistryNodeIID, NS_IREGISTRYNODE_IID);
static NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);
static NS_DEFINE_CID(kLocaleServiceCID, NS_LOCALESERVICE_CID); 
static NS_DEFINE_CID(kSupportsArrayCID, NS_SUPPORTSARRAY_CID); 

// Pattern of cached, commonly used, single byte decoder
#define NS_1BYTE_CODER_PATTERN "ISO-8859"
#define NS_1BYTE_CODER_PATTERN_LEN 8
//----------------------------------------------------------------------------
// Class nsCharsetConverterManager [declaration]

/**
 * The actual implementation of the nsICharsetConverterManager2 interface. It   
 * alsoimplements the soon to be deprecated nsICharsetConverterManager. 
 *
 * XXX completely deprecate the nsICharsetConverterManager interface
 * XXX optimise the memory allocations in "scriptable" and "friendly" methods
 *
 * @created         15/Nov/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsCharsetConverterManager : public nsICharsetConverterManager, 
public nsICharsetConverterManager2
{
  NS_DECL_ISUPPORTS

private:

  nsIStringBundle * mDataBundle;
  nsIStringBundle * mTitleBundle;

  nsresult LoadExtensibleBundle(const char * aRegistryKey, 
      nsIStringBundle ** aResult);

  static nsresult RegisterConverterTitles(nsIRegistry * aRegistry, 
      char * aRegistryPath);

  static nsresult RegisterConverterData(nsIRegistry * aRegistry, 
      char * aRegistryPath);

  nsresult GetBundleValue(nsIStringBundle * aBundle, const nsIAtom * aName, 
    nsString * aProp, PRUnichar ** aResult);

  nsresult GetBundleValue(nsIStringBundle * aBundle, const nsIAtom * aName, 
    nsString * aProp, nsIAtom ** aResult);

  nsresult GetRegistryEnumeration(char * aRegistryKey, char * aAddPrefix,
    nsISupportsArray ** aArray);

  nsresult GetRegistryEnumeration2(char * aRegistryKey, PRBool aDecoder,
    nsISupportsArray ** aArray);

public:

  nsCharsetConverterManager();
  virtual ~nsCharsetConverterManager();

  static nsresult RegisterConverterManagerData();

  //--------------------------------------------------------------------------
  // Interface nsICharsetConverterManager [declaration]

  NS_IMETHOD GetUnicodeEncoder(const nsString * aDest, 
      nsIUnicodeEncoder ** aResult);
  NS_IMETHOD GetUnicodeDecoder(const nsString * aSrc, 
      nsIUnicodeDecoder ** aResult);

  NS_IMETHOD GetCharsetLangGroup(nsString * aCharset, nsIAtom ** aResult);

  //--------------------------------------------------------------------------
  // Interface nsICharsetConverterManager2 [declaration]

  NS_IMETHOD GetUnicodeDecoder(const nsIAtom * aCharset,
    nsIUnicodeDecoder ** aResult);
  NS_IMETHOD GetUnicodeEncoder(const nsIAtom * aCharset,
    nsIUnicodeEncoder ** aResult);

  NS_IMETHOD GetDecoderList(nsISupportsArray ** aResult);
  NS_IMETHOD GetEncoderList(nsISupportsArray ** aResult);
  NS_IMETHOD GetCharsetDetectorList(nsISupportsArray ** aResult);

  NS_IMETHOD GetCharsetAtom(const PRUnichar * aCharset, nsIAtom ** aResult);
  NS_IMETHOD GetCharsetAtom2(const char * aCharset, nsIAtom ** aResult);
  NS_IMETHOD GetCharsetTitle(const nsIAtom * aCharset, PRUnichar ** aResult);
  NS_IMETHOD GetCharsetTitle2(const nsIAtom * aCharset, nsString * aResult);
  NS_IMETHOD GetCharsetData(const nsIAtom * aCharset, const PRUnichar * aProp, 
    PRUnichar ** aResult);
  NS_IMETHOD GetCharsetData2(const nsIAtom * aCharset, const PRUnichar * aProp, 
    nsString * aResult);
  NS_IMETHOD GetCharsetLangGroup(const nsIAtom * aCharset, nsIAtom ** aResult);
};

//----------------------------------------------------------------------------
// Global functions and data [implementation]

NS_IMETHODIMP NS_NewCharsetConverterManager(nsISupports* aOuter, 
                                            const nsIID& aIID,
                                            void** aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsCharsetConverterManager * inst = new nsCharsetConverterManager();
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

NS_IMETHODIMP NS_RegisterConverterManagerData()
{
  return nsCharsetConverterManager::RegisterConverterManagerData();
}

//----------------------------------------------------------------------------
// Class nsCharsetConverterManager [implementation]

NS_IMPL_THREADSAFE_ISUPPORTS2(nsCharsetConverterManager,
                              nsICharsetConverterManager, 
                              nsICharsetConverterManager2);

nsCharsetConverterManager::nsCharsetConverterManager() 
:mDataBundle(NULL), mTitleBundle(NULL)
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
}

nsCharsetConverterManager::~nsCharsetConverterManager() 
{
  NS_IF_RELEASE(mDataBundle);
  NS_IF_RELEASE(mTitleBundle);
  PR_AtomicDecrement(&g_InstanceCount);
}

nsresult nsCharsetConverterManager::RegisterConverterManagerData()
{
  nsresult res = NS_OK;

  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  RegisterConverterTitles(registry, NS_TITLE_BUNDLE_REGISTRY_KEY);
  RegisterConverterData(registry, NS_DATA_BUNDLE_REGISTRY_KEY);

  return NS_OK;
}

nsresult nsCharsetConverterManager::RegisterConverterTitles(
                                    nsIRegistry * aRegistry,
                                    char * aRegistryPath)
{
  nsresult res;
  nsRegistryKey key;

  nsAutoString str; str.AssignWithConversion(aRegistryPath);
  str.AppendWithConversion("defaultFile");

  char * p = str.ToNewCString();
  res = aRegistry->AddSubtree(nsIRegistry::Common, p, &key);
  nsMemory::Free(p);
  if (NS_FAILED(res)) return res;
  res = aRegistry->SetStringUTF8(key, "name", "chrome://global/locale/charsetTitles.properties");
  if (NS_FAILED(res)) return res;

  return NS_OK;
}

nsresult nsCharsetConverterManager::RegisterConverterData(
                                    nsIRegistry * aRegistry,
                                    char * aRegistryPath)
{
  nsresult res;
  nsRegistryKey key;

  nsAutoString str; str.AssignWithConversion(aRegistryPath);
  str.AppendWithConversion("defaultFile");

  char * p = str.ToNewCString();
  res = aRegistry->AddSubtree(nsIRegistry::Common, p, &key);
  nsMemory::Free(p);
  if (NS_FAILED(res)) return res;
  res = aRegistry->SetStringUTF8(key, "name", "resource:/res/charsetData.properties");
  if (NS_FAILED(res)) return res;

  return NS_OK;
}

nsresult nsCharsetConverterManager::LoadExtensibleBundle(
                                    const char* aRegistryKey, 
                                    nsIStringBundle ** aResult)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsILocale> locale = nsnull;

  NS_WITH_SERVICE(nsIStringBundleService, sbServ, kStringBundleServiceCID, &res);
  if (NS_FAILED(res)) return res;

  NS_WITH_SERVICE(nsILocaleService, localeServ, kLocaleServiceCID, &res);
  if (NS_FAILED(res)) return res;

  res = localeServ->GetApplicationLocale(getter_AddRefs(locale));
  if (NS_FAILED(res)) return res;

  res = sbServ->CreateExtensibleBundle(aRegistryKey, locale, aResult);
  if (NS_FAILED(res)) return res;

  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const nsIAtom * aName, 
                                                   nsString * aProp, 
                                                   PRUnichar ** aResult)
{
  nsresult res = NS_OK;

  nsAutoString key;
  res = ((nsIAtom *) aName)->ToString(key);
  if (NS_FAILED(res)) return res;

  key.ToLowerCase(); // we lowercase the main comparison key
  if (aProp != NULL) key.Append(*aProp); // yes, this param may be NULL

  res = aBundle->GetStringFromName(key.GetUnicode(), aResult);
  return res;
}

nsresult nsCharsetConverterManager::GetBundleValue(nsIStringBundle * aBundle, 
                                                   const nsIAtom * aName, 
                                                   nsString * aProp, 
                                                   nsIAtom ** aResult)
{
  nsresult res = NS_OK;

  PRUnichar * value;
  res = GetBundleValue(aBundle, aName, aProp, &value);
  if (NS_FAILED(res)) return res;

  *aResult =  NS_NewAtom(value);
  PR_Free(value);

  return NS_OK;
}

nsresult nsCharsetConverterManager::GetRegistryEnumeration(
                                    char * aRegistryKey,
                                    char * aAddPrefix,
                                    nsISupportsArray ** aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsISupportsArray> array = NULL;
  nsRegistryKey key;

  res = nsComponentManager::CreateInstance(kSupportsArrayCID, NULL, 
      NS_GET_IID(nsISupportsArray), getter_AddRefs(array));
  if (NS_FAILED(res)) return res;

  // get the registry
  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common, aRegistryKey, &key);
  if (NS_FAILED(res)) return res;

  // enumerate subtrees
  nsCOMPtr<nsIEnumerator> enumerator;
  res = registry->EnumerateSubtrees(key, getter_AddRefs(enumerator));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRegistryEnumerator> components = do_QueryInterface(enumerator, &res);
  if (NS_FAILED(res)) return res;

  res = components->First();
  if (NS_FAILED(res)) return res;

  while (NS_OK != components->IsDone()) {
    const char *name;
    nsAutoString fullName; fullName.AssignWithConversion(aAddPrefix);
    nsCOMPtr<nsIAtom> atom;

    res = components->CurrentItemInPlaceUTF8(&key, &name);
    if (NS_FAILED(res)) goto next;

    fullName.AppendWithConversion(name);
    res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
    if (NS_FAILED(res)) goto next;

    res = array->AppendElement(atom);
    if (NS_FAILED(res)) goto next;

next:
    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // everything was fine, set the result
  *aArray = array;
  NS_ADDREF(*aArray);

  return res;
}

// XXX deprecate this method and switch to GetRegistryEnumeration() when 
// changing the registration style for converters.
// The idea is to have two trees:
// .../uconv/decoder/name
// .../uconv/encoder/name
nsresult nsCharsetConverterManager::GetRegistryEnumeration2(
                                    char * aRegistryKey, 
                                    PRBool aDecoder,
                                    nsISupportsArray ** aArray)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsISupportsArray> array = NULL;
  nsRegistryKey key;

  res = nsComponentManager::CreateInstance(kSupportsArrayCID, NULL, 
      NS_GET_IID(nsISupportsArray), getter_AddRefs(array));
  if (NS_FAILED(res)) return res;

  // get the registry
  NS_WITH_SERVICE(nsIRegistry, registry, kRegistryCID, &res);
  if (NS_FAILED(res)) return res;

  // open registry if necessary
  PRBool regOpen = PR_FALSE;
  registry->IsOpen(&regOpen);
  if (!regOpen) {
    res = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
    if (NS_FAILED(res)) return res;
  }

  // get subtree
  res = registry->GetSubtree(nsIRegistry::Common, aRegistryKey, &key);
  if (NS_FAILED(res)) return res;

  // enumerate subtrees
  nsCOMPtr<nsIEnumerator> enumerator;
  res = registry->EnumerateSubtrees(key, getter_AddRefs(enumerator));
  if (NS_FAILED(res)) return res;

  nsCOMPtr<nsIRegistryEnumerator> components = do_QueryInterface(enumerator, &res);
  if (NS_FAILED(res)) return res;

  res = components->First();
  if (NS_FAILED(res)) return res;

  while (NS_OK != components->IsDone()) {
    const char *name;
    char *src;
    char *dest;
    nsAutoString fullName;
    nsCOMPtr<nsIAtom> atom;

    res = components->CurrentItemInPlaceUTF8(&key, &name);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "source", &src);
    if (NS_FAILED(res)) goto done1;

    res = registry->GetStringUTF8(key, "destination", &dest);
    if (NS_FAILED(res)) goto done1;

    if (aDecoder) {
      if (!strcmp(dest, "Unicode")) {
        fullName.AssignWithConversion(src);
        res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
        if (NS_FAILED(res)) goto done1;

        res = array->AppendElement(atom);
        if (NS_FAILED(res)) goto done1;
      }
    } else {
      if (!strcmp(src, "Unicode")) {
        fullName.AssignWithConversion(dest);
        res = GetCharsetAtom(fullName.GetUnicode(), getter_AddRefs(atom));
        if (NS_FAILED(res)) goto done1;

        res = array->AppendElement(atom);
        if (NS_FAILED(res)) goto done1;
      }
    }

done1:
    if (src != NULL) nsCRT::free(src);
    if (dest != NULL) nsCRT::free(dest);

    res = components->Next();
    if (NS_FAILED(res)) break; // this is NOT supposed to fail!
  }

  // everything was fine, set the result
  *aArray = array;
  NS_ADDREF(*aArray);

  return res;
}

//----------------------------------------------------------------------------
// Interface nsICharsetConverterManager [implementation]

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeEncoder(
                                         const nsString * aDest, 
                                         nsIUnicodeEncoder ** aResult)
{
  *aResult= nsnull;
  nsresult res = NS_OK;

  static const char kUnicodeEncoderProgIDBase[] = NS_UNICODEENCODER_PROGID_BASE;
  static PRInt32 baselen = sizeof(kUnicodeEncoderProgIDBase) - 1;

  char progid[256];
  PL_strncpy(progid, kUnicodeEncoderProgIDBase, 256);
  aDest->ToCString(progid + baselen, 256 - baselen);

  nsCOMPtr<nsIUnicodeEncoder> encoder;
  // Always create an instance since encoders hold state.
  encoder = do_CreateInstance(progid, &res);

  if (NS_FAILED(res))
    res = NS_ERROR_UCONV_NOCONV;
  else
  {
    *aResult = encoder.get();
    NS_ADDREF(*aResult);
  }
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeDecoder(
                                         const nsString * aSrc, 
                                         nsIUnicodeDecoder ** aResult)
{
  *aResult= nsnull;
  nsresult res = NS_OK;;

  static const char kUnicodeDecoderProgIDBase[] = NS_UNICODEDECODER_PROGID_BASE;
  static PRInt32 baselen = sizeof(kUnicodeDecoderProgIDBase) - 1;

  char progid[256];
  PL_strncpy(progid, kUnicodeDecoderProgIDBase, 256);
  aSrc->ToCString(progid + baselen, 256 - baselen);

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  if (!strncmp(progid+baselen, NS_1BYTE_CODER_PATTERN, NS_1BYTE_CODER_PATTERN_LEN))
  {
    // Single byte decoders dont hold state. Optimize by using a service.
    decoder = do_GetService(progid, &res);
  }
  else
  {
    decoder = do_CreateInstance(progid, &res);
  }
  if(NS_FAILED(res))
    res = NS_ERROR_UCONV_NOCONV;
  else
  {
    *aResult = decoder.get();
    NS_ADDREF(*aResult);
  }
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetLangGroup(
                                         nsString * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsAutoString prop; prop.AssignWithConversion(".LangGroup");

  nsCOMPtr<nsIAtom> atom;
  nsresult res = GetCharsetAtom(aCharset->GetUnicode(), getter_AddRefs(atom));
  if (NS_FAILED(res)) return res;

  res = GetCharsetLangGroup(atom, aResult);
  return res;
}

//----------------------------------------------------------------------------
// Interface nsICharsetConverterManager2 [implementation]

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeDecoder(
                                         const nsIAtom * aCharset, 
                                         nsIUnicodeDecoder ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  // XXX use nsImmutableString
  nsAutoString name;
  NS_CONST_CAST(nsIAtom*, aCharset)->ToString(name);
  return GetUnicodeDecoder(&name, aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeEncoder(
                                         const nsIAtom * aCharset, 
                                         nsIUnicodeEncoder ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  // XXX use nsImmutableString
  nsAutoString name;
  NS_CONST_CAST(nsIAtom*, aCharset)->ToString(name);
  return GetUnicodeEncoder(&name, aResult);
}

// XXX move this macro into the UnicodeDecoder/Encoder interface
#define NS_CHARSET_CONVERTER_REG_BASE   "software/netscape/intl/uconv"

NS_IMETHODIMP nsCharsetConverterManager::GetDecoderList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration2(NS_CHARSET_CONVERTER_REG_BASE, PR_TRUE, 
    aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetEncoderList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration2(NS_CHARSET_CONVERTER_REG_BASE, PR_FALSE, 
    aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetDetectorList(
                                         nsISupportsArray ** aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  return GetRegistryEnumeration(NS_CHARSET_DETECTOR_REG_BASE, "chardet.", 
    aResult);
}

// XXX Improve the implementation of this method. Right now, it is build on 
// top of two things: the nsCharsetAlias service and the Atom engine. We can 
// improve on both. First, make the nsCharsetAlias better, with its own hash 
// table (not the StringBundle anymore) and a nicer file format. Second, 
// reimplement the Atom engine for the specific Charset case - more optimal.
// Finally, unify the two for even better performance.
NS_IMETHODIMP nsCharsetConverterManager::GetCharsetAtom(
                                         const PRUnichar * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res;

  // We try to obtain the preferred name for this charset from the charset 
  // aliases. If we don't get it from there, we just use the original string
  nsAutoString charset(aCharset);
  NS_WITH_SERVICE(nsICharsetAlias, csAlias, kCharsetAliasCID, &res);
  NS_ASSERTION(NS_SUCCEEDED(res), "failed to get the CharsetAlias service");
  if (NS_SUCCEEDED(res)) {
    nsAutoString pref;
    res = csAlias->GetPreferred(charset, pref);
    if (NS_SUCCEEDED(res)) {
      charset.Assign(pref);
    }
  }

  // turn that cannonical name into an Atom
  nsCOMPtr<nsIAtom> atom =  getter_AddRefs(NS_NewAtom(charset));
  if (atom.get() == NULL) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // everything was fine, set the result
  *aResult = atom;
  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetAtom2(
                                         const char * aCharset, 
                                         nsIAtom ** aResult)
{
  nsAutoString str;
  str.AssignWithConversion(aCharset);
  return GetCharsetAtom(str.GetUnicode(), aResult);
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetTitle(
                                         const nsIAtom * aCharset, 
                                         PRUnichar ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop; prop.AssignWithConversion(".title");

  if (mTitleBundle == NULL) {
    res = LoadExtensibleBundle(NS_TITLE_BUNDLE_REGISTRY_KEY, &mTitleBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mTitleBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetTitle2(
                                         const nsIAtom * aCharset, 
                                         nsString * aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;

  PRUnichar * title;
  res = GetCharsetTitle(aCharset, &title);
  if (NS_FAILED(res)) return res;

  aResult->Assign(title);
  PR_Free(title);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetData(
                                         const nsIAtom * aCharset, 
                                         const PRUnichar * aProp,
                                         PRUnichar ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  // aProp can be NULL
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop(aProp);

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, &prop, aResult);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetData2(
                                         const nsIAtom * aCharset, 
                                         const PRUnichar * aProp,
                                         nsString * aResult)
{
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;

  PRUnichar * data;
  res = GetCharsetData(aCharset, aProp, &data);
  if (NS_FAILED(res)) return res;

  aResult->Assign(data);
  PR_Free(data);
  return res;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetLangGroup(
                                         const nsIAtom * aCharset, 
                                         nsIAtom ** aResult)
{
  if (aCharset == NULL) return NS_ERROR_NULL_POINTER;
  if (aResult == NULL) return NS_ERROR_NULL_POINTER;
  *aResult = NULL;

  nsresult res = NS_OK;
  nsAutoString prop; prop.AssignWithConversion(".LangGroup");

  if (mDataBundle == NULL) {
    res = LoadExtensibleBundle(NS_DATA_BUNDLE_REGISTRY_KEY, &mDataBundle);
    if (NS_FAILED(res)) return res;
  }

  res = GetBundleValue(mDataBundle, aCharset, &prop, aResult);
  return res;
}
