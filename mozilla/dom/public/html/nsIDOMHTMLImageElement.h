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

#ifndef nsIDOMHTMLImageElement_h__
#define nsIDOMHTMLImageElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMHTMLElement.h"


#define NS_IDOMHTMLIMAGEELEMENT_IID \
 { 0xa6cf90ab, 0x15b3, 0x11d2, \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class nsIDOMHTMLImageElement : public nsIDOMHTMLElement {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMHTMLIMAGEELEMENT_IID; return iid; }

  NS_IMETHOD    GetLowSrc(nsString& aLowSrc)=0;
  NS_IMETHOD    SetLowSrc(const nsString& aLowSrc)=0;

  NS_IMETHOD    GetName(nsString& aName)=0;
  NS_IMETHOD    SetName(const nsString& aName)=0;

  NS_IMETHOD    GetAlign(nsString& aAlign)=0;
  NS_IMETHOD    SetAlign(const nsString& aAlign)=0;

  NS_IMETHOD    GetAlt(nsString& aAlt)=0;
  NS_IMETHOD    SetAlt(const nsString& aAlt)=0;

  NS_IMETHOD    GetBorder(nsString& aBorder)=0;
  NS_IMETHOD    SetBorder(const nsString& aBorder)=0;

  NS_IMETHOD    GetHeight(nsString& aHeight)=0;
  NS_IMETHOD    SetHeight(const nsString& aHeight)=0;

  NS_IMETHOD    GetHspace(nsString& aHspace)=0;
  NS_IMETHOD    SetHspace(const nsString& aHspace)=0;

  NS_IMETHOD    GetIsMap(PRBool* aIsMap)=0;
  NS_IMETHOD    SetIsMap(PRBool aIsMap)=0;

  NS_IMETHOD    GetLongDesc(nsString& aLongDesc)=0;
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc)=0;

  NS_IMETHOD    GetSrc(nsString& aSrc)=0;
  NS_IMETHOD    SetSrc(const nsString& aSrc)=0;

  NS_IMETHOD    GetUseMap(nsString& aUseMap)=0;
  NS_IMETHOD    SetUseMap(const nsString& aUseMap)=0;

  NS_IMETHOD    GetVspace(nsString& aVspace)=0;
  NS_IMETHOD    SetVspace(const nsString& aVspace)=0;

  NS_IMETHOD    GetWidth(nsString& aWidth)=0;
  NS_IMETHOD    SetWidth(const nsString& aWidth)=0;
};


#define NS_DECL_IDOMHTMLIMAGEELEMENT   \
  NS_IMETHOD    GetLowSrc(nsString& aLowSrc);  \
  NS_IMETHOD    SetLowSrc(const nsString& aLowSrc);  \
  NS_IMETHOD    GetName(nsString& aName);  \
  NS_IMETHOD    SetName(const nsString& aName);  \
  NS_IMETHOD    GetAlign(nsString& aAlign);  \
  NS_IMETHOD    SetAlign(const nsString& aAlign);  \
  NS_IMETHOD    GetAlt(nsString& aAlt);  \
  NS_IMETHOD    SetAlt(const nsString& aAlt);  \
  NS_IMETHOD    GetBorder(nsString& aBorder);  \
  NS_IMETHOD    SetBorder(const nsString& aBorder);  \
  NS_IMETHOD    GetHeight(nsString& aHeight);  \
  NS_IMETHOD    SetHeight(const nsString& aHeight);  \
  NS_IMETHOD    GetHspace(nsString& aHspace);  \
  NS_IMETHOD    SetHspace(const nsString& aHspace);  \
  NS_IMETHOD    GetIsMap(PRBool* aIsMap);  \
  NS_IMETHOD    SetIsMap(PRBool aIsMap);  \
  NS_IMETHOD    GetLongDesc(nsString& aLongDesc);  \
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc);  \
  NS_IMETHOD    GetSrc(nsString& aSrc);  \
  NS_IMETHOD    SetSrc(const nsString& aSrc);  \
  NS_IMETHOD    GetUseMap(nsString& aUseMap);  \
  NS_IMETHOD    SetUseMap(const nsString& aUseMap);  \
  NS_IMETHOD    GetVspace(nsString& aVspace);  \
  NS_IMETHOD    SetVspace(const nsString& aVspace);  \
  NS_IMETHOD    GetWidth(nsString& aWidth);  \
  NS_IMETHOD    SetWidth(const nsString& aWidth);  \



#define NS_FORWARD_IDOMHTMLIMAGEELEMENT(_to)  \
  NS_IMETHOD    GetLowSrc(nsString& aLowSrc) { return _to GetLowSrc(aLowSrc); } \
  NS_IMETHOD    SetLowSrc(const nsString& aLowSrc) { return _to SetLowSrc(aLowSrc); } \
  NS_IMETHOD    GetName(nsString& aName) { return _to GetName(aName); } \
  NS_IMETHOD    SetName(const nsString& aName) { return _to SetName(aName); } \
  NS_IMETHOD    GetAlign(nsString& aAlign) { return _to GetAlign(aAlign); } \
  NS_IMETHOD    SetAlign(const nsString& aAlign) { return _to SetAlign(aAlign); } \
  NS_IMETHOD    GetAlt(nsString& aAlt) { return _to GetAlt(aAlt); } \
  NS_IMETHOD    SetAlt(const nsString& aAlt) { return _to SetAlt(aAlt); } \
  NS_IMETHOD    GetBorder(nsString& aBorder) { return _to GetBorder(aBorder); } \
  NS_IMETHOD    SetBorder(const nsString& aBorder) { return _to SetBorder(aBorder); } \
  NS_IMETHOD    GetHeight(nsString& aHeight) { return _to GetHeight(aHeight); } \
  NS_IMETHOD    SetHeight(const nsString& aHeight) { return _to SetHeight(aHeight); } \
  NS_IMETHOD    GetHspace(nsString& aHspace) { return _to GetHspace(aHspace); } \
  NS_IMETHOD    SetHspace(const nsString& aHspace) { return _to SetHspace(aHspace); } \
  NS_IMETHOD    GetIsMap(PRBool* aIsMap) { return _to GetIsMap(aIsMap); } \
  NS_IMETHOD    SetIsMap(PRBool aIsMap) { return _to SetIsMap(aIsMap); } \
  NS_IMETHOD    GetLongDesc(nsString& aLongDesc) { return _to GetLongDesc(aLongDesc); } \
  NS_IMETHOD    SetLongDesc(const nsString& aLongDesc) { return _to SetLongDesc(aLongDesc); } \
  NS_IMETHOD    GetSrc(nsString& aSrc) { return _to GetSrc(aSrc); } \
  NS_IMETHOD    SetSrc(const nsString& aSrc) { return _to SetSrc(aSrc); } \
  NS_IMETHOD    GetUseMap(nsString& aUseMap) { return _to GetUseMap(aUseMap); } \
  NS_IMETHOD    SetUseMap(const nsString& aUseMap) { return _to SetUseMap(aUseMap); } \
  NS_IMETHOD    GetVspace(nsString& aVspace) { return _to GetVspace(aVspace); } \
  NS_IMETHOD    SetVspace(const nsString& aVspace) { return _to SetVspace(aVspace); } \
  NS_IMETHOD    GetWidth(nsString& aWidth) { return _to GetWidth(aWidth); } \
  NS_IMETHOD    SetWidth(const nsString& aWidth) { return _to SetWidth(aWidth); } \


extern "C" NS_DOM nsresult NS_InitHTMLImageElementClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptHTMLImageElement(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMHTMLImageElement_h__
