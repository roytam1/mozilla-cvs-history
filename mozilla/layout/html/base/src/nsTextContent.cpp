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
#include "nsHTMLParts.h"
#include "nsCRT.h"
#include "nsSplittableFrame.h"
#include "nsHTMLContent.h"
#include "nsString.h"
#include "nsIPresContext.h"
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
#include "nsBlockFrame.h"
#include "prtime.h"
#include "prprf.h"
#include "nsIDOMText.h"

#ifdef NS_DEBUG
#undef NOISY
#undef NOISY_BLINK
#else
#undef NOISY
#undef NOISY_BLINK
#endif

// XXX TODO:
// 0. tune justified text
// 1. add in a rendering method that can render justified text
// 2. text renderer should negotiate with rendering context/font
//    metrics to see what it can handle; for example, if the renderer can
//    automatically deal with underlining, strikethrough, justification,
//    etc, then the text renderer should let the rc do the work;
//    otherwise there should be XP fallback code here.

// XXX Speedup ideas
// 1. justified text can use word width information during resize reflows
// 2. when we are doing an unconstrained reflow we know we are going to
// get reflowed again; collect up the word widths we are computing as we
// do this and then save them in the mWords; later on when we get reflowed
// again we can destroy them
// 3. when pulling up text get word width information from next-in-flow

// XXX temporary
#define XP_IS_SPACE(_ch) \
  (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n'))

// XXX need more of this in nsIFontMetrics.GetWidth
#define CH_NBSP 160

// XXX use a PreTextFrame for pre-formatted text?

#if 0
static NS_DEFINE_IID(kITextContentIID, NS_ITEXTCONTENT_IID);
#endif
static NS_DEFINE_IID(kStyleMoleculeSID, NS_STYLEMOLECULE_SID);
static NS_DEFINE_IID(kStyleFontSID, NS_STYLEFONT_SID);
static NS_DEFINE_IID(kStyleColorSID, NS_STYLECOLOR_SID);
static NS_DEFINE_IID(kIScriptObjectOwner, NS_ISCRIPTOBJECTOWNER_IID);

class TextFrame;

class TextTimer : public nsITimerCallback {
public:
  TextTimer();
  ~TextTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(TextFrame* aFrame);

  PRBool RemoveFrame(TextFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  virtual void Notify(nsITimer *timer);

  nsITimer* mTimer;
  nsVoidArray mFrames;
};

class TextFrame : public nsSplittableFrame {
public:
  TextFrame(nsIContent* aContent,
            PRInt32 aIndexInParent,
            nsIFrame* aParentFrame);

  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD ResizeReflow(nsIPresContext* aCX,
                          nsReflowMetrics& aDesiredSize,
                          const nsSize& aMaxSize,
                          nsSize* aMaxElementSize,
                          ReflowStatus& aStatus);

  NS_IMETHOD GetReflowMetrics(nsIPresContext*  aPresContext,
                              nsReflowMetrics& aMetrics);

  NS_IMETHOD JustifyReflow(nsIPresContext* aCX,
                           nscoord aAvailableSpace);

  NS_IMETHOD GetCursorAt(nsIPresContext& aPresContext,
                         const nsPoint& aPoint,
                         nsIFrame** aFrame,
                         PRInt32& aCursor);

  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;

protected:
  virtual ~TextFrame();

  void PaintRegularText(nsIRenderingContext& aRenderingContext,
                        const nsRect& aDirtyRect,
                        nscoord dx, nscoord dy);

  void PaintJustifiedText(nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect,
                          nscoord dx, nscoord dy);

  ReflowStatus ReflowPre(nsIPresContext* aCX,
                         nsReflowMetrics& aDesiredSize,
                         const nsSize& aMaxSize,
                         nsSize* aMaxElementSize,
                         nsStyleFont& aFont,
                         PRInt32 aStartingOffset,
                         nsBlockReflowState* aState);

  ReflowStatus ReflowNormal(nsIPresContext* aCX,
                            nsReflowMetrics& aDesiredSize,
                            const nsSize& aMaxSize,
                            nsSize* aMaxElementSize,
                            nsStyleFont& aFont,
                            nsStyleMolecule& aMol,
                            PRInt32 aStartingOffset,
                            nsBlockReflowState* aState);

public:
  PRInt32 mContentOffset;
  PRInt32 mContentLength;
  PRUint16 mFlags;
  PRUint16 mColumn;

  // When this text frame is justified, this pointer is not null
  // and points to data about each word/space in the text
  PRUint32* mWords;
#ifdef DEBUG_JUSTIFY
  PRInt32 mNumWords;
#endif
};

// Flag information used by rendering code. This information is
// computed by the ResizeReflow code. Remaining bits are used by the
// tab count.
#define TEXT_SKIP_LEADING_WS    0x1
#define TEXT_HAS_MULTIBYTE      0x2
#define TEXT_IS_PRE             0x4
#define TEXT_BLINK_ON           0x8

#define TEXT_GET_TAB_COUNT(_mf) ((_mf) >> 4)
#define TEXT_SET_TAB_COUNT(_mf,_tabs) (_mf) = (_mf) | ((_tabs)<< 4)

// mWords bit definitions
#define WORD_IS_WORD                    0x80000000L
#define WORD_IS_SPACE                   0x00000000L
#define WORD_LENGTH_MASK                0x000000FFL
#define WORD_LENGTH_SHIFT               0
#define WORD_WIDTH_MASK                 0x7FFFFF00L
#define WORD_WIDTH_SHIFT                8
#define MAX_WORD_LENGTH                 256
#define MAX_WORD_WIDTH                  0x7FFFFFL

#define GET_WORD_LENGTH(_wi) \
  ((PRIntn) (((_wi) & WORD_LENGTH_MASK)/* >> WORD_LENGTH_SHIFT*/))
#define GET_WORD_WIDTH(_wi) \
  ((nscoord) ((PRUint32(_wi) & WORD_WIDTH_MASK) >> WORD_WIDTH_SHIFT))

class Text : public nsHTMLContent, public nsIDOMText {
public:
  Text(const PRUnichar* aText, PRInt32 aLength);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

#if 0
  virtual PRInt32 GetLength();
  virtual void GetText(nsStringBuf* aBuf, PRInt32 aOffset, PRInt32 aCount);
#endif

  virtual void List(FILE* out, PRInt32 aIndent) const;

  virtual void ToHTMLString(nsString& aBuf) const;

  virtual nsIFrame* CreateFrame(nsIPresContext* aCX,
                                PRInt32 aIndexInParent,
                                nsIFrame* aParentFrame);

  // nsIScriptObjectOwner interface
  virtual nsresult  GetScriptObject(JSContext *aContext, void** aScriptObject);

  // nsIDOMText interface
  virtual nsresult            GetNodeType(PRInt32 *aType);
  virtual nsresult            GetParentNode(nsIDOMNode **aNode);
  virtual nsresult            GetChildNodes(nsIDOMNodeIterator **aIterator);
  virtual nsresult            HasChildNodes();
  virtual nsresult            GetFirstChild(nsIDOMNode **aNode);
  virtual nsresult            GetPreviousSibling(nsIDOMNode **aNode);
  virtual nsresult            GetNextSibling(nsIDOMNode **aNode);
  virtual nsresult            InsertBefore(nsIDOMNode *newChild, nsIDOMNode *refChild);
  virtual nsresult            ReplaceChild(nsIDOMNode *newChild, 
                                            nsIDOMNode *oldChild);
  virtual nsresult            RemoveChild(nsIDOMNode *oldChild);
  virtual nsresult            GetData(nsString &aString);
  virtual nsresult            SetData(nsString &aString);
  virtual nsresult            Append(nsString &aData);
  virtual nsresult            Insert(int offset, nsString &aData);
  virtual nsresult            Delete(int offset, int count);
  virtual nsresult            Replace(int offset, int count, nsString &aData);
  virtual nsresult            Splice(nsIDOMElement *element, int offset, int count);

  void ToCString(nsString& aBuf, PRInt32 aOffset, PRInt32 aLen) const;

  PRUnichar* mText;
  PRInt32 mLength;

protected:
  Text();
  virtual ~Text();
};

//----------------------------------------------------------------------

static PRBool gBlinkTextOff;
static TextTimer* gTextBlinker;
#ifdef NOISY_BLINK
static PRTime gLastTick;
#endif

TextTimer::TextTimer()
{
  NS_INIT_REFCNT();
  mTimer = nsnull;
}

TextTimer::~TextTimer()
{
  Stop();
}

void TextTimer::Start()
{
  nsresult rv = NS_NewTimer(&mTimer);
  if (NS_OK == rv) {
    mTimer->Init(this, 750);
  }
}

void TextTimer::Stop()
{
  NS_IF_RELEASE(mTimer);
}

static NS_DEFINE_IID(kITimerCallbackIID, NS_ITIMERCALLBACK_IID);
NS_IMPL_ISUPPORTS(TextTimer, kITimerCallbackIID);

void TextTimer::AddFrame(TextFrame* aFrame) {
  mFrames.AppendElement(aFrame);
  if (1 == mFrames.Count()) {
    Start();
  }
printf("%d blinking frames [add %p]\n", mFrames.Count(), aFrame);
}

PRBool TextTimer::RemoveFrame(TextFrame* aFrame) {
  PRBool rv = mFrames.RemoveElement(aFrame);
  if (0 == mFrames.Count()) {
    Stop();
  }
printf("%d blinking frames [remove %p] [%s]\n",
       mFrames.Count(), aFrame, rv ? "true" : "FALSE");
  return rv;
}

PRInt32 TextTimer::FrameCount() {
  return mFrames.Count();
}

void TextTimer::Notify(nsITimer *timer)
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
    TextFrame* text = (TextFrame*) mFrames.ElementAt(i);

    // Determine damaged area and tell view manager to redraw it
    nsPoint offset;
    nsRect bounds;
    text->GetRect(bounds);
    nsIView* view;
    text->GetOffsetFromView(offset, view);
    nsIViewManager* vm = view->GetViewManager();
    bounds.x = offset.x;
    bounds.y = offset.y;
    vm->UpdateView(view, &bounds, 0);
    NS_RELEASE(vm);
    NS_RELEASE(view);
  }
}

//----------------------------------------------------------------------

TextFrame::TextFrame(nsIContent* aContent,
                     PRInt32 aIndexInParent,
                     nsIFrame* aParentFrame)
  : nsSplittableFrame(aContent, aIndexInParent, aParentFrame)
{
  if (nsnull == gTextBlinker) {
    // Create text timer the first time out
    gTextBlinker = new TextTimer();
  }
  NS_ADDREF(gTextBlinker);
}

TextFrame::~TextFrame()
{
  if (0 != (mFlags & TEXT_BLINK_ON)) {
    NS_ASSERTION(nsnull != gTextBlinker, "corrupted blinker");
    gTextBlinker->RemoveFrame(this);
  }
  if (0 == gTextBlinker->Release()) {
    // Release text timer when the last text frame is gone
    gTextBlinker = nsnull;
  }
  if (nsnull != mWords) {
    delete mWords;
  }
}

NS_METHOD TextFrame::GetCursorAt(nsIPresContext& aPresContext,
                                 const nsPoint& aPoint,
                                 nsIFrame** aFrame,
                                 PRInt32& aCursor)
{
  nsStyleMolecule* mol = (nsStyleMolecule*)
    mStyleContext->GetData(kStyleMoleculeSID);
  if (mol->cursor != NS_STYLE_CURSOR_INHERIT) {
    // If this container has a particular cursor, use it, otherwise
    // let the child decide.
    *aFrame = this;
  }
  aCursor = (PRInt32) mol->cursor;
  return NS_OK;
}

// XXX it would be nice to use a method pointer here, but I don't want
// to pay for the storage.

NS_METHOD TextFrame::Paint(nsIPresContext& aPresContext,
                           nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  if ((0 != (mFlags & TEXT_BLINK_ON)) && gBlinkTextOff) {
    return NS_OK;
  }

  // Get style data
  nsStyleColor* color =
    (nsStyleColor*)mStyleContext->GetData(kStyleColorSID);
  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);

  // Set font and color
  aRenderingContext.SetColor(color->mColor);
  aRenderingContext.SetFont(font->mFont);

  if (nsnull != mWords) {
    PaintJustifiedText(aRenderingContext, aDirtyRect, 0, 0);
    if (font->mThreeD) {
      nscoord onePixel = nscoord(1.0f * aPresContext.GetPixelsToTwips());
      aRenderingContext.SetColor(color->mBackgroundColor);
      PaintJustifiedText(aRenderingContext, aDirtyRect, onePixel, onePixel);
    }
    return NS_OK;
  }
  PaintRegularText(aRenderingContext, aDirtyRect, 0, 0);
  if (font->mThreeD) {
    nscoord onePixel = nscoord(1.0f * aPresContext.GetPixelsToTwips());
    aRenderingContext.SetColor(color->mBackgroundColor);
    PaintRegularText(aRenderingContext, aDirtyRect, onePixel, onePixel);
  }

  return NS_OK;
}

void TextFrame::PaintRegularText(nsIRenderingContext& aRenderingContext,
                                 const nsRect& aDirtyRect,
                                 nscoord dx, nscoord dy)
{
  // Skip leading space if necessary
  Text* txt = (Text*) mContent;
  const PRUnichar* cp = txt->mText + mContentOffset;
  const PRUnichar* end = cp + mContentLength;
  if ((mFlags & TEXT_SKIP_LEADING_WS) != 0) {
    while (cp < end) {
      PRUnichar ch = *cp++;
      if (!XP_IS_SPACE(ch)) {
        cp--;
        break;
      }
    }
  }

  if ((mFlags & TEXT_IS_PRE) != 0) {
    if ((mFlags & TEXT_HAS_MULTIBYTE) != 0) {
      // XXX to be written
    } else {
      PRIntn tabs = TEXT_GET_TAB_COUNT(mFlags);

      // Make a copy of the text we are to render, translating tabs
      // into whitespace.
      char buf[100];
      char* s = buf;
      char* s0 = s;
      PRInt32 maxLen = (end - cp) + 8*tabs;
      if (maxLen > sizeof(buf)) {
        s0 = s = new char[maxLen];
      }

      // Translate tabs into whitespace; translate other whitespace into
      // spaces.
      PRIntn col = (PRIntn) mColumn;
      while (cp < end) {
        PRUint8 ch = (PRUint8) *cp++;
        if (XP_IS_SPACE(ch)) {
          if (ch == '\t') {
            PRIntn spaces = 8 - (col & 7);
            col += spaces;
            while (--spaces >= 0) {
              *s++ = ' ';
            }
            continue;
          } else {
            *s++ = ' ';
          }
        } else if (ch == CH_NBSP) {
          *s++ = ' ';
        } else {
          *s++ = ch;
        }
        col++;
      }

      // Strip trailing whitespace
      if (s != s0) {
        if (s[-1] == ' ') {
          s--;
        }
      }

      // Render the text
      aRenderingContext.DrawString(s0, s - s0, dx, dy, mRect.width);

      if (s0 != buf) {
        delete s0;
      }
    }
  } else {
    if ((mFlags & TEXT_HAS_MULTIBYTE) != 0) {
      // XXX to be written
    } else {
      // Make a copy of the text we are to render, compressing out
      // whitespace; translating whitespace to literal spaces;
      // eliminating trailing whitespace.
      char buf[100];
      char* s = buf;
      char* s0 = s;
      PRInt32 maxLen = end - cp;
      if (maxLen > sizeof(buf)) {
        s0 = s = new char[maxLen];
      }

      // Compress down the whitespace
      while (cp < end) {
        PRUint8 ch = (PRUint8) *cp++;
        if (XP_IS_SPACE(ch)) {
          while (cp < end) {
            ch = (PRUint8) *cp++;
            if (!XP_IS_SPACE(ch)) {
              cp--;
              break;
            }
          }
          *s++ = ' ';
        } else {
          *s++ = ch;
        }
      }

      // Strip trailing whitespace
      if (s != s0) {
        if (s[-1] == ' ') {
          s--;
        }
      }

      // Render the text
      aRenderingContext.DrawString(s0, s - s0, dx, dy, mRect.width);

      if (s0 != buf) {
        delete s0;
      }
    }
  }
}

void TextFrame::PaintJustifiedText(nsIRenderingContext& aRenderingContext,
                                   const nsRect& aDirtyRect,
                                   nscoord dx, nscoord dy)
{
  Text* txt = (Text*) mContent;
  const PRUnichar* cp = txt->mText + mContentOffset;
  const PRUnichar* end = cp + mContentLength;

  // Get a word buffer that's big enough to hold all of the text in
  // case all of the text happens to be a word (not possible given how
  // the justification workds, but it's the fastest check that gets us
  // a big enough buffer). We only do this if we aren't going to use
  // multibyte rendering.
  char buf[100];
  char* s0 = buf;
  if ((mFlags & TEXT_HAS_MULTIBYTE) == 0) {
    PRInt32 maxLen = end - cp;
    if (maxLen > sizeof(buf)) {
      s0 = new char[maxLen];
    }
  }

  // XXX justified text doesn't render underlines (etc) over spaces
  // while regular text does...fix this?
  PRUint32* wp = mWords;
  PRInt32 total = mContentLength;
  nscoord x = dx;
#ifdef DEBUG_JUSTIFY
  PRIntn numWords = mNumWords;
#endif
  while (total != 0) {
#ifdef DEBUG_JUSTIFY
    NS_ASSERTION(numWords > 0, "bad word data");
#endif
    PRUint32 wordInfo = *wp++;
    nscoord wordWidth = GET_WORD_WIDTH(wordInfo);
    PRIntn wordLen = GET_WORD_LENGTH(wordInfo);
    if (wordInfo & WORD_IS_WORD) {
      if (mFlags & TEXT_HAS_MULTIBYTE) {
        aRenderingContext.DrawString(cp, wordLen, x, dy, mRect.width);
        cp += wordLen;
      } else {
        char* s = s0;
        char* es = s + wordLen;
        while (s < es) {
          PRUint8 ch = (PRUint8) *cp++;
          *s++ = ch;
        }
        aRenderingContext.DrawString(s0, s - s0, x, dy, mRect.width);
      }
    } else {
      // skip over space characters in text array
      cp += wordLen;
    }
    x += wordWidth;
    total -= wordLen;
#ifdef DEBUG_JUSTIFY
    --numWords;
    NS_ASSERTION(total >= 0, "bad word data");
#endif
  }

  if (s0 != buf) {
    delete s0;
  }
}

NS_METHOD TextFrame::GetReflowMetrics(nsIPresContext* aCX,
                                      nsReflowMetrics& aMetrics)
{
  aMetrics.width = mRect.width;
  aMetrics.height = mRect.height;

  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);
  nsIFontMetrics* fm = aCX->GetMetricsFor(font->mFont);
  aMetrics.ascent = fm->GetMaxAscent();
  aMetrics.descent = fm->GetMaxDescent();
  NS_RELEASE(fm);
  return NS_OK;
}

NS_METHOD TextFrame::ResizeReflow(nsIPresContext* aCX,
                                  nsReflowMetrics& aDesiredSize,
                                  const nsSize& aMaxSize,
                                  nsSize* aMaxElementSize,
                                  ReflowStatus& aStatus)
{
#ifdef NOISY
  ListTag(stdout);
  printf(": resize reflow into %g,%g\n",
         NS_TWIPS_TO_POINTS_FLOAT(aMaxSize.width),
         NS_TWIPS_TO_POINTS_FLOAT(aMaxSize.height));
#endif

  // Wipe out old justification information since it's going to change
  if (nsnull != mWords) {
    delete mWords;
    mWords = nsnull;
  }
#ifdef DEBUG_JUSTIFY
  mNumWords = 0;
#endif

  // Get starting offset into the content
  PRInt32 startingOffset = 0;
  if (nsnull != mPrevInFlow) {
    TextFrame* prev = (TextFrame*) mPrevInFlow;
    startingOffset = prev->mContentOffset + prev->mContentLength;
  }

  // Get cached state for containing block frame
  nsBlockReflowState* state = nsnull;
  nsIFrame* parent = mGeometricParent;
  while (nsnull != parent) {
    nsIHTMLFrameType* ft;
    nsresult status = parent->QueryInterface(kIHTMLFrameTypeIID, (void**) &ft);
    if (NS_OK == status) {
      nsHTMLFrameType type = ft->GetFrameType();
      if (eHTMLFrame_Block == type) {
        break;
      }
    }
    parent->GetGeometricParent(parent);
  }
  if (nsnull != parent) {
    nsIPresShell* shell = aCX->GetShell();
    state = (nsBlockReflowState*) shell->GetCachedData(parent);
    NS_RELEASE(shell);
  }

  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);

  // Initialize mFlags (without destroying the TEXT_BLINK_ON bit) bits
  // that are filled in by the reflow routines.
  mFlags &= TEXT_BLINK_ON;
  if (font->mFont.decorations & NS_STYLE_TEXT_DECORATION_BLINK) {
    if (0 == (mFlags & TEXT_BLINK_ON)) {
      mFlags |= TEXT_BLINK_ON;
      gTextBlinker->AddFrame(this);
    }
  }

  nsStyleMolecule* mol =
    (nsStyleMolecule*)mStyleContext->GetData(kStyleMoleculeSID);

  if (NS_STYLE_WHITESPACE_PRE == mol->whiteSpace) {
    // Use a specialized routine for pre-formatted text
    aStatus = ReflowPre(aCX, aDesiredSize, aMaxSize,
                       aMaxElementSize, *font, startingOffset, state);
  } else {
    // Use normal wrapping routine for non-pre text (this includes
    // text that is not wrapping)
    aStatus = ReflowNormal(aCX, aDesiredSize, aMaxSize,
                          aMaxElementSize, *font, *mol, startingOffset, state);
  }

#ifdef NOISY
  ListTag(stdout);
  printf(": reflow %scomplete [flags=%x]\n",
         ((aStatus == frComplete) ? "" : "not "), mFlags);
#endif
  return NS_OK;
}

// Reflow normal text (stuff that doesn't have to deal with horizontal
// tabs). Normal text reflow may or may not wrap depending on the
// "whiteSpace" style property.
nsIFrame::ReflowStatus
TextFrame::ReflowNormal(nsIPresContext* aCX,
                        nsReflowMetrics& aDesiredSize,
                        const nsSize& aMaxSize,
                        nsSize* aMaxElementSize,
                        nsStyleFont& aFont,
                        nsStyleMolecule& aMol,
                        PRInt32 aStartingOffset,
                        nsBlockReflowState* aState)
{
  Text* txt = (Text*) mContent;
  const PRUnichar* cp = txt->mText + aStartingOffset;
  const PRUnichar* end = cp + txt->mLength - aStartingOffset;
  const PRUnichar* cpStart = cp;
  mContentOffset = aStartingOffset;

  nsIFontMetrics* fm = aCX->GetMetricsFor(aFont.mFont);
  PRInt32 spaceWidth = fm->GetWidth(' ');
  PRBool atLeftMargin = PR_TRUE;
  PRBool wrapping = PR_TRUE;
  if (NS_STYLE_WHITESPACE_NORMAL != aMol.whiteSpace) {
    wrapping = PR_FALSE;
  }

  // Set whitespace skip flag
  PRBool skipWhitespace = PR_FALSE;
  if (nsnull != aState) {
    if (!aState->allowLeadingWhitespace) {
      skipWhitespace = PR_TRUE;
      mFlags |= TEXT_SKIP_LEADING_WS;
    }
  }

  // Try to fit as much of the text as possible. Note that if we are
  // at the left margin then the first word always fits. In addition,
  // we compute the size of the largest word that we contain. If we
  // end up containing nothing (because there isn't enough space for
  // the first word) then we still compute the size of that first
  // non-fitting word.

  // XXX XP_IS_SPACE must not return true for the unicode &nbsp character
  // XXX what about &zwj and it's cousins?
  nscoord x = 0;
  nscoord maxWidth = aMaxSize.width;
  nscoord width = 0;
  nscoord maxWordWidth = 0;
  const PRUnichar* lastWordEnd = cpStart;
  PRBool hasMultibyte = PR_FALSE;
  PRBool endsInWhitespace = PR_FALSE;
  while (cp < end) {
    PRUnichar ch = *cp++;
    PRBool isWhitespace;
    if (XP_IS_SPACE(ch)) {
      // Compress whitespace down to a single whitespace
      while (cp < end) {
        ch = *cp;
        if (XP_IS_SPACE(ch)) {
          cp++;
          continue;
        }
        break;
      }
      if (skipWhitespace) {
        skipWhitespace = PR_FALSE;
        continue;
      }
      width = spaceWidth;
      isWhitespace = PR_TRUE;
    } else {
      // The character is not a space character. Find the end of the
      // word and then measure it.
      if (ch >= 256) {
        hasMultibyte = PR_TRUE;
      }
      const PRUnichar* wordStart = cp - 1;
      while (cp < end) {
        ch = *cp;
        if (ch >= 256) {
          hasMultibyte = PR_TRUE;
        }
        if (!XP_IS_SPACE(ch)) {
          cp++;
          continue;
        }
        break;
      }
      width = fm->GetWidth(wordStart, PRUint32(cp - wordStart));
      skipWhitespace = PR_FALSE;
      isWhitespace = PR_FALSE;
    }

    // Now that we have the end of the word or whitespace, see if it
    // will fit.
    if (!atLeftMargin && wrapping && (x + width > maxWidth)) {
      // The word/whitespace will not fit.
      cp = lastWordEnd;
      if (x == 0) {
        // Nothing fit at all. In this case, we still may want to know
        // the maxWordWidth so compute it right now.
        maxWordWidth = width;
      }
      break;
    }

    // The word fits. Add it to the run of text.
    x += width;
    if (width > maxWordWidth) {
      maxWordWidth = width;
    }
    atLeftMargin = PR_FALSE;
    lastWordEnd = cp;
    width = 0;
    endsInWhitespace = isWhitespace;
  }
  if (hasMultibyte) {
    mFlags |= TEXT_HAS_MULTIBYTE;
  }

  if (nsnull != aState) {
    if (0 == width) {
      // Since we collapsed into nothingness (all our whitespace
      // is ignored) leave the aState->allowLeadingWhitespace
      // flag alone since it doesn't want leading whitespace
    } else {
      aState->allowLeadingWhitespace = !endsInWhitespace;
    }
  }

  // Now we know our content length
  mContentLength = lastWordEnd - cpStart;
  if (0 == mContentLength) {
    if (cp == end) {
      // The entire chunk of text was whitespace that we skipped over.
      aDesiredSize.width = 0;
      aDesiredSize.height = 0;
      aDesiredSize.ascent = 0;
      aDesiredSize.descent = 0;
      mContentLength = end - cpStart;
      if (nsnull != aMaxElementSize) {
        aMaxElementSize->width = 0;
        aMaxElementSize->height = 0;
      }
      NS_RELEASE(fm);
      return frComplete;
    }
  }

  // Set desired size to the computed size
  aDesiredSize.width = x;
  aDesiredSize.height = fm->GetHeight();
  aDesiredSize.ascent = fm->GetMaxAscent();
  aDesiredSize.descent = fm->GetMaxDescent();
  if (nsnull != aMaxElementSize) {
    aMaxElementSize->width = maxWordWidth;
    aMaxElementSize->height = fm->GetHeight();
  }
  NS_RELEASE(fm);
  return (cp == end) ? frComplete : frNotComplete;
}

nsIFrame::ReflowStatus
TextFrame::ReflowPre(nsIPresContext* aCX,
                     nsReflowMetrics& aDesiredSize,
                     const nsSize& aMaxSize,
                     nsSize* aMaxElementSize,
                     nsStyleFont& aFont,
                     PRInt32 aStartingOffset,
                     nsBlockReflowState* aState)
{
  Text* txt = (Text*) mContent;
  const PRUnichar* cp = txt->mText + aStartingOffset;
  const PRUnichar* cpStart = cp;
  const PRUnichar* end = cp + txt->mLength - aStartingOffset;

  mFlags |= TEXT_IS_PRE;
  nsIFontMetrics* fm = aCX->GetMetricsFor(aFont.mFont);
  const PRInt32* widths = fm->GetWidths();
  PRInt32 width = 0;
  PRBool hasMultibyte = PR_FALSE;
  PRUint16 tabs = 0;
  PRIntn col = 0;
  if (nsnull != aState) {
    col = aState->column;
  }
  mColumn = (PRUint16) col;
  nscoord spaceWidth = widths[' '];
  while (cp < end) {
    PRUnichar ch = *cp++;
    if (ch == '\n') {
      if (nsnull != aState) {
        aState->breakAfterChild = PR_TRUE;
      }
      break;
    }
    if (ch == '\t') {
      // Advance to next tab stop
      PRIntn spaces = 8 - (col & 7);
      width += spaces * spaceWidth;
      col += spaces;
      tabs++;
      continue;
    }
    if (ch == CH_NBSP) {
      width += spaceWidth;
      col++;
      continue;
    }
    if (ch < 256) {
      width += widths[ch];
    } else {
      widths += fm->GetWidth(ch);
      hasMultibyte = PR_TRUE;
    }
    col++;
  }
  if (nsnull != aState) {
    aState->column = col;
  }
  if (hasMultibyte) {
    mFlags |= TEXT_HAS_MULTIBYTE;
  }
  TEXT_SET_TAB_COUNT(mFlags, tabs);

  mContentOffset = aStartingOffset;
  mContentLength = cp - cpStart;
  aDesiredSize.width = width;
  aDesiredSize.height = fm->GetHeight();
  aDesiredSize.ascent = fm->GetMaxAscent();
  aDesiredSize.descent = fm->GetMaxDescent();
  if (nsnull != aMaxElementSize) {
    aMaxElementSize->width = aDesiredSize.width;
    aMaxElementSize->height = aDesiredSize.height;
  }
  NS_RELEASE(fm);
  return (cp == end) ? frComplete : frNotComplete;
}

#define NUM_WORDS 20

NS_METHOD TextFrame::JustifyReflow(nsIPresContext* aCX, nscoord aAvailableSpace)
{
  if (mFlags & TEXT_IS_PRE) {
    // no way
    return NS_OK;
  }

  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);
  nsIFontMetrics* fm = aCX->GetMetricsFor(font->mFont);
  PRInt32 spaceWidth = fm->GetWidth(' ');

  PRUint32 words[NUM_WORDS];
  PRUint32* wp0 = words;
  PRIntn maxWords = NUM_WORDS;

  PRBool skipWhitespace = PRBool((mFlags & TEXT_SKIP_LEADING_WS) != 0);

  // Count the number of spaces and words in the text we are going to
  // justify. Also measure each word in the text and skip over leading
  // space and compress down whitespace.
  Text* txt = (Text*) mContent;
  const PRUnichar* cp = txt->mText + mContentOffset;
  const PRUnichar* cpStart = cp;
  const PRUnichar* end = cp + mContentLength;
  PRUint32* wp = wp0;
  PRUint32* wpend = wp0 + maxWords;
  PRIntn spaceCount = 0;
  PRIntn total = 0;
  while (cp < end) {
    if (wp == wpend) {
      // Make space for more words
      PRUint32 numWords = wp - wp0;
      maxWords = maxWords * 2;
      PRUint32* newwp0 = new PRUint32[maxWords];
      if (nsnull == newwp0) {
        goto bail;
      }
      nsCRT::memcpy(newwp0, wp0, sizeof(PRUint32) * numWords);
      if (wp0 != words) {
        delete wp0;
      }
      wp0 = newwp0;
      wp = wp0 + numWords;
      wpend = wp0 + maxWords;
    }

    PRUint32 wordInfo;
    PRUint32 wordLen;
    nscoord wordWidth;
    const PRUnichar* wordStart = cp;
    PRUnichar ch = *cp++;
    if (XP_IS_SPACE(ch)) {
      // Compress whitespace down to a single whitespace
      while (cp < end) {
        ch = *cp;
        if (XP_IS_SPACE(ch)) {
          cp++;
          continue;
        }
        break;
      }
      // Use wordWidth as a flag for spaces to indicate which spaces
      // we give zero space to and which we don't.
      if (skipWhitespace) {
        wordWidth = 0;
      } else {
        wordWidth = spaceWidth;
        spaceCount++;
      }
      wordInfo = WORD_IS_SPACE;
      wordLen = cp - wordStart;
    } else {
      while (cp < end) {
        ch = *cp;
        if (!XP_IS_SPACE(ch)) {
          cp++;
          continue;
        }
        break;
      }
      wordLen = cp - wordStart;
      wordWidth = fm->GetWidth(wordStart, wordLen);
      skipWhitespace = PR_FALSE;
      wordInfo = WORD_IS_WORD;
    }
    if ((wordWidth > MAX_WORD_WIDTH) || (wordLen > MAX_WORD_LENGTH)) {
      // We can't fit the information about this word into our
      // bitfield. Bail.
      goto bail;
    }
    wordInfo = wordInfo |
      (wordLen << WORD_LENGTH_SHIFT) |
      (wordWidth << WORD_WIDTH_SHIFT);
    *wp++ = wordInfo;
    total += wordLen;
  }
  NS_ASSERTION(total == mContentLength, "bad total");

  // Now that we know where all the words and spaces are, and we have
  // measured the words, divy up the aAvailableSpace to each of the
  // spaces.
  if (spaceCount != 0) {
    /*
     * See if the last word is a space. If it is, don't count it as a space
     * and give it's space to the available space.
     */
    PRUint32 wordInfo = wp[-1];
    if ((wordInfo & WORD_IS_WORD) == 0) {
      if (--spaceCount == 0) {
        // Never mind: the one and only space was at the end. Harumph.
        goto bail;
      }
      aAvailableSpace += spaceWidth;
      // Update wordInfo to have a zero width for the trailing space
      wp[-1] = WORD_IS_SPACE | (wordInfo & WORD_LENGTH_MASK);
    }
    nscoord add = aAvailableSpace / spaceCount;
    if (add == 0) {
      add = 1;
    }
    PRIntn numWords = wp - wp0;
    mWords = new PRUint32[numWords];
    if (nsnull == mWords) {
      goto bail;
    }
    wp = wp0;
    PRUint32* tp = mWords;
    PRUint32* tpend = tp + numWords;
    total = 0;
#ifdef DEBUG_JUSTIFY
    mNumWords = numWords;
#endif
    while (--numWords >= 0) {
      wordInfo = *wp++;
      if (wordInfo & WORD_IS_WORD) {
        // We already know about the word
        *tp++ = wordInfo;
        total += GET_WORD_LENGTH(wordInfo);
        continue;
      } else {
        nscoord spaceWidth = GET_WORD_WIDTH(wordInfo);
        if (spaceWidth == 0) {
          // This is leading space that doesn't count
          *tp++ = wordInfo;
          total += GET_WORD_LENGTH(wordInfo);
          continue;
        }
        // This is a space that gets some of the available space
        if (add > aAvailableSpace) {
          add = aAvailableSpace;
        }
        spaceWidth += add;
        aAvailableSpace -= add;
        if (aAvailableSpace == 0) {
          // We used it all up already so stop adding
          add = 0;
        }
        if (--spaceCount == 0) {
          // Give the last space the remaining available space
          spaceWidth += aAvailableSpace;
          aAvailableSpace = 0;
        }
        if (spaceWidth > MAX_WORD_WIDTH) {
          // Sad but true; leave it be. Maybe the next word can use it
          *tp++ = wordInfo;
          total += GET_WORD_LENGTH(wordInfo);
          continue;
        }
        wordInfo = (wordInfo & ~WORD_WIDTH_MASK) |
          (spaceWidth << WORD_WIDTH_SHIFT);
        *tp++ = wordInfo;
        total += GET_WORD_LENGTH(wordInfo);
      }
    }
    NS_ASSERTION(aAvailableSpace == 0, "bad divy up");
    NS_ASSERTION(tp == tpend, "bad tp");
    NS_ASSERTION(total == mContentLength, "bad total");
 }

bail:
  if (wp0 != words) {
    delete wp0;
  }
  NS_RELEASE(fm);
  return NS_OK;
}
#undef NUM_WORDS

NS_METHOD TextFrame::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "Text(%d)@%p[%d,%d] ", 
          mIndexInParent, this,
          mContentOffset, mContentOffset+mContentLength-1);
  out << mRect;
  aIndent++;

  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  Text* txt = (Text*) mContent;
  nsAutoString tmp;
  txt->ToCString(tmp, mContentOffset, mContentLength);
  fputs("\"", out);
  fputs(tmp, out);
  fputs("\"\n", out);

  aIndent--;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs(">\n", out);

#ifdef NOISY
  if (nsnull != mWords) {
    for (i = aIndent; --i >= 0; ) fputs("  ", out);
    fputs("  @words ", out);
    PRUint32* wp = mWords;
    PRInt32 total = mContentLength;
    while (total != 0) {
      PRUint32 wordInfo = *wp++;
      nscoord wordWidth = GET_WORD_WIDTH(wordInfo);
      PRIntn wordLen = GET_WORD_LENGTH(wordInfo);
      fprintf(out, "%c-%d-%d ",
              ((wordInfo & WORD_IS_WORD) ? 'w' : 's'),
              wordWidth, wordLen);
      total -= wordLen;
      NS_ASSERTION(total >= 0, "bad word data");
    }
    fputs("\n", out);
  }
#endif
  return NS_OK;
}

//----------------------------------------------------------------------
static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIDOMNodeIID, NS_IDOMNODE_IID);


Text::Text(const PRUnichar* aText, PRInt32 aLength)
{
  if (0 == aLength) {
    aLength = nsCRT::strlen(aText);
  }
  mLength = aLength;
  mText = new PRUnichar[aLength];
  nsCRT::memcpy(mText, aText, aLength * sizeof(PRUnichar));
}

Text::Text()
{
  mText = nsnull;
  mLength = 0;
}

Text::~Text()
{
  if (nsnull != mText) {
    delete mText;
    mText = nsnull;
  }
  mLength = 0;
}

nsresult Text::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(nsnull != aInstancePtr, "null pointer");
  
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(kIDOMTextIID)) {
    *aInstancePtr = (void*)(nsIDOMText*)this;
    nsHTMLContent::AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNodeIID)) {
    *aInstancePtr = (void*)(nsIDOMNode*)(nsIDOMText*)this;
    nsHTMLContent::AddRef();
    return NS_OK;
  }
  
  return nsHTMLContent::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt Text::AddRef(void)
{
  return nsHTMLContent::AddRef();
}

nsrefcnt Text::Release(void)
{
  return nsHTMLContent::Release();
}

void Text::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
  fputs("Text", out);
  ListAttributes(out);
  fprintf(out, " RefCnt=%d<\"", mRefCnt);

  nsAutoString tmp;
  ToCString(tmp, 0, mLength);
  fputs(tmp, out);

  fputs("\">\n", out);
}

void Text::ToHTMLString(nsString& aBuf) const
{
  aBuf.SetLength(0);
  aBuf.Append(mText, mLength);
}

void Text::ToCString(nsString& aBuf, PRInt32 aOffset, PRInt32 aLen) const
{
  PRUnichar* cp = mText + aOffset;
  PRUnichar* end = cp + aLen;
  while (cp < end) {
    PRUnichar ch = *cp++;
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

#if 0
// From nsITextContent; might want this later
PRInt32 Text::GetLength()
{
  return mLength;
}

void Text::GetText(nsString& aBuf, PRInt32 aOffset, PRInt32 aCount)
{
  aBuf.SetLength(0);
  if ((PRUint32(aOffset) >= PRUint32(mLength)) ||
      (aCount <= 0)) {
    return;
  }
  if (aOffset + aCount > mLength) {
    aCount = mLength - aOffset;
  }
  aBuf.Append(mText + aOffset, aCount);
}
#endif

nsIFrame* Text::CreateFrame(nsIPresContext* aCX,
                            PRInt32 aIndexInParent,
                            nsIFrame* aParentFrame)
{
  nsIFrame* rv = new TextFrame(this, aIndexInParent, aParentFrame);
  return rv;
}

nsresult Text::GetScriptObject(JSContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  if (nsnull == mScriptObject) {
    *aScriptObject = nsnull;
    if (nsnull != mParent) {
      nsIScriptObjectOwner *parent;
      if (NS_OK == mParent->QueryInterface(kIScriptObjectOwner, (void**)&parent)) {
        parent->GetScriptObject(aContext, aScriptObject);
        NS_RELEASE(parent);
      }
    }
    res = NS_NewScriptText(aContext, this, (JSObject*)*aScriptObject, (JSObject**)&mScriptObject);
  }
  *aScriptObject = mScriptObject;
  return res;
}

//
// nsIDOMText interface
//
nsresult Text::GetNodeType(PRInt32 *aType)
{
  *aType = nsHTMLContent::TEXT;
  return NS_OK;
}

nsresult Text::GetParentNode(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetParentNode(aNode);
}

nsresult Text::GetChildNodes(nsIDOMNodeIterator **aIterator)
{
  return nsHTMLContent::GetChildNodes(aIterator);
}

nsresult Text::HasChildNodes()
{
  return nsHTMLContent::HasChildNodes();
}

nsresult Text::GetFirstChild(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetFirstChild(aNode);
}

nsresult Text::GetPreviousSibling(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetPreviousSibling(aNode);
}

nsresult Text::GetNextSibling(nsIDOMNode **aNode)
{
  return nsHTMLContent::GetNextSibling(aNode);
}

nsresult Text::InsertBefore(nsIDOMNode *newChild, nsIDOMNode *refChild)
{
  return nsHTMLContent::InsertBefore(newChild, refChild);
}

nsresult Text::ReplaceChild(nsIDOMNode *newChild, nsIDOMNode *oldChild)
{
  return nsHTMLContent::ReplaceChild(newChild, oldChild);
}

nsresult Text::RemoveChild(nsIDOMNode *oldChild)
{
  return nsHTMLContent::RemoveChild(oldChild);
}

nsresult Text::GetData(nsString &aString)
{
  if (nsnull != mText) {
    aString.SetString(mText, mLength);
  }

  return NS_OK;
}

nsresult Text::SetData(nsString &aString)
{
  if (mText) delete[] mText;

  mLength = aString.Length();
  mText = aString.ToNewUnicode();
  return NS_OK;
}

nsresult Text::Append(nsString &aData)
{
  PRUint32 length = aData.Length();
  PRUnichar *text = new PRUnichar[mLength + length];
  nsCRT::memcpy(text, mText, mLength * sizeof(PRUnichar));
  nsCRT::memcpy(text + mLength * sizeof(PRUnichar), aData.GetUnicode(), length * sizeof(PRUnichar));

  if (mText) delete[] mText;
  mLength += length;
  mText = text;

  return NS_OK;
}

nsresult Text::Insert(int offset, nsString &aData)
{
  if (offset < mLength) {
    PRInt32 length = aData.Length();
    PRUnichar *text = new PRUnichar[mLength + length];
    PRUnichar *data = aData.GetUnicode();
    PRInt32 i;

    for (i = 0; i < offset; i++) {
      *(text++) = *(mText++);
    }
    for (i = 0; i < length; i++) {
      *(text++) = *(data++);
    }
    for (i = 0; i < mLength - offset; i++) {
      *(text++) = *(mText++);
    }

    if (mText) delete[] (mText - mLength);
    mLength += length;
    mText = text - mLength;
  }

  return NS_OK;
}

nsresult Text::Delete(int offset, int count)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult Text::Replace(int offset, int count, nsString &aData)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult Text::Splice(nsIDOMElement *element, int offset, int count)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
NS_NewHTMLText(nsIHTMLContent** aInstancePtrResult,
               const PRUnichar* us, PRInt32 uslen)
{
  nsIHTMLContent* it = new Text(us, uslen);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}

nsresult
NS_NewHTMLText(nsIHTMLContent** aInstancePtrResult,
               nsIArena* aArena, const PRUnichar* us, PRInt32 uslen)
{
  nsIHTMLContent* it = new(aArena) Text(us, uslen);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}

//----------------------------------------------------------------------

class SharedText : public Text {
public:
  SharedText(PRUnichar* aText, PRInt32 aLength);

protected:
  virtual ~SharedText();
};

SharedText::SharedText(PRUnichar* aText, PRInt32 aLength)
  : Text()
{
  if (0 == aLength) {
    aLength = nsCRT::strlen(aText);
  }
  mText = aText;
  mLength = aLength;
}

SharedText::~SharedText()
{
  mText = nsnull;
}

nsresult
NS_NewSharedHTMLText(nsIHTMLContent** aInstancePtrResult,
                     PRUnichar* us, PRInt32 uslen)
{
  nsIHTMLContent* it = new SharedText(us, uslen);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}

nsresult
NS_NewSharedHTMLText(nsIHTMLContent** aInstancePtrResult,
                     nsIArena* aArena, PRUnichar* us, PRInt32 uslen)
{
  nsIHTMLContent* it = new(aArena) SharedText(us, uslen);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}
