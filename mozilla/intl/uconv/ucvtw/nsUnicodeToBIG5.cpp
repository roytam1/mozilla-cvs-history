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

#include "nsUnicodeToBIG5.h"
#include "nsUCvTWDll.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]


static PRUint16 gAsciiShiftTable[] =  {
  0, u1ByteCharset,
  ShiftCell(0,   0, 0, 0, 0, 0, 0, 0),
};

static PRUint16 gBig5ShiftTable[] =  {
  0, u2BytesCharset,
  ShiftCell(0,   0, 0, 0, 0, 0, 0, 0),
};
 

static PRUint16 *g_Big5MappingTable[2] = {
  g_ASCIIMapping,
  g_ufBig5Mapping
};

static PRUint16 *g_Big5ShiftTable[2] =  {
  gAsciiShiftTable,
  gBig5ShiftTable
};

//----------------------------------------------------------------------
// Class nsUnicodeToBIG5 [implementation]

nsUnicodeToBIG5::nsUnicodeToBIG5() 
: nsMultiTableEncoderSupport(2,
                        (uShiftTable**) &g_Big5ShiftTable, 
                        (uMappingTable**) &g_Big5MappingTable)
{
}

nsresult nsUnicodeToBIG5::CreateInstance(nsISupports ** aResult) 
{
  nsIUnicodeEncoder *p = new nsUnicodeToBIG5();
  if(p) {
   *aResult = p;
   return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

//----------------------------------------------------------------------
// Subclassing of nsTableEncoderSupport class [implementation]

NS_IMETHODIMP nsUnicodeToBIG5::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  *aDestLength = 2 * aSrcLength;
  return NS_OK;
}
