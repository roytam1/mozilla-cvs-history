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
#include "nsFrame.h"
#include "nsIInlineReflow.h"
#include "nsCSSLineLayout.h"
#include "nsHTMLIIDs.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"

// Spacer type's
#define TYPE_WORD  0            // horizontal space
#define TYPE_LINE  1            // line-break + vertical space
#define TYPE_IMAGE 2            // acts like a sized image with nothing to see

class SpacerFrame : public nsFrame, private nsIInlineReflow {
public:
  SpacerFrame(nsIContent* aContent, nsIFrame* aParentFrame);

  // nsISupports
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  // nsIInlineReflow
  NS_IMETHOD FindTextRuns(nsCSSLineLayout&  aLineLayout,
                          nsIReflowCommand* aReflowCommand);
  NS_IMETHOD InlineReflow(nsCSSLineLayout&     aLineLayout,
                          nsReflowMetrics&     aDesiredSize,
                          const nsReflowState& aReflowState);

protected:
  virtual ~SpacerFrame();
};

class SpacerPart : public nsHTMLTagContent {
public:
  SpacerPart(nsIAtom* aTag);

  NS_IMETHOD CreateFrame(nsIPresContext* aPresContext,
                         nsIFrame* aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*& aResult);
  NS_IMETHOD SetAttribute(nsIAtom* aAttribute, const nsString& aValue,
                          PRBool aNotify);
  NS_IMETHOD MapAttributesInto(nsIStyleContext* aContext,
                               nsIPresContext* aPresContext);
  NS_IMETHOD AttributeToString(nsIAtom*     aAttribute,
                               nsHTMLValue& aValue,
                               nsString&    aResult) const;

  PRUint8 GetType();

protected:
  virtual ~SpacerPart();
};

//----------------------------------------------------------------------

SpacerFrame::SpacerFrame(nsIContent* aContent, nsIFrame* aParentFrame)
  : nsFrame(aContent, aParentFrame)
{
}

SpacerFrame::~SpacerFrame()
{
}

NS_IMETHODIMP
SpacerFrame::QueryInterface(REFNSIID aIID, void** aInstancePtrResult)
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

NS_IMETHODIMP
SpacerFrame::InlineReflow(nsCSSLineLayout&     aLineLayout,
                          nsReflowMetrics&     aMetrics,
                          const nsReflowState& aReflowState)
{
  nsresult rv = NS_FRAME_COMPLETE;

  // By default, we have no area
  aMetrics.width = 0;
  aMetrics.height = 0;
  aMetrics.ascent = 0;
  aMetrics.descent = 0;

  nscoord width = 0;
  nscoord height = 0;
  SpacerPart* part = (SpacerPart*) mContent;/* XXX decouple */
  PRUint8 type = part->GetType();
  nsresult ca;
  if (type != TYPE_IMAGE) {
    nsHTMLValue val;
    ca = part->GetAttribute(nsHTMLAtoms::size, val);
    if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
      width = val.GetPixelValue();
    }
  } else {
    nsHTMLValue val;
    ca = part->GetAttribute(nsHTMLAtoms::width, val);
    if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
      if (eHTMLUnit_Pixel == val.GetUnit()) {
        width = val.GetPixelValue();
      }
    }
    ca = part->GetAttribute(nsHTMLAtoms::height, val);
    if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
      if (eHTMLUnit_Pixel == val.GetUnit()) {
        height = val.GetPixelValue();
      }
    }
  }

  float p2t = aLineLayout.mPresContext->GetPixelsToTwips();
  switch (type) {
  case TYPE_WORD:
    if (0 != width) {
      aMetrics.width = NSIntPixelsToTwips(width, p2t);
    }
    break;

  case TYPE_LINE:
    if (0 != width) {
      rv = NS_INLINE_LINE_BREAK_AFTER(0);
      aMetrics.height = NSIntPixelsToTwips(width, p2t);
      aMetrics.ascent = aMetrics.height;
    }
    break;

  case TYPE_IMAGE:
    aMetrics.width = NSIntPixelsToTwips(width, p2t);
    aMetrics.height = NSIntPixelsToTwips(height, p2t);
    aMetrics.ascent = aMetrics.height;
    break;
  }

  if (nsnull != aMetrics.maxElementSize) {
    aMetrics.maxElementSize->width = aMetrics.width;
    aMetrics.maxElementSize->height = aMetrics.height;
  }

  return rv;
}

NS_IMETHODIMP
SpacerFrame::FindTextRuns(nsCSSLineLayout&  aLineLayout,
                          nsIReflowCommand* aReflowCommand)
{
  aLineLayout.EndTextRun();
  return NS_OK;
}

//----------------------------------------------------------------------

SpacerPart::SpacerPart(nsIAtom* aTag)
  : nsHTMLTagContent(aTag)
{
}

SpacerPart::~SpacerPart()
{
}

nsresult
SpacerPart::CreateFrame(nsIPresContext*  aPresContext,
                        nsIFrame*        aParentFrame,
                        nsIStyleContext* aStyleContext,
                        nsIFrame*&       aResult)
{
  nsIFrame* frame = new SpacerFrame(this, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return NS_OK;
}

PRUint8
SpacerPart::GetType()
{
  PRUint8 type = TYPE_WORD;
  nsHTMLValue value;
  if (NS_CONTENT_ATTR_HAS_VALUE ==
      nsHTMLTagContent::GetAttribute(nsHTMLAtoms::type, value)) {
    if (eHTMLUnit_Enumerated == value.GetUnit()) {
      type = value.GetIntValue();
    }
  }
  return type;
}

static nsHTMLTagContent::EnumTable kTypeTable[] = {
  { "line", TYPE_LINE },
  { "vert", TYPE_LINE },
  { "vertical", TYPE_LINE },
  { "block", TYPE_IMAGE },
  { 0 }
};

NS_IMETHODIMP
SpacerPart::SetAttribute(nsIAtom* aAttribute, const nsString& aValue,
                         PRBool aNotify)
{
  nsHTMLValue val;
  if (aAttribute == nsHTMLAtoms::type) {
    if (ParseEnumValue(aValue, kTypeTable, val)) {
      return nsHTMLTagContent::SetAttribute(aAttribute, val, aNotify);
    }
  }
  else if (aAttribute == nsHTMLAtoms::size) {
    ParseValue(aValue, 0, val, eHTMLUnit_Pixel);
    return nsHTMLTagContent::SetAttribute(aAttribute, val, aNotify);
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    ParseAlignParam(aValue, val);
    return nsHTMLTagContent::SetAttribute(aAttribute, val, aNotify);
  }
  else if ((aAttribute == nsHTMLAtoms::width) ||
           (aAttribute == nsHTMLAtoms::height)) {
    ParseValueOrPercent(aValue, val, eHTMLUnit_Pixel);
    return nsHTMLTagContent::SetAttribute(aAttribute, val, aNotify);
  }
  return nsHTMLTagContent::SetAttribute(aAttribute, aValue, aNotify);
}

NS_IMETHODIMP
SpacerPart::MapAttributesInto(nsIStyleContext* aContext,
                              nsIPresContext* aPresContext)
{
  if (nsnull != mAttributes) {
    nsHTMLValue value;
    nsStyleDisplay* display = (nsStyleDisplay*)
      aContext->GetMutableStyleData(eStyleStruct_Display);
    GetAttribute(nsHTMLAtoms::align, value);
    if (eHTMLUnit_Enumerated == value.GetUnit()) {
      switch (value.GetIntValue()) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        display->mFloats = NS_STYLE_FLOAT_LEFT;
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        display->mFloats = NS_STYLE_FLOAT_RIGHT;
        break;
      default:
        break;
      }
    }
    PRUint8 type = GetType();
    switch (type) {
    case TYPE_WORD:
    case TYPE_IMAGE:
      break;

    case TYPE_LINE:
      // This is not strictly 100% compatible: if the spacer is given
      // a width of zero then it is basically ignored.
      display->mDisplay = NS_STYLE_DISPLAY_BLOCK;
      break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
SpacerPart::AttributeToString(nsIAtom*     aAttribute,
                              nsHTMLValue& aValue,
                              nsString&    aResult) const
{
  if (aAttribute == nsHTMLAtoms::align) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      EnumValueToString(aValue, kTypeTable, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  else if (aAttribute == nsHTMLAtoms::type) {
    if (eHTMLUnit_Enumerated == aValue.GetUnit()) {
      AlignParamToString(aValue, aResult);
      return NS_CONTENT_ATTR_HAS_VALUE;
    }
  }
  return NS_CONTENT_ATTR_NOT_THERE;
}

nsresult
NS_NewHTMLSpacer(nsIHTMLContent** aInstancePtrResult, nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* it = new SpacerPart(aTag);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(kIHTMLContentIID, (void**) aInstancePtrResult);
}
