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

#ifndef nsImapProxyEvent_h__
#define nsImapProxyEvent_h__

#include "plevent.h"
#include "prthread.h"
#include "nsISupports.h"
#include "nsIURL.h"
#include "nsIImapLog.h"
#include "nsIImapMailFolderSink.h"
#include "nsIImapMessageSink.h"
#include "nsIImapExtensionSink.h"
#include "nsIImapMiscellaneousSink.h"
#include "nsIImapIncomingServer.h"
#include "nsCOMPtr.h"
class nsImapProxyBase
{
public:
    nsImapProxyBase(nsIImapProtocol* aProtocol,
                    nsIEventQueue* aEventQ,
                    PRThread* aThread);
    virtual ~nsImapProxyBase();

    nsIEventQueue* m_eventQueue;
    PRThread* m_thread;
    nsIImapProtocol* m_protocol;
};

class nsImapLogProxy : public nsIImapLog, 
                       public nsImapProxyBase
{
public:
	nsImapLogProxy(nsIImapLog* aImapLog,
                 nsIImapProtocol* aProtocol,
                 nsIEventQueue* aEventQ,
                 PRThread* aThread);
	virtual ~nsImapLogProxy();

	NS_DECL_ISUPPORTS

	NS_IMETHOD HandleImapLogData(const char* aLogData);

	nsIImapLog* m_realImapLog;
};

class nsImapMailFolderSinkProxy : public nsIImapMailFolderSink, 
                              public nsImapProxyBase
{
public:
    nsImapMailFolderSinkProxy(nsIImapMailFolderSink* aImapMailFolderSink,
                          nsIImapProtocol* aProtocol,
                          nsIEventQueue* aEventQ,
                          PRThread* aThread);
    virtual ~nsImapMailFolderSinkProxy();
    
    NS_DECL_ISUPPORTS

    // Tell mail master about the newly selected mailbox
    NS_IMETHOD UpdateImapMailboxInfo(nsIImapProtocol* aProtocol,
                                     mailbox_spec* aSpec);
    NS_IMETHOD UpdateImapMailboxStatus(nsIImapProtocol* aProtocol,
                                       mailbox_spec* aSpec);
    NS_IMETHOD ChildDiscoverySucceeded(nsIImapProtocol* aProtocol);
    NS_IMETHOD PromptUserForSubscribeUpdatePath(nsIImapProtocol* aProtocol,
                                                PRBool* aBool);

    NS_IMETHOD SetupHeaderParseStream(nsIImapProtocol* aProtocol,
                                   StreamInfo* aStreamInfo);

    NS_IMETHOD ParseAdoptedHeaderLine(nsIImapProtocol* aProtocol,
                                   msg_line_info* aMsgLineInfo);
    
    NS_IMETHOD NormalEndHeaderParseStream(nsIImapProtocol* aProtocol);
    
    NS_IMETHOD AbortHeaderParseStream(nsIImapProtocol* aProtocol);
    
    nsIImapMailFolderSink* m_realImapMailFolderSink;
};

class nsImapExtensionSinkProxy : public nsIImapExtensionSink, 
                             public nsImapProxyBase
{
public:
    nsImapExtensionSinkProxy(nsIImapExtensionSink* aImapExtensionSink,
                         nsIImapProtocol* aProtocol,
                         nsIEventQueue* aEventQ,
                         PRThread* aThread);
    virtual ~nsImapExtensionSinkProxy();

    NS_DECL_ISUPPORTS
  
    NS_IMETHOD ClearFolderRights(nsIImapProtocol* aProtocol,
                                 nsIMAPACLRightsInfo* aclRights);
    NS_IMETHOD AddFolderRights(nsIImapProtocol* aProtocol,
                             nsIMAPACLRightsInfo* aclRights);
    NS_IMETHOD RefreshFolderRights(nsIImapProtocol* aProtocol,
                                   nsIMAPACLRightsInfo* aclRights);
    NS_IMETHOD FolderNeedsACLInitialized(nsIImapProtocol* aProtocol,
                                         nsIMAPACLRightsInfo* aclRights);
    NS_IMETHOD SetCopyResponseUid(nsIImapProtocol* aProtocol,
                                  nsMsgKeyArray* aKeyArray,
                                  const char* msgIdString,
                                  nsISupports* copyState);
    NS_IMETHOD SetAppendMsgUid(nsIImapProtocol* aProtocol,
                               nsMsgKey aKey,
                               nsISupports* copyState);
    NS_IMETHOD GetMessageId(nsIImapProtocol* aProtocol,
                            nsCString* messageId,
                            nsISupports* copyState);
    
    nsIImapExtensionSink* m_realImapExtensionSink;
};

class nsImapMiscellaneousSinkProxy : public nsIImapMiscellaneousSink, 
                                 public nsImapProxyBase
{
public:
    nsImapMiscellaneousSinkProxy (nsIImapMiscellaneousSink* aImapMiscellaneousSink,
                              nsIImapProtocol* aProtocol,
                              nsIEventQueue* aEventQ,
                              PRThread* aThread);
    ~nsImapMiscellaneousSinkProxy ();

    NS_DECL_ISUPPORTS
	
    NS_IMETHOD AddSearchResult(nsIImapProtocol* aProtocol, 
														 const char* searchHitLine);
    NS_IMETHOD GetArbitraryHeaders(nsIImapProtocol* aProtocol,
                                   GenericInfo* aInfo);
    NS_IMETHOD GetShouldDownloadArbitraryHeaders(nsIImapProtocol* aProtocol,
                                                 GenericInfo* aInfo);
    NS_IMETHOD GetShowAttachmentsInline(nsIImapProtocol* aProtocol,
                                        PRBool* aBool);
    NS_IMETHOD HeaderFetchCompleted(nsIImapProtocol* aProtocol);
    NS_IMETHOD UpdateSecurityStatus(nsIImapProtocol* aProtocol);
    // ****
    NS_IMETHOD SetBiffStateAndUpdate(nsIImapProtocol* aProtocol,
                                     nsMsgBiffState biffState);
    NS_IMETHOD GetStoredUIDValidity(nsIImapProtocol* aProtocol,
                                    uid_validity_info* aInfo);
    NS_IMETHOD LiteSelectUIDValidity(nsIImapProtocol* aProtocol,
                                     PRUint32 uidValidity);
	NS_IMETHOD ProgressStatus(nsIImapProtocol* aProtocol,
                              PRUint32 statusMsgId, const char *extraInfo);
    NS_IMETHOD PercentProgress(nsIImapProtocol* aProtocol,
                               ProgressInfo* aInfo);
    NS_IMETHOD TunnelOutStream(nsIImapProtocol* aProtocol,
														 msg_line_info* aInfo);
    NS_IMETHOD ProcessTunnel(nsIImapProtocol* aProtocol,
                             TunnelInfo *aInfo);
    NS_IMETHOD CopyNextStreamMessage(nsIImapProtocol* aProtocl,
                                     nsISupports* copyState);

    NS_IMETHOD SetUrlState(nsIImapProtocol* aProtocol,
                           nsIMsgMailNewsUrl* aUrl,
                           PRBool isRunning,
                           nsresult statusCode);

    nsIImapMiscellaneousSink* m_realImapMiscellaneousSink;
};

/* ******* Imap Base Event struct ******** */
struct nsImapEvent : public PLEvent
{
  nsImapEvent();
	virtual ~nsImapEvent();
	virtual void InitEvent();

	NS_IMETHOD HandleEvent() = 0;
	void PostEvent(nsIEventQueue* aEventQ);
  virtual void SetNotifyCompletion(PRBool notifyCompletion);
  
	static void PR_CALLBACK imap_event_handler(PLEvent* aEvent);
	static void PR_CALLBACK imap_event_destructor(PLEvent *aEvent);
  PRBool m_notifyCompletion;
};

struct nsImapLogProxyEvent : public nsImapEvent
{
	nsImapLogProxyEvent(nsImapLogProxy* aProxy, 
                      const char* aLogData);
	virtual ~nsImapLogProxyEvent();

	NS_IMETHOD HandleEvent();
	char *m_logData;
	nsImapLogProxy *m_proxy;
};

struct nsImapMailFolderSinkProxyEvent : public nsImapEvent
{
    nsImapMailFolderSinkProxyEvent(nsImapMailFolderSinkProxy* aProxy);
    virtual ~nsImapMailFolderSinkProxyEvent();
    nsImapMailFolderSinkProxy* m_proxy;
};

struct UpdateImapMailboxInfoProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    UpdateImapMailboxInfoProxyEvent(nsImapMailFolderSinkProxy* aProxy,
                                    mailbox_spec* aSpec);
    virtual ~UpdateImapMailboxInfoProxyEvent();
    NS_IMETHOD HandleEvent();
    mailbox_spec m_mailboxSpec;
};

struct UpdateImapMailboxStatusProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    UpdateImapMailboxStatusProxyEvent(nsImapMailFolderSinkProxy* aProxy,
                                      mailbox_spec* aSpec);
    virtual ~UpdateImapMailboxStatusProxyEvent();
    NS_IMETHOD HandleEvent();
    mailbox_spec m_mailboxSpec;
};

struct ChildDiscoverySucceededProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    ChildDiscoverySucceededProxyEvent(nsImapMailFolderSinkProxy* aProxy);
    virtual ~ChildDiscoverySucceededProxyEvent();
    NS_IMETHOD HandleEvent();
};

struct PromptUserForSubscribeUpdatePathProxyEvent : 
    public nsImapMailFolderSinkProxyEvent 
{
    PromptUserForSubscribeUpdatePathProxyEvent(nsImapMailFolderSinkProxy* aProxy,
                                               PRBool* aBool);
    virtual ~PromptUserForSubscribeUpdatePathProxyEvent();
    NS_IMETHOD HandleEvent();
    PRBool m_bool;
};

struct SetupHeaderParseStreamProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    SetupHeaderParseStreamProxyEvent(nsImapMailFolderSinkProxy* aProxy,
                               StreamInfo* aStreamInfo);
    virtual ~SetupHeaderParseStreamProxyEvent();
    NS_IMETHOD HandleEvent();
    StreamInfo m_streamInfo;
};

struct NormalEndHeaderParseStreamProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    NormalEndHeaderParseStreamProxyEvent(nsImapMailFolderSinkProxy* aProxyo);
    virtual ~NormalEndHeaderParseStreamProxyEvent();
    NS_IMETHOD HandleEvent();
};

struct ParseAdoptedHeaderLineProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    ParseAdoptedHeaderLineProxyEvent(nsImapMailFolderSinkProxy* aProxy,
                               msg_line_info* aMsgLineInfo);
    virtual ~ParseAdoptedHeaderLineProxyEvent();
    NS_IMETHOD HandleEvent();
    msg_line_info m_msgLineInfo;
};

struct AbortHeaderParseStreamProxyEvent : public nsImapMailFolderSinkProxyEvent
{
    AbortHeaderParseStreamProxyEvent(nsImapMailFolderSinkProxy* aProxy);
    virtual ~AbortHeaderParseStreamProxyEvent();
    NS_IMETHOD HandleEvent();
};

struct nsImapExtensionSinkProxyEvent : nsImapEvent
{
    nsImapExtensionSinkProxyEvent(nsImapExtensionSinkProxy* aProxy);
    virtual ~nsImapExtensionSinkProxyEvent();
    nsImapExtensionSinkProxy* m_proxy;
};

struct ClearFolderRightsProxyEvent : nsImapExtensionSinkProxyEvent
{
    ClearFolderRightsProxyEvent(nsImapExtensionSinkProxy* aProxy,
                                nsIMAPACLRightsInfo* aclRights);
    virtual ~ClearFolderRightsProxyEvent();
    NS_IMETHOD HandleEvent();
    nsIMAPACLRightsInfo m_aclRightsInfo;
};

struct AddFolderRightsProxyEvent : nsImapExtensionSinkProxyEvent
{
    AddFolderRightsProxyEvent(nsImapExtensionSinkProxy* aProxy,
                                nsIMAPACLRightsInfo* aclRights);
    virtual ~AddFolderRightsProxyEvent();
    NS_IMETHOD HandleEvent();
    nsIMAPACLRightsInfo m_aclRightsInfo;
};

struct RefreshFolderRightsProxyEvent : nsImapExtensionSinkProxyEvent
{
    RefreshFolderRightsProxyEvent(nsImapExtensionSinkProxy* aProxy,
                                nsIMAPACLRightsInfo* aclRights);
    virtual ~RefreshFolderRightsProxyEvent();
    NS_IMETHOD HandleEvent();
    nsIMAPACLRightsInfo m_aclRightsInfo;
};

struct FolderNeedsACLInitializedProxyEvent : nsImapExtensionSinkProxyEvent
{
    FolderNeedsACLInitializedProxyEvent(nsImapExtensionSinkProxy* aProxy,
                                nsIMAPACLRightsInfo* aclRights);
    virtual ~FolderNeedsACLInitializedProxyEvent();
    NS_IMETHOD HandleEvent();
    nsIMAPACLRightsInfo m_aclRightsInfo;
};

struct SetCopyResponseUidProxyEvent : nsImapExtensionSinkProxyEvent
{
    SetCopyResponseUidProxyEvent(nsImapExtensionSinkProxy* aProxy,
                                 nsMsgKeyArray* aKeyArray, 
                                 const char* msgIdString,
                                 nsISupports* copyState);
    virtual ~SetCopyResponseUidProxyEvent();
    NS_IMETHOD HandleEvent();
    nsMsgKeyArray m_copyKeyArray;
    nsCString m_msgIdString;
    nsCOMPtr<nsISupports> m_copyState;
};

struct SetAppendMsgUidProxyEvent : nsImapExtensionSinkProxyEvent
{
    SetAppendMsgUidProxyEvent(nsImapExtensionSinkProxy* aProxy,
                              nsMsgKey aKey, nsISupports* copyState);
    virtual ~SetAppendMsgUidProxyEvent();
    NS_IMETHOD HandleEvent();
    nsMsgKey m_key;
    nsCOMPtr<nsISupports> m_copyState;
};

struct GetMessageIdProxyEvent : nsImapExtensionSinkProxyEvent
{
    GetMessageIdProxyEvent(nsImapExtensionSinkProxy* aProxy,
                           nsCString* messageId, nsISupports* copyState);
    virtual ~GetMessageIdProxyEvent();
    NS_IMETHOD HandleEvent();
    nsCString* m_messageId;
    nsCOMPtr<nsISupports> m_copyState;
};

struct nsImapMiscellaneousSinkProxyEvent : public nsImapEvent
{
    nsImapMiscellaneousSinkProxyEvent(nsImapMiscellaneousSinkProxy* aProxy);
    virtual ~nsImapMiscellaneousSinkProxyEvent();
    nsImapMiscellaneousSinkProxy* m_proxy;
};

struct AddSearchResultProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    AddSearchResultProxyEvent(nsImapMiscellaneousSinkProxy* aProxy, 
                              const char* searchHitLine);
    virtual ~AddSearchResultProxyEvent();
    NS_IMETHOD HandleEvent();
    char* m_searchHitLine;
};

struct GetArbitraryHeadersProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    GetArbitraryHeadersProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                                  GenericInfo* aInfo);
    virtual ~GetArbitraryHeadersProxyEvent();
    NS_IMETHOD HandleEvent();
    GenericInfo *m_info;        // pass in handle we don't own it
};

struct GetShouldDownloadArbitraryHeadersProxyEvent : 
    public nsImapMiscellaneousSinkProxyEvent
{
    GetShouldDownloadArbitraryHeadersProxyEvent(
        nsImapMiscellaneousSinkProxy* aProxy, GenericInfo* aInfo);
    virtual ~GetShouldDownloadArbitraryHeadersProxyEvent();
    NS_IMETHOD HandleEvent();
    GenericInfo *m_info;        // pass in handle we don't own it
};

struct GetShowAttachmentsInlineProxyEvent : 
    public nsImapMiscellaneousSinkProxyEvent
{
    GetShowAttachmentsInlineProxyEvent(
        nsImapMiscellaneousSinkProxy* aProxy, PRBool* aBool);
    virtual ~GetShowAttachmentsInlineProxyEvent();
    NS_IMETHOD HandleEvent();
    PRBool *m_bool;        // pass in handle we don't own it
};

struct HeaderFetchCompletedProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    HeaderFetchCompletedProxyEvent(nsImapMiscellaneousSinkProxy* aProxy);
    virtual ~HeaderFetchCompletedProxyEvent();
    NS_IMETHOD HandleEvent();
};

struct UpdateSecurityStatusProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    UpdateSecurityStatusProxyEvent(nsImapMiscellaneousSinkProxy* aProxy);
    virtual ~UpdateSecurityStatusProxyEvent();
    NS_IMETHOD HandleEvent();
};

struct SetBiffStateAndUpdateProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    SetBiffStateAndUpdateProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                                    nsMsgBiffState biffState);
    virtual ~SetBiffStateAndUpdateProxyEvent();
    NS_IMETHOD HandleEvent();
    nsMsgBiffState m_biffState;
};

struct GetStoredUIDValidityProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    GetStoredUIDValidityProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                                   uid_validity_info* aInfo);
    virtual ~GetStoredUIDValidityProxyEvent();
    NS_IMETHOD HandleEvent();
    uid_validity_info m_uidValidityInfo;
};

struct LiteSelectUIDValidityProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    LiteSelectUIDValidityProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                                    PRUint32 uidValidity);
    virtual ~LiteSelectUIDValidityProxyEvent();
    NS_IMETHOD HandleEvent();
    PRUint32 m_uidValidity;
};


struct ProgressStatusProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    ProgressStatusProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                            PRUint32 statusMsgId, const char *extraInfo);
    virtual ~ProgressStatusProxyEvent();
    NS_IMETHOD HandleEvent();
    PRUint32 m_statusMsgId;
	char	*m_extraInfo;
};

struct PercentProgressProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    PercentProgressProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                              ProgressInfo* aInfo);
    virtual ~PercentProgressProxyEvent();
    NS_IMETHOD HandleEvent();
    ProgressInfo m_progressInfo;
};

struct TunnelOutStreamProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    TunnelOutStreamProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                              msg_line_info* aInfo);
    virtual ~TunnelOutStreamProxyEvent();
    NS_IMETHOD HandleEvent();
    msg_line_info m_msgLineInfo;
};

struct ProcessTunnelProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    ProcessTunnelProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                            TunnelInfo *aInfo);
    virtual ~ProcessTunnelProxyEvent();
    NS_IMETHOD HandleEvent();
    TunnelInfo m_tunnelInfo;
};

struct CopyNextStreamMessageProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    CopyNextStreamMessageProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                                    nsISupports* copyState);
    virtual ~CopyNextStreamMessageProxyEvent();
    NS_IMETHOD HandleEvent();
    nsCOMPtr<nsISupports> m_copyState;
};

struct SetUrlStateProxyEvent : public nsImapMiscellaneousSinkProxyEvent
{
    SetUrlStateProxyEvent(nsImapMiscellaneousSinkProxy* aProxy,
                          nsIMsgMailNewsUrl* aUrl, PRBool isRunning, 
                          nsresult statusCode);
    virtual ~SetUrlStateProxyEvent();
    NS_IMETHOD HandleEvent();
    nsCOMPtr<nsIMsgMailNewsUrl> m_url;
    PRBool m_isRunning;
    nsresult m_status;
};

#endif
