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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Makoto Hamanaka <VYA04230@nifty.com>
 *   Paul Ashford <arougthopher@lizardland.net>
 *   Sergei Dolgov <sergei_d@fi.tartu.ee>
 */

#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include "nsICharsetConverterManager.h"
#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsdefs.h"
#include "nsIFileChannel.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsArray.h"

//
// Native BeOS FileSelector wrapper
//

#include <StorageKit.h>
#include <Message.h>
#include <Window.h>
#include <String.h>

class nsFilePicker : public nsBaseFilePicker
{
public:
	nsFilePicker();
	virtual ~nsFilePicker();

	NS_DECL_ISUPPORTS

	// nsIFilePicker (less what's in nsBaseFilePicker)
	NS_IMETHOD GetDefaultString(PRUnichar * *aDefaultString);
	NS_IMETHOD SetDefaultString(const PRUnichar * aDefaultString);
	NS_IMETHOD GetDefaultExtension(PRUnichar * *aDefaultExtension);
	NS_IMETHOD SetDefaultExtension(const PRUnichar * aDefaultExtension);
	NS_IMETHOD GetDisplayDirectory(nsILocalFile * *aDisplayDirectory);
	NS_IMETHOD SetDisplayDirectory(nsILocalFile * aDisplayDirectory);
	NS_IMETHOD GetFile(nsILocalFile * *aFile);
	NS_IMETHOD GetFileURL(nsIFileURL * *aFileURL);
	NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
	NS_IMETHOD Show(PRInt16 *_retval);
	NS_IMETHOD AppendFilter(const PRUnichar *aTitle,  const PRUnichar *aFilter) ;

protected:
	// method from nsBaseFilePicker
	NS_IMETHOD InitNative(nsIWidget *aParent,
	                      const PRUnichar *aTitle,
	                      PRInt16 aMode);


	void GetFilterListArray(nsString& aFilterList);
	//  static void GetFileSystemCharset(nsString & fileSystemCharset);
	//  char * ConvertToFileSystemCharset(const PRUnichar *inString);
	//  PRUnichar * ConvertFromFileSystemCharset(const char *inString);

	BWindow*                      mParentWindow;
	nsString                      mTitle;
	PRInt16                       mMode;
	nsCString                     mFile;
	nsString                      mDefault;
	nsString                      mFilterList;
	nsIUnicodeEncoder*            mUnicodeEncoder;
	nsIUnicodeDecoder*            mUnicodeDecoder;
	nsCOMPtr<nsILocalFile>        mDisplayDirectory;
	PRInt16                       mSelectedType;
	nsCOMPtr <nsISupportsArray>   mFiles;
};

class nsFilePanelBeOS : public BLooper, public BFilePanel
{
public:
	nsFilePanelBeOS(file_panel_mode mode,
	                uint32 node_flavors,
	                bool allow_multiple_selection,
	                bool modal,
	                bool hide_when_done);
	virtual ~nsFilePanelBeOS();

	virtual void MessageReceived(BMessage *message);
	virtual void WaitForSelection();

	virtual bool IsOpenSelected() {
		return (SelectedActivity() == OPEN_SELECTED);
	}
	virtual bool IsSaveSelected() {
		return (SelectedActivity() == SAVE_SELECTED);
	}
	virtual bool IsCancelSelected() {
		return (SelectedActivity() == CANCEL_SELECTED);
	}
	virtual uint32 SelectedActivity();

	virtual BList *OpenRefs() { return &mOpenRefs; }
	virtual BString SaveFileName() { return mSaveFileName; }
	virtual entry_ref SaveDirRef() { return mSaveDirRef; }

	enum {
	    NOT_SELECTED    = 0,
	    OPEN_SELECTED   = 1,
	    SAVE_SELECTED   = 2,
	    CANCEL_SELECTED = 3
	};

protected:

	sem_id wait_sem ;
	uint32 mSelectedActivity;
	bool mIsSelected;
	BString mSaveFileName;
	entry_ref mSaveDirRef;
	BList mOpenRefs;
};

#endif // nsFilePicker_h__
