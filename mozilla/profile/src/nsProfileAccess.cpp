/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsProfileAccess.h"
#include "nsProfile.h"

#include "pratom.h"
#include "prmem.h"
#include "plstr.h"
#include "prenv.h"

#include "nsIEnumerator.h"
#include "prprf.h"
#include "nsSpecialSystemDirectory.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIFileSpec.h"
#include "nsFileStream.h"
#include "nsEscape.h"

#define REGISTRY_YES_STRING             "yes"
#define REGISTRY_NO_STRING              "no"

// strings for items in the registry we'll be getting or setting
#define REGISTRY_PROFILE_SUBTREE_STRING     "Profiles"
#define REGISTRY_CURRENT_PROFILE_STRING     "CurrentProfile"
#define REGISTRY_NC_SERVICE_DENIAL_STRING   "NCServiceDenial"
#define REGISTRY_NC_PROFILE_NAME_STRING     "NCProfileName"
#define REGISTRY_NC_USER_EMAIL_STRING       "NCEmailAddress"
#define REGISTRY_NC_HAVE_PREG_INFO_STRING   "NCHavePregInfo"
#define REGISTRY_HAVE_PREG_INFO_STRING      "HavePregInfo"
#define REGISTRY_MIGRATED_STRING            "migrated"
#define REGISTRY_DIRECTORY_STRING           "directory"
#define REGISTRY_NEED_MIGRATION_STRING      "NeedMigration"

#define REGISTRY_VERSION_STRING				"Version"
#define REGISTRY_VERSION_1_0				"1.0"		

#define MAX_PERSISTENT_DATA_SIZE			1000
#define NUM_HEX_BYTES						8
#define ISHEX(c) ( ((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F') )

#if defined (XP_UNIX)
#define USER_ENVIRONMENT_VARIABLE "USER"
#define HOME_ENVIRONMENT_VARIABLE "HOME"
#define PROFILE_NAME_ENVIRONMENT_VARIABLE "PROFILE_NAME"
#define PROFILE_HOME_ENVIRONMENT_VARIABLE "PROFILE_HOME"
#elif defined (XP_BEOS)
#endif

// IID and CIDs of all the services needed
static NS_DEFINE_CID(kRegistryCID, NS_REGISTRY_CID);

/*
 * Constructor/Destructor
 * FillProfileInfo reads the registry and fills profileStructs
 */
nsProfileAccess::nsProfileAccess()
{
	m_registry		= null_nsCOMPtr();
	mCount			= 0;
	mNumProfiles		= 0; 
	mNumOldProfiles		= 0;
	m4xCount		= 0;
	mCurrentProfile		= nsnull;
	mVersion		= nsnull;
	mHavePREGInfo		= nsnull;
	mFixRegEntries		= PR_FALSE;
	mProfileDataChanged	= PR_FALSE;
	mForgetProfileCalled = PR_FALSE;
	mProfiles = new nsVoidArray();
	m4xProfiles = new nsVoidArray();

	FillProfileInfo();
}

// On the way out, close the registry if it is 
// still opened and free up the resources.
nsProfileAccess::~nsProfileAccess() 
{

    PRBool openalready = PR_FALSE;

	// Release all resources.
	CRTFREEIF(mCurrentProfile);
	CRTFREEIF(mVersion);
	CRTFREEIF(mHavePREGInfo);

    FreeProfileMembers(mProfiles, mCount);
    FreeProfileMembers(m4xProfiles, m4xCount);
}

// Free up the member profile structs
void
nsProfileAccess::FreeProfileMembers(nsVoidArray *profiles, PRInt32 numElems)
{

	PRInt32 index = 0;

	ProfileStruct* aProfile;
	for (index = 0; index < numElems; index++)
	{
	        aProfile = (ProfileStruct *) profiles->ElementAt(index);
	        delete aProfile;
	}
    if (profiles)
	    delete profiles;
}

// Close the registry.
nsresult
nsProfileAccess::CloseRegistry()
{
    m_registry = 0;
    return NS_OK;
}

// Open the registry.
// If already opened, just use it.
nsresult
nsProfileAccess::OpenRegistry()
{
    nsresult rv;
    PRBool openalready = PR_FALSE;

	
    if (!m_registry) {
        rv = nsComponentManager::CreateInstance(kRegistryCID,
                                                nsnull,
                                                NS_GET_IID(nsIRegistry),
                                                getter_AddRefs(m_registry));
        if (NS_FAILED(rv)) return rv;
        if (!m_registry) return NS_ERROR_FAILURE;
    }

    // Open the registry
    rv = m_registry->IsOpen( &openalready);
    if (NS_FAILED(rv)) return rv;

    if (!openalready)
        rv = m_registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);   

    return rv;
}

// Given the name of the profile, the structure that
// contains the relavant profile information will be filled.
// Caller must free up the profile struct.
nsresult	
nsProfileAccess::GetValue(const char* profileName, ProfileStruct** aProfile)
{
	PRInt32 index = 0;
	
	index = FindProfileIndex(profileName);

	if (index >= 0)
	{
		*aProfile = (ProfileStruct *) PR_Malloc(sizeof(ProfileStruct));
		if (!*aProfile)
			return NS_ERROR_OUT_OF_MEMORY;

		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		(*aProfile)->profileName		= nsnull;
		(*aProfile)->profileLocation	= nsnull;
		(*aProfile)->isMigrated			= nsnull;
		(*aProfile)->NCProfileName		= nsnull;
		(*aProfile)->NCDeniedService	= nsnull;
		(*aProfile)->NCEmailAddress		= nsnull;
		(*aProfile)->NCHavePregInfo     = nsnull;

		(*aProfile)->profileName		= nsCRT::strdup(profileItem->profileName);
		(*aProfile)->profileLocation	= nsCRT::strdup(profileItem->profileLocation);
		(*aProfile)->isMigrated			= nsCRT::strdup(profileItem->isMigrated);

		if (profileItem->NCProfileName)
			(*aProfile)->NCProfileName	= nsCRT::strdup(profileItem->NCProfileName);
		if (profileItem->NCDeniedService)
			(*aProfile)->NCProfileName	= nsCRT::strdup(profileItem->NCDeniedService);
        if (profileItem->NCEmailAddress)
            (*aProfile)->NCProfileName	= nsCRT::strdup(profileItem->NCEmailAddress);
        if (profileItem->NCHavePregInfo)
            (*aProfile)->NCProfileName	= nsCRT::strdup(profileItem->NCHavePregInfo);
	}
	else
		*aProfile = nsnull;

	return NS_OK;
}

// This method writes all changes to the array of the 
// profile structs. If it is an existing profile, it
// will be updated. If it is a new profile, it gets added
// to the list. 
nsresult
nsProfileAccess::SetValue(ProfileStruct* aProfile)
{
	PRInt32	index = 0;

	index = FindProfileIndex(aProfile->profileName);
	
	if (index >= 0)
	{
                ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		PRInt32 length = PL_strlen(aProfile->profileLocation);
		profileItem->profileLocation = (char *) PR_Realloc(profileItem->profileLocation, length+1);
		PL_strcpy(profileItem->profileLocation, aProfile->profileLocation);
		
		length = PL_strlen(aProfile->isMigrated);
		profileItem->isMigrated = (char *) PR_Realloc(profileItem->isMigrated, length+1);
		PL_strcpy(profileItem->isMigrated, aProfile->isMigrated);
		
		profileItem->updateProfileEntry = PR_TRUE;

		if (aProfile->NCProfileName)
		{
			length = PL_strlen(aProfile->NCProfileName);
			profileItem->NCProfileName = (char *) PR_Realloc(profileItem->NCProfileName, length+1);
			PL_strcpy(profileItem->NCProfileName, aProfile->NCProfileName);
		}
		if (aProfile->NCDeniedService)
		{
			length = PL_strlen(aProfile->NCDeniedService);
			profileItem->NCDeniedService = (char *) PR_Realloc(profileItem->NCDeniedService, length+1);
			PL_strcpy(profileItem->NCDeniedService, aProfile->NCDeniedService);
		}
        if (aProfile->NCEmailAddress)
		{
            length = PL_strlen(aProfile->NCEmailAddress);
            profileItem->NCEmailAddress = (char *) PR_Realloc(profileItem->NCEmailAddress, length+1);
            PL_strcpy(profileItem->NCEmailAddress, aProfile->NCEmailAddress);
		}
        if (aProfile->NCHavePregInfo)
		{
            length = PL_strlen(aProfile->NCHavePregInfo);
            profileItem->NCHavePregInfo = (char *) PR_Realloc(profileItem->NCHavePregInfo, length+1);
            PL_strcpy(profileItem->NCHavePregInfo, aProfile->NCHavePregInfo);
		}
	}
	else
	{
		ProfileStruct*	profileItem	= new ProfileStruct();
		if (!profileItem)
			return NS_ERROR_OUT_OF_MEMORY;

		profileItem->profileName		= nsnull;
		profileItem->profileLocation	= nsnull;
		profileItem->isMigrated			= nsnull;
		profileItem->NCProfileName		= nsnull;
		profileItem->NCDeniedService	= nsnull;
        profileItem->NCEmailAddress		= nsnull;
        profileItem->NCHavePregInfo	    = nsnull;

		profileItem->profileName		= nsCRT::strdup(aProfile->profileName);
		profileItem->profileLocation	= nsCRT::strdup(aProfile->profileLocation);
		profileItem->isMigrated			= nsCRT::strdup(aProfile->isMigrated);
		profileItem->updateProfileEntry	= PR_TRUE;

		if (aProfile->NCProfileName)
			profileItem->NCProfileName	= nsCRT::strdup(aProfile->NCProfileName);
		if (aProfile->NCDeniedService)
			profileItem->NCProfileName	= nsCRT::strdup(aProfile->NCDeniedService);
        if (aProfile->NCEmailAddress)
            profileItem->NCEmailAddress	= nsCRT::strdup(aProfile->NCEmailAddress);
        if (aProfile->NCHavePregInfo)
            profileItem->NCHavePregInfo	= nsCRT::strdup(aProfile->NCHavePregInfo);

		if (!mProfiles)
			mProfiles = new nsVoidArray();
		mProfiles->AppendElement((void*)profileItem);
		mCount++;
	}
	return NS_OK;
}

// Enumerates through the registry for profile
// information. Reads in the data into the array 
// of profile structs. After this, all the callers
// requesting profile info will get thier data from
// profiles array. All the udates will be done to this
// data structure to reflect the latest status.
// Data will be flushed at the end.
nsresult 
nsProfileAccess::FillProfileInfo()
{
    nsresult rv = NS_OK;

    // Make the fail: thing work
    mProfiles = nsnull;

    rv = OpenRegistry();
    if (NS_FAILED(rv)) return rv;       

    // Enumerate all subkeys (immediately) under the given node.
    nsCOMPtr<nsIEnumerator> enumKeys;
    nsRegistryKey profilesTreeKey;
    
    rv = m_registry->GetSubtree(nsIRegistry::Common, REGISTRY_PROFILE_SUBTREE_STRING, &profilesTreeKey);
    if (NS_FAILED(rv)) 
	{
		rv = m_registry->AddSubtree(nsIRegistry::Common, REGISTRY_PROFILE_SUBTREE_STRING, &profilesTreeKey);
		if (NS_FAILED(rv)) return rv;
	}

	// Get the current profile
	rv = m_registry->GetStringUTF8(profilesTreeKey, REGISTRY_CURRENT_PROFILE_STRING, &mCurrentProfile);

	// Get the profile version
	rv = m_registry->GetStringUTF8(profilesTreeKey, REGISTRY_VERSION_STRING, &mVersion);

	if (mVersion == nsnull)
	{
		mFixRegEntries = PR_TRUE;
		mVersion = nsCRT::strdup(REGISTRY_VERSION_1_0);
		mProfileDataChanged = PR_TRUE;
	}

	// Get the preg info
	rv = m_registry->GetStringUTF8(profilesTreeKey, REGISTRY_HAVE_PREG_INFO_STRING, &mHavePREGInfo);
	
	if (mHavePREGInfo == nsnull)
	{
		mHavePREGInfo = nsCRT::strdup(REGISTRY_NO_STRING);
		mProfileDataChanged = PR_TRUE;
	}

	rv = m_registry->EnumerateSubtrees( profilesTreeKey, getter_AddRefs(enumKeys));
    if (NS_FAILED(rv)) return rv;
            
	rv = enumKeys->First();
    if (NS_FAILED(rv)) return rv;
        
	mCount = 0;
	mNumProfiles = 0;
	mNumOldProfiles = 0;

	while( (NS_OK != enumKeys->IsDone()) ) 
    {
		nsCOMPtr<nsISupports> base;

		rv = enumKeys->CurrentItem( getter_AddRefs(base) );
		if (NS_FAILED(rv)) return rv;
                            
        // Get specific interface.
        nsCOMPtr <nsIRegistryNode> node;
        nsIID nodeIID = NS_IREGISTRYNODE_IID;
                                    
        rv = base->QueryInterface( nodeIID, getter_AddRefs(node));
		if (NS_FAILED(rv)) return rv;
                                    
		// Get node name.
        nsXPIDLCString profile;
        nsXPIDLCString isMigrated;
		nsXPIDLCString NCProfileName;
		nsXPIDLCString NCDeniedService;
        nsXPIDLCString NCEmailAddress;
        nsXPIDLCString NCHavePregInfo;
        char* directory = nsnull;
                                            
        rv = node->GetName( getter_Copies(profile) );
		if (NS_FAILED(rv)) return rv;

		if (profile)
        {
			nsRegistryKey profKey;								

            rv = m_registry->GetSubtree(profilesTreeKey, profile, &profKey);
			if (NS_FAILED(rv)) return rv;
                                                    
            rv = m_registry->GetStringUTF8(profKey, REGISTRY_DIRECTORY_STRING, &directory);
			if (NS_FAILED(rv)) return rv;

			if (mFixRegEntries)
				FixRegEntry(&directory);

            rv = m_registry->GetStringUTF8(profKey, REGISTRY_MIGRATED_STRING, getter_Copies(isMigrated));
			if (NS_FAILED(rv)) return rv;

            rv = m_registry->GetStringUTF8(profKey, REGISTRY_NC_PROFILE_NAME_STRING, getter_Copies(NCProfileName));
            rv = m_registry->GetStringUTF8(profKey, REGISTRY_NC_SERVICE_DENIAL_STRING, getter_Copies(NCDeniedService));
            rv = m_registry->GetStringUTF8(profKey, REGISTRY_NC_USER_EMAIL_STRING, getter_Copies(NCEmailAddress));
            rv = m_registry->GetStringUTF8(profKey, REGISTRY_NC_HAVE_PREG_INFO_STRING, getter_Copies(NCHavePregInfo));

            ProfileStruct*	profileItem	= new ProfileStruct();
            if (!profileItem)
                return NS_ERROR_OUT_OF_MEMORY;

            profileItem->profileName			= nsnull;
            profileItem->profileLocation		= nsnull;
            profileItem->isMigrated			= nsnull;
            profileItem->NCProfileName		= nsnull;
            profileItem->NCDeniedService		= nsnull;
            profileItem->NCEmailAddress		= nsnull;
            profileItem->NCHavePregInfo		= nsnull;
            profileItem->updateProfileEntry	= PR_TRUE;
	
            profileItem->profileName			= nsCRT::strdup(profile);
            profileItem->profileLocation		= nsCRT::strdup(directory);
            profileItem->isMigrated			= nsCRT::strdup(isMigrated);

            if (NCProfileName)
                profileItem->NCProfileName	= nsCRT::strdup(NCProfileName);
            if (NCDeniedService)
                profileItem->NCDeniedService	= nsCRT::strdup(NCDeniedService);
            if (NCEmailAddress)
                profileItem->NCEmailAddress	= nsCRT::strdup(NCEmailAddress);
            if (NCHavePregInfo)
                profileItem->NCHavePregInfo	= nsCRT::strdup(NCHavePregInfo);

            if (PL_strcmp(isMigrated, REGISTRY_YES_STRING) == 0)
		mNumProfiles++;
 	    else if (PL_strcmp(isMigrated, REGISTRY_NO_STRING) == 0)
                mNumOldProfiles++;

            if (!mProfiles)
                mProfiles = new nsVoidArray();
            mProfiles->AppendElement((void*)profileItem);			
			
            mCount++;
            CRTFREEIF(directory);
	}
        rv = enumKeys->Next();
     }

	mFixRegEntries = PR_FALSE;
	rv = CloseRegistry();
	return rv;
}

// Return the number of 5x profiles.
// A member variable mNumProfiles is used
// to keep track of 5x profiles. 
void
nsProfileAccess::GetNumProfiles(PRInt32 *numProfiles)
{
	*numProfiles = mNumProfiles;
}

// Return the number of 4x (>=4.5 & < 5.0) profiles.
// A member variable mNumOldProfiles is used
// to keep track of 4x profiles. 
void
nsProfileAccess::GetNum4xProfiles(PRInt32 *numProfiles)
{
	*numProfiles = mNumOldProfiles;
}

// If the application can't find the current profile,
// the first profile will be used as the current profile.
// This routine returns the first 5x profile.
// Caller must free up the string (firstProfile).
void 
nsProfileAccess::GetFirstProfile(char **firstProfile)
{
	PRInt32 index = 0;

	*firstProfile = nsnull;

	for(index = 0; index < mCount; index++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		if (PL_strcmp(profileItem->isMigrated, REGISTRY_YES_STRING) == 0)
		{
			*firstProfile = nsCRT::strdup(profileItem->profileName);
			break;
		}
	}
}

// Set the current profile. Opearting directly on the tree.
// A separate struct should be maintained for the top level info.
// That way we can eliminate additional registry access. For
// now, we depend on registry operations.
// Capture the current profile information into mCurrentProfile.
void
nsProfileAccess::SetCurrentProfile(const char *profileName)
{
	CRTFREEIF(mCurrentProfile);
	mCurrentProfile = nsCRT::strdup(profileName);
	mProfileDataChanged = PR_TRUE;
}

// Return the current profile value.
// If mCurrent profile is already set, that value is returned.
// If there is only one profile that value is set to CurrentProfile.
void 
nsProfileAccess::GetCurrentProfile(char **profileName)
{

	*profileName = nsnull;

	if (mCurrentProfile)
	{
		if ((PL_strcmp(mCurrentProfile,"") != 0) || mForgetProfileCalled)
		{
			*profileName = nsCRT::strdup(mCurrentProfile);
		}
	}

	// If there are profiles and profileName is not
	// set yet. Get the first one and set it as Current Profile.
	if (mNumProfiles > 0 && (*profileName == nsnull))
	{
		GetFirstProfile(profileName);
		SetCurrentProfile(*profileName);
	}
}

// Delete a profile from profile structs
void
nsProfileAccess::RemoveSubTree(const char* profileName)
{
	// delete this entry from the mProfiles array
	// by moving the pointers with something like memmove
	// decrement mCount if it works.
	PRInt32	index = 0;
	PRBool  isOldProfile = PR_FALSE;

	index = FindProfileIndex(profileName);

	if (index >= 0)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		if (PL_strcmp(profileItem->isMigrated, REGISTRY_NO_STRING) == 0)
			isOldProfile = PR_TRUE;

		mProfiles->RemoveElementAt(index);

		mCount--;
		if (isOldProfile)
			mNumOldProfiles--;
		else
			mNumProfiles--;

		if (PL_strcmp(profileName, mCurrentProfile) == 0)
			PL_strcpy(mCurrentProfile,"");
	}
}

// Fix registry incompatabilities with previous builds
void 
nsProfileAccess::FixRegEntry(char** dirName) 
{
	nsSimpleCharString decodedDirName;
	PRBool haveHexBytes = PR_TRUE;

	// Decode the directory name to return the ordinary string
	nsInputStringStream stream(*dirName);
	nsPersistentFileDescriptor descriptor;
	
	char bigBuffer[MAX_PERSISTENT_DATA_SIZE + 1];
	// The first 8 bytes of the data should be a hex version of the data size to follow.
	PRInt32 bytesRead = NUM_HEX_BYTES;
	bytesRead = stream.read(bigBuffer, bytesRead);
	if (bytesRead != NUM_HEX_BYTES)
		haveHexBytes = PR_FALSE;

	if (haveHexBytes)
	{
		bigBuffer[NUM_HEX_BYTES] = '\0';

		for (int i = 0; i < NUM_HEX_BYTES; i++)
		{
			if (!(ISHEX(bigBuffer[i])))
			{
				haveHexBytes = PR_FALSE;
				break;
			}
		}
	}

	if (haveHexBytes)
	{
		//stream(dirName);
		PR_sscanf(bigBuffer, "%x", (PRUint32*)&bytesRead);
		if (bytesRead > MAX_PERSISTENT_DATA_SIZE)
		{
			// Try to tolerate encoded values with no length header
			bytesRead = NUM_HEX_BYTES + stream.read(bigBuffer + NUM_HEX_BYTES, MAX_PERSISTENT_DATA_SIZE - NUM_HEX_BYTES);
		}
		else
		{
			// Now we know how many bytes to read, do it.
			bytesRead = stream.read(bigBuffer, bytesRead);
		}
		// Make sure we are null terminated
		bigBuffer[bytesRead]='\0';
		descriptor.SetData(bigBuffer, bytesRead);				
		descriptor.GetData(decodedDirName);

		*dirName = nsCRT::strdup(decodedDirName);
	}
}

// Return the index of a given profiel from the arraf of profile structs.
PRInt32
nsProfileAccess::FindProfileIndex(const char* profileName)
{
	PRInt32 retval = -1;
	PRInt32 index = 0;

	for (index=0; index < mCount; index++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		if (PL_strcmp(profileName, profileItem->profileName) == 0)
		{
			retval = index;
			break;
		}
	}
	return retval;
}

// Flush profile information from the data structure to the registry.
nsresult 
nsProfileAccess::UpdateRegistry()
{
	nsresult rv;

	if (!mProfileDataChanged)
	{
		return NS_OK;
	}

    rv = OpenRegistry();
    if (NS_FAILED(rv)) return rv;

    // Enumerate all subkeys (immediately) under the given node.
    nsCOMPtr<nsIEnumerator> enumKeys;
    nsRegistryKey profilesTreeKey;

	// Get the major subtree
    rv = m_registry->GetSubtree(nsIRegistry::Common, REGISTRY_PROFILE_SUBTREE_STRING, &profilesTreeKey);
	if (NS_FAILED(rv)) return rv;

	// Set the current profile
	rv = m_registry->SetStringUTF8(profilesTreeKey, REGISTRY_CURRENT_PROFILE_STRING, mCurrentProfile);
	if (NS_FAILED(rv)) return rv;

	// Set the registry version
	rv = m_registry->SetStringUTF8(profilesTreeKey, REGISTRY_VERSION_STRING, mVersion);
	if (NS_FAILED(rv)) return rv;

	// Set preg info
	rv = m_registry->SetStringUTF8(profilesTreeKey, REGISTRY_HAVE_PREG_INFO_STRING, mHavePREGInfo);
	if (NS_FAILED(rv)) return rv;

	rv = m_registry->EnumerateSubtrees(profilesTreeKey, getter_AddRefs(enumKeys));
    if (NS_FAILED(rv)) return rv;
            
	rv = enumKeys->First();
    if (NS_FAILED(rv)) return rv;
        
	while( (NS_OK != enumKeys->IsDone()) ) 
    {
		nsCOMPtr<nsISupports> base;

		rv = enumKeys->CurrentItem( getter_AddRefs(base) );
		if (NS_FAILED(rv)) return rv;
                            
        // Get specific interface.
        nsCOMPtr <nsIRegistryNode> node;
        nsIID nodeIID = NS_IREGISTRYNODE_IID;
                                    
        rv = base->QueryInterface( nodeIID, getter_AddRefs(node));
		if (NS_FAILED(rv)) return rv;
                                    
		// Get node name.
        nsXPIDLCString profile;
        nsXPIDLCString isMigrated;
        nsXPIDLCString directory;
                                            
        rv = node->GetName( getter_Copies(profile) );
		if (NS_FAILED(rv)) return rv;

		PRInt32 index = 0;

		index = FindProfileIndex(profile);

		if (index < 0)
		{
			// This profile is deleted.
			rv = m_registry->RemoveSubtree(profilesTreeKey, profile);
			if (NS_FAILED(rv)) return rv;
		}
		else
        {
			nsRegistryKey profKey;								

			ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

			rv = m_registry->GetSubtree(profilesTreeKey, profile, &profKey);
			if (NS_FAILED(rv)) return rv;
                                            
			rv = m_registry->SetStringUTF8(profKey, REGISTRY_DIRECTORY_STRING, profileItem->profileLocation);
			if (NS_FAILED(rv)) return rv;

			rv = m_registry->SetStringUTF8(profKey, REGISTRY_MIGRATED_STRING, profileItem->isMigrated);
			if (NS_FAILED(rv)) return rv;

            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_PROFILE_NAME_STRING, profileItem->NCProfileName);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_SERVICE_DENIAL_STRING, profileItem->NCDeniedService);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_USER_EMAIL_STRING, profileItem->NCEmailAddress);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_HAVE_PREG_INFO_STRING, profileItem->NCHavePregInfo);

			profileItem->updateProfileEntry = PR_FALSE;
		}
	    rv = enumKeys->Next();
	}

	// Take care of new nodes
	for (int i = 0; i < mCount; i++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(i));

		if (profileItem->updateProfileEntry)
		{
			nsRegistryKey profKey;								

			rv = m_registry->AddSubtree(profilesTreeKey, profileItem->profileName, &profKey);
			if (NS_FAILED(rv)) return rv;

			rv = m_registry->SetStringUTF8(profKey, REGISTRY_DIRECTORY_STRING, profileItem->profileLocation);
			if (NS_FAILED(rv)) return rv;

			rv = m_registry->SetStringUTF8(profKey, REGISTRY_MIGRATED_STRING, profileItem->isMigrated);
			if (NS_FAILED(rv)) return rv;

            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_PROFILE_NAME_STRING, profileItem->NCProfileName);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_SERVICE_DENIAL_STRING, profileItem->NCDeniedService);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_USER_EMAIL_STRING, profileItem->NCEmailAddress);
            rv = m_registry->SetStringUTF8(profKey, REGISTRY_NC_HAVE_PREG_INFO_STRING, profileItem->NCHavePregInfo);

			profileItem->updateProfileEntry = PR_FALSE;
		}
	}

	rv = CloseRegistry();
	mProfileDataChanged = PR_FALSE;

	return rv;
}

// Return the list of profiles, 4x and 5x.
// For 4x profiles text "- migrate" is appended
// to inform the JavaScript about the migration status.
void
nsProfileAccess::GetProfileList(char **profileListStr)
{
	nsCAutoString profileList("");

	for (PRInt32 index=0; index < mCount; index++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));

		if (index != 0)
		{
			profileList += ",";
		}
		profileList += profileItem->profileName;

		if (PL_strcmp(profileItem->isMigrated, REGISTRY_NO_STRING) == 0)
			profileList += " - migrate";
	}

    *profileListStr = nsCRT::strdup((const char *)profileList);
}

// Return a boolean based on the profile existence.
PRBool
nsProfileAccess::ProfileExists(const char *profileName)
{
	PRBool exists = PR_FALSE;

	for (PRInt32 index=0; index < mCount; index++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));
		if (PL_strcmp(profileItem->profileName, profileName) == 0)
		{
			exists = PR_TRUE;
			break;
		}
    }

	return exists;
}

// Capture the 4x profile information from the old registry (4x)
nsresult
nsProfileAccess::Get4xProfileInfo(const char *registryName)
{

	nsresult rv = NS_OK;
	mNumOldProfiles = 0;

#if defined(XP_PC) || defined(XP_MAC)
    nsCOMPtr <nsIRegistry> oldReg;
    rv = nsComponentManager::CreateInstance(kRegistryCID,
                                            nsnull,
                                            NS_GET_IID(nsIRegistry),
                                            getter_AddRefs(oldReg));
    if (NS_FAILED(rv)) return rv;
    
    rv = oldReg->Open(registryName);
    if (NS_FAILED(rv)) return rv;
    
    // Enumerate 4x tree and create an array of that information.
    // Enumerate all subkeys (immediately) under the given node.
    nsCOMPtr<nsIEnumerator> enumKeys;
    
    rv = oldReg->EnumerateSubtrees(nsIRegistry::Users,
                                   getter_AddRefs(enumKeys));
    if (NS_FAILED(rv)) return rv;

    rv = enumKeys->First();
    if (NS_FAILED(rv)) return rv;
            
    // Enumerate subkeys till done.
    while( (NS_OK != enumKeys->IsDone())) 
	{
		nsCOMPtr<nsISupports> base;
		rv = enumKeys->CurrentItem(getter_AddRefs(base));
		if (NS_FAILED(rv)) return rv;

        // Get specific interface.
        nsCOMPtr <nsIRegistryNode> node;
        nsIID nodeIID = NS_IREGISTRYNODE_IID;
        rv = base->QueryInterface( nodeIID, getter_AddRefs(node));
		if (NS_FAILED(rv)) return rv;
        
		char *profile = nsnull;
        rv = node->GetName(&profile);
		if (NS_FAILED(rv)) return rv;

		PRBool exists = PR_FALSE;;
		exists = ProfileExists(profile);
		if (exists)
		{		
			rv = enumKeys->Next();
			if (NS_FAILED(rv)) return rv;
			
			continue;
		}
	
        nsRegistryKey key;								
        
        rv = oldReg->GetSubtree(nsIRegistry::Users, profile, &key);
		if (NS_FAILED(rv)) return rv;
        
        nsXPIDLCString profLoc;
        
        rv = oldReg->GetStringUTF8( key, "ProfileLocation", getter_Copies(profLoc));
		if (NS_FAILED(rv)) return rv;
        
#if defined(DEBUG_profile)
        printf("oldProflie Location = %s\n", profLoc);
#endif
                        
        ProfileStruct*	profileItem	= new ProfileStruct();
        if (!profileItem)
                return NS_ERROR_OUT_OF_MEMORY;

        profileItem->profileName			= nsnull;
        profileItem->profileLocation		= nsnull;
        profileItem->isMigrated			= nsnull;
        profileItem->NCProfileName			= nsnull;
        profileItem->NCDeniedService		= nsnull;
        profileItem->NCEmailAddress			= nsnull;
        profileItem->NCHavePregInfo		= nsnull;
        profileItem->updateProfileEntry	= PR_TRUE;

        profileItem->profileName			= nsCRT::strdup(nsUnescape(profile));
        profileItem->profileLocation		= nsCRT::strdup(profLoc);
        profileItem->isMigrated			= nsCRT::strdup(REGISTRY_NO_STRING);

        if (!m4xProfiles)
                m4xProfiles = new nsVoidArray();
        m4xProfiles->AppendElement((void*)profileItem);

        mNumOldProfiles++;

        rv = enumKeys->Next();
        if (NS_FAILED(rv)) return rv;
    }

#elif defined (XP_BEOS)
#else
    /* XP_UNIX */
    char *unixProfileName = PR_GetEnv(PROFILE_NAME_ENVIRONMENT_VARIABLE);
    char *unixProfileDirectory = PR_GetEnv(PROFILE_HOME_ENVIRONMENT_VARIABLE);
	
    if (!unixProfileName || !unixProfileDirectory || (PL_strlen(unixProfileName) == 0) || (PL_strlen(unixProfileDirectory) == 0)) {
	    unixProfileName = PR_GetEnv(USER_ENVIRONMENT_VARIABLE);
	    unixProfileDirectory = PR_GetEnv(HOME_ENVIRONMENT_VARIABLE);
    }

    PRBool exists = PR_FALSE;;
    exists = ProfileExists(unixProfileName);
    if (exists)
	{		
        return NS_OK;
	}

    if (unixProfileName && unixProfileDirectory) {
        nsCAutoString profileLocation(unixProfileDirectory);
        profileLocation += "/.netscape";
      
        nsCOMPtr<nsIFileSpec> users4xDotNetscapeDirectory;
        rv = NS_NewFileSpec(getter_AddRefs(users4xDotNetscapeDirectory));
        if (NS_FAILED(rv)) return rv;
    
        rv = users4xDotNetscapeDirectory->SetNativePath((const char *)profileLocation);
        if (NS_FAILED(rv)) return rv;

        rv = users4xDotNetscapeDirectory->Exists(&exists);
        if (NS_FAILED(rv)) return rv;

#ifdef DEBUG
	printf("%s exists:  %d\n",(const char *)profileLocation, exists);
#endif
        if (exists) {
            ProfileStruct*  profileItem     = new ProfileStruct();
            if (!profileItem)
                return NS_ERROR_OUT_OF_MEMORY;


            profileItem->profileName			= nsnull;
            profileItem->profileLocation		= nsnull;
            profileItem->isMigrated			= nsnull;
            profileItem->NCProfileName			= nsnull;
            profileItem->NCDeniedService		= nsnull;
            profileItem->NCEmailAddress			= nsnull;
            profileItem->NCHavePregInfo		= nsnull;
            profileItem->updateProfileEntry	= PR_TRUE;
            
            profileItem->profileName			= nsCRT::strdup(nsUnescape(unixProfileName));
            profileItem->profileLocation		= nsCRT::strdup((const char *)profileLocation);
            
            profileItem->isMigrated			= nsCRT::strdup(REGISTRY_NO_STRING);
            
            if (!m4xProfiles)
                m4xProfiles = new nsVoidArray();
            m4xProfiles->AppendElement((void*)profileItem);
            
            mNumOldProfiles++;
        }
        else {
#ifdef DEBUG
            printf("no 4.x profile\n");
#endif
        }
	}
#endif
	
	m4xCount = mNumOldProfiles;

    if (m4xCount > 0) {
        UpdateProfileArray();
	}

	return rv;
}

// Update the mozregistry with the 4x profile names
// and thier locations. Entry REGISTRY_MIGRATED_STRING is set to REGISTRY_NO_STRING
// to differentiate these profiles from 5x profiles.
nsresult
nsProfileAccess::UpdateProfileArray()
{
	nsresult rv = NS_OK;

	for (PRInt32 idx = 0; idx < m4xCount; idx++)
	{
		ProfileStruct* profileItem = (ProfileStruct *) (m4xProfiles->ElementAt(idx));		
		nsFileSpec profileDir(profileItem->profileLocation);

		PRBool exists;
		exists = ProfileExists(profileItem->profileName);
		if (NS_FAILED(rv)) return rv;
    
		// That profile already exists...
		// move on.....
		if (exists) {
			continue;
		}
		
        nsXPIDLCString profileDirString;	
        nsCOMPtr<nsIFileSpec>spec;
        rv = NS_NewFileSpecWithSpec(profileDir, getter_AddRefs(spec));
		if (NS_FAILED(rv)) return rv;

		rv = spec->GetPersistentDescriptorString(getter_Copies(profileDirString));
                                
        if (NS_SUCCEEDED(rv) && profileDirString)
        {

			PRInt32 length = PL_strlen(profileDirString);
			profileItem->profileLocation = (char *) PR_Realloc(profileItem->profileLocation, length+1);

			PL_strcpy(profileItem->profileLocation, profileDirString);

			SetValue(profileItem);
		}
	}

	mProfileDataChanged = PR_TRUE;

	return rv;
}

// Set the PREG flag to indicate if that info exists
void
nsProfileAccess::SetPREGInfo(const char* pregInfo)
{
	mHavePREGInfo = nsCRT::strdup(pregInfo);
}

//Get the for PREG info.
void 
nsProfileAccess::GetPREGInfo(const char *profileName, char **info)
{
	*info = nsnull;
    PRInt32 index = 0;

    index = FindProfileIndex(profileName);

	if (index >= 0 )
	{
		ProfileStruct* profileItem = (ProfileStruct *) (mProfiles->ElementAt(index));
		
		if (profileItem->NCHavePregInfo)
		    *info = nsCRT::strdup(profileItem->NCHavePregInfo);
		else
            *info = nsCRT::strdup(REGISTRY_NO_STRING);
	}
}

