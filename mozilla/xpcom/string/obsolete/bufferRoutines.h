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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

/* bufferRoutines.h --- rickg's original string manipulation underpinnings;
    this code will be made obsolete by the new shared-buffer string (see bug #53065)
 */

#ifndef _BUFFERROUTINES_H
#define _BUFFERROUTINES_H

#ifndef nsStringDefines_h___
#include "nsStringDefines.h"
#endif


/******************************************************************************************
  MODULE NOTES:

  This file contains the workhorse copy and shift functions used in nsStrStruct.
  Ultimately, I plan to make the function pointers in this system available for 
  use by external modules. They'll be able to install their own "handlers".
  Not so, today though.

*******************************************************************************************/

#include "nsCRT.h"

#ifndef XPCOM_STANDALONE
#if !defined(RICKG_TESTBED) && !defined(STANDALONE_STRING_TESTS)
#include "nsUnicharUtilCIID.h"
#include "nsIServiceManager.h"
#include "nsICaseConversion.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#endif
#endif /* XPCOM_STANDALONE */

#define KSHIFTLEFT  (0)
#define KSHIFTRIGHT (1)

// uncomment the following line to caught nsString char* casting problem
//#define DEBUG_ILLEGAL_CAST_UP
//#define DEBUG_ILLEGAL_CAST_DOWN

#if  defined(DEBUG_ILLEGAL_CAST_UP) || defined(DEBUG_ILLEGAL_CAST_DOWN) 

static PRBool track_illegal = PR_TRUE;
static PRBool track_latin1  = PR_TRUE;

#ifdef XP_UNIX
#include "nsTraceRefcnt.h"
class CTraceFile {
public:
   CTraceFile() {
      mFile = fopen("nsStringTrace.txt" , "a+");
   }
   ~CTraceFile() {
      fflush(mFile);
      fclose(mFile);
   }
   void ReportCastUp(const char* data, const char* msg)
   {
      if(mFile) {
        fprintf(mFile, "ERRORTEXT= %s\n", msg);
        fprintf(mFile, "BEGINDATA\n");
        const char* s=data;
        while(*s) {
           if(*s & 0x80) {
             fprintf(mFile, "[%2X]", (char)*s);
	   } else {
             fprintf(mFile, "%c", *s);
           }
           s++;
        }
        fprintf(mFile, "\n");
        fprintf(mFile, "ENDDATA\n");
        fprintf(mFile, "BEGINSTACK\n");
        nsTraceRefcnt::WalkTheStack(mFile);
        fprintf(mFile, "\n");
        fprintf(mFile, "ENDSTACK\n");
        fflush(mFile);
      }
   }
   void ReportCastDown(const PRUnichar* data, const char* msg)
   {
      if(mFile) {
        fprintf(mFile, "ERRORTEXT=%s\n", msg);
        fprintf(mFile, "BEGINDATA\n");
        const PRUnichar* s=data;
        while(*s) {
           if(*s & 0xFF80) {
             fprintf(mFile, "\\u%X", *s);
	   } else {
             fprintf(mFile, "%c", *s);
           }
           s++;
        }
        fprintf(mFile, "\n");
        fprintf(mFile, "ENDDATA\n");
        fprintf(mFile, "BEGINSTACK\n");
        nsTraceRefcnt::WalkTheStack(mFile);
        fprintf(mFile, "\n");
        fprintf(mFile, "ENDSTACK\n");
        fflush(mFile);
      }
   }
private:
   FILE* mFile;
};
static CTraceFile gTrace;
#define  TRACE_ILLEGAL_CAST_UP(c, s, m)     if(!(c)) gTrace.ReportCastUp(s,m);
#define  TRACE_ILLEGAL_CAST_DOWN(c, s, m)   if(!(c)) gTrace.ReportCastDown(s,m);

#else // XP_UNIX

#define  TRACE_ILLEGAL_CAST_UP(c, s, m)   NS_ASSERTION((c), (m))
#define  TRACE_ILLEGAL_CAST_DOWN(c, s, m) NS_ASSERTION((c), (m))

#endif //XP_UNIX

#endif

#ifndef XPCOM_STANDALONE

// XXX bug: this doesn't map 0x80 to 0x9f properly
static const PRUnichar kIsoLatin1ToUCS2_2[256] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
   48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
   64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
   96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
  112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

#if !defined(RICKG_TESTBED) && !defined(STANDALONE_STRING_TESTS)
class HandleCaseConversionShutdown : public nsIObserver {
public :
   HandleCaseConversionShutdown(void) { NS_INIT_REFCNT(); }
   virtual ~HandleCaseConversionShutdown(void) {}
   NS_DECL_ISUPPORTS
   NS_DECL_NSIOBSERVER
};

static NS_DEFINE_CID(kUnicharUtilCID, NS_UNICHARUTIL_CID);
static nsICaseConversion * gCaseConv = 0; 

NS_IMPL_ISUPPORTS1(HandleCaseConversionShutdown, nsIObserver);

NS_IMETHODIMP
HandleCaseConversionShutdown::Observe(nsISupports *aSubject,
                                      const char* aTopic,
                                      const PRUnichar *aData)
{
    NS_IF_RELEASE(gCaseConv);
    return NS_OK;
}

static nsresult
NS_InitCaseConversion()
{
  if (gCaseConv) return NS_OK;
  
  nsresult rv;
    
  rv = nsServiceManager::GetService(kUnicharUtilCID,
                                    NS_GET_IID(nsICaseConversion),
                                    (nsISupports**) &gCaseConv);

  // try to release it on shutdown
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv)) {

      HandleCaseConversionShutdown* observer =
        new HandleCaseConversionShutdown();

      if (observer)
        obs->AddObserver(observer, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    }
  }
  return NS_OK;
}

#endif
#endif /* XPCOM_STANDALONE */

inline PRUnichar GetUnicharAt(const char* aString,PRUint32 anIndex) {
  return ((PRUnichar*)aString)[anIndex];
}

inline PRUnichar GetCharAt(const char* aString,PRUint32 anIndex) {
  return (PRUnichar)aString[anIndex];
}

//----------------------------------------------------------------------------------------
//
//  This set of methods is used to shift the contents of a char buffer.
//  The functions are differentiated by shift direction and the underlying charsize.
//

/**
 * This method shifts single byte characters left by a given amount from an given offset.
 * @update	gess 01/04/99
 * @param   aDest is a ptr to a cstring where left-shift is to be performed
 * @param   aLength is the known length of aDest
 * @param   anOffset is the index into aDest where shifting shall begin
 * @param   aCount is the number of chars to be "cut"
 */
void ShiftCharsLeft(char* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount);
void ShiftCharsLeft(char* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount) { 
  char*   dst = aDest+anOffset;
  char*   src = aDest+anOffset+aCount;

  memmove(dst,src,aLength-(aCount+anOffset));
}

/**
 * This method shifts single byte characters right by a given amount from an given offset.
 * @update	gess 01/04/99
 * @param   aDest is a ptr to a cstring where the shift is to be performed
 * @param   aLength is the known length of aDest
 * @param   anOffset is the index into aDest where shifting shall begin
 * @param   aCount is the number of chars to be "inserted"
 */
void ShiftCharsRight(char* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount);
void ShiftCharsRight(char* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount) { 
  char* src = aDest+anOffset;
  char* dst = aDest+anOffset+aCount;

  memmove(dst,src,aLength-anOffset);
}

/**
 * This method shifts unicode characters by a given amount from an given offset.
 * @update	gess 01/04/99
 * @param   aDest is a ptr to a cstring where the shift is to be performed
 * @param   aLength is the known length of aDest
 * @param   anOffset is the index into aDest where shifting shall begin
 * @param   aCount is the number of chars to be "cut"
 */
void ShiftDoubleCharsLeft(PRUnichar* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount);
void ShiftDoubleCharsLeft(PRUnichar* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount) { 
  PRUnichar* root= aDest;
  PRUnichar* dst = root+anOffset;
  PRUnichar* src = root+anOffset+aCount;

  memmove(dst,src,(aLength-(aCount+anOffset))*sizeof(PRUnichar));
}


/**
 * This method shifts unicode characters by a given amount from an given offset.
 * @update	gess 01/04/99
 * @param   aDest is a ptr to a cstring where the shift is to be performed
 * @param   aLength is the known length of aDest
 * @param   anOffset is the index into aDest where shifting shall begin
 * @param   aCount is the number of chars to be "inserted"
 */
void ShiftDoubleCharsRight(PRUnichar* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount);
void ShiftDoubleCharsRight(PRUnichar* aDest,PRUint32 aLength,PRUint32 anOffset,PRUint32 aCount) { 
  PRUnichar* root= aDest;
  PRUnichar* src = root+anOffset;
  PRUnichar* dst = root+anOffset+aCount;

  memmove(dst,src,sizeof(PRUnichar)*(aLength-anOffset));
}

//----------------------------------------------------------------------------------------
//
//  This set of methods is used to copy one buffer onto another.
//  The functions are differentiated by the size of source and dest character sizes.
//  WARNING: Your destination buffer MUST be big enough to hold all the source bytes.
//           We don't validate these ranges here (this should be done in higher level routines).
//


/**
 * Going 1 to 1 is easy, since we assume ascii. No conversions are necessary.
 * @update	gess 01/04/99
 * @param aDest is the destination buffer
 * @param aDestOffset is the pos to start copy to in the dest buffer
 * @param aSource is the source buffer
 * @param anOffset is the offset to start copying from in the source buffer
 * @param aCount is the (max) number of chars to copy
 */
void CopyChars1To1(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount);
void CopyChars1To1(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount) { 

  char* dst = aDest+anDestOffset;
  char* src = (char*)aSource+anOffset;

  memcpy(dst,src,aCount);
} 

/**
 * Going 1 to 2 requires a conversion from ascii to unicode. This can be expensive.
 * @param aDest is the destination buffer
 * @param aDestOffset is the pos to start copy to in the dest buffer
 * @param aSource is the source buffer
 * @param anOffset is the offset to start copying from in the source buffer
 * @param aCount is the (max) number of chars to copy
 */
void CopyChars1To2(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount);
void CopyChars1To2(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount) { 

  PRUnichar* theDest=(PRUnichar*)aDest;
  PRUnichar* to   = theDest+anDestOffset;
  const unsigned char* first= (const unsigned char*)aSource+anOffset;
  const unsigned char* last = first+aCount;

#ifdef DEBUG_ILLEGAL_CAST_UP
  PRBool illegal= PR_FALSE;
#endif
  //now loop over characters, shifting them left...
  while(first<last) {
    *to=(PRUnichar)(*first);  
#ifdef DEBUG_ILLEGAL_CAST_UP
    if(track_illegal && track_latin1 && ((*to)& 0x80))
      illegal= PR_TRUE;
#endif
    to++;
    first++;
  }
#ifdef DEBUG_ILLEGAL_CAST_UP
  TRACE_ILLEGAL_CAST_UP((!illegal), aSource, "illegal cast up in CopyChars1To2");
#endif

}
 

/**
 * Going 2 to 1 requires a conversion from unicode down to ascii. This can be lossy.
 * @update	gess 01/04/99
 * @param aDest is the destination buffer
 * @param aDestOffset is the pos to start copy to in the dest buffer
 * @param aSource is the source buffer
 * @param anOffset is the offset to start copying from in the source buffer
 * @param aCount is the (max) number of chars to copy
 */
void CopyChars2To1(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount);
void CopyChars2To1(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount) { 
  char*             to   = aDest+anDestOffset;
  PRUnichar*        theSource=(PRUnichar*)aSource;
  const PRUnichar*  first= theSource+anOffset;
  const PRUnichar*  last = first+aCount;

#ifdef DEBUG_ILLEGAL_CAST_DOWN
  PRBool illegal= PR_FALSE;
#endif
  //now loop over characters, shifting them left...
  while(first<last) {
    if(*first<256)
      *to=(char)*first;  
    else { 
      *to='.';
      NS_ASSERTION( (*first < 256), "data in U+0100-U+FFFF will be lost");
    }
#ifdef DEBUG_ILLEGAL_CAST_DOWN
    if(track_illegal) {
      if(track_latin1) {
        if(*first & 0xFF80)
          illegal = PR_TRUE;
      } else {
        if(*first & 0xFF00)
          illegal = PR_TRUE;
      } // track_latin1
    } // track_illegal
#endif
    to++;
    first++;
  }
#ifdef DEBUG_ILLEGAL_CAST_DOWN
  TRACE_ILLEGAL_CAST_DOWN((!illegal), theSource, "illegal cast down in CopyChars2To1");
#endif
}

/**
 * Going 2 to 2 is fast and efficient.
 * @update	gess 01/04/99
 * @param aDest is the destination buffer
 * @param aDestOffset is the pos to start copy to in the dest buffer
 * @param aSource is the source buffer
 * @param anOffset is the offset to start copying from in the source buffer
 * @param aCount is the (max) number of chars to copy
 */
void CopyChars2To2(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount);
void CopyChars2To2(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount) { 
  PRUnichar* theDest=(PRUnichar*)aDest;
  PRUnichar* to   = theDest+anDestOffset;
  PRUnichar* theSource=(PRUnichar*)aSource;
  PRUnichar* from= theSource+anOffset;

  memcpy((void*)to,(void*)from,aCount*sizeof(PRUnichar));
}


//--------------------------------------------------------------------------------------


typedef void (*CopyChars)(char* aDest,PRInt32 anDestOffset,const char* aSource,PRUint32 anOffset,PRUint32 aCount);

CopyChars gCopyChars[2][2]={
  {&CopyChars1To1,&CopyChars1To2},
  {&CopyChars2To1,&CopyChars2To2}
};


//----------------------------------------------------------------------------------------
//
//  This set of methods is used to search a buffer looking for a char.
//


/**
 *  This methods cans the given buffer for the given char
 *  
 *  @update  gess 02/17/00
 *  @param   aDest is the buffer to be searched
 *  @param   aDestLength is the size (in char-units, not bytes) of the buffer
 *  @param   anOffset is the start pos to begin searching
 *  @param   aChar is the target character we're looking for
 *  @param   aIgnorecase tells us whether to use a case sensitive search
 *  @param   aCount tells us how many characters to iterate through (which may be different than aLength); -1 means use full length.
 *  @return  index of pos if found, else -1 (kNotFound)
 */
inline PRInt32 FindChar1(const char* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRBool aIgnoreCase,PRInt32 aCount) {

  if(anOffset<0)
    anOffset=0;

  if(aCount<0)
    aCount = (PRInt32)aDestLength;

  if((aChar<256) && (0<aDestLength) && ((PRUint32)anOffset<aDestLength)) {

    //We'll only search if the given aChar is within the normal ascii a range,
    //(Since this string is definitely within the ascii range).
    
    if(0<aCount) {

      const char* left= aDest+anOffset;
      const char* last= left+aCount;
      const char* max = aDest+aDestLength;
      const char* end = (last<max) ? last : max;

      if(aIgnoreCase) {
        // safe because aChar < 256
        char theChar=nsCRT::ToUpper(char(aChar));
        while(left<end){
          if(nsCRT::ToUpper(*left)==theChar)
            return left-aDest;
          ++left;
        }
      }
      else {

        PRInt32 theMax = end-left;
        if(0<theMax) {
          unsigned char theChar = (unsigned char) aChar;
          const char* result=(const char*)memchr(left, (int)theChar, theMax);
          if(result) {
            return result-aDest;
          }
        }
      }
    }
  }

  return kNotFound;
}


/**
 *  This methods cans the given buffer for the given char
 *  
 *  @update  gess 3/25/98
 *  @param   aDest is the buffer to be searched
 *  @param   aDestLength is the size (in char-units, not bytes) of the buffer
 *  @param   anOffset is the start pos to begin searching
 *  @param   aChar is the target character we're looking for
 *  @param   aIgnorecase tells us whether to use a case sensitive search
 *  @param   aCount tells us how many characters to iterate through (which may be different than aLength); -1 means use full length.
 *  @return  index of pos if found, else -1 (kNotFound)
 */
inline PRInt32 FindChar2(const PRUnichar* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRBool aIgnoreCase,PRInt32 aCount) {
#ifndef XPCOM_STANDALONE

  if(anOffset<0)
    anOffset=0;

  if(aCount<0)
    aCount = (PRInt32)aDestLength;

  if((0<aDestLength) && ((PRUint32)anOffset<aDestLength)) {
 
    if(0<aCount) {

      const PRUnichar* root = aDest;
      const PRUnichar* left = root+anOffset;
      const PRUnichar* last = left+aCount;
      const PRUnichar* max  = root+aDestLength;
      const PRUnichar* end  = (last<max) ? last : max;

      if (aIgnoreCase && NS_FAILED(NS_InitCaseConversion()))
        aIgnoreCase = PR_FALSE;

      if (aIgnoreCase) {
        PRUnichar theChar;
        gCaseConv->ToUpper(aChar, &theChar);
        while(left<end){
          PRUnichar leftChar;
          gCaseConv->ToUpper(*left, &leftChar);
          if (leftChar == theChar)
            return left-root;
          ++left;
        }
      }
      else {
        while(left<end){
          if(*left==aChar)
            return (left-root);
          ++left;
        }
      }
    }
  }

#else
  NS_ERROR("call not supported in XPCOM_STANDALONE");
#endif
  return kNotFound;
}


/**
 *  This methods cans the given buffer (in reverse) for the given char
 *  
 *  @update  gess 02/17/00
 *  @param   aDest is the buffer to be searched
 *  @param   aDestLength is the size (in char-units, not bytes) of the buffer
 *  @param   anOffset is the start pos to begin searching
 *  @param   aChar is the target character we're looking for
 *  @param   aIgnorecase tells us whether to use a case sensitive search
 *  @param   aCount tells us how many characters to iterate through (which may be different than aLength); -1 means use full length.
 *  @return  index of pos if found, else -1 (kNotFound)
 */

inline PRInt32 RFindChar1(const char* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRBool aIgnoreCase,PRInt32 aCount) {

  if(anOffset<0)
    anOffset=(PRInt32)aDestLength-1;

  if(aCount<0)
    aCount = PRInt32(aDestLength);

  if((aChar<256) && (0<aDestLength) && ((PRUint32)anOffset<aDestLength)) {

    //We'll only search if the given aChar is within the normal ascii a range,
    //(Since this string is definitely within the ascii range).
 
    if(0<aCount) {

      const char* rightmost = aDest+anOffset;  
      const char* min       = rightmost-aCount+1;
      const char* leftmost  = (min<aDest) ? aDest: min;

      if(aIgnoreCase) {
        // safe because aChar < 256
        char theChar=nsCRT::ToUpper(char(aChar));
        while(leftmost<rightmost){
          if(nsCRT::ToUpper(*rightmost)==theChar)
            return rightmost-aDest;
          --rightmost;
        }
      }
      else {

        char theChar=(char)aChar;
        while(leftmost<=rightmost){
          if((*rightmost)==theChar)
            return rightmost-aDest;
          --rightmost;
        }
      }
    }
  }

  return kNotFound;
}


/**
 *  This methods cans the given buffer for the given char
 *  
 *  @update  gess 3/25/98
 *  @param   aDest is the buffer to be searched
 *  @param   aDestLength is the size (in char-units, not bytes) of the buffer
 *  @param   anOffset is the start pos to begin searching
 *  @param   aChar is the target character we're looking for
 *  @param   aIgnorecase tells us whether to use a case sensitive search
 *  @param   aCount tells us how many characters to iterate through (which may be different than aLength); -1 means use full length.
 *  @return  index of pos if found, else -1 (kNotFound)
 */
inline PRInt32 RFindChar2(const PRUnichar* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRBool aIgnoreCase,PRInt32 aCount) {
#ifndef XPCOM_STANDALONE

  if(anOffset<0)
    anOffset=(PRInt32)aDestLength-1;

  if(aCount<0)
    aCount = PRInt32(aDestLength);

  if((0<aDestLength) && ((PRUint32)anOffset<aDestLength)) {
 
    if(0<aCount) {

      const PRUnichar* root      = aDest;
      const PRUnichar* rightmost = root+anOffset;  
      const PRUnichar* min       = rightmost-aCount+1;
      const PRUnichar* leftmost  = (min<root) ? root: min;
      
      if (aIgnoreCase && NS_FAILED(NS_InitCaseConversion()))
        aIgnoreCase = PR_FALSE;

      if(aIgnoreCase) {
        PRUnichar theChar;
        gCaseConv->ToUpper(aChar, &theChar);
        while(leftmost<rightmost){
          PRUnichar rightChar;
          gCaseConv->ToUpper(*rightmost, &rightChar);
          if(rightChar==theChar)
            return rightmost-root;
          --rightmost;
        }
      }
      else {

        while(leftmost<=rightmost){
          if((*rightmost)==aChar)
            return rightmost-root;
          --rightmost;
        }
      }
    }
  }
#else
  NS_ERROR("call not supported in XPCOM_STANDALONE");
#endif

  return kNotFound;
}

//----------------------------------------------------------------------------------------
//
//  This set of methods is used to compare one buffer onto another.
//  The functions are differentiated by the size of source and dest character sizes.
//  WARNING: Your destination buffer MUST be big enough to hold all the source bytes.
//           We don't validate these ranges here (this should be done in higher level routines).
//


/**
 * This method compares the data in one buffer with another
 * @update	gess 01/04/99
 * @param   aStr1 is the first buffer to be compared
 * @param   aStr2 is the 2nd buffer to be compared
 * @param   aCount is the number of chars to compare
 * @param   aIgnorecase tells us whether to use a case-sensitive comparison
 * @return  -1,0,1 depending on <,==,>
 */
static inline PRInt32 Compare1To1(const char* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase);
PRInt32 Compare1To1(const char* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase){ 
  PRInt32 result=0;
  if(aIgnoreCase)
    result=nsCRT::strncasecmp(aStr1,aStr2,aCount);
  else result=memcmp(aStr1,aStr2,aCount);
  return result;
}

/**
 * This method compares the data in one buffer with another
 * @update	gess 01/04/99
 * @param   aStr1 is the first buffer to be compared
 * @param   aStr2 is the 2nd buffer to be compared
 * @param   aCount is the number of chars to compare
 * @param   aIgnorecase tells us whether to use a case-sensitive comparison
 * @return  -1,0,1 depending on <,==,>
 */
PRInt32 Compare2To2(const PRUnichar* aStr1,const PRUnichar* aStr2,PRUint32 aCount,PRBool aIgnoreCase);
PRInt32 Compare2To2(const PRUnichar* aStr1,const PRUnichar* aStr2,PRUint32 aCount,PRBool aIgnoreCase){
  PRInt32 result=0;
#ifndef XPCOM_STANDALONE
  if(aIgnoreCase && NS_SUCCEEDED(NS_InitCaseConversion()))
    gCaseConv->CaseInsensitiveCompare(aStr1, aStr2, aCount, &result);
  else result=nsCRT::strncmp(aStr1,aStr2,aCount);
#else
  NS_ERROR("call not supported in XPCOM_STANDALONE");
#endif
  return result;
}


/**
 * This method compares the data in one buffer with another
 * @update	gess 01/04/99
 * @param   aStr1 is the first buffer to be compared
 * @param   aStr2 is the 2nd buffer to be compared
 * @param   aCount is the number of chars to compare
 * @param   aIgnorecase tells us whether to use a case-sensitive comparison
 * @return  -1,0,1 depending on <,==,>
 */
static PRInt32 Compare2To1(const PRUnichar* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase);
PRInt32 Compare2To1(const PRUnichar* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase){
#ifndef XPCOM_STANDALONE
  const PRUnichar* s1 = aStr1;
  const char *s2 = aStr2;
  
  if (aIgnoreCase && NS_FAILED(NS_InitCaseConversion()))
      aIgnoreCase=PR_FALSE;
  
  if (aStr1 && aStr2) {
    if (aCount != 0) {
      do {
        PRUnichar c1 = *s1++;
        PRUnichar c2 = kIsoLatin1ToUCS2_2[*(const unsigned char*)s2++];
        
        if (c1 != c2) {
          if (aIgnoreCase) {
            
            // case insensitive comparison
            gCaseConv->ToLower(c1, &c1);
            gCaseConv->ToLower(c2, &c2);
            if (c1 == c2) continue;
          }

          if (c1 < c2) return -1;
          return 1;
        }
      } while (--aCount);
    }
  }
#else
  NS_ERROR("call not supported in XPCOM_STANDALONE");
#endif
  return 0;
}


/**
 * This method compares the data in one buffer with another
 * @update	gess 01/04/99
 * @param   aStr1 is the first buffer to be compared
 * @param   aStr2 is the 2nd buffer to be compared
 * @param   aCount is the number of chars to compare
 * @param   aIgnorecase tells us whether to use a case-sensitive comparison
 * @return  -1,0,1 depending on <,==,>
 */
static inline PRInt32 Compare1To2(const char* aStr1,const PRUnichar* aStr2,PRUint32 aCount,PRBool aIgnoreCase);
PRInt32 Compare1To2(const char* aStr1,const PRUnichar* aStr2,PRUint32 aCount,PRBool aIgnoreCase){
  return Compare2To1(aStr2, aStr1, aCount, aIgnoreCase) * -1;
}


//----------------------------------------------------------------------------------------
//
//  This set of methods is used compress char sequences in a buffer...
//


/**
 * This method compresses duplicate runs of a given char from the given buffer 
 *
 * @update	rickg 03.23.2000
 * @param   aString is the buffer to be manipulated
 * @param   aLength is the length of the buffer
 * @param   aSet tells us which chars to compress from given buffer
 * @param   aEliminateLeading tells us whether to strip chars from the start of the buffer
 * @param   aEliminateTrailing tells us whether to strip chars from the start of the buffer
 * @return  the new length of the given buffer
 */
PRInt32 CompressChars1(char* aString,PRUint32 aLength,const char* aSet);
PRInt32 CompressChars1(char* aString,PRUint32 aLength,const char* aSet){ 

  char*  from = aString;
  char*  end =  aString + aLength;
  char*  to = from;

    //this code converts /n, /t, /r into normal space ' ';
    //it also compresses runs of whitespace down to a single char...
  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);

    while (from < end) {
      char theChar = *from++;
      
      *to++=theChar; //always copy this char...

      if((kNotFound!=FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen))){
        while (from < end) {
          theChar = *from++;
          if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen)){
            *to++ = theChar;
            break;
          }
        } //while
      } //if
    } //if
    *to = 0;
  }
  return to - aString;
}



/**
 * This method compresses duplicate runs of a given char from the given buffer 
 *
 * @update	rickg 03.23.2000
 * @param   aString is the buffer to be manipulated
 * @param   aLength is the length of the buffer
 * @param   aSet tells us which chars to compress from given buffer
 * @param   aEliminateLeading tells us whether to strip chars from the start of the buffer
 * @param   aEliminateTrailing tells us whether to strip chars from the start of the buffer
 * @return  the new length of the given buffer
 */
PRInt32 CompressChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet);
PRInt32 CompressChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet){ 

  PRUnichar*  from = aString;
  PRUnichar*  end =  from + aLength;
  PRUnichar*  to = from;

    //this code converts /n, /t, /r into normal space ' ';
    //it also compresses runs of whitespace down to a single char...
  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);

    while (from < end) {
      PRUnichar theChar = *from++;
      
      *to++=theChar; //always copy this char...

      if((theChar<256) && (kNotFound!=FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen))){
        while (from < end) {
          theChar = *from++;
          if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen)){
            *to++ = theChar;
            break;
          }
        } //while
      } //if
    } //if
    *to = 0;
  }
  return to - (PRUnichar*)aString;
}

/**
 * This method strips chars in a given set from the given buffer 
 *
 * @update	gess 01/04/99
 * @param   aString is the buffer to be manipulated
 * @param   aLength is the length of the buffer
 * @param   aSet tells us which chars to compress from given buffer
 * @param   aEliminateLeading tells us whether to strip chars from the start of the buffer
 * @param   aEliminateTrailing tells us whether to strip chars from the start of the buffer
 * @return  the new length of the given buffer
 */
PRInt32 StripChars1(char* aString,PRUint32 aLength,const char* aSet);
PRInt32 StripChars1(char* aString,PRUint32 aLength,const char* aSet){ 

  char*  to   = aString;
  char*  from = aString-1;
  char*  end  = aString + aLength;

  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);
    while (++from < end) {
      char theChar = *from;
      if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen)){
        *to++ = theChar;
      }
    }
    *to = 0;
  }
  return to - (char*)aString;
}


/**
 * This method strips chars in a given set from the given buffer 
 *
 * @update	gess 01/04/99
 * @param   aString is the buffer to be manipulated
 * @param   aLength is the length of the buffer
 * @param   aSet tells us which chars to compress from given buffer
 * @param   aEliminateLeading tells us whether to strip chars from the start of the buffer
 * @param   aEliminateTrailing tells us whether to strip chars from the start of the buffer
 * @return  the new length of the given buffer
 */
PRInt32 StripChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet);
PRInt32 StripChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet){ 

  PRUnichar*  to   = aString;
  PRUnichar*  from = aString-1;
  PRUnichar*  end  = to + aLength;

  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);
    while (++from < end) {
      PRUnichar theChar = *from;
      //Note the test for ascii range below. If you have a real unicode char, 
      //and you're searching for chars in the (given) ascii string, there's no
      //point in doing the real search since it's out of the ascii range.
      if((255<theChar) || (kNotFound==FindChar1(aSet,aSetLen,0,theChar,PR_FALSE,aSetLen))){
        *to++ = theChar;
      }
    }
    *to = 0;
  }
  return to - (PRUnichar*)aString;
}

#endif
