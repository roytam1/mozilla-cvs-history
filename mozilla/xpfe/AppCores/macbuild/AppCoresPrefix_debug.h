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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/* Defining the following causes NS_DOM to be defined as NS_EXPORT. */
/* This is a temporary solution to fix export problem with auto     */
/* generated files (nsJSxxx.cpp) who are declared as extern "C"     */
/* NS_DOM. As Mailnews is build into a separate project and need    */
/* NS_InitJSBaseAppCore, NS_InitJSBaseAppCore must be correctly     */
/* exported, for more information, email to ducarroz@netscape.com   */
#define _IMPL_NS_DOM

#include "MacSharedPrefix_debug.h"
