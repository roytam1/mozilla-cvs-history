/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef _nsIHTTPInstance_h_
#define _nsIHTTPInstance_h_

#include "nsIProtocolInstance.h"

/* 
    The nsIHTTPInstance class is the interface to an intance
    of the HTTP that acts on a per URL basis.

*/

class nsIHTTPInstance : public nsIProtocolInstance
{

public:
    
    /*
        Request functions-
        These functions set parameters on the outbound request and may only
        be set before a connect/open function gets called. Calling them after 
        a connect method will result in a NS_ERROR_ALREADY_CONNECTED
    */
    NS_IMETHOD          SetHeader(const char* i_Header, const char* i_Value)=0;

    /*
        Response functions-
        These function can read off the information from the available response
        of the HTTP Connection. Any call/attempt to read will implicitly call
        connect on this protocol instance.Thats why its not const.
    */
    NS_IMETHOD          GetHeader(const char* i_Header, const char* *o_Value)=0;

    static const nsIID& GetIID() { 
        // {843D1020-D0DF-11d2-B013-006097BFC036}
        static const nsIID NS_IHTTPInstance_IID = 
            { 0x843d1020, 0xd0df, 0x11d2, { 0xb0, 0x13, 0x0, 0x60, 0x97, 0xbf, 0xc0, 0x36 } };

		return NS_IHTTPInstance_IID; 
	};

};

//Possible errors
//#define NS_ERROR_ NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 400);

#endif /* _nsIHTTPInstance_h_ */
