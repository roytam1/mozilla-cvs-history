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

#ifndef nsIDOMHTMLTbodyElement_h__
#define nsIDOMHTMLTbodyElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLTBODYELEMENT_IID \
{ 0x6f765323,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLTbodyElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetAlign(nsString& aAlign)=0;
  NS_IMETHOD    SetAlign(const nsString& aAlign)=0;

  NS_IMETHOD    GetCh(nsString& aCh)=0;
  NS_IMETHOD    SetCh(const nsString& aCh)=0;

  NS_IMETHOD    GetChOff(nsString& aChOff)=0;
  NS_IMETHOD    SetChOff(const nsString& aChOff)=0;

  NS_IMETHOD    GetVAlign(nsString& aVAlign)=0;
  NS_IMETHOD    SetVAlign(const nsString& aVAlign)=0;
};


#define NS_DECL_IDOMHTMLTBODYELEMENT   \
  NS_IMETHOD    GetAlign(nsString& aAlign);  \
  NS_IMETHOD    SetAlign(const nsString& aAlign);  \
  NS_IMETHOD    GetCh(nsString& aCh);  \
  NS_IMETHOD    SetCh(const nsString& aCh);  \
  NS_IMETHOD    GetChOff(nsString& aChOff);  \
  NS_IMETHOD    SetChOff(const nsString& aChOff);  \
  NS_IMETHOD    GetVAlign(nsString& aVAlign);  \
  NS_IMETHOD    SetVAlign(const nsString& aVAlign);  \



#define NS_FORWARD_IDOMHTMLTBODYELEMENT(superClass)  \
  NS_IMETHOD    GetAlign(nsString& aAlign) { return superClass::GetAlign(aAlign); } \
  NS_IMETHOD    SetAlign(const nsString& aAlign) { return superClass::SetAlign(aAlign); } \
  NS_IMETHOD    GetCh(nsString& aCh) { return superClass::GetCh(aCh); } \
  NS_IMETHOD    SetCh(const nsString& aCh) { return superClass::SetCh(aCh); } \
  NS_IMETHOD    GetChOff(nsString& aChOff) { return superClass::GetChOff(aChOff); } \
  NS_IMETHOD    SetChOff(const nsString& aChOff) { return superClass::SetChOff(aChOff); } \
  NS_IMETHOD    GetVAlign(nsString& aVAlign) { return superClass::GetVAlign(aVAlign); } \
  NS_IMETHOD    SetVAlign(const nsString& aVAlign) { return superClass::SetVAlign(aVAlign); } \


extern nsresult NS_InitHTMLTbodyElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLTbodyElement(nsIScriptContext *aContext, nsIDOMHTMLTbodyElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLTbodyElement_h__
