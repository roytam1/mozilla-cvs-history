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
#include "nsINetSupportDialogService.h"
#include "nsIWindowMediator.h"
#include "nsISessionHistory.h"
#include "rdf.h"
#include "nsICommonDialogs.h"
#include "nsIDialogParamBlock.h"
#include "nsAbout.h"
#include "nsIGenericFactory.h"


#include "nsIAppShellService.h"
#include "nsCommandLineService.h"  
#include "nsNetSupportDialog.h"
#include "nsAppShellService.h"
#include "nsXPConnectFactory.h"
#include "nsWindowMediator.h"
#include "nsSessionHistory.h"
#include "nsCommonDialogs.h"
#include "nsDialogParamBlock.h"
#include "nsFileLocations.h"

/* extern the factory entry points for each component... */
nsresult NS_NewAppShellServiceFactory(nsIFactory** aFactory);
nsresult NS_NewXPConnectFactoryFactory(nsIFactory** aResult);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsCmdLineService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShellService);
NS_GENERIC_FACTORY_CONSTRUCTOR(XPConnectFactoryImpl);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsNetSupportDialog);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowMediator);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSessionHistory);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCommonDialogs);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDialogParamBlock);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileLocator);


static nsModuleComponentInfo gAppShellModuleInfo[] =
{
  { "AppShell Service",
    NS_APPSHELL_SERVICE_CID,
    "component://netscape/appshell/appShellService",
    nsAppShellServiceConstructor,
  },
  { "CommandLine Service",
    NS_COMMANDLINE_SERVICE_CID,
    NULL,
    nsCmdLineServiceConstructor,
  },
  { "XPConnect Factory?",
    NS_XPCONNECTFACTORY_CID,
    NULL,
    XPConnectFactoryImplConstructor,
  },
  { "Net Support Dialogs",
    NS_NETSUPPORTDIALOG_CID,
    NULL,
    nsNetSupportDialogConstructor,
  },
  { "Window Mediator",
    NS_WINDOWMEDIATOR_CID,
    NS_RDF_DATASOURCE_PROGID_PREFIX "window-mediator",
    nsWindowMediatorConstructor,
  },
  { "Session History",
    NS_SESSIONHISTORY_CID,
    NULL,
    nsSessionHistoryConstructor,
  },
  { "Common Dialogs",
    NS_CommonDialog_CID,
    "component://netscape/appshell/commonDialogs",
    nsCommonDialogsConstructor,
  },
  { "kDialogParamBlockCID",
    NS_DialogParamBlock_CID,
    NULL,
    nsDialogParamBlockConstructor,
  },
  { "kAboutModuleCID",
    NS_ABOUT_CID,
    NS_ABOUT_MODULE_PROGID_PREFIX,
    nsAbout::Create,
  },
  { "File Locator Service",
    NS_FILELOCATOR_CID,
    NS_FILELOCATOR_PROGID,
    nsFileLocatorConstructor,
  },
};

NS_IMPL_NSGETMODULE("appshell", gAppShellModuleInfo)

