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

#include "nsDeviceContext.h"
#include "nsIFontCache.h"
#include "nsFont.h"
#include "nsIView.h"
#include "nsGfxCIID.h"
#include "nsImageNet.h"
#include "nsImageRequest.h"
#include "nsIImageGroup.h"
#include "il_util.h"

static NS_DEFINE_IID(kDeviceContextIID, NS_IDEVICE_CONTEXT_IID);

NS_IMPL_QUERY_INTERFACE(DeviceContextImpl, kDeviceContextIID)
NS_IMPL_ADDREF(DeviceContextImpl)
NS_IMPL_RELEASE(DeviceContextImpl)

DeviceContextImpl :: DeviceContextImpl()
{
  NS_INIT_REFCNT();
  mFontCache = nsnull;
  mDevUnitsToAppUnits = 1.0f;
  mAppUnitsToDevUnits = 1.0f;
  mGammaValue = 1.0f;
  mGammaTable = new PRUint8[256];
  mZoom = 1.0f;
  mWidget = nsnull;
  mIconImageGroup = nsnull;
  for (PRInt32 i = 0; i < NS_NUMBER_OF_ICONS; i++) {
    mIcons[i] = nsnull;
  }
  mFontAliasTable = nsnull;
  mColorSpace = nsnull;
}

static PRBool DeleteValue(nsHashKey* aKey, void* aValue, void* closure)
{
  delete ((nsString*)aValue);
  return PR_TRUE;
}


DeviceContextImpl :: ~DeviceContextImpl()
{
  NS_IF_RELEASE(mFontCache);

  if (nsnull != mGammaTable)
  {
    delete mGammaTable;
    mGammaTable = nsnull;
  }

  for (PRInt32 i = 0; i < NS_NUMBER_OF_ICONS; i++) {
    NS_IF_RELEASE(mIcons[i]);
  }

  /*
   * Destroy the GroupContext after releasing the ImageRequests
   * since IL_DestroyGroupContext(...) will destroy any IL_ImageReq
   * for the context.  These are the same IL_ImgReq being referenced
   * by mIcons[...]
   */
  IL_DestroyGroupContext(mIconImageGroup);

  if (nsnull != mFontAliasTable) {
    mFontAliasTable->Enumerate(DeleteValue);
    delete mFontAliasTable;
  }

  if (nsnull != mColorSpace) {
    IL_ReleaseColorSpace(mColorSpace);
  }
}

NS_IMETHODIMP DeviceContextImpl :: Init(nsNativeWidget aWidget)
{
  for (PRInt32 cnt = 0; cnt < 256; cnt++)
    mGammaTable[cnt] = cnt;

  mWidget = aWidget;

  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetTwipsToDevUnits(float &aTwipsToDevUnits) const
{
  aTwipsToDevUnits = mTwipsToPixels;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetDevUnitsToTwips(float &aDevUnitsToTwips) const
{
  aDevUnitsToTwips = mPixelsToTwips;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: SetAppUnitsToDevUnits(float aAppUnits)
{
  mAppUnitsToDevUnits = aAppUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: SetDevUnitsToAppUnits(float aDevUnits)
{
  mDevUnitsToAppUnits = aDevUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetAppUnitsToDevUnits(float &aAppUnits) const
{
  aAppUnits = mAppUnitsToDevUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetDevUnitsToAppUnits(float &aDevUnits) const
{
  aDevUnits = mDevUnitsToAppUnits;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext)
{
  nsIRenderingContext *pContext;
  nsIWidget           *win;
  aView->GetWidget(win);
  nsresult             rv;

  static NS_DEFINE_IID(kRCCID, NS_RENDERING_CONTEXT_CID);
  static NS_DEFINE_IID(kRCIID, NS_IRENDERING_CONTEXT_IID);

  aContext = nsnull;
  rv = nsRepository::CreateInstance(kRCCID, nsnull, kRCIID, (void **)&pContext);

  if (NS_OK == rv) {
    rv = InitRenderingContext(pContext, win);
    if (NS_OK != rv) {
      NS_RELEASE(pContext);
    }
  }

  NS_IF_RELEASE(win);
  aContext = pContext;
  return rv;
}

NS_IMETHODIMP DeviceContextImpl :: InitRenderingContext(nsIRenderingContext *aContext, nsIWidget *aWin)
{
  return (aContext->Init(this, aWin));
}

NS_IMETHODIMP DeviceContextImpl::GetFontCache(nsIFontCache*& aCache)
{
  nsresult  rv;

  if (nsnull == mFontCache) {
    rv = CreateFontCache();
    if (NS_FAILED(rv)) {
      aCache = nsnull;
      return rv;
    }
  }
  NS_ADDREF(mFontCache);
  aCache = mFontCache;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::FlushFontCache()
{
  NS_IF_RELEASE(mFontCache);
  return NS_OK;
}

nsresult DeviceContextImpl::CreateFontCache()
{
  nsresult rv = NS_NewFontCache(&mFontCache);
  if (NS_OK != rv) {
    return rv;
  }
  mFontCache->Init(this);
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetMetricsFor(const nsFont& aFont, nsIFontMetrics*& aMetrics)
{
  if (nsnull == mFontCache) {
    nsresult  rv = CreateFontCache();
    if (NS_FAILED(rv)) {
      aMetrics = nsnull;
      return rv;
    }
  }
  return mFontCache->GetMetricsFor(aFont, aMetrics);
}

NS_IMETHODIMP DeviceContextImpl :: SetZoom(float aZoom)
{
  mZoom = aZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetZoom(float &aZoom) const
{
  aZoom = mZoom;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetGamma(float &aGamma)
{
  aGamma = mGammaValue;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: SetGamma(float aGamma)
{
  if (aGamma != mGammaValue)
  {
    //we don't need to-recorrect existing images for this case
    //so pass in 1.0 for the current gamma regardless of what it
    //really happens to be. existing images will get a one time
    //re-correction when they're rendered the next time. MMP

    SetGammaTable(mGammaTable, 1.0f, aGamma);

    mGammaValue = aGamma;
  }
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl :: GetGammaTable(PRUint8 *&aGammaTable)
{
  //XXX we really need to ref count this somehow. MMP
  aGammaTable = mGammaTable;
  return NS_OK;
}

void DeviceContextImpl :: SetGammaTable(PRUint8 * aTable, float aCurrentGamma, float aNewGamma)
{
  double fgval = (1.0f / aCurrentGamma) * (1.0f / aNewGamma);

  for (PRInt32 cnt = 0; cnt < 256; cnt++)
    aTable[cnt] = (PRUint8)(pow((double)cnt * (1. / 256.), fgval) * 255.99999999);
}

NS_IMETHODIMP DeviceContextImpl :: GetNativeWidget(nsNativeWidget &aNativeWidget)
{
  aNativeWidget = mWidget;
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetDepth(PRUint32& aDepth)
{
  aDepth = 24;
  return NS_OK;
}

nsresult DeviceContextImpl::CreateIconILGroupContext()
{
  ilIImageRenderer* renderer;
  nsresult          result;
   
  // Create an image renderer
  result = NS_NewImageRenderer(&renderer);
  if (NS_FAILED(result)) {
    return result;
  }

  // Create an image group context. The image renderer code expects the
  // display_context to be a pointer to a device context
  mIconImageGroup = IL_NewGroupContext((void*)this, renderer);
  if (nsnull == mIconImageGroup) {
    NS_RELEASE(renderer);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // Initialize the image group context.
  IL_ColorSpace* colorSpace;
  result = GetILColorSpace(colorSpace);
  if (NS_FAILED(result)) {
    NS_RELEASE(renderer);
    IL_DestroyGroupContext(mIconImageGroup);
    return result;
  }

  // Icon loading is synchronous, so don't waste time with progressive display
  IL_DisplayData displayData;
  displayData.dither_mode = IL_Auto;
  displayData.color_space = colorSpace;
  displayData.progressive_display = PR_FALSE;
  IL_SetDisplayMode(mIconImageGroup, IL_COLOR_SPACE | IL_DITHER_MODE, &displayData);
  IL_ReleaseColorSpace(colorSpace);

  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::LoadIconImage(PRInt32 aId, nsIImage*& aImage)
{
  nsresult  result;

  // Initialize out parameter
  aImage = nsnull;

  // Make sure the icon number is valid
  if ((aId < 0) || (aId >= NS_NUMBER_OF_ICONS)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  // See if the icon is already loaded
  if (nsnull != mIcons[aId]) {
    aImage = mIcons[aId]->GetImage();
    return NS_OK;
  }

  // Make sure we have an image group context
  if (nsnull == mIconImageGroup) {
    result = CreateIconILGroupContext();
    if (NS_FAILED(result)) {
      return result;
    }
  }

  // Build the URL string
  char  url[128];
  sprintf(url, "resource://res/gfx/icon_%d.gif", aId);

  // Use a sync net context
  ilINetContext* netContext;
  result = NS_NewImageNetContextSync(&netContext);
  if (NS_FAILED(result)) {
    return result;
  }

  // Create an image request object which will do the actual load
  ImageRequestImpl* imageReq = new ImageRequestImpl();
  if (nsnull == imageReq) {
    result = NS_ERROR_OUT_OF_MEMORY;

  } else {
    // Load the image
    result = imageReq->Init(mIconImageGroup, url, nsnull, nsnull, 0, 0,
                            nsImageLoadFlags_kHighPriority, netContext);
    aImage = imageReq->GetImage();

    // Keep the image request object around and avoid reloading the image
    NS_ADDREF(imageReq);
    mIcons[aId] = imageReq;
  }
  
  NS_RELEASE(netContext);
  return result;
}

struct FontEnumData {
  FontEnumData(nsIDeviceContext* aDC, nsString& aFaceName)
    : mDC(aDC), mFaceName(aFaceName)
  {}
  nsIDeviceContext* mDC;
  nsString&         mFaceName;
};

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  FontEnumData* data = (FontEnumData*)aData;
  // XXX for now, all generic fonts are presumed to exist
  //     we may want to actually check if there's an installed conversion
  if (aGeneric) {
    data->mFaceName = aFamily;
    return PR_FALSE; // found one, stop.
  }
  else {
    nsAutoString  local;
    PRBool        aliased;
    data->mDC->GetLocalFontName(aFamily, local, aliased);
    if (aliased || (NS_OK == data->mDC->CheckFontExistence(local))) {
      data->mFaceName = local;
      return PR_FALSE; // found one, stop.
    }
  }
  return PR_TRUE; // didn't exist, continue looking
}

NS_IMETHODIMP DeviceContextImpl::FirstExistingFont(const nsFont& aFont, nsString& aFaceName)
{
  FontEnumData  data(this, aFaceName);
  if (aFont.EnumerateFamilies(FontEnumCallback, &data)) {
    return NS_ERROR_FAILURE;  // ran out
  }
  return NS_OK;
}

class StringKey: public nsHashKey 
{
public:
  StringKey(const nsString& aString)
    : mString(aString)
  {}

  virtual PRUint32 HashValue(void) const;
  virtual PRBool Equals(const nsHashKey *aKey) const;
  virtual nsHashKey *Clone(void) const;

  nsAutoString  mString;
};

PRUint32 StringKey::HashValue(void) const
{
  PRUint32 hash = 0;
  PRUnichar*  string = mString;
  PRUnichar ch;
  while ((ch = *string++) != 0) {
    // FYI: hash = hash*37 + ch
    ch = nsCRT::ToUpper(ch);
    hash = ((hash << 5) + (hash << 2) + hash) + ch;
  }
  return hash;
}

PRBool StringKey::Equals(const nsHashKey *aKey) const
{
  return mString.EqualsIgnoreCase(((StringKey*)aKey)->mString);
}

nsHashKey* StringKey::Clone(void) const
{
  return new StringKey(mString);
}

static nsAutoString  gTimes("Times");
static nsAutoString  gTimesNewRoman("Times New Roman");
static nsAutoString  gTimesRoman("Times Roman");
static nsAutoString  gArial("Arial");
static nsAutoString  gHelvetica("Helvetica");
static nsAutoString  gCourier("Courier");
static nsAutoString  gCourierNew("Courier New");
static nsAutoString  gUnicode("Unicode");
static nsAutoString  gBitstreamCyberbit("Bitstream Cyberbit");
static nsAutoString  gNullStr;

nsresult DeviceContextImpl::CreateFontAliasTable()
{
  nsresult result = NS_OK;

  if (nsnull == mFontAliasTable) {
    mFontAliasTable = new nsHashtable();
    if (nsnull != mFontAliasTable) {
      AliasFont(gTimes, gTimesNewRoman, gTimesRoman, PR_FALSE);
      AliasFont(gTimesRoman, gTimesNewRoman, gTimes, PR_FALSE);
      AliasFont(gTimesNewRoman, gTimesRoman, gTimes, PR_FALSE);
      AliasFont(gArial, gHelvetica, gNullStr, PR_FALSE);
      AliasFont(gHelvetica, gArial, gNullStr, PR_FALSE);
      AliasFont(gCourier, gCourierNew, gNullStr, PR_TRUE);
      AliasFont(gCourierNew, gCourier, gNullStr, PR_FALSE);
      AliasFont(gUnicode, gBitstreamCyberbit, gNullStr, PR_FALSE); // XXX ????
    }
    else {
      result = NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return result;
}

nsresult DeviceContextImpl::AliasFont(const nsString& aFont, 
                                      const nsString& aAlias, const nsString& aAltAlias,
                                      PRBool aForceAlias)
{
  nsresult result = NS_OK;

  if (nsnull != mFontAliasTable) {
    if (aForceAlias || (NS_OK != CheckFontExistence(aFont))) {
      if (NS_OK == CheckFontExistence(aAlias)) {
        nsString* entry = aAlias.ToNewString();
        if (nsnull != entry) {
          StringKey key(aFont);
          mFontAliasTable->Put(&key, entry);
        }
        else {
          result = NS_ERROR_OUT_OF_MEMORY;
        }
      }
      else if ((0 < aAltAlias.Length()) && (NS_OK == CheckFontExistence(aAltAlias))) {
        nsString* entry = aAltAlias.ToNewString();
        if (nsnull != entry) {
          StringKey key(aFont);
          mFontAliasTable->Put(&key, entry);
        }
        else {
          result = NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
  }
  else {
    result = NS_ERROR_FAILURE;
  }
  return result;
}

NS_IMETHODIMP DeviceContextImpl::GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                                                  PRBool& aAliased)
{
  nsresult result = NS_OK;

  if (nsnull == mFontAliasTable) {
    result = CreateFontAliasTable();
  }

  if (nsnull != mFontAliasTable) {
    StringKey key(aFaceName);
    const nsString* alias = (const nsString*)mFontAliasTable->Get(&key);
    if (nsnull != alias) {
      aLocalName = *alias;
      aAliased = PR_TRUE;
    }
    else {
      aLocalName = aFaceName;
      aAliased = PR_FALSE;
    }
  }
  return result;
}

NS_IMETHODIMP DeviceContextImpl::GetILColorSpace(IL_ColorSpace*& aColorSpace)
{
  if (nsnull == mColorSpace) {
    IL_RGBBits colorRGBBits;
  
    // Default is to create a 24-bit color space
    colorRGBBits.red_shift = 16;  
    colorRGBBits.red_bits = 8;
    colorRGBBits.green_shift = 8;
    colorRGBBits.green_bits = 8; 
    colorRGBBits.blue_shift = 0; 
    colorRGBBits.blue_bits = 8;  
  
    mColorSpace = IL_CreateTrueColorSpace(&colorRGBBits, 24);
    if (nsnull == mColorSpace) {
      aColorSpace = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_POSTCONDITION(nsnull != mColorSpace, "null color space");
  aColorSpace = mColorSpace;
  IL_AddRefToColorSpace(aColorSpace);
  return NS_OK;
}

NS_IMETHODIMP DeviceContextImpl::GetPaletteInfo(nsPaletteInfo& aPaletteInfo)
{
  aPaletteInfo.isPaletteDevice = PR_FALSE;
  aPaletteInfo.sizePalette = 0;
  aPaletteInfo.numReserved = 0;
  aPaletteInfo.palette = nsnull;
  return NS_OK;
}

