/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*
   addrbk.c	- a hack at the new address book STUFF

	       Created:    Benjie Chen - 6/8/96
	       Copied lots of code from hot.c
	       Then deleted lots of code from hot.c
	       Then deleted lots of code from here
	       Then faked bunch of code here

	       Modified:   Major changes - 7/17
	       		   - I am separating the addressing window stuff
			     from the address book stuff and putting the
			     addressing window stuff off mailcompose window
			     to allow multiple addressing windows and
			     multiple mailcompose window instances - Benjie
*/



#include "mozilla.h"
#include "xp_file.h"
#include "addrbk.h"


/* list of DIR_Server 
*/
static ABook*       AddrBook = NULL; 
extern ABook*       fe_GetABook(MWContext *); 

/*****************************************************
 * Actual code begins here 
 */
extern ABook* fe_GetABook(MWContext *context) 
{
  return AddrBook;
}

extern XP_List* FE_GetDirServers()
{
  	char tmp[256];
	XP_List* m_directories = XP_ListNew();

#ifdef MOZ_MAIL_NEWS
	/* database file is not html anymore ... 
	 */
	PR_snprintf(tmp, sizeof (tmp), "abook.nab"); 
	DIR_GetServerPreferences (&m_directories, tmp);
#endif /* MOZ_MAIL_NEWS */
	return m_directories;
}

ABook* FE_GetAddressBook(MSG_Pane *pane)
{
	return AddrBook;
}

#ifdef MOZ_MAIL_NEWS
void FE_InitAddrBook() 
{

	static XP_List *directories = NULL;
	char     *oldFile;
	XP_File  oldFp = 0;

  	char     tmp[1024];
	char	*tmp2;
  	char    *home = getenv("HOME");
  	/*DIR_Server *dir;*/

  	if (!home) home = "";

	oldFile = fe_GetConfigDirFilename("addrbook.db"); 
	if (oldFile) {
		oldFp = XP_FileOpen(oldFile, xpAddrBook, "r");
		if (oldFp) {
			char    newFile[256];
			XP_File newFp = 0;

			/* extern int XP_FileClose(XP_File file);
			 */
			XP_FileClose(oldFp);

			PR_snprintf(newFile, sizeof (newFile), "%s", "abook.nab");
			newFp = XP_FileOpen(newFile, xpAddrBookNew, "r");
			if (!newFp) {
				/* Rename file for backward compatibility reason
				 *   extern int XP_FileRename(const char * from, XP_FileType fromtype,
				 *     const char * to,   XP_FileType totype);
				 */
				XP_FileRename(oldFile, xpAddrBook,
							  newFile, xpAddrBookNew);
			}/* !newFp */
			else
				XP_FileClose(newFp);
	
		}/* if */
		free(oldFile);
	}

	/* all right, lets do the list of directories and stuff */
	directories = XP_ListNew();

	/* first the addressbook stuff */
	/* database file is not html anymore ... */
	PR_snprintf(tmp, sizeof (tmp), "abook.nab"); 

	DIR_GetServerPreferences (&directories, tmp);
	tmp2 = fe_GetConfigDirFilename("address-book.html");
	if (tmp2)
	{
		{
#ifndef MOZ_NEWADDR
			DIR_Server *pabDir = NULL;
			DIR_GetPersonalAddressBook(directories, &pabDir);
			AB_InitializeAddressBook(pabDir, &AddrBook, tmp);
#endif
		}
		free (tmp2);
	}
}

void FE_CloseAddrBook() {
#ifndef MOZ_NEWADDR
  AB_CloseAddressBook(&AddrBook);
#endif
}

#else
#if 0
/* compiler complains about definition vs. previous def at
   line 154 in file '../../include/dirprefs.h.  Dunno why, they're identical.
 */
extern XP_List* FE_GetDirServers(void)
{
  return 0 ;
}
#endif
#endif

