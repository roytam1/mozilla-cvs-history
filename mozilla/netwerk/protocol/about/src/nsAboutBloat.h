/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsAboutBloat_h__
#define nsAboutBloat_h__

#include "nsIAboutModule.h"

class nsAboutBloat : public nsIAboutModule 
{
public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIABOUTMODULE

    nsAboutBloat() { NS_INIT_REFCNT(); }
    virtual ~nsAboutBloat() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
};

#define NS_ABOUT_BLOAT_MODULE_CID                    \
{ /* f9666720-801f-11d3-9399-00104ba0fd40 */         \
    0xf9666720,                                      \
    0x801f,                                          \
    0x11d3,                                          \
    {0x93, 0x99, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

#endif // nsAboutBloat_h__
