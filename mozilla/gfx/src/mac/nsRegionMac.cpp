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

#include "nsRegionMac.h"
#include "prmem.h"

static NS_DEFINE_IID(kRegionIID, NS_IREGION_IID);

//---------------------------------------------------------------------

nsRegionMac :: nsRegionMac()
{
  NS_INIT_REFCNT();
	mRegion = nsnull;
  mRegionType = eRegionComplexity_empty;
}

//---------------------------------------------------------------------

nsRegionMac :: ~nsRegionMac()
{
  if (mRegion)
    ::DisposeRgn(mRegion);
  mRegion = nsnull;
}

NS_IMPL_QUERY_INTERFACE(nsRegionMac, kRegionIID)
NS_IMPL_ADDREF(nsRegionMac)
NS_IMPL_RELEASE(nsRegionMac)

//---------------------------------------------------------------------

nsresult nsRegionMac :: Init(void)
{
	mRegion = ::NewRgn();
  mRegionType = eRegionComplexity_empty;
  return NS_OK;
}

//---------------------------------------------------------------------

void nsRegionMac :: SetTo(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::CopyRgn(pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	::SetRectRgn(mRegion, aX, aY, aX + aWidth, aY + aHeight);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Intersect(const nsIRegion &aRegion)
{
  nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::SectRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::SectRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Union(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::UnionRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::UnionRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Subtract(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::DiffRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::DiffRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: IsEmpty(void)
{
  if (mRegionType == eRegionComplexity_empty)
    return PR_TRUE;
	else
  	return PR_FALSE;
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: IsEqual(const nsIRegion &aRegion)
{
  nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  return(::EqualRgn(mRegion, pRegion->mRegion));
}

//---------------------------------------------------------------------

void nsRegionMac :: GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  Rect macRect = (**mRegion).rgnBBox;

  *aX = macRect.left;
  *aY = macRect.top;
  *aWidth  = macRect.right - macRect.left;
  *aHeight = macRect.bottom - macRect.top;
}

//---------------------------------------------------------------------

void nsRegionMac :: Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
  ::OffsetRgn(mRegion, aXOffset, aYOffset);
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	Rect macRect;
	::SetRect(&macRect, aX, aY, aX + aWidth, aY + aHeight);
	return(::RectInRgn(&macRect, mRegion));
}

//---------------------------------------------------------------------

NS_IMETHODIMP nsRegionMac :: GetRects(nsRegionRectSet **aRects)
{
  nsRegionRectSet *rects;

  NS_ASSERTION(!(nsnull == aRects), "bad ptr");

  rects = *aRects;

  if (nsnull == rects)
  {
    rects = (nsRegionRectSet *)PR_Malloc(sizeof(nsRegionRectSet) + (sizeof(nsRegionRect) << 3));

    if (nsnull == rects)
    {
      *aRects = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rects->mRectsLen = 9;
  }

  rects->mNumRects = 0;
  rects->mArea = 0;

/* This is a minor adaptation of code written by Hugh Fisher
   and published in the RegionToRectangles example in the InfoMac archives.
   ported to raptor from old macfe. MMP
*/
#define EndMark 	32767
#define MaxY		32767
#define StackMax	1024

	typedef struct
  {
		short	size;
		Rect	bbox;
		short	data[];
  } ** Internal;
	
	Internal  region;
	short	    width, xAdjust, y, index, x1, x2, x;
	short	    stackStorage[1024];
	short     *buffer;
	
	region = (Internal)mRegion;
	
	/* Check for plain rectangle */
	if ((**region).size == 10)
  {
		rects->mRects[0].x = (**region).bbox.left;
		rects->mRects[0].y = (**region).bbox.top;
		rects->mRects[0].width = (**region).bbox.right - (**region).bbox.left;
		rects->mRects[0].height = (**region).bbox.bottom - (**region).bbox.top;

    rects->mNumRects = 1;
    rects->mArea = rects->mRects[0].width * rects->mRects[0].height;

    *aRects = rects;

		return NS_OK;
	}

	/* Got to scale x coordinates into range 0..something */
	xAdjust = (**region).bbox.left;
	width = (**region).bbox.right - xAdjust;

	/* Most regions will be less than 1024 pixels wide */

	if (width < StackMax)
		buffer = stackStorage;
	else
  {
		buffer = (short *)PR_Malloc(width * 2);

		if (buffer == NULL)
    {
			/* Truly humungous region or very low on memory.
			   Quietly doing nothing seems to be the
			   traditional Quickdraw response. */
      *aRects = rects;
			return NS_OK;
    }
	}

	/* Initialise scan line list to bottom edges */

	for (x = (**region).bbox.left; x < (**region).bbox.right; x++)
		buffer[x - xAdjust] = MaxY;

	index = 0;

	/* Loop until we hit an empty scan line */

	while ((**region).data[index] != EndMark)
  {
		y = (**region).data[index];
		index ++;

		/* Loop through horizontal runs on this line */

		while ((**region).data[index] != EndMark)
    {
			x1 = (**region).data[index];
			index ++;
			x2 = (**region).data[index];
			index ++;
			x = x1;

			while (x < x2)
      {
				if (buffer[x - xAdjust] < y)
        {
          nsRegionRect box;

					/* We have a bottom edge - how long for? */

					box.x = x;
					box.y  = buffer[x - xAdjust];

					while (x < x2 && buffer[x - xAdjust] == box.y)
          {
						buffer[x - xAdjust] = MaxY;
						x ++;
					}

					/* Pass to client proc */
					box.width  = x - box.x;
					box.height = y - box.y;

          if (rects->mNumRects == rects->mRectsLen)
          {
            void *buf = PR_Realloc((void *)rects, sizeof(nsRegionRectSet) + sizeof(nsRegionRect) * (rects->mRectsLen + 7));

            if (nsnull != buf)
            {
              rects = (nsRegionRectSet *)buf;
              rects->mRectsLen += 8;
            }
          }

          if (rects->mNumRects != rects->mRectsLen)
          {
            rects->mArea += box.width * box.height;
            rects->mRects[rects->mNumRects] = box;
            rects->mNumRects++;
          }
				}
        else
        {
					/* This becomes a top edge */

					buffer[x - xAdjust] = y;
					x ++;
				}
			}
		}

		index ++;
	}

	/* Clean up after ourselves */

	if (width >= StackMax)
		PR_Free((void *)buffer);

#undef EndMark
#undef MaxY
#undef StackMax

  *aRects = rects;

  return NS_OK;
}

//---------------------------------------------------------------------

NS_IMETHODIMP nsRegionMac :: FreeRects(nsRegionRectSet *aRects)
{
  if (nsnull != aRects)
    PR_Free((void *)aRects);

  return NS_OK;
}

//---------------------------------------------------------------------


NS_IMETHODIMP nsRegionMac :: GetNativeRegion(void *&aRegion) const
{
  aRegion = (void *)mRegion;
  return NS_OK;
}


nsresult nsRegionMac :: SetNativeRegion(void *aRegion)
{
  if (aRegion)
  {
    ::CopyRgn((RgnHandle)aRegion, mRegion);
    SetRegionType();
  }
  else
  {
  	Init();
  }
  return NS_OK;
}

//---------------------------------------------------------------------

NS_IMETHODIMP nsRegionMac :: GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
  aComplexity = mRegionType;
  return NS_OK;
}

//---------------------------------------------------------------------


void nsRegionMac :: SetRegionType()
{
  if (::EmptyRgn(mRegion) == PR_TRUE)
    mRegionType = eRegionComplexity_empty;
  else
    if ((*mRegion)->rgnSize == 10)
      mRegionType = eRegionComplexity_rect;
    else
      mRegionType = eRegionComplexity_complex;
}


//---------------------------------------------------------------------


void nsRegionMac :: SetRegionEmpty()
{
  ::SetEmptyRgn(mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------


RgnHandle nsRegionMac :: CreateRectRegion(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  return rectRgn;
}








