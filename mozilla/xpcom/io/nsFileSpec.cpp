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
 */
 
#include "nsFileSpec.h"

#include "nsFileStream.h"
#include "nsDebug.h"

#include "prtypes.h"
#include "plstr.h"
#include "plbase64.h"
#include "prmem.h"

#include <string.h>
#include <stdio.h>

//========================================================================================
NS_NAMESPACE nsFileSpecHelpers
//========================================================================================
{
    enum
    {    kMaxFilenameLength = 31                // should work on Macintosh, Unix, and Win32.
    ,    kMaxAltDigitLength    = 5
    ,    kMaxCoreLeafNameLength    = (kMaxFilenameLength - (kMaxAltDigitLength + 1))
    };
    NS_NAMESPACE_PROTOTYPE void LeafReplace(
        char*& ioPath,
        char inSeparator,
        const char* inLeafName);
#ifndef XP_MAC
    NS_NAMESPACE_PROTOTYPE void Canonify(char*& ioPath, PRBool inMakeDirs);
    NS_NAMESPACE_PROTOTYPE void MakeAllDirectories(const char* inPath, int mode);
#endif
    NS_NAMESPACE_PROTOTYPE char* GetLeaf(const char* inPath, char inSeparator); // allocated
    NS_NAMESPACE_PROTOTYPE char* StringDup(const char* inString, int allocLength = 0);
    NS_NAMESPACE_PROTOTYPE char* AllocCat(const char* inString1, const char* inString2);
    NS_NAMESPACE_PROTOTYPE char* StringAssign(char*& ioString, const char* inOther);
    NS_NAMESPACE_PROTOTYPE char* ReallocCat(char*& ioString, const char* inString1);
#ifdef XP_PC
    NS_NAMESPACE_PROTOTYPE void NativeToUnix(char*& ioPath);
    NS_NAMESPACE_PROTOTYPE void UnixToNative(char*& ioPath);
#endif
} NS_NAMESPACE_END


//----------------------------------------------------------------------------------------
char* nsFileSpecHelpers::StringDup(
    const char* inString,
    int allocLength)
//----------------------------------------------------------------------------------------
{
    if (!allocLength && inString)
        allocLength = strlen(inString);
    char* newPath = inString || allocLength ? new char[allocLength + 1] : nsnull;
    if (!newPath)
        return nsnull;
    strcpy(newPath, inString);
    return newPath;
} // nsFileSpecHelpers::StringDup

//----------------------------------------------------------------------------------------
char* nsFileSpecHelpers::AllocCat(
    const char* inString1,
    const char* inString2)
//----------------------------------------------------------------------------------------
{
    if (!inString1)
        return inString2 ? StringDup(inString2) : (char*)nsnull;
    if (!inString2)
        return StringDup(inString1);
    char* outString = StringDup(inString1, strlen(inString1) + strlen(inString2));
    if (outString)
        strcat(outString, inString2);
    return outString;
} // nsFileSpecHelpers::StringDup

//----------------------------------------------------------------------------------------
char* nsFileSpecHelpers::StringAssign(
    char*& ioString,
    const char* inString2)
//----------------------------------------------------------------------------------------
{
    if (!inString2)
    {
        delete [] ioString;
        ioString = (char*)nsnull;
        return ioString;
    }
    if (!ioString || (strlen(inString2) > strlen(ioString)))
    {
        delete [] ioString;
        ioString = StringDup(inString2);
        return ioString;
    }
    strcpy(ioString, inString2);
    return ioString;
} // nsFileSpecHelpers::StringAssign

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::LeafReplace(
    char*& ioPath,
    char inSeparator,
    const char* inLeafName)
//----------------------------------------------------------------------------------------
{
    // Find the existing leaf name
    if (!ioPath)
        return;
    if (!inLeafName)
    {
        *ioPath = '\0';
        return;
    }
    char* lastSeparator = strrchr(ioPath, inSeparator);
    int oldLength = strlen(ioPath);
    *(++lastSeparator) = '\0'; // strip the current leaf name
    int newLength = lastSeparator - ioPath + strlen(inLeafName);
    if (newLength > oldLength)
    {
        char* newPath = StringDup(ioPath, newLength + 1);
        delete [] ioPath;
        ioPath = newPath;
    }
    strcat(ioPath, inLeafName);
} // nsFileSpecHelpers::LeafReplace

//----------------------------------------------------------------------------------------
char* nsFileSpecHelpers::GetLeaf(const char* inPath, char inSeparator)
// Returns a pointer to an allocated string representing the leaf.
//----------------------------------------------------------------------------------------
{
    if (!inPath)
        return nsnull;
    char* lastSeparator = strrchr(inPath, inSeparator);
    if (lastSeparator)
        return StringDup(++lastSeparator);
    return StringDup(inPath);
} // nsFileSpecHelpers::GetLeaf

#if defined(XP_UNIX) || defined(XP_PC)

//----------------------------------------------------------------------------------------
void nsFileSpecHelpers::MakeAllDirectories(const char* inPath, int mode)
// Make the path a valid one by creating all the intermediate directories.  Does NOT
// make the leaf into a directory.  This should be a unix path.
//----------------------------------------------------------------------------------------
{
	if (!inPath)
		return;
		
    char* pathCopy = nsFileSpecHelpers::StringDup( inPath );
    if (!pathCopy)
    	return;

	const char kSeparator = '/'; // I repeat: this should be a unix-style path.
    const int kSkipFirst = 1;

#ifdef XP_PC
    // Either this is a relative path, or we ensure that it has
    // a drive letter specifier.
    NS_ASSERTION( pathCopy[0] != '/' || pathCopy[2] == '|', "No drive letter!" );
#endif
    char* currentStart = pathCopy;
    char* currentEnd = strchr(currentStart + kSkipFirst, kSeparator);
    if (currentEnd)
    {
		*currentEnd = '\0';
		nsFileSpec spec(nsFilePath(pathCopy, PR_FALSE));
		do
		{
			// If the node doesn't exist, and it is not the initial node in a full path,
			// then make a directory (We cannot make the initial (volume) node).
			if (!spec.Exists() && *currentStart != kSeparator)
				spec.CreateDirectory(mode);
			if (!spec.Exists())
			{
				NS_ASSERTION(spec.Exists(), "Could not create the directory?");
				break;
			}
			currentStart = ++currentEnd;
			currentEnd = strchr(currentStart, kSeparator);
			if (!currentEnd)
				break;
			spec += currentStart; // "lengthen" the path, adding the next node.
		} while (currentEnd);
    }
	delete [] pathCopy;
} // nsFileSpecHelpers::MakeAllDirectories

#endif // XP_PC || XP_UNIX

//----------------------------------------------------------------------------------------
char* nsFileSpecHelpers::ReallocCat(char*& ioString, const char* inString1)
//----------------------------------------------------------------------------------------
{
    char* newString = AllocCat(ioString, inString1);
    delete [] ioString;
    ioString = newString;
    return ioString;
} // nsFileSpecHelpers::ReallocCat

#if defined(XP_PC)
#include "windows/nsFileSpecWin.cpp" // Windows-specific implementations
#elif defined(XP_MAC)
#include "nsFileSpecMac.cpp" // Macintosh-specific implementations
#elif defined(XP_UNIX)
#include "unix/nsFileSpecUnix.cpp" // Unix-specific implementations
#endif

//========================================================================================
//                                nsFileURL implementation
//========================================================================================

#ifndef XP_MAC
//----------------------------------------------------------------------------------------
nsFileURL::nsFileURL(const char* inString, PRBool inCreateDirs)
//----------------------------------------------------------------------------------------
:    mURL(nsnull)
{
    if (!inString)
    	return;
    NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
    // Make canonical and absolute.
	nsFilePath path(inString + kFileURLPrefixLength, inCreateDirs);
	*this = path;
} // nsFileURL::nsFileURL
#endif

//----------------------------------------------------------------------------------------
nsFileURL::nsFileURL(const nsFileURL& inOther)
//----------------------------------------------------------------------------------------
:    mURL(nsFileSpecHelpers::StringDup(inOther.mURL))
#ifdef XP_MAC
,    mFileSpec(inOther.GetFileSpec())
#endif
{
} // nsFileURL::nsFileURL

//----------------------------------------------------------------------------------------
nsFileURL::nsFileURL(const nsFilePath& inOther)
//----------------------------------------------------------------------------------------
:    mURL(nsFileSpecHelpers::AllocCat(kFileURLPrefix, (const char*)inOther))
#ifdef XP_MAC
,    mFileSpec(inOther.GetFileSpec())
#endif
{
} // nsFileURL::nsFileURL

//----------------------------------------------------------------------------------------
nsFileURL::nsFileURL(const nsFileSpec& inOther)
:    mURL(nsFileSpecHelpers::AllocCat(kFileURLPrefix, (const char*)nsFilePath(inOther)))
#ifdef XP_MAC
,    mFileSpec(inOther)
#endif
//----------------------------------------------------------------------------------------
{
} // nsFileURL::nsFileURL

//----------------------------------------------------------------------------------------
nsFileURL::~nsFileURL()
//----------------------------------------------------------------------------------------
{
    delete [] mURL;
}

//----------------------------------------------------------------------------------------
void nsFileURL::operator = (const char* inString)
//----------------------------------------------------------------------------------------
{
    nsFileSpecHelpers::StringAssign(mURL, inString);
    NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
#ifdef XP_MAC
    mFileSpec = inString + kFileURLPrefixLength;
#endif
} // nsFileURL::operator =

//----------------------------------------------------------------------------------------
void nsFileURL::operator = (const nsFileURL& inOther)
//----------------------------------------------------------------------------------------
{
    mURL = nsFileSpecHelpers::StringAssign(mURL, inOther.mURL);
#ifdef XP_MAC
    mFileSpec = inOther.GetFileSpec();
#endif
} // nsFileURL::operator =

//----------------------------------------------------------------------------------------
void nsFileURL::operator = (const nsFilePath& inOther)
//----------------------------------------------------------------------------------------
{
    delete [] mURL;
    mURL = nsFileSpecHelpers::AllocCat(kFileURLPrefix, (const char*)inOther);
#ifdef XP_MAC
    mFileSpec  = inOther.GetFileSpec();
#endif
} // nsFileURL::operator =

//----------------------------------------------------------------------------------------
void nsFileURL::operator = (const nsFileSpec& inOther)
//----------------------------------------------------------------------------------------
{
    delete [] mURL;
    mURL = nsFileSpecHelpers::AllocCat(kFileURLPrefix, (const char*)nsFilePath(inOther));
#ifdef XP_MAC
    mFileSpec  = inOther;
#endif
} // nsFileURL::operator =

//----------------------------------------------------------------------------------------
nsBasicOutStream& operator << (nsBasicOutStream& s, const nsFileURL& url)
//----------------------------------------------------------------------------------------
{
    return (s << url.mURL);
}

//========================================================================================
//                                nsFilePath implementation
//========================================================================================

nsFilePath::nsFilePath(const nsFilePath& inPath)
    : mPath(nsFileSpecHelpers::StringDup(inPath.mPath))
#ifdef XP_MAC
    , mFileSpec(inPath.mFileSpec)
#endif
{
}

#ifndef XP_MAC
//----------------------------------------------------------------------------------------
nsFilePath::nsFilePath(const char* inString, PRBool inCreateDirs)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup(inString))
{
    NS_ASSERTION(strstr(inString, kFileURLPrefix) != inString, "URL passed as path");

#ifdef XP_PC
    nsFileSpecHelpers::UnixToNative(mPath);
#endif
    // Make canonical and absolute.
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
#ifdef XP_PC
    NS_ASSERTION( mPath[1] == ':', "unexpected canonical path" );
    nsFileSpecHelpers::NativeToUnix(mPath);
#endif
}
#endif

//----------------------------------------------------------------------------------------
nsFilePath::nsFilePath(const nsFileURL& inOther)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup(inOther.mURL + kFileURLPrefixLength))
#ifdef XP_MAC
,    mFileSpec(inOther.GetFileSpec())
#endif
{
}

#ifdef XP_UNIX
//----------------------------------------------------------------------------------------
nsFilePath::nsFilePath(const nsFileSpec& inOther)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup(inOther.mPath))
{
}
#endif // XP_UNIX

//----------------------------------------------------------------------------------------
nsFilePath::~nsFilePath()
//----------------------------------------------------------------------------------------
{
    delete [] mPath;
}

#ifdef XP_UNIX
//----------------------------------------------------------------------------------------
void nsFilePath::operator = (const nsFileSpec& inOther)
//----------------------------------------------------------------------------------------
{
    mPath = nsFileSpecHelpers::StringAssign(mPath, inOther.mPath);
}
#endif // XP_UNIX

//----------------------------------------------------------------------------------------
void nsFilePath::operator = (const char* inString)
//----------------------------------------------------------------------------------------
{
    NS_ASSERTION(strstr(inString, kFileURLPrefix) != inString, "URL passed as path");
#ifdef XP_MAC
    mFileSpec = inString;
	nsFileSpecHelpers::StringAssign(mPath, (const char*)nsFilePath(mFileSpec));
#else
#ifdef XP_PC
	nsFileSpecHelpers::UnixToNative(mPath);
#endif
    // Make canonical and absolute.
    nsFileSpecHelpers::Canonify(mPath, PR_FALSE /* XXX? */);
#ifdef XP_PC
	nsFileSpecHelpers::NativeToUnix(mPath);
#endif
#endif // XP_MAC
}

//----------------------------------------------------------------------------------------
void nsFilePath::operator = (const nsFileURL& inOther)
//----------------------------------------------------------------------------------------
{
    nsFileSpecHelpers::StringAssign(mPath, (const char*)nsFilePath(inOther));
#ifdef XP_MAC
    mFileSpec = inOther.GetFileSpec();
#endif
}

//========================================================================================
//                                nsFileSpec implementation
//========================================================================================

#ifndef XP_MAC
//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec()
//----------------------------------------------------------------------------------------
:    mPath(nsnull)
{
}
#endif

//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const nsPersistentFileDescriptor& inDescriptor)
//----------------------------------------------------------------------------------------
#ifndef XP_MAC
:    mPath(nsnull)
#endif
{
	*this = inDescriptor;
}

//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const nsFileURL& inURL)
//----------------------------------------------------------------------------------------
#ifndef XP_MAC
:    mPath(nsnull)
#endif
{
    *this = nsFilePath(inURL); // convert to unix path first
}

//----------------------------------------------------------------------------------------
void nsFileSpec::MakeUnique(const char* inSuggestedLeafName)
//----------------------------------------------------------------------------------------
{
    if (inSuggestedLeafName && *inSuggestedLeafName)
        SetLeafName(inSuggestedLeafName);

    MakeUnique();
} // nsFileSpec::MakeUnique

//----------------------------------------------------------------------------------------
void nsFileSpec::MakeUnique()
//----------------------------------------------------------------------------------------
{
    if (!Exists())
        return;

    char* leafName = GetLeafName();
    if (!leafName)
        return;

    char* lastDot = strrchr(leafName, '.');
    char* suffix = "";
    if (lastDot)
    {
        suffix = nsFileSpecHelpers::StringDup(lastDot); // include '.'
        *lastDot = '\0'; // strip suffix and dot.
    }
    const int kMaxRootLength
        = nsFileSpecHelpers::kMaxCoreLeafNameLength - strlen(suffix) - 1;
    if (strlen(leafName) > kMaxRootLength)
        leafName[kMaxRootLength] = '\0';
    for (short index = 1; index < 1000 && Exists(); index++)
    {
        // start with "Picture-1.jpg" after "Picture.jpg" exists
        char newName[nsFileSpecHelpers::kMaxFilenameLength + 1];
        sprintf(newName, "%s-%d%s", leafName, index, suffix);
        SetLeafName(newName);
    }
    if (*suffix)
        delete [] suffix;
    delete [] leafName;
} // nsFileSpec::MakeUnique

//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const nsFileURL& inURL)
//----------------------------------------------------------------------------------------
{
    *this = nsFilePath(inURL); // convert to unix path first
}

//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const nsPersistentFileDescriptor& inDescriptor)
//----------------------------------------------------------------------------------------
{
#ifdef XP_MAC
	void* data;
	PRInt32 dataSize;
    inDescriptor.GetData(data, dataSize);
    char* decodedData = PL_Base64Decode((const char*)data, (int)dataSize, nsnull);
	// Cast to an alias record and resolve.
	AliasHandle aliasH = nsnull;
	mError = PtrToHand(decodedData, &(Handle)aliasH, (dataSize * 3) / 4);
	PR_Free(decodedData);
	if (mError != noErr)
		return; // not enough memory?

	Boolean changed;
	mError = ::ResolveAlias(nsnull, aliasH, &mSpec, &changed);
	DisposeHandle((Handle) aliasH);
#else
    nsFileSpecHelpers::StringAssign(mPath, inDescriptor.GetString());
#endif
}

//========================================================================================
//                                UNIX & WIN nsFileSpec implementation
//========================================================================================

#ifdef XP_UNIX
//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const nsFilePath& inPath)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup((const char*)inPath))
{
}
#endif // XP_UNIX

#ifdef XP_UNIX
//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const nsFilePath& inPath)
//----------------------------------------------------------------------------------------
{
    nsFileSpecHelpers::StringAssign(mPath, (const char*)inPath);
}
#endif //XP_UNIX

#if defined(XP_UNIX) || defined(XP_PC)
//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup(inSpec.mPath))
{
}
#endif //XP_UNIX

#if defined(XP_UNIX) || defined(XP_PC)
//----------------------------------------------------------------------------------------
nsFileSpec::nsFileSpec(const char* inString, PRBool inCreateDirs)
//----------------------------------------------------------------------------------------
:    mPath(nsFileSpecHelpers::StringDup(inString))
{
    // Make canonical and absolute.
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
}
#endif //XP_UNIX,PC

//----------------------------------------------------------------------------------------
nsFileSpec::~nsFileSpec()
//----------------------------------------------------------------------------------------
{
#ifndef XP_MAC
    delete [] mPath;
#endif
}

#if defined(XP_UNIX) || defined(XP_PC)
//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
{
    mPath = nsFileSpecHelpers::StringAssign(mPath, inSpec.mPath);
}
#endif //XP_UNIX


#if defined(XP_UNIX) || defined(XP_PC)
//----------------------------------------------------------------------------------------
void nsFileSpec::operator = (const char* inString)
//----------------------------------------------------------------------------------------
{
    mPath = nsFileSpecHelpers::StringAssign(mPath, inString);
    // Make canonical and absolute.
    nsFileSpecHelpers::Canonify(mPath, PR_TRUE /* XXX? */);
}
#endif //XP_UNIX

#if (defined(XP_UNIX) || defined(XP_PC))
//----------------------------------------------------------------------------------------
nsBasicOutStream& operator << (nsBasicOutStream& s, const nsFileSpec& spec)
//----------------------------------------------------------------------------------------
{
    return (s << (const char*)spec.mPath);
}
#endif // DEBUG && XP_UNIX

//----------------------------------------------------------------------------------------
nsFileSpec nsFileSpec::operator + (const char* inRelativePath) const
//----------------------------------------------------------------------------------------
{
    nsFileSpec result = *this;
    result += inRelativePath;
    return result;
} // nsFileSpec::operator +

//========================================================================================
//	class nsPersistentFileDescriptor
//========================================================================================

//----------------------------------------------------------------------------------------
nsPersistentFileDescriptor::nsPersistentFileDescriptor(const nsPersistentFileDescriptor& inDesc)
//----------------------------------------------------------------------------------------
    : mDescriptorString(nsFileSpecHelpers::StringDup(inDesc.mDescriptorString))
{
} // nsPersistentFileDescriptor::nsPersistentFileDescriptor

//----------------------------------------------------------------------------------------
void nsPersistentFileDescriptor::operator = (const nsPersistentFileDescriptor& inDesc)
//----------------------------------------------------------------------------------------
{
    nsFileSpecHelpers::StringAssign(mDescriptorString, inDesc.mDescriptorString);
} // nsPersistentFileDescriptor::operator =

//----------------------------------------------------------------------------------------
nsPersistentFileDescriptor::nsPersistentFileDescriptor(const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
    : mDescriptorString(nsnull)
{
	*this = inSpec;
} // nsPersistentFileDescriptor::nsPersistentFileDescriptor

//----------------------------------------------------------------------------------------
void nsPersistentFileDescriptor::operator = (const nsFileSpec& inSpec)
//----------------------------------------------------------------------------------------
{
#ifdef XP_MAC
    if (inSpec.Error())
    	return;
	AliasHandle	aliasH;
	OSErr err = NewAlias(nil, inSpec.operator const FSSpec* const (), &aliasH);
	if (err != noErr)
		return;

	PRUint32 bytes = GetHandleSize((Handle) aliasH);
	HLock((Handle) aliasH);
	char* buf = PL_Base64Encode((const char*)*aliasH, bytes, nsnull);
	DisposeHandle((Handle) aliasH);

    nsFileSpecHelpers::StringAssign(mDescriptorString, buf);
    PR_Free(buf);
#else
    nsFileSpecHelpers::StringAssign(mDescriptorString, inSpec);
#endif // XP_MAC
} // nsPersistentFileDescriptor::operator =

//----------------------------------------------------------------------------------------
nsPersistentFileDescriptor::~nsPersistentFileDescriptor()
//----------------------------------------------------------------------------------------
{
    delete [] mDescriptorString;
} // nsPersistentFileDescriptor::~nsPersistentFileDescriptor

//----------------------------------------------------------------------------------------
void nsPersistentFileDescriptor::GetData(void*& outData, PRInt32& outSize) const
//----------------------------------------------------------------------------------------
{
    outSize = PL_strlen(mDescriptorString);
    outData = mDescriptorString;
}

//----------------------------------------------------------------------------------------
void nsPersistentFileDescriptor::SetData(const void* inData, PRInt32 inSize)
//----------------------------------------------------------------------------------------
{
	delete [] mDescriptorString;
	mDescriptorString = new char[1 + inSize];
	if (!mDescriptorString)
	    return;
	memcpy(mDescriptorString, inData, inSize);
	mDescriptorString[inSize] = '\0';
}

//----------------------------------------------------------------------------------------
nsBasicInStream& operator >> (nsBasicInStream& s, nsPersistentFileDescriptor& d)
// reads the data from a file
//----------------------------------------------------------------------------------------
{
	char bigBuffer[1000];
	// The first 8 bytes of the data should be a hex version of the data size to follow.
	PRInt32 bytesRead = 8;
	s.read(bigBuffer, bytesRead);
	if (bytesRead != 8)
		return s;
	bigBuffer[8] = '\0';
	sscanf(bigBuffer, "%lx", &bytesRead);
	if (bytesRead > 0x4000)
		return s; // preposterous.
	// Now we know how many bytes to read, do it.
	s.read(bigBuffer, bytesRead);
	d.SetData(bigBuffer, bytesRead);
	return s;
}

//----------------------------------------------------------------------------------------
nsBasicOutStream& operator << (nsBasicOutStream& s, const nsPersistentFileDescriptor& d)
// writes the data to a file
//----------------------------------------------------------------------------------------
{
	char littleBuf[9];
	PRInt32 dataSize;
	void* data;
	d.GetData(data, dataSize);
	// First write (in hex) the length of the data to follow.  Exactly 8 bytes
	sprintf(littleBuf, "%0.8x", dataSize);
	s << littleBuf;
	// Now write the data itself
	s << d.mDescriptorString;
	return s;
}
