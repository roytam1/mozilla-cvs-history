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

#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#ifndef XP_MACOSX
#include "nsILocalFileMac.h"
#endif
#include "nsIURL.h"
#include "nsVoidArray.h"
#include "nsIFileURL.h"
#include "nsToolkit.h"
#include "nsIEventSink.h"

#include <InternetConfig.h>

#include "nsMacControl.h"
#include "nsCarbonHelpers.h"

#include "nsFilePicker.h"
#include "nsWatchTask.h"

#include "nsIInternetConfigService.h"
#include "nsIMIMEInfo.h"


NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

OSType nsFilePicker::sCurrentProcessSignature = 0;

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

  if (sCurrentProcessSignature == 0)
  {
    ProcessSerialNumber psn;
    ProcessInfoRec  info;
    
    psn.highLongOfPSN = 0;
    psn.lowLongOfPSN  = kCurrentProcess;

    info.processInfoLength = sizeof(ProcessInfoRec);
    info.processName = nil;
    info.processAppSpec = nil;
    OSErr err = ::GetProcessInformation(&psn, &info);
    if (err == noErr)
        sCurrentProcessSignature = info.processSignature;
    // Try again next time if error
  }
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
  char *filterBuffer = ToNewCString(filterList);
    
  FSSpec theFile;
  PRInt16 userClicksOK = returnCancel;
  
  // XXX Ignore the filter list for now....
  
  if (mMode == modeOpen)
    userClicksOK = GetLocalFile(mTitle, &theFile);
  else if (mMode == modeSave)
    userClicksOK = PutLocalFile(mTitle, mDefault, &theFile);
  else if (mMode == modeGetFolder)
    userClicksOK = GetLocalFolder(mTitle, &theFile);

  // Clean up filter buffers
  delete[] filterBuffer;

#ifndef XP_MACOSX
  if (userClicksOK == returnOK || userClicksOK == returnReplace)
  {
    nsCOMPtr<nsILocalFile>    localFile(do_CreateInstance("@mozilla.org/file/local;1"));
	  nsCOMPtr<nsILocalFileMac> macFile(do_QueryInterface(localFile));

    nsresult rv = macFile->InitWithFSSpec(&theFile);
    if (NS_FAILED(rv)) return rv;

    mFile = do_QueryInterface(macFile);
  }
#endif
  
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
      nsCOMPtr<nsIEventSink> sink;
      nsToolkit::GetWindowEventSink ( window, getter_AddRefs(sink) );
      if ( sink ) {
        ::BeginUpdate(window);
        PRBool handled = PR_FALSE;
        sink->DispatchEvent(cbRec->eventData.eventDataParms.event, &handled);
        ::EndUpdate(window);	        
      }        
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
    if ( mFlatFilters[i]->Equals(extension) )
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




#if TARGET_CARBON
//-------------------------------------------------------------------------
//
// GetFile
//
// Use NavServices to do a GetFile. Returns PR_TRUE if the user presses OK in the dialog. If
// they do so, the selected file is in the FSSpec.
//
//-------------------------------------------------------------------------
PRInt16
nsFilePicker::GetLocalFile(const nsString & inTitle, /* filter list here later */ FSSpec* outSpec)
{
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails
  NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(FileDialogFilterProc);  // doesn't really matter if this fails
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;
  // Set defaults
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;
    
  // Set the options for how the get file dialog will appear
  dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  dialogCreateOptions.optionFlags ^= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, 
                                                     (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;

  // sets up the |mTypeLists| array so the filter proc can use it
  MapFilterToFileTypes();
	
  // allow packages to be chosen if the filter is "*"
  if (mAllFilesDisplayed)
    dialogCreateOptions.optionFlags |= kNavSupportPackages;		

  // Display the get file dialog. Only use a filter proc if there are any
  // filters registered.
  anErr = ::NavCreateGetFileDialog(
                                  &dialogCreateOptions,
                                  NULL, //  NavTypeListHandle
                                  eventProc,
                                  NULL, //  NavPreviewUPP
                                  mFilters.Count() ? filterProc : NULL,
                                  NULL, //  inClientData
                                  &dialog);
  if (anErr == noErr) {
    nsWatchTask::GetTask().Suspend();  
    anErr = ::NavDialogRun(dialog);
    nsWatchTask::GetTask().Resume();  
    if (anErr == noErr) {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord) {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSSpec      theFSSpec;
        FSRef       theFSRef;
        
          // Get the FSRef for the file to be opened (or directory in case of a package)
        anErr = ::AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType,
                              &theFSRef, sizeof(theFSRef), &actualSize);
        if (anErr == noErr) {
          // Convert to FSSpec
          anErr = ::FSGetCatalogInfo(
                                     &theFSRef,
                                     0,
                                     NULL,
                                     NULL,
                                     &theFSSpec,
                                     NULL);
         
          if (anErr == noErr) {
            *outSpec = theFSSpec;	// Return the FSSpec
            retVal = returnOK;
          }
        }
        // Some housekeeping for Nav Services 
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }
  ::CFRelease(titleRef);

  if (filterProc)
    ::DisposeNavObjectFilterUPP(filterProc);
	
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
nsFilePicker::GetLocalFolder(const nsString & inTitle, FSSpec* outSpec)
{
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;

  // Set defaults
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;

  // Set the options for how the get file dialog will appear
  dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  dialogCreateOptions.optionFlags ^= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;

  anErr = ::NavCreateChooseFolderDialog(
                                        &dialogCreateOptions,
                                        eventProc,
                                        NULL, // filter proc
                                        NULL, // inClientData
                                        &dialog);

  if (anErr == noErr) {
    nsWatchTask::GetTask().Suspend();  
    anErr = ::NavDialogRun(dialog);
    nsWatchTask::GetTask().Resume();  
    if (anErr == noErr) {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord) {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSSpec      theFSSpec;
         
        // Get the FSSpec for the selected folder
        anErr = ::AEGetNthPtr(&(reply.selection), 1, typeFSS, &theKeyword, &actualType,
                              &theFSSpec, sizeof(theFSSpec), &actualSize);
        if (anErr == noErr) {
          *outSpec = theFSSpec;	// Return the FSSpec
          retVal = returnOK;
        }
        // Some housekeeping for Nav Services 
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }
  ::CFRelease(titleRef);
	
  if (eventProc)
    ::DisposeNavEventUPP(eventProc);
		
  return retVal;
} // GetFolder

PRInt16
nsFilePicker::PutLocalFile(const nsString & inTitle, const nsString & inDefaultName, FSSpec* outFileSpec)
{
  PRInt16 retVal = returnCancel;
  NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails
  OSType typeToSave = 'TEXT';
  OSType creatorToSave = (sCurrentProcessSignature == 0) ? 'MOZZ' : sCurrentProcessSignature;
  NavDialogRef dialog;
  NavDialogCreationOptions dialogCreateOptions;

  // Set defaults
  OSErr anErr = ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
  if (anErr != noErr)
    return retVal;


  // Set the options for how the get file dialog will appear
  dialogCreateOptions.optionFlags |= kNavNoTypePopup;
  dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
  dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
  dialogCreateOptions.optionFlags ^= kNavAllowMultipleFiles;
  dialogCreateOptions.modality = kWindowModalityAppModal;
  dialogCreateOptions.parentWindow = NULL;
  CFStringRef titleRef = CFStringCreateWithCharacters(NULL, 
                                                      (const UniChar *) inTitle.get(), inTitle.Length());
  dialogCreateOptions.windowTitle = titleRef;
  CFStringRef defaultFileNameRef = CFStringCreateWithCharacters(NULL, 
                                                                (const UniChar *) inDefaultName.get(), 
                                                                inDefaultName.Length());
  dialogCreateOptions.saveFileName = defaultFileNameRef;

  anErr = ::NavCreatePutFileDialog(
                                   &dialogCreateOptions,
                                   typeToSave,
                                   creatorToSave,
                                   eventProc,
                                   NULL, // inClientData
                                   &dialog);

  if (anErr == noErr) {
    nsWatchTask::GetTask().Suspend();  
    anErr = ::NavDialogRun(dialog);
    nsWatchTask::GetTask().Resume();  
    if (anErr == noErr) {
    	NavReplyRecord reply;
      anErr = ::NavDialogGetReply(dialog, &reply);
      if (anErr == noErr && reply.validRecord) {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSSpec      theFSSpec;
        FSRef       theFSRef;
        FSCatalogInfo catalogInfo;

        // Get the FSRef for the directory where the file to be saved
        anErr = ::AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType,
                              &theFSRef, sizeof(theFSRef), &actualSize);
        if (anErr == noErr) {
          // Convert to FSSpec, also get nodeID.
          anErr = ::FSGetCatalogInfo(
                                     &theFSRef,
                                     kFSCatInfoNodeID,
                                     &catalogInfo,
                                     NULL,
                                     &theFSSpec,
                                     NULL);
          if (anErr == noErr) {
            theFSSpec.parID = catalogInfo.nodeID;
            TextEncoding theEncoding;
            anErr = ::UpgradeScriptInfoToTextEncoding(
                                                      smSystemScript, 
                                                      kTextLanguageDontCare,
                                                      kTextRegionDontCare,
                                                      NULL,
                                                      &theEncoding);
            if (anErr != noErr)
              theEncoding = kTextEncodingMacRoman;
            ::CFStringGetPascalString(
                                      reply.saveFileName,
                                      theFSSpec.name,
                                      sizeof(theFSSpec.name),
                                      theEncoding);
    				
             *outFileSpec = theFSSpec;	// Return the FSSpec

            if (reply.replacing)
              retVal = returnReplace;
            else
              retVal = returnOK;
          }
        }  			  
        // Some housekeeping for Nav Services 
        ::NavCompleteSave(&reply, kNavTranslateInPlace);
        ::NavDisposeReply(&reply);
      }
    }
    ::NavDialogDispose(dialog);
  }
  ::CFRelease(titleRef);
  ::CFRelease(defaultFileNameRef);
	
  if (eventProc)
    ::DisposeNavEventUPP(eventProc);
	
  return retVal;	
}
#else // TARGET_CARBON
//-------------------------------------------------------------------------
//
// GetFile
//
// Use NavServices to do a GetFile. Returns PR_TRUE if the user presses OK in the dialog. If
// they do so, the selected file is in the FSSpec.
//
//-------------------------------------------------------------------------
PRInt16
nsFilePicker::GetLocalFile(const nsString & inTitle, /* filter list here later */ FSSpec* outSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails
	NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(FileDialogFilterProc);  // doesn't really matter if this fails

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
    Str255 title;
    nsMacControl::StringToStr255(inTitle,title);
		::BlockMoveData(title, dialogOptions.windowTitle, *title + 1);
		
		// sets up the |mTypeLists| array so the filter proc can use it
		MapFilterToFileTypes();
		
    // allow packages to be chosen if the filter is "*"
    if ( mAllFilesDisplayed )
      dialogOptions.dialogOptionFlags |= kNavSupportPackages;		

		// Display the get file dialog. Only use a filter proc if there are any
		// filters registered.
    nsWatchTask::GetTask().Suspend();  
		anErr = ::NavGetFile(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					NULL, // preview proc
					mFilters.Count() ? filterProc : NULL,
					NULL, //typeList,
					this); // callbackUD - used by the filterProc
    nsWatchTask::GetTask().Resume();  
	
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
nsFilePicker::GetLocalFolder(const nsString & inTitle, FSSpec* outSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
    Str255 title;
    nsMacControl::StringToStr255(inTitle, title);
		::BlockMoveData(title, dialogOptions.windowTitle, *title + 1);
		
		// Display the get file dialog
    nsWatchTask::GetTask().Suspend();  
		anErr = ::NavChooseFolder(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					NULL, // filter proc
					NULL); // callbackUD	
    nsWatchTask::GetTask().Resume();  
	
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
nsFilePicker::PutLocalFile(const nsString & inTitle, const nsString & inDefaultName, FSSpec* outFileSpec)
{
 	PRInt16 retVal = returnCancel;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NewNavEventUPP(FileDialogEventHandlerProc);  // doesn't really matter if this fails
	OSType				typeToSave = 'TEXT';
	OSType				creatorToSave = 'MOZZ';

	OSErr anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)	{	
		// Set the options for how the get file dialog will appear
		dialogOptions.dialogOptionFlags |= kNavNoTypePopup;
		dialogOptions.dialogOptionFlags |= kNavDontAutoTranslate;
		dialogOptions.dialogOptionFlags |= kNavDontAddTranslateItems;
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;
    Str255 title, defaultname;
    nsMacControl::StringToStr255(inTitle, title);
    nsMacControl::StringToStr255(inDefaultName, defaultname);
		::BlockMoveData(title, dialogOptions.windowTitle, *title + 1);
		::BlockMoveData(defaultname, dialogOptions.savedFileName, *defaultname + 1);
		
		// Display the get file dialog
    nsWatchTask::GetTask().Suspend();  
		anErr = ::NavPutFile(
					NULL,
					&reply,
					&dialogOptions,
					eventProc,
					typeToSave,
					creatorToSave,
					NULL); // callbackUD	
    nsWatchTask::GetTask().Resume();  
	
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
#endif // TARGET_CARBON

//
// MapFilterToFileTypes
//
// Take the list of file types (in a nice win32-specific format) and ask IC to give us 
// the MacOS file type codes for them.
//
void
nsFilePicker :: MapFilterToFileTypes ( )
{
  nsCOMPtr<nsIInternetConfigService> icService ( do_GetService(NS_INTERNETCONFIGSERVICE_CONTRACTID) );
  NS_ASSERTION(icService, "Can't get InternetConfig Service, bailing out");
  if ( !icService ) {
    // We couldn't get the IC Service, bail. Since |mAllFilesDisplayed| is still
    // set, the dialog will allow selection of all files. 
    return;
  }
  
  if (mFilters.Count())
  {
    // First we allocate the memory for the Mac type lists
    for (PRUint32 loop1 = 0; loop1 < mFilters.Count() && loop1 < kMaxTypeListCount; loop1++)
    {
      mTypeLists[loop1] =
        (NavTypeListPtr)NewPtrClear(sizeof(NavTypeList) + kMaxTypesPerFilter * sizeof(OSType));     
      if ( !mTypeLists[loop1] )
        return;                     // don't worry, we'll clean up in the dtor
    }
    
    // Now loop through each of the filter strings
    for (PRUint32 loop1 = 0; loop1 < mFilters.Count(); loop1++)
    {
      const nsString& filterWide = *mFilters[loop1];
      char* filter = ToNewCString(filterWide);

      NS_ASSERTION ( filterWide.Length(), "Oops. filepicker.properties not correctly installed");       
      if ( filterWide.Length() && filter )
      {
        PRUint32 filterIndex = 0;         // Index into the filter string
        PRUint32 typeTempIndex = 0;       // Index into the temp string for a single filter type
        PRUint32 typesInThisFilter = 0;   // Count for # of types in this filter
        bool finishedThisFilter = false;  // Flag so we know when we're finsihed with the filter
        char typeTemp[256];
        char tempChar;           // char we're currently looking at

        // Loop through the characters of filter string. Every time we get to a
        // semicolon (or a null, meaning we've hit the end of the full string)
        // then we've found the filter and can pass it off to IC to get a macOS 
        // file type out of it.
        do
        {
          tempChar = filter[filterIndex];
          if ((tempChar == ';') || (tempChar == 0)) {   // End of filter type reached
            typeTemp[typeTempIndex] = '\0';             // null terminate
            
            // to make it easier to match file extensions while we're filtering, flatten
            // out the list. Ignore filters that are just "*" and also remove the
            // leading "*" from filters we do add.
            if ( strlen(typeTemp) > 1 )
              mFlatFilters.AppendCString ( nsCString((char*)&typeTemp[1]) );  // cut out the "*"
            
            // ask IC if it's not "all files" (designated by "*")
            if ( !(typeTemp[1] == '\0' && typeTemp[0] == '*') ) {
              mAllFilesDisplayed = PR_FALSE;
              
              nsCOMPtr<nsIMIMEInfo> icEntry;
              icService->GetMIMEInfoFromExtension(typeTemp, getter_AddRefs(icEntry));
              if ( icEntry )
              {
                bool addToList = true;
                OSType tempOSType;
                icEntry->GetMacType(NS_REINTERPRET_CAST(PRUint32*, (&tempOSType)));
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
            
            typeTempIndex = 0;      // Reset the temp string for the type
            if (tempChar == '\0')
              finishedThisFilter = true;
          }
          else
          {
            // strip out whitespace as we go
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

} // MapFilterToFileTypes


NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  NS_IF_ADDREF(*aFile = mFile);
  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  NS_ENSURE_TRUE(mFile, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURI> uri;
  NS_NewFileURI(getter_AddRefs(uri), mFile);
  nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(uri));
  NS_ENSURE_TRUE(fileURL, NS_ERROR_FAILURE);
  
  NS_ADDREF(*aFileURL = fileURL);
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
// The default extension to use for files
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetDefaultExtension(PRUnichar **aExtension)
{
  *aExtension = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetDefaultExtension(const PRUnichar *aExtension)
{
  return NS_OK;
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
  mFilters.AppendString(nsDependentString(aFilter));
  mTitles.AppendString(nsDependentString(aTitle));
  
  return NS_OK;
}

