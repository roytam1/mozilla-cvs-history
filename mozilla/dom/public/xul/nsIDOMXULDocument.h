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
 */
/* AUTO-GENERATED. DO NOT EDIT!!! */

#ifndef nsIDOMXULDocument_h__
#define nsIDOMXULDocument_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsIDOMDocument.h"

class nsIDOMElement;
class nsIDOMNode;
class nsIDOMXULCommandDispatcher;
class nsIDOMHTMLCollection;
class nsIDOMNodeList;

#define NS_IDOMXULDOCUMENT_IID \
 { 0x17ddd8c0, 0xc5f8, 0x11d2, \
        { 0xa6, 0xae, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } } 

class nsIDOMXULDocument : public nsIDOMDocument {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOMXULDOCUMENT_IID; return iid; }

  NS_IMETHOD    GetPopupNode(nsIDOMNode** aPopupNode)=0;
  NS_IMETHOD    SetPopupNode(nsIDOMNode* aPopupNode)=0;

  NS_IMETHOD    GetTooltipNode(nsIDOMNode** aTooltipNode)=0;
  NS_IMETHOD    SetTooltipNode(nsIDOMNode* aTooltipNode)=0;

  NS_IMETHOD    GetCommandDispatcher(nsIDOMXULCommandDispatcher** aCommandDispatcher)=0;

  NS_IMETHOD    GetControls(nsIDOMHTMLCollection** aControls)=0;

  NS_IMETHOD    GetElementById(const nsString& aId, nsIDOMElement** aReturn)=0;

  NS_IMETHOD    GetElementsByAttribute(const nsString& aName, const nsString& aValue, nsIDOMNodeList** aReturn)=0;

  NS_IMETHOD    Persist(const nsString& aId, const nsString& aAttr)=0;
};


#define NS_DECL_IDOMXULDOCUMENT   \
  NS_IMETHOD    GetPopupNode(nsIDOMNode** aPopupNode);  \
  NS_IMETHOD    SetPopupNode(nsIDOMNode* aPopupNode);  \
  NS_IMETHOD    GetTooltipNode(nsIDOMNode** aTooltipNode);  \
  NS_IMETHOD    SetTooltipNode(nsIDOMNode* aTooltipNode);  \
  NS_IMETHOD    GetCommandDispatcher(nsIDOMXULCommandDispatcher** aCommandDispatcher);  \
  NS_IMETHOD    GetControls(nsIDOMHTMLCollection** aControls);  \
  NS_IMETHOD    GetElementById(const nsString& aId, nsIDOMElement** aReturn);  \
  NS_IMETHOD    GetElementsByAttribute(const nsString& aName, const nsString& aValue, nsIDOMNodeList** aReturn);  \
  NS_IMETHOD    Persist(const nsString& aId, const nsString& aAttr);  \



#define NS_FORWARD_IDOMXULDOCUMENT(_to)  \
  NS_IMETHOD    GetPopupNode(nsIDOMNode** aPopupNode) { return _to GetPopupNode(aPopupNode); } \
  NS_IMETHOD    SetPopupNode(nsIDOMNode* aPopupNode) { return _to SetPopupNode(aPopupNode); } \
  NS_IMETHOD    GetTooltipNode(nsIDOMNode** aTooltipNode) { return _to GetTooltipNode(aTooltipNode); } \
  NS_IMETHOD    SetTooltipNode(nsIDOMNode* aTooltipNode) { return _to SetTooltipNode(aTooltipNode); } \
  NS_IMETHOD    GetCommandDispatcher(nsIDOMXULCommandDispatcher** aCommandDispatcher) { return _to GetCommandDispatcher(aCommandDispatcher); } \
  NS_IMETHOD    GetControls(nsIDOMHTMLCollection** aControls) { return _to GetControls(aControls); } \
  NS_IMETHOD    GetElementById(const nsString& aId, nsIDOMElement** aReturn) { return _to GetElementById(aId, aReturn); }  \
  NS_IMETHOD    GetElementsByAttribute(const nsString& aName, const nsString& aValue, nsIDOMNodeList** aReturn) { return _to GetElementsByAttribute(aName, aValue, aReturn); }  \
  NS_IMETHOD    Persist(const nsString& aId, const nsString& aAttr) { return _to Persist(aId, aAttr); }  \


extern "C" NS_DOM nsresult NS_InitXULDocumentClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" NS_DOM nsresult NS_NewScriptXULDocument(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif // nsIDOMXULDocument_h__
