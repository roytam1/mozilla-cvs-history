/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 
//    This file is included by nsFileSpec.cpp, and includes the Unix-specific
//    implementations.

#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "nsError.h"
#include "prio.h"   /* for PR_Rename */

#if defined(SCO_SV)
#define _SVID3  /* for statvfs.h */
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_STATVFS
#define STATFS	statvfs
#else
#define STATFS	statfs
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024  /* Guessing this is okay.  Works for SCO. */
#endif
 
#if defined(__QNX__)
#include <unix.h>	/* for realpath */
#define f_bavail	f_bfree
#endif

#if defined(SUNOS4)
extern "C" int statfs(char *, struct statfs *);
#endif

#if defined(OSF1)
extern "C" int statvfs(const char *, struct statvfs *);
#endif

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)
// Canonify, make absolute, and check whether directories exist
//----------------------------------------------------------------------------------------
{
    if (ioPath.IsEmpty())
        return;
    if (inMakeDirs)
    {
        const mode_t mode = 0700;
        nsFileSpecHelpers::MakeAllDirectories((const char*)ioPath, mode);
    }
    char buffer[MAXPATHLEN];
    errno = 0;
#if defined(SOLARIS)
    memset(buffer, '\0', sizeof(buffer)); // realpath reads it all, initialized or not.
#else
    *buffer = '\0';
#endif
    char* canonicalPath = realpath((const char*)ioPath, buffer);
    if (!canonicalPath)
    {
        // Linux's realpath() is pathetically buggy.  If the reason for the nil
        // result is just that the leaf does not exist, strip the leaf off,
        // process that, and then add the leaf back.
        nsSimpleCharString allButLeaf(ioPath);
        if (allButLeaf.IsEmpty())
            return;
        char* lastSeparator = strrchr((char*)allButLeaf, '/');
        if (lastSeparator)
        {
            *lastSeparator = '\0';
            canonicalPath = realpath((const char*)allButLeaf, buffer);
            strcat(buffer, "/");
            // Add back the leaf
            strcat(buffer, ++lastSeparator);
        }
    }
    if (!canonicalPath && ioPath[0] != '/' && !inMakeDirs)
    {
        // Well, if it's a relative path, hack it ourselves.
        canonicalPath = realpath(".", buffer);
        if (canonicalPath)
        {
            strcat(canonicalPath, "/");
            strcat(canonicalPath, ioPath);
        }
    }
    if (canonicalPath)
        ioPath = canonicalPath;
} // nsFileSpecHelpers::Canonify

//----------------------------------------------------------------------------------------
void nsFileSpec::SetLeafName(const char* inLeafName)
//----------------------------------------------------------------------------------------
{
    mPath.LeafReplace('/', inLeafName);
} // nsFileSpec::SetLeafName

//----------------------------------------------------------------------------------------
char* nsFileSpec::GetLeafName() const
//----------------------------------------------------------------------------------------
{
    return mPath.GetLeaf('/');
} // nsFileSpec::GetLeafName

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::Exists() const
//----------------------------------------------------------------------------------------
{
    struct stat st;
    return !mPath.IsEmpty() && 0 == stat(mPath, &st); 
} // nsFileSpec::Exists

//----------------------------------------------------------------------------------------
void nsFileSpec::GetModDate(TimeStamp& outStamp) const
//----------------------------------------------------------------------------------------
{
	struct stat st;
    if (!mPath.IsEmpty() && stat(mPath, &st) == 0) 
        outStamp = st.st_mtime; 
    else
        outStamp = 0;
} // nsFileSpec::GetModDate

//----------------------------------------------------------------------------------------
PRUint32 nsFileSpec::GetFileSize() const
//----------------------------------------------------------------------------------------
{
	struct stat st;
    if (!mPath.IsEmpty() && stat(mPath, &st) == 0) 
        return (PRUint32)st.st_size; 
    return 0;
} // nsFileSpec::GetFileSize

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsFile() const
//----------------------------------------------------------------------------------------
{
    struct stat st;
    return !mPath.IsEmpty() && stat(mPath, &st) == 0 && S_ISREG(st.st_mode); 
} // nsFileSpec::IsFile

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsDirectory() const
//----------------------------------------------------------------------------------------
{
    struct stat st;
    return !mPath.IsEmpty() && 0 == stat(mPath, &st) && S_ISDIR(st.st_mode); 
} // nsFileSpec::IsDirectory

//----------------------------------------------------------------------------------------
PRBool nsFileSpec::IsHidden() const
//----------------------------------------------------------------------------------------
{
    PRBool hidden = PR_TRUE;
    char *leafname = GetLeafName();
    if (nsnull != leafname)
    {
        if ((!strcmp(leafname, ".")) || (!strcmp(leafname, "..")))
        {
            hidden = PR_FALSE;
        }
        nsCRT::free(leafname);
    }
    return hidden;
} // nsFileSpec::IsHidden

//----------------------------------------------------------------------------------------
void nsFileSpec::GetParent(nsFileSpec& outSpec) const
//----------------------------------------------------------------------------------------
{
    outSpec.mPath = mPath;
	char* chars = (char*)outSpec.mPath;
	chars[outSpec.mPath.Length() - 1] = '\0'; // avoid trailing separator, if any
    char* cp = strrchr(chars, '/');
    if (cp++)
	    outSpec.mPath.SetLength(cp - chars); // truncate.
} // nsFileSpec::GetParent

//----------------------------------------------------------------------------------------
void nsFileSpec::operator += (const char* inRelativePath)
//----------------------------------------------------------------------------------------
{
    if (!inRelativePath || mPath.IsEmpty())
        return;
    
    char endChar = mPath[(int)(strlen(mPath) - 1)];
    if (endChar == '/')
        mPath += "x";
    else
        mPath += "/x";
    SetLeafName(inRelativePath);
} // nsFileSpec::operator +=

//----------------------------------------------------------------------------------------
void nsFileSpec::CreateDirectory(int mode)
//----------------------------------------------------------------------------------------
{
    // Note that mPath is canonical!
    if (mPath.IsEmpty())
        return;
    mkdir(mPath, mode);
} // nsFileSpec::CreateDirectory

//----------------------------------------------------------------------------------------
void nsFileSpec::Delete(PRBool inRecursive) const
// To check if this worked, call Exists() afterwards, see?
//----------------------------------------------------------------------------------------
{
    if (IsDirectory())
    {
        if (inRecursive)
        {
            for (nsDirectoryIterator i(*this); i.Exists(); i++)
            {
                nsFileSpec& child = (nsFileSpec&)i;
                child.Delete(inRecursive);
            }        
        }
        rmdir(mPath);
    }
    else if (!mPath.IsEmpty())
        remove(mPath);
} // nsFileSpec::Delete

//----------------------------------------------------------------------------------------
void nsFileSpec::RecursiveCopy(nsFileSpec newDir) const
//----------------------------------------------------------------------------------------
{
    if (IsDirectory())
    {
		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

		for (nsDirectoryIterator i(*this); i.Exists(); i++)
		{
			nsFileSpec& child = (nsFileSpec&)i;

			if (child.IsDirectory())
			{
				nsFileSpec tmpDirSpec(newDir);

				char *leafname = child.GetLeafName();
				tmpDirSpec += leafname;
				nsCRT::free(leafname);

				child.RecursiveCopy(tmpDirSpec);
			}
			else
			{
   				child.RecursiveCopy(newDir);
			}
		}
    }
    else if (!mPath.IsEmpty())
    {
		nsFileSpec& filePath = (nsFileSpec&) *this;

		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

        filePath.Copy(newDir);
    }
} // nsFileSpec::RecursiveCopy


//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Rename(const char* inNewName)
//----------------------------------------------------------------------------------------
{
    // This function should not be used to move a file on disk. 
    if (mPath.IsEmpty() || strchr(inNewName, '/')) 
        return NS_FILE_FAILURE;

    char* oldPath = nsCRT::strdup(mPath);
    
    SetLeafName(inNewName); 

    if (PR_Rename(oldPath, mPath) != NS_OK)
    {
        // Could not rename, set back to the original.
        mPath = oldPath;
        return NS_FILE_FAILURE;
    }
    
    nsCRT::free(oldPath);

    return NS_OK;
} // nsFileSpec::Rename

//----------------------------------------------------------------------------------------
static int CrudeFileCopy(const char* in, const char* out)
//----------------------------------------------------------------------------------------
{
	struct stat in_stat;
	int stat_result = -1;

	char	buf [1024];
	FILE	*ifp, *ofp;
	int	rbytes, wbytes;

	if (!in || !out)
		return -1;

	stat_result = stat (in, &in_stat);

	ifp = fopen (in, "r");
	if (!ifp) 
	{
		return -1;
	}

	ofp = fopen (out, "w");
	if (!ofp)
	{
		fclose (ifp);
		return -1;
	}

	while ((rbytes = fread (buf, 1, sizeof(buf), ifp)) > 0)
	{
		while (rbytes > 0)
		{
			if ( (wbytes = fwrite (buf, 1, rbytes, ofp)) < 0 )
			{
				fclose (ofp);
				fclose (ifp);
				unlink(out);
				return -1;
			}
			rbytes -= wbytes;
		}
	}
	fclose (ofp);
	fclose (ifp);

	if (stat_result == 0)
		chmod (out, in_stat.st_mode & 0777);

	return 0;
} // nsFileSpec::Rename

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Copy(const nsFileSpec& inParentDirectory) const
//----------------------------------------------------------------------------------------
{
    // We can only copy into a directory, and (for now) can not copy entire directories
    nsresult result = NS_FILE_FAILURE;

    if (inParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inParentDirectory.GetCString());
        destPath += "/";
        destPath += leafname;
        nsCRT::free(leafname);
        result = NS_FILE_RESULT(CrudeFileCopy(GetCString(), destPath));
    }
    return result;
} // nsFileSpec::Copy

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Move(const nsFileSpec& inNewParentDirectory)
//----------------------------------------------------------------------------------------
{
    // We can only copy into a directory, and (for now) can not copy entire directories
    nsresult result = NS_FILE_FAILURE;

    if (inNewParentDirectory.IsDirectory() && !IsDirectory())
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inNewParentDirectory.GetCString());
        destPath += "/";
        destPath += leafname;
        nsCRT::free(leafname);

        result = NS_FILE_RESULT(CrudeFileCopy(GetCString(), (const char*)destPath));
        if (result == NS_OK)
        {
            // cast to fix const-ness
		    ((nsFileSpec*)this)->Delete(PR_FALSE);
        
            *this = inNewParentDirectory + GetLeafName(); 
    	}
    }
    return result;
} 

//----------------------------------------------------------------------------------------
nsresult nsFileSpec::Execute(const char* inArgs ) const
//----------------------------------------------------------------------------------------
{
    nsresult result = NS_FILE_FAILURE;
    
    if (!mPath.IsEmpty() && !IsDirectory())
    {
        nsSimpleCharString fileNameWithArgs = mPath + " " + inArgs;
        result = NS_FILE_RESULT(system(fileNameWithArgs));
    } 

    return result;

} // nsFileSpec::Execute

//----------------------------------------------------------------------------------------
PRUint32 nsFileSpec::GetDiskSpaceAvailable() const
//----------------------------------------------------------------------------------------
{
    char curdir [MAXPATHLEN];
    if (mPath.IsEmpty())
    {
        (void) getcwd(curdir, MAXPATHLEN);
        if (!curdir)
            return ULONG_MAX;  /* hope for the best as we did in cheddar */
    }
    else
        sprintf(curdir, "%.200s", (const char*)mPath);
 
    struct STATFS fs_buf;
#if defined(__QNX__) && !defined(HAVE_STATVFS) /* Maybe this should be handled differently? */
    if (STATFS(curdir, &fs_buf, 0, 0) < 0)
#else
    if (STATFS(curdir, &fs_buf) < 0)
#endif
        return ULONG_MAX; /* hope for the best as we did in cheddar */
 
#ifdef DEBUG_DISK_SPACE
    printf("DiskSpaceAvailable: %d bytes\n", 
       fs_buf.f_bsize * (fs_buf.f_bavail - 1));
#endif
    return fs_buf.f_bsize * (fs_buf.f_bavail - 1);
} // nsFileSpec::GetDiskSpace()

//========================================================================================
//                                nsDirectoryIterator
//========================================================================================

//----------------------------------------------------------------------------------------
nsDirectoryIterator::nsDirectoryIterator(
    const nsFileSpec& inDirectory
,   int /*inIterateDirection*/)
//----------------------------------------------------------------------------------------
    : mCurrent(inDirectory)
    , mExists(PR_FALSE)
    , mDir(nsnull)
{
    mCurrent += "sysygy"; // prepare the path for SetLeafName
    mDir = opendir((const char*)nsFilePath(inDirectory));
    ++(*this);
} // nsDirectoryIterator::nsDirectoryIterator

//----------------------------------------------------------------------------------------
nsDirectoryIterator::~nsDirectoryIterator()
//----------------------------------------------------------------------------------------
{
    if (mDir)
        closedir(mDir);
} // nsDirectoryIterator::nsDirectoryIterator

//----------------------------------------------------------------------------------------
nsDirectoryIterator& nsDirectoryIterator::operator ++ ()
//----------------------------------------------------------------------------------------
{
    mExists = PR_FALSE;
    if (!mDir)
        return *this;
    char* dot    = ".";
    char* dotdot = "..";
    struct dirent* entry = readdir(mDir);
    if (entry && strcmp(entry->d_name, dot) == 0)
        entry = readdir(mDir);
    if (entry && strcmp(entry->d_name, dotdot) == 0)
        entry = readdir(mDir);
    if (entry)
    {
        mExists = PR_TRUE;
        mCurrent.SetLeafName(entry->d_name);
    }
    return *this;
} // nsDirectoryIterator::operator ++

//----------------------------------------------------------------------------------------
nsDirectoryIterator& nsDirectoryIterator::operator -- ()
//----------------------------------------------------------------------------------------
{
    return ++(*this); // can't do it backwards.
} // nsDirectoryIterator::operator --
