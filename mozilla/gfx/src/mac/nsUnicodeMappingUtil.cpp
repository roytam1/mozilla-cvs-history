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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"
#include "nsUnicodeMappingUtil.h"
#include "nsUnicodeFontMappingCache.h"
#include "nsDeviceContextMac.h"
static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

#define BAD_FONT_NUM -1 
//--------------------------------------------------------------------------

nsUnicodeMappingUtil *nsUnicodeMappingUtil::gSingleton = nsnull;
//--------------------------------------------------------------------------

static nsIPref* gPref = nsnull;
static int gUnicodeMappingUtilCount = 0;

nsUnicodeMappingUtil::nsUnicodeMappingUtil()
{
	InitScriptEnabled();
	InitGenericFontMapping();
	InitFromPref();
	InitScriptFontMapping();
	InitBlockToScriptMapping(); // this must be called after InitScriptEnabled()
	gCache = new nsUnicodeFontMappingCache();
	++gUnicodeMappingUtilCount;
}
//--------------------------------------------------------------------------

nsUnicodeMappingUtil::~nsUnicodeMappingUtil()
{
	for(int i= 0 ; i < 32; i ++) {
		for(int j=0; j < 5; j++) {
			if(mGenericFontMapping[i][j])
	  		 nsString::Recycle(mGenericFontMapping[i][j]);
	  	}
	}
	if(gCache)
		delete gCache;

	if(0 == --gUnicodeMappingUtilCount)
		NS_IF_RELEASE(gPref);
}

//--------------------------------------------------------------------------

void nsUnicodeMappingUtil::InitScriptEnabled()
{
	PRUint8 numOfScripts = ::GetScriptManagerVariable(smEnabled);
	ScriptCode script, gotScripts;
	mScriptEnabled = 0;
	PRUint32 scriptMask = 1;
	for(script = gotScripts = 0; 
		((script < 32) && ( gotScripts < numOfScripts)) ; 
			script++, scriptMask <<= 1)
	{
		if(::GetScriptVariable(script, smScriptEnabled))
			mScriptEnabled |= scriptMask;
	}
}
//--------------------------------------------------------------------------

void nsUnicodeMappingUtil::InitGenericFontMapping()
{
	for(int i= 0 ; i < 32; i ++)
		for(int j=0; j < 5; j++)
	  		 mGenericFontMapping[i][j] = nsnull;

	// We probabaly should put the following info into resource ....
	// smRoman
	mGenericFontMapping[smRoman][kSerif]     = new nsAutoString( NS_ConvertToString("Times") );
	mGenericFontMapping[smRoman][kSansSerif] = new nsAutoString( NS_ConvertToString("Helvetica") ); // note: MRJ use Geneva for Sans-Serif
	mGenericFontMapping[smRoman][kMonospace] = new nsAutoString( NS_ConvertToString("Courier") );
	mGenericFontMapping[smRoman][kCursive]   =  new nsAutoString( NS_ConvertToString("Zapf Chancery") );
	mGenericFontMapping[smRoman][kFantasy]   = new nsAutoString( NS_ConvertToString("New Century Schlbk") );

	// smJapanese
	static PRUnichar jfontname1[] = {
	0x672C, 0x660E, 0x671D, 0x2212, 0xFF2D, 0x0000 //    �{�����|�l
	};
	static PRUnichar jfontname2[] = {
	0x4E38, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0x2212, 0xFF2D, 0x0000 //    �ۃS�V�b�N�|�l
	};
	static PRUnichar jfontname3[] = {
	0x004F, 0x0073, 0x0061, 0x006B, 0x0061, 0x2212, 0x7B49, 0x5E45, 0x0000 //    Osaka�|����
	};   

	mGenericFontMapping[smJapanese][kSerif]     = new nsAutoString(jfontname1);
	mGenericFontMapping[smJapanese][kSansSerif] = new nsAutoString(jfontname2); 
	mGenericFontMapping[smJapanese][kMonospace] = new nsAutoString(jfontname3);

	// smTradChinese
	mGenericFontMapping[smTradChinese][kSerif]     = new nsAutoString( NS_ConvertToString("Apple LiSung Light") );
	mGenericFontMapping[smTradChinese][kSansSerif] = new nsAutoString( NS_ConvertToString("Apple LiGothic Medium") );
	mGenericFontMapping[smTradChinese][kMonospace] = new nsAutoString( NS_ConvertToString("Apple LiGothic Medium") );

	// smKorean
	mGenericFontMapping[smKorean][kSerif]     = new nsAutoString( NS_ConvertToString("AppleMyungjo") );
	mGenericFontMapping[smKorean][kSansSerif] = new nsAutoString( NS_ConvertToString("AppleGothic") );
	mGenericFontMapping[smKorean][kMonospace] = new nsAutoString( NS_ConvertToString("AppleGothic") );

	// smArabic
	static PRUnichar afontname1[] = {
	0x062B, 0x0644, 0x062B, 0x202E, 0x0020, 0x202C, 0x0623, 0x0628, 0x064A, 0x0636, 0x0000 //    ��ˠ����
	};
	static PRUnichar afontname2[] = {
	0x0627, 0x0644, 0x0628, 0x064A, 0x0627, 0x0646, 0x0000 //    ������
	};
	static PRUnichar afontname3[] = {
	0x062C, 0x064A, 0x0632, 0x0629, 0x0000 //    ����
	};   
	mGenericFontMapping[smArabic][kSerif]     = new nsAutoString(afontname1);
	mGenericFontMapping[smArabic][kSansSerif] = new nsAutoString(afontname2);
	mGenericFontMapping[smArabic][kMonospace] = new nsAutoString(afontname3);

	// smHebrew
	static PRUnichar hfontname1[] = {
	0x05E4, 0x05E0, 0x05D9, 0x05E0, 0x05D9, 0x05DD, 0x202E, 0x0020, 0x202C, 0x05D7, 0x05D3, 0x05E9, 0x0000 //    ���������
	};
	static PRUnichar hfontname2[] = {
	0x05D0, 0x05E8, 0x05D9, 0x05D0, 0x05DC, 0x0000 //    �����
	};

	mGenericFontMapping[smHebrew][kSerif]     = new nsAutoString(hfontname1);
	mGenericFontMapping[smHebrew][kSansSerif] = new nsAutoString(hfontname2);
	mGenericFontMapping[smHebrew][kMonospace] = new nsAutoString(hfontname2);

	// smCyrillic
	mGenericFontMapping[smCyrillic][kSerif]     = new nsAutoString( NS_ConvertToString("Latinski") );
	mGenericFontMapping[smCyrillic][kSansSerif] = new nsAutoString( NS_ConvertToString("Pryamoy Prop") );
	mGenericFontMapping[smCyrillic][kMonospace] = new nsAutoString( NS_ConvertToString("APC Courier") );

	// smDevanagari
	mGenericFontMapping[smDevanagari][kSerif]     = new nsAutoString( NS_ConvertToString("Devanagari MT") );
	mGenericFontMapping[smDevanagari][kSansSerif] = new nsAutoString( NS_ConvertToString("Devanagari MT") );
	mGenericFontMapping[smDevanagari][kMonospace] = new nsAutoString( NS_ConvertToString("Devanagari MT") );

	// smGurmukhi
	static nsAutoString gurukhiMT_str( NS_ConvertToString("Gurmukhi MT") );
	mGenericFontMapping[smGurmukhi][kSerif]     = new nsAutoString( NS_ConvertToString("Gurmukhi MT") );
	mGenericFontMapping[smGurmukhi][kSansSerif] = new nsAutoString( NS_ConvertToString("Gurmukhi MT") );
	mGenericFontMapping[smGurmukhi][kMonospace] = new nsAutoString( NS_ConvertToString("Gurmukhi MT") );

	// smGujarati
	mGenericFontMapping[smGujarati][kSerif]     = new nsAutoString( NS_ConvertToString("Gujarati MT") );
	mGenericFontMapping[smGujarati][kSansSerif] = new nsAutoString( NS_ConvertToString("Gujarati MT") );
	mGenericFontMapping[smGujarati][kMonospace] = new nsAutoString( NS_ConvertToString("Gujarati MT") );

	// smThai
	mGenericFontMapping[smThai][kSerif]     = new nsAutoString( NS_ConvertToString("Thonburi") );
	mGenericFontMapping[smThai][kSansSerif] = new nsAutoString( NS_ConvertToString("Krungthep") );
	mGenericFontMapping[smThai][kMonospace] = new nsAutoString( NS_ConvertToString("Ayuthaya") );

	// smSimpChinese
	mGenericFontMapping[smSimpChinese][kSerif]     = new nsAutoString( NS_ConvertToString("Song") );
	mGenericFontMapping[smSimpChinese][kSansSerif] = new nsAutoString( NS_ConvertToString("Hei") );
	mGenericFontMapping[smSimpChinese][kMonospace] = new nsAutoString( NS_ConvertToString("Hei") );

	// smCentralEuroRoman
	mGenericFontMapping[smCentralEuroRoman][kSerif]     = new nsAutoString( NS_ConvertToString("Times CE") );
	mGenericFontMapping[smCentralEuroRoman][kSansSerif] = new nsAutoString( NS_ConvertToString("Helvetica CE") );
	mGenericFontMapping[smCentralEuroRoman][kMonospace] = new nsAutoString( NS_ConvertToString("Courier CE") );
}
//--------------------------------------------------------------------------

ScriptCode nsUnicodeMappingUtil::MapLangGroupToScriptCode(const char* aLangGroup)
{
	if(0==nsCRT::strcmp(aLangGroup,  "x-western")) {
		return smRoman;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "x-central-euro")) {
		return smCentralEuroRoman;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "x-cyrillic")) {
		return smCyrillic;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "el")) {
		return smGreek;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "tr")) {
		return smRoman;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "he")) {
		return smHebrew;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "ar")) {
		return smArabic;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "x-baltic")) {
		return smRoman;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "th")) {
		return smThai;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "ja")) {
		return smJapanese;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "zh-CN")) {
		return smSimpChinese;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "ko")) {
		return smKorean;
	} else 
	if(0==nsCRT::strcmp(aLangGroup,  "zh-TW")) {
		return smTradChinese;
	} else 
	{
		return smRoman;
	}
}

#define FACESIZE 255 // font name is Str255 in Mac script code
void
PrefEnumCallback(const char* aName, void* aClosure)
{
	
	nsUnicodeMappingUtil* Self = (nsUnicodeMappingUtil*)aClosure;
  nsCAutoString curPrefName(aName);
  
  PRInt32 p1 = curPrefName.RFindChar('.', PR_TRUE);
  if(-1==p1)
  	return;
  PRInt32 p2 = curPrefName.RFindChar('.', PR_TRUE, p1-1);
  if(-1==p1)
  	return;
  
  nsCAutoString genName("");
  nsCAutoString langGroup("");
  
  curPrefName.Mid(langGroup,  p1+1, curPrefName.Length()-p1-1);
  curPrefName.Mid(genName, p2+1, p1-p2-1);
  
  if(langGroup.Equals(nsCAutoString("x-unicode")))
  	return;
  ScriptCode script = Self->MapLangGroupToScriptCode(langGroup);
  if(script >= smUninterp)
  	return;
  
  nsString genNameString; genNameString.AssignWithConversion(genName);
  nsGenericFontNameType type = Self->MapGenericFontNameType(genNameString);
  if(type >= kUknownGenericFontName)
  	return;
  	
  char* valueInUTF8 = nsnull;
  gPref->CopyCharPref(aName, &valueInUTF8);
  if((nsnull == valueInUTF8) || (PL_strlen(valueInUTF8) == 0))
  {
	  Recycle(valueInUTF8);
  	return;
  }
  PRUnichar valueInUCS2[FACESIZE]= { 0 };
  PRUnichar format[] = { '%', 's', 0 };
  PRUint32 n = nsTextFormatter::snprintf(valueInUCS2, FACESIZE, format, valueInUTF8);
  Recycle(valueInUTF8);
  if(n == 0)
  	return;     
  nsString *fontname = new nsAutoString(valueInUCS2);
  if(nsnull == fontname)
  	return;
  short fontID=0;
  if( (! nsDeviceContextMac::GetMacFontNumber(*fontname, fontID)) ||
  	  ( ::FontToScript(fontID) != script ))
  {
  	nsString::Recycle(fontname);
  	return;
  }
  if( Self->mGenericFontMapping[script][type] )
  	nsString::Recycle(Self->mGenericFontMapping[script][type]);
  Self->mGenericFontMapping[script][type] = fontname;
}
void nsUnicodeMappingUtil::InitFromPref()
{
  if (!gPref) {
    nsServiceManager::GetService(kPrefCID,
      NS_GET_IID(nsIPref), (nsISupports**) &gPref);
    if (!gPref) {
      return;
    }
  }	  
  gPref->EnumerateChildren("font.name.", PrefEnumCallback, this);
}
//--------------------------------------------------------------------------

void nsUnicodeMappingUtil::InitScriptFontMapping()
{
	// Get font from Script manager
	for(ScriptCode script = 0; script< 32 ; script++)
	{
		mScriptFontMapping[script] = BAD_FONT_NUM;			
		short fontNum;
		if(ScriptEnabled(script)) {
			long fondsize = ::GetScriptVariable(script, smScriptPrefFondSize);
			if((fondsize) && ((fondsize >> 16))) {
				fontNum = (fondsize >> 16);
				mScriptFontMapping[script] = fontNum;
			}
		}
	}
}
//--------------------------------------------------------------------------
void nsUnicodeMappingUtil::InitBlockToScriptMapping()
{
	static ScriptCode prebuildMapping[kUnicodeBlockSize] = 
	{
		// start the fixed section
		smGreek, 	smCyrillic,		smArmenian, 
		smHebrew, 	smArabic, 	smDevanagari,	smBengali, 	
		smGurmukhi, smGujarati, smOriya, 		smTamil, 	
		smTelugu, 	smKannada,	smMalayalam,	smThai,
		smLao,		smTibetan,	smGeorgian,		smKorean,
		smTradChinese,
		
		// start the variable section
		smRoman, 	smRoman,	smJapanese,	smJapanese,		smJapanese,
		smRoman
	};
	for(PRUint32 i= 0; i < kUnicodeBlockSize; i++) 
	{
		if( ScriptEnabled(prebuildMapping[i]) ) {
			mBlockToScriptMapping[i] = prebuildMapping[i];
		} else {
			if(i < kUnicodeBlockFixedScriptMax) {
				mBlockToScriptMapping[i] = (PRInt8) smRoman;
			} else {
				if( (kCJKMisc == i) || (kHiraganaKatakana == i) || (kCJKIdeographs == i) ) {
					// fallback in the following sequence
					// smTradChinese  smKorean smSimpChinese
					mBlockToScriptMapping[i] = (PRInt8)
						( ScriptEnabled(smTradChinese) ? smTradChinese :
							( ScriptEnabled(smKorean) ? smKorean :
								( ScriptEnabled(smSimpChinese) ? smSimpChinese : smRoman )												
							)					
						);
				}				
			}
		}
	}
}

//--------------------------------------------------------------------------
nsGenericFontNameType nsUnicodeMappingUtil::MapGenericFontNameType(const nsString& aGenericName)
{
	if (aGenericName.EqualsIgnoreCase("serif"))
	  return kSerif;
	if (aGenericName.EqualsIgnoreCase("sans-serif"))
	  return kSansSerif;
	if (aGenericName.EqualsIgnoreCase("monospace"))
	  return kMonospace;
	if (aGenericName.EqualsIgnoreCase("-moz-fixed"))
	  return kMonospace;
	if (aGenericName.EqualsIgnoreCase("cursive"))
	  return kCursive;
	if (aGenericName.EqualsIgnoreCase("fantasy"))
	  return kFantasy;
	  
	return kUknownGenericFontName;			
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

nsUnicodeMappingUtil* nsUnicodeMappingUtil::GetSingleton()
{
	if( ! gSingleton)
		gSingleton = new nsUnicodeMappingUtil();
	return gSingleton;
}
//--------------------------------------------------------------------------
