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
#ifndef nsWebCrawler_h___
#define nsWebCrawler_h___

#include "nsIBrowserWindow.h"
#include "nsVoidArray.h"
#include "nsString.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsITimer;
class nsIURL;
class nsViewerApp;
class AtomHashTable;

class nsWebCrawler : public nsISupports {
public:
  nsWebCrawler(nsViewerApp* aViewer);

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIStreamObserver
  NS_IMETHOD OnStartBinding(nsIURL* aURL, const char *aContentType);
  NS_IMETHOD OnProgress(nsIURL* aURL, PRInt32 aProgress, PRInt32 aProgressMax);
  NS_IMETHOD OnStatus(nsIURL* aURL, const nsString& aMsg);
  NS_IMETHOD OnStopBinding(nsIURL* aURL, PRInt32 status, const nsString& aMsg);

  // Add a url to load
  void AddURL(const nsString& aURL);

  // Add a domain that is safe to load url's from
  void AddSafeDomain(const nsString& aDomain);

  // Add a domain that must be avoided
  void AddAvoidDomain(const nsString& aDomain);

  void SetBrowserWindow(nsIBrowserWindow* aWindow) {
    mBrowser = aWindow;
    NS_ADDREF(mBrowser);
  }

  // Set the delay (by default, the timer is set to one second)
  void SetDelay(PRInt32 aSeconds) {
    mDelay = aSeconds;
  }

  void EnableJiggleLayout() {
    mJiggleLayout = PR_TRUE;
  }

  // If set to TRUE the loader will post an exit message on exit
  void SetExitOnDone(PRBool aPostExit) {
    mPostExit = aPostExit;
  }

  // Start loading documents
  void Start();

  // Enable the crawler; when a document contains links to other
  // documents the crawler will go to them subject to the limitations
  // on the total crawl count and the domain name checks.
  void EnableCrawler();

  void SetRecordFile(FILE* aFile) {
    mRecord = aFile;
  }

  void SetMaxPages(PRInt32 aMax) {
    mMaxPages = aMax;
  }

  void LoadNextURL();

protected:
  virtual ~nsWebCrawler();

  void FindURLsIn(nsIDocument* aDocument, nsIContent* aNode);

  void FindMoreURLs();

  PRBool OkToLoad(const nsString& aURLSpec);

  void RecordLoadedURL(const nsString& aURLSpec);

  nsIBrowserWindow* mBrowser;
  nsViewerApp* mViewer;
  nsITimer* mTimer;
  FILE* mRecord;
  nsIAtom* mLinkTag;
  nsIAtom* mFrameTag;
  nsIAtom* mIFrameTag;
  AtomHashTable* mVisited;

  PRBool mCrawl;
  PRBool mJiggleLayout;
  PRBool mPostExit;
  PRInt32 mDelay;
  PRInt32 mMaxPages;

  nsVoidArray mPendingURLs;
  nsVoidArray mSafeDomains;
  nsVoidArray mAvoidDomains;
};

#endif /* nsWebCrawler_h___ */
