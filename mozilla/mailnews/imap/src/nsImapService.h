/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsImapService_h___
#define nsImapService_h___

#include "nsIImapService.h"
#include "nsIMsgMessageService.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"

class nsIImapHostSessionList; 
class nsString2;
class nsIImapUrl;
class nsIMsgFolder;

class nsImapService : public nsIImapService, public nsIMsgMessageService
{
public:

	nsImapService();
	virtual ~nsImapService();
	
	NS_DECL_ISUPPORTS

	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIImapService interface 
	////////////////////////////////////////////////////////////////////////////////////////

	NS_IMETHOD SelectFolder(nsIEventQueue * aClientEventQueue, 
                            nsIMsgFolder *aImapMailFolder, 
                            nsIUrlListener * aUrlListener, 
                            nsIURL ** aURL);	
	NS_IMETHOD LiteSelectFolder(nsIEventQueue * aClientEventQueue, 
                                nsIMsgFolder * aImapMailFolder, 
                                nsIUrlListener * aUrlListener, 
                                nsIURL ** aURL);
	NS_IMETHOD FetchMessage(nsIEventQueue * aClientEventQueue, 
                            nsIMsgFolder * aImapMailFolder, 
                            nsIImapMessageSink * aImapMessage,
                            nsIUrlListener * aUrlListener, 
                            nsIURL ** aURL,
							nsISupports *aConsumer,
                            const char *messageIdentifierList,
                            PRBool messageIdsAreUID);
	NS_IMETHOD Noop(nsIEventQueue * aClientEventQueue, 
                    nsIMsgFolder * aImapMailFolder,
                    nsIUrlListener * aUrlListener, 
                    nsIURL ** aURL);
	NS_IMETHOD GetHeaders(nsIEventQueue * aClientEventQueue, 
                          nsIMsgFolder * aImapMailFolder, 
                          nsIUrlListener * aUrlListener, 
                          nsIURL ** aURL,
                          const char *messageIdentifierList,
                          PRBool messageIdsAreUID);
	NS_IMETHOD Expunge(nsIEventQueue * aClientEventQueue, 
                       nsIMsgFolder * aImapMailFolder,
                       nsIUrlListener * aUrlListener,
                       nsIURL ** aURL);
	NS_IMETHOD Biff(nsIEventQueue * aClientEventQueue, 
                    nsIMsgFolder * aImapMailFolder,
                    nsIUrlListener * aUrlListener,
                    nsIURL ** aURL,
                    PRUint32 uidHighWater);
	NS_IMETHOD DeleteMessages(nsIEventQueue * aClientEventQueue,
                              nsIMsgFolder * aImapMailFolder, 
                              nsIUrlListener * aUrlListener,
                              nsIURL ** aURL,
                              const char *messageIdentifierList,
                              PRBool messageIdsAreUID);
	NS_IMETHOD DeleteAllMessages(nsIEventQueue * aClientEventQueue, 
                                 nsIMsgFolder * aImapMailFolder,
                                 nsIUrlListener * aUrlListener,
                                 nsIURL ** aURL);
	NS_IMETHOD AddMessageFlags(nsIEventQueue * aClientEventQueue,
                               nsIMsgFolder * aImapMailFolder, 
                               nsIUrlListener * aUrlListener,
                               nsIURL ** aURL,
                               const char *messageIdentifierList,
                               imapMessageFlagsType flags,
                               PRBool messageIdsAreUID);
	NS_IMETHOD SubtractMessageFlags(nsIEventQueue * aClientEventQueue,
                                    nsIMsgFolder * aImapMailFolder, 
                                    nsIUrlListener * aUrlListener,
                                    nsIURL ** aURL,
                                    const char *messageIdentifierList,
                                    imapMessageFlagsType flags,
                                    PRBool messageIdsAreUID);
	NS_IMETHOD SetMessageFlags(nsIEventQueue * aClientEventQueue,
                               nsIMsgFolder * aImapMailFolder, 
                               nsIUrlListener * aUrlListener, 
                               nsIURL ** aURL,
                               const char *messageIdentifierList,
                               imapMessageFlagsType flags,
                               PRBool messageIdsAreUID);

    NS_IMETHOD DiscoverAllFolders(nsIEventQueue* aClientEventQueue,
                                  nsIMsgFolder* aImapMailFolder,
                                  nsIUrlListener* aUrlListener,
                                  nsIURL** aURL);
    NS_IMETHOD DiscoverAllAndSubscribedFolders(nsIEventQueue*
                                               aClientEventQueue,
                                               nsIMsgFolder* aImapMailFolder,
                                               nsIUrlListener* aUrlListener,
                                               nsIURL** aURL);
    NS_IMETHOD DiscoverChildren(nsIEventQueue* aClientEventQueue,
                                nsIMsgFolder* aImapMailFolder,
                                nsIUrlListener* aUrlListener,
                                nsIURL** aURL);
    NS_IMETHOD DiscoverLevelChildren(nsIEventQueue* aClientEventQueue,
                                     nsIMsgFolder* aImapMailFolder,
                                     nsIUrlListener* aUrlListener,
                                     PRInt32 level,
                                     nsIURL** aURL);
    NS_IMETHOD OnlineMessageCopy(nsIEventQueue* aClientEventQueue,
                                 nsIMsgFolder* aSrcFolder,
                                 const char* messageIds,
                                 nsIMsgFolder* aDstFolder,
                                 PRBool idsAreUids,
                                 PRBool isMove,
                                 nsIUrlListener* aUrlListener,
                                 nsIURL** aURL);
	////////////////////////////////////////////////////////////////////////////////////////
	// End support of nsIImapService interface 
	////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIMsgMessageService Interface 
	////////////////////////////////////////////////////////////////////////////////////////
	NS_IMETHOD CopyMessage(const char * aSrcMailboxURI, nsIStreamListener * aMailboxCopy, PRBool moveMessage,
						   nsIUrlListener * aUrlListener, nsIURL **aURL);

	NS_IMETHOD DisplayMessage(const char* aMessageURI, nsISupports * aDisplayConsumer, 
							  nsIUrlListener * aUrlListener, nsIURL ** aURL);

protected:
    nsresult GetFolderName(nsIMsgFolder* aImapFolder,
                           nsString2& folderName);
	nsresult CreateStartOfImapUrl(nsIImapUrl  * &imapUrl,
                                  nsIMsgFolder* &aImapFolder,
                                  nsString2 &urlSpec);
    nsresult GetImapConnectionAndLoadUrl(nsIEventQueue* aClientEventQueue, 
                                         nsIImapUrl* aImapUrl,
                                         nsIUrlListener* aUrlListener,
                                         nsISupports* aConsumer,
                                         nsIURL** aURL);
    nsresult SetImapUrlSink(nsIMsgFolder* aMsgFolder,
                              nsIImapUrl* aImapUrl);
	nsresult DiddleFlags(nsIEventQueue * aClientEventQueue,
                         nsIMsgFolder * aImapMailFolder, 
                         nsIUrlListener * aUrlListener, 
                         nsIURL ** aURL,
                         const char *messageIdentifierList,
                         const char *howToDiddle,
                         imapMessageFlagsType flags,
                         PRBool messageIdsAreUID);

};

#endif /* nsImapService_h___ */
