/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include "nsString.h"

class nsIURI;

#include "nsCookies.h"

#define COOKIEPERMISSION 0
#define IMAGEPERMISSION 1
#define WINDOWPERMISSION 2
#define NUMBER_OF_PERMISSIONS 3

typedef enum {
  PERMISSION_Accept,
  PERMISSION_DontAcceptForeign,
  PERMISSION_DontUse,
  PERMISSION_P3P
} PERMISSION_BehaviorEnum;

class nsIPrompt;

extern nsresult PERMISSION_Read();
extern void PERMISSION_Add(nsIURI * objectURI, PRBool permission, PRInt32 type);
extern void PERMISSION_TestForBlocking(nsIURI * objectURL, PRBool* blocked, PRInt32 type);
extern void PERMISSION_RemoveAll();
extern void PERMISSION_DeletePersistentUserData(void);

extern PRInt32 PERMISSION_HostCount();
extern PRInt32 PERMISSION_TypeCount(PRInt32 host);
extern nsresult PERMISSION_Enumerate
  (PRInt32 hostNumber, PRInt32 typeNumber, char **host, PRInt32 *type, PRBool *capability);
extern void PERMISSION_Remove(const nsACString & host, PRInt32 type);

extern PRBool Permission_Check
  (nsIPrompt *aPrompter, const char * hostname, PRInt32 type,
   PRBool warningPref, cookie_CookieStruct * cookie_s, const char * message_string, int count_for_message);
extern nsresult Permission_AddHost
  (const nsAFlatCString &host, PRBool permission, PRInt32 type, PRBool save);
extern nsresult permission_CheckFromList
  (const char* hostname, PRBool &permission, PRInt32 type);
//extern void Permission_Free(PRInt32 hostNumber, PRInt32 type, PRBool save);
extern void Permission_Save(PRBool notify);

#endif /* PERMISSIONS_H */
