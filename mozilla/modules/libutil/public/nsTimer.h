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

#ifndef __NSTIMER_H
#define __NSTIMER_H

#ifdef MOZ_PERF_METRICS

#include "stopwatch.h"
#include "nsStackBasedTimer.h"

// This should be set from preferences at runtime.  For now, make it a compile time flag.
#define ENABLE_DEBUG_OUTPUT PR_FALSE

// Uncomment and re-build to use the Mac Instrumentation SDK on a Mac.
// #define MOZ_TIMER_USE_MAC_ISDK

// Uncomment and re-build to use Quantify on Windows
// #define MOZ_TIMER_USE_QUANTIFY

// Timer macros for the Mac
#ifdef XP_MAC

#ifdef MOZ_TIMER_USE_MAC_ISDK

#include "InstrumentationHelpers.h"
#  define MOZ_TIMER_DECLARE(name)  
#  define MOZ_TIMER_CREATE(name)   \
  static InstTraceClassRef name = 0;  StInstrumentationLog __traceLog("Creating name..."), name)

#  define MOZ_TIMER_RESET(name, msg)
#  define MOZ_TIMER_START(name, msg)
#  define MOZ_TIMER_STOP(name, msg)
#  define MOZ_TIMER_SAVE(name, msg)
#  define MOZ_TIMER_RESTORE(name, msg)
#  define MOZ_TIMER_LOG(msg) \
  do { __traceLog.LogMiddleEvent(); } while(0)

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data) \
  do { traceLog.LogMiddleEventWithData((msg), (data)); } while (0)

#else

#define MOZ_TIMER_USE_STOPWATCH

#endif  // MOZ_TIMER_USE_MAC_ISDK

#endif  // XP_MAC


// Timer macros for Windows
#ifdef XP_WIN

#ifdef MOZ_TIMER_USE_QUANTIFY

#include "pure.h"
#  define MOZ_TIMER_DECLARE(name)
#  define MOZ_TIMER_CREATE(name)
#  define MOZ_TIMER_RESET(name)  \
  QuantifyClearData()

#  define MOZ_TIMER_START(name)  \
  QuantifyStartRecordingData()
  
#  define MOZ_TIMER_STOP(name) \
  QuantifyStopRecordingData()

#  define MOZ_TIMER_SAVE(name)
#  define MOZ_TIMER_RESTORE(name)

#  define MOZ_TIMER_PRINT(name)
 
#  define MOZ_TIMER_LOG(msg)    \ 
do {                            \
  char* str = __mysprintf msg;  \
  QuantifyAddAnnotation(str);   \
  delete [] str;                \
} while (0)

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)

#else

#define MOZ_TIMER_USE_STOPWATCH

#endif  // MOZ_TIMER_USE_QUANTIFY

#endif  // XP_WIN

// Timer macros for Unix
#ifdef XP_UNIX

#define MOZ_TIMER_USE_STOPWATCH

#endif  // XP_UNIX

#ifdef MOZ_TIMER_USE_STOPWATCH

#  define MOZ_TIMER_DECLARE(name)  \
  Stopwatch name;

#  define MOZ_TIMER_CREATE(name)    \
  static Stopwatch __sw_name;  nsStackBasedTimer name(&__sw_name)

#  define MOZ_TIMER_RESET(name)  \
  name.Reset();

#  define MOZ_TIMER_START(name)  \
  name.Start(PR_FALSE);

#  define MOZ_TIMER_STOP(name) \
  name.Stop();

#  define MOZ_TIMER_SAVE(name) \
  name.SaveState();

#  define MOZ_TIMER_RESTORE(name)  \
  name.RestoreState();

#  define MOZ_TIMER_PRINT(name)   \
  name.Print();

#  define MOZ_TIMER_LOG(msg)  \
  printf msg

#  define MOZ_TIMER_DEBUGLOG(msg) \
  if (ENABLE_DEBUG_OUTPUT) printf msg

#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)

#endif // MOZ_TIMER_USE_STOPWATCH

#else
#  define MOZ_TIMER_DECLARE(name)
#  define MOZ_TIMER_CREATE(name)
#  define MOZ_TIMER_RESET(name)
#  define MOZ_TIMER_START(name)
#  define MOZ_TIMER_STOP(name)
#  define MOZ_TIMER_SAVE(name)
#  define MOZ_TIMER_RESTORE(name)
#  define MOZ_TIMER_PRINT(name)
#  define MOZ_TIMER_LOG(msg)
#  define MOZ_TIMER_DEBUGLOG(msg)
#  define MOZ_TIMER_MACISDK_LOGDATA(msg, data)
#endif  // MOZ_PERF_METRICS

#endif  // __NSTIMER_H

