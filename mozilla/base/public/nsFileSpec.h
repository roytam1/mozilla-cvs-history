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

//    First checked in on 98/11/20 by John R. McMullen in the wrong directory.
//    Checked in again 98/12/04.
//    Polished version 98/12/08.

//========================================================================================
//
//  Classes defined:
//
//      nsFilePath, nsFileURL, nsFileSpec, nsPersistentFileDescriptor
//      nsDirectoryIterator. Oh, and a convenience class nsAutoCString.
//
//  Q.  How should I represent files at run time?
//  A.  Use nsFileSpec.  Using char* will lose information on some platforms.
//
//  Q.  Then what are nsFilePath and nsFileURL for?
//  A.  Only when you need a char* parameter for legacy code.
//
//  Q.  How should I represent files in a persistent way (eg, in a disk file)?
//  A.  Use nsPersistentFileDescriptor.  Convert to and from nsFileSpec at run time.
//
//  This suite provides the following services:
//
//      1.  Encapsulates all platform-specific file details, so that files can be
//          described correctly without any platform #ifdefs
//
//      2.  Type safety.  This will fix the problems that used to occur because people
//          confused file paths.  They used to use const char*, which could mean three
//          or four different things.  Bugs were introduced as people coded, right up
//          to the moment Communicator 4.5 shipped.
//
//      3.  Used in conjunction with nsFileStream.h (q.v.), this supports all the power
//          and readability of the ansi stream syntax.
//
//          Basic example:
//
//              nsFilePath myPath("/Development/iotest.txt");
//
//              nsOutputFileStream testStream(myPath);
//              testStream << "Hello World" << nsEndl;
//
//      4.  Handy methods for manipulating file specifiers safely, e.g. MakeUnique(),
//          SetLeafName(), Exists().
//
//      5.  Easy cross-conversion.
//
//          Examples:
//
//              Initialize a URL from a string without suffix
//
//                  nsFileURL fileURL("file:///Development/MPW/MPW%20Shell");
//
//              Initialize a Unix path from a URL
//
//                  nsFilePath filePath(fileURL);
//
//              Initialize a native file spec from a URL
//
//                  nsFileSpec fileSpec(fileURL);
//
//              Make the spec unique (this one has no suffix).
//
//                  fileSpec.MakeUnique();
//
//              Assign the spec to a URL
//
//                  fileURL = fileSpec;
//
//              Assign a unix path using a string with a suffix.
//
//                  filePath = "/Development/MPW/SysErrs.err";
//
//              Assign to a file spec using a unix path.
//
//                  fileSpec = filePath;
//
//              Make this unique (this one has a suffix).
//
//                  fileSpec.MakeUnique();
//
//      6.  Fixes a bug that have been there for a long time, and
//          is inevitable if you use NSPR alone, where files are described as paths.
//
//          The problem affects platforms (Macintosh) in which a path does not fully
//          specify a file, because two volumes can have the same name.  This
//          is solved by holding a "private" native file spec inside the
//          nsFilePath and nsFileURL classes, which is used when appropriate.
//
//      Not yet done:
//
//          Equality operators... much more.
//
//========================================================================================

#ifndef _FILESPEC_H_
#define _FILESPEC_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"

//========================================================================================
//                          Compiler-specific macros, as needed
//========================================================================================
#if !defined(NS_USING_NAMESPACE) && (defined(__MWERKS__) || defined(XP_PC))
#define NS_USING_NAMESPACE
#endif

#ifdef NS_USING_NAMESPACE

#define NS_NAMESPACE_PROTOTYPE
#define NS_NAMESPACE namespace
#define NS_NAMESPACE_END
#define NS_EXPLICIT explicit
#else

#define NS_NAMESPACE_PROTOTYPE static
#define NS_NAMESPACE struct
#define NS_NAMESPACE_END ;
#define NS_EXPLICIT

#endif
//=========================== End Compiler-specific macros ===============================

#ifdef XP_MAC
#include <Files.h>
#elif defined(XP_UNIX) || defined (XP_OS2)
#include <dirent.h>
#elif XP_PC
#include "prio.h"
#endif

//========================================================================================
// Here are the allowable ways to describe a file.
//========================================================================================

class nsFileSpec;             // Preferred.  For i/o use nsInputFileStream, nsOutputFileStream
class nsFilePath;             // This can be passed to NSPR file I/O routines, if you must.
class nsFileURL;
class nsPersistentFileDescriptor; // Used for storage across program launches.

#define kFileURLPrefix "file://"
#define kFileURLPrefixLength (7)

class nsOutputStream;
class nsInputStream;
class nsIOutputStream;
class nsIInputStream;
class nsOutputFileStream;
class nsInputFileStream;
class nsString;

//========================================================================================
// Conversion of native file errors to nsresult values. These are really only for use
// in the file module, clients of this interface shouldn't really need them.
// Error results returned from this interface have, in the low-order 16 bits,
// native errors that are masked to 16 bits.  Assumption: a native error of 0 is success
// on all platforms. Note the way we define this using an inline function.  This
// avoids multiple evaluation if people go NS_FILE_RESULT(function_call()).
#define NS_FILE_RESULT(x) ns_file_convert_result((PRInt32)x)
nsresult ns_file_convert_result(PRInt32 nativeErr);
#define NS_FILE_FAILURE NS_FILE_RESULT(-1)

//========================================================================================
class NS_BASE nsAutoCString
//
// This should be in nsString.h, but the owner would not reply to my proposal.  After four
// weeks, I decided to put it in here.
//
// This is a quiet little class that acts as a sort of autoptr for
// a const char*.  If you used to call nsString::ToNewCString(), just
// to pass the result a parameter list, it was a nuisance having to
// call delete [] on the result after the call.  Now you can say
//     nsString myStr;
//     ...
//     f(nsAutoCString(myStr));
// where f is declared as void f(const char*);  This call will
// make a temporary char* pointer on the stack and delete[] it
// when the function returns.
//========================================================================================
{
public:
    NS_EXPLICIT                  nsAutoCString(const nsString& other) : mCString(other.ToNewCString()) {}
    virtual                      ~nsAutoCString();    
                                 operator const char*() const { return mCString; }
                                 operator const char*() { return mCString; }
protected:
                                 const char* mCString;
}; // class nsAutoCString

//========================================================================================
class NS_BASE nsFileSpec
//    This is whatever each platform really prefers to describe files as.  Declared first
//  because the other two types have an embeded nsFileSpec object.
//========================================================================================
{
    public:
                                nsFileSpec();
        NS_EXPLICIT             nsFileSpec(const char* inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileSpec(const nsString& inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileSpec(const nsFilePath& inPath);
        NS_EXPLICIT             nsFileSpec(const nsFileURL& inURL);
        NS_EXPLICIT             nsFileSpec(const nsPersistentFileDescriptor& inURL);
                                nsFileSpec(const nsFileSpec& inPath);
        virtual                 ~nsFileSpec();

        void                    operator = (const char* inPath);
        void                    operator = (const nsString& inPath)
                                {
                                    const nsAutoCString path(inPath);
                                    *this = path;
                                }
        void                    operator = (const nsFilePath& inPath);
        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const nsFileSpec& inOther);
        void                    operator = (const nsPersistentFileDescriptor& inOther);

#ifndef XP_MAC
                                operator const char* () const { return mPath; }
                                    // This is the only automatic conversion to const char*
                                    // that is provided, and it allows the
                                    // path to be "passed" to NSPR file routines.
#endif

#ifdef XP_MAC
        // For Macintosh people, this is meant to be useful in its own right as a C++ version
        // of the FSSpec struct.        
                                nsFileSpec(
                                    short vRefNum,
                                    long parID,
                                    ConstStr255Param name);
                                nsFileSpec(const FSSpec& inSpec)
                                    : mSpec(inSpec), mError(NS_OK) {}

                                operator FSSpec* () { return &mSpec; }
                                operator const FSSpec* const () { return &mSpec; }
                                operator  FSSpec& () { return mSpec; }
                                operator const FSSpec& () const { return mSpec; }
        void                    MakeAliasSafe();
                                    // Called for the spec of an alias.  Copies the alias to
                                    // a secret temp directory and modifies the spec to point
                                    // to it.  Sets mError.
        void                    ResolveAlias(PRBool& wasAliased);
                                    // Called for the spec of an alias.  Modifies the spec to
                                    // point to the original.  Sets mError.
        void                    MakeUnique(ConstStr255Param inSuggestedLeafName);
        StringPtr               GetLeafPName() { return mSpec.name; }
        ConstStr255Param        GetLeafPName() const { return mSpec.name; }
#endif // end of Macintosh utility methods.

        PRBool                  Valid() const { return NS_SUCCEEDED(Error()); }
        nsresult                Error() const { return mError; }


        friend                  NS_BASE nsOutputStream& operator << (
                                    nsOutputStream& s,
                                    const nsFileSpec& spec); // THIS IS FOR DEBUGGING ONLY.
                                        // see PersistentFileDescriptor for the real deal.


        //--------------------------------------------------
        // Queries and path algebra.  These do not modify the disk.
        //--------------------------------------------------

        char*                   GetLeafName() const; // Allocated.  Use delete [].
        void                    SetLeafName(const char* inLeafName);
                                    // inLeafName can be a relative path, so this allows
                                    // one kind of concatenation of "paths".
        void                    SetLeafName(const nsString& inLeafName)
                                {
                                    const nsAutoCString leafName(inLeafName);
                                    SetLeafName(leafName);
                                }
        void                    GetParent(nsFileSpec& outSpec) const;
                                    // Return the filespec of the parent directory. Used
                                    // in conjunction with GetLeafName(), this lets you
                                    // parse a path into a list of node names.  Beware,
                                    // however, that the top node is still not a name,
                                    // but a spec.  Volumes on Macintosh can have identical
                                    // names.  Perhaps could be used for an operator --() ?

        nsFileSpec              operator + (const char* inRelativePath) const;
        nsFileSpec              operator + (const nsString& inRelativePath) const
                                {
                                    const nsAutoCString 
                                      relativePath(inRelativePath);
                                    return *this + relativePath;
                                }
        void			        operator += (const char* inRelativePath);
                                    // Concatenate the relative path to this directory.
                                    // Used for constructing the filespec of a descendant.
                                    // This must be a directory for this to work.  This differs
                                    // from SetLeafName(), since the latter will work
                                    // starting with a sibling of the directory and throws
                                    // away its leaf information, whereas this one assumes
                                    // this is a directory, and the relative path starts
                                    // "below" this.
        void			        operator += (const nsString& inRelativePath)
                                {
                                    const nsAutoCString relativePath(inRelativePath);
                                    *this += relativePath;
                                }

        void                    MakeUnique();
        void                    MakeUnique(const char* inSuggestedLeafName);
        void                    MakeUnique(const nsString& inSuggestedLeafName)
                                {
                                    const nsAutoCString suggestedLeafName(inSuggestedLeafName);
                                    MakeUnique(suggestedLeafName);
                                }
    
        PRBool                  IsDirectory() const;
                                    // More stringent than Exists()
        PRBool                  IsFile() const;
                                    // More stringent than Exists()
        PRBool                  Exists() const;

        //--------------------------------------------------
        // Creation and deletion of objects.  These can modify the disk.
        //--------------------------------------------------

        void                    CreateDirectory(int mode = 0700 /* for unix */);
        void                    Delete(PRBool inRecursive) const;
        
        nsresult                Rename(const char* inNewName); // not const: gets updated
        nsresult                Rename(const nsString& inNewName)
                                {
                                    const nsAutoCString newName(inNewName);
                                    return Rename(newName);
                                }
        nsresult                Copy(const nsFileSpec& inNewParentDirectory) const;
        nsresult                Move(const nsFileSpec& inNewParentDirectory) const;
        nsresult                Execute(const char* args) const;
        nsresult                Execute(const nsString& args) const
                                {
                                    const nsAutoCString argsString(args);
                                    return Execute(argsString);
                                }

        //--------------------------------------------------
        // Data
        //--------------------------------------------------

    private:
                                friend class nsFilePath;
#ifdef XP_MAC
        FSSpec                  mSpec;
#else
        char*                   mPath;
#endif
        nsresult                mError;
}; // class nsFileSpec

// FOR HISTORICAL REASONS:

typedef nsFileSpec nsNativeFileSpec;

//========================================================================================
class NS_BASE nsFileURL
//    This is an escaped string that looks like "file:///foo/bar/mumble%20fish".  Since URLs
//    are the standard way of doing things in mozilla, this allows a string constructor,
//    which just stashes the string with no conversion.
//========================================================================================
{
    public:
                                nsFileURL(const nsFileURL& inURL);
        NS_EXPLICIT             nsFileURL(const char* inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileURL(const nsString& inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFileURL(const nsFilePath& inPath);
        NS_EXPLICIT             nsFileURL(const nsFileSpec& inPath);
        virtual                 ~nsFileURL();

//        nsString             GetString() const { return mPath; }
                                    // may be needed for implementation reasons,
                                    // but should not provide a conversion constructor.

        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const char* inString);
        void                    operator = (const nsString& inString)
                                {
                                    const nsAutoCString string(inString);
                                    *this = string;
                                }
        void                    operator = (const nsFilePath& inOther);
        void                    operator = (const nsFileSpec& inOther);

                                operator const char* const () { return mURL; }

        friend                  NS_BASE nsOutputStream& operator << (
                                     nsOutputStream& s, const nsFileURL& spec);

#ifdef XP_MAC
                                // Accessor to allow quick assignment to a mFileSpec
        const nsFileSpec&       GetFileSpec() const { return mFileSpec; }
#endif
    private:
        // Should not be defined (only nsFilePath is to be treated as strings.
                                operator char* ();
    private:
                                friend class nsFilePath; // to allow construction of nsFilePath
        char*                   mURL;
#ifdef XP_MAC
        // Since the path on the macintosh does not uniquely specify a file (volumes
        // can have the same name), stash the secret nsFileSpec, too.
        nsFileSpec              mFileSpec;
#endif
}; // class nsFileURL

//========================================================================================
class NS_BASE nsFilePath
//    This is a string that looks like "/foo/bar/mumble%20fish".  Same as nsFileURL, but
//    without the "file:// prefix".
//========================================================================================
{
    public:
                                nsFilePath(const nsFilePath& inPath);
        NS_EXPLICIT             nsFilePath(const char* inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFilePath(const nsString& inString, PRBool inCreateDirs = PR_FALSE);
        NS_EXPLICIT             nsFilePath(const nsFileURL& inURL);
        NS_EXPLICIT             nsFilePath(const nsFileSpec& inPath);
        virtual                 ~nsFilePath();

                                
                                operator const char* () const { return mPath; }
                                    // This is the only automatic conversion to const char*
                                    // that is provided, and it allows the
                                    // path to be "passed" to NSPR file routines.
                                operator char* () { return mPath; }
                                    // This is the only automatic conversion to string
                                    // that is provided, because a naked string should
                                    // only mean a standard file path.

        void                    operator = (const nsFilePath& inPath);
        void                    operator = (const char* inString);
        void                    operator = (const nsString& inString)
                                {
                                    const nsAutoCString string(inString);
                                    *this = string;
                                }
        void                    operator = (const nsFileURL& inURL);
        void                    operator = (const nsFileSpec& inOther);

#ifdef XP_MAC
    public:
                                // Accessor to allow quick assignment to a mFileSpec
        const nsFileSpec&       GetFileSpec() const { return mFileSpec; }
#endif

    private:

        char*                    mPath;
#ifdef XP_MAC
        // Since the path on the macintosh does not uniquely specify a file (volumes
        // can have the same name), stash the secret nsFileSpec, too.
        nsFileSpec               mFileSpec;
#endif
}; // class nsFilePath

//========================================================================================
class NS_BASE nsPersistentFileDescriptor
//========================================================================================
{
    public:
                                nsPersistentFileDescriptor() : mDescriptorString(nsnull) {}
                                    // For use prior to reading in from a stream
                                nsPersistentFileDescriptor(const nsPersistentFileDescriptor& inPath);
        virtual                 ~nsPersistentFileDescriptor();
        void					operator = (const nsPersistentFileDescriptor& inPath);
        
        // Conversions
                                nsPersistentFileDescriptor(const nsFileSpec& inPath);
        void					operator = (const nsFileSpec& inPath);
        
    	nsresult                Read(nsIInputStream* aStream);
    	nsresult                Write(nsIOutputStream* aStream);
    	    // writes the data to a file
    	friend NS_BASE nsInputStream& operator >> (nsInputStream&, nsPersistentFileDescriptor&);
    		// reads the data from a file
    	friend NS_BASE nsOutputStream& operator << (nsOutputStream&, const nsPersistentFileDescriptor&);
    	    // writes the data to a file
        friend class nsFileSpec;

    private:
        // Here are the ways to get data in and out of a file.
        void                    GetData(void*& outData, PRInt32& outSize) const;
                                     // DON'T FREE the returned data!
        void                    SetData(const void* inData, PRInt32 inSize);

    private:

        char*                   mDescriptorString;

}; // class nsPersistentFileDescriptor

//========================================================================================
class NS_BASE nsDirectoryIterator
//  Example:
//
//       nsFileSpec parentDir(...); // directory over whose children we shall iterate
//       for (nsDirectoryIterator i(parentDir); i; i++)
//       {
//              // do something with (const nsFileSpec&)i
//       }
//
//  or:
//
//       for (nsDirectoryIterator i(parentDir, PR_FALSE); i; i--)
//       {
//              // do something with (const nsFileSpec&)i
//       }
//
//  Currently, the only platform on which backwards iteration actually goes backwards
//  is Macintosh.  On other platforms, both styles will work, but will go forwards.
//========================================================================================
{
	public:
	                            nsDirectoryIterator(
	                            	const nsFileSpec& parent,
	                            	int iterateDirection = +1);
#ifndef XP_MAC
	// Macintosh currently doesn't allocate, so needn't clean up.
	    virtual                 ~nsDirectoryIterator();
#endif
	    PRBool                  Exists() const { return mExists; }
	    nsDirectoryIterator&    operator ++(); // moves to the next item, if any.
	    nsDirectoryIterator&    operator ++(int) { return ++(*this); } // post-increment.
	    nsDirectoryIterator&    operator --(); // moves to the previous item, if any.
	    nsDirectoryIterator&    operator --(int) { return --(*this); } // post-decrement.
	                            operator nsFileSpec&() { return mCurrent; }
	private:
	    nsFileSpec              mCurrent;
	    PRBool                  mExists;
	      
#if defined(XP_UNIX)
        DIR*                    mDir;
#elif defined(XP_PC)
        PRDir*                  mDir; // XXX why not use PRDir for Unix & Mac, too?
#elif defined(XP_MAC)
        OSErr                   SetToIndex();
	    short                   mIndex;
	    short                   mMaxIndex;
#endif
}; // class nsDirectoryIterator

#endif //  _FILESPEC_H_
