/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2001 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#ifndef nsTimerImpl_h___
#define nsTimerImpl_h___

#define NS_TIMER_CONTRACTID "@mozilla.org/timer;1"

//#define FORCE_PR_LOG /* Allow logging in the release build */

#include "nsITimer.h"
#include "nsITimerCallback.h"

#include "nsIThread.h"

#include "nsCOMPtr.h"

#include "prlog.h"

#if defined(PR_LOGGING)
// stupid NS_DECL_LOG crap
#ifdef PRLogModuleInfo
#undef PRLogModuleInfo
#endif
#ifdef PR_NewLogModule
#undef PR_NewLogModule
#endif

static PRLogModuleInfo *gTimerLog = PR_NewLogModule("nsTimerImpl");
#define DEBUG_TIMERS
#else
#undef DEBUG_TIMERS
#endif

#define NS_TIMER_CLASSNAME "Timer"
#define NS_TIMER_CID \
{ /* 5ff24248-1dd2-11b2-8427-fbab44f29bc8 */         \
     0x5ff24248,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0x84, 0x27, 0xfb, 0xab, 0x44, 0xf2, 0x9b, 0xc8} \
}

enum {
  CALLBACK_TYPE_UNKNOWN   = 0,
  CALLBACK_TYPE_INTERFACE = 1,
  CALLBACK_TYPE_FUNC      = 2
};

class nsTimerImpl : public nsITimer 
{
public:

  nsTimerImpl();
  virtual ~nsTimerImpl();

  static void Shutdown();

  friend class TimerThread;

  void Fire();
  void Process();
  void SetDelayInternal(PRUint32 aDelay);

  NS_IMETHOD Init(nsTimerCallbackFunc aFunc,
                  void *aClosure,
                  PRUint32 aDelay,
                  PRUint32 aPriority,
                  PRUint32 aType);

  NS_IMETHOD Init(nsITimerCallback *aCallback,
                  PRUint32 aDelay,
                  PRUint32 aPriority,
                  PRUint32 aType);

  NS_DECL_ISUPPORTS

  NS_IMETHOD_(void) Cancel();

  NS_IMETHOD_(PRUint32) GetDelay() { return mDelay; }
  NS_IMETHOD_(void) SetDelay(PRUint32 aDelay);

  NS_IMETHOD_(PRUint32) GetPriority() { return (PRUint32)mPriority; }
  NS_IMETHOD_(void) SetPriority(PRUint32 aPriority);

  NS_IMETHOD_(PRUint32) GetType() { return (PRUint32)mType; }
  NS_IMETHOD_(void) SetType(PRUint32 aType);

  NS_IMETHOD_(void*) GetClosure() { return mClosure; }

private:
  nsCOMPtr<nsIThread>  mCallingThread;

  void *               mClosure;

  union {
    nsTimerCallbackFunc c;
    nsITimerCallback *  i;
  } mCallback;

  PRUint8              mCallbackType;
  PRUint8              mPriority;
  PRUint8              mType;
  PRUint8              mFiring;

  PRBool               mCancelled;

  PRUint32             mDelay;
  PRIntervalTime       mTimeout;

#ifdef DEBUG_TIMERS
  PRIntervalTime  mStart, mStart2;
  static double   sDeltaSum;
  static double   sDeltaSumSquared;
  static double   sNum;
#endif

};

#endif /* nsTimerImpl_h___ */
