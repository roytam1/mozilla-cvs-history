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

#ifndef nsIDOMHTMLDirectoryElement_h__
#define nsIDOMHTMLDirectoryElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLDIRECTORYELEMENT_IID \
{ 0x6f7652f8,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMHTMLDirectoryElement : public nsIDOMHTMLElement {
public:

  NS_IMETHOD    GetCompact(PRBool* aCompact)=0;
  NS_IMETHOD    SetCompact(PRBool aCompact)=0;
};


#define NS_DECL_IDOMHTMLDIRECTORYELEMENT   \
  NS_IMETHOD    GetCompact(PRBool* aCompact);  \
  NS_IMETHOD    SetCompact(PRBool aCompact);  \



#define NS_FORWARD_IDOMHTMLDIRECTORYELEMENT(_to)  \
  NS_IMETHOD    GetCompact(PRBool* aCompact) { return _to##GetCompact(aCompact); } \
  NS_IMETHOD    SetCompact(PRBool aCompact) { return _to##SetCompact(aCompact); } \


extern nsresult NS_InitHTMLDirectoryElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLDirectoryElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLDirectoryElement_h__
