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

#include "nsStyleUtil.h"
#include "nsCRT.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"

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

  return scale;
}

/*
 * Lifted from winfe/cxdc.cpp
 */
nscoord nsStyleUtil::CalcFontPointSize(PRInt32 aHTMLSize, PRInt32 aBasePointSize,
                                       float aScalingFactor)
{ // lifted directly from Nav 5.0 code to replicate rounding errors
  double dFontSize;

	switch(aHTMLSize)	{
	case 0:
		dFontSize = aBasePointSize / 2;
		break;
	case 1:
		dFontSize = 7 * aBasePointSize / 10;
		break;
	case 2:
		dFontSize = 85 * aBasePointSize / 100;
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
	case 8:
		dFontSize = 4 * aBasePointSize;
		break;
	case 3:
	default:
		dFontSize = aBasePointSize;
	}

  dFontSize *= aScalingFactor;

  return (nscoord)dFontSize;
}

PRInt32 nsStyleUtil::FindNextSmallerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                             float aScalingFactor)
{
  PRInt32 index;
  for (index = 7; index > 1; index--)
    if (aFontSize > nsStyleUtil::CalcFontPointSize(index, aBasePointSize, aScalingFactor))
      break;
  return index;
}

PRInt32 nsStyleUtil::FindNextLargerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                            float aScalingFactor)
{
  PRInt32 index;
  for (index = 1; index < 7; index++)
    if (aFontSize < nsStyleUtil::CalcFontPointSize(index, aBasePointSize, aScalingFactor))
      break;
  return index;
}

const nsStyleColor* nsStyleUtil::FindNonTransparentBackground(nsIStyleContext* aContext)
{
  const nsStyleColor* result = nsnull;
  nsIStyleContext*    context = aContext;

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

