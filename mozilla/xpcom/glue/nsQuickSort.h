/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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


/* We need this because Solaris' version of qsort is broken and
 * causes array bounds reads.
 */

#ifndef nsQuickSort_h___
#define nsQuickSort_h___

#include "nscore.h"
#include "prtypes.h"
/* Had to pull the following define out of xp_core.h
 * to avoid including xp_core.h.
 * That brought in too many header file dependencies.
 */
NS_BEGIN_EXTERN_C

PR_EXTERN(void) NS_QuickSort(void *, unsigned int, unsigned int,
                             int (*)(const void *, const void *, void *), 
                             void *);

NS_END_EXTERN_C

#endif /* nsQuickSort_h___ */
