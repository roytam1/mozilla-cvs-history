
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
#ifndef nsICollation_h__
#define nsICollation_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsILocale.h"

// {D4CF2F80-A98B-11d2-9119-006008A6EDF6}
#define NS_ICOLLATIONFACTORY_IID \
{ 0xd4cf2f80, 0xa98b, 0x11d2, \
{ 0x91, 0x19, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }

// {CDBFD3F0-A4FE-11d2-9119-006008A6EDF6}
#define NS_ICOLLATION_IID \
{ 0xcdbfd3f0, 0xa4fe, 0x11d2, \
{ 0x91, 0x19, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }

typedef enum {
  kCollationStrengthDefault = 0,      // use the primary comparison for the given local - no flags) 
  kCollationCaseInsensitiveAscii = 1, // do not consider case differences when doing the comparison i.e. A=a) 
  kCollationAccentInsenstive = 2,     // do not consider accent differences when doing the comparison a=�) 
  kCollationCaseSensitive = kCollationStrengthDefault,
  kCollationCaseInSensitive = (kCollationCaseInsensitiveAscii | kCollationAccentInsenstive)
} nsCollationStrength;


class nsICollation;

// Create a collation interface for an input locale.
// 
class nsICollationFactory : public nsISupports {

public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICOLLATIONFACTORY_IID)

  NS_IMETHOD CreateCollation(nsILocale* locale, nsICollation** instancePtr) = 0;
};

// Locale sensitive collation interface
// 
class nsICollation : public nsISupports {

public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICOLLATION_IID)

  // compare two strings
  // result is same as strcmp
  NS_IMETHOD CompareString(const nsCollationStrength strength, 
                           const nsAString& string1, const nsAString& string2, PRInt32* result) = 0;

  // allocate sort key from input string
  // returns newly allocated key and its byte length
  NS_IMETHOD AllocateRawSortKey(const nsCollationStrength strength, 
                                const nsAString& stringIn, PRUint8** key, PRUint32 *outLen) = 0;

  // compare two sort keys
  // length is a byte length, result is same as strcmp
  NS_IMETHOD CompareRawSortKey(const PRUint8* key1, const PRUint32 len1, 
                               const PRUint8* key2, const PRUint32 len2, 
                               PRInt32* result) = 0;

  // init this interface to a specified locale (should only be called by collation factory)
  //
  NS_IMETHOD Initialize(nsILocale* locale) = 0;
};

#endif  /* nsICollation_h__ */
