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

#ifndef nsFormControlHelper_h___
#define nsFormControlHelper_h___

#include "nsIFormControlFrame.h"
#include "nsIFormManager.h"
#include "nsISupports.h"
#include "nsIWidget.h"
#include "nsLeafFrame.h"
#include "nsCoord.h"
#include "nsIStyleContext.h"
#include "nsIPresContext.h"

class nsIView;
//class nsIPresContext;
class nsStyleCoord;
class nsFormFrame;
//class nsIStyleContext;

#define CSS_NOTSET -1
#define ATTR_NOTSET -1

#define NS_STRING_TRUE   "1"
#define NS_STRING_FALSE  "0"

/**
  * Enumeration of possible mouse states used to detect mouse clicks
  */
enum nsMouseState {
  eMouseNone,
  eMouseEnter,
  eMouseExit,
  eMouseDown,
  eMouseUp
};

struct nsInputDimensionSpec
{
  nsIAtom*  mColSizeAttr;            // attribute used to determine width
  PRBool    mColSizeAttrInPixels;    // is attribute value in pixels (otherwise num chars)
  nsIAtom*  mColValueAttr;           // attribute used to get value to determine size
                                     //    if not determined above
  nsString* mColDefaultValue;        // default value if not determined above
  nscoord   mColDefaultSize;         // default width if not determined above
  PRBool    mColDefaultSizeInPixels; // is default width in pixels (otherswise num chars)
  nsIAtom*  mRowSizeAttr;            // attribute used to determine height
  nscoord   mRowDefaultSize;         // default height if not determined above

  nsInputDimensionSpec(nsIAtom* aColSizeAttr, PRBool aColSizeAttrInPixels, 
                       nsIAtom* aColValueAttr, nsString* aColDefaultValue,
                       nscoord aColDefaultSize, PRBool aColDefaultSizeInPixels,
                       nsIAtom* aRowSizeAttr, nscoord aRowDefaultSize)
                       : mColSizeAttr(aColSizeAttr), mColSizeAttrInPixels(aColSizeAttrInPixels),
                         mColValueAttr(aColValueAttr), 
                         mColDefaultValue(aColDefaultValue), mColDefaultSize(aColDefaultSize),
                         mColDefaultSizeInPixels(aColDefaultSizeInPixels),
                         mRowSizeAttr(aRowSizeAttr), mRowDefaultSize(aRowDefaultSize)
  {
  }

};



/** 
  * nsFormControlHelper is the base class for frames of form controls. It
  * provides a uniform way of creating widgets, resizing, and painting.
  * @see nsLeafFrame and its base classes for more info
  */
class nsFormControlHelper 
{

public:
  

  static nscoord CalculateSize (nsIPresContext* aPresContext, nsIFormControlFrame* aFrame,
                                const nsSize& aCSSSize, nsInputDimensionSpec& aDimensionSpec, 
                                nsSize& aBounds, PRBool& aWidthExplicit, 
                                PRBool& aHeightExplicit, nscoord& aRowSize,
                                nsIRenderingContext *aRendContext);

  

  static nscoord GetTextSize(nsIPresContext& aContext, nsIFormControlFrame* aFrame,
                             const nsString& aString, nsSize& aSize,
                             nsIRenderingContext *aRendContext);

  static nscoord GetTextSize(nsIPresContext& aContext, nsIFormControlFrame* aFrame,
                             PRInt32 aNumChars, nsSize& aSize,
                             nsIRenderingContext *aRendContext);

  static void GetFont(nsIFormControlFrame *   aFormFrame,
                      nsIPresContext*        aPresContext, 
                       nsIStyleContext * aStyleContext, 
                      nsFont&                aFont);

  static void ForceDrawFrame(nsIFrame * aFrame);

 
 /** 
  * Utility to convert a string to a PRBool
  * @param aValue string to convert to a PRBool
  * @returns PR_TRUE if aValue = "1", PR_FALSE otherwise
  */

  static PRBool GetBool(const nsString& aValue);

 /** 
  * Utility to convert a PRBool to a string
  * @param aValue Boolean value to convert to string.
  * @param aResult string to hold the boolean value. It is set to "1" 
  *        if aValue equals PR_TRUE, "0" if aValue equals PR_FALSE.

  */

  static void  GetBoolString(const PRBool aValue, nsString& aResult);

  // XXX similar functionality needs to be added to widget library and these
  //     need to change to use it.
  static  nscoord GetScrollbarWidth(float aPixToTwip);

  static nsCompatibility GetRepChars(nsIPresContext& aPresContext, char& char1, char& char2);

//
//-------------------------------------------------------------------------------------
//  Utility methods for managing checkboxes and radiobuttons
//-------------------------------------------------------------------------------------
//   
   /**
    * Get the state of the checked attribute.
    * @param aState set to PR_TRUE if the checked attribute is set,
    * PR_FALSE if the checked attribute has been removed
    * @returns NS_OK or NS_CONTENT_ATTR_HAS_VALUE
    */

  nsresult GetCurrentCheckState(PRBool* aState);
 
   /**
    * Set the state of the checked attribute.
    * @param aState set to PR_TRUE to set the attribute,
    * PR_FALSE to unset the attribute
    * @returns NS_OK or NS_CONTENT_ATTR_HAS_VALUE
    */

  nsresult SetCurrentCheckState(PRBool aState);

   /**
    * Get the state of the defaultchecked attribute.
    * @param aState set to PR_TRUE if the defaultchecked attribute is set,
    * PR_FALSE if the defaultchecked attribute has been removed
    * @returns NS_OK or NS_CONTENT_ATTR_HAS_VALUE
    */
 
  nsresult GetDefaultCheckState(PRBool* aState);


//
//-------------------------------------------------------------------------------------
// Utility methods for rendering Form Elements using GFX
//-------------------------------------------------------------------------------------
//
// XXX: The following location for the paint code is TEMPORARY. 
// It is being used to get printing working
// under windows. Later it will be used to GFX-render the controls to the display. 
// Expect this code to repackaged and moved to a new location in the future.

   /**
    * Enumeration of possible mouse states used to detect mouse clicks
    */
   enum nsArrowDirection {
    eArrowDirection_Left,
    eArrowDirection_Right,
    eArrowDirection_Up,
    eArrowDirection_Down
   };

   /**
    * Scale, translate and convert an arrow of poitns from nscoord's to nsPoints's.
    *
    * @param aNumberOfPoints number of (x,y) pairs
    * @param aPoints arrow of points to convert
    * @param aScaleFactor scale factor to apply to each points translation.
    * @param aX x coordinate to add to each point after scaling
    * @param aY y coordinate to add to each point after scaling
    * @param aCenterX x coordinate of the center point in the original array of points.
    * @param aCenterY y coordiante of the center point in the original array of points.
    */

   static void SetupPoints(PRUint32 aNumberOfPoints, nscoord* aPoints, 
     nsPoint* aPolygon, nscoord aScaleFactor, nscoord aX, nscoord aY,
     nscoord aCenterX, nscoord aCenterY);

   /**
    * Paint a fat line. The line is drawn as a polygon with a specified width.
	  *  
    * @param aRenderingContext the rendering context
    * @param aSX starting x in pixels
	  * @param aSY starting y in pixels
	  * @param aEX ending x in pixels
	  * @param aEY ending y in pixels
    * @param aHorz PR_TRUE if aWidth is added to x coordinates to form polygon. If 
	  *              PR_FALSE  then aWidth as added to the y coordinates.
    * @param aOnePixel number of twips in a single pixel.
    */

  static void PaintLine(nsIRenderingContext& aRenderingContext, 
                 nscoord aSX, nscoord aSY, nscoord aEX, nscoord aEY, 
                 PRBool aHorz, nscoord aWidth, nscoord aOnePixel);

   /**
    * Draw an arrow glyph. 
	  * 
    * @param aRenderingContext the rendering context
    * @param aSX upper left x coordinate pixels
   	* @param aSY upper left y coordinate pixels
    * @param aType @see nsArrowDirection enumeration 
    * @param aOnePixel number of twips in a single pixel.
    */

  static void PaintArrowGlyph(nsIRenderingContext& aRenderingContext, 
                              nscoord aSX, nscoord aSY, nsArrowDirection aArrowDirection, 
                              nscoord aOnePixel);

   /**
    * Draw an arrow 
   	* 
    * @param aArrowDirection @see nsArrowDirection enumeration
    * @param aRenderingContext the rendering context
		* @param aPresContext the presentation context
		* @param aDirtyRect rectangle requiring update
    * @param aOnePixel number of TWIPS in a single pixel
		* @param aColor color of the arrow glph
		* @param aSpacing spacing for the arrow background
		* @param aForFrame frame which the arrow will be rendered into.
    * @param aFrameRect rectangle for the frame specified by aForFrame
    */

  static void PaintArrow(nsArrowDirection aArrowDirection,
			 nsIRenderingContext& aRenderingContext,
			 nsIPresContext& aPresContext, 
		 	 const nsRect& aDirtyRect,
                         nsRect& aRect, 
			 nscoord aOnePixel, 
                         nsIStyleContext* aArrowStyle,
			 const nsStyleSpacing& aSpacing,
			 nsIFrame* aForFrame,
                         nsRect& aFrameRect);
   /**
    * Paint a scrollbar
	  * 
    * @param aRenderingContext the rendering context
		* @param aPresContext the presentation context
		* @param aDirtyRect rectangle requiring update
    * @param aRect width and height of the scrollbar
		* @param aHorizontal if TRUE scrollbar is drawn horizontally, vertical if FALSE
		* @param aOnePixel number TWIPS per pixel
    * @param aScrollbarStyleContext style context for the scrollbar
    * @param aScrollbarArrowStyleContext style context for the scrollbar arrow
		* @param aForFrame the frame that the scrollbar will be rendered in to
    * @param aFrameRect the rectangle for the frame passed as aForFrame
    */

  static void PaintScrollbar(nsIRenderingContext& aRenderingContext,
																	nsIPresContext& aPresContext, 
																  const nsRect& aDirtyRect,
                                  nsRect& aRect, 
																  PRBool aHorizontal, 
																  nscoord aOnePixel, 
                                  nsIStyleContext* aScrollbarStyleContext,
                                  nsIStyleContext* aScrollbarArrowStyleContext,
																	nsIFrame* aForFrame,
                                  nsRect& aFrameRect);
   /**
    * Paint a fixed size checkmark
	  * 
    * @param aRenderingContext the rendering context
	  * @param aPixelsToTwips scale factor for convering pixels to twips.
    */

  static void PaintFixedSizeCheckMark(nsIRenderingContext& aRenderingContext, 
                                     float aPixelsToTwips);
                       
  /**
    * Paint a checkmark
	  * 
    * @param aRenderingContext the rendering context
	  * @param aPixelsToTwips scale factor for convering pixels to twips.
    * @param aWidth width in twips
    * @param aHeight height in twips
    */

  static void PaintCheckMark(nsIRenderingContext& aRenderingContext,
                             float aPixelsToTwips, PRUint32 aWidth, PRUint32 aHeight);

   /**
    * Paint a fixed size checkmark border
	  * 
    * @param aRenderingContext the rendering context
	  * @param aPixelsToTwips scale factor for convering pixels to twips.
    * @param aBackgroundColor color for background of the checkbox 
    */

  static void PaintFixedSizeCheckMarkBorder(nsIRenderingContext& aRenderingContext,
                         float aPixelsToTwips, const nsStyleColor& aBackgroundColor);

   /**
    * Paint a rectangular button. Includes background, string, and focus indicator
	  * 
    * @param aPresContext the presentation context
    * @param aRenderingContext the rendering context
    * @param aDirtyRect rectangle requiring update
    * @param aWidth width the checkmark border in TWIPS
    * @param aHeight height of the checkmark border in TWIPS
    * @param aShift if PR_TRUE offset button as if it were pressed
    * @param aShowFocus if PR_TRUE draw focus rectangle over button
    * @param aStyleContext style context used for drawing button background 
    * @param aLabel label for button
    * @param aForFrame the frame that the scrollbar will be rendered in to
    */

  static void PaintRectangularButton(nsIPresContext& aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect, PRUint32 aWidth, 
                            PRUint32 aHeight, PRBool aShift, PRBool aShowFocus,
                            nsIStyleContext* aStyleContext, nsString& aLabel, 
                            nsIFrame* aForFrame);
   /**
    * Paint a focus indicator.
	  * 
    * @param aRenderingContext the rendering context
    * @param aDirtyRect rectangle requiring update
	  * @param aInside border inside
    * @param aOutside border outside
    */

  static void PaintFocus(nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect, nsRect& aInside, nsRect& aOutside);

   /**
    * Get the rectangle for a circular area. To maintain the aspect ratio of the circular
    * area the rectangle is offset to center the circular area within the width and height
    * specified.
	  * 
    * @param aWidth width to center within
    * @param aHeight height to center within 
    * @param aRect the computed rectangle centering the circle by setting the x and y of the rect.
    */

  static void GetCircularRect(PRUint32 aWidth, PRUint32 aHeight, nsRect& aRect);

   /**
    * Paint a circular background
	  * 
    * @param aPresContext the presentation context
    * @param aRenderingContext the rendering context
    * @param aDirtyRect rectangle requiring update
    * @param aStyleContext style context specifying colors and spacing
    * @param aInset if PR_TRUE draw inset, otherwise draw outset
    * @param aForFrame the frame that the scrollbar will be rendered in to
    * @param aWidth width of the border in TWIPS
    * @param aHeight height ofthe border in TWIPS
    */

  static void PaintCircularBackground(nsIPresContext& aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect, nsIStyleContext* aStyleContext, PRBool aInset,
                         nsIFrame* aForFrame, PRUint32 aWidth, PRUint32 aHeight);


   /**
    * Paint a circular border
	  * 
    * @param aPresContext the presentation context
    * @param aRenderingContext the rendering context
    * @param aDirtyRect rectangle requiring update
    * @param aStyleContext style context specifying colors and spacing
    * @param aInset if PR_TRUE draw inset, otherwise draw outset
    * @param aForFrame the frame that the scrollbar will be rendered in to
    * @param aWidth width of the border in TWIPS
    * @param aHeight height ofthe border in TWIPS
    */

  static void PaintCircularBorder(nsIPresContext& aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect, nsIStyleContext* aStyleContext, PRBool aInset,
                         nsIFrame* aForFrame, PRUint32 aWidth, PRUint32 aHeight);



protected:
  nsFormControlHelper();
  virtual ~nsFormControlHelper();


};

#endif

