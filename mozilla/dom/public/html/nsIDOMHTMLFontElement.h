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

#ifndef nsIDOMHTMLFontElement_h__
#define nsIDOMHTMLFontElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLFONTELEMENT_IID \
{ 0x6f7652fd,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLFontElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetColor(nsString& aColor)=0;
  NS_IMETHOD    SetColor(const nsString& aColor)=0;

  NS_IMETHOD    GetFace(nsString& aFace)=0;
  NS_IMETHOD    SetFace(const nsString& aFace)=0;

  NS_IMETHOD    GetSize(nsString& aSize)=0;
  NS_IMETHOD    SetSize(const nsString& aSize)=0;
};


#define NS_DECL_IDOMHTMLFONTELEMENT   \
  NS_IMETHOD    GetColor(nsString& aColor);  \
  NS_IMETHOD    SetColor(const nsString& aColor);  \
  NS_IMETHOD    GetFace(nsString& aFace);  \
  NS_IMETHOD    SetFace(const nsString& aFace);  \
  NS_IMETHOD    GetSize(nsString& aSize);  \
  NS_IMETHOD    SetSize(const nsString& aSize);  \



#define NS_FORWARD_IDOMHTMLFONTELEMENT(superClass)  \
  NS_IMETHOD    GetColor(nsString& aColor) { return superClass::GetColor(aColor); } \
  NS_IMETHOD    SetColor(const nsString& aColor) { return superClass::SetColor(aColor); } \
  NS_IMETHOD    GetFace(nsString& aFace) { return superClass::GetFace(aFace); } \
  NS_IMETHOD    SetFace(const nsString& aFace) { return superClass::SetFace(aFace); } \
  NS_IMETHOD    GetSize(nsString& aSize) { return superClass::GetSize(aSize); } \
  NS_IMETHOD    SetSize(const nsString& aSize) { return superClass::SetSize(aSize); } \


extern nsresult NS_InitHTMLFontElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLFontElement(nsIScriptContext *aContext, nsIDOMHTMLFontElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLFontElement_h__
