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
 *  Stuart Parmenter <pavlov@netscape.com>
 *  Alexander Larsson (alla@lysator.liu.se)
 *  Christopher Blizzard <blizzard@mozilla.org>
 */

#define INTERVAL 10

#include "nsVoidArray.h"
#include "nsTimerGtk.h"
#include "nsCOMPtr.h"

static NS_DEFINE_IID(kITimerIID, NS_ITIMER_IID);

extern "C" gboolean nsTimerExpired(gpointer aCallData);



TimeVal::TimeVal()
{
  mSeconds = 0;
  mUSeconds = 0; 
}

TimeVal::TimeVal(const TimeVal &tv)
{
  mSeconds = tv.mSeconds;
  mUSeconds = tv.mUSeconds;
}

TimeVal::~TimeVal()
{

}

void TimeVal::Set(PRUint32 sec, PRUint32 usec)
{
  mSeconds = sec;
  mUSeconds = usec;
}

TimeVal& TimeVal::operator+=(PRInt32 msec) {
  mSeconds += (PRUint32)(msec / 1000);
  mUSeconds += (msec % 1000) * 1000;

  if (mUSeconds > 1000000) {
    mUSeconds -= 1000000;
    mSeconds ++;
  }

  return *this;
}

TimeVal operator+(const TimeVal& lhs, PRInt32 rhs)
{
  return TimeVal(lhs) += rhs;
}

TimeVal operator+(PRInt32 lhs, const TimeVal& rhs)
{
  return TimeVal(rhs) += lhs;
}

TimeVal& TimeVal::operator=(const struct timeval &tv)
{
  mSeconds = tv.tv_sec;
  mUSeconds = tv.tv_usec;
  return *this;
}

PRBool TimeVal::operator==(const TimeVal &tv) const
{
  return ((this->mSeconds == tv.mSeconds) && (this->mUSeconds == tv.mUSeconds));
}

PRBool TimeVal::operator==(const struct timeval &tv) const
{
  return ((this->mSeconds == (PRUint32)tv.tv_sec) && (this->mUSeconds == (PRUint32)tv.tv_usec));
}

PRBool TimeVal::operator<(const TimeVal &tv) const
{
  if (this->mSeconds == tv.mSeconds)
    return (this->mUSeconds < tv.mUSeconds);

  return (this->mSeconds < tv.mSeconds);
}

PRBool TimeVal::operator>(const TimeVal &tv) const
{
  if (this->mSeconds == tv.mSeconds)
    return (this->mUSeconds > tv.mUSeconds);

  return (this->mSeconds > tv.mSeconds);
}

PRBool TimeVal::operator<(const struct timeval &tv) const
{
  if (this->mSeconds == (PRUint32)tv.tv_sec)
    return (this->mUSeconds < (PRUint32)tv.tv_usec);

  return (this->mSeconds < (PRUint32)tv.tv_sec);
}

PRBool TimeVal::operator>(const struct timeval &tv) const
{
  if (this->mSeconds == (PRUint32)tv.tv_sec)
    return (this->mUSeconds > (PRUint32)tv.tv_usec);

  return (this->mSeconds > (PRUint32)tv.tv_sec);
}


PRBool TimeVal::operator>=(const TimeVal &tv) const
{
  if (this->operator>(tv) == PR_TRUE)
    return PR_TRUE;
  else if (this->operator==(tv) == PR_TRUE)
    return PR_TRUE;

  return PR_FALSE;
}

PRBool TimeVal::operator<=(const TimeVal &tv) const
{
  if (this->operator<(tv) == PR_TRUE)
    return PR_TRUE;
  else if (this->operator==(tv) == PR_TRUE)
    return PR_TRUE;

  return PR_FALSE;
}


PRBool TimeVal::operator>=(const struct timeval &tv) const
{
  if (this->operator<(tv) == PR_TRUE)
    return PR_TRUE;
  else if (this->operator==(tv) == PR_TRUE)
    return PR_TRUE;

  return PR_FALSE;
}

PRBool TimeVal::operator<=(const struct timeval &tv) const
{
  if (this->operator<(tv) == PR_TRUE)
    return PR_TRUE;
  else if (this->operator==(tv) == PR_TRUE)
    return PR_TRUE;

  return PR_FALSE;
}



PRBool nsTimerGtk::FireTimeout()
{
  //  printf("%p FireTimeout() priority = %i\n", this, mPriority);
  // because Notify can cause 'this' to get destroyed, we need to hold a ref
  nsCOMPtr<nsITimer> kungFuDeathGrip = this;
  
  if (mFunc != NULL) {
    (*mFunc)(this, mClosure);
  }
  else if (mCallback != NULL) {
    mCallback->Notify(this); // Fire the timer
  }
  
  return ((mType == NS_TYPE_REPEATING_SLACK) || (mType == NS_TYPE_REPEATING_PRECISE));
}

void nsTimerGtk::SetDelay(PRUint32 aDelay)
{
  mDelay = aDelay;
}

void nsTimerGtk::SetPriority(PRUint32 aPriority)
{
  mPriority = aPriority;
}

void nsTimerGtk::SetType(PRUint32 aType)
{
  mType = aType;
}

nsVoidArray *nsTimerGtk::gHighestList = (nsVoidArray *)nsnull;
nsVoidArray *nsTimerGtk::gHighList = (nsVoidArray *)nsnull;
nsVoidArray *nsTimerGtk::gNormalList = (nsVoidArray *)nsnull;
nsVoidArray *nsTimerGtk::gLowList = (nsVoidArray *)nsnull;
nsVoidArray *nsTimerGtk::gLowestList = (nsVoidArray *)nsnull;
PRBool nsTimerGtk::gTimeoutAdded = PR_FALSE;
PRBool nsTimerGtk::gProcessingTimer = PR_FALSE;
guint  nsTimerGtk::gTimerID = 0;

nsTimerGtk::nsTimerGtk()
{
  //  printf("nsTimerGtke::nsTimerGtk called for %p\n", this);
  NS_INIT_REFCNT();
  mFunc = NULL;
  mCallback = NULL;
  mDelay = 0;
  mClosure = NULL;
  mPriority = 0;
  mType = NS_TYPE_ONE_SHOT;
}

nsTimerGtk::~nsTimerGtk()
{
//  printf("nsTimerGtk::~nsTimerGtk called for %p\n", this);
  Cancel();
  NS_IF_RELEASE(mCallback);
}

/* inline */
void process_timers(nsVoidArray *array)
{
  int ret;

  PRInt32 count = array->Count();
  
  if (count == 0)
    return;
  
  nsTimerGtk *timer;
  int i;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  for( i = count; i >= 0; i--) {
    timer = (nsTimerGtk*)array->ElementAt(i);

    if (timer) {
      if (((timer->mSchedTime + timer->mDelay) <= tv)) {
        ret = timer->FireTimeout();
        if( ret == 0 ) {
          array->RemoveElement(timer);
        } else {
          struct timeval ntv;
          gettimeofday(&ntv, NULL);
          timer->mSchedTime = ntv;
        }
      }
    }
  }
}

int TimerCallbackFunc( gpointer data )
{
  NS_ASSERTION( nsTimerGtk::gProcessingTimer == PR_FALSE,
                "TimerCallbackFunc(): Timer reentrance" );

  nsTimerGtk::gProcessingTimer = PR_TRUE;

  process_timers(nsTimerGtk::gHighestList);
  process_timers(nsTimerGtk::gHighList);
  process_timers(nsTimerGtk::gNormalList);

  gboolean hasEvents = g_main_pending();
  if (hasEvents == FALSE) {
    process_timers(nsTimerGtk::gLowList);
    process_timers(nsTimerGtk::gLowestList);
  }
  
  nsTimerGtk::gProcessingTimer = PR_FALSE;
  return PR_TRUE;
}

nsresult nsTimerGtk::Init(nsTimerCallbackFunc aFunc,
                 void *aClosure,
                 PRUint32 aDelay,
                 PRUint32 aPriority,
                 PRUint32 aType)
{
  //printf("%p nsTimerGtk::Init() mDelay = %i\n", this, aDelay);
  mFunc = aFunc;
  mClosure = aClosure;
  mPriority = aPriority;
  mType = aType;
  mDelay = aDelay;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  mSchedTime = tv;

  if (!gTimeoutAdded) {
    nsTimerGtk::gHighestList = new nsVoidArray;
    nsTimerGtk::gHighList = new nsVoidArray;
    nsTimerGtk::gNormalList = new nsVoidArray;
    nsTimerGtk::gLowList = new nsVoidArray;
    nsTimerGtk::gLowestList = new nsVoidArray;
    gTimerID = gtk_timeout_add ( INTERVAL, TimerCallbackFunc, (gpointer)0);
    nsTimerGtk::gTimeoutAdded = PR_TRUE;
  }

  switch (aPriority)
  {
  case NS_PRIORITY_HIGHEST:
    nsTimerGtk::gHighestList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_HIGH:
    nsTimerGtk::gHighList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_NORMAL:
    nsTimerGtk::gNormalList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_LOW:
    nsTimerGtk::gLowList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_LOWEST:
    nsTimerGtk::gLowestList->InsertElementAt(this, 0);
    break;
  }
  
  return NS_OK;
}

nsresult nsTimerGtk::Init(nsITimerCallback *aCallback,
                 PRUint32 aDelay,
                 PRUint32 aPriority,
                 PRUint32 aType
                 )
{
  mCallback = aCallback;
  NS_ADDREF(mCallback);
  mPriority = aPriority;
  mType = aType;
  mDelay = aDelay;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  mSchedTime = tv;
  
  if (!gTimeoutAdded) {
    nsTimerGtk::gHighestList = new nsVoidArray;
    nsTimerGtk::gHighList = new nsVoidArray;
    nsTimerGtk::gNormalList = new nsVoidArray;
    nsTimerGtk::gLowList = new nsVoidArray;
    nsTimerGtk::gLowestList = new nsVoidArray;
    gTimerID = gtk_timeout_add (INTERVAL, TimerCallbackFunc, (gpointer)0);
    nsTimerGtk::gTimeoutAdded = PR_TRUE;
  }
  
  switch (aPriority)
  {
  case NS_PRIORITY_HIGHEST:
    nsTimerGtk::gHighestList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_HIGH:
    nsTimerGtk::gHighList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_NORMAL:
    nsTimerGtk::gNormalList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_LOW:
    nsTimerGtk::gLowList->InsertElementAt(this, 0);
    break;
  case NS_PRIORITY_LOWEST:
    nsTimerGtk::gLowestList->InsertElementAt(this, 0);
    break;
  }

  return NS_OK;
}

void nsTimerGtk::Shutdown()
{
    if (gTimeoutAdded) {
      gtk_timeout_remove(gTimerID);
      gTimerID = 0;
    }

    delete nsTimerGtk::gHighestList;
    nsTimerGtk::gHighestList = nsnull;

    delete nsTimerGtk::gHighList;
    nsTimerGtk::gHighList = nsnull;

    delete nsTimerGtk::gNormalList;
    nsTimerGtk::gNormalList = nsnull;

    delete nsTimerGtk::gLowList;
    nsTimerGtk::gLowList = nsnull;

    delete nsTimerGtk::gLowestList;
    nsTimerGtk::gLowestList = nsnull;

    nsTimerGtk::gTimeoutAdded = PR_FALSE;
    nsTimerGtk::gProcessingTimer = PR_FALSE;
}

NS_IMPL_ISUPPORTS1(nsTimerGtk, nsITimer)

void nsTimerGtk::Cancel()
{
  switch (mPriority)
  {
  case NS_PRIORITY_HIGHEST:
    nsTimerGtk::gHighestList->RemoveElement(this);
    break;
  case NS_PRIORITY_HIGH:
    nsTimerGtk::gHighList->RemoveElement(this);
    break;
  case NS_PRIORITY_NORMAL:
    nsTimerGtk::gNormalList->RemoveElement(this);
    break;
  case NS_PRIORITY_LOW:
    nsTimerGtk::gLowList->RemoveElement(this);
    break;
  case NS_PRIORITY_LOWEST:
    nsTimerGtk::gLowestList->RemoveElement(this);
    break;
  }
}

gboolean nsTimerExpired(gpointer aCallData)
{
  nsTimerGtk* timer = (nsTimerGtk *)aCallData;
  return timer->FireTimeout();
}

#ifdef MOZ_MONOLITHIC_TOOLKIT
nsresult NS_NewTimer(nsITimer** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }  
  
  nsTimerGtk *timer = new nsTimerGtk();
  if (nsnull == timer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return timer->QueryInterface(kITimerIID, (void **) aInstancePtrResult);
}

int NS_TimeToNextTimeout(struct timeval *aTimer) 
{
  return 0;
}

void NS_ProcessTimeouts(void) 
{
}
#endif /* MOZ_MONOLITHIC_TOOLKIT */
