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
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsBasicStreamGenerator.h"

const char * nsBasicStreamGenerator::mSignature = "Basic Keyed Stream Generator";

NS_IMPL_ISUPPORTS1(nsBasicStreamGenerator, nsIKeyedStreamGenerator)

nsBasicStreamGenerator::nsBasicStreamGenerator()
    : mLevel(NS_SECURITY_LEVEL), mSalt(0), mPassword(), mState(0)
{
    NS_INIT_ISUPPORTS();
}

nsBasicStreamGenerator::~nsBasicStreamGenerator()
{
}


NS_IMETHODIMP nsBasicStreamGenerator::GetSignature(char **signature)
{
    NS_ENSURE_ARG_POINTER(signature);
    *signature = nsCRT::strdup(mSignature);
    return NS_OK;
}

NS_IMETHODIMP nsBasicStreamGenerator::Setup(PRUint32 salt, nsISupports *consumer)
{
    nsresult rv = NS_OK;
    // Forget everything about previous setup
    mWeakPasswordSink = nsnull;
    // XXX whipe out the password to zero in memory
    mPassword.Truncate(0);

    // Reestablish setup
    if (consumer)
    {
        mWeakPasswordSink = NS_GetWeakReference(consumer, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    mSalt = salt;
    return NS_OK;
}

NS_IMETHODIMP nsBasicStreamGenerator::GetLevel(float *aLevel)
{
    NS_ENSURE_ARG_POINTER(aLevel);
    *aLevel = mLevel;
    return NS_OK;
}

NS_IMETHODIMP nsBasicStreamGenerator::GetByte(PRUint32 offset, PRUint8 *retval)
{
    NS_ENSURE_ARG_POINTER(retval);
    nsresult rv = NS_OK;
    if (mPassword.Length() == 0)
    {
        // First time we need the password. Get it.
        nsCOMPtr<nsIPasswordSink> weakPasswordSink = do_QueryReferent(mWeakPasswordSink);
        if (!weakPasswordSink) return NS_ERROR_FAILURE;
        PRUnichar *aPassword;
        rv = weakPasswordSink->GetPassword(&aPassword);
        if (NS_FAILED(rv)) return rv;
        mPassword = aPassword;
        nsAllocator::Free(aPassword);
        mState = 0;
    }

    // Get the offset byte from the stream. Our stream is just our password
    // repeating itself infinite times.
    //
    // mPassword being a nsString, its elements are PRUnichars.
    // We want to return either the high-order or low-order 8 bits of the PRUnichar
    // depending on whether or not this routine was called an odd or an even number of times
    PRUnichar ret16 = mPassword.CharAt((mState>>1) % mPassword.Length());
    if ((mState++) & 1) {
      ret16 = ret16>>8;
    }
    *retval = (PRUint8)(ret16 & 0xff);
    return rv;
}
