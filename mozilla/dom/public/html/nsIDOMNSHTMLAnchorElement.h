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

#ifndef nsIDOMNSHTMLAnchorElement_h__
#define nsIDOMNSHTMLAnchorElement_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"


#define NS_IDOMNSHTMLANCHORELEMENT_IID \
 { 0xa6cf911c, 0x15b3, 0x11d2,  \
  { 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } } 

class NS_NO_VTABLE nsIDOMNSHTMLAnchorElement : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IDOMNSHTMLANCHORELEMENT_IID)

  NS_IMETHOD    GetProtocol(nsAWritableString& aProtocol)=0;

  NS_IMETHOD    GetHost(nsAWritableString& aHost)=0;

  NS_IMETHOD    GetHostname(nsAWritableString& aHostname)=0;

  NS_IMETHOD    GetPathname(nsAWritableString& aPathname)=0;

  NS_IMETHOD    GetSearch(nsAWritableString& aSearch)=0;

  NS_IMETHOD    GetPort(nsAWritableString& aPort)=0;

  NS_IMETHOD    GetHash(nsAWritableString& aHash)=0;

  NS_IMETHOD    GetText(nsAWritableString& aText)=0;
};


#define NS_DECL_IDOMNSHTMLANCHORELEMENT   \
  NS_IMETHOD    GetProtocol(nsAWritableString& aProtocol);  \
  NS_IMETHOD    GetHost(nsAWritableString& aHost);  \
  NS_IMETHOD    GetHostname(nsAWritableString& aHostname);  \
  NS_IMETHOD    GetPathname(nsAWritableString& aPathname);  \
  NS_IMETHOD    GetSearch(nsAWritableString& aSearch);  \
  NS_IMETHOD    GetPort(nsAWritableString& aPort);  \
  NS_IMETHOD    GetHash(nsAWritableString& aHash);  \
  NS_IMETHOD    GetText(nsAWritableString& aText);  \



#define NS_FORWARD_IDOMNSHTMLANCHORELEMENT(_to)  \
  NS_IMETHOD    GetProtocol(nsAWritableString& aProtocol) { return _to GetProtocol(aProtocol); } \
  NS_IMETHOD    GetHost(nsAWritableString& aHost) { return _to GetHost(aHost); } \
  NS_IMETHOD    GetHostname(nsAWritableString& aHostname) { return _to GetHostname(aHostname); } \
  NS_IMETHOD    GetPathname(nsAWritableString& aPathname) { return _to GetPathname(aPathname); } \
  NS_IMETHOD    GetSearch(nsAWritableString& aSearch) { return _to GetSearch(aSearch); } \
  NS_IMETHOD    GetPort(nsAWritableString& aPort) { return _to GetPort(aPort); } \
  NS_IMETHOD    GetHash(nsAWritableString& aHash) { return _to GetHash(aHash); } \
  NS_IMETHOD    GetText(nsAWritableString& aText) { return _to GetText(aText); } \


#endif // nsIDOMNSHTMLAnchorElement_h__
