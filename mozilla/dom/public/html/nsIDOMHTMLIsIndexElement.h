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

#ifndef nsIDOMHTMLIsIndexElement_h__
#define nsIDOMHTMLIsIndexElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLFormElement;

#define NS_IDOMHTMLISINDEXELEMENT_IID \
{ 0x6f765309,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLIsIndexElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm)=0;
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm)=0;

  NS_IMETHOD    GetPrompt(nsString& aPrompt)=0;
  NS_IMETHOD    SetPrompt(const nsString& aPrompt)=0;
};


#define NS_DECL_IDOMHTMLISINDEXELEMENT   \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm);  \
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm);  \
  NS_IMETHOD    GetPrompt(nsString& aPrompt);  \
  NS_IMETHOD    SetPrompt(const nsString& aPrompt);  \



#define NS_FORWARD_IDOMHTMLISINDEXELEMENT(superClass)  \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm) { return superClass::GetForm(aForm); } \
  NS_IMETHOD    SetForm(nsIDOMHTMLFormElement* aForm) { return superClass::SetForm(aForm); } \
  NS_IMETHOD    GetPrompt(nsString& aPrompt) { return superClass::GetPrompt(aPrompt); } \
  NS_IMETHOD    SetPrompt(const nsString& aPrompt) { return superClass::SetPrompt(aPrompt); } \


extern nsresult NS_InitHTMLIsIndexElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLIsIndexElement(nsIScriptContext *aContext, nsIDOMHTMLIsIndexElement *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLIsIndexElement_h__
