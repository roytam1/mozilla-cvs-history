/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCOMPtr.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIPluginHost.h"
#include "nsIPluginManager.h"
#include "nsIServiceManager.h"
#include "nsPluginDocLoaderFactory.h"
#include "nsPluginViewer.h"
#include "nsPluginError.h"

NS_METHOD
nsPluginDocLoaderFactory::Create(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  NS_PRECONDITION(aOuter == nsnull, "no aggregation");
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsPluginDocLoaderFactory* factory = new nsPluginDocLoaderFactory();
  if (! factory)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  NS_ADDREF(factory);
  rv = factory->QueryInterface(aIID, aResult);
  NS_RELEASE(factory);
  return rv;
}

NS_IMPL_ISUPPORTS1(nsPluginDocLoaderFactory, nsIDocumentLoaderFactory);

NS_IMETHODIMP
nsPluginDocLoaderFactory::CreateInstance(const char *aCommand,
                                         nsIChannel* aChannel,
                                         nsILoadGroup* aLoadGroup,
                                         const char* aContentType, 
                                         nsISupports* aContainer,
                                         nsISupports* aExtraInfo,
                                         nsIStreamListener** aDocListener,
                                         nsIContentViewer** aDocViewer)
{
  static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
  nsCOMPtr<nsIPluginHost> pluginHost = do_GetService(kPluginManagerCID);
  if(! pluginHost)
    return NS_ERROR_FAILURE;

  if (NS_FAILED(pluginHost->IsPluginEnabledForType(aContentType))) {
    // if we fail here refresh plugins and try again, see bug 143178
    nsCOMPtr<nsIPluginManager> pluginManager = do_GetService(kPluginManagerCID);
    if (!pluginManager)
      return NS_ERROR_FAILURE;

    // no need to do anything if plugins have not been changed
    if (NS_ERROR_PLUGINS_PLUGINSNOTCHANGED == pluginManager->ReloadPlugins(PR_FALSE))
      return NS_ERROR_FAILURE;

    if (NS_FAILED(pluginHost->IsPluginEnabledForType(aContentType)))
      return NS_ERROR_FAILURE;
  }

  if (pluginHost->IsPluginEnabledForType(aContentType) != NS_OK)
    return NS_ERROR_FAILURE;

  return NS_NewPluginContentViewer(aCommand, aDocListener, aDocViewer);
}

NS_IMETHODIMP
nsPluginDocLoaderFactory::CreateInstanceForDocument(nsISupports* aContainer,
                                                    nsIDocument* aDocument,
                                                    const char *aCommand,
                                                    nsIContentViewer** aDocViewerResult)
{
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPluginDocLoaderFactory::CreateBlankDocument(nsILoadGroup *aLoadGroup,
                                              nsIDocument **_retval) {
  NS_NOTREACHED("how'd I get here?");
  return NS_ERROR_FAILURE;
}

