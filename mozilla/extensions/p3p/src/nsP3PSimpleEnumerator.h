/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and imitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2000 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * Contributor(s): IBM Corporation.
 *
 */

#ifndef nsP3PSimpleEnumerator_h__
#define nsP3PSimpleEnumerator_h__

#include "nsCOMPtr.h"
#include "nsISimpleEnumerator.h"

#include "nsISupportsArray.h"


class nsP3PSimpleEnumerator : public nsISimpleEnumerator {
public:
  // nsISupports
  NS_DECL_ISUPPORTS

  // nsISimpleEnumerator methods:
  NS_DECL_NSISIMPLEENUMERATOR

  // nsP3PSimpleEnumerator methods
  nsP3PSimpleEnumerator( nsISupportsArray  *aArray );
  virtual ~nsP3PSimpleEnumerator( );

  static
  NS_METHOD                  GetEnumerator( nsISupportsArray     *aSupportsArray,
                                            nsISimpleEnumerator **aEnumerator );

protected:
  nsCOMPtr<nsISupportsArray> mArray;                  // The nsISupports array being enumerated

  PRUint32                   mIndex;                  // The current position within the array
};

extern
NS_EXPORT NS_METHOD     NS_NewP3PSimpleEnumerator( nsISupportsArray     *aSupportsArray,
                                                   nsISimpleEnumerator **aEnumerator );

#endif                                           /* nsP3PSimpleEnumerator_h__ */
