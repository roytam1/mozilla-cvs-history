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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Robert O'Callahan <roc+moz@cs.cmu.edu>
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
#include "nsILookAndFeel.h"

#include "nsILineIterator.h"

//tripple click includes
#include "nsIPref.h"
#include "nsIServiceManager.h"

#ifndef PR_ABS
#define PR_ABS(x) ((x) < 0 ? -(x) : (x))
#endif

static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_CID(kPrefCID,     NS_PREF_CID);//for tripple click pref

#ifdef NS_DEBUG
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#undef NOISY_TRIM
#else
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#undef NOISY_TRIM
#endif

// #define DEBUGWORDJUMP

//----------------------------------------------------------------------

#define TEXT_BUF_SIZE 100

//----------------------------------------

struct nsAutoIndexBuffer {
  nsAutoIndexBuffer();
  ~nsAutoIndexBuffer();

  nsresult GrowTo(PRInt32 aAtLeast);

  PRInt32* mBuffer;
  PRInt32 mBufferLen;
  PRInt32 mAutoBuffer[TEXT_BUF_SIZE];
};

nsAutoIndexBuffer::nsAutoIndexBuffer()
  : mBuffer(mAutoBuffer),
    mBufferLen(TEXT_BUF_SIZE)
{
  nsCRT::memset(mAutoBuffer, 0, sizeof(mAutoBuffer));
}

nsAutoIndexBuffer::~nsAutoIndexBuffer()
{
  if (mBuffer && (mBuffer != mAutoBuffer)) {
    delete [] mBuffer;
  }
}

nsresult
nsAutoIndexBuffer::GrowTo(PRInt32 aAtLeast)
{
  if (aAtLeast > mBufferLen)
  {
    PRInt32 newSize = mBufferLen * 2;
    if (newSize < mBufferLen + aAtLeast) {
      newSize = mBufferLen * 2 + aAtLeast;
    }
    PRInt32* newBuffer = new PRInt32[newSize];
    if (!newBuffer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    nsCRT::memset(newBuffer, 0, sizeof(PRInt32) * newSize);
    nsCRT::memcpy(newBuffer, mBuffer, sizeof(PRInt32) * mBufferLen);
    if (mBuffer != mAutoBuffer) {
      delete [] mBuffer;
    }
    mBuffer = newBuffer;
    mBufferLen = newSize;
  }
  return NS_OK;
}


//----------------------------------------------------------------------

// Helper class for managing blinking text

class nsBlinkTimer : public nsITimerCallback
{
public:
  nsBlinkTimer();
  virtual ~nsBlinkTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsIPresContext* aPresContext, nsIFrame* aFrame);

  PRBool RemoveFrame(nsIFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  NS_IMETHOD_(void) Notify(nsITimer *timer);

  static nsresult AddBlinkFrame(nsIPresContext* aPresContext, nsIFrame* aFrame);
  static nsresult RemoveBlinkFrame(nsIFrame* aFrame);
  
  static PRBool   GetBlinkIsOff() { return sBlinkTextOff; }
  
protected:

  struct FrameData {
    nsIPresContext* mPresContext;  // pres context associated with the frame
    nsIFrame*       mFrame;


    FrameData(nsIPresContext* aPresContext,
              nsIFrame*       aFrame)
      : mPresContext(aPresContext), mFrame(aFrame) {}
  };

  nsITimer*       mTimer;
  nsVoidArray     mFrames;
  nsIPresContext* mPresContext;

protected:

  static nsBlinkTimer* sTextBlinker;
  static PRBool        sBlinkTextOff;
  
};

nsBlinkTimer* nsBlinkTimer::sTextBlinker = nsnull;
PRBool        nsBlinkTimer::sBlinkTextOff = PR_FALSE;

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
  sTextBlinker = nsnull;
}

void nsBlinkTimer::Start()
{
  nsresult rv = NS_NewTimer(&mTimer);
  if (NS_OK == rv) {
    mTimer->Init(this, 750, NS_PRIORITY_NORMAL, NS_TYPE_REPEATING_PRECISE);
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

void nsBlinkTimer::AddFrame(nsIPresContext* aPresContext, nsIFrame* aFrame) {
  FrameData* frameData = new FrameData(aPresContext, aFrame);
  mFrames.AppendElement(frameData);
  if (1 == mFrames.Count()) {
    Start();
  }
}

PRBool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  PRInt32 i, n = mFrames.Count();
  PRBool rv = PR_FALSE;
  for (i = 0; i < n; i++) {
    FrameData* frameData = (FrameData*) mFrames.ElementAt(i);

    if (frameData->mFrame == aFrame) {
      rv = mFrames.RemoveElementAt(i);
      delete frameData;
      break;
    }
  }
  
  if (0 == mFrames.Count()) {
    Stop();
  }
  return rv;
}

PRInt32 nsBlinkTimer::FrameCount() {
  return mFrames.Count();
}

NS_IMETHODIMP_(void) nsBlinkTimer::Notify(nsITimer *timer)
{
  // Toggle blink state bit so that text code knows whether or not to
  // render. All text code shares the same flag so that they all blink
  // in unison.
  sBlinkTextOff = PRBool(!sBlinkTextOff);

#ifndef REPEATING_TIMERS
  // XXX hack to get auto-repeating timers; restart before doing
  // expensive work so that time between ticks is more even
  Stop();
  Start();
#endif

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
    FrameData* frameData = (FrameData*) mFrames.ElementAt(i);

    // Determine damaged area and tell view manager to redraw it
    nsPoint offset;
    nsRect bounds;
    frameData->mFrame->GetRect(bounds);
    nsIView* view;
    frameData->mFrame->GetOffsetFromView(frameData->mPresContext, offset, &view);
    nsIViewManager* vm;
    view->GetViewManager(vm);
    bounds.x = offset.x;
    bounds.y = offset.y;
    vm->UpdateView(view, bounds, 0);
    NS_RELEASE(vm);
  }
}


// static
nsresult nsBlinkTimer::AddBlinkFrame(nsIPresContext* aPresContext, nsIFrame* aFrame)
{
  if (!sTextBlinker)
  {
    sTextBlinker = new nsBlinkTimer;
    if (!sTextBlinker) return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(sTextBlinker);

  sTextBlinker->AddFrame(aPresContext, aFrame);
  return NS_OK;
}


// static
nsresult nsBlinkTimer::RemoveBlinkFrame(nsIFrame* aFrame)
{
  NS_ASSERTION(sTextBlinker, "Should have blink timer here");
  
  nsBlinkTimer* blinkTimer = sTextBlinker;    // copy so we can call NS_RELEASE on it
  if (!blinkTimer) return NS_OK;
  
  blinkTimer->RemoveFrame(aFrame);  
  NS_RELEASE(blinkTimer);
  
  return NS_OK;
}


//----------------------------------------------------------------------

class nsTextFrame : public nsFrame {
public:
  nsTextFrame();

  // nsIFrame
  NS_IMETHOD Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer);

  NS_IMETHOD GetCursor(nsIPresContext* aPresContext,
                       nsPoint& aPoint,
                       PRInt32& aCursor);

  NS_IMETHOD ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent);

  NS_IMETHOD GetNextInFlow(nsIFrame** aNextInFlow) const {
    *aNextInFlow = mNextInFlow;
    return NS_OK;
  }
  NS_IMETHOD SetNextInFlow(nsIFrame* aNextInFlow) {
    mNextInFlow = aNextInFlow;
    return NS_OK;
  }
  
  NS_IMETHOD  IsSplittable(nsSplittableType& aIsSplittable) const {
    aIsSplittable = NS_FRAME_SPLITTABLE;
    return NS_OK;
  }

  /**
   * Get the "type" of the frame
   *
   * @see nsLayoutAtoms::textFrame
   */
  NS_IMETHOD GetFrameType(nsIAtom** aType) const;
  
#ifdef DEBUG
  NS_IMETHOD List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
  NS_IMETHOD GetFrameName(nsString& aResult) const;
#endif

  NS_IMETHOD GetPosition(nsIPresContext* aCX,
                         const nsPoint&  aPoint,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd);

  NS_IMETHOD GetContentAndOffsetsFromPoint(nsIPresContext* aCX,
                         const nsPoint&  aPoint,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd,
                         PRBool&         aBeginFrameContent);

  NS_IMETHOD GetPositionSlowly(nsIPresContext* aCX,
                         nsIRenderingContext * aRendContext,
                         const nsPoint&        aPoint,
                         nsIContent **         aNewContent,
                         PRInt32&              aOffset);


  NS_IMETHOD SetSelected(nsIPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);

  NS_IMETHOD PeekOffset(nsIPresContext* aPresContext, nsPeekOffsetStruct *aPos);

  NS_IMETHOD HandleMultiplePress(nsIPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

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
  NS_IMETHOD Reflow(nsIPresContext* aPresContext,
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
    nscoord mAveCharWidth;
    PRBool mJustifying;
    PRBool mPreformatted;
    PRInt32 mNumSpacesToRender;
    PRInt32 mNumSpacesToMeasure;
    nscoord mExtraSpacePerSpace;
    PRInt32 mNumSpacesReceivingExtraJot;

    TextStyle(nsIPresContext* aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsIStyleContext* sc)
    {
    mFont = nsnull;
    mText = nsnull;
    mColor = nsnull;
    mNormalFont = nsnull;
    mSmallFont = nsnull;
    mLastFont = nsnull;

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
#ifdef _WIN32
      mNormalFont->GetAveCharWidth(mAveCharWidth);
#else
      mAveCharWidth = 10;
#endif
      mLastFont = mNormalFont;

      // Get the small-caps font if needed
      mSmallCaps = NS_STYLE_FONT_VARIANT_SMALL_CAPS == plainFont->variant;
      if (mSmallCaps) {
        nscoord originalSize = plainFont->size;
        plainFont->size = nscoord(0.8 * plainFont->size);
        aPresContext->GetMetricsFor(*plainFont, &mSmallFont);
        // Reset to the size value saved earlier.
        plainFont->size = originalSize;
      }
      else {
        mSmallFont = nsnull;
      }

      // Reset to the decoration saved earlier
      plainFont->decorations = originalDecorations; 

      // Get colors from look&feel
      mSelectionBGColor = NS_RGB(0, 0, 0);
      mSelectionTextColor = NS_RGB(255, 255, 255);
	    nsILookAndFeel* look = nsnull;
	    if (NS_SUCCEEDED(aPresContext->GetLookAndFeel(&look)) && look) {
	      look->GetColor(nsILookAndFeel::eColor_TextSelectBackground, mSelectionBGColor);
	      look->GetColor(nsILookAndFeel::eColor_TextSelectForeground, mSelectionTextColor);
	      NS_RELEASE(look);
	    }

      // Get the word and letter spacing
      mWordSpacing = 0;
      PRIntn unit = mText->mWordSpacing.GetUnit();
      if (eStyleUnit_Coord == unit) {
        mWordSpacing = mText->mWordSpacing.GetCoordValue();
      }

      mLetterSpacing = 0;
      unit = mText->mLetterSpacing.GetUnit();
      if (eStyleUnit_Coord == unit) {
        mLetterSpacing = mText->mLetterSpacing.GetCoordValue();
      }
      mNumSpacesToRender = 0;
      mNumSpacesToMeasure = 0;
      mNumSpacesReceivingExtraJot = 0;
      mExtraSpacePerSpace = 0;
      mPreformatted = (NS_STYLE_WHITESPACE_PRE == mText->mWhiteSpace) ||
        (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == mText->mWhiteSpace);

      mJustifying = (NS_STYLE_TEXT_ALIGN_JUSTIFY == mText->mTextAlign) &&
        !mPreformatted;
    }

    ~TextStyle() {
      NS_RELEASE(mNormalFont);
      NS_IF_RELEASE(mSmallFont);

    mFont = nsnull;
    mText = nsnull;
    mColor = nsnull;
    mNormalFont = nsnull;
    mSmallFont = nsnull;
    mLastFont = nsnull;

    }
  };

  struct TextReflowData {
    PRInt32             mX;                   // OUT
    PRInt32             mOffset;              // IN/OUT How far along we are in the content
    nscoord             mMaxWordWidth;        // OUT
    PRPackedBool        mWrapping;            // IN
    PRPackedBool        mSkipWhitespace;      // IN
    PRPackedBool        mMeasureText;         // IN
    PRPackedBool        mInWord;              // IN
    PRPackedBool        mFirstLetterOK;       // IN
    PRPackedBool        mIsBreakable;         // IN
    PRPackedBool        mComputeMaxWordWidth; // IN
  
    TextReflowData(PRInt32 aStartingOffset,
                   PRBool  aWrapping,
                   PRBool  aSkipWhitespace,
                   PRBool  aMeasureText,
                   PRBool  aInWord,
                   PRBool  aFirstLetterOK,
                   PRBool  aIsBreakable,
                   PRBool  aComputeMaxWordWidth)
      : mX(0),
        mOffset(aStartingOffset),
        mMaxWordWidth(0),
        mWrapping(aWrapping),
        mSkipWhitespace(aSkipWhitespace),
        mMeasureText(aMeasureText),
        mInWord(aInWord),
        mFirstLetterOK(aFirstLetterOK),
        mIsBreakable(aIsBreakable),
        mComputeMaxWordWidth(aComputeMaxWordWidth)
    {}
  };

  nsIDocument* GetDocument(nsIPresContext* aPresContext);

  PRIntn PrepareUnicodeText(nsTextTransformer& aTransformer,
                            nsAutoIndexBuffer* aIndexBuffer,
                            nsAutoTextBuffer* aTextBuffer,
                            PRInt32* aTextLen);
  void ComputeExtraJustificationSpacing(nsIRenderingContext& aRenderingContext,
                                        TextStyle& aTextStyle,
                                        PRUnichar* aBuffer, PRInt32 aLength, PRInt32 aNumSpaces);

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

  nsReflowStatus MeasureText(nsIPresContext*          aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsTextTransformer&       aTx,
                             nsILineBreaker*          aLb,
                             TextStyle&               aTs,
                             TextReflowData&          aTextData);
  
  void GetWidth(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord* aWidthResult);

  //this returns the index into the PAINTBUFFER of the x coord aWidth(based on 0 as far left) 
  //also note: this is NOT added to mContentOffset since that would imply that this return is
  //meaningful to content yet. use index buffer from prepareunicodestring to find the content offset.
  PRInt32 GetLengthSlowly(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord aWidth);

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
                                PRUnichar* aWordBuf,
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

  void ToCString(nsString& aBuf, PRInt32* aTotalContentLength) const;

protected:
  virtual ~nsTextFrame();

  nsIFrame* mNextInFlow;
  PRInt32   mContentOffset;
  PRInt32   mContentLength;
  PRInt32   mColumn;
  //factored out method for getwidth and getlengthslowly. if aGetWidth is non-zero number then measure to that width and return the length. else shove total width into result
  PRInt32 GetWidthOrLength(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord* aWidthResult,
                PRBool aGetWidth/* true=get width false = return length up to aWidthResult size*/);
  nsresult GetContentAndOffsetsForSelection(nsIPresContext*  aPresContext,nsIContent **aContent, PRInt32 *aOffset, PRInt32 *aLength);

};

class nsContinuingTextFrame : public nsTextFrame {
public:
  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD GetPrevInFlow(nsIFrame** aPrevInFlow) const {
    *aPrevInFlow = mPrevInFlow;
    return NS_OK;
  }
  NS_IMETHOD SetPrevInFlow(nsIFrame* aPrevInFlow) {
    mPrevInFlow = aPrevInFlow;
    return NS_OK;
  }

#ifdef DEBUG
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const;
#endif

protected:
  nsIFrame* mPrevInFlow;
};

NS_IMETHODIMP
nsContinuingTextFrame::Init(nsIPresContext*  aPresContext,
                            nsIContent*      aContent,
                            nsIFrame*        aParent,
                            nsIStyleContext* aContext,
                            nsIFrame*        aPrevInFlow)
{
  nsresult  rv;
  
  rv = nsTextFrame::Init(aPresContext, aContent, aParent, aContext, aPrevInFlow);

  if (aPrevInFlow) {
    // Hook the frame into the flow
    mPrevInFlow = aPrevInFlow;
    aPrevInFlow->SetNextInFlow(this);
  }

  return rv;
}

#ifdef DEBUG
NS_IMETHODIMP
nsContinuingTextFrame::SizeOf(nsISizeOfHandler* aHandler, PRUint32* aResult) const
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = sizeof(*this);
  return NS_OK;
}
#endif


inline nscolor EnsureDifferentColors(nscolor colorA, nscolor colorB)
{
    if (colorA == colorB)
    {
      nscolor res;
      res = NS_RGB(NS_GET_R(colorA) ^ 0xff,
                   NS_GET_G(colorA) ^ 0xff,
                   NS_GET_B(colorA) ^ 0xff);
      return res;
    }
    return colorA;
}


//DRAW SELECTION ITERATOR USED FOR TEXTFRAMES ONLY
//helper class for drawing multiply selected text
class DrawSelectionIterator
{
  enum {DISABLED_COLOR = NS_RGB(176,176,176)};
  enum {SELECTION_TYPES_WE_CARE_ABOUT=nsISelectionController::SELECTION_NONE+nsISelectionController::SELECTION_NORMAL};
public:
  DrawSelectionIterator(const SelectionDetails *aSelDetails, PRUnichar *aText,
                        PRUint32 aTextLength, nsTextFrame::TextStyle &aTextStyle,
                        PRInt8 aSelectionStatus);
  ~DrawSelectionIterator();
  PRBool      First();
  PRBool      Next();
  PRBool      IsDone();

  PRUnichar * CurrentTextUnicharPtr();
  char *      CurrentTextCStrPtr();
  PRUint32    CurrentLength();
  nsTextFrame::TextStyle & CurrentStyle();
  nscolor     CurrentForeGroundColor();
  PRBool      CurrentBackGroundColor(nscolor &aColor);
private:
  union {
    PRUnichar *mUniStr;
    char *mCStr;
  };
	PRUint32  mLength;
	PRUint32  mCurrentIdx;
  PRUint32  mCurrentLength;
  nsTextFrame::TextStyle &mOldStyle;//base new styles on this one???
  const SelectionDetails *mDetails;
  PRBool    mDone;
  PRUint8 * mTypes;
  PRBool    mInit;
  PRInt8    mSelectionStatus;//see nsIDocument.h SetDisplaySelection()
  nscolor   mDisabledColor;
  //private methods
  void FillCurrentData();
};

DrawSelectionIterator::DrawSelectionIterator(const SelectionDetails *aSelDetails, PRUnichar *aText, 
							PRUint32 aTextLength, nsTextFrame::TextStyle &aTextStyle, PRInt8 aSelectionStatus)
              :mOldStyle(aTextStyle)
{
    mDetails = aSelDetails;
    mCurrentIdx = 0;
    mUniStr = aText;
    mLength = aTextLength;
    mTypes = nsnull;
    mInit = PR_FALSE;
    mSelectionStatus = aSelectionStatus;
    mDisabledColor = EnsureDifferentColors(DISABLED_COLOR, mOldStyle.mSelectionBGColor);

    if (!aSelDetails)
    {
      mDone = PR_TRUE;
      return;
    }
    mDone = (PRBool)(mCurrentIdx>=mLength);
    if (mDone)
      return;

    //special case for 1 selection. later
    const SelectionDetails *details = aSelDetails;
    if (details->mNext)
    {
      mTypes = new PRUint8[mLength];
      if (!mTypes)
        return;
      memset(mTypes,0,mLength);//initialize to 0
      while (details)
      {
        if ((details->mType & SELECTION_TYPES_WE_CARE_ABOUT ) && 
          (details->mStart != details->mEnd))
        {
          mInit = PR_TRUE;//WE FOUND SOMETHING WE CARE ABOUT
          for (int i = details->mStart; i < details->mEnd; i++)
          {
              if ((PRUint32)i>=mLength)
              {
                NS_ASSERTION(0,"Selection Details out of range?");
                return;//eh
              }
              mTypes[i]|=details->mType;//add this bit
          }
        }
        details= details->mNext;
      }
	    if (!mInit && mTypes) //we have details but none that we care about.
	    {
          delete mTypes;
    		  mTypes = nsnull;
		      mDone = PR_TRUE;//we are finished
	    }
    }
    else if (details->mStart == details->mEnd)//no collapsed selections here!
    {
      mDone = PR_TRUE;
      return;
    }
    else if (!(details->mType & SELECTION_TYPES_WE_CARE_ABOUT ))//if all we have is selection we DONT care about, do nothing
    {
        mDone = PR_TRUE;
        return;
    }
    mInit = PR_TRUE;
}

DrawSelectionIterator::~DrawSelectionIterator()
{
  if (mTypes)
    delete mTypes;
}

void
DrawSelectionIterator::FillCurrentData()
{
  if (mDone)
    return;
  if (!mTypes)
  {
    mCurrentIdx+=mCurrentLength;
    if (mCurrentIdx >= mLength)
    {
      mDone = PR_TRUE;
      return;
    }
    if (mCurrentIdx < (PRUint32)mDetails->mStart)
    {
      mCurrentLength = mDetails->mStart;
    }
    else if (mCurrentIdx == (PRUint32)mDetails->mStart)
    {//start
        mCurrentLength = mDetails->mEnd-mCurrentIdx;
    }
    else if (mCurrentIdx > (PRUint32)mDetails->mStart)//last unselected part
    {
      mCurrentLength = mLength - mDetails->mEnd;
    }
  }
  else
  {
    mCurrentIdx+=mCurrentLength;//advance to this chunk
    if (mCurrentIdx >= mLength)
    {
      mDone = PR_TRUE;
      return;
    }
    uint8 typevalue = mTypes[mCurrentIdx];
    while (typevalue == mTypes[mCurrentIdx+mCurrentLength] && (mCurrentIdx+mCurrentLength) <mLength)
    {
      mCurrentLength++;
    }
  }
}

PRBool
DrawSelectionIterator::First()
{
  if (!mInit)
    return PR_FALSE;
  mCurrentIdx = 0;
  mCurrentLength = 0;
  if (!mTypes && mDetails->mStart == mDetails->mEnd)//no collapsed selections here!
    mDone = PR_TRUE;
  mDone = (mCurrentIdx+mCurrentLength) >= mLength;
  FillCurrentData();
  return PR_TRUE;
}



PRBool
DrawSelectionIterator::Next()
{
  if (mDone || !mInit)
    return PR_FALSE;
  FillCurrentData();//advances to next chunk
  return PR_TRUE;
}

PRBool
DrawSelectionIterator::IsDone()
{
    return mDone || !mInit;
}


PRUnichar *
DrawSelectionIterator::CurrentTextUnicharPtr()
{
  return mUniStr+mCurrentIdx;
}

char *
DrawSelectionIterator::CurrentTextCStrPtr()
{
  return mCStr+mCurrentIdx;
}

PRUint32
DrawSelectionIterator::CurrentLength()
{
    return mCurrentLength;
}

nsTextFrame::TextStyle & 
DrawSelectionIterator::CurrentStyle()
{
  return mOldStyle;
}

nscolor
DrawSelectionIterator::CurrentForeGroundColor()
{
	nscolor foreColor;
	PRBool colorSet = PR_FALSE;
  
  if (!mTypes)
  {
      if (mCurrentIdx == (PRUint32)mDetails->mStart)
      {
   			foreColor = mOldStyle.mSelectionTextColor;
   			colorSet = PR_TRUE;
   		}
  }
  else if (mTypes[mCurrentIdx] | nsISelectionController::SELECTION_NORMAL)//Find color based on mTypes[mCurrentIdx];
  {
    foreColor = mOldStyle.mSelectionTextColor;
   	colorSet = PR_TRUE;
  }

	if (colorSet && (foreColor != NS_DONT_CHANGE_COLOR))
			return foreColor;

  return mOldStyle.mColor->mColor;
}

PRBool
DrawSelectionIterator::CurrentBackGroundColor(nscolor &aColor)
{
  //Find color based on mTypes[mCurrentIdx];
  if (!mTypes)
  {
      if (mCurrentIdx == (PRUint32)mDetails->mStart)
      {
        aColor = (mSelectionStatus==nsIDocument::SELECTION_ON)?mOldStyle.mSelectionBGColor:mDisabledColor;
        return PR_TRUE;
      }
  }
  else if (mTypes[mCurrentIdx] | nsISelectionController::SELECTION_NORMAL)
  {
    aColor = (mSelectionStatus==nsIDocument::SELECTION_ON)?mOldStyle.mSelectionBGColor:mDisabledColor;
    return PR_TRUE;
  }
  return PR_FALSE;
}


//END DRAWSELECTIONITERATOR!!




// Flag information used by rendering code. This information is
// computed by the ResizeReflow code. The flags are stored in the
// mState variable in the frame class private section.

// Flag indicating that whitespace was skipped
#define TEXT_SKIP_LEADING_WS 0x01000000
#define TEXT_HAS_MULTIBYTE   0x02000000
#define TEXT_IN_WORD         0x04000000
// This bit is set on the first frame in a continuation indicating
// that it was chopped short because of :first-letter style.
#define TEXT_FIRST_LETTER    0x08000000
#define TEXT_WAS_TRANSFORMED 0x10000000

// Bits in mState used for reflow flags
#define TEXT_REFLOW_FLAGS    0x1F000000

#define TEXT_TRIMMED_WS      0x20000000

#define TEXT_OPTIMIZE_RESIZE 0x40000000

#define TEXT_BLINK_ON        0x80000000

//----------------------------------------------------------------------

nsresult
NS_NewTextFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTextFrame* it = new (aPresShell) nsTextFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsresult
NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsContinuingTextFrame* it = new (aPresShell) nsContinuingTextFrame;
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aNewFrame = it;
  return NS_OK;
}

nsTextFrame::nsTextFrame()
{
}

nsTextFrame::~nsTextFrame()
{
  if (0 != (mState & TEXT_BLINK_ON))
  {
    nsBlinkTimer::RemoveBlinkFrame(this);
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
nsTextFrame::GetCursor(nsIPresContext* aPresContext,
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

static nsIFrame*
GetLastInFlow(nsIFrame* aFrame)
{
  nsIFrame* lastInFlow;
  nsIFrame* nextInFlow = aFrame;
  while (nsnull!=nextInFlow)  {
    lastInFlow = nextInFlow;
    lastInFlow->GetNextInFlow(&nextInFlow);
  }
  NS_POSTCONDITION(nsnull!=lastInFlow, "illegal state in flow chain.");
  return lastInFlow;
}

NS_IMETHODIMP
nsTextFrame::ContentChanged(nsIPresContext* aPresContext,
                            nsIContent*     aChild,
                            nsISupports*    aSubContent)
{
  nsIFrame* targetTextFrame = this;

  PRBool markAllDirty = PR_TRUE;
  if (aSubContent) {
    nsCOMPtr<nsITextContentChangeData> tccd = do_QueryInterface(aSubContent);
    if (tccd) {
      nsITextContentChangeData::ChangeType type;
      tccd->GetChangeType(&type);
      if (nsITextContentChangeData::Append == type) {
        markAllDirty = PR_FALSE;
        nsTextFrame* frame = (nsTextFrame*)::GetLastInFlow(this);
        frame->mState |= NS_FRAME_IS_DIRTY;
        targetTextFrame = frame;
      }
    }
  }

  if (markAllDirty) {
    // Mark this frame and all the next-in-flow frames as dirty
    nsTextFrame*  textFrame = this;
    while (textFrame) {
      textFrame->mState |= NS_FRAME_IS_DIRTY;
      textFrame = (nsTextFrame*)textFrame->mNextInFlow;
    }
  }

  // Ask the parent frame to reflow me.  
  nsresult rv;                                                    
  nsCOMPtr<nsIPresShell> shell;
  rv = aPresContext->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell && mParent) {
    mParent->ReflowDirtyChild(shell, targetTextFrame);
  }
  

  return rv;
}

NS_IMETHODIMP
nsTextFrame::Paint(nsIPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect,
                   nsFramePaintLayer aWhichLayer)
{
  if (NS_FRAME_PAINT_LAYER_FOREGROUND != aWhichLayer) {
    return NS_OK;
  }
  if ((0 != (mState & TEXT_BLINK_ON)) && nsBlinkTimer::GetBlinkIsOff()) {
    return NS_OK;
  }
  nsIStyleContext* sc = mStyleContext;
  const nsStyleDisplay* disp = (const nsStyleDisplay*)
    sc->GetStyleData(eStyleStruct_Display);
  if (disp->IsVisible()) {
    TextStyle ts(aPresContext, aRenderingContext, mStyleContext);
    if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing)
      || ts.mJustifying) {
      PaintTextSlowly(aPresContext, aRenderingContext, sc, ts, 0, 0);
    }
    else {
      // Get the text fragment
      nsCOMPtr<nsITextContent> tc = do_QueryInterface(mContent);
      const nsTextFragment* frag = nsnull;
      if (tc.get()) {
        tc->GetText(&frag);
      }
      if (!frag) {
        return NS_ERROR_FAILURE;
      }

      // Choose rendering pathway based on rendering context performance
      // hint, whether it needs to be transformed, and whether it's
      // multi-byte
      PRBool   hasMultiByteChars = (0 != (mState & TEXT_HAS_MULTIBYTE));
      PRUint32 hints = 0;
      aRenderingContext.GetHints(hints);

      // If we have ascii text that doesn't contain multi-byte characters
      // and the text doesn't need transforming then always render as ascii
      if ((0 == (mState & TEXT_WAS_TRANSFORMED)) && !frag->Is2b() && !hasMultiByteChars) {
        PaintAsciiText(aPresContext, aRenderingContext, sc, ts, 0, 0);
      
      } else if (hasMultiByteChars || (0 == (hints & NS_RENDERING_HINT_FAST_8BIT_TEXT))) {
        // If it has multi-byte characters then we have to render it as Unicode
        // regardless of whether the text fragment is 1-byte or 2-byte characters.
        // Or if the text fragment requires transforming then leave it up to the
        // rendering context's preference
        PaintUnicodeText(aPresContext, aRenderingContext, sc, ts, 0, 0);
      }
      else {
        // Use char rendering routine
        PaintAsciiText(aPresContext, aRenderingContext, sc, ts, 0, 0);
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
                                nsAutoIndexBuffer* aIndexBuffer,
                                nsAutoTextBuffer* aTextBuffer,
                                PRInt32* aTextLen)
{
  PRIntn numSpaces = 0;

  // Setup transform to operate starting in the content at our content
  // offset
  aTX.Init(this, mContent, mContentOffset);

  PRInt32 strInx = mContentOffset;
  PRInt32* indexp = aIndexBuffer ? aIndexBuffer->mBuffer : nsnull;

  // Skip over the leading whitespace
  PRInt32 n = mContentLength;
  if (0 != (mState & TEXT_SKIP_LEADING_WS)) {
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;
    aTX.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed);
    // we trip this assertion in bug 31053, but I think it's unnecessary
    //NS_ASSERTION(isWhitespace, "mState and content are out of sync");

    if (isWhitespace) {
      if (nsnull != indexp) {
        // Point mapping indicies at the same content index since
        // all of the compressed whitespace maps down to the same
        // renderable character.
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *indexp++ = strInx;
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
  PRInt32 dstOffset = 0;
  while (0 != n) {
    PRUnichar* bp;
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;

    // Get the next word
    bp = aTX.GetNextWord(inWord, &wordLen, &contentLen, &isWhitespace, &wasTransformed);
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
      if ('\t' == bp[0]) {
        PRInt32 spaces = 8 - (7 & column);
        PRUnichar* tp = bp;
        wordLen = spaces;
        while (--spaces >= 0) {
          *tp++ = ' ';
        }
        // XXX This is a one to many mapping that I think isn't handled well
        if (nsnull != indexp) {
          *indexp++ = strInx;
        	strInx += wordLen;
        }
      }
      else if ('\n' == bp[0]) {
        if (nsnull != indexp) {
          *indexp++ = strInx;
        }
        break;
      }
      else if (nsnull != indexp) {
        if (1 == wordLen) {
          // Point mapping indicies at the same content index since
          // all of the compressed whitespace maps down to the same
          // renderable character.
          PRInt32 i = contentLen;
          while (--i >= 0) {
            *indexp++ = strInx;
          }
          strInx++;
        } else {
          // Point mapping indicies at each content index in the word
          PRInt32 i = contentLen;
          while (--i >= 0) {
            *indexp++ = strInx++;
          }
        }
      }
      numSpaces += wordLen;
    }
    else {
      PRInt32 i;
      if (nsnull != indexp) {
        // Point mapping indicies at each content index in the word
        i = contentLen;
        while (--i >= 0) {
          *indexp++ = strInx++;
        }
      }
      // Nonbreaking spaces count as spaces, not letters
      PRUnichar* tp = bp;
      i = wordLen;
      while (--i >= 0) {
        if (*tp++ == ' ') {
          numSpaces++;
        }
      }
    }

    // Grow the buffer before we run out of room. The only time this
    // happens is because of tab expansion.
    if (aTextBuffer != nsnull && dstOffset + wordLen > aTextBuffer->mBufferLen) {
      nsresult rv = aTextBuffer->GrowBy(wordLen);
      if (NS_FAILED(rv)) {
        break;
      }
    }

    column += wordLen;
    textLength += wordLen;
    n -= contentLen;
    if (aTextBuffer != nsnull) {
      nsCRT::memcpy(aTextBuffer->mBuffer + dstOffset, bp,
                    sizeof(PRUnichar)*wordLen);
    }
    dstOffset += wordLen;
  }

#ifdef DEBUG
  if (aIndexBuffer) {
    NS_ASSERTION(indexp <= aIndexBuffer->mBuffer + aIndexBuffer->mBufferLen,
                 "yikes - we just overwrote memory");
  }
  if (aTextBuffer) {
    NS_ASSERTION(dstOffset <= aTextBuffer->mBufferLen,
                 "yikes - we just overwrote memory");
  }

#endif

  // Remove trailing whitespace if it was trimmed after reflow
  if (TEXT_TRIMMED_WS & mState) {
    NS_ASSERTION(aTextBuffer != nsnull,
      "Nonexistent text buffer should only occur during reflow, i.e. before whitespace is trimmed");
    if (--dstOffset >= 0) {
      PRUnichar ch = aTextBuffer->mBuffer[dstOffset];
      if (XP_IS_SPACE(ch)) {
        textLength--;
        numSpaces--;
      }
    }
  }

  if (aIndexBuffer) {
    PRInt32* ip = aIndexBuffer->mBuffer;
    ip[mContentLength] = ip[mContentLength-1];
    if ((ip[mContentLength] - mContentOffset) < textLength) {
      // Must set up last one for selection beyond edge if in boundary
      ip[mContentLength]++;
    }
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
#if defined(XP_PC) || defined(XP_UNIX) || defined(XP_MAC)
#define USE_INVERT_FOR_SELECTION
#endif

// XXX we should get the following from style sheet or LookAndFeel later
#define IME_RAW_COLOR NS_RGB(198,33,66)
#define IME_CONVERTED_COLOR NS_RGB(255,198,198)

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
    while(aDetails){
      const nscoord* sp= aSpacing;
      PRInt32 startOffset = 0;
      PRInt32 textWidth = 0;
      PRInt32 start = PR_MAX(0,(aDetails->mStart - (PRInt32)aIndex));
      PRInt32 end = PR_MIN((PRInt32)aLength,(aDetails->mEnd - (PRInt32)aIndex));
      PRInt32 i;
      if ((start < end) && ((aLength - start) > 0))
      {
        //aDetails allready processed to have offsets from frame start not content offsets
        if (start < end){
          if (aLength == 1)
            textWidth = aWidth;
          else {
            if (aDetails->mStart > 0){
              if (sp)
              {
                for (i = 0; i < start;i ++){
                  startOffset += *sp ++;
                }
              }
              else
                aRenderingContext.GetWidth(aText, start, startOffset);
            }
            if (sp){
              for (i = start; i < end;i ++){
                textWidth += *sp ++;
              }
            }
            else
              aRenderingContext.GetWidth(aText + start,
                                           PRUint32(end - start), textWidth);
  
          }
          switch (aDetails->mType)
          {
          case nsISelectionController::SELECTION_NORMAL:
#if 0
            {
            //using new selectionpainting now
//
// XOR InvertRect is currently implemented only in the unix and windows
// rendering contexts.  When other platforms implement InvertRect(), they
// can be added here.  Eventually this #ifdef should die.
//
// For platforms that dont implement InvertRect(), the selection will be 
// a non-filled rectangle.
#ifdef USE_INVERT_FOR_SELECTION
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(0,0,0));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif
            }
#endif //0
                                break;
           case nsISelectionController::SELECTION_SPELLCHECK:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(255,0,0));
              aRenderingContext.FillRect(aX + startOffset, aY + baseline - offset, textWidth, size);
                                }break;
          case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:{
#ifdef USE_INVERT_FOR_SELECTION
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(255,255,128));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif        
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(IME_RAW_COLOR);
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size);
                                }break;
          case nsISelectionController::SELECTION_IME_RAWINPUT:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(IME_RAW_COLOR);
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size);
                                }break;
          case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:{
#ifdef USE_INVERT_FOR_SELECTION
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(255,255,128));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif        
			  aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(IME_CONVERTED_COLOR);
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size);
                                }break;
          case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(IME_CONVERTED_COLOR);
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size);
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



nsresult
nsTextFrame::GetContentAndOffsetsForSelection(nsIPresContext *aPresContext, nsIContent **aContent, PRInt32 *aOffset, PRInt32 *aLength)
{
  if (!aContent || !aOffset || !aLength)
    return NS_ERROR_NULL_POINTER;
  //ARE WE GENERATED??
  *aContent = nsnull;
  *aOffset = mContentOffset;
  *aLength = mContentLength;
  nsIFrame *parent;
  nsresult rv = GetParent(&parent);
  if (NS_SUCCEEDED(rv) && parent)
  {
    nsFrameState  parentFrameState;

    parent->GetFrameState(&parentFrameState);
    if ((parentFrameState & NS_FRAME_GENERATED_CONTENT) != 0)//parent is generated so so are we.
    {
      //we COULD check the previous sibling but I dont think that is reliable
      rv = parent->GetContent(aContent);
      if (NS_FAILED(rv) || !*aContent)
        return rv?rv:NS_ERROR_FAILURE;

      //ARE WE A BEFORE FRAME? if not then we assume we are an after frame. this may be bad later
      nsIFrame *grandParent;
      nsIFrame *firstParent;
      rv = parent->GetParent(&grandParent);
      if (NS_SUCCEEDED(rv) && grandParent)
      {
        rv = grandParent->FirstChild(aPresContext,nsnull, &firstParent);
        if (NS_SUCCEEDED(rv) && firstParent)
        {
          *aLength = 0;
          if (firstParent == parent) //then our parent is the first child of granddad. use BEFORE
          {
            *aOffset = 0;
          }
          else
          {
            PRInt32 numChildren;
            if (NS_SUCCEEDED(rv = (*aContent)->ChildCount(numChildren)))
              *aOffset = numChildren;
            else
              return rv;
          }
        }
        else
          return rv;
      }
    }
  }
  //END GENERATED BLOCK 
  if (!*aContent)
  {
    *aContent = mContent;
    NS_IF_ADDREF(*aContent);
  }

  return NS_OK;
}


void
nsTextFrame::PaintUnicodeText(nsIPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsIStyleContext* aStyleContext,
                              TextStyle& aTextStyle,
                              nscoord dx, nscoord dy)
{
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  PRInt8 displaySelection;
  displaySelection = doc->GetDisplaySelection();

  // Make enough space to transform
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }
  nscoord width = mRect.width;

  // Transform text from content into renderable form
  // XXX If the text fragment is already Unicode and text text wasn't
  // transformed when we formatted it, then there's no need to do all
  // this and we should just render the text fragment directly. See
  // PaintAsciiText()...
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 textLength;
  // no need to worry about justification, that's always on the slow path
  PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                     &paintBuffer, &textLength);

  PRInt32* ip = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;
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
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;

      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          PRInt32 offset;
          PRInt32 length;

          rv = GetContentAndOffsetsForSelection(aPresContext,getter_AddRefs(content),&offset,&length);
          if (NS_SUCCEEDED(rv) && content){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details, PR_FALSE);
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
      //while we have substrings...
      PRBool drawn = PR_FALSE;
      DrawSelectionIterator iter(details,text,(PRUint32)textLength,aTextStyle, displaySelection);
      if (!iter.IsDone() && iter.First())
      {
        nscoord currentX = dx;
        nscoord newWidth;//temp
        while (!iter.IsDone())
        {
          PRUnichar *currenttext  = iter.CurrentTextUnicharPtr();
          PRUint32   currentlength= iter.CurrentLength();
          TextStyle &currentStyle = iter.CurrentStyle();
          nscolor    currentFGColor = iter.CurrentForeGroundColor();
          nscolor    currentBKColor;

          if (NS_SUCCEEDED(aRenderingContext.GetWidth(currenttext, currentlength,newWidth)))//ADJUST FOR CHAR SPACING
          {
            if (iter.CurrentBackGroundColor(currentBKColor))
            {//DRAW RECT HERE!!!
              aRenderingContext.SetColor(currentBKColor);
              aRenderingContext.FillRect(currentX, dy, newWidth, mRect.height);
							currentFGColor = EnsureDifferentColors(currentFGColor, currentBKColor);
            }
          }
          else
            newWidth =0;
          

          aRenderingContext.SetColor(currentFGColor);
          aRenderingContext.DrawString(currenttext, currentlength, currentX, dy);

          currentX+=newWidth;//increment twips X start

          iter.Next();
        }
      }
      else
      {
        aRenderingContext.SetColor(aTextStyle.mColor->mColor);
        aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      }
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
}

//measure Spaced Textvoid
nsresult
nsTextFrame::GetPositionSlowly(nsIPresContext* aPresContext,
                               nsIRenderingContext* aRendContext,
                               const nsPoint& aPoint,
                               nsIContent** aNewContent,
                               PRInt32& aOffset)

{
  if (!aRendContext || !aNewContent) {
    return NS_ERROR_NULL_POINTER;
  }

  TextStyle ts(aPresContext, *aRendContext, mStyleContext);
  if (!ts.mSmallCaps && !ts.mWordSpacing && !ts.mLetterSpacing && !ts.mJustifying) {
    return NS_ERROR_INVALID_ARG;
  }
  nsIView * view;
  nsPoint origin;
  GetView(aPresContext, &view);
  GetOffsetFromView(aPresContext, origin, &view);

  if (aPoint.x - origin.x < 0)
  {
      *aNewContent = mContent;
      aOffset =0;
  }
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  // Make enough space to transform
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 textLength;
  PRInt32 numSpaces;

  numSpaces = PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);
  if (textLength <= 0) {
    return NS_ERROR_FAILURE;
  }

  ComputeExtraJustificationSpacing(*aRendContext, ts, paintBuffer.mBuffer, textLength, numSpaces);

//IF STYLE SAYS TO SELECT TO END OF FRAME HERE...
  nsCOMPtr<nsIPref>     prefs;
  PRInt32 prefInt = 0;
  rv = nsServiceManager::GetService(kPrefCID, 
                                    NS_GET_IID(nsIPref), 
                                    (nsISupports**)&prefs); 
  PRBool outofstylehandled = PR_FALSE;
  if (NS_SUCCEEDED(rv) && prefs) 
  { 
    if (NS_SUCCEEDED(prefs->GetIntPref("browser.drag_out_of_frame_style", &prefInt)) && prefInt)
    {
      if (aPoint.y < mRect.y)//above rectangle
      {
        aOffset = mContentOffset;
        outofstylehandled = PR_TRUE;
      }
      else if (aPoint.y > (mRect.y + mRect.height))
      {
        aOffset = mContentOffset + mContentLength;
        outofstylehandled = PR_TRUE;
      }
    }
  }

  if (!outofstylehandled) //then we drag to closest X point and dont worry about the 'Y'
//END STYLE RULE
  {
    //the following will first get the index into the PAINTBUFFER then the actual content
    nscoord adjustedX = PR_MAX(0,aPoint.x-origin.x);

    aOffset = mContentOffset+GetLengthSlowly(*aRendContext, ts,paintBuffer.mBuffer,textLength,adjustedX);
    PRInt32 i;
    for (i = 0;i <= mContentLength; i ++){
      if (indexBuffer.mBuffer[i] >= aOffset){ //reverse mapping
          aOffset = i + mContentOffset;
          break;
      }
    }
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
    (0 != aTextStyle.mWordSpacing) || aTextStyle.mJustifying;
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
      glyphWidth = aTextStyle.mSpaceWidth + aTextStyle.mWordSpacing
        + aTextStyle.mExtraSpacePerSpace;
      if ((PRUint32)--aTextStyle.mNumSpacesToRender <
            (PRUint32)aTextStyle.mNumSpacesReceivingExtraJot) {
        glyphWidth++;
      }
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
        //aRenderingContext.SetColor(aTextStyle.mColor->mColor); commenting out redundat(and destructive) call to setcolor
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
  *aWidthResult = 0;
  GetWidth(rc, aTextStyle, aWord, aWordLength, aWidthResult);
  if (aTextStyle.mLastFont != aTextStyle.mNormalFont) {
    rc.SetFont(aTextStyle.mNormalFont);
    aTextStyle.mLastFont = aTextStyle.mNormalFont;
  }
}


PRInt32
nsTextFrame::GetWidthOrLength(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord* aWidthResult,
                PRBool aGetWidth/* true=get width false = return length up to aWidthResult size*/)
{
  PRUnichar *inBuffer = aBuffer;
  PRInt32 length = aLength;
  nsAutoTextBuffer widthBuffer;
  if (NS_FAILED(widthBuffer.GrowTo(length))) {
    *aWidthResult = 0;
    return 0;
  }
  PRUnichar* bp = widthBuffer.mBuffer;

  nsIFontMetrics* lastFont = aStyle.mLastFont;
  nscoord sum = 0;
  nscoord charWidth;
  while (--length >= 0) {
    nscoord glyphWidth;
    PRUnichar ch = *inBuffer++;
    if (aStyle.mSmallCaps && nsCRT::IsLower(ch)) {
      ch = nsCRT::ToUpper(ch);
      if (lastFont != aStyle.mSmallFont) {
        lastFont = aStyle.mSmallFont;
        aRenderingContext.SetFont(lastFont);
      }
      aRenderingContext.GetWidth(ch, charWidth);
      glyphWidth = charWidth + aStyle.mLetterSpacing;
    }
    else if (ch == ' ') {
      glyphWidth = aStyle.mSpaceWidth + aStyle.mWordSpacing
        + aStyle.mExtraSpacePerSpace;
      if ((PRUint32)--aStyle.mNumSpacesToMeasure
            < (PRUint32)aStyle.mNumSpacesReceivingExtraJot) {
        glyphWidth++;
      }
    }
    else {
      if (lastFont != aStyle.mNormalFont) {
        lastFont = aStyle.mNormalFont;
        aRenderingContext.SetFont(lastFont);
      }
      aRenderingContext.GetWidth(ch, charWidth);
      glyphWidth = charWidth + aStyle.mLetterSpacing;
    }
    sum += glyphWidth;
    *bp++ = ch;
    if (!aGetWidth && sum >= *aWidthResult)
    {
      PRInt32 result = aLength - length;
      if (2*(sum - *aWidthResult) > glyphWidth) //then we have gone too far, back up 1
        result--;
      aStyle.mLastFont = lastFont;
      return result;
    }
  }
  aStyle.mLastFont = lastFont;
  *aWidthResult = sum;
  return aLength;
}


// XXX factor in logic from RenderString into here; gaps, justification, etc.
void
nsTextFrame::GetWidth(nsIRenderingContext& aRenderingContext,
                      TextStyle& aTextStyle,
                      PRUnichar* aBuffer, PRInt32 aLength,
                      nscoord* aWidthResult)
{
  GetWidthOrLength(aRenderingContext,aTextStyle,aBuffer,aLength,aWidthResult,PR_TRUE);
}

PRInt32 
nsTextFrame::GetLengthSlowly(nsIRenderingContext& aRenderingContext,
                TextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength,
                nscoord aWidth)
{
  return GetWidthOrLength(aRenderingContext,aStyle,aBuffer,aLength,&aWidth,PR_FALSE);
}

void
nsTextFrame::ComputeExtraJustificationSpacing(nsIRenderingContext& aRenderingContext,
                                              TextStyle& aTextStyle,
                                              PRUnichar* aBuffer, PRInt32 aLength,
                                              PRInt32 aNumSpaces)
{
  if (aTextStyle.mJustifying) {
    nscoord trueWidth;
    
    // OK, so this is a bit ugly. The problem is that to get the right margin
    // nice and clean, we have to apply a little extra space to *some* of the
    // spaces. It has to be the same ones every time or things will go haywire.
    // This implies that the GetWidthOrLength and RenderString functions depend
    // on a little bit of secret state: which part of the prepared text they are
    // looking at. It turns out that they get called in a regular way: they look
    // at the text from the beginning to the end. So we just count which spaces
    // we're up to, for each context.
    // This is not a great solution, but a perfect solution requires much more
    // widespread changes, to explicitly annotate all the transformed text fragments
    // that are passed around with their position in the transformed text
    // for the entire frame.
    aTextStyle.mNumSpacesToMeasure = 0;
    aTextStyle.mExtraSpacePerSpace = 0;
    aTextStyle.mNumSpacesReceivingExtraJot = 0;
    
    GetWidth(aRenderingContext, aTextStyle, aBuffer, aLength, &trueWidth);
    
    aTextStyle.mNumSpacesToMeasure = aNumSpaces;
    aTextStyle.mNumSpacesToRender = aNumSpaces;
    
    nscoord extraSpace = mRect.width - trueWidth;
    
    if (extraSpace > 0 && aNumSpaces > 0) {
      aTextStyle.mExtraSpacePerSpace = extraSpace/aNumSpaces;
      aTextStyle.mNumSpacesReceivingExtraJot =
        extraSpace - aTextStyle.mExtraSpacePerSpace*aNumSpaces;
    }
  }
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
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
    return;
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 numSpaces;
  
  numSpaces = PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                                 &paintBuffer, &textLength);


  PRInt32* ip = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;
  nsFrameState  frameState;
  PRBool        isSelected;
  GetFrameState(&frameState);
  isSelected = (frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
  if (0 != textLength) {
    ComputeExtraJustificationSpacing(aRenderingContext, aTextStyle, text, textLength, numSpaces);
    if (!displaySelection || !isSelected) { 
      // When there is no selection showing, use the fastest and
      // simplest rendering approach
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      RenderString(aRenderingContext, aStyleContext, aTextStyle,
                   text, textLength, dx, dy, width);
    }
    else {
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;
      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          PRInt32 offset;
          PRInt32 length;

          rv = GetContentAndOffsetsForSelection(aPresContext,getter_AddRefs(content),&offset,&length);
          if (NS_SUCCEEDED(rv)){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details, PR_FALSE);
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

      DrawSelectionIterator iter(details,text,(PRUint32)textLength,aTextStyle, displaySelection);
      if (!iter.IsDone() && iter.First())
      {
	      nscoord currentX = dx;
	      nscoord newWidth;//temp
	      while (!iter.IsDone())
	      {
	      PRUnichar *currenttext  = iter.CurrentTextUnicharPtr();
	      PRUint32   currentlength= iter.CurrentLength();
	      TextStyle &currentStyle = iter.CurrentStyle();
	      nscolor    currentFGColor = iter.CurrentForeGroundColor();
	      nscolor    currentBKColor;
	      GetWidth(aRenderingContext,aTextStyle,currenttext, (PRInt32)currentlength,&newWidth);
	      if (newWidth)
	      {
		      if (iter.CurrentBackGroundColor(currentBKColor))
		      {//DRAW RECT HERE!!!
		      aRenderingContext.SetColor(currentBKColor);
		      aRenderingContext.FillRect(currentX, dy, newWidth, mRect.height);
						      currentFGColor = EnsureDifferentColors(currentFGColor, currentBKColor);
		      }
	      }
	      else
		      newWidth =0;
    
	      aRenderingContext.SetColor(currentFGColor);
	      RenderString(aRenderingContext,aStyleContext, aTextStyle, currenttext, 
					      currentlength, currentX, dy, width, details);
	      //increment twips X start but remember to get ready for next draw by reducing current x by letter spacing amount
	      currentX+=newWidth;// + aTextStyle.mLetterSpacing;

	      iter.Next();
	      }
      }
      else
      {
        aRenderingContext.SetColor(aTextStyle.mColor->mColor);
        RenderString(aRenderingContext,aStyleContext, aTextStyle, text, 
                    PRUint32(textLength), dx, dy, width, details);
      }
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
}

void
nsTextFrame::PaintAsciiText(nsIPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsIStyleContext* aStyleContext,
                            TextStyle& aTextStyle,
                            nscoord dx, nscoord dy)
{
  NS_PRECONDITION(0 == (TEXT_HAS_MULTIBYTE & mState), "text is multi-byte");
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));

  PRBool displaySelection;
  PRBool isSelected;
  displaySelection = doc->GetDisplaySelection();
  isSelected = (mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;

  // Get the text fragment
  nsCOMPtr<nsITextContent> tc = do_QueryInterface(mContent);
  const nsTextFragment* frag = nsnull;
  if (tc.get()) {
    tc->GetText(&frag);
  }
  if (!frag) {
    return;
  }
  
  // Make enough space to transform
  nsAutoTextBuffer unicodePaintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }

  // Construct a text transformer
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);

  // See if we need to transform the text. If the text fragment is ascii and
  // wasn't transformed, then we can skip this step. If we're displaying the
  // selection and the text is selected, then we need to do this step so we
  // can create the index buffer
  PRInt32     textLength;
  const char* text;
  char        paintBufMem[TEXT_BUF_SIZE];
  char*       paintBuf = paintBufMem;
  if (frag->Is2b() ||
      (0 != (mState & TEXT_WAS_TRANSFORMED)) ||
      (displaySelection && isSelected)) {
    
    // Transform text from content into Unicode renderable form
    // XXX If the text fragment is ascii, then we should ask the
    // text transformer to leave the text in ascii. That way we can
    // elimninate the conversion from Unicode back to ascii...
    PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                       &unicodePaintBuffer, &textLength);


    // Translate unicode data into ascii for rendering
    if (textLength > TEXT_BUF_SIZE) {
      paintBuf = new char[textLength];
      if (!paintBuf) {
        return;
      }
    }
    char* dst = paintBuf;
    char* end = dst + textLength;
    PRUnichar* src = unicodePaintBuffer.mBuffer;
    while (dst < end) {
      *dst++ = (char) ((unsigned char) *src++);
    }

    text = paintBuf;

  } else {
    text = frag->Get1b() + mContentOffset;
    textLength = mContentLength;

    // See if we should skip leading whitespace
    if (0 != (mState & TEXT_SKIP_LEADING_WS)) {
      while ((textLength > 0) && XP_IS_SPACE(*text)) {
        text++;
        textLength--;
      }
    }

    // See if the text ends in a newline
    if ((textLength > 0) && (text[textLength - 1] == '\n')) {
      textLength--;
    }
    NS_ASSERTION(textLength >= 0, "bad text length");
  }

  nscoord width = mRect.width;
  PRInt32* ip = indexBuffer.mBuffer;

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
      nsCOMPtr<nsIPresShell> shell;
      nsCOMPtr<nsIFrameSelection> frameSelection;
      nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell){
        rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
        if (NS_SUCCEEDED(rv) && frameSelection){
          nsCOMPtr<nsIContent> content;
          PRInt32 offset;
          PRInt32 length;

          rv = GetContentAndOffsetsForSelection(aPresContext, getter_AddRefs(content),&offset,&length);
          if (NS_SUCCEEDED(rv)){
            rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                  mContentLength , &details, PR_FALSE);
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
      DrawSelectionIterator iter(details,(PRUnichar *)text,(PRUint32)textLength,aTextStyle, displaySelection);//ITS OK TO CAST HERE THE RESULT WE USE WILLNOT DO BAD CONVERSION
      if (!iter.IsDone() && iter.First())
      {
        nscoord currentX = dx;
        nscoord newWidth;//temp
        while (!iter.IsDone())
        {
          char *currenttext  = iter.CurrentTextCStrPtr();
          PRUint32   currentlength= iter.CurrentLength();
          TextStyle &currentStyle = iter.CurrentStyle();
          nscolor    currentFGColor = iter.CurrentForeGroundColor();
          nscolor    currentBKColor;

          if (NS_SUCCEEDED(aRenderingContext.GetWidth(currenttext, currentlength,newWidth)))//ADJUST FOR CHAR SPACING
          {
            if (iter.CurrentBackGroundColor(currentBKColor))
            {//DRAW RECT HERE!!!
              aRenderingContext.SetColor(currentBKColor);
              aRenderingContext.FillRect(currentX, dy, newWidth, mRect.height);
							currentFGColor = EnsureDifferentColors(currentFGColor, currentBKColor);
            }
          }
          else
            newWidth =0;
          
          aRenderingContext.SetColor(currentFGColor);
          aRenderingContext.DrawString(currenttext, currentlength, currentX, dy);

          currentX+=newWidth;//increment twips X start

          iter.Next();
        }
      }
      else
      {
        aRenderingContext.SetColor(aTextStyle.mColor->mColor);
        aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
      }
      PaintTextDecorations(aRenderingContext, aStyleContext,
                           aTextStyle, dx, dy, width,
                           unicodePaintBuffer.mBuffer,
                           details, 0, textLength);
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
}

NS_IMETHODIMP
nsTextFrame::FindTextRuns(nsLineLayout& aLineLayout)
{
  nsIFrame* prevInFlow;
  GetPrevInFlow(&prevInFlow);
  if (nsnull == prevInFlow) {
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
nsTextFrame::GetPosition(nsIPresContext* aCX,
                         const nsPoint&  aPoint,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd)

{
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aCX->GetShell(getter_AddRefs(shell));
  if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIRenderingContext> acx;      
    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    if (NS_SUCCEEDED(rv)) {
      TextStyle ts(aCX, *acx, mStyleContext);
      if (ts.mSmallCaps || ts.mWordSpacing || ts.mLetterSpacing || ts.mJustifying) {
        nsresult result = GetPositionSlowly(aCX, acx, aPoint, aNewContent,
                                 aContentOffset);
        aContentOffsetEnd = aContentOffset;
        return result;
      }

      // Make enough space to transform
      nsAutoTextBuffer paintBuffer;
      nsAutoIndexBuffer indexBuffer;
      rv = indexBuffer.GrowTo(mContentLength + 1);
      if (NS_FAILED(rv)) {
        return rv;
      }

      // Find the font metrics for this text
      nsIStyleContext* styleContext;
      GetStyleContext(&styleContext);
      const nsStyleFont *font = (const nsStyleFont*)
        styleContext->GetStyleData(eStyleStruct_Font);
      NS_RELEASE(styleContext);
      nsCOMPtr<nsIFontMetrics> fm;
      aCX->GetMetricsFor(font->mFont, getter_AddRefs(fm));
      acx->SetFont(fm);

      // Get the renderable form of the text
      nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aCX)));
      nsCOMPtr<nsILineBreaker> lb;
      doc->GetLineBreaker(getter_AddRefs(lb));
      nsTextTransformer tx(lb, nsnull);
      PRInt32 textLength;
      // no need to worry about justification, that's always on the slow path
      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

      if (textLength <=0) {
        //invalid frame to get position on
        return NS_ERROR_FAILURE;
      }

      nsPoint origin;
      nsIView * view;
      GetOffsetFromView(aCX, origin, &view);

//IF STYLE SAYS TO SELECT TO END OF FRAME HERE...
      nsCOMPtr<nsIPref>     prefs;
      PRInt32 prefInt = 0;
      rv = nsServiceManager::GetService(kPrefCID, 
                                        NS_GET_IID(nsIPref), 
                                        (nsISupports**)&prefs); 
      PRBool outofstylehandled = PR_FALSE;
      if (NS_SUCCEEDED(rv) && prefs) 
      { 
        if (NS_SUCCEEDED(prefs->GetIntPref("browser.drag_out_of_frame_style", &prefInt)) && prefInt)
        {
          if ((aPoint.y - origin.y) < 0)//above rectangle
          {
            aContentOffset = mContentOffset;
            aContentOffsetEnd = aContentOffset;
            outofstylehandled = PR_TRUE;
          }
          else if ((aPoint.y - origin.y) > mRect.height)
          {
            aContentOffset = mContentOffset + mContentLength;
            aContentOffsetEnd = aContentOffset;
            outofstylehandled = PR_TRUE;
          }
        }
      }

      if (!outofstylehandled) //then we need to track based on the X coord only
      {
//END STYLE IF
        PRInt32* ip = indexBuffer.mBuffer;

        PRInt32 indx;
        PRInt32 textWidth = 0;
        PRUnichar* text = paintBuffer.mBuffer;
        PRBool found = BinarySearchForPosition(acx, text, origin.x, 0, 0,
                                               PRInt32(textLength),
                                               PRInt32(aPoint.x) , //go to local coordinates
                                               indx, textWidth);
        if (found) {
          PRInt32 charWidth;
          acx->GetWidth(text[indx], charWidth);
          charWidth /= 2;

          if ((aPoint.x - origin.x) > textWidth+charWidth) {
            indx++;
          }
        }

        aContentOffset = indx + mContentOffset;
        //reusing wordBufMem
        PRInt32 i;
        for (i = 0;i <= mContentLength; i ++){
          if (ip[i] >= aContentOffset){ //reverse mapping
              aContentOffset = i + mContentOffset;
              break;
          }
        }
        aContentOffsetEnd = aContentOffset;
        NS_ASSERTION(i<= mContentLength, "offset we got from binary search is messed up");
      }      
      *aNewContent = mContent;
      if (*aNewContent) {
        (*aNewContent)->AddRef();
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::GetContentAndOffsetsFromPoint(nsIPresContext* aCX,
                                           const nsPoint&  aPoint,
                                           nsIContent **   aNewContent,
                                           PRInt32&        aContentOffset,
                                           PRInt32&        aContentOffsetEnd,
                                           PRBool&         aBeginFrameContent)
{
  nsPoint newPoint;
  newPoint.y = aPoint.y;
  if (aPoint.x < 0)
    newPoint.x = 0;
  else
    newPoint.x = aPoint.x;

  nsresult rv = GetPosition(aCX, newPoint, aNewContent, aContentOffset, aContentOffsetEnd);
  if (aContentOffset == mContentOffset)
    aBeginFrameContent = PR_TRUE;
  else
    aBeginFrameContent = PR_FALSE;
  return rv;
}


// [HACK] Foward Declarations
void ForceDrawFrame(nsFrame * aFrame);

//null range means the whole thing
NS_IMETHODIMP
nsTextFrame::SetSelected(nsIPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread)
{
  nsresult result;
  if (aSelected && ParentDisablesSelection())
    return NS_OK;

  nsFrameState  frameState;
  GetFrameState(&frameState);
#if 0
  PRBool isSelected = ((frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);
  if (!aSelected && !isSelected) //already set thanks
  {
    return NS_OK;
  }
#endif

  // check whether style allows selection
	const nsStyleUserInterface* userinterface;
	GetStyleData(eStyleStruct_UserInterface, (const nsStyleStruct*&)userinterface);
	if (userinterface) {
		if (userinterface->mUserSelect == NS_STYLE_USER_SELECT_AUTO) {
				// if 'user-select' isn't set for this frame, use the parent's
				if (mParent) {
					mParent->GetStyleData(eStyleStruct_UserInterface, (const nsStyleStruct*&)userinterface);
				}
		}
		if (userinterface->mUserSelect == NS_STYLE_USER_SELECT_NONE) {
		  return NS_OK;//do not continue no selection for this frame.
		}
	}

  PRBool found = PR_FALSE;
  PRBool wholeContentFound = PR_FALSE;//if the entire content we look at is selected.
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

    if (thisNode == startNode)
    {
      if ((mContentOffset + mContentLength) >= startOffset)
      {
        found = PR_TRUE;
        if (thisNode == endNode)
        { //special case
          if (endOffset == startOffset) //no need to redraw since drawing takes place with cursor
            found = PR_FALSE;

          if (mContentOffset > endOffset)
            found = PR_FALSE;
        }
      }
    }
    else if (thisNode == endNode)
    {
      if (mContentOffset < endOffset)
        found = PR_TRUE;
      else
      {
        found = PR_FALSE;
        wholeContentFound = PR_TRUE;
      }
    }
    else
    {
      found = PR_TRUE;
    }
  }
  else {
    if ( aSelected != (PRBool)(frameState | NS_FRAME_SELECTED_CONTENT) ){
      found = PR_TRUE;
    }
  }

  if ( aSelected )
    frameState |=  NS_FRAME_SELECTED_CONTENT;
  else
  {//we need to see if any other selection available.
    SelectionDetails *details = nsnull;
    nsCOMPtr<nsIPresShell> shell;
    nsCOMPtr<nsIFrameSelection> frameSelection;

    nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
    if (NS_SUCCEEDED(rv) && shell){
      rv = shell->GetFrameSelection(getter_AddRefs(frameSelection));
      if (NS_SUCCEEDED(rv) && frameSelection){
        nsCOMPtr<nsIContent> content;
        PRInt32 offset;
        PRInt32 length;

        rv = GetContentAndOffsetsForSelection(aPresContext, getter_AddRefs(content),&offset,&length);
        if (NS_SUCCEEDED(rv) && content){
          rv = frameSelection->LookUpSelection(content, offset,
                                length , &details, PR_TRUE);
// PR_TRUE last param used here! we need to see if we are still selected. so no shortcut

        }
      }
    }
    if (!details)
      frameState &= ~NS_FRAME_SELECTED_CONTENT;
    else
    {
      SelectionDetails *sdptr = details;
      while ((sdptr = details->mNext) != nsnull) {
        delete details;
        details = sdptr;
      }
      delete details;
    }
  }
  SetFrameState(frameState);
  if (found){ //if range contains this frame...
    nsRect frameRect;
    GetRect(frameRect);
    nsRect rect(0, 0, frameRect.width, frameRect.height);
    Invalidate(aPresContext, rect, PR_FALSE);
//    ForceDrawFrame(this);
  }
  if (aSpread == eSpreadDown)
  {
    nsIFrame *frame;
    GetPrevInFlow(&frame);
    while(frame){
      frame->SetSelected(aPresContext, aRange,aSelected,eSpreadNone);
      result = frame->GetPrevInFlow(&frame);
      if (NS_FAILED(result))
        break;
    }
    GetNextInFlow(&frame);
    while (frame){
      frame->SetSelected(aPresContext, aRange,aSelected,eSpreadNone);
      result = frame->GetNextInFlow(&frame);
      if (NS_FAILED(result))
        break;
    }
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

  // Make enough space to transform
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    return rv;
  }

  // Transform text from content into renderable form
  nsCOMPtr<nsIDocument> doc(getter_AddRefs(GetDocument(aPresContext)));
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 textLength;
  PRInt32 numSpaces;

  numSpaces = PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

  ComputeExtraJustificationSpacing(*inRendContext, ts, paintBuffer.mBuffer, textLength, numSpaces);


  PRInt32* ip = indexBuffer.mBuffer;
  if (inOffset > mContentLength){
    NS_ASSERTION(0, "invalid offset passed to GetPointFromOffset");
    inOffset = mContentLength;
  }

  nscoord width = mRect.width;
  if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing) || ts.mJustifying)
  {
    GetWidth(*inRendContext, ts,
             paintBuffer.mBuffer, ip[inOffset]-mContentOffset,
             &width);
  }
  else
  {
    inRendContext->GetWidth(paintBuffer.mBuffer, ip[inOffset]-mContentOffset,width);
  }
  if (inOffset > textLength && (TEXT_TRIMMED_WS & mState)){
    //
    // Offset must be after a space that has
    // been trimmed off the end of the frame.
    // Add the width of the trimmed space back
    // to the total width, so the caret appears
    // in the proper place!
    //
    // NOTE: the trailing whitespace includes the word spacing!!
    width += ts.mSpaceWidth + ts.mWordSpacing;
  }

  outPoint->x = width;
  outPoint->y = 0;

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
    GetNextInFlow(&nextInFlow);
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
nsTextFrame::PeekOffset(nsIPresContext* aPresContext, nsPeekOffsetStruct *aPos) 
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
    GetNextInFlow(&nextInFlow);
    if (!nextInFlow){
      NS_ASSERTION(PR_FALSE,"nsTextFrame::PeekOffset no more flow \n");
      return NS_ERROR_INVALID_ARG;
    }
    return nextInFlow->PeekOffset(aPresContext, aPos);
  }
 
  if (aPos->mAmount == eSelectLine || aPos->mAmount == eSelectBeginLine 
      || aPos->mAmount == eSelectEndLine)
  {
      return nsFrame::PeekOffset(aPresContext, aPos);
  }

  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    return rv;
  }
  PRInt32* ip = indexBuffer.mBuffer;

  PRInt32 textLength;
  nsresult result(NS_ERROR_FAILURE);
  aPos->mResultContent = mContent;//do this right off
  switch (aPos->mAmount){
    case eSelectNoAmount:
    {
      // Transform text from content into renderable form
      nsIDocument* doc;
      result = mContent->GetDocument(doc);
      if (NS_FAILED(result) || !doc) {
        return result;
      }
      nsCOMPtr<nsILineBreaker> lb;
      doc->GetLineBreaker(getter_AddRefs(lb));
      NS_RELEASE(doc);

      nsTextTransformer tx(lb, nsnull);
      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

      if (textLength)//if no renderable length, you cant park here.
      {
        aPos->mContentOffset = aPos->mStartOffset;
        result = NS_OK;
      }
      else
      {
        aPos->mAmount = eSelectDir;//go to "next" or previous frame based on direction not THIS frame
        result = GetFrameFromDirection(aPresContext, aPos);
        if (NS_SUCCEEDED(result) && aPos->mResultFrame && aPos->mResultFrame!= this)
          return aPos->mResultFrame->PeekOffset(aPresContext, aPos);
      }
    }
    break;

    case eSelectCharacter:
    {
      // Transform text from content into renderable form
      nsIDocument* doc;
      result = mContent->GetDocument(doc);
      if (NS_FAILED(result) || !doc) {
        return result;
      }
      nsCOMPtr<nsILineBreaker> lb;
      doc->GetLineBreaker(getter_AddRefs(lb));
      NS_RELEASE(doc);

      nsTextTransformer tx(lb, nsnull);
      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

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
          GetPrevInFlow(&frameUsed);
          start = mContentOffset;
          aPos->mContentOffset = start;//in case next call fails we stop at this offset
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
          GetNextInFlow(&frameUsed);
          start = mContentOffset + mContentLength;
          aPos->mContentOffset = start;//in case next call fails we stop at this offset
        }
      }

      if (!found)
      {
        result = GetFrameFromDirection(aPresContext, aPos);
        if (NS_SUCCEEDED(result) && aPos->mResultFrame && aPos->mResultFrame!= this)
          result = aPos->mResultFrame->PeekOffset(aPresContext, aPos);
      }
      else 
        aPos->mResultContent = mContent;
    }
    break;

    case eSelectWord:
    {
      // Transform text from content into renderable form
      nsIDocument* doc;
      result = mContent->GetDocument(doc);
      if (NS_FAILED(result) || !doc) {
        return result;
      }

      nsCOMPtr<nsILineBreaker> lb;
      doc->GetLineBreaker(getter_AddRefs(lb));
      nsCOMPtr<nsIWordBreaker> wb;
      doc->GetWordBreaker(getter_AddRefs(wb));
      NS_RELEASE(doc);

      nsTextTransformer tx(lb, wb);

      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);
      nsIFrame *frameUsed = nsnull;
      PRBool keepSearching; //if you run out of chars before you hit the end of word, maybe next frame has more text to select?
      PRInt32 start;
      PRBool found = PR_FALSE;
      PRBool isWhitespace, wasTransformed;
      PRInt32 wordLen, contentLen;
      if (aPos->mDirection == eDirPrevious){
        keepSearching = PR_TRUE;
        tx.Init(this, mContent, aPos->mStartOffset);
        aPos->mContentOffset = mContentOffset;//initialize
        if (tx.GetPrevWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace,
                           PR_FALSE) &&
          (aPos->mStartOffset - contentLen >= mContentOffset) ){
          if ((aPos->mEatingWS && !isWhitespace) || !aPos->mEatingWS){
            aPos->mContentOffset = aPos->mStartOffset - contentLen;
            //check for whitespace next.
            if (isWhitespace && aPos->mContentOffset <= mContentOffset)
            {
              keepSearching = PR_FALSE;//reached the beginning of a word
              aPos->mEatingWS = PR_FALSE;//if no real word then
            }
            else{
              while (isWhitespace &&
                     tx.GetPrevWord(PR_FALSE, &wordLen, &contentLen,
                                    &isWhitespace, PR_FALSE)){
                aPos->mContentOffset -= contentLen;
                aPos->mEatingWS = PR_TRUE;
              }
              aPos->mEatingWS = !isWhitespace;//nowhite space, just eat chars.
              keepSearching = aPos->mContentOffset <= mContentOffset;
              if (!isWhitespace){
                if (!keepSearching)
                  found = PR_TRUE;
              else
                aPos->mEatingWS = PR_TRUE;
              }
            }
          }
          else {
            aPos->mContentOffset = mContentLength + mContentOffset;
            found = PR_TRUE;
          }
        }
      }
      else if (aPos->mDirection == eDirNext) {
        tx.Init(this, mContent, aPos->mStartOffset );
        aPos->mContentOffset = mContentOffset + mContentLength;//initialize

#ifdef DEBUGWORDJUMP
        printf("Next- Start=%d aPos->mEatingWS=%s\n", aPos->mStartOffset, aPos->mEatingWS ? "TRUE" : "FALSE");
#endif

        if (tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed, PR_TRUE, PR_FALSE) &&
          (aPos->mStartOffset + contentLen <= (mContentLength + mContentOffset))){

#ifdef DEBUGWORDJUMP
          printf("GetNextWord return non null, wordLen%d, contentLen%d isWhitespace=%s\n", 
                 wordLen, contentLen, isWhitespace ? "WS" : "NOT WS");
#endif
          if ((aPos->mEatingWS && isWhitespace) || !aPos->mEatingWS){
            aPos->mContentOffset = aPos->mStartOffset + contentLen;
            //check for whitespace next.
            keepSearching = PR_TRUE;
            aPos->mEatingWS = PR_TRUE;
            while (!isWhitespace &&
                   tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed, PR_TRUE, PR_FALSE)){
#ifdef DEBUGWORDJUMP
              printf("2-GetNextWord return non null, wordLen%d, contentLen%d isWhitespace=%s\n", 
                     wordLen, contentLen, isWhitespace ? "WS" : "NOT WS");
#endif
              aPos->mContentOffset += contentLen;
              keepSearching = PR_FALSE;
            }
          }
          else if (aPos->mEatingWS)
            aPos->mContentOffset = mContentOffset;
          found = isWhitespace;
          if (!isWhitespace){
            aPos->mEatingWS = PR_FALSE;
          }
          else if (!keepSearching) //we have found the "whole" word so just looking for WS
            aPos->mEatingWS = PR_TRUE;
        } 
        GetNextInFlow(&frameUsed);
        start = 0;
      }
#ifdef DEBUGWORDJUMP
      printf("aEatingWS = %s\n" , aPos->mEatingWS ? "TRUE" : "FALSE");
#endif
      if (!found || (aPos->mContentOffset > (mContentOffset + mContentLength)) || (aPos->mContentOffset < mContentOffset))
      {
        aPos->mContentOffset = PR_MIN(aPos->mContentOffset, mContentOffset + mContentLength);
        aPos->mContentOffset = PR_MAX(aPos->mContentOffset, mContentOffset);
        result = GetFrameFromDirection(aPresContext, aPos);
        if (NS_SUCCEEDED(result) && aPos->mResultFrame && aPos->mResultFrame!= this)
        {
          if (NS_SUCCEEDED(result = aPos->mResultFrame->PeekOffset(aPresContext, aPos)))
            return NS_OK;//else fall through
          else if (aPos->mDirection == eDirNext)
            aPos->mContentOffset = mContentOffset + mContentLength;
          else
            aPos->mContentOffset = mContentOffset;
        }
        else 
          aPos->mResultContent = mContent;
      }
      else 
      {
        aPos->mResultContent = mContent;
      }
    }
    break;
    default:
      result = NS_ERROR_FAILURE; break;
  }

  aPos->mContentOffsetEnd = aPos->mContentOffset;

  if (NS_FAILED(result)){
    aPos->mResultContent = mContent;
    //aPos->mContentOffset = aPos->mStartOffset;
    result = NS_OK;
  }
  aPos->mResultFrame = this;

  return result;
}

NS_IMETHODIMP
nsTextFrame::HandleMultiplePress(nsIPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  if (!DisplaySelection(aPresContext)) {
    return NS_OK;
  }
  
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = aPresContext->GetShell(getter_AddRefs(shell));
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(shell);
  if (NS_FAILED(rv) || !shell || !selCon)
    return rv ?rv:NS_ERROR_FAILURE;
  if (me->clickCount > 2)//triple clicking
  {
    nsCOMPtr<nsIPref>     mPrefs;
    PRInt32 prefInt = 0;
    rv = nsServiceManager::GetService(kPrefCID, 
                                               NS_GET_IID(nsIPref), 
                                               (nsISupports**)&mPrefs); 

    if (NS_SUCCEEDED(rv) && mPrefs) 
    { 
      if (NS_FAILED(mPrefs->GetIntPref("browser.triple_click_style", &prefInt)) || !prefInt)
        return nsFrame::HandleMultiplePress(aPresContext, aEvent, aEventStatus);
    }
    //THIS NEXT CODE IS FOR PARAGRAPH
    nsCOMPtr<nsIDOMNode> startNode;
    nsCOMPtr<nsIDOMNode> endNode;

    nsIFrame *currentFrame = this;
    nsIFrame *prevFrame;
     
    GetPrevInFlow(&prevFrame);
    while(prevFrame){
      currentFrame = prevFrame;
      currentFrame->GetPrevInFlow(&prevFrame);
      if (NS_FAILED(rv))
        break;
    }
    prevFrame = currentFrame;
    currentFrame = this;
    nsIFrame *nextFrame;
    GetNextInFlow(&nextFrame);

    while (nextFrame){
      currentFrame = nextFrame;
      currentFrame->GetNextInFlow(&nextFrame);
      if (NS_FAILED(rv))
        break;
    }
    nextFrame = currentFrame;
    nsCOMPtr<nsIContent> content;
    if (prevFrame)
      prevFrame->GetContent(getter_AddRefs(content));

    startNode = do_QueryInterface(content,&rv);
    if (NS_FAILED(rv) || !startNode)
      return rv?rv:NS_ERROR_FAILURE;

    nextFrame->GetContent(getter_AddRefs(content));

    endNode = do_QueryInterface(content,&rv);
    if (NS_FAILED(rv) || !endNode)
      return rv?rv:NS_ERROR_FAILURE;
    
    PRInt32 startOffset;
    PRInt32 endOffset;
    PRInt32 unusedOffset;

    rv = prevFrame->GetOffsets(startOffset,unusedOffset);
    if (NS_FAILED(rv))
      return rv;

    rv = nextFrame->GetOffsets(unusedOffset,endOffset);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIDOMSelection> selection;
    if (NS_SUCCEEDED(selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection)))){
      rv = selection->Collapse(startNode, startOffset);
      if (NS_FAILED(rv))
        return rv;
      rv = selection->Extend(endNode, endOffset);
      if (NS_FAILED(rv))
        return rv;
    }
  }
  else if (NS_SUCCEEDED(rv) && shell) {
    nsCOMPtr<nsIRenderingContext> acx;      
    nsCOMPtr<nsIFocusTracker> tracker;
    tracker = do_QueryInterface(shell, &rv);
    if (NS_FAILED(rv) || !tracker)
      return rv?rv:NS_ERROR_FAILURE;

    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    if (NS_SUCCEEDED(rv)){
      PRInt32 startPos = 0;
      PRInt32 contentOffsetEnd = 0;
      nsCOMPtr<nsIContent> newContent;
      //find which word needs to be selected! use peek offset one way then the other
      nsCOMPtr<nsIDOMNode> startNode;
      nsCOMPtr<nsIDOMNode> endNode;
      if (NS_SUCCEEDED(GetPosition(aPresContext, aEvent->point,
                       getter_AddRefs(newContent), startPos, contentOffsetEnd))){
        //peeks{}
        nsPeekOffsetStruct startpos;
        startpos.SetData(tracker, 
                        0, 
                        eSelectWord,
                        eDirPrevious,
                        startPos,
                        PR_FALSE,
                        PR_TRUE,
                        PR_FALSE);
        rv = PeekOffset(aPresContext, &startpos);
        if (NS_FAILED(rv))
          return rv;
        nsPeekOffsetStruct endpos;
        endpos.SetData(tracker, 
                        0, 
                        eSelectWord,
                        eDirNext,
                        startPos,
                        PR_FALSE,
                        PR_FALSE,
                        PR_FALSE);
        rv = PeekOffset(aPresContext, &endpos);
        if (NS_FAILED(rv))
          return rv;

        endNode = do_QueryInterface(endpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;
        startNode = do_QueryInterface(startpos.mResultContent,&rv);
        if (NS_FAILED(rv))
          return rv;

        nsCOMPtr<nsIDOMSelection> selection;
        if (NS_SUCCEEDED(selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection)))){
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
  
#define TEXT_MAX_NUM_SEGMENTS 65

struct SegmentData {
  PRUint32  mIsWhitespace : 1;
  PRUint32  mContentLen : 31;  // content length

  PRBool  IsWhitespace() {return PRBool(mIsWhitespace);}

  // Get the content length. This is a running total of all
  // the previous segments as well
  PRInt32 ContentLen() {return PRInt32(mContentLen);}
};

struct TextRun {
  // Total number of characters and the accumulated content length
  PRInt32       mTotalNumChars, mTotalContentLen;

  // Words and whitespace each count as a segment
  PRInt32       mNumSegments;

  // Possible break points specified as offsets into the buffer
  PRInt32       mBreaks[TEXT_MAX_NUM_SEGMENTS];

  // Per segment data
  SegmentData   mSegments[TEXT_MAX_NUM_SEGMENTS];

  TextRun()
  {
    Reset();
  }
  
  void Reset()
  {
    mNumSegments = 0;
    mTotalNumChars = 0;
    mTotalContentLen = 0;
  }

  // Returns PR_TRUE if we're currently buffering text
  PRBool IsBuffering()
  {
    return mNumSegments > 0;
  }

  void AddSegment(PRInt32 aNumChars, PRInt32 aContentLen, PRBool aIsWhitespace)
  {
    NS_PRECONDITION(mNumSegments < TEXT_MAX_NUM_SEGMENTS, "segment overflow");

    mTotalNumChars += aNumChars;
    mBreaks[mNumSegments] = mTotalNumChars;
    mSegments[mNumSegments].mIsWhitespace = aIsWhitespace;
    mTotalContentLen += aContentLen;
    mSegments[mNumSegments].mContentLen = PRUint32(mTotalContentLen);
    mNumSegments++;
  }
};

// Transforms characters in place from ascii to Unicode
static void
TransformTextToUnicode(char* aText, PRInt32 aNumChars)
{
  // Go backwards over the characters and convert them.
  unsigned char*  cp1 = (unsigned char*)aText + aNumChars - 1;
  PRUnichar*      cp2 = (PRUnichar*)aText + (aNumChars - 1);
  
  while (aNumChars-- > 0) {
    *cp2-- = PRUnichar(*cp1--);
  }
}
  
nsReflowStatus
nsTextFrame::MeasureText(nsIPresContext*          aPresContext,
                         const nsHTMLReflowState& aReflowState,
                         nsTextTransformer&       aTx,
                         nsILineBreaker*          aLb,
                         TextStyle&               aTs,
                         TextReflowData&          aTextData)
{
  PRBool firstThing = PR_TRUE;
  nscoord maxWidth = aReflowState.availableWidth;
  nsLineLayout& lineLayout = *aReflowState.mLineLayout;
  PRInt32 contentLength = aTx.GetContentLength();
  PRInt32 startingOffset = aTextData.mOffset;
  PRInt32 prevOffset = -1;
  PRInt32 column = mColumn;
  PRInt32 prevColumn = column;
  nscoord prevMaxWordWidth = 0;
  PRInt32 lastWordLen = 0;
  PRInt32 lastWordWidth = 0;
  PRUnichar* lastWordPtr = nsnull;
  PRBool  textStartsWithNBSP = PR_FALSE;
  PRBool  endsInWhitespace = PR_FALSE;
  PRBool  endsInNewline = PR_FALSE;
  PRBool  justDidFirstLetter = PR_FALSE;
#ifdef _WIN32
  PRBool  measureTextRuns = !aTextData.mComputeMaxWordWidth && !aTs.mPreformatted &&
                            !aTs.mSmallCaps && !aTs.mWordSpacing && !aTs.mLetterSpacing;
  // Don't measure text runs with letter spacing active, it doesn't work
#else
  PRBool  measureTextRuns = PR_FALSE;
#endif
  TextRun textRun;
  PRInt32 estimatedNumChars;
   
  // Estimate the number of characters that will fit. Use 105% of the available
  // width divided by the average character width
  estimatedNumChars = (maxWidth - aTextData.mX) / aTs.mAveCharWidth;
  estimatedNumChars += estimatedNumChars / 20;

  aTextData.mX = 0;
  for (;;firstThing = PR_FALSE) {
    // Get next word/whitespace from the text
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;
    union {
      char*       bp1;
      PRUnichar*  bp2;
    };
    bp2 = aTx.GetNextWord(aTextData.mInWord, &wordLen, &contentLen, &isWhitespace,
                          &wasTransformed, textRun.mNumSegments == 0);
    // Remember if the text was transformed
    if (wasTransformed) {
      mState |= TEXT_WAS_TRANSFORMED;
    }
    if (nsnull == bp2) {
      if (textRun.IsBuffering()) {
        // Measure the remaining text
        goto MeasureTextRun;
      }
      else {
        // Advance the offset in case we just consumed a bunch of
        // discarded characters. Otherwise, if this is the first piece
        // of content for this frame we will attempt to break-before it.
        aTextData.mOffset += contentLen;
        break;
      }
    }
    lastWordLen = wordLen;
    lastWordPtr = bp2;
    aTextData.mInWord = PR_FALSE;

    // Measure the word/whitespace
    nscoord width;
    PRUnichar firstChar;
    if (aTx.TransformedTextIsAscii()) {
      firstChar = *bp1;
    } else {
      firstChar = *bp2;
    }
    if (isWhitespace) {
      if ('\n' == firstChar) {
        // We hit a newline. Stop looping.
        NS_WARN_IF_FALSE(aTs.mPreformatted, "newline w/o ts.mPreformatted");
        prevOffset = aTextData.mOffset;
        aTextData.mOffset++;
        endsInWhitespace = PR_TRUE;
        endsInNewline = PR_TRUE;
        break;
      }
      if (aTextData.mSkipWhitespace) {
        aTextData.mOffset += contentLen;
        aTextData.mSkipWhitespace = PR_FALSE;

        if (wasTransformed) {
          // As long as there were no discarded characters, then don't consider
          // skipped leading whitespace as being transformed
          if (wordLen == contentLen) {
            mState &= ~TEXT_WAS_TRANSFORMED;
          }
        }

        // Only set flag when we actually do skip whitespace
        mState |= TEXT_SKIP_LEADING_WS;
        continue;
      }

      // NOTE: Even if the textRun absorbs the whitespace below, we still
      // want to remember that we're breakable.
      aTextData.mIsBreakable = PR_TRUE;
      aTextData.mFirstLetterOK = PR_FALSE;
 
      if ('\t' == firstChar) {
        // Expand tabs to the proper width
        wordLen = 8 - (7 & column);
        // Apply word spacing to every space derived from a tab
        width = (aTs.mSpaceWidth + aTs.mWordSpacing)*wordLen;

        // Because we have to expand the tab when rendering consider that
        // a transformation of the text
        mState |= TEXT_WAS_TRANSFORMED;
      }
      else if (textRun.IsBuffering()) {
        // Add a whitespace segment
        textRun.AddSegment(wordLen, contentLen, PR_TRUE);
        continue;
      }
      else {
        // Apply word spacing to every space, if there's more than one
        width = wordLen*(aTs.mWordSpacing + aTs.mSpaceWidth);// XXX simplistic
      }

      if (aTextData.mMeasureText) {
        // See if there is room for the text
        if ((0 != aTextData.mX) && aTextData.mWrapping && (aTextData.mX + width > maxWidth)) {
          // The text will not fit.
          break;
        }
        aTextData.mX += width;
      }
      prevColumn = column;
      column += wordLen;
      endsInWhitespace = PR_TRUE;
      prevOffset = aTextData.mOffset;
      aTextData.mOffset += contentLen;
    } else {
      // See if the first thing in the section of text is a
      // non-breaking space (html nbsp entity). If it is then make
      // note of that fact for the line layout logic.
      if (aTextData.mWrapping && firstThing && (firstChar == ' ')) {
        textStartsWithNBSP = PR_TRUE;
      }
      aTextData.mSkipWhitespace = PR_FALSE;

      if (aTextData.mFirstLetterOK) {
        // XXX need a lookup function here; plus look ahead using the
        // text-runs
        if ((firstChar == '\'') || (firstChar == '\"')) {
          wordLen = 2;
          contentLen = 2;
        }
        else {
          wordLen = 1;
          contentLen = 1;
        }
        justDidFirstLetter = PR_TRUE;
      }
      
      if (aTextData.mMeasureText) {
        if (measureTextRuns && !justDidFirstLetter) {
          // Add another word to the text run
          textRun.AddSegment(wordLen, contentLen, PR_FALSE);

          // See if we should measure the text
          if ((textRun.mTotalNumChars >= estimatedNumChars) ||
              (textRun.mNumSegments >= (TEXT_MAX_NUM_SEGMENTS - 1))) {
            goto MeasureTextRun;
          }
        }
        else {
          if (aTs.mSmallCaps) {
            MeasureSmallCapsText(aReflowState, aTs, bp2, wordLen, &width);
          }
          else {
            // Measure just the one word
            if (aTx.TransformedTextIsAscii()) {
              aReflowState.rendContext->GetWidth(bp1, wordLen, width);
            } else {
              aReflowState.rendContext->GetWidth(bp2, wordLen, width);
            }
            if (aTs.mLetterSpacing) {
              width += aTs.mLetterSpacing * wordLen;
            }
          }
          lastWordWidth = width;

          // See if there is room for the text
          if ((0 != aTextData.mX) && aTextData.mWrapping && (aTextData.mX + width > maxWidth)) {
            // The text will not fit.
            break;
          }
          aTextData.mX += width;
          prevMaxWordWidth = aTextData.mMaxWordWidth;
          if (width > aTextData.mMaxWordWidth) {
            aTextData.mMaxWordWidth = width;
          }

          prevColumn = column;
          column += wordLen;
          endsInWhitespace = PR_FALSE;
          prevOffset = aTextData.mOffset;
          aTextData.mOffset += contentLen;
          if (justDidFirstLetter) {
            // Time to stop
            break;
          }
        }
      }
      else {
        // We didn't measure the text, but we need to update our state
        prevColumn = column;
        column += wordLen;
        endsInWhitespace = PR_FALSE;
        prevOffset = aTextData.mOffset;
        aTextData.mOffset += contentLen;
        if (justDidFirstLetter) {
          // Time to stop
          break;
        }
      }
    }
    continue;

  MeasureTextRun:
#ifdef _WIN32
    PRInt32 numCharsFit;
    // These calls can return numCharsFit not positioned at a break in the textRun. Beware.
    if (aTx.TransformedTextIsAscii()) {
      aReflowState.rendContext->GetWidth((char*)aTx.GetWordBuffer(), textRun.mTotalNumChars,
                                         maxWidth - aTextData.mX,
                                         textRun.mBreaks, textRun.mNumSegments,
                                         width, numCharsFit);
    } else {
      aReflowState.rendContext->GetWidth(aTx.GetWordBuffer(), textRun.mTotalNumChars,
                                         maxWidth - aTextData.mX,
                                         textRun.mBreaks, textRun.mNumSegments,
                                         width, numCharsFit);
    }
    // See how much of the text fit
    if ((0 != aTextData.mX) && aTextData.mWrapping && (aTextData.mX + width > maxWidth)) {
      // None of the text fits
      break;
    }

    // Find the index of the last segment that fit
    PRInt32 lastSegment;
    if (numCharsFit == textRun.mTotalNumChars) { // fast path, normal case
      lastSegment = textRun.mNumSegments - 1;
    } else {
      for (lastSegment = 0; textRun.mBreaks[lastSegment] < numCharsFit; lastSegment++) ;
      NS_ASSERTION(lastSegment < textRun.mNumSegments, "failed to find segment");
      // now we have textRun.mBreaks[lastSegment] >= numCharsFit
      /* O'Callahan XXX: This snippet together with the snippet below prevents mail from loading
         Justification seems to work just fine without these changes.
         We get into trouble in a case where lastSegment gets set to -1

      if (textRun.mBreaks[lastSegment] > numCharsFit) {
        // NOTE: this segment did not actually fit!
        lastSegment--;
      }
      */
    }

    /* O'Callahan XXX: This snippet together with the snippet above prevents mail from loading

    if (lastSegment < 0) {        
      // no segments fit
      break;
    } else */
    if (lastSegment == 0) {
      // Only one segment fit
      prevColumn = column;
      prevOffset = aTextData.mOffset;
    } else {
      // The previous state is for the next to last word
      prevColumn = textRun.mBreaks[lastSegment - 1];
      prevOffset = textRun.mSegments[lastSegment - 1].ContentLen();
      // NOTE: The textRun data are relative to the last updated column and offset!
      prevColumn = column + textRun.mBreaks[lastSegment - 1];
      prevOffset = aTextData.mOffset + textRun.mSegments[lastSegment - 1].ContentLen();
    }

    aTextData.mX += width;
    column += numCharsFit;
    aTextData.mOffset += textRun.mSegments[lastSegment].ContentLen();
    endsInWhitespace = textRun.mSegments[lastSegment].IsWhitespace();
    // Since we measure multiple words we don't know what the last word
    // width is
    lastWordWidth = -1;

    // If all the text didn't fit, then we're done
    if (numCharsFit != textRun.mTotalNumChars) {
      break;
    }

    if (nsnull == bp2) {
      // No more text so we're all finished. Advance the offset in case the last
      // call to GetNextWord() discarded characters
      aTextData.mOffset += contentLen;
      break;
    }

    // Reset the number of text run segments
    textRun.Reset();

    // Estimate the remaining number of characters we think will fit
    estimatedNumChars = (maxWidth - aTextData.mX) / aTs.mAveCharWidth;
    estimatedNumChars += estimatedNumChars / 20;
#else
    int unused = -1;
#endif
  }

  // If we didn't actually measure any text, then make sure it looks
  // like we did
  if (!aTextData.mMeasureText) {
    aTextData.mX = mRect.width;
    if (mState & TEXT_TRIMMED_WS) {
      // Add back in the width of a space since it was trimmed away last time
      // NOTE: Trailing whitespace includes word spacing!
      aTextData.mX += aTs.mSpaceWidth + aTs.mWordSpacing;
    }
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
    else if ((aTextData.mOffset == contentLength) && (prevOffset >= 0)) {
      // Force breakable to false when we aren't wrapping (this
      // guarantees that the combined word will stay together)
      if (!aTextData.mWrapping) {
        aTextData.mIsBreakable = PR_FALSE;
      }

      // This frame does start a word. However, there is no point
      // messing around with it if we are already out of room. We
      // always have room if we are not breakable.
      if (!aTextData.mIsBreakable || (aTextData.mX <= maxWidth)) {
        // There is room for this word fragment. It's possible that
        // this word fragment is the end of the text-run. If it's not
        // then we continue with the look-ahead processing.
        nsIFrame* next = lineLayout.FindNextText(this);
        if (nsnull != next) {
#ifdef DEBUG_WORD_WRAPPING
          nsAutoString tmp(aTx.GetWordBuffer(), lastWordLen);
          ListTag(stdout);
          printf(": start='");
          fputs(tmp, stdout);
          printf("' lastWordLen=%d baseWidth=%d prevOffset=%d offset=%d next=",
                 lastWordLen, lastWordWidth, prevOffset, aTextData.mOffset);
          ListTag(stdout, next);
          printf("\n");
#endif
          PRUnichar* pWordBuf = lastWordPtr;
          PRUint32   wordBufLen = aTx.GetWordBufferLength() -
                                  (lastWordPtr - aTx.GetWordBuffer());

          if (aTx.TransformedTextIsAscii()) {
            // The text transform buffer contains ascii characters, so
            // transform it to Unicode
            NS_ASSERTION(wordBufLen >= PRUint32(lastWordLen), "no room to transform in place");
            TransformTextToUnicode((char*)lastWordPtr, lastWordLen);
          }

          // Look ahead in the text-run and compute the final word
          // width, taking into account any style changes and stopping
          // at the first breakable point.
          if (!aTextData.mMeasureText || (lastWordWidth == -1)) {
            // We either didn't measure any text or we measured multiple words
            // at once so either way we don't know lastWordWidth. We'll have to
            // compute it now
            if (prevOffset == startingOffset) {
              // There's only one word, so we don't have to measure after all
              lastWordWidth = aTextData.mX;
            }
            else if (aTs.mSmallCaps) {
              MeasureSmallCapsText(aReflowState, aTs, pWordBuf,
                                   lastWordLen, &lastWordWidth);
            }
            else {
              aReflowState.rendContext->GetWidth(pWordBuf, lastWordLen, lastWordWidth);
              if (aTs.mLetterSpacing) {
                lastWordWidth += aTs.mLetterSpacing * lastWordLen;
              }
            }
          }
          nscoord wordWidth = ComputeTotalWordWidth(aPresContext, aLb,
                                                    lineLayout,
                                                    aReflowState, next,
                                                    lastWordWidth,
                                                    pWordBuf,
                                                    lastWordLen,
                                                    wordBufLen);
          if (!aTextData.mIsBreakable || (aTextData.mX - lastWordWidth + wordWidth <= maxWidth)) {
            // The fully joined word has fit. Account for the joined
            // word's affect on the max-element-size here (since the
            // joined word is large than it's pieces, the right effect
            // will occur from the perspective of the container
            // reflowing this frame)
            if (wordWidth > aTextData.mMaxWordWidth) {
              aTextData.mMaxWordWidth = wordWidth;
            }
          }
          else {
#ifdef NOISY_REFLOW
            ListTag(stdout);
            printf(": look-ahead (didn't fit) x=%d wordWidth=%d lastWordWidth=%d\n",
                   aTextData.mX, wordWidth, lastWordWidth);
#endif
            // The fully joined word won't fit. We need to reduce our
            // size by the lastWordWidth.
            aTextData.mX -= lastWordWidth;
            aTextData.mMaxWordWidth = prevMaxWordWidth;
            aTextData.mOffset = prevOffset;
            column = prevColumn;
#ifdef DEBUG_WORD_WRAPPING
            printf("  x=%d maxWordWidth=%d len=%d\n", aTextData.mX, aTextData.mMaxWordWidth,
                   aTextData.mOffset - startingOffset);
#endif
            lineLayout.ForgetWordFrames();
          }
        }
      }
    }
  }

  // Inform line layout of how this piece of text ends in whitespace
  // (only text objects do this). Note that if x is zero then this
  // text object collapsed into nothingness which means it shouldn't
  // effect the current setting of the ends-in-whitespace flag.
  lineLayout.SetColumn(column);
  lineLayout.SetUnderstandsWhiteSpace(PR_TRUE);
  lineLayout.SetTextStartsWithNBSP(textStartsWithNBSP);
  if (0 != aTextData.mX) {
    lineLayout.SetEndsInWhiteSpace(endsInWhitespace);
  }
  if (justDidFirstLetter) {
    lineLayout.SetFirstLetterFrame(this);
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
    mState |= TEXT_FIRST_LETTER;
  }

  // Return our reflow status
  nsReflowStatus rs = (aTextData.mOffset == contentLength)
    ? NS_FRAME_COMPLETE
    : NS_FRAME_NOT_COMPLETE;
  if (endsInNewline) {
    rs = NS_INLINE_LINE_BREAK_AFTER(rs);
  }
  else if ((aTextData.mOffset != contentLength) && (aTextData.mOffset == startingOffset)) {
    // Break-before a long-word that doesn't fit here
    rs = NS_INLINE_LINE_BREAK_BEFORE();
  }

  return rs;
}

NS_IMETHODIMP
nsTextFrame::Reflow(nsIPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTextFrame", aReflowState.reason);
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
#ifdef MOZ_MATHML
    if (NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags)
      aMetrics.mBoundingMetrics.Clear();
#endif
    return NS_OK;
  }

  // Get starting offset into the content
  PRInt32 startingOffset = 0;
  nsIFrame* prevInFlow;

  GetPrevInFlow(&prevInFlow);
  if (nsnull != prevInFlow) {
    nsTextFrame* prev = (nsTextFrame*)prevInFlow;
    startingOffset = prev->mContentOffset + prev->mContentLength;

    // If our starting offset doesn't agree with mContentOffset, then our
    // prev-in-flow has changed the number of characters it maps and so we
    // need to measure text and not try and optimize a resize reflow
    if (startingOffset != mContentOffset) {
      mState &= ~TEXT_OPTIMIZE_RESIZE;
    }
  }

  nsLineLayout& lineLayout = *aReflowState.mLineLayout;
  TextStyle ts(aPresContext, *aReflowState.rendContext, mStyleContext);

  // Clear out the reflow state flags in mState (without destroying
  // the TEXT_BLINK_ON bit).
  PRBool lastTimeWeSkippedLeadingWS = 0 != (mState & TEXT_SKIP_LEADING_WS);
  mState &= ~TEXT_REFLOW_FLAGS;
  if (ts.mFont->mFont.decorations & NS_STYLE_TEXT_DECORATION_BLINK) {
    if (0 == (mState & TEXT_BLINK_ON)) {
      mState |= TEXT_BLINK_ON;
      nsBlinkTimer::AddBlinkFrame(aPresContext, this);
    }
  }
  else {
    if (0 != (mState & TEXT_BLINK_ON)) {
      mState &= ~TEXT_BLINK_ON;
      nsBlinkTimer::RemoveBlinkFrame(this);
    }
  }

  PRBool wrapping = (NS_STYLE_WHITESPACE_NORMAL == ts.mText->mWhiteSpace) ||
    (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == ts.mText->mWhiteSpace);

  // Set whitespace skip flag
  PRBool skipWhitespace = PR_FALSE;
  if (!ts.mPreformatted) {
    if (lineLayout.GetEndsInWhiteSpace()) {
      skipWhitespace = PR_TRUE;
    }
  }

  nscoord maxWidth = aReflowState.availableWidth;

  // Setup text transformer to transform this frames text content
  nsCOMPtr<nsIDocument> doc;
  mContent->GetDocument(*getter_AddRefs(doc));
  if (NS_WARN_IF_FALSE(doc, "Content has no document.")) { 
    return NS_ERROR_FAILURE; 
  }
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  // Keep the text in ascii if possible. Note that if we're measuring small
  // caps text then transform to Unicode because the helper function only
  // accepts Unicode text
  nsresult rv = tx.Init(this, mContent, startingOffset, !ts.mSmallCaps);
  if (NS_OK != rv) {
    return rv;
  }
  PRInt32 contentLength = tx.GetContentLength();

  // Set inWord to true if we are part of a previous piece of text's word. This
  // is only valid for one pass through the measuring loop.
  PRBool inWord = lineLayout.InWord() || ((nsnull != prevInFlow) && (((nsTextFrame*)prevInFlow)->mState & TEXT_FIRST_LETTER));
  if (inWord) {
    mState |= TEXT_IN_WORD;
  }
  mState &= ~TEXT_FIRST_LETTER;
  
  PRInt32 column = lineLayout.GetColumn();
  PRInt32 prevColumn = mColumn;
  mColumn = column;
  PRBool measureText = PR_TRUE;
  
  // We can avoid actually measuring the text if:
  // - this is a resize reflow
  // - we're not dirty (see ContentChanged() function)
  // - we don't have a next in flow
  // - the previous reflow successfully reflowed all text in the
  //   available space
  // - we aren't computing the max element size (that requires we measure
  //   text)
  // - skipping leading whitespace is the same as it was the last time
  // - we're wrapping text and the available width is at least as big as our
  //   current frame width -or-
  //   we're not wrapping text and we're at the same column as before (this is
  //   an issue for preformatted tabbed text only)
  // - AND we aren't justified (in which case the frame width has already been tweaked and can't be used)
  if ((eReflowReason_Resize == aReflowState.reason) &&
      (0 == (mState & NS_FRAME_IS_DIRTY))) {

    nscoord realWidth = mRect.width;
    if (mState & TEXT_TRIMMED_WS) {
      // NOTE: Trailing whitespace includes word spacing!
      realWidth += ts.mSpaceWidth + ts.mWordSpacing;    
    }
    if (!mNextInFlow &&
        (mState & TEXT_OPTIMIZE_RESIZE) &&
        !aMetrics.maxElementSize &&
        (lastTimeWeSkippedLeadingWS == skipWhitespace) &&
        ((wrapping && (maxWidth >= realWidth)) ||
         (!wrapping && (prevColumn == column))) &&
        !ts.mJustifying) {
      // We can skip measuring of text and use the value from our
      // previous reflow
      measureText = PR_FALSE;
#ifdef NOISY_REFLOW
      printf("  => measureText=%s wrapping=%s skipWhitespace=%s",
             measureText ? "yes" : "no",
             wrapping ? "yes" : "no",
             skipWhitespace ? "yes" : "no");
      printf(" realWidth=%d maxWidth=%d\n",
             realWidth, maxWidth);
#endif
    }
  }

  // Local state passed to the routines that do the actual text measurement
  TextReflowData  textData(startingOffset, wrapping, skipWhitespace, 
                           measureText, inWord, lineLayout.GetFirstLetterStyleOK(),
                           lineLayout.LineIsBreakable(), nsnull != aMetrics.maxElementSize);
  
  // Measure the text
  aStatus = MeasureText(aPresContext, aReflowState, tx, lb, ts, textData);

  if (tx.HasMultibyte()) {
    mState |= TEXT_HAS_MULTIBYTE;
  }
  mState &= ~TEXT_TRIMMED_WS;

  // Setup metrics for caller; store final max-element-size information
  aMetrics.width = textData.mX;
  if ((0 == textData.mX) && !ts.mPreformatted) {
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
    textData.mMaxWordWidth = textData.mX;
  }
  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width = textData.mMaxWordWidth;
    aMetrics.maxElementSize->height = aMetrics.height;
  }

  // Set content offset and length
  mContentOffset = startingOffset;
  mContentLength = textData.mOffset - startingOffset;

  // Compute space and letter counts for justification, if required
  if (ts.mJustifying) {
    PRInt32 numSpaces;
    PRInt32 textLength;

    // This will include a space for trailing whitespace, if any is present.
    // This is corrected for in nsLineLayout::TrimWhiteSpaceIn.

    // This work could be done in MeasureText, but it's complex to do accurately
    // there because of the need to repair counts when wrapped words are backed out.
    // So I do it via PrepareUnicodeText ... a little slower perhaps, but a lot saner,
    // and it localizes the counting logic to one place.
    numSpaces = PrepareUnicodeText(tx, nsnull, nsnull, &textLength);
    lineLayout.SetTextJustificationWeights(numSpaces, textLength - numSpaces);
  }


#ifdef MOZ_MATHML
  // Simple minded code to also return the bounding metrics if the caller wants it...
  // More consolidation is needed -- a better approach is to follow what is done by
  // the other routines that are doing measurements.
  if (NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags) {
    rv = NS_ERROR_UNEXPECTED; // initialize with an error; it is reset if things turn out well
    aMetrics.mBoundingMetrics.Clear();
    // Get the text to measure. 
    nsCOMPtr<nsIDOMText> domText(do_QueryInterface(mContent));
    if (domText.get()) {
      nsAutoString aData;
      domText->GetData(aData);

      // Extract the piece of text relevant to us -- XXX whitespace cause a mess here  
      nsAutoString aText;
      aData.Mid(aText, mContentOffset, mContentLength);

      // Set the font
      nsStyleFont font;
      mStyleContext->GetStyle(eStyleStruct_Font, font);
      aReflowState.rendContext->SetFont(font.mFont);

      // Now get the exact bounding metrics of the text
      nsBoundingMetrics bm;
      rv = aReflowState.rendContext->GetBoundingMetrics(aText.GetUnicode(), PRUint32(mContentLength), bm);
      if (NS_SUCCEEDED(rv)) aMetrics.mBoundingMetrics = bm;
    }
    if (NS_FAILED(rv)) { 
      // Things didn't turn out well, just return the reflow metrics.
      aMetrics.mBoundingMetrics.ascent  = aMetrics.ascent;
      aMetrics.mBoundingMetrics.descent = aMetrics.descent;
      aMetrics.mBoundingMetrics.width   = aMetrics.width;
#ifdef MOZ_MATHML_BOUNDINGMETRICS
      printf("nsTextFrame: could not perform GetBoundingMetrics()\n");
#endif
    }
  }
#endif

  // If it's an incremental reflow command, then invalidate our existing
  // bounds.
  // XXX We need a finer granularity than this, but it isn't clear what
  // has actually changed...
  if (eReflowReason_Incremental == aReflowState.reason ||
      eReflowReason_Dirty == aReflowState.reason) {
    Invalidate(aPresContext, mRect);
  }

  // For future resize reflows we would like to avoid measuring the text.
  // We can only do this if after this reflow we're:
  // - complete. If we're not complete then our desired width doesn't
  //   represent our total size
  // - we fit in the available space. We may be complete, but if we
  //   return a larger desired width than is available we may get pushed
  //   and our frame width won't get set
  if ((NS_FRAME_COMPLETE == aStatus) && (aMetrics.width <= maxWidth)) {
    mState |= TEXT_OPTIMIZE_RESIZE;
    mRect.width = aMetrics.width;
  }
  else {
    mState &= ~TEXT_OPTIMIZE_RESIZE;
  }

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
  if (mContentLength &&
      (NS_STYLE_WHITESPACE_PRE != textStyle->mWhiteSpace) &&
      (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP != textStyle->mWhiteSpace)) {

    // Get the text fragments that make up our content
    nsCOMPtr<nsITextContent> tc = do_QueryInterface(mContent);
    if (tc) {
      const nsTextFragment* frag;
      tc->GetText(&frag);
      PRInt32 lastCharIndex = mContentOffset + mContentLength - 1;
      if (lastCharIndex < frag->GetLength()) {
        PRUnichar ch = frag->CharAt(lastCharIndex);
        if (XP_IS_SPACE(ch)) {
          // Get font metrics for a space so we can adjust the width by the
          // right amount.
          const nsStyleFont* fontStyle = (const nsStyleFont*)
            mStyleContext->GetStyleData(eStyleStruct_Font);
          aRC.SetFont(fontStyle->mFont);
          aRC.GetWidth(' ', dw);
          // NOTE: Trailing whitespace includes word spacing!
          PRIntn unit = textStyle->mWordSpacing.GetUnit();
          if (eStyleUnit_Coord == unit) {
            dw += textStyle->mWordSpacing.GetCoordValue();
          }
        }
      }
    }
  }
#ifdef NOISY_TRIM
  ListTag(stdout);
  printf(": trim => %d\n", dw);
#endif
  if (0 != dw) {
    mState |= TEXT_TRIMMED_WS;
  }
  else {
    mState &= ~TEXT_TRIMMED_WS;
  }
  aDeltaWidth = dw;
  return NS_OK;
}

static void
RevertSpacesToNBSP(PRUnichar* aBuffer, PRInt32 aWordLen)
{
  PRUnichar* end = aBuffer + aWordLen;
  for (; aBuffer < end; aBuffer++) {
    PRUnichar ch = *aBuffer;
    if (ch == ' ') {
      *aBuffer = CH_NBSP;
    }
  }
}

nscoord
nsTextFrame::ComputeTotalWordWidth(nsIPresContext* aPresContext,
                                   nsILineBreaker* aLineBreaker,
                                   nsLineLayout& aLineLayout,
                                   const nsHTMLReflowState& aReflowState,
                                   nsIFrame* aNextFrame,
                                   nscoord aBaseWidth,
                                   PRUnichar* aWordBuf,
                                   PRUint32 aWordLen,
                                   PRUint32 aWordBufSize)
{
  // Before we get going, convert any spaces in the current word back
  // to nbsp's. This keeps the breaking logic happy.
  RevertSpacesToNBSP(aWordBuf, (PRInt32) aWordLen);

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
        PRBool stop = PR_FALSE;
        nscoord moreWidth = ComputeWordFragmentWidth(aPresContext,
                                                     aLineBreaker,
                                                     aLineLayout,
                                                     aReflowState,
                                                     aNextFrame, content, tc,
                                                     &stop,
                                                     aWordBuf,
                                                     aWordLen,
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
                                      PRUint32& aRunningWordLen,
                                      PRUint32 aWordBufSize)
{
  nsTextTransformer tx(aLineBreaker, nsnull);
  tx.Init(aTextFrame, aContent, 0);
  PRBool isWhitespace, wasTransformed;
  PRInt32 wordLen, contentLen;
  PRUnichar* bp = tx.GetNextWord(PR_TRUE, &wordLen, &contentLen, &isWhitespace, &wasTransformed);
  if (!bp || isWhitespace) {
    // Don't bother measuring nothing
    *aStop = PR_TRUE;
    return 0;
  }
  *aStop = contentLen < tx.GetContentLength();

  // Convert any spaces in the current word back to nbsp's. This keeps
  // the breaking logic happy.
  RevertSpacesToNBSP(bp, wordLen);

  // We need to adjust the length by look at the two pieces together
  // XXX this should grow aWordBuf if necessary
  PRUint32 copylen = sizeof(PRUnichar) *
    ( ((wordLen + aRunningWordLen) > aWordBufSize)
      ? (aWordBufSize - aRunningWordLen)
      : wordLen
      );
  if((aWordBufSize > aRunningWordLen) && (copylen > 0))
  {
    nsCRT::memcpy((void*)&(aWordBuf[aRunningWordLen]), bp, copylen);

    PRUint32 breakP=0;
    PRBool needMore=PR_TRUE;
    nsresult lres = aLineBreaker->Next(aWordBuf, aRunningWordLen+wordLen,
                                       0, &breakP, &needMore);
    if(NS_SUCCEEDED(lres)) 
    {
       // when we look at two pieces text together, we might decide to break
       // eariler than if we only look at the 2nd pieces of text
       if(!needMore && (breakP < (aRunningWordLen + wordLen)))
       {
           wordLen = breakP - aRunningWordLen; 
           if(wordLen < 0)
               wordLen = 0;
           *aStop = PR_TRUE;
       } 
    }

    // if we don't stop, we need to extend the buf so the next one can
    // see this part otherwise, it does not matter since we will stop
    // anyway
    if(! *aStop) 
      aRunningWordLen += wordLen;
  }
  if((*aStop) && (wordLen == 0))
	  return 0;

  nsCOMPtr<nsIStyleContext> sc;
  aTextFrame->GetStyleContext(getter_AddRefs(sc));
  if (sc) {
    // Measure the piece of text. Note that we have to select the
    // appropriate font into the text first because the rendering
    // context has our font in it, not the font that aText is using.
    nscoord width;
    nsIRenderingContext& rc = *aReflowState.rendContext;
    nsCOMPtr<nsIFontMetrics> oldfm;
    rc.GetFontMetrics(*getter_AddRefs(oldfm));

    TextStyle ts(aLineLayout.mPresContext, rc, sc);
    if (ts.mSmallCaps) {
      MeasureSmallCapsText(aReflowState, ts, bp, wordLen, &width);
    }
    else {
      rc.GetWidth(bp, wordLen, width);
      // NOTE: Don't forget to add letter spacing for the word fragment!
      width += wordLen*ts.mLetterSpacing;
    }
    rc.SetFont(oldfm);

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
nsTextFrame::ToCString(nsString& aBuf, PRInt32* aTotalContentLength) const
{
  const nsTextFragment* frag;

  // Get the frames text content
  nsITextContent* tc;
  if (NS_OK != mContent->QueryInterface(kITextContentIID, (void**) &tc)) {
    return;
  }
  tc->GetText(&frag);
  NS_RELEASE(tc);

  // Compute the total length of the text content.
  *aTotalContentLength = frag->GetLength();

  // Set current fragment and current fragment offset
  if (0 == mContentLength) {
    return;
  }
  PRInt32 fragOffset = mContentOffset;
  PRInt32 n = fragOffset + mContentLength;
  while (fragOffset < n) {
    PRUnichar ch = frag->CharAt(fragOffset++);
    if (ch == '\r') {
      aBuf.AppendWithConversion("\\r");
    } else if (ch == '\n') {
      aBuf.AppendWithConversion("\\n");
    } else if (ch == '\t') {
      aBuf.AppendWithConversion("\\t");
    } else if ((ch < ' ') || (ch >= 127)) {
      aBuf.AppendWithConversion("\\0");
      aBuf.AppendInt((PRInt32)ch, 8);
    } else {
      aBuf.Append(ch);
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

NS_IMETHODIMP
nsTextFrame::GetFrameName(nsString& aResult) const
{
  return MakeFrameName("Text", aResult);
}

NS_IMETHODIMP
nsTextFrame::List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const
{
  // Output the tag
  IndentBy(out, aIndent);
  ListTag(out);
  nsIView* view;
  GetView(aPresContext, &view);
  if (nsnull != view) {
    fprintf(out, " [view=%p]", view);
  }

  PRInt32 totalContentLength;
  nsAutoString tmp;
  ToCString(tmp, &totalContentLength);

  // Output the first/last content offset and prev/next in flow info
  PRBool isComplete = (mContentOffset + mContentLength) == totalContentLength;
  fprintf(out, "[%d,%d,%c] ", 
          mContentOffset, mContentLength,
          isComplete ? 'T':'F');
  
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", mNextSibling);
  }
  nsIFrame* prevInFlow;
  GetPrevInFlow(&prevInFlow);
  if (nsnull != prevInFlow) {
    fprintf(out, " prev-in-flow=%p", prevInFlow);
  }
  if (nsnull != mNextInFlow) {
    fprintf(out, " next-in-flow=%p", mNextInFlow);
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
#endif
