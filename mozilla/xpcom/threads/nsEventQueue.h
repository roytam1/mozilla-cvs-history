/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "prmon.h"
#include "nsIEventQueue.h"
#include "nsXPComFactory.h"

static NS_DEFINE_IID(kIEventQueueIID, NS_IEVENTQUEUE_IID);

class nsEventQueueImpl : public nsIEventQueue
{
public:
  nsEventQueueImpl();

  // nsISupports interface...
  NS_DECL_ISUPPORTS

  // nsIEventQueue interface...
	NS_IMETHOD_(PRStatus) PostEvent(PLEvent* aEvent);
	NS_IMETHOD PostSynchronousEvent(PLEvent* aEvent);
  NS_IMETHOD ProcessPendingEvents();
	NS_IMETHOD EventAvailable(PRBool& aResult);
	NS_IMETHOD GetEvent(PLEvent** aResult);

	NS_IMETHOD Init();
	NS_IMETHOD InitFromPLQueue(PLEventQueue* aQueue);

	// Helpers
	static NS_METHOD
  Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

	static const nsCID& CID() { static nsCID cid = NS_EVENTQUEUE_CID; return cid; }

protected:
  ~nsEventQueueImpl();

private:
  PLEventQueue*	mEventQueue;
};



