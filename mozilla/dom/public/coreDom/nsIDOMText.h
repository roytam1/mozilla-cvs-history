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

#ifndef nsIDOMText_h__
#define nsIDOMText_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMData.h"

class nsIDOMText;

#define NS_IDOMTEXT_IID \
{ 0x6f7652ee,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMText : public nsIDOMData {
public:

  NS_IMETHOD    SplitText(PRUint32 aOffset, nsIDOMText** aReturn)=0;

  NS_IMETHOD    JoinText(nsIDOMText* aNode1, nsIDOMText* aNode2, nsIDOMText** aReturn)=0;
};


#define NS_DECL_IDOMTEXT   \
  NS_IMETHOD    SplitText(PRUint32 aOffset, nsIDOMText** aReturn);  \
  NS_IMETHOD    JoinText(nsIDOMText* aNode1, nsIDOMText* aNode2, nsIDOMText** aReturn);  \



#define NS_FORWARD_IDOMTEXT(_to)  \
  NS_IMETHOD    SplitText(PRUint32 aOffset, nsIDOMText** aReturn) { return _to##SplitText(aOffset, aReturn); }  \
  NS_IMETHOD    JoinText(nsIDOMText* aNode1, nsIDOMText* aNode2, nsIDOMText** aReturn) { return _to##JoinText(aNode1, aNode2, aReturn); }  \


extern nsresult NS_InitTextClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptText(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMText_h__
