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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsPIBoxObject_h___
#define nsPIBoxObject_h___

// {9580E69B-8FD6-414e-80CD-3A1821017646}
#define NS_PIBOXOBJECT_IID \
{ 0x9580e69b, 0x8fd6, 0x414e, { 0x80, 0xcd, 0x3a, 0x18, 0x21, 0x1, 0x76, 0x46 } }

class nsIPresShell;
class nsIContent;
class nsIDocument;

class nsPIBoxObject : public nsISupports {

public:
  static const nsIID& GetIID() { static nsIID iid = NS_PIBOXOBJECT_IID; return iid; }

  NS_IMETHOD Init(nsIContent* aContent, nsIPresShell* aShell) = 0;
  NS_IMETHOD SetDocument(nsIDocument* aDocument) = 0;
};

#endif

