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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef nsILineBreakerFactory_h__
#define nsILineBreakerFactory_h__


#include "nsISupports.h"
#include "nsILineBreaker.h"
#include "nsString.h"

#include "nscore.h"


// {E86B3377-BF89-11d2-B3AF-00805F8A6670}
#define NS_ILINEBREAKERFACTORY_IID \
{ 0xe86b3377, 0xbf89, 0x11d2, \
    { 0xb3, 0xaf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

class nsILineBreakerFactory : public nsISupports
{
public:

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ILINEBREAKERFACTORY_IID)
  NS_IMETHOD GetBreaker(nsString& aParam, nsILineBreaker** breaker) = 0;
};


#endif  /* nsILineBreakerFactory_h__ */
