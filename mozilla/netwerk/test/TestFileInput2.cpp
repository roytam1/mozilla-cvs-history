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

#include "nsIServiceManager.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsISupportsArray.h"
#include "prinrval.h"
#include "nsIFileStreams.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include "prprf.h"
#include "nsAutoLock.h"

class nsTimeSampler {
public:
    nsTimeSampler();
    void Reset();
    void StartTime();
    void EndTime();
    void AddTime(PRIntervalTime time);
    PRIntervalTime LastInterval() { return mLastInterval; }
    char* PrintStats();
protected:
    PRIntervalTime      mStartTime;
    double              mSquares;
    double              mTotalTime;
    PRUint32            mCount;
    PRIntervalTime      mLastInterval;
};

nsTimeSampler::nsTimeSampler()
{
    Reset();
}

void
nsTimeSampler::Reset()
{
    mStartTime = 0;
    mSquares = 0;
    mTotalTime = 0;
    mCount = 0;
    mLastInterval = 0;
}

void
nsTimeSampler::StartTime()
{
    mStartTime = PR_IntervalNow();
}

void
nsTimeSampler::EndTime()
{
    NS_ASSERTION(mStartTime != 0, "Forgot to call StartTime");
    PRIntervalTime endTime = PR_IntervalNow();
    mLastInterval = endTime - mStartTime;
    AddTime(mLastInterval);
    mStartTime = 0;
}

void
nsTimeSampler::AddTime(PRIntervalTime time)
{
    nsAutoCMonitor mon(this);
    mTotalTime += time;
    mSquares += (double)time * (double)time;
    mCount++;
}

char*
nsTimeSampler::PrintStats()
{
    double mean = mTotalTime / mCount;
    double variance = fabs(mSquares / mCount - mean * mean);
    double stddev = sqrt(variance);
    PRUint32 imean = (PRUint32)mean;
    PRUint32 istddev = (PRUint32)stddev;
    return PR_smprintf("%d +/- %d ms", 
                       PR_IntervalToMilliseconds(imean),
                       PR_IntervalToMilliseconds(istddev));
}

////////////////////////////////////////////////////////////////////////////////

nsTimeSampler gTimeSampler;

typedef nsresult (*CreateFun)(nsIRunnable* *result,
                              nsIFile* inPath, 
                              nsIFile* outPath, 
                              PRUint32 bufferSize);

////////////////////////////////////////////////////////////////////////////////

nsresult
Copy(nsIInputStream* inStr, nsIOutputStream* outStr, 
     char* buf, PRUint32 bufSize, PRUint32 *copyCount)
{
    nsresult rv;
    while (PR_TRUE) {
        PRUint32 count;
        rv = inStr->Read(buf, bufSize, &count);
        if (NS_FAILED(rv)) return rv;
        if (count == 0) break;
            
        PRUint32 writeCount;
        rv = outStr->Write(buf, count, &writeCount);
        if (NS_FAILED(rv)) return rv;
        NS_ASSERTION(writeCount == count, "didn't write all the data");
        *copyCount += writeCount;
    }
    rv = outStr->Flush();
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

class FileSpecWorker : public nsIRunnable {
public:

    NS_IMETHOD Run() {
        nsresult rv;

        PRIntervalTime startTime = PR_IntervalNow();
        PRIntervalTime endTime;
        nsCOMPtr<nsIInputStream> inStr;
        nsCOMPtr<nsIOutputStream> outStr;
        PRUint32 copyCount = 0;

        // Open the input stream:
        rv = NS_NewFileInputStream(mInPath, getter_AddRefs(inStr));
        if (NS_FAILED(rv)) return rv;
        
        // Open the output stream:
        rv = NS_NewFileOutputStream(mOutPath, 
                                    PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE,
                                    0664,
                                    getter_AddRefs(outStr));
        if (NS_FAILED(rv)) return rv;

        // Copy from one to the other
        rv = Copy(inStr, outStr, mBuffer, mBufferSize, &copyCount);
        if (NS_FAILED(rv)) return rv;

        endTime = PR_IntervalNow();
        gTimeSampler.AddTime(endTime - startTime);

        return rv;
    }

    NS_DECL_ISUPPORTS

    FileSpecWorker()
        : mInPath(nsnull), mOutPath(nsnull), mBuffer(nsnull),
          mBufferSize(0)
    {
        NS_INIT_REFCNT();
    }

    nsresult Init(nsIFile* inPath, nsIFile* outPath,
                  PRUint32 bufferSize)
    {
        mInPath = inPath;
        mOutPath = outPath;
        mBuffer = new char[bufferSize];
        mBufferSize = bufferSize;
        return (mInPath && mOutPath && mBuffer)
            ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    static nsresult Create(nsIRunnable* *result,
                           nsIFile* inPath, 
                           nsIFile* outPath, 
                           PRUint32 bufferSize)
    {
        FileSpecWorker* worker = new FileSpecWorker();
        if (worker == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(worker);

        nsresult rv = worker->Init(inPath, outPath, bufferSize);
        if (NS_FAILED(rv)) {
            NS_RELEASE(worker);
            return rv;
        }
        *result = worker;
        return NS_OK;
    }

    virtual ~FileSpecWorker() {
        delete[] mBuffer;
    }

protected:
    nsCOMPtr<nsIFile>   mInPath;
    nsCOMPtr<nsIFile>   mOutPath;
    char*               mBuffer;
    PRUint32            mBufferSize;
};

NS_IMPL_ISUPPORTS(FileSpecWorker, NS_GET_IID(nsIRunnable));

////////////////////////////////////////////////////////////////////////////////

#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsIFileProtocolHandler.h"

class FileChannelWorker : public nsIRunnable {
public:

    NS_IMETHOD Run() {
        nsresult rv;

        PRIntervalTime startTime = PR_IntervalNow();
        PRIntervalTime endTime;
        PRUint32 copyCount = 0;
        nsCOMPtr<nsIFileChannel> inCh;
        nsCOMPtr<nsIFileChannel> outCh;
        nsCOMPtr<nsIInputStream> inStr;
        nsCOMPtr<nsIOutputStream> outStr;

        rv = NS_NewFileChannel(mInPath,
                               PR_RDONLY,
                               nsnull,   // contentType
                               0,        // contentLength,
                               nsnull,   // aLoadGroup
                               nsnull,   // notificationCallbacks
                               nsIChannel::LOAD_NORMAL,
                               nsnull,   // originalURI
                               0,        // bufferSegmentSize
                               0,        // bufferMaxSize
                               getter_AddRefs(inCh));
        if (NS_FAILED(rv)) return rv;

        rv = inCh->OpenInputStream(0, -1, getter_AddRefs(inStr));
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewFileChannel(mOutPath,
                               PR_RDWR,
                               nsnull,   // contentType
                               0,        // contentLength,
                               nsnull,   // aLoadGroup
                               nsnull,   // notificationCallbacks
                               nsIChannel::LOAD_NORMAL,
                               nsnull,   // originalURI
                               0,        // bufferSegmentSize
                               0,        // bufferMaxSize
                               getter_AddRefs(outCh));
        if (NS_FAILED(rv)) return rv;

        rv = outCh->OpenOutputStream(0, getter_AddRefs(outStr));
        if (NS_FAILED(rv)) return rv;

        // Copy from one to the other
        rv = Copy(inStr, outStr, mBuffer, mBufferSize, &copyCount);
        if (NS_FAILED(rv)) return rv;
        
        endTime = PR_IntervalNow();
        gTimeSampler.AddTime(endTime - startTime);

        return rv;
    }

    NS_DECL_ISUPPORTS

    FileChannelWorker()
        : mInPath(nsnull), mOutPath(nsnull), mBuffer(nsnull),
          mBufferSize(0)
    {
        NS_INIT_REFCNT();
    }

    nsresult Init(nsIFile* inPath, nsIFile* outPath,
                  PRUint32 bufferSize)
    {
        mInPath = inPath;
        mOutPath = outPath;
        mBuffer = new char[bufferSize];
        mBufferSize = bufferSize;
        return (mInPath && mOutPath && mBuffer)
            ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }

    static nsresult Create(nsIRunnable* *result,
                           nsIFile* inPath, 
                           nsIFile* outPath, 
                           PRUint32 bufferSize)
    {
        FileChannelWorker* worker = new FileChannelWorker();
        if (worker == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(worker);

        nsresult rv = worker->Init(inPath, outPath, bufferSize);
        if (NS_FAILED(rv)) {
            NS_RELEASE(worker);
            return rv;
        }
        *result = worker;
        return NS_OK;
    }

    virtual ~FileChannelWorker() {
        delete[] mBuffer;
    }

protected:
    nsCOMPtr<nsIFile>   mInPath;
    nsCOMPtr<nsIFile>   mOutPath;
    char*               mBuffer;
    PRUint32            mBufferSize;
};

NS_IMPL_ISUPPORTS(FileChannelWorker, NS_GET_IID(nsIRunnable));

////////////////////////////////////////////////////////////////////////////////

void
Test(CreateFun create, PRUint32 count,
     nsIFile* inDirSpec, nsIFile* outDirSpec, PRUint32 bufSize)
{
    nsresult rv;
    PRUint32 i;
    
    char* inDir;
    char* outDir;
    (void)inDirSpec->GetPath(&inDir);
    (void)outDirSpec->GetPath(&outDir);
    printf("###########\nTest: from %s to %s, bufSize = %d\n",
           inDir, outDir, bufSize);
    nsCRT::free(inDir);
    nsCRT::free(outDir);
    gTimeSampler.Reset();
    nsTimeSampler testTime;
    testTime.StartTime();
    
    nsISupportsArray* threads;
    rv = NS_NewISupportsArray(&threads);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_NewISupportsArray failed");

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = inDirSpec->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetDirectoryEntries failed");

    i = 0;
    PRBool hasMore;
    while (i < count && NS_SUCCEEDED(entries->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> next;
        rv = entries->GetNext(getter_AddRefs(next));
        if (NS_FAILED(rv)) goto done;

        nsCOMPtr<nsIFile> inSpec = do_QueryInterface(next, &rv);
        if (NS_FAILED(rv)) goto done;

        nsCOMPtr<nsIFile> outSpec;
        rv = outDirSpec->Clone(getter_AddRefs(outSpec)); // don't munge the original
        if (NS_FAILED(rv)) goto done;

        char* leafName;
        rv = inSpec->GetLeafName(&leafName);
        if (NS_FAILED(rv)) goto done;

        rv = outSpec->AppendPath(leafName);
        nsCRT::free(leafName);
        if (NS_FAILED(rv)) goto done;

        PRBool exists;
        rv = outSpec->Exists(&exists);
        if (NS_FAILED(rv)) goto done;

        if (exists) {
            rv = outSpec->Delete(PR_FALSE);
            if (NS_FAILED(rv)) goto done;
        }

        nsCOMPtr<nsIThread> thread;
        nsCOMPtr<nsIRunnable> worker;
        rv = create(getter_AddRefs(worker), 
                    inSpec,
                    outSpec,
                    bufSize);
        if (NS_FAILED(rv)) goto done;

        rv = NS_NewThread(getter_AddRefs(thread), worker, 0, PR_JOINABLE_THREAD);
        if (NS_FAILED(rv)) goto done;

        PRBool inserted = threads->InsertElementAt(thread, i);
        NS_ASSERTION(inserted, "not inserted");

        i++;
    }

    PRUint32 j;
    for (j = 0; j < i; j++) {
        nsIThread* thread = (nsIThread*)threads->ElementAt(j);
        if (NS_FAILED(rv)) goto done;
        thread->Join();
    }

  done:
    NS_RELEASE(threads);
    NS_ASSERTION(rv == NS_OK, "failed");

    testTime.EndTime();
    char* testStats = testTime.PrintStats();
    char* workerStats = gTimeSampler.PrintStats();
    printf("  threads = %d\n  work time = %s,\n  test time = %s\n",
           i, workerStats, testStats);
    PR_smprintf_free(workerStats);
    PR_smprintf_free(testStats);
}

////////////////////////////////////////////////////////////////////////////////

nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup, NULL /* default */);
  return rv;
}

int
main(int argc, char* argv[])
{
    nsresult rv;

    if (argc < 2) {
        printf("usage: %s <in-dir> <out-dir>\n", argv[0]);
        return -1;
    }
    char* inDir = argv[1];
    char* outDir = argv[2];

    rv = NS_AutoregisterComponents();
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILocalFile> inDirFile;
    rv = NS_NewLocalFile(inDir, getter_AddRefs(inDirFile));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILocalFile> outDirFile;
    rv = NS_NewLocalFile(outDir, getter_AddRefs(outDirFile));
    if (NS_FAILED(rv)) return rv;

    CreateFun create = FileChannelWorker::Create;
    Test(create, 1, inDirFile, outDirFile, 16 * 1024);
#if 1
    printf("FileChannelWorker *****************************\n");
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
#endif
    create = FileSpecWorker::Create;
    printf("FileSpecWorker ********************************\n");
#if 1
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
    Test(create, 20, inDirFile, outDirFile, 16 * 1024);
#endif
#if 1
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
    Test(create, 20, inDirFile, outDirFile, 4 * 1024);
#endif

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
