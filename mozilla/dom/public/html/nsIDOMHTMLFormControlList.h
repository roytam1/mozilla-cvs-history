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

#ifndef nsIDOMHTMLFormControlList_h__
#define nsIDOMHTMLFormControlList_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLCollection.h"
#include "jsapi.h"


#define NS_IDOMHTMLFORMCONTROLLIST_IID \
 { 0xa6cf911a, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMHTMLFormControlList : public nsIDOMHTMLCollection {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMHTMLFORMCONTROLLIST_IID; return iid; }

  NS_IMETHOD    NamedItem(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn)=0;

  NS_IMETHOD    Item(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn)=0;
};


#define NS_DECL_IDOMHTMLFORMCONTROLLIST   \
  NS_IMETHOD    NamedItem(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn);  \
  NS_IMETHOD    Item(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn);  \



#define NS_FORWARD_IDOMHTMLFORMCONTROLLIST(_to)  \
  NS_IMETHOD    NamedItem(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn) { return _to NamedItem(cx, argv, argc, aReturn); }  \
  NS_IMETHOD    Item(JSContext* cx, jsval* argv, PRUint32 argc, jsval* aReturn) { return _to Item(cx, argv, argc, aReturn); }  \


extern "C" NS_DOM nsresult NS_InitHTMLFormControlListClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLFormControlList(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLFormControlList_h__
