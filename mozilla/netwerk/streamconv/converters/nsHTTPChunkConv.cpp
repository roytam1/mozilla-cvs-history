/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Contributor(s): ruslan
 */

#include "nsHTTPChunkConv.h"
#include "nsIAllocator.h"
#include "plstr.h"
#include "prlog.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsIByteArrayInputStream.h"
#include "nsIStringStream.h"

#include <ctype.h>

// nsISupports implementation
NS_IMPL_THREADSAFE_ISUPPORTS2 (nsHTTPChunkConv, nsIStreamConverter, nsIStreamListener);

// nsFTPDirListingConv methods
nsHTTPChunkConv::nsHTTPChunkConv()
{
    NS_INIT_ISUPPORTS ();
    mListener    = nsnull;
    mChunkBuffer = NULL;
    mState = CHUNK_STATE_INIT;

    mValueBufLen = 0;
    mHeaderBufLen= 0;
    mHeadersCount= mHeadersExpected = 0;

    mChunkContext= NULL;
}

nsHTTPChunkConv::~nsHTTPChunkConv ()
{
    NS_IF_RELEASE(mListener);

    if (mChunkBuffer != NULL)
        nsAllocator::Free (mChunkBuffer);
}

NS_IMETHODIMP
nsHTTPChunkConv::AsyncConvertData (
                            const PRUnichar *aFromType, 
                            const PRUnichar *aToType, 
                            nsIStreamListener *aListener, 
                            nsISupports *aCtxt)
{
    nsString from (aFromType);
    nsString to   ( aToType );

    char * fromStr = from.ToNewCString ();
    char *   toStr =   to.ToNewCString ();

    if (!PL_strncasecmp (fromStr, HTTP_CHUNK_TYPE, strlen (HTTP_CHUNK_TYPE  ) )
        &&
        !PL_strncasecmp (toStr, HTTP_UNCHUNK_TYPE, strlen (HTTP_UNCHUNK_TYPE)))
        mMode = DO_UNCHUNKING;
    else
        mMode = DO_CHUNKING;

    nsAllocator::Free (fromStr);
    nsAllocator::Free (  toStr);

    // hook ourself up with the receiving listener. 
    mListener = aListener;
    NS_ADDREF (mListener);

    mAsyncConvContext = (nsISupportsVoid *) aCtxt;
    if (mAsyncConvContext)
    {
        const void *p;
        mAsyncConvContext -> GetData (&p);
        mChunkContext = (nsHTTPChunkConvContext *)p;
    }

    return NS_OK; 
} 

NS_IMETHODIMP
nsHTTPChunkConv::OnStartRequest (nsIChannel *aChannel, nsISupports *aContext)
{
    return mListener -> OnStartRequest (aChannel, aContext);
} 

NS_IMETHODIMP
nsHTTPChunkConv::OnStopRequest  (nsIChannel *aChannel, nsISupports *aContext, nsresult status, const PRUnichar *errorMsg)
{
    return mListener -> OnStopRequest  (aChannel, aContext, status, errorMsg);
} 

NS_IMETHODIMP
nsHTTPChunkConv::OnDataAvailable ( 
                              nsIChannel *aChannel, 
                              nsISupports *aContext, 
                              nsIInputStream *iStr, 
                              PRUint32 aSourceOffset, 
                              PRUint32 aCount)
{
    nsresult rv = NS_ERROR_FAILURE;
    PRUint32 rl;
    PRUint32 streamLen;
    char c = 0;

    rv = iStr -> Available (&streamLen);
    if (NS_FAILED (rv))
        return rv;

    if (streamLen == 0)
        return NS_OK;

    if (mMode == DO_CHUNKING)
    {
        mChunkBuffer = (char * )nsAllocator::Alloc (streamLen + 20);
        mChunkBufferPos = sprintf (mChunkBuffer, "%x%c%c", streamLen, '\r', '\n');

        rv = iStr -> Read (&mChunkBuffer[mChunkBufferPos], streamLen, &rl);
        if (NS_FAILED (rv) || rl != streamLen)
            return rv;

        mChunkBufferPos += streamLen;
        mChunkBuffer[mChunkBufferPos++] = '\r';
        mChunkBuffer[mChunkBufferPos++] = '\n';
        mChunkBuffer[mChunkBufferPos  ] = 0;

        mChunkBufferLength = mChunkBufferPos;

        nsIInputStream * convertedStream = nsnull; 
        nsIByteArrayInputStream * convertedStreamSup = nsnull;

        rv = NS_NewByteArrayInputStream (&convertedStreamSup, mChunkBuffer, mChunkBufferPos);
        if (NS_FAILED (rv)) 
            return rv;

        mChunkBuffer = NULL;
        mChunkBufferPos = 0;

        rv = convertedStreamSup -> QueryInterface (NS_GET_IID (nsIInputStream), (void**)&convertedStream);
        NS_RELEASE (convertedStreamSup);
 
        if (NS_FAILED (rv)) 
            return rv;

        rv = mListener -> OnDataAvailable (aChannel, aContext, convertedStream, aSourceOffset, mChunkBufferLength);
        
        if (NS_FAILED (rv))
            return rv;
    }
    else
    {
        // DO_UNCHUNKING

        while (mState != CHUNK_STATE_DONE)
        {
            switch (mState)
            {
                case CHUNK_STATE_INIT:
                    
                    if (mChunkBuffer != NULL)
                    {
                        nsAllocator::Free (mChunkBuffer);
                        mChunkBuffer = NULL;
                    }

                    mChunkBufferPos = mChunkBufferLength = 0;
                    mLenBufCnt = 0;
                    c = 0;

                    mState = CHUNK_STATE_LENGTH;
                    break;

                case CHUNK_STATE_FINAL:
                    // send data upstream

                    {
                        if (mChunkBufferLength > 0)
                        {
                            nsCOMPtr<nsIByteArrayInputStream> convertedStreamSup;
                            rv = NS_NewByteArrayInputStream (getter_AddRefs(convertedStreamSup), mChunkBuffer, mChunkBufferLength);
                            if (NS_FAILED (rv)) 
                                return rv;

                            mChunkBuffer = NULL;

                            nsCOMPtr<nsIInputStream> convertedStream = do_QueryInterface (convertedStreamSup, &rv);
 
                            if (NS_FAILED (rv))
                                return rv;

                            rv = mListener -> OnDataAvailable (aChannel, aContext, convertedStream, aSourceOffset, mChunkBufferLength);

                            if (NS_FAILED (rv))
                                return rv;

                            mState = CHUNK_STATE_INIT;
                        }
                        else
                        {
                            if (mChunkContext)
                                mChunkContext -> SetEOF (PR_TRUE);
                            
                            mState = CHUNK_STATE_DONE;
                        }
                    
                        if (mChunkBuffer != NULL)
                        {
                            nsAllocator::Free (mChunkBuffer);
                            mChunkBuffer = NULL;
                        }
                    }

                    break;

                case CHUNK_STATE_CR:
                case CHUNK_STATE_CR_FINAL:

                    if (!streamLen)
                        return NS_OK;

                    rv = iStr -> Read (&c, 1, &rl);
                    if (NS_FAILED (rv))
                        return rv;

                    if (c != '\r' && c != '\n') // be really relaxed here cuz of numerous spec violations
                        return NS_ERROR_FAILURE;
                
                    streamLen--;
                    mState = mState == CHUNK_STATE_CR ? CHUNK_STATE_LF : CHUNK_STATE_LF_FINAL;
                    break;

                case CHUNK_STATE_LF:
                case CHUNK_STATE_LF_FINAL:
                    
                    if (c != '\n')
                    {
                        if (!streamLen)
                            return NS_OK;

                        rv = iStr -> Read (&c, 1, &rl);
                        if (NS_FAILED (rv))
                            return rv;
                    }

                    if (c != '\n')
                        return NS_ERROR_FAILURE;

                    streamLen--;
                    if (mState == CHUNK_STATE_LF)
                    {
                        if (mChunkBufferLength > 0)
                        {
                            mChunkBuffer = (char * )nsAllocator::Alloc (mChunkBufferLength + 1);
                            mState = CHUNK_STATE_DATA;
                        }
                        else
                            mState = CHUNK_STATE_CR_FINAL;
                    }
                    else
                        mState = CHUNK_STATE_TRAILER;
                    
                    c = 0;
                    break;

                case CHUNK_STATE_LENGTH:
                
                    if (mLenBufCnt >= sizeof (mLenBuf) - 1)
                        return NS_ERROR_FAILURE;

                    if (!streamLen)
                        return NS_OK;

                    rv = iStr -> Read (&c, 1, &rl);
                    if (NS_FAILED (rv))
                        return rv;

                    streamLen--;
                
                    if (isxdigit (c))
                    {
                        mLenBuf[mLenBufCnt++] = c;
                        c = 0;
                    }
                    else
                    if (c == '\r' || c == '\n')
                    {
                        if (mLenBufCnt > 0)
                        {
                            // ruslan: have to add this due to a lot of spec violations
                            //      similating what IE does here

                            mLenBuf[mLenBufCnt] = 0;
                            sscanf (mLenBuf, "%x", &mChunkBufferLength);

                            mState = CHUNK_STATE_LF;
                        }
                    }
                    break;
            
                case CHUNK_STATE_DATA:
                    if (mChunkBufferLength - mChunkBufferPos <= streamLen)
                    {
                        // entire chunk
                        if (!streamLen)
                            return NS_OK;

                        rv = iStr -> Read (&mChunkBuffer[mChunkBufferPos], mChunkBufferLength - mChunkBufferPos, &rl);
                        if (NS_FAILED (rv))
                            return rv;

                        mChunkBufferPos += rl;
                        mChunkBuffer[mChunkBufferPos++] = 0;
                        streamLen -= rl;
                        mState = CHUNK_STATE_CR_FINAL;
                    }
                    else
                    {
                        if (!streamLen)
                            return NS_OK;

                        rv = iStr -> Read (&mChunkBuffer[mChunkBufferPos], streamLen, &rl);
                        if (NS_FAILED (rv))
                            return rv;

                        mChunkBufferPos += rl;
                        streamLen -= rl;
                    }
                    break;
                
                case CHUNK_STATE_TRAILER:
                    
                    if (!mChunkContext || mHeadersCount == mChunkContext -> GetTrailerHeaderCount ())
                        mState = CHUNK_STATE_FINAL;
                    else
                        mState = CHUNK_STATE_TRAILER_HEADER;

                    break;

                case CHUNK_STATE_TRAILER_HEADER:
                    
                    if (!streamLen)
                        return NS_OK;

                    rv = iStr -> Read (&c, 1, &rl);
                    if (NS_FAILED (rv))
                        return rv;

                    streamLen--;
                    if (isalnum (c) && mHeaderBufLen < sizeof (mHeaderBuf) - 1)
                        mHeaderBuf[mHeaderBufLen++] = c;
                    else
                    if (c == ':')
                    {
                        mHeaderBuf[mHeaderBufLen] = 0;
                        mState = CHUNK_STATE_TRAILER_VALUE;
                    }
                    break;
                
                case CHUNK_STATE_TRAILER_VALUE:

                    if (!streamLen)
                        return NS_OK;

                    rv = iStr -> Read (&c, 1, &rl);
                    if (NS_FAILED (rv))
                        return rv;

                    streamLen--;

                    if (isspace (c) && mValueBufLen == 0 || c == '\r')
                        break;
                    else
                    if (c == '\n')
                    {
                        mValueBuf[mValueBufLen] = 0;
                        mHeadersCount++;

                        mChunkContext -> SetResponseHeader (mHeaderBuf, mValueBuf);
                        mHeaderBufLen = mValueBufLen = 0;
                        mState = CHUNK_STATE_TRAILER;
                    }
                    else
                    if (mValueBufLen < sizeof (mValueBuf) - 1)
                        mValueBuf[mValueBufLen++] = c;

                    break;
            } /* switch */
        } /* while */
    } /* DO_UNCHUNKING */

    return NS_OK;
} /* OnDataAvailable */


// XXX/ruslan: need to implement this too

NS_IMETHODIMP
nsHTTPChunkConv::Convert (
                          nsIInputStream *aFromStream, 
                          const PRUnichar *aFromType, 
                          const PRUnichar *aToType, 
                          nsISupports *aCtxt, 
                          nsIInputStream **_retval)
{ 
    return NS_ERROR_NOT_IMPLEMENTED;
} 

nsresult
NS_NewHTTPChunkConv (nsHTTPChunkConv ** aHTTPChunkConv)
{
    NS_PRECONDITION(aHTTPChunkConv != nsnull, "null ptr");

    if (! aHTTPChunkConv)
        return NS_ERROR_NULL_POINTER;

    *aHTTPChunkConv = new nsHTTPChunkConv ();

    if (! *aHTTPChunkConv)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aHTTPChunkConv);
    return NS_OK;
}


