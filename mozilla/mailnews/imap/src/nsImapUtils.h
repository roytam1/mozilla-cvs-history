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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
*/

#ifndef NS_IMAPUTILS_H
#define NS_IMAPUTILS_H

#include "nsFileSpec.h"
#include "nsString.h"

static const char kImapRootURI[] = "imap:/";
static const char kImapMessageRootURI[] = "imap_message:/";

extern nsresult
nsImapURI2Path(const char* rootURI, const char* uriStr, 
               nsFileSpec& pathResult);

extern nsresult
nsPath2ImapURI(const char* rootURI, const nsFileSpec& path, char* *uri);

extern nsresult
nsImapURI2Name(const char* rootURI, const char* uriStr, nsString& name);

extern nsresult
nsImapURI2FullName(const char* rootURI, const char* hostname, char* uriStr,
                   nsString& name);

extern nsresult
nsImapURI2HostName(const char *rootURI, const char* uriStr, 
                   nsString& hostname);

extern nsresult
nsImapURI2UserName(const char *rootURI, const char* uriStr, 
                   nsString& username);

extern nsresult
nsURI2ProtocolType(const char* uriStr, nsString& type);

extern nsresult
nsParseImapMessageURI(const char* uri, nsString& folderURI, PRUint32 *key);

extern nsresult 
nsBuildImapMessageURI(const char *baseURI, PRUint32 key, char **uri);


#endif //NS_IMAPUTILS_H
