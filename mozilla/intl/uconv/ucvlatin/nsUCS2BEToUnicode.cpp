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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsUCConstructors.h"
#include "nsUCS2BEToUnicode.h"
#include "nsUCvLatinDll.h"
#include <string.h>
#include "prtypes.h"
//----------------------------------------------------------------------
// Global functions and data [declaration]

static const PRUint16 g_UCS2BEMappingTable[] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0xFFFF, 0x0000
};

static const PRInt16 g_UCS2BEShiftTable[] =  {
  0, u2BytesCharset, 
  ShiftCell(0,       0, 0, 0, 0, 0, 0, 0), 
};

//----------------------------------------------------------------------
// Class nsUCS2BEToUnicode [implementation]

nsUCS2BEToUnicode::nsUCS2BEToUnicode() 
: nsTableDecoderSupport((uShiftTable*) &g_UCS2BEShiftTable, 
                        (uMappingTable*) &g_UCS2BEMappingTable, 0)
{
}

//----------------------------------------------------------------------
// Subclassing of nsTableDecoderSupport class [implementation]

NS_IMETHODIMP nsUCS2BEToUnicode::GetMaxLength(const char * aSrc, 
                                            PRInt32 aSrcLength, 
                                            PRInt32 * aDestLength)
{
  if(0 == (aSrcLength % 2)) 
  {
    *aDestLength = aSrcLength / 2;
    return NS_OK_UENC_EXACTLENGTH;
  } else {
    *aDestLength = (aSrcLength  + 1) / 2;
    return NS_OK;
  }
}


class nsUTF16SameEndianToUnicode : public nsBasicDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsUTF16SameEndianToUnicode() { Reset();};

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 
  NS_IMETHOD Reset();

protected:
  //--------------------------------------------------------------------
  // Subclassing of nsDecoderSupport class [declaration]

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
protected:
  PRUint8 mState;
  PRUint8 mData;
};


//----------------------------------------------------------------------
// Subclassing of nsTableDecoderSupport class [implementation]

NS_IMETHODIMP nsUTF16SameEndianToUnicode::GetMaxLength(const char * aSrc, 
                                            PRInt32 aSrcLength, 
                                            PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + ((1==mState)?1:0))/2;
  return NS_OK;
}
NS_IMETHODIMP nsUTF16SameEndianToUnicode::Convert(
      const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength)
{
  const char* src = aSrc;
  const char* srcEnd = aSrc + *aSrcLength;
  PRUnichar* dest = aDest;
  PRUnichar* destEnd = aDest + *aDestLength;
  PRInt32 copybytes;

  if(2 == mState) // first time called
  {
    // eleminate BOM
    if(0xFEFF == *((PRUnichar*)src)) {
      src+=2;
    } else if(0xFFFE == *((PRUnichar*)src)) {
      *aSrcLength=0;
      *aDestLength=0;
      return NS_ERROR_ILLEGAL_INPUT;
    }  
    mState=0;
  }

  if((1 == mState) && (src < srcEnd))
  {
    if(dest >= destEnd)
      goto error;
    if(src>=srcEnd)
      goto done;
    char tmpbuf[2];
    PRUnichar * up = (PRUnichar*) &tmpbuf[0];
    tmpbuf[0]= mData;
    tmpbuf[1]= *src++;
    *dest++ = *up;
  }
  
  copybytes = (destEnd-dest)*2;
  if(copybytes > (0xFFFE & (srcEnd-src)))
      copybytes = 0xFFFE & (srcEnd-src);
  memcpy(dest,src,copybytes);
  src +=copybytes;
  dest +=(copybytes/2);
  if(srcEnd==src)  {
     mState = 0;
  } else if(1 == (srcEnd-src) ) {
     mState = 1;
     mData  = *src++;
  } else  {
     goto error;
  }
  
done:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return NS_OK;

error:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return  NS_OK_UDEC_MOREOUTPUT;
}

NS_IMETHODIMP nsUTF16SameEndianToUnicode::Reset()
{
  mState =2;
  mData=0;
  return NS_OK;
}
class nsUTF16DiffEndianToUnicode : public nsUTF16SameEndianToUnicode
{
public:
  nsUTF16DiffEndianToUnicode() {};
  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 
};

NS_IMETHODIMP nsUTF16DiffEndianToUnicode::Convert(
      const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength)
{
  if(2 == mState) // first time called
  {
    if(0xFFFE == *((PRUnichar*)aSrc)) {
      aSrc+=2;
      *aSrcLength-=2;
    } else if(0xFEFF == *((PRUnichar*)aSrc)) {
      // eleminate BOM
      *aSrcLength=0;
      *aDestLength=0;
      return NS_ERROR_ILLEGAL_INPUT;
    }  
    mState=0;
  }
  nsresult res = nsUTF16SameEndianToUnicode::Convert(aSrc,aSrcLength, aDest, aDestLength);
  PRInt32 i;

  // copy
  char* p;
  p=(char*)aDest;
  for(i=*aDestLength; i>0 ;i--,p+=2)
  {
     char tmp = *(p+1);
     *(p+1) = *p;
     *(p)= tmp;
  }

  return res;
}

static nsresult UTF16ConvertToUnicode(PRUint8& aState, PRUint8& aData, const char * aSrc, PRInt32 * aSrcLength, PRUnichar * aDest, PRInt32 * aDestLength)
{
  const char* src = aSrc;
  const char* srcEnd = aSrc + *aSrcLength;
  PRUnichar* dest = aDest;
  PRUnichar* destEnd = aDest + *aDestLength;
  PRInt32 copybytes;

  if(2 == aState) // first time called
  {
    // eleminate BOM
    if(0xFEFF == *((PRUnichar*)src)) {
      src+=2;
    } else if(0xFFFE == *((PRUnichar*)src)) {
      *aSrcLength=0;
      *aDestLength=0;
      return NS_ERROR_ILLEGAL_INPUT;
    }  
    aState=0;
  }

  if((1 == aState) && (src < srcEnd))
  {
    if(dest >= destEnd)
      goto error;
    if(src>=srcEnd)
      goto done;
    char tmpbuf[2];
    PRUnichar * up = (PRUnichar*) &tmpbuf[0];
    tmpbuf[0]= aData;
    tmpbuf[1]= *src++;
    *dest++ = *up;
  }
  
  copybytes = (destEnd-dest)*2;
  if(copybytes > (0xFFFE & (srcEnd-src)))
      copybytes = 0xFFFE & (srcEnd-src);
  memcpy(dest,src,copybytes);
  src +=copybytes;
  dest +=(copybytes/2);
  if(srcEnd==src)  {
     aState = 0;
  } else if(1 == (srcEnd-src) ) {
     aState = 1;
     aData  = *src++;
  } else  {
     goto error;
  }
  
done:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return NS_OK;

error:
  *aDestLength = dest - aDest;
  *aSrcLength =  src  - aSrc; 
  return  NS_OK_UDEC_MOREOUTPUT;
}

NS_IMETHODIMP nsUTF16BEToUnicode::Reset()
{
  mState =2;
  mData=0;
  return NS_OK;
}

NS_IMETHODIMP nsUTF16BEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength)
{
#ifdef IS_LITTLE_ENDIAN
    // process nsUTF16DiffEndianToUnicode
    if(2 == mState) // first time called
    {
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        aSrc+=2;
        *aSrcLength-=2;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        // eleminate BOM
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }  
      mState=0;
    }
#endif

  nsresult res = UTF16ConvertToUnicode(mState, mData, aSrc, aSrcLength, aDest, aDestLength);

#ifdef IS_LITTLE_ENDIAN
    // process nsUTF16DiffEndianToUnicode
    PRInt32 i;

    // copy
    char* p;
    p=(char*)aDest;
    for(i=*aDestLength; i>0 ;i--,p+=2)
    {
       char tmp = *(p+1);
       *(p+1) = *p;
       *(p)= tmp;
    }
#endif
  return res;
}

NS_IMETHODIMP nsUTF16BEToUnicode::GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + ((1==mState)?1:0))/2;
  return NS_OK;
}

NS_IMETHODIMP nsUTF16LEToUnicode::Reset()
{
  mState =2;
  mData=0;
  return NS_OK;
}

NS_IMETHODIMP nsUTF16LEToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength)
{
#ifdef IS_BIG_ENDIAN
    // process nsUTF16DiffEndianToUnicode
    if(2 == mState) // first time called
    {
      if(0xFFFE == *((PRUnichar*)aSrc)) {
        aSrc+=2;
        *aSrcLength-=2;
      } else if(0xFEFF == *((PRUnichar*)aSrc)) {
        // eleminate BOM
        *aSrcLength=0;
        *aDestLength=0;
        return NS_ERROR_ILLEGAL_INPUT;
      }  
      mState=0;
    }
#endif
    
  nsresult res = UTF16ConvertToUnicode(mState, mData, aSrc, aSrcLength, aDest, aDestLength);

#ifdef IS_BIG_ENDIAN
    // process nsUTF16DiffEndianToUnicode
    PRInt32 i;

    // copy
    char* p;
    p=(char*)aDest;
    for(i=*aDestLength; i>0 ;i--,p+=2)
    {
       char tmp = *(p+1);
       *(p+1) = *p;
       *(p)= tmp;
    }
#endif
  return res;
}

NS_IMETHODIMP nsUTF16LEToUnicode::GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength)
{
  *aDestLength = (aSrcLength + ((1==mState)?1:0))/2;
  return NS_OK;
}
