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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsMorkCID.h"
#include "nsIMdbFactoryFactory.h"
#include "mdb.h"

class nsMorkFactoryFactory : public nsIMdbFactoryFactory
{
public:
	nsMorkFactoryFactory();
	// nsISupports methods
	NS_DECL_ISUPPORTS 

	NS_IMETHOD GetMdbFactory(nsIMdbFactory **aFactory);

};


// include files for components this factory creates...
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kCMorkFactory, NS_MORK_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMorkFactoryFactory)

////////////////////////////////////////////////////////////
//
class nsMorkModule : public nsIModule
{   
public:

	nsMorkModule(); 
	virtual ~nsMorkModule();

	// nsISupports methods
	NS_DECL_ISUPPORTS 

    NS_DECL_NSIMODULE

protected:
    nsresult Initialize();

    void Shutdown();

    PRBool mInitialized;
	nsCOMPtr<nsIGenericFactory> mMorkFactory;
};   

nsMorkModule::nsMorkModule()
    : mInitialized(PR_FALSE)
{   
	NS_INIT_REFCNT();
}   


NS_IMPL_ISUPPORTS(nsMorkModule, NS_GET_IID(nsIModule))

nsMorkModule::~nsMorkModule()
{
    Shutdown();
}

// Perform our one-time intialization for this module
nsresult nsMorkModule::Initialize()
{
    if (mInitialized)
        return NS_OK;

    mInitialized = PR_TRUE;
    return NS_OK;
}

// Shutdown this module, releasing all of the module resources
void nsMorkModule::Shutdown()
{
    // Release the factory objects
    mMorkFactory = null_nsCOMPtr();
}

// Create a factory object for creating instances of aClass.
NS_IMETHODIMP nsMorkModule::GetClassObject(nsIComponentManager *aCompMgr,
                               const nsCID& aClass,
                               const nsIID& aIID,
                               void** r_classObj)
{
    nsresult rv;

    // Defensive programming: Initialize *r_classObj in case of error below
    if (!r_classObj)
        return NS_ERROR_INVALID_POINTER;

    *r_classObj = NULL;

    // Do one-time-only initialization if necessary
    if (!mInitialized) 
    {
        rv = Initialize();
        if (NS_FAILED(rv)) // Initialization failed! yikes!
            return rv;
    }

    // Choose the appropriate factory, based on the desired instance
    // class type (aClass).
    nsCOMPtr<nsIGenericFactory> fact;

    if (aClass.Equals(kCMorkFactory))
    {
        if (!mMorkFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mMorkFactory), &nsMorkFactoryFactoryConstructor);
        fact = mMorkFactory;
    }
    if (fact)
        rv = fact->QueryInterface(aIID, r_classObj);

    return rv;
}


struct Components {
    const char* mDescription;
    const nsID* mCID;
    const char* mProgID;
};

// The list of components we register
static Components gComponents[] = {
    { "Mork Factory", &kCMorkFactory,
      nsnull },
};
#define NUM_COMPONENTS (sizeof(gComponents) / sizeof(gComponents[0]))

NS_IMETHODIMP nsMorkModule::RegisterSelf(nsIComponentManager *aCompMgr,
                          nsIFileSpec* aPath,
                          const char* registryLocation,
                          const char* componentType)
{
    nsresult rv = NS_OK;

    Components* cp = gComponents;
    Components* end = cp + NUM_COMPONENTS;
    while (cp < end) 
    {
        rv = aCompMgr->RegisterComponentSpec(*cp->mCID, cp->mDescription,
                                             cp->mProgID, aPath, PR_TRUE,
                                             PR_TRUE);
        if (NS_FAILED(rv)) 
            break;
        cp++;
    }

    return rv;
}

NS_IMETHODIMP nsMorkModule::UnregisterSelf(nsIComponentManager* aCompMgr,
                            nsIFileSpec* aPath,
                            const char* registryLocation)
{
    Components* cp = gComponents;
    Components* end = cp + NUM_COMPONENTS;
    while (cp < end) 
    {
        aCompMgr->UnregisterComponentSpec(*cp->mCID, aPath);
        cp++;
    }

    return NS_OK;
}

NS_IMETHODIMP nsMorkModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
    if (!okToUnload)
        return NS_ERROR_INVALID_POINTER;

    *okToUnload = PR_FALSE;
    return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
static nsMorkModule *gModule = NULL;

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,
                                          nsIFileSpec* location,
                                          nsIModule** return_cobj)
{
    nsresult rv = NS_OK;

    NS_ASSERTION(return_cobj, "Null argument");
    NS_ASSERTION(gModule == NULL, "nsMorkModule: Module already created.");

    // Create an initialize the imap module instance
    nsMorkModule *module = new nsMorkModule();
    if (!module)
        return NS_ERROR_OUT_OF_MEMORY;

    // Increase refcnt and store away nsIModule interface to m in return_cobj
    rv = module->QueryInterface(nsIModule::GetIID(), (void**)return_cobj);
    if (NS_FAILED(rv)) 
    {
        delete module;
        module = nsnull;
    }
    gModule = module;                  // WARNING: Weak Reference
    return rv;
}

static nsIMdbFactory *gMDBFactory = nsnull;

NS_IMPL_ADDREF(nsMorkFactoryFactory)
NS_IMPL_RELEASE(nsMorkFactoryFactory)

NS_IMETHODIMP
nsMorkFactoryFactory::QueryInterface(REFNSIID iid, void** result)
{
	if (! result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
    if(iid.Equals(nsIMdbFactoryFactory::GetIID()) ||
		iid.Equals(nsCOMTypeInfo<nsISupports>::GetIID())) {
		*result = NS_STATIC_CAST(nsIMdbFactoryFactory*, this);
		AddRef();
		return NS_OK;
	}
    return NS_NOINTERFACE;
}



nsMorkFactoryFactory::nsMorkFactoryFactory()
{
	NS_INIT_REFCNT();
}

NS_IMETHODIMP nsMorkFactoryFactory::GetMdbFactory(nsIMdbFactory **aFactory)
{
	if (!gMDBFactory)
		gMDBFactory = MakeMdbFactory();
	*aFactory = gMDBFactory;
	return (gMDBFactory) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

