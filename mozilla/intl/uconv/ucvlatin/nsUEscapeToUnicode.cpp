/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


#include "nsUEscapeToUnicode.h"
#include "nsUCvLatinSupport.h"

//----------------------------------------------------------------------
// Class nsUEscapeToUnicode [declaration]

class nsUEscapeToUnicode : public nsBasicDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsUEscapeToUnicode() { Reset();};

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 
  NS_IMETHOD Reset();
  /**
   * Static class constructor.
   */
  static nsresult CreateInstance(nsISupports **aResult);

protected:
  //--------------------------------------------------------------------
  // Subclassing of nsDecoderSupport class [declaration]

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
private:
  PRUint8 mState;
  PRUint16 mData;
  PRUnichar mBuffer[2];
  PRUint32  mBufferLen;
};

nsresult nsUEscapeToUnicode::CreateInstance(nsISupports ** aResult) 
{
  *aResult = new nsUEscapeToUnicode();
  return (*aResult == NULL)? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

//----------------------------------------------------------------------
// Subclassing of nsTableDecoderSupport class [implementation]

NS_IMETHODIMP nsUEscapeToUnicode::GetMaxLength(const char * aSrc, 
                                            PRInt32 aSrcLength, 
                                            PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + mBufferLen);
  return NS_OK;
}
NS_IMETHODIMP nsUEscapeToUnicode::Convert(
      const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength)
{
  const char* srcPtr = aSrc;
  const char* srcEnd = aSrc + *aSrcLength;
  PRUnichar* destPtr = aDest;
  PRUnichar* destEnd = aDest + *aDestLength;
  while((mBufferLen >0) && (destPtr < destEnd))
    *destPtr++ = mBuffer[--mBufferLen];
  
  for(;(srcPtr < srcEnd) && (destPtr < destEnd); srcPtr++)
  {
      switch(mState)
      {
        case 0:
          if('\\' == *srcPtr) {
             mState++;
          } else {
             if(0x80 == (0x80 & *srcPtr))
             {
               *destPtr++ = (PRUnichar)0xFFFD;
             } else {
               *destPtr++ = (PRUnichar)*srcPtr;
             }
          }
          break;
        case 1: // got a '\'   
          if(('u' == *srcPtr)  || ('U' == *srcPtr)) {
             mState++;
             mData=0;
          } else {
             mState =0;
             *destPtr++ = (PRUnichar)*srcPtr;
          }
          break;
        case 2:  // got \u
        case 3:  // got \u1
        case 4:  // got \u12
        case 5:  // got \u123
             if(('0' <= *srcPtr) && (*srcPtr <='9')) {
                mData = (mData << 4) | (*srcPtr - '0');
                mState++;
             } else if(('a' <= *srcPtr) && (*srcPtr <='f')) {
                mData = (mData << 4) | ((*srcPtr - 'a') + 0x0a);
                mState++;
             } else if(('A' <= *srcPtr) && (*srcPtr <='F')) {
                mData = (mData << 4) | ((*srcPtr - 'A') + 0x0a);
                mState++;
             } else {
                // if we got  \u , \u2 , \u34
                if((destPtr+2) >= destEnd) {
                   // notice the data in mBuffer is stored in reversed order
                   mBufferLen = 2;
                   mBuffer[1] = (PRUnichar) mData;
                   mBuffer[0] = (PRUnichar)*srcPtr;
                   mState=0;
                   goto error;
                }
                *destPtr++ = (PRUnichar) mData;
                *destPtr++ = (PRUnichar)*srcPtr;
                mState=0;
             }
             if(6==mState) // got \u1234
             {
                *destPtr++ = (PRUnichar) mData;
                mState = 0;
             }
          break;
        default:
          break;
      };
  }
  
  *aDestLength = destPtr - aDest;
  *aSrcLength =  srcPtr  - aSrc; 
  return NS_OK;

error:
  *aDestLength = destPtr - aDest;
  *aSrcLength =  srcPtr  - aSrc; 
  return  NS_OK_UDEC_MOREOUTPUT;
}

NS_IMETHODIMP nsUEscapeToUnicode::Reset()
{
  mState =0;
  mData=0;
  mBufferLen =0;
  return NS_OK;
}
nsresult NEW_UEscapeToUnicode(nsISupports **aResult)
{
   *aResult = new nsUEscapeToUnicode();
   return (NULL == *aResult) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}
