/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsJARProtocolHandler_h___
#define nsJARProtocolHandler_h___

#include "nsIJARProtocolHandler.h"
#include "nsIProtocolHandler.h"
#include "nsIJARURI.h"
#include "nsIZipReader.h"
#include "nsCOMPtr.h"

#define NS_JARPROTOCOLHANDLER_CID					 \
{ /* 0xc7e410d4-0x85f2-11d3-9f63-006008a6efe9 */     \
    0xc7e410d4,                                      \
    0x85f2,                                          \
    0x11d3,                                          \
    {0x9f, 0x63, 0x00, 0x60, 0x08, 0xa6, 0xef, 0xe9} \
}


class nsJARProtocolHandler : public nsIJARProtocolHandler
{
public:
    NS_DECL_ISUPPORTS
	NS_DECL_NSIPROTOCOLHANDLER
	NS_DECL_NSIJARPROTOCOLHANDLER

    // nsJARProtocolHandler methods:
    nsJARProtocolHandler();
    virtual ~nsJARProtocolHandler();

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsresult Init();

protected:
    nsCOMPtr<nsIZipReaderCache> mJARCache;
};

#endif /* nsJARProtocolHandler_h___ */
