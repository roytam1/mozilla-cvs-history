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

#ifndef nsIDOMHTMLMetaElement_h__
#define nsIDOMHTMLMetaElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLMETAELEMENT_IID \
{ 0x6f765310,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLMetaElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetContent(nsString& aContent)=0;
  NS_IMETHOD    SetContent(const nsString& aContent)=0;

  NS_IMETHOD    GetHttpEquiv(nsString& aHttpEquiv)=0;
  NS_IMETHOD    SetHttpEquiv(const nsString& aHttpEquiv)=0;

  NS_IMETHOD    GetName(nsString& aName)=0;
  NS_IMETHOD    SetName(const nsString& aName)=0;

  NS_IMETHOD    GetScheme(nsString& aScheme)=0;
  NS_IMETHOD    SetScheme(const nsString& aScheme)=0;
};


#define NS_DECL_IDOMHTMLMETAELEMENT   \
  NS_IMETHOD    GetContent(nsString& aContent);  \
  NS_IMETHOD    SetContent(const nsString& aContent);  \
  NS_IMETHOD    GetHttpEquiv(nsString& aHttpEquiv);  \
  NS_IMETHOD    SetHttpEquiv(const nsString& aHttpEquiv);  \
  NS_IMETHOD    GetName(nsString& aName);  \
  NS_IMETHOD    SetName(const nsString& aName);  \
  NS_IMETHOD    GetScheme(nsString& aScheme);  \
  NS_IMETHOD    SetScheme(const nsString& aScheme);  \



#define NS_FORWARD_IDOMHTMLMETAELEMENT(superClass)  \
  NS_IMETHOD    GetContent(nsString& aContent) { return superClass::GetContent(aContent); } \
  NS_IMETHOD    SetContent(const nsString& aContent) { return superClass::SetContent(aContent); } \
  NS_IMETHOD    GetHttpEquiv(nsString& aHttpEquiv) { return superClass::GetHttpEquiv(aHttpEquiv); } \
  NS_IMETHOD    SetHttpEquiv(const nsString& aHttpEquiv) { return superClass::SetHttpEquiv(aHttpEquiv); } \
  NS_IMETHOD    GetName(nsString& aName) { return superClass::GetName(aName); } \
  NS_IMETHOD    SetName(const nsString& aName) { return superClass::SetName(aName); } \
  NS_IMETHOD    GetScheme(nsString& aScheme) { return superClass::GetScheme(aScheme); } \
  NS_IMETHOD    SetScheme(const nsString& aScheme) { return superClass::SetScheme(aScheme); } \


extern nsresult NS_InitHTMLMetaElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLMetaElement(nsIScriptContext *aContext, nsIDOMHTMLMetaElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLMetaElement_h__
