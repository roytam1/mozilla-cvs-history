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

#ifndef nsJARProtocolHandler_h___
#define nsJARProtocolHandler_h___

#include "nsIProtocolHandler.h"

#define NS_JARPROTOCOLHANDLER_CID                  \
{ /* 9e3b6c90-2f75-11d3-8cd0-0060b0fc14a3 */         \
    0x9e3b6c90,                                      \
    0x2f75,                                          \
    0x11d3,                                          \
    {0x8c, 0xd0, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsJARProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    // nsIProtocolHandler methods:
    NS_IMETHOD GetScheme(char * *aScheme);
    NS_IMETHOD GetDefaultPort(PRInt32 *aDefaultPort);
    NS_IMETHOD MakeAbsolute(const char *aRelativeSpec, nsIURI *aBaseURI,
                            char **_retval);
    NS_IMETHOD NewURI(const char *aSpec, nsIURI *aBaseURI,
                      nsIURI **_retval);
    NS_IMETHOD NewChannel(const char* verb, nsIURI* url,
                          nsIEventSinkGetter *eventSinkGetter,
                          nsIChannel **_retval);

    // nsJARProtocolHandler methods:
    nsJARProtocolHandler();
    virtual ~nsJARProtocolHandler();

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsresult Init();

protected:
};

#endif /* nsJARProtocolHandler_h___ */
