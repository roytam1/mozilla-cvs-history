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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsIEventQueueService_h__
#define nsIEventQueueService_h__

#include "nsISupports.h"
#include "prthread.h"
#include "plevent.h"
#include "nsIEventQueue.h"

/* a6cf90dc-15b3-11d2-932e-00805f8add32 */
#define NS_IEVENTQUEUESERVICE_IID \
{ 0xa6cf90dc, 0x15b3, 0x11d2, \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

/* be761f00-a3b0-11d2-996c-0080c7cb1080 */
#define NS_EVENTQUEUESERVICE_CID \
{ 0xbe761f00, 0xa3b0, 0x11d2, \
  {0x99, 0x6c, 0x00, 0x80, 0xc7, 0xcb, 0x10, 0x80} }

#define NS_EVENTQUEUESERVICE_PROGID "component://netscape/event-queue-service"
#define NS_EVENTQUEUESERVICE_CLASSNAME "Event Queue Service"

#define NS_CURRENT_THREAD    ((PRThread*)0)
#define NS_CURRENT_EVENTQ    ((nsIEventQueue*)0)

#define NS_UI_THREAD         ((PRThread*)1)
#define NS_UI_THREAD_EVENTQ  ((nsIEventQueue*)1)


class nsIThread;

class nsIEventQueueService : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IEVENTQUEUESERVICE_IID; return iid; }

  NS_IMETHOD CreateThreadEventQueue(void) = 0;
  NS_IMETHOD DestroyThreadEventQueue(void) = 0;

  NS_IMETHOD CreateFromIThread(nsIThread *aThread, nsIEventQueue **aResult) = 0;
  NS_IMETHOD CreateFromPLEventQueue(PLEventQueue* aPLEventQueue, nsIEventQueue** aResult) = 0;

  // Add a new event queue for the current thread, making it the "current"
  // queue. Return that queue in aNewQueue, addrefed.
  NS_IMETHOD PushThreadEventQueue(nsIEventQueue **aNewQueue) = 0;

  // release and disable the queue
  NS_IMETHOD PopThreadEventQueue(nsIEventQueue *aQueue) = 0;

  NS_IMETHOD GetThreadEventQueue(PRThread* aThread, nsIEventQueue** aResult) = 0;
  NS_IMETHOD ResolveEventQueue(nsIEventQueue* queueOrConstant, nsIEventQueue* *resultQueue) = 0;

#ifdef XP_MAC
// This is ment to be temporary until something better is worked out
 NS_IMETHOD ProcessEvents() = 0;
#endif
};

#endif /* nsIEventQueueService_h___ */
