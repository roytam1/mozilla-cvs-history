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

#ifndef nsIDOMCSSMediaRule_h__
#define nsIDOMCSSMediaRule_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMCSSStyleRule.h"

class nsIDOMCSSStyleRuleCollection;

#define NS_IDOMCSSMEDIARULE_IID \
{ 0x6f765333,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMCSSMediaRule : public nsIDOMCSSStyleRule {
public:

  NS_IMETHOD    GetMediaTypes(nsString& aMediaTypes)=0;
  NS_IMETHOD    SetMediaTypes(const nsString& aMediaTypes)=0;

  NS_IMETHOD    GetRules(nsIDOMCSSStyleRuleCollection** aRules)=0;

  NS_IMETHOD    AddRule(const nsString& aSelector, const nsString& aDeclaration, PRUint32 aIndex, PRUint32* aReturn)=0;

  NS_IMETHOD    RemoveRule(PRUint32 aIndex)=0;
};


#define NS_DECL_IDOMCSSMEDIARULE   \
  NS_IMETHOD    GetMediaTypes(nsString& aMediaTypes);  \
  NS_IMETHOD    SetMediaTypes(const nsString& aMediaTypes);  \
  NS_IMETHOD    GetRules(nsIDOMCSSStyleRuleCollection** aRules);  \
  NS_IMETHOD    AddRule(const nsString& aSelector, const nsString& aDeclaration, PRUint32 aIndex, PRUint32* aReturn);  \
  NS_IMETHOD    RemoveRule(PRUint32 aIndex);  \



#define NS_FORWARD_IDOMCSSMEDIARULE(_to)  \
  NS_IMETHOD    GetMediaTypes(nsString& aMediaTypes) { return _to##GetMediaTypes(aMediaTypes); } \
  NS_IMETHOD    SetMediaTypes(const nsString& aMediaTypes) { return _to##SetMediaTypes(aMediaTypes); } \
  NS_IMETHOD    GetRules(nsIDOMCSSStyleRuleCollection** aRules) { return _to##GetRules(aRules); } \
  NS_IMETHOD    AddRule(const nsString& aSelector, const nsString& aDeclaration, PRUint32 aIndex, PRUint32* aReturn) { return _to##AddRule(aSelector, aDeclaration, aIndex, aReturn); }  \
  NS_IMETHOD    RemoveRule(PRUint32 aIndex) { return _to##RemoveRule(aIndex); }  \


extern nsresult NS_InitCSSMediaRuleClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptCSSMediaRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMCSSMediaRule_h__
