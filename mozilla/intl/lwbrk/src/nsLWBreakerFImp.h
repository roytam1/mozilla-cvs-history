/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsLWBreakerFImp_h__
#define nsLWBreakerFImp_h__


#include "nsCom.h"
#include "nsISupports.h"

#include "nsILineBreakerFactory.h"
#include "nsILineBreaker.h"
#include "nsIWordBreakerFactory.h"
#include "nsIWordBreaker.h"


class nsLWBreakerFImp : public nsILineBreakerFactory , nsIWordBreakerFactory
{
  NS_DECL_ISUPPORTS

public:
  
  nsLWBreakerFImp();
  virtual ~nsLWBreakerFImp();
  
  NS_IMETHOD GetBreaker(nsString& aParam, nsILineBreaker** breaker);
  NS_IMETHOD GetBreaker(nsString& aParam, nsIWordBreaker** breaker);
};


#endif  /* nsLWBreakerFImp_h__ */
