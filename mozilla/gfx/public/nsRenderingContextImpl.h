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

#ifndef nsRenderingContextImpl_h___
#define nsRenderingContextImpl_h___

#include "nsIRenderingContext.h"


class nsRenderingContextImpl : public nsIRenderingContext
{

// CLASS MEMBERS
public:


protected:
  nsTransform2D		  *mTranMatrix;		// The rendering contexts transformation matrix

public:
  nsRenderingContextImpl();


// CLASS METHODS
  /** ---------------------------------------------------
   *  See documentation in nsIRenderingContext.h
   *	@update 03/29/00 dwc
   */
  NS_IMETHOD DrawTile(nsIImage *aImage,nscoord aX0,nscoord aY0,nscoord aX1,nscoord aY1,nscoord aWidth,nscoord aHeight);

  /** ---------------------------------------------------
   *  See documentation in nsIRenderingContext.h
   *	@update 03/29/00 dwc
   */
  NS_IMETHOD DrawPath(nsPathPoint aPointArray[],PRInt32 aNumPts);

    /** ---------------------------------------------------
   *  See documentation in nsIRenderingContext.h
   *	@update 03/29/00 dwc
   */
  NS_IMETHOD FillPath(nsPathPoint aPointArray[],PRInt32 aNumPts);

  /** ---------------------------------------------------
   *  See documentation in nsIRenderingContext.h
   *	@update 05/01/00 dwc
   */
  NS_IMETHOD DrawStdLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1) { return NS_OK;}

  /** ---------------------------------------------------
   *  See documentation in nsIRenderingContext.h
   *	@update 05/01/00 dwc
   */
  NS_IMETHOD FillStdPolygon(const nsPoint aPoints[], PRInt32 aNumPoints) { return NS_OK; }

protected:
  virtual ~nsRenderingContextImpl();

  /** ---------------------------------------------------
   *  Check to see if the given size of tile can be imaged by the RenderingContext
   *	@update 03/29/00 dwc
   *  @param aWidth The width of the tile
   *  @param aHeight The height of the tile
   *  @return PR_TRUE the RenderingContext can handle this tile
   */
  virtual PRBool CanTile(nscoord aWidth,nscoord aHeight) { return PR_FALSE; }

  
  /** ---------------------------------------------------
   *  A bit blitter to tile images to the background recursively
   *	@update 3/29/00 dwc
   *  @param aDS -- Target drawing surface for the rendering context
   *  @param aSrcRect -- Rectangle we are build with the image
   *  @param aHeight -- height of the tile
   *  @param aWidth -- width of the tile
   */
  void  TileImage(nsDrawingSurface  aDS,nsRect &aSrcRect,PRInt16 aWidth,PRInt16 aHeight);



public:

};


/** ---------------------------------------------------
 *  A point structure with floats for the Quadratic bezier curve
 *	@update 4/27/2000 dwc
 */
struct nsFloatPoint {
  float x, y;

  // Constructors
  nsFloatPoint() {}
  nsFloatPoint(const nsFloatPoint& aPoint) {x = aPoint.x; y = aPoint.y;}
  nsFloatPoint(float aX, float aY) {x = aX; y = aY;}

  void MoveTo(float aX, float aY) {x = aX; y = aY;}
  void MoveTo(nscoord aX, nscoord aY) {x = (float)aX; y = (float)aY;}
  void MoveBy(float aDx, float aDy) {x += aDx; y += aDy;}

  // Overloaded operators. Note that '=' isn't defined so we'll get the
  // compiler generated default assignment operator
  PRBool   operator==(const nsFloatPoint& aPoint) const {
    return (PRBool) ((x == aPoint.x) && (y == aPoint.y));
  }
  PRBool   operator!=(const nsFloatPoint& aPoint) const {
    return (PRBool) ((x != aPoint.x) || (y != aPoint.y));
  }
  nsFloatPoint operator+(const nsFloatPoint& aPoint) const {
    return nsFloatPoint(x + aPoint.x, y + aPoint.y);
  }
  nsFloatPoint operator-(const nsFloatPoint& aPoint) const {
    return nsFloatPoint(x - aPoint.x, y - aPoint.y);
  }
  nsFloatPoint& operator+=(const nsFloatPoint& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    return *this;
  }
  nsFloatPoint& operator-=(const nsFloatPoint& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    return *this;
  }
};


/** ---------------------------------------------------
 *  Class QBezierCurve, a quadratic bezier curve
 *	@update 4/27/2000 dwc
 */
class QBezierCurve
{

public:
	nsFloatPoint	mAnc1;
	nsFloatPoint	mCon;
	nsFloatPoint  mAnc2;

  QBezierCurve() {mAnc1.x=0;mAnc1.y=0;mCon=mAnc2=mAnc1;}
  void SetControls(nsFloatPoint &aAnc1,nsFloatPoint &aCon,nsFloatPoint &aAnc2) { mAnc1 = aAnc1; mCon = aCon; mAnc2 = aAnc2;}
  void SetPoints(nscoord a1x,nscoord a1y,nscoord acx,nscoord acy,nscoord a2x,nscoord a2y) {mAnc1.MoveTo(a1x,a1y),mCon.MoveTo(acx,acy),mAnc2.MoveTo(a2x,a2y);}
  void SetPoints(float a1x,float a1y,float acx,float acy,float a2x,float a2y) {mAnc1.MoveTo(a1x,a1y),mCon.MoveTo(acx,acy),mAnc2.MoveTo(a2x,a2y);}

/** ---------------------------------------------------
 *  Divide a Quadratic curve into line segments if it is not smaller than a certain size
 *  else it is so small that it can be approximated by 2 lineto calls
 *  @param aRenderingContext -- The RenderingContext to use to draw with
 *	@update 3/26/99 dwc
 */
  void SubDivide(nsIRenderingContext *aRenderingContext);

/** ---------------------------------------------------
 *  Divide a Quadratic curve into line segments if it is not smaller than a certain size
 *  else it is so small that it can be approximated by 2 lineto calls
 *  @param nsPoint* -- The points array to rasterize into
 *  @param aNumPts* -- Current number of points in this array
 *	@update 3/26/99 dwc
 */
  void SubDivide(nsPoint aThePoints[],PRInt16 *aNumPts);

/** ---------------------------------------------------
 *  Divide a Quadratic Bezier curve at the mid-point
 *	@update 3/26/99 dwc
 *  @param aCurve1 -- Curve 1 as a result of the division
 *  @param aCurve2 -- Curve 2 as a result of the division
 */
  void MidPointDivide(QBezierCurve *A,QBezierCurve *B);
};

  enum eSegType {eUNDEF,eLINE,eQCURVE,eCCURVE};


/** ---------------------------------------------------
 *  A class to iterate through a nsPathPoint array and return segments
 *	@update 04/27/00 dwc
 */
class nsPathIter {

public:
  enum eSegType {eUNDEF,eLINE,eQCURVE,eCCURVE};

private:
  PRUint32    mCurPoint;
  PRUint32    mNumPoints;
  nsPathPoint *mThePath;

public:
  nsPathIter();
  nsPathIter(nsPathPoint* aThePath,PRUint32 aNumPts);

  PRBool  NextSeg(QBezierCurve& TheSegment,eSegType& aCurveType);

};


#endif /* nsRenderingContextImpl */
