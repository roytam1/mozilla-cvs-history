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

#include "nsStyleUtil.h"
#include "nsCRT.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsUnitConversion.h"

#include <math.h>

#define POSITIVE_SCALE_FACTOR 1.10 /* 10% */
#define NEGATIVE_SCALE_FACTOR .90  /* 10% */

/*
 * Return the scaling percentage given a font scaler
 * Lifted from layutil.c
 */
float nsStyleUtil::GetScalingFactor(PRInt32 aScaler)
{
  double scale = 1.0;
  double mult;
  PRInt32 count;

  if(aScaler < 0)   {
    count = -aScaler;
    mult = NEGATIVE_SCALE_FACTOR;
  }
  else {
    count = aScaler;
    mult = POSITIVE_SCALE_FACTOR;
  }

  /* use the percentage scaling factor to the power of the pref */
  while(count--) {
    scale *= mult;
  }

  return (float)scale;
}

/*
 * Lifted from winfe/cxdc.cpp
 */
nscoord nsStyleUtil::CalcFontPointSize(PRInt32 aHTMLSize, PRInt32 aBasePointSize,
                                       float aScalingFactor)
{ // lifted directly from Nav 5.0 code to replicate rounding errors
  double dFontSize;

	switch(aHTMLSize)	{
	case 1:
		dFontSize = 7 * aBasePointSize / 10;
		break;
	case 2:
		dFontSize = 85 * aBasePointSize / 100;
		break;
	case 3:
		dFontSize = aBasePointSize;
    break;
	case 4:
		dFontSize = 12 * aBasePointSize / 10;
		break;
	case 5:
		dFontSize = 3 * aBasePointSize / 2;
		break;
	case 6:
		dFontSize = 2 * aBasePointSize;
		break;
	case 7:
		dFontSize = 3 * aBasePointSize;
		break;
	default:
    if (aHTMLSize < 1) {
      dFontSize = (7 * aBasePointSize / 10) / pow(1.1, 1 - aHTMLSize);
    }
    else {  // 7 < aHTMLSize
      dFontSize = (3 * aBasePointSize) * pow(1.2, aHTMLSize - 7);
    }
	}

  dFontSize *= aScalingFactor;

  if (1.0 < dFontSize) {
    return (nscoord)dFontSize;
  }
  return (nscoord)1;
}

PRInt32 nsStyleUtil::FindNextSmallerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                             float aScalingFactor)
{
  PRInt32 index;
  PRInt32 fontSize = NSTwipsToFloorIntPoints(aFontSize);

  if (NSTwipsToFloorIntPoints(CalcFontPointSize(1, aBasePointSize, aScalingFactor)) < fontSize) {
    if (fontSize <= NSTwipsToFloorIntPoints(CalcFontPointSize(7, aBasePointSize, aScalingFactor))) { // in HTML table
      for (index = 7; index > 1; index--)
        if (fontSize > NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor)))
          break;
    }
    else {  // larger than HTML table
      for (index = 8; ; index++)
        if (fontSize < NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor))) {
          index--;
          break;
        }
    }
  }
  else { // smaller than HTML table
    for (index = 0; -25<index ; index--) //prevent infinite loop (bug 17045)
      if (fontSize > NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor))) {
        break;
      }
  }
  return index;
}

PRInt32 nsStyleUtil::FindNextLargerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                            float aScalingFactor)
{
  PRInt32 index;
  PRInt32 fontSize = NSTwipsToFloorIntPoints(aFontSize);

  if (NSTwipsToFloorIntPoints(CalcFontPointSize(1, aBasePointSize, aScalingFactor)) <= fontSize) {
    if (fontSize < NSTwipsToFloorIntPoints(CalcFontPointSize(7, aBasePointSize, aScalingFactor))) { // in HTML table
      for (index = 1; index < 7; index++)
        if (fontSize < NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor)))
          break;
    }
    else {  // larger than HTML table
      for (index = 8; ; index++)
        if (fontSize < NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor)))
          break;
    }
  }
  else {  // smaller than HTML table
    for (index = 0; -25<index ; index--) //prevent infinite loop (bug 17045)
      if (fontSize > NSTwipsToFloorIntPoints(CalcFontPointSize(index, aBasePointSize, aScalingFactor))) {
        index++;
        break;
      }
  }
  return index;
}

PRInt32 
nsStyleUtil::ConstrainFontWeight(PRInt32 aWeight)
{
  aWeight = ((aWeight < 100) ? 100 : ((aWeight > 900) ? 900 : aWeight));
  PRInt32 base = ((aWeight / 100) * 100);
  PRInt32 step = (aWeight % 100);
  PRBool  negativeStep = PRBool(50 < step);
  PRInt32 maxStep;
  if (negativeStep) {
    step = 100 - step;
    maxStep = (base / 100);
    base += 100;
  }
  else {
    maxStep = ((900 - base) / 100);
  }
  if (maxStep < step) {
    step = maxStep;
  }
  return (base + ((negativeStep) ? -step : step));
}


const nsStyleColor* nsStyleUtil::FindNonTransparentBackground(nsIStyleContext* aContext,
                                                              PRBool aStartAtParent /*= PR_FALSE*/)
{
  const nsStyleColor* result = nsnull;
  nsIStyleContext*    context = aStartAtParent ? aContext->GetParent() : aContext;
  NS_ASSERTION( context != nsnull, "Cannot find NonTransparentBackground in a null context" );
  
  NS_IF_ADDREF(context);  // balance ending release
  while (nsnull != context) {
    result = (const nsStyleColor*)context->GetStyleData(eStyleStruct_Color);

    if (0 == (result->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)) {
      break;
    }
    else {
      nsIStyleContext* last = context;
      context = context->GetParent();
      NS_RELEASE(last);
    }
  }
  NS_IF_RELEASE(context);
  return result;
}

