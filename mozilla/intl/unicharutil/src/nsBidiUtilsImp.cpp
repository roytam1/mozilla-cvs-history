/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.  Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifdef IBMBIDI
#include "nsCom.h"
#include "pratom.h"
#include "nsUUDll.h"
#include "nsISupports.h"
#include "nsBidiUtilsImp.h"
#include "bidicattable.h"
#include "symmtable.h"

#include "nsIServiceManager.h"
#include "nsIUBidiUtils.h"
#include "nsIBidi.h"
static NS_DEFINE_CID(kBidiCID, NS_BIDI_CID);
NS_IMPL_ISUPPORTS(nsBidiUtilsImp, NS_GET_IID(nsIUBidiUtils));

static nsCharType ebc2ucd[15] = {
    eCharType_OtherNeutral, /* Placeholder -- there will never be a 0 index value */
    eCharType_LeftToRight,
    eCharType_RightToLeft,
    eCharType_RightToLeftArabic,
    eCharType_ArabicNumber,
    eCharType_EuropeanNumber,
    eCharType_EuropeanNumberSeparator,
    eCharType_EuropeanNumberTerminator,
    eCharType_CommonNumberSeparator,
    eCharType_OtherNeutral,
    eCharType_DirNonSpacingMark,
    eCharType_BoundaryNeutral,
    eCharType_BlockSeparator,
    eCharType_SegmentSeparator,
    eCharType_WhiteSpaceNeutral
};

static nsCharType cc2ucd[5] = {
    eCharType_LeftToRightEmbedding,
    eCharType_RightToLeftEmbedding,
    eCharType_PopDirectionalFormat,
    eCharType_LeftToRightOverride,
    eCharType_RightToLeftOverride
};

//#define FULL_ARABIC_SHAPING 1
// the Array Index = FE_CHAR - FE_TO_06_OFFSET

#define FE_TO_06_OFFSET 0xfe70

static PRUnichar  FE_TO_06 [][2] = {
    {0x064b,0x0000},{0x064b,0x0640},{0x064c,0x0000},
    {0x0000,0x0000},{0x064d,0x0000},{0x0000,0x0000},
    {0x064e,0x0000},{0x064e,0x0640},{0x064f,0x0000},
    {0x064f,0x0640},{0x0650,0x0000},{0x0650,0x0640},
    {0x0651,0x0000},{0x0651,0x0640},{0x0652,0x0000},
    {0x0652,0x0640},{0x0621,0x0000},{0x0622,0x0000},
    {0x0622,0x0000},{0x0623,0x0000},{0x0623,0x0000},
    {0x0624,0x0000},{0x0624,0x0000},{0x0625,0x0000},
    {0x0625,0x0000},{0x0626,0x0000},{0x0626,0x0000},
    {0x0626,0x0000},{0x0626,0x0000},{0x0627,0x0000},
    {0x0627,0x0000},{0x0628,0x0000},{0x0628,0x0000},
    {0x0628,0x0000},{0x0628,0x0000},{0x0629,0x0000},
    {0x0629,0x0000},{0x062a,0x0000},{0x062a,0x0000},
    {0x062a,0x0000},{0x062a,0x0000},{0x062b,0x0000},
    {0x062b,0x0000},{0x062b,0x0000},{0x062b,0x0000},
    {0x062c,0x0000},{0x062c,0x0000},{0x062c,0x0000},
    {0x062c,0x0000},{0x062d,0x0000},{0x062d,0x0000},
    {0x062d,0x0000},{0x062d,0x0000},{0x062e,0x0000},
    {0x062e,0x0000},{0x062e,0x0000},{0x062e,0x0000},
    {0x062f,0x0000},{0x062f,0x0000},{0x0630,0x0000},
    {0x0630,0x0000},{0x0631,0x0000},{0x0631,0x0000},
    {0x0632,0x0000},{0x0632,0x0000},{0x0633,0x0000},
    {0x0633,0x0000},{0x0633,0x0000},{0x0633,0x0000},
    {0x0634,0x0000},{0x0634,0x0000},{0x0634,0x0000},
    {0x0634,0x0000},{0x0635,0x0000},{0x0635,0x0000},
    {0x0635,0x0000},{0x0635,0x0000},{0x0636,0x0000},
    {0x0636,0x0000},{0x0636,0x0000},{0x0636,0x0000},
    {0x0637,0x0000},{0x0637,0x0000},{0x0637,0x0000},
    {0x0637,0x0000},{0x0638,0x0000},{0x0638,0x0000},
    {0x0638,0x0000},{0x0638,0x0000},{0x0639,0x0000},
    {0x0639,0x0000},{0x0639,0x0000},{0x0639,0x0000},
    {0x063a,0x0000},{0x063a,0x0000},{0x063a,0x0000},
    {0x063a,0x0000},{0x0641,0x0000},{0x0641,0x0000},
    {0x0641,0x0000},{0x0641,0x0000},{0x0642,0x0000},
    {0x0642,0x0000},{0x0642,0x0000},{0x0642,0x0000},
    {0x0643,0x0000},{0x0643,0x0000},{0x0643,0x0000},
    {0x0643,0x0000},{0x0644,0x0000},{0x0644,0x0000},
    {0x0644,0x0000},{0x0644,0x0000},{0x0645,0x0000},
    {0x0645,0x0000},{0x0645,0x0000},{0x0645,0x0000},
    {0x0646,0x0000},{0x0646,0x0000},{0x0646,0x0000},
    {0x0646,0x0000},{0x0647,0x0000},{0x0647,0x0000},
    {0x0647,0x0000},{0x0647,0x0000},{0x0648,0x0000},
    {0x0648,0x0000},{0x0649,0x0000},{0x0649,0x0000},
    {0x064a,0x0000},{0x064a,0x0000},{0x064a,0x0000},
    {0x064a,0x0000},{0x0644,0x0622},{0x0644,0x0622},
    {0x0644,0x0623},{0x0644,0x0623},{0x0644,0x0625},
    {0x0644,0x0625},{0x0644,0x0627},{0x0644,0x0627},
    {0x0000,0x0000},{0x0000,0x0000},{0x0000,0x0000}
};

//============ Begin Arabic Basic to Presentation Form B Code ============
// Note: the following code are moved from gfx/src/windows/nsRenderingContextWin.cpp
static PRUint8 gArabicMap1[] = {
            0x81, 0x83, 0x85, 0x87, 0x89, 0x8D, // 0622-0627
0x8F, 0x93, 0x95, 0x99, 0x9D, 0xA1, 0xA5, 0xA9, // 0628-062F
0xAB, 0xAD, 0xAF, 0xB1, 0xB5, 0xB9, 0xBD, 0xC1, // 0630-0637
0xC5, 0xC9, 0xCD                                // 0638-063A
};

static PRUint8 gArabicMap2[] = {
      0xD1, 0xD5, 0xD9, 0xDD, 0xE1, 0xE5, 0xE9, // 0641-0647
0xED, 0xEF, 0xF1                                // 0648-064A
};

#define PresentationFormB(c, form)                           \
  (((0x0622<=(c)) && ((c)<=0x063A)) ?                        \
    (0xFE00|(gArabicMap1[(c)-0x0622] + (form))) :            \
     (((0x0641<=(c)) && ((c)<=0x064A)) ?                     \
      (0xFE00|(gArabicMap2[(c)-0x0641] + (form))) : (c)))

enum {
   eIsolated,  // or Char N
   eFinal,     // or Char R
   eInitial,   // or Char L
   eMedial     // or Char M
} eArabicForm;
enum {
   eTr = 0, // Transparent
   eRJ = 1, // Right-Joining
   eLJ = 2, // Left-Joining
   eDJ = 3, // Dual-Joining
   eNJ  = 4,// Non-Joining
   eJC = 7, // Joining Causing
   eRightJCMask = 2, // bit of Right-Join Causing 
   eLeftJCMask = 1   // bit of Left-Join Causing 
} eArabicJoiningClass;

#define RightJCClass(j) (eRightJCMask&(j))
#define LeftJCClass(j)  (eLeftJCMask&(j))

#define DecideForm(jl,j,jr)                                 \
  (((eRJ == (j)) && RightJCClass(jr)) ? eFinal              \
                                      :                     \
   ((eDJ == (j)) ?                                          \
    ((RightJCClass(jr)) ?                                   \
     (((LeftJCClass(jl)) ? eMedial                          \
                         : eFinal))                         \
                        :                                   \
     (((LeftJCClass(jl)) ? eInitial                         \
                         : eIsolated))                      \
    )                     : eIsolated))                     \


static PRInt8 gJoiningClass[] = {
          eRJ, eRJ, eRJ, eRJ, eDJ, eRJ, // 0620-0627
eDJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, eRJ, // 0628-062F
eRJ, eRJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0630-0637
eDJ, eDJ, eDJ, eNJ, eNJ, eNJ, eNJ, eNJ, // 0638-063F
eJC, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0640-0647
eRJ, eRJ, eDJ, eTr, eTr, eTr, eTr, eTr, // 0648-064F
eTr, eTr, eTr                           // 0650-0652
};

#define GetJoiningClass(c)                   \
  (((0x0622 <= (c)) && ((c) <= 0x0652)) ?    \
       (gJoiningClass[(c) - 0x0622]) :       \
      ((0x200D == (c)) ? eJC : eTr))

static PRUint16 gArabicLigatureMap[] = 
{
0x82DF, // 0xFE82 0xFEDF -> 0xFEF5
0x82E0, // 0xFE82 0xFEE0 -> 0xFEF6
0x84DF, // 0xFE84 0xFEDF -> 0xFEF7
0x84E0, // 0xFE84 0xFEE0 -> 0xFEF8
0x88DF, // 0xFE88 0xFEDF -> 0xFEF9
0x88E0, // 0xFE88 0xFEE0 -> 0xFEFA
0x8EDF, // 0xFE8E 0xFEDF -> 0xFEFB
0x8EE0  // 0xFE8E 0xFEE0 -> 0xFEFC
};
#define CHAR_IS_HEBREW(c) ((0x0590 <= (c)) && ((c)<= 0x05FF))
#define CHAR_IS_ARABIC(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))
// Note: The above code are moved from gfx/src/windows/nsRenderingContextWin.cpp

#define LRM_CHAR 0x200e
#define ARABIC_TO_HINDI_DIGIT_INCREMENT (START_HINDI_DIGITS - START_ARABIC_DIGITS)
#define NUM_TO_ARABIC(c) \
  ((((c)>=START_HINDI_DIGITS) && ((c)<=END_HINDI_DIGITS)) ? \
   ((c) - (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT) : \
   (c))
#define NUM_TO_HINDI(c) \
  ((((c)>=START_ARABIC_DIGITS) && ((c)<=END_ARABIC_DIGITS)) ? \
   ((c) + (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT): \
   (c))

nsBidiUtilsImp::nsBidiUtilsImp()
{
  NS_INIT_REFCNT();
}

nsBidiUtilsImp::~nsBidiUtilsImp()
{
}

NS_IMETHODIMP nsBidiUtilsImp::GetBidiCategory(PRUnichar aChar, eBidiCategory* oResult)
{
  *oResult = GetBidiCat(aChar);
  if (eBidiCat_CC == *oResult)
    *oResult = (eBidiCategory)(aChar & 0xFF); /* Control codes have special treatment to keep the tables smaller */
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::IsBidiCategory(PRUnichar aChar, eBidiCategory aBidiCategory, PRBool* oResult)
{
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC == bCat)
    bCat = (eBidiCategory)(aChar & 0xFF);
  *oResult = (bCat == aBidiCategory);
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::IsBidiControl(PRUnichar aChar, PRBool* oResult)
{
  // This method is used when stripping Bidi control characters for
  // display, so it will return TRUE for LRM and RLM as well as the
  // characters with category eBidiCat_CC
  *oResult = (eBidiCat_CC == GetBidiCat(aChar) || ((aChar)&0xfffe)==LRM_CHAR);
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::GetCharType(PRUnichar aChar, nsCharType* oResult)
{
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC != bCat) {
    NS_ASSERTION(bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)), "size mismatch");
    if(bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)))
      *oResult = ebc2ucd[bCat];
    else 
      *oResult = ebc2ucd[0]; // something is very wrong, but we need to return a value
  } else {
    NS_ASSERTION((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)), "size mismatch");
    if((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)))
      *oResult = cc2ucd[aChar - 0x202a];
    else 
      *oResult = ebc2ucd[0]; // something is very wrong, but we need to return a value
  }
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::SymmSwap(PRUnichar* aChar)
{
  *aChar = Mirrored(*aChar);
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
                                            PRUnichar* aBuf, PRUint32 *aBufLen)
{
  // Note: The real implementation is still under reviewing process 
  // will be check in soon. 
  // a stub routine which simply copy the data is now place here untill the
  // real code get check in.
#ifdef FULL_ARABIC_SHAPING
  // Start Unreview Code
  const PRUnichar* src = aString;
  const PRUnichar* p;
  PRUnichar* dest = aBuf;
  PRUnichar formB;
  PRInt8 leftJ, thisJ, rightJ;
  PRInt8 leftNoTrJ, rightNoTrJ;
  thisJ = eNJ;
  rightJ = GetJoiningClass(*(src));
  while(src<aString+aLen-1) {
    leftJ = thisJ;

    if ((eTr != leftJ) || ((leftJ == eTr) && 
        ( ( (src-1) >= aString ) && !CHAR_IS_ARABIC(*(src-1)))))
      leftNoTrJ = thisJ;

    if(src-2 >= (aString)){
      for(p=src-2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p+1))) /*&& (p >= (aString))*/; p--)  
        leftNoTrJ = GetJoiningClass(*(p)) ;
    }

    thisJ = rightJ;
    rightJ = rightNoTrJ = GetJoiningClass(*(src+1)) ;

    if(src+2 <= (aString+aLen-1)){
      for(p=src+2; (eTr == rightNoTrJ) && (CHAR_IS_ARABIC(*(src+1))); p++)
        rightNoTrJ = GetJoiningClass(*(p)) ;
    }

    formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
    *dest++ = formB;
    src++;

  }
  if((eTr != thisJ) || 
     ((thisJ == eTr) && (((src-1)>=aString) && (!CHAR_IS_ARABIC(*(src-1))))))
    leftNoTrJ = thisJ;

  if(src-2 >= (aString)){
    for(p=src-2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p+1))); p--)
      leftNoTrJ = GetJoiningClass(*(p)) ;
  }

  formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
  *dest++ = formB;
  src++;

  PRUnichar *lSrc = aBuf;
  PRUnichar *lDest = aBuf;
  while(lSrc < (dest-1)) {
    PRUnichar next = *(lSrc+1);
    if(((0xFEDF == next) || (0xFEE0 == next)) && 
       (0xFE80 == (0xFFF1 & *lSrc))) {
      PRBool done = PR_FALSE;
      PRUint16 key = ((*lSrc) << 8) | ( 0x00FF & next);
      PRUint16 i;
      for(i=0;i<8;i++) {
        if(key == gArabicLigatureMap[i]) {
          done = PR_TRUE;
          *lDest++ = 0xFEF5 + i;
          lSrc+=2;
          break;
        }
      }
      if(! done)
        *lDest++ = *lSrc++; 
    } 
    else 
      *lDest++ = *lSrc++; 

  }
  if(lSrc < dest)
    *lDest++ = *lSrc++; 

  *aBufLen = lDest - aBuf;
  return NS_OK;
#else
  for(*aBufLen = 0;*aBufLen < aLen; (*aBufLen)++)
    aBuf[*aBufLen] = aString[*aBufLen];
  return NS_OK;
#endif
}


NS_IMETHODIMP nsBidiUtilsImp::HandleNumbers(PRUnichar* aBuffer, PRUint32 aSize, PRUint32 aNumFlag)
{
  PRUint32 i;
  // IBMBIDI_NUMERAL_REGULAR *
  // IBMBIDI_NUMERAL_HINDICONTEXT
  // IBMBIDI_NUMERAL_ARABIC
  // IBMBIDI_NUMERAL_HINDI
  mNumflag=aNumFlag;
  
  switch (aNumFlag) {
    case IBMBIDI_NUMERAL_HINDI:
      for (i=0;i<aSize;i++)
        aBuffer[i] = NUM_TO_HINDI(aBuffer[i]);
      break;
    case IBMBIDI_NUMERAL_ARABIC:
      for (i=0;i<aSize;i++)
        aBuffer[i] = NUM_TO_ARABIC(aBuffer[i]);
      break;
    default : // IBMBIDI_NUMERAL_REGULAR, IBMBIDI_NUMERAL_HINDICONTEXT for HandleNum() which is called for clipboard handling
      for (i=1;i<aSize;i++) {
        if (IS_ARABIC_CHAR(aBuffer[i-1])) 
          aBuffer[i] = NUM_TO_HINDI(aBuffer[i]);
        else 
          aBuffer[i] = NUM_TO_ARABIC(aBuffer[i]);
      }
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::HandleNumbers(const nsString aSrc, nsString & aDst)
{
  aDst = aSrc;
  return HandleNumbers((PRUnichar *)aDst.get(),aDst.Length(),mNumflag);
}

NS_IMETHODIMP nsBidiUtilsImp::Conv_FE_06(const nsString aSrc, nsString & aDst)
{
  // Note: The real implementation is still under reviewing process 
  // will be check in soon. 
  // a stub routine which simply copy the data is now place here untill the
  // real code get check in.
#ifdef FULL_ARABIC_SHAPING
  // Start Unreview Code
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRUint32 i, size = aSrc.Length();
  aDst = NS_ConvertASCIItoUCS2("");
  for (i=0;i<size;i++) { // i : Source
    aSrcUnichars[i];
    if (aSrcUnichars[i] == 0x0000) 
      break; // no need to convert char after the NULL
    if (IS_FE_CHAR(aSrcUnichars[i])) {
      //ahmed for lamalf
      PRUnichar ch = (FE_TO_06[aSrcUnichars[i] - FE_TO_06_OFFSET][1]);
      if(ch)
        aDst += ch;
      ch=(FE_TO_06[aSrcUnichars[i] - FE_TO_06_OFFSET][0]);
      if(ch)
        aDst += ch;
      else //if it is 00, just output what we have in FExx
        aDst += aSrcUnichars[i];
    } else {
      aDst += aSrcUnichars[i]; // copy it even if it is not in FE range
    }
  }// for : loop the buffer
#else
  aDst= aSrc;
#endif
  return NS_OK;
}
/////////////////////////////////////////////////////////////////////
NS_IMETHODIMP nsBidiUtilsImp::Conv_FE_06_WithReverse(const nsString aSrc, nsString & aDst)
{
  // Note: The real implementation is still under reviewing process 
  // will be check in soon. 
  // a stub routine which simply copy the data is now place here untill the
  // real code get check in.
#ifdef FULL_ARABIC_SHAPING
  // Start Unreview Code
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRBool foundArabic = PR_FALSE;
  PRUint32 i,endArabic, beginArabic, size = aSrc.Length();
  aDst = NS_ConvertASCIItoUCS2("");
  for (endArabic=0;endArabic<size;endArabic++) {
    if (aSrcUnichars[endArabic] == 0x0000) 
      break; // no need to convert char after the NULL

    while( (IS_FE_CHAR(aSrcUnichars[endArabic]))||
           (CHAR_IS_ARABIC(aSrcUnichars[endArabic]))||
           (IS_ARABIC_DIGIT(aSrcUnichars[endArabic]))||
           (aSrcUnichars[endArabic]==0x0020)) 
    {
      if(! foundArabic ) {
        beginArabic=endArabic;
        foundArabic= PR_TRUE;
      }
      endArabic++;
    }
    if(foundArabic) {
      endArabic--;
      for (i=endArabic; i>=beginArabic; i--) {
        if(IS_FE_CHAR(aSrcUnichars[i])) {
          //ahmed for the bug of lamalf
          aDst += FE_TO_06[aSrcUnichars[i] - FE_TO_06_OFFSET][0];
          if (FE_TO_06[aSrcUnichars[i] - FE_TO_06_OFFSET][1]) {
            // Two characters, we have to resize the buffer :(
            aDst += FE_TO_06[aSrcUnichars[i] - FE_TO_06_OFFSET][1];
          } // if expands to 2 char
        } else {
          // do we need to check the following if ?
          if((CHAR_IS_ARABIC(aSrcUnichars[i]))||
             (IS_ARABIC_DIGIT(aSrcUnichars[i]))||
             (aSrcUnichars[i]==0x0020))
            aDst += aSrcUnichars[i];
        }     
      }
    } else {
      aDst += aSrcUnichars[endArabic]; 
    }
    foundArabic=PR_FALSE;
  }// for : loop the buffer
#else
  aDst= aSrc;
#endif
  return NS_OK;
}
////////////////////////////////////////////////////////////
//ahmed
NS_IMETHODIMP nsBidiUtilsImp::Conv_06_FE_WithReverse(const nsString aSrc,
nsString & aDst,PRUint32 aDir)
{
  // Note: The real implementation is still under reviewing process 
  // will be check in soon. 
  // a stub routine which simply copy the data is now place here untill the
  // real code get check in.
#ifdef FULL_ARABIC_SHAPING
  // Start Unreview Code
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRUint32 i,beginArabic, endArabic, size = aSrc.Length();
  aDst = NS_ConvertASCIItoUCS2("");
  PRBool foundArabic = PR_FALSE;
  for (endArabic=0;endArabic<size;endArabic++) {
    if (aSrcUnichars[endArabic] == 0x0000) 
      break; // no need to convert char after the NULL

    while( (IS_06_CHAR(aSrcUnichars[endArabic])) || 
           (CHAR_IS_ARABIC(aSrcUnichars[endArabic])) || 
           (aSrcUnichars[endArabic]==0x0020) || 
           (IS_ARABIC_DIGIT(aSrcUnichars[endArabic]))  ) 
    {
      if(! foundArabic) {
        beginArabic=endArabic;
        foundArabic=PR_TRUE;
      }
      endArabic++;
    }
    if(foundArabic) {
      endArabic--;
      PRUnichar buf[8192];
      PRUint32 len=8192;
      //reverse the buffer for shaping

      for(i=beginArabic; i<=endArabic; i++) {
        buf[i-beginArabic]=aSrcUnichars[endArabic-i+beginArabic];
      }
      for(i=0; i<=endArabic-beginArabic; i++) {
        aSrcUnichars[i+beginArabic]=buf[i];
      }
      ArabicShaping(&aSrcUnichars[beginArabic], endArabic-beginArabic+1, buf, &len);
      // to reverse the numerals
      PRUint32 endNumeral, beginNumeral;
      for (endNumeral=0;endNumeral<=len-1;endNumeral++){
        PRBool foundNumeral = PR_FALSE;
        while((endNumeral < len) && (IS_ARABIC_DIGIT(buf[endNumeral]))  ) {
          if(!foundNumeral)
          {
            foundNumeral=PR_TRUE;
            beginNumeral=endNumeral;
          }
          endNumeral++;
        }
        if(foundNumeral){
          endNumeral--;
          PRUnichar numbuf[20];
          for(i=beginNumeral; i<=endNumeral; i++){
            numbuf[i-beginNumeral]=buf[endNumeral-i+beginNumeral];
          }
          for(i=0;i<=endNumeral-beginNumeral;i++){
            buf[i+beginNumeral]=numbuf[i];
          }
        }
      }
      if(aDir==1){//ltr
        for (i=0;i<=len-1;i++){
          aDst+= buf[i];
        } 
      }
      else if(aDir==2){//rtl
        for (i=0;i<=len-1;i++){
          aDst+= buf[len-1-i];
        } 
      }
    } else {
      aDst += aSrcUnichars[endArabic];
    }
    foundArabic=PR_FALSE;
  }// for : loop the buffer
#else
  aDst= aSrc;
#endif
  return NS_OK;
}

#endif // IBMBIDI
