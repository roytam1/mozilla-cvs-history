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

#ifndef nsInstallFileOpItem_h__
#define nsInstallFileOpItem_h__

#include "prtypes.h"

#include "nsFileSpec.h"
#include "nsSoftwareUpdate.h"
#include "nsInstallObject.h"
#include "nsInstall.h"

class nsInstallFileOpItem : public nsInstallObject
{
  public:
    /* Public Fields */
    enum 
    {
      ACTION_NONE                 = -401,
      ACTION_SUCCESS              = -402,
      ACTION_FAILED               = -403
    };


    /* Public Methods */
    // used by:
    //   FileOpFileDelete()
    nsInstallFileOpItem(nsInstall*    installObj,
                        PRInt32       aCommand,
                        nsFileSpec&   aTarget,
                        PRInt32       aFlags,
                        PRInt32*      aReturn);

    // used by:
    //   FileOpDirRemove()
    //   FileOpFileCopy()
    //   FileOpFileMove()
    //   FileMacAlias()
    nsInstallFileOpItem(nsInstall*    installObj,
                        PRInt32       aCommand,
                        nsFileSpec&   aSrc,
                        nsFileSpec&   aTarget,
                        PRInt32*      aReturn);

    // used by:
    //   FileOpDirCreate()
    nsInstallFileOpItem(nsInstall*    aInstallObj,
                        PRInt32       aCommand,
                        nsFileSpec&   aTarget,
                        PRInt32*      aReturn);

    // used by:
    //   FileOpDirRename()
    //   FileOpFileExecute()
    //   FileOpFileRename()
    nsInstallFileOpItem(nsInstall*    aInstallObj,
                        PRInt32       aCommand,
                        nsFileSpec&   a1,
                        nsString&     a2,
                        PRInt32*      aReturn);

    // used by:
    //   WindowsShortcut()
    nsInstallFileOpItem(nsInstall*     aInstallObj,
                        PRInt32        aCommand,
                        nsFileSpec&    aTarget,
                        nsFileSpec&    aShortcutPath,
                        nsString&      aDescription,
                        nsFileSpec&    aWorkingPath,
                        nsString&      aParams,
                        nsFileSpec&    aIcon,
                        PRInt32        aIconId,
                        PRInt32*       aReturn);

    virtual ~nsInstallFileOpItem();

    PRInt32       Prepare(void);
    PRInt32       Complete();
    char*         toString();
    void          Abort();
    
  /* should these be protected? */
    PRBool        CanUninstall();
    PRBool        RegisterPackageNode();
      
  private:
    
    /* Private Fields */
    
    nsInstall*    mIObj;        // initiating Install object
    nsFileSpec*   mSrc;
    nsFileSpec*   mTarget;
    nsFileSpec*   mShortcutPath;
    nsFileSpec*   mWorkingPath;
    nsFileSpec*   mIcon;
    nsString*     mDescription;
    nsString*     mStrTarget;
    nsString*     mParams;
    long          mFStat;
    PRInt32       mFlags;
    PRInt32       mIconId;
    PRInt32       mCommand;
    PRInt32       mAction;
    
    /* Private Methods */

    PRInt32       NativeFileOpDirCreatePrepare();
    PRInt32       NativeFileOpDirCreateAbort();
    PRInt32       NativeFileOpDirRemovePrepare();
    PRInt32       NativeFileOpDirRemoveComplete();
    PRInt32       NativeFileOpDirRenamePrepare();
    PRInt32       NativeFileOpDirRenameComplete();
    PRInt32       NativeFileOpDirRenameAbort();
    PRInt32       NativeFileOpFileCopyPrepare();
    PRInt32       NativeFileOpFileCopyComplete();
    PRInt32       NativeFileOpFileCopyAbort();
    PRInt32       NativeFileOpFileDeletePrepare();
    PRInt32       NativeFileOpFileDeleteComplete(nsFileSpec *aTarget);
    PRInt32       NativeFileOpFileExecutePrepare();
    PRInt32       NativeFileOpFileExecuteComplete();
    PRInt32       NativeFileOpFileMovePrepare();
    PRInt32       NativeFileOpFileMoveComplete();
    PRInt32       NativeFileOpFileMoveAbort();
    PRInt32       NativeFileOpFileRenamePrepare();
    PRInt32       NativeFileOpFileRenameComplete();
    PRInt32       NativeFileOpFileRenameAbort();
    PRInt32       NativeFileOpWindowsShortcutComplete();
    PRInt32       NativeFileOpWindowsShortcutAbort();
    PRInt32       NativeFileOpMacAliasComplete();
    PRInt32       NativeFileOpMacAliasAbort();
    PRInt32       NativeFileOpUnixLink();

};

#endif /* nsInstallFileOpItem_h__ */

