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
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsOutlookImport_h___
#define nsOutlookImport_h___

#include "nsIImportModule.h"
#include "nsCOMPtr.h"


#define NS_OUTLOOKIMPORT_CID					\
{ /* 1DB469A0-8B00-11d3-A206-00A0CC26DA63 */      \
	0x1db469a0, 0x8b00, 0x11d3,						\
	{0xa2, 0x6, 0x0, 0xa0, 0xcc, 0x26, 0xda, 0x63 }}




#define kOutlookSupportsString NS_IMPORT_MAIL_STR "," NS_IMPORT_ADDRESS_STR "," NS_IMPORT_SETTINGS_STR

class nsOutlookImport : public nsIImportModule
{
public:

	nsOutlookImport();
	virtual ~nsOutlookImport();
	
	NS_DECL_ISUPPORTS

	////////////////////////////////////////////////////////////////////////////////////////
	// we suppport the nsIImportModule interface 
	////////////////////////////////////////////////////////////////////////////////////////


	/* readonly attribute wstring name; */
	NS_IMETHOD GetName(PRUnichar * *aName);
	
	/* readonly attribute wstring description; */
	NS_IMETHOD GetDescription(PRUnichar * *aDescription);
	
	/* readonly attribute string supports; */
	NS_IMETHOD GetSupports(char * *aSupports);
	
	/* nsISupports GetImportInterface (in string importType); */
	NS_IMETHOD GetImportInterface(const char *importType, nsISupports **_retval);
		
protected:		
};

extern nsresult NS_NewOutlookImport(nsIImportModule** aImport);


#endif /* nsOutlookImport_h___ */
