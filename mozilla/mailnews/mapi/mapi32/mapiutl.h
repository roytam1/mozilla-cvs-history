/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifndef __UTILS_
#define __UTILS_
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MAPI_OLE	// Because MSFT doesn't do this for us :-(
#include <mapi.h>
#endif

//
// Utility functions...
//
void    SetLoggingEnabled(BOOL val);    // Set a logging enabled flag
void    LogString(LPCSTR pStr1);        // Log a string to a file...
void    BuildMemName(LPSTR name, ULONG winSeed);  // Shared memory name
HWND    GetCommunicatorIPCWindow(void);    // Get the IPC window we will use...
DWORD   SanityCheckAttachmentFiles(lpMapiMessage lpMessage); // Check attachments
DWORD   ValidateFile(LPCSTR szFile);       // Is this a valid file - 0=Yes 1 = NOT_FOUND 2 = OPEN_FAILURE

DWORD   GetFileCount(LPSTR pFiles, LPSTR delimChar); // Get File count from string of file1;file2, etc..
BOOL    ExtractFile(LPSTR pFiles, LPSTR delimChar, DWORD fIndex, LPSTR fName); // Extract a filename from a string

LPVOID  LoadBlobToMemory(LPSTR fName);  // Load the blob into memory!
LONG    GetTempAttachmentName(LPSTR fName); // Get a temp file name and put it in fName

UINT    GetTempMailNameWithExtension(LPSTR szTempFileName, LPSTR origName);
void    CleanupMAPITempFiles(void);
void  AddTempFile(LPCSTR pFileName);

void    *CleanMalloc(size_t mallocSize);
void    SafeFree(void *ptr);

//
// RICHIE - this is a temporary fix for now to get rid of
// html stuff within the text of a message - if there was a
// valid noteText buffer coming into this call, we need to 
// free it on the way out.
//
LPSTR   StripHTML(LPSTR noteText);

//
// Write a buffer to disk
// Return 0 on success -1 on failure
//
LONG    WriteMemoryBufferToDisk(LPSTR fName, LONG bufSize, LPSTR buf); 

#ifdef __cplusplus
}
#endif

#endif // __UTILS_
