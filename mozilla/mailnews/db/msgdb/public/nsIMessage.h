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

#ifndef nsIMessage_h__
#define nsIMessage_h__

#include "MailNewsTypes.h"
#include "nsString.h"

#define NS_IMESSAGE_IID                              \
{ /* 4e994f60-c317-11d2-8cc9-0060b0fc14a3 */         \
    0x4e994f60,                                      \
    0xc317,                                          \
    0x11d2,                                          \
    {0x8c, 0xc9, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsIMessage : public nsISupports {
public:
    static const nsIID& IID() { static nsIID iid = NS_IMESSAGE_IID; return iid; }

    NS_IMETHOD GetProperty(const char *propertyName, nsString &resultProperty) = 0;
    NS_IMETHOD SetProperty(const char *propertyName, nsString &propertyStr) = 0;
    NS_IMETHOD GetUint32Property(const char *propertyName, PRUint32 *pResult) = 0;
    NS_IMETHOD SetUint32Property(const char *propertyName, PRUint32 propertyVal) = 0;
    NS_IMETHOD GetNumReferences(PRUint16 *result) = 0;
    NS_IMETHOD GetStringReference(PRInt32 refNum, nsString &resultReference) = 0;
    NS_IMETHOD GetDate(time_t *result) = 0;
    NS_IMETHOD SetDate(time_t date) = 0;
    NS_IMETHOD SetMessageId(const char *messageId) = 0;
    NS_IMETHOD SetReferences(const char *references) = 0;
    NS_IMETHOD SetCCList(const char *ccList) = 0;
    NS_IMETHOD SetRecipients(const char *recipients, PRBool recipientsIsNewsgroup) = 0;
    NS_IMETHOD SetAuthor(const char *author) = 0;
    NS_IMETHOD SetSubject(const char *subject) = 0;
    NS_IMETHOD SetStatusOffset(PRUint32 statusOffset) = 0;

    // flag handling routines
    NS_IMETHOD GetFlags(PRUint32 *result) = 0;
    NS_IMETHOD SetFlags(PRUint32 flags) = 0;
    NS_IMETHOD OrFlags(PRUint32 flags, PRUint32 *result) = 0;
    NS_IMETHOD AndFlags(PRUint32 flags, PRUint32 *result) = 0;

    NS_IMETHOD GetMessageKey(nsMsgKey *result) = 0;
    NS_IMETHOD GetThreadId(nsMsgKey *result) = 0;
    NS_IMETHOD SetMessageKey(nsMsgKey inKey) = 0;
    NS_IMETHOD GetMessageSize(PRUint32 *result) = 0;
    NS_IMETHOD SetMessageSize(PRUint32 messageSize) = 0;
    NS_IMETHOD SetLineCount(PRUint32 lineCount) = 0;
    NS_IMETHOD SetPriority(nsMsgPriority priority) = 0;
    NS_IMETHOD SetPriority(const char *priority) = 0;
    NS_IMETHOD GetMessageOffset(PRUint32 *result) = 0;
    NS_IMETHOD GetStatusOffset(PRUint32 *result) = 0; 
};

#endif // nsIMessage_h__
