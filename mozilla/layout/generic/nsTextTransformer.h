/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#ifndef nsTextTransformer_h___
#define nsTextTransformer_h___

#include "nsTextFragment.h"
#include "nsISupports.h"

class nsIContent;
class nsIFrame;
class nsTextRun;
class nsILineBreaker;
class nsIWordBreaker;

#define NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE 100

/**
 * This object manages the transformation of text:
 *
 * <UL>
 * <LI>whitespace compression
 * <LI>capitalization
 * <LI>lowercasing
 * <LI>uppercasing
 * </UL>
 *
 * Note that no transformations are applied that would impact word
 * breaking (like mapping &nbsp; into space, for example). In
 * addition, this logic will not strip leading or trailing whitespace
 * (across the entire run of text; leading whitespace can be skipped
 * for a frames text because of whitespace compression).
 */
class nsTextTransformer {
public:
  // Note: The text transformer does not hold a reference to the line
  // breaker and work breaker objects
  nsTextTransformer(nsILineBreaker* aLineBreaker,
                    nsIWordBreaker *aWordBreaker);

  ~nsTextTransformer();

  /**
   * Initialize the text transform. This is when the transformation
   * occurs. Subsequent calls to GetTransformedTextFor will just
   * return the result of the single transformation.
   */
  nsresult Init(nsIFrame* aFrame,
                nsIContent* aContent,
                PRInt32 aStartingOffset);

  PRInt32 GetContentLength() const {
    return mContentLength;
  }

  PRUnichar* GetNextWord(PRBool aInWord,
                         PRInt32& aWordLenResult,
                         PRInt32& aContentLenResult,
                         PRBool& aIsWhitespaceResult,
                         PRBool aForLineBreak = PR_TRUE);

  PRUnichar* GetPrevWord(PRBool aInWord,
                               PRInt32& aWordLenResult,
                               PRInt32& aContentLenResult,
                               PRBool& aIsWhitespaceResult,
                               PRBool aForLineBreak = PR_TRUE);
  PRBool HasMultibyte() const {
    return mHasMultibyte;
  }

  PRUnichar* GetWordBuffer() {
    return mBuffer;
  }

  PRInt32 GetWordBufferLength() const {
    return mBufferLength;
  }

protected:
  PRBool GrowBuffer(PRBool aForNextWord = PR_TRUE);

  PRUnichar* mBuffer;
  PRInt32 mBufferLength;
  PRBool mHasMultibyte;

  PRInt32 mContentLength;
  PRInt32 mStartingOffset;
  PRInt32 mOffset;

  const nsTextFragment* mFrag;
  PRInt32 mCurrentFragOffset;

  PRUint8 mTextTransform;
  PRUint8 mPreformatted;

  nsILineBreaker* mLineBreaker;  // does NOT hold reference
  nsIWordBreaker* mWordBreaker;  // does NOT hold reference

  PRUnichar mAutoWordBuffer[NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE];
};

#endif /* nsTextTransformer_h___ */
