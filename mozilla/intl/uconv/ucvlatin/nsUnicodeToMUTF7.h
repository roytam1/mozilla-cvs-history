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

#ifndef nsUnicodeToMUTF7_h___
#define nsUnicodeToMUTF7_h___

#include "nsUnicodeToUTF7.h"

//----------------------------------------------------------------------
// Class nsUnicodeToMUTF7 [declaration]

/**
 * A character set converter from Unicode to Modified UTF-7.
 *
 * @created         18/May/1999
 * @author  Catalin Rotaru [CATA]
 */
class nsUnicodeToMUTF7 : public nsBasicUTF7Encoder
{
public:

  /**
   * Class constructor.
   */
  nsUnicodeToMUTF7();

  /**
   * Static class constructor.
   */
  static nsresult CreateInstance(nsISupports **aResult);
};

#endif /* nsUnicodeToMUTF7_h___ */
