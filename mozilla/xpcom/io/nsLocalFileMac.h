/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Steve Dagley <sdagley@netscape.com>
 */

#ifndef _nsLocalFileMAC_H_
#define _nsLocalFileMAC_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsILocalFileMac.h"
#include "nsIFactory.h"
#include "nsLocalFile.h"

#include <Files.h>

typedef enum {
	eInitWithPath = 0,
	eInitWithFSSpec
} nsLocalFileMacInitType;

class NS_COM nsLocalFile : public nsILocalFile, public nsILocalFileMac
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)
    
    nsLocalFile();
    virtual ~nsLocalFile();

    static NS_METHOD nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    // nsISupports interface
    NS_DECL_ISUPPORTS
    
    // nsIFile interface
    NS_DECL_NSIFILE
    
    // nsILocalFile interface
    NS_DECL_NSILOCALFILE

  NS_IMETHOD InitWithFSSpec(const FSSpec *fileSpec);

  NS_IMETHOD GetFSSpec(FSSpec *fileSpec);

  NS_IMETHOD GetType(OSType *type);
  NS_IMETHOD SetType(OSType type);
  
  NS_IMETHOD GetCreator(OSType *creator);
  NS_IMETHOD SetCreator(OSType creator);

private:

    // this is the flag which indicates if I can used cached information about the file
    PRBool mStatDirty;

    // If we're inited with a path then we store it here
    nsCString mWorkingPath;
    
    // Any nodes added with AppendPath are stored here
    nsCString mAppendedPath;

    // this will be the resolved path which will *NEVER* be returned to the user
    nsCString mResolvedPath;
    
    // The Mac data structure for a file system object
    FSSpec    mSpec;			// This is the raw spec from InitWIthPath or InitWithFSSpec
    FSSpec    mResolvedSpec;	// This is the spec we've called ResolveAlias on
    
    // Is the mResolvedSpec member valid?  Only after we resolve the mSpec or mWorkingPath
    PRBool	mHaveValidSpec;
    
    // 
    nsLocalFileMacInitType	mInitType;
    
    // Do we have to create the path hierarchy before the spec is usable?
    PRBool	mMustCreate;
    
    void MakeDirty();
    nsresult ResolvePath();
    nsresult ResolveAndStat(PRBool resolveTerminal);

    nsresult CopyMove(nsIFile *newParentDir, const char *newName, PRBool followSymlinks, PRBool move);
    nsresult CopySingleFile(nsIFile *source, nsIFile* dest, const char * newName, PRBool followSymlinks, PRBool move);

};

#endif

