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
#include "nsTableCol.h"
#include "nsTableColFrame.h"
#include "nsTableColGroup.h"
#include "nsTablePart.h"
#include "nsHTMLParts.h"
#include "nsHTMLContainer.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"

#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
static PRBool gsNoisyRefs = PR_FALSE;
#else
static const PRBool gsDebug = PR_FALSE;
static const PRBool gsNoisyRefs = PR_FALSE;
#endif


nsTableCol::nsTableCol(nsIAtom* aTag)
  : nsTableContent(aTag),
  mColGroup(0),
  mColIndex(0),
  mRepeat(1)
{
  Init();
}

nsTableCol::nsTableCol()
: nsTableContent(NS_NewAtom(nsTablePart::kColTagString)),
  mColGroup(0),
  mColIndex(0),
  mRepeat(1)
{
  Init();
}

nsTableCol::nsTableCol (PRBool aImplicit)
  : nsTableContent(NS_NewAtom(nsTablePart::kColTagString)),
  mColGroup(0),
  mColIndex(0),
  mRepeat(1)
{
  mImplicit = aImplicit;
  Init();
}

void nsTableCol::Init()
{
}

nsTableCol::~nsTableCol()
{
}

// Added for debuging purposes -- remove from final build
nsrefcnt nsTableCol::AddRef(void)
{
  if (gsNoisyRefs==PR_TRUE) printf("Add Ref: %x, nsTableCol cnt = %d \n",this, mRefCnt+1);
  return ++mRefCnt;
}

nsrefcnt nsTableCol::Release(void)
{
  if (gsNoisyRefs==PR_TRUE) printf("Release: %x, nsTableCol cnt = %d \n",this,mRefCnt-1);
  if (--mRefCnt == 0) {
    if (gsNoisyRefs==PR_TRUE) printf("Delete: %x, nsTableCol \n",this);
    delete this;
    return 0;
  }
  return mRefCnt;
}

void nsTableCol::SetAttribute(nsIAtom* aAttribute, const nsString& aValue)
{
  nsHTMLValue val;

  if (aAttribute == nsHTMLAtoms::width) 
  {
    ParseValueOrPercentOrProportional(aValue, val, eHTMLUnit_Pixel);
    nsHTMLTagContent::SetAttribute(aAttribute, val);
  }
  else if ( aAttribute == nsHTMLAtoms::repeat)
  {
    ParseValue(aValue, 0, val, eHTMLUnit_Integer);
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    SetRepeat(val.GetIntValue());
  }
  else if (aAttribute == nsHTMLAtoms::align) {
    nsHTMLValue val;
    if (ParseTableAlignParam(aValue, val)) {
      nsHTMLTagContent::SetAttribute(aAttribute, val);
    }
    return;
  }
  else if (aAttribute == nsHTMLAtoms::valign) {
    nsHTMLValue val;
    if (ParseTableAlignParam(aValue, val)) {
      nsHTMLTagContent::SetAttribute(aAttribute, val);
    }
    return;
  }
  // unknown attributes are handled by my parent
  nsTableContent::SetAttribute(aAttribute, aValue);
}

void nsTableCol::MapAttributesInto(nsIStyleContext* aContext,
                                   nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull!=aContext, "bad style context arg");
  NS_PRECONDITION(nsnull!=aPresContext, "bad presentation context arg");
  if (nsnull != mAttributes) {

    float p2t;
    nsHTMLValue value;
    nsStyleText* textStyle = nsnull;

    // width
    GetAttribute(nsHTMLAtoms::width, value);
    if (value.GetUnit() != eHTMLUnit_Null) {
      nsStylePosition* position = (nsStylePosition*)
        aContext->GetMutableStyleData(eStyleStruct_Position);
      switch (value.GetUnit()) {
      case eHTMLUnit_Percent:
        position->mWidth.SetPercentValue(value.GetPercentValue());
        break;

      case eHTMLUnit_Pixel:
        p2t = aPresContext->GetPixelsToTwips();
        position->mWidth.SetCoordValue(NSIntPixelsToTwips(value.GetPixelValue(), p2t));
        break;
      }
    }

    // align: enum
    GetAttribute(nsHTMLAtoms::align, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) 
    {
      textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mTextAlign = value.GetIntValue();
    }
    
    // valign: enum
    GetAttribute(nsHTMLAtoms::valign, value);
    if (value.GetUnit() == eHTMLUnit_Enumerated) 
    {
      if (nsnull==textStyle)
        textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mVerticalAlign.SetIntValue(value.GetIntValue(), eStyleUnit_Enumerated);
    }
  }
}



nsresult
nsTableCol::CreateFrame(nsIPresContext* aPresContext,
                        nsIFrame* aParentFrame,
                        nsIStyleContext* aStyleContext,
                        nsIFrame*& aResult)
{
  NS_PRECONDITION(nsnull!=aPresContext, "bad arg");

  nsIFrame* frame;
  nsresult rv = nsTableColFrame::NewFrame(&frame, this, aParentFrame);
  if (NS_OK != rv) {
    return rv;
  }
  ((nsTableColFrame*)frame)->Init(mColIndex, mRepeat);
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return rv;
}

void nsTableCol::ResetColumns ()
{
  if (nsnull != mColGroup)
    mColGroup->ResetColumns ();
}

/* ---------- Global Functions ---------- */

nsresult
NS_NewTableColPart(nsIHTMLContent** aInstancePtrResult,
                   nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* body = new nsTableCol(aTag);
  if (nsnull == body) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return body->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}
