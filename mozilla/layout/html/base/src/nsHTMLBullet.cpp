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
#include "nsHTMLTagContent.h"
#include "nsLeafFrame.h"
#include "nsBlockFrame.h"
#include "nsIInlineReflow.h"
#include "nsLineLayout.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLImage.h"
#include "prprf.h"
#include "nsHTMLBase.h"
#include "nsIView.h"

// XXX eliminate the bullet numbering hackery:

// 1) use a dependency graph of some sort that is updated when LI's
// come and go in the content model (how do we invalidate it when a
// content change occurs?): maybe for block's that are display:
// list-item, they keep a side data structure that is the ordinal data
// for it's children LI's?

// 2) Alternatively, have the LI's number themselves automatically and
// use a GetAttribute on the content to discover the correct number (N^2)

// XXX Also eliminate the way that we find the
// nsCSSBlockReflowState!!! It's bogus beyond belief! An ISA
// QueryInterface on the reflow-state should do the trick.

class Bullet : public nsHTMLTagContent {
public:
  Bullet();

  NS_IMETHOD IsSynthetic(PRBool& aResult);

  NS_IMETHOD GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const;

  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;

  NS_IMETHOD CreateFrame(nsIPresContext* aPresContext,
                         nsIFrame* aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*& aResult);
};

class BulletFrame : public nsFrame, private nsIInlineReflow {
public:
  BulletFrame(nsIContent* aContent, nsIFrame* aParentFrame);
  virtual ~BulletFrame();

  // nsISupports
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  // nsIFrame
  NS_IMETHOD DeleteFrame(nsIPresContext& aPresContext);
  NS_IMETHOD Paint(nsIPresContext &aCX,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);
  NS_IMETHOD ListTag(FILE* out) const;
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;

  // nsIInlineReflow
  NS_IMETHOD FindTextRuns(nsLineLayout&     aLineLayout,
                          nsIReflowCommand* aReflowCommand);
  NS_IMETHOD InlineReflow(nsLineLayout&        aLineLayout,
                          nsReflowMetrics&     aMetrics,
                          const nsReflowState& aReflowState);

protected:
  void GetDesiredSize(nsIPresContext*     aPresContext,
                      nsBlockReflowState* aBlockState,
                      const nsReflowState&   aReflowState,
                      nsReflowMetrics&       aMetrics);

  PRInt32 GetListItemOrdinal(nsIPresContext*     aCX,
                             nsBlockReflowState* aBlockState,
                             const nsStyleList&     aMol);

  void GetListItemText(nsIPresContext*     aCX,
                       nsBlockReflowState* aBlockState,
                       const nsStyleList&     aMol,
                       nsString&              aResult);

  nsBlockReflowState*
    GetListContainerReflowState(nsIPresContext*      aCX,
                                const nsReflowState& aReflowState);

  PRPackedBool mOrdinalValid;
  PRInt32 mOrdinal;
  nsMargin mPadding;
  nsHTMLImageLoader mImageLoader;
};

//----------------------------------------------------------------------

Bullet::Bullet()
{
}

static void
MapAttributesInto(nsIHTMLAttributes* aAttributes,
                  nsIStyleContext* aContext,
                  nsIPresContext* aPresContext)
{
  nsStyleDisplay* display = (nsStyleDisplay*)
    aContext->GetMutableStyleData(eStyleStruct_Display);
  display->mDisplay = NS_STYLE_DISPLAY_INLINE;
}

NS_IMETHODIMP
Bullet::GetAttributeMappingFunction(nsMapAttributesFunc& aMapFunc) const
{
  aMapFunc = &MapAttributesInto;
  return NS_OK;
}

NS_IMETHODIMP
Bullet::IsSynthetic(PRBool& aResult)
{
  aResult = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
Bullet::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 i = aIndent; --i >= 0; ) fputs("  ", out);
  fprintf(out, "Bullet RefCnt=%d<>\n", mRefCnt);
  return NS_OK;
}

NS_IMETHODIMP
Bullet::CreateFrame(nsIPresContext*  aPresContext,
                    nsIFrame*        aParentFrame,
                    nsIStyleContext* aStyleContext,
                    nsIFrame*&       aResult)
{
  BulletFrame* frame = new BulletFrame(this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = frame;
  frame->SetStyleContext(aPresContext, aStyleContext);
  return NS_OK;
}

nsresult
NS_NewHTMLBullet(nsIHTMLContent** aInstancePtrResult)
{
  Bullet* it = new Bullet();
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}

nsresult
NS_NewBulletFrame(nsIContent* aContent, nsIFrame* aParentFrame,
                  nsIFrame*& aResult)
{
  nsIFrame* frame = new BulletFrame(aContent, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = frame;
  return NS_OK;
}

//----------------------------------------------------------------------

BulletFrame::BulletFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsFrame(aContent, aParentFrame)
{
}

BulletFrame::~BulletFrame()
{
}

NS_IMETHODIMP
BulletFrame::QueryInterface(REFNSIID aIID, void** aInstancePtrResult)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null pointer");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIInlineReflowIID)) {
    *aInstancePtrResult = (void*) ((nsIInlineReflow*)this);
    return NS_OK;
  }
  return nsFrame::QueryInterface(aIID, aInstancePtrResult);
}

NS_METHOD
BulletFrame::DeleteFrame(nsIPresContext& aPresContext)
{
  // Release image loader first so that it's refcnt can go to zero
  mImageLoader.DestroyLoader();
  return nsFrame::DeleteFrame(aPresContext);
}

NS_IMETHODIMP
BulletFrame::ListTag(FILE* out) const
{
  PRInt32 contentIndex;
  GetContentIndex(contentIndex);
  fprintf(out, "Bullet(%d)@%p", contentIndex, this);
  return NS_OK;
}

NS_METHOD
BulletFrame::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 i;
  for (i = aIndent; --i >= 0; ) fputs("  ", out);
  PRInt32 contentIndex;
  GetContentIndex(contentIndex);
  fprintf(out, "Bullet(%d)@%p ", 
          contentIndex, this);
  nsIView* view;
  GetView(view);
  if (nsnull != view) {
    fprintf(out, " [view=%p]", view);
  }

  out << mRect;
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fputs("<>\n", out);
  return NS_OK;
}

NS_METHOD
BulletFrame::Paint(nsIPresContext&      aCX,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect)
{
  const nsStyleDisplay* disp =
    (const nsStyleDisplay*)mStyleContext->GetStyleData(eStyleStruct_Display);
  nscoord width;

  if (disp->mVisible) {
    const nsStyleList* myList =
      (const nsStyleList*)mStyleContext->GetStyleData(eStyleStruct_List);

    if (myList->mListStyleImage.Length() > 0) {
      nsIImage* image = mImageLoader.GetImage();
      if (nsnull == image) {
        if (!mImageLoader.GetLoadImageFailed()) {
          // No image yet
          return NS_OK;
        }
      }
      else {
        nsRect innerArea(mPadding.left, mPadding.top,
                         mRect.width - (mPadding.left + mPadding.right),
                         mRect.height - (mPadding.top + mPadding.bottom));
        aRenderingContext.DrawImage(image, innerArea);
        return NS_OK;
      }
    }

    const nsStyleFont* myFont =
      (const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font);
    const nsStyleColor* myColor =
      (const nsStyleColor*)mStyleContext->GetStyleData(eStyleStruct_Color);
    nsIFontMetrics* fm;
    aRenderingContext.SetColor(myColor->mColor);

    nsAutoString text;
    switch (myList->mListStyleType) {
    case NS_STYLE_LIST_STYLE_NONE:
      break;

    default:
    case NS_STYLE_LIST_STYLE_BASIC:
    case NS_STYLE_LIST_STYLE_DISC:
      aRenderingContext.FillEllipse(mPadding.left, mPadding.top,
                                    mRect.width - (mPadding.left + mPadding.right),
                                    mRect.height - (mPadding.top + mPadding.bottom));
      break;

    case NS_STYLE_LIST_STYLE_CIRCLE:
      aRenderingContext.DrawEllipse(mPadding.left, mPadding.top,
                                    mRect.width - (mPadding.left + mPadding.right),
                                    mRect.height - (mPadding.top + mPadding.bottom));
      break;

    case NS_STYLE_LIST_STYLE_SQUARE:
      aRenderingContext.FillRect(mPadding.left, mPadding.top,
                                 mRect.width - (mPadding.left + mPadding.right),
                                 mRect.height - (mPadding.top + mPadding.bottom));
      break;

    case NS_STYLE_LIST_STYLE_DECIMAL:
    case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
    case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
    case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
    case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
      fm = aCX.GetMetricsFor(myFont->mFont);
      GetListItemText(&aCX, nsnull, *myList, text);
      aRenderingContext.SetFont(myFont->mFont);
      fm->GetWidth(text, width);
      aRenderingContext.DrawString(text, mPadding.left, mPadding.top, width);
      NS_RELEASE(fm);
      break;
    }
  }
  return NS_OK;
}

// XXX This is a hack to discover the block container that is keeping
// track of our list item ordinal value

// Return the reflow state for the list container that contains this
// list item frame. There may be no list container (a dangling LI)
// therefore this may return nsnull.
nsBlockReflowState*
BulletFrame::GetListContainerReflowState(nsIPresContext*      aCX,
                                         const nsReflowState& aReflowState)
{
  const nsReflowState* rs = aReflowState.parentReflowState;
  while (nsnull != rs) {
    nsIContent* content;
    rs->frame->GetContent(content);
    nsIAtom* tag;
    content->GetTag(tag);
    NS_RELEASE(content);
    if ((tag == nsHTMLAtoms::ul) || (tag == nsHTMLAtoms::ol) ||
        (tag == nsHTMLAtoms::menu) || (tag == nsHTMLAtoms::dir)) {
      NS_RELEASE(tag);
      return (nsBlockReflowState*) rs;
    }
    NS_RELEASE(tag);
    rs = rs->parentReflowState;
  }
  return nsnull;
}

PRInt32
BulletFrame::GetListItemOrdinal(nsIPresContext*     aCX,
                                nsBlockReflowState* aReflowState,
                                const nsStyleList&  aListStyle)
{
  if (mOrdinalValid) {
    return mOrdinal;
  }

  PRInt32 ordinal = 0;

  // Try to get value directly from the list-item, if it specifies a
  // value attribute. We do this with our parent's content.
  nsHTMLValue value;
  nsIContent* parentContent;
  mContentParent->GetContent(parentContent);
  nsIHTMLContent* html = (nsIHTMLContent*) parentContent;
  if (NS_CONTENT_ATTR_HAS_VALUE == html->GetAttribute(nsHTMLAtoms::value, value)) {
    if (eHTMLUnit_Integer == value.GetUnit()) {
      ordinal = value.GetIntValue();
      if (nsnull != aReflowState) {
        aReflowState->mNextListOrdinal = ordinal + 1;
      }
      NS_RELEASE(html);
      goto done;
    }
  }
  NS_RELEASE(html);

  // Get ordinal from block reflow state
  if (nsnull != aReflowState) {
    ordinal = aReflowState->mNextListOrdinal;
    if (ordinal < 0) {
      // This is the first list item and the list container doesn't
      // have a "start" attribute. Get the starting ordinal value
      // correctly set.
      switch (aListStyle.mListStyleType) {
      case NS_STYLE_LIST_STYLE_DECIMAL:
      case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
      case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
        ordinal = 1;
        break;
      default:
        ordinal = 0;
        break;
      }
    }
    aReflowState->mNextListOrdinal = ordinal + 1;
  }

 done:
  mOrdinal = ordinal;
  mOrdinalValid = PR_TRUE;
  return ordinal;
}

static const char* gLowerRomanCharsA = "ixcm";
static const char* gUpperRomanCharsA = "IXCM";
static const char* gLowerRomanCharsB = "vld?";
static const char* gUpperRomanCharsB = "VLD?";
static const char* gLowerAlphaChars  = "abcdefghijklmnopqrstuvwxyz";
static const char* gUpperAlphaChars  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// XXX change roman/alpha to use unsigned math so that maxint and
// maxnegint will work
void
BulletFrame::GetListItemText(nsIPresContext* aCX,
                             nsBlockReflowState* aReflowState,
                             const nsStyleList& aListStyle,
                             nsString& result)
{
  PRInt32 ordinal = GetListItemOrdinal(aCX, aReflowState, aListStyle);
  char cbuf[40];
  switch (aListStyle.mListStyleType) {
  case NS_STYLE_LIST_STYLE_DECIMAL:
    PR_snprintf(cbuf, sizeof(cbuf), "%ld", ordinal);
    result.Append(cbuf);
    break;

  case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
  case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
  {
    if (0 == ordinal) {
      ordinal = 1;
    }
    nsAutoString addOn;
    nsAutoString decStr;
    decStr.Append(ordinal, 10);
    const PRUnichar* dp = decStr.GetUnicode();
    const PRUnichar* end = dp + decStr.Length();

    PRIntn           len=decStr.Length();
    PRIntn           romanPos=len;
    PRIntn           n;

    const char* achars;
    const char* bchars;
    if (aListStyle.mListStyleType == NS_STYLE_LIST_STYLE_LOWER_ROMAN) {
      achars = gLowerRomanCharsA;
      bchars = gLowerRomanCharsB;
    } else {
      achars = gUpperRomanCharsA;
      bchars = gUpperRomanCharsB;
    }
    ordinal=(ordinal < 0) ? -ordinal : ordinal;
    if (ordinal < 0) {
      // XXX max negative int
      break;
    }
    for (; dp < end; dp++)
    {
      romanPos--;
      addOn.SetLength(0);
      switch(*dp)
      {
      case '3':  addOn.Append(achars[romanPos]);
      case '2':  addOn.Append(achars[romanPos]);
      case '1':  addOn.Append(achars[romanPos]);
        break;

      case '4':
        addOn.Append(achars[romanPos]);

      case '5': case '6':
      case '7': case  '8':
        addOn.Append(bchars[romanPos]);
        for(n=0;n<(*dp-'5');n++) {
          addOn.Append(achars[romanPos]);
        }
        break;
      case '9':
        addOn.Append(achars[romanPos]);
        addOn.Append(achars[romanPos+1]);
        break;
      default:
        break;
      }
      result.Append(addOn);
    }
  }
  break;

  case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
  case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
  {
    PRInt32 anOffset = -1;
    PRInt32 aBase = 26;
    PRInt32 ndex=0;
    PRInt32 root=1;
    PRInt32 next=aBase;
    PRInt32 expn=1;
    const char* chars =
      (aListStyle.mListStyleType == NS_STYLE_LIST_STYLE_LOWER_ALPHA)
      ? gLowerAlphaChars : gUpperAlphaChars;

    // must be positive here...
    ordinal = (ordinal < 0) ? -ordinal : ordinal;
    if (ordinal < 0) {
      // XXX max negative int
      break;
    }
    while (next<=ordinal)      // scale up in baseN; exceed current value.
    {
      root=next;
      next*=aBase;
      expn++;
    }

    while(0!=(expn--))
    {
      ndex = ((root<=ordinal) && (0!=root)) ? (ordinal/root): 0;
      ordinal %= root;
      if (root>1)
        result.Append(chars[ndex+anOffset]);
      else
        result.Append(chars[ndex]);
      root /= aBase;
    }
  }
  break;
  }
  result.Append(".");
}

#define MIN_BULLET_SIZE 5               // from laytext.c

void
BulletFrame::GetDesiredSize(nsIPresContext*  aCX,
                            nsBlockReflowState* aState,
                            const nsReflowState& aReflowState,
                            nsReflowMetrics& aMetrics)
{
  const nsStyleList* myList =
    (const nsStyleList*)mStyleContext->GetStyleData(eStyleStruct_List);
  nscoord ascent;

  if (myList->mListStyleImage.Length() > 0) {
    mImageLoader.SetURL(myList->mListStyleImage);
    mImageLoader.GetDesiredSize(aCX, aReflowState, aMetrics);
    if (!mImageLoader.GetLoadImageFailed()) {
      nsHTMLBase::CreateViewForFrame(aCX, this, mStyleContext, PR_FALSE);
      aMetrics.ascent = aMetrics.height;
      aMetrics.descent = 0;
      return;
    }
  }

  const nsStyleFont* myFont =
    (const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font);
  nsIFontMetrics* fm = aCX->GetMetricsFor(myFont->mFont);
  nscoord bulletSize;
  float p2t;
  float t2p;

  nsAutoString text;
  switch (myList->mListStyleType) {
  case NS_STYLE_LIST_STYLE_NONE:
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.descent = 0;
    break;

  default:
  case NS_STYLE_LIST_STYLE_DISC:
  case NS_STYLE_LIST_STYLE_CIRCLE:
  case NS_STYLE_LIST_STYLE_BASIC:
  case NS_STYLE_LIST_STYLE_SQUARE:
    t2p = aCX->GetTwipsToPixels();
    fm->GetMaxAscent(ascent);
    bulletSize = NSTwipsToIntPixels((nscoord)NSToIntRound(0.8f * (float(ascent) / 2.0f)), t2p);
    if (bulletSize < 1) {
      bulletSize = MIN_BULLET_SIZE;
    }
    p2t = aCX->GetPixelsToTwips();
    bulletSize = NSIntPixelsToTwips(bulletSize, p2t);
    mPadding.bottom = ascent / 8;
    if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == myList->mListStylePosition) {
      mPadding.right = bulletSize / 2;
    }
    aMetrics.width = mPadding.right + bulletSize;
    aMetrics.height = mPadding.bottom + bulletSize;
    aMetrics.ascent = mPadding.bottom + bulletSize;
    aMetrics.descent = 0;
    break;

  case NS_STYLE_LIST_STYLE_DECIMAL:
  case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
  case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
  case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
  case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
    GetListItemText(aCX, aState, *myList, text);
    fm->GetHeight(aMetrics.height);
    if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == myList->mListStylePosition) {
      // Inside bullets need some extra width to get the padding
      // between the list item and the content that follows.
      mPadding.right = aMetrics.height / 2;          // From old layout engine
    }
    
    fm->GetWidth(text, aMetrics.width);
    aMetrics.width += mPadding.right;
    fm->GetMaxAscent(aMetrics.ascent);
    fm->GetMaxDescent(aMetrics.descent);
    break;
  }
  NS_RELEASE(fm);
}

NS_IMETHODIMP
BulletFrame::InlineReflow(nsLineLayout& aLineLayout,
                          nsReflowMetrics& aMetrics,
                          const nsReflowState& aReflowState)
{
  nsBlockReflowState* state =
    GetListContainerReflowState(aLineLayout.mPresContext, aReflowState);

  // Get the base size
  GetDesiredSize(aLineLayout.mPresContext, state, aReflowState, aMetrics);

  // Add in the border and padding; split the top/bottom between the
  // ascent and descent to make things look nice
  const nsStyleSpacing* space =(const nsStyleSpacing*)
    mStyleContext->GetStyleData(eStyleStruct_Spacing);
  nsMargin borderPadding;
  space->CalcBorderPaddingFor(this, borderPadding);
  aMetrics.width += borderPadding.left + borderPadding.right;
  aMetrics.height += borderPadding.top + borderPadding.bottom;
  aMetrics.ascent += borderPadding.top;
  aMetrics.descent += borderPadding.bottom;

  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width = aMetrics.width;
    aMetrics.maxElementSize->height = aMetrics.height;
  }
  return NS_FRAME_COMPLETE;
}

NS_IMETHODIMP
BulletFrame::FindTextRuns(nsLineLayout& aLineLayout,
                          nsIReflowCommand* aReflowCommand)
{
  aLineLayout.EndTextRun();
  return NS_OK;
}
