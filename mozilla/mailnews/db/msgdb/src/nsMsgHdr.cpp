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
#include "nsMsgHdr.h"
#include "nsMsgDatabase.h"

// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

NS_IMPL_ISUPPORTS_INHERITED(nsMsgHdr, nsRDFResource, nsIMessage)

nsMsgHdr::nsMsgHdr(nsMsgDatabase *db, nsIMdbRow *dbRow)
    : nsRDFResource()
{
    NS_INIT_REFCNT();
	m_mdb = db;
	if(m_mdb)
		m_mdb->AddRef();
	Init();
	m_mdbRow = dbRow;
}

void nsMsgHdr::Init()
{
	m_statusOffset = -1;
	m_messageKey = nsMsgKey_None;
	m_date = 0;
	m_messageSize = 0;
	m_csID = 0;
	m_flags = 0;
	m_mdbRow = NULL;
}

nsMsgHdr::~nsMsgHdr()
{
	if (m_mdbRow)
	{
		if (m_mdb)
		{	// presumably, acquiring a row increments strong ref count
			m_mdbRow->CutStrongRef(m_mdb->GetEnv());
			m_mdb->Release();
		}
	}
}

#if 0
NS_IMETHODIMP nsMsgHdr::GetMessageSize(PRUint32 *result)
{
    *result = m_messageSize;
    return NS_OK;
}
#endif

NS_IMETHODIMP nsMsgHdr::GetMessageKey(nsMsgKey *result)
{
	if (m_messageKey == nsMsgKey_None && m_mdbRow != NULL)
	{
		mdbOid outOid;
		if (m_mdbRow->GetOid(m_mdb->GetEnv(), &outOid) == NS_OK)
			m_messageKey = outOid.mOid_Id;

	}
	*result = m_messageKey;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetThreadId(nsMsgKey *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgHdr::SetMessageKey(nsMsgKey value)
{
		m_messageKey = value;
		return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetFlags(PRUint32 *result)
{
    *result = m_flags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::SetFlags(PRUint32 flags)
{
	m_flags = flags;
	SetUInt32Column(m_flags, m_mdb->m_flagsColumnToken);
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::OrFlags(PRUint32 flags, PRUint32 *result)
{
	if ((m_flags & flags) != flags)
		SetFlags (m_flags | flags);
	*result = m_flags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::AndFlags(PRUint32 flags, PRUint32 *result)
{
	if ((m_flags & flags) != m_flags)
		SetFlags (m_flags & flags);
	*result = m_flags;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetProperty(const char *propertyName, nsString &resultProperty)
{
	nsresult err = NS_OK;
	mdb_token	property_token;

	err = m_mdb->GetStore()->StringToToken(m_mdb->GetEnv(),  propertyName, &property_token);
	if (err == NS_OK)
		err = m_mdb->RowCellColumnTonsString(GetMDBRow(), property_token, resultProperty);

	return err;
}

NS_IMETHODIMP nsMsgHdr::SetProperty(const char *propertyName, nsString &propertyStr)
{
	nsresult err = NS_OK;
	mdb_token	property_token;

	err = m_mdb->GetStore()->StringToToken(m_mdb->GetEnv(),  propertyName, &property_token);
	if (err == NS_OK)
	{
		struct mdbYarn yarn;

		yarn.mYarn_Grow = NULL;
		err = m_mdbRow->AddColumn(m_mdb->GetEnv(), property_token, m_mdb->nsStringToYarn(&yarn, &propertyStr));
		delete[] yarn.mYarn_Buf;	// won't need this when we have nsCString
	}
	return err;
}

NS_IMETHODIMP nsMsgHdr::GetUint32Property(const char *propertyName, PRUint32 *pResult)
{
	nsresult err = NS_OK;
	mdb_token	property_token;

	err = m_mdb->GetStore()->StringToToken(m_mdb->GetEnv(),  propertyName, &property_token);
	if (err == NS_OK)
		err = m_mdb->RowCellColumnToUInt32(GetMDBRow(), property_token, pResult);

	return err;
}

NS_IMETHODIMP nsMsgHdr::SetUint32Property(const char *propertyName, PRUint32 value)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsMsgHdr::GetNumReferences(PRUint16 *result)
{
    *result = 0;
	return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetStringReference(PRInt32 refNum, nsString &resultReference)
{
	nsresult err = NS_OK;
	return err;
}

NS_IMETHODIMP nsMsgHdr::GetDate(time_t *result) 
{
	*result = m_date;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::SetMessageId(const char *messageId)
{
	return SetStringColumn(messageId, m_mdb->m_messageIdColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetSubject(const char *subject)
{
	return SetStringColumn(subject, m_mdb->m_subjectColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetAuthor(const char *author)
{
	return SetStringColumn(author, m_mdb->m_senderColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetReferences(const char *references)
{
	return SetStringColumn(references, m_mdb->m_referencesColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetRecipients(const char *recipients, PRBool rfc822 /* = PR_TRUE */)
{
	// need to put in rfc822 address parsing code here (or make caller do it...)
	return SetStringColumn(recipients, m_mdb->m_recipientsColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetRecipientsArray(const char *names, const char *addresses, PRUint32 numAddresses)
{
	nsresult ret;
	const char *curName = names;
	const char *curAddress = addresses;
	nsString	allRecipients;

	for (int i = 0; i < numAddresses; i++)
	{
		if (i > 0)
			allRecipients += ", ";

		if (strlen(curName))
		{
			allRecipients += curName;
			allRecipients += ' ';
		}

		if (strlen(curAddress))
		{
			if (strlen(curName))
				allRecipients += '<';
			else
				allRecipients = "<";
			allRecipients += curAddress;
			allRecipients += '>';
		}
		curName += strlen(curName) + 1;
		curAddress += strlen(curAddress) + 1;
	}
	char *cstringRecipients = allRecipients.ToNewCString();
	ret = SetRecipients(cstringRecipients, PR_TRUE);
	delete [] cstringRecipients;
	return ret;
}

NS_IMETHODIMP nsMsgHdr::SetCCList(const char *ccList)
{
	return SetStringColumn(ccList, m_mdb->m_ccListColumnToken);
}

// ###should make helper routine that takes column token!
NS_IMETHODIMP nsMsgHdr::SetCCListArray(const char *names, const char *addresses, PRUint32 numAddresses)
{
	nsresult ret;
	const char *curName = names;
	const char *curAddress = addresses;
	nsString	allRecipients;

	for (int i = 0; i < numAddresses; i++)
	{
		if (i > 0)
			allRecipients += ", ";

		if (strlen(curName))
		{
			allRecipients += curName;
			allRecipients += ' ';
		}

		if (strlen(curAddress))
		{
			if (strlen(curName))
				allRecipients += '<';
			else
				allRecipients += "<";
			allRecipients += curAddress;
			allRecipients += '>';
		}
		curName += strlen(curName) + 1;
		curAddress += strlen(curAddress) + 1;
	}
	char *cstringRecipients = allRecipients.ToNewCString();
	ret = SetCCList(cstringRecipients);
	delete [] cstringRecipients;
	return ret;
}


NS_IMETHODIMP nsMsgHdr::SetMessageSize(PRUint32 messageSize)
{
	SetUInt32Column(messageSize, m_mdb->m_messageSizeColumnToken);
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::SetLineCount(PRUint32 lineCount)
{
	SetUInt32Column(lineCount, m_mdb->m_numLinesColumnToken);
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::SetStatusOffset(PRUint32 statusOffset)
{
	return SetUInt32Column(statusOffset, m_mdb->m_statusOffsetColumnToken);
}

NS_IMETHODIMP nsMsgHdr::SetDate(time_t date)
{
    return SetUInt32Column((PRUint32) date, m_mdb->m_dateColumnToken);
}

NS_IMETHODIMP nsMsgHdr::GetStatusOffset(PRUint32 *result)
{
	PRUint32 offset = 0;
	nsresult res = GetUInt32Column(m_mdb->m_statusOffsetColumnToken, &offset);

	*result = offset;
    return res;
}

NS_IMETHODIMP nsMsgHdr::SetPriority(nsMsgPriority priority)
{
	m_priority = priority; 
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetMessageOffset(PRUint32 *result)
{
    *result = m_messageKey;
    return NS_OK;
}

NS_IMETHODIMP nsMsgHdr::GetMessageSize(PRUint32 *result)
{
	PRUint32 size;
	nsresult res = GetUInt32Column(m_mdb->m_messageSizeColumnToken, &size);

	*result = size;
	return res;
}

NS_IMETHODIMP nsMsgHdr::SetPriority(const char *priority)
{
// ### TODO
//	m_priority = MSG_GetPriorityFromString(priority);
	return SetPriority(nsMsgPriority_Normal);
}

NS_IMETHODIMP nsMsgHdr::GetAuthor(nsString &resultAuthor)
{
	return m_mdb->RowCellColumnTonsString(GetMDBRow(), m_mdb->m_senderColumnToken, resultAuthor);
}

NS_IMETHODIMP nsMsgHdr::GetSubject(nsString &resultSubject)
{
	return m_mdb->RowCellColumnTonsString(GetMDBRow(), m_mdb->m_subjectColumnToken, resultSubject);
}

NS_IMETHODIMP nsMsgHdr::GetRecipients(nsString &resultRecipients)
{
	return m_mdb->RowCellColumnTonsString(GetMDBRow(), m_mdb->m_recipientsColumnToken, resultRecipients);
}

NS_IMETHODIMP nsMsgHdr::GetCCList(nsString &resultCCList)
{
	return m_mdb->RowCellColumnTonsString(GetMDBRow(), m_mdb->m_ccListColumnToken, resultCCList);
}


nsresult nsMsgHdr::SetStringColumn(const char *str, mdb_token token)
{
	struct mdbYarn yarn;
	yarn.mYarn_Buf = (void *) (str ? str : "");
	yarn.mYarn_Size = PL_strlen((const char *) yarn.mYarn_Buf) + 1;
	yarn.mYarn_Fill = yarn.mYarn_Size - 1;
	yarn.mYarn_Form = 0;
	yarn.mYarn_Grow = NULL;
	return m_mdbRow->AddColumn(m_mdb->GetEnv(), token, &yarn);
}

nsresult nsMsgHdr::SetUInt32Column(PRUint32 value, mdb_token token)
{
	char	yarnBuf[100];

	struct mdbYarn yarn;
	yarn.mYarn_Buf = (void *) yarnBuf;
	yarn.mYarn_Size = sizeof(yarnBuf);
	yarn.mYarn_Fill = yarn.mYarn_Size;
	yarn.mYarn_Form = 0;
	yarn.mYarn_Grow = NULL;
	return m_mdbRow->AddColumn(m_mdb->GetEnv(),  token, nsMsgDatabase::UInt32ToYarn(&yarn, value));
}

nsresult nsMsgHdr::GetUInt32Column(mdb_token token, PRUint32 *pvalue)
{
	return m_mdb->RowCellColumnToUInt32(GetMDBRow(), token, pvalue);
}
