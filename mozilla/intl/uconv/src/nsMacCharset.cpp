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

#include "nsIPlatformCharset.h"
#include "pratom.h"
#include "nsURLProperties.h"
#include <Script.h>
#include "nsUConvDll.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIMacLocale.h"
#include "nsLocaleCID.h"

NS_DEFINE_IID(kMacLocaleIID,NS_IMACLOCALE_IID);
NS_DEFINE_CID(kMacLocaleFactoryCID,NS_MACLOCALEFACTORY_CID);


class nsMacCharset : public nsIPlatformCharset
{
  NS_DECL_ISUPPORTS

public:

  nsMacCharset();
  virtual ~nsMacCharset();

  NS_IMETHOD GetCharset(nsPlatformCharsetSel selector, nsString& oResult);
  NS_IMETHOD GetDefaultCharsetForLocale(const PRUnichar* localeName, PRUnichar** _retValue);

private:
  nsString mCharset;
};

NS_IMPL_ISUPPORTS(nsMacCharset, kIPlatformCharsetIID);

nsMacCharset::nsMacCharset()
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
  nsAutoString propertyURL("resource:/res/maccharset.properties");

  nsURLProperties *info = new nsURLProperties( propertyURL );

  if( info ) { 
	  long ret = ::GetScriptManagerVariable(smRegionCode);
	  PRInt32 reg = (PRInt32)(ret & 0x00FFFF);
 	  nsAutoString regionKey("region.");
	  regionKey.Append(reg, 10);
	  
	  nsresult res = info->Get(regionKey, mCharset);
	  if(NS_FAILED(res)) {
		  ret = ::GetScriptManagerVariable(smSysScript);
		  PRInt32 script = (PRInt32)(ret & 0x00FFFF);
		  nsAutoString scriptKey("script.");
		  scriptKey.Append(script, 10);
	 	  nsresult res = info->Get(scriptKey, mCharset);
	      if(NS_FAILED(res)) {
	      	  mCharset = "x-mac-roman";
		  }   
	  }
	  
	  delete info;
  } else {
  	mCharset = "x-mac-roman";
  }
}
nsMacCharset::~nsMacCharset()
{
  PR_AtomicDecrement(&g_InstanceCount);
}

NS_IMETHODIMP 
nsMacCharset::GetCharset(nsPlatformCharsetSel selector, nsString& oResult)
{
   oResult = mCharset; 
   return NS_OK;
}

NS_IMETHODIMP 
nsMacCharset::GetDefaultCharsetForLocale(const PRUnichar* localeName, PRUnichar** _retValue)
{
	nsCOMPtr<nsIMacLocale>	pMacLocale;
	nsString localeAsString(localeName), charset("x-mac-roman");
	short script, language, region;
	
	nsresult rv = nsComponentManager::CreateInstance(kMacLocaleFactoryCID,nsnull,kMacLocaleIID,
											getter_AddRefs(pMacLocale));
	if (NS_FAILED(rv)) { *_retValue = charset.ToNewUnicode(); return rv; }
	
	rv = pMacLocale->GetPlatformLocale(&localeAsString,&script,&language,&region);
	if (NS_FAILED(rv)) { *_retValue = charset.ToNewUnicode(); return rv; }
	
	nsAutoString property_url("resource:/res/maccharset.properties");
	nsURLProperties *charset_properties = new nsURLProperties(property_url);
	if (!charset_properties) { *_retValue = charset.ToNewUnicode(); return NS_ERROR_OUT_OF_MEMORY; }
	
	nsAutoString locale_key("region.");
	locale_key.Append(region,10);
	
	rv = charset_properties->Get(locale_key,charset);
	if (NS_FAILED(rv)) {
		locale_key = "script.";
		locale_key.Append(script,10);
		rv = charset_properties->Get(locale_key,charset);
		if (NS_FAILED(rv)) { charset="x-mac-roman";}
	}
	
	delete charset_properties;
	*_retValue = charset.ToNewUnicode();	
	return rv;
}

//----------------------------------------------------------------------

NS_IMETHODIMP
NS_NewPlatformCharset(nsISupports* aOuter, 
                      const nsIID &aIID,
                      void **aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsMacCharset* inst = new nsMacCharset();
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
