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
 * Contributor(s): 
 */

#include "nsFileStreams.h"
#include "nsILocalFile.h"
#include "nsXPIDLString.h"
#include "prerror.h"
#include "nsCRT.h"
#include "nsInt64.h"
#include "nsCExternalHandlerService.h"
#include "nsIMIMEService.h"
#include "nsIFile.h"
#include "nsDirectoryIndexStream.h"
#include "nsMimeTypes.h"

#define NS_NO_INPUT_BUFFERING 1 // see http://bugzilla.mozilla.org/show_bug.cgi?id=41067

#if defined(PR_LOGGING)
//
// Log module for nsFileTransport logging...
//
// To enable logging (see prlog.h for full details):
//
//    set NSPR_LOG_MODULES=nsFileIO:5
//    set NSPR_LOG_FILE=nspr.log
//
// this enables PR_LOG_DEBUG level information and places all output in
// the file nspr.log
//
PRLogModuleInfo* gFileIOLog = nsnull;

#endif /* PR_LOGGING */

////////////////////////////////////////////////////////////////////////////////
// nsFileIO

#define NS_INPUT_STREAM_BUFFER_SIZE     (16 * 1024)
#define NS_OUTPUT_STREAM_BUFFER_SIZE    (64 * 1024)

NS_IMPL_THREADSAFE_ISUPPORTS2(nsFileIO,
                              nsIFileIO,
                              nsIStreamIO)

nsFileIO::nsFileIO()
    : mIOFlags(0),
      mPerm(0),
      mStatus(NS_OK)
{
    NS_INIT_REFCNT();
#if defined(PR_LOGGING)
    //
    // Initialize the global PRLogModule for socket transport logging
    // if necessary...
    //
    if (nsnull == gFileIOLog) {
        gFileIOLog = PR_NewLogModule("nsFileIO");
    }

    mSpec = nsnull;
#endif /* PR_LOGGING */
}

nsFileIO::~nsFileIO()
{
    (void)Close(NS_OK);
#ifdef PR_LOGGING
    if (mSpec) nsCRT::free(mSpec);
#endif      
}

NS_METHOD
nsFileIO::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;
    nsFileIO* io = new nsFileIO();
    if (io == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(io);
    nsresult rv = io->QueryInterface(aIID, aResult);
    NS_RELEASE(io);
    return rv;
}

NS_IMETHODIMP
nsFileIO::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm)
{
   NS_ASSERTION(file, "File must not be null");
   if (file == nsnull)
        return NS_ERROR_NOT_INITIALIZED;

    mFile = file;
    mIOFlags = ioFlags;
    mPerm = perm;
#ifdef PR_LOGGING
    nsresult rv = mFile->GetPath(&mSpec);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetSpec failed");
#endif      
    return NS_OK;
}

NS_IMETHODIMP
nsFileIO::GetFile(nsIFile* *aFile)
{
    *aFile = mFile;
    NS_ADDREF(*aFile);
    return NS_OK;
}

NS_IMETHODIMP
nsFileIO::Open(char **contentType, PRInt32 *contentLength)
{
    NS_ASSERTION(mFile, "File must not be null");
    if (mFile == nsnull)
        return NS_ERROR_NOT_INITIALIZED;

    // don't actually open the file here -- we'll do it on demand in the
    // GetInputStream/GetOutputStream methods
    nsresult rv = NS_OK;

    // We'll try to use the file's length, if it has one. If not,
    // assume the file to be special, and set the content length
    // to -1, which means "read the stream until exhausted".
    PRInt64 size;
    rv = mFile->GetFileSize(&size);
    if (NS_SUCCEEDED(rv)) {
        *contentLength = nsInt64(size);
        if (! *contentLength)
            *contentLength = -1;
    }
    else 
        *contentLength = -1;

    PRBool isDir;
    rv = mFile->IsDirectory(&isDir);
    if (NS_SUCCEEDED(rv) && isDir) {
        // Directories turn into an HTTP-index stream, with
        // unbounded (i.e., read 'til the stream says it's done)
        // length.
        *contentType = nsCRT::strdup("application/http-index-format");
        *contentLength = -1;
    }
    else {
        nsCOMPtr<nsIMIMEService> mimeServ (do_GetService(NS_MIMESERVICE_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) {
            rv = mimeServ->GetTypeFromFile(mFile, contentType);
        }
	
        if (NS_FAILED(rv)) {
            // if all else fails treat it as text/html?
            *contentType = nsCRT::strdup(UNKNOWN_CONTENT_TYPE);
            if (*contentType == nsnull)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                rv = NS_OK;
        }
    }
    PR_LOG(gFileIOLog, PR_LOG_DEBUG,
           ("nsFileIO: logically opening %s: type=%s len=%d",
            mSpec, *contentType, *contentLength));
    return rv;
}

NS_IMETHODIMP
nsFileIO::Close(nsresult status)
{
    if (mFile) {
        PR_LOG(gFileIOLog, PR_LOG_DEBUG,
               ("nsFileIO: logically closing %s: status=%x",
                mSpec, status));
        mFile = nsnull;
    }
    mStatus = status;
    return NS_OK;
}

NS_IMETHODIMP
nsFileIO::GetInputStream(nsIInputStream * *aInputStream)
{
    NS_ASSERTION(mFile, "File must not be null");
    if (mFile == nsnull)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    PRBool isDir;
    rv = mFile->IsDirectory(&isDir);
    if (NS_SUCCEEDED(rv) && isDir) {
        rv = nsDirectoryIndexStream::Create(mFile, aInputStream);
        PR_LOG(gFileIOLog, PR_LOG_DEBUG,
               ("nsFileIO: opening local dir %s for input (%x)",
                mSpec, rv));
        return rv;
    }

    nsFileInputStream* fileIn = new nsFileInputStream();
    if (fileIn == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(fileIn);
    rv = fileIn->Init(mFile, mIOFlags, mPerm);
    if (NS_SUCCEEDED(rv)) {
#ifdef NS_NO_INPUT_BUFFERING
        *aInputStream = fileIn;
        NS_ADDREF(*aInputStream);
#else
        rv = NS_NewBufferedInputStream(aInputStream,
                                       fileIn, NS_OUTPUT_STREAM_BUFFER_SIZE);
#endif
    }
    NS_RELEASE(fileIn);

    PR_LOG(gFileIOLog, PR_LOG_DEBUG,
           ("nsFileIO: opening local file %s for input (%x)",
            mSpec, rv));
    return rv;
}

NS_IMETHODIMP
nsFileIO::GetOutputStream(nsIOutputStream * *aOutputStream)
{
    NS_ASSERTION(mFile, "File must not be null");
    if (mFile == nsnull)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    PRBool isDir;
    rv = mFile->IsDirectory(&isDir);
    if (NS_SUCCEEDED(rv) && isDir) {
        return NS_ERROR_FAILURE;
    }

    nsFileOutputStream* fileOut = new nsFileOutputStream();
    if (fileOut == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(fileOut);
    rv = fileOut->Init(mFile, mIOFlags, mPerm);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIOutputStream> bufStr;
#ifdef NS_NO_OUTPUT_BUFFERING
        *aOutputStream = fileOut;
        NS_ADDREF(*aOutputStream);
#else
        rv = NS_NewBufferedOutputStream(aOutputStream,
                                        fileOut, NS_OUTPUT_STREAM_BUFFER_SIZE);
#endif
    }
    NS_RELEASE(fileOut);

    PR_LOG(gFileIOLog, PR_LOG_DEBUG,
           ("nsFileIO: opening local file %s for output (%x)",
            mSpec, rv));
    return rv;
}

NS_IMETHODIMP
nsFileIO::GetName(char* *aName)
{
    NS_ASSERTION(mFile, "File must not be null");
    if (mFile == nsnull)
        return NS_ERROR_NOT_INITIALIZED;

    return mFile->GetPath(aName);
}

////////////////////////////////////////////////////////////////////////////////
// nsFileStream

nsFileStream::nsFileStream()
    : mFD(nsnull)
{
    NS_INIT_REFCNT();
}

nsFileStream::~nsFileStream()
{
    Close();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsFileStream, nsISeekableStream);

nsresult
nsFileStream::Close()
{
    if (mFD) {
        PR_Close(mFD);
        mFD = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileStream::Seek(PRInt32 whence, PRInt32 offset)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Seek(mFD, offset, (PRSeekWhence)whence);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFileStream::Tell(PRUint32 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Seek(mFD, 0, PR_SEEK_CUR);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsFileInputStream

NS_IMPL_ISUPPORTS_INHERITED2(nsFileInputStream, 
                             nsFileStream,
                             nsIInputStream,
                             nsIFileInputStream);

NS_METHOD
nsFileInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileInputStream* stream = new nsFileInputStream();
    if (stream == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsFileInputStream::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm)
{
    NS_ASSERTION(mFD == nsnull, "already inited");
    if (mFD != nsnull)
        return NS_ERROR_FAILURE;
    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    if (NS_FAILED(rv)) return rv;
    if (ioFlags == -1)
        ioFlags = PR_RDONLY;
    if (perm == -1)
        perm = 0;
    return localFile->OpenNSPRFileDesc(ioFlags, perm, &mFD);
}

NS_IMETHODIMP
nsFileInputStream::Close()
{
    return nsFileStream::Close();
}

NS_IMETHODIMP
nsFileInputStream::Available(PRUint32 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 avail = PR_Available(mFD);
    if (avail == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = avail;
    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::Read(char * buf, PRUint32 count, PRUint32 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Read(mFD, buf, count);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

NS_IMETHODIMP
nsFileInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval)
{
    NS_NOTREACHED("ReadSegments");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileInputStream::GetNonBlocking(PRBool *aNonBlocking)
{
    NS_NOTREACHED("GetNonBlocking");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileInputStream::GetObserver(nsIInputStreamObserver * *aObserver)
{
    NS_NOTREACHED("GetObserver");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileInputStream::SetObserver(nsIInputStreamObserver * aObserver)
{
    NS_NOTREACHED("SetObserver");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
// nsFileOutputStream

NS_IMPL_ISUPPORTS_INHERITED2(nsFileOutputStream, 
                             nsFileStream,
                             nsIOutputStream,
                             nsIFileOutputStream);
 
NS_METHOD
nsFileOutputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsFileOutputStream* stream = new nsFileOutputStream();
    if (stream == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(stream);
    nsresult rv = stream->QueryInterface(aIID, aResult);
    NS_RELEASE(stream);
    return rv;
}

NS_IMETHODIMP
nsFileOutputStream::Init(nsIFile* file, PRInt32 ioFlags, PRInt32 perm)
{
    NS_ASSERTION(mFD == nsnull, "already inited");
    if (mFD != nsnull)
        return NS_ERROR_FAILURE;
    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    if (NS_FAILED(rv)) return rv;
    if (ioFlags == -1)
        ioFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    if (perm <= 0)
        perm = 0664;
    return localFile->OpenNSPRFileDesc(ioFlags, perm, &mFD);
}

NS_IMETHODIMP
nsFileOutputStream::Close()
{
    return nsFileStream::Close();
}

NS_IMETHODIMP
nsFileOutputStream::Write(const char *buf, PRUint32 count, PRUint32 *result)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Write(mFD, buf, count);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    *result = cnt;
    return NS_OK;
}

NS_IMETHODIMP
nsFileOutputStream::Flush(void)
{
    if (mFD == nsnull)
        return NS_BASE_STREAM_CLOSED;

    PRInt32 cnt = PR_Sync(mFD);
    if (cnt == -1) {
        return NS_ErrorAccordingToNSPR();
    }
    return NS_OK;
}
    
NS_IMETHODIMP
nsFileOutputStream::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
    NS_NOTREACHED("WriteFrom");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileOutputStream::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    NS_NOTREACHED("WriteSegments");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileOutputStream::GetNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsFileOutputStream::SetNonBlocking(PRBool aNonBlocking)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileOutputStream::GetObserver(nsIOutputStreamObserver * *aObserver)
{
    NS_NOTREACHED("GetObserver");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFileOutputStream::SetObserver(nsIOutputStreamObserver * aObserver)
{
    NS_NOTREACHED("SetObserver");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////
