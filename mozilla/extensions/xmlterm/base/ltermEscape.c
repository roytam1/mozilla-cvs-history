/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "MPL"); you may not use this file
 * except in compliance with the MPL. You may obtain a copy of
 * the MPL at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the MPL is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the MPL for the specific language governing
 * rights and limitations under the MPL.
 * 
 * The Original Code is lineterm.
 * 
 * The Initial Developer of the Original Code is Ramalingam Saravanan.
 * Portions created by Ramalingam Saravanan <svn@xmlterm.org> are
 * Copyright (C) 1999 Ramalingam Saravanan. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License (the "GPL"), in which case
 * the provisions of the GPL are applicable instead of
 * those above. If you wish to allow use of your version of this
 * file only under the terms of the GPL and not to allow
 * others to use your version of this file under the MPL, indicate
 * your decision by deleting the provisions above and replace them
 * with the notice and other provisions required by the GPL.
 * If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/* ltermEscape.c: LTERM escape sequence processing
 */

/* public declarations */
#include "lineterm.h"

/* private declarations */
#include "ltermPrivate.h"


static int ltermProcessCSISequence(struct lterms *lts, const UNICHAR *buf,
                int count, const UNISTYLE *style, int *consumed, int *opcodes);
static int ltermProcessDECPrivateMode(struct lterms *lts,
            const int *paramValues, int paramCount, UNICHAR uch, int *opcodes);
static int ltermProcessXTERMSequence(struct lterms *lts, const UNICHAR *buf,
                  int count, const UNISTYLE *style,int *consumed,int *opcodes);
static int ltermProcessXMLTermSequence(struct lterms *lts, const UNICHAR *buf,
                  int count, const UNISTYLE *style,int *consumed,int *opcodes);


/** Processes ESCAPE sequence in string BUF containing COUNT characters,
 * returning the number of characters CONSUMED and the OPCODES.
 * Using Extended Backus-Naur Form notation:
 * OPCODES ::= 0 if the sequence was processed without any change of
 * outputMode or terminating condition.
 * OPCODES ::= LINEDATA OUTPUT if a switch from LineMode to ScreenMode
 *                         or StreamMode was triggered by the ESCAPE sqeuence.
 * OPCODES ::= SCREENDATA OUTPUT if a switch from ScreenMode to StreamMode
 *                         was triggered by the ESCAPE sequence.
 * (A switch from ScreenMode to LineMode is not considered a terminating
 *  condition and occurs transparently.)
 * OPCODES ::= SCREENDATA (  CLEAR
 *                         | INSERT MOVEDOWN?
 *                         | DELETE MOVEDOWN? )
 * if the ESCAPE sequence corresponds to the ScreenMode terminal operation.
 * OPCODES ::= LINEDATA CLEAR
 * if the ESCAPE sequence corresponds to the LineMode terminal operation.
 *
 * @return 0 on successful processing of the ESCAPE sequence,
 *         1 if BUF contains an incomplete ESCAPE sequence,
 *        -1 if error an error occurred.
 */
int ltermProcessEscape(struct lterms *lts, const UNICHAR *buf,
                int count, const UNISTYLE *style, int *consumed, int *opcodes)
{
  struct LtermOutput *lto = &(lts->ltermOutput);

  LTERM_LOG(ltermProcessEscape,50,("count=%d, buf[1]='%c', cursorChar=%d, Chars=%d\n",
                count, buf[1], lto->outputCursorChar, lto->outputChars));

  if (count < 2) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  if (buf[1] == U_LBRACKET) {
    /* ESC [ Process CSI sequence */
    return ltermProcessCSISequence(lts, buf, count, style, consumed, opcodes);
  }

  if (buf[1] == U_RBRACKET) {
    /* ESC ] Process XTERM sequence */
    return ltermProcessXTERMSequence(lts, buf, count, style, consumed,opcodes);
  }

  if (buf[1] == U_LCURLY) {
    /* ESC { Process XMLTerm sequence */
    return ltermProcessXMLTermSequence(lts, buf, count, style, consumed,opcodes);
  }

  /* Set returned opcodes to zero */
  *opcodes = 0;

  /* Assume two characters will be consumed at this point */
  *consumed = 2;

  switch (buf[1]) {
  /* Three character sequences */
  case U_NUMBER:            /* ESC # 8 DEC Screen Alignment Test */
  case U_LPAREN:            /* ESC ( C Designate G0 Character Set */
  case U_RPAREN:            /* ESC ) C Designate G1 Character Set */
  case U_STAR:              /* ESC * C Designate G2 Character Set */
  case U_PLUS:              /* ESC + C Designate G3 Character Set */
  case U_DOLLAR:            /* ESC $ C Designate Kanji Character Set */
    LTERM_LOG(ltermProcessEscape,51,("3 char sequence, buf[1:2]='%c%c'\n",
                  buf[1], buf[2]));

    if (count < 3) {
      /* Incomplete Escape sequence */
      *consumed = 0;
      return 1;
    }
    *consumed = 3;

    if (buf[1] == U_NUMBER) {
      /* ESC # 8 DEC Screen Alignment Test */
    } else {
      /* ESC ()*+$ C Designate Character Set */
    }
    return 0;

  /* Two character sequences */
  case U_SEVEN:             /* ESC 7 Save Cursor */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_EIGHT:             /* ESC 8 Restore Cursor */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_EQUALS:            /* ESC = Application Keypad */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_GREATERTHAN:       /* ESC > Normal Keypad */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_D_CHAR:            /* ESC D Index (IND) */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_E_CHAR:            /* ESC E Next Line (NEL) */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_H_CHAR:            /* ESC H Tab Set (HTS) */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_M_CHAR:            /* ESC M Reverse Index (TI) */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_N_CHAR:            /* ESC N Single Shift Select of G2 Character Set */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_O_CHAR:            /* ESC O Single Shift Select of G3 Character Set */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_Z_CHAR:            /* ESC Z Obsolete form of ESC[c */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_c_CHAR:            /* ESC c Full reset (RIS)  */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_n_CHAR:            /* ESC n Invoke the G2 Character Set  */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  case U_o_CHAR:            /* ESC o Invoke the G3 Character Set */
    LTERM_WARNING("ltermProcessEscape: Warning - unimplemented %c\n",
                  buf[1]);
    return 0;

  default:
    LTERM_WARNING("ltermProcessEscape: Warning - unknown sequence %x\n",
                  buf[1]);
    return 0;
  }
}


/** Processes CSI sequence (a special case of Escape sequence processing)
 * @return 0 on success and -1 on error.
 */
static int ltermProcessCSISequence(struct lterms *lts, const UNICHAR *buf,
                int count, const UNISTYLE *style, int *consumed, int *opcodes)
{
  struct LtermOutput *lto = &(lts->ltermOutput);
  int offset, value, privateMode;
  int paramCount, paramValues[MAXESCAPEPARAMS], param1, param2;

  if (count < 3) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  LTERM_LOG(ltermProcessCSISequence,50,("buf[2]='%c', cursorChar=%d, Chars=%d\n",
                buf[2], lto->outputCursorChar, lto->outputChars));

  privateMode = 0;
  offset = 2;
  if (buf[2] == U_QUERYMARK) {
    /* ESC [ ? Process DEC Private Mode */
    privateMode = 1;
    offset = 3;
  }

  /* Set returned opcodes to zero */
  *opcodes = 0;

  /* Process numerical parameters */
  paramCount = 0;
  while ((offset < count) &&
         ((buf[offset] >= U_ZERO) && (buf[offset] <= U_NINE)) ) {
    /* Starts with a digit */
    value = buf[offset] - U_ZERO;
    offset++;

    /* Process all contiguous digits */
    while ((offset < count) &&
           ((buf[offset] >= U_ZERO) && (buf[offset] <= U_NINE)) ) {
      value = value * 10 + buf[offset] - U_ZERO;
      offset++;
    }

    if (offset == count) {
      /* Incomplete Escape sequence */
      *consumed = 0;
      return 1;
    }

    if (paramCount < MAXESCAPEPARAMS) {
      /* Store numerical parameter */
      paramValues[paramCount++] = value;

    } else {
      /* Numeric parameter buffer overflow */
      LTERM_WARNING("ltermProcessCSISequence: Warning - numeric parameter buffer overflow\n");
    }

    /* If next character not semicolon, stop processing */
    if (buf[offset] != U_SEMICOLON)
      break;

    /* Process next argument */
    offset++;
  }

  if (offset == count) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  /* Assume that all parsed characters will be consumed at this point */
  *consumed = offset+1;

  LTERM_LOG(ltermProcessCSISequence,51,("paramCount=%d, offset=%d, buf[offset]='%d'\n",
                 paramCount, offset, buf[offset]));

  if (privateMode) {
    /* ESC [ ? Process DEC Private Mode */
    return ltermProcessDECPrivateMode( lts, paramValues, paramCount,
                                       buf[offset], opcodes);
  }

  /* Set returned opcodes to zero */
  *opcodes = 0;

  /* Default parameter values: 1, 1 */
  param1 = (paramCount > 0) ? paramValues[0] : 1;
  param2 = (paramCount > 1) ? paramValues[1] : 1;

  switch (buf[offset]) {

  case U_ATSIGN:    /* Insert Ps (Blank) Character(s) [default: 1] (ICH) */
    if (ltermInsDelEraseChar(lts, param1, LTERM_INSERT_CHAR) != 0)
      return -1;
    return 0;

  case U_e_CHAR:
  case U_A_CHAR:    /* Cursor Up Ps Times [default: 1] (CUU) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_B_CHAR:    /* Cursor Down Ps Times [default: 1] (CUD) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_a_CHAR:
  case U_C_CHAR:    /* Cursor Forward Ps Times [default: 1] (CUF) */
    LTERM_LOG(ltermProcessCSISequence,52,("Cursor Forward Count = %d\n",
                   param1));

    if (lto->outputMode == LTERM2_LINE_MODE) {

      if (param1 <= (lto->outputChars - lto->outputCursorChar)) {
        lto->outputCursorChar += param1;
      } else {
        lto->outputCursorChar = lto->outputChars;
      }
    }
    return 0;

  case U_D_CHAR:    /* Cursor Backward Ps Times [default: 1] (CUB) */
    LTERM_LOG(ltermProcessCSISequence,52,("Cursor Back Count = %d\n",
                   param1));

    if (lto->outputMode == LTERM2_LINE_MODE) {

      if (param1 <= lto->outputCursorChar) {
        lto->outputCursorChar -= param1;
      } else {
        lto->outputCursorChar = 0;
      }
    }
    return 0;

  case U_E_CHAR:    /* Cursor Down Ps Times [default: 1] and to first column */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_F_CHAR:    /* Cursor Up Ps Times [default: 1] and to first column */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_SNGLQUOTE:
  case U_G_CHAR:    /* Cursor to Column Ps (HPA) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_H_CHAR:    /* Cursor Position [row;column] [default: 1;1] (CUP) */
    LTERM_LOG(ltermProcessCSISequence,52,("Cursor Position = (%d, %d)\n",
                   param1, param2));

    if (lto->outputMode == LTERM2_LINE_MODE) {

      if ((param1 > 0) && ((param1-1) <= lto->outputChars)) {
        lto->outputCursorChar = param1-1;
      } else {
        lto->outputCursorChar = 0;
      }
    }
    return 0;

  case U_I_CHAR:    /* Move forward Ps tab stops [default: 1] */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_J_CHAR:    /* Erase in Display (ED)
                     * Ps = 0 Clear Below (default) 
                     * Ps = 1 Clear Above
                     * Ps = 2 Clear All 
                     */
    param1 = (paramCount > 0) ? paramValues[0] : 0;

    LTERM_LOG(ltermProcessCSISequence,52,("Erase display code %d\n",
                   param1));

    if (lto->outputMode == LTERM2_LINE_MODE) {
      /* Clear line */
      ltermClearOutputLine(lts);

      /* Set opcodes to return incomplete line */
      *opcodes = LTERM_LINEDATA_CODE|LTERM_CLEAR_CODE;
    }
    return 0;

  case U_K_CHAR:    /* Erase in Line (EL)
                     * Ps = 0 Clear to Right (default) 
                     * Ps = 1 Clear to Left
                     * Ps = 2 Clear All 
                     */
    param1 = (paramCount > 0) ? paramValues[0] : 0;

    LTERM_LOG(ltermProcessCSISequence,52,("Line erase code %d\n",
                   param1));

    if (lto->outputMode == LTERM2_LINE_MODE) {

      switch(param1) {
      case 0:        /* Clear to Right */
        lto->outputChars = lto->outputCursorChar;
        lto->outputModifiedChar = lto->outputCursorChar;
        break;
      case 1:        /* Clear to Left */
        lto->outputChars -= lto->outputCursorChar;
        lto->outputCursorChar = 0;
        lto->outputModifiedChar = 0;
        break;
      case 2:        /* Clear All */
        lto->outputChars = 0;
        lto->outputCursorChar = 0;
        lto->outputModifiedChar = 0;
        break;
      default:       /* Invalid erase code */
        LTERM_WARNING("ltermProcessCSISequence: Warning - invalid erase code %d\n", param1);
      }

    }
    return 0;

  case U_L_CHAR:    /* Insert Ps Line(s) [default: 1] (IL) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_M_CHAR:    /* Delete Ps Line(s) [default: 1] (DL) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_P_CHAR:    /* Delete Ps Character(s) [default: 1] (DCH) */
    if (ltermInsDelEraseChar(lts, param1, LTERM_DELETE_CHAR) != 0)
      return -1;
    return 0;

  case U_T_CHAR:    /* Initiate hilite mouse tracking. Parameters
                     * [func;startx;starty;firstrow;lastrow]. 
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_W_CHAR:    /* Tabulator functions:
                     *  Ps = 0  Tab Set (HTS)
                     *  Ps = 2  Tab Clear (TBC), Clear Current Column (default)
                     *  Ps = 5  Tab Clear (TBC), Clear All 
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_X_CHAR:    /* Erase Ps Character(s) [default: 1] (ECH) */
    if (ltermInsDelEraseChar(lts, param1, LTERM_ERASE_CHAR) != 0)
      return -1;
    return 0;

  case U_Z_CHAR:    /* Move backward Ps [default: 1] tab stops */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_c_CHAR:    /* Send Device Attributes (DA)
                     * Ps = 0 (or omitted) : request attributes from terminal 
                     * returns: ESC[?1;2c
                     *          (``I am a VT100 with Advanced Video Option'') 
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_d_CHAR:    /* Cursor to Line Ps (VPA) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_f_CHAR:    /* Horizontal and Vertical Position [row;column] (HVP)
                     * [default: 1;1]
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_g_CHAR:    /* Tab Clear (TBC) 
                     * Ps = 0 Clear Current Column (default) 
                     * Ps = 3 Clear All (TBC)
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_i_CHAR:    /* Printing 
                     * Ps = 4 disable transparent print mode (MC4) 
                     * Ps = 5 enable transparent print mode (MC5)
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_h_CHAR:    /* Set Mode (SM)
                     * Ps =  4 Insert Mode (SMIR)
                     * Ps = 20 Automatic Newline (LNM)
                     */
    LTERM_LOG(ltermProcessCSISequence,52,("Set Mode %d\n",
                  param1));

    if (param1 == 4) {
      lto->insertMode = 1;
    } else if (param1 == 4) {
      lto->automaticNewline = 1;
    }
    return 0;

  case U_l_CHAR:    /* Reset Mode (RM)
                     * Ps =  4 Replace Mode (RMIR)
                     * Ps = 20 Normal Linefeed (LNM)
                     */
    LTERM_LOG(ltermProcessCSISequence,52,("Reset Mode %d\n",
                  param1));

    if (param1 == 4) {
      lto->insertMode = 0;
    } else if (param1 == 4) {
      lto->automaticNewline = 0;
    }
    return 0;

  case U_m_CHAR:    /* Character Attributes (SGR) 
                     * Ps = 0 Normal (default) 
                     * Ps = 1 / 22 On / Off Bold (bright fg) 
                     * Ps = 4 / 24 On / Off Underline 
                     * Ps = 5 / 25 On / Off Blink (bright bg) 
                     * Ps = 7 / 27 On / Off Inverse 
                     * Ps = 30 / 40 fg/bg Black 
                     * Ps = 31 / 41 fg/bg Red 
                     * Ps = 32 / 42 fg/bg Green 
                     * Ps = 33 / 43 fg/bg Yellow 
                     * Ps = 34 / 44 fg/bg Blue 
                     * Ps = 35 / 45 fg/bg Magenta 
                     * Ps = 36 / 46 fg/bg Cyan 
                     * Ps = 37 / 47 fg/bg White 
                     * Ps = 39 / 49 fg/bg Default
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_n_CHAR:    /* Device Status Report (DSR) 
                     * Ps = 5 Status Report ESC [ 0 n (``OK'') 
                     * Ps = 6 Report Cursor Position (CPR) [row;column]
                     *         as ESC [ r ; c R 
                     * Ps = 7 Request Display Name 
                     * Ps = 8 Request Version Number (place in window title)
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_r_CHAR:    /* Set Scrolling Region [top;bottom] 
                     * [default: full size of window] (CSR) 
                     */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  case U_x_CHAR:    /* Request Terminal Parameters (DECREQTPARM) */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unimplemented %c\n",
                  buf[offset]);
    return 0;

  default:          /* Unknown Escape sequence */
    LTERM_WARNING("ltermProcessCSISequence: Warning - unknown sequence %x\n",
                  buf[offset]);
    return 0;
  }

}


/** Processes DEC Private Modes (a special case of CSI sequence processing)
 * ESC [ ? Pm h
 * @return 0 on success and -1 on error.
 */
static int ltermProcessDECPrivateMode(struct lterms *lts,
            const int *paramValues, int paramCount, UNICHAR uch, int *opcodes)
{
  struct LtermOutput *lto = &(lts->ltermOutput);

  LTERM_LOG(ltermProcessDECPrivateMode,50,("ch='%c', cursorChar=%d, Chars=%d\n",
                (char) uch, lto->outputCursorChar, lto->outputChars));

  /* Set returned opcodes to zero */
  *opcodes = 0;

  switch (uch) {
  case U_h_CHAR:    /* DEC Private Mode Set (DECSET) */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unimplemented %c\n",
                  (char) uch);
    return 0;

  case U_l_CHAR:    /* DEC Private Mode Reset (DECRST) */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unimplemented %c\n",
                  (char) uch);
    return 0;

  case U_r_CHAR:    /* Restore previously saved DEC Private Mode Values */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unimplemented %c\n",
                  (char) uch);
    return 0;

  case U_s_CHAR:    /* Save DEC Private Mode Values */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unimplemented %c\n",
                  (char) uch);
    return 0;

  case U_t_CHAR:    /* Toggle DEC Private Mode Values */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unimplemented %c\n",
                  (char) uch);
    return 0;

  default:          /* Unknown escape sequence */
    LTERM_WARNING("ltermProcessDECPrivateMode: Warning - unknown sequence %x\n",
                  uch);
    return 0;
  }

}


/** Processes XTERM sequence (a special case of Escape sequence processing)
 * ESC ] Ps;Pt BEL
 * @return 0 on success and -1 on error.
 */
static int ltermProcessXTERMSequence(struct lterms *lts, const UNICHAR *buf,
                  int count, const UNISTYLE *style,int *consumed,int *opcodes)
{
  struct LtermOutput *lto = &(lts->ltermOutput);
  int offset, paramValue, strLength;
  UNICHAR paramString[MAXSTRINGPARAM+1];

  if (count < 3) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  LTERM_LOG(ltermProcessXTERMSequence,50,("cursorChar=%d, Chars=%d\n",
                lto->outputCursorChar, lto->outputChars));

  offset = 2;

  /* Set returned opcodes to zero */
  *opcodes = 0;

  /* Process numerical parameter */
  paramValue = 0;
  while ((offset < count) &&
         ((buf[offset] >= U_ZERO) && (buf[offset] <= U_NINE)) ) {
    paramValue = paramValue * 10 + buf[offset] - U_ZERO;
    offset++;
  }

  if (offset == count) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  if (buf[offset] != U_SEMICOLON) {
    /* If next character not semicolon, return */
    *consumed = offset;
    return 0;
  }

  LTERM_LOG(ltermProcessXTERMSequence,51,("paramValue=%d, offset=%d, buf[offset]=%x\n",
                 paramValue, offset, buf[offset]));

  /* Skip the semicolon */
  offset++;

  /* Process string parameter */
  strLength = 0;
  while ((offset < count) && (buf[offset] != U_BEL)) {
    if (strLength < MAXSTRINGPARAM) {
      paramString[strLength++] = buf[offset];
      offset++;

    } else {
      LTERM_WARNING("ltermProcessXTERMSequence: Warning - string parameter too long; truncated\n");
      break;
    }
  }

  if (offset == count) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  /* Insert terminating NULL character in string */
  paramString[strLength] = U_NUL;

  LTERM_LOGUNICODE(ltermProcessXTERMSequence,52,(paramString, strLength));

  /* All parsed characters have been consumed at this point */
  *consumed = offset+1;

  LTERM_WARNING("ltermProcessXTERMSequence: Warning - unimplemented %c\n",
                buf[offset]);

  return 0;
}

/** Processes XMLTerm sequence (a special case of Escape sequence processing)
 * XMLterm escape sequences are of the form
 *   ESC { Pm C Pt BEL
 *  where Pm denotes multiple numeric arguments separated by semicolons,
 *  character C is a not a digit and not a semicolon, and
 *  text parameter Pt may be a null string.
 *  Omitted numeric parameters are assumed to be zero.
 * @return 0 on success and -1 on error.
 */
static int ltermProcessXMLTermSequence(struct lterms *lts, const UNICHAR *buf,
                  int count, const UNISTYLE *style,int *consumed,int *opcodes)
{
  struct LtermOutput *lto = &(lts->ltermOutput);
  int offset, value, strLength;
  int paramCount, paramValues[MAXESCAPEPARAMS], param1, param2, param3;
  UNICHAR termChar, paramString[MAXSTRINGPARAM+1];
  char paramCString[MAXSTRINGPARAM+1];

  if (count < 4) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  LTERM_LOG(ltermProcessXMLTermSequence,50,("cursorChar=%d, Chars=%d\n",
                lto->outputCursorChar, lto->outputChars));

  offset = 2;

  /* Set returned opcodes to zero */
  *opcodes = 0;

  /* Process numerical parameters */
  paramCount = 0;
  while ((offset < count) &&
         ((buf[offset] >= U_ZERO) && (buf[offset] <= U_NINE)) ) {
    /* Starts with a digit */
    value = buf[offset] - U_ZERO;
    offset++;

    /* Process all contiguous digits */
    while ((offset < count) &&
           ((buf[offset] >= U_ZERO) && (buf[offset] <= U_NINE)) ) {
      value = value * 10 + buf[offset] - U_ZERO;
      offset++;
    }

    if (offset == count) {
      /* Incomplete Escape sequence */
      *consumed = 0;
      return 1;
    }

    if (paramCount < MAXESCAPEPARAMS) {
      /* Store numerical parameter */
      paramValues[paramCount++] = value;

    } else {
      /* Numeric parameter buffer overflow */
      LTERM_WARNING("ltermProcessXMLTermSequence: Warning - numeric parameter buffer overflow\n");
    }

    /* If next character not semicolon, stop processing */
    if (buf[offset] != U_SEMICOLON)
      break;

    /* Process next argument */
    offset++;
  }

  if (offset == count) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  /* Terminating character */
  termChar = buf[offset];

  /* Skip terminating character */
  offset++;

  LTERM_LOG(ltermProcessXMLTermSequence,51,("paramCount=%d, offset=%d, termChar=%x\n",
                 paramCount, offset, termChar));

  /* Process string parameter */
  strLength = 0;
  while ((offset < count) && (buf[offset] != U_BEL)) {
    if (strLength < MAXSTRINGPARAM) {
      paramCString[strLength] = buf[offset];
      paramString[strLength++] = buf[offset];
      offset++;

    } else {
      LTERM_WARNING("ltermProcessXMLTermSequence: Warning - string parameter too long; truncated\n");
      break;
    }
  }

  if (offset == count) {
    /* Incomplete Escape sequence */
    *consumed = 0;
    return 1;
  }

  /* Insert terminating NULL character in string */
  paramCString[strLength] = U_NUL;
  paramString[strLength] = U_NUL;

  LTERM_LOGUNICODE(ltermProcessXMLTermSequence,52,(paramString, strLength));

  /* All parsed characters have been consumed at this point (including BEL) */
  *consumed = offset+1;

  /* Default parameter values: 0, 0, 0 */
  param1 = (paramCount > 0) ? paramValues[0] : 0;
  param2 = (paramCount > 1) ? paramValues[1] : 0;
  param3 = (paramCount > 2) ? paramValues[2] : 0;

  switch (termChar) {
  int streamOpcodes;

  case U_D_CHAR:    /* Set debugging messageLevel/search-string */
    LTERM_LOG(ltermProcessXMLTermSequence,52,("Setting message level etc.\n"));
    if (strLength == 0) {
      tlog_set_level(param1, param2, NULL);
    } else {
      tlog_set_level(param1, param2, paramCString);
    }
    return 0;

  case U_E_CHAR:    /* Enable/disable input echo setting */
    if (param1) {
      /* Enable input echo */
      lts->disabledInputEcho = 0;

      LTERM_LOG(ltermProcessXMLTermSequence,52,("Enabled input echo\n"));
    } else {
      /* Disable input echo */
      lts->disabledInputEcho = 1;

      LTERM_LOG(ltermProcessXMLTermSequence,52,("Disabled input echo\n"));
    }
    return 0;

  case U_R_CHAR:    /* Enable/disable raw input mode */
    if (param1) {
      /* Switch to raw input mode */
      ltermSwitchToRawMode(lts);

      LTERM_LOG(ltermProcessXMLTermSequence,52,("Raw input mode\n"));

    } else {
      /* Switch to regular input mode */
      ltermClearInputLine(lts);

      LTERM_LOG(ltermProcessXMLTermSequence,52,("Line input mode\n"));
    }
    return 0;

  case U_J_CHAR:    /* Switch to null-terminated Javascript stream mode */
  case U_S_CHAR:    /* Switch to null-terminated stream mode, with cookie */

    if (lto->outputMode == LTERM1_SCREEN_MODE) {
      *opcodes = LTERM_SCREENDATA_CODE | LTERM_OUTPUT_CODE;
    } else if (lto->outputMode == LTERM2_LINE_MODE) {
      *opcodes = LTERM_LINEDATA_CODE | LTERM_OUTPUT_CODE;
    }

    assert(lto->outputMode != LTERM0_STREAM_MODE);

    /* Set stream codes */
    streamOpcodes = 0;

    if (termChar == U_J_CHAR) {
      /* Javascript stream */
      streamOpcodes |= LTERM_JSSTREAM_CODE;

    } else {
      /* HTML/XML stream */
      if (param1)
        streamOpcodes |= LTERM_DOCSTREAM_CODE;

      if (param2)
        streamOpcodes |= LTERM_XMLSTREAM_CODE;

      if (param3)
        streamOpcodes |= LTERM_WINSTREAM_CODE;
    }

    /* Check if cookie matches */
    if ((strLength > 0) && (strcmp(paramCString, lts->cookie) == 0)) {
      streamOpcodes |= LTERM_COOKIESTR_CODE;
    }

    LTERM_LOG(ltermProcessXMLTermSequence,52,
              ("Switching to stream mode, codes=0x%x\n", streamOpcodes));

    if (0 != ltermSwitchToStreamMode(lts, streamOpcodes, NULL))
      return -1;

    return 0;

  default:          /* Unknown escape sequence */
    LTERM_WARNING("ltermProcessXMLTermSequence: Warning - unknown sequence %c\n",
                   termChar);
    return 0;
  }

  return 0;
}


/** Insert/delete/erase COUNT characters at current output cursor position.
 * ACTION = LTERM_INSERT_CHAR (insert)
 *          LTERM_DELETE_CHAR (delete)
 *          LTERM_ERASE_CHAR  (erase)
 * @return 0 on success and -1 on error.
 */
int ltermInsDelEraseChar(struct lterms *lts, int count, int action)
{
  struct LtermOutput *lto = &(lts->ltermOutput);
  int charCount = count;
  int j;

  assert(count >= 0);

  LTERM_LOG(ltermInsDelEraseChar,60,("count=%d, action=%d\n",
                 count, action));

  if (lto->outputMode == LTERM2_LINE_MODE) {
    /* Line mode */
    switch (action) {

    case LTERM_INSERT_CHAR:

      if (lto->outputChars + charCount > MAXCOLM1) {
        /* Output buffer overflow; ignore extra inserts */
        LTERM_WARNING("ltermInsDelEraseChar: Warning - output line buffer overflow\n");
        charCount = MAXCOLM1 - lto->outputChars;
      }

      LTERM_LOG(ltermInsDelEraseChar,62,("Line insert %d blank chars\n",
                     charCount));

      /* Shift characters to the right to make room for insertion */
      for (j=lto->outputChars-1; j>=lto->outputCursorChar; j--) {
        lto->outputLine[j+charCount] = lto->outputLine[j];
        lto->outputStyle[j+charCount] = lto->outputStyle[j];
      }

      /* Insert blank characters */
      for (j=lto->outputCursorChar; j<lto->outputCursorChar+charCount; j++) {
        lto->outputLine[j] = U_SPACE;
        lto->outputStyle[j] = LTERM_STDOUT_STYLE | lto->styleMask;
      }

      /* Increment character count */
      lto->outputChars += charCount;
      break;

    case LTERM_DELETE_CHAR:
      if (lto->outputCursorChar+charCount > lto->outputChars)
        charCount = lto->outputChars - lto->outputCursorChar;

      LTERM_LOG(ltermInsDelEraseChar,62,("Delete %d chars\n",
                     charCount));

      /* Shift characters to the left */
      for (j=lto->outputCursorChar; j<lto->outputChars-charCount; j++) {
        lto->outputLine[j] = lto->outputLine[j+charCount];
        lto->outputStyle[j] = lto->outputStyle[j+charCount];
      }

      /* Decrement character count */
      lto->outputChars -= charCount;
      break;

    case LTERM_ERASE_CHAR:
      if (lto->outputCursorChar+charCount > lto->outputChars)
        charCount = lto->outputChars - lto->outputCursorChar;

      LTERM_LOG(ltermInsDelEraseChar,62,("Erase %d chars\n",
                     charCount));

      /* Erase characters */
      for (j=lto->outputCursorChar; j<lto->outputCursorChar+charCount; j++) {
        lto->outputLine[j] = U_SPACE;
        lto->outputStyle[j] = LTERM_STDOUT_STYLE | lto->styleMask;
      }

      break;
    }

    /* Note modifications */
    if (lto->outputCursorChar < lto->outputModifiedChar)
      lto->outputModifiedChar = lto->outputCursorChar;

  } else if (lto->outputMode == LTERM1_SCREEN_MODE) {
    /* Screen mode */
  }

  return 0;
}
