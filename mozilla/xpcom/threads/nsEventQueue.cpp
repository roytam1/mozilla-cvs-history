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

#include "nsCOMPtr.h"
#include "nsEventQueue.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsString2.h"

// in a real system, these would be members in a header class...
static char *gActivatedNotification = "nsIEventQueueActivated";
static char *gDestroyedNotification = "nsIEventQueueDestroyed";

nsEventQueueImpl::nsEventQueueImpl()
{
  NS_INIT_REFCNT();
  AddRef(); 
  /* The slightly weird ownership model for eventqueues goes like this:
     there's an addref from the factory generally held by whoever asked for
     the queue. The queue addrefs itself (right here) and releases itself
     when it goes dark and empty. Chained queues also hold references to
     their immediate elder link, because we have code that assumes the random
     release of a queue can't break a chain earlier than our current pointer.
     A queue releases itself immediately upon being chained, dropping
     the addref given it by the factory. Only the references held to it by
     a younger chained queue (and from the outside), and this one here in the
     constructor, keep it alive. The release when it goes dark and empty
     will queue it, as it were, for destruction.
     (A dark queue no longer accepts events, an empty one has none.)
  */

  mEventQueue = NULL;
  mYoungerQueue = NULL;
  mElderQueue = NULL;
  mAcceptingEvents = PR_TRUE;
  mCouldHaveEvents = PR_TRUE;
}

nsEventQueueImpl::~nsEventQueueImpl()
{
  Unlink();
  if (mEventQueue != NULL) {
    NotifyObservers(gDestroyedNotification);
    PL_DestroyEventQueue(mEventQueue);
  }
}

NS_IMETHODIMP 
nsEventQueueImpl::Init()
{
  mEventQueue = PL_CreateNativeEventQueue("Thread event queue...", PR_GetCurrentThread());
  NotifyObservers(gActivatedNotification);
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::InitFromPLQueue(PLEventQueue* aQueue)
{
  mEventQueue = aQueue;
  NotifyObservers(gActivatedNotification);
  return NS_OK;
}

/* nsISupports interface implementation... */
NS_IMPL_ISUPPORTS2(nsEventQueueImpl,nsIEventQueue,nsPIEventQueueChain)

/* nsIEventQueue interface implementation... */

NS_IMETHODIMP
nsEventQueueImpl::StopAcceptingEvents()
{
  NS_ASSERTION(mElderQueue, "attempted to disable eldest queue in chain");
  mAcceptingEvents = PR_FALSE;
  CheckForDeactivation();
  return NS_OK;
}

// utility funtion to send observers a notification
void
nsEventQueueImpl::NotifyObservers(const char *aTopic)
{
  nsresult rv;
  nsAutoString topic(aTopic);
  nsISupports *us = NS_STATIC_CAST(nsISupports *,(NS_STATIC_CAST(nsIEventQueue *,this)));

  NS_WITH_SERVICE(nsIObserverService, os, NS_OBSERVERSERVICE_PROGID, &rv);
  if (NS_SUCCEEDED(rv))
    os->Notify(us, topic.GetUnicode(), NULL);
}

NS_IMETHODIMP_(PRStatus)
nsEventQueueImpl::PostEvent(PLEvent* aEvent)
{
  if (!mAcceptingEvents) {
    PRStatus rv = PR_FAILURE;
    NS_ASSERTION(mElderQueue, "event dropped because event chain is dead");
    if (mElderQueue) {
      nsCOMPtr<nsIEventQueue> elder(do_QueryInterface(mElderQueue));
      if (elder)
        rv = elder->PostEvent(aEvent);
    }
    return rv;
  }
  return PL_PostEvent(mEventQueue, aEvent);
}

NS_IMETHODIMP
nsEventQueueImpl::PostSynchronousEvent(PLEvent* aEvent, void** aResult)
{
  if (!mAcceptingEvents) {
    nsresult rv = NS_ERROR_NO_INTERFACE;
    NS_ASSERTION(mElderQueue, "event dropped because event chain is dead");
    if (mElderQueue) {
      nsCOMPtr<nsIEventQueue> elder(do_QueryInterface(mElderQueue));
      if (elder)
        rv = elder->PostSynchronousEvent(aEvent, aResult);
      return rv;
    }
    return NS_ERROR_ABORT;
  }

  void* result = PL_PostSynchronousEvent(mEventQueue, aEvent);
  if (aResult)
    *aResult = result;

  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::EnterMonitor()
{
    PL_ENTER_EVENT_QUEUE_MONITOR(mEventQueue);
    return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::ExitMonitor()
{
    PL_EXIT_EVENT_QUEUE_MONITOR(mEventQueue);
    return NS_OK;
}


NS_IMETHODIMP
nsEventQueueImpl::RevokeEvents(void* owner)
{
    PL_RevokeEvents(mEventQueue, owner);
    return NS_OK;
}


NS_IMETHODIMP
nsEventQueueImpl::GetPLEventQueue(PLEventQueue** aEventQueue)
{
    *aEventQueue = mEventQueue;
    
    if (mEventQueue == NULL)
        return NS_ERROR_NULL_POINTER;

    return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::IsQueueOnCurrentThread(PRBool *aResult)
{
    *aResult = PL_IsQueueOnCurrentThread( mEventQueue );
    return NS_OK;
}


NS_IMETHODIMP
nsEventQueueImpl::ProcessPendingEvents()
{
  PL_ProcessPendingEvents(mEventQueue);
  CheckForDeactivation();
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::EventLoop()
{
  PL_EventLoop(mEventQueue);
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::EventAvailable(PRBool& aResult)
{
  aResult = PL_EventAvailable(mEventQueue);
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::GetEvent(PLEvent** aResult)
{
  *aResult = PL_GetEvent(mEventQueue);
  CheckForDeactivation();
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::HandleEvent(PLEvent* aEvent)
{
  PL_HandleEvent(aEvent);
  return NS_OK;
}

NS_IMETHODIMP_(PRInt32) 
nsEventQueueImpl::GetEventQueueSelectFD() 
{
  return PL_GetEventQueueSelectFD(mEventQueue);
}

NS_METHOD
nsEventQueueImpl::Create(nsISupports *aOuter,
                            REFNSIID aIID,
                            void **aResult)
{
  nsEventQueueImpl* evt = new nsEventQueueImpl();
  if (evt == NULL)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = evt->QueryInterface(aIID, aResult);
  if (NS_FAILED(rv)) {
    delete evt;
  }
  return rv;
}

// ---------------- nsPIEventQueueChain -----------------

NS_IMETHODIMP
nsEventQueueImpl::AppendQueue(nsIEventQueue *aQueue)
{
  nsresult      rv;
  nsIEventQueue *end;
  nsCOMPtr<nsPIEventQueueChain> queueChain(do_QueryInterface(aQueue));

  if (!aQueue)
    return NS_ERROR_NO_INTERFACE;

/* this would be nice
  NS_ASSERTION(aQueue->mYoungerQueue == NULL && aQueue->mElderQueue == NULL,
               "event queue repeatedly appended to queue chain");
*/
  rv = NS_ERROR_NO_INTERFACE;

  // (be careful doing this outside nsEventQueueService's mEventQMonitor)

  GetYoungest(&end); // addrefs. released by Unlink.
  nsCOMPtr<nsPIEventQueueChain> endChain(do_QueryInterface(end));
  if (endChain) {
    endChain->SetYounger(queueChain);
    queueChain->SetElder(endChain);
    NS_RELEASE(aQueue); // the addref from the constructor
    rv = NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsEventQueueImpl::Unlink()
{
  nsPIEventQueueChain *young = mYoungerQueue,
                      *old = mElderQueue;

  // this is probably OK, but shouldn't happen by design, so tell me if it does
  NS_ASSERTION(!mYoungerQueue, "event queue chain broken in middle");

  // break links early in case the Release cascades back onto us
  mYoungerQueue = 0;
  mElderQueue = 0;

  if (young)
    young->SetElder(old);
  if (old) {
    old->SetYounger(young);
    if (!young)
      NS_RELEASE(old); // release addref from AppendQueue
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::GetYoungest(nsIEventQueue **aQueue)
{
  if (mYoungerQueue)
    return mYoungerQueue->GetYoungest(aQueue);

  nsIEventQueue *answer = NS_STATIC_CAST(nsIEventQueue *, this);
  NS_ADDREF(answer);
  *aQueue = answer;
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::GetYoungestActive(nsIEventQueue **aQueue)
{
  nsIEventQueue *answer = NULL;

  if (mYoungerQueue)
    mYoungerQueue->GetYoungestActive(&answer);
  if (answer == NULL)
    if (mAcceptingEvents && mCouldHaveEvents) {
      answer = NS_STATIC_CAST(nsIEventQueue *, this);
      NS_ADDREF(answer);
    } else
      CheckForDeactivation();
  *aQueue = answer;
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::SetYounger(nsPIEventQueueChain *aQueue)
{
  mYoungerQueue = aQueue;
  return NS_OK;
}

NS_IMETHODIMP
nsEventQueueImpl::SetElder(nsPIEventQueueChain *aQueue)
{
  mElderQueue = aQueue;
  return NS_OK;
}

