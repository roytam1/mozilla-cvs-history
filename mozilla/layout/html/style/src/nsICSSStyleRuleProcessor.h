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
#ifndef nsICSSStyleRuleProcessor_h___
#define nsICSSStyleRuleProcessor_h___

#include "nslayout.h"
#include "nsIStyleRuleProcessor.h"

class nsICSSStyleSheet;

// IID for the nsICSSStyleRuleProcessor interface {98bf169c-7b7c-11d3-ba05-001083023c2b}
#define NS_ICSS_STYLE_RULE_PROCESSOR_IID     \
{0x98bf169c, 0x7b7c, 0x11d3, {0xba, 0x05, 0x00, 0x10, 0x83, 0x02, 0x3c, 0x2b}}

/* The CSS style rule processor provides a mechanism for sibling style sheets
 * to combine their rule processing in order to allow proper cascading to happen.
 *
 * When queried for a rule processor, a CSS style sheet will append itself to 
 * the previous CSS processor if present, and return nsnull. Otherwise it will
 * create a new processor for itself.
 *
 * CSS style rule processors keep a live reference on all style sheets bound to them
 * The CSS style sheets keep a weak reference on all the processors that they are
 * bound to (many to many). The CSS style sheet is told when the rule processor is 
 * going away (via DropRuleProcessorReference).
 */
class nsICSSStyleRuleProcessor: public nsIStyleRuleProcessor {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ICSS_STYLE_RULE_PROCESSOR_IID; return iid; }

  NS_IMETHOD  AppendStyleSheet(nsICSSStyleSheet* aStyleSheet) = 0;
};

#endif /* nsICSSStyleRuleProcessor_h___ */
