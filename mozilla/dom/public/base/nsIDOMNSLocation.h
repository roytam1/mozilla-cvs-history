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

#ifndef nsIDOMNSLocation_h__
#define nsIDOMNSLocation_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "jsapi.h"


#define NS_IDOMNSLOCATION_IID \
 { 0xa6cf9108, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class NS_NO_VTABLE nsIDOMNSLocation : public nsISupports {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMNSLOCATION_IID; return iid; }

  NS_IMETHOD    Reload(JSContext* cx, jsval* argv, PRUint32 argc)=0;

  NS_IMETHOD    Replace(JSContext* cx, jsval* argv, PRUint32 argc)=0;
};


#define NS_DECL_IDOMNSLOCATION   \
  NS_IMETHOD    Reload(JSContext* cx, jsval* argv, PRUint32 argc);  \
  NS_IMETHOD    Replace(JSContext* cx, jsval* argv, PRUint32 argc);  \



#define NS_FORWARD_IDOMNSLOCATION(_to)  \
  NS_IMETHOD    Reload(JSContext* cx, jsval* argv, PRUint32 argc) { return _to Reload(cx, argv, argc); }  \
  NS_IMETHOD    Replace(JSContext* cx, jsval* argv, PRUint32 argc) { return _to Replace(cx, argv, argc); }  \


#endif // nsIDOMNSLocation_h__
