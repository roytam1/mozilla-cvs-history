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
 */

#include "prmem.h"
#include "plstr.h"
#include "prprf.h"

#include "nsCOMPtr.h"
#include "nsIPref.h"

#include "nsXPIDLString.h"
#include "nsIStringBundle.h"

#include "nsIPop3IncomingServer.h"
#include "nsPop3IncomingServer.h"
#include "nsIPop3Service.h"
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"
#include "nsMsgFolderFlags.h"
#include "nsIFileSpec.h"
#include "nsPop3Protocol.h"
#include "nsIMsgMailSession.h"

static NS_DEFINE_CID(kCPop3ServiceCID, NS_POP3SERVICE_CID);
static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define INBOX_NAME "Inbox"

nsrefcnt nsPop3IncomingServer::gInstanceCount	= 0;

PRUnichar *nsPop3IncomingServer::kInboxName = 0;
PRUnichar *nsPop3IncomingServer::kTrashName = 0;
PRUnichar *nsPop3IncomingServer::kSentName = 0;
PRUnichar *nsPop3IncomingServer::kDraftsName = 0;
PRUnichar *nsPop3IncomingServer::kTemplatesName = 0;
PRUnichar *nsPop3IncomingServer::kUnsentName = 0;

NS_IMPL_ISUPPORTS_INHERITED2(nsPop3IncomingServer,
				nsMsgIncomingServer,
                            	nsIPop3IncomingServer,
				nsILocalMailIncomingServer)

nsPop3IncomingServer::nsPop3IncomingServer()
{    
    NS_INIT_REFCNT();
    m_capabilityFlags = 
        POP3_AUTH_LOGIN_UNDEFINED |
        POP3_XSENDER_UNDEFINED |
        POP3_GURL_UNDEFINED |
        POP3_UIDL_UNDEFINED |
        POP3_TOP_UNDEFINED |
        POP3_XTND_XLST_UNDEFINED;

    if (gInstanceCount++ == 0) {
        initializeStrings();

    }
}

nsPop3IncomingServer::~nsPop3IncomingServer()
{
    if (--gInstanceCount == 0) {
        CRTFREEIF(kInboxName);
        CRTFREEIF(kTrashName);
        CRTFREEIF(kSentName);
        CRTFREEIF(kDraftsName);
        CRTFREEIF(kTemplatesName);
        CRTFREEIF(kUnsentName);
    }
}

nsresult
nsPop3IncomingServer::initializeStrings()
{
    nsresult rv;
    nsCOMPtr<nsIStringBundleService> bundleService =
        do_GetService(kStringBundleServiceCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle("chrome://messenger/locale/messenger.properties",
                                     nsnull,
                                     getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);
    
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("inboxFolderName").GetUnicode(),
                              &kInboxName);
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("trashFolderName").GetUnicode(),
                              &kTrashName);
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("sentFolderName").GetUnicode(),
                              &kSentName);
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("draftsFolderName").GetUnicode(),
                              &kDraftsName);
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("templatesFolderName").GetUnicode(),
                              &kTemplatesName);
    bundle->GetStringFromName(NS_ConvertASCIItoUCS2("unsentFolderName").GetUnicode(),
                              &kUnsentName);
    return NS_OK;
}


NS_IMPL_SERVERPREF_BOOL(nsPop3IncomingServer,
                        LeaveMessagesOnServer,
                        "leave_on_server")

NS_IMPL_SERVERPREF_BOOL(nsPop3IncomingServer,
                        DeleteMailLeftOnServer,
                        "delete_mail_left_on_server")

NS_IMPL_SERVERPREF_BOOL(nsPop3IncomingServer,
                        AuthLogin,
                        "auth_login")

NS_IMPL_SERVERPREF_BOOL(nsPop3IncomingServer,
                        DotFix,
                        "dot_fix")

NS_IMPL_SERVERPREF_BOOL(nsPop3IncomingServer,
                        LimitMessageSize,
                        "limit_message_size")

NS_IMPL_SERVERPREF_INT(nsPop3IncomingServer,
                       MaxMessageSize,
                       "max_size")
nsresult 
nsPop3IncomingServer::GetPop3CapabilityFlags(PRUint32 *flags)
{
    *flags = m_capabilityFlags;
    return NS_OK;
}

nsresult
nsPop3IncomingServer::SetPop3CapabilityFlags(PRUint32 flags)
{
    m_capabilityFlags = flags;
    return NS_OK;
}

nsresult
nsPop3IncomingServer::GetLocalStoreType(char **type)
{
    NS_ENSURE_ARG_POINTER(type);
    *type = nsCRT::strdup("mailbox");
    return NS_OK;
}

NS_IMETHODIMP nsPop3IncomingServer::PerformBiff()
{
	nsresult rv;

	NS_WITH_SERVICE(nsIPop3Service, pop3Service, kCPop3ServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

	nsCOMPtr<nsIMsgFolder> inbox;
	nsCOMPtr<nsIFolder> rootFolder;
	rv = GetRootFolder(getter_AddRefs(rootFolder));
	if(NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIMsgFolder> rootMsgFolder = do_QueryInterface(rootFolder);
		if(rootMsgFolder)
		{
			PRUint32 numFolders;
			rv = rootMsgFolder->GetFoldersWithFlag(MSG_FOLDER_FLAG_INBOX, 1,
                                                   &numFolders,
                                                   getter_AddRefs(inbox));
            if (NS_FAILED(rv) || numFolders != 1) return rv;
		}
	}

	//Biff just needs to give status in one of the windows. so do it in topmost window.
	NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
    if (NS_FAILED(rv)) return rv;

	
	nsCOMPtr<nsIMsgWindow> msgWindow;

	rv = mailSession->GetTopmostMsgWindow(getter_AddRefs(msgWindow));
	if(NS_SUCCEEDED(rv))
    {
        PRBool downloadOnBiff = PR_FALSE;
        rv = GetDownloadOnBiff(&downloadOnBiff);
        if (downloadOnBiff)
            rv = pop3Service->GetNewMail(msgWindow, nsnull, this, nsnull);
        else
            rv = pop3Service->CheckForNewMail(msgWindow, nsnull, inbox, this,
                                              nsnull);
    }

	return NS_OK;
}


NS_IMETHODIMP nsPop3IncomingServer::SetFlagsOnDefaultMailboxes()
{
	nsresult rv;

	nsCOMPtr<nsIFolder> rootFolder;
	rv = GetRootFolder(getter_AddRefs(rootFolder));
	if (NS_FAILED(rv)) return rv;
	if (!rootFolder) return NS_ERROR_FAILURE;

	nsCOMPtr <nsIFolder> folder;
	nsCOMPtr<nsIMsgFolder> msgFolder;

	rv = rootFolder->FindSubFolder(INBOX_NAME, getter_AddRefs(folder));
	if (NS_FAILED(rv)) return rv;
	if (!folder) return NS_ERROR_FAILURE;
 	msgFolder = do_QueryInterface(folder);
	if (!msgFolder) return NS_ERROR_FAILURE;
	rv = msgFolder->SetFlag(MSG_FOLDER_FLAG_INBOX);
	if (NS_FAILED(rv)) return rv;

    setSubFolderFlag(rootFolder, kSentName, MSG_FOLDER_FLAG_SENTMAIL);
    setSubFolderFlag(rootFolder, kDraftsName, MSG_FOLDER_FLAG_DRAFTS);
    setSubFolderFlag(rootFolder, kTemplatesName, MSG_FOLDER_FLAG_TEMPLATES);
    setSubFolderFlag(rootFolder, kTrashName, MSG_FOLDER_FLAG_TRASH);

	rv = rootFolder->FindSubFolder("Unsent Messages", getter_AddRefs(folder));
	if (NS_FAILED(rv)) return rv;
	if (!folder) return NS_ERROR_FAILURE;
 	msgFolder = do_QueryInterface(folder);
	if (!msgFolder) return NS_ERROR_FAILURE;
	rv = msgFolder->SetFlag(MSG_FOLDER_FLAG_QUEUE);
	if (NS_FAILED(rv)) return rv;
	
	return NS_OK;
}

nsresult
nsPop3IncomingServer::setSubFolderFlag(nsIFolder *aRootFolder,
                                       PRUnichar *aFolderName,
                                       PRUint32 flags)
{
    nsresult rv;

    // no-copy conversion to utf8 string
    nsSubsumeCStr utf8Name(nsSubsumeStr(aFolderName, PR_FALSE).ToNewUTF8String(), PR_TRUE);

    nsCOMPtr<nsIFolder> folder;
	rv = aRootFolder->FindSubFolder(utf8Name, getter_AddRefs(folder));
	if (NS_FAILED(rv)) return rv;
	if (!folder) return NS_ERROR_FAILURE;
    
 	nsCOMPtr<nsIMsgFolder> msgFolder = do_QueryInterface(folder);
	if (!msgFolder) return NS_ERROR_FAILURE;
    
	rv = msgFolder->SetFlag(MSG_FOLDER_FLAG_TRASH);
	if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP nsPop3IncomingServer::CreateDefaultMailboxes(nsIFileSpec *path)
{
	nsresult rv;
	PRBool exists;
	if (!path) return NS_ERROR_NULL_POINTER;

	// todo, use a string bundle for this
        rv =path->AppendRelativeUnixPath(INBOX_NAME);
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}
	
	rv = path->SetLeafName("Trash");
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (NS_FAILED(rv)) return rv;
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}

	rv = path->SetLeafName("Sent");
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (NS_FAILED(rv)) return rv;
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}
	
	rv = path->SetLeafName("Drafts");
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (NS_FAILED(rv)) return rv;
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}
	
	rv = path->SetLeafName("Templates");
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (NS_FAILED(rv)) return rv;
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}
	
	rv = path->SetLeafName("Unsent Messages");
	if (NS_FAILED(rv)) return rv;
	rv = path->Exists(&exists);
	if (NS_FAILED(rv)) return rv;
	if (!exists) {
		rv = path->Touch();
		if (NS_FAILED(rv)) return rv;
	}
        return rv;
}

NS_IMETHODIMP nsPop3IncomingServer::GetNewMail(nsIMsgWindow *aMsgWindow, nsIUrlListener *aUrlListener, nsIURI **aResult)
{
	nsresult rv;

	NS_WITH_SERVICE(nsIPop3Service, pop3Service, kCPop3ServiceCID, &rv);
  	if (NS_FAILED(rv)) return rv;

        rv = pop3Service->GetNewMail(aMsgWindow, aUrlListener, this, aResult);
	return rv;
}
