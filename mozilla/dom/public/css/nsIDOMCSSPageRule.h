/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMCSSPageRule_h__
#define nsIDOMCSSPageRule_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMCSSRule.h"

class nsIDOMCSSStyleDeclaration;

#define NS_IDOMCSSPAGERULE_IID \
 { 0xa6cf90bd, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMCSSPageRule : public nsIDOMCSSRule {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMCSSPAGERULE_IID; return iid; }

  NS_IMETHOD    GetSelectorText(nsString& aSelectorText)=0;
  NS_IMETHOD    SetSelectorText(const nsString& aSelectorText)=0;

  NS_IMETHOD    GetStyle(nsIDOMCSSStyleDeclaration** aStyle)=0;
};


#define NS_DECL_IDOMCSSPAGERULE   \
  NS_IMETHOD    GetSelectorText(nsString& aSelectorText);  \
  NS_IMETHOD    SetSelectorText(const nsString& aSelectorText);  \
  NS_IMETHOD    GetStyle(nsIDOMCSSStyleDeclaration** aStyle);  \



#define NS_FORWARD_IDOMCSSPAGERULE(_to)  \
  NS_IMETHOD    GetSelectorText(nsString& aSelectorText) { return _to GetSelectorText(aSelectorText); } \
  NS_IMETHOD    SetSelectorText(const nsString& aSelectorText) { return _to SetSelectorText(aSelectorText); } \
  NS_IMETHOD    GetStyle(nsIDOMCSSStyleDeclaration** aStyle) { return _to GetStyle(aStyle); } \


extern "C" NS_DOM nsresult NS_InitCSSPageRuleClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptCSSPageRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMCSSPageRule_h__
