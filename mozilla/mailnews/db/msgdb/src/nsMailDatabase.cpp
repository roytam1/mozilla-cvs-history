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
#include "nsMailDatabase.h"
#include "nsDBFolderInfo.h"
#include "nsMsgLocalFolderHdrs.h"
#include "nsFileStream.h"
#include "nsLocalFolderSummarySpec.h"
#include "nsFileSpec.h"


nsMailDatabase::nsMailDatabase()
    : m_reparse(PR_FALSE), m_folderSpec(nsnull), m_folderStream(nsnull)
{
}

nsMailDatabase::~nsMailDatabase()
{
	if(m_folderSpec)
		delete m_folderSpec;
}



NS_IMETHODIMP nsMailDatabase::Open(nsFileSpec &folderName, PRBool create, nsIMsgDatabase** pMessageDB, PRBool upgrading /*=PR_FALSE*/)
{
	nsMailDatabase	*mailDB;
	PRBool			summaryFileExists;
	PRBool			newFile = PR_FALSE;
	nsLocalFolderSummarySpec	summarySpec(folderName);

	nsIDBFolderInfo	*folderInfo = NULL;

	*pMessageDB = NULL;

	nsFileSpec dbPath(summarySpec);

	mailDB = (nsMailDatabase *) FindInCache(dbPath);
	if (mailDB)
	{
		*pMessageDB = mailDB;
		//FindInCache does the AddRef'ing
		//mailDB->AddRef();
		return(NS_OK);
	}

	// if the old summary doesn't exist, we're creating a new one.
	if (!summarySpec.Exists() && create)
		newFile = PR_TRUE;

	mailDB = new nsMailDatabase();

	if (!mailDB)
		return NS_ERROR_OUT_OF_MEMORY;
	mailDB->m_folderSpec = new nsFileSpec(folderName);
	mailDB->AddRef();
	// stat file before we open the db, because if we've latered
	// any messages, handling latered will change time stamp on
	// folder file.
	summaryFileExists = summarySpec.Exists();

	nsresult err = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
	
	err = mailDB->OpenMDB((const char *) summarySpec, create);

	if (NS_SUCCEEDED(err))
	{
		mailDB->GetDBFolderInfo(&folderInfo);
		if (folderInfo == NULL)
		{
			err = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
		}
		else
		{
			// if opening existing file, make sure summary file is up to date.
			// if caller is upgrading, don't return NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE so the caller
			// can pull out the transfer info for the new db.
			if (!newFile && summaryFileExists && !upgrading)
			{
				PRInt32 numNewMessages;
                PRUint32 folderSize;
				PRInt32 folderDateInSeconds;
                time_t  folderDate;
				nsFileSpec::TimeStamp actualFolderTimeStamp;

				mailDB->m_folderSpec->GetModDate(actualFolderTimeStamp) ;


				folderInfo->GetNumNewMessages(&numNewMessages);
                folderInfo->GetFolderSize(&folderSize);
                folderInfo->GetFolderDate(&folderDateInSeconds);
				folderDate = folderDateInSeconds;
				if (folderSize != mailDB->m_folderSpec->GetFileSize()||
                    folderDate != actualFolderTimeStamp ||
                    numNewMessages < 0)
					err = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
			}
			// compare current version of db versus filed out version info.
            PRUint32 version;
            folderInfo->GetVersion(&version);
			if (mailDB->GetCurVersion() != version)
				err = NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
			NS_RELEASE(folderInfo);
		}
		if (err != NS_OK)
		{
			// this will make the db folder info release its ref to the mail db...
			NS_IF_RELEASE(mailDB->m_dbFolderInfo);
			mailDB->ForceClosed();
			if (err == NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE)
				summarySpec.Delete(PR_FALSE);

			mailDB = NULL;
		}
	}
	if (err != NS_OK || newFile)
	{
		// if we couldn't open file, or we have a blank one, and we're supposed 
		// to upgrade, updgrade it.
		if (newFile && !upgrading)	// caller is upgrading, and we have empty summary file,
		{					// leave db around and open so caller can upgrade it.
			err = NS_MSG_ERROR_FOLDER_SUMMARY_MISSING;
		}
		else if (err != NS_OK)
		{
			*pMessageDB = NULL;
			delete mailDB;
			mailDB = NULL;
		}
	}
	if (err == NS_OK || err == NS_MSG_ERROR_FOLDER_SUMMARY_MISSING)
	{
		*pMessageDB = mailDB;
		if (mailDB)
			GetDBCache()->AppendElement(mailDB);
//		if (err == NS_OK)
//			mailDB->HandleLatered();

	}
	return err;
}

/* static */ nsresult nsMailDatabase::CloneInvalidDBInfoIntoNewDB(nsFileSpec &pathName, nsMailDatabase** pMailDB)
{
	nsresult ret = NS_OK;
	return ret;
}

nsresult nsMailDatabase::OnNewPath (nsFileSpec &newPath)
{
	nsresult ret = NS_OK;
	return ret;
}

nsresult nsMailDatabase::DeleteMessages(nsMsgKeyArray* nsMsgKeys, nsIDBChangeListener *instigator)
{
	nsresult ret = NS_OK;
	m_folderStream = new nsIOFileStream(nsFileSpec(m_dbName));
	ret = nsMsgDatabase::DeleteMessages(nsMsgKeys, instigator);
	if (m_folderStream)
		delete m_folderStream;
	m_folderStream = NULL;
	SetFolderInfoValid(m_folderSpec, 0, 0);
	return ret;
}


// Helper routine - lowest level of flag setting
PRBool nsMailDatabase::SetHdrFlag(nsIMsgDBHdr *msgHdr, PRBool bSet, MsgFlags flag)
{
	nsIOFileStream *fileStream = NULL;
	PRBool		ret = PR_FALSE;

	if (nsMsgDatabase::SetHdrFlag(msgHdr, bSet, flag))
	{
		UpdateFolderFlag(msgHdr, bSet, flag, &fileStream);
		if (fileStream != NULL)
		{
			delete fileStream;
			SetFolderInfoValid(m_folderSpec, 0, 0);
		}
		ret = PR_TRUE;
	}
	return ret;
}

#ifdef XP_MAC
extern PRFileDesc *gIncorporateFID;
extern const char* gIncorporatePath;
#endif // XP_MAC

// ### should move this into some utils class...
int msg_UnHex(char C)
{
	return ((C >= '0' && C <= '9') ? C - '0' :
			((C >= 'A' && C <= 'F') ? C - 'A' + 10 :
			 ((C >= 'a' && C <= 'f') ? C - 'a' + 10 : 0)));
}


// We let the caller close the file in case he's updating a lot of flags
// and we don't want to open and close the file every time through.
// As an experiment, try caching the fid in the db as m_folderFile.
// If this is set, use it but don't return *pFid.
void nsMailDatabase::UpdateFolderFlag(nsIMsgDBHdr *mailHdr, PRBool bSet, 
							  MsgFlags flag, nsIOFileStream **ppFileStream)
{
	static char buf[30];
	nsIOFileStream *fileStream = (m_folderStream) ? m_folderStream : *ppFileStream;
//#ifdef GET_FILE_STUFF_TOGETHER
#ifdef XP_MAC
/* ducarroz: Do we still need this ??
	// This is a horrible hack and we should make sure we don't need it anymore.
	// It has to do with multiple people having the same file open, I believe, but the
	// mac file system only has one handle, and they compete for the file position.
	// Prevent closing the file from under the incorporate stuff. #82785.
	int32 savedPosition = -1;
	if (!fid && gIncorporatePath && !XP_STRCMP(m_folderSpec, gIncorporatePath))
	{
		fid = gIncorporateFID;
		savedPosition = ftell(gIncorporateFID); // so we can restore it.
	}
*/
#endif // XP_MAC
    PRUint32 offset;
    (void)mailHdr->GetStatusOffset(&offset);
	if (offset > 0) 
	{
		
		if (fileStream == NULL) 
		{
			fileStream = new nsIOFileStream(nsFileSpec(*m_folderSpec));
		}
		if (fileStream) 
		{
            PRUint32 msgOffset;
            (void)mailHdr->GetMessageOffset(&msgOffset);
			PRUint32 position = offset + msgOffset;
			PR_ASSERT(offset < 10000);
			fileStream->seek(position);
			buf[0] = '\0';
			if (fileStream->readline(buf, sizeof(buf))) 
			{
				if (strncmp(buf, X_MOZILLA_STATUS, X_MOZILLA_STATUS_LEN) == 0 &&
					strncmp(buf + X_MOZILLA_STATUS_LEN, ": ", 2) == 0 &&
					strlen(buf) >= X_MOZILLA_STATUS_LEN + 6) 
				{
                    PRUint32 flags;
                    (void)mailHdr->GetFlags(&flags);
					if (!(flags & MSG_FLAG_EXPUNGED))
					{
						int i;
						char *p = buf + X_MOZILLA_STATUS_LEN + 2;
					
						for (i=0, flags = 0; i<4; i++, p++)
						{
							flags = (flags << 4) | msg_UnHex(*p);
						}
                        
                        PRUint32 curFlags;
                        (void)mailHdr->GetFlags(&curFlags);
						flags = (flags & MSG_FLAG_QUEUED) |
                          (curFlags & ~MSG_FLAG_RUNTIME_ONLY);
					}
					else
					{
						flags &= ~MSG_FLAG_RUNTIME_ONLY;
					}
					fileStream->seek(position);
					// We are filing out old Cheddar flags here
					PR_snprintf(buf, sizeof(buf), X_MOZILLA_STATUS_FORMAT, flags);
					fileStream->write(buf, PL_strlen(buf));

					// time to upate x-mozilla-status2
					position = fileStream->tell();
					fileStream->seek(position + MSG_LINEBREAK_LEN);
					if (fileStream->readline(buf, sizeof(buf))) 
					{
						if (strncmp(buf, X_MOZILLA_STATUS2, X_MOZILLA_STATUS2_LEN) == 0 &&
							strncmp(buf + X_MOZILLA_STATUS2_LEN, ": ", 2) == 0 &&
							strlen(buf) >= X_MOZILLA_STATUS2_LEN + 10) 
						{
							PRUint32 dbFlags;
                            (void)mailHdr->GetFlags(&dbFlags);
							dbFlags &= (MSG_FLAG_MDN_REPORT_NEEDED | MSG_FLAG_MDN_REPORT_SENT | MSG_FLAG_TEMPLATE);
							fileStream->seek(position + MSG_LINEBREAK_LEN);
							PR_snprintf(buf, sizeof(buf), X_MOZILLA_STATUS2_FORMAT, dbFlags);
							fileStream->write(buf, PL_strlen(buf));
						}
					}
				} else 
				{
					printf("Didn't find %s where expected at position %ld\n"
						  "instead, found %s.\n",
						  X_MOZILLA_STATUS, (long) position, buf);
					SetReparse(PR_TRUE);
				}			
			} 
			else 
			{
				printf("Couldn't read old status line at all at position %ld\n",
						(long) position);
				SetReparse(PR_TRUE);
			}
#ifdef XP_MAC
/* ducarroz: Do we still need this ??
			// Restore the file position
			if (savedPosition >= 0)
				XP_FileSeek(fid, savedPosition, SEEK_SET);
*/
#endif
		}
		else
		{
			printf("Couldn't open mail folder for update%s!\n",
                   (const char*)m_folderSpec);
			PR_ASSERT(PR_FALSE);
		}
	}
//#endif // GET_FILE_STUFF_TOGETHER
#ifdef XP_MAC
	if (!m_folderStream /*&& fid != gIncorporateFID*/)	/* ducarroz: Do we still need this ?? */
#else
	if (!m_folderStream)
#endif
		*ppFileStream = fileStream; // This tells the caller that we opened the file, and please to close it.
}

/* static */  nsresult nsMailDatabase::SetSummaryValid(PRBool valid)
{
	nsresult ret = NS_OK;

	if (!m_folderSpec->Exists()) 
		return NS_MSG_ERROR_FOLDER_MISSING;

	if (m_dbFolderInfo)
	{
		if (valid)
		{
			nsFileSpec::TimeStamp actualFolderTimeStamp;
			m_folderSpec->GetModDate(actualFolderTimeStamp) ;


			m_dbFolderInfo->SetFolderSize(m_folderSpec->GetFileSize());
			m_dbFolderInfo->SetFolderDate(actualFolderTimeStamp);
		}
		else
		{
			m_dbFolderInfo->SetFolderDate(0);	// that ought to do the trick.
		}
	}
	return ret;
}

nsresult nsMailDatabase::GetFolderName(nsString &folderName)
{
	folderName = *m_folderSpec;
	return NS_OK;
}


// The master is needed to find the folder info corresponding to the db.
// Perhaps if we passed in the folder info when we opened the db, 
// we wouldn't need the master. I don't remember why we sometimes need to
// get from the db to the folder info, but it's probably something like
// some poor soul who has a db pointer but no folderInfo.


MSG_FolderInfo *nsMailDatabase::GetFolderInfo()
{
	PR_ASSERT(PR_FALSE);
	return NULL;
}
	
nsresult nsMailDatabase::GetOfflineOpForKey(nsMsgKey opKey, PRBool create, nsOfflineImapOperation **)
{
	nsresult ret = NS_OK;
	return ret;
}

nsresult nsMailDatabase::AddOfflineOp(nsOfflineImapOperation *op)
{
	nsresult ret = NS_OK;
	return ret;
}

nsresult DeleteOfflineOp(nsMsgKey opKey)
{
	nsresult ret = NS_OK;
	return ret;
}

nsresult SetSourceMailbox(nsOfflineImapOperation *op, const char *mailbox, nsMsgKey key)
{
	nsresult ret = NS_OK;
	return ret;
}

	
nsresult nsMailDatabase::GetIdsWithNoBodies (nsMsgKeyArray &bodylessIds)
{
	nsresult ret = NS_OK;
	return ret;
}

/* static */
nsresult nsMailDatabase::SetFolderInfoValid(nsFileSpec *folderName, int num, int numunread)
{
	nsLocalFolderSummarySpec	summarySpec(*folderName);
	nsFileSpec					summaryPath(summarySpec);
	nsresult		err;

	if (!folderName->Exists())
		return NS_MSG_ERROR_FOLDER_SUMMARY_MISSING;

	// should we have type safe downcast methods again?
	nsMailDatabase *pMessageDB = (nsMailDatabase *) nsMailDatabase::FindInCache(summaryPath);
	if (pMessageDB == NULL)
	{
		pMessageDB = new nsMailDatabase();
		if(!pMessageDB)
			return NS_ERROR_OUT_OF_MEMORY;

		pMessageDB->m_folderSpec = new nsLocalFolderSummarySpec();
		if(!pMessageDB->m_folderSpec)
		{
			delete pMessageDB;
			return NS_ERROR_OUT_OF_MEMORY;
		}

		*(pMessageDB->m_folderSpec) = summarySpec;
		// ### this does later stuff (marks latered messages unread), which may be a problem
		err = pMessageDB->OpenMDB(summaryPath, PR_FALSE);
		if (err != NS_OK)
		{
			delete pMessageDB;
			pMessageDB = NULL;
		}
	}
	else
		pMessageDB->AddRef();


	if (pMessageDB == NULL)
	{
		printf("Exception opening summary file\n");
		return NS_MSG_ERROR_FOLDER_SUMMARY_OUT_OF_DATE;
	}
	
	{
		nsFileSpec::TimeStamp actualFolderTimeStamp;
		folderName->GetModDate(actualFolderTimeStamp) ;

		pMessageDB->m_dbFolderInfo->SetFolderSize(folderName->GetFileSize());
		pMessageDB->m_dbFolderInfo->SetFolderDate(actualFolderTimeStamp);
		pMessageDB->m_dbFolderInfo->ChangeNumVisibleMessages(num);
		pMessageDB->m_dbFolderInfo->ChangeNumNewMessages(numunread);
		pMessageDB->m_dbFolderInfo->ChangeNumMessages(num);
	}
	pMessageDB->Close(PR_TRUE);
	return NS_OK;
}


// This is used to remember that the db is out of sync with the mail folder
// and needs to be regenerated.
void nsMailDatabase::SetReparse(PRBool reparse)
{
	m_reparse = reparse;
}


static PRBool gGotThreadingPrefs = PR_FALSE;
static PRBool gThreadWithoutRe = PR_FALSE;


// should we thread messages with common subjects that don't start with Re: together?
// I imagine we might have separate preferences for mail and news, so this is a virtual method.
PRBool	nsMailDatabase::ThreadBySubjectWithoutRe()
{
	if (!gGotThreadingPrefs)
	{
		GetBoolPref("mail.thread_without_re", &gThreadWithoutRe);
		gGotThreadingPrefs = PR_TRUE;
	}

	return gThreadWithoutRe;
}

#ifdef DEBUG	// strictly for testing purposes
nsresult nsMailDatabase::PrePopulate()
{
	nsIMsgDBHdr	*msg;
	nsMsgHdr	*newHdr = NULL;
	PRTime resultTime, intermediateResult, microSecondsPerSecond;
	resultTime = PR_Now();
	time_t resDate;

	LL_I2L(microSecondsPerSecond, PR_USEC_PER_SEC);
	LL_DIV(intermediateResult, resultTime, microSecondsPerSecond);
	LL_L2I(resDate, intermediateResult);

	nsresult rv = CreateNewHdr(1, &msg);
    if (NS_FAILED(rv)) return rv;
    newHdr = NS_STATIC_CAST(nsMsgHdr*, msg);          // closed system, cast ok
	newHdr->SetAuthor("bird@celtics.com (Larry Bird)");
	newHdr->SetSubject("Why the Lakers suck");
	newHdr->SetDate(resDate);
	newHdr->SetRecipients("riley@heat.com (Pat Riley)", PR_FALSE);
	AddNewHdrToDB (newHdr, PR_TRUE);
	newHdr->Release();

	rv = CreateNewHdr(2, &msg);
    if (NS_FAILED(rv)) return rv;
    newHdr = NS_STATIC_CAST(nsMsgHdr*, msg);          // closed system, cast ok
	newHdr->SetAuthor("shaq@brick.com (Shaquille O'Neal)");
	newHdr->SetSubject("Anyone here know how to shoot free throws?");
	newHdr->SetDate(resDate);
	AddNewHdrToDB (newHdr, PR_TRUE);
	newHdr->Release();

	rv = CreateNewHdr(3, &msg);
    if (NS_FAILED(rv)) return rv;
    newHdr = NS_STATIC_CAST(nsMsgHdr*, msg);          // closed system, cast ok
	newHdr->SetAuthor("dj@celtics.com (Dennis Johnson)");
	newHdr->SetSubject("Has anyone seen my jump shot?");
	newHdr->SetDate(resDate);
	AddNewHdrToDB (newHdr, PR_TRUE);
	newHdr->Release();

	rv = CreateNewHdr(4, &msg);
    if (NS_FAILED(rv)) return rv;
    newHdr = NS_STATIC_CAST(nsMsgHdr*, msg);          // closed system, cast ok
	newHdr->SetAuthor("sichting@celtics.com (Jerry Sichting)");
	newHdr->SetSubject("Tips for fighting 7' 4\" guys");
	newHdr->SetDate(resDate);
	AddNewHdrToDB (newHdr, PR_TRUE);
	newHdr->Release();
	return NS_OK;
}

#endif
