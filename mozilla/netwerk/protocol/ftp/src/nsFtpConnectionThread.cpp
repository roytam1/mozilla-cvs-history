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
 */
#include "nsFTPChannel.h"
#include "nsFtpConnectionThread.h"
#include "nsFtpControlConnection.h"
#include "nsFtpProtocolHandler.h"

#include <limits.h>

#include "nsISocketTransport.h"
#include "nsIStreamConverterService.h"
#include "prprf.h"
#include "prlog.h"
#include "prnetdb.h"
#include "netCore.h"
#include "ftpCore.h"
#include "nsProxiedService.h"
#include "nsCRT.h"
#include "nsIInterfaceRequestor.h"
#include "nsIURL.h"
#include "nsEscape.h"
#include "nsNetUtil.h"
#include "nsIDNSService.h" // for host error code
#include "nsIWalletService.h"
#include "nsIProxy.h"
#include "nsIMemory.h"
#include "nsIStringStream.h"
#include "nsIPref.h"
#include "nsMimeTypes.h"

#ifdef DOUGT_NEW_CACHE
#include "nsIInputStreamTee.h"
#endif

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static NS_DEFINE_CID(kWalletServiceCID, NS_WALLETSERVICE_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID,    NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gFTPLog;
#endif /* PR_LOGGING */


class DataRequestForwarder : public nsIChannel, public nsIStreamListener
{
public:
    DataRequestForwarder();
    virtual ~DataRequestForwarder();
    nsresult Init(nsIRequest *request);

#ifdef DOUGT_NEW_CACHE
    nsresult SetCacheEntryDescriptor(nsICacheEntryDescriptor *desc);
#endif
    nsresult SetStreamListener(nsIStreamListener *listener);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    
    NS_FORWARD_NSIREQUEST(mRequest->)
    NS_FORWARD_NSICHANNEL(mChannel->)

protected:
#ifdef DOUGT_NEW_CACHE
    nsCOMPtr<nsICacheEntryDescriptor> mCacheWriteDescriptor;
    nsCOMPtr<nsIOutputStream> mCacheOutputStream;
#endif

    nsCOMPtr<nsIRequest> mRequest;
    nsCOMPtr<nsIChannel> mChannel;
    nsCOMPtr<nsIStreamListener> mListener;

    nsresult DelayedOnStartRequest(nsIRequest *request, nsISupports *ctxt);
    PRBool  mDelayedOnStartFired;
};


// The whole purpose of this class is to opaque 
// the socket transport so that clients only see
// the same nsIChannel/nsIRequest that they started.

NS_IMPL_THREADSAFE_ISUPPORTS4(DataRequestForwarder, 
                              nsIStreamListener, 
                              nsIRequestObserver, 
                              nsIChannel,
                              nsIRequest);


DataRequestForwarder::DataRequestForwarder()
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder CREATED\n", this));
        
    mDelayedOnStartFired = PR_FALSE;
    NS_INIT_ISUPPORTS();
}

DataRequestForwarder::~DataRequestForwarder()
{
}

#ifdef DOUGT_NEW_CACHE
nsresult 
DataRequestForwarder::SetCacheEntryDescriptor(nsICacheEntryDescriptor *desc)
{
    mCacheWriteDescriptor = desc; 
    return NS_OK;
}
#endif

nsresult 
DataRequestForwarder::Init(nsIRequest *request)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder Init [request=%x]\n", this, request));

    NS_ENSURE_ARG(request);

    // for the forwarding declarations.
    mRequest  = request;
    mChannel  = do_QueryInterface(request);
    
    if (!mRequest || !mChannel)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}

nsresult 
DataRequestForwarder::SetStreamListener(nsIStreamListener *listener)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder SetStreamListener [listener=%x]\n", this, listener)); 
    
    mListener = listener;
    if (!mListener) 
        return NS_ERROR_FAILURE;

    return NS_OK;
}

nsresult 
DataRequestForwarder::DelayedOnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder DelayedOnStartRequest \n", this)); 
#ifdef DOUGT_NEW_CACHE

    if (mCacheWriteDescriptor) {   
        nsCOMPtr<nsITransport> transport;

        nsresult rv = mCacheWriteDescriptor->GetTransport(getter_AddRefs(transport));
        if (NS_FAILED(rv)) {
            NS_ASSERTION(0, "Could not create cache transport.");
            return rv;
        }
        rv = transport->OpenOutputStream(0, PRUint32(-1), 0, getter_AddRefs(mCacheOutputStream));
        
        if (NS_FAILED(rv)) {
            NS_ASSERTION(0, "Could not create cache output stream.");
            return rv;
        }
    }
#endif
    return mListener->OnStartRequest(this, ctxt); 
}

NS_IMETHODIMP 
DataRequestForwarder::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder OnStartRequest \n", this)); 
    return NS_OK;
}

NS_IMETHODIMP
DataRequestForwarder::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult statusCode)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) DataRequestForwarder OnStopRequest [status=%x]\n", this, statusCode)); 
#ifdef DOUGT_NEW_CACHE
    if (mCacheWriteDescriptor) {
        nsresult rv;
        if (NS_SUCCEEDED(statusCode))
        {
            // We have a write now, lets just mark this entry valid.
            rv = mCacheWriteDescriptor->MarkValid();
        }
        else
        {
            rv = mCacheWriteDescriptor->Doom();
        }
        if (NS_FAILED(rv)) {
            NS_ASSERTION(0, "Modifing Cache during onStopRequest failed.");
            return rv;
        }
    }
#endif
    if (mListener)
        return mListener->OnStopRequest(this, ctxt, statusCode);
    
    return NS_OK;
}

NS_IMETHODIMP
DataRequestForwarder::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, nsIInputStream *input, PRUint32 offset, PRUint32 count)
{ 
    nsresult rv;
    NS_ASSERTION(mListener, "No Listener Set.");
    if (!mListener)
        return NS_ERROR_NOT_INITIALIZED;

    // we want to delay firing the onStartRequest until we know that there is data
    if (!mDelayedOnStartFired) { 
        mDelayedOnStartFired = PR_TRUE;
        rv = DelayedOnStartRequest(request, ctxt);
        if (NS_FAILED(rv)) return rv;
    }

#ifdef DOUGT_NEW_CACHE
    if (mCacheWriteDescriptor) {
        
        nsCOMPtr<nsIInputStream> tee;
        nsresult rv = NS_NewInputStreamTee(getter_AddRefs(tee), 
                                           input,
                                           mCacheOutputStream);
        if (NS_FAILED(rv))
            return rv;
        return mListener->OnDataAvailable(this, ctxt, tee, offset, count); 
    }
#endif
    return mListener->OnDataAvailable(this, ctxt, input, offset, count); 
} 


NS_IMPL_THREADSAFE_ISUPPORTS3(nsFtpState, 
                              nsIStreamListener, 
                              nsIRequestObserver, 
                              nsIRequest);

nsFtpState::nsFtpState() {
    NS_INIT_REFCNT();

    PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("(%x) nsFtpState created", this));
    // bool init
#ifdef DOUGT_NEW_CACHE
    mReadingFromCache = PR_FALSE;
#endif    
    mWaitingForDConn = mTryingCachedControl = mList = mRetryPass = PR_FALSE;
    mFireCallbacks = mKeepRunning = mAnonymous = PR_TRUE;

    mAction = GET;
    mState = FTP_COMMAND_CONNECT;
    mNextState = FTP_S_USER;

    mInternalError = NS_OK;
    mSuspendCount = 0;
    mPort = 21;

    mLastModified = LL_ZERO;
    mWriteCount = 0;
    
    mControlStatus = NS_OK;            
    mControlReadContinue = PR_FALSE;
    mControlReadBrokenLine = PR_FALSE;

    mIPv6Checked = PR_FALSE;
    mIPv6ServerAddress = nsnull;
    
    mControlConnection = nsnull;
    mDRequestForwarder = nsnull;

    mGenerateHTMLContent = PR_FALSE;
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, pPref, kPrefCID, &rv); 
    if (NS_SUCCEEDED(rv) || pPref) 
        pPref->GetBoolPref("network.dir.generate_html", &mGenerateHTMLContent);
}

nsFtpState::~nsFtpState() 
{
    PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("(%x) nsFtpState destroyed", this));
    
    if (mIPv6ServerAddress) nsMemory::Free(mIPv6ServerAddress);
    NS_IF_RELEASE(mDRequestForwarder);
}


// nsIStreamListener implementation
NS_IMETHODIMP
nsFtpState::OnDataAvailable(nsIRequest *request,
                            nsISupports *aContext,
                            nsIInputStream *aInStream,
                            PRUint32 aOffset, 
                            PRUint32 aCount)
{    
    if (aCount == 0)
        return NS_OK; /*** should this be an error?? */
    
    char* buffer = (char*)nsMemory::Alloc(aCount + 1);
    nsresult rv = aInStream->Read(buffer, aCount, &aCount);
    if (NS_FAILED(rv)) 
    {
        // EOF
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) nsFtpState - received EOF\n", this));
        mState = FTP_COMPLETE;
        mInternalError = NS_ERROR_FAILURE;
        return NS_ERROR_FAILURE;
    }
    buffer[aCount] = '\0';
        

#if defined(PR_LOGGING)
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) reading %d bytes: \"%s\"", this, aCount, buffer));
#if 0 //#ifdef DEBUG_dougt
    printf("!!! %s\n", buffer);
#endif
#endif

    if (mFTPEventSink)
        mFTPEventSink->OnFTPControlLog(PR_TRUE, buffer);
    
    // get the response code out.
    if (!mControlReadContinue && !mControlReadBrokenLine) {
        PR_sscanf(buffer, "%d", &mResponseCode);
        if (buffer[3] == '-') {
            mControlReadContinue = PR_TRUE;
        } else {
            mControlReadBrokenLine = !PL_strstr(buffer, CRLF);
        }
    }

    // see if we're handling a multi-line response.
    if (mControlReadContinue) {
        // yup, multi-line response, start appending
        char *tmpBuffer = nsnull, *crlf = nsnull;
        
        // if we have data from our last round be sure
        // to prepend it.
        char *cleanup = nsnull;
        if (!mControlReadCarryOverBuf.IsEmpty() ) {
            mControlReadCarryOverBuf += buffer;
            cleanup = tmpBuffer = mControlReadCarryOverBuf.ToNewCString();
            mControlReadCarryOverBuf.Truncate(0);
            if (!tmpBuffer) return NS_ERROR_OUT_OF_MEMORY;
        } else {
            tmpBuffer = buffer;
        }
        
        PRBool lastLine = PR_FALSE;
        while ( (crlf = PL_strstr(tmpBuffer, CRLF)) ) {
            char tmpChar = crlf[2];
            crlf[2] = '\0'; 
            // see if this is the last line
            if (tmpBuffer[3] != '-' &&
                nsCRT::IsAsciiDigit(tmpBuffer[0]) &&
                nsCRT::IsAsciiDigit(tmpBuffer[1]) &&
                nsCRT::IsAsciiDigit(tmpBuffer[2]) ) {
                lastLine = PR_TRUE;
            }
            mResponseMsg += tmpBuffer+4; // skip over the code and '-'
            crlf[2] = tmpChar;
            tmpBuffer = crlf+2;
        }
        if (*tmpBuffer)
            mControlReadCarryOverBuf = tmpBuffer;
        
        if (cleanup) nsMemory::Free(cleanup);
        
        // see if this was the last line
        if (lastLine) {
            // yup. last line, let's move on.
            if (mState == mNextState) {
                NS_ASSERTION(0, "ftp read state mixup");
                mInternalError = NS_ERROR_FAILURE;
                mState = FTP_ERROR;
            } else {
                mState = mNextState;
            }
            mControlReadContinue = PR_FALSE;
        } else {
            // don't increment state, we need to read more.
            mControlReadContinue = PR_TRUE;
        }
    }
    else
    {
        NS_ASSERTION(mState != mNextState, "ftp read state mixup");
    
        // get the rest of the line
        if (mControlReadBrokenLine) {
            mResponseMsg += buffer;
            mControlReadBrokenLine = !PL_strstr(buffer, CRLF);
            if (!mControlReadBrokenLine)
                mState = mNextState;
        } else {
            mResponseMsg = buffer+4;
            mState = mNextState;
        }
    }
    
    if (!mControlReadContinue)
        return Process();

    return NS_OK;
}


// nsIRequestObserver implementation
NS_IMETHODIMP
nsFtpState::OnStartRequest(nsIRequest *request, nsISupports *aContext)
{
#if defined(PR_LOGGING)
    nsXPIDLCString spec;
    (void)mURL->GetSpec(getter_Copies(spec));

    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) nsFtpState::OnStartRequest() (spec =%s)\n",
        this, NS_STATIC_CAST(const char*, spec)));
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::OnStopRequest(nsIRequest *request, nsISupports *aContext,
                            nsresult aStatus)
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) nsFtpState::OnStopRequest() rv=%x\n", this, aStatus));
    mControlStatus = aStatus;

    // HACK.  This may not always work.  I am assuming that I can mask the OnStopRequest
    // notification since I created it via the control connection.
    if (mTryingCachedControl && NS_FAILED(aStatus) && NS_SUCCEEDED(mInternalError)) {
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) nsFtpState::OnStopRequest() cache connect failed.  Reconnecting...\n", this, aStatus));
        mTryingCachedControl = PR_FALSE;
        Connect();
        return NS_OK;
    }        
    
    StopProcessing();
    return NS_OK;
}

nsresult
nsFtpState::EstablishControlConnection()
{
    nsresult rv;

    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) trying cached control\n", this));
        
    nsISupports* connection;
    (void) nsFtpProtocolHandler::RemoveConnection(mURL, &connection);
    
    if (connection) {
        mControlConnection = NS_STATIC_CAST(nsFtpControlConnection*, (nsIStreamListener*)connection);
        if (mControlConnection->IsAlive())
        {
            // set stream listener of the control connection to be us.        
            (void) mControlConnection->SetStreamListener(NS_STATIC_CAST(nsIStreamListener*, this));
            
            // read cached variables into us. 
            mServerType = mControlConnection->mServerType;           
            mCwd        = mControlConnection->mCwd;                  
            mList       = mControlConnection->mList;                 
            mPassword   = mControlConnection->mPassword;

            mTryingCachedControl = PR_TRUE;
            
            // we're already connected to this server, skip login.
            mState = FTP_S_PASV;
            mResponseCode = 530;  //assume the control connection was dropped.
            mControlStatus = NS_OK;            
            // if we succeed, return.  Otherwise, we need to 
            // create a transport
            return NS_OK;
        }
#if defined(PR_LOGGING)
        else 
        {
            PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) isAlive return false\n", this));
        }
#endif
    }

    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) creating control\n", this));

    nsXPIDLCString host;
    rv = mURL->GetHost(getter_Copies(host));
    if (NS_FAILED(rv)) return rv;
        
    nsCOMPtr<nsITransport> transport;
    // build our own
    rv = CreateTransport(host, 
                         mPort, 
                         FTP_COMMAND_CHANNEL_SEG_SIZE, 
                         FTP_COMMAND_CHANNEL_MAX_SIZE, 
                         getter_AddRefs(transport)); // the command transport
    if (NS_FAILED(rv)) return rv;
        
    mState = FTP_READ_BUF;
    mNextState = FTP_S_USER;

    mControlConnection = new nsFtpControlConnection(transport);
    if (!mControlConnection) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(mControlConnection);
        
    // Must do it this way 'cuz the channel intercepts the progress notifications.
    (void) mControlConnection->SetStreamListener(NS_STATIC_CAST(nsIStreamListener*, this));

    return mControlConnection->Connect();
}


nsresult
nsFtpState::Process() {
    nsresult    rv = NS_OK;
    PRBool      processingRead = PR_TRUE;
    
    while (mKeepRunning && processingRead) {
        switch(mState) {

//////////////////////////////
//// INTERNAL STATES
//////////////////////////////
            case FTP_COMMAND_CONNECT:
                {
                    KillControlConnnection();
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) Establishing control connection...", this));
                    rv = EstablishControlConnection();  // sets mState
                    mInternalError = rv;
                    if (NS_FAILED(rv)) {
                        mState = FTP_ERROR;
                        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                    }
                    else
                        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    }
              break;
            case FTP_READ_BUF:
                {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) Waiting for control data (%x)...", this, rv));
                    processingRead = PR_FALSE;
                    break;
                }
                // END: FTP_READ_BUF

            case FTP_ERROR: // xx needs more work to handle dropped control connection cases
                {
                 if (mTryingCachedControl && mResponseCode == 530 &&
                     mInternalError == NS_ERROR_FTP_PASV) {
                    // The user was logged out during an pasv operation
                    // we want to restart this request with a new control
                    // channel.
                    mState = FTP_COMMAND_CONNECT;
                 }
                 else if (mResponseCode == 421 && 
                          mInternalError != NS_ERROR_FTP_LOGIN) {
                    // The command channel dropped for some reason.
                    // Fire it back up, unless we were trying to login
                    // in which case the server might just be telling us
                    // that the max number of users has been reached...
                    mState = FTP_COMMAND_CONNECT;
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FTP_ERROR - Calling StopProcessing\n", this));
                    rv = StopProcessing();
                    NS_ASSERTION(NS_SUCCEEDED(rv), "StopProcessing failed.");
                    processingRead = PR_FALSE;
                }
                break;
                }
                // END: FTP_ERROR

            case FTP_COMPLETE:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) COMPLETE\n", this));
                rv = StopProcessing();
                NS_ASSERTION(NS_SUCCEEDED(rv), "StopProcessing failed.");
                processingRead = PR_FALSE;
                break;
                }
                // END: FTP_COMPLETE

//////////////////////////////
//// CONNECTION SETUP STATES
//////////////////////////////

            case FTP_S_USER:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_USER - ", this));
                rv = S_user();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_LOGIN;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_USER;
                }
                break;
                }
                // END: FTP_S_USER

            case FTP_R_USER:
                {
                mState = R_user();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_LOGIN;
                break;
                }
                // END: FTP_R_USER

            case FTP_S_PASS:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_PASS - ", this));
                rv = S_pass();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_LOGIN;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_PASS;
                }
                break;    
                }
                // END: FTP_S_PASS

            case FTP_R_PASS:
                {
                mState = R_pass();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_LOGIN;
                break;
                }
                // END: FTP_R_PASS    

            case FTP_S_SYST:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_SYST - ", this));
                rv = S_syst();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_SYST;
                }
                break;
                }
                // END: FTP_S_SYST

            case FTP_R_SYST: 
                {
                mState = R_syst();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                break;
                }
                // END: FTP_R_SYST

            case FTP_S_ACCT:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_ACCT - ", this));
                rv = S_acct();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_LOGIN;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_ACCT;
                }
                break;
                }
                // END: FTP_S_ACCT

            case FTP_R_ACCT:
                {
                mState = R_acct();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_LOGIN;
                break;
                }
                // END: FTP_R_ACCT

            case FTP_S_PWD:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_PWD - ", this));
                rv = S_pwd();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_PWD;
                }
                break;
                }
                // END: FTP_S_PWD

            case FTP_R_PWD:
                {
                mState = R_pwd();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                break;
                }
                // END: FTP_R_PWD

            case FTP_S_MODE:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_MODE - ", this));
                rv = S_mode();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_MODE;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_MODE;
                }
                break;
                }
                // END: FTP_S_MODE

            case FTP_R_MODE:
                {
                mState = R_mode();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_MODE;
                break;
                }
                // END: FTP_R_MODE

            case FTP_S_CWD:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_CWD - ", this));
                rv = S_cwd();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_CWD;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {                
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_CWD;
                }
                break;
                }
                // END: FTP_S_CWD

            case FTP_R_CWD:
                {
                mState = R_cwd();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_CWD;
                break;
                }
                // END: FTP_R_CWD

            case FTP_S_SIZE:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_SIZE - ", this));
                rv = S_size();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_SIZE;
                }
                break;
                }
                // END: FTP_S_SIZE

            case FTP_R_SIZE: 
                {
                mState = R_size();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                break;
                }
                // END: FTP_R_SIZE

            case FTP_S_MDTM:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_MDTM - ", this));
                rv = S_mdtm();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_MDTM;
                }
                break;
                }
                // END: FTP_S_MDTM;

            case FTP_R_MDTM:
                {
                mState = R_mdtm();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                break;
                }
                // END: FTP_R_MDTM

            case FTP_S_LIST:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_LIST - ", this));
                rv = S_list();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_LIST;
                }
                break;
                }
                // END: FTP_S_LIST

            case FTP_R_LIST:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) R_LIST - ", this));
                mState = R_list();
                if (FTP_ERROR == mState) {
                    mInternalError = NS_ERROR_FAILURE;
                    mNextState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    char *lf = PL_strchr(mResponseMsg.get(), LF);
                    if (lf && lf+1 && *(lf+1)) 
                        // we have a double response
                        // need to read again...
                        mState = FTP_READ_BUF;

                    mNextState = FTP_COMPLETE;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                }
                break;
                }
                // END: FTP_R_LIST

            case FTP_S_RETR:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_RETR - ", this));
                rv = S_retr();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_RETR;
                }
                break;
                }
                // END: FTP_S_RETR

            case FTP_R_RETR:
                {
                mState = R_retr();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                mNextState = FTP_COMPLETE;
                break;
                }
                // END: FTP_R_RETR


            case FTP_S_STOR:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_STOR - ", this));
                rv = S_stor();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_STOR;
                }
                break;
                }
                // END: FTP_S_RETR

            case FTP_R_STOR:
                {
                mState = R_stor();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                mNextState = FTP_COMPLETE;
                break;
                }
                // END: FTP_R_RETR

//////////////////////////////
//// DATA CONNECTION STATES
//////////////////////////////
          case FTP_S_PASV:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_PASV - ", this));
                rv = S_pasv();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_PASV;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_PASV;
                }
                break;
                }
                // FTP: FTP_S_PASV

            case FTP_R_PASV:
                {
                mState = R_pasv();
                if (FTP_ERROR == mState) {
                    mInternalError = NS_ERROR_FTP_PASV;
                    break;
                }
                mNextState = FTP_READ_BUF;
                break;
                }
                // END: FTP_R_PASV

//////////////////////////////
//// ACTION STATES
//////////////////////////////
            case FTP_S_DEL_FILE:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_DEL_FILE - ", this));
                rv = S_del_file();
                if (NS_FAILED(rv)) {
                    mInternalError = rv;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_DEL_FILE;
                }
                break;
                }
                // END: FTP_S_DEL_FILE

            case FTP_R_DEL_FILE:
                {
                mState = R_del_file();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FAILURE;
                break;
                }
                // END: FTP_R_DEL_FILE

            case FTP_S_DEL_DIR:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_DEL_DIR - ", this));
                rv = S_del_dir();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_DEL_DIR;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_DEL_DIR;
                }
                break;
                }
                // END: FTP_S_DEL_DIR

            case FTP_R_DEL_DIR:
                {
                mState = R_del_dir();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_DEL_DIR;
                break;
                }
                // END: FTP_R_DEL_DIR

            case FTP_S_MKDIR:
                {
                PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) S_MKDIR - ", this));
                rv = S_mkdir();
                if (NS_FAILED(rv)) {
                    mInternalError = NS_ERROR_FTP_MKDIR;
                    mState = FTP_ERROR;
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) FAILED\n", this));
                } else {
                    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) SUCCEEDED\n", this));
                    mState = FTP_READ_BUF;
                    mNextState = FTP_R_MKDIR;
                }
                break;
                }
                // END: FTP_S_MKDIR

            case FTP_R_MKDIR: 
                {
                mState = R_mkdir();
                if (FTP_ERROR == mState)
                    mInternalError = NS_ERROR_FTP_MKDIR;
                break;
                }
                // END: FTP_R_MKDIR

            default:
                ;

        } // END: switch 
    } // END: while loop
    
    return rv;
}

///////////////////////////////////
// STATE METHODS
///////////////////////////////////
nsresult
nsFtpState::S_user() {
    nsresult rv;
    nsCAutoString usernameStr("USER ");

    if (mAnonymous) {
        usernameStr.Append("anonymous");
    } else {
        if (!mUsername.Length()) {
            if (!mAuthPrompter) return NS_ERROR_NOT_INITIALIZED;
            PRUnichar *user = nsnull, *passwd = nsnull;
            PRBool retval;
            nsXPIDLCString host;
            rv = mURL->GetHost(getter_Copies(host));
            if (NS_FAILED(rv)) return rv;
            nsAutoString message(NS_LITERAL_STRING("Enter username and password for ")); //TODO localize it!
            message.AppendWithConversion(host);

            nsAutoString realm; // XXX i18n
            CopyASCIItoUCS2(nsLiteralCString(NS_STATIC_CAST(const char*, host)), realm);
            rv = mAuthPrompter->PromptUsernameAndPassword(nsnull,
                                                          message.GetUnicode(),
                                                          realm.GetUnicode(), nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                                          &user, &passwd, &retval);

            // if the user canceled or didn't supply a username we want to fail
            if (!retval || (user && !*user) )
                return NS_ERROR_FAILURE;
            mUsername = user;
            mPassword = passwd;
        }
        usernameStr.AppendWithConversion(mUsername);    
    }
    usernameStr.Append(CRLF);

    return SendFTPCommand(usernameStr);
}

FTP_STATE
nsFtpState::R_user() {
    if (mResponseCode/100 == 3) {
        // send off the password
        return FTP_S_PASS;
    } else if (mResponseCode/100 == 2) {
        // no password required, we're already logged in
        return FTP_S_SYST;
    } else if (mResponseCode/100 == 5) {
        // problem logging in. typically this means the server
        // has reached it's user limit.
        return FTP_ERROR;
    } else {
        // LOGIN FAILED
        if (mAnonymous) {
            // we just tried to login anonymously and failed.
            // kick back out to S_user() and try again after
            // gathering uname/pwd info from the user.
            mAnonymous = PR_FALSE;
            return FTP_S_USER;
        } else {
            return FTP_ERROR;
        }
    }
}


nsresult
nsFtpState::S_pass() {
    nsresult rv;
    nsCAutoString passwordStr("PASS ");

    mResponseMsg = "";

    if (mAnonymous) {
        char* anonPassword;
        NS_WITH_SERVICE(nsIPref, pPref, kPrefCID, &rv); 
        if (NS_SUCCEEDED(rv) || pPref) 
            rv = pPref->CopyCharPref("network.ftp.anonymous_password", &anonPassword);
        if (NS_SUCCEEDED(rv) && anonPassword && *anonPassword != '\0') {
            passwordStr.Append(anonPassword);
            nsMemory::Free(anonPassword);
        }
        else {
            // default to 
            passwordStr.Append("mozilla@");
        }
    } else {
        if (!mPassword.Length() || mRetryPass) {
            if (!mAuthPrompter) return NS_ERROR_NOT_INITIALIZED;

            PRUnichar *passwd = nsnull;
            PRBool retval;
            nsAutoString title(NS_LITERAL_STRING("Password"));
            
            nsXPIDLCString host;
            rv = mURL->GetHost(getter_Copies(host));
            if (NS_FAILED(rv)) return rv;
            nsAutoString message(NS_LITERAL_STRING("Enter password for ")); //TODO localize it!
            message += mUsername;
            message.Append(NS_LITERAL_STRING(" on "));
            message.AppendWithConversion(host);

            nsXPIDLCString prePath;
            rv = mURL->GetPrePath(getter_Copies(prePath));
            if (NS_FAILED(rv)) return rv;
            rv = mAuthPrompter->PromptPassword(title.GetUnicode(),
                                               message.GetUnicode(),
                                               NS_ConvertASCIItoUCS2(prePath).GetUnicode(), 
                                               nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                               &passwd, &retval);

            // we want to fail if the user canceled or didn't enter a password.
            if (!retval || (passwd && !*passwd) )
                return NS_ERROR_FAILURE;
            mPassword = passwd;
        }
        passwordStr.AppendWithConversion(mPassword);    
    }
    passwordStr.Append(CRLF);

    return SendFTPCommand(passwordStr);
}

FTP_STATE
nsFtpState::R_pass() {
    if (mResponseCode/100 == 3) {
        // send account info
        return FTP_S_ACCT;
    } else if (mResponseCode/100 == 2) {
        // logged in
        return FTP_S_SYST;
    } else if (mResponseCode == 503) {
        // start over w/ the user command.
        // note: the password was successful, and it's stored in mPassword
        mRetryPass = PR_FALSE;
        return FTP_S_USER;
    } else if (mResponseCode == 530) {
        // user limit reached
        return FTP_ERROR;
    } else {
        // kick back out to S_pass() and ask the user again.
        nsresult rv = NS_OK;

        NS_WITH_SERVICE(nsIWalletService, walletService, kWalletServiceCID, &rv);
        if (NS_FAILED(rv)) return FTP_ERROR;

        nsXPIDLCString uri;
        rv = mURL->GetSpec(getter_Copies(uri));
        if (NS_FAILED(rv)) return FTP_ERROR;

        rv = walletService->SI_RemoveUser(uri, nsnull);
        if (NS_FAILED(rv)) return FTP_ERROR;

        mRetryPass = PR_TRUE;
        return FTP_S_PASS;
    }
}

nsresult
nsFtpState::S_syst() {
    nsCString systString("SYST" CRLF);
    return SendFTPCommand( systString );
}

FTP_STATE
nsFtpState::R_syst() {
    if (mResponseCode/100 == 2) {
        if (mResponseMsg.Find("UNIX") > -1) {
            mServerType = FTP_UNIX_TYPE;
            mList = PR_TRUE;
        }
        else if ( ( mResponseMsg.Find("WIN32", PR_TRUE) > -1) ||
                  ( mResponseMsg.Find("windows", PR_TRUE) > -1) )
        {
            mServerType = FTP_NT_TYPE;
            mList = PR_TRUE;
        }
        else
        {
            NS_ASSERTION(0, "Guessing FTP server type.");
            // No clue.  We will just hope it is UNIX type server.
            mServerType = FTP_UNIX_TYPE;
            mList = PR_TRUE;
        }
    }
    return FTP_S_PWD;
}

nsresult
nsFtpState::S_acct() {
    nsCString acctString("ACCT noaccount" CRLF);
    return SendFTPCommand(acctString);
}

FTP_STATE
nsFtpState::R_acct() {
    if (mResponseCode/100 == 2)
        return FTP_S_SYST;
    else
        return FTP_ERROR;
}

nsresult
nsFtpState::S_pwd() {
    nsCString pwdString("PWD" CRLF); 
    return SendFTPCommand(pwdString);
}


FTP_STATE
nsFtpState::R_pwd() {
    FTP_STATE state = FTP_ERROR;

    // check for a quoted path
    PRInt32 beginQ, endQ;
    beginQ = mResponseMsg.Find("\"");
    if (beginQ > -1) {
        endQ = mResponseMsg.RFind("\"");
        if (endQ > beginQ) {
            // quoted path
            nsCAutoString tmpPath;
            mResponseMsg.Mid(tmpPath, beginQ+1, (endQ-beginQ-1));

            nsCAutoString   modPath(mPath);
            if ((modPath.Length() > 0) && (!modPath.Equals("/")))
            {
                tmpPath = modPath;
            }

            mResponseMsg = tmpPath;
            mURL->SetPath(tmpPath);
            mPath = tmpPath;            
        }
    } 

    // default next state
    state = FindActionState();

    if(mServerType == FTP_GENERIC_TYPE) {
        if (mResponseMsg.CharAt(1) == '/') {
            mServerType = FTP_UNIX_TYPE; // path names ending with '/' imply unix
            mList = PR_TRUE;
        } 
    }

    return state;
}

nsresult
nsFtpState::S_mode() {
    char * string;

    // we're connected figure out what type of transfer we're doing (ascii or binary)
    nsXPIDLCString type;

    nsresult rv = mChannel->GetContentType(getter_Copies(type));
    nsCAutoString typeStr;
    if (NS_FAILED(rv) || !type) 
        typeStr = "bin";
    else
        typeStr.Assign(type);

    PRInt32 textType = typeStr.Find("text");

    if ( textType != 0 )
        string = "TYPE I" CRLF;
    else
        string = "TYPE A" CRLF;

    nsCString typeString(string);

    return SendFTPCommand(typeString);
}

FTP_STATE
nsFtpState::R_mode() {
    if (mResponseCode/100 != 2) {
        return FTP_ERROR;
    }

    if (mAction == PUT) {
        return FTP_S_CWD;
    } else {
        return FTP_S_SIZE;
    }
}

nsresult
nsFtpState::S_cwd() {
    nsCAutoString cwdStr("CWD ");
    cwdStr.Append(mPath);
    cwdStr.Append(CRLF);

    cwdStr.Mid(mCwdAttempt, 4, cwdStr.Length() - 6);

    return SendFTPCommand(cwdStr);
}

FTP_STATE
nsFtpState::R_cwd() {
    FTP_STATE state = FTP_ERROR;
    if (mResponseCode/100 == 2) {
        mCwd = mCwdAttempt;

        // update
        mURL->SetPath(mCwd);
        nsresult rv;
        
        if (mGenerateHTMLContent)
            rv = mChannel->SetContentType("text/html");
        else
            rv = mChannel->SetContentType("application/http-index-format");
        
        if (NS_FAILED(rv)) return FTP_ERROR;

        // success
        if (mAction == PUT) {
            // we are uploading
            state = FTP_S_STOR;
        } else {
            // we are GETting a file or dir
            state = FTP_S_LIST;
        }
    } else {
        state = FTP_S_RETR;
    }
    return state;
}

nsresult
nsFtpState::S_size() {
    nsCAutoString sizeBuf("SIZE ");
    sizeBuf.Append(mPath);
    sizeBuf.Append(CRLF);

    return SendFTPCommand(sizeBuf);
}

FTP_STATE
nsFtpState::R_size() {
    if (mResponseCode/100 == 2) {
        PRInt32 conversionError;
        PRInt32 length = mResponseMsg.ToInteger(&conversionError);
        if (NS_FAILED(mChannel->SetContentLength(length))) return FTP_ERROR;
    }

    return FTP_S_MDTM;
}

nsresult
nsFtpState::S_mdtm() {
    nsCAutoString mdtmStr("MDTM ");
    mdtmStr.Append(mPath);
    mdtmStr.Append(CRLF);

    return SendFTPCommand(mdtmStr);
}

FTP_STATE
nsFtpState::R_mdtm() {
    if (mResponseCode/100 == 2) {
         // The time is returned in ISO 3307 "Representation
         // of the Time of Day" format. This format is YYYYMMDDHHmmSS or
         // YYYYMMDDHHmmSS.xxx, where
         //         YYYY    is the year
         //         MM      is the month (01-12)
         //         DD      is the day of the month (01-31)
         //         HH      is the hour of the day (00-23)
         //         mm      is the minute of the hour (00-59)
         //         SS      is the second of the hour (00-59)
         //         xxx     if present, is a fractional second and may be any length
         // Time is expressed in UTC (GMT), not local time.
        PRExplodedTime ts;
        char *timeStr = mResponseMsg.ToNewCString();
        if (!timeStr) return FTP_ERROR;

        ts.tm_year =
         (timeStr[0] - '0') * 1000 +
         (timeStr[1] - '0') *  100 +
         (timeStr[2] - '0') *   10 +
         (timeStr[3] - '0');

        ts.tm_month = ((timeStr[4] - '0') * 10 + (timeStr[5] - '0')) - 1;
        ts.tm_mday = (timeStr[6] - '0') * 10 + (timeStr[7] - '0');
        ts.tm_hour = (timeStr[8] - '0') * 10 + (timeStr[9] - '0');
        ts.tm_min  = (timeStr[10] - '0') * 10 + (timeStr[11] - '0');
        ts.tm_sec  = (timeStr[12] - '0') * 10 + (timeStr[13] - '0');
        ts.tm_usec = 0;
        nsMemory::Free(timeStr);
        mLastModified = PR_ImplodeTime(&ts);
    }

    return FTP_S_CWD;
}

nsresult
nsFtpState::S_list() {
    nsresult rv;

    if (!mDRequestForwarder) 
        return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIStreamListener> converter;
    
    rv = BuildStreamConverter(getter_AddRefs(converter));
    if (NS_FAILED(rv)) return rv;

    mDRequestForwarder->SetStreamListener(converter);

#ifdef DOUGT_NEW_CACHE
    if (mCacheEntry && mReadingFromCache)
    {            
        if (mGenerateHTMLContent)
            rv = mChannel->SetContentType("text/html");
        else
            rv = mChannel->SetContentType(APPLICATION_HTTP_INDEX_FORMAT);
        
        nsCOMPtr<nsITransport> transport;
        rv = mCacheEntry->GetTransport(getter_AddRefs(transport));
        if (NS_FAILED(rv)) {
            NS_ASSERTION(0, "Could not create cache transport.");
            return rv;
        }
        rv = transport->AsyncRead(forwarder, nsnull, 0, PRUint32(-1), 0, getter_AddRefs(mDPipeRequest));
        return rv;
    }

    (void) forwarder->SetCacheEntryDescriptor(mCacheEntry);
#endif

    char * string;
    if ( mList )
        string = "LIST" CRLF;
    else
        string = "NLST" CRLF;
    
    nsCString listString(string);

    return SendFTPCommand(listString);
}

FTP_STATE
nsFtpState::R_list() {
    if ((mResponseCode/100 == 4) || (mResponseCode/100 == 5)) {
        mFireCallbacks = PR_TRUE;
        return FTP_ERROR;
    }
    return FTP_READ_BUF;
}

nsresult
nsFtpState::S_retr() {
    nsresult rv = NS_OK;
    nsCAutoString retrStr("RETR ");
    retrStr.Append(mPath);
    retrStr.Append(CRLF);
    
    if (!mDRequestForwarder)
        return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIStreamListener> streamListener = do_QueryInterface(mChannel);
    mDRequestForwarder->SetStreamListener(streamListener);
    rv = SendFTPCommand(retrStr);
    return rv;
}

FTP_STATE
nsFtpState::R_retr() {
    if (mResponseCode/100 == 1) {
        // see if there's another response in this read.
        // this can happen if the server sends back two
        // responses before we've processed the first one.
        PRInt32 loc = -1;
        loc = mResponseMsg.FindChar(LF);
        if (loc > -1) {
            PRInt32 err;
            nsCAutoString response;
            mResponseMsg.Mid(response, loc, mResponseMsg.Length() - (loc+1));
            if (response.Length()) {
                PRInt32 code = response.ToInteger(&err);
                if (code/100 != 2)
                    return FTP_ERROR;
                else
                    return FTP_COMPLETE;
            }
        }
        return FTP_READ_BUF;
    }
    mFireCallbacks = PR_TRUE;
    return FTP_ERROR;
}


nsresult
nsFtpState::S_stor() {
    nsresult rv = NS_OK;
    nsCAutoString retrStr("STOR ");
    retrStr.Append(mPath);
    retrStr.Append(CRLF);

    rv = SendFTPCommand(retrStr);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mWriteStream, "we're trying to upload without any data");
    
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIRequestObserver> observer = do_QueryInterface(mChannel, &rv);
    if (NS_FAILED(rv)) return rv;

    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) writing on Data Transport\n", this));
    return NS_AsyncWriteFromStream(getter_AddRefs(mDPipeRequest), mDPipe, mWriteStream,
                                   0, mWriteCount, 0, observer, nsnull);
}

FTP_STATE
nsFtpState::R_stor() {
    if (mResponseCode/100 == 1) {
        return FTP_READ_BUF;
    }
    return FTP_ERROR;
}


nsresult
nsFtpState::S_pasv() {
    nsresult rv;

    if (mIPv6Checked == PR_FALSE) {
        // Find IPv6 socket address, if server is IPv6
        mIPv6Checked = PR_TRUE;
        PR_ASSERT(mIPv6ServerAddress == 0);
        nsCOMPtr<nsITransport> controlSocket;
        mControlConnection->GetTransport(getter_AddRefs(controlSocket));
        if (!controlSocket) return FTP_ERROR;
        nsCOMPtr<nsISocketTransport> sTrans = do_QueryInterface(controlSocket, &rv);
        
        if (sTrans) {
            rv = sTrans->GetIPStr(100, &mIPv6ServerAddress);
        }

        if (NS_SUCCEEDED(rv)) {
            PRNetAddr addr;
            if (PR_StringToNetAddr(mIPv6ServerAddress, &addr) != PR_SUCCESS ||
                PR_IsNetAddrType(&addr, PR_IpAddrV4Mapped)) {
                nsMemory::Free(mIPv6ServerAddress);
                mIPv6ServerAddress = 0;
            }
            PR_ASSERT(!mIPv6ServerAddress || addr.raw.family == PR_AF_INET6);
        }
    }


    char * string;
    if (mIPv6ServerAddress)
        string = "EPSV" CRLF;
    else
        string = "PASV" CRLF;

    nsCString pasvString(string);
    return SendFTPCommand(pasvString);
    
}

FTP_STATE
nsFtpState::R_pasv() {
    nsresult rv;
    PRInt32 port;

    if (mResponseCode/100 != 2)  {
        return FTP_ERROR;
    }

    char *response = mResponseMsg.ToNewCString();
    if (!response) return FTP_ERROR;
    char *ptr = response;

    nsCAutoString host;
    if (mIPv6ServerAddress) {
        // The returned string is of the form
        // text (|||ppp|)
        // Where '|' can be any single character
        char delim;
        while (*ptr && *ptr != '(') ptr++;
        if (*ptr++ != '(') {
            return FTP_ERROR;
        }
        delim = *ptr++;
        if (!delim || *ptr++ != delim || *ptr++ != delim || 
            *ptr < '0' || *ptr > '9') {
            return FTP_ERROR;
        }
        port = 0;
        do {
            port = port * 10 + *ptr++ - '0';
        } while (*ptr >= '0' && *ptr <= '9');
        if (*ptr++ != delim || *ptr != ')') {
            return FTP_ERROR;
        }
    } else {
        // The returned address string can be of the form
        // (xxx,xxx,xxx,xxx,ppp,ppp) or
        //  xxx,xxx,xxx,xxx,ppp,ppp (without parens)
        PRInt32 h0, h1, h2, h3, p0, p1;
        while (*ptr) {
            if (*ptr == '(') {
                // move just beyond the open paren
                ptr++;
                break;
            }
            if (*ptr == ',') {
                // backup to the start of the digits
                do {
                    ptr--;
                } while ((ptr >=response) && (*ptr >= '0') && (*ptr <= '9'));
                ptr++; // get back onto the numbers
                break;
            }
            ptr++;
        } // end while

        PRInt32 fields = PR_sscanf(ptr, 
            "%ld,%ld,%ld,%ld,%ld,%ld",
            &h0, &h1, &h2, &h3, &p0, &p1);

        if (fields < 6) {
            return FTP_ERROR;
        }

        port = ((PRInt32) (p0<<8)) + p1;
        host.AppendInt(h0);
        host.Append('.');
        host.AppendInt(h1);
        host.Append('.');
        host.AppendInt(h2);
        host.Append('.');
        host.AppendInt(h3);
    }

    nsMemory::Free(response);

    const char* hostStr = mIPv6ServerAddress ? mIPv6ServerAddress : host.get();

    // now we know where to connect our data channel
    rv = CreateTransport(hostStr, 
                         port, 
                         FTP_DATA_CHANNEL_SEG_SIZE, 
                         FTP_DATA_CHANNEL_MAX_SIZE, 
                         getter_AddRefs(mDPipe)); // the data channel
    if (NS_FAILED(rv)) return FTP_ERROR;

    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) Created Data Transport (%s:%x)\n", this, hostStr, port));

    nsCOMPtr<nsISocketTransport> sTrans = do_QueryInterface(mDPipe, &rv);
    if (NS_FAILED(rv)) return FTP_ERROR;

    if (NS_FAILED(sTrans->SetReuseConnection(PR_FALSE))) return FTP_ERROR;

    // hook ourself up as a proxy for progress notifications
    nsCOMPtr<nsIInterfaceRequestor> callbacks = do_QueryInterface(mChannel);
    mDPipe->SetNotificationCallbacks(callbacks, PR_FALSE);

    mDRequestForwarder = new DataRequestForwarder;
    if (!mDRequestForwarder) return FTP_ERROR;
    NS_ADDREF(mDRequestForwarder);

    rv = mDRequestForwarder->Init(mChannel);
    if (NS_FAILED(rv)){
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) forwarder->Init failed (rv=%x)\n", this, rv));
        return FTP_ERROR;
    }
    mWaitingForDConn = PR_TRUE;
    rv = mDPipe->AsyncRead(mDRequestForwarder, nsnull, 0, PRUint32(-1), 0, getter_AddRefs(mDPipeRequest));    
    
    return FTP_S_MODE;
}

nsresult
nsFtpState::S_del_file() {
    nsCAutoString delStr("DELE ");
    delStr.Append(mPath);
    delStr.Append(CRLF);

    return SendFTPCommand(delStr);
}

FTP_STATE
nsFtpState::R_del_file() {
    if (mResponseCode/100 != 2)
        return FTP_S_DEL_DIR;
    return FTP_COMPLETE;
}

nsresult
nsFtpState::S_del_dir() {
    nsCAutoString delDirStr("RMD ");
    delDirStr.Append(mPath);
    delDirStr.Append(CRLF);

    return SendFTPCommand(delDirStr);
}

FTP_STATE
nsFtpState::R_del_dir() {
    if (mResponseCode/100 != 2)
        return FTP_ERROR;
    return FTP_COMPLETE;
}

nsresult
nsFtpState::S_mkdir() {
    nsCAutoString mkdirStr("MKD ");
    mkdirStr.Append(mPath);
    mkdirStr.Append(CRLF);

    return SendFTPCommand(mkdirStr);
}

FTP_STATE
nsFtpState::R_mkdir() {
    if (mResponseCode/100 != 2)
        return FTP_ERROR;
    return FTP_COMPLETE;
}

///////////////////////////////////
// END: STATE METHODS
///////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

NS_IMETHODIMP
nsFtpState::GetName(PRUnichar* *result)
{
    nsresult rv;
    nsXPIDLCString urlStr;
    rv = mURL->GetSpec(getter_Copies(urlStr));
    if (NS_FAILED(rv)) return rv;
    nsString name;
    name.AppendWithConversion(urlStr);
    *result = name.ToNewUnicode();
    return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsFtpState::IsPending(PRBool *result)
{
    nsresult rv = NS_OK;
    *result = PR_FALSE;
    
    nsCOMPtr<nsIRequest> request;
    mControlConnection->GetReadRequest(getter_AddRefs(request));

    if (request) {
        rv = request->IsPending(result);
        if (NS_FAILED(rv)) return rv;
    }
    return rv;
}

NS_IMETHODIMP
nsFtpState::GetStatus(nsresult *status)
{
    *status = mInternalError;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::Cancel(nsresult status)
{
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) nsFtpState::Cancel() rv=%x\n", this, status));

    // we should try to recover the control connection....
    if (NS_SUCCEEDED(mControlStatus))
        mControlStatus = status;

    // kill the data connection immediately. 
    NS_IF_RELEASE(mDRequestForwarder);
    if (mDPipeRequest) {
        mDPipeRequest->Cancel(status);
        mDPipeRequest = 0;
    }
#ifdef DOUGT_NEW_CACHE
    if (mCacheEntry)
        mCacheEntry->Doom();
#endif

    (void) StopProcessing();   
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::Suspend(void)
{
    nsresult rv = NS_OK;
    
    // suspending the underlying socket transport will
    // cause the FTP state machine to "suspend" when it
    // tries to use the transport. May not be granular 
    // enough.
    if (mSuspendCount < 1) {
        mSuspendCount++;

        // only worry about the read request.
        nsCOMPtr<nsIRequest> request;
        mControlConnection->GetReadRequest(getter_AddRefs(request));

        if (request) {
            rv = request->Suspend();
            if (NS_FAILED(rv)) return rv;
        }

        if (mDPipeRequest) {
            rv = mDPipeRequest->Suspend();
            if (NS_FAILED(rv)) return rv;
        }
    }

    return rv;
}

NS_IMETHODIMP
nsFtpState::Resume(void)
{
    nsresult rv = NS_ERROR_FAILURE;
    
    // resuming the underlying socket transports will
    // cause the FTP state machine to unblock and 
    // go on about it's business.
    if (mSuspendCount) {
        // only worry about the read request.
        nsCOMPtr<nsIRequest> request;
        mControlConnection->GetReadRequest(getter_AddRefs(request));

        if (request) {
            rv = request->Resume();
            if (NS_FAILED(rv)) return rv;
        }
        if (mDPipeRequest) {
            rv = mDPipeRequest->Resume();
            if (NS_FAILED(rv)) return rv;
        }
        rv = NS_OK;
    }
    mSuspendCount--;
    return rv;
}

NS_IMETHODIMP
nsFtpState::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    *aLoadGroup = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFtpState::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = nsIRequest::LOAD_NORMAL;
    return NS_OK;
}

NS_IMETHODIMP
nsFtpState::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFtpState::Init(nsIFTPChannel* aChannel,
                 nsIPrompt*  aPrompter,
                 nsIAuthPrompt* aAuthPrompter,
                 nsIFTPEventSink* sink) 
{
    nsresult rv = NS_OK;

    mKeepRunning = PR_TRUE;
    mPrompter = aPrompter;
    mFTPEventSink = sink;
    mAuthPrompter = aAuthPrompter;

    // parameter validation
    NS_ASSERTION(aChannel, "FTP: needs a channel");

    mChannel = aChannel; // a straight com ptr to the channel

    rv = aChannel->GetURI(getter_AddRefs(mURL));
    if (NS_FAILED(rv)) return rv;

    char *path = nsnull;
    rv = mURL->GetPath(&path);
    if (NS_FAILED(rv)) return rv;
    mPath = nsUnescape(path);
    nsMemory::Free(path);

    // pull any username and/or password out of the uri
    nsXPIDLCString uname;
    rv = mURL->GetUsername(getter_Copies(uname));
    if (NS_FAILED(rv)) {
        return rv;
    } else {
        if ((const char*)uname && *(const char*)uname) {
            mAnonymous = PR_FALSE;
            mUsername.AssignWithConversion(uname);
        }
    }

    nsXPIDLCString password;
    rv = mURL->GetPassword(getter_Copies(password));
    if (NS_FAILED(rv))
        return rv;
    else
        mPassword.AssignWithConversion(password);
    
    // setup the connection cache key
    nsXPIDLCString host;
    rv = mURL->GetHost(getter_Copies(host));
    if (NS_FAILED(rv)) return rv;

    PRInt32 port;
    rv = mURL->GetPort(&port);
    if (NS_FAILED(rv)) return rv;

    if (port > 0)
        mPort = port;

#ifdef DOUGT_NEW_CACHE
    // if there is no cache, it is NOT an error.

    PRBool useCache = PR_TRUE;
    NS_WITH_SERVICE(nsIPref, pPref, kPrefCID, &rv); 
    if (NS_SUCCEEDED(rv) || pPref) 
        pPref->GetBoolPref("network.ftp.useCache", &useCache);

    if (useCache) {
        static NS_DEFINE_CID(kCacheServiceCID, NS_CACHESERVICE_CID);
        NS_WITH_SERVICE(nsICacheService, serv, kCacheServiceCID, &rv);
        if (NS_SUCCEEDED(rv)) {
            (void) serv->CreateSession("FTP Directory Listings",
                                       nsICache::STORE_IN_MEMORY,
                                       PR_TRUE,
                                       getter_AddRefs(mCacheSession));
        }
    }
#endif

    return NS_OK;
}

nsresult
nsFtpState::Connect()
{
    nsresult rv;

#ifdef DOUGT_NEW_CACHE
    //FIX: SYNC call.  Should make this async!!!
    if (mCacheSession) {

        nsXPIDLCString urlStr;
        rv = mURL->GetSpec(getter_Copies(urlStr));
        if (NS_FAILED(rv)) return rv;

        rv = mCacheSession->OpenCacheEntry(urlStr,
                                           nsICache::ACCESS_READ_WRITE,
                                           getter_AddRefs(mCacheEntry));
        if (NS_SUCCEEDED(rv)) {

            // Decide if we should write over this cache entry or use it...
    
            nsCacheAccessMode accessMode;
            mCacheEntry->GetAccessGranted(&accessMode);
        
            if (accessMode & nsICache::ACCESS_READ) {
                // we can read from the cache.
                PRUint32 aLoadFlags;
                mChannel->GetLoadFlags(&aLoadFlags);

                if (accessMode & nsICache::ACCESS_WRITE) {
                    // we said that we could validate, so here we do it.
                    mCacheEntry->MarkValid();
                }
                // we have a directory listing in our cache.  lets kick off a load
                mReadingFromCache = PR_TRUE;
                rv = S_list();
                if (NS_SUCCEEDED(rv))
                    return rv;
            }
        } 
    }
#endif

    mState = FTP_COMMAND_CONNECT;
    mNextState = FTP_S_USER;

    rv = Process();

    // check for errors.
    if (NS_FAILED(rv)) {
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("-- Connect() on Control Connect FAILED with rv = %x\n", rv));
        mInternalError = NS_ERROR_FAILURE;
        mState = FTP_ERROR;
    }

    return rv;
}
nsresult
nsFtpState::SetWriteStream(nsIInputStream* aInStream, PRUint32 aWriteCount) {
    mAction = PUT;
    mWriteStream = aInStream;
    mWriteCount  = aWriteCount; // The is the amount of data to write.
    return NS_OK;
}

void
nsFtpState::KillControlConnnection() {
    mControlReadContinue = PR_FALSE;
    mControlReadBrokenLine = PR_FALSE;
    mControlReadCarryOverBuf.Truncate(0);

#ifdef FTP_SIMULATE_DROPPED_CONTROL_CONNECTION        
        // hack to simulate dropped control connection
        if (mCPipe)
            mCPipe->Cancel(NS_BINDING_ABORTED);
#endif

    if (mDPipe) {
        mDPipe->SetNotificationCallbacks(nsnull, PR_FALSE);
        mDPipe = 0;
    }

    mIPv6Checked = PR_FALSE;
    if (mIPv6ServerAddress) {
        nsMemory::Free(mIPv6ServerAddress);
        mIPv6ServerAddress = 0;
    }
    // if everything went okay, save the connection. 
    // FIX: need a better way to determine if we can cache the connections.
    //      there are some errors which do not mean that we need to kill the connection
    //      e.g. fnf.

    if (!mControlConnection)
        return;

    // kill the reference to ourselves in the control connection.
    (void) mControlConnection->SetStreamListener(nsnull);

    if (FTP_CACHE_CONTROL_CONNECTION && 
        NS_SUCCEEDED(mInternalError) &&
        NS_SUCCEEDED(mControlStatus) &&
        mControlConnection->IsAlive()) {

        PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("(%x) nsFtpState caching control connection", this));

        // Store connection persistant data
        mControlConnection->mServerType = mServerType;           
        mControlConnection->mCwd        = mCwd;                  
        mControlConnection->mList       = mList;                 
        mControlConnection->mPassword   = mPassword;
        nsresult rv = nsFtpProtocolHandler::InsertConnection(mURL, 
                                           NS_STATIC_CAST(nsISupports*, (nsIStreamListener*)mControlConnection));
        // Can't cache it?  Kill it then.  
        if (NS_FAILED(rv))
            mControlConnection->Disconnect();
    } 
    else
        mControlConnection->Disconnect();

    NS_RELEASE(mControlConnection);
 
    return;
}

nsresult
nsFtpState::StopProcessing() {
    nsresult rv;
    PR_LOG(gFTPLog, PR_LOG_ALWAYS, ("(%x) nsFtpState stopping", this));

#ifdef DEBUG_dougt
    printf("FTP Stopped: [response code %d] [response msg follows:]\n%s\n", mResponseCode, mResponseMsg.get());
#endif
    
    // Lets mask the FTP error code so that a client does not have to parse it
    nsresult broadcastErrorCode = NS_BINDING_ABORTED;    

    // did the protocol fail?
    if ( NS_FAILED(mInternalError) ) 
    {
        // check to see if the control status is bad.
        // web shell wont throw an alert.  we better:
        nsAutoString    text;
        text.AssignWithConversion(mResponseMsg);
        
        NS_ASSERTION(mPrompter, "no prompter!");
        if (mPrompter)
            (void) mPrompter->Alert(nsnull, text.GetUnicode());
   }

    if ( NS_FAILED(mControlStatus) ) {
        broadcastErrorCode = mControlStatus;
    }

    if (mFireCallbacks && mChannel) {

        nsCOMPtr<nsIStreamListener> channelListener = do_QueryInterface(mChannel);
        nsCOMPtr<nsIRequest> channelRequest = do_QueryInterface(mChannel);
            
        nsCOMPtr<nsIStreamListener> asyncListener;
        rv = NS_NewAsyncStreamListener(getter_AddRefs(asyncListener), channelListener, NS_UI_THREAD_EVENTQ);
        if(NS_FAILED(rv)) return rv;

        //(void) asyncListener->OnStartRequest(channelRequest, nsnull);
        (void) asyncListener->OnStopRequest(channelRequest, nsnull, broadcastErrorCode);
    }
    
    // Clean up the event loop
    mKeepRunning = PR_FALSE;

    KillControlConnnection();

    // Release the Observers
    mWriteStream = 0;

    mPrompter = 0;
    mAuthPrompter = 0;
    mChannel = 0;

#ifdef DOUGT_NEW_CACHE
    mCacheEntry = 0;
#endif

    return NS_OK;
}

FTP_STATE
nsFtpState::FindActionState(void) {

    // These operations require the separate data channel.
    if (mAction == GET || mAction == PUT) {
        // we're doing an operation that requies the data channel.
        // figure out what kind of data channel we want to setup,
        // and do it.
        return FTP_S_PASV;
    }

    // These operations use the command channel response as the
    // data to return to the user.
    if (mAction == DEL)
        return FTP_S_DEL_FILE; // we assume it's a file

    if (mAction == MKDIR)
        return FTP_S_MKDIR;

    return FTP_ERROR;
}

void
nsFtpState::SetDirMIMEType(nsString& aString) {
    // the from content type is a string of the form
    // "text/ftp-dir-SERVER_TYPE" where SERVER_TYPE represents the server we're talking to.
    switch (mServerType) {
    case FTP_UNIX_TYPE:
        aString.Append(NS_LITERAL_STRING("unix"));
        break;
    case FTP_NT_TYPE:
        aString.Append(NS_LITERAL_STRING("nt"));
        break;
    default:
        aString.Append(NS_LITERAL_STRING("generic"));
    }
}

nsresult
nsFtpState::BuildStreamConverter(nsIStreamListener** convertStreamListener)
{
    nsresult rv;
    // setup a listener to push the data into. This listener sits inbetween the
    // unconverted data of fromType, and the final listener in the chain (in this case
    // the mListener).
    nsCOMPtr<nsIStreamListener> converterListener;
    nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(mChannel);

    NS_WITH_SERVICE(nsIStreamConverterService, 
                    scs, 
                    kStreamConverterServiceCID, 
                    &rv);

    if (NS_FAILED(rv)) 
        return rv;

    nsAutoString fromStr(NS_LITERAL_STRING("text/ftp-dir-"));
    SetDirMIMEType(fromStr);

#ifdef DOUGT_NEW_CACHE
    if ( mCacheEntry ) {
        if (!mReadingFromCache) {
            nsCString aString; aString.AssignWithConversion(fromStr);
            mCacheEntry->SetMetaDataElement((const char*)"MimeType", (const char*) aString);
        } else {
            nsXPIDLCString aString;
            mCacheEntry->GetMetaDataElement((const char*)"MimeType", getter_Copies(aString));
            fromStr.AssignWithConversion(aString);
        }
    } 
#endif

    if (mGenerateHTMLContent) {
        rv = scs->AsyncConvertData(fromStr.GetUnicode(), 
                                   NS_LITERAL_STRING("text/html").get(),
                                   listener, 
                                   mURL, 
                                   getter_AddRefs(converterListener));
    } 
    else 
    {
        rv = scs->AsyncConvertData(fromStr.GetUnicode(), 
                                   NS_LITERAL_STRING(APPLICATION_HTTP_INDEX_FORMAT).get(),
                                   listener, 
                                   mURL, 
                                   getter_AddRefs(converterListener));
    }

    if (NS_FAILED(rv)) {
        PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) scs->AsyncConvertData failed (rv=%x)\n", this, rv));
        return rv;
    }

    NS_ADDREF(*convertStreamListener = converterListener);
    return rv;
}

nsresult
nsFtpState::DataConnectionEstablished()
{
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) Data Connection established.", this));
    
    mFireCallbacks  = PR_FALSE; // observer callbacks will be handled by the transport.
    mWaitingForDConn= PR_FALSE;

    // sending empty string with (mWaitingForDConn == PR_FALSE) will cause the 
    // control socket to write out its buffer.
    nsCString a("");
    SendFTPCommand(a);
    
    return NS_OK;
}
nsresult
nsFtpState::CreateTransport(const char * host, PRInt32 port, PRUint32 bufferSegmentSize, PRUint32 bufferMaxSize, nsITransport** o_pTrans)
{
    nsresult rv;
    
    NS_WITH_SERVICE(nsISocketTransportService, 
                    sts,
                    kSocketTransportServiceCID,
                    &rv);
    
    if (NS_FAILED(rv)) return rv;
    
    PRBool usingProxy;
    if (NS_SUCCEEDED(mChannel->GetUsingTransparentProxy(&usingProxy)) && usingProxy) {
        
        nsCOMPtr<nsIProxy> channelProxy = do_QueryInterface(mChannel, &rv);
        
        if (NS_SUCCEEDED(rv)) {
            
            nsXPIDLCString proxyHost;
            nsXPIDLCString proxyType;
            PRInt32 proxyPort;
            
            rv = channelProxy->GetProxyHost(getter_Copies(proxyHost));
            if (NS_FAILED(rv)) return rv;
            
            rv = channelProxy->GetProxyPort(&proxyPort);
            if (NS_FAILED(rv)) return rv;

            rv = channelProxy->GetProxyType(getter_Copies(proxyType));
            if (NS_SUCCEEDED(rv) && nsCRT::strcasecmp(proxyType, "socks") == 0) {
                
                return sts->CreateTransportOfType("socks", host, port, proxyHost, proxyPort,
                                                  bufferSegmentSize,
                                                  bufferMaxSize, 
                                                  o_pTrans);
                
            }
                
            return sts->CreateTransport(host, port, proxyHost, proxyPort,
                                        bufferSegmentSize,
                                        bufferMaxSize,
                                        o_pTrans);
                
        }
    }
    
    return sts->CreateTransport(host, port, nsnull, PRUint32(-1),
                                bufferSegmentSize,
                                bufferMaxSize,
                                o_pTrans);
}

nsresult 
nsFtpState::SendFTPCommand(nsCString& command)
{
    NS_ASSERTION(mControlConnection, "null control connection");

#if defined(PR_LOGGING)
    PR_LOG(gFTPLog, PR_LOG_DEBUG, ("(%x) Writing \"%s\"\n", this, command.get()));
#if 0 // #ifdef DEBUG_dougt
    printf("!!! %s\n", command.get());
#endif
#endif

    if (mFTPEventSink)
        mFTPEventSink->OnFTPControlLog(PR_FALSE, command.get());

    if (mControlConnection) {
        return mControlConnection->Write(command, mWaitingForDConn);
    }
    return NS_ERROR_FAILURE;
}

