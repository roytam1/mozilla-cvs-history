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

#ifndef nsBIG5ToUnicode_h___
#define nsBIG5ToUnicode_h___

#include "nsUCvTWSupport.h"

//----------------------------------------------------------------------
// Class nsBIG5ToUnicode [declaration]

/**
 * A character set converter from BIG5 to Unicode.
 *
 * @created         06/Apr/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsBIG5ToUnicode : public nsMultiTableDecoderSupport
{
public:

  /**
   * Class constructor.
   */
  nsBIG5ToUnicode();

  /**
   * Static class constructor.
   */
  static nsresult CreateInstance(nsISupports **aResult);

protected:

  //--------------------------------------------------------------------
  // Subclassing of nsDecoderSupport class [declaration]

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
};

#endif /* nsBIG5ToUnicode_h___ */
