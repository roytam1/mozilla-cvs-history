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

#ifndef nsIDOMHTMLFrameElement_h__
#define nsIDOMHTMLFrameElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLFRAMEELEMENT_IID \
{ 0x6f7652ff,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLFrameElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetFrameBorder(nsString& aFrameBorder)=0;
  NS_IMETHOD    SetFrameBorder(const nsString& aFrameBorder)=0;

  NS_IMETHOD    GetLongDesc(nsString& aLongDesc)=0;
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc)=0;

  NS_IMETHOD    GetMarginHeight(nsString& aMarginHeight)=0;
  NS_IMETHOD    SetMarginHeight(const nsString& aMarginHeight)=0;

  NS_IMETHOD    GetMarginWidth(nsString& aMarginWidth)=0;
  NS_IMETHOD    SetMarginWidth(const nsString& aMarginWidth)=0;

  NS_IMETHOD    GetName(nsString& aName)=0;
  NS_IMETHOD    SetName(const nsString& aName)=0;

  NS_IMETHOD    GetNoResize(PRBool* aNoResize)=0;
  NS_IMETHOD    SetNoResize(PRBool aNoResize)=0;

  NS_IMETHOD    GetScrolling(nsString& aScrolling)=0;
  NS_IMETHOD    SetScrolling(const nsString& aScrolling)=0;

  NS_IMETHOD    GetSrc(nsString& aSrc)=0;
  NS_IMETHOD    SetSrc(const nsString& aSrc)=0;
};


#define NS_DECL_IDOMHTMLFRAMEELEMENT   \
  NS_IMETHOD    GetFrameBorder(nsString& aFrameBorder);  \
  NS_IMETHOD    SetFrameBorder(const nsString& aFrameBorder);  \
  NS_IMETHOD    GetLongDesc(nsString& aLongDesc);  \
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc);  \
  NS_IMETHOD    GetMarginHeight(nsString& aMarginHeight);  \
  NS_IMETHOD    SetMarginHeight(const nsString& aMarginHeight);  \
  NS_IMETHOD    GetMarginWidth(nsString& aMarginWidth);  \
  NS_IMETHOD    SetMarginWidth(const nsString& aMarginWidth);  \
  NS_IMETHOD    GetName(nsString& aName);  \
  NS_IMETHOD    SetName(const nsString& aName);  \
  NS_IMETHOD    GetNoResize(PRBool* aNoResize);  \
  NS_IMETHOD    SetNoResize(PRBool aNoResize);  \
  NS_IMETHOD    GetScrolling(nsString& aScrolling);  \
  NS_IMETHOD    SetScrolling(const nsString& aScrolling);  \
  NS_IMETHOD    GetSrc(nsString& aSrc);  \
  NS_IMETHOD    SetSrc(const nsString& aSrc);  \



#define NS_FORWARD_IDOMHTMLFRAMEELEMENT(superClass)  \
  NS_IMETHOD    GetFrameBorder(nsString& aFrameBorder) { return superClass::GetFrameBorder(aFrameBorder); } \
  NS_IMETHOD    SetFrameBorder(const nsString& aFrameBorder) { return superClass::SetFrameBorder(aFrameBorder); } \
  NS_IMETHOD    GetLongDesc(nsString& aLongDesc) { return superClass::GetLongDesc(aLongDesc); } \
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc) { return superClass::SetLongDesc(aLongDesc); } \
  NS_IMETHOD    GetMarginHeight(nsString& aMarginHeight) { return superClass::GetMarginHeight(aMarginHeight); } \
  NS_IMETHOD    SetMarginHeight(const nsString& aMarginHeight) { return superClass::SetMarginHeight(aMarginHeight); } \
  NS_IMETHOD    GetMarginWidth(nsString& aMarginWidth) { return superClass::GetMarginWidth(aMarginWidth); } \
  NS_IMETHOD    SetMarginWidth(const nsString& aMarginWidth) { return superClass::SetMarginWidth(aMarginWidth); } \
  NS_IMETHOD    GetName(nsString& aName) { return superClass::GetName(aName); } \
  NS_IMETHOD    SetName(const nsString& aName) { return superClass::SetName(aName); } \
  NS_IMETHOD    GetNoResize(PRBool* aNoResize) { return superClass::GetNoResize(aNoResize); } \
  NS_IMETHOD    SetNoResize(PRBool aNoResize) { return superClass::SetNoResize(aNoResize); } \
  NS_IMETHOD    GetScrolling(nsString& aScrolling) { return superClass::GetScrolling(aScrolling); } \
  NS_IMETHOD    SetScrolling(const nsString& aScrolling) { return superClass::SetScrolling(aScrolling); } \
  NS_IMETHOD    GetSrc(nsString& aSrc) { return superClass::GetSrc(aSrc); } \
  NS_IMETHOD    SetSrc(const nsString& aSrc) { return superClass::SetSrc(aSrc); } \


extern nsresult NS_InitHTMLFrameElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLFrameElement(nsIScriptContext *aContext, nsIDOMHTMLFrameElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLFrameElement_h__
