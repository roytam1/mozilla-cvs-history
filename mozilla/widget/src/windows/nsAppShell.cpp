/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/*
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
 *  Michael Lowe <michael.lowe@bigfoot.com>
 */

#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsIWidget.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include <windows.h>
#include "nsWidgetsCID.h"
#include "nsITimer.h"
#include "nsITimerQueue.h"
#ifdef MOZ_AIMM
#include "aimm.h"
#endif
#include "nslog.h"

NS_IMPL_LOG(nsAppShellLog, 0)
#define PRINTF NS_LOG_PRINTF(nsAppShellLog)
#define FLUSH  NS_LOG_FLUSH(nsAppShellLog)

static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kTimerManagerCID, NS_TIMERMANAGER_CID);

NS_IMPL_ISUPPORTS(nsAppShell, NS_IAPPSHELL_IID) 

//-------------------------------------------------------------------------
//
// nsAppShell constructor
//
//-------------------------------------------------------------------------
nsAppShell::nsAppShell()  
{ 
  NS_INIT_REFCNT();
  mDispatchListener = 0;
}



//-------------------------------------------------------------------------
//
// Create the application shell
//
//-------------------------------------------------------------------------

NS_METHOD nsAppShell::Create(int* argc, char ** argv)
{
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_METHOD nsAppShell::SetDispatchListener(nsDispatchListener* aDispatchListener) 
{
  mDispatchListener = aDispatchListener;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Enter a message handler loop
//
//-------------------------------------------------------------------------

NS_METHOD nsAppShell::Run(void)
{
  NS_ADDREF_THIS();
  MSG  msg;
  int  keepGoing = 1;
  
  nsresult rv;
  NS_WITH_SERVICE(nsITimerQueue, queue, kTimerManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  // Process messages
  do {
    // Give priority to system messages (in particular keyboard, mouse,
    // timer, and paint messages).
    if (::PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) ||
        ::PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        ::PeekMessage(&msg, NULL, 0, WM_USER-1, PM_REMOVE) || 
        ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

      keepGoing = (msg.message != WM_QUIT);

      if (keepGoing != 0) {
//#ifdef MOZ_AIMM // not need?
//      if (!nsToolkit::gAIMMMsgPumpOwner || (nsToolkit::gAIMMMsgPumpOwner->OnTranslateMessage(&msg) != S_OK))
//#endif
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (mDispatchListener)
          mDispatchListener->AfterDispatch();
      }

    // process timer queue.
    } else if (queue->HasReadyTimers(NS_PRIORITY_LOWEST)) {

      do {
        queue->FireNextReadyTimer(NS_PRIORITY_LOWEST);
      } while (queue->HasReadyTimers(NS_PRIORITY_LOWEST) && 
                !::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
      
    } else {
       // Block and wait for any posted application message
      ::WaitMessage();
    }

  } while (keepGoing != 0);
  Release();
  return msg.wParam;
}

inline NS_METHOD nsAppShell::Spinup(void)
{ return NS_OK; }

inline NS_METHOD nsAppShell::Spindown(void)
{ return NS_OK; }

inline NS_METHOD nsAppShell::ListenToEventQueue(nsIEventQueue * aQueue, PRBool aListen)
{ return NS_OK; }

NS_METHOD
nsAppShell::GetNativeEvent(PRBool &aRealEvent, void *&aEvent)
{
  static MSG msg;

  BOOL gotMessage = false;

  nsresult rv;
  NS_WITH_SERVICE(nsITimerQueue, queue, kTimerManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  do {
    // Give priority to system messages (in particular keyboard, mouse,
    // timer, and paint messages).
    if (::PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) ||
        ::PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) || 
        ::PeekMessage(&msg, NULL, 0, WM_USER-1, PM_REMOVE) || 
        ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

      gotMessage = true;

    // process timer queue.
    } else if (queue->HasReadyTimers(NS_PRIORITY_LOWEST)) {

      do {
        queue->FireNextReadyTimer(NS_PRIORITY_LOWEST);
      } while (queue->HasReadyTimers(NS_PRIORITY_LOWEST) && 
                !::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));

    } else {
       // Block and wait for any posted application message
      ::WaitMessage();
    }

  } while (!gotMessage);

#ifdef DEBUG_danm
  if (msg.message != WM_TIMER)
      PRINTF("-> %d", msg.message);
#endif

//#ifdef MOZ_AIMM // not need?
//  if (!nsToolkit::gAIMMMsgPumpOwner || (nsToolkit::gAIMMMsgPumpOwner->OnTranslateMessage(&msg) != S_OK))
//#endif
  TranslateMessage(&msg);
  aEvent = &msg;
  aRealEvent = PR_TRUE;
  return NS_OK;
}

nsresult nsAppShell::DispatchNativeEvent(PRBool aRealEvent, void *aEvent)
{
  DispatchMessage((MSG *)aEvent);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Exit a message handler loop
//
//-------------------------------------------------------------------------

NS_METHOD nsAppShell::Exit(void)
{
  PostQuitMessage(0);
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// nsAppShell destructor
//
//-------------------------------------------------------------------------
nsAppShell::~nsAppShell()
{
}

