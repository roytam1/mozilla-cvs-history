/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsSocketTransport_h___
#define nsSocketTransport_h___

#include "prclist.h"
#include "prio.h"
#include "prnetdb.h"
#include "prinrval.h"

#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIBufferInputStream.h"
#include "nsIBufferOutputStream.h"
#include "nsIEventQueueService.h"
#include "nsIStreamListener.h"
#include "nsIDNSListener.h"
#include "nsIPipe.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"

#define NS_SOCKET_TRANSPORT_SEGMENT_SIZE        (2*1024)
#define NS_SOCKET_TRANSPORT_BUFFER_SIZE         (8*1024)

//
// This is the maximum amount of data that will be read into a stream before
// another transport is processed...
//
#define MAX_IO_TRANSFER_SIZE  (8*1024)

enum nsSocketState {
  eSocketState_Created        = 0,
  eSocketState_WaitDNS        = 1,
  eSocketState_Closed         = 2,
  eSocketState_WaitConnect    = 3,
  eSocketState_Connected      = 4,
  eSocketState_WaitReadWrite  = 5,
  eSocketState_Done           = 6,
  eSocketState_Timeout        = 7,
  eSocketState_Error          = 8,
  eSocketState_Max            = 9
};

enum nsSocketOperation {
  eSocketOperation_None       = 0,
  eSocketOperation_Connect    = 1,
  eSocketOperation_ReadWrite  = 2,
  eSocketOperation_Max        = 3
};

//
// The following emun provides information about the currently
// active read and/or write requests...
//
// +-------------------------------+
// | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
// +-------------------------------+
// <-----flag bits----><-type bits->
//         
//  Bits:
//      0-3:  Type (ie. None, Async, Sync)
//        4:  Done flag.
//        5:  Wait flag.
//      6-7:  Unused flags...
//
//
//   
enum nsSocketReadWriteInfo {
  eSocketRead_None            = 0x0000,
  eSocketRead_Async           = 0x0001,
  eSocketRead_Sync            = 0x0002,
  eSocketRead_Done            = 0x0010,
  eSocketRead_Wait            = 0x0020,
  eSocketRead_Type_Mask       = 0x000F,
  eSocketRead_Flag_Mask       = 0x00F0,

  eSocketWrite_None           = 0x0000,
  eSocketWrite_Async          = 0x0100,
  eSocketWrite_Sync           = 0x0200,
  eSocketWrite_Done           = 0x1000,
  eSocketWrite_Wait           = 0x2000,
  eSocketWrite_Type_Mask      = 0x0F00,
  eSocketWrite_Flag_Mask      = 0xF000,

  eSocketDNS_Wait             = 0x2020
};

// Forward declarations...
class nsSocketTransportService;
class nsIInterfaceRequestor;

class nsSocketTransport : public nsIChannel, 
                          public nsIDNSListener,
                          public nsIPipeObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIPIPEOBSERVER
  NS_DECL_NSIDNSLISTENER

  // nsSocketTransport methods:
  nsSocketTransport();
  virtual ~nsSocketTransport();

  nsresult Init(nsSocketTransportService* aService,
                const char* aHost, 
                PRInt32 aPort,
                const char* aSocketType,
                const char* aPrintHost, // This host is used for status mesg
                PRUint32 bufferSegmentSize,
                PRUint32 bufferMaxSize);

  nsresult Process(PRInt16 aSelectFlags);

  nsresult CheckForTimeout(PRIntervalTime aCurrentTime);

  // Close this socket either right away or once done with the transaction. 
  nsresult CloseConnection(PRBool bNow=PR_TRUE);

  // Access methods used by the socket transport service...
  PRFileDesc* GetSocket(void)      { return mSocketFD;    }
  PRInt16     GetSelectFlags(void) { return mSelectFlags; }
  PRCList*    GetListNode(void)    { return &mListLink;   }

  static nsSocketTransport* GetInstance(PRCList* qp) { return (nsSocketTransport*)((char*)qp - offsetof(nsSocketTransport, mListLink)); }

  static void SetSocketTimeout(PRIntervalTime aTimeoutInterval);

  PRBool CanBeReused(void) { return 
    (mCurrentState != eSocketState_Error) && !mCloseConnectionOnceDone;}

protected:
  nsresult doConnection(PRInt16 aSelectFlags);
  nsresult doResolveHost(void);
  nsresult doRead(PRInt16 aSelectFlags);
  nsresult doWrite(PRInt16 aSelectFlags);

  nsresult doWriteFromBuffer(PRUint32 *aCount);
  nsresult doWriteFromStream(PRUint32 *aCount);

  nsresult fireStatus(PRUint32 aCode);
  nsresult GetSocketErrorString(PRUint32 iCode, PRUnichar** oString) const;

private:
  // Access methods for manipulating the ReadWriteInfo...
  inline void SetReadType(nsSocketReadWriteInfo aType) {
    mReadWriteState = (mReadWriteState & ~eSocketRead_Type_Mask) | aType; 
  }
  inline PRUint32 GetReadType(void) {
    return mReadWriteState & eSocketRead_Type_Mask;
  }
  inline void SetWriteType(nsSocketReadWriteInfo aType) {
    mReadWriteState = (mReadWriteState & ~eSocketWrite_Type_Mask) | aType; 
  }
  inline PRUint32 GetWriteType(void) {
    return mReadWriteState & eSocketWrite_Type_Mask;
  }
  inline void SetFlag(nsSocketReadWriteInfo aFlag) { 
    mReadWriteState |= aFlag;
  }
  inline PRUint32 GetFlag(nsSocketReadWriteInfo aFlag) {
    return mReadWriteState & aFlag;
  }

  inline void ClearFlag(nsSocketReadWriteInfo aFlag) {
    mReadWriteState &= ~aFlag;
  } 

protected:

  PRBool                            mCancelOperation;
  PRBool                            mCloseConnectionOnceDone;
  nsSocketState                     mCurrentState;
  nsCOMPtr<nsIRequest>              mDNSRequest;
  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mEventSink;
  char*                             mHostName;
  PRIntervalTime                    mLastActiveTime;
  PRCList                           mListLink;
  PRUint32                          mLoadAttributes;
  PRLock*                           mLock;
  PRNetAddr                         mNetAddress;
  nsCOMPtr<nsISupports>             mOpenContext;
  nsCOMPtr<nsIStreamObserver>       mOpenObserver;
  nsSocketOperation                 mOperation;
  nsCOMPtr<nsISupports>             mOwner;
  PRInt32                           mPort;
  char*                             mPrintHost; // not the proxy
  nsCOMPtr<nsISupports>             mReadContext;
  nsCOMPtr<nsIStreamListener>       mReadListener;
  nsCOMPtr<nsIBufferInputStream>    mReadPipeIn;
  nsCOMPtr<nsIBufferOutputStream>   mReadPipeOut;
  PRUint32                          mReadWriteState;
  PRInt16                           mSelectFlags;
  nsSocketTransportService*         mService;
  PRFileDesc*                       mSocketFD;
  char*                             mSocketType;
  PRUint32                          mReadOffset;
  PRUint32                          mWriteOffset;
  nsresult                          mStatus;
  PRInt32                           mSuspendCount;
  PRInt32                           mWriteCount;
  nsCOMPtr<nsISupports>             mWriteContext;
  nsCOMPtr<nsIInputStream>          mWriteFromStream;
  nsCOMPtr<nsIStreamObserver>       mWriteObserver;
  nsCOMPtr<nsIBufferInputStream>    mWritePipeIn;
  nsCOMPtr<nsIBufferOutputStream>   mWritePipeOut;
  PRUint32                          mBufferSegmentSize;
  PRUint32                          mBufferMaxSize;
};


#endif /* nsSocketTransport_h___ */
