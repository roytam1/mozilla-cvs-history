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
#include "nsIHTMLContent.h"
#include "nsFrame.h"
#include "nsIInlineReflow.h"
#include "nsLineLayout.h"
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
  NS_IMETHOD FindTextRuns(nsLineLayout&     aLineLayout,
                          nsIReflowCommand* aReflowCommand);
  NS_IMETHOD InlineReflow(nsLineLayout&        aLineLayout,
                          nsReflowMetrics&     aDesiredSize,
                          const nsReflowState& aReflowState);

  PRUint8 GetType();

protected:
  virtual ~SpacerFrame();
};

nsresult
NS_NewSpacerFrame(nsIContent* aContent,
                  nsIFrame* aParentFrame,
                  nsIFrame*& aResult)
{
  nsIFrame* frame = new SpacerFrame(aContent, aParentFrame);
  if (nsnull == frame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aResult = frame;
  return NS_OK;
}

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
SpacerFrame::InlineReflow(nsLineLayout&        aLineLayout,
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
  PRUint8 type = GetType();
  nsresult ca;
  nsIHTMLContent* hc = nsnull;
  mContent->QueryInterface(kIHTMLContentIID, (void**) &hc);
  if (nsnull != hc) {
    if (type != TYPE_IMAGE) {
      nsHTMLValue val;
      ca = hc->GetAttribute(nsHTMLAtoms::size, val);
      if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
        width = val.GetPixelValue();
      }
    } else {
      nsHTMLValue val;
      ca = hc->GetAttribute(nsHTMLAtoms::width, val);
      if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
        if (eHTMLUnit_Pixel == val.GetUnit()) {
          width = val.GetPixelValue();
        }
      }
      ca = hc->GetAttribute(nsHTMLAtoms::height, val);
      if (NS_CONTENT_ATTR_HAS_VALUE == ca) {
        if (eHTMLUnit_Pixel == val.GetUnit()) {
          height = val.GetPixelValue();
        }
      }
    }
    NS_RELEASE(hc);
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
SpacerFrame::FindTextRuns(nsLineLayout&     aLineLayout,
                          nsIReflowCommand* aReflowCommand)
{
  aLineLayout.EndTextRun();
  return NS_OK;
}

PRUint8
SpacerFrame::GetType()
{
  PRUint8 type = TYPE_WORD;
  nsAutoString value;
  if (NS_CONTENT_ATTR_HAS_VALUE == mContent->GetAttribute("type", value)) {
    if (value.EqualsIgnoreCase("line") ||
        value.EqualsIgnoreCase("vert") ||
        value.EqualsIgnoreCase("vertical")) {
      return TYPE_LINE;
    }
    if (value.EqualsIgnoreCase("block")) {
      return TYPE_IMAGE;
    }
  }
  return type;
}
