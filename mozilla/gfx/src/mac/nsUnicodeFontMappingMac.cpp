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
#include <Script.h>

#include "plhash.h"
#include "nsCRT.h"

#include "nsDeviceContextMac.h"
#include "nsFontEnumerator.h"

#include "nsUnicodeFontMappingMac.h"
#include "nsUnicodeFontMappingCache.h"
#include "nsUnicodeMappingUtil.h"


#define BAD_FONT_NUM	-1
#define BAD_SCRIPT 0x7f


//--------------------------------------------------------------------------

static void FillVarBlockToScript( PRInt8 script, PRInt8 *aMap)
{
	if(BAD_SCRIPT == aMap[kBasicLatin - kUnicodeBlockFixedScriptMax])
		aMap[kBasicLatin - kUnicodeBlockFixedScriptMax] = script;

	if(BAD_SCRIPT == aMap[kOthers - kUnicodeBlockFixedScriptMax])
		aMap[kOthers - kUnicodeBlockFixedScriptMax] = script;

	switch( script )
	{
		case smRoman: 
		case smCentralEuroRoman: 
		case smVietnamese: 
			if(BAD_SCRIPT == aMap[kLatin - kUnicodeBlockFixedScriptMax])
				aMap[kLatin - kUnicodeBlockFixedScriptMax] = script;
			break;
		case smTradChinese:
			if(BAD_SCRIPT == aMap[kCJKMisc - kUnicodeBlockFixedScriptMax])
				aMap[kCJKMisc - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax])
				aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax] = script;
			break;
		case smKorean:
		case smJapanese:
		case smSimpChinese:
			if(BAD_SCRIPT == aMap[kCJKMisc - kUnicodeBlockFixedScriptMax])
				aMap[kCJKMisc - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax])
				aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kHiraganaKatakana - kUnicodeBlockFixedScriptMax])
				aMap[kHiraganaKatakana - kUnicodeBlockFixedScriptMax] = script;
			break;			
	};
}

struct MyFontEnumData {
    MyFontEnumData(nsIDeviceContext* aDC, PRInt8* pMap, short* pFonts)  : mContext(aDC) {
    	mMap = pMap;
    	mFont = pFonts;
    };
    nsIDeviceContext* mContext;
	PRInt8 *mMap;
	short *mFont;
};
//--------------------------------------------------------------------------

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  MyFontEnumData* data = (MyFontEnumData*)aData;


  if (aGeneric)
  {
    nsGenericFontNameType type = nsUnicodeMappingUtil::MapGenericFontNameType(aFamily);

  	if( type != kUknownGenericFontName)
  	{
      nsUnicodeMappingUtil* unicodeMappingUtil = nsUnicodeMappingUtil::GetSingleton();

  	  for (ScriptCode script = 0; script < 32 ; script++)
  		{
  			const nsString* fontName = unicodeMappingUtil->GenericFontNameForScript(script,type);
  			if (nsnull != fontName)
		{
			    short fontNum;
  				nsFontEnumeratorMac::GetMacFontID(*fontName, fontNum);
					
				if((0 != fontNum) && (BAD_FONT_NUM == data->mFont[ script ])) 
					data->mFont[ script ] = fontNum;
				// since this is a generic font name, it won't impact the order of script we used
			}
		}
	}
  }
  else
  {
    nsAutoString realFace;
    PRBool  aliased;
    data->mContext->GetLocalFontName(aFamily, realFace, aliased);
    if (aliased || (NS_OK == data->mContext->CheckFontExistence(realFace)))
    {
	    short fontNum;
  		nsFontEnumeratorMac::GetMacFontID(realFace, fontNum);
		
		if(0 != fontNum) {
			ScriptCode script = ::FontToScript(fontNum);
			if(BAD_FONT_NUM == data->mFont[ script ]) 
				data->mFont[ script ] = fontNum;
			FillVarBlockToScript( script, data->mMap);
		}
	}
  }
  return PR_TRUE;
}

//--------------------------------------------------------------------------
nsUnicodeMappingUtil *nsUnicodeFontMappingMac::gUtil = nsnull;

//--------------------------------------------------------------------------

void nsUnicodeFontMappingMac::InitByFontFamily(nsFont* aFont, nsIDeviceContext *aDeviceContext) 
{
   	MyFontEnumData fontData(aDeviceContext, mPrivBlockToScript, mScriptFallbackFontIDs);
  	aFont->EnumerateFamilies(FontEnumCallback, &fontData);
}

//--------------------------------------------------------------------------

void nsUnicodeFontMappingMac::processOneLangRegion(const char* aLanguage, const char* aRegion )
{
	if(gUtil->ScriptEnabled(smTradChinese) &&
	   ((! nsCRT::strcmp(aLanguage,"zh")) &&
	    ((! nsCRT::strcmp(aRegion,"TW")) || (! nsCRT::strcmp(aRegion,"HK"))))) 
	{
		FillVarBlockToScript(smTradChinese, mPrivBlockToScript);
	} 
	else if(gUtil->ScriptEnabled(smSimpChinese) && (! nsCRT::strcmp(aLanguage,"zh"))) 
	{
		FillVarBlockToScript(smSimpChinese, mPrivBlockToScript);
	} 
	else if(gUtil->ScriptEnabled(smKorean) && (! nsCRT::strcmp(aLanguage,"ko"))) 
	{
		FillVarBlockToScript(smKorean, mPrivBlockToScript);
	}
	else if(gUtil->ScriptEnabled(smJapanese) && (! nsCRT::strcmp(aLanguage,"ja"))) 
	{
		FillVarBlockToScript(smJapanese, mPrivBlockToScript);
	}
}

//--------------------------------------------------------------------------
PRBool nsUnicodeFontMappingMac::ScriptMapInitComplete()
{
	PRInt32 i;
	for(i = kUnicodeBlockFixedScriptMax ; i < kUnicodeBlockSize; i++) {
	   if(BAD_SCRIPT == mPrivBlockToScript[i - kUnicodeBlockFixedScriptMax])
	   		return PR_FALSE;
	}
	return PR_TRUE;
}
//--------------------------------------------------------------------------

const PRUnichar kA = PRUnichar('A');
const PRUnichar kZ = PRUnichar('Z');
const PRUnichar ka = PRUnichar('a');
const PRUnichar kz = PRUnichar('z');
const PRUnichar kComma = PRUnichar(',');
const PRUnichar kUnderline = PRUnichar('_');
const PRUnichar kSpace = PRUnichar(' ');
const PRUnichar kNullCh       = PRUnichar('\0');
//--------------------------------------------------------------------------

void nsUnicodeFontMappingMac::InitByLANG(const nsString& aLANG)
{
	// do not countinue if there are no difference to look at the lang tag
	if( ScriptMapInitComplete() )
		return;
  	const PRUnichar *p = aLANG.get();
  	PRUint32 len = aLANG.Length();
  	char language[3];
  	char region[3];
  	language[2] = region[2]= '\0';;
  	language[0]= language[1] = region[0]= region[1] = ' ';
	PRUint32 state = 0;
	
	for(PRUint32 i = 0; (state != -1) && (i < len); i++, p++)
  	{
  		switch (state) {
  			case 0:
  				if(( ka <= *p) && (*p <= kz )) {
  					language[state++] = (char)*p;
  				} else if( kSpace == *p) {
  					state = 0;
  				} else {
  					state = -1;
  				}
  				break;
   			case 1:
  				if(( ka <= *p) && (*p <= kz )) {
  					language[state++] = (char)*p;
  				} else {
  					state = -1;
  				}
  				break;
  			case 2:
  				if(kComma == *p) {
  					processOneLangRegion(language, region);
				  	return;
  				} else if(kUnderline == *p) {
  					state = 3;
  				} else {
  					state = -1;
  				}
   				break;
   			case 3:
   			case 4:
  				if(( kA <= *p) && (*p <= kZ )) {
  					region[state - 3] = (char)*p;
  					state++;
  				} else {
  					state = -1;
  				}
  				break;
  			case 5:
  				if(kComma == *p) {
 					processOneLangRegion(language, region);
				  	return;
  				} else {
  					state = -1;
  				}
   				break;
  		};
  	}
  	if((2 == state) || (5 == state)) {
 		processOneLangRegion(language, region);
  	}
}
//--------------------------------------------------------------------------
void nsUnicodeFontMappingMac::InitByLangGroup(const nsString& aLangGroup)
{
	// do not countinue if there are no difference to look at the document Charset
	if( ScriptMapInitComplete() )
		return;
	if(gUtil->ScriptEnabled(smRoman) && aLangGroup.EqualsIgnoreCase("x-western"))
  	{
		FillVarBlockToScript(smRoman, mPrivBlockToScript);		
  	} else if(gUtil->ScriptEnabled(smSimpChinese) && aLangGroup.EqualsIgnoreCase("zh-CN"))
  	{
		FillVarBlockToScript(smSimpChinese, mPrivBlockToScript);
  	} else if(gUtil->ScriptEnabled(smKorean) && aLangGroup.EqualsIgnoreCase("ko"))
  	{
		FillVarBlockToScript(smKorean, mPrivBlockToScript);
  	} else if(gUtil->ScriptEnabled(smTradChinese)  && aLangGroup.EqualsIgnoreCase("zh-TW"))
  	{
		FillVarBlockToScript(smTradChinese, mPrivBlockToScript);
  	} else if(gUtil->ScriptEnabled(smJapanese) && aLangGroup.EqualsIgnoreCase("ja"))
  	{
		FillVarBlockToScript(smJapanese, mPrivBlockToScript);
  	}
}
//--------------------------------------------------------------------------

void nsUnicodeFontMappingMac::InitDefaultScriptFonts()
{
	for(PRInt32 i = 0 ; i < smPseudoTotalScripts; i++)
	{
		// copy from global mapping
	   if(BAD_FONT_NUM == mScriptFallbackFontIDs[i])
   			mScriptFallbackFontIDs[i] = gUtil->ScriptFont(i);
    }
 	for(PRInt32 k = kUnicodeBlockFixedScriptMax ; k < kUnicodeBlockSize; k++)
	{
		// copy from global mapping
	    if(BAD_SCRIPT == mPrivBlockToScript[k - kUnicodeBlockFixedScriptMax])
	   		mPrivBlockToScript[k - kUnicodeBlockFixedScriptMax] = gUtil->BlockToScript((nsUnicodeBlock) k);
    }
}
//--------------------------------------------------------------------------

nsUnicodeFontMappingMac::nsUnicodeFontMappingMac(
	nsFont* aFont, nsIDeviceContext *aDeviceContext, 
	const nsString& aLangGroup, const nsString& aLANG) 
{
  PRInt32 i;
  if (!gUtil)
    gUtil = nsUnicodeMappingUtil::GetSingleton();
	for(i = kUnicodeBlockFixedScriptMax ; i < kUnicodeBlockSize; i++)
	   mPrivBlockToScript[i - kUnicodeBlockFixedScriptMax] = BAD_SCRIPT;
	for(i = 0 ; i < smPseudoTotalScripts; i++)
	   mScriptFallbackFontIDs[i] = BAD_FONT_NUM;
	   
	InitByFontFamily(aFont, aDeviceContext);
	InitByLANG(aLANG);
	InitByLangGroup(aLangGroup);
	InitDefaultScriptFonts();
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

PRBool nsUnicodeFontMappingMac::Equals(const nsUnicodeFontMappingMac& aMap)
{
	PRUint32 i;
	if(&aMap != this) {
		for( i=0; i < smPseudoTotalScripts; i++)
		{
			if(mScriptFallbackFontIDs[i] != aMap.mScriptFallbackFontIDs[i])
				return PR_FALSE;
		}
		for( i=0; i < kUnicodeBlockVarScriptMax; i++)
		{
			if(mPrivBlockToScript[i] != aMap.mPrivBlockToScript[i])
				return PR_FALSE;
		}
	}
	return PR_TRUE;
}

//--------------------------------------------------------------------------

short nsUnicodeFontMappingMac::GetFontID(PRUnichar aChar) {
	nsUnicodeBlock block = GetBlock(aChar);
	if(block < kUnicodeBlockFixedScriptMax) 
		return mScriptFallbackFontIDs[gUtil->BlockToScript(block)];
	else
		return  mScriptFallbackFontIDs[ mPrivBlockToScript[ block - kUnicodeBlockFixedScriptMax] ];
}

//------------------------------------------------------------------------
static const nsUnicodeBlock gU0xxxMap[32]=
{
 kBasicLatin, kLatin,    // U+0000
 kLatin,      kLatin,    // U+0100
 kOthers,     kOthers,   // U+0200
 kOthers,     kGreek,    // U+0300
 kCyrillic,   kCyrillic, // U+0400
 kArmenian,   kOthers,   // U+0500
 kArabic,     kArabic,   // U+0600
 kOthers,     kOthers,   // U+0700
 kOthers,     kOthers,   // U+0800
 kDevanagari, kBengali,  // U+0900
 kGurmukhi,   kGujarati, // U+0a00
 kOriya,      kTamil,    // U+0b00
 kTelugu,     kKannada,  // U+0c00
 kMalayalam,  kOthers ,  // U+0d00
 kThai,       kLao,      // U+0e00
 kTibetan,    kTibetan,  // U+0f00
};
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockU0XXX(PRUnichar aChar)
{
 nsUnicodeBlock res = gU0xxxMap[ (aChar >> 7) & 0x1F];
 if(res == kOthers) {
   if((0x0200 <= aChar) && ( aChar <= 0x024F ))           res =  kLatin;
   else if((0x0370 <= aChar) && ( aChar <= 0x037F ))      res =  kGreek;
   else if((0x0580 <= aChar) && ( aChar <= 0x058F ))      res =  kArmenian;
   else if((0x0590 <= aChar) && ( aChar <= 0x05ff ))      res =  kHebrew;
 } 
 return res;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockU1XXX(PRUnichar aChar)
{
  switch( aChar & 0x0F00 )
  {
     case 0x0000: return kGeorgian;
     case 0x0100: return kHangul;
     case 0x0e00: return kLatin;
     case 0x0f00: return kGreek;
     default:     return kOthers;
  }
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockU2XXX(PRUnichar aChar)
{
  return kOthers;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockU3XXX(PRUnichar aChar)
{
  if(aChar < 0x3040)        return kCJKMisc;
  else if(aChar < 0x3100)   return kHiraganaKatakana; 
  else if(aChar < 0x3130)   return kBopomofo; 
  else if(aChar < 0x3190)   return kHangul; 
  else if(aChar >= 0x3400)  return kCJKIdeographs; // Unicode 3.0
  else                      return kCJKMisc;
}

//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockCJKIdeographs(PRUnichar aChar)
{
  return  kCJKIdeographs;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockHangul(PRUnichar aChar)
{
  return  kHangul;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockUAXXX(PRUnichar aChar)
{
  if(aChar < 0xAC00) return  kOthers;
  else               return  kHangul;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockUDXXX(PRUnichar aChar)
{
  if(aChar < 0xD800) return  kHangul;
  else               return  kOthers;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockUEXXX(PRUnichar aChar)
{
  return  kOthers;
}
//--------------------------------------------------------------------------

static nsUnicodeBlock GetBlockUFXXX(PRUnichar aChar)
{
  // U+FFxx is used more frequently in the U+Fxxxx block
  if(aChar >= 0xff00) 
  {
    if(aChar < 0xff60)           return kCJKMisc;
    else if(aChar < 0xffA0)      return kHiraganaKatakana;
    else if(aChar < 0xffe0)      return kHangul;
    else                         return kOthers;    
  }

  // The rest is rarely used, we don't care the performance below.
  if((0xf780 <= aChar) && (aChar <= 0xf7ff)) return kUserDefinedEncoding;
  else if(aChar < 0xf900)        return kOthers;
  else if(aChar < 0xfb00)        return kCJKIdeographs;
  else if(aChar < 0xfb10)        return kLatin;
  else if(aChar < 0xfb18)        return kArmenian;
  else if(aChar < 0xfb50)        return kHebrew;
  else if(aChar < 0xfe20)        return kArabic;
  else if(aChar < 0xfe70)        return kOthers;
  else                           return kArabic;
}
//--------------------------------------------------------------------------

typedef nsUnicodeBlock (* getUnicodeBlock)(PRUnichar aChar);
static const getUnicodeBlock gAllGetBlock[16] = 
{
  &GetBlockU0XXX,          // 0
  &GetBlockU1XXX,          // 1
  &GetBlockU2XXX,          // 2
  &GetBlockU3XXX,          // 3
  &GetBlockCJKIdeographs,  // 4
  &GetBlockCJKIdeographs,  // 5
  &GetBlockCJKIdeographs,  // 6
  &GetBlockCJKIdeographs,  // 7
  &GetBlockCJKIdeographs,  // 8
  &GetBlockCJKIdeographs,  // 9
  &GetBlockUAXXX,          // a
  &GetBlockHangul,         // b
  &GetBlockHangul,         // c
  &GetBlockUDXXX,          // d
  &GetBlockUEXXX,          // e
  &GetBlockUFXXX           // f
};
//--------------------------------------------------------------------------

nsUnicodeBlock nsUnicodeFontMappingMac:: GetBlock(PRUnichar aChar)
{
   return (*(gAllGetBlock[(aChar >> 12)]))(aChar);
}
