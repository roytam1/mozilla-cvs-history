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
#ifndef nsIStyleSheetLinkingElement_h__
#define nsIStyleSheetLinkingElement_h__


#include "nsISupports.h"

class nsIParser;
class nsIDocument;

#define NS_ISTYLESHEETLINKINGELEMENT_IID          \
  {0xa6cf90e9, 0x15b3, 0x11d2,                    \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsIStyleSheet;

class nsIStyleSheetLinkingElement : public nsISupports {
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISTYLESHEETLINKINGELEMENT_IID)

  /**
   * Used to make the association between a style sheet and
   * the element that linked it to the document.
   *
   * @param aStyleSheet the style sheet associated with this
   *                    element.
   */
  NS_IMETHOD SetStyleSheet(nsIStyleSheet* aStyleSheet) = 0;

  /**
   * Used to obtain the style sheet linked in by this element.
   *
   * @param aStyleSheet out parameter that returns the style
   *                    sheet associated with this element.
   */
  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aStyleSheet) = 0;

  /**
   * Initialize the stylesheet linking element. This method passes
   * in a parser that the element blocks if the stylesheet is
   * a stylesheet that should be loaded with the parser blocked.
   * If aDontLoadStyle is true the element will ignore the first
   * modification to the element that would cause a stylesheet to
   * be loaded. Subsequent modifications to the element will not
   * be ignored.
   */
  NS_IMETHOD InitStyleLinkElement(nsIParser *aParser, PRBool aDontLoadStyle) = 0;

  /**
   * Tells this element to update the stylesheet.
   *
   * @param aNotify .
   * @param aOldDocument .
   * @param aDocIndex .
   */
  NS_IMETHOD UpdateStyleSheet(PRBool aNotify,
                              nsIDocument *aOldDocument,
                              PRInt32 aDocIndex) = 0;

  /**
   * Tells this element wether to update the stylesheet when the
   * element's properties change.
   *
   * @param aEnableUpdates update on changes or not.
   */
  NS_IMETHOD SetEnableUpdates(PRBool aEnableUpdates) = 0;
};

#endif // nsILinkingElement_h__
