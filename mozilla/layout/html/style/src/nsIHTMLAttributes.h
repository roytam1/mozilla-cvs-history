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
#ifndef nsIHTMLAttributes_h___
#define nsIHTMLAttributes_h___

#include "nslayout.h"
#include "nsISupports.h"
#include "nsHTMLValue.h"
#include "nsIContent.h"
class nsIAtom;
class nsISizeOfHandler;
class nsISupportsArray;
class nsIHTMLContent;


// IID for the nsIHTMLAttributes interface {a18f85f0-c058-11d1-8031-006008159b5a}
#define NS_IHTML_ATTRIBUTES_IID     \
{0xa18f85f0, 0xc058, 0x11d1,        \
  {0x80, 0x31, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

class nsIHTMLAttributes : public nsISupports {
public:
  // XXX why is this part of the interface?
  NS_IMETHOD SetAttribute(nsIAtom* aAttribute, const nsString& aValue,
                          PRInt32& aCount) = 0;

  NS_IMETHOD SetAttribute(nsIAtom* aAttribute, 
                               const nsHTMLValue& aValue,
                               PRInt32& aCount) = 0;

  NS_IMETHOD UnsetAttribute(nsIAtom* aAttribute, PRInt32& aCount) = 0;

  NS_IMETHOD GetAttribute(nsIAtom* aAttribute,
                          nsHTMLValue& aValue) const = 0;

  NS_IMETHOD GetAllAttributeNames(nsISupportsArray* aArray,
                                  PRInt32& aCount) const = 0;

  NS_IMETHOD Count(PRInt32& aCount) const = 0;

  NS_IMETHOD SetID(nsIAtom* aID, PRInt32& aIndex) = 0;

  NS_IMETHOD GetID(nsIAtom*& aResult) const = 0;

  NS_IMETHOD SetClass(nsIAtom* aClass, PRInt32& aIndex) = 0;  // XXX this will have to change for CSS2

  NS_IMETHOD GetClass(nsIAtom*& aResult) const = 0;  // XXX this will have to change for CSS2

  /**
   * Add this object's size information to the sizeof handler.
   */
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const = 0;

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
};

extern NS_HTML nsresult
  NS_NewHTMLAttributes(nsIHTMLAttributes** aInstancePtrResult, nsIHTMLContent* aContent);

#endif /* nsIHTMLAttributes_h___ */

