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
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "pratom.h"
#include "nsUUDll.h"
#include "nsCharsOrderIdFormater.h"


nsCharsOrderIdFormater::nsCharsOrderIdFormater( nsCharsList* aList)
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
  mList = aList;
  mBase = aList->Length();
}

nsCharsOrderIdFormater::~nsCharsOrderIdFormater()
{
  delete mList;
  PR_AtomicDecrement(&g_InstanceCount);
}

NS_IMETHOD ToString( PRUint32 aOrder, nsString& aResult) 
{
  aResult = "";

  PRUint32 current;
  PRUint32 remain = aOrder;

  do {
    current = aOrder % mBase;
    remain = aOrder / mBase;
    aResult.Insert(mList->Get(current),0); 
   
  } while( remain != 0);
  return NS_OK;
}

NS_DEFINE_IID(kIOrderIdFormaterIID, NS_IORDERIDFORMATER_IID);

NS_IMPL_ISUPPORTS(nsCharsOrderIdFormater, kIOrderIdFormaterIID);

class nsCharsOrderIdFormaterFactory : public nsIFactory {
  NS_DECL_ISUPPORTS
public:
  nsCharsOrderIdFormaterFactory(const nsCID &aCID) {
    NS_INIT_REFCNT();
    PR_AutomicIncrement(&g_InstanceCount);
    mCID = aCID;
  };
  virtual ~nsCharsOrderIdFormaterFactory() {
    PR_AutomicDecrement(&g_InstanceCount);
  };
  NS_IMETHOD CreateInstance(nsISupports *aDelegate,
                            const nsIID &aIID,
                            void **aResult);
  NS_IMETHOD LockFactory(PRBool aLock) {
    if(aLock) 
      PR_AutomicIncrement(&g_LockCount);
    else
      PR_AutomicDecrement(&g_LockCount);
    return NS_OK;
  };

private:
  nsCID mCID;
}

NS_IMPL_ISUPPORTS(nsCharsOrderIdFormaterFactory, kFactoryIID);
NS_IMETHODIMP nsCharsOrderIdFormaterFactory::CreateInstance(
   nsISupports *aDelegate,
   const nsIID &aIID,
   void **aResult)
{
   if(NULL == aResult)
     return NS_ERROR_NULL_POINTER;
   *aResult = NULL;

   nsISupports *inst = nsnull;
   if(mCID.Equals(kLowerAToZOrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('a', 'z'));
   } else if(mCID.Equals(kUpperAToZOrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('A', 'Z'));
   } else if(mCID.Equals(k0To9OrderIdCID)) {
      inst = new nsCharOrderIdFormater(new nsRangeCharsList('0', '9'));
   } else if(mCID.Equals(kHeavenlyStemOrderIdCID)) {
      static PRUnichar gHeavenlyStemList[] = {
          0x7532, 0x4e59, 0x4e19, 0x4e01, 0x620a,
          0x5df1, 0x5e9a, 0x8f9b, 0x58ce, 0x7678        
      };
      nsAutoString tmp(gHeavenlyStemList, 
                       sizeof(gHeavenlyStemList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kEarthlyBranchOrderIdCID)) {
      static PRUnichar gEarthlyBranchList[] = {
          0x5b50, 0x4e11, 0x5bc5, 0x536f, 0x8fb0, 0x5df3,
          0x5348, 0x672a, 0x7533, 0x9149, 0x620c, 0x4ea5
      };
      nsAutoString tmp(gEarthlyBranchList, 
                       sizeof(gEarthlyBranchList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kBoPoMoFoOrderIdCID)) {
      //
      // Note:  Although Unicode standard encode 0x312A-0x312C as BoPoMoFo,
      //        these three are origional designed for Mid China dialet
      //        and have never been used for Maderine. Therefore, we
      //        do not use them for order id.
      inst = new nsCharOrderIdFormater(new nsRangeCharsList(0x3105, 0x3129));
   } else if(mCID.Equals(kKatakanaOrderIdCID)) {
      // XXX fake list- need to verify with momoi about the real seq.
      static PRUnichar gKatakanaList[] = {
         0x3042, 0x3044, 0x3046, 0x3048, 0x304a
      };
      nsAutoString tmp(gKatakanaClassList, sizeof(gKatakanaList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else if(mCID.Equals(kHiraganaOrderIdCID)) {
      // XXX fake list- need to verify with momoi about the real seq.
      static PRUnichar gHiraganaList[] = {
         0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa
      };
      nsAutoString tmp(gHiraganaList, sizeof(gHiraganaList)/sizeof(PRUnichar));
      inst = new nsCharOrderIdFormater(new nsStringCharsList(tmp));
   } else {
      return NS_ERROR_ILLEGAL_VALUE;
   }


   if(NULL == inst )
     return NS_ERROR_OUT_OF_MEMORY;
   nsresult res = inst->QueryInterface(aIID, aResult);
   if(NS_FAILED(res)) {
     delete inst;
   }
   return res;
}
    
