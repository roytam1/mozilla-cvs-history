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
#include "nsAbBaseCID.h"
#include "pratom.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "rdf.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"


/* Include all of the interfaces our factory can generate components for */

#include "nsDirectoryDataSource.h"
#include "nsCardDataSource.h"
#include "nsAbDirectory.h"
#include "nsAbCard.h"
#include "nsAddrDatabase.h"

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

static NS_DEFINE_CID(kAbDirectoryDataSourceCID, NS_ABDIRECTORYDATASOURCE_CID);
static NS_DEFINE_CID(kAbDirectoryCID, NS_ABDIRECTORYRESOURCE_CID); 
static NS_DEFINE_CID(kAbCardDataSourceCID, NS_ABCARDDATASOURCE_CID);
static NS_DEFINE_CID(kAbCardCID, NS_ABCARDRESOURCE_CID); 
static NS_DEFINE_CID(kAddressBookDB, NS_ADDRESSBOOKDB_CID);

////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////
static PRInt32 g_InstanceCount = 0;
static PRInt32 g_LockCount = 0;

class nsAbFactory : public nsIFactory
{   
public:
	// nsISupports methods
	NS_DECL_ISUPPORTS 

	nsAbFactory(const nsCID &aClass, const char* aClassName, const char* aProgID); 

	// nsIFactory methods   
	NS_IMETHOD CreateInstance(nsISupports *aOuter, const nsIID &aIID, void **aResult);   
	NS_IMETHOD LockFactory(PRBool aLock);   

protected:
	virtual ~nsAbFactory();   

	nsCID mClassID;
	char* mClassName;
	char* mProgID;

//	nsIServiceManager* mServiceManager;
};   

nsAbFactory::nsAbFactory(const nsCID &aClass, const char* aClassName, const char* aProgID)
  : mClassID(aClass), mClassName(nsCRT::strdup(aClassName)), mProgID(nsCRT::strdup(aProgID))
{   
	NS_INIT_REFCNT();
	// store a copy of the 
//	compMgrSupports->QueryInterface(nsIServiceManager::GetIID(),
//                                  (void **)&mServiceManager);
}   

nsAbFactory::~nsAbFactory()   
{
	NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");   
//	NS_IF_RELEASE(mServiceManager);
	PL_strfree(mClassName);
	PL_strfree(mProgID);
}   

nsresult nsAbFactory::QueryInterface(const nsIID &aIID, void **aResult)   
{   
	if (aResult == NULL) 
		return NS_ERROR_NULL_POINTER;  

	// Always NULL result, in case of failure   
	*aResult = NULL;   

	// we support two interfaces....nsISupports and nsFactory.....
	if (aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID()))    
		*aResult = (void *)(nsISupports*)this;   
	else if (aIID.Equals(nsCOMTypeInfo<nsIFactory>::GetIID()))   
		*aResult = (void *)(nsIFactory*)this;   

	if (*aResult == NULL)
		return NS_NOINTERFACE;

	AddRef(); // Increase reference count for caller   
	return NS_OK;   
}   


NS_IMPL_ADDREF(nsAbFactory)
NS_IMPL_RELEASE(nsAbFactory)

nsresult nsAbFactory::CreateInstance(nsISupports *aOuter, const nsIID &aIID, void **aResult)  
{  
	if (aResult == NULL)  
		return NS_ERROR_NULL_POINTER;  

	*aResult = NULL;  
    
	// ClassID check happens here
	// Whenever you add a new class that supports an interface, plug it in here!!!
	
	if (mClassID.Equals(kAbDirectoryDataSourceCID)) 
	{
		nsresult rv;
		nsABDirectoryDataSource * directoryDataSource = new nsABDirectoryDataSource();
		if (directoryDataSource)
			rv = directoryDataSource->QueryInterface(aIID, aResult);
		else
			rv = NS_ERROR_OUT_OF_MEMORY;

		if (NS_FAILED(rv) && directoryDataSource)
			delete directoryDataSource;
		return rv;
	}
	else if (mClassID.Equals(kAbDirectoryCID)) 
	{
		nsresult rv;
		nsABDirectory * directory = new nsABDirectory();
		if (directory)
			rv = directory->QueryInterface(aIID, aResult);
		else
			rv = NS_ERROR_OUT_OF_MEMORY;

		if (NS_FAILED(rv) && directory)
			delete directory;
		return rv;
	}
	if (mClassID.Equals(kAbCardDataSourceCID)) 
	{
		nsresult rv;
		nsABCardDataSource * cardDataSource = new nsABCardDataSource();
		if (cardDataSource)
			rv = cardDataSource->QueryInterface(aIID, aResult);
		else
			rv = NS_ERROR_OUT_OF_MEMORY;

		if (NS_FAILED(rv) && cardDataSource)
			delete cardDataSource;
		return rv;
	}
	else if (mClassID.Equals(kAbCardCID)) 
	{
		nsresult rv;
		nsABCard * card = new nsABCard();
		if (card)
			rv = card->QueryInterface(aIID, aResult);
		else
			rv = NS_ERROR_OUT_OF_MEMORY;

		if (NS_FAILED(rv) && card)
			delete card;
		return rv;
	}
	else if (mClassID.Equals(kAddressBookDB)) 
	{
		nsresult rv;
		nsAddrDatabase * abDatabase = new nsAddrDatabase();
		if (abDatabase)
			rv = abDatabase->QueryInterface(aIID, aResult);
		else
			rv = NS_ERROR_OUT_OF_MEMORY;

		if (NS_FAILED(rv) && abDatabase)
			delete abDatabase;
		return rv;
	}
	return NS_NOINTERFACE;  
}  

nsresult nsAbFactory::LockFactory(PRBool aLock)  
{  
	if (aLock)
		PR_AtomicIncrement(&g_LockCount); 
	else
		PR_AtomicDecrement(&g_LockCount); 

  return NS_OK;
}  

// return the proper factory to the caller. 
extern "C" NS_EXPORT nsresult NSGetFactory(nsISupports* aServMgr,
                                           const nsCID &aClass,
                                           const char *aClassName,
                                           const char *aProgID,
                                           nsIFactory **aFactory)
{
	if (nsnull == aFactory)
		return NS_ERROR_NULL_POINTER;

	*aFactory = new nsAbFactory(aClass, aClassName, aProgID);

	if (aFactory)
		return (*aFactory)->QueryInterface(nsCOMTypeInfo<nsIFactory>::GetIID(), (void**)aFactory); // they want a Factory Interface so give it to them
	else
		return NS_ERROR_OUT_OF_MEMORY;
}

extern "C" NS_EXPORT PRBool NSCanUnload(nsISupports* aServMgr) 
{
    return PRBool(g_InstanceCount == 0 && g_LockCount == 0);
}

extern "C" NS_EXPORT nsresult
NSRegisterSelf(nsISupports* aServMgr, const char* path)
{
	nsresult rv = NS_OK;
	nsresult finalResult = NS_OK;

	nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	// register our RDF datasources:
	rv = compMgr->RegisterComponent(kAbDirectoryDataSourceCID, 
							  "Mail/News Address Book Directory Data Source",
							  NS_RDF_DATASOURCE_PROGID_PREFIX "addressdirectory",
							  path, PR_TRUE, PR_TRUE);

	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kAbDirectoryCID,
								  "Mail/News Address Book Directory Factory",
								  NS_RDF_RESOURCE_FACTORY_PROGID_PREFIX "abdirectory",
								  path, PR_TRUE, PR_TRUE);

	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kAbCardDataSourceCID, 
							  "Mail/News Address Book Card Data Source",
							  NS_RDF_DATASOURCE_PROGID_PREFIX "addresscard",
							  path, PR_TRUE, PR_TRUE);

	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kAbCardCID,
								  "Mail/News Address Book Card Factory",
								  NS_RDF_RESOURCE_FACTORY_PROGID_PREFIX "abcard",
								  path, PR_TRUE, PR_TRUE);

	if (NS_FAILED(rv)) finalResult = rv;
	
	rv = compMgr->RegisterComponent(kAddressBookDB, nsnull, nsnull,
								  path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv))finalResult = rv;

	return finalResult;
}

extern "C" NS_EXPORT nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char* path)
{
	nsresult rv = NS_OK;
	nsresult finalResult = NS_OK;

	nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	rv = compMgr->UnregisterComponent(kAbDirectoryDataSourceCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kAbDirectoryCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kAbCardDataSourceCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kAbCardCID, path);
	if (NS_FAILED(rv)) finalResult = rv;
	
	rv = compMgr->UnregisterComponent(kAddressBookDB, path);
	if (NS_FAILED(rv)) finalResult = rv;

	return finalResult;
}
