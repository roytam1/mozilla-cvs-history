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
 * Corporation.	 Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *	   Steve Dagley <sdagley@netscape.com>
 *	   John R. McMullen
 */


#include "nsCOMPtr.h"
#include "nsIAllocator.h"

#include "nsLocalFileMac.h"

#include "nsISimpleEnumerator.h"
#include "nsIComponentManager.h"
#include "prtypes.h"
#include "prerror.h"
#include "pprio.h" // Include this rather than prio.h so we get def of PR_ImportFile
#include "prmem.h"

#include "FullPath.h"
#include "FileCopy.h"
#include "MoreFilesExtras.h"

#include <Errors.h>
#include <Script.h>
#include <Processes.h>
#include <StringCompare.h>

#include <AppleEvents.h>
#include <AEDataModel.h>

#include <Aliases.h>
#include <Folders.h>

// Stupid @#$% header looks like its got extern mojo but it doesn't really
extern "C"
{
#include <FSp_fopen.h>
}

#pragma mark [static util funcs]

static inline void ClearFSSpec(FSSpec& aSpec)
{
	aSpec.vRefNum = 0;
	aSpec.parID = 0;
	aSpec.name[0] = 0;
}


// Simple func to map Mac OS errors into nsresults
static nsresult MacErrorMapper(OSErr inErr)
{
	nsresult outErr;
	
	switch (inErr)
	{
		case noErr:
			outErr = NS_OK;
			break;

		case fnfErr:
			outErr = NS_ERROR_FILE_NOT_FOUND;
			break;

		case dupFNErr:
			outErr = NS_ERROR_FILE_ALREADY_EXISTS;
			break;
		
		case dskFulErr:
			outErr = NS_ERROR_FILE_DISK_FULL;
			break;
		
		case fLckdErr:
			outErr = NS_ERROR_FILE_IS_LOCKED;
			break;
		
		// Can't find good map for some
		case bdNamErr:
			outErr = NS_ERROR_FAILURE;
			break;

		default:	
			outErr = NS_ERROR_FAILURE;
			break;
	}
	return outErr;
}



/*----------------------------------------------------------------------------
	IsEqualFSSpec 
	
	Compare two canonical FSSpec records.
			
	Entry:	file1 = pointer to first FSSpec record.
			file2 = pointer to second FSSpec record.
	
	Exit:	function result = true if the FSSpec records are equal.
----------------------------------------------------------------------------*/

static PRBool IsEqualFSSpec(const FSSpec& file1, const FSSpec& file2)
{
	return
		file1.vRefNum == file2.vRefNum &&
		file1.parID == file2.parID &&
		EqualString(file1.name, file2.name, false, true);
}

/*----------------------------------------------------------------------------
	GetParentFolderSpec
	
	Given an FSSpec to a (possibly non-existent) file, get an FSSpec for its
	parent directory.
	
----------------------------------------------------------------------------*/

static OSErr GetParentFolderSpec(const FSSpec& fileSpec, FSSpec& parentDirSpec)
{
	CInfoPBRec 	pBlock = {0};
	OSErr		err = noErr;
	
	parentDirSpec.name[0] = 0;
	
	pBlock.dirInfo.ioVRefNum = fileSpec.vRefNum;
	pBlock.dirInfo.ioDrDirID = fileSpec.parID;
	pBlock.dirInfo.ioNamePtr = (StringPtr)parentDirSpec.name;
	pBlock.dirInfo.ioFDirIndex = -1;		//get info on parID
	err = PBGetCatInfoSync(&pBlock);
	if (err != noErr) return err;
	
	parentDirSpec.vRefNum = fileSpec.vRefNum;
	parentDirSpec.parID = pBlock.dirInfo.ioDrParID;
	
	return err;
}


/*----------------------------------------------------------------------------
	VolHasDesktopDB 
	
	Check to see if a volume supports the new desktop database.
	
	Entry:	vRefNum = vol ref num of volumn
			
	Exit:	function result = error code.
			*hasDesktop = true if volume has the new desktop database.
----------------------------------------------------------------------------*/

static OSErr VolHasDesktopDB (short vRefNum, Boolean *hasDesktop)
{
	HParamBlockRec 		pb;
	GetVolParmsInfoBuffer	info;
	OSErr 				err = noErr;
	
	pb.ioParam.ioCompletion = nil;
	pb.ioParam.ioNamePtr = nil;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioBuffer = (Ptr)&info;
	pb.ioParam.ioReqCount = sizeof(info);
	err = PBHGetVolParmsSync(&pb);
	*hasDesktop = err == noErr && (info.vMAttrib & (1L << bHasDesktopMgr)) != 0;
	return err;
}


/*----------------------------------------------------------------------------
	GetLastModDateTime
	
	Get the last mod date and time of a file.
	
	Entry:	fSpec = pointer to file spec.
	
	Exit:	function result = error code.
			*lastModDateTime = last mod date and time.
----------------------------------------------------------------------------*/

static OSErr GetLastModDateTime(const FSSpec *fSpec, unsigned long *lastModDateTime)
{
	CInfoPBRec	pBlock;
	OSErr 		err = noErr;
	
	pBlock.hFileInfo.ioNamePtr = (StringPtr)fSpec->name;
	pBlock.hFileInfo.ioVRefNum = fSpec->vRefNum;
	pBlock.hFileInfo.ioFDirIndex = 0;
	pBlock.hFileInfo.ioDirID = fSpec->parID;
	err = PBGetCatInfoSync(&pBlock);
	if (err != noErr) return err;
	*lastModDateTime = pBlock.hFileInfo.ioFlMdDat;
	return noErr;
}


/*----------------------------------------------------------------------------
	FindAppOnVolume 
	
	Find an application on a volume.
	
	Entry:	sig = application signature.
			vRefNum = vol ref num
			
	Exit:	function result = error code
				= afpItemNotFound if app not found on vol.
			*file = file spec for application on volume.
----------------------------------------------------------------------------*/

static OSErr FindAppOnVolume (OSType sig, short vRefNum, FSSpec *file)
{
	DTPBRec 	pb;
	OSErr 		err = noErr;
	short 		ioDTRefNum, i;
	FInfo 		fInfo;
	FSSpec 		candidate;
	unsigned long lastModDateTime, maxLastModDateTime;

	memset(&pb, 0, sizeof(DTPBRec));
	pb.ioCompletion = nil;
	pb.ioVRefNum = vRefNum;
	pb.ioNamePtr = nil;
	err = PBDTGetPath(&pb);
	if (err != noErr) return err;
	ioDTRefNum = pb.ioDTRefNum;

	memset(&pb, 0, sizeof(DTPBRec));
	pb.ioCompletion = nil;
	pb.ioIndex = 0;
	pb.ioFileCreator = sig;
	pb.ioNamePtr = file->name;
	pb.ioDTRefNum = ioDTRefNum;
	err = PBDTGetAPPLSync(&pb);
	
	if (err == fnfErr || err == paramErr) return afpItemNotFound;
	if (err != noErr) return err;

	file->vRefNum = vRefNum;
	file->parID = pb.ioAPPLParID;
	
	err = FSpGetFInfo(file, &fInfo);
	if (err == noErr) return noErr;
	
	i = 1;
	maxLastModDateTime = 0;
	while (true)
	{
		memset(&pb, 0, sizeof(DTPBRec)); 
		pb.ioCompletion = nil;
		pb.ioIndex = i;
		pb.ioFileCreator = sig;
		pb.ioNamePtr = candidate.name;
		pb.ioDTRefNum = ioDTRefNum;
		err = PBDTGetAPPLSync(&pb);
		if (err != noErr) break;
		candidate.vRefNum = vRefNum;
		candidate.parID = pb.ioAPPLParID;
		err = GetLastModDateTime(file, &lastModDateTime);
		if (err == noErr) {
			if (lastModDateTime > maxLastModDateTime) {
				maxLastModDateTime = lastModDateTime;
				*file = candidate;
			}
		}
		i++;
	}
	
	return maxLastModDateTime > 0 ? noErr : afpItemNotFound;
}


/*----------------------------------------------------------------------------
	GetIndVolume 
	
	Get a volume reference number by volume index.
	
	Entry:	index = volume index
			
	Exit:	function result = error code.
			*vRefNum = vol ref num of indexed volume.
----------------------------------------------------------------------------*/

static OSErr GetIndVolume(short index, short *vRefNum)
{
	ParamBlockRec pb;
	OSErr err = noErr;
	
	pb.volumeParam.ioCompletion = nil;
	pb.volumeParam.ioNamePtr = nil;
	pb.volumeParam.ioVolIndex = index;
	
	err = PBGetVInfoSync(&pb);
	
	*vRefNum = pb.volumeParam.ioVRefNum;
	return err;
}


// Private NSPR functions
static unsigned long gJanuaryFirst1970Seconds = 0;
/*
 * The geographic location and time zone information of a Mac
 * are stored in extended parameter RAM.  The ReadLocation
 * produdure uses the geographic location record, MachineLocation,
 * to read the geographic location and time zone information in
 * extended parameter RAM.
 *
 * Because serial port and SLIP conflict with ReadXPram calls,
 * we cache the call here.
 *
 * Caveat: this caching will give the wrong result if a session
 * extend across the DST changeover time.
 */

static void MyReadLocation(MachineLocation *loc)
{
    static MachineLocation storedLoc;
    static Boolean didReadLocation = false;
    
    if (!didReadLocation) {
        ReadLocation(&storedLoc);
        didReadLocation = true;
    }
    *loc = storedLoc;
}

static long GMTDelta(void)
{
    MachineLocation loc;
    long gmtDelta;

    MyReadLocation(&loc);
    gmtDelta = loc.u.gmtDelta & 0x00ffffff;
    if (gmtDelta & 0x00800000) {    /* test sign extend bit */
        gmtDelta |= 0xff000000;
    }
    return gmtDelta;
}

static void MacintoshInitializeTime(void)
{
    /*
     * The NSPR epoch is midnight, Jan. 1, 1970 GMT.
     *
     * At midnight Jan. 1, 1970 GMT, the local time was
     *     midnight Jan. 1, 1970 + GMTDelta().
     *
     * Midnight Jan. 1, 1970 is 86400 * (365 * (1970 - 1904) + 17)
     *     = 2082844800 seconds since the Mac epoch.
     * (There were 17 leap years from 1904 to 1970.)
     *
     * So the NSPR epoch is 2082844800 + GMTDelta() seconds since
     * the Mac epoch.  Whew! :-)
     */
    gJanuaryFirst1970Seconds = 2082844800 + GMTDelta();
}

static nsresult	ConvertMacTimeToUnixTime( PRInt64* aLastModificationDate, PRInt32 timestamp )
{
	if ( gJanuaryFirst1970Seconds == 0)
		MacintoshInitializeTime();
	timestamp -= gJanuaryFirst1970Seconds;
  PRTime			oneMillion, dateInMicroSeconds;
	LL_I2L(dateInMicroSeconds, timestamp);
	LL_I2L(oneMillion, 1000000UL);
	LL_MUL(*aLastModificationDate, oneMillion, dateInMicroSeconds);
	return NS_OK;
}

static nsresult ConvertUnixTimeToMacTime(PRInt64 aUnixTime, PRUint32 *aOutMacTime)
{
	NS_ENSURE_ARG( aOutMacTime );
		
	PRTime oneMillion, dateInSeconds;
	dateInSeconds = LL_ZERO;
	
	LL_I2L(oneMillion, 1000000UL);
	LL_DIV(dateInSeconds, aUnixTime, oneMillion); // dateInSeconds = aUnixTime/1,000,000
	LL_L2UI(*aOutMacTime, dateInSeconds);
	*aOutMacTime += 2082844800; // dateInSeconds + Mac epoch
	
	return NS_OK;
}


#pragma mark -

static void myPLstrcpy(Str255 dst, const char* src)
{
	int srcLength = strlen(src);
	NS_ASSERTION(srcLength <= 255, "Oops, Str255 can't hold >255 chars");
	if (srcLength > 255)
		srcLength = 255;
	dst[0] = srcLength;
	memcpy(&dst[1], src, srcLength);
}

static void myPLstrncpy(Str255 dst, const char* src, int inMax)
{
	int srcLength = strlen(src);
	if (srcLength > inMax)
		srcLength = inMax;
	dst[0] = srcLength;
	memcpy(&dst[1], src, srcLength);
}

// Aaargh!!!!  Gotta roll my own equivalents of PR_OpenDir, PR_ReadDir and PR_CloseDir
// I can't use the NSPR versions as they expect a unix path and I can't easily adapt
// the directory iterator fucntion from MoreFIles as it wants to iterate things itself
// and use a callback function to operate on the iterated entries

typedef struct
{
	short		ioVRefNum;
	long		ioDirID;
	short		ioFDirIndex;
	char		*currentEntryName;
} MyPRDir;

static MyPRDir * My_OpenDir(FSSpec *fileSpec)
{
	MyPRDir		*dir = NULL;
	OSErr		err;
	long		theDirID;
	Boolean		isDirectory;
	
	err = ::FSpGetDirectoryID(fileSpec, &theDirID, &isDirectory);
	if (err == noErr)
	{
		if (isDirectory)
		{
			/* This is a directory, store away the pertinent information.
			** We post increment.  I.e. index is always the nth. item we 
			** should get on the next call
			*/
			dir = PR_NEW(MyPRDir);
			if (dir)
			{
				dir->ioVRefNum = fileSpec->vRefNum;
				dir->ioDirID = theDirID;
				dir->currentEntryName = NULL;
				dir->ioFDirIndex = 1;
			}
		}
	}
	
	return dir;

}

static char * My_ReadDir(MyPRDir *mdDir, PRIntn flags)
{
	OSErr			err;
	CInfoPBRec		pb;
	char			*returnedCStr;
	Str255			pascalName = "\p";
	PRBool			foundEntry;
	
	do
	{
		// Release the last name read
		if (mdDir->currentEntryName)
		{
			PR_DELETE(mdDir->currentEntryName);
			mdDir->currentEntryName = NULL;
		}
			
		// We�ve got all the info we need, just get info about this guy.
		pb.hFileInfo.ioNamePtr = pascalName;
		pb.hFileInfo.ioVRefNum = mdDir->ioVRefNum;
		pb.hFileInfo.ioFDirIndex = mdDir->ioFDirIndex;
		pb.hFileInfo.ioDirID = mdDir->ioDirID;
		err = ::PBGetCatInfoSync(&pb);
		if (err != noErr)
			goto ErrorExit;
		
		// Convert the Pascal string to a C string (actual allocation occurs in CStrFromPStr)
		unsigned long len = pascalName[0];
		returnedCStr = (char *)PR_MALLOC(len + 1);
		if (returnedCStr)
		{
			::BlockMoveData(&pascalName[1], returnedCStr, len);
			returnedCStr[len] = NULL;
		}
		
		mdDir->currentEntryName = returnedCStr;
		mdDir->ioFDirIndex++;
		
		// If it is not a hidden file and the flags did not specify skipping, we are done.
		if ((flags & PR_SKIP_HIDDEN) && (pb.hFileInfo.ioFlFndrInfo.fdFlags & fInvisible))
			foundEntry = PR_FALSE;
		else
			foundEntry = PR_TRUE;	
		
	} while (!foundEntry);
	
	return (mdDir->currentEntryName);

ErrorExit:
	return NULL;
}

static void My_CloseDir(MyPRDir *mdDir)
{
	if (!mdDir)
		return;		// Not much we can do with a null mdDir
	
	// If we'd allocated an entry name then delete it
	if (mdDir->currentEntryName)
		PR_DELETE(mdDir->currentEntryName);
	
	// delete the directory info struct as well
	PR_DELETE(mdDir);
}

// The R**co FSSpec resolver -
//	it slices, it dices, it juliannes fries and it even creates FSSpecs out of whatever you feed it
// This function will take a path and a starting FSSpec and generate a FSSpec to represent
// the target of the two.  If the intial FSSpec is null the path alone will be resolved
static OSErr ResolvePathAndSpec(const char * filePath, FSSpec *inSpec, PRBool createDirs, FSSpec *outSpec)
{
	OSErr	err = noErr;
	size_t	inLength = strlen(filePath);
	Boolean isRelative = (inSpec && (strchr(filePath, ':') == 0 || *filePath == ':'));
	
	if (isRelative && inSpec)
	{
		outSpec->vRefNum = inSpec->vRefNum;
		outSpec->parID = inSpec->parID;
	}
	else
	{
		outSpec->vRefNum = 0;
		outSpec->parID = 0;
	}
	
	// Try making an FSSpec from the path
	if (inLength < 255)
	{
		Str255 pascalpath;
		myPLstrcpy(pascalpath, filePath);
		err = ::FSMakeFSSpec(outSpec->vRefNum, outSpec->parID, pascalpath, outSpec);
	}
	else if (!isRelative)
	{
		err = ::FSpLocationFromFullPath(inLength, filePath, outSpec);
	}
	else
	{	// If the path is relative and >255 characters we need to manually walk the
		// path appending each node to the initial FSSpec so to reach that code we
		// set the err to bdNamErr and fall into the code below
		err = bdNamErr;
	}
	
	// If we successfully created a spec then leave
	if (err == noErr)
		return err;
	
	// We get here when the directory hierarchy needs to be created or we're resolving
	// a relative path >255 characters long
	if (err == dirNFErr || err == bdNamErr)
	{
		const char* path = filePath;
		outSpec->vRefNum = 0;
		outSpec->parID = 0;

		do
		{
			// Locate the colon that terminates the node.
			// But if we've a partial path (starting with a colon), find the second one.
			const char* nextColon = strchr(path + (*path == ':'), ':');
			// Well, if there are no more colons, point to the end of the string.
			if (!nextColon)
				nextColon = path + strlen(path);

			// Make a pascal string out of this node.  Include initial
			// and final colon, if any!
			Str255 ppath;
			myPLstrncpy(ppath, path, nextColon - path + 1);
			
			// Use this string as a relative path using the directory created
			// on the previous round (or directory 0,0 on the first round).
			err = ::FSMakeFSSpec(outSpec->vRefNum, outSpec->parID, ppath, outSpec);

			// If this was the leaf node, then we are done.
			if (!*nextColon)
				break;

			// Since there's more to go, we have to get the directory ID, which becomes
			// the parID for the next round.
			if (err == noErr)
			{
				// The directory (or perhaps a file) exists. Find its dirID.
				long dirID;
				Boolean isDirectory;
				err = ::FSpGetDirectoryID(outSpec, &dirID, &isDirectory);
				if (!isDirectory)
					err = dupFNErr; // oops! a file exists with that name.
				if (err != noErr)
					break;			// bail if we've got an error
				outSpec->parID = dirID;
			}
			else if ((err == fnfErr) && createDirs)
			{
				// If we got "file not found" and we're allowed to create directories 
				// then we need to create aone
				err = ::FSpDirCreate(outSpec, smCurrentScript, &outSpec->parID);
				// For some reason, this usually returns fnfErr, even though it works.
				if (err == fnfErr)
					err = noErr;
			}
			if (err != noErr)
				break;
			path = nextColon; // next round
		} while (true);
	}
	
	return err;
}

#pragma mark -
#pragma mark [nsDirEnumerator]
class nsDirEnumerator : public nsISimpleEnumerator
{
	public:

		NS_DECL_ISUPPORTS

		nsDirEnumerator() : mDir(nsnull) 
		{
			NS_INIT_REFCNT();
		}

		nsresult Init(nsILocalFile* parent) 
		{
			FSSpec fileSpec;
			fileSpec.vRefNum = 0;
			fileSpec.parID = 0;
			
			nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface(parent);
			if (localFileMac) {
				localFileMac->GetResolvedFSSpec(&fileSpec);
			}
			
			// See if we managed to get a FSSpec
			if (!fileSpec.vRefNum && !fileSpec.parID)
			{
				return NS_ERROR_FAILURE;
			}
			
			mDir = My_OpenDir(&fileSpec);
			if (mDir == nsnull)	   // not a directory?
				return NS_ERROR_FAILURE;
		
			mParent			 = parent;	  
			return NS_OK;
		}

		NS_IMETHOD HasMoreElements(PRBool *result) 
		{
			nsresult rv = NS_OK;
			if (mNext == nsnull && mDir) 
			{
				char* name = My_ReadDir(mDir, PR_SKIP_BOTH);
				if (name == nsnull) 
				{
					// end of dir entries
					My_CloseDir(mDir);
					mDir = nsnull;
					*result = PR_FALSE;
					return NS_OK;
				}
				
				// Make a new nsILocalFile for the new element
				nsCOMPtr<nsILocalFile> file;
				rv =  NS_NewLocalFile("dummy:path", getter_AddRefs(file));
				if (NS_FAILED(rv)) 
					return rv;
				
				// Init with the FSSpec for the current dir
				FSSpec	tempSpec;
				tempSpec.vRefNum = mDir->ioVRefNum;
				tempSpec.parID = mDir->ioDirID;
				tempSpec.name[0] = 0;
				nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface(file);
				if (localFileMac) {
					localFileMac->InitWithFSSpec(&tempSpec);
				}
				
				// Now set the leaf name of the new nsILocalFile to the new element
				rv = file->SetLeafName(name);
				if (NS_FAILED(rv)) 
					return rv;
			
				mNext = do_QueryInterface(file);
			}
			*result = mNext != nsnull;
			return NS_OK;
		}

		NS_IMETHOD GetNext(nsISupports **result) 
		{
			nsresult rv = NS_OK;
			PRBool hasMore;
			rv = HasMoreElements(&hasMore);
			if (NS_FAILED(rv)) return rv;

			*result = mNext;		// might return nsnull
			NS_IF_ADDREF(*result);
			
			mNext = null_nsCOMPtr();
			return NS_OK;
		}

		virtual ~nsDirEnumerator() 
		{
			if (mDir) 
			{
				My_CloseDir(mDir);
			}
		}

	protected:
		MyPRDir*				mDir;
		nsCOMPtr<nsILocalFile>	mParent;
		nsCOMPtr<nsILocalFile>	mNext;
};

NS_IMPL_ISUPPORTS(nsDirEnumerator, NS_GET_IID(nsISimpleEnumerator));

#pragma mark -
#pragma mark [CTOR/DTOR]
nsLocalFile::nsLocalFile()
:	mInitType(eNotInitialized)
,	mLastResolveFlag(PR_FALSE)
,	mHaveFileInfo(PR_FALSE)
{
	NS_INIT_REFCNT();

	MakeDirty();	
	ClearFSSpec(mSpec);
}

nsLocalFile::~nsLocalFile()
{
}

#pragma mark -
#pragma mark [nsISupports interface implementation]
NS_IMPL_THREADSAFE_ISUPPORTS3(nsLocalFile, nsILocalFileMac, nsILocalFile, nsIFile)

NS_METHOD
nsLocalFile::nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
	NS_ENSURE_ARG_POINTER(aInstancePtr);
	NS_ENSURE_NO_AGGREGATION(outer);

	nsLocalFile* inst = new nsLocalFile();
	if (inst == NULL)
		return NS_ERROR_OUT_OF_MEMORY;
	
	nsresult rv = inst->QueryInterface(aIID, aInstancePtr);
	if (NS_FAILED(rv))
	{
		delete inst;
		return rv;
	}
	return NS_OK;
}

// This function resets any cached information about the file.
void
nsLocalFile::MakeDirty()
{
	mStatDirty = PR_TRUE;
	
	ClearFSSpec(mResolvedSpec);
	ClearFSSpec(mTargetSpec);
	
	mResolvedWasAlias = false;
	mResolvedWasFolder = false;
	
	mHaveFileInfo = PR_FALSE;
}


NS_IMETHODIMP
nsLocalFile::ResolveAndStat(PRBool resolveTerminal)
{
	OSErr	err = noErr;
	char	*filePath;
	
	if (!mStatDirty && resolveTerminal == mLastResolveFlag)
	{
		return NS_OK;
	}
	
	mLastResolveFlag = resolveTerminal;
	
	// See if we have been initialized with a spec
	switch (mInitType)
	{
		case eInitWithPath:
		{
			filePath = (char *)nsAllocator::Clone(mWorkingPath, strlen(mWorkingPath)+1);
			err = ResolvePathAndSpec(filePath, nsnull, PR_FALSE, &mResolvedSpec);
			nsAllocator::Free(filePath);
			break;
		}
		
		case eInitWithFSSpec:
		{
			if (strlen(mAppendedPath))
			{	// We've got an FSSpec and an appended path so pass 'em both to ResolvePathAndSpec
				filePath = (char *)nsAllocator::Clone(mAppendedPath, strlen(mAppendedPath)+1);
				err = ResolvePathAndSpec(filePath, &mSpec, PR_FALSE, &mResolvedSpec);
				nsAllocator::Free(filePath);
			}
			else
			{
				err = ::FSMakeFSSpec(mSpec.vRefNum, mSpec.parID, mSpec.name, &mResolvedSpec);
			}
			break;
		}
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			break;
	}
	
	if (resolveTerminal && err == noErr)
	{
		// Resolve the alias to the original file.
		FSSpec	spec = mResolvedSpec;
		err = ::ResolveAliasFile(&spec, TRUE, &mResolvedWasFolder, &mResolvedWasAlias);
		if (err != noErr)
			return MacErrorMapper(err);
		else
			mTargetSpec = spec;
	}		
		
	
	if (err == noErr)
	{
		mStatDirty = PR_FALSE;
	}
	
	return (MacErrorMapper(err));
}
	

NS_IMETHODIMP  
nsLocalFile::Clone(nsIFile **file)
{
	NS_ENSURE_ARG(file);
	*file = nsnull;
	
	  // Create the new nsLocalFile
	nsCOMPtr<nsILocalFile> localFile = new nsLocalFile();
	if (localFile == NULL)
	  return NS_ERROR_OUT_OF_MEMORY;
	
	// See if it's a nsLocalFileMac (shouldn't be possible for it not to be)
	nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface(localFile);
	if (localFileMac == NULL)
		  return NS_ERROR_NO_INTERFACE;
	
	nsresult rv = NS_OK;
	
	// Now we figure out how we were initialized in order to determine how to
	// initialize the clone
	nsLocalFileMacInitType initializedAs;
	localFileMac->GetInitType(&initializedAs);
	switch (mInitType)
	{
		case eInitWithPath:
			// The simple case
			char *path;
			GetPath(&path);
			rv = localFile->InitWithPath(path);
			break;
		
		case eInitWithFSSpec:
			// Slightly more complex as we need to set the FSSpec and any appended
			// path info
			// ????? Should we just set this to the resolved spec ?????
			localFileMac->InitWithFSSpec(&mSpec);
			// Now set any appended path info
			char *appendedPath;
			GetAppendedPath(&appendedPath);
			rv = localFileMac->SetAppendedPath(appendedPath);
			nsAllocator::Free(appendedPath);
			break;
			
		default:
			NS_NOTREACHED("we really shouldn't get here");
			break;
	}
	 
	if (NS_FAILED(rv))
	  return rv;
	   
	*file = localFile;
	NS_ADDREF(*file);
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::InitWithPath(const char *filePath)
{
	MakeDirty();
	NS_ENSURE_ARG(filePath);
	
	// Make sure there's a colon in the path and it is not the first character
	// so we know we got a full path, not a partial one
	if (strchr(filePath, ':') == 0 || *filePath == ':')
		return NS_ERROR_FILE_UNRECOGNIZED_PATH;

	// Just save the specified file path since we can't actually do anything
	// about turniung it into an FSSpec until the Create() method is called
	mWorkingPath.Assign(filePath);
	
	mInitType = eInitWithPath;
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::OpenNSPRFileDesc(PRInt32 flags, PRInt32 mode, PRFileDesc **_retval)
{
// Macintosh doesn't really have mode bits, just drop them
#pragma unused (mode)

	NS_ENSURE_ARG(_retval);
	
	FSSpec 	spec;
	OSErr err = noErr;
	
	nsresult rv = ResolveAndStat(PR_TRUE);
	if ((rv == NS_ERROR_FILE_NOT_FOUND) && (flags & PR_CREATE_FILE))
		spec = mResolvedSpec;
	else
		spec = mTargetSpec;

	// Resolve the alias to the original file.
	Boolean targetIsFolder;	  
	Boolean wasAliased;	  
	err = ::ResolveAliasFile(&spec, TRUE, &targetIsFolder, &wasAliased);
	
	// If we're going to create a file it's ok if it doesn't exist
	if (err == fnfErr && (flags & PR_CREATE_FILE))
		err = noErr;
	
	if (err != noErr)
		return MacErrorMapper(err);
	
	if (flags & PR_CREATE_FILE)
		err = ::FSpCreate(&spec, 'MOSS', 'TEXT', 0);
	   
	/* If opening with the PR_EXCL flag the existence of the file prior to opening is an error */
	if ((flags & PR_EXCL) &&  (err == dupFNErr))
		return MacErrorMapper(err);

	if (err == dupFNErr)
		err = noErr;
	if (err != noErr)
		return MacErrorMapper(err);
	
	SInt8 perm;
	if (flags & PR_RDWR)
	   perm = fsRdWrPerm;
	else if (flags & PR_WRONLY)
	   perm = fsWrPerm;
	else
	   perm = fsRdPerm;

	short refnum;
	err = ::FSpOpenDF(&spec, perm, &refnum);

	if (err == noErr && (flags & PR_TRUNCATE))
		err = ::SetEOF(refnum, 0);
	if (err == noErr && (flags & PR_APPEND))
		err = ::SetFPos(refnum, fsFromLEOF, 0);
	if (err != noErr)
		return MacErrorMapper(err);

	if ((*_retval = PR_ImportFile(refnum)) == 0)
		return NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_FILES,(PR_GetError() & 0xFFFF));
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::OpenANSIFileDesc(const char *mode, FILE * *_retval)
{
	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
		return rv; 
   
   
   // Resolve the alias to the original file.
	FSSpec	spec = mTargetSpec;
	Boolean targetIsFolder;	  
	Boolean wasAliased;	  
	OSErr err = ::ResolveAliasFile(&spec, TRUE, &targetIsFolder, &wasAliased);
	if (err != noErr)
		return MacErrorMapper(err);
		
		
	*_retval = FSp_fopen(&spec, mode);
	
	if (*_retval)
		return NS_OK;

	return NS_ERROR_FAILURE;
}


NS_IMETHODIMP  
nsLocalFile::Create(PRUint32 type, PRUint32 attributes)
{ 
	OSErr	err;
	char	*filePath;

	if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
		return NS_ERROR_FILE_UNKNOWN_TYPE;

	switch (mInitType)
	{
		case eInitWithPath:
		{
			filePath = (char *)nsAllocator::Clone(mWorkingPath, strlen(mWorkingPath)+1);
			err = ResolvePathAndSpec(filePath, nsnull, PR_FALSE, &mResolvedSpec);
			nsAllocator::Free(filePath);
			break;
		}
		
		case eInitWithFSSpec:
		{
			if (strlen(mAppendedPath))
			{	// We've got an FSSpec and an appended path so pass 'em both to ResolvePathAndSpec
				filePath = (char *)nsAllocator::Clone(mAppendedPath, strlen(mAppendedPath)+1);
				err = ResolvePathAndSpec(filePath, &mSpec, PR_FALSE, &mResolvedSpec);
				nsAllocator::Free(filePath);
			}
			else
			{
				err = ::FSMakeFSSpec(mSpec.vRefNum, mSpec.parID, mSpec.name, &mResolvedSpec);
			}
			break;
		}
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			break;
	}
	
	if (err != noErr && err != fnfErr)
		return (MacErrorMapper(err));

	switch (type)
	{
		case NORMAL_FILE_TYPE:
			// We really should use some sort of meaningful file type/creator but where
			// do we get the info from?
			err = ::FSpCreate(&mResolvedSpec, '????', '????', smCurrentScript);
			return (MacErrorMapper(err));
			break;

		case DIRECTORY_TYPE:
			err = ::FSpDirCreate(&mResolvedSpec, smCurrentScript, &mResolvedSpec.parID);
			// For some reason, this usually returns fnfErr, even though it works.
			if (err == fnfErr)
				err = noErr;
			return (MacErrorMapper(err));
			break;
		
		default:
			// For now just fall out of the switch into the default return NS_ERROR_FILE_UNKNOWN_TYPE
			break;

	}

	return NS_ERROR_FILE_UNKNOWN_TYPE;
}
	
NS_IMETHODIMP  
nsLocalFile::Append(const char *node)
{
	if ( (node == nsnull) )
		return NS_ERROR_FILE_UNRECOGNIZED_PATH;

	MakeDirty();

	// Yee Hah!	 We only get a single node at a time so just append it
	// to either the mWorkingPath or mAppendedPath, after adding the ':'
	// directory delimeter, depending on how we were initialized
	switch (mInitType)
	{
		case eInitWithPath:
			if ( (mWorkingPath.Last())!=':' )
				mWorkingPath.Append(":");
			mWorkingPath.Append(node);
			break;
		
		case eInitWithFSSpec:
			mAppendedPath.Append(":");
			mAppendedPath.Append(node);
			break;
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			break;
	}
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetLeafName(char * *aLeafName)
{
	NS_ENSURE_ARG_POINTER(aLeafName);

	switch (mInitType)
	{
		case eInitWithPath:
			const char* temp = mWorkingPath.GetBuffer();
			if (temp == nsnull)
				return NS_ERROR_FILE_UNRECOGNIZED_PATH;

			const char* leaf = strrchr(temp, ':');
			
			// if the working path is just a node without any directory delimeters.
			if (leaf == nsnull)
				leaf = temp;
			else
				leaf++;

			*aLeafName = (char*) nsAllocator::Clone(leaf, strlen(leaf)+1);
			break;
		
		case eInitWithFSSpec:
			// See if we've had a path appended
			if (mAppendedPath.Length())
			{
				const char* temp = mAppendedPath.GetBuffer();
				if (temp == nsnull)
					return NS_ERROR_FILE_UNRECOGNIZED_PATH;

				const char* leaf = strrchr(temp, ':');
				
				// if the working path is just a node without any directory delimeters.
				if (leaf == nsnull)
					leaf = temp;
				else
					leaf++;

				*aLeafName = (char*) nsAllocator::Clone(leaf, strlen(leaf)+1);
			}
			else
			{
				// We don't have an appended path so grab the leaf name from the FSSpec
				// Convert the Pascal string to a C string				
				PRInt32 len = mSpec.name[0];
				char* leafName = (char *)nsAllocator::Alloc(len + 1);
				if (!leafName) return NS_ERROR_OUT_OF_MEMORY;				
				::BlockMoveData(&mSpec.name[1], leafName, len);
				leafName[len] = '\0';
				*aLeafName = leafName;
			}
			break;
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			break;
	}

	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::SetLeafName(const char * aLeafName)
{
	NS_ENSURE_ARG_POINTER(aLeafName);
	
	switch (mInitType)
	{
		case eInitWithPath:
			PRInt32 offset = mWorkingPath.RFindChar(':');
			if (offset)
			{
				mWorkingPath.Truncate(offset + 1);
			}
			mWorkingPath.Append(aLeafName);
			break;
		
		case eInitWithFSSpec:
			// See if we've had a path appended
			if (mAppendedPath.Length())
			{	// Lop off the end of the appended path and replace it with the new leaf name
				PRInt32 offset = mAppendedPath.RFindChar(':');
				if (offset)
				{
					mAppendedPath.Truncate(offset + 1);
				}
				mAppendedPath.Append(aLeafName);
			}
			else
			{
				// We don't have an appended path so directly modify the FSSpec
				myPLstrcpy(mSpec.name, aLeafName);
			}
			break;
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			break;
	}
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetPath(char **_retval)
{
	NS_ENSURE_ARG_POINTER(_retval);
	
	switch (mInitType)
	{
		case eInitWithPath:
			*_retval = (char*) nsAllocator::Clone(mWorkingPath, strlen(mWorkingPath)+1);
			if (!*_retval)
				return NS_ERROR_OUT_OF_MEMORY;
			break;
		
		case eInitWithFSSpec:
		{	// Now would be a good time to call the code that makes an FSSpec into a path
			short	fullPathLen;
			Handle	fullPathHandle;
			ResolveAndStat(PR_TRUE);
			(void)::FSpGetFullPath(&mResolvedSpec, &fullPathLen, &fullPathHandle);
			if (!fullPathHandle)
				return NS_ERROR_OUT_OF_MEMORY;
			
			char* fullPath = (char *)nsAllocator::Alloc(fullPathLen + 1);
			if (!fullPath)
				return NS_ERROR_OUT_OF_MEMORY;
			
			::HLock(fullPathHandle);
			nsCRT::memcpy(fullPath, *fullPathHandle, fullPathLen);			
			fullPath[fullPathLen] = '\0';
			
			*_retval = fullPath;
			::DisposeHandle(fullPathHandle);
			break;
		}
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			break;
	}
	
	// For cross platform reasons we need to make sure that even if we have a path to a
	// directory we don't return the trailing colon.  This used to break the component
	// manager (Bugzilla bug #26102)
	PRUint32 lastChar = strlen(*_retval) - 1;
	if ((*_retval)[lastChar] == ':')
		(*_retval)[lastChar] = '\0';

	return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::CopyTo(nsIFile *newParentDir, const char *newName)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::CopyToFollowingLinks(nsIFile *newParentDir, const char *newName)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::MoveTo(nsIFile *newParentDir, const char *newName)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::Spawn(const char **args, PRUint32 count)
{
	PRBool isFile;
	nsresult rv = IsFile(&isFile);

	if (NS_FAILED(rv))
		return rv;


	return NS_ERROR_FILE_EXECUTION_FAILED;
}

NS_IMETHODIMP  
nsLocalFile::Load(PRLibrary * *_retval)
{
	PRBool isFile;
	nsresult rv = IsFile(&isFile);

	if (NS_FAILED(rv))
		return rv;
	
	if (! isFile)
		return NS_ERROR_FILE_IS_DIRECTORY;
	
	// Use the new PR_LoadLibraryWithFlags which allows us to use a FSSpec
	PRLibSpec libSpec;
	libSpec.type = PR_LibSpec_MacIndexedFragment;
	libSpec.value.mac_indexed_fragment.fsspec = &mTargetSpec;
	libSpec.value.mac_indexed_fragment.index = 0;
	*_retval =	PR_LoadLibraryWithFlags(libSpec, 0);
	
	if (*_retval)
		return NS_OK;

	return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP  
nsLocalFile::Delete(PRBool recursive)
{
	MakeDirty();
	
	PRBool isDir;
	
	nsresult rv = IsDirectory(&isDir);
	if (NS_FAILED(rv))
		return rv;

	const char *filePath = mResolvedPath.GetBuffer();

	if (isDir)
	{
		if (recursive)
		{
			nsDirEnumerator* dirEnum = new nsDirEnumerator();
			if (dirEnum)
				return NS_ERROR_OUT_OF_MEMORY;
		
			rv = dirEnum->Init(this);

			nsCOMPtr<nsISimpleEnumerator> dirIterator = do_QueryInterface(dirEnum);
		
			PRBool more;
			dirIterator->HasMoreElements(&more);
			while (more)
			{
				nsCOMPtr<nsISupports> item;
				nsCOMPtr<nsIFile> file;
				dirIterator->GetNext(getter_AddRefs(item));
				file = do_QueryInterface(item);
	
				file->Delete(recursive);
				
				dirIterator->HasMoreElements(&more);
			}
		}
		//rmdir(filePath);	// todo: save return value?
	}
	else
	{
		//remove(filePath); // todo: save return value?
	}
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetLastModificationDate(PRInt64 *aLastModificationDate)
{
	NS_ENSURE_ARG(aLastModificationDate);
	nsresult rv = ResolveAndStat( PR_TRUE );
	if ( NS_FAILED( rv ) )
		return rv;
		
	CInfoPBRec pb;
	PRUint32 timestamp;
	if (GetTargetSpecCatInfo(pb) == noErr)
		timestamp = ((DirInfo*)&pb)->ioDrMdDat; // The mod date is in the same spot for files and dirs.
	else
		timestamp = 0;
	
  return ConvertMacTimeToUnixTime( aLastModificationDate, timestamp );
}

NS_IMETHODIMP  
nsLocalFile::SetLastModificationDate(PRInt64 aLastModificationDate)
{
	MakeDirty();
	nsresult rv = ResolveAndStat( PR_TRUE );
	if ( NS_FAILED(rv) )
		return rv;
	
	CInfoPBRec pb;
	PRUint32 macTime = 0;
	OSErr err = noErr;
	PRBool bIsDir = PR_FALSE;
	
	ConvertUnixTimeToMacTime(aLastModificationDate, &macTime);
	rv = IsDirectory(&bIsDir);
	if ( NS_FAILED(rv) )
		return rv;
		
	err = GetTargetSpecCatInfo(pb);
	if (err == noErr)
	{
		if (bIsDir)
		{
			pb.dirInfo.ioDrMdDat = macTime; 
			pb.dirInfo.ioDrParID = mTargetSpec.parID;
		}
		else
		{
			pb.hFileInfo.ioFlMdDat = macTime;
			pb.hFileInfo.ioDirID = mTargetSpec.parID;
		}	
			
		err = ::PBSetCatInfoSync(&pb);
		if (err != noErr)
			return MacErrorMapper(err);
	}
	
	return MacErrorMapper(err);
}

NS_IMETHODIMP  
nsLocalFile::GetLastModificationDateOfLink(PRInt64 *aLastModificationDate)
{
	NS_ENSURE_ARG(aLastModificationDate);
	
	aLastModificationDate->hi = 0;
	aLastModificationDate->lo = 0;

	NS_ASSERTION(0, "Not implemented");
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::SetLastModificationDateOfLink(PRInt64 aLastModificationDate)
{
	MakeDirty();

	NS_ASSERTION(0, "Not implemented");
	return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP  
nsLocalFile::GetFileSize(PRInt64 *aFileSize)
{
	NS_ENSURE_ARG(aFileSize);
	
	aFileSize->hi = 0;
	aFileSize->lo = 0;

	ResolveAndStat(PR_TRUE);
	
	long dataSize = 0;
	long resSize = 0;
	
	OSErr err = FSpGetFileSize(&mTargetSpec, &dataSize, &resSize);
							   
	if (err != noErr && !mResolvedWasFolder)
		return MacErrorMapper(err);
	
	// For now we've only got 32 bits of file size info
	PRInt64 dataInt64 = LL_Zero();
	PRInt64 resInt64 = LL_Zero();
	
	// WARNING!!!!!!
	// 
	// For now we do NOT add the data and resource fork sizes as there are several
	// assumptions in the code (notably in form submit) that only the data fork is
	// used. 
	// LL_I2L(resInt64, resSize);
	
	LL_I2L(dataInt64, dataSize);
	
	LL_ADD((*aFileSize), dataInt64, resInt64);
	
	return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::SetFileSize(PRInt64 aFileSize)
{
	PRBool	exists;
	
	Exists(&exists);
	if (exists)
	{
		short	refNum;
		OSErr	err;
		PRInt32 aNewLength;
		
		LL_L2I(aNewLength, aFileSize);
		
		// Need to open the file to set the size
		if (::FSpOpenDF(&mTargetSpec, fsWrPerm, &refNum) != noErr)
			return NS_ERROR_FILE_ACCESS_DENIED;

		err = ::SetEOF(refNum, aNewLength);
			
		// Close the file unless we got an error that it was already closed
		if (err != fnOpnErr)
			(void)::FSClose(refNum);
			
		if (err != noErr)
			return MacErrorMapper(err);
	}
	else
	{
		return NS_ERROR_FILE_NOT_FOUND;
	}
		
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetFileSizeOfLink(PRInt64 *aFileSize)
{
	NS_ENSURE_ARG(aFileSize);
	
	aFileSize->hi = 0;
	aFileSize->lo = 0;

	ResolveAndStat(PR_TRUE);
	
	long dataSize = 0;
	long resSize = 0;
	
	OSErr err = FSpGetFileSize(&mResolvedSpec, &dataSize, &resSize);
							   
	if (err != noErr && !mResolvedWasFolder)
		return MacErrorMapper(err);
	
	// For now we've only got 32 bits of file size info
	PRInt64 dataInt64 = LL_Zero();
	PRInt64 resInt64 = LL_Zero();
	
	// WARNING!!!!!!
	// 
	// For now we do NOT add the data and resource fork sizes as there are several
	// assumptions in the code (notably in form submit) that only the data fork is
	// used. 
	// LL_I2L(resInt64, resSize);
	
	LL_I2L(dataInt64, dataSize);
	
	LL_ADD((*aFileSize), dataInt64, resInt64);
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetDiskSpaceAvailable(PRInt64 *aDiskSpaceAvailable)
{
	NS_ENSURE_ARG(aDiskSpaceAvailable);
		
	PRInt64 space64Bits;

	LL_I2L(space64Bits , LONG_MAX);

	XVolumeParam	pb;
	pb.ioCompletion = nsnull;
	pb.ioVolIndex = 0;
	pb.ioNamePtr = nsnull;
	pb.ioVRefNum = mResolvedSpec.vRefNum;
	
	// we should check if this call is available
	OSErr err = ::PBXGetVolInfoSync(&pb);
	
	if (err == noErr)
	{
		space64Bits.lo = pb.ioVFreeBytes.lo;
		space64Bits.hi = pb.ioVFreeBytes.hi;
	}
		
	*aDiskSpaceAvailable = space64Bits;

	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetParent(nsIFile * *aParent)
{
	NS_ENSURE_ARG_POINTER(aParent);
  nsresult rv = NS_OK;
  
	switch (mInitType)
	{
		case eInitWithPath:
		{	// The simple case when we have a full path to something
			nsCString parentPath = mWorkingPath;

			PRInt32 offset = parentPath.RFindChar(':');
			if (offset == -1)
				return NS_ERROR_FILE_UNRECOGNIZED_PATH;

			parentPath.Truncate(offset);

			nsCOMPtr<nsILocalFile> localFile;
			rv =  NS_NewLocalFile(parentPath.GetBuffer(), getter_AddRefs(localFile));
			if (NS_SUCCEEDED(rv) && localFile)
			{
				rv = localFile->QueryInterface(NS_GET_IID(nsIFile), (void**)aParent);
			}
			break;
		}
		
		case eInitWithFSSpec:
		{
			rv = ResolveAndStat(PR_TRUE);
			if (NS_FAILED(rv)) return rv;
 
			FSSpec      parentFolderSpec;		  
			CInfoPBRec 	pBlock = {0};

			parentFolderSpec.name[0] = 0;
			
			pBlock.dirInfo.ioVRefNum = mResolvedSpec.vRefNum;
			pBlock.dirInfo.ioDrDirID = mResolvedSpec.parID;
			pBlock.dirInfo.ioNamePtr = (StringPtr)parentFolderSpec.name;
			pBlock.dirInfo.ioFDirIndex = -1;		//get info on parID
			OSErr err = PBGetCatInfoSync(&pBlock);
			if (err != noErr) return MacErrorMapper(err);
			
			parentFolderSpec.vRefNum = mResolvedSpec.vRefNum;
			parentFolderSpec.parID = pBlock.dirInfo.ioDrParID;

			nsCOMPtr<nsILocalFile> file;
			rv =  NS_NewLocalFile("dummy:path", getter_AddRefs(file));
			if (NS_FAILED(rv)) 
				return rv;
			
			// Init with the FSSpec for the current dir
			nsCOMPtr<nsILocalFileMac> localFileMac = do_QueryInterface(file, &rv);
			if (localFileMac) {
				localFileMac->InitWithFSSpec(&parentFolderSpec);
				rv = localFileMac->QueryInterface(NS_GET_IID(nsIFile), (void**)aParent);
			}
      break;
		}
			
		default:
			// !!!!! Danger Will Robinson !!!!!
			// we really shouldn't get here
			NS_NOTREACHED("Unknown nsLocalFileMac init type");
			rv = NS_ERROR_FAILURE;
			break;
	}

	return rv;
}


NS_IMETHODIMP  
nsLocalFile::Exists(PRBool *_retval)
{
	NS_ENSURE_ARG(_retval);
	*_retval = PR_FALSE;		// Assume failure
	nsresult rv = ResolveAndStat(PR_TRUE);
	if ( rv == NS_ERROR_FILE_NOT_FOUND )
		return NS_OK;
	
	// XXX maybe Exists() should check the mResolvedSpec?
	FSSpec temp;
	if (::FSMakeFSSpec(mTargetSpec.vRefNum, mTargetSpec.parID, mTargetSpec.name, &temp) == noErr)
		*_retval = PR_TRUE;

	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsWritable(PRBool *outIsWritable)
{
	NS_ENSURE_ARG(outIsWritable);
	*outIsWritable = PR_TRUE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	
	CInfoPBRec	fileInfo;
	OSErr	err = GetTargetSpecCatInfo(fileInfo);
	if (err != noErr)
		return MacErrorMapper(err);
	
	// bit 0 in ioFlAttrib is the lock bit (apparently)
	*outIsWritable = !(fileInfo.hFileInfo.ioFlAttrib & 0x01);
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsReadable(PRBool *_retval)
{
	NS_ENSURE_ARG(_retval);

	// is it ever not readable on Mac?
	*_retval = PR_TRUE;
	return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::IsExecutable(PRBool *outIsExecutable)
{
	NS_ENSURE_ARG(outIsExecutable);
	*outIsExecutable = PR_FALSE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;

	OSType fileType, fileCreator;
	rv = GetFileTypeAndCreator(&fileType, &fileCreator);
	if (NS_FAILED(rv)) return rv;
	
	*outIsExecutable = (fileType == 'APPL' || fileType == 'appe' || fileType == 'FNDR');
	return NS_OK;
}


NS_IMETHODIMP  
nsLocalFile::IsDirectory(PRBool *outIsDir)
{
	NS_ENSURE_ARG(outIsDir);
	*outIsDir = PR_FALSE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	
	CInfoPBRec	fileInfo;
	OSErr	err = GetTargetSpecCatInfo(fileInfo);
	if (err != noErr)
		return MacErrorMapper(err);

	*outIsDir = (fileInfo.hFileInfo.ioFlAttrib & ioDirMask) != 0;
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsFile(PRBool *outIsFile)
{
	NS_ENSURE_ARG(outIsFile);
	*outIsFile = PR_FALSE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	
	if (NS_FAILED(rv))
		return rv;
	
	CInfoPBRec	fileInfo;
	OSErr	err = GetTargetSpecCatInfo(fileInfo);
	if (err != noErr)
		return MacErrorMapper(err);

	*outIsFile = (fileInfo.hFileInfo.ioFlAttrib & ioDirMask) == 0;
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::IsHidden(PRBool *_retval)
{
	NS_ENSURE_ARG(_retval);
	*_retval = PR_FALSE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	
	return TestFinderFlag(kIsInvisible, _retval);
}

NS_IMETHODIMP
nsLocalFile::IsSymlink(PRBool *_retval)
{
	NS_ENSURE_ARG(_retval);
	*_retval = PR_FALSE;

	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	
	return TestFinderFlag(kIsAlias, _retval, PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, PRBool *_retval)
{
	NS_ENSURE_ARG(inFile);
	NS_ENSURE_ARG(_retval);
	*_retval = PR_FALSE;

	char* inFilePath;
	inFile->GetPath(&inFilePath);
	
	char* filePath;
	GetPath(&filePath);

	if (strcmp(inFilePath, filePath) == 0)
		*_retval = PR_TRUE;
	
	nsAllocator::Free(inFilePath);
	nsAllocator::Free(filePath);

	return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Contains(nsIFile *inFile, PRBool recur, PRBool *outContains)
{
  /* Note here that we make no attempt to deal with the problem
     of folder aliases. Doing a 'Contains' test and dealing with
     folder aliases is Hard. Think about it.
  */
	*outContains = PR_FALSE;

	PRBool isDir;
	nsresult rv = IsDirectory(&isDir);    // need to cache this
	if (NS_FAILED(rv)) return rv;
	if (!isDir) return NS_OK;   // must be a dir to contain someone

	nsCOMPtr<nsILocalFileMac> macFile(do_QueryInterface(inFile));
	if (!macFile) return NS_OK;     // trying to compare non-local with local file

	FSSpec  mySpec = mResolvedSpec;
	FSSpec  compareSpec;

	rv = macFile->GetResolvedFSSpec(&compareSpec);
	if (NS_FAILED(rv)) return rv;

	// if they are on different volumes, bail
	if (mResolvedSpec.vRefNum != compareSpec.vRefNum)
		return NS_OK;

	// if recur == true, test every parent, otherwise just test the first one
	// (yes, recur does not get set in this loop)
	OSErr	err = noErr;
	do
	{
		FSSpec	parentFolderSpec;
		err = GetParentFolderSpec(compareSpec, parentFolderSpec);
		if (err != noErr) break;				// we reached the top	

		if (IsEqualFSSpec(parentFolderSpec, mySpec))
		{
			*outContains = PR_TRUE;
			break;
		}
		
		compareSpec = parentFolderSpec;
	} while (recur);
	  
	return NS_OK;
}



NS_IMETHODIMP
nsLocalFile::GetTarget(char **_retval)
{	
	NS_ENSURE_ARG(_retval);
	*_retval = nsnull;
	
	PRBool symLink;
	
	nsresult rv = IsSymlink(&symLink);
	if (NS_FAILED(rv))
		return rv;

	if (!symLink)
	{
		return NS_ERROR_FILE_INVALID_PATH;
	}
		
	*_retval = (char*) nsAllocator::Clone( mResolvedPath, strlen(mResolvedPath)+1 );
	return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator * *entries)
{
	nsresult rv;
	
	*entries = nsnull;

	PRBool isDir;
	rv = IsDirectory(&isDir);
	if (NS_FAILED(rv)) 
		return rv;
	if (!isDir)
		return NS_ERROR_FILE_NOT_DIRECTORY;

	nsDirEnumerator* dirEnum = new nsDirEnumerator();
	if (dirEnum == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;
	NS_ADDREF(dirEnum);
	rv = dirEnum->Init(this);
	if (NS_FAILED(rv)) 
	{
		NS_RELEASE(dirEnum);
		return rv;
	}
	
	*entries = dirEnum;
	return NS_OK;
}


#pragma mark -

// a stack-based, exception safe class for an AEDesc

#pragma mark
class StAEDesc: public AEDesc
{
public:
					StAEDesc()
					{
						descriptorType = typeNull;
						dataHandle = nil;
					}
										
					~StAEDesc()
					{
						::AEDisposeDesc(this);
					}

					void Clear()
					{
						::AEDisposeDesc(this);
						descriptorType = typeNull;
						dataHandle = nil;
					}
										
private:
	// disallow copies and assigns
							StAEDesc(const StAEDesc& rhs);		// copy constructor
	StAEDesc& 	operator= (const StAEDesc&rhs);		// throws OSErrs

};

#pragma mark -
#pragma mark [Utility methods]


OSErr nsLocalFile::GetTargetSpecCatInfo(CInfoPBRec& outInfo) 
{
	OSErr	err = noErr;
	
	if (!mHaveFileInfo)
	{
		mTargetFileInfoRec.hFileInfo.ioCompletion = nsnull;
		mTargetFileInfoRec.hFileInfo.ioFDirIndex = 0; // use dirID and name
		mTargetFileInfoRec.hFileInfo.ioVRefNum = mTargetSpec.vRefNum;
		mTargetFileInfoRec.hFileInfo.ioDirID = mTargetSpec.parID;
		mTargetFileInfoRec.hFileInfo.ioNamePtr = mTargetSpec.name;

		err =  PBGetCatInfoSync(&mTargetFileInfoRec);
		mHaveFileInfo = (err == noErr);
	}
	
	outInfo = mTargetFileInfoRec;
	return err;
}


nsresult nsLocalFile::TestFinderFlag(PRUint16 flagMask, PRBool *outFlagSet, PRBool testTargetSpec /* = PR_TRUE */)
{
	CInfoPBRec	fileInfo;
	
	// we assume here that ResolveAndStat has already been called
	
	*outFlagSet = PR_FALSE;
	OSErr	err = noErr;
	
	if (testTargetSpec)
	{
		err = GetTargetSpecCatInfo(fileInfo);
	}
	else
	{
		// we don't cache this CInfoPBRec, so do it manually
		
		fileInfo.hFileInfo.ioCompletion = nsnull;
		fileInfo.hFileInfo.ioFDirIndex = 0; // use dirID and name
		fileInfo.hFileInfo.ioVRefNum = mResolvedSpec.vRefNum;
		fileInfo.hFileInfo.ioDirID = mResolvedSpec.parID;
		fileInfo.hFileInfo.ioNamePtr = mResolvedSpec.name;

		err = PBGetCatInfoSync(&fileInfo);
	}
	
	if (err != noErr)
		return MacErrorMapper(err);
	
	*outFlagSet = (fileInfo.hFileInfo.ioFlFndrInfo.fdFlags & flagMask) != 0;
	return NS_OK;
}


nsresult nsLocalFile::FindRunningAppBySignature (OSType aAppSig, FSSpec& outSpec, ProcessSerialNumber& outPsn)
{
	ProcessInfoRec 	info;
	FSSpec					tempFSSpec;
	OSErr 					err = noErr;
	
	outPsn.highLongOfPSN = 0;
	outPsn.lowLongOfPSN  = kNoProcess;

	while (PR_TRUE)
	{
		err = ::GetNextProcess(&outPsn);
		if (err == procNotFound) break;
		if (err != noErr) return NS_ERROR_FAILURE;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processName = nil;
		info.processAppSpec = &tempFSSpec;
		err = ::GetProcessInformation(&outPsn, &info);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		if (info.processSignature == aAppSig)
		{
			outSpec = tempFSSpec;
			return NS_OK;
		}
	}
	
	return NS_ERROR_FILE_NOT_FOUND;		// really process not found
}


nsresult nsLocalFile::FindRunningAppByFSSpec(const FSSpec& appSpec, ProcessSerialNumber& outPsn)
{
	ProcessInfoRec 	info;
	FSSpec					tempFSSpec;
	OSErr 					err = noErr;
	
	outPsn.highLongOfPSN = 0;
	outPsn.lowLongOfPSN  = kNoProcess;

	while (PR_TRUE)
	{
		err = ::GetNextProcess(&outPsn);
		if (err == procNotFound) break;		
		if (err != noErr) return NS_ERROR_FAILURE;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processName = nil;
		info.processAppSpec = &tempFSSpec;
		err = ::GetProcessInformation(&outPsn, &info);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		if (IsEqualFSSpec(appSpec, *info.processAppSpec))
		{
  			return NS_OK;
		}
	}
	
	return NS_ERROR_FILE_NOT_FOUND;		// really process not found
}


nsresult nsLocalFile::FindAppOnLocalVolumes(OSType sig, FSSpec &outSpec)
{
	OSErr	err;
	
	// get the system volume
	long		systemFolderDirID;
	short		sysVRefNum;
	err = FindFolder(kOnSystemDisk, kSystemFolderType, false, &sysVRefNum, &systemFolderDirID);
	if (err != noErr) return NS_ERROR_FAILURE;

	short	vRefNum = sysVRefNum;
	short	index = 0;
	
	while (true)
	{
		if (index == 0 || vRefNum != sysVRefNum)
		{
			// should we avoid AppleShare volumes?
			
			Boolean	hasDesktopDB;
			err = VolHasDesktopDB(vRefNum, &hasDesktopDB);
			if (err != noErr) return err;
			if (hasDesktopDB)
			{
				err = FindAppOnVolume(sig, vRefNum, &outSpec);
				if (err != afpItemNotFound) return err;
			}
		}
		index++;
		err = GetIndVolume(index, &vRefNum);
		if (err == nsvErr) return fnfErr;
		if (err != noErr) return err;
	}

	return NS_OK;
}

nsresult nsLocalFile::MyLaunchAppWithDoc(const FSSpec& appSpec, const FSSpec* aDocToLoad, PRBool aLaunchInBackground)
{
	ProcessSerialNumber thePSN = {0};
	StAEDesc 			target;
	StAEDesc 			docDesc;
	StAEDesc 			launchDesc;
	StAEDesc		 	docList;
	AppleEvent 		theEvent = {0, nil};
	AppleEvent 		theReply = {0, nil};
	OSErr 				err = noErr;
	Boolean 			autoParamValue = false;
	Boolean				running = false;
	nsresult			rv = NS_OK;
	
	rv = FindRunningAppByFSSpec(appSpec, thePSN);
	running = NS_SUCCEEDED(rv);
	
	err = AECreateDesc(typeProcessSerialNumber, &thePSN, sizeof(thePSN), &target); 
	if (err != noErr) return NS_ERROR_FAILURE;
	
	err = AECreateAppleEvent(kCoreEventClass, aDocToLoad ? kAEOpenDocuments : kAEOpenApplication, &target,
							kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	if (err != noErr) return NS_ERROR_FAILURE;
	
	if (aDocToLoad)
	{
		FSSpec		docSpec;

		err = AECreateList(nil, 0, false, &docList);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		err = AECreateDesc(typeFSS, &docSpec, sizeof(FSSpec), &docDesc);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		err = AEPutDesc(&docList, 0, &docDesc);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		err = AEPutParamDesc(&theEvent, keyDirectObject, &docList);
		if (err != noErr) return NS_ERROR_FAILURE;
	}
	
	if (running)
	{
		err = AESend(&theEvent, &theReply, kAENoReply, kAENormalPriority, kNoTimeOut, nil, nil);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		if (!aLaunchInBackground)
		{
			err = ::SetFrontProcess(&thePSN);
			if (err != noErr) return NS_ERROR_FAILURE;
		}
	}
	else
	{
		LaunchParamBlockRec	launchThis = {0};
		PRUint16						launchControlFlags = (launchContinue | launchNoFileFlags);
		if (aLaunchInBackground)
			launchControlFlags |= launchDontSwitch;
		
		err = AECoerceDesc(&theEvent, typeAppParameters, &launchDesc);
		if (err != noErr) return NS_ERROR_FAILURE;

		::HLock(theEvent.dataHandle);
		
		launchThis.launchAppSpec = (FSSpecPtr)&appSpec;
		launchThis.launchAppParameters = (AppParametersPtr)*launchDesc.dataHandle;
		launchThis.launchBlockID = extendedBlock;
		launchThis.launchEPBLength = extendedBlockLen;
		launchThis.launchFileFlags = 0;
		launchThis.launchControlFlags = launchControlFlags;
		err = ::LaunchApplication(&launchThis);
		if (err != noErr) return NS_ERROR_FAILURE;
		
		// let's be nice and wait until it's running
		const PRUint32	kMaxTimeToWait = 60;		// wait 1 sec max
		PRUint32				endTicks = ::TickCount() + kMaxTimeToWait;

		PRBool					foundApp = PR_FALSE;
		
		do
		{
			EventRecord		theEvent;
			(void)WaitNextEvent(nullEvent, &theEvent, 1, NULL);

			ProcessSerialNumber		psn;
			foundApp = NS_SUCCEEDED(FindRunningAppByFSSpec(appSpec, psn));
		
		} while (!foundApp && (::TickCount() <= endTicks));

		NS_ASSERTION(foundApp, "Failed to find app after launching it");
	}
	
	if (theEvent.dataHandle != nil) AEDisposeDesc(&theEvent);
	if (theReply.dataHandle != nil) AEDisposeDesc(&theReply);

	return NS_OK;
}


#pragma mark -
#pragma mark [Methods that won't be implemented on Mac]

NS_IMETHODIMP
nsLocalFile::Normalize()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::GetPermissions(PRUint32 *aPermissions)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::GetPermissionsOfLink(PRUint32 *aPermissionsOfLink)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP  
nsLocalFile::SetPermissions(PRUint32 aPermissions)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::SetPermissionsOfLink(PRUint32 aPermissions)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP  
nsLocalFile::IsSpecial(PRBool *_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

#pragma mark -
#pragma mark [nsILocalFileMac]
// Implementation of Mac specific finctions from nsILocalFileMac


NS_IMETHODIMP nsLocalFile::GetInitType(nsLocalFileMacInitType *type)
{
	NS_ENSURE_ARG(type);
	*type = mInitType;
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::InitWithFSSpec(const FSSpec *fileSpec)
{
	MakeDirty();
	mSpec = *fileSpec;
	mInitType = eInitWithFSSpec;
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::InitFindingAppByCreatorCode(OSType aAppCreator)
{
	// is the app running?
	FSSpec							appSpec;
	ProcessSerialNumber psn;
	
	nsresult rv = FindRunningAppBySignature(aAppCreator, appSpec, psn);
	if (rv == NS_ERROR_FILE_NOT_FOUND)
	{
		// we have to look on disk
		rv = FindAppOnLocalVolumes(aAppCreator, appSpec);
		if (NS_FAILED(rv)) return rv;
	}
	else if (NS_FAILED(rv))
		return rv;

	// init with the spec here
	return InitWithFSSpec(&appSpec);
}

NS_IMETHODIMP nsLocalFile::GetFSSpec(FSSpec *fileSpec)
{
	NS_ENSURE_ARG(fileSpec);
	*fileSpec = mSpec;
	return NS_OK;

}

NS_IMETHODIMP nsLocalFile::GetResolvedFSSpec(FSSpec *fileSpec)
{
	NS_ENSURE_ARG(fileSpec);
	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	*fileSpec = mResolvedSpec;
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::GetTargetFSSpec(FSSpec *fileSpec)
{
	NS_ENSURE_ARG(fileSpec);
	nsresult rv = ResolveAndStat(PR_TRUE);
	if (NS_FAILED(rv)) return rv;
	*fileSpec = mTargetSpec;
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::SetAppendedPath(const char *aPath)
{
	MakeDirty();
	
	mAppendedPath.Assign(aPath);
	
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::GetAppendedPath(char **_retval)
{
	NS_ENSURE_ARG_POINTER(_retval);
	*_retval = (char*) nsAllocator::Clone(mAppendedPath, strlen(mAppendedPath)+1);
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::GetFileTypeAndCreator(OSType *type, OSType *creator)
{
	NS_ENSURE_ARG(type);
	NS_ENSURE_ARG(creator);
	
	ResolveAndStat(PR_TRUE);
	
	FInfo info;
	OSErr err = ::FSpGetFInfo(&mTargetSpec, &info);
	if (err != noErr)
		return NS_ERROR_FILE_NOT_FOUND;
	*type = info.fdType;
	*creator = info.fdCreator;	
	
	return NS_OK;
}

NS_IMETHODIMP nsLocalFile::SetFileTypeAndCreator(OSType type, OSType creator)
{
	FInfo info;
	OSErr err = ::FSpGetFInfo(&mTargetSpec, &info);
	if (err != noErr)
		return NS_ERROR_FILE_NOT_FOUND;
	
	// See if the user specified a type or creator before changing from what was read
	if (type)
		info.fdType = type;
	if (creator)
		info.fdCreator = creator;
	
	err = ::FSpSetFInfo(&mTargetSpec, &info);
	if (err != noErr)
		return NS_ERROR_FILE_ACCESS_DENIED;
	
	return NS_OK;
}

NS_IMETHODIMP  
nsLocalFile::GetFileSizeWithResFork(PRInt64 *aFileSize)
{
	NS_ENSURE_ARG(aFileSize);
	
	aFileSize->hi = 0;
	aFileSize->lo = 0;

	ResolveAndStat(PR_TRUE);
	
	long dataSize = 0;
	long resSize = 0;
	
	OSErr err = FSpGetFileSize(&mTargetSpec, &dataSize, &resSize);
							   
	if (err != noErr)
		return MacErrorMapper(err);
	
	// For now we've only got 32 bits of file size info
	PRInt64 dataInt64 = LL_Zero();
	PRInt64 resInt64 = LL_Zero();
	
	// Combine the size of the resource and data forks
	LL_I2L(resInt64, resSize);
	LL_I2L(dataInt64, dataSize);
	LL_ADD((*aFileSize), dataInt64, resInt64);
	
	return NS_OK;
}


// this nsLocalFile points to the app. We want to launch it, optionally with the document.
NS_IMETHODIMP
nsLocalFile::LaunchAppWithDoc(nsILocalFile* aDocToLoad, PRBool aLaunchInBackground)
{
	// are we launchable?
	PRBool isExecutable;
	nsresult rv = IsExecutable(&isExecutable);
	if (NS_FAILED(rv)) return rv;
	if (!isExecutable) return NS_ERROR_FILE_EXECUTION_FAILED;
	
	FSSpec			docSpec;
	FSSpecPtr		docSpecPtr = nsnull;
	
	nsCOMPtr<nsILocalFileMac> macDoc = do_QueryInterface(aDocToLoad);
	if (macDoc)
	{
		rv = macDoc->GetTargetFSSpec(&docSpec);
		if (NS_FAILED(rv)) return rv;
		
		docSpecPtr = &docSpec;
	}
	
	FSSpec	appSpec;
	rv = GetResolvedFSSpec(&appSpec);
	if (NS_FAILED(rv)) return rv;
	
	rv = MyLaunchAppWithDoc(appSpec, docSpecPtr, aLaunchInBackground);
	return rv;
}


NS_IMETHODIMP
nsLocalFile::OpenDocWithApp(nsILocalFile* aAppToOpenWith, PRBool aLaunchInBackground)
{
	// if aAppToOpenWith is nil, we have to find the app from the creator code
	// of the document
	nsresult rv = NS_OK;
	
	FSSpec		appSpec;
	
	if (aAppToOpenWith)
	{
		nsCOMPtr<nsILocalFileMac> appFileMac = do_QueryInterface(aAppToOpenWith, &rv);
		if (!appFileMac) return rv;
	
		rv = appFileMac->GetTargetFSSpec(&appSpec);
		if (NS_FAILED(rv)) return rv;

		// is it launchable?
		PRBool isExecutable;
		rv = aAppToOpenWith->IsExecutable(&isExecutable);
		if (NS_FAILED(rv)) return rv;
		if (!isExecutable) return NS_ERROR_FILE_EXECUTION_FAILED;
	}
	else
	{
		// look for one
		OSType	fileType, fileCreator;
		rv = GetFileTypeAndCreator(&fileType, &fileCreator);
		if (NS_FAILED(rv)) return rv;
		
		// just make one on the stack
		nsLocalFile	localAppFile;
		rv = localAppFile.InitFindingAppByCreatorCode(fileCreator);
		if (NS_FAILED(rv)) return rv;
		
		rv = localAppFile.GetTargetFSSpec(&appSpec);
		if (NS_FAILED(rv)) return rv;
	}
	
	FSSpec		docSpec;
	rv = GetResolvedFSSpec(&docSpec);
	if (NS_FAILED(rv)) return rv;

	rv = MyLaunchAppWithDoc(appSpec, &docSpec, aLaunchInBackground);
	return rv;
}


#pragma mark -

// Handy dandy utility create routine for something or the other
nsresult 
NS_NewLocalFile(const char* path, nsILocalFile* *result)
{
	nsLocalFile* file = new nsLocalFile();
	if (file == nsnull)
		return NS_ERROR_OUT_OF_MEMORY;
	NS_ADDREF(file);

	nsresult rv = file->InitWithPath(path);
	if (NS_FAILED(rv)) {
		NS_RELEASE(file);
		return rv;
	}
	*result = file;
	return NS_OK;
}

