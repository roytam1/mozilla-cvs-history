
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *   Don Bragg <dbragg@netscape.com>
 */

#include "InstallCleanup.h"

//----------------------------------------------------------------------------
// Native OS/2 file deletion function
//----------------------------------------------------------------------------
int NativeDeleteFile(const char* aFileToDelete)
{
    struct stat fileStack;
    if (stat(aFileToDelete, &fileStack)!= 0)
    {
      return DONE;// No file to delete, do nothing.
    }
    else 
    {
        if(remove(aFileToDelete)!= 0)
          return TRY_LATER;
    }
    return DONE;
}

//----------------------------------------------------------------------------
// Native OS/2 file replacement function
//----------------------------------------------------------------------------
int NativeReplaceFile(const char* replacementFile, const char* doomedFile )
{
    struct stat fileStack;

    // replacement file must exist, doomed file doesn't have to
    if (stat(replacementFile, &fileStack) != 0)
        return DONE;

    // don't have to do anything if the files are the same
    if (strcmp(replacementFile, doomedFile))
        return DONE;

    if (stat(doomedFile,&fileStack) != 0)
    {
      // doomedFile is gone move new file into place
      if (!rename(replacementFile, doomedFile))
          return TRY_LATER; // this shouldn't happen
    }
    else
    {
      //remove failed
      return TRY_LATER;
    }

    return DONE;
}

int main()
{
    HREG reg;
    int status = DONE;

    if ( REGERR_OK == NR_StartupRegistry())
    {
        if ( REGERR_OK == NR_RegOpen("", &reg) )
        {
            do {
                status = PerformScheduledTasks(reg);
                if (status != DONE)
                   DosSleep(1000);
            } while (status == TRY_LATER);
        }
        NR_ShutdownRegistry();
    }
    return(0);
}

