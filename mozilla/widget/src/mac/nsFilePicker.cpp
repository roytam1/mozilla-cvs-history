/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include <StandardFile.h>

#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsILocalFileMac.h"
#include "nsIURL.h"
#include "nsVoidArray.h"
#include "nsStringUtil.h"

#include <InternetConfig.h>

#include "nsMacControl.h"
#include "nsCarbonHelpers.h"

#include "nsFilePicker.h"
#include "nsMacWindow.h"
#include "nsMacMessageSink.h"




NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

//-------------------------------------------------------------------------
//
// nsFilePicker constructor
//
//-------------------------------------------------------------------------
nsFilePicker::nsFilePicker()
  : mAllFilesDisplayed(PR_TRUE)
{
  NS_INIT_REFCNT();
  
  // Zero out the type lists
  for (int i = 0; i < kMaxTypeListCount; i++)
  	mTypeLists[i] = 0L;
}


//-------------------------------------------------------------------------
//
// nsFilePicker destructor
//
//-------------------------------------------------------------------------
nsFilePicker::~nsFilePicker()
{
	// Destroy any filters we have built
	if ( mFilters.Count() ) {
	  for (int i = 0; i < kMaxTypeListCount; i++) {
	  	if (mTypeLists[i])
	  		DisposePtr((Ptr)mTypeLists[i]);
	  }
	}	
  mFilters.Clear();
  mTitles.Clear();

}


NS_IMETHODIMP
nsFilePicker::InitNative(nsIWidget *aParent, const PRUnichar *aTitle, PRInt16 aMode)
{
  mTitle = aTitle;
  mMode = aMode;

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Ok's the dialog
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::OnOk()
{
  mWasCancelled  = PR_FALSE;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Cancel the dialog
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::OnCancel()
{
  mWasCancelled  = PR_TRUE;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Show - Display the file dialog
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::Show(PRInt16 *retval)
{
  NS_ENSURE_ARG_POINTER(retval);

  *retval = returnCancel;
  
  nsString filterList;
  char *filterBuffer = filterList.ToNewCString();

  Str255 title;
  Str255 defaultName;
  nsMacControl::StringToStr255(mTitle,title);
  nsMacControl::StringToStr255(mDefault,defaultName);
    
  FSSpec theFile;
  PRInt16 userClicksOK = returnCancel;
  
  // XXX Ignore the filter list for now....
  
  if (mMode == modeOpen)
    userClicksOK = GetLocalFile(title, &theFile);
  else if (mMode == modeSave)
    userClicksOK = PutLocalFile(title, defaultName, &theFile);
  else if (mMode == modeGetFolder)
    userClicksOK = GetLocalFolder(title, &theFile);

  // Clean up filter buffers
  delete[] filterBuffer;

  if (userClicksOK == returnOK || userClicksOK == returnReplace)
  {
    nsCOMPtr<nsILocalFile>    localFile(do_CreateInstance("@mozilla.org/file/local;1"));
	  nsCOMPtr<nsILocalFileMac> macFile(do_QueryInterface(localFile));

    nsresult rv = macFile->InitWithFSSpec(&theFile);
    if (NS_FAILED(rv)) return rv;

    mFile = do_QueryInterface(macFile);
  }
  
  *retval = userClicksOK;
  return NS_OK;
}


//
// FileDialogEventHandlerProc
//
// An event filter proc for NavServices so the dialogs will be movable-modals.
//
static pascal void FileDialogEventHandlerProc( NavEventCallbackMessage msg, NavCBRecPtr cbRec, NavCallBackUserData data )
{
	switch ( msg ) {
	case kNavCBEvent:
		switch ( cbRec->eventData.eventDataParms.event->what ) {
		case updateEvt:
      WindowPtr window = reinterpret_cast<WindowPtr>(cbRec->eventData.eventDataParms.event->message);
      nsMacWindow* macWindow = nsMacMessageSink::GetNSWindowFromMacWindow(window);
      ::BeginUpdate(window);
      if (macWindow) {
        EventRecord   theEvent = *cbRec->eventData.eventDataParms.event;
        macWindow->HandleOSEvent(theEvent);
      }        
      ::EndUpdate(window);	        
			break;
		}
		break;
	}
}


//
// IsFileInFilterList
//
// Check our |mTypeLists| list to see if the given type is in there.
//
Boolean
nsFilePicker :: IsTypeInFilterList ( ResType inType )
{
  for ( int i = 0; i < mFilters.Count(); ++i ) {
    for ( int j = 0; j < mTypeLists[i]->osTypeCount; ++j ) {
      if ( mTypeLists[i]->osType[j] == inType ) 
        return true;
    }  // foreach type w/in the group
  } // for each filter group
  
  return false;
  
} // IsFileInFilterList


Boolean
nsFilePicker :: IsExtensionInFilterList ( StrFileName & inFileName )
{
  char extension[256];
  
  // determine the extension from the file name
  unsigned char* curr = &inFileName[inFileName[0]];
  while ( curr != inFileName && *curr-- != '.' ) ;
  if ( curr == inFileName )                         // no '.' in string, fails this check
    return false;
  ++curr;                                           // we took one too many steps back
  short extensionLen = (inFileName + inFileName[0]) - curr + 1;
  strncpy ( extension, (char*)curr, extensionLen);
  extension[extensionLen] = '\0';
  
  // see if it is in our list
  for ( int i = 0; i < mFlatFilters.Count(); ++i ) {
    if ( *mFlatFilters[i] == extension )
      return true;
  }
  return false;
}


//
// FileDialogFilterProc
//
// Called from navServices with our filePicker object as |callbackUD|, check our
// internal list to see if the file should be displayed.
//
pascal
Boolean 
nsFilePicker :: FileDialogFilterProc ( AEDesc* theItem, void* theInfo,
                                        NavCallBackUserData callbackUD, NavFilterModes filterMode )
{
  Boolean shouldDisplay = true;
  nsFilePicker* self = NS_REINTERPRET_CAST(nsFilePicker*, callbackUD);
  if ( self && !self->mAllFilesDisplayed ) {
    if ( theItem->descriptorType == typeFSS ) {
      NavFileOrFolderInfo* info = NS_REINTERPRET_CAST ( NavFileOrFolderInfo*, theInfo );
      if ( !info->isFolder ) {
        // check it against our list. If that fails, check the extension directly
        if ( ! self->IsTypeInFilterList(info->fileAndFolder.fileInfo.finderInfo.fdType) ) {
          FSSpec fileSpec;
          if ( ::AEGetDescData(theItem, &fileSpec, sizeof(FSSpec)) == noErr )
            if ( ! self->IsExtensionInFilterList(fileSpec.name) )
              shouldDisplay = false;
        }
      } // if file isn't a folder
    } // if the item is an FSSpec
  }
  
  return shouldDisplay;
  
} // FileDialogFilterProc                                        




//-------------------------------------------------------------------------
//
// GetFile
//
// Use NavServices to do a GetFile. Returns PR_TRUE if the user presses OK in the dialog. If
// they do so, the selected file is in the FSSpec.
//
//-------------------------------------------------------------------------
PRInt16
nsFilePicker::GetLocalFile(Str255 & inTitle, /* filter list here later */ FSSpec* outSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventProc(FileDialogEventHandlerProc);  // doesn't really matter if this fails
	NavObjectFilterUPP filterProc = NewNavObjectFilterProc(FileDialogFilterProc);  // doesn't really matter if this fails

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
		::BlockMoveData(inTitle, dialogOptions.message, *inTitle + 1);
		
		// sets up the |mTypeLists| array so the filter proc can use it
		MapFilterToFileTypes();
		
		// Display the get file dialog. Only use a filter proc if there are any
		// filters registered.
		anErr = ::NavGetFile(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					NULL, // preview proc
					mFilters.Count() ? filterProc : NULL,
					NULL, //typeList,
					this); // callbackUD - used by the filterProc
	
		// See if the user has selected save
		if (anErr == noErr && reply.validRecord) {
			AEKeyword	theKeyword;
			DescType	actualType;
			Size		actualSize;
			FSSpec		theFSSpec;
			
			// Get the FSSpec for the file to be opened
			anErr = AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
				&theFSSpec, sizeof(theFSSpec), &actualSize);
			
			if (anErr == noErr)
			{
				*outSpec = theFSSpec;	// Return the FSSpec
				retVal = returnOK;
				
				// Some housekeeping for Nav Services 
				::NavDisposeReply(&reply);
			}

		} // if user clicked OK	
	} // if can get dialog options
	
	if ( eventProc )
		::DisposeNavEventUPP(eventProc);
		
	return retVal;

} // GetFile


//-------------------------------------------------------------------------
//
// GetFolder
//
// Use NavServices to do a PutFile. Returns PR_TRUE if the user presses OK in the dialog. If
// they do so, the folder location is in the FSSpec.
//
//-------------------------------------------------------------------------
PRInt16
nsFilePicker::GetLocalFolder(Str255 & inTitle, FSSpec* outSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventProc(FileDialogEventHandlerProc);  // doesn't really matter if this fails

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
		::BlockMoveData(inTitle, dialogOptions.message, *inTitle + 1);
		
		// Display the get file dialog
		anErr = ::NavChooseFolder(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					NULL, // filter proc
					NULL); // callbackUD	
	
		// See if the user has selected save
		if (anErr == noErr && reply.validRecord) {
			AEKeyword	theKeyword;
			DescType	actualType;
			Size		actualSize;
			FSSpec		theFSSpec;
			
			// Get the FSSpec for the file to be opened
			anErr = AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
				&theFSSpec, sizeof(theFSSpec), &actualSize);
			
			if (anErr == noErr) {
				*outSpec = theFSSpec;	// Return the FSSpec
				retVal = returnOK;
				
				// Some housekeeping for Nav Services 
				::NavDisposeReply(&reply);
			}

		} // if user clicked OK	
	} // if can get dialog options
	
	if ( eventProc )
		::DisposeNavEventUPP(eventProc);
		
	return retVal;
} // GetFolder

PRInt16
nsFilePicker::PutLocalFile(Str255 & inTitle, Str255 & inDefaultName, FSSpec* outFileSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventProc(FileDialogEventHandlerProc);  // doesn't really matter if this fails
	OSType				typeToSave = 'TEXT';
	OSType				creatorToSave = 'MOZZ';

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
		::BlockMoveData(inTitle, dialogOptions.message, *inTitle + 1);
		::BlockMoveData(inDefaultName, dialogOptions.savedFileName, *inDefaultName + 1);
		
		// Display the get file dialog
		anErr = ::NavPutFile(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					typeToSave,
					creatorToSave,
					NULL); // callbackUD	
	
		// See if the user has selected save
		if (anErr == noErr && reply.validRecord)
		{
			AEKeyword	theKeyword;
			DescType	actualType;
			Size		actualSize;
			FSSpec		theFSSpec;
			
			// Get the FSSpec for the file to be opened
			anErr = AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
				&theFSSpec, sizeof(theFSSpec), &actualSize);
			
			if (anErr == noErr) {
				*outFileSpec = theFSSpec;	// Return the FSSpec
				
				if (reply.replacing)
					retVal = returnReplace;
				else
					retVal = returnOK;
				
				// Some housekeeping for Nav Services 
				::NavCompleteSave(&reply, kNavTranslateInPlace);
				::NavDisposeReply(&reply);
			}

		} // if user clicked OK	
	} // if can get dialog options
	
	if ( eventProc )
		::DisposeNavEventUPP(eventProc);
	
	return retVal;	
}


//
// MapFilterToFileTypes
//
// Take the list of file types (in a nice win32-specific format) and ask IC to give us 
// the MacOS file type codes for them.
//
void
nsFilePicker :: MapFilterToFileTypes ( )
{
	OSType			tempOSType;
	ICInstance		icInstance;
	OSStatus			icErr;
	Handle			mappings = NewHandleClear(4);
	ICAttr			attr;
	ICMapEntry		icEntry;
	
	// assume we're going to show all files until proven otherwise with
	// a filter that isn't "*"
	mAllFilesDisplayed = PR_TRUE;
	
	// grab the IC mappingDB so that it's a little faster looping over the file
	// types.
	icErr = ICStart(&icInstance, 'MOZZ');
	if (icErr == noErr)
	{
		icErr = ICFindConfigFile(icInstance, 0, nil);
		if (icErr == noErr)
		{
			icErr = ICFindPrefHandle(icInstance, kICMapping, &attr, mappings);
			if (icErr != noErr)
				goto bail_w_IC;
		}
		else
			goto bail_w_IC;
	}
	else
		goto bail_wo_IC;
	
	
	if (mFilters.Count())
	{
		// First we allocate the memory for the Mac type lists
		for (PRUint32 loop1 = 0; loop1 < mFilters.Count() && loop1 < kMaxTypeListCount; loop1++)
		{
			mTypeLists[loop1] =
				(NavTypeListPtr)NewPtrClear(sizeof(NavTypeList) + kMaxTypesPerFilter * sizeof(OSType));
			
			if (mTypeLists[loop1] == nil)
				goto bail_w_IC;
		}
		
		// Now loop through each of the filter strings
    for (PRUint32 loop1 = 0; loop1 < mFilters.Count(); loop1++)
  	{
  		const nsString& filterWide = *mFilters[loop1];
  		char* filter = filterWide.ToNewCString();

      NS_ASSERTION ( filterWide.Length(), "Oops. filepicker.properties not correctly installed");       
  		if ( filterWide.Length() && filter )
  		{
        PRUint32 filterIndex = 0;         // Index into the filter string
        PRUint32 typeTempIndex = 1;       // Index into the temp string for a single filter type
        PRUint32 typesInThisFilter = 0;   // Count for # of types in this filter
        bool finishedThisFilter = false;  // Flag so we know when we're finsihed with the filter
        Str255 typeTemp;
        char tempChar;           // char we're currently looking at

        // Loop through the characters of filter string. Every time we get to a
        // semicolon (or a null, meaning we've hit the end of the full string)
        // then we've found the filter and can pass it off to IC to get a macOS 
        // file type out of it.
  			do
	  		{
	  		  tempChar = filter[filterIndex];
	  			if ((tempChar == ';') || (tempChar == 0)) {   // End of filter type reached
	  				typeTemp[typeTempIndex] = 0;                // turn it into a pString 
	  				typeTemp[0] = typeTempIndex - 1;
	  				
	  				// to make it easier to match file extensions while we're filtering, flatten
	  				// out the list. Ignore filters that are just "*" and also remove the
	  				// leading "*" from filters we do add.
	  				if ( typeTemp[0] > 1 )
	  				  mFlatFilters.AppendCString ( nsCString((char*)&typeTemp[2]) );  // cut out the "*"
	  				
	  				// ask IC if it's not "all files" (designated by "*")
	  				if ( !(typeTemp[0] == 1 && typeTemp[1] == '*') ) {
	  				  mAllFilesDisplayed = PR_FALSE;
	  				  			
  	  				icErr = ICMapEntriesFilename(icInstance, mappings, typeTemp, &icEntry);
  	  				if (icErr != icPrefNotFoundErr)
  	  				{
  	  					bool addToList = true;
  	  					tempOSType = icEntry.fileType;
  	  					for (PRUint32 typeIndex = 0; typeIndex < typesInThisFilter; typeIndex++)
  	  					{
  	  						if (mTypeLists[loop1]->osType[typeIndex] == tempOSType)
  	  						{
  	  							addToList = false;
  	  							break;
  	  						}
  	  					}
  	  					if (addToList && typesInThisFilter < kMaxTypesPerFilter)
  	  						mTypeLists[loop1]->osType[typesInThisFilter++] = tempOSType;
  	  				}
	  				} // if not "*"
	  				
	  				typeTempIndex = 1;			// Reset the temp string for the type
	  				typeTemp[0] = 0;
	  				if (tempChar == 0)
	  					finishedThisFilter = true;
	  			}
	  			else
	  			{
	  			  // strip out whitespace as we goe
	  			  if ( tempChar != ' ' )
	  				  typeTemp[typeTempIndex++] = tempChar;
	  			}
	  			
	  			filterIndex++;
	  		} while (!finishedThisFilter);
	  		
	  		// Set how many OSTypes we actually found
	  		mTypeLists[loop1]->osTypeCount = typesInThisFilter;
	  		
	  		nsMemory :: Free ( NS_REINTERPRET_CAST(void*, filter) );
  		}
    }
	}

bail_w_IC:
	ICStop(icInstance);

bail_wo_IC:
  ;
}


NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  NS_IF_ADDREF(*aFile = mFile);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  nsCOMPtr<nsIFileURL> file(do_CreateInstance("@mozilla.org/network/standard-url;1"));
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);
  file->SetFile(mFile);
  NS_ADDREF(*aFileURL = file);
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the file + path
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::SetDefaultString(const PRUnichar *aString)
{
  mDefault = aString;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetDefaultString(PRUnichar **aString)
{
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
// Set the display directory
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::SetDisplayDirectory(nsILocalFile *aDirectory)
{
  mDisplayDirectory = aDirectory;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get the display directory
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetDisplayDirectory(nsILocalFile **aDirectory)
{
  *aDirectory = mDisplayDirectory;
  NS_IF_ADDREF(*aDirectory);
  return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::AppendFilter(const PRUnichar *aTitle, const PRUnichar *aFilter)
{
  mFilters.AppendString(nsLiteralString(aFilter));
  mTitles.AppendString(nsLiteralString(aTitle));
  
  return NS_OK;
}

