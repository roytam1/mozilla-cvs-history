/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsLocalFolderSummarySpec.h"
#include "plstr.h"
#include "nsString.h"

MOZ_DECL_CTOR_COUNTER(nsLocalFolderSummarySpec);

nsLocalFolderSummarySpec::~nsLocalFolderSummarySpec()
{
	MOZ_COUNT_DTOR(nsLocalFolderSummarySpec);

}

nsLocalFolderSummarySpec::nsLocalFolderSummarySpec()
{
	MOZ_COUNT_CTOR(nsLocalFolderSummarySpec);
}

nsLocalFolderSummarySpec::nsLocalFolderSummarySpec(const char *folderPath, PRBool create)
  : nsFileSpec(folderPath, create)
{
	MOZ_COUNT_CTOR(nsLocalFolderSummarySpec);
	CreateSummaryFileName();
}

nsLocalFolderSummarySpec::nsLocalFolderSummarySpec(const nsFileSpec& inFolderPath)
: nsFileSpec(inFolderPath)
{
	MOZ_COUNT_CTOR(nsLocalFolderSummarySpec);
	CreateSummaryFileName();
}

nsLocalFolderSummarySpec::nsLocalFolderSummarySpec(const nsFilePath &inFolderPath, PRBool create) : nsFileSpec(inFolderPath, create)
{
	MOZ_COUNT_CTOR(nsLocalFolderSummarySpec);
	CreateSummaryFileName();
}

void nsLocalFolderSummarySpec::SetFolderName(const char *folderPath)
{
	*this = folderPath;
}

void nsLocalFolderSummarySpec::	CreateSummaryFileName()
{
	char *leafName = GetLeafName();

	nsString fullLeafName(leafName);

	// Append .msf (msg summary file) this is what windows will want.
	// Mac and Unix can decide for themselves.

	fullLeafName += ".msf";				// message summary file
	char *cLeafName = fullLeafName.ToNewCString();
	SetLeafName(cLeafName);
    nsAllocator::Free(cLeafName);
	PL_strfree(leafName);
}

