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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsCSSProps.h"
#include "nsCSSKeywords.h"
#include "nsStyleConsts.h"

#include "nsILookAndFeel.h" // for system colors

#include "nsString.h"
#include "nsAVLTree.h"


// define an array of all CSS properties
#define CSS_PROP(_prop, _hint) #_prop,
const char* kCSSRawProperties[] = {
#include "nsCSSPropList.h"
};
#undef CSS_PROP

struct PropertyNode {
  PropertyNode(void)
    : mStr(),
      mEnum(eCSSProperty_UNKNOWN)
  {}

  PropertyNode(const nsStr& aStringValue, nsCSSProperty aEnumValue)
    : mStr(),
      mEnum(aEnumValue)
  { // point to the incomming buffer
    // note that the incomming buffer may really be 2 byte
    nsStr::Initialize(mStr, aStringValue.mStr, aStringValue.mCapacity, 
                      aStringValue.mLength, aStringValue.mCharSize, PR_FALSE);
  }

  nsCAutoString mStr;
  nsCSSProperty mEnum;
};

class PropertyComparitor: public nsAVLNodeComparitor {
public:
  virtual ~PropertyComparitor(void) {}
  virtual PRInt32 operator()(void* anItem1,void* anItem2) {
    PropertyNode* one = (PropertyNode*)anItem1;
    PropertyNode* two = (PropertyNode*)anItem2;
    return one->mStr.Compare(two->mStr, PR_TRUE);
  }
}; 


static PRInt32        gTableRefCount;
static PropertyNode*  gPropertyArray;
static nsAVLTree*     gPropertyTree;
static PropertyComparitor*  gComparitor;
static const nsCString* kNullStr;

void
nsCSSProps::AddRefTable(void) 
{
  if (0 == gTableRefCount++) {
    if (! gPropertyArray) {
      gPropertyArray= new PropertyNode[eCSSProperty_COUNT];
      gComparitor = new PropertyComparitor();
      if (gComparitor) {
        gPropertyTree = new nsAVLTree(*gComparitor, nsnull);
      }
      if (gPropertyArray && gPropertyTree) {
        PRInt32 index = -1;
        while (++index < PRInt32(eCSSProperty_COUNT)) {
          gPropertyArray[index].mStr = kCSSRawProperties[index];
          gPropertyArray[index].mStr.ReplaceChar('_', '-');
          gPropertyArray[index].mEnum = nsCSSProperty(index);
          gPropertyTree->AddItem(&(gPropertyArray[index]));
        }
      }
    }
    kNullStr = new nsCString();
  }
}

void
nsCSSProps::ReleaseTable(void) 
{
  if (0 == --gTableRefCount) {
    if (gPropertyArray) {
      delete [] gPropertyArray;
      gPropertyArray = nsnull;
    }
    if (gPropertyTree) {
      delete gPropertyTree;
      gPropertyTree = nsnull;
    }
    if (gComparitor) {
      delete gComparitor;
      gComparitor = nsnull;
    }
    delete kNullStr;
  }
}


nsCSSProperty 
nsCSSProps::LookupProperty(const nsStr& aProperty)
{
  NS_ASSERTION(gPropertyTree, "no lookup table, needs addref");
  if (gPropertyTree) {
    PropertyNode node(aProperty, eCSSProperty_UNKNOWN);
    PropertyNode*  found = (PropertyNode*)gPropertyTree->FindItem(&node);
    if (found) {
      NS_ASSERTION(found->mStr.EqualsIgnoreCase(aProperty), "bad tree");
      return found->mEnum;
    }
  }
  return eCSSProperty_UNKNOWN;
}


const nsCString& 
nsCSSProps::GetStringValue(nsCSSProperty aProperty)
{
  NS_ASSERTION(gPropertyArray, "no lookup table, needs addref");
  if ((eCSSProperty_UNKNOWN < aProperty) && 
      (aProperty < eCSSProperty_COUNT) && gPropertyArray) {
    return gPropertyArray[aProperty].mStr;
  }
  else {
    return *kNullStr;
  }
}



// Keyword id tables for variant/enum parsing
const PRInt32 nsCSSProps::kAzimuthKTable[] = {
  eCSSKeyword_left_side,    NS_STYLE_AZIMUTH_LEFT_SIDE,
  eCSSKeyword_far_left,     NS_STYLE_AZIMUTH_FAR_LEFT,
  eCSSKeyword_left,         NS_STYLE_AZIMUTH_LEFT,
  eCSSKeyword_center_left,  NS_STYLE_AZIMUTH_CENTER_LEFT,
  eCSSKeyword_center,       NS_STYLE_AZIMUTH_CENTER,
  eCSSKeyword_center_right, NS_STYLE_AZIMUTH_CENTER_RIGHT,
  eCSSKeyword_right,        NS_STYLE_AZIMUTH_RIGHT,
  eCSSKeyword_far_right,    NS_STYLE_AZIMUTH_FAR_RIGHT,
  eCSSKeyword_right_side,   NS_STYLE_AZIMUTH_RIGHT_SIDE,
  eCSSKeyword_behind,       NS_STYLE_AZIMUTH_BEHIND,
  eCSSKeyword_leftwards,    NS_STYLE_AZIMUTH_LEFTWARDS,
  eCSSKeyword_rightwards,   NS_STYLE_AZIMUTH_RIGHTWARDS,
  -1,-1
};

const PRInt32 nsCSSProps::kBackgroundAttachmentKTable[] = {
  eCSSKeyword_fixed, NS_STYLE_BG_ATTACHMENT_FIXED,
  eCSSKeyword_scroll, NS_STYLE_BG_ATTACHMENT_SCROLL,
  -1,-1
};

const PRInt32 nsCSSProps::kBackgroundColorKTable[] = {
  eCSSKeyword_transparent, NS_STYLE_BG_COLOR_TRANSPARENT,
  -1,-1
};

const PRInt32 nsCSSProps::kBackgroundRepeatKTable[] = {
  eCSSKeyword_no_repeat,  NS_STYLE_BG_REPEAT_OFF,
  eCSSKeyword_repeat,     NS_STYLE_BG_REPEAT_XY,
  eCSSKeyword_repeat_x,   NS_STYLE_BG_REPEAT_X,
  eCSSKeyword_repeat_y,   NS_STYLE_BG_REPEAT_Y,
  -1,-1
};

const PRInt32 nsCSSProps::kBorderCollapseKTable[] = {
  eCSSKeyword_collapse,  NS_STYLE_BORDER_COLLAPSE,
  eCSSKeyword_separate,  NS_STYLE_BORDER_SEPARATE,
  -1,-1
};

const PRInt32 nsCSSProps::kBorderColorKTable[] = {
  eCSSKeyword_transparent, NS_STYLE_COLOR_TRANSPARENT,
  -1,-1
};

const PRInt32 nsCSSProps::kBorderStyleKTable[] = {
  eCSSKeyword_hidden, NS_STYLE_BORDER_STYLE_HIDDEN,
  eCSSKeyword_dotted, NS_STYLE_BORDER_STYLE_DOTTED,
  eCSSKeyword_dashed, NS_STYLE_BORDER_STYLE_DASHED,
  eCSSKeyword_solid,  NS_STYLE_BORDER_STYLE_SOLID,
  eCSSKeyword_double, NS_STYLE_BORDER_STYLE_DOUBLE,
  eCSSKeyword_groove, NS_STYLE_BORDER_STYLE_GROOVE,
  eCSSKeyword_ridge,  NS_STYLE_BORDER_STYLE_RIDGE,
  eCSSKeyword_inset,  NS_STYLE_BORDER_STYLE_INSET,
  eCSSKeyword_outset, NS_STYLE_BORDER_STYLE_OUTSET,
  eCSSKeyword__moz_bg_inset,  NS_STYLE_BORDER_STYLE_BG_INSET,
  eCSSKeyword__moz_bg_outset, NS_STYLE_BORDER_STYLE_BG_OUTSET,
  -1,-1
};

const PRInt32 nsCSSProps::kBorderWidthKTable[] = {
  eCSSKeyword_thin, NS_STYLE_BORDER_WIDTH_THIN,
  eCSSKeyword_medium, NS_STYLE_BORDER_WIDTH_MEDIUM,
  eCSSKeyword_thick, NS_STYLE_BORDER_WIDTH_THICK,
  -1,-1
};

const PRInt32 nsCSSProps::kBoxSizingKTable[] = {
  eCSSKeyword_content_box,  NS_STYLE_BOX_SIZING_CONTENT,
  eCSSKeyword_border_box,   NS_STYLE_BOX_SIZING_BORDER,
  eCSSKeyword_padding_box,  NS_STYLE_BOX_SIZING_PADDING,
  -1,-1
};

const PRInt32 nsCSSProps::kCaptionSideKTable[] = {
  eCSSKeyword_top,    NS_SIDE_TOP,
  eCSSKeyword_right,  NS_SIDE_RIGHT,
  eCSSKeyword_bottom, NS_SIDE_BOTTOM,
  eCSSKeyword_left,   NS_SIDE_LEFT,
  -1,-1
};

const PRInt32 nsCSSProps::kClearKTable[] = {
  eCSSKeyword_left, NS_STYLE_CLEAR_LEFT,
  eCSSKeyword_right, NS_STYLE_CLEAR_RIGHT,
  eCSSKeyword_both, NS_STYLE_CLEAR_LEFT_AND_RIGHT,
  -1,-1
};

const PRInt32 nsCSSProps::kColorKTable[] = {
  eCSSKeyword_activeborder, nsILookAndFeel::eColor_activeborder,
  eCSSKeyword_activecaption, nsILookAndFeel::eColor_activecaption,
  eCSSKeyword_appworkspace, nsILookAndFeel::eColor_appworkspace,
  eCSSKeyword_background, nsILookAndFeel::eColor_background,
  eCSSKeyword_buttonface, nsILookAndFeel::eColor_buttonface,
  eCSSKeyword_buttonhighlight, nsILookAndFeel::eColor_buttonhighlight,
  eCSSKeyword_buttonshadow, nsILookAndFeel::eColor_buttonshadow,
  eCSSKeyword_buttontext, nsILookAndFeel::eColor_buttontext,
  eCSSKeyword_captiontext, nsILookAndFeel::eColor_captiontext,
  eCSSKeyword_graytext, nsILookAndFeel::eColor_graytext,
  eCSSKeyword_highlight, nsILookAndFeel::eColor_highlight,
  eCSSKeyword_highlighttext, nsILookAndFeel::eColor_highlighttext,
  eCSSKeyword_inactiveborder, nsILookAndFeel::eColor_inactiveborder,
  eCSSKeyword_inactivecaption, nsILookAndFeel::eColor_inactivecaption,
  eCSSKeyword_inactivecaptiontext, nsILookAndFeel::eColor_inactivecaptiontext,
  eCSSKeyword_infobackground, nsILookAndFeel::eColor_infobackground,
  eCSSKeyword_infotext, nsILookAndFeel::eColor_infotext,
  eCSSKeyword_menu, nsILookAndFeel::eColor_menu,
  eCSSKeyword_menutext, nsILookAndFeel::eColor_menutext,
  eCSSKeyword_scrollbar, nsILookAndFeel::eColor_scrollbar,
  eCSSKeyword_threeddarkshadow, nsILookAndFeel::eColor_threeddarkshadow,
  eCSSKeyword_threedface, nsILookAndFeel::eColor_threedface,
  eCSSKeyword_threedhighlight, nsILookAndFeel::eColor_threedhighlight,
  eCSSKeyword_threedlightshadow, nsILookAndFeel::eColor_threedlightshadow,
  eCSSKeyword_threedshadow, nsILookAndFeel::eColor_threedshadow,
  eCSSKeyword_window, nsILookAndFeel::eColor_window,
  eCSSKeyword_windowframe, nsILookAndFeel::eColor_windowframe,
  eCSSKeyword_windowtext, nsILookAndFeel::eColor_windowtext,
  -1,-1
};

const PRInt32 nsCSSProps::kContentKTable[] = {
  eCSSKeyword_open_quote, NS_STYLE_CONTENT_OPEN_QUOTE,
  eCSSKeyword_close_quote, NS_STYLE_CONTENT_CLOSE_QUOTE,
  eCSSKeyword_no_open_quote, NS_STYLE_CONTENT_NO_OPEN_QUOTE,
  eCSSKeyword_no_close_quote, NS_STYLE_CONTENT_NO_CLOSE_QUOTE,
  -1,-1
};

const PRInt32 nsCSSProps::kCursorKTable[] = {
  eCSSKeyword_crosshair, NS_STYLE_CURSOR_CROSSHAIR,
  eCSSKeyword_default, NS_STYLE_CURSOR_DEFAULT,
  eCSSKeyword_pointer, NS_STYLE_CURSOR_POINTER,
  eCSSKeyword_move, NS_STYLE_CURSOR_MOVE,
  eCSSKeyword_e_resize, NS_STYLE_CURSOR_E_RESIZE,
  eCSSKeyword_ne_resize, NS_STYLE_CURSOR_NE_RESIZE,
  eCSSKeyword_nw_resize, NS_STYLE_CURSOR_NW_RESIZE,
  eCSSKeyword_n_resize, NS_STYLE_CURSOR_N_RESIZE,
  eCSSKeyword_se_resize, NS_STYLE_CURSOR_SE_RESIZE,
  eCSSKeyword_sw_resize, NS_STYLE_CURSOR_SW_RESIZE,
  eCSSKeyword_s_resize, NS_STYLE_CURSOR_S_RESIZE,
  eCSSKeyword_w_resize, NS_STYLE_CURSOR_W_RESIZE,
  eCSSKeyword_text, NS_STYLE_CURSOR_TEXT,
  eCSSKeyword_wait, NS_STYLE_CURSOR_WAIT,
  eCSSKeyword_help, NS_STYLE_CURSOR_HELP,
  -1,-1
};

const PRInt32 nsCSSProps::kDirectionKTable[] = {
  eCSSKeyword_ltr,      NS_STYLE_DIRECTION_LTR,
  eCSSKeyword_rtl,      NS_STYLE_DIRECTION_RTL,
  -1,-1
};

const PRInt32 nsCSSProps::kDisplayKTable[] = {
  eCSSKeyword_inline,             NS_STYLE_DISPLAY_INLINE,
  eCSSKeyword_block,              NS_STYLE_DISPLAY_BLOCK,
  eCSSKeyword_inline_block,       NS_STYLE_DISPLAY_INLINE_BLOCK,
  eCSSKeyword_list_item,          NS_STYLE_DISPLAY_LIST_ITEM,
  eCSSKeyword_run_in,             NS_STYLE_DISPLAY_RUN_IN,
  eCSSKeyword_compact,            NS_STYLE_DISPLAY_COMPACT,
  eCSSKeyword_marker,             NS_STYLE_DISPLAY_MARKER,
  eCSSKeyword_table,              NS_STYLE_DISPLAY_TABLE,
  eCSSKeyword_inline_table,       NS_STYLE_DISPLAY_INLINE_TABLE,
  eCSSKeyword_table_row_group,    NS_STYLE_DISPLAY_TABLE_ROW_GROUP,
  eCSSKeyword_table_header_group, NS_STYLE_DISPLAY_TABLE_HEADER_GROUP,
  eCSSKeyword_table_footer_group, NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP,
  eCSSKeyword_table_row,          NS_STYLE_DISPLAY_TABLE_ROW,
  eCSSKeyword_table_column_group, NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP,
  eCSSKeyword_table_column,       NS_STYLE_DISPLAY_TABLE_COLUMN,
  eCSSKeyword_table_cell,         NS_STYLE_DISPLAY_TABLE_CELL,
  eCSSKeyword_table_caption,      NS_STYLE_DISPLAY_TABLE_CAPTION,
  eCSSKeyword_menu,               NS_STYLE_DISPLAY_MENU,
  -1,-1
};

const PRInt32 nsCSSProps::kElevationKTable[] = {
  eCSSKeyword_below,  NS_STYLE_ELEVATION_BELOW,
  eCSSKeyword_level,  NS_STYLE_ELEVATION_LEVEL,
  eCSSKeyword_above,  NS_STYLE_ELEVATION_ABOVE,
  eCSSKeyword_higher, NS_STYLE_ELEVATION_HIGHER,
  eCSSKeyword_lower,  NS_STYLE_ELEVATION_LOWER,
  -1,-1
};

const PRInt32 nsCSSProps::kEmptyCellsKTable[] = {
  eCSSKeyword_show,  NS_STYLE_TABLE_EMPTY_CELLS_SHOW,
  eCSSKeyword_hide,  NS_STYLE_TABLE_EMPTY_CELLS_HIDE,
  -1,-1
};

const PRInt32 nsCSSProps::kFloatKTable[] = {
  eCSSKeyword_left,  NS_STYLE_FLOAT_LEFT,
  eCSSKeyword_right, NS_STYLE_FLOAT_RIGHT,
  -1,-1
};

const PRInt32 nsCSSProps::kFloatEdgeKTable[] = {
  eCSSKeyword_content_box,  NS_STYLE_FLOAT_EDGE_CONTENT,
  eCSSKeyword_border_box,  NS_STYLE_FLOAT_EDGE_BORDER,
  eCSSKeyword_padding_box,  NS_STYLE_FLOAT_EDGE_PADDING,
  eCSSKeyword_margin_box,  NS_STYLE_FLOAT_EDGE_MARGIN,
  -1,-1
};

const PRInt32 nsCSSProps::kFontKTable[] = {
  eCSSKeyword_caption, NS_STYLE_FONT_CAPTION,
  eCSSKeyword_icon, NS_STYLE_FONT_ICON,
  eCSSKeyword_menu, NS_STYLE_FONT_MENU,
  eCSSKeyword_message_box, NS_STYLE_FONT_MESSAGE_BOX,
  eCSSKeyword_small_caption, NS_STYLE_FONT_SMALL_CAPTION,
  eCSSKeyword_status_bar, NS_STYLE_FONT_STATUS_BAR,
  -1,-1
};

const PRInt32 nsCSSProps::kFontSizeKTable[] = {
  eCSSKeyword_xx_small, NS_STYLE_FONT_SIZE_XXSMALL,
  eCSSKeyword_x_small, NS_STYLE_FONT_SIZE_XSMALL,
  eCSSKeyword_small, NS_STYLE_FONT_SIZE_SMALL,
  eCSSKeyword_medium, NS_STYLE_FONT_SIZE_MEDIUM,
  eCSSKeyword_large, NS_STYLE_FONT_SIZE_LARGE,
  eCSSKeyword_x_large, NS_STYLE_FONT_SIZE_XLARGE,
  eCSSKeyword_xx_large, NS_STYLE_FONT_SIZE_XXLARGE,
  eCSSKeyword_larger, NS_STYLE_FONT_SIZE_LARGER,
  eCSSKeyword_smaller, NS_STYLE_FONT_SIZE_SMALLER,
  -1,-1
};

const PRInt32 nsCSSProps::kFontStretchKTable[] = {
  eCSSKeyword_wider, NS_STYLE_FONT_STRETCH_WIDER,
  eCSSKeyword_narrower, NS_STYLE_FONT_STRETCH_NARROWER,
  eCSSKeyword_ultra_condensed, NS_STYLE_FONT_STRETCH_ULTRA_CONDENSED,
  eCSSKeyword_extra_condensed, NS_STYLE_FONT_STRETCH_EXTRA_CONDENSED,
  eCSSKeyword_condensed, NS_STYLE_FONT_STRETCH_CONDENSED,
  eCSSKeyword_semi_condensed, NS_STYLE_FONT_STRETCH_SEMI_CONDENSED,
  eCSSKeyword_semi_expanded, NS_STYLE_FONT_STRETCH_SEMI_EXPANDED,
  eCSSKeyword_expanded, NS_STYLE_FONT_STRETCH_EXPANDED,
  eCSSKeyword_extra_expanded, NS_STYLE_FONT_STRETCH_EXTRA_EXPANDED,
  eCSSKeyword_ultra_expanded, NS_STYLE_FONT_STRETCH_ULTRA_EXPANDED,
  -1,-1
};

const PRInt32 nsCSSProps::kFontStyleKTable[] = {
  eCSSKeyword_italic, NS_STYLE_FONT_STYLE_ITALIC,
  eCSSKeyword_oblique, NS_STYLE_FONT_STYLE_OBLIQUE,
  -1,-1
};

const PRInt32 nsCSSProps::kFontVariantKTable[] = {
  eCSSKeyword_small_caps, NS_STYLE_FONT_VARIANT_SMALL_CAPS,
  -1,-1
};

const PRInt32 nsCSSProps::kFontWeightKTable[] = {
    eCSSKeyword_bold, NS_STYLE_FONT_WEIGHT_BOLD,
    eCSSKeyword_bolder, NS_STYLE_FONT_WEIGHT_BOLDER,
    eCSSKeyword_lighter, NS_STYLE_FONT_WEIGHT_LIGHTER,
    -1,-1
};

const PRInt32 nsCSSProps::kKeyEquivalentKTable[] = {
  -1,-1
};

const PRInt32 nsCSSProps::kListStylePositionKTable[] = {
  eCSSKeyword_inside, NS_STYLE_LIST_STYLE_POSITION_INSIDE,
  eCSSKeyword_outside, NS_STYLE_LIST_STYLE_POSITION_OUTSIDE,
  -1,-1
};

const PRInt32 nsCSSProps::kListStyleKTable[] = {
  eCSSKeyword_disc, NS_STYLE_LIST_STYLE_DISC,
  eCSSKeyword_circle, NS_STYLE_LIST_STYLE_CIRCLE,
  eCSSKeyword_square, NS_STYLE_LIST_STYLE_SQUARE,
  eCSSKeyword_decimal, NS_STYLE_LIST_STYLE_DECIMAL,
  eCSSKeyword_decimal_leading_zero, NS_STYLE_LIST_STYLE_DECIMAL_LEADING_ZERO,
  eCSSKeyword_lower_roman, NS_STYLE_LIST_STYLE_LOWER_ROMAN,
  eCSSKeyword_upper_roman, NS_STYLE_LIST_STYLE_UPPER_ROMAN,
  eCSSKeyword_lower_greek, NS_STYLE_LIST_STYLE_LOWER_GREEK,
  eCSSKeyword_lower_alpha, NS_STYLE_LIST_STYLE_LOWER_ALPHA,
  eCSSKeyword_lower_latin, NS_STYLE_LIST_STYLE_LOWER_LATIN,
  eCSSKeyword_upper_alpha, NS_STYLE_LIST_STYLE_UPPER_ALPHA,
  eCSSKeyword_upper_latin, NS_STYLE_LIST_STYLE_UPPER_LATIN,
  eCSSKeyword_hebrew, NS_STYLE_LIST_STYLE_HEBREW,
  eCSSKeyword_armenian, NS_STYLE_LIST_STYLE_ARMENIAN,
  eCSSKeyword_georgian, NS_STYLE_LIST_STYLE_GEORGIAN,
  eCSSKeyword_cjk_ideographic, NS_STYLE_LIST_STYLE_CJK_IDEOGRAPHIC,
  eCSSKeyword_hiragana, NS_STYLE_LIST_STYLE_HIRAGANA,
  eCSSKeyword_katakana, NS_STYLE_LIST_STYLE_KATAKANA,
  eCSSKeyword_hiragana_iroha, NS_STYLE_LIST_STYLE_HIRAGANA_IROHA,
  eCSSKeyword_katakana_iroha, NS_STYLE_LIST_STYLE_KATAKANA_IROHA,
  -1,-1
};

const PRInt32 nsCSSProps::kOutlineColorKTable[] = {
  eCSSKeyword_invert, NS_STYLE_COLOR_INVERT,
  -1,-1
};

const PRInt32 nsCSSProps::kOverflowKTable[] = {
  eCSSKeyword_visible, NS_STYLE_OVERFLOW_VISIBLE,
  eCSSKeyword_hidden, NS_STYLE_OVERFLOW_HIDDEN,
  eCSSKeyword_scroll, NS_STYLE_OVERFLOW_SCROLL,
  -1,-1
};

const PRInt32 nsCSSProps::kPageBreakKTable[] = {
  eCSSKeyword_always, NS_STYLE_PAGE_BREAK_ALWAYS,
  eCSSKeyword_avoid, NS_STYLE_PAGE_BREAK_AVOID,
  eCSSKeyword_left, NS_STYLE_PAGE_BREAK_LEFT,
  eCSSKeyword_right, NS_STYLE_PAGE_BREAK_RIGHT,
  -1,-1
};

const PRInt32 nsCSSProps::kPageBreakInsideKTable[] = {
  eCSSKeyword_avoid, NS_STYLE_PAGE_BREAK_AVOID,
  -1,-1
};

const PRInt32 nsCSSProps::kPageMarksKTable[] = {
  eCSSKeyword_crop, NS_STYLE_PAGE_MARKS_CROP,
  eCSSKeyword_cross, NS_STYLE_PAGE_MARKS_REGISTER,
  -1,-1
};

const PRInt32 nsCSSProps::kPageSizeKTable[] = {
  eCSSKeyword_landscape, NS_STYLE_PAGE_SIZE_LANDSCAPE,
  eCSSKeyword_portrait, NS_STYLE_PAGE_SIZE_PORTRAIT,
  -1,-1
};

const PRInt32 nsCSSProps::kPitchKTable[] = {
  eCSSKeyword_x_low, NS_STYLE_PITCH_X_LOW,
  eCSSKeyword_low, NS_STYLE_PITCH_LOW,
  eCSSKeyword_medium, NS_STYLE_PITCH_MEDIUM,
  eCSSKeyword_high, NS_STYLE_PITCH_HIGH,
  eCSSKeyword_x_high, NS_STYLE_PITCH_X_HIGH,
  -1,-1
};

const PRInt32 nsCSSProps::kPlayDuringKTable[] = {
  eCSSKeyword_mix, NS_STYLE_PLAY_DURING_MIX,
  eCSSKeyword_repeat, NS_STYLE_PLAY_DURING_REPEAT,
  -1,-1
};

const PRInt32 nsCSSProps::kPositionKTable[] = {
  eCSSKeyword_static, NS_STYLE_POSITION_NORMAL,
  eCSSKeyword_relative, NS_STYLE_POSITION_RELATIVE,
  eCSSKeyword_absolute, NS_STYLE_POSITION_ABSOLUTE,
  eCSSKeyword_fixed, NS_STYLE_POSITION_FIXED,
  -1,-1
};

const PRInt32 nsCSSProps::kResizerKTable[] = {
  eCSSKeyword_both,       NS_STYLE_RESIZER_BOTH,
  eCSSKeyword_horizontal, NS_STYLE_RESIZER_HORIZONTAL,
  eCSSKeyword_vertical,   NS_STYLE_RESIZER_VERTICAL,
  -1,-1
};

const PRInt32 nsCSSProps::kSpeakKTable[] = {
  eCSSKeyword_spell_out, NS_STYLE_SPEAK_SPELL_OUT,
  -1,-1
};

const PRInt32 nsCSSProps::kSpeakHeaderKTable[] = {
  eCSSKeyword_once, NS_STYLE_SPEAK_HEADER_ONCE,
  eCSSKeyword_always, NS_STYLE_SPEAK_HEADER_ALWAYS,
  -1,-1
};

const PRInt32 nsCSSProps::kSpeakNumeralKTable[] = {
  eCSSKeyword_digits, NS_STYLE_SPEAK_NUMERAL_DIGITS,
  eCSSKeyword_continuous, NS_STYLE_SPEAK_NUMERAL_CONTINUOUS,
  -1,-1
};

const PRInt32 nsCSSProps::kSpeakPunctuationKTable[] = {
  eCSSKeyword_code, NS_STYLE_SPEAK_PUNCTUATION_CODE,
  -1,-1
};

const PRInt32 nsCSSProps::kSpeechRateKTable[] = {
  eCSSKeyword_x_slow, NS_STYLE_SPEECH_RATE_X_SLOW,
  eCSSKeyword_slow, NS_STYLE_SPEECH_RATE_SLOW,
  eCSSKeyword_medium, NS_STYLE_SPEECH_RATE_MEDIUM,
  eCSSKeyword_fast, NS_STYLE_SPEECH_RATE_FAST,
  eCSSKeyword_x_fast, NS_STYLE_SPEECH_RATE_X_FAST,
  eCSSKeyword_faster, NS_STYLE_SPEECH_RATE_FASTER,
  eCSSKeyword_slower, NS_STYLE_SPEECH_RATE_SLOWER,
  -1,-1
};

const PRInt32 nsCSSProps::kTableLayoutKTable[] = {
  eCSSKeyword_fixed, NS_STYLE_TABLE_LAYOUT_FIXED,
  -1,-1
};

const PRInt32 nsCSSProps::kTextAlignKTable[] = {
  eCSSKeyword_left, NS_STYLE_TEXT_ALIGN_LEFT,
  eCSSKeyword_right, NS_STYLE_TEXT_ALIGN_RIGHT,
  eCSSKeyword_center, NS_STYLE_TEXT_ALIGN_CENTER,
  eCSSKeyword_justify, NS_STYLE_TEXT_ALIGN_JUSTIFY,
  eCSSKeyword__moz_center, NS_STYLE_TEXT_ALIGN_MOZ_CENTER,
  eCSSKeyword__moz_right, NS_STYLE_TEXT_ALIGN_MOZ_RIGHT,
  -1,-1
};

const PRInt32 nsCSSProps::kTextDecorationKTable[] = {
  eCSSKeyword_underline, NS_STYLE_TEXT_DECORATION_UNDERLINE,
  eCSSKeyword_overline, NS_STYLE_TEXT_DECORATION_OVERLINE,
  eCSSKeyword_line_through, NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
  eCSSKeyword_blink, NS_STYLE_TEXT_DECORATION_BLINK,
  -1,-1
};

const PRInt32 nsCSSProps::kTextTransformKTable[] = {
  eCSSKeyword_capitalize, NS_STYLE_TEXT_TRANSFORM_CAPITALIZE,
  eCSSKeyword_lowercase, NS_STYLE_TEXT_TRANSFORM_LOWERCASE,
  eCSSKeyword_uppercase, NS_STYLE_TEXT_TRANSFORM_UPPERCASE,
  -1,-1
};

const PRInt32 nsCSSProps::kUnicodeBidiKTable[] = {
  eCSSKeyword_embed, NS_STYLE_UNICODE_BIDI_EMBED,
  eCSSKeyword_bidi_override, NS_STYLE_UNICODE_BIDI_OVERRIDE,
  -1,-1
};

const PRInt32 nsCSSProps::kUserFocusKTable[] = {
  eCSSKeyword_ignore,         NS_STYLE_USER_FOCUS_IGNORE,
  eCSSKeyword_select_all,     NS_STYLE_USER_FOCUS_SELECT_ALL,
  eCSSKeyword_select_before,  NS_STYLE_USER_FOCUS_SELECT_BEFORE,
  eCSSKeyword_select_after,   NS_STYLE_USER_FOCUS_SELECT_AFTER,
  eCSSKeyword_select_same,    NS_STYLE_USER_FOCUS_SELECT_SAME,
  eCSSKeyword_select_menu,    NS_STYLE_USER_FOCUS_SELECT_MENU,
  -1,-1
};

const PRInt32 nsCSSProps::kUserInputKTable[] = {
  eCSSKeyword_enabled,  NS_STYLE_USER_INPUT_ENABLED,
  eCSSKeyword_disabled, NS_STYLE_USER_INPUT_DISABLED,
  -1,-1
};

const PRInt32 nsCSSProps::kUserModifyKTable[] = {
  eCSSKeyword_read_only,  NS_STYLE_USER_MODIFY_READ_ONLY,
  eCSSKeyword_read_write, NS_STYLE_USER_MODIFY_READ_WRITE,
  eCSSKeyword_write_only, NS_STYLE_USER_MODIFY_WRITE_ONLY,
  eCSSKeyword_toggle,     NS_STYLE_USER_MODIFY_TOGGLE,
  eCSSKeyword_tri_state,  NS_STYLE_USER_MODIFY_TRI_STATE,
  -1,-1
};

const PRInt32 nsCSSProps::kUserSelectKTable[] = {
  eCSSKeyword_text,     NS_STYLE_USER_SELECT_TEXT,
  eCSSKeyword_element,  NS_STYLE_USER_SELECT_ELEMENT,
  eCSSKeyword_elements, NS_STYLE_USER_SELECT_ELEMENTS,
  eCSSKeyword_all,      NS_STYLE_USER_SELECT_ALL,
  -1,-1
};

const PRInt32 nsCSSProps::kVerticalAlignKTable[] = {
  eCSSKeyword_baseline, NS_STYLE_VERTICAL_ALIGN_BASELINE,
  eCSSKeyword_sub, NS_STYLE_VERTICAL_ALIGN_SUB,
  eCSSKeyword_super, NS_STYLE_VERTICAL_ALIGN_SUPER,
  eCSSKeyword_top, NS_STYLE_VERTICAL_ALIGN_TOP,
  eCSSKeyword_text_top, NS_STYLE_VERTICAL_ALIGN_TEXT_TOP,
  eCSSKeyword_middle, NS_STYLE_VERTICAL_ALIGN_MIDDLE,
  eCSSKeyword_bottom, NS_STYLE_VERTICAL_ALIGN_BOTTOM,
  eCSSKeyword_text_bottom, NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM,
  -1,-1
};

const PRInt32 nsCSSProps::kVisibilityKTable[] = {
  eCSSKeyword_visible, NS_STYLE_VISIBILITY_VISIBLE,
  eCSSKeyword_hidden, NS_STYLE_VISIBILITY_HIDDEN,
  eCSSKeyword_collapse, NS_STYLE_VISIBILITY_COLLAPSE,
  -1,-1
};

const PRInt32 nsCSSProps::kVolumeKTable[] = {
  eCSSKeyword_silent, NS_STYLE_VOLUME_SILENT,
  eCSSKeyword_x_soft, NS_STYLE_VOLUME_X_SOFT,
  eCSSKeyword_soft, NS_STYLE_VOLUME_SOFT,
  eCSSKeyword_medium, NS_STYLE_VOLUME_MEDIUM,
  eCSSKeyword_loud, NS_STYLE_VOLUME_LOUD,
  eCSSKeyword_x_loud, NS_STYLE_VOLUME_X_LOUD,
  -1,-1
};

const PRInt32 nsCSSProps::kWhitespaceKTable[] = {
  eCSSKeyword_pre, NS_STYLE_WHITESPACE_PRE,
  eCSSKeyword_nowrap, NS_STYLE_WHITESPACE_NOWRAP,
  eCSSKeyword__moz_pre_wrap, NS_STYLE_WHITESPACE_MOZ_PRE_WRAP,
  -1,-1
};

static const nsCString&
SearchKeywordTable(PRInt32 aValue, const PRInt32 aTable[])
{
  PRInt32 i = 1;
  for (;;) {
    if (aTable[i] < 0) {
      break;
    }
    if (aValue == aTable[i]) {
      return nsCSSKeywords::GetStringValue(nsCSSKeyword(aTable[i-1]));
    }
    i += 2;
  }
  return *kNullStr;
}



const nsCString& 
nsCSSProps::LookupPropertyValue(nsCSSProperty aProp, PRInt32 aValue)
{
static const PRInt32 kBackgroundXPositionKTable[] = {
  eCSSKeyword_left,   0,
  eCSSKeyword_center, 50,
  eCSSKeyword_right,  100,
  -1,-1
};

static const PRInt32 kBackgroundYPositionKTable[] = {
  eCSSKeyword_top,    0,
  eCSSKeyword_center, 50,
  eCSSKeyword_bottom, 100,
  -1,-1
};

  switch (aProp)  {

  case eCSSProperty__moz_border_radius:
    break;
  
  case eCSSProperty_azimuth:
    return SearchKeywordTable(aValue, kAzimuthKTable);

  case eCSSProperty_background:
    break;
  
  case eCSSProperty_background_attachment:
    return SearchKeywordTable(aValue, kBackgroundAttachmentKTable);

  case eCSSProperty_background_color:
    return SearchKeywordTable(aValue, kBackgroundColorKTable);
  
  case eCSSProperty_background_image:
    break;

  case eCSSProperty_background_position:
    break;

  case eCSSProperty_background_repeat:
    return SearchKeywordTable(aValue, kBackgroundRepeatKTable);

  case eCSSProperty_background_x_position:
    return SearchKeywordTable(aValue, kBackgroundXPositionKTable);

  case eCSSProperty_background_y_position:
    return SearchKeywordTable(aValue, kBackgroundYPositionKTable);

  case eCSSProperty_border:
    break;

  case eCSSProperty_border_collapse:
    return SearchKeywordTable(aValue, kBorderCollapseKTable);

  case eCSSProperty_box_sizing:
    return SearchKeywordTable(aValue, kBoxSizingKTable);

  case eCSSProperty_border_color:
  case eCSSProperty_border_spacing:
  case eCSSProperty_border_style:
  case eCSSProperty_border_bottom:
  case eCSSProperty_border_left:
  case eCSSProperty_border_right:
  case eCSSProperty_border_top:
    break;

  case eCSSProperty_border_bottom_color:
  case eCSSProperty_border_left_color:
  case eCSSProperty_border_right_color:
  case eCSSProperty_border_top_color:
    return SearchKeywordTable(aValue, kBorderColorKTable);

  case eCSSProperty_border_bottom_style:
  case eCSSProperty_border_left_style:
  case eCSSProperty_border_right_style:
  case eCSSProperty_border_top_style:
    return SearchKeywordTable(aValue, kBorderStyleKTable);

  case eCSSProperty_border_bottom_width:
  case eCSSProperty_border_left_width:
  case eCSSProperty_border_right_width:
  case eCSSProperty_border_top_width:
    return SearchKeywordTable(aValue, kBorderWidthKTable);
  
  case eCSSProperty_border_width:
  case eCSSProperty_border_x_spacing:
  case eCSSProperty_border_y_spacing:
  case eCSSProperty_bottom:
    break;

  case eCSSProperty_caption_side:
    return SearchKeywordTable(aValue, kCaptionSideKTable);  

  case eCSSProperty_clear:
    return SearchKeywordTable(aValue, kClearKTable);  
    
  case eCSSProperty_clip:
  case eCSSProperty_clip_bottom:
  case eCSSProperty_clip_left:
  case eCSSProperty_clip_right:
  case eCSSProperty_clip_top:
  case eCSSProperty_color:
    break;

  case eCSSProperty_content:
    return SearchKeywordTable(aValue, kContentKTable);  

  case eCSSProperty_counter_increment:
  case eCSSProperty_counter_reset:
  case eCSSProperty_cue:
  case eCSSProperty_cue_after:
  case eCSSProperty_cue_before:
    break;

  case eCSSProperty_cursor:
    return SearchKeywordTable(aValue, kCursorKTable);  

  case eCSSProperty_direction:
    return SearchKeywordTable(aValue, kDirectionKTable);  
  
  case eCSSProperty_display:
    return SearchKeywordTable(aValue, kDisplayKTable);  

  case eCSSProperty_elevation:
    return SearchKeywordTable(aValue, kElevationKTable);  

  case eCSSProperty_empty_cells:
    return SearchKeywordTable(aValue, kEmptyCellsKTable);  

  case eCSSProperty_float:
    return SearchKeywordTable(aValue, kFloatKTable);  

  case eCSSProperty_float_edge:
    return SearchKeywordTable(aValue, kFloatEdgeKTable);  

  case eCSSProperty_font:
    break;

  case eCSSProperty_font_family:
    return SearchKeywordTable(aValue, kFontKTable);  
    
  case eCSSProperty_font_size:
    return SearchKeywordTable(aValue, kFontSizeKTable);  

  case eCSSProperty_font_size_adjust:
    break;

  case eCSSProperty_font_stretch:
    return SearchKeywordTable(aValue, kFontStretchKTable);  

  case eCSSProperty_font_style:
    return SearchKeywordTable(aValue, kFontStyleKTable);  

  case eCSSProperty_font_variant:
    return SearchKeywordTable(aValue, kFontVariantKTable);  
  
  case eCSSProperty_font_weight:
    return SearchKeywordTable(aValue, kFontWeightKTable);  

  case eCSSProperty_height:
    break;

  case eCSSProperty_key_equivalent:
    return SearchKeywordTable(aValue, kKeyEquivalentKTable);

  case eCSSProperty_left:
  case eCSSProperty_letter_spacing:
  case eCSSProperty_line_height:
  case eCSSProperty_list_style:
  case eCSSProperty_list_style_image:
    break;

  case eCSSProperty_list_style_position:
    return SearchKeywordTable(aValue, kListStylePositionKTable);  
  
  case eCSSProperty_list_style_type:
    return SearchKeywordTable(aValue, kListStyleKTable);

  case eCSSProperty_margin:
  case eCSSProperty_margin_bottom:
  case eCSSProperty_margin_left:
  case eCSSProperty_margin_right:
  case eCSSProperty_margin_top:
  case eCSSProperty_marker_offset:
    break;

  case eCSSProperty_marks:
    return SearchKeywordTable(aValue, kPageMarksKTable);

  case eCSSProperty_max_height:
  case eCSSProperty_max_width:
  case eCSSProperty_min_height:
  case eCSSProperty_min_width:
    break;

  case eCSSProperty_opacity:
  case eCSSProperty_orphans:
  case eCSSProperty_outline:
    break;

  case eCSSProperty_outline_color:
    return SearchKeywordTable(aValue, kOutlineColorKTable);

  case eCSSProperty_outline_style:
    return SearchKeywordTable(aValue, kBorderStyleKTable);

  case eCSSProperty_outline_width:
    return SearchKeywordTable(aValue, kBorderWidthKTable);

  case eCSSProperty_overflow:
    return SearchKeywordTable(aValue, kOverflowKTable);
  
  case eCSSProperty_padding:
  case eCSSProperty_padding_bottom:
  case eCSSProperty_padding_left:
  case eCSSProperty_padding_right:
  case eCSSProperty_padding_top:
  case eCSSProperty_page:
    break;

  case eCSSProperty_page_break_before:
  case eCSSProperty_page_break_after:
    return SearchKeywordTable(aValue, kPageBreakKTable);

  case eCSSProperty_page_break_inside:
    return SearchKeywordTable(aValue, kPageBreakInsideKTable);
  
  case eCSSProperty_pause:
  case eCSSProperty_pause_after:
  case eCSSProperty_pause_before:
    break;

  case eCSSProperty_pitch:
    return SearchKeywordTable(aValue, kPitchKTable);

  case eCSSProperty_pitch_range:
  case eCSSProperty_play_during:
    break;

  case eCSSProperty_play_during_flags:
    return SearchKeywordTable(aValue, kPlayDuringKTable);

  case eCSSProperty_position:
    return SearchKeywordTable(aValue, kPositionKTable);
  
  case eCSSProperty_quotes:
  case eCSSProperty_quotes_close:
  case eCSSProperty_quotes_open:
    break;

  case eCSSProperty_resizer:
    return SearchKeywordTable(aValue, kResizerKTable);

  case eCSSProperty_richness:
  case eCSSProperty_right:
    break;

  case eCSSProperty_size:
    break;

  case eCSSProperty_size_height:
  case eCSSProperty_size_width:
    return SearchKeywordTable(aValue, kPageSizeKTable);

  case eCSSProperty_speak:
    return SearchKeywordTable(aValue, kSpeakKTable);

  case eCSSProperty_speak_header:
    return SearchKeywordTable(aValue, kSpeakHeaderKTable);

  case eCSSProperty_speak_numeral:
    return SearchKeywordTable(aValue, kSpeakNumeralKTable);

  case eCSSProperty_speak_punctuation:
    return SearchKeywordTable(aValue, kSpeakPunctuationKTable);

  case eCSSProperty_speech_rate:
    return SearchKeywordTable(aValue, kSpeechRateKTable);

  case eCSSProperty_stress:
    break;

  case eCSSProperty_table_layout:
    return SearchKeywordTable(aValue, kTableLayoutKTable);

  case eCSSProperty_text_align:
    return SearchKeywordTable(aValue, kTextAlignKTable);
  
  case eCSSProperty_text_decoration:
    return SearchKeywordTable(aValue, kTextDecorationKTable);

  case eCSSProperty_text_indent:
  case eCSSProperty_text_shadow:
  case eCSSProperty_text_shadow_color:
  case eCSSProperty_text_shadow_radius:
  case eCSSProperty_text_shadow_x:
  case eCSSProperty_text_shadow_y:
    break;
  
  case eCSSProperty_text_transform:
    return SearchKeywordTable(aValue, kTextTransformKTable);
  
  case eCSSProperty_top:
    break;

  case eCSSProperty_unicode_bidi:
    return SearchKeywordTable(aValue, kUnicodeBidiKTable);

  case eCSSProperty_user_focus:
    return SearchKeywordTable(aValue, kUserFocusKTable);

  case eCSSProperty_user_input:
    return SearchKeywordTable(aValue, kUserInputKTable);

  case eCSSProperty_user_modify:
    return SearchKeywordTable(aValue, kUserModifyKTable);

  case eCSSProperty_user_select:
    return SearchKeywordTable(aValue, kUserSelectKTable);

  case eCSSProperty_vertical_align:
    return SearchKeywordTable(aValue, kVerticalAlignKTable);
  
  case eCSSProperty_visibility:
    return SearchKeywordTable(aValue, kVisibilityKTable);

  case eCSSProperty_voice_family:
    break;

  case eCSSProperty_volume:
    return SearchKeywordTable(aValue, kVolumeKTable);
    
  case eCSSProperty_white_space:
    return SearchKeywordTable(aValue, kWhitespaceKTable);

  case eCSSProperty_widows:
  case eCSSProperty_width:
  case eCSSProperty_word_spacing:
  case eCSSProperty_z_index:
    break;

// no default case, let the compiler help find missing values
  case eCSSProperty_UNKNOWN:
  case eCSSProperty_COUNT:
    NS_ERROR("invalid property");
    break;
  }
  return *kNullStr;
}


// define array of all CSS property hints
#define CSS_PROP(_prop, _hint) NS_STYLE_HINT_##_hint,
const PRInt32 nsCSSProps::kHintTable[eCSSProperty_COUNT] = {
#include "nsCSSPropList.h"
};
#undef CSS_PROP

