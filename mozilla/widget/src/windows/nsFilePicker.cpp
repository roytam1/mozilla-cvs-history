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
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s):
 *   Stuart Parmenter <pavlov@netscape.com>
 */

// Define so header files for openfilename are included
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"
#include "nsFilePicker.h"
#include "nsILocalFile.h"
#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsIStringBundle.h"
#include "nsCRT.h"
#include <windows.h>
#include <SHLOBJ.H>

#if defined(WINCE)
#include <commdlg.h>
#endif

#ifdef MOZ_UNICODE
#include "nsString.h"
#include "nsToolkit.h"
#endif // MOZ_UNICODE

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

#ifdef MOZ_UNICODE
nsString nsFilePicker::mLastUsedUnicodeDirectory;
#endif
char nsFilePicker::mLastUsedDirectory[MAX_PATH+1] = { 0 };

#define MAX_EXTENSION_LENGTH 10

//-------------------------------------------------------------------------
//
// nsFilePicker constructor
//
//-------------------------------------------------------------------------
nsFilePicker::nsFilePicker()
{
  NS_INIT_REFCNT();
  mWnd = NULL;
  mUnicodeEncoder = nsnull;
  mUnicodeDecoder = nsnull;
  mSelectedType   = 1;
  mDisplayDirectory = do_CreateInstance("@mozilla.org/file/local;1");
}

//-------------------------------------------------------------------------
//
// nsFilePicker destructor
//
//-------------------------------------------------------------------------
nsFilePicker::~nsFilePicker()
{
  NS_IF_RELEASE(mUnicodeEncoder);
  NS_IF_RELEASE(mUnicodeDecoder);
}

//-------------------------------------------------------------------------
//
// Show - Display the file dialog
//
//-------------------------------------------------------------------------
#ifdef MOZ_UNICODE
NS_IMETHODIMP nsFilePicker::ShowW(PRInt16 *aReturnVal)
{
  NS_ENSURE_ARG_POINTER(aReturnVal);

  PRBool result = PR_FALSE;
  PRUnichar fileBuffer[MAX_PATH+1];
  wcsncpy(fileBuffer,  mDefault.get(), MAX_PATH);

  nsAutoString htmExt(NS_LITERAL_STRING("html"));
  PRUnichar *title = ToNewUnicode(mTitle);
  nsAutoString initialDir;
  mDisplayDirectory->GetPath(initialDir);

  // If no display directory, re-use the last one.
  if(initialDir.IsEmpty()) {
    // Allocate copy of last used dir.
    initialDir = mLastUsedUnicodeDirectory;
  }

  mUnicodeFile.SetLength(0);

  if (mMode == modeGetFolder) {
    PRUnichar dirBuffer[MAX_PATH+1];
    wcsncpy(dirBuffer, initialDir.get(), MAX_PATH);

    BROWSEINFOW browserInfo;
    browserInfo.hwndOwner      = mWnd;
    browserInfo.pidlRoot       = nsnull;
    browserInfo.pszDisplayName = (LPWSTR)dirBuffer;
    browserInfo.lpszTitle      = title;
    browserInfo.ulFlags        = BIF_RETURNONLYFSDIRS;//BIF_STATUSTEXT | BIF_RETURNONLYFSDIRS;
    browserInfo.lpfn           = nsnull;
    browserInfo.lParam         = nsnull;
    browserInfo.iImage         = nsnull;

    // XXX UNICODE support is needed here --> DONE
    LPITEMIDLIST list = ::SHBrowseForFolderW(&browserInfo);
    if (list != NULL) {
      result = ::SHGetPathFromIDListW(list, (LPWSTR)fileBuffer);
      if (result) {
          mUnicodeFile.Append(fileBuffer);
      }
  
      // free PIDL
      LPMALLOC pMalloc = NULL;
      ::SHGetMalloc(&pMalloc);
      if(pMalloc) {
         pMalloc->Free(list);
         pMalloc->Release();
      }
    }
  }
  else {

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    nsString filterBuffer = mFilterList;
                                  
    if (!initialDir.IsEmpty()) {
      ofn.lpstrInitialDir = initialDir.get();
    }
    
    ofn.lpstrTitle   = (LPCWSTR)title;
    ofn.lpstrFilter  = (LPCWSTR)filterBuffer.get();
    ofn.nFilterIndex = mSelectedType;
    ofn.hwndOwner    = mWnd;
    ofn.lpstrFile    = fileBuffer;
    ofn.nMaxFile     = MAX_PATH;

    ofn.Flags = OFN_NOCHANGEDIR | OFN_SHAREAWARE | OFN_LONGNAMES | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    if (!mDefaultExtension.IsEmpty()) {
      ofn.lpstrDefExt = mDefaultExtension.get();
    }
    else {
      // Get file extension from suggested filename
      //  to detect if we are saving an html file
      //XXX: nsIFile SHOULD HAVE A GetExtension() METHOD!
      PRInt32 extIndex = mDefault.RFind(".");
      if ( extIndex >= 0) {
        nsAutoString ext;
        mDefault.Right(ext, mDefault.Length() - extIndex);
        // Should we test for ".cgi", ".asp", ".jsp" and other
        // "generated" html pages?

        if ( ext.EqualsIgnoreCase(".htm")  ||
             ext.EqualsIgnoreCase(".html") ||
             ext.EqualsIgnoreCase(".shtml") ) {
          // This is supposed to append ".htm" if user doesn't supply an extension
          //XXX Actually, behavior is sort of weird:
          //    often appends ".html" even if you have an extension
          //    It obeys your extension if you put quotes around name
          ofn.lpstrDefExt = htmExt.get();
        }
      }
    }

    if (mMode == modeOpen) {
      // FILE MUST EXIST!
      ofn.Flags |= OFN_FILEMUSTEXIST;
      result = ::GetOpenFileNameW(&ofn);
    }
    else if (mMode == modeSave) {
      ofn.Flags |= OFN_NOREADONLYRETURN;
      result = ::GetSaveFileNameW(&ofn);
      if (!result) {
        // Error, find out what kind.
        if (::GetLastError() == ERROR_INVALID_PARAMETER ||
            ::CommDlgExtendedError() == FNERR_INVALIDFILENAME) {
          // probably the default file name is too long or contains illegal characters!
          // Try again, without a starting file name.
          ofn.lpstrFile[0] = 0;
          result = ::GetSaveFileNameW(&ofn);
        }
      }
    }
    else {
      NS_ASSERTION(0, "Only load, save and getFolder are supported modes"); 
    }
  
    // Remember what filter type the user selected
    mSelectedType = (PRInt16)ofn.nFilterIndex;

    // Set user-selected location of file or directory
    if (result == PR_TRUE) {
      // I think it also needs a conversion here (to unicode since appending to nsString) 
      // but doing that generates garbage file name, weird.
      mUnicodeFile.Append(fileBuffer);
    }

  }

  if (title)
    nsMemory::Free( title );

  if (result) {
    PRInt16 returnOKorReplace = returnOK;

    // Remember last used directory.
    nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    // work around.  InitWithPath() will convert UCS2 to FS path !!!  corrupts unicode 
    file->InitWithPath(mUnicodeFile);
    nsCOMPtr<nsIFile> dir;
    if (NS_SUCCEEDED(file->GetParent(getter_AddRefs(dir)))) {
      nsCOMPtr<nsILocalFile> localDir(do_QueryInterface(dir));
      if (localDir) {
        nsAutoString newDir;
        localDir->GetPath(newDir);
        if(!newDir.IsEmpty())
          mLastUsedUnicodeDirectory.Assign(newDir);
        // Update mDisplayDirectory with this directory, also.
        // Some callers rely on this.
        mDisplayDirectory->InitWithPath(mLastUsedUnicodeDirectory);
      }
    }

    if (mMode == modeSave) {
      // Windows does not return resultReplace,
      //   we must check if file already exists
      PRBool exists = PR_FALSE;
      file->Exists(&exists);
      if (exists)
        returnOKorReplace = returnReplace;
    }
    *aReturnVal = returnOKorReplace;
  }
  else {
    *aReturnVal = returnCancel;
  }
  return NS_OK;
}
#endif

NS_IMETHODIMP nsFilePicker::Show(PRInt16 *aReturnVal)
#ifdef MOZ_UNICODE
{
  return ShowW(aReturnVal);
}
#else
{
  NS_ENSURE_ARG_POINTER(aReturnVal);

  PRBool result = PR_FALSE;
#if !defined(UNICODE)
  char fileBuffer[MAX_PATH+1] = "";
  char *converted = ConvertToFileSystemCharset(mDefault.get());
  if (nsnull == converted) {
    mDefault.ToCString(fileBuffer,MAX_PATH);
  }
  else {
    PL_strncpyz(fileBuffer, converted, MAX_PATH+1);
    nsMemory::Free( converted );
  }
#endif /* UNICODE */

  TCHAR htmExt[] = _T("html");

  char *title = ConvertToFileSystemCharset(mTitle.get());
  if (nsnull == title)
    title = ToNewCString(mTitle);
  nsCAutoString initialDir;
  mDisplayDirectory->GetNativePath(initialDir);
  // If no display directory, re-use the last one.
  if(initialDir.IsEmpty()) {
    // Allocate copy of last used dir.
    initialDir = mLastUsedDirectory;
  }

  mFile.SetLength(0);

  if (mMode == modeGetFolder) {

    char dirBuffer[MAX_PATH+1];
    PL_strncpy(dirBuffer, initialDir.get(), MAX_PATH);
    BROWSEINFO browserInfo;
    browserInfo.hwndOwner      = mWnd;
    browserInfo.pidlRoot       = nsnull;
    browserInfo.pszDisplayName = (LPSTR)dirBuffer;
    browserInfo.lpszTitle      = title;
    browserInfo.ulFlags        = BIF_RETURNONLYFSDIRS;//BIF_STATUSTEXT | BIF_RETURNONLYFSDIRS;
    browserInfo.lpfn           = nsnull;
    browserInfo.lParam         = nsnull;
    browserInfo.iImage         = nsnull;

    // XXX UNICODE support is needed here --> DONE
    LPITEMIDLIST list = ::SHBrowseForFolder(&browserInfo);
    if (list != NULL) {
      result = ::SHGetPathFromIDList(list,
#if !defined(UNICODE)
          (LPTSTR)fileBuffer
#else
          (LPTSTR)mDefault.get()
#endif
          );
      if (result) {
#if !defined(UNICODE)
          mFile.Append(fileBuffer);
#else
          mFile.AppendWithConversion(mDefault.get());
#endif
      }
  
      // free PIDL
      LPMALLOC pMalloc = NULL;
      ::SHGetMalloc(&pMalloc);
      if(pMalloc) {
         pMalloc->Free(list);
         pMalloc->Release();
      }
    }
  }
  else {

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    char extensionBuffer[MAX_EXTENSION_LENGTH+1] = "";
    
#if !defined(UNICODE)
    PRInt32 l = (mFilterList.Length()+2)*2;
    char *filterBuffer = (char*) nsMemory::Alloc(l);
    int len = WideCharToMultiByte(CP_ACP, 0,
                                  mFilterList.get(),
                                  mFilterList.Length(),
                                  filterBuffer,
                                  l, NULL, NULL);
    filterBuffer[len] = NULL;
    filterBuffer[len+1] = NULL;
#endif
                                  
#if !defined(UNICODE)
    if (!initialDir.IsEmpty()) {
      ofn.lpstrInitialDir = initialDir.get();
    }
#else
    TCHAR winitialDir[MAX_PATH];
    if (!initialDir.IsEmpty()) {
      if(MultiByteToWideChar(CP_ACP, 0, initialDir.get(), -1, winitialDir, sizeof(winitialDir) / sizeof(TCHAR)))
        ofn.lpstrInitialDir = winitialDir;
    }
#endif
    
#if !defined(UNICODE)
    ofn.lpstrTitle   = title;
    ofn.lpstrFilter  = filterBuffer;
#else
    ofn.lpstrTitle = mTitle.get();
    ofn.lpstrFilter = mFilterList.get();
#endif
    ofn.nFilterIndex = mSelectedType;
    ofn.hwndOwner    = mWnd;
#if !defined(UNICODE)
    ofn.lpstrFile    = fileBuffer;
#else
    WCHAR wfileBuffer[MAX_PATH + 1];
    wcscpy(wfileBuffer, mDefault.get());
    ofn.lpstrFile = wfileBuffer;
#endif
    ofn.nMaxFile     = MAX_PATH;

    ofn.Flags = OFN_NOCHANGEDIR | OFN_SHAREAWARE | OFN_LONGNAMES | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    if (!mDefaultExtension.IsEmpty()) {
      // Someone was cool and told us what to do
      char *convertedExt = ConvertToFileSystemCharset(mDefaultExtension.get());
      if (!convertedExt) {
        mDefaultExtension.ToCString(extensionBuffer, MAX_EXTENSION_LENGTH);
      }
      else {
        PL_strncpyz(extensionBuffer, convertedExt, MAX_EXTENSION_LENGTH+1);
        nsMemory::Free( convertedExt );
      }
#if !defined(UNICODE)
      ofn.lpstrDefExt = extensionBuffer;
#else
      TCHAR wextensionBuffer[sizeof(extensionBuffer)];
      MultiByteToWideChar(CP_ACP, 0, extensionBuffer, -1, wextensionBuffer, sizeof(wextensionBuffer) / sizeof(TCHAR));
      ofn.lpstrDefExt = wextensionBuffer;
#endif
    }
    else {
      // Get file extension from suggested filename
      //  to detect if we are saving an html file
      //XXX: nsIFile SHOULD HAVE A GetExtension() METHOD!
      PRInt32 extIndex = mDefault.RFind(".");
      if ( extIndex >= 0) {
        nsAutoString ext;
        mDefault.Right(ext, mDefault.Length() - extIndex);
        // Should we test for ".cgi", ".asp", ".jsp" and other
        // "generated" html pages?

        if ( ext.EqualsIgnoreCase(".htm")  ||
             ext.EqualsIgnoreCase(".html") ||
             ext.EqualsIgnoreCase(".shtml") ) {
          // This is supposed to append ".htm" if user doesn't supply an extension
          //XXX Actually, behavior is sort of weird:
          //    often appends ".html" even if you have an extension
          //    It obeys your extension if you put quotes around name
          ofn.lpstrDefExt = htmExt;
        }
      }
    }

    if (mMode == modeOpen) {
      // FILE MUST EXIST!
      ofn.Flags |= OFN_FILEMUSTEXIST;
      result = ::GetOpenFileName(&ofn);
    }
    else if (mMode == modeSave) {
      ofn.Flags |= OFN_NOREADONLYRETURN;
      result = ::GetSaveFileName(&ofn);
      if (!result) {
        // Error, find out what kind.
        if (::GetLastError() == ERROR_INVALID_PARAMETER
#if defined(FNERR_INVALIDFILENAME)
            || ::CommDlgExtendedError() == FNERR_INVALIDFILENAME
#endif
            ) {
          // probably the default file name is too long or contains illegal characters!
          // Try again, without a starting file name.
          ofn.lpstrFile[0] = 0;
          result = ::GetSaveFileName(&ofn);
        }
      }
    }
    else {
      NS_ASSERTION(0, "Only load, save and getFolder are supported modes"); 
    }
  
    // Remember what filter type the user selected
    mSelectedType = (PRInt16)ofn.nFilterIndex;

#if !defined(UNICODE)
    // Clean up filter buffers
    if (filterBuffer)
      nsMemory::Free( filterBuffer );
#endif

    // Set user-selected location of file or directory
    if (result == PR_TRUE) {
      // I think it also needs a conversion here (to unicode since appending to nsString) 
      // but doing that generates garbage file name, weird.
#if !defined(UNICODE)
      mFile.Append(fileBuffer);
#else
      mFile.AppendWithConversion(wfileBuffer);
#endif
    }

  }

  if (title)
    nsMemory::Free( title );

  if (result) {
    PRInt16 returnOKorReplace = returnOK;

    // Remember last used directory.
    nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    file->InitWithNativePath(mFile);
    nsCOMPtr<nsIFile> dir;
    if (NS_SUCCEEDED(file->GetParent(getter_AddRefs(dir)))) {
      nsCOMPtr<nsILocalFile> localDir(do_QueryInterface(dir));
      if (localDir) {
        nsCAutoString newDir;
        localDir->GetNativePath(newDir);
        if(!newDir.IsEmpty())
          PL_strncpyz(mLastUsedDirectory, newDir.get(), MAX_PATH+1);
        // Update mDisplayDirectory with this directory, also.
        // Some callers rely on this.
        mDisplayDirectory->InitWithNativePath( nsDependentCString(mLastUsedDirectory) );
      }
    }

    if (mMode == modeSave) {
      // Windows does not return resultReplace,
      //   we must check if file already exists
      PRBool exists = PR_FALSE;
      file->Exists(&exists);
      if (exists)
        returnOKorReplace = returnReplace;
    }
    *aReturnVal = returnOKorReplace;
  }
  else {
    *aReturnVal = returnCancel;
  }
  return NS_OK;
}
#endif

NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);

#ifdef MOZ_UNICODE
  if (mUnicodeFile.IsEmpty())
      return NS_OK;
#else
  if (mFile.IsEmpty())
      return NS_OK;
#endif

  nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

#ifdef MOZ_UNICODE
  file->InitWithPath(mUnicodeFile);
#else
  file->InitWithNativePath(mFile);
#endif

  NS_ADDREF(*aFile = file);

  return NS_OK;
}

//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);
#ifdef MOZ_UNICODE
  file->InitWithPath(mUnicodeFile);
#else
  file->InitWithNativePath(mFile);
#endif

  nsCOMPtr<nsIURI> uri;
  NS_NewFileURI(getter_AddRefs(uri), file);
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

  //First, make sure the file name is not too long!
  PRInt32 nameLength;
  PRInt32 nameIndex = mDefault.RFind("\\");
  if (nameIndex == kNotFound)
    nameIndex = 0;
  else
    nameIndex ++;
  nameLength = mDefault.Length() - nameIndex;
  
  if (nameLength > _MAX_FNAME) {
    PRInt32 extIndex = mDefault.RFind(".");
    if (extIndex == kNotFound)
      extIndex = mDefault.Length();

    //Let's try to shave the needed characters from the name part
    PRInt32 charsToRemove = nameLength - _MAX_FNAME;
    if (extIndex - nameIndex >= charsToRemove) {
      mDefault.Cut(extIndex - charsToRemove, charsToRemove);
    }
  }

  //Then, we need to replace illegal characters.
  //At this stage, we cannot replace the backslash as the string might represent a file path.
  mDefault.ReplaceChar(FILE_ILLEGAL_CHARACTERS, '-');

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
  *aExtension = ToNewUnicode(mDefaultExtension);
  if (!*aExtension)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetDefaultExtension(const PRUnichar *aExtension)
{
  mDefaultExtension = aExtension;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Set the filter index
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  // Windows' filter index is 1-based, we use a 0-based system.
  *aFilterIndex = mSelectedType - 1;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  // Windows' filter index is 1-based, we use a 0-based system.
  mSelectedType = aFilterIndex + 1;
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



//-------------------------------------------------------------------------
NS_IMETHODIMP nsFilePicker::InitNative(nsIWidget *aParent,
                                       const PRUnichar *aTitle,
                                       PRInt16 aMode)
{
  mWnd = (HWND) ((aParent) ? aParent->GetNativeData(NS_NATIVE_WINDOW) : 0); 
  mTitle.SetLength(0);
  mTitle.Append(aTitle);
  mMode = aMode;
  return NS_OK;
}


//-------------------------------------------------------------------------
void nsFilePicker::GetFileSystemCharset(nsString & fileSystemCharset)
{
  static nsAutoString aCharset;
  nsresult rv;

  if (aCharset.Length() < 1) {
    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
	  if (NS_SUCCEEDED(rv)) 
		  rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, aCharset);

    NS_ASSERTION(NS_SUCCEEDED(rv), "error getting platform charset");
	  if (NS_FAILED(rv)) 
		  aCharset.Assign(NS_LITERAL_STRING("windows-1252"));
  }
  fileSystemCharset = aCharset;
}

//-------------------------------------------------------------------------
char * nsFilePicker::ConvertToFileSystemCharset(const PRUnichar *inString)
{
  char *outString = nsnull;
  nsresult rv = NS_OK;

  // get file system charset and create a unicode encoder
  if (nsnull == mUnicodeEncoder) {
    nsAutoString fileSystemCharset;
    GetFileSystemCharset(fileSystemCharset);

    nsCOMPtr<nsICharsetConverterManager> ccm = 
             do_GetService(kCharsetConverterManagerCID, &rv); 
    if (NS_SUCCEEDED(rv)) {
      rv = ccm->GetUnicodeEncoder(&fileSystemCharset, &mUnicodeEncoder);
    }
  }

  // converts from unicode to the file system charset
  if (NS_SUCCEEDED(rv)) {
    PRInt32 inLength = nsCRT::strlen(inString);
    PRInt32 outLength;
    rv = mUnicodeEncoder->GetMaxLength(inString, inLength, &outLength);
    if (NS_SUCCEEDED(rv)) {
      outString = NS_STATIC_CAST( char*, nsMemory::Alloc( outLength+1 ) );
      if (nsnull == outString) {
        return nsnull;
      }
      rv = mUnicodeEncoder->Convert(inString, &inLength, outString, &outLength);
      if (NS_SUCCEEDED(rv)) {
        outString[outLength] = '\0';
      }
    }
  }
  
  return NS_SUCCEEDED(rv) ? outString : nsnull;
}

//-------------------------------------------------------------------------
PRUnichar * nsFilePicker::ConvertFromFileSystemCharset(const char *inString)
{
  PRUnichar *outString = nsnull;
  nsresult rv = NS_OK;

  // get file system charset and create a unicode encoder
  if (nsnull == mUnicodeDecoder) {
    nsAutoString fileSystemCharset;
    GetFileSystemCharset(fileSystemCharset);

    nsCOMPtr<nsICharsetConverterManager> ccm = 
             do_GetService(kCharsetConverterManagerCID, &rv); 
    if (NS_SUCCEEDED(rv)) {
      rv = ccm->GetUnicodeDecoder(&fileSystemCharset, &mUnicodeDecoder);
    }
  }

  // converts from the file system charset to unicode
  if (NS_SUCCEEDED(rv)) {
    PRInt32 inLength = strlen(inString);
    PRInt32 outLength;
    rv = mUnicodeDecoder->GetMaxLength(inString, inLength, &outLength);
    if (NS_SUCCEEDED(rv)) {
      outString = NS_STATIC_CAST( PRUnichar*, nsMemory::Alloc( (outLength+1) * sizeof( PRUnichar ) ) );
      if (nsnull == outString) {
        return nsnull;
      }
      rv = mUnicodeDecoder->Convert(inString, &inLength, outString, &outLength);
      if (NS_SUCCEEDED(rv)) {
        outString[outLength] = 0;
      }
    }
  }

  NS_ASSERTION(NS_SUCCEEDED(rv), "error charset conversion");
  return NS_SUCCEEDED(rv) ? outString : nsnull;
}


NS_IMETHODIMP
nsFilePicker::AppendFilter(const PRUnichar *aTitle, const PRUnichar *aFilter)
{
  mFilterList.Append(aTitle);
  mFilterList.Append(PRUnichar('\0'));

  if (!nsCRT::strcmp(aFilter, NS_LITERAL_STRING("..apps").get()))
    mFilterList.Append(NS_LITERAL_STRING("*.exe; *.com"));
  else
    mFilterList.Append(aFilter);

  mFilterList.Append(PRUnichar('\0'));

  return NS_OK;
}
