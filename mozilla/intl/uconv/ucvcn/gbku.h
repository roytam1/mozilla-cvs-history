/*
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
 * The Initial Developer of the Original Code is Intel
 * Corporation.  Portions created by Intel are
 * Copyright (C) 1999 Intel Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): Yueheng Xu <yueheng.xu@intel.com>
 *                 Hoa Nguyen <hoa.nguyen@intel.com>
 */ 
// =======================================================================
// Original Author: Yueheng Xu
// email: yueheng.xu@intel.com
// phone: (503)264-2248
// Intel Corporation, Oregon, USA
// Last Update: September 7, 1999
// Revision History: 
// 09/07/1999 - initial version.
// 09/28/1999 - changed leftbyte and rightbyte from char to unsigned char 
//              in struct DByte
// 04/10/1999 - changed leftbyte. rightbyte to PRUint8 in struct DByte;
//              added table UnicodeToGBKTable[0x5200]
//            
// ======================================================================================
// Table GBKToUnicode[] maps the GBK code to its unicode.
// The mapping data of this GBK table is obtained from 
// ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP936.TXT
// Frank Tang of Netscape wrote the original perl tool to re-align the 
// mapping data into an 8-item per line format ( i.e. file cp936map.txt ).
//
// The valid GBK charset range: left byte is [0x81, 0xfe], right byte are
// [0x40, 0x7e] and [0x80, 0xfe]. But for the convenience of index 
// calculation, the table here has a single consecutive range of 
// [0x40, 0xfe] for the right byte. Those invalid chars whose right byte 
// is 0x7f will be mapped to undefined unicode 0xFFFF.
//
// 
// Table UnicodeToGBK[] maps the unicode to GBK code. To reduce memory usage, we
// only do Unicode to GBK table mapping for unicode between 0x4E00 and 0xA000; 
// Others let converter to do search from table GBKToUnicode[]. If we want further
// trade memory for performance, we can let more unicode to do table mapping to get
// its GBK instead of searching table GBKToUnicode[]. 

#ifdef _GBKU_TABLE_

#define GB_UNDEFINED 0xFFFF
#define MAX_GBK_LENGTH	  24066   /* (0xfe-0x80)*(0xfe-0x3f) */

typedef struct
{

  PRUint8 leftbyte;
  PRUint8 rightbyte;

} DByte;
extern PRUnichar GBKToUnicodeTable[MAX_GBK_LENGTH];

#ifdef _UNICODE_TO_GBK_ENCODER_
DByte UnicodeToGBKTable[0x5200];
#else
extern DByte UnicodeToGBKTable[0x5200]; // 0xA000 - 0x4E00 = 0x5200
#endif

#else

#define _GBKU_TABLE_


#define GB_UNDEFINED 0xFFFF
#define MAX_GBK_LENGTH	  24066   /* (0xfe-0x80)*(0xfe-0x3f) */

typedef struct 
{ 
  PRUint8 leftbyte;
  PRUint8 rightbyte;

} DByte;



PRUnichar GBKToUnicodeTable[MAX_GBK_LENGTH] =
{
#include "cp936map.h"
};


#ifdef _UNICODE_TO_GBK_ENCODER_
DByte UnicodeToGBKTable[0x5200];
#else
extern DByte UnicodeToGBKTable[0x5200]; // 0xA000 - 0x4E00 = 0x5200
#endif


#endif /* ifdef _GBKU_TABLE_ */

