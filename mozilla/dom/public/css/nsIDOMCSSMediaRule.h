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

#ifndef nsIDOMCSSMediaRule_h__
#define nsIDOMCSSMediaRule_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMCSSRule.h"

class nsIDOMMediaList;
class nsIDOMCSSRuleList;

#define NS_IDOMCSSMEDIARULE_IID \
 { 0xa6cf90bc, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMCSSMediaRule : public nsIDOMCSSRule {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMCSSMEDIARULE_IID; return iid; }

  NS_IMETHOD    GetMedia(nsIDOMMediaList** aMedia)=0;

  NS_IMETHOD    GetCssRules(nsIDOMCSSRuleList** aCssRules)=0;

  NS_IMETHOD    InsertRule(const nsString& aRule, PRUint32 aIndex, PRUint32* aReturn)=0;

  NS_IMETHOD    DeleteRule(PRUint32 aIndex)=0;
};


#define NS_DECL_IDOMCSSMEDIARULE   \
  NS_IMETHOD    GetMedia(nsIDOMMediaList** aMedia);  \
  NS_IMETHOD    GetCssRules(nsIDOMCSSRuleList** aCssRules);  \
  NS_IMETHOD    InsertRule(const nsString& aRule, PRUint32 aIndex, PRUint32* aReturn);  \
  NS_IMETHOD    DeleteRule(PRUint32 aIndex);  \



#define NS_FORWARD_IDOMCSSMEDIARULE(_to)  \
  NS_IMETHOD    GetMedia(nsIDOMMediaList** aMedia) { return _to GetMedia(aMedia); } \
  NS_IMETHOD    GetCssRules(nsIDOMCSSRuleList** aCssRules) { return _to GetCssRules(aCssRules); } \
  NS_IMETHOD    InsertRule(const nsString& aRule, PRUint32 aIndex, PRUint32* aReturn) { return _to InsertRule(aRule, aIndex, aReturn); }  \
  NS_IMETHOD    DeleteRule(PRUint32 aIndex) { return _to DeleteRule(aIndex); }  \


extern "C" NS_DOM nsresult NS_InitCSSMediaRuleClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptCSSMediaRule(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMCSSMediaRule_h__
