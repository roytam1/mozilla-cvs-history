/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nscore_h___
#define nscore_h___

#ifdef _WIN32
#define NS_WIN32 1
#endif

#if defined(__unix)
#define NS_UNIX 1
#endif

#include "prtypes.h"
#include "nsDebug.h"

/** ucs2 datatype for 2 byte unicode characters */
typedef PRUint16 PRUcs2;

/** ucs4 datatype for 4 byte unicode characters */
typedef PRUint32 PRUcs4;

#ifdef NS_UCS4
typedef PRUcs4 PRUnichar;
#else
typedef PRUcs2 PRUnichar;
#endif

/// The preferred symbol for null.
#define nsnull 0

/* Define brackets for protecting C code from C++ */
#ifdef __cplusplus
#define NS_BEGIN_EXTERN_C extern "C" {
#define NS_END_EXTERN_C }
#else
#define NS_BEGIN_EXTERN_C
#define NS_END_EXTERN_C
#endif

/*----------------------------------------------------------------------*/
/* Import/export defines */

#ifdef NS_WIN32
#define NS_IMPORT _declspec(dllimport)
#define NS_IMPORT_(type) type _declspec(dllimport) __stdcall
#define NS_EXPORT _declspec(dllexport)
// XXX NS_EXPORT_ defined in nsCOm.h (xpcom) differs in where the __declspec
// is placed. It needs to be done this way to make the 4.x compiler happy...
#undef NS_EXPORT_
#define NS_EXPORT_(type) type _declspec(dllexport) __stdcall
#else
/* XXX do something useful? */
#define NS_IMPORT
#define NS_IMPORT_(type) type
#define NS_EXPORT
#define NS_EXPORT_(type) type
#endif

#ifdef _IMPL_NS_BASE
#define NS_BASE NS_EXPORT
#else
#define NS_BASE NS_IMPORT
#endif

#ifdef _IMPL_NS_NET
#define NS_NET NS_EXPORT
#else
#define NS_NET NS_IMPORT
#endif

#ifdef _IMPL_NS_DOM
#define NS_DOM NS_EXPORT
#else
#define NS_DOM NS_IMPORT
#endif

#ifdef _IMPL_NS_WIDGET
#define NS_WIDGET NS_EXPORT
#else
#define NS_WIDGET NS_IMPORT
#endif

#ifdef _IMPL_NS_VIEW
#define NS_VIEW NS_EXPORT
#else
#define NS_VIEW NS_IMPORT
#endif

#ifdef _IMPL_NS_GFXNONXP
#define NS_GFXNONXP NS_EXPORT
#define NS_GFXNONXP_(type) NS_EXPORT_(type)
#else
#define NS_GFXNONXP NS_IMPORT
#define NS_GFXNONXP_(type) NS_IMPORT_(type)
#endif

#ifdef _IMPL_NS_GFX
#define NS_GFX NS_EXPORT
#define NS_GFX_(type) NS_EXPORT_(type)
#else
#define NS_GFX NS_IMPORT
#define NS_GFX_(type) NS_IMPORT_(type)
#endif

#ifdef _IMPL_NS_PLUGIN
#define NS_PLUGIN NS_EXPORT
#else
#define NS_PLUGIN NS_IMPORT
#endif

#endif /* nscore_h___ */
