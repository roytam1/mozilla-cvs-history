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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIFactory.h"
#include "nsISupports.h"
#include "msgCore.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsMsgBaseCID.h"
#include "pratom.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "rdf.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"

#include "nsMessengerBootstrap.h"
#include "nsMessenger.h"
#include "nsMsgGroupRecord.h"

#include "nsIAppShellComponent.h"
#include "nsIRegistry.h"

/* Include all of the interfaces our factory can generate components for */

#include "nsIUrlListenerManager.h"
#include "nsUrlListenerManager.h"
#include "nsMsgMailSession.h"
#include "nsMsgAccount.h"
#include "nsMsgAccountManager.h"
#include "nsMessengerMigrator.h"
#include "nsMsgIdentity.h"
#include "nsMsgIncomingServer.h"
#include "nsMsgFolderDataSource.h"

#include "nsMsgAccountManagerDS.h"

#include "nsMsgBiffManager.h"
#include "nsStatusBarBiffManager.h"
#include "nsMsgNotificationManager.h"

#include "nsCopyMessageStreamListener.h"
#include "nsMsgCopyService.h"

#include "nsMsgFolderCache.h"

#include "nsMsgStatusFeedback.h"

#include "nsMsgFilterService.h"
#include "nsMsgFilterDataSource.h"
#include "nsMsgFilterDelegateFactory.h"
#include "nsMsgWindow.h"
#include "nsMessage.h"

#include "nsMsgServiceProvider.h"
#include "nsSubscribeDataSource.h"
#include "nsSubscribableServer.h"

#include "nsMsgPrintEngine.h"
#include "nsMsgSearchSession.h"
#include "nsMsgSearchAdapter.h"
#include "nsMsgSearchDataSource.h"
#include "nsMsgFolderCompactor.h"
#include "nsMsgThreadedDBView.h"
#include "nsMsgSpecialViews.h"
#include "nsMsgSearchDBView.h"

#include "nsMsgOfflineManager.h"
// private factory declarations for each component we know how to produce

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMessengerBootstrap)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlListenerManager)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgMailSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMessenger)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgAccountManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMessengerMigrator, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgAccount)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgIdentity)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgFolderDataSource, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgAccountManagerDataSource, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgSearchSession)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgSearchValidityManager)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgSearchDataSource,Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgFilterService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgFilterDataSource)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgFilterDelegateFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgBiffManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsStatusBarBiffManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgNotificationManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCopyMessageStreamListener)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgCopyService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgFolderCache)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgStatusFeedback)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgWindow,Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMessage)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgServiceProviderService, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSubscribeDataSource, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSubscribableServer, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMsgPrintEngine, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFolderCompactState)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOfflineStoreCompactState)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgThreadedDBView);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgThreadsWithUnreadDBView);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgWatchedThreadsWithUnreadDBView);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgSearchDBView);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgOfflineManager);

// The list of components we register
static nsModuleComponentInfo gComponents[] = {
    { "Netscape Messenger Bootstrapper", NS_MESSENGERBOOTSTRAP_CID,
      NS_MESSENGERBOOTSTRAP_CONTRACTID,
      nsMessengerBootstrapConstructor,
    },
    { "Netscape Messenger Window Service", NS_MESSENGERWINDOWSERVICE_CID,
      NS_MESSENGERWINDOWSERVICE_CONTRACTID,
      nsMessengerBootstrapConstructor,
	},
    { "Mail Startup Handler", NS_MESSENGERBOOTSTRAP_CID,
      NS_MAILSTARTUPHANDLER_CONTRACTID,
      nsMessengerBootstrapConstructor,
      nsMessengerBootstrap::RegisterProc,
      nsMessengerBootstrap::UnregisterProc
    },
    { "UrlListenerManager", NS_URLLISTENERMANAGER_CID,
      NS_URLLISTENERMANAGER_CONTRACTID,
      nsUrlListenerManagerConstructor,
    },
    { "Mail Session", NS_MSGMAILSESSION_CID,
      NS_MSGMAILSESSION_CONTRACTID,
      nsMsgMailSessionConstructor,
    },
    { "Messenger DOM interaction object", NS_MESSENGER_CID,
      NS_MESSENGER_CONTRACTID,
      nsMessengerConstructor,
    },
    { "Messenger Account Manager", NS_MSGACCOUNTMANAGER_CID,
      NS_MSGACCOUNTMANAGER_CONTRACTID,
      nsMsgAccountManagerConstructor,
    },
    { "Messenger Migrator", NS_MESSENGERMIGRATOR_CID,
      NS_MESSENGERMIGRATOR_CONTRACTID,
      nsMessengerMigratorConstructor,
    },
    { "Messenger User Account", NS_MSGACCOUNT_CID,
      NS_MSGACCOUNT_CONTRACTID,
      nsMsgAccountConstructor,
    },
    { "Messenger User Identity", NS_MSGIDENTITY_CID,
      NS_MSGIDENTITY_CONTRACTID,
      nsMsgIdentityConstructor,
    },
    { "Mail/News Folder Data Source", NS_MAILNEWSFOLDERDATASOURCE_CID,
      NS_MAILNEWSFOLDERDATASOURCE_CONTRACTID,
      nsMsgFolderDataSourceConstructor,
    },
    { "Mail/News Account Manager Data Source", NS_MSGACCOUNTMANAGERDATASOURCE_CID,
      NS_RDF_DATASOURCE_CONTRACTID_PREFIX "msgaccountmanager",
      nsMsgAccountManagerDataSourceConstructor,
    },
    { "Message Filter Service", NS_MSGFILTERSERVICE_CID,
      NS_MSGFILTERSERVICE_CONTRACTID,
      nsMsgFilterServiceConstructor,
    },
    { "Message Search Session", NS_MSGSEARCHSESSION_CID,
      NS_MSGSEARCHSESSION_CONTRACTID,
      nsMsgSearchSessionConstructor
    },
    { "Message Search Validity Manager", NS_MSGSEARCHVALIDITYMANAGER_CID,
        NS_MSGSEARCHVALIDITYMANAGER_CONTRACTID,
        nsMsgSearchValidityManagerConstructor,
    },
    { "Search Datasource", NS_MSGSEARCHDATASOURCE_CID,
      NS_MSGSEARCHDATASOURCE_CONTRACTID,
      nsMsgSearchDataSourceConstructor,
    },
    { "Message Filter Service", NS_MSGFILTERSERVICE_CID,
      NS_MSGFILTERSERVICE_CONTRACTID,
      nsMsgFilterServiceConstructor,
    },
    { "Message Filter Datasource", NS_MSGFILTERDATASOURCE_CID,
      NS_MSGFILTERDATASOURCE_CONTRACTID,
      nsMsgFilterDataSourceConstructor,
    },
    // XXX temporarily do all the protocols here
    { "Message Filter Delegate Factory", NS_MSGFILTERDELEGATEFACTORY_CID,
      NS_MSGFILTERDELEGATEFACTORY_IMAP_CONTRACTID,
      nsMsgFilterDelegateFactoryConstructor,
    },
    { "Message Filter Delegate Factory", NS_MSGFILTERDELEGATEFACTORY_CID,
      NS_MSGFILTERDELEGATEFACTORY_MAILBOX_CONTRACTID,
      nsMsgFilterDelegateFactoryConstructor,
    },
    { "Message Filter Delegate Factory", NS_MSGFILTERDELEGATEFACTORY_CID,
      NS_MSGFILTERDELEGATEFACTORY_NEWS_CONTRACTID,
      nsMsgFilterDelegateFactoryConstructor,
    },
    // XXX done temporary registration
    
    { "Messenger Biff Manager", NS_MSGBIFFMANAGER_CID,
      NS_MSGBIFFMANAGER_CONTRACTID,
      nsMsgBiffManagerConstructor,
    },
    { "Status Bar Biff Manager", NS_STATUSBARBIFFMANAGER_CID,
      NS_STATUSBARBIFFMANAGER_CONTRACTID,
      nsStatusBarBiffManagerConstructor,
    },
    { "Mail/News Notification Manager", NS_MSGNOTIFICATIONMANAGER_CID,
      NS_MSGNOTIFICATIONMANAGER_CONTRACTID,
      nsMsgNotificationManagerConstructor,
    },
    { "Mail/News CopyMessage Stream Listener", NS_COPYMESSAGESTREAMLISTENER_CID,
      NS_COPYMESSAGESTREAMLISTENER_CONTRACTID,
      nsCopyMessageStreamListenerConstructor,
    },
    { "Mail/News Message Copy Service", NS_MSGCOPYSERVICE_CID,
      NS_MSGCOPYSERVICE_CONTRACTID,
      nsMsgCopyServiceConstructor,
    },
    { "Mail/News Folder Cache", NS_MSGFOLDERCACHE_CID,
      NS_MSGFOLDERCACHE_CONTRACTID,
      nsMsgFolderCacheConstructor,
    },
    { "Mail/News Status Feedback", NS_MSGSTATUSFEEDBACK_CID,
      NS_MSGSTATUSFEEDBACK_CONTRACTID,
      nsMsgStatusFeedbackConstructor,
    },
    { "Mail/News MsgWindow", NS_MSGWINDOW_CID,
      NS_MSGWINDOW_CONTRACTID,
      nsMsgWindowConstructor,
    },
    { "Message Resource", NS_MESSAGE_CID,
      NS_MESSAGE_MAILBOX_CONTRACTID,
      nsMessageConstructor,
    },
    { "Message Resource", NS_MESSAGE_CID,
      NS_MESSAGE_IMAP_CONTRACTID,
      nsMessageConstructor,
    },
    { "Message Resource", NS_MESSAGE_CID,
      NS_MESSAGE_NEWS_CONTRACTID,
      nsMessageConstructor,
    },
    { "Mail/News Print Engine", NS_MSG_PRINTENGINE_CID,
      NS_MSGPRINTENGINE_CONTRACTID,
      nsMsgPrintEngineConstructor,
    },
    { "Mail/News Service Provider Service", NS_MSGSERVICEPROVIDERSERVICE_CID,
      NS_MSGSERVICEPROVIDERSERVICE_CONTRACTID,
      nsMsgServiceProviderServiceConstructor,
    },
    { "Mail/News Subscribe Data Source", NS_SUBSCRIBEDATASOURCE_CID,
      NS_SUBSCRIBEDATASOURCE_CONTRACTID,
      nsSubscribeDataSourceConstructor,
    },
    { "Mail/News Subscribable Server", NS_SUBSCRIBABLESERVER_CID,
	  NS_SUBSCRIBABLESERVER_CONTRACTID,
	  nsSubscribableServerConstructor,
    },
    { "Local folder compactor", NS_MSGLOCALFOLDERCOMPACTOR_CID,
      NS_MSGLOCALFOLDERCOMPACTOR_CONTRACTID,
      nsFolderCompactStateConstructor,
    },
    { "offline store compactor", NS_MSG_OFFLINESTORECOMPACTOR_CID,
      NS_MSGOFFLINESTORECOMPACTOR_CONTRACTID,
      nsOfflineStoreCompactStateConstructor,
    },
    { "threaded db view", NS_MSGTHREADEDDBVIEW_CID,
      NS_MSGTHREADEDDBVIEW_CONTRACTID,
      nsMsgThreadedDBViewConstructor,
    },
    { "threads with unread db view", NS_MSGTHREADSWITHUNREADDBVIEW_CID,
      NS_MSGTHREADSWITHUNREADDBVIEW_CONTRACTID,
      nsMsgThreadsWithUnreadDBViewConstructor,
    },
    { "watched threads with unread db view", NS_MSGWATCHEDTHREADSWITHUNREADDBVIEW_CID,
      NS_MSGWATCHEDTHREADSWITHUNREADDBVIEW_CONTRACTID,
      nsMsgWatchedThreadsWithUnreadDBViewConstructor,
    },
    { "search db view", NS_MSGSEARCHDBVIEW_CID,
      NS_MSGSEARCHDBVIEW_CONTRACTID,
      nsMsgSearchDBViewConstructor,
    },
    { "Messenger Offline Manager", NS_MSGOFFLINEMANAGER_CID,
      NS_MSGOFFLINEMANAGER_CONTRACTID,
      nsMsgOfflineManagerConstructor,
    },

};

NS_IMPL_NSGETMODULE("nsMsgBaseModule", gComponents)
  
