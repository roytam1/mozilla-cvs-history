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

#ifndef nsIDOMHTMLIsIndexElement_h__
#define nsIDOMHTMLIsIndexElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"

class nsIDOMHTMLFormElement;

#define NS_IDOMHTMLISINDEXELEMENT_IID \
 { 0xa6cf908c, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class NS_NO_VTABLE nsIDOMHTMLIsIndexElement : public nsIDOMHTMLElement {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOMHTMLISINDEXELEMENT_IID)

  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm)=0;

  NS_IMETHOD    GetPrompt(nsAWritableString& aPrompt)=0;
  NS_IMETHOD    SetPrompt(const nsAReadableString& aPrompt)=0;
};


#define NS_DECL_IDOMHTMLISINDEXELEMENT   \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm);  \
  NS_IMETHOD    GetPrompt(nsAWritableString& aPrompt);  \
  NS_IMETHOD    SetPrompt(const nsAReadableString& aPrompt);  \



#define NS_FORWARD_IDOMHTMLISINDEXELEMENT(_to)  \
  NS_IMETHOD    GetForm(nsIDOMHTMLFormElement** aForm) { return _to GetForm(aForm); } \
  NS_IMETHOD    GetPrompt(nsAWritableString& aPrompt) { return _to GetPrompt(aPrompt); } \
  NS_IMETHOD    SetPrompt(const nsAReadableString& aPrompt) { return _to SetPrompt(aPrompt); } \


extern "C" NS_DOM nsresult NS_InitHTMLIsIndexElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLIsIndexElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLIsIndexElement_h__
