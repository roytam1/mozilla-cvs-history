/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Doug Turner <dougt@netscape.com>
 */

#include "nsFileLocations.h"
#include "nsIFileLocator.h"

#include "nsIComponentManager.h"
#include "nsSpecialSystemDirectory.h"
#include "nsDebug.h"

#ifdef XP_MAC
#include <Folders.h>
#include <Files.h>
#include <Memory.h>
#include <Processes.h>
#elif defined(XP_PC)
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#elif defined(XP_UNIX)
#include <unistd.h>
#include <sys/param.h>
#endif

#include "plstr.h"

static NS_DEFINE_IID(kIFileLocatorIID, NS_IFILELOCATOR_IID);

#if XP_PC
//----------------------------------------------------------------------------------------
static char* MakeUpperCase(char* aPath)
//----------------------------------------------------------------------------------------
{
  // windows does not care about case.  push to uppercase:
  int length = strlen(aPath);
  for (int i = 0; i < length; i++)
      if (islower(aPath[i]))
        aPath[i] = _toupper(aPath[i]);
    
  return aPath;
}
#endif

//----------------------------------------------------------------------------------------
nsSpecialFileSpec::nsSpecialFileSpec(Type aType)
//----------------------------------------------------------------------------------------
:    nsFileSpec((const char*)nsnull)
{
    *this = aType;
}

//----------------------------------------------------------------------------------------
nsSpecialFileSpec::~nsSpecialFileSpec()
//----------------------------------------------------------------------------------------
{
}

//----------------------------------------------------------------------------------------
void nsSpecialFileSpec::operator = (Type aType)
//----------------------------------------------------------------------------------------
{
    *this = (const char*)nsnull;
    switch (aType)
    {
        
	#ifdef XP_MAC
        case App_PrefsDirectory30:
        case App_PrefsDirectory40:
        case App_PrefsDirectory50:
	        {
	            *this = nsSpecialSystemDirectory(nsSpecialSystemDirectory::Mac_PreferencesDirectory);
	            *this += "Netscape \xC4";
	            break;
	        }
    #elif defined(XP_PC)
        case App_PrefsDirectory30:
        case App_PrefsDirectory40:
            NS_NOTYETIMPLEMENTED("Write me!");
            break;    
        case App_PrefsDirectory50:
	        {
	            *this = nsSpecialSystemDirectory(nsSpecialSystemDirectory::OS_CurrentProcessDirectory);
	            break;
	        }
	#else
        case App_PrefsDirectory30:
        case App_PrefsDirectory40:
        case App_PrefsDirectory50:
            NS_NOTYETIMPLEMENTED("Write me!");
            break;    
	#endif
        
        case App_UserProfileDirectory30:
        case App_UserProfileDirectory40:
        case App_UserProfileDirectory50:
            NS_NOTYETIMPLEMENTED("Write me!");
            break;    
        
        case App_PreferencesFile30:
	        {
	            *this = nsSpecialFileSpec(App_PrefsDirectory30);
	        #ifdef XP_MAC
	            *this += "Netscape Preferences";
	        #elif defined(XP_UNIX)
	            *this += "preferences.js";
	        #else
	            *this += "prefs.js";
	        #endif
	        }
	        break;
        case App_PreferencesFile40:
	        {
	            *this = nsSpecialFileSpec(App_PrefsDirectory40);
	        #ifdef XP_MAC
	            *this += "Netscape Preferences";
	        #elif defined(XP_UNIX)
	            *this += "preferences.js";
	        #else
	            *this += "prefs.js";
	        #endif
	        }
            break;    
        case App_PreferencesFile50:
	        {
	            *this = nsSpecialFileSpec(App_PrefsDirectory50);
	            *this += "prefs50.js";
	            break;
	        }
            break;    
        
        case App_BookmarksFile30:
	        #ifdef XP_MAC
	        {
	            // This is possibly correct on all platforms
	            *this = nsSpecialFileSpec(App_PrefsDirectory30);
	            *this += "Bookmarks.html";
	            break;
	        }
	        #endif
        case App_BookmarksFile40:
	        #ifdef XP_MAC
	        {
	            // This is possibly correct on all platforms
	            *this = nsSpecialFileSpec(App_PrefsDirectory40);
	            *this += "Bookmarks.html";
	            break;
	        }
	        #endif
        case App_BookmarksFile50:
            NS_NOTYETIMPLEMENTED("Write me!");
            break;    
        
        case App_Registry40:
	        #ifdef XP_MAC
	        {
	            *this = nsSpecialFileSpec(App_PrefsDirectory30);
	            *this += "Netscape Registry";
	            break;
	        }
	        #endif
        case App_Registry50:
	        #ifdef XP_MAC
	        {
	            *this = nsSpecialFileSpec(App_PrefsDirectory30);
	            *this += "Mozilla Registry";
	            break;
	        }
	        #endif
        
            NS_NOTYETIMPLEMENTED("Write me!");
            break;    

        case App_DirectoryBase:
        case App_FileBase:
        default:
            NS_ERROR("Invalid location type");
            break;    
    }
} // nsSpecialFileSpec::operator =

static NS_DEFINE_IID(kIFactoryIID,         NS_IFACTORY_IID);

//========================================================================================
class nsFileLocator : public nsIFileLocator
//========================================================================================
{
public:
  nsFileLocator();
  
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetFileLocation(
      PRUint32 aType, // NOTE: actually nsSpecialFileSpec:Type, see nsFileLocations.h
      nsFileSpec* outSpec);

protected:
  virtual ~nsFileLocator();

};


//----------------------------------------------------------------------------------------
nsFileLocator::nsFileLocator()
//----------------------------------------------------------------------------------------
{
  NS_INIT_REFCNT();
}

//----------------------------------------------------------------------------------------
nsFileLocator::~nsFileLocator()
//----------------------------------------------------------------------------------------
{
}

NS_IMPL_ISUPPORTS(nsFileLocator, kIFileLocatorIID);

//----------------------------------------------------------------------------------------
NS_IMETHODIMP nsFileLocator::GetFileLocation(
      PRUint32 aType,
      nsFileSpec* outSpec)
//----------------------------------------------------------------------------------------
{
  if (nsnull == outSpec)
    return NS_ERROR_NULL_POINTER;

   *(nsSpecialFileSpec*)outSpec = (nsSpecialFileSpec::Type)aType;
   return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_EXPORT nsresult NS_NewFileLocator(nsIFileLocator** aResult)
//----------------------------------------------------------------------------------------
{
  if (nsnull == aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = new nsFileLocator();
  if (nsnull == *aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

//========================================================================================
class nsFileLocatorFactory : public nsIFactory
//========================================================================================
{
public:
  nsFileLocatorFactory();
  NS_DECL_ISUPPORTS

  // nsIFactory methods
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);
  
  NS_IMETHOD LockFactory(PRBool aLock);  
protected:
  virtual ~nsFileLocatorFactory();
};

//----------------------------------------------------------------------------------------
nsFileLocatorFactory::nsFileLocatorFactory()
//----------------------------------------------------------------------------------------
{
  NS_INIT_REFCNT();
}

//----------------------------------------------------------------------------------------
nsresult nsFileLocatorFactory::LockFactory(PRBool /*lock*/)
//----------------------------------------------------------------------------------------
{
  return NS_OK;
}

//----------------------------------------------------------------------------------------
nsFileLocatorFactory::~nsFileLocatorFactory()
//----------------------------------------------------------------------------------------
{
  NS_ASSERTION(mRefCnt == 0, "non-zero refcnt at destruction");
}

NS_IMPL_ISUPPORTS(nsFileLocatorFactory, kIFactoryIID);

//----------------------------------------------------------------------------------------
nsresult nsFileLocatorFactory::CreateInstance(nsISupports *aOuter,
                                  const nsIID &aIID,
                                  void **aResult)
//----------------------------------------------------------------------------------------
{
    nsresult rv;
    nsFileLocator* inst;

    if (!aResult)
        return NS_ERROR_NULL_POINTER;

    *aResult = NULL;
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    NS_NEWXPCOM(inst, nsFileLocator);
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inst);
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}


//----------------------------------------------------------------------------------------
extern "C" NS_APPSHELL nsresult NS_NewFileLocatorFactory(nsIFactory** aFactory)
//----------------------------------------------------------------------------------------
{
    nsresult rv = NS_OK;
    nsIFactory* inst = new nsFileLocatorFactory();
    if (!inst)
        rv = NS_ERROR_OUT_OF_MEMORY;
    else
        NS_ADDREF(inst);

    *aFactory = inst;
    return rv;
}
