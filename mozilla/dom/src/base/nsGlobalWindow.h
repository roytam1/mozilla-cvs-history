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
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNavigator.h"
#include "nsITimer.h"
#include "nsIJSScriptObject.h"
#include "nsIDOMEventCapturer.h"
#include "nsGUIEvent.h"

class nsIEventListenerManager;
class nsIDOMDocument;
class nsIPresContext;
class nsIDOMEvent;
class nsIBrowserWindow;

#include "jsapi.h"

typedef struct nsTimeoutImpl nsTimeoutImpl;

// Global object for scripting
class GlobalWindowImpl : public nsIScriptObjectOwner, public nsIScriptGlobalObject, public nsIDOMWindow, 
                         public nsIJSScriptObject, public nsIDOMEventCapturer
{
public:
  GlobalWindowImpl();
  ~GlobalWindowImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();

  NS_IMETHOD_(void)       SetContext(nsIScriptContext *aContext);
  NS_IMETHOD_(void)       SetNewDocument(nsIDOMDocument *aDocument);
  NS_IMETHOD_(void)       SetWebShell(nsIWebShell *aWebShell);

  NS_IMETHOD    GetWindow(nsIDOMWindow** aWindow);
  NS_IMETHOD    GetSelf(nsIDOMWindow** aSelf);
  NS_IMETHOD    GetDocument(nsIDOMDocument** aDocument);
  NS_IMETHOD    GetNavigator(nsIDOMNavigator** aNavigator);
  NS_IMETHOD    GetOpener(nsIDOMWindow** aOpener);
  NS_IMETHOD    Dump(const nsString& aStr);
  NS_IMETHOD    Alert(const nsString& aStr);
  NS_IMETHOD    ClearTimeout(PRInt32 aTimerID);
  NS_IMETHOD    ClearInterval(PRInt32 aTimerID);
  NS_IMETHOD    SetTimeout(JSContext *cx, jsval *argv, PRUint32 argc, 
                           PRInt32* aReturn);
  NS_IMETHOD    SetInterval(JSContext *cx, jsval *argv, PRUint32 argc, 
                            PRInt32* aReturn);
  NS_IMETHOD    Open(JSContext *cx, jsval *argv, PRUint32 argc, 
                            PRInt32* aReturn);

  // nsIDOMEventCapturer interface
  NS_IMETHOD CaptureEvent(nsIDOMEventListener *aListener);
  NS_IMETHOD ReleaseEvent(nsIDOMEventListener *aListener);
  NS_IMETHOD AddEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD RemoveEventListener(nsIDOMEventListener *aListener, const nsIID& aIID);
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aInstancePtrResult);
  NS_IMETHOD GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult);

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
  
  friend void nsGlobalWindow_RunTimeout(nsITimer *aTimer, void *aClosure);

protected:
  void          RunTimeout(nsTimeoutImpl *aTimeout);
  nsresult      ClearTimeoutOrInterval(PRInt32 aTimerID);
  nsresult      SetTimeoutOrInterval(JSContext *cx, jsval *argv, 
                                     PRUint32 argc, PRInt32* aReturn, 
                                     PRBool aIsInterval);
  void          InsertTimeoutIntoList(nsTimeoutImpl **aInsertionPoint,
                                      nsTimeoutImpl *aTimeout);
  void          ClearAllTimeouts();
  void          DropTimeout(nsTimeoutImpl *aTimeout);
  void          HoldTimeout(nsTimeoutImpl *aTimeout);
  nsresult      GetBrowserWindowInterface(nsIBrowserWindow*& aBrowser);
  nsresult      CheckWindowName(JSContext *cx, nsString& aName);
  int32         WinHasOption(char *options, char *name);

  nsIScriptContext *mContext;
  void *mScriptObject;
  nsIDOMDocument *mDocument;
  nsIDOMNavigator *mNavigator;
  nsIWebShell *mWebShell;
  
  nsTimeoutImpl *mTimeouts;
  nsTimeoutImpl **mTimeoutInsertionPoint;
  nsTimeoutImpl *mRunningTimeout;
  PRUint32 mTimeoutPublicIdCounter;
  nsIEventListenerManager* mListenerManager;
};

/* 
 * Timeout struct that holds information about each JavaScript
 * timeout.
 */
struct nsTimeoutImpl {
  PRInt32             ref_count;      /* reference count to shared usage */
  GlobalWindowImpl    *window;        /* window for which this timeout fires */
  char                *expr;          /* the JS expression to evaluate */
  JSObject            *funobj;        /* or function to call, if !expr */
  nsITimer            *timer;         /* The actual timer object */
  jsval               *argv;          /* function actual arguments */
  PRUint16            argc;           /* and argument count */
  PRUint16            spare;          /* alignment padding */
  PRUint32            public_id;      /* Returned as value of setTimeout() */
  PRInt32             interval;       /* Non-zero if repetitive timeout */
  PRInt64             when;           /* nominal time to run this timeout */
  JSVersion           version;        /* Version of JavaScript to execute */
  JSPrincipals        *principals;    /* principals with which to execute */
  char                *filename;      /* filename of setTimeout call */
  PRUint32            lineno;         /* line number of setTimeout call */
  nsTimeoutImpl       *next;
};


// Script "navigator" object
class NavigatorImpl : public nsIScriptObjectOwner, public nsIDOMNavigator {
public:
  NavigatorImpl();
  ~NavigatorImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);
  NS_IMETHOD ResetScriptObject();

  NS_IMETHOD    GetUserAgent(nsString& aUserAgent);

  NS_IMETHOD    GetAppCodeName(nsString& aAppCodeName);

  NS_IMETHOD    GetAppVersion(nsString& aAppVersion);

  NS_IMETHOD    GetAppName(nsString& aAppName);

  NS_IMETHOD    GetLanguage(nsString& aLanguage);

  NS_IMETHOD    GetPlatform(nsString& aPlatform);

  NS_IMETHOD    GetSecurityPolicy(nsString& aSecurityPolicy);

  NS_IMETHOD    JavaEnabled(PRBool* aReturn);

protected:
  void *mScriptObject;
};

#endif /* nsGlobalWindow_h___ */
