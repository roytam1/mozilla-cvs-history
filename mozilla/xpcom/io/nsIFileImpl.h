/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Doug Turner <dougt@netscape.com>
 */

#ifndef _NSIFILEIMPL_H_
#define _NSIFILEIMPL_H_

#define NS_IFILEIMPL_CID {0x2e23e220, 0x60be, 0x11d3, {0x8c, 0x4a, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

// nsXPComInit needs to know about how we are implmented,
// so here we will export it.  Other users should not depend
// on this.

#ifdef XP_PC
#include "nsIFileImplWin.h"
#else
#error NOT_IMPLEMENTED
#endif

#endif