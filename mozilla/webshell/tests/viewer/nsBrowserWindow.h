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
#ifndef nsBrowserWindow_h___
#define nsBrowserWindow_h___

#include "nsIBrowserWindow.h"
#include "nsIStreamListener.h"
#include "nsINetSupport.h"
#include "nsIWebShell.h"
#include "nsIScriptContextOwner.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsCRT.h"


class nsITextWidget;
class nsIButton;
class nsIThrobber;
class nsViewerApp;
class nsIPresShell;
class nsIPref;

#define SAMPLES_BASE_URL "resource:/res/samples"

/**
 * Abstract base class for our test app's browser windows
 */
class nsBrowserWindow : public nsIBrowserWindow,
                        public nsIStreamObserver,
                        public nsINetSupport,
                        public nsIWebShellContainer
{
public:
  void* operator new(size_t sz) {
    void* rv = new char[sz];
    nsCRT::zero(rv, sz);
    return rv;
  }

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIBrowserWindow
  NS_IMETHOD Init(nsIAppShell* aAppShell,
                  nsIPref* aPrefs,
                  const nsRect& aBounds,
                  PRUint32 aChromeMask);
  NS_IMETHOD MoveTo(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD SizeTo(PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD GetBounds(nsRect& aBounds);
  NS_IMETHOD Show();
  NS_IMETHOD Hide();
  NS_IMETHOD ChangeChrome(PRUint32 aNewChromeMask);
  NS_IMETHOD GetChrome(PRUint32& aChromeMaskResult);
  NS_IMETHOD LoadURL(const nsString& aURL);
  NS_IMETHOD SetTitle(const nsString& aTitle);
  NS_IMETHOD GetTitle(nsString& aResult);
  NS_IMETHOD SetStatus(const nsString& aStatus);
  NS_IMETHOD GetStatus(nsString& aResult);
  NS_IMETHOD GetWebShell(nsIWebShell*& aResult);

  // nsIStreamObserver
  NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURL* aURL, const nsString& aMsg);
  NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg);

  // nsIWebShellContainer
  NS_IMETHOD WillLoadURL(nsIWebShell* aShell, const nsString& aURL);
  NS_IMETHOD BeginLoadURL(nsIWebShell* aShell, const nsString& aURL);
  NS_IMETHOD EndLoadURL(nsIWebShell* aShell, const nsString& aURL);

  // nsINetSupport
  NS_IMETHOD_(void) Alert(const nsString &aText);
  NS_IMETHOD_(PRBool) Confirm(const nsString &aText);
  NS_IMETHOD_(PRBool) Prompt(const nsString &aText,
                             const nsString &aDefault,
                             nsString &aResult);
  NS_IMETHOD_(PRBool) PromptUserAndPassword(const nsString &aText,
                                            nsString &aUser,
                                            nsString &aPassword);
  NS_IMETHOD_(PRBool) PromptPassword(const nsString &aText,
                                     nsString &aPassword);

  // nsBrowserWindow
  virtual nsresult CreateMenuBar(PRInt32 aWidth) = 0;
  virtual nsresult CreateToolBar(PRInt32 aWidth);
  virtual nsresult CreateStatusBar(PRInt32 aWidth);
  void Layout(PRInt32 aWidth, PRInt32 aHeight);
  void Destroy();
  void Back();
  void Forward();
  void GoTo(const nsString& aURL);
  void StartThrobber();
  void StopThrobber();
  void LoadThrobberImages();
  void DestroyThrobberImages();
  virtual nsEventStatus DispatchMenuItem(PRInt32 aID) = 0;

  void DoFileOpen();
  void DoCopy();
  nsIPresShell* GetPresShell();

#ifdef NS_DEBUG
  void DumpContent(FILE *out = stdout);
  void DumpFrames(FILE *out = stdout);
  void DumpViews(FILE *out = stdout);
  void DumpWebShells(FILE *out = stdout);
  void DumpStyleSheets(FILE *out = stdout);
  void DumpStyleContexts(FILE *out = stdout);
  void ToggleFrameBorders();
  void ForceRefresh();
  void ShowContentSize();
  void ShowFrameSize();
  void ShowStyleSize();
  void DoDebugSave();
  void DoToggleSelection();
  void DoDebugRobot();
  void DoSiteWalker();
  void DoJSConsole();
  void DoEditorMode();
  void DoSelectAll();
  nsEventStatus DispatchDebugMenu(PRInt32 aID);
#endif

  void SetApp(nsViewerApp* aApp) {
    mApp = aApp;
  }

  nsViewerApp* mApp;

  PRUint32 mChromeMask;
  nsString mTitle;

  nsIWidget* mWindow;
  nsIWebShell* mWebShell;

  // "Toolbar"
  nsITextWidget* mLocation;
  nsIButton* mBack;
  nsIButton* mForward;
  nsIThrobber* mThrobber;

  // "Status bar"
  nsITextWidget* mStatus;

  // Global window collection
  static nsVoidArray gBrowsers;
  static void AddBrowser(nsBrowserWindow* aBrowser);
  static void RemoveBrowser(nsBrowserWindow* aBrowser);
  static nsBrowserWindow* FindBrowserFor(nsIWidget* aWidget, PRIntn aWhich);

protected:
  nsBrowserWindow();
  virtual ~nsBrowserWindow();
};

// XXX This is bad; because we can't hang a closure off of the event
// callback we have no way to store our This pointer; therefore we
// have to hunt to find the browswer that events belong too!!!

// aWhich for FindBrowserFor
#define FIND_WINDOW   0
#define FIND_BACK     1
#define FIND_FORWARD  2
#define FIND_LOCATION 3

//----------------------------------------------------------------------

class nsNativeBrowserWindow : public nsBrowserWindow {
public:
  nsNativeBrowserWindow();
  ~nsNativeBrowserWindow();

  virtual nsresult CreateMenuBar(PRInt32 aWidth);
  virtual nsEventStatus DispatchMenuItem(PRInt32 aID);
};

#endif /* nsBrowserWindow_h___ */
