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

#ifndef nsIDOMHTMLElement_h__
#define nsIDOMHTMLElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMElement.h"


#define NS_IDOMHTMLELEMENT_IID \
{ 0x6f7652fc,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLElement : public nsIDOMElement {
public:

  NS_IMETHOD    GetId(nsString& aId)=0;
  NS_IMETHOD    SetId(const nsString& aId)=0;

  NS_IMETHOD    GetTitle(nsString& aTitle)=0;
  NS_IMETHOD    SetTitle(const nsString& aTitle)=0;

  NS_IMETHOD    GetLang(nsString& aLang)=0;
  NS_IMETHOD    SetLang(const nsString& aLang)=0;

  NS_IMETHOD    GetDir(nsString& aDir)=0;
  NS_IMETHOD    SetDir(const nsString& aDir)=0;

  NS_IMETHOD    GetClassName(nsString& aClassName)=0;
  NS_IMETHOD    SetClassName(const nsString& aClassName)=0;
};


#define NS_DECL_IDOMHTMLELEMENT   \
  NS_IMETHOD    GetId(nsString& aId);  \
  NS_IMETHOD    SetId(const nsString& aId);  \
  NS_IMETHOD    GetTitle(nsString& aTitle);  \
  NS_IMETHOD    SetTitle(const nsString& aTitle);  \
  NS_IMETHOD    GetLang(nsString& aLang);  \
  NS_IMETHOD    SetLang(const nsString& aLang);  \
  NS_IMETHOD    GetDir(nsString& aDir);  \
  NS_IMETHOD    SetDir(const nsString& aDir);  \
  NS_IMETHOD    GetClassName(nsString& aClassName);  \
  NS_IMETHOD    SetClassName(const nsString& aClassName);  \



#define NS_FORWARD_IDOMHTMLELEMENT(superClass)  \
  NS_IMETHOD    GetId(nsString& aId) { return superClass::GetId(aId); } \
  NS_IMETHOD    SetId(const nsString& aId) { return superClass::SetId(aId); } \
  NS_IMETHOD    GetTitle(nsString& aTitle) { return superClass::GetTitle(aTitle); } \
  NS_IMETHOD    SetTitle(const nsString& aTitle) { return superClass::SetTitle(aTitle); } \
  NS_IMETHOD    GetLang(nsString& aLang) { return superClass::GetLang(aLang); } \
  NS_IMETHOD    SetLang(const nsString& aLang) { return superClass::SetLang(aLang); } \
  NS_IMETHOD    GetDir(nsString& aDir) { return superClass::GetDir(aDir); } \
  NS_IMETHOD    SetDir(const nsString& aDir) { return superClass::SetDir(aDir); } \
  NS_IMETHOD    GetClassName(nsString& aClassName) { return superClass::GetClassName(aClassName); } \
  NS_IMETHOD    SetClassName(const nsString& aClassName) { return superClass::SetClassName(aClassName); } \


extern nsresult NS_InitHTMLElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLElement(nsIScriptContext *aContext, nsIDOMHTMLElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLElement_h__
