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
#include "nsIThread.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsAutoLock.h"
#include "nsIPageManager.h"
#include "nsCRT.h"
#include "prprf.h"
#include "prmem.h"
#include "prinrval.h"
#include <stdio.h>

class Reader : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        nsAutoMonitor mon(this);

        char buf[100];
        PRUint32 readCnt;
        char* readBuf;
        rv = mReadBuffer->GetReadBuffer(0, &readBuf, &readCnt);
        if (NS_FAILED(rv)) return rv;
        while (!(mDone && readCnt == 0)) {
            rv = mReadBuffer->Read(buf, 99, &readCnt);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                return rv;
            }
            if (readCnt == 0) {
                if (mDone)
                    break;
                mon.Notify(); // wake up writer
                mon.Wait();   // wait for more
                rv = mReadBuffer->GetReadBuffer(0, &readBuf, &readCnt);
                if (NS_FAILED(rv)) return rv;
            }
            else {
//                buf[readCnt] = 0;
//                printf(buf);
                mBytesRead += readCnt;
            }
        }
//        printf("reader done\n");
        return rv;
    }

    Reader(nsIBuffer* readBuffer)
        : mReadBuffer(readBuffer), mBytesRead(0), mDone(PR_FALSE) {
        NS_INIT_REFCNT();
        NS_ADDREF(mReadBuffer);
    }

    virtual ~Reader() {
        NS_RELEASE(mReadBuffer);
        printf("bytes read = %d\n", mBytesRead);
    }

    void SetEOF() {
        mDone = PR_TRUE;
    }

    PRUint32 GetBytesRead() { return mBytesRead; }

protected:
    nsIBuffer*  mReadBuffer;
    PRUint32    mBytesRead;
    PRBool      mDone;
};

NS_IMPL_ISUPPORTS(Reader, nsIRunnable::GetIID());

////////////////////////////////////////////////////////////////////////////////

#define ITERATIONS      20000

nsresult
WriteMessages(nsIBuffer* buffer)
{
    nsresult rv;
    nsIThread* thread;
    Reader* reader = new Reader(buffer);
    NS_ADDREF(reader);
    rv = NS_NewThread(&thread, reader);
    if (NS_FAILED(rv)) {
        printf("failed to create thread\n");
        return rv;
    }

    char* mem = nsnull;
    char* buf = nsnull;
    PRUint32 total = 0;
    PRUint32 cnt = 0;
    PRUint32 bufLen = 0;
    PRIntervalTime start = PR_IntervalNow();
    for (PRUint32 i = 0; i < ITERATIONS;) {
        if (bufLen == 0) {
            PR_FREEIF(mem);
            mem = PR_smprintf("%d. My hovercraft is full of eels.\n", i);
            buf = mem;
            NS_ASSERTION(buf, "out of memory");
            bufLen = nsCRT::strlen(buf);
            i++;
        }
        rv = buffer->Write(buf, bufLen, &cnt);
        if (NS_FAILED(rv)) {
            printf("failed to write %d\n", i);
            return rv;
        }
        if (cnt == 0) {
            nsAutoMonitor mon(reader);
            mon.Notify();       // wake up reader
            mon.Wait();         // and wait for reader to read all
        }
        else {
            total += cnt;
            buf += cnt;
            bufLen -= cnt;
        }
    }

    PR_FREEIF(mem);
    {
        reader->SetEOF();
        nsAutoMonitor mon(reader);
        mon.Notify();       // wake up reader
    }

    if (NS_FAILED(rv)) return rv;
    rv = thread->Join();
    PRIntervalTime end = PR_IntervalNow();
    if (NS_FAILED(rv)) return rv;
    printf("writer done: %d bytes %d ms\n", total,
           PR_IntervalToMilliseconds(end - start));
    NS_ASSERTION(reader->GetBytesRead() == total, "didn't read everything");

    NS_RELEASE(reader);
    NS_RELEASE(thread);
    NS_RELEASE(buffer);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

nsresult
TestMallocBuffers(PRUint32 growByPages, PRUint32 pageCount)
{
    nsresult rv;
    nsIBuffer* buffer;
    printf("Malloc Buffer Test: %d pages, grow by %d pages\n",
           pageCount, growByPages);
    rv = NS_NewBuffer(&buffer,
                      NS_PAGEMGR_PAGE_SIZE * growByPages, 
                      NS_PAGEMGR_PAGE_SIZE * pageCount);
    if (NS_FAILED(rv)) {
        printf("failed to create buffer\n");
        return rv;
    }

    rv = WriteMessages(buffer);
    return rv;
}

nsresult
TestPageBuffers(PRUint32 growByPages, PRUint32 pageCount)
{
    nsresult rv;
    nsIBuffer* buffer;
    printf("Page Buffer Test: %d pages, grow by %d pages\n",
           pageCount, growByPages);
    rv = NS_NewPageBuffer(&buffer,
                          NS_PAGEMGR_PAGE_SIZE * growByPages,
                          NS_PAGEMGR_PAGE_SIZE * pageCount);
    if (NS_FAILED(rv)) {
        printf("failed to create buffer\n");
        return rv;
    }

    rv = WriteMessages(buffer);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

void
TestSearch(const char* delim, PRUint32 segDataSize)
{
    nsresult rv;
    nsIBuffer* buffer;
    // need at least 2 segments to test boundary conditions:
    PRUint32 bufDataSize = segDataSize * 2;
    PRUint32 segSize = segDataSize + nsIBuffer::SEGMENT_OVERHEAD;
    PRUint32 bufSize = segSize * 2;
    rv = NS_NewBuffer(&buffer, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NewBuffer failed");

    PRUint32 i, amt;
    PRUint32 delimLen = nsCRT::strlen(delim);
    for (i = 0; i < bufDataSize; i++) {
        // first fill the buffer
        for (PRUint32 j = 0; j < i; j++) {
            rv = buffer->Write("-", 1, &amt);
            NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
        }
        rv = buffer->Write(delim, delimLen, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Write failed");
        if (i + amt < bufDataSize) {
            for (PRUint32 j = i + amt; j < bufDataSize; j++) {
                rv = buffer->Write("+", 1, &amt);
                NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
            }
        }
        
        // now search for the delimiter
        PRBool found;
        PRUint32 offset;
        rv = buffer->Search(delim, PR_FALSE, &found, &offset);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Search failed");

        // print the results
        char* bufferContents = new char[bufDataSize + 1];
        rv = buffer->Read(bufferContents, bufDataSize, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv) && amt == bufDataSize, "Read failed");
        bufferContents[bufDataSize] = '\0';
        printf("Buffer: %s\nDelim: %s %s offset: %d\n", bufferContents,
               delim, (found ? "found" : "not found"), offset);
    }
}

////////////////////////////////////////////////////////////////////////////////

int
main()
{
    nsresult rv;
    nsIServiceManager* servMgr;

    rv = NS_InitXPCOM(&servMgr);
    if (NS_FAILED(rv)) return rv;

    // XXX why do I have to do this?!
    rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup,
                                          "components");
    if (NS_FAILED(rv)) return rv;

    rv = TestMallocBuffers(1, 1);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestMallocBuffers failed");

    rv = TestPageBuffers(1, 1);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPageBuffers failed");

    rv = TestMallocBuffers(1, 10);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestMallocBuffers failed");

    rv = TestPageBuffers(1, 10);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPageBuffers failed");

    rv = TestMallocBuffers(5, 10);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestMallocBuffers failed");

    rv = TestPageBuffers(5, 10);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPageBuffers failed");

    TestSearch("foo", 8);
    TestSearch("bar", 6);
    TestSearch("baz", 2);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
