/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

/* nsDll
 *
 * Abstraction of a Dll. Stores modifiedTime and size for easy detection of
 * change in dll.
 *
 * dp Suresh <dp@netscape.com>
 */

#include "xcDll.h"
#include "nsDebug.h"
#include "nsIComponentManager.h"
#include "nsIModule.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#if defined(VMS) && defined(DEBUG)
#include <lib$routines.h>
#include <ssdef.h>
#endif

nsDll::nsDll(const char *codeDllName, int type)
  : m_dllName(NULL),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL),
    m_markForUnload(PR_FALSE), m_registryLocation(0)

{
	m_modDate = LL_Zero();
	m_size = LL_Zero();
	
    if (!codeDllName || !*codeDllName)
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }
    m_dllName = nsCRT::strdup(codeDllName);
    if (!m_dllName)
    {
        m_status = DLL_NO_MEM;
        return;
    }
}

nsDll::nsDll(nsIFile *dllSpec, const char *registryLocation)
  : m_dllName(NULL),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)

{
	m_modDate = LL_Zero();
	m_size = LL_Zero();
    m_dllSpec = dllSpec;

    m_registryLocation = nsCRT::strdup(registryLocation);
    Init(dllSpec);
    // Populate m_modDate and m_size
    if (NS_FAILED(Sync()))
    {
        m_status = DLL_INVALID_PARAM;
    }
}

nsDll::nsDll(nsIFile *dllSpec, const char *registryLocation, PRInt64* modDate, PRInt64* fileSize)
  : m_dllName(NULL),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL), m_markForUnload(PR_FALSE)

{
    m_modDate = LL_Zero();
    m_size = LL_Zero();
    m_dllSpec = dllSpec;

    m_registryLocation = nsCRT::strdup(registryLocation);
    Init(dllSpec);

    if (modDate)
        m_modDate = *modDate;
    else
        m_modDate = LL_Zero();
    
    if (fileSize)
        m_size = *fileSize;
    else
        m_size = LL_Zero();

}

nsDll::nsDll(const char *libPersistentDescriptor)
  : m_dllName(NULL),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL),
    m_markForUnload(PR_FALSE), m_registryLocation(0)

{
    m_modDate = LL_Zero();
    m_size = LL_Zero();

    Init(libPersistentDescriptor);
    // Populate m_modDate and m_size
    if (NS_FAILED(Sync()))
    {
        m_status = DLL_INVALID_PARAM;
    }
}

nsDll::nsDll(const char *libPersistentDescriptor, PRInt64* modDate, PRInt64* fileSize)
  : m_dllName(NULL),
    m_instance(NULL), m_status(DLL_OK), m_moduleObject(NULL),
    m_persistentDescriptor(NULL), m_nativePath(NULL),
    m_markForUnload(PR_FALSE), m_registryLocation(0)

{
    m_modDate = LL_Zero();
    m_size = LL_Zero();

    Init(libPersistentDescriptor);

    // and overwrite the modData and fileSize
	
    if (modDate)
        m_modDate = *modDate;
    else
        m_modDate = LL_Zero();
    
    if (fileSize)
        m_size = *fileSize;
    else
        m_size = LL_Zero();
}
 
void
nsDll::Init(nsIFile *dllSpec)
{
    // Addref the m_dllSpec
    m_dllSpec = dllSpec;

    // Make sure we are dealing with a file
    PRBool isFile = PR_FALSE;
    nsresult rv = m_dllSpec->IsFile(&isFile);
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

    if (isFile == PR_FALSE)
    {
      // Not a file. Cant work with it.
      m_status = DLL_NOT_FILE;
      return;
    }
    
	m_status = DLL_OK;			
}

void
nsDll::Init(const char *libPersistentDescriptor)
{
    nsresult rv;
    m_modDate = LL_Zero();
    m_size = LL_Zero();
	
	if (libPersistentDescriptor == NULL)
	{
		m_status = DLL_INVALID_PARAM;
		return;
	}

    // Create a FileSpec from the persistentDescriptor
    nsCOMPtr<nsILocalFile> dllSpec;
    
    nsCID clsid;
    nsComponentManager::ProgIDToClassID(NS_LOCAL_FILE_PROGID, &clsid);
    rv = nsComponentManager::CreateInstance(clsid, nsnull, 
                                            NS_GET_IID(nsILocalFile),
                                            (void**)getter_AddRefs(dllSpec));
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

    rv = dllSpec->InitWithPath((char *)libPersistentDescriptor);
    if (NS_FAILED(rv))
    {
        m_status = DLL_INVALID_PARAM;
        return;
    }

}


nsDll::~nsDll(void)
{
#if 0
    // The dll gets deleted when the dllStore is destroyed. This happens on
    // app shutdown. At that point, unloading dlls can cause crashes if we have
    // - dll dependencies
    // - callbacks
    // - static dtors
    // Hence turn it back on after all the above have been removed.
    Unload();
#endif
    if (m_dllName)
        nsCRT::free(m_dllName);
    if (m_persistentDescriptor)
        nsCRT::free(m_persistentDescriptor);
    if (m_nativePath)
        nsCRT::free(m_nativePath);
    if (m_registryLocation)
        nsCRT::free(m_registryLocation);

}

nsresult
nsDll::Sync()
{
    if (!m_dllSpec)
        return NS_ERROR_FAILURE;

    // Populate m_modDate and m_size
    nsresult rv = m_dllSpec->GetLastModificationDate(&m_modDate);
    if (NS_FAILED(rv)) return rv;
    rv = m_dllSpec->GetFileSize(&m_size);
    return rv;
}


const char *
nsDll::GetDisplayPath()
{
    if (m_dllName)
        return m_dllName;
    if (m_nativePath)
        return m_nativePath;
    m_dllSpec->GetPath(&m_nativePath);
    return m_nativePath;
}

const char *
nsDll::GetPersistentDescriptorString()
{
    if (m_dllName)
        return m_dllName;
    if (m_persistentDescriptor)
        return m_persistentDescriptor;
    m_dllSpec->GetPath(&m_persistentDescriptor);
    return m_persistentDescriptor;
}

PRBool
nsDll::HasChanged()
{
    if (m_dllName)
        return PR_FALSE;

    // If mod date has changed, then dll has changed
    PRInt64 currentDate;

    nsresult rv = m_dllSpec->GetLastModificationDate(&currentDate);
    
    if (NS_FAILED(rv) || LL_NE(currentDate, m_modDate))
      return PR_TRUE;

    // If size has changed, then dll has changed
    PRInt64 aSize;
    rv = m_dllSpec->GetFileSize(&aSize);
    if (NS_FAILED(rv) || LL_NE(aSize, m_size))
      return PR_TRUE;

    return PR_FALSE;
}




PRBool nsDll::Load(void)
{
	if (m_status != DLL_OK)
	{
		return (PR_FALSE);
	}
	if (m_instance != NULL)
	{
		// Already loaded
		return (PR_TRUE);
	}

    if (m_dllSpec)
    {
        nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(m_dllSpec);

        if (localFile)
            localFile->Load(&m_instance);
        
#ifdef NS_BUILD_REFCNT_LOGGING
        if (m_instance) {
            // Inform refcnt tracer of new library so that calls through the
            // new library can be traced.
            char* displayPath;
            m_dllSpec->GetPath(&displayPath);
            nsTraceRefcnt::LoadLibrarySymbols(displayPath, m_instance);
            nsAllocator::Free(displayPath);
        }
#endif
    }
    else if (m_dllName)
    {
        // if there is not an nsIFile, but there is a dll name, just try to load that..
        m_instance = PR_LoadLibrary(m_dllName);

#ifdef NS_BUILD_REFCNT_LOGGING
        if (m_instance) {
            // Inform refcnt tracer of new library so that calls through the
            // new library can be traced.
            nsTraceRefcnt::LoadLibrarySymbols(m_dllName, m_instance);
        }
#endif    
    }

#if defined(DEBUG) && defined(XP_UNIX)
    // Debugging help for components. Component dlls need to have their
    // symbols loaded before we can put a breakpoint in the debugger.
    // This will help figureing out the point when the dll was loaded.
    BreakAfterLoad(GetDisplayPath());
#endif

    return ((m_instance == NULL) ? PR_FALSE : PR_TRUE);
}

PRBool nsDll::Unload(void)
{
	if (m_status != DLL_OK || m_instance == NULL)
		return (PR_FALSE);

    // Shutdown the dll
    Shutdown();

	PRStatus ret = PR_UnloadLibrary(m_instance);
	if (ret == PR_SUCCESS)
	{
		m_instance = NULL;
		return (PR_TRUE);
	}
	else
		return (PR_FALSE);
}

void * nsDll::FindSymbol(const char *symbol)
{
	if (symbol == NULL)
		return (NULL);
	
	// If not already loaded, load it now.
	if (Load() != PR_TRUE)
		return (NULL);

    return(PR_FindSymbol(m_instance, symbol));
}


// Component dll specific functions
nsresult nsDll::GetDllSpec(nsIFile **fsobj)
{
    NS_ASSERTION(m_dllSpec, "m_dllSpec NULL");
    NS_ASSERTION(fsobj, "xcDll::GetModule : Null argument" );

    *fsobj = m_dllSpec;
    NS_ADDREF(*fsobj);
    return NS_OK;
}

nsresult nsDll::GetModule(nsISupports *servMgr, nsIModule **cobj)
{
    nsIComponentManager *compMgr;
    nsresult rv = NS_GetGlobalComponentManager(&compMgr);
    NS_ASSERTION(compMgr, "Global Component Manager is null" );
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(cobj, "xcDll::GetModule : Null argument" );

    if (m_moduleObject)
    {
        NS_ADDREF(m_moduleObject);
        *cobj = m_moduleObject;
        return NS_OK;
    }

	// If not already loaded, load it now.
	if (Load() != PR_TRUE) return NS_ERROR_FAILURE;

    // We need a nsIFile for location. If we dont
    // have one, create one.
    if (!m_dllSpec && m_dllName)
    {
        // Create m_dllSpec from m_dllName
    }

    nsGetModuleProc proc =
      (nsGetModuleProc) FindSymbol(NS_GET_MODULE_SYMBOL);

    if (proc == NULL)
        return NS_ERROR_FACTORY_NOT_LOADED;

    rv = (*proc) (compMgr, m_dllSpec, &m_moduleObject);
    if (NS_SUCCEEDED(rv))
    {
        NS_ADDREF(m_moduleObject);
        *cobj = m_moduleObject;
    }
    return rv;
}

nsresult nsDll::Shutdown(void)
{
    // Release the module object if we got one
    nsrefcnt refcnt;
    if (m_moduleObject)
    {
        NS_RELEASE2(m_moduleObject, refcnt);
        NS_ASSERTION(refcnt == 0, "Dll moduleObject refcount non zero");
    }
    return NS_OK;

}

void nsDll::BreakAfterLoad(const char *nsprPath)
{
#ifndef XP_BEOS
#ifdef DEBUG
    static PRBool firstTime = PR_TRUE;
    static nsCString breakList[16];
    static int count = 0;

    // return if invalid input
    if (!nsprPath || !*nsprPath) return;

    // return if nothing to break on
    if (!firstTime && count == 0) return;

    if (firstTime)
    {
        firstTime = PR_FALSE;
        // Form the list of dlls to break on load
        nsCAutoString envList(getenv("XPCOM_BREAK_ON_LOAD"));
        if (envList.IsEmpty()) return;
        PRInt32 ofset = 0;
        PRInt32 start = 0;
        do
        {
            ofset = envList.FindChar(':', PR_TRUE, start);
            envList.Mid(breakList[count], start, ofset);
            count++;
            start = ofset + 1;
        }
        while (ofset != -1 && count < 16);
    }

    // Find the dllname part of the string
    nsCString currentPath(nsprPath);
    PRInt32 lastSep = currentPath.RFindCharInSet(":\\/");

    for (int i=0; i<count; i++)
        if (currentPath.Find(breakList[i], PR_TRUE, lastSep) > 0)
        {
            // Loading a dll that we want to break on
            // Put your breakpoint here
            printf("...Loading module %s\n", nsprPath);
            // Break in the debugger here.
#if defined(linux) && defined(__i386)
            asm("int $3");
#elif defined(VMS)
            lib$signal(SS$_DEBUG);
#endif
        }
#endif /* DEBUG */
#endif /* !XP_BEOS */
    return;
}
