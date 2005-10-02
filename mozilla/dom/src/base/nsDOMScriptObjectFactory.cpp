/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK *****
 *
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/27/2000   IBM Corp.       Added PR_CALLBACK for Optlink
 *                               use in OS2
 */

#include "nsDOMScriptObjectFactory.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIObserverService.h"
#include "nsJSEnvironment.h"
#include "nsJSEventListener.h"
#include "nsGlobalWindow.h"
#include "nsIJSContextStack.h"
#include "nsISupportsPrimitives.h"
#include "nsDOMException.h"
#include "nsCRT.h"
#ifdef MOZ_XUL
#include "nsIXULPrototypeCache.h"
#endif
#include "nsICategoryManager.h"

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

nsDOMScriptObjectFactory::nsDOMScriptObjectFactory() :
  mLoadedAllLanguages(PR_FALSE)
{
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");

  if (observerService) {
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }

  nsCOMPtr<nsIExceptionService> xs =
    do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

  if (xs) {
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_DOM_RANGE);
#ifdef MOZ_SVG
    xs->RegisterExceptionProvider(this, NS_ERROR_MODULE_SVG);
#endif
  }
  // And pre-create the javascript language.
  NS_CreateJSRuntime(getter_AddRefs(mLanguageArray[nsIProgrammingLanguage::JAVASCRIPT-1]));
}

NS_INTERFACE_MAP_BEGIN(nsDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIDOMScriptObjectFactory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIExceptionProvider)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMScriptObjectFactory)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMScriptObjectFactory)
NS_IMPL_RELEASE(nsDOMScriptObjectFactory)

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetLanguageRuntime(const nsAString &aLanguageName,
                                             nsILanguageRuntime **aLanguage)
{
  // For now, we just treat the category manager as a hash-table, looking
  // up the contract ID each language request.
  // Hard-code JS for now.
  if (aLanguageName.Equals(NS_LITERAL_STRING("application/javascript")))
    return GetLanguageRuntimeByID(nsIProgrammingLanguage::JAVASCRIPT, aLanguage);

  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString cid;
  rv = catman->GetCategoryEntry(SCRIPT_LANGUAGE_CATEGORY,
                                NS_LossyConvertUTF16toASCII(aLanguageName).get(),
                                getter_Copies(cid));
  if (NS_FAILED(rv)) {
    NS_WARNING("No script language registered for this mime-type");
    return NS_ERROR_FACTORY_NOT_REGISTERED;
  }
  // Now get the language service.
  nsCOMPtr<nsILanguageRuntime> lang =
      do_GetService(cid, &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to get the script language");
    return rv;
  }
  // And stash it away in our array for fast lookup by ID.
  PRUint32 lang_ndx = lang->GetLanguage() - 1;
  NS_ASSERTION(mLanguageArray[lang_ndx] == nsnull || mLanguageArray[lang_ndx] == lang,
               "Got a different language for this ID???");
  mLanguageArray[lang_ndx] = lang;
  *aLanguage = lang;
  NS_IF_ADDREF(*aLanguage);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetLanguageRuntimeByID(PRUint32 aLanguageID, 
                                                 nsILanguageRuntime **aLanguage)
{
  if (aLanguageID == 0 || aLanguageID > nsIProgrammingLanguage::MAX) {
    NS_WARNING("Unknown script language");
    return NS_ERROR_UNEXPECTED;
  }
  *aLanguage = mLanguageArray[aLanguageID-1];
  if (!*aLanguage) {
    if (!mLoadedAllLanguages) {
      LoadAllLanguages();
      *aLanguage = mLanguageArray[aLanguageID-1];
    }
  }
  if (!*aLanguage) {
    NS_WARNING("No such language has been registered");
    return NS_ERROR_UNEXPECTED;
  }
  NS_IF_ADDREF(*aLanguage);
  return NS_OK;
}

nsresult
nsDOMScriptObjectFactory::LoadAllLanguages()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsISimpleEnumerator> catenum;
  rv = catman->EnumerateCategory(SCRIPT_LANGUAGE_CATEGORY,
                                 getter_AddRefs(catenum));
  if (NS_FAILED(rv))
    return rv;

  mLoadedAllLanguages = PR_TRUE;
  nsCOMPtr<nsISupports> supports;
  PRBool hasMoreElements;
  while (NS_SUCCEEDED(catenum->HasMoreElements(&hasMoreElements)) && hasMoreElements) {
    catenum->GetNext(getter_AddRefs(supports));
    if (supports != nsnull) {
      nsCOMPtr<nsISupportsCString> category = do_QueryInterface(supports, &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCAutoString categoryEntry;
        rv = category->GetData(categoryEntry);
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsILanguageRuntime> temp;
          // Just ask GetLanguageRuntime to load it.
          GetLanguageRuntime(NS_ConvertASCIItoUCS2(categoryEntry),
                             getter_AddRefs(temp));
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetIDForLanguage(const nsAString &aLanguageName,
                                           PRUint32 *aLang)
{
    nsCOMPtr<nsILanguageRuntime> languageRuntime;
    nsresult rv;
    rv = GetLanguageRuntime(aLanguageName, getter_AddRefs(languageRuntime));
    if (NS_FAILED(rv))
      return rv;

    *aLang = languageRuntime->GetLanguage();
    return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::NewScriptGlobalObject(PRBool aIsChrome,
                                                nsIScriptGlobalObject **aGlobal)
{
  return NS_NewScriptGlobalObject(aIsChrome, aGlobal);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetClassInfoInstance(nsDOMClassInfoID aID)
{
  return nsDOMClassInfo::GetClassInfoInstance(aID);
}

NS_IMETHODIMP_(nsISupports *)
nsDOMScriptObjectFactory::GetExternalClassInfoInstance(const nsAString& aName)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, nsnull);

  const nsGlobalNameStruct *globalStruct;
  gNameSpaceManager->LookupName(aName, &globalStruct);
  if (globalStruct) {
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
      nsresult rv;
      nsCOMPtr<nsIDOMCIExtension> creator(do_CreateInstance(globalStruct->mCID, &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = creator->RegisterDOMCI(NS_ConvertUCS2toUTF8(aName).get(), this);
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = gNameSpaceManager->LookupName(aName, &globalStruct);
      NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && globalStruct, nsnull);

      NS_ASSERTION(globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
                   "The classinfo data for this class didn't get registered.");
    }
    if (globalStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      return nsDOMClassInfo::GetClassInfoInstance(globalStruct->mData);
    }
  }
  return nsnull;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::Observe(nsISupports *aSubject,
                                  const char *aTopic,
                                  const PRUnichar *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
#ifdef MOZ_XUL
    // Flush the XUL cache since it holds JS roots, and we're about to
    // start the final GC.
    nsCOMPtr<nsIXULPrototypeCache> cache =
      do_GetService("@mozilla.org/xul/xul-prototype-cache;1");

    if (cache)
      cache->Flush();
#endif

    nsCOMPtr<nsIThreadJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (stack) {
      JSContext *cx = nsnull;

      stack->GetSafeJSContext(&cx);

      if (cx) {
        // Do one final GC to clean things up before shutdown.

        ::JS_GC(cx);
      }
    }

    nsGlobalWindow::ShutDown();
    nsDOMClassInfo::ShutDown();

    for (PRUint32 i=0;i<sizeof(mLanguageArray)/sizeof(mLanguageArray[0]);i++)
      if (mLanguageArray[i] != nsnull) {
        mLanguageArray[i]->ShutDown();
        mLanguageArray[i] = nsnull;
      }

    nsCOMPtr<nsIExceptionService> xs =
      do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);

    if (xs) {
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM);
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_DOM_RANGE);
#ifdef MOZ_SVG
      xs->UnregisterExceptionProvider(this, NS_ERROR_MODULE_SVG);
#endif
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::GetException(nsresult result,
				       nsIException *aDefaultException,
				       nsIException **_retval)
{
  switch (NS_ERROR_GET_MODULE(result))
  {
    case NS_ERROR_MODULE_DOM_RANGE:
      return NS_NewRangeException(result, aDefaultException, _retval);
#ifdef MOZ_SVG
    case NS_ERROR_MODULE_SVG:
      return NS_NewSVGException(result, aDefaultException, _retval);
#endif
    default:
      return NS_NewDOMException(result, aDefaultException, _retval);
  }
}

NS_IMETHODIMP
nsDOMScriptObjectFactory::RegisterDOMClassInfo(const char *aName,
					       nsDOMClassInfoExternalConstructorFnc aConstructorFptr,
					       const nsIID *aProtoChainInterface,
					       const nsIID **aInterfaces,
					       PRUint32 aScriptableFlags,
					       PRBool aHasClassInterface,
					       const nsCID *aConstructorCID)
{
  extern nsScriptNameSpaceManager *gNameSpaceManager;

  NS_ENSURE_TRUE(gNameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  return gNameSpaceManager->RegisterDOMCIData(aName,
                                              aConstructorFptr,
                                              aProtoChainInterface,
                                              aInterfaces,
                                              aScriptableFlags,
                                              aHasClassInterface,
                                              aConstructorCID);
}

// Factories
nsresult NS_GetLanguageRuntime(const nsAString &aLanguageName,
                               nsILanguageRuntime **aLanguage)
{
  nsresult rv;
  *aLanguage = nsnull;
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = \
        do_GetService(kDOMScriptObjectFactoryCID, &rv);
  if (NS_FAILED(rv))
    return rv;
  return factory->GetLanguageRuntime(aLanguageName, aLanguage);
}

nsresult NS_GetLanguageRuntimeByID(PRUint32 aLanguageID,
                               nsILanguageRuntime **aLanguage)
{
  nsresult rv;
  *aLanguage = nsnull;
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = \
        do_GetService(kDOMScriptObjectFactoryCID, &rv);
  if (NS_FAILED(rv))
    return rv;
  return factory->GetLanguageRuntimeByID(aLanguageID, aLanguage);
}
