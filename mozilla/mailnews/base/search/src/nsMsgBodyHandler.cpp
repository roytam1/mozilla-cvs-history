/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "msgCore.h"
#include "nsMsgSearchCore.h"
#include "nsMsgUtils.h"
#include "nsMsgBodyHandler.h"
#include "nsMsgSearchTerm.h"

nsMsgBodyHandler::nsMsgBodyHandler (nsMsgSearchScopeTerm * scope, PRUint32 offset, PRUint32 numLines, nsIMsgDBHdr* msg, nsIMsgDatabase * db)
{
	m_scope = scope;
	m_localFileOffset = offset;
	m_numLocalLines = numLines;
	m_msgHdr = msg;
	m_db = db;

	// the following are variables used when the body handler is handling stuff from filters....through this constructor, that is not the
	// case so we set them to NULL.
	m_headers = NULL;
	m_headersSize = 0;
	m_Filtering = PR_FALSE; // make sure we set this before we call initialize...

	Initialize();  // common initialization stuff
	OpenLocalFolder();	    
}

nsMsgBodyHandler::nsMsgBodyHandler(nsMsgSearchScopeTerm * scope,
                                   PRUint32 offset, PRUint32 numLines,
                                   nsIMsgDBHdr* msg, nsIMsgDatabase* db,
                                   const char * headers, PRUint32 headersSize,
                                   PRBool Filtering)
{
	m_scope = scope;
	m_localFileOffset = offset;
	m_numLocalLines = numLines;
	m_msgHdr = msg;
	m_db = db;
	m_headersSize = headersSize;
	m_Filtering = Filtering;

	Initialize();

	if (m_Filtering)
		m_headers = headers;
	else
		OpenLocalFolder();  // if nothing else applies, then we must be a POP folder file
}

void nsMsgBodyHandler::Initialize()
// common initialization code regardless of what body type we are handling...
{
	// Default transformations for local message search and MAPI access
	m_stripHeaders = PR_TRUE;
	m_stripHtml = PR_TRUE;
	m_messageIsHtml = PR_FALSE;
	m_passedHeaders = PR_FALSE;

	// set our offsets to 0 since we haven't handled any bytes yet...
	m_IMAPMessageOffset = 0;
	m_NewsArticleOffset = 0;
	m_headerBytesRead = 0;

}

nsMsgBodyHandler::~nsMsgBodyHandler()
{
	if (m_scope->m_fileStream)
	{
		delete m_scope->m_fileStream;
		m_scope->m_fileStream = NULL;
	}
}

		

PRInt32 nsMsgBodyHandler::GetNextLine (char * buf, int bufSize)
{
	PRInt32 length = 0;
	PRBool eatThisLine = PR_FALSE;

	do {
		// first, handle the filtering case...this is easy....
		if (m_Filtering)
			length = GetNextFilterLine(buf, bufSize);
		else
		{
			// 3 cases: Offline IMAP, POP, or we are dealing with a news message....
			if (m_db)
			{
				length = GetNextLocalLine (buf, bufSize); // (2) POP
			}
		}

		if (length > 0)
			length = ApplyTransformations (buf, length, eatThisLine);
	} while (eatThisLine && length);  // if we hit eof, make sure we break out of this loop. Bug #:
	return length;  
}
void nsMsgBodyHandler::OpenLocalFolder()
{
	if (!m_scope->m_fileStream)
	{
		nsCOMPtr <nsIFileSpec> fileSpec;
		nsresult rv = m_scope->GetMailPath(getter_AddRefs(fileSpec));
		if (NS_SUCCEEDED(rv) && fileSpec)
		{
			nsFileSpec path;
			fileSpec->GetFileSpec(&path);
			m_scope->m_fileStream = new nsIOFileStream(path);
		}
	}
	if (m_scope->m_fileStream)
		m_scope->m_fileStream->seek(m_localFileOffset); 
}


PRInt32 nsMsgBodyHandler::GetNextFilterLine(char * buf, PRUint32 bufSize)
{
	// m_nextHdr always points to the next header in the list....the list is NULL terminated...
	PRUint32 numBytesCopied = 0;
	if (m_headersSize > 0)
	{
		// #mscott. Ugly hack! filter headers list have CRs & LFs inside the NULL delimited list of header
		// strings. It is possible to have: To NULL CR LF From. We want to skip over these CR/LFs if they start
		// at the beginning of what we think is another header.
		while ((m_headers[0] == CR || m_headers[0] == LF || m_headers[0] == ' ' || m_headers[0] == '\0') && m_headersSize > 0)
		{
			m_headers++;  // skip over these chars...
			m_headersSize--;
		}

		if (m_headersSize > 0)
		{
			numBytesCopied = nsCRT::strlen(m_headers)+1 /* + 1 to include NULL */ < bufSize ? nsCRT::strlen(m_headers)+1 : (PRInt32) bufSize;
			nsCRT::memcpy(buf, m_headers, numBytesCopied);
			m_headers += numBytesCopied;  
			// be careful...m_headersSize is unsigned. Don't let it go negative or we overflow to 2^32....*yikes*	
			if (m_headersSize < numBytesCopied)
				m_headersSize = 0;
			else
				m_headersSize -= numBytesCopied;  // update # bytes we have read from the headers list

			return (PRInt32) numBytesCopied;
		}
	}
	return 0;
}


PRInt32 nsMsgBodyHandler::GetNextLocalLine(char * buf, int bufSize)
// returns number of bytes copied
{
	char * line = NULL;
	if (m_numLocalLines)
	{
		if (m_passedHeaders)
			m_numLocalLines--; // the line count is only for body lines
		// do we need to check the return value here?
		if (m_scope->m_fileStream->readline(buf, bufSize))
			return nsCRT::strlen(buf);
	}

	return 0;
}



PRInt32 nsMsgBodyHandler::ApplyTransformations (char *buf, PRInt32 length, PRBool &eatThisLine)
{
	PRInt32 newLength = length;
	eatThisLine = PR_FALSE;

	if (!m_passedHeaders)	// buf is a line from the message headers
	{
		if (m_stripHeaders)
			eatThisLine = PR_TRUE;

		if (!nsCRT::strncasecmp(buf, "Content-Type:", 13) && PL_strcasestr (buf, "text/html"))
			m_messageIsHtml = PR_TRUE;

		m_passedHeaders = EMPTY_MESSAGE_LINE(buf);
	}
	else	// buf is a line from the message body
	{
		if (m_stripHtml && m_messageIsHtml)
		{
			StripHtml (buf);
			newLength = nsCRT::strlen (buf);
		}
	}

	return newLength;
}


void nsMsgBodyHandler::StripHtml (char *pBufInOut)
{
	char *pBuf = (char*) PR_Malloc (nsCRT::strlen(pBufInOut) + 1);
	if (pBuf)
	{
		char *pWalk = pBuf;
		char *pWalkInOut = pBufInOut;
		PRBool inTag = PR_FALSE;
		while (*pWalkInOut) // throw away everything inside < >
		{
			if (!inTag)
				if (*pWalkInOut == '<')
					inTag = PR_TRUE;
				else
					*pWalk++ = *pWalkInOut;
			else
				if (*pWalkInOut == '>')
					inTag = PR_FALSE;
			pWalkInOut++;
		}
		*pWalk = 0; // null terminator

		// copy the temp buffer back to the real one
		pWalk = pBuf;
		pWalkInOut = pBufInOut;
		while (*pWalk)
			*pWalkInOut++ = *pWalk++;
		*pWalkInOut = *pWalk; // null terminator
		PR_Free (pBuf);
	}
}


