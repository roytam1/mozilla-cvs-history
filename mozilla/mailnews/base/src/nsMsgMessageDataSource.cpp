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

#include "msgCore.h"    // precompiled header...

#include "nsMsgMessageDataSource.h"
#include "nsMsgRDFUtils.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "rdf.h"
#include "nsEnumeratorUtils.h"
#include "nsIMessage.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIMsgMailSession.h"
#include "nsILocaleFactory.h"
#include "nsLocaleCID.h"
#include "nsDateTimeFormatCID.h"
#include "nsMsgBaseCID.h"


static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kMsgHeaderParserCID,		NS_MSGHEADERPARSER_CID); 
static NS_DEFINE_CID(kMsgMailSessionCID,		NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kLocaleFactoryCID,			NS_LOCALEFACTORY_CID);
static NS_DEFINE_CID(kDateTimeFormatCID,		NS_DATETIMEFORMAT_CID);
// we need this because of an egcs 1.0 (and possibly gcc) compiler bug
// that doesn't allow you to call ::nsISupports::GetIID() inside of a class
// that multiply inherits from nsISupports
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

//There's no GetIID for this.
static NS_DEFINE_IID(kIDateTimeFormatIID, NS_IDATETIMEFORMAT_IID);

nsIRDFResource* nsMsgMessageDataSource::kNC_Subject;
nsIRDFResource* nsMsgMessageDataSource::kNC_Sender;
nsIRDFResource* nsMsgMessageDataSource::kNC_Date;
nsIRDFResource* nsMsgMessageDataSource::kNC_Status;

//commands
nsIRDFResource* nsMsgMessageDataSource::kNC_MarkRead;
nsIRDFResource* nsMsgMessageDataSource::kNC_MarkUnread;
nsIRDFResource* nsMsgMessageDataSource::kNC_ToggleRead;


nsMsgMessageDataSource::nsMsgMessageDataSource():
  mInitialized(PR_FALSE),
  mRDFService(nsnull),
  mHeaderParser(nsnull)
{
  NS_INIT_REFCNT();

  nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
                                             nsIRDFService::GetIID(),
                                             (nsISupports**) &mRDFService); // XXX probably need shutdown listener here

	rv = nsComponentManager::CreateInstance(kMsgHeaderParserCID, 
                                          NULL, 
                                          nsIMsgHeaderParser::GetIID(), 
                                          (void **) &mHeaderParser);

	nsILocaleFactory *localeFactory; 
	rv = nsComponentManager::FindFactory(kLocaleFactoryCID, (nsIFactory**)&localeFactory); 

	if(NS_SUCCEEDED(rv) && localeFactory)
	{
		rv = localeFactory->GetApplicationLocale(getter_AddRefs(mApplicationLocale));
		NS_IF_RELEASE(localeFactory);
	}

	rv = nsComponentManager::CreateInstance(kDateTimeFormatCID, NULL,
		kIDateTimeFormatIID, getter_AddRefs(mDateTimeFormat));				
		
	NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kMsgMailSessionCID, &rv); 
	if(NS_SUCCEEDED(rv))
		mailSession->AddFolderListener(this);
	PR_ASSERT(NS_SUCCEEDED(rv));
}

nsMsgMessageDataSource::~nsMsgMessageDataSource (void)
{
	nsresult rv;
	mRDFService->UnregisterDataSource(this);

	NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kMsgMailSessionCID, &rv); 

	if(NS_SUCCEEDED(rv))
		mailSession->RemoveFolderListener(this);

	nsrefcnt refcnt;

	NS_RELEASE2(kNC_Subject, refcnt);
	NS_RELEASE2(kNC_Sender, refcnt);
	NS_RELEASE2(kNC_Date, refcnt);
	NS_RELEASE2(kNC_Status, refcnt);

	NS_RELEASE2(kNC_MarkRead, refcnt);
	NS_RELEASE2(kNC_MarkUnread, refcnt);
	NS_RELEASE2(kNC_ToggleRead, refcnt);

	nsServiceManager::ReleaseService(kRDFServiceCID, mRDFService); // XXX probably need shutdown listener here
	NS_IF_RELEASE(mHeaderParser);
	mRDFService = nsnull;
}

nsresult nsMsgMessageDataSource::Init()
{
  if (mInitialized)
      return NS_ERROR_ALREADY_INITIALIZED;

  mRDFService->RegisterDataSource(this, PR_FALSE);

  if (! kNC_Subject) {
    
	mRDFService->GetResource(NC_RDF_SUBJECT, &kNC_Subject);
	mRDFService->GetResource(NC_RDF_SENDER, &kNC_Sender);
    mRDFService->GetResource(NC_RDF_DATE, &kNC_Date);
    mRDFService->GetResource(NC_RDF_STATUS, &kNC_Status);

    mRDFService->GetResource(NC_RDF_MARKREAD, &kNC_MarkRead);
    mRDFService->GetResource(NC_RDF_MARKUNREAD, &kNC_MarkUnread);
    mRDFService->GetResource(NC_RDF_TOGGLEREAD, &kNC_ToggleRead);
    
  }
  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMPL_ADDREF(nsMsgMessageDataSource)
NS_IMPL_RELEASE(nsMsgMessageDataSource)

NS_IMETHODIMP
nsMsgMessageDataSource::QueryInterface(REFNSIID iid, void** result)
{
	if (! result)
		return NS_ERROR_NULL_POINTER;

	*result = nsnull;
	if(iid.Equals(nsIFolderListener::GetIID()))
	{
		*result = NS_STATIC_CAST(nsIFolderListener*, this);
		NS_ADDREF(this);
		return NS_OK;
	}
	else
		return nsMsgRDFDataSource::QueryInterface(iid, result);
}

 // nsIRDFDataSource methods
NS_IMETHODIMP nsMsgMessageDataSource::GetURI(char* *uri)
{
  if ((*uri = nsXPIDLCString::Copy("rdf:mailnewsmessages")) == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  else
    return NS_OK;
}

NS_IMETHODIMP nsMsgMessageDataSource::GetSource(nsIRDFResource* property,
                                               nsIRDFNode* target,
                                               PRBool tv,
                                               nsIRDFResource** source /* out */)
{
  PR_ASSERT(0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgMessageDataSource::GetTarget(nsIRDFResource* source,
                                               nsIRDFResource* property,
                                               PRBool tv,
                                               nsIRDFNode** target)
{
	nsresult rv = NS_RDF_NO_VALUE;

	// we only have positive assertions in the mail data source.
	if (! tv)
		return NS_RDF_NO_VALUE;

	nsCOMPtr<nsIMessage> message(do_QueryInterface(source, &rv));
	if (NS_SUCCEEDED(rv)) {
		rv = createMessageNode(message, property,target);
	}
	else
		return NS_RDF_NO_VALUE;
  
  return rv;
}

//sender is the string we need to parse.  senderuserName is the parsed user name we get back.
nsresult nsMsgMessageDataSource::GetSenderName(nsAutoString& sender, nsAutoString *senderUserName)
{
	//XXXOnce we get the csid, use Intl version
	nsresult rv = NS_OK;
	if(mHeaderParser)
	{
		char *name;
		if(NS_SUCCEEDED(rv = mHeaderParser->ExtractHeaderAddressName (nsnull, nsAutoCString(sender), &name)))
		{
			*senderUserName = name;
		}
		if(name)
			PL_strfree(name);
	}
	return rv;
}

NS_IMETHODIMP nsMsgMessageDataSource::GetSources(nsIRDFResource* property,
                                                nsIRDFNode* target,
                                                PRBool tv,
                                                nsISimpleEnumerator** sources)
{
  PR_ASSERT(0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgMessageDataSource::GetTargets(nsIRDFResource* source,
                                                nsIRDFResource* property,    
                                                PRBool tv,
                                                nsISimpleEnumerator** targets)
{
	nsresult rv = NS_RDF_NO_VALUE;

	if(!targets)
		return NS_ERROR_NULL_POINTER;

	*targets = nsnull;
	nsCOMPtr<nsIMessage> message(do_QueryInterface(source, &rv));
	if (NS_SUCCEEDED(rv)) {
		if(peq(kNC_Subject, property) || peq(kNC_Date, property) ||
			peq(kNC_Status, property))
		{
			nsSingletonEnumerator* cursor =
				new nsSingletonEnumerator(source);
			if (cursor == nsnull)
				return NS_ERROR_OUT_OF_MEMORY;
			NS_ADDREF(cursor);
			*targets = cursor;
			rv = NS_OK;
		}
	}
	if(!*targets) {
	  //create empty cursor
	  nsCOMPtr<nsISupportsArray> assertions;
	  rv = NS_NewISupportsArray(getter_AddRefs(assertions));
		if(NS_FAILED(rv))
			return rv;

	  nsArrayEnumerator* cursor = 
		  new nsArrayEnumerator(assertions);
	  if(cursor == nsnull)
		  return NS_ERROR_OUT_OF_MEMORY;
	  NS_ADDREF(cursor);
	  *targets = cursor;
	  rv = NS_OK;
	}
	return rv;
}

NS_IMETHODIMP nsMsgMessageDataSource::Assert(nsIRDFResource* source,
                      nsIRDFResource* property, 
                      nsIRDFNode* target,
                      PRBool tv)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgMessageDataSource::Unassert(nsIRDFResource* source,
                        nsIRDFResource* property,
                        nsIRDFNode* target)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsMsgMessageDataSource::HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion)
{
  *hasAssertion = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP nsMsgMessageDataSource::ArcLabelsIn(nsIRDFNode* node,
                                                 nsISimpleEnumerator** labels)
{
  PR_ASSERT(0);
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgMessageDataSource::ArcLabelsOut(nsIRDFResource* source,
                                                  nsISimpleEnumerator** labels)
{
  nsCOMPtr<nsISupportsArray> arcs;
  nsresult rv = NS_RDF_NO_VALUE;
  

  nsCOMPtr<nsIMessage> message(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv)) {
    fflush(stdout);
    rv = getMessageArcLabelsOut(message, getter_AddRefs(arcs));
  } else {
    // how to return an empty cursor?
    // for now return a 0-length nsISupportsArray
    rv = NS_NewISupportsArray(getter_AddRefs(arcs));
		if(NS_FAILED(rv))
			return rv;
  }

  nsArrayEnumerator* cursor =
    new nsArrayEnumerator(arcs);
  
  if (cursor == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(cursor);
  *labels = cursor;
  
  return NS_OK;
}

nsresult
nsMsgMessageDataSource::getMessageArcLabelsOut(nsIMessage *folder,
                                              nsISupportsArray **arcs)
{
	nsresult rv;
  rv = NS_NewISupportsArray(arcs);
	if(NS_FAILED(rv))
		return rv;

  (*arcs)->AppendElement(kNC_Subject);
  (*arcs)->AppendElement(kNC_Sender);
  (*arcs)->AppendElement(kNC_Date);
	(*arcs)->AppendElement(kNC_Status);
  return NS_OK;
}


NS_IMETHODIMP
nsMsgMessageDataSource::GetAllResources(nsISimpleEnumerator** aCursor)
{
  NS_NOTYETIMPLEMENTED("sorry!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgMessageDataSource::GetAllCommands(nsIRDFResource* source,
                                      nsIEnumerator/*<nsIRDFResource>*/** commands)
{
  nsresult rv;

  nsCOMPtr<nsISupportsArray> cmds;

  nsCOMPtr<nsIMessage> message(do_QueryInterface(source, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_NewISupportsArray(getter_AddRefs(cmds));
    if (NS_FAILED(rv)) return rv;
  }

  if (cmds != nsnull)
    return cmds->Enumerate(commands);
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMsgMessageDataSource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                        nsIRDFResource*   aCommand,
                                        nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                        PRBool* aResult)
{
  nsCOMPtr<nsIMessage> message;
	nsresult rv;

  PRUint32 cnt;
  rv = aSources->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  for (PRUint32 i = 0; i < cnt; i++) {
    nsCOMPtr<nsISupports> source = getter_AddRefs(aSources->ElementAt(i));
		message = do_QueryInterface(source, &rv);
		if (NS_SUCCEEDED(rv)) {

      // we don't care about the arguments -- message commands are always enabled
        *aResult = PR_FALSE;
        return NS_OK;
    }
  }
  *aResult = PR_TRUE;
  return NS_OK; // succeeded for all sources
}

NS_IMETHODIMP
nsMsgMessageDataSource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                 nsIRDFResource*   aCommand,
                                 nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
	nsresult rv = NS_OK;

	// XXX need to handle batching of command applied to all sources

	PRUint32 cnt;
	rv  = aSources->Count(&cnt);
	if(NS_FAILED(rv)) return rv;
	for (PRUint32 i = 0; i < cnt; i++)
	{
		nsISupports* source = aSources->ElementAt(i);
		nsCOMPtr<nsIMessage> message = do_QueryInterface(source, &rv);
		if (NS_SUCCEEDED(rv))
		{
			if(peq(aCommand, kNC_MarkRead))
				rv = DoMarkMessageRead(message, PR_TRUE);
			else if(peq(aCommand, kNC_MarkUnread))
				rv = DoMarkMessageRead(message, PR_FALSE);
		}
	}
  //for the moment return NS_OK, because failure stops entire DoCommand process.
  return NS_OK;
}

NS_IMETHODIMP nsMsgMessageDataSource::OnItemAdded(nsIFolder *parentFolder, nsISupports *item)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgMessageDataSource::OnItemRemoved(nsIFolder *parentFolder, nsISupports *item)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgMessageDataSource::OnItemPropertyChanged(nsISupports *item, const char *property,
														   const char *oldValue, const char *newValue)

{

	return NS_OK;
}

NS_IMETHODIMP nsMsgMessageDataSource::OnItemPropertyFlagChanged(nsISupports *item, const char *property,
									   PRUint32 oldFlag, PRUint32 newFlag)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIRDFResource> resource(do_QueryInterface(item, &rv));

	if(NS_SUCCEEDED(rv))
	{
		if(PL_strcmp("Status", property) == 0)
		{
			nsAutoString oldStatusStr((const char *)"",eOneByte), newStatusStr ((const char *)"", eOneByte);
			rv = createStatusStringFromFlag(oldFlag, oldStatusStr);
			if(NS_FAILED(rv))
				return rv;
			rv = createStatusStringFromFlag(newFlag, newStatusStr);
			if(NS_FAILED(rv))
				return rv;
			rv = NotifyPropertyChanged(resource, kNC_Status, oldStatusStr.GetBuffer(), 
								  newStatusStr.GetBuffer());
		}
	}
	return rv;
}

nsresult
nsMsgMessageDataSource::createMessageNode(nsIMessage *message,
                                         nsIRDFResource *property,
                                         nsIRDFNode **target)
{
	PRBool sort;
    if (peqCollationSort(kNC_Subject, property, &sort))
      return createMessageNameNode(message, sort, target);
    else if (peqCollationSort(kNC_Sender, property, &sort))
      return createMessageSenderNode(message, sort, target);
    else if (peq(kNC_Date, property))
      return createMessageDateNode(message, target);
		else if (peq(kNC_Status, property))
      return createMessageStatusNode(message, target);
    else
      return NS_RDF_NO_VALUE;
}


nsresult
nsMsgMessageDataSource::createMessageNameNode(nsIMessage *message,
                                             PRBool sort,
                                             nsIRDFNode **target)
{
  nsresult rv = NS_OK;
  nsAutoString subject;
  if(sort)
	{
      rv = message->GetSubjectCollationKey(subject);
	}
  else
	{
      rv = message->GetMime2EncodedSubject(subject);
			if(NS_FAILED(rv))
				return rv;
      PRUint32 flags;
      rv = message->GetFlags(&flags);
      if(NS_SUCCEEDED(rv) && (flags & MSG_FLAG_HAS_RE))
			{
					nsAutoString reStr="Re: ";
					reStr +=subject;
					subject = reStr;
			}
	}
	if(NS_SUCCEEDED(rv))
	 rv = createNode(subject, target);
  return rv;
}


nsresult
nsMsgMessageDataSource::createMessageSenderNode(nsIMessage *message,
                                               PRBool sort,
                                               nsIRDFNode **target)
{
  nsresult rv = NS_OK;
  nsAutoString sender, senderUserName;
  if(sort)
	{
      rv = message->GetAuthorCollationKey(sender);
			if(NS_SUCCEEDED(rv))
	      rv = createNode(sender, target);
	}
  else
	{
      rv = message->GetMime2EncodedAuthor(sender);
      if(NS_SUCCEEDED(rv))
				 rv = GetSenderName(sender, &senderUserName);
			if(NS_SUCCEEDED(rv))
	       rv = createNode(senderUserName, target);
	}
  return rv;
}

nsresult
nsMsgMessageDataSource::createMessageDateNode(nsIMessage *message,
                                             nsIRDFNode **target)
{
  nsAutoString date;
  nsresult rv = message->GetProperty("date", date);
	if(NS_FAILED(rv))
		return rv;
  PRInt32 error;
  time_t aTime = date.ToInteger(&error, 16);
  struct tm* tmTime = localtime(&aTime);
  nsString dateString;
  if(mDateTimeFormat)
	  rv = mDateTimeFormat->FormatTMTime(mApplicationLocale, kDateFormatShort, kTimeFormatNoSeconds, 
		                tmTime, dateString); 
  //Ensure that we always have some string for the date.
  if(!mDateTimeFormat || NS_FAILED(rv))
  {
	  dateString ="";
	  rv = NS_OK;
  }
  if(NS_SUCCEEDED(rv))
	  rv = createNode(dateString, target);
  return rv;
}

nsresult
nsMsgMessageDataSource::createMessageStatusNode(nsIMessage *message,
                                               nsIRDFNode **target)
{
	nsresult rv;
	PRUint32 flags;
	nsAutoString statusStr;
	rv = message->GetFlags(&flags);
	if(NS_FAILED(rv))
		return rv;
	rv = createStatusStringFromFlag(flags, statusStr);
	if(NS_FAILED(rv))
		return rv;
	rv = createNode(statusStr, target);
	return rv;
}

nsresult 
nsMsgMessageDataSource::createStatusStringFromFlag(PRUint32 flags, nsAutoString &statusStr)
{
	nsresult rv = NS_OK;
	statusStr = "";
	if(flags & MSG_FLAG_REPLIED)
		statusStr = "replied";
	else if(flags & MSG_FLAG_FORWARDED)
		statusStr = "forwarded";
	else if(flags & MSG_FLAG_NEW)
		statusStr = "new";
	else if(flags & MSG_FLAG_READ)
		statusStr = "read";
	return rv;
}

nsresult
nsMsgMessageDataSource::DoMarkMessageRead(nsIMessage *message, PRBool markRead)
{
	nsresult rv;
	rv = message->MarkRead(markRead);
	return rv;
}

nsresult nsMsgMessageDataSource::NotifyPropertyChanged(nsIRDFResource *resource,
													  nsIRDFResource *propertyResource,
													  const char *oldValue, const char *newValue)
{
	nsCOMPtr<nsIRDFNode> oldValueNode, newValueNode;
	nsString oldValueStr = oldValue;
	nsString newValueStr = newValue;
	createNode(oldValueStr,getter_AddRefs(oldValueNode));
	createNode(newValueStr, getter_AddRefs(newValueNode));
	NotifyObservers(resource, propertyResource, oldValueNode, PR_FALSE);
	NotifyObservers(resource, propertyResource, newValueNode, PR_TRUE);
	return NS_OK;
}


