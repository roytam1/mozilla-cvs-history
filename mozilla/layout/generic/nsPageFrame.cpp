/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsPageFrame.h"
#include "nsHTMLParts.h"
#include "nsIContent.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIReflowCommand.h"
#include "nsIRenderingContext.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsLayoutAtoms.h"
#include "nsIStyleSet.h"
#include "nsIPresShell.h"
#include "nsIDeviceContext.h"
#include "nsReadableUtils.h"

// for page number localization formatting
#include "nsTextFormatter.h"

// Temporary
#include "nsIFontMetrics.h"

// Print Options
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);

// static data members
PRUnichar * nsPageFrame::mDateTimeStr   = nsnull;
nsFont *    nsPageFrame::mHeadFootFont  = nsnull;
PRUnichar * nsPageFrame::mPageNumFormat = nsnull;
PRUnichar * nsPageFrame::mPageNumAndTotalsFormat = nsnull;

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
#define DEBUG_PRINTING
#endif

#ifdef DEBUG_PRINTING
#define PRINT_DEBUG_MSG1(_msg1) fprintf(mDebugFD, (_msg1))
#define PRINT_DEBUG_MSG2(_msg1, _msg2) fprintf(mDebugFD, (_msg1), (_msg2))
#define PRINT_DEBUG_MSG3(_msg1, _msg2, _msg3) fprintf(mDebugFD, (_msg1), (_msg2), (_msg3))
#define PRINT_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) fprintf(mDebugFD, (_msg1), (_msg2), (_msg3), (_msg4))
#define PRINT_DEBUG_MSG5(_msg1, _msg2, _msg3, _msg4, _msg5) fprintf(mDebugFD, (_msg1), (_msg2), (_msg3), (_msg4), (_msg5))
#else //--------------
#define PRINT_DEBUG_MSG1(_msg) 
#define PRINT_DEBUG_MSG2(_msg1, _msg2) 
#define PRINT_DEBUG_MSG3(_msg1, _msg2, _msg3) 
#define PRINT_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) 
#define PRINT_DEBUG_MSG5(_msg1, _msg2, _msg3, _msg4, _msg5) 
#endif

nsresult
NS_NewPageFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsPageFrame* it = new (aPresShell) nsPageFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsPageFrame::nsPageFrame() :
  mSupressHF(PR_FALSE),
  mClipRect(-1, -1, -1, -1)

{
#ifdef NS_DEBUG
  mDebugFD = stdout;
#endif
  nsresult rv;
  mPrintOptions = do_GetService(kPrintOptionsCID, &rv);

  if (mHeadFootFont == nsnull) {
    mHeadFootFont = new nsFont("serif", NS_FONT_STYLE_NORMAL,NS_FONT_VARIANT_NORMAL,
                               NS_FONT_WEIGHT_NORMAL,0,NSIntPointsToTwips(10));
  }
  // now get the default font form the print options
  mPrintOptions->GetDefaultFont(*mHeadFootFont);
}

nsPageFrame::~nsPageFrame()
{
  if (mHeadFootFont != nsnull) {
    delete mHeadFootFont;
    mHeadFootFont = nsnull;
  }

  if (mDateTimeStr) {
    nsMemory::Free(mDateTimeStr);
    mDateTimeStr = nsnull;
  }

  if (mPageNumFormat) {
    nsMemory::Free(mPageNumFormat);
    mPageNumFormat = nsnull;
  }

  if (mPageNumAndTotalsFormat) {
    nsMemory::Free(mPageNumAndTotalsFormat);
    mPageNumAndTotalsFormat = nsnull;
  }
}

NS_METHOD nsPageFrame::Reflow(nsIPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageFrame", aReflowState.reason);
  DISPLAY_REFLOW(this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;  // initialize out parameter

  if (eReflowReason_Incremental == aReflowState.reason) {
    // We don't expect the target of the reflow command to be page frame
#ifdef NS_DEUG
    NS_ASSERTION(nsnull != aReflowState.reflowCommand, "null reflow command");

    nsIFrame* target;
    aReflowState.reflowCommand->GetTarget(target);
    NS_ASSERTION(target != this, "page frame is reflow command target");
#endif
  
    // Verify the next reflow command frame is our one and only child frame
    nsIFrame* next;
    aReflowState.reflowCommand->GetNext(next);
    NS_ASSERTION(next == mFrames.FirstChild(), "bad reflow frame");

    // Dispatch the reflow command to our frame
    nsSize            maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState,
                                     mFrames.FirstChild(), maxSize);
  
    kidReflowState.isTopOfPage = PR_TRUE;
    ReflowChild(mFrames.FirstChild(), aPresContext, aDesiredSize,
                kidReflowState, 0, 0, 0, aStatus);
  
    // Place and size the child. Make sure the child is at least as
    // tall as our max size (the containing window)
    if (aDesiredSize.height < aReflowState.availableHeight) {
      aDesiredSize.height = aReflowState.availableHeight;
    }

    FinishReflowChild(mFrames.FirstChild(), aPresContext, aDesiredSize,
                      0, 0, 0);

  } else {
    // Do we have any children?
    // XXX We should use the overflow list instead...
    if (mFrames.IsEmpty() && (nsnull != mPrevInFlow)) {
      nsPageFrame*  prevPage = (nsPageFrame*)mPrevInFlow;

      nsIFrame* prevLastChild = prevPage->mFrames.LastChild();

      // Create a continuing child of the previous page's last child
      nsIPresShell* presShell;
      nsIStyleSet*  styleSet;
      nsIFrame*     newFrame;

      aPresContext->GetShell(&presShell);
      presShell->GetStyleSet(&styleSet);
      NS_RELEASE(presShell);
      styleSet->CreateContinuingFrame(aPresContext, prevLastChild, this, &newFrame);
      NS_RELEASE(styleSet);
      mFrames.SetFrames(newFrame);
    }

    // Resize our frame allowing it only to be as big as we are
    // XXX Pay attention to the page's border and padding...
    if (mFrames.NotEmpty()) {
      nsIFrame* frame = mFrames.FirstChild();
      nsSize  maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame,
                                       maxSize);
      kidReflowState.isTopOfPage = PR_TRUE;

      // Get the child's desired size
      ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);

      // Make sure the child is at least as tall as our max size (the containing window)
      if (aDesiredSize.height < aReflowState.availableHeight) {
        aDesiredSize.height = aReflowState.availableHeight;
      }

      // Place and size the child
      FinishReflowChild(frame, aPresContext, aDesiredSize, 0, 0, 0);

      // Is the frame complete?
      if (NS_FRAME_IS_COMPLETE(aStatus)) {
        nsIFrame* childNextInFlow;

        frame->GetNextInFlow(&childNextInFlow);
        NS_ASSERTION(nsnull == childNextInFlow, "bad child flow list");
      }
    }
    PRINT_DEBUG_MSG2("PageFrame::Reflow %p ", this);
    PRINT_DEBUG_MSG5("[%d,%d][%d,%d]\n", aDesiredSize.width, aDesiredSize.height, aReflowState.availableWidth, aReflowState.availableHeight);

    // Return our desired size
    aDesiredSize.width = aReflowState.availableWidth;
    aDesiredSize.height = aReflowState.availableHeight;
  }
  PRINT_DEBUG_MSG2("PageFrame::Reflow %p ", this);
  PRINT_DEBUG_MSG3("[%d,%d]\n", aReflowState.availableWidth, aReflowState.availableHeight);

  return NS_OK;
}

NS_IMETHODIMP
nsPageFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::pageFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsPageFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Page", aResult);
}
#endif

NS_IMETHODIMP
nsPageFrame::IsPercentageBase(PRBool& aBase) const
{
  aBase = PR_TRUE;
  return NS_OK;
}

//------------------------------------------------------------------------------
// helper function for converting from char * to unichar
static PRUnichar *
GetUStr(const char * aCStr)
{
  return ToNewUnicode(nsDependentCString(aCStr));
}

// replace the &<code> with the value, but if the value is empty
// set the string to zero length
static void
SubstValueForCode(nsString& aStr, const PRUnichar * aUKey, const PRUnichar * aUStr)
{
  nsAutoString str;
  str = aUStr;
  if (str.Length() == 0) {
    aStr.SetLength(0);
  } else {
    aStr.ReplaceSubstring(aUKey, aUStr);
  }
}
// done with static helper functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void 
nsPageFrame::ProcessSpecialCodes(const nsString& aStr, nsString& aNewStr)
{

  aNewStr = aStr;

  // Search to see if the &D code is in the string 
  // then subst in the current date/time
  PRUnichar * kDate = GetUStr("&D");
  if (kDate != nsnull) {
    if (aStr.Find(kDate) > -1) {
      if (mDateTimeStr != nsnull) {
        aNewStr.ReplaceSubstring(kDate, mDateTimeStr);
      } else {
        aNewStr.ReplaceSubstring(kDate, NS_LITERAL_STRING("").get());
      }
      nsMemory::Free(kDate);
      return;
    }
    nsMemory::Free(kDate);
  }

  // NOTE: Must search for &PT before searching for &P
  //
  // Search to see if the "page number and page" total code are in the string
  // and replace the page number and page total code with the actual values
  PRUnichar * kPage = GetUStr("&PT");
  if (kPage != nsnull) {
    if (aStr.Find(kPage) > -1) {
      PRUnichar * uStr = nsTextFormatter::smprintf(mPageNumAndTotalsFormat, mPageNum, mTotNumPages);
      aNewStr.ReplaceSubstring(kPage, uStr);
      nsMemory::Free(uStr);
      nsMemory::Free(kPage);
      return;
    }
    nsMemory::Free(kPage);
  }

  // Search to see if the page number code is in the string
  // and replace the page number code with the actual values
  kPage = GetUStr("&P");
  if (kPage != nsnull) {
    if (aStr.Find(kPage) > -1) {
      PRUnichar * uStr = nsTextFormatter::smprintf(mPageNumFormat, mPageNum);
      aNewStr.ReplaceSubstring(kPage, uStr);
      nsMemory::Free(uStr);
      nsMemory::Free(kPage);
      return;
    }
    nsMemory::Free(kPage);
  }

  PRUnichar * kTitle = GetUStr("&T");
  if (kTitle != nsnull) {
    if (aStr.Find(kTitle) > -1) {
      PRUnichar * uTitle;
      mPrintOptions->GetTitle(&uTitle);   // creates memory
      SubstValueForCode(aNewStr, kTitle, uTitle);
      nsMemory::Free(uTitle);
      nsMemory::Free(kTitle);
      return;
    }
    nsMemory::Free(kTitle);
  }

  PRUnichar * kDocURL = GetUStr("&U");
  if (kDocURL != nsnull) {
    if (aStr.Find(kDocURL) > -1) {
      PRUnichar * uDocURL;
      mPrintOptions->GetDocURL(&uDocURL);   // creates memory
      SubstValueForCode(aNewStr, kDocURL, uDocURL);
      nsMemory::Free(uDocURL);
      nsMemory::Free(kDocURL);
      return;
    }
    nsMemory::Free(kDocURL);
  }
}


//------------------------------------------------------------------------------
nscoord nsPageFrame::GetXPosition(nsIRenderingContext& aRenderingContext, 
                                  const nsRect&        aRect, 
                                  PRInt32              aJust,
                                  const nsString&      aStr)
{
  PRInt32 width;
  aRenderingContext.GetWidth(aStr, width);

  nscoord x = aRect.x;
  switch (aJust) {
    case nsIPrintOptions::kJustLeft:
      // do nothing, already set
      break;

    case nsIPrintOptions::kJustCenter:
      x += (aRect.width - width) / 2;
      break;

    case nsIPrintOptions::kJustRight:
      x += aRect.width - width;
      break;
  } // switch

  return x;
}

//------------------------------------------------------------------------------
// Draw a Header or footer text lrft,right or center justified
// @parm aRenderingContext - rendering content ot draw into
// @parm aHeaderFooter - indicates whether it is a header or footer
// @parm aJust - indicates the justification of the text
// @parm aStr - The string to be drawn
// @parm aRect - the rect of the page
// @parm aHeight - the height of the text
// @parm aUseHalfThePage - indicates whether the text should limited to  the width
//                         of the entire page or just half the page
void
nsPageFrame::DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                              nsIFrame *           aFrame,
                              nsHeaderFooterEnum   aHeaderFooter,
                              PRInt32              aJust,
                              const nsString&      aStr1,
                              const nsString&      aStr2,
                              const nsString&      aStr3,
                              const nsRect&        aRect,
                              nscoord              aHeight)
{
  PRInt32 numStrs = 0;
  if (!aStr1.IsEmpty()) numStrs++;
  if (!aStr2.IsEmpty()) numStrs++;
  if (!aStr3.IsEmpty()) numStrs++;

  if (numStrs == 0) return;
  nscoord strSpace = aRect.width / numStrs;

  if (!aStr1.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aFrame, aHeaderFooter, nsIPrintOptions::kJustLeft, aStr1, aRect, aHeight, strSpace);
  }
  if (!aStr2.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aFrame, aHeaderFooter, nsIPrintOptions::kJustCenter, aStr2, aRect, aHeight, strSpace);
  }
  if (!aStr3.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aFrame, aHeaderFooter, nsIPrintOptions::kJustRight, aStr3, aRect, aHeight, strSpace);
  }
}

//------------------------------------------------------------------------------
// Draw a Header or footer text lrft,right or center justified
// @parm aRenderingContext - rendering content ot draw into
// @parm aHeaderFooter - indicates whether it is a header or footer
// @parm aJust - indicates the justification of the text
// @parm aStr - The string to be drawn
// @parm aRect - the rect of the page
// @parm aHeight - the height of the text
// @parm aWidth - available width for any one of the strings
void
nsPageFrame::DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                              nsIFrame *           aFrame,
                              nsHeaderFooterEnum   aHeaderFooter,
                              PRInt32              aJust,
                              const nsString&      aStr,
                              const nsRect&        aRect,
                              nscoord              aHeight,
                              nscoord              aWidth)
{

  // first make sure we have a vaild string and that the height of the
  // text will fit in the margin
  if (aStr.Length() > 0 && 
      ((aHeaderFooter == eHeader && aHeight < mMargin.top) ||
       (aHeaderFooter == eFooter && aHeight < mMargin.bottom))) {
    // measure the width of the text
    nsAutoString str;
    ProcessSpecialCodes(aStr, str);

    PRInt32 width;
    aRenderingContext.GetWidth(str, width);
    PRBool addEllipse = PR_FALSE;

    // trim the text and add the elipses if it won't fit
    while (width >= aWidth && str.Length() > 1) {
      str.SetLength(str.Length()-1);
      aRenderingContext.GetWidth(str, width);
      addEllipse = PR_TRUE;
    }
    if (addEllipse && str.Length() > 3) {
      str.SetLength(str.Length()-3);
      str.AppendWithConversion("...");
      aRenderingContext.GetWidth(str, width);
    }

    // cacl the x and y positions of the text
    nsRect rect(aRect);
    nscoord quarterInch = NS_INCHES_TO_TWIPS(0.25);
    rect.Deflate(quarterInch,0);
    nscoord x = GetXPosition(aRenderingContext, rect, aJust, str);
    nscoord y;
    if (aHeaderFooter == eHeader) {
      nscoord offset = ((mMargin.top - aHeight) / 2);
      y = rect.y - offset - aHeight;
      rect.Inflate(0, offset + aHeight);
    } else {
      nscoord offset = ((mMargin.bottom - aHeight) / 2);
      y = rect.y + rect.height + offset;
      rect.height += offset + aHeight;
    }

    // set up new clip and draw the text
    PRBool clipEmpty;
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(rect, nsClipCombine_kReplace, clipEmpty);
    aRenderingContext.DrawString(str, x, y);
    aRenderingContext.PopState(clipEmpty);
#ifdef DEBUG_PRINTING
    PRINT_DEBUG_MSG2("Page: %p", this);
    char * s = ToNewCString(str);
    if (s) {
      PRINT_DEBUG_MSG2(" [%s]", s);
      nsMemory::Free(s);
    }
    char justStr[64];
    switch (aJust) {
      case nsIPrintOptions::kJustLeft:strcpy(justStr, "Left");break;
      case nsIPrintOptions::kJustCenter:strcpy(justStr, "Center");break;
      case nsIPrintOptions::kJustRight:strcpy(justStr, "Right");break;
    } // switch
    PRINT_DEBUG_MSG2(" HF: %s ", aHeaderFooter==eHeader?"Header":"Footer");
    PRINT_DEBUG_MSG2(" JST: %s ", justStr);
    PRINT_DEBUG_MSG3(" x,y: %d,%d", x, y);
    PRINT_DEBUG_MSG2(" Hgt: %d \n", aHeight);
#endif
  }
}

//------------------------------------------------------------------------------
NS_IMETHODIMP
nsPageFrame::Paint(nsIPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nsFramePaintLayer    aWhichLayer,
                   PRUint32             aFlags)
{
  aRenderingContext.PushState();
  aRenderingContext.SetColor(NS_RGB(255,255,255));

  nsRect rect;
  PRBool clipEmpty;
  if (mClipRect.width != -1 || mClipRect.height != -1) {
#ifdef DEBUG_PRINTING
    if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
      printf("*** ClipRect: %5d,%5d,%5d,%5d\n", mClipRect.x, mClipRect.y, mClipRect.width, mClipRect.height);
    }
#endif
    mClipRect.x = 0;
    mClipRect.y = 0;
    aRenderingContext.SetClipRect(mClipRect, nsClipCombine_kReplace, clipEmpty);
    rect = mClipRect;
  } else {
    rect = mRect;
  }

  if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
    aRenderingContext.SetColor(NS_RGB(255,255,255));
    rect.x = 0;
    rect.y = 0;
    aRenderingContext.FillRect(rect);
  }

  nsresult rv = nsContainerFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer) {
    nsRect r;
    fprintf(mDebugFD, "PF::Paint    -> %p  SupHF: %s  Rect: [%5d,%5d,%5d,%5d]\n", this, 
            mSupressHF?"Yes":"No", mRect.x, mRect.y, mRect.width, mRect.height);
  }
#endif

  if (NS_FRAME_PAINT_LAYER_FOREGROUND == aWhichLayer && !mSupressHF) {
    // get the current margin
    mPrintOptions->GetMarginInTwips(mMargin);

    nsRect rect(0,0,mRect.width, mRect.height);

#if defined(DEBUG_rods) || defined(DEBUG_dcone)
    // XXX Paint a one-pixel border around the page so it's easy to see where
    // each page begins and ends when we're
    float   p2t;
    aPresContext->GetPixelsToTwips(&p2t);
    rect.Deflate(NSToCoordRound(p2t), NSToCoordRound(p2t));
    aRenderingContext.SetColor(NS_RGB(0, 0, 0));
    aRenderingContext.DrawRect(rect);
    rect.Inflate(NSToCoordRound(p2t), NSToCoordRound(p2t));
    fprintf(mDebugFD, "PageFr::PaintChild -> Painting Frame %p Page No: %d\n", this, mPageNum);
#endif

    // use the whole page
    rect.width  += mMargin.left + mMargin.right;
    rect.x      -= mMargin.left;
    
    aRenderingContext.SetFont(*mHeadFootFont);
    aRenderingContext.SetColor(NS_RGB(0,0,0));

    // Get the FontMetrics to determine width.height of strings
    nsCOMPtr<nsIDeviceContext> deviceContext;
    aPresContext->GetDeviceContext(getter_AddRefs(deviceContext));
    NS_ASSERTION(deviceContext, "Couldn't get the device context"); 
    nsCOMPtr<nsIFontMetrics> fontMet;
    deviceContext->GetMetricsFor(*mHeadFootFont, *getter_AddRefs(fontMet));
    nscoord visibleHeight = 0;
    if (fontMet) {
      fontMet->GetHeight(visibleHeight);
    }

    // print document headers and footers
    PRUnichar * headers[3];
    mPrintOptions->GetHeaderStrLeft(&headers[0]);   // creates memory
    mPrintOptions->GetHeaderStrCenter(&headers[1]); // creates memory
    mPrintOptions->GetHeaderStrRight(&headers[2]);  // creates memory
    DrawHeaderFooter(aRenderingContext, this, eHeader, nsIPrintOptions::kJustLeft, 
                     nsAutoString(headers[0]), nsAutoString(headers[1]), nsAutoString(headers[2]), 
                     rect, visibleHeight);
    PRInt32 i;
    for (i=0;i<3;i++) nsMemory::Free(headers[i]);

    PRUnichar * footers[3];
    mPrintOptions->GetFooterStrLeft(&footers[0]);   // creates memory
    mPrintOptions->GetFooterStrCenter(&footers[1]); // creates memory
    mPrintOptions->GetFooterStrRight(&footers[2]);  // creates memory
    DrawHeaderFooter(aRenderingContext, this, eFooter, nsIPrintOptions::kJustRight, 
                     nsAutoString(footers[0]), nsAutoString(footers[1]), nsAutoString(footers[2]), 
                     rect, visibleHeight);
    for (i=0;i<3;i++) nsMemory::Free(footers[i]);

  }

  aRenderingContext.PopState(clipEmpty);
  return rv;
}

//------------------------------------------------------------------------------
void
nsPageFrame::SetPrintOptions(nsIPrintOptions * aPrintOptions) 
{ 
  NS_ASSERTION(aPrintOptions != nsnull, "Print Options can not be null!");

  mPrintOptions = aPrintOptions;
  // create a default font
  mHeadFootFont = new nsFont("serif", NS_FONT_STYLE_NORMAL,NS_FONT_VARIANT_NORMAL,
                             NS_FONT_WEIGHT_NORMAL,0,NSIntPointsToTwips(10));
  // now get the default font form the print options
  mPrintOptions->GetDefaultFont(*mHeadFootFont);
}

//------------------------------------------------------------------------------
void
nsPageFrame::SetPageNumInfo(PRInt32 aPageNumber, PRInt32 aTotalPages) 
{ 
  mPageNum     = aPageNumber; 
  mTotNumPages = aTotalPages;
}


//------------------------------------------------------------------------------
void
nsPageFrame::SetPageNumberFormat(PRUnichar * aFormatStr, PRBool aForPageNumOnly)
{ 
  NS_ASSERTION(aFormatStr != nsnull, "Format string cannot be null!");

  if (aForPageNumOnly) {
    if (mPageNumFormat != nsnull) {
      nsMemory::Free(mPageNumFormat);
    }
    mPageNumFormat = aFormatStr;
  } else {
    if (mPageNumAndTotalsFormat != nsnull) {
      nsMemory::Free(mPageNumAndTotalsFormat);
    }
    mPageNumAndTotalsFormat = aFormatStr;
  }
}

//------------------------------------------------------------------------------
void
nsPageFrame::SetDateTimeStr(PRUnichar * aDateTimeStr)
{ 
  NS_ASSERTION(aDateTimeStr != nsnull, "DateTime string cannot be null!");

  if (mDateTimeStr != nsnull) {
    nsMemory::Free(mDateTimeStr);
  }
  mDateTimeStr = aDateTimeStr;
}
