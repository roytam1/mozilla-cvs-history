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

#ifndef nsIRenderingContext_h___
#define nsIRenderingContext_h___

#include "nscore.h"
#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"

class nsIWidget;
class nsIFontMetrics;
class nsIImage;
class nsTransform2D;
class nsString;
class nsIDeviceContext;
class nsIRegion;

struct nsFont;
struct nsPoint;
struct nsRect;

//cliprect/region combination methods

typedef enum
{
  nsClipCombine_kIntersect = 0,
  nsClipCombine_kUnion = 1,
  nsClipCombine_kSubtract = 2,
  nsClipCombine_kReplace = 3
} nsClipCombine;

//linestyles
typedef enum
{
  nsLineStyle_kNone   = 0,
  nsLineStyle_kSolid  = 1,
  nsLineStyle_kDashed = 2,
  nsLineStyle_kDotted = 3
} nsLineStyle;

// IID for the nsIRenderingContext interface
#define NS_IRENDERING_CONTEXT_IID \
 { 0xa6cf9068, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

//----------------------------------------------------------------------

//a cross platform way of specifying a native rendering context
typedef void * nsDrawingSurface;

// RenderingContext interface
class nsIRenderingContext : public nsISupports
{
public:
  //TBD: bind/unbind, transformation of scalars (hacky), 
  //potential drawmode for selection, polygons. MMP

  /**
   * Initialize the RenderingContext
   * @param aContext the device context to use.
   * @param aWidget the widget to hook up to
   * @result The result of the initialization, NS_Ok if no errors
   */
  NS_IMETHOD Init(nsIDeviceContext* aContext,nsIWidget *aWidget) = 0;

  /**
   * Initialize the RenderingContext
   * @param aContext the device context to use for the drawing.
   * @param aSurface the surface to draw into
   * @result The result of the initialization, NS_Ok if no errors
   */
  NS_IMETHOD Init(nsIDeviceContext* aContext,nsDrawingSurface aSurface) = 0;

  /**
   * Reset the rendering context
   */
  virtual void Reset() = 0;

  /**
   * Get the DeviceContext that this RenderingContext was initialized with
   * @result the device context
   */
  virtual nsIDeviceContext * GetDeviceContext(void) = 0;

  /**
   * Selects an offscreen drawing surface into the RenderingContext to draw to.
   * @param aSurface is the offscreen surface we are going to draw to.
   */
  NS_IMETHOD SelectOffScreenDrawingSurface(nsDrawingSurface aSurface) = 0;

  /**
   * Returns in aResult any rendering hints that the context has.
   * See below for the hints bits. Always returns NS_OK.
   */
  NS_IMETHOD GetHints(PRUint32& aResult) = 0;

  /**
   * Save a graphical state onto a stack.
   */
  virtual void PushState(void) = 0;

  /**
   * Get and and set RenderingContext to this graphical state
   * @return if PR_TRUE, indicates that the clipping region after
   *         popping state is empty, else PR_FALSE
   */
  virtual PRBool PopState(void) = 0;

  /**
   * Tells if a given rectangle is visible within the rendering context
   * @param aRect is the rectangle that will be checked for visiblity
   * @result If true, that rectanglular area is visable.
   */
  virtual PRBool IsVisibleRect(const nsRect& aRect) = 0;

  /**
   * Sets the clipping for the RenderingContext to the passed in rectangle
   * @param aRect The rectangle to set the clipping rectangle to
   * @param aCombine how to combine this rect with the current clip region.
   *        see the bottom of nsIRenderingContext.h
   * @return PR_TRUE if the clip region is now empty, else PR_FALSE
   */
  virtual PRBool SetClipRect(const nsRect& aRect, nsClipCombine aCombine) = 0;

  /**
   * Gets the bounds of the clip region of the RenderingContext
   * @param aRect out parameter to contain the clip region bounds
   *        for the RenderingContext
   * @return PR_TRUE if the rendering context has a local cliprect set
   *         else aRect is undefined
   */
  virtual PRBool GetClipRect(nsRect &aRect) = 0;

  /**
   * Sets the line style for the RenderingContext 
   * @param aLineStyle The line style
   * @return NS_OK if the line style is correctly set
   */
  NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle) = 0;

  /**
   * Gets the line style for the RenderingContext
   * @param aLineStyle The line style to be retrieved
   * @return NS_OK if the line style is correctly retrieved
   */
  NS_IMETHOD GetLineStyle(nsLineStyle &aLineStyle) = 0;

  /**
   * Sets the clipping for the RenderingContext to the passed in region
   * @param aRegion The region to set the clipping area to
   * @param aCombine how to combine this region with the current clip region.
   *        see the bottom of nsIRenderingContext.h
   * @return PR_TRUE if the clip region is now empty, else PR_FALSE
   */
  virtual PRBool SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine) = 0;

  /**
   * Gets the current clipping region for the RenderingContext
   * @param aRegion out parameter representing the clip region.
   *        if SetClipRegion() is called, do not assume that GetClipRegion()
   *        will return the same object.
   */
  virtual void GetClipRegion(nsIRegion **aRegion) = 0;

  /**
   * Sets the forground color for the RenderingContext
   * @param aColor The color to set the RenderingContext to
   */
  virtual void SetColor(nscolor aColor) = 0;

  /**
   * Get the forground color for the RenderingContext
   * @return The current forground color of the RenderingContext
   */
  virtual nscolor GetColor() const = 0;

  /**
   * Sets the font for the RenderingContext
   * @param aFont The font to use in the RenderingContext
   */
  virtual void SetFont(const nsFont& aFont) = 0;

  /**
   * Sets the font for the RenderingContext
   * @param aFontMetric The font metrics representing the
   *        font to use in the RenderingContext
   */
  virtual void SetFont(nsIFontMetrics *aFontMetrics) = 0;

  /**
   * Get the current font for the RenderingContext
   * @return The current font of the RenderingContext
   */
  virtual const nsFont& GetFont() = 0;

  /**
   * Get the current fontmetrics for the RenderingContext
   * @return The current font of the RenderingContext
   */
  virtual nsIFontMetrics* GetFontMetrics() = 0;

  /**
   *  Add in a translate to the RenderingContext's transformation matrix
   * @param aX The horizontal translation
   * @param aY The vertical translation
   */
  virtual void Translate(nscoord aX, nscoord aY) = 0;

  /**
   *  Add in a scale to the RenderingContext's transformation matrix
   * @param aX The horizontal scale
   * @param aY The vertical scale
   */
  virtual void Scale(float aSx, float aSy) = 0;

  /** 
   * Get the current transformation matrix for the RenderingContext
   * @return The transformation matrix for the RenderingContext
   */
  virtual nsTransform2D * GetCurrentTransform() = 0;

  /**
   * Create an offscreen drawing surface compatible with this RenderingContext
   * @param aBounds A rectangle representing the size for the drawing surface.
   *                if nsnull then a bitmap will not be created and associated
   *                with the new drawing surface
   * @return A nsDrawingSurface
   */
  virtual nsDrawingSurface CreateDrawingSurface(nsRect *aBounds) = 0;

  /**
   * Destroy a drawing surface created by CreateDrawingSurface()
   * @param aDS A drawing surface to destroy
   */
  virtual void DestroyDrawingSurface(nsDrawingSurface aDS) = 0;

  /**
   * Draw a line
   * @param aXO starting horiztonal coord in twips
   * @param aY0 starting vertical coord in twips
   * @param aX1 end horiztonal coord in twips
   * @param aY1 end vertical coord in twips
   */
  virtual void DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1) = 0;

  /**
   * Draw a polyline
   * @param aPoints array of endpoints
   * @param aNumPonts number of points
   */
  virtual void DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints) = 0;

  /**
   * Draw a rectangle
   * @param aRect The rectangle to draw
   */
  virtual void DrawRect(const nsRect& aRect) = 0;

  /**
   * Draw a rectangle
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of rectangle in twips
   * @param aHeight Height of rectangle in twips
   */
  virtual void DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  /**
   * Fill a rectangle in the current foreground color
   * @param aRect The rectangle to draw
   */
  virtual void FillRect(const nsRect& aRect) = 0;

  /**
   * Fill a rectangle in the current foreground color
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of rectangle in twips
   * @param aHeight Height of rectangle in twips
   */
  virtual void FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  /**
   * Draw a poly in the current foreground color
   * @param aPoints points to use for the drawing, last must equal first
   * @param aNumPonts number of points in the polygon
   */
  virtual void DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints) = 0;

  /**
   * Fill a poly in the current foreground color
   * @param aPoints points to use for the drawing, last must equal first
   * @param aNumPonts number of points in the polygon
   */
  virtual void FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints) = 0;

  /**
   * Draw an ellipse in the current foreground color
   * @param aRect The rectangle define bounds of ellipse to draw
   */
  virtual void DrawEllipse(const nsRect& aRect) = 0;

  /**
   * Draw an ellipse in the current foreground color
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of horizontal axis in twips
   * @param aHeight Height of vertical axis in twips
   */
  virtual void DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;

  /**
   * Fill an ellipse in the current foreground color
   * @param aRect The rectangle define bounds of ellipse to draw
   */
  virtual void FillEllipse(const nsRect& aRect) = 0;
  /**
   * Fill an ellipse in the current foreground color
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of horizontal axis in twips
   * @param aHeight Height of vertical axis in twips
   */
  virtual void FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) = 0;
  /**
   * Draw an arc in the current forground color
   * @param aRect The rectangle define bounds of ellipse to use
   * @param aStartAngle the starting angle of the arc, in the ellipse
   * @param aEndAngle The ending angle of the arc, in the ellipse
   */
  virtual void DrawArc(const nsRect& aRect,
                       float aStartAngle, float aEndAngle) = 0;
  /**
   * Draw an arc in the current forground color
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of horizontal axis in twips
   * @param aHeight Height of vertical axis in twips
   * @param aStartAngle the starting angle of the arc, in the ellipse
   * @param aEndAngle The ending angle of the arc, in the ellipse
   */
  virtual void DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                       float aStartAngle, float aEndAngle) = 0;
  /**
   * Fill an arc in the current forground color
   * @param aRect The rectangle define bounds of ellipse to use
   * @param aStartAngle the starting angle of the arc, in the ellipse
   * @param aEndAngle The ending angle of the arc, in the ellipse
   */
  virtual void FillArc(const nsRect& aRect,
                       float aStartAngle, float aEndAngle) = 0;
  /**
   * Fill an arc in the current forground color
   * @param aX Horizontal left Coordinate in twips
   * @param aY Vertical top Coordinate in twips
   * @param aWidth Width of horizontal axis in twips
   * @param aHeight Height of vertical axis in twips
   * @param aStartAngle the starting angle of the arc, in the ellipse
   * @param aEndAngle The ending angle of the arc, in the ellipse
   */
  virtual void FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                       float aStartAngle, float aEndAngle) = 0;

  /**
   * Returns the width (in app units) of an 8-bit character
   */
  NS_IMETHOD  GetWidth(char aC, nscoord &aWidth) = 0;

  /**
   * Returns the width (in app units) of a unicode character
   */
  NS_IMETHOD  GetWidth(PRUnichar aC, nscoord &aWidth) = 0;

  /**
   * Returns the width (in app units) of an nsString
   */
  NS_IMETHOD  GetWidth(const nsString& aString, nscoord &aWidth) = 0;

  /**
   * Returns the width (in app units) of an 8-bit character string
   */
  NS_IMETHOD  GetWidth(const char* aString, nscoord& aWidth) = 0;

  /**
   * Returns the width (in app units) of an 8-bit character string
   */
  NS_IMETHOD  GetWidth(const char* aString, PRUint32 aLength,
                       nscoord& aWidth) = 0;

  /**
   * Returns the width (in app units) of a Unicode character string
   */
  NS_IMETHOD  GetWidth(const PRUnichar *aString, PRUint32 aLength,
                       nscoord &aWidth) = 0;

  /**
   * Draw a string in the RenderingContext
   * @param aString The string to draw
   * @param aLength The length of the aString
   * @param aX Horizontal starting point of baseline
   * @param aY Vertical starting point of baseline.
   * @param aWidth Width of the underline
   */
  virtual void DrawString(const char *aString, PRUint32 aLength,
                          nscoord aX, nscoord aY,
                          nscoord aWidth) = 0;

  /**
   * Draw a string in the RenderingContext
   * @param aString A PRUnichar of the string
   * @param aLength The length of the aString
   * @param aX Horizontal starting point of baseline
   * @param aY Vertical starting point of baseline.
   * @param aWidth length in twips of the underline
   */
  virtual void DrawString(const PRUnichar *aString, PRUint32 aLength,
                          nscoord aX, nscoord aY,
                          nscoord aWidth) = 0;

  /**
   * Draw a string in the RenderingContext
   * @param aString A nsString of the string
   * @param aX Horizontal starting point of baseline
   * @param aY Vertical starting point of baseline.
   * @param aWidth Width of the underline
   */
  virtual void DrawString(const nsString& aString, nscoord aX, nscoord aY,
                          nscoord aWidth) = 0;

  /**
   * Copy an image to the RenderingContext
   * @param aX Horzontal left destination coordinate
   * @param aY Vertical top of destinatio coordinate
   */
  virtual void DrawImage(nsIImage *aImage, nscoord aX, nscoord aY) = 0;

  /**
   * Copy an image to the RenderingContext, scaling can occur if width/hieght does not match source
   * @param aX Horzontal left destination coordinate
   * @param aY Vertical top of destinatio coordinate
   * @param aWidth Width of destination, 
   * @param aHeight Height of destination
   */
  virtual void DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight) = 0; 
  /**
   * Copy an image to the RenderingContext, scaling can occur if source/dest rects differ
   * @param aRect Destination rectangle to copy the image to
   */
  virtual void DrawImage(nsIImage *aImage, const nsRect& aRect) = 0;
  /**
   * Copy an image to the RenderingContext, scaling/clipping can occur if source/dest rects differ
   * @param aSRect Source rectangle to copy from
   * @param aDRect Destination rectangle to copy the image to
   */
  virtual void DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)=0;

  /**
   * Copy offscreen pixelmap to this RenderingContext
   * @param aBounds Destination rectangle to copy to
   */
  NS_IMETHOD CopyOffScreenBits(nsRect &aBounds) = 0;
};

//modifiers for text rendering

#define NS_DRAWSTRING_NORMAL            0x0
#define NS_DRAWSTRING_UNDERLINE         0x1
#define NS_DRAWSTRING_OVERLINE          0x2
#define NS_DRAWSTRING_LINE_THROUGH      0x4

// Bit values for GetHints

/**
 * This bit, when set, indicates that the underlying rendering system
 * prefers 8 bit text rendering over PRUnichar text rendering. When this
 * bit is <b>not</b> set the opposite is true: the system prefers PRUnichar
 * rendering, not 8 bit rendering.
 */
#define NS_RENDERING_HINT_FAST_8BIT_TEXT   0x1

/**
 * This bit, when set, indicates that the rendering is being done
 * remotely.
 */
#define NS_RENDERING_HINT_REMOTE_RENDERING 0x2

#endif /* nsIRenderingContext_h___ */
