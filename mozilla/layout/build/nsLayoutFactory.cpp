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
#include "nslayout.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsLayoutCID.h"
#include "nsIDocument.h"
#include "nsIHTMLContent.h"
#include "nsITextContent.h"
#include "nsIPresShell.h"
#include "nsIDOMSelection.h"
#include "nsIFrameUtil.h"

#include "nsHTMLAtoms.h"
#include "nsHTMLParts.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsICSSParser.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsIDOMRange.h"
#include "nsIContentIterator.h"
#include "nsINameSpaceManager.h"
#include "nsIScriptNameSetRegistry.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsIScriptExternalNameSet.h"
#include "nsIEventListenerManager.h"

static NS_DEFINE_IID(kCHTMLDocumentCID, NS_HTMLDOCUMENT_CID);
static NS_DEFINE_IID(kCXMLDocumentCID, NS_XMLDOCUMENT_CID);
static NS_DEFINE_IID(kCImageDocumentCID, NS_IMAGEDOCUMENT_CID);
static NS_DEFINE_IID(kCCSSParserCID,     NS_CSSPARSER_CID);
static NS_DEFINE_CID(kHTMLStyleSheetCID, NS_HTMLSTYLESHEET_CID);
static NS_DEFINE_CID(kHTMLCSSStyleSheetCID, NS_HTML_CSS_STYLESHEET_CID);
static NS_DEFINE_IID(kCHTMLImageElementCID, NS_HTMLIMAGEELEMENT_CID);
static NS_DEFINE_IID(kCHTMLOptionElementCID, NS_HTMLOPTIONELEMENT_CID);
static NS_DEFINE_IID(kCRangeListCID, NS_RANGELIST_CID);
static NS_DEFINE_IID(kCRangeCID,     NS_RANGE_CID);
static NS_DEFINE_IID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_IID(kCSubtreeIteratorCID, NS_SUBTREEITERATOR_CID);
static NS_DEFINE_CID(kPresShellCID,  NS_PRESSHELL_CID);
static NS_DEFINE_CID(kTextNodeCID,   NS_TEXTNODE_CID);
static NS_DEFINE_CID(kNameSpaceManagerCID,  NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kFrameUtilCID,  NS_FRAME_UTIL_CID);
static NS_DEFINE_CID(kEventListenerManagerCID, NS_EVENTLISTENERMANAGER_CID);


extern nsresult NS_NewRangeList(nsIDOMSelection **);
extern nsresult NS_NewRange(nsIDOMRange **);
extern nsresult NS_NewContentIterator(nsIContentIterator **);
extern nsresult NS_NewContentSubtreeIterator(nsIContentIterator **);
extern nsresult NS_NewFrameUtil(nsIFrameUtil** aResult);


////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID, NS_IFACTORY_IID);

class nsLayoutFactory : public nsIFactory
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

    nsLayoutFactory(const nsCID &aClass);   

  protected:
    virtual ~nsLayoutFactory();   

  private:   
    nsrefcnt  mRefCnt;   
    nsCID     mClassID;
};   

nsLayoutFactory::nsLayoutFactory(const nsCID &aClass)   
{   
  mRefCnt = 0;
  mClassID = aClass;
}   

nsLayoutFactory::~nsLayoutFactory()   
{   
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");   
}   

nsresult nsLayoutFactory::QueryInterface(const nsIID &aIID,   
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

  AddRef(); // Increase reference count for caller   
  return NS_OK;   
}   

nsrefcnt nsLayoutFactory::AddRef()   
{   
  return ++mRefCnt;   
}   

nsrefcnt nsLayoutFactory::Release()   
{   
  if (--mRefCnt == 0) {   
    delete this;   
    return 0; // Don't access mRefCnt after deleting!   
  }   
  return mRefCnt;   
}  

nsresult nsLayoutFactory::CreateInstance(nsISupports *aOuter,  
                                         const nsIID &aIID,  
                                         void **aResult)  
{  
  nsresult res;
  PRBool refCounted = PR_TRUE;

  if (aResult == NULL) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = NULL;  
  
  nsISupports *inst = nsnull;

  // XXX ClassID check happens here
  if (mClassID.Equals(kCHTMLDocumentCID)) {
    res = NS_NewHTMLDocument((nsIDocument **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCXMLDocumentCID)) {
    res = NS_NewXMLDocument((nsIDocument **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCImageDocumentCID)) {
    res = NS_NewImageDocument((nsIDocument **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCHTMLImageElementCID)) {
    res = NS_NewHTMLImageElement((nsIHTMLContent**)&inst, nsHTMLAtoms::img);
    if (NS_FAILED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCHTMLOptionElementCID)) {
    res = NS_NewHTMLOptionElement((nsIHTMLContent**)&inst, nsHTMLAtoms::option);
    if (NS_FAILED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kPresShellCID)) {
    res = NS_NewPresShell((nsIPresShell**) &inst);
    if (NS_FAILED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCRangeListCID)) {
    res = NS_NewRangeList((nsIDOMSelection**)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kCRangeCID)) {
    res = NS_NewRange((nsIDOMRange **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  } 
  else if (mClassID.Equals(kCContentIteratorCID)) {
    res = NS_NewContentIterator((nsIContentIterator **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  } 
  else if (mClassID.Equals(kCSubtreeIteratorCID)) {
    res = NS_NewContentSubtreeIterator((nsIContentIterator **)&inst);
    if (!NS_SUCCEEDED(res)) {
      return res;
    }
    refCounted = PR_TRUE;
  } 
  else if (mClassID.Equals(kCCSSParserCID)) {
    // XXX this should really be factored into a style-specific DLL so
    // that all the HTML, generic layout, and style stuff isn't munged
    // together.
    if (NS_FAILED(res = NS_NewCSSParser((nsICSSParser**)&inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kHTMLStyleSheetCID)) {
    // XXX ibid
    if (NS_FAILED(res = NS_NewHTMLStyleSheet((nsIHTMLStyleSheet**)&inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kHTMLCSSStyleSheetCID)) {
    // XXX ibid
    if (NS_FAILED(res = NS_NewHTMLCSSStyleSheet((nsIHTMLCSSStyleSheet**)&inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kTextNodeCID)) {
    // XXX ibid
    if (NS_FAILED(res = NS_NewTextNode((nsIContent**) &inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kNameSpaceManagerCID)) {
    if (NS_FAILED(res = NS_NewNameSpaceManager((nsINameSpaceManager**)&inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kFrameUtilCID)) {
    // XXX ibid
    if (NS_FAILED(res = NS_NewFrameUtil((nsIFrameUtil**) &inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else if (mClassID.Equals(kEventListenerManagerCID)) {
    if (NS_FAILED(res = NS_NewEventListenerManager((nsIEventListenerManager**) &inst)))
      return res;
    refCounted = PR_TRUE;
  }
  else 
  {
    return NS_NOINTERFACE;
  }
  if (inst == NULL) {  
    return NS_ERROR_OUT_OF_MEMORY;  
  }  

  res = inst->QueryInterface(aIID, aResult);

  if (refCounted) {
    NS_RELEASE(inst);
  }
  else if (res != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }  

  return res;  
}  

nsresult nsLayoutFactory::LockFactory(PRBool aLock)  
{  
  // Not implemented in simplest case.  
  return NS_OK;
}  

////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////


static NS_DEFINE_IID(kIScriptNameSetRegistryIID, NS_ISCRIPTNAMESETREGISTRY_IID);
static NS_DEFINE_IID(kCScriptNameSetRegistryCID, NS_SCRIPT_NAMESET_REGISTRY_CID);
static NS_DEFINE_IID(kIScriptNameSpaceManagerIID, NS_ISCRIPTNAMESPACEMANAGER_IID);
static NS_DEFINE_IID(kIScriptExternalNameSetIID, NS_ISCRIPTEXTERNALNAMESET_IID);

class LayoutScriptNameSet : public nsIScriptExternalNameSet {
public:
  LayoutScriptNameSet();
  ~LayoutScriptNameSet();

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD InitializeClasses(nsIScriptContext* aScriptContext);
  NS_IMETHOD AddNameSet(nsIScriptContext* aScriptContext);
};

LayoutScriptNameSet::LayoutScriptNameSet()
{
  NS_INIT_REFCNT();
}

LayoutScriptNameSet::~LayoutScriptNameSet()
{
}

NS_IMPL_ISUPPORTS(LayoutScriptNameSet, kIScriptExternalNameSetIID);

NS_IMETHODIMP 
LayoutScriptNameSet::InitializeClasses(nsIScriptContext* aScriptContext)
{
  return NS_OK;
}

NS_IMETHODIMP
LayoutScriptNameSet::AddNameSet(nsIScriptContext* aScriptContext)
{
  nsresult result = NS_OK;
  nsIScriptNameSpaceManager* manager;

  result = aScriptContext->GetNameSpaceManager(&manager);
  if (NS_OK == result) {
    result = manager->RegisterGlobalName("HTMLImageElement", 
                                         kCHTMLImageElementCID, 
                                         PR_TRUE);
    if (NS_FAILED(result)) {
      NS_RELEASE(manager);
      return result;
    }
      
    result = manager->RegisterGlobalName("HTMLOptionElement", 
                                         kCHTMLOptionElementCID, 
                                         PR_TRUE);
    if (NS_FAILED(result)) {
      NS_RELEASE(manager);
      return result;
    }
        
    NS_RELEASE(manager);
  }
  
  return result;
}

////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////

static nsIScriptNameSetRegistry *gRegistry;

// return the proper factory to the caller
#if defined(XP_MAC) && defined(MAC_STATIC)
extern "C" NS_LAYOUT nsresult 
NSGetFactory_LAYOUT_DLL(nsISupports* serviceMgr,
                        const nsCID &aClass,
                        const char *aClassName,
                        const char *aProgID,
                        nsIFactory **aFactory)
#else
extern "C" NS_LAYOUT nsresult 
NSGetFactory(nsISupports* serviceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
#endif
{
  if (nsnull == gRegistry) {
    nsresult result = nsServiceManager::GetService(kCScriptNameSetRegistryCID,
                                                   kIScriptNameSetRegistryIID,
                                                   (nsISupports **)&gRegistry);
    if (NS_OK == result) {
      LayoutScriptNameSet* nameSet = new LayoutScriptNameSet();
      gRegistry->AddExternalNameSet(nameSet);
    }
  }

  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsLayoutFactory(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}
