/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 *
 */

// nsMsgPrintEngine.cpp: provides a DocShell container for use 
// in printing

#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsIComponentManager.h"

#include "nsISupports.h"

#include "nsIURI.h"
#include "nsEscape.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIWebShell.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocumentViewer.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIMsgMessageService.h"
#include "nsMsgUtils.h"
#include "nsIWebProgress.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIMsgMailSession.h"
#include "nsMsgPrintEngine.h"
#include "nsMsgBaseCID.h"
#include "nsIDocumentLoader.h"

// Interfaces Needed
#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIWebNavigation.h"
#include "nsIChannel.h"

// Print Options
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);

/////////////////////////////////////////////////////////////////////////
// nsMsgPrintEngine implementation
/////////////////////////////////////////////////////////////////////////

static NS_DEFINE_CID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

nsMsgPrintEngine::nsMsgPrintEngine()
{
  mCurrentlyPrintingURI = -1;
  NS_INIT_REFCNT();
}


nsMsgPrintEngine::~nsMsgPrintEngine()
{
}

// Implement AddRef and Release
NS_IMPL_ISUPPORTS3(nsMsgPrintEngine,
                         nsIMsgPrintEngine, 
                         nsIWebProgressListener, 
                         nsISupportsWeakReference);

// nsIWebProgressListener implementation
NS_IMETHODIMP
nsMsgPrintEngine::OnStateChange(nsIWebProgress* aWebProgress, 
                   nsIRequest *aRequest, 
                   PRInt32 progressStateFlags, 
                   nsresult aStatus)
{
  nsresult rv = NS_OK;

  // top-level document load data
  if (progressStateFlags & nsIWebProgressListener::STATE_IS_DOCUMENT) {
    if (progressStateFlags & nsIWebProgressListener::STATE_START) {
      // Tell the user we are loading...
      PRUnichar *msg = GetString(NS_LITERAL_STRING("LoadingMessageToPrint").get());
      SetStatusMessage( msg );
      CRTFREEIF(msg)
    }

    if (progressStateFlags & nsIWebProgressListener::STATE_STOP) {
      PRBool isPrintingCancelled = PR_FALSE;
      if (mPrintSettings)
      {
        mPrintSettings->GetIsCancelled(&isPrintingCancelled);
      }
      if (!isPrintingCancelled) {
        // if aWebProgress is a documentloader than the notification from
        // loading the documents. If it is NULL (or not a DocLoader) then it 
        // it coming from Printing
        nsCOMPtr<nsIDocumentLoader> docLoader(do_QueryInterface(aWebProgress));
        if (docLoader) {
          // Now, fire off the print operation!
          rv = NS_ERROR_FAILURE;

          // Tell the user the message is loaded...
          PRUnichar *msg = GetString(NS_LITERAL_STRING("MessageLoaded").get());
          SetStatusMessage( msg );
          if (msg) nsCRT::free(msg);

          NS_ASSERTION(mDocShell,"can't print, there is no docshell");
          if ( (!mDocShell) || (!aRequest) ) 
          {
            return StartNextPrintOperation();
          }
          nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(aRequest);
          if (!aChannel) return NS_ERROR_FAILURE;

          // Make sure this isn't just "about:blank" finishing....
          nsCOMPtr<nsIURI> originalURI = nsnull;
          if (NS_SUCCEEDED(aChannel->GetOriginalURI(getter_AddRefs(originalURI))) && originalURI)
          {
            nsXPIDLCString spec;

            if (NS_SUCCEEDED(originalURI->GetSpec(getter_Copies(spec))))
            {      
              if (spec.Equals("about:blank"))
              {
                return StartNextPrintOperation();
              }
            }
          }

          mDocShell->GetContentViewer(getter_AddRefs(mContentViewer));  
          if (mContentViewer) 
          {
            mWebBrowserPrint = do_QueryInterface(mContentViewer);
            if (mWebBrowserPrint) 
            {
              if (!mPrintSettings) 
              {
                nsCOMPtr<nsIPrintOptions> printService = do_GetService(kPrintOptionsCID, &rv);
                if (NS_SUCCEEDED(rv)) 
                {
                  printService->CreatePrintSettings(getter_AddRefs(mPrintSettings));
                  NS_ASSERTION(mPrintSettings, "You can't PrintPreview without a PrintSettings!");
                }
              }
              mPrintSettings->SetPrintSilent(mCurrentlyPrintingURI != 0);
              rv = mWebBrowserPrint->Print(mPrintSettings, (nsIWebProgressListener *)this);

              if (NS_FAILED(rv))
              {
                mWebBrowserPrint = nsnull;
                mContentViewer = nsnull;
                PRBool isPrintingCancelled = PR_FALSE;
                if (mPrintSettings)
                {
                  mPrintSettings->GetIsCancelled(&isPrintingCancelled);
                }
                if (!isPrintingCancelled) 
                {
                  StartNextPrintOperation();
                } 
                else 
                {
                  mWindow->Close();
                }
              }
              else
              {
                // Tell the user we started printing...
                msg = GetString(NS_LITERAL_STRING("PrintingMessage").get());
                SetStatusMessage( msg );
                CRTFREEIF(msg)
              }
            }
          }
        } else {
          StartNextPrintOperation();
          rv = NS_OK;
        }
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsMsgPrintEngine::OnProgressChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     PRInt32 aCurSelfProgress,
                                     PRInt32 aMaxSelfProgress,
                                     PRInt32 aCurTotalProgress,
                                     PRInt32 aMaxTotalProgress) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMsgPrintEngine::OnLocationChange(nsIWebProgress* aWebProgress,
                      nsIRequest* aRequest,
                      nsIURI *location) {
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMsgPrintEngine::OnStatusChange(nsIWebProgress* aWebProgress,
                    nsIRequest* aRequest,
                    nsresult aStatus,
                    const PRUnichar* aMessage) {
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMsgPrintEngine::OnSecurityChange(nsIWebProgress *aWebProgress, 
                      nsIRequest *aRequest, 
                      PRInt32 state) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsMsgPrintEngine::SetWindow(nsIDOMWindowInternal *aWin)
{
	if (!aWin)
  {
    // It isn't an error to pass in null for aWin, in fact it means we are shutting
    // down and we should start cleaning things up...
		return NS_OK;
  }

  mWindow = aWin;

  nsCOMPtr<nsIScriptGlobalObject> globalObj( do_QueryInterface(aWin) );
  NS_ENSURE_TRUE(globalObj, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell;
  globalObj->GetDocShell(getter_AddRefs(docShell));

  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> rootAsItem;
  docShellAsItem->GetSameTypeRootTreeItem(getter_AddRefs(rootAsItem));

  nsCOMPtr<nsIDocShellTreeNode> rootAsNode(do_QueryInterface(rootAsItem));
  NS_ENSURE_TRUE(rootAsNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShellTreeItem> childItem;
  rootAsNode->FindChildWithName(NS_LITERAL_STRING("printengine").get(), PR_TRUE, PR_FALSE, nsnull,
    getter_AddRefs(childItem));

  mDocShell = do_QueryInterface(childItem);

  if(mDocShell)
    SetupObserver();

  return NS_OK;
}

NS_IMETHODIMP
nsMsgPrintEngine::AddPrintURI(const PRUnichar *aMsgURI)
{
  mURIArray.AppendString(nsDependentString(aMsgURI));
  return NS_OK;
}

NS_IMETHODIMP
nsMsgPrintEngine::SetPrintURICount(PRInt32 aCount)
{
  mURICount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgPrintEngine::StartPrintOperation()
{
  // Load the about:blank on the tail end...
  nsresult rv = AddPrintURI(NS_LITERAL_STRING("about:blank").get()); 
  if (NS_FAILED(rv)) return rv; 
  return StartNextPrintOperation();
}

NS_IMETHODIMP
nsMsgPrintEngine::StartNextPrintOperation()
{
  nsresult      rv;

  // Only do this the first time through...
  if (mCurrentlyPrintingURI == -1)
    InitializeDisplayCharset();

  mCurrentlyPrintingURI++;

  // First, check if we are at the end of this stuff!
  if ( mCurrentlyPrintingURI >= mURIArray.Count() )
  {
    // This is the end...dum, dum, dum....my only friend...the end
    mWindow->Close();

    // Tell the user we are done...
    PRUnichar *msg = GetString(NS_LITERAL_STRING("PrintingComplete").get());
    SetStatusMessage( msg );
    CRTFREEIF(msg)
    
    return NS_OK;
  }

  if (!mDocShell)
    return StartNextPrintOperation();

  nsString *uri = mURIArray.StringAt(mCurrentlyPrintingURI);
  rv = FireThatLoadOperation(uri);
  if (NS_FAILED(rv))
    return StartNextPrintOperation();
  else
    return rv;
}

NS_IMETHODIMP    
nsMsgPrintEngine::SetStatusFeedback(nsIMsgStatusFeedback *aFeedback)
{
	mFeedback = aFeedback;
	return NS_OK;
}

#define DATA_URL_PREFIX     "data:"
#define DATA_URL_PREFIX_LEN 5

#define ADDBOOK_URL_PREFIX     "addbook:"
#define ADDBOOK_URL_PREFIX_LEN 8

NS_IMETHODIMP
nsMsgPrintEngine::FireThatLoadOperation(nsString *uri)
{
  nsresult rv = NS_OK;

  char  *tString = ToNewCString(*uri);
  if (!tString)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr <nsIMsgMessageService> messageService;

  // if this is a data: url, skip it, because 
  // we've already got something we can print
  // and we know it is not a message.
  //
  // if this an about:blank url, skip it, because
  //
  // if this is an addbook: url, skip it, because
  // we know that isn't a message.
  if (PL_strncmp(tString, DATA_URL_PREFIX, DATA_URL_PREFIX_LEN) && 
      PL_strncmp(tString, ADDBOOK_URL_PREFIX, ADDBOOK_URL_PREFIX_LEN) && 
      PL_strcmp(tString, "about:blank")) {
    rv = GetMessageServiceFromURI(tString, getter_AddRefs(messageService));
  }

  if (NS_SUCCEEDED(rv) && messageService)
  {
    nsCOMPtr<nsIWebShell> webShell(do_QueryInterface(mDocShell));
    rv = messageService->DisplayMessageForPrinting(tString, webShell, nsnull, nsnull, nsnull);
  }
  //If it's not something we know about, then just load try loading it directly.
  else
  {
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
    if (webNav)
      rv = webNav->LoadURI(uri->get(),                        // URI string
                           nsIWebNavigation::LOAD_FLAGS_NONE, // Load flags
                           nsnull,                            // Refering URI
                           nsnull,                            // Post data
                           nsnull);                           // Extra headers
  }

  if (tString) nsCRT::free(tString);
  return rv;
}

void
nsMsgPrintEngine::InitializeDisplayCharset()
{
  // libmime always converts to UTF-8 (both HTML and XML)
  if (mDocShell) 
  {
    nsAutoString aForceCharacterSet(NS_LITERAL_STRING("UTF-8"));
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) 
    {
      nsCOMPtr<nsIMarkupDocumentViewer> muDV = do_QueryInterface(cv);
      if (muDV) 
      {
        muDV->SetForceCharacterSet(aForceCharacterSet.get());
      }
    }
  }
}

void
nsMsgPrintEngine::SetupObserver()
{
  if (!mDocShell)
    return;

  if (mDocShell)
  {
    nsCOMPtr<nsIWebProgress> progress(do_GetInterface(mDocShell));
    NS_ASSERTION(progress, "we were expecting a nsIWebProgress");
    if (progress) {
      (void) progress->AddProgressListener((nsIWebProgressListener *)this);
    }
  }
}

nsresult
nsMsgPrintEngine::SetStatusMessage(PRUnichar *aMsgString)
{
  if ( (!mFeedback) || (!aMsgString) )
    return NS_OK;

  mFeedback->ShowStatusString(aMsgString);
  return NS_OK;
}

#define MESSENGER_STRING_URL       "chrome://messenger/locale/messenger.properties"

PRUnichar *
nsMsgPrintEngine::GetString(const PRUnichar *aStringName)
{
	nsresult    res = NS_OK;
  PRUnichar   *ptrv = nsnull;

	if (!mStringBundle)
	{
		static const char propertyURL[] = MESSENGER_STRING_URL;

		nsCOMPtr<nsIStringBundleService> sBundleService = 
		         do_GetService(kStringBundleServiceCID, &res); 
		if (NS_SUCCEEDED(res) && (nsnull != sBundleService)) 
		{
			res = sBundleService->CreateBundle(propertyURL, getter_AddRefs(mStringBundle));
		}
	}

	if (mStringBundle)
		res = mStringBundle->GetStringFromName(aStringName, &ptrv);

  if ( NS_SUCCEEDED(res) && (ptrv) )
    return ptrv;
  else
    return nsCRT::strdup(aStringName);
}
