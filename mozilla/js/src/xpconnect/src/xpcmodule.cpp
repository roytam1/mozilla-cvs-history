/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   John Bandhauer <jband@netscape.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* Module level methods. */

#include "xpcprivate.h"

#include "mozJSLoaderConstructors.h"

/* Module implementation for the xpconnect library. */

NS_DECL_CLASSINFO(XPCVariant)

// {DC524540-487E-4501-9AC7-AAA784B17C1C}
#define XPCVARIANT_CID \
    {0xdc524540, 0x487e, 0x4501, \
      { 0x9a, 0xc7, 0xaa, 0xa7, 0x84, 0xb1, 0x7c, 0x1c } }

#define XPCVARIANT_CONTRACTID "@mozilla.org/xpcvariant;1"

// {FE4F7592-C1FC-4662-AC83-538841318803}
#define SCRIPTABLE_INTERFACES_CID \
    {0xfe4f7592, 0xc1fc, 0x4662, \
      { 0xac, 0x83, 0x53, 0x88, 0x41, 0x31, 0x88, 0x3 } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsJSID)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCException)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIXPConnect, nsXPConnect::GetSingleton)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIJSContextStack, nsXPCThreadJSContextStackImpl::GetSingleton)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIJSRuntimeService, nsJSRuntimeServiceImpl::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptError)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCComponents_Interfaces)

NS_DECL_CLASSINFO(nsXPCException)

#ifdef XPCONNECT_STANDALONE
#define NO_SUBSCRIPT_LOADER
#endif

static const nsModuleComponentInfo components[] = {
  {nsnull, NS_JS_ID_CID,                         XPC_ID_CONTRACTID,            nsJSIDConstructor             },
  {nsnull, NS_XPCONNECT_CID,                     XPC_XPCONNECT_CONTRACTID,     nsIXPConnectConstructor       },
  {nsnull, NS_XPC_THREAD_JSCONTEXT_STACK_CID,    XPC_CONTEXT_STACK_CONTRACTID, nsIJSContextStackConstructor  },
  {nsnull, NS_XPCEXCEPTION_CID,                  XPC_EXCEPTION_CONTRACTID,     nsXPCExceptionConstructor, nsnull, nsnull, nsnull, NS_CI_INTERFACE_GETTER_NAME(nsXPCException), nsnull, &NS_CLASSINFO_NAME(nsXPCException)},
  {nsnull, NS_JS_RUNTIME_SERVICE_CID,            XPC_RUNTIME_CONTRACTID,       nsIJSRuntimeServiceConstructor},
  {NS_SCRIPTERROR_CLASSNAME, NS_SCRIPTERROR_CID, NS_SCRIPTERROR_CONTRACTID,    nsScriptErrorConstructor      },
  {nsnull, SCRIPTABLE_INTERFACES_CID,            NS_SCRIPTABLE_INTERFACES_CONTRACTID,        nsXPCComponents_InterfacesConstructor },
  {nsnull, XPCVARIANT_CID,                       XPCVARIANT_CONTRACTID,        nsnull, nsnull, nsnull, nsnull, NS_CI_INTERFACE_GETTER_NAME(XPCVariant), nsnull, &NS_CLASSINFO_NAME(XPCVariant)},

  // jsloader stuff
  { "JS component loader", MOZJSCOMPONENTLOADER_CID,
    mozJSComponentLoaderContractID, mozJSComponentLoaderConstructor,
    RegisterJSLoader, UnregisterJSLoader },
#ifndef NO_SUBSCRIPT_LOADER
  { "JS subscript loader", MOZ_JSSUBSCRIPTLOADER_CID,
    mozJSSubScriptLoadContractID, mozJSSubScriptLoaderConstructor },
#endif
};

PR_STATIC_CALLBACK(void)
xpcModuleDtor(nsIModule* self)
{
    // Release our singletons
    nsXPConnect::ReleaseXPConnectSingleton();
    nsXPCThreadJSContextStackImpl::FreeSingleton();
    nsJSRuntimeServiceImpl::FreeSingleton();
    xpc_DestroyJSxIDClassObjects();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(xpconnect, components, xpcModuleDtor)
