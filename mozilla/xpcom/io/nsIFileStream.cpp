/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIFileStream.h"
#include "nsFileSpec.h"
#include "nsCOMPtr.h"

#include "prerror.h"

#include "nsSegmentedBuffer.h"

#ifdef XP_MAC
#include "pprio.h" // To get PR_ImportFile
#else
#include "prio.h"
#endif

#ifdef XP_MAC
#include <Errors.h>
#include <iostream>
#endif

//========================================================================================
class FileImpl
    : public nsIRandomAccessStore
    , public nsIFileOutputStream
    , public nsIFileInputStream
    , public nsIOpenFile
//========================================================================================
{
    public:
                                        FileImpl(PRFileDesc* inDesc)
                                            : mFileDesc(inDesc)
											, mNSPRMode(0)
                                            , mFailed(PR_FALSE)
                                            , mEOF(PR_FALSE)
                                            , mLength(-1)
                                        {
                                            NS_INIT_REFCNT();

                                            nsresult rv = mOutBuffer.Init(4096, 4096);
                                            if (NS_FAILED(rv)) mFailed = PR_TRUE;

                                            mWriteCursor = nsnull;
                                            mWriteLimit  = nsnull;

                                        }
                                        FileImpl(
                                            const nsFileSpec& inFile,
                                            int nsprMode,
                                            PRIntn accessMode)
                                            : mFileDesc(nsnull)
											, mNSPRMode(-1)
                                            , mFailed(PR_FALSE)
                                            , mEOF(PR_FALSE)
                                            , mLength(-1)
                                        {
                                            NS_INIT_REFCNT();

                                            nsresult rv = mOutBuffer.Init(4096, 4096);
                                            if (NS_FAILED(rv)) mFailed = PR_TRUE;
    
                                            mWriteCursor = nsnull;
                                            mWriteLimit  = nsnull;

                                            Open(inFile, nsprMode, accessMode);
                                        }
        virtual                         ~FileImpl()
                                        {
                                            Close();
                                        }
        
        // nsISupports interface
                                        NS_DECL_ISUPPORTS

		// nsIOpenFile interface
        NS_IMETHOD                      Open(
                                            const nsFileSpec& inFile,
                                            int nsprMode,
                                            PRIntn accessMode);
        NS_IMETHOD                      Close();
        NS_IMETHOD                      Seek(PRSeekWhence whence, PRInt32 offset);

        NS_IMETHOD                      GetIsOpen(PRBool* outOpen)
                                        {
                                            *outOpen = (mFileDesc != nsnull);
                                            return NS_OK;
                                        }
        NS_IMETHOD                      Tell(PRIntn* outWhere);

		// nsIInputStream interface
        NS_IMETHOD                      Available(PRUint32 *aLength)
                                        {
                                            NS_PRECONDITION(aLength != nsnull, "null ptr");
                                            if (!aLength)
                                                return NS_ERROR_NULL_POINTER;
                                            if (mLength < 0)
                                                return NS_ERROR_UNEXPECTED;
                                            *aLength = mLength;
                                            return NS_OK;
                                        }
		NS_IMETHOD                      Read(char* aBuf,
			                                PRUint32 aCount,
					                        PRUint32 *aReadCount)
								        {
								            NS_PRECONDITION(aBuf != nsnull, "null ptr");
								            if (!aBuf)
								                return NS_ERROR_NULL_POINTER;
								            NS_PRECONDITION(aReadCount != nsnull, "null ptr");
								            if (!aReadCount)
								                return NS_ERROR_NULL_POINTER;
								            if (!mFileDesc)
								                return NS_FILE_RESULT(PR_BAD_DESCRIPTOR_ERROR);
								            if (mFailed)
								                return NS_ERROR_FAILURE;
								            PRInt32 bytesRead = PR_Read(mFileDesc, aBuf, aCount);
								            if (bytesRead < 0)
								            {
								                *aReadCount = 0;
								                mFailed = PR_TRUE;
								                return NS_FILE_RESULT(PR_GetError());
								            }
                                            else if (bytesRead == 0) {
                                                mEOF = PR_TRUE;
                                            }
								            *aReadCount = bytesRead;
								            return NS_OK;
								        }
		// nsIOutputStream interface
		NS_IMETHOD                      Write(const char* aBuf,
			                                PRUint32 aCount,
					                        PRUint32 *aWriteCount)
								        {
								            NS_PRECONDITION(aBuf != nsnull, "null ptr");
								            NS_PRECONDITION(aWriteCount != nsnull, "null ptr");
                                            
                                            *aWriteCount = 0;

								#ifdef XP_MAC
								            // Calling PR_Write on stdout is sure suicide.
								            if (mFileDesc == PR_STDOUT || mFileDesc == PR_STDERR)
								            {
								                cout.write(aBuf, aCount);
								                *aWriteCount = aCount;
								                return NS_OK;
								            }
								#endif
								            if (!mFileDesc)
								                return NS_FILE_RESULT(PR_BAD_DESCRIPTOR_ERROR);
								            if (mFailed)
								               return NS_ERROR_FAILURE;

                                            PRUint32 bufOffset = 0;
                                            
                                            while (aCount > 0) 
                                            {
                                                if (mWriteCursor == nsnull || mWriteCursor == mWriteLimit)
                                                {
                                                    char* seg = mOutBuffer.AppendNewSegment();
                                                    if (seg == nsnull) 
                                                    {
                                                        // buffer is full, try again
                                                        Flush();
                                                        seg = mOutBuffer.AppendNewSegment();
                                                        if (seg == nsnull)
                                                            return NS_ERROR_OUT_OF_MEMORY;
                                                    }
                                                    mWriteCursor = seg;
                                                    mWriteLimit  = seg + mOutBuffer.GetSegmentSize();
                                                }
                                                
                                                // move
                                                
                                                *aWriteCount = mWriteLimit - mWriteCursor;
                                                
                                                if (aCount < *aWriteCount)
                                                    *aWriteCount = aCount;

                                                memcpy(mWriteCursor, (aBuf + bufOffset), *aWriteCount);
                                                
                                                aCount    -= *aWriteCount;
                                                bufOffset += *aWriteCount;
                                                mWriteCursor += *aWriteCount;                                          
                                            }								            
								            return NS_OK;
								        }
        NS_IMETHOD                      Flush();

        NS_IMETHOD                      GetAtEOF(PRBool* outAtEOF)
        						        {
        						            *outAtEOF = mEOF;
        						            return NS_OK;
        						        }
        NS_IMETHOD                      SetAtEOF(PRBool inAtEOF)
        						        {
        						            mEOF = inAtEOF;
        						            return NS_OK;
        						        }

    protected:
    
        PRFileDesc*                     mFileDesc;
        int                             mNSPRMode;
        PRBool                          mFailed;
        PRBool                          mEOF;
        PRInt32                         mLength;

        nsSegmentedBuffer               mOutBuffer;
        char*                           mWriteCursor;
        char*                           mWriteLimit;

}; // class FileImpl

NS_IMPL_RELEASE(FileImpl)
NS_IMPL_ADDREF(FileImpl)

NS_IMPL_QUERY_HEAD(FileImpl)
  NS_IMPL_QUERY_BODY(nsIOpenFile)
  NS_IMPL_QUERY_BODY(nsIRandomAccessStore)
  NS_IMPL_QUERY_BODY(nsIOutputStream)
  NS_IMPL_QUERY_BODY(nsIInputStream)
  NS_IMPL_QUERY_BODY(nsIFileInputStream)
  NS_IMPL_QUERY_BODY(nsIFileOutputStream)
  if ( aIID.Equals(NS_GET_IID(nsIBaseStream)) )
    foundInterface = NS_STATIC_CAST(nsIBaseStream*, NS_STATIC_CAST(nsIOutputStream*, this));
  else
NS_IMPL_QUERY_TAIL(nsIOutputStream)


//----------------------------------------------------------------------------------------
NS_IMETHODIMP FileImpl::Open(
    const nsFileSpec& inFile,
    int nsprMode,
    PRIntn accessMode)
//----------------------------------------------------------------------------------------
{
    if (mFileDesc)
        if ((nsprMode & mNSPRMode) == nsprMode)
            return NS_OK;
        else
            return NS_FILE_RESULT(PR_ILLEGAL_ACCESS_ERROR);
        
    const int nspr_modes[]={
        PR_WRONLY | PR_CREATE_FILE,
        PR_WRONLY | PR_CREATE_FILE | PR_APPEND,
        PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
        PR_RDONLY,
        PR_RDONLY | PR_APPEND,
        PR_RDWR | PR_CREATE_FILE,
        PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE,
//      "wb",
//      "ab", 
//      "wb",
//      "rb",
//      "r+b",
//      "w+b",
        0 };
    const int* currentLegalMode = nspr_modes;
    while (*currentLegalMode && nsprMode != *currentLegalMode)
        ++currentLegalMode;
    if (!*currentLegalMode) 
        return NS_FILE_RESULT(PR_ILLEGAL_ACCESS_ERROR);

#ifdef XP_MAC
     // Use the file spec to open the file, because one path can be common to
     // several files on the Macintosh (you can have several volumes with the
     // same name, see).
    mFileDesc = 0;
    OSErr err = inFile.Error();
    if (err != noErr)
    	if (err != fnfErr || !(nsprMode & PR_CREATE_FILE))
        	return NS_FILE_RESULT(inFile.Error());
    err = noErr;
#if DEBUG
    const OSType kCreator = 'CWIE';
#else
    const OSType kCreator = 'MOSS';
#endif
    // Resolve the alias to the original file.
    nsFileSpec original = inFile;
    PRBool ignoredResult;
    original.ResolveSymlink(ignoredResult);
    const FSSpec& spec = original.operator const FSSpec&();
    if (nsprMode & PR_CREATE_FILE)
        err = FSpCreate(&spec, kCreator, 'TEXT', 0);
    if (err == dupFNErr)
        err = noErr;
    if (err != noErr)
        return NS_FILE_RESULT(err);
    
    SInt8 perm;
    if (nsprMode & PR_RDWR)
       perm = fsRdWrPerm;
    else if (nsprMode & PR_WRONLY)
       perm = fsWrPerm;
    else
       perm = fsRdPerm;

    short refnum;
    err = FSpOpenDF(&spec, perm, &refnum);

    if (err == noErr && (nsprMode & PR_TRUNCATE))
        err = SetEOF(refnum, 0);
    if (err == noErr && (nsprMode & PR_APPEND))
        err = SetFPos(refnum, fsFromLEOF, 0);
    if (err != noErr)
        return NS_FILE_RESULT(err);

    if ((mFileDesc = PR_ImportFile(refnum)) == 0)
        return NS_FILE_RESULT(PR_GetError());
#else
    //    Platforms other than Macintosh...
    //  Another bug in NSPR: Mac PR_Open assumes a unix style path, but Win PR_Open assumes
    //  a windows path.
    if ((mFileDesc = PR_Open((const char*)nsFileSpec(inFile), nsprMode, accessMode)) == 0)
        return NS_FILE_RESULT(PR_GetError());
#endif
     mNSPRMode = nsprMode;
     mLength = PR_Available(mFileDesc);
     return NS_OK;
} // FileImpl::Open

//----------------------------------------------------------------------------------------
NS_IMETHODIMP FileImpl::Seek(PRSeekWhence whence, PRInt32 offset)
//----------------------------------------------------------------------------------------
{
    if (mFileDesc==PR_STDIN || mFileDesc==PR_STDOUT || mFileDesc==PR_STDERR || !mFileDesc) 
       return NS_FILE_RESULT(PR_BAD_DESCRIPTOR_ERROR);
    mFailed = PR_FALSE; // reset on a seek.
    mEOF = PR_FALSE; // reset on a seek.
    PRInt32 position = PR_Seek(mFileDesc, 0, PR_SEEK_CUR);
    PRInt32 available = PR_Available(mFileDesc);
    PRInt32 fileSize = position + available;
    PRInt32 newPosition = 0;
    switch (whence)
    {
        case PR_SEEK_CUR: newPosition = position + offset; break;
        case PR_SEEK_SET: newPosition = offset; break;
        case PR_SEEK_END: newPosition = fileSize + offset; break;
    }
    if (newPosition < 0)
    {
        newPosition = 0;
        mFailed = PR_TRUE;
    }
    if (newPosition >= fileSize) // nb: not "else if".
    {
        newPosition = fileSize;
        mEOF = PR_TRUE;
    }
    if (PR_Seek(mFileDesc, newPosition, PR_SEEK_SET) < 0)
        mFailed = PR_TRUE;
    return NS_OK;
} // FileImpl::Seek

//----------------------------------------------------------------------------------------
NS_IMETHODIMP FileImpl::Tell(PRIntn* outWhere)
//----------------------------------------------------------------------------------------
{
    if (mFileDesc==PR_STDIN || mFileDesc==PR_STDOUT || mFileDesc==PR_STDERR || !mFileDesc) 
       return NS_FILE_RESULT(PR_BAD_DESCRIPTOR_ERROR);
    *outWhere = PR_Seek(mFileDesc, 0, PR_SEEK_CUR);
    return NS_OK;
} // FileImpl::Tell

//----------------------------------------------------------------------------------------
NS_IMETHODIMP FileImpl::Close()
//----------------------------------------------------------------------------------------
{
    Flush();

    if (mFileDesc==PR_STDIN || mFileDesc==PR_STDOUT || mFileDesc==PR_STDERR || !mFileDesc) 
       return NS_OK;
    if (PR_Close(mFileDesc) == PR_SUCCESS)
        mFileDesc = 0;
    else
        return NS_FILE_RESULT(PR_GetError());
    return NS_OK;
} // FileImpl::close

//----------------------------------------------------------------------------------------
NS_IMETHODIMP FileImpl::Flush()
//----------------------------------------------------------------------------------------
{
#ifdef XP_MAC
    if (mFileDesc == PR_STDOUT || mFileDesc == PR_STDERR)
    {
        cout.flush();
        return NS_OK;
    }
#endif
    if (!mFileDesc) 
        return NS_FILE_RESULT(PR_BAD_DESCRIPTOR_ERROR);
    
    PRInt32 segCount = mOutBuffer.GetSegmentCount();
    PRUint32 segSize = mOutBuffer.GetSegmentSize();

    for (PRInt32 i = 0; i < segCount; i++) 
    {
        char* seg = mOutBuffer.GetSegment(i);

        // if it is the last buffer, it may not be completely full.  
        if(i == (segCount-1))
            segSize = (mWriteCursor - seg);

        PRInt32 bytesWrit = PR_Write(mFileDesc, seg, segSize);
        if (bytesWrit != (PRInt32)segSize)
		{
			mFailed = PR_TRUE;
			return NS_FILE_RESULT(PR_GetError());
		}
    }

    mOutBuffer.Empty();
    mWriteCursor = nsnull;
    mWriteLimit  = nsnull;

#ifdef XP_MAC
    // On unix, it seems to fail always.
    if (PR_Sync(mFileDesc) != PR_SUCCESS)
        mFailed = PR_TRUE;
#endif
                                                
    return NS_OK;
} // FileImpl::flush

//----------------------------------------------------------------------------------------
NS_COM nsresult NS_NewTypicalInputFileStream(
    nsISupports** aResult,
    const nsFileSpec& inFile
    /*Default nsprMode == PR_RDONLY*/
    /*Default accessmode = 0700 (octal)*/)
// Factory method to get an nsInputStream from a file, using most common options
//----------------------------------------------------------------------------------------
{
  // This QueryInterface was needed because NS_NewIOFileStream
  // does a cast from (void *) to (nsISupports *) thus causing a 
  // vtable problem on Windows, where we really didn't have the proper pointer
  // to an nsIInputStream, this ensures that we do 
#if 1
    nsISupports    * supports;
    nsIInputStream * inStr;

    nsresult rv = NS_NewIOFileStream(&supports, inFile, PR_RDONLY, 0700);

    *aResult = nsnull;
    if (NS_SUCCEEDED(rv)) {
      if (NS_SUCCEEDED(supports->QueryInterface(nsCOMTypeInfo<nsIInputStream>::GetIID(), (void**)&inStr))) {
        *aResult = inStr;
      }
      NS_RELEASE(supports);
    }
    return rv;
#else
    return NS_NewIOFileStream(aResult, inFile, PR_RDONLY, 0700);
#endif
}

//----------------------------------------------------------------------------------------
NS_COM nsresult NS_NewOutputConsoleStream(
    nsISupports** aResult)
// Factory method to get an nsOutputStream to the console.
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    FileImpl* stream = new FileImpl(PR_STDOUT);
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aResult = (nsISupports*)(void*)stream;
    return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_COM nsresult NS_NewTypicalOutputFileStream(
    nsISupports** aResult,
    const nsFileSpec& inFile
    /*default nsprMode= (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE)*/
    /*Default accessMode= 0700 (octal)*/)
// Factory method to get an nsOutputStream to a file - most common case.
//----------------------------------------------------------------------------------------
{
  // This QueryInterface was needed because NS_NewIOFileStream
  // does a cast from (void *) to (nsISupports *) thus causing a 
  // vtable problem on Windows, where we really didn't have the proper pointer
  // to an nsIOutputStream, this ensures that we do 
#if 1
/*    nsISupports     * supports;
    nsIOutputStream * outStr;

    nsresult rv = NS_NewIOFileStream(
        &supports,
        inFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0700);

    *aResult = nsnull;
    if (NS_SUCCEEDED(rv)) { 
      if (NS_SUCCEEDED(supports->QueryInterface(nsCOMTypeInfo<nsIOutputStream>::GetIID(), (void**)&outStr))) {
        *aResult = outStr;
      }
      NS_RELEASE(supports);
    }
    return rv;
    */

    nsCOMPtr<nsISupports> supports;
    nsIOutputStream * outStr;

    nsresult rv = NS_NewIOFileStream(
        getter_AddRefs(supports),
        inFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0700);

    *aResult = nsnull;
    if (NS_SUCCEEDED(rv)) { 
      if (NS_SUCCEEDED(supports->QueryInterface(nsCOMTypeInfo<nsIOutputStream>::GetIID(), (void**)&outStr))) {
        *aResult = outStr;
      }
    }
    return rv;
#else
    return NS_NewIOFileStream(
        aResult,
        inFile,
        (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE),
        0700);
#endif
}

//----------------------------------------------------------------------------------------
NS_COM nsresult NS_NewIOFileStream(
    nsISupports** aResult,
    const nsFileSpec& inFile,
    PRInt32 nsprMode /*default = (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE)*/,
    PRInt32 accessMode /*Default = 0700 (octal)*/)
    // Factory method to get an object that implements both nsIInputStream
    // and nsIOutputStream, associated with a file.
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (!aResult)
        return NS_ERROR_NULL_POINTER;

    FileImpl* stream = new FileImpl(inFile, nsprMode, accessMode);
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aResult = (nsISupports*)(void*)stream;
    return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_COM nsresult NS_NewTypicalIOFileStream(
    nsISupports** aResult,
    const nsFileSpec& inFile
    /*default nsprMode= (PR_RDWR | PR_CREATE_FILE)*/
    /*Default accessMode= 0700 (octal)*/)
    // Factory method to get an object that implements both nsIInputStream
    // and nsIOutputStream, associated with a single file.
//----------------------------------------------------------------------------------------
{
    return NS_NewIOFileStream(
        aResult,
        inFile,
        (PR_RDWR | PR_CREATE_FILE),
        0700);
}
