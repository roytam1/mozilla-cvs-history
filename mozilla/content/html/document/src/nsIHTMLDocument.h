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
#ifndef nsIHTMLDocument_h___
#define nsIHTMLDocument_h___

#include "nsISupports.h"
#include "nsIDTD.h"

class nsIImageMap;
class nsString;
class nsIDOMNodeList;
class nsIDOMHTMLCollection;
class nsIDOMHTMLFormElement;
class nsIDOMHTMLMapElement;
class nsIHTMLStyleSheet;
class nsIStyleSheet;
class nsICSSLoader;
class nsIContent;
class nsIDOMHTMLBodyElement;

/* b2a848b0-d0a9-11d1-89b1-006008911b81 */
#define NS_IHTMLDOCUMENT_IID \
{0xb2a848b0, 0xd0a9, 0x11d1, {0x89, 0xb1, 0x00, 0x60, 0x08, 0x91, 0x1b, 0x81}}


/**
 * HTML document extensions to nsIDocument.
 */
class nsIHTMLDocument : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IHTMLDOCUMENT_IID)

  NS_IMETHOD AddImageMap(nsIDOMHTMLMapElement* aMap) = 0;

  NS_IMETHOD GetImageMap(const nsString& aMapName,
                         nsIDOMHTMLMapElement** aResult) = 0;

  NS_IMETHOD RemoveImageMap(nsIDOMHTMLMapElement* aMap) = 0;

  NS_IMETHOD SetLastModified(const nsAReadableString& aLastModified) = 0;
  NS_IMETHOD SetReferrer(const nsAReadableString& aReferrer) = 0;

  /**
   * Access DTD compatibility mode for this document
   */
  NS_IMETHOD GetDTDMode(nsDTDMode& aMode) = 0;
  NS_IMETHOD SetDTDMode(nsDTDMode aMode) = 0;

  /*
   * Returns true if document.domain was set for this document
   */
  NS_IMETHOD WasDomainSet(PRBool* aDomainWasSet) = 0;

  NS_IMETHOD ResolveName(const nsAReadableString& aName,
                         nsIDOMHTMLFormElement *aForm,
                         nsISupports **aResult) = 0;

  NS_IMETHOD GetFormControlElements(nsIDOMNodeList** aReturn) = 0;

  NS_IMETHOD GetBodyElement(nsIDOMHTMLBodyElement** aBody) = 0; 

};

#endif /* nsIHTMLDocument_h___ */
