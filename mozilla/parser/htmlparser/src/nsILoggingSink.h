/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */
#ifndef nsILoggingSink_h___
#define nsILoggingSink_h___

#include "nsIHTMLContentSink.h"
#include "nsString.h"
#include <iostream.h>

// IID for nsILoggingSink
#define NS_ILOGGING_SINK_IID \
 {0xa6cf9061, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

// Class IID for the logging sink
#define NS_LOGGING_SINK_IID \
 {0xa6cf9060, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsILoggingSink : public nsIHTMLContentSink {
public:
  NS_IMETHOD SetOutputStream(ostream& aStream) =0;
};

extern "C" nsresult NS_NewHTMLLoggingSink(nsIContentSink** aInstancePtrResult);

#endif /* nsILoggingSink_h___ */
