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

#ifndef nsISocketTransportService_h___
#define nsISocketTransportService_h___

#include "nsISupports.h"

class nsITransport;

// XXX regenerate:
#define NS_ISOCKETTRANSPORTSERVICE_IID               \
{ /* 2355dca0-ea35-11d2-931b-00104ba0fd40 */         \
    0x2355dca0,                                      \
    0xea35,                                          \
    0x11d2,                                          \
    {0x93, 0x1b, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

// XXX regenerate:
#define NS_SOCKETTRANSPORTSERVICE_CID                \
{ /* 2bb2b250-ea35-11d2-931b-00104ba0fd40 */         \
    0x2bb2b250,                                      \
    0xea35,                                          \
    0x11d2,                                          \
    {0x93, 0x1b, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

class nsISocketTransportService : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISOCKETTRANSPORTSERVICE_IID);

    NS_IMETHOD CreateTransport(const char* host, PRInt32 port,
                               nsITransport* *result) = 0;

};

////////////////////////////////////////////////////////////////////////////////

#endif /* nsISocketTransportService_h___ */
