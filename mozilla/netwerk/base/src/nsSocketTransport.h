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

#include "nsIChannel.h"
#include "nsIBuffer.h"
#include "nsIInputStream.h"
#include "nsIBufferInputStream.h"

//
// This is the size of the global buffer used by all nsSocketTransport 
// instances when reading from or writing to the network.
//
#define MAX_IO_BUFFER_SIZE   8192

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
  eSocketState_DoneRead       = 6,
  eSocketState_DoneWrite      = 7,
  eSocketState_Done           = 8,
  eSocketState_Timeout        = 9,
  eSocketState_Error          = 10,
  eSocketState_Max            = 11
};

enum nsSocketOperation {
  eSocketOperation_None       = 0,
  eSocketOperation_Connect    = 1,
  eSocketOperation_ReadWrite  = 2,
  eSocketOperation_Max        = 3
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
  NS_IMETHOD Cancel(void);
  NS_IMETHOD Suspend(void);
  NS_IMETHOD Resume(void);

  // nsIChannel methods:
  NS_IMETHOD GetURI(nsIURI * *aURL);
  NS_IMETHOD OpenInputStream(PRUint32 startPosition, PRInt32 readCount, nsIInputStream **_retval);
  NS_IMETHOD OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval);
  NS_IMETHOD AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                       nsISupports *ctxt,
                       nsIEventQueue *eventQueue,
                       nsIStreamListener *listener);
  NS_IMETHOD AsyncWrite(nsIInputStream *fromStream, 
                        PRUint32 startPosition, PRInt32 writeCount,
                        nsISupports *ctxt,
                        nsIEventQueue *eventQueue,
                        nsIStreamObserver *observer);
  NS_IMETHOD GetLoadAttributes(PRUint32 *aLoadAttributes);
  NS_IMETHOD SetLoadAttributes(PRUint32 aLoadAttributes);
  NS_IMETHOD GetContentType(char * *aContentType);

  // nsIBufferObserver methods:
  NS_IMETHOD OnFull(nsIBuffer* buffer);
  NS_IMETHOD OnEmpty(nsIBuffer* buffer);

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

protected:
  PRCList           mListLink;

  PRLock*           mLock;
  nsSocketState     mCurrentState;
  nsSocketOperation mOperation;

  PRBool            mIsWaitingForRead;
  PRInt32           mSuspendCount;

  PRFileDesc*   mSocketFD;
  PRNetAddr     mNetAddress;
  PRInt16       mSelectFlags;

  char*         mHostName;
  PRInt32       mPort;

  nsISupports*              mReadContext;
  nsIStreamListener*        mReadListener;
  nsIBufferInputStream*     mReadStream;
  nsIBuffer*                mReadBuffer;

  PRInt32                   mWriteCount;
  nsISupports*              mWriteContext;
  nsIStreamObserver*        mWriteObserver;
  nsIInputStream*           mWriteStream;
  
  PRUint32 mSourceOffset;

  nsSocketTransportService* mService;
  PRUint32                  mLoadAttributes;
};


#endif /* nsSocketTransport_h___ */
