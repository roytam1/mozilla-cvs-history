/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsSoftwareUpdateStream.h"
#include "nscore.h"
#include "nsFileSpec.h"
#include "nsVector.h"

#include "nsISupports.h"
#include "nsIServiceManager.h"

#include "nsIURL.h"
#include "nsINetlibURL.h"
#ifndef NECKO
#include "nsINetService.h"
#else
#include "nsIIOService.h"
#include "nsIChannel.h"
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
#include "nsIEventQueueService.h"
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);
#endif // NECKO
#include "nsIInputStream.h"
#include "nsIStreamListener.h"

#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"


static NS_DEFINE_IID(kISoftwareUpdateIID, NS_ISOFTWAREUPDATE_IID);
static NS_DEFINE_IID(kSoftwareUpdateCID,  NS_SoftwareUpdate_CID);


nsSoftwareUpdateListener::nsSoftwareUpdateListener(const nsString& fromURL, const nsString& localFile, long flags)
{
    NS_INIT_REFCNT();
    
    mFromURL    = fromURL;
    mLocalFile  = localFile;
    mFlags      = flags;

    mOutFileDesc = PR_Open(nsAutoCString(localFile),  PR_CREATE_FILE | PR_RDWR, 0744);
    
    if(mOutFileDesc == NULL)
    {
        mResult = -1;
    };

    mResult = nsServiceManager::GetService(  kSoftwareUpdateCID, 
                                             kISoftwareUpdateIID,
                                             (nsISupports**)&mSoftwareUpdate);
    
    if (NS_FAILED(mResult))
        return;

    nsIURL  *pURL  = nsnull;
#ifndef NECKO
    mResult = NS_NewURL(&pURL, fromURL);
    if (NS_FAILED(mResult)) 
        return;
 
    mResult = NS_OpenURL(pURL, this);
#else
    nsresult rv;
    NS_WITH_SERVICE(nsIIOService, service, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return;

    nsIEventQueue *eventQ = nsnull;
    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueService, &rv);
    if (NS_FAILED(rv)) return;
    
    mResult = eventQService->GetThreadEventQueue(PR_CurrentThread(), &eventQ);
    if (NS_FAILED(rv)) return;

    // XXX NECKO verb? getter?
    nsIChannel *channel = nsnull;
    const char *urlStr = fromURL.GetBuffer();
    rv = service->NewChannel("load", urlStr, nsnull, nsnull, &channel);
    if (NS_FAILED(mResult)) return;

    rv = channel->AsyncRead(0, -1, nsnull, eventQ, this);
    NS_RELEASE(eventQ);
    NS_RELEASE(channel);
    mResult = rv;
    return;
#endif // NECKO
}

nsSoftwareUpdateListener::~nsSoftwareUpdateListener()
{    
    mSoftwareUpdate->Release();
}


NS_IMPL_ISUPPORTS( nsSoftwareUpdateListener, kIStreamListenerIID )

NS_IMETHODIMP
nsSoftwareUpdateListener::GetBindInfo(nsIURL* aURL, nsStreamBindingInfo* info)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSoftwareUpdateListener::OnProgress( nsIURL* aURL,
                          PRUint32 Progress,
                          PRUint32 ProgressMax)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSoftwareUpdateListener::OnStatus(nsIURL* aURL, 
                       const PRUnichar* aMsg)
{ 
  return NS_OK;
}

NS_IMETHODIMP
nsSoftwareUpdateListener::OnStartBinding(nsIURL* aURL, 
                             const char *aContentType)
{
  return NS_OK;
}

NS_IMETHODIMP
nsSoftwareUpdateListener::OnStopBinding(nsIURL* aURL,
                                        nsresult status,
                                        const PRUnichar* aMsg)
{
    switch( status ) 
    {

        case NS_BINDING_SUCCEEDED:
                PR_Close(mOutFileDesc);
                mSoftwareUpdate->InstallJar(mFromURL,
                                            mLocalFile,
                                            mFlags );
            break;

        case NS_BINDING_FAILED:
        case NS_BINDING_ABORTED:
            mResult = status;
            PR_Close(mOutFileDesc);
            break;

        default:
            mResult = NS_ERROR_ILLEGAL_VALUE;
    }

    return mResult;
}

#define BUF_SIZE 1024

NS_IMETHODIMP
nsSoftwareUpdateListener::OnDataAvailable(nsIURL* aURL, nsIInputStream *pIStream, PRUint32 length)
{
    PRUint32 len;
    nsresult err;
    char buffer[BUF_SIZE];
    
    do 
    {
        err = pIStream->Read(buffer, BUF_SIZE, &len);
        
        if (mResult == 0 && err == 0)
        {
            if ( PR_Write(mOutFileDesc, buffer, len) == -1 )
            {
                /* Error */ 
                return -1;
            }
        }

    } while (len > 0 && err == NS_OK);

    return 0;
}