/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): 
 *
 */

#define INCL_PM
#define INCL_DOS
#include <os2.h>

#include "nsViewerApp.h"
#include "nsBrowserWindow.h"
#include "nsIAppShell.h"
#include "resources.h"

nsNativeBrowserWindow::nsNativeBrowserWindow() {}
nsNativeBrowserWindow::~nsNativeBrowserWindow() {}

nsresult
nsNativeBrowserWindow::InitNativeWindow()
{
	// override to do something special with platform native windows
  return NS_OK;
}

nsresult nsNativeBrowserWindow::CreateMenuBar(PRInt32 aWidth)
{
   HWND hwnd = (HWND)mWindow->GetNativeData(NS_NATIVE_WIDGET);
   HWND hwndFrame = WinQueryWindow( hwnd, QW_PARENT);
   HWND hwndMenu = WinLoadMenu( hwndFrame, 0, IDM_VIEWERBAR);
   WinSendMsg( hwndFrame, WM_UPDATEFRAME, MPFROMLONG(FCF_MENU), 0);
   return NS_OK; 
}

nsEventStatus nsNativeBrowserWindow::DispatchMenuItem(PRInt32 aID)
{
   return nsBrowserWindow::DispatchMenuItem(aID);
}

nsNativeViewerApp::nsNativeViewerApp()
{
}

nsNativeViewerApp::~nsNativeViewerApp()
{
}

int nsNativeViewerApp::Run()
{
  OpenWindow();

  mAppShell->Run();

  return 0;
}

int main(int argc, char **argv)
{
   nsViewerApp* app = new nsNativeViewerApp;
   NS_ADDREF(app);
   app->Initialize(argc, argv);
   app->Run();
   NS_RELEASE(app);

   return 0;
}
