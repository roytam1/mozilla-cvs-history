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

#include "prmem.h"
#include "prmon.h"
#include "prlog.h"
#include "prprf.h"
#include "xp.h"
#include "softupdt.h"
#include "su_instl.h"
#include "nsInstallDelete.h"
#include "nsVersionRegistry.h"
#include "nsSUError.h"
#include "NSReg.h"

#include "nsPrivilegeManager.h"
#include "nsTarget.h"

extern int SU_DETAILS_DELETE_FILE;
extern int SU_DETAILS_DELETE_COMPONENT;
extern int SU_ERROR_NOT_IN_REGISTRY;
extern int SU_ERROR_FILE_READ_ONLY;
extern int SU_ERROR_FILE_IS_DIRECTORY;
extern int SU_ERROR_UNEXPECTED;


PR_BEGIN_EXTERN_C

/* PUBLIC METHODS */

/*	Constructor
 *    inFolder	- a folder object representing the directory that contains the file
 *    inRelativeFileName  - a relative path and file name
 */
nsInstallDelete::nsInstallDelete(nsSoftwareUpdate* inSoftUpdate, 
                                 nsFolderSpec* inFolder, 
                                 char* inRelativeFileName, 
                                 char* *errorMsg) : nsInstallObject(inSoftUpdate)
{
  registryName = NULL;
  finalFile = NULL;
  deleteStatus = DELETE_FILE;
  FILE_DOES_NOT_EXIST = nsSoftUpdateError_FILE_DOES_NOT_EXIST;
  FILE_READ_ONLY = nsSoftUpdateError_FILE_READ_ONLY;
  FILE_IS_DIRECTORY = nsSoftUpdateError_FILE_IS_DIRECTORY;

  finalFile =	inFolder->MakeFullPath(inRelativeFileName, errorMsg);
  if (*errorMsg != NULL) {
    return;
  }
  processInstallDelete(errorMsg);
}

/*	Constructor
 *    inRegistryName	- name of the component in the registry
 */
nsInstallDelete::nsInstallDelete(nsSoftwareUpdate* inSoftUpdate, 
                                 char* inRegistryName, 
                                 char* *errorMsg) : nsInstallObject(inSoftUpdate)
{
  finalFile = NULL;
  deleteStatus = DELETE_COMPONENT;
  FILE_DOES_NOT_EXIST = nsSoftUpdateError_FILE_DOES_NOT_EXIST;
  FILE_READ_ONLY = nsSoftUpdateError_FILE_READ_ONLY;
  FILE_IS_DIRECTORY = nsSoftUpdateError_FILE_IS_DIRECTORY;
  registryName = XP_STRDUP(inRegistryName);
  processInstallDelete(errorMsg);
}

nsInstallDelete::~nsInstallDelete()
{
  XP_FREEIF(finalFile);
  XP_FREEIF(registryName);
}

char* nsInstallDelete::Prepare()
{
  // no set-up necessary
  return NULL;
}

/* Complete
 * Completes the install by deleting the file
 * Security hazard: make sure we request the right permissions
 */
char* nsInstallDelete::Complete()
{
  char* errorMsg = NULL;
  int err = -1;
  nsTarget* execTarget = NULL;

  nsPrivilegeManager* privMgr = nsPrivilegeManager::getPrivilegeManager();
  nsTarget* impersonation = nsTarget::findTarget(IMPERSONATOR);

  if ((privMgr != NULL) && (impersonation != NULL)) {
    /* XXX: We should get the SystemPrincipal and enablePrivilege on that. 
     * Or may be we should get rid of impersonation
     */
    privMgr->enablePrivilege(impersonation, 1);
    execTarget = nsTarget::findTarget(INSTALL_PRIV);
    if (execTarget != NULL) {
      if (!privMgr->enablePrivilege( execTarget, softUpdate->GetPrincipal(), 1 )) {
        return SU_GetErrorMsg3("Permssion was denied", nsSoftUpdateError_ACCESS_DENIED);
      }
    }
  }

  if (deleteStatus == DELETE_COMPONENT) {
    err = nsVersionRegistry::deleteComponent(registryName);
  }
  char *msg = NULL;
  if ((deleteStatus == DELETE_FILE) || (err == REGERR_OK)) {
    if (finalFile != NULL) {
      err = NativeComplete();
      if ((err != 0) && (err != nsSoftUpdateError_FILE_DOES_NOT_EXIST))	{
        if (execTarget != NULL) {
          privMgr->revertPrivilege( execTarget, 1 );
        }
        msg = SU_GetString1(SU_ERROR_UNEXPECTED, finalFile);
      }
    }
  }	else {
    msg = SU_GetString1(SU_ERROR_UNEXPECTED, finalFile);
  }
  if (msg != NULL) {
    errorMsg = SU_GetErrorMsg3(msg, err);
    PR_FREEIF(msg);
  }
  return errorMsg;
}

void nsInstallDelete::Abort()
{
}

char* nsInstallDelete::toString()
{
  if (deleteStatus == DELETE_FILE) {
    return SU_GetString1(SU_DETAILS_DELETE_FILE, finalFile);
  }	else {
    return SU_GetString1(SU_DETAILS_DELETE_COMPONENT, registryName);
  }
}

/* PRIVATE METHODS */
void nsInstallDelete::processInstallDelete(char* *errorMsg)
{
  int err;

  nsTarget* target = NULL;

  /* Request impersonation privileges */
  nsPrivilegeManager* privMgr = nsPrivilegeManager::getPrivilegeManager();
  nsTarget* impersonation = nsTarget::findTarget(IMPERSONATOR);

  if ((privMgr != NULL) && (impersonation != NULL)) {
    /* XXX: We should get the SystemPrincipal and enablePrivilege on that. 
     * Or may be we should get rid of impersonation
     */
    privMgr->enablePrivilege(impersonation, 1);
    target = nsTarget::findTarget(INSTALL_PRIV);
    if (target != NULL) {
      /* XXX: we need a way to indicate that a dialog box should appear.*/
      if (!privMgr->enablePrivilege( target, softUpdate->GetPrincipal(), 1 )) {
        *errorMsg = SU_GetErrorMsg3("Permssion was denied", nsSoftUpdateError_ACCESS_DENIED);
        return;
      }
    }
  }
  
  if (deleteStatus == DELETE_COMPONENT) {
    /* Check if the component is in the registry */
    err = nsVersionRegistry::inRegistry(registryName);
    if (err != REGERR_OK) {
      char *msg = NULL;
      msg = SU_GetString1(SU_ERROR_NOT_IN_REGISTRY, registryName);
      *errorMsg = SU_GetErrorMsg3(msg, nsSoftUpdateError_NO_SUCH_COMPONENT);
      PR_FREEIF(msg);
      return;
    } else {
      finalFile = nsVersionRegistry::componentPath(registryName);
    }
  }
  
  /* Check if the file exists and is not read only */
  if (finalFile != NULL) {
    err = NativeCheckFileStatus();
    char *msg = NULL;
    if (err == 0) {
      /* System.out.println("File exists and is not read only" + finalFile);*/
    } else if (err == nsSoftUpdateError_FILE_DOES_NOT_EXIST)	{
      /*throw( new SoftUpdateException(Strings.error_FileDoesNotExist()  +  finalFile, err));*/
    } else if (err == nsSoftUpdateError_FILE_READ_ONLY) {
      msg = SU_GetString1(SU_ERROR_FILE_READ_ONLY, finalFile);
    } else if (err == nsSoftUpdateError_FILE_IS_DIRECTORY) {
      msg = SU_GetString1(SU_ERROR_FILE_IS_DIRECTORY, finalFile);
    } else {
      msg = SU_GetString1(SU_ERROR_UNEXPECTED, finalFile);
    }
    if (msg != NULL) {
      *errorMsg = SU_GetErrorMsg3(msg, err);
      PR_FREEIF(msg);
    }
  }
}

int nsInstallDelete::NativeComplete()
{
  char * fileName;
  int32 err;
  XP_StatStruct statinfo;
  
  fileName = XP_PlatformFileToURL(finalFile);
  if (fileName != NULL)
	{
      char * temp = XP_STRDUP(&fileName[7]);
      XP_FREEIF(fileName);
      fileName = temp;
      if (fileName)
		{
          err = XP_Stat(fileName, &statinfo, xpURL);
          if (err != -1)
			{
              if ( XP_STAT_READONLY( statinfo ) )
				{
                  err = FILE_READ_ONLY;
				}
              else if (!S_ISDIR(statinfo.st_mode))
				{
                  err = XP_FileRemove ( fileName, xpURL );
                  if (err != 0)
                    {
#ifdef XP_PC
                      /* REMIND  need to move function to generic XP file */
                      err = nsSoftwareUpdate_REBOOT_NEEDED;
                      su_DeleteOldFileLater( (char*)finalFile );
#endif
					}
                  
				}
              else
                {
                  err = FILE_IS_DIRECTORY;
                }
			}
          else
			{
              err = FILE_DOES_NOT_EXIST;
			}
          
        }
      else
		{
          err = -1;
		}
	}   
  
  XP_FREEIF(fileName);
  return err;
}

int nsInstallDelete::NativeCheckFileStatus()
{
  char * fileName;
  int32 err;
  XP_StatStruct statinfo;
  
  fileName = XP_PlatformFileToURL(finalFile);
  if (fileName != NULL)
	{
      char * temp = XP_STRDUP(&fileName[7]);
      XP_FREEIF(fileName);
      fileName = temp;
      
      if (fileName)
		{
          err = XP_Stat(fileName, &statinfo, xpURL);
          if (err != -1)
			{
              if ( XP_STAT_READONLY( statinfo ) )
				{
                  err = FILE_READ_ONLY;
				}
              else if (!S_ISDIR(statinfo.st_mode))
                {
                  ;
                }
              else
                {
                  err = FILE_IS_DIRECTORY;
                }
			}
          else
			{
              err = FILE_DOES_NOT_EXIST;
			}
        }
      else
		{
          err = -1;
		}
      
	}
  else
	{
      err = -1;
	}
  XP_FREEIF(fileName);
  return err;
}

PR_END_EXTERN_C
