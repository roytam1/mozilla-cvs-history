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

#include "nsUCvMinSupport.h"
#include "nsMacRomanToUnicode.h"

NS_IMETHODIMP NS_NewMacRomanToUnicode(nsISupports* aOuter, 
                                            const nsIID& aIID,
                                            void** aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aOuter) {
    *aResult = nsnull;
    return NS_ERROR_NO_AGGREGATION;
  }
  nsMacRomanToUnicode * inst = new nsMacRomanToUnicode();
  if (!inst) {
    *aResult = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult res = inst->QueryInterface(aIID, aResult);
  if (NS_FAILED(res)) {
    *aResult = nsnull;
    delete inst;
  }
  return res;
}

//----------------------------------------------------------------------
// Global functions and data [declaration]

static PRUint16 g_MacRomanMappingTable[] = {
#include "macroman.ut"
};

static PRInt16 g_MacRomanShiftTable[] =  {
  1, u1ByteCharset ,
  ShiftCell(0,0,0,0,0,0,0,0)
};

//----------------------------------------------------------------------
// Class nsMacRomanToUnicode [implementation]

nsMacRomanToUnicode::nsMacRomanToUnicode() 
: nsTableDecoderSupport((uShiftTable*) &g_MacRomanShiftTable, 
                        (uMappingTable*) &g_MacRomanMappingTable)
{
}

//----------------------------------------------------------------------
// Subclassing of nsTableDecoderSupport class [implementation]

NS_IMETHODIMP nsMacRomanToUnicode::GetMaxLength(const char * aSrc, 
                                              PRInt32 aSrcLength, 
                                              PRInt32 * aDestLength)
{
  // we are a single byte to Unicode converter, so...
  *aDestLength = aSrcLength;
  return NS_OK_UDEC_EXACTLENGTH;
}
