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

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsGfxCIID.h"
#include "nsFontMetricsWin.h"
#include "nsRenderingContextWin.h"
#include "nsImageWin.h"
#include "nsDeviceContextWin.h"
#include "nsRegionWin.h"
#include "nsBlender.h"
#include "nsDeviceContextSpecWin.h"
#include "nsDeviceContextSpecFactoryW.h"
#include "nsScriptableRegion.h"
#include "nsIImageManager.h"
#include "nsScreenManagerWin.h"
#include "nsPrintOptionsWin.h"
#include "nsFontList.h"
#include "nsIGenericFactory.h"

//NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDrawingSurfaceWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecFactoryWin)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptableRegion)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageManagerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrintOptionsWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerWin)


static PRBool
UseAFunctions()
{
  static PRBool useAFunctions = PR_FALSE;
  static PRBool init = PR_FALSE;
  if (!init) {
    init = 1;
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    if ((os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
        (os.dwMajorVersion == 4) &&
        (os.dwMinorVersion == 0) &&    // Windows 95 (not 98)
        (::GetACP() == 932)) {         // Shift-JIS (Japanese)
      useAFunctions = 1;
    }
  }

  return useAFunctions;
}

static NS_IMETHODIMP
nsFontMetricsWinConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsFontMetricsWin* result;
  if (UseAFunctions())
    result = new nsFontMetricsWinA();
  else
    result = new nsFontMetricsWin();

  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  NS_ADDREF(result);
  rv = result->QueryInterface(aIID, aResult);
  NS_RELEASE(result);
  return rv;
}

static NS_IMETHODIMP
nsScriptableRegionConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsRegionWin* region = new nsRegionWin();
  if (! region)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(region);

  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  nsScriptableRegion* result = new nsScriptableRegion(region);
  if (result) {
    NS_ADDREF(result);
    rv = result->QueryInterface(aIID, aResult);
    NS_RELEASE(result);
  }

  NS_RELEASE(region);
  return rv;
}

static NS_IMETHODIMP
nsImageManagerImplConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsresult rv;
  nsCOMPtr<nsIImageManager> result;
  rv = NS_NewImageManager(getter_AddRefs(result));

  if (result)
    rv = result->QueryInterface(aIID, aResult);

  return rv;
}

static nsModuleComponentInfo components[] =
{
  { "nsFontMetricsWin",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsWinConstructor },

  { "nsDeviceContextWin",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextWinConstructor },

  { "nsRenderingContextWin",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextWinConstructor },

  { "nsImageWin",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    nsImageWinConstructor },

  { "nsRegionWin",
    NS_REGION_CID,
    "@mozilla.org/gfx/unscriptable-region;1",
    nsRegionWinConstructor },

  { "nsBlender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },

  { "nsDrawingSurfaceWin",
    NS_DRAWING_SURFACE_CID,
    "@mozilla.org/gfx/drawing-surface;1",
    nsDrawingSurfaceWinConstructor },

  { "nsDeviceContextSpecWin",
    NS_DEVICE_CONTEXT_SPEC_CID,
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecWinConstructor },

  { "nsDeviceContextSpecFactoryWin",
    NS_DEVICE_CONTEXT_SPEC_FACTORY_CID,
    "@mozilla.org/gfx/devicecontextspecfactory;1",
    nsDeviceContextSpecFactoryWinConstructor },

  { "nsScriptableRegion",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },

  { "nsImageManagerImpl",
    NS_IMAGEMANAGER_CID,
    "@mozilla.org/gfx/imagemanager;1",
    nsImageManagerImplConstructor },

  { "nsPrintOptionsWin",
    NS_PRINTOPTIONS_CID,
    "@mozilla.org/gfx/printoptions;1",
    nsPrintOptionsWinConstructor },

  { "nsFontEnumeratorWin",
    NS_FONT_ENUMERATOR_CID,
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorWinConstructor },

  { "nsFontList",
    NS_FONTLIST_CID,
    "@mozilla.org/gfx/fontlist;1",
    nsFontListConstructor },

  { "nsScreenManagerWin",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerWinConstructor },
};

NS_IMPL_NSGETMODULE(nsGfxModule, components)
