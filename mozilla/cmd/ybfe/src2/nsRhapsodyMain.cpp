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
 */
#include "nsViewerApp.h"
#include "nsBrowserWindow.h"
#include "nsIImageManager.h"
#include <stdlib.h>
#include "plevent.h"

extern "C" char *fe_GetConfigDir(void) {
  printf("XXX: return /tmp for fe_GetConfigDir\n");
  return strdup("/tmp");
}

static nsNativeViewerApp* gTheApp;
PLEventQueue*  gUnixMainEventQueue = nsnull;

extern void nsWebShell_SetUnixEventQueue(PLEventQueue* aEventQueue);

#if 0
static void nsUnixEventProcessorCallback(XtPointer aClosure, int* aFd, XtIntervalId *aId) 
{
  NS_ASSERTION(*aFd==PR_GetEventQueueSelectFD(gUnixMainEventQueue), "Error in nsUnixMain.cpp:nsUnixEventProcessCallback");
  PR_ProcessPendingEvents(gUnixMainEventQueue);
}
#endif

nsNativeViewerApp::nsNativeViewerApp()
{
}

nsNativeViewerApp::~nsNativeViewerApp()
{
}

int
nsNativeViewerApp::Run()
{
   // Setup event queue for dispatching 
   // asynchronous posts of form data + clicking on links.
   // Lifted from cmd/xfe/mozilla.c

  gUnixMainEventQueue = PR_CreateEventQueue("viewer-event-queue", PR_GetCurrentThread());
  if (nsnull == gUnixMainEventQueue) {
     // Force an assertion
    NS_ASSERTION("Can not create an event loop", PR_FALSE); 
  }

   // XXX Setup webshell's event queue. This should be changed
  nsWebShell_SetUnixEventQueue(gUnixMainEventQueue);

#if 0
  XtAppAddInput(gAppContext, PR_GetEventQueueSelectFD(gUnixMainEventQueue), 
   (XtPointer)(XtInputReadMask), nsUnixEventProcessorCallback, 0);
#endif

  OpenWindow();
  mAppShell->Run();
  return 0;
}

//----------------------------------------------------------------------

nsNativeBrowserWindow::nsNativeBrowserWindow()
{
}

nsNativeBrowserWindow::~nsNativeBrowserWindow()
{
}

static void MenuProc(PRUint32 aId) 
{
  // XXX our menus are horked: we can't support multiple windows!
  nsBrowserWindow* bw = (nsBrowserWindow*)
    nsBrowserWindow::gBrowsers.ElementAt(0);
  bw->DispatchMenuItem(aId);
}

nsresult
nsNativeBrowserWindow::CreateMenuBar(PRInt32 aWidth)
{
  ;

  return NS_OK;
}

nsEventStatus
nsNativeBrowserWindow::DispatchMenuItem(PRInt32 aID)
{
  // Dispatch motif-only menu code goes here

  // Dispatch xp menu items
  return nsBrowserWindow::DispatchMenuItem(aID);
}

//----------------------------------------------------------------------

int main(int argc, char **argv)
{
  // Hack to get il_ss set so it doesn't fail in xpcompat.c
  nsIImageManager *manager;
  NS_NewImageManager(&manager);

  gTheApp = new nsNativeViewerApp();
  gTheApp->Initialize(argc, argv);
  gTheApp->Run();

  return 0;
}


