#include "msgCore.h"
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"
#include "nsMsgCompCID.h"

#include <stdio.h>
#ifdef XP_PC
#include <windows.h>
#endif

#include "nsIComponentManager.h" 
#include "nsMsgCompCID.h"
#include "nsIMsgCompose.h"
#include "nsIMsgCompFields.h"
#include "nsIMsgSend.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nscore.h"
#include "nsIMsgMailSession.h"
#include "nsINetSupportDialogService.h"
#include "nsIAppShellService.h"
#include "nsAppShellCIDs.h"


#include "nsINetService.h"
#include "nsIComponentManager.h"
#include "nsString.h"

#include "nsISmtpService.h"
#include "nsISmtpUrl.h"
#include "nsIUrlListener.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsIFileLocator.h"

#include "MsgCompGlue.h"
#include "nsCRT.h"
#include "prmem.h"

#include "nsIMimeURLUtils.h"

#ifdef XP_PC
#define NETLIB_DLL "netlib.dll"
#define XPCOM_DLL  "xpcom32.dll"
#define PREF_DLL   "xppref32.dll"
#define APPSHELL_DLL "nsappshell.dll"
#define MIME_DLL "mime.dll"
#else
#ifdef XP_MAC
#include "nsMacRepository.h"
#else
#define NETLIB_DLL "libnetlib"MOZ_DLL_SUFFIX
#define XPCOM_DLL  "libxpcom"MOZ_DLL_SUFFIX
#define PREF_DLL   "libpref"MOZ_DLL_SUFFIX
#define APPCORES_DLL  "libappcores"MOZ_DLL_SUFFIX
#define APPSHELL_DLL "libnsappshell"MOZ_DLL_SUFFIX
#define MIME_DLL "libmime"MOZ_DLL_SUFFIX
#endif
#endif


/////////////////////////////////////////////////////////////////////////////////
// Define keys for all of the interfaces we are going to require for this test
/////////////////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kNetServiceCID, NS_NETSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kSmtpServiceCID, NS_SMTPSERVICE_CID);
static NS_DEFINE_CID(kFileLocatorCID, NS_FILELOCATOR_CID);
static NS_DEFINE_CID(kEventQueueCID, NS_EVENTQUEUE_CID);
static NS_DEFINE_IID(kIMsgComposeIID, NS_IMSGCOMPOSE_IID); 
static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kMsgComposeCID, NS_MSGCOMPOSE_CID); 
static NS_DEFINE_IID(kIMsgCompFieldsIID, NS_IMSGCOMPFIELDS_IID); 
static NS_DEFINE_CID(kMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID); 
static NS_DEFINE_IID(kIMsgSendIID, NS_IMSGSEND_IID); 
static NS_DEFINE_CID(kMsgSendCID, NS_MSGSEND_CID); 
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kMimeURLUtilsCID, NS_IMIME_URLUTILS_CID);
static NS_DEFINE_CID(kNetSupportDialogCID, NS_NETSUPPORTDIALOG_CID);
static NS_DEFINE_CID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_IID(kIAppShellServiceIID,       NS_IAPPSHELL_SERVICE_IID);

nsresult OnIdentityCheck()
{
	nsresult result = NS_OK;
	NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &result); 
	if (NS_SUCCEEDED(result) && mailSession)
	{
		// mscott: we really don't check an identity, we check
		// for an outgoing 
		nsIMsgIncomingServer * incomingServer = nsnull;
		result = mailSession->GetCurrentServer(&incomingServer);
		if (NS_SUCCEEDED(result) && incomingServer)
		{
			char * value = nsnull;
			incomingServer->GetPrettyName(&value);
			printf("Server pretty name: %s\n", value ? value : "");
			incomingServer->GetUsername(&value);
			printf("User Name: %s\n", value ? value : "");
			incomingServer->GetHostName(&value);
			printf("Pop Server: %s\n", value ? value : "");
			incomingServer->GetPassword(&value);
			printf("Pop Password: %s\n", value ? value : "");

			NS_RELEASE(incomingServer);
		}
		else
			printf("Unable to retrieve the outgoing server interface....\n");
	}
	else
		printf("Unable to retrieve the mail session service....\n");

	return result;
}

//nsMsgAttachmentData *
nsMsgAttachedFile *
GetAttachments(void)
{  
  int attachCount = 3;
  nsMsgAttachedFile *attachments = (nsMsgAttachedFile *) PR_Malloc(sizeof(nsMsgAttachedFile) * attachCount);

  if (!attachments)
    return NULL;
  
  nsCRT::memset(attachments, 0, sizeof(MSG_AttachedFile) * attachCount);
  attachments[0].orig_url = PL_strdup("file://C:/boxster.jpg");
  attachments[0].file_name = PL_strdup("C:\\boxster.jpg");
  attachments[0].type = PL_strdup("image/jpeg");
  attachments[0].encoding = PL_strdup(ENCODING_BINARY);
  attachments[0].description = PL_strdup("Boxster Image");

  attachments[1].orig_url = PL_strdup("file://C:/boxster.jpg");
  attachments[1].file_name = PL_strdup("C:\\boxster.jpg");
  attachments[1].type = PL_strdup("image/jpeg");
  attachments[1].encoding = PL_strdup(ENCODING_BINARY);
  attachments[1].description = PL_strdup("Boxster Image");
  return attachments;
}

char *email = {"\
<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">\n\
<html>\n\
<body text=\"#000000\" bgcolor=\"#FFFFFF\" link=\"#FF0000\" vlink=\"#800080\" alink=\"#0000FF\">\n\
<b><font face=\"Arial,Helvetica\"><font color=\"#FF0000\">Here is some HTML\n\
in RED!</font></font></b>\n\
<br><b><font face=\"Arial,Helvetica\"><font color=\"#FF0000\">Now a picture:</font></font></b>\n\
<br><img SRC=\"file://C:/test.jpg\" height=8 width=10>\n\
<br>All done!\n\
<br>&nbsp;\n\
</body>\n\
</html>"};

nsresult
CallMe(nsresult aExitCode, void *tagData, nsFileSpec *fs)
{
  char *buf = (char *)tagData;
  
  printf("Called ME!\n");
  printf("Exit code = %d\n", aExitCode);
  printf("What were the magic words => [%s]\n", buf);
  PR_FREEIF(buf);

  if (fs)
  {
    printf("Delivery NOT Requested: Just created an RFC822 file. URL=[%s]\n", fs->GetCString());
    delete fs;
  }

  return NS_OK;
}

/* 
 * This is a test stub for mail composition. This will be enhanced as the
 * development continues for message send functions. 
 */
int main(int argc, char *argv[]) 
{ 
  nsIMsgCompose *pMsgCompose; 
  nsIMsgCompFields *pMsgCompFields;
  nsIMsgSend *pMsgSend;
  nsresult rv = NS_OK;
  nsIAppShellService* appShell = nsnull;

  nsComponentManager::RegisterComponent(kNetServiceCID, NULL, NULL, NETLIB_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kEventQueueServiceCID, NULL, NULL, XPCOM_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kEventQueueCID, NULL, NULL, XPCOM_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kPrefCID, nsnull, nsnull, PREF_DLL, PR_TRUE, PR_TRUE);
	nsComponentManager::RegisterComponent(kFileLocatorCID,  NULL, NULL, APPSHELL_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kMimeURLUtilsCID,  NULL, NULL, MIME_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kNetSupportDialogCID,  NULL, NULL, APPSHELL_DLL, PR_FALSE, PR_FALSE);
	nsComponentManager::RegisterComponent(kAppShellServiceCID,  NULL, NULL, APPSHELL_DLL, PR_FALSE, PR_FALSE);

  /*
   * Create the Application Shell instance...
   */
  nsCOMPtr<nsIWebShellWindow> newWindow;
  rv = nsServiceManager::GetService(kAppShellServiceCID,
                                    kIAppShellServiceIID,
                                   (nsISupports**)&appShell);
	if (NS_SUCCEEDED(rv)) 
    appShell->CreateTopLevelWindow(nsnull, nsnull, PR_TRUE, *getter_AddRefs(newWindow),
                nsnull, nsnull, 200, 200);

  // Create the Event Queue for this thread...
	NS_WITH_SERVICE(nsIEventQueueService, pEventQService, kEventQueueServiceCID, &rv); 

	if (NS_FAILED(rv)) 
  {
    printf("Failed to get event queue\n");
    return rv;
  }

  rv = pEventQService->CreateThreadEventQueue();
	if (NS_FAILED(rv)) 
  {
    printf("Failed to create event queue\n");
    return rv;
  }



	// make sure prefs get initialized and loaded..
	// mscott - this is just a bad bad bad hack right now until prefs
	// has the ability to take nsnull as a parameter. Once that happens,
	// prefs will do the work of figuring out which prefs file to load...
	NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv); 
    if (NS_FAILED(rv) || (prefs == nsnull)) {
        exit(rv);
    }

  NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
  if (NS_FAILED(rv)) 
  {
    printf("Failure on Mail Session Init!\n");
    return rv;
  }  

  OnIdentityCheck();

  rv = nsComponentManager::CreateInstance(kMsgCompFieldsCID, NULL, 
                                           nsIMsgCompFields::GetIID(), (void **) &pMsgCompFields);   
  if (rv == NS_OK && pMsgCompFields) { 
    printf("We succesfully obtained a nsIMsgCompFields interface....\n");
    printf("Releasing the interface now...\n");
    pMsgCompFields->Release(); 
  } 
  
  rv = nsComponentManager::CreateInstance(kMsgComposeCID, NULL, 
                                           nsIMsgCompose::GetIID(), (void **) &pMsgCompose);
  if (rv == NS_OK && pMsgCompose)
  {
    printf("We succesfully obtained a nsIMsgCompose interface....\n");
    printf("Releasing the interface now...\n");
    pMsgCompose->Release(); 
  } 

  rv = nsComponentManager::CreateInstance(kMsgSendCID, NULL, kIMsgSendIID, (void **) &pMsgSend); 
  if (rv == NS_OK && pMsgSend) 
  { 
    printf("We succesfully obtained a nsIMsgSend interface....\n");    
    rv = nsComponentManager::CreateInstance(kMsgCompFieldsCID, NULL, kIMsgCompFieldsIID, 
                                             (void **) &pMsgCompFields); 
    if (rv == NS_OK && pMsgCompFields)
    { 
      pMsgCompFields->SetFrom(", rhp@netscape.com, ", NULL);
      pMsgCompFields->SetTo("rhp@netscape.com", NULL);
      pMsgCompFields->SetSubject("[spam] test", NULL);
      //pMsgCompFields->SetBody("Sample message sent with Mozilla\n\nPlease do not reply, thanks\n\nRich Pizzarro\n", NULL);
      pMsgCompFields->SetBody(email, NULL);
      // pMsgCompFields->SetAttachments("c:\\boxster.jpg", NULL);

      PRInt32 nBodyLength;
      char    *pBody;

	    pMsgCompFields->GetBody(&pBody);
	    if (pBody)
		    nBodyLength = PL_strlen(pBody);
	    else
		    nBodyLength = 0;

      nsMsgAttachedFile *ptr = NULL;
      //nsMsgAttachedFile *ptr = GetAttachments();

      char *tagBuf = (char *)PR_Malloc(256);
      if (tagBuf)
        PL_strcpy(tagBuf, "Do that voodo, that you do, soooo weeeelllll!");
      
      pMsgSend->CreateAndSendMessage(pMsgCompFields, 
						    PR_FALSE,         // PRBool                            digest_p,
                PR_FALSE,         // PRBool                            dont_deliver_p,
						    nsMsgDeliverNow,   // nsMsgDeliverMode                  mode,
						    TEXT_HTML, //TEXT_PLAIN,       // const char                        *attachment1_type,
						    pBody,            // const char                        *attachment1_body,
						    nBodyLength,      // PRUint32                          attachment1_body_length,
						    NULL,             // const struct nsMsgAttachmentData   *attachments,
						    ptr,              // const struct nsMsgAttachedFile     *preloaded_attachments,
						    NULL,             // nsMsgSendPart                     *relatedPart,
						    CallMe,
						    tagBuf);

      PR_FREEIF(ptr);
    }    
  }

#ifdef XP_PC
  printf("Sitting in an event processing loop ...Hit Cntl-C to exit...");
  while (1)
  {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
#endif

  printf("Releasing the interface now...\n");
  pMsgSend->Release(); 
  pMsgCompFields->Release(); 


  return 0; 
}

