/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _NSMSGUTILS_H
#define _NSMSGUTILS_H

#include "nsIURL.h"
#include "nsIMsgMessageService.h"
#include "nsString.h"
#include "nsIEnumerator.h"
#include "nsIMsgFolder.h"
#include "msgCore.h"
#include "nsCOMPtr.h"

//These are utility functions that can used throughout the mailnews code

//Utilities for getting a message service.
NS_MSG_BASE nsresult GetMessageServiceContractIDForURI(const char *uri, nsString &contractID);
//Use ReleaseMessageServiceFromURI to release the service.
NS_MSG_BASE nsresult GetMessageServiceFromURI(const char *uri, nsIMsgMessageService **messageService);

NS_MSG_BASE nsresult GetMsgDBHdrFromURI(const char *uri, nsIMsgDBHdr **msgHdr);

NS_MSG_BASE nsresult ReleaseMessageServiceFromURI(const char *uri, nsIMsgMessageService *messageService);

NS_MSG_BASE nsresult CreateStartupUrl(char *uri, nsIURI** aUrl);

NS_MSG_BASE nsresult NS_MsgGetPriorityFromString(const char *priority, nsMsgPriorityValue *outPriority);

NS_MSG_BASE nsresult NS_MsgGetUntranslatedPriorityName (nsMsgPriorityValue p, nsString *outName);

NS_MSG_BASE nsresult NS_MsgHashIfNecessary(nsCAutoString &name);

NS_MSG_BASE nsresult NS_MsgCreatePathStringFromFolderURI(const char *folderURI, nsCString& pathString);

NS_MSG_BASE PRBool NS_MsgStripRE(const char **stringP, PRUint32 *lengthP);

NS_MSG_BASE char * NS_MsgSACopy(char **destination, const char *source);

NS_MSG_BASE char * NS_MsgSACat(char **destination, const char *source);

NS_MSG_BASE nsresult NS_MsgEscapeEncodeURLPath(const PRUnichar *str, char **result);

NS_MSG_BASE nsresult NS_MsgDecodeUnescapeURLPath(const char *path, PRUnichar **result);

#endif

