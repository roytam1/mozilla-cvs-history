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
#include <windows.h>
#include "nsViewerApp.h"
#include "nsBrowserWindow.h"
#include "nsITimer.h"
#include "JSConsole.h"
#include "plevent.h"
#include "nsIServiceManager.h"

JSConsole *gConsole;
HINSTANCE gInstance, gPrevInstance;
static nsITimer* gNetTimer;

nsNativeViewerApp::nsNativeViewerApp()
{
}

nsNativeViewerApp::~nsNativeViewerApp()
{
}

int
nsNativeViewerApp::Run() 
{
  if (mJustShutdown)
    return 0;

  OpenWindow();
 
  // Process messages
  MSG  msg;
  int  keepGoing = 1;

  // Pump all messages
  do {
//    BOOL  havePriorityMessage;

    // Give priority to system messages (in particular keyboard, mouse,
    // timer, and paint messages).
    // Note: on Win98 and NT 5.0 we can also use PM_QS_INPUT and PM_QS_PAINT flags.
    if (::PeekMessage(&msg, NULL, 0, WM_USER-1, PM_REMOVE)) {
      keepGoing = (msg.message != WM_QUIT);
    
    } else {
      // Block and wait for any posted application message
      keepGoing = ::GetMessage(&msg, NULL, 0, 0);
    }

    // If we successfully retrieved a message then dispatch it
    if (keepGoing >= 0) {
      if (!JSConsole::sAccelTable || !gConsole || !gConsole->GetMainWindow() ||
          !TranslateAccelerator(gConsole->GetMainWindow(), JSConsole::sAccelTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  } while (keepGoing != 0);

  return msg.wParam;
}

//----------------------------------------------------------------------

nsNativeBrowserWindow::nsNativeBrowserWindow()
{
}

nsNativeBrowserWindow::~nsNativeBrowserWindow()
{
}

nsresult
nsNativeBrowserWindow::InitNativeWindow()
{
	// override to do something special with platform native windows
  return NS_OK;
}

nsresult
nsNativeBrowserWindow::CreateMenuBar(PRInt32 aWidth)
{
  HMENU menu = ::LoadMenu(gInstance, "Viewer");
  HWND hwnd = (HWND)mWindow->GetNativeData(NS_NATIVE_WIDGET);
  ::SetMenu(hwnd, menu);

  return NS_OK;
}

nsresult
nsNativeBrowserWindow::GetMenuBarHeight(PRInt32 * aHeightOut)
{
  NS_ASSERTION(nsnull != aHeightOut,"null out param.");

  *aHeightOut = 0;

  return NS_OK;
}

nsEventStatus
nsNativeBrowserWindow::DispatchMenuItem(PRInt32 aID)
{
  // Dispatch windows-only menu code goes here

  // Dispatch xp menu items
  return nsBrowserWindow::DispatchMenuItem(aID);
}

//----------------------------------------------------------------------

int main(int argc, char **argv)
{
  nsresult rv;
  nsIServiceManager* servMgr;
  rv = NS_InitXPCOM(&servMgr, nsnull, nsnull);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_InitXPCOM failed");
  nsViewerApp* app = new nsNativeViewerApp();
  NS_ADDREF(app);
  app->Initialize(argc, argv);
  int result = app->Run();
  app->Exit();  // this exit is needed for the -x case where the close box is never clicked
  NS_RELEASE(app);
  rv = NS_ShutdownXPCOM(servMgr);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
  NS_RELEASE(servMgr);
  return result;
}

int PASCAL
WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdParam, 
        int nCmdShow)
{
  gInstance = instance;
  gPrevInstance = prevInstance;
  return main(0, nsnull);
}
