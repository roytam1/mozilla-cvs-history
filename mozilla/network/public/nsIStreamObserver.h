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

#ifndef nsIStreamObserver_h___
#define nsIStreamObserver_h___

#include "nsISupports.h"
#include "nscore.h"

/* forward declaration */
class nsIInputStream;
class nsString;
class nsIURI;


/* 97566110-ff60-11d1-beb9-00805f8a66dc */
#define NS_ISTREAMOBSERVER_IID   \
{ 0x97566110, 0xff60, 0x11d1, \
  {0xbe, 0xb9, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }


class nsIStreamObserver : public nsISupports {
public:
    static const nsIID& GetIID() { static nsIID iid = NS_ISTREAMOBSERVER_IID; return iid; }

    /**
     * Notify the observer that the URL has started to load.  This method is
     * called only once, at the beginning of a URL load.<BR><BR>
     *
     * @return The return value is currently ignored.  In the future it may be
     * used to cancel the URL load..
     */
    NS_IMETHOD OnStartRequest(nsIURI* aURL, const char *aContentType) = 0;

    /**
     * Notify the observer that progress as occurred for the URL load.<BR>
     */
    NS_IMETHOD OnProgress(nsIURI* aURL, PRUint32 aProgress, PRUint32 aProgressMax) = 0;

    /**
     * Notify the observer with a status message for the URL load.<BR>
     */
    NS_IMETHOD OnStatus(nsIURI* aURL, const PRUnichar* aMsg) = 0;

    /**
     * Notify the observer that the URL has finished loading.  This method is 
     * called once when the networking library has finished processing the 
     * URL transaction initiatied via the nsINetService::Open(...) call.<BR><BR>
     * 
     * This method is called regardless of whether the URL loaded successfully.<BR><BR>
     * 
     * @param status    Status code for the URL load.
     * @param msg   A text string describing the error.
     * @return The return value is currently ignored.
     */
    NS_IMETHOD OnStopRequest(nsIURI* aURL, nsresult aStatus, const PRUnichar* aMsg) = 0;
};

/* Generic status codes for OnStopRequest */
#define NS_BINDING_SUCCEEDED    NS_OK
#define NS_BINDING_FAILED       NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 1)
#define NS_BINDING_ABORTED      NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 2)

#endif /* nsIStreamObserver_h___ */
