/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIBuffer.h"
#include "nsIBufferInputStream.h"
#include "nsIBufferOutputStream.h"
#include "nsAutoLock.h"

class nsBufferInputStream;

////////////////////////////////////////////////////////////////////////////////

class nsBufferInputStream : public nsIBufferInputStream
{
public:
    NS_DECL_ISUPPORTS

    // nsIBaseStream methods:
    NS_IMETHOD Close(void);

    // nsIInputStream methods:
    NS_IMETHOD GetLength(PRUint32 *aLength);
    NS_IMETHOD Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount); 

    // nsIBufferInputStream methods:
    NS_IMETHOD GetBuffer(nsIBuffer* *result);
    NS_IMETHOD Search(const char *forString, PRBool ignoreCase, PRBool *found, PRUint32 *offsetSearchedTo);
    NS_IMETHOD Fill(const char *buf, PRUint32 count, PRUint32 *_retval);
    NS_IMETHOD FillFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval);

    // nsBufferInputStream methods:
    nsBufferInputStream(nsIBuffer* buf, PRBool blocking);
    virtual ~nsBufferInputStream();

    PRUint32 ReadableAmount() {
        nsresult rv;
        PRUint32 amt;
        const char* buf;
        rv = mBuffer->GetReadSegment(0, &buf, &amt); // should never fail
        NS_ASSERTION(NS_SUCCEEDED(rv), "GetInputBuffer failed");
        return amt;
    }

    nsresult SetEOF();
    nsresult Fill();

protected:
    nsIBuffer*          mBuffer;
    PRBool              mBlocking;
};

////////////////////////////////////////////////////////////////////////////////

class nsBufferOutputStream : public nsIBufferOutputStream
{
public:
    NS_DECL_ISUPPORTS

    // nsIBaseStream methods:
    NS_IMETHOD Close(void);

    // nsIOutputStream methods:
    NS_IMETHOD Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount); 
    NS_IMETHOD Flush(void);

    // nsIBufferOutputStream methods:
    NS_IMETHOD GetBuffer(nsIBuffer * *aBuffer);
    NS_IMETHOD WriteFrom(nsIInputStream* fromStream, PRUint32 aCount,
                         PRUint32 *aWriteCount);

    // nsBufferOutputStream methods:
    nsBufferOutputStream(nsIBuffer* buf, PRBool blocking);
    virtual ~nsBufferOutputStream();

protected:
    nsIBuffer*  mBuffer;
    PRBool      mBlocking;
};

////////////////////////////////////////////////////////////////////////////////
// nsBufferInputStream methods:
////////////////////////////////////////////////////////////////////////////////

nsBufferInputStream::nsBufferInputStream(nsIBuffer* buf, PRBool blocking)
    : mBuffer(buf), mBlocking(blocking)
{
    NS_INIT_REFCNT();
    NS_ADDREF(mBuffer);
}

nsBufferInputStream::~nsBufferInputStream()
{
    (void)Close();
}

NS_IMPL_ADDREF(nsBufferInputStream);
NS_IMPL_RELEASE(nsBufferInputStream);

NS_IMETHODIMP
nsBufferInputStream::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (aInstancePtr == nsnull)
        return NS_ERROR_NULL_POINTER;
    if (aIID.Equals(nsIBufferInputStream::GetIID()) ||
        aIID.Equals(nsIInputStream::GetIID()) ||
        aIID.Equals(nsIBaseStream::GetIID()) ||
        aIID.Equals(nsISupports::GetIID())) {
        *aInstancePtr = this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsBufferInputStream::Close(void)
{
    nsresult rv;

#ifdef DEBUG
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    rv = mBuffer->ReaderClosed();
    if (NS_FAILED(rv)) return rv;
    if (mBlocking) {
        nsAutoCMonitor mon(mBuffer);
        rv = mon.Notify();   // wake up the writer
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBufferInputStream::GetLength(PRUint32 *aLength)
{
#ifdef DEBUG
    nsresult rv;
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    return mBuffer->GetReadableAmount(aLength);
}

NS_IMETHODIMP
nsBufferInputStream::Read(char* aBuf, PRUint32 aCount, PRUint32 *aReadCount)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    *aReadCount = 0; int n = 0, m=0;
    while (aCount > 0) {
        PRUint32 amt;
        rv = mBuffer->Read(aBuf, aCount, &amt);n=rv;
        if (rv == NS_BASE_STREAM_EOF) {
            rv = (*aReadCount == 0) ? rv : NS_OK;
            break;
        }
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
        if (NS_FAILED(rv)) break;

        if (amt == 0) {
            rv = Fill();m=rv;
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
            if (NS_FAILED(rv)) break;
        }
        else {
            *aReadCount += amt;
            aBuf += amt;
            aCount -= amt;
        }
    }
    if (rv == NS_BASE_STREAM_EOF) {
        // all we're ever going to get -- so wake up anyone in Flush
#ifdef DEBUG
        PRUint32 amt;
        const char* buf;
        nsresult rv2 = mBuffer->GetReadSegment(0, &buf, &amt);
        NS_ASSERTION(rv2 == NS_BASE_STREAM_EOF ||
                     (NS_SUCCEEDED(rv2) && amt == 0), "Read failed");
#endif
        nsAutoCMonitor mon(mBuffer);
        mon.Notify();   // wake up writer
    }
    return (*aReadCount == 0) ? rv : NS_OK;
}

NS_IMETHODIMP
nsBufferInputStream::GetBuffer(nsIBuffer* *result)
{
    *result = mBuffer;
    NS_ADDREF(mBuffer);
    return NS_OK;
}

NS_IMETHODIMP
nsBufferInputStream::Search(const char *forString, PRBool ignoreCase, PRBool *found, PRUint32 *offsetSearchedTo)
{
#ifdef DEBUG
    nsresult rv;
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    return mBuffer->Search(forString, ignoreCase, found, offsetSearchedTo);
}

NS_IMETHODIMP
nsBufferInputStream::Fill(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    *aWriteCount = 0;
    while (aCount > 0) {
        PRUint32 amt;
        rv = mBuffer->Write(aBuf, aCount, &amt);
        if (rv == NS_BASE_STREAM_EOF)
            return *aWriteCount > 0 ? NS_OK : rv;
        if (NS_FAILED(rv)) return rv;
        if (amt == 0) {
            rv = Fill();
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                return *aWriteCount > 0 ? NS_OK : rv;
            if (NS_FAILED(rv)) return rv;
        }
        else {
            aBuf += amt;
            aCount -= amt;
            *aWriteCount += amt;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsBufferInputStream::FillFrom(nsIInputStream *fromStream, PRUint32 aCount, PRUint32 *aWriteCount)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    *aWriteCount = 0;
    while (aCount > 0) {
        PRUint32 amt;
        rv = mBuffer->WriteFrom(fromStream, aCount, &amt);
        if (rv == NS_BASE_STREAM_EOF)
            return *aWriteCount > 0 ? NS_OK : rv;
        if (NS_FAILED(rv)) return rv;
        if (amt == 0) {
            rv = Fill();
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                return *aWriteCount > 0 ? NS_OK : rv;
            if (NS_FAILED(rv)) return rv;
        }
        else {
            aCount -= amt;
            *aWriteCount += amt;
        }
    }
    return rv;
}

nsresult
nsBufferInputStream::SetEOF()
{
    nsresult rv;

#ifdef DEBUG
    PRBool closed;
    rv = mBuffer->GetReaderClosed(&closed);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !closed, "state change error");
#endif

    if (mBlocking) {
        nsAutoCMonitor mon(mBuffer);
        rv = mBuffer->SetEOF();
        if (NS_FAILED(rv)) return rv;
        rv = mon.Notify();   // wake up the writer
        if (NS_FAILED(rv)) return rv;
    }
    else {
        rv = mBuffer->SetEOF();
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

nsresult
nsBufferInputStream::Fill()
{
    nsresult rv;
    if (mBlocking) {
        nsAutoCMonitor mon(mBuffer);

        // check read buffer again while in the monitor
        PRUint32 amt;
        const char* buf;
        rv = mBuffer->GetReadSegment(0, &buf, &amt);
        if (rv == NS_BASE_STREAM_EOF) return rv;
        if (NS_SUCCEEDED(rv) && amt > 0) return NS_OK;

        // else notify the writer and wait
        rv = mon.Notify();
        if (NS_FAILED(rv)) return rv;   // interrupted
        rv = mon.Wait();
        if (NS_FAILED(rv)) return rv;   // interrupted

#ifdef DEBUG
        rv = mBuffer->GetReadSegment(0, &buf, &amt);
        NS_ASSERTION(rv == NS_BASE_STREAM_EOF ||
                     (NS_SUCCEEDED(rv) && amt > 0), "Fill failed");
#endif
    }
    else {
        return NS_BASE_STREAM_WOULD_BLOCK;
    }
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_NewBufferInputStream(nsIBufferInputStream* *result,
                        nsIBuffer* buffer, PRBool blocking)
{
    nsBufferInputStream* str = new nsBufferInputStream(buffer, blocking);
    if (str == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(str);
    *result = str;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsBufferOutputStream methods:
////////////////////////////////////////////////////////////////////////////////

nsBufferOutputStream::nsBufferOutputStream(nsIBuffer* buf, PRBool blocking)
    : mBuffer(buf), mBlocking(blocking)
{
    NS_INIT_REFCNT();
    NS_ADDREF(mBuffer);
}

nsBufferOutputStream::~nsBufferOutputStream()
{
    (void)Close();
}

NS_IMPL_ADDREF(nsBufferOutputStream);
NS_IMPL_RELEASE(nsBufferOutputStream);

NS_IMETHODIMP
nsBufferOutputStream::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    if (aInstancePtr == nsnull)
        return NS_ERROR_NULL_POINTER;
    if (aIID.Equals(nsIBufferOutputStream::GetIID()) ||
        aIID.Equals(nsIOutputStream::GetIID()) ||
        aIID.Equals(nsIBaseStream::GetIID()) ||
        aIID.Equals(nsISupports::GetIID())) {
        *aInstancePtr = this;
        NS_ADDREF_THIS();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsBufferOutputStream::Close(void)
{
    nsresult rv;
    rv = mBuffer->SetEOF();
    if (NS_FAILED(rv)) return rv;

    if (mBlocking) {
        nsAutoCMonitor mon(mBuffer);
        rv = mon.Notify();   // wake up the writer
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBufferOutputStream::Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    PRBool eof;
    rv = mBuffer->AtEOF(&eof);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !eof, "state change error");
#endif

    *aWriteCount = 0;
    while (aCount > 0) {
        PRUint32 amt;
        rv = mBuffer->Write(aBuf, aCount, &amt);
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
        if (NS_FAILED(rv)) break;

        if (amt == 0) {
            rv = Flush();
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
            if (NS_FAILED(rv)) break;
        }
        else {
            aBuf += amt;
            aCount -= amt;
            *aWriteCount += amt;
        }
    }
    if (rv == NS_BASE_STREAM_FULL && mBlocking) {
        // all we're going to get for now -- so wake up anyone in Flush
#ifdef DEBUG
        PRUint32 amt;
        const char* buf;
        nsresult rv2 = mBuffer->GetReadSegment(0, &buf, &amt);
        NS_ASSERTION(rv2 == NS_BASE_STREAM_EOF ||
                     (NS_SUCCEEDED(rv2) && amt > 0), "Write failed");
#endif
        nsAutoCMonitor mon(mBuffer);
        mon.Notify();   // wake up reader
    }
    return (*aWriteCount == 0) ? rv : NS_OK;
}

NS_IMETHODIMP
nsBufferOutputStream::WriteFrom(nsIInputStream* fromStream, PRUint32 aCount,
                                PRUint32 *aWriteCount)
{
    nsresult rv = NS_OK;

#ifdef DEBUG
    PRBool eof;
    rv = mBuffer->AtEOF(&eof);
    NS_ASSERTION(NS_SUCCEEDED(rv) && !eof, "state change error");
#endif

    *aWriteCount = 0;
    while (aCount > 0) {
        PRUint32 amt;
        rv = mBuffer->WriteFrom(fromStream, aCount, &amt);
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
        if (NS_FAILED(rv)) break;

        if (amt == 0) {
            rv = Flush();
            if (rv == NS_BASE_STREAM_WOULD_BLOCK) break;
            if (NS_FAILED(rv)) break;
        }
        else {
            aCount -= amt;
            *aWriteCount += amt;
        }
    }
    if (rv == NS_BASE_STREAM_WOULD_BLOCK && mBlocking) {
        // all we're going to get for now -- so wake up anyone in Flush
#ifdef DEBUG
        PRUint32 amt;
        const char* buf;
        nsresult rv2 = mBuffer->GetReadSegment(0, &buf, &amt);
        NS_ASSERTION(rv2 == NS_BASE_STREAM_EOF ||
                     (NS_SUCCEEDED(rv2) && amt > 0), "WriteFrom failed");
#endif
        nsAutoCMonitor mon(mBuffer);
        mon.Notify();   // wake up reader
    }
    return (*aWriteCount == 0) ? rv : NS_OK;
}

NS_IMETHODIMP
nsBufferOutputStream::Flush(void)
{
    nsresult rv = NS_OK;
    if (mBlocking) {
        nsresult rv;
        nsAutoCMonitor mon(mBuffer);

        // check write buffer again while in the monitor
        PRUint32 amt;
        const char* buf;
        rv = mBuffer->GetReadSegment(0, &buf, &amt);
        if (rv == NS_BASE_STREAM_EOF) return NS_OK;
        if (amt == 0) return NS_OK;

        // else notify the reader and wait
        rv = mon.Notify();
        if (NS_FAILED(rv)) return rv;   // interrupted
        rv = mon.Wait();
        if (NS_FAILED(rv)) return rv;   // interrupted

#ifdef DEBUG
        rv = mBuffer->GetReadSegment(0, &buf, &amt);
        NS_ASSERTION(rv == NS_BASE_STREAM_EOF ||
                     (NS_SUCCEEDED(rv) && amt == 0), "Flush failed");
#endif
    }
    else {
        return NS_BASE_STREAM_WOULD_BLOCK;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBufferOutputStream::GetBuffer(nsIBuffer * *aBuffer)
{
    *aBuffer = mBuffer;
    NS_ADDREF(mBuffer);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_NewBufferOutputStream(nsIBufferOutputStream* *result,
                         nsIBuffer* buffer, PRBool blocking)
{
    nsBufferOutputStream* ostr = new nsBufferOutputStream(buffer, blocking);
    if (ostr == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(ostr);
    *result = ostr;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_COM nsresult
NS_NewPipe(nsIBufferInputStream* *inStrResult,
           nsIBufferOutputStream* *outStrResult,
           PRUint32 growBySize, PRUint32 maxSize,
           PRBool blocking, nsIBufferObserver* observer)
{
    nsresult rv;
    nsIBufferInputStream* inStr = nsnull;
    nsIBufferOutputStream* outStr = nsnull;
    nsIBuffer* buf = nsnull;
    
    rv = NS_NewPageBuffer(&buf, growBySize, maxSize, observer);
    if (NS_FAILED(rv)) goto error;

    rv = NS_NewBufferInputStream(&inStr, buf, blocking);
    if (NS_FAILED(rv)) goto error;
    
    rv = NS_NewBufferOutputStream(&outStr, buf, blocking);
    if (NS_FAILED(rv)) goto error;
    
    NS_RELEASE(buf);
    *inStrResult = inStr;
    *outStrResult = outStr;
    return NS_OK;

  error:
    NS_IF_RELEASE(inStr);
    NS_IF_RELEASE(outStr);
    NS_IF_RELEASE(buf);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

