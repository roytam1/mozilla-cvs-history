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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIThread.h"
#include "nsIRunnable.h"
#if 0   // obsolete old implementation
#include "nsIByteBufferInputStream.h"
#endif
#ifdef OLD_BUFFERS
#include "nsIBuffer.h"
#endif
#include "nsIBufferInputStream.h"
#include "nsIBufferOutputStream.h"
#include "nsIServiceManager.h"
#include "prprf.h"
#include "prinrval.h"
#include "plstr.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include <stdio.h>
#include "nsIPipe.h"    // new implementation
#include "nsAutoLock.h"
#include <stdlib.h>     // for rand

#define KEY             0xa7
#define ITERATIONS      33333
char kTestPattern[] = "My hovercraft is full of eels.\n";

PRBool gTrace = PR_FALSE;

class nsReceiver : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        char buf[101];
        PRUint32 count;
        PRIntervalTime start = PR_IntervalNow();
        while (PR_TRUE) {
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {
//                printf("EOF count = %d\n", mCount);
                break;
            }

            if (gTrace) {
                printf("read:  ");
                buf[count] = '\0';
                printf(buf);
                printf("\n");
            }
            mCount += count;
        }
        PRIntervalTime end = PR_IntervalNow();
        printf("read  %d bytes, time = %dms\n", mCount,
               PR_IntervalToMilliseconds(end - start));
        return rv;
    }

    nsReceiver(nsIInputStream* in) : mIn(in), mCount(0) {
        NS_INIT_REFCNT();
        NS_ADDREF(in);
    }

    virtual ~nsReceiver() {
        NS_RELEASE(mIn);
    }

    PRUint32 GetBytesRead() { return mCount; }

protected:
    nsIInputStream*     mIn;
    PRUint32            mCount;
};

NS_IMPL_ISUPPORTS(nsReceiver, NS_GET_IID(nsIRunnable));

nsresult
TestPipe(nsIInputStream* in, nsIOutputStream* out)
{
    nsresult rv;
    nsIThread* thread;
    nsReceiver* receiver = new nsReceiver(in);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&thread, receiver, 0, PR_JOINABLE_THREAD);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    PRIntervalTime start = PR_IntervalNow();
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        rv = out->Write(buf, nsCRT::strlen(buf), &writeCount);
        if (gTrace) {
            printf("wrote: ");
            for (PRUint32 j = 0; j < writeCount; j++) {
                putc(buf[j], stdout);
            }
            printf("\n");
        }
        PR_smprintf_free(buf);
        if (NS_FAILED(rv)) return rv;
        total += writeCount;
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    PRIntervalTime end = PR_IntervalNow();

    thread->Join();

    printf("wrote %d bytes, time = %dms\n", total,
           PR_IntervalToMilliseconds(end - start));
    NS_ASSERTION(receiver->GetBytesRead() == total, "didn't read everything");

    NS_RELEASE(thread);
    NS_RELEASE(receiver);

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

class nsShortReader : public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD Run() {
        nsresult rv;
        char buf[101];
        PRUint32 count;
        PRUint32 total = 0;
        while (PR_TRUE) {
            rv = mIn->Read(buf, 100, &count);
            if (NS_FAILED(rv)) {
                printf("read failed\n");
                break;
            }
            if (count == 0) {
                break;
            }
            buf[count] = '\0';
            if (gTrace)
                printf("read %d bytes: %s\n", count, buf);
            Received(count);
            total += count;
        }
        printf("read  %d bytes\n", total);
        return rv;
    }

    nsShortReader(nsIInputStream* in) : mIn(in), mReceived(0) {
        NS_INIT_REFCNT();
        NS_ADDREF(in);
    }

    virtual ~nsShortReader() {
        NS_RELEASE(mIn);
    }

    void Received(PRUint32 count) {
        nsAutoCMonitor mon(this);
        mReceived += count;
        mon.Notify();
    }

    PRUint32 WaitForReceipt() {
        PRUint32 result = mReceived;
        nsAutoCMonitor mon(this);
        if (mReceived == 0) {
            mon.Wait();
            NS_ASSERTION(mReceived >= 0, "failed to receive");
            result = mReceived;
        }
        mReceived = 0;
        return result;
    }

protected:
    nsIInputStream*     mIn;
    PRUint32            mReceived;
};

NS_IMPL_ISUPPORTS(nsShortReader, NS_GET_IID(nsIRunnable));

nsresult
TestShortWrites(nsIInputStream* in, nsIOutputStream* out)
{
    nsresult rv;
    nsIThread* thread;
    nsShortReader* receiver = new nsShortReader(in);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&thread, receiver, 0, PR_JOINABLE_THREAD);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        PRUint32 len = nsCRT::strlen(buf);
        len = len * rand() / RAND_MAX;
        len = PR_MAX(1, len);
        rv = out->Write(buf, len, &writeCount);
        if (NS_FAILED(rv)) return rv;
        NS_ASSERTION(writeCount == len, "didn't write enough");
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);
        PR_smprintf_free(buf);
        out->Flush();
        PRUint32 received = receiver->WaitForReceipt();
        NS_ASSERTION(received == writeCount, "received wrong amount");
    }
    rv = out->Close();
    if (NS_FAILED(rv)) return rv;

    thread->Join();
    printf("wrote %d bytes\n", total);

    NS_RELEASE(thread);
    NS_RELEASE(receiver);

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

class nsPipeObserver : public nsIPipeObserver {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD OnFull(nsIPipe *pipe) {
        printf("OnFull pipe=%p\n", pipe);
        return NS_OK;
    }

    NS_IMETHOD OnWrite(nsIPipe *pipe, PRUint32 amount) {
        printf("OnWrite pipe=%p amount=%d\n", pipe, amount);
        return NS_OK;
    }

    NS_IMETHOD OnEmpty(nsIPipe *pipe) {
        printf("OnEmpty pipe=%p\n", pipe);
        return NS_OK;
    }

    nsPipeObserver() { NS_INIT_REFCNT(); }
    virtual ~nsPipeObserver() {}
};

NS_IMPL_ISUPPORTS(nsPipeObserver, NS_GET_IID(nsIPipeObserver));

nsresult
TestPipeObserver()
{
    nsresult rv;
    nsPipeObserver* obs = new nsPipeObserver();
    if (obs == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(obs);

    printf("TestPipeObserver: OnWrite and OnFull should be called once, OnEmpty should be called twice.\n");
    nsIBufferInputStream* in;
    nsIBufferOutputStream* out;
    rv = NS_NewPipe(&in, &out, obs, 20, 20);
    if (NS_FAILED(rv)) return rv;

    rv = in->SetNonBlocking(PR_TRUE);
    if (NS_FAILED(rv)) return rv;
    rv = out->SetNonBlocking(PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    char buf[] = "puirt a beul: a style of Gaelic vocal music intended for dancing.";
    PRUint32 cnt;
    // this should print OnWrite message:
    rv = out->Write(buf, 20, &cnt);
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION(cnt == 20, "Write failed");

    // this should print OnFull message:
    rv = out->Write(buf + 20, 1, &cnt);
    if (NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK) return rv;
    NS_ASSERTION(cnt == 0 && rv == NS_BASE_STREAM_WOULD_BLOCK, "Write failed");

    char buf2[20];
    rv = in->Read(buf2, 20, &cnt);
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION(cnt == 20, "Read failed");
    NS_ASSERTION(nsCRT::strncmp(buf, buf2, 20) == 0, "Read wrong stuff");

    // this should print OnEmpty message:
    rv = in->Read(buf2, 1, &cnt);
    if (NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK) return rv;
    NS_ASSERTION(cnt == 0 && rv == NS_BASE_STREAM_WOULD_BLOCK, "Read failed");

    NS_RELEASE(obs);
    NS_RELEASE(in);
    NS_RELEASE(out);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

class nsPump : public nsIPipeObserver, public nsIRunnable {
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD OnFull(nsIPipe *pipe) {
        printf("OnFull pipe=%p\n", pipe);
        nsAutoCMonitor mon(this);
        mon.Notify();
        return NS_OK;
    }

    NS_IMETHOD OnWrite(nsIPipe *pipe, PRUint32 amount) {
        printf("OnWrite pipe=%p amount=%d\n", pipe, amount);
        return NS_OK;
    }

    NS_IMETHOD OnEmpty(nsIPipe *pipe) {
        printf("OnEmpty pipe=%p\n", pipe);
        nsAutoCMonitor mon(this);
        mon.Notify();
        return NS_OK;
    }

    NS_IMETHOD Run() {
        nsresult rv;
        PRUint32 count;
        while (PR_TRUE) {
            nsAutoCMonitor mon(this);
            rv = mOut->WriteFrom(mIn, -1, &count);
            if (NS_FAILED(rv)) {
                printf("Write failed\n");
                break;
            }
            if (count == 0) {
                printf("EOF count = %d\n", mCount);
                break;
            }

            if (gTrace) {
                printf("Wrote: %d\n", count);
            }
            mCount += count;
        }
        mOut->Close();
        return rv;
    }

    nsPump(nsIBufferInputStream* in,
           nsIBufferOutputStream* out)
        : mIn(in), mOut(out), mCount(0) {
        NS_INIT_REFCNT();
    }

    virtual ~nsPump() {
    }

protected:
    nsCOMPtr<nsIBufferInputStream>      mIn;
    nsCOMPtr<nsIBufferOutputStream>     mOut;
    PRUint32                            mCount;
};

NS_IMPL_ISUPPORTS2(nsPump, nsIPipeObserver, nsIRunnable)

nsresult
TestChainedPipes()
{
    nsresult rv;
    printf("TestChainedPipes\n");

    nsIBufferInputStream* in1;
    nsIBufferOutputStream* out1;
    rv = NS_NewPipe(&in1, &out1, nsnull, 20, 1999);
    if (NS_FAILED(rv)) return rv;

    nsIBufferInputStream* in2;
    nsIBufferOutputStream* out2;
    rv = NS_NewPipe(&in2, &out2, nsnull, 200, 401);
    if (NS_FAILED(rv)) return rv;

    nsIThread* thread;
    nsPump* pump = new nsPump(in1, out2);
    if (pump == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(pump);

    rv = NS_NewThread(&thread, pump, 0, PR_JOINABLE_THREAD);
    if (NS_FAILED(rv)) return rv;

    nsIThread* receiverThread;
    nsReceiver* receiver = new nsReceiver(in2);
    if (receiver == nsnull) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(receiver);

    rv = NS_NewThread(&receiverThread, receiver, 0, PR_JOINABLE_THREAD);
    if (NS_FAILED(rv)) return rv;

    PRUint32 total = 0;
    for (PRUint32 i = 0; i < ITERATIONS; i++) {
        PRUint32 writeCount;
        char* buf = PR_smprintf("%d %s", i, kTestPattern);
        PRUint32 len = nsCRT::strlen(buf);
        len = len * rand() / RAND_MAX;
        len = PR_MAX(1, len);
        rv = out1->Write(buf, len, &writeCount);
        if (NS_FAILED(rv)) return rv;
        NS_ASSERTION(writeCount == len, "didn't write enough");
        total += writeCount;

        if (gTrace)
            printf("wrote %d bytes: %s\n", writeCount, buf);
        //out1->Flush();  // wakes up the pump

        PR_smprintf_free(buf);
    }
    printf("wrote total of %d bytes\n", total);
    rv = out1->Close();
    if (NS_FAILED(rv)) return rv;

    thread->Join();
    receiverThread->Join();

    NS_RELEASE(thread);
    NS_RELEASE(pump);
    NS_RELEASE(receiverThread);
    NS_RELEASE(receiver);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

void
RunTests(PRUint32 segSize, PRUint32 segCount)
{
    nsresult rv;
    nsIBufferInputStream* in;
    nsIBufferOutputStream* out;
    PRUint32 bufSize;
#ifdef OLD_BUFFERS
    bufSize = (segSize + nsIBuffer::SEGMENT_OVERHEAD) * segCount;
    printf("Testing Old Pipes: segment size %d buffer size %d\n", segSize, segSize * segCount);

    printf("Testing long writes...\n");
    rv = NS_NewPipe(&in, &out, segSize + nsIBuffer::SEGMENT_OVERHEAD, bufSize, PR_TRUE, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestPipe(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");

    printf("Testing short writes...\n");
    rv = NS_NewPipe(&in, &out, segSize + nsIBuffer::SEGMENT_OVERHEAD, bufSize, PR_TRUE, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestShortWrites(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");
#endif
    bufSize = segSize * segCount;
    printf("Testing New Pipes: segment size %d buffer size %d\n", segSize, bufSize);

    printf("Testing long writes...\n");
    rv = NS_NewPipe(&in, &out, nsnull, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestPipe(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");

    printf("Testing short writes...\n");
    rv = NS_NewPipe(&in, &out, nsnull, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    rv = TestShortWrites(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipe failed");
}

////////////////////////////////////////////////////////////////////////////////

void
TestSearch(const char* delim, PRUint32 segSize)
{
    nsresult rv;
    // need at least 2 segments to test boundary conditions:
    PRUint32 bufDataSize = segSize * 2;
    PRUint32 bufSize = segSize * 2;
    nsIBufferInputStream* in;
    nsIBufferOutputStream* out;
    rv = NS_NewPipe(&in, &out, nsnull, segSize, bufSize);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewPipe failed");
    out->SetNonBlocking(PR_TRUE);

    PRUint32 i, j, amt;
    PRUint32 delimLen = nsCRT::strlen(delim);
    for (i = 0; i < bufDataSize; i++) {
        // first fill the buffer
        for (j = 0; j < i; j++) {
            rv = out->Write("-", 1, &amt);
            NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
        }
        rv = out->Write(delim, delimLen, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Write failed");
        if (i + amt < bufDataSize) {
            for (j = i + amt; j < bufDataSize; j++) {
                rv = out->Write("+", 1, &amt);
                NS_ASSERTION(NS_SUCCEEDED(rv) && amt == 1, "Write failed");
            }
        }
        
        // now search for the delimiter
        PRBool found;
        PRUint32 offset;
        rv = in->Search(delim, PR_FALSE, &found, &offset);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Search failed");

        // print the results
        char* bufferContents = new char[bufDataSize + 1];
        rv = in->Read(bufferContents, bufDataSize, &amt);
        NS_ASSERTION(NS_SUCCEEDED(rv) && amt == bufDataSize, "Read failed");
        bufferContents[bufDataSize] = '\0';
        printf("Buffer: %s\nDelim: %s %s offset: %d\n", bufferContents,
               delim, (found ? "found" : "not found"), offset);
    }
    NS_RELEASE(in);
    NS_RELEASE(out);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
extern NS_COM void
TestSegmentedBuffer();
#endif

int
main(int argc, char* argv[])
{
    nsresult rv;
    nsIServiceManager* servMgr;

    rv = NS_InitXPCOM(&servMgr, NULL, NULL);
    if (NS_FAILED(rv)) return rv;

    if (argc > 1 && nsCRT::strcmp(argv[1], "-trace") == 0)
        gTrace = PR_TRUE;

#ifdef DEBUG
    TestSegmentedBuffer();
#endif

#if 0   // obsolete old implementation
    rv = NS_NewPipe(&in, &out, PR_TRUE, 4096 * 4);
    if (NS_FAILED(rv)) {
        printf("NewPipe failed\n");
        return -1;
    }

    rv = TestPipe(in, out);
    NS_RELEASE(in);
    NS_RELEASE(out);
    if (NS_FAILED(rv)) {
        printf("TestPipe failed\n");
        return -1;
    }
#endif
    TestSearch("foo", 8);
    TestSearch("bar", 6);
    TestSearch("baz", 2);

    rv = TestPipeObserver();
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestPipeObserver failed");
    rv = TestChainedPipes();
    NS_ASSERTION(NS_SUCCEEDED(rv), "TestChainedPipes failed");
    RunTests(16, 1);
    RunTests(4096, 16);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
