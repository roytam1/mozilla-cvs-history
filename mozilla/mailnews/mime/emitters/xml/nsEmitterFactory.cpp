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

#include "nsIFactory.h"
#include "nsISupports.h"
#include "msgCore.h"
#include "pratom.h"

/* Include all of the interfaces our factory can generate components for */
#include "nsIMimeEmitter.h"
#include "nsMimeEmitter.h"


static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kMimeEmitterCID, NS_XML_MIME_EMITTER_CID);

////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////
static PRInt32 g_InstanceCount = 0;
static PRInt32 g_LockCount = 0;

class nsXmlEmitterFactory : public nsIFactory
{   
public:
	// nsISupports methods
	NS_DECL_ISUPPORTS 

  nsXmlEmitterFactory(const nsCID &aClass,
               const char* aClassName,
               const char* aProgID,
               nsISupports*);

  // nsIFactory methods   
  NS_IMETHOD CreateInstance(nsISupports *aOuter, const nsIID &aIID, void **aResult);   
  NS_IMETHOD LockFactory(PRBool aLock);   

protected:
  virtual ~nsXmlEmitterFactory();   

  nsCID mClassID;
  char* mClassName;
  char* mProgID;
  nsIServiceManager* mServiceManager;
};   

nsXmlEmitterFactory::nsXmlEmitterFactory(const nsCID &aClass,
                           const char* aClassName,
                           const char* aProgID,
                           nsISupports *compMgrSupports)
  : mClassID(aClass),
    mClassName(nsCRT::strdup(aClassName)),
    mProgID(nsCRT::strdup(aProgID))
{
	NS_INIT_REFCNT();

  // store a copy of the 
  compMgrSupports->QueryInterface(nsIServiceManager::GetIID(),
                                  (void **)&mServiceManager);
}   

nsXmlEmitterFactory::~nsXmlEmitterFactory()   
{
	NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
  
  NS_IF_RELEASE(mServiceManager);
  PL_strfree(mClassName);
  PL_strfree(mProgID);
}   

nsresult
nsXmlEmitterFactory::QueryInterface(const nsIID &aIID, void **aResult)   
{   
  if (aResult == NULL)  
    return NS_ERROR_NULL_POINTER;  

  // Always NULL result, in case of failure   
  *aResult = NULL;   

  // we support two interfaces....nsISupports and nsFactory.....
  if (aIID.Equals(::nsISupports::GetIID()))    
    *aResult = (void *)(nsISupports*)this;   
  else if (aIID.Equals(nsIFactory::GetIID()))   
    *aResult = (void *)(nsIFactory*)this;   

  if (*aResult == NULL)
    return NS_NOINTERFACE;

  AddRef(); // Increase reference count for caller   
  return NS_OK;   
}   

NS_IMPL_ADDREF(nsXmlEmitterFactory)
NS_IMPL_RELEASE(nsXmlEmitterFactory)

nsresult
nsXmlEmitterFactory::CreateInstance(nsISupports *aOuter,
                             const nsIID &aIID,
                             void **aResult)  
{  
	nsresult res = NS_OK;

	if (aResult == NULL)  
		return NS_ERROR_NULL_POINTER;  

	*aResult = NULL;  
  
	nsISupports *inst = nsnull;

	// ClassID check happens here
	// Whenever you add a new class that supports an interface, plug it in here!!!
	
	// do they want a mime emitter interface ?
	if (mClassID.Equals(kMimeEmitterCID)) 
	{
		res = NS_NewMimeEmitter((nsIMimeEmitter **) &inst);
		if (NS_FAILED(res))  // was there a problem creating the object ?
		  return res;   
	}

	// End of checking the interface ID code....
	if (inst) 
	{
		// so we now have the class that supports the desired interface...we need to turn around and
		// query for our desired interface.....
		res = inst->QueryInterface(aIID, aResult);
		if (res != NS_OK)  // if the query interface failed for some reason, then the object did not get ref counted...delete it.
			delete inst; 
	}
	else
		res = NS_ERROR_OUT_OF_MEMORY;

  return res;  
}  

nsresult
nsXmlEmitterFactory::LockFactory(PRBool aLock)  
{  
	if (aLock)
		PR_AtomicIncrement(&g_LockCount); 
	else
		PR_AtomicDecrement(&g_LockCount);

	return NS_OK;
}  

////////////////////////////////////////////////////////////////////////////////

// return the proper factory to the caller. 
extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports* aServMgr,
                                           const nsCID &aClass,
                                           const char *aClassName,
                                           const char *aProgID,
                                           nsIFactory **aFactory)
{
	if (nsnull == aFactory)
		return NS_ERROR_NULL_POINTER;

  *aFactory = new nsXmlEmitterFactory(aClass, aClassName, aProgID, aServMgr);
  if (aFactory)
    return (*aFactory)->QueryInterface(nsIFactory::GetIID(),
                                       (void**)aFactory);
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

extern "C" NS_EXPORT PRBool NSCanUnload(nsISupports* aServMgr) 
{
	return PRBool(g_InstanceCount == 0 && g_LockCount == 0);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" NS_EXPORT nsresult
NSRegisterSelf(nsISupports* aServMgr, const char* path)
{
  nsresult rv = NS_OK;

  NS_WITH_SERVICE(nsIComponentManager, compMgr, kComponentManagerCID, &rv); 
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kMimeEmitterCID,
                                       "RFC822 Parser",
                                       nsnull,
                                       path, PR_TRUE, PR_TRUE);
  return rv;
}

extern "C" NS_EXPORT nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char* path)
{
  nsresult rv = NS_OK;
  NS_WITH_SERVICE(nsIComponentManager, compMgr, kComponentManagerCID, &rv); 
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->UnregisterComponent(kMimeEmitterCID, path);
  return rv;
}

////////////////////////////////////////////////////////////////////////////////

