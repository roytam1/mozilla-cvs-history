/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#ifndef nsUCvLatinCID_h___
#define nsUCvLatinCID_h___

#include "nsISupports.h"

// Class ID for our Latin1ToUnicode charset converter
// {A3254CB0-8E20-11d2-8A98-00600811A836}
NS_DECLARE_ID(kLatin1ToUnicodeCID,
  0xa3254cb0, 0x8e20, 0x11d2, 0x8a, 0x98, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our ISO88597ToUnicode charset converter
// {AF7A9951-AA48-11d2-B3AE-00805F8A6670}
NS_DECLARE_ID(kISO88597ToUnicodeCID, 
  0xaf7a9951, 0xaa48, 0x11d2, 0xb3, 0xae, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our CP1253ToUnicode charset converter
// {AF7A9952-AA48-11d2-B3AE-00805F8A6670}
NS_DECLARE_ID(kCP1253ToUnicodeCID, 
  0xaf7a9952, 0xaa48, 0x11d2, 0xb3, 0xae, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToLatin1 charset converter
// {920307B0-C6E8-11d2-8AA8-00600811A836}
NS_DECLARE_ID(kUnicodeToLatin1CID, 
  0x920307b0, 0xc6e8, 0x11d2, 0x8a, 0xa8, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

#endif /* nsUCvLatinCID_h___ */
