/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsCRT_h___
#define nsCRT_h___

#include <stdlib.h>
#include <string.h>
#include "plstr.h"
#include "nscore.h"

/// This is a wrapper class around all the C runtime functions. 

class NS_BASE nsCRT {
public:
  /** Copy bytes from aSrc to aDest.
    @param aDest the destination address
    @param aSrc the source address
    @param aCount the number of bytes to copy
    */
  static void memcpy(void* aDest, const void* aSrc, PRInt32 aCount) {
    ::memcpy(aDest, aSrc, (size_t)aCount);
  }

  static void memmove(void* aDest, const void* aSrc, PRInt32 aCount) {
    ::memmove(aDest, aSrc, (size_t)aCount);
  }

  static void memset(void* aDest, PRUint8 aByte, PRInt32 aCount) {
    ::memset(aDest, aByte, aCount);
  }

  static void zero(void* aDest, PRInt32 aCount) {
    ::memset(aDest, 0, (size_t)aCount);
  }

  /** Compute the string length of s
   @param s the string in question
   @return the length of s
   */
  static PRInt32 strlen(const char* s) {
    return PRInt32(::strlen(s));
  }

  /// Compare s1 and s2.
  static PRInt32 strcmp(const char* s1, const char* s2) {
    return PRInt32(PL_strcmp(s1, s2));
  }

  /// Case-insensitive string comparison.
  static PRInt32 strcasecmp(const char* s1, const char* s2) {
    return PRInt32(PL_strcasecmp(s1, s2));
  }

  /// Like strlen except for ucs2 strings
  static PRInt32 strlen(const PRUnichar* s);

  /// Like strcmp except for ucs2 strings
  static PRInt32 strcmp(const PRUnichar* s1, const PRUnichar* s2);
  /// Like strcmp except for ucs2 strings
  static PRInt32 strncmp(const PRUnichar* s1, const PRUnichar* s2,
                         PRInt32 aMaxLen);

  /// Like strcasecmp except for ucs2 strings
  static PRInt32 strcasecmp(const PRUnichar* s1, const PRUnichar* s2);
  /// Like strncasecmp except for ucs2 strings
  static PRInt32 strncasecmp(const PRUnichar* s1, const PRUnichar* s2,
                             PRInt32 aMaxLen);

  /// Like strcmp with a char* and a ucs2 string
  static PRInt32 strcmp(const PRUnichar* s1, const char* s2);
  /// Like strncmp with a char* and a ucs2 string
  static PRInt32 strncmp(const PRUnichar* s1, const char* s2,
                         PRInt32 aMaxLen);

  /// Like strcasecmp with a char* and a ucs2 string
  static PRInt32 strcasecmp(const PRUnichar* s1, const char* s2);
  /// Like strncasecmp with a char* and a ucs2 string
  static PRInt32 strncasecmp(const PRUnichar* s1, const char* s2,
                             PRInt32 aMaxLen);

  /// Compute a hashcode for a ucs2 string
  static PRInt32 HashCode(const PRUnichar* s1);

  /// Same as above except that we return the length in s1len
  static PRInt32 HashCode(const PRUnichar* s1, PRInt32* s1len);

  /// String to integer.
  static PRInt32 atoi( const PRUnichar *string );
  
  static PRUnichar ToUpper(PRUnichar aChar);

  static PRUnichar ToLower(PRUnichar aChar);
};

#endif /* nsCRT_h___ */
