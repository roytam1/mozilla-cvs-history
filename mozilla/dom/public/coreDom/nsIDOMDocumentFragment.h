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

#ifndef nsIDOMDocumentFragment_h__
#define nsIDOMDocumentFragment_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMNode.h"

class nsIDOMDocument;

#define NS_IDOMDOCUMENTFRAGMENT_IID \
{ 0x6f7652e5,  0xee43, 0x11d1, \
 { 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } } 

class nsIDOMDocumentFragment : public nsIDOMNode {
public:

  NS_IMETHOD    GetMasterDoc(nsIDOMDocument** aMasterDoc)=0;
};


#define NS_DECL_IDOMDOCUMENTFRAGMENT   \
  NS_IMETHOD    GetMasterDoc(nsIDOMDocument** aMasterDoc);  \



#define NS_FORWARD_IDOMDOCUMENTFRAGMENT(superClass)  \
  NS_IMETHOD    GetMasterDoc(nsIDOMDocument** aMasterDoc) { return superClass::GetMasterDoc(aMasterDoc); } \


extern nsresult NS_InitDocumentFragmentClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptDocumentFragment(nsIScriptContext *aContext, nsIDOMDocumentFragment *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMDocumentFragment_h__
