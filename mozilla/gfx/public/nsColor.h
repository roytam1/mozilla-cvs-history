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

#ifndef nsColor_h___
#define nsColor_h___

#include "nscore.h"

// A color is a 32 bit unsigned integer with four components: R, G, B
// and A.
typedef PRUint32 nscolor;

// Make a color out of r,g,b values. This assumes that the r,g,b values are
// properly constrained to 0-255. This also assumes that a is 255.
#define NS_RGB(_r,_g,_b) \
  ((nscolor) ((255 << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))

// Make a color out of r,g,b,a values. This assumes that the r,g,b,a
// values are properly constrained to 0-255.
#define NS_RGBA(_r,_g,_b,_a) \
  ((nscolor) (((_a) << 24) | ((_b)<<16) | ((_g)<<8) | (_r)))

// Extract color components from nscolor
#define NS_GET_R(_rgba) ((PRUint8) ((_rgba) & 0xff))
#define NS_GET_G(_rgba) ((PRUint8) (((_rgba) >> 8) & 0xff))
#define NS_GET_B(_rgba) ((PRUint8) (((_rgba) >> 16) & 0xff))
#define NS_GET_A(_rgba) ((PRUint8) (((_rgba) >> 24) & 0xff))

// Translate a hex string to a color. Return true if it parses ok,
// otherwise return false.
// This accepts only 3, 6 or 9 digits
extern "C" NS_GFX_(PRBool) NS_HexToRGB(const char* aBuf, nscolor* aResult);

// Translate a hex string to a color. Return true if it parses ok,
// otherwise return false.
// This version accepts 1 to 9 digits (missing digits are 0)
extern "C" NS_GFX_(PRBool) NS_LooseHexToRGB(const char* aBuf, nscolor* aResult);

// Translate a color name to a color. Return true if it parses ok,
// otherwise return false.
extern "C" NS_GFX_(PRBool) NS_ColorNameToRGB(const char* aBuf, nscolor* aResult);

// Weird color computing code stolen from winfe which was stolen
// from the xfe which was written originally by Eric Bina. So there.
extern "C" NS_GFX_(void) NS_Get3DColors(nscolor aResult[2], nscolor aColor);

// To determin colors based on the background brightness
extern "C" NS_GFX_(void) NS_GetSpecial3DColors(nscolor aResult[2],
											   nscolor aBackgroundColor,
											   nscolor aBorderColor);

// Special method to brighten a Color and have it shift to white when
// fully saturated.
extern "C" NS_GFX_(nscolor) NS_BrightenColor(nscolor inColor);

// Special method to darken a Color and have it shift to black when
// darkest component underflows
extern "C" NS_GFX_(nscolor) NS_DarkenColor(nscolor inColor);

// Determins brightness for a specific color
extern "C" NS_GFX_(int) NS_GetBrightness(PRUint8 aRed, PRUint8 aGreen, PRUint8 aBlue);

#endif /* nsColor_h___ */
