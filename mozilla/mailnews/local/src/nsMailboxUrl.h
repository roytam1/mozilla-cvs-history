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

#ifndef nsMailboxUrl_h__
#define nsMailboxUrl_h__

#include "nsIMailboxUrl.h"
#include "nsMsgMailNewsUrl.h"
#include "nsFileSpec.h"
#include "nsIFileSpec.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

class nsMailboxUrl : public nsIMailboxUrl, public nsMsgMailNewsUrl, public nsIMsgMessageUrl
{
public:
	// nsIURI over-ride...
	NS_IMETHOD SetSpec(const char * aSpec);

	// from nsIMailboxUrl:
	NS_IMETHOD GetMessageHeader(nsIMsgDBHdr ** aMsgHdr);
	NS_IMETHOD SetMailboxParser(nsIStreamListener * aConsumer);
	NS_IMETHOD GetMailboxParser(nsIStreamListener ** aConsumer);
	NS_IMETHOD SetMailboxCopyHandler(nsIStreamListener *  aConsumer);
	NS_IMETHOD GetMailboxCopyHandler(nsIStreamListener ** aConsumer);
	
	NS_IMETHOD GetFileSpec(nsFileSpec ** aFilePath);
	NS_IMETHOD GetMessageKey(nsMsgKey* aMessageKey);
    NS_IMETHOD GetMessageSize(PRUint32 *aMessageSize);
	NS_IMETHOD SetMessageSize(PRUint32 aMessageSize);
	NS_IMPL_CLASS_GETSET(MailboxAction, nsMailboxAction, m_mailboxAction);

    // nsMailboxUrl
    nsMailboxUrl();
	virtual ~nsMailboxUrl();
    NS_DECL_NSIMSGMESSAGEURL
    NS_DECL_ISUPPORTS_INHERITED


protected:
	// protocol specific code to parse a url...
    virtual nsresult ParseUrl(const nsString& aSpec);
	virtual const char * GetUserName() { return nsnull;}

	// mailboxurl specific state
	nsCOMPtr<nsIStreamListener> m_mailboxParser;
	nsCOMPtr<nsIStreamListener> m_mailboxCopyHandler;

	nsMailboxAction m_mailboxAction; // the action this url represents...parse mailbox, display messages, etc.
	nsFileSpec	*m_filePath; 
	char		*m_messageID;
	PRUint32	m_messageSize;
	nsMsgKey	m_messageKey;
	nsXPIDLCString m_file;

	// used by save message to disk
	nsCOMPtr<nsIFileSpec> m_messageFileSpec;
    PRBool                m_addDummyEnvelope;
	nsresult ParseSearchPart();
};

#endif // nsMailboxUrl_h__
