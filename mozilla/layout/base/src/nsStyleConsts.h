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
#ifndef nsStyleConsts_h___
#define nsStyleConsts_h___

// Defines for various style related constants

// See nsStyleMolecule.backgroundAttachment
#define NS_STYLE_BG_ATTACHMENT_FIXED            0x01
#define NS_STYLE_BG_ATTACHMENT_SCROLL           0x02

// See nsStyleMolecule.backgroundFlags
#define NS_STYLE_BG_COLOR_TRANSPARENT           0x01
#define NS_STYLE_BG_IMAGE_NONE                  0x02
#define NS_STYLE_BG_X_POSITION_PCT              0x04
#define NS_STYLE_BG_X_POSITION_LENGTH           0x08
#define NS_STYLE_BG_Y_POSITION_PCT              0x10
#define NS_STYLE_BG_Y_POSITION_LENGTH           0x20

// See nsStyleMolecule.backgroundRepeat
#define NS_STYLE_BG_REPEAT_OFF                  0x00
#define NS_STYLE_BG_REPEAT_X                    0x01
#define NS_STYLE_BG_REPEAT_Y                    0x02
#define NS_STYLE_BG_REPEAT_XY                   0x03

// See nsStyleMolecule.borderSizeFlags
#define NS_STYLE_BORDER_WIDTH_THIN              0
#define NS_STYLE_BORDER_WIDTH_MEDIUM            1
#define NS_STYLE_BORDER_WIDTH_THICK             2
#define NS_STYLE_BORDER_WIDTH_LENGTH_VALUE      3

// See nsStyleMolecule.borderStyle
#define NS_STYLE_BORDER_STYLE_NONE              0
#define NS_STYLE_BORDER_STYLE_GROOVE            1
#define NS_STYLE_BORDER_STYLE_RIDGE             2
#define NS_STYLE_BORDER_STYLE_DOTTED            3
#define NS_STYLE_BORDER_STYLE_DASHED            4
#define NS_STYLE_BORDER_STYLE_SOLID             5
#define NS_STYLE_BORDER_STYLE_DOUBLE            6
#define NS_STYLE_BORDER_STYLE_BLANK             7
#define NS_STYLE_BORDER_STYLE_INSET             8
#define NS_STYLE_BORDER_STYLE_OUTSET            9

// See nsStyleMolecule.clear
#define NS_STYLE_CLEAR_NONE                     0x0
#define NS_STYLE_CLEAR_LEFT                     0x1
#define NS_STYLE_CLEAR_RIGHT                    0x2
#define NS_STYLE_CLEAR_BOTH                     0x3

// See nsStyleMolecule.clipFlags
#define NS_STYLE_CLIP_AUTO                      0

// See nsStyleMolecule.cursor
#define NS_STYLE_CURSOR_INHERIT                 0
#define NS_STYLE_CURSOR_DEFAULT                 1
#define NS_STYLE_CURSOR_HAND                    2
#define NS_STYLE_CURSOR_IBEAM                   3

// See nsStyleMolecule.direction
#define NS_STYLE_DIRECTION_LTR                  0
#define NS_STYLE_DIRECTION_RTL                  1

// See nsStyleMolecule.display
#define NS_STYLE_DISPLAY_NONE                   0
#define NS_STYLE_DISPLAY_BLOCK                  1
#define NS_STYLE_DISPLAY_INLINE                 2
#define NS_STYLE_DISPLAY_LIST_ITEM              3

// See nsStyleMolecule.floats
#define NS_STYLE_FLOAT_NONE                     0
#define NS_STYLE_FLOAT_LEFT                     1
#define NS_STYLE_FLOAT_RIGHT                    2

// See nsStyleMolecule.fontStyle
#define NS_STYLE_FONT_STYLE_NORMAL              0
#define NS_STYLE_FONT_STYLE_ITALIC              1
#define NS_STYLE_FONT_STYLE_OBLIQUE             2

// See nsStyleMolecule.fontVariant
#define NS_STYLE_FONT_VARIANT_NORMAL            0
#define NS_STYLE_FONT_VARIANT_SMALL_CAPS        1

// See nsStyleMolecule.fontWeight
#define NS_STYLE_FONT_WEIGHT_NORMAL             400
#define NS_STYLE_FONT_WEIGHT_BOLD               700
#define NS_STYLE_FONT_WEIGHT_BOLDER             100
#define NS_STYLE_FONT_WEIGHT_LIGHTER            -100

// See nsStyleMolecule.fontSize
#define NS_STYLE_FONT_SIZE_XXSMALL              0
#define NS_STYLE_FONT_SIZE_XSMALL               1
#define NS_STYLE_FONT_SIZE_SMALL                2
#define NS_STYLE_FONT_SIZE_MEDIUM               3
#define NS_STYLE_FONT_SIZE_LARGE                4
#define NS_STYLE_FONT_SIZE_XLARGE               5
#define NS_STYLE_FONT_SIZE_XXLARGE              6
#define NS_STYLE_FONT_SIZE_LARGER               7
#define NS_STYLE_FONT_SIZE_SMALLER              8

// See nsStylePosition
#define NS_STYLE_POSITION_STATIC                0
#define NS_STYLE_POSITION_RELATIVE              1
#define NS_STYLE_POSITION_ABSOLUTE              2

// See nsStylePosition
#define NS_STYLE_POSITION_VALUE_LENGTH          0
#define NS_STYLE_POSITION_VALUE_PCT             1
#define NS_STYLE_POSITION_VALUE_AUTO            2
#define NS_STYLE_POSITION_VALUE_INHERIT         3

#define NS_STYLE_HEIGHT_AUTO                    0

#define NS_STYLE_LEFT_AUTO                      0

#define NS_STYLE_LINE_HEIGHT_NORMAL             0

#define NS_STYLE_LIST_STYLE_IMAGE_NONE          0

#define NS_STYLE_LIST_STYLE_NONE                0
#define NS_STYLE_LIST_STYLE_DISC                1
#define NS_STYLE_LIST_STYLE_CIRCLE              2
#define NS_STYLE_LIST_STYLE_SQUARE              3
#define NS_STYLE_LIST_STYLE_DECIMAL             4
#define NS_STYLE_LIST_STYLE_LOWER_ROMAN         5
#define NS_STYLE_LIST_STYLE_UPPER_ROMAN         6
#define NS_STYLE_LIST_STYLE_LOWER_ALPHA         7
#define NS_STYLE_LIST_STYLE_UPPER_ALPHA         8
#define NS_STYLE_LIST_STYLE_BASIC               9       // not in css

#define NS_STYLE_LIST_STYLE_POSITION_INSIDE     0
#define NS_STYLE_LIST_STYLE_POSITION_OUTSIDE    1

#define NS_STYLE_MARGIN_SIZE_AUTO               0

#define NS_STYLE_OVERFLOW_VISIBLE               0
#define NS_STYLE_OVERFLOW_HIDDEN                1
#define NS_STYLE_OVERFLOW_SCROLL                2
#define NS_STYLE_OVERFLOW_AUTO                  3

#define NS_STYLE_SPACING_NORMAL                 0

#define NS_STYLE_TEXT_ALIGN_LEFT                0
#define NS_STYLE_TEXT_ALIGN_RIGHT               1
#define NS_STYLE_TEXT_ALIGN_CENTER              2
#define NS_STYLE_TEXT_ALIGN_JUSTIFY             3

#define NS_STYLE_TEXT_DECORATION_NONE           0
#define NS_STYLE_TEXT_DECORATION_UNDERLINE      0x1
#define NS_STYLE_TEXT_DECORATION_OVERLINE       0x2
#define NS_STYLE_TEXT_DECORATION_LINE_THROUGH   0x4
#define NS_STYLE_TEXT_DECORATION_BLINK          0x8

#define NS_STYLE_TEXT_TRANSFORM_NONE            0
#define NS_STYLE_TEXT_TRANSFORM_CAPITALIZE      1
#define NS_STYLE_TEXT_TRANSFORM_LOWERCASE       2
#define NS_STYLE_TEXT_TRANSFORM_UPPERCASE       3

#define NS_STYLE_TOP_AUTO                       0

// Note: these values pickup after the text-align values because there
// are a few html cases where an object can have both types of
// alignment applied with a single attribute
#define NS_STYLE_VERTICAL_ALIGN_BASELINE        10
#define NS_STYLE_VERTICAL_ALIGN_SUB             11
#define NS_STYLE_VERTICAL_ALIGN_SUPER           12
#define NS_STYLE_VERTICAL_ALIGN_TOP             13
#define NS_STYLE_VERTICAL_ALIGN_TEXT_TOP        14
#define NS_STYLE_VERTICAL_ALIGN_MIDDLE          15
#define NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM     16
#define NS_STYLE_VERTICAL_ALIGN_BOTTOM          17
#define NS_STYLE_VERTICAL_ALIGN_LENGTH          18
#define NS_STYLE_VERTICAL_ALIGN_PCT             19

#define NS_STYLE_VISIBILITY_INHERIT             0
#define NS_STYLE_VISIBILITY_VISIBLE             1
#define NS_STYLE_VISIBILITY_HIDDEN              2

#define NS_STYLE_WIDTH_AUTO                     0

#define NS_STYLE_WHITESPACE_NORMAL              0
#define NS_STYLE_WHITESPACE_PRE                 1
#define NS_STYLE_WHITESPACE_NOWRAP              2

#endif /* nsStyleConsts_h___ */
