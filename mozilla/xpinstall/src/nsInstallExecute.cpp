/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Daniel Veditz <dveditz@netscape.com>
 *     Douglas Turner <dougt@netscape.com>
 */



#include "prmem.h"

#include "nsFileSpec.h"

#include "VerReg.h"
#include "nsInstallExecute.h"
#include "nsInstallResources.h"
#include "ScheduledTasks.h"

#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"

nsInstallExecute:: nsInstallExecute(  nsInstall* inInstall,
                                      const nsString& inJarLocation,
                                      const nsString& inArgs,
                                      PRInt32 *error)

: nsInstallObject(inInstall)
{
    if ((inInstall == nsnull) || (inJarLocation.Equals("")) )
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    mJarLocation        = inJarLocation;
    mArgs               = inArgs;
    mExecutableFile     = nsnull;

}


nsInstallExecute::~nsInstallExecute()
{
    if (mExecutableFile)
        delete mExecutableFile;
}



PRInt32 nsInstallExecute::Prepare()
{
    if (mInstall == NULL || mJarLocation.Equals("")) 
        return nsInstall::INVALID_ARGUMENTS;

    return mInstall->ExtractFileFromJar(mJarLocation, nsnull, &mExecutableFile);
}

PRInt32 nsInstallExecute::Complete()
{
    if (mExecutableFile == nsnull)
        return nsInstall::INVALID_ARGUMENTS;

    nsFileSpec app( *mExecutableFile);
    
    if (!app.Exists())
	{
		return nsInstall::INVALID_ARGUMENTS;
	}

    PRInt32 result = app.Execute( mArgs );
    
    DeleteFileNowOrSchedule( app );
    
    return result;
}

void nsInstallExecute::Abort()
{
    /* Get the names */
    if (mExecutableFile == nsnull) 
        return;

    DeleteFileNowOrSchedule(*mExecutableFile);
}

char* nsInstallExecute::toString()
{
    char* buffer = new char[1024];
    char* rsrcVal = nsnull;

    if (buffer == nsnull || !mInstall)
        return nsnull;

    // if the FileSpec is NULL, just us the in jar file name.

    if (mExecutableFile == nsnull)
    {
        char *tempString = mJarLocation.ToNewCString();
        rsrcVal = mInstall->GetResourcedString("Execute");

        if (rsrcVal)
        {
            sprintf( buffer, rsrcVal, tempString);
            nsCRT::free(rsrcVal);
        }
        
        if (tempString)
            delete [] tempString;
    }
    else
    {
        rsrcVal = mInstall->GetResourcedString("Execute");

        if (rsrcVal)
        {
            sprintf( buffer, rsrcVal, mExecutableFile->GetCString());
            nsCRT::free(rsrcVal);
        }
    }
    return buffer;
}


PRBool
nsInstallExecute::CanUninstall()
{
    return PR_FALSE;
}

PRBool
nsInstallExecute::RegisterPackageNode()
{
    return PR_FALSE;
}

