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

#ifndef nsIDOMHTMLScriptElement_h__
#define nsIDOMHTMLScriptElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLScriptElement;

#define NS_IDOMHTMLSCRIPTELEMENT_IID \
{ 0x6f76531a,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLScriptElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetText(nsString& aText)=0;
  NS_IMETHOD    SetText(const nsString& aText)=0;

  NS_IMETHOD    GetHtmlFor(nsString& aHtmlFor)=0;
  NS_IMETHOD    SetHtmlFor(const nsString& aHtmlFor)=0;

  NS_IMETHOD    GetEvent(nsString& aEvent)=0;
  NS_IMETHOD    SetEvent(const nsString& aEvent)=0;

  NS_IMETHOD    GetCharset(nsString& aCharset)=0;
  NS_IMETHOD    SetCharset(const nsString& aCharset)=0;

  NS_IMETHOD    GetDefer(PRBool* aDefer)=0;
  NS_IMETHOD    SetDefer(PRBool aDefer)=0;

  NS_IMETHOD    GetSrc(nsString& aSrc)=0;
  NS_IMETHOD    SetSrc(const nsString& aSrc)=0;

  NS_IMETHOD    GetType(nsString& aType)=0;
  NS_IMETHOD    SetType(const nsString& aType)=0;
};

extern nsresult NS_InitHTMLScriptElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLScriptElement(nsIScriptContext *aContext, nsIDOMHTMLScriptElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLScriptElement_h__
