/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//----------------------------------------------------------------------
// Global functions and data [declaration]
#include "nsUCvMinSupport.h"
#include "nsUnicodeToUTF8.h"
#include <string.h>

static const PRUint16 g_UTF8MappingTable[] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0xFFFF, 0x0000
};

static const PRInt16 g_UTF8ShiftTable[] =  {
  3, uMultibytesCharset, 
  ShiftCell(u1ByteChar,       1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F), 
  ShiftCell(u2BytesUTF8,      2, 0xC0, 0xDF, 0x00, 0x00, 0x07, 0xFF), 
  ShiftCell(u3BytesUTF8,      3, 0xE0, 0xEF, 0x08, 0x00, 0xFF, 0xFF) 
};

//----------------------------------------------------------------------
// Class nsUnicodeToUTF8 [implementation]

nsUnicodeToUTF8::nsUnicodeToUTF8() 
: nsTableEncoderSupport((uShiftTable*) &g_UTF8ShiftTable, 
                        (uMappingTable*) &g_UTF8MappingTable)
{
}

//----------------------------------------------------------------------
// Subclassing of nsTableEncoderSupport class [implementation]

NS_IMETHODIMP nsUnicodeToUTF8::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  // in theory it should be 6, but since we do not handle 
  // UCS4 and UTF-16 here. It is 3. We should change it to 6 when we
  // support UCS4 or UTF-16
  *aDestLength = 3*aSrcLength;
  return NS_OK;
}
NS_IMETHODIMP nsUnicodeToUTF8::FillInfo(PRUint32 *aInfo)
{
  memset(aInfo, 0xFF, (0x10000L >> 3));
  return NS_OK;
}
