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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsMsgComposeService.h"
#include "nsMsgCompCID.h"
#include "nsISupportsArray.h"
#include "nsIServiceManager.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"
#include "nsXPIDLString.h"
#include "nsIMsgIdentity.h"
#include "nsISmtpUrl.h"
#include "nsIURI.h"
#include "nsMsgI18N.h"
#include "nsIMsgDraft.h"
#include "nsIMsgComposeParams.h"
#include "nsEscape.h"

#ifdef MSGCOMP_TRACE_PERFORMANCE
#include "nsAppDirectoryServiceDefs.h"
#include "nsIFileStreams.h"
#include "nsIPref.h"
#include "nsIMsgHdr.h"
#include "nsIMsgMessageService.h"
#include "nsMsgUtils.h"
#endif

static NS_DEFINE_CID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kMsgComposeCID, NS_MSGCOMPOSE_CID);

#ifdef NS_DEBUG
static PRBool _just_to_be_sure_we_create_only_on_compose_service_ = PR_FALSE;
#endif

#define DEFAULT_CHROME  "chrome://messenger/content/messengercompose/messengercompose.xul"

#ifdef MSGCOMP_TRACE_PERFORMANCE
static PRUint32 GetMessageSizeFromURI(const char * originalMsgURI)
{
  PRUint32 msgSize = 0;

  if (originalMsgURI && *originalMsgURI)
  {
    nsCOMPtr <nsIMsgDBHdr> originalMsgHdr;
    GetMsgDBHdrFromURI(originalMsgURI, getter_AddRefs(originalMsgHdr));
    if (originalMsgHdr)
    originalMsgHdr->GetMessageSize(&msgSize);
  }
  
  return msgSize;
}
#endif

nsMsgComposeService::nsMsgComposeService()
{
#ifdef NS_DEBUG
  NS_ASSERTION(!_just_to_be_sure_we_create_only_on_compose_service_, "You cannot create several message compose service!");
  _just_to_be_sure_we_create_only_on_compose_service_ = PR_TRUE;
#endif

#ifdef MSGCOMP_TRACE_PERFORMANCE
  mTraceInitialized = PR_FALSE;
  mStartTime = PR_IntervalNow();
  mPreviousTime = mStartTime;
#endif
	NS_INIT_REFCNT();
}

/* the following macro actually implement addref, release and query interface for our component. */
NS_IMPL_ISUPPORTS2(nsMsgComposeService, nsIMsgComposeService, nsICmdLineHandler);

nsMsgComposeService::~nsMsgComposeService()
{
#ifdef MSGCOMP_TRACE_PERFORMANCE
  if (mTraceOutputStream)
    mTraceOutputStream->Close();
#endif
}

// Utility function to open a message compose window and pass an nsIMsgComposeParams parameter to it.
static nsresult openWindow( const char *chrome, nsIMsgComposeParams *params )
{
  nsCOMPtr<nsIDOMWindowInternal> hiddenWindow;
  JSContext *jsContext;
  nsresult rv;

  nsCOMPtr<nsIAppShellService> appShell (do_GetService(kAppShellServiceCID));
  if (appShell)
  {
    rv = appShell->GetHiddenWindowAndJSContext(getter_AddRefs(hiddenWindow), &jsContext);
    if (NS_SUCCEEDED(rv))
    {
      // Set up arguments for "window.openDialog"
      void *stackPtr;
      jsval *argv = JS_PushArguments( jsContext,
                                      &stackPtr,
                                      "sss%ip",
                                      (chrome && *chrome) ? chrome : DEFAULT_CHROME,
                                      "_blank",
                                      "chrome,dialog=no,all",
                                      (const nsIID*) (&NS_GET_IID(nsIMsgComposeParams)),
                                      (nsISupports*) params);
      if (argv)
      {
        nsCOMPtr<nsIDOMWindowInternal> newWindow;
        rv = hiddenWindow->OpenDialog(jsContext,
                                      argv,
                                      4,
                                      getter_AddRefs(newWindow));
                                      JS_PopArguments(jsContext, stackPtr);
      }
    }
  }
  return rv;
}

nsresult nsMsgComposeService::OpenComposeWindow(const char *msgComposeWindowURL, const char *originalMsgURI,
	MSG_ComposeType type, MSG_ComposeFormat format, nsIMsgIdentity * identity)
{
	nsresult rv;
	
	/* Actually, the only way to implement forward inline is to simulate a template message. 
	   Maybe one day when we will have more time we can change that
	*/
  if (type == nsIMsgCompType::ForwardInline || type == nsIMsgCompType::Draft || type == nsIMsgCompType::Template)
	{
    nsCOMPtr<nsIMsgDraft> pMsgDraft (do_CreateInstance(NS_MSGDRAFT_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv) && pMsgDraft)
		{
      nsCAutoString uriToOpen(originalMsgURI);
      uriToOpen.Append("?fetchCompleteMessage=true"); 

			switch(type)
			{
				case nsIMsgCompType::ForwardInline:
	    			rv = pMsgDraft->OpenDraftMsg(uriToOpen.get(), nsnull, identity, PR_TRUE);
					break;
				case nsIMsgCompType::Draft:
	    			rv = pMsgDraft->OpenDraftMsg(uriToOpen.get(), nsnull, identity, PR_FALSE);
					break;
				case nsIMsgCompType::Template:
	    			rv = pMsgDraft->OpenEditorTemplate(uriToOpen.get(), nsnull, identity);
					break;
			}
		}
		return rv;
	}

  nsCOMPtr<nsIMsgComposeParams> pMsgComposeParams (do_CreateInstance(NS_MSGCOMPOSEPARAMS_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv) && pMsgComposeParams)
  {
    nsCOMPtr<nsIMsgCompFields> pMsgCompFields (do_CreateInstance(NS_MSGCOMPFIELDS_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv) && pMsgCompFields)
    {
      pMsgComposeParams->SetType(type);
      pMsgComposeParams->SetFormat(format);
      pMsgComposeParams->SetIdentity(identity);
      
  	  if (originalMsgURI && *originalMsgURI)
  		  if (type == nsIMsgCompType::NewsPost) 
        {
          nsCAutoString newsURI(originalMsgURI);
          nsCAutoString group;
          nsCAutoString host;
        
          PRInt32 slashpos = newsURI.FindChar('/');
          if (slashpos > 0 )
          {
            // uri is "host/group"
            newsURI.Left(host, slashpos);
            newsURI.Right(group, newsURI.Length() - slashpos - 1);
          }
          else
            group = originalMsgURI;

          pMsgCompFields->SetNewsgroups(group);
          pMsgCompFields->SetNewshost(host);
  		}
  		else
        pMsgComposeParams->SetOriginalMsgURI(originalMsgURI);
        
      pMsgComposeParams->SetComposeFields(pMsgCompFields);

#ifdef MSGCOMP_TRACE_PERFORMANCE
      char buff[256];
      sprintf(buff, "Start opening the window, message size = %d", GetMessageSizeFromURI(originalMsgURI));
      TimeStamp(buff, PR_TRUE);
#endif
      rv = openWindow(msgComposeWindowURL, pMsgComposeParams);
    }
  }
	return rv;
}

NS_IMETHODIMP nsMsgComposeService::OpenComposeWindowWithURI(const char * aMsgComposeWindowURL, nsIURI * aURI)
{
  nsresult rv = NS_OK;
  if (aURI)
  { 
    nsCOMPtr<nsIMailtoUrl> aMailtoUrl;
    rv = aURI->QueryInterface(NS_GET_IID(nsIMailtoUrl), getter_AddRefs(aMailtoUrl));
    if (NS_SUCCEEDED(rv))
    {
       PRBool aPlainText = PR_FALSE;
       nsXPIDLCString aToPart;
       nsXPIDLCString aCcPart;
       nsXPIDLCString aBccPart;
       nsXPIDLCString aSubjectPart;
       nsXPIDLCString aBodyPart;
       nsXPIDLCString aAttachmentPart;
       nsXPIDLCString aNewsgroup;

       aMailtoUrl->GetMessageContents(getter_Copies(aToPart), getter_Copies(aCcPart), 
                                    getter_Copies(aBccPart), nsnull /* from part */,
                                    nsnull /* follow */, nsnull /* organization */, 
                                    nsnull /* reply to part */, getter_Copies(aSubjectPart),
                                    getter_Copies(aBodyPart), nsnull /* html part */, 
                                    nsnull /* a ref part */, getter_Copies(aAttachmentPart),
                                    nsnull /* priority */, getter_Copies(aNewsgroup), nsnull /* host */,
                                    &aPlainText);

       MSG_ComposeFormat format = nsIMsgCompFormat::Default;
       if (aPlainText)
         format = nsIMsgCompFormat::PlainText;

       //ugghh more conversion work!!!!
       rv = OpenComposeWindowWithValues(aMsgComposeWindowURL,
       									nsIMsgCompType::MailToUrl,
       									format,
       									NS_ConvertASCIItoUCS2(aToPart).GetUnicode(), 
                        NS_ConvertASCIItoUCS2(aCcPart).GetUnicode(),
                        NS_ConvertASCIItoUCS2(aBccPart).GetUnicode(), 
                        aNewsgroup, 
                        NS_ConvertASCIItoUCS2(aSubjectPart).GetUnicode(),
                        NS_ConvertASCIItoUCS2(aBodyPart).GetUnicode(), 
                        aAttachmentPart,
                        nsnull);
    }
  }

  return rv;
}

nsresult nsMsgComposeService::OpenComposeWindowWithValues(const char *msgComposeWindowURL,
														  MSG_ComposeType type,
														  MSG_ComposeFormat format,
														  const PRUnichar *to,
														  const PRUnichar *cc,
														  const PRUnichar *bcc,
														  const char *newsgroups,
														  const PRUnichar *subject,
														  const PRUnichar *body,
														  const char *attachment,
														  nsIMsgIdentity *identity)
{
	nsresult rv;
  nsCOMPtr<nsIMsgCompFields> pCompFields (do_CreateInstance(NS_MSGCOMPFIELDS_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv) && pCompFields)
  {
		if (to)			    {pCompFields->SetTo(to);}
		if (cc)			    {pCompFields->SetCc(cc);}
		if (bcc)		    {pCompFields->SetBcc(bcc);}
		if (newsgroups)	{pCompFields->SetNewsgroups(newsgroups);}
		if (subject)	  {pCompFields->SetSubject(subject);}
		if (attachment)	{pCompFields->SetAttachments(attachment);}
		if (body)		    {pCompFields->SetBody(body);}
	
		rv = OpenComposeWindowWithCompFields(msgComposeWindowURL, type, format, pCompFields, identity);
  }
    
  return rv;
}

nsresult nsMsgComposeService::OpenComposeWindowWithCompFields(const char *msgComposeWindowURL,
														  MSG_ComposeType type,
														  MSG_ComposeFormat format,
														  nsIMsgCompFields *compFields,
														  nsIMsgIdentity *identity)
{
	nsresult rv;

  nsCOMPtr<nsIMsgComposeParams> pMsgComposeParams (do_CreateInstance(NS_MSGCOMPOSEPARAMS_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv) && pMsgComposeParams)
  {
      pMsgComposeParams->SetType(type);
      pMsgComposeParams->SetFormat(format);
      pMsgComposeParams->SetIdentity(identity);
      pMsgComposeParams->SetComposeFields(compFields);    

#ifdef MSGCOMP_TRACE_PERFORMANCE
      TimeStamp("Start opening the window", PR_TRUE);
#endif
      rv = openWindow(msgComposeWindowURL, pMsgComposeParams);
  }

	return rv;
}

nsresult nsMsgComposeService::InitCompose(nsIDOMWindowInternal *aWindow,
                                          nsIMsgComposeParams *params,
                                          nsIMsgCompose **_retval)
{
	nsresult rv;
	nsIMsgCompose * msgCompose = nsnull;
	
	rv = nsComponentManager::CreateInstance(kMsgComposeCID, nsnull,
	                                        NS_GET_IID(nsIMsgCompose),
	                                        (void **) &msgCompose);
	if (NS_SUCCEEDED(rv) && msgCompose)
	{
		msgCompose->Initialize(aWindow, params);
		*_retval = msgCompose;
	}
	
	return rv;
}


nsresult nsMsgComposeService::DisposeCompose(nsIMsgCompose *compose)
{
#ifdef MSGCOMP_TRACE_PERFORMANCE
  if (mTraceOutputStream)
    mTraceOutputStream->Flush();
#endif
	return NS_OK;
}


NS_IMETHODIMP nsMsgComposeService::TimeStamp(const char * label, PRBool resetTime)
{
#ifdef MSGCOMP_TRACE_PERFORMANCE
  nsresult rv;

  if (!mTraceInitialized)
    InitTrace();

  if (!mTraceToConsole && !mTraceToFile)
    return NS_OK;

  char buff[256];
  PRUint32 byteCount;
  PRIntervalTime now;

  if (!label || !*label)
  {
    if (mTraceOutputStream)
      mTraceOutputStream->Flush();
    return NS_OK;
  }

  if (resetTime)
  {      
    if (mTraceToFile)
      rv = mTraceOutputStream->Write("\r\n\r\n--------------------\r\n", 26, &byteCount);

    mStartTime = PR_IntervalNow();
    mPreviousTime = mStartTime;
    now = mStartTime;
  }
  else
    now = PR_IntervalNow();

  PRIntervalTime totalTime = PR_IntervalToMilliseconds(now - mStartTime);
  PRIntervalTime deltaTime = PR_IntervalToMilliseconds(now - mPreviousTime);

  if (mTraceToConsole)
    rv = printf(">>> Time Stamp: [%5d][%5d] - %s\n", totalTime, deltaTime, label);

  if (mTraceToFile)
  {
    sprintf(buff, "[%5d][%5d] - %s\r\n", totalTime, deltaTime, label);
    byteCount = strlen(buff);
    rv = mTraceOutputStream->Write(buff, byteCount, &byteCount);
  }

  mPreviousTime = now;
#endif
  return NS_OK;
}


#ifdef MSGCOMP_TRACE_PERFORMANCE
nsresult nsMsgComposeService::InitTrace()
{
  nsresult rv;

  mTraceToConsole = PR_FALSE;
  mTraceToFile = PR_FALSE;

  nsCOMPtr<nsIPref> prefs (do_GetService(NS_PREF_CONTRACTID));
  if (prefs)
  {
		prefs->GetBoolPref("mail.compose.trace_to_console", &mTraceToConsole);
		prefs->GetBoolPref("mail.compose.trace_to_file", &mTraceToFile);
  }

  if (mTraceToFile)
  {
    nsCOMPtr<nsIFile> localFile;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(localFile));
    rv = localFile->Append("msgcomposetrace.txt");
    rv = localFile->CreateUnique(nsnull, nsIFile::NORMAL_FILE_TYPE, 0644);

    nsCOMPtr<nsIFileChannel> fileChannel = do_CreateInstance(NS_LOCALFILECHANNEL_CONTRACTID, &rv);
    if (fileChannel)
    {
      rv = fileChannel->Init(localFile, -1, 0);
      rv = fileChannel->OpenOutputStream(getter_AddRefs(mTraceOutputStream));
    }
    if (NS_FAILED(rv) || !mTraceOutputStream)
      mTraceToFile = PR_FALSE;
  }

  mTraceInitialized = PR_TRUE;

  return rv;
}
#endif

CMDLINEHANDLER_IMPL(nsMsgComposeService, "-compose", "general.startup.messengercompose", DEFAULT_CHROME,
                    "Start with messenger compose.", NS_MSGCOMPOSESTARTUPHANDLER_CONTRACTID, "Messenger Compose Startup Handler",
                    PR_TRUE, "about:blank", PR_TRUE)

