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

#ifndef nsMailboxProtocol_h___
#define nsMailboxProtocol_h___

#include "nsMsgProtocol.h"
#include "nsCOMPtr.h"
#include "nsIFileSpec.h"
#include "nsIChannel.h"

#include "nsIOutputStream.h"
#include "nsIMailboxUrl.h"

#include "nsIWebShell.h"  // mscott - this dependency should only be temporary!

// State Flags (Note, I use the word state in terms of storing 
// state information about the connection (authentication, have we sent
// commands, etc. I do not intend it to refer to protocol state)

#define MAILBOX_PAUSE_FOR_READ			0x00000001  /* should we pause for the next read */
#define MAILBOX_MSG_PARSE_FIRST_LINE    0x00000002 /* have we read in the first line of the msg */

/* states of the machine
 */
typedef enum _MailboxStatesEnum {
	MAILBOX_READ_FOLDER,
	MAILBOX_FINISH_OPEN_FOLDER,
	MAILBOX_OPEN_MESSAGE,
	MAILBOX_OPEN_STREAM,
	MAILBOX_READ_MESSAGE,
	MAILBOX_COMPRESS_FOLDER,
	MAILBOX_FINISH_COMPRESS_FOLDER,
	MAILBOX_BACKGROUND,
	MAILBOX_NULL,
	MAILBOX_NULL2,
	MAILBOX_DELIVER_QUEUED,
	MAILBOX_FINISH_DELIVER_QUEUED,
	MAILBOX_DONE,
	MAILBOX_ERROR_DONE,
	MAILBOX_FREE,
	MAILBOX_COPY_MESSAGES,
	MAILBOX_FINISH_COPY_MESSAGES
} MailboxStatesEnum;

class nsMsgLineStreamBuffer;

class nsMailboxProtocol : public nsMsgProtocol
{
public:
	// Creating a protocol instance requires the URL which needs to be run AND it requires
	// a transport layer. 
	nsMailboxProtocol(nsIURI * aURL, nsIURI* originalURI);
	virtual ~nsMailboxProtocol();

	// the consumer of the url might be something like an nsIWebShell....
	virtual nsresult LoadUrl(nsIURI * aURL, nsISupports * aConsumer);

	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIStreamListener interface 
	////////////////////////////////////////////////////////////////////////////////////////

	NS_IMETHOD OnStartRequest(nsIChannel * aChannel, nsISupports *ctxt);
	NS_IMETHOD OnStopRequest(nsIChannel * aChannel, nsISupports *ctxt, nsresult aStatus, const PRUnichar *aMsg);

	////////////////////////////////////////////////////////////////////////////////////////
	// End of nsIStreamListenerSupport
	////////////////////////////////////////////////////////////////////////////////////////
	NS_IMETHOD Cancel();  // handle stop button

private:
	nsCOMPtr<nsIMailboxUrl>	m_runningUrl; // the nsIMailboxURL that is currently running
	nsMailboxAction m_mailboxAction; // current mailbox action associated with this connnection...
	PRInt32			m_originalContentLength; /* the content length at the time of calling graph progress */

	// Event sink handles
	nsCOMPtr<nsIStreamListener> m_mailboxParser;

	// Local state for the current operation
	nsMsgLineStreamBuffer   * m_lineStreamBuffer; // used to efficiently extract lines from the incoming data stream

	// Generic state information -- What state are we in? What state do we want to go to
	// after the next response? What was the last response code? etc. 
	MailboxStatesEnum  m_nextState;
    MailboxStatesEnum  m_initialState;

	PRUint32	m_messageID;

	nsCOMPtr<nsIFileSpec> m_tempMessageFile;

	virtual nsresult ProcessProtocolState(nsIURI * url, nsIInputStream * inputStream, 
									      PRUint32 sourceOffset, PRUint32 length);
	virtual nsresult CloseSocket();

	// initialization function given a new url and transport layer
	void Initialize(nsIURI * aURL);
	PRInt32 SetupMessageExtraction();

	////////////////////////////////////////////////////////////////////////////////////////
	// Protocol Methods --> This protocol is state driven so each protocol method is 
	//						designed to re-act to the current "state". I've attempted to 
	//						group them together based on functionality. 
	////////////////////////////////////////////////////////////////////////////////////////

	// When parsing a mailbox folder in chunks, this protocol state reads in the current chunk
	// and forwards it to the mailbox parser. 
	PRInt32 ReadFolderResponse(nsIInputStream * inputStream, PRUint32 sourceOffset, PRUint32 length);
	PRInt32 ReadMessageResponse(nsIInputStream * inputStream, PRUint32 sourceOffset, PRUint32 length);
	PRInt32 DoneReadingMessage();

	////////////////////////////////////////////////////////////////////////////////////////
	// End of Protocol Methods
	////////////////////////////////////////////////////////////////////////////////////////
};

#endif  // nsMailboxProtocol_h___
