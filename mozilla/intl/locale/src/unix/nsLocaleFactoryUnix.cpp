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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nscore.h"
#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h"
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
  NS_INIT_ISUPPORTS();
  mClassID = aClass;
}   

nsLocaleUnixFactory::~nsLocaleUnixFactory()   
{   
}   

NS_IMPL_THREADSAFE_ISUPPORTS1(nsLocaleUnixFactory, nsIFactory)

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
  else if (aIID.Equals(NS_GET_IID(nsIScriptableDateFormat))) {
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
  
  NS_ADDREF(inst);
  nsresult ret = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);
  return ret;
}

nsresult nsLocaleUnixFactory::LockFactory(PRBool aLock)  
{  
  // Not implemented in simplest case.  
  return NS_OK;
}  

