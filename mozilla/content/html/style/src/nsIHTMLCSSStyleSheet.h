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
#ifndef nsIHTMLCSSStyleSheet_h___
#define nsIHTMLCSSStyleSheet_h___

#include "nslayout.h"
#include "nsIStyleSheet.h"

// IID for the nsIHTMLCSSStyleSheet interface {b5cc4ac0-eab6-11d1-8031-006008159b5a}
#define NS_IHTML_CSS_STYLE_SHEET_IID     \
{0xb5cc4ac0, 0xeab6, 0x11d1, {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsIHTMLCSSStyleSheet : public nsIStyleSheet {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IHTML_CSS_STYLE_SHEET_IID; return iid; }

  NS_IMETHOD Init(nsIURI* aURL, nsIDocument* aDocument) = 0;
  NS_IMETHOD Reset(nsIURI* aURL) = 0;
};

// XXX for convenience and backward compatibility
extern NS_HTML nsresult
  NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult, nsIURI* aURL,
                          nsIDocument* aDocument);

extern NS_HTML nsresult
  NS_NewHTMLCSSStyleSheet(nsIHTMLCSSStyleSheet** aInstancePtrResult);

#endif /* nsIHTMLCSSStyleSheet_h___ */
