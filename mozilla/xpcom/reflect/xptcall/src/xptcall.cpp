/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/* entry point wrappers. */

#if defined(XP_MAC)
#pragma export on
#endif

#include "xptcprivate.h"

// This method is never called and is only here so the compiler
// will generate a vtbl for this class. 
// *Needed by the Irix implementation.*
NS_IMETHODIMP nsXPTCStubBase::QueryInterface(REFNSIID aIID,
                                             void** aInstancePtr)
{
   NS_ASSERTION(0,"wowa! nsXPTCStubBase::QueryInterface called");
   return NS_ERROR_FAILURE;
}
#if defined(XP_MAC)
#pragma export off
#endif

void
xptc_dummy2()
{
}
