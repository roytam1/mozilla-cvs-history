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

#include "nsUnicodeToKSC5601.h"
#include "nsUCvKODll.h"

//----------------------------------------------------------------------
// Global functions and data [declaration]

static PRUint16 g2BytesShiftTable[] =  {
  0, u2BytesCharset,  
  ShiftCell(0,   0, 0, 0, 0, 0, 0, 0),
};

static PRUint16 gComposedHangulShiftTable[] =  {
  0, uComposedHangulCharset,
  ShiftCell(0,   0, 0, 0, 0, 0, 0, 0),
};

static PRUint16 *g_MappingTable[3] = {
  g_ufKSC5601Mapping,
  g_HangulNullMapping
};

static PRUint16 *g_ShiftTable[3] =  {
  g2BytesShiftTable,
  gComposedHangulShiftTable
};

//----------------------------------------------------------------------
// Class nsUnicodeToKSC5601 [implementation]

nsUnicodeToKSC5601::nsUnicodeToKSC5601() 
: nsMultiTableEncoderSupport(2, (uShiftTable**) g_ShiftTable, 
                        (uMappingTable**) g_MappingTable)
{
}

nsresult nsUnicodeToKSC5601::CreateInstance(nsISupports ** aResult) 
{
  nsIUnicodeEncoder *p = new nsUnicodeToKSC5601();
  if(p) {
   *aResult = p;
   return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

//----------------------------------------------------------------------
// Subclassing of nsTableEncoderSupport class [implementation]

NS_IMETHODIMP nsUnicodeToKSC5601::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength * 8;
  return NS_OK;
}
