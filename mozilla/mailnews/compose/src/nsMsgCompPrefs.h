/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _MsgCompPrefs_H_
#define _MsgCompPrefs_H_

#include "msgCore.h"


class nsMsgCompPrefs {
public:
	nsMsgCompPrefs();
	virtual ~nsMsgCompPrefs();

	const char * GetOrganization() {return m_organization;}
	const char * GetUserFullName() {return m_userFullName;}
	const char * GetUserEmail() {return m_userEmail;}
	const char * GetReplyTo() {return m_replyTo;}
	const PRBool GetUseHtml() {return m_useHTML;}
	const PRInt32 GetWrapColumn() {return m_wrapColumn;}

private:
	char *	m_organization;
	char *	m_userFullName;
	char *	m_userEmail;
	char *	m_replyTo;
	PRBool	m_useHTML;
	PRInt32	m_wrapColumn;
};


#endif /* _MsgCompPrefs_H_ */
