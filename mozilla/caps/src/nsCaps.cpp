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


#include "prtypes.h"
#include "nspr.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"

#include "nsCaps.h"
#include "nsPrivilegeManager.h"
#include "nsPrivilege.h"
#include "nsPrivilegeTable.h"
#include "nsPrincipal.h"
#include "nsTarget.h"

PR_BEGIN_EXTERN_C


/* 
 *             C  API  FOR  JS
 *
 * All of the following methods are used by JS (the code located
 * in lib/libmocha area).
 */

/* wrappers for nsPrivilegeManager object */
PR_IMPLEMENT(PRBool) 
nsCapsInitialize() 
{
#if defined(_WIN32)
  nsPrincipal *sysPrin = CreateSystemPrincipal("java/classes/java40.jar", "java/lang/Object.class");
#else
  nsPrincipal *sysPrin = CreateSystemPrincipal("java40.jar", "java/lang/Object.class");
#endif
  if (sysPrin == NULL) {
    sysPrin = new nsPrincipal(nsPrincipalType_Cert, "52:54:45:4e:4e:45:54:49", 
                                         strlen("52:54:45:4e:4e:45:54:49"));
  }

  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  if (nsPrivManager == NULL) {
    nsPrivilegeManagerInitialize();
    nsPrivilegeInitialize();
    nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  }
  PR_ASSERT(nsPrivManager != NULL);
  nsPrivManager->registerSystemPrincipal(sysPrin);
  return PR_TRUE;
}


/* wrappers for nsPrivilegeManager object */
PR_IMPLEMENT(PRBool) 
nsCapsRegisterPrincipal(struct nsPrincipal *principal) 
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  nsPrivManager->registerPrincipal(principal);
  return PR_TRUE;
}

PR_IMPLEMENT(PRBool) 
nsCapsEnablePrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->enablePrivilege(context, target, callerDepth);
}

PR_IMPLEMENT(PRBool) 
nsCapsIsPrivilegeEnabled(void* context, struct nsTarget *target, PRInt32 callerDepth)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->isPrivilegeEnabled(context, target, callerDepth);
}

PR_IMPLEMENT(PRBool) 
nsCapsRevertPrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->revertPrivilege(context, target, callerDepth);
}

PR_IMPLEMENT(PRBool) 
nsCapsDisablePrivilege(void* context, struct nsTarget *target, PRInt32 callerDepth)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->disablePrivilege(context, target, callerDepth);
}

PR_IMPLEMENT(void*) 
nsCapsGetClassPrincipalsFromStack(void* context, PRInt32 callerDepth)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return (void *)nsPrivManager->getClassPrincipalsFromStack(context, callerDepth);
}

PR_IMPLEMENT(nsSetComparisonType) 
nsCapsComparePrincipalArray(void* prin1Array, void* prin2Array)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->comparePrincipalArray((nsPrincipalArray*)prin1Array, 
                                              (nsPrincipalArray*)prin2Array);
}

PR_IMPLEMENT(void*) 
nsCapsIntersectPrincipalArray(void* prin1Array, void* prin2Array)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->intersectPrincipalArray((nsPrincipalArray*)prin1Array, 
                                                (nsPrincipalArray*)prin2Array);
}

PR_IMPLEMENT(PRBool) 
nsCapsCanExtendTrust(void* from, void* to)
{
  nsPrivilegeManager *nsPrivManager = nsPrivilegeManager::getPrivilegeManager();
  return nsPrivManager->canExtendTrust((nsPrincipalArray*)from, 
                                       (nsPrincipalArray*)to);
}

/* wrappers for nsPrincipal object */
PR_IMPLEMENT(struct nsPrincipal *) 
nsCapsNewPrincipal(nsPrincipalType type, void * key, 
                   PRUint32 key_len, void *zig)
{
  return new nsPrincipal(type, key, key_len, zig);
}

PR_IMPLEMENT(const char *) 
nsCapsPrincipalToString(struct nsPrincipal *principal)
{
  return principal->toString();
}

PR_IMPLEMENT(PRBool) 
nsCapsIsCodebaseExact(struct nsPrincipal *principal)
{
  return principal->isCodebaseExact();
}

PR_IMPLEMENT(const char *) 
nsCapsPrincipalGetVendor(struct nsPrincipal *principal)
{
  return principal->getVendor();
}

PR_EXTERN(void *) 
nsCapsNewPrincipalArray(PRUint32 count)
{
  nsPrincipalArray *prinArray = new nsPrincipalArray();
  prinArray->SetSize(count, 1);
  return prinArray;
}

PR_EXTERN(void) 
nsCapsFreePrincipalArray(void *prinArrayArg)
{
  nsPrincipalArray *prinArray = (nsPrincipalArray *)prinArrayArg;
  if (prinArray) {
    prinArray->RemoveAll();
    delete prinArray;
  }
}

PR_EXTERN(void *) 
nsCapsGetPrincipalArrayElement(void *prinArrayArg, PRUint32 index)
{
  nsPrincipalArray *prinArray = (nsPrincipalArray *)prinArrayArg;
  if (prinArray == NULL) {
    return NULL;
  }
  return prinArray->Get(index);
}

PR_EXTERN(void) 
nsCapsSetPrincipalArrayElement(void *prinArrayArg, PRUint32 index, void *element)
{
  nsPrincipalArray *prinArray = (nsPrincipalArray *)prinArrayArg;
  if (prinArray == NULL) {
    return;
  }
  prinArray->Set(index, element);
}

PR_EXTERN(PRUint32) 
nsCapsGetPrincipalArraySize(void *prinArrayArg) 
{
  nsPrincipalArray *prinArray = (nsPrincipalArray *)prinArrayArg;
  if (prinArray == NULL) {
    return 0;
  }
  return prinArray->GetSize();
}

/* wrappers for nsTarget object */
PR_IMPLEMENT(struct nsTarget *) 
nsCapsFindTarget(char *name)
{
  return nsTarget::findTarget(name);
}

/* wrappers for nsPrivilege object */
PR_IMPLEMENT(nsPermissionState) 
nsCapsGetPermission(struct nsPrivilege *privilege)
{
  return privilege->getPermission();
}

/* wrappers for nsPrivilegeTable object */
PR_IMPLEMENT(struct nsPrivilege *)
nsCapsGetPrivilege(struct nsPrivilegeTable *annotation, struct nsTarget *target)
{
  return annotation->get(target);
}


/* Methods for stack walking */
struct NSJSJavaFrameWrapper * (*nsCapsNewNSJSJavaFrameWrapperCallback)(void *) = NULL;
PR_IMPLEMENT(void)
setNewNSJSJavaFrameWrapperCallback(struct NSJSJavaFrameWrapper * (*fp)(void *))
{
    nsCapsNewNSJSJavaFrameWrapperCallback = fp;
}


void (*nsCapsFreeNSJSJavaFrameWrapperCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setFreeNSJSJavaFrameWrapperCallback(void (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsFreeNSJSJavaFrameWrapperCallback = fp;
}


void (*nsCapsGetStartFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setGetStartFrameCallback(void (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsGetStartFrameCallback = fp;
}

PRBool (*nsCapsIsEndOfFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setIsEndOfFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsIsEndOfFrameCallback = fp;
}


PRBool (*nsCapsIsValidFrameCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setIsValidFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsIsValidFrameCallback = fp;
}


void * (*nsCapsGetNextFrameCallback)(struct NSJSJavaFrameWrapper *, int *);
PR_IMPLEMENT(void)
setGetNextFrameCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, int *))
{
    nsCapsGetNextFrameCallback = fp;
}


void * (*nsCapsGetPrincipalArrayCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setOJIGetPrincipalArrayCallback(void * (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsGetPrincipalArrayCallback = fp;
}


void * (*nsCapsGetAnnotationCallback)(struct NSJSJavaFrameWrapper *);
PR_IMPLEMENT(void)
setOJIGetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *))
{
    nsCapsGetAnnotationCallback = fp;
}


void * (*nsCapsSetAnnotationCallback)(struct NSJSJavaFrameWrapper *, void *);
PR_IMPLEMENT(void)
setOJISetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, void *))
{
    nsCapsSetAnnotationCallback = fp;
}

PR_END_EXTERN_C
