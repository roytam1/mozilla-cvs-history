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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
 
#include "nsISupports.h"

class nsIEditor;

#define NS_IGFXTEXTCONTROLFRAME_IID \
{/* d3ea33ea-9e00-11d3-bccc-0060b0fc76bd*/ \
0xd3ea33ea, 0x9e00, 0x11d3, \
{0xbc, 0xcc, 0x0, 0x60, 0xb0, 0xfc, 0x76, 0xbd} }

class nsIGfxTextControlFrame : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IGFXTEXTCONTROLFRAME_IID; return iid; }

	NS_IMETHOD GetEditor(nsIEditor **aEditor) = 0;
};
