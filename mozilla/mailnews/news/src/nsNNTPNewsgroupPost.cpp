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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "msgCore.h"    // precompiled header...

#include "nsINNTPNewsgroupPost.h"
#include "nsNNTPNewsgroupPost.h"

#include "plstr.h"
#include "prmem.h"
#include "nslog.h"

NS_IMPL_LOG(nsNNTPNewsgroupPostLog)
#define PRINTF NS_LOG_PRINTF(nsNNTPNewsgroupPostLog)
#define FLUSH  NS_LOG_FLUSH(nsNNTPNewsgroupPostLog)

// for CRLF
//#include "fe_proto.h"

const char* nsNNTPNewsgroupPost::m_headerName[HEADER_LAST+1]=
{
    "From: ",
    "Newsgroups: ",
    "Subject: ",

    "Path: ",
    "Date: ",
    
    "Reply-To: ",
    "Sender: ",
    "Followup-To: ",
    "Date-Received: ",
    "Expires: ",
    "Control: ",
    "Distribution: ",
    "Organization: ",
    "References: ",
    
    "Relay-Version: ",
    "Posting-Version: ",
    "Message-ID: ",
};
    
NS_IMPL_ISUPPORTS(nsNNTPNewsgroupPost, NS_GET_IID(nsINNTPNewsgroupPost));

nsNNTPNewsgroupPost::nsNNTPNewsgroupPost()
{
	NS_INIT_REFCNT();

    int i;
    for (i=0; i <= HEADER_LAST; i++)
        m_header[i]=nsnull;

    m_body=nsnull;
    m_messageBuffer=nsnull;
    
    m_isControl=PR_FALSE;
	m_postMessageFile = nsnull;
}

nsNNTPNewsgroupPost::~nsNNTPNewsgroupPost()
{
    int i;
    for (i=0; i<=HEADER_LAST; i++)
        if (m_header[i]) PR_FREEIF(m_header[i]);

    PR_FREEIF(m_body);
    PR_FREEIF(m_messageBuffer);
    NS_IF_RELEASE(m_postMessageFile);
}

nsresult
nsNNTPNewsgroupPost::IsValid(PRBool *_retval)
{
    if (!_retval) return NS_OK;

    // hack to get lazy-generated message ID to work
    char *messageid;
    GetMessageID(&messageid);

    PRBool valid=PR_TRUE;
    int i;
    for (i=0; i<=HEADER_LAST_REQUIRED;i++) {
        if (!m_header[i]) {
            valid=PR_FALSE;
            break;
        }
    }

    *_retval=valid;
    PRINTF("nsNNTPNewsgroupPost::IsValid() -> %s (%d is first invalid)\n",
           *_retval ? "TRUE": "FALSE", i);
    return NS_OK;
}

/* XXX - I'm just guessing at how this works, see RFC850 for more */
nsresult
nsNNTPNewsgroupPost::MakeControlCancel(const char *messageID)
{
    char *new_subject = (char *)PR_Calloc(PL_strlen(messageID) +
                                          PL_strlen("CANCEL ") + 1,
                                          sizeof(char));
    PL_strcpy(new_subject, "CANCEL ");
    PL_strcat(new_subject, messageID);

    m_isControl = PR_TRUE;
    SetSubject(new_subject);

    return NS_OK;
}

nsresult
nsNNTPNewsgroupPost::GetFullMessage(char **message)
{
    if (!message) return NS_ERROR_NULL_POINTER;
    PRBool valid=PR_FALSE;
    IsValid(&valid);
    if (!valid) return NS_ERROR_NOT_INITIALIZED;

    int len=0;
    int i;
    for (i=0; i<=HEADER_LAST; i++) {
        if (m_header[i]) {
            len+=PL_strlen(m_headerName[i]);
            len+=PL_strlen(m_header[i]);
            len+=2;
        }
    }
    len += PL_strlen(m_body);

    // for trailing \0 and CRLF between headers and message
    len+= PL_strlen(CRLF)+1;

    PR_FREEIF(m_messageBuffer);
    m_messageBuffer = (char *)PR_Calloc(len, sizeof(char));

    PL_strcpy(m_messageBuffer,"");
    for (i=0; i<=HEADER_LAST; i++) {
        if (m_header[i]) {
            PL_strcat(m_messageBuffer, m_headerName[i]);
            PL_strcat(m_messageBuffer, m_header[i]);
            PL_strcat(m_messageBuffer, CRLF);
        }
    }
    
    PL_strcat(m_messageBuffer, CRLF);
    PL_strcat(m_messageBuffer, m_body);
    
    *message=m_messageBuffer;

    PRINTF("Assembled message:\n%s\n",m_messageBuffer);
    return NS_OK;
}

char *
nsNNTPNewsgroupPost::AppendAndAlloc(char *string,
                                    const char *newSubstring,
                                    PRBool withComma)
{
    if (!newSubstring) return NULL;
    
    if (!string) return PL_strdup(newSubstring);
    
    char *separator = (char *) (withComma ? ", " : " ");
    char *oldString = string;
    
    string = (char *)PR_Calloc(PL_strlen(oldString) +
                               PL_strlen(separator) +
                               PL_strlen(newSubstring) + 1,
                               sizeof(char));
    
    PL_strcpy(string, oldString);
    PL_strcat(string, separator);
    PL_strcat(string, newSubstring);

    PR_Free(oldString);
    return string;
}

nsresult
nsNNTPNewsgroupPost::AddNewsgroup(const char *newsgroup)
{
    m_header[HEADER_NEWSGROUPS]=AppendAndAlloc(m_header[HEADER_NEWSGROUPS], newsgroup, PR_TRUE);
    return NS_OK;
}
nsresult
nsNNTPNewsgroupPost::AddReference(const char *reference)
{
    m_header[HEADER_REFERENCES]=AppendAndAlloc(m_header[HEADER_REFERENCES], reference, PR_FALSE);
    return NS_OK;
}

nsresult
nsNNTPNewsgroupPost::GetMessageID(char **messageID)
{
    // hack - this is where we will somehow generate the ID
    static char *fakeID = "<ABCDEFG@mcom.com>";
    //    m_header[HEADER_MESSAGEID] = fakeID;
    
    *messageID = fakeID;
    if (!messageID) return NS_ERROR_NULL_POINTER;
    return NS_OK;
}


// the message can be stored in a file....allow accessors for getting and setting
// the file name to post...
nsresult
nsNNTPNewsgroupPost::SetPostMessageFile(nsIFileSpec * aPostMessageFile)
{
	NS_LOCK_INSTANCE();
	if (aPostMessageFile)
	{
		NS_IF_RELEASE(m_postMessageFile);
		m_postMessageFile = aPostMessageFile;
		NS_IF_ADDREF(m_postMessageFile);
	}
	NS_UNLOCK_INSTANCE();
	return NS_OK;
}

nsresult 
nsNNTPNewsgroupPost::GetPostMessageFile(nsIFileSpec ** aPostMessageFile)
{
	NS_LOCK_INSTANCE();
	if (aPostMessageFile)
	{
		*aPostMessageFile = m_postMessageFile;
		NS_IF_ADDREF(m_postMessageFile);
	}
    NS_UNLOCK_INSTANCE();
    return NS_OK;
}
