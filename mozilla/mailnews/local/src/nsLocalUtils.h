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

#ifndef NS_LOCALUTILS_H
#define NS_LOCALUTILS_H

#include "nsFileSpec.h"
#include "nsString.h"

static const char kMailboxRootURI[] = "mailbox:/";
static const char kMailboxMessageRootURI[] = "mailbox_message:/";

nsresult
nsGetMailboxRoot(const char* hostname, nsFileSpec &result);

nsresult
nsLocalURI2Path(const char* rootURI, const char* uriStr, nsFileSpec& pathResult);

nsresult
nsLocalURI2Name(const char* rootURI, char* uriStr, nsString& name);

nsresult
nsParseLocalMessageURI(const char* uri, nsString& folderURI, PRUint32 *key);

nsresult nsBuildLocalMessageURI(const char* baseURI, PRUint32 key, char** uri);

nsresult nsGetMailboxHostName(const char *rootURI, const char *uriStr, char **hostName);

#endif //NS_LOCALUTILS_H
