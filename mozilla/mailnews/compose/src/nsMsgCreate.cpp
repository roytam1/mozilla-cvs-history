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
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsIEventQueueService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIMimeStreamConverter.h"
#include "nsFileStream.h"
#include "nsFileSpec.h"
#include "nsMimeTypes.h"
#include "nsIPref.h"
#include "nsICharsetConverterManager.h"
#include "prprf.h"
#include "nsMsgCreate.h" 
#include "nsMsgCompUtils.h"
#include "nsIMsgMessageService.h"
#include "nsMsgUtils.h"
#include "nsMsgDeliveryListener.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgIdentity.h"
#include "nsIEnumerator.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCopy.h"
#include "nsNetUtil.h"
#include "nsMsgMimeCID.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsMsgBaseCID.h"


// CID's needed
static NS_DEFINE_CID(kPrefCID,            NS_PREF_CID);

//
// Implementation...
//
nsMsgDraft::nsMsgDraft()
{
	NS_INIT_REFCNT();

  mURI = nsnull;
  mMessageService = nsnull;
  mOutType = nsMimeOutput::nsMimeMessageDraftOrTemplate;
  mAddInlineHeaders = PR_FALSE;
}

nsMsgDraft::~nsMsgDraft()
{
  if (mMessageService)
  {
    ReleaseMessageServiceFromURI(mURI, mMessageService);
    mMessageService = nsnull;
  }

  PR_FREEIF(mURI);
}

/* the following macro actually implement addref, release and query interface for our component. */
NS_IMPL_ISUPPORTS1(nsMsgDraft, nsIMsgDraft)

// stream converter
static NS_DEFINE_CID(kStreamConverterCID,    NS_MAILNEWS_MIME_STREAM_CONVERTER_CID);

nsresult    
nsMsgDraft::ProcessDraftOrTemplateOperation(const char *msgURI, nsMimeOutputType aOutType, 
                                            nsIMsgIdentity * identity, nsIMsgDBHdr **aMsgToReplace)
{
  nsresult  rv;

  mOutType = aOutType;

printf("___TRACE___BUG_79775___CP#009___\n");
  if (!msgURI)
    return NS_ERROR_INVALID_ARG;

  mURI = nsCRT::strdup(msgURI);

printf("___TRACE___BUG_79775___CP#010___\n");
  if (!mURI)
    return NS_ERROR_OUT_OF_MEMORY;

printf("___TRACE___BUG_79775___CP#011___\n");
  rv = GetMessageServiceFromURI(mURI, &mMessageService);
  if (NS_FAILED(rv) && !mMessageService)
  {
    return rv;
  }

  NS_ADDREF(this);

printf("___TRACE___BUG_79775___CP#012___\n");
  // Now, we can create a mime parser (nsIStreamConverter)!
  nsCOMPtr<nsIStreamConverter> mimeParser;
  rv = nsComponentManager::CreateInstance(kStreamConverterCID, 
                                          NULL, NS_GET_IID(nsIStreamConverter), 
                                          (void **) getter_AddRefs(mimeParser)); 
  if (NS_FAILED(rv) || !mimeParser)
  {
printf("___TRACE___BUG_79775___CP#013___\n");
    Release();
    ReleaseMessageServiceFromURI(mURI, mMessageService);
    mMessageService = nsnull;
#ifdef NS_DEBUG
    printf("Failed to create MIME stream converter...\n");
#endif
    return rv;
  }
  
printf("___TRACE___BUG_79775___CP#014___\n");
  // Set us as the output stream for HTML data from libmime...
  nsCOMPtr<nsIMimeStreamConverter> mimeConverter = do_QueryInterface(mimeParser);
  if (mimeConverter)
  {
printf("___TRACE___BUG_79775___CP#015___\n");
	  mimeConverter->SetMimeOutputType(mOutType);  // Set the type of output for libmime
      mimeConverter->SetForwardInline(mAddInlineHeaders);
      mimeConverter->SetIdentity(identity);
  }

  nsCOMPtr<nsIStreamListener> convertedListener = do_QueryInterface(mimeParser);
  if (!convertedListener)
  {
printf("___TRACE___BUG_79775___CP#016___\n");
    Release();
    ReleaseMessageServiceFromURI(mURI, mMessageService);
    mMessageService = nsnull;
#ifdef NS_DEBUG
    printf("Unable to get the nsIStreamListener interface from libmime\n");
#endif
    return NS_ERROR_UNEXPECTED;
  }  

  nsCOMPtr<nsIURI> aURL;
  rv = CreateStartupUrl(mURI, getter_AddRefs(aURL));

printf("___TRACE___BUG_79775___CP#017___\n");
  // HACK: if we are forwarding a message and that message used a charset over ride
  // (as speciifed in the top most window (assuming the reply originated from that window)
  // then use that over ride charset instead of the charset specified in the message
	nsCOMPtr <nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID);          
  nsXPIDLString mailCharset;
  if (mailSession)
  {
    nsCOMPtr<nsIMsgWindow>    msgWindow;
    mailSession->GetTopmostMsgWindow(getter_AddRefs(msgWindow));
    if (msgWindow)
    {
      
      msgWindow->GetMailCharacterSet(getter_Copies(mailCharset));
      if (mailCharset)
      {
        nsCOMPtr<nsIMsgI18NUrl> i18nUrl(do_QueryInterface(aURL));
        if (i18nUrl)
          i18nUrl->SetCharsetOverRide(mailCharset);
      }
    }
  }

  nsCOMPtr<nsIChannel> dummyChannel;
  rv = NS_NewInputStreamChannel(getter_AddRefs(dummyChannel), aURL, nsnull, nsnull, -1);
  if (NS_FAILED(mimeParser->AsyncConvertData(nsnull, nsnull, nsnull, dummyChannel)))
  {
printf("___TRACE___BUG_79775___CP#018___\n");
    Release();
    ReleaseMessageServiceFromURI(mURI, mMessageService);
    mMessageService = nsnull;
#ifdef NS_DEBUG
    printf("Unable to set the output stream for the mime parser...\ncould be failure to create internal libmime data\n");
#endif

    return NS_ERROR_UNEXPECTED;
  }

  // Make sure we return this if requested!
  if (aMsgToReplace)
     GetMsgDBHdrFromURI(msgURI, aMsgToReplace);

  // Now, just plug the two together and get the hell out of the way!
  rv = mMessageService->DisplayMessage(mURI, convertedListener, nsnull, nsnull, mailCharset, nsnull);

  ReleaseMessageServiceFromURI(mURI, mMessageService);
  mMessageService = nsnull;
  Release();

printf("___TRACE___BUG_79775___CP#019___\n");
	if (NS_FAILED(rv))
    return rv;    
  else
    return NS_OK;
}

nsresult
nsMsgDraft::OpenDraftMsg(const char *msgURI, nsIMsgDBHdr **aMsgToReplace,
                         nsIMsgIdentity * identity, PRBool addInlineHeaders)
{
printf("___TRACE___BUG_79775___CP#007___\n");
  // We should really never get here, but if we do, just return 
  // with an error
  if (!msgURI)
    return NS_ERROR_FAILURE;
  
  mAddInlineHeaders = addInlineHeaders;
printf("___TRACE___BUG_79775___CP#008___\n");
  return ProcessDraftOrTemplateOperation(msgURI, nsMimeOutput::nsMimeMessageDraftOrTemplate, 
                                         identity, aMsgToReplace);
}

nsresult
nsMsgDraft::OpenEditorTemplate(const char *msgURI, nsIMsgDBHdr **aMsgToReplace,
							   nsIMsgIdentity * identity)
{
  return ProcessDraftOrTemplateOperation(msgURI, nsMimeOutput::nsMimeMessageEditorTemplate, 
                                         identity, aMsgToReplace);
}

