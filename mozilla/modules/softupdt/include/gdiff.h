/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/*--------------------------------------------------------------
 * GDIFF.H
 *
 * Constants used in processing the GDIFF format
 *--------------------------------------------------------------*/


#define GDIFF_MAGIC         "\xD1\xFF\xD1\xFF"
#define GDIFF_MAGIC_LEN     4
#define GDIFF_VER           5
#define GDIFF_EOF           "\0"

#define GDIFF_VER_POS       4
#define GDIFF_CS_POS        5
#define GDIFF_CSLEN_POS     6

#define GDIFF_HEADERSIZE    7
#define GDIFF_APPDATALEN    4

#define GDIFF_CS_NONE       0
#define GDIFF_CS_MD5        1
#define GDIFF_CS_SHA        2
#define GDIFF_CS_CRC32      32

#define CRC32_LEN           4

/*--------------------------------------
 *  GDIFF opcodes
 *------------------------------------*/
#define ENDDIFF     0
#define ADD8MAX     246
#define ADD16       247
#define ADD32       248
#define COPY16BYTE  249
#define COPY16SHORT 250
#define COPY16LONG  251
#define COPY32BYTE  252
#define COPY32SHORT 253
#define COPY32LONG  254
#define COPY64      255

/* instruction sizes */
#define ADD16SIZE           2
#define ADD32SIZE           4
#define COPY16BYTESIZE      3
#define COPY16SHORTSIZE     4
#define COPY16LONGSIZE      6
#define COPY32BYTESIZE      5
#define COPY32SHORTSIZE     6
#define COPY32LONGSIZE      8
#define COPY64SIZE          12


/*--------------------------------------
 *  error codes
 *------------------------------------*/
#define GDIFF_OK                0
#define GDIFF_ERR_UNKNOWN       -1
#define GDIFF_ERR_ARGS          -2
#define GDIFF_ERR_ACCESS        -3
#define GDIFF_ERR_MEM           -4
#define GDIFF_ERR_HEADER        -5
#define GDIFF_ERR_BADDIFF       -6
#define GDIFF_ERR_OPCODE        -7
#define GDIFF_ERR_OLDFILE       -8
#define GDIFF_ERR_CHKSUMTYPE    -9
#define GDIFF_ERR_CHECKSUM      -10


/*--------------------------------------
 *  types
 *------------------------------------*/
#ifndef AIX
#ifdef OSF1
#include <sys/types.h>
#else
typedef unsigned char uchar;
#endif
#endif

typedef struct _diffdata {
    XP_File     fSrc;
    XP_File     fOut;
    XP_File     fDiff;
    uint8       checksumType;
    uint8       checksumLength;
    uchar*      oldChecksum;
    uchar*      newChecksum;
    XP_Bool     bMacAppleSingle;
    XP_Bool     bWin32BoundImage;
    uchar*      databuf;
    uint32      bufsize;
} DIFFDATA;

typedef DIFFDATA* pDIFFDATA;


/*--------------------------------------
 *  miscellaneous
 *------------------------------------*/

#define APPFLAG_W32BOUND        "autoinstall:Win32PE"
#define APPFLAG_APPLESINGLE     "autoinstall:AppleSingle"

#ifndef TRUE
  #define TRUE  1
#endif

#ifndef FALSE
  #define FALSE 0
#endif


