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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsParseMailbox_H
#define nsParseMailbox_H

#include "nsIURI.h"
#include "nsIMsgParseMailMsgState.h"
#include "nsMsgKeyArray.h"
#include "nsVoidArray.h"
#include "nsIStreamListener.h"
#include "nsMsgLineBuffer.h"
#include "nsIMsgHeaderParser.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgHdr.h"
#include "nsIMsgStatusFeedback.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsTime.h"


#include "nsIMsgFilterList.h"
#include "nsIMsgFilterHitNotify.h"

class nsFileSpec;
class nsByteArray;
class nsOutputFileStream;
class nsIOFileStream;
class nsInputFileStream;
class nsIMsgFilter;
class MSG_FolderInfoMail;
class nsIMsgFilterList;
class nsIFolder;

NS_BEGIN_EXTERN_C

nsresult NS_NewParseMailMessageState(const nsIID& iid, void **result);

NS_END_EXTERN_C

/* Used for the various things that parse RFC822 headers...
 */
typedef struct message_header
{
  const char *value; /* The contents of a header (after ": ") */
  PRInt32 length;      /* The length of the data (it is not NULL-terminated.) */
} message_header;

// This object maintains the parse state for a single mail message.
class nsParseMailMessageState : public nsIMsgParseMailMsgState
{
public:
	NS_DECL_ISUPPORTS
					nsParseMailMessageState();
	virtual			~nsParseMailMessageState();
	
	// nsIMsgParseMailMsgState support
	NS_IMETHOD SetMailDB(nsIMsgDatabase * aDatabase);
	NS_IMETHOD Clear();
	NS_IMETHOD SetState(nsMailboxParseState aState);
	NS_IMETHOD SetEnvelopePos(PRUint32 aEnvelopePos);
	NS_IMETHOD ParseAFolderLine(const char *line, PRUint32 lineLength);
	NS_IMETHOD GetNewMsgHdr(nsIMsgDBHdr ** aMsgHeader);
	NS_IMETHOD FinishHeader();
	NS_IMETHOD GetAllHeaders(char **headers, PRInt32 *headersSize);
	void			Init(PRUint32 fileposition);
	virtual PRInt32	ParseFolderLine(const char *line, PRUint32 lineLength);
	virtual int		StartNewEnvelope(const char *line, PRUint32 lineLength);
	int				ParseHeaders();
	int				FinalizeHeaders();
	int				ParseEnvelope (const char *line, PRUint32 line_size);
	int				InternSubject (struct message_header *header);
	nsresult		InternRfc822 (struct message_header *header, 
								char **ret_name);

	static PRBool	IsEnvelopeLine(const char *buf, PRInt32 buf_size);
	static PRBool	msg_StripRE(const char **stringP, PRUint32 *lengthP);
	static int		msg_UnHex(char C); 

	nsCOMPtr<nsIMsgHeaderParser> m_HeaderAddressParser;

	nsCOMPtr<nsIMsgDBHdr>		m_newMsgHdr;		/* current message header we're building */
	nsCOMPtr<nsIMsgDatabase>	m_mailDB;

	nsMailboxParseState m_state;
	PRUint32			m_position;
	PRUint32			m_envelope_pos;
	PRUint32			m_headerstartpos;

	nsByteArray		m_headers;

	nsByteArray		m_envelope;

	struct message_header m_message_id;
	struct message_header m_references;
	struct message_header m_date;
	struct message_header m_from;
	struct message_header m_sender;
	struct message_header m_newsgroups;
	struct message_header m_subject;
	struct message_header m_status;
	struct message_header m_mozstatus;
	struct message_header m_mozstatus2;

	// Support for having multiple To or CC header lines in a message
	nsVoidArray m_toList;
	nsVoidArray m_ccList;
	struct message_header *GetNextHeaderInAggregate (nsVoidArray &list);
	void GetAggregateHeader (nsVoidArray &list, struct message_header *);
	void ClearAggregateHeader (nsVoidArray &list);

	struct message_header m_envelope_from;
	struct message_header m_envelope_date;
	struct message_header m_priority;
	// Mdn support
	struct message_header m_mdn_original_recipient;
	struct message_header m_return_path;
	struct message_header m_mdn_dnt; /* MDN Disposition-Notification-To: header */

	PRUint16			m_body_lines;
	
	PRBool			m_IgnoreXMozillaStatus;
};

// this should go in some utility class.
inline int	nsParseMailMessageState::msg_UnHex(char C)
{
	return ((C >= '0' && C <= '9') ? C - '0' :
		((C >= 'A' && C <= 'F') ? C - 'A' + 10 :
		 ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)));
}

// This class is part of the mailbox parsing state machine 
class nsMsgMailboxParser : public nsIStreamListener, public nsParseMailMessageState, public nsMsgLineBuffer, public nsMsgLineBufferHandler 
{
public:
	nsMsgMailboxParser();
	virtual ~nsMsgMailboxParser();

	PRBool  IsRunningUrl() { return m_urlInProgress;} // returns true if we are currently running a url and false otherwise...
	NS_DECL_ISUPPORTS_INHERITED

	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIStreamListener interface 
	////////////////////////////////////////////////////////////////////////////////////////
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER

	void			SetDB (nsIMsgDatabase *mailDB) {m_mailDB = dont_QueryInterface(mailDB); }
	char			*GetMailboxName() {return m_mailboxName;}

	void			SetIncrementalUpdate(PRBool update) {m_updateAsWeGo = update;}
	void			SetIgnoreNonMailFolder(PRBool ignoreNonMailFolder) {m_ignoreNonMailFolder = ignoreNonMailFolder;}
	PRBool			GetIsRealMailFolder() {return m_isRealMailFolder;}

	// message socket libnet callbacks, which come through folder pane
	virtual int ProcessMailboxInputStream(nsIURI* aURL, nsIInputStream *aIStream, PRUint32 aLength);

	virtual void	DoneParsingFolder();
	virtual void	AbortNewHeader();
	virtual PRInt32 HandleLine(char *line, PRUint32 line_length);

	void			UpdateDBFolderInfo();
	void			UpdateDBFolderInfo(nsIMsgDatabase *mailDB, const char *mailboxName);
	void			UpdateStatusText (PRUint32 stringID);

	// Update the progress bar based on what we know.
	virtual void    UpdateProgressPercent ();

protected:
	nsCOMPtr<nsIMsgStatusFeedback> m_statusFeedback;

	virtual PRInt32			PublishMsgHeader();
	virtual void			FolderTypeSpecificTweakMsgHeader(nsIMsgDBHdr *tweakMe);
	void					FreeBuffers();

	// data
	nsXPIDLCString   m_folderName;
	char			*m_mailboxName;
	nsByteArray		m_inputStream;
	PRInt32			m_obuffer_size;
	char			*m_obuffer;
	PRUint32			m_ibuffer_fp;
	char			*m_ibuffer;
	PRUint32			m_ibuffer_size;
	PRInt32			m_graph_progress_total;
	PRInt32			m_graph_progress_received;
	PRBool			m_updateAsWeGo;
	PRBool			m_parsingDone;
	PRBool			m_ignoreNonMailFolder;
	PRBool			m_isRealMailFolder;
	nsTime			m_startTime;
private:
		// the following flag is used to determine when a url is currently being run. It is cleared on calls
	// to ::StopBinding and it is set whenever we call Load on a url
	PRBool	m_urlInProgress;	

};

class nsParseNewMailState : public nsMsgMailboxParser 
, public nsIMsgFilterHitNotify
{
public:
	nsParseNewMailState();
	virtual ~nsParseNewMailState();
	NS_DECL_ISUPPORTS_INHERITED
    nsresult Init(nsIFolder *rootFolder, nsFileSpec &folder, nsIOFileStream *inboxFileStream);

	virtual void	DoneParsingFolder();
	virtual void	SetUsingTempDB(PRBool usingTempDB, char *tmpDBName);

	void DisableFilters() {m_disableFilters = PR_TRUE;}

#ifdef DOING_JSFILTERS
	// from jsmsg.cpp
	friend void JSMailFilter_MoveMessage(ParseNewMailState *state, 
										 nsIMsgDBHdr *msgHdr, 
										 nsMailDatabase *mailDB, 
										 const char *folder, 
										 nsIMsgFilter *filter,
										 PRBool *pMoved);
#endif
	// nsIMsgFilterHitNotification method(s)
	NS_IMETHOD ApplyFilterHit(nsIMsgFilter *filter, PRBool *applyMore);

	nsOutputFileStream *GetLogFile();
protected:
	virtual PRInt32	PublishMsgHeader();
	char				*m_tmpdbName;				// Temporary filename of new database
	PRBool				m_usingTempDB;
	virtual void	ApplyFilters(PRBool *pMoved);
	virtual nsresult GetTrashFolder(nsIMsgFolder **pTrashFolder);
	virtual nsresult	MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr, 
											   nsIMsgDatabase *sourceDB, 
											   char *destFolder,
											   nsIMsgFilter *filter);
	virtual	int			MarkFilteredMessageRead(nsIMsgDBHdr *msgHdr);
			void		LogRuleHit(nsIMsgFilter *filter, nsIMsgDBHdr *msgHdr);
	nsCOMPtr <nsIMsgFilterList> m_filterList;
	nsCOMPtr <nsIFolder> m_rootFolder;
	nsOutputFileStream	*m_logFile;
	nsIOFileStream		*m_inboxFileStream;
	nsFileSpec			m_inboxFileSpec;
	PRBool				m_disableFilters;
	PRBool				m_msgMovedByFilter;
};

#ifdef IMAP_NEW_MAIL_HANDLED


class ParseIMAPMailboxState : public ParseNewMailState 
{
public:
	ParseIMAPMailboxState(MSG_IMAPHost *host, MSG_FolderInfoMail *folder, MSG_UrlQueue *urlQueue, TImapFlagAndUidState *flagStateAdopted);
	virtual ~ParseIMAPMailboxState();
	
	// close the db
	virtual void			DoneParsingFolder();
	
	virtual void			SetPublishUID(PRInt32 uid);
	virtual void			SetPublishByteLength(PRUint32 byteLength);
	virtual void			SetNextSequenceNum(PRInt32 seqNum);
	
	const IDArray 			&GetBodyKeys() { return fFetchBodyKeys; }
	MSG_UrlQueue	*GetFilterUrlQueue() {return fUrlQueue;}
protected:
	virtual PRInt32			PublishMsgHeader();
	virtual void			FolderTypeSpecificTweakMsgHeader(nsIMsgDBHdr *tweakMe);
	virtual void			ApplyFilters(PRBool *pMoved);
	
	virtual MSG_FolderInfoMail *GetTrashFolder();
	virtual nsresult		MoveIncorporatedMessage(nsIMsgDBHdr *mailHdr, 
											   nsIMsgDatabase *sourceDB, 
											   char *destFolder,
											   nsIMsgFilter *filter);
	virtual	int				MarkFilteredMessageRead(nsIMsgDBHdr *msgHdr);
private:
	PRBool fParsingInbox;
	PRBool	fB2HaveWarnedUserOfOfflineFiltertarget;
	PRInt32 	fNextMessagePublishUID;
	PRUint32	fNextMessageByteLength;
	MSG_UrlQueue *fUrlQueue;
	TImapFlagAndUidState *fFlagState;
	nsMsgKeyArray fFetchBodyKeys;
	PRInt32	fNextSequenceNum;
	MSG_FolderInfoContainer *m_imapContainer;
	MSG_IMAPHost	*m_host;
};

// If m_out_file is set, this parser will also write out the message,
// adding a mozilla status line if one is not present.
class ParseOutgoingMessage : public ParseMailMessageState
{
public:
	ParseOutgoingMessage();
	virtual ~ParseOutgoingMessage();
	void			SetOutFile(XP_File out_file) {m_out_file = out_file;}
	XP_File			GetOutFile() {return m_out_file;}
	void			SetWriteMozillaStatus(PRBool writeMozStatus) 
						{m_writeMozillaStatus = writeMozStatus;}
	virtual int		StartNewEnvelope(const char *line, PRUint32 lineLength);
	virtual void	FinishHeader();
	virtual PRInt32	ParseFolderLine(const char *line, PRUint32 lineLength);
	virtual PRInt32	ParseBlock(const char *block, PRUint32 lineLength);
	virtual void	Clear();
	void			AdvanceOutPosition(PRUint32 amountToAdvance);
	void			SetWriteToOutFile(PRBool writeToOutFile) {m_writeToOutFile = writeToOutFile;}

	void			FlushOutputBuffer();
	PRUint32			m_bytes_written;
protected:
	static PRInt32	LineBufferCallback(char *line, PRUint32 lineLength, void *closure);
	PRBool			m_wroteXMozillaStatus;
	PRBool			m_writeMozillaStatus;
	PRBool			m_writeToOutFile;
	PRBool			m_lastBodyLineEmpty;
	XP_File			m_out_file;
	PRUint32			m_ouputBufferSize;
	char			*m_outputBuffer;
	PRUint32			m_outputBufferIndex;
};

#endif /* #ifdef NEW_MAIL_HANDLED */
 

#endif
