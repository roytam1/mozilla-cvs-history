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
#ifndef nsCSSStruct_h___
#define nsCSSStruct_h___

#include "nsISupports.h"
#include "nsColor.h"
#include <stdio.h>
#include "nsString.h"
#include "nsCoord.h"
#include "nsCSSValue.h"

struct nsCSSStruct {
  // EMPTY on purpose.  ABSTRACT with no virtuals (typedef void nsCSSStruct?)
};

// Eventually we should stop using the nsCSS* structures for storing
// nsCSSDeclaration's data, because they're extremely bloated.  However,
// we'll still want to use them for nsRuleData.  So, for now, use
// typedefs and inheritance (forwards, when the rule data needs extra
// data) to make the rule data structs from the declaration structs.
typedef nsCSSStruct nsRuleDataStruct;


// SID for the nsCSSFont struct {f645dbf8-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_FONT_SID   \
{0xf645dbf8, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSColor struct {fd825f22-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_COLOR_SID   \
{0xfd825f22, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSText struct {fe13ce94-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_TEXT_SID   \
{0xfe13ce94, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSDisplay struct {fe13ce95-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_DISPLAY_SID   \
{0xfe13ce95, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSMargin struct {fe6019d4-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_MARGIN_SID   \
{0xfe6019d4, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSPosition struct {fee33b2a-b48a-11d1-9ca5-0060088f9ff7}
#define NS_CSS_POSITION_SID   \
{0xfee33b2a, 0xb48a, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSList struct {603f8266-b48b-11d1-9ca5-0060088f9ff7}
#define NS_CSS_LIST_SID   \
{0x603f8266, 0xb48b, 0x11d1, {0x9c, 0xa5, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

// SID for the nsCSSTable struct {16aa4b30-5a3b-11d2-803b-006008159b5a}
#define NS_CSS_TABLE_SID  \
{0x16aa4b30, 0x5a3b, 0x11d2, {0x80, 0x3b, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// SID for the nsCSSBreaks struct {15124c20-5a3b-11d2-803b-006008159b5a}
#define NS_CSS_BREAKS_SID \
{0x15124c20, 0x5a3b, 0x11d2, {0x80, 0x3b, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// SID for the nsCSSPage struct {15dd8810-5a3b-11d2-803b-006008159b5a}
#define NS_CSS_PAGE_SID  \
{0x15dd8810, 0x5a3b, 0x11d2, {0x80, 0x3b, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// SID for the nsCSSContent struct {1629ef70-5a3b-11d2-803b-006008159b5a}
#define NS_CSS_CONTENT_SID  \
{0x1629ef70, 0x5a3b, 0x11d2, {0x80, 0x3b, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// SID for the nsCSSUserInterface struct {4397c3a0-3efe-11d3-8060-006008159b5a}
#define NS_CSS_USER_INTERFACE_SID  \
{0x4397c3a0, 0x3efe, 0x11d3, {0x80, 0x60, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// SID for the nsCSSAural struct {166d2bb0-5a3b-11d2-803b-006008159b5a}
#define NS_CSS_AURAL_SID  \
{0x166d2bb0, 0x5a3b, 0x11d2, {0x80, 0x3b, 0x00, 0x60, 0x08, 0x15, 0x9b, 0x5a}}

// {FC075D62-B1CF-47a1-AF4E-CB40E11A4314}
#define NS_CSS_XUL_SID  \
{ 0xfc075d62, 0xb1cf, 0x47a1, { 0xaf, 0x4e, 0xcb, 0x40, 0xe1, 0x1a, 0x43, 0x14 } }

#ifdef MOZ_SVG
// {9A41A036-027B-45ef-89C9-6E32797839E7}
#define NS_CSS_SVG_SID \
{ 0x9a41a036, 0x27b, 0x45ef, { 0x89, 0xc9, 0x6e, 0x32, 0x79, 0x78, 0x39, 0xe7 } }
#endif

struct nsCSSFont : public nsCSSStruct {
  nsCSSFont(void);
  nsCSSFont(const nsCSSFont& aCopy);
  ~nsCSSFont(void);
  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mFamily;
  nsCSSValue mStyle;
  nsCSSValue mVariant;
  nsCSSValue mWeight;
  nsCSSValue mSize;
  nsCSSValue mSizeAdjust; // NEW
  nsCSSValue mStretch; // NEW
};

struct nsRuleDataFont : public nsCSSFont {
  PRBool mFamilyFromHTML; // Is the family from an HTML FONT element
};

struct nsCSSValueList {
  nsCSSValueList(void);
  nsCSSValueList(const nsCSSValueList& aCopy);
  ~nsCSSValueList(void);

  nsCSSValue      mValue;
  nsCSSValueList* mNext;
};

struct nsCSSColor : public nsCSSStruct  {
  nsCSSColor(void);
  nsCSSColor(const nsCSSColor& aCopy);
  ~nsCSSColor(void);
  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue      mColor;
  nsCSSValue      mBackColor;
  nsCSSValue      mBackImage;
  nsCSSValue      mBackRepeat;
  nsCSSValue      mBackAttachment;
  nsCSSValue      mBackPositionX;
  nsCSSValue      mBackPositionY;
  nsCSSValue      mBackClip;
  nsCSSValue      mBackOrigin;
  nsCSSValue      mBackInlinePolicy;
};

struct nsRuleDataColor : public nsCSSColor {
};

struct nsCSSShadow {
  nsCSSShadow(void);
  nsCSSShadow(const nsCSSShadow& aCopy);
  ~nsCSSShadow(void);

  nsCSSValue mColor;
  nsCSSValue mXOffset;
  nsCSSValue mYOffset;
  nsCSSValue mRadius;
  nsCSSShadow*  mNext;
};

struct nsCSSText : public nsCSSStruct  {
  nsCSSText(void);
  nsCSSText(const nsCSSText& aCopy);
  ~nsCSSText(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mWordSpacing;
  nsCSSValue mLetterSpacing;
  nsCSSValue mVerticalAlign;
  nsCSSValue mTextTransform;
  nsCSSValue mTextAlign;
  nsCSSValue mTextIndent;
  nsCSSValue mDecoration;
  nsCSSShadow* mTextShadow; // NEW
  nsCSSValue mUnicodeBidi;  // NEW
  nsCSSValue mLineHeight;
  nsCSSValue mWhiteSpace;
};

struct nsRuleDataText : public nsCSSText {
};

struct nsCSSRect {
  nsCSSRect(void);
  nsCSSRect(const nsCSSRect& aCopy);
  ~nsCSSRect();
#ifdef DEBUG
  void List(FILE* out = 0, nsCSSProperty aPropID = eCSSProperty_UNKNOWN, PRInt32 aIndent = 0) const;
  void List(FILE* out, PRInt32 aIndent, const nsCSSProperty aTRBL[]) const;
#endif

  nsCSSValue mTop;
  nsCSSValue mRight;
  nsCSSValue mBottom;
  nsCSSValue mLeft;
};

struct nsCSSDisplay : public nsCSSStruct  {
  nsCSSDisplay(void);
  nsCSSDisplay(const nsCSSDisplay& aCopy);
  ~nsCSSDisplay(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mDirection;
  nsCSSValue mDisplay;
  nsCSSValue mBinding;
  nsCSSValue mAppearance;
  nsCSSValue mPosition;
  nsCSSValue mFloat;
  nsCSSValue mClear;
  nsCSSRect* mClip;
  nsCSSValue mOverflow;
  nsCSSValue mVisibility;
  nsCSSValue mOpacity;

  // temp fix for bug 24000 
  nsCSSValue mBreakBefore;
  nsCSSValue mBreakAfter;
  // end temp fix
};

struct nsRuleDataDisplay : public nsCSSDisplay {
  nsCSSValue mLang;
};

struct nsCSSMargin : public nsCSSStruct  {
  nsCSSMargin(void);
  nsCSSMargin(const nsCSSMargin& aCopy);
  ~nsCSSMargin(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  void EnsureBorderColors();

  nsCSSRect*  mMargin;
  nsCSSRect*  mPadding;
  nsCSSRect*  mBorderWidth;
  nsCSSRect*  mBorderColor;
  nsCSSValueList** mBorderColors;
  nsCSSRect*  mBorderStyle;
  nsCSSRect*  mBorderRadius;  // (extension)
  nsCSSValue  mOutlineWidth;
  nsCSSValue  mOutlineColor;
  nsCSSValue  mOutlineStyle;
  nsCSSRect*  mOutlineRadius; // (extension)
  nsCSSValue  mFloatEdge; // NEW
};

struct nsRuleDataMargin : public nsCSSMargin {
};

struct nsCSSPosition : public nsCSSStruct  {
  nsCSSPosition(void);
  nsCSSPosition(const nsCSSPosition& aCopy);
  ~nsCSSPosition(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue  mWidth;
  nsCSSValue  mMinWidth;
  nsCSSValue  mMaxWidth;
  nsCSSValue  mHeight;
  nsCSSValue  mMinHeight;
  nsCSSValue  mMaxHeight;
  nsCSSValue  mBoxSizing; // NEW
  nsCSSRect*  mOffset;
  nsCSSValue  mZIndex;
};

struct nsRuleDataPosition : public nsCSSPosition {
};

struct nsCSSList : public nsCSSStruct  {
  nsCSSList(void);
  nsCSSList(const nsCSSList& aCopy);
  ~nsCSSList(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mType;
  nsCSSValue mImage;
  nsCSSValue mPosition;
  nsCSSRect*  mImageRegion;
};

struct nsRuleDataList : public nsCSSList {
};

struct nsCSSTable : public nsCSSStruct  { // NEW
  nsCSSTable(void);
  nsCSSTable(const nsCSSTable& aCopy);
  ~nsCSSTable(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mBorderCollapse;
  nsCSSValue mBorderSpacingX;
  nsCSSValue mBorderSpacingY;
  nsCSSValue mCaptionSide;
  nsCSSValue mEmptyCells;
  
  nsCSSValue mLayout;
  nsCSSValue mFrame; // Not mappable via CSS, only using HTML4 table attrs.
  nsCSSValue mRules; // Not mappable via CSS, only using HTML4 table attrs.
  nsCSSValue mSpan; // Not mappable via CSS, only using HTML4 table attrs.
  nsCSSValue mCols; // Not mappable via CSS, only using HTML4 table attrs.
};

struct nsRuleDataTable : public nsCSSTable {
};

struct nsCSSBreaks : public nsCSSStruct  { // NEW
  nsCSSBreaks(void);
  nsCSSBreaks(const nsCSSBreaks& aCopy);
  ~nsCSSBreaks(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mOrphans;
  nsCSSValue mWidows;
  nsCSSValue mPage;
  nsCSSValue mPageBreakAfter;
  nsCSSValue mPageBreakBefore;
  nsCSSValue mPageBreakInside;
};

struct nsRuleDataBreaks : public nsCSSBreaks {
};

struct nsCSSPage : public nsCSSStruct  { // NEW
  nsCSSPage(void);
  nsCSSPage(const nsCSSPage& aCopy);
  ~nsCSSPage(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mMarks;
  nsCSSValue mSizeWidth;
  nsCSSValue mSizeHeight;
};

struct nsRuleDataPage : public nsCSSPage {
};

struct nsCSSCounterData {
  nsCSSCounterData(void);
  nsCSSCounterData(const nsCSSCounterData& aCopy);
  ~nsCSSCounterData(void);

  nsCSSValue        mCounter;
  nsCSSValue        mValue;
  nsCSSCounterData* mNext;
};

struct nsCSSQuotes {
  nsCSSQuotes(void);
  nsCSSQuotes(const nsCSSQuotes& aCopy);
  ~nsCSSQuotes(void);

  nsCSSValue    mOpen;
  nsCSSValue    mClose;
  nsCSSQuotes*  mNext;
};

struct nsCSSContent : public nsCSSStruct  {
  nsCSSContent(void);
  nsCSSContent(const nsCSSContent& aCopy);
  ~nsCSSContent(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValueList*   mContent;
  nsCSSCounterData* mCounterIncrement;
  nsCSSCounterData* mCounterReset;
  nsCSSValue        mMarkerOffset;
  nsCSSQuotes*      mQuotes;
};

struct nsRuleDataContent : public nsCSSContent {
};

struct nsCSSUserInterface : public nsCSSStruct  { // NEW
  nsCSSUserInterface(void);
  nsCSSUserInterface(const nsCSSUserInterface& aCopy);
  ~nsCSSUserInterface(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue      mUserInput;
  nsCSSValue      mUserModify;
  nsCSSValue      mUserSelect;
  nsCSSValueList* mKeyEquivalent;
  nsCSSValue      mUserFocus;
  nsCSSValue      mResizer;
  
  nsCSSValueList* mCursor;
  nsCSSValue      mForceBrokenImageIcon;
};

struct nsRuleDataUserInterface : public nsCSSUserInterface {
};

struct nsCSSAural : public nsCSSStruct  { // NEW
  nsCSSAural(void);
  nsCSSAural(const nsCSSAural& aCopy);
  ~nsCSSAural(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mAzimuth;
  nsCSSValue mElevation;
  nsCSSValue mCueAfter;
  nsCSSValue mCueBefore;
  nsCSSValue mPauseAfter;
  nsCSSValue mPauseBefore;
  nsCSSValue mPitch;
  nsCSSValue mPitchRange;
  nsCSSValue mPlayDuring;
  nsCSSValue mPlayDuringFlags;
  nsCSSValue mRichness;
  nsCSSValue mSpeak;
  nsCSSValue mSpeakHeader;
  nsCSSValue mSpeakNumeral;
  nsCSSValue mSpeakPunctuation;
  nsCSSValue mSpeechRate;
  nsCSSValue mStress;
  nsCSSValue mVoiceFamily;
  nsCSSValue mVolume;
};

struct nsRuleDataAural : public nsCSSAural {
};

struct nsCSSXUL : public nsCSSStruct  {
  nsCSSXUL(void);
  nsCSSXUL(const nsCSSXUL& aCopy);
  ~nsCSSXUL(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue  mBoxAlign;
  nsCSSValue  mBoxDirection;
  nsCSSValue  mBoxFlex;
  nsCSSValue  mBoxOrient;
  nsCSSValue  mBoxPack;
  nsCSSValue  mBoxOrdinal;
};

struct nsRuleDataXUL : public nsCSSXUL {
};

#ifdef MOZ_SVG
struct nsCSSSVG : public nsCSSStruct {
  nsCSSSVG(void);
  nsCSSSVG(const nsCSSSVG& aCopy);
  ~nsCSSSVG(void);

  const nsID& GetID(void);
#ifdef DEBUG
  void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  nsCSSValue mFill;
  nsCSSValue mFillOpacity;
  nsCSSValue mFillRule;
  nsCSSValue mStroke;
  nsCSSValue mStrokeDasharray;
  nsCSSValue mStrokeDashoffset;
  nsCSSValue mStrokeLinecap;
  nsCSSValue mStrokeLinejoin;
  nsCSSValue mStrokeMiterlimit;
  nsCSSValue mStrokeOpacity;
  nsCSSValue mStrokeWidth;
};

struct nsRuleDataSVG : public nsCSSSVG {
};
#endif

//
// Some useful types.
//
typedef PRUint16 nsCSSBitField;
typedef PRUint16 nsCSSDeclRefCount;
struct nsCSSDeclContains
{
    nsCSSBitField mHasDisplay:1;          //  1
    nsCSSBitField mHasText:1;             //  2
    nsCSSBitField mHasColor:1;            //  3
    nsCSSBitField mHasMargin:1;           //  4
    nsCSSBitField mHasList:1;             //  4
    nsCSSBitField mHasFont:1;             //  5
    nsCSSBitField mHasPosition:1;         //  6
    nsCSSBitField mHasUserInterface:1;    //  8
    nsCSSBitField mHasTable:1;            //  9
    nsCSSBitField mHasContent:1;          // 10
    nsCSSBitField mHasXUL:1;              // 11
    nsCSSBitField mHasBreaks:1;           // 12
    nsCSSBitField mHasPage:1;             // 13
    nsCSSBitField mHasAural:1;            // 14
#if defined(MOZ_SVG)
    nsCSSBitField mHasSVG:1;              // 15
#endif
};

//
// Macros used to figure out at what index the pointer to the object is.
// These macros have been placed in the order of what is considered most
//  created nsCSSStruct first, such that "less math for most likely" will
//  lend itself to the speed of the index lookup calculation.
// I have also ordered the bit fields, in case the compiler can come up
//  with an optimization if the ordering would matter.
// Presence (mContains.mHasWhatever) is NOT tested for, so do that yourself.
//
#define CSSDECLIDX_Display(decl)       ((decl).mContains.mHasDisplay - 1)
#define CSSDECLIDX_Text(decl)          ((decl).mContains.mHasText + CSSDECLIDX_Display(decl))
#define CSSDECLIDX_Color(decl)         ((decl).mContains.mHasColor + CSSDECLIDX_Text(decl))
#define CSSDECLIDX_Margin(decl)        ((decl).mContains.mHasMargin + CSSDECLIDX_Color(decl))
#define CSSDECLIDX_List(decl)          ((decl).mContains.mHasList + CSSDECLIDX_Margin(decl))
#define CSSDECLIDX_Font(decl)          ((decl).mContains.mHasFont + CSSDECLIDX_List(decl))
#define CSSDECLIDX_Position(decl)      ((decl).mContains.mHasPosition + CSSDECLIDX_Font(decl))
#define CSSDECLIDX_UserInterface(decl) ((decl).mContains.mHasUserInterface + CSSDECLIDX_Position(decl))
#define CSSDECLIDX_Table(decl)         ((decl).mContains.mHasTable + CSSDECLIDX_UserInterface(decl))
#define CSSDECLIDX_Content(decl)       ((decl).mContains.mHasContent + CSSDECLIDX_Table(decl))
#define CSSDECLIDX_XUL(decl)           ((decl).mContains.mHasXUL + CSSDECLIDX_Content(decl))
#define CSSDECLIDX_Breaks(decl)        ((decl).mContains.mHasBreaks + CSSDECLIDX_XUL(decl))
#define CSSDECLIDX_Page(decl)          ((decl).mContains.mHasPage + CSSDECLIDX_Breaks(decl))
#define CSSDECLIDX_Aural(decl)         ((decl).mContains.mHasAural + CSSDECLIDX_Page(decl))
#if defined(MOZ_SVG)
#define CSSDECLIDX_SVG(decl)           ((decl).mContains.mHasSVG + CSSDECLIDX_Aural(decl))
#endif

#endif /* nsCSSStruct_h___ */
