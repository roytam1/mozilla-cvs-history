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
#ifndef nsIHTMLContentContainer_h___
#define nsIHTMLContentContainer_h___

#include "nsISupports.h"

class nsIHTMLStyleSheet;
class nsIHTMLCSSStyleSheet;
class nsICSSLoader;

/* a6cf90cc-15b3-11d2-932e-00805f8add32 */
#define NS_IHTMLCONTENTCONTAINER_IID \
  {0xa6cf90cc, 0x15b3, 0x11d2,       \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/**
 * Interface implemented by any document class that can container
 * HTML content.
 * XXX Could do with a better name for this interface.
 */
class nsIHTMLContentContainer : public nsISupports {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IHTMLCONTENTCONTAINER_IID; return iid; }
  NS_IMETHOD GetAttributeStyleSheet(nsIHTMLStyleSheet** aStyleSheet) = 0;
  NS_IMETHOD GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aStyleSheet) = 0;
  NS_IMETHOD GetCSSLoader(nsICSSLoader*& aLoader) = 0;
};

#endif // nsIHTMLContentContainer_h___
