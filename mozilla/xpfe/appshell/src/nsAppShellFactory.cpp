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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include "nsIFactory.h"
#include "nsRepository.h"
#include "nscore.h"
#include "nsIComponentManager.h"
#include "nsAppShellCIDs.h"
#include "nsICmdLineService.h"
#include "nsIFileLocator.h"
#include "nsIWindowMediator.h"
#include "rdf.h"
#include "nsAbout.h"
#include "nsIGenericFactory.h"


#include "nsIAppShellService.h"
#include "nsCommandLineService.h"  
#include "nsAppShellService.h"
#include "nsXPConnectFactory.h"
#include "nsWindowMediator.h"
#include "nsFileLocations.h"

#ifdef XP_MAC
#include "nsMacMIMEDataSource.h"
#include "nsInternetConfigService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacMIMEDataSource);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsInternetConfigService);
#endif

#include "nsUserInfo.h"

/* extern the factory entry points for each component... */
nsresult NS_NewAppShellServiceFactory(nsIFactory** aFactory);
nsresult NS_NewXPConnectFactoryFactory(nsIFactory** aResult);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsCmdLineService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShellService);
NS_GENERIC_FACTORY_CONSTRUCTOR(XPConnectFactoryImpl);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowMediator);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileLocator);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUserInfo);

static nsModuleComponentInfo gAppShellModuleInfo[] =
{
  { "AppShell Service",
    NS_APPSHELL_SERVICE_CID,
    "@mozilla.org/appshell/appShellService;1",
    nsAppShellServiceConstructor,
  },
  { "CommandLine Service",
    NS_COMMANDLINE_SERVICE_CID,
    "@mozilla.org/appshell/commandLineService;1",
    nsCmdLineServiceConstructor,
  },
  { "XPConnect Factory?",
    NS_XPCONNECTFACTORY_CID,
    NULL,
    XPConnectFactoryImplConstructor,
  },
  { "Window Mediator",
    NS_WINDOWMEDIATOR_CID,
    NS_RDF_DATASOURCE_CONTRACTID_PREFIX "window-mediator",
    nsWindowMediatorConstructor,
  },
  { "kAboutModuleCID",
    NS_ABOUT_CID,
    NS_ABOUT_MODULE_CONTRACTID_PREFIX,
    nsAbout::Create,
  },
  { "File Locator Service",
    NS_FILELOCATOR_CID,
    NS_FILELOCATOR_CONTRACTID,
    nsFileLocatorConstructor,
  },
  { "User Info Service",
    NS_USERINFO_CID,
    NS_USERINFO_CONTRACTID,
    nsUserInfoConstructor,
  },
 #if XP_MAC
   { "MacMIME data source",
    NS_NATIVEMIMEDATASOURCE_CID,
    NS_NATIVEMIMEDATASOURCE_CONTRACTID,
    nsMacMIMEDataSourceConstructor,
  },
   { "Internet Config Service",
   NS_INTERNETCONFIGSERVICE_CID,
   NS_INTERNETCONFIGSERVICE_CONTRACTID,
   nsInternetConfigServiceConstructor,
   },
 #endif
};

NS_IMPL_NSGETMODULE("appshell", gAppShellModuleInfo)


#ifdef XP_WIN32
  //in addition to returning a version number for this module,
  //this also provides a convenient hook for the preloader
  //to keep (some if not all) of the module resident.
extern "C" __declspec(dllexport) float GetVersionNumber(void) {
  return 1.0;
}
#endif

