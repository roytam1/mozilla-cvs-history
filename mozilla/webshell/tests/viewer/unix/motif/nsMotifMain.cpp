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
#include "nsMotifMenu.h"
#include "nsIImageManager.h"
#include "nsIServiceManager.h"
#include "nsGfxCIID.h"

static NS_DEFINE_IID(kImageManagerCID, NS_IMAGEMANAGER_CID);

static nsNativeViewerApp* gTheApp;

nsNativeViewerApp::nsNativeViewerApp()
{
}

nsNativeViewerApp::~nsNativeViewerApp()
{
}

int
nsNativeViewerApp::Run()
{
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

nsresult
nsNativeBrowserWindow::InitNativeWindow()
{
	// override to do something special with platform native windows
  return NS_OK;
}

static void MenuProc(PRUint32 aID) 
{
  //  return nsBrowserWindow::DispatchMenuItem(aID);
  return;
}

nsresult
nsNativeBrowserWindow::CreateMenuBar(PRInt32 aWidth)
{
  CreateViewerMenus(XtParent((Widget)mWindow->GetNativeData(NS_NATIVE_WIDGET)), MenuProc);

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
  // Dispatch motif-only menu code goes here

  // Dispatch xp menu items
  //  return nsBrowserWindow::DispatchMenuItem(aID);
  return nsEventStatus_eIgnore;
}


//----------------------------------------------------------------------

int main(int argc, char **argv)
{
  nsresult result;
  // Hack to get il_ss set so it doesn't fail in xpcompat.c
  nsCOMPtr<nsIImageManager> manager;
  
  manager = do_GetService(kImageManagerCID, &result);
  if (NS_FAILED(result)) {
  /* This is just to provide backwards compatibility, until the ImageManagerImpl
     can be converted to a service on all platforms. Once, we havedone the conversion
     on all platforms, we should be removing the call to NS_NewImageManager(...)
   */
      if ((result = NS_NewImageManager(getter_AddRefs(manager))) != NS_OK) {
          return result;
      }
		// WARNING Extra addref to simulate older be
		nsIImageManager *aManager = manager.get();
		NS_ADDREF(aManager);
      
  }

  gTheApp = new nsNativeViewerApp();

  putenv("MOZ_TOOLKIT=motif");

  gTheApp->Initialize(argc, argv);
  gTheApp->Run();

  return 0;
}
