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
#include "nsListItemFrame.h"
#include "nsIStyleContext.h"
#include "nsFrame.h"
#include "nsString.h"
#include "nsIRenderingContext.h"
#include "nsIPresContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleConsts.h"
#include "nsUnitConversion.h"
#include "nsIContent.h"
#include "nsHTMLAtoms.h"
#include "nsHTMLValue.h"
#include "nsIHTMLContent.h"
#include "nsHTMLIIDs.h"
#include "nsIPresShell.h"
#include "prprf.h"
#include "nsIView.h"

// XXX TODO:
// 1. If container is RTL then place bullets on the right side
// 2. list-style-image
// 3. proper ascent alignment
// 4. number bullets correctly when dealing with <LI VALUE=x>
// 5. size and render circle, square, disc bullets like nav4 does

static NS_DEFINE_IID(kStyleFontSID, NS_STYLEFONT_SID);
static NS_DEFINE_IID(kStyleColorSID, NS_STYLECOLOR_SID);
static NS_DEFINE_IID(kStyleListSID, NS_STYLELIST_SID);

/**
 * A pseudo-frame class that reflows the list item bullet.
 */
class BulletFrame : public nsContainerFrame {
public:
  BulletFrame(nsIContent* aContent,
              PRInt32 aIndexInParent,
              nsIFrame* aParentFrame);
  virtual ~BulletFrame();

  NS_IMETHOD Paint(nsIPresContext &aCX,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  NS_IMETHOD ResizeReflow(nsIPresContext* aCX,
                          nsReflowMetrics& aDesiredSize,
                          const nsSize& aMaxSize,
                          nsSize* aMaxElementSize,
                          ReflowStatus& aStatus);

  NS_IMETHOD IncrementalReflow(nsIPresContext* aCX,
                               nsReflowMetrics& aDesiredSize,
                               const nsSize& aMaxSize,
                               nsReflowCommand& aReflowCommand,
                               ReflowStatus& aStatus);

  void GetBulletSize(nsIPresContext* aCX,
                     nsReflowMetrics& aDesiredSize,
                     const nsSize& aMaxSize);

  PRInt32 GetListItemOrdinal(nsIPresContext* aCX, nsStyleList& aMol);

  void GetListItemText(nsIPresContext* aCX, nsString& aResult,
                       nsStyleList& aMol);

  PRPackedBool mOrdinalValid;
  PRInt32 mOrdinal;
};

BulletFrame::BulletFrame(nsIContent* aContent,
                         PRInt32 aIndexInParent,
                         nsIFrame* aParentFrame)
  : nsContainerFrame(aContent, aIndexInParent, aParentFrame)
{
}

BulletFrame::~BulletFrame()
{
}

// XXX padding for around the bullet; should come from style system
#define PAD_DISC        NS_POINTS_TO_TWIPS_INT(1)

NS_METHOD BulletFrame::Paint(nsIPresContext& aCX,
                             nsIRenderingContext& aRenderingContext,
                             const nsRect& aDirtyRect)
{
  nsStyleFont* myFont =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);
  nsStyleColor* myColor =
    (nsStyleColor*)mStyleContext->GetData(kStyleColorSID);
  nsStyleList* myList =
    (nsStyleList*)mStyleContext->GetData(kStyleListSID);
  nsIFontMetrics* fm = aCX.GetMetricsFor(myFont->mFont);

  nscoord pad;

  nsAutoString text;
  switch (myList->mListStyleType) {
  case NS_STYLE_LIST_STYLE_NONE:
    break;

  case NS_STYLE_LIST_STYLE_DISC:
  case NS_STYLE_LIST_STYLE_CIRCLE:
  case NS_STYLE_LIST_STYLE_SQUARE:
    pad = PAD_DISC;
    aRenderingContext.SetColor(myColor->mColor);
    aRenderingContext.FillRect(pad, pad, mRect.width - (pad + pad),
                               mRect.height - (pad + pad));/* XXX */
    break;

  case NS_STYLE_LIST_STYLE_DECIMAL:
  case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
  case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
  case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
  case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
    GetListItemText(&aCX, text, *myList);
    aRenderingContext.SetColor(myColor->mColor);
    aRenderingContext.SetFont(myFont->mFont);
    aRenderingContext.DrawString(text, 0, 0, fm->GetWidth(text));
    break;
  }
  NS_RELEASE(fm);
  return NS_OK;
}

NS_METHOD BulletFrame::ResizeReflow(nsIPresContext* aCX,
                                    nsReflowMetrics& aDesiredSize,
                                    const nsSize& aMaxSize,
                                    nsSize* aMaxElementSize,
                                    ReflowStatus& aStatus)
{
  GetBulletSize(aCX, aDesiredSize, aMaxSize);
  if (nsnull != aMaxElementSize) {
    aMaxElementSize->width = aDesiredSize.width;
    aMaxElementSize->height = aDesiredSize.height;
  }

  // This is done so that our containers VerifyTree code will work
  // correctly.  Otherwise it will think that the child that follows
  // the bullet must be mapping the second content object instead of
  // mapping the first content object.
  mLastContentIsComplete = PR_FALSE;
  aStatus = frComplete;
  return NS_OK;
}

NS_METHOD BulletFrame::IncrementalReflow(nsIPresContext* aCX,
                                         nsReflowMetrics& aDesiredSize,
                                         const nsSize& aMaxSize,
                                         nsReflowCommand& aReflowCommand,
                                         ReflowStatus& aStatus)
{
  // XXX Unless the reflow command is a style change, we should
  // just return the current size, otherwise we should invoke
  // GetBulletSize
  GetBulletSize(aCX, aDesiredSize, aMaxSize);

  aStatus = frComplete;
  return NS_OK;
}

PRInt32 BulletFrame::GetListItemOrdinal(nsIPresContext* aCX,
                                        nsStyleList& aListStyle)
{
  if (mOrdinalValid) {
    return mOrdinal;
  }

  PRInt32 ordinal = 0;

  // Get block reflow state for the list container
  nsListItemFrame* listItem = (nsListItemFrame*) mGeometricParent;
  nsBlockReflowState* state = listItem->GetListContainerReflowState(aCX);

  // Try to get value directly from the list-item, if it specifies a
  // value attribute.
  nsHTMLValue value;
  nsIHTMLContent* html = (nsIHTMLContent*) mContent;
  if (eContentAttr_HasValue == html->GetAttribute(nsHTMLAtoms::value, value)) {
    if (eHTMLUnit_Absolute == value.GetUnit()) {
      ordinal = value.GetIntValue();
      if (nsnull != state) {
        state->nextListOrdinal = ordinal + 1;
      }
      goto done;
    }
  }

  // Get ordinal from block reflow state
  if (nsnull != state) {
    ordinal = state->nextListOrdinal;
    if (ordinal < 0) {
      // This is the first list item and the list container doesn't
      // have a "start" attribute. Get the starting ordinal value
      // correctly set.
      switch (aListStyle.mListStyleType) {
      case NS_STYLE_LIST_STYLE_DECIMAL:
        ordinal = 1;
        break;
      default:
        ordinal = 0;
        break;
      }
    }
    state->nextListOrdinal = ordinal + 1;
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

void
BulletFrame::GetListItemText(nsIPresContext* aCX,
                             nsString& result,
                             nsStyleList& aListStyle)
{
  PRInt32 ordinal = GetListItemOrdinal(aCX, aListStyle);
  char cbuf[40];
  switch (aListStyle.mListStyleType) {
  case NS_STYLE_LIST_STYLE_DECIMAL:
    PR_snprintf(cbuf, sizeof(cbuf), "%ld", ordinal);
    result.Append(cbuf);
    break;

  case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
  case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
  {
    // XXX deal with an ordinal of ZERO
  	nsAutoString addOn;
  	nsAutoString decStr;
    decStr.Append(ordinal, 10);
    const PRUnichar* dp = decStr.GetUnicode();
    const PRUnichar* end = dp + decStr.Length();

    PRIntn           len=decStr.Length();
    PRIntn           romanPos=len;
  	PRIntn           n;
  	PRBool           negative=PRBool(ordinal<0);

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
      case  '3':  addOn.Append(achars[romanPos]);
      case  '2':  addOn.Append(achars[romanPos]);
      case	'1':  addOn.Append(achars[romanPos]);
        break;

      case	'4':
        addOn.Append(achars[romanPos]);

      case 	'5':	case	'6':
      case	'7':	case 	'8':
        addOn.Append(bchars[romanPos]);
        for(n=0;n<(*dp-'5');n++) {
          addOn.Append(achars[romanPos]);
        }
        break;
      case	'9':
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
    while (next<=ordinal)		    // scale up in baseN; exceed current value.
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

void BulletFrame::GetBulletSize(nsIPresContext* aCX,
                                nsReflowMetrics& aDesiredSize,
                                const nsSize& aMaxSize)
{
  nsStyleList* myList =
    (nsStyleList*)mStyleContext->GetData(kStyleListSID);
  nsStyleFont* myFont =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);

  nscoord pad;

  nsIFontMetrics* fm = aCX->GetMetricsFor(myFont->mFont);
  nsAutoString text;
  switch (myList->mListStyleType) {
  case NS_STYLE_LIST_STYLE_NONE:
    aDesiredSize.width = 0;
    aDesiredSize.height = 0;
    aDesiredSize.ascent = 0;
    aDesiredSize.descent = 0;
    break;

  case NS_STYLE_LIST_STYLE_DISC:
  case NS_STYLE_LIST_STYLE_CIRCLE:
  case NS_STYLE_LIST_STYLE_SQUARE:
    /* XXX match navigator */
    pad = PAD_DISC;
    aDesiredSize.width = pad + pad + fm->GetWidth('x');
    aDesiredSize.height = pad + pad + fm->GetWidth('x');
    aDesiredSize.ascent = pad + fm->GetWidth('x');
    aDesiredSize.descent = pad;
    break;

  case NS_STYLE_LIST_STYLE_DECIMAL:
  case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
  case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
  case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
  case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
    GetListItemText(aCX, text, *myList);
    if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == myList->mListStylePosition) {
      // Inside bullets need some extra width to get the padding
      // between the list item and the content that follows.
      pad = fm->GetHeight() / 2;          // From old layout engine
    } else {
      // Outside bullets get there padding by placement not by sizing
      pad = 0;
    }
    aDesiredSize.width = pad + fm->GetWidth(text);
    aDesiredSize.height = fm->GetHeight();
    aDesiredSize.ascent = fm->GetMaxAscent();
    aDesiredSize.descent = fm->GetMaxDescent();
    break;
  }
  NS_RELEASE(fm);
}

//----------------------------------------------------------------------

nsresult nsListItemFrame::NewFrame(nsIFrame** aInstancePtrResult,
                                   nsIContent* aContent,
                                   PRInt32     aIndexInParent,
                                   nsIFrame*   aParent)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIFrame* it = new nsListItemFrame(aContent, aIndexInParent, aParent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aInstancePtrResult = it;
  return NS_OK;
}

nsListItemFrame::nsListItemFrame(nsIContent* aContent,
                                 PRInt32     aIndexInParent,
                                 nsIFrame*   aParent)
  : nsBlockFrame(aContent, aIndexInParent, aParent)
{
}

nsListItemFrame::~nsListItemFrame()
{
}

nsIFrame* nsListItemFrame::CreateBullet()
{
  // Create bullet.  The bullet shares the same style context as
  // ourselves.
  nsIFrame* bullet = new BulletFrame(mContent, mIndexInParent, this);
  bullet->SetStyleContext(mStyleContext);
  return bullet;
}

void nsListItemFrame::PlaceOutsideBullet(nsIFrame* aBullet,
                                         nsIPresContext* aCX)
{
  nsSize maxSize(0, 0);
  nsReflowMetrics bulletSize;

  // Size the bullet
  ReflowChild(aBullet, aCX, bulletSize, maxSize, nsnull);

  // We can only back the bullet over so far XXX what does nav do with
  // a big bullet in a blockquote? does the bullet leak out of the
  // blockquote or what?
  nscoord minX = -mRect.x;

  // Get bullet style (which is our style)
  nsStyleFont* font =
    (nsStyleFont*)mStyleContext->GetData(kStyleFontSID);
  nsIFontMetrics* fm = aCX->GetMetricsFor(font->mFont);
  nscoord kidAscent = fm->GetMaxAscent();
  nscoord dx = fm->GetHeight() / 2;             // from old layout engine
  NS_RELEASE(fm);

  // Find the bullet x,y
  nscoord x = 0;
  nscoord y = 0;
#if 0
  nsIFrame* firstKid = mFirstChild;
  if (nsnull != firstKid) {
    nsIContent* kidContent = firstKid->GetContent();
    nsIStyleContext* kidStyleContext = firstKid->GetStyleContext(aCX);
    nsStyleFont* kidFont =
      (nsStyleFont*)kidStyleContext->GetData(kStyleFontSID);
    nsPoint origin;
    firstKid->GetOrigin(origin);
    // XXX this is wrong if the first kid was relative positioned!
    x = origin.x;
    y = origin.y;
    // XXX This is an approximation. If the kid has borders and
    // padding then this value will be wrong. If the kid is a
    // container then margins may mess it up as well.
    nsIFontMetrics* fm = aCX->GetMetricsFor(kidFont->mFont);
    kidAscent = fm->GetMaxAscent();
    NS_RELEASE(fm);

    NS_RELEASE(kidStyleContext);
    NS_RELEASE(kidContent);
  }
#endif

  x = x - dx - bulletSize.width;
#if XXX
  // XXX can't do this yet: 1. our mRect value is not right on the
  // first reflow; 2. it's the wrong limiter: nav4 compares against
  // the window min x.
  if (x < minX) {
    x = minX;
  }
#endif
  y = y + kidAscent - bulletSize.ascent;

  aBullet->SetRect(nsRect(x, y, bulletSize.width, bulletSize.height));
}

// Return the reflow state for the list container that contains this
// list item frame. There may be no list container (a dangling LI)
// therefore this may return nsnull.
nsBlockReflowState*
nsListItemFrame::GetListContainerReflowState(nsIPresContext* aCX)
{
  nsBlockReflowState* state = nsnull;
  nsIFrame* parent = mGeometricParent;
  while (nsnull != parent) {
    nsIHTMLFrameType* ft;
    nsresult status = parent->QueryInterface(kIHTMLFrameTypeIID, (void**) &ft);
    if (NS_OK == status) {
      nsHTMLFrameType type = ft->GetFrameType();
      if (eHTMLFrame_Block == type) {
        // The parent is a block. See if its content object is a list
        // container. Only UL, OL, MENU or DIR can be list containers.
        // XXX need something more flexible, say style?
        nsIContent* parentContent;
         
        parent->GetContent(parentContent);
        nsIAtom* tag = parentContent->GetTag();
        NS_RELEASE(parentContent);
        if ((tag == nsHTMLAtoms::ul) || (tag == nsHTMLAtoms::ol) ||
            (tag == nsHTMLAtoms::menu) || (tag == nsHTMLAtoms::dir)) {
          NS_RELEASE(tag);
          break;
        }
        NS_RELEASE(tag);
      }
    }
    parent->GetGeometricParent(parent);
  }
  if (nsnull != parent) {
    nsIPresShell* shell = aCX->GetShell();
    state = (nsBlockReflowState*) shell->GetCachedData(parent);
    NS_RELEASE(shell);
  }
  return state;
}

/**
 * The basic approach here is pretty simple: let our base class do all
 * the hard work, and after it's done, get the bullet placed. We only
 * have a bullet if we are not a continuation.
 */
// XXX we may need to grow to accomodate the bullet
// XXX check for compatability: <LI><H1>dah dah</H1> where is bullet?
NS_METHOD nsListItemFrame::ResizeReflow(nsIPresContext* aCX,
                                        nsISpaceManager* aSpaceManager,
                                        const nsSize& aMaxSize,
                                        nsRect& aDesiredRect,
                                        nsSize* aMaxElementSize,
                                        ReflowStatus& aStatus)
{
  PRBool insideBullet = PR_FALSE;

  // Get bullet style (which is our style)
  nsStyleList* myList =
    (nsStyleList*)mStyleContext->GetData(kStyleListSID);
  if (NS_STYLE_LIST_STYLE_POSITION_INSIDE == myList->mListStylePosition) {
    insideBullet = PR_TRUE;
  }

  nsIFrame* bullet = nsnull;
  if (nsnull == mPrevInFlow) {
    if (insideBullet) {
      if (nsnull == mFirstChild) {
        // Inside bullets get placed on the list immediately so that
        // the regular reflow logic can place them.
        bullet = CreateBullet();
        mFirstChild = bullet;
        mChildCount++;
      } else {
        // We already have a first child. It's the bullet (check?)
        // so we don't need to do anything here
      }
    } else {
      if (nsnull == mFirstChild) {
        // Create outside bullet the first time through
        bullet = CreateBullet();
      } else {
        // Pull bullet off list (we'll put it back later)
        bullet = mFirstChild;
        bullet->GetNextSibling(mFirstChild);
        mChildCount--;
      }
    }
  }

  // Let base class do things first
  nsBlockReflowState state;
  SetupState(aCX, state, aMaxSize, aMaxElementSize, aSpaceManager);
  state.firstChildIsInsideBullet = insideBullet;
  DoResizeReflow(aCX, state, aDesiredRect, aStatus);

  // Now place the bullet and put it at the head of the list of children
  if (!insideBullet && (nsnull != bullet)) {
    // XXX Change PlaceOutsideBullet to just size the bullet frame, then let
    // the normal vertical alignment code run. The reason it's not
    // implemented yet is that I don't have the ascent/descent
    // information for the first line.
    PlaceOutsideBullet(bullet, aCX);
    bullet->SetNextSibling(mFirstChild);
    mFirstChild = bullet;
    mChildCount++;
    if (nsnull == mLines) {
      mLines = new PRInt32[1];
      mLines[0] = 1;
      mNumLines = 1;
    } else {
      mLines[0]++;
    }
  }
  return NS_OK;
}

// XXX we may need to grow to accomodate the bullet
NS_METHOD nsListItemFrame::IncrementalReflow(nsIPresContext* aCX,
                                             nsISpaceManager* aSpaceManager,
                                             const nsSize& aMaxSize,
                                             nsRect& aDesiredRect,
                                             nsReflowCommand& aReflowCommand,
                                             ReflowStatus& aStatus)
{
  aStatus = frComplete;
  return NS_OK;
}

NS_METHOD nsListItemFrame::CreateContinuingFrame(nsIPresContext* aCX,
                                                 nsIFrame* aParent,
                                                 nsIFrame*& aContinuingFrame)
{
  nsListItemFrame* cf = new nsListItemFrame(mContent, mIndexInParent, aParent);
  PrepareContinuingFrame(aCX, aParent, cf);
  aContinuingFrame = cf;
  return NS_OK;
}

// aDirtyRect is in our coordinate system
// child rect's are also in our coordinate system
void nsListItemFrame::PaintChildren(nsIPresContext& aCX,
                                    nsIRenderingContext& aRenderingContext,
                                    const nsRect& aDirtyRect)
{
  nsIFrame* kid = mFirstChild;
  while (nsnull != kid) {
    nsIView *pView;
     
    kid->GetView(pView);
    if (nsnull == pView) {
      nsRect kidRect;
      kid->GetRect(kidRect);
      nsRect damageArea;
      PRBool overlap = damageArea.IntersectRect(aDirtyRect, kidRect);
      // Note the special check here for our first child. We do this
      // because we sometimes position bullets (our first child) outside
      // of our frame.
      if (overlap || (kid == mFirstChild)) {
        // Translate damage area into kid's coordinate system
        nsRect kidDamageArea(damageArea.x - kidRect.x, damageArea.y - kidRect.y,
                             damageArea.width, damageArea.height);
        aRenderingContext.PushState();
        aRenderingContext.Translate(kidRect.x, kidRect.y);
        kid->Paint(aCX, aRenderingContext, kidDamageArea);
        if (nsIFrame::GetShowFrameBorders()) {
          aRenderingContext.SetColor(NS_RGB(255,0,0));
          aRenderingContext.DrawRect(0, 0, kidRect.width, kidRect.height);
        }
        aRenderingContext.PopState();
      }
    } else {
      NS_RELEASE(pView);
    }
    kid->GetNextSibling(kid);
  }
}
