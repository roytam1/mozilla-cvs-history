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

class nsIImapHostSessionList; 
class nsString2;
class nsIImapUrl;

class nsImapService : public nsIImapService
{
public:

	nsImapService();
	virtual ~nsImapService();
	
	NS_DECL_ISUPPORTS

	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIImapService interface 
	////////////////////////////////////////////////////////////////////////////////////////

	NS_IMETHOD CreateImapConnection (PLEventQueue *aEventQueue, nsIImapProtocol ** aImapConnection);

	NS_IMETHOD SelectFolder(PLEventQueue * aClientEventQueue, nsIImapMailFolderSink *, nsIUrlListener * aUrlListener, nsIURL ** aURL);	
	NS_IMETHOD LiteSelectFolder(PLEventQueue * aClientEventQueue, nsIImapMailFolderSink * aImapMailFolder, 
											  nsIUrlListener * aUrlListener, nsIURL ** aURL);
	NS_IMETHOD FetchMessage(PLEventQueue * aClientEventQueue, 
												nsIImapMailFolderSink * aImapMailFolder, 
												nsIImapMessageSink * aImapMessage,
												nsIUrlListener * aUrlListener, nsIURL ** aURL,
												const char *messageIdentifierList,
												PRBool messageIdsAreUID);
	////////////////////////////////////////////////////////////////////////////////////////
	// End support of nsIImapService interface 
	////////////////////////////////////////////////////////////////////////////////////////

protected:
	nsresult GetImapConnectionAndUrl(PLEventQueue * aClientEventQueue, nsIImapUrl  * &imapUrl, 
		nsIImapProtocol * &protocolInstance, nsString2 &urlSpec);
	nsresult CreateStartOfImapUrl(nsIImapUrl &imapUrl, nsString2 &urlString);
	nsIImapHostSessionList * m_sessionList; // the one and only list of all host sessions...

};

#endif /* nsImapService_h___ */
