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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/*
 * The offline manager service - manages going online and offline, and synchronization
 */
#include "msgCore.h"
#include "nsMsgOfflineManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsMsgBaseCID.h"
#include "nsIImapService.h"
#include "nsMsgImapCID.h"
#include "nsIMsgSendLater.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgCompCID.h"
#include "nsIIOService.h"
#include "nsMsgNewsCID.h"
#include "nsINntpService.h"

static NS_DEFINE_CID(kCImapService, NS_IMAPSERVICE_CID);
static NS_DEFINE_CID(kCMsgAccountManagerCID, NS_MSGACCOUNTMANAGER_CID);
static NS_DEFINE_CID(kMsgSendLaterCID, NS_MSGSENDLATER_CID); 
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kNntpServiceCID, NS_NNTPSERVICE_CID);

NS_IMPL_THREADSAFE_ISUPPORTS5(nsMsgOfflineManager,
                              nsIMsgOfflineManager,
                              nsIMsgSendLaterListener,
                              nsIObserver,
                              nsISupportsWeakReference,
                              nsIUrlListener)

nsMsgOfflineManager::nsMsgOfflineManager() :
  m_inProgress (PR_FALSE),
  m_sendUnsentMessages(PR_FALSE),
  m_downloadNews(PR_FALSE),
  m_downloadMail(PR_FALSE),
  m_playbackOfflineImapOps(PR_FALSE),
  m_goOfflineWhenDone(PR_FALSE),
  m_curState(eNoState),
  m_curOperation(eNoOp)
{
  NS_INIT_REFCNT();
}

nsMsgOfflineManager::~nsMsgOfflineManager()
{
}

/* attribute nsIMsgWindow window; */
NS_IMETHODIMP nsMsgOfflineManager::GetWindow(nsIMsgWindow * *aWindow)
{
  NS_ENSURE_ARG(aWindow);
  *aWindow = m_window;
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}
NS_IMETHODIMP nsMsgOfflineManager::SetWindow(nsIMsgWindow * aWindow)
{
  m_window = aWindow;
  if (m_window)
    m_window->GetStatusFeedback(getter_AddRefs(m_statusFeedback));
  else
    m_statusFeedback = nsnull;
  return NS_OK;
}

/* attribute boolean inProgress; */
NS_IMETHODIMP nsMsgOfflineManager::GetInProgress(PRBool *aInProgress)
{
  NS_ENSURE_ARG(aInProgress);
  *aInProgress = m_inProgress;
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::SetInProgress(PRBool aInProgress)
{
  m_inProgress = aInProgress;
  return NS_OK;
}

nsresult nsMsgOfflineManager::StopRunning(nsresult exitStatus)
{
  m_inProgress = PR_FALSE;
  return exitStatus;
}

nsresult nsMsgOfflineManager::AdvanceToNextState(nsresult exitStatus)
{
  if (!NS_SUCCEEDED(exitStatus))
  {
    return StopRunning(exitStatus);
  }
  if (m_curOperation == eGoingOnline)
  {
    switch (m_curState)
    {
    case eNoState:

      if (m_sendUnsentMessages)
      {
        m_curState = eSendingUnsent;
        SendUnsentMessages();
      }
      else
        AdvanceToNextState(NS_OK);
      break;
    case eSendingUnsent:

      m_curState = eSynchronizingOfflineImapChanges;
      if (m_playbackOfflineImapOps)
        return SynchronizeOfflineImapChanges();
      else
        AdvanceToNextState(NS_OK); // recurse to next state.
      break;
    case eSynchronizingOfflineImapChanges:
      m_curState = eDone;
      return StopRunning(exitStatus);
      default:
        NS_ASSERTION(PR_FALSE, "unhandled current state when going online");
    }
  }
  else if (m_curOperation == eDownloadingForOffline)
  {
    switch (m_curState)
    {
      case eNoState:
        m_curState = eDownloadingNews;
        if (m_downloadNews)
          DownloadOfflineNewsgroups();
        else
          AdvanceToNextState(NS_OK);
        break;
      case eSendingUnsent:
        if (m_goOfflineWhenDone)
        {
          SetOnlineState(PR_FALSE);
        }
        break;
      case eDownloadingNews:
        m_curState = eDownloadingMail;
        if (m_downloadMail)
          DownloadMail();
        else
          AdvanceToNextState(NS_OK);
        break;
      case eDownloadingMail:
        m_curState = eSendingUnsent;
        if (m_sendUnsentMessages)
          SendUnsentMessages();
        else
          AdvanceToNextState(NS_OK);
        break;
      default:
        NS_ASSERTION(PR_FALSE, "unhandled current state when downloading for offline");
    }

  }
  return NS_OK;
}

nsresult nsMsgOfflineManager::SynchronizeOfflineImapChanges()
{
  nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return imapService->PlaybackAllOfflineOperations(m_window, this);
}

nsresult nsMsgOfflineManager::SendUnsentMessages()
{
	nsresult rv;
	nsCOMPtr<nsIMsgSendLater> pMsgSendLater = do_CreateInstance(kMsgSendLaterCID, &rv); 
  NS_ENSURE_SUCCESS(rv, rv);
  NS_WITH_SERVICE(nsIMsgAccountManager,accountManager,kCMsgAccountManagerCID,&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  // now we have to iterate over the identities, finding the *unique* unsent messages folder
  // for each one, determine if they have unsent messages, and if so, add them to the list
  // of identities to send unsent messages from.
  // However, I think there's only ever one unsent messages folder at the moment,
  // so I think we'll go with that for now.
  nsCOMPtr<nsISupportsArray> identities;

  if (NS_SUCCEEDED(rv) && accountManager) 
  {
    rv = accountManager->GetAllIdentities(getter_AddRefs(identities));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr <nsIMsgIdentity> identityToUse;
  PRUint32 numIndentities;
  identities->Count(&numIndentities);
  for (PRUint32 i = 0; i < numIndentities; i++)
  {
    // convert supports->Identity
    nsCOMPtr<nsISupports> thisSupports;
    rv = identities->GetElementAt(i, getter_AddRefs(thisSupports));
    if (NS_FAILED(rv)) continue;
    
    nsCOMPtr<nsIMsgIdentity> thisIdentity = do_QueryInterface(thisSupports, &rv);

    if (NS_SUCCEEDED(rv) && thisIdentity)
    {
      nsCOMPtr <nsIMsgFolder> outboxFolder;
      pMsgSendLater->GetUnsentMessagesFolder(thisIdentity, getter_AddRefs(outboxFolder));
      if (outboxFolder)
      {
        PRInt32 numMessages;
        outboxFolder->GetTotalMessages(PR_FALSE, &numMessages);
        if (numMessages > 0)
        {
          identityToUse = thisIdentity;
          break;
        }
      }
    }
  }
	if (identityToUse) 
	{ 
    pMsgSendLater->AddListener(this);
    pMsgSendLater->SetMsgWindow(m_window);
    rv = pMsgSendLater->SendUnsentMessages(identityToUse); 
    // if we succeeded, return - we'll run the next operation when the
    // send finishes. Otherwise, advance to the next state.
    if (NS_SUCCEEDED(rv))
      return rv;
	} 
	return AdvanceToNextState(rv);

}

nsresult nsMsgOfflineManager::DownloadOfflineNewsgroups()
{
	nsresult rv;
  NS_WITH_SERVICE(nsINntpService, nntpService, kNntpServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && nntpService)
    rv = nntpService->DownloadNewsgroupsForOffline(m_window, this);

  if (!NS_SUCCEEDED(rv))
    return AdvanceToNextState(rv);
  return rv;
}

nsresult nsMsgOfflineManager::DownloadMail()
{
  nsresult rv = NS_OK;
	NS_WITH_SERVICE(nsIImapService, imapService, kCImapService, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return imapService->DownloadAllOffineImapFolders(m_window, this);
  // ### we should do get new mail on pop servers, and download imap messages for offline use.
}

/* void goOnline (in boolean sendUnsentMessages, in boolean playbackOfflineImapOperations, in nsIMsgWindow aMsgWindow); */
NS_IMETHODIMP nsMsgOfflineManager::GoOnline(PRBool sendUnsentMessages, PRBool playbackOfflineImapOperations, nsIMsgWindow *aMsgWindow)
{
  m_sendUnsentMessages = sendUnsentMessages;
  m_playbackOfflineImapOps = playbackOfflineImapOperations;
  m_curOperation = eGoingOnline;
  SetWindow(aMsgWindow);
  if (!m_sendUnsentMessages && !playbackOfflineImapOperations)
  {
    return SetOnlineState(PR_TRUE);
  }
  else
    AdvanceToNextState(NS_OK);
  return NS_OK;
}

/* void synchronizeForOffline (in boolean downloadNews, in boolean downloadMail, in boolean sendUnsentMessages, in boolean goOfflineWhenDone, in nsIMsgWindow aMsgWindow); */
NS_IMETHODIMP nsMsgOfflineManager::SynchronizeForOffline(PRBool downloadNews, PRBool downloadMail, PRBool sendUnsentMessages, PRBool goOfflineWhenDone, nsIMsgWindow *aMsgWindow)
{
  m_curOperation = eDownloadingForOffline;
	nsresult rv = NS_OK;
  m_downloadNews = downloadNews;
  m_downloadMail = downloadMail;
  m_sendUnsentMessages = sendUnsentMessages;
  m_window = aMsgWindow;
  m_goOfflineWhenDone = goOfflineWhenDone;
  m_curState = eNoState;
  if (!downloadNews && !downloadMail && !sendUnsentMessages)
  {
    if (goOfflineWhenDone)
      return SetOnlineState(PR_FALSE);
  }
  else
    return AdvanceToNextState(NS_OK);
  return NS_OK;
}

nsresult nsMsgOfflineManager::SetOnlineState(PRBool online)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIIOService, netService, kIOServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && netService)
  {
    rv = netService->SetOffline(!online);
  }
  return rv;
}

  // nsIUrlListener methods

NS_IMETHODIMP
nsMsgOfflineManager::OnStartRunningUrl(nsIURI * aUrl)
{
    return NS_OK;
}

NS_IMETHODIMP
nsMsgOfflineManager::OnStopRunningUrl(nsIURI * aUrl, nsresult aExitCode)
{
  AdvanceToNextState(aExitCode);
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
  return NS_OK;
}

// nsIMsgSendLaterListener implementation 
NS_IMETHODIMP nsMsgOfflineManager::OnStartSending(PRUint32 aTotalMessageCount)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnProgress(PRUint32 aCurrentMessage, PRUint32 aTotalMessage)
{
  if (m_statusFeedback && aTotalMessage)
    return m_statusFeedback->ShowProgress ((100 * aCurrentMessage) / aTotalMessage);
  else
    return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnStatus(const PRUnichar *aMsg)
{
  if (m_statusFeedback && aMsg)
    return m_statusFeedback->ShowStatusString (aMsg);
  else
    return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnStopSending(nsresult aStatus, const PRUnichar *aMsg, PRUint32 aTotalTried, 
                                 PRUint32 aSuccessful) 
{
#ifdef NS_DEBUG
  if (NS_SUCCEEDED(aStatus))
    printf("SendLaterListener::OnStopSending: Tried to send %d messages. %d successful.\n",
            aTotalTried, aSuccessful);
#endif
  return AdvanceToNextState(aStatus);
}
