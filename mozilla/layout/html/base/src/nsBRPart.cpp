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
#include "nsFrame.h"
#include "nsHTMLParts.h"
#include "nsHTMLTagContent.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsIInlineReflow.h"
#include "nsCSSLineLayout.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsIStyleContext.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"

class BRFrame : public nsFrame, public nsIInlineReflow {
public:
  BRFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  // nsISupports
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  // nsIFrame
  NS_IMETHOD Paint(nsIPresContext& aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect);

  // nsIInlineReflow
  NS_IMETHOD FindTextRuns(nsCSSLineLayout&  aLineLayout,
                          nsIReflowCommand* aReflowCommand);
  NS_IMETHOD InlineReflow(nsCSSLineLayout&     aLineLayout,
                          nsReflowMetrics&     aDesiredSize,
                          const nsReflowState& aReflowState);

protected:
  virtual ~BRFrame();
};

BRFrame::BRFrame(nsIContent* aContent,
                 nsIFrame* aParentFrame)
  : nsFrame(aContent, aParentFrame)
{
}

BRFrame::~BRFrame()
{
}

NS_IMETHODIMP
BRFrame::QueryInterface(REFNSIID aIID, void** aInstancePtrResult)
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
BRFrame::Paint(nsIPresContext&      aPresContext,
               nsIRenderingContext& aRenderingContext,
               const nsRect&        aDirtyRect)
{
  if (nsIFrame::GetShowFrameBorders()) {
    float p2t = aPresContext.GetPixelsToTwips();
    aRenderingContext.SetColor(NS_RGB(0, 255, 255));
    aRenderingContext.FillRect(0, 0, NSIntPixelsToTwips(5, p2t), mRect.height);
  }
  return NS_OK;
}

NS_IMETHODIMP
BRFrame::FindTextRuns(nsCSSLineLayout&  aLineLayout,
                      nsIReflowCommand* aReflowCommand)
{
  aLineLayout.EndTextRun();
  return NS_OK;
}

NS_IMETHODIMP
BRFrame::InlineReflow(nsCSSLineLayout&      aLineLayout,
                      nsReflowMetrics&      aMetrics,
                      const nsReflowState&  aReflowState)
{
  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width = 0;
    aMetrics.maxElementSize->height = 0;
  }

  // We have no width, but we're the height of the default font
  const nsStyleFont* font = (const nsStyleFont*)
    mStyleContext->GetStyleData(eStyleStruct_Font);
  nsIFontMetrics* fm = aLineLayout.mPresContext->GetMetricsFor(font->mFont);

  fm->GetMaxAscent(aMetrics.ascent);
  fm->GetMaxDescent(aMetrics.descent);
  aMetrics.height = aMetrics.ascent + aMetrics.descent;
  aMetrics.width = 0;
  NS_RELEASE(fm);

  // Return our inline reflow status
  const nsStyleDisplay* display = (const nsStyleDisplay*)
    mStyleContext->GetStyleData(eStyleStruct_Display);
  PRUint32 breakType = display->mBreakType;
  if (NS_STYLE_CLEAR_NONE == breakType) {
    breakType = NS_STYLE_CLEAR_LINE;
  }

  return NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
    NS_INLINE_MAKE_BREAK_TYPE(breakType);
}

//----------------------------------------------------------------------

class BRPart : public nsHTMLTagContent {
public:
  BRPart(nsIAtom* aTag);

  virtual void SetAttribute(nsIAtom* aAttribute, const nsString& aValue);
  virtual void MapAttributesInto(nsIStyleContext* aContext,
                                 nsIPresContext* aPresContext);
  virtual nsresult CreateFrame(nsIPresContext* aPresContext,
                               nsIFrame* aParentFrame,
                               nsIStyleContext* aStyleContext,
                               nsIFrame*& aResult);

protected:
  virtual ~BRPart();
  virtual nsContentAttr AttributeToString(nsIAtom* aAttribute,
                                          nsHTMLValue& aValue,
                                          nsString& aResult) const;
};

BRPart::BRPart(nsIAtom* aTag)
  : nsHTMLTagContent(aTag)
{
}

BRPart::~BRPart()
{
}

nsresult
BRPart::CreateFrame(nsIPresContext*  aPresContext,
                    nsIFrame*        aParentFrame,
                    nsIStyleContext* aStyleContext,
                    nsIFrame*&       aResult)
{
  nsIFrame* frame = new BRFrame(this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return NS_OK;
}

static nsHTMLTagContent::EnumTable kClearTable[] = {
  { "left", NS_STYLE_CLEAR_LEFT },
  { "right", NS_STYLE_CLEAR_RIGHT },
  { "all", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { "both", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { 0 }
};

void
BRPart::SetAttribute(nsIAtom* aAttribute, const nsString& aString)
{
  if (aAttribute == nsHTMLAtoms::clear) {
    nsHTMLValue value;
    if (ParseEnumValue(aString, kClearTable, value)) {
      nsHTMLTagContent::SetAttribute(aAttribute, value);
    }
    return;
  }
  nsHTMLTagContent::SetAttribute(aAttribute, aString);
}

void
BRPart::MapAttributesInto(nsIStyleContext* aContext,
                          nsIPresContext* aPresContext)
{
  if (nsnull != mAttributes) {
    nsStyleDisplay* display = (nsStyleDisplay*)
      aContext->GetMutableStyleData(eStyleStruct_Display);
    nsHTMLValue value;
    GetAttribute(nsHTMLAtoms::clear, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) {
      display->mBreakType = value.GetIntValue();
    }
  }
}

nsContentAttr
BRPart::AttributeToString(nsIAtom*     aAttribute,
                          nsHTMLValue& aValue,
                          nsString&    aResult) const
{
  if (aAttribute == nsHTMLAtoms::clear) {
    if ((eHTMLUnit_Enumerated == aValue.GetUnit()) &&
        (NS_STYLE_CLEAR_NONE != aValue.GetIntValue())) {
      EnumValueToString(aValue, kClearTable, aResult);
      return eContentAttr_HasValue;
    }
  }
  return eContentAttr_NotThere;
}

//----------------------------------------------------------------------

nsresult
NS_NewHTMLBreak(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new BRPart(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}
