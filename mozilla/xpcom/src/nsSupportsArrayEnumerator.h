/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef nsSupportsArrayEnumerator_h___
#define nsSupportsArrayEnumerator_h___

#include "nsIEnumerator.h"

class nsISupportsArray;

class NS_COM nsSupportsArrayEnumerator : public nsIBidirectionalEnumerator {
public:
  NS_DECL_ISUPPORTS

  nsSupportsArrayEnumerator(nsISupportsArray* array);
  virtual ~nsSupportsArrayEnumerator();

  // nsIEnumerator methods:
  NS_IMETHOD First();
  NS_IMETHOD Next();
  NS_IMETHOD CurrentItem(nsISupports **aItem);
  NS_IMETHOD IsDone();

  // nsIBidirectionalEnumerator methods:
  NS_IMETHOD Last();
  NS_IMETHOD Prev();

protected:
  nsISupportsArray*     mArray;
  PRInt32               mCursor;

};

#endif // __nsSupportsArrayEnumerator_h

