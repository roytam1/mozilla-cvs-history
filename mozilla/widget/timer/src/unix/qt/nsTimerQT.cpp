/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsCRT.h"
#include "prlog.h"
#include <stdio.h>
#include <limits.h>
#include "nsTimerQT.h"
#include <qtimer.h>

static NS_DEFINE_IID(kITimerIID, NS_ITIMER_IID);

nsTimerEventHandler::nsTimerEventHandler(nsITimer * aTimer,
                                         nsTimerCallbackFunc aFunc,
                                         void *aClosure,
                                         nsITimerCallback *aCallback)
{
    mTimer    = aTimer;
    mFunc     = aFunc;
    mClosure  = aClosure;
    mCallback = aCallback;
}
    
void nsTimerEventHandler::FireTimeout()
{
    //debug("nsTimerEventHandler::FireTimeout called");
    if (mFunc != NULL) 
    {
        (*mFunc)(mTimer, mClosure);
    }
    else if (mCallback != NULL) 
    {
        mCallback->Notify(mTimer); // Fire the timer
    }
    
// Always repeating here
    
// if (mRepeat)
//  mTimerId = gtk_timeout_add(aDelay, nsTimerExpired, this);
}

TimerImpl::TimerImpl() //: QObject()
{
    //debug("TimerImpl::TimerImpl called for %p", this);
    NS_INIT_REFCNT();
    mFunc         = nsnull;
    mCallback     = nsnull;
    mNext         = nsnull;
    mTimer        = nsnull;
    mDelay        = 0;
    mClosure      = nsnull;
    mEventHandler = nsnull;
}

TimerImpl::~TimerImpl()
{
    //debug("TimerImpl::~TimerImpl called for %p", this);
    Cancel();
    NS_IF_RELEASE(mCallback);
    if (mEventHandler);
    {
        delete mEventHandler;
    }

    if (mTimer)
    {
      delete mTimer;
    }
}

nsresult 
TimerImpl::Init(nsTimerCallbackFunc aFunc,
                void *aClosure,
//              PRBool aRepeat, 
                PRUint32 aDelay)
{
    //debug("TimerImpl::Init called with func + closure with %u delay", aDelay);
    mFunc = aFunc;
    mClosure = aClosure;
    // mRepeat = aRepeat;

    if ((aDelay > 10000) || (aDelay < 0)) 
    {
        printf("Timer::Init() called with bogus value \"%d\"!  Not enabling timer.",
               aDelay);
        return Init(aDelay);
    }

    return Init(aDelay);
}

nsresult 
TimerImpl::Init(nsITimerCallback *aCallback,
//              PRBool aRepeat, 
                PRUint32 aDelay)
{
    //debug("TimerImpl::Init called with callback only with %u delay", aDelay);
    mCallback = aCallback;
    NS_ADDREF(mCallback);
    // mRepeat = aRepeat;
    if ((aDelay > 10000) || (aDelay < 0)) 
    {
        printf("Timer::Init() called with bogus value \"%d\"!  Not enabling timer.",
               aDelay);
        return NS_OK;
        //return Init(aDelay);
    }

    return Init(aDelay);
}

nsresult
TimerImpl::Init(PRUint32 aDelay)
{
    //debug("TimerImpl::Init called with delay %d only for %p", aDelay, this);

    mEventHandler = new nsTimerEventHandler(this, mFunc, mClosure, mCallback);

    mTimer = new QTimer();
    
    if (!mTimer) 
    {
        return NS_ERROR_NOT_INITIALIZED;
    }
    QObject::connect((QTimer *)mTimer, 
                     SIGNAL(timeout()), 
                     mEventHandler, 
                     SLOT(FireTimeout()));
    mTimer->start(aDelay);

    mDelay = aDelay;
    //NS_ADDREF(this);

    return NS_OK;
}

NS_IMPL_ISUPPORTS(TimerImpl, kITimerIID)


void
TimerImpl::Cancel()
{
    //debug("TimerImpl::Cancel called for %p", this);
    if (mTimer) 
    {
        mTimer->stop();
    }
}

NS_BASE nsresult NS_NewTimer(nsITimer** aInstancePtrResult)
{
    NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
    if (nsnull == aInstancePtrResult) 
    {
        return NS_ERROR_NULL_POINTER;
    }  

    TimerImpl *timer = new TimerImpl();
    if (nsnull == timer) 
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return timer->QueryInterface(kITimerIID, (void **) aInstancePtrResult);
}
