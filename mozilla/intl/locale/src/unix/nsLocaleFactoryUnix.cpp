/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nscore.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsCollationUnix.h"
#include "nsIScriptableDateFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsDateTimeFormatUnix.h"
#include "nsLocaleFactoryUnix.h"


NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
NS_DEFINE_IID(kIFactoryIID,  NS_IFACTORY_IID);
NS_DEFINE_IID(kICollationFactoryIID, NS_ICOLLATIONFACTORY_IID);                                                         
NS_DEFINE_IID(kICollationIID, NS_ICOLLATION_IID);                                                         
NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);
NS_DEFINE_CID(kScriptableDateFormatCID, NS_SCRIPTABLEDATEFORMAT_CID);



nsLocaleUnixFactory::nsLocaleUnixFactory(const nsCID &aClass)   
{   
  mRefCnt = 0;
  mClassID = aClass;
}   

nsLocaleUnixFactory::~nsLocaleUnixFactory()   
{   
}   

nsresult nsLocaleUnixFactory::QueryInterface(const nsIID &aIID,   
                                      void **aResult)   
{   
  if (aResult == NULL) {   
    return NS_ERROR_NULL_POINTER;   
  }   

  // Always NULL result, in case of failure   
  *aResult = NULL;   

  if (aIID.Equals(kISupportsIID)) {   
    *aResult = (void *)(nsISupports*)this;   
  } else if (aIID.Equals(kIFactoryIID)) {   
    *aResult = (void *)(nsIFactory*)this;   
  }   

  if (*aResult == NULL) {   
    return NS_NOINTERFACE;   
  }   

  NS_ADDREF_THIS(); // Increase reference count for caller   
  return NS_OK;   
}   

NS_IMPL_ADDREF(nsLocaleUnixFactory);
NS_IMPL_RELEASE(nsLocaleUnixFactory);

nsresult nsLocaleUnixFactory::CreateInstance(nsISupports *aOuter,  
                                         const nsIID &aIID,  
                                         void **aResult)  
{
  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  

  nsISupports *inst = NULL;

  if (aIID.Equals(kICollationFactoryIID)) {
     NS_NEWXPCOM(inst, nsCollationFactory);
  }
  else if (aIID.Equals(kICollationIID)) {
     NS_NEWXPCOM(inst, nsCollationUnix);
  }
  else if (aIID.Equals(kIDateTimeFormatIID)) {
     NS_NEWXPCOM(inst, nsDateTimeFormatUnix);
  }
  else if (aIID.Equals(nsIScriptableDateFormat::GetIID())) {
     inst = NEW_SCRIPTABLE_DATEFORMAT();
  }
  else if (mClassID.Equals(kScriptableDateFormatCID)) {
     inst = NEW_SCRIPTABLE_DATEFORMAT();
  }
  else 
  {
    return NS_NOINTERFACE;
  }

  if (NULL == inst) {
    return NS_ERROR_OUT_OF_MEMORY;  
  }
  
  nsresult res = inst->QueryInterface(aIID, aResult);
  
  if(NS_FAILED(res)) {
    delete inst;
  }

  return res;
}

nsresult nsLocaleUnixFactory::LockFactory(PRBool aLock)  
{  
  // Not implemented in simplest case.  
  return NS_OK;
}  

