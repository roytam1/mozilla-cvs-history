/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Glazman <glazman@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nscore.h"
#include "nsCSSStruct.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsCSSProps.h"
#include "nsUnitConversion.h"
#include "nsVoidArray.h"
#include "nsFont.h"

#include "nsStyleConsts.h"

#include "nsCOMPtr.h"
#include "nsIStyleSet.h"

static NS_DEFINE_IID(kCSSFontSID, NS_CSS_FONT_SID);
static NS_DEFINE_IID(kCSSColorSID, NS_CSS_COLOR_SID);
static NS_DEFINE_IID(kCSSDisplaySID, NS_CSS_DISPLAY_SID);
static NS_DEFINE_IID(kCSSTextSID, NS_CSS_TEXT_SID);
static NS_DEFINE_IID(kCSSMarginSID, NS_CSS_MARGIN_SID);
static NS_DEFINE_IID(kCSSPositionSID, NS_CSS_POSITION_SID);
static NS_DEFINE_IID(kCSSListSID, NS_CSS_LIST_SID);
static NS_DEFINE_IID(kCSSTableSID, NS_CSS_TABLE_SID);
static NS_DEFINE_IID(kCSSBreaksSID, NS_CSS_BREAKS_SID);
static NS_DEFINE_IID(kCSSPageSID, NS_CSS_PAGE_SID);
static NS_DEFINE_IID(kCSSContentSID, NS_CSS_CONTENT_SID);
static NS_DEFINE_IID(kCSSUserInterfaceSID, NS_CSS_USER_INTERFACE_SID);
static NS_DEFINE_IID(kCSSAuralSID, NS_CSS_AURAL_SID);
static NS_DEFINE_IID(kCSSXULSID, NS_CSS_XUL_SID);

#ifdef MOZ_SVG
static NS_DEFINE_IID(kCSSSVGSID, NS_CSS_SVG_SID);
#endif

#define CSS_IF_DELETE(ptr)  if (nsnull != ptr)  { delete ptr; ptr = nsnull; }

// --- nsCSSFont -----------------

nsCSSFont::nsCSSFont(void)
{
  MOZ_COUNT_CTOR(nsCSSFont);
}

nsCSSFont::nsCSSFont(const nsCSSFont& aCopy)
  : mFamily(aCopy.mFamily),
    mStyle(aCopy.mStyle),
    mVariant(aCopy.mVariant),
    mWeight(aCopy.mWeight),
    mSize(aCopy.mSize),
    mSizeAdjust(aCopy.mSizeAdjust),
    mStretch(aCopy.mStretch)
{
  MOZ_COUNT_CTOR(nsCSSFont);
}

nsCSSFont::~nsCSSFont(void)
{
  MOZ_COUNT_DTOR(nsCSSFont);
}

const nsID& nsCSSFont::GetID(void)
{
  return kCSSFontSID;
}

#ifdef DEBUG
void nsCSSFont::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mFamily.AppendToString(buffer, eCSSProperty_font_family);
  mStyle.AppendToString(buffer, eCSSProperty_font_style);
  mVariant.AppendToString(buffer, eCSSProperty_font_variant);
  mWeight.AppendToString(buffer, eCSSProperty_font_weight);
  mSize.AppendToString(buffer, eCSSProperty_font_size);
  mSizeAdjust.AppendToString(buffer, eCSSProperty_font_size_adjust);
  mStretch.AppendToString(buffer, eCSSProperty_font_stretch);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- support -----------------

#define CSS_IF_COPY(val, type) \
  if (aCopy.val) (val) = new type(*(aCopy.val));

nsCSSValueList::nsCSSValueList(void)
  : mValue(),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSValueList);
}

nsCSSValueList::nsCSSValueList(const nsCSSValueList& aCopy)
  : mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSValueList);
  CSS_IF_COPY(mNext, nsCSSValueList);
}

nsCSSValueList::~nsCSSValueList(void)
{
  MOZ_COUNT_DTOR(nsCSSValueList);
  CSS_IF_DELETE(mNext);
}

// --- nsCSSColor -----------------

nsCSSColor::nsCSSColor(void)
{
  MOZ_COUNT_CTOR(nsCSSColor);
}

nsCSSColor::nsCSSColor(const nsCSSColor& aCopy)
  : mColor(aCopy.mColor),
    mBackColor(aCopy.mBackColor),
    mBackImage(aCopy.mBackImage),
    mBackRepeat(aCopy.mBackRepeat),
    mBackAttachment(aCopy.mBackAttachment),
    mBackPositionX(aCopy.mBackPositionX),
    mBackPositionY(aCopy.mBackPositionY),
    mBackClip(aCopy.mBackClip),
    mBackOrigin(aCopy.mBackOrigin),
    mBackInlinePolicy(aCopy.mBackInlinePolicy)

{
  MOZ_COUNT_CTOR(nsCSSColor);
}

nsCSSColor::~nsCSSColor(void)
{
  MOZ_COUNT_DTOR(nsCSSColor);
}

const nsID& nsCSSColor::GetID(void)
{
  return kCSSColorSID;
}

#ifdef DEBUG
void nsCSSColor::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mColor.AppendToString(buffer, eCSSProperty_color);
  mBackColor.AppendToString(buffer, eCSSProperty_background_color);
  mBackImage.AppendToString(buffer, eCSSProperty_background_image);
  mBackRepeat.AppendToString(buffer, eCSSProperty_background_repeat);
  mBackAttachment.AppendToString(buffer, eCSSProperty_background_attachment);
  mBackPositionX.AppendToString(buffer, eCSSProperty_background_x_position);
  mBackPositionY.AppendToString(buffer, eCSSProperty_background_y_position);
  mBackClip.AppendToString(buffer, eCSSProperty__moz_background_clip);
  mBackOrigin.AppendToString(buffer, eCSSProperty__moz_background_origin);
  mBackInlinePolicy.AppendToString(buffer, eCSSProperty__moz_background_inline_policy);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSText support -----------------

nsCSSShadow::nsCSSShadow(void)
  : mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSShadow);
}

nsCSSShadow::nsCSSShadow(const nsCSSShadow& aCopy)
  : mColor(aCopy.mColor),
    mXOffset(aCopy.mXOffset),
    mYOffset(aCopy.mYOffset),
    mRadius(aCopy.mRadius),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSShadow);
  CSS_IF_COPY(mNext, nsCSSShadow);
}

nsCSSShadow::~nsCSSShadow(void)
{
  MOZ_COUNT_DTOR(nsCSSShadow);
  CSS_IF_DELETE(mNext);
}

// --- nsCSSText -----------------

nsCSSText::nsCSSText(void)
  : mTextShadow(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSText);
}

nsCSSText::nsCSSText(const nsCSSText& aCopy)
  : mWordSpacing(aCopy.mWordSpacing),
    mLetterSpacing(aCopy.mLetterSpacing),
    mVerticalAlign(aCopy.mVerticalAlign),
    mTextTransform(aCopy.mTextTransform),
    mTextAlign(aCopy.mTextAlign),
    mTextIndent(aCopy.mTextIndent),
    mDecoration(aCopy.mDecoration),
    mTextShadow(nsnull),
    mUnicodeBidi(aCopy.mUnicodeBidi),
    mLineHeight(aCopy.mLineHeight),
    mWhiteSpace(aCopy.mWhiteSpace)
{
  MOZ_COUNT_CTOR(nsCSSText);
}

nsCSSText::~nsCSSText(void)
{
  MOZ_COUNT_DTOR(nsCSSText);
  CSS_IF_DELETE(mTextShadow);
}

const nsID& nsCSSText::GetID(void)
{
  return kCSSTextSID;
}

#ifdef DEBUG
void nsCSSText::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mWordSpacing.AppendToString(buffer, eCSSProperty_word_spacing);
  mLetterSpacing.AppendToString(buffer, eCSSProperty_letter_spacing);
  mDecoration.AppendToString(buffer, eCSSProperty_text_decoration);
  mVerticalAlign.AppendToString(buffer, eCSSProperty_vertical_align);
  mTextTransform.AppendToString(buffer, eCSSProperty_text_transform);
  mTextAlign.AppendToString(buffer, eCSSProperty_text_align);
  mTextIndent.AppendToString(buffer, eCSSProperty_text_indent);
  if (nsnull != mTextShadow) {
    if (mTextShadow->mXOffset.IsLengthUnit()) {
      nsCSSShadow*  shadow = mTextShadow;
      while (nsnull != shadow) {
        shadow->mColor.AppendToString(buffer, eCSSProperty_text_shadow_color);
        shadow->mXOffset.AppendToString(buffer, eCSSProperty_text_shadow_x);
        shadow->mYOffset.AppendToString(buffer, eCSSProperty_text_shadow_y);
        shadow->mRadius.AppendToString(buffer, eCSSProperty_text_shadow_radius);
        shadow = shadow->mNext;
      }
    }
    else {
      mTextShadow->mXOffset.AppendToString(buffer, eCSSProperty_text_shadow);
    }
  }
  mUnicodeBidi.AppendToString(buffer, eCSSProperty_unicode_bidi);
  mLineHeight.AppendToString(buffer, eCSSProperty_line_height);
  mWhiteSpace.AppendToString(buffer, eCSSProperty_white_space);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSRect -----------------

nsCSSRect::nsCSSRect(void)
{
  MOZ_COUNT_CTOR(nsCSSRect);
}

nsCSSRect::nsCSSRect(const nsCSSRect& aCopy)
  : mTop(aCopy.mTop),
    mRight(aCopy.mRight),
    mBottom(aCopy.mBottom),
    mLeft(aCopy.mLeft)
{
  MOZ_COUNT_CTOR(nsCSSRect);
}

nsCSSRect::~nsCSSRect()
{
  MOZ_COUNT_DTOR(nsCSSRect);
}


#ifdef DEBUG
void nsCSSRect::List(FILE* out, nsCSSProperty aPropID, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  if (eCSSProperty_UNKNOWN < aPropID) {
    buffer.AppendWithConversion(nsCSSProps::GetStringValue(aPropID).get());
    buffer.Append(NS_LITERAL_STRING(": "));
  }

  mTop.AppendToString(buffer);
  mRight.AppendToString(buffer);
  mBottom.AppendToString(buffer); 
  mLeft.AppendToString(buffer);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}

void nsCSSRect::List(FILE* out, PRInt32 aIndent, const nsCSSProperty aTRBL[]) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  if (eCSSUnit_Null != mTop.GetUnit()) {
    buffer.AppendWithConversion(nsCSSProps::GetStringValue(aTRBL[0]).get());
    buffer.Append(NS_LITERAL_STRING(": "));
    mTop.AppendToString(buffer);
  }
  if (eCSSUnit_Null != mRight.GetUnit()) {
    buffer.AppendWithConversion(nsCSSProps::GetStringValue(aTRBL[1]).get());
    buffer.Append(NS_LITERAL_STRING(": "));
    mRight.AppendToString(buffer);
  }
  if (eCSSUnit_Null != mBottom.GetUnit()) {
    buffer.AppendWithConversion(nsCSSProps::GetStringValue(aTRBL[2]).get());
    buffer.Append(NS_LITERAL_STRING(": "));
    mBottom.AppendToString(buffer); 
  }
  if (eCSSUnit_Null != mLeft.GetUnit()) {
    buffer.AppendWithConversion(nsCSSProps::GetStringValue(aTRBL[3]).get());
    buffer.Append(NS_LITERAL_STRING(": "));
    mLeft.AppendToString(buffer);
  }

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSDisplay -----------------

nsCSSDisplay::nsCSSDisplay(void)
  : mClip(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSDisplay);
}

nsCSSDisplay::nsCSSDisplay(const nsCSSDisplay& aCopy)
  : mDirection(aCopy.mDirection),
    mDisplay(aCopy.mDisplay),
    mBinding(aCopy.mBinding),
    mPosition(aCopy.mPosition),
    mFloat(aCopy.mFloat),
    mClear(aCopy.mClear),
    mClip(nsnull),
    mOverflow(aCopy.mOverflow),
    mVisibility(aCopy.mVisibility),
    mOpacity(aCopy.mOpacity),
    // temp fix for bug 24000
    mBreakBefore(aCopy.mBreakBefore),
    mBreakAfter(aCopy.mBreakAfter)
    // end temp
{
  MOZ_COUNT_CTOR(nsCSSDisplay);
  CSS_IF_COPY(mClip, nsCSSRect);
}

nsCSSDisplay::~nsCSSDisplay(void)
{
  MOZ_COUNT_DTOR(nsCSSDisplay);
  CSS_IF_DELETE(mClip);
}

const nsID& nsCSSDisplay::GetID(void)
{
  return kCSSDisplaySID;
}

#ifdef DEBUG
void nsCSSDisplay::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mAppearance.AppendToString(buffer, eCSSProperty_appearance);
  mDirection.AppendToString(buffer, eCSSProperty_direction);
  mDisplay.AppendToString(buffer, eCSSProperty_display);
  mBinding.AppendToString(buffer, eCSSProperty_binding);
  mPosition.AppendToString(buffer, eCSSProperty_position);
  mFloat.AppendToString(buffer, eCSSProperty_float);
  mClear.AppendToString(buffer, eCSSProperty_clear);
  mVisibility.AppendToString(buffer, eCSSProperty_visibility);
  mOpacity.AppendToString(buffer, eCSSProperty_opacity);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
  if (nsnull != mClip) {
    mClip->List(out, eCSSProperty_clip);
  }
  buffer.SetLength(0);
  mOverflow.AppendToString(buffer, eCSSProperty_overflow);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSMargin -----------------

void nsCSSMargin::EnsureBorderColors()
{
  if (!mBorderColors) {
    PRInt32 i;
    mBorderColors = new nsCSSValueList*[4];
    for (i = 0; i < 4; i++)
      mBorderColors[i] = nsnull;
  }
}

nsCSSMargin::nsCSSMargin(void)
  : mMargin(nsnull), mPadding(nsnull), 
    mBorderWidth(nsnull), mBorderColor(nsnull), mBorderColors(nsnull),
    mBorderStyle(nsnull), mBorderRadius(nsnull), mOutlineRadius(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSMargin);
}

nsCSSMargin::nsCSSMargin(const nsCSSMargin& aCopy)
  : mMargin(nsnull), mPadding(nsnull), 
    mBorderWidth(nsnull), mBorderColor(nsnull), mBorderColors(nsnull),
    mBorderStyle(nsnull), mBorderRadius(nsnull),
    mOutlineWidth(aCopy.mOutlineWidth),
    mOutlineColor(aCopy.mOutlineColor),
    mOutlineStyle(aCopy.mOutlineStyle),
    mOutlineRadius(nsnull),
    mFloatEdge(aCopy.mFloatEdge)
{
  MOZ_COUNT_CTOR(nsCSSMargin);
  CSS_IF_COPY(mMargin, nsCSSRect);
  CSS_IF_COPY(mPadding, nsCSSRect);
  CSS_IF_COPY(mBorderWidth, nsCSSRect);
  CSS_IF_COPY(mBorderColor, nsCSSRect);
  CSS_IF_COPY(mBorderStyle, nsCSSRect);
  CSS_IF_COPY(mBorderRadius, nsCSSRect);
  CSS_IF_COPY(mOutlineRadius, nsCSSRect);
  if (aCopy.mBorderColors) {
    EnsureBorderColors();
    for (PRInt32 i = 0; i < 4; i++)
      CSS_IF_COPY(mBorderColors[i], nsCSSValueList);
  }
}

nsCSSMargin::~nsCSSMargin(void)
{
  MOZ_COUNT_DTOR(nsCSSMargin);
  CSS_IF_DELETE(mMargin);
  CSS_IF_DELETE(mPadding);
  CSS_IF_DELETE(mBorderWidth);
  CSS_IF_DELETE(mBorderColor);
  CSS_IF_DELETE(mBorderStyle);
  CSS_IF_DELETE(mBorderRadius);
  CSS_IF_DELETE(mOutlineRadius);
  if (mBorderColors) {
    for (PRInt32 i = 0; i < 4; i++)
      CSS_IF_DELETE(mBorderColors[i]);
    delete []mBorderColors;
  }
}

const nsID& nsCSSMargin::GetID(void)
{
  return kCSSMarginSID;
}

#ifdef DEBUG
void nsCSSMargin::List(FILE* out, PRInt32 aIndent) const
{
  if (nsnull != mMargin) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty_margin_top,
      eCSSProperty_margin_right,
      eCSSProperty_margin_bottom,
      eCSSProperty_margin_left
    };
    mMargin->List(out, aIndent, trbl);
  }
  if (nsnull != mPadding) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty_padding_top,
      eCSSProperty_padding_right,
      eCSSProperty_padding_bottom,
      eCSSProperty_padding_left
    };
    mPadding->List(out, aIndent, trbl);
  }
  if (nsnull != mBorderWidth) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty_border_top_width,
      eCSSProperty_border_right_width,
      eCSSProperty_border_bottom_width,
      eCSSProperty_border_left_width
    };
    mBorderWidth->List(out, aIndent, trbl);
  }
  if (nsnull != mBorderColor) {
    mBorderColor->List(out, eCSSProperty_border_color, aIndent);
  }
  if (nsnull != mBorderStyle) {
    mBorderStyle->List(out, eCSSProperty_border_style, aIndent);
  }
  if (nsnull != mBorderRadius) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty__moz_border_radius_topLeft,
      eCSSProperty__moz_border_radius_topRight,
      eCSSProperty__moz_border_radius_bottomRight,
      eCSSProperty__moz_border_radius_bottomLeft
    };
    mBorderRadius->List(out, aIndent, trbl);
  }

  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);
 
  nsAutoString  buffer;
  mOutlineWidth.AppendToString(buffer, eCSSProperty__moz_outline_width);
  mOutlineColor.AppendToString(buffer, eCSSProperty__moz_outline_color);
  mOutlineStyle.AppendToString(buffer, eCSSProperty__moz_outline_style);
  if (nsnull != mOutlineRadius) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty__moz_outline_radius_topLeft,
      eCSSProperty__moz_outline_radius_topRight,
      eCSSProperty__moz_outline_radius_bottomRight,
      eCSSProperty__moz_outline_radius_bottomLeft
    };
    mOutlineRadius->List(out, aIndent, trbl);
  }
  mFloatEdge.AppendToString(buffer, eCSSProperty_float_edge);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSPosition -----------------

nsCSSPosition::nsCSSPosition(void)
  : mOffset(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSPosition);
}

nsCSSPosition::nsCSSPosition(const nsCSSPosition& aCopy)
  : mWidth(aCopy.mWidth),
    mMinWidth(aCopy.mMinWidth),
    mMaxWidth(aCopy.mMaxWidth),
    mHeight(aCopy.mHeight),
    mMinHeight(aCopy.mMinHeight),
    mMaxHeight(aCopy.mMaxHeight),
    mBoxSizing(aCopy.mBoxSizing),
    mOffset(nsnull),
    mZIndex(aCopy.mZIndex)
{
  MOZ_COUNT_CTOR(nsCSSPosition);
  CSS_IF_COPY(mOffset, nsCSSRect);
}

nsCSSPosition::~nsCSSPosition(void)
{
  MOZ_COUNT_DTOR(nsCSSPosition);
  CSS_IF_DELETE(mOffset);
}

const nsID& nsCSSPosition::GetID(void)
{
  return kCSSPositionSID;
}

#ifdef DEBUG
void nsCSSPosition::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mWidth.AppendToString(buffer, eCSSProperty_width);
  mMinWidth.AppendToString(buffer, eCSSProperty_min_width);
  mMaxWidth.AppendToString(buffer, eCSSProperty_max_width);
  mHeight.AppendToString(buffer, eCSSProperty_height);
  mMinHeight.AppendToString(buffer, eCSSProperty_min_height);
  mMaxHeight.AppendToString(buffer, eCSSProperty_max_height);
  mBoxSizing.AppendToString(buffer, eCSSProperty_box_sizing);
  mZIndex.AppendToString(buffer, eCSSProperty_z_index);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);

  if (nsnull != mOffset) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty_top,
      eCSSProperty_right,
      eCSSProperty_bottom,
      eCSSProperty_left
    };
    mOffset->List(out, aIndent, trbl);
  }
}
#endif

// --- nsCSSList -----------------

nsCSSList::nsCSSList(void)
:mImageRegion(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSList);
}

nsCSSList::nsCSSList(const nsCSSList& aCopy)
  : mType(aCopy.mType),
    mImage(aCopy.mImage),
    mPosition(aCopy.mPosition),
    mImageRegion(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSList);
  CSS_IF_COPY(mImageRegion, nsCSSRect);
}

nsCSSList::~nsCSSList(void)
{
  MOZ_COUNT_DTOR(nsCSSList);
  CSS_IF_DELETE(mImageRegion);
}

const nsID& nsCSSList::GetID(void)
{
  return kCSSListSID;
}

#ifdef DEBUG
void nsCSSList::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mType.AppendToString(buffer, eCSSProperty_list_style_type);
  mImage.AppendToString(buffer, eCSSProperty_list_style_image);
  mPosition.AppendToString(buffer, eCSSProperty_list_style_position);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);

  if (mImageRegion) {
    static const nsCSSProperty trbl[] = {
      eCSSProperty_top,
      eCSSProperty_right,
      eCSSProperty_bottom,
      eCSSProperty_left
    };
    mImageRegion->List(out, aIndent, trbl);
  }
}
#endif

// --- nsCSSTable -----------------

nsCSSTable::nsCSSTable(void)
{
  MOZ_COUNT_CTOR(nsCSSTable);
}

nsCSSTable::nsCSSTable(const nsCSSTable& aCopy)
  : mBorderCollapse(aCopy.mBorderCollapse),
    mBorderSpacingX(aCopy.mBorderSpacingX),
    mBorderSpacingY(aCopy.mBorderSpacingY),
    mCaptionSide(aCopy.mCaptionSide),
    mEmptyCells(aCopy.mEmptyCells),
    mLayout(aCopy.mLayout)
{
  MOZ_COUNT_CTOR(nsCSSTable);
}

nsCSSTable::~nsCSSTable(void)
{
  MOZ_COUNT_DTOR(nsCSSTable);
}

const nsID& nsCSSTable::GetID(void)
{
  return kCSSTableSID;
}

#ifdef DEBUG
void nsCSSTable::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mBorderCollapse.AppendToString(buffer, eCSSProperty_border_collapse);
  mBorderSpacingX.AppendToString(buffer, eCSSProperty_border_x_spacing);
  mBorderSpacingY.AppendToString(buffer, eCSSProperty_border_y_spacing);
  mCaptionSide.AppendToString(buffer, eCSSProperty_caption_side);
  mEmptyCells.AppendToString(buffer, eCSSProperty_empty_cells);
  mLayout.AppendToString(buffer, eCSSProperty_table_layout);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSBreaks -----------------

nsCSSBreaks::nsCSSBreaks(void)
{
  MOZ_COUNT_CTOR(nsCSSBreaks);
}

nsCSSBreaks::nsCSSBreaks(const nsCSSBreaks& aCopy)
  : mOrphans(aCopy.mOrphans),
    mWidows(aCopy.mWidows),
    mPage(aCopy.mPage),
    mPageBreakAfter(aCopy.mPageBreakAfter),
    mPageBreakBefore(aCopy.mPageBreakBefore),
    mPageBreakInside(aCopy.mPageBreakInside)
{
  MOZ_COUNT_CTOR(nsCSSBreaks);
}

nsCSSBreaks::~nsCSSBreaks(void)
{
  MOZ_COUNT_DTOR(nsCSSBreaks);
}

const nsID& nsCSSBreaks::GetID(void)
{
  return kCSSBreaksSID;
}

#ifdef DEBUG
void nsCSSBreaks::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mOrphans.AppendToString(buffer, eCSSProperty_orphans);
  mWidows.AppendToString(buffer, eCSSProperty_widows);
  mPage.AppendToString(buffer, eCSSProperty_page);
  mPageBreakAfter.AppendToString(buffer, eCSSProperty_page_break_after);
  mPageBreakBefore.AppendToString(buffer, eCSSProperty_page_break_before);
  mPageBreakInside.AppendToString(buffer, eCSSProperty_page_break_inside);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSPage -----------------

nsCSSPage::nsCSSPage(void)
{
  MOZ_COUNT_CTOR(nsCSSPage);
}

nsCSSPage::nsCSSPage(const nsCSSPage& aCopy)
  : mMarks(aCopy.mMarks),
    mSizeWidth(aCopy.mSizeWidth),
    mSizeHeight(aCopy.mSizeHeight)
{
  MOZ_COUNT_CTOR(nsCSSPage);
}

nsCSSPage::~nsCSSPage(void)
{
  MOZ_COUNT_DTOR(nsCSSPage);
}

const nsID& nsCSSPage::GetID(void)
{
  return kCSSPageSID;
}

#ifdef DEBUG
void nsCSSPage::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mMarks.AppendToString(buffer, eCSSProperty_marks);
  mSizeWidth.AppendToString(buffer, eCSSProperty_size_width);
  mSizeHeight.AppendToString(buffer, eCSSProperty_size_height);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSContent support -----------------

nsCSSCounterData::nsCSSCounterData(void)
  : mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSCounterData);
}

nsCSSCounterData::nsCSSCounterData(const nsCSSCounterData& aCopy)
  : mCounter(aCopy.mCounter),
    mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSCounterData);
  CSS_IF_COPY(mNext, nsCSSCounterData);
}

nsCSSCounterData::~nsCSSCounterData(void)
{
  MOZ_COUNT_DTOR(nsCSSCounterData);
  CSS_IF_DELETE(mNext);
}

nsCSSQuotes::nsCSSQuotes(void)
  : mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSQuotes);
}

nsCSSQuotes::nsCSSQuotes(const nsCSSQuotes& aCopy)
  : mOpen(aCopy.mOpen),
    mClose(aCopy.mClose),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSQuotes);
  CSS_IF_COPY(mNext, nsCSSQuotes);
}

nsCSSQuotes::~nsCSSQuotes(void)
{
  MOZ_COUNT_DTOR(nsCSSQuotes);
  CSS_IF_DELETE(mNext);
}

// --- nsCSSContent -----------------

nsCSSContent::nsCSSContent(void)
  : mContent(nsnull),
    mCounterIncrement(nsnull),
    mCounterReset(nsnull),
    mQuotes(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSContent);
}

nsCSSContent::nsCSSContent(const nsCSSContent& aCopy)
  : mContent(nsnull),
    mCounterIncrement(nsnull),
    mCounterReset(nsnull),
    mMarkerOffset(aCopy.mMarkerOffset),
    mQuotes(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSContent);
  CSS_IF_COPY(mContent, nsCSSValueList);
  CSS_IF_COPY(mCounterIncrement, nsCSSCounterData);
  CSS_IF_COPY(mCounterReset, nsCSSCounterData);
  CSS_IF_COPY(mQuotes, nsCSSQuotes);
}

nsCSSContent::~nsCSSContent(void)
{
  MOZ_COUNT_DTOR(nsCSSContent);
  CSS_IF_DELETE(mContent);
  CSS_IF_DELETE(mCounterIncrement);
  CSS_IF_DELETE(mCounterReset);
  CSS_IF_DELETE(mQuotes);
}

const nsID& nsCSSContent::GetID(void)
{
  return kCSSContentSID;
}

#ifdef DEBUG
void nsCSSContent::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  nsCSSValueList*  content = mContent;
  while (nsnull != content) {
    content->mValue.AppendToString(buffer, eCSSProperty_content);
    content = content->mNext;
  }
  nsCSSCounterData* counter = mCounterIncrement;
  while (nsnull != counter) {
    counter->mCounter.AppendToString(buffer, eCSSProperty__moz_counter_increment);
    counter->mValue.AppendToString(buffer, eCSSProperty_UNKNOWN);
    counter = counter->mNext;
  }
  counter = mCounterReset;
  while (nsnull != counter) {
    counter->mCounter.AppendToString(buffer, eCSSProperty__moz_counter_reset);
    counter->mValue.AppendToString(buffer, eCSSProperty_UNKNOWN);
    counter = counter->mNext;
  }
  mMarkerOffset.AppendToString(buffer, eCSSProperty_marker_offset);
  nsCSSQuotes*  quotes = mQuotes;
  while (nsnull != quotes) {
    quotes->mOpen.AppendToString(buffer, eCSSProperty_quotes_open);
    quotes->mClose.AppendToString(buffer, eCSSProperty_quotes_close);
    quotes = quotes->mNext;
  }

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSUserInterface -----------------

nsCSSUserInterface::nsCSSUserInterface(void)
  : mKeyEquivalent(nsnull), mCursor(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSUserInterface);
}

nsCSSUserInterface::nsCSSUserInterface(const nsCSSUserInterface& aCopy)
  : mUserInput(aCopy.mUserInput),
    mUserModify(aCopy.mUserModify),
    mUserSelect(aCopy.mUserSelect),
    mKeyEquivalent(nsnull),
    mUserFocus(aCopy.mUserFocus),
    mResizer(aCopy.mResizer),
    mCursor(nsnull),
    mForceBrokenImageIcon(aCopy.mForceBrokenImageIcon)
{
  MOZ_COUNT_CTOR(nsCSSUserInterface);
  CSS_IF_COPY(mCursor, nsCSSValueList);
  CSS_IF_COPY(mKeyEquivalent, nsCSSValueList);
}

nsCSSUserInterface::~nsCSSUserInterface(void)
{
  MOZ_COUNT_DTOR(nsCSSUserInterface);
  CSS_IF_DELETE(mKeyEquivalent);
  CSS_IF_DELETE(mCursor);
}

const nsID& nsCSSUserInterface::GetID(void)
{
  return kCSSUserInterfaceSID;
}

#ifdef DEBUG
void nsCSSUserInterface::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mUserInput.AppendToString(buffer, eCSSProperty_user_input);
  mUserModify.AppendToString(buffer, eCSSProperty_user_modify);
  mUserSelect.AppendToString(buffer, eCSSProperty_user_select);
  nsCSSValueList*  keyEquiv = mKeyEquivalent;
  while (nsnull != keyEquiv) {
    keyEquiv->mValue.AppendToString(buffer, eCSSProperty_key_equivalent);
    keyEquiv= keyEquiv->mNext;
  }
  mUserFocus.AppendToString(buffer, eCSSProperty_user_focus);
  mResizer.AppendToString(buffer, eCSSProperty_resizer);
  
  nsCSSValueList*  cursor = mCursor;
  while (nsnull != cursor) {
    cursor->mValue.AppendToString(buffer, eCSSProperty_cursor);
    cursor = cursor->mNext;
  }

  mForceBrokenImageIcon.AppendToString(buffer,eCSSProperty_force_broken_image_icon);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSAural -----------------

nsCSSAural::nsCSSAural(void)
{
  MOZ_COUNT_CTOR(nsCSSAural);
}

nsCSSAural::nsCSSAural(const nsCSSAural& aCopy)
  : mAzimuth(aCopy.mAzimuth),
    mElevation(aCopy.mElevation),
    mCueAfter(aCopy.mCueAfter),
    mCueBefore(aCopy.mCueBefore),
    mPauseAfter(aCopy.mPauseAfter),
    mPauseBefore(aCopy.mPauseBefore),
    mPitch(aCopy.mPitch),
    mPitchRange(aCopy.mPitchRange),
    mPlayDuring(aCopy.mPlayDuring),
    mPlayDuringFlags(aCopy.mPlayDuringFlags),
    mRichness(aCopy.mRichness),
    mSpeak(aCopy.mSpeak),
    mSpeakHeader(aCopy.mSpeakHeader),
    mSpeakNumeral(aCopy.mSpeakNumeral),
    mSpeakPunctuation(aCopy.mSpeakPunctuation),
    mSpeechRate(aCopy.mSpeechRate),
    mStress(aCopy.mStress),
    mVoiceFamily(aCopy.mVoiceFamily),
    mVolume(aCopy.mVolume)
{
  MOZ_COUNT_CTOR(nsCSSAural);
}

nsCSSAural::~nsCSSAural(void)
{
  MOZ_COUNT_DTOR(nsCSSAural);
}

const nsID& nsCSSAural::GetID(void)
{
  return kCSSAuralSID;
}

#ifdef DEBUG
void nsCSSAural::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mAzimuth.AppendToString(buffer, eCSSProperty_azimuth);
  mElevation.AppendToString(buffer, eCSSProperty_elevation);
  mCueAfter.AppendToString(buffer, eCSSProperty_cue_after);
  mCueBefore.AppendToString(buffer, eCSSProperty_cue_before);
  mPauseAfter.AppendToString(buffer, eCSSProperty_pause_after);
  mPauseBefore.AppendToString(buffer, eCSSProperty_pause_before);
  mPitch.AppendToString(buffer, eCSSProperty_pitch);
  mPitchRange.AppendToString(buffer, eCSSProperty_pitch_range);
  mPlayDuring.AppendToString(buffer, eCSSProperty_play_during);
  mPlayDuringFlags.AppendToString(buffer, eCSSProperty_play_during_flags);
  mRichness.AppendToString(buffer, eCSSProperty_richness);
  mSpeak.AppendToString(buffer, eCSSProperty_speak);
  mSpeakHeader.AppendToString(buffer, eCSSProperty_speak_header);
  mSpeakNumeral.AppendToString(buffer, eCSSProperty_speak_numeral);
  mSpeakPunctuation.AppendToString(buffer, eCSSProperty_speak_punctuation);
  mSpeechRate.AppendToString(buffer, eCSSProperty_speech_rate);
  mStress.AppendToString(buffer, eCSSProperty_stress);
  mVoiceFamily.AppendToString(buffer, eCSSProperty_voice_family);
  mVolume.AppendToString(buffer, eCSSProperty_volume);

  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

// --- nsCSSXUL -----------------

nsCSSXUL::nsCSSXUL(void)
{
  MOZ_COUNT_CTOR(nsCSSXUL);
}

nsCSSXUL::nsCSSXUL(const nsCSSXUL& aCopy)
  : mBoxAlign(aCopy.mBoxAlign), mBoxDirection(aCopy.mBoxDirection),
    mBoxFlex(aCopy.mBoxFlex), mBoxOrient(aCopy.mBoxOrient),
    mBoxPack(aCopy.mBoxPack), mBoxOrdinal(aCopy.mBoxOrdinal)
{
  MOZ_COUNT_CTOR(nsCSSXUL);
}

nsCSSXUL::~nsCSSXUL(void)
{
  MOZ_COUNT_DTOR(nsCSSXUL);
}

const nsID& nsCSSXUL::GetID(void)
{
  return kCSSXULSID;
}

#ifdef DEBUG
void nsCSSXUL::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mBoxAlign.AppendToString(buffer, eCSSProperty_box_align);
  mBoxDirection.AppendToString(buffer, eCSSProperty_box_direction);
  mBoxFlex.AppendToString(buffer, eCSSProperty_box_flex);
  mBoxOrient.AppendToString(buffer, eCSSProperty_box_orient);
  mBoxPack.AppendToString(buffer, eCSSProperty_box_pack);
  mBoxOrdinal.AppendToString(buffer, eCSSProperty_box_ordinal_group);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

#ifdef MOZ_SVG
// --- nsCSSSVG -----------------

nsCSSSVG::nsCSSSVG(void)
{
  MOZ_COUNT_CTOR(nsCSSSVG);
}

nsCSSSVG::nsCSSSVG(const nsCSSSVG& aCopy)
    : mFill(aCopy.mFill),
      mFillOpacity(aCopy.mFillOpacity),
      mFillRule(aCopy.mFillRule),
      mStroke(aCopy.mStroke),
      mStrokeDasharray(aCopy.mStrokeDasharray),
      mStrokeDashoffset(aCopy.mStrokeDashoffset),
      mStrokeLinecap(aCopy.mStrokeLinecap),
      mStrokeLinejoin(aCopy.mStrokeLinejoin),
      mStrokeMiterlimit(aCopy.mStrokeMiterlimit),
      mStrokeOpacity(aCopy.mStrokeOpacity),
      mStrokeWidth(aCopy.mStrokeWidth)
{
  MOZ_COUNT_CTOR(nsCSSSVG);
}

nsCSSSVG::~nsCSSSVG(void)
{
  MOZ_COUNT_DTOR(nsCSSSVG);
}

const nsID& nsCSSSVG::GetID(void)
{
  return kCSSSVGSID;
}

#ifdef DEBUG
void nsCSSSVG::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;

  mFill.AppendToString(buffer, eCSSProperty_fill);
  mFillOpacity.AppendToString(buffer, eCSSProperty_fill_opacity);
  mFillRule.AppendToString(buffer, eCSSProperty_fill_rule);
  mStroke.AppendToString(buffer, eCSSProperty_stroke);
  mStrokeDasharray.AppendToString(buffer, eCSSProperty_stroke_dasharray);
  mStrokeDashoffset.AppendToString(buffer, eCSSProperty_stroke_dashoffset);
  mStrokeLinecap.AppendToString(buffer, eCSSProperty_stroke_linecap);
  mStrokeLinejoin.AppendToString(buffer, eCSSProperty_stroke_linejoin);
  mStrokeMiterlimit.AppendToString(buffer, eCSSProperty_stroke_miterlimit);
  mStrokeOpacity.AppendToString(buffer, eCSSProperty_stroke_opacity);
  mStrokeWidth.AppendToString(buffer, eCSSProperty_stroke_width);
  fputs(NS_LossyConvertUCS2toASCII(buffer).get(), out);
}
#endif

#endif // MOZ_SVG

