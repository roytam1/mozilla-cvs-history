/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsInputStreamChannel.h"
#include "nsIStreamListener.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"

////////////////////////////////////////////////////////////////////////////////
// nsInputStreamChannel methods:

nsInputStreamChannel::nsInputStreamChannel()
    : mURI(nsnull), mContentType(nsnull), mInputStream(nsnull), mLoadGroup(nsnull)
{
    NS_INIT_REFCNT(); 
}

nsInputStreamChannel::~nsInputStreamChannel() {
    NS_IF_RELEASE(mURI);
    if (mContentType) nsCRT::free(mContentType);
    NS_IF_RELEASE(mInputStream);
}

NS_METHOD
nsInputStreamChannel::Create(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult)
{
    nsInputStreamChannel* about = new nsInputStreamChannel();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}

nsresult
nsInputStreamChannel::Init(nsIURI* uri, const char* contentType,
                           nsIInputStream* in)
{
    mURI = uri;
    NS_IF_ADDREF(mURI);
    mContentType = nsCRT::strdup(contentType);
    if (mContentType == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    mInputStream = in;
    NS_IF_ADDREF(mInputStream);
    return NS_OK;
}

NS_IMPL_ISUPPORTS(nsInputStreamChannel, nsCOMTypeInfo<nsIChannel>::GetIID());

////////////////////////////////////////////////////////////////////////////////
// nsIRequest methods:

NS_IMETHODIMP
nsInputStreamChannel::IsPending(PRBool *result)
{
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Cancel(void)
{
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Suspend(void)
{
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::Resume(void)
{
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsIChannel methods:

NS_IMETHODIMP
nsInputStreamChannel::GetURI(nsIURI * *aURI)
{
    *aURI = mURI;
    NS_IF_ADDREF(mURI);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount,
                                      nsIInputStream **result)
{
    // if we had seekable streams, we could seek here:
    NS_ASSERTION(startPosition == 0, "Can't seek in nsInputStreamChannel");
    *result = mInputStream;
    NS_ADDREF(mInputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream **_retval)
{
    // we don't do output
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInputStreamChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                                nsISupports *ctxt, nsIStreamListener *listener,
                                nsILoadGroup* group)
{
    // currently this happens before AsyncRead returns -- hope that's ok
    nsresult rv;

    // Do an extra AddRef so that this method's synchronous operation doesn't end up destroying
    // the listener prematurely.
    nsCOMPtr<nsIStreamListener> l(listener);

    NS_ASSERTION(mLoadGroup == nsnull, "recursively entered AsyncRead?");
    mLoadGroup = group;
    NS_ADDREF(mLoadGroup);

    if (group) {
        (void)group->AddChannel(this, ctxt);
    }

    rv = listener->OnStartRequest(this, ctxt);
    if (NS_FAILED(rv)) return rv;

    PRUint32 amt;
    while (PR_TRUE) {
        rv = mInputStream->GetLength(&amt);
        if (rv == NS_BASE_STREAM_EOF) {
            rv = NS_OK;
            break;
        }
        if (NS_FAILED(rv)) break;
        if (readCount != -1)
            amt = PR_MIN((PRUint32)readCount, amt);
        if (amt == 0) 
            break;
        rv = listener->OnDataAvailable(this, ctxt, mInputStream, 0, amt);
        if (NS_FAILED(rv)) break;
    }

    rv = listener->OnStopRequest(this, ctxt, rv, nsnull);       // XXX error message 

    if (group) {
        (void)group->RemoveChannel(this, ctxt, rv, nsnull);     // XXX error message 
    }
    NS_RELEASE(mLoadGroup);
    
    return rv;
}

NS_IMETHODIMP
nsInputStreamChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition,
                      PRInt32 writeCount, nsISupports *ctxt,
                      nsIStreamObserver *observer, nsILoadGroup* group)
{
    // we don't do output
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsInputStreamChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    *aLoadAttributes = LOAD_NORMAL;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    // ignore attempts to set load attributes
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetContentType(char * *aContentType)
{
    *aContentType = nsCRT::strdup("text/html");
    return *aContentType ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsInputStreamChannel::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  *aLoadGroup = mLoadGroup;
  NS_ADDREF(*aLoadGroup);
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
