/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef _nsEmitterUtils_h_
#define _nsEmitterUtils_h_

#include "prmem.h"
#include "plstr.h"

extern "C" PRBool   EmitThisHeaderForPrefSetting(PRInt32 dispType, const char *header);

//
// Create a file spec or file name using the name passed
// in as a template
//
nsFileSpec  *nsMsgCreateTempFileSpec(char *tFileName);
char        *nsMsgCreateTempFileName(char *tFileName);

char        *nsMimePlatformFileToURL (nsFileSpec  aFileSpec);

#endif // _nsEmitterUtils_h_

