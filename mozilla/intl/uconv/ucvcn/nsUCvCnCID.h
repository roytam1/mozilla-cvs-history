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

#ifndef nsUCvCnCID_h___
#define nsUCvCnCID_h___

#include "nsISupports.h"

// Class ID for our GB2312ToUnicode charset converter
// {379C2774-EC77-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kGB2312ToUnicodeCID,
  0x379c2774, 0xec77, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our ISO2022CNToUnicode charset converter
// {BA615199-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kISO2022CNToUnicodeCID,
0xba615199, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);
 
// Class ID for our HZToUnicode charset converter
// {BA61519A-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kHZToUnicodeCID,
0xba61519a, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our GBKToUnicode charset converter
// {BA61519E-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kGBKToUnicodeCID,
0xba61519e, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);


// Class ID for our UnicodeToGB2312 charset converter
// {379C2777-EC77-11d2-8AAC-00600811A836}
NS_DECLARE_ID(kUnicodeToGB2312CID, 
  0x379c2777, 0xec77, 0x11d2, 0x8a, 0xac, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

// Class ID for our UnicodeToGB2312GL charset converter
// {BA615196-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToGB2312GLCID, 
0xba615196, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToGBK charset converter
// {BA61519B-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToGBKCID, 
0xba61519b, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToISO2022CN charset converter
// {BA61519C-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToISO2022CNCID, 
0xba61519c, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our UnicodeToHZ charset converter
// {BA61519D-1DFA-11d3-B3BF-00805F8A6670}
NS_DECLARE_ID(kUnicodeToHZCID, 
0xba61519d, 0x1dfa, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);

// Class ID for our CP936ToUnicode charset converter
NS_DECLARE_ID(kCP936ToUnicodeCID,
// {9416BFC0-1F93-11d3-B3BF-00805F8A6670}
0x9416bfc0, 0x1f93, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);


// Class ID for our UnicodeToCP936 charset converter
NS_DECLARE_ID(kUnicodeToCP936CID, 
// {9416BFC1-1F93-11d3-B3BF-00805F8A6670}
0x9416bfc1, 0x1f93, 0x11d3, 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70);


#endif /* nsUCvCnCID_h___ */
