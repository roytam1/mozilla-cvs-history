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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef _nsMsgHdr_H
#define _nsMsgHdr_H

#include "nsIMsgHdr.h"
#include "nsString.h"
#include "MailNewsTypes.h"
#include "xp.h"
#include "mdb.h"

class nsMsgDatabase;
class nsCString;

class nsMsgHdr : public nsIMsgDBHdr {
public:

	friend class nsMsgDatabase;
    ////////////////////////////////////////////////////////////////////////////
    // nsIMsghdr methods:
    NS_IMETHOD GetProperty(const char *propertyName, nsString &resultProperty);
    NS_IMETHOD SetProperty(const char *propertyName, nsString &propertyStr);
    NS_IMETHOD GetUint32Property(const char *propertyName, PRUint32 *pResult);
    NS_IMETHOD SetUint32Property(const char *propertyName, PRUint32 propertyVal);
    NS_IMETHOD GetNumReferences(PRUint16 *result);
    NS_IMETHOD GetStringReference(PRInt32 refNum, nsCString &resultReference);
    NS_IMETHOD GetDate(PRTime *result);
    NS_IMETHOD SetDate(PRTime date);
    NS_IMETHOD SetMessageId(const char *messageId);
    NS_IMETHOD SetReferences(const char *references);
    NS_IMETHOD SetCcList(const char *ccList);
    NS_IMETHOD SetRecipients(const char *recipients, PRBool recipientsIsNewsgroup);
	NS_IMETHOD SetRecipientsArray(const char *names, const char *addresses, PRUint32 numAddresses);
    NS_IMETHOD SetCCListArray(const char *names, const char *addresses, PRUint32 numAddresses);
    NS_IMETHOD SetAuthor(const char *author);
    NS_IMETHOD SetSubject(const char *subject);
    NS_IMETHOD SetStatusOffset(PRUint32 statusOffset);

	NS_IMETHOD GetAuthor(nsString *resultAuthor);
	NS_IMETHOD GetSubject(nsString *resultSubject);
	NS_IMETHOD GetRecipients(nsString *resultRecipients);
	NS_IMETHOD GetMessageId(char **resultMessageId);

	NS_IMETHOD GetMime2DecodedAuthor(nsString *resultAuthor);
	NS_IMETHOD GetMime2DecodedSubject(nsString *resultSubject);
	NS_IMETHOD GetMime2DecodedRecipients(nsString *resultRecipients);

	NS_IMETHOD GetAuthorCollationKey(nsString *resultAuthor);
	NS_IMETHOD GetSubjectCollationKey(nsString *resultSubject);
	NS_IMETHOD GetRecipientsCollationKey(nsString *resultRecipients);

	NS_IMETHOD GetCcList(char **ccList);
    // flag handling routines
    NS_IMETHOD GetFlags(PRUint32 *result);
    NS_IMETHOD SetFlags(PRUint32 flags);
    NS_IMETHOD OrFlags(PRUint32 flags, PRUint32 *result);
    NS_IMETHOD AndFlags(PRUint32 flags, PRUint32 *result);

	// Mark message routines
	NS_IMETHOD MarkRead(PRBool bRead);
	NS_IMETHOD MarkFlagged(PRBool bRead);

    NS_IMETHOD GetMessageKey(nsMsgKey *result);
    NS_IMETHOD GetThreadId(nsMsgKey *result);
    NS_IMETHOD SetThreadId(nsMsgKey inKey);
    NS_IMETHOD SetMessageKey(nsMsgKey inKey);
    NS_IMETHOD GetMessageSize(PRUint32 *result);
    NS_IMETHOD SetMessageSize(PRUint32 messageSize);
    NS_IMETHOD GetLineCount(PRUint32 *result);
    NS_IMETHOD SetLineCount(PRUint32 lineCount);
    NS_IMETHOD SetPriority(nsMsgPriority priority);
    NS_IMETHOD SetPriorityString(const char *priority);
    NS_IMETHOD GetMessageOffset(PRUint32 *result);
    NS_IMETHOD GetStatusOffset(PRUint32 *result); 
	NS_IMETHOD GetCharSet(nsString *result);
	NS_IMETHOD GetPriority(nsMsgPriority *msgPriority);
    NS_IMETHOD GetThreadParent(nsMsgKey *result);
    NS_IMETHOD SetThreadParent(nsMsgKey inKey);
    ////////////////////////////////////////////////////////////////////////////
    // nsMsgHdr methods:
    nsMsgHdr(nsMsgDatabase *db, nsIMdbRow *dbRow);
    virtual				~nsMsgHdr();

    void				Init();
	virtual nsresult	InitCachedValues();
	virtual nsresult	InitFlags();

    NS_DECL_ISUPPORTS

    nsIMdbRow		*GetMDBRow() {return m_mdbRow;}
	PRBool			IsParentOf(nsIMsgDBHdr *possibleChild);
protected:
    nsresult	SetStringColumn(const char *str, mdb_token token);
    nsresult	SetUInt32Column(PRUint32 value, mdb_token token);
    nsresult	GetUInt32Column(mdb_token token, PRUint32 *pvalue);

	// reference and threading stuff.
	const char*	GetNextReference(const char *startNextRef, nsCString &reference);
	const char* GetPrevReference(const char *prevRef, nsCString &reference);

    nsMsgKey	m_threadId; 
    nsMsgKey	m_messageKey; 	//news: article number, mail mbox offset, imap uid...
	nsMsgKey	m_threadParent;	// message this is a reply to, in thread.
    PRTime  		m_date;                         
    PRUint32		m_messageSize;	// lines for news articles, bytes for mail messages
    PRUint32		m_statusOffset;	// offset in a local mail message of the mozilla status hdr
    PRUint32		m_flags;
    PRUint16		m_numReferences;	// x-ref header for threading
    PRInt16			m_csID;			// cs id of message
	nsCString		m_charSet;		// OK, charset of headers, since cs id's aren't supported.
	nsCString		m_references;
    nsMsgPriority	m_priority;

    // nsMsgHdrs will have to know what db and row they belong to, since they are really
    // just a wrapper around the msg row in the mdb. This could cause problems,
    // though I hope not.
    nsMsgDatabase	*m_mdb;
    nsIMdbRow		*m_mdbRow;
	PRUint32		m_initedValues;
};

#endif

