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

#ifndef _nsHTMLTextAccessible_H_
#define _nsHTMLTextAccessible_H_

#include "nsGenericAccessible.h"

class nsIWeakReference;
class nsITextControlFrame;

class nsHTMLTextAccessible : public nsLeafDOMAccessible
{

public:
  nsHTMLTextAccessible(nsIPresShell* aShell, nsIDOMNode* aDomNode);
  NS_IMETHOD GetAccName(PRUnichar **_retval); 
  NS_IMETHOD GetAccRole(PRUnichar **_retval); 
  NS_IMETHOD GetAccState(PRUint32 *_retval);
  NS_IMETHOD GetAccDefaultAction(PRUnichar **_retval);
  NS_IMETHOD AccDoDefaultAction();

protected:
  nsCOMPtr<nsIDOMNode> mDomNode;
  PRBool IsALink();
  PRBool mIsALinkCached;  // -1 = unknown, 0 = not a link, 1 = is a link
  nsCOMPtr<nsIContent> mLinkContent;
  PRBool mIsLinkVisited;
};

#endif  
