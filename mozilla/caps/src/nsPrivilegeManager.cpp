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

#include "nsPrivilegeManager.h"
#include "nsPrivilege.h"
#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prprf.h"
#include "plbase64.h"
#include "jpermission.h"
#include "rdf.h"

#ifdef DEBUG_raman_xxx
/* ifdef'ed out ADMIN UI until we merge changes with rdf branch.
 * All these changes are under DEBUG_raman_xxx
 */
#include "jsec2rdf.h"
#endif /* DEBUG_raman_xxx */

static nsPrivilegeManager * thePrivilegeManager = NULL;

static nsPrincipal *theSystemPrincipal = NULL;

static nsPrincipal *theUnsignedPrincipal;
static nsPrincipalArray *theUnsignedPrincipalArray;

static nsPrincipal *theUnknownPrincipal;
static nsPrincipalArray *theUnknownPrincipalArray;


static PRMonitor *caps_lock = NULL;

/* We could avoid the following globals if nsHashTable's Enumerate accepted
 * a void * as argument and it passed that argument as a parameter to the 
 * callback function.
 */
char *gListOfPrincipals;
char *gForever;
char *gSession;
char *gDenied;
nsPrivilegeTable *gPrivilegeTable;

static PRBool getPrincipalString(nsHashKey *aKey, void *aData);

#ifdef DEBUG_raman_xxx
static nsPrincipal *RDF_getPrincipal(JSec_Principal jsec_pr);
static PRBool RDF_RemovePrincipal(nsPrincipal *prin);
static PRBool RDF_RemovePrincipalsPrivilege(nsPrincipal *prin, nsTarget *target);
#endif /* DEBUG_raman_xxx */


PR_BEGIN_EXTERN_C
#include "xp.h"
#include "prefapi.h"

PRBool CMGetBoolPref(char * pref_name) 
{
  XP_Bool pref;

  if (PREF_GetBoolPref(pref_name, &pref) >=0) {
    return pref;
  }
  return FALSE;
}
PR_END_EXTERN_C

PRBool
nsCaps_lock(void)
{
    if(caps_lock == NULL)
    {
	caps_lock = PR_NewMonitor();
	if(caps_lock == NULL)
	    return PR_FALSE;
    } 
    PR_EnterMonitor(caps_lock);
    return PR_TRUE;
}

void
nsCaps_unlock(void)
{
    PR_ASSERT(caps_lock != NULL);
  
    if(caps_lock != NULL)
    {
        PR_ExitMonitor(caps_lock);
    }
}



//
// 			PUBLIC METHODS 
//

nsPrivilegeManager::nsPrivilegeManager(void)
{
  nsCaps_lock();
  itsPrinToPrivTable = new nsHashtable();
  itsPrinToMacroTargetPrivTable = new nsHashtable();
  itsPrinNameToPrincipalTable = new nsHashtable();
  nsCaps_unlock();

}

nsPrivilegeManager::~nsPrivilegeManager(void)
{
  nsCaps_lock();
  if (itsPrinToPrivTable)
    delete itsPrinToPrivTable;
  if (itsPrinToMacroTargetPrivTable)
    delete itsPrinToMacroTargetPrivTable;
  if (itsPrinNameToPrincipalTable)
    delete itsPrinNameToPrincipalTable;
  nsCaps_unlock();
}

void nsPrivilegeManager::addToPrinNameToPrincipalTable(nsPrincipal *prin)
{
  char *prinName = prin->toString();
  if (prinName == NULL) {
    return;
  }
  StringKey prinNameKey(prinName);
  nsCaps_lock();
  if (NULL == itsPrinNameToPrincipalTable->Get(&prinNameKey)) {
    itsPrinNameToPrincipalTable->Put(&prinNameKey, prin);
  }
  nsCaps_unlock();
}


void nsPrivilegeManager::registerSystemPrincipal(nsPrincipal *prin)
{
  PrincipalKey prinKey(prin);
  nsCaps_lock();
  if (NULL == itsPrinToPrivTable->Get(&prinKey)) {
    itsPrinToPrivTable->Put(&prinKey, new nsSystemPrivilegeTable());
  }
  if (NULL == itsPrinToMacroTargetPrivTable->Get(&prinKey)) {
    itsPrinToMacroTargetPrivTable->Put(&prinKey, new nsSystemPrivilegeTable());
  }
  theSystemPrincipal = prin;
  CreateSystemTargets(prin);
  // Load the signed applet's ACL from the persistence store
  load();
  nsCaps_unlock();
}

void nsPrivilegeManager::registerPrincipal(nsPrincipal *prin)
{
  //
  // the new PrivilegeTable will have all privileges "blank forever"
  // until changed by calls to enablePrincipalPrivilegeHelper
  //
  PrincipalKey prinKey(prin);
  nsCaps_lock();
  if (NULL == itsPrinToPrivTable->Get(&prinKey)) {
    itsPrinToPrivTable->Put(&prinKey, new nsPrivilegeTable());
  }
  if (NULL == itsPrinToMacroTargetPrivTable->Get(&prinKey)) {
    itsPrinToMacroTargetPrivTable->Put(&prinKey, new nsPrivilegeTable());
  }
  addToPrinNameToPrincipalTable(prin);
  nsCaps_unlock();
}


PRBool nsPrivilegeManager::unregisterPrincipal(nsPrincipal *prin)
{
  if (prin->equals(getSystemPrincipal())) {
    return PR_FALSE;
  }
  PrincipalKey prinKey(prin);
  nsCaps_lock();

  /* Get the privilegetables and free them up */
  nsPrivilegeTable *pt = 
    (nsPrivilegeTable *)itsPrinToPrivTable->Get(&prinKey);
  if (pt != NULL) {
    delete pt;
  }
  nsPrivilegeTable *mpt = 
    (nsPrivilegeTable *)itsPrinToMacroTargetPrivTable->Get(&prinKey);
  if (mpt != NULL) {
    delete mpt;
  }

  /* Remove the principal */
  void *old_prin = itsPrinToPrivTable->Remove(&prinKey);
  void *old_prin1 = itsPrinToMacroTargetPrivTable->Remove(&prinKey);

  /* remove principal from PrinNameToPrincipalTable */
  char *prinName = prin->toString();
  StringKey prinNameKey(prinName);
  itsPrinNameToPrincipalTable->Remove(&prinNameKey);

  /* remove the principal from RDF also */
#ifdef DEBUG_raman_xxx
  RDF_RemovePrincipal(prin);
#endif /* DEBUG_raman_xxx */

  nsCaps_unlock();
  if ((old_prin == NULL) && (old_prin1 == NULL)) {
    return PR_FALSE;
  } 
  return PR_TRUE;
}


PRBool nsPrivilegeManager::isPrivilegeEnabled(nsTarget *target, PRInt32 callerDepth)
{
  nsTargetArray * targetArray = new nsTargetArray();
  nsTarget *targ = nsTarget::findTarget(target);
  if (targ != target)  {
    return PR_FALSE;
  }
  targetArray->Add(targ);
  return (checkPrivilegeEnabled(targetArray, callerDepth, NULL) != NULL) ? PR_FALSE : PR_TRUE;
}

PRBool nsPrivilegeManager::enablePrivilege(nsTarget *target, PRInt32 callerDepth)
{
  return enablePrivilegePrivate(target, NULL, callerDepth);
}

PRBool nsPrivilegeManager::enablePrivilege(nsTarget *target, 
                                           nsPrincipal *preferredPrincipal, 
                                           PRInt32 callerDepth)
{
  return enablePrivilegePrivate(target, preferredPrincipal, callerDepth);
}

PRBool nsPrivilegeManager::revertPrivilege(nsTarget *target, 
                                           PRInt32 callerDepth)
{
  nsTarget *targ = nsTarget::findTarget(target);
  if (targ != target) {
    //throw new ForbiddenTargetException(target + " is not a registered target");
    return PR_FALSE;
  }

  nsPrivilegeTable *privTable = getPrivilegeTableFromStack(callerDepth, 
                                                           PR_TRUE);
  nsCaps_lock();
  privTable->put(target, nsPrivilege::findPrivilege(nsPermissionState_Blank,
                                                    nsDurationState_Scope));
  nsCaps_unlock();
  return PR_TRUE;
}


PRBool nsPrivilegeManager::disablePrivilege(nsTarget *target,
                                            PRInt32 callerDepth)
{
  nsTarget *targ = nsTarget::findTarget(target);
  if (targ != target) {
    //throw new ForbiddenTargetException(target + " is not a registered target");
    return PR_FALSE;
  }
  nsPrivilegeTable *privTable = getPrivilegeTableFromStack(callerDepth, 
                                                           PR_TRUE);
  nsCaps_lock();
  privTable->put(target, nsPrivilege::findPrivilege(nsPermissionState_Forbidden,
                                                    nsDurationState_Scope));
  nsCaps_unlock();
  return PR_TRUE;
}

PRBool nsPrivilegeManager::enablePrincipalPrivilegeHelper(nsTarget *target, 
                                                          PRInt32 callerDepth, 
                                                          nsPrincipal *preferredPrin, 
                                                          void * data,
                                                          nsTarget *impersonator)
{
  nsPrincipalArray* callerPrinArray;
  nsPrincipal *useThisPrin = NULL;
  char *err;

  /* Get the registered target */
  nsTarget *targ = nsTarget::findTarget(target);
  if (targ != target) {
    return PR_FALSE;
    /* throw new ForbiddenTargetException(target + " is not a registered target"); */
  }
  callerPrinArray = getClassPrincipalsFromStack(callerDepth);

  if (preferredPrin != NULL) {
    nsPrincipal *callerPrin;
    for (PRUint32 i = callerPrinArray->GetSize(); i-- > 0;) {
      callerPrin = (nsPrincipal *)callerPrinArray->Get(i);
      if ((callerPrin->equals(preferredPrin)) &&
          ((callerPrin->isCert() ||
            callerPrin->isCertFingerprint()))) {

        useThisPrin = callerPrin;
        break;
      }
    }
    if ((useThisPrin == NULL) && (impersonator)){
      nsTarget *t1;
      t1 = impersonator;
      if (PR_FALSE == isPrivilegeEnabled(t1, 0))
        return PR_FALSE;
      useThisPrin = preferredPrin;
      callerPrinArray = new nsPrincipalArray();
      callerPrinArray->Add(preferredPrin);
    }
  }

  if (isPermissionGranted(target, callerPrinArray, data))
    return PR_TRUE;

  // Do a user dialog
  nsPrivilege *newPrivilege;

  //
  // before we do the user dialog, we need to figure out which principal
  // gets the user's blessing.  The applet is allowed to bias this
  // by offering a preferred principal.  We only honor this if the
  // principal is *registered* (stored in itsPrinToPrivTable) and
  // is based on cryptographic credentials, rather than a codebase.
  //
  // if no preferredPrin is specified, or we don't like preferredPrin,
  // we'll use the first principal on the calling class.  We know that
  // cryptographic credentials go first in the list, so this should
  // get us something reasonable.
  //

  if (useThisPrin == NULL) {
    if (callerPrinArray->GetSize() == 0)
      // throw new ForbiddenTargetException("request's caller has no principal!");

      return PR_FALSE;
    useThisPrin = (nsPrincipal *)callerPrinArray->Get(0);
  }


  /* Get the Lock to display the dialog */
  nsCaps_lock();

  PRBool ret_val=PR_FALSE;

  if (PR_TRUE == isPermissionGranted(target, callerPrinArray, data)) {
    ret_val = PR_TRUE;
    goto done;
  }

  newPrivilege = target->enablePrivilege(useThisPrin, data);

  // Forbidden for session is equivelent to decide later.
  // If the privilege is DECIDE_LATER then throw exception.
  // That is user should be prompted again when this applet 
  // performs the same privileged operation
  //
  if ((!newPrivilege->isAllowed()) &&
      (newPrivilege->getDuration() == nsDurationState_Session)) {
    // "User didn't grant the " + target->getName() + " privilege.";
    ret_val = PR_FALSE;
    goto done;
  }

  registerPrincipalAndSetPrivileges(useThisPrin, target, newPrivilege);

  //System.out.println("Privilege table modified for: " + 
  // 		useThisPrin.toVerboseString() + " for target " + 
  //		target + " Privilege " + newPrivilege);

  // Save the signed applet's ACL to the persistence store
  err = useThisPrin->savePrincipalPermanently();
  if ((err == NULL) && 
      (newPrivilege->getDuration() == nsDurationState_Forever)) {

    //XXX: How do we save permanent access for unsigned principals
    ///
    if (!useThisPrin->equals(theUnsignedPrincipal)) {
      save(useThisPrin, target, newPrivilege);
    }
  }

  // if newPrivilege is FORBIDDEN then throw an exception
  if (newPrivilege->isForbidden()) {
    // "User didn't grant the " + target->getName() + " privilege.";
    ret_val = PR_FALSE;
    goto done;
  }

  ret_val = PR_TRUE;

done:
  nsCaps_unlock();
  return PR_TRUE;

}


nsPrivilegeTable *
nsPrivilegeManager::enableScopePrivilegeHelper(nsTarget *target, 
                                               PRInt32 callerDepth, 
                                               void *data, 
                                               PRBool helpingSetScopePrivilege, 
                                               nsPrincipal *prefPrin)
{
  nsPrivilegeTable *privTable;
  nsPrivilege *allowedScope;
  PRBool res;

  nsTarget *targ = nsTarget::findTarget(target);
  if (targ != target) {
    //throw new ForbiddenTargetException(target + " is not a registered target");
    return NULL;
  }
  if (prefPrin != NULL) {
    res = checkPrivilegeGranted(target, prefPrin, data);
  } else {
    res = checkPrivilegeGranted(target, callerDepth, data);
  }
  if (res == PR_FALSE)
    return NULL;

  privTable = getPrivilegeTableFromStack(callerDepth, 
                                         (helpingSetScopePrivilege ? PR_FALSE : PR_TRUE));
  if (helpingSetScopePrivilege) {
    if (privTable == NULL)  {
      privTable = new nsPrivilegeTable();
    }
  }
                
  allowedScope = nsPrivilege::findPrivilege(nsPermissionState_Allowed,
                                            nsDurationState_Scope);
  updatePrivilegeTable(target, privTable, allowedScope);
  return privTable;
}


void 
nsPrivilegeManager::registerPrincipalAndSetPrivileges(nsPrincipal *prin, 
                                                      nsTarget *target, 
                                                      nsPrivilege *newPrivilege)
{
  nsPrivilegeTable *privTable;

  registerPrincipal(prin);

  //Store the list of targets for which the user has given privilege
  PrincipalKey prinKey(prin);
  nsCaps_lock();
  privTable = (nsPrivilegeTable *)itsPrinToMacroTargetPrivTable->Get(&prinKey);
  privTable->put(target, newPrivilege);
  nsCaps_unlock();

  privTable = (nsPrivilegeTable *)itsPrinToPrivTable->Get(&prinKey);
  updatePrivilegeTable(target, privTable, newPrivilege); 
}


void nsPrivilegeManager::updatePrivilegeTable(nsTarget *target, 
                                              nsPrivilegeTable *privTable, 
                                              nsPrivilege *newPrivilege)
{
  nsTargetArray* primitiveTargets = target->getFlattenedTargetArray();
  nsPrivilege *oldPrivilege;
  nsPrivilege *privilege;
  nsTarget *primTarget;

  nsCaps_lock();
  for (int i = primitiveTargets->GetSize(); i-- > 0;) {
    primTarget = (nsTarget *)primitiveTargets->Get(i);
    oldPrivilege = privTable->get(primTarget);
    if (oldPrivilege != NULL) {
      privilege = nsPrivilege::add(oldPrivilege, newPrivilege);
    } else {
      privilege = newPrivilege;
    }
    privTable->put(primTarget, privilege);
  }
  nsCaps_unlock();
}

PRBool nsPrivilegeManager::checkPrivilegeGranted(nsTarget *target, 
                                                 PRInt32 callerDepth)
{
  return checkPrivilegeGranted(target, callerDepth, NULL);
}

PRBool nsPrivilegeManager::checkPrivilegeGranted(nsTarget *target, 
                                                 nsPrincipal *prin, 
                                                 void *data)
{
  nsPrivilege * privilege = getPrincipalPrivilege(target, prin, data);
  if (!privilege->isAllowed()) {
    //throw new ForbiddenTargetException("access to target denied");
    return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool nsPrivilegeManager::checkPrivilegeGranted(nsTarget *target, 
                                                 PRInt32 callerDepth,
                                                 void *data)
{
  nsPrincipalArray* callerPrinArray = 
    getClassPrincipalsFromStack(callerDepth);

  nsPermissionState privilege = 
    getPrincipalPrivilege(target, callerPrinArray, data);
  if (privilege != nsPermissionState_Allowed) {
    //throw new ForbiddenTargetException("access to target denied");
    return PR_FALSE;
  }
  return PR_TRUE;
}

nsPrivilegeManager * nsPrivilegeManager::getPrivilegeManager(void)
{
  return thePrivilegeManager;
}

nsPrincipalArray* nsPrivilegeManager::getMyPrincipals(PRInt32 callerDepth)
{
  if (thePrivilegeManager == NULL)
    return NULL;
  return thePrivilegeManager->getClassPrincipalsFromStack(callerDepth);
}

nsPrincipal* nsPrivilegeManager::getSystemPrincipal(void)
{
  return theSystemPrincipal;
}

PRBool nsPrivilegeManager::hasSystemPrincipal(nsPrincipalArray *prinArray)
{
  nsPrincipal *sysPrin = getSystemPrincipal();
  nsPrincipal *prin;

  if (sysPrin == NULL)
    return PR_FALSE;
  for (int i = prinArray->GetSize(); i-- > 0;) {
    prin = (nsPrincipal *)prinArray->Get(i);
    if (sysPrin->equals(prin))
      return PR_TRUE;
  }
  return PR_FALSE;
}

nsPrincipal* nsPrivilegeManager::getUnsignedPrincipal(void)
{
  return theUnsignedPrincipal;
}

nsPrincipal* nsPrivilegeManager::getUnknownPrincipal(void)
{
  return theUnknownPrincipal;
}

nsSetComparisonType 
nsPrivilegeManager::comparePrincipalArray(nsPrincipalArray* p1, 
                                          nsPrincipalArray* p2)
{
  nsHashtable *p2Hashtable = new nsHashtable();
  PRBool value;
  nsPrincipal *prin;
  PRUint32 i;

  for (i = p2->GetSize(); i-- > 0;) {
    prin = (nsPrincipal *)p2->Get(i);
    PrincipalKey prinKey(prin);
    p2Hashtable->Put(&prinKey, (void *)PR_TRUE);
  }

  for (i = p1->GetSize(); i-- > 0;) {
    prin = (nsPrincipal *)p1->Get(i);
    PrincipalKey prinKey(prin);
    value = (PRBool)p2Hashtable->Get(&prinKey);
    if (!value) {
      // We found an extra element in p1
      return nsSetComparisonType_NoSubset;
    }
    if (value == PR_TRUE) {
      p2Hashtable->Put(&prinKey, (void *)PR_FALSE);
    }
  }

  for (i = p2->GetSize(); i-- > 0;) {
    prin = (nsPrincipal *)p2->Get(i);
    PrincipalKey prinKey(prin);
    value = (PRBool)p2Hashtable->Get(&prinKey);
    if (value == PR_TRUE) {
      // We found an extra element in p2
      return nsSetComparisonType_ProperSubset;
    }
  }

  return nsSetComparisonType_Equal;
}

nsPrincipalArray* 
nsPrivilegeManager::intersectPrincipalArray(nsPrincipalArray* p1, 
                                            nsPrincipalArray* p2)
{
  int p1_length = p1->GetSize();
  int p2_length = p2->GetSize();
  nsPrincipalArray *in = new nsPrincipalArray();
  int count = 0;
  nsPrincipal *prin1;
  nsPrincipal *prin2;
  int i;

  in->SetSize(p1_length, 1);
  int in_length = in->GetSize();
    
  for (i=0; i < p1_length; i++) {
    for (int j=0; j < p2_length; j++) {
      prin1 = (nsPrincipal *)p1->Get(i);
      prin2 = (nsPrincipal *)p2->Get(j);
      if (prin1->equals(prin2)) {
        in->Set(i, (void *)PR_TRUE);
        count++;
        break;
      } else {
        in->Set(i, (void *)PR_FALSE);
      }
    }
  }
    
  nsPrincipalArray *result = new nsPrincipalArray();
  result->SetSize(count, 1);
  
  PRBool doesIntersect;
  int j=0;
  PR_ASSERT(in_length == (int)(p1->GetSize()));
  PR_ASSERT(in_length == (int)(in->GetSize()));
  for (i=0; i < in_length; i++) {
    doesIntersect = (PRBool)in->Get(i);
    if (doesIntersect) {
      PR_ASSERT(j < count);
      result->Set(j, p1->Get(i));
      j++;
    }
  }
    
  return result;
}

PRBool nsPrivilegeManager::canExtendTrust(nsPrincipalArray* from, 
                                          nsPrincipalArray* to)
{
  nsPrincipalArray * intersect = intersectPrincipalArray(from, to);
  if (intersect->GetSize() == from->GetSize())
    return PR_TRUE;
  if (intersect->GetSize() == 0 || 
      (intersect->GetSize() != (from->GetSize() - 1)))
    return PR_FALSE;
  int intersect_size = intersect->GetSize();
  nsPrincipal *prin;
  int i;
  for (i=0; i < intersect_size; i++) {
    prin = (nsPrincipal *)intersect->Get(i);
    if (prin->isCodebase())
      return PR_FALSE;
  }
  int codebaseCount = 0;
  int from_size = from->GetSize();
  for (i=0; i < from_size; i++) {
    prin = (nsPrincipal *)from->Get(i);
    if (prin->isCodebase())
      codebaseCount++;
  }
  return (codebaseCount == 1) ? PR_TRUE : PR_FALSE;
}

PRBool nsPrivilegeManager::checkMatchPrincipal(nsPrincipal *prin, 
                                               PRInt32 callerDepth)
{
  nsPrincipalArray *prinArray = new nsPrincipalArray();
  prinArray->Add(prin);
  nsPrincipalArray *classPrinArray = getClassPrincipalsFromStack(callerDepth);
  return (comparePrincipalArray(prinArray, classPrinArray) != nsSetComparisonType_NoSubset) ? PR_TRUE : PR_FALSE;
}

static PRBool getPrincipalString(nsHashKey *aKey, void *aData) 
{
  /* Admin UI */
  /* XXX: Ignore empty strings */
  const char *string = ((StringKey *) aKey)->itsString;
  if (gListOfPrincipals == NULL) {
    gListOfPrincipals = PR_sprintf_append(gListOfPrincipals, "\"%s\"", string);
  } else {
    gListOfPrincipals = PR_sprintf_append(gListOfPrincipals, ",\"%s\"", string);
  }
  return PR_TRUE;
}


const char * nsPrivilegeManager::getAllPrincipalsString(void)
{
  /* Admin UI */
  char *principalStrings=NULL;
  if (itsPrinNameToPrincipalTable == NULL) {
    return NULL;
  }

  nsCaps_lock();
  gListOfPrincipals = NULL;
  itsPrinNameToPrincipalTable->Enumerate(getPrincipalString);
  if (gListOfPrincipals) {
    /* Make a copy of the principals and return it */
    principalStrings = XP_AppendStr(principalStrings, gListOfPrincipals);
    PR_Free(gListOfPrincipals);
  }
  gListOfPrincipals = NULL;
  nsCaps_unlock();

  return principalStrings;
}


nsPrincipal * nsPrivilegeManager::getPrincipalFromString(char *prinName)
{
  /* Admin UI */
  StringKey prinNameKey(prinName);
  nsCaps_lock();
  nsPrincipal *prin = 
    (nsPrincipal *)itsPrinNameToPrincipalTable->Get(&prinNameKey);
  nsCaps_unlock();
  return prin;
}

static PRBool getPermissionsString(nsHashKey *aKey, void *aData) 
{
  /* Admin UI */
  TargetKey *targetKey = (TargetKey *) aKey;
  nsTarget *target = targetKey->itsTarget;
  nsPrivilege *priv = (nsPrivilege *)aData;
  char *desc = target->getDescription();
  if (priv->isAllowedForever()) {
    gForever = PR_sprintf_append(gForever, "<option>%s", desc);
  } else if (priv->isForbiddenForever()) {
    gDenied = PR_sprintf_append(gDenied, "<option>%s", desc);
  } else if (priv->isAllowed()) {
    gSession = PR_sprintf_append(gSession, "<option>%s", desc);
  }
  return PR_TRUE;
}


void nsPrivilegeManager::getTargetsWithPrivileges(char *prinName, 
                                                  char** forever, 
                                                  char** session, 
                                                  char **denied)
{
  /* Admin UI */
  nsCaps_lock();
  *forever = gForever = NULL;
  *session = gSession = NULL;
  *denied = gDenied = NULL;
  nsPrincipal *prin = getPrincipalFromString(prinName);
  if (prin == NULL) {
    nsCaps_unlock();
    return;
  }
  PrincipalKey prinKey(prin);
  nsPrivilegeTable *pt = 
    (nsPrivilegeTable *)itsPrinToMacroTargetPrivTable->Get(&prinKey);
  if (pt == NULL) {
    nsCaps_unlock();
    return;
  }
  pt->Enumerate(getPermissionsString);
  /* The caller should free up the allocated memory */
  *forever = gForever;
  *session = gSession;
  *denied = gDenied;
  gForever = NULL;
  gSession = NULL;
  gDenied = NULL;
  nsCaps_unlock();
}

PRBool nsPrivilegeManager::removePrincipal(char *prinName)
{
  /* Admin UI */
  nsCaps_lock();
  nsPrincipal *prin = getPrincipalFromString(prinName);
  if (prin == NULL) {
    nsCaps_unlock();
    return PR_FALSE;
  }
  unregisterPrincipal(prin);
  nsCaps_unlock();
  return PR_TRUE;
}

PRBool nsPrivilegeManager::removePrincipalsPrivilege(char *prinName, 
                                                     char *targetDesc)
{
  /* Admin UI */
  nsPrincipal *prin = getPrincipalFromString(prinName);
  if (prin == NULL) {
    return PR_FALSE;
  }
  /* targetDesc is passed to the admin UI in getPermissionsString */
  nsTarget *target = nsTarget::getTargetFromDescription(targetDesc);
  if (target == NULL) {
    return PR_FALSE;
  }
  remove(prin, target);
  return PR_TRUE;
}

static PRBool updatePrivileges(nsHashKey *aKey, void *aData) 
{
  /* Admin UI */
  TargetKey *targetKey = (TargetKey *) aKey;
  nsTarget *target = targetKey->itsTarget;
  nsPrivilege *priv = (nsPrivilege *)aData;
  nsPrivilegeManager *mgr = nsPrivilegeManager::getPrivilegeManager();
  mgr->updatePrivilegeTable(target, gPrivilegeTable, priv);
  return PR_TRUE;
}

void nsPrivilegeManager::remove(nsPrincipal *prin, nsTarget *target)
{
  /* Admin UI */
  nsCaps_lock();
  if ((prin == NULL) || (target == NULL) ||
      (itsPrinToMacroTargetPrivTable == NULL)) {
    nsCaps_unlock();
    return;
  }
  PrincipalKey prinKey(prin);
  nsPrivilegeTable *mpt = 
    (nsPrivilegeTable *)itsPrinToMacroTargetPrivTable->Get(&prinKey);
  if (mpt == NULL) {
    nsCaps_unlock();
    return;
  }
  mpt->remove(target);

  /* remove the prin/target from RDF also */
#ifdef DEBUG_raman_xxx
  RDF_RemovePrincipalsPrivilege(prin, target);
#endif /* DEBUG_raman_xxx */

  /* Regenerate the expnaded prvileges for this principal */
  nsPrivilegeTable *pt = (nsPrivilegeTable *)itsPrinToPrivTable->Get(&prinKey);
  if (pt != NULL) {
    delete pt;
  }
  gPrivilegeTable = pt = new nsPrivilegeTable();

  itsPrinToPrivTable->Put(&prinKey, pt);
  mpt->Enumerate(updatePrivileges);
  gPrivilegeTable = NULL;
  nsCaps_unlock();
}


//
// 			PRIVATE METHODS 
//

#define UNSIGNED_PRIN_KEY "4a:52:4f:53:4b:49:4e:44"
#define UNKNOWN_PRIN_KEY "52:4f:53:4b:49:4e:44:4a"


PRBool nsPrivilegeManager::enablePrivilegePrivate(nsTarget *target, 
                                                  nsPrincipal *prefPrin,
                                                  PRInt32 callerDepth)
{
  // default "data" as null
  if (PR_FALSE == enablePrincipalPrivilegeHelper(target, callerDepth, 
                                                 prefPrin, NULL, NULL)) {
    return PR_FALSE;
  }

  // default "data" as null
  if (NULL == enableScopePrivilegeHelper(target, callerDepth, NULL, PR_FALSE, 
                                         prefPrin))
    return PR_FALSE;
  return PR_TRUE;
}


nsPermissionState 
nsPrivilegeManager::getPrincipalPrivilege(nsTarget *target, 
                                          nsPrincipalArray* callerPrinArray,
                                          void *data)
{
  nsPrivilege *privilege;
  nsPrincipal *principal;
  PRBool isAllowed = PR_FALSE;

  for (int i = callerPrinArray->GetSize(); i-- > 0; ) {
    principal = (nsPrincipal *)callerPrinArray->Get(i);
    privilege = getPrincipalPrivilege(target, principal, data);
    if (privilege == NULL) {
      // the principal isn't registered, so ignore it
      continue;
    }

    switch(privilege->getPermission()) {
    case nsPermissionState_Allowed:
      isAllowed = PR_TRUE;
      // Fall through
    case nsPermissionState_Blank:
      continue;

    default:
      PR_ASSERT(FALSE);
    case nsPermissionState_Forbidden:
      return nsPermissionState_Forbidden;
    }
  }

  if (isAllowed) {
    return nsPermissionState_Allowed;
  }
  return nsPermissionState_Blank;
}

nsPrivilege *nsPrivilegeManager::getPrincipalPrivilege(nsTarget *target, 
                                                       nsPrincipal *prin, 
                                                       void *data)
{
  if (getSystemPrincipal() == prin) {
    return nsPrivilege::findPrivilege(nsPermissionState_Allowed, 
                                      nsDurationState_Forever);
  }

  PrincipalKey prinKey(prin);
  nsPrivilegeTable *privTable = (nsPrivilegeTable *) itsPrinToPrivTable->Get(&prinKey);
  if (privTable == NULL) {
    // the principal isn't registered, so ignore it
    return NULL;
  }

  nsTarget *tempTarget = nsTarget::findTarget(target);
  if (tempTarget != target) {
    // Target is not registered, so ignore it
    return NULL;
  }

  return privTable->get(target);
}

PRBool 
nsPrivilegeManager::isPermissionGranted(nsTarget *target, 
                                        nsPrincipalArray* callerPrinArray, 
                                        void *data)
{
  nsPermissionState privilege = 
    getPrincipalPrivilege(target, callerPrinArray, data);

  switch(privilege) {
  case nsPermissionState_Allowed:
    return PR_TRUE;

  default:
    // shouldn't ever happen! Fall Through
    PR_ASSERT(PR_FALSE);
  case nsPermissionState_Forbidden:
    /* throw new ForbiddenTargetException("access to target denied"); */

  case nsPermissionState_Blank:
    return PR_FALSE;
  }
}

/* The following method should call native code */

static struct NSJSJavaFrameWrapper * (*NewNSJSJavaFrameWrapperCallback)(void);

void 
setNewNSJSJavaFrameWrapperCallback(struct NSJSJavaFrameWrapper * (*fp)(void))
{
    NewNSJSJavaFrameWrapperCallback = fp;
}


static void (*FreeNSJSJavaFrameWrapperCallback)(struct NSJSJavaFrameWrapper *);

void 
setFreeNSJSJavaFrameWrapperCallback(void (*fp)(struct NSJSJavaFrameWrapper *))
{
    FreeNSJSJavaFrameWrapperCallback = fp;
}


static void (*GetStartFrameCallback)(struct NSJSJavaFrameWrapper *);

void 
setGetStartFrameCallback(void (*fp)(struct NSJSJavaFrameWrapper *))
{
    GetStartFrameCallback = fp;
}


static PRBool (*IsEndOfFrameCallback)(struct NSJSJavaFrameWrapper *);

void 
setIsEndOfFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *))
{
    IsEndOfFrameCallback = fp;
}


static PRBool (*IsValidFrameCallback)(struct NSJSJavaFrameWrapper *);

void 
setIsValidFrameCallback(PRBool (*fp)(struct NSJSJavaFrameWrapper *))
{
    IsValidFrameCallback = fp;
}


static void * (*GetNextFrameCallback)(struct NSJSJavaFrameWrapper *, int *);

void 
setGetNextFrameCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, int *))
{
    GetNextFrameCallback = fp;
}


static void * (*GetPrincipalArrayCallback)(struct NSJSJavaFrameWrapper *);

void 
setOJIGetPrincipalArrayCallback(void * (*fp)(struct NSJSJavaFrameWrapper *))
{
    GetPrincipalArrayCallback = fp;
}


static void * (*GetAnnotationCallback)(struct NSJSJavaFrameWrapper *);

void 
setOJIGetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *))
{
    GetAnnotationCallback = fp;
}


static void * (*SetAnnotationCallback)(struct NSJSJavaFrameWrapper *, void *);

void 
setOJISetAnnotationCallback(void * (*fp)(struct NSJSJavaFrameWrapper *, void *))
{
    SetAnnotationCallback = fp;
}

char *
nsPrivilegeManager::checkPrivilegeEnabled(nsTargetArray * targetArray, 
                                          PRInt32 callerDepth,
                                          void *data)
{
  struct NSJSJavaFrameWrapper *wrapper = NULL;
  nsTarget *target;
  nsPrivilegeTable *annotation;
  nsPrivilege *privilege;
  nsPrincipalArray * prinArray = theUnknownPrincipalArray;
  int depth = 0;
  nsPermissionState perm;
  nsPermissionState scopePerm;
  nsPermissionState prinPerm;
  PRBool saw_non_system_code = PR_FALSE;
  PRBool saw_unsigned_principal = PR_FALSE;
  PRInt32 noOfTargets;
  PRInt32 idx;
  char *errMsg = NULL; 

  if (targetArray == NULL) {
    return "internal error - null target array";
  }

  wrapper = (*NewNSJSJavaFrameWrapperCallback)();
  if (wrapper == NULL) {
    return NULL;
  }

  noOfTargets = targetArray->GetSize();

  for ((*GetStartFrameCallback)(wrapper); 
       (!(*IsEndOfFrameCallback)(wrapper));
       ) 
    {
      if ((*IsValidFrameCallback)(wrapper)) {
        if (depth >= callerDepth) {
          scopePerm = nsPermissionState_Blank;
          prinPerm = nsPermissionState_Blank;
          for (idx = 0; idx < noOfTargets; idx++) {
            target = (nsTarget *)targetArray->Get(idx);
            if (!target) {
              errMsg = "internal error - a null target in the Array";
              goto done;
            }

            annotation = (nsPrivilegeTable *) (*GetAnnotationCallback)(wrapper);
            prinArray = (nsPrincipalArray *) (*GetPrincipalArrayCallback)(wrapper);
            /*
             * frame->annotation holds a PrivilegeTable, describing
             * the scope privileges of this frame.  We'll check
             * if it permits the target, and if so, we just return.
             * If it forbids the target, we'll throw an exception,
             * and return.  If it's blank, things get funny.
             *
             * In the blank case, we need to continue walking up
             * the stack, looking for a non-blank privilege.  To
             * prevent "luring" attacks, these blank frames must
             * have the ability that they *could have requested*
             * privilege, but actually didn't.  This property
             * insures that we don't have a non-permitted (attacker)
             * class somewhere in the call chain between the request
             * for scope privilege and the chedk for privilege.
             */

            /*
             * If there isn't a annotation, then we assume the default
             * value of blank and avoid allocating a new annotation.
             */
            if (annotation) {
              privilege = annotation->get(target);
              PR_ASSERT(privilege != NULL);
              perm = privilege->getPermission();
              scopePerm = nsPrivilege::add(perm, scopePerm);
            }
            if (prinArray != NULL) {
              /* XXX: We need to allow sub-classing of Target, so that
               * we would call the method on Developer Sub-class'ed Target.
               * That would allow us to implement Parameterized targets
               * May be checkPrivilegeEnabled should go back into Java.
               */
              privilege = target->checkPrivilegeEnabled(prinArray,data);
              PR_ASSERT(privilege != NULL);
              perm = privilege->getPermission();
              scopePerm = nsPrivilege::add(perm, scopePerm);
              perm = getPrincipalPrivilege(target,prinArray,data);
              prinPerm = nsPrivilege::add(perm, prinPerm);
              if (!nsPrivilegeManager::hasSystemPrincipal(prinArray))
                saw_non_system_code = PR_TRUE;
            } else {
              saw_unsigned_principal = PR_TRUE;
            }
          }

          if (scopePerm == nsPermissionState_Allowed) {
            goto done;
          }
          if ((scopePerm == nsPermissionState_Forbidden) ||
              (saw_non_system_code && 
               (prinPerm != nsPermissionState_Allowed))) {
            errMsg = "access to target forbidden";
            goto done;
          }
        }
      }
      if (!(*GetNextFrameCallback)(wrapper, &depth))
        break;
    }
    /*
     * If we get here, there is no non-blank capability on the stack,
     * and there is no ClassLoader, thus give privilege for now
     */
    if (saw_non_system_code) {
      errMsg = "access to target forbidden. Target was not enabled on stack (stack included non-system code)";
      goto done;
    }
    if (CMGetBoolPref("signed.applets.local_classes_have_30_powers")) {
      goto done;
    }

    errMsg =  "access to target forbidden. Target was not enabled on stack (stack included only system code)";
    
done:
    (*FreeNSJSJavaFrameWrapperCallback)(wrapper);
    return errMsg;
}


nsPrincipalArray *
nsPrivilegeManager::getClassPrincipalsFromStack(PRInt32 callerDepth)
{
  nsPrincipalArray * principalArray = theUnknownPrincipalArray;
  int depth = 0;
  struct NSJSJavaFrameWrapper *wrapper = NULL;

  wrapper = (*NewNSJSJavaFrameWrapperCallback)();
  if (wrapper == NULL)
    return NULL;

  for ((*GetStartFrameCallback)(wrapper) ; 
       (!(*IsEndOfFrameCallback)(wrapper)) ;
       ) {
    if ((*IsValidFrameCallback)(wrapper)) {
      if (depth >= callerDepth) {
        principalArray = (nsPrincipalArray *) (*GetPrincipalArrayCallback)(wrapper);
	break;
      }
    }
    if (!(*GetNextFrameCallback)(wrapper, &depth))
      break;
  }
  (*FreeNSJSJavaFrameWrapperCallback)(wrapper);
  return principalArray;
}


nsPrivilegeTable * 
nsPrivilegeManager::getPrivilegeTableFromStack(PRInt32 callerDepth, 
                                               PRBool createIfNull)
{
  nsPrivilegeTable *privTable = NULL;
  int depth = 0;
  struct NSJSJavaFrameWrapper *wrapper = NULL;
  nsPrivilegeTable *annotation;

  wrapper = (*NewNSJSJavaFrameWrapperCallback)();
  if (wrapper == NULL)
    return NULL;

  for ((*GetStartFrameCallback)(wrapper) ; 
       (!(*IsEndOfFrameCallback)(wrapper)) ;
       ) {
    if ((*IsValidFrameCallback)(wrapper)) {
      if (depth >= callerDepth) {
        /*
         * it's possible for the annotation to be NULL, meaning
         * nobody's ever asked for it before.  The correct
         * response is to create a new PrivilegeTable (with
         * default "blank forever" privileges), assign that
         * to the annotation, and return it.
         */
        annotation = (nsPrivilegeTable *) (*GetAnnotationCallback)(wrapper);
        if (createIfNull && annotation == NULL) {
          privTable = new nsPrivilegeTable();
          if (privTable == NULL) {
            /*
             * no memory?!??!  return NULL, and let our
             * parent deal with the exception 
             */
            break;
          }
          PR_ASSERT(privTable != NULL);
          (*SetAnnotationCallback)(wrapper, privTable);
        } else {
          privTable = annotation;
        }
        break;
      }
    }
    if (!(*GetNextFrameCallback)(wrapper, &depth))
      break;
  }
  (*FreeNSJSJavaFrameWrapperCallback)(wrapper);
  return privTable;
}

#ifdef DEBUG_raman_xxx
JSec_Principal 
RDF_CreatePrincipal(nsPrincipal *prin)
{
  /* For certificate principals we should convert the key into string, because 
   * key could have NULL characters before we store it in RDF. 
   */
  char *key = prin->getKey();
  PRUint32 keyLen = prin->getKeyLength();
  char *prinName = PL_Base64Encode(key, keyLen, NULL);
  JSec_Principal pr = RDFJSec_NewPrincipal(prinName);
  char buf[256];
  XP_SPRINTF(buf, "%d", keyLen);
  RDFJSec_SetPrincipalAttribute(pr, "keyLen", (void *)buf);
  nsPrincipalType type = prin->getType();
  XP_SPRINTF(buf, "%d", type);
  RDFJSec_SetPrincipalAttribute(pr, "type", (void *)buf);
  RDFJSec_AddPrincipal(pr);
  return pr;
}

static PRBool 
RDF_RemovePrincipal(nsPrincipal *prin)
{
  nsCaps_lock();

  RDFJSec_InitPrivilegeDB();
  RDF_Cursor prin_cursor = RDFJSec_ListAllPrincipals();
  if (prin_cursor == NULL) {
    nsCaps_unlock();
    return PR_FALSE;
  }
  
  JSec_Principal jsec_prin;
  nsPrincipal *cur_prin = NULL;
  PRBool found = PR_FALSE;
  while ((jsec_prin = RDFJSec_NextPrincipal(prin_cursor)) != NULL) {
    if ((cur_prin = RDF_getPrincipal(jsec_prin)) == NULL) {
      continue;
    }
    if (prin->equals(cur_prin)) {
      found = PR_TRUE;
      break;
    }
  }

  RDFJSec_ReleaseCursor(prin_cursor);
  if (found) {
    RDFJSec_DeletePrincipal(jsec_prin);
  }
  nsCaps_unlock();
  return found;
}

static nsPrincipal *
RDF_getPrincipal(JSec_Principal jsec_pr)
{
  char *enc_key = RDFJSec_PrincipalID(jsec_pr);
  char *key = PL_Base64Decode(enc_key, 0, NULL);
  char *key_len_str = (char *)RDFJSec_AttributeOfPrincipal(jsec_pr, "keyLen");
  PRUint32 keyLen;
  sscanf(key_len_str, "%d", &keyLen);
  PRInt32 type_int;
  nsPrincipalType type;
  char *type_str = (char *)RDFJSec_AttributeOfPrincipal(jsec_pr, "type");
  sscanf(type_str, "%d", &type_int);
  type = (nsPrincipalType)type_int;
  nsPrincipal *prin = new nsPrincipal(type, key, keyLen);
  PR_Free(key);
  PR_Free(enc_key);
  return prin;
}

JSec_Target 
RDF_CreateTarget(nsTarget *target)
{
  char *targetName = target->getName();
  nsPrincipal *prin = target->getPrincipal();
  JSec_Principal pr = RDF_CreatePrincipal(prin);
  return RDFJSec_NewTarget(targetName, pr);
}

nsTarget *
RDF_getTarget(JSec_Target jsec_target)
{
  char *targetName = RDFJSec_GetTargetName(jsec_target);
  return nsTarget::findTarget(targetName);
}

static PRBool 
RDF_RemovePrincipalsPrivilege(nsPrincipal *prin, nsTarget *target)
{
  nsCaps_lock();

  RDFJSec_InitPrivilegeDB();
  RDF_Cursor prin_cursor = RDFJSec_ListAllPrincipals();
  if (prin_cursor == NULL) {
    nsCaps_unlock();
    return PR_FALSE;
  }
  
  JSec_Principal jsec_prin;
  nsPrincipal *cur_prin = NULL;
  PRBool found = PR_FALSE;
  JSec_PrincipalUse jsec_pr_use = NULL;

  while ((jsec_prin = RDFJSec_NextPrincipal(prin_cursor)) != NULL) {
    /* Find the principal */
    if ((cur_prin = RDF_getPrincipal(jsec_prin)) == NULL) {
      continue;
    }
    if (!cur_prin->equals(prin)) {
      continue;
    }
    /* Found the principal. Find the target for this principal */ 
    RDF_Cursor prin_use_cursor = RDFJSec_ListAllPrincipalUses(jsec_prin);
    while ((jsec_pr_use = RDFJSec_NextPrincipalUse(prin_use_cursor)) != NULL) {
      JSec_Target jsec_target = RDFJSec_TargetOfPrincipalUse(jsec_pr_use);
      if (jsec_target == NULL) {
        continue;
      }
      nsTarget *cur_target = RDF_getTarget(jsec_target);
      if ((cur_target == NULL) || (!cur_target->equals(target))) {
        continue;
      }
      found = PR_TRUE;
      break;
    }
    RDFJSec_ReleaseCursor(prin_use_cursor);
    if (found) {
      break;
    }
  }

  RDFJSec_ReleaseCursor(prin_cursor);
  if (found) {
    RDFJSec_DeletePrincipalUse(jsec_prin, jsec_pr_use);
  }
  nsCaps_unlock();
  return found;
}
#endif /* DEBUG_raman_xxx */


/* The following methods are used to save and load the persistent store */

void nsPrivilegeManager::save(nsPrincipal *prin, 
                              nsTarget *target, 
                              nsPrivilege *newPrivilege)
{
  /* Don't save permissions for system Principals */
  if (prin->equals(getSystemPrincipal())) {
    return;
  }
#ifdef DEBUG_raman_xxx
  nsCaps_lock();
  RDFJSec_InitPrivilegeDB();
  JSec_Principal pr = RDF_CreatePrincipal(prin);
  JSec_Target tr = RDF_CreateTarget(target);
  char *privilege = newPrivilege->toString();
  JSec_PrincipalUse prUse = RDFJSec_NewPrincipalUse(pr, tr, privilege);
  RDFJSec_AddPrincipalUse(pr, prUse);
  
  nsCaps_unlock();
#endif /* DEBUG_raman_xxx */
}

/* The following routine should be called after setting up the system targets 
 * XXX: May be we should add a PR_ASSERT for that.
 */
void nsPrivilegeManager::load(void)
{
#ifdef DEBUG_raman_xxx
  nsCaps_lock();
  RDFJSec_InitPrivilegeDB();
  RDF_Cursor prin_cursor = RDFJSec_ListAllPrincipals();
  if (prin_cursor == NULL) {
    nsCaps_unlock();
    return;
  }
  
  JSec_Principal jsec_prin;
  nsPrincipal *prin;
  while ((jsec_prin = RDFJSec_NextPrincipal(prin_cursor)) != NULL) {
    if ((prin = RDF_getPrincipal(jsec_prin)) == NULL) {
      continue;
    }
    RDF_Cursor prin_use_cursor = RDFJSec_ListAllPrincipalUses(jsec_prin);
    JSec_PrincipalUse jsec_pr_use;
    while ((jsec_pr_use = RDFJSec_NextPrincipalUse(prin_use_cursor)) != NULL) {
      char *privilege_str = RDFJSec_PrivilegeOfPrincipalUse(jsec_pr_use);
      if (privilege_str == NULL) {
        continue;
      }
      JSec_Target jsec_target = RDFJSec_TargetOfPrincipalUse(jsec_pr_use);
      if (jsec_target == NULL) {
        continue;
      }
      nsTarget *target = RDF_getTarget(jsec_target);
      if (target == NULL) {
        continue;
      }
      nsPrivilege *privilege = nsPrivilege::findPrivilege(privilege_str);
      registerPrincipalAndSetPrivileges(prin, target, privilege);
    }
    RDFJSec_ReleaseCursor(prin_use_cursor);
  }

  RDFJSec_ReleaseCursor(prin_cursor);
  nsCaps_unlock();
#endif /* DEBUG_raman_xxx */
}


PRBool nsPrivilegeManagerInitialize(void) 
{
  /* XXX: How do we determine SystemPrincipal so that we can create System 
     Targets? Are all SystemTargtes Java specific only. What about JS 
     specific targets
   */
  theUnsignedPrincipal = new nsPrincipal(nsPrincipalType_Cert, 
                                         UNSIGNED_PRIN_KEY, 
                                         strlen(UNSIGNED_PRIN_KEY));
  theUnsignedPrincipalArray = new nsPrincipalArray();
  theUnsignedPrincipalArray->Add(theUnsignedPrincipal);

  theUnknownPrincipal = new nsPrincipal(nsPrincipalType_Cert, 
                                        UNKNOWN_PRIN_KEY, 
                                        strlen(UNKNOWN_PRIN_KEY));
  theUnknownPrincipalArray = new nsPrincipalArray();
  theUnknownPrincipalArray->Add(theUnknownPrincipal);

  thePrivilegeManager = new nsPrivilegeManager();
#ifdef DEBUG_raman_xxx
  RDFJSec_InitPrivilegeDB();
#endif /* DEBUG_raman_xxx */
  return PR_FALSE;
}

PRBool nsPrivilegeManager::theInited = nsPrivilegeManagerInitialize();


