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

// IBMBIDI - Egypt - Start
#include "nsIServiceManager.h"
//#include "nsITextContent.h"
//#include "nsTextRun.h"
//#include "nsTextFragment.h"
//#include "nsTextTransformer.h"
//#include "nsBidiOptions.h"
#include "nsIUBidiUtils.h"
#include "nsIBidi.h"
// Reverse function should be in nsUBidiUtils
static NS_DEFINE_CID(kBidiCID, NS_BIDI_CID);
// IBMBIDI - Egypt - End

NS_DEFINE_IID(kIUBidiUtilsIID, NS_IUBIDIUTILS_IID);

NS_IMPL_ISUPPORTS(nsBidiUtilsImp, kIUBidiUtilsIID);


nsBidiUtilsImp::nsBidiUtilsImp()
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
}

nsBidiUtilsImp::~nsBidiUtilsImp()
{
  PR_AtomicDecrement(&g_InstanceCount);
}

NS_IMETHODIMP nsBidiUtilsImp::Get(PRUnichar aChar, eBidiCategory* oResult)
{
  *oResult = GetBidiCat(aChar);
  if (eBidiCat_CC == *oResult)
    *oResult = (eBidiCategory)(aChar & 0xFF); /* Control codes have special treatment to keep the tables smaller */
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::Is( PRUnichar aChar, eBidiCategory aBidiCategory, PRBool* oResult)
{
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC == bCat)
    bCat = (eBidiCategory)(aChar & 0xFF);
  *oResult = (bCat == aBidiCategory);
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::IsControl( PRUnichar aChar, PRBool* oResult)
{
#define LRM_CHAR 0x200e
    *oResult = (eBidiCat_CC == GetBidiCat(aChar) || ((aChar)&0xfffe)==LRM_CHAR);
    return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::GetICU( PRUnichar aChar, UCharDirection* oResult)
{
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC != bCat) {
    *oResult = ebc2ucd[bCat];
  } else {
    *oResult = cc2ucd[aChar - 0x202a];
  }
  return NS_OK;
}

NS_IMETHODIMP nsBidiUtilsImp::SymmSwap(PRUnichar* aChar)
{
  switch (*aChar & 0xFF00) {

    case 0x0000:
      *aChar ^= symmtable_00[*aChar & 0xff];
      break;

    case 0x2000:
      *aChar ^= symmtable_20[*aChar & 0xff];
      break;

#ifdef HANDLE_GLYPHS_WITHOUT_MATES // placeholder for code to do something in these cases
    PRUint8 mask;

    case 0x2200:
      mask = symmtable_22[*aChar & 0xff];
      if (GWM == mask)
        ; // Do something
      else
        *aChar ^= mask;
      break;

    case 0x2300:
      mask = symmtable_23[*aChar & 0xff];
      if (GWM == mask)
        ; // Do something
      else
        *aChar ^= mask;
      break;
#else
    case 0x2200:
      *aChar ^= symmtable_22[*aChar & 0xff];
      break;

    case 0x2300:
      *aChar ^= symmtable_23[*aChar & 0xff];
      break;
#endif
    case 0x3000:
      *aChar ^= symmtable_30[*aChar & 0xff];
      break;
  }
  return NS_OK;
}
// IBMBIDI - EGYPT - Start

void nsBidiUtilsImp::HebrewReordering(const PRUnichar *aString, PRUint32 aLen,
        PRUnichar* aBuf, PRUint32 &aBufLen)
{
   const PRUnichar* src=aString + aLen - 1;
   PRUnichar* dest= aBuf;
   while(src>=aString)
       *dest++ =  *src--;
   aBufLen = aLen;
}

void nsBidiUtilsImp::ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
             PRUnichar* aBuf, PRUint32 &aBufLen, PRUint32* map)
{
//ahmed: changes to sparate reversing and shaping
#ifdef IBMBIDI
	const PRUnichar* src = aString;
#else
const PRUnichar* src = aString+aLen-1;
#endif // IBMBIDI

   const PRUnichar* p;
   PRUnichar* dest = aBuf;
   PRUnichar formB;

   PRInt8 leftJ, thisJ, rightJ;
   PRInt8 leftNoTrJ, rightNoTrJ;
   
	 thisJ = eNJ;
   rightJ = GetJoiningClass(*(src)) ;
#ifdef IBMBIDI
	 while(src<aString+aLen-1)
	 {
      leftJ = thisJ;
	 
			if ((eTr != leftJ) || ((leftJ == eTr) && !CHAR_IS_ARABIC(*(src-1))))
				leftNoTrJ = thisJ;
      
			for(p=src-2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p+1))) && (p >= (aString)); p--)  
				leftNoTrJ = GetJoiningClass(*(p)) ;
      
			thisJ = rightJ;
			rightJ = rightNoTrJ = GetJoiningClass(*(src+1)) ;
      
			for(p=src+2; (eTr == rightNoTrJ) && (CHAR_IS_ARABIC(*(src+1))) && (p <= (aString+aLen-1)); p++)
				rightNoTrJ = GetJoiningClass(*(p)) ;
      
			formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
      
			if(FONT_HAS_GLYPH(map,formB))
          *dest++ = formB;
      else
          *dest++ = PresentationFormB(*src, eIsolated);
			
			src++;

   }
	 if((eTr != thisJ) || ((thisJ == eTr) && (!CHAR_IS_ARABIC(*(src-1)))))
		 leftNoTrJ = thisJ;

   for(p=src-2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p+1))) && (p >= (aString)); p--)
     leftNoTrJ = GetJoiningClass(*(p)) ;

   formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
   
	 if(FONT_HAS_GLYPH(map,formB))
       *dest++ = formB;
   else
       *dest++ = PresentationFormB(*src, eIsolated);
   src++;

#else //not IBMBIDI   ahmed

	  while(src>aString)
			{
	     leftJ = thisJ;

       if(eTr != thisJ)
         leftNoTrJ = thisJ;

       thisJ = rightJ;
       rightJ = rightNoTrJ = GetJoiningClass(*(src-1)) ;

	     for(p=src-2; (eTr == rightNoTrJ) && (p >= src); p--) 
  			rightNoTrJ = GetJoiningClass(*(p)) ;

       formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
      
			 if(FONT_HAS_GLYPH(map,formB))
          *dest++ = formB;
       else
          *dest++ = PresentationFormB(*src, eIsolated);
      
			 src--;

			}
   
		if(eTr != thisJ)
     leftNoTrJ = thisJ;

    formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
   
		if(FONT_HAS_GLYPH(map,formB))
       *dest++ = formB;
    else
       *dest++ = PresentationFormB(*src, eIsolated);
   src--;

#endif// IBMbidi

	 PRUnichar *lSrc = aBuf;
   PRUnichar *lDest = aBuf;
   while(lSrc < (dest-1))
   {
      PRUnichar next = *(lSrc+1);
      if(((0xFEDF == next) || (0xFEE0 == next)) && 
         (0xFE80 == (0xFFF1 & *lSrc))) 
			{
         PRBool done = PR_FALSE;
         PRUint16 key = ((*lSrc) << 8) | ( 0x00FF & next);
         PRUint16 i;
         for(i=0;i<8;i++)
				 {
             if(key == gArabicLigatureMap[i])
             {
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
   
	 aBufLen = lDest - aBuf; 

#if 0
printf("[");
for(PRUint32 k=0;k<aBufLen;k++)
  printf("%x ", aBuf[k]);
printf("]\n");
#endif
}

PRBool nsBidiUtilsImp::NeedComplexScriptHandling(const PRUnichar *aString, PRUint32 aLen,
       PRBool bFontSupportHebrew, PRBool* oHebrew,
       PRBool bFontSupportArabic, PRBool* oArabic)
{
  PRUint32 i;
  *oHebrew = *oArabic = PR_FALSE;
  if(bFontSupportArabic && bFontSupportHebrew)
  {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=PR_TRUE;
          break;
       } else if(CHAR_IS_ARABIC(aString[i])&& ! IS_HINDI_DIGIT(aString[i])) {//ahmed: aded condition to prevent reversing the hidi nubbers
          *oArabic=PR_TRUE;
          break;
       }
}
  } else if(bFontSupportHebrew) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=PR_TRUE;
          break;
       }
     }
  } else if(bFontSupportArabic) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_ARABIC(aString[i])&& ! IS_HINDI_DIGIT(aString[i])) {//ahmed: aded condition to prevent reversing the hidi nubbers
          *oArabic=PR_TRUE;
          break;
       }
     }
  }
  return *oArabic || *oHebrew;
}

void nsBidiUtilsImp::numbers_to_arabic (PRUnichar* uch)
{
  if ((*uch>=BIDI_START_HINDI_DIGITS) && (*uch<=BIDI_END_HINDI_DIGITS))
    *uch -= (uint16)BIDI_DIGIT_INCREMENT;
}

void nsBidiUtilsImp::numbers_to_hindi (PRUnichar* uch)
{
  if ((*uch>=BIDI_START_ARABIC_DIGITS) && (*uch<=BIDI_END_ARABIC_DIGITS))
    *uch += (uint16)BIDI_DIGIT_INCREMENT;
}

void nsBidiUtilsImp::HandleNumbers (PRUnichar* Buffer, PRUint32 size, PRUint32  Num_Flag)
{
  uint32 i;
	// IBMBIDI_NUMERAL_REGULAR *
	// IBMBIDI_NUMERAL_HINDICONTEXT
	// IBMBIDI_NUMERAL_ARABIC
	// IBMBIDI_NUMERAL_HINDI
	switch (Num_Flag)
	{
  case IBMBIDI_NUMERAL_HINDI:
    for (i=0;i<size;i++)
      nsBidiUtilsImp::numbers_to_hindi(&(Buffer[i]));
		break;
  case IBMBIDI_NUMERAL_ARABIC:
    for (i=0;i<size;i++)
      nsBidiUtilsImp::numbers_to_arabic(&(Buffer[i]));
		break;
	default : // IBMBIDI_NUMERAL_REGULAR, IBMBIDI_NUMERAL_HINDICONTEXT
    for (i=0;i<size;i++)
		{
			if (i>0) // not 1st char
				if (IS_ARABIC_CHAR(Buffer[i-1])) nsBidiUtilsImp::numbers_to_hindi(&(Buffer[i]));
			else nsBidiUtilsImp::numbers_to_arabic(&(Buffer[i]));
		}
		break;
  }
}
void nsBidiUtilsImp::Conv_FE_06 (const nsString aSrc, nsString & aDst)
{
	PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.GetUnicode();
  uint32 i,j, size = aSrc.Length();
	aDst = NS_ConvertToString("");
  for (j=0;j<size;j++)
	{ // j : Source, i : Distination
		aSrcUnichars[j];
		if (aSrcUnichars[j] == 0x0000) break; // no need to convert char after the NULL
		if IS_FE_CHAR(aSrcUnichars[j])
		{
			aDst += FE_TO_06[aSrcUnichars[j] - FE_TO_06_OFFSET][0];
			if (FE_TO_06[aSrcUnichars[j] - FE_TO_06_OFFSET][1])
			{
				// Two characters, we have to resize the buffer :(
				aDst += FE_TO_06[aSrcUnichars[j] - FE_TO_06_OFFSET][1];
				//size ++;
			} // if expands to 2 char
		} else aDst += aSrcUnichars[j]; // copy it even if it is not in FE range
	}// for : loop the buffer
}
// IBMBIDI - EGYPT - End

#endif // IBMBIDI
