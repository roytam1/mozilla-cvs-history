/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsBrowserAppCore_h___
#define nsBrowserAppCore_h___

//#include "nsAppCores.h"

#include "nscore.h"
#include "nsString.h"
#include "nsISupports.h"

#include "nsIDOMBrowserAppCore.h"
#include "nsBaseAppCore.h"

#include "nsIStreamObserver.h"
#include "nsIDocumentLoaderObserver.h"
#include "nsIObserver.h"
#include "nsISessionHistory.h"

class nsIBrowserWindow;
class nsIWebShell;
class nsIScriptContext;
class nsIDOMWindow;
class nsIURI;
class nsIWebShellWindow;
class nsIFindComponent;

#define SHISTORY_POPUP_LIST 10
////////////////////////////////////////////////////////////////////////////////
// nsBrowserAppCore:
////////////////////////////////////////////////////////////////////////////////

class nsBrowserAppCore : public nsBaseAppCore, 
                         public nsIDOMBrowserAppCore,
                         //     public nsIStreamObserver,
                         public nsIDocumentLoaderObserver,
                         public nsIObserver,
					            	 public nsISessionHistory
{
  public:

    nsBrowserAppCore();
    virtual ~nsBrowserAppCore();
                 
    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD    GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
    NS_IMETHOD    Init(const nsString& aId);
    NS_IMETHOD    GetId(nsString& aId) { return nsBaseAppCore::GetId(aId); } 

    NS_IMETHOD    Back();
    NS_IMETHOD    Forward();
#ifdef NECKO
    NS_IMETHOD    Reload(nsLoadFlags aType);
#else
    NS_IMETHOD    Reload(PRInt32 aType);
#endif
    NS_IMETHOD    Stop();
    NS_IMETHOD    BackButtonPopup();
    NS_IMETHOD    ForwardButtonPopup();
    NS_IMETHOD    GotoHistoryIndex(PRInt32 aIndex);

    NS_IMETHOD    WalletPreview(nsIDOMWindow* aWin, nsIDOMWindow* aForm);
    NS_IMETHOD    SignonViewer(nsIDOMWindow* aWin);
    NS_IMETHOD    CookieViewer(nsIDOMWindow* aWin);
    NS_IMETHOD    WalletEditor(nsIDOMWindow* aWin);
    NS_IMETHOD    WalletChangePassword();
    NS_IMETHOD    WalletQuickFillin(nsIDOMWindow* aWin);
    NS_IMETHOD    WalletRequestToCapture(nsIDOMWindow* aWin);
    NS_IMETHOD    WalletSamples();

    NS_IMETHOD    LoadUrl(const nsString& aUrl);
    NS_IMETHOD    SetToolbarWindow(nsIDOMWindow* aWin);
    NS_IMETHOD    SetContentWindow(nsIDOMWindow* aWin);
    NS_IMETHOD    SetWebShellWindow(nsIDOMWindow* aWin);
    NS_IMETHOD    NewWindow();
    NS_IMETHOD    OpenWindow();
    NS_IMETHOD    PrintPreview();
    NS_IMETHOD    Print();
    NS_IMETHOD    Copy();
    NS_IMETHOD    Close();
    NS_IMETHOD    Exit();
    NS_IMETHOD    SelectAll();
    NS_IMETHOD    Find();
    NS_IMETHOD    FindNext();
    NS_IMETHOD    SetDocumentCharset(const nsString& aCharset); 
    NS_IMETHOD    LoadInitialPage();

    // nsIDocumentLoaderObserver
#ifdef NECKO
    NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aCommand);
    NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsresult aStatus, nsIDocumentLoaderObserver* aObserver);
    NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsIContentViewer* aViewer);
    NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, PRUint32 aProgress, PRUint32 aProgressMax);
    NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsString& aMsg);
    NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, nsIChannel* channel, nsresult aStatus);
    NS_IMETHOD HandleUnknownContentType(nsIDocumentLoader* loader, nsIChannel* channel, const char *aContentType,const char *aCommand );		
#else
    NS_IMETHOD OnStartDocumentLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aCommand);
    NS_IMETHOD OnEndDocumentLoad(nsIDocumentLoader* loader, nsIURI *aUrl, PRInt32 aStatus,
								 nsIDocumentLoaderObserver * aObserver);

    NS_IMETHOD OnStartURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, const char* aContentType, 
                            nsIContentViewer* aViewer);
    NS_IMETHOD OnProgressURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, PRUint32 aProgress, 
                               PRUint32 aProgressMax);
    NS_IMETHOD OnStatusURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, nsString& aMsg);
    NS_IMETHOD OnEndURLLoad(nsIDocumentLoader* loader, nsIURI* aURL, PRInt32 aStatus);
    NS_IMETHOD HandleUnknownContentType(nsIDocumentLoader* loader,
                                        nsIURI *aURL,
                                        const char *aContentType,
                                        const char *aCommand );
#endif



    // nsIObserver
    NS_DECL_NSIOBSERVER


	// nsISessionHistory methods 
    NS_IMETHOD GoForward(nsIWebShell * aPrev);

    NS_IMETHOD GoBack(nsIWebShell * aPrev);
#ifdef NECKO
	NS_IMETHOD Reload(nsIWebShell * aPrev, nsLoadFlags aType);
#else
	NS_IMETHOD Reload(nsIWebShell * aPrev, nsURLReloadType aType);
#endif  /* NECKO */

    NS_IMETHOD canForward(PRBool &aResult);

    NS_IMETHOD canBack(PRBool &aResult);

    NS_IMETHOD add(nsIWebShell * aWebShell);

    NS_IMETHOD Goto(PRInt32 aHistoryIndex, nsIWebShell * aPrev, PRBool aIsReloading);

    NS_IMETHOD getHistoryLength(PRInt32 & aResult);

    NS_IMETHOD getCurrentIndex(PRInt32 & aResult);

  //  NS_IMETHOD cloneHistory(nsISessionHistory * aSessionHistory);

    NS_IMETHOD SetLoadingFlag(PRBool aFlag);

    NS_IMETHOD GetLoadingFlag(PRBool &aFlag);

    NS_IMETHOD SetLoadingHistoryEntry(nsHistoryEntry * aHistoryEntry);

    NS_IMETHOD GetURLForIndex(PRInt32 aIndex, const PRUnichar ** aURL);

    NS_IMETHOD SetURLForIndex(PRInt32 aIndex, const PRUnichar * aURL);

	NS_IMETHOD GetTitleForIndex(PRInt32 aIndex, const PRUnichar ** aTitle);

    NS_IMETHOD SetTitleForIndex(PRInt32 aIndex, const PRUnichar * aTitle);
    
	NS_IMETHOD GetHistoryObjectForIndex(PRInt32 aIndex, nsISupports ** aState);

    NS_IMETHOD SetHistoryObjectForIndex(PRInt32 aIndex, nsISupports * aState);

  protected:
    NS_IMETHOD DoDialog();
    NS_IMETHOD ExecuteScript(nsIScriptContext * aContext, const nsString& aScript);
    void SetButtonImage(nsIDOMNode * aParentNode, PRInt32 aBtnNum, const nsString &aResName);
    void InitializeSearch(nsIFindComponent*);
    void BeginObserving();
    void EndObserving();
    NS_IMETHOD CreateMenuItem(nsIDOMNode * , PRInt32,const PRUnichar * );
	NS_IMETHOD UpdateGoMenu();
	NS_IMETHOD ClearHistoryPopup(nsIDOMNode * );

    nsIScriptContext   *mToolbarScriptContext;			// weak reference
    nsIScriptContext   *mContentScriptContext;			// weak reference

    nsIDOMWindow       *mToolbarWindow;							// weak reference
    nsIDOMWindow       *mContentWindow;							// weak reference

    nsIWebShellWindow  *mWebShellWin;								// weak reference
    nsIWebShell *       mWebShell;									// weak reference
    nsIWebShell *       mContentAreaWebShell;				// weak reference

    nsISessionHistory*  mSHistory;			           // this is a service

    nsCOMPtr<nsISupports>  mSearchContext;				// at last, something we really own
#ifdef DEBUG_warren
    PRIntervalTime      mLoadStartTime;
#endif
};

#endif // nsBrowserAppCore_h___
