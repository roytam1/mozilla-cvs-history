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

#ifndef nsIDOMHTMLQuoteElement_h__
#define nsIDOMHTMLQuoteElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLQUOTEELEMENT_IID \
{ 0x6f76531a,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLQuoteElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetCite(nsString& aCite)=0;
  NS_IMETHOD    SetCite(const nsString& aCite)=0;
};


#define NS_DECL_IDOMHTMLQUOTEELEMENT   \
  NS_IMETHOD    GetCite(nsString& aCite);  \
  NS_IMETHOD    SetCite(const nsString& aCite);  \



#define NS_FORWARD_IDOMHTMLQUOTEELEMENT(_to)  \
  NS_IMETHOD    GetCite(nsString& aCite) { return _to##GetCite(aCite); } \
  NS_IMETHOD    SetCite(const nsString& aCite) { return _to##SetCite(aCite); } \


extern nsresult NS_InitHTMLQuoteElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLQuoteElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLQuoteElement_h__
