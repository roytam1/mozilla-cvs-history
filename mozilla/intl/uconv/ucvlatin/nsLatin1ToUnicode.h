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

#ifndef nsLatin1ToUnicode_h___
#define nsLatin1ToUnicode_h___

#include "nsIFactory.h"
#include "nsICharsetConverterInfo.h"

//----------------------------------------------------------------------
// Class nsLatin1ToUnicodeFactory [declaration]

/**
 * Factory class for the nsLatin1ToUnicode objects.
 * 
 * @created         08/Dec/1998
 * @author  Catalin Rotaru [CATA]
 */
class nsLatin1ToUnicodeFactory : public nsIFactory, 
public nsICharsetConverterInfo
{
  NS_DECL_ISUPPORTS

public:

  /**
   * Class constructor.
   */
  nsLatin1ToUnicodeFactory();

  /**
   * Class destructor.
   */
  ~nsLatin1ToUnicodeFactory();

  //--------------------------------------------------------------------
  // Interface nsIFactory [declaration]

  NS_IMETHOD CreateInstance(nsISupports *aDelegate, const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

  //--------------------------------------------------------------------
  // Interface nsICharsetConverterInfo [declaration]

  NS_IMETHOD GetCharsetSrc(char ** aCharset);
  NS_IMETHOD GetCharsetDest(char ** aCharset);
};



#endif /* nsLatin1ToUnicode_h___ */
