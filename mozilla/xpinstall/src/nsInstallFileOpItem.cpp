/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "nspr.h"
#include "nsInstall.h"
#include "nsInstallFileOpEnums.h"
#include "nsInstallFileOpItem.h"
#include "ScheduledTasks.h"

#ifdef _WINDOWS
#include "nsWinShortcut.h"
#endif

#ifdef XP_MAC
#include "Aliases.h"
#include "Gestalt.h"
#include "Resources.h"
#include "script.h"
#include "nsILocalFileMac.h"
#endif

/* Public Methods */

MOZ_DECL_CTOR_COUNTER(nsInstallFileOpItem);

nsInstallFileOpItem::nsInstallFileOpItem(nsInstall*     aInstallObj,
                                         PRInt32        aCommand,
                                         nsIFile*       aTarget,
                                         PRInt32        aFlags,
                                         PRInt32*       aReturn)
:nsInstallObject(aInstallObj),
 mTarget(aTarget)
{
    MOZ_COUNT_CTOR(nsInstallFileOpItem);

    *aReturn      = nsInstall::SUCCESS;
    mIObj         = aInstallObj;
    mCommand      = aCommand;
    mFlags        = aFlags;
    mSrc          = nsnull;
    mParams       = nsnull;
    mStrTarget    = nsnull;
    mShortcutPath = nsnull;
    mDescription  = nsnull;
    mWorkingPath  = nsnull;
    mParams       = nsnull;
    mIcon         = nsnull;
}

nsInstallFileOpItem::nsInstallFileOpItem(nsInstall*     aInstallObj,
                                         PRInt32        aCommand,
                                         nsIFile*       aSrc,
                                         nsIFile*       aTarget,
                                         PRInt32*       aReturn)
:nsInstallObject(aInstallObj),
 mSrc(aSrc),
 mTarget(aTarget)
{
  MOZ_COUNT_CTOR(nsInstallFileOpItem);

  *aReturn    = nsInstall::SUCCESS;
  mIObj       = aInstallObj;
  mCommand    = aCommand;
  mFlags      = 0;
  mParams     = nsnull;
  mStrTarget  = nsnull;
  mAction     = ACTION_NONE;
  mShortcutPath = nsnull;
  mDescription  = nsnull;
  mWorkingPath  = nsnull;
  mParams       = nsnull;
  mIcon         = nsnull;
}

nsInstallFileOpItem::nsInstallFileOpItem(nsInstall*     aInstallObj,
                                         PRInt32        aCommand,
                                         nsIFile*       aTarget,
                                         PRInt32*       aReturn)
:nsInstallObject(aInstallObj),
 mTarget(aTarget)
{
  MOZ_COUNT_CTOR(nsInstallFileOpItem);

  *aReturn    = nsInstall::SUCCESS;
  mIObj       = aInstallObj;
	mCommand    = aCommand;
	mFlags      = 0;
	mSrc        = nsnull;
  mParams     = nsnull;
	mStrTarget  = nsnull;
  mAction     = ACTION_NONE;
  mShortcutPath = nsnull;
  mDescription  = nsnull;
  mWorkingPath  = nsnull;
  mParams       = nsnull;
  mIcon         = nsnull;
}

nsInstallFileOpItem::nsInstallFileOpItem(nsInstall*     aInstallObj,
                                         PRInt32        aCommand,
                                         nsIFile*       a1,
                                         nsString&      a2,
                                         PRInt32*       aReturn)
:nsInstallObject(aInstallObj)
{
    MOZ_COUNT_CTOR(nsInstallFileOpItem);

    *aReturn      = nsInstall::SUCCESS;
    mIObj         = aInstallObj;
    mCommand      = aCommand;
    mFlags        = 0;
    mAction       = ACTION_NONE;
    mShortcutPath = nsnull;
    mDescription  = nsnull;
    mWorkingPath  = nsnull;
    mParams       = nsnull;
    mIcon         = nsnull;

    switch(mCommand)
    {
        case NS_FOP_DIR_RENAME:
        case NS_FOP_FILE_RENAME:
            mSrc = a1;
            mTarget     = nsnull;
            mParams     = nsnull;
            mStrTarget  = new nsString(a2);

            if (mSrc == nsnull || mStrTarget == nsnull)
                *aReturn = nsInstall::OUT_OF_MEMORY;

            break;

        case NS_FOP_FILE_EXECUTE:
        default:
            mSrc        = nsnull;
            mTarget = a1;
            mParams     = new nsString(a2);
            mStrTarget  = nsnull;

    }
}

nsInstallFileOpItem::nsInstallFileOpItem(nsInstall*     aInstallObj,
                                         PRInt32        aCommand,
                                         nsIFile*       aTarget,
                                         nsIFile*       aShortcutPath,
                                         nsString&      aDescription,
                                         nsIFile*       aWorkingPath,
                                         nsString&      aParams,
                                         nsIFile*       aIcon,
                                         PRInt32        aIconId,
                                         PRInt32*       aReturn)
:nsInstallObject(aInstallObj),
 mTarget(aTarget),
 mShortcutPath(aShortcutPath),
 mWorkingPath(aWorkingPath),
 mIcon(aIcon)
{
    MOZ_COUNT_CTOR(nsInstallFileOpItem);

  *aReturn    = nsInstall::SUCCESS;
  mIObj       = aInstallObj;
  mCommand    = aCommand;
  mIconId     = aIconId;
  mFlags      = 0;
  mSrc        = nsnull;
  mStrTarget  = nsnull;
  mAction     = ACTION_NONE;

  mDescription = new nsString(aDescription);
  if(mDescription == nsnull)
    *aReturn = nsInstall::OUT_OF_MEMORY;

  mParams = new nsString(aParams);
  if(mParams == nsnull)
    *aReturn = nsInstall::OUT_OF_MEMORY;
}

nsInstallFileOpItem::~nsInstallFileOpItem()
{
  //if(mSrc)
  //  delete mSrc;
  //if(mTarget)
  //  delete mTarget;
  if(mStrTarget)
    delete mStrTarget;
  if(mParams)
    delete mParams;
  //if(mShortcutPath)
  //  delete mShortcutPath;
  if(mDescription)
    delete mDescription;
  //if(mWorkingPath)
  //  delete mWorkingPath;
  //if(mIcon)
  //  delete mIcon;

  MOZ_COUNT_DTOR(nsInstallFileOpItem);
}

#ifdef XP_MAC
#pragma mark -
#endif

PRInt32 nsInstallFileOpItem::Complete()
{
  PRInt32 ret = nsInstall::SUCCESS;

  switch(mCommand)
  {
    case NS_FOP_FILE_COPY:
      ret = NativeFileOpFileCopyComplete();
      break;
    case NS_FOP_FILE_DELETE:
      ret = NativeFileOpFileDeleteComplete(mTarget);
      break;
    case NS_FOP_FILE_EXECUTE:
      ret = NativeFileOpFileExecuteComplete();
      break;
    case NS_FOP_FILE_MOVE:
      ret = NativeFileOpFileMoveComplete();
      break;
    case NS_FOP_FILE_RENAME:
      ret = NativeFileOpFileRenameComplete();
      break;
    case NS_FOP_DIR_CREATE:
      // operation is done in the prepare phase
      break;
    case NS_FOP_DIR_REMOVE:
      ret = NativeFileOpDirRemoveComplete();
      break;
    case NS_FOP_DIR_RENAME:
      ret = NativeFileOpDirRenameComplete();
      break;
    case NS_FOP_WIN_SHORTCUT:
      ret = NativeFileOpWindowsShortcutComplete();
      break;
    case NS_FOP_MAC_ALIAS:
      ret = NativeFileOpMacAliasComplete();
      break;
    case NS_FOP_UNIX_LINK:
      ret = NativeFileOpUnixLink();
      break;
  }

  if ( (ret != nsInstall::SUCCESS) && (ret < nsInstall::GESTALT_INVALID_ARGUMENT || ret > nsInstall::REBOOT_NEEDED) )
    ret = nsInstall::UNEXPECTED_ERROR; /* translate to XPInstall error */
  	
  return ret;
}
  
char* nsInstallFileOpItem::toString()
{
  nsString result;
  char*    resultCString;
  char*    temp;

  // XXX these hardcoded strings should be replaced by nsInstall::GetResourcedString(id)

    // STRING USE WARNING: perhaps |result| should be an |nsCAutoString| to avoid all this double converting
  
  switch(mCommand)
  {
    case NS_FOP_FILE_COPY:
      result.AssignWithConversion("Copy File: ");
      mSrc->GetPath(&temp);
      result.AppendWithConversion(temp);
      result.AppendWithConversion(" to ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_FILE_DELETE:
     result.AssignWithConversion("Delete File: ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_FILE_EXECUTE:
      result.AssignWithConversion("Execute File: ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      result.AppendWithConversion(" ");
      result.Append(*mParams);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_FILE_MOVE:
      result.AssignWithConversion("Move File: ");
      mSrc->GetPath(&temp);
      result.AppendWithConversion(temp);
      result.AppendWithConversion(" to ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_FILE_RENAME:
      result.AssignWithConversion("Rename File: ");
      result.Append(*mStrTarget);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_DIR_CREATE:
      result.AssignWithConversion("Create Folder: ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_DIR_REMOVE:
      result.AssignWithConversion("Remove Folder: ");
      mTarget->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_DIR_RENAME:
      result.AssignWithConversion("Rename Dir: ");
      result.Append(*mStrTarget);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_WIN_SHORTCUT:
      result.AssignWithConversion("Windows Shortcut: ");
      mShortcutPath->GetPath(&temp);
      result.AppendWithConversion(temp);
      result.AppendWithConversion("\\");
      result.Append(*mDescription);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_MAC_ALIAS:
      result.AssignWithConversion("Mac Alias: ");
      mSrc->GetPath(&temp);
      result.AppendWithConversion(temp);
      resultCString = result.ToNewCString();
      break;
    case NS_FOP_UNIX_LINK:
      break;
    default:
      result.AssignWithConversion("Unkown file operation command!");
      resultCString = result.ToNewCString();
      break;
  }
  return resultCString;
}

PRInt32 nsInstallFileOpItem::Prepare()
{
  PRInt32 ret = nsInstall::SUCCESS;

  switch(mCommand)
  {
    case NS_FOP_FILE_COPY:
      ret = NativeFileOpFileCopyPrepare();
      break;
    case NS_FOP_FILE_DELETE:
      ret = NativeFileOpFileDeletePrepare();
      break;
    case NS_FOP_FILE_EXECUTE:
      ret = NativeFileOpFileExecutePrepare();
      break;
    case NS_FOP_FILE_MOVE:
      ret = NativeFileOpFileMovePrepare();
      break;
    case NS_FOP_FILE_RENAME:
      ret = NativeFileOpFileRenamePrepare();
      break;
    case NS_FOP_DIR_CREATE:
      ret = NativeFileOpDirCreatePrepare();
      break;
    case NS_FOP_DIR_REMOVE:
      ret = NativeFileOpDirRemovePrepare();
      break;
    case NS_FOP_DIR_RENAME:
      ret = NativeFileOpDirRenamePrepare();
      break;
    case NS_FOP_WIN_SHORTCUT:
      break;
    case NS_FOP_MAC_ALIAS:
      break;
    case NS_FOP_UNIX_LINK:
      break;
    default:
      break;
  }

  if ( (ret != nsInstall::SUCCESS) && (ret < nsInstall::GESTALT_INVALID_ARGUMENT || ret > nsInstall::REBOOT_NEEDED) )
    ret = nsInstall::UNEXPECTED_ERROR; /* translate to XPInstall error */

  return(ret);
}

void nsInstallFileOpItem::Abort()
{
  switch(mCommand)
  {
    case NS_FOP_FILE_COPY:
      NativeFileOpFileCopyAbort();
      break;
    case NS_FOP_FILE_DELETE:
      // does nothing
      break;
    case NS_FOP_FILE_EXECUTE:
      // does nothing
      break;
    case NS_FOP_FILE_MOVE:
      NativeFileOpFileMoveAbort();
      break;
    case NS_FOP_FILE_RENAME:
      NativeFileOpFileRenameAbort();
      break;
    case NS_FOP_DIR_CREATE:
      NativeFileOpDirCreateAbort();
      break;
    case NS_FOP_DIR_REMOVE:
      break;
    case NS_FOP_DIR_RENAME:
      NativeFileOpDirRenameAbort();
      break;
    case NS_FOP_WIN_SHORTCUT:
      NativeFileOpWindowsShortcutAbort();
      break;
    case NS_FOP_MAC_ALIAS:
      NativeFileOpMacAliasAbort();
      break;
    case NS_FOP_UNIX_LINK:
      break;
  }
}

/* Private Methods */

/* CanUninstall
* InstallFileOpItem() does not install any files which can be uninstalled,
* hence this function returns false. 
*/
PRBool
nsInstallFileOpItem::CanUninstall()
{
    return PR_FALSE;
}

/* RegisterPackageNode
* InstallFileOpItem() does notinstall files which need to be registered,
* hence this function returns false.
*/
PRBool
nsInstallFileOpItem::RegisterPackageNode()
{
    return PR_FALSE;
}

#ifdef XP_MAC
#pragma mark -
#endif

//
// File operation functions begin here
//
PRInt32
nsInstallFileOpItem::NativeFileOpDirCreatePrepare()
{
  PRInt32 ret = nsInstall::ALREADY_EXISTS;
  PRBool  flagExists;

  mAction = nsInstallFileOpItem::ACTION_FAILED;

  mTarget->Exists(&flagExists);
  if (!flagExists)
  {
      mTarget->Create(1, 0644);
      mAction = nsInstallFileOpItem::ACTION_SUCCESS;
      ret     = nsInstall::SUCCESS;
  }

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirCreateAbort()
{
  if(nsInstallFileOpItem::ACTION_SUCCESS == mAction)
    mTarget->Delete(PR_FALSE);

  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirRemovePrepare()
{
  PRBool flagExists, flagIsFile;

  mTarget->Exists(&flagExists);

  if(flagExists)
  {
    mTarget->IsFile(&flagIsFile);
    if(!flagIsFile)
      return nsInstall::SUCCESS;
    else
      return nsInstall::IS_FILE;
  }
    
  return nsInstall::DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirRemoveComplete()
{
  mTarget->Delete(mFlags);
  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileRenamePrepare()
{
  PRBool flagExists, flagIsFile;

  // XXX needs to check file attributes to make sure
  // user has proper permissions to delete file.
  // Waiting on dougt's fix to nsFileSpec().
  // In the meantime, check as much as possible.
  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mSrc->IsFile(&flagIsFile);
    if(flagIsFile)
    {
      nsIFile* target;

      mSrc->GetParent(&target);
      nsAutoCString tempTargetString(*mStrTarget);
      target->Append(tempTargetString);

      target->Exists(&flagExists);
      if(flagExists)
        return nsInstall::ALREADY_EXISTS;
      else
        return nsInstall::SUCCESS;
    }
    else
      return nsInstall::SOURCE_IS_DIRECTORY;
  }
    
  return nsInstall::SOURCE_DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileRenameComplete()
{
  PRInt32 ret = nsInstall::SUCCESS;
  PRBool flagExists, flagIsFile;
  
  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mSrc->IsFile(&flagIsFile);
    if(flagIsFile)
    {
        nsCOMPtr<nsIFile> parent;
        nsCOMPtr<nsIFile> target;

        mSrc->GetParent(getter_AddRefs(parent)); //need parent seprated for use in MoveTo method
        if(parent)
        {
            mSrc->GetParent(getter_AddRefs(target)); //need target for path assembly to check if the file already exists

            if (target)
            {
                nsAutoCString tempTargetString(*mStrTarget);
                target->Append(tempTargetString);
            }
            else
                return nsInstall::UNEXPECTED_ERROR;

            target->Exists(&flagExists);
            if(!flagExists)
            {
                nsAutoCString tempTargetString(*mStrTarget);
                mSrc->MoveTo(parent, tempTargetString);
            }
            else
                return nsInstall::ALREADY_EXISTS;
        }
        else
            return nsInstall::UNEXPECTED_ERROR;
    }
    else
      ret = nsInstall::SOURCE_IS_DIRECTORY;
  }
  else  
    ret = nsInstall::SOURCE_DOES_NOT_EXIST;

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileRenameAbort()
{
  PRInt32   ret  = nsInstall::SUCCESS;
  PRBool    flagExists;
  char*     leafName;
  nsCOMPtr<nsIFile>  newFilename;
  nsCOMPtr<nsIFile>  parent;

  mSrc->Exists(&flagExists);
  if(!flagExists)
  {
    mSrc->GetParent(getter_AddRefs(newFilename));
    if(newFilename)
    {
      mSrc->GetParent(getter_AddRefs(parent));
      if(parent)
      {
        nsAutoCString tempTargetString(*mStrTarget);
        newFilename->Append(tempTargetString);
    
        mSrc->GetLeafName(&leafName);

        newFilename->MoveTo(parent, leafName);
    
        if(leafName)
            nsCRT::free(leafName);
      }
      else
        return nsInstall::UNEXPECTED_ERROR;
    }
    else
      return nsInstall::UNEXPECTED_ERROR;
  }

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileCopyPrepare()
{
  PRBool flagExists, flagIsFile;
  char* leafName;
  nsCOMPtr<nsIFile> tempVar;

  // XXX needs to check file attributes to make sure
  // user has proper permissions to delete file.
  // Waiting on dougt's fix to nsFileSpec().
  // In the meantime, check as much as possible.
  
  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mSrc->IsFile(&flagIsFile);
    if(flagIsFile)
    {
      mTarget->Exists(&flagExists);
      if(!flagExists)
        return nsInstall::DOES_NOT_EXIST;
      else
      {
        mTarget->IsFile(&flagIsFile);
        if(flagIsFile)
          return nsInstall::IS_FILE;
        else
        {
          tempVar = mTarget;
          mSrc->GetLeafName(&leafName);
          tempVar->Append(leafName);
        }

        tempVar->Exists(&flagExists);
        if(flagExists)
          return nsInstall::ALREADY_EXISTS;
      }

      return nsInstall::SUCCESS;
    }
    else
      return nsInstall::SOURCE_IS_DIRECTORY;
  }
    
  return nsInstall::SOURCE_DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileCopyComplete()
{
  PRInt32 ret;
  char* leafName;
  nsCOMPtr<nsIFile> parent;

  mAction = nsInstallFileOpItem::ACTION_FAILED;

  mSrc->GetLeafName(&leafName);
  mTarget->GetParent(getter_AddRefs(parent));
  ret = mSrc->CopyTo(parent, leafName);
  if(nsInstall::SUCCESS == ret)
    mAction = nsInstallFileOpItem::ACTION_SUCCESS;

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileCopyAbort()
{
  nsCOMPtr<nsIFile> fullTarget = mTarget;
  PRInt32 ret = nsInstall::SUCCESS;

  if(nsInstallFileOpItem::ACTION_SUCCESS == mAction)
  {
    char* leafName;
    mSrc->GetLeafName(&leafName);
    fullTarget->Append(leafName);
    fullTarget->Delete(PR_FALSE);
  }

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileDeletePrepare()
{
  PRBool flagExists, flagIsFile;

  // XXX needs to check file attributes to make sure
  // user has proper permissions to delete file.
  // Waiting on dougt's fix to nsFileSpec().
  // In the meantime, check as much as possible.
  
  mTarget->Exists(&flagExists);
  if(flagExists)
  {
    mTarget->IsFile(&flagIsFile);
    if(flagIsFile)
      return nsInstall::SUCCESS;
    else
      return nsInstall::IS_DIRECTORY;
  }
    
  return nsInstall::DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileDeleteComplete(nsIFile *aTarget)
{
  PRBool flagExists, flagIsFile;

  aTarget->Exists(&flagExists);
  if(flagExists)
  {
    aTarget->IsFile(&flagIsFile);
    if(flagIsFile)
      return DeleteFileNowOrSchedule(aTarget);
    else
      return nsInstall::IS_DIRECTORY;
  }
    
  return nsInstall::DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileExecutePrepare()
{
  PRBool flagExists, flagIsFile;
  // XXX needs to check file attributes to make sure
  // user has proper permissions to delete file.
  // Waiting on dougt's fix to nsFileSpec().
  // In the meantime, check as much as possible.
  // Also, an absolute path (with filename) must be
  // used.  Xpinstall does not assume files are on the path.
  mTarget->Exists(&flagExists);
  if(flagExists)
  {
    mTarget->IsFile(&flagIsFile);
    if(flagIsFile)
      return nsInstall::SUCCESS;
    else
      return nsInstall::IS_DIRECTORY;
  }
    
  return nsInstall::DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileExecuteComplete()
{
  //mTarget->Execute(*mParams);
  //mTarget->Spawn(nsAutoCString(*mParams), 0);

  // We don't care if it succeeded or not since we
  // don't wait for the process to end anyways.
  // If the file doesn't exist, it was already detected
  // during the prepare phase.
  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileMovePrepare()
{
  PRBool flagExists, flagIsFile;

  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mTarget->Exists(&flagExists);
    if(!flagExists)
      return nsInstall::DOES_NOT_EXIST;
    else
    {
      mTarget->IsFile(&flagIsFile);
      if(flagIsFile)
        return nsInstall::IS_FILE;
      else
      {
        nsCOMPtr<nsIFile> tempVar;
        char* leaf;

        mTarget->Clone(getter_AddRefs(tempVar));
        mSrc->GetLeafName(&leaf);
        tempVar->Append(leaf);

        tempVar->Exists(&flagExists);
        if(flagExists)
          return nsInstall::ALREADY_EXISTS;
        else
          return NativeFileOpFileCopyPrepare();
      }
    }
  }

  return nsInstall::SOURCE_DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileMoveComplete()
{
  PRBool flagExists;
  PRInt32 ret = nsInstall::SUCCESS;

  mAction = nsInstallFileOpItem::ACTION_FAILED;
  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mTarget->Exists(&flagExists);
    if(!flagExists)
      ret = nsInstall::DOES_NOT_EXIST;
    else
    {
      PRInt32 ret2 = nsInstall::SUCCESS;

      ret = NativeFileOpFileCopyComplete();
      if(nsInstall::SUCCESS == ret)
      {
        mAction = nsInstallFileOpItem::ACTION_SUCCESS;
        ret2    = NativeFileOpFileDeleteComplete(mSrc);

        // We don't care if the value of ret2 is other than
        // REBOOT_NEEDED.  ret takes precedence otherwise.
        if(nsInstall::REBOOT_NEEDED == ret2)
          ret = ret2;
      }
    }
  }
  else
    ret = nsInstall::SOURCE_DOES_NOT_EXIST;

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpFileMoveAbort()
{
  PRBool flagExists;
  PRInt32 ret = nsInstall::SUCCESS;

  if(nsInstallFileOpItem::ACTION_SUCCESS == mAction)
  {
    mSrc->Exists(&flagExists);
    if(flagExists)
      ret = NativeFileOpFileDeleteComplete(mTarget);
    else
    {
      mTarget->Exists(&flagExists);
      if(flagExists)
      {
        nsCOMPtr<nsIFile> tempVar;
        PRInt32    ret2 = nsInstall::SUCCESS;

        // switch the values of mSrc and mTarget
        // so the original state can be restored.
        // NativeFileOpFileCopyComplete() copies from
        // mSrc to mTarget by default.
        mTarget->Clone(getter_AddRefs(tempVar));
        mSrc->Clone(getter_AddRefs(mTarget));
        tempVar->Clone(getter_AddRefs(mSrc));

        ret = NativeFileOpFileCopyComplete();
        if(nsInstall::SUCCESS == ret)
        {
          ret2 = NativeFileOpFileDeleteComplete(mSrc);

          // We don't care if the value of ret2 is other than
          // REBOOT_NEEDED.  ret takes precedence otherwise.
          if(nsInstall::REBOOT_NEEDED == ret2)
            ret = ret2;
        }
      }
      else
        ret = nsInstall::DOES_NOT_EXIST;
    }
  }

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirRenamePrepare()
{
  PRBool flagExists, flagIsFile;
  // XXX needs to check file attributes to make sure
  // user has proper permissions to delete file.
  // Waiting on dougt's fix to nsFileSpec().
  // In the meantime, check as much as possible.
  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mSrc->IsFile(&flagIsFile);
    if(!flagIsFile)
    {
      nsCOMPtr<nsIFile> target;

      mSrc->GetParent(getter_AddRefs(target));
      target->Append(nsAutoCString(*mStrTarget));

      target->Exists(&flagExists);
      if(flagExists)
        return nsInstall::ALREADY_EXISTS;
      else
        return nsInstall::SUCCESS;
    }
    else
      return nsInstall::IS_FILE;
  }
    
  return nsInstall::SOURCE_DOES_NOT_EXIST;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirRenameComplete()
{
  PRBool flagExists, flagIsFile;
  PRInt32 ret = nsInstall::SUCCESS;

  mSrc->Exists(&flagExists);
  if(flagExists)
  {
    mSrc->IsFile(&flagIsFile);
    if(!flagIsFile)
    {
      nsCOMPtr<nsIFile> target;

      mSrc->GetParent(getter_AddRefs(target));
      target->Append(nsAutoCString(*mStrTarget));

      target->Exists(&flagExists);
      if(!flagExists)
      {
        nsAutoCString cStrTarget(*mStrTarget);

        nsCOMPtr<nsIFile> parent;
        mSrc->GetParent(getter_AddRefs(parent));
        ret = mSrc->MoveTo(parent, cStrTarget);
      }
      else
        return nsInstall::ALREADY_EXISTS;
    }
    else
      ret = nsInstall::SOURCE_IS_FILE;
  }
  else  
    ret = nsInstall::SOURCE_DOES_NOT_EXIST;

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpDirRenameAbort()
{
  PRBool     flagExists;
  PRInt32    ret = nsInstall::SUCCESS;
  char*      leafName;
  nsCOMPtr<nsIFile> newDirName;
  nsCOMPtr<nsIFile> parent;

  mSrc->Exists(&flagExists);
  if(!flagExists)
  {
    mSrc->GetParent(getter_AddRefs(newDirName));
    newDirName->Append(nsAutoCString(*mStrTarget));
    mSrc->GetLeafName(&leafName);
    mSrc->GetParent(getter_AddRefs(parent));
    ret = newDirName->MoveTo(parent, leafName);
    
    if(leafName)
      nsCRT::free(leafName);
  }

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpWindowsShortcutComplete()
{
  PRInt32 ret = nsInstall::SUCCESS;

#ifdef _WINDOWS
  char *cDescription             = nsnull;
  char *cParams                  = nsnull;
  char *targetNativePathStr      = nsnull;
  char *shortcutNativePathStr    = nsnull;
  char *workingpathNativePathStr = nsnull;
  char *iconNativePathStr        = nsnull;

  if(mDescription)
    cDescription = mDescription->ToNewCString();
  if(mParams)
    cParams = mParams->ToNewCString();

  if((cDescription == nsnull) || (cParams == nsnull))
    ret = nsInstall::OUT_OF_MEMORY;
  else
  {
    if(mTarget)
      mTarget->GetPath(&targetNativePathStr);
    if(mShortcutPath)
      mShortcutPath->GetPath(&shortcutNativePathStr);
    if(mWorkingPath)
      mWorkingPath->GetPath(&workingpathNativePathStr);
    if(mIcon)
      mIcon->GetPath(&iconNativePathStr);

    ret = CreateALink(targetNativePathStr,
                      shortcutNativePathStr,
                      cDescription,
                      workingpathNativePathStr,
                      cParams,
                      iconNativePathStr,
                      mIconId);

    if(nsInstall::SUCCESS == ret)
      mAction = nsInstallFileOpItem::ACTION_SUCCESS;
  }

  if(cDescription)
    delete(cDescription);
  if(cParams)
    delete(cParams);
#endif

  return ret;
}

PRInt32
nsInstallFileOpItem::NativeFileOpWindowsShortcutAbort()
{
#ifdef _WINDOWS
  nsString   shortcutDescription;
  nsCOMPtr<nsIFile> shortcutTarget;

  shortcutDescription = *mDescription;
  shortcutDescription.AppendWithConversion(".lnk");
  mShortcutPath->Clone(getter_AddRefs(shortcutTarget));
  shortcutTarget->Append(nsAutoCString(shortcutDescription));

  NativeFileOpFileDeleteComplete(shortcutTarget);
#endif

  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpMacAliasComplete()
{

#ifdef XP_MAC
  // XXX gestalt to see if alias manager is around
  
  nsCOMPtr<nsILocalFileMac> localFileMacTarget = do_QueryInterface(mTarget);
  nsCOMPtr<nsILocalFileMac> localFileMacSrc = do_QueryInterface(mSrc);
  
  FSSpec        *fsPtrAlias, *srcPtrAlias;
  AliasHandle   aliasH;
  FInfo         info;
  OSErr         err = noErr;
  
  localFileMacTarget->GetFSSpec(fsPtrAlias);
  localFileMacSrc->GetFSSpec(srcPtrAlias);

  err = NewAliasMinimal( srcPtrAlias, &aliasH );
  if (err != noErr)  // bubble up Alias Manager error
  	return err;
  	
  // create the alias file
  FSpGetFInfo(srcPtrAlias, &info);
  FSpCreateResFile(fsPtrAlias, info.fdCreator, info.fdType, smRoman);
  short refNum = FSpOpenResFile(fsPtrAlias, fsRdWrPerm);
  if (refNum != -1)
  {
    UseResFile(refNum);
    AddResource((Handle)aliasH, rAliasType, 0, fsPtrAlias->name);
    ReleaseResource((Handle)aliasH);
    UpdateResFile(refNum);
    CloseResFile(refNum);
  }
  else
    return nsInstall::SUCCESS;
  
  // mark newly created file as an alias file
  FSpGetFInfo(fsPtrAlias, &info);
  info.fdFlags |= kIsAlias;
  FSpSetFInfo(fsPtrAlias, &info);
#endif

  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpMacAliasAbort()
{  
#ifdef XP_MAC
  NativeFileOpFileDeleteComplete(mTarget);
#endif 

  return nsInstall::SUCCESS;
}

PRInt32
nsInstallFileOpItem::NativeFileOpUnixLink()
{
  return nsInstall::SUCCESS;
}

