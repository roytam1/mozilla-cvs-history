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

#ifndef TextDebugLog_h___
#define TextDebugLog_h___

#ifdef NS_DEBUG
#define IMPORT_DEBUG	1
#endif

#ifdef IMPORT_DEBUG
#include <stdio.h>

#define	IMPORT_LOG0( x)	printf( x)
#define	IMPORT_LOG1( x, y)	printf( x, y)
#define	IMPORT_LOG2( x, y, z)	printf( x, y, z)
#define	IMPORT_LOG3( a, b, c, d)	printf( a, b, c, d)

#else

#define	IMPORT_LOG0( x)
#define	IMPORT_LOG1( x, y)
#define	IMPORT_LOG2( x, y, z)
#define	IMPORT_LOG3( a, b, c, d)

#endif



#endif /* TextDebugLog_h___ */
