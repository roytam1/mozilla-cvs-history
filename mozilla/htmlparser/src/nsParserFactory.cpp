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

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIHTMLContentSink.h"
#include "nsParserCIID.h"
#include "nsILoggingSink.h"
#include "nsParser.h"
#include "nsParserNode.h"
#include "nsWellFormedDTD.h"
#include "CNavDTD.h"

#include "nsHTMLTags.h"
#include "nsHTMLEntities.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID,  NS_IFACTORY_IID);

static NS_DEFINE_IID(kCParser,          NS_PARSER_IID);
static NS_DEFINE_IID(kCParserNode,      NS_PARSER_NODE_IID);
static NS_DEFINE_IID(kLoggingSinkCID,   NS_LOGGING_SINK_CID);
static NS_DEFINE_CID(kWellFormedDTDCID, NS_WELLFORMEDDTD_CID);
static NS_DEFINE_CID(kCNavDTDCID,		NS_CNAVDTD_CID);

class nsParserFactory : public nsIFactory
{   
  public:   
    // nsISupports methods   
    NS_IMETHOD QueryInterface(const nsIID &aIID,    
                              void **aResult);   
    NS_IMETHOD_(nsrefcnt) AddRef(void);   
    NS_IMETHOD_(nsrefcnt) Release(void);   

    // nsIFactory methods   
    NS_IMETHOD CreateInstance(nsISupports *aOuter,   
                              const nsIID &aIID,   
                              void **aResult);   

    NS_IMETHOD LockFactory(PRBool aLock);   

    nsParserFactory(const nsCID &aClass);   
    virtual ~nsParserFactory();   

  private:   
    nsrefcnt  mRefCnt;   
    nsCID     mClassID;
};   

nsParserFactory::nsParserFactory(const nsCID &aClass)   
{   
  mRefCnt = 0;
  mClassID = aClass;
  nsHTMLTags::AddRefTable();
  nsHTMLEntities::AddRefTable();
}   

nsParserFactory::~nsParserFactory()   
{   
  nsHTMLEntities::ReleaseTable();
  nsHTMLTags::ReleaseTable();
}   

nsresult nsParserFactory::QueryInterface(const nsIID &aIID,   
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

NS_IMPL_ADDREF(nsParserFactory);
NS_IMPL_RELEASE(nsParserFactory);

nsresult nsParserFactory::CreateInstance(nsISupports *aOuter,  
                                         const nsIID &aIID,  
                                         void **aResult)  
{  
  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  nsISupports *inst = nsnull;

  if (mClassID.Equals(kCParser)) {
    nsParser* p;
    NS_NEWXPCOM(p, nsParser);
    inst = (nsISupports *)(nsIParser *)p;
  }
  else if (mClassID.Equals(kCParserNode)) {
    nsCParserNode* cpn;
    NS_NEWXPCOM(cpn, nsCParserNode);
    inst = (nsISupports *)(nsIParserNode *) cpn;
  }
  else if (mClassID.Equals(kLoggingSinkCID)) {
    nsIContentSink* cs;
    nsresult rv = NS_NewHTMLLoggingSink(&cs);
    if (NS_OK != rv) {
      return rv;
    }
    *aResult = cs;
    return rv;
  }
  else if (mClassID.Equals(kWellFormedDTDCID)) {
    nsresult rv = NS_NewWellFormed_DTD((nsIDTD**) &inst);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  else if (mClassID.Equals(kCNavDTDCID)) {
    nsresult rv = NS_NewNavHTMLDTD((nsIDTD**) &inst);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  if (inst == NULL) {  
    return NS_ERROR_OUT_OF_MEMORY;  
  }  

  nsresult res = inst->QueryInterface(aIID, aResult);

  if (res != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  

  return res;  
}  

nsresult nsParserFactory::LockFactory(PRBool aLock)  
{  
  // Not implemented in simplest case.  
  return NS_OK;
}  

// return the proper factory to the caller
#if defined(XP_MAC) && defined(MAC_STATIC)
extern "C" NS_EXPORT nsresult
NSGetFactory_PARSER_DLL(nsISupports* serviceMgr,
                        const nsCID &aClass,
                        const char *aClassName,
                        const char *aProgID,
                        nsIFactory **aFactory)
#else
extern "C" NS_EXPORT nsresult
NSGetFactory(nsISupports* serviceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
#endif
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsParserFactory(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}
