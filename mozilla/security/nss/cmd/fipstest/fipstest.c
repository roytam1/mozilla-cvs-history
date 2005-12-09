/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "secitem.h"
#include "blapi.h"
#include "nss.h"
#if 0
#include "../../lib/freebl/mpi/mpi.h"
#endif

#define ENCRYPT 1
#define DECRYPT 0
                  
static const unsigned char
table3[32][8] = {
  { 0x10, 0x46, 0x91, 0x34, 0x89, 0x98, 0x01, 0x31 },
  { 0x10, 0x07, 0x10, 0x34, 0x89, 0x98, 0x80, 0x20 },
  { 0x10, 0x07, 0x10, 0x34, 0xc8, 0x98, 0x01, 0x20 },
  { 0x10, 0x46, 0x10, 0x34, 0x89, 0x98, 0x80, 0x20 },
  { 0x10, 0x86, 0x91, 0x15, 0x19, 0x19, 0x01, 0x01 },
  { 0x10, 0x86, 0x91, 0x15, 0x19, 0x58, 0x01, 0x01 },
  { 0x51, 0x07, 0xb0, 0x15, 0x19, 0x58, 0x01, 0x01 },
  { 0x10, 0x07, 0xb0, 0x15, 0x19, 0x19, 0x01, 0x01 },
  { 0x31, 0x07, 0x91, 0x54, 0x98, 0x08, 0x01, 0x01 },
  { 0x31, 0x07, 0x91, 0x94, 0x98, 0x08, 0x01, 0x01 },
  { 0x10, 0x07, 0x91, 0x15, 0xb9, 0x08, 0x01, 0x40 },
  { 0x31, 0x07, 0x91, 0x15, 0x98, 0x08, 0x01, 0x40 },
  { 0x10, 0x07, 0xd0, 0x15, 0x89, 0x98, 0x01, 0x01 },
  { 0x91, 0x07, 0x91, 0x15, 0x89, 0x98, 0x01, 0x01 },
  { 0x91, 0x07, 0xd0, 0x15, 0x89, 0x19, 0x01, 0x01 },
  { 0x10, 0x07, 0xd0, 0x15, 0x98, 0x98, 0x01, 0x20 },
  { 0x10, 0x07, 0x94, 0x04, 0x98, 0x19, 0x01, 0x01 },
  { 0x01, 0x07, 0x91, 0x04, 0x91, 0x19, 0x04, 0x01 },
  { 0x01, 0x07, 0x91, 0x04, 0x91, 0x19, 0x01, 0x01 },
  { 0x01, 0x07, 0x94, 0x04, 0x91, 0x19, 0x04, 0x01 },
  { 0x19, 0x07, 0x92, 0x10, 0x98, 0x1a, 0x01, 0x01 },
  { 0x10, 0x07, 0x91, 0x19, 0x98, 0x19, 0x08, 0x01 },
  { 0x10, 0x07, 0x91, 0x19, 0x98, 0x1a, 0x08, 0x01 },
  { 0x10, 0x07, 0x92, 0x10, 0x98, 0x19, 0x01, 0x01 },
  { 0x10, 0x07, 0x91, 0x15, 0x98, 0x19, 0x01, 0x0b },
  { 0x10, 0x04, 0x80, 0x15, 0x98, 0x19, 0x01, 0x01 },
  { 0x10, 0x04, 0x80, 0x15, 0x98, 0x19, 0x01, 0x02 },
  { 0x10, 0x04, 0x80, 0x15, 0x98, 0x19, 0x01, 0x08 },
  { 0x10, 0x02, 0x91, 0x15, 0x98, 0x10, 0x01, 0x04 },
  { 0x10, 0x02, 0x91, 0x15, 0x98, 0x19, 0x01, 0x04 },
  { 0x10, 0x02, 0x91, 0x15, 0x98, 0x10, 0x02, 0x01 },
  { 0x10, 0x02, 0x91, 0x16, 0x98, 0x10, 0x01, 0x01 }
};

static const unsigned char
table4_key[19][8] = {
  { 0x7c, 0xa1, 0x10, 0x45, 0x4a, 0x1a, 0x6e, 0x57 },
  { 0x01, 0x31, 0xd9, 0x61, 0x9d, 0xc1, 0x37, 0x6e },
  { 0x07, 0xa1, 0x13, 0x3e, 0x4a, 0x0b, 0x26, 0x86 },
  { 0x38, 0x49, 0x67, 0x4c, 0x26, 0x02, 0x31, 0x9e },
  { 0x04, 0xb9, 0x15, 0xba, 0x43, 0xfe, 0xb5, 0xb6 },
  { 0x01, 0x13, 0xb9, 0x70, 0xfd, 0x34, 0xf2, 0xce },
  { 0x01, 0x70, 0xf1, 0x75, 0x46, 0x8f, 0xb5, 0xe6 },
  { 0x43, 0x29, 0x7f, 0xad, 0x38, 0xe3, 0x73, 0xfe },
  { 0x07, 0xa7, 0x13, 0x70, 0x45, 0xda, 0x2a, 0x16 },
  { 0x04, 0x68, 0x91, 0x04, 0xc2, 0xfd, 0x3b, 0x2f },
  { 0x37, 0xd0, 0x6b, 0xb5, 0x16, 0xcb, 0x75, 0x46 },
  { 0x1f, 0x08, 0x26, 0x0d, 0x1a, 0xc2, 0x46, 0x5e },
  { 0x58, 0x40, 0x23, 0x64, 0x1a, 0xba, 0x61, 0x76 },
  { 0x02, 0x58, 0x16, 0x16, 0x46, 0x29, 0xb0, 0x07 },
  { 0x49, 0x79, 0x3e, 0xbc, 0x79, 0xb3, 0x25, 0x8f },
  { 0x4f, 0xb0, 0x5e, 0x15, 0x15, 0xab, 0x73, 0xa7 },
  { 0x49, 0xe9, 0x5d, 0x6d, 0x4c, 0xa2, 0x29, 0xbf },
  { 0x01, 0x83, 0x10, 0xdc, 0x40, 0x9b, 0x26, 0xd6 },
  { 0x1c, 0x58, 0x7f, 0x1c, 0x13, 0x92, 0x4f, 0xef }
};

static const unsigned char
table4_inp[19][8] = {
  { 0x01, 0xa1, 0xd6, 0xd0, 0x39, 0x77, 0x67, 0x42 },
  { 0x5c, 0xd5, 0x4c, 0xa8, 0x3d, 0xef, 0x57, 0xda },
  { 0x02, 0x48, 0xd4, 0x38, 0x06, 0xf6, 0x71, 0x72 },
  { 0x51, 0x45, 0x4b, 0x58, 0x2d, 0xdf, 0x44, 0x0a },
  { 0x42, 0xfd, 0x44, 0x30, 0x59, 0x57, 0x7f, 0xa2 },
  { 0x05, 0x9b, 0x5e, 0x08, 0x51, 0xcf, 0x14, 0x3a },
  { 0x07, 0x56, 0xd8, 0xe0, 0x77, 0x47, 0x61, 0xd2 },
  { 0x76, 0x25, 0x14, 0xb8, 0x29, 0xbf, 0x48, 0x6a },
  { 0x3b, 0xdd, 0x11, 0x90, 0x49, 0x37, 0x28, 0x02 },
  { 0x26, 0x95, 0x5f, 0x68, 0x35, 0xaf, 0x60, 0x9a },
  { 0x16, 0x4d, 0x5e, 0x40, 0x4f, 0x27, 0x52, 0x32 },
  { 0x6b, 0x05, 0x6e, 0x18, 0x75, 0x9f, 0x5c, 0xca },
  { 0x00, 0x4b, 0xd6, 0xef, 0x09, 0x17, 0x60, 0x62 },
  { 0x48, 0x0d, 0x39, 0x00, 0x6e, 0xe7, 0x62, 0xf2 },
  { 0x43, 0x75, 0x40, 0xc8, 0x69, 0x8f, 0x3c, 0xfa },
  { 0x07, 0x2d, 0x43, 0xa0, 0x77, 0x07, 0x52, 0x92 },
  { 0x02, 0xfe, 0x55, 0x77, 0x81, 0x17, 0xf1, 0x2a },
  { 0x1d, 0x9d, 0x5c, 0x50, 0x18, 0xf7, 0x28, 0xc2 },
  { 0x30, 0x55, 0x32, 0x28, 0x6d, 0x6f, 0x29, 0x5a }
};

static const unsigned char 
des_ecb_enc_sample_key[8] = { 0x97, 0xae, 0x43, 0x08, 0xb6, 0xa8, 0x7a, 0x08 };
static const unsigned char 
des_ecb_enc_sample_inp[8] = { 0xcf, 0xcd, 0x91, 0xf1, 0xb3, 0x40, 0xc9, 0x91 };

static const unsigned char 
des_ecb_dec_sample_key[8] = { 0x0b, 0x8c, 0x38, 0xef, 0x52, 0x01, 0xda, 0x13 };
static const unsigned char 
des_ecb_dec_sample_inp[8] = { 0x58, 0x0b, 0x39, 0x57, 0x3d, 0x9b, 0x8d, 0xdf };

static const unsigned char 
des_cbc_enc_sample_key[8] = { 0x58, 0x62, 0xd3, 0xf8, 0x04, 0xe9, 0xb3, 0x98 };
static const unsigned char 
des_cbc_enc_sample_iv[8]  = { 0xac, 0xcf, 0x45, 0x4c, 0x1a, 0x28, 0x68, 0xcf };
static const unsigned char 
des_cbc_enc_sample_inp[8] = { 0xf1, 0x55, 0x47, 0x63, 0x76, 0x0e, 0x43, 0xa9 };

static const unsigned char 
des_cbc_dec_sample_key[8] = { 0x64, 0x6d, 0x02, 0x75, 0xe9, 0x34, 0xe6, 0x7a };
static const unsigned char 
des_cbc_dec_sample_iv[8]  = { 0xb4, 0x32, 0xa3, 0x8c, 0xd5, 0xe3, 0x20, 0x1a };
static const unsigned char 
des_cbc_dec_sample_inp[8] = { 0x5a, 0xfe, 0xe8, 0xf2, 0xf6, 0x63, 0x4f, 0xb6 };

static const unsigned char 
tdea_ecb_enc_sample_key[24] = { 
                            0x0b, 0x62, 0x7f, 0x67, 0xea, 0xda, 0x0b, 0x34,
                            0x08, 0x07, 0x3b, 0xc8, 0x8c, 0x23, 0x1a, 0xb6,
                            0x75, 0x0b, 0x9e, 0x57, 0x83, 0xf4, 0xe6, 0xa4 };
static const unsigned char 
tdea_ecb_enc_sample_inp[8] = { 0x44, 0x15, 0x7a, 0xb0, 0x0a, 0x78, 0x6d, 0xbf };

static const unsigned char 
tdea_ecb_dec_sample_key[24] = { 
                           0x91, 0xe5, 0x07, 0xba, 0x01, 0x01, 0xb6, 0xdc,
                           0x0e, 0x51, 0xf1, 0xd0, 0x25, 0xc2, 0xc2, 0x1c,
                           0x1f, 0x54, 0x2f, 0xa1, 0xf8, 0xce, 0xda, 0x89  };
static const unsigned char 
tdea_ecb_dec_sample_inp[8] = { 0x66, 0xe8, 0x72, 0x0d, 0x42, 0x85, 0x4b, 0xba };

static const unsigned char 
tdea_cbc_enc_sample_key[24] = { 
                           0xd5, 0xe5, 0x61, 0x61, 0xb0, 0xc4, 0xa4, 0x25,
                           0x45, 0x1a, 0x15, 0x67, 0xa4, 0x89, 0x6b, 0xc4,
                           0x3b, 0x54, 0x1a, 0x4c, 0x1a, 0xb5, 0x49, 0x0d };
static const unsigned char 
tdea_cbc_enc_sample_iv[8]  = { 0x5a, 0xb2, 0xa7, 0x3e, 0xc4, 0x3c, 0xe7, 0x1e };
static const unsigned char 
tdea_cbc_enc_sample_inp[8] = { 0x9e, 0x76, 0x87, 0x7c, 0x54, 0x14, 0xab, 0x50 };

static const unsigned char 
tdea_cbc_dec_sample_key[24] = {
                           0xf8, 0x25, 0xcd, 0x02, 0xc7, 0x76, 0xe6, 0xce,
                           0x9e, 0x16, 0xe6, 0x40, 0x7f, 0xcd, 0x01, 0x80,
                           0x5b, 0x38, 0xc4, 0xe0, 0xb5, 0x6e, 0x94, 0x61 };
static const unsigned char 
tdea_cbc_dec_sample_iv[8]  = { 0x74, 0x3e, 0xdc, 0xc2, 0xc6, 0xc4, 0x18, 0xe3 };
static const unsigned char 
tdea_cbc_dec_sample_inp[8] = { 0xbe, 0x47, 0xd1, 0x77, 0xa5, 0xe8, 0x29, 0xfb };


static const unsigned char 
des_ecb_enc_key[8] = { 0x49, 0x45, 0xd9, 0x3d, 0x83, 0xcd, 0x61, 0x9b };
static const unsigned char 
des_ecb_enc_inp[8] = { 0x81, 0xf2, 0x12, 0x0d, 0x99, 0x04, 0x5d, 0x16 };

static const unsigned char 
des_ecb_dec_key[8] = { 0x7a, 0x6b, 0x61, 0x76, 0xc8, 0x85, 0x43, 0x31 };
static const unsigned char 
des_ecb_dec_inp[8] = { 0xef, 0xe4, 0x6e, 0x4f, 0x4f, 0xc3, 0x28, 0xcc };

static const unsigned char 
des_cbc_enc_key[8] = { 0xc8, 0x5e, 0xfd, 0xa7, 0xa7, 0xc2, 0xc4, 0x0d };
static const unsigned char 
des_cbc_enc_iv[8]  = { 0x4c, 0xb9, 0xcf, 0x46, 0xff, 0x7a, 0x3d, 0xff };
static const unsigned char 
des_cbc_enc_inp[8] = { 0x80, 0x1b, 0x24, 0x9b, 0x24, 0x0e, 0xa5, 0x96 };

static const unsigned char 
des_cbc_dec_key[8] = { 0x2c, 0x3d, 0xa1, 0x67, 0x4c, 0xfb, 0x85, 0x23 };
static const unsigned char 
des_cbc_dec_iv[8]  = { 0x7a, 0x0a, 0xc2, 0x15, 0x1d, 0x22, 0x98, 0x3a };
static const unsigned char 
des_cbc_dec_inp[8] = { 0x2d, 0x5d, 0x02, 0x04, 0x98, 0x5d, 0x5e, 0x28 };

static const unsigned char 
tdea1_ecb_enc_key[24] = { 
                        0x89, 0xcd, 0xd3, 0xf1, 0x01, 0xc1, 0x1a, 0xf4,
                        0x89, 0xcd, 0xd3, 0xf1, 0x01, 0xc1, 0x1a, 0xf4,
                        0x89, 0xcd, 0xd3, 0xf1, 0x01, 0xc1, 0x1a, 0xf4 };
                        
static const unsigned char 
tdea1_ecb_enc_inp[8] = { 0xe5, 0x8c, 0x48, 0xf0, 0x91, 0x4e, 0xeb, 0x87 };

static const unsigned char 
tdea1_ecb_dec_key[24] = {
                        0xbf, 0x86, 0x94, 0xe0, 0x83, 0x46, 0x70, 0x37,
                        0xbf, 0x86, 0x94, 0xe0, 0x83, 0x46, 0x70, 0x37,
                        0xbf, 0x86, 0x94, 0xe0, 0x83, 0x46, 0x70, 0x37 };
                        
static const unsigned char 
tdea1_ecb_dec_inp[8] = { 0x35, 0x7a, 0x6c, 0x05, 0xe0, 0x8c, 0x3d, 0xb7 };

static const unsigned char 
tdea1_cbc_enc_key[24] = { 
                        0x46, 0xf1, 0x6d, 0xbf, 0xe3, 0xd5, 0xd3, 0x94,
                        0x46, 0xf1, 0x6d, 0xbf, 0xe3, 0xd5, 0xd3, 0x94,
                        0x46, 0xf1, 0x6d, 0xbf, 0xe3, 0xd5, 0xd3, 0x94 };
                        
                        
static const unsigned char 
tdea1_cbc_enc_iv[8]  = { 0xf7, 0x3e, 0x14, 0x05, 0x88, 0xeb, 0x2e, 0x96 };
static const unsigned char 
tdea1_cbc_enc_inp[8] = { 0x18, 0x1b, 0xdf, 0x18, 0x10, 0xb2, 0xe0, 0x05 };

static const unsigned char 
tdea1_cbc_dec_key[24] = {
                        0x83, 0xd0, 0x54, 0xa2, 0x92, 0xe9, 0x6e, 0x7c,
                        0x83, 0xd0, 0x54, 0xa2, 0x92, 0xe9, 0x6e, 0x7c,
                        0x83, 0xd0, 0x54, 0xa2, 0x92, 0xe9, 0x6e, 0x7c };
                        
                        
static const unsigned char 
tdea1_cbc_dec_iv[8]  = { 0xb9, 0x65, 0x4a, 0x94, 0xba, 0x6a, 0x66, 0xf9 };
static const unsigned char 
tdea1_cbc_dec_inp[8] = { 0xce, 0xb8, 0x30, 0x95, 0xac, 0x82, 0xdf, 0x9b };

static const unsigned char 
tdea2_ecb_enc_key[24] = { 
                        0x79, 0x98, 0x4a, 0xe9, 0x23, 0xad, 0x10, 0xda,
                        0x16, 0x3e, 0xb5, 0xfe, 0xcd, 0x52, 0x20, 0x01,
                        0x79, 0x98, 0x4a, 0xe9, 0x23, 0xad, 0x10, 0xda };
static const unsigned char 
tdea2_ecb_enc_inp[8] = { 0x99, 0xd2, 0xca, 0xe8, 0xa7, 0x90, 0x13, 0xc2 };

static const unsigned char 
tdea2_ecb_dec_key[24] = { 
                        0x98, 0xcd, 0x29, 0x52, 0x85, 0x91, 0x75, 0xe3,
			0xab, 0x29, 0xe3, 0x10, 0xa2, 0x10, 0x04, 0x58,
                        0x98, 0xcd, 0x29, 0x52, 0x85, 0x91, 0x75, 0xe3 };
                        
static const unsigned char 
tdea2_ecb_dec_inp[8] = { 0xc0, 0x35, 0x24, 0x1f, 0xc9, 0x29, 0x5c, 0x7a };

static const unsigned char 
tdea2_cbc_enc_key[24] = {
                        0xba, 0x5d, 0x70, 0xf8, 0x08, 0x13, 0xb0, 0x4c,
			0xf8, 0x46, 0xa8, 0xce, 0xe6, 0xb3, 0x08, 0x02,
                        0xba, 0x5d, 0x70, 0xf8, 0x08, 0x13, 0xb0, 0x4c };
                        
                        
static const unsigned char 
tdea2_cbc_enc_iv[8]  = { 0xe8, 0x39, 0xd7, 0x3a, 0x8d, 0x8c, 0x59, 0x8a };
static const unsigned char 
tdea2_cbc_enc_inp[8] = { 0x6e, 0x85, 0x0a, 0x4c, 0x86, 0x86, 0x70, 0x23 };

static const unsigned char 
tdea2_cbc_dec_key[24] = {
                        0x25, 0xf8, 0x9e, 0x7a, 0xef, 0x26, 0xb5, 0x9e,
			0x46, 0x32, 0x19, 0x9b, 0xea, 0x1c, 0x19, 0xad,
                        0x25, 0xf8, 0x9e, 0x7a, 0xef, 0x26, 0xb5, 0x9e };
                        
                        
static const unsigned char 
tdea2_cbc_dec_iv[8]  = { 0x48, 0x07, 0x6f, 0xf9, 0x05, 0x14, 0xc1, 0xdc };
static const unsigned char 
tdea2_cbc_dec_inp[8] = { 0x9e, 0xf4, 0x10, 0x55, 0xe8, 0x7e, 0x7e, 0x25 }; 

static const unsigned char 
tdea3_ecb_enc_key[24] = { 
                        0x6d, 0x37, 0x16, 0x31, 0x6e, 0x02, 0x83, 0xb6,
                        0xf7, 0x16, 0xa2, 0x64, 0x57, 0x8c, 0xae, 0x34,
                        0xd0, 0xce, 0x38, 0xb6, 0x31, 0x5e, 0xae, 0x1a };
static const unsigned char 
tdea3_ecb_enc_inp[8] = { 0x28, 0x8a, 0x45, 0x22, 0x53, 0x95, 0xba, 0x3c };

static const unsigned char 
tdea3_ecb_dec_key[24] = {
                        0xb0, 0x75, 0x92, 0x2c, 0xfd, 0x67, 0x8a, 0x26,
                        0xc8, 0xba, 0xad, 0x68, 0xb6, 0xba, 0x92, 0x49,
                        0xe3, 0x2c, 0xec, 0x83, 0x34, 0xe6, 0xda, 0x98 };
static const unsigned char 
tdea3_ecb_dec_inp[8] = { 0x03, 0xcc, 0xe6, 0x65, 0xf6, 0xc5, 0xc3, 0xba };

static const unsigned char 
tdea3_cbc_enc_key[24] = { 
                        0x01, 0x32, 0x73, 0xe9, 0xcb, 0x8a, 0x89, 0x80,
                        0x02, 0x7a, 0xc1, 0x5d, 0xf4, 0xd5, 0x6b, 0x76,
                        0x2f, 0xef, 0xfd, 0x58, 0x57, 0x1a, 0xce, 0x29 };
static const unsigned char 
tdea3_cbc_enc_iv[8]  = { 0x93, 0x98, 0x7c, 0x66, 0x98, 0x21, 0x5b, 0x9e };
static const unsigned char 
tdea3_cbc_enc_inp[8] = { 0x16, 0x54, 0x09, 0xd2, 0x2c, 0xad, 0x6d, 0x99 };

static const unsigned char 
tdea3_cbc_dec_key[24] = {
                        0x57, 0x70, 0x3b, 0x4f, 0xae, 0xe6, 0x9d, 0x0e,
                        0x4c, 0x3b, 0x23, 0xcd, 0x54, 0x20, 0xbc, 0x58,
                        0x3b, 0x8a, 0x4a, 0xf1, 0x73, 0xf8, 0xf8, 0x38 };
static const unsigned char 
tdea3_cbc_dec_iv[8]  = { 0x5f, 0x62, 0xe4, 0xea, 0xa7, 0xb2, 0xb5, 0x70 };
static const unsigned char 
tdea3_cbc_dec_inp[8] = { 0x44, 0xb3, 0xe6, 0x3b, 0x1f, 0xbb, 0x43, 0x02 };

SECStatus
hex_from_2char(unsigned char *c2, unsigned char *byteval)
{
    int i;
    unsigned char offset;
    *byteval = 0;
    for (i=0; i<2; i++) {
	if (c2[i] >= '0' && c2[i] <= '9') {
	    offset = c2[i] - '0';
	    *byteval |= offset << 4*(1-i);
	} else if (c2[i] >= 'a' && c2[i] <= 'f') {
	    offset = c2[i] - 'a';
	    *byteval |= (offset + 10) << 4*(1-i);
	} else if (c2[i] >= 'A' && c2[i] <= 'F') {
	    offset = c2[i] - 'A';
	    *byteval |= (offset + 10) << 4*(1-i);
	} else {
	    return SECFailure;
	}
    }
    return SECSuccess;
}

SECStatus
char2_from_hex(unsigned char byteval, unsigned char *c2, char a)
{
    int i;
    unsigned char offset;
    for (i=0; i<2; i++) {
	offset = (byteval >> 4*(1-i)) & 0x0f;
	if (offset < 10) {
	    c2[i] = '0' + offset;
	} else {
	    c2[i] = a + offset - 10;
	}
    }
    return SECSuccess;
}

void
to_hex_str(char *str, const unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
	char2_from_hex(buf[i], &str[2*i], 'a');
    }
    str[2*len] = '\0';
}

void
to_hex_str_cap(char *str, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
	char2_from_hex(buf[i], &str[2*i], 'A');
    }
}

void
des_var_pt_kat(int mode, PRBool encrypt, unsigned int len,
               unsigned char *key, unsigned char *iv,
               unsigned char *inp)
{
    int i;
    unsigned int olen, mbnum = 0;
    unsigned char mod_byte = 0x80;
    unsigned char in[8];
    unsigned char out[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL, *cx2 = NULL;
    memset(in, 0, sizeof in);
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    in[mbnum] = mod_byte;
    for (i=1; i<=64; i++) {
	cx1 = DES_CreateContext(key, iv, mode, PR_TRUE);
	if (!encrypt) {
	    cx2 = DES_CreateContext(key, iv, mode, PR_FALSE);
	}
	if (len > 8) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, key, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, key+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, key+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, key, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, iv, 8);
	    printf("IV=%s%c", ivstr, tchar);
	}
	DES_Encrypt(cx1, out, &olen, 8, in, 8);
	if (encrypt) {
	    to_hex_str(instr, in, 8);
	    to_hex_str(outstr, out, 8);
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    unsigned char inv[8];
	    DES_Decrypt(cx2, inv, &olen, 8, out, 8);
	    to_hex_str(instr, out, 8);
	    to_hex_str(outstr, inv, 8);
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	if (mod_byte > 0x01) {
	    mod_byte >>= 1;
	} else {
	    in[mbnum] = 0x00;
	    mod_byte = 0x80;
	    mbnum++;
	}
	in[mbnum] = mod_byte;
	DES_DestroyContext(cx1, PR_TRUE);
	if (cx2) {
	    DES_DestroyContext(cx2, PR_TRUE);
	}
    }
}

void
des_inv_perm_kat(int mode, PRBool encrypt, unsigned int len,
                 unsigned char *key, unsigned char *iv,
                 unsigned char *inp)
{
    int i;
    unsigned int olen, mbnum = 0;
    unsigned char mod_byte = 0x80;
    unsigned char in[8];
    unsigned char out[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL, *cx2 = NULL;
    memset(in, 0, sizeof in);
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    in[mbnum] = mod_byte;
    for (i=1; i<=64; i++) {
	if (encrypt) {
	    cx1 = DES_CreateContext(key, iv, mode, PR_TRUE);
	    cx2 = DES_CreateContext(key, iv, mode, PR_TRUE);
	} else {
	    cx1 = DES_CreateContext(key, iv, mode, PR_FALSE);
	}
	if (len > 8) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, key, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, key+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, key+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, key, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, iv, 8);
	    printf("IV=%s%c", ivstr, tchar);
	}
	if (encrypt) {
	    unsigned char inv[8];
	    DES_Encrypt(cx1, out, &olen, 8, in, 8);
	    DES_Encrypt(cx2, inv, &olen, 8, out, 8);
	    to_hex_str(instr, out, 8);
	    to_hex_str(outstr, inv, 8);
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    DES_Decrypt(cx1, out, &olen, 8, in, 8);
	    to_hex_str(instr, in, 8);
	    to_hex_str(outstr, out, 8);
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	if (mod_byte > 0x01) {
	    mod_byte >>= 1;
	} else {
	    in[mbnum] = 0x00;
	    mod_byte = 0x80;
	    mbnum++;
	}
	in[mbnum] = mod_byte;
	DES_DestroyContext(cx1, PR_TRUE);
	if (cx2) {
	    DES_DestroyContext(cx2, PR_TRUE);
	}
    }
}

void
des_var_key_kat(int mode, PRBool encrypt, unsigned int len,
                unsigned char *key, unsigned char *iv,
                unsigned char *inp)
{
    int i;
    unsigned int olen, mbnum = 0;
    unsigned char mod_byte = 0x80;
    unsigned char keyin[24];
    unsigned char out[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL, *cx2 = NULL;
    memset(keyin, 1, sizeof keyin);
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    keyin[mbnum] = mod_byte;
    keyin[mbnum+8] = mod_byte;
    keyin[mbnum+16] = mod_byte;
    for (i=1; i<=56; i++) {
	cx1 = DES_CreateContext(keyin, iv, mode, PR_TRUE);
	if (!encrypt) {
	    cx2 = DES_CreateContext(keyin, iv, mode, PR_FALSE);
	}
	if (len > 8) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, keyin, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, keyin+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, keyin+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, keyin, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, iv, 8);
	    printf("IV=%s%c", ivstr, tchar);
	}
	DES_Encrypt(cx1, out, &olen, 8, inp, 8);
	if (encrypt) {
	    to_hex_str(instr, inp, 8);
	    to_hex_str(outstr, out, 8);
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    unsigned char inv[8];
	    DES_Decrypt(cx2, inv, &olen, 8, out, 8);
	    to_hex_str(instr, out, 8);
	    to_hex_str(outstr, inv, 8);
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	if (mod_byte > 0x02) {
	    mod_byte >>= 1;
	} else {
	    keyin[mbnum] = 0x01;
	    keyin[mbnum+8] = 0x01;
	    keyin[mbnum+16] = 0x01;
	    mod_byte = 0x80;
	    mbnum++;
	}
	keyin[mbnum] = mod_byte;
	keyin[mbnum+8] = mod_byte;
	keyin[mbnum+16] = mod_byte;
	DES_DestroyContext(cx1, PR_TRUE);
	if (cx2) {
	    DES_DestroyContext(cx2, PR_TRUE);
	}
    }
}

void
des_perm_op_kat(int mode, PRBool encrypt, unsigned int len,
                unsigned char *key, unsigned char *iv,
                unsigned char *inp)
{
    int i;
    unsigned int olen;
    unsigned char keyin[24];
    unsigned char out[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL, *cx2 = NULL;
    memset(keyin, 0, sizeof keyin);
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    for (i=0; i<32; i++) {
	memcpy(keyin, table3[i], 8);
	memcpy(keyin+8, table3[i], 8);
	memcpy(keyin+16, table3[i], 8);
	cx1 = DES_CreateContext(keyin, iv, mode, PR_TRUE);
	if (!encrypt) {
	    cx2 = DES_CreateContext(keyin, iv, mode, PR_FALSE);
	}
	if (len > 8) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, keyin, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, keyin+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, keyin+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, keyin, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, iv, 8);
	    printf("IV=%s%c", ivstr, tchar);
	}
	DES_Encrypt(cx1, out, &olen, 8, inp, 8);
	if (encrypt) {
	    to_hex_str(instr, inp, 8);
	    to_hex_str(outstr, out, 8);
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    unsigned char inv[8];
	    DES_Decrypt(cx2, inv, &olen, 8, out, 8);
	    to_hex_str(instr, out, 8);
	    to_hex_str(outstr, inv, 8);
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	DES_DestroyContext(cx1, PR_TRUE);
	if (cx2) {
	    DES_DestroyContext(cx2, PR_TRUE);
	}
    }
}

void
des_sub_tbl_kat(int mode, PRBool encrypt, unsigned int len,
                unsigned char *key, unsigned char *iv,
                unsigned char *inp)
{
    int i;
    unsigned int olen;
    unsigned char keyin[24];
    unsigned char out[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL, *cx2 = NULL;
    memset(keyin, 0, sizeof keyin);
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    for (i=0; i<19; i++) {
	memcpy(keyin, table4_key[i], 8);
	memcpy(keyin+8, table4_key[i], 8);
	memcpy(keyin+16, table4_key[i], 8);
	cx1 = DES_CreateContext(keyin, iv, mode, PR_TRUE);
	if (!encrypt) {
	    cx2 = DES_CreateContext(keyin, iv, mode, PR_FALSE);
	}
	if (len > 8) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, keyin, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, keyin+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, keyin+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, keyin, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, iv, 8);
	    printf("IV=%s%c", ivstr, tchar);
	}
	DES_Encrypt(cx1, out, &olen, 8, table4_inp[i], 8);
	if (encrypt) {
	    to_hex_str(instr, table4_inp[i], 8);
	    to_hex_str(outstr, out, 8);
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    unsigned char inv[8];
	    DES_Decrypt(cx2, inv, &olen, 8, out, 8);
	    to_hex_str(instr, out, 8);
	    to_hex_str(outstr, inv, 8);
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	DES_DestroyContext(cx1, PR_TRUE);
	if (cx2) {
	    DES_DestroyContext(cx2, PR_TRUE);
	}
    }
}

unsigned char make_odd_parity(unsigned char b)
{
    int i;
    int sum = 0;
    for (i=1; i<8; i++) {
	sum += (b & (1 << i)) ? 1 : 0;
    }
    if (sum & 0x01) {
	return (b & 0xfe);
    } else {
	return (b | 0x01);
    }
}

void
des_modes(int mode, PRBool encrypt, unsigned int len,
          const unsigned char *key, const unsigned char *iv,
          const unsigned char *inp, int keymode)
{
    int i, j;
    unsigned int olen;
    unsigned char keyin[24];
    unsigned char in[8];
    unsigned char cv[8];
    unsigned char cv0[8];
    unsigned char in0[8];
    unsigned char out[8];
    unsigned char cj9998[8], cj9997[8];
    char keystr[17], ivstr[17], instr[17], outstr[17];
    char *ptty = (len == 8) ? "PT" : "PLAINTEXT";
    char *ctty = (len == 8) ? "CT" : "CIPHERTEXT";
    char tchar = (len == 8) ? '\t' : '\n';
    DESContext *cx1 = NULL;
    memset(keystr, 0, sizeof keystr);
    memset(ivstr, 0, sizeof ivstr);
    memset(instr, 0, sizeof instr);
    memset(outstr, 0, sizeof outstr);
    memcpy(in, inp, 8);
    if (iv) memcpy(cv, iv, 8);
    memcpy(keyin, key, len);
    for (i=0; i<400; i++) {
	if (iv) memcpy(cv0, cv, 8);
	memcpy(in0, in, 8);
	for (j=0; j<10000; j++) {
	    if (encrypt) {
		cx1 = DES_CreateContext(keyin, cv, mode, PR_TRUE);
		DES_Encrypt(cx1, out, &olen, 8, in, 8);
	    } else {
		cx1 = DES_CreateContext(keyin, cv, mode, PR_FALSE);
		DES_Decrypt(cx1, out, &olen, 8, in, 8);
	    }
	    if (j==9997) memcpy(cj9997, out, 8);
	    if (j==9998) memcpy(cj9998, out, 8);
	    if (iv) {
		if (encrypt) {
		    memcpy(in, cv, 8);
		    memcpy(cv, out, 8);
		} else {
		    memcpy(cv, in, 8);
		    memcpy(in, out, 8);
		}
	    } else {
		memcpy(in, out, 8);
	    }
	    DES_DestroyContext(cx1, PR_TRUE);
	}
	if (keymode > 0) {
	    printf("COUNT = %d\n", i);
	    to_hex_str(keystr, keyin, 8);
	    printf("KEY1=%s\n", keystr);
	    to_hex_str(keystr, keyin+8, 8);
	    printf("KEY2=%s\n", keystr);
	    to_hex_str(keystr, keyin+16, 8);
	    printf("KEY3=%s\n", keystr);
	} else {
	    to_hex_str(keystr, keyin, 8);
	    printf("%ld\tKEY=%s\t", i, keystr);
	}
	if (iv) {
	    to_hex_str(ivstr, cv0, 8);
	    printf("CV=%s%c", ivstr, tchar);
	}
	to_hex_str(instr, in0, 8);
	to_hex_str(outstr, out, 8);
	if (encrypt) {
	    printf("%s=%s%c%s=%s\n\n", ptty, instr, tchar, ctty, outstr);
	} else {
	    printf("%s=%s%c%s=%s\n\n", ctty, instr, tchar, ptty, outstr);
	}
	for (j=0; j<8; j++) {
	    keyin[j] ^= out[j];
	    keyin[j] = make_odd_parity(keyin[j]);
	    if (keymode == 0) continue;
	    if (keymode > 1) {
		keyin[j+8] ^= cj9998[j];
		keyin[j+8] = make_odd_parity(keyin[j+8]);
	    } else {
		keyin[j+8] = keyin[j];
	    }
	    if (keymode > 2) {
		keyin[j+16] ^= cj9997[j];
		keyin[j+16] = make_odd_parity(keyin[j+16]);
	    } else {
		keyin[j+16] = keyin[j];
	    }
	}
    }
}

SECStatus
tdea_encrypt_buf(
    int mode,
    const unsigned char *key, 
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    DESContext *cx;
    unsigned char doublecheck[8*20];  /* 1 to 20 blocks */
    unsigned int doublechecklen = 0;

    cx = DES_CreateContext(key, iv, mode, PR_TRUE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Encrypt(cx, output, outputlen, maxoutputlen, input, inputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (*outputlen != inputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    /*
     * Doublecheck our result by decrypting the ciphertext and
     * compare the output with the input plaintext.
     */
    cx = DES_CreateContext(key, iv, mode, PR_FALSE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Decrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
                    output, *outputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (doublechecklen != *outputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
        goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
        DES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
tdea_decrypt_buf(
    int mode,
    const unsigned char *key, 
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    DESContext *cx;
    unsigned char doublecheck[8*20];  /* 1 to 20 blocks */
    unsigned int doublechecklen = 0;

    cx = DES_CreateContext(key, iv, mode, PR_FALSE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Decrypt(cx, output, outputlen, maxoutputlen,
                    input, inputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (*outputlen != inputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    /*
     * Doublecheck our result by encrypting the plaintext and
     * compare the output with the input ciphertext.
     */
    cx = DES_CreateContext(key, iv, mode, PR_TRUE);
    if (cx == NULL) {
        goto loser;
    }
    rv = DES_Encrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
        output, *outputlen);
    if (rv != SECSuccess) {
        goto loser;
    }
    if (doublechecklen != *outputlen) {
        goto loser;
    }
    DES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
        goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
        DES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

/*
 * Perform the TDEA Known Answer Test (KAT) or Multi-block Message
 * Test (MMT) in ECB or CBC mode.  The KAT (there are five types)
 * and MMT have the same structure: given the key and IV (CBC mode
 * only), encrypt the given plaintext or decrypt the given ciphertext.
 * So we can handle them the same way.
 *
 * reqfn is the pathname of the REQUEST file.
 *
 * The output RESPONSE file is written to stdout.
 */
void
tdea_kat_mmt(char *reqfn)
{
    char buf[180];      /* holds one line from the input REQUEST file.
                         * needs to be large enough to hold the longest
                         * line "CIPHERTEXT = <180 hex digits>\n".
                         */
    FILE *req;       /* input stream from the REQUEST file */
    FILE *resp;      /* output stream to the RESPONSE file */
    int i, j;
    int mode;           /* NSS_DES_EDE3 (ECB) or NSS_DES_EDE3_CBC */
    int crypt = DECRYPT;    /* 1 means encrypt, 0 means decrypt */
    unsigned char key[24];              /* TDEA 3 key bundle */
    unsigned int numKeys = 0;
    unsigned char iv[8];		/* for all modes except ECB */
    unsigned char plaintext[8*20];     /* 1 to 20 blocks */
    unsigned int plaintextlen;
    unsigned char ciphertext[8*20];   /* 1 to 20 blocks */  
    unsigned int ciphertextlen;
    SECStatus rv;

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, sizeof buf, req) != NULL) {
        /* a comment or blank line */
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        /* [ENCRYPT] or [DECRYPT] */
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
                crypt = ENCRYPT;
            } else {
                crypt = DECRYPT;
            }
            fputs(buf, resp);
            continue;
        }
        /* NumKeys */
        if (strncmp(&buf[0], "NumKeys", 7) == 0) {
            i = 7;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            numKeys = buf[i];
            fputs(buf, resp);
            continue;
        }
        /* "COUNT = x" begins a new data set */
        if (strncmp(buf, "COUNT", 5) == 0) {
            /* mode defaults to ECB, if dataset has IV mode will be set CBC */
            mode = NSS_DES_EDE3;
            /* zeroize the variables for the test with this data set */
            memset(key, 0, sizeof key);
            memset(iv, 0, sizeof iv);
            memset(plaintext, 0, sizeof plaintext);
            plaintextlen = 0;
            memset(ciphertext, 0, sizeof ciphertext);
            ciphertextlen = 0;
            fputs(buf, resp);
            continue;
        }
        if (numKeys == 0) {
            if (strncmp(buf, "KEYs", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=0; isxdigit(buf[i]); i+=2,j++) {
                    hex_from_2char(&buf[i], &key[j]);
                    key[j+8] = key[j];
                    key[j+16] = key[j];
                }
                fputs(buf, resp);
                continue;
            }
        } else {
            /* KEY1 = ... */
            if (strncmp(buf, "KEY1", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=0; isxdigit(buf[i]); i+=2,j++) {
                    hex_from_2char(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
            /* KEY2 = ... */
            if (strncmp(buf, "KEY2", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=8; isxdigit(buf[i]); i+=2,j++) {
                    hex_from_2char(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
            /* KEY3 = ... */
            if (strncmp(buf, "KEY3", 4) == 0) {
                i = 4;
                while (isspace(buf[i]) || buf[i] == '=') {
                    i++;
                }
                for (j=16; isxdigit(buf[i]); i+=2,j++) {
                    hex_from_2char(&buf[i], &key[j]);
                }
                fputs(buf, resp);
                continue;
            }
        }

        /* IV = ... */
        if (strncmp(buf, "IV", 2) == 0) {
            mode = NSS_DES_EDE3_CBC;
            i = 2;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof iv; i+=2,j++) {
                hex_from_2char(&buf[i], &iv[j]);
            }
            fputs(buf, resp);
            continue;
        }

        /* PLAINTEXT = ... */
        if (strncmp(buf, "PLAINTEXT", 9) == 0) {
            /* sanity check */
            if (crypt != ENCRYPT) {
                goto loser;
            }
            i = 9;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &plaintext[j]);
            }
            plaintextlen = j;
            rv = tdea_encrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            ciphertext, &ciphertextlen, sizeof ciphertext,
                            plaintext, plaintextlen);
            if (rv != SECSuccess) {
                goto loser;
            }
    
            fputs(buf, resp);
            fputs("CIPHERTEXT = ", resp);
            to_hex_str(buf, ciphertext, ciphertextlen);
            fputs(buf, resp);
            fputc('\n', resp);
            continue;
        }
        /* CIPHERTEXT = ... */
        if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
            /* sanity check */
            if (crypt != DECRYPT) {
                goto loser;
            }
 
            i = 10;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &ciphertext[j]);
            }
            ciphertextlen = j;
 
            rv = tdea_decrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            plaintext, &plaintextlen, sizeof plaintext,
                            ciphertext, ciphertextlen);
            if (rv != SECSuccess) {
                goto loser;
            }
 
            fputs(buf, resp);
            fputs("PLAINTEXT = ", resp);
            to_hex_str(buf, plaintext, plaintextlen);
            fputs(buf, resp);
            fputc('\n', resp);
            continue;
        }
    }

loser:
    fclose(req);
}

/*
 * Generate Keys [i+1] from Key[i], PT/CT[j-2], PT/CT[j-1], and PT/CT[j] 
 * for TDEA Monte Carlo Test (MCT) in ECB and CBC modes.
 */
void
tdea_mct_next_keys(unsigned char *key,
    const unsigned char *text_2, const unsigned char *text_1, 
    const unsigned char *text, unsigned int numKeys)
{
    int k;

    /* key1[i+1] = key1[i] xor PT/CT[j] */
    for (k=0; k<8; k++) {
        key[k] ^= text[k];
    }
    /* key2 */
    if (numKeys == 2 || numKeys == 3)  {
        /* key2 independent */
        for (k=8; k<16; k++) {
            /* key2[i+1] = KEY2[i] xor PT/CT[j-1] */
            key[k] ^= text_1[k-8];
        }
    } else {
        /* key2 == key 1 */
        for (k=8; k<16; k++) {
            /* key2[i+1] = KEY2[i] xor PT/CT[j] */
            key[k] = key[k-8];
        }
    }
    /* key3 */
    if (numKeys == 1 || numKeys == 2) {
        /* key3 == key 1 */
        for (k=16; k<24; k++) {
            /* key3[i+1] = KEY3[i] xor PT/CT[j] */
            key[k] = key[k-16];
        }
    } else {
        /* key3 independent */ 
        for (k=16; k<24; k++) {
            /* key3[i+1] = KEY3[i] xor PT/CT[j-2] */
            key[k] ^= text_2[k-16];
        }
    }
}

/*
 * Perform the Monte Carlo Test
 *
 * mode = NSS_DES_EDE3 or NSS_DES_EDE3_CBC
 * crypt = ENCRYPT || DECRYPT
 * inputtext = plaintext or Cyphertext depending on the value of crypt
 * inputlength is expected to be size 8 bytes 
 * iv = needs to be set for NSS_DES_EDE3_CBC mode
 * resp = is the output response file. 
 */
 void                                                       
tdea_mct_test(int mode, unsigned char* key, unsigned int numKeys, 
              unsigned int crypt, unsigned char* inputtext, 
              unsigned int inputlength, unsigned char* iv, FILE *resp) { 

    int i, j, k;
    unsigned char outputtext_1[8];      /* PT/CT[j-1] */
    unsigned char outputtext_2[8];      /* PT/CT[j-2] */
    char buf[80];       /* holds one line from the input REQUEST file. */
    unsigned char tempKey[8];
    unsigned int outputlen;
    unsigned char outputtext[8];
    
        
    SECStatus rv;

    if (mode == NSS_DES_EDE3 && iv != NULL) {
        printf("IV must be NULL for NSS_DES_EDE3 mode");
        goto loser;
    } else if (mode == NSS_DES_EDE3_CBC && iv == NULL) {
        printf("IV must not be NULL for NSS_DES_EDE3_CBC mode");
        goto loser;
    }

    /* loop 400 times */
    for (i=0; i<400; i++) {
        /* if i == 0 CV[0] = IV  not necessary */        
        /* record the count and key values and plainText */
        sprintf(buf, "COUNT = %d\n", i);
        fputs(buf, resp);
        /* Output KEY1[i] */
        fputs("KEY1 = ", resp);
        for (j=0; j < 8; j++) tempKey[j] = key[j];
        to_hex_str(buf, tempKey, 8);
        fputs(buf, resp);
        fputc('\n', resp);
        /* Output KEY2[i] */
        fputs("KEY2 = ", resp);
        for (j=0; j < 8; j++) tempKey[j] = key[j+8];
        to_hex_str(buf, tempKey, 8);
        fputs(buf, resp);
        fputc('\n', resp);
        /* Output KEY3[i] */
        fputs("KEY3 = ", resp);
        for (j=0; j < 8; j++) tempKey[j] = key[j+16];
        to_hex_str(buf, tempKey, 8);
        fputs(buf, resp);
        fputc('\n', resp);
        if (mode == NSS_DES_EDE3_CBC) {
            /* Output CV[i] */
            fputs("IV = ", resp);
            to_hex_str(buf, iv, 8);
            fputs(buf, resp);
            fputc('\n', resp);
        }
        if (crypt == ENCRYPT) {
            /* Output PT[0] */
            fputs("PLAINTEXT = ", resp);
        } else {
            /* Output CT[0] */
            fputs("CIPHERTEXT = ", resp);
        }

        to_hex_str(buf, inputtext, inputlength);
        fputs(buf, resp);
        fputc('\n', resp);

        /* loop 10,000 times */
        for (j=0; j<10000; j++) {

            outputlen = 0;
            if (crypt == ENCRYPT) {
                /* inputtext == ciphertext outputtext == plaintext*/
                rv = tdea_encrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            outputtext, &outputlen, 8,
                            inputtext, 8);
            } else {
                /* inputtext == plaintext outputtext == ciphertext */
                rv = tdea_decrypt_buf(mode, key,
                            (mode == NSS_DES_EDE3) ? NULL : iv,
                            outputtext, &outputlen, 8,
                            inputtext, 8);
            }

            if (rv != SECSuccess) {
                goto loser;
            }
            if (outputlen != inputlength) {
                goto loser;
            }

            if (mode == NSS_DES_EDE3_CBC) {
                if (crypt == ENCRYPT) {
                    if (j == 0) {
                        /*P[j+1] = CV[0] */
                        memcpy(inputtext, iv, 8);
                    } else {
                        /* p[j+1] = C[j-1] */
                        memcpy(inputtext, outputtext_1, 8);
                        /* save C[j-1] */
                        memcpy(outputtext_1, outputtext, 8);
                    }
                    /* CV[j+1] = C[j] */
                    memcpy(iv, outputtext, 8);
                } else { /* DECRYPT */
                    /* CV[j+1] = C[j] */
                    memcpy(iv, inputtext, 8);
                    /*C[j+1] = P[j] */
                    memcpy(inputtext, outputtext, 8);
                }
            } else {
                /* ECB mode PT/CT[j+1] = CT/PT[j] */
                memcpy(inputtext, outputtext, 8);
            }

            /* Save PT/CT[j-2] and PT/CT[j-1] */
            if (j==9997) memcpy(outputtext_2, outputtext_1, 8);
            if (j==9998) memcpy(outputtext_1, outputtext, 8);
            /* done at the end of the for(j) loop */
        }


        if (crypt == ENCRYPT) {
            /* Output CT[j] */
            fputs("CIPHERTEXT = ", resp);
        } else {
            /* Output PT[j] */
            fputs("PLAINTEXT = ", resp);
        }
        to_hex_str(buf, outputtext, 8);
        fputs(buf, resp);
        fputc('\n', resp);

        /* Key[i+1] = Key[i] xor ...  outputtext_2 == PT/CT[j-2] 
         *  outputtext_1 == PT/CT[j-1] outputtext == PT/CT[j] 
         */
        tdea_mct_next_keys(key, outputtext_2, 
                           outputtext_1, outputtext, numKeys);

        if (mode == NSS_DES_EDE3_CBC) {
            if (crypt == ENCRYPT) {
                /* P[i] = C[j-1] */
                memcpy(inputtext, outputtext_1, 8);
                /*CV[i] = C[j] */
                memcpy(iv, outputtext, 8);
            } else {
                /*CV[i] = C[j] */
                memcpy(iv, inputtext, 8);
                /*C[i] = P[j]  */
                memcpy(inputtext, outputtext, 8);
            }
        } else {
            /* ECB PT/CT[i] = PT/CT[j]  */
            memcpy(inputtext, outputtext, 8);
        }
        /* done at the end of the for(i) loop */
        fputc('\n', resp);
    }

loser:
    return;
}

/*
 * Perform the TDEA Monte Carlo Test (MCT) in ECB/CBC modes.
 * by gathering the input from the request file, and then 
 * calling tdea_mct_test.
 *
 * reqfn is the pathname of the input REQUEST file.
 *
 * The output RESPONSE file is written to stdout.
 */
void
tdea_mct(int mode, char *reqfn)
{
    int i, j;
    char buf[80];    /* holds one line from the input REQUEST file. */
    FILE *req;       /* input stream from the REQUEST file */
    FILE *resp;      /* output stream to the RESPONSE file */
    unsigned int crypt = 0;    /* 1 means encrypt, 0 means decrypt */
    unsigned char key[24];              /* TDEA 3 key bundle */
    unsigned int numKeys = 0;
    unsigned char plaintext[8];        /* PT[j] */
    unsigned char ciphertext[8];       /* CT[j] */
    unsigned char iv[8];

    /* zeroize the variables for the test with this data set */
    memset(key, 0, sizeof key);
    memset(plaintext, 0, sizeof plaintext);
    memset(ciphertext, 0, sizeof ciphertext);
    memset(iv, 0, sizeof iv);

    req = fopen(reqfn, "r");
    resp = stdout;
    while (fgets(buf, sizeof buf, req) != NULL) {
        /* a comment or blank line */
        if (buf[0] == '#' || buf[0] == '\n') {
            fputs(buf, resp);
            continue;
        }
        /* [ENCRYPT] or [DECRYPT] */
        if (buf[0] == '[') {
            if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
                crypt = ENCRYPT;
            } else {
                crypt = DECRYPT;
           }
           fputs(buf, resp);
           continue;
        }
        /* NumKeys */
        if (strncmp(&buf[0], "NumKeys", 7) == 0) {
            i = 7;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            numKeys = atoi(&buf[i]);
            fputs(buf, resp);
            continue;
        }
        /* KEY1 = ... */
        if (strncmp(buf, "KEY1", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &key[j]);
            }
            fputs(buf, resp);
            continue;
        }
        /* KEY2 = ... */
        if (strncmp(buf, "KEY2", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=8; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &key[j]);
            }
            fputs(buf, resp);
            continue;
        }
        /* KEY3 = ... */
        if (strncmp(buf, "KEY3", 4) == 0) {
            i = 4;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=16; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &key[j]);
            }
            fputs(buf, resp);
            continue;
        }

        /* IV = ... */
        if (strncmp(buf, "IV", 2) == 0) {
            i = 2;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof iv; i+=2,j++) {
                hex_from_2char(&buf[i], &iv[j]);
            }
           fputs(buf, resp);
           continue;
       }

       /* PLAINTEXT = ... */
       if (strncmp(buf, "PLAINTEXT", 9) == 0) {

            /* sanity check */
            if (crypt != ENCRYPT) {
                goto loser;
            }
            /* PT[0] = PT */
            i = 9;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; j<sizeof plaintext; i+=2,j++) {
                hex_from_2char(&buf[i], &plaintext[j]);
            }                                     

            fputs(buf, resp);
            fputc('\n', resp);

            /* do the Monte Carlo test */
            if (mode==NSS_DES_EDE3) {
                tdea_mct_test(NSS_DES_EDE3, key, numKeys, crypt, plaintext, sizeof plaintext, NULL, resp);
            } else {
                tdea_mct_test(NSS_DES_EDE3_CBC, key, numKeys, crypt, plaintext, sizeof plaintext, iv, resp);
            }
            continue;
        }
        /* CIPHERTEXT = ... */
        if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
            /* sanity check */
            if (crypt != DECRYPT) {
                goto loser;
            }
            /* CT[0] = CT */
            i = 10;
            while (isspace(buf[i]) || buf[i] == '=') {
                i++;
            }
            for (j=0; isxdigit(buf[i]); i+=2,j++) {
                hex_from_2char(&buf[i], &ciphertext[j]);
            }
            
            fputs(buf, resp);
            fputc('\n', resp);

            /* do the Monte Carlo test */
            if (mode==NSS_DES_EDE3) {
                tdea_mct_test(NSS_DES_EDE3, key, numKeys, crypt, ciphertext, sizeof ciphertext, NULL, resp); 
            } else {
                tdea_mct_test(NSS_DES_EDE3_CBC, key, numKeys, crypt, ciphertext, sizeof ciphertext, iv, resp); 
            }
            continue;
        }
    }

loser:
    fclose(req);
}


SECStatus
aes_encrypt_buf(
    int mode,
    const unsigned char *key, unsigned int keysize,
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    AESContext *cx;
    unsigned char doublecheck[10*16];  /* 1 to 10 blocks */
    unsigned int doublechecklen = 0;

    cx = AES_CreateContext(key, iv, mode, PR_TRUE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Encrypt(cx, output, outputlen, maxoutputlen, input, inputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (*outputlen != inputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    /*
     * Doublecheck our result by decrypting the ciphertext and
     * compare the output with the input plaintext.
     */
    cx = AES_CreateContext(key, iv, mode, PR_FALSE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Decrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
	output, *outputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (doublechecklen != *outputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
	goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
aes_decrypt_buf(
    int mode,
    const unsigned char *key, unsigned int keysize,
    const unsigned char *iv,
    unsigned char *output, unsigned int *outputlen, unsigned int maxoutputlen,
    const unsigned char *input, unsigned int inputlen)
{
    SECStatus rv = SECFailure;
    AESContext *cx;
    unsigned char doublecheck[10*16];  /* 1 to 10 blocks */
    unsigned int doublechecklen = 0;

    cx = AES_CreateContext(key, iv, mode, PR_FALSE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Decrypt(cx, output, outputlen, maxoutputlen,
	input, inputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (*outputlen != inputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;

    /*
     * Doublecheck our result by encrypting the plaintext and
     * compare the output with the input ciphertext.
     */
    cx = AES_CreateContext(key, iv, mode, PR_TRUE, keysize, 16);
    if (cx == NULL) {
	goto loser;
    }
    rv = AES_Encrypt(cx, doublecheck, &doublechecklen, sizeof doublecheck,
	output, *outputlen);
    if (rv != SECSuccess) {
	goto loser;
    }
    if (doublechecklen != *outputlen) {
	goto loser;
    }
    AES_DestroyContext(cx, PR_TRUE);
    cx = NULL;
    if (memcmp(doublecheck, input, inputlen) != 0) {
	goto loser;
    }
    rv = SECSuccess;

loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

/*
 * Perform the AES Known Answer Test (KAT) or Multi-block Message
 * Test (MMT) in ECB or CBC mode.  The KAT (there are four types)
 * and MMT have the same structure: given the key and IV (CBC mode
 * only), encrypt the given plaintext or decrypt the given ciphertext.
 * So we can handle them the same way.
 *
 * reqfn is the pathname of the REQUEST file.
 *
 * The output RESPONSE file is written to stdout.
 */
void
aes_kat_mmt(char *reqfn)
{
    char buf[512];      /* holds one line from the input REQUEST file.
                         * needs to be large enough to hold the longest
                         * line "CIPHERTEXT = <320 hex digits>\n".
                         */
    FILE *aesreq;       /* input stream from the REQUEST file */
    FILE *aesresp;      /* output stream to the RESPONSE file */
    int i, j;
    int mode;           /* NSS_AES (ECB) or NSS_AES_CBC */
    int encrypt = 0;    /* 1 means encrypt, 0 means decrypt */
    unsigned char key[32];              /* 128, 192, or 256 bits */
    unsigned int keysize;
    unsigned char iv[16];		/* for all modes except ECB */
    unsigned char plaintext[10*16];     /* 1 to 10 blocks */
    unsigned int plaintextlen;
    unsigned char ciphertext[10*16];    /* 1 to 10 blocks */
    unsigned int ciphertextlen;
    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	/* a comment or blank line */
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	/* [ENCRYPT] or [DECRYPT] */
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	/* "COUNT = x" begins a new data set */
	if (strncmp(buf, "COUNT", 5) == 0) {
	    mode = NSS_AES;
	    /* zeroize the variables for the test with this data set */
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(iv, 0, sizeof iv);
	    memset(plaintext, 0, sizeof plaintext);
	    plaintextlen = 0;
	    memset(ciphertext, 0, sizeof ciphertext);
	    ciphertextlen = 0;
	    fputs(buf, aesresp);
	    continue;
	}
	/* KEY = ... */
	if (strncmp(buf, "KEY", 3) == 0) {
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &key[j]);
	    }
	    keysize = j;
	    fputs(buf, aesresp);
	    continue;
	}
	/* IV = ... */
	if (strncmp(buf, "IV", 2) == 0) {
	    mode = NSS_AES_CBC;
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof iv; i+=2,j++) {
		hex_from_2char(&buf[i], &iv[j]);
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	/* PLAINTEXT = ... */
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    /* sanity check */
	    if (!encrypt) {
		goto loser;
	    }

	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &plaintext[j]);
	    }
	    plaintextlen = j;

	    rv = aes_encrypt_buf(mode, key, keysize,
		(mode == NSS_AES) ? NULL : iv,
		ciphertext, &ciphertextlen, sizeof ciphertext,
		plaintext, plaintextlen);
	    if (rv != SECSuccess) {
		goto loser;
	    }

	    fputs(buf, aesresp);
	    fputs("CIPHERTEXT = ", aesresp);
	    to_hex_str(buf, ciphertext, ciphertextlen);
	    fputs(buf, aesresp);
	    fputc('\n', aesresp);
	    continue;
	}
	/* CIPHERTEXT = ... */
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    /* sanity check */
	    if (encrypt) {
		goto loser;
	    }

	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &ciphertext[j]);
	    }
	    ciphertextlen = j;

	    rv = aes_decrypt_buf(mode, key, keysize,
		(mode == NSS_AES) ? NULL : iv,
		plaintext, &plaintextlen, sizeof plaintext,
		ciphertext, ciphertextlen);
	    if (rv != SECSuccess) {
		goto loser;
	    }

	    fputs(buf, aesresp);
	    fputs("PLAINTEXT = ", aesresp);
	    to_hex_str(buf, plaintext, plaintextlen);
	    fputs(buf, aesresp);
	    fputc('\n', aesresp);
	    continue;
	}
    }
loser:
    fclose(aesreq);
}

/*
 * Generate Key[i+1] from Key[i], CT[j-1], and CT[j] for AES Monte Carlo
 * Test (MCT) in ECB and CBC modes.
 */
void
aes_mct_next_key(unsigned char *key, unsigned int keysize,
    const unsigned char *ciphertext_1, const unsigned char *ciphertext)
{
    int k;

    switch (keysize) {
    case 16:  /* 128-bit key */
	/* Key[i+1] = Key[i] xor CT[j] */
	for (k=0; k<16; k++) {
	    key[k] ^= ciphertext[k];
	}
	break;
    case 24:  /* 192-bit key */
	/*
	 * Key[i+1] = Key[i] xor (last 64-bits of
	 *            CT[j-1] || CT[j])
	 */
	for (k=0; k<8; k++) {
	    key[k] ^= ciphertext_1[k+8];
	}
	for (k=8; k<24; k++) {
	    key[k] ^= ciphertext[k-8];
	}
	break;
    case 32:  /* 256-bit key */
	/* Key[i+1] = Key[i] xor (CT[j-1] || CT[j]) */
	for (k=0; k<16; k++) {
	    key[k] ^= ciphertext_1[k];
	}
	for (k=16; k<32; k++) {
	    key[k] ^= ciphertext[k-16];
	}
	break;
    }
}

/*
 * Perform the AES Monte Carlo Test (MCT) in ECB mode.  MCT exercises
 * our AES code in streaming mode because the plaintext or ciphertext
 * is generated block by block as we go, so we can't collect all the
 * plaintext or ciphertext in one buffer and encrypt or decrypt it in
 * one shot.
 *
 * reqfn is the pathname of the input REQUEST file.
 *
 * The output RESPONSE file is written to stdout.
 */
void
aes_ecb_mct(char *reqfn)
{
    char buf[80];       /* holds one line from the input REQUEST file.
                         * needs to be large enough to hold the longest
                         * line "KEY = <64 hex digits>\n".
                         */
    FILE *aesreq;       /* input stream from the REQUEST file */
    FILE *aesresp;      /* output stream to the RESPONSE file */
    int i, j;
    int encrypt = 0;    /* 1 means encrypt, 0 means decrypt */
    unsigned char key[32];              /* 128, 192, or 256 bits */
    unsigned int keysize;
    unsigned char plaintext[16];        /* PT[j] */
    unsigned char plaintext_1[16];      /* PT[j-1] */
    unsigned char ciphertext[16];       /* CT[j] */
    unsigned char ciphertext_1[16];     /* CT[j-1] */
    unsigned char doublecheck[16];
    unsigned int outputlen;
    AESContext *cx = NULL;	/* the operation being tested */
    AESContext *cx2 = NULL;     /* the inverse operation done in parallel
                                 * to doublecheck our result.
                                 */
    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	/* a comment or blank line */
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	/* [ENCRYPT] or [DECRYPT] */
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	/* "COUNT = x" begins a new data set */
	if (strncmp(buf, "COUNT", 5) == 0) {
	    /* zeroize the variables for the test with this data set */
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(plaintext, 0, sizeof plaintext);
	    memset(ciphertext, 0, sizeof ciphertext);
	    continue;
	}
	/* KEY = ... */
	if (strncmp(buf, "KEY", 3) == 0) {
	    /* Key[0] = Key */
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &key[j]);
	    }
	    keysize = j;
	    continue;
	}
	/* PLAINTEXT = ... */
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    /* sanity check */
	    if (!encrypt) {
		goto loser;
	    }
	    /* PT[0] = PT */
	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof plaintext; i+=2,j++) {
		hex_from_2char(&buf[i], &plaintext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		/* Output Key[i] */
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output PT[0] */
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, NULL, NSS_AES,
		    PR_TRUE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		/*
		 * doublecheck our result by decrypting the result
		 * and comparing the output with the plaintext.
		 */
		cx2 = AES_CreateContext(key, NULL, NSS_AES,
		    PR_FALSE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		for (j=0; j<1000; j++) {
		    /* Save CT[j-1] */
		    memcpy(ciphertext_1, ciphertext, sizeof ciphertext);

		    /* CT[j] = AES(Key[i], PT[j]) */
		    outputlen = 0;
		    rv = AES_Encrypt(cx,
			ciphertext, &outputlen, sizeof ciphertext,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }

		    /* doublecheck our result */
		    outputlen = 0;
		    rv = AES_Decrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, plaintext, sizeof plaintext)) {
			goto loser;
		    }

		    /* PT[j+1] = CT[j] */
		    memcpy(plaintext, ciphertext, sizeof plaintext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		/* Output CT[j] */
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		/* Key[i+1] = Key[i] xor ... */
		aes_mct_next_key(key, keysize, ciphertext_1, ciphertext);
		/* PT[0] = CT[j] */
		/* done at the end of the for(j) loop */

		fputc('\n', aesresp);
	    }

	    continue;
	}
	/* CIPHERTEXT = ... */
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    /* sanity check */
	    if (encrypt) {
		goto loser;
	    }
	    /* CT[0] = CT */
	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &ciphertext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		/* Output Key[i] */
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output CT[0] */
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, NULL, NSS_AES,
		    PR_FALSE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		/*
		 * doublecheck our result by encrypting the result
		 * and comparing the output with the ciphertext.
		 */
		cx2 = AES_CreateContext(key, NULL, NSS_AES,
		    PR_TRUE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		for (j=0; j<1000; j++) {
		    /* Save PT[j-1] */
		    memcpy(plaintext_1, plaintext, sizeof plaintext);

		    /* PT[j] = AES(Key[i], CT[j]) */
		    outputlen = 0;
		    rv = AES_Decrypt(cx,
			plaintext, &outputlen, sizeof plaintext,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }

		    /* doublecheck our result */
		    outputlen = 0;
		    rv = AES_Encrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, ciphertext, sizeof ciphertext)) {
			goto loser;
		    }

		    /* CT[j+1] = PT[j] */
		    memcpy(ciphertext, plaintext, sizeof ciphertext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		/* Output PT[j] */
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		/* Key[i+1] = Key[i] xor ... */
		aes_mct_next_key(key, keysize, plaintext_1, plaintext);
		/* CT[0] = PT[j] */
		/* done at the end of the for(j) loop */

		fputc('\n', aesresp);
	    }

	    continue;
	}
    }
loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    if (cx2 != NULL) {
	AES_DestroyContext(cx2, PR_TRUE);
    }
    fclose(aesreq);
}

/*
 * Perform the AES Monte Carlo Test (MCT) in CBC mode.  MCT exercises
 * our AES code in streaming mode because the plaintext or ciphertext
 * is generated block by block as we go, so we can't collect all the
 * plaintext or ciphertext in one buffer and encrypt or decrypt it in
 * one shot.
 *
 * reqfn is the pathname of the input REQUEST file.
 *
 * The output RESPONSE file is written to stdout.
 */
void
aes_cbc_mct(char *reqfn)
{
    char buf[80];       /* holds one line from the input REQUEST file.
                         * needs to be large enough to hold the longest
                         * line "KEY = <64 hex digits>\n".
                         */
    FILE *aesreq;       /* input stream from the REQUEST file */
    FILE *aesresp;      /* output stream to the RESPONSE file */
    int i, j;
    int encrypt = 0;    /* 1 means encrypt, 0 means decrypt */
    unsigned char key[32];              /* 128, 192, or 256 bits */
    unsigned int keysize;
    unsigned char iv[16];
    unsigned char plaintext[16];        /* PT[j] */
    unsigned char plaintext_1[16];      /* PT[j-1] */
    unsigned char ciphertext[16];       /* CT[j] */
    unsigned char ciphertext_1[16];     /* CT[j-1] */
    unsigned char doublecheck[16];
    unsigned int outputlen;
    AESContext *cx = NULL;	/* the operation being tested */
    AESContext *cx2 = NULL;     /* the inverse operation done in parallel
                                 * to doublecheck our result.
                                 */
    SECStatus rv;

    aesreq = fopen(reqfn, "r");
    aesresp = stdout;
    while (fgets(buf, sizeof buf, aesreq) != NULL) {
	/* a comment or blank line */
	if (buf[0] == '#' || buf[0] == '\n') {
	    fputs(buf, aesresp);
	    continue;
	}
	/* [ENCRYPT] or [DECRYPT] */
	if (buf[0] == '[') {
	    if (strncmp(&buf[1], "ENCRYPT", 7) == 0) {
		encrypt = 1;
	    } else {
		encrypt = 0;
	    }
	    fputs(buf, aesresp);
	    continue;
	}
	/* "COUNT = x" begins a new data set */
	if (strncmp(buf, "COUNT", 5) == 0) {
	    /* zeroize the variables for the test with this data set */
	    memset(key, 0, sizeof key);
	    keysize = 0;
	    memset(iv, 0, sizeof iv);
	    memset(plaintext, 0, sizeof plaintext);
	    memset(ciphertext, 0, sizeof ciphertext);
	    continue;
	}
	/* KEY = ... */
	if (strncmp(buf, "KEY", 3) == 0) {
	    /* Key[0] = Key */
	    i = 3;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &key[j]);
	    }
	    keysize = j;
	    continue;
	}
	/* IV = ... */
	if (strncmp(buf, "IV", 2) == 0) {
	    /* IV[0] = IV */
	    i = 2;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof iv; i+=2,j++) {
		hex_from_2char(&buf[i], &iv[j]);
	    }
	    continue;
	}
	/* PLAINTEXT = ... */
	if (strncmp(buf, "PLAINTEXT", 9) == 0) {
	    /* sanity check */
	    if (!encrypt) {
		goto loser;
	    }
	    /* PT[0] = PT */
	    i = 9;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; j<sizeof plaintext; i+=2,j++) {
		hex_from_2char(&buf[i], &plaintext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		/* Output Key[i] */
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output IV[i] */
		fputs("IV = ", aesresp);
		to_hex_str(buf, iv, sizeof iv);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output PT[0] */
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_TRUE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		/*
		 * doublecheck our result by decrypting the result
		 * and comparing the output with the plaintext.
		 */
		cx2 = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_FALSE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		/* CT[-1] = IV[i] */
		memcpy(ciphertext, iv, sizeof ciphertext);
		for (j=0; j<1000; j++) {
		    /* Save CT[j-1] */
		    memcpy(ciphertext_1, ciphertext, sizeof ciphertext);
		    /*
		     * If ( j=0 )
		     *      CT[j] = AES(Key[i], IV[i], PT[j])
		     *      PT[j+1] = IV[i] (= CT[j-1])
		     * Else
		     *      CT[j] = AES(Key[i], PT[j])
		     *      PT[j+1] = CT[j-1]
		     */
		    outputlen = 0;
		    rv = AES_Encrypt(cx,
			ciphertext, &outputlen, sizeof ciphertext,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }

		    /* doublecheck our result */
		    outputlen = 0;
		    rv = AES_Decrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, plaintext, sizeof plaintext)) {
			goto loser;
		    }

		    memcpy(plaintext, ciphertext_1, sizeof plaintext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		/* Output CT[j] */
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		/* Key[i+1] = Key[i] xor ... */
		aes_mct_next_key(key, keysize, ciphertext_1, ciphertext);
		/* IV[i+1] = CT[j] */
		memcpy(iv, ciphertext, sizeof iv);
		/* PT[0] = CT[j-1] */
		/* done at the end of the for(j) loop */

		fputc('\n', aesresp);
	    }

	    continue;
	}
	/* CIPHERTEXT = ... */
	if (strncmp(buf, "CIPHERTEXT", 10) == 0) {
	    /* sanity check */
	    if (encrypt) {
		goto loser;
	    }
	    /* CT[0] = CT */
	    i = 10;
	    while (isspace(buf[i]) || buf[i] == '=') {
		i++;
	    }
	    for (j=0; isxdigit(buf[i]); i+=2,j++) {
		hex_from_2char(&buf[i], &ciphertext[j]);
	    }

	    for (i=0; i<100; i++) {
		sprintf(buf, "COUNT = %d\n", i);
	        fputs(buf, aesresp);
		/* Output Key[i] */
		fputs("KEY = ", aesresp);
		to_hex_str(buf, key, keysize);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output IV[i] */
		fputs("IV = ", aesresp);
		to_hex_str(buf, iv, sizeof iv);
		fputs(buf, aesresp);
		fputc('\n', aesresp);
		/* Output CT[0] */
		fputs("CIPHERTEXT = ", aesresp);
		to_hex_str(buf, ciphertext, sizeof ciphertext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		cx = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_FALSE, keysize, 16);
		if (cx == NULL) {
		    goto loser;
		}
		/*
		 * doublecheck our result by encrypting the result
		 * and comparing the output with the ciphertext.
		 */
		cx2 = AES_CreateContext(key, iv, NSS_AES_CBC,
		    PR_TRUE, keysize, 16);
		if (cx2 == NULL) {
		    goto loser;
		}
		/* PT[-1] = IV[i] */
		memcpy(plaintext, iv, sizeof plaintext);
		for (j=0; j<1000; j++) {
		    /* Save PT[j-1] */
		    memcpy(plaintext_1, plaintext, sizeof plaintext);
		    /*
		     * If ( j=0 )
		     *      PT[j] = AES(Key[i], IV[i], CT[j])
		     *      CT[j+1] = IV[i] (= PT[j-1])
		     * Else
		     *      PT[j] = AES(Key[i], CT[j])
		     *      CT[j+1] = PT[j-1]
		     */
		    outputlen = 0;
		    rv = AES_Decrypt(cx,
			plaintext, &outputlen, sizeof plaintext,
			ciphertext, sizeof ciphertext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof ciphertext) {
			goto loser;
		    }

		    /* doublecheck our result */
		    outputlen = 0;
		    rv = AES_Encrypt(cx2,
			doublecheck, &outputlen, sizeof doublecheck,
			plaintext, sizeof plaintext);
		    if (rv != SECSuccess) {
			goto loser;
		    }
		    if (outputlen != sizeof plaintext) {
			goto loser;
		    }
		    if (memcmp(doublecheck, ciphertext, sizeof ciphertext)) {
			goto loser;
		    }

		    memcpy(ciphertext, plaintext_1, sizeof ciphertext);
		}
		AES_DestroyContext(cx, PR_TRUE);
		cx = NULL;
		AES_DestroyContext(cx2, PR_TRUE);
		cx2 = NULL;

		/* Output PT[j] */
		fputs("PLAINTEXT = ", aesresp);
		to_hex_str(buf, plaintext, sizeof plaintext);
		fputs(buf, aesresp);
		fputc('\n', aesresp);

		/* Key[i+1] = Key[i] xor ... */
		aes_mct_next_key(key, keysize, plaintext_1, plaintext);
		/* IV[i+1] = PT[j] */
		memcpy(iv, plaintext, sizeof iv);
		/* CT[0] = PT[j-1] */
		/* done at the end of the for(j) loop */

		fputc('\n', aesresp);
	    }

	    continue;
	}
    }
loser:
    if (cx != NULL) {
	AES_DestroyContext(cx, PR_TRUE);
    }
    if (cx2 != NULL) {
	AES_DestroyContext(cx2, PR_TRUE);
    }
    fclose(aesreq);
}

void write_compact_string(FILE *out, unsigned char *hash, unsigned int len)
{
    unsigned int i;
    int j, count = 0, last = -1, z = 0;
    long start = ftell(out);
    for (i=0; i<len; i++) {
	for (j=7; j>=0; j--) {
	    if (last < 0) {
		last = (hash[i] & (1 << j)) ? 1 : 0;
		fprintf(out, "%d ", last);
		count = 1;
	    } else if (hash[i] & (1 << j)) {
		if (last) {
		    count++; 
		} else { 
		    last = 0;
		    fprintf(out, "%d ", count);
		    count = 1;
		    z++;
		}
	    } else {
		if (!last) {
		    count++; 
		} else { 
		    last = 1;
		    fprintf(out, "%d ", count);
		    count = 1;
		    z++;
		}
	    }
	}
    }
    fprintf(out, "^\n");
    fseek(out, start, SEEK_SET);
    fprintf(out, "%d ", z);
    fseek(out, 0, SEEK_END);
}

void do_shs_type3(FILE *out, unsigned char *M, unsigned int len)
{
    int i, j, a;
    unsigned char zero[30];
    unsigned char iword[4];
    unsigned int l = len;
    char hashstr[41];
    SHA1Context *cx;
    memset(zero, 0, sizeof zero);
    for (j=0; j<100; j++) {
	cx = SHA1_NewContext();
	for (i=1; i<=50000; i++) {
	    SHA1_Begin(cx);
	    SHA1_Update(cx, M, l);
	    a = j/4 + 3;
	    SHA1_Update(cx, zero, a);
	    iword[3] = (char)i;
	    iword[2] = (char)(i >> 8);
	    iword[1] = (char)(i >> 16);
	    iword[0] = (char)(i >> 24);
	    SHA1_Update(cx, iword, 4);
	    SHA1_End(cx, M, &l, 20);
	}
	SHA1_DestroyContext(cx, PR_TRUE);
	to_hex_str_cap(hashstr, M, l);
	hashstr[40] = '\0';
	fprintf(out, "%s ^", hashstr);
	if (j<99) fprintf(out, "\n");
    }
}

void
shs_test(char *reqfn)
{
    char buf[80];
    FILE *shareq, *sharesp;
    char readbuf[64];
    int i, nr;
    int newline, skip, r_z, r_b, r_n, r, b, z, n, reading;
    unsigned char hash[20];
    char hashstr[41];
    unsigned char input[13000];
    int next_bit = 0;
    int shs_type = 0;
    shareq = fopen(reqfn, "r");
    sharesp = stdout;
    newline = 1;
    reading = skip = r_z = r_b = r_n = z = r = n = 0;
    while ((nr = fread(buf, 1, sizeof buf, shareq)) > 0) {
	for (i=0; i<nr; i++) {
	    if (newline) {
		if (buf[i] == '#' || buf[i] == 'D' || buf[i] == '<') {
		    skip = 1;
		} else if (buf[i] == 'H') {
		    skip = 0;
		    shs_type++;
		    fprintf(sharesp, "H>SHS Type %d Hashes<H", shs_type);
		} else if (isdigit(buf[i])) {
		    r_z = 1;
		    readbuf[r++] = buf[i];
		}
		newline =  (buf[i] == '\n') ? 1 : 0;
	    } else {
		if (buf[i] == '\n' && !r_n) {
		    skip = r_z = r_n = 0;
		    newline = 1;
		} else if (r_z) {
		    if (buf[i] == ' ') {
			r_z = 0;
			readbuf[r] = '\0';
			z = atoi(readbuf);
			r_b = 1;
			r = 0;
		    } else if (isdigit(buf[i])) {
			readbuf[r++] = buf[i];
		    }
		} else if (r_b) {
		    if (buf[i] == ' ') {
			r_b = 0;
			readbuf[r] = '\0';
			b = atoi(readbuf);
			r_n = 1;
			r = 0;
		    } else if (isdigit(buf[i])) {
			readbuf[r++] = buf[i];
		    }
		} else if (r_n) {
		    if (buf[i] == ' ') {
			readbuf[r++] = '\0';
			n = atoi(readbuf);
			if (b == 0) {
			    next_bit += n;
			    b = 1;
			} else {
			    int next_byte = next_bit / 8;
			    int shift = next_bit % 8;
			    unsigned char m = 0xff;
			    if (n < 8 - shift) {
				m <<= (8 - n);
				m >>= shift;
				input[next_byte] |= m;
				next_bit += n;
			    } else {
				m >>= shift;
				input[next_byte++] |= m;
				next_bit += 8 - shift;
				n -= (8 - shift);
				while (n > 8) {
				    m = 0xff;
				    input[next_byte++] |= m;
				    next_bit += 8;
				    n -= 8;
				}
				if (n > 0) {
				    m = 0xff << (8 - n);
				    input[next_byte] |= m;
				    next_bit += n;
				}
			    }
			    b = 0;
			}
			r = 0;
		    } else if (buf[i] == '^') {
			r_n = 0;
			if (shs_type < 3) {
			    SHA1_HashBuf(hash, input, next_bit/8);
			    to_hex_str_cap(hashstr, hash, sizeof hash);
			    hashstr[40] = '\0';
			    fprintf(sharesp, "%s ^", hashstr);
			    memset(input, 0, sizeof input);
			    next_bit = 0;
			} else {
			    do_shs_type3(sharesp, input, next_bit/8);
			}
		    } else if (isdigit(buf[i])) {
			readbuf[r++] = buf[i];
		    }
		}
	    }
	    if (skip || newline) {
		fprintf(sharesp, "%c", buf[i]);
	    }
	}
    }
}

int get_next_line(FILE *req, char *key, char *val, FILE *rsp)
{
    int ignore = 0;
    char *writeto = key;
    int w = 0;
    int c;
    while ((c = fgetc(req)) != EOF) {
	if (ignore) {
	    fprintf(rsp, "%c", c);
	    if (c == '\n') return ignore;
	} else if (c == '\n') {
	    break;
	} else if (c == '#') {
	    ignore = 1;
	    fprintf(rsp, "%c", c);
	} else if (c == '=') {
	    writeto[w] = '\0';
	    w = 0;
	    writeto = val;
	} else if (c == ' ' || c == '[' || c == ']') {
	    continue;
	} else {
	    writeto[w++] = c;
	}
    }
    writeto[w] = '\0';
    return (c == EOF) ? -1 : ignore;
}

void
dss_test(char *reqdir, char *rspdir)
{
    char filename[128];
    char key[24], val[1024];
    FILE *req, *rsp;
    unsigned int mod;
    SECItem digest = { 0 }, sig = { 0 };
    DSAPublicKey pubkey = { 0 };
    DSAPrivateKey privkey = { 0 };
    PQGParams params;
    PQGVerify verify;
    unsigned int i;
    int j, rv;
    goto do_pqggen;
#if 0
    /* primality test */
do_prime:
    sprintf(filename, "%s/prime.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/prime.rsp", rspdir);
    rsp = fopen(filename, "w");
    while ((rv = get_next_line(req, key, val, rsp)) >= 0) {
	if (rv == 0) {
	    if (strcmp(key, "mod") == 0) {
		mod = atoi(val);
		fprintf(rsp, "[mod=%d]\n", mod);
	    } else if (strcmp(key, "Prime") == 0) {
		unsigned char octets[128];
		mp_int mp;
		fprintf(rsp, "Prime= %s\n", val);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, octets + i);
		}
		mp_init(&mp);
		mp_read_unsigned_octets(&mp, octets, i);
		if (mpp_pprime(&mp, 50) == MP_YES) {
		    fprintf(rsp, "result= P\n");
		} else {
		    fprintf(rsp, "result= F\n");
		}
	    }
	}
    }
    fclose(req);
    fclose(rsp);
#endif
do_pqggen:
    /* PQG Gen */
    sprintf(filename, "%s/pqg.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/pqg.rsp", rspdir);
    rsp = fopen(filename, "w");
    while ((rv = get_next_line(req, key, val, rsp)) >= 0) {
	if (rv == 0) {
	    if (strcmp(key, "mod") == 0) {
		mod = atoi(val);
		fprintf(rsp, "[mod=%d]\n", mod);
	    } else if (strcmp(key, "N") == 0) {
		char str[264];
		unsigned int jj;
		int N = atoi(val);
		for (i=0; i<N; i++) {
		    PQGParams *pqg;
		    PQGVerify *vfy;
		    PQG_ParamGenSeedLen(PQG_PBITS_TO_INDEX(mod), 20, &pqg, &vfy);
#if 0
		    if (!(vfy->seed.data[0] & 0x80)) {
			to_hex_str(str, vfy->seed.data, vfy->seed.len);
			fprintf(stderr, "rejected %s\n", str);
			--i;
			continue;
		    }
#endif
		    to_hex_str(str, pqg->prime.data, pqg->prime.len);
		    fprintf(rsp, "P= %s\n", str);
		    to_hex_str(str, pqg->subPrime.data, pqg->subPrime.len);
		    fprintf(rsp, "Q= %s\n", str);
		    to_hex_str(str, pqg->base.data, pqg->base.len);
		    fprintf(rsp, "G= %s\n", str);
		    to_hex_str(str, vfy->seed.data, vfy->seed.len);
		    fprintf(rsp, "Seed= %s\n", str);
		    to_hex_str(str, vfy->h.data, vfy->h.len);
		    fprintf(rsp, "H= ");
		    for (jj=vfy->h.len; jj<pqg->prime.len; jj++) {
			fprintf(rsp, "00");
		    }
		    fprintf(rsp, "%s\n", str);
		    fprintf(rsp, "c= %d\n", vfy->counter);
		}
	    }
	}
    }
    fclose(req);
    fclose(rsp);
    return;
do_pqgver:
    /* PQG Verification */
    sprintf(filename, "%s/verpqg.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/verpqg.rsp", rspdir);
    rsp = fopen(filename, "w");
    memset(&params, 0, sizeof(params));
    memset(&verify, 0, sizeof(verify));
    while ((rv = get_next_line(req, key, val, rsp)) >= 0) {
	if (rv == 0) {
	    if (strcmp(key, "mod") == 0) {
		mod = atoi(val);
		fprintf(rsp, "[mod=%d]\n", mod);
	    } else if (strcmp(key, "P") == 0) {
		if (params.prime.data) {
		    SECITEM_ZfreeItem(&params.prime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &params.prime, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, params.prime.data + i);
		}
		fprintf(rsp, "P= %s\n", val);
	    } else if (strcmp(key, "Q") == 0) {
		if (params.subPrime.data) {
		    SECITEM_ZfreeItem(&params.subPrime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &params.subPrime,strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, params.subPrime.data + i);
		}
		fprintf(rsp, "Q= %s\n", val);
	    } else if (strcmp(key, "G") == 0) {
		if (params.base.data) {
		    SECITEM_ZfreeItem(&params.base, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &params.base, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, params.base.data + i);
		}
		fprintf(rsp, "G= %s\n", val);
	    } else if (strcmp(key, "Seed") == 0) {
		if (verify.seed.data) {
		    SECITEM_ZfreeItem(&verify.seed, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &verify.seed, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, verify.seed.data + i);
		}
		fprintf(rsp, "Seed= %s\n", val);
	    } else if (strcmp(key, "H") == 0) {
		if (verify.h.data) {
		    SECITEM_ZfreeItem(&verify.h, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &verify.h, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, verify.h.data + i);
		}
		fprintf(rsp, "H= %s\n", val);
	    } else if (strcmp(key, "c") == 0) {
		SECStatus pqgrv, result;
		verify.counter = atoi(val);
		fprintf(rsp, "c= %d\n", verify.counter);
		pqgrv = PQG_VerifyParams(&params, &verify, &result);
		if (result == SECSuccess) {
		    fprintf(rsp, "result= P\n");
		} else {
		    fprintf(rsp, "result= F\n");
		}
	    }
	}
    }
    fclose(req);
    fclose(rsp);
    return;
do_keygen:
    /* Key Gen */
    sprintf(filename, "%s/xy.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/xy.rsp", rspdir);
    rsp = fopen(filename, "w");
    while ((rv = get_next_line(req, key, val, rsp)) >= 0);
    for (j=0; j<=8; j++) {
	char str[264];
	PQGParams *pqg;
	PQGVerify *vfy;
	fprintf(rsp, "[mod=%d]\n", 512 + j*64);
	PQG_ParamGen(j, &pqg, &vfy);
	to_hex_str(str, pqg->prime.data, pqg->prime.len);
	fprintf(rsp, "P= %s\n", str);
	to_hex_str(str, pqg->subPrime.data, pqg->subPrime.len);
	fprintf(rsp, "Q= %s\n", str);
	to_hex_str(str, pqg->base.data, pqg->base.len);
	fprintf(rsp, "G= %s\n", str);
	for (i=0; i<10; i++) {
	    DSAPrivateKey *dsakey;
	    DSA_NewKey(pqg, &dsakey);
	    to_hex_str(str, dsakey->privateValue.data,dsakey->privateValue.len);
	    fprintf(rsp, "X= %s\n", str);
	    to_hex_str(str, dsakey->publicValue.data, dsakey->publicValue.len);
	    fprintf(rsp, "Y= %s\n", str);
	    PORT_FreeArena(dsakey->params.arena, PR_TRUE);
	    dsakey = NULL;
	}
    }
    fclose(req);
    fclose(rsp);
    return;
do_siggen:
    /* Signature Gen */
    sprintf(filename, "%s/gensig.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/gensig.rsp", rspdir);
    rsp = fopen(filename, "w");
    while ((rv = get_next_line(req, key, val, rsp)) >= 0) {
	if (rv == 0) {
	    if (strcmp(key, "mod") == 0) {
		mod = atoi(val);
		fprintf(rsp, "[mod=%d]\n", mod);
	    } else if (strcmp(key, "P") == 0) {
		if (privkey.params.prime.data) {
		    SECITEM_ZfreeItem(&privkey.params.prime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &privkey.params.prime, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, privkey.params.prime.data + i);
		}
		fprintf(rsp, "P= %s\n", val);
	    } else if (strcmp(key, "Q") == 0) {
		if (privkey.params.subPrime.data) {
		    SECITEM_ZfreeItem(&privkey.params.subPrime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &privkey.params.subPrime,strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, privkey.params.subPrime.data + i);
		}
		fprintf(rsp, "Q= %s\n", val);
	    } else if (strcmp(key, "G") == 0) {
		if (privkey.params.base.data) {
		    SECITEM_ZfreeItem(&privkey.params.base, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &privkey.params.base, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, privkey.params.base.data + i);
		}
		fprintf(rsp, "G= %s\n", val);
	    } else if (strcmp(key, "X") == 0) {
		if (privkey.privateValue.data) {
		    SECITEM_ZfreeItem(&privkey.privateValue, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &privkey.privateValue, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, privkey.privateValue.data + i);
		}
		fprintf(rsp, "X= %s\n", val);
	    } else if (strcmp(key, "Msg") == 0) {
		char msg[512];
		char str[81];
		if (digest.data) {
		    SECITEM_ZfreeItem(&digest, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &digest, 20);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, msg + i);
		}
		msg[i] = '\0';
		/*SHA1_Hash(digest.data, msg);*/
		SHA1_HashBuf(digest.data, msg, i);
		fprintf(rsp, "Msg= %s\n", val);
		if (sig.data) {
		    SECITEM_ZfreeItem(&sig, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &sig, 40);
		rv = DSA_SignDigest(&privkey, &sig, &digest);
		to_hex_str(str, sig.data, sig.len);
		fprintf(rsp, "Sig= %s\n", str);
	    }
	}
    }
    fclose(req);
    fclose(rsp);
do_sigver:
    /* Signature Verification */
    sprintf(filename, "%s/versig.req", reqdir);
    req = fopen(filename, "r");
    sprintf(filename, "%s/versig.rsp", rspdir);
    rsp = fopen(filename, "w");
    while ((rv = get_next_line(req, key, val, rsp)) >= 0) {
	if (rv == 0) {
	    if (strcmp(key, "mod") == 0) {
		mod = atoi(val);
		fprintf(rsp, "[mod=%d]\n", mod);
	    } else if (strcmp(key, "P") == 0) {
		if (pubkey.params.prime.data) {
		    SECITEM_ZfreeItem(&pubkey.params.prime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &pubkey.params.prime, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, pubkey.params.prime.data + i);
		}
		fprintf(rsp, "P= %s\n", val);
	    } else if (strcmp(key, "Q") == 0) {
		if (pubkey.params.subPrime.data) {
		    SECITEM_ZfreeItem(&pubkey.params.subPrime, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &pubkey.params.subPrime, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, pubkey.params.subPrime.data + i);
		}
		fprintf(rsp, "Q= %s\n", val);
	    } else if (strcmp(key, "G") == 0) {
		if (pubkey.params.base.data) {
		    SECITEM_ZfreeItem(&pubkey.params.base, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &pubkey.params.base, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, pubkey.params.base.data + i);
		}
		fprintf(rsp, "G= %s\n", val);
	    } else if (strcmp(key, "Y") == 0) {
		if (pubkey.publicValue.data) {
		    SECITEM_ZfreeItem(&pubkey.publicValue, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &pubkey.publicValue, strlen(val)/2);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, pubkey.publicValue.data + i);
		}
		fprintf(rsp, "Y= %s\n", val);
	    } else if (strcmp(key, "Msg") == 0) {
		char msg[512];
		if (digest.data) {
		    SECITEM_ZfreeItem(&digest, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &digest, 20);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, msg + i);
		}
		msg[i] = '\0';
		SHA1_HashBuf(digest.data, msg, i);
		/*SHA1_Hash(digest.data, msg);*/
		fprintf(rsp, "Msg= %s\n", val);
	    } else if (strcmp(key, "Sig") == 0) {
		SECStatus rv;
		if (sig.data) {
		    SECITEM_ZfreeItem(&sig, PR_FALSE);
		}
		SECITEM_AllocItem(NULL, &sig, 40);
		for (i=0; i<strlen(val) / 2; i++) {
		    hex_from_2char(val + 2*i, sig.data + i);
		}
		rv = DSA_VerifyDigest(&pubkey, &sig, &digest);
		fprintf(rsp, "Sig= %s\n", val);
		if (rv == SECSuccess) {
		    fprintf(rsp, "result= P\n");
		} else {
		    fprintf(rsp, "result= F\n");
		}
	    }
	}
    }
    fclose(req);
    fclose(rsp);
}

void do_random()
{
    int i, j, k = 0;
    unsigned char buf[500];
    for (i=0; i<5; i++) {
	RNG_GenerateGlobalRandomBytes(buf, sizeof buf);
	for (j=0; j<sizeof buf / 2; j++) {
	    printf("0x%02x%02x", buf[2*j], buf[2*j+1]);
	    if (++k % 8 == 0) printf("\n"); else printf(" ");
	}
    }
}

int main(int argc, char **argv)
{
    unsigned char key[24];
    unsigned char inp[24];
    unsigned char iv[8];
    if (argc < 2) exit (-1);
    NSS_NoDB_Init(NULL);
    memset(inp, 0, sizeof inp);
    /*************/
    /*    DES    */
    /*************/
    /**** ECB ****/
    /* encrypt */
    if (       strcmp(argv[1], "des_5_1_1_1") == 0) {
	printf("Variable Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_var_pt_kat(NSS_DES, PR_TRUE, 8, key, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_1_2") == 0) {
	printf("Inverse Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_inv_perm_kat(NSS_DES, PR_TRUE, 8, key, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_1_3") == 0) {
	printf("Variable Key Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_var_key_kat(NSS_DES, PR_TRUE, 8, key, NULL, inp);
    } else if (strcmp(argv[1], "des_5_1_1_4") == 0) {
	printf("Permutation Operation Known Answer Test - Encryption\n\n");
	des_perm_op_kat(NSS_DES, PR_TRUE, 8, NULL, NULL, inp);
    } else if (strcmp(argv[1], "des_5_1_1_5") == 0) {
	printf("Substitution Table Known Answer Test - Encryption\n\n");
	des_sub_tbl_kat(NSS_DES, PR_TRUE, 8, NULL, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_1_6") == 0) {
	printf("Modes Test for the Encryption Process\n\n");
	des_modes(NSS_DES, PR_TRUE, 8, des_ecb_enc_key, 
                  NULL, des_ecb_enc_inp, 0);
    /* decrypt */
    } else if (strcmp(argv[1], "des_5_1_2_1") == 0) {
	printf("Variable Ciphertext Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_var_pt_kat(NSS_DES, PR_FALSE, 8, key, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_2_2") == 0) {
	printf("Inverse Permutation Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_inv_perm_kat(NSS_DES, PR_FALSE, 8, key, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_2_3") == 0) {
	printf("Variable Key Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_var_key_kat(NSS_DES, PR_FALSE, 8, key, NULL, inp);
    } else if (strcmp(argv[1], "des_5_1_2_4") == 0) {
	printf("Permutation Operation Known Answer Test - Decryption\n\n");
	des_perm_op_kat(NSS_DES, PR_FALSE, 8, NULL, NULL, inp);
    } else if (strcmp(argv[1], "des_5_1_2_5") == 0) {
	printf("Substitution Table Known Answer Test - Decryption\n\n");
	des_sub_tbl_kat(NSS_DES, PR_FALSE, 8, NULL, NULL, NULL);
    } else if (strcmp(argv[1], "des_5_1_2_6") == 0) {
	printf("Modes Test for the Decryption Process\n\n");
	des_modes(NSS_DES, PR_FALSE, 8, des_ecb_dec_key, 
                  NULL, des_ecb_dec_inp, 0);
    /**** CBC ****/
    /* encrypt */
    } else if (strcmp(argv[1], "des_5_2_1_1") == 0) {
	printf("Variable Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_pt_kat(NSS_DES_CBC, PR_TRUE, 8, key, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_1_2") == 0) {
	printf("Inverse Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_inv_perm_kat(NSS_DES_CBC, PR_TRUE, 8, key, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_1_3") == 0) {
	printf("Variable Key Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_key_kat(NSS_DES_CBC, PR_TRUE, 8, key, iv, inp);
    } else if (strcmp(argv[1], "des_5_2_1_4") == 0) {
	printf("Permutation Operation Known Answer Test - Encryption\n\n");
	memset(iv, 0, sizeof iv);
	des_perm_op_kat(NSS_DES_CBC, PR_TRUE, 8, NULL, iv, inp);
    } else if (strcmp(argv[1], "des_5_2_1_5") == 0) {
	printf("Substitution Table Known Answer Test - Encryption\n\n");
	memset(iv, 0, sizeof iv);
	des_sub_tbl_kat(NSS_DES_CBC, PR_TRUE, 8, NULL, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_1_6") == 0) {
	printf("Modes Test for the Encryption Process\n\n");
	des_modes(NSS_DES_CBC, PR_TRUE, 8, des_cbc_enc_key, 
                  des_cbc_enc_iv, des_cbc_enc_inp, 0);
    /* decrypt */
    } else if (strcmp(argv[1], "des_5_2_2_1") == 0) {
	printf("Variable Ciphertext Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_pt_kat(NSS_DES_CBC, PR_FALSE, 8, key, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_2_2") == 0) {
	printf("Inverse Permutation Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_inv_perm_kat(NSS_DES_CBC, PR_FALSE, 8, key, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_2_3") == 0) {
	printf("Variable Key Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_key_kat(NSS_DES_CBC, PR_FALSE, 8, key, iv, inp);
    } else if (strcmp(argv[1], "des_5_2_2_4") == 0) {
	printf("Permutation Operation Known Answer Test - Decryption\n\n");
	memset(iv, 0, sizeof iv);
	des_perm_op_kat(NSS_DES_CBC, PR_FALSE, 8, NULL, iv, inp);
    } else if (strcmp(argv[1], "des_5_2_2_5") == 0) {
	printf("Substitution Table Known Answer Test - Decryption\n\n");
	memset(iv, 0, sizeof iv);
	des_sub_tbl_kat(NSS_DES_CBC, PR_FALSE, 8, NULL, iv, NULL);
    } else if (strcmp(argv[1], "des_5_2_2_6") == 0) {
	printf("Modes Test for the Decryption Process\n\n");
	des_modes(NSS_DES_CBC, PR_FALSE, 8, des_cbc_dec_key, 
                  des_cbc_dec_iv, des_cbc_dec_inp, 0);
    /*************/
    /*   TDEA    */
    /*************/
    /**** ECB ****/
    /* encrypt */
    } else if (strcmp(argv[1], "tdea_5_1_1_1") == 0) {
	printf("Variable Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_var_pt_kat(NSS_DES_EDE3, PR_TRUE, 24, key, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_1_2") == 0) {
	printf("Inverse Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_inv_perm_kat(NSS_DES_EDE3, PR_TRUE, 24, key, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_1_3") == 0) {
	printf("Variable Key Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	des_var_key_kat(NSS_DES_EDE3, PR_TRUE, 24, key, NULL, inp);
    } else if (strcmp(argv[1], "tdea_5_1_1_4") == 0) {
	printf("Permutation Operation Known Answer Test - Encryption\n\n");
	des_perm_op_kat(NSS_DES_EDE3, PR_TRUE, 24, NULL, NULL, inp);
    } else if (strcmp(argv[1], "tdea_5_1_1_5") == 0) {
	printf("Substitution Table Known Answer Test - Encryption\n\n");
	des_sub_tbl_kat(NSS_DES_EDE3, PR_TRUE, 24, NULL, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_1_6_3") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datecbmontee1\n\n");
	des_modes(NSS_DES_EDE3, PR_TRUE, 24, tdea1_ecb_enc_key, 
                  NULL, tdea1_ecb_enc_inp, 1);
    } else if (strcmp(argv[1], "tdea_5_1_1_6_2") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datecbmontee2\n\n");
	des_modes(NSS_DES_EDE3, PR_TRUE, 24, tdea2_ecb_enc_key, 
                  NULL, tdea2_ecb_enc_inp, 2);
    } else if (strcmp(argv[1], "tdea_5_1_1_6_1") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datecbmontee3\n\n");
	des_modes(NSS_DES_EDE3, PR_TRUE, 24, tdea3_ecb_enc_key, 
                  NULL, tdea3_ecb_enc_inp, 3);
    /* decrypt */
    } else if (strcmp(argv[1], "tdea_5_1_2_1") == 0) {
	printf("Variable Ciphertext Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_var_pt_kat(NSS_DES_EDE3, PR_FALSE, 24, key, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_2_2") == 0) {
	printf("Inverse Permutation Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_inv_perm_kat(NSS_DES_EDE3, PR_FALSE, 24, key, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_2_3") == 0) {
	printf("Variable Key Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	des_var_key_kat(NSS_DES_EDE3, PR_FALSE, 24, key, NULL, inp);
    } else if (strcmp(argv[1], "tdea_5_1_2_4") == 0) {
	printf("Permutation Operation Known Answer Test - Decryption\n\n");
	des_perm_op_kat(NSS_DES_EDE3, PR_FALSE, 24, NULL, NULL, inp);
    } else if (strcmp(argv[1], "tdea_5_1_2_5") == 0) {
	printf("Substitution Table Known Answer Test - Decryption\n\n");
	des_sub_tbl_kat(NSS_DES_EDE3, PR_FALSE, 24, NULL, NULL, NULL);
    } else if (strcmp(argv[1], "tdea_5_1_2_6_3") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datecbmonted1\n\n");
	des_modes(NSS_DES_EDE3, PR_FALSE, 24, tdea1_ecb_dec_key, 
                  NULL, tdea1_ecb_dec_inp, 1);
    } else if (strcmp(argv[1], "tdea_5_1_2_6_2") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datecbmonted2\n\n");
	des_modes(NSS_DES_EDE3, PR_FALSE, 24, tdea2_ecb_dec_key, 
                  NULL, tdea2_ecb_dec_inp, 2);
    } else if (strcmp(argv[1], "tdea_5_1_2_6_1") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datecbmonted3\n\n");
	des_modes(NSS_DES_EDE3, PR_FALSE, 24, tdea3_ecb_dec_key, 
                  NULL, tdea3_ecb_dec_inp, 3);
    /**** CBC ****/
    /* encrypt */
    } else if (strcmp(argv[1], "tdea_5_2_1_1") == 0) {
	printf("Variable Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_pt_kat(NSS_DES_EDE3_CBC, PR_TRUE, 24, key, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_1_2") == 0) {
	printf("Inverse Plaintext Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_inv_perm_kat(NSS_DES_EDE3_CBC, PR_TRUE, 24, key, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_1_3") == 0) {
	printf("Variable Key Known Answer Test - Encryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_key_kat(NSS_DES_EDE3_CBC, PR_TRUE, 24, key, iv, inp);
    } else if (strcmp(argv[1], "tdea_5_2_1_4") == 0) {
	printf("Permutation Operation Known Answer Test - Encryption\n\n");
	memset(iv, 0, sizeof iv);
	des_perm_op_kat(NSS_DES_EDE3_CBC, PR_TRUE, 24, NULL, iv, inp);
    } else if (strcmp(argv[1], "tdea_5_2_1_5") == 0) {
	memset(iv, 0, sizeof iv);
	printf("Substitution Table Known Answer Test - Encryption\n\n");
	des_sub_tbl_kat(NSS_DES_EDE3_CBC, PR_TRUE, 24, NULL, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_1_6_3") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmontee1\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_TRUE, 24, tdea1_cbc_enc_key, 
                  tdea1_cbc_enc_iv, tdea1_cbc_enc_inp, 1);
    } else if (strcmp(argv[1], "tdea_5_2_1_6_2") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmontee2\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_TRUE, 24, tdea2_cbc_enc_key, 
                  tdea2_cbc_enc_iv, tdea2_cbc_enc_inp, 2);
    } else if (strcmp(argv[1], "tdea_5_2_1_6_1") == 0) {
	printf("Modes Test for the Encryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmontee3\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_TRUE, 24, tdea3_cbc_enc_key, 
                  tdea3_cbc_enc_iv, tdea3_cbc_enc_inp, 3);
    /* decrypt */
    } else if (strcmp(argv[1], "tdea_5_2_2_1") == 0) {
	printf("Variable Ciphertext Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_pt_kat(NSS_DES_EDE3_CBC, PR_FALSE, 24, key, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_2_2") == 0) {
	printf("Inverse Permutation Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_inv_perm_kat(NSS_DES_EDE3_CBC, PR_FALSE, 24, key, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_2_3") == 0) {
	printf("Variable Key Known Answer Test - Decryption\n\n");
	memset(key, 1, sizeof key);
	memset(iv, 0, sizeof iv);
	des_var_key_kat(NSS_DES_EDE3_CBC, PR_FALSE, 24, key, iv, inp);
    } else if (strcmp(argv[1], "tdea_5_2_2_4") == 0) {
	printf("Permutation Operation Known Answer Test - Decryption\n\n");
	memset(iv, 0, sizeof iv);
	des_perm_op_kat(NSS_DES_EDE3_CBC, PR_FALSE, 24, NULL, iv, inp);
    } else if (strcmp(argv[1], "tdea_5_2_2_5") == 0) {
	printf("Substitution Table Known Answer Test - Decryption\n\n");
	memset(iv, 0, sizeof iv);
	des_sub_tbl_kat(NSS_DES_EDE3_CBC, PR_FALSE, 24, NULL, iv, NULL);
    } else if (strcmp(argv[1], "tdea_5_2_2_6_3") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmonted1\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_FALSE, 24, tdea1_cbc_dec_key, 
                  tdea1_cbc_dec_iv, tdea1_cbc_dec_inp, 1);
    } else if (strcmp(argv[1], "tdea_5_2_2_6_2") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmonted2\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_FALSE, 24, tdea2_cbc_dec_key, 
                  tdea2_cbc_dec_iv, tdea2_cbc_dec_inp, 2);
    } else if (strcmp(argv[1], "tdea_5_2_2_6_1") == 0) {
	printf("Modes Test for the Decryption Process\n");
	printf("DATA FILE UTILIZED: datcbcmonted3\n\n");
	des_modes(NSS_DES_EDE3_CBC, PR_FALSE, 24, tdea3_cbc_dec_key, 
                  tdea3_cbc_dec_iv, tdea3_cbc_dec_inp, 3);
    /*************/
    /*   TDEA    */
    /*************/
    /* encrypt */
    } else if (strcmp(argv[1], "tdea") == 0) {
        /* argv[2]=kat|mmt|mct argv[3]=ecb|cbc argv[4]=<test name>.req */
        if (strcmp(argv[2], "kat") == 0) {
            /* Known Answer Test (KAT) */
            tdea_kat_mmt(argv[4]);     
        } else if (strcmp(argv[2], "mmt") == 0) {
            /* Multi-block Message Test (MMT) */
                tdea_kat_mmt(argv[4]);
        } else if (strcmp(argv[2], "mct") == 0) {
                /* Monte Carlo Test (MCT) */
                if (strcmp(argv[3], "ecb") == 0) {
                    /* ECB mode */
                    tdea_mct(NSS_DES_EDE3, argv[4]); 
                } else if (strcmp(argv[3], "cbc") == 0) {
                    /* CBC mode */
                    tdea_mct(NSS_DES_EDE3_CBC, argv[4]);
                }
        }
    /*************/
    /*   AES     */
    /*************/
    } else if (strcmp(argv[1], "aes") == 0) {
	/* argv[2]=kat|mmt|mct argv[3]=ecb|cbc argv[4]=<test name>.req */
	if (       strcmp(argv[2], "kat") == 0) {
	    /* Known Answer Test (KAT) */
	    aes_kat_mmt(argv[4]);
	} else if (strcmp(argv[2], "mmt") == 0) {
	    /* Multi-block Message Test (MMT) */
	    aes_kat_mmt(argv[4]);
	} else if (strcmp(argv[2], "mct") == 0) {
	    /* Monte Carlo Test (MCT) */
	    if (       strcmp(argv[3], "ecb") == 0) {
		/* ECB mode */
		aes_ecb_mct(argv[4]);
	    } else if (strcmp(argv[3], "cbc") == 0) {
		/* CBC mode */
		aes_cbc_mct(argv[4]);
	    }
	}
    /*************/
    /*   SHS     */
    /*************/
    } else if (strcmp(argv[1], "shs") == 0) {
	shs_test(argv[2]);
    /*************/
    /*   DSS     */
    /*************/
    } else if (strcmp(argv[1], "dss") == 0) {
	dss_test(argv[2], argv[3]);
    /*************/
    /*   RNG     */
    /*************/
    } else if (strcmp(argv[1], "rng") == 0) {
	do_random();
    }
    return 0;
}
