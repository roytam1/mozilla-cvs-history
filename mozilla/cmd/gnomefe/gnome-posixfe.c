/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*   gnome-posixfe.c --- stuff that should live in posixfe */

#include "structs.h"

void fe_GetProgramDirectory(char *path, int len)
{
}

void
FE_ShowMinibuffer(MWContext *context)
{
  printf("FE_ShowMinibuffer (empty)\n");
}

void FEU_StayingAlive(void)
{
}

void FE_AlternateCompose(
    char * from, char * reply_to, char * to, char * cc, char * bcc,
    char * fcc, char * newsgroups, char * followup_to,
    char * organization, char * subject, char * references,
    char * other_random_headers, char * priority,
    char * attachment, char * newspost_url, char * body)
{
}

extern
XP_Bool
FE_GetLabelAndMnemonic(char* name, char** str, void* v_xm_str, void* v_mnemonic)

{
    printf( "FE_GetLabelAndMnemonic %s, %p, %p, %p\n",
	name, str, v_xm_str, v_mnemonic);
    return 0;
}
