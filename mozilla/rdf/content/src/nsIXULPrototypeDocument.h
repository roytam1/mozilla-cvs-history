/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/*



 */

#ifndef nsIXULPrototypeDocument_h__
#define nsIXULPrototypeDocument_h__

#include "nsISupports.h"

class nsIURI;
class nsIStyleSheet;
class nsVoidArray;
struct nsXULPrototypeElement;

// {187A63D0-8337-11d3-BE47-00104BDE6048}
#define NS_IXULPROTOTYPEDOCUMENT_IID \
{ 0x187a63d0, 0x8337, 0x11d3, { 0xbe, 0x47, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }


class nsIXULPrototypeDocument : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IXULPROTOTYPEDOCUMENT_IID);

    /**
     * Retrieve the URI of the document
     */
    NS_IMETHOD GetURI(nsIURI** aResult) = 0;

    /**
     * Retrieve the root XULPrototype element of the document.
     */
    NS_IMETHOD GetRootElement(nsXULPrototypeElement** aResult) = 0;
    NS_IMETHOD SetRootElement(nsXULPrototypeElement* aElement) = 0;

    NS_IMETHOD AddStyleSheet(nsIStyleSheet* aStyleSheet) = 0;
    NS_IMETHOD GetStyleSheets(nsVoidArray& aResult) = 0;

    NS_IMETHOD AddOverlayReference(nsIURI* aURI) = 0;
    NS_IMETHOD GetOverlayReferences(nsVoidArray& aResult) = 0;
};

#endif // nsIXULPrototypeDocument_h__
