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

 
/* nsDllStore
 *
 * Stores dll and their accociated info in a hash keyed on the system format
 * full dll path name e.g C:\Program Files\Netscape\Program\raptor.dll
 *
 * NOTE: dll names are considered to be case sensitive.
 */

#include "plhash.h"
#include "xcDll.h"

class nsDllStore
{
private:
	PLHashTable *m_dllHashTable;

public:
	// Constructor
	nsDllStore(void);
	~nsDllStore(void);

	// Caller is not expected to delete nsDll returned
	// The nsDll returned in NOT removed from the hash
	nsDll* Get(const char *filename);
	PRBool Put(const char *filename, nsDll *dllInfo);

	// The nsDll returned is removed from the hash
	// Caller is expected to delete the returned nsDll
	nsDll* Remove(const char *filename);
};
