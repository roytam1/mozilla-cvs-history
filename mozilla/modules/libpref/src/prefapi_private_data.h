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

/* Data shared between prefapi.c and nsPref.cpp */

NSPR_BEGIN_EXTERN_C
extern JSTaskState *		gMochaTaskState;
extern JSContext *			gMochaContext;
extern JSObject *			gMochaPrefObject;
extern JSObject *			gGlobalConfigObject;
extern JSClass				global_class;
extern JSClass				autoconf_class;
extern JSPropertySpec		autoconf_props[];
extern JSFunctionSpec       autoconf_methods[];

#ifdef PREF_SUPPORT_OLD_PATH_STRINGS
extern char *				gFileName;
extern char *				gLIFileName;
#endif /*PREF_SUPPORT_OLD_PATH_STRINGS*/

extern struct CallbackNode*	gCallbacks;
extern PRBool				gErrorOpeningUserPrefs;
extern PRBool				gCallbacksEnabled;
extern PRBool				gIsAnyPrefLocked;
extern PLHashTable*			gHashTable;
extern char *               gSavedLine;       
extern PLHashAllocOps       pref_HashAllocOps;

PR_EXTERN(JSBool) PR_CALLBACK pref_BranchCallback(JSContext *cx, JSScript *script);
PR_EXTERN(PrefResult) pref_savePref(PLHashEntry *he, int i, void *arg);
PR_EXTERN(PrefResult) pref_saveLIPref(PLHashEntry *he, int i, void *arg);
PR_EXTERN(PRBool) pref_VerifyLockFile(char* buf, long buflen);
PR_EXTERN(PrefResult) PREF_SetSpecialPrefsLocal(void);
PR_EXTERN(int) pref_CompareStrings(const void *v1, const void *v2, void* unused);
extern JSBool pref_InitInitialObjects(void);

NSPR_END_EXTERN_C

/* Possibly exportable */
#if defined(__cplusplus)
PR_EXTERN(PrefResult) PREF_SavePrefFileSpecWith(
	nsIFileSpec* fileSpec,
	PLHashEnumerator heSaveProc);
#endif /*__cplusplus*/

#ifdef XP_MAC
#  define LINEBREAK           "\012"
#  define LINEBREAK_LEN 1
#elif defined(XP_PC) || defined(XP_OS2)
#  define LINEBREAK           "\015\012"
#  define LINEBREAK_LEN       2
#elif defined(XP_UNIX) || defined(XP_BEOS)
#  define LINEBREAK           "\012"
#  define LINEBREAK_LEN       1
#endif /* XP_MAC */

