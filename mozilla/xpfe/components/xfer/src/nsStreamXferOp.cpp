/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
#include "nsStreamXferOp.h"
#include "nsIStreamTransfer.h"

// Basic dependencies.
#include "nsIServiceManager.h"

// For notifying observer.
#include "nsIObserver.h"

// For opening dialog.
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "jsapi.h"

// For opening input/output streams.
#include "nsIFileTransportService.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"
#include "nsILocalFile.h"

#include "prprf.h"

#ifdef NS_DEBUG
#define DEBUG_PRINTF PR_fprintf
#else
#define DEBUG_PRINTF (void)
#endif

NS_IMPL_ISUPPORTS4(nsStreamXferOp, nsIStreamObserver, nsIStreamTransferOperation, nsIProgressEventSink, nsIInterfaceRequestor);

// ctor - save arguments in data members.
nsStreamXferOp::nsStreamXferOp( nsIChannel *source, nsIFileSpec *target ) 
    : mInputChannel( source ),
      mOutputChannel( 0 ),
      mOutputStream( 0 ),
      mOutputSpec( target ),
      mObserver( 0 ),
      mContentLength( 0 ),
      mBytesProcessed( 0 ) {
    // Properly initialize refcnt.
    NS_INIT_REFCNT();
}

// dtor
nsStreamXferOp::~nsStreamXferOp() {
    // Delete dynamically allocated members (file and buffer).
#ifdef DEBUG_law
    DEBUG_PRINTF( PR_STDOUT, "nsStreamXferOp destructor called\n" );
#endif
}

// Invoke nsIDOMWindow::OpenDialog, passing this object as argument.
NS_IMETHODIMP
nsStreamXferOp::OpenDialog( nsIDOMWindow *parent ) {
    nsresult rv = NS_OK;
    // Get JS context from parent window.
    nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface( parent, &rv );
    if ( NS_SUCCEEDED( rv ) && sgo ) {
        nsCOMPtr<nsIScriptContext> context;
        sgo->GetContext( getter_AddRefs( context ) );
        if ( context ) {
            JSContext *jsContext = (JSContext*)context->GetNativeContext();
            if ( jsContext ) {
                void *stackPtr;
                jsval *argv = JS_PushArguments( jsContext,
                                                &stackPtr,
                                                "sss%ip",
                                                "chrome://global/content/downloadProgress.xul",
                                                "_blank",
                                                "chrome",
                                                (const nsIID*)(&NS_GET_IID(nsIStreamTransferOperation)),
                                                (nsISupports*)(nsIStreamTransferOperation*)this );
                if ( argv ) {
                    nsCOMPtr<nsIDOMWindow> newWindow;
                    rv = parent->OpenDialog( jsContext, argv, 4, getter_AddRefs( newWindow ) );
                    if ( NS_FAILED( rv ) ) {
                        DEBUG_PRINTF( PR_STDOUT, "%s %d: nsIDOMWindow::OpenDialog failed, rv=0x%08X\n",
                                      (char*)__FILE__, (int)__LINE__, (int)rv );
                    }
                    JS_PopArguments( jsContext, stackPtr );
                } else {
                    DEBUG_PRINTF( PR_STDOUT, "%s %d: JS_PushArguments failed\n",
                                  (char*)__FILE__, (int)__LINE__ );
                    rv = NS_ERROR_FAILURE;
                }
            } else {
                DEBUG_PRINTF( PR_STDOUT, "%s %d: GetNativeContext failed\n",
                              (char*)__FILE__, (int)__LINE__ );
                rv = NS_ERROR_FAILURE;
            }
        } else {
            DEBUG_PRINTF( PR_STDOUT, "%s %d: GetContext failed\n",
                          (char*)__FILE__, (int)__LINE__ );
            rv = NS_ERROR_FAILURE;
        }
    } else {
        DEBUG_PRINTF( PR_STDOUT, "%s %d: QueryInterface (for nsIScriptGlobalObject) failed, rv=0x%08X\n",
                      (char*)__FILE__, (int)__LINE__, (int)rv );
    }
    return rv;
}

// Notify observer of error.
NS_IMETHODIMP
nsStreamXferOp::OnError( int operation, nsresult errorCode ) {
    nsresult rv = NS_OK;

#ifdef DEBUG_law
    DEBUG_PRINTF( PR_STDOUT, "nsStreamXferOp::OnError; op=%d, rv=0x%08X\n",
                  operation, (int)errorCode );
#endif

    if ( mObserver ) {
        char buf[32];
        PR_snprintf( buf, sizeof( buf ), "%d %X", operation, (int)errorCode );
        rv = mObserver->Observe( (nsIStreamTransferOperation*)this,
                                 nsAutoString( NS_ISTREAMTRANSFER_PROGID ";onError" ).GetUnicode(),
                                 nsAutoString( buf ).GetUnicode() );
        if ( NS_FAILED( rv ) ) {
            DEBUG_PRINTF( PR_STDOUT, "%s %d: Observe failed, rv=0x%08X\n",
                          (char*)__FILE__, (int)__LINE__, (int)rv );
        }
    }

    return rv;
};

// Start the download by opening the output file and then reading the input channel.
NS_IMETHODIMP
nsStreamXferOp::Start( void ) {
    nsresult rv = NS_OK;

    if ( mInputChannel ) {
        if ( !mOutputChannel ) {
            // First, get file transport service.
            NS_DEFINE_IID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);
            NS_WITH_SERVICE( nsIFileTransportService, fts, kFileTransportServiceCID, &rv );
    
            if ( NS_SUCCEEDED( rv ) ) {
                // Next, create output file channel.
                nsFileSpec target;      // XXX eliminate
                mOutputSpec->GetFileSpec( &target );
                nsCOMPtr<nsILocalFile> file;
                rv = NS_NewLocalFile(target, getter_AddRefs(file));
                if (NS_SUCCEEDED(rv)) {
                    // create the file on the file system.

                    rv = file->Create(nsIFile::NORMAL_FILE_TYPE, 0644); // XXX what permissions???
                    if (NS_ERROR_FILE_ALREADY_EXISTS == rv) {
                        rv = file->Delete(PR_FALSE); // the user has already confirmed they
                                                     // want to overwrite the file.
                        if (NS_FAILED(rv)) {
                            this->OnError(kOpCreateFile, rv);
                            return rv;
                        }

                        rv = file->Create(nsIFile::NORMAL_FILE_TYPE, 0644); // XXX what permissions???
                        if (NS_FAILED(rv)) {
                            this->OnError(kOpCreateFile, rv);
                            return rv;
                        }
                    } else if (NS_FAILED(rv)) {
                        this->OnError(kOpCreateFile, rv);
                        return rv;
                    }

                    rv = fts->CreateTransport(file, PR_RDONLY, "load", 0, 0,
                                              getter_AddRefs( mOutputChannel));
                }
    
                if ( NS_SUCCEEDED( rv ) ) {
                    // reset the channel's interface requestor so we receive status
                    // notifications.
                    rv = mInputChannel->SetNotificationCallbacks(NS_STATIC_CAST(nsIInterfaceRequestor*,this));
                    if (NS_FAILED(rv)) {
                        this->OnError(0, rv);
                        return rv;
                    }

                    // get the input stream from the channel.
                    nsCOMPtr<nsIInputStream> inStream;
                    rv = mInputChannel->OpenInputStream(0, -1, getter_AddRefs(inStream));
                    if (NS_FAILED(rv)) {
                        this->OnError(0, rv);
                        return rv;
                    }

                    // hand the output channel our input stream. it will take care
                    // of reading data from the stream and writing it to disk.
                    rv = mOutputChannel->AsyncWrite(inStream, 0, -1, nsnull, NS_STATIC_CAST(nsIStreamObserver*,this));
                    if ( NS_FAILED( rv ) ) {
                        this->OnError( kOpAsyncWrite, rv );
                    }
                } else {
                    this->OnError( kOpCreateTransport, rv );
                    rv = NS_ERROR_OUT_OF_MEMORY;
                }
            } else {
                this->OnError( kOpGetService, rv );
            }
        } else {
            rv = NS_ERROR_ALREADY_INITIALIZED;
            this->OnError( 0, rv );
        }
    } else {
        rv = NS_ERROR_NOT_INITIALIZED;
        this->OnError( 0, rv );
    }

    // If an error occurred, shut down.
    if ( NS_FAILED( rv ) ) {
        this->Stop();
    }

    return rv;
}

// Terminate the download by cancelling/closing input and output channels.
NS_IMETHODIMP
nsStreamXferOp::Stop( void ) {
    nsresult rv = NS_OK;

    // Cancel input.
    if ( mInputChannel ) {
        // Unhook it first.
        nsCOMPtr<nsIChannel> channel = mInputChannel;
        mInputChannel = 0;
        // Now cancel it.
        rv = channel->Cancel();
        if ( NS_FAILED( rv ) ) {
            this->OnError( kOpInputCancel, rv );
        }
    }

    // Close output stream.
    if ( mOutputStream ) {
        // Unhook it first.
        nsCOMPtr<nsIOutputStream> stream = mOutputStream;
        mOutputStream = 0;

        // Now close it.
        rv = stream->Close();
        if ( NS_FAILED( rv ) ) {
            this->OnError( kOpOutputClose, rv );
        }
    }

    // Cancel output channel.
    mOutputChannel = 0;

    return rv;
}

// This is called when the input channel is successfully opened.
//
// We also open the output stream at this point.
NS_IMETHODIMP
nsStreamXferOp::OnStartRequest(nsIChannel* channel, nsISupports* aContext) {
    nsresult rv = NS_OK;

#ifdef DEBUG_law
    DEBUG_PRINTF( PR_STDOUT, "nsStreamXferOp::OnStartRequest; channel=0x%08X, context=0x%08X\n",
                  (int)(void*)channel, (int)(void*)aContext );
#endif

    return rv;
}


// As an event sink getter, we get ourself.
NS_IMETHODIMP
nsStreamXferOp::GetInterface(const nsIID &anIID, void **aResult ) {
    return this->QueryInterface( anIID, (void**)aResult );
}

// Pass notification to our observer (if we have one). This object is the
// "subject", the topic is the component progid (plus ";onProgress"), and
// the data is the progress numbers (in the form "%lu %lu" where the first
// value is the number of bytes processed, the second the total number
// expected.
//
NS_IMETHODIMP
nsStreamXferOp::OnProgress(nsIChannel* channel, nsISupports* aContext,
                                     PRUint32 aProgress, PRUint32 aProgressMax) {
    nsresult rv = NS_OK;

    if (mContentLength < 1) {
        NS_ASSERTION(channel, "should have a channel");
        rv = mInputChannel->GetContentLength(&mContentLength);
        if (NS_FAILED(rv)) return rv;
    }

    if ( mObserver ) {
        char buf[32];
        PR_snprintf( buf, sizeof buf, "%lu %ld", (unsigned long)aProgress, (long)mContentLength );
        rv = mObserver->Observe( (nsIStreamTransferOperation*)this,
                                  nsString( NS_ISTREAMTRANSFER_PROGID ";onProgress" ).GetUnicode(),
                                  nsString( buf ).GetUnicode() );
        if ( NS_FAILED( rv ) ) {
            DEBUG_PRINTF( PR_STDOUT, "%s %d: Observe failed, rv=0x%08X\n",
                          (char*)__FILE__, (int)__LINE__, (int)rv );
        }
    }

    return rv;
}

// Pass notification to our observer (if we have one). This object is the
// "subject", the topic is the component progid (plus ";onStatus"), and
// the data is the status text.
NS_IMETHODIMP
nsStreamXferOp::OnStatus( nsIChannel      *channel,
                          nsISupports     *aContext,
                          const PRUnichar *aMsg ) {
    nsresult rv = NS_OK;

    if ( mObserver ) {
        nsString msg = aMsg;
        rv = mObserver->Observe( (nsIStreamTransferOperation*)this,
                                  nsString( NS_ISTREAMTRANSFER_PROGID ";onStatus" ).GetUnicode(),
                                  nsString( msg ).GetUnicode() );
        if ( NS_FAILED( rv ) ) {
            DEBUG_PRINTF( PR_STDOUT, "%s %d: Observe failed, rv=0x%08X\n",
                          (char*)__FILE__, (int)__LINE__, (int)rv );
        }
    }

    return rv;
}

// This is called when the end of input is reached on the input channel.
NS_IMETHODIMP
nsStreamXferOp::OnStopRequest( nsIChannel      *channel,
                               nsISupports     *aContext,
                               nsresult         aStatus,
                               const PRUnichar *aMsg ) {
    nsresult rv = NS_OK;

    // Close the output stream.
    if ( mOutputStream ) {
        rv = mOutputStream->Close();
        if ( NS_FAILED( rv ) ) {
            this->OnError( kOpOutputClose, rv );
        }
    }

    // Unhook input/output channels (don't need to cancel 'em).
    mInputChannel = 0;
    mOutputChannel = 0;

    // Notify observer that the download is complete.
    if ( mObserver ) {
        nsString msg = aMsg;
        rv = mObserver->Observe( (nsIStreamTransferOperation*)this,
                                  nsString( NS_ISTREAMTRANSFER_PROGID ";onCompletion" ).GetUnicode(),
                                  nsString( msg ).GetUnicode() );
        if ( NS_FAILED( rv ) ) {
            DEBUG_PRINTF( PR_STDOUT, "%s %d: Observe failed, rv=0x%08X\n",
                          (char*)__FILE__, (int)__LINE__, (int)rv );
        }
    }

    return rv;
}

// Attribute getters/setters...

NS_IMETHODIMP
nsStreamXferOp::GetSource( nsIChannel**result ) {
    nsresult rv = NS_OK;

    if ( result ) {
        *result = mInputChannel;
        NS_IF_ADDREF( *result );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsStreamXferOp::GetTarget( nsIFileSpec**result ) {
    nsresult rv = NS_OK;

    if ( result ) {
        *result = mOutputSpec;
        NS_IF_ADDREF( *result );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsStreamXferOp::GetObserver( nsIObserver**result ) {
    nsresult rv = NS_OK;

    if ( result ) {
        *result = mObserver;
        NS_IF_ADDREF( *result );
    } else {
        rv = NS_ERROR_NULL_POINTER;
    }

    return rv;
}

NS_IMETHODIMP
nsStreamXferOp::SetObserver( nsIObserver*aObserver ) {
    nsresult rv = NS_OK;

    NS_IF_RELEASE( mObserver );
    mObserver = aObserver;
    NS_IF_ADDREF( mObserver );

    return rv;
}
