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
 * Author: Aaron Leventhal (aaronl@netscape.com)
 * Contributor(s): 
 */

#ifndef _nsHTMLTableAccessible_H_
#define _nsHTMLTableAccessible_H_

#include "nsAccessible.h"

class nsHTMLTableCellAccessible : public nsHTMLBlockAccessible
{
public:
  nsHTMLTableCellAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetAccRole(PRUint32 *_retval); 
};


class nsHTMLTableAccessible : public nsHTMLBlockAccessible
{
public:
  nsHTMLTableAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetAccRole(PRUint32 *_retval); 
};

#endif  
