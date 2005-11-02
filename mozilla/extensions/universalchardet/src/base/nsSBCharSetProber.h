/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsSingleByteCharSetProber_h__
#define nsSingleByteCharSetProber_h__

#include "nsCharSetProber.h"

#define SAMPLE_SIZE 64
#define SB_ENOUGH_REL_THRESHOLD  1024
#define POSITIVE_SHORTCUT_THRESHOLD  (float)0.95
#define NEGATIVE_SHORTCUT_THRESHOLD  (float)0.05
#define SYMBOL_CAT_ORDER  250

typedef struct
{
  unsigned char *charToOrderMap;     //[256]
  char *precedenceMatrix;           //[SAMPLE_SIZE][SAMPLE_SIZE];
  PRBool keepEnglishLetter;
  const char* charsetName;
} SequenceModel;


class nsSingleByteCharSetProber : public nsCharSetProber{
public:
  nsSingleByteCharSetProber(SequenceModel *model){mModel = model; Reset();};
  const char* GetCharSetName() {return mModel->charsetName;};
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  nsProbingState GetState(void) {return mState;};
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {};
  PRBool KeepEnglishLetters() {return mModel->keepEnglishLetter;};

protected:
  nsProbingState mState;
  SequenceModel *mModel;
  unsigned char mLastOrder;

  PRUint32 mTotalSeqs;
  PRUint32 mNegativeSeqs;

  PRUint32 mTotalChar;
  PRUint32 mFreqChar;
};


extern SequenceModel Koi8rModel;
extern SequenceModel Win1251Model;
extern SequenceModel Latin5Model;
extern SequenceModel MacCyrillicModel;
extern SequenceModel Ibm866Model;
extern SequenceModel Ibm855Model;
extern SequenceModel Latin7Model;
extern SequenceModel Win1253Model;
extern SequenceModel Latin5BulgarianModel;
extern SequenceModel Win1251BulgarianModel;
extern SequenceModel Latin2HungarianModel;
extern SequenceModel Win1250HungarianModel;

#endif /* nsSingleByteCharSetProber_h__ */

