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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsIRDFNodeList_h__
#define nsIRDFNodeList_h__

#include "nsISupports.h"

// {661D1971-5CD2-11d3-BE36-00104BDE6048}
#define NS_IRDFNODELIST_IID \
{ 0x661d1971, 0x5cd2, 0x11d3, { 0xbe, 0x36, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

class nsIRDFNodeList : public nsISupports
{
public:
    static const nsIID& GetIID() { static nsIID iid = NS_IRDFNODELIST_IID; return iid; }

    NS_IMETHOD AppendNode(nsIDOMNode* aNode) = 0;
    NS_IMETHOD RemoveNode(nsIDOMNode* aNode) = 0;
};


#endif // nsIRDFNodeList_h__

