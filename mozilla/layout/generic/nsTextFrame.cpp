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
#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsCRT.h"
#include "nsSplittableFrame.h"
#include "nsLineLayout.h"
#include "nsString.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsStyleConsts.h"
#include "nsIStyleContext.h"
#include "nsCoord.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"
#include "nsHTMLIIDs.h"
#include "nsIPresShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsITimerCallback.h"
#include "nsITimer.h"
#include "prtime.h"
#include "nsVoidArray.h"
#include "prprf.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsIFocusTracker.h"
#include "nsICaret.h"
#include "nsXIFConverter.h"
#include "nsHTMLAtoms.h"
#include "nsILineBreaker.h"
#include "nsIWordBreaker.h"

#include "nsITextContent.h"
#include "nsTextRun.h"
#include "nsTextFragment.h"
#include "nsTextTransformer.h"
#include "nsLayoutAtoms.h"
#include "nsIFrameSelection.h"
#include "nsIDOMSelection.h"
#include "nsIDOMRange.h"

#include "nsILineIterator.h"

#ifndef PR_ABS
#define PR_ABS(x) (x < 0 ? -x : x)
#endif

static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);

#ifdef NS_DEBUG
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#else
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#endif

// #define DEBUGWORDJUMP

#define WORD_BUF_SIZE 100
#define TEXT_BUF_SIZE 1000

// Helper class for managing blinking text

class nsBlinkTimer : public nsITimerCallback {
public:
  nsBlinkTimer();
  virtual ~nsBlinkTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsIFrame* aFrame);

  PRBool RemoveFrame(nsIFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  virtual void Notify(nsITimer *timer);

  nsITimer* mTimer;
  nsVoidArray mFrames;
};

static PRBool gBlinkTextOff;
static nsBlinkTimer* gTextBlinker;
#ifdef NOISY_BLINK
static PRTime gLastTick;
#endif

nsBlinkTimer::nsBlinkTimer()
{
  NS_INIT_REFCNT();
  mTimer = nsnull;
}

nsBlinkTimer::~nsBlinkTimer()
{
  Stop();
}

void nsBlinkTimer::Start()
{
  nsresult rv = NS_NewTimer(&mTimer);
  if (NS_OK == rv) {
    mTimer->Init(this, 750);
  }
}

void nsBlinkTimer::Stop()
{
  if (nsnull != mTimer) {
    mTimer->Cancel();
    NS_RELEASE(mTimer);
  }
}

static NS_DEFINE_IID(kITimerCallbackIID, NS_ITIMERCALLBACK_IID);
NS_IMPL_ISUPPORTS(nsBlinkTimer, kITimerCallbackIID);

void nsBlinkTimer::AddFrame(nsIFrame* aFrame) {
  mFrames.AppendElement(aFrame);
  if (1 == mFrames.Count()) {
    Start();
  }
}

PRBool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  PRBool rv = mFrames.RemoveElement(aFrame);
  if (0 == mFrames.Count()) {
    Stop();
  }
  return rv;
}

PRInt32 nsBlinkTimer::FrameCount() {
  return mFrames.Count();
}

void nsBlinkTimer::Notify(nsITimer *timer)
{
  // Toggle blink state bit so that text code knows whether or not to
  // render. All text code shares the same flag so that they all blink
  // in unison.
  gBlinkTextOff = PRBool(!gBlinkTextOff);

  // XXX hack to get auto-repeating timers; restart before doing
  // expensive work so that time between ticks is more even
  Stop();
  Start();

#ifdef NOISY_BLINK
  PRTime now = PR_Now();
  char buf[50];
  PRTime delta;
  LL_SUB(delta, now, gLastTick);
  gLastTick = now;
  PR_snprintf(buf, sizeof(buf), "%lldusec", delta);
  printf("%s\n", buf);
#endif

  PRInt32 i, n = mFrames.Count();
  for (i = 0; i < n; i++) {
    nsIFrame* text = (nsIFrame*) mFrames.ElementAt(i);

    // Determine damaged area and tell view manager to redraw it
    nsPoint offset;
    nsRect bounds;
    text->GetRect(bounds);
    nsIView* view;
    text->GetOffsetFromView(offset, &view);
    nsIViewManager* vm;
    view->GetViewManager(vm);
    bounds.x = offset.x;
    bounds.y = offset.y;
    vm->UpdateView(view, bounds, 0);
    NS_RELEASE(vm);
  }
}

//----------------------------------------------------------------------

class nsTextFrame : public nsSplittableFrame {
public:
  nsTextFrame();

  // nsIFrame
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  NS_IMETHOD GetCursor(nsIPresContext& aPresContext,
                       nsPoint& aPoint,
                       PRInt32& aCursor);

  NS_IMETHOD ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent);

  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::textFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  
  NS_IMETHOD GetFrameName(nsString& aResult) const;

#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

  NS_IMETHOD GetPosition(nsIPresContext& aCX,
                         nscoord         aXCoord,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd);

  NS_IMETHOD GetPositionSlowly(nsIPresContext& aCX,
                         nsIRenderingContext * aRendContext,
                         nscoord         aXCoord,
                         nsIContent **      aNewContent,
                         PRInt32&       aOffset);


  NS_IMETHOD SetSelected(nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread);

  NS_IMETHOD PeekOffset(nsPeekOffsetStruct *aPos);

  NS_IMETHOD HandleMultiplePress(nsIPresContext& aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus&  aEventStatus);

  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end)const;

  NS_IMETHOD GetPointFromOffset(nsIPresContext*         inPresContext,
                                nsIRenderingContext*    inRendContext,
                                PRInt32                 inOffset,
                                nsPoint*                outPoint);
                                
  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32     inContentOffset,
                                PRBool                  inHint,
                                PRInt32*                outFrameContentOffset,
                                nsIFrame*               *outChildFrame);

  // nsIHTMLReflow
  NS_IMETHOD FindTextRuns(nsLineLayout& aLineLayout);
  NS_IMETHOD Reflow(nsIPresContext& aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  NS_IMETHOD AdjustFrameSize(nscoord aExtraSpace, nscoord& aUsedSpace);
  NS_IMETHOD TrimTrailingWhiteSpace(nsIPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth);

  struct TextStyle {
    const nsStyleFont* mFont;
    const nsStyleText* mText;
    const nsStyleColor* mColor;
    nsIFontMetrics* mNormalFont;
    nsIFontMetrics* mSmallFont;
    nsIFontMetrics* mLastFont;
    PRBool mSmallCaps;
    nscoord mWordSpacing;
    nscoord mLetterSpacing;
    nscolor mSelectionTextColor;
    nscolor mSelectionBGColor;
    nscoord mSpaceWidth;
    PRBool mJustifying;
    PRBool mPreformatted;
    PRIntn mNumSpaces;
    nscoord mExtraSpacePerSpace;
    nscoord mRemainingExtraSpace;

    TextStyle(nsIPresContext* aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsIStyleContext* sc)
    {
      // Get style data
      mColor = (const nsStyleColor*) sc->GetStyleData(eStyleStruct_Color);
      mFont = (const nsStyleFont*) sc->GetStyleData(eStyleStruct_Font);
      mText = (const nsStyleText*) sc->GetStyleData(eStyleStruct_Text);

      // Cache the original decorations and reuse the current font
      // to query metrics, rather than creating a new font which is expensive.
      nsFont* plainFont = (nsFont *)&mFont->mFont; //XXX: Change to use a CONST_CAST macro.
      PRUint8 originalDecorations = plainFont->decorations;
      plainFont->decorations = NS_FONT_DECORATION_NONE;
      aPresContext->GetMetricsFor(*plainFont, &mNormalFont);
      aRenderingContext.SetFont(mNormalFont);
      aRenderingContext.GetWidth(' ', mSpaceWidth);
      mLastFont = mNormalFont;

      // Get the small-caps font if needed
      mSmallCaps = NS_STYLE_FONT_VARIANT_SMALL_CAPS == plainFont->variant;
      if (mSmallCaps) {
        nscoord originalSize = plainFont->size;
        plainFont->size = nscoord(0.7 * plainFont->size);
        aPresContext->GetMetricsFor(*plainFont, &mSmallFont);
        // Reset to the size value saved earlier.
        plainFont->size = originalSize;
      }
      else {
        mSmallFont = nsnull;
      }

      // Reset to the decoration saved earlier
      plainFont->decorations = originalDecorations; 

      // XXX Get these from style
      mSelectionBGColor = NS_RGB(0, 0, 0);
      mSelectionTextColor = NS_RGB(255, 255, 255);

      // Get the word and letter spacing
      mWordSpacing = 0;
      mLetterSpacing = 0;
      PRIntn unit = mText->mWordSpacing.GetUnit();
      if (eStyleUnit_Coord == unit) {
        mWordSpacing = mText->mWordSpacing.GetCoordValue();
      }
      unit = mText->mLetterSpacing.GetUnit();
      if (eStyleUnit_Coord == unit) {
        mLetterSpacing = mText->mLetterSpacing.GetCoordValue();
      }
      mNumSpaces = 0;
      mRemainingExtraSpace = 0;
      mExtraSpacePerSpace = 0;
      mPreformatted = (NS_STYLE_WHITESPACE_PRE == mText->mWhiteSpace) ||
        (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == mText->mWhiteSpace);
    }

    ~TextStyle() {
      NS_RELEASE(mNormalFont);
      NS_IF_RELEASE(mSmallFont);
    }
  };

  nsIDocument* GetDocument(nsIPresContext* aPresContext);

  PRIntn PrepareUnicodeText(nsTextTransformer& aTransformer,
                            PRInt32* aIndicies,
                            PRUnichar* aBuffer,
                            PRInt32* aTextLen);

  void PaintTextDecorations(nsIRenderingContext& aRenderingContext,
                            nsIStyleContext* aStyleContext,
                            TextStyle& aStyle,
                            nscoord aX, nscoord aY, nscoord aWidth,
                            PRUnichar* aText = nsnull,
                            SelectionDetails *aDetails = nsnull,
                            PRUint32 aIndex = 0,
                            PRUint32 aLength = 0,
                            const nscoord* aSpacing = nsnull);

  void PaintTextSlowly(nsIPresContext* aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       nsIStyleContext* aStyleContext,
                       TextStyle& aStyle,
                       nscoord aX, nscoord aY);

  void RenderString(nsIRenderingContext& aRenderingContext,
                    nsIStyleContext* aStyleContext,
                    TextStyle& aStyle,
                    PRUnichar* aBuffer, PRInt32 aLength,
                    nscoord aX, nscoord aY,
                    nscoord aWidth,
                    SelectionDetails *aDetails = nsnull);

  void MeasureSmallCapsText(const nsHTMLReflowState& aReflowState,
                            TextStyle& aStyle,
                            PRUnichar* aWord,
                            PRInt32 aWordLength,
                            nscoord* aWidthResult);

  void GetWidth(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord* aWidthResult);

  void PaintUnicodeText(nsIPresContext* aPresContext,
                        nsIRenderingContext& aRenderingContext,
                        nsIStyleContext* aStyleContext,
                        TextStyle& aStyle,
                        nscoord dx, nscoord dy);

  void PaintAsciiText(nsIPresContext* aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      nsIStyleContext* aStyleContext,
                      TextStyle& aStyle,
                      nscoord dx, nscoord dy);

  nscoord ComputeTotalWordWidth(nsIPresContext* aPresContext,
                                nsILineBreaker* aLineBreaker,
                                nsLineLayout& aLineLayout,
                                const nsHTMLReflowState& aReflowState,
                                nsIFrame* aNextFrame,
                                nscoord aBaseWidth,
                                const PRUnichar* aWordBuf,
                                PRUint32   aWordBufLen,
                                PRUint32   aWordBufSize);

  nscoord ComputeWordFragmentWidth(nsIPresContext* aPresContext,
                                   nsILineBreaker* aLineBreaker,
                                   nsLineLayout& aLineLayout,
                                   const nsHTMLReflowState& aReflowState,
                                   nsIFrame* aNextFrame,
                                   nsIContent* aContent,
                                   nsITextContent* aText,
                                   PRBool* aStop,
                                   const PRUnichar* aWordBuf,
                                   PRUint32 &aWordBufLen,
                                   PRUint32 aWordBufSize);

  void ToCString(nsString& aBuf, PRInt32* aContentLength) const;

protected:
  virtual ~nsTextFrame();

  PRInt32 mContentOffset;
  PRInt32 mContentLength;
  PRInt32 mColumn;
  nscoord mComputedWidth;
};

// Flag information used by rendering code. This information is
// computed by the ResizeReflow code. The flags are stored in the
// mState variable in the frame class private section.

// Flag indicating that whitespace was skipped
#define TEXT_SKIP_LEADING_WS 0x01000000

#define TEXT_HAS_MULTIBYTE   0x02000000

#define TEXT_IN_WORD         0x04000000

#define TEXT_TRIMMED_WS      0x08000000

// This bit is set on the first frame in a continuation indicating
// that it was chopped short because of :first-letter style.
#define TEXT_FIRST_LETTER    0x10000000

// Bits in mState used for reflow flags
#define TEXT_REFLOW_FLAGS    0x7F000000

#define TEXT_BLINK_ON        0x80000000

//----------------------------------------------------------------------

nsresult
NS_NewTextFrame(nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTextFrame* it = new nsTextFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsTextFrame::nsTextFrame()
  : nsSplittableFrame()
{
  if (nsnull == gTextBlinker) {
    // Create text timer the first time out
    gTextBlinker = new nsBlinkTimer();
  }
  NS_ADDREF(gTextBlinker);
}

nsTextFrame::~nsTextFrame()
{
  if (0 != (mState & TEXT_BLINK_ON)) {
    NS_ASSERTION(nsnull != gTextBlinker, "corrupted blinker");
    gTextBlinker->RemoveFrame(this);
  }
  if (0 == gTextBlinker->Release()) {
    // Release text timer when the last text frame is gone
    gTextBlinker = nsnull;
  }
}

nsIDocument*
nsTextFrame::GetDocument(nsIPresContext* aPresContext)
{
  nsIDocument* result = nsnull;
  if (mContent) {
    mContent->GetDocument(result);
  }
  if (!result && aPresContext) {
    nsIPresShell* shell;
    aPresContext->GetShell(&shell);
    if (shell) {
      shell->GetDocument(&result);
      NS_RELEASE(shell);
    }
  }
  return result;
}

NS_IMETHODIMP
nsTextFrame::GetCursor(nsIPresContext& aPresContext,
                       nsPoint& aPoint,
                       PRInt32& aCursor)
{
  const nsStyleColor* styleColor;
  GetStyleData(eStyleStruct_Color, (const nsStyleStruct*&)styleColor);
  aCursor = styleColor->mCursor;

  if (NS_STYLE_CURSOR_AUTO == aCursor && nsnull != mParent) {
    mParent->GetCursor(aPresContext, aPoint, aCursor);
    if (NS_STYLE_CURSOR_AUTO == aCursor) {
      aCursor = NS_STYLE_CURSOR_TEXT;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent)
{
  // Generate a reflow command with this frame as the target frame
  nsIReflowCommand* cmd;
  nsresult          rv;
                                                
  rv = NS_NewHTMLReflowCommand(&cmd, this, nsIReflowCommand::ContentChanged);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPresShell> shell;
    rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_SUCCEEDED(rv) && shell) {
      shell->AppendReflowCommand(cmd);
      NS_RELEASE(cmd);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsTextFrame::Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer)
{
  if (NS_FRAME_PAINT_LAYER_FOREGROUND != aWhichLayer) {
    return NS_OK;
  }
  if ((0 != (mState & TEXT_BLINK_ON)) && gBlinkTextOff) {
    return NS_OK;
  }
  nsIStyleContext* sc = mStyleContext;
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    sc->GetStyleData(eStyleStruct_Display);
  if (disp->mVisible) {
    TextStyle ts(&aPresContext, aRenderingContext, mStyleContext);
    if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing) ||
        ((NS_STYLE_TEXT_ALIGN_JUSTIFY == ts.mText->mTextAlign) &&
         (mRect.width > mComputedWidth))) {
      PaintTextSlowly(&aPresContext, aRenderingContext, sc, ts, 0, 0);
    }
    else {
      // Choose rendering pathway based on rendering context
      // performance hint.
      PRUint32 hints = 0;
      aRenderingContext.GetHints(hints);
      if ((TEXT_HAS_MULTIBYTE & mState) ||
          (0 == (hints & NS_RENDERING_HINT_FAST_8BIT_TEXT))) {
        // Use PRUnichar rendering routine
        PaintUnicodeText(&aPresContext, aRenderingContext, sc, ts, 0, 0);
      }
      else {
        // Use char rendering routine
        PaintAsciiText(&aPresContext, aRenderingContext, sc, ts, 0, 0);
      }
    }
  }
  return NS_OK;
}


/**
 * Prepare the text in the content for rendering. If aIndexes is not nsnull
 * then fill in aIndexes's with the mapping from the original input to
 * the prepared output.
 */
PRIntn
nsTextFrame::PrepareUnicodeText(nsTextTransformer& aTX,
                                PRInt32* aIndexes,
                                PRUnichar* aBuffer,
                                PRInt32* aTextLen)
{
  PRIntn numSpaces = 0;
  PRUnichar* dst = aBuffer;

  // Setup transform to operate starting in the content at our content
  // offset
  aTX.Init(this, mContent, mContentOffset);

  PRInt32 strInx = mContentOffset;

  // Skip over the leading whitespace
  PRInt32 n = mContentLength;
  if (0 != (mState & TEXT_SKIP_LEADING_WS)) {
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;
    aTX.GetNextWord(PR_FALSE, wordLen, contentLen, isWhitespace);
    NS_ASSERTION(isWhitespace, "mState and content are out of sync");
    if (isWhitespace) {
      if (nsnull != aIndexes) {
        // Point mapping indicies at the same content index since
        // all of the compressed whitespace maps down to the same
        // renderable character.
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *aIndexes++ = strInx;
        }
      }
      n -= contentLen;
      NS_ASSERTION(n >= 0, "whoops");
    }
  }

  // Rescan the content and transform it. Stop when we have consumed
  // mContentLength characters.
  PRBool inWord = (TEXT_IN_WORD & mState) ? PR_TRUE : PR_FALSE;
  PRInt32 column = mColumn;
  PRInt32 textLength = 0;
  while (0 != n) {
    PRUnichar* bp;
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;

    // Get the next word
    bp = aTX.GetNextWord(inWord, wordLen, contentLen, isWhitespace);
    if (nsnull == bp) {
      break;
    }
    if (contentLen > n) {
      contentLen = n;
    }
    if (wordLen > n) {
      wordLen = n;
    }
    inWord = PR_FALSE;
    if (isWhitespace) {
      numSpaces++;
      if ('\t' == bp[0]) {
        PRInt32 spaces = 8 - (7 & column);
        PRUnichar* tp = bp;
        wordLen = spaces;
        while (--spaces >= 0) {
          *tp++ = ' ';
        }
        // XXX This is a one to many mapping that I think isn't handled well
        if (nsnull != aIndexes) {
          *aIndexes++ = strInx;
        	strInx += wordLen;
        }
      }
      else if (0 == wordLen) {
        if (nsnull != aIndexes)
          *aIndexes++ = strInx;
        break;
      }
      else if (nsnull != aIndexes) {
        // Point mapping indicies at the same content index since
        // all of the compressed whitespace maps down to the same
        // renderable character.
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *aIndexes++ = strInx;
        }
        strInx++;
      }
    }
    else {
      if (nsnull != aIndexes) {
        // Point mapping indicies at each content index in the word
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *aIndexes++ = strInx++;
        }
      }
    }
    column += wordLen;
    textLength += wordLen;
    n -= contentLen;
    nsCRT::memcpy(dst, bp, sizeof(PRUnichar) * wordLen);
    dst += wordLen;
  }

  // Remove trailing whitespace if it was trimmed after reflow
  if (TEXT_TRIMMED_WS & mState) {
    if (--dst >= aBuffer) {
      PRUnichar ch = *dst;
      if (XP_IS_SPACE(ch)) {
        textLength--;
      }
    }
    numSpaces--;
  }

  *aTextLen = textLength;
  return numSpaces;
}


//#define SHOW_SELECTION_CURSOR			// should be turned off when the caret code is activated

#ifdef SHOW_SELECTION_CURSOR

// XXX This clearly needs to be done by the container, *somehow*
#define CURSOR_COLOR NS_RGB(0,0,255)
static void
RenderSelectionCursor(nsIRenderingContext& aRenderingContext,
                      nscoord dx, nscoord dy, nscoord aHeight,
                      nscolor aCursorColor)
{
  nsPoint pnts[4];
  nscoord ox = aHeight / 4;
  nscoord oy = ox;
  nscoord x0 = dx;
  nscoord y0 = dy + aHeight;
  pnts[0].x = x0 - ox;
  pnts[0].y = y0;
  pnts[1].x = x0;
  pnts[1].y = y0 - oy;
  pnts[2].x = x0 + ox;
  pnts[2].y = y0;
  pnts[3].x = x0 - ox;
  pnts[3].y = y0;

  // Draw little blue triangle
  aRenderingContext.SetColor(aCursorColor);
  aRenderingContext.FillPolygon(pnts, 4);
}

#endif

// XXX letter-spacing
// XXX word-spacing

void 
nsTextFrame::PaintTextDecorations(nsIRenderingContext& aRenderingContext,
                                  nsIStyleContext* aStyleContext,
                                  TextStyle& aTextStyle,
                                  nscoord aX, nscoord aY, nscoord aWidth,
                                  PRUnichar *aText, /*=nsnull*/
                                  SelectionDetails *aDetails,/*= nsnull*/
                                  PRUint32 aIndex,  /*= 0*/
                                  PRUint32 aLength, /*= 0*/
                                  const nscoord* aSpacing /* = nsnull*/ )

{
  nscolor overColor;
  nscolor underColor;
  nscolor strikeColor;
  nsIStyleContext*  context = aStyleContext;
  PRUint8 decorations = aTextStyle.mFont->mFont.decorations;
  PRUint8 decorMask = decorations;

  NS_ADDREF(context);
  do {  // find decoration colors
    const nsStyleText* styleText = 
      (const nsStyleText*)context->GetStyleData(eStyleStruct_Text);
    if (decorMask & styleText->mTextDecoration) {  // a decoration defined here
      const nsStyleColor* styleColor =
        (const nsStyleColor*)context->GetStyleData(eStyleStruct_Color);
      if (NS_STYLE_TEXT_DECORATION_UNDERLINE & decorMask & styleText->mTextDecoration) {
        underColor = styleColor->mColor;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_OVERLINE & decorMask & styleText->mTextDecoration) {
        overColor = styleColor->mColor;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & decorMask & styleText->mTextDecoration) {
        strikeColor = styleColor->mColor;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
      }
    }
    if (0 != decorMask) {
      nsIStyleContext*  lastContext = context;
      context = context->GetParent();
      NS_RELEASE(lastContext);
    }
  } while ((nsnull != context) && (0 != decorMask));
  NS_IF_RELEASE(context);

  nscoord offset;
  nscoord size;
  nscoord baseline;
  aTextStyle.mNormalFont->GetMaxAscent(baseline);
  if (decorations & (NS_FONT_DECORATION_OVERLINE | NS_FONT_DECORATION_UNDERLINE)) {
    aTextStyle.mNormalFont->GetUnderline(offset, size);
    if (decorations & NS_FONT_DECORATION_OVERLINE) {
      aRenderingContext.SetColor(overColor);
      aRenderingContext.FillRect(aX, aY, aWidth, size);
    }
    if (decorations & NS_FONT_DECORATION_UNDERLINE) {
      aRenderingContext.SetColor(underColor);
      aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
    }
  }
  if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
    aTextStyle.mNormalFont->GetStrikeout(offset, size);
    aRenderingContext.SetColor(strikeColor);
    aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
  }
  if (aDetails){
    nsRect rect;
    GetRect(rect);
    PRInt32 startOffset = 0;
    PRInt32 textWidth = 0;
    while(aDetails){
      PRInt32 start = PR_MAX(0,(aDetails->mStart - (PRInt32)aIndex));
      PRInt32 end = PR_MIN((PRInt32)aLength,(aDetails->mEnd - (PRInt32)aIndex));
      PRInt32 i;
      if (start < end && (aLength - start) > 0)
      {
        //aDetails allready processed to have offsets from frame start not content offsets
        if (start < end){
          if (aLength == 1)
            textWidth = aWidth;
          else {
            if (aDetails->mStart > 0){
              if (aSpacing)
              {
                for (i = 0; i < start;i ++){
                  startOffset += *aSpacing ++;
                }
              }
              else
                aRenderingContext.GetWidth(aText, start, startOffset);
            }
            if (aSpacing){
              for (i = start; i < end;i ++){
                textWidth += *aSpacing ++;
              }
            }
            else
              aRenderingContext.GetWidth(aText + start,
                                           PRUint32(end - start), textWidth);
  
          }
          switch (aDetails->mType)
          {
          case SELECTION_NORMAL:{
//
// XOR InvertRect is currently implemented only in the unix and windows
// rendering contexts.  When other platforms implement InvertRect(), they
// can be added here.  Eventually this #ifdef should die.
//
// For platforms that dont implement InvertRect(), the selection will be 
// a non-filled rectangle.
#if defined(XP_PC) || defined(XP_UNIX) || defined(XP_MAC)
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(0,0,0));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif
                                }break;
          case SELECTION_SPELLCHECK:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(255,0,0));
              aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
                                }break;
          case SELECTION_IME_SOLID:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(0,0,255));
              aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
                                }break;
          case SELECTION_IME_DASHED:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(128,0,255));
              aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
                                }break;
          default:
            NS_ASSERTION(0,"what type of selection do i not know about?");
            break;
          }

        }
      }
      aDetails = aDetails->mNext;
    }
  }
}

void
nsTextFrame::PaintUnicodeText(nsIPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsIStyleContext* aStyleContext,
                              TextStyle& aTextStyle,
                              nscoord dx, nscoord dy)
{
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  PRBool displaySelection;
  displaySelection = doc->GetDisplaySelection();

  // Make enough space to transform
  PRUnichar wordBufMem[WORD_BUF_SIZE];
  PRUnichar paintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRInt32* ip = indicies;
  PRUnichar* paintBuf = paintBufMem;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength+1];
    paintBuf = new PRUnichar[mContentLength];
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  // nsCOMPtr<nsIWordBreaker> wb;
  // doc->GetWordBreaker(getter_AddRefs(wb));
  nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE,lb,nsnull);
  PrepareUnicodeText(tx, (displaySelection ? ip : nsnull),
                     paintBuf, &textLength);
  PRUnichar* text = paintBuf;
  nsFrameState  frameState;
  PRBool        isSelected;
  GetFrameState(&frameState);
  isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;

  if (0 != textLength) {
    if (!displaySelection || !isSelected ) { 
      // When there is no selection showing, use the fastest and
      // simplest rendering approach
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      PaintTextDecorations(aRenderingContext, aStyleContext, aTextStyle,
                           dx, dy, width);
    }
    else {
      ip[mContentLength] = ip[mContentLength-1];
      if ((ip[mContentLength]-mContentOffset) < textLength) {
        //must set up last one for selection beyond edge if in boundary
        ip[mContentLength]++;
      }
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;

      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          rv = GetContent(getter_AddRefs(content));
          if (NS_SUCCEEDED(rv) && content){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details);// last param notused
          }
        }
      }

        
      //where are the selection points "really"
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
        sdptr = sdptr->mNext;
      }
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      PaintTextDecorations(aRenderingContext, aStyleContext,
                           aTextStyle, dx, dy, width, text, details,0,(PRUint32)textLength);
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
  }

  // Cleanup
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
  if (ip != indicies) {
    delete [] ip;
  }
}


//measure Spaced Textvoid
nsresult
nsTextFrame::GetPositionSlowly(nsIPresContext& aPresContext,
                               nsIRenderingContext* aRendContext,
                               nscoord         aXCoord,
                               nsIContent** aNewContent,
                               PRInt32& aOffset)

{
  if (!aRendContext || !aNewContent) {
    return NS_ERROR_NULL_POINTER;
  }

  TextStyle ts(&aPresContext, *aRendContext, mStyleContext);
  if (!ts.mSmallCaps && !ts.mWordSpacing && !ts.mLetterSpacing) {
    return NS_ERROR_INVALID_ARG;
  }
  nsIView * view;
  nsPoint origin;
  GetView(&view);
  GetOffsetFromView(origin, &view);

  if (aXCoord - origin.x <0)
  {
      *aNewContent = mContent;
      aOffset =0;
  }
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(&aPresContext)));

  // Make enough space to transform
  PRUnichar paintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRUnichar wordBufMem[WORD_BUF_SIZE];
  PRInt32* ip = indicies;
  PRUnichar* paintBuf = paintBufMem;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength+1];
    paintBuf = new PRUnichar[mContentLength];
  }
  PRInt32 textLength;
  PRInt32 actualLength;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  //nsCOMPtr<nsIWordBreaker> wb;
  //doc->GetWordBreaker(getter_AddRefs(wb));
  nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE, lb,nsnull);
  PrepareUnicodeText(tx, ip, paintBuf, &textLength);
  if (textLength <= 0)
    return NS_ERROR_FAILURE;
  actualLength = textLength;

  nscoord charWidth,widthsofar = 0;
  PRInt32 indx = 0;
  PRBool found = PR_FALSE;
  PRUnichar* startBuf = paintBuf;
  nsIFontMetrics* lastFont = ts.mLastFont;
  for (; --textLength >= 0; paintBuf++,indx++) {
    nsIFontMetrics* nextFont;
    nscoord glyphWidth;
    PRUnichar ch = *paintBuf;
    if (ts.mSmallCaps && nsCRT::IsLower(ch)) {
      nextFont = ts.mSmallFont;
      ch = nsCRT::ToUpper(ch);
      if (lastFont != ts.mSmallFont) {
        aRendContext->SetFont(ts.mSmallFont);
        aRendContext->GetWidth(ch, charWidth);
        aRendContext->SetFont(ts.mNormalFont);
      }
      else {
        aRendContext->GetWidth(ch, charWidth);
      }
      glyphWidth = charWidth + ts.mLetterSpacing;
    }
    else if (ch == ' ') {
      nextFont = lastFont;
      glyphWidth = ts.mSpaceWidth + ts.mWordSpacing;
      nscoord extra = ts.mExtraSpacePerSpace;
      if (--ts.mNumSpaces == 0) {
        extra += ts.mRemainingExtraSpace;
      }
      glyphWidth += extra;
    }
    else {
      nextFont = lastFont;
      aRendContext->GetWidth(ch, charWidth);
      glyphWidth = charWidth + ts.mLetterSpacing;
    }
    if ((aXCoord - origin.x) >= widthsofar && (aXCoord - origin.x) <= (widthsofar + glyphWidth)){
      if ( ((aXCoord - origin.x) - widthsofar) <= (glyphWidth /2)){
        aOffset = indx;
        found = PR_TRUE;
        break;
      }
      else{
        aOffset = indx+1;
        found = PR_TRUE;
        break;
      }

    }
    if (nextFont != lastFont)
      lastFont = nextFont;

    widthsofar += glyphWidth;
  }
  paintBuf = startBuf;
  if (!found){
    aOffset = textLength;
    if (aOffset <0)
      aOffset = actualLength;//max length before textlength was reduced
  }
  aOffset += mContentOffset;//offset;//((nsTextFrame *)aNewFrame)->mContentOffset;
  PRInt32 i;
  for (i = 0;i <= mContentLength; i ++){
    if (ip[i] == aOffset){ //reverse mapping
      aOffset = i + mContentOffset;
      break;
    }
  }

  // Cleanup
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
  if (ip != indicies) {
    delete [] ip;
  }
  *aNewContent = mContent;
  if (*aNewContent)
    (*aNewContent)->AddRef();
  return NS_OK;
}

void
nsTextFrame::RenderString(nsIRenderingContext& aRenderingContext,
                          nsIStyleContext* aStyleContext,
                          TextStyle& aTextStyle,
                          PRUnichar* aBuffer, PRInt32 aLength,
                          nscoord aX, nscoord aY,
                          nscoord aWidth, 
                          SelectionDetails *aDetails /*=nsnull*/)
{
  PRUnichar buf[TEXT_BUF_SIZE];
  PRUnichar* bp0 = buf;
  if (aLength > TEXT_BUF_SIZE) {
    bp0 = new PRUnichar[aLength];
  }
  PRUnichar* bp = bp0;

  PRBool spacing = (0 != aTextStyle.mLetterSpacing) ||
    (0 != aTextStyle.mWordSpacing) || (mRect.width > mComputedWidth);
  nscoord spacingMem[TEXT_BUF_SIZE];
  PRIntn* sp0 = spacingMem; 
  if (spacing && (aLength > TEXT_BUF_SIZE)) {
    sp0 = new nscoord[aLength];
  }
  PRIntn* sp = sp0;

  nscoord smallY = aY;
  if (aTextStyle.mSmallCaps) {
    nscoord normalAscent, smallAscent;
    aTextStyle.mNormalFont->GetMaxAscent(normalAscent);
    aTextStyle.mSmallFont->GetMaxAscent(smallAscent);
    if (normalAscent > smallAscent) {
      smallY = aY + normalAscent - smallAscent;
    }
  }

  nsIFontMetrics* lastFont = aTextStyle.mLastFont;
  nscoord lastY = aY;
  if (lastFont == aTextStyle.mSmallFont) {
    lastY = smallY;
  }
  PRInt32 pendingCount;
  PRUnichar* runStart = bp;
  nscoord charWidth, width = 0;
  PRInt32 countSoFar = 0;
  for (; --aLength >= 0; aBuffer++) {
    nsIFontMetrics* nextFont;
    nscoord nextY, glyphWidth;
    PRUnichar ch = *aBuffer;
    if (aTextStyle.mSmallCaps && nsCRT::IsLower(ch)) {
      nextFont = aTextStyle.mSmallFont;
      nextY = smallY;
      ch = nsCRT::ToUpper(ch);
      if (lastFont != aTextStyle.mSmallFont) {
        aRenderingContext.SetFont(aTextStyle.mSmallFont);
        aRenderingContext.GetWidth(ch, charWidth);
        aRenderingContext.SetFont(aTextStyle.mNormalFont);
      }
      else {
        aRenderingContext.GetWidth(ch, charWidth);
      }
      glyphWidth = charWidth + aTextStyle.mLetterSpacing;
    }
    else if (ch == ' ') {
      nextFont = aTextStyle.mNormalFont;
      nextY = aY;
      glyphWidth = aTextStyle.mSpaceWidth + aTextStyle.mWordSpacing;
      nscoord extra = aTextStyle.mExtraSpacePerSpace;
      if (--aTextStyle.mNumSpaces == 0) {
        extra += aTextStyle.mRemainingExtraSpace;
      }
      glyphWidth += extra;
    }
    else {
      if (lastFont != aTextStyle.mNormalFont) {
        aRenderingContext.SetFont(aTextStyle.mNormalFont);
        aRenderingContext.GetWidth(ch, charWidth);
        aRenderingContext.SetFont(aTextStyle.mSmallFont);
      }
      else {
        aRenderingContext.GetWidth(ch, charWidth);
      }
      nextFont = aTextStyle.mNormalFont;
      nextY = aY;
      glyphWidth = charWidth + aTextStyle.mLetterSpacing;
    }
    if (nextFont != lastFont) {
      pendingCount = bp - runStart;
      if (0 != pendingCount) {
        // Measure previous run of characters using the previous font
        aRenderingContext.SetColor(aTextStyle.mColor->mColor);
        aRenderingContext.DrawString(runStart, pendingCount,
                                     aX, lastY, -1,
                                     spacing ? sp0 : nsnull);

        // Note: use aY not small-y so that decorations are drawn with
        // respect to the normal-font not the current font.
        PaintTextDecorations(aRenderingContext, aStyleContext, aTextStyle,
                             aX, aY, width, runStart, aDetails,countSoFar,pendingCount, spacing ? sp0 : nsnull);
        countSoFar += pendingCount;
        aWidth -= width;
        aX += width;
        runStart = bp = bp0;
        sp = sp0;
        width = 0;
      }
      aRenderingContext.SetFont(nextFont);
      lastFont = nextFont;
      lastY = nextY;
    }
    *bp++ = ch;
    *sp++ = glyphWidth;
    width += glyphWidth;
  }
  pendingCount = bp - runStart;
  if (0 != pendingCount) {
    // Measure previous run of characters using the previous font
    aRenderingContext.SetColor(aTextStyle.mColor->mColor);
    aRenderingContext.DrawString(runStart, pendingCount, aX, lastY, -1,
                                 spacing ? sp0 : nsnull);

    // Note: use aY not small-y so that decorations are drawn with
    // respect to the normal-font not the current font.
    PaintTextDecorations(aRenderingContext, aStyleContext, aTextStyle,
                         aX, aY, aWidth, runStart, aDetails,countSoFar,pendingCount,
                         spacing ? sp0 : nsnull);
  }
  aTextStyle.mLastFont = lastFont;

  if (bp0 != buf) {
    delete [] bp0;
  }
  if (sp0 != spacingMem) {
    delete [] sp0;
  }
}

inline void
nsTextFrame::MeasureSmallCapsText(const nsHTMLReflowState& aReflowState,
                                  TextStyle& aTextStyle,
                                  PRUnichar* aWord,
                                  PRInt32 aWordLength,
                                  nscoord* aWidthResult)
{
  nsIRenderingContext& rc = *aReflowState.rendContext;
  GetWidth(rc, aTextStyle, aWord, aWordLength, aWidthResult);
  if (aTextStyle.mLastFont != aTextStyle.mNormalFont) {
    rc.SetFont(aTextStyle.mNormalFont);
    aTextStyle.mLastFont = aTextStyle.mNormalFont;
  }
}

// XXX factor in logic from RenderString into here; gaps, justification, etc.
void
nsTextFrame::GetWidth(nsIRenderingContext& aRenderingContext,
                      TextStyle& aTextStyle,
                      PRUnichar* aBuffer, PRInt32 aLength,
                      nscoord* aWidthResult)
{
  PRUnichar buf[TEXT_BUF_SIZE];
  PRUnichar* bp0 = buf;
  if (aLength > TEXT_BUF_SIZE) {
    bp0 = new PRUnichar[aLength];
  }
  PRUnichar* bp = bp0;

  nsIFontMetrics* lastFont = aTextStyle.mLastFont;
  nscoord sum = 0;
  nscoord charWidth;
  for (; --aLength >= 0; aBuffer++) {
    nscoord glyphWidth;
    PRUnichar ch = *aBuffer;
    if (aTextStyle.mSmallCaps && nsCRT::IsLower(ch)) {
      ch = nsCRT::ToUpper(ch);
      if (lastFont != aTextStyle.mSmallFont) {
        lastFont = aTextStyle.mSmallFont;
        aRenderingContext.SetFont(lastFont);
      }
      aRenderingContext.GetWidth(ch, charWidth);
      glyphWidth = charWidth + aTextStyle.mLetterSpacing;
    }
    else if (ch == ' ') {
      glyphWidth = aTextStyle.mSpaceWidth + aTextStyle.mWordSpacing;
      nscoord extra = aTextStyle.mExtraSpacePerSpace;
      if (--aTextStyle.mNumSpaces == 0) {
        extra += aTextStyle.mRemainingExtraSpace;
      }
      glyphWidth += extra;
    }
    else {
      if (lastFont != aTextStyle.mNormalFont) {
        lastFont = aTextStyle.mNormalFont;
        aRenderingContext.SetFont(lastFont);
      }
      aRenderingContext.GetWidth(ch, charWidth);
      glyphWidth = charWidth + aTextStyle.mLetterSpacing;
    }
    sum += glyphWidth;
    *bp++ = ch;
  }
  if (bp0 != buf) {
    delete [] bp0;
  }
  aTextStyle.mLastFont = lastFont;
  *aWidthResult = sum;
}

void
nsTextFrame::PaintTextSlowly(nsIPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             nsIStyleContext* aStyleContext,
                             TextStyle& aTextStyle,
                             nscoord dx, nscoord dy)
{
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  PRBool displaySelection;
  displaySelection = doc->GetDisplaySelection();

  // Make enough space to transform
  PRUnichar wordBufMem[WORD_BUF_SIZE];
  PRUnichar paintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRInt32* ip = indicies;
  PRUnichar* paintBuf = paintBufMem;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength+1];
    paintBuf = new PRUnichar[mContentLength];
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  //nsCOMPtr<nsIWordBreaker> wb;
  //doc->GetWordBreaker(getter_AddRefs(wb));
  nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE, lb,nsnull);
  aTextStyle.mNumSpaces = PrepareUnicodeText(tx,
                                             displaySelection ? ip : nsnull,
                                             paintBuf, &textLength);
  if (mRect.width > mComputedWidth) {
    if (0 != aTextStyle.mNumSpaces) {
      nscoord extra = mRect.width - mComputedWidth;
#if XXX
      nscoord adjustPerSpace =
        aTextStyle.mExtraSpacePerSpace = extra / aTextStyle.mNumSpaces;
#endif
      aTextStyle.mRemainingExtraSpace = extra -
        (aTextStyle.mExtraSpacePerSpace * aTextStyle.mNumSpaces);
    }
    else {
      // We have no whitespace but were given some extra space. There
      // are two plausible places to put the extra space: to the left
      // and to the right. If this is anywhere but the last place on
      // the line then the correct answer is to the right.
    }
  }

  PRUnichar* text = paintBuf;
  nsFrameState  frameState;
  PRBool        isSelected;
  GetFrameState(&frameState);
  isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
  if (0 != textLength) {
    if (!displaySelection || !isSelected) { 
      // When there is no selection showing, use the fastest and
      // simplest rendering approach
      RenderString(aRenderingContext, aStyleContext, aTextStyle,
                   text, textLength, dx, dy, width);
    }
    else {
      SelectionDetails *details = nsnull;
      ip[mContentLength] = ip[mContentLength-1];
      if ((ip[mContentLength]-mContentOffset) < textLength) {
        //must set up last one for selection beyond edge if in boundary
        ip[mContentLength]++;
      }
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;
      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          rv = GetContent(getter_AddRefs(content));
          if (NS_SUCCEEDED(rv)){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details);// last param notused
          }
        }
      }

      //where are the selection points "really"
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
        sdptr = sdptr->mNext;
      }
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      RenderString(aRenderingContext,aStyleContext, aTextStyle, text, 
                    PRUint32(textLength), dx, dy, width, details);
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
  }
  // Cleanup
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
  if (ip != indicies) {
    delete [] ip;
  }
}

void
nsTextFrame::PaintAsciiText(nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIStyleContext* aStyleContext,
                            TextStyle& aTextStyle,
                            nscoord dx, nscoord dy)
{
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  PRBool displaySelection;
  displaySelection = doc->GetDisplaySelection();

  // Make enough space to transform
  PRUnichar wordBufMem[WORD_BUF_SIZE];
  char paintBufMem[TEXT_BUF_SIZE];
  PRUnichar rawPaintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRInt32* ip = indicies;
  char* paintBuf = paintBufMem;
  PRUnichar* rawPaintBuf = rawPaintBufMem;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength];
    paintBuf = new char[mContentLength];
    rawPaintBuf = new PRUnichar[mContentLength];
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  //nsCOMPtr<nsIWordBreaker> wb;
  //doc->GetWordBreaker(getter_AddRefs(wb));
  nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE,lb,nsnull);
  PrepareUnicodeText(tx, (displaySelection ? ip : nsnull),
                     rawPaintBuf, &textLength);
  // Translate unicode data into ascii for rendering
  char* dst = paintBuf;
  char* end = dst + textLength;
  PRUnichar* src = rawPaintBuf;
  while (dst < end) {
    *dst++ = (char) ((unsigned char) *src++);
  }

  char* text = paintBuf;
  nsFrameState  frameState;
  PRBool        isSelected;
  GetFrameState(&frameState);
  isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;

  if (0 != textLength) {
    if (!displaySelection || !isSelected) { 
      //if selection is > content length then selection has "slid off"
      // When there is no selection showing, use the fastest and
      // simplest rendering approach
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      PaintTextDecorations(aRenderingContext, aStyleContext, aTextStyle,
                           dx, dy, width);
    }
    else {
      SelectionDetails *details;
      ip[mContentLength] = ip[mContentLength-1];
      if ((ip[mContentLength]-mContentOffset) < textLength) {
        //must set up last one for selection beyond edge if in boundary
        ip[mContentLength]++;
      }
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;
      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          rv = GetContent(getter_AddRefs(content));
          if (NS_SUCCEEDED(rv)){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details);// last param notused
          }
        }
      }

        
      //where are the selection points "really"
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
        sdptr = sdptr->mNext;
      }
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      PaintTextDecorations(aRenderingContext, aStyleContext,
                           aTextStyle, dx, dy, width, rawPaintBuf, details,0,textLength);
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
  }

  // Cleanup
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
  if (ip != indicies) {
    delete [] ip;
  }
}

NS_IMETHODIMP
nsTextFrame::FindTextRuns(nsLineLayout& aLineLayout)
{
  if (nsnull == mPrevInFlow) {
    aLineLayout.AddText(this);
  }
  return NS_OK;
}

//---------------------------------------------------
// Uses a binary search for find where the cursor falls in the line of text
// It also keeps track of the part of the string that has already been measured
// so it doesn't have to keep measuring the same text over and over
//
// Param "aBaseWidth" contains the width in twips of the portion 
// of the text that has already been measured, and aBaseInx contains
// the index of the text that has already been measured.
//
// aTextWidth returns the (in twips) the length of the text that falls before the cursor
// aIndex contains the index of the text where the cursor falls
static PRBool
BinarySearchForPosition(nsIRenderingContext* acx, 
                        PRUnichar* aText,
                        PRInt32    aBaseWidth,
                        PRInt32    aBaseInx,
                        PRInt32    aStartInx, 
                        PRInt32    aEndInx, 
                        PRInt32    aCursorPos, 
                        PRInt32&   aIndex,
                        PRInt32&   aTextWidth)
{
  PRInt32 range = aEndInx - aStartInx;
  if (range == 1) {
    aIndex   = aStartInx + aBaseInx;
    acx->GetWidth(aText, aIndex, aTextWidth);
    return PR_TRUE;
  }
  PRInt32 inx = aStartInx + (range / 2);

  PRInt32 textWidth = 0;
  acx->GetWidth(aText, inx, textWidth);

  PRInt32 fullWidth = aBaseWidth + textWidth;
  if (fullWidth == aCursorPos) {
    aTextWidth = textWidth;
    aIndex = inx;
    return PR_TRUE;
  } else if (aCursorPos < fullWidth) {
    aTextWidth = aBaseWidth;
    if (BinarySearchForPosition(acx, aText, aBaseWidth, aBaseInx, aStartInx, inx, aCursorPos, aIndex, aTextWidth)) {
      return PR_TRUE;
    }
  } else {
    aTextWidth = fullWidth;
    if (BinarySearchForPosition(acx, aText, aBaseWidth, aBaseInx, inx, aEndInx, aCursorPos, aIndex, aTextWidth)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

//---------------------------------------------------------------------------
// Uses a binary search to find the position of the cursor in the text.
// The "indices array is used to map from the compressed text back to the 
// un-compressed text, selection is based on the un-compressed text, the visual 
// display of selection is based on the compressed text.
//---------------------------------------------------------------------------
NS_IMETHODIMP
nsTextFrame::GetPosition(nsIPresContext& aCX,
                         nscoord         aXCoord,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd)

{
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aCX.GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIRenderingContext> acx;      
    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    if (NS_SUCCEEDED(rv)) {
      TextStyle ts(&aCX, *acx, mStyleContext);
      if (ts.mSmallCaps || ts.mWordSpacing || ts.mLetterSpacing) {

        nsresult result = GetPositionSlowly(aCX, acx, aXCoord, aNewContent,
                                 aContentOffset);
        aContentOffsetEnd = aContentOffset;
        return result;
      }

      PRUnichar wordBufMem[WORD_BUF_SIZE];
      PRUnichar paintBufMem[TEXT_BUF_SIZE];
      PRInt32 indicies[TEXT_BUF_SIZE];
      PRInt32* ip = indicies;
      PRUnichar* paintBuf = paintBufMem;
      if (mContentLength >= TEXT_BUF_SIZE) {
        ip = new PRInt32[mContentLength+1];
        paintBuf = new PRUnichar[mContentLength];
      }
      PRInt32 textLength;

      // Find the font metrics for this text
      nsIStyleContext* styleContext;
      GetStyleContext(&styleContext);
      const nsStyleFont *font = (const nsStyleFont*)
        styleContext->GetStyleData(eStyleStruct_Font);
      NS_RELEASE(styleContext);
      nsCOMPtr<nsIFontMetrics> fm;
      aCX.GetMetricsFor(font->mFont, getter_AddRefs(fm));
      acx->SetFont(fm);

      // Get the document
      nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(&aCX)));

      // Get the renderable form of the text
      nsCOMPtr<nsILineBreaker> lb;
      doc->GetLineBreaker(getter_AddRefs(lb));
      //nsCOMPtr<nsIWordBreaker> wb;
      //doc->GetWordBreaker(getter_AddRefs(wb));
      nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE,lb,nsnull);
      PrepareUnicodeText(tx, ip, paintBuf, &textLength);
      if (textLength <=0) //invalid frame to get position on
        return NS_ERROR_FAILURE;
      ip[mContentLength] = ip[mContentLength-1];
      if ((ip[mContentLength]-mContentOffset) < textLength) {
        //must set up last one for selection beyond edge if in boundary
        ip[mContentLength]++;
      }

      PRInt32 indx;
      PRInt32 textWidth = 0;
      PRUnichar* text = paintBuf;
      nsPoint origin;
      nsIView * view;
      GetOffsetFromView(origin, &view);
      PRBool found = BinarySearchForPosition(acx, text, origin.x, 0, 0,
                                             PRInt32(textLength),
                                             PRInt32(aXCoord) , //go to local coordinates
                                             indx, textWidth);
      if (found) {
        PRInt32 charWidth;
        acx->GetWidth(text[indx], charWidth);
        charWidth /= 2;

        if (PR_ABS(PRInt32(aXCoord) - origin.x) > textWidth+charWidth) {
          indx++;
        }
      }

      if (ip != indicies) {
        delete [] ip;
      }
      if (paintBuf != paintBufMem) {
        delete [] paintBuf;
      }

      aContentOffset = indx + mContentOffset;
      //reusing wordBufMem
      PRInt32 i;
      for (i = 0;i <= mContentLength; i ++){
        if (ip[i] == aContentOffset){ //reverse mapping
            aContentOffset = i + mContentOffset;
            break;
        }
      }
      aContentOffsetEnd = aContentOffset;
      NS_ASSERTION(i<= mContentLength, "offset we got from binary search is messed up");
      
      *aNewContent = mContent;
      if (*aNewContent) {
        (*aNewContent)->AddRef();
      }
    }
  }
  return NS_OK;
}


// [HACK] Foward Declarations
void ForceDrawFrame(nsFrame * aFrame);

//null range means the whole thing
NS_IMETHODIMP
nsTextFrame::SetSelected(nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread)
{
  nsresult result;
  if (aSelected && ParentDisablesSelection())
    return NS_OK;
  if (aSpread == eSpreadDown)
  {
    nsIFrame *frame = GetPrevInFlow();
    while(frame){
      frame->SetSelected(aRange,aSelected,eSpreadNone);
      result = frame->GetPrevInFlow(&frame);
      if (NS_FAILED(result))
        break;
    }
    frame = GetNextInFlow();
    while (frame){
      frame->SetSelected(aRange,aSelected,eSpreadNone);
      result = frame->GetNextInFlow(&frame);
      if (NS_FAILED(result))
        break;
    }
  }
  nsFrameState  frameState;
  GetFrameState(&frameState);
  PRBool isSelected = ((frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);
  if (!aSelected && !isSelected) //already set thanks
  {
    return NS_OK;
  }

  PRBool found = PR_FALSE;
  if (aRange) {
    //lets see if the range contains us, if so we must redraw!
    nsCOMPtr<nsIDOMNode> endNode;
    PRInt32 endOffset;
    nsCOMPtr<nsIDOMNode> startNode;
    PRInt32 startOffset;
    aRange->GetEndParent(getter_AddRefs(endNode));
    aRange->GetEndOffset(&endOffset);
    aRange->GetStartParent(getter_AddRefs(startNode));
    aRange->GetStartOffset(&startOffset);
    nsCOMPtr<nsIContent> content;
    result = GetContent(getter_AddRefs(content));
    nsCOMPtr<nsIDOMNode> thisNode;
    thisNode = do_QueryInterface(content);

    if (thisNode == startNode){
      if ((mContentOffset + mContentLength) >= startOffset){
        found = PR_TRUE;
        if (thisNode == endNode){ //special case
          if (endOffset == startOffset) //no need to redraw since drawing takes place with cursor
            found = PR_FALSE;

          if (mContentOffset > endOffset)
            found = PR_FALSE;
        }
      }
    }
    else if (thisNode == endNode){
      if (mContentOffset < endOffset)
        found = PR_TRUE;
      else
        found = PR_FALSE;
    }
    else
      found = PR_TRUE;
  }
  else {
    if ( aSelected != (PRBool)(frameState | NS_FRAME_SELECTED_CONTENT) ){
      found = PR_TRUE;
    }
  }

  if ( aSelected )
    frameState |=  NS_FRAME_SELECTED_CONTENT;
  else
    frameState &= ~NS_FRAME_SELECTED_CONTENT;
  SetFrameState(frameState);
  if (found){ //if range contains this frame...
    nsRect frameRect;
    GetRect(frameRect);
    nsRect rect(0, 0, frameRect.width, frameRect.height);
    Invalidate(rect, PR_FALSE);
//    ForceDrawFrame(this);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::GetPointFromOffset(nsIPresContext* aPresContext,
                                nsIRenderingContext* inRendContext,
                                PRInt32 inOffset,
                                nsPoint* outPoint)
{
  if (!aPresContext || !inRendContext || !outPoint)
    return NS_ERROR_NULL_POINTER;

  if (mContentLength <= 0) {
    outPoint->x = 0;
    outPoint->y = 0;
    return NS_OK;
  }

  inOffset-=mContentOffset;
  if (inOffset < 0){
    NS_ASSERTION(0,"offset less than this frame has in GetPointFromOffset");
    inOffset = 0;
  }
  TextStyle ts(aPresContext, *inRendContext, mStyleContext);

  
  PRUnichar wordBufMem[WORD_BUF_SIZE];
  PRUnichar paintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRUnichar* paintBuf = paintBufMem;
  PRInt32* ip = indicies;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength+1];
    paintBuf = new PRUnichar[mContentLength];
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  // Get the document
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  //nsCOMPtr<nsIWordBreaker> wb;
  //doc->GetWordBreaker(getter_AddRefs(wb));
  nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE, lb,nsnull);
  PrepareUnicodeText(tx, ip, paintBuf, &textLength);
  ip[mContentLength] = ip[mContentLength-1];
  if ((ip[mContentLength]-mContentOffset) < textLength)//must set up last one for selection beyond edge if in boundary
    ip[mContentLength]++;
  if (inOffset > mContentLength){
    NS_ASSERTION(0, "invalid offset passed to GetPointFromOffset");
    inOffset = mContentLength;
  }
  GetWidth(*inRendContext, ts,
           paintBuf, ip[inOffset]-mContentOffset,
           &width);
  (*outPoint).x = width;
  (*outPoint).y = 0;

  if (paintBuf != paintBufMem)
    delete [] paintBuf;
  if (ip != indicies) {
    delete [] ip;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsTextFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset,
                                           PRBool  inHint,
                                           PRInt32* outFrameContentOffset,
                                           nsIFrame **outChildFrame)
{
  if (nsnull == outChildFrame)
    return NS_ERROR_NULL_POINTER;
  nsresult result;
  PRInt32 contentOffset = inContentOffset;
  
  if (contentOffset != -1) //-1 signified the end of the current content
    contentOffset = inContentOffset - mContentOffset;

  if ((contentOffset > mContentLength) || ((contentOffset == mContentLength) && inHint) )
  {
    //this is not the frame we are looking for.
    nsIFrame *nextInFlow;
    nextInFlow = GetNextInFlow();
    if (nextInFlow)
    {
      return nextInFlow->GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
    }
    else if (contentOffset != mContentLength) //that condition was only for when there is a choice
      return NS_ERROR_FAILURE;
  }

  if (inContentOffset < mContentOffset) //could happen with floaters!
  {
    result = GetPrevInFlow(outChildFrame);
    if (NS_SUCCEEDED(result) && outChildFrame)
      return (*outChildFrame)->GetChildFrameContainingOffset(inContentOffset, inHint,
        outFrameContentOffset,outChildFrame);
    else
      return result;
  }
  
  *outFrameContentOffset = contentOffset;
  *outChildFrame = this;
  return NS_OK;
}


NS_IMETHODIMP
nsTextFrame::PeekOffset(nsPeekOffsetStruct *aPos) 
{

  if (!aPos || !mContent)
    return NS_ERROR_NULL_POINTER;
  if (aPos->mStartOffset < 0 )
    aPos->mStartOffset = mContentLength + mContentOffset;
  if (aPos->mStartOffset < mContentOffset){
    aPos->mStartOffset = mContentOffset;
  }
  if (aPos->mStartOffset > (mContentOffset + mContentLength)){
    nsIFrame *nextInFlow;
    nextInFlow = GetNextInFlow();
    if (!nextInFlow){
      NS_ASSERTION(PR_FALSE,"nsTextFrame::PeekOffset no more flow \n");
      return NS_ERROR_INVALID_ARG;
    }
    return nextInFlow->PeekOffset(aPos);
  }
 
  if (aPos->mAmount == eSelectLine || aPos->mAmount == eSelectBeginLine 
      || aPos->mAmount == eSelectEndLine)
  {
      return nsFrame::PeekOffset(aPos);
  }

  PRUnichar wordBufMem[WORD_BUF_SIZE];
  PRUnichar paintBufMem[TEXT_BUF_SIZE];
  PRInt32 indicies[TEXT_BUF_SIZE];
  PRInt32* ip = indicies;
  PRUnichar* paintBuf = paintBufMem;
  if (mContentLength > TEXT_BUF_SIZE) {
    ip = new PRInt32[mContentLength+1];
    paintBuf = new PRUnichar[mContentLength];
  }
  PRInt32 textLength;
  nsresult result(NS_OK);

  aPos->mResultContent = mContent;//do this right off

  switch (aPos->mAmount){
  case eSelectNoAmount : {
      aPos->mContentOffset = aPos->mStartOffset;
    }
    break;
  case eSelectCharacter : {
    // Transform text from content into renderable form
    nsIDocument* doc;
    result = mContent->GetDocument(doc);
    if (NS_FAILED(result) || !doc)
      return result;
    nsCOMPtr<nsILineBreaker> lb;
    doc->GetLineBreaker(getter_AddRefs(lb));
    //nsCOMPtr<nsIWordBreaker> wb;
    //doc->GetWordBreaker(getter_AddRefs(wb));
    NS_RELEASE(doc);

    nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE,lb,nsnull);

    PrepareUnicodeText(tx, ip, paintBuf, &textLength);
    ip[mContentLength] = ip[mContentLength-1];
    if ((ip[mContentLength]-mContentOffset) < textLength)//must set up last one for selection beyond edge if in boundary
      ip[mContentLength]++;
    nsIFrame *frameUsed = nsnull;
    PRInt32 start;
    PRBool found = PR_TRUE;
    if (aPos->mDirection == eDirPrevious){
      aPos->mContentOffset = 0;
      PRInt32 i;
      for (i = aPos->mStartOffset -1 - mContentOffset; i >=0;  i--){
        if (ip[i] < ip[aPos->mStartOffset - mContentOffset]){
          aPos->mContentOffset = i + mContentOffset;
          break;
        }
      }
      if (i <0){
        found = PR_FALSE;
        frameUsed = GetPrevInFlow();
        start = mContentOffset;
      }
    }
    else if (aPos->mDirection == eDirNext){
      PRInt32 i;
      aPos->mContentOffset = mContentLength;
      for (i = aPos->mStartOffset +1 - mContentOffset; i <= mContentLength;  i++){
        if (ip[i] > ip[aPos->mStartOffset - mContentOffset]){
          aPos->mContentOffset = i + mContentOffset;
          break;
        }
      }
/*      if (aStartOffset == 0 && (mState & TEXT_SKIP_LEADING_WS))
        i--; //back up because we just skipped over some white space. why skip over the char also?
        */
      if (i > mContentLength){
        found = PR_FALSE;
        frameUsed = GetNextInFlow();
        start = mContentOffset + mContentLength;
      }
    }
    if (!found){
      if (frameUsed){
        result = frameUsed->PeekOffset(aPos);
      }
      else {//reached end ask the frame for help
        result = nsFrame::PeekOffset(aPos);
      }
    }
    else {
      aPos->mResultContent = mContent;
    }
  }
  break;
  case eSelectWord : {
    // Transform text from content into renderable form
    nsIDocument* doc;
    result = mContent->GetDocument(doc);
    if (NS_FAILED(result) || !doc)
      return result;
    nsCOMPtr<nsILineBreaker> lb;
    doc->GetLineBreaker(getter_AddRefs(lb));
    nsCOMPtr<nsIWordBreaker> wb;
    doc->GetWordBreaker(getter_AddRefs(wb));
    NS_RELEASE(doc);

    nsTextTransformer tx(wordBufMem, WORD_BUF_SIZE,lb,wb);

    PrepareUnicodeText(tx, ip, paintBuf, &textLength);
    nsIFrame *frameUsed = nsnull;
    PRBool keepSearching; //if you run out of chars before you hit the end of word, maybe next frame has more text to select?
    PRInt32 start;
    PRBool found = PR_FALSE;
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;
    if (aPos->mDirection == eDirPrevious){
      keepSearching = PR_TRUE;
      tx.Init(this, mContent, aPos->mStartOffset);
      if (tx.GetPrevWord(PR_FALSE, wordLen, contentLen, isWhitespace, PR_FALSE)){
        if ((aPos->mEatingWS && !isWhitespace) || !aPos->mEatingWS){
          aPos->mContentOffset = aPos->mStartOffset - contentLen;
          //check for whitespace next.
          if (aPos->mContentOffset > mContentOffset)
            keepSearching = PR_FALSE;//reached the beginning of a word
          aPos->mEatingWS = !isWhitespace;//nowhite space, just eat chars.
          while (isWhitespace && tx.GetPrevWord(PR_FALSE, wordLen, contentLen, isWhitespace, PR_FALSE)){
            aPos->mContentOffset -= contentLen;
            aPos->mEatingWS = PR_FALSE;
          }
          keepSearching = aPos->mContentOffset <= mContentOffset;
          if (!isWhitespace){
            if (!keepSearching)
              found = PR_TRUE;
            else
              aPos->mEatingWS = PR_TRUE;
          }
        }
        else {
          aPos->mContentOffset = mContentLength + mContentOffset;
          found = PR_TRUE;
        }
      }
      frameUsed = GetPrevInFlow();
      start = -1; //start at end
    }
    else if (aPos->mDirection == eDirNext){
      tx.Init(this, mContent, aPos->mStartOffset );

#ifdef DEBUGWORDJUMP
printf("Next- Start=%d aPos->mEatingWS=%s\n", aPos->mStartOffset, aPos->mEatingWS ? "TRUE" : "FALSE");
#endif

      if (tx.GetNextWord(PR_FALSE, wordLen, contentLen, isWhitespace, PR_FALSE)){

#ifdef DEBUGWORDJUMP
printf("GetNextWord return non null, wordLen%d, contentLen%d isWhitespace=%s\n", 
       wordLen, contentLen, isWhitespace ? "WS" : "NOT WS");
#endif

        if ((aPos->mEatingWS && isWhitespace) || !aPos->mEatingWS){
          aPos->mContentOffset = aPos->mStartOffset + contentLen;
          //check for whitespace next.
          keepSearching = PR_TRUE;
          isWhitespace = PR_TRUE;
          while (tx.GetNextWord(PR_FALSE, wordLen, contentLen, isWhitespace, PR_FALSE) && isWhitespace){
#ifdef DEBUGWORDJUMP
printf("2-GetNextWord return non null, wordLen%d, contentLen%d isWhitespace=%s\n", 
       wordLen, contentLen, isWhitespace ? "WS" : "NOT WS");
#endif
            aPos->mContentOffset += contentLen;
            keepSearching = PR_FALSE;
            isWhitespace = PR_FALSE;
          }
        }
        else if (aPos->mEatingWS)
          aPos->mContentOffset = mContentOffset;

        if (!isWhitespace){
          found = PR_TRUE;
          aPos->mEatingWS = PR_FALSE;
        }
        else if (!keepSearching) //we have found the "whole" word so just looking for WS
          aPos->mEatingWS = PR_TRUE;
      }
      frameUsed = GetNextInFlow();
      start = 0;
    }
#ifdef DEBUGWORDJUMP
printf("aEatingWS = %s\n" , aPos->mEatingWS ? "TRUE" : "FALSE");
#endif
    if (!found || (aPos->mContentOffset > (mContentOffset + mContentLength)) || (aPos->mContentOffset < mContentOffset)){ //gone too far
      if (frameUsed){
        aPos->mStartOffset = start;
        result = frameUsed->PeekOffset(aPos);
      }
      else {//reached end ask the frame for help
        result = nsFrame::PeekOffset(aPos);
      }
    }
    else {
      aPos->mResultContent = mContent;
    }
  }
    break;
  default: result = NS_ERROR_FAILURE; break;
  }
  // Cleanup
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
  if (ip != indicies) {
    delete [] ip;
  }
  if (NS_FAILED(result)){
    aPos->mResultContent = mContent;
    //aPos->mContentOffset = aPos->mStartOffset;
    result = NS_OK;
  }
  aPos->mResultFrame = this;
  return result;
}

NS_IMETHODIMP
nsTextFrame::HandleMultiplePress(nsIPresContext& aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus&  aEventStatus)
{
  if (!DisplaySelection(aPresContext)) {
    return NS_OK;
  }
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (me->clickCount > 2)
    return nsFrame::HandleMultiplePress(aPresContext,aEvent,aEventStatus);
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext.GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIRenderingContext> acx;      
    nsCOMPtr<nsIFocusTracker> tracker;
    tracker = do_QueryInterface(shell, &rv);
    if (NS_FAILED(rv) || !tracker)
      return rv;
    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    if (NS_SUCCEEDED(rv)){
      PRInt32 startPos = 0;
      PRInt32 contentOffsetEnd = 0;
      nsCOMPtr<nsIContent> newContent;
      if (NS_SUCCEEDED(GetPosition(aPresContext, aEvent->point.x,
                       getter_AddRefs(newContent), startPos, contentOffsetEnd))){
        //find which word needs to be selected! use peek offset one way then the other
        nsCOMPtr<nsIContent> startContent;
        nsCOMPtr<nsIDOMNode> startNode;
        nsCOMPtr<nsIContent> endContent;
        nsCOMPtr<nsIDOMNode> endNode;
        //peeks{}
        nsPeekOffsetStruct startpos;
        startpos.SetData(tracker, 
                        0, 
                        eSelectWord,
                        eDirPrevious,
                        startPos,
                        PR_FALSE,
                        PR_TRUE);
        rv = PeekOffset(&startpos);
        if (NS_FAILED(rv))
          return rv;
        nsPeekOffsetStruct endpos;
        endpos.SetData(tracker, 
                        0, 
                        eSelectWord,
                        eDirNext,
                        startPos,
                        PR_FALSE,
                        PR_FALSE);
        rv = PeekOffset(&endpos);
        if (NS_FAILED(rv))
          return rv;

        endNode = do_QueryInterface(endpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;
        startNode = do_QueryInterface(startpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;

        nsCOMPtr<nsIDOMSelection> selection;
        if (NS_SUCCEEDED(shell->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection)))){
          rv = selection->Collapse(startNode,startpos.mContentOffset);
          if (NS_FAILED(rv))
            return rv;
          rv = selection->Extend(endNode,endpos.mContentOffset);
          if (NS_FAILED(rv))
            return rv;
        }
        //no release 
      }
    }
  }
  return NS_OK;

}

NS_IMETHODIMP
nsTextFrame::GetOffsets(PRInt32 &start, PRInt32 &end) const
{
  start = mContentOffset;
  end = mContentOffset+mContentLength;
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::Reflow(nsIPresContext& aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": BeginReflow: availableSize=%d,%d\n",
         aReflowState.availableWidth, aReflowState.availableHeight);
#endif

  // XXX If there's no line layout, we shouldn't even have created this
  // frame. This may happen if, for example, this is text inside a table
  // but not inside a cell. For now, just don't reflow.
  if (nsnull == aReflowState.mLineLayout) {
    // XXX Add a method to aMetrics that does this; we do it several places
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
    if (nsnull != aMetrics.maxElementSize) {
      aMetrics.maxElementSize->width = 0;
      aMetrics.maxElementSize->height = 0;
    }
    return NS_OK;
  }

  // Get starting offset into the content
  PRInt32 startingOffset = 0;
  if (nsnull != mPrevInFlow) {
    nsTextFrame* prev = (nsTextFrame*) mPrevInFlow;
    startingOffset = prev->mContentOffset + prev->mContentLength;
  }

  nsLineLayout& lineLayout = *aReflowState.mLineLayout;
  TextStyle ts(&aPresContext, *aReflowState.rendContext, mStyleContext);

  // Clear out the reflow state flags in mState (without destroying
  // the TEXT_BLINK_ON bit).
  mState &= ~TEXT_REFLOW_FLAGS;
  if (ts.mFont->mFont.decorations & NS_STYLE_TEXT_DECORATION_BLINK) {
    if (0 == (mState & TEXT_BLINK_ON)) {
      mState |= TEXT_BLINK_ON;
      gTextBlinker->AddFrame(this);
    }
  }
  else {
    if (0 != (mState & TEXT_BLINK_ON)) {
      mState &= ~TEXT_BLINK_ON;
      gTextBlinker->RemoveFrame(this);
    }
  }

  PRBool wrapping = (NS_STYLE_WHITESPACE_NORMAL == ts.mText->mWhiteSpace) ||
    (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == ts.mText->mWhiteSpace);
  PRBool firstLetterOK = lineLayout.GetFirstLetterStyleOK();
  PRBool justDidFirstLetter = PR_FALSE;

  // Set whitespace skip flag
  PRBool skipWhitespace = PR_FALSE;
  if (!ts.mPreformatted) {
    if (lineLayout.GetEndsInWhiteSpace()) {
      skipWhitespace = PR_TRUE;
    }
  }

  nscoord x = 0;
  nscoord maxWidth = aReflowState.availableWidth;
  nscoord maxWordWidth = 0;
  nscoord prevMaxWordWidth = 0;
  PRBool endsInWhitespace = PR_FALSE;
  PRBool endsInNewline = PR_FALSE;

  nsCOMPtr<nsIDocument> doc;
  mContent->GetDocument(*getter_AddRefs(doc));
  // Setup text transformer to transform this frames text content
  PRUnichar wordBuf[WORD_BUF_SIZE];
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(wordBuf, WORD_BUF_SIZE,lb,nsnull);
  nsresult rv = tx.Init(this, mContent, startingOffset);
  if (NS_OK != rv) {
    return rv;
  }
  PRInt32 contentLength = tx.GetContentLength();

  // Offset tracks how far along we are in the content
  PRInt32 offset = startingOffset;
  PRInt32 prevOffset = -1;
  nscoord lastWordWidth = 0;

  // Loop over words and whitespace in content and measure. Set inWord
  // to true if we are part of a previous piece of text's word. This
  // is only valid for one pass through the measuring loop.
  PRBool inWord = lineLayout.InWord() ||
    ((nsnull != mPrevInFlow) &&
     (((nsTextFrame*)mPrevInFlow)->mState & TEXT_FIRST_LETTER));
  if (inWord) {
    mState |= TEXT_IN_WORD;
  }
  mState &= ~TEXT_FIRST_LETTER;

  PRInt32 column = lineLayout.GetColumn();
  PRInt32 prevColumn = column;
  mColumn = column;
  PRBool breakable = lineLayout.LineIsBreakable();
  PRInt32 lastWordLen;
  PRUnichar* bp = nsnull;
  for (;;) {
    // Get next word/whitespace from the text
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;
    bp = tx.GetNextWord(inWord, wordLen, contentLen, isWhitespace);
    if (nsnull == bp) {
      break;
    }
    lastWordLen = wordLen;
    inWord = PR_FALSE;

    // Measure the word/whitespace
    nscoord width;
    if (isWhitespace) {
      if (0 == wordLen) {
        // We hit a newline. Stop looping.
        prevOffset = offset;
        offset++;
        endsInWhitespace = PR_TRUE;
        if (ts.mPreformatted) {
          endsInNewline = PR_TRUE;
        }
        break;
      }
      if (skipWhitespace) {
        offset += contentLen;
        skipWhitespace = PR_FALSE;

        // Only set flag when we actually do skip whitespace
        mState |= TEXT_SKIP_LEADING_WS;
        continue;
      }
      if ('\t' == bp[0]) {
        // Expand tabs to the proper width
        wordLen = 8 - (7 & column);
        width = ts.mSpaceWidth * wordLen;
      }
      else {
        width = ts.mSpaceWidth + ts.mWordSpacing;/* XXX simplistic */
      }
      breakable = PR_TRUE;
      firstLetterOK = PR_FALSE;
    } else {
      if (firstLetterOK) {
        // XXX need a lookup function here; plus look ahead using the
        // text-runs
        if ((bp[0] == '\'') || (bp[0] == '\"')) {
          wordLen = 2;
          contentLen = 2;
        }
        else {
          wordLen = 1;
          contentLen = 1;
        }
        justDidFirstLetter = PR_TRUE;
      }
      if (ts.mSmallCaps) {
        MeasureSmallCapsText(aReflowState, ts, bp, wordLen, &width);
      }
      else {
        aReflowState.rendContext->GetWidth(bp, wordLen, width);
        if (ts.mLetterSpacing) {
          width += ts.mLetterSpacing * wordLen;
        }
      }
      skipWhitespace = PR_FALSE;
      lastWordWidth = width;
    }

    // See if there is room for the text
    if ((0 != x) && wrapping && (x + width > maxWidth)) {
      // The text will not fit.
#ifdef NOISY_REFLOW
      ListTag(stdout);
      printf(": won't fit (at offset=%d) x=%d width=%d maxWidth=%d\n",
             offset, x, width, maxWidth);
#endif
      break;
    }
    prevColumn = column;
    column += wordLen;
    x += width;
    prevMaxWordWidth = maxWordWidth;
    if (width > maxWordWidth) {
      maxWordWidth = width;
    }
    endsInWhitespace = isWhitespace;
    prevOffset = offset;
    offset += contentLen;
    if (!isWhitespace && justDidFirstLetter) {
      // Time to stop
      break;
    }
  }
  if (tx.HasMultibyte()) {
    mState |= TEXT_HAS_MULTIBYTE;
  }

  // Post processing logic to deal with word-breaking that spans
  // multiple frames.
  if (lineLayout.InWord()) {
    // We are already in a word. This means a text frame prior to this
    // one had a fragment of a nbword that is joined with this
    // frame. It also means that the prior frame already found this
    // frame and recorded it as part of the word.
#ifdef DEBUG_WORD_WRAPPING
    ListTag(stdout);
    printf(": in word; skipping\n");
#endif
    lineLayout.ForgetWordFrame(this);
  }

  if (!lineLayout.InWord()) {
    // There is no currently active word. This frame may contain the
    // start of one.
    if (endsInWhitespace) {
      // Nope, this frame doesn't start a word.
      lineLayout.ForgetWordFrames();
    }
    else if ((offset == contentLength) && (prevOffset >= 0)) {
      // Force breakable to false when we aren't wrapping (this
      // guarantees that the combined word will stay together)
      if (!wrapping) {
        breakable = PR_FALSE;
      }

      // This frame does start a word. However, there is no point
      // messing around with it if we are already out of room. We
      // always have room if we are not breakable.
      if (!breakable || (x <= maxWidth)) {
        // There is room for this word fragment. It's possible that
        // this word fragment is the end of the text-run. If it's not
        // then we continue with the look-ahead processing.
        nsIFrame* next = lineLayout.FindNextText(this);
        if (nsnull != next) {
#ifdef DEBUG_WORD_WRAPPING
          nsAutoString tmp(tx.GetTextAt(0), offset-prevOffset);
          ListTag(stdout);
          printf(": start='");
          fputs(tmp, stdout);
          printf("' baseWidth=%d [%d,%d] next=",
                 lastWordWidth, prevOffset, offset);
          ListTag(stdout, next);
          printf("\n");
#endif
          PRUnichar* pWordBuf = wordBuf;
          PRUint32   aWordBufLen = WORD_BUF_SIZE;
          if((nsnull !=bp) && (bp != wordBuf))
          {
             pWordBuf = bp;            
             // XXX To Do:
             // make the aWordBufLen equal to mBufferLength of nsTextTransformer here
          }
          // Look ahead in the text-run and compute the final word
          // width, taking into account any style changes and stopping
          // at the first breakable point.
          nscoord wordWidth = ComputeTotalWordWidth(&aPresContext, lb, lineLayout,
                                                    aReflowState, next,
                                                    lastWordWidth,
                                                    pWordBuf,
                                                    lastWordLen,
                                                    aWordBufLen
                                                    );
          if (!breakable || (x - lastWordWidth + wordWidth <= maxWidth)) {
            // The fully joined word has fit. Account for the joined
            // word's affect on the max-element-size here (since the
            // joined word is large than it's pieces, the right effect
            // will occur from the perspective of the container
            // reflowing this frame)
            if (wordWidth > maxWordWidth) {
              maxWordWidth = wordWidth;
            }
          }
          else {
#ifdef NOISY_REFLOW
            ListTag(stdout);
            printf(": look-ahead (didn't fit) x=%d wordWidth=%d lastWordWidth=%d\n",
                   x, wordWidth, lastWordWidth);
#endif
            // The fully joined word won't fit. We need to reduce our
            // size by the lastWordWidth.
            x -= lastWordWidth;
            maxWordWidth = prevMaxWordWidth;
            offset = prevOffset;
            column = prevColumn;
#ifdef DEBUG_WORD_WRAPPING
            printf("  x=%d maxWordWidth=%d len=%d\n", x, maxWordWidth,
                   offset - startingOffset);
#endif
            lineLayout.ForgetWordFrames();
          }
        }
      }
    }
  }
  lineLayout.SetColumn(column);

  // Inform line layout of how this piece of text ends in whitespace
  // (only text objects do this). Note that if x is zero then this
  // text object collapsed into nothingness which means it shouldn't
  // effect the current setting of the ends-in-whitespace flag.
  lineLayout.SetUnderstandsWhiteSpace(PR_TRUE);
  if (0 != x) {
    lineLayout.SetEndsInWhiteSpace(endsInWhitespace);
  }
  if (justDidFirstLetter) {
    lineLayout.SetFirstLetterFrame(this);
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
    mState |= TEXT_FIRST_LETTER;
  }

  // Setup metrics for caller; store final max-element-size information
  aMetrics.width = x;
  mComputedWidth = x;
  if ((0 == x) && !ts.mPreformatted) {
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
  }
  else {
    ts.mNormalFont->GetHeight(aMetrics.height);
    ts.mNormalFont->GetMaxAscent(aMetrics.ascent);
    ts.mNormalFont->GetMaxDescent(aMetrics.descent);
  }
  if (!wrapping) {
    maxWordWidth = x;
  }
  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width = maxWordWidth;
    aMetrics.maxElementSize->height = aMetrics.height;
  }

  // Set content offset and length
  mContentOffset = startingOffset;
  mContentLength = offset - startingOffset;

  // If it's an incremental reflow command, then invalidate our existing
  // bounds.
  // XXX We need a finer granularity than this, but it isn't clear what
  // has actually changed...
  if (eReflowReason_Incremental == aReflowState.reason) {
    Invalidate(mRect);
  }

  //go to selection and ask if we are selected here. and where!!

  nsReflowStatus rs = (offset == contentLength)
    ? NS_FRAME_COMPLETE
    : NS_FRAME_NOT_COMPLETE;
  if (endsInNewline) {
    rs = NS_INLINE_LINE_BREAK_AFTER(rs);
  }
  else if ((offset != contentLength) && (offset == startingOffset)) {
    // Break-before a long-word that doesn't fit here
    rs = NS_INLINE_LINE_BREAK_BEFORE();
  }
  aStatus = rs;

#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": desiredSize=%d,%d(a=%d/d=%d) status=%x\n",
         aMetrics.width, aMetrics.height, aMetrics.ascent, aMetrics.descent,
         aStatus);
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::AdjustFrameSize(nscoord aExtraSpace, nscoord& aUsedSpace)
{
  // Get the text fragments that make up our content
  const nsTextFragment* frag;
  PRInt32 numFrags;
  nsITextContent* tc;
  if (NS_OK == mContent->QueryInterface(kITextContentIID, (void**) &tc)) {
    tc->GetText(frag, numFrags);
    NS_RELEASE(tc);

    // Find fragment that contains the end of the mapped content
    PRInt32 endIndex = mContentOffset + mContentLength;
    PRInt32 offset = 0;
    const nsTextFragment* lastFrag = frag + numFrags;
    while (frag < lastFrag) {
      PRInt32 fragLen = frag->GetLength();
      if (endIndex <= offset + fragLen) {
        offset = mContentOffset - offset;
        if (frag->Is2b()) {
          const PRUnichar* cp = frag->Get2b() + offset;
          const PRUnichar* end = cp + mContentLength;
          while (cp < end) {
            PRUnichar ch = *cp++;
            if (XP_IS_SPACE(ch)) {
              aUsedSpace = aExtraSpace;
              mRect.width += aExtraSpace;
              return NS_OK;
            }
          }
        }
        else {
          const unsigned char* cp =
            ((const unsigned char*)frag->Get1b()) + offset;
          const unsigned char* end = cp + mContentLength;
          while (cp < end) {
            PRUnichar ch = PRUnichar(*cp++);
            if (XP_IS_SPACE(ch)) {
              aUsedSpace = aExtraSpace;
              mRect.width += aExtraSpace;
              return NS_OK;
            }
          }
        }
        break;
      }
      offset += fragLen;
      frag++;
    }
  }
  aUsedSpace = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::TrimTrailingWhiteSpace(nsIPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth)
{
  nscoord dw = 0;
  const nsStyleText* textStyle = (const nsStyleText*)
    mStyleContext->GetStyleData(eStyleStruct_Text);
  if ((NS_STYLE_WHITESPACE_PRE != textStyle->mWhiteSpace) &&
      (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP != textStyle->mWhiteSpace)) {
    // Get font metrics for a space so we can adjust the width by the
    // right amount.
    const nsStyleFont* fontStyle = (const nsStyleFont*)
      mStyleContext->GetStyleData(eStyleStruct_Font);
    nscoord spaceWidth;
    aRC.SetFont(fontStyle->mFont);
    aRC.GetWidth(' ', spaceWidth);

    // Get the text fragments that make up our content
    const nsTextFragment* frag;
    PRInt32 numFrags;
    nsITextContent* tc;
    if (NS_OK == mContent->QueryInterface(kITextContentIID, (void**) &tc)) {
      tc->GetText(frag, numFrags);
      NS_RELEASE(tc);

      // Find fragment that contains the end of the mapped content
      PRInt32 endIndex = mContentOffset + mContentLength;
      PRInt32 offset = 0;
      const nsTextFragment* lastFrag = frag + numFrags;
      while (frag < lastFrag) {
        PRInt32 fragLen = frag->GetLength();
        if (endIndex <= offset + fragLen) {
          // Look inside the fragments last few characters and see if they
          // are whitespace. If so, count how much width was supplied by
          // them.
          offset = mContentOffset - offset;
          if (frag->Is2b()) {
            // XXX If by chance the last content fragment is *all*
            // whitespace then this won't back up far enough.
            const PRUnichar* cp = frag->Get2b() + offset;
            const PRUnichar* end = cp + mContentLength;
            if (--end >= cp) {
              PRUnichar ch = *end;
              if (XP_IS_SPACE(ch)) {
                dw = spaceWidth;
              }
            }
          }
          else {
            const unsigned char* cp =
              ((const unsigned char*)frag->Get1b()) + offset;
            const unsigned char* end = cp + mContentLength;
            if (--end >= cp) {
              PRUnichar ch = PRUnichar(*end);
              if (XP_IS_SPACE(ch)) {
                dw = spaceWidth;
              }
            }
          }
          break;
        }
        offset += fragLen;
        frag++;
      }
    }
    if (mRect.width > dw) {
      mRect.width -= dw;
    }
    else {
      dw = mRect.width;
      mRect.width = 0;
    }
    mComputedWidth -= dw;
  }
  if (0 != dw) {
    mState |= TEXT_TRIMMED_WS;
  }
  else {
    mState &= ~TEXT_TRIMMED_WS;
  }
  aDeltaWidth = dw;
  return NS_OK;
}

nscoord
nsTextFrame::ComputeTotalWordWidth(nsIPresContext* aPresContext,
                                   nsILineBreaker* aLineBreaker,
                                   nsLineLayout& aLineLayout,
                                   const nsHTMLReflowState& aReflowState,
                                   nsIFrame* aNextFrame,
                                   nscoord aBaseWidth,
                                   const PRUnichar* aWordBuf,
                                   PRUint32 aWordBufLen,
                                   PRUint32 aWordBufSize)
{
  nscoord addedWidth = 0;
  while (nsnull != aNextFrame) {
    nsIContent* content = nsnull;
    if ((NS_OK == aNextFrame->GetContent(&content)) && (nsnull != content)) {
#ifdef DEBUG_WORD_WRAPPING
      printf("  next textRun=");
      nsFrame::ListTag(stdout, aNextFrame);
      printf("\n");
#endif
      nsITextContent* tc;
      if (NS_OK == content->QueryInterface(kITextContentIID, (void**)&tc)) {
        PRBool stop;
        nscoord moreWidth = ComputeWordFragmentWidth(aPresContext,
                                                     aLineBreaker,
                                                     aLineLayout,
                                                     aReflowState,
                                                     aNextFrame, content, tc,
                                                     &stop,
                                                     aWordBuf,
                                                     aWordBufLen,
                                                     aWordBufSize);
        NS_RELEASE(tc);
        NS_RELEASE(content);
        addedWidth += moreWidth;
#ifdef DEBUG_WORD_WRAPPING
        printf("  moreWidth=%d (addedWidth=%d) stop=%c\n", moreWidth,
               addedWidth, stop?'T':'F');
#endif
        if (stop) {
          goto done;
        }
      }
      else {
        // It claimed it was text but it doesn't implement the
        // nsITextContent API. Therefore I don't know what to do with it
        // and can't look inside it. Oh well.
        NS_RELEASE(content);
        goto done;
      }
    }

    // Move on to the next frame in the text-run
    aNextFrame = aLineLayout.FindNextText(aNextFrame);
  }

 done:;
#ifdef DEBUG_WORD_WRAPPING
  printf("  total word width=%d\n", aBaseWidth + addedWidth);
#endif
  return aBaseWidth + addedWidth;
}

#if 0
nsIStyleContext*
nsTextFrame::GetCorrectStyleContext(nsIPresContext* aPresContext,
                                    nsLineLayout& aLineLayout,
                                    const nsHTMLReflowState& aReflowState,
                                    nsIFrame* aTextFrame)
{
}
#endif
                                    
nscoord
nsTextFrame::ComputeWordFragmentWidth(nsIPresContext* aPresContext,
                                      nsILineBreaker* aLineBreaker,
                                      nsLineLayout& aLineLayout,
                                      const nsHTMLReflowState& aReflowState,
                                      nsIFrame* aTextFrame,
                                      nsIContent* aContent,
                                      nsITextContent* aText,
                                      PRBool* aStop,
                                      const PRUnichar* aWordBuf,
                                      PRUint32 &aWordBufLen,
                                      PRUint32 aWordBufSize)
{
  PRUnichar buf[TEXT_BUF_SIZE];
  nsTextTransformer tx(buf, TEXT_BUF_SIZE,aLineBreaker,nsnull);
  tx.Init(aTextFrame, aContent, 0);
                       
  PRBool isWhitespace;
  PRInt32 wordLen, contentLen;
  PRUnichar* bp = tx.GetNextWord(PR_TRUE, wordLen, contentLen, isWhitespace);
  if ((nsnull == bp) || isWhitespace) {
    // Don't bother measuring nothing
    *aStop = PR_TRUE;
    return 0;
  }
  *aStop = contentLen < tx.GetContentLength();

  // we need to adjust the length by look at the two pieces together
  PRUint32 copylen = 2*( ((wordLen + aWordBufLen) > aWordBufSize) ? 
                         (aWordBufSize - aWordBufLen) : 
                         wordLen
                       );
  if((aWordBufSize > aWordBufLen) && (copylen > 0))
  {
    nsCRT::memcpy((void*)&(aWordBuf[aWordBufLen]), buf, copylen);

    PRUint32 breakP=0;
    PRBool needMore=PR_TRUE;
    nsresult lres = aLineBreaker->Next(aWordBuf, aWordBufLen+wordLen, 0, &breakP, &needMore);
    if(NS_SUCCEEDED(lres)) 
    {
       // when we look at two pieces text together, we might decide to break
       // eariler than if we only look at the 2nd pieces of text
       if(!needMore && (breakP < (aWordBufLen + wordLen)))
       {
           wordLen = breakP - aWordBufLen; 
           if(wordLen < 0)
               wordLen = 0;
           *aStop = PR_TRUE;
       } 
    }
    // if we don't stop, we need to extent the buf so the next
    // one can see this part otherwise, it does not matter since we will stop anyway
    if(! *aStop) 
       aWordBufLen += wordLen;
  }
  if((*aStop) && (wordLen == 0))
	  return 0;

  nsIStyleContext* sc;
  if ((NS_OK == aTextFrame->GetStyleContext(&sc)) &&
      (nsnull != sc)) {
    // Measure the piece of text. Note that we have to select the
    // appropriate font into the text first because the rendering
    // context has our font in it, not the font that aText is using.
    nscoord width;
    nsIRenderingContext& rc = *aReflowState.rendContext;
    nsIFontMetrics* oldfm;
    rc.GetFontMetrics(oldfm);

    TextStyle ts(&aLineLayout.mPresContext, rc, sc);
    if (ts.mSmallCaps) {
      MeasureSmallCapsText(aReflowState, ts, buf, wordLen, &width);
    }
    else {
      rc.GetWidth(buf, wordLen, width);
    }
    rc.SetFont(oldfm);
    NS_IF_RELEASE(oldfm);
    NS_RELEASE(sc);

#ifdef DEBUG_WORD_WRAPPING
    nsAutoString tmp(bp, wordLen);
    printf("  fragment='");
    fputs(tmp, stdout);
    printf("' width=%d [wordLen=%d contentLen=%d ContentLength=%d]\n",
           width, wordLen, contentLen, tx.GetContentLength());
#endif

    // Remember the text frame for later so that we don't bother doing
    // the word look ahead.
    aLineLayout.RecordWordFrame(aTextFrame);
    return width;
  }

  *aStop = PR_TRUE;
  return 0;
}

// Translate the mapped content into a string that's printable
void
nsTextFrame::ToCString(nsString& aBuf, PRInt32* aContentLength) const
{
  const nsTextFragment* frag;
  PRInt32 numFrags;

  // Get the frames text content
  nsITextContent* tc;
  if (NS_OK != mContent->QueryInterface(kITextContentIID, (void**) &tc)) {
    return;
  }
  tc->GetText(frag, numFrags);
  NS_RELEASE(tc);

  // Compute the total length of the text content.
  PRInt32 sum = 0;
  PRInt32 i, n = numFrags;
  for (i = 0; i < n; i++) {
    sum += frag[i].GetLength();
  }
  *aContentLength = sum;

  // Set current fragment and current fragment offset
  PRInt32 fragOffset = 0, offset = 0;
  n = numFrags;
  while (--n >= 0) {
    if (mContentOffset < offset + frag->GetLength()) {
      fragOffset = mContentOffset - offset;
      break;
    }
    offset += frag->GetLength();
    frag++;
  }

  if (0 == mContentLength) {
    return;
  }

  n = mContentLength;
  for (;;) {
    PRUnichar ch = frag->CharAt(fragOffset);
    if (ch == '\r') {
      aBuf.Append("\\r");
    } else if (ch == '\n') {
      aBuf.Append("\\n");
    } else if (ch == '\t') {
      aBuf.Append("\\t");
    } else if ((ch < ' ') || (ch >= 127)) {
      aBuf.Append("\\0");
      aBuf.Append((PRInt32)ch, 8);
    } else {
      aBuf.Append(ch);
    }
    if (--n == 0) {
      break;
    }
    if (++fragOffset == frag->GetLength()) {
      frag++;
      fragOffset = 0;
    }
  }
}

NS_IMETHODIMP
nsTextFrame::GetFrameType(nsIAtom** aType) const
{
  NS_PRECONDITION(nsnull != aType, "null OUT parameter pointer");
  *aType = nsLayoutAtoms::textFrame; 
  NS_ADDREF(*aType);
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTextFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = sizeof(*this);
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsTextFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Text", aResult);
}

NS_IMETHODIMP
nsTextFrame::List(FILE* out, PRInt32 aIndent) const
{
  // Output the tag
  IndentBy(out, aIndent);
  ListTag(out);
  nsIView* view;
  GetView(&view);
  if (nsnull != view) {
    fprintf(out, " [view=%p]", view);
  }

  PRInt32 contentLength;
  nsAutoString tmp;
  ToCString(tmp, &contentLength);

  // Output the first/last content offset and prev/next in flow info
  PRBool isComplete = (mContentOffset + mContentLength) == contentLength;
  fprintf(out, "[%d,%d,%c] ", 
          mContentOffset, mContentOffset+mContentLength-1,
          isComplete ? 'T':'F');
  
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", mNextSibling);
  }
  if (nsnull != mPrevInFlow) {
    fprintf(out, "prev-in-flow=%p ", mPrevInFlow);
  }
  if (nsnull != mNextInFlow) {
    fprintf(out, "next-in-flow=%p ", mNextInFlow);
  }

  // Output the rect and state
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fprintf(out, " sc=%p<\n", mStyleContext);

  // Output the text
  aIndent++;

  IndentBy(out, aIndent);
  fputs("\"", out);
  fputs(tmp, out);
  fputs("\"\n", out);

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}



//STORAGE FOR LATER MAYBE
#if 0
      nsTextFrame *frame = this;
      while(frame = (nsTextFrame *)frame->GetPrevInFlow()){
        result  = NS_OK;
        nsPoint futurecoord;
	      frame->GetOffsetFromView(futurecoord, &view);
	      if (view == nsnull) return NS_ERROR_UNEXPECTED;
        nscoord x,y;
	      do {
		      view->GetPosition(&x, &y);
		      futurecoord.x += x;
		      futurecoord.y += y;
		      view->GetParent(view);
	      } while (view);
        if (coord.y > futurecoord.y)
        {
          if (coord.x < futurecoord.x)
          {//keep going back until y is up again or coord.x is greater than future coord.x
            nsTextFrame *lookahead = nsnull;
            while(lookahead = (nsTextFrame *)frame->GetPrevInFlow()){
              result  = NS_OK;
              nsPoint futurecoord2;
	            lookahead->GetOffsetFromView(futurecoord2, &view);
	            if (view == nsnull) return NS_ERROR_UNEXPECTED;
	            do {
		            view->GetPosition(&x, &y);
		            futurecoord2.x += x;
		            futurecoord2.y += y;
		            view->GetParent(view);
	            } while (view);
              if (futurecoord.y > futurecoord2.y)//gone too far
                break;
              frame = lookahead;
              futurecoord = futurecoord2;
              if (coord.x >=futurecoord2.x)
                break;
            }
          }
          //definately this one then
          nscoord newcoord;
          newcoord = coord.x ;//- futurecoord.x;
#endif
