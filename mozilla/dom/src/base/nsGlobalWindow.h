/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsGlobalWindow_h___
#define nsGlobalWindow_h___

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsIPrincipal.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNavigator.h"
#include "nsIDOMLocation.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMNSLocation.h"
#include "nsIDOMScreen.h"
#include "nsITimer.h"
#include "nsIJSScriptObject.h"
#include "nsGUIEvent.h"
#include "nsFrameList.h"
#include "nsIScriptGlobalObjectData.h"
#include "nsDOMWindowList.h"
#include "nsIDOMEventTarget.h"
#include "nsIControllers.h"
#include "nsPIDOMWindow.h"

#define DEFAULT_HOME_PAGE "www.mozilla.org"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"

class nsIEventListenerManager;
class nsIDOMBarProp;
class nsIDOMDocument;
class nsIDocument;
class nsIContent;
class nsIPresContext;
class nsIDOMEvent;
class nsIBrowserWindow;
class nsIModalWindowSupport;
class nsIScrollableView;

#include "jsapi.h"

typedef struct nsTimeoutImpl nsTimeoutImpl;

class BarPropImpl;
class LocationImpl;
class NavigatorImpl;
class ScreenImpl;
class HistoryImpl;

// Global object for scripting
class GlobalWindowImpl : public nsIScriptObjectOwner, public nsIScriptGlobalObject, public nsIDOMWindow, 
                         public nsIJSScriptObject, public nsIScriptGlobalObjectData, public nsIDOMEventReceiver,
                         public nsPIDOMWindow
{
public:
  GlobalWindowImpl();
  virtual ~GlobalWindowImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

  NS_IMETHOD_(void)       SetContext(nsIScriptContext *aContext);
  NS_IMETHOD_(void)       GetContext(nsIScriptContext **aContext);
  NS_IMETHOD_(void)       SetNewDocument(nsIDOMDocument *aDocument);
  NS_IMETHOD_(void)       SetWebShell(nsIWebShell *aWebShell);
  NS_IMETHOD_(void)       GetWebShell(nsIWebShell **aWebShell);// XXX This may be temporary - rods
  NS_IMETHOD_(void)       SetOpenerWindow(nsIDOMWindow *aOpener);

  NS_IMETHOD    GetWindow(nsIDOMWindow** aWindow);
  NS_IMETHOD    GetSelf(nsIDOMWindow** aSelf);
  NS_IMETHOD    GetDocument(nsIDOMDocument** aDocument);
  NS_IMETHOD    GetNavigator(nsIDOMNavigator** aNavigator);
  NS_IMETHOD    GetScreen(nsIDOMScreen** aScreen);
  NS_IMETHOD    GetHistory(nsIDOMHistory** aHistory);
  NS_IMETHOD    GetLocation(nsIDOMLocation** aLocation);
  NS_IMETHOD    GetParent(nsIDOMWindow** aOpener);
  NS_IMETHOD    GetTop(nsIDOMWindow** aTop);
  NS_IMETHOD    GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD    GetClosed(PRBool* aClosed);
  NS_IMETHOD    GetMenubar(nsIDOMBarProp** aMenubar);
  NS_IMETHOD    GetToolbar(nsIDOMBarProp** aToolbar);
  NS_IMETHOD    GetLocationbar(nsIDOMBarProp** aLocationbar);
  NS_IMETHOD    GetPersonalbar(nsIDOMBarProp** aPersonalbar);
  NS_IMETHOD    GetStatusbar(nsIDOMBarProp** aStatusbar);
  NS_IMETHOD    GetScrollbars(nsIDOMBarProp** aScrollbars);
  NS_IMETHOD    GetDirectories(nsIDOMBarProp** aDirectories);
  NS_IMETHOD    GetFrames(nsIDOMWindowCollection** aFrames);

  NS_IMETHOD    GetOpener(nsIDOMWindow** aOpener);
  NS_IMETHOD    SetOpener(nsIDOMWindow* aOpener);

  NS_IMETHOD    GetStatus(nsString& aStatus);
  NS_IMETHOD    SetStatus(const nsString& aStatus);

  NS_IMETHOD    GetDefaultStatus(nsString& aDefaultStatus);
  NS_IMETHOD    SetDefaultStatus(const nsString& aDefaultStatus);

  NS_IMETHOD    GetName(nsString& aName);
  NS_IMETHOD    SetName(const nsString& aName);

  NS_IMETHOD    GetInnerWidth(PRInt32* aInnerWidth);
  NS_IMETHOD    SetInnerWidth(PRInt32 aInnerWidth);

  NS_IMETHOD    GetInnerHeight(PRInt32* aInnerHeight);
  NS_IMETHOD    SetInnerHeight(PRInt32 aInnerHeight);

  NS_IMETHOD    GetOuterWidth(PRInt32* aOuterWidth);
  NS_IMETHOD    SetOuterWidth(PRInt32 aOuterWidth);

  NS_IMETHOD    GetOuterHeight(PRInt32* aOuterHeight);
  NS_IMETHOD    SetOuterHeight(PRInt32 aOuterHeight);

  NS_IMETHOD    GetScreenX(PRInt32* aScreenX);
  NS_IMETHOD    SetScreenX(PRInt32 aScreenX);

  NS_IMETHOD    GetScreenY(PRInt32* aScreenY);
  NS_IMETHOD    SetScreenY(PRInt32 aScreenY);

  NS_IMETHOD    GetPageXOffset(PRInt32* aPageXOffset);
  NS_IMETHOD    SetPageXOffset(PRInt32 aPageXOffset);

  NS_IMETHOD    GetPageYOffset(PRInt32* aPageYOffset);
  NS_IMETHOD    SetPageYOffset(PRInt32 aPageYOffset);

  NS_IMETHOD    Dump(const nsString& aStr);
  NS_IMETHOD    Alert(JSContext *cx, jsval *argv, PRUint32 argc);
  NS_IMETHOD    Confirm(JSContext *cx, jsval *argv, PRUint32 argc, PRBool* aReturn);
  NS_IMETHOD    Prompt(JSContext *cx, jsval *argv, PRUint32 argc, nsString& aReturn);
  NS_IMETHOD    Focus();
  NS_IMETHOD    Blur();
  NS_IMETHOD    Close();
  NS_IMETHOD    Forward();
  NS_IMETHOD    Back();
  NS_IMETHOD    Home();
  NS_IMETHOD    Stop();
  NS_IMETHOD    Print();
  NS_IMETHOD    MoveTo(PRInt32 aXPos, PRInt32 aYPos);
  NS_IMETHOD    MoveBy(PRInt32 aXDif, PRInt32 aYDif);
  NS_IMETHOD    ResizeTo(PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD    ResizeBy(PRInt32 aWidthDif, PRInt32 aHeightDif);
  NS_IMETHOD    SizeToContent();
  NS_IMETHOD    ScrollTo(PRInt32 aXScroll, PRInt32 aYScroll);
  NS_IMETHOD    ScrollBy(PRInt32 aXScrollDif, PRInt32 aYScrollDif);
  NS_IMETHOD    GetScrollX(PRInt32* aScrollX);
  NS_IMETHOD    GetScrollY(PRInt32* aScrollY);

  NS_IMETHOD    ClearTimeout(PRInt32 aTimerID);
  NS_IMETHOD    ClearInterval(PRInt32 aTimerID);
  NS_IMETHOD    SetTimeout(JSContext *cx, jsval *argv, PRUint32 argc, 
                           PRInt32* aReturn);
  NS_IMETHOD    SetInterval(JSContext *cx, jsval *argv, PRUint32 argc, 
                            PRInt32* aReturn);
  NS_IMETHOD    Open(JSContext *cx, jsval *argv, PRUint32 argc, 
                            nsIDOMWindow** aReturn);
  NS_IMETHOD    OpenDialog(JSContext *cx, jsval *argv, PRUint32 argc, 
                            nsIDOMWindow** aReturn);

  NS_IMETHOD    CaptureEvents(PRInt32 aEventFlags);
  NS_IMETHOD    ReleaseEvents(PRInt32 aEventFlags);
  NS_IMETHOD    RouteEvent(nsIDOMEvent* aEvt);
  NS_IMETHOD    EnableExternalCapture();
  NS_IMETHOD    DisableExternalCapture();

  NS_IMETHOD    CreatePopup(nsIDOMElement* aElement, nsIDOMElement* aPopupContent, 
                            PRInt32 aXPos, PRInt32 aYPos, 
                            const nsString& aPopupType, const nsString& anAnchorAlignment,
                            const nsString& aPopupAlignment, nsIDOMWindow** outPopup);
  NS_IMETHOD    GetControllers(nsIControllers** aResult);

  // nsIDOMEventReceiver interface
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);

  // nsIDOMEventTarget interface
  NS_IMETHOD AddEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                            PRBool aUseCapture);
  NS_IMETHOD RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                               PRBool aUseCapture);



  NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext, 
                            nsEvent* aEvent, 
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus& aEventStatus);

  // nsIJSScriptObject interface
  virtual PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    EnumerateProperty(JSContext *aContext);
  virtual PRBool    Resolve(JSContext *aContext, jsval aID);
  virtual PRBool    Convert(JSContext *aContext, jsval aID);
  virtual void      Finalize(JSContext *aContext);
  
  // nsIScriptGlobalObjectData interface
  NS_IMETHOD        GetPrincipal(nsIPrincipal **prin);

  // nsPIDOMWindowInterface
  NS_IMETHOD        GetPrivateParent(nsPIDOMWindow** aResult);

  friend void nsGlobalWindow_RunTimeout(nsITimer *aTimer, void *aClosure);

protected:
  PRBool        RunTimeout(nsTimeoutImpl *aTimeout);
  nsresult      ClearTimeoutOrInterval(PRInt32 aTimerID);
  nsresult      SetTimeoutOrInterval(JSContext *cx, jsval *argv, 
                                     PRUint32 argc, PRInt32* aReturn, 
                                     PRBool aIsInterval);
  void          InsertTimeoutIntoList(nsTimeoutImpl **aInsertionPoint,
                                      nsTimeoutImpl *aTimeout);
  void          ClearAllTimeouts();
  void          DropTimeout(nsTimeoutImpl *aTimeout,
                            nsIScriptContext* aContext=nsnull);
  void          HoldTimeout(nsTimeoutImpl *aTimeout);
  nsresult      GetBrowserWindowInterface(nsIBrowserWindow*& aBrowser,
                            nsIWebShell *aWebShell=nsnull);
  nsresult      CheckWindowName(JSContext *cx, nsString& aName);
  PRInt32       WinHasOption(char *options, char *name, PRBool& aPresenceFlag);
  PRBool        CheckForEventListener(JSContext *aContext, nsString& aPropName);

  nsresult      OpenInternal(JSContext *cx, jsval *argv, PRUint32 argc, 
                             PRBool aDialog, nsIDOMWindow** aReturn);
  nsresult      AttachArguments(nsIDOMWindow *aWindow, jsval *argv, PRUint32 argc);
  PRUint32      CalculateChromeFlags(char *aFeatures, PRBool aDialog);
  nsresult      SizeAndShowOpenedWebShell(nsIWebShell *aOuterShell,
                  char *aFeatures, PRBool aNewWindow, PRBool aDialog);
  nsresult      ReadyOpenedWebShell(nsIWebShell *aWebShell, nsIDOMWindow **aDOMWindow);
  nsresult      GetModalWindowSupport(nsIModalWindowSupport **msw);
  nsresult      GetScrollInfo(nsIScrollableView** aScrollableView,
                              float* aP2T, float* aT2P);

  static nsresult WebShellToDOMWindow(nsIWebShell *aWebShell, nsIDOMWindow **aDOMWindow);

  nsIScriptContext *mContext;
  void *mScriptObject;
  nsIDOMDocument *mDocument;
  NavigatorImpl *mNavigator;
  LocationImpl *mLocation;
  ScreenImpl *mScreen;
  HistoryImpl *mHistory;
  nsIWebShell *mWebShell;
  nsIDOMWindow *mOpener;

  BarPropImpl *mMenubar;
  BarPropImpl *mToolbar;
  BarPropImpl *mLocationbar;
  BarPropImpl *mPersonalbar;
  BarPropImpl *mStatusbar;
  BarPropImpl *mScrollbars;
  
  nsIContent* mChromeElement;
  nsCOMPtr<nsIControllers> mControllers;

  nsTimeoutImpl *mTimeouts;
  nsTimeoutImpl **mTimeoutInsertionPoint;
  nsTimeoutImpl *mRunningTimeout;
  PRUint32 mTimeoutPublicIdCounter;
  nsIEventListenerManager* mListenerManager;
  nsDOMWindowList *mFrames;

  PRBool mFirstDocumentLoad;
};

/* 
 * Timeout struct that holds information about each JavaScript
 * timeout.
 */
struct nsTimeoutImpl {
  PRInt32             ref_count;      /* reference count to shared usage */
  GlobalWindowImpl    *window;        /* window for which this timeout fires */
  JSString            *expr;          /* the JS expression to evaluate */
  JSObject            *funobj;        /* or function to call, if !expr */
  nsITimer            *timer;         /* The actual timer object */
  jsval               *argv;          /* function actual arguments */
  PRUint16            argc;           /* and argument count */
  PRUint16            spare;          /* alignment padding */
  PRUint32            public_id;      /* Returned as value of setTimeout() */
  PRInt32             interval;       /* Non-zero if repetitive timeout */
  PRInt64             when;           /* nominal time to run this timeout */
  JSVersion           version;        /* Version of JavaScript to execute */
  nsIPrincipal       *principal;    /* principals with which to execute */
  char                *filename;      /* filename of setTimeout call */
  PRUint32            lineno;         /* line number of setTimeout call */
  nsTimeoutImpl       *next;
};


// Script "navigator" object
class NavigatorImpl : public nsIScriptObjectOwner, public nsIDOMNavigator {
public:
  NavigatorImpl();
  virtual ~NavigatorImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

  NS_IMETHOD    GetAppCodeName(nsString& aAppCodeName);

  NS_IMETHOD    GetAppName(nsString& aAppName);

  NS_IMETHOD    GetAppVersion(nsString& aAppVersion);

  NS_IMETHOD    GetLanguage(nsString& aLanguage);

  NS_IMETHOD    GetMimeTypes(nsIDOMMimeTypeArray** aMimeTypes);

  NS_IMETHOD    GetPlatform(nsString& aPlatform);

  NS_IMETHOD    GetPlugins(nsIDOMPluginArray** aPlugins);

  NS_IMETHOD    GetSecurityPolicy(nsString& aSecurityPolicy);

  NS_IMETHOD    GetUserAgent(nsString& aUserAgent);

  NS_IMETHOD    JavaEnabled(PRBool* aReturn);

  NS_IMETHOD    TaintEnabled(PRBool* aReturn);

  NS_IMETHOD    Preference(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn);

protected:
  void *mScriptObject;
  nsIDOMMimeTypeArray* mMimeTypes;
  nsIDOMPluginArray* mPlugins;
};

class nsIURI;

class LocationImpl : public nsIScriptObjectOwner, 
                     public nsIDOMLocation, 
                     public nsIDOMNSLocation,
                     public nsIJSScriptObject
{

protected:
public:
  LocationImpl(nsIWebShell *aWebShell);
  virtual ~LocationImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD SetScriptObject(void *aScriptObject);

  NS_IMETHOD_(void)       SetWebShell(nsIWebShell *aWebShell);

  // nsIDOMLocation
  NS_IMETHOD    GetHash(nsString& aHash);
  NS_IMETHOD    SetHash(const nsString& aHash);
  NS_IMETHOD    GetHost(nsString& aHost);
  NS_IMETHOD    SetHost(const nsString& aHost);
  NS_IMETHOD    GetHostname(nsString& aHostname);
  NS_IMETHOD    SetHostname(const nsString& aHostname);
  NS_IMETHOD    GetHref(nsString& aHref);
  NS_IMETHOD    SetHref(const nsString& aHref);
  NS_IMETHOD    GetPathname(nsString& aPathname);
  NS_IMETHOD    SetPathname(const nsString& aPathname);
  NS_IMETHOD    GetPort(nsString& aPort);
  NS_IMETHOD    SetPort(const nsString& aPort);
  NS_IMETHOD    GetProtocol(nsString& aProtocol);
  NS_IMETHOD    SetProtocol(const nsString& aProtocol);
  NS_IMETHOD    GetSearch(nsString& aSearch);
  NS_IMETHOD    SetSearch(const nsString& aSearch);
  NS_IMETHOD    Reload(PRBool aForceget);
  NS_IMETHOD    Replace(const nsString& aUrl);
  NS_IMETHOD    ToString(nsString& aReturn);
  
  // nsIDOMNSLocation
  NS_IMETHOD    Reload(JSContext *cx, jsval *argv, PRUint32 argc);
  NS_IMETHOD    Replace(JSContext *cx, jsval *argv, PRUint32 argc);

  // nsIJSScriptObject
  virtual PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    EnumerateProperty(JSContext *aContext);
  virtual PRBool    Resolve(JSContext *aContext, jsval aID);
  virtual PRBool    Convert(JSContext *aContext, jsval aID);
  virtual void      Finalize(JSContext *aContext);

protected:
  nsresult SetURL(nsIURI* aURL);
  nsresult SetHrefWithBase(const nsString& aHref, 
                           nsIURI* aBase, 
                           PRBool aReplace);
  nsresult GetSourceURL(JSContext* cx,
                        nsIURI** sourceURL);
  nsresult CheckURL(nsIURI *url);

  nsIWebShell *mWebShell;
  void *mScriptObject;
};

#endif /* nsGlobalWindow_h___ */
