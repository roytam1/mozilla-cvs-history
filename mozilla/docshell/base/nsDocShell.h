/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Travis Bogard <travis@netscape.com>
 */

#ifndef nsDocShell_h__
#define nsDocShell_h__

#include "nsIParser.h"  // for nsCharSetSource
#include "nsIPresShell.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIContentViewer.h"
#include "nsIPref.h"
#include "nsVoidArray.h"

#include "nsCDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIContentViewerContainer.h"
#include "nsIDeviceContext.h"

#include "nsIDocumentLoader.h"
#include "nsIDocumentLoaderObserver.h"

#include "nsWeakReference.h"

// Local Includes
#include "nsDSURIContentListener.h"

// Helper Classes
#include "nsCOMPtr.h"
#include "nsPoint.h" // mCurrent/mDefaultScrollbarPreferences
#include "nsString.h"

//#define SH_IN_FRAMES 1

// Interfaces Needed
#include "nsIDocumentCharsetInfo.h"
#include "nsIGlobalHistory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIPrompt.h"
#include "nsIRefreshURI.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsISHistory.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsITimerCallback.h"
#include "nsIWebNavigation.h"
#include "nsIWebProgressListener.h"
#include "nsISHContainer.h"

//*****************************************************************************
//***    nsRefreshTimer
//*****************************************************************************

class nsRefreshTimer : public nsITimerCallback
{
public:
   nsRefreshTimer();

   NS_DECL_ISUPPORTS
     
   // nsITimerCallback interface
   NS_IMETHOD_(void) Notify(nsITimer *timer);

   nsCOMPtr<nsIDocShell>   mDocShell;
   nsCOMPtr<nsIURI>        mURI;
   PRBool                  mRepeat;
   PRInt32                 mDelay;

protected:
   virtual ~nsRefreshTimer();
};

//*****************************************************************************
//***    nsDocShellInitInfo
//*****************************************************************************

class nsDocShellInitInfo
{
public:
   //nsIGenericWindow Stuff
   PRInt32        x;
   PRInt32        y;
   PRInt32        cx;
   PRInt32        cy;
};

//*****************************************************************************
//***    nsDocShell
//*****************************************************************************

class nsDocShell : public nsIDocShell,
                   public nsIDocShellTreeItem, 
                   public nsIDocShellTreeNode,
                   public nsIWebNavigation,
                   public nsIBaseWindow, 
                   public nsIScrollable, 
                   public nsITextScroll, 
                   public nsIContentViewerContainer,
                   public nsIInterfaceRequestor,
                   public nsIScriptGlobalObjectOwner,
                   public nsIRefreshURI,
                   public nsIWebProgressListener,
                   public nsSupportsWeakReference
{
friend class nsDSURIContentListener;

public:
   // Object Management
   nsDocShell();

   NS_DECL_ISUPPORTS

   NS_DECL_NSIDOCSHELL
   NS_DECL_NSIDOCSHELLTREEITEM
   NS_DECL_NSIDOCSHELLTREENODE
   NS_DECL_NSIWEBNAVIGATION
   NS_DECL_NSIBASEWINDOW
   NS_DECL_NSISCROLLABLE
   NS_DECL_NSITEXTSCROLL
   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSISCRIPTGLOBALOBJECTOWNER
   NS_DECL_NSIWEBPROGRESSLISTENER

   // nsIRefreshURI
   NS_IMETHOD RefreshURI(nsIURI *aURI, PRInt32 aDelay, PRBool aRepeat);
   NS_IMETHOD CancelRefreshURITimers();

   // XXX: move to a macro
   // nsIContentViewerContainer
   NS_IMETHOD Embed(nsIContentViewer* aDocViewer, 
                   const char* aCommand,
                   nsISupports* aExtraInfo);


  nsresult SetLoadCookie(nsISupports *aCookie);
  nsresult GetLoadCookie(nsISupports **aResult);

protected:
   // Object Management
   virtual ~nsDocShell();
   NS_IMETHOD DestroyChildren();

   // Content Viewer Management
   NS_IMETHOD EnsureContentViewer();
   NS_IMETHOD EnsureDeviceContext();
   NS_IMETHOD CreateAboutBlankContentViewer();
   NS_IMETHOD CreateContentViewer(const char* aContentType, 
      nsIChannel* aOpenedChannel, nsIStreamListener** aContentHandler);
   NS_IMETHOD NewContentViewerObj(const char* aContentType, 
      nsIChannel* aOpenedChannel, nsILoadGroup* aLoadGroup, 
      nsIStreamListener** aContentHandler, nsIContentViewer** aViewer);
   NS_IMETHOD SetupNewViewer(nsIContentViewer* aNewViewer);

   // Site Loading
   typedef enum
   {
      loadNormal,          // Normal Load
      loadNormalReplace,   // Normal Load but replaces current history slot
      loadHistory,         // Load from history
      loadReloadNormal,    // Reload
      loadReloadBypassCache,
      loadReloadBypassProxy,
      loadReloadBypassProxyAndCache,
      loadLink,
      loadRefresh
   } loadType;
#ifdef SH_IN_FRAMES
   NS_IMETHOD InternalLoad(nsIURI* aURI, nsIURI* aReferrerURI, 
      nsISupports* owner, const char* aWindowTarget=nsnull, 
      nsIInputStream* aPostData=nsnull, loadType aLoadType=loadNormal, nsISHEntry * aSHEntry = nsnull);
#else
   NS_IMETHOD InternalLoad(nsIURI* aURI, nsIURI* aReferrerURI, 
      nsISupports* owner, const char* aWindowTarget=nsnull, 
      nsIInputStream* aPostData=nsnull, loadType aLoadType=loadNormal);
#endif
   NS_IMETHOD CreateFixupURI(const PRUnichar* aStringURI, nsIURI** aURI);
   NS_IMETHOD FileURIFixup(const PRUnichar* aStringURI, nsIURI** aURI);
   NS_IMETHOD ConvertFileToStringURI(nsString& aIn, nsString& aOut);
   NS_IMETHOD ConvertStringURIToFileCharset(nsString& aIn, nsCString& aOut);
   NS_IMETHOD KeywordURIFixup(const PRUnichar* aStringURI, nsIURI** aURI);
   NS_IMETHOD GetCurrentDocumentOwner(nsISupports** aOwner);
   NS_IMETHOD DoURILoad(nsIURI* aURI, nsIURI* aReferrer, nsISupports *aOwner,
      nsURILoadCommand aLoadCmd, const char* aWindowTarget, 
      nsIInputStream* aPostData);
   NS_IMETHOD ScrollIfAnchor(nsIURI* aURI, PRBool* aWasAnchor);
   NS_IMETHOD OnLoadingSite(nsIChannel* aChannel);
   NS_IMETHOD OnNewURI(nsIURI *aURI, nsIChannel* aChannel, loadType aLoadType);
   virtual void SetCurrentURI(nsIURI* aURI);
   virtual void SetReferrerURI(nsIURI* aURI);

   // Session History
   NS_IMETHOD ShouldAddToSessionHistory(nsIURI* aURI, PRBool* aShouldAdd);
   NS_IMETHOD ShouldPersistInSessionHistory(nsIURI* aURI, PRBool* aShouldPersist);
   NS_IMETHOD AddToSessionHistory(nsIURI* aURI, nsIChannel *aChannel);
   NS_IMETHOD UpdateCurrentSessionHistory();
   NS_IMETHOD LoadHistoryEntry(nsISHEntry* aEntry);
//   NS_IMETHOD GetCurrentSHE(PRInt32 aChildOffset, nsISHEntry ** aResult);
   NS_IMETHOD PersistLayoutHistoryState();
   NS_IMETHOD CloneAndReplace(nsISHEntry * srcEntry, nsISHEntry * aCloneRef,
							nsISHEntry * areplaceEntry, nsISHEntry * destEntry);
   // Global History
   NS_IMETHOD ShouldAddToGlobalHistory(nsIURI* aURI, PRBool* aShouldAdd);
   NS_IMETHOD AddToGlobalHistory(nsIURI* aURI);
   NS_IMETHOD UpdateCurrentGlobalHistory();

   // Helper Routines
   nsDocShellInitInfo* InitInfo();
   NS_IMETHOD GetPromptAndStringBundle(nsIPrompt** aPrompt, nsIStringBundle**
      aStringBundle);
   NS_IMETHOD GetChildOffset(nsIDOMNode* aChild, nsIDOMNode* aParent, 
      PRInt32* aOffset);
   NS_IMETHOD GetRootScrollableView(nsIScrollableView** aOutScrollView);
   NS_IMETHOD EnsureContentListener();
   NS_IMETHOD EnsureScriptEnvironment();

   PRBool IsFrame();

protected:
   nsString                   mName;
   nsString                   mTitle;
   nsVoidArray                mChildren;
   nsCOMPtr<nsISupportsArray> mRefreshURIList;
   nsDSURIContentListener*    mContentListener;
   nsDocShellInitInfo*        mInitInfo;
   nsCOMPtr<nsIContentViewer> mContentViewer;
   nsCOMPtr<nsIDocumentCharsetInfo> mDocumentCharsetInfo;
   nsCOMPtr<nsIDeviceContext> mDeviceContext;
   nsCOMPtr<nsIDocumentLoader>mDocLoader;
   nsCOMPtr<nsIDocumentLoaderObserver> mDocLoaderObserver;
   nsCOMPtr<nsIWidget>        mParentWidget;
   nsCOMPtr<nsIPref>          mPrefs;
   nsCOMPtr<nsIURI>           mCurrentURI;
   nsCOMPtr<nsIURI>           mReferrerURI;
   nsCOMPtr<nsIScriptGlobalObject> mScriptGlobal;
   nsCOMPtr<nsIScriptContext> mScriptContext;
   nsCOMPtr<nsISHistory>      mSessionHistory;
   nsCOMPtr<nsIGlobalHistory> mGlobalHistory;
   nsCOMPtr<nsISupports>      mLoadCookie; // the load cookie associated with the window context.
   PRInt32                    mMarginWidth;
   PRInt32                    mMarginHeight;
   PRInt32                    mItemType;
   nsPoint                    mCurrentScrollbarPref; // this document only
   nsPoint                    mDefaultScrollbarPref; // persistent across doc loads
   loadType                   mLoadType;
   PRBool                     mInitialPageLoad;
   PRBool                     mAllowPlugins;
   PRInt32                    mViewMode;

   PRInt32                    mOffset;  // Offset in the parent's child list.
   // Reference to the SHEntry for this docshell until the page is destroyed.
   // Somebody give me better name
   nsCOMPtr<nsISHEntry>       OSHE; 
   // Reference to the SHEntry for this docshell until the page is loaded
   // Somebody give me better name
   nsCOMPtr<nsISHEntry>       LSHE;
   // this flag is for bug #21358. a docshell may load many urls
   // which don't result in new documents being created (i.e. a new content viewer)
   // we want to make sure we don't call a on load event more than once for a given
   // content viewer. 
   PRBool                     mEODForCurrentDocument; 

   /* WEAK REFERENCES BELOW HERE.
   Note these are intentionally not addrefd.  Doing so will create a cycle.
   For that reasons don't use nsCOMPtr.*/
   nsIDocShellTreeItem*       mParent;  // Weak Reference
   nsIDocShellTreeOwner*      mTreeOwner; // Weak Reference
   nsIChromeEventHandler*     mChromeEventHandler; //Weak Reference
};

#endif /* nsDocShell_h__ */
