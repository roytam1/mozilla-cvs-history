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

#ifndef nsIContentConnector_h___
#define nsIContentConnector_h___

#include "nsGUIEvent.h"

class nsIContent;

// {FC41CD61-796E-11d2-BF86-00105A1B0627}
#define NS_ICONTENTCONNECTOR_IID      \
{ 0xfc41cd61, 0x796e, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

class nsIContentConnector : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ICONTENTCONNECTOR_IID; return iid; }

  NS_IMETHOD SetContentRoot(nsIContent* pContent) = 0;

  NS_IMETHOD_(nsEventStatus) HandleEvent(nsGUIEvent *aEvent) = 0;
};

#endif /* nsIContentConnector_h___ */

