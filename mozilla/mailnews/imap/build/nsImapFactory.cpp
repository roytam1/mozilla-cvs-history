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

#include "msgCore.h" // for pre-compiled headers...
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsIMAPHostSessionList.h"
#include "nsImapIncomingServer.h"
#include "nsImapService.h"
#include "nsImapMailFolder.h"
#include "nsImapMessage.h"
#include "nsImapUrl.h"
#include "nsImapProtocol.h"
#include "nsMsgImapCID.h"

static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kCImapUrl, NS_IMAPURL_CID);
static NS_DEFINE_CID(kCImapProtocol, NS_IMAPPROTOCOL_CID);
static NS_DEFINE_CID(kCImapHostSessionList, NS_IIMAPHOSTSESSIONLIST_CID);
static NS_DEFINE_CID(kCImapIncomingServer, NS_IMAPINCOMINGSERVER_CID);
static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);
static NS_DEFINE_CID(kCImapResource, NS_IMAPRESOURCE_CID);
static NS_DEFINE_CID(kCImapMessageResource, NS_IMAPMESSAGERESOURCE_CID);
static NS_DEFINE_CID(kCImapMockChannel, NS_IMAPMOCKCHANNEL_CID);

// private factory declarations for each component we know how to produce
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsImapUrl, Initialize)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapProtocol)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIMAPHostSessionList)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapIncomingServer)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapMailFolder)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapMockChannel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImapMessage)


// Module implementation for the sample library
class nsMsgImapModule : public nsIModule
{
public:
    nsMsgImapModule();
    virtual ~nsMsgImapModule();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIMODULE

protected:
    nsresult Initialize();

    void Shutdown();

    PRBool mInitialized;
    nsCOMPtr<nsIGenericFactory> mImapUrlFactory;
    nsCOMPtr<nsIGenericFactory> mImapProtocolFactory;
    nsCOMPtr<nsIGenericFactory> mImapHostSessionFactory;
    nsCOMPtr<nsIGenericFactory> mImapIncomingServerFactory;
    nsCOMPtr<nsIGenericFactory> mImapServiceFactory;
    nsCOMPtr<nsIGenericFactory> mImapMailFolderFactory;
    nsCOMPtr<nsIGenericFactory> mImapMockChannelFactory;
    nsCOMPtr<nsIGenericFactory> mImapMessageFactory;
};


nsMsgImapModule::nsMsgImapModule()
    : mInitialized(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
}

nsMsgImapModule::~nsMsgImapModule()
{
    Shutdown();
}

NS_IMPL_ISUPPORTS(nsMsgImapModule, NS_GET_IID(nsIModule))

// Perform our one-time intialization for this module
nsresult nsMsgImapModule::Initialize()
{
    if (mInitialized)
        return NS_OK;

    mInitialized = PR_TRUE;
    return NS_OK;
}

// Shutdown this module, releasing all of the module resources
void nsMsgImapModule::Shutdown()
{
    // Release the factory object
    mImapUrlFactory = null_nsCOMPtr();
    mImapProtocolFactory = null_nsCOMPtr();
    mImapHostSessionFactory = null_nsCOMPtr();
    mImapIncomingServerFactory = null_nsCOMPtr();
    mImapServiceFactory = null_nsCOMPtr();
    mImapMailFolderFactory = null_nsCOMPtr();
    mImapMockChannelFactory = null_nsCOMPtr();
    mImapMessageFactory = null_nsCOMPtr();
}

// Create a factory object for creating instances of aClass.
NS_IMETHODIMP nsMsgImapModule::GetClassObject(nsIComponentManager *aCompMgr,
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

    if (aClass.Equals(kCImapUrl))
    {
        if (!mImapUrlFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapUrlFactory), &nsImapUrlConstructor);
        fact = mImapUrlFactory;
    }
    else if (aClass.Equals(kCImapProtocol))
    {
        if (!mImapProtocolFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapProtocolFactory), &nsImapProtocolConstructor);
        fact = mImapProtocolFactory;
    }
    else if (aClass.Equals(kCImapHostSessionList))
    {
        if (!mImapHostSessionFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapHostSessionFactory), &nsIMAPHostSessionListConstructor);
        fact = mImapHostSessionFactory;
    }
    else if (aClass.Equals(kCImapIncomingServer))
    {
        if (!mImapIncomingServerFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapIncomingServerFactory), &nsImapIncomingServerConstructor);
        fact = mImapIncomingServerFactory;
    }
    else if (aClass.Equals(kCImapService))
    {
        if (!mImapServiceFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapServiceFactory), &nsImapServiceConstructor);
        fact = mImapServiceFactory;
    }
    else if (aClass.Equals(kCImapResource))
    {
        if (!mImapMailFolderFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapMailFolderFactory), &nsImapMailFolderConstructor);
        fact = mImapMailFolderFactory;
    }
    else if (aClass.Equals(kCImapMockChannel))
    {
        if (!mImapMockChannelFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapMockChannelFactory), &nsImapMockChannelConstructor);
        fact = mImapMockChannelFactory;
    }
    else if (aClass.Equals(kCImapMessageResource)) 
    {
        if (!mImapMessageFactory)
            rv = NS_NewGenericFactory(getter_AddRefs(mImapMessageFactory), &nsImapMessageConstructor);
        fact = mImapMessageFactory;
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
    { "Imap Url", &kCImapUrl,
      nsnull },
    { "Imap Protocol Channel", &kCImapProtocol,
      nsnull },    
    { "Imap Mock Channel", &kCImapMockChannel,
      nsnull },
    { "Imap Host Session List", &kCImapHostSessionList,
      nsnull },
    { "Imap Incoming Server", &kCImapIncomingServer,
      "component://netscape/messenger/server&type=imap" },
    { "Mail/News Imap Resource Factory", &kCImapResource,
      NS_RDF_RESOURCE_FACTORY_PROGID_PREFIX "imap" },
    { "Imap Service", &kCImapService,
      "component://netscape/messenger/messageservice;type=imap_message" },
    { "Imap Service", &kCImapService,
      "component://netscape/messenger/messageservice;type=imap"},
    { "Imap Protocol Handler", &kCImapService,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "imap"},
    { "Imap Protocol Handler", &kCImapService,
      NS_IMAPPROTOCOLINFO_PROGID},
    { "Imap Message Resource Factory", &kCImapMessageResource,
      NS_RDF_RESOURCE_FACTORY_PROGID_PREFIX "imap_message"}

};
#define NUM_COMPONENTS (sizeof(gComponents) / sizeof(gComponents[0]))

NS_IMETHODIMP nsMsgImapModule::RegisterSelf(nsIComponentManager *aCompMgr,
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

NS_IMETHODIMP nsMsgImapModule::UnregisterSelf(nsIComponentManager* aCompMgr,
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

NS_IMETHODIMP nsMsgImapModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
    if (!okToUnload)
        return NS_ERROR_INVALID_POINTER;

    *okToUnload = PR_FALSE;
    return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------

static nsMsgImapModule *gModule = NULL;

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,
                                          nsIFileSpec* location,
                                          nsIModule** return_cobj)
{
    nsresult rv = NS_OK;

    NS_ASSERTION(return_cobj, "Null argument");
    NS_ASSERTION(gModule == NULL, "nsMsgImapModule: Module already created.");

    // Create an initialize the imap module instance
    nsMsgImapModule *module = new nsMsgImapModule();
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
