/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Seth Spitzer <sspitzer@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// this file implements the nsMsgFilter interface 

#include "msgCore.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgHdr.h"
#include "nsMsgFilterList.h"    // for kFileVersion
#include "nsMsgFilter.h"
#include "nsMsgUtils.h"
#include "nsFileStream.h"
#include "nsMsgLocalSearch.h"
#include "nsMsgSearchTerm.h"
#include "nsXPIDLString.h"
#include "nsMsgSearchScopeTerm.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgIncomingServer.h"
#include "nsMsgSearchValue.h"
#include "nsReadableUtils.h"
#include "nsEscape.h"
#include "nsMsgUtf7Utils.h"
#include "nsIImportService.h"
#include "nsISupportsObsolete.h"
#include "nsIOutputStream.h"
#include "nsEscape.h"

static const char *kImapPrefix = "//imap:";

nsMsgRuleAction::nsMsgRuleAction()
{
}

nsMsgRuleAction::~nsMsgRuleAction()
{
}

nsMsgFilter::nsMsgFilter():
    m_type(1),
    m_temporary(PR_FALSE),
    m_filterList(nsnull)
{
	NS_INIT_ISUPPORTS();
  NS_NewISupportsArray(getter_AddRefs(m_termList));
}

nsMsgFilter::~nsMsgFilter()
{
}

NS_IMPL_ISUPPORTS1(nsMsgFilter, nsIMsgFilter)

NS_IMPL_GETTER(nsMsgFilter::GetFilterType, nsMsgFilterTypeType, m_type);
NS_IMPL_GETSET(nsMsgFilter, Enabled, PRBool, m_enabled);
NS_IMPL_GETSET(nsMsgFilter, Temporary, PRBool, m_temporary);
NS_IMPL_GETSET(nsMsgFilter, Action, nsMsgRuleActionType, m_action.m_type);

NS_IMETHODIMP nsMsgFilter::SetActionPriority(nsMsgPriorityValue aPriority)
{
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::ChangePriority,
                   NS_ERROR_ILLEGAL_VALUE);
    m_action.m_priority = aPriority;
    return NS_OK;
}

NS_IMETHODIMP
nsMsgFilter::GetActionPriority(nsMsgPriorityValue *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::ChangePriority,
                   NS_ERROR_ILLEGAL_VALUE);
    *aResult = m_action.m_priority;
    return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::SetActionLabel(nsMsgLabelValue aLabel)
{
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::Label,
                   NS_ERROR_ILLEGAL_VALUE);
    m_action.m_label = aLabel;
    return NS_OK;
}

NS_IMETHODIMP
nsMsgFilter::GetActionLabel(nsMsgLabelValue *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::Label,
                   NS_ERROR_ILLEGAL_VALUE);
    *aResult = m_action.m_label;
     return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::GetFilterName(PRUnichar **name)
{
  NS_ENSURE_ARG_POINTER(name);  
  *name = ToNewUnicode(m_filterName);
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::SetFilterName(const PRUnichar *name)
{
  m_filterName.Assign(name);
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::GetFilterDesc(char **description)
{
  NS_ENSURE_ARG_POINTER(description);
  *description = ToNewCString(m_description);
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::SetFilterDesc(const char *description)
{
  m_description.Assign(description);
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::AddTerm(     
	nsMsgSearchAttribValue attrib,    /* attribute for this term          */
	nsMsgSearchOpValue op,         /* operator e.g. opContains           */
	nsIMsgSearchValue *value,        /* value e.g. "Dogbert"               */
	PRBool BooleanAND, 	       /* PR_TRUE if AND is the boolean operator.
                                  PR_FALSE if OR is the boolean operators */
	const char * arbitraryHeader)  /* arbitrary header specified by user.
                                      ignored unless attrib = attribOtherHeader */
{
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::AppendTerm(nsIMsgSearchTerm * aTerm)
{
    NS_ENSURE_TRUE(aTerm, NS_ERROR_NULL_POINTER);
    
    return m_termList->AppendElement(NS_STATIC_CAST(nsISupports*,aTerm));
}

NS_IMETHODIMP
nsMsgFilter::CreateTerm(nsIMsgSearchTerm **aResult)
{
    nsMsgSearchTerm *term = new nsMsgSearchTerm;
    NS_ENSURE_TRUE(term, NS_ERROR_OUT_OF_MEMORY);

    *aResult = NS_STATIC_CAST(nsIMsgSearchTerm*,term);
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::GetTerm(PRInt32 termIndex, 
                                   nsMsgSearchAttribValue *attrib,    /* attribute for this term          */
                                   nsMsgSearchOpValue *op,         /* operator e.g. opContains           */
                                   nsIMsgSearchValue **value,         /* value e.g. "Dogbert"               */
                                   PRBool *booleanAnd, /* PR_TRUE if AND is the boolean operator. PR_FALSE if OR is the boolean operator */
                                   char ** arbitraryHeader) /* arbitrary header specified by user.ignore unless attrib = attribOtherHeader */
{
  nsresult rv;
  nsCOMPtr<nsIMsgSearchTerm> term;
  rv = m_termList->QueryElementAt(termIndex, NS_GET_IID(nsIMsgSearchTerm),
                                    (void **)getter_AddRefs(term));
	if (NS_SUCCEEDED(rv) && term)
	{
    if(attrib)
      term->GetAttrib(attrib);
    if(op)
      term->GetOp(op);
    if(value)
      term->GetValue(value);
    if(booleanAnd)
      term->GetBooleanAnd(booleanAnd);
    if (attrib && arbitraryHeader) {
		  if (*attrib > nsMsgSearchAttrib::OtherHeader && *attrib < nsMsgSearchAttrib::kNumMsgSearchAttributes)
        term->GetArbitraryHeader(arbitraryHeader);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::GetSearchTerms(nsISupportsArray **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_IF_ADDREF(*aResult = m_termList);
    return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::SetSearchTerms(nsISupportsArray *aSearchList)
{
    m_termList = aSearchList;
    return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::SetScope(nsIMsgSearchScopeTerm *aResult)
{
	m_scope = aResult;
	return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::GetScope(nsIMsgSearchScopeTerm **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_IF_ADDREF(*aResult = m_scope);
    return NS_OK;
}

NS_IMETHODIMP
    nsMsgFilter::SetActionTargetFolderUri(const char *aUri)
{
    nsresult rv=NS_OK;
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::MoveToFolder,
                   NS_ERROR_ILLEGAL_VALUE);
    if (aUri)
      m_action.m_folderUri = aUri;
    else
      SetEnabled(PR_FALSE);

    return rv;
}

NS_IMETHODIMP
nsMsgFilter::GetActionTargetFolderUri(char** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_TRUE(m_action.m_type == nsMsgFilterAction::MoveToFolder,
                   NS_ERROR_ILLEGAL_VALUE);
    if (m_action.m_folderUri.get())
      *aResult = ToNewCString(m_action.m_folderUri);
    return NS_OK;
}

#define LOG_ENTRY_START_TAG "<p>\n"
#define LOG_ENTRY_START_TAG_LEN (strlen(LOG_ENTRY_START_TAG))
#define LOG_ENTRY_END_TAG "</p>\n"
#define LOG_ENTRY_END_TAG_LEN (strlen(LOG_ENTRY_END_TAG))

NS_IMETHODIMP nsMsgFilter::LogRuleHit(nsIMsgDBHdr *aMsgHdr)
{
    nsCOMPtr <nsIOutputStream> logStream;
    nsresult rv = m_filterList->GetLogStream(getter_AddRefs(logStream));
    NS_ENSURE_SUCCESS(rv,rv);
  
    PRTime date;
    char dateStr[40];	/* 30 probably not enough */
    nsMsgRuleActionType actionType;
    nsXPIDLCString actionFolderUri;
    
    nsXPIDLCString author;
    nsXPIDLCString subject;
    nsXPIDLString filterName;

    GetFilterName(getter_Copies(filterName));
    GetAction(&actionType);
    rv = aMsgHdr->GetDate(&date);
    PRExplodedTime exploded;
    PR_ExplodeTime(date, PR_LocalTimeParameters, &exploded);
    PR_FormatTimeUSEnglish(dateStr, 100, "%m/%d/%Y %I:%M %p", &exploded);

    aMsgHdr->GetAuthor(getter_Copies(author));
    aMsgHdr->GetSubject(getter_Copies(subject));

    nsCString buffer;
    // this is big enough to hold a log entry.  
    // do this so we avoid growing and copying as we append to the log.
    buffer.SetCapacity(512);  
    
    buffer = "Applied filter \"";
    buffer +=  NS_ConvertUCS2toUTF8(filterName).get();
    buffer +=  "\" to message from ";
    buffer +=  (const char*)author;
    buffer +=  " - ";
    buffer +=  (const char *)subject;
    buffer +=  " at ";
    buffer +=  dateStr;
    buffer +=  "\n";
    const char *actionStr = GetActionStr(actionType);
    buffer +=  "Action = ";
    buffer +=  actionStr;
    buffer +=  " ";
        
    if (actionType == nsMsgFilterAction::MoveToFolder) {
      GetActionTargetFolderUri(getter_Copies(actionFolderUri));
      buffer += actionFolderUri.get();
    } 
         
    buffer += "\n";
    if (actionType == nsMsgFilterAction::MoveToFolder) {
      nsXPIDLCString msgId;
      aMsgHdr->GetMessageId(getter_Copies(msgId));
      buffer += (const char *) actionFolderUri;
      buffer += " id = ";
      buffer += (const char*)msgId;
      buffer += "\n";
    }
    
    PRUint32 writeCount;

    rv = logStream->Write(LOG_ENTRY_START_TAG, LOG_ENTRY_START_TAG_LEN, &writeCount);
    NS_ENSURE_SUCCESS(rv,rv);
    NS_ASSERTION(writeCount == LOG_ENTRY_START_TAG_LEN, "failed to write out start log tag");

    // html escape the log for security reasons.
    // we don't want some to send us a message with a subject with
    // html tags, especially <script>
    char *escapedBuffer = nsEscapeHTML(buffer.get());
    if (!escapedBuffer)
      return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 escapedBufferLen = strlen(escapedBuffer);
    rv = logStream->Write(escapedBuffer, escapedBufferLen, &writeCount);
    PR_FREEIF(escapedBuffer);
    NS_ENSURE_SUCCESS(rv,rv);
    NS_ASSERTION(writeCount == escapedBufferLen, "failed to write out log hit");
 
    rv = logStream->Write(LOG_ENTRY_END_TAG, LOG_ENTRY_END_TAG_LEN, &writeCount);
    NS_ENSURE_SUCCESS(rv,rv);
    NS_ASSERTION(writeCount == LOG_ENTRY_END_TAG_LEN, "failed to write out end log tag");
    return NS_OK;
}

NS_IMETHODIMP nsMsgFilter::MatchHdr(nsIMsgDBHdr	*msgHdr, nsIMsgFolder *folder, nsIMsgDatabase *db, 
									const char *headers, PRUint32 headersSize, PRBool *pResult)
{
    // use offlineMail because
    nsMsgSearchScopeTerm* scope = new nsMsgSearchScopeTerm(nsnull, nsMsgSearchScope::offlineMail, folder);
    if (!scope) return NS_ERROR_OUT_OF_MEMORY;

    nsXPIDLString folderCharset;
    folder->GetCharset(getter_Copies(folderCharset));
    nsresult rv = nsMsgSearchOfflineMail::MatchTermsForFilter(msgHdr, m_termList,
                                                           NS_ConvertUCS2toUTF8(folderCharset).get(),
                                                           scope,
                                                           db, 
                                                           headers,
                                                           headersSize,
														   pResult);
	delete scope;
	return rv;
}

void
nsMsgFilter::SetFilterList(nsIMsgFilterList *filterList)
{
    // hold weak ref
	m_filterList = filterList;
}

nsresult
nsMsgFilter::GetFilterList(nsIMsgFilterList **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_IF_ADDREF(*aResult = m_filterList);
    return NS_OK;
}

void nsMsgFilter::SetFilterScript(nsCString *fileName) 
{
	m_scriptFileName = *fileName;
}

nsresult nsMsgFilter::ConvertMoveToFolderValue(nsCString &moveValue)
{

  PRInt16 filterVersion = kFileVersion;
  if (m_filterList)
      m_filterList->GetVersion(&filterVersion);
  if (filterVersion <= k60Beta1Version)
  {
    nsCOMPtr <nsIImportService> impSvc = do_GetService(NS_IMPORTSERVICE_CONTRACTID);
    NS_ASSERTION(impSvc, "cannot get importService");
    nsCOMPtr <nsIMsgFolder> rootFolder;
    nsXPIDLCString folderUri;

    m_filterList->GetFolder(getter_AddRefs(rootFolder));

	  // if relative path starts with kImap, this is a move to folder on the same server
	  if (moveValue.Find(kImapPrefix) == 0)
	  {
		  PRInt32 prefixLen = PL_strlen(kImapPrefix);
		  moveValue.Mid(m_action.m_originalServerPath, prefixLen, moveValue.Length() - prefixLen);
          if ( filterVersion == k45Version && impSvc)
          {
            nsAutoString unicodeStr;
            impSvc->SystemStringToUnicode(m_action.m_originalServerPath.get(), unicodeStr);
            char *utfNewName = CreateUtf7ConvertedStringFromUnicode(unicodeStr.get());
            m_action.m_originalServerPath.Assign(utfNewName);
            nsCRT::free(utfNewName);
          }

      nsCOMPtr <nsIFolder> destIFolder;
      if (rootFolder)
      {
        rootFolder->FindSubFolder (m_action.m_originalServerPath.get(), getter_AddRefs(destIFolder));
        if (destIFolder)
        {
          nsCOMPtr <nsIMsgFolder> msgFolder;
          msgFolder = do_QueryInterface(destIFolder);	
          destIFolder->GetURI(getter_Copies(folderUri));
		  m_action.m_folderUri.Assign(folderUri);
          moveValue.Assign(folderUri);
        }
      }
    }
	  else
    {
      // start off leaving the value the same.
      m_action.m_folderUri = moveValue;
      nsresult rv = NS_OK;

      nsCOMPtr <nsIFolder> localMailRoot;
      rootFolder->GetURI(getter_Copies(folderUri));
      // if the root folder is not imap, than the local mail root is the server root.
      // otherwise, it's the migrated local folders.
      if (nsCRT::strncmp("imap:", folderUri, 5))
      {
        localMailRoot = rootFolder;
      }
      else
      {
        nsCOMPtr<nsIMsgAccountManager> accountManager = 
                 do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv))
        {
          nsCOMPtr <nsIMsgIncomingServer> server; 
          rv = accountManager->GetLocalFoldersServer(getter_AddRefs(server)); 
          if (NS_SUCCEEDED(rv) && server)
          {
            rv = server->GetRootFolder(getter_AddRefs(localMailRoot));
          }
        }
      }
      if (NS_SUCCEEDED(rv) && localMailRoot)
      {
        nsXPIDLCString localRootURI;
        nsCOMPtr <nsIMsgFolder> destIMsgFolder;
        nsCOMPtr <nsIMsgFolder> localMailRootMsgFolder = do_QueryInterface(localMailRoot);
        localMailRoot->GetURI(getter_Copies(localRootURI));
        nsCString destFolderUri;
	      destFolderUri.Assign( localRootURI);
        // need to remove ".sbd" from moveValue, and perhaps escape it.
        moveValue.ReplaceSubstring(".sbd/", "/");

#ifdef XP_MAC
        char *unescapedMoveValue = ToNewCString(moveValue);
        nsUnescape(unescapedMoveValue);
        moveValue.Assign(unescapedMoveValue);
        nsCRT::free(unescapedMoveValue);
#endif
        destFolderUri.Append('/');
        if ( filterVersion == k45Version && impSvc)
        {
          nsAutoString unicodeStr;
          impSvc->SystemStringToUnicode(moveValue.get(), unicodeStr);
          nsXPIDLCString escapedName;
          rv =NS_MsgEscapeEncodeURLPath(unicodeStr.get(), getter_Copies(escapedName));
          if (NS_SUCCEEDED(rv) && escapedName)
            moveValue.Assign(escapedName.get());
        }
        destFolderUri.Append(moveValue);
        localMailRootMsgFolder->GetChildWithURI (destFolderUri.get(), PR_TRUE, PR_FALSE /*caseInsensitive*/, getter_AddRefs(destIMsgFolder));

        if (destIMsgFolder)
        {
          destIMsgFolder->GetURI(getter_Copies(folderUri));
		  m_action.m_folderUri.Assign(folderUri);
          moveValue.Assign(folderUri);
        }
      }
    }
  }
  else
    SetActionTargetFolderUri(moveValue.get());
    
	return NS_OK;
	// set m_action.m_value.m_folderUri
}

nsresult nsMsgFilter::SaveToTextFile(nsIOFileStream *aStream)
{
  nsresult err = m_filterList->WriteWstrAttr(nsIMsgFilterList::attribName, m_filterName.get(), aStream);
  err = m_filterList->WriteBoolAttr(nsIMsgFilterList::attribEnabled, m_enabled, aStream);
  err = m_filterList->WriteStrAttr(nsIMsgFilterList::attribDescription, m_description.get(), aStream);
  err = m_filterList->WriteIntAttr(nsIMsgFilterList::attribType, m_type, aStream);
  if (IsScript())
    err = m_filterList->WriteStrAttr(nsIMsgFilterList::attribScriptFile, m_scriptFileName.get(), aStream);
  else
    err = SaveRule(aStream);
  return err;
}

nsresult nsMsgFilter::SaveRule(nsIOFileStream *aStream)
{
  nsresult err = NS_OK;
  nsCOMPtr<nsIMsgFilterList> filterList;
  GetFilterList(getter_AddRefs(filterList));
  nsCAutoString	actionFilingStr;
  
  GetActionFilingStr(m_action.m_type, actionFilingStr);
  
  err = filterList->WriteStrAttr(nsIMsgFilterList::attribAction, actionFilingStr.get(), aStream);
  NS_ENSURE_SUCCESS(err, err);
  
  switch(m_action.m_type)
  {
  case nsMsgFilterAction::MoveToFolder:
    {
      nsCAutoString imapTargetString(m_action.m_folderUri);
      err = filterList->WriteStrAttr(nsIMsgFilterList::attribActionValue, imapTargetString.get(), aStream);
    }
    break;
  case nsMsgFilterAction::ChangePriority:
    {
      nsAutoString priority;
      NS_MsgGetUntranslatedPriorityName (m_action.m_priority, &priority);
      nsCAutoString cStr;
      cStr.AssignWithConversion(priority);
      err = filterList->WriteStrAttr(nsIMsgFilterList::attribActionValue, cStr.get(), aStream);
    }
    break;
  case nsMsgFilterAction::Label:
      err = filterList->WriteIntAttr(nsIMsgFilterList::attribActionValue, m_action.m_label, aStream);
      break;
  default:
    break;
  }
  // and here the fun begins - file out term list...
  PRUint32 searchIndex;
  nsCAutoString  condition;
  PRUint32 count;
  m_termList->Count(&count);
  for (searchIndex = 0; searchIndex < count && NS_SUCCEEDED(err);
  searchIndex++)
  {
    nsCAutoString	stream;
    
    nsCOMPtr<nsIMsgSearchTerm> term;
    m_termList->QueryElementAt(searchIndex, NS_GET_IID(nsIMsgSearchTerm),
      (void **)getter_AddRefs(term));
    if (!term)
      continue;
    
    if (condition.Length() > 1)
      condition += ' ';
    
    PRBool booleanAnd;
    term->GetBooleanAnd(&booleanAnd);
    if (booleanAnd)
      condition += "AND (";
    else
      condition += "OR (";
    
    nsresult searchError = term->EnStreamNew(stream);
    if (NS_FAILED(searchError))
    {
      err = searchError;
      break;
    }
    
    condition += stream;
    condition += ')';
  }
  if (NS_SUCCEEDED(err))
    err = filterList->WriteStrAttr(nsIMsgFilterList::attribCondition, condition.get(), aStream);
  return err;
}

// for each action, this table encodes the filterTypes that support the action.
struct RuleActionsTableEntry
{
	nsMsgRuleActionType	action;
	nsMsgFilterTypeType		supportedTypes;
	PRInt32				xp_strIndex;
	const char			*actionFilingStr;	/* used for filing out filters, don't translate! */
};

// Because HP_UX native C++ compiler can't initialize static objects with ints,
//  we can't initialize this structure directly, so we have to do it in two phases.
static struct RuleActionsTableEntry ruleActionsTable[] =
{
  { nsMsgFilterAction::MoveToFolder,    nsMsgFilterType::Inbox, 0, /*XP_FILTER_MOVE_TO_FOLDER*/   "Move to folder"},
  { nsMsgFilterAction::ChangePriority,  nsMsgFilterType::Inbox, 0, /*XP_FILTER_CHANGE_PRIORITY*/  "Change priority"},
  { nsMsgFilterAction::Delete,          nsMsgFilterType::All,   0, /*XP_FILTER_DELETE */          "Delete"},
  { nsMsgFilterAction::MarkRead,        nsMsgFilterType::All,   0, /*XP_FILTER_MARK_READ */       "Mark read"},
  { nsMsgFilterAction::KillThread,      nsMsgFilterType::All,   0, /*XP_FILTER_KILL_THREAD */     "Ignore thread"},
  { nsMsgFilterAction::WatchThread,     nsMsgFilterType::All,   0, /*XP_FILTER_WATCH_THREAD */    "Watch thread"},
  { nsMsgFilterAction::MarkFlagged,     nsMsgFilterType::All,   0, /*XP_FILTER_MARK_FLAGGED */    "Mark flagged"},
  { nsMsgFilterAction::Label,           nsMsgFilterType::All,   0, /*XP_FILTER_LABEL */           "Label"}
};

const char *nsMsgFilter::GetActionStr(nsMsgRuleActionType action)
{
  int	numActions = sizeof(ruleActionsTable) / sizeof(ruleActionsTable[0]);
  
  for (int i = 0; i < numActions; i++)
  {
    if (action == ruleActionsTable[i].action)
      return ruleActionsTable[i].actionFilingStr; // XP_GetString(ruleActionsTable[i].xp_strIndex);
  }
  return "";
}
/*static */nsresult nsMsgFilter::GetActionFilingStr(nsMsgRuleActionType action, nsCString &actionStr)
{
  int	numActions = sizeof(ruleActionsTable) / sizeof(ruleActionsTable[0]);
  
  for (int i = 0; i < numActions; i++)
  {
    if (action == ruleActionsTable[i].action)
    {
      actionStr = ruleActionsTable[i].actionFilingStr;
      return NS_OK;
    }
  }
  return NS_ERROR_INVALID_ARG;
}


nsMsgRuleActionType nsMsgFilter::GetActionForFilingStr(nsCString &actionStr)
{
	int	numActions = sizeof(ruleActionsTable) / sizeof(ruleActionsTable[0]);

	for (int i = 0; i < numActions; i++)
	{
		if (actionStr.Equals(ruleActionsTable[i].actionFilingStr))
			return ruleActionsTable[i].action;
	}
	return nsMsgFilterAction::None;
}

PRInt16
nsMsgFilter::GetVersion()
{
    if (!m_filterList) return 0;
    PRInt16 version;
    m_filterList->GetVersion(&version);
    return version;
}

#ifdef DEBUG
void nsMsgFilter::Dump()
{
	nsCString s;
	CopyUCS2toASCII(m_filterName, s);
	printf("filter %s type = %c desc = %s\n", s.get(), m_type + '0', m_description.get());
}
#endif

