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

#include "nsTimerXlib.h"

#include <unistd.h>
#include <stdio.h>

#include "prlog.h"

static NS_DEFINE_IID(kITimerIID, NS_ITIMER_IID);

extern "C" int  NS_TimeToNextTimeout(struct timeval *aTimer);
extern "C" void NS_ProcessTimeouts(void);

nsTimerXlib *nsTimerXlib::gTimerList = NULL;
struct timeval nsTimerXlib::gTimer = {0, 0};
struct timeval nsTimerXlib::gNextFire = {0, 0};

nsTimerXlib::nsTimerXlib()
{
  //printf("nsTimerXlib::nsTimerXlib (%p) called.\n",
  //this);
  NS_INIT_REFCNT();
  mFunc = NULL;
  mCallback = NULL;
  mNext = NULL;
  mClosure = NULL;
}

nsTimerXlib::~nsTimerXlib()
{
  //printf("nsTimerXlib::~nsTimerXlib (%p) called.\n",
  //       this);
  Cancel();
  NS_IF_RELEASE(mCallback);
}

NS_IMPL_ISUPPORTS(nsTimerXlib, kITimerIID)

nsresult
nsTimerXlib::Init(nsTimerCallbackFunc aFunc,
                void *aClosure,
                PRUint32 aDelay)
{
  mFunc = aFunc;
  mClosure = aClosure;
  return Init(aDelay);
}

nsresult 
nsTimerXlib::Init(nsITimerCallback *aCallback,
                PRUint32 aDelay)
{
    mCallback = aCallback;
    NS_ADDREF(mCallback);

    return Init(aDelay);
}

nsresult
nsTimerXlib::Init(PRUint32 aDelay)
{
  struct timeval Now;
  //  printf("nsTimerXlib::Init (%p) called with delay %d\n",
  //this, aDelay);
  // get the cuurent time
  gettimeofday(&Now, NULL);
  mFireTime.tv_sec = Now.tv_sec + (aDelay / 1000);
  mFireTime.tv_usec = Now.tv_usec + (aDelay * 1000);
  //printf("fire set to %ld / %ld\n",
  //mFireTime.tv_sec, mFireTime.tv_usec);
  // set the next pointer to nothing.
  mNext = NULL;
  // add ourself to the list
  if (!gTimerList) {
    // no list here.  I'm the start!
    //printf("This is the beginning of the list..\n");
    gTimerList = this;
  }
  else {
    // is it before everything else on the list?
    if ((mFireTime.tv_sec < gTimerList->mFireTime.tv_sec) &&
        (mFireTime.tv_usec < gTimerList->mFireTime.tv_usec)) {
      //      printf("This is before the head of the list...\n");
      mNext = gTimerList;
      gTimerList = this;
    }
    else {
      nsTimerXlib *pPrev = gTimerList;
      nsTimerXlib *pCurrent = gTimerList;
      while (pCurrent && ((pCurrent->mFireTime.tv_sec <= mFireTime.tv_sec) &&
                          (pCurrent->mFireTime.tv_usec <= mFireTime.tv_usec))) {
        pPrev = pCurrent;
        pCurrent = pCurrent->mNext;
      }
      PR_ASSERT(pPrev);

      // isnert it after pPrev ( this could be at the end of the list)
      mNext = pPrev->mNext;
      pPrev->mNext = this;
    }
  }
  NS_ADDREF(this);
  return NS_OK;
}

void
nsTimerXlib::Fire(struct timeval *aNow)
{
  //  printf("nsTimerXlib::Fire (%p) called at %ld / %ld\n",
  //         this,
  //aNow->tv_sec, aNow->tv_usec);
  if (mFunc != NULL) {
    (*mFunc)(this, mClosure);
  }
  else if (mCallback != NULL) {
    mCallback->Notify(this);
  }
}

void
nsTimerXlib::Cancel()
{
  nsTimerXlib *me = this;
  nsTimerXlib *p;
  //  printf("nsTimerXlib::Cancel (%p) called.\n",
  //         this);
  if (gTimerList == this) {
    // first element in the list lossage...
    gTimerList = mNext;
  }
  else {
    // walk until there's no next pointer
    for (p = gTimerList; p && p->mNext && (p->mNext != this); p = p->mNext)
      ;

    // if we found something valid pull it out of the list
    if (p && p->mNext && p->mNext == this) {
      p->mNext = mNext;
    }
    else {
      // get out before we delete something that looks bogus
      return;
    }
  }
  // if we got here it must have been a valid element so trash it
  NS_RELEASE(me);
  
}

void
nsTimerXlib::ProcessTimeouts(struct timeval *aNow)
{
  nsTimerXlib *p = gTimerList;
  if (aNow->tv_sec == 0 &&
      aNow->tv_usec == 0) {
    gettimeofday(aNow, NULL);
  }
  //  printf("nsTimerXlib::ProcessTimeouts called at %ld / %ld\n",
  //         aNow->tv_sec, aNow->tv_usec);
  while (p) {
    if ((p->mFireTime.tv_sec < aNow->tv_sec) ||
        ((p->mFireTime.tv_sec == aNow->tv_sec) &&
         (p->mFireTime.tv_usec <= aNow->tv_usec))) {
      //  Make sure that the timer cannot be deleted during the
      //  Fire(...) call which may release *all* other references
      //  to p...
      //printf("Firing timeout for (%p)\n",
      //           p);
      NS_ADDREF(p);
      p->Fire(aNow);
      //  Clear the timer.
      //  Period synced.
      p->Cancel();
      NS_RELEASE(p);
      //  Reset the loop (can't look at p->pNext now, and called
      //      code may have added/cleared timers).
      //  (could do this by going recursive and returning).
      p = gTimerList;
    }
    else {
      p = p->mNext;
    }
  }
}

int NS_TimeToNextTimeout(struct timeval *aTimer) {
  nsTimerXlib *timer;
  timer = nsTimerXlib::gTimerList;
  if (timer) {
    if ((timer->mFireTime.tv_sec < aTimer->tv_sec) ||
        ((timer->mFireTime.tv_sec == aTimer->tv_sec) &&
         (timer->mFireTime.tv_usec <= aTimer->tv_usec))) {
      aTimer->tv_sec = 0;
      aTimer->tv_usec = 0;
      return 1;
    }
    else {
      aTimer->tv_sec -= timer->mFireTime.tv_sec;
      // handle the overflow case
      if (aTimer->tv_usec < timer->mFireTime.tv_usec) {
        aTimer->tv_usec = timer->mFireTime.tv_usec - aTimer->tv_usec;
        // make sure we don't go past zero when we decrement
        if (aTimer->tv_sec)
          aTimer->tv_sec--;
      }
      else {
        aTimer->tv_usec -= timer->mFireTime.tv_usec;
      }
      return 1;
    }
  }
  else {
    return 0;
  }
}

void NS_ProcessTimeouts(void) {
  struct timeval now;
  now.tv_sec = 0;
  now.tv_usec = 0;
  nsTimerXlib::ProcessTimeouts(&now);
}

NS_TIMER nsresult NS_NewTimer(nsITimer** aInstancePtrResult)
{
    NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
    if (nsnull == aInstancePtrResult) {
      return NS_ERROR_NULL_POINTER;
    }  

    nsTimerXlib *timer = new nsTimerXlib();
    if (nsnull == timer) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return timer->QueryInterface(kITimerIID, (void **) aInstancePtrResult);
}
