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
#ifndef nsViewerApp_h___
#define nsViewerApp_h___

#include "nsIAppShell.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsVoidArray.h"

class nsIPref;
//class nsWebCrawler;
class nsBrowserWindow;
class nsIBrowserWindow;

class nsViewerApp : public nsDispatchListener
{
public:
  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  nsViewerApp();
  virtual ~nsViewerApp();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsDispatchListener
  virtual void AfterDispatch();

  // nsViewerApp
  NS_IMETHOD SetupRegistry();
  NS_IMETHOD Initialize(int argc, char** argv);
  NS_IMETHOD ProcessArguments(int argc, char** argv);
  NS_IMETHOD OpenWindow();
  NS_IMETHOD OpenWindow(PRUint32 aNewChromeMask, nsIBrowserWindow*& aNewWindow);
  NS_IMETHOD CreateRobot(nsBrowserWindow* aWindow);
  NS_IMETHOD CreateSiteWalker(nsBrowserWindow* aWindow);
  NS_IMETHOD CreateJSConsole(nsBrowserWindow* aWindow);
  NS_IMETHOD Exit();

  NS_IMETHOD DoPrefs(nsBrowserWindow* aWindow);

  NS_IMETHOD Run();

protected:

  void Destroy();

  nsIAppShell* mAppShell;
  nsIPref* mPrefs;
  nsString mStartURL;
  PRBool mDoPurify;
  PRBool mLoadTestFromFile;
  //PRBool mCrawl;
  nsString mInputFileName;
  PRInt32 mNumSamples;
  PRInt32 mDelay;
  PRInt32 mRepeatCount;
  //nsWebCrawler* mCrawler;
  PRBool mAllowPlugins;
  PRBool mIsInitialized;
};


#endif /* nsViewerApp_h___ */

