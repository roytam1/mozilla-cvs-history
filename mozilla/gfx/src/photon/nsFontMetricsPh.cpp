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
 */

#include "xp_core.h"
#include "nsQuickSort.h"
#include "nsFontMetricsPh.h"
#include "nsPhGfxLog.h"
#include "nsHashtable.h"
#include "nslog.h"

NS_IMPL_LOG(nsFontMetricsPhLog)
#define PRINTF(args) NS_LOG_PRINTF(nsFontMetricsPhLog, args)
#define FLUSH()      NS_LOG_FLUSH(nsFontMetricsPhLog)

#include <errno.h>

static int gGotAllFontNames = 0;

// XXX many of these statics need to be freed at shutdown time
static PLHashTable* gFamilies = nsnull;

static nsHashtable* gFontMetricsCache = nsnull;
static int gFontMetricsCacheCount = 0;

static NS_DEFINE_IID(kIFontMetricsIID, NS_IFONT_METRICS_IID);

nsFontMetricsPh :: nsFontMetricsPh()
{
  NS_INIT_REFCNT();
  mDeviceContext = nsnull;
  mFont = nsnull;
	  
  mHeight = 0;
  mAscent = 0;
  mDescent = 0;
  mLeading = 0;
  mEmHeight = 0;
  mEmAscent = 0;
  mEmDescent = 0;
  mMaxHeight = 0;
  mMaxAscent = 0;
  mMaxDescent = 0;
  mMaxAdvance = 0;
  mXHeight = 0;
  mSuperscriptOffset = 0;
  mSubscriptOffset = 0;
  mStrikeoutSize = 0;
  mStrikeoutOffset = 0;
  mUnderlineSize = 0;
  mUnderlineOffset = 0;
  mSpaceWidth = 0;
  mAveCharWidth = 0;
}

static void InitGlobals()
{
	gFontMetricsCache = new nsHashtable();
}

static PRBool
FreeFontMetricsCache(nsHashKey* aKey, void* aData, void* aClosure)
{
  FontQueryInfo * node = (FontQueryInfo*) aData;

  delete node;

  return PR_TRUE;
}

static void FreeGlobals()
{
	if (gFontMetricsCache)
	{
		gFontMetricsCache->Reset(FreeFontMetricsCache, nsnull);
		delete gFontMetricsCache;
		gFontMetricsCache = nsnull;
		gFontMetricsCacheCount = 0;

	}
}
  
nsFontMetricsPh :: ~nsFontMetricsPh()
{
  PR_LOG(PhGfxLog, PR_LOG_DEBUG, ("nsFontMetricsPh::~nsFontMetricsPh Destructor called\n"));

  if (nsnull != mFont)
  {
    delete mFont;
    mFont = nsnull;
  }

  	mDeviceContext = nsnull;

//  	gFontMetricsCacheCount--;
//  	if (gFontMetricsCacheCount == 0)
//  		FreeGlobals();
}


NS_IMPL_ISUPPORTS1(nsFontMetricsPh, nsIFontMetrics)

NS_IMETHODIMP
nsFontMetricsPh :: Init ( const nsFont& aFont, nsIAtom* aLangGroup,
                          nsIDeviceContext* aContext )
{
  NS_ASSERTION(!(nsnull == aContext), "attempt to init fontmetrics with null device context");

  nsAutoString  firstFace;
  char          *str = nsnull;
  nsresult      result;  	
  nsresult      ret_code = NS_ERROR_FAILURE;
  int           MAX_FONTDETAIL = 50;
  FontDetails   fDetails[MAX_FONTDETAIL];
  int           fontcount;
  int           index;
  PhRect_t      extent;


	result = aContext->FirstExistingFont(aFont, firstFace);

	str = firstFace.ToNewCString();

	if (NS_OK != result)
	{
		aFont.GetFirstFamily(firstFace);
		char *str = nsnull;
		str = firstFace.ToNewCString();
		delete [] str;
	}

	if (!str || !str[0])
	{
		delete [] str;
		str = strdup("serif");
	}

	mFont = new nsFont(aFont);
	mLangGroup = aLangGroup;
	mDeviceContext = (nsDeviceContextPh *) aContext;

	float app2dev, app2twip, scale = 1.0, textZoom = 1.0;

	mDeviceContext->GetAppUnitsToDevUnits(app2dev);
	mDeviceContext->GetDevUnitsToTwips(app2twip);
	mDeviceContext->GetCanonicalPixelScale(scale);
	mDeviceContext->GetTextZoom(textZoom);

	//PRInt32 sizePoints2 = NSToIntRound(app2dev * textZoom * mFont->size);
	app2twip *= (app2dev * textZoom);
	PRInt32 sizePoints = NSTwipsToFloorIntPoints(nscoord(mFont->size * app2twip * 0.90));
  
	//PRINTF(("FONTSIZE: %f, %f, %f, %f, %d, %d (%d)\n", app2dev, app2twip, scale, textZoom, \
	//	mFont->size, sizePoints, sizePoints2);

	char NSFontName[64];	/* Local buffer to keep the fontname in */
	char NSFontSuffix[5];
	char NSFullFontName[MAX_FONT_TAG];
    	
	NSFontSuffix[0] = nsnull;

	unsigned int uiFlags = 0L;

	if(aFont.weight > NS_FONT_WEIGHT_NORMAL)
		uiFlags |= PF_STYLE_BOLD;

	if(aFont.style & NS_FONT_STYLE_ITALIC)
		uiFlags |= PF_STYLE_ITALIC;

	if(aFont.style & NS_FONT_STYLE_OBLIQUE)
		uiFlags |= PF_STYLE_ANTIALIAS;

	if(PfGenerateFontName((const uchar_t *)str, uiFlags, sizePoints, (uchar_t *)NSFullFontName) == NULL)
	{
		NS_WARNING("nsFontMetricsPh::Init Name generate failed");
		if (PfGenerateFontName((const uchar_t *)"Courier 10 Pitch BT", uiFlags, sizePoints, (uchar_t *)NSFullFontName) == NULL)
		{
  		  NS_ASSERTION(0,"nsFontMetricsPh::Init Name generate failed for default font\n");
        PRINTF((" nsFontMetricsPh::Init Name generate failed for default font:  %s, %d, %d\n", str, uiFlags, sizePoints));
 		}
	}

	/* Once the Photon Font String is built get the attributes */
	FontQueryInfo *node;
	int ret;

	if (!gFontMetricsCacheCount)
		InitGlobals();

	//nsStringKey key((char *)(NSFullFontName));
	nsCStringKey key((char *)(NSFullFontName));
	node = (FontQueryInfo *) gFontMetricsCache->Get(&key);
	if (!node)
	{
		node = (FontQueryInfo *)calloc(sizeof(FontQueryInfo), 1);
		PfQueryFont(NSFullFontName, node);
		gFontMetricsCache->Put(&key, node);
		gFontMetricsCacheCount++;
	}

	float dev2app;
	int height;
	nscoord onePixel;

	mDeviceContext->GetDevUnitsToAppUnits(dev2app);
	onePixel = NSToCoordRound(1 * dev2app);
	height = node->descender - node->ascender + 1.0;
	PfExtentText(&extent, NULL, NSFullFontName, " ", 1);
	mSpaceWidth = (int) ((extent.lr.x - extent.ul.x + 1) * dev2app);

	mLeading = NSToCoordRound(0);
	mEmHeight = NSToCoordRound(height * dev2app);
	mEmAscent = NSToCoordRound(node->ascender * dev2app * -1.0);
	mEmDescent = NSToCoordRound(node->descender * dev2app);
	mHeight = mMaxHeight = NSToCoordRound(height * dev2app);
	mAscent = mMaxAscent = NSToCoordRound(node->ascender * dev2app * -1.0);
	mDescent = mMaxDescent = NSToCoordRound(node->descender * dev2app);
	mMaxAdvance = NSToCoordRound(node->width * dev2app);
	mAveCharWidth = PR_MAX(1, NSToCoordRound(mSpaceWidth * dev2app));

	mXHeight = NSToCoordRound((float)node->ascender * dev2app * 0.56f * -1.0); // 56% of ascent, best guess for non-true type
	mSuperscriptOffset = mXHeight;     // XXX temporary code!
	mSubscriptOffset = mXHeight;     // XXX temporary code!

	mStrikeoutSize = onePixel; // XXX this is a guess
	mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0f); // 50% of xHeight
	mUnderlineSize = onePixel; // XXX this is a guess
	mUnderlineOffset = -NSToCoordRound((float)node->descender * dev2app * 0.30f); // 30% of descent

	mFontHandle = strdup(NSFullFontName); /* memory leak */

	delete [] str;
	return NS_OK;
})

NS_IMETHODIMP
nsFontMetricsPh :: Destroy()
{
  mDeviceContext = nsnull;
  return NS_OK;
}

static void
apGenericFamilyToFont(const nsString& aGenericFamily,
                       nsIDeviceContext* aDC,
                       nsString& aFontFace)
{
  char *str = aGenericFamily.ToNewCString();
  delete [] str;
}

struct FontEnumData
{
  FontEnumData(nsIDeviceContext* aContext, char* aFaceName)
  {
    mContext = aContext;
    mFaceName = aFaceName;
  }
  nsIDeviceContext* mContext;
  char* mFaceName;
};

static PRBool
FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  return PR_TRUE;
}

void
nsFontMetricsPh::RealizeFont()
{
}

NS_IMETHODIMP
nsFontMetricsPh :: GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutOffset;
  aSize = mStrikeoutSize;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlineOffset;
  aSize = mUnderlineSize;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetHeight(nscoord &aHeight)
{
  aHeight = mHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh ::GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsPh::GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsPh::GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsPh::GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsPh::GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh :: GetFont(const nsFont *&aFont)
{
  aFont = mFont;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsPh::GetLangGroup(nsIAtom** aLangGroup)
{
  if (!aLangGroup) {
    return NS_ERROR_NULL_POINTER;
  }

  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);

  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPh::GetFontHandle(nsFontHandle &aHandle)
{
  aHandle = (nsFontHandle) mFontHandle;
  return NS_OK;
}

nsresult
nsFontMetricsPh::GetSpaceWidth(nscoord &aSpaceWidth)
{
  aSpaceWidth = mSpaceWidth;
  return NS_OK;
}

struct nsFontFamily
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  PLHashTable* mCharSets;
};


// The Font Enumerator

nsFontEnumeratorPh::nsFontEnumeratorPh()
{
  NS_INIT_REFCNT();
}

NS_IMPL_ISUPPORTS(nsFontEnumeratorPh, NS_GET_IID(nsIFontEnumerator));

static int gInitializedFontEnumerator = 0;
static PLHashNumber HashKey(const void* aString)
{
  const nsString* key = (const nsString*) aString;
  return (PLHashNumber)
    nsCRT::HashCode(key->GetUnicode());
    //nsCRT::HashCode(key->GetUnicode(), key->Length());
}

static PRIntn
CompareKeys(const void* aStr1, const void* aStr2)
{
  return nsCRT::strcmp(((const nsString*) aStr1)->GetUnicode(),
    ((const nsString*) aStr2)->GetUnicode()) == 0;
}

static int
InitializeFontEnumerator(void)
{
  gInitializedFontEnumerator = 1;

  if (!gGotAllFontNames)
  {
    gGotAllFontNames = 1;
  }

  return 1;
}

typedef struct EnumerateFamilyInfo
{
  PRUnichar** mArray;
  int         mIndex;
} EnumerateFamilyInfo;

static PRIntn
EnumerateFamily(PLHashEntry* he, PRIntn i, void* arg)
{
  EnumerateFamilyInfo* info = (EnumerateFamilyInfo*) arg;
  PRUnichar** array = info->mArray;
  int j = info->mIndex;
  PRUnichar* str = ((nsString*) he->key)->ToNewUnicode();


  if (!str) {
    for (j = j - 1; j >= 0; j--) {
      nsMemory::Free(array[j]);
    }
    info->mIndex = 0;
    return HT_ENUMERATE_STOP;
  }
  array[j] = str;
  info->mIndex++;

  return HT_ENUMERATE_NEXT;
}

static int
CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure)
{
  const PRUnichar* str1 = *((const PRUnichar**) aArg1);
  const PRUnichar* str2 = *((const PRUnichar**) aArg2);

  // XXX add nsICollation stuff

  return nsCRT::strcmp(str1, str2);
}

NS_IMETHODIMP
nsFontEnumeratorPh::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{

  if (aCount) {
    *aCount = 0;
  }
  else {
    return NS_ERROR_NULL_POINTER;
  }
  if (aResult) {
    *aResult = nsnull;
  }
  else {
    return NS_ERROR_NULL_POINTER;
  }

  if (!gInitializedFontEnumerator) {
    if (!InitializeFontEnumerator()) {
      return NS_ERROR_FAILURE;
    }
  }


  if (gFamilies)
  {
    PRUnichar** array = (PRUnichar**) nsMemory::Alloc(gFamilies->nentries * sizeof(PRUnichar*));
    if (!array)
    {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    EnumerateFamilyInfo info = { array, 0 };
    PL_HashTableEnumerateEntries(gFamilies, EnumerateFamily, &info);
    if (!info.mIndex)
    {
      nsMemory::Free(array);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_QuickSort(array, gFamilies->nentries, sizeof(PRUnichar*),
    CompareFontNames, nsnull);

    *aCount = gFamilies->nentries;
    *aResult = array;


    return NS_OK;
  }
  else
  {
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP
nsFontEnumeratorPh::EnumerateFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  if ((!aLangGroup) || (!aGeneric)) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aCount) {
    *aCount = 0;
  }
  else {
    return NS_ERROR_NULL_POINTER;
  }
  if (aResult) {
    *aResult = nsnull;
  }
  else {
    return NS_ERROR_NULL_POINTER;
  }

  if ((!strcmp(aLangGroup, "x-unicode")) ||
      (!strcmp(aLangGroup, "x-user-def"))) {
    return EnumerateAllFonts(aCount, aResult);
  }

  if (!gInitializedFontEnumerator) {
    if (!InitializeFontEnumerator()) {
      return NS_ERROR_FAILURE;
    }
  }

  // XXX still need to implement aLangGroup and aGeneric
  return EnumerateAllFonts(aCount, aResult);
}
