/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

#include "nsComponentManager.h"
#include "nsIServiceManager.h"
#include <stdlib.h>

extern "C" NS_EXPORT nsresult
NS_RegistryGetFactory(nsISupports* servMgr, nsIFactory** aFactory);

#include "plstr.h"
#include "prlink.h"
#include "prsystem.h"
#include "prprf.h"
#include "xcDll.h"
#include "prlog.h"
#include "prerror.h"
#include "prmem.h"

#include "prcmon.h"
#include "prthread.h" /* XXX: only used for the NSPR initialization hack (rick) */

#ifdef NS_DEBUG
PRLogModuleInfo* nsComponentManagerLog = NULL;
#endif

// Common Key Names 
const char xpcomBaseName[]="XPCOM";
const char xpcomKeyName[] ="Software/Netscape/XPCOM";
const char netscapeKeyName[]="Software/Netscape";
const char classesKeyName[]="Classes";
const char classIDKeyName[]="CLSID";
const char classesClassIDKeyName[]="Classes/CLSID";


// Common Value Names
const char classIDValueName[]="CLSID";
const char versionValueName[]="VersionString";
const char lastModValueName[]="LastModTimeStamp";
const char fileSizeValueName[]="FileSize";
const char componentCountValueName[]="ComponentsCount";
const char progIDValueName[]="ProgID";
const char classNameValueName[]="ClassName";
const char inprocServerValueName[]="InprocServer";

// We define a CID that is used to indicate the non-existence of a
// progid in the hash table.
#define NS_NO_CID { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }
    static NS_DEFINE_CID(kNoCID, NS_NO_CID);



////////////////////////////////////////////////////////////////////////////////
// nsFactoryEntry
////////////////////////////////////////////////////////////////////////////////

nsFactoryEntry::nsFactoryEntry()
    : factory(NULL), dll(NULL)
{
}

nsFactoryEntry::nsFactoryEntry(const nsCID &aClass, nsIFactory *aFactory)
    : cid(aClass), factory(aFactory), dll(NULL)

{
    NS_ADDREF(aFactory);
}

nsFactoryEntry::~nsFactoryEntry(void)
{
    if (factory != NULL) {
        NS_RELEASE(factory);
    }
    // DO NOT DELETE nsDll *dll;
}

nsresult
nsFactoryEntry::Init(nsHashtable* dllCollection, 
                     const nsCID &aClass, const char *aLibrary,
                     PRTime lastModTime, PRUint32 fileSize)
{
    if (aLibrary == NULL) {
        return NS_OK;
    }
    cid = aClass;
      
    nsCStringKey key(aLibrary);

    // If dll not already in dllCollection, add it.
    dll = (nsDll *) dllCollection->Get(&key);
    // PR_ExitMonitor(mMon);
    	
    if (dll == NULL) {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: New dll \"%s\".", aLibrary));
#endif /* NS_DEBUG */

        // Add a new Dll into the nsDllStore
        dll = new nsDll(aLibrary, lastModTime, fileSize);
        if (dll == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
        if (dll->GetStatus() != DLL_OK) {
            // Cant create a nsDll. Backoff.
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: New dll status error. \"%s\".", aLibrary));
#endif /* NS_DEBUG */
            delete dll;
            dll = NULL;
        }
        else {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Adding New dll \"%s\" to mDllStore.",
                    aLibrary));
#endif /* NS_DEBUG */

            dllCollection->Put(&key, (void *)dll);
        }
    }
    else {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Found in mDllStore \"%s\".", aLibrary));
#endif /* NS_DEBUG */
        // XXX We found the dll in the dllCollection.
        // XXX Consistency check: dll needs to have the same
        // XXX lastModTime and fileSize. If not that would mean
        // XXX that the dll wasn't registered properly.
    }
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////////////
// autoFree
////////////////////////////////////////////////////////////////////////////////

//
// To prevent string leaks, we are using these classes. Typical use would be
// for each ptr to be deleted, create an object of these types with that ptr.
// Once that object goes out of scope, deletion and hence memory free will
// automatically happen.
//
class autoFree
{
private:
    void *mPtr;
public:
    autoFree(void *Ptr=NULL): mPtr(Ptr) {}
    ~autoFree() { PR_FREEIF(mPtr); }
};


////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl
////////////////////////////////////////////////////////////////////////////////


nsComponentManagerImpl::nsComponentManagerImpl()
    : mFactories(NULL), mProgIDs(NULL), mMon(NULL), mDllStore(NULL), mRegistry(NULL)
{
    NS_INIT_REFCNT();
}

nsresult nsComponentManagerImpl::Init(void) 
{
    if (mFactories == NULL) {
        mFactories = new nsHashtable(256, /* Thread Safe */ PR_TRUE);
        if (mFactories == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mProgIDs == NULL) {
        mProgIDs = new nsHashtable(256, /* Thread Safe */ PR_TRUE);
        if (mProgIDs == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mMon == NULL) {
        mMon = PR_NewMonitor();
        if (mMon == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mDllStore == NULL) {
        mDllStore = new nsHashtable(256, /* Thead Safe */ PR_TRUE);
        if (mDllStore == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    if(mRegistry==NULL) {		
        nsIFactory *registryFactory = NULL;
        nsresult rv;

        nsIServiceManager *srvMgr;

        rv = nsServiceManager::GetGlobalServiceManager(&srvMgr);
        if(NS_FAILED(rv))
        {
            return rv;
        }
        rv = NS_RegistryGetFactory(srvMgr, &registryFactory);
        if (NS_SUCCEEDED(rv))
        {
            NS_DEFINE_IID(kRegistryIID, NS_IREGISTRY_IID);
            rv = registryFactory->CreateInstance(NULL, kRegistryIID,(void **)&mRegistry);
            if (NS_FAILED(rv))
            {
                return rv;
            }

            NS_RELEASE(registryFactory);
        }
    }
    

#ifdef NS_DEBUG
    if (nsComponentManagerLog == NULL) {
        nsComponentManagerLog = PR_NewLogModule("nsComponentManager");
    }
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
           ("nsComponentManager: Initialized."));
#endif

#ifdef USE_NSREG

#ifdef XP_UNIX
    // Create ~/.mozilla as that is the default place for the registry file

    /* The default registry on the unix system is $HOME/.mozilla/registry per
     * vr_findGlobalRegName(). vr_findRegFile() will create the registry file
     * if it doesn't exist. But it wont create directories.
     *
     * Hence we need to create the directory if it doesn't exist already.
     *
     * Why create it here as opposed to the app ?
     * ------------------------------------------
     * The app cannot create the directory in main() as most of the registry
     * and initialization happens due to use of static variables.
     * And we dont want to be dependent on the order in which
     * these static stuff happen.
     *
     * Permission for the $HOME/.mozilla will be Read,Write,Execute
     * for user only. Nothing to group and others.
     */
    char *home = getenv("HOME");
    if (home != NULL)
    {
        char dotMozillaDir[1024];
        PR_snprintf(dotMozillaDir, sizeof(dotMozillaDir),
                    "%s/" NS_MOZILLA_DIR_NAME, home);
        if (PR_Access(dotMozillaDir, PR_ACCESS_EXISTS) != PR_SUCCESS)
        {
            PR_MkDir(dotMozillaDir, NS_MOZILLA_DIR_PERMISSION);
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Creating Directory %s", dotMozillaDir));
#endif /* NS_DEBUG */
        }
    }
#endif /* XP_UNIX */
    
    // No error checking on the following because the program
    // can work perfectly well even if no registry is available.

    // Open the default registry. We will keep it open forever!
    mRegistry->Open();

    // Check the version of registry. Nuke old versions.
    PlatformVersionCheck();

    // Open common registry keys here to speed access
    // Do this after PlatformVersionCheck as it may re-create our keys
    nsresult rv;
    rv = mRegistry->AddSubtree(nsIRegistry::Common, xpcomKeyName, &mXPCOMKey);
    		
    if (NS_FAILED(rv))
    {        
        return rv;
    }

    rv = mRegistry->AddSubtree(nsIRegistry::Common, classesKeyName, &mClassesKey);
    if (NS_FAILED(rv))
    {        
        return rv;
    }

    rv = mRegistry->AddSubtree(nsIRegistry::Common, classIDKeyName, &mCLSIDKey);
    if (NS_FAILED(rv))
    {
        return rv;
    }

#endif

    return NS_OK;
}

nsComponentManagerImpl::~nsComponentManagerImpl()
{
    if (mFactories)
        delete mFactories;
    if (mProgIDs)
        delete mProgIDs;
    if (mMon)
        PR_DestroyMonitor(mMon);
    if (mDllStore)
        delete mDllStore;
    if(mRegistry)
        NS_RELEASE(mRegistry);
}

NS_IMPL_ISUPPORTS(nsComponentManagerImpl, nsIComponentManager::GetIID());

////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl: Platform methods
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_NSREG
#define USE_REGISTRY

/**
 * PlatformVersionCheck()
 *
 * Checks to see if the XPCOM hierarchy in the registry is the same as that of
 * the software as defined by NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING
 */
nsresult
nsComponentManagerImpl::PlatformVersionCheck()
{

	nsIRegistry::Key xpcomKey;
	nsresult rv;
	rv = mRegistry->AddSubtree(nsIRegistry::Common, xpcomKeyName, &xpcomKey);

    		
    if (NS_FAILED(rv))
    {        
        return rv;
    }

	char *buf;
    nsresult err = mRegistry->GetString(xpcomKey, versionValueName, &buf);
    autoFree bufAutoFree(buf);

    // If there is a version mismatch or no version string, we got an old registry.
    // Delete the old repository hierarchies and recreate version string
    if (NS_FAILED(err) || PL_strcmp(buf, NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING))
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Registry version mismatch (%s vs %s). Nuking xpcom "
                "registry hierarchy.", buf, NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING));
#endif /* NS_DEBUG */

        // Delete the XPCOM and CLSID hierarchy
        nsIRegistry::Key netscapeKey;
        rv = mRegistry->GetSubtree(nsIRegistry::Common,netscapeKeyName, &netscapeKey);
        if(NS_FAILED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Get Subtree (%s)",netscapeKeyName));         
#endif /* NS_DEBUG */
        }
        else
        {
            rv = mRegistry->RemoveSubtreeRaw(netscapeKey, xpcomBaseName);
            if(NS_FAILED(rv))
            {
#ifdef NS_DEBUG
                PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                       ("nsComponentManager: Failed To Nuke Subtree (%s)",xpcomKeyName));
#endif /* NS_DEBUG */
                return rv;
            }
        }

        rv = mRegistry->GetSubtree(nsIRegistry::Common,classesKeyName, &netscapeKey);
        if(NS_FAILED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Get Subtree (%s)",classesKeyName));
#endif /* NS_DEBUG */
        }
        else
        {
            rv = mRegistry->RemoveSubtreeRaw(netscapeKey, classIDKeyName);
            if(NS_FAILED(rv))
            {
#ifdef NS_DEBUG
                PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                       ("nsComponentManager: Failed To Nuke Subtree (%s/%s)",classesKeyName,classIDKeyName));
#endif /* NS_DEBUG */
                return rv;
            }
        }
        
        // Recreate XPCOM and CLSID keys		
        rv = mRegistry->AddSubtree(nsIRegistry::Common,xpcomKeyName, &xpcomKey);
        if(NS_FAILED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Add Subtree (%s)",xpcomKeyName));
#endif /* NS_DEBUG */
            return rv;

        }

        rv = mRegistry->AddSubtree(nsIRegistry::Common,classesClassIDKeyName, NULL);
        if(NS_FAILED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Add Subtree (%s)",classesClassIDKeyName));
#endif /* NS_DEBUG */
            return rv;

        }

        rv = mRegistry->SetString(xpcomKey,versionValueName, NS_XPCOM_COMPONENT_MANAGER_VERSION_STRING);
        if(NS_FAILED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Failed To Set String (Version) Under (%s)",xpcomKeyName));
#endif /* NS_DEBUG */
            return rv;
        }

    }
    else 
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: platformVersionCheck() passed."));
#endif /* NS_DEBUG */
    }

    return NS_OK;
}


void
nsComponentManagerImpl::PlatformGetFileInfo(nsIRegistry::Key key,PRTime *lastModifiedTime,PRUint32 *fileSize)
{
    PR_ASSERT(lastModifiedTime);
    PR_ASSERT(fileSize);

    nsresult rv;
    PRTime *lastModPtr;
    uint32 n = sizeof(PRTime);
    rv = mRegistry->GetBytes(key, lastModValueName, (void **)&lastModPtr, &n);
    if(NS_SUCCEEDED(rv))
    {
        *lastModifiedTime = *lastModPtr;
        PR_Free(lastModPtr);
    }
    	
    int32 fsize = 0;
    rv = mRegistry->GetInt(key, fileSizeValueName, &fsize);
    if (NS_SUCCEEDED(rv))
    {
        *fileSize = fsize;
    }
}


/**
 * PlatformCreateDll(const char *fullname)
 *
 * Creates a nsDll from the registry representation of dll 'fullname'.
 * Looks under
 *		ROOTKEY_COMMON/Software/Netscape/XPCOM/fullname
 */
nsresult
nsComponentManagerImpl::PlatformCreateDll(const char *fullname, nsDll* *result)
{
    PR_ASSERT(mRegistry!=NULL);

    nsresult rv;

    nsIRegistry::Key fullnameKey;

    rv = mRegistry->GetSubtreeRaw(mXPCOMKey,fullname, &fullnameKey);
    if(NS_FAILED(rv))
    {        
        return rv;
    }
    	
    PRTime lastModTime=LL_ZERO;
    PRUint32 fileSize=0;

    PlatformGetFileInfo(fullnameKey,&lastModTime,&fileSize);

    	
    nsDll *dll = new nsDll(fullname, lastModTime, fileSize);
    if (dll == NULL)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    *result = dll;
    return NS_OK;
}

/**
 * PlatformMarkNoComponents(nsDll *dll)
 *
 * Stores the dll name, last modified time, size and 0 for number of
 * components in dll in the registry at location
 *		ROOTKEY_COMMON/Software/Netscape/XPCOM/dllname
 */
nsresult
nsComponentManagerImpl::PlatformMarkNoComponents(nsDll *dll)
{
    PR_ASSERT(mRegistry!=NULL);
    
    // XXX Gross. LongLongs dont have a serialization format. This makes
    // XXX the registry non-xp. Someone beat on the nspr people to get
    // XXX a longlong serialization function please!

    nsresult rv;

    nsIRegistry::Key dllPathKey;
    rv = mRegistry->AddSubtreeRaw(mXPCOMKey, dll->GetFullPath(), &dllPathKey);    
    if(NS_FAILED(rv))
    {
        return rv;
    }
    	
    PRTime lastModTime = dll->GetLastModifiedTime();
    PRUint32 fileSize = dll->GetSize();
    int32 nComponents = 0;

    rv = mRegistry->SetBytes(dllPathKey,lastModValueName,&lastModTime, sizeof(lastModTime));
    rv = mRegistry->SetInt(dllPathKey,fileSizeValueName,fileSize);
    rv = mRegistry->SetInt(dllPathKey, componentCountValueName, nComponents);
      
    return rv;
}

nsresult
nsComponentManagerImpl::PlatformRegister(QuickRegisterData* regd, nsDll *dll)
{
    // Preconditions
    PR_ASSERT(regd != NULL);
    PR_ASSERT(regd->CIDString != NULL);
    PR_ASSERT(dll != NULL);
    PR_ASSERT(mRegistry!=NULL);

    nsresult rv;
    
    nsIRegistry::Key IDkey;
    rv = mRegistry->AddSubtreeRaw(mCLSIDKey, regd->CIDString, &IDkey);
    if (NS_FAILED(rv)) return (rv);


    rv = mRegistry->SetString(IDkey,classNameValueName, regd->className);
    if (regd->progID)
    {
        rv = mRegistry->SetString(IDkey,progIDValueName, regd->progID);        
    }
    rv = mRegistry->SetString(IDkey, inprocServerValueName, dll->GetFullPath());
    
    if (regd->progID)
    {
        nsIRegistry::Key progIDKey;
        rv = mRegistry->AddSubtreeRaw(mClassesKey, regd->progID, &progIDKey);
        rv = mRegistry->SetString(progIDKey, classIDValueName, regd->CIDString);
    }

    // XXX Gross. LongLongs dont have a serialization format. This makes
    // XXX the registry non-xp. Someone beat on the nspr people to get
    // XXX a longlong serialization function please!
    
    nsIRegistry::Key dllPathKey;
    rv = mRegistry->AddSubtreeRaw(mXPCOMKey,dll->GetFullPath(), &dllPathKey);

    PRTime lastModTime = dll->GetLastModifiedTime();
    int32 fileSize = dll->GetSize();

    rv = mRegistry->SetBytes(dllPathKey,lastModValueName,&lastModTime, sizeof(lastModTime));
    rv = mRegistry->SetInt(dllPathKey,fileSizeValueName, fileSize);

    int32 nComponents = 0;
    rv = mRegistry->GetInt(dllPathKey, componentCountValueName, &nComponents);
    nComponents++;
    rv = mRegistry->SetInt(dllPathKey,componentCountValueName, nComponents);

    return rv;
}

nsresult
nsComponentManagerImpl::PlatformUnregister(QuickRegisterData* regd, const char *aLibrary)
{  
    PR_ASSERT(mRegistry!=NULL);

    
    nsresult rv;

    nsIRegistry::Key cidKey;
    rv = mRegistry->AddSubtreeRaw(mCLSIDKey,regd->CIDString, &cidKey);

    char *progID = NULL;
    rv = mRegistry->GetString(cidKey, progIDValueName, &progID);
    if(NS_SUCCEEDED(rv))
    {
        // XXX RemoveSubtreeRaw
        mRegistry->RemoveSubtree(mClassesKey, progID);
        PR_FREEIF(progID);
    }

    mRegistry->RemoveSubtree(mCLSIDKey, regd->CIDString);
    	
    nsIRegistry::Key libKey;
    rv = mRegistry->GetSubtreeRaw(mXPCOMKey,aLibrary, &libKey);
    if(NS_FAILED(rv)) return rv;

    // We need to reduce the ComponentCount by 1.
    // If the ComponentCount hits 0, delete the entire key.
    int32 nComponents = 0;
    rv = mRegistry->GetInt(libKey, componentCountValueName, &nComponents);
    if(NS_FAILED(rv)) return rv;
    nComponents--;
    
    if (nComponents <= 0)
    {
        // XXX RemoveSubtreeRaw
        rv = mRegistry->RemoveSubtree(libKey, aLibrary);
    }
    else
    {
        rv = mRegistry->SetInt(libKey, componentCountValueName, nComponents);
    }

    return rv;
}

nsresult
nsComponentManagerImpl::PlatformFind(const nsCID &aCID, nsFactoryEntry* *result)
{
    PR_ASSERT(mRegistry!=NULL);

    nsresult rv;

    char *cidString = aCID.ToString();
    nsIRegistry::Key cidKey;
    rv = mRegistry->GetSubtreeRaw(mCLSIDKey, cidString, &cidKey);
    delete [] cidString;

    if (NS_FAILED(rv)) return rv;
    
    char *library = NULL;
    rv = mRegistry->GetString(cidKey, inprocServerValueName, &library);
    if (NS_FAILED(rv))
    {
        // Registry inconsistent. No File name for CLSID.
        return rv;
    }
    autoFree libAutoFree(library);

    // XXX Gross. LongLongs dont have a serialization format. This makes
    // XXX the registry non-xp. Someone beat on the nspr people to get
    // XXX a longlong serialization function please!

    // Get the library name, modifiedtime and size
    PRTime lastModTime = LL_ZERO;
    PRUint32 fileSize = 0;

    nsIRegistry::Key key;
    rv = mRegistry->GetSubtreeRaw(mXPCOMKey, library, &key);
    if (NS_SUCCEEDED(rv))
    {
        PlatformGetFileInfo(key, &lastModTime, &fileSize);			
    }

    nsFactoryEntry *res = new nsFactoryEntry;
    if (res == NULL) return NS_ERROR_OUT_OF_MEMORY;
    rv = res->Init(mDllStore, aCID, library, lastModTime, fileSize);
    if (NS_FAILED(rv)) return rv;

    *result = res;
    return NS_OK;
}

//
// HashProgID
//
nsresult 
nsComponentManagerImpl::HashProgID(const char *aProgID, const nsCID &aClass)
{
    if(!aProgID)
    {
        return NS_ERROR_NULL_POINTER;
    }
    
    nsCStringKey key(aProgID);
    nsCID* cid = (nsCID*) mProgIDs->Get(&key);
    if (cid)
    {
        if (cid == &kNoCID)
        {
            // we don't delete this ptr as it's static (ugh)
        }
        else
        {
            delete cid;
        }
    }
    
    cid = new nsCID(aClass);
    if (!cid)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }
        
    mProgIDs->Put(&key, cid);
    return NS_OK;
}


nsresult
nsComponentManagerImpl::PlatformProgIDToCLSID(const char *aProgID, nsCID *aClass) 
{
    PR_ASSERT(aClass != NULL);
    PR_ASSERT(mRegistry);

    nsresult rv;
    	
    nsIRegistry::Key progIDKey;
    rv = mRegistry->GetSubtreeRaw(mClassesKey, aProgID, &progIDKey);
    if (NS_FAILED(rv)) return rv;

    char *cidString;
    rv = mRegistry->GetString(progIDKey, classIDValueName, &cidString);
    if(NS_FAILED(rv)) return rv;
    autoFree cidAutoFree(cidString);

    if (!(aClass->Parse(cidString)))
    {
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

nsresult
nsComponentManagerImpl::PlatformCLSIDToProgID(nsCID *aClass,
                                              char* *aClassName, char* *aProgID)
{
    	
    PR_ASSERT(aClass);
    PR_ASSERT(mRegistry);

    nsresult rv;

    char* cidStr = aClass->ToString();
    nsIRegistry::Key cidKey;
    rv = mRegistry->GetSubtreeRaw(mClassesKey,cidStr,&cidKey);
    if(NS_FAILED(rv)) return rv;
    PR_FREEIF(cidStr);

    char* classnameString;
    rv = mRegistry->GetString(cidKey, classNameValueName, &classnameString);
    if(NS_FAILED(rv)) return rv;
    *aClassName = classnameString;

    char* progidString;
    rv = mRegistry->GetString(cidKey,progIDValueName,&progidString);
    if (NS_FAILED(rv)) return rv;
    *aProgID = progidString;

    return NS_OK;

}

#endif // USE_NSREG

////////////////////////////////////////////////////////////////////////////////
// nsComponentManagerImpl: Public methods
////////////////////////////////////////////////////////////////////////////////

/**
 * LoadFactory()
 *
 * Given a FactoryEntry, this loads the dll if it has to, find the NSGetFactory
 * symbol, calls the routine to create a new factory and returns it to the
 * caller.
 *
 * No attempt is made to store the factory in any form anywhere.
 */
nsresult
nsComponentManagerImpl::LoadFactory(nsFactoryEntry *aEntry,
                                    nsIFactory **aFactory)
{
    if (aFactory == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aFactory = NULL;

    // LoadFactory() cannot be called for entries that are CID<->factory
    // mapping entries for the session.
    PR_ASSERT(aEntry->dll != NULL);
    	
    if (aEntry->dll->IsLoaded() == PR_FALSE)
    {
        // Load the dll
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
               ("nsComponentManager: + Loading \"%s\".", aEntry->dll->GetFullPath()));
#endif /* NS_DEBUG */
        if (aEntry->dll->Load() == PR_FALSE)
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ERROR,
                   ("nsComponentManager: Library load unsuccessful."));
#endif /* NS_DEBUG */
            
            char errorMsg[1024] = "Cannot get error from nspr. Not enough memory.";
            if (PR_GetErrorTextLength() < (int) sizeof(errorMsg))
                PR_GetErrorText(errorMsg);
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Load(%s) FAILED with error:%s", aEntry->dll->GetFullPath(), errorMsg));
#endif /* NS_DEBUG */

            // Put the error message on the screen.
            printf("**************************************************\n"
    		   "nsComponentManager: Load(%s) FAILED with error: %s\n"
            	   "**************************************************\n",
    		   aEntry->dll->GetFullPath(), errorMsg);

            return NS_ERROR_FAILURE;
        }
    }
    	
#ifdef MOZ_TRACE_XPCOM_REFCNT
    // Inform refcnt tracer of new library so that calls through the
    // new library can be traced.
    nsTraceRefcnt::LoadLibrarySymbols(aEntry->dll->GetFullPath(), aEntry->dll->GetInstance());
#endif
    nsFactoryProc proc = (nsFactoryProc) aEntry->dll->FindSymbol("NSGetFactory");
    if (proc != NULL)
    {
        char* className = NULL;
        char* progID = NULL;
        nsresult rv;

        // XXX dp, warren: deal with this!
#if 0
        rv = CLSIDToProgID(&aEntry->cid, &className, &progID);
        // if CLSIDToProgID fails, just pass null to NSGetFactory
#endif

        nsIServiceManager* serviceMgr = NULL;
        rv = nsServiceManager::GetGlobalServiceManager(&serviceMgr);
        if (NS_FAILED(rv)) return rv;

        rv = proc(serviceMgr, aEntry->cid, className, progID, aFactory);
        if (NS_FAILED(rv)) return rv;

        if (className)
            delete[] className;
        if (progID)
            delete[] progID;
        return rv;
    }
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_ERROR, 
           ("nsComponentManager: NSGetFactory entrypoint not found."));
#endif /* NS_DEBUG */
    return NS_ERROR_FACTORY_NOT_LOADED;
}

/**
 * FindFactory()
 *
 * Given a classID, this finds the factory for this CID by first searching the
 * local CID<->factory mapping. Next it searches for a Dll that implements
 * this classID and calls LoadFactory() to create the factory.
 *
 * Again, no attempt is made at storing the factory.
 */
nsresult
nsComponentManagerImpl::FindFactory(const nsCID &aClass,
                                    nsIFactory **aFactory) 
{
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: FindFactory(%s)", buf);
        delete [] buf;
    }
#endif
    	
    PR_ASSERT(aFactory != NULL);
    	
    PR_EnterMonitor(mMon);
    	
    nsIDKey key(aClass);
    nsFactoryEntry *entry = (nsFactoryEntry*) mFactories->Get(&key);
    	
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;
    	
#ifdef USE_REGISTRY
    if (entry == NULL)
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("\t\tnot found in factory cache. Looking in registry"));
#endif /* NS_DEBUG */
        nsresult rv = PlatformFind(aClass, &entry);

        // If we got one, cache it in our hashtable
        if (NS_SUCCEEDED(rv))
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("\t\tfound in registry."));
#endif /* NS_DEBUG */
            mFactories->Put(&key, entry);
        }
    }
    else
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("\t\tfound in factory cache."));
#endif /* NS_DEBUG */
    }
#endif /* USE_REGISTRY */
    	
    PR_ExitMonitor(mMon);
    	
    if (entry != NULL)
    {
        if ((entry)->factory == NULL)
        {
            res = LoadFactory(entry, aFactory);
        }
        else
        {
            *aFactory = entry->factory;
            NS_ADDREF(*aFactory);
            res = NS_OK;
        }
    }
    	
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tFindFactory() %s",
            NS_SUCCEEDED(res) ? "succeeded" : "FAILED"));
#endif /* NS_DEBUG */
    	
    return res;
}

/**
 * ProgIDToCLSID()
 *
 * Mapping function from a ProgID to a classID. Directly talks to the registry.
 *
 * XXX We need to add a cache this translation as this is done a lot.
 */
nsresult
nsComponentManagerImpl::ProgIDToCLSID(const char *aProgID, nsCID *aClass) 
{
    NS_PRECONDITION(aProgID != NULL, "null ptr");
    if (! aProgID)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aClass != NULL, "null ptr");
    if (! aClass)
        return NS_ERROR_NULL_POINTER;

    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;

#ifdef USE_REGISTRY
    // XXX This isn't quite the best way to do this: we should
    // probably move an nsArray<ProgID> into the FactoryEntry class,
    // and then have the construct/destructor of the factory entry
    // keep the ProgID to CID cache up-to-date. However, doing this
    // significantly improves performance, so it'll do for now.

    nsCStringKey key(aProgID);
    nsCID* cid = (nsCID*) mProgIDs->Get(&key);
    if (cid) {
        if (cid == &kNoCID) {
            // we've already tried to map this ProgID to a CLSID, and found
            // that there _was_ no such mapping in the registry.
        }
        else {
            *aClass = *cid;
            res = NS_OK;
        }
    }
    else {
        // This is the first time someone has asked for this
        // ProgID. Go to the registry to find the CID.
        res = PlatformProgIDToCLSID(aProgID, aClass);

        if (NS_SUCCEEDED(res)) {
            // Found it. So put it into the cache.
            cid = new nsCID(*aClass);
            if (!cid)
                return NS_ERROR_OUT_OF_MEMORY;

            mProgIDs->Put(&key, cid);
        }
        else {
            // Didn't find it. Put a special CID in the cache so we
            // don't need to hit the registry on subsequent requests
            // for the same ProgID.
            mProgIDs->Put(&key, (void*) &kNoCID);
        }
    }
#endif /* USE_REGISTRY */

#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS)) {
        char *buf;
        if (NS_SUCCEEDED(res))
            buf = aClass->ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: ProgIDToCLSID(%s)->%s", aProgID,
                NS_SUCCEEDED(res) ? buf : "[FAILED]"));
        if (NS_SUCCEEDED(res))
            delete [] buf;
    }
#endif
    	
    return res;
}

/**
 * CLSIDToProgID()
 *
 * Translates a classID to a {ProgID, Class Name}. Does direct registry
 * access to do the translation.
 *
 * XXX Would be nice to hook in a cache here too.
 */
nsresult
nsComponentManagerImpl::CLSIDToProgID(nsCID *aClass,
                                      char* *aClassName,
                                      char* *aProgID)
{
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;

#ifdef USE_REGISTRY
    res = PlatformCLSIDToProgID(aClass, aClassName, aProgID);
#endif /* USE_REGISTRY */

#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass->ToString();
        PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
               ("nsComponentManager: CLSIDToProgID(%s)->%s", buf,
                NS_SUCCEEDED(res) ? *aProgID : "[FAILED]"));
        delete [] buf;
    }
#endif

    return res;
}

/**
 * CreateInstance()
 *
 * Create an instance of an object that implements an interface and belongs
 * to the implementation aClass using the factory. The factory is immediately
 * released and not held onto for any longer.
 */
nsresult 
nsComponentManagerImpl::CreateInstance(const nsCID &aClass, 
                                       nsISupports *aDelegate,
                                       const nsIID &aIID,
                                       void **aResult)
{
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: CreateInstance(%s)", buf);
        delete [] buf;
    }
#endif
    if (aResult == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = NULL;
    	
    nsIFactory *factory = NULL;
    nsresult res = FindFactory(aClass, &factory);
    if (NS_SUCCEEDED(res))
    {
        res = factory->CreateInstance(aDelegate, aIID, aResult);
        NS_RELEASE(factory);
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("\t\tFactory CreateInstance() %s.",
                NS_SUCCEEDED(res) ? "succeeded" : "FAILED"));
#endif /* NS_DEBUG */
        return res;
    }

#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
           ("\t\tCreateInstance() FAILED."));
#endif /* NS_DEBUG */
    return NS_ERROR_FACTORY_NOT_REGISTERED;
}

/**
 * CreateInstance()
 *
 * An overload of CreateInstance() that creates an instance of the object that
 * implements the interface aIID and whose implementation has a progID aProgID.
 *
 * This is only a convenience routine that turns around can calls the
 * CreateInstance() with classid and iid.
 *
 * XXX This is a function overload. We need to remove it.
 */
nsresult
nsComponentManagerImpl::CreateInstance(const char *aProgID,
                                       nsISupports *aDelegate,
                                       const nsIID &aIID,
                                       void **aResult)
{
    nsCID clsid;
    nsresult rv = ProgIDToCLSID(aProgID, &clsid);
    if (NS_FAILED(rv)) return rv; 
    return CreateInstance(clsid, aDelegate, aIID, aResult);
}


/**
 * RegisterFactory()
 *
 * Register a factory to be responsible for creation of implementation of
 * classID aClass. Plus creates as association of aClassName and aProgID
 * to the classID. If replace is PR_TRUE, we replace any existing registrations
 * with this one.
 *
 * Once registration is complete, we add the class to the factories cache
 * that we maintain. The factories cache is the ONLY place where these
 * registrations are ever kept.
 *
 * XXX This uses FindFactory() to test if a factory already exists. This
 * XXX has the bad side effect of loading the factory if the previous
 * XXX registration was a dll for this class. We might be able to do away
 * XXX with such a load.
 */
nsresult
nsComponentManagerImpl::RegisterFactory(const nsCID &aClass,
                                        const char *aClassName,
                                        const char *aProgID,
                                        nsIFactory *aFactory, 
                                        PRBool aReplace)
{
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: RegisterFactory(%s, %s, %s), replace = %d.",
                    buf, aClassName, aProgID, (int)aReplace);
        delete [] buf;
    }
#endif

    nsIFactory *old = NULL;
    FindFactory(aClass, &old);

    if (old != NULL)
    {
        NS_RELEASE(old);
        if (!aReplace)
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tFactory already registered."));
#endif /* NS_DEBUG */
            return NS_ERROR_FACTORY_EXISTS;
        }
        else
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tdeleting old Factory Entry."));
#endif /* NS_DEBUG */
        }
    }
    	
    PR_EnterMonitor(mMon);
	
    nsIDKey key(aClass);
    nsFactoryEntry* entry = new nsFactoryEntry(aClass, aFactory);
    if (entry == NULL)
    {
        PR_ExitMonitor(mMon);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mFactories->Put(&key, entry);


    // Update the ProgID->CLSID Map
    if (aProgID)
    {
        nsresult rv = HashProgID(aProgID, aClass);
        if(NS_FAILED(rv))
        {
            // Adding progID mapping failed. This would result in
            // an error in CreateInstance(..progid,..) However
            // we have already added the mapping of CLSID -> factory
            // and hence, a CreateInstance(..,CLSID,..) would pass.
            //
            // mmh! Should I pass back an error or not.
            //
            // thinking..ok we are passing back an error as we
            // think for people registering with progid,
            // CreateInstance(..progid..) is the more used one.
            //
            PR_ExitMonitor(mMon);
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tFactory register succeeded. PROGID->CLSID mapping failed."));
#endif /* NS_DEBUG */
            return (rv);
        }
    }
    PR_ExitMonitor(mMon);
    	
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tFactory register succeeded."));
#endif /* NS_DEBUG */
    	
    return NS_OK;
}

nsresult
nsComponentManagerImpl::RegisterComponent(const nsCID &aClass,
                                          const char *aClassName,
                                          const char *aProgID,
                                          const char *aLibrary,
                                          PRBool aReplace,
                                          PRBool aPersist)
{
    nsresult rv = NS_OK;
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: RegisterComponent(%s, %s, %s, %s), replace = %d, persist = %d.",
                    buf, aClassName, aProgID, aLibrary, (int)aReplace, (int)aPersist);
        delete [] buf;
    }
#endif

    nsIFactory *old = NULL;
    FindFactory(aClass, &old);
    	
    if (old != NULL)
    {
        NS_RELEASE(old);
        if (!aReplace)
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tFactory already registered."));
#endif /* NS_DEBUG */
            return NS_ERROR_FACTORY_EXISTS;
        }
        else
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
                   ("\t\tdeleting registered Factory."));
#endif /* NS_DEBUG */
        }
    }
    	
    PR_EnterMonitor(mMon);
    	
#ifdef USE_REGISTRY
    if (aPersist == PR_TRUE)
    {
        // Add it to the registry
        nsDll *dll = new nsDll(aLibrary);
        if (dll == NULL) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto done;
        }
        // XXX temp hack until we get the dll to give us the entire
        // XXX NSQuickRegisterClassData
        QuickRegisterData cregd = {0};
        cregd.CIDString = aClass.ToString();
        cregd.className = aClassName;
        cregd.progID = aProgID;
        PlatformRegister(&cregd, dll);
        delete [] (char *)cregd.CIDString;
        delete dll;
    } 
    else
#endif
    {
        nsDll *dll = new nsDll(aLibrary);
        if (dll == NULL) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto done;
        }
        nsIDKey key(aClass);
        nsFactoryEntry* entry = new nsFactoryEntry;
        if (entry == NULL) {
            delete dll;
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto done;
        }
        rv = entry->Init(mDllStore, aClass, aLibrary,
                         dll->GetLastModifiedTime(), dll->GetSize());
        delete dll;
        if (NS_FAILED(rv)) {
            goto done;
        }
        mFactories->Put(&key, entry);
    }
    	
 done:
	
    if(NS_SUCCEEDED(rv))
    {
        // Update the ProgID->CLSID Map if we were successful
        // If we do this unconditionally, this map could grow into a map
        // of all component progid. We want to populate the ProgID->CLSID mapping
        // only if we aren't storing the mapping in the registry. If we are
        // storing in the registry, on first creation, the mapping will get
        // added.
#ifdef USE_REGISTRY
        if (aPersist != PR_TRUE)
        {
            rv = HashProgID(aProgID, aClass);
        }
#else /* USE_REGISTRY */
        rv = HashProgID(aProgID, aClass);
#endif /* USE_REGISTRY */
    }
    
    PR_ExitMonitor(mMon);
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("\t\tFactory register %s.",
            rv == NS_OK ? "succeeded" : "failed"));
#endif /* NS_DEBUG */
    return rv;
}

nsresult
nsComponentManagerImpl::UnregisterFactory(const nsCID &aClass,
                                          nsIFactory *aFactory)
{
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS)) 
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: Unregistering Factory.");
        PR_LogPrint("nsComponentManager: + %s.", buf);
        delete [] buf;
    }
#endif
    	
    nsIDKey key(aClass);
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;
    nsFactoryEntry *old = (nsFactoryEntry *) mFactories->Get(&key);
    if (old != NULL)
    {
        if (old->factory == aFactory)
        {
            PR_EnterMonitor(mMon);
            old = (nsFactoryEntry *) mFactories->Remove(&key);
            PR_ExitMonitor(mMon);
            delete old;
            res = NS_OK;
        }

    }

#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("nsComponentManager: ! Factory unregister %s.", 
            NS_SUCCEEDED(res) ? "succeeded" : "failed"));
#endif /* NS_DEBUG */
    	
    return res;
}

nsresult
nsComponentManagerImpl::UnregisterComponent(const nsCID &aClass,
                                            const char *aLibrary)
{
#ifdef NS_DEBUG
    if (PR_LOG_TEST(nsComponentManagerLog, PR_LOG_ALWAYS))
    {
        char *buf = aClass.ToString();
        PR_LogPrint("nsComponentManager: Unregistering Factory.");
        PR_LogPrint("nsComponentManager: + %s in \"%s\".", buf, aLibrary);
        delete [] buf;
    }
#endif
    	
    nsIDKey key(aClass);
    nsFactoryEntry *old = (nsFactoryEntry *) mFactories->Get(&key);
    	
    nsresult res = NS_ERROR_FACTORY_NOT_REGISTERED;
    	
    PR_EnterMonitor(mMon);
    	
    if (old != NULL && old->dll != NULL)
    {
        if (old->dll->GetFullPath() != NULL &&
#ifdef XP_UNIX
            PL_strcasecmp(old->dll->GetFullPath(), aLibrary)
#else
            PL_strcmp(old->dll->GetFullPath(), aLibrary)
#endif
            )
        {
            nsFactoryEntry *entry = (nsFactoryEntry *) mFactories->Remove(&key);
            delete entry;
            res = NS_OK;
        }
#ifdef USE_REGISTRY
        // XXX temp hack until we get the dll to give us the entire
        // XXX NSQuickRegisterClassData
        QuickRegisterData cregd = {0};
        cregd.CIDString = aClass.ToString();
        res = PlatformUnregister(&cregd, aLibrary);
        delete [] (char *)cregd.CIDString;
#endif
    }
    	
    PR_ExitMonitor(mMon);
    	
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_WARNING,
           ("nsComponentManager: ! Factory unregister %s.", 
            NS_SUCCEEDED(res) ? "succeeded" : "failed"));
#endif /* NS_DEBUG */
    	
    return res;
}

static PRBool
nsFreeLibraryEnum(nsHashKey *aKey, void *aData, void* closure) 
{
    nsFactoryEntry *entry = (nsFactoryEntry *) aData;
    nsIServiceManager* serviceMgr = (nsIServiceManager*)closure;
    	
    if (entry->dll && entry->dll->IsLoaded() == PR_TRUE)
    {
        nsCanUnloadProc proc = (nsCanUnloadProc) entry->dll->FindSymbol("NSCanUnload");
        if (proc != NULL) {
            nsresult rv = proc(serviceMgr);
            if (NS_FAILED(rv))
            {
#ifdef NS_DEBUG
                PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
                       ("nsComponentManager: + Unloading \"%s\".", entry->dll->GetFullPath()));
#endif /* NS_DEBUG */
                entry->dll->Unload();
            }
        }
    }
    	
    return PR_TRUE;
}

nsresult
nsComponentManagerImpl::FreeLibraries(void) 
{
    PR_EnterMonitor(mMon);
    	
#ifdef NS_DEBUG
    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
           ("nsComponentManager: Freeing Libraries."));
#endif /* NS_DEBUG */

    nsIServiceManager* serviceMgr = NULL;
    nsresult rv = nsServiceManager::GetGlobalServiceManager(&serviceMgr);
    if (NS_FAILED(rv)) return rv;
    mFactories->Enumerate(nsFreeLibraryEnum, serviceMgr);
    	
    PR_ExitMonitor(mMon);
    	
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * AutoRegister(RegistrationInstant, const char *directory)
 *
 * Given a directory in the following format, this will ensure proper registration
 * of all components. No default director is looked at.
 *
 *    Directory and fullname are what NSPR will accept. For eg.
 *     	WIN	y:/home/dp/mozilla/dist/bin
 *  	UNIX	/home/dp/mozilla/dist/bin
 *  	MAC	/Hard drive/mozilla/dist/apprunner
 *
 * This will take care not loading already registered dlls, finding and
 * registering new dlls, re-registration of modified dlls
 *
 */

nsresult
nsComponentManagerImpl::AutoRegister(RegistrationTime when, const char* dir)
{
    nsresult rv = NS_ERROR_FAILURE;

    if (dir != NULL)
    {
#ifdef DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
               ("nsComponentManager: Autoregistration begins. dir = %s", dir));
        printf("nsComponentManager: Autoregistration begins. dir = %s\n", dir);
#endif /* DEBUG */
        rv = SyncComponentsInDir(when, dir);
#ifdef DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
               ("nsComponentManager: Autoregistration ends. dir = %s", dir));
        printf("nsComponentManager: Autoregistration ends. dir = %s\n", dir);
#endif /* DEBUG */
    }

    return rv;
}


nsresult
nsComponentManagerImpl::SyncComponentsInDir(RegistrationTime when, const char *dir)
{
    PRDir *prdir = PR_OpenDir(dir);
    if (prdir == NULL)
        return NS_ERROR_FAILURE;
    	
    // Create a buffer that has dir/ in it so we can append
    // the filename each time in the loop
    char fullname[NS_MAX_FILENAME_LEN];
    PL_strncpyz(fullname, dir, sizeof(fullname));
    unsigned int n = strlen(fullname);
    if (n+1 < sizeof(fullname))
    {
        fullname[n] = '/';
        n++;
    }
    char *filepart = fullname + n;
    	
    PRDirEntry *dirent = NULL;
    while ((dirent = PR_ReadDir(prdir, PR_SKIP_BOTH)) != NULL)
    {
        PL_strncpyz(filepart, dirent->name, sizeof(fullname)-n);
        nsresult ret = AutoRegisterComponent(when, fullname);
        if (NS_FAILED(ret) && ret == NS_ERROR_IS_DIR) {
            SyncComponentsInDir(when, fullname);
        }
    } // foreach file
    PR_CloseDir(prdir);
    return NS_OK;
}

nsresult
nsComponentManagerImpl::AutoRegisterComponent(RegistrationTime when, const char *fullname)
{
    const char *ValidDllExtensions[] = {
        ".dll",	/* Windows */
        ".dso",	/* Unix */
        ".so",	/* Unix */
        ".sl",	/* Unix: HP */
        ".shlb",	/* Mac ? */
        ".dlm",	/* new for all platforms */
        NULL
    };
    	
    	
    PRFileInfo statbuf;
    if (PR_GetFileInfo(fullname,&statbuf) != PR_SUCCESS)
    {
        // Skip files that cannot be stat
        return NS_ERROR_FAILURE;
    }

    if (statbuf.type == PR_FILE_DIRECTORY)
    {
        // Cant register a directory
        return NS_ERROR_IS_DIR;
    }
    else if (statbuf.type != PR_FILE_FILE)
    {
        // Skip non-files
        return NS_ERROR_FAILURE;
    }
    	
    // deal with only files that have the right extension
    PRBool validExtension = PR_FALSE;
    int flen = PL_strlen(fullname);
    for (int i=0; ValidDllExtensions[i] != NULL; i++)
    {
        int extlen = PL_strlen(ValidDllExtensions[i]);
    		
        // Does fullname end with this extension
        if (flen >= extlen &&
            !PL_strcasecmp(&(fullname[flen - extlen]), ValidDllExtensions[i])
            )
        {
            validExtension = PR_TRUE;
            break;
        }
    }
    	
    if (validExtension == PR_FALSE)
    {
        // Skip invalid extensions
        return NS_ERROR_FAILURE;
    }
    	
    // Check if dll is one that we have already seen
    nsCStringKey key(fullname);
    nsDll *dll = (nsDll *) mDllStore->Get(&key);
    nsresult rv = NS_OK;
    if (dll == NULL)
    {
        // Create nsDll for this from registry and
        // add it to our dll cache mDllStore.
#ifdef USE_REGISTRY
        rv = PlatformCreateDll(fullname, &dll);
        if (NS_SUCCEEDED(rv))
        {
            mDllStore->Put(&key, (void *) dll);
        }
#endif /* USE_REGISTRY */
    }
    	
    if (NS_SUCCEEDED(rv))
    {
        // Make sure the dll is OK
        if (dll->GetStatus() != NS_OK)
        {
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
                   ("nsComponentManager: + nsDll not NS_OK \"%s\". Skipping...",
                    dll->GetFullPath()));
#endif /* NS_DEBUG */
            return NS_ERROR_FAILURE;
        }
    		
        // We already have seen this dll. Check if this dll changed
        if (LL_EQ(dll->GetLastModifiedTime(), statbuf.modifyTime) &&
            (dll->GetSize() == statbuf.size))
        {
            // Dll hasn't changed. Skip.
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
                   ("nsComponentManager: + nsDll not changed \"%s\". Skipping...",
                    dll->GetFullPath()));
#endif /* NS_DEBUG */
            return NS_OK;
        }
    		
        // Aagh! the dll has changed since the last time we saw it.
        // re-register dll
        if (dll->IsLoaded())
        {
            // We are screwed. We loaded the old version of the dll
            // and now we find that the on-disk copy if newer.
            // The only thing to do would be to ask the dll if it can
            // unload itself. It can do that if it hasn't created objects
            // yet.
            nsCanUnloadProc proc = (nsCanUnloadProc)
                dll->FindSymbol("NSCanUnload");
            if (proc != NULL)
            {
                PRBool res = proc(this /*, PR_TRUE*/);
                if (res)
                {
#ifdef NS_DEBUG
                    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
                           ("nsComponentManager: + Unloading \"%s\".",
                            dll->GetFullPath()));
#endif /* NS_DEBUG */
                    dll->Unload();
                }
                else
                {
                    // THIS IS THE WORST SITUATION TO BE IN.
                    // Dll doesn't want to be unloaded. Cannot re-register
                    // this dll.
#ifdef NS_DEBUG
                    PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                           ("nsComponentManager: *** Dll already loaded. "
                            "Cannot unload either. Hence cannot re-register "
                            "\"%s\". Skipping...", dll->GetFullPath()));
#endif /* NS_DEBUG */
                    return NS_ERROR_FAILURE;
                }
            }
            else {
                // dll doesn't have a CanUnload proc. Guess it is
                // ok to unload it.
                dll->Unload();
#ifdef NS_DEBUG
                PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS, 
                       ("nsComponentManager: + Unloading \"%s\". (no CanUnloadProc).",
                        dll->GetFullPath()));
    				
#endif /* NS_DEBUG */
            }
    			
        } // dll isloaded
    		
        // Sanity.
        if (dll->IsLoaded())
        {
            // We went through all the above to make sure the dll
            // is unloaded. And here we are with the dll still
            // loaded. Whoever taught dp programming...
#ifdef NS_DEBUG
            PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
                   ("nsComponentManager: Dll still loaded. Cannot re-register "
                    "\"%s\". Skipping...", dll->GetFullPath()));
#endif /* NS_DEBUG */
            return NS_ERROR_FAILURE;
        }
    } // dll != NULL
    else
    {
        // Create and add the dll to the mDllStore
        // It is ok to do this even if the creation of nsDll
        // didnt succeed. That way we wont do this again
        // when we encounter the same dll.
        dll = new nsDll(fullname);
        if (dll == NULL)
            return NS_ERROR_OUT_OF_MEMORY;
        mDllStore->Put(&key, (void *) dll);
    } // dll == NULL
    	
    // Either we are seeing the dll for the first time or the dll has
    // changed since we last saw it and it is unloaded successfully.
    //
    // Now we can try register the dll for sure. 
    nsresult res = SelfRegisterDll(dll);
    nsresult ret = NS_OK;
    if (NS_FAILED(res))
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Autoregistration FAILED for "
                "\"%s\". Skipping...", dll->GetFullPath()));
#endif /* NS_DEBUG */
        // Mark dll as not xpcom dll along with modified time and size in
        // the registry so that we wont need to load the dll again every
        // session until the dll changes.
#ifdef USE_REGISTRY
        PlatformMarkNoComponents(dll);
#endif /* USE_REGISTRY */
        ret = NS_ERROR_FAILURE;
    }
    else
    {
#ifdef NS_DEBUG
        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: Autoregistration Passed for "
                "\"%s\".", dll->GetFullPath()));
#endif /* NS_DEBUG */
        // Marking dll along with modified time and size in the
        // registry happens at PlatformRegister(). No need to do it
        // here again.
    }
    return ret;
}

/*
 * SelfRegisterDll
 *
 * Given a dll abstraction, this will load, selfregister the dll and
 * unload the dll.
 *
 */
nsresult
nsComponentManagerImpl::SelfRegisterDll(nsDll *dll)
{
    // Precondition: dll is not loaded already
    PR_ASSERT(dll->IsLoaded() == PR_FALSE);

    nsresult res = NS_ERROR_FAILURE;
    	
    if (dll->Load() == PR_FALSE)
    {
#ifdef NS_DEBUG
        // Cannot load. Probably not a dll.
        char errorMsg[1024] = "Cannot get error from nspr. Not enough memory.";
        if (PR_GetErrorTextLength() < (int) sizeof(errorMsg))
            PR_GetErrorText(errorMsg);

        PR_LOG(nsComponentManagerLog, PR_LOG_ALWAYS,
               ("nsComponentManager: SelfRegisterDll(%s) Load FAILED with error:%s", dll->GetFullPath(), errorMsg));

    	// Put the error message on the screen.
        // For now this message is also for optimized builds. Hence no ifdef DEBUG
        printf("**************************************************\n"
               "nsComponentManager: Load(%s) FAILED with error: %s\n"
               "**************************************************\n",
               dll->GetFullPath(), errorMsg);
#endif /* NS_DEBUG */
        return(NS_ERROR_FAILURE);
    }
    	
    nsRegisterProc regproc = (nsRegisterProc)dll->FindSymbol("NSRegisterSelf");

    if (regproc == NULL)
    {
        // Smart registration
        QuickRegisterData* qr =
            (QuickRegisterData*)dll->FindSymbol(NS_QUICKREGISTER_DATA_SYMBOL);
        if (qr == NULL)
        {
            res = NS_ERROR_NO_INTERFACE;
        }
        else
        {
            // XXX register the quick registration data on behalf of the dll
            // XXX for now return failure
            res = NS_ERROR_FAILURE;
        }
    }
    else
    {
        // Call the NSRegisterSelfProc to enable dll registration
        nsIServiceManager* serviceMgr = NULL;
        res = nsServiceManager::GetGlobalServiceManager(&serviceMgr);
        if (NS_SUCCEEDED(res)) {
            res = regproc(serviceMgr, dll->GetFullPath());
        }
    }
    dll->Unload();
    return res;
}

nsresult
nsComponentManagerImpl::SelfUnregisterDll(nsDll *dll)
{
    // Precondition: dll is not loaded
    PR_ASSERT(dll->IsLoaded() == PR_FALSE);
    	
    if (dll->Load() == PR_FALSE)
    {
        // Cannot load. Probably not a dll.
        return(NS_ERROR_FAILURE);
    }
    	
    nsUnregisterProc unregproc =
        (nsUnregisterProc) dll->FindSymbol("NSUnregisterSelf");
    nsresult res = NS_OK;
    	
    if (unregproc == NULL)
    {
        // Smart unregistration
        QuickRegisterData* qr = (QuickRegisterData*)
            dll->FindSymbol(NS_QUICKREGISTER_DATA_SYMBOL);
        if (qr == NULL)
        {
            return(NS_ERROR_NO_INTERFACE);
        }
        // XXX unregister the dll based on the quick registration data
    }
    else
    {
        // Call the NSUnregisterSelfProc to enable dll de-registration
        nsIServiceManager* serviceMgr = NULL;
        res = nsServiceManager::GetGlobalServiceManager(&serviceMgr);
        if (NS_SUCCEEDED(res)) {
            res = unregproc(serviceMgr, dll->GetFullPath());
        }
    }
    dll->Unload();
    return res;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_GetGlobalComponentManager(nsIComponentManager* *result)
{
    nsresult rv = NS_OK;

    if (nsComponentManagerImpl::gComponentManager == NULL)
    {
        // XPCOM needs initialization.
        rv = NS_InitXPCOM(NULL);
    }

    if (NS_SUCCEEDED(rv))
    {
        // NO ADDREF since this is never intended to be released.
        *result = nsComponentManagerImpl::gComponentManager;
    }

    return rv;
}

////////////////////////////////////////////////////////////////////////////////
