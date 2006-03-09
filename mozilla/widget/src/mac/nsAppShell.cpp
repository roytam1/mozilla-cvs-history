/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/* vim:set ts=2 sw=2 sts=2 ci et: */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
 *
 * ***** END LICENSE BLOCK ***** */

// 
// nsAppShell
//
// This file contains the default implementation of the application shell. Clients
// may either use this implementation or write their own. If you write your
// own, you must create a message sink to route events to. (The message sink
// interface may change, so this comment must be updated accordingly.)
//

#include "nsAppShell.h"
#include "nsIAppShell.h"

#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIWidget.h"
#include "nsMacMessagePump.h"
#include "nsToolkit.h"
#include <Quickdraw.h>
#include <Fonts.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>

#include <stdlib.h>

PRBool nsAppShell::mInitializedToolbox = PR_FALSE;


//-------------------------------------------------------------------------
//
// Create the application shell
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsAppShell::Init(int* argc, char ** argv)
{
  nsresult rv = NS_GetCurrentToolkit(getter_AddRefs(mToolkit));
  if (NS_FAILED(rv))
   return rv;

  nsIToolkit* toolkit = mToolkit.get();
  mMacPump.reset(new nsMacMessagePump(static_cast<nsToolkit*>(toolkit)));

  if (!mMacPump.get() || ! nsMacMemoryCushion::EnsureMemoryCushion())
    return NS_ERROR_OUT_OF_MEMORY;

  return nsBaseAppShell::Init(argc, argv);;
}

#if 0
//-------------------------------------------------------------------------
//
// Enter a message handler loop
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Run(void)
{
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  NS_ENSURE_STATE(thread);

  nsToolkit::AppInForeground();

  mExitCalled = PR_FALSE;
  while (!mExitCalled)
    thread->RunNextTask(nsIThread::RUN_NORMAL);

  NS_RunPendingTasks(thread);

  NS_RELEASE_THIS();  // hack: see below
  return NS_OK;

#if 0
	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	mMacPump->StartRunning();
	mMacPump->DoMessagePump();

	if (mExitCalled)	// hack: see below
	{
		--mRefCnt;
		if (mRefCnt == 0)
			delete this;
	}

  return NS_OK;
#endif
}

//-------------------------------------------------------------------------
//
// Exit appshell
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Exit(void)
{
  if (!mMacPump.get())
    return NS_OK;

  Spindown();
  mExitCalled = PR_TRUE;

  NS_ADDREF_THIS(); // hack: since the applications are likely to delete us
                    // after calling this method (see nsViewerApp::Exit()),
                    // we temporarily bump the refCnt to let the message pump
                    // exit properly. The object will delete itself afterwards.
	return NS_OK;
}
#endif

#if 0
//-------------------------------------------------------------------------
//
// respond to notifications that an event queue has come or gone
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen)
{ // unnecessary; handled elsewhere
  return NS_OK;
}
#endif

#if 0
//-------------------------------------------------------------------------
//
// Prepare to process events
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Spinup(void)
{
  NS_ENSURE_TRUE(mMacPump.get(), NS_ERROR_NOT_INITIALIZED);

	mMacPump->StartRunning();
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Stop being prepared to process events.
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsAppShell::Spindown(void)
{
	if (mMacPump.get())
		mMacPump->StopRunning();
	return NS_OK;
}
#endif

//-------------------------------------------------------------------------
//
// nsAppShell constructor
//
//-------------------------------------------------------------------------
nsAppShell::nsAppShell()
{

  mInitializedToolbox = PR_TRUE;
  mRefCnt = 0;
  mExitCalled = PR_FALSE;
}

#if 0
NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
	static EventRecord	theEvent;	// icky icky static (can't really do any better)

	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	aRealEvent = mMacPump->GetEvent(theEvent);
	aEvent = &theEvent;
	return NS_OK;
}

NS_METHOD
nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
	if (!mMacPump.get())
		return NS_ERROR_NOT_INITIALIZED;

	mMacPump->DispatchEvent(aRealEvent, (EventRecord *) aEvent);
	return NS_OK;
}
#endif

//-------------------------------------------------------------------------
//
// Thread observer methods
//
//-------------------------------------------------------------------------

#if 0
#if defined(XP_MACOSX)
#if defined(MOZ_WIDGET_COCOA)
#include <CoreFoundation/CoreFoundation.h>
#define MAC_USE_CFRUNLOOPSOURCE
#elif defined(TARGET_CARBON)
/* #include <CarbonEvents.h> */
/* #define MAC_USE_CARBON_EVENT */
#include <CoreFoundation/CoreFoundation.h>
#define MAC_USE_CFRUNLOOPSOURCE
#endif
#endif

#if defined(MAC_USE_CFRUNLOOPSOURCE)
    CFRunLoopSourceRef  mRunLoopSource;
    CFRunLoopRef        mMainRunLoop;
#elif defined(MAC_USE_CARBON_EVENT)
    EventHandlerUPP     eventHandlerUPP;
    EventHandlerRef     eventHandlerRef;
#endif

static void _md_CreateEventQueue( PLEventQueue *eventQueue )
{
#if defined(MAC_USE_CFRUNLOOPSOURCE)
    CFRunLoopSourceContext sourceContext = { 0 };
    sourceContext.version = 0;
    sourceContext.info = (void*)eventQueue;
    sourceContext.perform = _md_EventReceiverProc;

    /* make a run loop source */
    eventQueue->mRunLoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0 /* order */, &sourceContext);
    PR_ASSERT(eventQueue->mRunLoopSource);
    
    eventQueue->mMainRunLoop = CFRunLoopGetCurrent();
    CFRetain(eventQueue->mMainRunLoop);
    
    /* and add it to the run loop */
    CFRunLoopAddSource(eventQueue->mMainRunLoop, eventQueue->mRunLoopSource, kCFRunLoopCommonModes);

#elif defined(MAC_USE_CARBON_EVENT)
    eventQueue->eventHandlerUPP = NewEventHandlerUPP(_md_EventReceiverProc);
    PR_ASSERT(eventQueue->eventHandlerUPP);
    if (eventQueue->eventHandlerUPP)
    {
      EventTypeSpec     eventType;

      eventType.eventClass = kEventClassPL;
      eventType.eventKind  = kEventProcessPLEvents;

      InstallApplicationEventHandler(eventQueue->eventHandlerUPP, 1, &eventType,
                                     eventQueue, &eventQueue->eventHandlerRef);
      PR_ASSERT(eventQueue->eventHandlerRef);
    }
#endif
} /* end _md_CreateEventQueue() */

static PRStatus
_pl_NativeNotify(PLEventQueue* self)
{
#if defined(MAC_USE_CFRUNLOOPSOURCE)
  	CFRunLoopSourceSignal(self->mRunLoopSource);
  	CFRunLoopWakeUp(self->mMainRunLoop);
#elif defined(MAC_USE_CARBON_EVENT)
    OSErr err;
    EventRef newEvent;
    if (CreateEvent(NULL, kEventClassPL, kEventProcessPLEvents,
                    0, kEventAttributeNone, &newEvent) != noErr)
        return PR_FAILURE;
    err = SetEventParameter(newEvent, kEventParamPLEventQueue,
                            typeUInt32, sizeof(PREventQueue*), &self);
    if (err == noErr) {
        err = PostEventToQueue(GetMainEventQueue(), newEvent, kEventPriorityLow);
        ReleaseEvent(newEvent);
    }
    if (err != noErr)
        return PR_FAILURE;
#endif
    return PR_SUCCESS;
}
#endif

NS_IMETHODIMP nsAppShell::OnNewTask(nsIThreadInternal *thr, PRUint32 flags)
{
  // post a message to the native event queue...
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnBeforeRunNextTask(nsIThreadInternal *thr,
                                              PRUint32 flags)
{
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnAfterRunNextTask(nsIThreadInternal *thr,
                                             PRUint32 flags,
                                             nsresult status)
{
  // process any pending native events...
  return NS_OK;
}

NS_IMETHODIMP nsAppShell::OnWaitNextTask(nsIThreadInternal *thr,
                                         PRUint32 flags)
{
  // while (!thr->HasPendingTask())
  //   wait for and process native events
  return NS_OK;
}
