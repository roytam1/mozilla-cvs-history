/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#include "nsBlockBandData.h"
#include "nsIFrame.h"
#include "nsIFrameReflow.h"
#include "nsIStyleContext.h"
#include "nsVoidArray.h"

nsBlockBandData::nsBlockBandData()
  : mSpaceManager(nsnull),
    mSpaceManagerX(0),
    mSpaceManagerY(0),
    mSpace(0, 0)
{
  mSize = NS_BLOCK_BAND_DATA_TRAPS;
  mTrapezoids = mData;
}

nsBlockBandData::~nsBlockBandData()
{
  NS_IF_RELEASE(mSpaceManager);
  if (mTrapezoids != mData) {
    delete [] mTrapezoids;
  }
}

nsresult
nsBlockBandData::Init(nsISpaceManager* aSpaceManager,
                      const nsSize& aSpace)
{
  NS_PRECONDITION(nsnull != aSpaceManager, "null pointer");
  if (nsnull == aSpaceManager) {
    return NS_ERROR_NULL_POINTER;
  }

  NS_IF_RELEASE(mSpaceManager);
  mSpaceManager = aSpaceManager;
  NS_ADDREF(aSpaceManager);
  aSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);

  mSpace = aSpace;
  mLeftFloaters = 0;
  mRightFloaters = 0;
  return NS_OK;
}

// Get the available reflow space for the current y coordinate. The
// available space is relative to our coordinate system (0,0) is our
// upper left corner.
nsresult
nsBlockBandData::GetAvailableSpace(nscoord aY, nsRect& aResult)
{
  // Get the raw band data for the given Y coordinate
  nsresult rv = mSpaceManager->GetBandData(aY, mSpace, *this);
  while (NS_FAILED(rv)) {
    // We need more space for our bands
    if (mTrapezoids != mData) {
      delete [] mTrapezoids;
    }
    PRInt32 newSize = mSize * 2;
    mTrapezoids = new nsBandTrapezoid[newSize];
    if (!mTrapezoids) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mSize = newSize;
    rv = mSpaceManager->GetBandData(aY, mSpace, *this);
  }

  // Compute the bounding rect of the available space, i.e. space
  // between any left and right floaters.
  ComputeAvailSpaceRect();
  aResult = mAvailSpace;
  return NS_OK;
}

/**
 * Computes the bounding rect of the available space, i.e. space
 * between any left and right floaters. Uses the current trapezoid
 * data, see nsISpaceManager::GetBandData(). Also updates member
 * data "availSpace".
 */
void
nsBlockBandData::ComputeAvailSpaceRect()
{
  if (0 == mCount) {
    mAvailSpace.x = 0;
    mAvailSpace.y = 0;
    mAvailSpace.width = 0;
    mAvailSpace.height = 0;
    return;
  }

  nsBandTrapezoid* trapezoid = mTrapezoids;
  nsBandTrapezoid* rightTrapezoid = nsnull;

  PRInt32 leftFloaters = 0;
  PRInt32 rightFloaters = 0;
  if (mCount > 1) {
    // If there's more than one trapezoid that means there are floaters
    PRInt32 i;

    // Examine each trapezoid in the band, counting up the number of
    // left and right floaters. Use the right-most floater to
    // determine where the right edge of the available space is.
    for (i = 0; i < mCount; i++) {
      trapezoid = &mTrapezoids[i];
      if (trapezoid->mState != nsBandTrapezoid::Available) {
        const nsStyleDisplay* display;
        if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
          PRInt32 j, numFrames = trapezoid->mFrames->Count();
          NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
          for (j = 0; j < numFrames; j++) {
            nsIFrame* f = (nsIFrame*) trapezoid->mFrames->ElementAt(j);
            f->GetStyleData(eStyleStruct_Display,
                            (const nsStyleStruct*&)display);
            if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
              leftFloaters++;
            }
            else if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
              rightFloaters++;
              if ((nsnull == rightTrapezoid) && (i > 0)) {
                rightTrapezoid = &mTrapezoids[i - 1];
              }
            }
          }
        } else {
          trapezoid->mFrame->GetStyleData(eStyleStruct_Display,
                                    (const nsStyleStruct*&)display);
          if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
            leftFloaters++;
          }
          else if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
            rightFloaters++;
            if ((nsnull == rightTrapezoid) && (i > 0)) {
              rightTrapezoid = &mTrapezoids[i - 1];
            }
          }
        }
      }
    }
  }
  else if (mTrapezoids[0].mState != nsBandTrapezoid::Available) {
    // We have a floater using up all the available space
    leftFloaters = 1;
  }
  mLeftFloaters = leftFloaters;
  mRightFloaters = rightFloaters;

  if (nsnull != rightTrapezoid) {
    trapezoid = rightTrapezoid;
  }
  trapezoid->GetRect(mAvailSpace);

  // When there is no available space, we still need a proper X
  // coordinate to place objects that end up here anyway.
  if (nsBandTrapezoid::Available != trapezoid->mState) {
    const nsStyleDisplay* display;
    if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
      // It's not clear what coordinate to use when there is no
      // available space and the space is multiply occupied...So: If
      // any of the floaters that are a part of the trapezoid are left
      // floaters then we move over to the right edge of the
      // unavaliable space.
      PRInt32 j, numFrames = trapezoid->mFrames->Count();
      NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
      for (j = 0; j < numFrames; j++) {
        nsIFrame* f = (nsIFrame*) trapezoid->mFrames->ElementAt(j);
        f->GetStyleData(eStyleStruct_Display,
                        (const nsStyleStruct*&)display);
        if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
          mAvailSpace.x = mAvailSpace.XMost();
          break;
        }
      }
    }
    else {
      trapezoid->mFrame->GetStyleData(eStyleStruct_Display,
                                     (const nsStyleStruct*&)display);
      if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
        mAvailSpace.x = mAvailSpace.XMost();
      }
    }
    mAvailSpace.width = 0;
  }

  // Fixup width
  if (NS_UNCONSTRAINEDSIZE == mSpace.width) {
    mAvailSpace.width = NS_UNCONSTRAINEDSIZE;
  }
}

/**
 * See if the given frame should be cleared
 */
PRBool
nsBlockBandData::ShouldClearFrame(nsIFrame* aFrame, PRUint8 aBreakType)
{
  PRBool result = PR_FALSE;
  const nsStyleDisplay* display;
  nsresult rv = aFrame->GetStyleData(eStyleStruct_Display,
                                     (const nsStyleStruct*&)display);
  if (NS_SUCCEEDED(rv) && (nsnull != display)) {
    if (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aBreakType) {
      result = PR_TRUE;
    }
    else if (NS_STYLE_FLOAT_LEFT == display->mFloats) {
      if (NS_STYLE_CLEAR_LEFT == aBreakType) {
        result = PR_TRUE;
      }
    }
    else if (NS_STYLE_FLOAT_RIGHT == display->mFloats) {
      if (NS_STYLE_CLEAR_RIGHT == aBreakType) {
        result = PR_TRUE;
      }
    }
  }
  return result;
}

// XXX optimization? use mFloaters to avoid doing anything
nscoord
nsBlockBandData::ClearFloaters(nscoord aY, PRUint8 aBreakType)
{
  for (;;) {
    // Update band information based on target Y before clearing.
    mSpaceManager->GetBandData(aY, mSpace, *this);
    ComputeAvailSpaceRect();

    // Compute aYS as aY in space-manager "root" coordinates.
    nscoord aYS = aY + mSpaceManagerY;

    // Find the largest frame YMost for the appropriate floaters in
    // this band.
    nscoord yMost = aYS;
    PRInt32 i;
    for (i = 0; i < mCount; i++) {
      nsBandTrapezoid* trapezoid = &mTrapezoids[i];
      if (nsBandTrapezoid::Available != trapezoid->mState) {
        if (nsBandTrapezoid::OccupiedMultiple == trapezoid->mState) {
          PRInt32 fn, numFrames = trapezoid->mFrames->Count();
          NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
          for (fn = 0; fn < numFrames; fn++) {
            nsIFrame* frame = (nsIFrame*) trapezoid->mFrames->ElementAt(fn);
            if (ShouldClearFrame(frame, aBreakType)) {
              nscoord ym = trapezoid->mBottomY + mSpaceManagerY;
              if (ym > yMost) yMost = ym;
            }
          }
        }
        else if (ShouldClearFrame(trapezoid->mFrame, aBreakType)) {
          nscoord ym = trapezoid->mBottomY + mSpaceManagerY;
          if (ym > yMost) yMost = ym;
        }
      }
    }

    // If yMost is unchanged (aYS) then there were no appropriate
    // floaters in the band. Time to stop clearing.
    if (yMost == aYS) {
      break;
    }
    aY = aY + (yMost - aYS);
  }
  return aY;
}

void
nsBlockBandData::GetMaxElementSize(nscoord* aWidthResult,
                                   nscoord* aHeightResult) const
{
  nsRect r;
  nscoord maxWidth = 0;
  nscoord maxHeight = 0;
  for (PRInt32 i = 0; i < mCount; i++) {
    const nsBandTrapezoid* trap = &mTrapezoids[i];
    if (trap->mState != nsBandTrapezoid::Available) {
      // Get the width of the impacted area and update the maxWidth
      trap->GetRect(r);
      if (r.width > maxWidth) maxWidth = r.width;

      // Get the total height of the frame to compute the maxHeight,
      // not just the height that is part of this band.
      // XXX vertical margins!
      if (nsBandTrapezoid::OccupiedMultiple == trap->mState) {
        PRInt32 j, numFrames = trap->mFrames->Count();
        NS_ASSERTION(numFrames > 0, "bad trapezoid frame list");
        for (j = 0; j < numFrames; j++) {
          nsIFrame* f = (nsIFrame*) trap->mFrames->ElementAt(j);
          f->GetRect(r);
          if (r.height > maxHeight) maxHeight = r.height;
        }
      } else {
        trap->mFrame->GetRect(r);
        if (r.height > maxHeight) maxHeight = r.height;
      }
    }
  }
  *aWidthResult = maxWidth;
  *aHeightResult = maxHeight;
}
