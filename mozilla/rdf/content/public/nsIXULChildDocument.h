/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsIXULChildDocument_h__
#define nsIXULChildDocument_h__

#include "nsString.h"

// {7B75C621-D641-11d2-BF86-00105A1B0627}
#define NS_IXULCHILDDOCUMENT_IID \
{ 0x7b75c621, 0xd641, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

class nsIContentViewerContainer;

class nsIXULChildDocument: public nsISupports {
public:
    static const nsIID& GetIID() { static nsIID iid = NS_IXULCHILDDOCUMENT_IID; return iid; }

    NS_IMETHOD SetFragmentRoot(nsIContent* aContent) = 0;
    NS_IMETHOD GetFragmentRoot(nsIContent** aContent) = 0;
};

#endif // nsIXULChildDocument_h__
