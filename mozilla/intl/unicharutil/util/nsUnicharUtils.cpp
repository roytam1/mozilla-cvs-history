/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *  Alec Flett <alecf@netscape.com>
 */

#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtilCIID.h"
#include "nsICaseConversion.h"
#include "nsIServiceManager.h"

// global cache of the case conversion service
static nsICaseConversion *gCaseConv = nsnull;
static NS_DEFINE_CID(kUnicharUtilCID, NS_UNICHARUTIL_CID);

static nsresult NS_InitCaseConversion() {
    if (gCaseConv) return NS_OK;

    return nsServiceManager::GetService(kUnicharUtilCID,
                                        NS_GET_IID(nsICaseConversion),
                                        (nsISupports**)&gCaseConv);

}

class ConvertToLowerCase
{
public:
    typedef PRUnichar value_type;
    
    PRUint32 write( const PRUnichar* aSource, PRUint32 aSourceLength)
    {
        NS_InitCaseConversion();
        gCaseConv->ToLower(aSource, NS_CONST_CAST(PRUnichar*,aSource), aSourceLength);
        return aSourceLength;
    }
};

void
ToLowerCase( nsAString& aString)
  {
    nsAString::iterator fromBegin, fromEnd;
    ConvertToLowerCase converter;
    copy_string(aString.BeginWriting(fromBegin), aString.EndWriting(fromEnd), converter);
  }

class ConvertToUpperCase
{
public:
    typedef PRUnichar value_type;
    
    PRUint32 write( const PRUnichar* aSource, PRUint32 aSourceLength)
    {
        NS_InitCaseConversion();
        gCaseConv->ToUpper(aSource, NS_CONST_CAST(PRUnichar*,aSource), aSourceLength);
        return 0;
    }
};

void
ToUpperCase( nsAString& aString )
  {
    nsAString::iterator fromBegin, fromEnd;
    ConvertToUpperCase converter;
    copy_string(aString.BeginWriting(fromBegin), aString.EndWriting(fromEnd), converter);
  }


class CaseInsensitivePRUnicharComparator
  {
    public:
      PRBool operator()( PRUnichar lhs, PRUnichar rhs ) const {
          NS_InitCaseConversion();
          
          PRUnichar lhsUpper; PRUnichar rhsUpper;
          gCaseConv->ToUpper(lhs, &lhsUpper);
          gCaseConv->ToUpper(rhs, &rhsUpper);
          return lhsUpper = rhsUpper;
      }
  };

PRBool
CaseInsensitiveFindInReadable( const nsAString& aPattern, nsAString::const_iterator& aSearchStart, nsAString::const_iterator& aSearchEnd )
  {
      nsAutoString lowerPattern(aPattern);
      ToLowerCase(lowerPattern);

      nsAutoString lowerString;
      CopyUnicodeTo(aSearchStart, aSearchEnd, lowerString);

      nsAString::const_iterator match_start, match_end;
      return FindInReadable(lowerPattern,
                            lowerString.BeginReading(match_start),
                            lowerString.EndReading(match_end));
  }

int
nsCaseInsensitiveStringComparator::operator()( const PRUnichar* lhs, const PRUnichar* rhs, PRUint32 aLength ) const
  {
    return 0; //nsCRT::strncasecmp(lhs, rhs, aLength);
  }

