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

#ifndef nsIStreamListener_h___
#define nsIStreamListener_h___

#include "nsISupports.h"
#include "nscore.h"
#include "plevent.h"

class nsIUrl;
class nsIInputStream;

// XXX regenerate:
#define NS_ISTREAMLISTENER_IID                       \
{ /* 677d9a90-93ee-11d2-816a-006008119d7a */         \
    0x677d9a90,                                      \
    0x93ee,                                          \
    0x11d2,                                          \
    {0x81, 0x6a, 0x00, 0x60, 0x09, 0x11, 0x9d, 0x7a} \
}

struct nsStreamBindingInfo {
    PRBool      seekable;
    /* ...more... */
};

class nsIStreamListener : public nsISupports
{
public:
    NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISTREAMLISTENER_IID);

    /**
     * Notify the listener that the URL has started to load.  This method is
     * called only once, at the beginning of a URL load.<BR><BR>
     *
     * @return The return value is currently ignored.  In the future it may be
     * used to cancel the URL load..
     */
    NS_IMETHOD OnStartBinding(nsIUrl* aUrl, const char *aContentType) = 0;

    /**
     * Notify the client that data is available in the input stream.  This
     * method is called whenver data is written into the input stream by the
     * networking library...<BR><BR>
     * 
     * @param pIStream  The input stream containing the data.  This stream can
     * be either a blocking or non-blocking stream.
     * @param length    The amount of data that was just pushed into the stream.
     * @return The return value is currently ignored.
     */
    NS_IMETHOD OnDataAvailable(nsIUrl* aUrl, nsIInputStream *aIStream, 
                               PRUint32 aLength) = 0;

    /**
     * Notify the listener that the URL has finished loading.  This method is 
     * called once when the networking library has finished processing the 
     * URL transaction initiatied via the nsINetService::Open(...) call.<BR><BR>
     * 
     * This method is called regardless of whether the URL loaded successfully.<BR><BR>
     * 
     * @param status    Status code for the URL load.
     * @param msg   A text string describing the error.
     * @return The return value is currently ignored.
     */
    NS_IMETHOD OnStopBinding(nsIUrl* aUrl, nsresult aStatus, const PRUnichar* aMsg) = 0;

    /**
     * Return information regarding the current URL load.<BR>
     * The info structure that is passed in is filled out and returned
     * to the caller. 
     * 
     * This method is currently not called.  
     */
    NS_IMETHOD GetBindInfo(nsIUrl* aUrl, nsStreamBindingInfo* aInfo) = 0;

};

/* Generic status codes for OnStopBinding */
#define NS_BINDING_SUCCEEDED    NS_OK
#define NS_BINDING_FAILED       NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 1)
#define NS_BINDING_ABORTED      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 2)

extern nsresult
NS_NewMarshalingStreamListener(PLEventQueue* eventQueue,
                               nsIStreamListener* receiver,
                               nsIStreamListener* *result);

#endif /* nsIIStreamListener_h___ */
