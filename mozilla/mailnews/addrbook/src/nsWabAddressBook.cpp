/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Initial Developer of the Original Code is Sun
 * Microsystems, Inc.  Portions created by Sun are
 * Copyright (C) 2001 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Created by Cyrille Moureaux <Cyrille.Moureaux@sun.com>
 */
#include "nsWabAddressBook.h"
#include "nsAbUtils.h"
#include "nsAutoLock.h"
#include "nslog.h"

NS_IMPL_LOG(nsWabAddressBookLog)

#define PRINTF NS_LOG_PRINTF(nsWabAddressBookLog)
#define FLUSH  NS_LOG_FLUSH(nsWabAddressBookLog)

HMODULE nsWabAddressBook::mLibrary = NULL ;
PRInt32 nsWabAddressBook::mLibUsage = 0 ;
LPWABOPEN nsWabAddressBook::mWABOpen = NULL ;
LPWABOBJECT nsWabAddressBook::mRootSession = NULL ;
LPADRBOOK nsWabAddressBook::mRootBook = NULL ;

BOOL nsWabAddressBook::LoadWabLibrary(void)
{
    if (mLibrary) { ++ mLibUsage ; return TRUE ; }
    // We try to fetch the location of the WAB DLL from the registry
    TCHAR wabDLLPath [MAX_PATH] ;
    DWORD keyType = 0 ;
    ULONG byteCount = sizeof(wabDLLPath) ;
    HKEY keyHandle = NULL ;
    
    wabDLLPath [MAX_PATH - 1] = 0 ;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &keyHandle) == ERROR_SUCCESS) {
        RegQueryValueEx(keyHandle, "", NULL, &keyType, (LPBYTE) wabDLLPath, &byteCount) ;
    }
    if (keyHandle) { RegCloseKey(keyHandle) ; }
    mLibrary = LoadLibrary( (lstrlen(wabDLLPath)) ? wabDLLPath : WAB_DLL_NAME );
    if (!mLibrary) { return FALSE ; }
    ++ mLibUsage ;
    mWABOpen = NS_REINTERPRET_CAST(LPWABOPEN, GetProcAddress(mLibrary, "WABOpen")) ;
    if (!mWABOpen) { return FALSE ; }
    HRESULT retCode = mWABOpen(&mRootBook, &mRootSession, NULL, 0) ;

    if (HR_FAILED(retCode)) {
        PRINTF("Cannot initialize WAB %08x.\n", retCode) ; return FALSE ;
    }
    return TRUE ;
}

void nsWabAddressBook::FreeWabLibrary(void)
{
    if (mLibrary) {
        if (-- mLibUsage == 0) {
            if (mRootBook) { mRootBook->Release() ; }
            if (mRootSession) { mRootSession->Release() ; }
            FreeLibrary(mLibrary) ;
            mLibrary = NULL ;
        }
    }
}

MOZ_DECL_CTOR_COUNTER(nsWabAddressBook)

nsWabAddressBook::nsWabAddressBook(void)
: nsAbWinHelper()
{
    BOOL result = Initialize() ;

    NS_ASSERTION(result == TRUE, "Couldn't initialize Wab Helper") ;
    MOZ_COUNT_CTOR(nsWabAddressBook) ;
}

nsWabAddressBook::~nsWabAddressBook(void)
{
    nsAutoLock guard(mMutex) ;
    FreeWabLibrary() ;
    MOZ_COUNT_DTOR(nsWabAddressBook) ;
}

BOOL nsWabAddressBook::Initialize(void)
{
    if (mAddressBook) { return TRUE ; }
    nsAutoLock guard(mMutex) ;

    if (!LoadWabLibrary()) {
        PRINTF("Cannot load library.\n") ;
        return FALSE ;
    }
    mAddressBook = mRootBook ;
    return TRUE ;
}

void nsWabAddressBook::AllocateBuffer(ULONG aByteCount, LPVOID *aBuffer)
{
    mRootSession->AllocateBuffer(aByteCount, aBuffer) ;
}

void nsWabAddressBook::FreeBuffer(LPVOID aBuffer)
{
    mRootSession->FreeBuffer(aBuffer) ;
}






