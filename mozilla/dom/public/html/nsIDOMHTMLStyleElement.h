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

#ifndef nsIDOMHTMLStyleElement_h__
#define nsIDOMHTMLStyleElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLSTYLEELEMENT_IID \
{ 0x6f76531c,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLStyleElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetDisabled(PRBool* aDisabled)=0;
  NS_IMETHOD    SetDisabled(PRBool aDisabled)=0;

  NS_IMETHOD    GetMedia(nsString& aMedia)=0;
  NS_IMETHOD    SetMedia(const nsString& aMedia)=0;

  NS_IMETHOD    GetType(nsString& aType)=0;
  NS_IMETHOD    SetType(const nsString& aType)=0;
};


#define NS_DECL_IDOMHTMLSTYLEELEMENT   \
  NS_IMETHOD    GetDisabled(PRBool* aDisabled);  \
  NS_IMETHOD    SetDisabled(PRBool aDisabled);  \
  NS_IMETHOD    GetMedia(nsString& aMedia);  \
  NS_IMETHOD    SetMedia(const nsString& aMedia);  \
  NS_IMETHOD    GetType(nsString& aType);  \
  NS_IMETHOD    SetType(const nsString& aType);  \



#define NS_FORWARD_IDOMHTMLSTYLEELEMENT(_to)  \
  NS_IMETHOD    GetDisabled(PRBool* aDisabled) { return _to##GetDisabled(aDisabled); } \
  NS_IMETHOD    SetDisabled(PRBool aDisabled) { return _to##SetDisabled(aDisabled); } \
  NS_IMETHOD    GetMedia(nsString& aMedia) { return _to##GetMedia(aMedia); } \
  NS_IMETHOD    SetMedia(const nsString& aMedia) { return _to##SetMedia(aMedia); } \
  NS_IMETHOD    GetType(nsString& aType) { return _to##GetType(aType); } \
  NS_IMETHOD    SetType(const nsString& aType) { return _to##SetType(aType); } \


extern nsresult NS_InitHTMLStyleElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLStyleElement(nsIScriptContext *aContext, nsIDOMHTMLStyleElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLStyleElement_h__
