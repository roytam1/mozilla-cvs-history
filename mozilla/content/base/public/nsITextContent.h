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
#ifndef nsITextContent_h___
#define nsITextContent_h___

#include "nslayout.h"
class nsString;

// IID for the nsITextContent interface
#define NS_ITEXT_CONTENT_IID \
 {0xa6cf9065, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/**
 * Interface for textual content. This interface is used to provide
 * an efficient access to text content.
 */
class nsITextContent : public nsISupports {
public:
  /*
   * Get direct access to the text in the text content.
   */
  NS_IMETHOD GetText(const PRUnichar*& aBaseResult,
                     PRInt32& aLengthResult) = 0;
  /*
   * Set the text to the given value. If aNotify is PR_TRUE then
   * the document is notified of the content change.
   */
  NS_IMETHOD SetText(const PRUnichar* aBuffer,
                     PRInt32 aLength,
                     PRBool aNotify) = 0;
};

#endif /* nsITextContent_h___ */
