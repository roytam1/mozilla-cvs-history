/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* must be first in order to correctly pick up TARGET_CARBON define before
 * it is set in ConditionalMacros.h */
#include "DefinesMac.h"

#include <Types.h>
#include <stdlib.h>

#include "IDE_Options.h"

#ifdef DEBUG

/* Debug macros and switches */

#define DEBUG_HEAP_INTEGRITY	1
#define STATS_MAC_MEMORY		0

#define MEM_ASSERT(condition, message)		((condition) ? ((void)0) : DebugStr("\p"message))




#else

/* Non-debug macros and switches */
#define DEBUG_HEAP_INTEGRITY	0
#define STATS_MAC_MEMORY		0



#define MEM_ASSERT(condition, message)		((void)0)

#endif


