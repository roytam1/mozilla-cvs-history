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

#include "nsPermissions.h"
#include "nsUtils.h"

#include "nsVoidArray.h"
#include "xp_core.h"
#include "prmem.h"
#include "nsIPref.h"
#include "nsTextFormatter.h"
#include "nsIServiceManager.h"

#define image_behaviorPref "network.image.imageBehavior"
#define image_warningPref "network.image.warnAboutImages"

typedef struct _permission_HostStruct {
  char * host;
  nsVoidArray * permissionList;
} permission_HostStruct;

typedef struct _permission_TypeStruct {
  PRInt32 type;
  PRBool permission;
} permission_TypeStruct;

PRIVATE PERMISSION_BehaviorEnum image_behavior = PERMISSION_Accept;
PRIVATE PRBool image_warning = PR_FALSE;

PRIVATE void
image_SetBehaviorPref(PERMISSION_BehaviorEnum x) {
  image_behavior = x;
}

PRIVATE void
image_SetWarningPref(PRBool x) {
  image_warning = x;
}

PRIVATE PERMISSION_BehaviorEnum
image_GetBehaviorPref() {
  return image_behavior;
}

PRIVATE PRBool
image_GetWarningPref() {
  return image_warning;
}

MODULE_PRIVATE int PR_CALLBACK
image_BehaviorPrefChanged(const char * newpref, void * data) {
  PRInt32 n;
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_FAILED(prefs->GetIntPref(image_behaviorPref, &n))) {
    image_SetBehaviorPref(PERMISSION_Accept);
  } else {
    image_SetBehaviorPref((PERMISSION_BehaviorEnum)n);
  }
  return 0;
}

MODULE_PRIVATE int PR_CALLBACK
image_WarningPrefChanged(const char * newpref, void * data) {
  PRBool x;
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_FAILED(prefs->GetBoolPref(image_warningPref, &x))) {
    x = PR_FALSE;
  }
  image_SetWarningPref(x);
  return 0;
}

PUBLIC void
IMAGE_RegisterPrefCallbacks(void) {
  PRInt32 n;
  PRBool x;
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);

  // Initialize for image_behaviorPref
  if (NS_FAILED(prefs->GetIntPref(image_behaviorPref, &n))) {
    n = PERMISSION_Accept;
  }
  image_SetBehaviorPref((PERMISSION_BehaviorEnum)n);
  prefs->RegisterCallback(image_behaviorPref, image_BehaviorPrefChanged, NULL);

  // Initialize for image_warningPref
  if (NS_FAILED(prefs->GetBoolPref(image_warningPref, &x))) {
    x = PR_FALSE;
  }
  image_SetWarningPref(x);
  prefs->RegisterCallback(image_warningPref, image_WarningPrefChanged, NULL);
}

PUBLIC nsresult
IMAGE_CheckForPermission
    (const char * hostname, const char * firstHostname, PRBool *permission) {

  /* exit if imageblocker is not enabled */
  nsresult rv;
  PRBool prefvalue = PR_FALSE;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
  if (NS_FAILED(rv) || 
      NS_FAILED(prefs->GetBoolPref("imageblocker.enabled", &prefvalue)) ||
      !prefvalue) {
    *permission = (image_GetBehaviorPref() != PERMISSION_DontUse);
    return NS_OK;
  }

  /* try to make a decision based on pref settings */
  if ((image_GetBehaviorPref() == PERMISSION_DontUse)) {
    *permission = PR_FALSE;
    return NS_OK;
  }
  if (image_GetBehaviorPref() == PERMISSION_DontAcceptForeign) {
    /* compare tails of names checking to see if they have a common domain */
    /* we do this by comparing the tails of both names where each tail includes at least one dot */
    PRInt32 dotcount = 0;
    const char * tailHostname = hostname + PL_strlen(hostname) - 1;
    while (tailHostname > hostname) { 
      if (*tailHostname == '.') {
        dotcount++;
      }
      if (dotcount == 2) {
        tailHostname++;
        break;
      }
      tailHostname--;
    }
    dotcount = 0;
    const char * tailFirstHostname = firstHostname + PL_strlen(firstHostname) - 1;
    while (tailFirstHostname > firstHostname) { 
      if (*tailFirstHostname == '.') {
        dotcount++;
      }
      if (dotcount == 2) {
        tailFirstHostname++;
        break;
      }
      tailFirstHostname--;
    }
    if (PL_strcmp(tailFirstHostname, tailHostname)) {
      *permission = PR_FALSE;
      return NS_OK;
    }
  }

  /* use common routine to make decision */
  PRUnichar * message = CKutil_Localize(NS_LITERAL_STRING("PermissionToAcceptImage").get());
  PRUnichar * new_string = nsTextFormatter::smprintf(message, hostname ? hostname : "");
  if (NS_SUCCEEDED(PERMISSION_Read())) {
    *permission = Permission_Check(0, hostname, IMAGEPERMISSION,
                                   image_GetWarningPref(), new_string);
  } else {
    *permission = PR_TRUE;
  }
  PR_FREEIF(new_string);
  Recycle(message);
  return NS_OK;
}

PUBLIC nsresult
IMAGE_Block(const char* imageURL) {
  if (!imageURL || !(*imageURL)) {
    return NS_ERROR_NULL_POINTER;
  }
  char *host = CKutil_ParseURL(imageURL, GET_HOST_PART);
  Permission_AddHost(host, PR_FALSE, IMAGEPERMISSION, PR_TRUE);
  return NS_OK;
}
