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

//tripple click includes
#include "nsIPref.h"
#include "nsIServiceManager.h"

#ifndef PR_ABS
#define PR_ABS(x) (x < 0 ? -x : x)
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
  PRInt32 newSize = mBufferLen * 2;
  if (newSize < mBufferLen + aAtLeast) {
    newSize = mBufferLen * 2 + aAtLeast;
  }
  PRInt32* newBuffer = new PRInt32[newSize];
  if (!newBuffer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCRT::memcpy(newBuffer, mBuffer, sizeof(PRInt32) * mBufferLen);
  if (mBuffer != mAutoBuffer) {
    delete [] mBuffer;
  }
  mBuffer = newBuffer;
  mBufferLen = newSize;
  return NS_OK;
}

//----------------------------------------------------------------------

// Helper class for managing blinking text

class nsBlinkTimer : public nsITimerCallback {
public:
  nsBlinkTimer();
  virtual ~nsBlinkTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsIPresContext* aPresContext, nsIFrame* aFrame);

  PRBool RemoveFrame(nsIFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  virtual void Notify(nsITimer *timer);

  struct FrameData {
    nsIPresContext* mPresContext;  // pres context associated with the frame
    nsIFrame*       mFrame;


    FrameData(nsIPresContext* aPresContext,
              nsIFrame*       aFrame)
      : mPresContext(aPresContext), mFrame(aFrame) {}
  };

  nsITimer* mTimer;
  nsVoidArray mFrames;
  nsIPresContext* mPresContext;
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
    PRBool mJustifying;
    PRBool mPreformatted;
    PRIntn mNumSpaces;
    nscoord mExtraSpacePerSpace;
    nscoord mRemainingExtraSpace;

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

    mFont = nsnull;
    mText = nsnull;
    mColor = nsnull;
    mNormalFont = nsnull;
    mSmallFont = nsnull;
    mLastFont = nsnull;

    }
  };

  nsIDocument* GetDocument(nsIPresContext* aPresContext);

  PRIntn PrepareUnicodeText(nsTextTransformer& aTransformer,
                            nsAutoIndexBuffer* aIndexBuffer,
                            nsAutoTextBuffer* aTextBuffer,
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

// Bits in mState used for reflow flags
#define TEXT_REFLOW_FLAGS    0x0F000000

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

  // Generate a reflow command with this frame as the target frame
  nsIReflowCommand* cmd;
  nsresult          rv;
                                                
  rv = NS_NewHTMLReflowCommand(&cmd, targetTextFrame,
                               nsIReflowCommand::ContentChanged);
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
nsTextFrame::Paint(nsIPresContext* aPresContext,
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
    TextStyle ts(aPresContext, aRenderingContext, mStyleContext);
    if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing)) {
      PaintTextSlowly(aPresContext, aRenderingContext, sc, ts, 0, 0);
    }
    else {
      // Choose rendering pathway based on rendering context
      // performance hint.
      PRUint32 hints = 0;
      aRenderingContext.GetHints(hints);
      if ((TEXT_HAS_MULTIBYTE & mState) ||
          (0 == (hints & NS_RENDERING_HINT_FAST_8BIT_TEXT))) {
        // Use PRUnichar rendering routine
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
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;
    aTX.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace);
    NS_ASSERTION(isWhitespace, "mState and content are out of sync");
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
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;

    // Get the next word
    bp = aTX.GetNextWord(inWord, &wordLen, &contentLen, &isWhitespace);
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
    }
    else {
      if (nsnull != indexp) {
        // Point mapping indicies at each content index in the word
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *indexp++ = strInx++;
        }
      }
    }

    // Grow the buffer before we run out of room. The only time this
    // happens is because of tab expansion.
    if (dstOffset + wordLen > aTextBuffer->mBufferLen) {
      nsresult rv = aTextBuffer->GrowBy(wordLen);
      if (NS_FAILED(rv)) {
        break;
      }
    }

    column += wordLen;
    textLength += wordLen;
    n -= contentLen;
    nsCRT::memcpy(aTextBuffer->mBuffer + dstOffset, bp,
                  sizeof(PRUnichar)*wordLen);
    dstOffset += wordLen;
  }

#ifdef DEBUG
  if (aIndexBuffer) {
    NS_ASSERTION(indexp <= aIndexBuffer->mBuffer + aIndexBuffer->mBufferLen,
                 "yikes - we just overwrote memory");
  }
  NS_ASSERTION(dstOffset <= aTextBuffer->mBufferLen,
               "yikes - we just overwrote memory");
#endif

  // Remove trailing whitespace if it was trimmed after reflow
  if (TEXT_TRIMMED_WS & mState) {
    if (--dstOffset >= 0) {
      PRUnichar ch = aTextBuffer->mBuffer[dstOffset];
      if (XP_IS_SPACE(ch)) {
        textLength--;
      }
    }
    numSpaces--;
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
          case SELECTION_NORMAL:{
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
                                }break;
           case SELECTION_SPELLCHECK:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(255,0,0));
              aRenderingContext.FillRect(aX + startOffset, aY + baseline - offset, textWidth, size);
                                }break;
          case SELECTION_IME_SELECTEDRAWTEXT:{
#ifdef USE_INVERT_FOR_SELECTION
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(255,255,128));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif        
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(189,33,66));
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset+size, textWidth-2*size, size);
                                }break;
          case SELECTION_IME_RAWINPUT:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(189,33,66));
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset+size, textWidth-2*size, size);
                                }break;
          case SELECTION_IME_SELECTEDCONVERTEDTEXT:{
#ifdef USE_INVERT_FOR_SELECTION
              aRenderingContext.SetColor(NS_RGB(255,255,255));
              aRenderingContext.InvertRect(aX + startOffset, aY, textWidth, rect.height);
#else
              aRenderingContext.SetColor(NS_RGB(255,255,128));
              aRenderingContext.DrawRect(aX + startOffset, aY, textWidth, rect.height);
#endif        
			  aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(189,33,66));
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size*2);
                                }break;
          case SELECTION_IME_CONVERTEDTEXT:{
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetColor(NS_RGB(189,33,66));
              aRenderingContext.FillRect(aX + startOffset+size, aY + baseline - offset, textWidth-2*size, size*2);
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
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }
  nscoord width = mRect.width;

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 textLength;
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
          rv = GetContent(getter_AddRefs(content));
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
  if (!ts.mSmallCaps && !ts.mWordSpacing && !ts.mLetterSpacing) {
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
  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);
  if (textLength <= 0) {
    return NS_ERROR_FAILURE;
  }

//IF STYLE SAYS TO SELCT TO END OF FRAME HERE...
  nsCOMPtr<nsIPref>     prefs;
  PRInt32 prefInt = 0;
  rv = nsServiceManager::GetService(kPrefCID, 
                                    nsIPref::GetIID(), 
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
    PRInt32 actualLength = textLength;

    nscoord charWidth,widthsofar = 0;
    PRInt32 indx = 0;
    PRBool found = PR_FALSE;
    PRInt32* ip = indexBuffer.mBuffer;
    PRUnichar* paintBuf = paintBuffer.mBuffer;
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
      if ((aPoint.x  - origin.x) >= widthsofar && (aPoint.x - origin.x) <= (widthsofar + glyphWidth)){
        if ( ((aPoint.x - origin.x) - widthsofar) <= (glyphWidth /2)){
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
    (0 != aTextStyle.mWordSpacing);
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
  nsAutoTextBuffer widthBuffer;
  if (NS_FAILED(widthBuffer.GrowTo(aLength))) {
    *aWidthResult = 0;
    return;
  }
  PRUnichar* bp = widthBuffer.mBuffer;

  nsIFontMetrics* lastFont = aTextStyle.mLastFont;
  nscoord sum = 0;
  nscoord charWidth;
  while (--aLength >= 0) {
    nscoord glyphWidth;
    PRUnichar ch = *aBuffer++;
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
  aTextStyle.mNumSpaces = PrepareUnicodeText(tx,
                                             (displaySelection
                                              ? &indexBuffer : nsnull),
                                             &paintBuffer, &textLength);

  PRInt32* ip = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;
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
  nsAutoTextBuffer unicodePaintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }

  // Transform text from content into renderable form
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
  PRInt32 textLength;
  PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                     &unicodePaintBuffer, &textLength);

  // Translate unicode data into ascii for rendering
  char paintBufMem[TEXT_BUF_SIZE];
  char* paintBuf = paintBufMem;
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

  nscoord width = mRect.width;
  PRInt32* ip = indexBuffer.mBuffer;
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
      aRenderingContext.SetColor(aTextStyle.mColor->mColor);
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy);
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
      if (ts.mSmallCaps || ts.mWordSpacing || ts.mLetterSpacing) {

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
      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

      if (textLength <=0) {
        //invalid frame to get position on
        return NS_ERROR_FAILURE;
      }

      nsPoint origin;
      nsIView * view;
      GetOffsetFromView(aCX, origin, &view);

//IF SYLE SAYS TO SELCT TO END OF FRAME HERE...
      nsCOMPtr<nsIPref>     prefs;
      PRInt32 prefInt = 0;
      rv = nsServiceManager::GetService(kPrefCID, 
                                        nsIPref::GetIID(), 
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

          if (PR_ABS(PRInt32(aPoint.x) - origin.x) > textWidth+charWidth) {
            indx++;
          }
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
  PRBool isSelected = ((frameState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);
  /*if (!aSelected && !isSelected) //already set thanks
  {
    return NS_OK;
  }*/

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
        rv = GetContent(getter_AddRefs(content));
        if (NS_SUCCEEDED(rv) && content){
          rv = frameSelection->LookUpSelection(content, mContentOffset, 
                                mContentLength , &details, PR_TRUE);
// PR_TRUE last param used here! we need to see if we are still selected. so no shortcut

        }
      }
    }
    if (!details)
      frameState &= ~NS_FRAME_SELECTED_CONTENT;
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
#if 0
    else //we need to talk to siblings as well as flow
    {
      result = GetNextSibling(&frame);
      while (NS_SUCCEEDED(result) && frame){
        frame->SetSelected(aRange,aSelected,eSpreadDown);
        result = frame->GetNextSibling(&frame);
        if (NS_FAILED(result))
          break;
      }
    }
#endif
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
  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

  PRInt32* ip = indexBuffer.mBuffer;
  if (inOffset > mContentLength){
    NS_ASSERTION(0, "invalid offset passed to GetPointFromOffset");
    inOffset = mContentLength;
  }

  nscoord width = mRect.width;
  GetWidth(*inRendContext, ts,
           paintBuffer.mBuffer, ip[inOffset]-mContentOffset,
           &width);

  if (inOffset > textLength && (TEXT_TRIMMED_WS & mState)){
    //
    // Offset must be after a space that has
    // been trimmed off the end of the frame.
    // Add the width of the trimmed space back
    // to the total width, so the caret appears
    // in the proper place!
    //
    width += ts.mSpaceWidth;
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
        result = GetFrameFromDirection(aPos);
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
        result = GetFrameFromDirection(aPos);
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
      PRBool isWhitespace;
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

        if (tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, PR_FALSE) &&
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
            while (!isWhitespace && tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, PR_FALSE)){
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
        result = GetFrameFromDirection(aPos);
        if (NS_SUCCEEDED(result) && aPos->mResultFrame && aPos->mResultFrame!= this)
        {
          if (NS_SUCCEEDED(result = aPos->mResultFrame->PeekOffset(aPresContext, aPos)))
            return NS_OK;//else fall through
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

  if (me->clickCount > 2)//triple clicking
  {
    nsCOMPtr<nsIPref>     mPrefs;
    PRInt32 prefInt = 0;
    rv = nsServiceManager::GetService(kPrefCID, 
                                               nsIPref::GetIID(), 
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
    if (NS_SUCCEEDED(shell->GetSelection(SELECTION_NORMAL, getter_AddRefs(selection)))){
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
nsTextFrame::Reflow(nsIPresContext* aPresContext,
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
      gTextBlinker->AddFrame(aPresContext, this);
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

  // Setup text transformer to transform this frames text content
  nsCOMPtr<nsIDocument> doc;
  mContent->GetDocument(*getter_AddRefs(doc));
  if (NS_WARN_IF_FALSE(doc, "Content has no document.")) { 
    return NS_ERROR_FAILURE; 
  }
  nsCOMPtr<nsILineBreaker> lb;
  doc->GetLineBreaker(getter_AddRefs(lb));
  nsTextTransformer tx(lb, nsnull);
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
    ((nsnull != prevInFlow) &&
     (((nsTextFrame*)prevInFlow)->mState & TEXT_FIRST_LETTER));
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
  if ((eReflowReason_Resize == aReflowState.reason) &&
      (0 == (mState & NS_FRAME_IS_DIRTY))) {

    nscoord realWidth = mRect.width;
    if (mState & TEXT_TRIMMED_WS) {
      realWidth += ts.mSpaceWidth;
    }
    if (!mNextInFlow &&
        (mState & TEXT_OPTIMIZE_RESIZE) &&
        !aMetrics.maxElementSize &&
        (lastTimeWeSkippedLeadingWS == skipWhitespace) &&
        ((wrapping && (maxWidth >= realWidth)) ||
         (!wrapping && (prevColumn == column)))) {
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

  PRBool firstThing = PR_TRUE;
  PRBool textStartsWithNBSP = PR_FALSE;
  for (;;) {
    // Get next word/whitespace from the text
    PRBool isWhitespace;
    PRInt32 wordLen, contentLen;
    bp = tx.GetNextWord(inWord, &wordLen, &contentLen, &isWhitespace);
    if (nsnull == bp) {
      // Advance the offset in case we just consumed a bunch of
      // discarded characters. Otherwise, if this is the first piece
      // of content for this frame we will attempt to break-before it.
      offset += contentLen;
      break;
    }
    lastWordLen = wordLen;
    inWord = PR_FALSE;

    // Measure the word/whitespace
    nscoord width;
    if (isWhitespace) {
      if ('\n' == bp[0]) {
        // We hit a newline. Stop looping.
        NS_WARN_IF_FALSE(ts.mPreformatted, "newline w/o ts.mPreformatted");
        prevOffset = offset;
        offset++;
        endsInWhitespace = PR_TRUE;
        endsInNewline = PR_TRUE;
        break;
      }
      if (skipWhitespace) {
        offset += contentLen;
        skipWhitespace = PR_FALSE;

        // Only set flag when we actually do skip whitespace
        mState |= TEXT_SKIP_LEADING_WS;
        firstThing = PR_FALSE;
        continue;
      }
      if ('\t' == bp[0]) {
        // Expand tabs to the proper width
        wordLen = 8 - (7 & column);
        width = ts.mSpaceWidth * wordLen;
      }
      else {
        width = (wordLen * ts.mSpaceWidth) + ts.mWordSpacing;// XXX simplistic
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
      
      if (measureText) {
        if (ts.mSmallCaps) {
          MeasureSmallCapsText(aReflowState, ts, bp, wordLen, &width);
        }
        else {
          aReflowState.rendContext->GetWidth(bp, wordLen, width);
          if (ts.mLetterSpacing) {
            width += ts.mLetterSpacing * wordLen;
          }
        }
        lastWordWidth = width;
      }

      // See if the first thing in the section of text is a
      // non-breaking space (html nbsp entity). If it is then make
      // note of that fact for the line layout logic.
      if (wrapping && firstThing && (bp[0] == ' ')) {
        textStartsWithNBSP = PR_TRUE;
      }
      skipWhitespace = PR_FALSE;
    }
    firstThing = PR_FALSE;

    // See if there is room for the text
    if (measureText) {
      if ((0 != x) && wrapping && (x + width > maxWidth)) {
        // The text will not fit.
#ifdef NOISY_REFLOW
        ListTag(stdout);
        printf(": won't fit (at offset=%d) x=%d width=%d maxWidth=%d\n",
               offset, x, width, maxWidth);
#endif
        break;
      }
      x += width;
      prevMaxWordWidth = maxWordWidth;
      if (width > maxWordWidth) {
        maxWordWidth = width;
      }
    }

    prevColumn = column;
    column += wordLen;
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

  // If we didn't actually measure any text, then make sure it looks
  // like we did
  if (!measureText) {
    x = mRect.width;
    if (mState & TEXT_TRIMMED_WS) {
      // Add back in the width of a space since it was trimmed away last time
      x += ts.mSpaceWidth;
    }
  }
  mState &= ~TEXT_TRIMMED_WS;

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
          nsAutoString tmp(tx.GetWordBuffer(), lastWordLen);
          ListTag(stdout);
          printf(": start='");
          fputs(tmp, stdout);
          printf("' lastWordLen=%d baseWidth=%d prevOffset=%d offset=%d next=",
                 lastWordLen, lastWordWidth, prevOffset, offset);
          ListTag(stdout, next);
          printf("\n");
#endif
          PRUnichar* pWordBuf = tx.GetWordBuffer();
          PRUint32   wordBufLen = tx.GetWordBufferLength();

          // Look ahead in the text-run and compute the final word
          // width, taking into account any style changes and stopping
          // at the first breakable point.
          if (!measureText) {
            // We didn't measure any text so we don't know lastWordWidth.
            // We have to compute it now
            if (prevOffset == startingOffset) {
              // There's only one word, so we don't have to measure after all
              lastWordWidth = x;
            }
            else if (ts.mSmallCaps) {
              MeasureSmallCapsText(aReflowState, ts, tx.GetWordBuffer(),
                                   lastWordLen, &lastWordWidth);
            }
            else {
              aReflowState.rendContext->GetWidth(tx.GetWordBuffer(),
                                                 lastWordLen, lastWordWidth);
              if (ts.mLetterSpacing) {
                lastWordWidth += ts.mLetterSpacing * lastWordLen;
              }
            }
          }
          nscoord wordWidth = ComputeTotalWordWidth(aPresContext, lb,
                                                    lineLayout,
                                                    aReflowState, next,
                                                    lastWordWidth,
                                                    pWordBuf,
                                                    lastWordLen,
                                                    wordBufLen);
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
  lineLayout.SetTextStartsWithNBSP(textStartsWithNBSP);
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
      aMetrics.mBoundingMetrics.ascent  =  aMetrics.ascent;
      aMetrics.mBoundingMetrics.descent = -aMetrics.descent;
      aMetrics.mBoundingMetrics.width   =  aMetrics.width;
      printf("nsTextFrame: could not perform GetBoundingMetrics()\n");
    }
  }
#endif

  // If it's an incremental reflow command, then invalidate our existing
  // bounds.
  // XXX We need a finer granularity than this, but it isn't clear what
  // has actually changed...
  if (eReflowReason_Incremental == aReflowState.reason) {
    Invalidate(aPresContext, mRect);
  }

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
  PRBool isWhitespace;
  PRInt32 wordLen, contentLen;
  PRUnichar* bp = tx.GetNextWord(PR_TRUE, &wordLen, &contentLen,
                                 &isWhitespace);
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
