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

#ifndef nsUCVJA2CID_h___
#define nsUCVJA2CID_h___

#include "nsISupports.h"

// Class ID for our EUCJPToUnicode charset converter
// {3F6FE6A1-AC0A-11d2-B3AE-00805F8A6670}
NS_DECLARE_ID(kEUCJPToUnicodeCID, 
0x3f6fe6a1, 0xac0a, 0x11d2, 0xb3, 0xae, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our ISO2022JPToUnicode charset converter
// {3F6FE6A2-AC0A-11d2-B3AE-00805F8A6670}
NS_DECLARE_ID(kISO2022JPToUnicodeCID, 
0x3f6fe6a2, 0xac0a, 0x11d2, 0xb3, 0xae, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToEUCJP charset converter
// {45C23A20-D71C-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kUnicodeToEUCJPCID, 
  0x45c23a20, 0xd71c, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our UnicodeToISO2022JP charset converter
// {4F76E100-D71C-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kUnicodeToISO2022JPCID, 
  0x4f76e100, 0xd71c, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our UnicodeToJISx0201 charset converter
// {BA615191-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToJISx0201CID, 
  0xba615191, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToJISx0208 charset converter
// {BA615192-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToJISx0208CID, 
  0xba615192, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToJISx0212 charset converter
// {BA615193-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToJISx0212CID, 
  0xba615193, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);


// {3AEE9A81-4EAF-11d3-B3C3-00805F8A6670}
NS_DECLARE_ID(kObsoletedShiftJISToUnicodeCID, 
0x3aee9a81, 0x4eaf, 0x11d3, 0xb3, 0xc3, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// {3AEE9A82-4EAF-11d3-B3C3-00805F8A6670}
NS_DECLARE_ID(kObsoletedEUCJPToUnicodeCID, 
0x3aee9a82, 0x4eaf, 0x11d3, 0xb3, 0xc3, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// {3AEE9A83-4EAF-11d3-B3C3-00805F8A6670}
NS_DECLARE_ID(kObsoletedISO2022JPToUnicodeCID, 
0x3aee9a83, 0x4eaf, 0x11d3, 0xb3, 0xc3, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

#endif /* nsUCVJA2CID_h___ */
