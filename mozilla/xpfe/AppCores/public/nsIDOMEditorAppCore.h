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
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMEditorAppCore_h__
#define nsIDOMEditorAppCore_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMBaseAppCore.h"

class nsIDOMWindow;

#define NS_IDOMEDITORAPPCORE_IID \
 { 0x9afff72b, 0xca9a, 0x11d2, \
    {0x96, 0xc9, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56}} 

class nsIDOMEditorAppCore : public nsIDOMBaseAppCore {
public:
  static const nsIID& IID() { static nsIID iid = NS_IDOMEDITORAPPCORE_IID; return iid; }

  NS_IMETHOD    SetAttribute(const nsString& aAttr, const nsString& aValue)=0;

  NS_IMETHOD    Undo()=0;

  NS_IMETHOD    Redo()=0;

  NS_IMETHOD    Exit()=0;

  NS_IMETHOD    SetToolbarWindow(nsIDOMWindow* aWin)=0;

  NS_IMETHOD    SetContentWindow(nsIDOMWindow* aWin)=0;

  NS_IMETHOD    SetWebShellWindow(nsIDOMWindow* aWin)=0;
};


#define NS_DECL_IDOMEDITORAPPCORE   \
  NS_IMETHOD    SetAttribute(const nsString& aAttr, const nsString& aValue);  \
  NS_IMETHOD    Undo();  \
  NS_IMETHOD    Redo();  \
  NS_IMETHOD    Exit();  \
  NS_IMETHOD    SetToolbarWindow(nsIDOMWindow* aWin);  \
  NS_IMETHOD    SetContentWindow(nsIDOMWindow* aWin);  \
  NS_IMETHOD    SetWebShellWindow(nsIDOMWindow* aWin);  \



#define NS_FORWARD_IDOMEDITORAPPCORE(_to)  \
  NS_IMETHOD    SetAttribute(const nsString& aAttr, const nsString& aValue) { return _to##SetAttribute(aAttr, aValue); }  \
  NS_IMETHOD    Undo() { return _to##Undo(); }  \
  NS_IMETHOD    Redo() { return _to##Redo(); }  \
  NS_IMETHOD    Exit() { return _to##Exit(); }  \
  NS_IMETHOD    SetToolbarWindow(nsIDOMWindow* aWin) { return _to##SetToolbarWindow(aWin); }  \
  NS_IMETHOD    SetContentWindow(nsIDOMWindow* aWin) { return _to##SetContentWindow(aWin); }  \
  NS_IMETHOD    SetWebShellWindow(nsIDOMWindow* aWin) { return _to##SetWebShellWindow(aWin); }  \


extern "C" NS_DOM nsresult NS_InitEditorAppCoreClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptEditorAppCore(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMEditorAppCore_h__
