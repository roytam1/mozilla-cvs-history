/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef _NS_CAPS_H_
#define _NS_CAPS_H_

#include "prtypes.h"
#include "nsCapsEnums.h"

PR_BEGIN_EXTERN_C

struct nsPrincipal;
struct nsTarget;
struct nsPrivilegeManager;
struct nsPrivilege;
struct nsPrivilegeTable;
struct NSJSJavaFrameWrapper;

/* wrappers for nsPrivilegeManager object */
PR_EXTERN(PRBool) 
nsCapsRegisterPrincipal(struct nsPrincipal *principal); 

PR_EXTERN(PRBool) 
nsCapsEnablePrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth);

PR_EXTERN(PRBool) 
nsCapsIsPrivilegeEnabled(void* context, struct nsTarget *target, PRInt32 callerDepth);

PR_EXTERN(PRBool) 
nsCapsRevertPrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth);

PR_EXTERN(PRBool) 
nsCapsDisablePrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth);

PR_EXTERN(void*) 
nsCapsGetClassPrincipalsFromStack(void* context, PRInt32 callerDepth);

PR_EXTERN(nsSetComparisonType) 
nsCapsComparePrincipalArray(void* prin1Array, void* prin2Array);

PR_EXTERN(void*) 
nsCapsIntersectPrincipalArray(void* prin1Array, void* prin2Array);

PR_EXTERN(PRBool) 
nsCapsCanExtendTrust(void* from, void* to);


/* wrappers for nsPrincipal object */
PR_EXTERN(struct nsPrincipal *) 
nsCapsNewPrincipal(nsPrincipalType type, void * key, 
                   PRUint32 key_len, void *zig);

PR_EXTERN(const char *) 
nsCapsPrincipalToString(struct nsPrincipal *principal);

PR_EXTERN(PRBool) 
nsCapsIsCodebaseExact(struct nsPrincipal *principal);

PR_EXTERN(const char *) 
nsCapsPrincipalGetVendor(struct nsPrincipal *principal);

PR_EXTERN(void *) 
nsCapsNewPrincipalArray(PRUint32 count);

PR_EXTERN(void) 
nsCapsFreePrincipalArray(void *prinArray);

PR_EXTERN(void *) 
nsCapsGetPrincipalArrayElement(void *prinArray, PRUint32 index);

PR_EXTERN(void) 
nsCapsSetPrincipalArrayElement(void *prinArray, PRUint32 index, void *element);

PR_EXTERN(PRUint32) 
nsCapsGetPrincipalArraySize(void *prinArray);


/* wrappers for nsTarget object */
PR_EXTERN(struct nsTarget *) 
nsCapsFindTarget(char *name);


/* wrappers for nsPrivilege object */
PR_EXTERN(nsPermissionState) 
nsCapsGetPermission(struct nsPrivilege *privilege);


/* wrappers for nsPrivilegeTable object */
PR_EXTERN(struct nsPrivilege *)
nsCapsGetPrivilege(struct nsPrivilegeTable *annotation, struct nsTarget *target);

/* Methods for stack walking */

extern struct NSJSJavaFrameWrapper * (*nsCapsNewNSJSJavaFrameWrapperCallback)(void *);
PR_EXTERN(void)
setNewNSJSJavaFrameWrapperCallback(struct NSJSJavaFrameWrapper * (*fp)(void *));

extern void (*nsCapsFreeNSJSJavaFrameWrapperCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setFreeNSJSJavaFrameWrapperCallback(void (*fp)(struct NSJSJavaFrameWrapper *));

extern void (*nsCapsGetStartFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setGetStartFrameCallback(void (*fp)(struct NSJSJavaFrameWrapper *));

extern PRBool (*nsCapsIsEndOfFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setIsEndOfFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *));

extern PRBool (*nsCapsIsValidFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setIsValidFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *));

extern void * (*nsCapsGetNextFrameCallback)(struct NSJSJavaFrameWrapper *, int *);
PR_EXTERN(void)
setGetNextFrameCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, int *));

extern void * (*nsCapsGetPrincipalArrayCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setOJIGetPrincipalArrayCallback(void * (*fp)(struct NSJSJavaFrameWrapper *));

extern void * (*nsCapsGetAnnotationCallback)(struct NSJSJavaFrameWrapper *);
PR_EXTERN(void)
setOJIGetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *));

extern void * (*nsCapsSetAnnotationCallback)(struct NSJSJavaFrameWrapper *, void *);
PR_EXTERN(void)
setOJISetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, void *));


PR_END_EXTERN_C

#endif /* _NS_CAPS_H_ */
