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
#include "nsTableCell.h"
#include "nsTableCellFrame.h"
#include "nsHTMLParts.h"
#include "nsIStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIPresContext.h"
#include "nsIDocument.h"
#include "nsIURL.h"
#include "nsHTMLIIDs.h"
#include "nsHTMLAtoms.h"
#include "nsIPtr.h"

// hack, remove when hack in nsTableCol constructor is removed
static PRInt32 HACKcounter=0;
static nsIAtom *HACKattribute=nsnull;
#include "prprf.h"  // remove when nsTableCol constructor hack is removed
// end hack code


NS_DEF_PTR(nsIStyleContext);

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

#ifdef NS_DEBUG
static PRBool gsDebug = PR_FALSE;
static PRBool gsNoisyRefs = PR_FALSE;
#else
static const PRBool gsDebug = PR_FALSE;
static const PRBool gsNoisyRefs = PR_FALSE;
#endif

// nsTableContent checks aTag
nsTableCell::nsTableCell(nsIAtom* aTag)
  : nsTableContent(aTag),
  mRow(0),
  mRowSpan(1),
  mColSpan(1),
  mColIndex(0)
{
  mImplicit = PR_FALSE;
  Init();
}

nsTableCell::nsTableCell(PRBool aImplicit)
  : nsTableContent(NS_NewAtom(nsTablePart::kDataCellTagString)),
  mRow(0),
  mRowSpan(1),
  mColSpan(1),
  mColIndex(0)
{
  mImplicit = aImplicit;
  Init();
}

// nsTableContent checks aTag
nsTableCell::nsTableCell (nsIAtom* aTag, int aRowSpan, int aColSpan)
  : nsTableContent(aTag),
  mRow(0),
  mRowSpan(aRowSpan),
  mColSpan(aColSpan),
  mColIndex(0)
{
  NS_ASSERTION(0<aRowSpan, "bad row span");
  NS_ASSERTION(0<aColSpan, "bad col span");
  mImplicit = PR_FALSE;
  Init();
}

void nsTableCell::Init()
{
  /* begin hack */
  // temporary hack to get around style sheet optimization that folds all
  // col style context into one, unless there is a unique HTML attribute set
  char out[40];
  PR_snprintf(out, 40, "%d", HACKcounter);
  const nsAutoString value(out);
  if (nsnull==HACKattribute)
    HACKattribute = NS_NewAtom("Steve's unbelievable hack attribute");
  SetAttribute(HACKattribute, value);
  HACKcounter++;
  /* end hack */
}

nsTableCell::~nsTableCell()
{
  NS_IF_RELEASE(mRow);
}

// for debugging only
nsrefcnt nsTableCell::AddRef(void)
{
  if (gsNoisyRefs) printf("Add Ref: %x, nsTableCell cnt = %d \n",this, mRefCnt+1);
  return ++mRefCnt;
}

// for debugging only
nsrefcnt nsTableCell::Release(void)
{
  if (gsNoisyRefs==PR_TRUE) printf("Release: %x, nsTableCell cnt = %d \n",this, mRefCnt-1);
  if (--mRefCnt == 0) {
    if (gsNoisyRefs==PR_TRUE) printf("Delete: %x, nsTableCell \n",this);
    delete this;
    return 0;
  }
  return mRefCnt;
}

int nsTableCell::GetType()
{
  return nsITableContent::kTableCellType;
}

int nsTableCell::GetRowSpan ()
{
  NS_ASSERTION(0<mRowSpan, "bad row span");
  return mRowSpan;
}

int nsTableCell::GetColSpan ()
{
  NS_ASSERTION(0<mColSpan, "bad col span");
  return mColSpan;
}

nsTableRow * nsTableCell::GetRow ()
{
  NS_IF_ADDREF(mRow);
  return mRow;
}

void nsTableCell::SetRow (nsTableRow * aRow)
{
  NS_IF_RELEASE(mRow);
  mRow = aRow;
  NS_IF_ADDREF(aRow);
}

int nsTableCell::GetColIndex ()
{
  NS_ASSERTION(0<=mColIndex, "bad column index");
  return mColIndex;
}

void nsTableCell::SetColIndex (int aColIndex)
{
  NS_ASSERTION(0<=aColIndex, "illegal negative column index.");
  mColIndex = aColIndex;
}

nsresult
nsTableCell::CreateFrame(nsIPresContext* aPresContext,
                         nsIFrame* aParentFrame,
                         nsIStyleContext* aStyleContext,
                         nsIFrame*& aResult)
{
  NS_PRECONDITION(nsnull!=aPresContext, "bad arg");

  nsIFrame* frame;
  nsresult rv = nsTableCellFrame::NewFrame(&frame, this, aParentFrame);
  if (NS_OK != rv) {
    return rv;
  }
  ((nsTableCellFrame*)frame)->Init(mRowSpan, mColSpan, mColIndex);
  frame->SetStyleContext(aPresContext, aStyleContext);
  aResult = frame;
  return rv;
}

void nsTableCell::SetAttribute(nsIAtom* aAttribute, const nsString& aValue)
{
  NS_PRECONDITION(nsnull!=aAttribute, "bad attribute arg");
  nsHTMLValue val;
  if ((aAttribute == nsHTMLAtoms::align) &&
      ParseDivAlignParam(aValue, val)) {
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if ((aAttribute == nsHTMLAtoms::valign) &&
      ParseAlignParam(aValue, val)) {
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::background) {
    nsAutoString href(aValue);
    href.StripWhitespace();
    nsHTMLTagContent::SetAttribute(aAttribute, href);
    return;
  }
  if (aAttribute == nsHTMLAtoms::bgcolor) {
    ParseColor(aValue, val);
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::rowspan) {
    ParseValue(aValue, 1, 10000, val, eHTMLUnit_Integer);
    SetRowSpan(val.GetIntValue());
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::colspan) {
    ParseValue(aValue, 1, 1000, val, eHTMLUnit_Integer);
    SetColSpan(val.GetIntValue());
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::width) {
    ParseValueOrPercent(aValue, val, eHTMLUnit_Pixel);
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::height) {
    ParseValueOrPercent(aValue, val, eHTMLUnit_Pixel);
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  if (aAttribute == nsHTMLAtoms::nowrap) {
    val.SetEmptyValue();
    nsHTMLTagContent::SetAttribute(aAttribute, val);
    return;
  }
  // Use default attribute catching code
  nsTableContent::SetAttribute(aAttribute, aValue);
}



void nsTableCell::MapAttributesInto(nsIStyleContext* aContext,
                                    nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull!=aContext, "bad style context arg");
  NS_PRECONDITION(nsnull!=aPresContext, "bad presentation context arg");

  nsHTMLValue value;
  nsHTMLValue widthValue;
  nsStyleText* textStyle = nsnull;

  // align: enum
  GetAttribute(nsHTMLAtoms::align, value);
  if (value.GetUnit() == eHTMLUnit_Enumerated) 
  {
    textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
    textStyle->mTextAlign = value.GetIntValue();
  }
  // otherwise check the row for align and inherit it
  else {
    if (nsnull!=mRow)
    {
      // TODO: optimize by putting a flag on the row to say whether align attr is set
      nsHTMLValue parentAlignValue;
      mRow->GetAttribute(nsHTMLAtoms::align, parentAlignValue);
      if (parentAlignValue.GetUnit() == eHTMLUnit_Enumerated)
      {
        PRUint8 rowAlign = parentAlignValue.GetIntValue();
        if (nsnull==textStyle)
          textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
        textStyle->mTextAlign = rowAlign;
      }
      else
      { // we need to check the row group as well
        nsTableRowGroup *rowGroup = mRow->GetRowGroup();
        if (nsnull!=rowGroup)
        {
          rowGroup->GetAttribute(nsHTMLAtoms::align, parentAlignValue);
          if (parentAlignValue.GetUnit() == eHTMLUnit_Enumerated)
          {
            PRUint8 rowGroupAlign = parentAlignValue.GetIntValue();
            if (nsnull==textStyle)
              textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
            textStyle->mTextAlign = rowGroupAlign;
          }
          NS_RELEASE(rowGroup);
        }
      }
    }
  }
  
  // valign: enum
  GetAttribute(nsHTMLAtoms::valign, value);
  if (value.GetUnit() == eHTMLUnit_Enumerated) 
  {
    if (nsnull==textStyle)
      textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
    textStyle->mVerticalAlign.SetIntValue(value.GetIntValue(), eStyleUnit_Enumerated);
  }
  // otherwise check the row for valign and inherit it
  else {
    if (nsnull!=mRow)
    {
      // TODO: optimize by putting a flag on the row to say whether valign attr is set
      nsHTMLValue parentAlignValue;
      mRow->GetAttribute(nsHTMLAtoms::valign, parentAlignValue);
      if (parentAlignValue.GetUnit() == eHTMLUnit_Enumerated)
      {
        PRUint8 rowVAlign = parentAlignValue.GetIntValue();
        if (nsnull==textStyle)
          textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
        textStyle->mVerticalAlign.SetIntValue(rowVAlign, eStyleUnit_Enumerated);
      }
      else
      { // we need to check the row group as well
        nsTableRowGroup *rowGroup = mRow->GetRowGroup();
        if (nsnull!=rowGroup)
        {
          rowGroup->GetAttribute(nsHTMLAtoms::valign, parentAlignValue);
          if (parentAlignValue.GetUnit() == eHTMLUnit_Enumerated)
          {
            PRUint8 rowGroupVAlign = parentAlignValue.GetIntValue();
            if (nsnull==textStyle)
              textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
            textStyle->mVerticalAlign.SetIntValue(rowGroupVAlign, eStyleUnit_Enumerated);
          }
          NS_RELEASE(rowGroup);
        }
      }
    }
  }

  MapBackgroundAttributesInto(aContext, aPresContext);

  // width: pixel
  float p2t = aPresContext->GetPixelsToTwips();
  nsStylePosition* pos = (nsStylePosition*)
    aContext->GetMutableStyleData(eStyleStruct_Position);
  GetAttribute(nsHTMLAtoms::width, widthValue);
  if (widthValue.GetUnit() == eHTMLUnit_Pixel) {
    nscoord twips = NSIntPixelsToTwips(widthValue.GetPixelValue(), p2t);
    pos->mWidth.SetCoordValue(twips);
  }
  else if (widthValue.GetUnit() == eHTMLUnit_Percent) {
    pos->mWidth.SetPercentValue(widthValue.GetPercentValue());
  }

  // height: pixel
  GetAttribute(nsHTMLAtoms::height, value);
  if (value.GetUnit() == eHTMLUnit_Pixel) {
    nscoord twips = NSIntPixelsToTwips(value.GetPixelValue(), p2t);
    pos->mHeight.SetCoordValue(twips);
  }

  // nowrap
  // nowrap depends on the width attribute, so be sure to handle it after width is mapped!
  GetAttribute(nsHTMLAtoms::nowrap, value);
  if (value.GetUnit() == eHTMLUnit_Empty)
  {
    if (widthValue.GetUnit() != eHTMLUnit_Pixel)
    {
      if (nsnull==textStyle)
        textStyle = (nsStyleText*)aContext->GetMutableStyleData(eStyleStruct_Text);
      textStyle->mWhiteSpace = NS_STYLE_WHITESPACE_NOWRAP;
    }
  }
}

void
nsTableCell::MapBackgroundAttributesInto(nsIStyleContext* aContext,
                                         nsIPresContext* aPresContext)
{
  nsHTMLValue value;
  nsStyleColor* color=nsnull;

  // background
  if (eContentAttr_HasValue == GetAttribute(nsHTMLAtoms::background, value)) {
    if (eHTMLUnit_String == value.GetUnit()) {
      color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
      SetBackgroundFromAttribute(color, &value);
    }
  }
  // otherwise check the row for background and inherit it
  else 
  {
    if (nsnull!=mRow)
    {
      // TODO: optimize by putting a flag on the row to say whether background attr is set
      mRow->GetAttribute(nsHTMLAtoms::background, value);
      if (value.GetUnit() == eHTMLUnit_String)
      {
        nsString rowBackground;
        value.GetStringValue(rowBackground);
        if (nsnull==color)
          color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
        SetBackgroundFromAttribute(color, &value);
      }
      else
      { // we need to check the row group as well
        nsTableRowGroup *rowGroup = mRow->GetRowGroup();
        if (nsnull!=rowGroup)
        {
          rowGroup->GetAttribute(nsHTMLAtoms::background, value);
          if (value.GetUnit() == eHTMLUnit_String)
          {
            nsString rowBackground;
            value.GetStringValue(rowBackground);
            if (nsnull==color)
              color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
            SetBackgroundFromAttribute(color, &value);
          }
          NS_RELEASE(rowGroup);
        }
      }
    }
  }

  // bgcolor
  if (eContentAttr_HasValue == GetAttribute(nsHTMLAtoms::bgcolor, value)) {
    if (eHTMLUnit_Color == value.GetUnit()) {
      if (nsnull==color)
        color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
      color->mBackgroundColor = value.GetColorValue();
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
    else if (eHTMLUnit_String == value.GetUnit()) {
      nsAutoString buffer;
      value.GetStringValue(buffer);
      char cbuf[40];
      buffer.ToCString(cbuf, sizeof(cbuf));

      if (nsnull==color)
        color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
      NS_ColorNameToRGB(cbuf, &(color->mBackgroundColor));
      color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
    }
  }
  // otherwise check the row for background and inherit it
  else 
  {
    if (nsnull!=mRow)
    {
      // TODO: optimize by putting a flag on the row to say whether bgcolor attr is set
      if (eContentAttr_HasValue == mRow->GetAttribute(nsHTMLAtoms::bgcolor, value))
      {
        if (eHTMLUnit_Color == value.GetUnit()) {
          if (nsnull==color)
            color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
          color->mBackgroundColor = value.GetColorValue();
          color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
        }
        else if (eHTMLUnit_String == value.GetUnit()) {
          nsAutoString buffer;
          value.GetStringValue(buffer);
          char cbuf[40];
          buffer.ToCString(cbuf, sizeof(cbuf));
          if (nsnull==color)
            color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
          NS_ColorNameToRGB(cbuf, &(color->mBackgroundColor));
          color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
        }
      }
      else
      { // we need to check the row group as well
        nsTableRowGroup *rowGroup = mRow->GetRowGroup();
        if (nsnull!=rowGroup)
        {
          if (eContentAttr_HasValue == rowGroup->GetAttribute(nsHTMLAtoms::bgcolor, value)) {
            if (eHTMLUnit_Color == value.GetUnit()) {
              if (nsnull==color)
                color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
              color->mBackgroundColor = value.GetColorValue();
              color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
            }
            else if (eHTMLUnit_String == value.GetUnit()) {
              nsAutoString buffer;
              value.GetStringValue(buffer);
              char cbuf[40];
              buffer.ToCString(cbuf, sizeof(cbuf));

              if (nsnull==color)
                color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);
              NS_ColorNameToRGB(cbuf, &(color->mBackgroundColor));
              color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
            }
          }
          NS_RELEASE(rowGroup);
        }
      }
    }
  }
}

void nsTableCell::SetBackgroundFromAttribute(nsStyleColor *aColor, nsHTMLValue *aValue)
{
  NS_ASSERTION(nsnull!=aColor && nsnull!=aValue, "bad args");

  // Resolve url to an absolute url
  nsIURL* docURL = nsnull;
  nsIDocument* doc = mDocument;
  if (nsnull != doc) {
    docURL = doc->GetDocumentURL();
  }

  nsAutoString absURLSpec;
  nsAutoString spec;
  aValue->GetStringValue(spec);
  nsresult rv = NS_MakeAbsoluteURL(docURL, "", spec, absURLSpec);
  if (nsnull != docURL) {
    NS_RELEASE(docURL);
  }
  aColor->mBackgroundImage = absURLSpec;
  aColor->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
  aColor->mBackgroundRepeat = NS_STYLE_BG_REPEAT_XY;
}

nsContentAttr
nsTableCell::AttributeToString(nsIAtom* aAttribute,
                               nsHTMLValue& aValue,
                               nsString& aResult) const
{
  nsContentAttr ca = eContentAttr_NotThere;
  if (aAttribute == nsHTMLAtoms::valign) {
    AlignParamToString(aValue, aResult);
    ca = eContentAttr_HasValue;
  }
  else {
    ca = nsTableContent::AttributeToString(aAttribute, aValue, aResult);
  }
  return ca;
}

void nsTableCell::SetRowSpan(int aRowSpan)
{
  NS_ASSERTION(0<aRowSpan, "bad row span");
  int oldSpan = mRowSpan;
  mRowSpan = aRowSpan;
}

void nsTableCell::SetColSpan (int aColSpan)
{
  NS_ASSERTION(0<aColSpan, "bad col span");
  int oldSpan = mColSpan;
  mColSpan = aColSpan;
}

nsresult
NS_NewTableCellPart(nsIHTMLContent** aInstancePtrResult,
                    nsIAtom* aTag)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsIHTMLContent* body = new nsTableCell(aTag);
  if (nsnull == body) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return body->QueryInterface(kIHTMLContentIID, (void **) aInstancePtrResult);
}
