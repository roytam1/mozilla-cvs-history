/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */


#define NS_IMPL_IDS


#include "nsICharsetAlias.h"
#include "nsCharsetAliasFactory.h"
#include "pratom.h"

#include "nsUConvDll.h"


#include "nsURLProperties.h"
//==============================================================
class nsCharsetAlias2 : public nsICharsetAlias
{
  NS_DECL_ISUPPORTS

public:

  nsCharsetAlias2();
  virtual ~nsCharsetAlias2();

   NS_IMETHOD GetPreferred(const nsString& aAlias, nsString& oResult);
   NS_IMETHOD GetPreferred(const PRUnichar* aAlias, const PRUnichar** oResult) ;
   NS_IMETHOD GetPreferred(const char* aAlias, char* oResult, PRInt32 aBufLength) ;

   NS_IMETHOD Equals(const nsString& aCharset1, const nsString& aCharset2, PRBool* oResult) ;
   NS_IMETHOD Equals(const PRUnichar* aCharset1, const PRUnichar* aCharset2, PRBool* oResult) ;
   NS_IMETHOD Equals(const char* aCharset1, const char* aCharset2, PRBool* oResult) ;

private:
   nsURLProperties* mDelegate;
};

//--------------------------------------------------------------
NS_IMPL_ISUPPORTS(nsCharsetAlias2, kICharsetAliasIID);

//--------------------------------------------------------------
nsCharsetAlias2::nsCharsetAlias2()
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
  nsAutoString propertyURL("resource://res/charsetalias.properties");
  mDelegate = new nsURLProperties( propertyURL );
}
//--------------------------------------------------------------
nsCharsetAlias2::~nsCharsetAlias2()
{
  PR_AtomicDecrement(&g_InstanceCount);
  if(mDelegate)
     delete mDelegate;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::GetPreferred(const nsString& aAlias, nsString& oResult)
{
   nsAutoString aKey;
   aAlias.ToLowerCase(aKey);
   oResult = "";
   return mDelegate->Get(aKey, oResult);
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::GetPreferred(const PRUnichar* aAlias, const PRUnichar** oResult)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::GetPreferred(const char* aAlias, char* oResult, PRInt32 aBufLength) 
{
   return NS_ERROR_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::Equals(const nsString& aCharset1, const nsString& aCharset2, PRBool* oResult)
{
   nsresult res = NS_OK;

   if(aCharset1.EqualsIgnoreCase(aCharset2)) {
      *oResult = PR_TRUE;
      return res;
   }

   if(aCharset1.Equals("") || aCharset2.Equals("")) {
      *oResult = PR_FALSE;
      return res;
   }

   *oResult = PR_FALSE;
   nsString name1;
   nsString name2;
   res = this->GetPreferred(aCharset1, name1);
   if(NS_SUCCEEDED(res)) {
      res = this->GetPreferred(aCharset2, name2);
      if(NS_SUCCEEDED(res)) {
          *oResult = (name1.EqualsIgnoreCase(name2)) ? PR_TRUE : PR_FALSE;
      }
   }
   
   return res;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::Equals(const PRUnichar* aCharset1, const PRUnichar* aCharset2, PRBool* oResult)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias2::Equals(const char* aCharset1, const char* aCharset2, PRBool* oResult)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}
 



//==============================================================
class nsCharsetAlias : public nsICharsetAlias
{
  NS_DECL_ISUPPORTS

public:

  nsCharsetAlias();
  virtual ~nsCharsetAlias();

   NS_IMETHOD GetPreferred(const nsString& aAlias, nsString& oResult);
   NS_IMETHOD GetPreferred(const PRUnichar* aAlias, const PRUnichar** oResult) ;
   NS_IMETHOD GetPreferred(const char* aAlias, char* oResult, PRInt32 aBufLength) ;


   NS_IMETHOD Equals(const nsString& aCharset1, const nsString& aCharset2, PRBool* oResult) ;
   NS_IMETHOD Equals(const PRUnichar* aCharset1, const PRUnichar* aCharset2, PRBool* oResult) ;
   NS_IMETHOD Equals(const char* aCharset1, const char* aCharset2, PRBool* oResult) ;

protected:
   PRBool Equals(const nsString& aCharset1, const nsString& aCharset2);
   const nsString& GetPreferred(const nsString& aAlias) const ;

private:
  
// XXX Hack
   nsString iso88591;
   nsString iso88592;
   nsString iso88597;
   nsString iso88599;
   nsString sjis;
   nsString eucjp;
   nsString iso2022jp;
   nsString utf8;
   nsString xmacroman;
   nsString xmacce;
   nsString xmacgreek;
   nsString xmacturkish;
   nsString unknown;
   nsString windows1250;
   nsString windows1252;
   nsString windows1253;
   nsString windows1254;
   nsString big5;
   nsString euckr;
   nsString euctw;
   nsString gb2312;

};

NS_IMPL_ISUPPORTS(nsCharsetAlias, kICharsetAliasIID);

//--------------------------------------------------------------
nsCharsetAlias::nsCharsetAlias()
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
  
// XXX Hack
  iso88591 =  "ISO-8859-1";
  iso88592 =  "ISO-8859-2";
  iso88597 =  "ISO-8859-7";
  iso88599 =  "ISO-8859-9";
  sjis =      "Shift_JIS";
  eucjp =     "EUC-JP";
  iso2022jp = "ISO-2022-JP";
  xmacroman = "x-mac-roman";
  xmacce = 	  "x-mac-ce";
  xmacgreek = "x-mac-greek";
  xmacturkish = "x-mac-trukish";
  utf8      =  "UTF-8";
  windows1250 =  "Windows-1250";
  windows1252 =  "Windows-1252";
  windows1253 =  "Windows-1253";
  windows1254 =  "Windows-1254";
  big5 =  "BIG5";
  gb2312 =  "GB2312";
  euckr =  "EUC-KR";
  euctw =  "X-EUC-TW";
  unknown   = "";
}

//--------------------------------------------------------------
nsCharsetAlias::~nsCharsetAlias()
{
  PR_AtomicDecrement(&g_InstanceCount);
}

//--------------------------------------------------------------
const nsString& nsCharsetAlias::GetPreferred(
   const nsString& aAlias) const
{
   nsAutoString aKey;
   aAlias.ToLowerCase(aKey);

   // XXX Hack
   // we should delegate the tast to a property file 
   if(aKey.Equals("iso-8859-1") ||
      aKey.Equals("latin1") ||
      aKey.Equals("iso_8859-1") ||
      aKey.Equals("iso_8859-1:1987") ||
      aKey.Equals("iso-ir-100") ||
      aKey.Equals("l1") ||
      aKey.Equals("ibm819") ||
      aKey.Equals("cp819") ||
      aKey.Equals("iso-8859-1-windows-3.0-latin-1") ||
      aKey.Equals("iso-8859-1-windows-3.1-latin-1") ||
      aKey.Equals("windows-1252") )
   {
      return iso88591;
   }
   if(aKey.Equals("utf-8") ||
      aKey.Equals("unicode-1-1-utf-8") )
   {
      return utf8;
   }

   if(aKey.Equals("x-sjis") ||
      aKey.Equals("ms_kanji") ||
      aKey.Equals("csshiftjis") ||
      aKey.Equals("shift_jis") ||
      aKey.Equals("windows-31j") )
   {
      return sjis;
   }
   if(aKey.Equals("euc-jp") ||
      aKey.Equals("cseucjpkdfmtjapanese") ||
      aKey.Equals("x-euc-jp") )
   {
      return eucjp;
   } 
   if(aKey.Equals("iso-2022-jp") ||
      aKey.Equals("csiso2022jp") )
   {
      return iso2022jp;
   } 
   
   // XXX need to verify the BIG5 list
   if(aKey.Equals("big5"))
   {
      return big5;
   }

   // XXX need to verify the x-euc-tw list
   if(aKey.Equals("x-euc-tw"))
   {
      return euctw;
   }

   // XXX need to verify the EUC-KR list
   if(aKey.Equals("euc-kr"))
   {
      return euckr;
   }

   // XXX need to verify the gb2312 list
   if(aKey.Equals("gb2312"))
   {
      return gb2312;
   }

   if(aKey.Equals("x-mac-roman") )
   {
      return xmacroman;
   } 

   // XXX need to verify the x-mac-ce list
   if(aKey.Equals("x-mac-ce") )
   {
      return xmacce;
   } 
   // XXX need to verify the x-mac-greek list
   if(aKey.Equals("x-mac-greek") )
   {
      return xmacgreek;
   } 
   // XXX need to verify the x-mac-turkish list
   if(aKey.Equals("x-mac-turkish") )
   {
      return xmacturkish;
   } 

   if(aKey.Equals("iso-8859-7") ||
      aKey.Equals("iso-ir-126") ||
      aKey.Equals("iso_8859-7") ||
      aKey.Equals("iso_8859-7:1987") ||
      aKey.Equals("elot_928") ||
      aKey.Equals("ecma-118") ||
      aKey.Equals("greek") ||
      aKey.Equals("greek8") ||
      aKey.Equals("csisolatingreek") )
   {
      return iso88597;
   } 

   // XXX need to verify the ISO-8859-2 list
   if(aKey.Equals("iso-8859-2"))
   {
      return iso88592;
   }
   // XXX need to verify the ISO-8859-9 list
   if(aKey.Equals("iso-8859-9"))
   {
      return iso88599;
   }
   
   // XXX need to verify the 1250 list
   if(aKey.Equals("windows-1250") ||
      aKey.Equals("x-cp1250") )
   {
      return windows1250;
   } 
   
   // XXXX windows-1252 
   if(aKey.Equals("windows-1253") ||
      aKey.Equals("x-cp1253") )
   {
      return windows1253;
   } 
   
   // XXX need to verify the 1254 list
   if(aKey.Equals("windows-1254") ||
      aKey.Equals("x-cp1254") )
   {
      return windows1254;
   } 

   return unknown;
}

//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias::GetPreferred(
   const nsString& aAlias, nsString& oResult)
{
   oResult = GetPreferred(aAlias);
   
   if(oResult.Equals(""))
     return NS_ERROR_NOT_AVAILABLE;
   return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias::GetPreferred(
   const PRUnichar* aAlias, const PRUnichar** oResult) 
{
   const nsString& res = GetPreferred(aAlias);
   if(res.Equals(""))
   {
     *oResult = NULL;
     return NS_ERROR_NOT_AVAILABLE;
   } 
   *oResult = res.GetUnicode();
   return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias::GetPreferred(
   const char* aAlias, char* oResult, PRInt32 aBufLength) 
{
   const nsString& res = GetPreferred(aAlias);
   if(res.Equals(""))
   {
     *oResult = (char) NULL;
     return NS_ERROR_NOT_AVAILABLE;
   }
   res.ToCString(oResult, aBufLength);
   return NS_OK;
}

//--------------------------------------------------------------
PRBool nsCharsetAlias::Equals(
   const nsString& aCharset1, const nsString& aCharset2)
{
   if(aCharset1.EqualsIgnoreCase(aCharset2))
      return PR_TRUE;

   if(aCharset1.Equals("") || aCharset2.Equals(""))
      return PR_FALSE;

   const nsString& name1 = GetPreferred(aCharset1);
   const nsString& name2 = GetPreferred(aCharset2);
   return (name1.EqualsIgnoreCase(name2)) ? PR_TRUE : PR_FALSE;
   
}

//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias::Equals(
   const nsString& aCharset1, const nsString& aCharset2, PRBool* oResult) 
{
   PRBool ret = Equals(aCharset1, aCharset2);
   *oResult = ret;
   return NS_OK;
}

//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAlias::Equals(
   const PRUnichar* aCharset1, const PRUnichar* aCharset2, PRBool* oResult) 
{
   PRBool ret = Equals(aCharset1, aCharset2);
   *oResult = ret;
   return NS_OK;
}
//--------------------------------------------------------------

NS_IMETHODIMP nsCharsetAlias::Equals(
   const char* aCharset1, const char* aCharset2, PRBool* oResult) 
{
   PRBool ret = Equals(aCharset1, aCharset2);
   *oResult = ret;
   return NS_OK;
}



//==============================================================
class nsCharsetAliasFactory : public nsIFactory {
   NS_DECL_ISUPPORTS

public:
   nsCharsetAliasFactory() {
     NS_INIT_REFCNT();
     PR_AtomicIncrement(&g_InstanceCount);
   }
   virtual ~nsCharsetAliasFactory() {
     PR_AtomicDecrement(&g_InstanceCount);
   }

   NS_IMETHOD CreateInstance(nsISupports* aDelegate, const nsIID& aIID, void** aResult);
   NS_IMETHOD LockFactory(PRBool aLock);
 
};

//--------------------------------------------------------------
NS_DEFINE_IID( kIFactoryIID, NS_IFACTORY_IID);
NS_IMPL_ISUPPORTS( nsCharsetAliasFactory , kIFactoryIID);

NS_IMETHODIMP nsCharsetAliasFactory::CreateInstance(
    nsISupports* aDelegate, const nsIID &aIID, void** aResult)
{
  if(NULL == aResult) 
        return NS_ERROR_NULL_POINTER;
  if(NULL != aDelegate) 
        return NS_ERROR_NO_AGGREGATION;

  *aResult = NULL;

  nsISupports *inst = new nsCharsetAlias();


  if(NULL == inst) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult res =inst->QueryInterface(aIID, aResult);
  if(NS_FAILED(res)) {
     delete inst;
  }
  
  return res;
}
//--------------------------------------------------------------
NS_IMETHODIMP nsCharsetAliasFactory::LockFactory(PRBool aLock)
{
  if(aLock)
     PR_AtomicIncrement( &g_LockCount );
  else
     PR_AtomicDecrement( &g_LockCount );
  return NS_OK;
}

//==============================================================
nsIFactory* NEW_CHARSETALIASFACTORY()
{
  return new nsCharsetAliasFactory();
}

