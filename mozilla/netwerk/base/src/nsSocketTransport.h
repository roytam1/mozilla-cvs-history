/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef nsSocketTransport_h___
#define nsSocketTransport_h___

#include "prclist.h"
#include "prio.h"
#include "prnetdb.h"

#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIBuffer.h"
#include "nsIInputStream.h"
#include "nsIBufferInputStream.h"
#include "nsIEventQueueService.h"
#include "nsIStreamListener.h"

//
// This is the size of the global buffer used by all nsSocketTransport 
// instances when reading from or writing to the network.
//
#define MAX_IO_BUFFER_SIZE   (1024*1024)//8192

//
// This is the maximum amount of data that will be read into a stream before
// another transport is processed...
//
#define MAX_IO_TRANSFER_SIZE  32768

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
  eSocketWrite_Flag_Mask      = 0xF000
};

// Forward declarations...
class nsSocketTransportService;

class nsSocketTransport : public nsIChannel, 
                          public nsIBufferObserver
{
public:
  // nsISupports methods:
  NS_DECL_ISUPPORTS

  // nsIRequest methods:
  NS_IMETHOD IsPending(PRBool *result);
  NS_IMETHOD Cancel(void);
  NS_IMETHOD Suspend(void);
  NS_IMETHOD Resume(void);

  // nsIChannel methods:
  NS_IMETHOD GetURI(nsIURI * *aURL);
  NS_IMETHOD OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval);
  NS_IMETHOD OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval);
  NS_IMETHOD AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                       nsISupports *ctxt,
                       nsIStreamListener *listener);
  NS_IMETHOD AsyncWrite(nsIInputStream *fromStream, 
                        PRUint32 startPosition, PRInt32 writeCount,
                        nsISupports *ctxt,
                        nsIStreamObserver *observer);
  NS_IMETHOD GetLoadAttributes(PRUint32 *aLoadAttributes);
  NS_IMETHOD SetLoadAttributes(PRUint32 aLoadAttributes);
  NS_IMETHOD GetContentType(char * *aContentType);

  // nsIBufferObserver methods:
  NS_IMETHOD OnFull (nsIBuffer* aBuffer);
  NS_IMETHOD OnWrite(nsIBuffer* aBuffer, PRUint32 aCount);
  NS_IMETHOD OnEmpty(nsIBuffer* aBuffer);

  // nsSocketTransport methods:
  nsSocketTransport();
  virtual ~nsSocketTransport();

  nsresult Init(nsSocketTransportService* aService,
                const char* aHost, 
                PRInt32 aPort);
  nsresult Process(PRInt16 aSelectFlags);

  nsresult CloseConnection(void);

  // Access methods used by the socket transport service...
  PRFileDesc* GetSocket(void)      { return mSocketFD;    }
  PRInt16     GetSelectFlags(void) { return mSelectFlags; }
  PRCList*    GetListNode(void)    { return &mListLink;   }

  static nsSocketTransport* GetInstance(PRCList* qp) { return (nsSocketTransport*)((char*)qp - offsetof(nsSocketTransport, mListLink)); }

protected:
  nsresult doConnection(PRInt16 aSelectFlags);
  nsresult doResolveHost(void);
  nsresult doRead(PRInt16 aSelectFlags);
  nsresult doWrite(PRInt16 aSelectFlags);

  nsresult doWriteFromBuffer(PRUint32 *aCount);
  nsresult doWriteFromStream(PRUint32 *aCount);

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
  PRCList           mListLink;

  PRLock*           mLock;
  nsSocketState     mCurrentState;
  nsSocketOperation mOperation;

  PRUint32          mReadWriteState;

  PRInt32           mSuspendCount;

  PRFileDesc*   mSocketFD;
  PRNetAddr     mNetAddress;
  PRInt16       mSelectFlags;

  char*         mHostName;
  PRInt32       mPort;

  nsCOMPtr<nsISupports>       mReadContext;
  nsCOMPtr<nsIStreamListener> mReadListener;
  nsCOMPtr<nsIInputStream>    mReadStream;
  nsCOMPtr<nsIBuffer>         mReadBuffer;

  PRInt32                     mWriteCount;
  nsCOMPtr<nsISupports>       mWriteContext;
  nsCOMPtr<nsIStreamObserver> mWriteObserver;
  nsCOMPtr<nsIInputStream>    mWriteStream;
  nsCOMPtr<nsIBuffer>         mWriteBuffer;
  
  PRUint32 mSourceOffset;

  nsSocketTransportService* mService;
  PRUint32                  mLoadAttributes;
};


#endif /* nsSocketTransport_h___ */
