/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Daniel Veditz <dveditz@netscape.com>
 */

#include "nscore.h"
#include "nsFileSpec.h"
#include "pratom.h"

#include "nsISupports.h"
#include "nsIServiceManager.h"

#include "nsIURL.h"

#include "nsNetUtil.h"
#include "nsIBufferInputStream.h"
#include "nsIInputStream.h"
#include "nsIStreamListener.h"

#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"

#include "nsXPITriggerInfo.h"
#include "nsXPInstallManager.h"
#include "nsInstallProgressDialog.h"
#include "nsInstallResources.h"
#include "nsSpecialSystemDirectory.h"
#include "nsFileStream.h"
#include "nsProxyObjectManager.h"

#include "nsIAppShellComponentImpl.h"
#include "nsIPrompt.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kAppShellServiceCID, NS_APPSHELL_SERVICE_CID );
static NS_DEFINE_IID(kProxyObjectManagerCID, NS_PROXYEVENT_MANAGER_CID);
static NS_DEFINE_IID(kStringBundleServiceCID, NS_STRINGBUNDLESERVICE_CID);

#define XPINSTALL_BUNDLE_URL "chrome://xpinstall/locale/xpinstall.properties"

nsXPInstallManager::nsXPInstallManager()
  : mTriggers(0), mItem(0), mNextItem(0), mNumJars(0), mFinalizing(PR_FALSE)
{
    NS_INIT_ISUPPORTS();

    // we need to own ourself because we have a longer
    // lifetime than the scriptlet that created us.
    NS_ADDREF_THIS();

    // get the resourced xpinstall string bundle
    mStringBundle = nsnull;
    nsIStringBundleService *service;
    nsresult rv = nsServiceManager::GetService( kStringBundleServiceCID, 
                                       nsIStringBundleService::GetIID(),
                                       (nsISupports**) &service );
    if (NS_SUCCEEDED(rv) && service)
    {
        nsILocale* locale = nsnull;
        rv = service->CreateBundle( XPINSTALL_BUNDLE_URL, locale,
                                    getter_AddRefs(mStringBundle) );
        nsServiceManager::ReleaseService( kStringBundleServiceCID, service );
    }
}



nsXPInstallManager::~nsXPInstallManager()
{    
    if (mTriggers) 
        delete mTriggers;
}


NS_IMPL_ADDREF( nsXPInstallManager );
NS_IMPL_RELEASE( nsXPInstallManager );

NS_IMETHODIMP 
nsXPInstallManager::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
  if (!aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(nsIXPINotifier::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIXPINotifier*,this);
  else if (aIID.Equals(nsIStreamListener::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIStreamListener*,this);
  else if (aIID.Equals(nsIXULWindowCallbacks::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIXULWindowCallbacks*,this);
  else if (aIID.Equals(nsIProgressEventSink::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIProgressEventSink*,this);
  else if (aIID.Equals(nsIInterfaceRequestor::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIInterfaceRequestor*,this);
  else if (aIID.Equals(kISupportsIID))
    *aInstancePtr = NS_STATIC_CAST( nsISupports*, NS_STATIC_CAST(nsIXPINotifier*,this));
  else
    *aInstancePtr = 0;

  nsresult status;
  if ( !*aInstancePtr )
    status = NS_NOINTERFACE;
  else 
  {
    NS_ADDREF_THIS();
    status = NS_OK;
  }

  return status;
}



NS_IMETHODIMP
nsXPInstallManager::InitManager(nsXPITriggerInfo* aTriggers)
{
    nsresult rv = NS_OK;
    mTriggers = aTriggers;

    if ( !mTriggers || mTriggers->Size() == 0 )
    {
        rv = NS_ERROR_INVALID_POINTER;
        NS_RELEASE_THIS();
        return rv;
    }

    //-----------------------------------------------------
    // confirm that install is OK... use stock Confirm()
    // dialog for now, later we'll want a fancier one.
    //-----------------------------------------------------
    PRBool OKtoInstall = PR_FALSE;

    NS_WITH_SERVICE(nsIAppShellService, appShell, kAppShellServiceCID, &rv );
    if ( NS_SUCCEEDED( rv ) ) 
    {
        nsCOMPtr<nsIWebShellWindow> wbwin;
        rv = appShell->GetHiddenWindow(getter_AddRefs(wbwin));
        if ( NS_SUCCEEDED(rv) )
        {
            nsCOMPtr<nsIPrompt> prompt( do_QueryInterface(wbwin, &rv) );
            if ( NS_SUCCEEDED(rv) && prompt )
            {
                PRBool bStrBdlSuccess = PR_FALSE;
                nsString rsrcName = "ShouldWeInstallMsg";

                if (mStringBundle)
                {
                    const PRUnichar* ucRsrcName = rsrcName.GetUnicode();
                    PRUnichar* ucRsrcVal = nsnull;
                    rv = mStringBundle->GetStringFromName(ucRsrcName, &ucRsrcVal);
                    if (NS_SUCCEEDED(rv) && ucRsrcVal)
                    {
                        prompt->Confirm( ucRsrcVal, &OKtoInstall);
                        nsCRT::free(ucRsrcVal);
                        bStrBdlSuccess = PR_TRUE;
                    }
                }

                /* failover to default english strings */
                if (!bStrBdlSuccess)
                {
                    char *cResName = rsrcName.ToNewCString();
                    nsString resVal = nsInstallResources::GetDefaultVal(cResName);

                    if (!resVal.IsEmpty())
                        prompt->Confirm( resVal.GetUnicode(), &OKtoInstall );

                    if (cResName)
                        Recycle(cResName);
                }
            }
        }
    }


    // --- create and open the progress dialog
    if (NS_SUCCEEDED(rv) && OKtoInstall)
    {
        nsInstallProgressDialog* dlg;
        nsCOMPtr<nsISupports>    Idlg;

        dlg = new nsInstallProgressDialog(this);

        if ( dlg )
        {
            rv = dlg->QueryInterface( nsIXPIProgressDlg::GetIID(), 
                                      getter_AddRefs(mDlg) );
            if (NS_SUCCEEDED(rv))
            {
                NS_WITH_SERVICE( nsIProxyObjectManager, pmgr, kProxyObjectManagerCID, &rv);
                if (NS_SUCCEEDED(rv))
                {
                    rv = pmgr->GetProxyObject( 0, nsIXPIProgressDlg::GetIID(),
                            mDlg, PROXY_SYNC | PROXY_ALWAYS, getter_AddRefs(mProxy) );

                }
                
                if (NS_SUCCEEDED(rv))
                {        
                    rv = mDlg->Open();
                }
            }
        }
        else
            rv = NS_ERROR_OUT_OF_MEMORY;
    }

    PRInt32 cbstatus = 0;  // callback status
    if (NS_SUCCEEDED(rv) && !OKtoInstall )
        cbstatus = nsInstall::USER_CANCELLED;
    else if (!NS_SUCCEEDED(rv))
        cbstatus = nsInstall::UNEXPECTED_ERROR;

    if ( cbstatus != 0 )
    {
        // --- inform callbacks of error
        for (PRUint32 i = 0; i < mTriggers->Size(); i++)
        {
            mTriggers->SendStatus( mTriggers->Get(i)->mURL.GetUnicode(),
                                   cbstatus );
        }

        // -- must delete ourselves if not continuing
        NS_RELEASE_THIS();
    }

    return rv;
}


nsresult nsXPInstallManager::DownloadNext()
{
    nsresult rv;
    if ( mNextItem < mTriggers->Size() )
    {
        // download the next item in list

        mItem = (nsXPITriggerItem*)mTriggers->Get(mNextItem++);

        NS_ASSERTION( mItem, "bogus Trigger slipped through" );
        NS_ASSERTION( mItem->mURL.Length() > 0, "bogus trigger");
        if ( !mItem || mItem->mURL.Length() == 0 )
        {
            // XXX serious problem with trigger! try to carry on
            rv = DownloadNext();
        }
        else if ( mItem->IsFileURL() )
        {
            // don't need to download, just point at local file
            rv = NS_NewFileSpecWithSpec( nsFileSpec(nsFileURL(mItem->mURL)),
                    getter_AddRefs(mItem->mFile) );
            if (NS_FAILED(rv))
            {
                // serious problem with trigger! try to carry on
                mTriggers->SendStatus( mItem->mURL.GetUnicode(), 
                                       nsInstall::DOWNLOAD_ERROR );
                mItem->mFile = 0;
            }

            rv = DownloadNext();
        }
        else
        {
            // We have one to download

            // --- figure out a temp file name
            nsSpecialSystemDirectory temp(nsSpecialSystemDirectory::OS_TemporaryDirectory);
            PRInt32 pos = mItem->mURL.RFindChar('/');
            if ( pos != -1 )
            {
                nsString jarleaf;
                mItem->mURL.Right( jarleaf, mItem->mURL.Length() - pos);
                temp += jarleaf;
            }
            else
                temp += "xpinstall.xpi";

            temp.MakeUnique();

            rv = NS_NewFileSpecWithSpec( temp, getter_AddRefs(mItem->mFile) );
            if (NS_SUCCEEDED(rv))
            {
                 // --- start the download
                nsCOMPtr<nsIURI> pURL;
                rv = NS_NewURI(getter_AddRefs(pURL), mItem->mURL);
                
                if (NS_SUCCEEDED(rv)) 
                {
                    nsCOMPtr<nsIChannel> channel;
                
                    rv = NS_OpenURI(getter_AddRefs(channel), pURL, nsnull, this);
                
                    if (NS_SUCCEEDED(rv))
                    {
                        rv = channel->AsyncRead(0, -1, nsnull, this);
                    }
                }
            }

            if (NS_FAILED(rv))
            {
                // announce failure
                mTriggers->SendStatus( mItem->mURL.GetUnicode(), 
                                       nsInstall::DOWNLOAD_ERROR );
                mItem->mFile = 0;
                // carry on with the next one
                rv = DownloadNext();
            }
        }
    }
    else
    {
        // all downloaded, queue them for installation

        NS_WITH_SERVICE(nsISoftwareUpdate, softupdate, nsSoftwareUpdate::GetCID(), &rv);
        if (NS_SUCCEEDED(rv))
        {
            for (PRUint32 i = 0; i < mTriggers->Size(); ++i)
            {
                mItem = (nsXPITriggerItem*)mTriggers->Get(i);
                if ( mItem && mItem->mFile )
                {
                    rv = softupdate->InstallJar(mItem->mFile,
                                                mItem->mURL.GetUnicode(),
                                                mItem->mArguments.GetUnicode(),
                                                mItem->mFlags,
                                                this );
                    if (NS_SUCCEEDED(rv))
                        PR_AtomicIncrement(&mNumJars);
                    else
                        mTriggers->SendStatus( mItem->mURL.GetUnicode(),
                                               nsInstall::UNEXPECTED_ERROR );
                }
            }
        }
        else
        {
            ; // XXX gotta clean up all those files...
        }

        if ( mNumJars == 0 )
        {
            // We must clean ourself up now -- we won't be called back
            Shutdown();
        }
    }

    return rv;
}

void nsXPInstallManager::Shutdown()
{
    if (mProxy)
    {
        // proxy exists: we're being called from script thread
        mProxy->Close();
        mProxy = 0;
    }
    else if (mDlg)
        mDlg->Close();

    mDlg = 0;

    NS_RELEASE_THIS();
}


NS_IMETHODIMP
nsXPInstallManager::OnStartRequest(nsIChannel* channel, nsISupports *ctxt)
{
    mItem->mFile->OpenStreamForWriting();
    return NS_OK;
}

NS_IMETHODIMP
nsXPInstallManager::OnStopRequest(nsIChannel* channel, nsISupports *ctxt,
                                  nsresult status, const PRUnichar *errorMsg)
{
    nsresult rv;
    switch( status ) 
    {

        case NS_BINDING_SUCCEEDED:
            rv = NS_OK;
            break;

        case NS_BINDING_FAILED:
        case NS_BINDING_ABORTED:
            rv = status;
            // XXX need to note failure, both to send back status
            // to the callback, and also so we don't try to install
            // this probably corrupt file.
            break;

        default:
            rv = NS_ERROR_ILLEGAL_VALUE;
    }

    mItem->mFile->CloseStream();

    if (!NS_SUCCEEDED(rv))
    {
        nsFileSpec fspec;
        rv = mItem->mFile->GetFileSpec(&fspec);
        if ( NS_SUCCEEDED(rv) && fspec.Exists() )
            fspec.Delete(0);

        mItem->mFile = 0;

        mTriggers->SendStatus( mItem->mURL.GetUnicode(),
                               nsInstall::DOWNLOAD_ERROR );
    }
    DownloadNext();
    return rv;
}
NS_IMETHODIMP
nsXPInstallManager::OnDataAvailable(nsIChannel* channel, nsISupports *ctxt, 
                                    nsIInputStream *pIStream,
                                    PRUint32 sourceOffset, 
                                    PRUint32 length)
{
    PRUint32 amt;
    PRInt32  result;
    nsresult err;
    char buffer[1025];
    
    do 
    {
        err = pIStream->Read(buffer, 1024, &amt);
        if (amt == 0) break;
        if (NS_FAILED(err)) 
        {
            //printf("pIStream->Read Failed!  %d", err);   
            return err;
        }
        err = mItem->mFile->Write( buffer, amt, &result);
        //printf("mItem->mFile->Write err:%d   amt:%d    result:%d\n", err, amt, result);
        if (NS_FAILED(err) || result != (PRInt32)amt) 
        {
            //printf("mItem->mFile->Write Failed!  err:%d   amt:%d    result:%d\n", err, amt, result);
            return NS_ERROR_FAILURE;
        }
    } while (amt > 0);

    return NS_OK;
}

NS_IMETHODIMP 
nsXPInstallManager::OnProgress(nsIChannel *channel, nsISupports *ctxt, PRUint32 aProgress, PRUint32 aProgressMax)
{
     return mProxy->SetProgress(aProgress, aProgressMax);
}

NS_IMETHODIMP 
nsXPInstallManager::OnStatus(nsIChannel *channel, nsISupports *ctxt, const PRUnichar *aMsg)
{
    return mProxy->SetActionText(aMsg);
}

// nsIInterfaceRequestor method
NS_IMETHODIMP 
nsXPInstallManager::GetInterface(const nsIID & eventSinkIID, void* *_retval)
{
    return QueryInterface(eventSinkIID, (void**)_retval);
}

// IXPINotifier methods

NS_IMETHODIMP 
nsXPInstallManager::BeforeJavascriptEvaluation(const PRUnichar *URL)
{
    nsresult rv = NS_OK;

    mFinalizing = PR_FALSE;

    return rv;
}

NS_IMETHODIMP 
nsXPInstallManager::AfterJavascriptEvaluation(const PRUnichar *URL)
{
    PR_AtomicDecrement( &mNumJars );
    if ( mNumJars == 0 )
        Shutdown();

    return NS_OK;
}

NS_IMETHODIMP 
nsXPInstallManager::InstallStarted(const PRUnichar *URL, const PRUnichar *UIPackageName)
{
    mProxy->SetActionText(nsnull);
    return mProxy->SetHeading( nsString(UIPackageName).GetUnicode() );
}

NS_IMETHODIMP 
nsXPInstallManager::ItemScheduled(const PRUnichar *message)
{
    return mProxy->SetActionText( nsString(message).GetUnicode() );
}

NS_IMETHODIMP 
nsXPInstallManager::FinalizeProgress(const PRUnichar *message, PRInt32 itemNum, PRInt32 totNum)
{
    if (!mFinalizing)
    {
        mFinalizing = PR_TRUE;
        if (mStringBundle)
        {
            nsString rsrcName = "FinishingInstallMsg";
            const PRUnichar* ucRsrcName = rsrcName.GetUnicode();
            PRUnichar* ucRsrcVal = nsnull;
            nsresult rv = mStringBundle->GetStringFromName(ucRsrcName, &ucRsrcVal);
            if (NS_SUCCEEDED(rv) && ucRsrcVal)
            {
                mProxy->SetActionText( ucRsrcVal );
                nsCRT::free(ucRsrcVal);
            }
        }
    }
    return mProxy->SetProgress( itemNum, totNum );
}

NS_IMETHODIMP 
nsXPInstallManager::FinalStatus(const PRUnichar *URL, PRInt32 status)
{
    mTriggers->SendStatus( URL, status );
    return NS_OK;
}

NS_IMETHODIMP 
nsXPInstallManager::LogComment(const PRUnichar* comment)
{
    return NS_OK;
}

// nsIXULWindowCallbacks

NS_IMETHODIMP
nsXPInstallManager::ConstructBeforeJavaScript(nsIWebShell *aWebShell)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPInstallManager::ConstructAfterJavaScript(nsIWebShell *aWebShell)
{
    return DownloadNext();
}
