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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


/*
   singsign.h --- prototypes for wallet functions.
*/


#ifndef _SINGSIGN_H
#define _SINGSIGN_H

#include "ntypes.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsIPref.h"

/* Duplicates defines as in nsIPrompt.idl -- keep in sync! */
#define SINGSIGN_SAVE_PASSWORD_NEVER 0
#define SINGSIGN_SAVE_FOR_SESSION    1
#define SINGSIGN_SAVE_PERMANENTLY    2

class nsIPrompt;
XP_BEGIN_PROTOS

extern void
SINGSIGN_GetSignonListForViewer (nsString& aSignonList);

extern void
SINGSIGN_GetRejectListForViewer (nsString& aRejectList);

extern void
SINGSIGN_SignonViewerReturn(const nsString& results);

extern void
SINGSIGN_RestoreSignonData(nsIPrompt* dialog, const char* passwordRealm, const PRUnichar* name, PRUnichar** value, PRUint32 elementNumber);

extern nsresult
SINGSIGN_PromptUsernameAndPassword
    (const PRUnichar *dialogTitle, const PRUnichar *text, PRUnichar **user, PRUnichar **pwd,
     const char* passwordRealm, nsIPrompt* dialog, PRBool *returnValue, PRUint32 savePassword = SINGSIGN_SAVE_PERMANENTLY);

extern nsresult
SINGSIGN_PromptPassword
    (const PRUnichar *dialogTitle, const PRUnichar *text, PRUnichar **pwd, const char* passwordRealm,
     nsIPrompt* dialog, PRBool *returnValue, PRUint32 savePassword = SINGSIGN_SAVE_PERMANENTLY);

extern nsresult
SINGSIGN_Prompt
    (const PRUnichar *dialogTitle, const PRUnichar *text, const PRUnichar *defaultText, PRUnichar **resultText,
     const char* passwordRealm, nsIPrompt* dialog, PRBool *returnValue, PRUint32 savePassword = SINGSIGN_SAVE_PERMANENTLY);

extern PRBool
SINGSIGN_RemoveUser
    (const char* passwordRealm, const PRUnichar *userName);

extern PRBool
SINGSIGN_StorePassword
    (const char* passwordRealm, const PRUnichar *userName, const PRUnichar *password);

extern nsresult
SINGSIGN_HaveData(nsIPrompt* dialog, const char* passwordRealm, const PRUnichar *userName, PRBool *retval);

extern void
SI_RegisterCallback(const char* domain, PrefChangedFunc callback, void* instance_data);

extern PRBool
SI_GetBoolPref(const char * prefname, PRBool defaultvalue);

extern void
SI_SetBoolPref(const char * prefname, PRBool prefvalue);

extern void
SI_SetCharPref(const char * prefname, const char * prefvalue);

extern void
SI_GetCharPref(const char * prefname, char** aPrefvalue);

extern void SI_InitSignonFileName();

extern PRBool
SI_InSequence(const nsString& sequence, int number);

extern PRUnichar*
SI_FindValueInArgs(const nsString& results, const nsString& name);

extern void
SI_DeleteAll();

extern PRBool
SINGSIGN_ReencryptAll();

extern void
SINGSIGN_RememberSignonData(nsIPrompt* dialog, const char* URLName, nsVoidArray * signonData);

XP_END_PROTOS

#endif /* !_SINGSIGN_H */
