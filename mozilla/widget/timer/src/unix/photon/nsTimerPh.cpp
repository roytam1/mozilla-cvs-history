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
 *  Alexander Larsson (alla@lysator.liu.se)
 *  Jerry L. Kirk (Jerry.Kirk@Nexwarecorp.com)
 */

#include "nsTimerPh.h"

/* Use the Widget Debug log */
#include "prlog.h"
#include <errno.h>
struct PRLogModuleInfo  *PhTimLog =  nsnull;


PRBool nsTimerPh::FireTimeout()
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::FireTimeout this=<%p>\n", this));

  if (mFunc != NULL) {
    (*mFunc)(this, mClosure);
  }
  else if (mCallback != NULL) {
    mCallback->Notify(this); // Fire the timer
  }

	/* FIXME: the use of REPEATING_SLACK is not know, more testing should be done.  */
  return (mType == NS_TYPE_REPEATING_SLACK || (mType == NS_TYPE_REPEATING_PRECISE));
}

void nsTimerPh::SetDelay(PRUint32 aDelay)
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::SetDelay this=<%p> aDelay=<%d>\n", this, aDelay));

  if (aDelay!=mDelay)
  {
   int err;
    Cancel();
    mDelay=aDelay;
    err=SetupTimer();
  }
};

void nsTimerPh::SetPriority(PRUint32 aPriority)
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::SetPriority this=<%p> aPriority=<%d>\n", this, aPriority));

  if (aPriority!=mPriority)
  {
   int err;
    Cancel();
    mPriority = aPriority;
    err=SetupTimer();
  }
}

void nsTimerPh::SetType(PRUint32 aType)
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::SetType this=<%p> aType=<%d>\n", this, aType));

  if (aType!=mType)
  {
   int err;
    Cancel();
    mType = aType;
    err=SetupTimer();
  }
}

nsTimerPh::nsTimerPh()
{

#ifdef DEBUG
  if (!PhTimLog)
  {
    PhTimLog =  PR_NewLogModule("PhTimLog");
  }
#endif

  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::nsTimerPh this=<%p>\n", this));

  NS_INIT_REFCNT();
  mFunc = NULL;
  mCallback = NULL;
  mNext = NULL;
  mTimerId = -1;
  mDelay = 0;
  mClosure = NULL;
  mPriority = 0;
  mType = NS_TYPE_ONE_SHOT;
  mPulsePid = 0;
  mPulseMsgId = NULL;
  mInputId = NULL;
}

nsTimerPh::~nsTimerPh()
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::~nsTimerPh this=<%p>\n", this));

  Cancel();
  NS_IF_RELEASE(mCallback);
}

nsresult 
nsTimerPh::Init(nsTimerCallbackFunc aFunc,
                 void *aClosure,
                 PRUint32 aDelay,
                 PRUint32 aPriority,
                 PRUint32 aType
                )
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::Init this=<%p> aClosure=<%p> aDelay=<%d> aPriority=<%d> aType=<%d>\n", this, aClosure,
     aDelay, aPriority, aType));
  int err;
  
    mFunc = aFunc;
    mClosure = aClosure;
    mPriority = aPriority;
    mType = aType;
    mDelay = aDelay;

    err = SetupTimer();
    //mTimerId = g_timeout_add_full(calc_priority(mPriority),mDelay, nsTimerExpired, this, NULL);
 
    return err;
}

nsresult 
nsTimerPh::Init(nsITimerCallback *aCallback,
                 PRUint32 aDelay,
                 PRUint32 aPriority,
                 PRUint32 aType
                 )
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::Init this=<%p> aDelay=<%d> aPriority=<%d> aType=<%d>\n", this,
     aDelay, aPriority, aType));

  int err;
    mCallback = aCallback;
    NS_ADDREF(mCallback);
    mPriority = aPriority;
    mType = aType;
    mDelay = aDelay;

    err = SetupTimer();
    //mTimerId = g_timeout_add_full(calc_priority(mPriority), mDelay, nsTimerExpired, this, NULL);
 
    return err;
}

NS_IMPL_ISUPPORTS1(nsTimerPh, nsITimer)

void
nsTimerPh::Cancel()
{
  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::Cancel this=<%p>\n", this));
 int err;
  
  if( mTimerId >= 0)
  {
    err = timer_delete( mTimerId );
    if (err < 0)
    {
      char buf[256];
	  sprintf(buf, "TimerImpl::Cancel Failed in timer_delete mTimerId=<%d> err=<%d> errno=<%d>", mTimerId, err, errno);
      //NS_ASSERTION(0,"TimerImpl::Cancel Failed in timer_delete");
      NS_ASSERTION(0,buf);
      return;
    }
	
    mTimerId = -1;
  }

  if( mInputId )
  {
    PtAppRemoveInput( NULL, mInputId );
    mInputId = NULL;
  }

  if( mPulseMsgId )
  {
    PtPulseDisarm( mPulseMsgId );
    mPulseMsgId = NULL;
  }

  if( mPulsePid )
  {
    PtAppDeletePulse( NULL, mPulsePid );
    mPulsePid = 0;
  }
}

// This is the timer handler that gets called by the Photon
// input proc

int nsTimerPh::TimerEventHandler( void *aData, pid_t aRcvId, void *aMsg, size_t aMsgLen )
{
  int localTimerId;
  nsTimerPh* timer = (nsTimerPh *)aData;

  PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::TimerEventHandler this=<%p>\n", aData));
  NS_ASSERTION(timer, "nsTimerPh::TimerEventHandler is NULL!");

  // because Notify can cause 'timer' to get destroyed, we need to hold a ref
  nsCOMPtr<nsITimer> kungFuDeathGrip = timer;

  localTimerId = timer->mTimerId;
	
  if( timer->mFunc != NULL )
  {
    (*timer->mFunc)( timer, timer->mClosure );
  }
  else if ( timer->mCallback != NULL )
  {
    timer->mCallback->Notify( timer );
  }

  return Pt_CONTINUE;
}



NS_METHOD nsTimerPh::SetupTimer()
{
	PR_LOG(PhTimLog, PR_LOG_DEBUG, ("nsTimerPh::SetupTimer this=<%p>\n", this));

	struct itimerspec  tv;
	int err;

	if( mPulsePid )
	{
		NS_ASSERTION(0,"TimerImpl::SetupTimer - reuse of timer not allowed!");
		return NS_ERROR_FAILURE;
	}
	//int CurrentPriority = 10;		/* How Do I get this?? */
	/* We get the current priority of the thread, by calling getprio(0) which is Neutrino specific.  */
	int CurrentPriority = getprio (0);
	int NewPriority = CurrentPriority;
  
	switch(mPriority)
	{
		case NS_PRIORITY_LOWEST:
		NewPriority = CurrentPriority - 2;
		break;  

		case NS_PRIORITY_LOW:
		NewPriority = CurrentPriority - 1;
		break;  

		case NS_PRIORITY_NORMAL:
		NewPriority = CurrentPriority;
		break;  

		case NS_PRIORITY_HIGH:
		NewPriority = CurrentPriority +1;
		break;  

		case NS_PRIORITY_HIGHEST:
		NewPriority = CurrentPriority + 2;
		break;  
	}

	if(( mPulsePid = PtAppCreatePulse( NULL, NewPriority )) == 0 )
	{
		NS_ASSERTION(0,"TimerImpl::SetupTimer - failed to create pulse");
		return NS_ERROR_FAILURE;
	}
	if(( mPulseMsgId = PtPulseArmPid( NULL, mPulsePid, getpid(), &mPulseMsg )) == NULL )
	{
		NS_ASSERTION(0,"TimerImpl::SetupTimer - failed to arm pulse!");
		return NS_ERROR_FAILURE;
	}
	if(( mInputId = PtAppAddInput( NULL, mPulsePid, TimerEventHandler, this )) == NULL )
	{
		NS_ASSERTION(0,"TimerImpl::SetupTimer - failed to add input handler!");
		return NS_ERROR_FAILURE;
	}

	mTimerId = -1;
	err = timer_create( CLOCK_MONOTONIC, &mPulseMsg, &mTimerId );
	if( err != 0 )
	{ 
		NS_ASSERTION(0,"TimerImpl::SetupTimer - timer_create error");
		return NS_ERROR_FAILURE;
	}

	switch(mType)
	{
		case NS_TYPE_ONE_SHOT:
		tv.it_interval.tv_sec  = 0;
		tv.it_interval.tv_nsec = 0;
		break;
		case NS_TYPE_REPEATING_SLACK:									/* HACK WRONG! */
		case NS_TYPE_REPEATING_PRECISE:
		tv.it_interval.tv_sec     = ( mDelay / 1000 );
		tv.it_interval.tv_nsec    = ( mDelay % 1000 ) * 1000000L;   
		break;
	}
	tv.it_value.tv_sec     = ( mDelay / 1000 );
	tv.it_value.tv_nsec    = ( mDelay % 1000 ) * 1000000L;

	/* If delay is set to 0 seconds then change it to 1 nsec. */
	if ( (tv.it_value.tv_sec == 0) && (tv.it_value.tv_nsec == 0))
	{
		tv.it_value.tv_nsec = 1;
	}

	err = timer_settime( mTimerId, 0, &tv, NULL );
	if( err != 0 )
	{
		NS_ASSERTION(0,"TimerImpl::SetupTimer timer_settime");
		return NS_ERROR_FAILURE;
	}
  
	return NS_OK;
}


#ifdef MOZ_MONOLITHIC_TOOLKIT
nsresult NS_NewTimer(nsITimer** aInstancePtrResult)
{
    NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
    if (nsnull == aInstancePtrResult) {
      return NS_ERROR_NULL_POINTER;
    }  

    nsTimerPh *timer = new nsTimerPh();
    if (nsnull == timer) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return CallQueryInterface(timer, aInstancePtrResult);
}

int NS_TimeToNextTimeout(struct timeval *aTimer) 
{
  return 0;
}

void NS_ProcessTimeouts(void) 
{
}
#endif /* MOZ_MONOLITHIC_TOOLKIT */
