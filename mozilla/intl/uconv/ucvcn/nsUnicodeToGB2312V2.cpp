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

#include "nsUnicodeToGB2312V2.h"
#include "nsUCvCnDll.h"

#define _GBKU_TABLE_		// use a shared table
#include "gbku.h"

//----------------------------------------------------------------------
// Class nsUnicodeToGB2312V2 [implementation]

#define TRUE 1
#define FALSE 0

nsUnicodeToGB2312V2::nsUnicodeToGB2312V2()
{
  PRUint8 left, right;
  PRUnichar unicode;
  PRUint16 i;

  for ( i=0; i<MAX_GBK_LENGTH; i++ )
    {
      
      left =  ( i / 0x00BF + 0x0081);
      right = ( i % 0x00BF+ 0x0040);
      unicode = GBKToUnicodeTable[i];
      
      // to reduce size of UnicodeToGBKTable, we only do direct unicode to GB 
      // table mapping between unicode 0x4E00 and 0xA000. Others by searching
      // GBKToUnicodeTable. There is a trade off between memory usage and speed.
      if ( (unicode >= 0x4E00 ) && ( unicode <= 0xA000 ))
        {
          unicode -= 0x4E00; 
          UnicodeToGBKTable[unicode].leftbyte = left;
          UnicodeToGBKTable[unicode].rightbyte = right;
        }
    } 
}

NS_IMETHODIMP nsUnicodeToGB2312V2::ConvertNoBuff(const PRUnichar * aSrc, 
                                                 PRInt32 * aSrcLength, 
                                                 char * aDest, 
                                                 PRInt32 * aDestLength)
{
	PRInt32 i=0;
	PRInt32 iSrcLength = *aSrcLength;
  DByte *pDestDBCode;
  DByte *pSrcDBCode; 
	PRInt32 iDestLength = 0;
  PRUnichar unicode;
  PRUint8 left, right;
  
	PRUnichar *pSrc = (PRUnichar *)aSrc;
	pDestDBCode = (DByte *)aDest;
  
  for (i=0;i< iSrcLength;i++)
	{
		pDestDBCode = (DByte *)aDest;
    
    unicode = *pSrc;
		//if unicode's hi byte has something, it is not ASCII, must be a GB
		if( unicode & 0xff00 )
      {
        // To reduce the UnicodeToGBKTable size, we only use direct table mapping 
        // for unicode between 0x4E00 - 0xA000
        if ( (unicode >= 0x4E00 ) && ( unicode <= 0xA000 ) )
          {
            unicode -= 0x4E00;
            pDestDBCode->leftbyte =  UnicodeToGBKTable[unicode].leftbyte ;                
            pDestDBCode->rightbyte = UnicodeToGBKTable[unicode].rightbyte ;  
          }
        else  
          {
            // other ones we search in GBK to Unicode table
            for ( i=0; i<MAX_GBK_LENGTH; i++)
              {
                if ( unicode  == GBKToUnicodeTable[i] )
                  {
                    //this manipulation handles the little endian / big endian issues
                    left = (char) ( i / 0x00BF + 0x0081) | 0x80  ;
                    right = (char) ( i % 0x00BF+ 0x0040) | 0x80;
                    pDestDBCode->leftbyte = left;  
                    pDestDBCode->rightbyte = right;  
                  }
              }
          } 
        
        //	UnicodeToGBK( *pSrc, pDestDBCode);
        aDest += 2;	// increment 2 bytes
        pDestDBCode = (DByte *)aDest;
        iDestLength +=2;
      }
		else
      {
        // this is an ASCII
        pSrcDBCode = (DByte *)pSrc;
        *aDest = pSrcDBCode->leftbyte;
        aDest++; // increment 1 byte
        iDestLength +=1;
      }
		pSrc++;	 // increment 2 bytes
    
		if ( iDestLength >= (*aDestLength) )
      {
        break;
      }
	}
  
	*aDestLength = iDestLength;
	*aSrcLength = i;
  
  return NS_OK;
}


nsresult nsUnicodeToGB2312V2::CreateInstance(nsISupports ** aResult) 
{
  nsIUnicodeEncoder *p = new nsUnicodeToGB2312V2();
  if(p) {
    *aResult = p;
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

//----------------------------------------------------------------------
// Subclassing of nsTableEncoderSupport class [implementation]

NS_IMETHODIMP nsUnicodeToGB2312V2::GetMaxLength(const PRUnichar * aSrc, 
                                              PRInt32 aSrcLength,
                                              PRInt32 * aDestLength)
{
  *aDestLength = 2 * aSrcLength;
  return NS_OK;
}



#define SET_REPRESENTABLE(info, c)  (info)[(c) >> 5] |= (1L << ((c) & 0x1f))

NS_IMETHODIMP nsUnicodeToGB2312V2::FillInfo(PRUint32 *aInfo)
{
	PRUint16 i,j, k;
  PRUnichar SrcUnicode;

  // aInfo should already been initialized as 2048 element array of 32 bit by caller
  NS_ASSERTION( sizeof(aInfo[0]) == 4, "aInfo size incorrect" );

  // valid GBK rows are in 0x81 to 0xFE
  for ( i=0x0081;i<0x00FF;i++) 
    {
      // HZ and GB2312 starts at row 0x21|0x80 = 0xA1
      if ( i < 0xA1 )
        continue;

      // valid GBK columns are in 0x41 to 0xFE
      for( j=0x0041;j<0x00FF;j++)
        {
          //HZ and GB2312 starts at col 0x21 | 0x80 = 0xA1
          if ( j < 0xA1 )
            continue;
          
          // k is index in GBKU.H table
          k = (i - 0x0081)*(0x00FE - 0x0080)+(j-0x0041);
          
          SrcUnicode = GBKToUnicodeTable[k];
          if (( SrcUnicode != 0xFFFF ) && (SrcUnicode != 0xFFFD) )
            {
              SET_REPRESENTABLE(aInfo, SrcUnicode);
            }            
        }
    }                   
  
  return NS_OK;
}
