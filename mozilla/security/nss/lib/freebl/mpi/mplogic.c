/*
 *  mplogic.c
 *
 *  Bitwise logical operations on MPI values
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the MPI Arbitrary Precision Integer Arithmetic
 * library.
 *
 * The Initial Developer of the Original Code is Michael J. Fromberger.
 * Portions created by Michael J. Fromberger are 
 * Copyright (C) 1998, 1999, 2000 Michael J. Fromberger. 
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable
 * instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the GPL.
 *
 *  $Id$
 */

#include "mplogic.h"
#include <stdlib.h>

/* Some things from the guts of the MPI library we make use of... */
extern mp_err   s_mp_lshd(mp_int *mp, mp_size p);
extern void     s_mp_rshd(mp_int *mp, mp_size p);
extern mp_err   s_mp_pad(mp_int *mp, mp_size min); /* left pad with zeroes */

#define  s_mp_clamp(mp)\
   { while(USED(mp) > 1 && DIGIT((mp), USED(mp) - 1) == 0) USED(mp) -= 1; }

/* {{{ Lookup table for population count */

static unsigned char bitc[] = {
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
   4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/* }}} */

/*------------------------------------------------------------------------*/
/*
  mpl_not(a, b)    - compute b = ~a
  mpl_and(a, b, c) - compute c = a & b
  mpl_or(a, b, c)  - compute c = a | b
  mpl_xor(a, b, c) - compute c = a ^ b
 */

/* {{{ mpl_not(a, b) */

mp_err mpl_not(mp_int *a, mp_int *b)
{
  mp_err   res;
  int      ix;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  /* This relies on the fact that the digit type is unsigned */
  for(ix = 0; ix < USED(b); ix++) 
    DIGIT(b, ix) = ~DIGIT(b, ix);

  s_mp_clamp(b);

  return MP_OKAY;

} /* end mpl_not() */

/* }}} */

/* {{{ mpl_and(a, b, c) */

mp_err mpl_and(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) <= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) &= DIGIT(other, ix);

  s_mp_clamp(c);

  return MP_OKAY;

} /* end mpl_and() */

/* }}} */

/* {{{ mpl_or(a, b, c) */

mp_err mpl_or(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) >= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) |= DIGIT(other, ix);

  return MP_OKAY;

} /* end mpl_or() */

/* }}} */

/* {{{ mpl_xor(a, b, c) */

mp_err mpl_xor(mp_int *a, mp_int *b, mp_int *c)
{
  mp_int  *which, *other;
  mp_err   res;
  int      ix;

  ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);

  if(USED(a) >= USED(b)) {
    which = a;
    other = b;
  } else {
    which = b;
    other = a;
  }

  if((res = mp_copy(which, c)) != MP_OKAY)
    return res;

  for(ix = 0; ix < USED(which); ix++)
    DIGIT(c, ix) ^= DIGIT(other, ix);

  s_mp_clamp(c);

  return MP_OKAY;

} /* end mpl_xor() */

/* }}} */

/*------------------------------------------------------------------------*/
/*
  mpl_rsh(a, b, d)     - b = a >> d
  mpl_lsh(a, b, d)     - b = a << d
 */

/* {{{ mpl_rsh(a, b, d) */

mp_err mpl_rsh(mp_int *a, mp_int *b, mp_digit d)
{
  mp_err   res;
  mp_digit dshift, bshift;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  dshift = d / DIGIT_BIT;  /* How many whole digits to shift by */
  bshift = d % DIGIT_BIT;  /* How many bits to shift by         */

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  /* Shift over as many whole digits as necessary */
  if(dshift) 
    s_mp_rshd(b, dshift);

  /* Now handle any remaining bit shifting */
  if(bshift)
  {
    mp_digit  prev = 0, next, mask = (1 << bshift) - 1;
    int       ix;

    /*
      'mask' is a digit with the lower bshift bits set, the rest
      clear.  It is used to mask off the bottom bshift bits of each
      digit, which are then shifted on to the top of the next lower
      digit.
     */
    for(ix = USED(b) - 1; ix >= 0; ix--) {
      /* Take off the lower bits and shift them up... */
      next = (DIGIT(b, ix) & mask) << (DIGIT_BIT - bshift);

      /* Shift down the current digit, and mask in the bits saved
	 from the previous digit 
       */
      DIGIT(b, ix) = (DIGIT(b, ix) >> bshift) | prev;
      prev = next;
    }
  }

  s_mp_clamp(b);   

  return MP_OKAY;

} /* end mpl_rsh() */

/* }}} */

/* {{{ mpl_lsh(a, b, d) */

mp_err mpl_lsh(mp_int *a, mp_int *b, mp_digit d)
{
  mp_err   res;
  mp_digit dshift, bshift;

  ARGCHK(a != NULL && b != NULL, MP_BADARG);

  dshift = d / DIGIT_BIT;
  bshift = d % DIGIT_BIT;

  if((res = mp_copy(a, b)) != MP_OKAY)
    return res;

  if(dshift)
    if((res = s_mp_lshd(b, dshift)) != MP_OKAY)
      return res;

  if(bshift){ 
    int       ix;
    mp_digit  prev = 0, next, mask = (1 << bshift) - 1;

    for(ix = 0; ix < USED(b); ix--) {
      next = (DIGIT(b, ix) >> (DIGIT_BIT - bshift)) & mask;

      DIGIT(b, ix) = (DIGIT(b, ix) << bshift) | prev;
      prev = next;
    }
  }

  s_mp_clamp(b);

  return MP_OKAY;

} /* end mpl_lsh() */

/* }}} */

/*------------------------------------------------------------------------*/
/*
  mpl_num_set(a, num)

  Count the number of set bits in the binary representation of a.
  Returns MP_OKAY and sets 'num' to be the number of such bits, if
  possible.  If num is NULL, the result is thrown away, but it is
  not considered an error.

  mpl_num_clear() does basically the same thing for clear bits.
 */

/* {{{ mpl_num_set(a, num) */

mp_err mpl_num_set(mp_int *a, int *num)
{
  int            ix, db, nset = 0;
  mp_digit       cur;
  unsigned char  reg;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    cur = DIGIT(a, ix);
    
    for(db = 0; db < sizeof(mp_digit); db++) {
      reg = (unsigned char)(cur >> (CHAR_BIT * db));

      nset += bitc[reg];
    }
  }

  if(num)
    *num = nset;

  return MP_OKAY;

} /* end mpl_num_set() */

/* }}} */

/* {{{ mpl_num_clear(a, num) */

mp_err mpl_num_clear(mp_int *a, int *num)
{
  int            ix, db, nset = 0;
  mp_digit       cur;
  unsigned char  reg;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    cur = DIGIT(a, ix);
    
    for(db = 0; db < sizeof(mp_digit); db++) {
      reg = (unsigned char)(cur >> (CHAR_BIT * db));

      nset += bitc[UCHAR_MAX - reg];
    }
  }

  if(num)
    *num = nset;

  return MP_OKAY;


} /* end mpl_num_clear() */

/* }}} */

/*------------------------------------------------------------------------*/
/*
  mpl_parity(a)

  Determines the bitwise parity of the value given.  Returns MP_EVEN
  if an even number of digits are set, MP_ODD if an odd number are
  set.
 */

/* {{{ mpl_parity(a) */

mp_err mpl_parity(mp_int *a)
{
  int      ix, par = 0;
  mp_digit cur;

  ARGCHK(a != NULL, MP_BADARG);

  for(ix = 0; ix < USED(a); ix++) {
    int   shft = (sizeof(mp_digit) * CHAR_BIT) / 2;

    cur = DIGIT(a, ix);

    /* Compute parity for current digit */
    while(shft != 0) {
      cur ^= (cur >> shft);
      shft >>= 1;
    }
    cur &= 1;

    /* XOR with running parity so far   */
    par ^= cur;
  }

  if(par)
    return MP_ODD;
  else
    return MP_EVEN;

} /* end mpl_parity() */

/* }}} */

/*
  mpl_set_bit

  Returns MP_OKAY or some error code.
  Grows a if needed to set a bit to 1.
 */
mp_err mpl_set_bit(mp_int *a, unsigned int bitNum, unsigned int value)
{
  unsigned int ix;
  mp_err       rv;
  mp_digit     mask;

  ARGCHK(a != NULL, MP_BADARG);

  ix = bitNum / MP_DIGIT_BIT;
  if (value && (ix > MP_USED(a) - 1)) {
    rv = s_mp_pad(a, ix);
    if (rv != MP_OKAY)
      return rv;
  }

  bitNum = bitNum % MP_DIGIT_BIT;
  mask = (mp_digit)1 << bitNum;
  if (value)
    MP_DIGIT(a,ix) |= mask;
  else
    MP_DIGIT(a,ix) &= ~mask;
  return MP_OKAY;
}

/*
  mpl_get_bit

  returns 0 or 1 or some (negative) error code.
 */
mp_err mpl_get_bit(mp_int *a, unsigned int bitNum)
{
  unsigned int bit, ix;
  mp_err       rv;

  ARGCHK(a != NULL, MP_BADARG);

  ix = bitNum / MP_DIGIT_BIT;
  ARGCHK(ix <= MP_USED(a) - 1, MP_RANGE);

  bit   = bitNum % MP_DIGIT_BIT;
  rv = (mp_err)(MP_DIGIT(a, ix) >> bit) & 1;
  return rv;
}

/*
  mpl_significant_bits
  returns number of significnant bits in abs(a).
  returns 1 if value is zero.
 */
mp_err mpl_significant_bits(mp_int *a)
{
  mp_err bits 	= 0;
  int    ix;

  ARGCHK(a != NULL, MP_BADARG);

  ix = MP_USED(a);
  for (ix = MP_USED(a); ix > 0; ) {
    mp_digit d = MP_DIGIT(a, --ix);
    if (d) {
      while (d) {
	++bits;
	d >>= 1;
      }
      break;
    }
  }
  bits += ix * MP_DIGIT_BIT;
  if (!bits)
    bits = 1;
  return bits;
}

/*------------------------------------------------------------------------*/
/* HERE THERE BE DRAGONS                                                  */
