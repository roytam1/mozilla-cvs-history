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

#ifndef nsIUnicodeDecoder_h___
#define nsIUnicodeDecoder_h___

#include "nscore.h"
#include "nsISupports.h"

// Interface ID for our Unicode Decoder interface
// {B2F178E1-832A-11d2-8A8E-00600811A836}
NS_DECLARE_ID(kIUnicodeDecoderIID,
  0xb2f178e1, 0x832a, 0x11d2, 0x8a, 0x8e, 0x0, 0x60, 0x8, 0x11, 0xa8, 0x36);

#define NS_EXACT_LENGTH \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 11)

#define NS_PARTIAL_MORE_INPUT \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 12)

#define NS_PARTIAL_MORE_OUTPUT \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_UCONV, 13)

#define NS_ERROR_ILLEGAL_INPUT \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_UCONV, 14)

/**
 * Interface for a Converter from a Charset into Unicode.
 *
 * XXX Rethink, rewrite and redoc the error handling.
 * XXX Swap the params order to be: Source, Destination.
 * XXX Rename error value macros
 *
 * @created         23/Nov/1998
 * @author  Catalin Rotaru [CATA]
 */
class nsIUnicodeDecoder : public nsISupports
{
public:

  enum {
    kOnError_Recover,       // on an error, recover and continue
    kOnError_Signal,        // on an error, stop and signal
  };

  /**
   * Converts the data from one Charset to Unicode.
   *
   * About the byte ordering:
   * - For input, if the converter cares (that depends of the charset, for 
   * example a singlebyte will ignore the byte ordering) it should assume 
   * network order. If necessary and requested, we can add a method 
   * SetInputByteOrder() so that the reverse order can be used, too. That 
   * method would have as default the assumed network order.
   * - The output stream is Unicode, having the byte order which is internal
   * for the machine on which the converter is running on.
   *
   * Unless there is not enough output space, this method must consume all the
   * available input data! The eventual incomplete final character data will be
   * stored internally in the converter and used when the method is called 
   * again for continuing the conversion. This way, the caller will not have to
   * worry about managing incomplete input data by mergeing it with the next 
   * buffer.
   *
   * @param aDest       [OUT] the destination data buffer
   * @param aDestOffset [IN] the offset in the destination data buffer
   * @param aDestLength [IN/OUT] the length of the destination data buffer;
   *                    after conversion will contain the number of Unicode
   *                    characters written
   * @param aSrc        [IN] the source data buffer
   * @param aSrcOffset  [IN] the offset in the source data buffer
   * @param aSrcLength  [IN/OUT] the length of source data buffer; after
   *                    conversion will contain the number of bytes read
   * @return            NS_PARTIAL_MORE_INPUT if only a partial conversion was
   *                    done; more input is needed to continue
   *                    NS_PARTIAL_MORE_OUTPUT if only  a partial conversion
   *                    was done; more output space is needed to continue
   *                    NS_ERROR_ILLEGAL_INPUT if an illegal input sequence
   *                    was encountered and the behavior was set to "signal"
   */
  NS_IMETHOD Convert(PRUnichar * aDest, PRInt32 aDestOffset, 
      PRInt32 * aDestLength, const char * aSrc, PRInt32 aSrcOffset, 
      PRInt32 * aSrcLength) = 0;

  /**
   * Finishes the conversion. The converter has the possibility to write some 
   * extra data and flush its final state.
   *
   * @param aDest       [OUT] the destination data buffer
   * @param aDestOffset [IN] the offset in the destination data buffer
   * @param aDestLength [IN/OUT] the length of destination data buffer; after
   *                    conversion it will contain the number of Unicode 
   *                    characters written
   * @return            NS_PARTIAL_MORE_OUTPUT if only a partial finish was
   *                    done; more output space is needed to continue
   */
  NS_IMETHOD Finish(PRUnichar * aDest, PRInt32 aDestOffset, PRInt32 * aDestLength)
      = 0;

  /**
   * Returns a quick estimation of the size of the buffer needed to hold the
   * converted data. Remember: this is an estimation and not necessarily 
   * correct. Its purpose is to help the caller allocate the destination 
   * buffer.
   *
   * @param aSrc        [IN] the source data buffer
   * @param aSrcOffset  [IN] the offset in the source data buffer
   * @param aSrcLength  [IN] the length of source data buffer
   * @param aDestLength [OUT] the needed size of the destination buffer
   * @return            NS_EXACT_LENGTH if an exact length was computed
   */
  NS_IMETHOD Length(const char * aSrc, PRInt32 aSrcOffset, 
      PRInt32 aSrcLength, PRInt32 * aDestLength) = 0;

  /**
   * Resets the charset converter so it may be recycled for a completely 
   * different and urelated buffer of data.
   */
  NS_IMETHOD Reset() = 0;

  /**
   * Specify what to do when an illegal input sequence is encountered.
   * - stop and signal error
   * - recover and continue (default)
   *
   * @param aOrder      [IN] the behavior; taken from the enum
   */
  NS_IMETHOD SetInputErrorBehavior(PRInt32 aBehavior) = 0;
};

#endif /* nsIUnicodeDecoder_h___ */
