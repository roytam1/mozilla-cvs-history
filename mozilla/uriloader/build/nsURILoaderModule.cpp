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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIGenericFactory.h"
#include "nsURILoader.h"
#include "nsDocLoader.h"
#include "nsOSHelperAppService.h"
#include "nsExternalProtocolHandler.h"

////////////////////////////////////////////////////////////////////////
// Define the contructor function for the objects
//
// NOTE: This creates an instance of objects by using the default constructor
//
NS_GENERIC_FACTORY_CONSTRUCTOR(nsURILoader)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDocLoaderImpl, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOSHelperAppService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExternalProtocolHandler)

////////////////////////////////////////////////////////////////////////
// Define a table of CIDs implemented by this module along with other
// information like the function to create an instance, contractid, and
// class name.
//
static nsModuleComponentInfo components[] = {
  { "Netscape URI Loader Service", NS_URI_LOADER_CID, NS_URI_LOADER_CONTRACTID, nsURILoaderConstructor, },
  { "Netscape Doc Loader", NS_DOCUMENTLOADER_CID, NS_DOCUMENTLOADER_CONTRACTID, nsDocLoaderImplConstructor, },
  { "Netscape Doc Loader Service", NS_DOCUMENTLOADER_SERVICE_CID, NS_DOCUMENTLOADER_SERVICE_CONTRACTID, 
     nsDocLoaderImplConstructor, },
  { "Netscape External Helper App Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_EXTERNALHELPERAPPSERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape External Helper App Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_EXTERNALPROTOCOLSERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape Mime Mapping Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_MIMESERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape Default Protocol Handler", NS_EXTERNALPROTOCOLHANDLER_CID, NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default", 
     nsExternalProtocolHandlerConstructor, } 
};


////////////////////////////////////////////////////////////////////////
// Implement the NSGetModule() exported function for your module
// and the entire implementation of the module object.
//
NS_IMPL_NSGETMODULE("nsURILoaderModule", components)

#ifdef XP_WIN32
  //in addition to returning a version number for this module,
  //this also provides a convenient hook for the preloader
  //to keep (some if not all) of the module resident.
extern "C" __declspec(dllexport) float GetVersionNumber(void) {
  return 1.0;
}
#endif

