/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsICategoryManager.h"
#include "nsAutoComplete.h"
#include "nsBookmarksService.h"
#include "nsDirectoryViewer.h"
#include "nsGlobalHistory.h"
#include "nsLocalSearchService.h"
#include "nsInternetSearchService.h"
#include "nsRelatedLinksHandlerImpl.h"
#include "nsTimeBomb.h"
#include "nsUrlbarHistory.h"
#include "nsXPIDLString.h"
#if defined(XP_WIN)
#include "nsUrlWidget.h"
#include "nsWindowsHooks.h"
#endif // Windows
#if defined(MOZ_LDAP_XPCOM)
#include "nsLDAPAutoCompleteSession.h"
#endif

// Factory constructors
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteItem)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteResults)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsBookmarksService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPIndex, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDirectoryViewerFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGlobalHistory, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(LocalSearchDataSource, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(InternetSearchDataSource, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(RelatedLinksHandlerImpl, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimeBomb)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlbarHistory)
#if defined(XP_WIN)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUrlWidget, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowsHooks)
#endif // Windows
#if defined(MOZ_LDAP_XPCOM)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPAutoCompleteSession)
#endif

static NS_METHOD
RegisterProc(nsIComponentManager *aCompMgr,
             nsIFile *aPath,
             const char *registryLocation,
             const char *componentType,
             const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    // add the MIME types layotu can handle to the handlers category.
    // this allows users of layout's viewers (the docshell for example)
    // to query the types of viewers layout can create.
    nsXPIDLCString previous;
    rv = catman->AddCategoryEntry("Gecko-Content-Viewers", "application/http-index-format",
                                   NS_DOCUMENT_LOADER_FACTORY_CONTRACTID_PREFIX "view;1?type=application/http-index-format",
                                   PR_TRUE, 
                                   PR_TRUE, 
                                   getter_Copies(previous));
    return rv;
}
static NS_METHOD 
UnregisterProc(nsIComponentManager *aCompMgr,
               nsIFile *aPath,
               const char *registryLocation,
               const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = catman->DeleteCategoryEntry("Gecko-Content-Viewers", 
                                     "application/http-index-format", PR_TRUE);

    return NS_OK;
}

static nsModuleComponentInfo components[] = {
    { "AutoComplete Search Results", NS_AUTOCOMPLETERESULTS_CID, NS_AUTOCOMPLETERESULTS_CONTRACTID,
      nsAutoCompleteResultsConstructor},
    { "AutoComplete Search Item", NS_AUTOCOMPLETEITEM_CID, NS_AUTOCOMPLETEITEM_CONTRACTID,
      nsAutoCompleteItemConstructor},
    { "Bookmarks", NS_BOOKMARKS_SERVICE_CID, NS_BOOKMARKS_SERVICE_CONTRACTID,
      nsBookmarksServiceConstructor },
    { "Bookmarks", NS_BOOKMARKS_SERVICE_CID, NS_BOOKMARKS_DATASOURCE_CONTRACTID,
      nsBookmarksServiceConstructor },
    { "Directory Viewer", NS_DIRECTORYVIEWERFACTORY_CID,
      NS_DOCUMENT_LOADER_FACTORY_CONTRACTID_PREFIX "view;1?type=application/http-index-format",
      nsDirectoryViewerFactoryConstructor, RegisterProc, UnregisterProc  },
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_SERVICE_CONTRACTID,
      nsHTTPIndexConstructor },
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_DATASOURCE_CONTRACTID,
      nsHTTPIndexConstructor },
    { "Global History", NS_GLOBALHISTORY_CID, NS_GLOBALHISTORY_CONTRACTID,
      nsGlobalHistoryConstructor },
    { "Global History", NS_GLOBALHISTORY_CID, NS_GLOBALHISTORY_DATASOURCE_CONTRACTID,
      nsGlobalHistoryConstructor },
    { "Global History", NS_GLOBALHISTORY_CID, NS_GLOBALHISTORY_AUTOCOMPLETE_CONTRACTID,
      nsGlobalHistoryConstructor },
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_SERVICE_CONTRACTID, LocalSearchDataSourceConstructor },
    { "Local Search", NS_RDFFINDDATASOURCE_CID,
      NS_LOCALSEARCH_DATASOURCE_CONTRACTID, LocalSearchDataSourceConstructor },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_SERVICE_CONTRACTID, InternetSearchDataSourceConstructor },
    { "Internet Search", NS_RDFSEARCHDATASOURCE_CID,
      NS_INTERNETSEARCH_DATASOURCE_CONTRACTID, InternetSearchDataSourceConstructor },
    { "Related Links Handler", NS_RELATEDLINKSHANDLER_CID, NS_RELATEDLINKSHANDLER_CONTRACTID,
	  RelatedLinksHandlerImplConstructor},
    { "Netscape TimeBomb", NS_TIMEBOMB_CID, NS_TIMEBOMB_CONTRACTID, nsTimeBombConstructor},
    { "nsUrlbarHistory", NS_URLBARHISTORY_CID,
      NS_URLBARHISTORY_CONTRACTID, nsUrlbarHistoryConstructor },
    { "nsUrlbarHistory", NS_URLBARHISTORY_CID,
      NS_URLBARAUTOCOMPLETE_CONTRACTID, nsUrlbarHistoryConstructor },
#if defined(XP_WIN)
    { NS_IURLWIDGET_CLASSNAME, NS_IURLWIDGET_CID, NS_IURLWIDGET_CONTRACTID, 
      nsUrlWidgetConstructor }, 
    { NS_IWINDOWSHOOKS_CLASSNAME, NS_IWINDOWSHOOKS_CID, NS_IWINDOWSHOOKS_CONTRACTID, 
      nsWindowsHooksConstructor },
#endif // Windows
#if defined(MOZ_LDAP_XPCOM)
    { "LDAP Autocomplete Session", NS_LDAPAUTOCOMPLETESESSION_CID,
	  "@mozilla.org/autocompleteSession;1?type=ldap", 
	  nsLDAPAutoCompleteSessionConstructor },
#endif
};

NS_IMPL_NSGETMODULE(application, components)
