/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*******************************************************************************
 * Netscape version of jni_md.h -- depends on jri_md.h
 ******************************************************************************/

#ifndef JNI_MD_H
#define JNI_MD_H

/*******************************************************************************
 * WHAT'S UP WITH THIS FILE?
 * 
 * This is where we define the mystical JNI_PUBLIC_API macro that works on all
 * platforms. If you're running with Visual C++, Symantec C, or Borland's 
 * development environment on the PC, you're all set. Or if you're on the Mac
 * with Metrowerks, Symantec or MPW with SC you're ok too. For UNIX it shouldn't
 * matter.

 * Changes by sailesh on 9/26 

 * There are two symbols used in the declaration of the JNI functions
 * and native code that uses the JNI:
 * JNICALL - specifies the calling convention 
 * JNIEXPORT - specifies export status of the function 
 * 
 * The syntax to specify calling conventions is different in Win16 and
 * Win32 - the brains at Micro$oft at work here. JavaSoft in their
 * infinite wisdom cares for no platform other than Win32, and so they
 * just define these two symbols as:

 #define JNIEXPORT __declspec(dllexport)
 #define JNICALL __stdcall

 * We deal with this, in the way JRI defines the JRI_PUBLIC_API, by
 * defining a macro called JNI_PUBLIC_API. Any of our developers who
 * wish to use code for Win16 and Win32, _must_ use JNI_PUBLIC_API to
 * be able to export functions properly.

 * Since we must also maintain compatibility with JavaSoft, we
 * continue to define the symbol JNIEXPORT. However, use of this
 * internally is deprecated, since it will cause a mess on Win16.

 * We _do not_ need a new symbol called JNICALL. Instead we
 * redefine JNICALL in the same way JRI_CALLBACK was defined.

 ******************************************************************************/

/* DLL Entry modifiers... */

/* PC */
#if defined(XP_PC) || defined(_WINDOWS) || defined(WIN32) || defined(_WIN32)
#	include <windows.h>
#	if defined(_MSC_VER)
#		if defined(WIN32) || defined(_WIN32)
#			define JNI_PUBLIC_API(ResultType)	_declspec(dllexport) ResultType __stdcall
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	_declspec(dllexport) ResultType
#			define JNICALL                          __stdcall
#		else /* !_WIN32 */
#		    if defined(_WINDLL)
#			define JNI_PUBLIC_API(ResultType)	ResultType __cdecl __export __loadds 
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType __cdecl __loadds
#			define JNICALL			        __loadds
#		    else /* !WINDLL */
#			define JNI_PUBLIC_API(ResultType)	ResultType __cdecl __export
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType __cdecl __export
#			define JNICALL			        __export
#                   endif /* !WINDLL */
#		endif /* !_WIN32 */
#	elif defined(__BORLANDC__)
#		if defined(WIN32) || defined(_WIN32)
#			define JNI_PUBLIC_API(ResultType)	__export ResultType
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	 __export ResultType
#			define JNICALL
#		else /* !_WIN32 */
#			define JNI_PUBLIC_API(ResultType)	ResultType _cdecl _export _loadds 
#			define JNI_PUBLIC_VAR(VarType)		VarType
#			define JNI_NATIVE_STUB(ResultType)	ResultType _cdecl _loadds
#			define JNICALL			_loadds
#		endif
#	else
#		error Unsupported PC development environment.	
#	endif
#	ifndef IS_LITTLE_ENDIAN
#		define IS_LITTLE_ENDIAN
#	endif
	/*  This is the stuff inherited from JavaSoft .. */
#	define JNIEXPORT __declspec(dllexport)


/* Mac */
#elif macintosh || Macintosh || THINK_C
#	if defined(__MWERKS__)				/* Metrowerks */
#		if !__option(enumsalwaysint)
#			error You need to define 'Enums Always Int' for your project.
#		endif
#		if defined(GENERATING68K) && !GENERATINGCFM 
#			if !__option(fourbyteints) 
#				error You need to define 'Struct Alignment: 68k' for your project.
#			endif
#		endif /* !GENERATINGCFM */
#		define JNI_PUBLIC_API(ResultType)	__declspec(export) ResultType 
#		define JNI_PUBLIC_VAR(VarType)		JNI_PUBLIC_API(VarType)
#		define JNI_NATIVE_STUB(ResultType)	JNI_PUBLIC_API(ResultType)
#	elif defined(__SC__)				/* Symantec */
#		error What are the Symantec defines? (warren@netscape.com)
#	elif macintosh && applec			/* MPW */
#		error Please upgrade to the latest MPW compiler (SC).
#	else
#		error Unsupported Mac development environment.
#	endif
#	define JNICALL
	/*  This is the stuff inherited from JavaSoft .. */
#	define JNIEXPORT

/* Unix or else */
#else
#	define JNI_PUBLIC_API(ResultType)		ResultType
#       define JNI_PUBLIC_VAR(VarType)                  VarType
#       define JNI_NATIVE_STUB(ResultType)              ResultType
#	define JNICALL
	/*  This is the stuff inherited from JavaSoft .. */
#	define JNIEXPORT
#endif

#ifndef FAR		/* for non-Win16 */
#define FAR
#endif

/* Get the rest of the stuff from jri_md.h */
#include "jri_md.h"

#endif /* JNI_MD_H */
