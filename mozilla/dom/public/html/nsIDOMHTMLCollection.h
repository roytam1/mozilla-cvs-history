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

#ifndef nsIDOMHTMLCollection_h__
#define nsIDOMHTMLCollection_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"

class nsIDOMNode;

#define NS_IDOMHTMLCOLLECTION_IID \
 { 0xa6cf9083, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMHTMLCollection : public nsISupports {
public:

  NS_IMETHOD    GetLength(PRUint32* aLength)=0;

  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn)=0;

  NS_IMETHOD    NamedItem(const nsString& aName, nsIDOMNode** aReturn)=0;
};


#define NS_DECL_IDOMHTMLCOLLECTION   \
  NS_IMETHOD    GetLength(PRUint32* aLength);  \
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn);  \
  NS_IMETHOD    NamedItem(const nsString& aName, nsIDOMNode** aReturn);  \



#define NS_FORWARD_IDOMHTMLCOLLECTION(_to)  \
  NS_IMETHOD    GetLength(PRUint32* aLength) { return _to##GetLength(aLength); } \
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMNode** aReturn) { return _to##Item(aIndex, aReturn); }  \
  NS_IMETHOD    NamedItem(const nsString& aName, nsIDOMNode** aReturn) { return _to##NamedItem(aName, aReturn); }  \


extern nsresult NS_InitHTMLCollectionClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLCollection(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLCollection_h__
