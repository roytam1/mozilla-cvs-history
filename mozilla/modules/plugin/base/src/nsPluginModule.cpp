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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIGenericFactory.h"
#include "nsIPluginManager.h"
#include "nsPluginsCID.h"
#include "nsPluginHostImpl.h"
#include "nsPluginDocLoaderFactory.h"

static nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    "component://netscape/plugin/host",
    nsPluginHostImpl::Create },

  { "Plugin Manager",
    NS_PLUGINMANAGER_CID,
    "component://netscape/plugin/manager",
    nsPluginHostImpl::Create },

  { "Plugin Doc Loader Factory",
    NS_PLUGINDOCLOADERFACTORY_CID,
    "component://netscape/plugin/doc-loader/factory",
    nsPluginDocLoaderFactory::Create },
};

NS_IMPL_NSGETMODULE("nsPluginModule", gComponentInfo);
