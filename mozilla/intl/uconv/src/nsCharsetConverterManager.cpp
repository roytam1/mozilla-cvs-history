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
 */

#define NS_IMPL_IDS

#include "pratom.h"
#include "nsString.h"
#include "nsIComponentManager.h"
#include "nsIRegistry.h"
#include "nsIEnumerator.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetConverterInfo.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsUConvDll.h"

// just for CIDs
#include "nsIUnicodeDecodeHelper.h"
#include "nsIUnicodeEncodeHelper.h"

// XXX to be moved with its own factory
#include "nsIUnicodeDecodeUtil.h"
#include "nsUnicodeDecodeUtil.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]

struct ConverterInfo
{
  nsString      * mCharset;
  const nsCID   * mCID;
  PRBool          mFreeCID;

  ConverterInfo()
  :  mCharset(nsnull), mCID(nsnull), mFreeCID(PR_FALSE) {}

  ~ConverterInfo()
  {
    if (mCharset != NULL) delete mCharset;
    if (mFreeCID) delete (nsCID*)mCID;
  }
};

//----------------------------------------------------------------------
// Class nsCharsetConverterManager [declaration]

/**
 * The actual implementation of the nsICharsetConverterManager interface.
 *
 * Requirements for a Manager:
 * - singleton
 * - special lifetime (survive even if it's not used, die only under memory 
 * pressure conditions or app termination)
 * 
 * Ways of implementing it & fulfill those requirements:
 * + simple xpcom object (current implementation model)
 * - xpcom service (best, but no support available yet)
 * - global class with static methods (ie part of the platform)
 *
 * Interesting observation: the NS_IMPL_IDS macros suck! In a given file, one
 * can only declare OR define IDs, not both. Considering the unique-per-dll 
 * implementation, the headers inclusion and IDs declaration vs. definition 
 * becomes quite tricky.
 *
 * XXX make this component thread safe
 * 
 * XXX Use the more general xpcom extensibility model when it will be ready.
 * That means: Component Categories + Monikers. Better performance!
 * 
 * @created         17/Nov/1998
 * @author  Catalin Rotaru [CATA]
 */
class nsCharsetConverterManager : public nsICharsetConverterManager
{
  NS_DECL_ISUPPORTS

private:

  /**
   * Pointer to the unique instance of this class.
   */
  static nsICharsetConverterManager * mInstance;

  /**
   * The Mapping data structures.
   */
  ConverterInfo     mEncArray[100];
  PRInt32           mEncSize;
  ConverterInfo     mDecArray[100];
  PRInt32           mDecSize;

  PRBool            mMappingDone;

  /**
   * Class constructor.
   */
  nsCharsetConverterManager();

  /**
   * Creates some sort of mapping (Charset, Charset) -> Converter.
   */
  nsresult CreateMapping();

  /**
   * Gathers the necessary informations about each Converter.
   */
  nsresult GatherConvertersInfo();

  /**
   * Attempts to return a ICharsetConverterInfo reference for the given charset
   * in the array. If errors, the converter will be eliminated from the array.
   */
  nsICharsetConverterInfo * GetICharsetConverterInfo(ConverterInfo * ci, 
      PRInt32 aIndex, PRInt32 * aSize);

  /**
   * General method for finding and instantiating a Converter.
   */
  nsresult GetCharsetConverter(const nsString * aSrc, void ** aResult, 
      const nsCID * aCID, const ConverterInfo * aArray, PRInt32 aSize);

public:

  /**
   * Class destructor.
   */
  virtual ~nsCharsetConverterManager();

  /**
   * Unique factory method for this class (the constructor is private).
   *
   * @return    a singleton object, instance of this class
   */
  static nsICharsetConverterManager * GetInstance();

  //--------------------------------------------------------------------
  // Interface nsICharsetConverterManager [declaration]

  NS_IMETHOD GetUnicodeEncoder(const nsString * aDest, 
      nsIUnicodeEncoder ** aResult);
  NS_IMETHOD GetUnicodeDecoder(const nsString * aSrc, 
      nsIUnicodeDecoder ** aResult);
  NS_IMETHOD GetEncodableCharsets(nsString *** aResult, PRInt32 * aCount);
  NS_IMETHOD GetDecodableCharsets(nsString *** aResult, PRInt32 * aCount);
  NS_IMETHOD GetCharsetName(const nsString * aCharset, nsString ** aResult);
  NS_IMETHOD GetCharsetNames(const nsString * aCharset, nsString *** aResult, 
      PRInt32 * aCount);
};

//----------------------------------------------------------------------
// Class nsCharsetConverterManager [implementation]

NS_IMPL_ISUPPORTS(nsCharsetConverterManager, nsCOMTypeInfo<nsICharsetConverterManager>::GetIID());

nsICharsetConverterManager * nsCharsetConverterManager::mInstance = NULL;

nsCharsetConverterManager::nsCharsetConverterManager() 
{
  mEncSize      = 0;
  mDecSize      = 0;

  mMappingDone  = PR_FALSE;

  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);

  CreateMapping();
}

nsCharsetConverterManager::~nsCharsetConverterManager() 
{
  mInstance = NULL;
  PR_AtomicDecrement(&g_InstanceCount);
}

nsICharsetConverterManager * nsCharsetConverterManager::GetInstance()
{
  if (mInstance == NULL) mInstance = new nsCharsetConverterManager();
  return mInstance;
}

nsresult nsCharsetConverterManager::CreateMapping()
{
  mMappingDone = PR_TRUE;

  nsresult res = NS_OK;
  nsIRegistry * registry = NULL;
  nsIEnumerator * components = NULL;
  nsRegistryKey uconvKey, key;

  // XXX hack; make these dynamic
  mEncSize = mDecSize = 0;

  // get the registry
  res = nsServiceManager::GetService(NS_REGISTRY_PROGID, 
                                     nsIRegistry::GetIID(), 
                                     (nsISupports**)&registry);
  if (NS_FAILED(res)) goto done;

  // open the registry
  res = registry->OpenWellKnownRegistry(
      nsIRegistry::ApplicationComponentRegistry);
  if (NS_FAILED(res)) goto done;

  // get subtree
  res = registry -> GetSubtree(nsIRegistry::Common, 
                               "software/netscape/intl/uconv", 
                               &uconvKey);
  if (NS_FAILED(res)) goto done;

  // enumerate subtrees
  res = registry -> EnumerateSubtrees(uconvKey, &components);
  if (NS_FAILED(res)) goto done;
  res = components -> First();
  if (NS_FAILED(res)) goto done;

  // XXX take these KONSTANTS out of here
  // XXX check return values, free stuff
  // XXX bit hacky, clean me up
  while (NS_OK != components -> IsDone()) {
    nsISupports * base;
    res = components -> CurrentItem(&base);
    if (NS_SUCCEEDED(res)) {
      nsIRegistryNode * node;
      nsIID nodeIID = NS_IREGISTRYNODE_IID;
      res = base->QueryInterface (nodeIID, (void**)&node);
      if (NS_FAILED(res)) continue;

      char *name;
      res = node->GetName(&name);
      if (NS_FAILED(res)) continue;

      nsCID * cid = new nsCID();
      char * src;
      char * dest;
      if (!cid->Parse(name)) continue;

      res = node->GetKey(&key);
      if (NS_FAILED(res)) continue;

      res = registry->GetString(key, "source", &src);
      if (NS_FAILED(res)) continue;
      res = registry->GetString(key, "destination", &dest);
      if (NS_FAILED(res)) {
          nsCRT::free(src);
          continue;
      }

      nsAutoString str;

      if (!strcmp(src, "Unicode")) {
        str.Assign(dest);
        GetCharsetName(&str,&mEncArray[mEncSize].mCharset);
        mEncArray[mEncSize].mCID = cid;
        mEncArray[mEncSize].mFreeCID = PR_TRUE;
        mEncSize++;
      } else if (!strcmp(dest, "Unicode")) {
        str.Assign(src);
        GetCharsetName(&str,&mDecArray[mDecSize].mCharset);
        mDecArray[mDecSize].mCID = cid;
        mDecArray[mDecSize].mFreeCID = PR_TRUE;
        mDecSize++;
      }

      nsCRT::free(src);
      nsCRT::free(dest);
      nsCRT::free(name);
      NS_RELEASE(node);
      NS_RELEASE(base);
    }

    res = components->Next();
  }

done:
  if (registry != NULL) {
    registry -> Close();
    nsServiceManager::ReleaseService(NS_REGISTRY_PROGID, registry);
  }

  NS_IF_RELEASE(components);

  return res;
}


nsresult nsCharsetConverterManager::GatherConvertersInfo()
{
  nsICharsetConverterInfo * info;
  nsString * str;
  PRInt32 i;

  for (i=0;i<mEncSize;) {
    info = GetICharsetConverterInfo(mEncArray, i, &mEncSize);
    if (info == NULL) continue;

    char *charset;
    info->GetCharsetDest(&charset);
    str = new nsString(charset);
    GetCharsetName(str,&mEncArray[i].mCharset);
    delete str;
	i++;
    NS_RELEASE(info);
  }

  for (i=0;i<mDecSize;) {
    info = GetICharsetConverterInfo(mDecArray, i, &mDecSize);
    if (info == NULL) continue;

    char *charset;
    info->GetCharsetSrc(&charset);
    str = new nsString(charset);
    GetCharsetName(str,&mDecArray[i].mCharset);
    delete str;
	i++;
    NS_RELEASE(info);
  }

  return NS_OK;
}

nsICharsetConverterInfo * 
nsCharsetConverterManager::GetICharsetConverterInfo(ConverterInfo * aArray,
                                                    PRInt32 aIndex,
                                                    PRInt32 * aSize)
{
  nsresult res;
  nsIFactory * factory;
  nsICharsetConverterInfo * info;

  res=nsComponentManager::FindFactory(*(aArray[aIndex].mCID), &factory);
  if (NS_FAILED(res)) {
    #if 1
      printf(" get charset info FindFactory failed %d %x", aIndex, res);
    #endif
    goto reduceArray;
  }

  res=factory->QueryInterface(kICharsetConverterInfoIID, (void ** )&info);
  NS_RELEASE(factory);
  if (NS_FAILED(res)) {
    #if 1
      printf(" get charset info QI failed %d %x", aIndex, res);
    #endif
    goto reduceArray;
  }

  return info;

reduceArray:

  PRInt32 i;

  (*aSize)--;
  for (i=aIndex; i<*aSize;) aArray[i] = aArray[++i];
  if (i>=0) {
    aArray[i].mCharset = NULL;
    aArray[i].mCID = NULL;
  }

  return NULL;
}

nsresult nsCharsetConverterManager::GetCharsetConverter(
                                    const nsString * aSrc, 
                                    void ** aResult,
                                    const nsCID * aCID,
                                    const ConverterInfo * aArray,
                                    PRInt32 aSize)
{
  nsresult res = NS_ERROR_UCONV_NOCONV;
  nsString * str;
  GetCharsetName(aSrc, &str);

  *aResult = NULL;
  for (PRInt32 i=0; i<aSize; i++) if (str->EqualsIgnoreCase(*(aArray[i].mCharset))) {
    res = nsComponentManager::CreateInstance(*(aArray[i].mCID),NULL,*aCID,aResult);
    break;
  }

  delete str;

  // well, we didn't found any converter. Damn, life sucks!
  if ((*aResult == NULL) && (NS_SUCCEEDED(res))) 
    res = NS_ERROR_UCONV_NOCONV;

  return res;
}

//----------------------------------------------------------------------
// Interface nsICharsetConverterManager [implementation]

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeEncoder(
                                         const nsString * aDest, 
                                         nsIUnicodeEncoder ** aResult)
{
  nsresult res;
  if (!mMappingDone) {
    res = CreateMapping();
    if NS_FAILED(res) return res;
  }

  return GetCharsetConverter(aDest, (void **) aResult, &kIUnicodeEncoderIID, 
      mEncArray, mEncSize);
}

NS_IMETHODIMP nsCharsetConverterManager::GetUnicodeDecoder(
                                         const nsString * aSrc, 
                                         nsIUnicodeDecoder ** aResult)
{
  nsresult res;
  if (!mMappingDone) {
    res = CreateMapping();
    if NS_FAILED(res) return res;
  }

  return GetCharsetConverter(aSrc, (void **) aResult, &kIUnicodeDecoderIID, 
      mDecArray, mDecSize);
}

NS_IMETHODIMP nsCharsetConverterManager::GetEncodableCharsets(
                                         nsString *** aResult, 
                                         PRInt32 * aCount)
{
  nsresult res;
  if (!mMappingDone) {
    res = CreateMapping();
    if NS_FAILED(res) return res;
  }

  *aResult = NULL;
  *aCount = 0;
  if (mEncSize != 0) {
    *aResult = new nsString * [mEncSize];
    *aCount = mEncSize;
    for (PRInt32 i=0;i<mEncSize;i++) (*aResult)[i]=mEncArray[i].mCharset;
  }

  return NS_OK;
}

NS_IMETHODIMP nsCharsetConverterManager::GetDecodableCharsets(
                                         nsString *** aResult, 
                                         PRInt32 * aCount)
{
  nsresult res;
  if (!mMappingDone) {
    res = CreateMapping();
    if NS_FAILED(res) return res;
  }

  *aResult = NULL;
  *aCount = 0;
  if (mDecSize != 0) {
    *aResult = new nsString * [mDecSize];
    *aCount = mDecSize;
    for (PRInt32 i=0;i<mDecSize;i++) (*aResult)[i]=mDecArray[i].mCharset;
  }

  return NS_OK;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetName(
                                         const nsString * aCharset,
                                         nsString ** aResult)
{
  // XXX add aliases capability here

  *aResult = new nsString(*aCharset);
  return NS_OK;
}

NS_IMETHODIMP nsCharsetConverterManager::GetCharsetNames(
                                         const nsString * aCharset, 
                                         nsString *** aResult, 
                                         PRInt32 * aCount)
{
  // XXX write me
  return NS_OK;
}

//----------------------------------------------------------------------

NS_IMETHODIMP
NS_NewCharsetConverterManager(nsISupports* aOuter, const nsIID& aIID,
                              void** aResult)
{
  if (!aResult) {
    return NS_ERROR_INVALID_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsICharsetConverterManager* inst = nsCharsetConverterManager::GetInstance();
  if (!inst) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return inst->QueryInterface(aIID, aResult);
}
