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

#ifndef nsIDOMMimeTypeArray_h__
#define nsIDOMMimeTypeArray_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"

class nsIDOMMimeType;

#define NS_IDOMMIMETYPEARRAY_IID \
 { 0xf6134683, 0xf28b, 0x11d2, { 0x83, 0x60, 0xc9, 0x08, 0x99, 0x04, 0x9c, 0x3c } } 

class NS_NO_VTABLE nsIDOMMimeTypeArray : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOMMIMETYPEARRAY_IID)

  NS_IMETHOD    GetLength(PRUint32* aLength)=0;

  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMMimeType** aReturn)=0;

  NS_IMETHOD    NamedItem(const nsAReadableString& aName, nsIDOMMimeType** aReturn)=0;
};


#define NS_DECL_IDOMMIMETYPEARRAY   \
  NS_IMETHOD    GetLength(PRUint32* aLength);  \
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMMimeType** aReturn);  \
  NS_IMETHOD    NamedItem(const nsAReadableString& aName, nsIDOMMimeType** aReturn);  \



#define NS_FORWARD_IDOMMIMETYPEARRAY(_to)  \
  NS_IMETHOD    GetLength(PRUint32* aLength) { return _to GetLength(aLength); } \
  NS_IMETHOD    Item(PRUint32 aIndex, nsIDOMMimeType** aReturn) { return _to Item(aIndex, aReturn); }  \
  NS_IMETHOD    NamedItem(const nsAReadableString& aName, nsIDOMMimeType** aReturn) { return _to NamedItem(aName, aReturn); }  \


extern "C" NS_DOM nsresult NS_InitMimeTypeArrayClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptMimeTypeArray(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMMimeTypeArray_h__
