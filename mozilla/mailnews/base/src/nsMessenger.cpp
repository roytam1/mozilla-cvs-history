/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "prsystem.h"

#include "nsMessenger.h"

/* rhp - for access to webshell */
#include "nsIDOMWindow.h"
#include "nsIBrowserWindow.h"
#include "nsIWebShellWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsAppShellCIDs.h"
#include "nsIAppShellService.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDocumentViewer.h"
#include "nsIDOMXULDocument.h"

#include "nsIMsgMailSession.h"
#include "nsIMsgIncomingServer.h"
#include "nsIPop3IncomingServer.h"
#include "nsINoIncomingServer.h"
#include "nsIMsgMessageService.h"
#include "nsIFileSpecWithUI.h"
#include "nsFileStream.h"

#include "nsIMessage.h"
#include "nsIMsgFolder.h"
#include "nsIPop3Service.h"

#include "nsIDOMXULElement.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIAppShell.h"
#include "nsIAppShellService.h"
#include "nsIIOService.h"
#include "nsAppShellCIDs.h"
#include "nsMsgRDFUtils.h"
#include "nsMsgFolderFlags.h"

#include "nsICopyMsgStreamListener.h"
#include "nsICopyMessageListener.h"

#include "nsMsgUtils.h"
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"

#include "nsIComponentManager.h"
#include "nsTransactionManagerCID.h"

#include "nsIMsgSendLater.h" 
#include "nsMsgCompCID.h"
#include "nsIMsgSendLaterListener.h"
#include "nsIMsgDraft.h"

#include "nsIMsgCopyService.h"
#include "nsIMsgCopyServiceListener.h"

#include "nsMsgStatusFeedback.h"

#include "nsIContentViewer.h" 
#include "nsIPref.h"
#include "nsLayoutCID.h"
#include "nsIPresContext.h"
#include "nsIStringStream.h"
#include "nsEscape.h"
#include "nsIStreamListener.h"
#include "nsIStreamConverterService.h"
#include "nsIMsgCompose.h"
#include "nsIMsgCompFields.h"
#include "nsIMsgComposeService.h"
#include "nsMsgCompFieldsFact.h"
#include "nsMsgI18N.h"

static NS_DEFINE_CID(kIStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kCMsgMailSessionCID, NS_MSGMAILSESSION_CID); 
static NS_DEFINE_CID(kCPop3ServiceCID, NS_POP3SERVICE_CID);
static NS_DEFINE_CID(kRDFServiceCID,	NS_RDFSERVICE_CID);
static NS_DEFINE_IID(kIDocumentViewerIID,     NS_IDOCUMENT_VIEWER_IID); 
static NS_DEFINE_IID(kAppShellServiceCID,        NS_APPSHELL_SERVICE_CID);
static NS_DEFINE_CID(kTransactionManagerCID, NS_TRANSACTIONMANAGER_CID);
static NS_DEFINE_CID(kComponentManagerCID,  NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kMsgSendLaterCID, NS_MSGSENDLATER_CID); 
static NS_DEFINE_CID(kCopyMessageStreamListenerCID, NS_COPYMESSAGESTREAMLISTENER_CID); 
static NS_DEFINE_CID(kMsgDraftCID, NS_MSGDRAFT_CID);
static NS_DEFINE_CID(kPrintPreviewContextCID, NS_PRINT_PREVIEW_CONTEXT_CID);
static NS_DEFINE_IID(kIPresContextIID, NS_IPRESCONTEXT_IID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kMsgCopyServiceCID,		NS_MSGCOPYSERVICE_CID);
static NS_DEFINE_CID(kMsgComposeServiceCID, NS_MSGCOMPOSESERVICE_CID);
static NS_DEFINE_CID(kMsgComposeCID, NS_MSGCOMPOSE_CID);
static NS_DEFINE_CID(kMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID);

#if defined(DEBUG_seth_) || defined(DEBUG_sspitzer_) || defined(DEBUG_jefft)
#define DEBUG_MESSENGER
#endif

#define FOUR_K 4096

// ***************************************************
// jefft - this is a rather obscured class serves for Save Message As File,
// Save Message As Template, and Save Attachment to a file
// 
class nsSaveAsListener : public nsIUrlListener,
                         public nsIMsgCopyServiceListener,
                         public nsIStreamListener
{
public:
    nsSaveAsListener(nsIFileSpec* fileSpec);
    virtual ~nsSaveAsListener();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIURLLISTENER
    NS_DECL_NSIMSGCOPYSERVICELISTENER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMOBSERVER

    nsCOMPtr<nsIFileSpec> m_fileSpec;
    nsCOMPtr<nsIOutputStream> m_outputStream;
    char *m_dataBuffer;
    nsCOMPtr<nsIChannel> m_channel;
};



static nsresult ConvertDOMListToResourceArray(nsIDOMNodeList *nodeList, nsISupportsArray **resourceArray)
{
	nsresult rv = NS_OK;
	PRUint32 listLength;
	nsIDOMNode *node;
	nsIDOMXULElement *xulElement;
	nsIRDFResource *resource;

	if(!resourceArray)
		return NS_ERROR_NULL_POINTER;

	if(NS_FAILED(rv = nodeList->GetLength(&listLength)))
		return rv;

	if(NS_FAILED(NS_NewISupportsArray(resourceArray)))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	for(PRUint32 i = 0; i < listLength; i++)
	{
		if(NS_FAILED(nodeList->Item(i, &node)))
			return rv;

		if(NS_SUCCEEDED(rv = node->QueryInterface(nsCOMTypeInfo<nsIDOMXULElement>::GetIID(), (void**)&xulElement)))
		{
			if(NS_SUCCEEDED(rv = xulElement->GetResource(&resource)) && resource)
			{
				(*resourceArray)->AppendElement(resource);
				NS_RELEASE(resource);
			}
			NS_RELEASE(xulElement);
		}
		NS_RELEASE(node);
		
	}

	return rv;
}


//
// nsMessenger
//
nsMessenger::nsMessenger() : m_folderPath("")
{
	NS_INIT_REFCNT();
	mScriptObject = nsnull;
	mWebShell = nsnull; 
	mWindow = nsnull;

	InitializeFolderRoot();
}

nsMessenger::~nsMessenger()
{
    NS_IF_RELEASE(mWindow);
    NS_IF_RELEASE(mWebShell);
}

//
// nsISupports
//
NS_IMPL_ISUPPORTS(nsMessenger, nsCOMTypeInfo<nsIMessenger>::GetIID())

//
// nsIMsgAppCore
//
NS_IMETHODIMP    
nsMessenger::Open3PaneWindow()
{
	const char *  urlstr=nsnull;
	nsresult rv = NS_OK;
	
	nsCOMPtr<nsIWebShellWindow> newWindow;

	urlstr = "resource:/res/samples/messenger.html";
	NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  
	nsCOMPtr<nsIURI> url;
	NS_WITH_SERVICE(nsIIOService, pNetService, kIOServiceCID, &rv);

	if (NS_SUCCEEDED(rv) && pNetService) 
		rv = pNetService->NewURI(urlstr, nsnull, getter_AddRefs(url));


	if (NS_SUCCEEDED(rv))
		rv = appShell->CreateTopLevelWindow(nsnull,      // parent
                                   url,
                                   PR_TRUE,
                                   PR_TRUE,
                                   NS_CHROME_ALL_CHROME,
                                   nsnull,      // callbacks
                                   NS_SIZETOCONTENT,           // width
                                   NS_SIZETOCONTENT,           // height
                                   getter_AddRefs(newWindow)); // result widget
	return rv;
}

nsresult
nsMessenger::GetNewMessages(nsIRDFCompositeDataSource *db, nsIDOMXULElement *folderElement)
{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> folderResource;
	nsCOMPtr<nsISupportsArray> folderArray;

	if(!folderElement || !db)
		return NS_ERROR_NULL_POINTER;

	rv = folderElement->GetResource(getter_AddRefs(folderResource));
	if(NS_FAILED(rv))
		return rv;

	if(NS_FAILED(NS_NewISupportsArray(getter_AddRefs(folderArray))))
		return NS_ERROR_OUT_OF_MEMORY;

	folderArray->AppendElement(folderResource);

	DoCommand(db, NC_RDF_GETNEWMESSAGES, folderArray, nsnull);

	return rv;
}


NS_IMETHODIMP    
nsMessenger::SetWindow(nsIDOMWindow *aWin, nsIMsgStatusFeedback *aStatusFeedback)
{
	if(!aWin)
		return NS_ERROR_NULL_POINTER;

  nsAutoString  webShellName("messagepane");
  NS_IF_RELEASE(mWindow);
  mWindow = aWin;
  NS_ADDREF(aWin);

#ifdef DEBUG
  /* rhp - Needed to access the webshell to drive message display */
  printf("nsMessenger::SetWindow(): Getting the webShell of interest...\n");
#endif

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  if (!globalObj) 
  {
    return NS_ERROR_FAILURE;
  }

  nsIWebShell *webShell = nsnull;
  nsIWebShell *rootWebShell = nsnull;

  globalObj->GetWebShell(&webShell);
  if (nsnull == webShell) 
  {
    return NS_ERROR_FAILURE;
  }

  webShell->GetRootWebShell(rootWebShell);
  if (nsnull != rootWebShell) 
  {
    nsresult rv = rootWebShell->FindChildWithName(webShellName.GetUnicode(), mWebShell);
#ifdef NS_DEBUG
    if (NS_SUCCEEDED(rv) && nsnull != mWebShell)
        printf("nsMessenger::SetWindow(): Got the webShell %s.\n", (const char *) nsAutoCString(webShellName));
    else
        printf("nsMessenger::SetWindow(): Failed to find webshell %s.\n", (const char *) nsAutoCString(webShellName));
#endif
	if (mWebShell)
	{
		if (aStatusFeedback)
		{
			m_docLoaderObserver = do_QueryInterface(aStatusFeedback);
			aStatusFeedback->SetWebShell(mWebShell, mWindow);
			mWebShell->SetDocLoaderObserver(m_docLoaderObserver);
            NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
            if(NS_SUCCEEDED(rv))
	            mailSession->SetTemporaryMsgStatusFeedback(aStatusFeedback);
		}
	}
    NS_RELEASE(rootWebShell);
  }

  NS_RELEASE(webShell);

  // libmime always converts to UTF-8 (both HTML and XML)
  if (nsnull != mWebShell) 
  {
	  nsAutoString aForceCharacterSet("UTF-8");
	  mWebShell->SetForceCharacterSet(aForceCharacterSet.GetUnicode());
  }

  return NS_OK;
}


// this should really go through all the pop servers and initialize all
// folder roots
void nsMessenger::InitializeFolderRoot()
{
    nsresult rv;
    
	// get the current identity from the mail session....
    NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
    if (NS_FAILED(rv)) return;

    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = mailSession->GetCurrentServer(getter_AddRefs(server));
    
    nsCOMPtr<nsIFileSpec> folderRoot;
    if (NS_SUCCEEDED(rv) && server)
        rv = server->GetLocalPath(getter_AddRefs(folderRoot));
    
    if (NS_SUCCEEDED(rv) && folderRoot) {
        // everyone should have a inbox so let's
        // tack that folder name on to the root path...
        rv = folderRoot->GetFileSpec(&m_folderPath);
        if (NS_SUCCEEDED(rv))
          m_folderPath += "Inbox";
    } // if we have a folder root for the current server
    
    // create Undo/Redo Transaction Manager
    NS_WITH_SERVICE (nsIComponentManager, compMgr, kComponentManagerCID, &rv);
    if (NS_SUCCEEDED(rv))
    {
      rv = compMgr->CreateInstance(kTransactionManagerCID, nsnull, 
                                   nsCOMTypeInfo<nsITransactionManager>::GetIID(),
                                   getter_AddRefs(mTxnMgr));
      if (NS_SUCCEEDED(rv))
        mTxnMgr->SetMaxTransactionCount(-1);
    }
}

NS_IMETHODIMP
nsMessenger::OpenURL(const char * url)
{
	if (url)
	{
#ifdef DEBUG_MESSENGER
		printf("nsMessenger::OpenURL(%s)\n",url);
#endif    
        char* unescapedUrl = PL_strdup(url);
        if (unescapedUrl)
        {
          nsUnescape(unescapedUrl);
          
          nsIMsgMessageService * messageService = nsnull;
          nsresult rv = GetMessageServiceFromURI(unescapedUrl,
                                                 &messageService);
          
          if (NS_SUCCEEDED(rv) && messageService)
          {
			messageService->DisplayMessage(unescapedUrl, mWebShell, nsnull, nsnull);
			ReleaseMessageServiceFromURI(unescapedUrl, messageService);
          }
		//If it's not something we know about, then just load the url.
          else
          {
			nsString urlStr(unescapedUrl);
			if (mWebShell) {
				mWebShell->LoadURL(urlStr.GetUnicode());
			}
          }
          PL_strfree(unescapedUrl);
        }
        else
        {
          return NS_ERROR_OUT_OF_MEMORY;
        }
	}
	return NS_OK;
}


NS_IMETHODIMP
nsMessenger::OpenAttachment(const char * url, const char * displayName, 
                            const char * messageUri)
{
  nsresult rv = NS_ERROR_FAILURE;
  char *unescapedUrl = nsnull;
  nsIMsgMessageService * messageService = nsnull;

	if (url)
	{
#ifdef DEBUG_MESSENGER
		printf("nsMessenger::OpenAttachment(%s)\n",url);
#endif    
        unescapedUrl = PL_strdup(url);
        if (unescapedUrl)
        {
          nsUnescape(unescapedUrl);
          
          nsCOMPtr<nsIFileSpec> aSpec;
          nsCOMPtr<nsIFileSpecWithUI>
            fileSpec(getter_AddRefs(NS_CreateFileSpecWithUI()));
          
          if (!fileSpec) goto done;

          rv = fileSpec->ChooseOutputFile("Save Attachment", displayName,
                                          nsIFileSpecWithUI::eAllFiles);
          if (NS_FAILED(rv)) goto done;
            
          aSpec = do_QueryInterface(fileSpec, &rv);
          if (NS_FAILED(rv)) goto done;

          nsSaveAsListener *aListener = new nsSaveAsListener(aSpec);
          if (aListener)
          {
            NS_ADDREF(aListener);
            nsCOMPtr<nsIURI> aURL;

            nsString urlString = unescapedUrl;
            char *urlCString = urlString.ToNewCString();
            rv = CreateStartupUrl(urlCString, getter_AddRefs(aURL));
            nsCRT::free(urlCString);
            if (NS_FAILED(rv)) 
            {
              NS_RELEASE(aListener);
              goto done;
            }
            aListener->m_channel = null_nsCOMPtr();
            NS_WITH_SERVICE(nsIIOService, netService, kIOServiceCID, &rv);
            rv = netService->NewInputStreamChannel(aURL,
                                                   nsnull,      // contentType
                                                   -1,          // contentLength
                                                   nsnull,      // inputStream
                                                   nsnull,      // loadGroup
                                                   nsnull,      // originalURI
                                                   getter_AddRefs(aListener->m_channel));
            nsAutoString from, to;
            from = "message/rfc822";
            to = "text/xul";
            NS_WITH_SERVICE(nsIStreamConverterService,
                            streamConverterService,  
                            kIStreamConverterServiceCID, &rv);
            nsCOMPtr<nsISupports> channelSupport =
              do_QueryInterface(aListener->m_channel);
            nsCOMPtr<nsIStreamListener> convertedListener;
            rv = streamConverterService->AsyncConvertData(
              from.GetUnicode(), to.GetUnicode(), aListener,
              channelSupport, getter_AddRefs(convertedListener));

            rv = GetMessageServiceFromURI(messageUri, &messageService);
            
            if (NS_SUCCEEDED(rv) && messageService)
            {
              rv = messageService->DisplayMessage(messageUri,
                                                  convertedListener,
                                                  nsnull, nsnull); 
            }
          }
          else
          {
            rv = NS_ERROR_OUT_OF_MEMORY;
          }
        }
        else
        {
          rv =  NS_ERROR_OUT_OF_MEMORY;
        }
	}
done:
    if (messageService)
      ReleaseMessageServiceFromURI(unescapedUrl, messageService);
    PR_FREEIF(unescapedUrl);
	return rv;
}


NS_IMETHODIMP
nsMessenger::SaveAs(const char* url, PRBool asFile)
{
	nsresult rv = NS_OK;
    nsIMsgMessageService* messageService = nsnull;
    
    if (url)
    {
      rv = GetMessageServiceFromURI(url, &messageService);

      if (NS_SUCCEEDED(rv) && messageService)
      {
        nsCOMPtr<nsIFileSpec> aSpec;
        PRUint32 saveAsFileType = 0; // 0 - raw, 1 = html, 2 = text;

        if (asFile)
        {
          nsCOMPtr<nsIFileSpecWithUI>
            fileSpec(getter_AddRefs(NS_CreateFileSpecWithUI()));
 
          if (!fileSpec) {
            rv = NS_ERROR_FAILURE;
            goto done;
          }

          rv = fileSpec->ChooseOutputFile("Save Message", "",
                                nsIFileSpecWithUI::eAllMailOutputFilters);
          if (NS_FAILED(rv)) goto done;
          char *fileName = nsnull;
          fileSpec->GetLeafName(&fileName);

          nsCString tmpFilenameString = fileName;
      
          nsCRT::free(fileName);

          if (tmpFilenameString.RFind(".htm", PR_TRUE) != -1)
            saveAsFileType = 1;
          else if (tmpFilenameString.RFind(".txt", PR_TRUE) != -1)
            saveAsFileType = 2;
          
          aSpec = do_QueryInterface(fileSpec, &rv);
        }
        else
        {
          nsFileSpec tmpFileSpec("msgTemp.eml");
          rv = NS_NewFileSpecWithSpec(tmpFileSpec, getter_AddRefs(aSpec));
        }
        if (NS_FAILED(rv)) goto done;
        nsCOMPtr<nsIUrlListener> urlListener;
        if (aSpec)
        {
          if (asFile)
          {
            switch (saveAsFileType) 
            {
              case 0:
              default:
                messageService->SaveMessageToDisk(url, aSpec, PR_TRUE,
                                                  urlListener, nsnull);
                break;
              case 1:
              case 2:
              {
                nsSaveAsListener *aListener = new nsSaveAsListener(aSpec);
                if (aListener)
                {
                  NS_ADDREF(aListener);
                  nsCOMPtr<nsIURI> aURL;
                  nsString urlString = url;
                  urlString += "?header=none";
                  char *urlCString = urlString.ToNewCString();
                  rv = CreateStartupUrl(urlCString, getter_AddRefs(aURL));
                  nsCRT::free(urlCString);
                  if (NS_FAILED(rv)) 
                  {
                    NS_RELEASE(aListener);
                    goto done;
                  }
                  aListener->m_channel = null_nsCOMPtr();
                  NS_WITH_SERVICE(nsIIOService, netService, kIOServiceCID,
                                  &rv);
                  rv = netService->NewInputStreamChannel(aURL, 
                                                         nsnull,      // contentType
                                                         -1,          // contentLength
                                                         nsnull,      // inputStream
                                                         nsnull,      // loadGroup
                                                         nsnull,      // originalURI
                                                         getter_AddRefs(aListener->m_channel));
                  nsAutoString from, to;
                  from = "message/rfc822";
                  to = saveAsFileType == 1 ? "text/html" : "text/plain";
                  NS_WITH_SERVICE(nsIStreamConverterService,
                                  streamConverterService,  
                                  kIStreamConverterServiceCID, &rv);
                  nsCOMPtr<nsISupports> channelSupport =
                    do_QueryInterface(aListener->m_channel);
                  nsCOMPtr<nsIStreamListener> convertedListener;
                  rv = streamConverterService->AsyncConvertData(
                    from.GetUnicode(), to.GetUnicode(), aListener,
                    channelSupport, getter_AddRefs(convertedListener));
                  if (NS_SUCCEEDED(rv))
                    messageService->DisplayMessage(url, convertedListener,
                                                   nsnull, nsnull);
                }
              }
            }
          }
          else
          { 
              // ** save as Template
              PRBool needDummyHeader = PR_TRUE;
              char * templateUri = nsnull;
              NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
              if (NS_FAILED(rv)) goto done;
              prefs->CopyCharPref("mail.default_templates_uri", &templateUri);
              if (!templateUri || !*templateUri) 
                  return NS_ERROR_FAILURE;
              needDummyHeader =
                  PL_strcasestr(templateUri, "mailbox") != nsnull;
              nsCRT::free(templateUri);
              nsSaveAsListener *aListener = new nsSaveAsListener(aSpec);
              if (aListener)
              {
                  rv = aListener->QueryInterface(
                      nsCOMTypeInfo<nsIUrlListener>::GetIID(),
                      getter_AddRefs(urlListener));
                  if (NS_FAILED(rv))
                  {
                      delete aListener;
                      return rv;
                  }
                  NS_ADDREF(aListener); 
                  // nsUrlListenerManager uses nsVoidArray
                  // to keep trach of all listeners we have
                  // to manually add refs ourself
                  messageService->SaveMessageToDisk(url, aSpec, 
                                                    needDummyHeader,
                                                    urlListener, nsnull);
              }
          }
        }
      }
    }
done:
    if (messageService)
        ReleaseMessageServiceFromURI(url, messageService);

	return rv;
}

nsresult
nsMessenger::DoCommand(nsIRDFCompositeDataSource* db, char *command,
                       nsISupportsArray *srcArray, 
                       nsISupportsArray *argumentArray)
{

	nsresult rv;

    NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv);
	if(NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsIRDFResource> commandResource;
	rv = rdfService->GetResource(command, getter_AddRefs(commandResource));
	if(NS_SUCCEEDED(rv))
	{
        // ** jt - temporary solution for pickybacking the undo manager into
        // the nsISupportArray
        if (mTxnMgr)
            srcArray->InsertElementAt(mTxnMgr, 0);
		rv = db->DoCommand(srcArray, commandResource, argumentArray);
	}

	return rv;

}

NS_IMETHODIMP
nsMessenger::DeleteMessages(nsIDOMXULElement *tree, nsIDOMXULElement *srcFolderElement, nsIDOMNodeList *nodeList)
{
	nsresult rv;

	if(!tree || !srcFolderElement || !nodeList)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsIRDFCompositeDataSource> database;
	nsCOMPtr<nsISupportsArray> resourceArray, folderArray;
	nsCOMPtr<nsIRDFResource> resource;

	rv = srcFolderElement->GetResource(getter_AddRefs(resource));

	if(NS_FAILED(rv))
		return rv;

	rv = tree->GetDatabase(getter_AddRefs(database));
	if(NS_FAILED(rv))
		return rv;

	rv =ConvertDOMListToResourceArray(nodeList, getter_AddRefs(resourceArray));
	if(NS_FAILED(rv))
		return rv;

	rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	folderArray->AppendElement(resource);
	
	rv = DoCommand(database, NC_RDF_DELETE, folderArray, resourceArray);

	return rv;
}

NS_IMETHODIMP nsMessenger::DeleteFolders(nsIRDFCompositeDataSource *db, nsIDOMXULElement *parentFolderElement,
							nsIDOMXULElement *folderElement)
{
	nsresult rv;

	if(!db || !parentFolderElement || !folderElement)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsISupportsArray> parentArray, deletedArray;
	nsCOMPtr<nsIRDFResource> parentResource, deletedFolderResource;

	rv = parentFolderElement->GetResource(getter_AddRefs(parentResource));

	if(NS_FAILED(rv))
		return rv;

	rv = folderElement->GetResource(getter_AddRefs(deletedFolderResource));

	if(NS_FAILED(rv))
		return rv;

	rv = NS_NewISupportsArray(getter_AddRefs(parentArray));

	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	rv = NS_NewISupportsArray(getter_AddRefs(deletedArray));

	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	parentArray->AppendElement(parentResource);
	deletedArray->AppendElement(deletedFolderResource);

	rv = DoCommand(db, NC_RDF_DELETE, parentArray, deletedArray);

	return NS_OK;
}

NS_IMETHODIMP
nsMessenger::CopyMessages(nsIRDFCompositeDataSource *database, nsIDOMXULElement *srcFolderElement,
						  nsIDOMXULElement *dstFolderElement, nsIDOMNodeList *messages, PRBool isMove)
{
	nsresult rv;

	if(!srcFolderElement || !dstFolderElement || !messages)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsIRDFResource> srcResource, dstResource;
	nsCOMPtr<nsIMsgFolder> srcFolder;
	nsCOMPtr<nsISupportsArray> argumentArray;
	nsCOMPtr<nsISupportsArray> folderArray;

	rv = dstFolderElement->GetResource(getter_AddRefs(dstResource));
	if(NS_FAILED(rv))
		return rv;

	rv = srcFolderElement->GetResource(getter_AddRefs(srcResource));
	if(NS_FAILED(rv))
		return rv;

	srcFolder = do_QueryInterface(srcResource);
	if(!srcFolder)
		return NS_ERROR_NO_INTERFACE;

	rv =ConvertDOMListToResourceArray(messages, getter_AddRefs(argumentArray));
	if(NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsISupports> srcFolderSupports(do_QueryInterface(srcFolder));
	if(srcFolderSupports)
		argumentArray->InsertElementAt(srcFolderSupports, 0);

	rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	folderArray->AppendElement(dstResource);
	
	rv = DoCommand(database, isMove ? (char *)NC_RDF_MOVE : (char *)NC_RDF_COPY, folderArray, argumentArray);
	return rv;

}

NS_IMETHODIMP
nsMessenger::GetRDFResourceForMessage(nsIDOMXULElement *tree,
                                       nsIDOMNodeList *nodeList, nsISupports
                                       **aSupport) 
{
      nsresult rv;
	  if(!tree || !nodeList)
		  return NS_ERROR_NULL_POINTER;

      nsISupportsArray *resourceArray;
    nsIBidirectionalEnumerator *aEnumerator = nsnull;
    *aSupport = nsnull;
    nsISupports *aItem = nsnull;

      if(NS_FAILED(rv =ConvertDOMListToResourceArray(nodeList, &resourceArray)))
              return rv;

    rv = NS_NewISupportsArrayEnumerator(resourceArray, &aEnumerator);
    if (NS_FAILED(rv)) return rv;

    rv = aEnumerator->First();
    while (rv == NS_OK)
    {
        rv = aEnumerator->CurrentItem(&aItem);
        if (rv != NS_OK) break;
        rv = aItem->QueryInterface(nsCOMTypeInfo<nsIMessage>::GetIID(), (void**)aSupport);
        aItem->Release();
        if (rv == NS_OK && *aSupport) break;
        rv = aEnumerator->Next();
    }

    aEnumerator->Release();
      NS_RELEASE(resourceArray);
      return rv;
}

NS_IMETHODIMP
nsMessenger::Exit()
{
	nsresult rv = NS_OK;
  /*
   * Create the Application Shell instance...
   */
  NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv);
  if (NS_SUCCEEDED(rv))
    appShell->Shutdown();
  return NS_OK;
}

NS_IMETHODIMP
nsMessenger::OnUnload()
{
    // ** clean up
    // *** jt - We seem to have one extra ref count. I have no idea where it
    // came from. This could be the global object we created in commandglue.js
    // which causes us to have one more ref count. Call Release() here
    // seems the right thing to do. This gurantees the nsMessenger instance
    // gets deleted after we close down the messenger window.
    
    // smfr the one extra refcount is the result of a bug 8555, which I have 
    // checked in a fix for. So I'm commenting out this extra release.
    //Release();
    return NS_OK;
}

NS_IMETHODIMP
nsMessenger::Close()
{
    nsresult rv = NS_OK;
    if (mWindow)
    {
        nsCOMPtr<nsIScriptGlobalObject>
            globalScript(do_QueryInterface(mWindow));
        nsCOMPtr<nsIWebShell> webshell, rootWebshell;
        if (globalScript)
            globalScript->GetWebShell(getter_AddRefs(webshell));
        if (webshell)
            webshell->GetRootWebShell(*getter_AddRefs(rootWebshell));
        if (rootWebshell) 
        {
            nsCOMPtr<nsIWebShellContainer> webshellContainer;
            nsCOMPtr<nsIWebShellWindow> webWindow;
            rootWebshell->GetContainer(*getter_AddRefs(webshellContainer));
            webWindow = do_QueryInterface(webshellContainer);
            if (webWindow) 
			{
				webWindow->Show(PR_FALSE);
                webWindow->Close();
			}
         }
    }

    return rv;
}


NS_IMETHODIMP
nsMessenger::MarkMessageRead(nsIRDFCompositeDataSource *database, nsIDOMXULElement *message, PRBool markRead)
{
	if(!database || !message)
		return NS_ERROR_NULL_POINTER;

	nsresult rv;

	nsCOMPtr<nsIRDFResource> messageResource;
	rv = message->GetResource(getter_AddRefs(messageResource));
	if(NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsISupportsArray> resourceArray;

	rv = NS_NewISupportsArray(getter_AddRefs(resourceArray));
	if(NS_FAILED(rv))
		return NS_ERROR_OUT_OF_MEMORY;

	resourceArray->AppendElement(messageResource);

	rv = DoMarkMessagesRead(database, resourceArray, markRead);

	return rv;
}

NS_IMETHODIMP
nsMessenger::MarkMessagesRead(nsIRDFCompositeDataSource *database, nsIDOMNodeList *messages, PRBool markRead)
{
	nsresult rv;

	if(!database || !messages)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsISupportsArray> resourceArray;


	rv =ConvertDOMListToResourceArray(messages, getter_AddRefs(resourceArray));
	if(NS_FAILED(rv))
		return rv;

	rv= DoMarkMessagesRead(database, resourceArray, markRead);


	return rv;
}

nsresult
nsMessenger::DoMarkMessagesRead(nsIRDFCompositeDataSource *database, nsISupportsArray *resourceArray, PRBool markRead)
{
	nsresult rv;
	nsCOMPtr<nsISupportsArray> argumentArray;

	rv = NS_NewISupportsArray(getter_AddRefs(argumentArray));
	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	if(markRead)
		rv = DoCommand(database, NC_RDF_MARKREAD, resourceArray, argumentArray);
	else
		rv = DoCommand(database, NC_RDF_MARKUNREAD, resourceArray,  argumentArray);

	return rv;

}

NS_IMETHODIMP
nsMessenger::MarkAllMessagesRead(nsIRDFCompositeDataSource *database, nsIDOMXULElement *folder)
{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> folderResource;
	nsCOMPtr<nsISupportsArray> folderArray;

	if(!folder || !database)
		return NS_ERROR_NULL_POINTER;

	rv = folder->GetResource(getter_AddRefs(folderResource));
	if(NS_FAILED(rv))
		return rv;

	if(NS_FAILED(NS_NewISupportsArray(getter_AddRefs(folderArray))))
		return NS_ERROR_OUT_OF_MEMORY;

	folderArray->AppendElement(folderResource);

	DoCommand(database, NC_RDF_MARKALLMESSAGESREAD, folderArray, nsnull);

	return rv;
}

NS_IMETHODIMP
nsMessenger::MarkMessageFlagged(nsIRDFCompositeDataSource *database, nsIDOMXULElement *message, PRBool markFlagged)
{
	if(!database || !message)
		return NS_ERROR_NULL_POINTER;

	nsresult rv;

	nsCOMPtr<nsIRDFResource> messageResource;
	rv = message->GetResource(getter_AddRefs(messageResource));
	if(NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsISupportsArray> resourceArray;

	rv = NS_NewISupportsArray(getter_AddRefs(resourceArray));
	if(NS_FAILED(rv))
		return NS_ERROR_OUT_OF_MEMORY;

	resourceArray->AppendElement(messageResource);

	rv = DoMarkMessagesFlagged(database, resourceArray, markFlagged);

	return rv;
}

NS_IMETHODIMP
nsMessenger::MarkMessagesFlagged(nsIRDFCompositeDataSource *database, nsIDOMNodeList *messages, PRBool markFlagged)
{
	nsresult rv;

	if(!database || !messages)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsISupportsArray> resourceArray;


	rv =ConvertDOMListToResourceArray(messages, getter_AddRefs(resourceArray));
	if(NS_FAILED(rv))
		return rv;

	rv= DoMarkMessagesFlagged(database, resourceArray, markFlagged);


	return rv;
}

nsresult
nsMessenger::DoMarkMessagesFlagged(nsIRDFCompositeDataSource *database, nsISupportsArray *resourceArray, PRBool markFlagged)
{
	nsresult rv;
	nsCOMPtr<nsISupportsArray> argumentArray;

	rv = NS_NewISupportsArray(getter_AddRefs(argumentArray));
	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	if(markFlagged)
		rv = DoCommand(database, NC_RDF_MARKFLAGGED, resourceArray, argumentArray);
	else
		rv = DoCommand(database, NC_RDF_MARKUNFLAGGED, resourceArray,  argumentArray);

	return rv;

}

NS_IMETHODIMP
nsMessenger::NewFolder(nsIRDFCompositeDataSource *database, nsIDOMXULElement *parentFolderElement,
						const char *name)
{
	nsresult rv;
	nsCOMPtr<nsIRDFResource> folderResource;
	nsCOMPtr<nsISupportsArray> nameArray, folderArray;

	if(!parentFolderElement || !name)
		return NS_ERROR_NULL_POINTER;

	rv = parentFolderElement->GetResource(getter_AddRefs(folderResource));
	if(NS_FAILED(rv))
		return rv;

	rv = NS_NewISupportsArray(getter_AddRefs(nameArray));
	if(NS_FAILED(rv))
	{
		return NS_ERROR_OUT_OF_MEMORY;
	}

	rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
	if(NS_FAILED(rv))
		return NS_ERROR_OUT_OF_MEMORY;

	folderArray->AppendElement(folderResource);

    NS_WITH_SERVICE(nsIRDFService, rdfService, kRDFServiceCID, &rv);
	if(NS_SUCCEEDED(rv))
	{
		nsString nameStr = name;
		nsCOMPtr<nsIRDFLiteral> nameLiteral;

		rdfService->GetLiteral(nameStr.GetUnicode(), getter_AddRefs(nameLiteral));
		nameArray->AppendElement(nameLiteral);
		rv = DoCommand(database, NC_RDF_NEWFOLDER, folderArray, nameArray);
	}
	return rv;
}

NS_IMETHODIMP
nsMessenger::RenameFolder(nsIRDFCompositeDataSource* db,
                          nsIDOMXULElement* folder,
                          const char* name)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  if (!db || !folder || !name || !*name) return rv;
  nsCOMPtr<nsISupports> streamSupport;
  rv = NS_NewCharInputStream(getter_AddRefs(streamSupport), name);
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsISupportsArray> folderArray;
    nsCOMPtr<nsISupportsArray> argsArray;
    nsCOMPtr<nsIRDFResource> folderResource;
    rv = folder->GetResource(getter_AddRefs(folderResource));
    if (NS_SUCCEEDED(rv))
    {
      rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
      if (NS_FAILED(rv)) return rv;
      folderArray->AppendElement(folderResource);
      rv = NS_NewISupportsArray(getter_AddRefs(argsArray));
      if (NS_FAILED(rv)) return rv;
      argsArray->AppendElement(streamSupport);
      rv = DoCommand(db, NC_RDF_RENAME, folderArray, argsArray);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsMessenger::CompactFolder(nsIRDFCompositeDataSource* db,
                           nsIDOMXULElement* folder)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  
  if (!db || !folder) return rv;
  nsCOMPtr<nsISupportsArray> folderArray;
  nsCOMPtr<nsIRDFResource> folderResource;

  rv = folder->GetResource(getter_AddRefs(folderResource));
  if (NS_SUCCEEDED(rv))
  {
    rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
    if (NS_FAILED(rv)) return rv;
    folderArray->AppendElement(folderResource);
    rv = DoCommand(db, NC_RDF_COMPACT, folderArray, nsnull);
  }
  return rv;
}

NS_IMETHODIMP
nsMessenger::EmptyTrash(nsIRDFCompositeDataSource* db,
                        nsIDOMXULElement* folder)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  
  if (!db || !folder) return rv;
  nsCOMPtr<nsISupportsArray> folderArray;
  nsCOMPtr<nsIRDFResource> folderResource;

  rv = folder->GetResource(getter_AddRefs(folderResource));
  if (NS_SUCCEEDED(rv))
  {
    rv = NS_NewISupportsArray(getter_AddRefs(folderArray));
    if (NS_FAILED(rv)) return rv;
    folderArray->AppendElement(folderResource);
    rv = DoCommand(db, NC_RDF_EMPTYTRASH, folderArray, nsnull);
  }
  return rv;
}

NS_IMETHODIMP
nsMessenger::Undo()
{
  nsresult rv = NS_OK;
  if (mTxnMgr)
  {
    PRInt32 numTxn = 0;
    rv = mTxnMgr->GetNumberOfUndoItems(&numTxn);
    if (NS_SUCCEEDED(rv) && numTxn > 0)
      mTxnMgr->Undo();
  }
  return rv;
}

NS_IMETHODIMP
nsMessenger::Redo()
{
  nsresult rv = NS_OK;
  if (mTxnMgr)
  {
    PRInt32 numTxn = 0;
    rv = mTxnMgr->GetNumberOfRedoItems(&numTxn);
    if (NS_SUCCEEDED(rv) && numTxn > 0)
      mTxnMgr->Redo();
  }
  return rv;
}

NS_IMETHODIMP
nsMessenger::GetTransactionManager(nsITransactionManager* *aTxnMgr)
{
  if (!mTxnMgr || !aTxnMgr)
    return NS_ERROR_NULL_POINTER;

  *aTxnMgr = mTxnMgr;
  NS_ADDREF(*aTxnMgr);

  return NS_OK;
}

NS_IMETHODIMP nsMessenger::SetDocumentCharset(const PRUnichar *characterSet)
{
	// Set a default charset of the webshell. 
	if (nsnull != mWebShell) {
		mWebShell->SetDefaultCharacterSet(characterSet);
	}
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the send operation. 
////////////////////////////////////////////////////////////////////////////////////
class SendLaterListener: public nsIMsgSendLaterListener
{
public:
  SendLaterListener(void);
  virtual ~SendLaterListener(void);

  // nsISupports interface
  NS_DECL_ISUPPORTS

  /* void OnStartSending (in PRUint32 aTotalMessageCount); */
  NS_IMETHOD OnStartSending(PRUint32 aTotalMessageCount);

  /* void OnProgress (in PRUint32 aCurrentMessage, in PRUint32 aTotalMessage); */
  NS_IMETHOD OnProgress(PRUint32 aCurrentMessage, PRUint32 aTotalMessage);

  /* void OnStatus (in wstring aMsg); */
  NS_IMETHOD OnStatus(const PRUnichar *aMsg);

  /* void OnStopSending (in nsresult aStatus, in wstring aMsg, in PRUint32 aTotalTried, in PRUint32 aSuccessful); */
  NS_IMETHOD OnStopSending(nsresult aStatus, const PRUnichar *aMsg, PRUint32 aTotalTried, PRUint32 aSuccessful);
};

NS_IMPL_ISUPPORTS(SendLaterListener, nsCOMTypeInfo<nsIMsgSendLaterListener>::GetIID());

SendLaterListener::SendLaterListener()
{
  NS_INIT_REFCNT();
}

SendLaterListener::~SendLaterListener()
{
}

nsresult
SendLaterListener::OnStartSending(PRUint32 aTotalMessageCount)
{
  return NS_OK;
}

nsresult
SendLaterListener::OnProgress(PRUint32 aCurrentMessage, PRUint32 aTotalMessage)
{
  return NS_OK;
}

nsresult
SendLaterListener::OnStatus(const PRUnichar *aMsg)
{
  return NS_OK;
}

nsresult
SendLaterListener::OnStopSending(nsresult aStatus, const PRUnichar *aMsg, PRUint32 aTotalTried, 
                                 PRUint32 aSuccessful) 
{
#ifdef NS_DEBUG
  if (NS_SUCCEEDED(aStatus))
    printf("SendLaterListener::OnStopSending: Tried to send %d messages. %d successful.\n",
            aTotalTried, aSuccessful);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsMessenger::SendUnsentMessages()
{
	nsresult rv;
	nsCOMPtr<nsIMsgSendLater> pMsgSendLater; 
	rv = nsComponentManager::CreateInstance(kMsgSendLaterCID, NULL,nsCOMTypeInfo<nsIMsgSendLater>::GetIID(),
																					(void **)getter_AddRefs(pMsgSendLater)); 
	if (NS_SUCCEEDED(rv) && pMsgSendLater) 
	{ 
#ifdef DEBUG
		printf("We succesfully obtained a nsIMsgSendLater interface....\n"); 
#endif

    SendLaterListener *sendLaterListener = new SendLaterListener();
    if (!sendLaterListener)
        return NS_ERROR_FAILURE;

    NS_ADDREF(sendLaterListener);
    pMsgSendLater->AddListener(sendLaterListener);

    // temporary hack to get the current identity
    NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIMsgIdentity> identity;
    rv = mailSession->GetCurrentIdentity(getter_AddRefs(identity));
    if (NS_FAILED(rv)) return rv;
      
    pMsgSendLater->SendUnsentMessages(identity, nsnull); 
    NS_RELEASE(sendLaterListener);
	} 
	return NS_OK;
}

NS_IMETHODIMP
nsMessenger::LoadFirstDraft()
{
	nsresult              rv = NS_ERROR_FAILURE;
	nsCOMPtr<nsIMsgDraft> pMsgDraft; 

	rv = nsComponentManager::CreateInstance(kMsgDraftCID, NULL, nsCOMTypeInfo<nsIMsgDraft>::GetIID(),
																					(void **)getter_AddRefs(pMsgDraft)); 
	if (NS_SUCCEEDED(rv) && pMsgDraft) 
	{ 
#ifdef DEBUG
		printf("We succesfully obtained a nsIMsgDraft interface....\n");
#endif


    // This should really pass in a URI, but for now, just to test, we can pass in nsnull
    rv = pMsgDraft->OpenDraftMsg(nsnull, nsnull, PR_FALSE); 
  } 

  return rv;
}

NS_IMETHODIMP nsMessenger::DoPrint()
{
#ifdef DEBUG_MESSENGER
  printf("nsMessenger::DoPrint()\n");
#endif

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIContentViewer> viewer;

  NS_ASSERTION(mWebShell,"can't print, there is no webshell");
  if (!mWebShell) {
	return rv;
  }

  mWebShell->GetContentViewer(getter_AddRefs(viewer));

  if (viewer) {
    rv = viewer->Print();
  }
#ifdef DEBUG_MESSENGER
  else {
	printf("failed to get the viewer for printing\n");
  }
#endif
  
  return rv;
}

NS_IMETHODIMP nsMessenger::DoPrintPreview()
{
  nsresult rv = NS_ERROR_NOT_IMPLEMENTED;
#ifdef DEBUG_MESSENGER
  printf("nsMessenger::DoPrintPreview() not implemented yet\n");
#endif
  return rv;  
}

nsSaveAsListener::nsSaveAsListener(nsIFileSpec* aSpec)
{
    NS_INIT_REFCNT();
    if (aSpec)
      m_fileSpec = do_QueryInterface(aSpec);
    m_dataBuffer = nsnull;
}

nsSaveAsListener::~nsSaveAsListener()
{
}

// 
// nsISupports
//
NS_IMPL_ISUPPORTS3(nsSaveAsListener, nsIUrlListener,
                   nsIMsgCopyServiceListener, nsIStreamListener)

// 
// nsIUrlListener
// 
NS_IMETHODIMP
nsSaveAsListener::OnStartRunningUrl(nsIURI* url)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSaveAsListener::OnStopRunningUrl(nsIURI* url, nsresult exitCode)
{
  nsresult rv = exitCode;
  char *templateUri = nsnull;

  if (m_fileSpec)
  {
    m_fileSpec->Flush();
    m_fileSpec->CloseStream();
    if (NS_FAILED(rv)) goto done;
    NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
    if (NS_FAILED(rv)) goto done;
    prefs->CopyCharPref("mail.default_templates_uri", &templateUri);
    if (!templateUri || !*templateUri) 
    {
      rv = NS_ERROR_FAILURE;
      goto done;
    }
    NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv);
    if (NS_FAILED(rv)) goto done;
    nsCOMPtr<nsIRDFResource> res;
    rv = rdf->GetResource(templateUri, getter_AddRefs(res));
    if (NS_FAILED(rv)) goto done;
    nsCOMPtr<nsIMsgFolder> templateFolder;
    templateFolder = do_QueryInterface(res, &rv);
    if (NS_FAILED(rv)) goto done;
    NS_WITH_SERVICE(nsIMsgCopyService, copyService, kMsgCopyServiceCID, &rv);
    if (NS_FAILED(rv)) goto done;
    rv = copyService->CopyFileMessage(m_fileSpec, templateFolder, nsnull,
                                      PR_TRUE, this, nsnull);
  }

done:
  if (NS_FAILED(rv))
  {
    if (m_fileSpec)
    {
      nsFileSpec realSpec;
      m_fileSpec->GetFileSpec(&realSpec);
      realSpec.Delete(PR_FALSE);
      Release(); // no more work to be done; kill ourself
    }
  }
  PR_FREEIF(templateUri);
  return rv;
}

NS_IMETHODIMP
nsSaveAsListener::OnStartCopy(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSaveAsListener::OnProgress(PRUint32 aProgress, PRUint32 aProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSaveAsListener::SetMessageKey(PRUint32 aKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSaveAsListener::GetMessageId(nsCString* aMessageId)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSaveAsListener::OnStopCopy(nsresult aStatus)
{
  if (m_fileSpec)
  {
    nsFileSpec realSpec;
    m_fileSpec->GetFileSpec(&realSpec);
    realSpec.Delete(PR_FALSE);
  }
  Release(); // all done kill ourself
  return aStatus;
}

NS_IMETHODIMP
nsSaveAsListener::OnStartRequest(nsIChannel* aChannel, nsISupports* aSupport)
{
  if (m_fileSpec)
    m_fileSpec->GetOutputStream(getter_AddRefs(m_outputStream));
  if (!m_dataBuffer)
    m_dataBuffer = (char*) PR_CALLOC(FOUR_K+1);
  return NS_OK;
}

NS_IMETHODIMP
nsSaveAsListener::OnStopRequest(nsIChannel* aChannel, nsISupports* aSupport,
                                nsresult status, const PRUnichar* aMsg)
{
    // close down the file stream and release ourself
  if (m_fileSpec)
  {
    m_fileSpec->Flush();
    m_fileSpec->CloseStream();
    m_outputStream = null_nsCOMPtr();
  }
  Release(); // all done kill ourself
  return NS_OK;
}

NS_IMETHODIMP
nsSaveAsListener::OnDataAvailable(nsIChannel* aChannel, 
                                  nsISupports* aSupport,
                                  nsIInputStream* inStream, 
                                  PRUint32 srcOffset,
                                  PRUint32 count)
{
  nsresult rv = NS_OK;
  if (m_dataBuffer && m_outputStream)
  {
    PRUint32 available, readCount, maxReadCount = FOUR_K;
    PRUint32 writeCount;
    rv = inStream->Available(&available);
    while (NS_SUCCEEDED(rv) && available)
    {
      if (maxReadCount > available)
        maxReadCount = available;
      rv = inStream->Read(m_dataBuffer, maxReadCount, &readCount);
      if (NS_SUCCEEDED(rv))
        rv = m_outputStream->Write(m_dataBuffer, readCount, &writeCount);
      available -= readCount;
    }
  }
  return rv;
}

// **** ForwardMessages ****
// type: -1: base on prefs 0: as attachment 1: as quoted 2: as inline
NS_IMETHODIMP
nsMessenger::ForwardMessages(nsIDOMNodeList *domNodeList, 
                             PRInt32 type)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISupportsArray> messageArray;

  if (!domNodeList) return rv;
  rv = ConvertDOMListToResourceArray(domNodeList,
                                     getter_AddRefs(messageArray));
  if (NS_FAILED(rv)) return rv;
  PRUint32 cnt = 0, i;
  rv = messageArray->Count(&cnt);
  if (NS_FAILED(rv)) return rv;
  if (!cnt) return NS_ERROR_NULL_POINTER;
  
  if (type < 0 || type > 2)
  {
    NS_WITH_SERVICE(nsIPref, prefs, kPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = prefs->GetIntPref("mail.forward_message_mode", &type);
    // use forward as attachment if failed
    if (NS_FAILED(rv))
      type = 0;
  }

  NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kCMsgMailSessionCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIMsgIdentity> identity;
  rv = mailSession->GetCurrentIdentity(getter_AddRefs(identity));
  if (NS_FAILED(rv)) return rv;

  NS_WITH_SERVICE (nsIMsgComposeService, composeService,
                   kMsgComposeServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;

  NS_WITH_SERVICE (nsIComponentManager, compMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIMsgCompFields> compFields;
  nsCOMPtr<nsIRDFResource> rdfResource;
  nsCOMPtr<nsISupports> msgSupport;
  char *msgUri = nsnull;
  
  switch (type)
  {
      default:
      case 0: // forward as attachments
      {
          if (NS_SUCCEEDED(rv))
          {
              rv = compMgr->CreateInstance(kMsgCompFieldsCID, nsnull, 
                                           nsCOMTypeInfo<nsIMsgCompFields>::GetIID(),
                                           getter_AddRefs(compFields));
              if (NS_FAILED(rv)) return rv;
          }
          nsString attachments;
          for (i=0; i<cnt; i++)
          {
              msgSupport = getter_AddRefs(messageArray->ElementAt(i));
              if (msgSupport)
              {
                  rdfResource = do_QueryInterface(msgSupport, &rv);
                  if (NS_SUCCEEDED(rv))
                  {
                      rv = rdfResource->GetValue(&msgUri);
                      if (msgUri)
                      {
                          if (attachments.Length())
                              attachments += ',';
                          attachments += msgUri;
                          nsCRT::free(msgUri);
                      }
                  }
              }
          }
          nsString subject;
          nsString fwdSubject;
          nsString charSet;
          nsString encodedCharset;
          nsString decodedString;
          msgSupport = getter_AddRefs(messageArray->ElementAt(0));
          nsCOMPtr<nsIDBMessage> aMessage = do_QueryInterface(msgSupport);
          if (aMessage)
          {
            nsCOMPtr<nsIMsgDBHdr> aMsgHdr;
            aMessage->GetMsgDBHdr(getter_AddRefs(aMsgHdr));
            if (aMsgHdr)
            {
              aMsgHdr->GetCharSet(&charSet);
              aMsgHdr->GetSubject(&subject);
              fwdSubject = "[Fwd: ";
              fwdSubject += subject;
              fwdSubject += "]";
            }
          }
          if (!charSet.Equals(""))
            compFields->SetCharacterSet(charSet.GetUnicode());
          if (NS_SUCCEEDED(rv = nsMsgI18NDecodeMimePartIIStr(fwdSubject, 
                                                             encodedCharset, 
                                                             decodedString)))
            compFields->SetSubject(decodedString.GetUnicode());
          else if (fwdSubject.Length())
            compFields->SetSubject(fwdSubject.GetUnicode());
          compFields->SetAttachments(attachments.GetUnicode());
          rv = composeService->OpenComposeWindowWithCompFields(nsnull,
                                     nsIMsgCompFormat::Default, compFields);
          break;
      }
      case 1: // forward as quoted; obsolete, treat it as inline
      case 2: // forward as inline
      {
          for (i=0; i<cnt; i++)
          {
              msgSupport = getter_AddRefs(messageArray->ElementAt(i));
              if (msgSupport)
              {
                  rdfResource = do_QueryInterface(msgSupport, &rv);
                  if(NS_SUCCEEDED(rv))
                  {
                      rv = rdfResource->GetValue(&msgUri);
                      if (NS_SUCCEEDED(rv) && msgUri)
                      {
                        nsAutoString str (msgUri);
                        nsCOMPtr<nsIMsgDraft> pMsgDraft;
                        rv = compMgr->CreateInstance(kMsgDraftCID,
                                                     nsnull,
                                                     nsCOMTypeInfo<nsIMsgDraft>::GetIID(), 
                                                     getter_AddRefs(pMsgDraft));
                        if (NS_SUCCEEDED(rv) && pMsgDraft)
                          pMsgDraft->OpenDraftMsg(str.GetUnicode(),
                                                  nsnull, PR_TRUE);
						nsAllocator::Free(msgUri);
                      }
                  }
              }
          }
          break;
      }
  }

  return rv;
}
