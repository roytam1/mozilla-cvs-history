/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Original Author: Eric Vaughan (evaughan@netscape.com)
 *
 * Contributor(s): 
 */
#ifndef __nsHTMLSelectAccessible_h__
#define __nsHTMLSelectAccessible_h__

#include "nsAccessible.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"


class nsHTMLSelectAccessible : public nsAccessible
{
public:
  
  nsHTMLSelectAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  NS_IMETHOD GetAccLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetAccFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetAccRole(PRUint32 *_retval);
  NS_IMETHOD GetAccValue(PRUnichar **_retval);
  NS_IMETHOD GetAccChildCount(PRInt32 *_retval);

  virtual ~nsHTMLSelectAccessible() {}
};

#endif
