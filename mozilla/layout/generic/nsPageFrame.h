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
#ifndef nsPageFrame_h___
#define nsPageFrame_h___

#include "nsContainerFrame.h"
#include "nsIPrintOptions.h"

// Page frame class used by the simple page sequence frame
class nsPageFrame : public nsContainerFrame {
public:
  friend nsresult NS_NewPageFrame(nsIPresShell* aPresShell, nsIFrame** aResult);

  // nsIFrame
  NS_IMETHOD  Reflow(nsIPresContext*      aPresContext,
                     nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aMaxSize,
                     nsReflowStatus&      aStatus);

  NS_IMETHOD  Paint(nsIPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer,
                    PRUint32             aFlags = 0);

  NS_IMETHOD IsPercentageBase(PRBool& aBase) const;

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::pageFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  
#ifdef NS_DEBUG
  // Debugging
  NS_IMETHOD  GetFrameName(nsString& aResult) const;
  void SetDebugFD(FILE* aFD) { mDebugFD = aFD; }
  FILE * mDebugFD;
#endif

  //////////////////
  // For Printing
  //////////////////

  // Set the print options object into the page for printing
  virtual void  SetPrintOptions(nsIPrintOptions * aPrintOptions);

  // Tell the page which page number it is out of how many
  virtual void  SetPageNumInfo(PRInt32 aPageNumber, PRInt32 aTotalPages);

  virtual void  SuppressHeadersAndFooters(PRBool aDoSup) { mSupressHF = aDoSup; }
  virtual void  SetClipRect(nsRect* aClipRect)           { mClipRect = *aClipRect; }

  // This is class is now responsible for freeing the memory
  static void SetPageNumberFormat(PRUnichar * aFormatStr);

protected:
  nsPageFrame();
  virtual ~nsPageFrame();

  typedef enum {
    eHeader,
    eFooter
  } nsHeaderFooterEnum;

  nscoord GetXPosition(nsIRenderingContext& aRenderingContext, 
                       const nsRect&        aRect, 
                       PRInt32              aJust,
                       const nsString&      aStr);

  void DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                        nsIFrame *           aFrame,
                        nsHeaderFooterEnum   aHeaderFooter,
                        PRInt32              aJust,
                        const nsString&      sStr,
                        const nsRect&        aRect,
                        nscoord              aHeight,
                        PRBool               aUseHalfThePage = PR_TRUE);


  nsCOMPtr<nsIPrintOptions> mPrintOptions;
  PRInt32     mPageNum;
  PRInt32     mTotNumPages;
  nsMargin    mMargin;
  nsFont *    mHeadFootFont;

  PRPackedBool mSupressHF;
  nsRect       mClipRect;

  static PRUnichar * mPageNumFormat;

};

#endif /* nsPageFrame_h___ */

