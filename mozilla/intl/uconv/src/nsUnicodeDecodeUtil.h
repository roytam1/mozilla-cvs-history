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

#ifndef nsUnicodeDecodeUtil_h__
#define nsUnicodeDecodeUtil_h__

#include "unicpriv.h"
#include "nscore.h"
#include "nsIUnicodeDecodeUtil.h"


#define UCONV_ASSERT(a)



class nsUnicodeDecodeUtil : public nsIUnicodeDecodeUtil {
    NS_DECL_ISUPPORTS

public:
    /**
     * 
     */
    nsUnicodeDecodeUtil();

    virtual ~nsUnicodeDecodeUtil();

public:
    NS_IMETHOD Convert(
      PRUnichar       *aDest,
      PRInt32          aDestOffset,
      PRInt32         *aDestLength,
      const char      *aSrc,
      PRInt32          aSrcOffset,
      PRInt32         *aSrcLength,
      PRInt32          aBehavior,
      uShiftTable     *aShiftTable,
      uMappingTable   *aMappingTable
   );

   NS_IMETHOD Convert(
      PRUnichar       *aDest,
      PRInt32          aDestOffset,
      PRInt32         *aDestLength,
      const char      *aSrc,
      PRInt32          aSrcOffset,
      PRInt32         *aSrcLength,
      PRInt32          aBehavior,
      PRUint16         numOfTable,
      uRange          *aRangeArray,
      uShiftTable     **aShiftTableArray,
      uMappingTable   **aMappingTableArray
   );

   NS_IMETHOD Init1ByteFastTable(
      uMappingTable   *aMappingTable,
      PRUnichar       *aFastTable
   );

   NS_IMETHOD Convert(
      PRUnichar       *aDest,
      PRInt32          aDestOffset,
      PRInt32         *aDestLength,
      const char      *aSrc,
      PRInt32          aSrcOffset,
      PRInt32         *aSrcLength,
      const PRUnichar *aFastTable
   );
};

#endif
