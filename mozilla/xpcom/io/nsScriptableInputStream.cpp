/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * Contributor(s): Jud Valeski & Brendan Eich
 */

#include "nsScriptableInputStream.h"
#include "nsIAllocator.h"

NS_IMPL_ISUPPORTS2(nsScriptableInputStream, nsIBaseStream, nsIScriptableInputStream);

// nsIBaseStream methods
NS_IMETHODIMP
nsScriptableInputStream::Close(void) {
    if (mInputStream) return NS_ERROR_NOT_INITIALIZED;
    return mInputStream->Close();
}

// nsIScriptableInputStream methods
NS_IMETHODIMP
nsScriptableInputStream::Init(nsIInputStream *aInputStream) {
    if (!aInputStream) return NS_ERROR_NULL_POINTER;
    mInputStream = aInputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptableInputStream::Available(PRUint32 *_retval) {
    if (mInputStream) return NS_ERROR_NOT_INITIALIZED;
    return mInputStream->Available(_retval);
}

NS_IMETHODIMP
nsScriptableInputStream::Read(PRUint32 aCount, char **_retval) {
    nsresult rv = NS_OK;
    PRUint32 count = 0;
    char *buffer = nsnull;

    if (!mInputStream) return NS_ERROR_NOT_INITIALIZED;

    rv = mInputStream->Available(&count);
    if (NS_FAILED(rv)) return rv;

    count = PR_MIN(count, aCount);
    buffer = (char*)nsAllocator::Alloc(count+1); // make room for '\0'
    if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 amtRead = 0;
    rv = mInputStream->Read(buffer, count, &amtRead);
    if (NS_FAILED(rv)) {
        nsAllocator::Free(buffer);
        return rv;
    }

    buffer[amtRead] = '\0';
    *_retval = buffer;
    return NS_OK;
}

NS_METHOD
nsScriptableInputStream::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
    if (aOuter) return NS_ERROR_NO_AGGREGATION;

    nsScriptableInputStream *sis = new nsScriptableInputStream();
    if (!sis) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(sis);
    nsresult rv = sis->QueryInterface(aIID, aResult);
    NS_RELEASE(sis);
    return rv;
}
