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

/* Dll
 *
 * Programmatic representation of a dll. Stores modifiedTime and size for
 * easy detection of change in dll.
 *
 * dp Suresh <dp@netscape.com>
 */

#include "prio.h"
#include "prlink.h"

typedef enum nsDllStatus
{
	DLL_OK = 0,
	DLL_NO_MEM = 1,
	DLL_STAT_ERROR = 2,
	DLL_NOT_FILE = 3,
	DLL_INVALID_PARAM = 4
} nsDllStatus;

class nsDll
{
private:
	char *m_fullpath;		// system format full filename of dll
	PRTime m_lastModTime;	// last modified time
	PRUint32 m_size;		// size of the dynamic library
	PRLibrary *m_instance;	// Load instance
	nsDllStatus m_status;		// holds current status

public:
 
	nsDll(const char *libFullPath);
	nsDll(const char *libFullPath, PRTime lastModTime, PRUint32 fileSize);

	~nsDll(void);

	// Status checking on operations completed
	nsDllStatus GetStatus(void) { return (m_status); }

	// Dll Loading
	PRBool Load(void);
	PRBool Unload(void);
	PRBool IsLoaded(void)
	{
		return ((m_instance != 0) ? PR_TRUE : PR_FALSE);
	}
	void *FindSymbol(const char *symbol);
	
	const char *GetFullPath(void) { return (m_fullpath); }
	PRTime GetLastModifiedTime(void) { return(m_lastModTime); }
	PRUint32 GetSize(void) { return(m_size); }
	PRLibrary *GetInstance(void) { return (m_instance); }
};
