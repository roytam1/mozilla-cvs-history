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

#include "msgCore.h"
#include "prprf.h"

#include <stdio.h>
#include <assert.h>

#include "nsIStreamListener.h"
#include "nsIInputStream.h"

///////////////////////////////////////////////////////////////////////////
// This is a stub of a mailbox parser used to test the mailbox url code
// for parsing a mailbox. Eventually bienvenu will hook up a real mail box
// parser and we won't need this stub..
// 
// Right now, a mailbox parser supports the nsIStreamListener interface
////////////////////////////////////////////////////////////////////////////

class nsMsgMailboxParserStub : nsIStreamListener
{

public:
    NS_DECL_ISUPPORTS

    nsMsgMailboxParserStub();

	// nsIStreamListener interfaces:
    NS_IMETHOD GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* info);
    NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 Progress, PRUint32 ProgressMax);
    NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg);
    NS_IMETHOD OnStartRequest(nsIURI* aURL, const char *aContentType);
    NS_IMETHOD OnDataAvailable(nsIURI* aURL, nsIInputStream *pIStream, PRUint32 length);
    NS_IMETHOD OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg);

protected:
    ~nsMsgMailboxParserStub();

};

nsMsgMailboxParserStub::nsMsgMailboxParserStub()
{
	NS_INIT_REFCNT();
}

nsMsgMailboxParserStub::~nsMsgMailboxParserStub()
{}

NS_IMPL_ISUPPORTS(nsMsgMailboxParserStub,NS_GET_IID(nsIStreamListener));

NS_IMETHODIMP nsMsgMailboxParserStub::GetBindInfo(nsIURI* aURL, nsStreamBindingInfo* info)
{
    printf("\n+++ nsMsgMailboxParserStub::GetBindInfo: URL: %p\n", aURL);
    return 0;
}

NS_IMETHODIMP nsMsgMailboxParserStub::OnProgress(nsIURI* aURL, PRUint32 Progress, 
                                       PRUint32 ProgressMax)
{
    printf("\n+++ nsMsgMailboxParserStub::OnProgress: URL: %p - %d of total %d\n", aURL, Progress, ProgressMax);
    return 0;
}

NS_IMETHODIMP nsMsgMailboxParserStub::OnStatus(nsIURI* aURL, const PRUnichar* aMsg)
{
	printf("\n+++ nsMsgMailboxParserStub::OnStatus: ");
    nsString str(aMsg);
    fputs(nsCAutoString(str), stdout);
    fputs("\n", stdout);
    
    return 0;
}

NS_IMETHODIMP nsMsgMailboxParserStub::OnStartRequest(nsIURI* aURL, const char *aContentType)
{
	printf("\n+++ nsMsgMailboxParserStub::OnStartRequest: URL: %p, Content type: %s\n", aURL, aContentType);
    return 0;
}


NS_IMETHODIMP nsMsgMailboxParserStub::OnDataAvailable(nsIURI* aURL, nsIInputStream *pIStream, PRUint32 length) 
{
    PRUint32 totalBytesRead = 0;
    PRUint32 len = 0;
	// mscott - for small mailboxes, it might be useful to print the data we get back out to the screen.
	// however, as our test files get bigger, that would be information overload and we'd really just like
	// to see (or count) the number of times OnDataAvailable has been called.

    printf("\n+++ nsMsgMailboxParserStub::OnDataAvailable: URL: %p, %d bytes available...\n", aURL, length);

    do {

        nsresult err;
        char buffer[1000];

		PRUint32 numToRead = length > 1000 ? 1000 : length;
        err = pIStream->Read(buffer, numToRead, &len);
		totalBytesRead += len;
#if 0 
        if (err == NS_OK) {
            PRUint32 i = 0;
            for (i=0; i<len; i++) {
                putchar(buffer[i]);
            }
        }
#endif
    } while (totalBytesRead < length && (len > 0) );

    return 0;
}


NS_IMETHODIMP nsMsgMailboxParserStub::OnStopRequest(nsIURI* aURL, nsresult status, const PRUnichar* aMsg)
{
	// on stop binding is called by the mailbox protocol when there is no more data forthcoming...

    printf("\n+++ nsMsgMailboxParserStub::OnStopRequest... URL: %p status: %d\n", aURL, status);

    /* The document has been loaded, so drop out of the message pump... */
    return 0;
}

NS_BEGIN_EXTERN_C

nsresult NS_NewMsgParser(nsIStreamListener ** aInstancePtr)
{
	nsresult rv = NS_OK;
	if (aInstancePtr)
	{
		nsMsgMailboxParserStub * parser = new nsMsgMailboxParserStub();
		if (parser)
			rv =parser->QueryInterface(NS_GET_IID(nsIStreamListener), (void **) aInstancePtr);		
	}

	return rv;
}

NS_END_EXTERN_C

