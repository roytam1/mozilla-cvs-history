/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIProfile.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIRegistry.h"
#include "nsFileSpec.h"
#include "nsString.h"
#include "nsICmdLineService.h"
#include "nsProfileAccess.h"

// Interfaces Needed
#include "nsIXULWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIURIContentListener.h"

#define _MAX_LENGTH             256

class nsProfile: public nsIProfile, public nsIURIContentListener, public nsIInterfaceRequestor
{
	NS_DECL_ISUPPORTS
	NS_DECL_NSIPROFILE

  // these are necessary if the profile manager is going to load urls for
  // the registration stuff....
  NS_DECL_NSIURICONTENTLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

private:
	nsresult ProcessArgs(nsICmdLineService *service,
					   PRBool *profileDirSet,
					   nsCString & profileURLStr);
	nsresult LoadDefaultProfileDir(nsCString & profileURLStr);
	PRBool mAutomigrate;
   nsCOMPtr<nsIXULWindow> mPregWindow;

public:
	nsProfile();
	virtual ~nsProfile();

	nsresult RenameProfileDir(const char *newProfileName);

	// Creates associated user directories on the creation of a new profile
    nsresult CreateUserDirectories(const nsFileSpec& profileDir);

	// Deletes associated user directories
	nsresult DeleteUserDirectories(const nsFileSpec& profileDir);

	// Copies all the registry keys from old profile to new profile
	nsresult CopyRegKey(const char *oldProfile, const char *newProfile);

	// Routine that frees the memory allocated to temp profile struct
	void FreeProfileStruct(ProfileStruct* aProfile);

	nsresult AutoMigrate();

	nsresult CreateDefaultProfile(void);
	nsresult TriggerActivation(const char *profileName);
	nsresult CleanUp();
	nsresult CheckDomain(PRBool *valid, char *domainName);
};

