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

#ifndef _nsHTTPChannel_h_
#define _nsHTTPChannel_h_

#include "nsIHTTPChannel.h"
#include "nsIChannel.h"
#include "nsHTTPEnums.h"
#include "nsIURI.h"
#include "nsHTTPHandler.h"
#include "nsIEventQueue.h"
#include "nsIHttpEventSink.h"

class nsHTTPRequest;
class nsHTTPResponse;
/* 
    The nsHTTPChannel class is an example implementation of a
    protocol instnce that is active on a per-URL basis.

    Currently this is being built with the Netlib dll. But after
    the registration stuff that DP is working on gets completed 
    this will move to the HTTP lib.

    -Gagan Saksena 02/25/99
*/
class nsHTTPChannel : public nsIHTTPChannel
{

public:

    // Constructors and Destructor
    nsHTTPChannel(nsIURI* i_URL, 
                  nsIEventQueue* i_EQ, 
                  nsIHTTPEventSink* i_HTTPEventSink,
                  nsIHTTPHandler* i_Handler);

    ~nsHTTPChannel();

    // Functions from nsISupports
    NS_DECL_ISUPPORTS;

    // nsIChannel methods:
    NS_IMETHOD GetURI(nsIURI * *aURL);
    NS_IMETHOD OpenInputStream(nsIInputStream **result);
    NS_IMETHOD OpenOutputStream(nsIOutputStream **result);
    NS_IMETHOD AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                         nsISupports *ctxt,
                         nsIEventQueue *eventQueue,
                         nsIStreamListener *listener);
    NS_IMETHOD AsyncWrite(nsIInputStream *fromStream,
                          PRUint32 startPosition,
                          PRInt32 writeCount,
                          nsISupports *ctxt,
                          nsIEventQueue *eventQueue,
                          nsIStreamObserver *observer);
    NS_IMETHOD Cancel();
    NS_IMETHOD Suspend();
    NS_IMETHOD Resume();

    // nsIHTTPChannel methods:
    NS_IMETHOD GetRequestHeader(const char *headerName, char **_retval);
    NS_IMETHOD SetRequestHeader(const char *headerName, const char *value);
    NS_IMETHOD SetRequestMethod(PRUint32 method);
    NS_IMETHOD GetResponseHeader(const char *headerName, char **_retval);
    NS_IMETHOD GetResponseStatus(nsresult *aResponseStatus);
    NS_IMETHOD GetResponseString(char * *aResponseString);
    NS_IMETHOD GetEventSink(nsIHTTPEventSink* *eventSink);
    NS_IMETHOD GetResponseDataListener(nsIStreamListener* *aListener);

    // nsHTTPChannel methods:
    nsresult            Init();
    nsresult            Open();
    nsIEventQueue*      EventQueue(void) const { return m_pEventQ; };
    nsresult            SetResponse(nsHTTPResponse* i_pResp);

protected:
    nsCOMPtr<nsIURI>            m_URI;
    PRBool                      m_bConnected; 
    nsCOMPtr<nsIHTTPHandler>    m_pHandler;
    HTTPState                   m_State;
    nsCOMPtr<nsIEventQueue>     m_pEventQ;
    nsCOMPtr<nsIHTTPEventSink>  m_pEventSink;
    nsHTTPRequest*              m_pRequest;
    nsHTTPResponse*             m_pResponse;
    nsIStreamListener*          m_pResponseDataListener;
};

#endif /* _nsHTTPChannel_h_ */
